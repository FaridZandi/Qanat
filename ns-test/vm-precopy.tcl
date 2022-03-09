source "tcp-common-opt.tcl"

set src_node [lindex $argv 0]
set dst_node [lindex $argv 1]


#############  Agents ################
set link_rate 0.04
set load 0.25
set meanFlowSize 10
set mean_npkts 10
set pktSize 1400
set connections_per_pair 1
set flowlog './single.tr'
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

set agtagr($src_node,$dst_node) [new Agent_Aggr_pair]
$agtagr($src_node,$dst_node) setup $src_node $dst_node "$src_node $dst_node" $connections_per_pair $init_fid "TCP_pair"
$agtagr($src_node,$dst_node) attach-logfile $flowlog

#For Poisson/Pareto
$agtagr($src_node,$dst_node) set_PCarrival_process [expr $lambda/($S - 1)] $flow_cdf [expr 17*1+1244*0] [expr 33*1+4369*0]

$ns at 0.1 "$agtagr($src_node,$dst_node) warmup 0.5 5"
set n_packets 10
$ns at [$ns now] "$agtagr($src_node,$dst_node) send_single_flow $n_packets"