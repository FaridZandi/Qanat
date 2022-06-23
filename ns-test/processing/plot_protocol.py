from audioop import add
from platform import node
import pandas as pd 
import matplotlib.pyplot as plt
import matplotlib
import numpy as np
from matplotlib.patches import Patch
from argparse import ArgumentParser
import sys
import os 

times = {}
min_time = 10000
max_time = 0 

file_path = "protocol.out"

parser = ArgumentParser()

parser.add_argument("-m", "--mode", dest="mode",
                    help="operation mode", default="file", metavar="MODE")

parser.add_argument("-d", "--directory", 
                    dest="directory", required=False,
                    help="the directory to perform the processing", 
                    metavar="Directory")

parser.add_argument("-r", "--reload",
                    action="store_true", dest="reload",
                    default=False,
                    help="reload the dataset from the file")

parser.add_argument("-v", "--verbose",
                    action="store_true", dest="verbose", default=False,
                    help="print the lines to ouput")


parser.add_argument("-n", "--no-plot",
                    action="store_true", dest="no_plot", default=False,
                    help="don't plot the data")


args = parser.parse_args()


def setup_directories():
    data_dir = args.directory + "/" + "data"
    os.system('mkdir -p ' + data_dir)

    plots_dir = args.directory + "/" + "plots"
    os.system('mkdir -p ' + plots_dir)
    
    protocol_plots_dir = plots_dir + "/" + "protocol"
    os.system('mkdir -p ' + protocol_plots_dir)

setup_directories()


def process_line(line): 
    global times, min_time, max_time, args

    try: 
        if args.verbose:
            print(line[:-1])

        line = line.strip()

        if not line.startswith("protocol_event"):
            return 

        s = line.split(" ")

        time_str = s[1]
        time = float(time_str[1:-1]) * 1000

        address_str = s[2]
        address = address_str[1:-1]
        
        if address == 0 or address == "0":
            return 

        if address == "":
            return 

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

    except Exception as e: 
        print(e)
        print("*", end="") 


# load the data

if args.reload: 
    if args.mode == "file":
        log_file = args.directory + "/" + "logFile.tr"
        print("reading from", log_file)
        with open(log_file) as f: 
            for line in f.readlines(): 
                process_line(line)

    elif args.mode == "input":
        print("reading the log from input")
        for line in sys.stdin:
            process_line(line)

    print("done with getting the input")

    df = pd.DataFrame(columns=[
        "address", "id", "layer", "which_tree", 
        "type", "start_pre", "end_pre", "start_mig", 
        "end_mig", "start_buf", "end_buf"
    ])

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

    data_path = args.directory + "/data/protocol.csv" 
    df.to_csv(data_path)
else: 
    data_path = args.directory + "/data/protocol.csv" 
    df = pd.read_csv(data_path)

if args.no_plot:
    exit()


graph_data = {}
log_file = args.directory + "/" + "logFile.tr"
with open(log_file) as f: 
    for line in f.readlines(): 
        line = line.strip()
        if not line.startswith("graph_data"):
            continue 

        s = line.split(" ")

        uid = int(s[1])

        children = [] 
        for i in range (5, len(s)):
            children.append(int(s[i]))

        node_data = {
            "layer": int(s[2]),
            "child_index": int(s[3]),
            "children": children,
            "x": 0, 
            "y": 0, 
        }

        graph_data[uid] = node_data
        
# process the data 

max_time = max(
    df.end_mig.max(), 
    df.end_pre.max(), 
    df.end_buf.max()) 

min_time = min(
    df.start_mig[df.start_mig > 0].min(),
    df.start_pre[df.start_pre > 0].min(),
    df.start_buf[df.start_buf > 0].min()
)

# df["start_pre"] = df["start_pre"] - min_time  
# df["start_mig"] = df["start_mig"] - min_time  
# df["start_buf"] = df["start_buf"] - min_time  

# max_time -= min_time
# min_time = 0

# plot the protocol 


for i in range(3): 

    if i == 0: 
        half_df = df[df["which_tree"] == 0]
    if i == 1: 
        half_df = df[df["which_tree"] == 1]
    if i == 2: 
        half_df = df

    vm_df = half_df[half_df["type"] == "VM"]
    gw_df = half_df[half_df["type"] == "GW"]

    def change_to_gw(address): 
        return "VNF" + address[2:]

    gw_df.address = gw_df.address.apply(change_to_gw)

    def get_level(address):
        return int(address.split("-")[1])

    def get_level_keys(col):
        return col.apply(get_level)

    gw_df = gw_df.sort_values("start_mig").sort_values(by="address", key=get_level_keys)
    print(half_df)

    for index, row in half_df.iterrows():
        address = row["address"]
        uid = int(address.split("-")[2])
        node_data = graph_data[uid]
        # print(node_data)

    if i == 0: 
        plot_height = 10 
    if i == 1: 
        plot_height = 10
    if i == 2: 
        plot_height = 16
    
    legend_elements = [
        # Patch(facecolor='lime', edgecolor='g', label='VM Precopy'),
        # Patch(facecolor='green', edgecolor='g', label='VM Migration'),
        Patch(facecolor='red', edgecolor='r', label='VNF Migration'),
        Patch(facecolor='orange', edgecolor='r', label='VNF Tunneling'),
    ]

    font = {'family' : 'DejaVu Sans',
            'size'   : 20}

    matplotlib.rc('font', **font)

    fig, ax = plt.subplots(1, figsize=(plot_height, plot_height))
    
    ax.set_xlim(min_time, max_time)

    # ax.barh(vm_df.address, vm_df.len_pre, left=vm_df.start_pre, color="lime")
    # ax.barh(vm_df.address, vm_df.len_mig, left=vm_df.start_mig, color="green")
    ax.barh(gw_df.address, gw_df.len_mig, left=gw_df.start_mig, color="red")
    ax.barh(gw_df.address, gw_df.len_pre, left=gw_df.start_pre, color="orange")
    # ax.barh(half_df.address, half_df.len_buf, left=half_df.start_buf, color="gray")


    # ax.legend(handles=legend_elements, loc='lower right', bbox_to_anchor=(1.45, 0.75))
    # ax.legend(handles=legend_elements ,loc='center right', bbox_to_anchor=(1, 1))

    plt.xlabel("Simulation Time (ms)")
    plt.ylabel("Node (layer - id)")
    
    plots_dir = args.directory + "/" + "plots"
    protocol_plots_dir = plots_dir + "/" + "protocol"    
    
    if i == 0: 
        name = protocol_plots_dir + "/protocol_src"
    if i == 1: 
        name = protocol_plots_dir + "/protocol_dst"
    if i == 2: 
        name = protocol_plots_dir + "/protocol_all"

    plt.savefig(name+".png", dpi=300, bbox_inches='tight')
    print("min time:", min_time )
    print("max time:", max_time )
    exit()