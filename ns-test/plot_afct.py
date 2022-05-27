import pandas as pd 
import matplotlib.pyplot as plt
import matplotlib
import numpy as np
from matplotlib.patches import Patch
from argparse import ArgumentParser
import sys
import seaborn as sns

flows = {}

total_completion_time = 0 

parser = ArgumentParser()

parser.add_argument("-f", "--file", dest="filename", default="protocol.out",
                    help="read protocol log from", metavar="FILE")

parser.add_argument("-r", "--reload",
                    action="store_true", dest="reload",
                    default=False,
                    help="reload the dataset from the file")

parser.add_argument("-d", "--datastore", 
                    dest="datastore", default="data/flow_stat",
                    help="store the data in the file for faster access", 
                    metavar="DATASTORE")

parser.add_argument("-v", "--verbose",
                    action="store_true", dest="verbose", default=False,
                    help="print the lines to ouput")

args = parser.parse_args()

if args.reload:
    with open(args.filename) as f:
        try: 
            for line in f.readlines():

                if not line.startswith("flow_stats"):
                    # ignore the lines not starting with [time]
                    continue
            
                line = line.strip()

                time_brak = line.split(" ")[1]
                time = round(float(time_brak[1:-1]),6)

                event = line.split(" ")[2]

                if event == "flow_start":
                    fid = int(line.split(" ")[3])
                    size = int(float(line.split(" ")[5]))

                    flows[fid] = {
                        "start": time, 
                        "size": size,
                        "src": 0,
                        "dst": 0,
                        "end": 0, 
                        "ret": 0, 
                        "fct": 0,
                        "avg_rate": 0 
                    }

                elif event == "flow_end":
                    s = line.split(" ") 
                    fid = int(s[3])
                    ret = int(s[5]) 
                    src = int(s[7]) 
                    dst = int(s[9])
                    start = flows[fid]["start"]
                    size = flows[fid]["size"]
                    fct = round(time - start, 6)

                    flows[fid] = {
                        "start": start, 
                        "size": size,
                        "src": src,
                        "dst": dst,
                        "end": time, 
                        "ret": ret, 
                        "fct": fct,
                        "avg_rate": size / fct  
                    }

                # if len(flows) % 1000 == 0: 
                    # print("seen", len(flows), "flowes")
                    # print(flows[len(flows) - 1])

        except Exception as e:
            print(e)
    df = pd.DataFrame(flows).transpose()
    df.to_csv(args.datastore + ".csv")
else: 
    df = pd.read_csv(args.datastore + ".csv")

print("finished loading the dataset")
print(df)

print("AFCT:", df.fct.mean())


# plot a histogram of the flow sizes
def plot_fct_hist(df): 
    sns.histplot(df, x="fct")
    plt.savefig("plots/flow_fct_hist.png", dpi=300)
    plt.clf()

# plot a histogram of the dst nodes
def plot_dst_hist(df): 
    sns.histplot(df, x="dst")
    plt.savefig("plots/flow_dst_hist.png", dpi=300)
    plt.clf()

# plot the fcts over time
def plot_fct_over_time(df): 
    sns.lineplot(x=df.start, y=df.fct)
    plt.savefig("plots/flow_start_fct.png", dpi=300)
    plt.clf()

# throughput over time 
def plot_throughput_over_time_1(df):
    interval = 0.00001 
    max_time = df.end.max()
    min_time = df.start.min()
    timeslots = int((max_time) / interval) + 1
    total_throughput = [0] * (timeslots)
    print (timeslots)
    for flow in flows: 
        if flow % 100 == 0: 
            print("processing", flow) 

        start = flows[flow]["start"]
        end = flows[flow]["end"]
        avg_rate = flows[flow]["avg_rate"]

        if end < 0.1: 
            continue

        starting_point = start - (start % interval) + interval

        current_point = starting_point
        while current_point < end: 
            try: 
                this_timeslot = int(current_point / interval)
                if this_timeslot > timeslots: 
                    # print(this_timeslot)
                    break
                total_throughput[this_timeslot] += avg_rate 
                current_point += interval
            except Exception as e: 
                print (e)

    sns.lineplot(y = total_throughput, x=range(timeslots))
    plt.savefig("plots/flow_total_throughput_1.png", dpi=300)
    plt.clf()



# throughput over time (attempt 2) 
def plot_throughput_over_time_2(df):
    destinations = df.dst.unique() 
    for dst in destinations:
        print("plotting the throughput for dst", dst)
        interval = 0.01 
        max_time = df.end.max()
        min_time = df.start.min()
        timeslots = int((max_time) / interval) + 1
        total_throughput2 = [0] * (timeslots)
        active_flows = [0] * (timeslots)
        print (timeslots)

        dst_df = df[df["dst"] == dst]
        print("dst_df shape", dst_df.shape)
        for i in range(timeslots):
            current_time = i * interval
            afdf = dst_df
            afdf = afdf[afdf["start"] <= current_time]
            afdf = afdf[afdf["end"] >= current_time]

            active_flows[i] = afdf.shape[0]
            total_throughput2[i] = afdf.avg_rate.sum()
            print("active flows at time", current_time, "are", afdf.shape[0])
            

        sns.lineplot(y = total_throughput2, x=range(timeslots))
        plt.savefig("plots/flow_total_throughput_{}_2.png".format(dst), dpi=300)
        plt.clf()

        sns.lineplot(y = active_flows, x=range(timeslots))
        plt.savefig("plots/flows_active_{}.png".format(dst), dpi=300)
        plt.clf()



# plot_fct_hist(df)
# plot_dst_hist(df) 
# plot_fct_over_time(df)
# plot_throughput_over_time_1(df)
plot_throughput_over_time_2(df)

# print (df[df["dst"] == 41])