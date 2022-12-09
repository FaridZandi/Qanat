
pids=""
RESULT=0

python3 processing/plot_summary_noms.py parallel_test & 
pids="$pids $!"
python3 processing/plot_summary_noms.py prio_test & 
pids="$pids $!"
python3 processing/plot_summary_noms.py bg_test & 
pids="$pids $!"
python3 processing/plot_summary_noms.py bg_test_no_legend & 
pids="$pids $!"
python3 processing/plot_summary_noms.py bg_test_log & 
pids="$pids $!"
python3 processing/plot_summary_noms.py bg_test_no_legend_log & 
pids="$pids $!"
python3 processing/plot_compare3_cdf_temp.py -d1 exps/exp-cdf/NoMig-10 -d2 exps/exp-cdf/BottomUp-10/ -d3 exps/exp-cdf/TopDown-10/ &
pids="$pids $!"
python3 processing/plot_compare3_cdf.py -d1 exps/prio-exp-cdf/1-level-10 -d2 exps/prio-exp-cdf/2-level-10/ -d3 exps/prio-exp-cdf/3-level-10/ &
pids="$pids $!"

for pid in $pids; do
    wait $pid || let "RESULT=1"
done

if [ "$RESULT" == "1" ];
    then
       exit 1
fi



