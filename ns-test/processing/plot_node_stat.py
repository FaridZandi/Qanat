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


args = parser.parse_args()


def setup_directories():
    data_dir = args.directory + "/" + "data"
    os.system('mkdir -p ' + data_dir)

    plots_dir = args.directory + "/" + "plots"
    os.system('mkdir -p ' + plots_dir)
    
    nodes_plots_dir = plots_dir + "/" + "nodes"
    os.system('mkdir -p ' + nodes_plots_dir)


setup_directories() 


# load the data


data = []

if args.reload:
    log_file = args.directory + "/" + "logFile.tr"

    with open(log_file) as f:
        try: 
            for line in f.readlines():
                if not line.startswith("node_stats_recorder"):
                    # ignore the lines not starting with stat_recorder
                    continue

                line = line.strip()
                s = line.split(" ")

                uid_brak = s[1]
                uid = int(uid_brak[1:-1])

                time_brak = s[2]
                time = float(time_brak[1:-1]) * 1000

                high_prio_buf = int(s[3])
                low_prio_buf = int(s[4])
                packet_count = int(s[5])
                ooo_packets = int(s[6])

                data.append({
                    "uid": uid, 
                    "time": time, 
                    "high_prio_buf": high_prio_buf, 
                    "low_prio_buf": low_prio_buf, 
                    "packet_count": packet_count, 
                    "ooo_packets": ooo_packets,
                })

                if len(data) % 10000 == 0:
                    print ("loaded", len(data), "data points")

        except Exception as e:
            print(e)

    df = pd.DataFrame(data)    
    data_path = args.directory + "/data/nodes.csv" 
    df.to_csv(data_path)
else: 
    print("Loading the data from the file")
    data_path = args.directory + "/data/nodes.csv" 
    df = pd.read_csv(data_path)
    print("Loading data from the file finished")

# sample the protocol df enteries to plot them 
# df['low_prio_buf'] = df['low_prio_buf'].rolling(100).max()
# df['high_prio_buf'] = df['high_prio_buf'].rolling(100).max()
# df = df.iloc[::100, :]
print(df.size)

# load the protocol data for drawing the colored bars
protocol_df = pd.read_csv(args.directory + "/data/protocol.csv") 
print(protocol_df)


def plot_bar(ax, bar_len, bar_start, color): 
    if bar_len == 0: 
        return 

    y_lim = ax.get_ylim()

    ax.barh(
        y = -y_lim[1] / 10, 
        width=bar_len,
        left=bar_start,
        color=color,
        height=y_lim[1] / 10,
    )

def plot_node(node, ax): 

    node_df = df[df.uid == node].copy()
    node_df["packet_count_diff"] = node_df.packet_count.diff(1)
    node_df = node_df.dropna()

    sns.lineplot(ax=ax, x=node_df.time, y=node_df[measure])
    ax.set_title("node: " + str(node))

    try:
        proto = protocol_df[protocol_df.id == node].iloc[0]
        plot_bar(ax, proto.len_pre, proto.start_pre, "orange")
        plot_bar(ax, proto.len_mig, proto.start_mig, "red")
        plot_bar(ax, proto.len_buf, proto.start_buf, "gray")
    except Exception as e:
        pass

def plot_measure(measure, nodes, prefix):

    fig, axes = plt.subplots(len(nodes), 1, sharey=True)
    fig.set_size_inches(12, len(nodes) * 3)

    plot_index = 0 
    for node in nodes:
        print("drawing subplot", plot_index + 1)
        plot_node(node, axes[plot_index])       
        plot_index += 1 

    plots_dir = args.directory + "/" + "plots"
    nodes_plots_dir = plots_dir + "/" + "nodes"
    plot_path = "{}/{}_{}.png".format(nodes_plots_dir, prefix, measure)


    print("saving", plot_path)
    plt.tight_layout()
    plt.savefig(plot_path, dpi=300, bbox_inches='tight')



for measure in ["packet_count_diff", "packet_count", "ooo_packets", "low_prio_buf", "high_prio_buf"]:
    nodes = df.uid.unique()

    src_nodes = nodes[0:(len(nodes) // 2)]
    dst_nodes = nodes[(len(nodes) // 2):]

    plot_measure(measure, src_nodes, "src")
    plot_measure(measure, dst_nodes, "dst")


