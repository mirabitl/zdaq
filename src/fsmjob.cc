#include "fsmjob.hh"
#include "fsmwebCaller.hh"
#include "fileTailer.hh"
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <string>
#include <iostream>
using namespace std;
bool checkIP(std::string host)
{  int i;
    struct hostent *he;
    struct hostent *hlocal;
    struct in_addr **addr_list;
    struct in_addr **addr_local;

    vector<string> localips;
    vector<string> hostips;
    
    
    if ((he = gethostbyname(host.c_str())) == NULL) {  // get the host info
        herror("gethostbyname");
        return false;
    }
    addr_list = (struct in_addr **)he->h_addr_list;
    for(i = 0; addr_list[i] != NULL; i++) {
      //printf("%s ", inet_ntoa(*addr_list[i]));
      hostips.push_back(std::string(inet_ntoa(*addr_list[i])));
    }
    
    // print information about this host:
    //printf("Official name is: %s\n", he->h_name);
    char domain[1024];
    int len=1024;
    gethostname(domain,len);
    // printf("Official PC name is: %s\n", domain);
    hlocal = gethostbyname(domain);
    //printf("    IP addresses: ");
    addr_list = (struct in_addr **)hlocal->h_addr_list;
    for(i = 0; addr_list[i] != NULL; i++) {
      //printf("%s ", inet_ntoa(*addr_list[i]));
	localips.push_back(std::string(inet_ntoa(*addr_list[i])));
    }
    //printf("\n");

    for (auto hi:hostips)
      for (auto hl:localips)
	if (hi.compare(hl)==0)
	  {//printf(" found \n");
	    return true;}
    return false;
}





std::string wget(std::string url);

using namespace zdaq;

#define INFO_PRINT_ENABLED 1

#if DEBUG_PRINT_ENABLED
#define INFO_PRINT_ENABLED 1
#define DEBUG_PRINT printf
#else
#define DEBUG_PRINT(format, args...) ((void)0)
#endif

#if INFO_PRINT_ENABLED
#define INFO_PRINT printf
#else
#define INFO_PRINT(format, args...) ((void)0)
#endif

#define _jobControl_NMAX 4096
#define _jobControl_MMAX 16
#define _jobControl_MAXENV 100

std::string &trimString(std::string &str)
{
	if(str.empty())
		return str;

	while(str.at(0) == ' ')
		str.erase(0, 1);

	if(str.empty())
		return str;

	while(str.at(str.size()-1) == ' ')
		str.erase(str.size()-2, 1);

	return str;
}

//-------------------------------------------------------------------------------------------------

std::string processStatus(uint32_t lpid)
{
    std::stringstream s;
    s << "/proc/" << lpid << "/status";

    std::ifstream infile(s.str().c_str());

    if (!infile.good())
      return std::string("X (dead)");

    std::string line;

    while (std::getline(infile, line))
    {
    	if (line.substr(0,6).compare("State:") == 0)
    	{
    		std::string state = line.substr(6);
    		return trimString(state);
    	}
    }

    infile.close();

    return std::string("X (dead)");
}

processData::processData(const std::string &jsonString) : m_string(jsonString)
{
  //  std::cout<<" On rentre dans process Data"<<std::endl;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(jsonString, m_processInfo);
  // std::cout<<" On rentre dans process Data 1"<<std::endl;
  if (parsingSuccessful)
    {
      Json::StyledWriter styledWriter;
      std::cout << styledWriter.write(m_processInfo) << std::endl;
    }
  // std::cout<<" On rentre dans process Data 2"<<std::endl;
  m_childPid = 0;
  m_status = NOT_CREATED;

}
fsmjob::fsmjob(std::string name,uint32_t port)  : m_port(port),_login("")
{
  _fsm=new fsmweb(name);



  char hname[80];
  gethostname(hname, 80);
  m_hostname = hname;

  // Register state
  _fsm->addState("CREATED");
  _fsm->addState("REGISTERING");
  _fsm->addState("INITIALISED");
  _fsm->addState("RUNNING");

  _fsm->addTransition("INITIALISE","CREATED","INITIALISED",boost::bind(&fsmjob::initialise, this,_1));
  _fsm->addTransition("REGISTRATION","CREATED","REGISTERING",boost::bind(&fsmjob::registration, this,_1));
  _fsm->addTransition("REGISTERJOB","REGISTERING","REGISTERING",boost::bind(&fsmjob::registerjob, this,_1));
  _fsm->addTransition("ENDREGISTRATION","REGISTERING","INITIALISED",boost::bind(&fsmjob::endregistration, this,_1));
  _fsm->addTransition("START","INITIALISED","RUNNING",boost::bind(&fsmjob::start, this,_1));
  _fsm->addTransition("KILL","RUNNING","INITIALISED",boost::bind(&fsmjob::kill, this,_1));
  _fsm->addTransition("DESTROY","INITIALISED","CREATED",boost::bind(&fsmjob::destroy, this,_1));

  _fsm->addCommand("STATUS",boost::bind(&fsmjob::status, this,_1,_2));
  _fsm->addCommand("JOBLOG",boost::bind(&fsmjob::joblog, this,_1,_2));
  _fsm->addCommand("KILLJOB",boost::bind(&fsmjob::killjob, this,_1,_2));
  _fsm->addCommand("RESTARTJOB",boost::bind(&fsmjob::restartjob, this,_1,_2));
  //_fsm->addCommand("REGISTERJOB",boost::bind(&fsmjob::registerjob, this,_1,_2));
  _fsm->addCommand("REGISTERFILE",boost::bind(&fsmjob::registerfile, this,_1,_2));


  // parse if any the configuration file
  std::string line;
  std::ifstream myfile ("/etc/ljc.conf");
  _envConf.clear();
  if (myfile.is_open())
  {
    while ( getline (myfile,line) )
    {
      std::cout << line << '\n';
      std::string::size_type n;
      n=line.find("=");
      if (n == std::string::npos) {
        std::cout << " = not found\n";
      } else {
        std::cout << "found: " << line.substr(0,n) <<"---->"<< line.substr(n+1,line.length())<<'\n';
	setenv( line.substr(0,n).c_str(),line.substr(n+1,line.length()).c_str(),1);
      }

      _envConf.push_back(line);
    }
    myfile.close();
  }

  else cout << "Unable to open /etc/ljc.conf file"; 
  //Start server
  char* wl=getenv("WEBLOGIN");
  if (wl!=NULL) _login=std::string(wl);
  _fsm->start(port);
}


void fsmjob::registration(zdaq::fsmmessage* m)
{
  std::cout<<"Received "<<m->command()<<std::endl;
  std::cout<<"Received "<<m->value()<<std::endl;

  // Delet existing datasources
  m_jfile.clear();m_jconf.clear();
}
void fsmjob::endregistration(zdaq::fsmmessage* m)
{
  std::cout<<"Received "<<m->command()<<std::endl;
  std::cout<<"Received "<<m->value()<<std::endl;

  m->setAnswer(m_jconf);
}

void fsmjob::buildJobConfig()
{
  for (auto x:m_jfile["HOSTS"].getMemberNames())
    if (checkIP(x))
      {
	 m_jconf=m_jfile["HOSTS"][x];
	 return;
      }
}

void fsmjob::initialise(zdaq::fsmmessage* m)
{
  std::cout<<"Received "<<m->command()<<std::endl;
  std::cout<<"Received "<<m->value()<<std::endl;

  // Delet existing datasources
  m_jfile.clear();m_jconf.clear();
 
  for (PidToProcessMap::iterator it=m_processMap.begin();it!=m_processMap.end();it++)
      delete it->second;
  m_processMap.clear();
  // Add a data source
  // Parse the json message
  // {"command": "CONFIGURE", "content": {"detid": 100, "sourceid": [23, 24, 26]}}
  Json::Value jc=m->content();
   if (jc.isMember("login"))
    {
      _login=jc["login"].asString();
    }
  // else
  //   _login=std::string("");
   std::cout<<"WEB LOGIN is "<<_login<<std::endl<<std::flush;
  if (jc.isMember("file"))
    {
      std::string fileName=jc["file"].asString();
      Json::Reader reader;
      std::ifstream ifs (fileName.c_str(), std::ifstream::in);
      
      bool parsingSuccessful = reader.parse(ifs, m_jfile,false);
      
      if (parsingSuccessful)
	{
	  this->buildJobConfig();
	  //m_jconf=m_jfile["HOSTS"][m_hostname];
	  Json::StyledWriter styledWriter;
	  std::cout << styledWriter.write(m_jconf) << std::endl;
	}
    }
  else
    if (jc.isMember("url"))
      {
	std::string url=jc["url"].asString();
	std::cout<<url<<std::endl;
    std::cout<<"Hostname "<<m_hostname<<std::endl;
    /*
	std::string jsconf=wget(url);
	std::cout<<jsconf<<std::endl;
	Json::Reader reader;
	bool parsingSuccessful = reader.parse(jsconf, m_jfile);
    */

	std::string jsconf=fsmwebCaller::curlQuery(url,_login);
	std::cout<<jsconf<<std::endl;
	Json::Reader reader;
	Json::Value jcc;
	bool parsingSuccessful = reader.parse(jsconf,jcc);
	if (jcc.isMember("content"))
	  m_jfile=jcc["content"];
	else
	  m_jfile=jcc;


	
	if (parsingSuccessful)
	  {
	    //m_jconf=m_jfile["HOSTS"][m_hostname];
	    this->buildJobConfig();
	    Json::StyledWriter styledWriter;
	    std::cout << styledWriter.write(m_jconf) << std::endl;
	  }
      }
  
  // Overwrite msg
    //Prepare complex answer
  m->setAnswer(m_jconf);
 
}
void fsmjob::startProcess(zdaq::processData* pProcessData)
{
  if (pProcessData->m_status != zdaq::processData::NOT_CREATED)
    return;

  std::string programName = pProcessData->m_processInfo["PROGRAM"].asString();
  std::string processName = pProcessData->m_processInfo["NAME"].asString();
  std::vector<std::string> arguments;
  std::vector<std::string> environmentVars;
  
  for (uint32_t ia=0 ; ia<pProcessData->m_processInfo["ARGS"].size() ; ia++)
    arguments.push_back(pProcessData->m_processInfo["ARGS"][ia].asString());

  for (uint32_t ia=0;ia<pProcessData->m_processInfo["ENV"].size();ia++)
    environmentVars.push_back(pProcessData->m_processInfo["ENV"][ia].asString());

  signal(SIGCHLD, SIG_IGN);

  // forking
  pid_t pid = fork();

  // parent case
  if (pid != 0)
    {
      pProcessData->m_childPid = pid;
      pProcessData->m_status = zdaq::processData::RUNNING;
      return;
    }

  // child case
  char executivePath[_jobControl_NMAX];
  char argv[_jobControl_MMAX][_jobControl_NMAX];  // build and initialize argv[][]
  char *pArgv[_jobControl_MMAX];
  char envp[_jobControl_MAXENV][_jobControl_NMAX];
  char* pEnvp[_jobControl_MAXENV];
  

  // Executive Path
  sprintf(executivePath, programName.c_str());
  
  // fills arguments list
  for (int i = 0; i<_jobControl_MMAX ; i++) {
    for (int j = 0; j<_jobControl_NMAX ; j++) {
      argv[i][j] = (char)NULL;
    }
  }

  int i = 1;
  
  for (std::vector<std::string>::const_iterator iter = arguments.begin(), endIter = arguments.end() ;
       iter != endIter ; ++iter)
    {
      sprintf( argv[i], "%s", (*iter).c_str());
      pArgv[i] = & argv[i][0];
      i++;
    }

  pArgv[0] = executivePath;
  pArgv[i] = NULL;
  
  // brute force close
  // xdaq only opens first 5.
  close(0);
  for (int i = 3; i < 32; i++)
    close(i);
  
  i = 0;

  // Fills environment list
  for (std::vector<std::string>::const_iterator iter = _envConf.begin(), endIter = _envConf.end() ;
       endIter != iter ; ++iter)
    {
      sprintf( envp[i], "%s", (*iter).c_str());
      pEnvp[i] = & envp[i][0];
      i++;
    }
  for (std::vector<std::string>::const_iterator iter = environmentVars.begin(), endIter = environmentVars.end() ;
       endIter != iter ; ++iter)
    {
      sprintf( envp[i], "%s", (*iter).c_str());
      pEnvp[i] = & envp[i][0];
      i++;
    }

    std::stringstream ss;
    ss<<"PROCESSNAME="<<processName;
    sprintf(envp[i],"%s",ss.str().c_str());
    pEnvp[i]=&envp[i][0];
    i++;
  pEnvp[i] = NULL;
  
  // set new user id to root
  int ret = 0;
  ret = setuid(0);
  
  if ( ret != 0 )
    {
      //Let's try a second time
      ret = setuid(0);
      
      if ( ret != 0 )
	INFO_PRINT("child: FATAL couldn't setuid() to %i.\n",0);
    }

  
  // open procID+log for stdout and stderr
  char logPath[100];
  
  pid_t mypid = getpid(); // get my pid to append to filename
  sprintf(logPath,"/tmp/fsmjobPID%i.log",mypid);             // construct filename to /tmp/....

  try
    {
      int tmpout = open( logPath , O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH );    // open file
      dup2( tmpout, 1 );                                         // stdout to file
      dup2( tmpout, 2 );                                         // stderr to file
      close( tmpout );                                       // close unused descriptor
    }
  catch (std::exception &e)
    {
      INFO_PRINT("child: FATAL couldn't write log file to %s.\n", logPath);
      exit(-1);
    }
  catch (...)
    {
      INFO_PRINT("child: FATAL couldn't write log file to %s.\n", logPath);
      exit(-1);
    }

  ret = execve( executivePath, pArgv, pEnvp);

  INFO_PRINT("jobControl: FATAL OOps, we came back with ret = %i , errno = %i , dying",ret, errno);
  exit(-1);
}

void fsmjob::killProcess(uint32_t pid, uint32_t sig)
{
	PidToProcessMap::iterator iter = m_processMap.begin();

	while (iter != m_processMap.end())
	{
	  if (iter->second->m_status == zdaq::processData::RUNNING && pid == iter->first)
		{
		  PidToProcessMap::iterator toErase = iter;

		  ::kill(iter->first, sig);

		  iter->second->m_status = zdaq::processData::KILLED;
		  delete iter->second;

		  ++iter;
		  m_processMap.erase(toErase);

		  break;
		}
	  else
	    ++iter;
	}
}

void fsmjob::start(zdaq::fsmmessage* m)
{
  //  std::cout<<"ON RENTRE "<<m_jconf<<std::endl;
  const Json::Value& books = m_jconf;
  Json::FastWriter fastWriter;
  for (Json::ValueConstIterator it = books.begin(); it != books.end(); ++it)
    {
      const Json::Value& book = *it;

      //  std::cout<<"In Start "<<book<<std::endl;
      std::string jsonString = fastWriter.write(book);
      // std::cout<<"In Start1 "<<std::endl;
		// create, start and register the process
      zdaq::processData *pProcessData = new zdaq::processData(jsonString);
      //std::cout<<" apres le process "<<std::endl;
      //getchar();
      this->startProcess(pProcessData);
      //std::cout<<" apres le start "<<pProcessData->m_childPid<<std::endl;
      m_processMap.insert(PidToProcessMap::value_type(pProcessData->m_childPid, pProcessData));

    }
  m->setAnswer(this->jsonStatus());
  
}

Json::Value fsmjob::jsonStatus()
{
  Json::Value array;

  for (PidToProcessMap::iterator iter = m_processMap.begin(), endIter = m_processMap.end() ;
       endIter != iter ; ++iter)
    {
      Json::Value pinf;
      
      pinf["HOST"] = m_hostname;
      pinf["PID"] = iter->first;
      pinf["NAME"] = iter->second->m_processInfo["NAME"];
      pinf["STATUS"] = processStatus(iter->first);

      for (uint32_t ia=0;ia<iter->second->m_processInfo["ENV"].size();ia++)
	{
	  std::string svar=iter->second->m_processInfo["ENV"][ia].asString();
	  if (svar.substr(0,7).compare("WEBPORT")==0)
	  {
	    pinf["PORT"]=svar.substr(8,svar.length());
	  }
	}
      array.append(pinf);
    }
  return array;
}
void fsmjob::kill(zdaq::fsmmessage* m)
{
  
  const Json::Value& books = this->jsonStatus();
  for (Json::ValueConstIterator it = books.begin(); it != books.end(); ++it)
    {
      const Json::Value& book = *it;
      this->killProcess(book["PID"].asUInt());

    }
  m->setAnswer(this->jsonStatus());
  
}

void fsmjob::destroy(zdaq::fsmmessage* m)
{
  // Done  m_jfile.clear();m_jconf.clear();at the start of initialise
   m_jfile.clear();m_jconf.clear();
}
void fsmjob::status(Mongoose::Request &request, Mongoose::JsonResponse &response)
{
  response["JOBS"]=this->jsonStatus();
  std::cout<<"STATUS"<<m_jfile;
  if (m_jfile.isMember("NAME"))
    response["NAME"]=m_jfile["NAME"];
  else
    response["NAME"]="UNKNOWN";

  std::cout<<"RESPONSE =>"<<response;
  response["STATUS"]="DONE";
}
void fsmjob::killjob(Mongoose::Request &request, Mongoose::JsonResponse &response)
{
  std::string name=request.get("processname","NONE");
  uint32_t pid =atol(request.get("pid","0").c_str());
  uint32_t sig =atol(request.get("signal","9").c_str());
  if (pid ==0 && name.compare("NONE")==0)
    {
      response["STATUS"]="No pid or name given";
      return;
    }
  if (pid!=0)
    {
      killProcess(pid,sig);
      response["JOBS"]=this->jsonStatus();
      response["STATUS"]="DONE";
      return;
    }
  
  for (PidToProcessMap::iterator iter = m_processMap.begin(), endIter = m_processMap.end() ;
       endIter != iter ; ++iter)
      if (iter->second->m_processInfo["NAME"].asString().compare(name)==0)
	{pid=iter->first;break;}

  if (pid!=0)
    {
      killProcess(pid,sig);
      response["JOBS"]=this->jsonStatus();
      response["STATUS"]="DONE";
      return;
    }
  else
    {
      response["JOBS"]=this->jsonStatus();
      response["STATUS"]="name not found in pid list";
    }
}
void fsmjob::restartjob(Mongoose::Request &request, Mongoose::JsonResponse &response)
{
  std::string name=request.get("processname","NONE");
  uint32_t pid =atol(request.get("pid","0").c_str());
  uint32_t sig =atol(request.get("signal","9").c_str());
  if (pid ==0 && name.compare("NONE")==0)
    {
      response["STATUS"]="No pid or name given";
      return;
    }

  std::string jss="NONE";
  for (PidToProcessMap::iterator iter = m_processMap.begin(), endIter = m_processMap.end() ;endIter != iter ; ++iter)
    if (iter->second->m_processInfo["NAME"].asString().compare(name)==0 || pid==iter->first)
	{
	  pid=iter->first;
	  jss=iter->second->m_string;
	  break;
	}

  
  if (pid!=0 && jss.compare("NONE")!=0)
    {
      killProcess(pid,sig);

      zdaq::processData *pProcessData = new zdaq::processData(jss);
      this->startProcess(pProcessData);
      m_processMap.insert(PidToProcessMap::value_type(pProcessData->m_childPid, pProcessData));
      response["JOBS"]=this->jsonStatus();
      response["STATUS"]="DONE";
      return;
    }
  response["JOBS"]=this->jsonStatus();
  response["STATUS"]="pid or name not found in pid list";
    
}

void fsmjob::joblog(Mongoose::Request &request, Mongoose::JsonResponse &response)
{
  std::string name=request.get("processname","NONE");
  uint32_t pid =atol(request.get("pid","0").c_str());
  uint32_t nlines=atol(request.get("lines","100").c_str());


  

  if (pid ==0 && name.compare("NONE")==0)
    {
      response["STATUS"]="No pid or name given";
      return;
    }
  if (pid==0)
    for (PidToProcessMap::iterator iter = m_processMap.begin(), endIter = m_processMap.end() ;
	 endIter != iter ; ++iter)
      if (iter->second->m_processInfo["NAME"].asString().compare(name)==0)
	{pid=iter->first;break;}


  if (pid!=0)
    {
      std::stringstream s;
      s<<"/tmp/fsmjobPID"<<pid<<".log";
      std::stringstream so;
      fileTailer t(1024*512);
      char buf[1024*512];
      t.tail(s.str(),nlines,buf);
      so<<buf;
      response["STATUS"]="DONE";
      response["FILE"]=s.str();
      response["LINES"]=so.str();
    }
  else
    {
      response["JOBS"]=this->jsonStatus();
      response["STATUS"]="name not found in pid list";
    }
}


void fsmjob::registerjob(zdaq::fsmmessage* m)
{
  Json::Value jcr=m->content();
  std::string pname=jcr["processname"].asString();
  std::string pargs=jcr["processargs"].asString();
  std::string penv=jcr["processenv"].asString();
  std::string pbin=jcr["processbin"].asString();


  std::cout <<pname<<std::endl;
  std::cout <<pargs<<std::endl;
  std::cout <<penv<<std::endl;
  std::cout <<pbin<<std::endl;
    Json::Value jc,jargs,jenv,jbin;
  jc["NAME"]=pname;

  Json::Reader reader;
  bool parsingSuccessful = reader.parse(pargs,jargs);
  parsingSuccessful = reader.parse(penv,jenv);
  parsingSuccessful = reader.parse(pbin,jbin);
  jc["ARGS"]=jargs;
  jc["ENV"]=jenv;
  jc["PROGRAM"]=jbin;

  const Json::Value& books = m_jconf;
  for (Json::ValueConstIterator it = books.begin(); it != books.end(); ++it)
    {
      if ((*it)["NAME"].asString().compare(pname)==0)
	{
	  return;
	}
    }
  m_jconf.append(jc);
  m->setAnswer(m_jconf);
    
}
void fsmjob::registerfile(Mongoose::Request &request, Mongoose::JsonResponse &response)
{
 if (_fsm->state().compare("INITIALISED")!=0)
    {
      response["STATUS"]="cannot register jobs if not initialised";
      return;

    }
  // Delet existing datasources
  m_jfile.clear();m_jconf.clear();
  
  for (PidToProcessMap::iterator it=m_processMap.begin();it!=m_processMap.end();it++)
      delete it->second;
  m_processMap.clear();
  // Add a data source
  // Parse the json message
  // {"command": "CONFIGURE", "content": {"detid": 100, "sourceid": [23, 24, 26]}}

  std::string fileName=request.get("file","NONE");
  if (fileName.compare("NONE")==0)
    {
      response["STATUS"]="No file  given";
      return;
    }
  
  Json::Reader reader;
  std::ifstream ifs (fileName.c_str(), std::ifstream::in);

  bool parsingSuccessful = reader.parse(ifs, m_jfile,false);

  if (parsingSuccessful)
    {
      m_jconf=m_jfile["HOSTS"][m_hostname];
      Json::StyledWriter styledWriter;
      std::cout << styledWriter.write(m_jconf) << std::endl;
    }
  // Overwrite msg
    //Prepare complex answer
  response["JOBS"]=m_jconf;
  response["STATUS"]="DONE";

}
