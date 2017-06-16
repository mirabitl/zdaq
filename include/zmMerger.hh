#ifndef _zdaq_zmmerger_h
#define _zdaq_zmmerger_h

#include <stdint.h>
#include <stdlib.h>
#include "zmBuffer.hh"
#include "zmPuller.hh"
#include <vector>
#include <map>
#include <string>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <json/json.h>

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#define dskey(d,s) ( (s&0xFFF) | ((d &0xFFF)<<12))
#define source_id(k) (k&0xFFF)
#define detector_id(k) ((k>>12)&0xFFF)
namespace zdaq {
  class zmprocessor
  {
  public:
    virtual void start(uint32_t run)=0;
    virtual void stop()=0;
    virtual  void processEvent(uint32_t key,std::vector<zdaq::buffer*> dss)=0;
    virtual  void processRunHeader(std::vector<uint32_t> header)=0;
  };

  class zmMerger : public zmPuller
  {
  public:

    zmMerger(zmq::context_t* c);
    ~zmMerger();
    void virtual processData(std::string idd,zmq::message_t *message);
    void clear();
    void scanMemory();
    void registerProcessor(std::string name);
    void registerDataSource(std::string url);
    inline uint32_t numberOfDataSource(){return _nDifs;}
    inline uint32_t setNumberOfDataSource(uint32_t k){_nDifs=k;}
    uint32_t numberOfDataPacket(uint32_t k);
    void unregisterProcessor(zdaq::zmprocessor* p);
    
    void start(uint32_t nr);
    void processEvent(uint32_t id);
    void processRunHeader();
    std::vector<uint32_t>& runHeader(){return _runHeader;}
    void stop();
    void summary();
    Json::Value status();
  private:
    bool _useEventId;
    uint32_t _nDifs;
    std::vector<zmprocessor* > _processors;
    std::map<uint64_t,std::vector<zdaq::buffer*> > _eventMap;
	
    boost::thread_group _gThread;
    bool _running;
    uint32_t _run,_evt,_build;
    std::vector<uint32_t> _runHeader;

    std::map<std::string,uint64_t> _mReceived;
  };
};
#endif
