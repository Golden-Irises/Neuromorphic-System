#pragma once

#include <iostream>
#include "csv"

using namespace std;
using namespace kokkoro;

int main(int argc, char *argv[], char *envp[]) {
    cout << "hello, world." << endl;

    auto in = csv_in("SRC/ARCHIVE/CSV_IN.csv");

    csv_print(in);
    
    kokkoro_set<kokkoro_set<string>> table = {
        {"ID", "Name", "Age", "Gender"},
        {"0" , "Doxy", "22" , "Female"},
        {"1" , "Roal", "31" , "Female"},
        {"2" , "Celu", "26" , "Male"  }
    };

    cout << endl;

    csv_out(table, "SRC/ARCHIVE/CSV_OUT.csv");

    return EXIT_SUCCESS;
}