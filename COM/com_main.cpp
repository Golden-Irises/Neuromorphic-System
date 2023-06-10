#pragma once

#define DCB_FLAG    true

#define DCB_MSG     false
#define DCB_BUF_LEN 0x0010

#include <iostream>
#include <dcb.h>

using namespace std;

int main(int argc, char *argv[], char *envp[]) {
    cout << "hello, world." << endl;

    // 端口句柄
    HANDLE h_port {};
    // 打开端口
    if (!dcb_open_port(h_port,
                       /* 端口句柄 */
                       3,
                       /* 端口 */
                       50,
                       /* 读取每个 byte 后的最大延时
                        * 如果读取中超过该值那么将结束读取并返回读取缓冲区中的数据 */
                       10,
                       /* 读取每个 byte 数据的演示 */
                       50,
                       /* 读取整个数据后的延时 */
                       10,
                       /* 写入每个 byte 的延时 */
                       50
                       /* 写入整个数据后的延时 */)) return EXIT_FAILURE;

    // 端口参数
    if (!dcb_port_params(h_port,    // 端口句柄
                         19200,     // 波特率 19200
                         20,        // 比特数 20
                         1,         // 停止位 1
                         NOPARITY)) // 无奇偶校验
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