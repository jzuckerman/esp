#!/usr/bin/python
import sys
from scipy.stats import gmean

if len(sys.argv) < 4:
    print("Usage: python process_train.py file phases iters")
    exit()

phases=int(sys.argv[2])
iters=int(sys.argv[3])

file=sys.argv[1]

with open(file, 'r') as f:
    lines = f.readlines()

outname = "train_" + str(iters) + "_processed.csv"
out = open(outname, 'w')

line_idx=1
time_idx=5
ddr_idx=6

#process non-coherent baseline
non_coh_time=[]
non_coh_ddr=[]
for _ in range(phases):
#    print(line_idx)
    line = lines[line_idx].split(',')
    non_coh_time.append(line[time_idx])
    non_coh_ddr.append(line[ddr_idx])
    line_idx += 1

for j in range(iters):
    #skip training trials
    line_idx += phases
    learn_time_scaled = []
    learn_ddr_scaled = []
    for i in range(phases):
        line = lines[line_idx].split(',')
        learn_time_scaled.append(float(line[time_idx]) / float(non_coh_time[i]))
        learn_ddr_scaled.append((float(line[ddr_idx]) + 1.0) / float(non_coh_ddr[i]))
        line_idx += 1
   
    learn_time_mean=gmean(learn_time_scaled)
    learn_ddr_mean=gmean(learn_ddr_scaled)
    out.write("learn" + str(j) + "," + str(learn_time_mean) + "," + str(learn_ddr_mean) + "\n")
