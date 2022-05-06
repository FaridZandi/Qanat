for f in results/*; do 
    echo "$f"; 
    cd $f; 
    # tail -n 2 $f/logFile.tr;
    cat logFile.tr | grep flowstart | sort > flowstart.log
    cat logFile.tr | grep flowend | sort > flowend.log
    python3 ../../fct.py
    cd -; 
done