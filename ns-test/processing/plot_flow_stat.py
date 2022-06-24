from ast import Expression
from itertools import groupby
from tkinter import CENTER
import pandas as pd 
import matplotlib.pyplot as plt
import matplotlib
import numpy as np
from matplotlib.patches import Patch
from argparse import ArgumentParser
import sys
import seaborn as sns
import os 


off_time_threshold = 30 


parser = ArgumentParser()

parser.add_argument("-d", "--directory", 
                    dest="directory", required=True,
                    help="the directory to perform the processing", 
                    metavar="Directory")

parser.add_argument("-v", "--verbose",
                    action="store_true", 
                    dest="verbose", default=False,
                    help="print the lines to ouput")

parser.add_argument("-r", "--reload",
                    action="store_true", dest="reload",
                    default=False,
                    help="reload the dataset from the file")


parser.add_argument("-n", "--no-plot",
                    action="store_true", dest="no_plot", default=False,
                    help="don't plot the data")


args = parser.parse_args()

def setup_directories():
    data_dir = args.directory + "/" + "data"
    os.system('mkdir -p ' + data_dir)

    plots_dir = args.directory + "/" + "plots"
    os.system('mkdir -p ' + plots_dir)
    
    nodes_plots_dir = plots_dir + "/" + "flow_stats"
    os.system('mkdir -p ' + nodes_plots_dir)


setup_directories() 


# load the data


data = []

if args.reload:
    log_file = args.directory + "/" + "logFile.tr"

    with open(log_file) as f:
        try: 
            for line in f.readlines():
                if not line.startswith("flow_stats_recorder"):
                    # ignore the lines not starting with stat_recorder
                    continue

                line = line.strip()
                s = line.split(" ")

                fid_brak = s[1]
                fid = int(fid_brak[1:-1])

                time_brak = s[2]
                time = float(time_brak[1:-1]) * 1000

                packets_received = int(s[3])
                average_in_flight_time = int(s[4])
                average_buffered_time = int(s[5])

                data.append({
                    "fid": fid, 
                    "time": time, 
                    "packets_received": packets_received, 
                    "average_in_flight_time": average_in_flight_time, 
                    "average_buffered_time": average_buffered_time, 
                })

                if len(data) % 10000 == 0:
                    print ("loaded", len(data), "data points")

        except Exception as e:
            print(e)

    df = pd.DataFrame(data)
    data_path = args.directory + "/data/flow_stats.csv" 
    df.to_csv(data_path)
else: 
    print("Loading the data from the file")
    data_path = args.directory + "/data/flow_stats.csv" 
    df = pd.read_csv(data_path)
    print("Loading data from the file finished")


if args.no_plot:
    exit()



df['packets_received_orig'] = df['packets_received']
# df['packets_received'] = df['packets_received'].rolling(50).mean()
# df['average_in_flight_time'] = df[['average_in_flight_time', 'packets_received']].rolling(10).apply(lambda x: print(x))
# df['average_buffered_time'] = df[['average_buffered_time', 'packets_received']].rolling(10).apply(lambda x: np.sum(x.average_buffered_time * x.packets_received))

try: 
    protocol_df = pd.read_csv(args.directory + "/data/protocol.csv") 

    protocol_end = max(
        protocol_df.end_mig.max(), 
        protocol_df.end_pre.max(), 
        protocol_df.end_buf.max()) 

    protocol_start = min(
        protocol_df.start_mig[protocol_df.start_mig > 0].min(),
        protocol_df.start_pre[protocol_df.start_pre > 0].min(),
        protocol_df.start_buf[protocol_df.start_buf > 0].min()
    )
    protocol_end += 100
    protocol_start -= 100

except Exception as e: 
    print("loading the default")
    protocol_start = 4500
    protocol_end = 6000


df = df[df["time"] >= protocol_start]
df = df[df["time"] <= protocol_end]


def plot_flow(fid, ax, flow_df): 
    flow_df['consec_grp'] = (flow_df.packets_received_orig.diff(1) != 0).astype('int').cumsum()
    consec = pd.DataFrame({
        'begin_time' : flow_df.groupby('consec_grp').time.first(), 
        'end_time' : flow_df.groupby('consec_grp').time.last(),
        'consecutive' : flow_df.groupby('consec_grp').size()
    }).reset_index(drop=True)

    off_times = consec[consec.consecutive > off_time_threshold]
    # print(off_times)

    print(flow_df.size)
    sns.lineplot(ax=ax, data=flow_df, x="time", y=measure)

    for i, row in off_times.iterrows():
        ax.axvspan(row.begin_time, row.end_time, alpha=0.5, color='gray')

    ax.set_title("flow: " + str(fid))

def plot_measure(measure):
    flow_per_dst = 6 
    fids = df.fid.unique()
    list_of_groups = [fids[i:i+flow_per_dst] for i in range(0, len(fids), flow_per_dst)]

    plot_count = len(list_of_groups) 
    fig, axes = plt.subplots(plot_count, 1, sharex=True, sharey=True)
    fig.set_size_inches(12, plot_count * 3)

    plot_index = 0 

    for group in list_of_groups:
        print("drawing subplot", plot_index + 1)

        flow_dfs = [df[df.fid == fid].copy() for fid in group]
        group_df = pd.concat(flow_dfs)
        group_df.average_in_flight_time = group_df.average_in_flight_time * group_df.packets_received
        group_df.average_buffered_time = group_df.average_buffered_time * group_df.packets_received
        sum_df = group_df.groupby(['time']).sum().reset_index()
        group_df.average_in_flight_time = group_df.average_in_flight_time / group_df.packets_received
        group_df.average_buffered_time = group_df.average_buffered_time / group_df.packets_received
        
        roll = 21
        sum_df.packets_received = sum_df.packets_received.rolling(roll, center=True).mean()

        if plot_count > 1:
            ax = axes[plot_index]
        else:
            ax = axes

        plot_flow(plot_index, ax, sum_df)   
        plot_index += 1 
    
    plots_dir = args.directory + "/" + "plots"
    flows_plots_dir = plots_dir + "/" + "flow_stats/"
    plot_path = "{}/{}.png".format(flows_plots_dir, measure)

    print("saving", plot_path)
    plt.tight_layout()
    plt.savefig(plot_path, dpi=300, bbox_inches='tight')
    plt.clf()


    fig = plt.figure()
    ax = fig.add_subplot(111)
    sum_df = df.groupby(['time']).sum().reset_index()
    plot_flow("total", ax, sum_df)

    plot_path = "{}/{}_total.png".format(flows_plots_dir, measure)
    print("saving", plot_path)
    plt.tight_layout()
    plt.savefig(plot_path, dpi=300, bbox_inches='tight')



for measure in ["packets_received", "average_in_flight_time", "average_buffered_time"]:
    plot_measure(measure)


