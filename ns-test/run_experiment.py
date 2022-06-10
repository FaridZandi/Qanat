import sys
sys.dont_write_bytecode = True

import itertools
import threading
import os
import Queue
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


# helper functions

def process_results(exp_directory):
	print("Processing results on %s" % exp_directory)
	os.system("./processing/process_setting.sh %s" % exp_directory)


def worker():
	while True:
		try:
			j = q.get(block = 0)
		except Queue.Empty:
			return

		command = j[0]
		directory_name = j[1]

		os.system('mkdir -p ' + directory_name)

		os.system(command)

		process_results(directory_name)


def setup_exp(exp):

	mig_sizes = exp["mig_sizes"]

	os.system('mkdir -p ' + "exps/%s/" % exp["exp_name"])

	if exp["run_migration"] == "yes":
		mig_str = "mig"
	elif exp["run_migration"] == "no":
		mig_str = "nomig"
	elif exp["run_migration"] == "skip":
		mig_str = "skipmig"

	directory_name = 'exps/%s/%s_P%d_L%d_O%d_MS%s_A%s_OR%d_Pr%s_Sd%d_Dd%d' % (
		exp["exp_name"], 

		mig_str,
		exp["parallel_mig"],
		int(exp["load"]*100), 
		exp["oversub"], 
		str(mig_sizes[0]) + "-" + str(mig_sizes[1]) + "-" + str(mig_sizes[2]),
		exp["sourceAlg"],
		exp["orch_type"],
		str(exp["enable_prioritization"]),
		int(exp["src_zone_delay"] * 1000000),
		int(exp["dst_zone_delay"] * 1000000),
	)

	directory_name = directory_name.lower()
	
	print (directory_name)

	if exp["network_topo"] == "datacenter":
		sim_script = 'topo_spine_empirical.tcl'
		cmd = ns_path + ' ' + sim_script + ' ' \
			+ str(exp["sim_end"])+' '\
			+ str(link_rate)+' '\
			+ str(mean_link_delay)+' '\
			+ str(host_delay)+' '\
			+ str(queueSize)+' '\
			+ str(exp["load"])+' '\
			+ str(connections_per_pair)+' '\
			+ str(meanFlowSize)+' '\
			+ str(paretoShape)+' '\
			+ str(flow_cdf)+' '\
			+ str(enableMultiPath)+' '\
			+ str(perflowMP)+' '\
			+ str(exp["sourceAlg"])+' '\
			+ str(initWindow)+' '\
			+ str(ackRatio)+' '\
			+ str(slowstartrestart)+' '\
			+ str(DCTCP_g)+' '\
			+ str(min_rto)+' '\
			+ str(prob_cap_)+' '\
			+ str(switchAlg)+' '\
			+ str(DCTCP_K)+' '\
			+ str(drop_prio_)+' '\
			+ str(prio_scheme_)+' '\
			+ str(deque_prio_)+' '\
			+ str(keep_order_)+' '\
			+ str(prio_num_)+' '\
			+ str(ECN_scheme_)+' '\
			+ str(pias_thresh_0)+' '\
			+ str(pias_thresh_1)+' '\
			+ str(pias_thresh_2)+' '\
			+ str(pias_thresh_3)+' '\
			+ str(pias_thresh_4)+' '\
			+ str(pias_thresh_5)+' '\
			+ str(pias_thresh_6)+' '\
			+ str(exp["dc_size"][2])+' '\
			+ str(exp["dc_size"][1])+' '\
			+ str(exp["dc_size"][0])+' '\
			+ str(exp["oversub"])+' '\
			+ str(mig_sizes[0] * 1000000)+' '\
			+ str(mig_sizes[1] * 1000000)+' '\
			+ str(mig_sizes[2] * 1000000)+' '\
			+ str(exp["parallel_mig"])+' '\
			+ str(exp["run_migration"])+' '\
			+ str(exp["stat_record_interval"])+' '\
			+ str(exp["orch_type"])+' '\
			+ str(exp["enable_prioritization"])+' '\
			+ str(exp["src_zone_delay"])+' '\
			+ str(exp["dst_zone_delay"])+' '\
			+ str(exp["enable_bg_traffic"])+' '\
			+ str(exp["vm_flow_size"])+' '\
			+ str(exp["enable_rt_dv"])+' '\
			+ str(exp["tree_shape"][0])+' '\
			+ str(exp["tree_shape"][1])+' '\
			+ str(exp["tree_shape"][2])+' '\
			+ str('./'+directory_name+'/flow.tr')+'  >'\
			+ str('./'+directory_name+'/logFile.tr')
	else: 
		sim_script = "topo_dumbbell.tcl"
		print "implement dumbbell script"

	q.put([cmd, directory_name])


# main 
if __name__ == "__main__":

	# define the configs to do experiments over 
	# don't forget to set a proper name for the experiment

	#check if enough arguments are given
	if len(sys.argv) < 2:
		print "Usage: python run_exp.py <exp_name>"
		exit(0)

	exp_name = sys.argv[1]

	if exp_name == "mig_effect":
		configs = {
			"exp_name": ["mig_effect"],
			"mig_sizes": [(200, 100, 10)],
			"parallel_mig": [2], 
			"load": [0.6, 0.7, 0.8, 0.9, 1.0],
			"oversub": [2.0],
			"sourceAlg":['TCP'],
			"network_topo": ["datacenter"], # "dumbell" 
			"run_migration": ["yes", "no"], # "no", "skip"
			"enable_prioritization": [1], # 0: disable, 1: enable
			"orch_type": [1], # 1: bottom-up, 2: top-down, 3: random
			###########################################################
			########| don't make a list out of the following |#########
			###########################################################
			"enable_rt_dv": [1], # 0: disable, 1: enable
			"enable_bg_traffic": [1], # 0: disable, 1: enable
			"stat_record_interval": [0.0001], # in seconds
			"src_zone_delay": [0.00001], # in seconds
			"dst_zone_delay": [0.00002], # in seconds 
			"sim_end": [100000], # number of flows
			"vm_flow_size": [1000000], # in packets,
			"dc_size": [(2, 2, 16)], # (spines, bg_tors, spt)
			"tree_shape": [(2, 2, 2)], #branching factors of the tree
		} 

	elif exp_name == "test":
		configs = {
			"exp_name": ["test"],
			"mig_sizes": [(1, 1, 1)],
			"parallel_mig": [2], 
			"load": [0.1],
			"oversub": [2.0],
			"sourceAlg":['TCP'],
			"network_topo": ["datacenter"], # "dumbell" 
			"run_migration": ["yes"], # "no", "skip"
			"enable_prioritization": [1], # 0: disable, 1: enable
			"orch_type": [1], # 1: bottom-up, 2: top-down, 3: random
			###########################################################
			########| don't make a list out of the following |#########
			###########################################################
			"enable_rt_dv": [1], # 0: disable, 1: enable
			"enable_bg_traffic": [1], # 0: disable, 1: enable
			"stat_record_interval": [0.0001], # in seconds
			"src_zone_delay": [0.00001], # in seconds
			"dst_zone_delay": [0.00002], # in seconds
			"sim_end": [500], # number of flows
			"vm_flow_size": [300000], # in packets
			"dc_size": [(1, 1, 5)], # (spines, bg_tors, spt)
			"tree_shape": [(1, 1, 2)], #branching factors of the tree
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

	
	exp_directory = "exps/" + configs["exp_name"][0]
	print("Summarizing results on %s" % exp_directory)
	os.system("python3 processing/summarize_exp.py -d %s -r" % exp_directory)

