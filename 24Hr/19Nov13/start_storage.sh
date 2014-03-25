#/bin/bash

DATE=`date +%Y%m%d`
OBS=`date +%Y%m%d-%H%M%S`
OBSMODE=$1
PORT=4346
touch $OBS-$OBSMODE.meta
touch $OBS-$OBSMODE.copy
touch $OBS-$OBSMODE.split
for STATION in CS002 CS003 CS004 CS005 CS006 CS007
do
  for RSP in 0 1 2 3
  do
    NODE=`grep LANE_0${RSP}_DSTIP RSPDriver.conf.$STATION | awk '{ print $3; }'`
    if [ $RSP -eq 0 ]; then
	AGGREGATOR_NODE=$NODE
    fi

    RSPPORT=`echo $PORT+$RSP | bc -l`
    echo starting udp-copy for $STATION RSP $RSP on locus node $NODE
    ssh $NODE mkdir -p /data/AARTFAAC-$DATE
    # ssh $NODE numactl -C 0-11 /globalhome/romein/bin.x86_64/udp-copy 0:$RSPPORT /data/AARTFAAC-$DATE/$STATION-RSP$RSP-$OBS-$OBSMODE.raw &
    ssh $NODE numactl -C 0-11 /globalhome/mol/prasad/24Hr/udp-copy-8bit-2sb 0:$RSPPORT /data/AARTFAAC-$DATE/$STATION-RSP$RSP-$OBS-$OBSMODE.raw &
    # echo "ssh $NODE numactl -C 0-11 /globalhome/romein/bin.x86_64/udp-copy 0:$RSPPORT /data/AARTFAAC-$DATE/$STATION-RSP$RSP-$OBS-$OBSMODE.raw &"
    #########################
    # Generate script to test location of every raw data file
    echo "ssh $NODE ls -lh /data/AARTFAAC-$DATE/$STATION-RSP$RSP-$OBS-$OBSMODE.raw" >> $OBS-$OBSMODE.meta 
    #########################

    #########################
    # Generate script to enable copying of a single station's data to a common 
    # location, for splitting into per-dipole data
    if [ $RSP -ne 0 ]; then
     echo "rsync -avz $NODE:/data/AARTFAAC-$DATE/$STATION-RSP$RSP-$OBS-$OBSMODE.raw $AGGREGATOR_NODE:/data/AARTFAAC-$DATE/&" >> $OBS-$OBSMODE.copy
    else
      echo "# NODE $AGGREGATOR_NODE is the aggregator" >>  $OBS-$OBSMODE.copy
    fi
    #########################

    #########################
    # Generate script to enable copying of each RSP board output to fs5
    # for splitting into per-dipole data
    echo "ssh $NODE scp -p /data/AARTFAAC-$DATE/$STATION-RSP$RSP-$OBS-$OBSMODE.raw pprasad@10.149.5.254:/var/scratch/pprasad/19Nov13/" >> $OBS-$OBSMODE.fs5
    #########################
    
    #########################
    # Generate script to split the data into per dipole streams, post copying
    echo "scp split-dipole.sh $AGGREGATOR_NODE:/data/AARTFAAC-$DATE/" >> $OBS-$OBSMODE.split
    echo "ssh $AGGREGATOR_NODE /data/AARTFAAC-$DATE/split-dipole.sh" >> $OBS-$OBSMODE.split
    #########################
    
  done
done
    echo "ssh $AGGREGATOR_NODE watch ls -ltrh /data/AARTFAAC-$DATE/" >> $OBS-$OBSMODE.copy
