KOKKORO_BEGIN

kokkoro_set<kokkoro_set<std::string>> csv_in(const std::string &file_path) {
    std::ifstream in(file_path);
    kokkoro_set<kokkoro_set<std::string>> ans;
    if (!in.is_open()) {
        in.close();
        return ans;
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string dat(buffer.str());
    in.close();
    ans.init(std::count(dat.begin(), dat.end(), csv_enter) + 1);
    auto from = dat.begin(),
         to   = from;
    for (auto i = 0ull; i < ans.length; ++i) {
        for (; to != dat.end(); ++to) if (*to == csv_enter) break;
        ans[i].init(std::count(from, to, csv_comma) + 1);
        auto j = 0ull;
        for (; from != to; ++from) if (*from == csv_comma) ++j;
        else ans[i][j].push_back(*from);
        ++from;
        ++to;
    }
    return ans;
}

bool csv_out(const kokkoro_set<kokkoro_set<std::string>> &output_strings, const std::string &file_path) {
    std::ofstream of_file;
    of_file.open(file_path, std::ios::out | std::ios::trunc);
    if (!of_file.is_open()) {
        return false;
        of_file.close();
    }
    for(auto i = 0ull; i < output_strings.size(); ++i) {
        for(auto j=0ull; j<output_strings[i].size(); ++j) of_file << output_strings[i][j] << csv_comma;
        of_file << std::endl;
    }
    of_file.close();
    return true;
}

void csv_print(const kokkoro_set<kokkoro_set<std::string>> &src) {
    for (auto i = 0ull; i < src.length; ++i) {
        for (auto j = 0ull; j < src[i].length; ++j) {
            std::cout << src[i][j];
            if (j + 1 < src[i].length) std::cout << csv_tab;
        }
        if (i + 1 < src.length) std::cout << csv_enter;
    }
}

KOKKORO_END