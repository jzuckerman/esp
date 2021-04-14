import os
import random as rand
import math 
import sys

#change these
nacc = 7
phases = 36
sizes = ["S", "M", "L"]

coherence_choices = ["none", "llc", "recall", "full"]
alloc_choices = ["preferred", "balanced", "lloaded", "lutil"]
flow_choices = ["serial"]

i = 0
while True:
    s = "cfg_multiacc_" + str(i) + ".txt"
    if os.path.isfile(s):
        i += 1
    else:
        break

f = open(s, "w")

f.write(str(phases) + "\n")

for p in range(phases):
    devices = list(range(nacc))
    #nthreads = rand.choice(range(1, 9))
    threads =int(p / 3) + 1
    #threads = 12
    #threads = 1
    f.write(str(threads) + "\n")
    #512 MB / 4 bytes per word
    alloc = rand.choice(alloc_choices)
    for t in range(threads):
        # NDEVICES
        ndev = 1 

        #DATA FLOW
        if ndev == 1:
            flow_choice = "serial"
        else:
            flow_choice = rand.choice(flow_choices)
        
        loop_cnt = rand.choice(range(1,4)) 

        f.write(str(ndev) + "\n")
        f.write(flow_choice + "\n")
        f.write(alloc + "\n")
        f.write(str(loop_cnt) + "\n") 
        
        #ALLOCATION
        for d in range(ndev):
            dev = rand.randint(0,nacc-1)
            f.write(str(dev) + " ")                   
            #INPUT SIZE
            if p % 4 == 0:
                size = "S" 
            elif p % 4 == 1:
                size = "M"
            elif p % 4 == 2:
                size = "L"
            else:
                size = rand.choice(sizes)
            f.write(size + " ")
            #DEVICE
            d = rand.choice(devices)
           
            #COHERENCE
            coh = rand.choice(coherence_choices)
            if flow_choice == "p2p":
                coherence = "none"
            else:
                coherence = coh 
            f.write(coh + "\n")
