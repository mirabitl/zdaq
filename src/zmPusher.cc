#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sstream>
#include <iostream>
#include "zmPusher.hh"
#include "zdaqLogger.hh"

#include <zhelpers.hpp>

#include <unistd.h>
using namespace zdaq;
zmPusher::zmPusher( zmq::context_t* c, uint32_t det,uint32_t dif) : _detId(det),_sourceId(dif), _context(c),_compress(true)
{
  std::stringstream sheader;
  sheader<<"DS-"<<det<<"-"<<dif;
  _header=sheader.str();
  _pusher= new zmq::socket_t((*_context), ZMQ_PUSH);
  _pusher->setsockopt(ZMQ_SNDHWM,128);
  _buffer=new zdaq::buffer(512*1024);


  _buffer->setDetectorId(det);
  _buffer->setDataSourceId(dif);
}
void zmPusher::connect(std::string dest)
{
  _pusher->connect(dest);
}
void zmPusher::bind(std::string dest)
{
  _pusher->bind(dest);
}

char* zmPusher::payload(){return _buffer->payload();}
void zmPusher::publish(uint64_t bx, uint32_t gtc,uint32_t len)
{
  std::stringstream ss;
  //printf("PUSHER detid %x \n",_buffer->detectorId());
  ss<<_header<<" "<<gtc<<" "<<bx;
  try {
    //s_sendmore((*_pusher),ss.str());
    //std::cout<<" bx"<<(uint64_t) bx<<" "<<ss.str()<<"\n";
    zmq::message_t message1(ss.str().size());
    memcpy (message1.data(), ss.str().data(), ss.str().size());

    bool rc = _pusher->send (message1, ZMQ_SNDMORE);


      
    _buffer->setBxId(bx);
    _buffer->setEventId(gtc);
    _buffer->setPayloadSize(len);

    if (_compress)
      {
	uint32_t bb=_buffer->size();

	_buffer->compress();
	LOG4CXX_DEBUG(_logZdaq," Compressing data"<<_buffer->size());
      }
    zmq::message_t message(_buffer->size());
    memcpy(message.data(),_buffer->ptr(),_buffer->size());
    _pusher->send(message);
  }
  catch (zmq::error_t e)
    {
      LOG4CXX_ERROR(_logZdaq,e.num()<<" error number");
      return;
    }
}
void zmPusher::collectorRegister()
{
  std::stringstream ss;
  //printf("PUSHER detid %x \n",_buffer->detectorId());
  ss<<"ID-"<<_detId<<"-"<<_sourceId;
  try {
    //s_sendmore((*_pusher),ss.str());
    //std::cout<<" bx"<<(uint64_t) bx<<" "<<ss.str()<<"\n";
    zmq::message_t message1(ss.str().size());
    memcpy (message1.data(), ss.str().data(), ss.str().size());

    bool rc = _pusher->send (message1, ZMQ_SNDMORE);


      
    _buffer->setBxId(0);
    _buffer->setEventId(0);
    _buffer->setPayloadSize(64);

  
    zmq::message_t message(_buffer->size());
    memcpy(message.data(),_buffer->ptr(),_buffer->size());
    _pusher->send(message);
  }
  catch (zmq::error_t e)
    {
      LOG4CXX_ERROR(_logZdaq,e.num()<<" error number");

      return;
    }
}
