#include "binarywriter.hh"
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
binarywriter::binarywriter(std::string dire) : _directory(dire),_run(0),_started(false),_fdOut(-1),_totalSize(0),_event(0) {}
void binarywriter::start(uint32_t run)
{
  _run=run; 
  std::stringstream filename("");    
  char dateStr [64];
            
  time_t tm= time(NULL);
  strftime(dateStr,20,"SMM_%d%m%y_%H%M%S",localtime(&tm));
  filename<<_directory<<"/"<<dateStr<<"_"<<run<<".dat";
  _fdOut= ::open(filename.str().c_str(),O_CREAT| O_RDWR | O_NONBLOCK,S_IRWXU);
  if (_fdOut<0)
    {
      perror("No way to store to file :");
      //std::cout<<" No way to store to file"<<std::endl;
      return;
    }  
  _event=0;
  _started=true;
}
void binarywriter::loadParameters(Json::Value params)
{
  if (params.isMember("directory"))
    _directory=params["directory"].asString();
}
void binarywriter::stop()
{
  _started=false;
  ::sleep(1);
  if (_fdOut>0)
    {
      ::close(_fdOut);
      _fdOut=-1;
    }


}
uint32_t binarywriter::totalSize(){return _totalSize;}
uint32_t binarywriter::eventNumber(){return _event;}
uint32_t binarywriter::runNumber(){return _run;}
void binarywriter::processRunHeader(std::vector<uint32_t> header)
{
  if (_fdOut>0 && header.size()<256)
    {
      uint32_t ibuf[256];
      for (int i=0;i<header.size();i++) ibuf[i]=header[i];
      int ier=write(_fdOut,&_run,sizeof(uint32_t));
      int nb=1;
      ier=write(_fdOut,&nb,sizeof(uint32_t));
      // Construct one zdaq buffer with header content
      zdaq::buffer b(128+header.size());
      b.setDetectorId(255);
      b.setDataSourceId(1);
      b.setEventId(0);
      b.setBxId(0);
      b.setPayload(ibuf,header.size()*sizeof(uint32_t));
      b.compress();
      uint32_t bsize=b.size();
      _totalSize+=bsize;
      ier=write(_fdOut,&bsize,sizeof(uint32_t));
      ier=write(_fdOut,b.ptr(),bsize);
    }
                
}
void binarywriter::processEvent(uint32_t key,std::vector<zdaq::buffer*> vbuf)
{

  if (!_started) return;
  uint32_t theNumberOfDIF=vbuf.size();
  if (_event%100==0) 
    std::cout<<"Standard completion "<<_event<<" GTC "<<key<<" size "<<_totalSize<<std::endl;
  // To be implemented
  printf("Event %d key %d writing TotalSize %d :\n",_event,key,_totalSize);fflush(stdout);
  if (_fdOut>0)
    {
      int ier=write(_fdOut,&_event,sizeof(uint32_t));
      ier=write(_fdOut,&theNumberOfDIF,sizeof(uint32_t));
      for (std::vector<zdaq::buffer*>::iterator iv=vbuf.begin();iv!=vbuf.end();iv++) 
	{
	  printf("\t writing %d bytes \n",(*iv)->size());
	  (*iv)->compress();
	  uint32_t bsize=(*iv)->size();
	  printf("\t writing %d compressed bytes\n",bsize);fflush(stdout);
	  _totalSize+=bsize;
	  ier=write(_fdOut,&bsize,sizeof(uint32_t));
	  ier=write(_fdOut,(*iv)->ptr(),bsize);
	}
                
                

      _event++;
    }

  if (_totalSize>1900*1024*1024)
    {
      ::close(_fdOut);
      _totalSize=0;
      this->start(_run);
    }


}
extern "C" 
{
    // loadDHCALAnalyzer function creates new LowPassDHCALAnalyzer object and returns it.  
  zdaq::zmprocessor* loadProcessor(void)
    {
      return (new zdaq::binarywriter);
    }
    // The deleteDHCALAnalyzer function deletes the LowPassDHCALAnalyzer that is passed 
    // to it.  This isn't a very safe function, since there's no 
    // way to ensure that the object provided is indeed a LowPassDHCALAnalyzer.
  void deleteProcessor(zdaq::zmprocessor* obj)
    {
      delete obj;
    }
}
