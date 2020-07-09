#include "fsmweb.hh"
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>
using namespace zdaq;
fsmweb::fsmweb(std::string name) : zdaq::fsm(name),_running(false),_name(name)
{
  _commands.clear();
  _service= new FSMMongo(name,this);
}
void fsmweb::serving(uint32_t port)
{
  Mongoose::Server server(port);
  server.registerController(_service);
  server.setOption("enable_directory_listing", "true");
  server.start();
  
  cout << "Server started, routes:" << endl;
  _service->dumpRoutes();
  
  while (_running) {
    ::sleep(1);
  }
  
  server.stop();
}
void fsmweb::start(uint32_t port)
{
  _running=true;
  std::stringstream s0;
  std::stringstream sw;
  s0.str(std::string());
  sw.str(std::string());
  s0<<"/FSM/"<<_name<<"/WEB";
  char hostname[80];
  char* nickname=getenv("NICKNAME");
  std::string shost;
  if (nickname!=NULL)
    shost=nickname;
  else
    {
      gethostname(hostname,80);
      shost=hostname;
    }
  sw<<"http://"<<shost<<":"<<port<<"/"<<_name;
  _webName=sw.str();
 
  g_d.create_thread(boost::bind(&fsmweb::serving,this,port));
}

std::string fsmweb::processCommand(zdaq::fsmmessage* msg)
{
 
  return zdaq::fsm::processCommand(msg);
}
void fsmweb::stop()
{
  _running=false;
  g_d.join_all();
}
void fsmweb::addCommand(std::string s,MGRFunctor f)
{
  std::pair<std::string,MGRFunctor> p(s,f);
  _commands.insert(p);
}

void fsmweb::handleRequest(Request &request, JsonResponse &response)
{
#ifdef DEBUG
  std::cout<<"cmdProcess"<<request.getUrl()<<" "<<request.getMethod()<<" "<<request.getData()<<std::endl;
#endif
  // Get the command and content
  Json::Value v; v.clear();
  std::string scommand=request.get("name", "NOTSET");
  if (scommand.compare("NOTSET")==0)
    {
      response["status"]="No command or content given";
      return;
    }
  else
    {
      std::map<std::string,MGRFunctor >::iterator icmd=_commands.find(scommand);
      if (icmd==_commands.end())
	{
	  response["status"]="Unknown Command";
	  return;
	  
	}
      JsonResponse pRep;
      icmd->second(request,pRep);
      response["answer"]=pRep;
      response["status"]="OK";
      return;
    }
}

Json::Value fsmweb::commandsList()
{
  Json::Value jrep;jrep.clear();
  for (std::map<std::string,MGRFunctor >::iterator ic=_commands.begin();ic!=_commands.end();ic++)
    {
      Json::Value jc;
      jc["name"]=ic->first;
      jrep.append(jc);
    }
  return jrep;
}




FSMMongo::FSMMongo(std::string name,fsmweb* f) : _name(name), _fsm(f)
{
  
}

void FSMMongo::fsmProcess(Request &request, JsonResponse &response)
{
  response.setHeader("Access-Control-Allow-Origin","*");
#ifdef DEBUG
  std::cout<<"fsmProcess"<<request.getUrl()<<" "<<request.getMethod()<<" "<<request.getData()<<std::endl;
#endif
  // Get the command and content
  Json::Value v; v.clear();
  std::string scommand=request.get("command", "NOTSET");
  std::string scontent=request.get("content", "NOTSET");
#ifdef DEBUG
  std::cout<<"command=>"<<scommand<<std::endl;
  std::cout<<"content=>"<<scontent<<std::endl;
#endif
  if (scommand.compare("NOTSET")==0 || scontent.compare("NOTSET")==0)
    {
      response["status"]="No command or content given";
      return;
    }
  else
    {
      v["command"]=scommand;
      Json::Reader reader;Json::Value vc;
      bool parsing =reader.parse(scontent,vc);
      
      v["content"]=vc;
      Json::FastWriter fastWriter;
#ifdef DEBUG
      std::cout<<"JSON send "<<v<<std::endl;
      std::cout<<"JSON send "<<fastWriter.write(v)<<std::endl;
#endif
      zdaq::fsmmessage m;
      m.setValue(fastWriter.write(v));
#ifdef DEBUG
      std::cout<<"CALLING FSM "<<m.value();
      std::cout<<"CALLING FSM CONTENT "<<m.content();
#endif
      std::string res=_fsm->processCommand(&m);
#ifdef DEBUG
      std::cout<<"RC FSM "<<res<<"==>"<<m.value();
#endif
      parsing =reader.parse(m.value(),response);
      return;
    }
}
void FSMMongo::cmdProcess(Request &request, JsonResponse &response)
{
  response.setHeader("Access-Control-Allow-Origin","*");
  _fsm->handleRequest(request,response);
}

void FSMMongo::List(Request &request, JsonResponse &response)
{

  response["PREFIX"]=_name;
  response["FSM"]=_fsm->transitionsList();
  response["ALLOWED"]=_fsm->allowList();
  response["CMD"]=_fsm->commandsList();
  response["STATE"]=_fsm->state();
  response["PID"]=getpid();
  response.setHeader("Access-Control-Allow-Origin","*");
}

void FSMMongo::setup()
{
  // Example of prefix, putting all the urls into "/api"
  addRouteResponse("GET", "/", FSMMongo,List, JsonResponse);
  std::stringstream os;
  os<<"/"<<_name;
  setPrefix(os.str());
  
  // Command handling
  addRouteResponse("GET", "/", FSMMongo,List, JsonResponse);
  //addRouteResponse("GET", "/", FSMMongo, fsmProcess, JsonResponse);
  addRouteResponse("GET", "/FSM",FSMMongo,fsmProcess, JsonResponse);
  addRouteResponse("GET", "/CMD",FSMMongo,cmdProcess, JsonResponse);
}
