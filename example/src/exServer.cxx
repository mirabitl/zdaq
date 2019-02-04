#include "exServer.hh"
#include <iostream>
#include <sstream>

using namespace zdaq;

zdaq::exServer::exServer(std::string name) : zdaq::baseApplication(name),_running(false),_detid(0),_event(0),_bx(0)
{
  _fsm=this->fsm();
  
  // Register state
  _fsm->addState("CREATED");
  _fsm->addState("INITIALISED");
  _fsm->addState("CONFIGURED");
  _fsm->addState("RUNNING");
  _fsm->addTransition("INITIALISE","CREATED","INITIALISED",boost::bind(&zdaq::exServer::initialise, this,_1));
  _fsm->addTransition("CONFIGURE","INITIALISED","CONFIGURED",boost::bind(&zdaq::exServer::configure, this,_1));
  _fsm->addTransition("CONFIGURE","CONFIGURED","CONFIGURED",boost::bind(&zdaq::exServer::configure, this,_1));
  _fsm->addTransition("START","CONFIGURED","RUNNING",boost::bind(&zdaq::exServer::start, this,_1));
  _fsm->addTransition("STOP","RUNNING","CONFIGURED",boost::bind(&zdaq::exServer::stop, this,_1));
  _fsm->addTransition("HALT","RUNNING","INITIALISED",boost::bind(&zdaq::exServer::halt, this,_1));
  _fsm->addTransition("HALT","CONFIGURED","INITIALISED",boost::bind(&zdaq::exServer::halt, this,_1));
  
  _fsm->addCommand("DOWNLOAD",boost::bind(&zdaq::exServer::download, this,_1,_2));
  _fsm->addCommand("STATUS",boost::bind(&zdaq::exServer::status, this,_1,_2));
  
  //Start server
  

  char* wp=getenv("WEBPORT");
  if (wp!=NULL)
    {
      std::cout<<"Service "<<name<<" started on port "<<atoi(wp)<<std::endl;
      this->fsm()->start(atoi(wp));
    }
  _context=new zmq::context_t();
  _triggerSubscriber = new  zdaq::zSubscriber(_context); 
  _triggerSubscriber->addHandler(boost::bind(&zdaq::exServer::checkTrigger, this,_1));
  for (int i=1;i<0x20000;i++) _plrand[i]= std::rand();
}
void zdaq::exServer::initialise(zdaq::fsmmessage* m)
{
  
  LOG4CXX_INFO(_logZdaqex," Receiving: "<<m->command()<<" value:"<<m->value());
  this->autoDiscover();
}
void zdaq::exServer::configure(zdaq::fsmmessage* m)
{
  
  LOG4CXX_INFO(_logZdaqex," Receiving: "<<m->command()<<" value:"<<m->value());
  // Delet existing zmPushers
  for (std::vector<zdaq::zmPusher*>::iterator it=_sources.begin();it!=_sources.end();it++)
    delete (*it);
  _sources.clear();
  // Clear statistics
  _stat.clear();
  // Add a data source
  // Parse the json message
  // {"command": "CONFIGURE", "content": {"detid": 100, "sourceid": [23, 24, 26]}}

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
  // check parameters
  if (!this->parameters().isMember("detid")) {LOG4CXX_ERROR(_logZdaqex,"Missing detid");return;}
  if (!this->parameters().isMember("sourceid")) {LOG4CXX_ERROR(_logZdaqex,"Missing sourceid");return;}
  if (!this->parameters().isMember("pushdata")) {LOG4CXX_ERROR(_logZdaqex,"Missing pushdata address");return;}
  if (!this->parameters().isMember("trigsub")) {LOG4CXX_ERROR(_logZdaqex,"Missing trigsub address");return;}
  if (!this->parameters().isMember("paysize")) {LOG4CXX_ERROR(_logZdaqex,"Missing paysize payload size");return;}
  if (!this->parameters().isMember("mode")) {LOG4CXX_ERROR(_logZdaqex,"Missing mode TRIGGER/ALONE");return;}

  Json::Value jc=this->parameters();
  int32_t det=jc["detid"].asInt();
  _detid=det;
  const Json::Value& books = jc["sourceid"];
  Json::Value array_keys;
  for (Json::ValueConstIterator it = books.begin(); it != books.end(); ++it)
    {
      const Json::Value& book = *it;
      int32_t sid=(*it).asInt();
      // rest as before
      LOG4CXX_INFO(_logZdaqex,"Creating data source "<<det<<" "<<sid);
      array_keys.append((det<<16)|sid);
      zdaq::zmPusher* ds= new zdaq::zmPusher(_context,det,sid);
      ds->connect(this->parameters()["pushdata"].asString());

      if (this->parameters().isMember("compress"))
	ds->setCompress(this->parameters()["compress"].asUInt()==1);
      
      _sources.push_back(ds);
      _stat.insert(std::pair<uint32_t,uint32_t>((det<<16)|sid,0));
	
    }
  _triggerSubscriber->addStream(this->parameters()["trigsub"].asString());
  // Overwrite msg
  //Prepare complex answer
  m->setAnswer(array_keys);
  
}
/**
 * Thread process per zmPusher
 */
void zdaq::exServer::fillEvent(uint32_t event,uint64_t bx,zdaq::zmPusher* ds,uint32_t eventSize)
{
  if (eventSize==0)
    {
      eventSize=int(std::rand()*1.*0x20000/(RAND_MAX))-1;
      if (eventSize<10) eventSize=10;
      if (eventSize>0x20000-10) eventSize=0x20000-10;
    }
  // Payload address
  uint32_t* pld=(uint32_t*) ds->payload();
  // Random data with tags at start and end of data payload 
  for (int i=1;i<eventSize-1;i++) pld[i]= _plrand[i];
  pld[0]=event;
  pld[eventSize-1]=event;
  // Publish the data source
  ds->publish(_event,bx,eventSize*sizeof(uint32_t));
  // Update statistics
  std::map<uint32_t,uint32_t>::iterator its=_stat.find((ds->buffer()->detectorId()<<16)|ds->buffer()->dataSourceId());
  if (its!=_stat.end())
    its->second=event;
	
}
void zdaq::exServer::checkTrigger(std::vector<zdaq::publishedItem*>& items)
{
  if (this->parameters()["mode"].asString().compare("ALONE")!=0)
    for (auto x:items)
      if (x->hardware().compare("SoftTrigger")==0)
	{
	  _event=x->status()["event"].asUInt();
	  _bx=x->status()["bxid"].asUInt64();
	  uint32_t psi=x->status()["size"].asUInt();
	  for (std::vector<zdaq::zmPusher*>::iterator ids=_sources.begin();ids!=_sources.end();ids++)
	    {
	      this->fillEvent(_event,_bx,(*ids),psi);
	    }
	}
}
void zdaq::exServer::readdata(zdaq::zmPusher *ds)
{
  uint32_t last_evt=0;
  std::srand(std::time(0));
  while (_running)
    {
      ::usleep(10000);
      if (!_running) break;
      if (_event == last_evt) continue;
      if (_event%100==0)
	std::cout<<"Thread of "<<ds->buffer()->dataSourceId()<<" is running "<<_event<<" events and status is "<<_running<<std::endl;

      // Just fun 
      // Create a dummy buffer of fix length depending on source id and random data
      // 
      uint32_t psi=this->parameters()["paysize"].asUInt();
      this->fillEvent(_event,_bx,ds,psi);
      last_evt=_event;
      _event++;
      _bx++;
    }
  std::cout<<"Thread of "<<ds->buffer()->dataSourceId()<<" is exiting after "<<last_evt<<"events"<<std::endl;
}
/**
 * Transition from CONFIGURED to RUNNING, starts one thread per data source
 */
void zdaq::exServer::start(zdaq::fsmmessage* m)
{
  std::cout<<"Received "<<m->command()<<std::endl;
  _event=0;
  _running=true;
  if (this->parameters()["mode"].asString().compare("ALONE")==0)
    {
      for (std::vector<zdaq::zmPusher*>::iterator ids=_sources.begin();ids!=_sources.end();ids++)
	{
    (*ids)->collectorRegister();
	  _gthr.create_thread(boost::bind(&zdaq::exServer::readdata, this,(*ids)));
	  ::usleep(500000);
	}
    }
  else
    {
      LOG4CXX_INFO(_logZdaqex,"Working in event handling mode");
  for (std::vector<zdaq::zmPusher*>::iterator ids=_sources.begin();ids!=_sources.end();ids++)
	{
    (*ids)->collectorRegister();
	}
      _triggerSubscriber->start();
    }
}
/**
 * RUNNING to CONFIGURED, Stop threads 
 */
void zdaq::exServer::stop(zdaq::fsmmessage* m)
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
void zdaq::exServer::halt(zdaq::fsmmessage* m)
{
  
  
  std::cout<<"Received "<<m->command()<<std::endl;
  if (_running)
    this->stop(m);
  std::cout<<"Destroying"<<std::endl;
  //stop data sources
  for (std::vector<zdaq::zmPusher*>::iterator it=_sources.begin();it!=_sources.end();it++)
    delete (*it);
  _sources.clear();
}

/**
 * Standalone command DOWNLOAD, unused but it might be used to download data to 
 * configure the hardware
 */
void zdaq::exServer::download(Mongoose::Request &request, Mongoose::JsonResponse &response)
{
  std::cout<<"download"<<request.getUrl()<<" "<<request.getMethod()<<" "<<request.getData()<<std::endl;
  response["answer"]="download called to be implemented";
}
/**
 * Standalone command LIST to get the statistics of each data source 
 */
void zdaq::exServer::status(Mongoose::Request &request, Mongoose::JsonResponse &response)
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
  response["zmPushers"]=array_keys;

}

