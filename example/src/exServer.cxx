#include "exServer.hh"
#include <iostream>
#include <sstream>

using namespace zdaq;
using namespace zdaq::example;

zdaq::example::exServer::exServer(std::string name) : zdaq::baseApplication(name),_running(false),_detid(0),_event(0),_bx(0)
{
  _fsm=this->fsm();
  
  // Register state
  _fsm->addState("CREATED");
  _fsm->addState("INITIALISED");
  _fsm->addState("CONFIGURED");
  _fsm->addState("RUNNING");

  // Register the transitions
  _fsm->addTransition("INITIALISE","CREATED","INITIALISED",boost::bind(&zdaq::example::exServer::initialise, this,_1));
  _fsm->addTransition("CONFIGURE","INITIALISED","CONFIGURED",boost::bind(&zdaq::example::exServer::configure, this,_1));
  _fsm->addTransition("CONFIGURE","CONFIGURED","CONFIGURED",boost::bind(&zdaq::example::exServer::configure, this,_1));
  _fsm->addTransition("START","CONFIGURED","RUNNING",boost::bind(&zdaq::example::exServer::start, this,_1));
  _fsm->addTransition("STOP","RUNNING","CONFIGURED",boost::bind(&zdaq::example::exServer::stop, this,_1));
  _fsm->addTransition("HALT","RUNNING","INITIALISED",boost::bind(&zdaq::example::exServer::halt, this,_1));
  _fsm->addTransition("HALT","CONFIGURED","INITIALISED",boost::bind(&zdaq::example::exServer::halt, this,_1));

  // Register standalone commands
  _fsm->addCommand("GENERATE",boost::bind(&zdaq::example::exServer::generate, this,_1,_2));
  _fsm->addCommand("STATUS",boost::bind(&zdaq::example::exServer::status, this,_1,_2));
  
  //Start server
  
  char* wp=getenv("WEBPORT");
  if (wp!=NULL)
    {
      std::cout<<"Service "<<name<<" started on port "<<atoi(wp)<<std::endl;
      this->fsm()->start(atoi(wp));
    }
  // Initialise ZMQ 
  _context=new zmq::context_t();

}
void zdaq::example::exServer::initialise(zdaq::fsmmessage* m)
{
  
  LOG4CXX_DEBUG(_logZdaqex," Receiving: "<<m->command()<<" value:"<<m->value());
  // Initialise random data packet
  for (int i=1;i<0x20000;i++) _plrand[i]= std::rand();
  this->discover();
}
void zdaq::example::exServer::configure(zdaq::fsmmessage* m)
{
  
  LOG4CXX_DEBUG(_logZdaqex," Receiving: "<<m->command()<<" value:"<<m->value());
  // Delete any  existing zmSenders
  for (std::vector<zdaq::zmSender*>::iterator it=_sources.begin();it!=_sources.end();it++)
    delete (*it);
  _sources.clear();
  // Clear statistics
  _stat.clear();
  // Update information if any
  // Parse the json message
  // {"command": "CONFIGURE", "content": {"detid": 100, "sourceid": [23, 24, 26],....}}
  // Informations arnormally already stored in the PARAMETER tag of the configurations but can be updated
  
  if (m->content().isMember("detid"))
    { 
      this->parameters()["detid"]=m->content()["detid"];
    }
  if (m->content().isMember("sourceid"))
    { 
      this->parameters()["sourceid"]=m->content()["sourceid"];
    }
  if (m->content().isMember("pushdata"))
    { 
      this->parameters()["pushdata"]=m->content()["pushdata"];
    }
  if (m->content().isMember("trigsub"))
    { 
      this->parameters()["trigsub"]=m->content()["trigsub"];
    }

  if (m->content().isMember("paysize"))
    { 
      this->parameters()["paysize"]=m->content()["paysize"];
    }
  if (m->content().isMember("mode"))
    { 
      this->parameters()["mode"]=m->content()["mode"];
    }
  if (m->content().isMember("compress"))
    { 
      this->parameters()["compress"]=m->content()["compress"];
    }
  // check parameters exist
  if (!this->parameters().isMember("detid")) {LOG4CXX_ERROR(_logZdaqex,"Missing detid");return;}
  if (!this->parameters().isMember("sourceid")) {LOG4CXX_ERROR(_logZdaqex,"Missing sourceid");return;}
  if (!this->parameters().isMember("pushdata")) {LOG4CXX_ERROR(_logZdaqex,"Missing pushdata address");return;}
  if (!this->parameters().isMember("trigsub")) {LOG4CXX_ERROR(_logZdaqex,"Missing trigsub address");return;}
  if (!this->parameters().isMember("paysize")) {LOG4CXX_ERROR(_logZdaqex,"Missing paysize payload size");return;}
  if (!this->parameters().isMember("mode")) {LOG4CXX_ERROR(_logZdaqex,"Missing mode TRIGGER/ALONE");return;}

  // declare source and create zmSenders
  Json::Value jc=this->parameters();
  int32_t det=jc["detid"].asInt();
  _detid=det;
  const Json::Value& jsitems = jc["sourceid"];
  Json::Value array_keys;
  for (Json::ValueConstIterator it = jsitems.begin(); it != jsitems.end(); ++it)
    {
      const Json::Value& jsitem = *it;
      int32_t sid=(*it).asInt();
      // rest as before
      LOG4CXX_INFO(_logZdaqex,"Creating data source "<<det<<" "<<sid);
      array_keys.append((det<<16)|sid);
      zdaq::zmSender* ds= new zdaq::zmSender(_context,det,sid);
      //ds->connect(this->parameters()["pushdata"].asString());
      for (auto x:_vStream)
	ds->connect(x);
      ds->collectorRegister();


      if (this->parameters().isMember("compress"))
	ds->setCompress(this->parameters()["compress"].asUInt()==1);
      
      
      _sources.push_back(ds);
      _stat.insert(std::pair<uint32_t,uint32_t>((det<<16)|sid,0));
	
    }

  // Subscribe to the soft trigger source
    // Subscribe to a software trigger provider
   zmq::context_t* scontext=new zmq::context_t(1);
  _triggerSubscriber = new  zdaq::mon::zSubscriber(scontext); 
  _triggerSubscriber->addHandler(boost::bind(&zdaq::example::exServer::checkTrigger, this,_1));

  _triggerSubscriber->addStream(this->parameters()["trigsub"].asString());

  LOG4CXX_INFO(_logZdaqex," Subscribing: "<<this->parameters()["trigsub"].asString());

  // Overwrite msg
  //Prepare complex answer
  m->setAnswer(array_keys);
  
}
/**
 * Thread process per zmSender: Fill and publish an event
 */
void zdaq::example::exServer::fillEvent(uint32_t event,uint64_t bx,zdaq::zmSender* ds,uint32_t eventSize)
{
  // randomize event size if not set
  if (eventSize==0)
    {
      eventSize=int(std::rand()*1.*0x20000/(RAND_MAX))-1;
      if (eventSize<10) eventSize=10;
      if (eventSize>0x20000-10) eventSize=0x20000-10;
    }
  // Payload address
  uint32_t* pld=(uint32_t*) ds->payload();
  // Copy Random data with tags at start and end of data payload
  memcpy(pld,_plrand,eventSize*sizeof(uint32_t));
  //for (int i=1;i<eventSize-1;i++) pld[i]= _plrand[i];
  pld[0]=event;
  pld[eventSize-1]=event;
  // Publish the data source
  ds->publish(event,bx,eventSize*sizeof(uint32_t));
  // Update statistics
  std::map<uint32_t,uint32_t>::iterator its=_stat.find((ds->buffer()->detectorId()<<16)|ds->buffer()->dataSourceId());
  if (its!=_stat.end())
    its->second=event;
	
}
/**
 * Soft trigger handler
 */
void zdaq::example::exServer::checkTrigger(std::vector<zdaq::mon::publishedItem*>& items)
{
  // In trigger mode, create fake events according to the message content and publish them
  if (this->parameters()["mode"].asString().compare("ALONE")!=0)
    for (auto x:items)
      if (x->hardware().compare("DUMMYTRIGGER")==0)
	{
	  
	  _event=x->status()["event"].asUInt();
	  _bx=x->status()["bxid"].asUInt64();
	  uint32_t psi=x->status()["size"].asUInt();
	  uint32_t ntrg=x->status()["ntrg"].asUInt();
	  for (int it=0;it<ntrg;it++)
	    {

	      if (_event%100==0)
		LOG4CXX_INFO(_logZdaqex," New event "<<_event<<" "<<_bx<<" "<<psi);
	      for (std::vector<zdaq::zmSender*>::iterator ids=_sources.begin();ids!=_sources.end();ids++)
		{
		  this->fillEvent(_event,_bx,(*ids),psi);
		}
	      _event++;
	      _bx++;
	    }
	}
}
/**
 * Standalone thread with no external trigger to publish continously data
 */
void zdaq::example::exServer::streamdata(zdaq::zmSender *ds)
{
  uint32_t last_evt=0;
  std::srand(std::time(0));
  while (_running)
    {
      ::usleep(10000);
      if (!_running) break;
      if (_event == last_evt) continue;
      if (_event%100==0)
	LOG4CXX_INFO(_logZdaqex," Thread of: "<<ds->buffer()->dataSourceId()<<" is running "<<_event<<" events and status is "<<_running);
      // Just fun 
      // Create a dummy buffer of fix length depending on source id and random data
      // 
      uint32_t psi=this->parameters()["paysize"].asUInt();
      this->fillEvent(_event,_bx,ds,psi);
      last_evt=_event;
      _event++;
      _bx++;
    }
  LOG4CXX_INFO(_logZdaqex," Thread of: "<<ds->buffer()->dataSourceId()<<" is exiting after "<<last_evt<<"events");
}
/**
 * Transition from CONFIGURED to RUNNING, starts one thread per data source in standalone mode
 */
void zdaq::example::exServer::start(zdaq::fsmmessage* m)
{
  std::cout<<"Received "<<m->command()<<std::endl;
  _event=0;
  _running=true;
  if (this->parameters()["mode"].asString().compare("ALONE")==0)
    {
      // Standalone all datasources are publishing continously events of fixed size
      for (std::vector<zdaq::zmSender*>::iterator ids=_sources.begin();ids!=_sources.end();ids++)
	{
	  //(*ids)->collectorRegister();
	  _gthr.create_thread(boost::bind(&zdaq::example::exServer::streamdata, this,(*ids)));
	  ::usleep(500000);
	}
    }
  else
    {
      // Soft trigger mode, events are published when a trigger is received
      LOG4CXX_INFO(_logZdaqex,"Working in trigger handling mode");
      
      // for (std::vector<zdaq::zmSender*>::iterator ids=_sources.begin();ids!=_sources.end();ids++)
      // 	{
      // 	  (*ids)->collectorRegister();
      // 	}

      // start polling
      _triggerSubscriber->start();
    }
}
/**
 * RUNNING to CONFIGURED, Stop threads 
 */
void zdaq::example::exServer::stop(zdaq::fsmmessage* m)
{
  
  
  std::cout<<"Received "<<m->command()<<std::endl;
  
  // Stop running
  _running=false;
  ::sleep(1);

  std::cout<<"joining"<<std::endl;
  if (this->parameters()["mode"].asString().compare("ALONE")==0)
    _gthr.join_all();
  else
    _triggerSubscriber->stop();
}
/**
 * go back to CREATED, call stop and destroy sources
 */
void zdaq::example::exServer::halt(zdaq::fsmmessage* m)
{
  
  
  std::cout<<"Received "<<m->command()<<std::endl;
  if (_running)
    this->stop(m);
  std::cout<<"Destroying"<<std::endl;
  //stop data sources
  for (std::vector<zdaq::zmSender*>::iterator it=_sources.begin();it!=_sources.end();it++)
    delete (*it);
  _sources.clear();
}

/**
 * Standalone command GENERATE, unused but it might be used to generate data to 
 * configure the hardware
 */
void zdaq::example::exServer::generate(Mongoose::Request &request, Mongoose::JsonResponse &response)
{
  std::cout<<"download"<<request.getUrl()<<" "<<request.getMethod()<<" "<<request.getData()<<std::endl;
    // Initialise random data packet
  for (int i=1;i<0x20000;i++) _plrand[i]= std::rand();
  response["answer"]="DONE";
}
/**
 * Standalone command LIST to get the statistics of each data source 
 */
void zdaq::example::exServer::status(Mongoose::Request &request, Mongoose::JsonResponse &response)
{
  std::cout<<"list"<<request.getUrl()<<" "<<request.getMethod()<<" "<<request.getData()<<std::endl;
  Json::Value array_keys;
  for (std::map<uint32_t,uint32_t>::iterator it=_stat.begin();it!=_stat.end();it++)
    {
      Json::Value js;
      js["detid"]=(it->first>>16)&0xFFFF;
      js["sourceid"]=it->first&0xFFFF;
      js["event"]=it->second;
      array_keys.append(js);
      //std::cout<<it->first<<" "<<it->second<<std::endl;
      //std::cout<<js<<std::endl;
      
    }
  std::cout<<array_keys<<std::endl;
  response["detector"]=_detid;
  response["zmSenders"]=array_keys;

}

void exServer::discover()
{

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
	  if (p_name.compare("BUILDER")!=0) continue;
	  Json::Value p_param=Json::Value::null;
	  if (process.isMember("PARAMETER")) p_param=process["PARAMETER"];
	  if (p_param.isMember("collectingPort"))
	    {
	      std::stringstream ss;
	      ss<<"tcp://"<<host<<":"<<p_param["collectingPort"].asUInt();
	      _vStream.push_back(ss.str());
	      LOG4CXX_INFO(_logZdaqex," Builder paramaters "<<host<<" collecting on "<<ss.str());	  
	    }

	}

    }
  

}
