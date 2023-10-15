/* データを準備完了ございます、主様！
 * Data is ready, my master!
 */
#pragma once

#define kokkoro_dcb_msg true

#include <iostream>
#include "../../ANN/kokkoro"
#include "kokkoro_data"

using namespace kokkoro;
using namespace std;

int main(int argc, char *argv[], char *envp[]) {
    kokkoro_dcb_handle v{3, CBR_2400, 400, 1024};
    kokkoro_dcb_data_save(v, "dataset_plus.csv");
    return EXIT_SUCCESS;
}