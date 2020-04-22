#include "exRunControl.hh"
#include <unistd.h>
#include <stdint.h>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <iostream>
#include <sstream>
#include <fstream>
using namespace zdaq;
using namespace zdaq::example;

exRunControl::exRunControl(std::string name) : zdaq::baseApplication(name)
  {
    _builderClients.clear();_triggerClient=0;
    
    _exServerClients.clear();
    
    _fsm=this->fsm();
    
    
    this->fsm()->addState("INITIALISED");
    this->fsm()->addState("CONFIGURED");
    this->fsm()->addState("RUNNING");

    this->fsm()->addTransition("INITIALISE","CREATED","INITIALISED",boost::bind(&exRunControl::initialise, this,_1));
    this->fsm()->addTransition("CONFIGURE","INITIALISED","CONFIGURED",boost::bind(&exRunControl::configure, this,_1));
    this->fsm()->addTransition("CONFIGURE","CONFIGURED","CONFIGURED",boost::bind(&exRunControl::configure, this,_1));
    this->fsm()->addTransition("START","CONFIGURED","RUNNING",boost::bind(&exRunControl::start, this,_1));
    this->fsm()->addTransition("STOP","RUNNING","CONFIGURED",boost::bind(&exRunControl::stop, this,_1));
    this->fsm()->addTransition("HALT","CONFIGURED","CREATED",boost::bind(&exRunControl::halt, this,_1));
    this->fsm()->addTransition("HALT","INITIALISED","CREATED",boost::bind(&exRunControl::halt, this,_1));

    // Commands

    this->fsm()->addCommand("STATUS",boost::bind(&exRunControl::c_status,this,_1,_2));

    this->fsm()->addCommand("LISTPROCESS",boost::bind(&exRunControl::c_listProcess,this,_1,_2));
    this->fsm()->addCommand("BUILDERSTATUS",boost::bind(&exRunControl::c_builderStatus,this,_1,_2));
    this->fsm()->addCommand("EXSERVERSTATUS",boost::bind(&exRunControl::c_exServerStatus,this,_1,_2));
    this->fsm()->addCommand("FORCESTATE",boost::bind(&exRunControl::c_changeState,this,_1,_2));

  cout<<"Building exRunControl"<<endl;
  

  char* wp=getenv("WEBPORT");
  if (wp!=NULL)
    {
      LOG4CXX_ERROR(_logZdaqex,__PRETTY_FUNCTION__<<"Service "<<name<<" started on port "<<atoi(wp));
    this->fsm()->start(atoi(wp));
    }

  _jConfigContent=Json::Value::null;
  
  }


void  exRunControl::userCreate(zdaq::fsmmessage* m)
{
  // Stored the configuration file used
    if (m->content().isMember("url"))
      {
	_jConfigContent["url"]=m->content()["url"];
	if (m->content().isMember("login"))
	  _jConfigContent["login"]=m->content()["login"];
      }
    
    else
      if (m->content().isMember("file"))
	_jConfigContent["file"]=m->content()["file"];
     else
      if (m->content().isMember("mongo"))
	_jConfigContent["mongo"]=m->content()["mongo"];
    // Discover and send CREATE transition to all needed applications
    this->discover();
}

void exRunControl::c_listProcess(Mongoose::Request &request, Mongoose::JsonResponse &response)
{
  Json::Value rep;
  Json::Value cjs=this->configuration()["HOSTS"];
  //  std::cout<<cjs<<std::endl;
  std::vector<std::string> lhosts=this->configuration()["HOSTS"].getMemberNames();
  // Loop on hosts
  for (auto host:lhosts)
    {
      //std::cout<<" Host "<<host<<" found"<<std::endl;
      // Loop on processes
      const Json::Value cjsources=this->configuration()["HOSTS"][host];
      //std::cout<<cjsources<<std::endl;
      for (Json::ValueConstIterator it = cjsources.begin(); it != cjsources.end(); ++it)
	{
	  const Json::Value& process = *it;
	  std::string p_name=process["NAME"].asString();
	  Json::Value p_param=Json::Value::null;
	  if (process.isMember("PARAMETER")) p_param=process["PARAMETER"];
	  // Loop on environenemntal variable
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
	  // Now analyse process Name
	  if (p_name.compare("BUILDER")==0 )
	    {
	      fsmwebCaller builderClient(host,port);
	      Json::Value jstat=builderClient.queryWebStatus();Json::Value jres;jres["NAME"]=p_name;jres["HOST"]=host;jres["PORT"]=port;jres["PID"]=jstat["PID"];jres["STATE"]=jstat["STATE"];rep.append(jres);
	      
	
	     
	    }
	  
	  if (p_name.compare("TRIGGER")==0)
	    {
	      if (_triggerClient == NULL) _triggerClient= new fsmwebCaller(host,port);
	      Json::Value jstat=_triggerClient->queryWebStatus();Json::Value jres;jres["NAME"]=p_name;jres["HOST"]=host;jres["PORT"]=port;jres["PID"]=jstat["PID"];jres["STATE"]=jstat["STATE"];rep.append(jres);
	     
	    }
	 
	   if (p_name.compare("EXSERVER")==0)
	    {
	      fsmwebCaller tdc(host,port);
	      //printf("TDC client %x \n",tdc);
	      Json::Value jstat=tdc.queryWebStatus();Json::Value jres;jres["NAME"]=p_name;jres["HOST"]=host;jres["PORT"]=port;jres["PID"]=jstat["PID"];jres["STATE"]=jstat["STATE"];rep.append(jres);

	    }
	   
	  
	}

    }
  
  response["LIST"]=rep;
  
}
   
void exRunControl::discover()
{
  _exServerClients.clear();
  _builderClients.clear();
  Json::Value cjs=this->configuration()["HOSTS"];
  //  std::cout<<cjs<<std::endl;
  std::vector<std::string> lhosts=this->configuration()["HOSTS"].getMemberNames();
  // Loop on hosts
  for (auto host:lhosts)
    {
      //std::cout<<" Host "<<host<<" found"<<std::endl;
      // Loop on processes and find their hots,name and port
      const Json::Value cjsources=this->configuration()["HOSTS"][host];
      //std::cout<<cjsources<<std::endl;
      for (Json::ValueConstIterator it = cjsources.begin(); it != cjsources.end(); ++it)
	{
	  const Json::Value& process = *it;
	  std::string p_name=process["NAME"].asString();
	  Json::Value p_param=Json::Value::null;
	  if (process.isMember("PARAMETER")) p_param=process["PARAMETER"];
	  // Loop on environenemntal variable
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
	  // Now analyse process Name
	  if (p_name.compare("BUILDER")==0)
	    {
	      fsmwebCaller* builderClient= new fsmwebCaller(host,port);
	      std::string state=builderClient->queryState();
	      fprintf(stderr,"Builder client   %s \n",state.c_str());
	      if (state.compare("VOID")==0 && !_jConfigContent.empty())
		{
		  builderClient->sendTransition("CREATE",_jConfigContent);
		}
	      if (!p_param.empty()) this->parameters()["builder"]=p_param;
	      _builderClients.push_back(builderClient);
	    }
	  
	  if (p_name.compare("TRIGGER")==0)
	    {
	      _triggerClient= new fsmwebCaller(host,port);
	      std::string state=_triggerClient->queryState();
	      fprintf(stderr,"Mdcc client  %s \n",state.c_str());
	      if (state.compare("VOID")==0 && !_jConfigContent.empty())
		{
		  _triggerClient->sendTransition("CREATE",_jConfigContent);
		}
	      if (!p_param.empty()) this->parameters()["trigger"]=p_param;
		      
	      //fprintf(stderr,"MDCC client %x \n",_triggerClient);
	    }
	
	   if (p_name.compare("EXSERVER")==0)
	    {
	      fsmwebCaller* dc= new fsmwebCaller(host,port);
	      //fprintf(stderr,"DIF client %x \n",dc);
	      std::string state=dc->queryState();
	      fprintf(stderr,"exServer client  %s \n",state.c_str());
	      if (state.compare("VOID")==0 && !_jConfigContent.empty())
		{
		  dc->sendTransition("CREATE",_jConfigContent);
		}

	      _exServerClients.push_back(dc);
	    }
	  
	   
	  
	}

    }
  

}







void exRunControl::singleconfigure(fsmwebCaller* d)
{
  // Essai Json::Value jc=this->parameters()["db"];
  Json::Value jc;
  jc["difid"]=0;
  d->sendTransition("CONFIGURE",jc);
}
void exRunControl::singlestart(fsmwebCaller* d)
{
 
  Json::Value c;
  c["difid"]=0;
  d->sendTransition("START",c);
  //std::cout<<"received "<<d->reply()<<std::endl;
}
void exRunControl::singlestop(fsmwebCaller* d)
{

  Json::Value c;
  c["difid"]=0;
  d->sendTransition("STOP",c);
}


void exRunControl::initialise(zdaq::fsmmessage* m)
{
  // Configure the Event builder
  if (_builderClients.size()>0)
    {
      LOG4CXX_DEBUG(_logZdaqex,__PRETTY_FUNCTION__<<"Builder initialisation"<<_builderClients.size());

      for (auto x=_builderClients.begin();x!=_builderClients.end();x++)
	(*x)->sendTransition("CONFIGURE");
    }
 
  // Initialise all data sources server
 
  LOG4CXX_DEBUG(_logZdaqex,__PRETTY_FUNCTION__<<"Nothing to do at initialise .... ");
for (std::vector<fsmwebCaller*>::iterator it=_exServerClients.begin();it!=_exServerClients.end();it++)
    {
      Json::Value jc;
  jc["difid"]=0;
  (*it)->sendTransition("INITIALISE",jc);
    }
  // Fill status
  Json::Value jsta="NOTHING";
  m->setAnswer(jsta);
}

void exRunControl::configure(zdaq::fsmmessage* m)
{
  // Configure trigger
  if (_triggerClient)
    {
      _triggerClient->sendTransition("CONFIGURE");  
    }


  // Configure exServer in //
  boost::thread_group g;
  for (std::vector<fsmwebCaller*>::iterator it=_exServerClients.begin();it!=_exServerClients.end();it++)
    {
      g.create_thread(boost::bind(&exRunControl::singleconfigure, this,(*it)));
    }
  g.join_all();
 
  Json::Value jsta="DONE";
  m->setAnswer(jsta);
}

void exRunControl::start(zdaq::fsmmessage* m)
{
  // Get the new run number


  LOG4CXX_INFO(_logZdaqex,__PRETTY_FUNCTION__<<" calling for new runs from  the Mongo Db");
  if (!_jConfigContent.isMember("mongo"))
    {
      _run=10000;
    }
  else
    {
      std::string comment="Not yet set";
      if (m->content().isMember("comment"))
	comment=m->content()["comment"].asString();
      std::stringstream scmd;
      scmd << "/bin/bash -c 'mgjob --new-run "<<" --comment=\""<<comment<<"\" --location=" <<this->parameters()["location"] <<"'";
      std::cout<<scmd.str()<<std::endl;
      int ier=system(scmd.str().c_str());
      std::stringstream sname;
      sname << "/dev/shm/mgjob/lastrun.json";
      
      fprintf(stderr, "Parsing the file %s\n", sname.str().c_str());
      Json::Reader reader;
      Json::Value jcc;
      std::ifstream ifs(sname.str().c_str(), std::ifstream::in);
      //      Json::Value _jall;
      fprintf(stderr, "Before Parsing the file %s\n", sname.str().c_str());
      bool parsingSuccessful = reader.parse(ifs, jcc, false);
      ier=system("mv  /dev/shm/mgjob/lastrun.json /dev/shm/mgjob/lastrun.json.back");
       if (jcc.isMember("run"))
        _run=jcc["run"].asUInt();
      else
        _run=10000;
    }
  LOG4CXX_INFO(_logZdaqex,__PRETTY_FUNCTION__<<" new run "<<_run);
  // Start the builder
  if (_builderClients.size())
    {
      LOG4CXX_DEBUG(_logZdaqex,__PRETTY_FUNCTION__<<" calling Builder start ");
      Json::Value jl;
      jl["run"]=_run;
      for (auto x=_builderClients.begin();x!=_builderClients.end();x++)
	(*x)->sendTransition("START",jl);
      ::sleep(1);
    }
    




   // Start the servers 
  boost::thread_group g;
  for (std::vector<fsmwebCaller*>::iterator it=_exServerClients.begin();it!=_exServerClients.end();it++)
    {
      LOG4CXX_DEBUG(_logZdaqex,__PRETTY_FUNCTION__<<" calling  singlestart for exServer ");
      //std::cout<<"Creating thread"<<std::endl;
      g.create_thread(boost::bind(&exRunControl::singlestart, this,(*it)));
    }
  g.join_all();
  //::sleep(5);
  
  //Start the CCC
   if (_triggerClient)
     {
       LOG4CXX_DEBUG(_logZdaqex,__PRETTY_FUNCTION__<<" calling Trigger start ");
       _triggerClient->sendTransition("START");
     }
  
   LOG4CXX_DEBUG(_logZdaqex,__PRETTY_FUNCTION__<<" calling ends");
   m->setAnswer(this->json_status());  
}
void exRunControl::stop(zdaq::fsmmessage* m)
{
    // Pause the MDCC
   if (_triggerClient)
     {
       _triggerClient->sendTransition("STOP");
     }

    
   // Stop the DIFs data source server
  boost::thread_group g;
  for (std::vector<fsmwebCaller*>::iterator it=_exServerClients.begin();it!=_exServerClients.end();it++)
    {
      g.create_thread(boost::bind(&exRunControl::singlestop, this,(*it)));
    }
  g.join_all();
  LOG4CXX_DEBUG(_logZdaqex,__PRETTY_FUNCTION__<<"end of STOP of exServers Status ");
  
  // Stop the builder
  if (_builderClients.size())
    {
      for (auto x=_builderClients.begin();x!=_builderClients.end();x++)
	(*x)->sendTransition("STOP");
    }
   LOG4CXX_DEBUG(_logZdaqex,__PRETTY_FUNCTION__<<"end of STOP  ");
   

   m->setAnswer(this->json_status());
}

void exRunControl::halt(zdaq::fsmmessage* m)
{
   // Pause the MDCC
   if (_triggerClient)
     {
       _triggerClient->sendTransition("HALT");
     }
  for (std::vector<fsmwebCaller*>::iterator it=_exServerClients.begin();it!=_exServerClients.end();it++)
    {
      Json::Value jl;
      jl["difid"]=0;
      (*it)->sendTransition("HALT",jl);
    }
  if (_builderClients.size())
    {
      for (auto x=_builderClients.begin();x!=_builderClients.end();x++)
	(*x)->sendTransition("HALT");
    }
   LOG4CXX_DEBUG(_logZdaqex,__PRETTY_FUNCTION__<<"end of HALT  ");
}

exRunControl::~exRunControl() 
{
 
  if (_triggerClient) delete _triggerClient;
  for (auto x=_builderClients.begin();x!=_builderClients.end();x++)
    delete (*x);
  for (std::vector<fsmwebCaller*>::iterator it=_exServerClients.begin();it!=_exServerClients.end();it++)
    delete (*it);
  
  _exServerClients.clear();
  _builderClients.clear();
  
}


void exRunControl::c_changeState(Mongoose::Request &request, Mongoose::JsonResponse &response)
  {
    
    std::string states=request.get("state",this->fsm()->state());
    this->fsm()->setState(states);
     response["STATUS"]="DONE";
     response["NEWSTATE"]=states;
  }



Json::Value exRunControl::json_builder_status()
{
  Json::Value rep;
  for (auto x=_builderClients.begin();x!=_builderClients.end();x++)
    {
      Json::Value r=Json::Value::null;
      r["run"]=-1;
      r["event"]=-1;
      r["url"]=(*x)->url();

      (*x)->sendCommand("STATUS");
      LOG4CXX_DEBUG(_logZdaqex,__PRETTY_FUNCTION__<<(*x)->answer());
      if (!(*x)->answer().empty())
	{
	  if ((*x)->answer().isMember("answer"))
	
	    r["run"]=(*x)->answer()["answer"]["answer"]["run"];
	  r["event"]=(*x)->answer()["answer"]["answer"]["event"];
	  r["builder"]=(*x)->answer()["answer"]["answer"]["difs"];
	  r["built"]=(*x)->answer()["answer"]["answer"]["build"];
	  r["total"]=(*x)->answer()["answer"]["answer"]["total"];
	  r["compressed"]=(*x)->answer()["answer"]["answer"]["compressed"];
	  r["time"]=(int) time(0);
	
	}
      rep.append(r);
    }
  return rep;
}


void  exRunControl::c_builderStatus(Mongoose::Request &request, Mongoose::JsonResponse &response)
{
  response["VALUE"]=this->json_builder_status();
  response["STATUS"]="DONE";
}
Json::Value exRunControl::json_exServer_status()
{
  Json::Value devlist=Json::Value::null;;
  for (std::vector<fsmwebCaller*>::iterator it=_exServerClients.begin();it!=_exServerClients.end();it++)
    {

      (*it)->sendCommand("STATUS");
      const Json::Value& jdevs=(*it)->answer();
      if (jdevs.isMember("answer"))
	if (jdevs["answer"].isMember("zmPushers"))
      //for (Json::ValueConstIterator jt = jdevs.begin(); jt != jdevs.end(); ++jt)
	  devlist.append(jdevs["answer"]["zmPushers"]);
    }
    return devlist; 
}
void exRunControl::c_exServerStatus(Mongoose::Request &request, Mongoose::JsonResponse &response)
{
  
  
  response["pushers"]=this->json_exServer_status();
  response["STATUS"]="DONE";
}

Json::Value exRunControl::json_status()
{
  Json::Value response=Json::Value::null;
    response["builder"]=this->json_builder_status();
  response["pushers"]=this->json_exServer_status();
  return response;
}
void exRunControl::c_status(Mongoose::Request &request, Mongoose::JsonResponse &response)
{
 
  response["builder"]=this->json_builder_status();
  response["pushers"]=this->json_exServer_status();
  response["STATUS"]="DONE";
}


