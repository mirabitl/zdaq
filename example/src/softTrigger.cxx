#include "softTrigger.hh"
#include <iostream>
#include <sstream>

using namespace zdaq;

zdaq::softTrigger::softTrigger(std::string name) : zdaq::baseApplication(name),_running(false),_microsleep(100000),_event(0),_bx(0),_hardware("SoftTrigger"),_location("ANYWHWERE")
{
  _fsm=this->fsm();
  
  // Register state
  _fsm->addState("CREATED");
  _fsm->addState("CONFIGURED");
  _fsm->addState("RUNNING");
  _fsm->addTransition("CONFIGURE","CREATED","CONFIGURED",boost::bind(&zdaq::softTrigger::configure, this,_1));
  _fsm->addTransition("CONFIGURE","CONFIGURED","CONFIGURED",boost::bind(&zdaq::softTrigger::configure, this,_1));
  _fsm->addTransition("START","CONFIGURED","RUNNING",boost::bind(&zdaq::softTrigger::start, this,_1));
  _fsm->addTransition("STOP","RUNNING","CONFIGURED",boost::bind(&zdaq::softTrigger::stop, this,_1));
  _fsm->addTransition("HALT","RUNNING","CREATED",boost::bind(&zdaq::softTrigger::halt, this,_1));
  _fsm->addTransition("HALT","CONFIGURED","CREATED",boost::bind(&zdaq::softTrigger::halt, this,_1));
  
  _fsm->addCommand("STATUS",boost::bind(&zdaq::softTrigger::c_status, this,_1,_2));
  
  //Start server
  

  char* wp=getenv("WEBPORT");
  if (wp!=NULL)
    {
      std::cout<<"Service "<<name<<" started on port "<<atoi(wp)<<std::endl;
      this->fsm()->start(atoi(wp));
    }
  _context=new zmq::context_t();
  _triggerPublisher=NULL;
}

void zdaq::softTrigger::configure(zdaq::fsmmessage* m)
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

  // check parameters
  if (!this->parameters().isMember("microsleep")) {LOG4CXX_ERROR(_logZdaqex,"Missing microsleep");return;}
  if (!this->parameters().isMember("tcpPort")) {LOG4CXX_ERROR(_logZdaqex,"Missing tcpPort");return;}
  if (!this->parameters().isMember("datasize")) {LOG4CXX_ERROR(_logZdaqex,"Missing datasize");return;}

  _microsleep=this->parameters()["microsleep"].asUInt();
  _tcpPort=this->parameters()["tcpPort"].asUInt();
  _datasize=this->parameters()["datasize"].asUInt();

  if (_triggerPublisher==NULL)
    _triggerPublisher = new  zdaq::zPublisher(_hardware,_location,_tcpPort,_context); 
   
}
/**
 * Thread process per zmPusher
 */
Json::Value zdaq::softTrigger::status()
{
  
  Json::Value r=Json::Value::null;
  r["event"]=_event;
  Json::Value j((Json::Value::UInt64) _bx);
  r["bxid"]=j;
  r["size"]=_datasize;
  return r;
}

void zdaq::softTrigger::publishingThread()
{
  _event=0;
  while (_running)
    {
      ::usleep(_microsleep);
      if (!_running) break;
	
      _triggerPublisher->post(this->status());
	
      _event++;
      _bx++;
    }
}
/**
 * Transition from CONFIGURED to RUNNING, starts one thread per data source
 */
void zdaq::softTrigger::start(zdaq::fsmmessage* m)
{
  std::cout<<"Received "<<m->command()<<std::endl;
  _event=0;
  _running=true;
  
  _gthr.create_thread(boost::bind(&zdaq::softTrigger::publishingThread, this));
    
}
/**
 * RUNNING to CONFIGURED, Stop threads 
 */
void zdaq::softTrigger::stop(zdaq::fsmmessage* m)
{
  
  
  std::cout<<"Received "<<m->command()<<std::endl;
  
  // Stop running
  _running=false;
  ::sleep(1);

  std::cout<<"joining"<<std::endl;
  
  _gthr.join_all();
   
}
/**
 * go back to CREATED, call stop and destroy sources
 */
void zdaq::softTrigger::halt(zdaq::fsmmessage* m)
{
  
  
  std::cout<<"Received "<<m->command()<<std::endl;
  if (_running)
    this->stop(m);
  std::cout<<"Destroying"<<std::endl;
  //stop data sources
  _bx=0;
  if (_triggerPublisher!=NULL) delete _triggerPublisher;
}


/**
 * Standalone command STATUS to get the statistics of each data source 
 */
void zdaq::softTrigger::c_status(Mongoose::Request &request, Mongoose::JsonResponse &response)
{
  std::cout<<"list"<<request.getUrl()<<" "<<request.getMethod()<<" "<<request.getData()<<std::endl;
  response["answer"]=this->status();

}
