set ns [new Simulator]
set sim_end 50


# $ns rtproto DV
# Agent/rtProto/DV set advertInterval [expr 2*$sim_end]
Node set multiPath_ 1
Agent/TCP/FullTcp set segsize_ 1400

# set tf [open out.tr w]
# $ns trace-all $tf

proc finish {} {
        puts "simulation finished"
        global t ns
        # $ns flush-trace
        # close $tf
        $t print_stats
        exit 0
}



MyTopology set verbose_ 0
MyTopology set verbose_nf_ 0
MyTopology set verbose_mig_ 0
MyTopology set vm_precopy_size_  1000000
MyTopology set vm_snapshot_size_ 1000000
MyTopology set gw_snapshot_size_ 1000000
MyTopology set parallel_mig_ 1
MyTopology set prioritization_level_ 2
MyTopology set stat_record_interval_ 0.001
MyTopology set process_after_migration_ 0
MyTopology set orch_type_ [lindex $argv 0]


set DCTCP_K 65.0
set drop_prio_ true
set deque_prio_ true
set pktSize 1460

Agent/TCP set ecn_ 1
Agent/TCP set old_ecn_ 1
Agent/TCP set packetSize_ $pktSize
Agent/TCP/FullTcp set segsize_ $pktSize
Agent/TCP/FullTcp set spa_thresh_ 0
Agent/TCP set slow_start_restart_ true
Agent/TCP set windowOption_ 0
Agent/TCP set minrto_ 0.2
Agent/TCP set tcpTick_ 0.000001
Agent/TCP set maxrto_ 64
Agent/TCP set lldct_w_min_ 0.125
Agent/TCP set lldct_w_max_ 2.5
Agent/TCP set lldct_size_min_ 204800
Agent/TCP set lldct_size_max_ 1048576

Agent/TCP set ecnhat_ true
Agent/TCPSink set ecnhat_ true
Agent/TCP set ecnhat_g_ 0.0625
Agent/TCP set lldct_ false
Agent/TCP/FullTcp set dynamic_dupack_ 0; #disable dupack
Agent/TCP set window_ 1328;
Agent/TCP set windowInit_ 70
Agent/TCP set rtxcur_init_ 0.05;
# Agent/TCP set slow_start_restart_ false
Queue set limit_ 240



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

set BW 1Gb
set LAT 1ms
set QTYPE MyQueue
set child_count 100


set n_mid_left [$ns node]
set n_mid_right [$ns node]
$ns duplex-link $n_mid_left $n_mid_right 100Gb 1us $QTYPE


for { set x 2} { $x < $child_count+2} { incr x } {
    set n($x) [$ns node]
    $ns duplex-link $n_mid_left $n($x) $BW $LAT $QTYPE 
    $t add_node_to_dest $n($x)
}


for { set x [expr $child_count+2]} { $x < [expr 2*$child_count+2]} { incr x } {
    set n($x) [$ns node]
    $ns duplex-link $n_mid_right $n($x) $BW $LAT $QTYPE
    $t add_node_to_source $n($x)
}


set leaves(0) $n(198)
set leaves(1) $n(197)
set leaves(2) $n(195)
set leaves(3) $n(194)
set leaves(4) $n(191)
set leaves(5) $n(190)
set leaves(6) $n(188)
set leaves(7) $n(187)

set senders(0) $n(50)
set senders(1) $n(51)
set senders(2) $n(52)
set senders(3) $n(53)
set senders(4) $n(54)
set senders(5) $n(55)
set senders(6) $n(56)
set senders(7) $n(57)

set upd_connections 0 
set tcp_connections 0 

# set up a connection from one sender to one receiver
for {set i 0} {$i < $upd_connections} {incr i} {
    set udp($i) [new Agent/UDP]
    set null($i) [new Agent/Null]

    $ns attach-agent $senders($i) $udp($i)
    $ns attach-agent $leaves($i) $null($i)

    set cbr($i) [new Application/Traffic/CBR]
    $cbr($i) set packetSize_ 1460
    $cbr($i) set interval_ 0.001
    $cbr($i) attach-agent $udp($i)

    $ns connect $udp($i) $null($i)
    # $ns at 1.5 "$cbr($i) start"
    # $ns at 5 "$cbr($i) stop"
}



 for {set i 0} {$i < $tcp_connections} {incr i} {    
    # make agents 
    set tcp($i) [new Agent/TCP/FullTcp]
    set sink($i) [new Agent/TCP/FullTcp]

    $ns attach-agent $senders($i) $tcp($i)
    $ns attach-agent $leaves($i) $sink($i)

    $tcp($i) set fid_ [expr $i + 100]
    $sink($i) set fid_ [expr $i + 100]

    $tcp($i) set minrto_ 0.2
    $sink($i) set minrto_ 0.2

    # attach the apps
    set src_app($i) [new Application]
    set sink_app($i) [new Application]
    $src_app($i) attach-agent $tcp($i)
    $sink_app($i) attach-agent $sink($i)

    # make the connection
    $ns connect $tcp($i) $sink($i)
    $sink($i) listen

    $ns at 1.9 "$sink($i) record_stat"
    $ns at 1.9 "$src_app($i) send 600000000"
}

$t rate_limit_node $senders(0) 83333
 
# $t make_tree 1 2 2 2
# $t make_tree 1 1 1 8
# $t make_tree 2 2 2
$t make_dag 4 4 4 


$t duplicate_dag
# $t print_graph

$ns at 1 "$t setup_nodes"
# $ns at 1.5 "$t start_stat_record"
$ns at 4.5 "$t start_migration"

# $ns at 2 "puts 2" 
# $ns at 3 "puts 3" 
# $ns at 4 "puts 4" 
# $ns at 5 "puts 5" 
# $ns at 6 "puts 6" 
# $ns at 7 "puts 7" 
# $ns at 8 "puts 8" 
# $ns at 9 "puts 9" 
# $ns at 10 "puts 10" 

$ns at $sim_end "finish"
$ns run