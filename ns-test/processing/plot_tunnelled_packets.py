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
    
    tunnels_plots_dir = plots_dir + "/" + "tunnelled_packets"
    os.system('mkdir -p ' + tunnels_plots_dir)


setup_directories() 


# load the data
data = []

if args.reload:
    log_file = args.directory + "/" + "logFile.tr"

    with open(log_file) as f:
        try: 
            for line in f.readlines():
                if not line.startswith("tunnelled_packets_recorder"):
                    # ignore the lines not starting with stat_recorder
                    continue

                line = line.strip()
                s = line.split(" ")

                time_brak = s[1]
                time = float(time_brak[1:-1]) * 1000

                tunnelled_packets = int(s[2])

                data.append({
                    "time": time, 
                    "tunnelled_packets": tunnelled_packets, 
                })

                if len(data) % 1000 == 0:
                    print ("loaded", len(data), "data points")

        except Exception as e:
            print(e)

    df = pd.DataFrame(data)

    df["tunnelled_packets_diff"] = df.tunnelled_packets.diff()

    data_path = args.directory + "/data/tunnelled_stats.csv" 
    df.to_csv(data_path)
else: 
    print("Loading the data from the file")
    data_path = args.directory + "/data/tunnelled_stats.csv" 
    df = pd.read_csv(data_path)
    print("Loading data from the file finished")

if args.no_plot:
    exit()

protocol_df = pd.read_csv(args.directory + "/data/protocol.csv") 
print(protocol_df)

max_time = max(
    protocol_df.end_mig.max(), 
    protocol_df.end_pre.max(), 
    protocol_df.end_buf.max())

min_time = min(
    protocol_df.start_mig[protocol_df.start_mig > 0].min(),
    protocol_df.start_pre[protocol_df.start_pre > 0].min(),
    protocol_df.start_buf[protocol_df.start_buf > 0].min()
)




def plot_flow(ax): 
    sns.lineplot(ax=ax, x=df.time, y=df[measure])
    ax.set_title("tunnelled_packets")

def plot_measure(measure):
    fig, ax = plt.subplots(1, 1)
    fig.set_size_inches(6, 3)

    ax.set_xlim(min_time, max_time)

    print("drawing plot")    
    plot_flow(ax)   

    plots_dir = args.directory + "/" + "plots"
    tunnels_plots_dir = plots_dir + "/" + "tunnelled_packets"
    plot_path = "{}/{}.png".format(tunnels_plots_dir, measure)

    print("saving", plot_path)
    plt.tight_layout()
    plt.savefig(plot_path, dpi=300, bbox_inches='tight')


for measure in ["tunnelled_packets", "tunnelled_packets_diff"]:
    plot_measure(measure)


