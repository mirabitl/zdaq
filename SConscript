import sys,os,commands
import  os
import re
import sys

def SWIGSharedLibrary(env, library, sources, **args):
  swigre = re.compile('(.*).i')
  if env.WhereIs('swig') is None:
    sourcesbis = []
    for source in sources:
      cName = swigre.sub(r'\1_wrap.c', source)
      cppName = swigre.sub(r'\1_wrap.cc', source)
      if os.path.exists(cName):
        sourcesbis.append(cName)
      elif os.path.exists(cppName):
        sourcesbis.append(cppName)
      else:
        sourcesbis.append(source)
  else:
    sourcesbis = sources
 
  if 'SWIGFLAGS' in args:
    args['SWIGFLAGS'] += ['-python']
  else:
    args['SWIGFLAGS'] = ['-python'] + env['SWIGFLAGS']
  args['SHLIBPREFIX']=""
  if sys.version >= '2.5':
    args['SHLIBSUFFIX']=".pyd"
 
  cat=env.SharedLibrary(library, sourcesbis, **args)
  return cat
 


# environment
#import xdaq
#print xdaq.INCLUDES
#print xdaq.LIBRARY_PATHS
#print xdaq.LIBRARIES

#print "----------------------------------------------"
Decider('MD5-timestamp')
ZDAQ_ROOT="/opt/zdaq"

fres=os.popen('uname -r')
kl=fres.readline().split(".")

platform="UBUNTU"
if (kl[len(kl)-1][0:3] == 'el5'):
    platform="SLC5"

if (kl[len(kl)-2][0:3] == 'el6'):
    platform="SLC6"
print kl[len(kl)-2][0:3]
fres=os.popen('uname -p')
kp=fres.readline()
osv=kp[0:len(kp)-1]

print platform,osv

Bit64=False
Bit64=os.uname()[4]=='x86_64'

kl=os.uname()[2].split(".")
platform="UBUNTU"
if (kl[len(kl)-1][0:3] == 'el5'):
    platform="SLC5"

if (kl[len(kl)-2][0:3] == 'el6'):
    platform="SLC6"

Arm=os.uname()[4]=='armv7l'

if Arm or platform=="UBUNTU":
  boostsystem='boost_system'
  boostthread='boost_thread'
else:
  boostsystem='boost_system-mt'
  boostthread='boost_thread-mt'

#Library ROOT + some of XDAQ + DB 
ROOT_LIBS=[lib[2:] for lib in filter(lambda x: (x[:2]=="-l"), commands.getoutput("$ROOTSYS/bin/root-config --libs --ldflags --glibs").split(" "))
]
ROOT_LIBS.append('XMLIO')
ROOT_LIBPATH=[lib[2:] for lib in filter(lambda x: (x[:2]=="-L"), commands.getoutput("$ROOTSYS/bin/root-config --libs ").split(" "))]


  

Use_Mongoose=os.path.exists("/usr/local/include/mongoose")

# includes
INCLUDES=['include',"/usr/include/boost141/","/usr/include/jsoncpp"]

INCLUDES.append(commands.getoutput("python -c 'import distutils.sysconfig as conf; print conf.get_python_inc()'"))

  
CPPFLAGS=["-pthread","-g","-O2","-DLINUX", "-DREENTRANT" ,"-Dlinux", "-DLITTLE_ENDIAN__ ", "-Dx86",  "-DXERCES=2", "-DDAQ_VERSION_2","-std=c++11"]
LIBRARIES=['pthread', 'm', 'z','stdc++',boostsystem,boostthread,'curl','zmq','jsoncpp','log4cxx']


#Library path XDAQ,DHCAL and ROOT + Python
if (Bit64):
  LIBRARY_PATHS=["/usr/lib64","/usr/local/lib","/opt/dhcal/levbdim/lib"]
else:
  LIBRARY_PATHS=["/usr/lib","/usr/local/lib","/opt/dhcal/levbdim/lib"]
LIBRARY_PATHS.append(commands.getoutput("python -c 'import distutils.sysconfig as conf; print conf.PREFIX'")+"/lib")

if (Use_Mongoose):
  LIBRARY_PATHS.append('/usr/local/lib')
  LIBRARIES.append('mongoose')
  INCLUDES.append('/usr/local/include/')
  INCLUDES.append('/usr/local/include/mongoose')
 
  
#link flags
LDFLAGS=["-fPIC","-dynamiclib"]

# SWIG
SWIGSF=["-c++","-classic"]

for i in INCLUDES:
    SWIGSF.append("-I"+i)
print SWIGSF

# Create the Environment
env = Environment(CPPPATH=INCLUDES,CPPFLAGS=CPPFLAGS,LINKFLAGS=LDFLAGS, LIBS=LIBRARIES,LIBPATH=LIBRARY_PATHS,SWIGFLAGS=SWIGSF)

#print "CC is:",env.subst('$CPPPATH')

env['BUILDERS']['PythonModule'] = SWIGSharedLibrary


# Library source
LIBRARY_SOURCES=Glob("#src/*.cc")

#print LIBRARY_SOURCES
#Shared library
lzdaq=env.SharedLibrary("#lib/zdaq",LIBRARY_SOURCES)



#Daemon 
EXE_LIBPATH=LIBRARY_PATHS
EXE_LIBPATH.append("#lib")
EXE_LIBS=LIBRARIES
EXE_LIBS.append("zdaq")

plugbase=env.SharedLibrary("lib/binarywriter",source="src/pluggins/binarywriter.cc",LIBPATH=EXE_LIBPATH,LIBS=EXE_LIBS)

dumbase=env.SharedLibrary("lib/dummywriter",source="src/pluggins/dummywriter.cc",LIBPATH=EXE_LIBPATH,LIBS=EXE_LIBS)

plist=[]
mpub=env.Program("bin/mpubserver",source="src/mpubserver.cpp",LIBPATH=EXE_LIBPATH,LIBS=EXE_LIBS)
plist.append(mpub)
mcoll=env.Program("bin/mpubcollector",source="src/mpubcollector.cpp",LIBPATH=EXE_LIBPATH,LIBS=EXE_LIBS)
plist.append(mcoll)
mbuil=env.Program("bin/mpubbuilder",source="src/mpubbuilder.cpp",LIBPATH=EXE_LIBPATH,LIBS=EXE_LIBS)
plist.append(mbuil)
mtestfsm=env.Program("bin/testfsm",source="src/utils/testfsm.cpp",LIBPATH=EXE_LIBPATH,LIBS=EXE_LIBS)
plist.append(mtestfsm)
mtestb=env.Program("bin/testbuilder",source="src/utils/testbuilder.cpp",LIBPATH=EXE_LIBPATH,LIBS=EXE_LIBS)
plist.append(mtestb)

pljc=env.Program("bin/ljc",source="src/utils/ljc.cxx",LIBPATH=EXE_LIBPATH,LIBS=EXE_LIBS)
plist.append(pljc)

myinc=[]
for x in Glob("#include/*.hh"):
  myinc.append("include/"+x.name)
print plist
#env.Install(DHCAL_ROOT+"/opt/dhcal/lib",levbdimdaq)
#env.Install(DHCAL_ROOT+"/opt/dhcal/bin",plist)
###env.Install("/opt/dhcal/lib",levbdimdaq)
###env.Install("/opt/dhcal/include/readout",myinc)

#env.Alias('install', [DHCAL_ROOT+"/opt/dhcal/lib",DHCAL_ROOT+"/opt/dhcal/bin"])



