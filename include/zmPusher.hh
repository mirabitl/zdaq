#ifndef _ZMPUSHER_HH_
#define _ZMPUSHER_HH_
#include <zmq.hpp>
#include "zmBuffer.hh"

namespace zdaq
{
  /**
     \class zmPusher
     \brief The default object to send data to the event builder
     \details It provides all the mechanism to push data (PUSH/PULL) mechanism to the event builder
        - it allocates a ZMQ_PUSH socket
	- it creates a 512 kBytes zdaq::buffer
    \author    Laurent Mirabito
    \version   1.0
    \date      January 2019
    \copyright GNU Public License.
  */
class zmPusher
{
public:
  /**
     \brief Constructor
     \param c , the ZMQ context
     \param det , the detecor Id of the allocated buffer
     \param det , the data source Id of the allocated buffer
   */
  zmPusher( zmq::context_t* c, uint32_t det,uint32_t dif);

  /**
     \brief connect the socket to its client (Event Builder)
     \param dest, client address (ex  "tcp://lyoxxx:5555")
   */
  void connect(std::string dest);

  /**
     \brief connect the socket to any server
     \param dest, listenning address (ex  "tcp://*:5556")
   */
  void bind(std::string dest);

  /**
     \brief Send the buffer to the socket
     \param bx ,  bunch crossing id
     \param gtc , event or window id
     \param len , actual len of the buffer
   */
  void publish(uint64_t bx, uint32_t gtc,uint32_t len);

  /**
     \brief Send an ID message on the socket in order to register the data source in the collector process
   */
  void collectorRegister();

  /**
     \brief access to the buffer payload
     \return Pointer to the payload
   */
  char* payload();

  /**
     \brief Switch compression (zlib)
     \param t , True of False
   */
  inline void setCompress(bool t){_compress=t;}

  /**
     \brief Compressions status
     \return True if compression active
   */
  inline bool isCompress() {return _compress;}

  /**
     \brief Access to zdaq::buffer
     \return pointer to the zdaq::buffer
   */
  inline zdaq::buffer* buffer(){return _buffer;}

  /**
     \brief Detector ID
   */
  inline uint32_t detectorId(){return _detId;}

    /**
     \brief Data source ID
   */
  inline uint32_t sourceId(){return _sourceId;} 
private:
  zmq::context_t* _context;
  uint32_t _detId,_sourceId;
  zmq::socket_t *_pusher;
  std::string _header;
  zdaq::buffer* _buffer;
  bool _compress;
};
};
#endif
