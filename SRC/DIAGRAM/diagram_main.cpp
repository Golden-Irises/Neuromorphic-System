#pragma once

#define __POINT_COUNT__ 20

#include <iostream>
#include "diagram"

using namespace std;
using namespace neunet;

int main(int argc, char *argv[], char *envp[]) {
    cout << "hello, world." << endl;
    auto tm = neunet_chrono_time_point;

    net_queue<double> que_test;
    auto area = std::acos(-1) / 10;

    std::thread set_data([&que_test, area]{ for (auto i = 0; i < 300; ++i) {
        que_test.en_queue(std::sin(i * area) * 20);
        _sleep(500);
    } });

    diagram_scroll_info<20, 'O'> info;
    auto len = que_test.size();
    while (true) {
        auto console_x = 0,
             console_y = 0;
        diagram_rect(console_x, console_y);
        diagram_scroll_add_point(info, que_test);
        diagram_scroll_update_axis(info, console_x, console_y);
        diagram_scroll_update_point(info, que_test);
    }
    diagram_scroll_flush(info);
    set_data.join();

    cout << neunet_chrono_time_point - tm << "ms" << endl;
    return 0;
}