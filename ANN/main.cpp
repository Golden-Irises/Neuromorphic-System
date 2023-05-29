#pragma once

#define neunet_eps  DBL_EPSILON

#include <iostream>
#include "neunet"

using namespace std;
using namespace neunet;

template <uint64_t col_cnt> void test(double src[][col_cnt]) {
    constexpr auto ln_cnt = sizeof(src) / sizeof(src[0]);
    for (auto i = 0ull; i < ln_cnt; ++i) for (auto j = 0ull; j < col_cnt; ++j) cout << src[i][j] << endl;
}

int main(int argc, char *argv[], char *envp[]) {
    auto chrono_begin = neunet_chrono_time_point;
    cout << "hello, world." << endl;

    net_matrix in = {{1, 0, 1},
                     {2, 2, 2},
                     {0, 1, 1},
                     {1, 0, 0},
                     {1, 3, 1},
                     {3, 2, 3},
                     {0, 1, 3},
                     {2, 1, 3},
                     {2, 0, 2}};
    uint64_t caffe_ln_cnt = 0, caffe_col_cnt = 0, out_ln_cnt, out_col_cnt = 0;
    auto caffe_data = CaffeIdx(caffe_ln_cnt, caffe_col_cnt, out_ln_cnt, out_col_cnt, 3, 3, 3, 2, 2, 4, 1, 1, 0, 0);
    net_set<net_set<uint64_t>> elem_idx;
    auto out = PoolMax(in, caffe_data, 4, caffe_ln_cnt, elem_idx);
    cout << out << endl;
    cout << endl;
    auto grad = PoolGradMaxIn(out, 9, elem_idx);
    cout << im2col_to_tensor(grad, 3, 3) << endl;

    cout << neunet_chrono_time_point - chrono_begin << "ms" << endl;
    return EXIT_SUCCESS;
}