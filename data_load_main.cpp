/* Kokkoro data loading test file
ご武運を
*/

#pragma once

#include <iostream>
#include "kokkoro"

using namespace std;
using namespace kokkoro;

int main(int argc, char *argv[], char *envp[]) {
    cout << "Welcome back home, my master." << endl;
    auto tm_start = kokkoro_chrono_time_point;

    std::string sCSVRoot = "SRC\\ARCHIVE\\";

    kokkoro_set<kokkoro_matrix> setDataset, setTestset;
    kokkoro_set<uint64_t> setLblset, setTestLbl;
    auto setDatasetIdx = kokkoro_dataset_idx_init(setDataset.length);
    kokkoro_csv_data_load(setDataset, setLblset, sCSVRoot + "data_file_v.csv", sCSVRoot + "lbl_file_v.csv");
    kokkoro_csv_data_load(setTestset, setTestLbl, sCSVRoot + "data_test_v.csv", sCSVRoot + "lbl_test_v.csv");

    cout << kokkoro_chrono_time_point - tm_start << "ms" << endl;
    return 0;
}