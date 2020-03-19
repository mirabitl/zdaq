#ifndef _zPublisher_hh
#define _zPublisher_hh
#include <string>
#include <json/json.h>
using namespace std;
#include <sstream>
#include <zmq.hpp>


namespace zdaq {
  namespace mon {
   /**
     \class zPublisher
     \brief A generic application object which provides s Publication mechanism for slow control
     \details It creates a server socket on a given port, registers a hardware name and a setup location. It also provides a post method that will publish a given Json status with hardware,location, and time tag.
       
    \author    Laurent Mirabito
    \version   1.0
    \date      January 2019
    \copyright GNU Public License.
  */ 
    class zPublisher 
    {
    public:
      /**
       * \brief Constructor
       *
       * \param hardware is the name of the hardware monitored (BMP,HIH8000,CAEN1527...)
       * \param location is the name of the setup/experiment
       * \param tcp is the TCP port used to publish
       * \param c is the ZMQ context
       *
       * \details the ZMQ_PUB socket is created and bind
       */
      zPublisher(std::string hardware,std::string location,uint32_t tcp,zmq::context_t* c) : _hardware(hardware),_location(location),_tcpPort(tcp),_context(c)
      {
        _publisher= new zmq::socket_t((*_context), ZMQ_PUB);
	std::stringstream sport;
	sport<<"tcp://*:"<<_tcpPort;
	std::cout<<"Binding to "<<sport.str()<<std::endl;
	_publisher->bind(sport.str());
      }
      /**
       * \brief Post the hardware status
       * \param status is the JSON value of the hardware status (free to the user)
       */
      void post(Json::Value status)
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
	_publisher->send(ma1, ZMQ_SNDMORE);
	// Status
	Json::Value jstatus=status;
	std::string scont= fastWriter.write(jstatus);
	zmq::message_t ma2((void*)scont.c_str(), scont.length(), NULL); 

	std::cout<<"publishing "<<head<<" =>"<<scont<<std::endl;
	_publisher->send(ma2);
	 
      }
      /**
       * \brief Hardware name
       */
      std::string hardware(){return _hardware;}
      /**
       * \brief Setup/experiment name
       */
      std::string location() {return _location;}
    protected:
      std::string _hardware,_location;
      uint32_t _tcpPort;
      zmq::context_t* _context;
      zmq::socket_t *_publisher;
    };
  };
};
#endif
