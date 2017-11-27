set terminal png size 720,400
set output 'PercentileQueueLength.png'

set ylabel "Percentile Queue Length"

set autoscale

set xlabel "time"

set title "TCP Reno and Tahoe: Percentile Queue Length vs time"
set key reverse Left outside

set style data linespoints

plot "a-test-reno-BANDWIDTH-3_p.dat" using 1:2 lt 1 lc rgb "orange" title "Reno bandwidth 3", \
"a-test-reno-BANDWIDTH-5_p.dat" using 1:2 lt 1 lc rgb "red" title "Reno bandwidth 5", \
"a-test-reno-BANDWIDTH-10_p.dat" using 1:2 lt 1 lc rgb "blue" title "Reno bandwidth 10", \
"a-test-reno-BANDWIDTH-20_p.dat"  using 1:2 lt 1 lc rgb "green" title "Reno bandwidth 20", \
"a-test-tahoe-BANDWIDTH-3_p.dat" using 1:2 lt 2 lc rgb "orange" title "Tahoe bandwidth 3", \
"a-test-tahoe-BANDWIDTH-5_p.dat" using 1:2 lt 2 lc rgb "red" title "Tahoe bandwidth 5", \
"a-test-tahoe-BANDWIDTH-10_p.dat" using 1:2 lt 2 lc rgb "blue" title "Tahoe bandwidth 10", \
"a-test-tahoe-BANDWIDTH-20_p.dat"  using 1:2 lt 2 lc rgb "green" title "Tahoe bandwidth 20"
