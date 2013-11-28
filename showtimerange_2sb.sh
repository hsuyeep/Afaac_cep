#!/bin/bash
for i in `seq 2 7`; do fname=`printf '%s/CS00%dD12*.dip\n' $1 $i`; echo $fname; read_timestamps_1ant2sb <$fname | head -n 1; read_timestamps_1ant2sb <$fname | tail -n 2; printf '\n'; pkill read_timestamps_1ant2sb; done
