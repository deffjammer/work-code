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

#import  os
#import  time

info = {
    'agentAddr':        '172.24.0.2'                              ,
    'het_type':         'ipmi'                                    , # moved up to near the top
    'guid':             'r1lead'                                  , # also moved up
    'host':             'r1lead'                                  , # duplication - investigate
    'sn':               'X1--------'                              , # moved up
    'alert':            '(False, X1--------,r1lead,0x5d,fan, 9)' , # this line makes no sense, delete
    'alertSeverity':    'NON-RECOVERABLE'                         ,
    'epoch':            '1362345911.04'                           , # not useful, delete
    'event':            'lnrGoingLow'                             ,
    'sensorName':       'None'                                    , # the sensor detail is moved up
    'sensorNumber':     '0x5d',
    'sensorThreshold':  '133',
    'sensorTypeName':   'fan',
    'sensorType':       '4'                                       ,# sensor type only used if sensortypename not avail
    'sensorValue':      '246',
    'event1':           '0x54',                                    # event1,2,3 only listed if cannot translate
    'event2':           '0xf6',
    'event3':           '0x85',
    'eventClassName':   'threshold',
    'eventOffset':      '4'        ,                               # only print this if it cannot be translated
    'eventSourceType':  '0x20'     ,                               # not useful, delete
    'eventType':        '1'        ,                               # only print this if eventClassName not available
    'flap_count':       '9',
    'flap_since_epoch': '1362345911.04'                           , # not useful, delete
    'flap_since_when':  '2013-03-03.15.25.11 CDT',
    'flap_state':       'False',
    'oid':              '.1.3.6.1.4.1.3183.1.1',                    # sooo not useful, delete
    'queryAddr':        '172.24.0.2'           ,                    # duplicate, delete
    'specificTrap':     '0x00040104'           ,                    # not useful, delete
    'srcAddr':          '127.0.0.1'            ,                    # not useful, delete
    'timeStamp':        '981709'               ,                    # delete
    'trapSourceType':   '0x20'                 ,                    # delete
    'trouble':          '(False, None, None)'  ,                    # delete
    'type':             'ipmi'                 ,                    # delete
    'when':             '2013-03-03.15.25.11 CDT'
}
print_bytes_list = [
    'agentAddr',
    'het_type',
    'guid',
    'sn',
    'alertSeverity',
    'event',
    'sensorName',
    'sensorNumber',
    'sensorThreshold',
    'sensorTypeName',
    'sensorValue',
    'eventClassName',
    'flap_count',
    'flap_since_when',
    'flap_state',
    'when'
    # if not sensorTypeName
    # sensorType
    # event1,2,3
    # eventType
]


print_bytes = {
    'sn':               'X1--------',
    'alertSeverity':    'NON-RECOVERABLE',
    'event':            'lnrGoingLow',
    'sensorName':       'None',
    'sensorNumber':     '0x5d',
    'sensorThreshold':  '133',
    'sensorTypeName':   'fan',
    'sensorValue':      '246',
    'eventClassName':   'threshold',
    'flap_count':       '9',
    'flap_since_when':  '2013-03-03.15.25.11 CDT',
    'flap_state':        'False',
    'when':              '2013-03-03.15.25.11 CDT'
    # if not sensorTypeName
    # sensorType
    # event1,2,3
    # eventType
}

# Globals defines
# NOTE: Please sync up with /etc/sysconfig/het ( het.sysconfig )
snmp_oids     = {
    'ipmi':       '.1.3.6.1.4.1.3183'       # Intel IPMI Controllers
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
    , 'ipmi': {
        # MUST match snmp_oid keys
        # Key              Value,Enabled Position Fmt
        # Position:0 = do not collect
        'guid1':             [ None, -1,       1,      '0x%02x' ]
        , 'guid2':           [ None, -1,       2,      '0x%02x' ]
        , 'guid3':           [ None, -1,       3,      '0x%02x' ]
        , 'guid4':           [ None, -1,       4,      '0x%02x' ]
        , 'guid5':           [ None, -1,       5,      '0x%02x' ]
        , 'guid6':           [ None, -1,       6,      '0x%02x' ]
        , 'guid7':           [ None, -1,       7,      '0x%02x' ]
        , 'guid8':           [ None, -1,       8,      '0x%02x' ]
        , 'guid9':           [ None, -1,       9,      '0x%02x' ]
        , 'guid10':          [ None, -1,      10,      '0x%02x' ]
        , 'guid11':          [ None, -1,      11,      '0x%02x' ]
        , 'guid12':          [ None, -1,      12,      '0x%02x' ]
        , 'guid13':          [ None, -1,      13,      '0x%02x' ]
        , 'guid14':          [ None, -1,      14,      '0x%02x' ]
        , 'guid15':          [ None, -1,      15,      '0x%02x' ]
        , 'guid16':          [ None, -1,      16,      '0x%02x' ]
        , 'guid':            [ None,  1,       0,      '"%s"'   ]
        , 'seqNo':           [ None,  0,      17,      '0x%02x' ]
        , 'seqCookie':       [ None,  0,      18,      '0x%02x' ]
        , 'trapSourceType':  [ None,  1,      25,      '0x%02x' ]
        , 'eventSourceType': [ None,  1,      26,      '0x%02x' ]
        , 'alertSeverity':   [ None,  1,      27,      '0x%02x' ]
        , 'sensorDevice':    [ None,  0,      28,      '0x%02x' ]
        , 'sensorNumber':    [ None,  1,      29,      '0x%02x' ]
        , 'entity':          [ None,  0,      30,      '0x%02x' ]
        , 'instance':        [ None,  0,      31,      '0x%02x' ]
        , 'event1':          [ None,  1,      32,      '0x%02x' ]
        , 'event2':          [ None,  1,      33,      '0x%02x' ]
        , 'event3':          [ None,  1,      34,      '0x%02x' ]
    }
    , 'opto22': {
        # MUST match snmp_oid keys
        # Key              Value,Enabled Position Fmt
        # Position: do not collect
        # TBD
    }
    , 'liebert': {
        # MUST match snmp_oid keys
        # Key              Value,Enabled Position Fmt
        # Position: do not collect
        # TBD
    }
    , 'ups': {
        # MUST match snmp_oid keys
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

alt_print_bytes = [
    'sensorType',
    'eventType',
    'eventOffset',
    'event1',
    'event2',
    'event3'
]

if __name__ == '__main__':

#    for var in trap_bytes['header'].keys():
#        #format_dump(keys, var)
#        print "%s " % var

#    print "list %s " % print_bytes_list

#    for k in d1:
#        if d2.get(k, 0) < l:
#            print k, d2.get(k, 0)
#    for k, v in trap_bytes.iteritems():
#        print "%s %s" % (k, v)

#    for k in print_bytes:
#        print "k - %s get %s" % (k, trap_bytes.get(k, None))

#    for k in trap_bytes.iterkeys():
#        for i in trap_bytes[k].iterkeys():
#            print "%s iner %s" % (k, i)

    keys = info.keys()
    #keys.sort()
    #for k in keys:
        # if one of desired keys, elog/print
    #    print "%-16s %-16s " % (k, info[k])

    print "-----------------------------------------"
#    for k in print_bytes.keys():
#        print_bytes[k] = '00'
#    for k in print_bytes.keys():
#        print "%-16s %s" % (k, info.get(k, None))
    #Winner
    for k in print_bytes_list:
        value = info.get(k, None)
        if value is 'None' and k is 'sensorTypeName' :
            # insert other values
            print "%s has no value %s" % (k, value)
            for i in alt_print_bytes:
                print "%-16s %s" % (i, info.get(i, None))

        print "%-16s %s" % (k, value)
