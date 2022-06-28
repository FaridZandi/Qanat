pids=""
RESULT=0

python3 processing/summarize_exp.py -d exps/orch_test/ & 
pids="$pids $!"
python3 processing/summarize_exp.py -d exps/parallel_test/ & 
pids="$pids $!"
python3 processing/summarize_exp.py -d exps/prio_test/ & 
pids="$pids $!"
python3 processing/summarize_exp.py -d exps/bg_test/ & 
pids="$pids $!"
python3 processing/summarize_exp.py -d exps/size_test/ & 
pids="$pids $!"
python3 processing/summarize_exp.py -d exps/latency_test/ & 
pids="$pids $!"


for pid in $pids; do
    wait $pid || let "RESULT=1"
done

if [ "$RESULT" == "1" ];
    then
       exit 1
fi


