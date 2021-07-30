#include "libesp.h"
#include "string.h"
// include headers for accelerators used by this app
#include "cfg.h"
#include "socmap.h"

#define DEBUG 1
#define dprintf if(DEBUG) printf

#define NPHASES_MAX 100
#define NTHREADS_MAX 16
#define NDEV_MAX 16
#define IRREGULAR_SEED_MAX 2048

#define NACC 8
#define GEMM0 0
#define FFT0 1
#define CONV2D0 2
#define GEMM1 3
#define CONV2D1 4
#define VITDODEC0 5
#define FFT1 6
#define VITDODEC1 7

#define CFG 0
#define FIXED 1
#define AUTO 2
#define RAND 3
#define FIRST 4
#define DESIGN 5

#define SIZES 5
#define EXTRA_SMALL 0
#define SMALL 1
#define MEDIUM 2
#define LARGE 3
#define EXTRA_LARGE 4


unsigned long long total_alloc = 0;
int32_t spmv_buf_s[5794], spmv_buf_m[39224], spmv_buf_l[1141362];

typedef struct accelerator_thread_info {
	int tid; 
	int ndev; 
	int chain[NDEV_MAX];
	int p2p;
	int loop_cnt;
	int workload_size;
    size_t memsz;
	size_t memsz_in;
	enum contig_alloc_policy alloc_choice; 
	struct timespec th_start;
	struct timespec th_end; 
	unsigned long long delay;
	unsigned long long thread_time;
	struct esp_status_alloc alloc_status;
} accelerator_thread_info_t;

char* devnames[] = {
    "gemm_stratus.0",
    "fft_stratus.0",
    "conv2d_stratus.0",
    "gemm_stratus.1",
    "conv2d_stratus.1",
    "vitdodec_stratus.0",
    "fft_stratus.1",
    "vitdodec_stratus.1",
};

char* accnames[] = {
    "gemm",
    "fft",
    "conv2d",
    "gemm",
    "conv2d",
    "vitdodec",
    "fft",
    "vitdodec",
};

enum accelerator_coherence design_choices[] = {
ACC_COH_RECALL,
ACC_COH_RECALL,
ACC_COH_LLC,
ACC_COH_RECALL,
ACC_COH_LLC,
ACC_COH_NONE,
ACC_COH_RECALL,
ACC_COH_NONE,
};

static void config_threads(FILE* f, accelerator_thread_info_t **thread_info, esp_thread_info_t ***cfg,
			   int phase, int* nthreads, int coherence_mode,
			   enum accelerator_coherence coherence, unsigned **nacc, unsigned **loop_cnt){

	fscanf(f, "%d", nthreads); 
	dprintf("%d threads in phase %d\n", *nthreads, phase); 
	*cfg = malloc(sizeof(esp_thread_info_t*) * *nthreads);
	*nacc = malloc(sizeof(unsigned) * *nthreads);
	*loop_cnt = malloc(sizeof(unsigned) * *nthreads); 

	for (int t = 0; t < *nthreads; t++){
		thread_info[t] = malloc(sizeof(accelerator_thread_info_t));
		thread_info[t]->tid = t;
                
		//get number of devices and size
		fscanf(f, "%d\n", &(thread_info[t]->ndev));
		dprintf("%d devices in thread %d.%d\n", thread_info[t]->ndev, phase, t);
 
		(*cfg)[t] = malloc(sizeof(esp_thread_info_t) * thread_info[t]->ndev);
		(*nacc)[t] = thread_info[t]->ndev;

		char flow_choice[7];
		fscanf(f, "%s\n", flow_choice);

		if(!strcmp(flow_choice, "p2p")){
			thread_info[t]->p2p = 1;
		} else { 
			thread_info[t]->p2p = 0;
		}
        
		char alloc_choice[13];
		fscanf(f, "%s\n", alloc_choice);

		if (!strcmp(alloc_choice, "preferred")){
			thread_info[t]->alloc_choice = CONTIG_ALLOC_PREFERRED;
		} else if (!strcmp(alloc_choice, "lloaded")){
			thread_info[t]->alloc_choice = CONTIG_ALLOC_LEAST_LOADED;
		} else if (!strcmp(alloc_choice, "balanced")){  
			thread_info[t]->alloc_choice = CONTIG_ALLOC_BALANCED;
		}

		fscanf(f, "%d\n", &thread_info[t]->loop_cnt);
		(*loop_cnt)[t] = thread_info[t]->loop_cnt;
		dprintf("%d loops in thread %d.%d\n", thread_info[t]->loop_cnt, phase, t);

		size_t in_size = 0;
		size_t first_in_size = 0;
		size_t out_size = 0;
		size_t memsz = 0;
		unsigned in_place = 0;
		unsigned int offset = 0;
        size_t prev_out_size;
        unsigned conv_in_size, conv_weight_size, conv_bias_size;
		/* char pattern[10]; */
		char coh_choice[7];
		for (int d = 0; d < thread_info[t]->ndev; d++){

			dprintf("device %d in thread %d\n", d, t);

			fscanf(f, "%d", &(thread_info[t]->chain[d])); 
            
			//esp accelerator parameters
			int devid = thread_info[t]->chain[d];
			(*cfg)[t][d].run = true;
			(*cfg)[t][d].devname = devnames[devid];

			fscanf(f, "%d %s", &(thread_info[t]->workload_size), coh_choice);
			
            unsigned s = thread_info[t]->workload_size;
            printf("%d: %s\n", devid, accnames[devid]);
            printf("workload size: %d\n", s);
            switch(devid){
                case VITDODEC0 :
                    vitdodec_cfg_000[0].esp.coherence = coherence;
                    vitdodec_cfg_000[0].esp.devid = VITDODEC0;
                    if (s == EXTRA_SMALL) {
                        vitdodec_cfg_000[0].nbatches = 1;           
                    } else if (s == SMALL) {
                        vitdodec_cfg_000[0].nbatches = 4;           
                    } else if (s == MEDIUM) {
                        vitdodec_cfg_000[0].nbatches = 6; 
                    } else if (s == LARGE) {
                        vitdodec_cfg_000[0].nbatches = 24;           
                    } else if (s == EXTRA_LARGE) {
                        vitdodec_cfg_000[0].nbatches = 96; 
                    }
                    in_size = 24856 * vitdodec_cfg_000[0].nbatches / 4;
                    out_size = 18592 * vitdodec_cfg_000[0].nbatches / 4;
                    in_place = 0;
                    vitdodec_cfg_000[0].src_offset = offset;
                    vitdodec_cfg_000[0].dst_offset = offset;
                    (*cfg)[t][d].ioctl_req = VITDODEC_STRATUS_IOC_ACCESS;
                    (*cfg)[t][d].esp_desc = &(vitdodec_cfg_000[0].esp);
                break;

                case VITDODEC1 :
                    vitdodec_cfg_001[0].esp.coherence = coherence;
                    vitdodec_cfg_001[0].esp.devid = VITDODEC1;
                    if (s == EXTRA_SMALL) {
                        vitdodec_cfg_001[0].nbatches = 1;           
                    } else if (s == SMALL) {
                        vitdodec_cfg_001[0].nbatches = 4;           
                    } else if (s == MEDIUM) {
                        vitdodec_cfg_001[0].nbatches = 6; 
                    } else if (s == LARGE) {
                        vitdodec_cfg_001[0].nbatches = 24;           
                    } else if (s == EXTRA_LARGE) {
                        vitdodec_cfg_001[0].nbatches = 96; 
                    }
                    in_size = 24856 * vitdodec_cfg_001[0].nbatches / 4;
                    out_size = 18592 * vitdodec_cfg_001[0].nbatches / 4;
                    in_place = 0;
                    vitdodec_cfg_001[0].src_offset = offset;
                    vitdodec_cfg_001[0].dst_offset = offset;
                    (*cfg)[t][d].ioctl_req = VITDODEC_STRATUS_IOC_ACCESS;
                    (*cfg)[t][d].esp_desc = &(vitdodec_cfg_001[0].esp);
                break;

                case FFT0 : 
                    fft_cfg_000[0].esp.coherence = coherence;
                    fft_cfg_000[0].esp.devid = FFT0;
                    if (s == EXTRA_SMALL) {
                        fft_cfg_000[0].batch_size = 1; 
                        fft_cfg_000[0].log_len = 10; 
                    } else if (s == SMALL) {
                        fft_cfg_000[0].batch_size = 2; 
                        fft_cfg_000[0].log_len = 11; 
                    } else if (s == MEDIUM) {
                        fft_cfg_000[0].batch_size = 4; 
                        fft_cfg_000[0].log_len = 12; 
                    } else if (s == LARGE) {
                        fft_cfg_000[0].batch_size = 8; 
                        fft_cfg_000[0].log_len = 13; 
                    } else if (s == EXTRA_LARGE) {
                        fft_cfg_000[0].batch_size = 32;
                        fft_cfg_000[0].log_len = 13; 
                    }
                    in_size = (1 << (fft_cfg_000[0].log_len + 1))
                                * fft_cfg_000[0].batch_size;
                    out_size = in_size;
                    in_place = 0;
                    fft_cfg_000[0].src_offset = offset;
                    fft_cfg_000[0].dst_offset = offset + in_size * 4;
                    (*cfg)[t][d].ioctl_req = FFT_STRATUS_IOC_ACCESS;
                    (*cfg)[t][d].esp_desc = &(fft_cfg_000[0].esp);
                    
                break;
                
                case FFT1 : 
                    fft_cfg_001[0].esp.coherence = coherence;
                    fft_cfg_001[0].esp.devid = FFT1;
                    if (s == EXTRA_SMALL) {
                        fft_cfg_001[0].batch_size = 1; 
                        fft_cfg_001[0].log_len = 10; 
                    } else if (s == SMALL) {
                        fft_cfg_001[0].batch_size = 2; 
                        fft_cfg_001[0].log_len = 11; 
                    } else if (s == MEDIUM) {
                        fft_cfg_001[0].batch_size = 4; 
                        fft_cfg_001[0].log_len = 12; 
                    } else if (s == LARGE) {
                        fft_cfg_001[0].batch_size = 8; 
                        fft_cfg_001[0].log_len = 13; 
                    } else if (s == EXTRA_LARGE) {
                        fft_cfg_001[0].batch_size = 32;
                        fft_cfg_001[0].log_len = 13; 
                    }
                    in_size = (1 << (fft_cfg_001[0].log_len + 1))
                                * fft_cfg_001[0].batch_size;
                    out_size = in_size;
                    in_place = 0;
                    fft_cfg_001[0].src_offset = offset;
                    fft_cfg_001[0].dst_offset = offset + in_size * 4;
                    (*cfg)[t][d].ioctl_req = FFT_STRATUS_IOC_ACCESS;
                    (*cfg)[t][d].esp_desc = &(fft_cfg_001[0].esp);
                    
                break;

                case CONV2D0 :
                    conv2d_cfg_000[0].esp.coherence = coherence;
                    conv2d_cfg_000[0].esp.devid = CONV2D0;
                    if (s == EXTRA_SMALL) {
                        conv2d_cfg_000[0].batch_size = 4;
                        conv2d_cfg_000[0].n_channels = 8;
                        conv2d_cfg_000[0].n_filters = 6;
                        conv2d_cfg_000[0].feature_map_width = 8;
                        conv2d_cfg_000[0].feature_map_height = 8;
                        conv2d_cfg_000[0].filter_dim = 3;
                    } else if (s == SMALL) {
                        conv2d_cfg_000[0].batch_size = 4;
                        conv2d_cfg_000[0].n_channels = 8;
                        conv2d_cfg_000[0].n_filters = 8;
                        conv2d_cfg_000[0].feature_map_width = 16;
                        conv2d_cfg_000[0].feature_map_height = 16;
                        conv2d_cfg_000[0].filter_dim = 1;
                    } else if (s == MEDIUM) {
                        conv2d_cfg_000[0].batch_size = 8;
                        conv2d_cfg_000[0].n_channels = 16;
                        conv2d_cfg_000[0].n_filters = 16;
                        conv2d_cfg_000[0].feature_map_width = 16;
                        conv2d_cfg_000[0].feature_map_height = 16;
                        conv2d_cfg_000[0].filter_dim = 3;
                    } else if (s == LARGE) {
                        conv2d_cfg_000[0].batch_size = 8;
                        conv2d_cfg_000[0].n_channels = 16;
                        conv2d_cfg_000[0].n_filters = 16;
                        conv2d_cfg_000[0].feature_map_width = 32;
                        conv2d_cfg_000[0].feature_map_height = 32;
                        conv2d_cfg_000[0].filter_dim = 5;
                    } else if (s == EXTRA_LARGE) {
                        conv2d_cfg_000[0].batch_size = 16;
                        conv2d_cfg_000[0].n_channels = 32;
                        conv2d_cfg_000[0].n_filters = 32;
                        conv2d_cfg_000[0].feature_map_width = 32;
                        conv2d_cfg_000[0].feature_map_height = 32;
                        conv2d_cfg_000[0].filter_dim = 3;
                    }
                    conv_in_size = conv2d_cfg_000[0].batch_size * conv2d_cfg_000[0].n_channels
                              * conv2d_cfg_000[0].feature_map_width * conv2d_cfg_000[0].feature_map_height;
                    conv_weight_size = conv2d_cfg_000[0].n_filters * conv2d_cfg_000[0].n_channels *
                                       conv2d_cfg_000[0].filter_dim * conv2d_cfg_000[0].filter_dim;
                    conv_bias_size = conv2d_cfg_000[0].n_filters;
                    in_size = conv_in_size + conv_weight_size + conv_bias_size;
                    out_size = conv2d_cfg_000[0].batch_size * conv2d_cfg_000[0].n_filters *
                               conv2d_cfg_000[0].feature_map_width * conv2d_cfg_000[0].feature_map_height;
                    in_place = 0;
                    conv2d_cfg_000[0].src_offset = offset;
                    conv2d_cfg_000[0].dst_offset = offset;
                    (*cfg)[t][d].ioctl_req = CONV2D_STRATUS_IOC_ACCESS;
                    (*cfg)[t][d].esp_desc = &(conv2d_cfg_000[0].esp);
                break;
                
                case CONV2D1 :
                    conv2d_cfg_001[0].esp.coherence = coherence;
                    conv2d_cfg_001[0].esp.devid = CONV2D1;
                    if (s == EXTRA_SMALL) {
                        conv2d_cfg_001[0].batch_size = 4;
                        conv2d_cfg_001[0].n_channels = 8;
                        conv2d_cfg_001[0].n_filters = 6;
                        conv2d_cfg_001[0].feature_map_width = 8;
                        conv2d_cfg_001[0].feature_map_height = 8;
                        conv2d_cfg_001[0].filter_dim = 3;
                    } else if (s == SMALL) {
                        conv2d_cfg_001[0].batch_size = 4;
                        conv2d_cfg_001[0].n_channels = 8;
                        conv2d_cfg_001[0].n_filters = 8;
                        conv2d_cfg_001[0].feature_map_width = 16;
                        conv2d_cfg_001[0].feature_map_height = 16;
                        conv2d_cfg_001[0].filter_dim = 1;
                    } else if (s == MEDIUM) {
                        conv2d_cfg_001[0].batch_size = 8;
                        conv2d_cfg_001[0].n_channels = 16;
                        conv2d_cfg_001[0].n_filters = 16;
                        conv2d_cfg_001[0].feature_map_width = 16;
                        conv2d_cfg_001[0].feature_map_height = 16;
                        conv2d_cfg_001[0].filter_dim = 3;
                    } else if (s == LARGE) {
                        conv2d_cfg_001[0].batch_size = 8;
                        conv2d_cfg_001[0].n_channels = 16;
                        conv2d_cfg_001[0].n_filters = 16;
                        conv2d_cfg_001[0].feature_map_width = 32;
                        conv2d_cfg_001[0].feature_map_height = 32;
                        conv2d_cfg_001[0].filter_dim = 5;
                    } else if (s == EXTRA_LARGE) {
                        conv2d_cfg_001[0].batch_size = 16;
                        conv2d_cfg_001[0].n_channels = 32;
                        conv2d_cfg_001[0].n_filters = 32;
                        conv2d_cfg_001[0].feature_map_width = 32;
                        conv2d_cfg_001[0].feature_map_height = 32;
                        conv2d_cfg_001[0].filter_dim = 3;
                    }
                    conv_in_size = conv2d_cfg_001[0].batch_size * conv2d_cfg_001[0].n_channels
                              * conv2d_cfg_001[0].feature_map_width * conv2d_cfg_001[0].feature_map_height;
                    conv_weight_size = conv2d_cfg_001[0].n_filters * conv2d_cfg_001[0].n_channels *
                                       conv2d_cfg_001[0].filter_dim * conv2d_cfg_001[0].filter_dim;
                    conv_bias_size = conv2d_cfg_001[0].n_filters;
                    in_size = conv_in_size + conv_weight_size + conv_bias_size;
                    out_size = conv2d_cfg_001[0].batch_size * conv2d_cfg_001[0].n_filters *
                               conv2d_cfg_001[0].feature_map_width * conv2d_cfg_001[0].feature_map_height;
                    in_place = 0;
                    conv2d_cfg_001[0].src_offset = offset;
                    conv2d_cfg_001[0].dst_offset = offset;
                    (*cfg)[t][d].ioctl_req = CONV2D_STRATUS_IOC_ACCESS;
                    (*cfg)[t][d].esp_desc = &(conv2d_cfg_001[0].esp);
                break;

            case GEMM0 : 
                gemm_cfg_000[0].esp.coherence = coherence;
                gemm_cfg_000[0].esp.devid = GEMM0;
                if (s == EXTRA_SMALL) {
                    gemm_cfg_000[0].ninputs = 2;
                    gemm_cfg_000[0].d1 = 32;
                    gemm_cfg_000[0].d2 = 32;
                    gemm_cfg_000[0].d3 = 16;
                    gemm_cfg_000[0].transpose = 1;
                } else if (s == SMALL) {
                    gemm_cfg_000[0].ninputs = 6;
                    gemm_cfg_000[0].d1 = 32;
                    gemm_cfg_000[0].d2 = 28;
                    gemm_cfg_000[0].d3 = 32;
                    gemm_cfg_000[0].transpose = 0;
                } else if (s == MEDIUM) {
                    gemm_cfg_000[0].ninputs = 6;
                    gemm_cfg_000[0].d1 = 64;
                    gemm_cfg_000[0].d2 = 52;
                    gemm_cfg_000[0].d3 = 64;
                    gemm_cfg_000[0].transpose = 1;
                } else if (s == LARGE) {
                    gemm_cfg_000[0].ninputs = 8;
                    gemm_cfg_000[0].d1 = 128;
                    gemm_cfg_000[0].d2 = 64;
                    gemm_cfg_000[0].d3 = 128;
                    gemm_cfg_000[0].transpose = 0;
                } else if (s == EXTRA_LARGE) {
                    gemm_cfg_000[0].ninputs = 8;
                    gemm_cfg_000[0].d1 = 256;
                    gemm_cfg_000[0].d2 = 128;
                    gemm_cfg_000[0].d3 = 256;
                    gemm_cfg_000[0].transpose = 1;
                }
                
                gemm_cfg_000[0].ld_offset2 = gemm_cfg_000[0].ninputs *
                                             gemm_cfg_000[0].d1 *
                                             gemm_cfg_000[0].d2;
                gemm_cfg_000[0].st_offset = gemm_cfg_000[0].ld_offset2 + 
                                            gemm_cfg_000[0].ninputs *
                                            gemm_cfg_000[0].d2 *
                                            gemm_cfg_000[0].d3;
                in_size = gemm_cfg_000[0].st_offset * 4;
                out_size = gemm_cfg_000[0].ninputs *
                           gemm_cfg_000[0].d1 *
                           gemm_cfg_000[0].d3 * 4;
                in_place = 0;
                gemm_cfg_000[0].src_offset = offset;
                gemm_cfg_000[0].dst_offset = offset;
                (*cfg)[t][d].ioctl_req = GEMM_STRATUS_IOC_ACCESS;
                (*cfg)[t][d].esp_desc = &(gemm_cfg_000[0].esp);
            break;

            case GEMM1 : 
                gemm_cfg_001[0].esp.coherence = coherence;
                gemm_cfg_001[0].esp.devid = GEMM1;
                if (s == EXTRA_SMALL) {
                    gemm_cfg_001[0].ninputs = 2;
                    gemm_cfg_001[0].d1 = 32;
                    gemm_cfg_001[0].d2 = 32;
                    gemm_cfg_001[0].d3 = 16;
                    gemm_cfg_001[0].transpose = 1;
                } else if (s == SMALL) {
                    gemm_cfg_001[0].ninputs = 6;
                    gemm_cfg_001[0].d1 = 32;
                    gemm_cfg_001[0].d2 = 28;
                    gemm_cfg_001[0].d3 = 32;
                    gemm_cfg_001[0].transpose = 0;
                } else if (s == MEDIUM) {
                    gemm_cfg_001[0].ninputs = 6;
                    gemm_cfg_001[0].d1 = 64;
                    gemm_cfg_001[0].d2 = 52;
                    gemm_cfg_001[0].d3 = 64;
                    gemm_cfg_001[0].transpose = 1;
                } else if (s == LARGE) {
                    gemm_cfg_001[0].ninputs = 8;
                    gemm_cfg_001[0].d1 = 128;
                    gemm_cfg_001[0].d2 = 64;
                    gemm_cfg_001[0].d3 = 128;
                    gemm_cfg_001[0].transpose = 0;
                } else if (s == EXTRA_LARGE) {
                    gemm_cfg_001[0].ninputs = 8;
                    gemm_cfg_001[0].d1 = 256;
                    gemm_cfg_001[0].d2 = 128;
                    gemm_cfg_001[0].d3 = 256;
                    gemm_cfg_001[0].transpose = 1;
                }
                
                gemm_cfg_001[0].ld_offset2 = gemm_cfg_001[0].ninputs *
                                             gemm_cfg_001[0].d1 *
                                             gemm_cfg_001[0].d2;
                gemm_cfg_001[0].st_offset = gemm_cfg_001[0].ld_offset2 + 
                                            gemm_cfg_001[0].ninputs *
                                            gemm_cfg_001[0].d2 *
                                            gemm_cfg_001[0].d3;
                in_size = gemm_cfg_001[0].st_offset * 4;
                out_size = gemm_cfg_001[0].ninputs *
                           gemm_cfg_001[0].d1 *
                           gemm_cfg_001[0].d3 * 4;
                in_place = 0;
                gemm_cfg_001[0].src_offset = offset;
                gemm_cfg_001[0].dst_offset = offset;
                (*cfg)[t][d].ioctl_req = GEMM_STRATUS_IOC_ACCESS;
                (*cfg)[t][d].esp_desc = &(gemm_cfg_001[0].esp);
            break;
            
            }			
            // TODO double check these lines for the case of multiple acc in a thread	
            if (!d)
                memsz += in_size;
            else if (in_size > prev_out_size)
                memsz += (in_size - prev_out_size);

			if(!in_place && !thread_info[t]->p2p){
				memsz += out_size;
				offset += in_size;
			}

			if (thread_info[t]->p2p && d == thread_info[t]->ndev - 1){
				memsz += out_size;
			}

			unsigned int footprint = in_size;

			if (!in_place && (!thread_info[t]->p2p || d == thread_info[t]->ndev - 1))
				footprint += out_size;
	        prev_out_size = out_size;

			((*cfg)[t][d].esp_desc)->learn = 0;
			if (thread_info[t]->p2p){
				((*cfg)[t][d].esp_desc)->coherence = ACC_COH_NONE;
			}
			else if (coherence_mode == FIXED){
				((*cfg)[t][d].esp_desc)->coherence = coherence;
				if (coherence == ACC_COH_LEARN)
					((*cfg)[t][d].esp_desc)->learn = 1;
			} 
            else if (coherence_mode == DESIGN){
                ((*cfg)[t][d].esp_desc)->coherence = design_choices[devid];
            }
			else if (coherence_mode == RAND){
				((*cfg)[t][d].esp_desc)->coherence = rand() % 4;
			}
			else if (!strcmp(coh_choice, "none")){
				((*cfg)[t][d].esp_desc)->coherence = ACC_COH_NONE;
			}
			else if (!strcmp(coh_choice, "llc")){
				((*cfg)[t][d].esp_desc)->coherence = ACC_COH_LLC;
			}
			else if (!strcmp(coh_choice, "recall")){
				((*cfg)[t][d].esp_desc)->coherence = ACC_COH_RECALL;
			}
			else if (!strcmp(coh_choice, "full")){
				((*cfg)[t][d].esp_desc)->coherence = ACC_COH_FULL;
			}

			((*cfg)[t][d].esp_desc)->footprint = footprint * 4;
			((*cfg)[t][d].esp_desc)->in_place = in_place;
			((*cfg)[t][d].esp_desc)->reuse_factor = 1;

			// in_size = out_size;
			if (!d)
				first_in_size = in_size;
        }
        thread_info[t]->memsz = memsz * 4;
		thread_info[t]->memsz_in = first_in_size;
	}
}

static void alloc_phase(accelerator_thread_info_t **thread_info, esp_thread_info_t ***cfg, int nthreads, int alloc_mode, enum contig_alloc_policy alloc, uint32_t **buffers, int phase, struct contig_alloc_params *alloc_params){

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

        for (int j = 0; j < thread_info[i]->memsz_in; j++){
            buffers[i][j] = rand();
        }
	
		for (int acc = 0; acc < thread_info[i]->ndev; acc++){
			(*cfg)[i][acc].hw_buf = (void*) buffers[i];
		}
	}
}

static int validate_buffer(accelerator_thread_info_t *thread_info, esp_thread_info_t **cfg, uint32_t *buf){
	int errors = 0; 
	/*   for (int i = 0; i < thread_info->ndev; i++){
	     if (thread_info->p2p && i != thread_info->ndev - 1)
	     continue;

	     int t = thread_info->tid;
	     int offset = cfg[t][i].desc.synth_desc.offset;
	     int in_size = cfg[t][i].desc.synth_desc.in_size;
	     int out_size = cfg[t][i].desc.synth_desc.out_size;
	     int in_place = cfg[t][i].desc.synth_desc.in_place;
	     int wr_data = cfg[t][i].desc.synth_desc.wr_data;
	     int next_in_place;
        
	     if (i != thread_info->ndev - 1){
	     next_in_place = cfg[t][i+1].desc.synth_desc.in_place;
	     if (next_in_place)
	     continue;
	     }

	     if (!in_place && !thread_info->p2p)
	     offset += in_size; 
	     else if (thread_info->p2p)
	     offset = cfg[t][0].desc.synth_desc.in_size;

	     for (int j = offset; j < offset + out_size; j++){
	     if (j == offset + out_size - 1 && buf[j] != wr_data){
	     errors += buf[j];
	     printf("%u read errors in thread %d device %d\n", buf[j], t, i);
	     }
	     else if (j != offset + out_size - 1 && buf[j] != wr_data){
	     errors++;
	     }
	     }
	     }*/
	return errors;
	//return 0;
}

static void free_phase(accelerator_thread_info_t **thread_info, esp_thread_info_t **cfg, int nthreads){
	for (int i = 0; i < nthreads; i++){
        esp_free(cfg[i]->hw_buf); 
		free(thread_info[i]);
		free(cfg[i]);
	}
	free(cfg);
}

static void dump_results(FILE* out_file_d, FILE* out_file_t, FILE* out_file_p, FILE* out_file_status_d, accelerator_thread_info_t **thread_info, esp_thread_info_t ** cfg, int phase, int nthreads, char** argv, int test_no, unsigned long long *phase_total_cycles, unsigned long long phase_time, unsigned long long phase_ddr_accesses){
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
				if ((cfg[t][d].esp_desc)->coherence == ACC_COH_FULL)
					fprintf(out_file_d,"full,");
				else if ((cfg[t][d].esp_desc)->coherence == ACC_COH_LLC)
					fprintf(out_file_d,"llc,");
				else if ((cfg[t][d].esp_desc)->coherence == ACC_COH_RECALL)
					fprintf(out_file_d,"recall,");
				else if ((cfg[t][d].esp_desc)->coherence == ACC_COH_NONE)
					fprintf(out_file_d,"none,");
				else if ((cfg[t][d].esp_desc)->coherence == ACC_COH_AUTO)
					fprintf(out_file_d,"auto,");
				else if ((cfg[t][d].esp_desc)->coherence == ACC_COH_LEARN)
					fprintf(out_file_d,"learn,");

				if (thread_info[t]->alloc_choice == CONTIG_ALLOC_BALANCED)
					fprintf(out_file_d,"balanced,");
				else if (thread_info[t]->alloc_choice == CONTIG_ALLOC_PREFERRED)
					fprintf(out_file_d,"preferred,");
				else if (thread_info[t]->alloc_choice == CONTIG_ALLOC_LEAST_LOADED)
					fprintf(out_file_d,"lloaded,");
				else if (thread_info[t]->alloc_choice == CONTIG_ALLOC_LEAST_UTILIZED)
					fprintf(out_file_d,"lutil,");

				fprintf(out_file_d, "%u,", (cfg[t][d].esp_desc)->footprint); 

				fprintf(out_file_d,"%d,", (cfg[t][d].esp_desc)->ddr_node);
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
        
		fprintf(out_file_t,"%d,", (cfg[t][0].esp_desc)->ddr_node);
		fprintf(out_file_t, "%llu,", thread_info[t]->thread_time);
		fprintf(out_file_t, "%llu,", thread_ddr_accesses);
		fprintf(out_file_t, "%u,", thread_info[t]->alloc_status.active_threads);
		for (i = 0; i < SOC_NDDR_CONTIG; i++){
			fprintf(out_file_t, "%u", thread_info[t]->alloc_status.active_allocations_split[i]);
			if (i == SOC_NDDR_CONTIG-1)
				fprintf(out_file_t, "\n");
			else 
				fprintf(out_file_t, ",");
		}
	}
	fprintf(out_file_p, "%d,", phase_adj);
	fprintf(out_file_p, "%d,", thread_info[0]->chain[0]);
	if ((cfg[0][0].esp_desc)->coherence == ACC_COH_FULL)
        fprintf(out_file_p,"full,");
    else if ((cfg[0][0].esp_desc)->coherence == ACC_COH_LLC)
        fprintf(out_file_p,"llc,");
    else if ((cfg[0][0].esp_desc)->coherence == ACC_COH_RECALL)
        fprintf(out_file_p,"recall,");
    else if ((cfg[0][0].esp_desc)->coherence == ACC_COH_NONE)
        fprintf(out_file_p,"none,");
    else if ((cfg[0][0].esp_desc)->coherence == ACC_COH_AUTO)
        fprintf(out_file_p,"auto,");
    else if ((cfg[0][0].esp_desc)->coherence == ACC_COH_LEARN)
        fprintf(out_file_p,"learn,");
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
		printf("Usage: ./multiacc.exe file coherence alloc [epsilon] [alpha]\n");
		return -1;
	}
	//command line args
	FILE* f;
	f = fopen(argv[1], "r");

	int test_no = argv[1][9] - 48;
	printf("test_no %d\n", test_no);
	const char* out_name_d = "multiacc_devices.csv";
	const char* out_name_t = "multiacc_threads.csv";
	const char* out_name_p = "multiacc_phases.csv";
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
		fprintf(out_file_t,  "Thread name, nacc, loop_cnt, coherence, allocation, footprint, DDR#, time, ddr_accesses, active_threads,");
		for (i = 0; i < SOC_NDDR_CONTIG; i++){
			fprintf(out_file_t, "active_allocations_split%d", i);
			if (i == SOC_NDDR_CONTIG-1)
				fprintf(out_file_t, "\n");
			else 
				fprintf(out_file_t, ",");
		}
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
	enum contig_alloc_policy alloc = CONTIG_ALLOC_PREFERRED; 
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
	else if (!strcmp(argv[2], "learn")){
		coherence_mode = FIXED;
		coherence = ACC_COH_LEARN;
		if (argc != 7){
			printf ("Supply learning parameters\n");
			return -1;
		}
		set_learning_params(atof(argv[4]), atof(argv[5]), atof(argv[6]));        
	} else if (!strcmp(argv[2], "design")){
        coherence_mode = DESIGN;
    }
	else{
		printf("Valid coherence choices include none, llc, recall, full, auto, cfg, rand, learn, and design\n");
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
	else if (!strcmp(argv[3], "auto")){
		alloc_mode = FIXED;
		alloc = CONTIG_ALLOC_AUTO;
	}
	else if (!strcmp(argv[3], "cfg")){
		alloc_mode = CFG;
	}
	else if (!strcmp(argv[3], "rand")){
		alloc_mode = RAND;
	}
	else if (!strcmp(argv[3], "first")){
		alloc_mode = FIRST;
		alloc = CONTIG_ALLOC_PREFERRED;
	}
	else{
		printf("Valid alloc choices include preferred, lloaded, balanced, auto, cfg, and learn\n");
		return 1;
	}

	if (argc == 7){
		set_learning_params(atof(argv[4]), atof(argv[5]), atof(argv[6]));
	} else {
		printf("No learning parameters supplied, using defaults\n");
	}

	esp_init();
   
    //struct contig_alloc_params cache_fill_params;
    //struct esp_status_alloc status_alloc;
    //cache_fill_params.pol.balanced.threshold = 0;
	//cache_fill_params.pol.balanced.cluster_size = 1;
    //cache_fill_params.pol.balanced.ddr_node = 0;
    //int *cache_fill_buffer = (int*) esp_alloc_policy(&cache_fill_params, 3145728, &status_alloc);
   
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
	unsigned *nacc = NULL;
	unsigned *loop_cnt = NULL;
	unsigned long long total_cycles = 0, total_ddr_accesses = 0;
	unsigned long long phase_cycles, phase_ddr_accesses;
	unsigned int ddr_accesses_start[SOC_NDDR_CONTIG];
	unsigned int ddr_accesses_end[SOC_NDDR_CONTIG];
	//loop over phases - config, alloc, spawn thread, validate, and free
	for (int p = 0; p < nphases; p++){
        config_threads(f, thread_info[p], &cfg, p, &nthreads, coherence_mode,
			       coherence, &nacc, &loop_cnt); 
        alloc_phase(thread_info[p], &cfg, nthreads, alloc_mode,
			    alloc, buffers, p, params);
        
    //    for (int j = 0; j < 3145728; j++)
     //       cache_fill_buffer[j] = j;

		read_ddr_accesses(ddr_accesses_start);
		gettime(&th_start);

		esp_run_parallel(cfg, nthreads, nacc, loop_cnt);

		gettime(&th_end);
		read_ddr_accesses(ddr_accesses_end);
        
		for (int t = 0; t < nthreads; t++){
			int errors = validate_buffer(thread_info[p][t], cfg, buffers[t]);
			if (errors)
				printf("[FAIL] Thread %d.%d : %u errors\n", p, t, errors);
			else
				printf("[PASS] Thread %d.%d\n", p, t);
		}
        
		phase_ddr_accesses = calculate_phase_ddr_accesses(ddr_accesses_start, ddr_accesses_end);
        
		hw_ns = ts_subtract(&th_start, &th_end);
		hw_ns_total += hw_ns;
		hw_s = (float) hw_ns / 1000000000;

		printf("PHASE.%d %.4f s\n", p, hw_s);
      
		dump_results(out_file_d, out_file_t, out_file_p, out_file_status_d, thread_info[p], cfg, p, nthreads, argv, test_no, &phase_cycles, hw_ns, phase_ddr_accesses);

		total_cycles += phase_cycles;
		total_ddr_accesses += phase_ddr_accesses;

		free_phase(thread_info[p], cfg, nthreads);
		free(nacc);
	}
	hw_s_total = (float) hw_ns_total / 1000000000;
	//   total_s = (float) total_ns / 1000000000.0;
	printf("TOTAL TIME %.4f s\n", hw_s_total);
    
	//printf("TOTAL CYCLES %llu\n", total_cycles);
	printf("TOTAL DDR ACCESSES %llu\n", total_ddr_accesses);
//    esp_free(cache_fill_buffer);
	fclose(f);
	fclose(out_file_d);
	fclose(out_file_t);
	fclose(out_file_p);
	esp_cleanup();
    return 0; 
}
