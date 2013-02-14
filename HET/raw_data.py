#! /usr/bin/python -u

import sys

f = open("r1i1c.sdrcache","rb")
#f = open("service0.sdrcache","rb")

sensor = {}
record_length = {}
t_type = {}

print "id1, id2, version, type, length, raw_len, nam_code, nam_pos, nam, raw"
while True:
    try:
        id1    = f.read(1)
        id2    = f.read(1)
        version    = ord( f.read(1) )
        type    = ord( f.read(1) )
        length    = ord( f.read(1) )
        raw    = f.read(length)
        raw_len = len(raw)
        nam_pos = raw_len - 16
        nam     = raw[nam_pos:].replace('\0','',1000)
        nam_code= ord( raw[nam_pos -1] )
        no_pos  = 2
        sensor_n=ord(raw[no_pos:no_pos+1])

        if nam_code == 0:
            continue

        sensor[ '0x%02x' % sensor_n ] = nam
	record_length[ '0x%02x' % sensor_n ] = raw_len
	t_type[ '0x%02x' % sensor_n ] = type

        # sensor.append(  ( # id1, id2, version, type, length, raw_len, nam_code, nam_pos, nam #, raw # nam, sensor_n # ) )
    except:
        break
f.close();

for k in sorted(sensor.keys()):
    print "%s\t%s\t%s\t%x\t%x" % ( "dico", k, sensor[k], record_length[k], t_type[k] )


# sys.exit(0)

