import sys
sys.dont_write_bytecode = True

# Optional fabric import for remote runs only
try:
    from fabric import Connection
except Exception:
    Connection = None
import itertools
import threading
import os
import queue
from run_constants import * 
import time

# setup basics 

exp_q = queue.Queue()

threads = []
# Restore default worker threads for full experiment
number_worker_threads = 30
exp_rep_count = 1

DEBUG_VALGRIND = False
DEBUG_GDB = False
REMOTE_RUN = False

ns_path = '../ns'
if DEBUG_VALGRIND:
	ns_path = 'valgrind -s --track-origins=yes --leak-check=full ../ns'
if DEBUG_GDB:
	ns_path = 'gdb -ex "run" --args ../ns'


# helper functions

def process_results(exp_directory):
	print("Processing results on %s" % exp_directory)
	os.system("./processing/process_setting.sh %s" % exp_directory)
	os.system("python3 processing/summarize_setting.py -d %s" % exp_directory)

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
		# os.system('rm -rf {}/data/'.format(directory_name))
		# os.system('rm -rf {}/plots/'.format(directory_name))
		# os.system('rm {}/logFile.tr'.format(directory_name))
		# os.system('rm {}/flow.tr'.format(directory_name))


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
		
		# delete the useless files
		os.system('rm -rf {}/data/'.format(directory_name))
		os.system('rm -rf {}/plots/'.format(directory_name))
		os.system('rm {}/logFile.tr'.format(directory_name))
		os.system('rm {}/flow.tr'.format(directory_name))
		with conn.cd(script_dir):
			conn.run('rm -rf ' + directory_name)


def setup_exp(exp):
	mig_sizes = exp["mig_sizes"]
	exp_name = exp["exp_name"]

	if exp["run_migration"] == "yes":
		mig_str = "mig"
	elif exp["run_migration"] == "no":
		mig_str = "nomig"
	elif exp["run_migration"] == "skip":
		mig_str = "skipmig"

	directory_name = 'exps/%s/%s_%s_P%d_L%d_O%d_MS%s_A%s_OR%d_Pr%s_Sd%d_Dd%d_Td%d_B%d_i%d' % (
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
		exp["iteration"],
	)

	directory_name = directory_name.lower()

	cdf_file = "CDF_" + exp["bg_traffic_cdf"][0] + ".tcl"
	mean_flow_size = exp["bg_traffic_cdf"][1]
	
	if exp_name == "random_test":
		gnrtd_flows = exp["sim_end"]
	else:	
		gnrtd_flows = int(exp["sim_end"] * exp["load"] * (1138.0 / mean_flow_size))
 
	if exp["network_topo"] == "datacenter":
		sim_script = 'topo_spine_empirical.tcl'
		cmd_parts = [
			ns_path,
			sim_script,
			str(gnrtd_flows),  # 0
			str(exp["link_rate"]),  # 1
			str(mean_link_delay),  # 2
			str(host_delay),  # 3
			str(queueSize),  # 4
			str(exp["load"]),  # 5
			str(connections_per_pair),  # 6
			str(mean_flow_size * 1460),  # 7
			str(paretoShape),  # 8
			str(cdf_file),  # 9
			str(enableMultiPath),  # 10
			str(perflowMP),  # 11
			str(exp["Protocol"][0]),  # 12
			str(initWindow),  # 13
			str(ackRatio),  # 14
			str(slowstartrestart),  # 15
			str(DCTCP_g),  # 16
			str(min_rto),  # 17
			str(prob_cap_),  # 18
			str(exp["Protocol"][1]),  # 19
			str(DCTCP_K),  # 20
			str(drop_prio_),  # 21
			str(prio_scheme_),  # 22
			str(deque_prio_),  # 23
			str(keep_order_),  # 24
			str(prio_num_),  # 25
			str(ECN_scheme_),  # 26
			str(pias_thresh_0),  # 27
			str(pias_thresh_1),  # 28
			str(pias_thresh_2),  # 29
			str(pias_thresh_3),  # 30
			str(pias_thresh_4),  # 31
			str(pias_thresh_5),  # 32
			str(pias_thresh_6),  # 33
			str(exp["dc_size"][2]),  # 34
			str(exp["dc_size"][1]),  # 35
			str(exp["dc_size"][0]),  # 36
			str(exp["oversub"]),  # 37
			str(mig_sizes[0] * 1000000),  # 38
			str(mig_sizes[1] * 1000000),  # 39
			str(mig_sizes[2] * 1000000),  # 40
			str(exp["parallel_mig"]),  # 41
			str(exp["run_migration"]),  # 42
			str(exp["stat_record_interval"]),  # 43
			str(exp["orch_type"]),  # 44
			str(exp["prioritization"]),  # 45
			str(exp["src_zone_delay"]),  # 46
			str(exp["dst_zone_delay"]),  # 47
			str(exp["enable_bg_traffic"]),  # 48
			str(exp["vm_flow_size"]),  # 49
			str(exp["enable_rt_dv"]),  # 50
			str(exp["tree_shape"][0]),  # 51
			str(exp["tree_shape"][1]),  # 52
			str(exp["tree_shape"][2]),  # 53
			str(exp["traffic_zone_delay"]),  # 54
			str(exp["iteration"]),  # 55
			'./' + directory_name + '/flow.tr',  # 56
			'>' + './' + directory_name + '/logFile.tr'  
		]
		cmd = ' '.join(cmd_parts)
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

 
	# step back a dir, run make, then come back
	
	os.chdir("../")
	print("running make...")
	ret = os.system("make -j")
	if ret != 0:
		print("Make failed with exit code", ret)
		exit(1)
	os.chdir("ns-test")
	print("make done.")
 
	exp_name = sys.argv[1]

	configs = None

	if exp_name == "random_test":
		configs = [{
			"mig_sizes": [(20, 20, 20)],
			"parallel_mig": [1], 
			"load": [0.01],
			"oversub": [1.0],
			"src_zone_delay": [0.00002], # in seconds
			"dst_zone_delay": [0.00002], # in seconds
			"traffic_zone_delay": [0.01], # in seconds
			"network_topo": ["datacenter"], # "dumbell" 
			"run_migration": ["yes", "no"], # full grid
			"prioritization": [0, 1, 2], # full grid
			"orch_type": [1, 2, 3, 4, 5], # full grid
			"bg_traffic_cdf": [("dctcp", 1138)],
			"Protocol": [("DCTCP", "MamadQueue")], 
			"link_rate": [10],
			###########################################################
			########| don't make a list out of the following |#########
			###########################################################
			"exp_name": [exp_name],
			"enable_rt_dv": [1], # 0: disable, 1: enable
			"enable_bg_traffic": [0], # 0: disable, 1: enable
			"stat_record_interval": [0.001], # in seconds
			"sim_end": [100], # number of flows
			"vm_flow_size": [30000], # in packets
			"dc_size": [(1, 1, 16)], # (spines, bg_tors, spt)
			"tree_shape": [(2, 2, 2)], #branching factors of the tree
		}]

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

	elif exp_name == "paper_tests": 
		# = 64 + 84 + 32 + 7 + 336 + 50 = 
		# = 148 + 39 + 386 = 
		# = 187 + 386 = 400 + 160 + 13 = 573
		# = 3 round of execution of 6 machines of 35 threads.

		configs = [ 
			# 2 * 2 * 3 * 2 = 24 settings 
			# 24 settings * 20 iterations = 480 runs 
			{	
				"mig_sizes": [(1, 10, 10), (1, 50, 50)],
				"parallel_mig": [1], 
				"load": [0.5],
				"oversub": [2.0],
				"src_zone_delay": [0.000005], # in seconds
				"dst_zone_delay": [0.000005], # in seconds 
				"traffic_zone_delay": [0.01], # in seconds
				"network_topo": ["datacenter"], # "dumbell" 
				"run_migration": ["no", "yes"], # "no", "skip"
				"prioritization": [0, 1, 2], # 0: disable, 1: enable_lvl_1, 2: enable_lvl_2
				"orch_type": [1, 2], # 1: bottom-up, 2: top-down, 3: random
				"bg_traffic_cdf": [("dctcp", 1138)],
				"Protocol": [("DCTCP", "MamadQueue")], 
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

			# 2 * 13 * 3  = 26 * 3 = 78 settings 
			# 78 * 20 = 1400 + 160 = 1560 runs  
			{	
				"mig_sizes": [(1, 10, 10), (1, 50, 50)],
				"parallel_mig": [1], 
				"load": [0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5, 0.55, 0.6, 0.65, 0.7],
				"oversub": [2.0],
				"src_zone_delay": [0.000005], # in seconds
				"dst_zone_delay": [0.000005], # in seconds 
				"traffic_zone_delay": [0.01], # in seconds
				"network_topo": ["datacenter"], # "dumbell" 
				"run_migration": ["yes"], # "no", "skip"
				"prioritization": [0, 1, 2], # 0: disable, 1: enable_lvl_1, 2: enable_lvl_2
				"orch_type": [1], # 1: bottom-up, 2: top-down, 3: random
				"bg_traffic_cdf": [("dctcp", 1138)],
				"Protocol": [("DCTCP", "MamadQueue")], 
				"link_rate": [10],
				###########################################################
				########| don't make a list out of the following |#########
				###########################################################
				"exp_name": ["prio_test"],
				"enable_rt_dv": [1], # 0: disable, 1: enable
				"enable_bg_traffic": [1], # 0: disable, 1: enable
				"stat_record_interval": [0.001], # in seconds
				"sim_end": [500000], # number of flows
				"vm_flow_size": [10000], # in packets,
				"dc_size": [(3, 8, 16)], # (spines, bg_tors, spt)
				"tree_shape": [(2, 2, 2)], #branching factors of the tree
			},

			# 2 * 8 * 4 * 3 = 16 * 12 = 192 settings 
			# 192 * 20 = 3840 runs 
			{	
				"mig_sizes": [(1, 10, 10), (1, 50, 50)],
				"parallel_mig": [1, 2, 3, 4, 5, 6, 7, 8], 
				"load": [0.5],
				"oversub": [2.0, 4.0, 8.0, 16.0],
				"src_zone_delay": [0.000005], # in seconds
				"dst_zone_delay": [0.000005], # in seconds 
				"traffic_zone_delay": [0.01], # in seconds
				"network_topo": ["datacenter"], # "dumbell" 
				"run_migration": ["yes"], # "no", "skip"
				"prioritization": [0, 1, 2], # 0: disable, 1: enable_lvl_1, 2: enable_lvl_2
				"orch_type": [1], # 1: bottom-up, 2: top-down, 3: random
				"bg_traffic_cdf": [("dctcp", 1138)],
				"Protocol": [("DCTCP", "MamadQueue")], 
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

			# 2 * 13 * 2 * 3 * 2 * 2 = 26 * 6 * 4 = 26 * 24 = 624 
			# 624 * 20 = 12480 
			{	
				"mig_sizes": [(1, 10, 10), (1, 50, 50)],
				"parallel_mig": [1], 
				"load": [0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5, 0.55, 0.6, 0.65, 0.7],
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

			# 5 * 5 * 2 = 50 
			# 50 * 20 = 1000 runs
			{		
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
		config["iteration"] = range(exp_rep_count)
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
				time.sleep(0.1)
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
