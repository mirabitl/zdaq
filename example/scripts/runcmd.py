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
import daqcontrol as dqc
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
                        help='Send the CREATE transition to the FDAQ process (i.e parameters loads of all controlled application) ')
grp_action.add_argument('--daq-discover', action='store_true',
                        help='FDAQ scans controlled application and forward CREATE transition to all controlled processes')
grp_action.add_argument('--daq-setparameters', action='store_true',
                        help='send the parameters described in $DAQCONFIG file to the DAQ')
grp_action.add_argument('--daq-getparameters', action='store_true',
                        help='get the parameters described in $DAQCONFIG file to the DAQ')
grp_action.add_argument('--daq-forceState', action='store_true',
                        help='force the sate name of the FDAQ with the --state option, ex --forceState --state=DISCOVERED')
grp_action.add_argument('--daq-services', action='store_true',
                        help='Send INITIALISE/CONFIGURE transition to services application (MDCC,DB,BUILDER mainly)')
grp_action.add_argument('--daq-state', action='store_true',
                        help=' display DAQ state')
# Running
grp_action.add_argument('--daq-initialise', action='store_true',
                        help='Initialise FDAQ, data source discovery and Builder final configuration')
grp_action.add_argument('--daq-configure', action='store_true',
                        help=' Configure FDAQ, front end ASICs configuration')
grp_action.add_argument('--daq-startrun', action='store_true',
                        help=' start the run')
grp_action.add_argument('--daq-stoprun', action='store_true',
                        help=' stop the run')
grp_action.add_argument('--daq-destroy', action='store_true',
                        help='destroy the readout, back to the PREPARED state')
# Status
grp_action.add_argument('--daq-difstatus', action='store_true',
                        help=' display DAQ status of all SDHCAL DIF')
grp_action.add_argument('--daq-tdcstatus', action='store_true',
                        help=' display DAQ status of all FEBCMS TDC')
grp_action.add_argument('--daq-evbstatus', action='store_true',
                        help=' display event builder status')
grp_action.add_argument('--daq-dbstatus', action='store_true',
                        help='get current run and state from db')
# Actions
grp_action.add_argument('--daq-downloaddb', action='store_true',
                        help='download the dbsate specified in --dbstate=state')

#DIF  Calibration
grp_action.add_argument('--dif-setgain', action='store_true',
                        help='change the gain of HARDROC2 and reconfigure chips with --gain=xxx')
grp_action.add_argument('--dif-setthreshold', action='store_true',
                        help='change the threholds of HARDROC2 and reconfigure chips with --B0=xxx --B1=yyy --B2=zzz')
grp_action.add_argument('--dif-ctrlreg', action='store_true',
                        help='set the SDHCAL DIF ctrlregister specified with --ctrlreg=register')

#FEBV1 actions
grp_action.add_argument('--feb-resettdc', action='store_true',
                        help=' Send Reset to all FEB TDC')
grp_action.add_argument('--feb-calibdac', action='store_true',
                        help='Start a PETIROC calibdac run')
grp_action.add_argument('--feb-scurve', action='store_true',
                        help='Start a PETIROC scurve run')
grp_action.add_argument('--feb-setvth', action='store_true',
                        help='change the VTHTIME threshold of PETIROC2 and reconfigure chips with -vth=xxx')
grp_action.add_argument('--feb-setmask', action='store_true',
                        help='change the MASK of PETIROC2reconfigure chips with -mask=xxx')
grp_action.add_argument('--feb-settdcmode', action='store_true',
                        help='change the trigger mode before starting the run with --value=1/0')
grp_action.add_argument('--feb-settdcdelays', action='store_true',
                        help='change the PETIROC mode before starting the run with --active=1-255, --dead=1-255 ')
grp_action.add_argument('--feb-lutcalib', action='store_true',
                        help='Calibrate --tdc=i --channel=k ')
grp_action.add_argument('--feb-lutdraw', action='store_true',
                        help='Calibrate --tdc=i --channel=k ')
grp_action.add_argument('--histo-draw', action='store_true',
                        help='Draw histo --name=histo ')


# TRIGGER MDCC
grp_action.add_argument('--trig-status', action='store_true',
                        help=' display trigger (MDCC) counter status')
grp_action.add_argument('--trig-reset', action='store_true',
                        help=' reset trigger (MDCC) counter')
grp_action.add_argument('--trig-pause', action='store_true',
                        help=' trigger (MDCC) soft veto')
grp_action.add_argument('--trig-resume', action='store_true',
                        help=' release trigger (MDCC) soft veto ')
grp_action.add_argument('--ecal-pause', action='store_true',
                        help=' (Obsolete) Ecal soft veto')
grp_action.add_argument('--ecal-resume', action='store_true',
                        help=' (Obsolete) release Ecal soft veto')
grp_action.add_argument('--trig-spillon', action='store_true',
                        help=' set spill nclock on with --clock=nc (20ns)')
grp_action.add_argument('--trig-spilloff', action='store_true',
                        help=' set spill nclock off with --clock=nc (20ns) ')
grp_action.add_argument('--trig-beam', action='store_true',
                        help=' set beam length to nclock with --clock=nc (20ns) ')
grp_action.add_argument('--trig-spillregister', action='store_true',
                        help=' set the value of the spill register (FEB CMS) (32/64 for beamtest or cosmic) --value=xx ')
grp_action.add_argument('--trig-calibon', action='store_true',
                        help=' set the value of the calibon register (FEB calibration) --value=xx ')
grp_action.add_argument('--trig-calibcount', action='store_true',
                        help=' set the value of the calibcount (FEB calibration) register --value=xx ')
grp_action.add_argument('--trig-rearm', action='store_true',
                        help=' reload calib (FEB calibration)  ')
grp_action.add_argument('--trig-hardreset', action='store_true',
                        help=' send a hard reset to mezzanines ')
grp_action.add_argument('--trig-setregister', action='store_true',
                        help=' set the value of the MDCC register --value=xx --address=yy')
grp_action.add_argument('--trig-getregister', action='store_true',
                        help=' get the value of the MDCC register  --address=yy')
grp_action.add_argument('--trig-tdcreset', action='store_true',
                        help=' hard reset of TDC (FEB) counters')


# Slow Control Managment
grp_action.add_argument('--slc-create', action='store_true',
                        help='Send a CREATE transition to FSLOW app (parsing of configuration)')
grp_action.add_argument('--slc-initialise', action='store_true',
                        help='Send a INITIALISE transition to FSLOW (auto discovery and CREATE transition to all controlled apps) ')
grp_action.add_argument('--slc-configure', action='store_true',
                        help='Send a CONFIGURE transition to FSLOW (configuration of all controlled apps)')
grp_action.add_argument('--slc-start', action='store_true',
                        help='Send a START transition to all monitorApplication controlled ')
grp_action.add_argument('--slc-stop', action='store_true',
                        help='Send a STOP transition to all monitorApplication controlled')
# LV
grp_action.add_argument(
    '--slc-lvon', action='store_true', help='put Zup LV ON')
grp_action.add_argument(
    '--slc-lvoff', action='store_true', help='put Zup LV OFF')
grp_action.add_argument(
    '--slc-lvstatus', action='store_true', help='Dump LV status')

#grp_action.add_argument('--slc-loadreferences',action='store_true',help='load in the wiener crate chambers references voltage download from DB')
grp_action.add_argument('--slc-hvstatus', action='store_true',
                        help='display hvstatus of all channel of the wiener crate')

#grp_action.add_argument('--slc-setperiod',action='store_true',help='set the readout period of Wiener and BMP with --period=second(s)')
grp_action.add_argument('--slc-vset', action='store_true',
                        help='set the voltage V of channel i to k with --first=i --last=k --voltage=V')
grp_action.add_argument('--slc-iset', action='store_true',
                        help='set the current limit I (microA) of channel i to k with --first=i --last=k --current=I')
grp_action.add_argument('--slc-rampup', action='store_true',
                        help='set the ramp (V/S) of channel i to k with --first=i --last=k --ramp=r')

grp_action.add_argument('--slc-hvon', action='store_true',
                        help='set the voltage ON  of channel i to k with --first=i --last=k ')
grp_action.add_argument('--slc-hvoff', action='store_true',
                        help='set the voltage OFF  of channel i to k with --first=i --last=k ')
grp_action.add_argument('--slc-clearalarm', action='store_true',
                        help='Clear alarm  of channel i to k with --first=i --last=k ')


# Environmental
grp_action.add_argument('--slc-ptstatus', action='store_true',
                        help='display the P and T from the BMP183 readout')
grp_action.add_argument('--slc-humstatus', action='store_true',
                        help='display the Humidity and T from the HIH8000 readout')
grp_action.add_argument('--slc-ptcor', action='store_true',
                        help='display the needed voltage for --v0=v --p0=p (mbar) --t0=t (K)')


#grp_action.add_argument('--slc-store',action='store_true',help='start the data storage in the mysql DB at period p  with --period=p (s) ')
#grp_action.add_argument('--slc-store-stop',action='store_true',help='stop the data storage in the mysql DB ')
#grp_action.add_argument('--slc-check',action='store_true',help='start the voltage tuning wrt references at period p  with --period=p (s) ')
#grp_action.add_argument('--slc-check-stop',action='store_true',help='stop the voltage tuning ')


# Arguments
parser.add_argument('--config', action='store', dest='config',
                    default=None, help='json config file')
parser.add_argument('--socks', action='store', type=int,
                    dest='sockport', default=None, help='use SOCKS port ')
parser.add_argument('--dbstate', action='store', default=None,
                    dest='dbstate', help='set the dbstate')
parser.add_argument('--ctrlreg', action='store', default=None,
                    dest='ctrlreg', help='set the Control register in hexa')
parser.add_argument('--version', action='version', version='%(prog)s 1.0')
parser.add_argument('--state', action='store', type=str,
                    default=None, dest='fstate', help='set the Daq state')
parser.add_argument('--clock', action='store', type=int, default=None,
                    dest='clock', help='set the number of 20 ns clock')
parser.add_argument('--directory', action='store', type=str,
                    dest='directory', default=None, help='shm publisher directory')
parser.add_argument('--vth', action='store', type=int,
                    default=None, dest='vth', help='set the vth for chips')
parser.add_argument('--mask', action='store', type=int,
                    default=None, dest='mask', help='set the mask for chips')
parser.add_argument('--gain', action='store', type=int,
                    default=None, dest='gain', help='set the gain for chips')
parser.add_argument('--B0', action='store', type=int,
                    default=None, dest='B0', help='set the B0 for chips')
parser.add_argument('--B1', action='store', type=int,
                    default=None, dest='B1', help='set the B1 for chips')
parser.add_argument('--B2', action='store', type=int,
                    default=None, dest='B2', help='set the B2 for chips')

parser.add_argument('--v0', action='store', type=float,
                    default=None, dest='v0', help='reference hv')
parser.add_argument('--p0', action='store', type=float,
                    default=None, dest='p0', help='reference P')
parser.add_argument('--t0', action='store', type=float,
                    default=None, dest='t0', help='reference T')

parser.add_argument('--tdc', action='store', type=int,
                    default=None, dest='tdc', help='set the tdc process')

# Slow
parser.add_argument('--channel', action='store', type=int,
                    default=None, dest='channel', help='set the hvchannel')
parser.add_argument('--first', action='store', type=int,
                    default=None, dest='first', help='set the first hvchannel')
parser.add_argument('--last', action='store', type=int,
                    default=None, dest='last', help='set the last hvchannel')
parser.add_argument('--step', action='store', type=int, default=None,
                    dest='step', help='set the step of the calibration')
parser.add_argument('--voltage', action='store', type=float,
                    default=None, dest='voltage', help='set the hv voltage')
parser.add_argument('--current', action='store', type=float,
                    default=None, dest='current', help='set the hv current')
parser.add_argument('--ramp', action='store', type=float,
                    default=None, dest='ramp', help='set the hv ramp')

parser.add_argument('--account', action='store', default=None,
                    dest='account', help='set the Slow Control mysql account')
parser.add_argument('--period', action='store', type=int, default=None,
                    dest='period', help='set the temporistaion period (s)')

parser.add_argument('--spillon', action='store', type=float,
                    default=None, dest='spillon', help='spill on')
parser.add_argument('--spilloff', action='store', type=float,
                    default=None, dest='spilloff', help='spilloff')

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
parser.add_argument('--address', action='store', type=int,
                    dest='address', default=None, help='address to pass')
parser.add_argument('--active', action='store', type=int,
                    dest='active', default=None, help='active time of Petiroc')
parser.add_argument('--dead', action='store', type=int,
                    dest='dead', default=None, help='dead time of Petiroc')
parser.add_argument('--asic', action='store', type=int,
                    dest='asic', default=None, help='asic header of Petiroc')
parser.add_argument('--feb', action='store', type=int,
                    dest='feb', default=None, help='FEB id')
parser.add_argument('--dbversion', action='store', type=int,
                    dest='dbversion', default=None, help='DB state version')
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

fdc = dqc.fdaqClient()
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
elif(results.daq_services):
    r_cmd = 'prepareServices'
    fdc.daq_services()
    exit(0)
elif(results.daq_initialise):
    r_cmd = 'initialise'
    fdc.daq_initialise()
    exit(0)
elif(results.daq_configure):
    r_cmd = 'configure'
    fdc.daq_configure()
    exit(0)
elif(results.daq_difstatus):
    r_cmd = 'status'
    sr = fdc.daq_difstatus()
    if (results.verbose):
        print sr
    else:
        dqc.parseReturn(r_cmd, sr)
    exit(0)
elif(results.daq_tdcstatus):
    r_cmd = 'tdcstatus'
    sr = fdc.daq_tdcstatus()
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
    fdc.trig_pause()
    fdc.daq_stop()
    exit(0)
elif(results.daq_destroy):
    r_cmd = 'destroy'
    fdc.daq_destroy()
    exit(0)
elif(results.daq_dbstatus):
    r_cmd = 'dbStatus'
    sr = fdc.daq_dbstatus()
    if (results.verbose):
        print sr
    else:
        dqc.parseReturn(r_cmd, sr)
    exit(0)
elif(results.daq_evbstatus):
    r_cmd = 'shmStatus'
    sr = fdc.daq_evbstatus()
    if (results.verbose):
        print sr
    else:
        dqc.parseReturn(r_cmd, sr)
    exit(0)
elif(results.daq_downloaddb):
    r_cmd = 'downloadDB'
    if (results.dbstate != None):
        lcgi['state'] = results.dbstate
    else:
        print 'Please specify the state --dbstate=STATE'
        exit(0)
    version=0
    if (results.dbversion != None):
        version = results.dbversion
    fdc.daq_downloaddb(results.dbstate,version)
    exit(0)

### DIF

elif(results.dif_setgain):
    r_cmd = 'SETGAIN'
    if (results.gain == None):
        print 'Please specify the gain --gain=value'
        exit(0)
    fdc.dif_setgain(results.gain)
    exit(0)
elif(results.dif_setthreshold):
    r_cmd = 'SETTHRESHOLD'
    if (results.B0 == None):
        print 'Please specify the B0 --B0=value'
        exit(0)
    if (results.B1 == None):
        print 'Please specify the B1 --B1=value'
        exit(0)
    if (results.B2 == None):
        print 'Please specify the B2 --B2=value'
        exit(0)
    fdc.dif_setthreshold(results.B0, results.B1, results.B2)
    exit(0)
elif(results.dif_ctrlreg):
    r_cmd = 'setControlRegister'
    lct = 0
    if (results.ctrlreg != None):
        lct = int(results.ctrlreg, 16)
    else:
        print 'Please specify the value --ctrlreg=0X######'
        exit(0)
    fdc.dif_ctrlreg(results.ctrlreg)
    exit(0)


### FEB

elif(results.feb_resettdc):
    r_cmd = 'tdcstatus'
    sr = fdc.feb_resettdc()
    print sr
    exit(0)
elif(results.feb_setvth):
    r_cmd = 'SETVTH'
    if (results.vth == None):
        print 'Please specify the vth --vth=value'
        exit(0)
    fdc.feb_setvthtime(results.vth)
    exit(0)
elif(results.feb_setmask):
    r_cmd = 'SETMASK'
    if (results.mask == None):
        print 'Please specify the vth --mask=value'
        exit(0)
    asic = 255
    if (results.asic != None):
        asic = results.asic
    fdc.feb_setmask(results.mask, asic)
    exit(0)

elif(results.feb_settdcmode):
    r_cmd = 'SETTDCMODE'
    if (results.value == None):
        print 'Please specify the mode --value=XX'
        exit(0)
    fdc.feb_settdcmode(results.value)
    exit(0)

elif(results.feb_settdcdelays):
    r_cmd = 'SETTDCDELAYS'
    if (results.active == None):
        print 'Please specify the active time --active=XX'
        exit(0)
    if (results.dead == None):
        print 'Please specify the dead time --dead=XX'
        exit(0)
    fdc.feb_settdcdelays(results.active, results.dead)
    exit(0)

elif(results.feb_calibdac):
    r_cmd = 'calibdac'
    fdc.feb_calibdac(200, 15, 15, 61, 4294967295)
    exit(0)
elif(results.feb_scurve):
    r_cmd = 'scurve'
    first = 300
    last = 600
    chan = 255
    step = 2
    spillon = 200
    spilloff = 1000
    mask = 255
    if (results.first != None):
        first = results.first
    if (results.last != None):
        last = results.last
    if (results.channel != None):
        chan = results.channel
    if (results.step != None):
        step = results.step
    if (results.spillon != None):
        spillon = results.spillon
    if (results.spilloff != None):
        spilloff = results.spilloff
    if (results.mask != None):
        mask = results.mask

    print "Mode %d from %d to %d with step %d Spill On %d Off %d ASIC mask %d" % (chan, first, last, step, spillon, spilloff, mask)
    val = raw_input()

    fdc.feb_fullscurve(chan, spillon, spilloff, first, last, step, mask)
    exit(0)
elif(results.feb_lutcalib):
    r_cmd = 'lutcalib'
    if (results.tdc == None):
        print 'Please specify the tdc process --tdc=#'
        exit(0)
    if (results.channel == None):
        print 'Please specify the channel --channel=#'
        exit(0)
    fdc.feb_lut_calib(results.tdc, results.channel)
    exit(0)
elif(results.feb_lutdraw):
    r_cmd = 'lutdraw'
    if (results.tdc == None):
        print 'Please specify the tdc process --tdc=#'
        exit(0)
    if (results.channel == None):
        print 'Please specify the channel --channel=#'
        exit(0)
    fdc.feb_lut_draw(results.tdc, results.channel)
    exit(0)
elif(results.histo_draw):
    r_cmd = 'lutdraw'
    if (results.name == None):
        print 'Please specify the histo name --name=#'
        exit(0)
    fdc.histo_draw(results.name)
    exit(0)


### MDCC

elif(results.trig_status):
    r_cmd = 'triggerStatus'
    res=fdc.trig_status()
    print res
    exit(0)

elif(results.trig_beam):
    r_cmd = 'triggerBeam'
    if (results.clock != None):
        fdc.trig_beam(results.clock)
    else:
        print 'Please specify the number of clock --clock=xx'
    exit(0)


elif(results.trig_spillon):
    r_cmd = 'triggerSpillOn'
    if (results.clock != None):
        fdc.trig_spillon(results.clock)
    else:
        print 'Please specify the number of clock --clock=xx'
    exit(0)


elif(results.trig_spilloff):
    r_cmd = 'triggerSpillOff'
    if (results.clock != None):
        fdc.trig_spilloff(results.clock)
    else:
        print 'Please specify the number of clock --clock=xx'
    exit(0)

elif(results.trig_spillregister):
    r_cmd = 'triggerSpillRegister'
    if (results.value != None):
        fdc.trig_spillregister(results.value)
    else:
        print 'Please specify the value --value=xx'
    exit(0)
elif(results.trig_setregister):
    r_cmd = 'triggerSetRegister'
    if (results.value == None):
        print 'Please specify the value --value=xx'
        exit(0)
    if (results.address == None):
        print 'Please specify the address --address=xx'
        exit(0)
    sr = fdc.trig_setregister(results.address, results.value)
    print sr
    exit(0)
elif(results.trig_getregister):
    r_cmd = 'triggerGetRegister'
    if (results.address == None):
        print 'Please specify the address --address=xx'
        exit(0)
    sr = fdc.trig_getregister(results.address)
    print sr
    exit(0)
elif(results.trig_tdcreset):
    r_cmd = 'trigResetTdc'
    sr = fdc.trig_tdcreset()
    print sr
    exit(0)
elif(results.trig_calibon):
    r_cmd = 'triggerSpillRegister'
    if (results.value != None):
        fdc.trig_calibon(results.value)
    else:
        print 'Please specify the value --value=xx'
    exit(0)
elif(results.trig_calibcount):
    r_cmd = 'triggerSpillRegister'
    if (results.value != None):
        fdc.trig_calibcount(results.value)
    else:
        print 'Please specify the value --value=xx'
    exit(0)
elif(results.trig_rearm):
    r_cmd = 'triggerSpillRegister'

    fdc.trig_reloadcalib()
    exit(0)
elif(results.trig_hardreset):
    r_cmd = 'triggerhardReset'

    fdc.trig_hardreset()

    exit(0)

elif(results.ecal_pause):
    r_cmd = 'pauseEcal'
    fdc.ecal_pause()
    exit(0)

elif(results.ecal_resume):
    r_cmd = 'resumeEcal'
    fdc.ecal_resume()
    exit(0)

elif(results.trig_reset):
    r_cmd = 'resetTrigger'
    fdc.trig_reset()
    exit(0)
elif(results.trig_pause):
    r_cmd = 'pause'
    fdc.trig_pause()
    exit(0)
elif(results.trig_resume):
    r_cmd = 'resume'
    fdc.trig_resume()
    exit(0)

### SLOW CONTROL

elif(results.slc_create):
    r_cmd = 'createSlowControl'
    fdc.slow_create()
    exit(0)

elif(results.slc_initialise):
    r_cmd = 'initialise'
    fdc.slow_discover()
    exit(0)
elif(results.slc_configure):
    r_cmd = 'configure'
    fdc.slow_configure()
    exit(0)
elif(results.slc_start):
    r_cmd = 'start'
    fdc.slow_start()
    exit(0)
elif(results.slc_stop):
    r_cmd = 'stop'
    fdc.slow_stop()
    exit(0)
elif(results.slc_lvon):
    r_cmd = 'LVON'
    sr = fdc.slow_lvon()
    if (results.verbose):
        print sr
    else:
        dqc.parseReturn('LVSTATUS', sr)
    exit(0)

elif(results.slc_lvoff):
    r_cmd = 'LVOFF'
    sr = fdc.slow_lvoff()
    if (results.verbose):
        print sr
    else:
        dqc.parseReturn('LVSTATUS', sr)
    exit(0)
elif(results.slc_lvstatus):
    r_cmd = 'LVStatus'
    sr = fdc.slow_lvstatus()
    if (results.verbose):
        print sr
    else:
        dqc.parseReturn('LVSTATUS', sr)
    exit(0)

elif(results.slc_hvstatus):
    r_cmd = 'hvStatus'
    if (results.first == None):
        print 'Please specify the channels --first=# --last=#'
        exit(0)
    if (results.last == None):
        print 'Please specify the channels --first=# --last=#'
        exit(0)
    sr = fdc.slow_hvstatus(results.first, results.last)
    if (results.verbose):
        print sr
    else:
        dqc.parseReturn(r_cmd, sr)
    exit(0)
elif(results.slc_hvon):
    r_cmd = 'hvStatus'
    if (results.first == None):
        print 'Please specify the channels --first=# --last=#'
        exit(0)
    if (results.last == None):
        print 'Please specify the channels --first=# --last=#'
        exit(0)
    sr = fdc.slow_hvon(results.first, results.last)
    if (results.verbose):
        print sr
    else:
        dqc.parseReturn(r_cmd, sr)
    exit(0)
elif(results.slc_hvoff):
    r_cmd = 'hvStatus'
    if (results.first == None):
        print 'Please specify the channels --first=# --last=#'
        exit(0)
    if (results.last == None):
        print 'Please specify the channels --first=# --last=#'
        exit(0)
    sr = fdc.slow_hvoff(results.first, results.last)
    if (results.verbose):
        print sr
    else:
        dqc.parseReturn(r_cmd, sr)
    exit(0)

elif(results.slc_ptstatus):
    r_cmd = 'PT'
    sr = fdc.slow_ptstatus()
    if (results.verbose):
        print sr
    else:
        dqc.parseReturn('PTSTATUS', sr)
    exit(0)
elif(results.slc_humstatus):
    r_cmd = 'PT'
    sr = fdc.slow_humstatus()
    if (results.verbose):
        print sr
    else:
        dqc.parseReturn('HUMSTATUS', sr)
    exit(0)

elif(results.slc_ptcor):
    r_cmd = 'PT'
    sr = fdc.slow_ptstatus()
    if (results.verbose):
        print sr
    else:
        dqc.parseReturn('PTCOR', sr, results)
    exit(0)

elif(results.slc_vset):
    r_cmd = 'setVoltage'
    if (fdc.slowhost == None or fdc.slowport == None):
      print "No FSLOW application exiting"
      exit(0)
    if (results.first == None):
        print 'Please specify the channels --first=# --last=#'
        exit(0)
    if (results.last == None):
        print 'Please specify the channels --first=# --last=#'
        exit(0)
    if (results.voltage == None):
        print 'Please specify the voltage --voltage=V'
        exit(0)
    sr = fdc.slow_vset(results.first, results.last, results.voltage)
    if (results.verbose):
        print sr
    else:
        dqc.parseReturn('hvStatus', sr)
    exit(0)
elif(results.slc_iset):
    r_cmd = 'setCurrent'
    if (fdc.slowhost == None or fdc.slowport == None):
      print "No FSLOW application exiting"
      exit(0)
    if (results.first == None):
        print 'Please specify the channels --first=# --last=#'
        exit(0)
    if (results.last == None):
        print 'Please specify the channels --first=# --last=#'
        exit(0)
    if (results.current == None):
        print 'Please specify the current --current=V'
        exit(0)
    sr = fdc.slow_iset(results.first, results.last, results.current)
    if (results.verbose):
        print sr
    else:
        dqc.parseReturn('hvStatus', sr)
    exit(0)

elif(results.slc_rampup):
    r_cmd = 'setVoltage'
    if (fdc.slowhost == None or fdc.slowport == None):
      print "No FSLOW application exiting"
      exit(0)
    if (results.first == None):
        print 'Please specify the channels --first=# --last=#'
        exit(0)
    if (results.last == None):
        print 'Please specify the channels --first=# --last=#'
        exit(0)
    if (results.ramp == None):
        print 'Please specify the ramp --ramp=V'
        exit(0)
    fdc.slow_rampup(results.first, results.last, results.ramp)
    if (results.verbose):
        print sr
    else:
        dqc.parseReturn('hvStatus', sr)
    exit(0)
