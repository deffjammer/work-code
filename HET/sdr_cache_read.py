#! /usr/bin/python -u

#struct sdr_get_rs {
#        uint16_t id;             /* Record id */
#        uint8_t  version;        /* SDR version (51h) */
#        uint8_t  type;           /* Record type */
#        uint8_t  length;         /* Remaining record bytes */
#        uint8_t  r1;             /* Sensor Ownser ID */
#        uint8_t  r2;             /* Sensor Owner LUN */
#	     uint8_t  sensor_id;      /* Sensor ID */
#};
SDR_RECORD_TYPE_FULL_SENSOR           = 0x01
SDR_RECORD_TYPE_COMPACT_SENSOR        = 0x02
SDR_RECORD_TYPE_EVENTONLY_SENSOR      = 0x03
SDR_RECORD_TYPE_ENTITY_ASSOC          = 0x08
SDR_RECORD_TYPE_DEVICE_ENTITY_ASSOC   = 0x09
SDR_RECORD_TYPE_GENERIC_DEVICE_LOCATOR  = 0x10
SDR_RECORD_TYPE_FRU_DEVICE_LOCATOR    = 0x11
SDR_RECORD_TYPE_MC_DEVICE_LOCATOR     = 0x12
SDR_RECORD_TYPE_MC_CONFIRMATION       = 0x13
SDR_RECORD_TYPE_BMC_MSG_CHANNEL_INFO  = 0x14
SDR_RECORD_TYPE_OEM                   = 0xc0
SDR_FULL_SENSOR_LENGTH_OFFSET         = 0x2f # 47
SDR_COMPACT_SENSOR_OFFSET             = 0x1f # 31
SDR_EVENTONLY_SENSOR_OFFSET           = 0x10 # 16

REC_LEN_OFFSET = 5
LENGTH_MASK    = 0x0f

import sys
import cPickle as pickle
import pprint


class Record:

    RecordCount = 0

    def __init__(self, id, sensor_type, name):
        self.id   = id
        self.sensor_type = sensor_type
        self.name = name
        Record.RecordCount += 1

    def displayCount(self):
        print "Total Employee %d" % Record.RecordCount

    def displayRecord(self):
        print "ID : ", self.ID, "Type : ", self.sensor_type,  ", Name: ", self.name


# list
sensor_list = []
# dict
sensor_dict = {}

#f = open("service0.sdrcache", "rb")
f = open("r1i1c.sdrcache", "rb")
hostname = 'r1i1c'
SSN      = 0x45ff
while True:
    try:
        pos         = f.tell()
        id1         = ord(f.read(1))
        id2         = ord(f.read(1))
        version     = ord(f.read(1))
        sensor_type = ord(f.read(1))
        length      = ord(f.read(1))
        r1          = ord(f.read(1))
        r2          = ord(f.read(1))
        sensor_id   = ord(f.read(1))

        if sensor_type == SDR_RECORD_TYPE_FULL_SENSOR:
            f.seek(pos + SDR_FULL_SENSOR_LENGTH_OFFSET)

        elif sensor_type == SDR_RECORD_TYPE_COMPACT_SENSOR:
            f.seek(pos + SDR_COMPACT_SENSOR_OFFSET)

        elif sensor_type == SDR_RECORD_TYPE_EVENTONLY_SENSOR:
            f.seek(pos + SDR_EVENTONLY_SENSOR_OFFSET)

        else:
            print 'Type not wanted 0x%02x' % sensor_type
            f.seek(pos + (length + REC_LEN_OFFSET))
            continue

        name_code = ord(f.read(1))
        name_len = LENGTH_MASK & name_code
        name = f.read(name_len)
        f.seek(pos + (length + REC_LEN_OFFSET))

        #print 'ID 0x%02x, Type 0x%02x, Length 0x%02x, Sensor ID 0x%02x Name Code 0x%02x, Name %s' \
        #    % (id1, sensor_type, length, sensor_id, name_code, name)
        sensor_dict['%s-%02x-%04x' % (hostname, sensor_id, SSN)] = name

    except:
        break
f.close()

#print '###### Printed as Stored List #####'
#print 'ID\tSensor Type\tName'
pp = pprint.PrettyPrinter(indent=2)

# Dump pickle object
pf = open('sensor.pickle', 'wb')
pickle.dump(sensor_dict, pf)
pf.close

# Load and print pickle object
#print '###### Printed as Stored Pickle List #####'
pf = open('sensor.pickle', "rb")
sensor_data = pickle.load(pf)
#pp.pprint(sensor_data)
pf.close()

# Iterate through dict and print keys and values
#print '####### Printed with ID as Index ######'
#for k in sorted(sensor_dict.keys()):
#    print '%s\t%s' % (k, sensor_dict[k])

# Print pickle loaded data
print '####### Loaded pickle object (dict ######'
for k in sorted(sensor_data.keys()):
    print '%s\t%s' % (k, sensor_data[k])


sys.exit(0)
