#ifndef _zdaq_fsmmessage_h
#define _zdaq_fsmmessage_h

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <json/json.h>
namespace zdaq {
  /**
     \class zdaq::fsmmessage
     \brief The message object used for zdaq::fsm transition control
     
     \details It handles a Json::Value with a dictionnary of 3 parts:
     - command is the name of the transition
     - content is a set of parameters used for the transition, s["content"]["answer"] is filled with results of the transition
     - status is the status  of the transition (OK,FAILED,NOTSET)
     \author    Laurent Mirabito
     \version   1.0
     \date      January 2019
     \copyright GNU Public License.
  */
  class fsmmessage
  {
  public:
    fsmmessage(); ///< Constructor

    /**
       \brief Constructor
       
       \param s is a JSON string parsed to fill the message
     */
    fsmmessage(std::string s);
      
    std::string& value(); ///< JSON value of the message
    std::string command(); ///< _jsroot["command"]
    Json::Value content();///< _jsroot["content"]
    Json::Value status();///<_jsroot["status"]

    /**
       \brief Fill the content["answer"] with a Json::Value

       \param rep Json::Value to store
     */
    void setAnswer(Json::Value rep);

    /**
       \brief Overwrite message content

       \param s is a JSON string parsed to fill the message
     */
    void setValue(std::string s);
  private:
    std::string _sroot;
    Json::Value _jsroot;
  };
};
#endif
