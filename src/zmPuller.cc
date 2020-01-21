#include <iostream>
#include <sstream>
#include <stdio.h>
#include <dirent.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include "zhelpers.hpp"
#include "zmPuller.hh"
#include "zdaqLogger.hh"
#include <regex>
#define ONETHREAD
using namespace zdaq;
zmPuller::zmPuller( zmq::context_t* c) : _context(c),_publisher(NULL)
{
  _connectStream.clear();
  _bindStream.clear();
  _socks.clear();
  memset(_items,0,255*sizeof(zmq_pollitem_t));
}

void  zmPuller::addInputStream(std::string fn,bool server)
{
  if (server)
    _bindStream.push_back(fn);
  else
    _connectStream.push_back(fn);
#ifdef ONETHREAD
  zmq::socket_t *subscriber=new zmq::socket_t((*_context),ZMQ_PULL);
  if (server)
    subscriber->bind(fn);
  else
    subscriber->connect(fn);
  int idx=_socks.size();
  _items[idx].socket =(*subscriber);
  _items[idx].events=ZMQ_POLLIN;
  _socks.push_back(subscriber);
#endif
  // _puller=new zmq::socket_t((*_context),ZMQ_PULL);
  // std::cout<<" binding "<<fn<<"\n";
  // try {
  //   _puller->bind(fn);
  // } catch (zmq::error_t e)
  //     {
  //       std::cout<<e.num()<<std::endl;
  //       return;
  //     }
  // std::cout<<"binding complete \n";
}
void  zmPuller::addOutputStream(std::string fn)
{
  if (_publisher==NULL)
    {
      _publisher=new zmq::socket_t((*_context),ZMQ_PUSH);
    }
  _publisher->connect(fn);
    
}

void  zmPuller::enablePolling() {_running=true;}
void  zmPuller::disablePolling() {_running=false;}
void  zmPuller::poll()
{
#ifndef ONETHREAD
  
  for (auto x:_connectStream)
    {
      zmq::socket_t *subscriber=new zmq::socket_t((*_context),ZMQ_PULL);
      subscriber->connect(x);
      int idx=_socks.size();
      _items[idx].socket =(*subscriber);
      _items[idx].events=ZMQ_POLLIN;
      _socks.push_back(subscriber);
    }
  for (auto x:_bindStream)
    {
      zmq::socket_t *subscriber=new zmq::socket_t((*_context),ZMQ_PULL);
      subscriber->bind(x);
      int idx=_socks.size();
      _items[idx].socket =(*subscriber);
      _items[idx].events=ZMQ_POLLIN;
      _socks.push_back(subscriber);
    }

#endif
  LOG4CXX_INFO(_logZdaq,"start polling on "<<_socks.size()<<" sockets");
  zmq::message_t message;
  _nregistered=0;
  while (_running)
    {

      int rc = zmq::poll(_items, _socks.size(), 2000);
      if (rc<0) continue;
      for (int i=0;i<_socks.size();i++)
	{
	  if (_items [i].revents & ZMQ_POLLIN)
	    {
	      try {
		std::string identity = s_recv((*_socks[i]));
		uint32_t detid,sid,gtc;
		uint64_t bx;
		bool registering=(identity.compare(0,2,"ID") == 0);
		if (registering)
		  {
		    sscanf(identity.c_str(),"ID-%d-%d %d %ld",&detid,&sid,&gtc,&bx);
		    LOG4CXX_INFO(_logZdaq," New Source registered:"<<detid<<"-"<<sid);
		    _nregistered++;
		  }
		else 
		  {
		    sscanf(identity.c_str(),"DS-%d-%d %d %ld",&detid,&sid,&gtc,&bx);
		  }
		//std::cout<<identity<<std::endl;
		_socks[i]->recv(&message);
		if (gtc%100==0)
		  {
		    printf("Socket ID %s  size %d : %d %d %d %ld\n",identity.c_str(),message.size(),detid,sid,gtc,bx);
		  }
		//printf("Socket %d ID %s  size %d \n",i,identity.c_str(),message.size());
		if (_publisher)
		  {
		    s_sendmore((*_publisher),identity);
		    _publisher->send(message);
		    if (gtc%100==0)
		      std::cout<<"Forwarding\n";
		    //std::cout<<"Forwarding\n";
		  }
		if (!registering)
		  this->processData(identity,&message);
	      }
	      catch (zmq::error_t e)
		{
		  LOG4CXX_ERROR(_logZdaq,"Poll error nb:"<<e.num());
		  continue;
		}
	    }
	}


      
     
     
      ::usleep(1);
    }
}

