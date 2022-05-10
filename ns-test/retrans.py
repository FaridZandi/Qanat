flows = {}

with open('retrans.log') as f:
    for line in f.readlines():
        line = line.strip()

        fid = int(line.split(" ")[1])
        rtx = int(line.split(" ")[2])
        seq = int(line.split(" ")[3])

        if fid != 0:
            flows[fid] = [rtx, seq]


retranss = [] 
for flow in flows:
    # print(flow, flows[flow][0], flows[flow][1], flows[flow][1] - flows[flow][0])
    retrans = flows[flow][0] - 1 * flows[flow][1]
    retranss.append(float(retrans) / flows[flow][1])

average_retrans = sum(retranss)/len(retranss)

with open('retrans.log', "w") as f:

    print("average_retrans:", average_retrans)

    f.write("average_retrans: ")
    f.write("%.10f" % round(average_retrans, 10))
    f.write("\n\n\n")

    for flow, stat in sorted(flows.items()):
    # for flow in flows:

        f.write(str(flow))
        f.write("\t\t\t")
        f.write(str(stat[0]))
        f.write("\t")
        f.write(str(stat[1]))
        f.write("\t")

        retrans = stat[0] - 1 * stat[1]
        f.write(str(retrans))
        f.write("\t")
        f.write("%.10f" % round(float(retrans) / stat[1], 10))
        f.write("\n")


        
