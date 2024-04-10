#pragma once

#define kokkoro_learnrate   1e-2
#define kokkoro_bnrate      1e-5
#define kokkoro_len         0x80
// #define kokkoro_ann_wait_ms 0x32

#include <iostream>
#include "kokkoro"

using namespace std;
using namespace kokkoro;

#define morse_lbl_sz 0x04
#define morse_arr_sz 0x04

// return index set
kokkoro_set<size_t> morse_load_data(kokkoro_set<kokkoro_matrix> &data, kokkoro_set<size_t> &lbl, const std::string &path) {
    auto table = csv_in(path);
    data.init(table.length - 1);
    lbl.init(table.length - 1);
    kokkoro_set<size_t> ans(table.length - 1);
    for (auto i = 1ull; i < table.length; ++i) {
        auto lbl_idx = table[i].length - 1;
        auto dat_idx = i - 1;
        data[dat_idx] = kokkoro_matrix{lbl_idx, 1};
        for (auto j = 0ull; j < lbl_idx; ++j)
        data[dat_idx].index(j) = std::atof(table[i][j].c_str());
        switch (table[i][lbl_idx][0]) {
        case 'U': lbl[i] = 0; break;
        case 'C': lbl[i] = 1; break;
        case 'A': lbl[i] = 2; break;
        case 'S': lbl[i] = 3; break;
        default: break;
        }
        ans[dat_idx] = dat_idx;
    }
    return ans;
}

int main(int argc, char *argv[], char *envp[]) {
    std::string sCSVRoot = "SRC\\ARCHIVE\\";

    uint64_t iTrainBatsz  = 10,
             iDeduceBatsz = 10;
    KokkoroANN kkrCore {iTrainBatsz, iDeduceBatsz};

    // F1 4 neurons
    KokkoroAddLayer<LayerFC<4, kokkoro_learnrate>>(kkrCore, sCSVRoot + "morse_F1.csv");
    KokkoroAddLayer<LayerBN<0., 1., kokkoro_bnrate, kokkoro_bnrate>>(kkrCore,
        sCSVRoot + "morse_B1_shift.csv",
        sCSVRoot + "morse_B1_scale.csv"
    );
    KokkoroAddLayer<LayerAct<kokkoro_Sigmoid>>(kkrCore);
    // F2 6 neurons
    KokkoroAddLayer<LayerFC<6, kokkoro_learnrate>>(kkrCore, sCSVRoot + "morse_F2.csv");
    KokkoroAddLayer<LayerBN<0., 1., kokkoro_bnrate, kokkoro_bnrate>>(kkrCore,
        sCSVRoot + "morse_B2_shift.csv",
        sCSVRoot + "morse_B2_scale.csv"
    );
    KokkoroAddLayer<LayerAct<kokkoro_Sigmoid>>(kkrCore);
    // F3 8 neurons
    KokkoroAddLayer<LayerFC<8, kokkoro_learnrate>>(kkrCore, sCSVRoot + "morse_F3.csv");
    KokkoroAddLayer<LayerBN<0., 1., kokkoro_bnrate, kokkoro_bnrate>>(kkrCore,
        sCSVRoot + "morse_B3_shift.csv",
        sCSVRoot + "morse_B3_scale.csv"
    );
    KokkoroAddLayer<LayerAct<kokkoro_Sigmoid>>(kkrCore);
    // F4 4 neurons
    KokkoroAddLayer<LayerFC<4, kokkoro_learnrate>>(kkrCore, sCSVRoot + "morse_F4.csv");
    KokkoroAddLayer<LayerBN<0., 1., kokkoro_bnrate, kokkoro_bnrate>>(kkrCore,
        sCSVRoot + "morse_B4_shift.csv",
        sCSVRoot + "morse_B4_scale.csv"
    );
    KokkoroAddLayer<LayerAct<kokkoro_Softmax>>(kkrCore);

    kokkoro_set<kokkoro_matrix> train_data, test_data;
    kokkoro_set<size_t> train_lbl, test_lbl;
    auto train_idx = morse_load_data(train_data, train_lbl, sCSVRoot + "morsec_train.csv"),
         test_idx  = morse_load_data(test_data, test_lbl, sCSVRoot + "morsec_test.csv");
    KokkoroTrainInit(kkrCore, train_data.length, train_data.length, morse_arr_sz, 1, 1);
    KokkoroTrain(kkrCore, train_data, train_lbl, train_idx, train_data, train_lbl, morse_arr_sz);
    KokkoroTrainResult(kkrCore);

    return 0;
}