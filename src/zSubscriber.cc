//
//  Weather update client in C++
//  Connects SUB socket to tcp://localhost:5556
//  Collects weather updates and finds avg temp in zipcode
//
//  Olivier Chamoux <olivier.chamoux@fr.thalesgroup.com>
//
#include "zSubscriber.hh"
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h> 
#include <string>
#include "zhelpers.hpp"
#include <boost/algorithm/string.hpp>
using namespace zdaq;


zdaq::publishedItem::publishedItem(std::string add, zmq::context_t &c) : _address(add),_location(""),_hardware(""),_time(0),_socket(0)
{

  _socket =new zmq::socket_t(c, ZMQ_SUB);
  assert(_socket);
  _socket->connect(_address.c_str());
  _socket->setsockopt( ZMQ_SUBSCRIBE, "", 0);

  _item.socket = (*_socket);
   _item.events = ZMQ_POLLIN;
   _item.fd=0;
   _item.revents=0;
  
}
zdaq::publishedItem::~publishedItem()
{
  if (_socket!=NULL) delete _socket;
}
void zdaq::publishedItem::processData(std::string address,std::string contents)
{
  std::vector<std::string> strs;
  strs.clear();
  boost::split(strs,address, boost::is_any_of("@"));
  _hardware=strs[0];
  _location=strs[1];
  sscanf(strs[2].c_str(),"%lld",(long long *) &_time);
  _status.clear();
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(contents,_status);
  
}

zdaq::zSubscriber::zSubscriber(zmq::context_t* context) :  _running(false),_context(context)
{
  //_fsm=this->fsm();
  this->unlock();
}
void zdaq::zSubscriber::clear()
{
  if (_context==0) return;
  for (auto x:_items)
    delete x;
  _items.clear();
  _handlers.clear();
  
}
void zdaq::zSubscriber::addStream(std::string str)
{
 
    LOG4CXX_INFO(_logZdaq," Registering stream: "<<str);
	  zdaq::publishedItem* item=new  zdaq::publishedItem(str,(*_context));
	  _items.push_back(item);
	
  
}

void zdaq::zSubscriber::start()
{
   LOG4CXX_INFO(_logZdaq," Starting");
  _running=true;
  g_d.create_thread(boost::bind(&zdaq::zSubscriber::poll,this));
}

void zdaq::zSubscriber::stop()
{
   LOG4CXX_INFO(_logZdaq," Stopping");

  _running=false;
  g_d.join_all();
}

void zdaq::zSubscriber::poll()
{
  char *zErrMsg = 0;
  int rc;
  //Initialise pollitems
  _nItems= _items.size();
 
  for (int i=0;i<_nItems;i++)
    {
      //memset(&_pollitems[i],0,sizeof(zmq::pollitem_t));
      _pollitems[i]=_items[i]->item();
      /*
      _pollitems[i].socket=_items[i]->socket();
      _pollitems[i].fd=0;
      _pollitems[i].events=ZMQ_POLLIN;
      _pollitems[i].revents=0;
      */
    }
  // Loop
  std::vector<std::string> strs;
  LOG4CXX_INFO(_logZdaq," Polling started: "<<_nItems);
    while (_running)
    {
      LOG4CXX_DEBUG(_logZdaq," Polling loop: "<<_nItems);

      rc=zmq::poll (&_pollitems [0], _nItems, 3000);

      LOG4CXX_DEBUG(_logZdaq," Polling results: "<<rc);
      if (rc==0) continue;
      for (uint16_t i=0;i<_nItems;i++)
        if (_pollitems[i].revents & ZMQ_POLLIN) {


	        std::string address = s_recv (_items[i]->socket());

	  // split address hardware@location@time
	        strs.clear();
	        boost::split(strs,address, boost::is_any_of("@"));
	  //  Read message contents
	        zmq::message_t message;
	        _items[i]->socket().recv(&message);
	  //std::cout<<"Message size is "<<message.size()<<std::endl;
	  //char buffer[65536];


	        std::string contents ;
	        contents.clear();
	        contents.assign((char*) message.data(),message.size());

	        _items[i]->processData(address,contents);
	  
	  

	        }
            //call handlers
      for (auto x=_handlers.begin();x!=_handlers.end();x++)
            (*x)(_items);
    }
    
}
