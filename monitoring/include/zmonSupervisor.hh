#ifndef _zdaq_zmonSupervisor_h
#define _zdaq_zmonSupervisor_h
#include "baseApplication.hh"
#include "zmonPlugin.hh"
#include "zdaqLogger.hh"

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
      void destroy(zdaq::fsmmessage* m);

      void status(Mongoose::Request &request, Mongoose::JsonResponse &response);
      void registerPlugin(std::string name);

    private:

      bool _running,_readout;
      zmq::context_t* _context;
      std::vector<zmonPlugin* > _plugins;

    };
  };
};
#endif


