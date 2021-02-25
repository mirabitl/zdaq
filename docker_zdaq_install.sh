#!/bin/bash
apt update
apt install -y  g++ scons git  liblog4cxx-dev libboost-dev libboost-system-dev libboost-filesystem-dev  libboost-thread-dev libjsoncpp-dev libzmq3-dev cmake make  libmongoc-dev libcurl4-gnutls-dev curl nano python3 python3-pip
pip3 install requests pysocks pymongo transitions pyserial json2html
cd opt/
git clone http://github.com/mirabitl/zdaq
export ZDAQROOT=/opt/zdaq
# INSTALL MONGOOSE-CPP
cd /tmp/
tar zxvf ${ZDAQROOT}/extras/mongoose.tgz 
cd mongoose-cpp/
mkdir build/
cd build/
rm -rf *
cp  ${ZDAQROOT}/extras/CMakeLists.txt  ../
cmake -DEXAMPLES=ON -DWEBSOCKET=OFF  -DHAS_JSONCPP=ON ..
make -j4
make install
# Extras include ZMQ
cd /tmp
tar zxvf ${ZDAQROOT}/extras/zguide.tar.gz
cp zguide/examples/C++/zhelpers.hpp /usr/local/include
cd /tmp
tar zxvf ${ZDAQROOT}/extras/cppzmq.tar.gz
cp cppzmq/*.hpp /usr/local/include
rm -rf /tmp/*
cd $ZDAQROOT/
scons -j4


mkdir -p /usr/local/share/zdaq
mkdir -p /usr/local/include/zdaq

find /opt/zdaq -name '*.py' -exec ln -sf {} /usr/local/share/zdaq/ \;
ln -sf /opt/zdaq/scripts/mgjob /usr/local/bin/

find /opt/zdaq -name '*.h*' -exec ln -sf {} /usr/local/include/zdaq/ \;
find /opt/zdaq -name 'lib*.so' -exec ln -sf {} /usr/local/lib/ \;


find /opt/zdaq/bin/ -wholename '*/bin/*' -type f -executable  -exec ln -sf {} /usr/local/bin \;
find /opt/zdaq/monitoring/ -wholename '*/bin/*' -type f -executable  -exec ln -sf {} /usr/local/bin \;

find /opt/zdaq -name '*.os' -exec rm {} \;
find /opt/zdaq -name '*.o' -exec rm {} \;



