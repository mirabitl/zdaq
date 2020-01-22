#ifndef _exRunControl_h
#define _exRunControl_h
#include "fsmwebCaller.hh"
#include "baseApplication.hh"
#include <string>
#include <vector>
#include <json/json.h>
#include "zdaqLogger.hh"

namespace zdaq
{
  namespace example
  {
    class exRunControl : public zdaq::baseApplication
    {
    public:
      exRunControl(std::string name);
      ~exRunControl();
 
  
  
      void  initialise(zdaq::fsmmessage* m);
      void  configure(zdaq::fsmmessage* m);
      void  start(zdaq::fsmmessage* m);
      void  stop(zdaq::fsmmessage* m);
      void  halt(zdaq::fsmmessage* m);
      void  discover();


      void singleconfigure(fsmwebCaller* d);
      void singlestart(fsmwebCaller* d);
      void singlestop(fsmwebCaller* d);

      Json::Value json_builder_status();
      Json::Value json_exServer_status();
      Json::Value json_status();
  
      void  c_listProcess(Mongoose::Request &request, Mongoose::JsonResponse &response);

      void  c_builderStatus(Mongoose::Request &request, Mongoose::JsonResponse &response);
      void  c_exServerStatus(Mongoose::Request &request, Mongoose::JsonResponse &response);
      void c_status(Mongoose::Request &request, Mongoose::JsonResponse &response);
      void c_changeState(Mongoose::Request &request, Mongoose::JsonResponse &response);
 
  
      std::string state(){return _fsm->state();}


      // Virtual from baseAPplication
      virtual void  userCreate(zdaq::fsmmessage* m);

  
    private:
      zdaq::fsmweb* _fsm;
      fsmwebCaller *_triggerClient,*_builderClient;
      std::vector<fsmwebCaller*> _exServerClients;
 
  
      uint32_t _run;
      Json::Value _jConfigContent;
  
  

    };
  };
};
#endif
