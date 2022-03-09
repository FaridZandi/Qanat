set ns [new Simulator]
set sim_end 1

set tf [open out.tr w]
$ns trace-all $tf

proc finish {} {
        global ns tf
        $ns flush-trace
        close $tf
        exit 0
}



set src [$ns node] 
set n1 [$ns node] 
set n2 [$ns node] 
set n3 [$ns node]
set n4 [$ns node]

set g1 [$ns node]  
set g2 [$ns node]

set f1 [$ns node]
set f2 [$ns node]

set bw 5Mb
set delay 3ms
set queue_policy MyQueue

$ns duplex-link $src $f1 $bw $delay $queue_policy

$ns duplex-link $f1 $f2 $bw $delay $queue_policy

$ns duplex-link $f1 $g1 $bw $delay $queue_policy
$ns duplex-link $f1 $g2 $bw $delay $queue_policy
$ns duplex-link $f2 $g1 $bw $delay $queue_policy
$ns duplex-link $f2 $g2 $bw $delay $queue_policy

$ns duplex-link $n1 $g1 $bw $delay $queue_policy
$ns duplex-link $n2 $g1 $bw $delay $queue_policy
$ns duplex-link $n3 $g2 $bw $delay $queue_policy
$ns duplex-link $n4 $g2 $bw $delay $queue_policy

$ns rtproto DV
Agent/rtProto/DV set advertInterval	[expr 2*$sim_end]  
Node set multiPath_ 1 

proc setup {} {
    global ns src n1 n2 n3 n4 g1 g2 f1
    

    # set up the UDP agents
    set udp_src1 [new Agent/UDP]
    set udp_src2 [new Agent/UDP]
    set udp_n1 [new Agent/UDP]
    set udp_n2 [new Agent/UDP]
    set udp_n3 [new Agent/UDP]
    set udp_n4 [new Agent/UDP]

    # attach the udp agents to the nodes
    $ns attach-agent $src $udp_src1
    $ns attach-agent $src $udp_src2

    $ns attach-agent $n1 $udp_n1
    $ns attach-agent $n2 $udp_n2
    $ns attach-agent $n3 $udp_n3
    $ns attach-agent $n4 $udp_n4

    # Connect the udp agents to each other 
    # This will create the required routes  
    $ns connect $udp_src1 $udp_n1
    $ns connect $udp_src2 $udp_n4

    # set up applications 
    set app_src1 [new Application]
    set app_src2 [new Application]
    $app_src1 attach-agent $udp_src1
    $app_src2 attach-agent $udp_src2

    #run the simulation 
    # $ns at 0.2 "$app_src1 send 1000"
    # $ns at 0.3 "$app_src2 send 1000"
    $ns at 0.4 "$g1 activate-tunnel $g2 $src $n1 $n3"
    $ns at 0.4 "$g2 activate-tunnel $g1 $src $n4 $n2"
    $ns at 0.5 "$app_src1 send 1000"
    $ns at 0.6 "$app_src2 send 1000"
    # $ns at 0.7 "$g1 deactivate_tunnel 0"
    # $ns at 0.7 "$g2 deactivate_tunnel 1"
    # $ns at 0.8 "$app_src1 send 1000"
    # $ns at 0.9 "$app_src2 send 1000"
    

}


$ns at 0.1 "setup"


$ns at $sim_end "finish"
$ns run
