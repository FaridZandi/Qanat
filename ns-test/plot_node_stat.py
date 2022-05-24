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


parser.add_argument("-p", "--plotsdir", 
                    dest="plotsdir", default="plots",
                    help="directory to store the plots in", 
                    metavar="PLOTSDIR")

args = parser.parse_args()



df = pd.DataFrame(columns=[
    "uid", "time", 
    "high_prio_buf", 
    "low_prio_buf", 
    "packet_count", 
])


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
                time = round(float(time_brak[1:-1]),6)

                high_prio_buf = s[3]
                low_prio_buf = s[4]
                packet_count = s[5]

                df = df.append({
                    "uid":uid, 
                    "time":time, 
                    "high_prio_buf":high_prio_buf, 
                    "low_prio_buf":low_prio_buf, 
                    "packet_count":packet_count, 
                },ignore_index=True)

                # print(df.size)

        except Exception as e:
            print(e)

    df.to_csv(args.datastore + ".csv")
else: 
    df = pd.read_csv(args.datastore + ".csv")

print(df)

nodes = df.uid.unique()
node_count = len(nodes)

print("node_count", node_count)


for measure in ["packet_count", "high_prio_buf", "low_prio_buf"]:
    plot_index = 0 

    fig, axes = plt.subplots(node_count, 1, sharey=True, sharex=True)

    for node in nodes:
        node_df = df[df.uid == node]
        sns.lineplot(ax=axes[plot_index], x=node_df.time, y=node_df[measure])
        axes[plot_index].set_title("node: " + str(node))
        plot_index += 1 

    plot_path = "{}/{}.png".format(args.plotsdir, measure)
    plt.savefig(plot_path, dpi=300, bbox_inches='tight')




