/* データを準備完了ございます、主様！
 * Data is ready, my master!
 */
#pragma once

// get DCB message
#define kokkoro_dcb_msg     true
// DCB start flag number -> 0x3f
#define kokkoro_dcb_start   0x3f
// COM message width - 3 * 8bits = 3 bytes
#define kokkoro_data_segcnt 0x03
// sensor array size = 9
#define kokkoro_data_sensz  0x09
// bit size for each sensor equivalent data value
#define kokkoro_data_bitsz  0x02

#include <iostream>
#include "../../ANN/kokkoro"
#include "kokkoro_data"

using namespace kokkoro;
using namespace std;

int main(int argc, char *argv[], char *envp[]) {
    // DCB paramter
    kokkoro_dcb_handle handle_v {3,          // No.3 COM
                                 CBR_2400,   // 2400 Baudrate
                                 400,        // 400ms interval time period
                                 1024,       // 1024 bits buffer size for COM
                                 6,          // 6 databits
                                 ONESTOPBIT, // 1 stopbit(s)
                                 NOPARITY};  // no parity

    // Debugging
    kokkoro_dcb_startup(handle_v);

    // Thread function for sensor array data reading
    kokkoro_array_read(handle_v);

    kokkoro_dcb_shutdown(handle_v);

    return EXIT_SUCCESS;
}