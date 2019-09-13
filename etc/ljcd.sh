#!/bin/bash


mkdir -p /dev/shm/monitor/closed


export LD_LIBRARY_PATH=/usr/lib:/usr/local/lib:/opt/zdaq/lib:$LD_LIBRARY_PATH

source /etc/difdim.cfg
/opt/dhcal/bin/ljc > /var/log/ljc.log 2>&1
