from __future__ import absolute_import
from __future__ import print_function
import os
import socks
import socket
import six.moves.http_client
import six.moves.urllib.request, six.moves.urllib.parse, six.moves.urllib.error
import six.moves.urllib.request, six.moves.urllib.error, six.moves.urllib.parse
from six.moves.urllib.error import URLError, HTTPError
import json
from copy import deepcopy
import base64
import time
import requests
import six
try:
    from urllib.parse import urlparse
except ImportError:
     from six.moves.urllib.parse import urlparse
# Sock support
sockport = None
sp = os.getenv("SOCKPORT", "Not Found")
if (sp != "Not Found"):
    sockport = int(sp)
if (sockport != None):
    # print "Using SOCKPORT ",sockport
    socks.setdefaultproxy(socks.PROXY_TYPE_SOCKS5, "127.0.0.1", sockport)
    socket.socket = socks.socksocket
    # socks.wrapmodule(urllib2)


class FSMAccess:
    def __init__(self, vhost, vport):
        """
        Handle all application definition and p  arameters , It controls the acquisition via the FDAQ application and the Slow control via the FSLOW application
        """
        self.host = vhost
        self.port = vport
        self.url = "http://%s:%d" % (vhost, vport)
        self.getInfo()
        self.appType = 'UNKNOWN'
        self.appInstance = 0
        self.infos = {}
        self.procInfos = {}

    def executeRequest(self, url):
        """
         Acces to an url

        :param surl: The url
        :return: url answer
        """
        #print "Access to %s " % url
        req = six.moves.urllib.request.Request(url)
        try:
            r1 = six.moves.urllib.request.urlopen(req)
        except URLError as e:
            p_rep = {}
            p_rep["STATE"] = "DEAD"
            return json.dumps(p_rep, sort_keys=True)

        return r1.read()

    def executeFSM(self,host,port,prefix,cmd,params):
        """
        Access to the FSM of a zdaq::baseApplication
   
        :param host: Host name
        :param port: Application port
        :param prefix: Prefix of the application , ie, http:://host:port/prefix/.....
        :param cmd: Transition
        :param params: Value of the content tag
   
        :return: url answer
        """
        if (params!=None):
            myurl = "http://"+host+ ":%d" % (port)
            lq={}
       
            lq["content"]=json.dumps(params,sort_keys=True)

            lq["command"]=cmd           
            lqs=six.moves.urllib.parse.urlencode(lq)
            saction = '/%s/FSM?%s' % (prefix,lqs)
            myurl=myurl+saction
            #print myurl
            req=six.moves.urllib.request.Request(myurl)

            r1=six.moves.urllib.request.urlopen(req)

            return r1.read()
        else:
            return '{"STATE"="FAILED","STATUS"="No Params given"}'
        

    def executeCMD(self,host,port,prefix,cmd,params):
        """
        Access to the CoMmanDs of a zdaq::baseApplication
        
        :param host: Host name
        :param port: Application port
        :param prefix: Prefix of the application , ie, http:://host:port/prefix/.....
        :param cmd: Command name
        :param params: CGI additional parameters
        :return: url answer
        """

        if (params!=None and cmd!=None):
            myurl = "http://"+host+ ":%d" % (port)

            lq={}
            lq["name"]=cmd
            for x,y in six.iteritems(params):
                lq[x]=y
            lqs=six.moves.urllib.parse.urlencode(lq)
            saction = '/%s/CMD?%s' % (prefix,lqs)
            myurl=myurl+saction

            req=six.moves.urllib.request.Request(myurl)
            try:
                r1=six.moves.urllib.request.urlopen(req)
            except URLError as e:
                p_rep={}
                p_rep["STATE"]="DEAD"
                return json.dumps(p_rep,sort_keys=True)
            else:
                return r1.read()

        else:
            myurl = "http://"+host+ ":%d/%s/" % (port,prefix)
            req=six.moves.urllib.request.Request(myurl)
            try:
                r1=six.moves.urllib.request.urlopen(req)
            except URLError as e:
                p_rep={}
                p_rep["STATE"]="DEAD"
                return json.dumps(p_rep,sort_keys=True)
            else:
                return r1.read()
    
    def getProcInfo(self):

        sr = self.executeRequest(self.url)
        #print("Avant ",sr,type(sr))
        if (type(sr) is bytes):
            sr=sr.decode("utf-8")
        #print(sr)
        self.procInfos = json.loads(sr)
        if (self.procInfos['STATE'] !="DEAD"): 
            self.pid = self.procInfos['PID']
            self.prefix = self.procInfos['PREFIX']
        self.fUrl = "http://%s:%d/%s" % (self.host, self.port, self.prefix)
        self.state = self.procInfos['STATE']

    def getInfo(self):
        self.getProcInfo()
        if (self.state == "DEAD"):
           return
        if (self.pid < 0):
            return
        if (self.isBaseApplication(self.procInfos)):
            sinf = self.sendCommand('INFO', {})
            if (type(sinf) is bytes):
                sinf=sinf.decode("utf-8")

            self.infos = json.loads(sinf)['answer']['INFO']
            self.appType = self.infos['name']
            self.appInstance = self.infos['instance']
            spar = self.sendCommand('GETPARAM', {})
            if (type(spar) is bytes):
                spar=spar.decode("utf-8")

            jpar=json.loads(spar)
            if ('PARAMETER' in jpar['answer']):
                self.params = json.loads(spar)['answer']['PARAMETER']
            else:
                self.params={}
    def allInfos(self):
        jdict={}
        jdict['infos']=self.infos
        jdict['params']=self.params
        jdict['process']=self.procInfos
        return jdict
    def isBaseApplication(self, m):
        base = False
        for key, value in m.items():
            if (key == 'CMD'):
                for x in value:
                    if (x['name'] == 'GETPARAM'):
                        base = True
        return base

    def sendCommand(self, name, content):
        self.getProcInfo()
        isValid = False
        for key, value in self.procInfos.items():
            if (key == 'CMD'):
                for x in value:
                    if (x['name'] == name):
                        isValid = True
        if (not isValid):
            return '{"answer":"invalid","status":"FAILED"}'
        
        #luri = "%s/CMD?name=%s" % (self.fUrl, name)
        #if (content != None):
        #    for key, value in content.items():
        #        luri = luri + "&%s=%s" % (key, value)

        #rep = self.executeRequest(luri)
        #return rep
        rep=self.executeCMD(self.host,self.port,self.prefix,name,content)
        if (type(rep) is bytes):
            rep=rep.decode("utf-8")
        return rep
    def sendTransition(self, name, content):
        self.getProcInfo()
        #print "Send Transition",self.procInfos
        isValid = False
        for key, value in self.procInfos.items():
            if (key == 'ALLOWED'):
                for x in value:
                    if (x['name'] == name):
                        isValid = True
        if (not isValid):
            return '{"answer":"invalid","status":"FAILED"}'

        #luri = "%s/FSM?command=%s&content=%s" % (
        #    self.fUrl, name, json.dumps(content))
        #rep = self.executeRequest(luri)
        #return rep
        rep=self.executeFSM(self.host,self.port,self.prefix,name,content)
        if (type(rep) is bytes):
            rep=rep.decode("utf-8")
        return rep

    def printInfos(self, vverb):
        if (vverb):
            #print "PROCINFO ",self.procInfos
            print("\n FSM is %s on %s, PID %d Service %s" % (self.procInfos["STATE"], self.url, self.procInfos["PID"], self.procInfos["PREFIX"]))
            # print COMMAND and TRANSITION
            for k, v in self.procInfos.items():
                if (k == 'ALLOWED' or k == 'CMD' or k == 'FSM'):
                    s = " \t %s \t" % k
                    for x in v:
                        s = s + x['name'] + " "
                    print(s)
            print("\n BaseApplication %s _ %d" % (self.infos["name"], self.infos["instance"]))
            print("\t Parameters")
            for k, v in self.params.items():
                print("\t \t", k, v)
        else:
            print("FSM is %s on %s, PID %d Service %s"  % (self.procInfos["STATE"], self.url, self.procInfos["PID"], self.procInfos["PREFIX"]))
            
