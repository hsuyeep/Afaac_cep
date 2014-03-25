#!/bin/bash

let s=$1
for f in /data/AARTFAAC-20131119/CS00?-RSP?-*123504*8b1sbrun3.raw;do
for b in `seq 0 11`;do
  # echo "/globalhome/romein/src/split-rsp $b <$f >`dirname $f`/`echo \`basename $f\`|awk -F - '{ print $1 }'`D`printf '%02u' $s`.dip &"
  # /globalhome/romein/src/split-rsp $b <$f >`dirname $f`/`echo \`basename $f\`|awk -F - '{ print $1 }'`D`printf '%02u' $s`.dip& 
  echo "/globalhome/mol/prasad/24Hr/split-rsp-8bit1sb $b <$f >`dirname $f`/`echo \`basename $f\`|awk -F - '{ print $1 }'`D`printf '%02u' $s`_8bit1sb.dip &"
  /globalhome/mol/prasad/24Hr/split-rsp-8bit1sb $b <$f >`dirname $f`/`echo \`basename $f\`|awk -F - '{ print $1 }'`D`printf '%02u' $s`_8bit1sb.dip & 
  let s=s+1
done

  # Wait for files to reach a given size
#  sleep 2
#  maxfilesize=1073741824;
#  ind=`expr $s - 12`
#  fname=`printf '*D%02u*.dip' $ind`
#  echo $fname
#  currfilesize=`ls -l $fname | awk '{ print $5}'`
#  echo "Current file size: $currfilesize";
#
#  while  [ $currfilesize -le $maxfilesize ] 
#  do
#    currfilesize=`ls -l $fname | awk '{ print $5}'`
#    echo "Current file size: $currfilesize";
#    sleep 3;
#  done
#  echo "File size exceeded, killing processes"
#  pkill split-rsp;
#  sleep 2;
  
# wait
done
