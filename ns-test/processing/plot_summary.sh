
pids=""
RESULT=0

python3 processing/plot_summary.py latency_test_heatmap & 
pids="$pids $!"
python3 processing/plot_summary.py parallel_test & 
pids="$pids $!"
python3 processing/plot_summary.py prio_test_bar & 
pids="$pids $!"
python3 processing/plot_summary.py bg_test_2_bar & 
pids="$pids $!"
python3 processing/plot_summary.py bg_test_3_bar & 
pids="$pids $!"
python3 processing/plot_summary.py bg_test_4_bar & 
pids="$pids $!"
python3 processing/plot_summary.py bg_test_5_bar & 
pids="$pids $!"
python3 processing/plot_compare3_cdf_temp.py -d1 exp-cdf/NoMig-10 -d2 exp-cdf/BottomUp-10/ -d3 exp-cdf/TopDown-10/ &
pids="$pids $!"
python3 processing/plot_compare3_cdf.py -d1 prio-exp-cdf/1-level-10 -d2 prio-exp-cdf/2-level-10/ -d3 prio-exp-cdf/3-level-10/ &
pids="$pids $!"

for pid in $pids; do
    wait $pid || let "RESULT=1"
done

if [ "$RESULT" == "1" ];
    then
       exit 1
fi



