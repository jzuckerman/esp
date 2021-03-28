import os
import random as rand
import math 
import sys

#change these
nacc = 12 
nthreads = [1, 2, 3, 4, 6, 8, 10, 12] 
phases = 40

x = open("../../../hw/synth.xml", "r")
lines = x.readlines()
line = lines[2]
index = line.index("data_size=")
start = index + 11
word = line[start: start + 3]
pt_size = int(word)
x.close()

max_words = pt_size / 4
sizes = ["K1", "K2", "K4", "K8", "K16", "K32", "K64", "K128", "K256", "K512", "M1", "M2", "M4"]
patterns = ["STREAMING", "STRIDED", "IRREGULAR"]
burst_lens = [4, 8, 16, 32, 64, 128]
cb_factors = [1, 2, 4] 
reuse_factors = [1, 2, 4]
ld_st_ratios = [1, 2, 4]
stride_lens = [32, 64, 128, 256, 512]
coherence_choices = ["none", "llc", "recall", "full"]
alloc_choices = ["preferred", "balanced", "lloaded", "lutil"]
flow_choices = ["serial"]

acc_cfgs = []
if len(sys.argv) == 1:
    i = 0
    while True:
        s = str(i)
        if os.path.isdir(s):
            i += 1
        else:
            break
    dir = s 
    os.makedirs(s)
    acc_file = open(s + "/acc_cfgs" + s + ".txt", "w")

    for a in range(nacc):
        acc = {}
        #PATTERN
        pattern = rand.choice(patterns)
        
        #ACCESS FACTOR
        if pattern == "IRREGULAR":
            access_factor = rand.randint(0, 4)
        else:
            access_factor = 0
        
        burst_len = rand.choice(burst_lens)
        
        #COMPUTE BOUND FACTOR
        cb_factor = rand.choice(cb_factors)
        
        #REUSE FACTOR
        reuse_factor = rand.choice(reuse_factors)
        
        #LD ST RATIO   
        ld_st_ratio = rand.choice(ld_st_ratios)
        
        #STRIDE LEN
        if pattern == "STRIDED":
            if burst_len == 256:
                stride_len = 512
            elif burst_len >= 32:
                index = stride_lens.index(burst_len)
                stride_len = rand.choice(stride_lens[index+1:-1])
            else:
                stride_len = rand.choice(stride_lens)
        else:
            stride_len = 0
        
        #IN PLACE
#        in_place = rand.randint(0, 1)
        in_place = 0

        if in_place ==  1:
            if pattern == "IRREGULAR":
                pattern = "STREAMING"
                access_factor = 0
            if ld_st_ratio > 1 and reuse_factor > 1:
                ld_st_ratio = 1
           
        acc_file.write(str(a) + " ")    
        acc["pattern"] = pattern
        acc_file.write(pattern + " ")    
        acc["access_factor"] = access_factor
        acc_file.write(str(access_factor) + " ")    
        acc["burst_len"] = burst_len
        acc_file.write(str(burst_len) + " ")    
        acc["cb_factor"] = cb_factor
        acc_file.write(str(cb_factor) + " ")    
        acc["reuse_factor"] = reuse_factor
        acc_file.write(str(reuse_factor) + " ")    
        acc["ld_st_ratio"] = ld_st_ratio
        acc_file.write(str(ld_st_ratio) + " ")    
        acc["stride_len"] = stride_len
        acc_file.write(str(stride_len) + " ")    
        acc["in_place"] = in_place
        acc_file.write(str(in_place) + "\n")    
        acc_cfgs.append(acc);

elif len(sys.argv) == 2:
    dir = sys.argv[1]
    if not os.path.isfile(dir + "/acc_cfgs" + dir + ".txt"):
        print("acc_cfgs in dir " + sys.argv[1] + " does not exist\n")
        sys.exit()
    
    
    with open(dir + "/acc_cfgs" + dir + ".txt", "r") as acc_file:
        acc_lines = acc_file.readlines()

    for acc in range(len(acc_lines)):
        acc_params = acc_lines[acc].split(" ")
        acc_cfg = {}
        acc_cfg["pattern"] = acc_params[1]
        acc_cfg["access_factor"] = int(acc_params[2])
        acc_cfg["burst_len"] = int(acc_params[3])
        acc_cfg["cb_factor"] = int(acc_params[4])
        acc_cfg["reuse_factor"] = int(acc_params[5])
        acc_cfg["ld_st_ratio"] = int(acc_params[6])
        acc_cfg["stride_len"] = int(acc_params[7])
        acc_cfg["in_place"] = int(acc_params[8])
        acc_cfgs.append(acc_cfg)

else:
    print("usage: python3 cfg_synth_static.py [soc_config_file]")
    sys.exit()

i = 0
while True:
    s = "cfg_synth" + dir + "_" + str(i) + ".txt"
    if os.path.isfile(dir + "/" + s):
        i += 1
    else:
        break

f = open(dir + "/" + s, "w")

f.write(str(phases) + "\n")

for p in range(phases):
    devices = list(range(nacc))
    #nthreads = rand.choice(range(1, 9))
    threads = nthreads[int(p / 5)]
    #threads = 12
    #threads = 1
    f.write(str(threads) + "\n")
    #512 MB / 4 bytes per word
    total_size_avail = math.pow(2, 29) / 4
    alloc = rand.choice(alloc_choices)
    for t in range(threads):
        # NDEVICES
        valid = False
        while not valid:
            valid = True
            devs_used = []
            avail = len(devices) - (threads - (t + 1))
            devs = rand.choice(range(1, avail+1)) 
            ndev = devs

            #DATA FLOW
            if ndev == 1:
                flow_choice = "serial"
            else:
                flow_choice = rand.choice(flow_choices)

            if flow_choice == "p2p":
                p2p_burst_len = rand.choice(burst_lens)
                p2p_reuse_factor = rand.choice(reuse_factors) 

            #INPUT SIZE
            if p % 5 == 0:
                size = rand.choice(sizes[0:5])
            elif p % 5 == 1:
                size = rand.choice(sizes[5:8])
            elif p % 5 == 2:
                size = rand.choice(sizes[8:10])
            elif p % 5 == 3:
                size = rand.choice(sizes[10:13])
            else:
                size = rand.choice(sizes)

            log_size = 10 + sizes.index(size)
            thread_size_start = math.pow(2, log_size) 
            while True:
                if thread_size_start <= total_size_avail:
                    break
                size = rand.choice(sizes)
                log_size = 10 + sizes.index(size)
                thread_size_start = math.pow(2, log_size) 

            thread_size_avail = pt_size * math.pow(2, 20) / 4
            thread_size_avail -= thread_size_start 
            total_size_avail -= thread_size_start
            
            #ALLOCATION
            #loop_cnt = 1
            loop_cnt = rand.choice(range(1,4)) 
            first = True
            for d in range(ndev):
                if log_size < 10:
                    valid = False
                
                #DEVICE
                d = rand.choice(devices)
                devices.remove(d)
                devs_used.append(d)
                #PATTERN
                pattern = acc_cfgs[d]["pattern"]

                #ACCESS FACTOR
                access_factor = acc_cfgs[d]["access_factor"]
                log_size -= access_factor
                
                #BURST LEN
                burst_len = acc_cfgs[d]["burst_len"]
                
                #COMPUTE BOUND FACTOR
                cb_factor = acc_cfgs[d]["cb_factor"]
                cb_factor = rand.choice(cb_factors)
                
                #REUSE FACTOR
                reuse_factor = acc_cfgs[d]["reuse_factor"]
                
                #LD ST RATIO   
                ld_st_ratio = acc_cfgs[d]["ld_st_ratio"]
                log_size -= ld_st_ratios.index(ld_st_ratio)
                
                #STRIDE LEN
                stride_len = acc_cfgs[d]["stride_len"]
                
                #IN PLACE
                out_size = math.pow(2, log_size)
                
                in_place = acc_cfgs[d]["in_place"]
                
                if in_place == 0 and (not flow_choice == "p2p" or d == ndev - 1):
                    thread_size_avail -= out_size 
                    total_size_avail -= out_size
                
                if thread_size_avail < 0 or total_size_avail < 0:
                    valid = False

                #WRITE DATA
                wr_data = rand.randint(0, 4294967295)
                acc_cfgs[d]["wr_data"] = wr_data

                #READ DATA
                if first:
                    rd_data = rand.randint(0, 4294967295)
                else: 
                    rd_data = last_wr_data
                acc_cfgs[d]["rd_data"] = rd_data
                
                last_wr_data = wr_data
                first = False

                #COHERENCE
                coh = rand.choice(coherence_choices)
                if flow_choice == "p2p":
                    coherence = "none"
                else:
                    coherence = coh 
                acc_cfgs[d]["coherence"] = coherence

                if not valid:
                    for d in devs_used:
                        devs_used.remove(d)
                        devices.append(d)
                    total_size_avail += thread_size_start
        
        last_thread_size = thread_size_start
        f.write(str(ndev) + "\n")
        f.write(flow_choice + "\n")
        f.write(str(size) + "\n")
        f.write(alloc + "\n")
        f.write(str(loop_cnt) + "\n") 
        for d in devs_used:    
            f.write(str(d) + " ")
            f.write(str(acc_cfgs[d]["pattern"]) + " ")
            f.write(str(acc_cfgs[d]["access_factor"]) + " ")
            f.write(str(acc_cfgs[d]["burst_len"]) + " ")
            f.write(str(acc_cfgs[d]["cb_factor"]) + " ")
            f.write(str(acc_cfgs[d]["reuse_factor"]) + " ")
            f.write(str(acc_cfgs[d]["ld_st_ratio"]) + " ")
            f.write(str(acc_cfgs[d]["stride_len"]) + " ")
            f.write(str(acc_cfgs[d]["in_place"]) + " ")
            f.write(str(acc_cfgs[d]["wr_data"]) + " ")
            f.write(str(acc_cfgs[d]["rd_data"]) + " ")
            f.write(acc_cfgs[d]["coherence"] + "\n")
            
f.close()
