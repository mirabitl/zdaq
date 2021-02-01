#include "localCollector.hh"

using namespace zdaq;
using namespace zdaq::builder;

zdaq::builder::collector::collector(std::string name) : zdaq::baseApplication(name), _running(false), _merger(NULL)
{
  // Create the context and the merger
  _context = new zmq::context_t();
  _merger = new zdaq::zmMerger(_context);

  // Register state
  this->fsm()->addState("CREATED");
  this->fsm()->addState("CONFIGURED");
  this->fsm()->addState("RUNNING");

  // Register transitions
  this->fsm()->addTransition("CONFIGURE", "CREATED", "CONFIGURED", boost::bind(&zdaq::builder::collector::configure, this, _1));
  this->fsm()->addTransition("CONFIGURE", "CONFIGURED", "CONFIGURED", boost::bind(&zdaq::builder::collector::configure, this, _1));
  this->fsm()->addTransition("START", "CONFIGURED", "RUNNING", boost::bind(&zdaq::builder::collector::start, this, _1));
  this->fsm()->addTransition("STOP", "RUNNING", "CONFIGURED", boost::bind(&zdaq::builder::collector::stop, this, _1));
  this->fsm()->addTransition("HALT", "RUNNING", "CREATED", boost::bind(&zdaq::builder::collector::halt, this, _1));
  this->fsm()->addTransition("HALT", "CONFIGURED", "CREATED", boost::bind(&zdaq::builder::collector::halt, this, _1));

  // Standalone command
  this->fsm()->addCommand("STATUS", boost::bind(&zdaq::builder::collector::status, this, _1, _2));
  this->fsm()->addCommand("SETHEADER", boost::bind(&zdaq::builder::collector::c_setheader, this, _1, _2));
  this->fsm()->addCommand("PURGE", boost::bind(&zdaq::builder::collector::c_purge, this, _1, _2));

  //Start server
  char *wp = getenv("WEBPORT");
  if (wp != NULL)
  {
    LOG4CXX_INFO(_logZdaqex, __PRETTY_FUNCTION__ << "Service " << name << " started on port " << atoi(wp));
    this->fsm()->start(atoi(wp));
  }
}

void zdaq::builder::collector::configure(zdaq::fsmmessage *m)
{
  LOG4CXX_INFO(_logZdaqex, __PRETTY_FUNCTION__ << "Received " << m->command() << " Value " << m->value());
  // Store message content in paramters

  if (m->content().isMember("collectingPort"))
  {
    this->parameters()["collectingPort"] = m->content()["collectingPort"];
  }
  if (m->content().isMember("processor"))
  {
    this->parameters()["processor"] = m->content()["processor"];
  }
  // Check that needed parameters exists

  if (!this->parameters().isMember("collectingPort"))
  {
    LOG4CXX_ERROR(_logZdaqex, "Missing collectingPort,no data stream");
    return;
  }
  if (!this->parameters().isMember("processor"))
  {
    LOG4CXX_ERROR(_logZdaqex, "Missing processor, list of processing pluggins");
    return;
  }

  // register data source and processors
  Json::Value jc = this->parameters();
  if (jc.isMember("purge"))
    _merger->setPurge(jc["purge"].asInt() != 0);

  // Register the data source
  std::stringstream st("");
  st<<"tcp://*:"<<jc["collectingPort"].asUInt();
  LOG4CXX_INFO(_logZdaqex, "Registering " << st.str());
  _merger->registerDataSource(st.str());
  array_keys.append(st.str());

  // Register the processors
  const Json::Value &pbooks = jc["processor"];
  Json::Value parray_keys;
  for (Json::ValueConstIterator it = pbooks.begin(); it != pbooks.end(); ++it)
  {
    const Json::Value &book = *it;
    LOG4CXX_INFO(_logZdaqex, "registering " << (*it).asString());
    _merger->registerProcessor((*it).asString());
    parray_keys.append((*it).asString());
  }

  LOG4CXX_INFO(_logZdaqex, " Setting parameters for processors and merger ");
  _merger->loadParameters(jc);

  
  // Overwrite msg
  //Prepare complex answer
  Json::Value prep;
  prep["sourceRegistered"] = array_keys;
  prep["processorRegistered"] = parray_keys;

  m->setAnswer(prep);
  LOG4CXX_DEBUG(_logZdaqex, "end of configure");
  return;
}

void zdaq::builder::collector::start(zdaq::fsmmessage *m)
{
  LOG4CXX_DEBUG(_logZdaqex, "Received " << m->command() << " Value " << m->value());
  Json::Value jc = m->content();
  _merger->start(jc["run"].asInt());
  _running = true;

  LOG4CXX_INFO(_logZdaqex, "Builder Run " << jc["run"].asInt() << " is started ");
}
void zdaq::builder::collector::stop(zdaq::fsmmessage *m)
{
  LOG4CXX_DEBUG(_logZdaqex, "Received " << m->command() << " Value " << m->value());
  _merger->stop();
  _running = false;
  LOG4CXX_INFO(_logZdaqex, "Builder is stopped \n");
  fflush(stdout);
}
void zdaq::builder::collector::halt(zdaq::fsmmessage *m)
{

  LOG4CXX_DEBUG(_logZdaqex, "Received " << m->command());
  if (_running)
    this->stop(m);

  LOG4CXX_INFO(_logZdaqex, "Destroying Builder Sources");
  //stop data sources
  _merger->clear();
}
void zdaq::builder::collector::status(Mongoose::Request &request, Mongoose::JsonResponse &response)
{

  if (_merger != NULL)
  {

    response["answer"] = _merger->status();
  }
  else
    response["answer"] = "NO merger created yet";
}

void zdaq::builder::collector::c_purge(Mongoose::Request &request, Mongoose::JsonResponse &response)
{
  if (_merger != NULL)
  {
    LOG4CXX_INFO(_logZdaqex, "Setting Purge flag to "<<request.get("active", "0"));

    _merger->setPurge(atoi(request.get("active", "0").c_str()) != 0);
    response["answer"] = atoi(request.get("active", "0").c_str());
  }
  else
    response["answer"] = "NO merger created yet";
}
void zdaq::builder::collector::c_setheader(Mongoose::Request &request, Mongoose::JsonResponse &response)
{

  if (_merger == NULL)
  {
    response["STATUS"] = "NO EVB created";
    return;
  }
  int32_t nextevent = atoi(request.get("nextevent", "-1").c_str());
  std::string shead = request.get("header", "None");
  if (shead.compare("None") == 0)
  {
    response["STATUS"] = "NO header provided ";
    return;
  }
  Json::Reader reader;
  Json::Value jsta;
  bool parsingSuccessful = reader.parse(shead, jsta);
  if (!parsingSuccessful)
  {
    response["STATUS"] = "Cannot parse header tag ";
    return;
  }
  const Json::Value &jdevs = jsta;
  LOG4CXX_DEBUG(_logZdaqex, "Header " << jdevs);
  std::vector<uint32_t> &v = _merger->runHeader();
  v.clear();
  for (Json::ValueConstIterator jt = jdevs.begin(); jt != jdevs.end(); ++jt)
    v.push_back((*jt).asInt());


  if (nextevent != -1)
    _merger->setRunHeaderEvent(nextevent);
  _merger->processRunHeader();

  response["STATUS"] = "DONE";
  response["VALUE"] = jsta;
}

