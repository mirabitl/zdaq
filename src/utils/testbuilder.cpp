#include "zmMerger.hh"
#include "fsmweb.hh"
#include "zhelpers.hpp"
#include <boost/interprocess/sync/interprocess_mutex.hpp>

class dsBuilder {
public:
  dsBuilder(std::string name,uint32_t port);
  void configure(zdaq::fsmmessage* m);
  void start(zdaq::fsmmessage* m);
  void stop(zdaq::fsmmessage* m);
  void halt(zdaq::fsmmessage* m);
  void status(Mongoose::Request &request, Mongoose::JsonResponse &response);

private:
  zdaq::fsmweb* _fsm;
  zdaq::zmMerger* _merger;
  bool _running,_readout;
  zmq::context_t* _context;

};



dsBuilder::dsBuilder(std::string name,uint32_t port) : _running(false),_merger(NULL)
{
  _fsm=new zdaq::fsmweb(name);
  _context = new zmq::context_t (1);
  _merger= new zdaq::zmMerger(_context);
  // Register state
  _fsm->addState("CREATED");
  _fsm->addState("CONFIGURED");
  _fsm->addState("RUNNING");
  _fsm->addTransition("CONFIGURE","CREATED","CONFIGURED",boost::bind(&dsBuilder::configure, this,_1));
  _fsm->addTransition("CONFIGURE","CONFIGURED","CONFIGURED",boost::bind(&dsBuilder::configure, this,_1));
  _fsm->addTransition("START","CONFIGURED","RUNNING",boost::bind(&dsBuilder::start, this,_1));
  _fsm->addTransition("STOP","RUNNING","CONFIGURED",boost::bind(&dsBuilder::stop, this,_1));
  _fsm->addTransition("HALT","RUNNING","CREATED",boost::bind(&dsBuilder::halt, this,_1));
  _fsm->addTransition("HALT","CONFIGURED","CREATED",boost::bind(&dsBuilder::halt, this,_1));

  _fsm->addCommand("STATUS",boost::bind(&dsBuilder::status, this,_1,_2));


  //Start server
  _fsm->start(port);
}

void dsBuilder::configure(zdaq::fsmmessage* m)
{
  std::cout<<"Received "<<m->command()<<std::endl;
  std::cout<<"Received "<<m->value()<<std::endl;

  
  Json::Value jc=m->content();
  _merger->setNumberOfDataSource(jc["dif"].asInt());
  const Json::Value& books = jc["stream"];
  Json::Value array_keys;
  for (Json::ValueConstIterator it = books.begin(); it != books.end(); ++it)
    {
      const Json::Value& book = *it;
      std::cout<<"Registering"<<(*it).asString()<<std::endl;
      _merger->registerDataSource((*it).asString());
      std::cout<<"done"<<std::endl;
      array_keys.append((*it).asString());

    }
  const Json::Value& pbooks = jc["processor"];
  Json::Value parray_keys;
  for (Json::ValueConstIterator it = pbooks.begin(); it != pbooks.end(); ++it)
    {
      const Json::Value& book = *it;
      _merger->registerProcessor((*it).asString());
      parray_keys.append((*it).asString());

    }

  
  // Overwrite msg
    //Prepare complex answer
  Json::Value prep;
  prep["sourceRegistered"]=array_keys;
  prep["processorRegistered"]=parray_keys;
       
  m->setAnswer(prep);
  std::cout <<"end of configure"<<std::endl;
  return;
}

void dsBuilder::start(zdaq::fsmmessage* m)
{
    std::cout<<"Received "<<m->command()<<std::endl;
    Json::Value jc=m->content();
    _merger->start(jc["run"].asInt());
    _running=true;
}
void dsBuilder::stop(zdaq::fsmmessage* m)
{
  
  _merger->stop();
  _running=false;
}
void dsBuilder::halt(zdaq::fsmmessage* m)
{
  
  
    std::cout<<"Received "<<m->command()<<std::endl;
    if (_running)
      this->stop(m);
    std::cout<<"Destroying"<<std::endl;
    //stop data sources
    _merger->clear();
}
void dsBuilder::status(Mongoose::Request &request, Mongoose::JsonResponse &response)
{
    std::cout<<"dowmnload"<<request.getUrl()<<" "<<request.getMethod()<<" "<<request.getData()<<std::endl;
    if (_merger!=NULL)
      {

      response["answer"]=_merger->status();

      }
    else
      response["answer"]="NO merger created yet";
}


int main()
{
  dsBuilder s("ZMTEST",46000);
  while (1)
    {
      ::sleep(1);
    }
}





