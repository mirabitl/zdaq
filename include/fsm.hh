#ifndef _zdaq_fsm_h
#define _zdaq_fsm_h

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <json/json.h>
#include <sstream>
#include <map>
#include <vector>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include "fsmmessage.hh"
using namespace zdaq;
typedef boost::function<void (zdaq::fsmmessage*)> PFunctor;
namespace zdaq {
  
  class fsmTransition
  {
    public:
      fsmTransition(std::string istate,std::string fstate,PFunctor f) : _istate(istate),_fstate(fstate),_callback(f) {}
      std::string initialState(){return _istate;}
      std::string finalState(){return _fstate;}
      PFunctor callback(){return _callback;}
    private:
      std::string _istate,_fstate;
      PFunctor _callback;
  };
 
  class fsm
  {
  public:

    fsm(std::string name);
    virtual std::string processCommand(zdaq::fsmmessage* msg);
    void addState(std::string statename);
    void addTransition(std::string cmd,std::string istate,std::string fstate,PFunctor f);
    void setState(std::string s);
    std::string state();
    void publishState();
    Json::Value transitionsList();
    Json::Value allowList();
  private:
    std::vector<std::string> _states;
    std::string _state;
    std::map<std::string,std::vector<zdaq::fsmTransition> > _transitions;
  };
};
#endif
