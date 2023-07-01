#pragma once

#include <iostream>
#include "diagram"

using namespace std;
using namespace neunet;

int main(int argc, char *argv[], char *envp[]) {
    cout << "hello, world." << endl;
    auto tm = neunet_chrono_time_point;

    

    cout << neunet_chrono_time_point - tm << "ms" << endl;
    return 0;
}