#pragma once

#include <iostream>
#include <fstream>
#include <sstream>

#include "..\..\ANN\net_set"

NEUNET_BEGIN

net_set<net_set<std::string>> csv_in(const std::string &file_path) {
    std::ifstream in(file_path);
    net_set<net_set<std::string>> ans;
    if (!in.is_open()) {
        std::cerr << "Error opening file";
        in.close();
        return ans;
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string dat(buffer.str());
    in.close();
    ans.init(std::count(dat.begin(), dat.end(), '\n') + 1);
    auto from = dat.begin(),
         to   = from;
    for (auto i = 0ull; i < ans.length; ++i) {
        for (; to != dat.end(); ++to) if (*to == '\n') break;
        ans[i].init(std::count(from, to, ',') + 1);
        auto j = 0ull;
        for (; from != to; ++from) if (*from == ',') ++j;
        else ans[i][j].push_back(*from);
        ++from;
        ++to;
    }
    return ans;
}

void csv_out(const net_set<net_set<std::string>> &output_strings, const std::string &file_path) {
    std::ofstream of_file;
    of_file.open(file_path, std::ios::out|std::ios::trunc);
    for(auto i = 0ull; i < output_strings.size(); ++i) {
        for(auto j=0ull; j<output_strings[i].size(); ++j) of_file << output_strings[i][j] << ',';
        of_file << std::endl;
    }
    of_file.close();
}

void csv_print(const net_set<net_set<std::string>> &src) {
    for (auto i = 0ull; i < src.length; ++i) {
        for (auto j = 0ull; j < src[i].length; ++j) {
            std::cout << src[i][j];
            if (j + 1 < src[i].length) std::cout << '\t';
        }
        if (i + 1 < src.length) std::cout << '\n';
    }
}

NEUNET_END

using namespace std;
using namespace neunet;

int main(int argc, char *argv[], char *envp[]) {
    cout << "hello, world." << endl;

    auto in = csv_in("SRC/CSV/CSV_IN.csv");

    csv_print(in);
    
    net_set<net_set<string>> table = {
        {"ID", "Name", "Age", "Gender"},
        {"0" , "Doxy", "22" , "Female"},
        {"1" , "Roal", "31" , "Female"},
        {"2" , "Celu", "26" , "Male"  }
    };

    cout << endl;

    csv_out(table, "SRC/CSV/CSV_OUT.csv");

    return EXIT_SUCCESS;
}