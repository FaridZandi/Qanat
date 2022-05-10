import threading
import os
import Queue

def worker():
	while True:
		try:
			j = q.get(block = 0)
		except Queue.Empty:
			return
		#Make directory to save results
		os.system('mkdir -p '+j[1])
		os.system(j[0])

q = Queue.Queue()

DEBUG_VALGRIND = False

sim_end = 120 # simulate for 30 seconds
link_rate = 10
mean_link_delay = 0.0000002
host_delay = 0.000020
queueSize = 140

load = 0.2

# load_arr = [0.9,0.8,0.7,0.6,0.5]
# load_arr = [0.2]
# l = 0.01
# for i in range(40):
	# load_arr.append(l)
	# l += 0.01

connections_per_pair = 1
meanFlowSize = 1138*1460
paretoShape = 1.05

eventual_timeouts = [0.01]
# l = 0.01
# for i in range(10):
# 	eventual_timeouts.append(l)
# 	l += 0.01
# eventual_timeouts *= 5

topology_shapes = [1, 2, 3]
# topology_shapes = [1]
remote_storage_rate = 1000000
local_storage_rate = 1000000

# key_types = [1]
key_types = [1,2,3,4,5,6]

# access_modes = [0,1,2,3] 
access_modes = [3] 
# access_mode = 0 # local 
# access_mode = 1 # remote 
# access_mode = 2 # eventual
# access_mode = 3 # cache 

# flow_cdf = 'CDF_My.tcl'
flow_cdf = 'CDF_dctcp.tcl'

enableMultiPath = 1
perflowMP = 0

sourceAlg = 'DCTCP-Sack'
initWindow = 70
ackRatio = 1
slowstartrestart = 'true'
DCTCP_g = 0.0625
min_rto = 0.000250
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

# topology_spt = 16
# topology_tors = 9
# topology_spines = 4
# topology_x = 1
# smaller topology
topology_spt = 32
topology_tors = 2
topology_spines = 1
topology_x = 1
#sets the number of machines needed on the destination (assumed to be on the same rack)
topology_dest_servers = 4 

ns_path = 'ns'
if DEBUG_VALGRIND:
	ns_path = 'valgrind -s --track-origins=yes --leak-check=full ns'

sim_script = 'spine_empirical.tcl'


i = 0

for key_type in key_types: 
	for topology_shape in topology_shapes: 
		for eventual_timeout in eventual_timeouts:
			for access_mode in access_modes:
				i += 1 
				prio_scheme_ = 2 
				scheme = 'unknown'

				#Directory name: workload_scheme_load_[load]
				access_mode_name = "" 

				if access_mode == 0: 
					access_mode_name = "local"
				elif access_mode == 1:
					access_mode_name = "remote"
				elif access_mode == 2:
					access_mode_name = "eventual"
				elif access_mode == 3:
					access_mode_name = "cache"


				directory_name = 'results/%d_%s_%f_%i_%i_%i' % (int(load*100), access_mode_name, eventual_timeout, key_type, topology_shape, i)
				
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
					+str(topology_x)+' '\
					+str(topology_dest_servers)+' '\
					+str(eventual_timeout)+' '\
					+str(access_mode)+' '\
					+str(topology_shape)+' '\
					+str(remote_storage_rate)+' '\
					+str(local_storage_rate)+' '\
					+str(key_type)+' '\
					+str('./'+directory_name+'/flow.tr')+'  >'\
					+str('./'+directory_name+'/logFile.tr')

				q.put([cmd, directory_name])

#Create all worker threads
threads = []
number_worker_threads = 12

#Start threads to process jobs
for i in range(number_worker_threads):
	t = threading.Thread(target = worker)
	threads.append(t)
	t.start()

#Join all completed threads
for t in threads:
	t.join()