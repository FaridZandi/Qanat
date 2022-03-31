set ns [new Simulator]
ns use-scheduler List
set sim_end 2

#$ns rtproto DV
#Agent/rtProto/DV set advertInterval [expr 2*$sim_end]
Node set multiPath_ 1
Agent/TCP/FullTcp set segsize_ 1400

set tf [open out.tr w]
$ns trace-all $tf

proc finish {} {
        puts "simulation finished"
        global ns tf
        $ns flush-trace
        close $tf
        exit 0
}

# Set up the topology
set t [new MyTopology]
$t set_simulator $ns

set src [$t make_node]

set f1 [$t make_node]
set f2 [$t make_node]
set g1 [$t make_node]
set g2 [$t make_node]

$t add_child $f1 $src
$t add_child $f1 $g1
$t add_child $f1 $g2
$t add_child $f2 $g1
$t add_child $f2 $g2

$t make_tree $g1 1 1
$t duplicate_tree $g1 $g2
$t set_mig_root $g1
$t set_traffic_src $src
$t print_graph



# set up the apps, agents, etc. 

$t setup_apps
# test

set n1 [$t find_node $g1 1 1]
set n3 [$t find_node $g2 1 1]

# puts [ $n1 address? ]

# $ns at 0.4 "$t connect_agents $src $n1"
# $ns at 0.5 "$t send_data $src $n1"
# $ns at 0.55 "$t send_data $src $n1"
# $ns at 0.6 "$t send_data $src $n1"
# $ns at 0.65 "$t send_data $src $n1"
# $ns at 0.70 "$t send_data $src $n1"
# $ns at 0.8 "$t send_data $src $n1"
# $ns at 0.85 "$t send_data $src $n1"
# $ns at 0.9 "$t send_data $src $n1"
# $ns at 0.95 "$t send_data $src $n1"

# $ns at 0.4 "$t connect_agents $n1 $n3"
# $ns at 0.6 "$t send_data $n1 $n3"
$ns at 1 "$t start_migration"

# $ns at 1 "$t send_data $src $n1"  
# $ns at 1.05 "$t send_data $src $n1"
# $ns at 1.1 "$t send_data $src $n1"
# $ns at 1.15 "$t send_data $src $n1"
# $ns at 1.2 "$t send_data $src $n1"
# $ns at 1.25 "$t send_data $src $n1"
# $ns at 1.3 "$t send_data $src $n1"
# $ns at 1.35 "$t send_data $src $n1"
# $ns at 1.4 "$t send_data $src $n1"
# $ns at 1.45 "$t send_data $src $n1"
# $ns at 1.5 "$t send_data $src $n1"
# $ns at 1.55 "$t send_data $src $n1"
# $ns at 1.6 "$t send_data $src $n1"
# $ns at 1.65 "$t send_data $src $n1"
# $ns at 1.7 "$t send_data $src $n1"
# $ns at 1.75 "$t send_data $src $n1"

$ns at $sim_end "finish"
$ns run


