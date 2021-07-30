import os
import random as rand
import math 
import sys

#change these
nacc = 8
phases = 36
nthreads = [1, 2, 3, 4, 6, 8]
coherence_choices = ["none", "llc", "recall", "full"]
alloc_choices = ["preferred", "balanced", "lloaded"]
flow_choices = ["serial"]

i = 0
while True:
    s = "cfg_auton_driving_" + str(i) + ".txt"
    if os.path.isfile(s):
        i += 1
    else:
        break

f = open(s, "w")

#0 = gemm
#1 = fft
#2 = conv
#3 = gemm
#4 = conv
#5 = vitdodec
#6 = fft
#7 = vitdodec


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
    valid_chains = [[2, 0], [2, 3], [4, 0], [4, 3], [1, 5], [1,7], [6,5], [6,7]]
    for t in range(threads):
        
        valid = False;
        while not valid:
            devs = []
            cohs = []
            valid = True
            # NDEVICES
            if threads <= 4:
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
                size = 0 
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
            
            if threads > 4 and (size == 3 or size == 4):
                size = 2

            if size == 4 or size == 3:
                loop_cnt = 1

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
