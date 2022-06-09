remove-all-packet-headers;            # removes all packet headers
add-packet-header IP TCP;             # adds TCP/IP headers
set ns [new Simulator];               # instantiate the simulator
set sim_end 100

global defaultRNG
$defaultRNG seed 999

# create nodes
set n_src [$ns node]
set n_sink [$ns node]

# setup links
$ns duplex-link $n_src $n_sink 100Mb 1ms DropTail

set tf [open out.tr w]
$ns trace-all $tf

proc finish {} {
        puts "simulation finished"
        global ns tf
        $ns flush-trace
        close $tf
        exit 0
}

set src [new Agent/TCP/FullTcp]
set sink [new Agent/TCP/FullTcp]
$src set fid_ 1
$sink set fid_ 1

# attach agents to nodes
$ns attach-agent $n_src $src
$ns attach-agent $n_sink $sink

# make the connection
$ns connect $src $sink
$sink listen

set app1 [new Application]
$app1 attach-agent $src
set app2 [new Application]
$app2 attach-agent $sink

$ns at 1 "$app1 send 10000000"

set src2 [new Agent/TCP/FullTcp]
set sink2 [new Agent/TCP/FullTcp]
$src2 set fid_ 2
$sink2 set fid_ 2

# attach agents to nodes
$ns attach-agent $n_src $src2
$ns attach-agent $n_sink $sink2

# make the connection
$ns connect $src2 $sink2
$sink2 listen

set app3 [new Application]
$app3 attach-agent $src2
set app4 [new Application]
$app4 attach-agent $sink2

$ns at 5 "$app3 send 10000000"

# start the simulation
$ns at $sim_end "finish"
$ns run

