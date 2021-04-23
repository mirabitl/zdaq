#include "lsc.hh"
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
#include <fstream>      // std::ifstream

using namespace zdaq;


zdaq::lsc::lsc(std::string name,uint32_t port) :_running(false), _period(30)
{

  _fsm=new fsmweb(name);

  // Register state
  _fsm->addState("VOID");
  _fsm->addState("CONFIGURED");
  _fsm->addState("RUNNING");

  // Register transitions
  _fsm->addTransition("CONFIGURE", "VOID", "CONFIGURED", boost::bind(&zdaq::lsc::configure, this, _1));
  _fsm->addTransition("START", "CONFIGURED", "RUNNING", boost::bind(&zdaq::lsc::start, this, _1));
  _fsm->addTransition("STOP", "RUNNING", "CONFIGURED", boost::bind(&zdaq::lsc::stop, this, _1));

  // Standalone command
  _fsm->addCommand("STATUS", boost::bind(&zdaq::lsc::status, this, _1, _2));

  _fsm->setState("VOID");


  //Start server
  _fsm->start(port);

  LOG4CXX_INFO(_logZdaqex, __PRETTY_FUNCTION__ << "Service " << name << " started on port " << port);

 
  
}

void zdaq::lsc::parseDefaults(zdaq::fsmmessage *m)
{

  Json::Reader reader;
  std::ifstream ifs ("/etc/lsc.conf", std::ifstream::in);
  
  bool parsingSuccessful = reader.parse(ifs,_params,false);
      
  if (parsingSuccessful)
    {
      Json::StyledWriter styledWriter;
      std::cout << styledWriter.write(_params) << std::endl;

        // Look for zmonStore declarations
      if (_params.isMember("stores"))
	{
	  const Json::Value &pstores = _params["stores"];
	  for (Json::ValueConstIterator it = pstores.begin(); it != pstores.end(); ++it)
	    {
	      const Json::Value &store = *it;
	      LOG4CXX_INFO(_logZdaqex, "registering " << (*it).asString());
	      this->registerStore((*it).asString());
	    }
	  
	  LOG4CXX_INFO(_logZdaqex, " Setting parameters for stores ");
	  for (auto x=_stores.begin();x!=_stores.end();x++)
	    x->second.ptr()->loadParameters(_params);
	  LOG4CXX_INFO(_logZdaqex, " Connect stores  ");
	  for (auto x=_stores.begin();x!=_stores.end();x++)
	    x->second.ptr()->connect();
	}

      if (_params.isMember("TCPPort"))
	{
	  _context= new zmq::context_t(1);
	  _publisher= new zmq::socket_t((*_context), ZMQ_PUB);
	  std::stringstream sport;
	  sport<<"tcp://*:"<<_params["TCPPort"].asUInt();
	  std::cout<<"Binding to "<<sport.str()<<std::endl;
	  _publisher->bind(sport.str());
      
	}
      if (!_params.isMember("location"))
	_params["location"]="/HOME";

        // Register the processors
      const Json::Value &pplugs = _params["plugins"];
      Json::Value parray_keys;
      for (Json::ValueConstIterator it = pplugs.begin(); it != pplugs.end(); ++it)
	{
	  const Json::Value &plug = *it;
	  LOG4CXX_INFO(_logZdaqex, "registering " << (*it).asString());
	  this->registerPlugin((*it).asString());
	  parray_keys.append((*it).asString());
	}
      
      LOG4CXX_INFO(_logZdaqex, " Setting parameters for Default  plugins ");
      for (auto x=_plugins.begin();x!=_plugins.end();x++)
	x->second.ptr()->loadParameters(_params);
      LOG4CXX_INFO(_logZdaqex, " Registering Default plugins command ");
      for (auto x=_plugins.begin();x!=_plugins.end();x++)
	x->second.ptr()->registerCommands(_fsm);
      LOG4CXX_INFO(_logZdaqex, " Open Default plugins  ");
      for (auto x=_plugins.begin();x!=_plugins.end();x++)
	x->second.ptr()->open(m);


    }

}
void zdaq::lsc::configure(zdaq::fsmmessage *m)
{
  LOG4CXX_INFO(_logZdaqex, __PRETTY_FUNCTION__ << "Received " << m->command() << " Value " << m->value());
  // Store message content in paramters


    // Load defaults
  this->parseDefaults(m);


  LOG4CXX_DEBUG(_logZdaqex, "end of configure");
  return;
}
void zdaq::lsc::registerPlugin(std::string name)
{
  #ifdef OLDPLUGIN
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
  #else
  zdaq::pluginUtil<zdaq::zmonPlugin> p_info(name,"loadPlugin","deletePlugin");
  
  _plugins.insert(std::pair<std::string,zdaq::pluginUtil<zdaq::zmonPlugin> >(name,p_info));

  #endif

}
void zdaq::lsc::registerStore(std::string name)
{
#ifdef OLDPLUGIN
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

    #else
  zdaq::pluginUtil<zdaq::zmonStore> p_info(name,"loadStore","deleteStore");
  
  _stores.insert(std::pair<std::string,zdaq::pluginUtil<zdaq::zmonStore> >(name,p_info));

  #endif

}

void zdaq::lsc::start(zdaq::fsmmessage *m)
{
  LOG4CXX_DEBUG(_logZdaqex, "Received " << m->command() << " Value " << m->value());

  g_store.create_thread(boost::bind(&zdaq::lsc::monitor, this));
	
  _running = true;

}
void zdaq::lsc::stop(zdaq::fsmmessage *m)
{
  LOG4CXX_DEBUG(_logZdaqex, "Received " << m->command() << " Value " << m->value());
  _running=false;
  g_store.join_all();
}
void zdaq::lsc::status(Mongoose::Request &request, Mongoose::JsonResponse &response)
{

  if (_plugins.size() != 0)
    {
      for (auto x=_plugins.begin();x!=_plugins.end();x++)
	{
	  response[x->second.ptr()->hardware()] = x->second.ptr()->status();
	}
    }
  else
    response["error"] = "No plugins load yet";
}


void zdaq::lsc::monitor()
{
  Json::FastWriter fastWriter;
  while (_running)
    {
    
      // ID
      for (auto x=_plugins.begin();x!=_plugins.end();x++)
	{
	  if (_publisher!=NULL)
	    {
	      std::stringstream sheader;
	      sheader<<x->second.ptr()->hardware()<<"@"<<_params["location"].asString()<<"@"<<time(0);
	      std::string head=sheader.str();
    
	      zmq::message_t ma1((void*)head.c_str(), head.length(), NULL); 
	      _publisher->send(ma1, ZMQ_SNDMORE);
	      // Status
	      Json::Value jstatus=x->second.ptr()->status();
	      std::string scont= fastWriter.write(jstatus);
	      zmq::message_t ma2((void*)scont.c_str(), scont.length(), NULL); 

	      std::cout<<"publishing "<<head<<" =>"<<scont<<std::endl;
	      _publisher->send(ma2);
	    }
	  // Save the status in all conncted stores
	  for (auto y=_stores.begin();y!=_stores.end();y++)
	    y->second.ptr()->store(_params["location"].asString(),x->second.ptr()->hardware(),(uint32_t) time(0),x->second.ptr()->status());
	}
      if (!_running) break;
      ::sleep(_period);

      std::cout<<"End of monitoring task"<<std::endl;

    }
}
