#ifndef _FSMWEBCALLER_HH
#define _FSMWEBCALLER_HH
#include <curl/curl.h>
#include <iostream>
#include <sstream>
#include <json/json.h>
#include <stdint.h>
using namespace std;
//char* CurlQuery(char* AddURL,char* Chaine);


void url_encoder_rfc_tables_init();

 
char *url_encode( char *table, unsigned char *s, char *enc);
size_t FsmCurlWrite_CallbackFunc_StdString(void *contents, size_t size, size_t nmemb, std::string *s);
std::string escapeJsonString(const std::string& input); 

/**
   \class fsmwebCaller
   \brief A basic curl query interface to zdaq::fsmweb object
   
   \details It provides simple request for commands, transitions and status
   \author    Laurent Mirabito
   \version   1.0
   \date      January 2019
   \copyright GNU Public License.
*/
class fsmwebCaller
{
public:
  /**
     \brief Constructor

     \param host is the hostname where the zdaq::fsmweb is running
     \param port is the port of the fsmweb

     \details During creation it gets the full status of the fsmweb
   */
  fsmwebCaller(std::string host,uint32_t port);
 

  
  std::string queryState(); ///< make a query and get the current state
  Json::Value queryWebStatus();///< return the full status of the fsmweb

  /**
     \brief curl base query utility
   */
  static std::string curlQuery(std::string url,std::string login=std::string(""));

  /**
     \brief Send a transition command

     \param name of the transition
     \param cnt Json:Value of the content tag of the fsmmessage

     \details it fills the _answer private Json::Value with the return value of the query
   */
  std::string sendTransition(std::string name,Json::Value cnt=Json::Value::null);

  /**
     \brief Send a Standalone command

     \param name of the Command
     \param params is the cgi style parameters of the url

     \details it fills the _answer private Json::Value with the return value of the query
   */
  std::string sendCommand(std::string name,std::string params=std::string(""));
  
  Json::Value answer();///< Value of the _answer private Json::Value

  std::string url(){return _url;}
  std::string host(){return _host;}
  uint32_t port(){return _port;}
private:
  bool _parseOk;
  Json::Value _jConfig;
  std::string _url;
  Json::Value _answer;
  std::string _host;
  uint32_t _port;
};
#endif
