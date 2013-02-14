#!/bin/bash
# run on rack leads
#  cexec --all /tmp/alerttest
# may need to push
#  cpush --nolocal /tmp/alerttest /tmp/alerttest 


# start up in band ipmi if needed
/etc/init.d/ipmi start

n=1
while [ $n -le 10 ]
do
        ipmitool raw 0x4 0x16 1 1 0 0 0 4 1 1 0 0 0
        n=$(( $n + 1))
done

