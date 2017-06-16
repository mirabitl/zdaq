//
//  Weather update server in C++
//  Binds PUB socket to tcp://*:5556
//  Publishes random weather updates
//
//  Olivier Chamoux <olivier.chamoux@fr.thalesgroup.com>
//
#include <zmq.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sstream>
#include <iostream>
#include "zmPusher.hh"
#include "zhelpers.hpp"
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>

boost::interprocess::interprocess_mutex _bsem;

class dsReader
{
public:
  dsReader(zmq::context_t* c,uint32_t det,uint32_t sou,std::string dest) : _destination(dest),_detId(det),_sourceId(sou),_context(c)
  {  
     _pusher=new zdaq::zmPusher(_context,_detId,_sourceId);
     std::stringstream ss;
     ss<<"ipc:///tmp/ds-"<<_detId<<"-"<<_sourceId;
     _pusher->connect(dest);
  }
  void readout()
  {
    
    uint32_t evt=0;
    uint64_t bx;
    while (1)
      {
	//std::cout<<_detId<<" "<<_sourceId<<" Before publishing "<<evt<<"\n";

	uint32_t len=within(65500);
	bx=time(0)*1000;
	uint32_t *ida=(uint32_t*) _pusher->payload();
	ida[0]=len;
	//std::cout<<_detId<<" "<<_sourceId<<" Before publishing "<<evt<<"\n";

	//_bsem.lock();
	_pusher->publish(bx,evt,len);
	//_bsem.unlock();
	std::cout<<_detId<<" "<<_sourceId<<" publishing "<<evt<<"\n";
	evt++;
	::usleep(10000);
      }
    std::cout<<_running<<" exiting\n";
  }
  void start(){    _running=true;  }
  void stop()  {_running=false;}
private:
   zmq::context_t* _context;
  bool _running;
  uint32_t _detId,_sourceId;
  zdaq::zmPusher* _pusher;
  std::string _destination;
};

void startdsReaderThread(dsReader* d)
{
   boost::thread_group g_d;
   g_d.create_thread(boost::bind(&dsReader::readout,d));
}

int main () {

  zmq::context_t context (1);

   srandom ((unsigned) time (NULL));
   std::vector<dsReader*> vpub;
   for (int idif=12;idif<15;idif++)
     {
       dsReader* zp=new dsReader(&context,110,idif,"ipc:///tmp/DSIN.ipc");
       vpub.push_back(zp);
     }
   for (auto x:vpub)
     {
       x->start();
       startdsReaderThread(x);
     }
   while (1) ::sleep(1);
  #ifdef WITHPUSHER
    //  Prepare our context and publisher
    zmq::context_t context (1);
    //zmq::socket_t publisher (context, ZMQ_PUB);
    //publisher.bind("tcp://*:5556");
    //publisher.bind("ipc:///tmp/weather1.ipc");				// Not usable on Windows.

    //  Initialize random number generator
    srandom ((unsigned) time (NULL));
    std::vector<zdaq::zmPusher*> vpub;
    for (int idif=12;idif<15;idif++)
      {
	zdaq::zmPusher* zp=new zdaq::zmPusher(&context,110,idif);
	zp->connect("ipc:///tmp/DSIN.ipc");
	vpub.push_back(zp);
      }
    uint32_t evt=0;
    uint64_t bx;
    char cda[512*1024];
   
    while (1)
      {

	for (auto x:vpub)
	  {
	    uint32_t len=within(65500);
	    bx=time(0)*within(1000);
	    uint32_t *ida=(uint32_t*)x->payload();
	    ida[0]=len;
	    x->publish(bx,evt,len);
	    std::cout<<"publishing "<<evt<<"\n";
	  }
	::usleep(1000);
	evt++;
        // int zipcode, temperature, relhumidity;
	
        // //  Get values that will fool the boss
	// int idet=110;
	// for (int idif=12;idif<25;idif++)
	//   {
	//     std::stringstream sheader;
	//     sheader<<"DS-"<<idet<<"-"<<idif;
	//     s_sendmore(publisher,sheader.str());
	//     zipcode     = within (100000);
	//     temperature = within (215) - 80;
	//     relhumidity = within (50) + 10;
	    
	//     //  Send message to all subscribers
	//     zmq::message_t message(within(65500));
	//     snprintf ((char *) message.data(), 20 ,
	// 	      "%05d %d %d", zipcode, temperature, relhumidity);
	//     //printf("Message is %s \n",(char *) message.data());
	//     publisher.send(message);
	//  }
      }
	//::usleep(100000);
#endif

    return 0;
}
