/* データを準備完了ございます、主様！
 * Data is ready, my master!
 */
#pragma once

#define kokkoro_dcb_msg false

#include <iostream>
#include "../../ANN/kokkoro"
#include "kokkoro_data"

using namespace kokkoro;
using namespace std;

int main(int argc, char *argv[], char *envp[]) {
    kokkoro_dcb_handle test {3, CBR_9600};
    cout << test.data_cnt << endl;
    return EXIT_SUCCESS;
}