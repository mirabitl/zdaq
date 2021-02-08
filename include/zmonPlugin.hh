#ifndef _zdaq_zmonplugin_h
#define _zdaq_zmonplugin_h

#include <stdint.h>
#include <stdlib.h>
#include "fsmweb.hh"
#include <vector>
#include <map>
#include <string>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <json/json.h>

#include <boost/interprocess/sync/interprocess_mutex.hpp>
namespace zdaq {
    /**
     \class zmonplugin
     \brief Purely virtual interface to process data

     \details A zmonPlugin is accesible as a pluggin it must implement 2 methods
     - loadProcessor
     - deleteProcessor

     \b Example

     extern "C" 
     {

     zdaq::zmonPlugin* loadMonitorPlugin(void)
     {
     return (new myprocessor);
     }

     void deleteMonitorPlugin(zdaq::zmonPlugin* obj)
     {
     delete obj;
     }

     }

     \author    Laurent Mirabito
     \version   1.0
     \date      January 2019
     \copyright GNU Public License.
  */
  class zmonPlugin
  {
  public:
    virtual void open(zdaq::fsmmessage* m)=0;
    virtual void close(zdaq::fsmmessage* m)=0;
    virtual Json::Value status()=0;
    virtual std::string hardware()=0;
    virtual void registerCommands(zdaq::fsmweb* f)=0;
    /**
       \brief Parameter setting interface
       \param params is a Json::value where parameters are stored
     */
    virtual  void loadParameters(Json::Value params)=0;
  };

};
#endif
