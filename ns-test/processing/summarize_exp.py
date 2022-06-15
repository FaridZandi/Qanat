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

parser.add_argument("-r", "--reload",
                    action="store_true", dest="reload",
                    default=False,
                    help="reload the dataset from the file")

parser.add_argument("-v", "--verbose",
                    action="store_true", dest="verbose", default=False,
                    help="print the lines to ouput")

args = parser.parse_args()


def translate_orch_type(orch_type):
    if orch_type == 1:
        return "BottUp"
    elif orch_type == 2:
        return "TopDwn"
    elif orch_type == 3:
        return "Random"


if args.reload: 
    directory = args.directory

    df = pd.DataFrame()

    for file_name in os.listdir(directory):
        if file_name == "summary.csv":
            continue

        print ("Summarising exp:" + file_name)

        exp_info = {}
        
        # extract the exp setting from the file name
        file_name_split = file_name.split("_")
        exp_info["bg_cdf"] = file_name_split[0]
        exp_info["migration_status"] = file_name_split[1]
        exp_info["parallel_mig"] = int(file_name_split[2][1:])
        exp_info["load"] = int(file_name_split[3][1:])
        exp_info["oversub"] = int(file_name_split[4][1:])
        migration_sizes = file_name_split[5][2:]
        exp_info["vm_precopy_size"] = int(migration_sizes.split("-")[0])
        exp_info["vm_snapshot_size"] = int(migration_sizes.split("-")[1])
        exp_info["gw_snapshot_size"] = int(migration_sizes.split("-")[2])
        exp_info["cc_protocol"] = file_name_split[6][1:]
        exp_info["orch_type"] = int(file_name_split[7][2:])
        exp_info["prioritization"] = int(file_name_split[8][2:])
        exp_info["src_zone_delay"] = int(file_name_split[9][2:])
        exp_info["dst_zone_delay"] = int(file_name_split[10][2:])
        exp_info["orch_type"] = translate_orch_type(exp_info["orch_type"])

        # list of tuples in shape of (metrics, metric_name)


        # load the data files 
        data_directory = args.directory + "/" + file_name + "/data/"
        
        df_flows = pd.read_csv(data_directory + "flows.csv")
        vm_flows = df_flows[df_flows["type"] == "vm_traffic"]
        bg_flows = df_flows[df_flows["type"] == "bg_traffic"]
        
        df_protocol = pd.read_csv(data_directory + "protocol.csv")
        df_node_stats = pd.read_csv(data_directory + "nodes.csv")
        df_flow_stats = pd.read_csv(data_directory + "flow_stats.csv")
        df_tunnelled_packets = pd.read_csv(data_directory + "tunnelled_stats.csv")


        ######################################################
        ##################### metrics ########################
        ######################################################

        # average fct 
        exp_info["vm_afct"] = vm_flows["fct"].mean()
        exp_info["bg_afct"] = bg_flows["fct"].mean()

        # retransmission rates 
        exp_info["vm_ret"] = vm_flows["ret"].mean()
        exp_info["bg_ret"] = bg_flows["ret"].mean()
        
        ################ Total Migration time ###############

        protocol_end = 0
        protocol_start = 0

        try: 
            if exp_info["migration_status"] == "mig":
                protocol_end = max(
                    df_protocol.end_mig.max(), 
                    df_protocol.end_pre.max(), 
                    df_protocol.end_buf.max()
                ) 

                protocol_start = min(
                    df_protocol.start_mig[df_protocol.start_mig > 0].min(),
                    df_protocol.start_pre[df_protocol.start_pre > 0].min(),
                    df_protocol.start_buf[df_protocol.start_buf > 0].min()
                )
                exp_info["tot_mig_time"] = protocol_end - protocol_start
            else: 
                exp_info["tot_mig_time"] = 0
        except Exception as e: 
            pass 
    

        ################ OOO delivery ####################

        ################ Buffer Sizes ####################

        # node df corresponding to the protocol running time 
        mig_node_stats_df = df_node_stats[df_node_stats["time"] >= protocol_start]
        mig_node_stats_df = mig_node_stats_df[mig_node_stats_df["time"] <= protocol_end]

        exp_info["max_hpq"] = mig_node_stats_df.high_prio_buf.max()
        exp_info["max_lpq"] = mig_node_stats_df.low_prio_buf.max()

        exp_info["avg_hpq"] = mig_node_stats_df.high_prio_buf.mean()
        exp_info["avg_lpq"] = mig_node_stats_df.low_prio_buf.mean()

        ################ VM flows stats ####################

        # vm flows stats corresponding to the protocol running time
        mig_flow_stats_df = df_flow_stats[df_flow_stats["time"] >= protocol_start]
        mig_flow_stats_df = mig_flow_stats_df[mig_flow_stats_df["time"] <= protocol_end]

        # avg
        exp_info["vm_avg_pkt_flight_t"] = mig_flow_stats_df.average_in_flight_time.mean()
        exp_info["vm_avg_pkt_buff_t"] = mig_flow_stats_df.average_buffered_time.mean()

        # max 
        exp_info["vm_max_pkt_in_flight_t"] = mig_flow_stats_df.average_in_flight_time.max()
        exp_info["vm_max_pkt_buff_t"] = mig_flow_stats_df.average_buffered_time.max()

        ################ Tunnelled Packets ####################

        exp_info["tnld_pkt_cnt"] = df_tunnelled_packets.tunnelled_packets.max()

        ######################################################
        ######################################################
        ######################################################

        df = df.append(exp_info, ignore_index=True)

    columns = [
        ("settings", "bg_cdf"), 
        ("settings", "migration_status"),
        ("settings", "parallel_mig"),
        ("settings", "load"),
        ("settings", "oversub"),
        ("settings", "vm_precopy_size"),
        ("settings", "vm_snapshot_size"),
        ("settings", "gw_snapshot_size"),
        ("settings", "cc_protocol"),
        ("settings", "orch_type"),
        ("settings", "prioritization"),
        ("settings", "src_zone_delay"),
        ("settings", "dst_zone_delay"),
        ("flow_metrics", "vm_afct"),
        ("flow_metrics", "bg_afct"),
        ("flow_metrics", "vm_ret"),
        ("flow_metrics", "bg_ret"),
        ("protocol_metrics", "tot_mig_time"),
        ("buffer_metrics", "max_hpq"),
        ("buffer_metrics", "max_lpq"),
        ("buffer_metrics", "avg_hpq"),
        ("buffer_metrics", "avg_lpq"),
        ("ptk_lever_metrics", "vm_avg_pkt_flight_t"),
        ("ptk_lever_metrics", "vm_avg_pkt_buff_t"),
        ("ptk_lever_metrics", "vm_max_pkt_in_flight_t"),
        ("ptk_lever_metrics", "vm_max_pkt_buff_t"),
        ("tunnell_metrics", "tnld_pkt_cnt")
    ]

    column_order = [x[1] for x in columns]
    df = df[column_order]

    # sort the datafram
    df.columns = pd.MultiIndex.from_tuples(columns)

    # sort by setting columns
    df = df.sort_values(by=columns[0:13])


    df = df[:].round(decimals = 2)
    
    data_path = args.directory + "/summary.csv" 
    df.to_csv(data_path)
else: 
    data_path = args.directory + "/summary.csv" 
    df = pd.read_csv(data_path)