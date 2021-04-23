#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <iostream>
#include <sstream>
namespace zdaq
{
  template <typename T>
  class pluginUtil
  {
  public:
    pluginUtil(std::string libname, std::string create_name, std::string destroy_name) : _libname(libname),_handler(NULL)
    {
      std::stringstream s;
      s << "lib" << libname << ".so";
      _handler = dlopen(s.str().c_str(), RTLD_NOW);
      if (_handler == NULL)
	{
	  std::cerr << " Error " << dlerror() << " Library open address " << std::hex << _handler << std::dec << std::endl;
	  return;
	}

      std::cout << "Library" << s.str() << " is  opened at address " << std::hex << _handler << std::dec << std::endl;
      // Get the loadFilter function, for loading objects
      _maker = dlsym(_handler, create_name.c_str());
      if (_maker == NULL)
	{
	  std::cerr << " Error " << dlerror() << " Cannot find" << create_name << " method " << std::endl;
	  return;
	}
      std::cout << create_name << " method at  address " << std::hex << _maker << std::dec << std::endl;

      _destroyer = dlsym(_handler, destroy_name.c_str());
      if (_destroyer == NULL)
	{
	  std::cerr << " Error " << dlerror() << " Cannot find" << destroy_name << " method " << std::endl;
	  return;
	}
      std::cout << destroy_name << " method at  address " << std::hex << _destroyer << std::dec << std::endl;

      // Get a new zmonStore object
      T *(*create)() = (T * (*)()) _maker;
      _object = (T *)create();
    }
    void clear(){_handler=NULL;_maker=NULL;_destroyer=NULL;}
    T *ptr() const { return _object; }
    std::string name() { return _libname; }
    void close()
    {
      void *(*destroy)(T *) = (void *(*)(T *))_destroyer;
      std::cout << "Destroying the plugin" << std::endl;
      destroy(_object);
      std::cout << "Closing the library" << std::endl;
      dlclose(_handler);
      clear();
    }
    bool isAlived() const {return _handler!=NULL;}
  private:
    std::string _libname;
    void *_handler;
    void *_maker;
    void *_destroyer;
    T *_object;
  };
};
