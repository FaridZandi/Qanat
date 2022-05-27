import pandas as pd 
import matplotlib.pyplot as plt
import matplotlib
import numpy as np
from matplotlib.patches import Patch
from argparse import ArgumentParser
import sys
import seaborn as sns


parser = ArgumentParser()

parser.add_argument("-f", "--file", dest="filename",
                    default="protocol.out", 
                    help="read protocol log from", 
                    metavar="FILE")

parser.add_argument("-v", "--verbose",
                    action="store_true", 
                    dest="verbose", default=False,
                    help="print the lines to ouput")

parser.add_argument("-r", "--reload",
                    action="store_true", dest="reload",
                    default=False,
                    help="reload the dataset from the file")


parser.add_argument("-d", "--datastore", 
                    dest="datastore", default="data/node_stat",
                    help="store the data in the file for faster access", 
                    metavar="DATASTORE")

parser.add_argument("-c", "--protocol", 
                    dest="protocol", default="data/protocol",
                    help="the data for the protocol", 
                    metavar="PROTOCOL")


parser.add_argument("-p", "--plotsdir", 
                    dest="plotsdir", default="plots",
                    help="directory to store the plots in", 
                    metavar="PLOTSDIR")

args = parser.parse_args()


data = []

if args.reload:
    with open(args.filename) as f:
        try: 
            for line in f.readlines():
                if not line.startswith("stat_recorder"):
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

                data.append({
                    "uid":uid, 
                    "time":time, 
                    "high_prio_buf":high_prio_buf, 
                    "low_prio_buf":low_prio_buf, 
                    "packet_count":packet_count, 
                })

                if len(data) % 1000 == 0:
                    print ("loaded", len(data), "data points")

        except Exception as e:
            print(e)

    df = pd.DataFrame(data)
    df.to_csv(args.datastore + ".csv")
else: 
    df = pd.read_csv(args.datastore + ".csv")

df['low_prio_buf'] = df['low_prio_buf'].rolling(100).max()
df['high_prio_buf'] = df['high_prio_buf'].rolling(100).max()

df = df.iloc[::100, :]
print(df.size)

protocol_df = pd.read_csv(args.protocol + ".csv")
protocol_df["len_pre"] = protocol_df["end_pre"] - protocol_df["start_pre"]  
protocol_df["len_mig"] = protocol_df["end_mig"] - protocol_df["start_mig"]  
protocol_df["len_buf"] = protocol_df["end_buf"] - protocol_df["start_buf"]  
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

    node_df = df[df.uid == node]
    sns.lineplot(ax=ax, x=node_df.time, y=node_df[measure])
    ax.set_title("node: " + str(node))

    proto = protocol_df[protocol_df.id == node].iloc[0]
    plot_bar(ax, proto.len_pre, proto.start_pre, "orange")
    plot_bar(ax, proto.len_mig, proto.start_mig, "red")
    plot_bar(ax, proto.len_buf, proto.start_buf, "gray")


def plot_measure(measure, nodes, prefix):

    fig, axes = plt.subplots(len(nodes), 1, sharey=True)
    fig.set_size_inches(6, len(nodes) * 3)

    plot_index = 0 
    for node in nodes:
        print("drawing subplot", plot_index + 1)
        plot_node(node, axes[plot_index])       
        plot_index += 1 


    plot_path = "{}/{}_{}.png".format(args.plotsdir, prefix, measure)
    print("saving", plot_path)
    plt.tight_layout()
    plt.savefig(plot_path, dpi=300, bbox_inches='tight')





for measure in ["low_prio_buf"]: #["packet_count", "low_prio_buf", "high_prio_buf"]:
    nodes = df.uid.unique()

    src_nodes = nodes[0:(len(nodes) // 2)]
    dst_nodes = nodes[(len(nodes) // 2):]

    plot_measure(measure, src_nodes, "src")
    plot_measure(measure, dst_nodes, "dst")


