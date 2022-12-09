link_rate = 10
mean_link_delay = 0.000005
host_delay = 0.000010
queueSize = 240
connections_per_pair = 1
meanFlowSize = 1138*1460
paretoShape = 1.05

enableMultiPath = 1
perflowMP = 0

initWindow = 70
ackRatio = 1
slowstartrestart = 'false'
DCTCP_g = 0.0625
min_rto = 0.2
prob_cap_ = 5

switchAlg = 'MyQueue'
DCTCP_K = 65.0
drop_prio_ = 'true'
# prio_scheme_arr = [2,3]
prio_scheme_ = 0
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



#===========
# The following constants are only for remote runs
#===========

# Repo constants
REPO_DIR = '~/ns-allinone-2.34/'

# Remote machines 
USER = 'nsuser'
# USER = 'sepehr'

MACHINES = [
    # '10.70.10.101',
    '10.70.10.102',
    '10.70.10.104',
    '10.70.10.105',
    '10.70.10.106',
    '10.70.10.107',
    '10.70.10.108',
    '10.70.10.110',
]

master_machine = '10.70.10.101'
