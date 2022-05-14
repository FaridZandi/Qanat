import pandas as pd 
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Patch

times = {}
min_time = 10000
max_time = 0 

file_path = "protocol.out"

with open(file_path) as f: 
    for line in f.readlines(): 
        line = line.strip()

        if not line.startswith("["):
            continue 

        s = line.split(" ")

        if len(s) > 1 and s[1] != "Buffer":
            time_str = s[0]
            time = float(time_str[1:-1]) * 1000

            address_str = s[2]
            address = address_str[1:-1]

            if address == "":
                continue 

            if time > max_time:
                max_time = time 

            if min_time > time:
                min_time = time 

            if address not in times:
                times[address] = {
                    "type" : "", 
                    "start_pre": 0,
                    "end_pre": 0,
                    "start_mig": 0,
                    "end_mig": 0,
                    "start_buf": 0,
                    "end_buf": 0,
                }

            if s[3] == "start":
                if s[4] == "vm":
                    if s[5] == "precopy":
                        times[address]["type"] = "VM"
                        times[address]["start_pre"] = time

                    elif s[5] == "migration":
                        times[address]["type"] = "VM"
                        times[address]["start_mig"] = time

                    elif s[5] == "buffering":
                        times[address]["type"] = "VM"
                        times[address]["start_buf"] = time
                        
                elif s[4] == "gw":
                    if s[5] == "precopy":
                        times[address]["type"] = "GW"
                        times[address]["start_pre"] = time

                    if s[5] == "migration":
                        times[address]["type"] = "GW"
                        times[address]["start_mig"] = time

                    elif s[5] == "buffering":
                        times[address]["type"] = "GW"
                        times[address]["start_buf"] = time
                        
                
                    
            elif s[3] == "end":
                if s[4] == "vm":
                    if s[5] == "precopy":
                        times[address]["type"] = "VM"
                        times[address]["end_pre"] = time

                    elif s[5] == "migration":
                        times[address]["type"] = "VM"
                        times[address]["end_mig"] = time

                    elif s[5] == "buffering":
                        times[address]["type"] = "VM"
                        times[address]["end_buf"] = time
                        
                elif s[4] == "gw":
                    if s[5] == "precopy":
                        times[address]["type"] = "GW"
                        times[address]["end_pre"] = time

                    if s[5] == "migration":
                        times[address]["type"] = "GW"
                        times[address]["end_mig"] = time

                    elif s[5] == "buffering":
                        times[address]["type"] = "GW"
                        times[address]["end_buf"] = time



df = pd.DataFrame(columns=["address", "id", "layer", "which_tree", "type", "start_pre", "end_pre", "start_mig", "end_mig", "start_buf", "end_buf"])


for address in times: 

    uid = int(address.split("-")[2])

    if uid < 10:
        new_address = address[:-3] + "00" + address[-3:]
    elif uid < 100:
        new_address = address[:-4] + "0" + address[-4:]

    # if times[address]["type"] == "VM":
    #     address = VM + address.split("-")[2]


    df = df.append({
        "address": new_address[:-2], 
        "id": uid, 
        "layer": int(address.split("-")[1]), 
        "which_tree": int(address.split("-")[3]), 
        "type": times[address]["type"], 
        "start_pre": times[address]["start_pre"], 
        "end_pre": times[address]["end_pre"], 
        "start_mig": times[address]["start_mig"], 
        "end_mig": times[address]["end_mig"], 
        "start_buf": times[address]["start_buf"], 
        "end_buf": times[address]["end_buf"],

    }, ignore_index=True)

df["len_pre"] = df["end_pre"] - df["start_pre"]  
df["len_mig"] = df["end_mig"] - df["start_mig"]  
df["len_buf"] = df["end_buf"] - df["start_buf"]  

df["start_pre"] = df["start_pre"] - min_time  
df["start_mig"] = df["start_mig"] - min_time  
df["start_buf"] = df["start_buf"] - min_time  

max_time -= min_time
min_time = 0

for i in range(3): 

    if i == 0: 
        half_df = df[df["which_tree"] == 0]
    if i == 1: 
        half_df = df[df["which_tree"] == 1]
    if i == 2: 
        half_df = df

    vm_df = half_df[half_df["type"] == "VM"]
    gw_df = half_df[half_df["type"] == "GW"]

    gw_df = gw_df.sort_values("address")

    print(half_df)

    if i == 0: 
        plot_height = 10 
    if i == 1: 
        plot_height = 10
    if i == 2: 
        plot_height = 16
    
    legend_elements = [
        Patch(facecolor='lime', edgecolor='g', label='VM Precopy'),
        Patch(facecolor='green', edgecolor='g', label='VM Migration'),
        Patch(facecolor='red', edgecolor='r', label='GW Migration'),
        Patch(facecolor='orange', edgecolor='r', label='GW Tunneling'),
    ]

    fig, ax = plt.subplots(1, figsize=(plot_height, plot_height))
    
    ax.set_xlim(min_time, max_time)

    ax.barh(vm_df.address, vm_df.len_pre, left=vm_df.start_pre, color="lime")
    ax.barh(vm_df.address, vm_df.len_mig, left=vm_df.start_mig, color="green")
    ax.barh(gw_df.address, gw_df.len_mig, left=gw_df.start_mig, color="red")
    ax.barh(gw_df.address, gw_df.len_pre, left=gw_df.start_pre, color="orange")
    ax.barh(half_df.address, half_df.len_buf, left=half_df.start_buf, color="gray")
    
    ax.legend(handles=legend_elements, loc='lower right')

    plt.xlabel("Simulation Time (ms)")
    plt.ylabel("Node (type - layer - id)")
    
    if i == 0: 
        name = "protocol_src"
    if i == 1: 
        name = "protocol_dst"
    if i == 2: 
        name = "protocol_all"

    plt.savefig(name, dpi=300)