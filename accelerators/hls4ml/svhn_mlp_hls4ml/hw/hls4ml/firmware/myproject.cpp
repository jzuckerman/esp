//
//    rfnoc-hls-neuralnet: Vivado HLS code for neural-net building blocks
//
//    Copyright (C) 2017 EJ Kreinar
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <iostream>

#include "myproject.h"

//hls-fpga-machine-learning insert weights
#include "weights/w2.h"
#include "weights/b2.h"
#include "weights/w4.h"
#include "weights/b4.h"
#include "weights/w6.h"
#include "weights/b6.h"
#include "weights/w8.h"
#include "weights/b8.h"
#include "weights/w10.h"
#include "weights/b10.h"

void myproject(
    input_t input1[N_INPUT_1_1],
    result_t layer11_out[N_LAYER_10],
    unsigned short &const_size_in_1,
    unsigned short &const_size_out_1
) {

    //hls-fpga-machine-learning insert IO
    // #pragma HLS ARRAY_RESHAPE variable=input1 complete dim=0 
    // #pragma HLS ARRAY_RESHAPE variable=layer11_out complete dim=0 
    // #pragma HLS INTERFACE ap_vld port=input1,layer11_out 
    // #pragma HLS DATAFLOW 

    const_size_in_1 = N_INPUT_1_1;
    const_size_out_1 = N_LAYER_10;

#ifndef __SYNTHESIS__
    // printf("INFO: load weight and bias values from files\n");
    //hls-fpga-machine-learning insert load weights
    load_txt_file< ap_fixed<18,8>, config2::n_in * config2::n_out >(w2, "w2.txt");
    load_txt_file< ap_fixed<18,8>, config2::n_out >(b2, "b2.txt");
    load_txt_file< ap_fixed<18,8>, config4::n_in * config4::n_out >(w4, "w4.txt");
    load_txt_file< ap_fixed<18,8>, config4::n_out >(b4, "b4.txt");
    load_txt_file< ap_fixed<18,8>, config6::n_in * config6::n_out >(w6, "w6.txt");
    load_txt_file< ap_fixed<18,8>, config6::n_out >(b6, "b6.txt");
    load_txt_file< ap_fixed<18,8>, config8::n_in * config8::n_out >(w8, "w8.txt");
    load_txt_file< ap_fixed<18,8>, config8::n_out >(b8, "b8.txt");
    load_txt_file< ap_fixed<18,8>, config10::n_in * config10::n_out >(w10, "w10.txt");
    load_txt_file< ap_fixed<18,8>, config10::n_out >(b10, "b10.txt");
#endif

    // ****************************************
    // NETWORK INSTANTIATION
    // ****************************************

    //hls-fpga-machine-learning insert layers

    layer2_t layer2_out[N_LAYER_2];
    #pragma HLS ARRAY_PARTITION variable=layer2_out complete dim=0
    nnet::dense_large<input_t, layer2_t, config2>(input1, layer2_out, w2, b2);

    layer3_t layer3_out[N_LAYER_2];
    #pragma HLS ARRAY_PARTITION variable=layer3_out complete dim=0
    nnet::relu<layer2_t, layer3_t, relu_config3>(layer2_out, layer3_out);

    layer4_t layer4_out[N_LAYER_4];
    #pragma HLS ARRAY_PARTITION variable=layer4_out complete dim=0
    nnet::dense_large<layer3_t, layer4_t, config4>(layer3_out, layer4_out, w4, b4);

    layer5_t layer5_out[N_LAYER_4];
    #pragma HLS ARRAY_PARTITION variable=layer5_out complete dim=0
    nnet::relu<layer4_t, layer5_t, relu_config5>(layer4_out, layer5_out);

    layer6_t layer6_out[N_LAYER_6];
    #pragma HLS ARRAY_PARTITION variable=layer6_out complete dim=0
    nnet::dense_large<layer5_t, layer6_t, config6>(layer5_out, layer6_out, w6, b6);

    layer7_t layer7_out[N_LAYER_6];
    #pragma HLS ARRAY_PARTITION variable=layer7_out complete dim=0
    nnet::relu<layer6_t, layer7_t, relu_config7>(layer6_out, layer7_out);

    layer8_t layer8_out[N_LAYER_8];
    #pragma HLS ARRAY_PARTITION variable=layer8_out complete dim=0
    nnet::dense_large<layer7_t, layer8_t, config8>(layer7_out, layer8_out, w8, b8);

    layer9_t layer9_out[N_LAYER_8];
    #pragma HLS ARRAY_PARTITION variable=layer9_out complete dim=0
    nnet::relu<layer8_t, layer9_t, relu_config9>(layer8_out, layer9_out);

    layer10_t layer10_out[N_LAYER_10];
    #pragma HLS ARRAY_PARTITION variable=layer10_out complete dim=0
    nnet::dense_large<layer9_t, layer10_t, config10>(layer9_out, layer10_out, w10, b10);

    nnet::softmax<layer10_t, result_t, softmax_config11>(layer10_out, layer11_out);
}
