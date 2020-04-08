#!/usr/bin/env python
import os
import socks
import socket
import httplib
import urllib
import urllib2
from urllib2 import URLError, HTTPError
import json
from copy import deepcopy
import base64
import time
import argparse
import requests
import runcontrol as dqc
import sys

parser = argparse.ArgumentParser()

# configure all the actions
grp_action = parser.add_mutually_exclusive_group()
# JOB Control
grp_action.add_argument('--available', action='store_true',
                        help='Obsolete: Use --jc-status instead')
grp_action.add_argument('--app-info', action='store_true',
                        help='Obsolete:  Use --jc-status instead')
grp_action.add_argument('--webstatus', action='store_true',
                        help='Obsolete:  Use --jc-status instead')

grp_action.add_argument('--jc-create', action='store_true',
                        help='Loads configuration in all jobcontrol process ')
grp_action.add_argument('--jc-kill', action='store_true',
                        help='kill all running processes')
grp_action.add_argument('--jc-destroy', action='store_true',
                        help='delete all jobcontrol configuration')
grp_action.add_argument('--jc-start', action='store_true',
                        help='start all controled processes described in $DAQCONFIG jsonfile variable')
grp_action.add_argument('--jc-restart', action='store_true',
                        help='restart one job with --jobname=name --jobpid=pid --host=hostname')
grp_action.add_argument('--jc-status', action='store_true',
                        help='show the status all controled processes or of the process specified in --name=PROC')
grp_action.add_argument('--jc-info', action='store_true',
                        help='show the status all controled processesof the host specified in --host=Host')
grp_action.add_argument('--jc-appcreate', action='store_true',
                        help='Create all ZDAQ app on all hosts')

#DAQ preparation
grp_action.add_argument('--daq-create', action='store_true',
                        help='Send the CREATE transition to the RunControl process (i.e parameters loads of all controlled application) ')
grp_action.add_argument('--daq-discover', action='store_true',
                        help='RunControl scans controlled application and forward CREATE transition to all controlled processes')
grp_action.add_argument('--daq-setparameters', action='store_true',
                        help='send the parameters described in $DAQCONFIG file to the DAQ')
grp_action.add_argument('--daq-getparameters', action='store_true',
                        help='get the parameters described in $DAQCONFIG file to the DAQ')
grp_action.add_argument('--daq-forceState', action='store_true',
                        help='force the sate name of the RunControl with the --state option, ex --forceState --state=DISCOVERED')
grp_action.add_argument('--daq-services', action='store_true',
                        help='Send INITIALISE/CONFIGURE transition to services application (MDCC,DB,BUILDER mainly)')
grp_action.add_argument('--daq-state', action='store_true',
                        help=' display DAQ state')
# Running
grp_action.add_argument('--daq-initialise', action='store_true',
                        help='Initialise RunControl, data source discovery and Builder final configuration')
grp_action.add_argument('--daq-configure', action='store_true',
                        help=' Configure RunControl, front end ASICs configuration')
grp_action.add_argument('--daq-startrun', action='store_true',
                        help=' start the run')
grp_action.add_argument('--daq-stoprun', action='store_true',
                        help=' stop the run')
grp_action.add_argument('--daq-destroy', action='store_true',
                        help='destroy the readout, back to the PREPARED state')
# Status
grp_action.add_argument('--daq-dsstatus', action='store_true',
                        help=' display DAQ status of all SDHCAL DIF')
grp_action.add_argument('--daq-evbstatus', action='store_true',
                        help=' display event builder status')
# TRIGGER MDCC
grp_action.add_argument('--trig-status', action='store_true',
                        help=' display trigger counter status')

grp_action.add_argument('--trig-period', action='store_true',
                        help=' Set the trigger period in micro s')

grp_action.add_argument('--trig-size', action='store_true',
                        help=' Set the Event size in int32')

# Arguments
parser.add_argument('--config', action='store', dest='config',
                    default=None, help='json config file')
parser.add_argument('--socks', action='store', type=int,
                    dest='sockport', default=None, help='use SOCKS port ')
parser.add_argument('--version', action='version', version='%(prog)s 1.0')
parser.add_argument('--state', action='store', type=str,
                    default=None, dest='fstate', help='set the Daq state')
# Job
parser.add_argument('--lines', action='store', type=int, default=None,
                    dest='lines', help='set the number of lines to be dump')
parser.add_argument('--host', action='store', dest='host',
                    default=None, help='host for log')
parser.add_argument('--name', action='store', dest='name',
                    default=None, help='application name')
parser.add_argument('--jobname', action='store',
                    dest='jobname', default=None, help='job name')
parser.add_argument('--jobpid', action='store', type=int,
                    dest='jobpid', default=None, help='job pid')
parser.add_argument('--value', action='store', type=int,
                    dest='value', default=None, help='value to pass')
parser.add_argument('-v', '--verbose', action='store_true',
                    default=False, help='Raw Json output')
parser.add_argument('--comment', action='store', default=None,
                    dest='comment', help=' Comment for start run')

if len(sys.argv)==1:
    parser.print_help(sys.stderr)
    sys.exit(1)
results = parser.parse_args()


# Analyse results
# set the connection mode
if (results.sockport == None):
    sp = os.getenv("SOCKPORT", "Not Found")
    if (sp != "Not Found"):
        results.sockport = int(sp)


if (results.sockport != None):
    socks.setdefaultproxy(socks.PROXY_TYPE_SOCKS5,
                          "127.0.0.1", results.sockport)
    socket.socket = socks.socksocket
    #print "on utilise sock",results.sockport


# fill parameters

fdc = dqc.RCClient()
fdc.loadConfig()

# analyse the command
lcgi = {}
r_cmd = None
if (results.daq_create):
    r_cmd = 'createDaq'
    fdc.daq_create()
    exit(0)
elif(results.available):
    r_cmd = 'available'
    fdc.daq_list()
    exit(0)
elif(results.app_info):
    r_cmd = 'available'
    if (results.name != None):
        fdc.daq_info(results.name)
    else:
        print 'Please specify the name --name=name'
    exit(0)
## JOBCONTROL
elif(results.jc_create):
    r_cmd = 'createJobControl'
    fdc.jc_create()
    exit(0)
elif(results.jc_kill):
    r_cmd = 'jobKillAll'
    fdc.jc_kill()
    exit(0)
elif(results.jc_start):
    r_cmd = 'jobStartAll'
    fdc.jc_start()
    exit(0)
elif(results.jc_destroy):
    r_cmd = 'jobDestroy'
    fdc.jc_destroy()
    exit(0)
elif(results.jc_restart):
    lcgi.clear()
    if (results.host == None):
        print "set the host "
        exit(0)
    if (results.jobname == None):
        print "set the jobname "
        exit(0)
    if (results.jobpid == None):
        print "set the jobpid "
        exit(0)
    fdc.jc_restart(results.host, results.jobname, results.jobpid)
    r_cmd = 'jobReStart'
    exit(0)
elif(results.jc_appcreate):
    sr = fdc.jc_appcreate()
    print sr
    exit(0)
elif(results.jc_status):
    if (results.name != None):
        sr = fdc.jc_status(results.name)
        exit(0)
    else:
        sr = fdc.jc_status()
    exit(0)
elif(results.jc_info):
    if (results.host != None):
        if (results.name != None):
            sr = fdc.jc_info(results.host,results.name)
            exit(0)
        else:
            sr = fdc.jc_info(results.host)

        exit(0)
    else:
        print "Host name missing"
    exit(0)

### DAQ

elif(results.daq_state):
    r_cmd = 'state'
    fdc.daq_state()
    exit(0)
elif(results.daq_discover):
    r_cmd = 'Discover'
    fdc.daq_discover()
    exit(0)
elif(results.daq_setparameters):
    r_cmd = 'setParameters'
    fdc.daq_setparameters()
    exit(0)
elif(results.daq_getparameters):
    r_cmd = 'getParameters'
    fdc.daq_getparameters()
    exit(0)
elif(results.daq_forceState):
    r_cmd = 'forceState'
    if (results.fstate != None):
        fdc.daq_forceState(results.fstate)
    else:
        print 'Please specify the state --state=STATE'
    exit(0)
elif(results.daq_initialise):
    r_cmd = 'initialise'
    fdc.daq_initialise()
    exit(0)
elif(results.daq_configure):
    r_cmd = 'configure'
    fdc.daq_configure()
    exit(0)
elif(results.daq_dsstatus):
    r_cmd = 'status'
    sr = fdc.daq_ds_status()
    if (results.verbose):
        print sr
    else:
        dqc.parseReturn(r_cmd, sr)
    exit(0)
elif(results.daq_startrun):
    r_cmd = 'start'
    if (results.comment != None):
        fdc.daq_start(results.comment)
    else:
        fdc.daq_start()
    exit(0)

elif(results.daq_stoprun):
    r_cmd = 'stop'

    fdc.daq_stop()
    exit(0)
elif(results.daq_destroy):
    r_cmd = 'destroy'
    fdc.daq_destroy()
    exit(0)
elif(results.daq_evbstatus):
    r_cmd = 'shmStatus'
    sr = fdc.daq_evb_status()
    if (results.verbose):
        print sr
    else:
        dqc.parseReturn(r_cmd, sr)
    exit(0)
### Trigger

elif(results.trig_status):
    r_cmd = 'triggerStatus'
    res=fdc.trg_status()
    print res
    exit(0)

elif(results.trig_size):
    r_cmd = 'triggerSize'
    if (results.value != None):
        fdc.trg_size(results.value)
    else:
        print 'Please specify the size --value=xx'
    exit(0)

elif(results.trig_period):
    r_cmd = 'triggerPeriod'
    if (results.value != None):
        fdc.trg_period(results.clock)
    else:
        print 'Please specify the period --value=xx'
    exit(0)

