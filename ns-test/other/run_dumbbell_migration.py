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

vm_precopy_size = 100000000
vm_snapshot_size = 10000000
gw_snapshot_size = 1000000

# parallel_mig_arr = [8,2,1]
parallel_mig_arr = [1]
# BW_arr = ["40Gb", "10Gb", "1Gb"]
# BW_arr = ["10Gb", "1Gb"]
BW_arr = ["10Gb"]
sim_script = 'logicaltree.tcl'

ns_path = 'ns'

for BW in BW_arr:
    for parallel_mig in parallel_mig_arr:
        #Directory name: workload_scheme_load_[load]
        directory_name = 'dumbbell_P%d_B%s' % (parallel_mig, BW)
        directory_name = directory_name.lower()
        #Simulation command
        cmd = ns_path+' '+sim_script+' '\
            +str(parallel_mig)+' '\
            +str(BW)+'  >'\
            +str('./'+directory_name+'/logFile.tr')

        q.put([cmd, directory_name])

#Create all worker threads
threads = []
number_worker_threads = 8

#Start threads to process jobs
for i in range(number_worker_threads):
	t = threading.Thread(target = worker)
	threads.append(t)
	t.start()

#Join all completed threads
for t in threads:
	t.join()