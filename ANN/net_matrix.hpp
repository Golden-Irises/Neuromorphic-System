NEUNET_BEGIN

template <uint64_t ln_cnt = 1, uint64_t col_cnt = 1>
struct net_matrix_base {
    static void matrix_srand_init() { std::srand(std::time(NULL)); }
    constexpr static uint64_t elem_cnt = ln_cnt * col_cnt;
    double value[elem_cnt] {};
    friend std::ostream &operator<<(std::ostream &os, const net_matrix_base &src) {
        for (auto i = 0ull; i < ln_cnt; ++i) {
            for (auto j = 0ull; j < col_cnt; ++j) {
                os << src.value[i * col_cnt + j];
                if (j + 1 < col_cnt) os<< '\t';
            }
            if (i + 1 < ln_cnt) os << '\n';
        }
        return os;
    }
};

template <uint64_t ln_cnt, uint64_t col_cnt>
void matrix_rand(net_matrix_base<ln_cnt, col_cnt> &src, double fst_rng = -1, double snd_rng = 1) {
    auto rng_dif = snd_rng - fst_rng;
    for (auto i = 0ull; i < src.elem_cnt; ++i) src.value[i] = (rng_dif / RAND_MAX) * std::rand() + fst_rng;
}

template <uint64_t ln_cnt, uint64_t col_cnt>
net_matrix_base<ln_cnt, col_cnt> matrix_add(const net_matrix_base<ln_cnt, col_cnt> &fst, const net_matrix_base<ln_cnt, col_cnt> &snd, bool sub = false) {
    net_matrix_base ans;
    for (auto i = 0ull; i < fst.elem_cnt; ++i) ans.value[i] = sub ? fst[i] - snd[i] : fst[i] + snd[i];
    return ans;
}



NEUNET_END