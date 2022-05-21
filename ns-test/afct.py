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
            time = round(float(time_brak[1:-1]),6)

            event = line.split(" ")[1]

            if event == "flow_start":
                fid = int(line.split(" ")[2])
                size = int(float(line.split(" ")[4]))

                flows[fid] = {
                    "start": time, 
                    "size": size,
                    "src": 0,
                    "dst": 0,
                    "end": 0, 
                    "ret": 0, 
                    "fct": 0,
                }

            elif event == "flow_end":
                fid = int(line.split(" ")[2])
                flows[fid]["end"] = time    
                flows[fid]["ret"] = int(line.split(" ")[4])    
                flows[fid]["src"] = int(line.split(" ")[6])
                flows[fid]["dst"] = int(line.split(" ")[8])
                flows[fid]["fct"] = round(flows[fid]["end"] - flows[fid]["start"], 6) 


    except Exception as e:
        print(e)


sum_fct = 0 
for flow in flows: 
    print (flows[flow])
    sum_fct += flows[flow]["fct"]

print("AFCT:", sum_fct / len(flows))

