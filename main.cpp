#pragma once

#define neunet_eps  DBL_EPSILON
#define nuenet_len  0x0080

#define MNIST_MSG   false

#include <iostream>

#include "ANN/neunet"
#include "SRC/MNIST/mnist.h"

using namespace std;
using namespace neunet;

int main(int argc, char *argv[], char *envp[]) {
    auto chrono_begin = neunet_chrono_time_point;
    cout << "hello, world." << endl;

    constexpr auto dNormLearnRate = .4,
                   dBNLearnRate   = 1e-5;

    NeunetCore net_core {80, 80};
    NeunetAddLayer(net_core, LayerConv<20, 5, 5, 1, 1, 0, 0, dNormLearnRate> {});
    
    mnist_data data;
    mnist_stream stream;
    
    std::string root = "E:\\VS Code project data\\MNIST\\";
    auto train_elem  = root + "train-images.idx3-ubyte",
         train_lbl   = root + "train-labels.idx1-ubyte",
         test_elem   = root + "t10k-images.idx3-ubyte",
         test_lbl    = root + "t10k-labels.idx1-ubyte";
    
    if (mnist_open(&stream, test_elem.c_str(), test_lbl.c_str())) std::printf("MNIST opened.\n");
    if (mnist_magic_verify(&stream)) std::printf("MNIST magic is valid.\n");
    if (mnist_qty_verify(&stream, &data)) std::printf("MNIST quatity is valid.\n");
    auto ln_cnt  = mnist_ln_cnt(&stream),
         col_cnt = mnist_col_cnt(&stream);
    mnist_read(&stream, &data, ln_cnt, col_cnt);
    mnist_close(&stream);
    // mnist_save_image("SRC/MNIST/IMG", &data, 10);    
    
    cout << neunet_chrono_time_point - chrono_begin << "ms" << endl;
    return EXIT_SUCCESS;
}