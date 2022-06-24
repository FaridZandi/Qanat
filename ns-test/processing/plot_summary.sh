
pids=""
RESULT=0

python3 processing/plot_summary.py orch_test & 
pids="$pids $!"
python3 processing/plot_summary.py latency_test & 
pids="$pids $!"
python3 processing/plot_summary.py size_test & 
pids="$pids $!"
python3 processing/plot_summary.py parallel_test & 
pids="$pids $!"
python3 processing/plot_summary.py prio_test & 
pids="$pids $!"
python3 processing/plot_summary.py bg_test & 
pids="$pids $!"
python3 processing/plot_summary.py bg_test_2 & 
pids="$pids $!"

for pid in $pids; do
    wait $pid || let "RESULT=1"
done

if [ "$RESULT" == "1" ];
    then
       exit 1
fi



