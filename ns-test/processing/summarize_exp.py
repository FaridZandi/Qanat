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


if args.reload: 
    directory = args.directory

    df = pd.DataFrame()

    for file_name in os.listdir(directory):
        if file_name == "summary.csv":
            continue

        exp_info = {}
        
        # extract the exp setting from the file name
        file_name_split = file_name.split("_")
        exp_info["migration_status"] = file_name_split[0]
        exp_info["parallel_mig"] = int(file_name_split[1][1:])
        exp_info["load"] = int(file_name_split[2][1:])
        exp_info["oversub"] = int(file_name_split[3][1:])
        migration_sizes = file_name_split[4][2:]
        exp_info["vm_precopy_size"] = int(migration_sizes.split("-")[0])
        exp_info["vm_snapshot_size"] = int(migration_sizes.split("-")[1])
        exp_info["gw_snapshot_size"] = int(migration_sizes.split("-")[2])
        exp_info["cc_protocol"] = file_name_split[5][1:]
        exp_info["orch_type"] = file_name_split[6][2:]
        exp_info["prioritization"] = bool(file_name_split[7][2:])
        exp_info["src_zone_delay"] = int(file_name_split[8][2:])
        exp_info["dst_zone_delay"] = int(file_name_split[9][2:])


        # load the data files 
        data_directory = args.directory + "/" + file_name + "/data/"
        df_flows = pd.read_csv(data_directory + "flows.csv")
        vm_flows = df_flows[df_flows["type"] == "vm_traffic"]
        bg_flows = df_flows[df_flows["type"] == "bg_traffic"]
        df_protocol = pd.read_csv(data_directory + "protocol.csv")
        df_node_stats = pd.read_csv(data_directory + "nodes.csv")


        ######################################################
        ##################### metrics ########################
        ######################################################

        # average fct 
        exp_info["vm_afct"] = vm_flows["fct"].mean()
        exp_info["bg_afct"] = bg_flows["fct"].mean()

        # retransmission rates 
        exp_info["vm_ret"] = vm_flows["ret"].mean()
        exp_info["bg_ret"] = bg_flows["ret"].mean()

        # total migration time 
        if exp_info["migration_status"] == "mig":
            protocol_start = max(
                df_protocol.end_mig.max(), 
                df_protocol.end_pre.max(), 
                df_protocol.end_buf.max()
            ) 

            protocol_end = min(
                df_protocol.start_mig[df_protocol.start_mig > 0].min(),
                df_protocol.start_pre[df_protocol.start_pre > 0].min(),
                df_protocol.start_buf[df_protocol.start_buf > 0].min()
            )
            exp_info["total_mig_time"] = protocol_end - protocol_start
        else: 
            exp_info["total_mig_time"] = 0
        
        # OoO delivery 

        # Buffer sizes

        # Per-Packet delay 

        # per-packet buffering time 



        


        # add the exp info to the dataframe
        df = df.append(exp_info, ignore_index=True)

    data_path = args.directory + "/summary.csv" 
    df.to_csv(data_path)
else: 
    data_path = args.directory + "/summary.csv" 
    df = pd.read_csv(data_path)