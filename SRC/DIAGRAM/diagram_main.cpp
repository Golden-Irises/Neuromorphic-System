#pragma once

#define __POINT_COUNT__ 5

#include <iostream>
#include "diagram"

using namespace std;
using namespace neunet;

int main(int argc, char *argv[], char *envp[]) {
    cout << "hello, world." << endl;
    auto tm = neunet_chrono_time_point;

    net_queue<double> que_test;
    que_test.en_queue(7.);
    que_test.en_queue(14.);
    que_test.en_queue(4.);
    que_test.en_queue(25.);
    que_test.en_queue(21.);
    que_test.en_queue(17.);
    que_test.en_queue(6.);
    que_test.en_queue(4.);
    que_test.en_queue(10.);
    que_test.en_queue(13.);

    diagram_scroll_info info;
    auto len = que_test.size();
    for (auto i = 0ull; i < len; ++i) {
        auto console_x = 0,
             console_y = 0;
        diagram_rect(console_x, console_y);
        diagram_scroll_add_point(info, que_test);
        diagram_scroll_update_axis(info, console_x, console_y);
        diagram_scroll_update_point(info, que_test);
        Sleep(2000);
    }
    diagram_scroll_flush(info);

    cout << neunet_chrono_time_point - tm << "ms" << endl;
    return 0;
}