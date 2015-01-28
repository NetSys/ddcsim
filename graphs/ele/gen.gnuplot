set datafile separator ","

set terminal pdfcairo font "Gill Sans,9" linewidth 4 rounded fontscale 1.0

# Line style for axes
set style line 80 lt rgb "#808080"

# Line style for grid
set style line 81 lt 0  # dashed
set style line 81 lt rgb "#808080"  # grey
set grid back linestyle 81

# Remove border on top and right.  These borders are useless and make it harder
# to see plotted lines near the border. Also, put it in grey; no need for so
# much emphasis on a border.
set border 3 back linestyle 80
set xtics nomirror rotate
set ytics nomirror

set title "Total Bandwidth Usage"
set ylabel "GB"
set xlabel "Time (hh:mm)"
set xdata time
set timefmt "%s"
set key off
set output "out/total_bw.pdf"

plot "tmp/total_bw.txt" using 1:2 with lines lw 1

set title "Bandwidth Used by Link State Requests"
set ylabel "MB"
set xlabel "Time (hh:mm)"
set xdata time
set timefmt "%s"
set key off
set output "out/lsr_bw.pdf"

plot "tmp/lsr_bw.txt" using 1:2 with lines lw 1

set title "Bandwidth Used by LS Updates Caused by LS Requests"
set ylabel "GB"
set xlabel "Time (hh:mm)"
set xdata time
set timefmt "%s"
set key off
set output "out/lsu_from_lsr.pdf"

plot "tmp/lsu_from_lsr_bw.txt" using 1:2 with lines lw 1

set title "Reachability"
set ylabel "Fraction of Host Pairs"
set xlabel "Time (hh:mm)"
set xdata time
set timefmt "%s"
set key off
set output "out/reach.pdf"

plot "tmp/reach.txt" using 1:2 with lines lw 1