#ifndef _zdaq_zmmerger_h
#define _zdaq_zmmerger_h

#include <stdint.h>
#include <stdlib.h>
#include "zmBuffer.hh"
#include "zmPuller.hh"
#include "zPublisher.hh"
#include <vector>
#include <map>
#include <string>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <json/json.h>

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#define dskey(d,s) ( (s&0xFFF) | ((d &0xFFF)<<12))
#define source_id(k) (k&0xFFF)
#define detector_id(k) ((k>>12)&0xFFF)
namespace zdaq {
    /**
     \class zmprocessor
     \brief Purely virtual interface to process data

     \details A zmprocessor is accesible as a pluggin it must implement 2 methods
     - loadProcessor
     - deleteProcessor

     \b Example

     extern "C" 
     {

     zdaq::zmprocessor* loadProcessor(void)
     {
     return (new myprocessor);
     }

     void deleteProcessor(zdaq::zmprocessor* obj)
     {
     delete obj;
     }

     }

     \author    Laurent Mirabito
     \version   1.0
     \date      January 2019
     \copyright GNU Public License.
  */
  class zmprocessor
  {
  public:
    /**
       \brief Start of run call
       \param run is the run number
     */
    virtual void start(uint32_t run)=0;
    /**
       \brief End of run
     */
    virtual void stop()=0;
    /**
       \brief Completed event processing
       
       \param key is the trigger/window id
       \param dss is a completed vector of zdaq::buffer from collected data sources
     */
    virtual  void processEvent(uint32_t key,std::vector<zdaq::buffer*> dss)=0;
    /**
       \brief  Run header processing
       \param header is an int vetor to be stored
     */
    virtual  void processRunHeader(std::vector<uint32_t> header)=0;
    /**
       \brief Parameter setting interface
       \param params is a Json::value where parameters are stored
     */
    virtual  void loadParameters(Json::Value params)=0;
  };

  
    /**
     \class zmMerger
     \brief The basic Event Builder
     \details It inherits from zmPuller and implements the processData method
     - Based on the trigger id of each buffer, it stores data sources zdaq::buffer in a map until a given trigger id has collected packet from all data sources registered
     - It then processes the buffer vector  by calling the processEvent method
     - In this method, all registered zmprocessor are called thru their processEvent method, at the end the data line is cleared from the map
     - A separate process allow the writing of any run header
     \author    Laurent Mirabito
     \version   1.0
     \date      January 2019
     \copyright GNU Public License.
  */
  class zmMerger : public zmPuller
  {
  public:
    /**
       \brief Constructor

       \param c the ZMQ context
     */
    zmMerger(zmq::context_t* c);

    /**
       \brief destructor
     */
    ~zmMerger();

    /**
       \brief implementation of the zmPuller method

       \param idd identity, ie the header string of the received buffer
       \param message the zmq message received

       \details It finds the source id and GTC from the identity string and fill the event map with the message buffer. It checks if any map slot is completed
       and calls processEvent for full slot. Eventually it cleans the map from processed slot
     */
    void virtual processData(std::string idd,zmq::message_t *message);
    
    void clear();///< Clear the event map
    void scanMemory();///< call the zmPuller polling

    /**
       \brief add a zmprocessor pluggin to the processor list

       \param name shared library name, lib'name'.so should be in the LD_LIBRARY_PATH
     */
    void registerProcessor(std::string name);
    void registerDataSource(std::string url);///<  addInputStream zmPuller call in server mode

    /**
       \brief number of registered data sources
    */
    inline uint32_t numberOfDataSource(){return _nDifs;}


    /**
       \brief Force the number of registered data source
    */
    inline void setNumberOfDataSource(uint32_t k){_nDifs=k;}
    uint32_t numberOfDataPacket(uint32_t k);///< Number of received packet
    void unregisterProcessor(zdaq::zmprocessor* p);///< remove a processor from the list

    /**
       \brief Start of run
       
       \param nr is the run number

       \details It start a separate thread and launch scanMemory in it
     */
    void start(uint32_t nr);

    /**
       \brief Process of an event

       \param id is the trigger ID

       \details it loops on all processors registered and calls their processEvent method
    */
    void processEvent(uint32_t id);

    /**
       \brief Process the run header
       
       \details A private vector _runHeader can be filled by user and is provided to the processRunHeader method of all processors
     */
    void processRunHeader();

    
    void loadParameters(Json::Value params);///< Loops on processors and calls their loadParameter method

    /**
       \brief access to the private _runHeader vector
     */
    std::vector<uint32_t>& runHeader(){return _runHeader;}

    /**
       \brief Stop the run

       \details it disables the polling and delete the associated thread
     */
    void stop();
    
    void summary();///< print out of received buffer

    /**
       \brief Json::Value conating the status of the processing

       \details run,event,built event,state, purge, size of the event map,number of data sources, and summary of received buffers
     */
    Json::Value status();

    /**
       \brief Allow (true) to purge old uncompleted event
     */
    void setPurge(bool t){_purge=t;}
    /**
       \brief Set event number when the run header will be written
     */
    void setRunHeaderEvent(int32_t n){_nextEventHeader=n;}
  private:
    bool _useEventId;
    uint32_t _nDifs;
    std::vector<zmprocessor* > _processors;
    std::map<uint64_t,std::vector<zdaq::buffer*> > _eventMap;
	
    boost::thread_group _gThread;
    bool _running,_purge,_writeHeader;
    uint32_t _run,_evt,_build,_totalSize,_compressedSize;
    int32_t _nextEventHeader;
    std::vector<uint32_t> _runHeader;

    std::map<std::string,uint64_t> _mReceived;
    // Status publication

    zdaq::mon::zPublisher* _statusPublisher;

  };
};
#endif
