//
//  Weather update client in C++
//  Connects SUB socket to tcp://localhost:5556
//  Collects weather updates and finds avg temp in zipcode
//
//  Olivier Chamoux <olivier.chamoux@fr.thalesgroup.com>
//
#include <zmq.hpp>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <dirent.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include "zhelpers.hpp"
#include <regex>
#include "zmBuffer.hh"

int main (int argc, char *argv[])
{
    zmq::context_t context (2);
   
    //  Socket to talk to server
    std::cout << "Collecting updates from weather server...\n" << std::endl;
    zmq::socket_t subscriber (context, ZMQ_PULL);
    subscriber.connect("tcp://lyopc252:5556");

    //  Subscribe to zipcode, default is NYC, 10001
    printf("Message collection \n");

    //  Process 100 updates
    int update_nbr;
    long total_temp = 0;
    //for (update_nbr = 0; update_nbr < 10000; update_nbr++) {
    while(1)
      {

        zmq::message_t update;
        int zipcode, temperature, relhumidity;

        //subscriber.recv(&update);
	std::string identity = s_recv(subscriber);
	
	//printf("%s \n",identity.c_str());
	subscriber.recv(&update);
	uint32_t detid,sid,gtc;
	uint64_t bx;
	sscanf(identity.c_str(),"DS-%d-%d %d %ld",&detid,&sid,&gtc,&bx);
      
	if (gtc%100==0)
	   {
	     printf("Socket ID %s  size %d : %d %d %d %ld\n",identity.c_str(),update.size(),detid,sid,gtc,bx);}

      }

    return 0;
}
