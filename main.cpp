/* 主様、おめでとうございます！
 * Congratulations, my master!
 */

#pragma once

#include <iostream>
#include "kokkoro"

#define kokkoro_core_train  true

using namespace std;
using namespace kokkoro;

int main(int argc, char *argv[], char *envp[]) {
    cout << "Welcome back home, my master." << endl;
    auto tm_start = kokkoro_chrono_time_point;

    #if kokkoro_core_train

    

    #else



    #endif

    cout << kokkoro_chrono_time_point - tm_start << "ms" << endl;
    return EXIT_SUCCESS;
}