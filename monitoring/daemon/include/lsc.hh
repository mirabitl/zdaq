#ifndef _zdaq_lsc_h
#define _zdaq_lsc_h
#include "fsmweb.hh"
#include "zmonPlugin.hh"
#include "zmonStore.hh"
#include "zdaqLogger.hh"
#include <zmq.hpp>

namespace zdaq
{
  class lsc  {
  public:
    lsc(std::string name,uint32_t port);
    void configure(zdaq::fsmmessage* m);
    void start(zdaq::fsmmessage* m);
    void stop(zdaq::fsmmessage* m);
    void monitor();
    void parseDefaults(zdaq::fsmmessage *m);
    void status(Mongoose::Request &request, Mongoose::JsonResponse &response);
    
    void registerPlugin(std::string name);
    void registerStore(std::string name);
    
  private:
    fsmweb* _fsm;
    bool _running,_readout;
    uint32_t _period;
    Json::Value _params;
    std::vector<zdaq::zmonPlugin* > _plugins;
    std::vector<zdaq::zmonStore* > _stores;
    zmq::context_t* _context;      
    zmq::socket_t *_publisher;
    boost::thread_group g_store;



  };

};
#endif


