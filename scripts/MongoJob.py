#!/usr/bin/env python3
import os
from pymongo import MongoClient
import json
from bson.objectid import ObjectId
import time





def IP2Int(ip):
    """
    convert IP adress string to int

    :param IP: the IP address
    :return: the encoded integer 
    """
    o = list(map(int, ip.split('.')))
    res = (16777216 * o[3]) + (65536 * o[2]) + (256 * o[1]) + o[0]
    return res


class MongoJob:
    """
    Main class to access the Mongo DB 
    """

    def __init__(self, host,port,dbname,username,pwd):
        """
        connect Mongodb database 

        :param host: Hostanme of the PC running the mongo DB

        :param port: Port to access the base

        :param dbname: Data base name

        :param username: Remote access user

        :param pwd: Remote access password

        """
      
        self.connection=MongoClient(host,port)
        self.db=self.connection[dbname]
        self.db.authenticate(username,pwd)

    def reset(self):
        """
        Reset connection to download another configuration
        """
        self.bson_id=[] 
    def uploadConfig(self,name,fname,comment,version=1):
        """
        jobcontrol configuration upload

        :param name: Name of the configuration
        :param fname: File name to upload
        :param comment: A comment on the configuration
        :param version: The version of the configuration
        """
        s={}
        s["content"]=json.loads(open(fname).read())
        s["name"]=name
        s["time"]=time.time()
        s["comment"]=comment
        s["version"]=version
        resconf=self.db.configurations.insert_one(s)
        print(resconf)

    def configurations(self):
        """
        List all the configurations stored
        """
        cl=[]
        res=self.db.configurations.find({})
        for x in res:
            if ("comment" in x):
                print(time.ctime(x["time"]),x["version"],x["name"],x["comment"])
                cl.append((x["name"],x['version'],x['comment']))
        return cl
    def runs(self):
        """
        List all the run informations stored
        """
        res=self.db.runs.find({})
        for x in res:
            if ("run" in x):
                if ("comment" in x and "time" in x):
                    print(time.ctime(x["time"]),x["location"],x["run"],x["comment"])
                else:
                    if ("run" in x):
                        print(x["location"],x["run"],x["comment"])
                #print(x["time"],x["location"],x["run"],x["comment"])
    def runInfo(self,run,loc):
        """
        Eun info on a given run
        """
        res=self.db.runs.find({"run":run,"location":loc})
        for x in res:

            if ("comment" in x):
                print(x["time"],x["location"],x["run"],x["comment"])
            return x
        return None

    def setFsmInfo(self,name,version,location,job=None,daq=None):
        """
        fsm info insertion
        :param name: Configuration name
        :param version: Configuration version
        :param location: Setup name
        :param job: job state name or none
        :param daq: daq state name or none
        """
        s=self.fsmInfo(name,version,location)
        if (s == None):
            s={}
            s["name"]=name
            s["location"]=location
            s["version"]=version
            s["job"]="NOTSET"
            s["daq"]="NOTSET"
        else:
            del s["_id"]
        s["time"]=time.time()
        if (job!=None):
            s["job"]=job
        if (daq!=None):
            s["daq"]=daq
        resconf=self.db.fsm.insert_one(s)
        print(resconf)

    def fsmInfo(self,name,version,location):
        """
        Get FSM's information for a given configuration and setup
        :param name: Configuration name
        :param version: Configuration version
        :param location: Setup name
        """
        res=self.db.fsm.find({"name":name,"version":version,"location":location})
        last={}
        last["time"]=0
        for x in res:
            #print(x["name"],x["version"],x["location"],x["time"],x["job"],x["daq"])
            if (x["time"]>last["time"]):
                last=x
        if (last["time"]!=0):
            return last
        else:
            return None

    def fsms(self):
        """
        Get FSM's informations dump
        """
        res=self.db.fsm.find({})
        for x in res:
            print(x["name"],x["version"],x["location"],time.ctime(x["time"]),x["job"],x["daq"])

    def downloadConfig(self,cname,version,toFileOnly=False):
        """
        Download a jobcontrol configuration to /dev/shm/mgjob/ directory
        
        :param cname: Configuration name
        :param version: Configuration version
        :param toFileOnly:if True and /dev/shm/mgjob/cname_version.json exists, then it exits
        """
        os.system("mkdir -p /dev/shm/mgjob")
        fname="/dev/shm/mgjob/%s_%s.json" % (cname,version)
        if os.path.isfile(fname) and toFileOnly:
            #print('%s already download, Exiting' % fname)
            return
        res=self.db.configurations.find({'name':cname,'version':version})
        for x in res:
            print(x["name"],x["version"],x["comment"])
            #var=raw_input()
            slc=x["content"]
            os.system("mkdir -p /dev/shm/mgjob")
            fname="/dev/shm/mgjob/%s_%s.json" % (cname,version)
            f=open(fname,"w+")
            f.write(json.dumps(slc, indent=2, sort_keys=True))
            f.close()
            return slc
        
    def getRun(self,location,comment="Not set"):
        """
        Get a new run number for a given setup

        :param location: Setup Name
        :param comment: Comment on the run
        :return: a dictionnary corresponding to the base insertion {run,location,time,comment}
        """
        res=self.db.runs.find({'location':location})
        runid={}
        for x in res:
            #print(x["location"],x["run"],x["comment"])
            #var=raw_input()
            runid=x
        if ("location" in runid):
            runid["run"]=runid["run"]+1
            del runid["_id"]
        else:
            runid["run"]=1000
            runid["location"]=location
        runid["time"]=time.time()
        runid["comment"]=comment
        os.system("mkdir -p /dev/shm/mgjob")
        fname="/dev/shm/mgjob/lastrun.json"
        f=open(fname,"w+")
        f.write(json.dumps(runid, indent=2, sort_keys=True))
        f.close()
        resconf=self.db.runs.insert_one(runid)
        print(resconf)
        return runid
 
     
def instance():
    """
    Create a MongoJob Object
    
    The ENV varaible MGDBLOGIN=user/pwd@host:port@dbname mut be set

    :return: The MongoJob Object
    """
    # create the default access
    login=os.getenv("MGDBLOGIN","NONE")
    if (login == "NONE"):
        print("The ENV varaible MGDBLOGIN=user/pwd@host:port@dbname mut be set")
        exit(0)
    userinfo=login.split("@")[0]
    hostinfo=login.split("@")[1]
    dbname=login.split("@")[2]
    user=userinfo.split("/")[0]
    pwd=userinfo.split("/")[1]
    host=hostinfo.split(":")[0]
    port=int(hostinfo.split(":")[1])
    print(host,port,dbname,user,pwd)
    _wdd=MongoJob(host,port,dbname,user,pwd)
    print("apres")
    return _wdd
