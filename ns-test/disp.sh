for f in results/*; do 
    echo "$f"; 
    cd $f; 
    # tail -n 2 $f/logFile.tr;
    cat logFile.tr | grep flowstart | sort > flowstart.log
    cat logFile.tr | grep flowend | sort > flowend.log
    cat logFile.tr | grep retrans | sort > retrans.log
    python3 ../../fct.py
    python3 ../../retrans.py
    cd ../..; 
done