#pragma once

#define neunet_eps  1e-8

#include <iostream>
#include "net_base"

using namespace std;
using namespace neunet;

template <uint64_t col_cnt> void test(double src[][col_cnt]) {
    constexpr auto ln_cnt = sizeof(src) / sizeof(src[0]);
    for (auto i = 0ull; i < ln_cnt; ++i) for (auto j = 0ull; j < col_cnt; ++j) cout << src[i][j] << endl;
}

int main(int argc, char *argv[], char *envp[]) {
    auto chrono_begin = neunet_chrono_time_point;
    cout << "hello, world." << endl;

    net_matrix test {{1, 2, 3},
                     {3, 4, 5}};
    cout << test << endl;

    cout << neunet_chrono_time_point - chrono_begin << "ms" << endl;
    return EXIT_SUCCESS;
}