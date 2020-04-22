
#ifndef EXSERVER_HH
# define EXSERVER_HH
#include "baseApplication.hh"
# include "zmSender.hh"
#include "zSubscriber.hh"
#include "zdaqLogger.hh"


namespace zdaq {
  namespace example {
    class exServer :public zdaq::baseApplication {
    public:
      exServer(std::string name);
      void initialise(zdaq::fsmmessage* m);
      void configure(zdaq::fsmmessage* m);
      void start(zdaq::fsmmessage* m);
      void stop(zdaq::fsmmessage* m);
      void halt(zdaq::fsmmessage* m);
      void streamdata(zdaq::zmSender *ds);
      void generate(Mongoose::Request &request, Mongoose::JsonResponse &response);
      void status(Mongoose::Request &request, Mongoose::JsonResponse &response);
      void incrementEvent() {_event++;_bx++;}
      bool running(){return _running;}
      inline void setDetectorId(uint32_t id) {_detid=id;}
      inline uint32_t getDetectorId() {return _detid;}
      void checkTrigger(std::vector<zdaq::mon::publishedItem*>& items);
      void fillEvent(uint32_t event,uint64_t bx,zdaq::zmSender* ds,uint32_t eventSize);
      void discover();
    private:
      zdaq::fsmweb* _fsm;
      uint32_t _detid;
      std::map<uint32_t,std::string> _mStream;
      std::vector<zdaq::zmSender*> _sources;
      std::map<uint32_t,uint32_t> _stat;
      bool _running,_readout;
      boost::thread_group _gthr;
      uint32_t _event;
      uint64_t _bx;
      zmq::context_t* _context;
      uint32_t _plrand[0x20000];
      // Trigger Polling
  
      zdaq::mon::zSubscriber* _triggerSubscriber;
    };
  };
};
#endif

