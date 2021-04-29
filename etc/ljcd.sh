#!/bin/bash


mkdir -p /dev/shm/monitor/closed


export LD_LIBRARY_PATH=/usr/lib:/usr/local/lib:/usr/local/zdaq/lib:$LD_LIBRARY_PATH
export PATH=/usr/local/zdaq/bin:$PATH
export PYTHONPATH=/usr/local/zdaq/share:$PYTHONPATH
source /etc/difdim.cfg
/usr/local/zdaq/bin/ljc > /var/log/ljc.log 2>&1
