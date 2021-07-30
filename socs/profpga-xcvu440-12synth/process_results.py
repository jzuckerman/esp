#!/usr/bin/python
import sys
from scipy.stats import gmean

if len(sys.argv) < 2:
    print("Usage: python process_results.py file")
    exit()

phases=40

file=sys.argv[1]

with open(file, 'r') as f:
    lines = f.readlines()

outname = "soc0_results.csv"
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

    out.write("non-coh-dma,1,1\n")

#skip training trials
llc_time_scaled = []
llc_ddr_scaled = []
for i in range(phases):
    line = lines[line_idx].split(',')
    llc_time_scaled.append(float(line[time_idx]) / float(non_coh_time[i]))
    llc_ddr_scaled.append((float(line[ddr_idx]) + 1.0) / float(non_coh_ddr[i]))
    line_idx += 1

    llc_time_mean=gmean(llc_time_scaled)
    llc_ddr_mean=gmean(llc_ddr_scaled)
    out.write("llc-coh-dma" +  "," + str(llc_time_mean) + "," + str(llc_ddr_mean) + "\n")

recall_time_scaled = []
recall_ddr_scaled = []
for i in range(phases):
    line = lines[line_idx].split(',')
    recall_time_scaled.append(float(line[time_idx]) / float(non_coh_time[i]))
    recall_ddr_scaled.append((float(line[ddr_idx]) + 1.0) / float(non_coh_ddr[i]))
    line_idx += 1

    recall_time_mean=gmean(recall_time_scaled)
    recall_ddr_mean=gmean(recall_ddr_scaled)
    out.write("coh-dma" +  "," + str(recall_time_mean) + "," + str(recall_ddr_mean) + "\n")

full_time_scaled = []
full_ddr_scaled = []
for i in range(phases):
    line = lines[line_idx].split(',')
    full_time_scaled.append(float(line[time_idx]) / float(non_coh_time[i]))
    full_ddr_scaled.append((float(line[ddr_idx]) + 1.0) / float(non_coh_ddr[i]))
    line_idx += 1

    full_time_mean=gmean(full_time_scaled)
    full_ddr_mean=gmean(full_ddr_scaled)
    out.write("full-coh" +  "," + str(full_time_mean) + "," + str(full_ddr_mean) + "\n")

rand_time_scaled = []
rand_ddr_scaled = []
for i in range(phases):
    line = lines[line_idx].split(',')
    rand_time_scaled.append(float(line[time_idx]) / float(non_coh_time[i]))
    rand_ddr_scaled.append((float(line[ddr_idx]) + 1.0) / float(non_coh_ddr[i]))
    line_idx += 1

    rand_time_mean=gmean(rand_time_scaled)
    rand_ddr_mean=gmean(rand_ddr_scaled)
    out.write("rand" +  "," + str(rand_time_mean) + "," + str(rand_ddr_mean) + "\n")rand_time_scaled = []

design_time_scaled = []
design_ddr_scaled = []
for i in range(phases):
    line = lines[line_idx].split(',')
    design_time_scaled.append(float(line[time_idx]) / float(non_coh_time[i]))
    design_ddr_scaled.append((float(line[ddr_idx]) + 1.0) / float(non_coh_ddr[i]))
    line_idx += 1

    design_time_mean=gmean(design_time_scaled)
    design_ddr_mean=gmean(design_ddr_scaled)
    out.write("design" +  "," + str(design_time_mean) + "," + str(design_ddr_mean) + "\n")

manual_time_scaled = []
manual_ddr_scaled = []
for i in range(phases):
    line = lines[line_idx].split(',')
    manual_time_scaled.append(float(line[time_idx]) / float(non_coh_time[i]))
    manual_ddr_scaled.append((float(line[ddr_idx]) + 1.0) / float(non_coh_ddr[i]))
    line_idx += 1

    manual_time_mean=gmean(manual_time_scaled)
    manual_ddr_mean=gmean(manual_ddr_scaled)
    out.write("manual" +  "," + str(manual_time_mean) + "," + str(manual_ddr_mean) + "\n")

learn_time_scaled = []
learn_ddr_scaled = []
for i in range(phases):
    line = lines[line_idx].split(',')
    learn_time_scaled.append(float(line[time_idx]) / float(non_coh_time[i]))
    learn_ddr_scaled.append((float(line[ddr_idx]) + 1.0) / float(non_coh_ddr[i]))
    line_idx += 1

    learn_time_mean=gmean(learn_time_scaled)
    learn_ddr_mean=gmean(learn_ddr_scaled)
    out.write("learn" +  "," + str(learn_time_mean) + "," + str(learn_ddr_mean) + "\n")
