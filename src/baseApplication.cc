#include "baseApplication.hh"
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include "fsmwebCaller.hh"
#include "zdaqLogger.hh"

using namespace zdaq;
std::string wget(std::string url);
baseApplication::baseApplication(std::string name) : _login("")
{
  
  _fsm=new fsmweb(name);
  
  
  // Register state
  _fsm->addState("VOID");
  _fsm->addState("CREATED");
  
  _fsm->addTransition("CREATE","VOID","CREATED",boost::bind(&baseApplication::create, this,_1));
  
  // Commands
  _fsm->addCommand("GETCONFIG",boost::bind(&baseApplication::c_getconfiguration,this,_1,_2));
  _fsm->addCommand("GETPARAM",boost::bind(&baseApplication::c_getparameter,this,_1,_2));
  _fsm->addCommand("SETPARAM",boost::bind(&baseApplication::c_setparameter,this,_1,_2));
  _fsm->addCommand("INFO",boost::bind(&baseApplication::c_info,this,_1,_2));
  
  _fsm->setState("VOID");
  
  
  
  // Host and process name
  char hname[80];
  gethostname(hname, 80);
  _hostname = hname;
  char* pname=getenv("PROCESSNAME");
  if (pname!=NULL)
    _processName=pname;
  else
    _processName="UNKNOWN";

  // Find the instance
  _instance=0;
 
  char* wi=getenv("INSTANCE");
  if (wi!=NULL) _instance=atoi(wi);
  // Find the port
  _port=0;
 
  char* wp=getenv("WEBPORT");
  if (wp!=NULL) _port=atoi(wp);

  char* wl=getenv("WEBLOGIN");
  if (wl!=NULL) _login=std::string(wl);
  
  _jConfig=Json::Value::null;
  _jParam=Json::Value::null;
_jInfo=Json::Value::null;
 _jInfo["host"]=this->host();
 _jInfo["port"]=this->port();
 _jInfo["name"]=this->name();
 _jInfo["instance"]=this->instance();
 _jInfo["login"]=this->login();
}

void  baseApplication::create(zdaq::fsmmessage* m)
{

  Json::Value jc=m->content();
  if (jc.isMember("login"))
    {
      _login=jc["login"].asString();
    }
  std::cout<<"WEB LOGIN is "<<_login<<std::endl<<std::flush;
  // else
  //   _login=std::string("");
  if (jc.isMember("file"))
  {
    std::string fileName=jc["file"].asString();
    Json::Reader reader;
    std::ifstream ifs (fileName.c_str(), std::ifstream::in);
    
    bool parsingSuccessful = reader.parse(ifs,_jConfig,false);
    
    
  }
  else
    if (jc.isMember("url"))
    {
      std::string url=jc["url"].asString();
      std::cout<<url<<std::endl;
      std::cout<<"Hostname "<<_hostname<<std::endl;
      //std::string jsconf=wget(url);

      std::string jsconf=fsmwebCaller::curlQuery(url,_login);
      std::cout<<jsconf<<std::endl;
      Json::Reader reader;
      Json::Value jcc;
      bool parsingSuccessful = reader.parse(jsconf,jcc);
      if (jcc.isMember("content"))
	_jConfig=jcc["content"];
      else
	_jConfig=jcc;
      
      
    }
    
    // Overwrite msg
    //Prepare complex answer
    // Now parse the config find the host and the PROCESSNAME
    if (_jConfig==Json::Value::null) 
    {
      Json::Value rep;
      rep["error"]="Missing configuration";
      m->setAnswer(rep);
      return;
    }
    if (!_jConfig.isMember("HOSTS")) 
    {
      Json::Value rep;
      rep["error"]="Missing HOSTS tag";
      m->setAnswer(rep);
      return;
    }
    if (!_jConfig["HOSTS"].isMember(_hostname)) 
    {
      Json::Value rep;
      rep["error"]="Missing hostname in list";
      rep["config"]=_jConfig;
      m->setAnswer(rep);
      return;
    }
    Json::Value _jconf=_jConfig["HOSTS"][_hostname];
    const Json::Value& blist = _jconf;
    
    for (Json::ValueConstIterator it = blist.begin(); it != blist.end(); ++it)
    {
      const Json::Value& b = *it;
      
      if (b.isMember("NAME"))
      {
        if (b["NAME"].asString().compare(_processName)==0)
        {
	  uint32_t port=0,instance=0;
	  if (b.isMember("ENV"))
	    {
	      const Json::Value& jenv=b["ENV"];

	      for (Json::ValueConstIterator ie = jenv.begin(); ie != jenv.end(); ++ie)
		{
		  std::string envp=(*ie).asString();
              //      std::cout<<"Env found "<<envp.substr(0,7)<<std::endl;
              //std::cout<<"Env found "<<envp.substr(8,envp.length()-7)<<std::endl;
		  if (envp.substr(0,7).compare("WEBPORT")==0)
		    {
		      port=atol(envp.substr(8,envp.length()-7).c_str());
		    }
		  if (envp.substr(0,8).compare("INSTANCE")==0)
		    {
		      instance=atol(envp.substr(9,envp.length()-8).c_str());
		    }

		}
	    }
	    if (port!=_port || instance!=_instance) continue;
          if (b.isMember("PARAMETER")) 
          {
            _jParam=b["PARAMETER"];
            break;
          }
        }   
      } 
      
    }      
  end:
  
    Json::Value rep;
    rep["param"]=_jParam;
    rep["config"]=_jConfig;
    m->setAnswer(rep);
    this->userCreate(m);
    return;

}
void baseApplication::c_getconfiguration(Mongoose::Request &request, Mongoose::JsonResponse &response)
{
  if (_jConfig==Json::Value::null)    {response["STATUS"]="NO Configuration yet"; return;}
  //uint32_t adr=atol(request.get("address","2").c_str());
  //uint32_t val=ccc->getCCCReadout()->DoReadRegister(adr);
  
  response["STATUS"]="DONE";
  response["configuration"]=_jConfig;
}
void baseApplication::c_getparameter(Mongoose::Request &request, Mongoose::JsonResponse &response)
{
  if (_jParam==Json::Value::null)    {response["STATUS"]="No Parameters yet"; return;}
  //std::string request.get("address","2").c_str());
  //uint32_t val=ccc->getCCCReadout()->DoReadRegister(adr);
  
  response["STATUS"]="DONE";
  response["PARAMETER"]=_jParam;
}
void baseApplication::c_info(Mongoose::Request &request, Mongoose::JsonResponse &response)
{
  

  response["STATUS"]="DONE";
  response["INFO"]=_jInfo;
}
void baseApplication::c_setparameter(Mongoose::Request &request, Mongoose::JsonResponse &response)
{
  std::string str=request.get("PARAMETER","NONE");
  if (str.compare("NONE")==0) {response["STATUS"]="No PARAMETER tag found"; return;}
  Json::Reader reader;
  Json::Value newconf;
  bool parsingSuccessfull = reader.parse(str,newconf);
  if (parsingSuccessfull) 
    {
      _jParam=newconf;
      response["STATUS"]="DONE";
      response["PARAMETER"]=_jParam;
    }
  else    
    response["STATUS"]="Invalid parsing of PARAMETER";
}
void  baseApplication::userCreate(zdaq::fsmmessage* m) {;}
Json::Value baseApplication::configuration() { return _jConfig;}
Json::Value& baseApplication::parameters() {return _jParam;}
Json::Value& baseApplication::infos() {return _jInfo;}
fsmweb* baseApplication::fsm(){return _fsm;}


void baseApplication::autoDiscover()
{
  _apps.clear();
  Json::Value cjs=this->configuration()["HOSTS"];
  //  std::cout<<cjs<<std::endl;
  std::vector<std::string> lhosts=this->configuration()["HOSTS"].getMemberNames();
  // Loop on hosts
  for (auto host:lhosts)
    {
      const Json::Value cjsources=this->configuration()["HOSTS"][host];
      //std::cout<<cjsources<<std::endl;
      for (Json::ValueConstIterator it = cjsources.begin(); it != cjsources.end(); ++it)
        {
          const Json::Value& process = *it;
          std::string p_name=process["NAME"].asString();
          uint32_t port=0;
          const Json::Value& cenv=process["ENV"];
          for (Json::ValueConstIterator iev = cenv.begin(); iev != cenv.end(); ++iev)
            {
              std::string envp=(*iev).asString();
              //      std::cout<<"Env found "<<envp.substr(0,7)<<std::endl;
              //std::cout<<"Env found "<<envp.substr(8,envp.length()-7)<<std::endl;
              if (envp.substr(0,7).compare("WEBPORT")==0)
          {
            port=atol(envp.substr(8,envp.length()-7).c_str());
            break;
          }
            }
          if (port==0) continue;
          if (port==this->port() && host.compare(this->host())==0) continue;
          LOG4CXX_INFO(_logZdaq," Name "<<p_name<<" "<<host<<":"<<port);
          continue;
          fsmwebCaller* b= new fsmwebCaller(host,port);
          std::map<std::string,std::vector<fsmwebCaller*> >::iterator it_app=_apps.find(p_name);

          if (it_app!=_apps.end())
            it_app->second.push_back(b);
          else
            {
            std::vector<fsmwebCaller*> v;
            v.clear();
            v.push_back(b);
                
            std::pair<std::string,std::vector<fsmwebCaller*> > p(p_name,v);
            _apps.insert(p);
            it_app=_apps.find(p_name);
            }


	      }

    }
  

}

