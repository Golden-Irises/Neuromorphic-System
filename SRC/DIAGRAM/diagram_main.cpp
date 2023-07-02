#pragma once

#define __POINT_COUNT__ 0

#include <iostream>
#include "diagram"

using namespace std;
using namespace neunet;

int main(int argc, char *argv[], char *envp[]) {
    cout << "hello, world." << endl;
    auto tm = neunet_chrono_time_point;

    net_queue<diagram_pt> que_test;
    diagram_scroll_info info;
    info.max_y = 100;
    info.min_y = 10;
    diagram_scroll_print(info, que_test);

    cout << neunet_chrono_time_point - tm << "ms" << endl;
    return 0;
}