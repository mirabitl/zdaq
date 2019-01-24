#include "exBuilder.hh"

using namespace zdaq;

zdaq::exBuilder::exBuilder(std::string name) : zdaq::baseApplication(name),_running(false),_merger(NULL)
{
  _context = new zmq::context_t (1);
  _merger= new zdaq::zmMerger(_context);
  // Register state
  this->fsm()->addState("CREATED");
  this->fsm()->addState("CONFIGURED");
  this->fsm()->addState("RUNNING");
  this->fsm()->addTransition("CONFIGURE","CREATED","CONFIGURED",boost::bind(&zdaq::exBuilder::configure, this,_1));
  this->fsm()->addTransition("CONFIGURE","CONFIGURED","CONFIGURED",boost::bind(&zdaq::exBuilder::configure, this,_1));
  this->fsm()->addTransition("START","CONFIGURED","RUNNING",boost::bind(&zdaq::exBuilder::start, this,_1));
  this->fsm()->addTransition("STOP","RUNNING","CONFIGURED",boost::bind(&zdaq::exBuilder::stop, this,_1));
  this->fsm()->addTransition("HALT","RUNNING","CREATED",boost::bind(&zdaq::exBuilder::halt, this,_1));
  this->fsm()->addTransition("HALT","CONFIGURED","CREATED",boost::bind(&zdaq::exBuilder::halt, this,_1));

  this->fsm()->addCommand("STATUS",boost::bind(&zdaq::exBuilder::status, this,_1,_2));
  this->fsm()->addCommand("REGISTERDS",boost::bind(&zdaq::exBuilder::registerds, this,_1,_2));
  this->fsm()->addCommand("SETHEADER",boost::bind(&zdaq::exBuilder::c_setheader,this,_1,_2));

  //Start server
  char* wp=getenv("WEBPORT");
  if (wp!=NULL)
    {
      LOG4CXX_INFO(_logZdaqex,__PRETTY_FUNCTION__<<"Service "<<name<<" started on port "<<atoi(wp));
      this->fsm()->start(atoi(wp));
    }


}

void zdaq::exBuilder::configure(zdaq::fsmmessage* m)
{
  LOG4CXX_INFO(_logZdaqex,__PRETTY_FUNCTION__<<"Received "<<m->command()<<" Value "<<m->value());
  // Store message content in paramters

  if (m->content().isMember("dsnumber"))
    { 
      this->parameters()["dsnumber"]=m->content()["dsnumber"];
    }
  if (m->content().isMember("stream"))
    { 
      this->parameters()["stream"]=m->content()["stream"];
    }
  if (m->content().isMember("processor"))
    { 
      this->parameters()["processor"]=m->content()["processor"];
    }
  // Check that all paramters exists
  if (!this->parameters().isMember("dsnumber")) {LOG4CXX_ERROR(_logZdaqex,"Missing dsnumber, number of data source");return;}
  if (!this->parameters().isMember("stream")) {LOG4CXX_ERROR(_logZdaqex,"Missing stream, list of data stream");return;}
  if (!this->parameters().isMember("processor")) {LOG4CXX_ERROR(_logZdaqex,"Missing processor, list of processing pluggins");return;}

  Json::Value jc=this->parameters();
  if (jc.isMember("dsnumber"))
    _merger->setNumberOfDataSource(jc["dsnumber"].asInt());
  
    
  const Json::Value& books = jc["stream"];
  Json::Value array_keys;
  for (Json::ValueConstIterator it = books.begin(); it != books.end(); ++it)
    {
      const Json::Value& book = *it;
      LOG4CXX_INFO(_logZdaqex,__PRETTY_FUNCTION__<<"Registering "<<(*it).asString());
      _merger->registerDataSource((*it).asString());
      
      array_keys.append((*it).asString());

    }
  const Json::Value& pbooks = jc["processor"];
  Json::Value parray_keys;
  for (Json::ValueConstIterator it = pbooks.begin(); it != pbooks.end(); ++it)
    {
      const Json::Value& book = *it;
      LOG4CXX_INFO(_logZdaqex,__PRETTY_FUNCTION__<<"registering "<<(*it).asString());
      _merger->registerProcessor((*it).asString());
      parray_keys.append((*it).asString());
    }

  LOG4CXX_INFO(_logZdaqex,__PRETTY_FUNCTION__<<" Setting parameters for processors and merger ");
  _merger->loadParameters(jc);
  // Overwrite msg
  //Prepare complex answer
  Json::Value prep;
  prep["sourceRegistered"]=array_keys;
  prep["processorRegistered"]=parray_keys;
       
  m->setAnswer(prep);
  LOG4CXX_DEBUG(_logZdaqex,__PRETTY_FUNCTION__<<"end of configure");
  return;
}

void zdaq::exBuilder::start(zdaq::fsmmessage* m)
{

  Json::Value jc=m->content();
  _merger->start(jc["run"].asInt());
  _running=true;

  LOG4CXX_INFO(_logZdaqex,__PRETTY_FUNCTION__<<"Run "<<jc["run"].asInt()<<" is started ");
}
void zdaq::exBuilder::stop(zdaq::fsmmessage* m)
{
  
  _merger->stop();
  _running=false;
  LOG4CXX_INFO(_logZdaqex,__PRETTY_FUNCTION__<<"Builder is stopped \n");fflush(stdout);
}
void zdaq::exBuilder::halt(zdaq::fsmmessage* m)
{
  
  
  LOG4CXX_INFO(_logZdaqex,__PRETTY_FUNCTION__<<"Received "<<m->command());
  if (_running)
    this->stop(m);
  std::cout<<"Destroying"<<std::endl;
  //stop data sources
  _merger->clear();
}
void zdaq::exBuilder::status(Mongoose::Request &request, Mongoose::JsonResponse &response)
{
  //std::cout<<"dowmnload"<<request.getUrl()<<" "<<request.getMethod()<<" "<<request.getData()<<std::endl;
  if (_merger!=NULL)
    {

      response["answer"]=_merger->status();

    }
  else
    response["answer"]="NO merger created yet";
}
void zdaq::exBuilder::registerds(Mongoose::Request &request, Mongoose::JsonResponse &response)
{
  //std::cout<<"registerds"<<request.getUrl()<<" "<<request.getMethod()<<" "<<request.getData()<<std::endl;
  if (_merger!=NULL)
    {
      _merger->setNumberOfDataSource(atoi(request.get("dsnumber","0").c_str()));
      response["answer"]=atoi(request.get("dsnumber","0").c_str());

    }
  else
    response["answer"]="NO merger created yet";
}
void zdaq::exBuilder::c_setheader(Mongoose::Request &request, Mongoose::JsonResponse &response)
{

  if (_merger==NULL)    {response["STATUS"]="NO EVB created"; return;}
  std::string shead=request.get("header","None");
  if (shead.compare("None")==0)
    {response["STATUS"]="NO header provided "; return;}
  Json::Reader reader;
  Json::Value jsta;
  bool parsingSuccessful = reader.parse(shead,jsta);
  if (!parsingSuccessful)
    {response["STATUS"]="Cannot parse header tag "; return;}
  const Json::Value& jdevs=jsta;
  LOG4CXX_DEBUG(_logZdaqex,__PRETTY_FUNCTION__<<"Header "<<jdevs);
  std::vector<uint32_t>& v=_merger->runHeader();
  v.clear();
  for (Json::ValueConstIterator jt = jdevs.begin(); jt != jdevs.end(); ++jt)
    v.push_back((*jt).asInt());

  //std::cout<<jdevs<<std::endl;
  //  std::cout<<" LOL "<<std::endl;
  _merger->processRunHeader();
  //  std::cout<<" LOL AGAIN "<<std::endl;
  response["STATUS"]="DONE";
  response["VALUE"]=jsta;

}



