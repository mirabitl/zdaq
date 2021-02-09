#ifndef _zdaq_zmonSupervisor_h
#define _zdaq_zmonSupervisor_h
#include "baseApplication.hh"
#include "zmonPlugin.hh"
#include "zdaqLogger.hh"
#include <zmq.hpp>

namespace zdaq
{
  namespace monitoring {
    class supervisor : public zdaq::baseApplication {
    public:
      supervisor(std::string name);
      void configure(zdaq::fsmmessage* m);
      void start(zdaq::fsmmessage* m);
      void stop(zdaq::fsmmessage* m);
      void halt(zdaq::fsmmessage* m);
      void monitor();

      void status(Mongoose::Request &request, Mongoose::JsonResponse &response);
      void registerPlugin(std::string name);
      void lock(){theSync_.lock();}
      void unlock(){theSync_.unlock();}
    private:

      bool _running,_readout;
      uint32_t _period;

      std::vector<zdaq::zmonPlugin* > _plugins;
      zmq::context_t* _context;      
      zmq::socket_t *_publisher;
      boost::thread_group g_store;
      boost::interprocess::interprocess_mutex theSync_;


    };
  };
};
#endif


