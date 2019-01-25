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
    dm=os.getenv("DAQFILE","NONE")
    print dm
    if (dm!="NONE"):
        self.daq_file=dm
        with open(dm) as data_file:    
            self.p_conf = json.load(data_file)
            print self.p_conf
  def jc_create(self):
    lcgi={}
    if (self.daq_url!=None):
        lcgi["url"]=self.daq_url
        if (self.login!="NONE"):
            lcgi["login"]=self.login
    else:
        if (self.daq_file!=None):
            lcgi["file"]=self.daq_file
    rep={}
    for x,y in self.p_conf["HOSTS"].iteritems():
        #print x,"  found"
        sr=executeFSM(x,9999,"LJC-%s" % x,"INITIALISE",lcgi)
    rep[x]=json.loads(sr)
    return json.dumps(rep)
            
  def jc_start(self):
    lcgi={}
    rep={}
    for x,y in self.p_conf["HOSTS"].iteritems():
        #print x,"  found"
        sr=executeFSM(x,9999,"LJC-%s" % x,"START",lcgi)
        rep[x]=json.loads(sr)
    return json.dumps(rep)
  def jc_kill(self):
    lcgi={}
    rep={}
    for x,y in self.p_conf["HOSTS"].iteritems():
        #print x," found"
        sr=executeFSM(x,9999,"LJC-%s" % x,"KILL",lcgi)
        rep[x]= json.loads(sr)
    return json.dumps(rep)        
  def jc_destroy(self):
    lcgi={}
    rep={}
    for x,y in self.p_conf["HOSTS"].iteritems():
        # print x," found"
        sr=executeFSM(x,9999,"LJC-%s" % x,"DESTROY",lcgi)
        rep[x]= json.loads(sr)
    return json.dumps(rep)  

  def jc_status(self):
    lcgi={}

    for x,y in self.p_conf["HOSTS"].iteritems():
        #print "HOST ",x
        
        sr=executeCMD(x,9999,"LJC-%s" % x,"STATUS",lcgi)
        sj=json.loads(sr)
        #print "ON A ",sj
        ssj=sj["answer"]["JOBS"]
        rep=""
        if (ssj != None):
        #print "\033[1m %6s %15s %25s %20s \033[0m" % ('PID','NAME','HOST','STATUS')
            for x in ssj:
                print x
                print "%6d %15s %25s %20s" % (x['PID'],x['NAME'],x['HOST'],x['STATUS'])
                rep =rep + "%6d %15s %25s %20s\n" % (x['PID'],x['NAME'],x['HOST'],x['STATUS'])
        else:
            rep="No Jobs"
    return rep
  def jc_restart(self,host,jobname,jobpid):
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


