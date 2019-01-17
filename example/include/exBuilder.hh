#ifndef _exBuilder_h
#define _exBuilder_h

#include "baseApplication.hh"
#include "zmMerger.hh"
#include "binarywriter.hh"

namespace zdaq {
  class exBuilder : public zdaq::baseApplication {
  public:
    exBuilder(std::string name);
    void configure(zdaq::fsmmessage* m);
    void start(zdaq::fsmmessage* m);
    void stop(zdaq::fsmmessage* m);
    void halt(zdaq::fsmmessage* m);
    void destroy(zdaq::fsmmessage* m);

    void status(Mongoose::Request &request, Mongoose::JsonResponse &response);
    void registerds(Mongoose::Request &request, Mongoose::JsonResponse &response);
    void c_setheader(Mongoose::Request &request, Mongoose::JsonResponse &response);

  private:
    zdaq::zmMerger* _merger;
    bool _running,_readout;
    zmq::context_t* _context;
    
  };
};
#endif
