#ifndef _zdaq_zmonstore_h
#define _zdaq_zmonstore_h

#include <stdint.h>
#include <stdlib.h>
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
     \class zmonstore
     \brief Purely virtual interface to process data

     \details A zmonStore is accesible as a pluggin it must implement 2 methods
     - loadProcessor
     - deleteProcessor

     \b Example

     extern "C" 
     {

     zdaq::zmonStore* loadStore(void)
     {
     return (new myprocessor);
     }

     void deleteStore(zdaq::zmonStore* obj)
     {
     delete obj;
     }

     }

     \author    Laurent Mirabito
     \version   1.0
     \date      January 2019
     \copyright GNU Public License.
  */
  class zmonStore
  {
  public:
    virtual void connect()=0;
    virtual void store(std::string loc,std::string hw,uint32_t ti,Json::Value status)=0;
    /**
       \brief Parameter setting interface
       \param params is a Json::value where parameters are stored
     */
    virtual  void loadParameters(Json::Value params)=0;
  };

};
#endif
