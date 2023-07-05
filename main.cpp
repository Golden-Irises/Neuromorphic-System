#pragma once

#define neunet_eps  DBL_EPSILON
#define nuenet_len  0x0080

#define nuenet_arg  false

#define MNIST_MSG   false

#include <iostream>

#include "SRC/CSV/csv"
#include "ANN/neunet"
#include "SRC/MNIST/mnist.h"

#define LEARN_RATE       .4     // 1e-5
#define BN_LEARN_RATE    1e-5

using namespace std;
using namespace neunet;

int main(int argc, char *argv[], char *envp[]) {
    auto chrono_begin = neunet_chrono_time_point;
    cout << "hello, world." << endl;

    NeunetCore net_core {125, 125};

    string root = "SRC\\ARCHIVE\\";
    NeunetAddLayer<LayerConv<20, 5, 5, 1, 1, 0, 0, LEARN_RATE>>(net_core, root + "C0.csv");
    NeunetAddLayer<LayerBN<0., 1., BN_LEARN_RATE, BN_LEARN_RATE>>(net_core, root + "B0_shift.csv", root + "B0_scale.csv");
    // NeunetAddLayer<LayerBias<LEARN_RATE>>(net_core, root + "S0.csv");
    NeunetAddLayer<LayerAct<neunet_ReLU>>(net_core);
    NeunetAddLayer<LayerPool<neunet_avg_pool, 2, 2, 2, 2>>(net_core);
    NeunetAddLayer<LayerConv<50, 5, 5, 1, 1, 0, 0, LEARN_RATE>>(net_core, root + "C2.csv");
    NeunetAddLayer<LayerBN<0., 1., BN_LEARN_RATE, BN_LEARN_RATE>>(net_core, root + "B2_shift.csv", root + "B2_scale.csv");
    // NeunetAddLayer<LayerBias<LEARN_RATE>>(net_core, root + "S2.csv");
    NeunetAddLayer<LayerAct<neunet_ReLU>>(net_core);
    NeunetAddLayer<LayerPool<neunet_avg_pool, 2, 2, 2, 2>>(net_core);
    NeunetAddLayer<LayerFlat>(net_core);
    NeunetAddLayer<LayerFC<500, LEARN_RATE>>(net_core, root + "F4.csv");
    NeunetAddLayer<LayerBN<0., 1., BN_LEARN_RATE, BN_LEARN_RATE>>(net_core, root + "B4_shift.csv", root + "B4_scale.csv");
    // NeunetAddLayer<LayerBias<LEARN_RATE>>(net_core, root + "S4.csv");
    NeunetAddLayer<LayerAct<neunet_sigmoid>>(net_core);
    NeunetAddLayer<LayerFC<10, LEARN_RATE>>(net_core, root + "F5.csv");
    NeunetAddLayer<LayerBN<0., 1., BN_LEARN_RATE, BN_LEARN_RATE>>(net_core, root + "B5_shift.csv", root + "B5_scale.csv");
    // NeunetAddLayer<LayerBias<LEARN_RATE>>(net_core, root + "S5.csv");
    NeunetAddLayer<LayerAct<neunet_softmax>>(net_core);

    root = "...\\MNIST\\";
    auto train_elem = root + "train-images.idx3-ubyte",
         train_lbl  = root + "train-labels.idx1-ubyte",
         test_elem  = root + "t10k-images.idx3-ubyte",
         test_lbl   = root + "t10k-labels.idx1-ubyte";

    mnist_stream train_file, test_file;
	mnist_data train_data, test_data;

	auto train_data_load = mnist_open(&train_file, train_elem.c_str(), train_lbl.c_str()) &&
                           mnist_magic_verify(&train_file) &&
                           mnist_qty_verify(&train_file, &train_data);
    if (!train_data_load) return EXIT_FAILURE;
    auto ln_cnt  = mnist_ln_cnt(&train_file),
         col_cnt = mnist_col_cnt(&train_file);
    mnist_read(&train_file, &train_data, ln_cnt, col_cnt, 0, true);
    mnist_close(&train_file);
	auto train_idx = mnist_idx(&train_data);
    
    auto test_data_load = mnist_open(&test_file, test_elem.c_str(), test_lbl.c_str()) &&
                          mnist_magic_verify(&test_file) &&
                          mnist_qty_verify(&test_file, &test_data);
    if (!test_data_load) return EXIT_FAILURE;
    mnist_ln_cnt(&test_file);
    mnist_col_cnt(&test_file);
    mnist_read(&test_file, &test_data, ln_cnt, col_cnt, 0, true);
    mnist_close(&test_file);
    
    NeunetInit(net_core, train_data.lbl.length, test_data.lbl.length, ln_cnt, col_cnt, 1);
	NeunetRun(net_core, train_data.elem, train_data.lbl, train_idx, test_data .elem, test_data.lbl, MNIST_ORGN_SZ);
    NeunetResult(net_core);
    
    cout << neunet_chrono_time_point - chrono_begin << "ms" << endl;
    return EXIT_SUCCESS;
}