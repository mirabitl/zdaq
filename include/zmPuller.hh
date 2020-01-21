#ifndef _ZMPULLER_HH_
#define _ZMPULLER_HH_
#include <zmq.hpp>
#include <vector>
namespace zdaq {
  /**
     \class zmPuller
     \brief The default object to collect data from various zdaq::zmPusher
     \details It can
     - collect data from zmPusher (ZMQ_PULL)
     - forward collected data to other collector

     The poll() method should be run in a separate thread since it continously listen on all connected socket.
     It counts the number of registered data source identified with specific headers

     Eventually it provides a virtual method to process the received message
     \author    Laurent Mirabito
     \version   1.0
     \date      January 2019
     \copyright GNU Public License.
  */
class zmPuller
{
public:
  zmPuller( zmq::context_t* c);
  void addInputStream(std::string fn,bool server=true);
  void addOutputStream(std::string fn);
  void enablePolling();
  void disablePolling(); 
  void poll();
  inline uint32_t registered(){return _nregistered;}
  void virtual processData(std::string idd,zmq::message_t *message){;}

private:
  std::vector<std::string> _connectStream,_bindStream;
  std::vector<zmq::socket_t*> _socks;
  zmq_pollitem_t _items[255];
  //zmq::socket_t* _puller;
  zmq::context_t* _context;
  zmq::socket_t* _publisher;
  bool _running;
  uint32_t _nregistered;
};
};
#endif
