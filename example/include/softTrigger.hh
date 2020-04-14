#ifndef softTrigger_HH
# define softTrigger_HH
#include "baseApplication.hh"
#include "zPublisher.hh"
#include "zdaqLogger.hh"


namespace zdaq {
  namespace example {
    class softTrigger :public zdaq::baseApplication {
    public:
      softTrigger(std::string name);
      void configure(zdaq::fsmmessage* m);
      void start(zdaq::fsmmessage* m);
      void stop(zdaq::fsmmessage* m);
      void halt(zdaq::fsmmessage* m);
      void publishingThread();
      Json::Value status();
      void c_status(Mongoose::Request &request, Mongoose::JsonResponse &response);
      void c_period(Mongoose::Request &request, Mongoose::JsonResponse &response);
      void c_size(Mongoose::Request &request, Mongoose::JsonResponse &response);
      void c_ntrg(Mongoose::Request &request, Mongoose::JsonResponse &response);
      bool running(){return _running;}
    private:
      bool _running,_readout;
      boost::thread_group _gthr;
      uint32_t _event;
      uint64_t _bx;
      zmq::context_t* _context;
      uint32_t _tcpPort;
      uint32_t _microsleep;
      uint32_t _datasize;
      uint32_t _ntrg;
      std::string _hardware;
      std::string _location;

      // Trigger publication

      zdaq::mon::zPublisher* _triggerPublisher;
    };
  };
};
#endif

