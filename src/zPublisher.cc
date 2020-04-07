
#include "zPublisher.hh"
using namespace std;
#include <iostream>
#include <sstream>
#include <zmq.hpp>

using namespace zdaq;
using namespace zdaq::mon;
zdaq::mon::zPublisher::zPublisher(std::string hardware,std::string location,uint32_t tcp,zmq::context_t* c) : _hardware(hardware),_location(location),_tcpPort(tcp),_context(c)
{
 
  _publisher= new zmq::socket_t((*_context), ZMQ_PUB);
  std::stringstream sport;
  sport<<"tcp://*:"<<_tcpPort;
  std::cout<<"Binding to "<<sport.str()<<std::endl;
  _publisher->bind(sport.str());
}
void zdaq::mon::zPublisher::post(Json::Value status)
{
  Json::FastWriter fastWriter;
  
  
  if (_publisher==NULL)
    {
      std::cout<<"No publisher defined"<<std::endl;
      return;
    }
  // ID
  std::stringstream sheader;
  sheader<<this->hardware()<<"@"<<location()<<"@"<<time(0);
  std::string head=sheader.str();
  
  zmq::message_t ma1((void*)head.c_str(), head.length(), NULL);

  std::cout<<"Message :"<<(char*) ma1.data()<<" size"<<ma1.size()<<std::endl;
  _publisher->send(ma1, ZMQ_SNDMORE);
  // Status
  Json::Value jstatus=status;
  std::string scont= fastWriter.write(jstatus);
  zmq::message_t ma2((void*)scont.c_str(), scont.length(), NULL); 
  std::cout<<"Message :"<<(char*) ma2.data()<<" size"<<ma2.size()<<std::endl;
  std::cout<<"publishing "<<head<<" =>"<<scont<<std::endl;
  _publisher->send(ma2);
	 
}
std::string const zdaq::mon::zPublisher::hardware(){return _hardware;}
std::string const zdaq::mon::zPublisher::location() {return _location;}
