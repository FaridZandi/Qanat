source "tcp-common-opt.tcl"

set ns [new Simulator]
set sim_start [clock seconds]

if {$argc != 43} {
    puts "wrong number of arguments $argc"
    exit 0
}


set tf [open out.tr w]
$ns trace-all $tf

set sim_end [lindex $argv 0]
set link_rate [lindex $argv 1]
set mean_link_delay [lindex $argv 2]
set host_delay [lindex $argv 3]
set queueSize [lindex $argv 4]
set load [lindex $argv 5]
set connections_per_pair [lindex $argv 6]
set meanFlowSize [lindex $argv 7]
set paretoShape [lindex $argv 8]
set flow_cdf [lindex $argv 9]

#### Multipath
set enableMultiPath [lindex $argv 10]
set perflowMP [lindex $argv 11]

#### Transport settings options
set sourceAlg [lindex $argv 12] ; # Sack or DCTCP-Sack
set initWindow [lindex $argv 13]
set ackRatio [lindex $argv 14]
set slowstartrestart [lindex $argv 15]
set DCTCP_g [lindex $argv 16] ; # DCTCP alpha estimation gain
set min_rto [lindex $argv 17]
set prob_cap_ [lindex $argv 18] ; # Threshold of consecutive timeouts to trigger probe mode

#### Switch side options
set switchAlg [lindex $argv 19] ; # DropTail (pFabric), RED (DCTCP) or Priority (PIAS)
set DCTCP_K [lindex $argv 20]
set drop_prio_ [lindex $argv 21]
set prio_scheme_ [lindex $argv 22]
set deque_prio_ [lindex $argv 23]
set keep_order_ [lindex $argv 24]
set prio_num_ [lindex $argv 25]
set ECN_scheme_ [lindex $argv 26]
set pias_thresh_0 [lindex $argv 27]
set pias_thresh_1 [lindex $argv 28]
set pias_thresh_2 [lindex $argv 29]
set pias_thresh_3 [lindex $argv 30]
set pias_thresh_4 [lindex $argv 31]
set pias_thresh_5 [lindex $argv 32]
set pias_thresh_6 [lindex $argv 33]

#### topology
set topology_spt [lindex $argv 34]
set topology_tors [lindex $argv 35]
set topology_spines [lindex $argv 36]
set topology_x [lindex $argv 37]

#### migration params 
set vm_precopy_size [lindex $argv 38]
set vm_snapshot_size [lindex $argv 39]
set gw_snapshot_size [lindex $argv 40]
set parallel_mig [lindex $argv 41]

### result file
set flowlog [open [lindex $argv 42] w]

#### Packet size is in bytes.
set pktSize 1460
#### trace frequency
set queueSamplingInterval 0.0001
#set queueSamplingInterval 1

puts "Simulation input:"
puts "Dynamic Flow - Pareto"
puts "topology: spines server per rack = $topology_spt, x = $topology_x"
puts "sim_end $sim_end"
puts "link_rate $link_rate Gbps"
puts "link_delay $mean_link_delay sec"
puts "host_delay $host_delay sec"
puts "queue size $queueSize pkts"
puts "load $load"
puts "connections_per_pair $connections_per_pair"
puts "enableMultiPath=$enableMultiPath, perflowMP=$perflowMP"
puts "source algorithm: $sourceAlg"
puts "TCP initial window: $initWindow"
puts "ackRatio $ackRatio"
puts "DCTCP_g $DCTCP_g"
puts "slow-start Restart $slowstartrestart"
puts "switch algorithm $switchAlg"
puts "DCTCP_K_ $DCTCP_K"
puts "pktSize(payload) $pktSize Bytes"
puts "pktSize(include header) [expr $pktSize + 40] Bytes"
puts "vm precopy size $vm_precopy_size Bytes"
puts "vm snapshot size $vm_snapshot_size Bytes"
puts "GW snapshot size $gw_snapshot_size Bytes"
puts "Number of parallel VMs to migrate $parallel_mig"

puts " "

################# Transport Options ####################
Agent/TCP set ecn_ 1
Agent/TCP set old_ecn_ 1
Agent/TCP set packetSize_ $pktSize
Agent/TCP/FullTcp set segsize_ $pktSize
Agent/TCP/FullTcp set spa_thresh_ 0
Agent/TCP set slow_start_restart_ $slowstartrestart
Agent/TCP set windowOption_ 0
Agent/TCP set minrto_ $min_rto
Agent/TCP set tcpTick_ 0.000001
Agent/TCP set maxrto_ 64
Agent/TCP set lldct_w_min_ 0.125
Agent/TCP set lldct_w_max_ 2.5
Agent/TCP set lldct_size_min_ 204800
Agent/TCP set lldct_size_max_ 1048576

Agent/TCP/FullTcp set nodelay_ true; # disable Nagle
Agent/TCP/FullTcp set segsperack_ $ackRatio;
Agent/TCP/FullTcp set interval_ 0.000006

if {$ackRatio > 2} {
    Agent/TCP/FullTcp set spa_thresh_ [expr ($ackRatio - 1) * $pktSize]
}

if {[string compare $sourceAlg "DCTCP-Sack"] == 0} {
    Agent/TCP set ecnhat_ true
    Agent/TCPSink set ecnhat_ true
    Agent/TCP set ecnhat_g_ $DCTCP_g
    Agent/TCP set lldct_ false

} elseif {[string compare $sourceAlg "LLDCT-Sack"] == 0} {
    Agent/TCP set ecnhat_ true
    Agent/TCPSink set ecnhat_ true
    Agent/TCP set ecnhat_g_ $DCTCP_g;
    Agent/TCP set lldct_ true
} elseif {[string compare $sourceAlg "TCP"] == 0} {
    Agent/TCP set ecnhat_ false
    Agent/TCPSink set ecnhat_ false
}

#Shuang
Agent/TCP/FullTcp set dynamic_dupack_ 0; #disable dupack
Agent/TCP set window_ 100
Agent/TCP set windowInit_ $initWindow
Agent/TCP set rtxcur_init_ $min_rto;

################# Switch Options ######################
Queue set limit_ $queueSize

Queue/DropTail set queue_in_bytes_ true
Queue/DropTail set mean_pktsize_ [expr $pktSize+40]
Queue/DropTail set drop_prio_ $drop_prio_
Queue/DropTail set deque_prio_ $deque_prio_
Queue/DropTail set keep_order_ $keep_order_

Queue/RED set bytes_ false
Queue/RED set queue_in_bytes_ true
Queue/RED set mean_pktsize_ [expr $pktSize+40]
Queue/RED set setbit_ true
Queue/RED set gentle_ false
Queue/RED set q_weight_ 1.0
Queue/RED set mark_p_ 1.0
Queue/RED set thresh_ $DCTCP_K
Queue/RED set maxthresh_ $DCTCP_K
Queue/RED set drop_prio_ $drop_prio_
Queue/RED set deque_prio_ $deque_prio_

# Queue/Priority set queue_num_ $prio_num_
# Queue/Priority set thresh_ $DCTCP_K
# Queue/Priority set mean_pktsize_ [expr $pktSize+40]
# Queue/Priority set marking_scheme_ $ECN_scheme_

############## Multipathing ###########################
if {$enableMultiPath == 1} {
    $ns rtproto DV
    Agent/rtProto/DV set advertInterval	[expr 2*$sim_end]
    Node set multiPath_ 1
    if {$perflowMP != 0} {
        Classifier/MultiPath set perflow_ 1
        Agent/TCP/FullTcp set dynamic_dupack_ 0; # enable duplicate ACK
    }
}

set myAgent "Agent/TCP/FullTcp";

############# Topoplgy #########################
set S [expr $topology_spt * $topology_tors] ; #number of servers
set UCap [expr $link_rate * $topology_spt / $topology_spines / $topology_x] ; #uplink rate

puts "UCap: $UCap"

for {set i 0} {$i < $S} {incr i} {
    set s($i) [$ns node]
}

## destination servers
for {set i 0} {$i < $topology_spt} {incr i} {
    set ds($i) [$ns node]
}

## one additional ToR switch at the destination zone
for {set i 0} {$i < [expr $topology_tors + 1]} {incr i} {
    set n($i) [$ns node]
}

## one additional spine switch at the destination zone
for {set i 0} {$i < [expr $topology_spines + 1]} {incr i} {
    set a($i) [$ns node]
}

$ns duplex-link $a($topology_spines) $a([expr $topology_spines-1]) [set UCap]Gb $mean_link_delay $switchAlg

############ Edge links ##############
for {set i 0} {$i < $S} {incr i} {
    set j [expr $i/$topology_spt]
    $ns duplex-link $s($i) $n($j) [set link_rate]Gb [expr $host_delay + $mean_link_delay] $switchAlg
}

## destination-side links between the ToR and servers
for {set i 0} {$i < $topology_spt} {incr i} {
    $ns duplex-link $ds($i) $n($topology_tors) [set link_rate]Gb [expr $host_delay + $mean_link_delay] $switchAlg
}

############ Core links ##############
for {set i 0} {$i < [expr $topology_tors + 1]} {incr i} {
    for {set j 0} {$j < [expr $topology_spines + 1]} {incr j} {
        $ns duplex-link $n($i) $a($j) [set UCap]Gb $mean_link_delay $switchAlg
    }
}

############# Tree Structure ##############
set tf [open out.tr w]
MyTopology set verbose_ 0
MyTopology set verbose_nf_ 0
MyTopology set verbose_mig_ 0
MyTopology set vm_precopy_size_ $vm_precopy_size
MyTopology set vm_snapshot_size_ $vm_snapshot_size
MyTopology set gw_snapshot_size_ $gw_snapshot_size
MyTopology set parallel_mig_ $parallel_mig


set t [new MyTopology]

$t set_simulator $ns

set child_count 20
set VM_link_rate 10

for { set x 0} { $x < $child_count} { incr x } {
    set ss($x) [$ns node]
    $ns duplex-link $ss($x) $s(0) [set VM_link_rate]Gb [expr $host_delay] $switchAlg
    $t add_node_to_source $ss($x)
}

for { set x 0} { $x < $child_count} { incr x } {
    set dvm($x) [$ns node]
    $ns duplex-link $dvm($x) $ds(0) [set VM_link_rate]Gb [expr $host_delay] $switchAlg
    $t add_node_to_dest $dvm($x)
}

$t make_tree 1 4
# $t duplicate_tree
# $t print_graph

set logical_leaves {}
set logical_leaves_counter 0; 
while {1} {
    set this_leaf [$t get_logical_leaf $logical_leaves_counter]
    if {$this_leaf == -1} {
        break 
    } 
    lappend logical_leaves $this_leaf;
    set logical_leaves_counter [expr {$logical_leaves_counter + 1}];
}


$ns at 0.01 "$t setup_nodes"
# $ns at 1 "$t start_migration"

#############  Agents ################
set lambda [expr ($link_rate*$load*1000000000)/($meanFlowSize*8.0/1460*1500)]
#set lambda [expr ($link_rate*$load*1000000000)/($mean_npkts*($pktSize+40)*8.0)]
puts "Arrival: Poisson with inter-arrival [expr 1/$lambda * 1000] ms"
puts "FlowSize: Pareto with mean = $meanFlowSize, shape = $paretoShape"

puts "Setting up connections ..."; flush stdout

set flow_gen 0
set flow_fin 0

set init_fid 0
# for {set j 1} {$j < $S } {incr j} {
#     for {set i 1} {$i < $S } {incr i} {
#         if {$i != $j} {
#                 set agtagr($i,$j) [new Agent_Aggr_pair]
#                 $agtagr($i,$j) setup $s($i) $s($j) "$i $j" $connections_per_pair $init_fid "TCP_pair"
#                 $agtagr($i,$j) attach-logfile $flowlog

#                 # puts -nonewline "($i,$j) "
#                 #For Poisson/Pareto
#                 $agtagr($i,$j) set_PCarrival_process [expr $lambda/($S - 1)] $flow_cdf [expr 17*$i+1244*$j] [expr 33*$i+4369*$j]

#                 $ns at 0.1 "$agtagr($i,$j) warmup 0.5 5"
#                 $ns at 1 "$agtagr($i,$j) init_schedule"

#                 set init_fid [expr $init_fid + $connections_per_pair];
#             }
#         }
# }

# set init_fid 0
set j 0 
foreach leaf $logical_leaves {
    puts $leaf
    for {set i 4} {$i < 8} {incr i} {
        if {$i != $j} {
            set agtagr($i,$j) [new Agent_Aggr_pair]
            $agtagr($i,$j) setup $s($i) $leaf "$i $j" $connections_per_pair $init_fid "TCP_pair"
            $agtagr($i,$j) attach-logfile $flowlog

            # puts -nonewline "($i,$j) "
            #For Poisson/Pareto
            $agtagr($i,$j) set_PCarrival_process [expr $lambda/($S - 1)] $flow_cdf [expr 17*$i+1244*$j] [expr 33*$i+4369*$j]

            $ns at 0.1 "$agtagr($i,$j) warmup 0.5 5"
            $ns at 1 "$agtagr($i,$j) init_schedule"

            set init_fid [expr $init_fid + $connections_per_pair];
        }
    }
    set j [expr {$j + 1}];
    # break 
}


# set init_j 10000
# set j $init_j
# foreach vm $logical_leaves {  
#     for {set i $topology_spt} {$i < [expr 2*$topology_spt] } {incr i} {
#         set agtagr($i,$j) [new Agent_Aggr_pair]
#         $agtagr($i,$j) setup $s($i) $vm "$i $j" $connections_per_pair $init_fid "TCP_pair"
#         $agtagr($i,$j) attach-logfile $flowlog

#         # puts -nonewline "($i,$j) "
#         #For Poisson/Pareto
#         # multiply lambda by 8 to account for 8 VMs on a physical host
#         $agtagr($i,$j) set_PCarrival_process [expr $lambda*8/($S - 1)] $flow_cdf [expr 17*$i+1244*($j-$init_j)] [expr 33*$i+4369*($j-$init_j)]

#         $ns at 0.1 "$agtagr($i,$j) warmup 0.5 5"
#         $ns at 1 "$agtagr($i,$j) init_schedule"

#         set init_fid [expr $init_fid + $connections_per_pair];
#     }    
#     set j [expr {$j + 1}]; 
# }

puts "Initial agent creation done";flush stdout
puts "Simulation started!"

proc finish {} {
        puts "simulation finished"
        global ns tf
        $ns flush-trace
        close $tf
        exit 0
}

$ns at $sim_end "finish"

$ns run