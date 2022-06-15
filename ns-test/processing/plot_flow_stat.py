from ast import Expression
import pandas as pd 
import matplotlib.pyplot as plt
import matplotlib
import numpy as np
from matplotlib.patches import Patch
from argparse import ArgumentParser
import sys
import seaborn as sns
import os 


off_time_threshold = 10 


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




df['packets_received_orig'] = df['packets_received']
# df['packets_received'] = df['packets_received'].rolling(10).mean()
df['average_in_flight_time'] = df['average_in_flight_time'].rolling(10).max()
df['average_buffered_time'] = df['average_buffered_time'].rolling(10).max()


def plot_flow(fid, ax, flow_df): 
    flow_df['consec_grp'] = (flow_df.packets_received_orig.diff(1) != 0).astype('int').cumsum()
    consec = pd.DataFrame({
        'begin_time' : flow_df.groupby('consec_grp').time.first(), 
        'end_time' : flow_df.groupby('consec_grp').time.last(),
        'consecutive' : flow_df.groupby('consec_grp').size()
    }).reset_index(drop=True)

    off_times = consec[consec.consecutive > off_time_threshold]
    # print(off_times)

    sns.lineplot(ax=ax, x=flow_df.time, y=flow_df[measure])

    for i, row in off_times.iterrows():
        ax.axvspan(row.begin_time, row.end_time, alpha=0.5, color='gray')

    ax.set_title("flow: " + str(fid))

def plot_measure(measure):
 
    fids = df.fid.unique()

    plot_count = len(fids) 
    fig, axes = plt.subplots(plot_count, 1, sharex=True, sharey=True)
    fig.set_size_inches(12, plot_count * 3)

    plot_index = 0 

    for fid in fids:
        print("drawing subplot", plot_index + 1)
        flow_df = df[df.fid == fid].copy()
        plot_flow(fid, axes[plot_index], flow_df)   
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


