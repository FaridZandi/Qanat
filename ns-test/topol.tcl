source "tcp-common-opt.tcl"

set ns [new Simulator]
ns use-scheduler List
set sim_end 100

$ns rtproto DV
Agent/rtProto/DV set advertInterval [expr 2*$sim_end]
Node set multiPath_ 1

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

$t make_tree $g1 2 2
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

$ns at 10 "$t start_migration"

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

#############  Agents ################
set link_rate 0.04
set load 0.25
set meanFlowSize 10
set mean_npkts 10
set pktSize 1400
set connections_per_pair 1
set flowlog './flows.tr'
set S 2
set flow_cdf CDF_dctcp.tcl
set paretoShape 1.05

set lambda [expr ($link_rate*$load*1000000000)/($meanFlowSize*8.0/1460*1500)]
#set lambda [expr ($link_rate*$load*1000000000)/($mean_npkts*($pktSize+40)*8.0)]
puts "Arrival: Poisson with inter-arrival [expr 1/$lambda * 1000] ms"
puts "FlowSize: Pareto with mean = $meanFlowSize, shape = $paretoShape"

puts "Setting up connections ..."; flush stdout

set flow_gen 0
set flow_fin 0

set init_fid 0

set myAgent "Agent/TCP/FullTcp";
set myAgentSink "Agent/TCP/FullTcp";

set agtagr(0,1) [new Agent_Aggr_pair]
$agtagr(0,1) setup $src $n1 "0 1" $connections_per_pair $init_fid "TCP_pair"
$agtagr(0,1) attach-logfile $flowlog

puts -nonewline "(0,1) "
#For Poisson/Pareto
$agtagr(0,1) set_PCarrival_process [expr $lambda/($S - 1)] $flow_cdf [expr 17*1+1244*0] [expr 33*1+4369*0]

$ns at 0.1 "$agtagr(0,1) warmup 0.5 5"
$ns at 1 "$agtagr(0,1) init_schedule"

set init_fid [expr $init_fid + $connections_per_pair];

$ns at $sim_end "finish"
$ns run
