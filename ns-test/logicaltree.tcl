set ns [new Simulator]
set sim_end 100


$ns rtproto DV
Agent/rtProto/DV set advertInterval [expr 2*$sim_end]
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


set n01 [$ns node]
set n02 [$ns node]
set n03 [$ns node]
set n04 [$ns node]
set n05 [$ns node]
set n06 [$ns node]
set n07 [$ns node]
set n08 [$ns node]
set n09 [$ns node]
set n10 [$ns node]
set n11 [$ns node]
set n12 [$ns node]
set n13 [$ns node]
set n14 [$ns node]
set n15 [$ns node]
set n16 [$ns node]
set n17 [$ns node]
set n18 [$ns node]
set n19 [$ns node]
set n20 [$ns node]
set n21 [$ns node]
set n22 [$ns node]
set n23 [$ns node]
set n24 [$ns node]
set n25 [$ns node]
set n26 [$ns node]
set n27 [$ns node]
set n28 [$ns node]
set n29 [$ns node]
set n30 [$ns node]


$ns duplex-link $n15 $n01 2Mb 10ms DropTail
$ns duplex-link $n15 $n02 2Mb 10ms DropTail
$ns duplex-link $n15 $n03 2Mb 10ms DropTail
$ns duplex-link $n15 $n04 2Mb 10ms DropTail
$ns duplex-link $n15 $n05 2Mb 10ms DropTail
$ns duplex-link $n15 $n06 2Mb 10ms DropTail
$ns duplex-link $n15 $n07 2Mb 10ms DropTail
$ns duplex-link $n15 $n08 2Mb 10ms DropTail
$ns duplex-link $n15 $n09 2Mb 10ms DropTail
$ns duplex-link $n15 $n10 2Mb 10ms DropTail
$ns duplex-link $n15 $n11 2Mb 10ms DropTail
$ns duplex-link $n15 $n12 2Mb 10ms DropTail
$ns duplex-link $n15 $n13 2Mb 10ms DropTail
$ns duplex-link $n15 $n14 2Mb 10ms DropTail

$ns duplex-link $n15 $n16 2Mb 10ms DropTail

$ns duplex-link $n16 $n17 2Mb 10ms DropTail
$ns duplex-link $n16 $n18 2Mb 10ms DropTail
$ns duplex-link $n16 $n19 2Mb 10ms DropTail
$ns duplex-link $n16 $n20 2Mb 10ms DropTail
$ns duplex-link $n16 $n21 2Mb 10ms DropTail
$ns duplex-link $n16 $n22 2Mb 10ms DropTail
$ns duplex-link $n16 $n23 2Mb 10ms DropTail
$ns duplex-link $n16 $n24 2Mb 10ms DropTail
$ns duplex-link $n16 $n25 2Mb 10ms DropTail
$ns duplex-link $n16 $n26 2Mb 10ms DropTail
$ns duplex-link $n16 $n27 2Mb 10ms DropTail
$ns duplex-link $n16 $n28 2Mb 10ms DropTail
$ns duplex-link $n16 $n29 2Mb 10ms DropTail
$ns duplex-link $n16 $n30 2Mb 10ms DropTail


set t [new MyTopology]
$t set_simulator $ns



$t add_node_to_dest $n01
$t add_node_to_dest $n02
$t add_node_to_dest $n03
$t add_node_to_dest $n04
$t add_node_to_dest $n05
$t add_node_to_dest $n06
$t add_node_to_dest $n07
$t add_node_to_dest $n08
$t add_node_to_dest $n09
$t add_node_to_dest $n10
$t add_node_to_dest $n11
$t add_node_to_dest $n12
$t add_node_to_dest $n15
$t add_node_to_dest $n13
$t add_node_to_dest $n14

$t add_node_to_source $n16
$t add_node_to_source $n17
$t add_node_to_source $n18
$t add_node_to_source $n19
$t add_node_to_source $n20
$t add_node_to_source $n21
$t add_node_to_source $n22
$t add_node_to_source $n23
$t add_node_to_source $n24
$t add_node_to_source $n25
$t add_node_to_source $n26
$t add_node_to_source $n27
$t add_node_to_source $n28
$t add_node_to_source $n29
$t add_node_to_source $n30


$t make_tree 1
$t duplicate_tree

$t print_graph

$ns at $sim_end "finish"
$ns at 1 "$t start_migration"
$ns run
