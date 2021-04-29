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
typedef boost::function<void (zdaq::fsmmessage*)> PFunctor; ///< A boost functor to a function with a fsmmessage argument 
namespace zdaq {

   /**
     \class zdaq::fsmTransition

     \brief Transition class between 2 states with boost::functor handler
       \author    Laurent Mirabito
     \version   1.0
     \date      January 2019
     \copyright GNU Public License.
  */
  class fsmTransition
  {
    public:

    /**
       \brief Constructor

       \param istate initial state name
       \param fstate final state name
       \param f Pfunctor called

       \details In an object of class MyObj with a methode create(zdaq::fsmmessage* m), the PFunctor is declared with 'boost::bind(&MyObj::create, this,_1)'

     */
      fsmTransition(std::string istate,std::string fstate,PFunctor f) : _istate(istate),_fstate(fstate),_callback(f) {}
    std::string initialState(){return _istate;} ///< Initial state name
    std::string finalState(){return _fstate;} ///< Final state name
    PFunctor callback(){return _callback;} ///< PFunctor access
    private:
      std::string _istate,_fstate;
      PFunctor _callback;
  };

   /**
     \class zdaq::fsm

     \brief Finite State Machine implementation
       \author    Laurent Mirabito
     \version   1.0
     \date      January 2019
     \copyright GNU Public License.
  */
  class fsm
  {
  public:

    fsm(std::string name); ///< Constructor

    /**
       \brief Handler of zdaq::fsmmessage, it processes the transition

       \param msg the transition message

       \details the fsmmessage contains 2 JSON object
         - The command,  a string with the transition name
	 - The content , a directory of parameters for the transition (user defined)
	 The msg contains an answer part that is filled with information from the transition

	\return the final state of the transition or ERROR
    
     */
    virtual std::string processCommand(zdaq::fsmmessage* msg);

    /**
       \brief Register a new state
       
       \param statename the name of the state
     */
    void addState(std::string statename);

    /**
       \brief register a transition
       \param cmd Transition name
       \param istate Initial state
       \param fstate Final state
       \param f PFunctor(see fsmTransition class)
     */
    void addTransition(std::string cmd,std::string istate,std::string fstate,PFunctor f);

    /**
       \brief Set the current state
       \param s the state name
     */
    void setState(std::string s);
    
    std::string state();///< Current state name
    void publishState();///< Unused

    /**
       \brief List all possible transitions
       \return JSON list of transitions
     */
    Json::Value transitionsList();
    
    /**
       \brief List all possible transitions for the current state
       \return JSON list of transitions
     */
    Json::Value allowList();
  private:
    std::vector<std::string> _states;
    std::string _state;
    std::map<std::string,std::vector<fsmTransition> > _transitions;
  };
};
#endif
