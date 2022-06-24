
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
    "bg_cdf": "Background Traffic CDF", 
    "migration_status": "Migration Status",
    "parallel_mig": "Parallel Migrations",
    "load": "Background Load (%)",
    "oversub": "Over Subscription",
    "vm_precopy_size": "VM Precopy Size (MB)",
    "vm_snapshot_size": "VM Snapshot Size (MB)",
    "gw_snapshot_size": "GW Snapshot Size (MB)",
    "cc_protocol": "CC Protocol",
    "orch_type": "Orchesterator Type",
    "prioritization": "Prioritization",
    "src_zone_delay": "Source Zone Delay (us)",
    "dst_zone_delay": "Destination Zone Delay (us)",
    "vm_traffic_gen_delay": "VNF Traffic Traffic Generator Delay (us)",
    "link_rate": "Link Rate (Gbps)",
    "vm_afct": "VNF Traffic Average FCT (s)",
    "bg_afct": "Background Average FCT (s)",
    "vm_ret": "VNF Traffic Retransmissions",
    "bg_ret": "Background Retransmissions",
    "vm_avg_r": "VNF Traffic Average Rate (Mbps)",
    "bg_avg_r": "Background Average Rate (Gpbs)",
    "tot_mig_time": "Total Migration Time (ms)",
    "max_hpq": "Max High-Priority Queue (Packets)",
    "max_lpq": "Max Low-Priority Queue (Packets)",
    "avg_hpq": "Average High-Priority Queue (Packets)",
    "avg_lpq": "Average Low-Priority Queue (Packets)",
    "vm_avg_pkt_flight_t": "Avg. VNF Traffic 1-Way Delay (ms)",
    "vm_avg_pkt_buff_t": "VNF Traffic Average Packet Buffer Time (ms)",
    "vm_max_pkt_in_flight_t": "Max VNF Traffic 1-Way Delay (ms)",
    "vm_max_pkt_buff_t": "VNF Traffic Max Packet Buffer Time (ms)",
    "tnld_pkt_cnt": "Tunnelled Packets Count",
    "vm_90_ptk_in_flight_t": "90th(%) VNF Traffic 1-Way Delay (ms)",
    "vm_95_ptk_in_flight_t": "95th(%) VNF Traffic 1-Way Delay (ms)",
    "vm_99_ptk_in_flight_t": "99th(%) VNF Traffic 1-Way Delay (ms)",
    "vm_90_pkt_buff_t": "90th(%) VNF Traffic Buffering (ms)",
    "vm_95_pkt_buff_t": "95th(%) VNF Traffic Buffering (ms)",
    "vm_99_pkt_buff_t": "99th(%) VNF Traffic Buffering (ms)",
    "bg_50_ret": "50% Shortest Background Ret.",
    "bg_75_ret": "75% Shortest Background Ret.",
    "bg_90_ret": "90% Shortest Background Ret.",
    "bg_50_afct": "50% Shortest Background AFCT (s)",
    "bg_75_afct": "75% Shortest Background AFCT (s)",
    "bg_90_afct": "90% Shortest Background AFCT (s)",
    "avg_bpq": "Average Queue Size (Packets)",
    "max_bpq": "Max Queue Size (Packets)",
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
    "tnld_pkt_cnt",
    "vm_90_ptk_in_flight_t",
    "vm_95_ptk_in_flight_t",
    "vm_99_ptk_in_flight_t",
    "vm_90_pkt_buff_t",
    "vm_95_pkt_buff_t",
    "vm_99_pkt_buff_t",
    "bg_50_ret",
    "bg_75_ret",
    "bg_90_ret",
    "bg_50_afct",
    "bg_75_afct",
    "bg_90_afct",
    "avg_bpq",
    "max_bpq"
]

migration_related_metrics = [
    "tot_mig_time",
    "max_hpq",
    "max_lpq",
    "avg_hpq",
    "avg_lpq",
    "vm_avg_pkt_flight_t",
    "vm_avg_pkt_buff_t",
    "vm_max_pkt_in_flight_t",
    "vm_max_pkt_buff_t",
    "tnld_pkt_cnt",
    "vm_90_ptk_in_flight_t",
    "vm_95_ptk_in_flight_t",
    "vm_99_ptk_in_flight_t",
    "vm_90_pkt_buff_t",
    "vm_95_pkt_buff_t",
    "vm_99_pkt_buff_t",
    "avg_bpq",
    "max_bpq"
]


def clean_data(df):
    df.replace('', 0, inplace=True) 
    
    df.bg_cdf = df.bg_cdf.replace("vl2", "DataMining")
    df.bg_cdf = df.bg_cdf.replace("dctcp", "WebSearch")

    df.orch_type = df.orch_type.replace("BottUp", "Bottom Up")
    df.orch_type = df.orch_type.replace("TopDwn", "Top Down")

    df.cc_protocol = df.cc_protocol.replace("dctcp", "DCTCP")
    df.cc_protocol = df.cc_protocol.replace("tcp", "TCP")

    df.migration_status = df.migration_status.replace("nomig", "No Migration")
    df.migration_status = df.migration_status.replace("mig", "Migration")

    df.prioritization = df.prioritization.replace(0, "1-Level")
    df.prioritization = df.prioritization.replace(1, "2-Levels")
    df.prioritization = df.prioritization.replace(2, "3-Levels")

    df.load = df.load.astype(int)

    df.vm_avg_r = (df.vm_avg_r / 1000000) * 8
    df.bg_avg_r = (df.bg_avg_r / 1000000000) * 8 

    df.vm_avg_pkt_flight_t = df.vm_avg_pkt_flight_t / 1000
    df.vm_avg_pkt_buff_t = df.vm_avg_pkt_buff_t / 1000
    df.vm_max_pkt_in_flight_t = df.vm_max_pkt_in_flight_t / 1000
    df.vm_max_pkt_buff_t = df.vm_max_pkt_buff_t / 1000


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
            metric_df = u_df.copy()
            x_setting = [elem for elem in conf["x_setting"]]
            hue_setting = [elem for elem in conf["hue_setting"]]
            print("plotting", conf["plot_name"], ":", u_name, ":", metric)

            # remove migration status from x and hue if metric is 
            # related to migration. (metric is not meaningful when
            # when there is no migration)
            if metric in migration_related_metrics: 
                if "migration_status" in x_setting:
                    x_setting.remove("migration_status")
                if hue_setting != None: 
                    if "migration_status" in hue_setting: 
                        hue_setting.remove("migration_status")

                metric_df = metric_df[metric_df["migration_status"] == "Migration"]

            # setting up the columns
            x_title = merge_columns(metric_df, x_setting, "plot_x")
            hue_title = merge_columns(metric_df, hue_setting, "plot_hue")

            # sort the hues
            hue_order = []
            if hue_title != None: 
                hue_order = sorted(metric_df.plot_hue.unique())

            # sort the x 
            metric_df = metric_df.sort_values("plot_x")

            # draw the plot 
            ax = sns.barplot(
                data=metric_df, 
                x="plot_x",
                hue="plot_hue" if hue_title else None,
                hue_order=hue_order if hue_title else None, 
                y=metric, 
            )

            # pattern fill the patches for B&W readablity
            hatches = ['-', '\\\\', '//', 'x','o', '/', '.', '\\']
            for bars, hatch in zip(ax.containers, hatches):
                for bar in bars:
                    bar.set_hatch(hatch)

                    
            # set plot settings 
            ax.set_xlabel(x_title)
            ax.set_ylabel(desc_map[metric])

            if conf["hue_setting"]:
                plt.legend(title = hue_title)


            # save the plot 
            file_path = "{}/{}.png".format(u_dir, metric)
            plt.savefig(file_path, bbox_inches='tight', dpi=300)
            plt.clf() 



if len(sys.argv) < 2:
    print("Usage: python3 plot_summary.py <plot_name>")
    exit(0)

plot_name = sys.argv[1]

if plot_name == "orch_test":
    plot_exp_bar({
        "exp_name": "orch_test", 
        "plot_name": "orch_test",
        "x_setting": ["prioritization"],
        "hue_setting": ["migration_status", "orch_type",],
        "repeat_per": ["gw_snapshot_size", "cc_protocol", "parallel_mig"],
        "metrics": metrics,
    }) 

if plot_name == "parallel_test":
    plot_exp_bar({
        "exp_name": "parallel_test", 
        "plot_name": "parallel_test",
        "x_setting": ["parallel_mig"],
        "hue_setting": ["gw_snapshot_size"],
        "repeat_per": ["cc_protocol"],
        "metrics": metrics,
    }) 

if plot_name == "prio_test":
    plot_exp_bar({
        "exp_name": "prio_test", 
        "plot_name": "prio_test",
        "x_setting": ["load"],
        "hue_setting": ["prioritization"],
        "repeat_per": ["gw_snapshot_size", "cc_protocol"],
        "metrics": metrics,
    })  

if plot_name == "size_test":
    plot_exp_bar({
        "exp_name": "size_test", 
        "plot_name": "size_test",
        "x_setting": ["vm_snapshot_size", "gw_snapshot_size"],
        "hue_setting": None,
        "repeat_per": ["cc_protocol"],
        "metrics": metrics,
    }) 

if plot_name == "latency_test":
    plot_exp_bar({
        "exp_name": "latency_test", 
        "plot_name": "latency_test",
        "x_setting": ["src_zone_delay"],
        "hue_setting": ["migration_status", "dst_zone_delay"],
        "repeat_per": ["gw_snapshot_size"],
        "metrics": metrics,
    }) 


if plot_name == "bg_test":
    plot_exp_bar({
        "exp_name": "bg_test", 
        "plot_name": "bg_test",
        "x_setting": ["prioritization"],
        "hue_setting": ["cc_protocol", "bg_cdf", "migration_status"],
        "repeat_per": ["gw_snapshot_size", "load"],
        "metrics": metrics,
    }) 


if plot_name == "bg_test_2":
    plot_exp_bar({
        "exp_name": "bg_test", 
        "plot_name": "bg_test_2",
        "x_setting": ["load"],
        "hue_setting": ["bg_cdf", "migration_status"],
        "repeat_per": ["cc_protocol","gw_snapshot_size", "prioritization"],
        "metrics": metrics,
    }) 