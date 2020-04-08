#ifndef _zdaq_dummywriter_h
#define _zdaq_dummywriter_h

#include <stdint.h>
#include <stdlib.h>
#include "zmBuffer.hh"
#include "zmMerger.hh"
#include <vector>
#include <map>
#include <string>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
namespace zdaq {
  
  /**
     \class zdaq::dummywritter

     \brief A basic implementation of the zmprocessor interface
     \details Data are just thrown away
     \author    Laurent Mirabito
     \version   1.0
     \date      January 2019
     \copyright GNU Public License.
  */
  class dummywriter : public zmprocessor
  {
  public:

    /**
       \brief Constructor
       \param dire is the directory path for writing
     */
    dummywriter(std::string dire="/tmp");

    /**
       \brief Start a run
       \param run is the run number
       \details It creates the file SMM_dayMonthYear_HourMinuteSeconde_run.dat
     */
    virtual void start(uint32_t run);

    
    virtual void stop(); ///< Close the run and the file

    /**
       \brief Process the list of zda::buffer

       \param key  is the trigger ID
       \param dss is the list of zdaq::buffer
     */
    virtual  void processEvent(uint32_t key,std::vector<zdaq::buffer*> dss);

    /**
       \brief Store the run header

       \param header a (256 max) vector of int to be stored

       \details It copies the vectore in a zdaq::buffer with detectorId 255 and eventId 0 and store it on disk
     */
    virtual  void processRunHeader(std::vector<uint32_t> header);

    /**
       \brief Set parameters with a JSON descriptor

       \params a Json::Value object conating parameters values
     */
    virtual void loadParameters(Json::Value params); 
    uint32_t totalSize(); ///< Size of dat written
    uint32_t eventNumber(); ///< Event number 
    uint32_t runNumber(); ///<Run Number
  private:
    std::string _directory;
    uint32_t _run,_event,_totalSize;
    int32_t _fdOut;
    bool _started,_dummy;
  };
};
#endif
