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

typedef boost::function<void (Mongoose::Request&,Mongoose::JsonResponse&)> MGRFunctor;  ///< A boost functor to a function with Mongoose request and response arguments
namespace zdaq {
class FSMMongo;

  /**
     \class zdaq::fsmweb
     \brief A Mongoose-cpp web server binding of zdaq::fsm Finite State  Machine

     \details It starts a web server and Mongoose JsonController handler taht implements
      - a web service associated to the fsm.
      - a web service asociated to standalone command
     \author    Laurent Mirabito
     \version   1.0
     \date      January 2019
     \copyright GNU Public License.
   */
class fsmweb : public zdaq::fsm
{
public:
  /**
     \brief Constructor

     \param name of the webservice to be started
   */
  fsmweb(std::string name);

  /**
     \brief Start a Mongoose webserver 

     \param port of the web server
   */
  void serving(uint32_t port);

  /**
     \brief Start the webserver (serving method) in a thread

     \param port of the webserver
   */
  void start(uint32_t port);

  
  void stop();///< Stop the server

  /**
     \brief Register Stand alone command
       \param s is the command name
       \param f MGRFunctor called

       \details In an object of class MyObj with a methode setLvOn(Request &request, JsonResponse &response), the MGRFunctor is declared with 'boost::bind(&MyObj::setLvOn, this,_1,_2)'

       The Request object handles the cgi parameter of the http request:
         
          uint32_t nbclock=atol(request.get("nclock","10"))

       The JsonResponse is a jsoncpp Json::Value object containing the Json answer to the http request

   */
  void addCommand(std::string s,MGRFunctor f);

  /**
     \brief Standalone command handler

     \details if the command exists it response["status"]="OK" and response["answer] is the JsonResponse of the MGRFunctor register, otherwie status is "NOTSET"
   */
  void handleRequest(Request &request, JsonResponse &response);
  Json::Value commandsList(); ///< List available standalone commands 
  virtual std::string processCommand(zdaq::fsmmessage* msg); ///< zdaq::fsm transition handler
private:
  FSMMongo* _service;
  bool _running;
  boost::thread_group g_d;
  std::map<std::string,MGRFunctor> _commands;
  std::string _webName,_name;
};

  /**
     \class zdaq::FSMMongo
     \brief A Mongoose-cpp JsonController that implements the web services
     
     \details The setup method implemts 3 services handling:
     - FSM? the handling of FSM transitions
     - CMD? the handling of Standalone commands
     - ./ The list of available services
     \author    Laurent Mirabito
     \version   1.0
     \date      January 2019
     \copyright GNU Public License.
  */
class FSMMongo : public JsonController
{
public:
  /**
     \brief Cosntructor
     
     \param name is the service prefix
     \param f is a pointer to a fsmweb object
   */
  FSMMongo(std::string name,fsmweb *f);

  /**
     \brief handler of FSM? request

     \details if the request object contains a valid 'command' transition, it creates a zdaq::fsmnessage and call the fsm processCommand method
     The response contains the value of the fsmmessage after the transition
   */
  void fsmProcess(Request &request, JsonResponse &response);

  /**
     \brief handler of CMD? request

     \details it calls the fsm handleRequest method
   */  
  void cmdProcess(Request &request, JsonResponse &response);

  /**
     \brief Handler of the './' request

     \details it fills response with name,state,transitions list, allowed transitions list and command list
   */
  void List(Request &request, JsonResponse &response);

  /**
     \brief services Paths declaration
     \details Internally called by the server when the object is registered
   */
  void setup();
private:

  std::string _name;
  fsmweb* _fsm;
};
};

#endif
