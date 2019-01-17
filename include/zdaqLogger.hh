#ifndef _zdaq_Logger_
#define _zdaq_Logger_
#include <log4cxx/logger.h>
#include <log4cxx/xml/domconfigurator.h>
using namespace log4cxx;
using namespace log4cxx::xml;
using namespace log4cxx::helpers;
using namespace std;
static LoggerPtr _logZdaq(Logger::getLogger("ZDAQ"));
static LoggerPtr _logZdaqex(Logger::getLogger("ZDAQEXAMPLE"));

#endif
