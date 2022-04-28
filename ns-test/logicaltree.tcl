set ns [new Simulator]
set sim_end 100


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


set t [new MyTopology]
$t set_simulator $ns

set BW 10Gb 
set LAT 5us
set QTYPE DropTail 
set child_count 100

set n_mid_left [$ns node]
set n_mid_right [$ns node]
$ns duplex-link $n_mid_left $n_mid_right $BW $LAT $QTYPE


for { set x 0} { $x < $child_count} { incr x } {
    set n($x) [$ns node]
    $ns duplex-link $n_mid_left $n($x) $BW $LAT $QTYPE 
    $t add_node_to_dest $n($x)
}


for { set x $child_count} { $x < 2 * $child_count} { incr x } {
    set n($x) [$ns node]
    $ns duplex-link $n_mid_right $n($x) $BW $LAT $QTYPE
    $t add_node_to_source $n($x)
}



# make agents 
set src [new Agent/TCP/FullTcp]
set sink [new Agent/TCP/FullTcp]
$src set fid_ 100
$sink set fid_ 100

# make apps
set src_app [new Application]
set sink_app [new Application]

# attach agents to nodes
$ns attach-agent $n(150) $src
$ns attach-agent $n(195) $sink

# attach the apps
$src_app attach-agent $src
$sink_app attach-agent $sink


# make the connection
$ns connect $src $sink
$sink listen

 
# $t make_tree 4 1 1 1
# $t make_tree 2 2 2
$t make_tree 3 2 1
$t duplicate_tree
$t print_graph

$ns at 1 "$t setup_nodes"
# $ns at 1.5 "$t activate_tunnel $n28 $n12 $n26 $n10 "
$ns at 2 "$t start_migration"

# $ns at 1.9 "$src_app send 300000000"

$ns at $sim_end "finish"
$ns run
