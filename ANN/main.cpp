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

    auto a = new double [10](0);
    for (auto i = 0ull; i < 10; ++i) a[i] = i;
    std::memset(a, 2, 10 * sizeof(double) / sizeof(int));
    for (auto i = 0ull; i < 10; ++i) cout << a[i] << endl;
    delete[] a;
    a = nullptr;

    cout << neunet_chrono_time_point - chrono_begin << "ms" << endl;
    return EXIT_SUCCESS;
}