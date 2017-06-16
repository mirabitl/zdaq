#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sstream>
#include <iostream>
#include "zmPublisher.hh"

#include <zhelpers.hpp>

#include <unistd.h>
using namespace zdaq;
zmPublisher::zmPublisher( zmq::context_t* c, uint32_t det,uint32_t dif) : _detId(det),_sourceId(dif), _context(c)
  {
    std::stringstream sheader;
    sheader<<"DS-"<<det<<"-"<<dif;
    _header=sheader.str();
    _publisher= new zmq::socket_t((*_context), ZMQ_PUSH);
    _buffer=new zdaq::buffer(512*1024);
    sheader.str(std::string());
    sheader<<"ipc://"<<"/dev/shm/DS-"<<det<<"-"<<dif<<".ipc";
    _publisher->bind(sheader.str());

    _buffer->setDetectorId(det);
    _buffer->setDataSourceId(dif);
  }
char* zmPublisher::payload(){return _buffer->payload();}
void zmPublisher::publish(uint64_t bx, uint32_t gtc,uint32_t len)
  {
    std::stringstream ss;
    ss<<_header<<" "<<gtc<<" "<<bx;
    s_sendmore((*_publisher),ss.str());
    _buffer->setBxId(bx);
    _buffer->setEventId(gtc);
    _buffer->setPayloadSize(len);

    
    zmq::message_t message(_buffer->size());
    memcpy(message.data(),_buffer->ptr(),_buffer->size());
    _publisher->send(message);
  }
