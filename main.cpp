/* 主様、おめでとうございます！
 * Congratulations, my master!
 */

#pragma once

#pragma comment(lib, "Kernel32.lib")
#include <windows.h>

#include <iostream>
#include "ANN/kokkoro"

using namespace std;

int main(int argc, char *argv[], char *envp[]) {
    cout << "Welcome back home, my master." << endl;
    auto tm_start = kokkoro_chrono_time_point;

    // powercfg /batteryreport /output "..\html_file_path.html"
    SYSTEM_POWER_STATUS power_stat;
    GetSystemPowerStatus(&power_stat);
    cout << int(power_stat.BatteryLifePercent) << '%' << endl;

    cout << kokkoro_chrono_time_point - tm_start << "ms" << endl;
    return EXIT_SUCCESS;
}