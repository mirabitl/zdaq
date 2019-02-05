#include <dlfcn.h>


#include "localCollector.hh"
#include "zdaqLogger.hh"
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

using namespace zdaq;
localCollector::localCollector(zmq::context_t* c) : zmPuller(c), _running(false)
{
  _builders.clear();

}
localCollector::~localCollector()
{
  
}

void  localCollector::registerBuilder(std::string name)
{

  zmq::socket_t* s=new zmq::socket_t((*_context),ZMQ_PUSH);
  
  s->setsockopt(ZMQ_SNDHWM,128);
  s->connect(name);
  _builders.push_back(s);
}



void localCollector::registerDataSource(std::string url)
{
  
  LOG4CXX_INFO(_logZdaq,"Adding input Stream "<<url);

  this->addInputStream(url,true);
}




void localCollector::loadParameters(Json::Value params)
{
  
}
    
void localCollector::start(uint32_t nr)
{
  // Do the start of the the processors
  _run=nr;
  _evt=0;
  _packet=0;

  LOG4CXX_INFO(_logZdaq,"run : "<<_run<<" localCollector START for "<<numberOfDataSource()<<" sources");
  for (std::vector<zdaq::zmprocessor*>::iterator itp=_processors.begin();itp!=_processors.end();itp++)
    {
      (*itp)->start(nr);
    }

  _running=true;
  _gThread.create_thread(boost::bind(&zdaq::localCollector::scanMemory, this));
  //_gThread.create_thread(boost::bind(&zdaq::localCollector::processEvents, this));

}
void localCollector::scanMemory()
{
  this->enablePolling();
  this->poll();
}
void localCollector::stop()
{
  _running=false;
  this->disablePolling();
  LOG4CXX_INFO(_logZdaq,"Stopping the threads");
  //  printf("ZmMeger =>Stopping the threads \n");
  _gThread.join_all();


  LOG4CXX_INFO(_logZdaq,"Leaving Stop method");

}
void  localCollector::processData(std::string idd,zmq::message_t *message)
{
  bool registering=(idd.compare(0,2,"ID") == 0);

  uint32_t detid,sid,gtc;
  uint64_t bx;
  if (!registering)
  {
    sscanf(idd.c_str(),"DS-%d-%d %d %ld",&detid,&sid,&gtc,&bx);
     uint32_t ib=gtc%_builders.size();
  	 s_sendmore((*_builders[ib]),idd);
		_builders[ib]->send(message);
    _evt=gtc;
    _packet++;
  }
  else
  {
    sscanf(idd.c_str(),"ID-%d-%d %d %ld",&detid,&sid,&gtc,&bx);
    for (auto x=_builders.begin();x!=_builders.end();x++)
    {
      s_sendmore((*x),idd);
      x->setNumberOfDataSource(message);
    }
  }

 
  
}


Json::Value localCollector::status()
{
  Json::Value jrep,jsta;
  jsta["run"]=_run;
  jsta["event"]=_evt;
  jsta["packet"]=_packet;
  jsta["running"]=_running;
 
  return jsta;
}
