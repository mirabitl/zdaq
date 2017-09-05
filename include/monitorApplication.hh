#ifndef _monitorZdaqApplication_hh
#define _monitorZdaqApplication_hh
#include "baseApplication.hh"
#include <string>
#include <vector>
#include <json/json.h>
using namespace std;
#include <sstream>
#include "ReadoutLogger.hh"
#include <zmq.hpp>


namespace zdaq {
  class monitorApplication : public zdaq::baseApplication
{
public:
  monitorApplication(std::string name) : zdaq::baseApplication(name) ,_running(false),_context(NULL),_publisher(NULL),_period(120)
  {
    this->unlock();
  _fsm=this->fsm();
  
// Register state
  _fsm->addState("STOPPED");
  _fsm->addState("RUNNING");

  _fsm->addTransition("OPEN","CREATED","STOPPED",boost::bind(&zdaq::monitorApplication::open, this,_1));
  _fsm->addTransition("START","STOPPED","RUNNING",boost::bind(&zdaq::monitorApplication::start, this,_1));
  _fsm->addTransition("STOP","RUNNING","STOPPED",boost::bind(&zdaq::monitorApplication::stop, this,_1));
  _fsm->addTransition("CLOSE","STOPPED","CREATED",boost::bind(&zdaq::monitorApplication::close, this,_1));
  

    
  }
  virtual void userCreate(zdaq::fsmmessage* m)
  {
    std::cout<<"On rentre dans userCreate "<<std::endl;
    if (this->parameters().isMember("TCPPort"))
      {
	std::stringstream ss;
	ss<<"tcp://"<<this->host()<<":"<<this->parameters()["TCPPort"].asUInt();
	this->infos()["service"]=ss.str();
      }
  }
  virtual void open(zdaq::fsmmessage* m)=0;
  virtual void close(zdaq::fsmmessage* m)=0;
  virtual Json::Value status()=0;
  virtual std::string hardware()=0;
  
  void stop(zdaq::fsmmessage* m)
  {
    //
    _running=false;
    g_store.join_all();
  }

  void start(zdaq::fsmmessage* m)
  {
    
    if (m->content().isMember("period"))
      { 
	this->parameters()["period"]=m->content()["period"];
      }
    if (!this->parameters().isMember("period"))
    {
      std::cout<<"Please define Reading period"<<std::endl;
      return;
    }
    else
    _period=this->parameters()["period"].asUInt();
  
    if (this->parameters().isMember("TCPPort"))
    {
      _context= new zmq::context_t(1);
      _publisher= new zmq::socket_t((*_context), ZMQ_PUB);
      std::stringstream sport;
      sport<<"tcp://*:"<<this->parameters()["TCPPort"].asUInt();
      std::cout<<"Binding to "<<sport.str()<<std::endl;
      _publisher->bind(sport.str());
      
    }
    
    if (m->content().isMember("location"))
    { 
      this->parameters()["location"]=m->content()["location"];
    }
   else
     if (!this->parameters().isMember("location"))
	this->parameters()["location"]="/HOME";	 
    g_store.create_thread(boost::bind(&zdaq::monitorApplication::monitor, this));
  _running=true;
    
}

  void monitor()
  {
    Json::FastWriter fastWriter;
  std::stringstream sheader;
  sheader<<this->hardware()<<"@"<<this->parameters()["location"].asString();
  std::string head=sheader.str();
  while (_running)
  {
    
    if (_publisher==NULL)
    {
      std::cout<<"No publisher defined"<<std::endl;
      break;
    }
    
    zmq::message_t ma1((void*)head.c_str(), head.length(), NULL); 
    _publisher->send(ma1, ZMQ_SNDMORE); 
    Json::Value jstatus=this->status();
    std::string scont= fastWriter.write(jstatus);
    zmq::message_t ma2((void*)scont.c_str(), scont.length(), NULL); 

    std::cout<<"publishing "<<head<<" =>"<<scont<<std::endl;
    _publisher->send(ma2);
    if (!_running) break;
    ::sleep(_period);
  }
  std::cout<<"End of monitoring task"<<std::endl;

}
  void lock(){theSync_.lock();}
  void unlock(){theSync_.unlock();}
protected:
  zdaq::fsmweb* _fsm;
  boost::thread_group g_store;
  bool _running;
  uint32_t _period;
  zmq::context_t* _context;
  zmq::socket_t *_publisher;
  boost::interprocess::interprocess_mutex theSync_;
};
};
#endif
