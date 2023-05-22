#include <iostream>
#include <Windows.h>

// https://blog.csdn.net/u010835747/article/details/117357403

namespace nms {

int open_port(HANDLE h_port, int idx) {}

int close_port(HANDLE h_port) {}

int set_port_boudrate(HANDLE h_port, int rate) {}

int set_port_databits(HANDLE h_port, int bits) {}

int set_port_stopbits(HANDLE h_port, int bits) {}

int set_port_parity(HANDLE h_port, int parity) {}

int get_port_boudrate(HANDLE h_port) {}

int get_port_databits(HANDLE h_port) {}

int get_port_stopbits(HANDLE h_port) {}

int get_port_parity(HANDLE h_port) {}

int send_data(HANDLE h_port, const char *src) {}

int get_data(HANDLE h_port, char *dest, int len) {}

int init(HANDLE h_port, int idx, int rate, int databits, int stopbits, int parity) {}

int send_data(HANDLE h_port, const char *src) {}

int get_data(HANDLE h_port, char *dest, int len) {}

}

using namespace std;

int main(int argc, char *argv[], char *envp[]) {
    cout << "hello, world." << endl;
    return EXIT_SUCCESS;
}