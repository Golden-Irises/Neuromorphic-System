#pragma once

#define KOKKORO_DCB_HDL_SZ 0x0010
#define KOKKORO_DCB_BUF_SZ 0x0080

#include <iostream>
#include <bitset>
#include "dcb.h"

using namespace kokkoro;
using namespace std;

int main(int argc, char *argv[], char *envp[]) {
    cout << "hello, world." << endl;

    dcb_hdl h_port = {0};
    
    dcb_startup(h_port,
                3,
                CBR_9600,
                6,
                ONESTOPBIT,
                NOPARITY);

    char s_tmp[KOKKORO_DCB_BUF_SZ] = {0};
    auto buf_sz {0},
         buf_bt {0};
    do {
        buf_sz = dcb_read(h_port,s_tmp, KOKKORO_DCB_BUF_SZ);
        for (auto i = 0; i < buf_sz; ++i) cout << bitset<8>(s_tmp[i]) << ' '; cout << endl;
        cout << "Continue[Y/N]?";
        auto YoN = 'Y';
        cin >> YoN;
        if (YoN != 'N') buf_sz = 0;
    } while (buf_sz);

    dcb_shutdown(h_port);

    return EXIT_SUCCESS;
}