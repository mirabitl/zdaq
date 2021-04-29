#!/bin/bash



export LD_LIBRARY_PATH=/usr/lib:/usr/local/lib:/usr/local/zdaq/lib:/usr/local/lydaq/lib:$LD_LIBRARY_PATH
export PATH=/usr/local/zdaq/bin:$PATH
export PYTHONPATH=/usr/local/zdaq/share:$PYTHONPATH
source /etc/difdim.cfg
/usr/local/zdaq/bin/lscm > /var/log/lsc.log 2>&1
