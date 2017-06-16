#ifndef _zdaqFsmweb_hh
#define _zdaqFsmweb_hh
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <mongoose/Server.h>
#include <mongoose/JsonController.h>
#include "fsm.hh"
using namespace std;
using namespace Mongoose;

typedef boost::function<void (Mongoose::Request&,Mongoose::JsonResponse&)> MGRFunctor;
namespace zdaq {
class FSMMongo;  
class fsmweb : public zdaq::fsm
{
public:
  fsmweb(std::string name);
  void serving(uint32_t port);
  void start(uint32_t port);
  void stop();
  void addCommand(std::string s,MGRFunctor f);
  void handleRequest(Request &request, JsonResponse &response);
  Json::Value commandsList();
  virtual std::string processCommand(zdaq::fsmmessage* msg);
private:
  FSMMongo* _service;
  bool _running;
  boost::thread_group g_d;
  std::map<std::string,MGRFunctor> _commands;
  std::string _webName,_name;
};


class FSMMongo : public JsonController
{
public:
  FSMMongo(std::string name,fsmweb *f);

  void fsmProcess(Request &request, JsonResponse &response);
  void cmdProcess(Request &request, JsonResponse &response);
  void List(Request &request, JsonResponse &response);
  void setup();
private:

  std::string _name;
  fsmweb* _fsm;
};
};

#endif
