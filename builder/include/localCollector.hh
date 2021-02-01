#ifndef _zdaq_localCollector_h
#define _zdaq_localCollector_h
#include "baseApplication.hh"
#include "zmMerger.hh"
#include "binarywriter.hh"
#include "zdaqLogger.hh"

namespace zdaq
{
  namespace builder {
    class collector : public zdaq::baseApplication {
    public:
      collector(std::string name);
      void configure(zdaq::fsmmessage* m);
      void start(zdaq::fsmmessage* m);
      void stop(zdaq::fsmmessage* m);
      void halt(zdaq::fsmmessage* m);
      void destroy(zdaq::fsmmessage* m);

      void status(Mongoose::Request &request, Mongoose::JsonResponse &response);

      void c_setheader(Mongoose::Request &request, Mongoose::JsonResponse &response);
      void c_purge(Mongoose::Request &request, Mongoose::JsonResponse &response);

    private:
      zdaq::zmMerger* _merger;
      bool _running,_readout;
      zmq::context_t* _context;
    };
  };
};
#endif


