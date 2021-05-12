/*
 * Copyright (c) 2011-2021 Columbia University, System Level Design Group
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ESPLIB_H__
#define __ESPLIB_H__
#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "my_stringify.h"
#include "fixed_point.h"
#include "contig.h"
#include "esp.h"
#include "test/time.h"
#include "test/test.h"

#include <esp.h>
#include <esp_accelerator.h>

#define NLOOPS_MAX 4

unsigned DMA_WORD_PER_BEAT(unsigned _st);

struct esp_status_coh {
	unsigned int active_acc_cnt;
	unsigned int active_acc_cnt_full;
	unsigned int active_non_coh_split[N_MEM];
    unsigned int active_llc_coh_split[N_MEM];
    unsigned int active_to_llc_split[N_MEM];
    unsigned int active_full_coh_split[N_MEM];
    unsigned int active_footprint;
    unsigned int active_llc_footprint_split[N_MEM];
	unsigned int active_footprint_split[N_MEM]; // 2 mem ctrl
};

struct esp_status_alloc {
    unsigned int active_threads;
    unsigned int active_allocations_cnt[N_MEM];
    unsigned int active_allocations_split[N_MEM];
};

typedef struct esp_accelerator_thread_info {
	bool run;
	char *devname;
	void *hw_buf;
	int ioctl_req;
	/* Partially Filled-in by ESPLIB */
	struct esp_access *esp_desc;
	/* Filled-in by ESPLIB */
	int fd;
    unsigned long long hw_ns[NLOOPS_MAX];
    acc_stats_t acc_stats[NLOOPS_MAX];
    unsigned int ddr_accesses[NLOOPS_MAX];
    unsigned int ddr_accesses_start[NLOOPS_MAX][N_MEM];
    unsigned int ddr_accesses_end[NLOOPS_MAX][N_MEM];
    struct esp_status_coh esp_status[NLOOPS_MAX];
    unsigned int state[NLOOPS_MAX];
    float reward[NLOOPS_MAX];
    float best_time[NLOOPS_MAX];
    unsigned int *invocation_cnt_ptr;
    float *total_reward_ptr;
} esp_thread_info_t;

typedef struct buf2handle_node {
    void *buf;
    contig_handle_t *handle;
    enum contig_alloc_policy policy;
    unsigned int ddr_node;
    size_t size;
    struct buf2handle_node *next;
} buf2handle_node;

struct thread_args {
    esp_thread_info_t* info; 
    unsigned nacc;
    unsigned loop_cnt;
};

struct p2p_thread_args {
    esp_thread_info_t* info; 
    unsigned loop_iter;
};

void esp_init(void);
void esp_cleanup(void);
void *esp_alloc_policy_accs(struct contig_alloc_params *params, size_t size, int *accs, unsigned int nacc);
void *esp_alloc_policy(struct contig_alloc_params *params, size_t size);
void *esp_alloc(size_t size);
void esp_run_parallel(esp_thread_info_t* cfg[], unsigned nthreads, unsigned* nacc, unsigned* loop_cnt);
void esp_run(esp_thread_info_t cfg[], unsigned nacc);
void esp_free(void *buf);
void set_learning_params(float new_epsilon, float new_alpha, float new_discount);
void read_ddr_accesses(unsigned int *ddr_accesses);
char *coh_to_str(unsigned coherence);
#endif /* __ESPLIB_H__ */
