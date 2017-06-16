#include <iostream>
#include <sstream>
#include <stdio.h>
#include <dirent.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include "zhelpers.hpp"
#include "zmPuller.hh"
#include <regex>

using namespace zdaq;
zmPuller::zmPuller( zmq::context_t* c) : _context(c),_puller(NULL),_publisher(NULL)
{
}

void  zmPuller::addInputStream(std::string fn)
{
 _puller=new zmq::socket_t((*_context),ZMQ_PULL);
 std::cout<<" binding "<<fn<<"\n";
 try {
   _puller->bind(fn);
 } catch (zmq::error_t e)
     {
       std::cout<<e.num()<<std::endl;
       return;
     }
 std::cout<<"binding complete \n";
}
void  zmPuller::addOutputStream(std::string fn)
{
  if (_publisher==NULL)
    {
      _publisher=new zmq::socket_t((*_context),ZMQ_PUSH);
    }
  _publisher->connect(fn);
    
}

void  zmPuller::start() {_running=true;}
void  zmPuller::stop() {_running=false;}
void  zmPuller::poll()
{
  zmq::message_t message;
  while (_running)
    {
     try
       {
	 std::string identity = s_recv((*_puller));
	 uint32_t detid,sid,gtc;
	 uint64_t bx;
	 sscanf(identity.c_str(),"DS-%d-%d %d %ld",&detid,&sid,&gtc,&bx);
      _puller->recv(&message);
	 if (gtc%100==0)
	   {
	     printf("Socket ID %s  size %d : %d %d %d %ld\n",identity.c_str(),message.size(),detid,sid,gtc,bx);}
      if (_publisher!=NULL)
	{
	  s_sendmore((*_publisher),identity);
	  _publisher->send(message);
	  if (gtc%100==0)
	    std::cout<<"Forwarding\n";
	}
       }
      catch (zmq::error_t e)
     {
       std::cout<<e.num()<<std::endl;
       continue;
     }
      ::usleep(1);
    }
}

