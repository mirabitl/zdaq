#ifndef _baseZdaqApplication_hh
#define _baseZdaqApplication_hh
#include "fsmweb.hh"
#include <string>
#include <vector>
#include <json/json.h>
namespace zdaq {
class baseApplication
{
public:
  baseApplication(std::string name);
  void  create(zdaq::fsmmessage* m);
  void c_getconfiguration(Mongoose::Request &request, Mongoose::JsonResponse &response);
  void c_getparameter(Mongoose::Request &request, Mongoose::JsonResponse &response);
  void c_setparameter(Mongoose::Request &request, Mongoose::JsonResponse &response);
  void c_info(Mongoose::Request &request, Mongoose::JsonResponse &response);
  virtual void  userCreate(zdaq::fsmmessage* m);
  Json::Value configuration();
  Json::Value& parameters();
  Json::Value& infos();
  fsmweb* fsm();
  uint32_t instance(){return _instance;}
  uint32_t port(){return _port;}
  std::string login(){return _login;}
  std::string host() {return _hostname;}
  std::string name() {return _processName;}
protected:
  zdaq::fsmweb* _fsm;
  std::string _hostname;
  std::string _login;
  std::string _processName;
  Json::Value _jConfig;
  Json::Value _jParam;
  Json::Value _jInfo;
  uint32_t _instance,_port;
  
};
};
#endif
