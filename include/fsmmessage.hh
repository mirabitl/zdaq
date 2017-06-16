#ifndef _zdaq_fsmmessage_h
#define _zdaq_fsmmessage_h

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <json/json.h>
namespace zdaq {

  class fsmmessage
  {
  public:
    fsmmessage();
    fsmmessage(std::string s);
      
    std::string& value();
    std::string command();
    Json::Value content();
    Json::Value status();
    void setAnswer(Json::Value rep);
    void setValue(std::string s);
  private:
    std::string _sroot;
    Json::Value _jsroot;
  };
};
#endif
