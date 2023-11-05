/* お疲れ様でした、主様。
 * God job, my master.
 */

#pragma once

#define kokkoro_eps  DBL_EPSILON
#define kokkoro_len  0x0080

#define kokkoro_arg  false

#define MNIST_MSG   false

#include <iostream>

#include "../SRC/CSV/csv"
#include "kokkoro_ann"
#include "../SRC/MNIST/mnist.h"

#define LEARN_RATE       .4     // 1e-5
#define BN_LEARN_RATE    1e-5

using namespace std;
using namespace kokkoro;

int main(int argc, char *argv[], char *envp[]) {
    auto chrono_begin = kokkoro_chrono_time_point;
    cout << "Kokkoro is working, my master..." << endl;

    KokkoroANN kokkoro_ann {125, 125};

    string root = "SRC\\ARCHIVE\\";
    KokkoroAddLayer<LayerConv<20, 5, 5, 1, 1, 0, 0, LEARN_RATE>>(kokkoro_ann, root + "C0.csv");
    KokkoroAddLayer<LayerBN<0., 1., BN_LEARN_RATE, BN_LEARN_RATE>>(kokkoro_ann, root + "B0_shift.csv", root + "B0_scale.csv");
    // KokkoroAddLayer<LayerBias<LEARN_RATE>>(kokkoro_ann, root + "S0.csv");
    KokkoroAddLayer<LayerAct<kokkoro_ReLU>>(kokkoro_ann);
    KokkoroAddLayer<LayerPool<kokkoro_avg_pool, 2, 2, 2, 2>>(kokkoro_ann);
    KokkoroAddLayer<LayerConv<50, 5, 5, 1, 1, 0, 0, LEARN_RATE>>(kokkoro_ann, root + "C2.csv");
    KokkoroAddLayer<LayerBN<0., 1., BN_LEARN_RATE, BN_LEARN_RATE>>(kokkoro_ann, root + "B2_shift.csv", root + "B2_scale.csv");
    // KokkoroAddLayer<LayerBias<LEARN_RATE>>(kokkoro_ann, root + "S2.csv");
    KokkoroAddLayer<LayerAct<kokkoro_ReLU>>(kokkoro_ann);
    KokkoroAddLayer<LayerPool<kokkoro_avg_pool, 2, 2, 2, 2>>(kokkoro_ann);
    KokkoroAddLayer<LayerFlat>(kokkoro_ann);
    KokkoroAddLayer<LayerFC<500, LEARN_RATE>>(kokkoro_ann, root + "F4.csv");
    KokkoroAddLayer<LayerBN<0., 1., BN_LEARN_RATE, BN_LEARN_RATE>>(kokkoro_ann, root + "B4_shift.csv", root + "B4_scale.csv");
    // KokkoroAddLayer<LayerBias<LEARN_RATE>>(kokkoro_ann, root + "S4.csv");
    KokkoroAddLayer<LayerAct<kokkoro_Sigmoid>>(kokkoro_ann);
    KokkoroAddLayer<LayerFC<10, LEARN_RATE>>(kokkoro_ann, root + "F5.csv");
    KokkoroAddLayer<LayerBN<0., 1., BN_LEARN_RATE, BN_LEARN_RATE>>(kokkoro_ann, root + "B5_shift.csv", root + "B5_scale.csv");
    // KokkoroAddLayer<LayerBias<LEARN_RATE>>(kokkoro_ann, root + "S5.csv");
    KokkoroAddLayer<LayerAct<kokkoro_Softmax>>(kokkoro_ann);

    root = "E:\\VS Code project data\\MNIST\\";
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
	auto train_idx = kokkoro_dataset_idx_init(train_data.elem.length);
    
    auto test_data_load = mnist_open(&test_file, test_elem.c_str(), test_lbl.c_str()) &&
                          mnist_magic_verify(&test_file) &&
                          mnist_qty_verify(&test_file, &test_data);
    if (!test_data_load) return EXIT_FAILURE;
    mnist_ln_cnt(&test_file);
    mnist_col_cnt(&test_file);
    mnist_read(&test_file, &test_data, ln_cnt, col_cnt, 0, true);
    mnist_close(&test_file);
    
    KokkoroTrainInit(kokkoro_ann, train_data.lbl.length, test_data.lbl.length, ln_cnt, col_cnt, 1);
	KokkoroTrain(kokkoro_ann, train_data.elem, train_data.lbl, train_idx, test_data.elem, test_data.lbl, MNIST_ORGN_SZ);
    KokkoroTrainResult(kokkoro_ann);
    
    cout << kokkoro_chrono_time_point - chrono_begin << "ms" << endl;
    return EXIT_SUCCESS;
}