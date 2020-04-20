#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sstream>
#include <iostream>
#include "zmSender.hh"
#include "zdaqLogger.hh"

#include <zhelpers.hpp>

#include <unistd.h>
using namespace zdaq;
zmSender::zmSender( zmq::context_t* c, uint32_t det,uint32_t dif) : _detId(det),_sourceId(dif), _context(c),_compress(true)
{
  std::stringstream sheader;
  sheader<<"DS-"<<det<<"-"<<dif;
  _header=sheader.str();
  _buffer=new zdaq::buffer(512*1024);


  _buffer->setDetectorId(det);
  _buffer->setDataSourceId(dif);
}
void zmSender::connect(std::string dest)
{
  zmq::socket_t* sender= new zmq::socket_t((*_context), ZMQ_PUSH);
  sender->setsockopt(ZMQ_SNDHWM,128);
  sender->connect(dest);
  _vSender.push_back(sender);
}

char* zmSender::payload(){return _buffer->payload();}
void zmSender::publish(uint64_t bx, uint32_t gtc,uint32_t len)
{
  std::stringstream ss;
  //printf("SENDER detid %x \n",_buffer->detectorId());
  ss<<_header<<" "<<gtc<<" "<<bx;
  try {
    //s_sendmore((*_sender),ss.str());
    //std::cout<<" bx"<<(uint64_t) bx<<" "<<ss.str()<<"\n";
    zmq::message_t message1(ss.str().size());
    memcpy (message1.data(), ss.str().data(), ss.str().size());

    bool rc = _vSender[gtc%_vSender.size()]->send (message1, ZMQ_SNDMORE);


      
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
    _vSender[gtc%_vSender.size()]->send(message);
  }
  catch (zmq::error_t e)
    {
      LOG4CXX_ERROR(_logZdaq,e.num()<<" error number");
      return;
    }
}
void zmSender::collectorRegister()
{
  std::stringstream ss;
  //printf("SENDER detid %x \n",_buffer->detectorId());
  ss<<"ID-"<<_detId<<"-"<<_sourceId;
  for (auto snd=_vSender.begin();snd!=_vSender.end();snd++)
    {
      try {
	//s_sendmore((*snd),ss.str());
	//std::cout<<" bx"<<(uint64_t) bx<<" "<<ss.str()<<"\n";
	zmq::message_t message1(ss.str().size());
	memcpy (message1.data(), ss.str().data(), ss.str().size());

	bool rc = (*snd)->send (message1, ZMQ_SNDMORE);


      
	_buffer->setBxId(0);
	_buffer->setEventId(0);
	_buffer->setPayloadSize(64);

  
	zmq::message_t message(_buffer->size());
	memcpy(message.data(),_buffer->ptr(),_buffer->size());
	(*snd)->send(message);
      }
      catch (zmq::error_t e)
	{
	  LOG4CXX_ERROR(_logZdaq,e.num()<<" error number");

	  return;
	}
    }
}
