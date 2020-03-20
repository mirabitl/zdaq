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
  namespace mon
  {
     /**
     \class publishedItem
     \brief A structure class to handle the zdaq::mon::zPublisher messages
     \details It provides 
        - a data processing method to extract status,hardware,location and time infos.
	- the getters to those informations
    \author    Laurent Mirabito
    \version   1.0
    \date      January 2019
    \copyright GNU Public License.
  */
    class publishedItem {
    public:
      /**
       * \brief Constructor
       * \param address is the connection address , i.e tcp://monpc:5556
       * \param c is the ZMQ context
       * \details it connects to the specified socket (ZMQ_SUBSCRIBE)
       */
      publishedItem(std::string address, zmq::context_t &c);
      /**
       * \brief destructor
       * \details it disconnects the socket
       */
      ~publishedItem();
      /**
       * \brief access to the zmq_pollitem_t
       */
      zmq_pollitem_t& item(){return _item;}

      /**
       * \brief Decodes the pollitem content
       * \param address is the header of the monitoring item "hardware@location@time"
       * \param contents is the JSON string of the status
       */
      virtual void processData(std::string address,std::string contents);
      /**
       * \brief Getter to the location
       */
      std::string location(){return _location;}
       /**
       * \brief Getter to the hardware name
       */
      std::string hardware(){return _hardware;}
       /**
       * \brief Getter to the time stamp
       */
      time_t time(){return _time;}
       /**
       * \brief Getter to the status
       */
      Json::Value status(){return _status;}
       /**
       * \brief Getter to the zmq socket
       */
      zmq::socket_t& socket(){return (*_socket);}
       /**
       * \brief Getter to the connected address
       */
      std::string address(){return _address;}
    private:
      zmq::socket_t *_socket;
      std::string _address;
      zmq_pollitem_t _item;
      std::string _location,_hardware;
      time_t _time;
      Json::Value _status;
    
    };
    /**
     * \brief Boos functor to handle publishedItem processing
     */
    typedef boost::function<void (std::vector<zdaq::mon::publishedItem*>&) > PubFunctor;
    /**
    * \class zSubscriber
    * \brief The "event Builder" class of the monitoring system
    * \details A zdaq::zSubscriber object can subscribe to various zdaq::zPublisher sockets. 
    * Each subscription is corresponding to a zdaq::publishedItem object. The collected items are then analysed by 
    * registered handlers (PubFunctor)
    * \author    Laurent Mirabito
    * \version   1.0
    * \date      January 2019
    * \copyright GNU Public License.
    */  
    class zSubscriber 
    {
    public:
    /**
     * \brief Constructor
     * \param c is the zmq::context
    */
      zSubscriber(zmq::context_t *c);
    /**
     * \brief Destroy all registered items and clears handlers list
     */ 
      void clear();
      /**
       * \brief Add a zdaq::mon::publishedItem to the list of subscribed services
       * \param str is the address of the service , i.e tcp://monpc:5556
       */
      void addStream(std::string str);
      /**
       * \brief Polling loop on all subscribed services and processing thru the handlers
       */
      void poll();
      /**
       * \brief Start the polling loop
       */
      void start();
      /**
       * \brief Stop the polling loop
       */
      void stop();
      /**
       * \brief access to the registered zdaq::mon::publishedItem vector
       */
      std::vector<zdaq::mon::publishedItem*>& items(){return _items;}
      /**
       * \brief Lock semaphore
       */
      void lock(){_sync.lock();}
      /**
       * \brief Unlock semaphore
       */
      void unlock(){_sync.unlock();}
      /**
       * \brief Add processing handler. All register handlers are called each time  an item is updated
       * \param f is a PubFunctor that process a vector of publishedItem
       */
      void addHandler(PubFunctor f){_handlers.push_back(f);}
    private:
      std::vector<zdaq::mon::publishedItem*> _items;
      zmq::pollitem_t _pollitems[1024];
      uint32_t _nItems;
      boost::thread_group g_d;
      zmq::context_t* _context;
      bool _running;
      boost::interprocess::interprocess_mutex _sync;
      std::vector<PubFunctor> _handlers;
    };
  



  };
};

#endif

  
