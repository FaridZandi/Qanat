flows = {}

with open('flowstart.log') as f:
    for line in f.readlines():
        line = line.strip()

        fid = int(line.split(" ")[1])
        time_brak = line.split(" ")[0]
        time = float(time_brak[1:-1])

        if fid != 0:
            # print(fid, time)
            flows[fid] = [time, 0]



with open('flowend.log') as f:
    for line in f.readlines():
        line = line.strip()

        fid = int(line.split(" ")[1])
        time_brak = line.split(" ")[0]
        time = float(time_brak[1:-1])

        if fid != 0:
            # print(fid, time)
            flows[fid][1] = time   

fcts = [] 
for flow in flows:
    # print(flow, flows[flow][0], flows[flow][1], flows[flow][1] - flows[flow][0])
    fcts.append(flows[flow][1] - flows[flow][0])

 
average_fct = sum(fcts)/len(fcts)

with open('fct.log', "w") as f:
    print("average_fct:", average_fct)

    f.write("average_fct: ")
    f.write(str(average_fct))
    f.write("\n\n\n")

    for flow,times in sorted(flows.items()):
    # for flow in flows:

        f.write(str(flow))
        f.write("\t\t\t")
        f.write("%.2f" % round(times[0], 2))
        f.write("\t")
        f.write("%.2f" % round(times[1], 2))
        f.write("\t")
        f.write("%.2f" % round(times[1] - times[0], 2))
        f.write("\n")


        
