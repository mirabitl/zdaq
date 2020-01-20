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

  /**
   * \brief GETCONFIG command handler
   * \return the JSON configuration in the response['configuration'] 
   */
  void c_getconfiguration(Mongoose::Request &request, Mongoose::JsonResponse &response);

  /**
   * \brief GETPARAM command handler
   * \return the JSON Parameter set in the response['PARAMETER']
  */
  void c_getparameter(Mongoose::Request &request, Mongoose::JsonResponse &response);

   /**
   * \brief SETPARAM command handler
   * \details it replaces the parameters set with the content of the CGI PARAMETER value
   * \return the JSON Parameter set in the response['PARAMETER']
  */
  void c_setparameter(Mongoose::Request &request, Mongoose::JsonResponse &response);

  /**
   * \brief INFO command handler
   * \details it returns general information on the application
   * \return the Info set in the response['INFO']
  */
  void c_info(Mongoose::Request &request, Mongoose::JsonResponse &response);

  /**
   * \brief virtual user handler to the CREATE command, called after the create method
   */
  virtual void  userCreate(zdaq::fsmmessage* m);

  /**
   * \brief  JSON configuration getter
   * \return  JSON configuration
   */
  Json::Value configuration();

  /**
   * \brief  JSON Parameter set getter
   * \return  JSON Parameter set
   */
  Json::Value& parameters();

  /**
   * \brief  JSON  Information set getter
   * \return  JSON Inforamtion set
   */  Json::Value& infos();


  /**
   * \brief Pointer to the zdaq::fsmweb
   */
  fsmweb* fsm();

  /**
   * \brief  Application instance
   */
  uint32_t instance(){return _instance;}

   /**
   * \brief  Application port
   */
  uint32_t port(){return _port;}

  /**
   * \brief login (if any)
   */
  std::string login(){return _login;}

  /**
   * \brief IP host name
   */
  std::string host() {return _hostname;}

  /**
   * \brief process name
   */
  std::string name() {return _processName;}


  /**
   * \brief Parses the configuration and find all other application with name specified
   * \param aname is the application name to search for
   * 
   * The vector of fsmwebCaller is filled for each instance found.
   */
  void findApps(std::string aname);

  /**
   * \brief the vector of fsmwebCaller filled with findApps
   */
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
