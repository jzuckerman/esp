#define LIBESP
#include <limits.h>

#define LOCS
#include "socmap.h"

#define N_FULL_COH_ACC_BINS 3
#define N_NON_COH_TO_TILE_BINS 3
#define N_TO_TILE_LLC_BINS 3
#define N_FOOTPRINT_TILE_BINS 3
#define N_FOOTPRINT_BINS 3

#define STATE_SPACE_SIZE_COH (N_FULL_COH_ACC_BINS * N_NON_COH_TO_TILE_BINS * N_TO_TILE_LLC_BINS * N_FOOTPRINT_TILE_BINS * N_FOOTPRINT_BINS)

#define N_ACTIONS_COH 4

float best_times[SOC_NACC];
float worst_times[SOC_NACC];
float best_ddr[SOC_NACC];
float worst_ddr[SOC_NACC];
float best_ratios[SOC_NACC];
float worst_ratios[SOC_NACC];
float q_values_coh[STATE_SPACE_SIZE_COH][N_ACTIONS_COH];

float alpha = 0.1;
float epsilon = 0.5;
float discount = 0.5;

static unsigned int cnt_to_bin(unsigned int cnt){
    if (cnt >= 2)
        return 2;
    else if (cnt >= 1)
        return 1;
    else 
        return 0;
}

static unsigned int footprint_to_bin(unsigned int footprint, unsigned int aggr_llc_size){
    if (footprint <= cache_l2_size)
        return 0;
    else if (footprint <= aggr_llc_size) 
        return 1;
    else
        return 2;   
}

static unsigned int calculate_state_coh(struct esp_status_coh status, unsigned int contig_alloc_policy, unsigned int footprint, unsigned int ddr_n){
    unsigned int tile_non_coh_cnt, tile_llc_cnt, tile_llc_footprint, aggr_llc_size, state;
    int full_coh_bin, tile_non_coh_bin, tile_llc_bin, tile_llc_footprint_bin, footprint_bin;
    int m;
    
    tile_non_coh_cnt = 0;
    tile_llc_cnt = 0;
    tile_llc_footprint = 0;

    if (contig_alloc_policy == CONTIG_ALLOC_BALANCED){
        for (m = 0; m < SOC_NDDR_CONTIG; m++){
            tile_non_coh_cnt += status.active_non_coh_split[m];
            tile_llc_cnt += status.active_to_llc_split[m];
            tile_llc_footprint += status.active_llc_footprint_split[m];
        }
        tile_non_coh_cnt = (int) ((float) tile_non_coh_cnt/ (float)  SOC_NDDR_CONTIG);
        tile_llc_cnt = (int) ((float) tile_llc_cnt/ (float)  SOC_NDDR_CONTIG);
        tile_llc_footprint = (int) ((float) tile_llc_footprint / (float)  SOC_NDDR_CONTIG);
        aggr_llc_size = SOC_NDDR_CONTIG * cache_llc_bank_size;
    } else {
        tile_non_coh_cnt = status.active_non_coh_split[ddr_n];
        tile_llc_cnt = status.active_to_llc_split[ddr_n]; 
        tile_llc_footprint = status.active_llc_footprint_split[ddr_n];
        aggr_llc_size = cache_llc_bank_size;
    }
    
    full_coh_bin = cnt_to_bin(status.active_acc_cnt_full);
    tile_non_coh_bin = cnt_to_bin(tile_non_coh_cnt);
    tile_llc_bin = cnt_to_bin(tile_llc_cnt);
    tile_llc_footprint_bin = footprint_to_bin(tile_llc_footprint, aggr_llc_size);
    footprint_bin = footprint_to_bin(footprint, aggr_llc_size);
    
    state = full_coh_bin + tile_non_coh_bin * N_FULL_COH_ACC_BINS 
            + tile_llc_bin * (N_FULL_COH_ACC_BINS * N_NON_COH_TO_TILE_BINS) 
            + tile_llc_footprint_bin * (N_FULL_COH_ACC_BINS * N_NON_COH_TO_TILE_BINS * N_TO_TILE_LLC_BINS) 
            + footprint_bin * (N_FULL_COH_ACC_BINS * N_NON_COH_TO_TILE_BINS *  N_TO_TILE_LLC_BINS * N_FOOTPRINT_TILE_BINS);
    return state;
}

static float calculate_reward(unsigned int ddr_accesses, acc_stats_t acc_stats, unsigned long long hw_ns, unsigned int footprint, unsigned int acc_id, enum contig_alloc_policy policy){
    float comm_ratio, normalized_execution_time, time_ratio, ddr_access_ratio, reward, normalized_ddr_accesses, normalized_comm_ratio;
    float mapped_time_ratio, mapped_comm_ratio, mapped_ddr_ratio;
    comm_ratio = (float) acc_stats.acc_mem_lo / (float) acc_stats.acc_tot_lo;
    normalized_execution_time = (float) hw_ns / (float) footprint;
    normalized_ddr_accesses = (float) ddr_accesses / (float) footprint;
    float ddr_diff, ddr_range;

    if (best_times[acc_id] < 0.0){
        time_ratio = 0.5;
        best_times[acc_id] = normalized_execution_time;
        worst_times[acc_id] = normalized_execution_time;
    }
    else { 
        time_ratio = best_times[acc_id] / normalized_execution_time;

        if (normalized_execution_time < best_times[acc_id])
            best_times[acc_id] = normalized_execution_time;
    
        if (normalized_execution_time > worst_times[acc_id])
            worst_times[acc_id] = normalized_execution_time;
    
        }

    if (best_ddr[acc_id] < 0.0){
        ddr_access_ratio = 0.5;
        best_ddr[acc_id] = normalized_ddr_accesses;
        worst_ddr[acc_id] = normalized_ddr_accesses;
    } else{
        if (normalized_ddr_accesses < best_ddr[acc_id])
            best_ddr[acc_id] = normalized_ddr_accesses;
        
        if (normalized_ddr_accesses > worst_ddr[acc_id])
            worst_ddr[acc_id] = normalized_ddr_accesses;
        ddr_diff =  normalized_ddr_accesses - best_ddr[acc_id];
        ddr_range = worst_ddr[acc_id] - best_ddr[acc_id];
        if (ddr_range == 0.0)
            ddr_access_ratio = 1.0;
        else
            ddr_access_ratio = 1.0 - (ddr_diff / ddr_range);
    }

    if (best_ratios[acc_id] < 0.0){
        normalized_comm_ratio = 0.5;
        best_ratios[acc_id] = comm_ratio;
        worst_ratios[acc_id] = comm_ratio;
    } else{
        
        normalized_comm_ratio = best_ratios[acc_id] / comm_ratio; 
        
        if (comm_ratio < best_ratios[acc_id])
            best_ratios[acc_id] = comm_ratio;
    
        if (comm_ratio > worst_ratios[acc_id])
            worst_ratios[acc_id] = comm_ratio;
    
    }
    
    mapped_time_ratio = time_ratio - 0.5;
    mapped_ddr_ratio = ddr_access_ratio - 0.5;
    mapped_comm_ratio = normalized_comm_ratio - 0.5;
     
    reward = 0.75*(0.9 * mapped_time_ratio + 0.1 * mapped_comm_ratio) + 0.25 * mapped_ddr_ratio;
    return reward;
}

static void update_q_table_coh(unsigned int acc_id, unsigned int state, unsigned int action, float reward, float alpha){
    q_values_coh[state][action] = (1.0 - alpha) * q_values_coh[state][action] + alpha * reward;
}

static unsigned int choose_action_coh(unsigned int acc_id, unsigned int state){
    unsigned int best_action;
    float best_val, sample; 
    int a;
    
    sample = (float) rand() / (float) RAND_MAX;
    if (sample < epsilon){
        best_action = rand() % 4;
        return best_action;
    } else {
        best_val = q_values_coh[state][0];
        best_action = 0;
        for (a = 1; a < N_ACTIONS_COH; a++){
            if (q_values_coh[state][a] > best_val){
                best_val = q_values_coh[state][a];
                best_action = a;
            }
        }
        return best_action;
    }
}

void print_values(){
    FILE *q_file_coh = fopen("q_vals_coh.bin", "wb");
    FILE *best_time_file = fopen("best_times.bin", "wb");
    FILE *worst_time_file = fopen("worst_times.bin", "wb");
    FILE *best_ddr_file = fopen("best_ddr.bin", "wb");
    FILE *worst_ddr_file = fopen("worst_ddr.bin", "wb");
    FILE *best_ratio_file = fopen("best_ratios.bin", "wb");
    FILE *worst_ratio_file = fopen("worst_ratios.bin", "wb");
    
    fwrite(q_values_coh, sizeof(float), STATE_SPACE_SIZE_COH * N_ACTIONS_COH, q_file_coh);
    fwrite(best_times, sizeof(float), SOC_NACC, best_time_file);
    fwrite(worst_times, sizeof(float), SOC_NACC, worst_time_file);
    fwrite(best_ddr, sizeof(float), SOC_NACC, best_ddr_file);
    fwrite(worst_ddr, sizeof(float), SOC_NACC, worst_ddr_file);
    fwrite(best_ratios, sizeof(float), SOC_NACC, best_ratio_file);
    fwrite(worst_ratios, sizeof(float), SOC_NACC, worst_ratio_file);
    
    fclose(q_file_coh);
    fclose(best_time_file);
    fclose(worst_time_file);
    fclose(best_ddr_file);
    fclose(worst_ddr_file);
    fclose(best_ratio_file);
    fclose(worst_ratio_file);
}

void read_values(){
    if (access("q_vals_coh.bin", F_OK) != -1){
        FILE *q_file_coh = fopen("q_vals_coh.bin", "rb");
        fread(q_values_coh, sizeof(float), STATE_SPACE_SIZE_COH * N_ACTIONS_COH, q_file_coh);
        fclose(q_file_coh);
    } else {
        memset(q_values_coh, 0, sizeof(float) * STATE_SPACE_SIZE_COH * N_ACTIONS_COH); 
    }

    if (access("best_times.bin", F_OK) != -1){
        FILE *time_file = fopen("best_times.bin", "rb");
        fread(best_times, sizeof(float), SOC_NACC, time_file);
        fclose(time_file);
    } else {
       for (int a = 0; a < SOC_NACC; a++)
           best_times[a] = -1.0;
    }

    if (access("worst_times.bin", F_OK) != -1){
        FILE *time_file = fopen("worst_times.bin", "rb");
        fread(worst_times, sizeof(float), SOC_NACC, time_file);
        fclose(time_file);
    } else {
       for (int a = 0; a < SOC_NACC; a++)
           worst_times[a] = -1.0;
    }
    
    if (access("best_ddr.bin", F_OK) != -1){
        FILE *ddr_file = fopen("best_ddr.bin", "rb");
        fread(best_ddr, sizeof(float), SOC_NACC, ddr_file);
        fclose(ddr_file);
    } else {
       for (int a = 0; a < SOC_NACC; a++){
           best_ddr[a] = -1.0;
       }
    }
 
    if (access("worst_ddr.bin", F_OK) != -1){
        FILE *ddr_file = fopen("worst_ddr.bin", "rb");
        fread(worst_ddr, sizeof(float), SOC_NACC, ddr_file);
        fclose(ddr_file);
    } else {
       for (int a = 0; a < SOC_NACC; a++){
            worst_ddr[a] = -1.0;
       }
    }
   
    if (access("best_ratios.bin", F_OK) != -1){
        FILE *ratio_file = fopen("best_ratios.bin", "rb");
        fread(best_ratios, sizeof(float), SOC_NACC, ratio_file);
        fclose(ratio_file);
    } else {
       for (int a = 0; a < SOC_NACC; a++)
           best_ratios[a] = -1.0;
    }

    if (access("worst_ratios.bin", F_OK) != -1){
        FILE *ratio_file = fopen("worst_ratios.bin", "rb");
        fread(worst_ratios, sizeof(float), SOC_NACC, ratio_file);
        fclose(ratio_file);
    } else {
       for (int a = 0; a < SOC_NACC; a++)
           worst_ratios[a] = -1.0;
    }
}

void set_learning_params(float new_epsilon, float new_alpha, float new_discount){
    epsilon = new_epsilon;
    alpha = new_alpha;
    discount = new_discount;
}
