#pragma once

#define __POINT_COUNT__   20
#define __POINT_PATTERN__ '#'

#include <iostream>
#include "diagram"

using namespace std;
using namespace kokkoro;

int main(int argc, char *argv[], char *envp[]) {
    cout << "hello, world." << endl;
    auto tm = kokkoro_chrono_time_point;

    kokkoro_queue<double> que_test;
    auto area = std::acos(-1) / 10;

    std::thread set_data([&que_test, area]{ for (auto i = 0; i < 500; ++i) {
        que_test.en_queue(std::sin(i * area) * 20);
        _sleep(50);
        if (i == 499) i = 0;
    } });

    diagram_scroll_info<20, '#'> info;
    auto len = que_test.size();
    while (true) {
        diagram_scroll_add_point(info, que_test);
        diagram_scroll_update_axis(info);
        diagram_scroll_update_point(info);
    }
    set_data.join();
    diagram_scroll_flush(info);

    cout << kokkoro_chrono_time_point - tm << "ms" << endl;
    return 0;
}