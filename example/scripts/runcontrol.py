#!/usr/bin/env python
import os
import socks
import socket
import httplib, urllib,urllib2
from urllib2 import URLError, HTTPError
import json
from copy import deepcopy
import base64
import time
import argparse
import requests
import MongoJob as mg

sockport=None
sp=os.getenv("SOCKPORT","Not Found")
if (sp!="Not Found"):
    sockport=int(sp)
if (sockport !=None):
    print "Using SOCKPORT ",sockport
    socks.setdefaultproxy(socks.PROXY_TYPE_SOCKS5, "127.0.0.1", sockport)
    socket.socket = socks.socksocket
    #socks.wrapmodule(urllib2)

def parseReturn(command,sr,res=None):
    if (command=="jobStatus"):
        #s=r1.read()
        #print s["answer"]
        sj=json.loads(sr)
        #sj=s
        #ssj=json.loads(sj["answer"]["ANSWER"])
        ssj=sj["answer"]["ANSWER"]
        print "\033[1m %6s %15s %25s %20s \033[0m" % ('PID','NAME','HOST','STATUS')
        for x in ssj:
            if (x['DAQ']=='Y'):
                print "%6d %15s %25s %20s" % (x['PID'],x['NAME'],x['HOST'],x['STATUS'])
    if (command=="hvStatus"):
        sj=json.loads(sr.decode('latin-1').encode("utf-8"))
        device=sj["answer"]["STATUS"]["name"]
        ssj=sj["answer"]["STATUS"]["channels"]
        #print ssj

        if (device == "ISEG"):
            print "\033[1m %5s %10s %10s %10s %10s %10s %30s \033[0m" % ('Chan','VSET','ISET','VOUT','IOUT','RAMPUP',"STATUS")
            for x in ssj:
                print "#%.4d %10.2f %10.2f %10.2f %10.2f %10.2f %30s " % (x['id'],x['vset'],x['iset']*1E6,x['vout'],x['iout']*1E6,x['rampup'],x['status'][x['status'].find("=")+1:len(x['status'])-1])
        if (device == "SY1527"):
            print "\033[1m %5s %10s %10s %10s %10s %10s %10s \033[0m" % ('Chan','VSET','ISET','VOUT','IOUT','RAMPUP',"STATUS")
            for x in ssj:
                print "#%.4d %10.2f %10.2f %10.2f %10.2f %10.2f %10d " % (x['id'],x['vset'],x['iset'],x['vout'],x['iout'],x['rampup'],x['status'])
    if (command=="LVSTATUS"):
        sj=json.loads(sr)
        
        ssj=sj["answer"]["STATUS"]
        print "\033[1m %10s %10s %10s \033[0m" % ('VSET','VOUT','IOUT')
        print " %10.2f %10.2f %10.2f" % (ssj['vset'],ssj['vout'],ssj['iout'])
    if (command=="PTSTATUS"):
        sj=json.loads(sr)
        
        ssj=sj["answer"]["STATUS"]
        print "\033[1m %10s %10s  \033[0m" % ('P','T')
        print " %10.2f %10.2f " % (ssj['pressure'],ssj['temperature']+273.15)
    if (command=="HUMSTATUS"):
        sj=json.loads(sr)
        
        ssj=sj["answer"]["STATUS"]
        print "\033[1m %10s %10s %10s %10s  \033[0m" % ('H0','T0','H1','T1')
        print " %10.2f %10.2f %10.2f %10.2f " % (ssj['humidity0'],ssj['temperature0'],ssj['humidity11'],ssj['temperature1'])
    if (command=="PTCOR"):
        sj=json.loads(sr)
        
        ssj=sj["answer"]["STATUS"]
        print "\033[1m %10s %10s  \033[0m" % ('P','T')
        print " %10.2f %10.2f " % (ssj['pressure'],ssj['temperature']+273.15)
        p=ssj['pressure']
        t=ssj['temperature']+273.15
        if (res.v0==None):
	  print "V0 must be set"
	  return
        if (res.t0==None):
	  print "T0 must be set"
	  return
        if (res.p0==None):
	  print "P0 must be set"
	  return
	print "\033[1m Voltage to be set is\033[0m %10.2f" % (res.v0*p/res.p0*res.t0/t)
    if (command=="status" and not results.verbose):

        sj=json.loads(sr)
        ssj=sj["answer"]["diflist"]
        print "\033[1m %4s %5s %6s %12s %12s %15s  %s \033[0m" % ('DIF','SLC','EVENT','BCID','BYTES','SERVER','STATUS')

        for d in ssj:
            #print d
            #for d in x["difs"]:
            print '#%4d %5x %6d %12d %12d %15s %s ' % (d["id"],d["slc"],d["gtc"],d["bcid"],d["bytes"],d["host"],d["state"])
    if (command=="tdcstatus" and not results.verbose):

        sj=json.loads(sr)
        ssj=sj["answer"]["tdclist"]
        print "\033[1m %4s %6s %6s %12s %5s %5s \033[0m" % ('DIF','SLC','EVENT','BCID','DETID','TRIGS')

        for d in ssj:
            #print d
            #for d in x["difs"]:
            #print ((d["sourceid"]-10)/256,d["event"],d["gtc"],d["abcid"],d["detid"])
            for x in d:
                print '#%4d %6d %6d %12d %5d %5d ' % ((x["sourceid"]-10)/256,x["gtc"],x["event"],x["abcid"],x["detid"],x["triggers"])
    if (command=="dbStatus" ):
        sj=json.loads(sr)
        ssj=sj["answer"]
        print "\033[1m %10s %10s \033[0m" % ('Run','State')
        print " %10d %s " % (ssj['run'],ssj['state'])
    if (command=="shmStatus" ):
        sj=json.loads(sr)
        ssj=sj["answer"]
        print "\033[1m %10s %10s \033[0m" % ('Run','Event')
        print " %10d %10d " % (ssj['run'],ssj['event'])
    if (command=="state"):
        sj=json.loads(sr)
        print "\033[1m State \033[0m :",sj["STATE"]
        scm=""
        for z in sj["CMD"]:
            scm=scm+"%s:" % z["name"]
        scf=""
        for z in sj["FSM"]:
            scf=scf+"%s:" % z["name"]

        print "\033[1m Commands \033[0m :",scm
        print "\033[1m F S M \033[0m :",scf
    if (command=="triggerStatus"):
          
        sj=json.loads(sr)
        ssj=sj["answer"]["COUNTERS"]
        print "\033[1m %10s %10s %10s %10s %12s %12s %10s %10s %10s \033[0m" % ('Spill','Busy1','Busy2','Busy3','SpillOn','SpillOff','Beam','Mask','EcalMask')
        print " %10d %10d %10d %10d  %12d %12d %12d %10d %10d " % (ssj['spill'],ssj['busy1'],ssj['busy2'],ssj['busy3'],ssj['spillon'],ssj['spilloff'],ssj['beam'],ssj['mask'],ssj['ecalmask'])
    if (command=="difLog" or command=="cccLog" or command=="mdccLog" or command =="zupLog"):
          
        sj=json.loads(sr)
        print  "\033[1m %s \033[0m" % sj["answer"]["FILE"]
        ssj=sj["answer"]["LINES"]
        print ssj
        #print "\033[1m %10s %10s %10s %10s %12s %12s %10s %10s %10s \033[0m" % ('Spill','Busy1','Busy2','Busy3','SpillOn','SpillOff','Beam','Mask','EcalMask')
        #print " %10d %10d %10d %10d  %12d %12d %12d %10d %10d " % (ssj['spill'],ssj['busy1'],ssj['busy2'],ssj['busy3'],ssj['spillon'],ssj['spilloff'],ssj['beam'],ssj['mask'],ssj['ecalmask'])

        
def executeFSM(host,port,prefix,cmd,params):
   if (params!=None):
       myurl = "http://"+host+ ":%d" % (port)
       #conn = httplib.HTTPConnection(myurl)
       #if (name!=None):
       #    lq['name']=name
       #if (value!=None):
       #    lq['value']=value
       myurl = "http://"+host+ ":%d" % (port)
       #conn = httplib.HTTPConnection(myurl)
       lq={}
       
       lq["content"]=json.dumps(params,sort_keys=True)
       #for x,y in params.iteritems():
       #    lq["content"][x]=y
       lq["command"]=cmd           
       lqs=urllib.urlencode(lq)
       #print lqs
       saction = '/%s/FSM?%s' % (prefix,lqs)
       myurl=myurl+saction
       #print myurl
       req=urllib2.Request(myurl)
       r1=urllib2.urlopen(req)
       return r1.read()

def executeCMD(host,port,prefix,cmd,params):
   if (params!=None and cmd!=None):
       myurl = "http://"+host+ ":%d" % (port)
       #conn = httplib.HTTPConnection(myurl)
       #if (name!=None):
       #    lq['name']=name
       #if (value!=None):
       #    lq['value']=value
       myurl = "http://"+host+ ":%d" % (port)
       #conn = httplib.HTTPConnection(myurl)
       lq={}
       lq["name"]=cmd
       for x,y in params.iteritems():
           lq[x]=y
       lqs=urllib.urlencode(lq)
       saction = '/%s/CMD?%s' % (prefix,lqs)
       myurl=myurl+saction
       #1print myurl
       req=urllib2.Request(myurl)
       try:
           r1=urllib2.urlopen(req)
       except URLError, e:
           p_rep={}
           p_rep["STATE"]="DEAD"
           return json.dumps(p_rep,sort_keys=True)
       else:
           return r1.read()
       #proxies={"http":"socks5://127.0.0.1:2080"}
       #requests.get(myurl,timeout=24,proxies=proxies)
   else:
       myurl = "http://"+host+ ":%d/%s/" % (port,prefix)
       #conn = httplib.HTTPConnection(myurl)
       #print myurl
       req=urllib2.Request(myurl)
       try:
           r1=urllib2.urlopen(req)
       except URLError, e:
           p_rep={}
           p_rep["STATE"]="DEAD"
           return json.dumps(p_rep,sort_keys=True)
       else:
           return r1.read()
    
def executeRequest(surl):
    """
     Acces to an url

    :param surl: The url
    :return: url answer
    """
    req=urllib2.Request(surl)
    try:
        r1=urllib2.urlopen(req)
    except URLError, e:
        p_rep={}
        p_rep["STATE"]="DEAD"
        return json.dumps(p_rep,sort_keys=True)

    return r1.read()

#
# Check the configuration
#

class RCClient:
  """
  Handle all application definition and parameters
  """
  def __init__(self):
      self.p_conf=None
      self.daq_url=None
      self.daq_file=None
      #self.parseConfig()
      self.daqhost=None
      self.daqport=None
      self.trghost=None
      self.trgport=None
      self.anhost=None
      self.anport=None
      self.daq_par={}
      self.slow_par={}
      self.scurve_running=False
      self.login=None
      #print self.daqhost,self.daqport,self.daq_par
      #print self.slowhost,self.slowport

  def loadConfig(self,name=None,login=None):
      if (name == None):
          self.login=os.getenv("DAQLOGIN","NONE")
          useAuth=self.login!="NONE"
          self.dm=os.getenv("DAQURL","NONE")
      else:
          self.login=login
          self.dm=name
      self.parseConfig()
      for x,y in self.p_conf["HOSTS"].iteritems():
          for p in y:
              port=0
              
              for e in p["ENV"]:
                  if (e.split("=")[0]=="WEBPORT"):
                      port=int(e.split("=")[1])
              print p["NAME"]
              if (p["NAME"]=="RUNCONTROL"):
                  self.daqhost=x
                  self.daqport=port
                  if ("PARAMETER" in p):
                      self.daq_par.update(p["PARAMETER"])
                      if ("s_ctrlreg" in self.daq_par):
                          self.daq_par["ctrlreg"]=int(self.daq_par["s_ctrlreg"],16)
              
              if (p["NAME"]=="BUILDER"):
                  if ("PARAMETER" in p):
                      self.daq_par["builder"]=p["PARAMETER"]
              if (p["NAME"]=="TRIGGER"):
                  self.trghost=x
                  self.trgport=port
                  if ("PARAMETER" in p):
                      self.daq_par["trigger"]=p["PARAMETER"]
      print self.trghost,self.trgport
  def parseConfig(self):
    useAuth=self.login!=None
    dm=self.dm
    print dm
    if (dm!=None and dm!="NONE"):
        self.daq_url=dm
        read_conf=None
        if (useAuth):
            r = requests.get(dm, auth=(self.login.split(":")[0],self.login.split(":")[1]))
            read_conf=r.json()
        else:
            req=urllib2.Request(dm)
            try:
                r1=urllib2.urlopen(req)
            except URLError, e:
                print e
                p_rep={}
                return
            
            read_conf=json.loads(r1.read())
        #print read_conf
        if ("content" in read_conf):
            self.p_conf=read_conf["content"]
        else:
            self.p_conf=read_conf
        return
    dm=os.getenv("DAQFILE","NONE")
    print dm
    if (dm!="NONE"):
        self.daq_file=dm
        with open(dm) as data_file:    
            self.p_conf = json.load(data_file)
            print self.p_conf
        return
    dm=os.getenv("DAQMONGO","NONE")
    if (dm!="NONE"):
        sa=mg.instance()
        cn=dm.split(":")[0]
        cv=dm.split(":")[1]
        sa.downloadConfig(cn,int(cv),True)
        self.daq_mongo=dm
        daq_file="/dev/shm/mgjob/"+cn+"_"+cv+".json"
        with open(daq_file) as data_file:    
            self.p_conf = json.load(data_file)
        return
  def jc_create(self):
    """
    Loop on all computers defined in the configuration and initialise their job control
    with the loaded configuration

    :return: Dictionnary of answer to INITIALISE transition
    """  
    lcgi={}
    if (self.daq_url!=None):
        lcgi["url"]=self.daq_url
        if (self.login!="NONE"):
            lcgi["login"]=self.login
    else:
        if (self.daq_file!=None):
            lcgi["file"]=self.daq_file
        else:
            if (self.daq_mongo!=None):
                lcgi["mongo"]=self.daq_mongo
    rep={}
    for x,y in self.p_conf["HOSTS"].iteritems():
        #print x,"  found"
        sr=executeFSM(x,9999,"LJC-%s" % x,"INITIALISE",lcgi)
        rep[x]=json.loads(sr)
    return json.dumps(rep)
            
  def jc_start(self):
    """
    Loop on all computers defined in the configuration and start the process defined with the loaded configuration

    :return: Dictionnary of answer to START transition
    """  

    lcgi={}
    rep={}
    for x,y in self.p_conf["HOSTS"].iteritems():
        #print x,"  found"
        sr=executeFSM(x,9999,"LJC-%s" % x,"START",lcgi)
        rep[x]=json.loads(sr)
    return json.dumps(rep)

  def jc_kill(self):
    """
    Loop on all computers defined in the configuration and kill the process defined
    with the loaded configuration

    :return: Dictionnary of answer to KILL transition
    """
    lcgi={}
    rep={}
    for x,y in self.p_conf["HOSTS"].iteritems():
        #print x," found"
        sr=executeFSM(x,9999,"LJC-%s" % x,"KILL",lcgi)
        rep[x]= json.loads(sr)
    return json.dumps(rep)

  def jc_destroy(self):
    """
    Loop on all computers defined in the configuration and clean the loaded configuration

    :return: Dictionnary of answer to DESTROY transition
    """
    lcgi={}
    rep={}
    for x,y in self.p_conf["HOSTS"].iteritems():
        # print x," found"
        sr=executeFSM(x,9999,"LJC-%s" % x,"DESTROY",lcgi)
        rep[x]= json.loads(sr)
    return json.dumps(rep)  

  def jc_appcreate(self):
    """
    Loop on all computers defined in the configuration and send an APPCREATE Command, it sends a transition CREATE to all baseApplication

    :return: Dictionnary of answer to APPCREATE command
    """
    lcgi={}
    rep={}
    for x,y in self.p_conf["HOSTS"].iteritems():
        #print x,"  found"
        sr=executeCMD(x,9999,"LJC-%s" % x,"APPCREATE",lcgi)
        rep[x]=json.loads(sr)
    return json.dumps(rep)

  def jc_info(self,hostname,apname=None):
    """
    Pretty print of processes status and application settings

    :param hostanem: Host name where the jobcontrol is running
    :param appname: Application name , if None all application are dumped
    """
    lcgi={}
    resum={}
    rep=""
    for xh in [hostname]:
        #print "== HOST:  ==",xh
        rep = rep+ "== HOST: %s  == \n" % xh
        resum[xh]=[]
        sr=executeCMD(xh,9999,"LJC-%s" % xh,"STATUS",lcgi)
        sj=json.loads(sr)
        #print "== Acquisition %s ===\n \n == Processus ==\n" % sj['answer']['NAME']
        rep=rep+"== Acquisition %s ===\n \n == Processus ==\n" % sj['answer']['NAME']
        ssj=sj["answer"]["JOBS"]

        if (ssj != None):
            #print "== %6s %15s %25s %20s %20s ==" % ('PID','NAME','HOST','STATUS','PORT')
            rep=rep+"== %6s %15s %25s %20s %20s ==\n" % ('PID','NAME','HOST','STATUS','PORT')
            for x in ssj:
                #print x
                if (apname!=None and x['NAME']!=apname):
                    continue

                if ('PORT' in x):
                    srcmd=executeRequest("http://%s:%s/" % (x['HOST'],x['PORT']))
                    sjcmd=json.loads(srcmd)
                    #print sjcmd
                    resum[xh].append([x,sjcmd])
                else:
                    x['PORT']="0"
                #print "%6d %15s %25s %20s %20s " % (x['PID'],x['NAME'],x['HOST'],x['STATUS'],x['PORT'])
                rep =rep + "%6d %15s %25s %20s %20s\n" % (x['PID'],x['NAME'],x['HOST'],x['STATUS'],x['PORT'])
        else:
            rep=rep+"No Jobs"
    #print "== List of Applications  =="
    rep=rep+ "== List of Applications  ==\n"
    for xh,y in resum.iteritems():
        for x in y:
            # Dead process or non ZDAQ
            if ('PREFIX' not in x[1]):
                #print "================================================= \n == %s   running on %s PID %s is  %s == \n ==========================================\n " % (x[0]['NAME'],x[0]['HOST'],x[0]['PID'],x[0]['STATUS'])
                rep=rep+"================================================= \n == %s   running on %s PID %s is  %s == \n ==========================================\n " % (x[0]['NAME'],x[0]['HOST'],x[0]['PID'],x[0]['STATUS'])
                continue
            # normal process
            #print "== %s :== http://%s:%s/%s/ on %s status %s" % (x[0]['NAME'],x[0]['HOST'],x[0]['PORT'],x[1]['PREFIX'],x[0]['PID'],x[0]['STATUS'])
            rep=rep+"== %s :== http://%s:%s/%s/ on %s status %s\n" % (x[0]['NAME'],x[0]['HOST'],x[0]['PORT'],x[1]['PREFIX'],x[0]['PID'],x[0]['STATUS'])
            #print "\t == STATE :==",x[1]['STATE']
            rep=rep+"\t == STATE : %s == \n" % x[1]['STATE']
            rep=rep+ "\t == FSM :== "
            for z in x[1]['FSM']:
                rep=rep+" "+z['name']
            rep=rep+"\n"
            rep=rep+"\t == ALLOWED :== "
            for z in x[1]['ALLOWED']:
                rep=rep+" "+z['name']
            rep=rep+"\n"
            rep=rep+"\t == COMMAND :=="
            for z in x[1]['CMD']:
                rep=rep+" "+z['name']
            rep=rep+"\n"
            #print "\t == FSM PARAMETERS :=="
            rep=rep+"\t == FSM PARAMETERS :== \n"
            for z in x[1]['CMD']:
                if (z['name'] != "GETPARAM"):
                    continue
                srcmd1=executeRequest("http://%s:%s/%s/CMD?name=GETPARAM" % (x[0]['HOST'],x[0]['PORT'],x[1]['PREFIX']))
                sjcmd1=json.loads(srcmd1)
                #print sjcmd1
                if ('PARAMETER' in sjcmd1['answer']):
                    for xp,vp in sjcmd1['answer']['PARAMETER'].iteritems():
                        #print "\t \t  == %s : == %s " % (xp,vp)
                        rep=rep+ "\t \t  == %s : == %s \n" % (xp,vp)
            
    print rep
    return rep

  def jc_status(self,apname=None):
    """
    Pretty print of the status  of all process and application informations defined in the configuration

    :param appname: if set , only this application name is dump
    """
    lcgi={}
    resum={}
    rep=""
    for xh,y in self.p_conf["HOSTS"].iteritems():
      rep=rep+self.jc_info(xh,apname)
    return rep

  def jc_oldstatus(self,apname=None):
    """
    Same as jc_status 

    :obsolete: use jc_status instead
    """
    lcgi={}
    resum={}
    rep=""
    for xh,y in self.p_conf["HOSTS"].iteritems():
        print "== HOST:  ==",xh
        resum[xh]=[]
        sr=executeCMD(xh,9999,"LJC-%s" % xh,"STATUS",lcgi)
        sj=json.loads(sr)
        print "== Acquisition %s \n \n Processus \n==" % sj['answer']['NAME']
        ssj=sj["answer"]["JOBS"]

        if (ssj != None):
            print "== %6s %15s %25s %20s %20s ==" % ('PID','NAME','HOST','STATUS','PORT')
            for x in ssj:
                #print x
                if (apname!=None and x['NAME']!=apname):
                    continue
                if ('PORT' in x):
                    srcmd=executeRequest("http://%s:%s/" % (x['HOST'],x['PORT']))
                    sjcmd=json.loads(srcmd)
                    #print sjcmd
                    resum[xh].append([x,sjcmd])
                else:
                    x['PORT']="0"
                print "%6d %15s %25s %20s %20s " % (x['PID'],x['NAME'],x['HOST'],x['STATUS'],x['PORT'])
                rep =rep + "%6d %15s %25s %20s %20s\n" % (x['PID'],x['NAME'],x['HOST'],x['STATUS'],x['PORT'])
        else:
            rep="No Jobs"
    print "== List of Applications  ==" 
    for xh,y in resum.iteritems():
        for x in y:
            # Dead process or non ZDAQ
            if ('PREFIX' not in x[1]):
                print "================================================= \n == %s   running on %s PID %s is  %s == \n ==========================================\n " % (x[0]['NAME'],x[0]['HOST'],x[0]['PID'],x[0]['STATUS'])
                continue
            # normal process
            print "== %s :== http://%s:%s/%s/ on %s status %s" % (x[0]['NAME'],x[0]['HOST'],x[0]['PORT'],x[1]['PREFIX'],x[0]['PID'],x[0]['STATUS'])
            print "\t == STATE :==",x[1]['STATE']
            rep="\t == FSM :=="
            for z in x[1]['FSM']:
                rep=rep+" "+z['name']
            print rep
            rep="\t == ALLOWED :=="
            for z in x[1]['ALLOWED']:
                rep=rep+" "+z['name']
            print rep
            rep="\t == COMMAND :=="
            for z in x[1]['CMD']:
                rep=rep+" "+z['name']
            print rep
            print "\t == FSM PARAMETERS :=="
            for z in x[1]['CMD']:
                if (z['name'] != "GETPARAM"):
                    continue
                srcmd1=executeRequest("http://%s:%s/%s/CMD?name=GETPARAM" % (x[0]['HOST'],x[0]['PORT'],x[1]['PREFIX']))
                sjcmd1=json.loads(srcmd1)
                #print sjcmd1
                if ('PARAMETER' in sjcmd1['answer']):
                    for xp,vp in sjcmd1['answer']['PARAMETER'].iteritems():
                        print "\t \t  == %s : == %s " % (xp,vp)
            

    return rep

  def jc_restart(self,host,jobname,jobpid):
    """
    Kill and restart one process

    :param host: Host name
    :param jobname: Name of the process in the job control
    :param pid: Process id
    """
    lcgi={}
    lcgi["processname"]=jobname
    lcgi["pid"]=jobpid
    sr=executeCMD(host,9999,"LJC-%s" % host,"RESTARTJOB",lcgi)
    print sr
    
  def daq_create(self):
      lcgi={}
      rep={}
      if (self.daq_url!=None):
          lcgi["url"]=self.daq_url
          if (self.login!="NONE"):
            lcgi["login"]=self.login
      else:
          if (self.daq_file!=None):
              lcgi["file"]=self.daq_file
          else:
              if (self.daq_mongo!=None):
                  lcgi["mongo"]=self.daq_mongo
      for x,y in self.p_conf["HOSTS"].iteritems():
          for p in y:
              if (p["NAME"] != "RUNCONTROL"):
                  continue;
              print x,p["NAME"]," process found"
              port=0
              for e in p["ENV"]:
                  if (e.split("=")[0]=="WEBPORT"):
                      port=int(e.split("=")[1])
              if (port==0):
                  continue
              p_rep={}
              surl="http://%s:%d/" % (x,port)
              req=urllib2.Request(surl)
              try:
                  r1=urllib2.urlopen(req)
                  p_rep=json.loads(r1.read())
              except URLError, e:
                  print surl,e
                  p_rep={}
              print x,port,p["NAME"],p_rep
              if ("STATE" in p_rep):
                  if (p_rep["STATE"]=="VOID"):
                      sr=executeFSM(x,port,p_rep["PREFIX"],"CREATE",lcgi)
                      #print sr
  def daq_list(self):
      lcgi={}
      rep=""
      if (self.daq_url!=None):
          lcgi["url"]=self.daq_url
      else:
          if (self.daq_file!=None):
              lcgi["file"]=self.daq_file
      for x,y in self.p_conf["HOSTS"].iteritems():
          print "HOST ",x
          rep=rep+" Host %s \n" % x
          print "\033[1m %12s %12s %8s %8s %20s \033[0m" % ('NAME','INSTANCE','PORT','PID','STATE')
          rep=rep+"\033[1m %12s %12s %8s %8s %20s \033[0m\n" % ('NAME','INSTANCE','PORT','PID','STATE')
          for p in y:
              #print x,p["NAME"]," process found"
              port=0
              for e in p["ENV"]:
                  if (e.split("=")[0]=="WEBPORT"):
                      port=int(e.split("=")[1])
              if (port==0):
                  continue
              p_rep={}
              surl="http://%s:%d/" % (x,port)
              req=urllib2.Request(surl)
              try:
                  r1=urllib2.urlopen(req)
                  p_rep=json.loads(r1.read())
                  print "%12s %12s %8d %8d %20s" % (p["NAME"],p_rep["PREFIX"],port,p_rep["PID"],p_rep["STATE"])
                  rep =rep +"%12s %12s %8d %8d %20s\n" % (p["NAME"],p_rep["PREFIX"],port,p_rep["PID"],p_rep["STATE"])
              except URLError, e:
                  print surl,e
                  p_rep={}
      return rep
              
  def daq_info(self,name):
      lcgi={}
      rep=""
      if (self.daq_url!=None):
          lcgi["url"]=self.daq_url
      else:
          if (self.daq_file!=None):
              lcgi["file"]=self.daq_file
      for x,y in self.p_conf["HOSTS"].iteritems():
          print "HOST ",x
          rep=rep+" Host %s \n" % x
          print "\033[1m %12s %12s %8s %8s %20s \033[0m" % ('NAME','INSTANCE','PORT','PID','STATE')
          rep=rep+"\033[1m %12s %12s %8s %8s %20s \033[0m\n" % ('NAME','INSTANCE','PORT','PID','STATE')
          for p in y:
              #print x,p["NAME"]," process found"
              port=0
              for e in p["ENV"]:
                  if (e.split("=")[0]=="WEBPORT"):
                      port=int(e.split("=")[1])
              if (port==0):
                  continue
              p_rep={}
              surl="http://%s:%d/" % (x,port)
              req=urllib2.Request(surl)
              try:
                  r1=urllib2.urlopen(req)
                  p_rep=json.loads(r1.read())
                  if (p["NAME"]!=name):
                      continue
                  print p
                  print "%12s %12s %8d %8d %20s" % (p["NAME"],p_rep["PREFIX"],port,p_rep["PID"],p_rep["STATE"])
                  rep =rep +"%12s %12s %8d %8d %20s\n" % (p["NAME"],p_rep["PREFIX"],port,p_rep["PID"],p_rep["STATE"])
              except URLError, e:
                  print surl,e
                  p_rep={}
      return rep
              
  def daq_discover(self):
      lcgi={}
      sr=executeFSM(self.daqhost,self.daqport,"RunControl","DISCOVER",lcgi)
      return sr
      
     
  def daq_setparameters(self):
      lcgi={}
      lcgi["PARAMETER"]=json.dumps(self.daq_par,sort_keys=True)
      
      sr=executeCMD(self.daqhost,self.daqport,"RunControl","SETPARAM",lcgi)
      print sr
      
  def daq_getparameters(self):
      lcgi={}
      sr=executeCMD(self.daqhost,self.daqport,"RunControl","GETPARAM",lcgi)
      print sr
  def daq_forceState(self,name):
      lcgi={}
      lcgi["state"]=name
      sr=executeCMD(self.daqhost,self.daqport,"RunControl","FORCESTATE",lcgi)
      print sr
      
  def daq_initialise(self):
      lcgi={}
      sr=executeFSM(self.daqhost,self.daqport,"RunControl","INITIALISE",lcgi)
      rep=json.loads(sr)
      #print rep
      #print "COUCOU"
      return json.dumps(rep)

  def daq_configure(self):
      lcgi=self.daq_par
      sr=executeFSM(self.daqhost,self.daqport,"RunControl","CONFIGURE",lcgi)
      rep=json.loads(sr)
      return json.dumps(rep)

  def daq_start(self,runid=1234):
      lcgi={}
      lcgi["runid"]=runid
      srs=executeFSM(self.daqhost,self.daqport,"RunControl","START",lcgi)
      rep=json.loads(srs)
      #self.daq_setrunheader(0,0)
      return json.dumps(rep)

  def daq_stop(self):
      lcgi={}
      sr=executeFSM(self.daqhost,self.daqport,"RunControl","STOP",lcgi)
      rep=json.loads(sr)

      return json.dumps(rep)
 
  def daq_halt(self):
      lcgi={}
      sr=executeFSM(self.daqhost,self.daqport,"RunControl","HALT",lcgi)
      rep=json.loads(sr)
      return json.dumps(sr)

  def daq_status(self):
      lcgi={}
      sr=executeCMD(self.daqhost,self.daqport,"RunControl","STATUS",lcgi)
      return sr


  def daq_evb_status(self):
      lcgi={}
      sr=executeCMD(self.daqhost,self.daqport,"RunControl","BUILDERSTATUS",lcgi)
      return sr

  def daq_ds_status(self):
      lcgi={}
      sr=executeCMD(self.daqhost,self.daqport,"RunControl","EXSERVERSTATUS",lcgi)
      return sr    
     
      
  def daq_state(self):
      p_rep={}
      state="UNKNOWN"
      req=urllib2.Request("http://%s:%d/" % (self.daqhost,self.daqport))
      try:
         r1=urllib2.urlopen(req)
         p_rep=json.loads(r1.read())
      except URLError, e:
         print "no connection to DAQ"
         return "NO CONNECTION"
         exit(0)
      if ("STATE" in p_rep):
         #print p_rep["STATE"]
         return p_rep["STATE"]
         

  def daq_process(self):
      lcgi={}
      sr=executeCMD(self.daqhost,self.daqport,"RunControl","LISTPROCESS",lcgi)
      rep =json.loads(sr)
      return json.dumps(rep)

  def trg_period(self,n=1000000):
      lcgi={}
      lcgi["period"]=n
      sr=executeCMD(self.trghost,self.trgport,"softTrigger-0","PERIOD",lcgi)
      rep =json.loads(sr)
      return json.dumps(rep)

  def trg_size(self,n=64):
      lcgi={}
      lcgi["size"]=n
      sr=executeCMD(self.trghost,self.trgport,"softTrigger-0","SIZE",lcgi)
      rep =json.loads(sr)
      return json.dumps(rep)

  def trg_status(self):
      lcgi={}
      sr=executeCMD(self.trghost,self.trgport,"softTrigger-0","STATUS",lcgi)
      rep =json.loads(sr)
      return json.dumps(rep["answer"]["answer"])


