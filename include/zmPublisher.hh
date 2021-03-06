#ifndef _ZMPUBLISHER_HH_
#define _ZMPUBLISHER_HH_
#include <zmq.hpp>
#include "zmBuffer.hh"

namespace zdaq {
  /**
     \brief First implementation of zmPusher
     
     \b obsolete Use zmPusher instead
   */
class zmPublisher
{
public:
  zmPublisher( zmq::context_t* c, uint32_t det,uint32_t dif);
  void publish(uint64_t bx, uint32_t gtc,uint32_t len);
  char* payload();
private:
  zmq::context_t* _context;
  uint32_t _detId,_sourceId;
  zmq::socket_t *_publisher;
  std::string _header;
  zdaq::buffer* _buffer;
};
};
#endif
