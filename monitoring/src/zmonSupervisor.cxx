#include "zmonSupervisor.hh"
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/dir.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <dlfcn.h>


using namespace zdaq;
using namespace zdaq::monitoring;

zdaq::monitoring::supervisor::supervisor(std::string name) : zdaq::baseApplication(name), _running(false), _publisher(NULL),_period(30)
{
  this->unlock();
  // Register state
  this->fsm()->addState("CREATED");
  this->fsm()->addState("CONFIGURED");
  this->fsm()->addState("RUNNING");

  // Register transitions
  this->fsm()->addTransition("CONFIGURE", "CREATED", "CONFIGURED", boost::bind(&zdaq::monitoring::supervisor::configure, this, _1));
  this->fsm()->addTransition("START", "CONFIGURED", "RUNNING", boost::bind(&zdaq::monitoring::supervisor::start, this, _1));
  this->fsm()->addTransition("STOP", "RUNNING", "CONFIGURED", boost::bind(&zdaq::monitoring::supervisor::stop, this, _1));
  this->fsm()->addTransition("HALT", "RUNNING", "CREATED", boost::bind(&zdaq::monitoring::supervisor::halt, this, _1));
  this->fsm()->addTransition("HALT", "CONFIGURED", "CREATED", boost::bind(&zdaq::monitoring::supervisor::halt, this, _1));

  // Standalone command
  this->fsm()->addCommand("STATUS", boost::bind(&zdaq::monitoring::supervisor::status, this, _1, _2));

  //Start server
  char *wp = getenv("WEBPORT");
  if (wp != NULL)
    {
      LOG4CXX_INFO(_logZdaqex, __PRETTY_FUNCTION__ << "Service " << name << " started on port " << atoi(wp));
      this->fsm()->start(atoi(wp));
    }
}

void zdaq::monitoring::supervisor::configure(zdaq::fsmmessage *m)
{
  LOG4CXX_INFO(_logZdaqex, __PRETTY_FUNCTION__ << "Received " << m->command() << " Value " << m->value());
  // Store message content in paramters

  if (m->content().isMember("TCPPort"))
    {
      this->parameters()["TCPPort"] = m->content()["TCPPort"];
    }
  if (m->content().isMember("period"))
    {
      this->parameters()["period"] = m->content()["period"];
    }
  if (m->content().isMember("plugins"))
    {
      this->parameters()["plugins"] = m->content()["plugins"];
    }
  // Check that needed parameters exists

  
  if (!this->parameters().isMember("plugins"))
    {
      LOG4CXX_ERROR(_logZdaqex, "Missing plugins, list of monitoring pluggins");
      return;
    }
  if (!this->parameters().isMember("period"))
    {
      LOG4CXX_ERROR(_logZdaqex, "Missing period, period of monitoring readout");
      return;
    }
  Json::Value jc = this->parameters();

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
  
  //Prepare complex answer
  Json::Value prep;

  // Register the processors
  const Json::Value &pbooks = jc["plugins"];
  Json::Value parray_keys;
  for (Json::ValueConstIterator it = pbooks.begin(); it != pbooks.end(); ++it)
    {
      const Json::Value &book = *it;
      LOG4CXX_INFO(_logZdaqex, "registering " << (*it).asString());
      this->registerPlugin((*it).asString());
      parray_keys.append((*it).asString());
    }

  LOG4CXX_INFO(_logZdaqex, " Setting parameters for plugins ");
  for (auto x:_plugins)
    x->loadParameters(this->parameters());
  LOG4CXX_INFO(_logZdaqex, " Registering plugins command ");
  for (auto x:_plugins)
    x->registerCommands(this->fsm());
  LOG4CXX_INFO(_logZdaqex, " Open plugins  ");
  for (auto x:_plugins)
    x->open(m);
  prep["plugins"] = parray_keys;
  
  // Look for zmonStore declarations
  if (jc.isMember("stores"))
    {
      const Json::Value &pbooks = jc["stores"];
      Json::Value parray_keys;
      for (Json::ValueConstIterator it = pbooks.begin(); it != pbooks.end(); ++it)
	{
	  const Json::Value &book = *it;
	  LOG4CXX_INFO(_logZdaqex, "registering " << (*it).asString());
	  this->registerStore((*it).asString());
	  parray_keys.append((*it).asString());
	}

      LOG4CXX_INFO(_logZdaqex, " Setting parameters for stores ");
      for (auto x:_stores)
	x->loadParameters(this->parameters());
      LOG4CXX_INFO(_logZdaqex, " Connect stores  ");
      for (auto x:_stores)
	x->connect();
      prep["stores"] = parray_keys;
    }

  // Overwrite msg


  m->setAnswer(prep);
  LOG4CXX_DEBUG(_logZdaqex, "end of configure");
  return;
}
void zdaq::monitoring::supervisor::registerPlugin(std::string name)
{
  std::stringstream s;
  s << "lib" << name << ".so";
  void *library = dlopen(s.str().c_str(), RTLD_NOW);

  //printf("%s %x \n",dlerror(),(unsigned int) library);
  LOG4CXX_INFO(_logZdaq, " Error " << dlerror() << " Library open address " << std::hex << library << std::dec);
  // Get the loadFilter function, for loading objects
  zdaq::zmonPlugin *(*create)();
  create = (zdaq::zmonPlugin * (*)()) dlsym(library, "loadPlugin");
  LOG4CXX_INFO(_logZdaq, " Error " << dlerror() << " file " << s.str() << " loads to processor address " << std::hex << create << std::dec);
  //printf("%s %x \n",dlerror(),(unsigned int) create);
  // printf("%s lods to %x \n",s.str().c_str(),(unsigned int) create);
  //void (*destroy)(Filter*);
  // destroy = (void (*)(Filter*))dlsym(library, "deleteFilter");
  // Get a new filter object
  zdaq::zmonPlugin *a = (zdaq::zmonPlugin *)create();
  _plugins.push_back(a);

}
void zdaq::monitoring::supervisor::registerStore(std::string name)
{
  std::stringstream s;
  s << "lib" << name << ".so";
  void *library = dlopen(s.str().c_str(), RTLD_NOW);

  //printf("%s %x \n",dlerror(),(unsigned int) library);
  LOG4CXX_INFO(_logZdaq, " Error " << dlerror() << " Library open address " << std::hex << library << std::dec);
  // Get the loadFilter function, for loading objects
  zdaq::zmonStore *(*create)();
  create = (zdaq::zmonStore * (*)()) dlsym(library, "loadStore");
  LOG4CXX_INFO(_logZdaq, " Error " << dlerror() << " file " << s.str() << " loads to processor address " << std::hex << create << std::dec);
  //printf("%s %x \n",dlerror(),(unsigned int) create);
  // printf("%s lods to %x \n",s.str().c_str(),(unsigned int) create);
  //void (*destroy)(Filter*);
  // destroy = (void (*)(Filter*))dlsym(library, "deleteFilter");
  // Get a new filter object
  zdaq::zmonStore *a = (zdaq::zmonStore *)create();
  _stores.push_back(a);

}

void zdaq::monitoring::supervisor::start(zdaq::fsmmessage *m)
{
  LOG4CXX_DEBUG(_logZdaqex, "Received " << m->command() << " Value " << m->value());

  g_store.create_thread(boost::bind(&zdaq::monitoring::supervisor::monitor, this));
	
  _running = true;

}
void zdaq::monitoring::supervisor::stop(zdaq::fsmmessage *m)
{
  LOG4CXX_DEBUG(_logZdaqex, "Received " << m->command() << " Value " << m->value());
  _running=false;
  g_store.join_all();
}
void zdaq::monitoring::supervisor::halt(zdaq::fsmmessage *m)
{

  LOG4CXX_DEBUG(_logZdaqex, "Received " << m->command());
  if (_running)
    this->stop(m);

  LOG4CXX_INFO(_logZdaqex, "Destroying plugins");
  //stop data sources
  _plugins.clear();
  _stores.clear();
}
void zdaq::monitoring::supervisor::status(Mongoose::Request &request, Mongoose::JsonResponse &response)
{

  if (_plugins.size() != 0)
    {
      for (auto x:_plugins)
	{
	  response[x->hardware()] = x->status();
	}
    }
  else
    response["error"] = "No plugins load yet";
}


void zdaq::monitoring::supervisor::monitor()
{
  Json::FastWriter fastWriter;
  while (_running)
    {
    
      // ID
      for (auto x:_plugins)
	{
	  if (_publisher!=NULL)
	    {
	      std::stringstream sheader;
	      sheader<<x->hardware()<<"@"<<this->parameters()["location"].asString()<<"@"<<time(0);
	      std::string head=sheader.str();
    
	      zmq::message_t ma1((void*)head.c_str(), head.length(), NULL); 
	      _publisher->send(ma1, ZMQ_SNDMORE);
	      // Status
	      Json::Value jstatus=x->status();
	      std::string scont= fastWriter.write(jstatus);
	      zmq::message_t ma2((void*)scont.c_str(), scont.length(), NULL); 

	      std::cout<<"publishing "<<head<<" =>"<<scont<<std::endl;
	      _publisher->send(ma2);
	    }
	  // Save the status in all conncted stores
	  for (auto y:_stores)
	    y->store(this->parameters()["location"].asString(),x->hardware(),(uint32_t) time(0),x->status());
	}
      if (!_running) break;
      ::sleep(_period);

      std::cout<<"End of monitoring task"<<std::endl;

    }
}
