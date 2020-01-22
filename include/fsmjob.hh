#ifndef _ZDAQFSMJOB_HH
#define _ZDAQFSMJOB_HH
#include "fsmweb.hh"
#include <iostream>
#include <sstream>
// -- std headers
#include <stdint.h>
#include <sys/types.h>
#include <map>
#include <string>

// -- json headers
#include "json/json.h"

namespace zdaq
{
  namespace jc
  {
    /**
       \class processData
       \author    Laurent Mirabito
       \version   1.0
       \date      January 2019
       \copyright GNU Public License.

       *  @brief  Process Information  class
       */
    class processData
    {
    public:
      /**
       *  @brief  Status enum
       */
      enum Status
	{
	  NOT_CREATED = 0,
	  RUNNING = 1,
	  KILLED = 2
	};

      /**
       *  @brief  Constructor. Construct a process structure from a json string
       *          that contains all needed infos on a process
       *
       *  @param  jsonString the jsonString to initialize the process
       */
      processData(const std::string &jsonString);

      Json::Value    m_processInfo;     ///< The json process value
      pid_t          m_childPid;        ///< The process pid
      Status         m_status;          ///< The process status
      std::string m_string;
    };

    /**
     
       \class fsmjob
       \brief main class of job control mainly inspired from XDAQ jobcontrol

       \details It's a web application running on port 9999 on each computer
       that can start or kill process on demand
       \author    Laurent Mirabito
       \version   1.0
       \date      January 2019
       \copyright GNU Public License.
     

    */
    class fsmjob {
    public:
      /**
	 \brief Constructor
     
	 \param name fsmweb prefix name
	 \param port running port
      */
      fsmjob(std::string name,uint32_t port);

      /**
	 \brief FSM handler for INITIALISE transition
	 \details the message content part has either a \a file or  \a url or \a mongo tag that point to the process configuration file taht is parsed at this stage. The computer is identified in the configuration file nad processData strucuture are created to handle the process managment

	 \param m the fsmmessage
      */
      void initialise(zdaq::fsmmessage* m);


      /**
	 \brief FSM handler for START transition
	 \details All identified processData are parse and associated process are started

	 \param m the fsmmessage
      */
  
      void start(zdaq::fsmmessage* m);

      /**
	 \brief FSM handler for KILL transition
	 \details All identified processData are parse and associated running process are killed

	 \param m the fsmmessage
      */

      void kill(zdaq::fsmmessage* m);

      /**
	 \brief FSM handler for DESTROY transition
	 \details All identified processData are deleted and the service is ready for a new configuration
     
	 \param m the fsmmessage
      */

      void destroy(zdaq::fsmmessage* m);

      /**
	 \brief FSM handler for REGISTRATION transition
	 \details clear process map for standalone job registration
     
	 \param m the fsmmessage
      */  
      void registration(zdaq::fsmmessage* m);

      /**
	 \brief FSM handler for REGISTERJOB transition
	 \details add one standalone process to the map. The \a content should define:
	 -processname
	 -processargs
	 -processenv
	 -processbin
     
	 \param m the fsmmessage
      */
      void registerjob(zdaq::fsmmessage* m);

      /**
	 \brief FSM handler for ENDREGISTRATION transition
	 \details Transition to INITIALISED
     
	 \param m the fsmmessage
      */  

      void endregistration(zdaq::fsmmessage* m);

      /**
	 \brief return in response the list of process with their status
      */
      void status(Mongoose::Request &request, Mongoose::JsonResponse &response);

      /**
	 \brief Utility to send a CREATE transition to all baseApplication of the computer with the loaded configuration
      */
      void appcreate(Mongoose::Request &request, Mongoose::JsonResponse &response);

      /**
	 \brief Get the log of a process

	 \param request should have a tag \a pid with  the process pid, a \a processname one, and \a lines one with the length of the tailed file
      */
      void joblog(Mongoose::Request &request, Mongoose::JsonResponse &response);

      /**
	 \brief Kill a  single process

	 \param request should have a tag \a pid with  the process pid, a \a processname one, and \a signal one with the kill level
      */
      void killjob(Mongoose::Request &request, Mongoose::JsonResponse &response);

      /**
	 \brief Restart  a  single process
     
	 \param request should have a tag \a pid with  the process pid, a \a processname one, and \a signal one with the kill level
      */
  
      void restartjob(Mongoose::Request &request, Mongoose::JsonResponse &response);

      /**
	 \brief Overwrite the process map with a local file definition
	 \details Transition to INITIALISED
     
	 \param request should contain a 'file' tag
      */  
      void registerfile(Mongoose::Request &request, Mongoose::JsonResponse &response);
    protected:
      void startProcess(zdaq::jc::processData* pd);
      void killProcess(uint32_t pid,uint32_t sig=9);
      Json::Value jsonStatus();
      Json::Value jsonInfo();
      void buildJobConfig();
    private:
      zdaq::fsmweb* _fsm;
      typedef std::map<pid_t,zdaq::jc::processData*> PidToProcessMap;
      Json::Value m_jfile,m_jconf,m_configContent;
      std::string        m_hostname;         ///< The host name on which the job control is running
      uint32_t m_port;
      PidToProcessMap    m_processMap;       ///< The handled process map
      std::string _login;
      std::vector<std::string> _envConf;
    };


  };
};
#endif
