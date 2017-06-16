#include <dlfcn.h>


#include "zmMerger.hh"
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
zmMerger::zmMerger(zmq::context_t* c) : zmPuller(c), _running(false),_nDifs(0)
{
  _eventMap.clear();
  _processors.clear();

  _mReceived.clear();
}
zmMerger::~zmMerger()
{
  this->clear();
}
void zmMerger::clear()
{
  for (std::map<uint64_t,std::vector<zdaq::buffer*> >::iterator it=_eventMap.begin();it!=_eventMap.end();it++)
    {
      for (std::vector<zdaq::buffer*>::iterator jt=it->second.begin();jt!=it->second.end();jt++) delete (*jt);
    }
  _eventMap.clear();
  _processors.clear();

  _mReceived.clear();

}

void  zmMerger::registerProcessor(std::string name)
{
  std::stringstream s;
  s<<"lib"<<name<<".so";
  void* library = dlopen(s.str().c_str(), RTLD_NOW);

  printf("%s %x \n",dlerror(),library);
    // Get the loadFilter function, for loading objects
  zdaq::zmprocessor* (*create)();
  create = (zdaq::zmprocessor* (*)())dlsym(library, "loadProcessor");
  printf("%s %x \n",dlerror(),create);
  printf("%s lods to %x \n",s.str().c_str(),create); 
  //void (*destroy)(Filter*);
  // destroy = (void (*)(Filter*))dlsym(library, "deleteFilter");
    // Get a new filter object
  zdaq::zmprocessor* a=(zdaq::zmprocessor*) create();
  _processors.push_back(a);
}


void zmMerger::unregisterProcessor(zdaq::zmprocessor* p)
{
  std::vector<zdaq::zmprocessor*>::iterator it=std::find(_processors.begin(),_processors.end(),p);
  if (it!=_processors.end())
    _processors.erase(it);
}
void zmMerger::registerDataSource(std::string url)
{
  std::cout<<"adding input Stream "<<url<<std::endl;
  this->addInputStream(url,true);
}


uint32_t zmMerger::numberOfDataPacket(uint32_t k)
{
  std::map<uint64_t,std::vector<zdaq::buffer*> >::iterator it=_eventMap.find(k);
  if (it!=_eventMap.end())
    return it->second.size();
  else
    return 0;
}


void zmMerger::processEvent(uint32_t idx)
{
  std::map<uint64_t,std::vector<zdaq::buffer*> >::iterator it=_eventMap.find(idx);
  if (it->second.size()!=numberOfDataSource()) return;
  if (it->first==0) return; // do not process event 0
  _evt=it->first;
  _build++;
  //std::cout<<"full  event find " <<it->first<<std::endl;
  for (std::vector<zdaq::zmprocessor*>::iterator itp=_processors.begin();itp!=_processors.end();itp++)
    {
      (*itp)->processEvent(it->first,it->second);
    }

  
  // remove completed events
  for (std::vector<zdaq::buffer*>::iterator iv=it->second.begin();iv!=it->second.end();iv++) delete (*iv);
  it->second.clear();
  _eventMap.erase(it);
  

  
}
void zmMerger::processRunHeader()
{
  for (std::vector<zdaq::zmprocessor*>::iterator itp=_processors.begin();itp!=_processors.end();itp++)
    {
      (*itp)->processRunHeader(_runHeader);
    }
}
    
void zmMerger::start(uint32_t nr)
{
  // Do the start of the the processors
  _run=nr;
  _evt=0;
  _build=0;
  std::cout<<"run : "<<_run<<" ZMMERGER START for "<<numberOfDataSource()<<" sources"<<std::endl;
  for (std::vector<zdaq::zmprocessor*>::iterator itp=_processors.begin();itp!=_processors.end();itp++)
    {
      (*itp)->start(nr);
    }

  _running=true;
  _gThread.create_thread(boost::bind(&zdaq::zmMerger::scanMemory, this));
  //_gThread.create_thread(boost::bind(&zdaq::zmMerger::processEvents, this));

}
void zmMerger::scanMemory()
{
  this->enablePolling();
  this->poll();
}
void zmMerger::stop()
{
  _running=false;
  this->disablePolling();
  _gThread.join_all();

  // Do the stop of the the processors
  for (std::vector<zdaq::zmprocessor*>::iterator itp=_processors.begin();itp!=_processors.end();itp++)
    {
      (*itp)->stop();
    }

}
void  zmMerger::processData(std::string idd,zmq::message_t *message)
{
  uint32_t detid,sid,gtc;
  uint64_t bx;
  sscanf(idd.c_str(),"DS-%d-%d %d %ld",&detid,&sid,&gtc,&bx);
  std::map<uint64_t,std::vector<zdaq::buffer*> >::iterator it_gtc=_eventMap.find(gtc);

  zdaq::buffer *b = new zdaq::buffer(0x100000);
  memcpy(b->ptr(),message->data(),message->size());
  b->uncompress();
  if (it_gtc!=_eventMap.end())
    it_gtc->second.push_back(b);
  else
    {
      std::vector<zdaq::buffer*> v;
      v.clear();
      v.push_back(b);
          
      std::pair<uint64_t,std::vector<zdaq::buffer*> > p(gtc,v);
      _eventMap.insert(p);
      it_gtc=_eventMap.find(gtc);
    }
  if (it_gtc->second.size()==this->numberOfDataSource())
    {
      //if (it_gtc->first%100==0)
      printf("GTC %lu %lu  %d\n",it_gtc->first,it_gtc->second.size(),this->numberOfDataSource());
      this->processEvent(gtc);
    }

  // Fill summary
  std::stringstream ss;
  ss<<"DS-"<<detid<<"-"<<sid;

  std::map<std::string,uint64_t>::iterator itsum=_mReceived.find(ss.str());
  if (itsum==_mReceived.end())
    {
      std::pair<std::string,uint64_t> p(ss.str(),1);
      _mReceived.insert(p);
    }
  else
    itsum->second++;
   
}
void zmMerger::summary()
{
  for (auto x:_mReceived)
    {
      printf("%s => %d \n",x.first.c_str(),x.second);
    }
}

Json::Value zmMerger::status()
{
  Json::Value jrep,jsta;
  jsta["run"]=_run;
  jsta["event"]=_evt;
  jsta["build"]=_build;
  for (auto x:_mReceived)
    {
      Json::Value jds;
      jds["id"]=x.first;
      jds["received"]=(Json::Value::UInt64) x.second;
      jrep.append(jds);

    }
  jsta["difs"]=jrep;
  return jsta;
}
