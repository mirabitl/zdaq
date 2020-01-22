#include "fsmjob.hh"
#include <unistd.h>
using namespace zdaq;
using namespace zdaq::jc;
int main()
{
  char hostn[80];
  gethostname(hostn,80);
  std::stringstream s;
  s<<"LJC-"<<hostn;
  fsmjob fs(s.str(),9999);
  while (1)
    {
      ::sleep(1);
    }
}
