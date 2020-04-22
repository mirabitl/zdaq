#include "softTrigger.hh"
#include <iostream>
#include <sstream>

using namespace zdaq;
using namespace zdaq::example;

zdaq::example::softTrigger::softTrigger(std::string name) : zdaq::baseApplication(name),_running(false),_paused(true),_microsleep(100000),_event(0),_bx(0),_hardware("SoftTrigger"),_location("ANYWHWERE")
{
  _fsm=this->fsm();
  
  // Register state
  _fsm->addState("CREATED");
  _fsm->addState("CONFIGURED");
  _fsm->addState("RUNNING");

  // Register transition
  _fsm->addTransition("CONFIGURE","CREATED","CONFIGURED",boost::bind(&zdaq::example::softTrigger::configure, this,_1));
  _fsm->addTransition("CONFIGURE","CONFIGURED","CONFIGURED",boost::bind(&zdaq::example::softTrigger::configure, this,_1));
  _fsm->addTransition("START","CONFIGURED","RUNNING",boost::bind(&zdaq::example::softTrigger::start, this,_1));
  _fsm->addTransition("STOP","RUNNING","CONFIGURED",boost::bind(&zdaq::example::softTrigger::stop, this,_1));
  _fsm->addTransition("HALT","RUNNING","CREATED",boost::bind(&zdaq::example::softTrigger::halt, this,_1));
  _fsm->addTransition("HALT","CONFIGURED","CREATED",boost::bind(&zdaq::example::softTrigger::halt, this,_1));


  // Register standalone commands
  _fsm->addCommand("STATUS",boost::bind(&zdaq::example::softTrigger::c_status, this,_1,_2));
  _fsm->addCommand("PERIOD",boost::bind(&zdaq::example::softTrigger::c_period, this,_1,_2));
  _fsm->addCommand("SIZE",boost::bind(&zdaq::example::softTrigger::c_size, this,_1,_2));
  _fsm->addCommand("NTRG",boost::bind(&zdaq::example::softTrigger::c_ntrg, this,_1,_2));
  _fsm->addCommand("PAUSE",boost::bind(&zdaq::example::softTrigger::c_pause, this,_1,_2));
  
  //Start server
  

  char* wp=getenv("WEBPORT");
  if (wp!=NULL)
    {
      LOG4CXX_INFO(_logZdaqex,"Service "<<name<<" started on port "<<atoi(wp));
      this->fsm()->start(atoi(wp));
    }
  _context=new zmq::context_t(1);
  _triggerPublisher=NULL;
}
/**
 * \brief Configuration, parameters are either comming from the configuration or 
 *  from the configuration messages
 */
void zdaq::example::softTrigger::configure(zdaq::fsmmessage* m)
{
  
  LOG4CXX_INFO(_logZdaqex," Receiving: "<<m->command()<<" value:"<<m->value());

 
  if (m->content().isMember("microsleep"))
    { 
      this->parameters()["microsleep"]=m->content()["microsleep"];
    }
  if (m->content().isMember("datasize"))
    { 
      this->parameters()["datasize"]=m->content()["datasize"];
    }
  if (m->content().isMember("tcpPort"))
    { 
      this->parameters()["tcpPort"]=m->content()["tcpPort"];
    }
  if (m->content().isMember("ntrg"))
    { 
      this->parameters()["ntrg"]=m->content()["ntrg"];
    }

  // check parameters
  if (!this->parameters().isMember("microsleep")) {LOG4CXX_ERROR(_logZdaqex,"Missing microsleep");return;}
  if (!this->parameters().isMember("tcpPort")) {LOG4CXX_ERROR(_logZdaqex,"Missing tcpPort");return;}
  if (!this->parameters().isMember("datasize")) {LOG4CXX_ERROR(_logZdaqex,"Missing datasize");return;}
  if (!this->parameters().isMember("ntrg")) {LOG4CXX_ERROR(_logZdaqex,"Missing ntrg");return;}

  _microsleep=this->parameters()["microsleep"].asUInt();
  _tcpPort=this->parameters()["tcpPort"].asUInt();
  _datasize=this->parameters()["datasize"].asUInt();
  _ntrg=this->parameters()["ntrg"].asUInt();
  if (this->parameters().isMember("location"))
    _location=this->parameters()["location"].asString();
  if (this->parameters().isMember("hardware"))
    _hardware=this->parameters()["hardware"].asString();
  // Create the publisher
  if (_triggerPublisher==NULL)
    {
    _triggerPublisher = new  zdaq::mon::zPublisher(_hardware,_location,_tcpPort,_context);

    LOG4CXX_INFO(_logZdaqex,"Publisher created: "<<_hardware<<" "<<_location<<" "<<_tcpPort);
    }
  this->findClients();
}

void zdaq::example::softTrigger::findClients()
{
  _clients.clear();
  if (!this->parameters().isMember("clientname")) return;
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

	  std::string p_name=process["NAME"].asString();
	  if (p_name.compare(this->parameters()["clientname"].asString())!=0) continue;


	  fsmwebCaller* client= new fsmwebCaller(host,port);
	  _clients.push_back(client);



	}

    }
  

}


/**
 * Return the current status of the event number,bx,datasize and period
 */
Json::Value zdaq::example::softTrigger::status()
{
  
  Json::Value r=Json::Value::null;
  r["event"]=_event;
  Json::Value j((Json::Value::UInt64) _bx);
  r["bxid"]=j;
  r["size"]=_datasize;
  r["period"]=_microsleep;
  r["ntrg"]=_ntrg;
  return r;
}
/**
 * Publishing thread, the event number and bx are periodically updated and the staus published
 */
void zdaq::example::softTrigger::publishingThread()
{
  _event=0;
  while (_running)
    {
      ::usleep(_microsleep);

      // Check the status of clients
      bool wait=false;uint32_t nsmax=0;
      for (auto x=_clients.begin();x!=_clients.end();x++)
	{
	  (*x)->sendCommand("STATUS");
	  
	  const Json::Value cjsources=(*x)->answer()["answer"]["zmSenders"];
	  //std::cout<<(*x)->answer()["answer"]<<std::endl;
	  for (Json::ValueConstIterator it = cjsources.begin(); it != cjsources.end(); ++it)
	    {
	      const Json::Value& p = *it;
	      uint32_t ns=p["event"].asUInt();
	      if (ns>nsmax) nsmax=ns;
	      wait=wait || ((_event-ns)>2*_ntrg);
	    }
	}
      if (wait) {
	LOG4CXX_DEBUG(_logZdaqex," waiting "<<nsmax<<" sent:"<<_event);
	continue;
      }



      //
      if (_paused) continue;
      if (_throttled) continue;
      if (!_running) break;
	
      _triggerPublisher->post(this->status());

      if (_event%100==0)
	LOG4CXX_INFO(_logZdaqex,"Publishing "<<this->status()<<" "<<_microsleep);
      _event=_event+_ntrg;
      _bx=_bx+_ntrg;


    
      
    }
}
/**
 * Transition from CONFIGURED to RUNNING, starts one thread to publish
 */
void zdaq::example::softTrigger::start(zdaq::fsmmessage* m)
{
  LOG4CXX_INFO(_logZdaqex,"Received "<<m->command());
  _event=0;
  _running=true;
  _paused=false;
  _throttled=false;
  _gthr.create_thread(boost::bind(&zdaq::example::softTrigger::publishingThread, this));
}
/**
 * RUNNING to CONFIGURED, Stop the publishing thread
 */
void zdaq::example::softTrigger::stop(zdaq::fsmmessage* m)
{
  
  
  LOG4CXX_INFO(_logZdaqex,"Received "<<m->command());
  
  // Stop running
  _running=false;
  
  ::sleep(1);

  LOG4CXX_INFO(_logZdaqex,"joining");
  
  _gthr.join_all();
  _paused=true;
  

}
/**
 * go back to CREATED, call stop and destroy sources
 */
void zdaq::example::softTrigger::halt(zdaq::fsmmessage* m)
{
  
  
  LOG4CXX_INFO(_logZdaqex,"Received "<<m->command());
  if (_running)
    this->stop(m);
  LOG4CXX_INFO(_logZdaqex,"Destroying");
  //stop data sources
  _bx=0;
  if (_triggerPublisher!=NULL) delete _triggerPublisher;
}


/**
 * Standalone command STATUS to get the statistics of each data source 
 */
void zdaq::example::softTrigger::c_status(Mongoose::Request &request, Mongoose::JsonResponse &response)
{
  LOG4CXX_INFO(_logZdaqex,"list"<<request.getUrl()<<" "<<request.getMethod()<<" "<<request.getData());
  response["answer"]=this->status();

}
/**
 * Change the period of publication
 */
void zdaq::example::softTrigger::c_period(Mongoose::Request &request, Mongoose::JsonResponse &response)
{

  uint32_t period=atoi(request.get("value","1000000").c_str());
  _microsleep=period;
  LOG4CXX_INFO(_logZdaqex,"PERIOD "<<_microsleep);
  response["answer"]=this->status();

}

/**
 * Change the data size request, 0 is random on exServer side
 */


void zdaq::example::softTrigger::c_size(Mongoose::Request &request, Mongoose::JsonResponse &response)
{

  uint32_t psize=atoi(request.get("value","32").c_str());

    LOG4CXX_INFO(_logZdaqex,"Data size "<<psize);
  _datasize=psize;
    
  response["answer"]=this->status();

}
void zdaq::example::softTrigger::c_ntrg(Mongoose::Request &request, Mongoose::JsonResponse &response)
{

  uint32_t psize=atoi(request.get("value","100").c_str());

  LOG4CXX_INFO(_logZdaqex,"NTRG"<<psize);
  _ntrg=psize;
    
  response["answer"]=this->status();

}

void zdaq::example::softTrigger::c_pause(Mongoose::Request &request, Mongoose::JsonResponse &response)
{

  uint32_t ipause=atoi(request.get("value","1").c_str());
  _paused=(ipause==1);
  LOG4CXX_INFO(_logZdaqex,"PAUSED "<<ipause);
    
  response["answer"]=this->status();

}

