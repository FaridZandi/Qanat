# plot everthing  

directory_name=$1

python3 processing/plot_porotocol.py -f $directory_name -r

python3 processing/plot_node_stat.py -f $directory_name -r

python3 processing/plot_afct.py -f $directory_name -r 
