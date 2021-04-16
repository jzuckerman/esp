#ifndef DEFINES_H_
#define DEFINES_H_

#include <complex>
#include "ap_int.h"
#include "ap_fixed.h"

//hls-fpga-machine-learning insert numbers
#define N_INPUT_1_1 1024
#define N_LAYER_2 256
#define N_LAYER_4 128
#define N_LAYER_6 256
#define N_LAYER_8 1024

//hls-fpga-machine-learning insert layer-precision
typedef ap_fixed<16,6> model_default_t;
typedef ap_fixed<16,6> input_t;
typedef ap_fixed<16,6> layer2_t;
typedef ap_fixed<16,6> layer3_t;
typedef ap_fixed<16,6> layer4_t;
typedef ap_fixed<16,6> layer5_t;
typedef ap_fixed<16,6> layer6_t;
typedef ap_fixed<16,6> layer7_t;
typedef ap_fixed<16,6> layer8_t;
typedef ap_fixed<16,6> result_t;

#endif
