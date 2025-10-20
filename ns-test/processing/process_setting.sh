# plot everthing  

directory_name=$1

# noplotflag="-n"
noplotflag=""

python3 processing/plot_afct.py -d $directory_name -r $noplotflag

python3 processing/plot_protocol.py -d $directory_name -r $noplotflag   

python3 processing/plot_node_stat.py -d $directory_name -r $noplotflag

python3 processing/plot_flow_stat.py -d $directory_name -r $noplotflag

python3 processing/plot_flow_cdf.py -d $directory_name -r $noplotflag

python3 processing/plot_tunnelled_packets.py -d $directory_name -r $noplotflag
