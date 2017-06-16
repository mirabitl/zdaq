
#include <curl/curl.h>
#include <iostream>
#include <sstream>
#include <json/json.h>
#include <stdint.h>
#include "fsmwebCaller.hh"
using namespace std;
//char* CurlQuery(char* AddURL,char* Chaine);
char rfc3986[256] = {0};
char html5[256] = {0};

void url_encoder_rfc_tables_init(){

  int i;

  for (i = 0; i < 256; i++){

    rfc3986[i] = isalnum( i) || i == '~' || i == '-' || i == '.' || i == '_' ? i : 0;
    html5[i] = isalnum( i) || i == '*' || i == '-' || i == '.' || i == '_' ? i : (i == ' ') ? '+' : 0;
  }
}

char *url_encode( char *table, unsigned char *s, char *enc){

  for (; *s; s++){

    if (table[*s]) sprintf( enc, "%c", table[*s]);
    else sprintf( enc, "%%%02X", *s);
    while (*++enc);
  }

  return( enc);
}
size_t FsmCurlWrite_CallbackFunc_StdString(void *contents, size_t size, size_t nmemb, std::string *s){
  size_t newLength = size*nmemb;
  size_t oldLength = s->size();
  try{
    s->resize(oldLength + newLength);
  }
  catch(std::bad_alloc &e){
    //handle memory problem
    return 0;
  }
  
  std::copy((char*)contents,(char*)contents+newLength,s->begin()+oldLength);
  return size*nmemb;
}
std::string escapeJsonString(const std::string& input) {
  std::ostringstream ss;
  for (auto iter = input.cbegin(); iter != input.cend(); iter++) {
    //C++98/03:
    //for (std::string::const_iterator iter = input.begin(); iter != input.end(); iter++) {
    switch (*iter) {
    case '\\': ss << "\\\\"; break;
    case '"': ss << "\\\""; break;
    case '/': ss << "\\/"; break;
    case '\b': ss << "\\b"; break;
    case '\f': ss << "\\f"; break;
    case '\n': ss << "\\n"; break;
    case '\r': ss << "\\r"; break;
    case '\t': ss << "\\t"; break;
    default: ss << *iter; break;
    }
  }
  return ss.str();
}


fsmwebCaller::fsmwebCaller(std::string host,uint32_t port)
{
  std::stringstream s;
  s<<"http://"<<host<<":"<<port<<"/";
  // Check the prefix
  std::string rc=fsmwebCaller::curlQuery((char*) s.str().c_str());
  
  Json::Reader reader;
  _parseOk = reader.parse( rc, _jConfig);
  std::cout<<"fsmwebCaller failed => "<<s.str();
  if (!_parseOk) return;
  s<<_jConfig["PREFIX"].asString()<<"/";
  _url=s.str();
    std::cout<<"fsmwebCaller ok url => "<<_url;
}

std::string fsmwebCaller::queryState()
  {
    std::string rc=fsmwebCaller::curlQuery((char*) _url.c_str());
    
    Json::Reader reader;
    Json::Value jc;
    _parseOk = reader.parse( rc, jc);
    if (!_parseOk) return "UNKOWN";
    return jc["STATE"].asString();
   
  }
Json::Value fsmwebCaller::queryWebStatus()
  {
    std::string rc=fsmwebCaller::curlQuery((char*) _url.c_str());
    
    Json::Reader reader;
    Json::Value jc;
    _parseOk = reader.parse( rc, jc);
    if (!_parseOk) return Json::Value::null;
    return jc;
   
  }
 std::string fsmwebCaller::curlQuery(std::string url,std::string login)
  {
    CURL *curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    std::string s;
    if(curl) {
      curl_easy_setopt(curl, CURLOPT_URL,url.c_str());
      /* enable all supported built-in compressions */
      //curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
      /*Do not output result to stdout but to a local string object*/
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, FsmCurlWrite_CallbackFunc_StdString);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
      if (login.size()!=0)
	curl_easy_setopt(curl, CURLOPT_USERPWD,login.c_str());
      //      curl_easy_setopt(ch, CURLOPT_CUSTOMREQUEST, "POST");
      // curl_easy_setopt(ch, CURLOPT_POSTFIELDS, json_object_to_json_string(json));
      
      /* Perform the request, res will get the return code */ 
      res = curl_easy_perform(curl);
      /* Check for errors */

      if(res != CURLE_OK){
	ostringstream oss;
	oss<<"CURL connection failed : "<<curl_easy_strerror(res)<<endl;
      }

    
      /* always cleanup */ 
      curl_easy_cleanup(curl);
    
    }
    curl_global_cleanup();
    return s;

  }
std::string fsmwebCaller::sendTransition(std::string name,Json::Value cnt)
  {
    Json::Value content=cnt;
    //    printf("On envoie %s %s\n",name.c_str(),_url.c_str());
     std::stringstream ss;
     ss<<_url<<"FSM?command="<<name;
     std::cout<<content<<std::endl;
     // printf("1 Sending %s \n",ss.str().c_str());
     
     if (!content.isNull())
       {
	 //printf("content non null\n");
	 Json::FastWriter fastWriter;
	 //ss<<"&content="<<escapeJsonString(fastWriter.write(content));
	 std::string sc=fastWriter.write(content);
	 char out[4096];
	 url_encoder_rfc_tables_init();

	 url_encode( html5,(unsigned char*) sc.c_str(),out);
	 ss<<"&content="<<out;
	 //	 printf("2 Sending %s \n",ss.str().c_str());
       }
     else
       {
	 //printf("content  null\n");
	 ss<<"&content={}";
	 // printf("3 Sending %s \n",ss.str().c_str());
       }
     //std::cout<<"4 sending "<<ss.str()<<std::endl;
     //printf("Sending %s \n",ss.str().c_str());
     //     return "none";
   
     std::string rc=fsmwebCaller::curlQuery((char*) ss.str().c_str());
    //printf("return %s %s \n",ss.str().c_str(),rc.c_str());
    Json::Reader reader;
    Json::Value jsta;
    bool parsingSuccessful = reader.parse(rc,jsta);
    if (parsingSuccessful)
      if (jsta["content"].isMember("answer"))
	_answer=jsta["content"]["answer"];
      else
	_answer=Json::Value::null;
    else
      _answer=Json::Value::null;
    return rc;
  }
std::string fsmwebCaller::sendCommand(std::string name,std::string params)
  {
     std::stringstream s;
     s<<_url<<"CMD?name="<<name;
     if (params.length()>2)
       s<<params;
     //std::cout<<"SC Sending "<<s<<std::endl;
     std::string rc=fsmwebCaller::curlQuery((char*) s.str().c_str());
     //std::cout<<"SC received "<<rc<<std::endl;
     Json::Reader reader;
     Json::Value jsta;
     bool parsingSuccessful = reader.parse(rc,jsta);
     if (parsingSuccessful)
       {
	 //std::cout<<"SC parsing sucess"<<jsta<<std::endl;
       _answer=jsta;
       }
       else 
       _answer=Json::Value::null;
    return rc;


  }
Json::Value fsmwebCaller::answer(){return _answer;}
