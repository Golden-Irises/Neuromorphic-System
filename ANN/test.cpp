#pragma once

#include <iostream>
#include "net_matrix"

using namespace std;
using namespace neunet;

net_matrix_base mul_gemm(const net_matrix_base &fst, const net_matrix_base &snd) {
    net_matrix_base ans {fst.ln_cnt, snd.col_cnt};
    for (auto i = 0ull; i < fst.ln_cnt; ++i) {
        auto ans_ptr = ans.ptr + i * snd.col_cnt;
        for (auto j = 0ull; j < fst.col_cnt; ++j) {
            auto coe_val = fst[i][j];
            auto snd_ptr = snd.ptr + j * snd.col_cnt;
            for (auto k = 0ull; k < snd.col_cnt; ++k) *(ans_ptr + k) += coe_val * *(snd_ptr + k);
        }
    }
    return ans;
}

void mul_simd(net_matrix_base &ans, const net_matrix_base &fst, const net_matrix_base &snd, uint64_t fst_ln, uint64_t snd_col, uint64_t fst_col) {
    auto fst_ln_blk  = fst_ln + neunet_blk_sz,
         fst_col_blk = fst_col + neunet_blk_sz,
         snd_col_blk = snd_col + neunet_blk_sz;
    auto blk_flag = true;
    if (snd_col_blk > snd.col_cnt) {
        snd_col_blk = snd.col_cnt;
        blk_flag    = false;
    }
    if (fst_ln_blk > fst.ln_cnt) fst_ln_blk = fst.ln_cnt;
    if (fst_col_blk > fst.col_cnt) fst_col_blk = fst.col_cnt;
	for (auto i = fst_ln; i < fst_ln_blk; ++i) if (blk_flag) for (auto j = snd_col; j < snd_col_blk; j += neunet_blk_sz) {
		__m256d ans_reg[neunet_unroll];
		for (auto x = 0; x < neunet_unroll; ++x) ans_reg[x] = _mm256_load_pd(ans.ptr + i * snd.col_cnt + j + x * neunet_reg_sz);

		for (auto k = fst_col; k < fst_col_blk; ++k) {
			__m256d fst_reg = _mm256_broadcast_sd(fst.ptr + i * fst.col_cnt + k);
			for (auto x = 0; x < neunet_unroll; ++x) ans_reg[x] = _mm256_add_pd(ans_reg[x], _mm256_mul_pd(fst_reg, _mm256_load_pd(snd.ptr + k * snd.col_cnt + j + x * neunet_reg_sz)));
		}

		for (auto x = 0; x < neunet_unroll; ++x) _mm256_store_pd(ans.ptr + i * snd.col_cnt + j + x * neunet_reg_sz, ans_reg[x]);
	} else for (auto j = fst_col; j < fst_col_blk; ++j) {
        auto coe = fst[i][j];
        for (auto k = snd_col; k < snd_col_blk; ++k) ans[i][k] += coe * snd[j][k];
    }
}
void _mul_simd(net_matrix_base &ans, const net_matrix_base &fst, const net_matrix_base &snd, uint64_t fst_ln, uint64_t snd_col, uint64_t fst_col) {
    auto fst_ln_blk  = fst_ln + neunet_blk_sz,
         fst_col_blk = fst_col + neunet_blk_sz,
         snd_col_blk = snd_col + neunet_blk_sz;
    auto blk_flag = true;
    if (snd_col_blk > snd.col_cnt) {
        snd_col_blk = snd.col_cnt;
        blk_flag    = false;
    }
    if (fst_ln_blk > fst.ln_cnt) fst_ln_blk = fst.ln_cnt;
    if (fst_col_blk > fst.col_cnt) fst_col_blk = fst.col_cnt;
	for (auto i = fst_ln; i < fst_ln_blk; ++i) {
        auto ans_ptr = ans.ptr + i * snd.col_cnt;
        if (blk_flag) for (auto j = snd_col; j < snd_col_blk; j += neunet_blk_sz) {
		    __m256d ans_reg[neunet_unroll];
		    for (auto x = 0; x < neunet_unroll; ++x) ans_reg[x] = _mm256_load_pd(ans_ptr + j + x * neunet_reg_sz);
            
		    for (auto k = fst_col; k < fst_col_blk; ++k) {
			    __m256d fst_reg = _mm256_broadcast_sd(fst.ptr + i * fst.col_cnt + k);
			    for (auto x = 0; x < neunet_unroll; ++x) ans_reg[x] = _mm256_add_pd(ans_reg[x], _mm256_mul_pd(fst_reg, _mm256_load_pd(snd.ptr + k * snd.col_cnt + j + x * neunet_reg_sz)));
		    }
            
		    for (auto x = 0; x < neunet_unroll; ++x) _mm256_store_pd(ans_ptr + j + x * neunet_reg_sz, ans_reg[x]);
	    } else for (auto j = fst_col; j < fst_col_blk; ++j) {
            auto coe_val = fst[i][j];
            auto snd_ptr = snd.ptr + j * snd.col_cnt;
            for (auto k = snd_col; k < snd_col_blk; ++k) *(ans_ptr + k) += coe_val * *(snd_ptr + k);
        }
        ans_ptr = nullptr;
    }
}
net_matrix_base mul_simd(const net_matrix_base &fst, const net_matrix_base &snd) {
    if (fst.col_cnt != snd.ln_cnt) return {};
    net_matrix_base ans {fst.ln_cnt, snd.col_cnt};
    for (auto i = 0ull; i < fst.ln_cnt; i += neunet_blk_sz) for (auto j = 0ull; j < snd.col_cnt; j += neunet_blk_sz) for (auto k = 0ull; k < fst.col_cnt; k += neunet_blk_sz) _mul_simd(ans, fst, snd, i, j, k);
    return ans;
}

int main() {
    net_matrix_base fst{1000, 1000}, snd{1000, 1000};
    matrix_rand(fst); matrix_rand(snd);
    auto start = neunet_chrono_time_point;

    // auto g_ans = mul_gemm(fst, snd);
    auto s_ans = mul_simd(fst, snd);
    // cout << (s_ans == g_ans) << endl;

    cout << neunet_chrono_time_point - start << "ms" << endl;
    return 0;
}