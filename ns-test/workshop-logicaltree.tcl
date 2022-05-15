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

set parallel_mig [lindex $argv 0]
set UDP_INTERARRIVAL [lindex $argv 1]
set QT [lindex $argv 2]
set BW 10Gb

MyTopology set verbose_ 0
MyTopology set verbose_nf_ 0
MyTopology set verbose_mig_ 0
MyTopology set vm_precopy_size_  50000000
MyTopology set vm_snapshot_size_ 10000000
MyTopology set gw_snapshot_size_ 10000000

MyTopology set parallel_mig_ $parallel_mig




set t [new MyTopology]
$t set_simulator $ns

set LAT 5us
set QTYPE $QT 
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

 
$t make_tree 1 2 2 2
$t duplicate_tree
$t print_graph

$ns at 1 "$t setup_nodes"
$ns at 2 "$t start_migration"

# $ns at 1.99 "$src_app send 1000000000"
# $ns at 1.99 "$src_app2 send 30000000000"

$ns at $sim_end "finish"
$ns run
