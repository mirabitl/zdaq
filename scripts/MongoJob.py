
import os
from pymongo import MongoClient
import json
from bson.objectid import ObjectId
import time
import prettyjson as pj




def IP2Int(ip):
    o = map(int, ip.split('.'))
    res = (16777216 * o[3]) + (65536 * o[2]) + (256 * o[1]) + o[0]
    return res


class MongoJob:
    """
    Main class to access the Mongo DB 
    """

    def __init__(self, host,port,dbname,username,pwd):
        """
        connect Mongodb database named dbname
        """
        self.connection=MongoClient(host,port)
        self.db=self.connection[dbname]
        self.db.authenticate(username,pwd)

    def reset(self):
        """
        Reset connection to download another state
        """
        self.bson_id=[] 
    def uploadConfig(self,name,fname,comment,version=1):
        s={}
        s["content"]=json.loads(open(fname).read())
        s["name"]=name
        s["time"]=time.time()
        s["comment"]=comment
        s["version"]=version
        resconf=self.db.configurations.insert_one(s)
        print resconf

    def configurations(self):
        res=self.db.configurations.find({})
        for x in res:
            if ("comment" in x):
                print time.ctime(x["time"]),x["version"],x["name"],x["comment"]

    def downloadConfig(self,cname,version):
        res=self.db.configurations.find({'name':cname,'version':version})
        for x in res:
            print x["name"],x["version"],x["comment"]
            #var=raw_input()
            slc=x["content"]
            os.system("mkdir -p /dev/shm/mgjob")
            fname="/dev/shm/mgjob/%s_%s.json" % (cname,version)
            f=open(fname,"w+")
            f.write(json.dumps(slc, indent=2, sort_keys=True))
            f.close()
            return slc
 
     
def instance():
    # create the default access
    login=os.getenv("MGDBLOGIN","NONE")
    userinfo=login.split("@")[0]
    hostinfo=login.split("@")[1]
    dbname=login.split("@")[2]
    user=userinfo.split("/")[0]
    pwd=userinfo.split("/")[1]
    host=hostinfo.split(":")[0]
    port=int(hostinfo.split(":")[1])
    _wdd=MongoJob(host,port,dbname,user,pwd)
    return _wdd
