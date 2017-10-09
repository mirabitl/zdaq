#include "fsm.hh"
using namespace zdaq;
fsm::fsm(std::string name) : _state("CREATED")
{
  _states.clear();
  _transitions.clear();
    
}


void fsm::addState(std::string statename) 
{
  _states.push_back(statename);
}
  
void fsm::addTransition(std::string cmd,std::string istate,std::string fstate,PFunctor f)
{
  if (_transitions.find(cmd)!=_transitions.end())
    {
      std::map<std::string,std::vector<zdaq::fsmTransition> >::iterator iv=_transitions.find(cmd);
      bool found=false;
      for (std::vector<zdaq::fsmTransition>::iterator it=iv->second.begin();it!=iv->second.end();it++)
	{
	  if (it->initialState().compare(istate)==0)
	    {/*already stor */ return;}
	}
      zdaq::fsmTransition t(istate,fstate,f);;
      iv->second.push_back(t);

    }
  else
    {
      zdaq::fsmTransition t(istate,fstate,f);
      std::vector<zdaq::fsmTransition> vp;
      vp.push_back(t);
      std::pair<std::string,std::vector<zdaq::fsmTransition> > p(cmd,vp);
      _transitions.insert(p);
    }
}

void fsm::setState(std::string s){ _state=s;}
std::string fsm::state(){return _state;}
void fsm::publishState() {}

std::string fsm::processCommand(zdaq::fsmmessage* msg)
{
  std::map<std::string,std::vector<zdaq::fsmTransition> >::iterator it=_transitions.find(msg->command());
  if (it==_transitions.end())
    {
      Json::Value jrep;
      jrep["command"]=msg->command();
      jrep["status"]="FAILED";
      std::stringstream s0;
      s0.str(std::string());  
      s0<<msg->command()<<" not found in transitions list ";
      jrep["content"]=msg->content();
      jrep["content"]["answer"]=s0.str();
      Json::FastWriter fastWriter;
      msg->setValue(fastWriter.write(jrep));
      return "ERROR";
    }
  else
    {
      // loop on vector of transition
      std::vector<zdaq::fsmTransition> &vp=it->second;
      for (std::vector<zdaq::fsmTransition>::iterator ift=vp.begin();ift!=vp.end();ift++)
	if (ift->initialState().compare(_state)==0)
	  {
#ifdef DEBUG
	    std::cout<<"calling callback"<<ift->finalState()<<"\n";
#endif
	    ift->callback()(msg);
#ifdef DEBUG
	    std::cout<<"Message processed\n";
#endif
	    _state=ift->finalState();
	    this->publishState();
	    Json::Value jrep;
	    std::stringstream s0;
	    s0.str(std::string());  
	    s0<<msg->command()<<"_DONE";

	    jrep["command"]=msg->command();
	    jrep["status"]="DONE";
	    jrep["content"]=msg->content();
	    //jrep["content"]["msg"]="OK";
	    Json::FastWriter fastWriter;
	    msg->setValue(fastWriter.write(jrep));
#ifdef DEBUG
	    std::cout<<"RC "<<jrep<<std::endl;
#endif
	    return _state;

	  }
      // No initialState corresponding to _state
      //if (it->second.initialState().compare(_state)!=0)
      //  {
      Json::Value jrep;
      jrep["command"]=msg->command();
      jrep["status"]="FAILED";
      std::stringstream s0;
      s0.str(std::string());  
      s0<<"Current State="<<_state<<" is not an initial state of the command "<<msg->command();
      jrep["content"]=msg->content();
      jrep["content"]["answer"]=s0.str();
      Json::FastWriter fastWriter;
      msg->setValue(fastWriter.write(jrep));
      return "ERROR";
    }
}
Json::Value fsm::transitionsList()
{
  Json::Value jrep;jrep.clear();
  for( std::map<std::string,std::vector<zdaq::fsmTransition> >::iterator it=_transitions.begin();it!=_transitions.end();it++)
    {
      Json::Value jc;
      jc["name"]=it->first;
      jrep.append(jc);
    }
  return jrep;
}
Json::Value fsm::allowList()
{
  Json::Value jrep;jrep.clear();
  for( std::map<std::string,std::vector<zdaq::fsmTransition> >::iterator it=_transitions.begin();it!=_transitions.end();it++)
    {
      bool allowed=false;
      std::vector<zdaq::fsmTransition> &vp=it->second;
      for (std::vector<zdaq::fsmTransition>::iterator ift=vp.begin();ift!=vp.end();ift++)
	if (ift->initialState().compare(_state)==0)
	  {allowed=true;break;}
      if (allowed)
	{
	  Json::Value jc;
	  jc["name"]=it->first;
	  jrep.append(jc);
	}
    }
  return jrep;
}
