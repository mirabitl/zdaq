#ifndef _baseZdaqApplication_hh
#define _baseZdaqApplication_hh
#include "fsmweb.hh"
#include "fsmwebCaller.hh"
#include <map>
#include <string>
#include <vector>
#include <json/json.h>
namespace zdaq {  
  /**
     \class baseApplication
     \brief A generic application object which provides a FSM and parameters setting
     \details It provides 
        - a zdaq::fsmweb object with 2 predefined states VOID and CREATED
        the CREATE transition parse the JSON daq configuration specified in the transition zdaq::fsmmessage,
        the application identifies itself ans set its paramaters
        - a virtual method userCreate to add additional work during CREATE transition
        - A parameter set 
    \author    Laurent Mirabito
    \version   1.0
    \date      January 2019
    \copyright GNU Public License.
  */

class baseApplication
{
public:

  /**
   * \brief Constructor
   * \param name is the name of the zdaq::fsmweb web service
   */
  baseApplication(std::string name);

  /**
   * \brief CREATE transition handler
   * \param m is the zdaq::fsmmessage received for the transition, it should
   * contain a tag 'url' or 'file' or 'mongo' in its 'content' directory to access the daq configuration file  
   */
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
  void findApps(std::string aname);
  std::vector<fsmwebCaller*>& callers() {return _apps;}
protected:
  zdaq::fsmweb* _fsm;
  std::string _hostname;
  std::string _login;
  std::string _processName;
  Json::Value _jConfig;
  Json::Value _jParam;
  Json::Value _jInfo;
  uint32_t _instance,_port;
  std::vector<fsmwebCaller*> _apps;
  
};
};
#endif
