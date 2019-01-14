#ifndef __zsubscriber_HH__
#define __zsubscriber_HH__
#include <zmq.hpp>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <dirent.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <json/json.h>
#include <time.h>
#include <sstream>
#include <map>
#include <vector>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/bind.hpp>
#include "zdaqLogger.hh"
#include <stdlib.h>

namespace zdaq
{
  class publishedItem {
  public:
    publishedItem(std::string address, zmq::context_t &c);
    ~publishedItem();
    zmq_pollitem_t& item(){return _item;}
    virtual void processData(std::string address,std::string contents);
    std::string location(){return _location;}
    std::string hardware(){return _hardware;}
    time_t time(){return _time;}
    Json::Value status(){return _status;}
    zmq::socket_t& socket(){return (*_socket);}
    std::string address(){return _address;}
  private:
    zmq::socket_t *_socket;
    std::string _address;
    zmq_pollitem_t _item;
    std::string _location,_hardware;
    time_t _time;
    Json::Value _status;
    
  };
typedef boost::function<void (std::vector<publishedItem*>&)> PubFunctor;
    class zSubscriber 
    {
    public:
      zSubscriber(zmq::context_t *c);
      void clear();
      void addStream(std::string str);
      void poll();
      void start();
      void stop();
      std::vector<publishedItem*>& items(){return _items;}
      void lock(){_sync.lock();}
      void unlock(){_sync.unlock();}
      void addHandler(PubFunctor f){_handlers.push_back(f);}
    private:
      std::vector<publishedItem*> _items;
      zmq::pollitem_t _pollitems[1024];
      uint32_t _nItems;
      boost::thread_group g_d;
      zmq::context_t* _context;
      bool _running;
      boost::interprocess::interprocess_mutex _sync;
      std::vector<PubFunctor> _handlers;
    };
  



  };


#endif

  