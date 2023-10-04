#pragma once

#define KOKKORO_DCB_HDL_SZ 0x0010
#define KOKKORO_DCB_BUF_SZ 0x0080

#include <iostream>
#include "dcb.h"

using namespace kokkoro;
using namespace std;

int main(int argc, char *argv[], char *envp[]) {
    cout << "hello, world." << endl;

    dcb_hdl h_port = {0};
    
    dcb_startup(h_port,
                3,
                CBR_9600,
                2,
                ONESTOPBIT,
                NOPARITY);

    auto buf_sz = KOKKORO_DCB_BUF_SZ;
    char s_tmp[KOKKORO_DCB_BUF_SZ] = {0};
    while (buf_sz) {
        if (!dcb_read(h_port,s_tmp, KOKKORO_DCB_BUF_SZ)) break;
        cout << s_tmp << endl;
    }

    dcb_shutdown(h_port);

    return EXIT_SUCCESS;
}