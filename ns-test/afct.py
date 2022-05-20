import pandas as pd 
import matplotlib.pyplot as plt
import matplotlib
import numpy as np
from matplotlib.patches import Patch
from argparse import ArgumentParser
import sys


flows = {}

total_completion_time = 0 

parser = ArgumentParser()

parser.add_argument("-f", "--file", dest="filename", default="protocol.out",
                    help="read protocol log from", metavar="FILE")

parser.add_argument("-v", "--verbose",
                    action="store_true", dest="verbose", default=False,
                    help="print the lines to ouput")

args = parser.parse_args()


with open(args.filename) as f:
    try: 
        for line in f.readlines():

            if not line.startswith("["):
                # ignore the lines not starting with [time]
                continue
        
            line = line.strip()

            time_brak = line.split(" ")[0]
            time = float(time_brak[1:-1])

            event = line.split(" ")[1]

            if event == "flow_start":
                fid = int(line.split(" ")[2])
                size = int(float(line.split(" ")[4]))

                flows[fid] = {
                    "start": time, 
                    "size": size,
                    "end": 0, 
                    "retransmissions": 0, 
                    "fct": 0,
                }
            elif event == "flow_end":
                fid = int(line.split(" ")[2])
                flows[fid]["end"] = time    
                retransmissions = int(line.split(" ")[4])
                flows[fid]["retransmissions"] = retransmissions    
                flows[fid]["fct"] = flows[fid]["end"] - flows[fid]["start"]    


    except Exception as e:
        print(e)


sum_fct = 0 
for flow in flows: 
    print (flows[flow])
    sum_fct += flows[flow]["fct"]

print("AFCT:", sum_fct / len(flows))

