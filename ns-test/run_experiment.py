import itertools
import threading
import os
import Queue
import sys

sys.dont_write_bytecode = True

from run_constants import * 


# setup basics 

q = Queue.Queue()

threads = []
number_worker_threads = 10

DEBUG_VALGRIND = False
DEBUG_GDB = False

ns_path = 'ns'
if DEBUG_VALGRIND:
	ns_path = 'valgrind -s --track-origins=yes --leak-check=full ns'
if DEBUG_GDB:
	ns_path = 'gdb -ex "run" --args ns'

sim_script = 'spine_empirical.tcl'

# helper functions

def worker():
	while True:
		try:
			j = q.get(block = 0)
		except Queue.Empty:
			return
		#Make directory to save results
		os.system('mkdir -p '+j[1])
		os.system(j[0])
		# print(j[0])


def setup_exp(exp):

	mig_sizes = exp["mig_sizes"]

	directory_name = 'exps/%s_P%d_L%d_O%d_MS%s_A%s' % (
		exp["exp_name"], 
		exp["parallel_mig"],
		int(exp["load"]*100), 
		exp["oversub"], 
		str(mig_sizes[0]) + "MB" + str(mig_sizes[1]) + "MB" + str(mig_sizes[2]) + "MB",
		exp["sourceAlg"]
	)

	directory_name = directory_name.lower()
	
	print (directory_name)

	cmd = ns_path + ' ' + sim_script + ' ' \
		+str(sim_end)+' '\
		+str(link_rate)+' '\
		+str(mean_link_delay)+' '\
		+str(host_delay)+' '\
		+str(queueSize)+' '\
		+str(exp["load"])+' '\
		+str(connections_per_pair)+' '\
		+str(meanFlowSize)+' '\
		+str(paretoShape)+' '\
		+str(flow_cdf)+' '\
		+str(enableMultiPath)+' '\
		+str(perflowMP)+' '\
		+str(exp["sourceAlg"])+' '\
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
		+str(exp["oversub"])+' '\
		+str(mig_sizes[0] * 1000000)+' '\
		+str(mig_sizes[1] * 1000000)+' '\
		+str(mig_sizes[2] * 1000000)+' '\
		+str(exp["parallel_mig"])+' '\
		+str('./'+directory_name+'/flow.tr')+'  >'\
		+str('./'+directory_name+'/logFile.tr')

	q.put([cmd, directory_name])


# main 
if __name__ == "__main__":

	# define the configs to do experiments over 
	# don't forget to set a proper name for the experiment
	configs = {
		"exp_name": ["with_mig_100000_flows"],
		"mig_sizes": [(100, 100, 10)],
		"parallel_mig": [2], 
		"load": [0.5, 0.6],
		"oversub": [2.0],
		"sourceAlg":['TCP'], 
	}

	# Add all possible experiments to the queue
	keys, values = zip(*configs.items())
	permutations_dicts = [dict(zip(keys, v)) for v in itertools.product(*values)]
	for exp in permutations_dicts:
		setup_exp(exp)

	# Start threads to process jobs
	for i in range(number_worker_threads):
		t = threading.Thread(target = worker)
		threads.append(t)
		t.start()

	# Join all completed threads
	for t in threads:
		t.join()
		
		