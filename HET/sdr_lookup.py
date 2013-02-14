#! /usr/bin/python -u

import sys

try:
    sdrcache_f = open("r1i0n0.sdrlookup", "r")
except:
    print"Open Failed"
    sys.exit(1)

print "Beginning"

lines = [sensorLine.strip().split('|', 2) for sensorLine in sdrcache_f]

#print lines
#for sensorLines in sdrcache_f:
#    print sensorLines.split('|')

#Need check for blank line at end of file?
for item in lines:
    if not item[0].strip():
        continue
    sensorNum  = item[0].strip()
    sensorName = item[1].strip()
    print "Sensor Num %s 0x%s Name %s" % (sensorNum, sensorNum, sensorName)

sdrcache_f.close()
