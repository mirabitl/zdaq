#ifndef _ZMROUTER_HH_
#define _ZMROUTER_HH_
#include <zmq.hpp>
#include <vector>
namespace zdaq {
class zmRouter
{
public:
  zmRouter( zmq::context_t* c);
  void list(std::string dir="/dev/shm");
  void addInputStream(std::string fn);
  void addOutputStream(std::string fn);
  void start();
  void stop(); 
  void poll();

private:
  std::vector<zmq::socket_t*> _socks;
  zmq_pollitem_t _items[255];
  zmq::context_t* _context;
  zmq::socket_t* _publisher;
  bool _running;
};
};
#endif
