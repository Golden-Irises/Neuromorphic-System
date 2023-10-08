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
    char test[] = {0x1d, 0x3a, 0x14, 0x3f};
    for (auto i = 0; i < 4; ++i) cout << '[' << bitset<8>(test[i]) << ']'; cout << endl;
    int deci[9] = {0};
    kokkoro_dcb_vect_mask(deci, test);
    for (auto i = 0; i < 9; i += 3) {
        cout << "[  ";
        for (auto j = 3; j; --j) cout << ' ' << deci[i + j - 1];
        cout << ']';
    }
    cout << "[idle]" << endl;
    return EXIT_SUCCESS;
}