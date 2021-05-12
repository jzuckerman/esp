// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include "cfg.h"
#include "utils/fft_utils.h"

static unsigned in_words_adj;
static unsigned out_words_adj;
static unsigned in_len;
static unsigned out_len;
static unsigned in_size;
static unsigned out_size;
static unsigned out_offset;
static unsigned size;

const float ERR_TH = 0.05;

/* User-defined code */
static int validate_buffer(token_t *out, float *gold)
{
	int j;
	unsigned errors = 0;

	for (j = 0; j < 2 * len * num_batches; j++) {
		native_t val = fx2float(out[j], FX_IL);
		if ((fabs(gold[j] - val) / fabs(gold[j])) > ERR_TH) {
			errors++;
        }
	}

//	printf("  + Relative error > %.02f for %d output values out of %d\n", ERR_TH, errors, 2 * len * num_batches);

	return errors;
}


/* User-defined code */
static void init_buffer(token_t *in, float *gold)
{
	int j, b;
	const float LO = -1.0;
	const float HI = 1.0;

	srand((unsigned int) time(NULL));

	for (j = 0; j < 2 * len * num_batches; j++) {
		float scaling_factor = (float) rand () / (float) RAND_MAX;
		gold[j] = LO + scaling_factor * (HI - LO);
	}
	// preprocess with bitreverse (fast in software anyway)
	if (!do_bitrev)
   	    fft_bit_reverse(gold, len, log_len);

	// convert input to fixed point
	for (j = 0; j < in_len; j++)
		in[j] = float2fx((native_t) gold[j], FX_IL);

	// Compute golden output
	for (b = 0; b < num_batches; b++)
        fft_comp(&gold[b*2*len], len, log_len,  -1,  do_bitrev);
}


/* User-defined code */
static void init_parameters()
{
	if (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) {
		in_words_adj = 2 * len * num_batches;
		out_words_adj = 2 * len * num_batches;
	} else {
		in_words_adj = round_up(2 * len * num_batches, DMA_WORD_PER_BEAT(sizeof(token_t)));
		out_words_adj = round_up(2 * len * num_batches, DMA_WORD_PER_BEAT(sizeof(token_t)));
	}
	in_len = in_words_adj;
	out_len =  out_words_adj;
	in_size = in_len * sizeof(token_t); 
	out_size = out_len * sizeof(token_t);
	out_offset = in_len;
	size = (out_offset * sizeof(token_t)) + out_size;
}

int main(int argc, char **argv)
{
	int errors;

	float *gold;
	token_t *buf;

    const float ERROR_COUNT_TH = 0.005;
     
    //Peformance Results
    unsigned long long time_ns[4];
    acc_stats_t acc_stats[4];
    unsigned int mem_accesses[4];

    if (argc != 2) {
        printf("usage: ./fft_stratus.exe num_batch\n");
        return -1;
    }
    
    //set parameters
    num_batches = atoi(argv[1]);
    init_parameters();
    fft_cfg_000[0].batch_size = num_batches;
    fft_cfg_000[0].dst_offset = in_size;
    
    //allocate buffers
    buf = (token_t *) esp_alloc(size);
	cfg_000[0].hw_buf = buf;
	gold = malloc(out_len * sizeof(float) * num_batches);
        
    printf("  .len = %d\n", len);
    printf("  .batch_size = %d\n", fft_cfg_000[0].batch_size);

    unsigned coherence;
    for (coherence = ACC_COH_NONE; coherence <= ACC_COH_FULL; coherence++) {
        
        init_buffer(buf, gold);
        
        //set coherence mode
        fft_cfg_000[0].esp.coherence = coherence;
        printf("\n====== %s ======\n\n", coh_to_str(coherence));
        
        //run accelerator
        printf("  ** START **\n");
        esp_run(cfg_000, NACC);
        printf("  ** DONE **\n");
        
        //validate output
        errors = validate_buffer(&buf[out_offset], gold);
        float err_rate = (float) errors / (float) (2 * len * num_batches);

        if (err_rate > ERROR_COUNT_TH)
		    printf("\n  + TEST FAIL: exceeding error count threshold\n");
        else
		    printf("\n  + TEST PASS: not exceeding error count threshold\n");
    
        //record statistics
        acc_stats[coherence] = *cfg_000[0].acc_stats;
        time_ns[coherence] = *cfg_000[0].hw_ns;
        mem_accesses[coherence] = *cfg_000[0].ddr_accesses;
    }

    //cleanup
	free(gold);
	esp_free(buf);
    
    //Report Statistics
	printf("\n====== STATISTICS ======\n\n");
    printf("coherence\t\ttime(ns)\t\tmem-accesses\t\tacc-cycles\n");
    printf("-------------------------------------------------------------------------------------\n");
    for (coherence = ACC_COH_NONE; coherence <= ACC_COH_FULL; coherence++) {
        printf("%s\t\t%llu \t\t%d\t\t\t%d\n", coh_to_str(coherence), time_ns[coherence], mem_accesses[coherence], 
                                           acc_stats[coherence].acc_tot_lo);        
    }

	return errors;
}
