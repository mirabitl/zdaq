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
//#include <sqlite3.h> 
#include <string>
#include "zhelpers.hpp"
#include <boost/algorithm/string.hpp>
using namespace zdaq;
using namespace zdaq::mon;


zdaq::mon::publishedItem::publishedItem(std::string add, zmq::context_t &c) : _address(add),_location(""),_hardware(""),_time(0),_socket(0)
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
zdaq::mon::publishedItem::~publishedItem()
{
  if (_socket!=NULL) delete _socket;
}
void zdaq::mon::publishedItem::processData(std::string address,std::string contents)
{
  //LOG4CXX_INFO(_logZdaq,"Entering process data: "<<contents);
  std::vector<std::string> strs;
  strs.clear();
  boost::split(strs,address, boost::is_any_of("@"));
  _hardware=strs[0];
  _location=strs[1];
  
  sscanf(strs[2].c_str(),"%lld",(long long *) &_time);
  //LOG4CXX_INFO(_logZdaq,"HW: "<<_hardware<< " location: "<<_location<<" time"<<_time);
  //_status.clear();
  //LOG4CXX_INFO(_logZdaq,"2HW: "<<_hardware<< " location: "<<_location<<" time"<<_time);
  Json::Reader reader;
  // LOG4CXX_INFO(_logZdaq,"3HW: "<<_hardware<< " location: "<<_location<<" time"<<_time);
  Json::Value v;
  //LOG4CXX_INFO(_logZdaq,"4HW: "<<_hardware<< " location: "<<_location<<" time"<<_time);
  bool parsingSuccessful;
  try {
    parsingSuccessful = reader.parse(contents,v);
    //LOG4CXX_INFO(_logZdaq,"5HW: "<<_hardware<< " location: "<<_location<<" time"<<_time);
  }
  catch(...)
    {
      LOG4CXX_ERROR(_logZdaq,"parsing failed");
    }
  if (parsingSuccessful)
    _status=v;
  //  LOG4CXX_INFO(_logZdaq,"End: "<<_status);

}

zdaq::mon::zSubscriber::zSubscriber(zmq::context_t* context) :  _running(false),_context(context)
{
  //_fsm=this->fsm();
  this->unlock();
}
void zdaq::mon::zSubscriber::clear()
{
  if (_context==0) return;
  for (auto x:_items)
    delete x;
  _items.clear();
  _handlers.clear();
  
}
void zdaq::mon::zSubscriber::addStream(std::string str)
{
 
    LOG4CXX_INFO(_logZdaq," Registering stream: "<<str);
    zdaq::mon::publishedItem* item=new  zdaq::mon::publishedItem(str,(*_context));
    _items.push_back(item);
	
  
}

void zdaq::mon::zSubscriber::start()
{
   LOG4CXX_INFO(_logZdaq," Starting");
  _running=true;
  g_d.create_thread(boost::bind(&zdaq::mon::zSubscriber::poll,this));
}

void zdaq::mon::zSubscriber::stop()
{
   LOG4CXX_INFO(_logZdaq," Stopping");

  _running=false;
  g_d.join_all();
}

void zdaq::mon::zSubscriber::poll()
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
  std::string address,contents;
  zmq::message_t* m= new zmq::message_t(512*1024);
  LOG4CXX_INFO(_logZdaq," Polling started: "<<_nItems);
    while (_running)
    {
      //LOG4CXX_DEBUG(_logZdaq," Polling loop: "<<_nItems);

      rc=zmq::poll (&_pollitems [0], _nItems, 3000);

      //LOG4CXX_DEBUG(_logZdaq," Polling results: "<<rc);
      if (rc==0) continue;
      for (uint16_t i=0;i<_nItems;i++)
	{
	  //  LOG4CXX_DEBUG(_logZdaq," Polling results: ["<<i<<"]"<<_pollitems[i].revents);
        if (_pollitems[i].revents & ZMQ_POLLIN) {

	  address.clear();
	  //address = s_recv (_items[i]->socket());
	  _items[i]->socket().recv(m);
	  /*
	  std::cout<<"Message "<<(char*) m->data()<<" size is "<<m->size()<<std::endl;
	  for (int i=0;i<m->size();i++)
	    fprintf(stderr,"%.2x ",((uint8_t*)m->data())[i]);
	  fprintf(stderr,"\n==>\n");
	  for (int i=0;i<m->size();i++)
	    fprintf(stderr,"%c ",((uint8_t*)m->data())[i]);
	  fprintf(stderr,"\n==>\n");
	  */
	   address.assign((char*) m->data(),m->size());
	  // split address hardware@location@time
	   strs.clear();
	   boost::split(strs,address, boost::is_any_of("@"));
	  //  Read message contents
	   _items[i]->socket().recv(m);
	  //std::cout<<"Message size is "<<message.size()<<std::endl;
	  //char buffer[65536];


	        std::string contents ;
	        contents.clear();
	        contents.assign((char*) m->data(),m->size());

		//LOG4CXX_INFO(_logZdaq,"Message size is "<<m->size()<<" ADR: "<<address<<" CONT: "<<contents);
	        _items[i]->processData(address,contents);
		//LOG4CXX_INFO(_logZdaq,"Message is processed");


	        }
	}
            //call handlers
      //LOG4CXX_INFO(_logZdaq,"Message is sent to handlers");
      for (auto x=_handlers.begin();x!=_handlers.end();x++)
            (*x)(_items);
      //LOG4CXX_INFO(_logZdaq,"Message is processed by handlers");
    }
    
}

