#ifndef _zdaq_localCollector_h
#define _zdaq_localCollector_h

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
namespace zdaq {
 
  class localCollector : public zmPuller
  {
  public:

    localCollector(zmq::context_t* c);
    ~localCollector();
    void virtual processData(std::string idd,zmq::message_t *message);
    void scanMemory();
    void registerBuilder(std::string name);
    void registerDataSource(std::string url);

    void start(uint32_t nr);
    void stop();
    Json::Value status();
  private:
    std::vector<zmq::socket_t* > _builders;
    boost::thread_group _gThread;
    bool _running;
    uint32_t _run,_evt,_packet;
  };
};
#endif
