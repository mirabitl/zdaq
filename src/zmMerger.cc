#include <dlfcn.h>


#include "zmMerger.hh"
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
zmMerger::zmMerger(zmq::context_t* c) : zmPuller(c), _running(false),_nDifs(0),_purge(true),_writeHeader(false),_nextEventHeader(-1)
{
  _eventMap.clear();
  _processors.clear();

  _mReceived.clear();

  /*
  _statusPublisher = new  zdaq::mon::zPublisher("builder","example",4444,c);

  LOG4CXX_INFO(_logZdaq," Status Publisher created on port 4444");
  */

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

  //printf("%s %x \n",dlerror(),(unsigned int) library);
  LOG4CXX_INFO(_logZdaq," Error "<<dlerror()<<" Library open address "<<std::hex<<library<<std::dec);
    // Get the loadFilter function, for loading objects
  zdaq::zmprocessor* (*create)();
  create = (zdaq::zmprocessor* (*)())dlsym(library, "loadProcessor");
  LOG4CXX_INFO(_logZdaq," Error "<<dlerror()<<" file "<<s.str()<<" loads to processor address "<<std::hex<<create<<std::dec);
  //printf("%s %x \n",dlerror(),(unsigned int) create);
  // printf("%s lods to %x \n",s.str().c_str(),(unsigned int) create); 
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
  
  LOG4CXX_INFO(_logZdaq,"Adding input Stream "<<url);

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
  //printf("Processing %d Size %d for %d Map %d \n",it->first,it->second.size(),numberOfDataSource(),_eventMap.size());
  if (it->second.size()!=numberOfDataSource()) return;
  if (it->first==0) return; // do not process event 0
  _evt=it->first;
  _build++;
  //std::cout<<"full  event find " <<it->first<<std::endl;
  for (std::vector<zdaq::zmprocessor*>::iterator itp=_processors.begin();itp!=_processors.end();itp++)
    {

      if (_writeHeader)
	{
	  LOG4CXX_INFO(_logZdaq,"Processing Header "<<_evt<<" "<<_nextEventHeader<<" "<<idx)
	  if (_nextEventHeader>0 && _nextEventHeader==idx)
	    {
	      (*itp)->processRunHeader(_runHeader);
	      _writeHeader=false;
	      _nextEventHeader=-1;
	    }
	}
      (*itp)->processEvent(it->first,it->second);
    }

  
  // remove completed events
  // for (std::vector<zdaq::buffer*>::iterator iv=it->second.begin();iv!=it->second.end();iv++) delete (*iv);
  // it->second.clear();
  // _eventMap.erase(it);
  //printf("End of processing %d Map size %d \n",_evt,_eventMap.size());
  if (_build%100==0)
    LOG4CXX_DEBUG(_logZdaq,"End of processing of event "<<_evt<<" remaining map size "<<_eventMap.size()<<"  built"<<_build);
  // Clearing uncompleted event with GTC< 100 current GTC

  /*
  if (_build%1000==0)
    {
      LOG4CXX_DEBUG(_logZdaq,"Publishing status "<<this->status());
      _statusPublisher->post(this->status());
    }
  */
}
void zmMerger::processRunHeader()
{
  _writeHeader=true;
  /*
  for (std::vector<zdaq::zmprocessor*>::iterator itp=_processors.begin();itp!=_processors.end();itp++)
    {
      //std::cout<<"On enevoie"<<std::endl;
      (*itp)->processRunHeader(_runHeader);
      //std::cout<<"Apres enevoie"<<std::endl;
    }
  */
}
void zmMerger::loadParameters(Json::Value params)
{
  for (std::vector<zdaq::zmprocessor*>::iterator itp=_processors.begin();itp!=_processors.end();itp++)
    {
      (*itp)->loadParameters(params);
    }
}
    
void zmMerger::start(uint32_t nr)
{
  // Do the start of the the processors
  _run=nr;
  _evt=0;
  _build=0;
  _totalSize=0;
  _compressedSize=0;
    // clear event Map
  for (std::map<uint64_t,std::vector<zdaq::buffer*> >::iterator it=_eventMap.begin();it!=_eventMap.end();it++)
    {
      for (std::vector<zdaq::buffer*>::iterator jt=it->second.begin();jt!=it->second.end();jt++) delete (*jt);
    }
  _eventMap.clear();

  LOG4CXX_INFO(_logZdaq,"run : "<<_run<<" ZMMERGER START for "<<numberOfDataSource()<<" sources");
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
  LOG4CXX_INFO(_logZdaq,"Stopping the threads");
  //  printf("ZmMeger =>Stopping the threads \n");
  _gThread.join_all();

  // Do the stop of the the processors
  LOG4CXX_INFO(_logZdaq,"Stopping theprocessors");
  //printf("ZmMeger =>Stopping the processors \n");
  for (std::vector<zdaq::zmprocessor*>::iterator itp=_processors.begin();itp!=_processors.end();itp++)
    {
      (*itp)->stop();
    }
  LOG4CXX_INFO(_logZdaq,"Leaving Stop method");

}
void  zmMerger::processData(std::string idd,zmq::message_t *message)
{
  //printf("Processing %s  Map size %d \n",idd.c_str(),_eventMap.size());

  if (this->registered())
  _nDifs=this->registered();
  uint32_t detid,sid,gtc;
  uint64_t bx;
  sscanf(idd.c_str(),"DS-%d-%d %d %ld",&detid,&sid,&gtc,&bx);
  //fprintf(stderr,"Message %s DS-%d-%d %d %ld\n",idd.c_str(),detid,sid,gtc,bx);
  std::map<uint64_t,std::vector<zdaq::buffer*> >::iterator it_gtc=_eventMap.find(gtc);
  if (gtc%20==0)
      LOG4CXX_INFO(_logZdaq,"Event Map size "<<_eventMap.size());
  zdaq::buffer *b = new zdaq::buffer(512*1024);
  // uint32_t* iptr=(uint32_t*) message->data();
  //   uint8_t* cptr=(uint8_t*) message->data();
  //   uint64_t* iptr64=(uint64_t*) &cptr[12];
    // printf("Message 0) %x %d %d %ld \n",iptr[0],iptr[1],iptr[2],iptr64[0]);



  memcpy(b->ptr(),message->data(),message->size());
  b->setSize(message->size());
  _compressedSize+=b->payloadSize();
  // printf("Message 1) %d %d %d \n",b->detectorId(),b->dataSourceId(),b->eventId());
  b->uncompress();

  _totalSize+=b->payloadSize();
  // printf("Message 2) %d %d %d \n",b->detectorId(),b->dataSourceId(),b->eventId());
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
  int32_t lastgtc=0;
  for ( std::map<uint64_t,std::vector<zdaq::buffer*> >::iterator itm=_eventMap.begin();itm!=_eventMap.end();itm++)
    {
  if (itm->second.size()==this->numberOfDataSource())
    {
      //if (it_gtc->first%100==0)
      //printf("GTC %lu %lu  %d\n",itm->first,itm->second.size(),this->numberOfDataSource());
      this->processEvent(itm->first);
      lastgtc=itm->first;
    }
    }
  // Clear writen event
   for (std::map<uint64_t,std::vector<zdaq::buffer*> >::iterator it=_eventMap.begin();it!=_eventMap.end();)
	{
	  
	  if (it->second.size()==this->numberOfDataSource())
	    {
	      //std::cout<<"Deleting Event "<<it->first<<" Last gtc "<<lastgtc<<std::endl; 
	      for (std::vector<zdaq::buffer*>::iterator iv=it->second.begin();iv!=it->second.end();iv++) delete (*iv);
	      it->second.clear();
	      _eventMap.erase(it++);
	    }
	  else
	    it++;
	}
  
   // Clear old uncompleted event
   if (_purge)
     {
       if (gtc%20==0)
	 LOG4CXX_INFO(_logZdaq,"PURGING size "<<_eventMap.size());
       for (std::map<uint64_t,std::vector<zdaq::buffer*> >::iterator it=_eventMap.begin();it!=_eventMap.end();)
	 {
	  
	   if (it->first+1000<lastgtc)
	     {
	       //std::cout<<"Deleting Event "<<it->first<<" Last gtc "<<lastgtc<<std::endl; 
	       for (std::vector<zdaq::buffer*>::iterator iv=it->second.begin();iv!=it->second.end();iv++) delete (*iv);
	       it->second.clear();
	       _eventMap.erase(it++);
	     }
	   else
	     it++;
	 }
       // Force purge if size>200
       if (_eventMap.size()>200)
	 {
	   LOG4CXX_INFO(_logZdaq,"REAL PURGING size "<<_eventMap.size());
	   for (std::map<uint64_t,std::vector<zdaq::buffer*> >::iterator it=_eventMap.begin();it!=_eventMap.end();)
	     {
	  
	       if (it->first>1)
		 {
		   std::cout<<"Deleting Event "<<it->first<<" Last gtc "<<lastgtc<<" size "<<it->second.size()<< std::endl; 
		   for (std::vector<zdaq::buffer*>::iterator iv=it->second.begin();iv!=it->second.end();iv++) delete (*iv);
		   it->second.clear();
		   _eventMap.erase(it++);
		 }
	       else
		 it++;
	     }
	   LOG4CXX_INFO(_logZdaq,"END PURGING size "<<_eventMap.size());
	 }
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
      printf("%s => %lu \n",x.first.c_str(),x.second);
    }
}

Json::Value zmMerger::status()
{
  Json::Value jrep,jsta;
  jsta["run"]=_run;
  jsta["event"]=_evt;
  jsta["build"]=_build;
  jsta["compressed"]=_compressedSize;
  jsta["total"]=_totalSize;
  
  jsta["running"]=_running;
  jsta["purge"]=_purge;
  jsta["size"]=(uint32_t) _eventMap.size();
  jsta["registered"]=this->registered();
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
