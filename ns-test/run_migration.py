import threading
import os
import Queue
import itertools

def worker():
	while True:
		try:
			j = q.get(block = 0)
		except Queue.Empty:
			return
		#Make directory to save results
		os.system('mkdir '+j[1])
		os.system(j[0])

q = Queue.Queue()

DEBUG_VALGRIND = False

vm_precopy_size = 100000000
vm_snapshot_size = 10000000
gw_snapshot_size = 1000000
parallel_mig = 1


sim_end = 1200000
link_rate = 10
mean_link_delay = 0.000005
host_delay = 0.000010
queueSize = 240
parallel_mig_arr = [1,2,3,4,5,6]
load_arr = [0.4, 0.5, 0.6, 0.7, 0.8, 0.9]
oversub_arr = [1.0,4.0]
# parallel_mig_arr = [2]
# load_arr = [0.1]
# oversub_arr = [1.0]
connections_per_pair = 1
meanFlowSize = 1138*1460
paretoShape = 1.05
flow_cdf = 'CDF_dctcp.tcl'

enableMultiPath = 1
perflowMP = 0

sourceAlg = 'DCTCP-Sack'
initWindow = 10
ackRatio = 1
slowstartrestart = 'true'
DCTCP_g = 0.0625
min_rto = 0.001
prob_cap_ = 5

switchAlg = 'DropTail'
DCTCP_K = 65.0
drop_prio_ = 'true'
# prio_scheme_arr = [2,3]
prio_scheme_arr = [0]
deque_prio_ = 'true'
keep_order_ = 'true'
prio_num_ = 1
ECN_scheme_ = 2 #Per-port ECN marking
pias_thresh_0 = 46*1460
pias_thresh_1 = 1084*1460
pias_thresh_2 = 1717*1460
pias_thresh_3 = 1943*1460
pias_thresh_4 = 1989*1460
pias_thresh_5 = 1999*1460
pias_thresh_6 = 2001*1460

topology_spt = 16
topology_tors = 9
topology_spines = 4

## sample debug method:
# run: gdb ns
# command to run inside gdb:
# run spine_empirical.tcl 120 10 2e-07 2e-05 140 0.5 1 1661480 1.05 CDF_dctcp.tcl 1 0 DCTCP-Sack 70 1 true 0.0625 0.00025 5 DropTail 65.0 true 0 true true 1 2 67160 1582640 2506820 2836780 2903940 2918540 2921460 4 2 1 1 4 ./websearch_unknown_50/flow.tr
ns_path = 'ns'
if DEBUG_VALGRIND:
	ns_path = 'valgrind -s --track-origins=yes --leak-check=full ns'

sim_script = 'spine_empirical.tcl'

for prio_scheme_ in prio_scheme_arr:
	for load, parallel_mig, oversub in itertools.product(load_arr, parallel_mig_arr, oversub_arr):
		scheme = 'unknown'

		if prio_scheme_ == 2:
			scheme = 'pfabric_remainingSize'
		elif prio_scheme_ == 3:
			scheme = 'pfabric_bytesSent'

		#Directory name: workload_scheme_load_[load]
		directory_name = 'websearch_P%d_L%d_O%d' % (parallel_mig,int(load*100), oversub)
		directory_name = directory_name.lower()
		#Simulation command
		cmd = ns_path+' '+sim_script+' '\
			+str(sim_end)+' '\
			+str(link_rate)+' '\
			+str(mean_link_delay)+' '\
			+str(host_delay)+' '\
			+str(queueSize)+' '\
			+str(load)+' '\
			+str(connections_per_pair)+' '\
			+str(meanFlowSize)+' '\
			+str(paretoShape)+' '\
			+str(flow_cdf)+' '\
			+str(enableMultiPath)+' '\
			+str(perflowMP)+' '\
			+str(sourceAlg)+' '\
			+str(initWindow)+' '\
			+str(ackRatio)+' '\
			+str(slowstartrestart)+' '\
			+str(DCTCP_g)+' '\
			+str(min_rto)+' '\
			+str(prob_cap_)+' '\
			+str(switchAlg)+' '\
			+str(DCTCP_K)+' '\
			+str(drop_prio_)+' '\
			+str(prio_scheme_)+' '\
			+str(deque_prio_)+' '\
			+str(keep_order_)+' '\
			+str(prio_num_)+' '\
			+str(ECN_scheme_)+' '\
			+str(pias_thresh_0)+' '\
			+str(pias_thresh_1)+' '\
			+str(pias_thresh_2)+' '\
			+str(pias_thresh_3)+' '\
			+str(pias_thresh_4)+' '\
			+str(pias_thresh_5)+' '\
			+str(pias_thresh_6)+' '\
			+str(topology_spt)+' '\
			+str(topology_tors)+' '\
			+str(topology_spines)+' '\
			+str(oversub)+' '\
			+str(vm_precopy_size)+' '\
			+str(vm_snapshot_size)+' '\
			+str(gw_snapshot_size)+' '\
			+str(parallel_mig)+' '\
			+str('./'+directory_name+'/flow.tr')+'  >'\
			+str('./'+directory_name+'/logFile.tr')

		q.put([cmd, directory_name])

#Create all worker threads
threads = []
number_worker_threads = 10

#Start threads to process jobs
for i in range(number_worker_threads):
	t = threading.Thread(target = worker)
	threads.append(t)
	t.start()

#Join all completed threads
for t in threads:
	t.join()