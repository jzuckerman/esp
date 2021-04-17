import os
import random as rand
import math 
import sys

#change these
nacc = 9
phases = 36
nthreads = [1, 2, 4, 6, 8, 9]
coherence_choices = ["none", "llc", "recall", "full"]
alloc_choices = ["preferred", "balanced", "lloaded"]
flow_choices = ["serial"]

i = 0
while True:
    s = "cfg_esp4ml_" + str(i) + ".txt"
    if os.path.isfile(s):
        i += 1
    else:
        break

f = open(s, "w")

#0 = mlp 
#1 = autoenc
#2 = mlp
#3 = nv
#4 = nv
#5 = mlp
#6 = nv
#7 = nv
#8 = mlp


f.write(str(phases) + "\n")


for p in range(phases):
    devices = list(range(nacc))
    #nthreads = rand.choice(range(1, 9))
    threads = nthreads[int(p/6)]
    #threads = 12
    #threads = 1
    f.write(str(threads) + "\n")
    #512 MB / 4 bytes per word
    alloc = rand.choice(alloc_choices)
    valid_chains = [[3, 0], [3, 2], [3, 5], [3,8],
                    [4, 0], [4, 2], [4, 5], [4,8],
                    [6, 0], [6, 2], [6, 5], [6,8],
                    [7, 0], [7, 2], [7, 5], [7,8],
                    [1, 0], [1, 2], [1, 5], [1,8]]
    
    for t in range(threads):
        
        valid = False;
        while not valid:
            devs = []
            cohs = []
            valid = True
            # NDEVICES
            if threads <= 6:
                ndev = rand.randint(1,2)
            else:
                ndev = 1
            #DATA FLOW
            if ndev == 1:
                flow_choice = "serial"
            else:
                flow_choice = rand.choice(flow_choices)
            
            loop_cnt = rand.choice(range(1,4)) 
            if ndev == 2:
                chain = rand.choice(valid_chains)
            else: 
                chain = []
    
            if ndev == 2:
                valid_chains.remove(chain)
              
            #INPUT SIZE
            if p % 6 == 0:
                size = 4 
            elif p % 6 == 1:
                size = 1
            elif p % 6 == 2:
                size = 2
            elif p % 6 == 3:
                size = 3
            elif p % 6 == 4:
                size = 4
            else:
                size = rand.randint(0,4)
           
            #ALLOCATION
            for d in range(ndev):
                if ndev == 2:
                    dev = chain[d]
                else :
                    dev = rand.choice(devices)
                
                if not dev in devices:
                    valid = False
                    break

                devices.remove(dev)
                devs.append(dev)
                    
                #COHERENCE
                coh = rand.choice(coherence_choices)
                if flow_choice == "p2p":
                    coherence = "none"
                else:
                    coherence = coh 
                cohs.append(coherence)
         
            if not valid:
                for d in devs:
                    devices.append(d)
                    valid_chains.append(chain)

        f.write(str(ndev) + "\n")
        f.write(flow_choice + "\n")
        f.write(alloc + "\n")
        f.write(str(loop_cnt) + "\n")
        for d in range(ndev):
            f.write(str(devs[d]) + " ")
            f.write(str(size) + " ")
            f.write(cohs[d] + "\n")
