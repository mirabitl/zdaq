#ifndef _ZMROUTER_HH_
#define _ZMROUTER_HH_
#include <zmq.hpp>
#include <vector>
namespace zdaq {
    /**
     \class zmRouter
     \brief ipc to tcpip socket converter

     \details The zmRouter collects data from ipc stream found in a directory to a tcp socket
     decalred as output stream
     - The list command registered all existing ipc stream 
     - A polling mechanism read the streams and forward the messages
     \author    Laurent Mirabito
     \version   1.0
     \date      January 2019
     \copyright GNU Public License.
  */
class zmRouter
{
public:
  /**
     \brief Constructor
     \apram c the ZMQ context
   */
  zmRouter( zmq::context_t* c);

  /**
     \brief Find all the IPC stream in a given directory and register them as input 

     \param dir Directory to scan
   */
  void list(std::string dir="/dev/shm");

  /**
     \briedf connect to an input stream and add it to the input list
   */

  void addInputStream(std::string fn);
  /**
     \brief add an output stream
   */
  void addOutputStream(std::string fn);
  void start();///< enable polling
  void stop(); ///<disable polling
  void poll();///< Polling loop

private:
  std::vector<zmq::socket_t*> _socks;
  zmq_pollitem_t _items[255];
  zmq::context_t* _context;
  zmq::socket_t* _publisher;
  bool _running;
};
};
#endif
