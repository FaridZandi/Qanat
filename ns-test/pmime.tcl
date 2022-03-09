# packetmime template obtained from file: pm-end-pairs.tcl
set ns [new Simulator]

$ns rtproto DV
Agent/rtProto/DV set advertInterval 0.001
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

# useful constants
set CLIENT 0
set SERVER 1

set goal_pairs 50

remove-all-packet-headers;             # removes all packet headers
add-packet-header IP TCP;              # adds TCP/IP headers
$ns use-scheduler Heap;                # use the Heap scheduler

set n1 [$t find_node $g1 1 1]
set n1P [$t find_node $g2 1 1]

# SETUP PACKMIME
set rate 10
set pm [new PackMimeHTTP]
$pm set-client $src;                  # name $src as client
$pm set-server $n1;                  # name $n1 as server
$pm set-rate $rate;                    # new connections per second
$pm set-http-1.1;                      # use HTTP/1.1

# SETUP PACKMIME RANDOM VARIABLES

# create RNGs (appropriate RNG seeds are assigned automatically)
set flowRNG [new RNG]
set reqsizeRNG [new RNG]
set rspsizeRNG [new RNG]

# create RandomVariables
set flow_arrive [new RandomVariable/PackMimeHTTPFlowArrive $rate]
set req_size [new RandomVariable/PackMimeHTTPFileSize $rate $CLIENT]
set rsp_size [new RandomVariable/PackMimeHTTPFileSize $rate $SERVER]

# assign RNGs to RandomVariables
$flow_arrive use-rng $flowRNG
$req_size use-rng $reqsizeRNG
$rsp_size use-rng $rspsizeRNG

# set PackMime variables
$pm set-flow_arrive $flow_arrive
$pm set-req_size $req_size
$pm set-rsp_size $rsp_size

# record HTTP statistics
$pm set-outfile "pm-end-pairs.dat"

$ns at 0.1 "$pm start"

# force quit after completing desired number of pairs
for {set i 0} {$i < 1000000} {incr i} {
    $ns at $i "check_pairs"
}

proc check_pairs {} {
    global pm goal_pairs

    set cur_pairs [$pm get-pairs]
    if {$cur_pairs >= $goal_pairs} {
	#
	# NOTE: "$pm stop" will not stop PackMime immediately.  Those 
	#       connections that have already started will be allowed to 
	#       complete, so pm-end-pairs.dat will likely contain slightly 
	#       more than "goal_pairs" entries.
	#
	puts stderr "Completed $goal_pairs HTTP pairs.  Finishing..."
	$pm stop
    global ns tf
    $ns flush-trace
    close $tf
    exit 0
    }

}

$ns run