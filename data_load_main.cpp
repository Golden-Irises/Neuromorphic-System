/* Kokkoro data loading test file
ご武運を
*/

#pragma once

#include <iostream>
#include "kokkoro"

using namespace std;
using namespace kokkoro;

struct file_out {
    std::ofstream out_s;
};

bool file_open(file_out &file_stream, const std::string &file_dir, const std::string &file_name) {
    file_stream.out_s.open(file_dir + file_name, std::ios::out | std::ios::trunc);
    return file_stream.out_s.is_open();
}

void file_close(file_out &file_stream) { file_stream.out_s.close(); }

void file_write(file_out &file_stream, const std::string &content) { file_stream.out_s << content; }

int main(int argc, char *argv[], char *envp[]) {
    cout << "Welcome back home, my master." << endl;
    auto tm_start = kokkoro_chrono_time_point;

    std::string sCSVRoot = "SRC\\ARCHIVE\\";

    // kokkoro_set<kokkoro_matrix> setDataset, setTestset;
    // kokkoro_set<uint64_t> setLblset, setTestLbl;
    // auto setDatasetIdx = kokkoro_dataset_idx_init(setDataset.length);
    // kokkoro_csv_data_load(setDataset, setLblset, sCSVRoot + "data_file_v.csv", sCSVRoot + "lbl_file_v.csv");
    // kokkoro_csv_data_load(setTestset, setTestLbl, sCSVRoot + "data_test_v.csv", sCSVRoot + "lbl_test_v.csv");

    file_out file;
    file_open(file, sCSVRoot, "test_file.csv");

    file_write(file, "name,gender,age\n");
    file_write(file, "Jim,male,16");

    file_close(file);

    cout << kokkoro_chrono_time_point - tm_start << "ms" << endl;
    return 0;
}