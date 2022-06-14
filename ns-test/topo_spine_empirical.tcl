source "tcp-common-opt.tcl"

set ns [new Simulator]
set sim_start [clock seconds]

if {$argc != 55} {
    puts "wrong number of arguments $argc"
    exit 0
}


# set tf [open out.tr w]
# $ns trace-all $tf

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
set run_migration [lindex $argv 42]
set stat_record_interval [lindex $argv 43]
set orch_type [lindex $argv 44]
set prioritization [lindex $argv 45]
set src_zone_delay [lindex $argv 46]
set dst_zone_delay [lindex $argv 47]
set enable_bg_traffic [lindex $argv 48]
set vm_flow_size [lindex $argv 49]
set enable_rt_dv [lindex $argv 50]
set b_factor_1 [lindex $argv 51]
set b_factor_2 [lindex $argv 52]
set b_factor_3 [lindex $argv 53]


### result file
set flowlog [open [lindex $argv 54] w]

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

if {[string compare $sourceAlg "DCTCP"] == 0} {
    Agent/TCP set ecnhat_ true
    Agent/TCPSink set ecnhat_ true
    Agent/TCP set ecnhat_g_ $DCTCP_g
    Agent/TCP set lldct_ false
    
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
    if {$enable_rt_dv == 1} {
        puts "setting RT-DV"
        $ns rtproto DV
    } else {
        puts "not setting RT-DV"
    }
    Agent/rtProto/DV set advertInterval	[expr 2*$sim_end]
    Node set multiPath_ 1
    if {$perflowMP != 0} {
        Classifier/MultiPath set perflow_ 1
        Agent/TCP/FullTcp set dynamic_dupack_ 0; # enable duplicate ACK
    }
}

set myAgent "Agent/TCP/FullTcp";



############# Tree Structure ##############
MyTopology set verbose_ 1
MyTopology set verbose_nf_ 0
MyTopology set verbose_mig_ 0
MyTopology set vm_precopy_size_ $vm_precopy_size
MyTopology set vm_snapshot_size_ $vm_snapshot_size
MyTopology set gw_snapshot_size_ $gw_snapshot_size
MyTopology set parallel_mig_ $parallel_mig
MyTopology set stat_record_interval_ $stat_record_interval
MyTopology set orch_type_ $orch_type
MyTopology set prioritization_level_ $prioritization
MyTopology set process_after_migration_ 0

puts "setting stat record interval: $stat_record_interval"

set t [new MyTopology]
$t set_simulator $ns
set vm_link_rate 10

############# Topoplgy #########################

puts "servers per rack = $topology_spt"
puts "total number of racks = $topology_tors"
puts "total number of spines = $topology_spines"

set logical_node_count [expr {1 + $b_factor_1 + $b_factor_1 * $b_factor_2 + $b_factor_1 * $b_factor_2 * $b_factor_3}]
puts "logical_node_count: $logical_node_count"


set mig_zone_tors [expr {($logical_node_count + ($topology_spt - 1)) / $topology_spt}]
puts "source_zone_tors: $mig_zone_tors"

if {$mig_zone_tors > 1} {
    puts "---------------------------------------------------"
    puts "ERROR: migration zone does not fit in a single rack"
    puts "increase the size of the rack or decrease the size of the logical node count"
    exit
}

set needed_tors [expr {$topology_tors + 3}]
puts "topology_tors: $topology_tors"
puts "needed_tors: $needed_tors"

set bg_tor_id_start [expr {0}]
set src_zone_tor_id [expr $topology_tors + 0]
set dst_zone_tor_id [expr $topology_tors + 1]
set vm_traffic_tor_id [expr $topology_tors + 2]

puts "background traffic ToR starting id : $bg_tor_id_start"
puts "VM traffic ToR id : $vm_traffic_tor_id"
puts "source zone ToR id : $src_zone_tor_id"
puts "destination zone ToR id : $dst_zone_tor_id"

set UCap [expr $link_rate * $topology_spt / $topology_spines / $topology_x] ; #uplink rate
puts "UCap: $UCap"

set background_servers [expr $topology_spt * $topology_tors] ; #number of servers
puts "background_servers: $background_servers"

set total_servers [expr $topology_spt * $needed_tors] ; #total number of servers
puts "total_servers: $total_servers"

set needed_vms [expr $topology_spt * 2] ; #number of VMs
puts "needed_vms: $needed_vms"

for {set i 0} {$i < $total_servers} {incr i} {
    set s($i) [$ns node]
}

for {set i 0} {$i < [expr $needed_tors]} {incr i} {
    set n($i) [$ns node]
}

for {set i 0} {$i < [expr $topology_spines]} {incr i} {
    set a($i) [$ns node]
}

for {set i 0} {$i < [expr $needed_vms]} {incr i} {
    set v($i) [$ns node]
}



############ Core links ##############

set spine_link_delay 0

for {set i 0} {$i < [expr $needed_tors]} {incr i} {
    if {$i == $dst_zone_tor_id} {
        set spine_link_delay [expr $dst_zone_delay]
    } elseif { $i == $src_zone_tor_id } {
        set spine_link_delay [expr $src_zone_delay]
    } else {
        set spine_link_delay [expr $mean_link_delay]
    }

    for {set j 0} {$j < [expr $topology_spines]} {incr j} {
        $ns duplex-link $n($i) $a($j) [set UCap]Gb $mean_link_delay $switchAlg
        puts "--------link tor:[expr $i + 1] to spine:[expr $j + 1] is created with delay $spine_link_delay"
    }
}

############ Edge links ##############
for {set i 0} {$i < $total_servers} {incr i} {
    set j [expr $i/$topology_spt]
    $ns duplex-link $s($i) $n($j) [set link_rate]Gb [expr $host_delay + $mean_link_delay] $switchAlg
    puts "--------link server:[expr $i + 1] to tor:[expr $j + 1] is created"
}

############ VM links ##############

set vm_counter 0 
for {set j 0} { $j < $topology_spt} {incr j} {

    set server_num [expr $j + $src_zone_tor_id * $topology_spt]
    $ns duplex-link $v($vm_counter) $s($server_num) [set vm_link_rate]Gb [expr $host_delay] $switchAlg
    $t add_node_to_source $v($vm_counter)
    set vm_counter [expr $vm_counter + 1]

    puts "--------link vm:[expr $vm_counter] to server:[expr $server_num + 1] is created"
}

for {set j 0} { $j < $topology_spt} {incr j} {
    
    set server_num [expr $j + $dst_zone_tor_id * $topology_spt]
    $ns duplex-link $v($vm_counter) $s($server_num) [set vm_link_rate]Gb [expr $host_delay] $switchAlg
    $t add_node_to_dest $v($vm_counter)
    set vm_counter [expr $vm_counter + 1]

    puts "--------link vm:[expr $vm_counter] to server:[expr $server_num + 1] is created"
}



############ Making the tree ##############

$t make_tree $b_factor_1 $b_factor_2 $b_factor_3
$t duplicate_tree
$t print_graph

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


$ns at 4 "$t start_stat_record"

if { $run_migration == "yes" } {
    puts "migration mode: migration will be run." 
    $ns at 4.5 "$t start_migration"
} elseif { $run_migration == "no" } {
    puts "migration mode: migration will not be run. to study normal-mode operation, without migration." 
} elseif { $run_migration == "skip" } {
    puts "migration mode: migartion will be skipped. assume all the VMs are already in the destination." 
    MyTopology set process_after_migration_ 1
} 

$ns at 0.1 "$t setup_nodes"


#############  Agents ################
set lambda [expr ($link_rate*$load*1000000000)/($meanFlowSize*8.0/1460*1500)]
#set lambda [expr ($link_rate*$load*1000000000)/($mean_npkts*($pktSize+40)*8.0)]
puts "Arrival: Poisson with inter-arrival [expr 1/$lambda * 1000] ms"
puts "FlowSize: Pareto with mean = $meanFlowSize, shape = $paretoShape"

puts "Setting up connections ..."; flush stdout

set flow_gen 0
set flow_fin 0
set init_fid 0



# set up dummay warmup flows between all the vms 
# couldn't route properly without this step.
# (might be my mistake. This problem was not supposed to happen with 
# RTProtoDV in use. Maybe works without this as well. Try Removing this)
puts "Setting up warmup flows to make routes ..."; 

set start_server_index 0
set end_server_index $needed_vms
for {set j $start_server_index} {$j < $end_server_index} {incr j} {
    for {set i $start_server_index} {$i < $end_server_index } {incr i} {
        if { $i == $j } {
            continue
        }
        set pair_first [expr $i + 1000]
        set pair_second [expr $j + 1000]
        set agtagr($pair_first,$pair_second) [new Agent_Aggr_pair]
        $agtagr($pair_first,$pair_second) setup $v($i) $v($j) "$pair_first $pair_second" $connections_per_pair $init_fid "TCP_pair" 0
        $agtagr($pair_first,$pair_second) attach-logfile $flowlog
        $ns at 0.2 "$agtagr($pair_first,$pair_second) warmup 1 5"
    }
}


# set up flows from the dedicated servers to the vms 
# as the vm traffic
puts "Setting up flows from the dedicated servers to the vms ..."; 

set start_server_index [expr $vm_traffic_tor_id * $topology_spt]
set end_server_index [expr ($vm_traffic_tor_id + 1) * $topology_spt]
puts "vm traffic start_server_index: $start_server_index"
puts "vm traffic end_server_index: $end_server_index"
set j 0 

foreach leaf $logical_leaves {
    puts $leaf
    for {set i $start_server_index} {$i < $end_server_index} {incr i} {
        if { [expr {$i - $start_server_index}] == $j} {

            set pair_first [expr $i + 2000]
            set pair_second [expr $j + 2000]

            set agtagr($pair_first,$pair_second) [new Agent_Aggr_pair]
            $agtagr($pair_first,$pair_second) setup $s($i) $leaf "$pair_first $pair_second" $connections_per_pair $init_fid "TCP_pair" 1 
            $agtagr($pair_first,$pair_second) attach-logfile $flowlog

            $agtagr($pair_first,$pair_second) send_single_flow $vm_flow_size 4

            $ns at 0.1 "$agtagr($pair_first,$pair_second) warmup 3 5"

            set init_fid [expr $init_fid + $connections_per_pair];
        }   
    }
    set j [expr {$j + 1}];
}


# enable all to all background flows among all servers 
if {$enable_bg_traffic == 1} {
    puts "enable all to all background flows among all servers"
    set start_server_index 0
    set end_server_index [expr $vm_traffic_tor_id * $topology_spt]

    for {set j $start_server_index} {$j < $end_server_index} {incr j} {
        for {set i $start_server_index} {$i < $end_server_index } {incr i} {
            if {$i != $j} {
                set agtagr($i,$j) [new Agent_Aggr_pair]
                $agtagr($i,$j) setup $s($i) $s($j) "$i $j" $connections_per_pair $init_fid "TCP_pair" 0
                $agtagr($i,$j) attach-logfile $flowlog

                $agtagr($i,$j) set_PCarrival_process [expr $lambda/($total_servers - 1)] $flow_cdf [expr 17*$i+1244*$j] [expr 33*$i+4369*$j]

                $ns at 0.1 "$agtagr($i,$j) warmup 1 5"
                $ns at 4 "$agtagr($i,$j) init_schedule"

                set init_fid [expr $init_fid + $connections_per_pair];
            }
        }
    }
}



puts "Initial agent creation done";flush stdout
puts "Simulation started!"

proc finish {} {
        global t ns
        puts "simulation finished at [$ns now]"
        $t print_stats
        exit 0
}

$ns at 1000 "finish"

$ns run