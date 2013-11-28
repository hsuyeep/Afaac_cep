#!/bin/bash
# Script to extract out a minute of data every 15 mins into a separate folder

# Find out range of times in the raw files specified
times=(`read_fast_timestamps_24sb CS002-RSP0*$1.raw | awk '{print $3}'`)
nsec=`expr ${times[1]} - ${times[0]}`;
nmin=`bc<<EOF
$nsec/900
EOF`;
echo "Found $nsec seconds in file, splitting into $nmin segments";

# For each instance, generate a filename with timerange
# Packet size = 12*2*16*2*2 + 16 = 1552Bytes
# 1 subband = 2000000000/1024 = 195312.50 samples/sec
# Hence, number of packets per second = 195312.5/16(samps per pkt) = 12207.03125
# Hence, number of packets per minute = 12207.03125*60 = 732421.875
# Hence, number of packets per 30secs = 12207.03125*30 = 366211

count=`expr 12208 \* 30`;
files=(CS00*$1.raw);
nmin=`expr $nmin - 1`;
for ts in `seq 0 $nmin`; do
  skip=`expr $ts \* 10986328`;
  tstart=`expr ${times[0]} + $ts \* 900`;
  for f in ${files[@]}; do
    fname=$(printf '%s_%02d_%s' $f $ts $tstart);
    echo "dd if=$f of=$fname skip=$skip bs=1552 count=$count"
    dd if=$f of=$fname skip=$skip bs=1552 count=$count
  done; 
done;
