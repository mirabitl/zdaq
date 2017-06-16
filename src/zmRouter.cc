#include <iostream>
#include <sstream>
#include <stdio.h>
#include <dirent.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include "zhelpers.hpp"
#include "zmRouter.hh"
#include <regex>
using namespace zdaq;
int dsfilter(const struct dirent *dir)
// post: returns 1/true if name of dir ends in .mp3 
{
  std::string pattern="DS-.*+\\.ipc";
  const char *s = dir->d_name;
  std::regex txt_regex(pattern);
  std::string fname( dir->d_name);
  return (std::regex_match(fname, txt_regex));
 
  //return 0;
  
}


zmRouter::zmRouter( zmq::context_t* c) : _context(c),_publisher(NULL)
{
  _socks.clear();
  memset(_items,0,255*sizeof(zmq_pollitem_t));
}
void zmRouter::list(std::string sdir)
{
  struct dirent **eps;
  int n;
    
  n = scandir ((const char*) sdir.c_str(), &eps, dsfilter, alphasort);
  if (n >= 0)
    {
      int cnt;
      for (cnt = 0; cnt < n; ++cnt)
	{
	  std::stringstream ss;
	  ss<<"ipc:///dev/shm/"<<eps[cnt]->d_name;
	  std::cout<<ss.str()<<std::endl;
	  this->addInputStream(ss.str());
	}
    }
  else
    perror ("Couldn't open the directory");

    
}
void  zmRouter::addInputStream(std::string fn)
{
  zmq::socket_t *subscriber=new zmq::socket_t((*_context),ZMQ_SUB);
  subscriber->setsockopt(ZMQ_SUBSCRIBE, "", 0);
  subscriber->connect(fn);
  int idx=_socks.size();
  _items[idx].socket =(*subscriber);
  _items[idx].events=ZMQ_POLLIN;
  _socks.push_back(subscriber);
}
void  zmRouter::addOutputStream(std::string fn)
{
  if (_publisher==NULL)
    {
      _publisher=new zmq::socket_t((*_context),ZMQ_PUB);
    }
  _publisher->bind(fn);
    
}

void  zmRouter::start() {_running=true;}
void  zmRouter::stop() {_running=false;}
void  zmRouter::poll()
{
  zmq::message_t message;
  while (_running)
    {
      int rc = zmq::poll(_items, _socks.size(), -1);
      for (int i=0;i<_socks.size();i++)
	{
	  if (_items [i].revents & ZMQ_POLLIN)
	    {
	      std::string identity = s_recv((*_socks[i]));

	      _socks[i]->recv(&message);
	      printf("Socket %d ID %s  size %d \n",i,identity.c_str(),message.size());
	      if (_publisher)
		{
		  s_sendmore((*_publisher),identity);
		  _publisher->send(message);
		  std::cout<<"Forwarding\n";
		}
	    }
	}
      ::usleep(1);
    }
}

