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
    cout << im2col_to_tensor(in, 3, 3) << endl << endl;
    uint64_t pad_ln = 0, pad_col = 0;
    auto pad_idx = im2col_pad_in_idx(pad_ln, pad_col, 3, 3, 3, 1, 2, 3, 4, 1, 2);
    net_matrix pad_ans {pad_ln * pad_col, 3};
    for (auto i = 0ull; i < pad_idx.length; ++i) pad_ans.index(pad_idx[i]) = in.index(i);
    cout << im2col_to_tensor(pad_ans, pad_ln, pad_col) << endl << endl;
    uint64_t crop_ln = 0, crop_col = 0;
    auto crop_idx = im2col_crop_out_idx(crop_ln, crop_col, pad_ln, pad_col, 3, 1, 2, 3, 4, 1, 2);
    net_matrix crop_ans {crop_ln * crop_col, 3};
    for (auto i = 0ull; i < pad_idx.length; ++i) crop_ans.index(i) = pad_ans.index(crop_idx[i]);
    cout << im2col_to_tensor(crop_ans, crop_ln, crop_col) << endl;

    cout << neunet_chrono_time_point - chrono_begin << "ms" << endl;
    return EXIT_SUCCESS;
}