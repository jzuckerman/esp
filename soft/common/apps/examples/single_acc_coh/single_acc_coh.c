// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include "cfg.h"

static unsigned in_size;
static unsigned out_size;
static unsigned size;

#define NACC 7
#define SORT 0
#define CHOLESKY 1
#define VITDODEC 2
#define SPMV 3
#define FFT 4
#define NIGHTVISION 5
#define MRIQ 6

#define SIZES 3
#define SMALL 0
#define MEDIUM 1
#define LARGE 2

#define TRIALS 10

char* devnames[] = {
"sort_stratus.0",
"cholesky_stratus.0",
"vitdodec_stratus.0",
"spmv_stratus.0",
"fft_stratus.0",
"nightvision_stratus.0",
"mriq_stratus.0",
};

token_t spmv_buf_s[5794];
token_t spmv_buf_m[39224];
token_t spmv_buf_l[1141362];

char* acc_num_to_name(unsigned acc_num){
    switch(acc_num){
        case SORT :
            return "Sort";
        case CHOLESKY :
            return "Cholesky";
        case VITDODEC :
            return "Viterbi";
        case FFT : 
            return "FFT";
        case NIGHTVISION :
            return "Night-vision";
        case SPMV : 
            return "SPMV";
        case MRIQ : 
            return "MRI-Q";
        default :
            return "INVALID";
    }
}

char* size_to_name (unsigned s) {
    switch(s) {
        case SMALL :
            return "Small";
        case MEDIUM :
            return "Medium";
        case LARGE : 
            return "Large";
        default : 
            return "INVALID";
    }
}

/* User-defined code */
static void init_buffer(token_t *in, unsigned acc, unsigned size)
{
    token_t *buf;
    unsigned buf_size, i;

    if (acc == SPMV) {
        switch (size) {
            case SMALL : 
                buf = spmv_buf_s;
                buf_size = 5794;
            break;

            case MEDIUM :
                buf = spmv_buf_m;
                buf_size = 39224;
            break; 

            case LARGE :
                buf = spmv_buf_l;
                buf_size = 1141362;
            break;

            default: 
                return;
        }
        for (i = 0; i < buf_size; i++)
            in[i] = buf[i]; 
    } else {
        for (i = 0; i < in_size / 4; i++)
            in[i] = rand();       
    }
}

/* User-defined code */
static void init_parameters(unsigned acc, unsigned s, unsigned coherence)
{
    int in_place = 0;
    switch(acc){
        case SORT :
            sort_cfg_000[0].esp.coherence = coherence;
            sort_cfg_000[0].esp.devid = SORT;
            if (s == SMALL) {
                sort_cfg_000[0].size = 64;
                sort_cfg_000[0].batch = 64;
            } else if (s == MEDIUM) {
                sort_cfg_000[0].size = 256;
                sort_cfg_000[0].batch = 256;
            } else if (s == LARGE) {
                sort_cfg_000[0].size = 1024;
                sort_cfg_000[0].batch = 1024;
            }
            in_size = sort_cfg_000[0].size * sort_cfg_000[0].batch * 4;
            out_size = in_size;
            in_place = 1;
            cfg_000[0].ioctl_req = SORT_STRATUS_IOC_ACCESS;
            cfg_000[0].esp_desc = &(sort_cfg_000[0].esp);
        break;

        case CHOLESKY :
            cholesky_cfg_000[0].esp.coherence = coherence;
            cholesky_cfg_000[0].esp.devid = CHOLESKY;
            if (s == SMALL) {
                cholesky_cfg_000[0].input_rows = 64;
                cholesky_cfg_000[0].output_rows = 64;
            } else if (s == MEDIUM) {
                cholesky_cfg_000[0].input_rows = 256;
                cholesky_cfg_000[0].output_rows = 256;
            } else if (s == LARGE) {
                cholesky_cfg_000[0].input_rows = 1024;
                cholesky_cfg_000[0].output_rows = 1024;
            }
            in_size = cholesky_cfg_000[0].input_rows *
                      cholesky_cfg_000[0].input_rows * 4;
            out_size = cholesky_cfg_000[0].output_rows *
                      cholesky_cfg_000[0].output_rows * 4;
            in_place = 0;
            cfg_000[0].ioctl_req = CHOLESKY_STRATUS_IOC_ACCESS;
            cfg_000[0].esp_desc = &(cholesky_cfg_000[0].esp);
        break;

        case VITDODEC :
            vitdodec_cfg_000[0].esp.coherence = coherence;
            vitdodec_cfg_000[0].esp.devid = VITDODEC;
            if (s == SMALL) {
                vitdodec_cfg_000[0].nbatches = 1;           
            } else if (s == MEDIUM) {
                vitdodec_cfg_000[0].nbatches = 6;           
            } else if (s == LARGE) {
                vitdodec_cfg_000[0].nbatches = 96; 
            }
            in_size = 24856 * vitdodec_cfg_000[0].nbatches;
            out_size = 18592 * vitdodec_cfg_000[0].nbatches;
            in_place = 0;
            cfg_000[0].ioctl_req = VITDODEC_STRATUS_IOC_ACCESS;
            cfg_000[0].esp_desc = &(vitdodec_cfg_000[0].esp);
        break;

        case FFT : 
            fft_cfg_000[0].esp.coherence = coherence;
            fft_cfg_000[0].esp.devid = FFT;
            if (s == SMALL) {
                fft_cfg_000[0].batch_size = 1; 
            } else if (s == MEDIUM) {
                fft_cfg_000[0].batch_size = 16; 
            } else if (s == LARGE) {
                fft_cfg_000[0].batch_size = 256; 
            }
            in_size = (1 << (fft_cfg_000[0].log_len + 1))
                        * fft_cfg_000[0].batch_size * 4;
            out_size = in_size;
            in_place = 1;
            cfg_000[0].ioctl_req = FFT_STRATUS_IOC_ACCESS;
            cfg_000[0].esp_desc = &(fft_cfg_000[0].esp);
            
        break;
    
        case NIGHTVISION :
            nightvision_cfg_000[0].esp.coherence = coherence;
            nightvision_cfg_000[0].esp.devid = NIGHTVISION;
            if (s == SMALL) {
                nightvision_cfg_000[0].nimages = 4;
            } else if (s == MEDIUM) {
                nightvision_cfg_000[0].nimages = 16;
            } else if (s == LARGE) {
                nightvision_cfg_000[0].nimages = 256;
            }
            in_size = nightvision_cfg_000[0].nimages * 
                      nightvision_cfg_000[0].rows * 
                      nightvision_cfg_000[0].cols * 4;
            out_size = in_size;
            in_place = 1;
            cfg_000[0].ioctl_req = NIGHTVISION_STRATUS_IOC_ACCESS;
            cfg_000[0].esp_desc = &(nightvision_cfg_000[0].esp);

        break;

        case SPMV : 
            spmv_cfg_000[0].esp.coherence = coherence;
            spmv_cfg_000[0].esp.devid = SPMV;
            if (s == SMALL) {
                spmv_cfg_000[0].nrows = 512;
                spmv_cfg_000[0].ncols = 512;
                spmv_cfg_000[0].max_nonzero = 8;
                spmv_cfg_000[0].mtx_len = 2385;
            } else if (s == MEDIUM) {
                spmv_cfg_000[0].nrows = 2048;
                spmv_cfg_000[0].ncols = 2048;
                spmv_cfg_000[0].max_nonzero = 16;
                spmv_cfg_000[0].mtx_len = 17564;
            } else if (s == LARGE) {
                spmv_cfg_000[0].nrows = 32768;
                spmv_cfg_000[0].ncols = 32768;
                spmv_cfg_000[0].max_nonzero = 32;
                spmv_cfg_000[0].mtx_len = 537913;
            }
            in_size = (spmv_cfg_000[0].mtx_len * 2 +
                      spmv_cfg_000[0].nrows +
                      spmv_cfg_000[0].ncols) * 4;
            out_size = spmv_cfg_000[0].nrows * 4;
            in_place = 0;
            cfg_000[0].ioctl_req = SPMV_STRATUS_IOC_ACCESS;
            cfg_000[0].esp_desc = &(spmv_cfg_000[0].esp);

        break;

        case MRIQ : 
            mriq_cfg_000[0].esp.coherence = coherence;
            mriq_cfg_000[0].esp.devid = MRIQ;
            if (s == SMALL) {
                mriq_cfg_000[0].batch_size_x = 128;
                mriq_cfg_000[0].batch_size_k = 128;
                mriq_cfg_000[0].num_batch_x = 4;
                mriq_cfg_000[0].num_batch_k = 4;
            } else if (s == MEDIUM) {
                mriq_cfg_000[0].batch_size_x = 128;
                mriq_cfg_000[0].batch_size_k = 1024;
                mriq_cfg_000[0].num_batch_x = 6;
                mriq_cfg_000[0].num_batch_k = 6;
            } else if (s == LARGE) {
                mriq_cfg_000[0].batch_size_x = 128;
                mriq_cfg_000[0].batch_size_k = 1024;
                mriq_cfg_000[0].num_batch_x = 48;
                mriq_cfg_000[0].num_batch_k = 48;
            }
            in_size = (3 * mriq_cfg_000[0].batch_size_x * mriq_cfg_000[0].num_batch_x +
                       5 * mriq_cfg_000[0].batch_size_k * mriq_cfg_000[0].num_batch_k) * 4; 
            out_size = (2 * mriq_cfg_000[0].batch_size_x * mriq_cfg_000[0].num_batch_x) * 4; 
            in_place = 0;
            cfg_000[0].ioctl_req = MRIQ_STRATUS_IOC_ACCESS;
            cfg_000[0].esp_desc = &(mriq_cfg_000[0].esp);
        break;
    }

    size = in_size;
    if (!in_place)
        size += out_size;
    (cfg_000[0].esp_desc)->footprint = size;
    cfg_000[0].devname = devnames[acc]; 
}

static void read_spmv(uint32_t* buffer, unsigned s)
{
    FILE *fp = NULL;
    char str_tmp[4];
    int i;
    unsigned nrows, ncols, max_nonzero, mtx_len;

    if (s == SMALL) {
        fp = fopen("in1.data", "r");
    } else if(s == MEDIUM) {
        fp = fopen("in2.data", "r");
    } else if(s == LARGE) {
        fp = fopen("in4.data", "r");
    }

    if (!fp)
        die_errno("%s: cannot open SPMV file", __func__);

    // Read configuration
    fscanf(fp, "%u %u %u %u\n", &nrows, &ncols, &max_nonzero, &mtx_len);

    /* printf("SPMV config: %u %u %u %u\n", nrows, ncols, max_nonzero, mtx_len); */

    // Read input data
    // Vals
    fscanf(fp, "%s\n", str_tmp); // Read separator line: %%
    for (i = 0; i < mtx_len; i++) {
        float val;
        fscanf(fp, "%f\n", &val);
        buffer[i] = float_to_fixed32(val, 16);
    }
    // Cols
    fscanf(fp, "%s\n", str_tmp); // Read separator line: %%
    for (; i < mtx_len*2; i++) {
        uint32_t col;
        fscanf(fp, "%u\n", &col);
        buffer[i] = col;
    }
    // Rows
    fscanf(fp, "%s\n", str_tmp); // Read separator line: %%
    fscanf(fp, "%s\n", str_tmp); // Read 0
    for (; i < mtx_len*2 + nrows; i++) {
        uint32_t row;
        fscanf(fp, "%u\n", &row);
        buffer[i] = row;
    }
    // Vect
    fscanf(fp, "%s\n", str_tmp); // Read separator line: %%
    for (; i < mtx_len*2 + nrows + ncols; i++) {
        float vect;
        fscanf(fp, "%f\n", &vect);
        buffer[i] = float_to_fixed32(vect, 16);
    }

    fclose(fp);
}

int main(int argc, char **argv)
{
	token_t *buf;

    //Peformance Results
    unsigned int time_ns[4];
    unsigned int mem_accesses[4];
    
    float norm_execution, norm_mem;
    
    read_spmv(spmv_buf_s, SMALL);       
    read_spmv(spmv_buf_m, MEDIUM);       
    read_spmv(spmv_buf_l, LARGE);       

    //allocate buffers
    FILE* outfile = fopen("motiv_coh.csv", "w");
    fprintf(outfile, "xAxisTopLabel,xAxisGroupLabel,xAxisLabel,xAxisSubLabel,yAxisLabel\n");
    unsigned coherence, acc, s, i;
    for (acc = 0; acc < NACC; acc++){
        for (s = 0; s < SIZES; s++) {        
            for (coherence = ACC_COH_NONE; coherence <= ACC_COH_FULL; coherence++) {
                printf("%d, %d, %d\n", acc, s, coherence);
                
                init_parameters(acc, s, coherence);
                buf = (token_t *) esp_alloc(size);
	            cfg_000[0].hw_buf = buf;
                
                mem_accesses[coherence] = 0;
                time_ns[coherence] = 0;

                printf("\n====== %s ======\n\n", coh_to_str(coherence));
                for (i = 0; i < TRIALS; i++) { 
                    init_buffer(buf, acc, s);
                    //run accelerator
                    printf("  ** START **\n");
                    esp_run(cfg_000, 1);
                    printf("  ** DONE **\n");
                    //record statistics
                    time_ns[coherence] += *cfg_000[0].hw_ns;
                    mem_accesses[coherence] += *cfg_000[0].ddr_accesses;
                }
                time_ns[coherence] /= TRIALS;
                mem_accesses[coherence] /= TRIALS;
                esp_free(buf);
            }
            
            for (coherence = ACC_COH_NONE; coherence <= ACC_COH_FULL; coherence++) {
                norm_execution = (float) time_ns[coherence] / (float) time_ns[0];
                norm_mem = (float) mem_accesses[coherence] / (float) mem_accesses[0];
                fprintf(outfile, "%s,%s,execution time,%s,%f\n", acc_num_to_name(acc), size_to_name(s), coh_to_str(coherence), norm_execution);
                fprintf(outfile, "%s,%s,off-chip memory accesses,%s,%f\n", acc_num_to_name(acc), size_to_name(s), coh_to_str(coherence), norm_mem);
            }
        }
    }
    fclose(outfile); 
	return 0;
}
