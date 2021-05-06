// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include "libesp.h"
#include "cfg.h"
#include <fixed_point.h>

static unsigned in_words_adj;
static unsigned out_words_adj;
static unsigned in_len;
static unsigned out_len;
static unsigned in_size;
static unsigned out_size;
static unsigned out_offset;
static unsigned size;

#define fx2float fixed32_to_float
#define float2fx float_to_fixed32
#define FX_IL 12

/* User-defined code */
static int validate_buffer(token_t *out, token_t *gold)
{
	int i;
	int j;
	unsigned errors = 0;
    const float ERR_TH = 0.2f;
    float out_fl, gold_fl;
    for (i = 0; i < 1; i++)
        for (j = 0; j < output_rows * output_rows; j++) {
            out_fl = fx2float(out[j], FX_IL);
            gold_fl = fx2float(gold[j], FX_IL);
            if ((fabs(gold_fl - out_fl) / fabs(gold_fl)) > ERR_TH) {
                printf(" GOLD = %f   and  OUT = %f  and J = %d \n", gold_fl, out_fl, j);
                errors++;
            }
        }

	return errors;
}

/* User-defined code */
static void init_parameters()
{
	if (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) {
		in_words_adj = input_rows * input_rows;
		out_words_adj = output_rows * output_rows;
	} else {
		in_words_adj = round_up(input_rows * input_rows, DMA_WORD_PER_BEAT(sizeof(token_t)));
		out_words_adj = round_up(output_rows * output_rows, DMA_WORD_PER_BEAT(sizeof(token_t)));
	}
	in_len = in_words_adj * (1);
	out_len =  out_words_adj * (1);
	in_size = in_len * sizeof(token_t);
	out_size = out_len * sizeof(token_t);
	out_offset = in_len;
	size = (out_offset * sizeof(token_t)) + out_size;
}

int main(int argc, char **argv)
{
	int errors;

    if (argc >= 2){
        if (!strcmp(argv[1], "non-coh"))
            cholesky_cfg_000[0].esp.coherence = ACC_COH_NONE;
        else if (!strcmp(argv[1], "llc-coh"))
            cholesky_cfg_000[0].esp.coherence = ACC_COH_LLC;
        else if (!strcmp(argv[1], "coh-dma"))
            cholesky_cfg_000[0].esp.coherence = ACC_COH_RECALL;
        else if (!strcmp(argv[1], "full-coh"))
            cholesky_cfg_000[0].esp.coherence = ACC_COH_FULL;
        else {
            printf("usage: ./cholesky_stratus.exe [coh] [rows]\n");
            printf("coh - non-coh | llc-coh | coh-dma | full-coh\n");
            printf("if rows is supplied, random data is used, with no validation\n");
            return 1;
        }
    }

    if (argc >= 3) {
        int rows = atoi(argv[2]);
        if (rows <= 0) {
            printf("usage: ./cholesky_stratus.exe [coh] [rows]\n");
            printf("coh - non-coh | llc-coh | coh-dma | full-coh\n");
            printf("if rows is supplied, random data is used, with no validation\n");
            return 1;
        }
        input_rows = rows;
        output_rows = rows;
        cholesky_cfg_000[0].input_rows = rows;
        cholesky_cfg_000[0].output_rows = rows;
    }
	token_t *gold;
	token_t *buf;

	init_parameters();

    cholesky_cfg_000[0].esp.footprint = size;
    cholesky_cfg_000[0].esp.alloc_policy = CONTIG_ALLOC_PREFERRED;

	buf = (token_t *) esp_alloc(size);
    cfg_000[0].hw_buf = buf;

    gold = malloc(out_size);

    #include "data.h"
    if (argc >= 3) {
        for (int i = 0; i < cholesky_cfg_000[0].input_rows * cholesky_cfg_000[0].input_rows; i++)
            buf[i] = rand();
    }
    printf("\n====== %s ======\n\n", cfg_000[0].devname);

    /* <<--print-params-->> */
	printf("  .input_rows = %d\n", input_rows);
	printf("  .output_rows = %d\n", output_rows);
	printf("\n  ** START **\n");

	esp_run(cfg_000, NACC);

	printf("\n  ** DONE **\n");
    if (argc < 3)
	    errors = validate_buffer(&buf[out_offset], gold);
    else
        errors = 0;

	free(gold);
	esp_free(buf);

	if (!errors)
		printf("+ Test PASSED\n");
	else
		printf("+ Test FAILED with %d errors\n", errors);

	printf("\n====== %s ======\n\n", cfg_000[0].devname);

	return errors;
}
