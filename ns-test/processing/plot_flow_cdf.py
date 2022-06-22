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


protocol_df = pd.read_csv(args.directory + "/data/protocol.csv") 

protocol_end = max(
    protocol_df.end_mig.max(), 
    protocol_df.end_pre.max(), 
    protocol_df.end_buf.max()
) + 100

protocol_start = min(
    protocol_df.start_mig[protocol_df.start_mig > 0].min(),
    protocol_df.start_pre[protocol_df.start_pre > 0].min(),
    protocol_df.start_buf[protocol_df.start_buf > 0].min()
) - 100

df = df[df["time"] >= protocol_start]
df = df[df["time"] <= protocol_end]


df['packets_received_orig'] = df['packets_received']
# df['packets_received'] = df['packets_received'].rolling(10).mean()
# df['average_in_flight_time'] = df['average_in_flight_time'].rolling(10).max()
# df['average_buffered_time'] = df['average_buffered_time'].rolling(10).max()


# group is a pandas dataframe
def weighted_average(group, weight_col, value_col):
    weights = group[weight_col].sum()
    values = group[weight_col] * group[value_col]
    return values.sum() / weights

def plot_flow(fid, ax, flow_df): 
    sns.ecdfplot(ax=ax, x=flow_df[measure], weights=flow_df.packets_received)
    ax.set_title("flow: " + str(fid))

def plot_measure(measure):
    
    plots_dir = args.directory + "/" + "plots"
    flows_plots_dir = plots_dir + "/" + "flow_stats/"
    plot_path = "{}/{}_cdf_total.png".format(flows_plots_dir, measure)

    measure_avg = weighted_average(df, "packets_received", measure)

    fig = plt.figure()
    ax = fig.add_subplot(111)
    ax.set_xscale('log')
    # sum_df = df.groupby(['time']).sum().reset_index()
    plot_flow("total", ax, df)

    plt.axvline(measure_avg, color='red')

    print("saving", plot_path)
    plt.tight_layout()
    plt.xlim([1,50000])
    plt.ylim([0,1.01])
    plt.savefig(plot_path, dpi=300, bbox_inches='tight')



for measure in ["average_in_flight_time", "average_buffered_time"]:
    plot_measure(measure)


