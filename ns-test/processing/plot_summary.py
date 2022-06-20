
import pandas as pd 
import matplotlib.pyplot as plt
import matplotlib
import numpy as np
from matplotlib.patches import Patch
from argparse import ArgumentParser
import sys
import seaborn as sns
import os 


plots_dir = "plots"

desc_map = {
    "bg_cdf": "background CDF", 
    "migration_status": "Migration Status",
    "parallel_mig": "Parallel Migrations",
    "load": "Load (%)",
    "oversub": "OverSubscription",
    "vm_precopy_size": "VM Precopy Size (MB)",
    "vm_snapshot_size": "VM Snapshot Size (MB)",
    "gw_snapshot_size": "GW Snapshot Size (MB)",
    "cc_protocol": "CC Protocol",
    "orch_type": "Orchesterator Type",
    "prioritization": "Prioritization",
    "src_zone_delay": "Source Zone Delay (us)",
    "dst_zone_delay": "Destination Zone Delay (us)",
    "vm_traffic_gen_delay": "Control Traffic Gen Delay (us)",
    "link_rate": "Link Rate (Gbps)",
    "vm_afct": "Control Traffic Average FCT (s)",
    "bg_afct": "Background Average FCT (s)",
    "vm_ret": "Control Traffic Retransmissions",
    "bg_ret": "Background Retransmissions",
    "vm_avg_r": "Control Traffic Average Rate (B/s)",
    "bg_avg_r": "Background Average Rate (B/s)",
    "tot_mig_time": "Total Migration Time (ms)",
    "max_hpq": "Max High-P Queue (Packets)",
    "max_lpq": "Max Low-P Queue (Packets)",
    "avg_hpq": "Average High-P Queue (Packets)",
    "avg_lpq": "Average Low-P Queue (Packets)",
    "vm_avg_pkt_flight_t": "VM Average Packets Flight Time (us)",
    "vm_avg_pkt_buff_t": "VM Average Packet Buffer Time (us)",
    "vm_max_pkt_in_flight_t": "VM Max Packet Flight Time (us)",
    "vm_max_pkt_buff_t": "VM Max Packet Buffer Time (us)",
    "tnld_pkt_cnt": "Tunnelled Packets Count"
}


metrics = [
    "vm_afct",
    "bg_afct",
    "vm_ret",
    "bg_ret",
    "vm_avg_r",
    "bg_avg_r",
    "tot_mig_time",
    "max_hpq",
    "max_lpq",
    "avg_hpq",
    "avg_lpq",
    "vm_avg_pkt_flight_t",
    "vm_avg_pkt_buff_t",
    "vm_max_pkt_in_flight_t",
    "vm_max_pkt_buff_t",
    "tnld_pkt_cnt"
]




def clean_data(df):
    df.bg_cdf = df.bg_cdf.replace("vl2", "DataMining")
    df.bg_cdf = df.bg_cdf.replace("dctcp", "WebSearch")

    df.orch_type = df.orch_type.replace("BottUp", "Bottom Up")
    df.orch_type = df.orch_type.replace("TopDwn", "Top Down")

    df.cc_protocol = df.cc_protocol.replace("dctcp", "DCTCP")
    df.cc_protocol = df.cc_protocol.replace("tcp", "TCP")



def merge_columns(u_df, columns, merged_name):
    if columns == None:
        return None 

    u_df[merged_name] = ""
    x_title = ""

    i = 0
    for x in columns:
        i += 1 
        delim = "" if i == len(columns) else " - "
        u_df[merged_name] += u_df[x].map(str) + delim
        x_title += desc_map[x] + delim

    return x_title

def plot_exp_bar(conf):

    os.system("mkdir -p {}".format(plots_dir))
    os.system("mkdir -p {}/{}".format(plots_dir, conf["plot_name"]))

    summary_path = "exps/{}/summary.csv".format(conf["exp_name"])
    summary_df = pd.read_csv(summary_path, header=[1]).reset_index()

    clean_data(summary_df)

    u = summary_df[conf["repeat_per"]].drop_duplicates()

    for index, row in u.iterrows():
        u_df = summary_df.copy() 
        u_name = ""

        for setting in conf["repeat_per"]:
            u_name += setting + ":" + str(row[setting]) + "+"
            u_df = u_df[u_df[setting] == row[setting]]

        u_name = u_name[:-1]

        print(u_df)
        
        u_dir = "{}/{}/{}".format(plots_dir, conf["plot_name"], u_name)
        os.system("mkdir -p {}".format(u_dir))
        
        for metric in conf["metrics"]:
            print("plotting", conf["plot_name"], ":", u_name, ":", metric)

            # setting up the columns
            x_title = merge_columns(u_df, conf["x_setting"], "plot_x")
            hue_title = merge_columns(u_df, conf["hue_setting"], "plot_hue")

            # draw the plot 
            ax = sns.barplot(
                data=u_df, 
                x="plot_x",
                hue="plot_hue" if hue_title else None,
                y=metric, 
            )

            # set plot settings 
            ax.set_xlabel(x_title)
            ax.set_ylabel(desc_map[metric])
            
            if conf["hue_setting"]:
                plt.legend(title = hue_title)


            # save the plot 
            file_path = "{}/{}.png".format(u_dir, metric)
            plt.savefig(file_path, bbox_inches='tight')
            plt.clf() 


plot_exp_bar({
    "exp_name": "orch_test", 
    "plot_name": "orch_test",
    "x_setting": ["gw_snapshot_size", "parallel_mig"],
    "hue_setting": ["orch_type"],
    "repeat_per": ["cc_protocol"],
    "metrics": metrics,
}) 

plot_exp_bar({
    "exp_name": "parallel_test", 
    "plot_name": "parallel_test",
    "x_setting": ["parallel_mig"],
    "hue_setting": ["gw_snapshot_size"],
    "repeat_per": ["cc_protocol"],
    "metrics": metrics,
}) 

plot_exp_bar({
    "exp_name": "prio_test", 
    "plot_name": "prio_test",
    "x_setting": ["load"],
    "hue_setting": ["prioritization"],
    "repeat_per": ["gw_snapshot_size", "cc_protocol"],
    "metrics": metrics,
}) 

plot_exp_bar({
    "exp_name": "size_test", 
    "plot_name": "size_test",
    "x_setting": ["vm_snapshot_size", "gw_snapshot_size"],
    "hue_setting": None,
    "repeat_per": ["cc_protocol"],
    "metrics": metrics,
}) 

plot_exp_bar({
    "exp_name": "latency_test", 
    "plot_name": "latency_test",
    "x_setting": ["src_zone_delay"],
    "hue_setting": ["dst_zone_delay"],
    "repeat_per": ["cc_protocol"],
    "metrics": metrics,
}) 

plot_exp_bar({
    "exp_name": "bg_test", 
    "plot_name": "bg_test",
    "x_setting": ["prioritization"],
    "hue_setting": ["bg_cdf", "cc_protocol"],
    "repeat_per": ["gw_snapshot_size", "load"],
    "metrics": metrics,
}) 

plot_exp_bar({
    "exp_name": "bg_test", 
    "plot_name": "bg_test_2",
    "x_setting": ["load"],
    "hue_setting": ["bg_cdf", "cc_protocol"],
    "repeat_per": ["gw_snapshot_size", "prioritization"],
    "metrics": metrics,
}) 
