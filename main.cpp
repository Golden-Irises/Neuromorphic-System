/* 主様、おめでとうございます！
 * Congratulations, my master!
 */

#pragma once

#include <iostream>
#include "kokkoro"

#define kokkoro_deduce_flag true
#define kokkoro_learnrate   .4
#define kokkoro_bnrate      1e-5
#define kokkoro_len         0x0080

using namespace std;
using namespace kokkoro;

int main(int argc, char *argv[], char *envp[]) {
    cout << "Welcome back home, my master." << endl;
    auto tm_start = kokkoro_chrono_time_point;

    std::string sCSVRoot = "SRC\\ARCHIVE\\";

    #if kokkoro_deduce_flag
    KokkoroCore kkrCore {};
    #else
    uint64_t iTrainBatsz  = 110,
             iDeduceBatsz = 110;
    KokkoroANN kkrCore {iTrainBatsz, iDeduceBatsz};
    #endif

    // F1 18 neurons
    KokkoroAddLayer<LayerFC<18, kokkoro_learnrate>>(kkrCore,
        #if kokkoro_deduce_flag
        "",
        #endif
        sCSVRoot + "F1.csv"
    );
    KokkoroAddLayer<LayerBN<0., 1., kokkoro_bnrate, kokkoro_bnrate>>(kkrCore,
        #if kokkoro_deduce_flag
        "", "", "", "",
        #endif
        sCSVRoot + "B1_shift.csv",
        sCSVRoot + "B1_scale.csv",
        sCSVRoot + "B1_exp_mubeta.csv",
        sCSVRoot + "B1_exp_sigmaeps.csv"
    );
    KokkoroAddLayer<LayerAct<kokkoro_Sigmoid>>(kkrCore);
    // F2 12 neurons
    KokkoroAddLayer<LayerFC<12, kokkoro_learnrate>>(kkrCore,
        #if kokkoro_deduce_flag
        "",
        #endif
        sCSVRoot + "F2.csv"
    );
    KokkoroAddLayer<LayerBN<0., 1., kokkoro_bnrate, kokkoro_bnrate>>(kkrCore,
        #if kokkoro_deduce_flag
        "", "", "", "",
        #endif
        sCSVRoot + "B2_shift.csv",
        sCSVRoot + "B2_scale.csv",
        sCSVRoot + "B2_exp_mubeta.csv",
        sCSVRoot + "B2_exp_sigmaeps.csv"
    );
    KokkoroAddLayer<LayerAct<kokkoro_Sigmoid>>(kkrCore);
    // F3 6 neurons
    KokkoroAddLayer<LayerFC<6, kokkoro_learnrate>>(kkrCore,
        #if kokkoro_deduce_flag
        "",
        #endif
        sCSVRoot + "F3.csv"
    );
    KokkoroAddLayer<LayerBN<0., 1., kokkoro_bnrate, kokkoro_bnrate>>(kkrCore,
        #if kokkoro_deduce_flag
        "", "", "", "",
        #endif
        sCSVRoot + "B3_shift.csv",
        sCSVRoot + "B3_scale.csv",
        sCSVRoot + "B3_exp_mubeta.csv",
        sCSVRoot + "B3_exp_sigmaeps.csv"
    );
    KokkoroAddLayer<LayerAct<kokkoro_Sigmoid>>(kkrCore);
    // F4 5 neurons (Gaussian connections)
    KokkoroAddLayer<LayerFC<5, kokkoro_learnrate>>(kkrCore,
        #if kokkoro_deduce_flag
        "",
        #endif
        sCSVRoot + "F4.csv"
    );
    KokkoroAddLayer<LayerBN<0., 1., kokkoro_bnrate, kokkoro_bnrate>>(kkrCore,
        #if kokkoro_deduce_flag
        "", "", "", "",
        #endif
        sCSVRoot + "B4_shift.csv",
        sCSVRoot + "B4_scale.csv",
        sCSVRoot + "B4_exp_mubeta.csv",
        sCSVRoot + "B4_exp_sigmaeps.csv"
    );
    KokkoroAddLayer<LayerAct<kokkoro_Softmax>>(kkrCore);

    #if kokkoro_deduce_flag
    // cout << "Battery " << kkrCore.MasterMachineBattery("begin report") << endl;
    // deploy
    kkrCore.Run();
    // cout << "Battery " << kkrCore.MasterMachineBattery("end report") << endl;
    #else
    // TODO: load tain dataset
    kokkoro_set<kokkoro_matrix> setDataset, setTestset;
    kokkoro_set<uint64_t> setLblset, setTestLbl;
    kokkoro_csv_data_load(setDataset, setLblset, sCSVRoot + "train_data.csv", sCSVRoot + "train_lbl.csv");
    // TODO: load test dataset
    kokkoro_csv_data_load(setTestset, setTestLbl, sCSVRoot + "test_data.csv", sCSVRoot + "test_lbl.csv");
    auto setDatasetIdx = kokkoro_dataset_idx_init(setDataset.length);
    setDatasetIdx.shuffle();
    // train
    KokkoroTrainInit(kkrCore, setDataset.length, setTestset.length, kokkoro_data_arrsz, 1, 1, sCSVRoot + "acc_record.csv", sCSVRoot + "rc_record.csv");
	KokkoroTrain(kkrCore, setDataset, setLblset, setDatasetIdx, setTestset, setTestLbl, kokkoro_syb_cnt);
    KokkoroTrainResult(kkrCore);
    #endif


    cout << kokkoro_chrono_time_point - tm_start << "ms" << endl;
    return EXIT_SUCCESS;
}