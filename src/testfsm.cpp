#include "zmPusher.hh"
#include "fsmweb.hh"
#include "zhelpers.hpp"
class dsServer {
public:
  dsServer(std::string name,uint32_t port);
  void configure(zdaq::fsmmessage* m);
  void start(zdaq::fsmmessage* m);
  void stop(zdaq::fsmmessage* m);
  void halt(zdaq::fsmmessage* m);
  void readdata(zdaq::zmPusher *ds);
  void download(Mongoose::Request &request, Mongoose::JsonResponse &response);
  void list(Mongoose::Request &request, Mongoose::JsonResponse &response);

private:
  fsmweb* _fsm;
  std::vector<zdaq::zmPusher*> _sources;
  bool _running,_readout;
  boost::thread_group _gthr;
  zmq::context_t* _context;
};



dsServer::dsServer(std::string name,uint32_t port) : _running(false)
{
  _fsm=new fsmweb(name);
  _context = new zmq::context_t (1);

  // Register state
  _fsm->addState("CREATED");
  _fsm->addState("CONFIGURED");
  _fsm->addState("RUNNING");
  _fsm->addTransition("CONFIGURE","CREATED","CONFIGURED",boost::bind(&dsServer::configure, this,_1));
    _fsm->addTransition("CONFIGURE","CONFIGURED","CONFIGURED",boost::bind(&dsServer::configure, this,_1));
  _fsm->addTransition("START","CONFIGURED","RUNNING",boost::bind(&dsServer::start, this,_1));
  _fsm->addTransition("STOP","RUNNING","CONFIGURED",boost::bind(&dsServer::stop, this,_1));
  _fsm->addTransition("HALT","RUNNING","CREATED",boost::bind(&dsServer::halt, this,_1));
  _fsm->addTransition("HALT","CONFIGURED","CREATED",boost::bind(&dsServer::halt, this,_1));

  _fsm->addCommand("DOWNLOAD",boost::bind(&dsServer::download, this,_1,_2));
  _fsm->addCommand("LIST",boost::bind(&dsServer::list, this,_1,_2));

  //Start server
  _fsm->start(port);
}

void dsServer::configure(zdaq::fsmmessage* m)
{
  std::cout<<"Received "<<m->command()<<std::endl;
  std::cout<<"Received "<<m->value()<<std::endl;

  // Delet existing zmPushers
  for (std::vector<zdaq::zmPusher*>::iterator it=_sources.begin();it!=_sources.end();it++)
      delete (*it);
  _sources.clear();
  // Add a data source
  // Parse the json message
  // {"command": "CONFIGURE", "content": {"detid": 100, "sourceid": [23, 24, 26]}}
  
  Json::Value jc=m->content();
  int32_t det=jc["detid"].asInt();
  const Json::Value& books = jc["sourceid"];
  Json::Value array_keys;
  for (Json::ValueConstIterator it = books.begin(); it != books.end(); ++it)
    {
      const Json::Value& book = *it;
      int32_t sid=(*it).asInt();
    // rest as before
      std::cout <<"Creating datatsource "<<det<<" "<<sid<<std::endl;
      array_keys.append((det<<16)|sid);
      zdaq::zmPusher* ds= new zdaq::zmPusher(_context,det,sid);
      ds->connect("ipc:///tmp/DSIN.ipc");
      //ds->connect("tcp://lyosdhcal10:5556");
      _sources.push_back(ds);

    }

  // Overwrite msg
    //Prepare complex answer
  m->setAnswer(array_keys);
  std::cout <<"end of configure"<<std::endl;
  return;
  /*
  Json::Value rep;
  rep["command"]=m->command();
  rep["content"]=m->content();
  rep["content"]["answer"]=array_keys;
  Json::FastWriter fastWriter;
  m->setValue(fastWriter.write(rep));
  */
}
void dsServer::readdata(zdaq::zmPusher *ds)
{
  uint32_t evt=0; uint64_t bx=0;
  std::srand(std::time(0));
  while (_running)
    {
      ::usleep(1000);
      //      ::sleep(1);
      if (!_running) break;
      if (evt%100==0)
	std::cout<<"Thread of "<<ds->sourceId()<<" is running "<<evt<<" "<<_running<<std::endl;
      // Just fun , ds is publishing a buffer containing sourceid X int of value sourceid
      uint32_t psi=std::rand()%65535+1;
      
      uint32_t* pld=(uint32_t*) ds->payload();
      //std::cout<<psi*4<<std::endl;
      for (int i=0;i<psi;i++) pld[i]= std::rand();
      pld[0]=evt;
      pld[psi-1]=evt;
      bx=time(0);
      //std::cout<<"sending"<<std::endl;
      ds->publish(bx,evt,psi*sizeof(uint32_t));
      //std::cout<<"sent"<<std::endl;
     evt++;
    }
 
  std::cout<<"Thread of "<<ds->sourceId()<<" is exiting"<<std::endl;
}
void dsServer::start(zdaq::fsmmessage* m)
{
    std::cout<<"Received "<<m->command()<<std::endl;

    _running=true;

    for (std::vector<zdaq::zmPusher*>::iterator ids=_sources.begin();ids!=_sources.end();ids++)
      {
	_gthr.create_thread(boost::bind(&dsServer::readdata, this,(*ids)));
	::usleep(500000);
      }

}
void dsServer::stop(zdaq::fsmmessage* m)
{
  
  
    std::cout<<"Received "<<m->command()<<std::endl;
  
    // Stop running
    _running=false;
    ::sleep(1);
    std::cout<<"joining"<<std::endl;
    _gthr.join_all();
}
void dsServer::halt(zdaq::fsmmessage* m)
{
  
  
    std::cout<<"Received "<<m->command()<<std::endl;
    if (_running)
      this->stop(m);
    std::cout<<"Destroying"<<std::endl;
    //stop data sources
    for (std::vector<zdaq::zmPusher*>::iterator it=_sources.begin();it!=_sources.end();it++)
      delete (*it);
     _sources.clear();
}
void dsServer::download(Mongoose::Request &request, Mongoose::JsonResponse &response)
{
    std::cout<<"dowmnload"<<request.getUrl()<<" "<<request.getMethod()<<" "<<request.getData()<<std::endl;
    response["answer"]="download called";
}

void dsServer::list(Mongoose::Request &request, Mongoose::JsonResponse &response)
{
    std::cout<<"list"<<request.getUrl()<<" "<<request.getMethod()<<" "<<request.getData()<<std::endl;
    response["answer"]="list called";
}


int main()
{
  dsServer s("ZMTEST",45000);
  while (1)
    {
      ::sleep(1);
    }
}





