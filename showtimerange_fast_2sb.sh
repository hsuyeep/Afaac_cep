#!/bin/bash
for i in `seq 2 7`; do fname=`printf '%s/CS00%dD12*.dip\n' $1 $i`; echo $fname; read_fast_timestamps $fname;  done
