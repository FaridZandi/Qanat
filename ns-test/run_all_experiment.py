import sys
sys.dont_write_bytecode = True

from fabric import Connection
import itertools
import threading
import os
import queue
from run_constants import * 


# setup basics 

exp_q = queue.Queue()

threads = []
number_worker_threads = 35

DEBUG_VALGRIND = False
DEBUG_GDB = False
REMOTE_RUN = True

ns_path = '../ns'
if DEBUG_VALGRIND:
	ns_path = 'valgrind -s --track-origins=yes --leak-check=full ../ns'
if DEBUG_GDB:
	ns_path = 'gdb -ex "run" --args ../ns'


# helper functions

def process_results(exp_directory):
	print("Processing results on %s" % exp_directory)
	os.system("./processing/process_setting.sh %s" % exp_directory)


def worker():
	while True:
		try:
			exp = exp_q.get(block = 0)
		except queue.Empty:
			return

		command, directory_name, exp_name = setup_exp(exp)
		
		print("running exp on ", directory_name)

		os.system('mkdir -p ' + "exps/%s/" % exp_name)
		os.system('mkdir -p ' + directory_name)

		os.system(command)

		process_results(directory_name)

def remote_worker(m_ip):
	conn = Connection('{}@{}'.format(USER, m_ip))
	while True:
		try:
			exp = exp_q.get(block = 0)
		except queue.Empty:
			return
		
		command, directory_name, exp_name = setup_exp(exp)
		
		print("worker", m_ip, "running", command)

		script_dir = '/home/{}/ns-allinone-2.34/ns-2.34/ns-test'.format(USER)
		with conn.cd(script_dir):
			conn.run('rm -rf ' + directory_name)
			conn.run('mkdir -p ' + "exps/%s/" % exp_name)
			conn.run('mkdir -p ' + directory_name)
			conn.run(command)
			zipped_file = '{}.tar.gz'.format(directory_name.split('/')[-1])
			cmd = 'tar -czvf {} {}/*'.format(zipped_file, directory_name)
			conn.run(cmd)
			# download the log files to the local machine
			conn.get(script_dir+"/"+zipped_file)
		# process them locally
		os.system('tar -xzvf {} && rm {}'.format(zipped_file, zipped_file))
		process_results(directory_name)

def setup_exp(exp):

	mig_sizes = exp["mig_sizes"]
	exp_name = exp["exp_name"]

	if exp["run_migration"] == "yes":
		mig_str = "mig"
	elif exp["run_migration"] == "no":
		mig_str = "nomig"
	elif exp["run_migration"] == "skip":
		mig_str = "skipmig"

	directory_name = 'exps/%s/%s_%s_P%d_L%d_O%d_MS%s_A%s_OR%d_Pr%s_Sd%d_Dd%d_Td%d_B%d' % (
		exp["exp_name"],
		exp["bg_traffic_cdf"][0], 
		mig_str,
		exp["parallel_mig"],
		int(exp["load"]*100), 
		exp["oversub"], 
		str(mig_sizes[0]) + "-" + str(mig_sizes[1]) + "-" + str(mig_sizes[2]),
		exp["Protocol"][0],
		exp["orch_type"],
		str(exp["prioritization"]),
		int(exp["src_zone_delay"] * 1000000),
		int(exp["dst_zone_delay"] * 1000000),
		int(exp["traffic_zone_delay"] * 1000000),
		exp["link_rate"], 
	)

	directory_name = directory_name.lower()

	cdf_file = "CDF_" + exp["bg_traffic_cdf"][0] + ".tcl"
	mean_flow_size = exp["bg_traffic_cdf"][1]
	gnrtd_flows = int(exp["sim_end"] * exp["load"] * (1138.0 / mean_flow_size))
	
	if exp["network_topo"] == "datacenter":
		sim_script = 'topo_spine_empirical.tcl'
		cmd = ns_path + ' ' + sim_script + ' ' \
			+ str(gnrtd_flows)+' '\
			+ str(exp["link_rate"])+' '\
			+ str(mean_link_delay)+' '\
			+ str(host_delay)+' '\
			+ str(queueSize)+' '\
			+ str(exp["load"])+' '\
			+ str(connections_per_pair)+' '\
			+ str(mean_flow_size * 1460)+' '\
			+ str(paretoShape)+' '\
			+ str(cdf_file)+' '\
			+ str(enableMultiPath)+' '\
			+ str(perflowMP)+' '\
			+ str(exp["Protocol"][0])+' '\
			+ str(initWindow)+' '\
			+ str(ackRatio)+' '\
			+ str(slowstartrestart)+' '\
			+ str(DCTCP_g)+' '\
			+ str(min_rto)+' '\
			+ str(prob_cap_)+' '\
			+ str(exp["Protocol"][1])+' '\
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
			+ str(exp["prioritization"])+' '\
			+ str(exp["src_zone_delay"])+' '\
			+ str(exp["dst_zone_delay"])+' '\
			+ str(exp["enable_bg_traffic"])+' '\
			+ str(exp["vm_flow_size"])+' '\
			+ str(exp["enable_rt_dv"])+' '\
			+ str(exp["tree_shape"][0])+' '\
			+ str(exp["tree_shape"][1])+' '\
			+ str(exp["tree_shape"][2])+' '\
			+ str(exp["traffic_zone_delay"])+' '\
			+ str('./'+directory_name+'/flow.tr')+'  >'\
			+ str('./'+directory_name+'/logFile.tr')
	else: 
		pass

	return (cmd, directory_name, exp_name)

# main 
if __name__ == "__main__":

	# define the configs to do experiments over 
	# don't forget to set a proper name for the experiment

	#check if enough arguments are given
	if len(sys.argv) < 2:
		print("Usage: python run_exp.py <exp_name>")
		exit(0)

	exp_name = sys.argv[1]

	configs = None

	if exp_name == "vm_test":
		configs = [{
			"mig_sizes": [(1000, 10, 10)],
			"parallel_mig": [1], 
			"load": [0.01],
			"oversub": [1.0],
			"src_zone_delay": [0.00002], # in seconds
			"dst_zone_delay": [0.00002], # in seconds
			"traffic_zone_delay": [0.01], # in seconds
			"network_topo": ["datacenter"], # "dumbell" 
			"run_migration": ["yes"], # "no", "yes"
			"prioritization": [0, 1, 2], # 0: disable, 1: enable_lvl_1, 2: enable_lvl_2
			"orch_type": [1, 2], # 1: bottom-up, 2: top-down, 3: random
			"bg_traffic_cdf": [("dctcp", 1138)],
			"Protocol": [("DCTCP", "MamadQueue")], 
			"link_rate": [10],
			###########################################################
			########| don't make a list out of the following |#########
			###########################################################
			"exp_name": [exp_name],
			"enable_rt_dv": [0], # 0: disable, 1: enable
			"enable_bg_traffic": [0], # 0: disable, 1: enable
			"stat_record_interval": [0.001], # in seconds
			"sim_end": [100000], # number of flows
			"vm_flow_size": [100000], # in packets
			"dc_size": [(1, 1, 16)], # (spines, bg_tors, spt)
			"tree_shape": [(2, 2, 2)], #branching factors of the tree
		}]

	elif exp_name == "more_paper_tests":
		configs = [{# 2 * 8 * 2 = 32 
				"mig_sizes": [(1, 10, 10), (1, 50, 50)],
				"parallel_mig": [1, 2, 3, 4, 5, 6, 7, 8], 
				"load": [0.5],
				"oversub": [4.0, 8.0],
				"src_zone_delay": [0.000005], # in seconds
				"dst_zone_delay": [0.000005], # in seconds 
				"traffic_zone_delay": [0.01], # in seconds
				"network_topo": ["datacenter"], # "dumbell" 
				"run_migration": ["yes"], # "no", "skip"
				"prioritization": [1], # 0: disable, 1: enable_lvl_1, 2: enable_lvl_2
				"orch_type": [1], # 1: bottom-up, 2: top-down, 3: random
				"bg_traffic_cdf": [("dctcp", 1138)],
				"Protocol": [("TCP", "MyQueue"), ("DCTCP", "MamadQueue")], 
				"link_rate": [10],
				###########################################################
				########| don't make a list out of the following |#########
				###########################################################
				"exp_name": ["parallel_test"],
				"enable_rt_dv": [1], # 0: disable, 1: enable
				"enable_bg_traffic": [1], # 0: disable, 1: enable
				"stat_record_interval": [0.001], # in seconds
				"sim_end": [500000], # number of flows
				"vm_flow_size": [100000], # in packets,
				"dc_size": [(3, 8, 16)], # (spines, bg_tors, spt)
				"tree_shape": [(2, 2, 2)], #branching factors of the tree
			}]

	elif exp_name == "paper_tests": 
		
		# = 64 + 84 + 32 + 7 + 336 + 50 = 
		# = 148 + 39 + 386 = 
		# = 187 + 386 = 400 + 160 + 13 = 573
		# = 3 round of execution of 6 machines of 35 threads.

		configs = [ # 2 * 2 * 2 * 2 * 2  * 2 = 64 
			{	
				"mig_sizes": [(1, 10, 10), (1, 50, 50)],
				"parallel_mig": [1, 2], 
				"load": [0.5],
				"oversub": [2.0],
				"src_zone_delay": [0.000005], # in seconds
				"dst_zone_delay": [0.000005], # in seconds 
				"traffic_zone_delay": [0.01], # in seconds
				"network_topo": ["datacenter"], # "dumbell" 
				"run_migration": ["no", "yes"], # "no", "skip"
				"prioritization": [1, 2], # 0: disable, 1: enable_lvl_1, 2: enable_lvl_2
				"orch_type": [1, 2], # 1: bottom-up, 2: top-down, 3: random
				"bg_traffic_cdf": [("dctcp", 1138)],
				"Protocol": [("TCP", "MyQueue"), ("DCTCP", "MamadQueue")], 
				"link_rate": [10],
				###########################################################
				########| don't make a list out of the following |#########
				###########################################################
				"exp_name": ["orch_test"],
				"enable_rt_dv": [1], # 0: disable, 1: enable
				"enable_bg_traffic": [1], # 0: disable, 1: enable
				"stat_record_interval": [0.001], # in seconds
				"sim_end": [500000], # number of flows
				"vm_flow_size": [100000], # in packets,
				"dc_size": [(3, 8, 16)], # (spines, bg_tors, spt)
				"tree_shape": [(2, 2, 2)], #branching factors of the tree
			},


			{	# 2 * 7 * 2 * 3 = 14 * 6 = 84 
				"mig_sizes": [(1, 10, 10), (1, 50, 50)],
				"parallel_mig": [1], 
				"load": [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7],
				"oversub": [2.0],
				"src_zone_delay": [0.000005], # in seconds
				"dst_zone_delay": [0.000005], # in seconds 
				"traffic_zone_delay": [0.01], # in seconds
				"network_topo": ["datacenter"], # "dumbell" 
				"run_migration": ["yes"], # "no", "skip"
				"prioritization": [0, 1, 2], # 0: disable, 1: enable_lvl_1, 2: enable_lvl_2
				"orch_type": [1], # 1: bottom-up, 2: top-down, 3: random
				"bg_traffic_cdf": [("dctcp", 1138)],
				"Protocol": [("TCP", "MyQueue"), ("DCTCP", "MamadQueue")], 
				"link_rate": [10],
				###########################################################
				########| don't make a list out of the following |#########
				###########################################################
				"exp_name": ["prio_test"],
				"enable_rt_dv": [1], # 0: disable, 1: enable
				"enable_bg_traffic": [1], # 0: disable, 1: enable
				"stat_record_interval": [0.001], # in seconds
				"sim_end": [500000], # number of flows
				"vm_flow_size": [100000], # in packets,
				"dc_size": [(3, 8, 16)], # (spines, bg_tors, spt)
				"tree_shape": [(2, 2, 2)], #branching factors of the tree
			},

			{	# 2 * 8 * 2 = 32 
				"mig_sizes": [(1, 10, 10), (1, 50, 50)],
				"parallel_mig": [1, 2, 3, 4, 5, 6, 7, 8], 
				"load": [0.5],
				"oversub": [2.0],
				"src_zone_delay": [0.000005], # in seconds
				"dst_zone_delay": [0.000005], # in seconds 
				"traffic_zone_delay": [0.01], # in seconds
				"network_topo": ["datacenter"], # "dumbell" 
				"run_migration": ["yes"], # "no", "skip"
				"prioritization": [1], # 0: disable, 1: enable_lvl_1, 2: enable_lvl_2
				"orch_type": [1], # 1: bottom-up, 2: top-down, 3: random
				"bg_traffic_cdf": [("dctcp", 1138)],
				"Protocol": [("TCP", "MyQueue"), ("DCTCP", "MamadQueue")], 
				"link_rate": [10],
				###########################################################
				########| don't make a list out of the following |#########
				###########################################################
				"exp_name": ["parallel_test"],
				"enable_rt_dv": [1], # 0: disable, 1: enable
				"enable_bg_traffic": [1], # 0: disable, 1: enable
				"stat_record_interval": [0.001], # in seconds
				"sim_end": [500000], # number of flows
				"vm_flow_size": [100000], # in packets,
				"dc_size": [(3, 8, 16)], # (spines, bg_tors, spt)
				"tree_shape": [(2, 2, 2)], #branching factors of the tree
			},

			

			{	# 7 
				"mig_sizes": [
					(1, 10, 10), (1, 50, 50), 
					(1, 100, 100), (1, 200, 200),
					(1, 50, 10), (1, 100, 10), (1, 200, 10)
				],
				"parallel_mig": [1], 
				"load": [0.5],
				"oversub": [2.0],
				"src_zone_delay": [0.000005], # in seconds
				"dst_zone_delay": [0.000005], # in seconds 
				"traffic_zone_delay": [0.01], # in seconds
				"network_topo": ["datacenter"], # "dumbell" 
				"run_migration": ["yes"], # "no", "skip"
				"prioritization": [1], # 0: disable, 1: enable_lvl_1, 2: enable_lvl_2
				"orch_type": [1], # 1: bottom-up, 2: top-down, 3: random
				"bg_traffic_cdf": [("dctcp", 1138)],
				"Protocol": [("DCTCP", "MamadQueue")], 
				"link_rate": [10],
				###########################################################
				########| don't make a list out of the following |#########
				###########################################################
				"exp_name": ["size_test"],
				"enable_rt_dv": [1], # 0: disable, 1: enable
				"enable_bg_traffic": [1], # 0: disable, 1: enable
				"stat_record_interval": [0.001], # in seconds
				"sim_end": [500000], # number of flows
				"vm_flow_size": [100000], # in packets,
				"dc_size": [(3, 8, 16)], # (spines, bg_tors, spt)
				"tree_shape": [(2, 2, 2)], #branching factors of the tree
			},


			{	# 2 * 7 * 3 * 2 * 2 * 2 = 336
				"mig_sizes": [(1, 10, 10), (1, 50, 50)],
				"parallel_mig": [1], 
				"load": [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7],
				"oversub": [2.0],
				"src_zone_delay": [0.000005], # in seconds
				"dst_zone_delay": [0.000005], # in seconds 
				"traffic_zone_delay": [0.01], # in seconds
				"network_topo": ["datacenter"], # "dumbell" 
				"run_migration": ["no", "yes"], # "no", "skip"
				"prioritization": [0, 1, 2], # 0: disable, 1: enable_lvl_1, 2: enable_lvl_2
				"orch_type": [1], # 1: bottom-up, 2: top-down, 3: random
				"bg_traffic_cdf": [("dctcp", 1138), ("vl2", 5117)],
				"Protocol": [("TCP", "MyQueue"), ("DCTCP", "MamadQueue")], 
				"link_rate": [10],
				###########################################################
				########| don't make a list out of the following |#########
				###########################################################
				"exp_name": ["bg_test"],
				"enable_rt_dv": [1], # 0: disable, 1: enable
				"enable_bg_traffic": [1], # 0: disable, 1: enable
				"stat_record_interval": [0.001], # in seconds
				"sim_end": [500000], # number of flows
				"vm_flow_size": [100000], # in packets,
				"dc_size": [(3, 8, 16)], # (spines, bg_tors, spt)
				"tree_shape": [(2, 2, 2)], #branching factors of the tree
			},


			{	# 5 * 5 * 2 = 50 
				"mig_sizes": [(1, 10, 10), (1, 50, 50)],
				"parallel_mig": [1], 
				"load": [0.5],
				"oversub": [2.0],
				"src_zone_delay": [0.000005, 0.00005, 0.0005, 0.005], # in seconds
				"dst_zone_delay": [0.000005, 0.00005, 0.0005, 0.005], # in seconds 
				"traffic_zone_delay": [0.01], # in seconds
				"network_topo": ["datacenter"], # "dumbell" 
				"run_migration": ["no", "yes"], # "no", "skip"
				"prioritization": [1], # 0: disable, 1: enable_lvl_1, 2: enable_lvl_2
				"orch_type": [1], # 1: bottom-up, 2: top-down, 3: random
				"bg_traffic_cdf": [("dctcp", 1138)],
				"Protocol": [("DCTCP", "MamadQueue")], 
				"link_rate": [10],
				###########################################################
				########| don't make a list out of the following |#########
				###########################################################
				"exp_name": ["latency_test"],
				"enable_rt_dv": [1], # 0: disable, 1: enable
				"enable_bg_traffic": [1], # 0: disable, 1: enable
				"stat_record_interval": [0.001], # in seconds
				"sim_end": [500000], # number of flows
				"vm_flow_size": [100000], # in packets,
				"dc_size": [(3, 8, 16)], # (spines, bg_tors, spt)
				"tree_shape": [(2, 2, 2)], #branching factors of the tree
			},
		]
	


	if not configs:
		print("No configs found for experiment: ", exp_name)
		exit(0)
	
	for config in configs:
		# Add all possible experiments to the queue
		keys, values = zip(*config.items())
		permutations_dicts = [dict(zip(keys, v)) for v in itertools.product(*values)]
		for exp in permutations_dicts:
			exp_q.put(exp)

	# Start threads to process jobs
	if REMOTE_RUN:
		for i in range(number_worker_threads):
			for m_ip in MACHINES:
				args = (m_ip,)
				t = threading.Thread(target = remote_worker, args=args)
				threads.append(t)
				t.start()
	else:
		for i in range(number_worker_threads):
			t = threading.Thread(target = worker)
			threads.append(t)
			t.start()

	# Join all completed threads
	for t in threads:
		t.join()

	for config in configs:
		exp_directory = "exps/" + config["exp_name"][0]
		print("Summarizing results on %s" % exp_directory)
		os.system("python3 processing/summarize_exp.py -d %s -r" % exp_directory)
