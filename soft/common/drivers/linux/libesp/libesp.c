/*
 * Copyright (c) 2011-2021 Columbia University, System Level Design Group
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cache_cfg.h"
#include "libesp.h"

buf2handle_node *head = NULL;

struct esp_status_coh esp_status_coh;
pthread_mutex_t esp_status_coh_lock; 

struct esp_status_alloc esp_status_alloc;
pthread_mutex_t esp_status_alloc_lock; 

static size_t cache_l2_size = L2_SETS * L2_WAYS * LINE_BYTES;

#ifdef CACHE_RTL
static size_t cache_llc_bank_size = LLC_SETS * LLC_WAYS * LINE_BYTES / LLC_BANKS;
static size_t cache_llc_size = LLC_SETS * LLC_WAYS * LINE_BYTES;
#else
static size_t cache_llc_bank_size = LLC_SETS * LLC_WAYS * LINE_BYTES;
static size_t cache_llc_size = LLC_SETS * LLC_WAYS * LINE_BYTES *  LLC_BANKS;
#endif

#include "rl.h"


bool thread_is_p2p(esp_thread_info_t *thread)
{
	return ((thread->esp_desc)->p2p_store || (thread->esp_desc)->p2p_nsrcs);
}

unsigned DMA_WORD_PER_BEAT(unsigned _st)
{
	return (sizeof(void *) / _st);
}

static void esp_runtime_alloc_config (struct contig_alloc_params *params, size_t size){
    unsigned int preferred_ddr, lloaded_ddr, threshold, cluster_size, min_allocated;
    int m;

    min_allocated = UINT_MAX;
    for (m = 0; m < SOC_NDDR_CONTIG; m++){
        if (esp_status_alloc.active_allocations_split[m] <= min_allocated){
            min_allocated = esp_status_alloc.active_allocations_split[m];
            lloaded_ddr = m;
        }
    }
   
    if (params->policy == CONTIG_ALLOC_AUTO){ 
        threshold = params->pol.automatic.threshold;
        cluster_size = params->pol.automatic.cluster_size;
        preferred_ddr = params->pol.automatic.ddr_node;


        if (esp_status_alloc.active_allocations_split[preferred_ddr] == 0)
            params->policy = CONTIG_ALLOC_PREFERRED;
        else if (esp_status_alloc.active_allocations_split[lloaded_ddr] == 0)
            params->policy = CONTIG_ALLOC_LEAST_LOADED;
        else if (esp_status_alloc.active_threads > 5)
            params->policy = CONTIG_ALLOC_LEAST_LOADED;
        else if (size > cache_llc_size && esp_status_alloc.active_threads < SOC_NDDR_CONTIG)
            params->policy = CONTIG_ALLOC_BALANCED;
        else if (esp_status_alloc.active_allocations_split[preferred_ddr] < cache_llc_bank_size)
            params->policy = CONTIG_ALLOC_PREFERRED;
        else 
            params->policy = CONTIG_ALLOC_LEAST_LOADED;
                       

        if (params->policy == CONTIG_ALLOC_PREFERRED){
                params->pol.first.ddr_node = preferred_ddr;
        } else if (params->policy == CONTIG_ALLOC_LEAST_LOADED){
                params->pol.lloaded.threshold = threshold;
        } else if (params->policy == CONTIG_ALLOC_BALANCED){
                params->pol.balanced.threshold = threshold;
                params->pol.balanced.cluster_size = cluster_size;
        }
    }
    
    if (params->policy == CONTIG_ALLOC_LEAST_UTILIZED){
        params->policy = CONTIG_ALLOC_PREFERRED;
        params->pol.first.ddr_node = lloaded_ddr;
    }

    return;
}  

static void add_allocation_to_status(struct contig_alloc_params params, size_t size, unsigned int ddr_node){
    int m;
    esp_status_alloc.active_threads += 1;
    
   /* if (params.policy == CONTIG_ALLOC_LEAST_LOADED && ddr_node != min_ddr){
        printf("LEAST LOADED MISMATCH calculated is %d, actual is %d!!\n", min_ddr, ddr_node);
    }*/

    if (params.policy == CONTIG_ALLOC_PREFERRED || params.policy == CONTIG_ALLOC_LEAST_UTILIZED || params.policy == CONTIG_ALLOC_LEAST_LOADED){
        esp_status_alloc.active_allocations_split[ddr_node] += size;
        esp_status_alloc.active_allocations_cnt[ddr_node]++;
    } else {
        for (m = 0; m < SOC_NDDR_CONTIG; m++){
            esp_status_alloc.active_allocations_split[m] += size / SOC_NDDR_CONTIG;
            esp_status_alloc.active_allocations_cnt[m]++;
        }
    }
}

static void esp_update_status_alloc(enum contig_alloc_policy policy, size_t size, unsigned int ddr_node){
    int m;

    esp_status_alloc.active_threads -= 1;
    if (policy ==  CONTIG_ALLOC_PREFERRED || policy  == CONTIG_ALLOC_LEAST_LOADED || policy == CONTIG_ALLOC_LEAST_UTILIZED){
        esp_status_alloc.active_allocations_split[ddr_node] -= size;
        esp_status_alloc.active_allocations_cnt[ddr_node]--;
    } else {
        for (m = 0; m < SOC_NDDR_CONTIG; m++){
            esp_status_alloc.active_allocations_cnt[m]--;
            esp_status_alloc.active_allocations_split[m] -= size / SOC_NDDR_CONTIG;
        }
    }
}

static void esp_runtime_coh_config(struct esp_access *esp, struct esp_status_coh *status, unsigned int *state){
   
    if  (esp->coherence == ACC_COH_AUTO){
        unsigned int total_llc_size = cache_llc_bank_size;
        unsigned int active_to_llc = esp_status_coh.active_to_llc_split[esp->ddr_node];
        unsigned int active_full_coh = esp_status_coh.active_full_coh_split[esp->ddr_node];
        unsigned int active_footprint = esp_status_coh.active_footprint_split[esp->ddr_node];
        unsigned int active_non_coh = 0;
        unsigned int active_llc_coh = 0;
        for (int m = 0; m < SOC_NDDR_CONTIG; m++){
            active_non_coh += esp_status_coh.active_non_coh_split[m];
            active_llc_coh += esp_status_coh.active_llc_coh_split[m];
        }

        if (esp->alloc_policy == CONTIG_ALLOC_BALANCED){
            total_llc_size *= SOC_NDDR_CONTIG;
            for (int m = 0; m < SOC_NDDR_CONTIG; m++){
                if (m != esp->ddr_node){
                    active_to_llc += esp_status_coh.active_to_llc_split[m];
                    active_full_coh += esp_status_coh.active_full_coh_split[m];
                    active_footprint += esp_status_coh.active_footprint_split[m];
                }
            }
            active_to_llc /= SOC_NDDR_CONTIG;
            active_full_coh /= SOC_NDDR_CONTIG;
        }
        // Cache coherence choice
        if (esp->footprint <= 8192){
            esp->coherence = ACC_COH_FULL;
        }
        else if (esp->footprint <= cache_l2_size) {
            if (active_to_llc - active_full_coh > active_full_coh) 
               esp->coherence = ACC_COH_FULL;
           else
               esp->coherence = ACC_COH_RECALL;
        } else if (esp->footprint + active_footprint > total_llc_size) {
            esp->coherence = ACC_COH_NONE;
        } else {
            if (active_non_coh >= 2)
                esp->coherence = ACC_COH_LLC;
            else 
                esp->coherence = ACC_COH_RECALL;
        }
    } 
    
    *state = calculate_state_coh(esp_status_coh, esp->alloc_policy, esp->footprint, esp->ddr_node);
    
    if (esp->learn){
         esp->coherence = choose_action_coh(esp->devid, *state);
    }

#ifdef ACCS_PRESENT
    if (!acc_has_l2[esp->devid] && esp->coherence == ACC_COH_FULL)
        esp->coherence  = ACC_COH_RECALL;
#endif

    *status = esp_status_coh;

	esp_status_coh.active_acc_cnt += 1;
    esp_status_coh.active_footprint += esp->footprint;
    
	if (esp->coherence == ACC_COH_FULL) {
		esp_status_coh.active_acc_cnt_full += 1;
	    esp_status_coh.active_full_coh_split[esp->ddr_node] += 1;
    }
    
    if (esp->coherence == ACC_COH_LLC){
        esp_status_coh.active_llc_coh_split[esp->ddr_node] += 1;
    } 
    
    if (esp->alloc_policy == CONTIG_ALLOC_PREFERRED ||
        esp->alloc_policy == CONTIG_ALLOC_LEAST_LOADED){
        
        esp_status_coh.active_footprint_split[esp->ddr_node] += esp->footprint;

        if (esp->coherence == ACC_COH_NONE){
            esp_status_coh.active_non_coh_split[esp->ddr_node] += 1;
        } else{
            esp_status_coh.active_to_llc_split[esp->ddr_node] += 1;
            esp_status_coh.active_llc_footprint_split[esp->ddr_node] += esp->footprint;
        }
    } else {
        //BALANCED, assume equal split 
        int i;
        for (i = 0; i < SOC_NDDR_CONTIG; i++){
            esp_status_coh.active_footprint_split[i] += esp->footprint / SOC_NDDR_CONTIG;

            if (esp->coherence == ACC_COH_NONE){
                esp_status_coh.active_non_coh_split[i] += 1;
            } else{
                esp_status_coh.active_to_llc_split[i] += 1;
                esp_status_coh.active_llc_footprint_split[i] += esp->footprint / SOC_NDDR_CONTIG;
            }
        }
    }
}

static void esp_update_status_coh(int loop_iter, esp_thread_info_t *info, struct esp_access *esp, unsigned int state, unsigned int *ddr_accesses_start, unsigned int *ddr_accesses_end, acc_stats_t acc_stats, unsigned long long hw_ns, unsigned int *ddr_access_return, float *reward, float *best_time)
{
	unsigned int ddr_access_adjusted, ddr_accesses;
    
    for (int m = 0; m < SOC_NDDR_CONTIG; m++){
        info->ddr_accesses_start[loop_iter][m] = ddr_accesses_start[m]; 
        info->ddr_accesses_end[loop_iter][m] = ddr_accesses_end[m]; 
    }

    if (esp->alloc_policy == CONTIG_ALLOC_BALANCED){
        ddr_access_adjusted = 0;
        for (int m = 0; m < SOC_NDDR_CONTIG; m++){
            if (ddr_accesses_start[m] <= ddr_accesses_end[m])
                ddr_accesses = ddr_accesses_end[m] - ddr_accesses_start[m];
            else 
                ddr_accesses = (0xFFFFFFFF - ddr_accesses_start[m]) + ddr_accesses_end[m]; 
            ddr_access_adjusted += (int) (((long long) ddr_accesses * (long long) esp->footprint) / 
                                    ((long long) SOC_NDDR_CONTIG * (long long) esp_status_coh.active_footprint_split[m])); 
        }
    } else {
        if (ddr_accesses_start[esp->ddr_node] <= ddr_accesses_end[esp->ddr_node])
            ddr_accesses = ddr_accesses_end[esp->ddr_node] - ddr_accesses_start[esp->ddr_node];
        else
            ddr_accesses = (0xFFFFFFFF - ddr_accesses_start[esp->ddr_node]) + ddr_accesses_end[esp->ddr_node];
            
        ddr_access_adjusted =(int) (((long long) ddr_accesses * (long long) esp->footprint) / 
                                        esp_status_coh.active_footprint_split[esp->ddr_node]); 
    }
    *ddr_access_return = ddr_access_adjusted;
    *best_time = best_times[esp->devid];

    if (esp->learn && alpha > 0.001){
       *reward = calculate_reward(ddr_access_adjusted, acc_stats, hw_ns, esp->footprint, esp->devid, esp->alloc_policy);
       update_q_table_coh(esp->devid, state, esp->coherence, *reward, alpha);
    }
   
    esp_status_coh.active_acc_cnt -= 1;
    esp_status_coh.active_footprint -= esp->footprint;
    
	if (esp->coherence == ACC_COH_FULL) {
		esp_status_coh.active_acc_cnt_full -= 1;
	    esp_status_coh.active_full_coh_split[esp->ddr_node] -= 1;
	}

    if (esp->coherence == ACC_COH_LLC){
        esp_status_coh.active_llc_coh_split[esp->ddr_node] -= 1;
    }

    if (esp->alloc_policy == CONTIG_ALLOC_PREFERRED ||
        esp->alloc_policy == CONTIG_ALLOC_LEAST_LOADED){
        
        esp_status_coh.active_footprint_split[esp->ddr_node] -= esp->footprint;

        if (esp->coherence == ACC_COH_NONE){
            esp_status_coh.active_non_coh_split[esp->ddr_node] -= 1;
        } else{
            esp_status_coh.active_to_llc_split[esp->ddr_node] -= 1;
            esp_status_coh.active_llc_footprint_split[esp->ddr_node] -= esp->footprint;
        }
    } else {
        //BALANCED, assume equal split 
        int i;
        for (i = 0; i < SOC_NDDR_CONTIG; i++){
            esp_status_coh.active_footprint_split[i] -= esp->footprint / SOC_NDDR_CONTIG;

            if (esp->coherence == ACC_COH_NONE){
                esp_status_coh.active_non_coh_split[i] -= 1;
            } else{
                esp_status_coh.active_to_llc_split[i] -= 1;
                esp_status_coh.active_llc_footprint_split[i] -= esp->footprint / SOC_NDDR_CONTIG;
            }
        }
    }
}
void *accelerator_thread( void *ptr )
{
	struct p2p_thread_args *p2p_args = (struct p2p_thread_args *) ptr;
    esp_thread_info_t *info = p2p_args->info;
	unsigned l = p2p_args->loop_iter;
	struct timespec th_start;
	struct timespec th_end;
	int rc = 0;

    pthread_mutex_lock(&esp_status_coh_lock);
    esp_runtime_coh_config(info->esp_desc, &info->esp_status[l], &info->state[l]);
    pthread_mutex_unlock(&esp_status_coh_lock);

	gettime(&th_start);
	rc = ioctl(info->fd, info->ioctl_req, info->esp_desc);
	gettime(&th_end);
	
    if (rc < 0) {
		perror("ioctl");
	}

	info->hw_ns[l] = ts_subtract(&th_start, &th_end);
    info->acc_stats[l] = (info->esp_desc)->acc_stats;

    pthread_mutex_lock(&esp_status_coh_lock);
    esp_update_status_coh(l, info, info->esp_desc, info->state[l], (info->esp_desc)->ddr_accesses_start, (info->esp_desc)->ddr_accesses_end, info->acc_stats[l], info->hw_ns[l], &info->ddr_accesses[l], &info->reward[l], &info->best_time[l]);
    pthread_mutex_unlock(&esp_status_coh_lock);
     
	return NULL;
}

void *accelerator_thread_p2p(void *ptr)
{
	struct thread_args *args = (struct thread_args*) ptr;
	esp_thread_info_t *thread = args->info;
    unsigned loop_cnt = args->loop_cnt;
	unsigned nacc = args->nacc;
	int rc = 0;
	int i, l;

	pthread_t *threads = malloc(nacc * sizeof(pthread_t));
    
    for (l = 0; l < loop_cnt; l++){
        for (i = 0; i < nacc; i++) {
            esp_thread_info_t *info = thread + i;
            if (!info->run)
                continue;
            struct p2p_thread_args *p2p_args = malloc(sizeof(struct p2p_thread_args));
            p2p_args->info = info;
            p2p_args->loop_iter = l;
            rc = pthread_create(&threads[i], NULL, accelerator_thread, (void*) p2p_args);
            if (rc != 0)
                perror("pthread_create");
        }

        for (i = 0; i < nacc; i++) {
            esp_thread_info_t *info = thread + i;
            if (!info->run)
                continue;
            rc = pthread_join(threads[i], NULL);
            if (rc != 0)
                perror("pthread_join");
        }
    }
    for (i = 0; i < nacc; i++){
        esp_thread_info_t *info = thread + i;
        if (!info->run)
            continue;
        close(info->fd);
    }

    free(threads);
	free(ptr);
	return NULL;
}

void *accelerator_thread_serial(void *ptr)
{
    struct thread_args *args = (struct thread_args*) ptr;
	esp_thread_info_t *thread = args->info;
    unsigned nacc = args->nacc;
    unsigned loop_cnt = args->loop_cnt;
	
    int i, l;
	for (l = 0; l < loop_cnt; l++){
        for (i = 0; i < nacc; i++) {

            struct timespec th_start;
            struct timespec th_end;
            int rc = 0;
            esp_thread_info_t *info = thread + i;
            if (!info->run)
                continue;

            pthread_mutex_lock(&esp_status_coh_lock);
            esp_runtime_coh_config(info->esp_desc, &info->esp_status[l], &info->state[l]);
            pthread_mutex_unlock(&esp_status_coh_lock);

            gettime(&th_start);
            rc = ioctl(info->fd, info->ioctl_req, info->esp_desc);
            gettime(&th_end);
            if (rc < 0) {
                perror("ioctl");
            }

            info->hw_ns[l] = ts_subtract(&th_start, &th_end);
            info->acc_stats[l] = (info->esp_desc)->acc_stats;

            pthread_mutex_lock(&esp_status_coh_lock);
            esp_update_status_coh(l, info, info->esp_desc, info->state[l], (info->esp_desc)->ddr_accesses_start, (info->esp_desc)->ddr_accesses_end, info->acc_stats[l], info->hw_ns[l], &info->ddr_accesses[l], &info->reward[l], &info->best_time[l]);
            pthread_mutex_unlock(&esp_status_coh_lock);
        }
    }
	
    for (i = 0; i < nacc; i++){
        esp_thread_info_t *info = thread + i; 
            
        if (!info->run)
            continue;
            
        close(info->fd);
    }
    
    free(ptr);
	return NULL;
}

void insert_buf(void *buf, contig_handle_t *handle, enum contig_alloc_policy policy, size_t size, unsigned int ddr_node)
{
	buf2handle_node *new = malloc(sizeof(buf2handle_node));
	new->buf = buf;
	new->handle = handle;
	new->policy = policy;
    new->ddr_node = ddr_node;
    new->size = size;

	new->next = head;
	head = new;
}

contig_handle_t* lookup_handle(void *buf, enum contig_alloc_policy *policy)
{
	buf2handle_node *cur = head;
	while (cur != NULL) {
		if (cur->buf == buf) {
			if (policy != NULL)
				*policy = cur->policy;
			return cur->handle;
		}
		cur = cur->next;
	}
	die("buf not in active allocations\n");
}

void remove_buf(void *buf)
{
	buf2handle_node *cur = head;
	if (cur->buf == buf) {
        esp_update_status_alloc(cur->policy, cur->size, cur->ddr_node);
		contig_free(*(cur->handle));
		head = cur->next;
		free(cur);
		return;
	}

	buf2handle_node *prev;
	while (cur != NULL && cur->buf != buf) {
		prev = cur;
		cur = cur->next;
	}

	if (cur == NULL)
		die("buf not in active allocations\n");
    
    esp_update_status_alloc(cur->policy, cur->size, cur->ddr_node);
	contig_free(*(cur->handle));

	prev->next = cur->next;
	free(cur->handle);
	free(cur);
}

void *esp_alloc_policy_accs(struct contig_alloc_params *params, size_t size, int *accs, unsigned int nacc)
{
	void *contig_ptr;
    unsigned int ddr_node;
    int ddr_node_cost[SOC_NDDR_CONTIG];
    int preferred_node_cost, preferred_node, m, d, devid;
    
    if (params->policy == CONTIG_ALLOC_PREFERRED || params->policy == CONTIG_ALLOC_AUTO) {
        for (m = 0; m < SOC_NDDR_CONTIG; m++){
            ddr_node_cost[m] = 0;
        }

        for (d = 0; d < nacc; d++){
            for (m = 0; m < SOC_NDDR_CONTIG; m++){
                devid = accs[d];       
                ddr_node_cost[m] += abs(acc_locs[devid].row - contig_alloc_locs[m].row) + abs(acc_locs[devid].col - contig_alloc_locs[m].col);
            }
        }

        preferred_node_cost = ddr_node_cost[0];
        preferred_node = 0;
        for (m = 1; m < SOC_NDDR_CONTIG; m++){
            if (ddr_node_cost[m] < preferred_node_cost){
                preferred_node_cost = ddr_node_cost[m];
                preferred_node = m;
            }
        }
        if (params->policy == CONTIG_ALLOC_PREFERRED)
            params->pol.first.ddr_node = preferred_node;
        else
            params->pol.automatic.ddr_node = preferred_node;
    }
    
    esp_runtime_alloc_config(params, size);
    
    contig_handle_t *handle = malloc(sizeof(contig_handle_t));
    contig_ptr = contig_alloc_policy(*params, size, handle);
	ddr_node = contig_to_most_allocated(*handle);
    add_allocation_to_status(*params, size, ddr_node);

    insert_buf(contig_ptr, handle, params->policy, size, ddr_node);
	return contig_ptr;
}

void *esp_alloc_policy(struct contig_alloc_params *params, size_t size)
{
	unsigned int ddr_node;
    esp_runtime_alloc_config(params, size);
    void *contig_ptr;

    contig_handle_t *handle = malloc(sizeof(contig_handle_t));
    contig_ptr = contig_alloc_policy(*params, size, handle);
	ddr_node = contig_to_most_allocated(*handle);
    add_allocation_to_status(*params, size, ddr_node);

    insert_buf(contig_ptr, handle, params->policy, size, ddr_node);
	return contig_ptr;
}

void *esp_alloc(size_t size)
{
    unsigned int ddr_node;
	struct contig_alloc_params params;
    void *contig_ptr;

    params.policy = CONTIG_ALLOC_PREFERRED;
    params.pol.first.ddr_node = 0;
    
    esp_runtime_alloc_config(&params, size);

    contig_handle_t *handle = malloc(sizeof(contig_handle_t));
	contig_ptr = contig_alloc(size, handle);
	ddr_node = contig_to_most_allocated(*handle);
    add_allocation_to_status(params, size, ddr_node);
    
    insert_buf(contig_ptr, handle, CONTIG_ALLOC_PREFERRED, size, ddr_node);
	return contig_ptr;
}

static void esp_config(esp_thread_info_t* cfg[], unsigned nthreads, unsigned *nacc)
{
	int i, j;
	for (i = 0; i < nthreads; i++) {
		unsigned len = nacc[i];
		for(j = 0; j < len; j++) {
			esp_thread_info_t *info = cfg[i] + j;
			if (!info->run)
				continue;

			enum contig_alloc_policy policy;
			contig_handle_t *handle = lookup_handle(info->hw_buf, &policy);

			(info->esp_desc)->contig = contig_to_khandle(*handle);
			(info->esp_desc)->ddr_node = contig_to_most_allocated(*handle);
			(info->esp_desc)->alloc_policy = policy;
			(info->esp_desc)->run = true;
		}
	}
}

static void print_time_info(esp_thread_info_t *info[], unsigned long long hw_ns, int nthreads, unsigned* nacc, unsigned* loop_cnt)
{
	int i, j, l;

	printf("  > Test time: %llu ns\n", hw_ns);
	for (i = 0; i < nthreads; i++) {
		unsigned loop = loop_cnt[i];
        unsigned len = nacc[i];
		for (l = 0; l < loop; l++){
            for (j = 0; j < len; j++) {
                esp_thread_info_t* cur = info[i] + j;
                if (cur->run)
                    printf("    - %s-%d time: %llu ns\n", cur->devname, l, cur->hw_ns[l]);
            }
        }
	}
}

void esp_run(esp_thread_info_t cfg[], unsigned nacc)
{
	int i;

	if (thread_is_p2p(&cfg[0])) {
		esp_thread_info_t *cfg_ptrs[1];
		cfg_ptrs[0] = cfg;
        unsigned loop_cnt = 1;

		esp_run_parallel(cfg_ptrs, 1, &nacc, &loop_cnt);
	} else{
		esp_thread_info_t **cfg_ptrs = malloc(sizeof(esp_thread_info_t*) * nacc);
		unsigned *nacc_arr = malloc(sizeof(unsigned) * nacc);
        unsigned *loop_cnt = malloc(sizeof(unsigned) * nacc);      
		
        for (i = 0; i < nacc; i++) {
			nacc_arr[i] = 1;
            loop_cnt[i] = 1;
			cfg_ptrs[i] = &cfg[i];
		}
		esp_run_parallel(cfg_ptrs, nacc, nacc_arr, loop_cnt);
		free(nacc_arr);
		free(cfg_ptrs);
	}
}

void esp_run_parallel(esp_thread_info_t* cfg[], unsigned nthreads, unsigned* nacc, unsigned* loop_cnt)
{
	int i, j;
	struct timespec th_start;
	struct timespec th_end;
	pthread_t *thread = malloc(nthreads * sizeof(pthread_t));
	int rc = 0;
	esp_config(cfg, nthreads, nacc);
    for (i = 0; i < nthreads; i++) {
		unsigned len = nacc[i];
		for (j = 0; j < len; j++) {
			esp_thread_info_t *info = cfg[i] + j;
			const char *prefix = "/dev/";
			char path[70];

			if (strlen(info->devname) > 64) {
				contig_handle_t *handle = lookup_handle(info->hw_buf, NULL);
				contig_free(*handle);
				die("Error: device name %s exceeds maximum length of 64 characters\n",
				    info->devname);
			}

			sprintf(path, "%s%s", prefix, info->devname);

			info->fd = open(path, O_RDWR, 0);
			if (info->fd < 0) {
				contig_handle_t *handle = lookup_handle(info->hw_buf, NULL);
				contig_free(*handle);
				die_errno("fopen failed\n");
			}
		}
	}

	gettime(&th_start);
	for (i = 0; i < nthreads; i++) {
		struct thread_args *args = malloc(sizeof(struct thread_args));;
		args->info = cfg[i];
		args->nacc = nacc[i];
        args->loop_cnt = loop_cnt[i];
        
		if (thread_is_p2p(cfg[i])){
            if (nthreads == 1)
                accelerator_thread_p2p( (void*) args);
            else
                rc = pthread_create(&thread[i], NULL, accelerator_thread_p2p, (void*) args);

        }   else  {
            if (nthreads == 1)
                accelerator_thread_serial( (void*) args);
            else
                rc = pthread_create(&thread[i], NULL, accelerator_thread_serial, (void*) args);
        }
		
        if(rc != 0) {
			perror("pthread_create");
		}
	}
    
    if (nthreads > 1) {
        for (i = 0; i < nthreads; i++) {
            rc = pthread_join(thread[i], NULL);

            if(rc != 0) {
                perror("pthread_join");
            }
        }
    }

	gettime(&th_end);
	print_time_info(cfg, ts_subtract(&th_start, &th_end), nthreads, nacc, loop_cnt);
    
	free(thread);
}


void esp_free(void *buf)
{
	remove_buf(buf);
}

void *monitor_base_ptr = NULL;

void mmap_monitors(){
    int fd = open("/dev/mem", O_RDWR);
    monitor_base_ptr = mmap(NULL, SOC_ROWS * SOC_COLS * 0x200, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x80090000);
    close(fd);
}

void munmap_monitors(){
    munmap(monitor_base_ptr, SOC_ROWS * SOC_COLS * 0x200);
}

void read_ddr_accesses(unsigned int *ddr_accesses){
    soc_loc_t loc;
    unsigned int tile_no, offset;
    unsigned int *mem_addr;
    int m ;
    for (m = 0; m < SOC_NDDR_CONTIG; m++){
        loc = contig_alloc_locs[m];
        tile_no = loc.row * SOC_COLS + loc.col;
        offset = tile_no * (0x200 / sizeof(unsigned int)); 
        mem_addr = ((unsigned int*) monitor_base_ptr) + offset + 4;
        *(ddr_accesses + m) = *mem_addr;
    }
}

void esp_init(void) {

	int i;
    pthread_mutex_init(&esp_status_coh_lock, NULL);
    
    esp_status_coh.active_acc_cnt = 0;
	esp_status_coh.active_acc_cnt_full = 0;
    esp_status_coh.active_footprint = 0;
	esp_status_alloc.active_threads = 0;
    for (i = 0; i < LLC_BANKS; i++) {
        esp_status_coh.active_non_coh_split[i] = 0;
        esp_status_coh.active_to_llc_split[i] = 0;
        esp_status_coh.active_llc_coh_split[i] = 0;
        esp_status_coh.active_full_coh_split[i] = 0;
        esp_status_coh.active_llc_footprint_split[i] = 0;
        esp_status_coh.active_footprint_split[i] = 0;
        esp_status_alloc.active_allocations_split[i] = 0;
        esp_status_alloc.active_allocations_cnt[i] = 0;
    }
    read_values();
    mmap_monitors();
}

void esp_cleanup()
{
    print_values();
    munmap_monitors();
}

char* coh_to_str(unsigned coherence)
{
    if (coherence == ACC_COH_NONE)
        return "non-coh-DMA";
    else if (coherence == ACC_COH_LLC)
        return "llc-coh-DMA";
    else if (coherence == ACC_COH_RECALL)
        return "coh-DMA";
    else
        return "full-coh";
}
