#pragma once

#include <iostream>
#include "neunet"

using namespace neunet;

struct net_matrix_block {
    constexpr static auto reg_sz        = 0x0004;
    constexpr static auto block_sz      = 0x0010;
    constexpr static auto block_elem_sz = block_sz * block_sz;

    double *block = nullptr;

    void value_copy(const net_matrix_block &src) { std::copy(src.block, src.block + block_elem_sz, block); }

    void value_move(net_matrix_block &&src) {
        block     = src.block;
        src.block = nullptr;
    }
    
    net_matrix_block() :
    block(new double[block_elem_sz](0.)) {}
    net_matrix_block(const net_matrix_block &src) { value_copy(src); }
    net_matrix_block(net_matrix_block &&src) { value_move(std::move(src)); }

    void elem_mul(const net_matrix_block &src) { for (auto i = 0; i < block_elem_sz; ++i) *(block + i) *= *(src.block + i); }
    
    template<bool divr = true>
    void elem_div(const net_matrix_block &src) { for (auto i = 0; i < block_elem_sz; ++i) if constexpr (divr) *(block + i) /= *(src.block + i); else *(block + i) = *(src.block + i) / *(block + i); }
    template<bool divr = true>
    void elem_div(double src) { for (auto i = 0; i < block_elem_sz; ++i) if constexpr (divr) *(block + i) /= src; else *(block + i) = src / *(block + i); }

    template<bool times = true>
    void elem_pow(const net_matrix_block &src) { for (auto i = 0; i < block_elem_sz; ++i) if constexpr (times) *(block + i) = std::pow(*(block + i), *(src.block + i)); else *(block + i) = std::pow(*(src.block + i), *(block + i)); }
    template<bool times = true>
    void elem_pow(double src) { for (auto i = 0; i < block_elem_sz; ++i) if constexpr (times) *(block + i) = std::pow(*(block + i), src); else *(block + i) = std::pow(src, *(block + i)); }

    void elem_rand(double fst_rng = -1, double snd_rng = 1) {
        if (fst_rng == snd_rng) return;
        if (snd_rng > snd_rng) std::swap(fst_rng, snd_rng);
        auto rng_dif = snd_rng - fst_rng;
        for (auto i = 0ull; i < block_elem_sz; ++i) *(block + i) = (rng_dif / RAND_MAX) * std::rand() + fst_rng;
    }

    template <bool sub = false>
    void broadcast_add(double src) {
        if constexpr (sub) src *= -1;
        for (auto i = 0; i < block_elem_sz; ++i) *(block + i) += src;
    }

    void reset() { while (block) {
        delete [] block;
        block = nullptr;
    } }

    ~net_matrix_block() { reset(); }

    void operator+=(const net_matrix_block &src) { for (auto i = 0; i < block_elem_sz; ++i) *(block + i) += *(src.block + i); }

    void operator-=(const net_matrix_block &src) { for (auto i = 0; i < block_elem_sz; ++i) *(block + i) -= *(src.block + i); }

    // bool operator==(const net_matrix_block &src) {
    //     for (auto i = 0; i < block_elem_sz; ++i) if (*(block + i) != *(src.block + i)) return false;
    //     return true;
    // }

    net_matrix_block operator*(const net_matrix_block &src) {
        net_matrix_block ans;
        for (auto i = 0; i < block_sz; ++i) {
            auto ans_ptr = ans.block + i * block_sz;
            auto lptr    = block + i * block_sz;
            __m256d ans_reg[reg_sz];
            for (auto x = 0; x < reg_sz; ++x) ans_reg[x] = _mm256_load_pd(ans_ptr + x * reg_sz);
            for (auto j = 0; j < block_sz; ++j) {
                __m256d lreg = _mm256_broadcast_sd(lptr + j);
                for (auto x = 0; x < reg_sz; ++x) ans_reg[x] = _mm256_add_pd(ans_reg[x], _mm256_mul_pd(lreg, _mm256_load_pd(src.block + j * block_sz + x * reg_sz)));
            }
            for (auto x = 0; x < reg_sz; ++x) _mm256_store_pd(ans_ptr + x * reg_sz, ans_reg[x]);
        }
        return ans;
    }

    net_matrix_block& operator=(const net_matrix_block &src) {
        value_copy(src);
        return *this;
    }
    net_matrix_block& operator=(net_matrix_block &&src) {
        value_move(std::move(src));
        return *this;
    }
};

struct __net_matrix {
    uint64_t ln_cnt  = 0,
             col_cnt = 0;

    uint64_t ln_block_cnt   = 0,
             col_block_cnt  = 0,
             elem_block_cnt = 0;

    net_matrix_block *val = nullptr;

    void elem_rand() { for (auto i = 0ull; i < elem_block_cnt; ++i) val[i].elem_rand(); }

    void value_assign(const __net_matrix &src) {
        ln_cnt         = src.ln_cnt;
        col_cnt        = src.col_cnt;
        ln_block_cnt   = src.ln_block_cnt;
        col_block_cnt  = src.col_block_cnt;
        elem_block_cnt = src.elem_block_cnt;
    }

    void value_copy(const __net_matrix &src) {
        if (src.elem_block_cnt != elem_block_cnt) {
            reset();
            val = new net_matrix_block[elem_block_cnt];
        }
        
        std::copy(src.val, src.val + elem_block_cnt, val);
    }

    void value_move(__net_matrix &&src) {
        value_assign(src);
        val     = src.val;
        src.val = nullptr;
        src.reset();
    }

    __net_matrix(uint64_t ln_cnt = 0, uint64_t col_cnt = 0) :
        ln_cnt(ln_cnt),
        col_cnt(col_cnt) {
        if (!(ln_cnt && col_cnt)) {
            ln_cnt  = 0;
            col_cnt = 0;
            return;
        }
        ln_block_cnt   = ln_cnt / net_matrix_block::block_sz + 1;
        col_block_cnt  = col_cnt / net_matrix_block::block_sz + 1;
        elem_block_cnt = ln_block_cnt * col_block_cnt;
        val = new net_matrix_block[elem_block_cnt];
    }
    __net_matrix(const __net_matrix &src) { value_copy(src); }
    __net_matrix(__net_matrix &&src) { value_move(std::move(src)); }

    void reset() {
        for (auto i = 0ull; i < elem_block_cnt; ++i) val[i].reset();
        while (val) {
            delete [] val;
            val = nullptr;
        }
    }
    
    ~__net_matrix() { reset(); }

    __net_matrix& operator=(const __net_matrix &src) { value_copy(src); return *this; }
    __net_matrix& operator=(__net_matrix &&src) { value_move(std::move(src)); return *this; }

    // bool operator==(const __net_matrix &src) {
    //     if (!(ln_cnt == src.ln_cnt && col_cnt == src.col_cnt)) return false;
        
    //     return true;
    // }

    __net_matrix operator*(const __net_matrix &src) {
        __net_matrix ans;
        ans.ln_cnt         = ln_cnt;
        ans.col_cnt        = src.col_cnt;
        ans.ln_block_cnt   = ln_block_cnt;
        ans.col_block_cnt  = src.col_block_cnt;
        ans.elem_block_cnt = ln_block_cnt * src.col_block_cnt;
        ans.val            = new net_matrix_block[ans.elem_block_cnt];
        for (auto i = 0ull; i < ln_block_cnt; ++i) {
            auto ans_ptr = ans.val + i * ans.col_block_cnt;
            auto fst_ptr = val + i * col_block_cnt;
            for (auto j = 0ull; j < src.col_block_cnt; ++j) for (auto k = 0ull; k < col_block_cnt; ++k) *(ans_ptr + k) = *(fst_ptr + k) * *(src.val + k * src.col_block_cnt + j);
        }
        return ans;
    }
};

using namespace std;

int main() {
    __net_matrix fst{500, 500}, snd{500, 500};
    fst.elem_rand(); snd.elem_rand();
    auto start = neunet_chrono_time_point;

    auto ans = fst * snd;

    cout << neunet_chrono_time_point - start << "ms" << endl;
    return 0;
}