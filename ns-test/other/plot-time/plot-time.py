import matplotlib.pyplot as plt
import numpy as np
import os
import pandas as pd
import shutil
rootdir = './data-dumbbell'
dst_dir = "./figs/dumbbell-load/"
logfile_name = 'logFile.tr'
test_case = 'dumbbell'
mig_start_time = 2
replace_vm_label = True
data = []
pickled_path = os.path.join(rootdir, 'data.pickle')
if os.path.isfile(pickled_path):
    data = pd.read_pickle(pickled_path)
else: 
    for subdir, dirs, files in os.walk(rootdir):
        if (test_case in subdir):
            splitted_params = subdir.split("_")
            if (len(splitted_params) == 3):
                parallel = int(splitted_params[1][1:])
                load = float(splitted_params[2][1:])
                # oversub = int(splitted_params[3][1:])
                # vm_size = int(splitted_params[4][1:])
                # if replace_vm_label:
                #     vm_size = str(int(vm_size/10**6))+'MB'
                # cc_alg = splitted_params[5][1:]
                m_time = 0
                with open(os.path.join(subdir, logfile_name)) as f:
                    lines = f.readlines()
                    for line in lines:
                        if "migration finished" in line:
                            vals = line.split(']')
                            if vals[1].strip() == "migration finished":
                                # print(vals[0][1:].strip())
                                # to get the time
                                m_time = float(vals[0][1:].strip()) - mig_start_time
                                m_time = int(m_time*1000) # to make it in ms
                data.append([parallel, load,  m_time])
    data = pd.DataFrame(data)
    col_names = ['parallel', 'load', 'mig_time']
    data.set_axis(col_names, axis=1, inplace=True)
    pd.to_pickle(data, pickled_path)
    
print(data)
data = data[data['parallel']==4]
data = data.drop('parallel', 1)
print(data)
# data = data[data['load']=='90']
# data = data.drop('load', 1)
# df_pivot = pd.pivot_table(
#     data, 
#     values='mig_time',
#     index='parallel',
#     columns='vm_size'
# )
# ax = df_pivot.plot(kind='bar', ylabel='Time (ms)', logy=True, xlabel='number of parallel VM migrations')
data.set_index('load', inplace=True)
ax = data.sort_index().plot(kind='bar', ylabel='Time (ms)', logy=False, xlabel='Imposed load in the DC')
ax.set_ylim(min(data['mig_time']-40), max(data['mig_time']+40))
# ax.legend(title="VM precopy size",loc='center right', bbox_to_anchor=(1, 1))
if (not os.path.exists(dst_dir)):
    os.mkdir(dst_dir)
shutil.copy(os.path.abspath(__file__), os.path.join(dst_dir, "plotter.py"))
plt.savefig(os.path.join(dst_dir, "TIME.png"))
plt.savefig(os.path.join(dst_dir, "TIME.svg"))