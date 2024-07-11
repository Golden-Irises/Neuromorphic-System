/* データを準備完了ございます、主様！
 * Data is ready, my master!
 */
#pragma once

// get DCB message
#define kokkoro_dcb_msg     false
// get DCB peak count
#define kokkoro_dcb_peak    false
// DCB start flag number -> 0x3f
#define kokkoro_dcb_start   0x3f
// COM message width - 3 * 8bits = 3 bytes
#define kokkoro_data_segcnt 0x03
// sensor array size = 9
#define kokkoro_data_arrsz  0x09
// bit size for each sensor equivalent data value
#define kokkoro_data_bitsz  0x02
// DCB array data saving mode, for csv
#define kokkoro_data_save   true

#include <iostream>
#include "../ANN/kokkoro_ann"
#include "kokkoro_data"

using namespace kokkoro;
using namespace std;

int main(int argc, char *argv[], char *envp[]) {
    // DCB paramter
    kokkoro_array_handle handle_v {3,          // No.3 COM
                                   CBR_2400,   // 2400 Baudrate
                                   400,        // 400ms interval time period
                                   1024,       // 1024 bits buffer size for COM
                                   6,          // 6 databits
                                   ONESTOPBIT, // 1 stopbit(s)
                                   NOPARITY};  // no parity
    // Debugging
    kokkoro_array_startup(handle_v
                          #if kokkoro_data_save
                          ,"testcode_data_v"
                          ,"testcode_lbl_v"
                          ,"SRC\\ARCHIVE\\"
                          #endif
                         );
    // read from DCB
    kokkoro_array_read_thread(handle_v);
    // data save
    kokkoro_array_save_thread(handle_v);
    // end
    kokkoro_array_shutdown(handle_v);

    return EXIT_SUCCESS;
}