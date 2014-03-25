#/bin/bash

DATE=`date +%Y%m%d`
OBS=`date +%Y%m%d-%H%M%S`
for STATION in CS002 CS003 CS004 CS005 CS006 CS007
do
  for RSP in 0 1 2 3
  do
    NODE=`grep LANE_0${RSP}_DSTIP RSPDriver.conf.$STATION | awk '{ print $3; }'`

    echo stopping udp-copy for $STATION RSP $RSP on locus node $NODE
    ssh $NODE killall udp-copy-8bit-dip
    ssh $NODE killall numactl
  done
done

