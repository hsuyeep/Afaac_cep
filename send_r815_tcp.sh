#!/bin/bash
# Script to send per dipole data over tcp to r815 on the port specified in the 
# parset file.
# pep/16Jan14

startport=4567
fname=(`ls *.dip`);
for stat in `seq 2 7`; do 
  for dip in `seq 0 47`; do
	  let "statoff=$stat-2";
      let "offset = $statoff*48 + $dip";
	  let "testport= $startport + $offset";
	  # check if testport is in use. NOTE: User needs to check!
	  printf 'Sending file %s to r815 port %d\n' ${fname[$offset]} $testport;
      # nc r815 $testport < ${fname[$offset]} &
	  /home/romein/src/udp-copy file:${fname[$offset]} tcp:r815:$testport&
  	  printf 'PIC.Core.Station.CS00%dD%02d.RSP.ports = [tcp:0:%d]\n' $stat $dip $testport;
  done;
done; 
