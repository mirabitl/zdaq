#ifndef _zdaq_binarywriter_h
#define _zdaq_binarywriter_h

#include <stdint.h>
#include <stdlib.h>
#include "zmBuffer.hh"
#include "zmMerger.hh"
#include <vector>
#include <map>
#include <string>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
namespace zdaq {

  class binarywriter : public zmprocessor
  {
  public:
    binarywriter(std::string dire="/tmp");
    virtual void start(uint32_t run);
    virtual void stop();
    virtual  void processEvent(uint32_t key,std::vector<zdaq::buffer*> dss);
    virtual  void processRunHeader(std::vector<uint32_t> header);
    
    uint32_t totalSize();
    uint32_t eventNumber();
    uint32_t runNumber();
  private:
    std::string _directory;
    uint32_t _run,_event,_totalSize;
    int32_t _fdOut;
    bool _started;
  };
};
#endif
