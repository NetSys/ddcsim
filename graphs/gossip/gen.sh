#!/bin/bash

sim_out=../../saved_output/gossip/pilosim_noconv.WARNING

cat $sim_out | grep "bw in" | cut -c 56- | sed 's/) is \[/,/g' | sed 's/]//g' | python bw.py
cat $sim_out | grep "reachability" | cut -c 62- | sed 's/ is /,/g' | sed 's/\//,/g' | python reach.py > tmp/reach.txt
gnuplot gen.gnuplot
