set ns [new Simulator]
set sim_end 1

$ns rtproto DV
Agent/rtProto/DV set advertInterval [expr 2*$sim_end]  
Node set multiPath_ 1 

set tf [open out.tr w]
$ns trace-all $tf

proc finish {} {
        global ns tf
        $ns flush-trace
        close $tf
        exit 0
}

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

$t make_tree $g1 2
# $t duplicate_tree $g1 $g2

$t print_graph
$t setup_apps

$t set_mig_root $g1
$t start_migration

set n1 [$t find_node $g1 1]
puts [ $n1 address? ]
$ns at 0.05 "$t send_data $src $n1"
$ns at 0.6 "puts hello"


$ns at $sim_end "finish"
$ns run
