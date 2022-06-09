# plot everthing  

directory_name=$1

python3 processing/plot_protocol.py -d $directory_name -r

python3 processing/plot_node_stat.py -d $directory_name -r

python3 processing/plot_afct.py -d $directory_name -r 
