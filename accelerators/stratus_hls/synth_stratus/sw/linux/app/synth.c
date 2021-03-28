// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include "libesp.h"
#include "synth_stratus.h"
#include "string.h"
#include "socmap.h"

#define DEBUG 1
#define dprintf if(DEBUG) printf

#define NPHASES_MAX 100
#define NTHREADS_MAX 16
#define NDEV_MAX 16
#define IRREGULAR_SEED_MAX 2048

#define CFG 0
#define FIXED 1
#define AUTO 2
#define RAND 3
#define FIRST 4
#define DESIGN 5

unsigned long long total_alloc = 0;

typedef struct accelerator_thread_info {
    int tid; 
    int ndev; 
    int chain[NDEV_MAX];
    int p2p;
    int loop_cnt;
    size_t memsz;
    enum contig_alloc_policy alloc_choice; 
    struct timespec th_start;
    struct timespec th_end; 
    unsigned long long thread_time;
} accelerator_thread_info_t;

size_t size_to_bytes (char* size){
    if (!strncmp(size, "M8", 2)){
        return 8388608;
    } else if (!strncmp(size, "M4", 2)){
        return 4194304;
    } else if (!strncmp(size, "M2", 2)){
        return 2097152;
    } else if (!strncmp(size, "M1", 2)){
        return 1048576;
    } else if (!strncmp(size, "K512", 4)){
        return 524288;
    } else if (!strncmp(size, "K256", 4)){
        return 262144;
    } else if (!strncmp(size, "K128", 4)){
        return 131072;
    } else if (!strncmp(size, "K64", 3)){
        return 65536;
    } else if (!strncmp(size, "K32", 3)){
        return 32768;
    } else if (!strncmp(size, "K16", 3)){
        return 16384;
    } else if (!strncmp(size, "K8", 2)){
        return 8192;
    } else if (!strncmp(size, "K4", 2)){
        return 4096;
    } else if (!strncmp(size, "K2", 2)){
        return 2048;
    } else if (!strncmp(size, "K1", 2)){
        return 1024;
    } else {
        return -1;
    }
}

char *devnames[] = {
"synth_stratus.0",
"synth_stratus.1",
"synth_stratus.2",
"synth_stratus.3",
"synth_stratus.4",
"synth_stratus.5",
"synth_stratus.6",
"synth_stratus.7",
"synth_stratus.8",
"synth_stratus.9",
"synth_stratus.10",
"synth_stratus.11",
"synth_stratus.12",
"synth_stratus.13",
"synth_stratus.14",
"synth_stratus.15",
};

enum accelerator_coherence design_choices[] = {
ACC_COH_NONE,
ACC_COH_NONE,
ACC_COH_NONE,
ACC_COH_NONE,
ACC_COH_NONE,
ACC_COH_NONE,
ACC_COH_NONE,
ACC_COH_LLC,
ACC_COH_NONE,
ACC_COH_LLC,
ACC_COH_NONE,
ACC_COH_LLC,
};

static void config_threads(FILE* f, accelerator_thread_info_t **thread_info, esp_thread_info_t ***cfg, struct synth_stratus_access ***synth_cfg, int phase, int* nthreads, int coherence_mode, enum accelerator_coherence coherence, unsigned **nacc, unsigned **loop_cnt){
    fscanf(f, "%d", nthreads); 
    dprintf("%d threads in phase %d\n", *nthreads, phase); 
    *cfg = malloc(sizeof(esp_thread_info_t*) * *nthreads);
    *nacc = malloc(sizeof(unsigned) * *nthreads);
    *loop_cnt = malloc(sizeof(unsigned) * *nthreads); 
    *synth_cfg = malloc(sizeof(struct synth_stratus_access*) * *nthreads);
    for (int t = 0; t < *nthreads; t++){
        thread_info[t] = malloc(sizeof(accelerator_thread_info_t));
        thread_info[t]->tid = t;
                
        //get number of devices and size
        fscanf(f, "%d\n", &(thread_info[t]->ndev));
        dprintf("%d devices in thread %d.%d\n", thread_info[t]->ndev, phase, t);
 
        (*cfg)[t] = malloc(sizeof(esp_thread_info_t) * thread_info[t]->ndev);
        (*synth_cfg)[t] = malloc(sizeof(struct synth_stratus_access) * thread_info[t]->ndev);
        (*nacc)[t] = thread_info[t]->ndev;
        char flow_choice[7];
        fscanf(f, "%s\n", flow_choice);

        if(!strcmp(flow_choice, "p2p")){
           thread_info[t]->p2p = 1;
        } else { 
           thread_info[t]->p2p = 0;
        }
        
        char size[5];
        fscanf(f, "%s\n", size); 
        
        char alloc_choice[13];
        fscanf(f, "%s\n", alloc_choice);

        if (!strcmp(alloc_choice, "preferred")){
            thread_info[t]->alloc_choice = CONTIG_ALLOC_PREFERRED;
        } else if (!strcmp(alloc_choice, "lloaded")){
            thread_info[t]->alloc_choice = CONTIG_ALLOC_LEAST_LOADED;
        } else if (!strcmp(alloc_choice, "balanced")){  
            thread_info[t]->alloc_choice = CONTIG_ALLOC_BALANCED;
        } else if (!strcmp(alloc_choice, "lutil")){
            thread_info[t]->alloc_choice = CONTIG_ALLOC_LEAST_UTILIZED;
        } 

        fscanf(f, "%d\n", &thread_info[t]->loop_cnt);
        (*loop_cnt)[t] = thread_info[t]->loop_cnt;
        dprintf("%d loops in thread %d.%d\n", thread_info[t]->loop_cnt, phase, t);

        size_t in_size;  
        in_size = size_to_bytes(size); 

        size_t memsz = in_size; 
        unsigned int offset = 0; 

        char pattern[10];
        char coh_choice[7];
        int prev_devid = 0;
        for (int d = 0; d < thread_info[t]->ndev; d++){
            fscanf(f, "%d", &(thread_info[t]->chain[d])); 
            
            //esp accelerator parameters
            int devid = thread_info[t]->chain[d];
            (*cfg)[t][d].run = true;
            (*cfg)[t][d].devname = devnames[devid];
            (*cfg)[t][d].ioctl_req = SYNTH_STRATUS_IOC_ACCESS;
            (*cfg)[t][d].esp_desc = &((*synth_cfg)[t][d].esp);
            ((*cfg)[t][d].esp_desc)->devid = devid;

            (*synth_cfg)[t][d].src_offset = 0;
            (*synth_cfg)[t][d].dst_offset = 0;
            
            if (thread_info[t]->p2p && d != thread_info[t]->ndev - 1){
                (*synth_cfg)[t][d].esp.p2p_store = 1;
            } else {
                (*synth_cfg)[t][d].esp.p2p_store = 0;
            }

            if (thread_info[t]->p2p && d != 0){
                (*synth_cfg)[t][d].esp.p2p_nsrcs = 1;
                strcpy((*synth_cfg)[t][d].esp.p2p_srcs[0], devnames[prev_devid]);
            } else {
                (*synth_cfg)[t][d].esp.p2p_nsrcs = 0;
            }
            
            //read parameters into esp_thread_info_t     
            fscanf(f, "%s", pattern); 
            if (!strncmp(pattern, "STREAMING", 9)){
                (*synth_cfg)[t][d].pattern = PATTERN_STREAMING;
            } else if (!strncmp(pattern, "STRIDED", 7)){
                (*synth_cfg)[t][d].pattern = PATTERN_STRIDED;
            } else if (!strncmp(pattern, "IRREGULAR", 9)){
                (*synth_cfg)[t][d].pattern = PATTERN_IRREGULAR;
            }
            fscanf(f, "%d %d %d %d %d %d %d %d %d %s", 
                &(*synth_cfg)[t][d].access_factor,
                &(*synth_cfg)[t][d].burst_len,
                &(*synth_cfg)[t][d].compute_bound_factor,
                &(*synth_cfg)[t][d].reuse_factor,
                &(*synth_cfg)[t][d].ld_st_ratio,
                &(*synth_cfg)[t][d].stride_len,
                &(*synth_cfg)[t][d].in_place,
                &(*synth_cfg)[t][d].wr_data,
                &(*synth_cfg)[t][d].rd_data,
                coh_choice);
            
            if ((*synth_cfg)[t][d].pattern == PATTERN_IRREGULAR)
                (*synth_cfg)[t][d].irregular_seed = rand() % IRREGULAR_SEED_MAX;
        
            //calculate output size, offset, and memsize
            (*synth_cfg)[t][d].in_size = in_size;  
            unsigned int out_size = (in_size >> (*synth_cfg)[t][d].access_factor) 
                / (*synth_cfg)[t][d].ld_st_ratio;
            (*synth_cfg)[t][d].out_size = out_size;        
            (*synth_cfg)[t][d].offset = offset; 
           
            if((*synth_cfg)[t][d].in_place == 0 && !thread_info[t]->p2p){
                memsz += out_size;
                offset += in_size;
            }

            if (thread_info[t]->p2p && d == thread_info[t]->ndev - 1){
                memsz += out_size;
                (*synth_cfg)[t][d].offset = (*synth_cfg)[t][0].in_size - in_size;
            }
           
            unsigned int footprint = in_size >> (*synth_cfg)[t][d].access_factor;

            if (!(*synth_cfg)[t][d].in_place && (!thread_info[t]->p2p || d == thread_info[t]->ndev - 1))
                footprint += out_size;
                    
            (*synth_cfg)[t][d].esp.learn = 0;
            if (thread_info[t]->p2p){
                (*synth_cfg)[t][d].esp.coherence = ACC_COH_NONE;
            } 
            else if (coherence_mode == FIXED){
                (*synth_cfg)[t][d].esp.coherence = coherence;
                if (coherence == ACC_COH_LEARN)
                    (*synth_cfg)[t][d].esp.learn = 1;
            }
            else if (coherence_mode == RAND){
                (*synth_cfg)[t][d].esp.coherence = rand() % 4;
            }
            else if (coherence_mode == DESIGN){
                (*synth_cfg)[t][d].esp.coherence = design_choices[devid];
            }
            else if (!strcmp(coh_choice, "none")){
                (*synth_cfg)[t][d].esp.coherence = ACC_COH_NONE;
            }
            else if (!strcmp(coh_choice, "llc")){
                (*synth_cfg)[t][d].esp.coherence = ACC_COH_LLC;
            }
            else if (!strcmp(coh_choice, "recall")){
                (*synth_cfg)[t][d].esp.coherence = ACC_COH_RECALL;
            }
            else if (!strcmp(coh_choice, "full")){
                (*synth_cfg)[t][d].esp.coherence = ACC_COH_FULL;
            }

            (*synth_cfg)[t][d].esp.footprint = footprint * 4; 
            (*synth_cfg)[t][d].esp.in_place = (*synth_cfg)[t][d].in_place; 
            (*synth_cfg)[t][d].esp.reuse_factor = (*synth_cfg)[t][d].reuse_factor;

            in_size = out_size; 
            prev_devid = devid;
        }
        thread_info[t]->memsz = memsz * 4;
    }
}

static void alloc_phase(accelerator_thread_info_t **thread_info, esp_thread_info_t ***cfg, struct synth_stratus_access ***synth_cfg, int nthreads, int alloc_mode, enum alloc_effort alloc, uint32_t **buffers, int phase, struct contig_alloc_params *alloc_params){
       
    //set policy
    for (int i = 0; i < nthreads; i++){
        struct contig_alloc_params params; 

        if (alloc_mode == CFG){
            params.policy = thread_info[i]->alloc_choice;
        } 
        else if (alloc_mode == FIXED || alloc_mode == FIRST){
            params.policy = alloc;
            thread_info[i]->alloc_choice = alloc;
        }
        else if (alloc_mode == RAND){
            alloc = rand() % 3;
            params.policy = alloc;
            thread_info[i]->alloc_choice = alloc;
        }
        if (alloc_mode == CFG || alloc_mode == FIXED || alloc_mode == RAND || alloc_mode == FIRST){
            if (params.policy == CONTIG_ALLOC_PREFERRED){
                params.pol.first.ddr_node = 0;
            } else if (params.policy == CONTIG_ALLOC_BALANCED){
                params.pol.balanced.threshold = 0;
                params.pol.balanced.cluster_size = 1;
            } else if (params.policy == CONTIG_ALLOC_LEAST_LOADED){
                params.pol.lloaded.threshold = 0; 
            }  else if (params.policy == CONTIG_ALLOC_AUTO){
                params.pol.automatic.threshold = 0;
                params.pol.automatic.cluster_size = 1;
                params.pol.automatic.ddr_node = 0;
            }
        }
        
        alloc_params[i] = params;
        total_alloc += thread_info[i]->memsz;
        
        if (alloc_mode == FIRST)
            buffers[i] = (uint32_t *) esp_alloc_policy(&params, thread_info[i]->memsz);  
        else
            buffers[i] = (uint32_t *) esp_alloc_policy_accs(&params, thread_info[i]->memsz, thread_info[i]->chain, thread_info[i]->ndev);  

        thread_info[i]->alloc_choice = params.policy;
        if (buffers[i] == NULL){
            die_errno("error: cannot allocate %zu contig bytes", thread_info[i]->memsz);   
        }

        for (int j = 0; j < (*synth_cfg)[i][0].in_size; j++){
           buffers[i][j] = (*synth_cfg)[i][0].rd_data;
        }

        for (int acc = 0; acc < thread_info[i]->ndev; acc++){
            (*cfg)[i][acc].hw_buf = (void*) buffers[i];
        }
    }
}

static int validate_buffer(accelerator_thread_info_t *thread_info,  struct synth_stratus_access **synth_cfg, uint32_t *buf){
    int errors = 0; 
    for (int i = 0; i < thread_info->ndev; i++){
        if (thread_info->p2p && i != thread_info->ndev - 1)
            continue;

        int t = thread_info->tid;
        int offset = synth_cfg[t][i].offset;
        int in_size = synth_cfg[t][i].in_size;
        int out_size = synth_cfg[t][i].out_size;
        int in_place = synth_cfg[t][i].in_place;
        int wr_data = synth_cfg[t][i].wr_data;
        int next_in_place;
        
        if (i != thread_info->ndev - 1){
           next_in_place = synth_cfg[t][i+1].in_place;
           if (next_in_place)
               continue;
        }

        if (!in_place && !thread_info->p2p)
            offset += in_size; 
        else if (thread_info->p2p)
            offset = synth_cfg[t][0].in_size;

        for (int j = offset; j < offset + out_size; j++){
            if (j == offset + out_size - 1 && buf[j] != wr_data){
                errors += buf[j];
                printf("%u read errors in thread %d device %d\n", buf[j], t, i);
            }
            else if (j != offset + out_size - 1 && buf[j] != wr_data){
                errors++;
            }
        }
    }
    return errors;
}

static void free_phase(accelerator_thread_info_t **thread_info, esp_thread_info_t **cfg, struct synth_stratus_access **synth_cfg, int nthreads){
    for (int i = 0; i < nthreads; i++){
        esp_free(cfg[i]->hw_buf); 
        free(thread_info[i]);
        free(cfg[i]);
        free(synth_cfg[i]);
    }
    free(synth_cfg);
    free(cfg);
}

static void dump_results(FILE* out_file_d, FILE* out_file_t, FILE* out_file_p, FILE* out_file_status_d, accelerator_thread_info_t **thread_info, esp_thread_info_t ** cfg, struct synth_stratus_access **synth_cfg, int phase, int nthreads, char** argv, int test_no, unsigned long long *phase_total_cycles, unsigned long long phase_time, unsigned long long phase_ddr_accesses){
    int t, d, l;
    unsigned long long thread_ns;
    unsigned long long thread_ddr_accesses;
    unsigned long long phase_size = 0;
    unsigned long long phase_cycles = 0;
    unsigned long long phase_ns = 0;
    int phase_adj = test_no * 40 + phase;
    int i;
    for (t = 0; t < nthreads; t++){
        thread_ns = 0;
        thread_ddr_accesses = 0;
        for (l = 0; l < thread_info[t]->loop_cnt; l++){
            for (d = 0; d < thread_info[t]->ndev; d++){
                fprintf(out_file_status_d, "%u,%u,", cfg[t][d].esp_status[l].active_acc_cnt, cfg[t][d].esp_status[l].active_acc_cnt_full);
                for (i = 0; i < SOC_NDDR_CONTIG; i++)
                    fprintf(out_file_status_d, "%u,", cfg[t][d].esp_status[l].active_non_coh_split[i]);
                for (i = 0; i < SOC_NDDR_CONTIG; i++)
                    fprintf(out_file_status_d, "%u,", cfg[t][d].esp_status[l].active_to_llc_split[i]);
                fprintf(out_file_status_d, "%u,",cfg[t][d].esp_status[l].active_footprint);
                for (i = 0; i < SOC_NDDR_CONTIG; i++)
                    fprintf(out_file_status_d, "%u,", cfg[t][d].esp_status[l].active_llc_footprint_split[i]);
                for (i = 0; i < SOC_NDDR_CONTIG; i++){
                    fprintf(out_file_status_d, "%u", cfg[t][d].esp_status[l].active_footprint_split[i]);
                    if (i == SOC_NDDR_CONTIG-1)
                        fprintf(out_file_status_d, "\n");
                    else 
                        fprintf(out_file_status_d, ",");
                }
                thread_ns += cfg[t][d].hw_ns[l];
                phase_ns += cfg[t][d].hw_ns[l];
                thread_ddr_accesses += cfg[t][d].ddr_accesses[l];
                phase_cycles += cfg[t][d].acc_stats[l].acc_tot_lo;

                fprintf(out_file_d,"%d-%d-%d-%d,", phase_adj, t, l, d);
                fprintf(out_file_d,"%d,", thread_info[t]->chain[d]);
                if (synth_cfg[t][d].esp.coherence == ACC_COH_FULL)
                    fprintf(out_file_d,"full,");
                else if (synth_cfg[t][d].esp.coherence == ACC_COH_LLC)
                    fprintf(out_file_d,"llc,");
                else if (synth_cfg[t][d].esp.coherence == ACC_COH_RECALL)
                    fprintf(out_file_d,"recall,");
                else if (synth_cfg[t][d].esp.coherence == ACC_COH_NONE)
                    fprintf(out_file_d,"none,");
                else if (synth_cfg[t][d].esp.coherence == ACC_COH_AUTO)
                    fprintf(out_file_d,"auto,");
                else if (synth_cfg[t][d].esp.coherence == ACC_COH_LEARN)
                    fprintf(out_file_d,"learn,");

                if (thread_info[t]->alloc_choice == CONTIG_ALLOC_BALANCED)
                    fprintf(out_file_d,"balanced,");
                else if (thread_info[t]->alloc_choice == CONTIG_ALLOC_PREFERRED)
                    fprintf(out_file_d,"preferred,");
                else if (thread_info[t]->alloc_choice == CONTIG_ALLOC_LEAST_LOADED)
                    fprintf(out_file_d,"lloaded,");
                else if (thread_info[t]->alloc_choice == CONTIG_ALLOC_LEAST_UTILIZED)
                    fprintf(out_file_d,"lutil,");

                fprintf(out_file_d, "%u,", synth_cfg[t][d].esp.footprint); 

                fprintf(out_file_d,"%d,", synth_cfg[t][d].esp.ddr_node);
                fprintf(out_file_d,"%llu,", cfg[t][d].hw_ns[l]);
                fprintf(out_file_d,"%u,", cfg[t][d].ddr_accesses[l]);
                fprintf(out_file_d,"%u,", cfg[t][d].acc_stats[l].acc_tlb);
                fprintf(out_file_d,"%u,", cfg[t][d].acc_stats[l].acc_mem_lo);
                fprintf(out_file_d,"%u,", cfg[t][d].acc_stats[l].acc_mem_hi);
                fprintf(out_file_d,"%u,", cfg[t][d].acc_stats[l].acc_tot_lo);
                fprintf(out_file_d,"%d,", cfg[t][d].acc_stats[l].acc_tot_hi);
                fprintf(out_file_d,"%u,", cfg[t][d].state[l]);
                fprintf(out_file_d,"%f,", cfg[t][d].best_time[l]);
                fprintf(out_file_d,"%f\n", cfg[t][d].reward[l]);
            }
        }
        fprintf(out_file_t, "%d-%d,", phase_adj, t);
        fprintf(out_file_t, "%d,", thread_info[t]->ndev);
        fprintf(out_file_t, "%d,", thread_info[t]->loop_cnt);
        fprintf(out_file_t, "%s,", argv[2]);
        if (thread_info[t]->alloc_choice == CONTIG_ALLOC_BALANCED)
            fprintf(out_file_t,"balanced,");
        else if (thread_info[t]->alloc_choice == CONTIG_ALLOC_PREFERRED)
            fprintf(out_file_t,"preferred,");
        else if (thread_info[t]->alloc_choice == CONTIG_ALLOC_LEAST_LOADED)
            fprintf(out_file_t,"lloaded,");
        else if (thread_info[t]->alloc_choice == CONTIG_ALLOC_LEAST_UTILIZED)
            fprintf(out_file_t,"lutil,");
        fprintf(out_file_t, "%zu,", thread_info[t]->memsz);
        phase_size += thread_info[t]->memsz; 
        
        fprintf(out_file_t,"%d,", synth_cfg[t][0].esp.ddr_node);
        fprintf(out_file_t, "%llu,", thread_info[t]->thread_time);
        fprintf(out_file_t, "%llu\n", thread_ddr_accesses);
   }
    fprintf(out_file_p, "%d,", phase_adj);
    fprintf(out_file_p, "%d,", nthreads);
    fprintf(out_file_p, "%s,", argv[2]);
    fprintf(out_file_p, "%s,", argv[3]);
    fprintf(out_file_p, "%llu,", phase_size);
    fprintf(out_file_p, "%llu,", phase_time);
    fprintf(out_file_p, "%llu\n", phase_ddr_accesses);
    *phase_total_cycles = phase_cycles;
}

static unsigned long long calculate_phase_ddr_accesses(unsigned int *ddr_accesses_start, unsigned int* ddr_accesses_end){
    unsigned long long ddr_accesses_total = 0;
    for (int m = 0; m < SOC_NDDR_CONTIG; m++){
        if (ddr_accesses_start[m] <= ddr_accesses_end[m]){
            ddr_accesses_total += ddr_accesses_end[m] - ddr_accesses_start[m];
        } else {
            ddr_accesses_total += (0xFFFFFFFF - ddr_accesses_start[m]) + ddr_accesses_end[m];
        }
    }
    return ddr_accesses_total;
}

int main (int argc, char** argv)
{
    srand(time(NULL));

    if (argc < 4){
        printf("Usage: ./synth.exe file coherence alloc [epsilon] [alpha]\n");
        return -1;
    }
    //command line args
    FILE* f;
    f = fopen(argv[1], "r");

    int test_no = argv[1][9] - 48;
    printf("test_no %d\n", test_no);
    const char* out_name_d = "synth_devices.csv";
    const char* out_name_t = "synth_threads.csv";
    const char* out_name_p = "synth_phases.csv";
    const char* out_name_status_d = "status_devices.csv";
    int i;

    FILE *out_file_d, *out_file_t, *out_file_p, *out_file_status_d;
    if (access(out_name_d, F_OK) != -1){
        out_file_d = fopen(out_name_d, "a");
    } else {
        out_file_d = fopen(out_name_d, "w");
        fprintf(out_file_d, "Device name, devID, coherence, allocation, footprint, DDR#, time, ddr_accesses, acc_tlb, acc_mem_lo, acc_mem_hi, acc_tot_lo, acc_tot_hi, state, avg_time, reward\n");
    }
    
    if (access(out_name_t, F_OK) != -1){
        out_file_t = fopen(out_name_t, "a");
    } else {
        out_file_t = fopen(out_name_t, "w");
        fprintf(out_file_t,  "Thread name, nacc, loop_cnt, coherence, allocation, footprint, DDR#, time, ddr_accesses\n");
    }
    
    if (access(out_name_p, F_OK) != -1){
        out_file_p = fopen(out_name_p, "a");
    } else {
        out_file_p = fopen(out_name_p, "w");
        fprintf(out_file_p,  "Phase name, nthreads, coherence, allocation, footprint, time, ddr_accesses\n");
    }

    if (access(out_name_status_d, F_OK) != -1){
        out_file_status_d = fopen(out_name_status_d, "a");
    } else {
        out_file_status_d = fopen(out_name_status_d, "w");
        fprintf(out_file_status_d, "active_acc_cnt, active_acc_cnt_full,");
        for (i = 0; i < SOC_NDDR_CONTIG; i++)
            fprintf(out_file_status_d, "active_non_coh_split%d,", i);
        for (i = 0; i < SOC_NDDR_CONTIG; i++)
            fprintf(out_file_status_d, "active_to_llc_split%d,", i);
        fprintf(out_file_status_d, "active_footprint,");
        for (i = 0; i < SOC_NDDR_CONTIG; i++)
            fprintf(out_file_status_d, "active_llc_footprint_split%d,", i);
        for (i = 0; i < SOC_NDDR_CONTIG; i++){
            fprintf(out_file_status_d, "active_footprint_split%d", i);
            if (i == SOC_NDDR_CONTIG-1)
                fprintf(out_file_status_d, "\n");
            else 
                fprintf(out_file_status_d, ",");
        }
    }    
    
    enum accelerator_coherence coherence = ACC_COH_NONE; 
    enum alloc_effort alloc = CONTIG_ALLOC_PREFERRED; 
    int coherence_mode, alloc_mode;
    
    if (!strcmp(argv[2], "none")){
        coherence_mode = FIXED;
        coherence = ACC_COH_NONE;
    }
    else if (!strcmp(argv[2], "llc")){
        coherence_mode = FIXED;
        coherence = ACC_COH_LLC;
    }
    else if (!strcmp(argv[2], "recall")){
        coherence_mode = FIXED;
        coherence = ACC_COH_RECALL;
    }
    else if (!strcmp(argv[2], "full")){
        coherence_mode = FIXED;
        coherence = ACC_COH_FULL;
    }
    else if (!strcmp(argv[2], "auto")){
        coherence_mode = FIXED;
        coherence = ACC_COH_AUTO;
    }
    else if (!strcmp(argv[2], "cfg")){
        coherence_mode = CFG;
    }
    else if (!strcmp(argv[2], "rand")){
        coherence_mode = RAND;
    } 
    else if (!strcmp(argv[2], "design")){
        coherence_mode = DESIGN;
    }
    else if (!strcmp(argv[2], "learn")){
        coherence_mode = FIXED;
        coherence = ACC_COH_LEARN;
        if (argc != 7){
            printf ("Supply learning parameters\n");
            return -1;
        }
        set_learning_params(atof(argv[4]), atof(argv[5]), atof(argv[6]));        
    }
    else{
        printf("Valid coherence choices include none, llc, recall, full, auto, cfg, rand, and learn\n");
        return 1;
    }

    if (!strcmp(argv[3], "preferred")){
        alloc_mode = FIXED;
        alloc = CONTIG_ALLOC_PREFERRED;
    }
    else if (!strcmp(argv[3], "lloaded")){
        alloc_mode = FIXED;
        alloc = CONTIG_ALLOC_LEAST_LOADED;
    }
    else if (!strcmp(argv[3], "balanced")){
        alloc_mode = FIXED;
        alloc = CONTIG_ALLOC_BALANCED;
    }
    else if (!strcmp(argv[3], "lutil")){
        alloc_mode = FIXED;
        alloc = CONTIG_ALLOC_LEAST_UTILIZED;
    }else if (!strcmp(argv[3], "auto")){
        alloc_mode = FIXED;
        alloc = CONTIG_ALLOC_AUTO;
    }
    else if (!strcmp(argv[3], "cfg")){
        alloc_mode = CFG;
    }
    else if (!strcmp(argv[3], "rand")){
        alloc_mode = RAND;
    } else if (!strcmp(argv[3], "first")){
        alloc_mode = FIRST;
        alloc = CONTIG_ALLOC_PREFERRED;
    }
    else{
        printf("Valid alloc choices include preferred, lloaded, balanced, auto, cfg, rand, and first\n");
        return 1;
    }

    if (argc == 7){
        set_learning_params(atof(argv[4]), atof(argv[5]), atof(argv[6]));
    } else {
        printf("No learning parameters supplied, using defaults\n");
    }

    esp_init();

    //get phases
    int nphases; 
    fscanf(f, "%d\n", &nphases); 
    dprintf("%d phases\n", nphases);
    
    struct timespec th_start;
    struct timespec th_end;
    unsigned long long hw_ns = 0, hw_ns_total = 0;
    float hw_s = 0, hw_s_total = 0;//, total_s = 0; 
    
    int nthreads;
    accelerator_thread_info_t *thread_info[NPHASES_MAX][NTHREADS_MAX];
    uint32_t *buffers[NTHREADS_MAX];
    struct contig_alloc_params params[NTHREADS_MAX];
    esp_thread_info_t **cfg = NULL;
    struct synth_stratus_access **synth_cfg = NULL;
    unsigned *nacc = NULL;
    unsigned *loop_cnt = NULL;
    unsigned long long total_cycles = 0, total_ddr_accesses = 0;
    unsigned long long phase_cycles, phase_ddr_accesses;
    unsigned int ddr_accesses_start[SOC_NDDR_CONTIG];
    unsigned int ddr_accesses_end[SOC_NDDR_CONTIG];
    //loop over phases - config, alloc, spawn thread, validate, and free
    for (int p = 0; p < nphases; p++){
        config_threads(f, thread_info[p], &cfg, &synth_cfg, p, &nthreads, coherence_mode, coherence, &nacc, &loop_cnt); 
        alloc_phase(thread_info[p], &cfg, &synth_cfg, nthreads, alloc_mode, alloc, buffers, p, params); 
        
        read_ddr_accesses(ddr_accesses_start);
        gettime(&th_start);
        
        esp_run_parallel(cfg, nthreads, nacc, loop_cnt);

        gettime(&th_end); 
        read_ddr_accesses(ddr_accesses_end);
        for (int t = 0; t < nthreads; t++){
            int errors = validate_buffer(thread_info[p][t], synth_cfg, buffers[t]);
            if (errors)
                printf("[FAIL] Thread %d.%d : %u errors\n", p, t, errors);
            else 
                printf("[PASS] Thread %d.%d\n", p, t);  
        }
        
        phase_ddr_accesses = calculate_phase_ddr_accesses(ddr_accesses_start, ddr_accesses_end);;
        
        hw_ns = ts_subtract(&th_start, &th_end);
        hw_ns_total += hw_ns; 
        hw_s = (float) hw_ns / 1000000000;

        printf("PHASE.%d %.4f s\n", p, hw_s);
      
        dump_results(out_file_d, out_file_t, out_file_p, out_file_status_d, thread_info[p], cfg, synth_cfg, p, nthreads, argv, test_no, &phase_cycles, hw_ns, phase_ddr_accesses);

        total_cycles += phase_cycles;
        total_ddr_accesses += phase_ddr_accesses;

        free_phase(thread_info[p], cfg, synth_cfg, nthreads);
        free(nacc); 
    }
    hw_s_total = (float) hw_ns_total / 1000000000;
 //   total_s = (float) total_ns / 1000000000.0;
    printf("TOTAL TIME %.9f s\n", hw_s_total); 
    
    //printf("TOTAL CYCLES %llu\n", total_cycles);
    printf("TOTAL DDR ACCESSES %llu\n", total_ddr_accesses);
    fclose(f);
    fclose(out_file_d); 
    fclose(out_file_t); 
    fclose(out_file_p); 
    esp_cleanup();
    return 0; 
}
