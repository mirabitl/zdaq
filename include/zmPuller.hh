#ifndef _ZMPULLER_HH_
#define _ZMPULLER_HH_
#include <zmq.hpp>
#include <vector>
namespace zdaq {
class zmPuller
{
public:
  zmPuller( zmq::context_t* c);
  void addInputStream(std::string fn);
  void addOutputStream(std::string fn);
  void start();
  void stop(); 
  void poll();

private:
  zmq::socket_t* _puller;
  zmq::context_t* _context;
  zmq::socket_t* _publisher;
  bool _running;
};
};
#endif
