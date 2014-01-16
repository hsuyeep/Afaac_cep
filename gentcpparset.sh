#!/bin/bash
# Script to generate an AARTFAAC correlation parset with data coming over tcp.
# pep/16Jan14

startport=4567;
for stat in `seq 2 7`; do 
  for dip in `seq 0 47`; do
	  let "statoff=$stat-2";
      let "offset = $statoff*48 + $dip";
	  let "testport= $startport + $offset";
	  # check if testport is in use. NOTE: User needs to check!
	  found=`netstat -lntp | egrep $testport`;
  	  printf 'PIC.Core.Station.CS00%dD%02d.RSP.ports = [tcp:0:%d]\n' $stat $dip $testport;
  done;
done; 
