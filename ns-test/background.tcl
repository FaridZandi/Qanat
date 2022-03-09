source "tcp-common-opt.tcl"

set ns [new Simulator]
set sim_start [clock seconds]

$ns rtproto DV
Agent/rtProto/DV set advertInterval 1
Node set multiPath_ 1

set tf [open out.tr w]
$ns trace-all $tf

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

set n1 [$t find_node $g1 1 1]
set n1P [$t find_node $g2 1 1]


set flow_gen 0
set flow_fin 0

set init_fid 0

set sim_end 10
set connections_per_pair 1
set meanFlowSize 1138*1460
set flow_cdf 'CDF_dctcp.tcl'
set link_rate 0.1
set meanFlowSize 1138*1460

# BEGIN{only one pair for now}
set init_fid 1
set flowlog './flow.tr'

set agtagr(0,1) [new Agent_Aggr_pair]
puts "Setting up connections ..."; flush stdout

set myAgent "Agent/TCP/FullTcp";
set myAgentSink "Agent/TCP/FullTcp";

$agtagr(0,1) setup $n1 $src "0 1" $connections_per_pair $init_fid "TCP_pair"
puts "Log attched ..."; flush stdout
$agtagr(0,1) attach-logfile $flowlog
puts -nonewline "(0,1) "
#For Poisson/Pareto
set S 2
set load 0.5
set lambda [expr ($link_rate*$load*1000000000)/($meanFlowSize*8.0/1460*1500)]
$agtagr(0,1) set_PCarrival_process [expr $lambda/($S - 1)] $flow_cdf [expr 17*0+1244*1] [expr 33*0+4369*1]

$ns at 0.1 "$agtagr(0,1) warmup 0.5 5"
$ns at 1 "$agtagr(0,1) init_schedule"

set init_fid [expr $init_fid + $connections_per_pair];
# END{only one pair for now}

puts "Initial agent creation done";flush stdout
puts "Simulation started!"

$ns run