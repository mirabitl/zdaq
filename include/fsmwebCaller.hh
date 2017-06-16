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

class fsmwebCaller
{
public:
  fsmwebCaller(std::string host,uint32_t port);
 

  std::string queryState();
  Json::Value queryWebStatus();
  static std::string curlQuery(std::string url,std::string login=std::string(""));
  std::string sendTransition(std::string name,Json::Value cnt=Json::Value::null);
  std::string sendCommand(std::string name,std::string params=std::string(""));
  Json::Value answer();
private:
  bool _parseOk;
  Json::Value _jConfig;
  std::string _url;
  Json::Value _answer;
};
#endif
