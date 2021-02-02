from __future__ import absolute_import
from __future__ import print_function
import zrcfsm
import MongoJob as mg
import json
import time
import os
from transitions import Machine,State
class daqControl:
    def __init__(self,account,config):
        self.account=account
        self.config=config
        self.jobcontrols=[]
        self.appMap={}
        # DB access
        self.db=mg.instance()
        #Job control
        self.jc=Machine(model='self',states=['VOID','INITIALISED','RUNNING','CONFIGURED'],initial='VOID')
        self.jc.add_transition('initialise','VOID','INITIALISED',after=self.jc_initialising)
        self.jc.add_transition('start','INITIALISED','RUNNING',after=self.jc_starting)
        self.jc.add_transition('kill',['RUNNING','CONFIGURED'],'INITIALISED',after=self.jc_killing)
        self.jc.add_transition('configure','RUNNING','CONFIGURED',after=self.jc_appcreate)
        self.jc.add_transition('destroy','INITIALISED','VOID',after=self.jc_destroying)
        self.job_answer=None
        #DAQ PART
        self.daq_answer=None
        self.daqfsm=Machine(model=self,states=['VOID','INITIALISED','CONFIGURED','RUNNING','CONFIGURED'],initial='VOID')
        self.daqfsm.add_transition('initialise','VOID','INITIALISED',after='daq_initialising',conditions='isConfigured')
        self.daqfsm.add_transition('configure',['INITIALISED','CONFIGURED'],'CONFIGURED',after='daq_configuring',conditions='isConfigured')
        self.daqfsm.add_transition('start','CONFIGURED','RUNNING',after='daq_starting',conditions='isConfigured')
        self.daqfsm.add_transition('stop','RUNNING','CONFIGURED',after='daq_stopping',conditions='isConfigured')
        self.daqfsm.add_transition('destroy','CONFIGURED','VOID',after='daq_destroying',conditions='isConfigured')

        self.stored_state=self.getStoredState()
    def fsmStatus(self):
        x=self.stored_state
        print(" FSM Status:",x["name"],x["version"],x["location"],time.ctime(x["time"]),x["job"],x["daq"])
    def getStoredState(self):
        self.config_name=self.config.split(":")[0]
        self.config_version=int(self.config.split(":")[1])
        self.daq_setup=os.getenv('DAQSETUP')
        if (self.daq_setup==None):
            self.jc.to_VOID()
            self.to_VOID()
            return None
        rep=self.db.fsmInfo(self.config_name,self.config_version,self.daq_setup)
        if (rep==None):
            self.jc.to_VOID()
            self.to_VOID()
            return None
        else:
            if (rep['job']!="NOTSET"):
                self.jc.state=rep['job']
            else:
                self.jc.to_VOID()
            if (rep['daq']!="NOTSET"):
                self.state=rep['daq']
            else:
                self.to_VOID()   
            return rep
        return None

    def storeState(self):
        # Force DAQ state to VOID if job control is not CONFIGURED
        dstate="VOID"
        if (self.jc.state == "CONFIGURED"):
            dstate=self.state
        self.db.setFsmInfo(self.config_name,self.config_version,self.daq_setup,job=self.jc.state,daq=dstate)
        self.stored_state=self.getStoredState()
        self.fsmStatus()
        return

    def isConfigured(self):
        self.getStoredState()
        return (self.jc.state=="CONFIGURED" )
    
    def parseMongo(self):
        self.db.downloadConfig(self.config.split(":")[0],int(self.config.split(":")[1]),True)
        daq_file="/dev/shm/mgjob/"+self.config.split(":")[0]+"_"+self.config.split(":")[1]+".json"
        with open(daq_file) as data_file:    
            self._mgConfig = json.load(data_file)

            
    def getLog(self,host,pid):
         for x in self.jobcontrols:
             if ( x.host!=host):
                 continue
             par={}
             par["pid"]=pid
             par["lines"]=500
             s=x.sendCommand("JOBLOG",par)
             if (type(s) is bytes):
                 s=s.decode("utf-8")
             return s
    def discover(self):
        if (not "HOSTS" in self._mgConfig):
            return
        self.appMap={}
        self.jobcontrols=[]
        mh = self._mgConfig['HOSTS'];
        for  key,value in mh.items():
            #print "Host found %s" % key
            fsm = zrcfsm.FSMAccess(key, 9999);
            self.jobcontrols.append(fsm)
    
        for x in self.jobcontrols:
            x.getInfo()
            if (x.state == "FAILED"):
                print("Failed request %s exiting" % x.url)
                exit(0)
      
            s = x.sendCommand("STATUS",{})
            if (type(s) is bytes):
                s=s.decode("utf-8")

            m = json.loads(s)
            #print s
            if (not 'JOBS' in m['answer']):
                print("%s has NO Jobs : %s" % (x.url,s))
            else:
                if (m['answer']['JOBS'] != None):
                    for  pcs in m['answer']['JOBS']:
                        if (pcs['STATUS'].split(' ')[0] == 'X'):
                            #print(pcs)
                            print("\t \t FATAL Host %s Port %d PID %d is DEAD" % (pcs['HOST'], int(pcs['PORT']),pcs['PID'] ))

                            continue
          
                        bapp = zrcfsm.FSMAccess(pcs['HOST'], int(pcs['PORT']))
                        bapp.getInfo();
                        #print(bapp.state)
                        if (bapp.state == "DEAD"):
                            print("Host %s Port %d Name %s is DEAD" % (pcs['HOST'], int(pcs['PORT']),bapp.infos['name'] ))
                        else:   
                            if (not bapp.infos['name'] in self.appMap):
                                l=[]
                                l.append(bapp)
                                self.appMap[bapp.infos['name']]=l
                            else:
                                self.appMap[bapp.infos['name']].append(bapp) 
    
    def updateInfo(self,printout,vverbose):
        if (printout):
            print("""
        \t \t *****************************    
        \t \t ** Application information **
        \t \t *****************************
        """)
        if (len(self.appMap)==0):
            print("No Application Map found. Please Connect first or create process")
        for k,v in self.appMap.items():
            for x in v:
                x.getInfo()
                if (printout):
                    x.printInfos(vverbose)

    def getAllInfos(self):
        summary=[]
        if (len(self.appMap)==0):
            self.discover()
        for k,v in self.appMap.items():
            for x in v:
                x.getInfo()
                summary.append(x.allInfos())
        return json.dumps(summary,sort_keys=True)
    def processCommand(self,cmd,appname,param):
        r={}
        if (not appname in self.appMap):
            return '{"answer":"invalidname","status":"FAILED"}'
        for x in self.appMap[appname]:
            rep=x.sendCommand(cmd,param)
            if (type(rep) is bytes):
                rep=rep.decode("utf-8")
            s=json.loads(rep)
            r["%s_%d" % (appname,x.appInstance)]=s
        return json.dumps(r)
  
    # JOB Control
    def jc_transition(self,Transition,par):
        rep={}
        if (len(self.jobcontrols)==0):
            print("No jobcontrols found. Please Connect first")
            exit(0)
        for x in self.jobcontrols:
            print("Calling",Transition,par)
            ans=x.sendTransition(Transition,par)
            if (type(ans) is bytes):
                ans=ans.decode("utf-8")
            rep["%s" % x.host] = json.loads(ans)
        return json.dumps(rep)
    
    def jc_command(self,Command,par):
        rep={}
        if (len(self.jobcontrols)==0):
            print("No jobcontrols found. Please Connect first")
            exit(0)
        for x in self.jobcontrols:
            ans=x.sendCommand(Command,par)
            if (type(ans) is bytes):
                ans=ans.decode("utf-8")

            rep["%s" % x.host] = json.loads(ans)
        return json.dumps(rep)

    def jc_status(self):
        return self.jc_command("STATUS",{})

    # Transition
    def jc_initialising(self):
        par={}
        par['mongo']=self.config
        print("Initialising",par)
        self.job_answer= self.jc_transition("INITIALISE",par)
        self.storeState()
    def jc_starting(self):
        self.job_answer= self.jc_transition("START",{})
        self.storeState()
    def jc_killing(self):
        self.job_answer= self.jc_transition("KILL",{})
        self.storeState()
    def jc_destroying(self):
        self.job_answer==self.jc_transition("DESTROY",{})
        self.appMap={}
        self.storeState()
    def jc_appcreate(self):
        self.job_answer= self.jc_command("APPCREATE",{})    
        self.storeState()
