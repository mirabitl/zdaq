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
  /**
     \brief Constructor

     \param c the ZMQ context
   */
  zmPuller( zmq::context_t* c);

  /**
     \brief add an input data stream (zmPusher)

     \param fn Input data stream name, typically 'tcp://*:5555' Where 5555 is the listening port in server mode
     \param server Mode is true when binding connection from data source, non server mode is not deeply tested
   */
  void addInputStream(std::string fn,bool server=true);

  /**
     \brief Forward stream

     \param fn Output data stream, typically 'tcp://monpc:5556', it is used to forward collected data to another builder
   */
  void addOutputStream(std::string fn);

  
  void enablePolling(); ///< To be called before poll()
  void disablePolling(); ///< Stop the polling loop

  /**
     \brief Polling loop

     \details During the polling loop, data from input stream are collected:
      - If the header is an IDentity type , the number of registered data sources is incremented
      - if the header is a DS(Data Source) type, the data are processed with the processData method
      - if an OutputStream exists the data are forward to it
   */
  void poll();

  /**
     \brief Number of registered data  source
   */
  inline uint32_t registered(){return _nregistered;}

  /**
     \brief Virtual data processing, to be implemented in daughter class
   */
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
