#! /usr/bin/python
# -*- encoding: iso-8859-15 -*-
# 
#   This work is held in copyright as an unpublished work by
#   Silicon Graphics, Inc.  All rights reserved.
#  
#   Copyright (c) 2012-2013 Silicon Graphics, Inc.
#   All rights reserved.
#  
# 

from socket     import gethostname
from threading  import Condition, RLock
import syslog
import signal

MY_NAME        = "het"
    
# Global default parameters
debug_level    = 0
snmp_port      = 162 + 22000                  # Still un-assigned by IANA.  Tue Nov 13 17:23:33 CST 2012
snmp_iface     = ''
snmp_community = 'sgi'
snmp_pipe      = 0
snmp_pipe_file = '@HET_PIPE@'

# ===== Useful constants
SECONDS= 1
MINUTE = 60*SECONDS
HOUR   = MINUTE*60
DAY    = HOUR*24
WEEK   = DAY*7
MONTH  = DAY*30
YEAR   = DAY*365



# Globals defines
# NOTE: Please sync up with /etc/sysconfig/het ( het.sysconfig )
snmp_oids     = {
      'ipmi':       '.1.3.6.1.4.1.3183'       # Intel IPMI Controllers
#   , 'opto22':     '.1.3.6.1.4.1.4473'       # Opto-22 Enterprise OID: too general
    , 'opto22':     '.1.3.6.1.4.1.4473.6.152' # Opto-22 Enterprise OID for brainSnapB3000ENET
    , 'liebert':    '.1.3.6.1.4.1.476.1.42.3' # LIEBERT-GP-REGISTRATION-MIB::lgpFoundation
    , 'ups':        '.1.3.6.1.2.1.33.2'       # UPS-MIB::upsTraps ( comes w/Liebert: optional )
      }
trap_bytes = {
      'header': {
           # Key              Value,Enabled Reqname              	Fmt
           # Enabled:1: Disabled:0  1:Collect+Print -1:Collect,NoPrint
          'srcAddr':        [ None,  1,      'src[0]',			'"%s"' ]
        , 'srcPort':        [ None,  0,      'src[1]',			'%s'   ]
        , 'version':        [ None,  0,      'req["version"]',		'%s'   ]
        , 'tag':            [ None,  0,      'req["tag"]',		'"%s"' ]
        , 'oid':            [ None,  1,      'req["enterprise"]',	'"%s"' ]
        # ----------------------------------------------------------------------
        # All_Type: Next value is not collected but derived from the 'oid'
        # above.
        , 'type':           [ None,  1,      None,                      '%s'   ]
        # ----------------------------------------------------------------------
        , 'agentAddr':      [ None,  1,      'req["agent_addr"]',	'"%s"' ]
        # XXX:  When the srcAddr is not 127.0.0.1, that means it's not coming thru the snmptrapd
        #       'forward' directive, ie. the sender send directly to our listening port 22162.
        #       This is TRUE for Opto22 ( CRC ) as a WAR because their agentAddr is incorrectly set
        #       as port 1 IP when their trap is sent thru port2 IP and querying thru port1
        #       just doesn't work , ie hang, timeout, etc ...
        #     
        #       So, a generic solution is to use the following for all ipmi/snmp equipments:
        #
        #                queryAddr = srcAddr == "127.0.0.1" ? agentAddr : srcAddr
        #       
        , 'queryAddr':      [ None,  1,      'IIF(src[0] == "127.0.0.1", req["agent_addr"], src[0])', '"%s"' ]
        , 'genericTrap':    [ None, -1,      'req["generic_trap"]',	'%s'   ]
        , 'specificTrap':   [ None,  1,      '"0x%08x" % int(req["specific_trap"])', '%s' ]
        # ----------------------------------------------------------------------
        # IPMI: Next 3 values are not collected from the PDU trap but
        #       derived from specificTrap data above
        , 'sensorType':     [ None,  1,      None,                      '%s'   ]
        , 'eventType':      [ None,  1,      None,                      '%s'   ]
        , 'eventOffset':    [ None,  1,      None,                      '%s'   ]
        # ----------------------------------------------------------------------
        , 'timeStamp':      [ None,  1,      'req["time_stamp"]',	'%s'   ]
        , 'when':           [ None,  1,      'time.time()',             '%s'   ]
      }
    , 'ipmi': { # MUST match snmp_oid keys
           # Key              Value,Enabled Position Fmt    
           # Position:0 = do not collect
          'guid1':          [ None, -1,       1,      '0x%02x' ]
        , 'guid2':          [ None, -1,       2,      '0x%02x' ]
        , 'guid3':          [ None, -1,       3,      '0x%02x' ]
        , 'guid4':          [ None, -1,       4,      '0x%02x' ]
        , 'guid5':          [ None, -1,       5,      '0x%02x' ]
        , 'guid6':          [ None, -1,       6,      '0x%02x' ]
        , 'guid7':          [ None, -1,       7,      '0x%02x' ]
        , 'guid8':          [ None, -1,       8,      '0x%02x' ]
        , 'guid9':          [ None, -1,       9,      '0x%02x' ]
        , 'guid10':         [ None, -1,      10,      '0x%02x' ]
        , 'guid11':         [ None, -1,      11,      '0x%02x' ]
        , 'guid12':         [ None, -1,      12,      '0x%02x' ]
        , 'guid13':         [ None, -1,      13,      '0x%02x' ]
        , 'guid14':         [ None, -1,      14,      '0x%02x' ]
        , 'guid15':         [ None, -1,      15,      '0x%02x' ]
        , 'guid16':         [ None, -1,      16,      '0x%02x' ]
        , 'guid':           [ None,  1,       0,      '"%s"'   ]
        , 'seqNo':          [ None,  0,      17,      '0x%02x' ]
        , 'seqCookie':      [ None,  0,      18,      '0x%02x' ]
        , 'trapSourceType': [ None,  1,      25,      '0x%02x' ]
        , 'eventSourceType':[ None,  1,      26,      '0x%02x' ]
        , 'alertSeverity':  [ None,  1,      27,      '0x%02x' ]
        , 'sensorDevice':   [ None,  0,      28,      '0x%02x' ]
        , 'sensorNumber':   [ None,  1,      29,      '0x%02x' ]
        , 'entity':         [ None,  0,      30,      '0x%02x' ]
        , 'instance':       [ None,  0,      31,      '0x%02x' ]
        , 'event1':         [ None,  1,      32,      '0x%02x' ]
        , 'event2':         [ None,  1,      33,      '0x%02x' ]
        , 'event3':         [ None,  1,      34,      '0x%02x' ]
        }
    , 'opto22': {   # MUST match snmp_oid keys
           # Key              Value,Enabled Position Fmt    
           # Position: do not collect
           # TBD
        }
    , 'liebert': {   # MUST match snmp_oid keys
           # Key              Value,Enabled Position Fmt    
           # Position: do not collect
           # TBD
        }
    , 'ups': {   # MUST match snmp_oid keys
           # Key              Value,Enabled Position Fmt    
           # Position: do not collect
           # TBD
        }
    }

# Table 17-2, byte 4
ipmi_severities = {
      0x00: "NONE"
    , 0x01: "MONITOR"
    , 0x02: "INFORMATION"
    , 0x04: "OK"
    , 0x08: "WARNING"
    , 0x10: "CRITICAL"
    , 0x20: "NON-RECOVERABLE"
}

# ===== Global variables
PARAM={}                    # provide command-line params
CONFIG={}                   # provide param/nodes/traps config
LOCALHOST=gethostname()
CONFIG_FILE='@HET_CONF@'
CYCLE_DEBUG_MAX=3
CYCLE_DEBUG_SIGNO=signal.SIGUSR2

# Flap stuff
HET_FLAP_COUNT    = 10
HET_FLAP_INTERVAL = 7200    # 7200 seconds ( 2 hours )

# Actions stuff
ACTION_DIR  = "@ACTION_DIR@"


LOGFAC=syslog.LOG_DAEMON
LOGOPT=syslog.LOG_PID|syslog.LOG_NDELAY
LOG_DEBUG1=syslog.LOG_DEBUG
LOG_DEBUG2=syslog.LOG_DEBUG+1
LOG_DEBUG3=syslog.LOG_DEBUG+2
LOG_DEBUG4=syslog.LOG_DEBUG+3
LOG_DEBUG5=syslog.LOG_DEBUG+4

# ===== TRAP information: monitor and current status
#from trap_list  import *        # trap_list is generated and provides TRAPS{} dico

# vim: syntax=python sw=4 sts=4 et
