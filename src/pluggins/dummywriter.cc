#include "dummywriter.hh"
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
#include "zdaqLogger.hh"


using namespace zdaq;

dummywriter::dummywriter(std::string dire) : _directory(dire),_run(0),_started(false),_fdOut(-1),_totalSize(0),_event(0),_dummy(false) {}
void dummywriter::start(uint32_t run)
{
  _run=run; 

  char dateStr [64];
            
  time_t tm= time(NULL);
  strftime(dateStr,20,"SMM_%d%m%y_%H%M%S",localtime(&tm));
  std::cout<<_directory<<"/"<<dateStr<<"_"<<run<<".dat"<<std::endl;
  _event=0;
  _started=true;
}
void dummywriter::loadParameters(Json::Value params)
{
  if (params.isMember("directory"))
    _directory=params["directory"].asString();
   if (params.isMember("dummy"))
     _dummy=(params["dummy"].asUInt()!=0);
}
void dummywriter::stop()
{
  _started=false;

}
uint32_t dummywriter::totalSize(){return _totalSize;}
uint32_t dummywriter::eventNumber(){return _event;}
uint32_t dummywriter::runNumber(){return _run;}
void dummywriter::processRunHeader(std::vector<uint32_t> header)
{
                
}
void dummywriter::processEvent(uint32_t key,std::vector<zdaq::buffer*> vbuf)
{

  if (!_started) return;
  uint32_t theNumberOfDIF=vbuf.size();
  if (key%1000==0)
    LOG4CXX_DEBUG(_logZdaq," Event :"<<key<<" Processed");


}
extern "C" 
{
    // loadDHCALAnalyzer function creates new LowPassDHCALAnalyzer object and returns it.  
  zdaq::zmprocessor* loadProcessor(void)
    {
      return (new zdaq::dummywriter);
    }
    // The deleteDHCALAnalyzer function deletes the LowPassDHCALAnalyzer that is passed 
    // to it.  This isn't a very safe function, since there's no 
    // way to ensure that the object provided is indeed a LowPassDHCALAnalyzer.
  void deleteProcessor(zdaq::zmprocessor* obj)
    {
      delete obj;
    }
}
