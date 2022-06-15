set ns [new Simulator]
set sim_end 100000


# $ns rtproto DV
# Agent/rtProto/DV set advertInterval [expr 2*$sim_end]
Node set multiPath_ 1
Agent/TCP/FullTcp set segsize_ 1400

set tf [open out.tr w]
# $ns trace-all $tf

proc finish {} {
        puts "simulation finished"
        global ns tf
        # $ns flush-trace
        close $tf
        exit 0
}



MyTopology set verbose_ 0
MyTopology set verbose_nf_ 0
MyTopology set verbose_mig_ 0
MyTopology set vm_precopy_size_  10000000
MyTopology set vm_snapshot_size_ 10000000
MyTopology set gw_snapshot_size_ 10000000
MyTopology set parallel_mig_ 1
MyTopology set prioritization_level_ 2
MyTopology set stat_record_interval_ 0.01
MyTopology set process_after_migration_ 0
MyTopology set orch_type_ [lindex $argv 0]


set DCTCP_K 65.0
set drop_prio_ true
set deque_prio_ true
set pktSize 1460

Queue/MamadQueue set bytes_ false
Queue/MamadQueue set queue_in_bytes_ true
Queue/MamadQueue set mean_pktsize_ [expr $pktSize+40]
Queue/MamadQueue set setbit_ true
Queue/MamadQueue set gentle_ false
Queue/MamadQueue set q_weight_ 1.0
Queue/MamadQueue set mark_p_ 1.0
Queue/MamadQueue set thresh_ $DCTCP_K
Queue/MamadQueue set maxthresh_ $DCTCP_K
Queue/MamadQueue set drop_prio_ $drop_prio_
Queue/MamadQueue set deque_prio_ $deque_prio_

Queue/MamadQueue set bytes_1_ false
Queue/MamadQueue set queue_in_bytes_1_ true
Queue/MamadQueue set mean_pktsize_1_ [expr $pktSize+40]
Queue/MamadQueue set setbit_1_ true
Queue/MamadQueue set gentle_1_ false
Queue/MamadQueue set q_weight_1_ 1.0
Queue/MamadQueue set mark_p_1_ 1.0
Queue/MamadQueue set thresh_1_ $DCTCP_K
Queue/MamadQueue set maxthresh_1_ $DCTCP_K
Queue/MamadQueue set drop_prio_1_ $drop_prio_
Queue/MamadQueue set deque_prio_1_ $deque_prio_

Queue/MamadQueue set bytes_2_ false
Queue/MamadQueue set queue_in_bytes_2_ true
Queue/MamadQueue set mean_pktsize_2_ [expr $pktSize+40]
Queue/MamadQueue set setbit_2_ true
Queue/MamadQueue set gentle_2_ false
Queue/MamadQueue set q_weight_2_ 1.0
Queue/MamadQueue set mark_p_2_ 1.0
Queue/MamadQueue set thresh_2_ $DCTCP_K
Queue/MamadQueue set maxthresh_2_ $DCTCP_K
Queue/MamadQueue set drop_prio_2_ $drop_prio_
Queue/MamadQueue set deque_prio_2_ $deque_prio_



set t [new MyTopology]
$t set_simulator $ns

set UDP_INTERARRIVAL 0.000005
set BW 10Gb
set LAT 5us
# set QTYPE RED
set QTYPE MamadQueue
set child_count 100

Queue set limit_ 400

set n_mid_left [$ns node]
set n_mid_right [$ns node]
$ns duplex-link $n_mid_left $n_mid_right $BW $LAT $QTYPE


for { set x 2} { $x < $child_count+2} { incr x } {
    set n($x) [$ns node]
    $ns duplex-link $n_mid_left $n($x) 10Gb $LAT $QTYPE 
    $t add_node_to_dest $n($x)
}


for { set x [expr $child_count+2]} { $x < [expr 2*$child_count+2]} { incr x } {
    set n($x) [$ns node]
    $ns duplex-link $n_mid_right $n($x) 10Gb $LAT $QTYPE
    $t add_node_to_source $n($x)
}



# make agents 
set src [new Agent/TCP/FullTcp]
set sink [new Agent/TCP/FullTcp]
$src set fid_ 100
$sink set fid_ 100

set src2 [new Agent/TCP/FullTcp]
set sink2 [new Agent/TCP/FullTcp]
$src2 set fid_ 10
$sink2 set fid_ 10

# make apps
set src_app [new Application]
set sink_app [new Application]
set udp0 [new Agent/UDP]


set src_app2 [new Application]
set sink_app2 [new Application]

# attach agents to nodes
$ns attach-agent $n(150) $src
$ns attach-agent $n(50) $sink
$ns attach-agent $n(149) $src2
$ns attach-agent $n(49) $sink2

$ns attach-agent $n(150) $udp0
set cbr0 [new Application/Traffic/CBR]
$cbr0 set packetSize_ 1000
$cbr0 set interval_ $UDP_INTERARRIVAL
$cbr0 attach-agent $udp0
set null0 [new Agent/Null]
$ns attach-agent $n(50) $null0
$ns connect $udp0 $null0
# $ns at 1.9 "$cbr0 start"
# $ns at 10 "$cbr0 stop"



# attach the apps
# $src_app attach-agent $src
# $sink_app attach-agent $sink
# $src_app2 attach-agent $src2
# $sink_app2 attach-agent $sink2


# make the connection
$ns connect $src $sink
$sink listen

$ns connect $src2 $sink2
$sink2 listen

 
# $t make_tree 1 2 2 2
# $t make_tree 1 1 1 8
$t make_tree 2 2 2

$t duplicate_tree
$t print_graph

$ns at 1 "$t setup_nodes"
$ns at 2 "$t start_migration"

$ns at $sim_end "finish"
$ns run
