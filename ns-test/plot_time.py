import matplotlib.pyplot as plt
import numpy as np
import os

rootdir = './'
logfile_name = 'logFile.tr'

x_axis_BW = {}
x_axis = []
y_axis = []

for subdir, dirs, files in os.walk(rootdir):
    if ("dumbbell" in subdir):
        splitted_params = subdir.split("_")
        if (len(splitted_params) == 3):
            parallel = splitted_params[1][1:]
            load = splitted_params[2][1:]
            # x_axis.append(load)
            if not load in x_axis_BW:
                x_axis_BW[load] = []
            x_axis.append(parallel)
            print(load)
            with open(os.path.join(subdir, logfile_name)) as f:
                lines = f.readlines()
                for line in lines:
                    if "migration finished" in line:
                        vals = line.split(']')
                        if vals[1].strip() == "migration finished":
                            # y_axis.append(float(vals[0][1:].strip()))
                            x_axis_BW[load].append(float(vals[0][1:].strip()))
# print(x_axis)
# print(y_axis)
print(x_axis_BW)
plt.plot(x_axis, y_axis)
plt.savefig("test.png")

# monitored_node = 99
# q1_time = [2.0]
# q1_len = [0]

# q2_time = [2.0]
# q2_len = [0]

# with open("tmp.out") as output:
#     for line in output:
#         if line.startswith("["):
#             vals = line.split("]")
#             if (len(vals)>2):
#                 time = float(vals[0][1:].strip())
#                 nf_type = vals[1].strip()[1:]
#                 node_number = int(vals[2][1:-1].split()[1].split("(")[0])
#                 q_number = 0
#                 # print(nf_type)
#                 if (nf_type in ["selbuf", "buffer"] and ("Buffering" in vals[3] or "releasing" in vals[3]) and node_number==monitored_node):
#                     q_number = 1 if "Q 1" in vals[3] else 2
#                     is_release = True if "releasing" in vals[3] else False
#                     # print(node_number)
#                     if (q_number == 1):
#                         q1_time.append(time)
#                         new_len = q1_len[-1]-1 if is_release else q1_len[-1]+1
#                         q1_len.append(new_len)
#                     else:
#                         q2_time.append(time)
#                         new_len = q2_len[-1]-1 if is_release else q2_len[-1]+1
#                         q2_len.append(new_len)

# print(q2_len)

# plt.plot(q1_time, q1_len, label="Q1")
# plt.plot(q2_time, q2_len, label="Q2")
# plt.legend()
# plt.savefig("test.png")