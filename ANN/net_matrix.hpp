NEUNET_BEGIN

struct net_matrix_base {
    uint64_t ln_cnt   = 0,
             col_cnt  = 0,
             elem_cnt = 0;

    double *ptr = nullptr;

    void value_assign(const net_matrix_base &src) {
        ln_cnt   = src.ln_cnt;
        col_cnt  = src.col_cnt;
        elem_cnt = src.elem_cnt;
    }

    void value_copy(const net_matrix_base &src) {
        if (this == &src) return;
        if (elem_cnt == src.elem_cnt) {
            if (!elem_cnt) return;
            ln_cnt  = src.ln_cnt;
            col_cnt = src.col_cnt;
        } else {
            reset();
            if (src.elem_cnt) value_assign(src);
            else return;
            ptr = new double[elem_cnt];
        }
        std::copy(src.ptr, src.ptr + elem_cnt, ptr);
    }

    void value_move(net_matrix_base &&src) {
        if (this == &src) return;
        if (ptr) reset();
        value_assign(src);
        ptr     = src.ptr;
        src.ptr = nullptr;
        src.reset();
    }

    net_matrix_base(uint64_t ln_cnt = 0, uint64_t col_cnt = 0) { if (ln_cnt && col_cnt) {
        this->ln_cnt  = ln_cnt;
        this->col_cnt = col_cnt;
        elem_cnt      = ln_cnt * col_cnt;
        ptr           = new double[elem_cnt](0);
    } }
    net_matrix_base(const net_matrix_base &src) { value_copy(src); }
    net_matrix_base(net_matrix_base &&src) { value_move(std::move(src)); }

    void clear() { for (auto i = 0ull; i < elem_cnt; ++i) ptr[i] = 0; }

    bool is_matrix() const { return ptr && ln_cnt && col_cnt; }

    void reset() {
        while (ptr) {
            delete [] ptr;
            ptr = nullptr;
        }
        ln_cnt   = 0;
        col_cnt  = 0;
        elem_cnt = 0;
    }

    ~net_matrix_base() { reset(); }

    net_matrix_base &operator=(const net_matrix_base &src) {
        value_copy(src);
        return *this;
    }
    net_matrix_base &operator=(net_matrix_base &&src) {
        value_move(std::move(src));
        return *this;
    }

    bool operator==(const net_matrix_base &src) const { return std::equal(ptr, ptr + elem_cnt, src.ptr); }

    double *operator[](uint64_t ln) const { return ptr + ln * col_cnt; }

    friend std::ostream &operator<<(std::ostream &os, const net_matrix_base &src) { for (auto i = 0ull; i < src.ln_cnt; ++i) {
        for (auto j = 0ull; j < src.col_cnt; ++j) {
            os << src.ptr[i * src.col_cnt + j];
            if (j + 1 < src.col_cnt) os << '\t';
        }
        if (i + 1 < src.ln_cnt) os << '\n';
    } return os; }
};

void matrix_rand(net_matrix_base &src, double fst_rng = -1, double snd_rng = 1) { if (src.elem_cnt) {
    if (fst_rng == snd_rng) return;
    if (snd_rng > snd_rng) std::swap(fst_rng, snd_rng);
    auto rng_dif = snd_rng - fst_rng;
    for (auto i = 0ull; i < src.elem_cnt; ++i) src.ptr[i] = (rng_dif / RAND_MAX) * std::rand() + fst_rng;
} }

void matrix_abs(net_matrix_base &src) { for (auto i = 0ull; i < src.elem_cnt; ++i) src.ptr[i] = std::abs(src.ptr[i]); }

uint64_t matrix_pad_dir_cnt(uint64_t prev_pad, uint64_t rear_pad, uint64_t dir_cnt, uint64_t dir_dist) { return prev_pad + rear_pad + dir_cnt + (dir_cnt - 1) * dir_dist; }

uint64_t matrix_crop_dir_cnt(uint64_t prev_crop, uint64_t rear_crop, uint64_t dir_cnt, uint64_t dir_dist) { return (dir_cnt - (prev_crop + rear_crop) + dir_dist) / (dir_dist + 1); }

double matrix_elem_sum(const net_matrix_base &src) { return std::accumulate(src.ptr + 1, src.ptr + src.elem_cnt, *src.ptr); }

bool matrix_shape_verify(const net_matrix_base &src, uint64_t ln_cnt, uint64_t col_cnt) { return ln_cnt == src.ln_cnt && col_cnt == src.col_cnt; }

template <bool sub = false>
void matrix_add(net_matrix_base &src, const net_matrix_base &val) {
    if (!matrix_shape_verify(src, val.ln_cnt, val.col_cnt)) return;
    for (auto i = 0ull; i < src.elem_cnt; ++i) if constexpr (sub) src.ptr[i] -= val.ptr[i];
    else src.ptr[i] += val.ptr[i];
}

template <bool sub = false>
void matrix_broadcast_add(net_matrix_base &src, double param) {
    for (auto i = 0ull; i < src.elem_cnt; ++i) if constexpr (sub) src.ptr[i] -= param;
    else src.ptr[i] += param;
}

void matrix_mul(net_matrix_base &ans, const net_matrix_base &fst, const net_matrix_base &snd, uint64_t fst_ln, uint64_t snd_col, uint64_t fst_col) {
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
void matrix_mul(net_matrix_base &ans, const net_matrix_base &fst, const net_matrix_base &snd_vect) {
    uint64_t ln  = 0,
             col = 0;
    
    auto blk_ln_cnt = fst.ln_cnt / neunet_byte_sz;
    
    for(ln = 0ull; ln < blk_ln_cnt; ++ln) {
        double blk_val[neunet_byte_sz] = {};
        
        auto curr_ln = ln * neunet_byte_sz;
        
        for(col = 0ull; col < fst.col_cnt; ++col) for (auto x = 0ull; x < neunet_byte_sz; ++x) blk_val[x] += fst.ptr[(curr_ln + x) * fst.col_cnt + col] * snd_vect.ptr[col];
        
        for (auto x = 0; x < neunet_byte_sz; ++x) ans.ptr[curr_ln + x] += blk_val[x];
    }
    
    for(ln *= neunet_byte_sz; ln < fst.ln_cnt; ++ln) for(col = 0; col < fst.col_cnt; ++col) ans.ptr[ln] += fst.ptr[ln * fst.col_cnt + col] * snd_vect.ptr[col];
}
net_matrix_base matrix_mul(const net_matrix_base &fst, const net_matrix_base &snd) {
    if (fst.col_cnt != snd.ln_cnt) return {};
    net_matrix_base ans {fst.ln_cnt, snd.col_cnt};
    if (snd.col_cnt == 0x0001) matrix_mul(ans, fst, snd);
    else for (auto i = 0ull; i < fst.ln_cnt; i += neunet_blk_sz) for (auto j = 0ull; j < snd.col_cnt; j += neunet_blk_sz) for (auto k = 0ull; k < fst.col_cnt; k += neunet_blk_sz) matrix_mul(ans, fst, snd, i, j, k);
    return ans;
}
void matrix_mul(net_matrix_base &src, double param) { for (auto i = 0ull; i < src.elem_cnt; ++i) src.ptr[i] *= param; }

void matrix_elem_mul(net_matrix_base &src, const net_matrix_base &val) {
    if (!matrix_shape_verify(src, val.ln_cnt, val.col_cnt)) return;
    for (auto i = 0ull; i < src.elem_cnt; ++i) src.ptr[i] *= val.ptr[i];
}

template <bool param_divr = true>
void matrix_elem_div(net_matrix_base &src, double param) {
    for (auto i = 0ull; i < src.elem_cnt; ++i) if constexpr (param_divr) src.ptr[i] /= param;
    else src.ptr[i] = param / src.ptr[i];
}
void matrix_elem_div(net_matrix_base &src, const net_matrix_base &val) {
    if (!matrix_shape_verify(src, val.ln_cnt, val.col_cnt)) return;
    for (auto i = 0ull; i < src.elem_cnt; ++i) src.ptr[i] /= val.ptr[i];
}

template <bool param_time = true>
void matrix_elem_pow(net_matrix_base &src, double param) {
    for (auto i = 0ull; i < src.elem_cnt; ++i) if constexpr (param_time) src.ptr[i] = std::pow(src.ptr[i], param);
    else src.ptr[i] = std::pow(param, src.ptr[i]);
}

net_matrix_base matrix_transpose(const net_matrix_base &src) {
    net_matrix_base ans {src.col_cnt, src.ln_cnt};
    for (auto i = 0ull; i < src.ln_cnt; ++i) for (auto j = 0ull; j < src.col_cnt; ++j) ans[j][i] = src[i][j];
    return ans;
}

void matrix_clear(net_matrix_base &src) { for (auto i = 0ull; i < src.elem_cnt; ++i) src.ptr[i] = 0; }

class net_matrix {
public:
    net_matrix(uint64_t ln_cnt = 0, uint64_t col_cnt = 0) :
        proto(ln_cnt, col_cnt) {}
    net_matrix(std::initializer_list<std::initializer_list<double>> src) : net_matrix(src.size(), src.begin()->size())  {
        if (!proto.elem_cnt) return;
        auto elem_cnt = 0ull;
        for (auto row_tmp : src) for (auto col_tmp : row_tmp) proto.ptr[elem_cnt++] = col_tmp;
        if (proto.elem_cnt != elem_cnt) proto.reset();
    }

    void elem_rand(double fst_rng = -1, double snd_rng = 1) { matrix_rand(proto, fst_rng, snd_rng); }

    uint64_t ln_cnt() const { return proto.ln_cnt; }

    uint64_t col_cnt() const { return proto.col_cnt; }

    uint64_t elem_cnt() const { return proto.elem_cnt; }

    double elem_sum() const { return matrix_elem_sum(proto); }

    void abs() { matrix_abs(proto); }

    bool shape_verify(uint64_t ln_cnt, uint64_t col_cnt) const { return matrix_shape_verify(proto, ln_cnt, col_cnt); }
    bool shape_verify(const net_matrix &src) const { return shape_verify(src.proto.ln_cnt, src.proto.col_cnt); }

    bool reshape(uint64_t ln_cnt, uint64_t col_cnt) {
        auto elem_cnt = ln_cnt * col_cnt;
        if (elem_cnt != proto.elem_cnt) return false;
        proto.ln_cnt  = ln_cnt;
        proto.col_cnt = col_cnt;
        return true;
    }
    bool reshape(const net_matrix &src) { return reshape(src.proto.ln_cnt, src.proto.col_cnt); }

    bool is_matrix() const { return proto.is_matrix(); }

    template <bool sub = false>
    void broadcast_add(double param) { matrix_broadcast_add<sub>(proto, param); }

    net_matrix transposition() const {
        net_matrix ans;
        ans.proto = matrix_transpose(proto);
        return ans;
    }

    void elem_wise_mul(const net_matrix &src) { matrix_elem_mul(proto, src.proto); }
    
    void elem_wise_div(const net_matrix &src) { matrix_elem_div(proto, src.proto); }

    template <bool divr = true>
    void elem_wise_div(double src) { matrix_elem_div<divr>(proto, src); }

    template <bool times = true>
    void elem_wise_pow(double src) { matrix_elem_pow<times>(proto, src); }

    double &index(uint64_t idx) const { return proto.ptr[idx]; }

    void clear() { matrix_clear(proto); }

    static net_matrix mul(const net_matrix &fst, const net_matrix &snd) {
        net_matrix ans {fst.proto.ln_cnt, snd.proto.col_cnt};
        for (auto i = 0ull; i < fst.proto.ln_cnt; ++i) for (auto j = 0ull; j < fst.proto.col_cnt; ++j) {
            auto coe = fst.proto[i][j];
            for (auto k = 0ull; k < snd.proto.col_cnt; ++k) ans.proto[i][k] += coe * snd.proto[j][k];
        }
        return ans;
    }

    static net_matrix sigma(const net_set<net_matrix> &src) {
        net_matrix ans;
        ans.proto.ln_cnt   = src[0].proto.ln_cnt;
        ans.proto.col_cnt  = src[0].proto.col_cnt;
        ans.proto.elem_cnt = src[0].proto.elem_cnt;

        __m256d ans_reg[neunet_unroll];
        auto ans_ptr  = new double [ans.proto.elem_cnt];
        auto idx_temp = 0ull;
        while (idx_temp < ans.proto.elem_cnt) {
            auto idx_next = idx_temp + neunet_blk_sz;

            if (idx_next > ans.proto.elem_cnt) {
                for (auto i = idx_temp; i < ans.proto.elem_cnt; ++i) {
                    ans_ptr[i] = src[0].proto.ptr[i];
                    for (auto j = 1ull; j < src.length; ++j) ans_ptr[i] += src[j].proto.ptr[i];
                }
                break;
            }

            for (auto x = 0; x < neunet_unroll; ++x) ans_reg[x] = _mm256_load_pd(src[0].proto.ptr + idx_temp + x * neunet_reg_sz);

            for (auto i = 1ull; i < src.length; ++i) for (auto x = 0; x < neunet_unroll; ++x) ans_reg[x] = _mm256_add_pd(ans_reg[x], _mm256_load_pd(src[i].proto.ptr + idx_temp + x * neunet_reg_sz));

            for (auto x = 0; x < neunet_unroll; ++x) _mm256_store_pd(ans_ptr + idx_temp + x * neunet_reg_sz, ans_reg[x]);

            idx_temp = idx_next;
        }

        ans.proto.ptr = ans_ptr;
        ans_ptr       = nullptr;
        
        return ans;
    }

protected: net_matrix_base proto;

public:
    __declspec(property(get = ln_cnt))        uint64_t   line_count;
    __declspec(property(get = col_cnt))       uint64_t   column_count;
    __declspec(property(get = elem_cnt))      uint64_t   element_count;
    __declspec(property(get = is_matrix))     bool       verify;
    __declspec(property(get = transposition)) net_matrix transpose;

    friend net_matrix operator+(const net_matrix &fst, const net_matrix &snd) {
        auto ans = fst;
        ans     += snd;
        return ans;
    }
    void operator+=(const net_matrix &src) { matrix_add(proto, src.proto); }

    friend net_matrix operator-(const net_matrix &fst, const net_matrix &snd) {
        auto ans = fst;
        ans     -= snd;
        return ans;
    }
    void operator-=(const net_matrix &src) { matrix_add<true>(proto, src.proto); }

    net_matrix operator*(double src) const {
        auto ans = *this;
        ans     *= src;
        return ans;
    }
    friend net_matrix operator*(double src, const net_matrix &param) { return param * src; }
    friend net_matrix operator*(const net_matrix &fst, const net_matrix &snd) {
        net_matrix ans;
        ans.proto = matrix_mul(fst.proto, snd.proto);
        return ans;
    }
    void operator*=(double src) { matrix_mul(proto, src); }
    void operator*=(const net_matrix &src) { *this = *this * src; }

    bool operator==(const net_matrix &src) const { return proto == src.proto; }

    double *operator[](uint64_t ln) const { return proto[ln]; }

    friend std::ostream &operator<<(std::ostream &os, const net_matrix &src) {
        os << src.proto;
        return os;
    }

};

NEUNET_END