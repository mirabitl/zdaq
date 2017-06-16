#ifndef _ZMPUSHER_HH_
#define _ZMPUSHER_HH_
#include <zmq.hpp>
#include "zmBuffer.hh"

namespace zdaq
{
class zmPusher
{
public:
  zmPusher( zmq::context_t* c, uint32_t det,uint32_t dif);
  void connect(std::string dest);
  void bind(std::string dest);

  void publish(uint64_t bx, uint32_t gtc,uint32_t len);
  char* payload();
  inline uint32_t detectorId(){return _detId;}
  inline uint32_t sourceId(){return _sourceId;} 
private:
  zmq::context_t* _context;
  uint32_t _detId,_sourceId;
  zmq::socket_t *_pusher;
  std::string _header;
  zdaq::buffer* _buffer;
};
};
#endif
