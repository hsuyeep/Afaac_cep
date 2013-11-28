#!/bin/bash
#if test $@ -lt 1
#   echo "
#fi

# let s=$1
filesuffix=$1;
filebase=(${filesuffix//./ });
for stat in `seq 2 7`;do
  let s=0
  for f in CS00$stat-RSP?-$filesuffix;do
    echo "Now working on file $f. s = $s";
    for b in `seq 0 11`;do
      # echo "/globalhome/romein/src/split-rsp $b <$f >`dirname $f`/`echo \`basename $f\`|awk -F - '{ print $1 }'`D`printf '%02u' $s`.dip &"
      # /globalhome/romein/src/split-rsp $b <$f >`dirname $f`/`echo \`basename $f\`|awk -F - '{ print $1 }'`D`printf '%02u' $s`.dip& 
      echo "split-rsp-8bit2sb $b <$f >`dirname $f`/`echo \`basename $f\`|awk -F - '{ print $1 }'`D`printf '%02u' $s`_${filebase[0]}.dip &"
      split-rsp-8bit2sb $b <$f >`dirname $f`/`echo \`basename $f\`|awk -F - '{ print $1 }'`D`printf '%02u' $s`_${filebase[0]}.dip &
      let s=s+1
    done;
  done;
done;

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
