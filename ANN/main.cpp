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

    int cnt = 6;

    net_matrix sum {2, 2};
    net_queue<net_matrix> test;

    async_controller ctrl;

    async_pool pool {2};

    pool.add_task([&test, &sum, &ctrl, cnt] {
        for (auto i = 0; i < cnt; ++i) sum += test.de_queue();
        ctrl.thread_wake_one();
    });

    pool.add_task([&test, cnt] {
        for (auto i = 0; i < cnt; ++i) {
            net_matrix tmp {2, 2};
            for (auto j = 0; j < tmp.element_count; ++j) tmp.index(j) = i + 1;
            test.en_queue(tmp);
        }
    });

    ctrl.thread_sleep();
    cout << sum << endl;

    cout << neunet_chrono_time_point - chrono_begin << "ms" << endl;
    return EXIT_SUCCESS;
}