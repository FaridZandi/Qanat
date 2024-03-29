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

parser.add_argument("-d1", "--directory1", 
                    dest="directory1", required=True,
                    help="the first directory to perform the processing", 
                    metavar="Directory1")

parser.add_argument("-d2", "--directory2", 
                    dest="directory2", required=True,
                    help="the second directory to perform the processing", 
                    metavar="Directory2")

parser.add_argument("-d3", "--directory3", 
                    dest="directory3", required=True,
                    help="the third directory to perform the processing", 
                    metavar="Directory3")


parser.add_argument("-v", "--verbose",
                    action="store_true", 
                    dest="verbose", default=False,
                    help="print the lines to ouput")

args = parser.parse_args()

def setup_directories():
    data_dir = args.directory1 + "/" + "data"
    os.system('mkdir -p ' + data_dir)

    plots_dir = args.directory1 + "/" + "plots"
    os.system('mkdir -p ' + plots_dir)
    
    nodes_plots_dir = plots_dir + "/" + "flow_stats"
    os.system('mkdir -p ' + nodes_plots_dir)

setup_directories() 

# load the data
data_path_1 = args.directory1 + "/data/flow_stats.csv" 
df1 = pd.read_csv(data_path_1)
data_path_2 = args.directory2 + "/data/flow_stats.csv" 
df2 = pd.read_csv(data_path_2)
data_path_3 = args.directory3 + "/data/flow_stats.csv" 
df3 = pd.read_csv(data_path_3)

# protocol_df_1 = pd.read_csv(args.directory1 + "/data/protocol.csv") 
protocol_df_2 = pd.read_csv(args.directory2 + "/data/protocol.csv") 
protocol_df_3 = pd.read_csv(args.directory3 + "/data/protocol.csv") 


protocol_end_1 = 6000
protocol_start_1 = 4490


protocol_end_2 = max(
    protocol_df_2.end_mig.max(), 
    protocol_df_2.end_pre.max(), 
    protocol_df_2.end_buf.max()
) + 10

protocol_start_2 = min(
    protocol_df_2.start_mig[protocol_df_2.start_mig > 0].min(),
    protocol_df_2.start_pre[protocol_df_2.start_pre > 0].min(),
    protocol_df_2.start_buf[protocol_df_2.start_buf > 0].min()
) - 10


protocol_end_3 = max(
    protocol_df_3.end_mig.max(), 
    protocol_df_3.end_pre.max(), 
    protocol_df_3.end_buf.max()
) + 10

protocol_start_3 = min(
    protocol_df_3.start_mig[protocol_df_3.start_mig > 0].min(),
    protocol_df_3.start_pre[protocol_df_3.start_pre > 0].min(),
    protocol_df_3.start_buf[protocol_df_3.start_buf > 0].min()
) - 10



df1 = df1[df1["time"] >= protocol_start_1]
df1 = df1[df1["time"] <= protocol_end_1]

df2 = df2[df2["time"] >= protocol_start_2]
df2 = df2[df2["time"] <= protocol_end_2]

df3 = df3[df3["time"] >= protocol_start_3]
df3 = df3[df3["time"] <= protocol_end_3]


# group is a pandas dataframe
def weighted_average(group, weight_col, value_col):
    weights = group[weight_col].sum()
    values = group[weight_col] * group[value_col]
    return values.sum() / weights


def plot_measure(measure):
    
    plots_dir = args.directory1 + "/" + "plots"
    flows_plots_dir = plots_dir + "/" + "flow_stats/"

    measure_avg_1 = weighted_average(df1, "packets_received", measure) / 1000
    measure_avg_2 = weighted_average(df2, "packets_received", measure) / 1000
    measure_avg_3 = weighted_average(df3, "packets_received", measure) / 1000

    new_df = pd.concat([df1, df2, df3], keys=['No Migration', "Bottom Up", "Top Down"]).reset_index(); 
    new_df[measure] /= 1000

    p = sns.ecdfplot(
        data=new_df,
        hue="level_0", 
        x=measure, 
        weights=new_df.packets_received, 
        palette=['blue', 'orange', "green"]
    )

    print("average:", measure_avg_1)
    print("average:", measure_avg_2)
    print("average:", measure_avg_3)

    print("max:", measure_avg_1)
    print("max:", measure_avg_2)
    print("max:", measure_avg_3)


    plt.axvline(measure_avg_1, color='blue', linestyle="--")
    plt.axvline(measure_avg_2, color='orange', linestyle="--")
    plt.axvline(measure_avg_3, color='green', linestyle="--")

    plt.gcf().set_size_inches(5, 2)
    plt.tight_layout()
    # plt.xscale("log")
    # if measure == "average_in_flight_time":
        # plt.xlim([0,400])
        # plt.xlim([0,163])
    # else: 
        # plt.xlim([0, 400])
        # plt.xlim([0,150])

    plt.ylim([0.75, 1])
 
    desc_map = {
        "average_in_flight_time": "One-Way Delay (ms)", 
        "average_buffered_time": "VNF Buffering Time (ms)"
    }

    p.set_xlabel(desc_map[measure])
    p.set_ylabel("CDF")

    plt.legend(title='Layering', loc='lower right', labels=["Top Down", "Bottom Up", 'No Migration'])
    
    plot_path = "{}/{}_cdf_total.png".format(flows_plots_dir, measure)
    plt.savefig(plot_path, dpi=300, bbox_inches='tight')

    plot_path = "{}/{}_cdf_total.pdf".format(flows_plots_dir, measure)
    plt.savefig(plot_path, dpi=300, bbox_inches='tight')
    
    plt.clf()


for measure in ["average_in_flight_time", "average_buffered_time"]:
    plot_measure(measure)


