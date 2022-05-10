import seaborn as sns
import pandas as pd  
import matplotlib.pyplot as plt

def plot_local_rate(): 
    df = pd.DataFrame(columns=["approach", "AFCT", "TCT", "Rate", "key", "topology"])

    with open("local_rate.log") as file: 
        rate = 100000
        afct = 0 
        tct = 0 
        retrans = 0
        key = 0
        topology = 0 
        
        i = 0 
        for line in file.readlines():
            
            if i == 0: 
                key = line.strip().split("_")[5]
                topology = line.strip().split("_")[6]
            if i == 1: 
                afct = line.strip().split(" ")[1]
            if i == 2: 
                tct = line.strip().split(" ")[1]
            if i == 3: 
                retrans = line.strip().split(" ")[1]
            i +=1 
            if i == 4:
                df = df.append({
                    'approach': "local", 
                    'AFCT': float(afct), 
                    'TCT': float(tct), 
                    "Rate": float(rate),
                    "key": int(key),
                    "topology": int(topology),
                }, ignore_index=True)
                i = 0  
                rate += 100000
                if rate == 1000000:
                    rate = 100000

    print (df)
    ax = sns.lineplot(data=df,  x="Rate", y="AFCT", markers=["o"], palette='flare', hue="topology")
    ax = sns.lineplot(data=df,  x="Rate", y="TCT", markers=["o"], palette='crest', hue="topology")
    # plt.legend(title='Topology', labels=['1', '2', '3', '4', '5', '6'])
    ax.set_ylabel("time (s)")
    ax.set_ylim([0, 30])

    plt.savefig("local_rate.png", dpi=300, bbox_inches='tight')


def plot_remote_rate(): 
    df = pd.DataFrame(columns=["approach", "AFCT", "TCT", "Rate", "key", "topology"])

    with open("remote_rate.log") as file: 
        rate = 100000
        afct = 0 
        tct = 0 
        retrans = 0
        key = 0
        topology = 0 
        
        i = 0 
        for line in file.readlines():
            if i == 0: 
                key = line.strip().split("_")[5]
                topology = line.strip().split("_")[6]
            if i == 1: 
                afct = line.strip().split(" ")[1]
            if i == 2: 
                tct = line.strip().split(" ")[1]
            if i == 3: 
                retrans = line.strip().split(" ")[1]
            i +=1 
            if i == 4:
                df = df.append({
                    'approach': "remote", 
                    'AFCT': float(afct), 
                    'TCT': float(tct), 
                    "Rate": float(rate),
                    "key": int(key),
                    "topology": int(topology),
                }, ignore_index=True)
                i = 0  
                rate += 100000
                if rate == 1000000:
                    rate = 100000

    print (df)
    ax = sns.lineplot(data=df,  x="Rate", y="AFCT", markers=["o"], palette='flare', hue="topology")
    ax = sns.lineplot(data=df,  x="Rate", y="TCT", markers=["o"], palette='crest', hue="topology")
    #ax.legend(labels=['AFCT','TCT'])
    ax.set_ylim([0, 30])
    ax.set_ylabel("time (s)")
    plt.savefig("remote_rate.png", dpi=300, bbox_inches='tight')



def plot_eventual_timeout(): 
    df = pd.DataFrame(columns=["approach", "AFCT", "TCT", "retransmission", "timeout", "key", "topology"])

    with open("eventual_timeout.log") as file: 
        timeout = 0
        afct = 0 
        tct = 0 
        retrans = 0
        key = 0
        topology = 0 

        i = 0 
        for line in file.readlines():
            if i == 0: 
                timeout = line.strip().split("_")[4]
                key = line.strip().split("_")[5]
                topology = line.strip().split("_")[6]
            if i == 1: 
                afct = line.strip().split(" ")[1]
            if i == 2: 
                tct = line.strip().split(" ")[1]
            if i == 3: 
                retrans = line.strip().split(" ")[1]
            i +=1 
            if i == 4:
                df = df.append({
                    'approach': "eventual", 
                    'AFCT': float(afct), 
                    'TCT': float(tct), 
                    "timeout": float(timeout),
                    "key": int(key),
                    "topology": int(topology),
                }, ignore_index=True)
                i = 0  

    print (df)
    ax = sns.lineplot(data=df,  x="timeout", y="AFCT", markers=["o"], palette='flare', hue="topology")
    ax = sns.lineplot(data=df,  x="timeout", y="TCT", markers=["o"], palette='crest', hue="topology")
    # ax.legend(labels=['AFCT','TCT'])
    # ax.set_ylim([0, 30])
    ax.set_ylabel("time (s)")
    plt.savefig("eventual_timeout.png", dpi=300, bbox_inches='tight')


def plot_cache_key(): 
    df = pd.DataFrame(columns=["approach", "AFCT", "TCT", "retransmission", "key", "topology"])

    with open("cache_key.log") as file: 
        timeout = 0
        afct = 0 
        tct = 0 
        retrans = 0
        i = 0 
        key = 0
        topology = 0 

        for line in file.readlines():
            if i == 0: 
                key = line.strip().split("_")[3]
                topology = line.strip().split("_")[4]
            if i == 1: 
                afct = line.strip().split(" ")[1]
            if i == 2: 
                tct = line.strip().split(" ")[1]
            if i == 3: 
                retrans = line.strip().split(" ")[1]
            i +=1 
            if i == 4:
                df = df.append({
                    'approach': "cache", 
                    'AFCT': float(afct), 
                    'TCT': float(tct), 
                    "retransmission": float(retrans),
                    "key": int(key),
                    "topology": int(topology),
                }, ignore_index=True)
                i = 0  

    print (df)

    for measure in ["AFCT", "TCT"]:
        t1 = df[['key', 'topology', measure]].groupby(['key', 'topology']).sum().unstack('topology')
        print (t1)

    # ax = sns.lineplot(data=df,  x="timeout", y="AFCT", markers=["o"])
    # ax = sns.lineplot(data=df,  x="timeout", y="TCT", markers=["o"])
    # ax.legend(labels=['AFCT','TCT'])
    # ax.set_ylim([0, 30])
    # ax.set_ylabel("time (s)")
    # plt.savefig("eventual_timeout.png", dpi=300, bbox_inches='tight')

plot_local_rate()
plt.clf()
plot_remote_rate()
plt.clf()
# plot_eventual_timeout() 
# plt.clf()
plot_cache_key() 