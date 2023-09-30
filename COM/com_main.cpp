#pragma once

#define DCB_FLAG    true

#define DCB_MSG     false
#define DCB_BUF_LEN 0x0010

#include <iostream>
#include "dcb.h"

using namespace kokkoro;
using namespace std;

int main(int argc, char *argv[], char *envp[]) {
    cout << "hello, world." << endl;

    // 端口句柄
    dcb_hdl h_port;
    // 打开端口
    if (!dcb_open_port(h_port, //端口句柄
                       3       // 端口
                       )) return EXIT_FAILURE;

    // 端口参数
    if (!dcb_port_params(h_port,     // 端口句柄
                         CBR_9600,   // 波特率 9600
                         20,         // 比特数 20
                         ONESTOPBIT, // 停止位 1
                         NOPARITY))  // 无奇偶校验
                         return EXIT_FAILURE;

    // Windows 串口消息
    while (DCB_FLAG) {
        // 端口数据
        char buffer [DCB_BUF_LEN];
        dcb_read(h_port, buffer, DCB_BUF_LEN);
        cout << buffer << endl;
        // 休眠 1s
        _sleep(1000);
    }

    // 销毁句柄
    if (dcb_close_port(h_port)) return EXIT_SUCCESS;
    return EXIT_FAILURE;
}