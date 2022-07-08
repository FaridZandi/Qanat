import pandas as pd 
import matplotlib.pyplot as plt
import matplotlib
import numpy as np
from matplotlib.patches import Patch
from argparse import ArgumentParser
import sys
import seaborn as sns
import os 
import traceback 

parser = ArgumentParser()

parser.add_argument("-d", "--directory", 
                    dest="directory", required=True,
                    help="the directory to perform the processing", 
                    metavar="Directory")

parser.add_argument("-r", "--reload",
                    action="store_true", dest="reload",
                    default=True,
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


# group is a pandas dataframe
def weighted_average(group, weight_col, value_col):
    weights = group[weight_col].sum()
    values = group[weight_col] * group[value_col]
    if weights == 0:
        return 0 
    else: 
        return values.sum() / weights


def weighted_percentile(data, percents, weights=None):
    if weights is None:
        return np.percentile(data, percents)

    ind = np.argsort(data)
    d = data[ind]
    w = weights[ind]
    p = 1. * w.cumsum() / w.sum() * 100
    y = np.interp(percents, p, d)
    return y.tolist()
    

if args.reload: 
    directory = args.directory

    dfs = []
    for file_name in os.listdir(directory):
        try: 
            setting_df_path = "{}/{}/summary.csv".format(args.directory, file_name)
            setting_df = pd.read_csv(setting_df_path, header=[1])  
            dfs.append(setting_df)

        except Exception as e: 
            print("could not process exp setting: " + file_name)
            print(e)
            pass

    df = pd.concat(dfs)
    
    print(df)
    
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
        ("settings", "vm_traffic_gen_delay"),
        ("settings", "link_rate"),
        ("flow_metrics", "vm_afct"),
        ("flow_metrics", "bg_afct"),
        ("flow_metrics", "bg_50_afct"),
        ("flow_metrics", "bg_75_afct"),
        ("flow_metrics", "bg_90_afct"),
        ("flow_metrics", "bg_100k_afct"),
        ("flow_metrics", "bg_big_afct"),
        ("flow_metrics", "vm_ret"),
        ("flow_metrics", "bg_ret"),
        ("flow_metrics", "bg_50_ret"),
        ("flow_metrics", "bg_75_ret"),
        ("flow_metrics", "bg_90_ret"),
        ("flow_metrics", "bg_100k_ret"),
        ("flow_metrics", "bg_big_ret"),
        ("flow_metrics", "vm_avg_r"),
        ("flow_metrics", "bg_avg_r"),
        ("protocol_metrics", "tot_mig_time"),
        ("buffer_metrics", "max_hpq"),
        ("buffer_metrics", "max_lpq"),
        ("buffer_metrics", "max_bpq"),
        ("buffer_metrics", "avg_hpq"),
        ("buffer_metrics", "avg_lpq"),
        ("buffer_metrics", "avg_bpq"),
        ("ptk_lever_metrics", "vm_avg_pkt_flight_t"),
        ("ptk_lever_metrics", "vm_avg_pkt_buff_t"),
        ("ptk_lever_metrics", "vm_max_pkt_in_flight_t"),
        ("ptk_lever_metrics", "vm_max_pkt_buff_t"),
        ("ptk_lever_metrics", "vm_90_ptk_in_flight_t"),
        ("ptk_lever_metrics", "vm_95_ptk_in_flight_t"),
        ("ptk_lever_metrics", "vm_99_ptk_in_flight_t"),
        ("ptk_lever_metrics", "vm_90_pkt_buff_t"),
        ("ptk_lever_metrics", "vm_95_pkt_buff_t"),
        ("ptk_lever_metrics", "vm_99_pkt_buff_t"),
        ("tunnell_metrics", "tnld_pkt_cnt")
    ]

    print (df) 

    column_order = [x[1] for x in columns]
    df = df[column_order]

    # sort the datafram
    df.columns = pd.MultiIndex.from_tuples(columns)

    # sort by setting columns
    df = df.sort_values(by=columns[0:15])


    df = df[:].round(decimals = 6)
    
    data_path = args.directory + "/summary.csv" 
    df.to_csv(data_path)
else: 
    data_path = args.directory + "/summary.csv" 
    df = pd.read_csv(data_path)