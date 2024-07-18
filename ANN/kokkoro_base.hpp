KOKKORO_BEGIN

double sigmoid(double src) { return 1 / (1 + 1 / std::exp(src)); }

double sigmoid_dv(double src) { { return sigmoid(src) * (1 - sigmoid(src)); } }

double ReLU(double src) { return src < 0 ? 0 : src; }

double ReLU_dv(double src) { return src < 0 ? 0 : 1; }

void softmax(kokkoro_matrix &src) {
    kokkoro_traverse(src, std::exp);
    src.elem_wise_div(src.elem_sum());
}

void softmax_cec_grad(kokkoro_matrix &out, kokkoro_matrix &orgn) {
    orgn.elem_wise_div(out);
    double sum = 0;
    for (auto i = 0ull; i < orgn.element_count; ++i) sum += orgn.index(i) * out.index(i);
    for (auto i = 0ull; i < orgn.element_count; ++i) orgn.index(i) = sum - orgn.index(i);
    out.elem_wise_mul(orgn);
}

uint64_t samp_block_cnt(uint64_t filter_dir_cnt, uint64_t dir_dilate) { return (dir_dilate + 1) * filter_dir_cnt - dir_dilate; }

uint64_t samp_trace_pos(uint64_t output_dir_pos, uint64_t filter_dir_pos, uint64_t dir_stride, uint64_t dir_dilate) { return output_dir_pos * dir_stride + filter_dir_pos * (1 + dir_dilate);}

uint64_t samp_output_dir_cnt(uint64_t input_dir_cnt, uint64_t filter_dir_cnt, uint64_t dir_stride, uint64_t dir_dilate) { return (input_dir_cnt - samp_block_cnt(filter_dir_cnt, dir_dilate)) / dir_stride + 1; }

uint64_t samp_input_dir_cnt(uint64_t output_dir_cnt, uint64_t filter_dir_cnt, uint64_t dir_stride, uint64_t dir_dilate) { return (output_dir_cnt - 1) * dir_stride + samp_block_cnt(filter_dir_cnt, dir_dilate); }

kokkoro_set<uint64_t> im2col_pad_in_idx( uint64_t &ans_ln_cnt, uint64_t &ans_col_cnt, uint64_t in_ln_cnt, uint64_t in_col_cnt, uint64_t in_chann_cnt, uint64_t top_cnt, uint64_t right_cnt, uint64_t bottom_cnt, uint64_t left_cnt, uint64_t ln_dist, uint64_t col_dist) {
    if (!(top_cnt || right_cnt || bottom_cnt || left_cnt || ln_dist || col_dist)) {
        ans_ln_cnt  = in_ln_cnt;
        ans_col_cnt = in_col_cnt;
        return {};
    }
    ans_ln_cnt  = matrix_pad_dir_cnt(top_cnt, bottom_cnt, in_ln_cnt, ln_dist);
    ans_col_cnt = matrix_pad_dir_cnt(left_cnt, right_cnt, in_col_cnt, col_dist);
    kokkoro_set<uint64_t> in_idx_set(in_ln_cnt * in_col_cnt * in_chann_cnt);
    for (auto i = 0ull; i < in_ln_cnt; ++i) for (auto j = 0ull; j < in_col_cnt; ++j) {
        auto im2col_ln = ((top_cnt + i * (ln_dist + 1)) * ans_col_cnt + left_cnt + j * (col_dist + 1)) * in_chann_cnt;
        auto in_index  = (i * in_col_cnt + j) * in_chann_cnt;
        for (auto k = 0ull; k < in_chann_cnt; ++k) in_idx_set[in_index + k] = im2col_ln + k;
    }
    return in_idx_set;
}

kokkoro_set<uint64_t> im2col_crop_out_idx(uint64_t &ans_ln_cnt, uint64_t &ans_col_cnt, uint64_t in_ln_cnt, uint64_t in_col_cnt, uint64_t in_chann_cnt, uint64_t top_cnt, uint64_t right_cnt, uint64_t bottom_cnt, uint64_t left_cnt, uint64_t ln_dist, uint64_t col_dist) {
    if (!(top_cnt || right_cnt || bottom_cnt || left_cnt || ln_dist || col_dist)) {
        ans_ln_cnt  = in_ln_cnt;
        ans_col_cnt = in_col_cnt;
        return {};
    }
    ans_ln_cnt  = matrix_crop_dir_cnt(top_cnt, bottom_cnt, in_ln_cnt, ln_dist);
    ans_col_cnt = matrix_crop_dir_cnt(left_cnt, right_cnt, in_col_cnt, col_dist);
    kokkoro_set<uint64_t> out_idx_set(ans_ln_cnt * ans_col_cnt * in_chann_cnt);
    for (auto i = 0ull; i < ans_ln_cnt; ++i) for (auto j = 0ull; j < ans_col_cnt; ++j) {
        auto im2col_ln = (i * ans_col_cnt + j) * in_chann_cnt;
        auto in_index  = ((top_cnt + i * (ln_dist + 1)) * in_col_cnt + left_cnt + j * (col_dist + 1)) * in_chann_cnt;
        for (auto k = 0ull; k < in_chann_cnt; ++k) out_idx_set[im2col_ln + k] = in_index + k;
    }
    return out_idx_set;
}

kokkoro_matrix im2col_from_tensor(const kokkoro_set<kokkoro_matrix> &src) {
    kokkoro_matrix ans(src[0].element_count, src.length);
    for (auto i = 0ull; i < src.length; ++i) for (auto j = 0ull; j < src[i].element_count; ++j) ans[j][i] = src[i].index(j);
    return ans;
}
kokkoro_set<kokkoro_matrix> im2col_to_tensor(const kokkoro_matrix &src, uint64_t ln_cnt, uint64_t col_cnt) {
    auto elem_cnt = ln_cnt * col_cnt;
    if (src.line_count != elem_cnt) return {};
    kokkoro_set<kokkoro_matrix> ans(src.column_count);
    for (auto i = 0ull; i < ans.length; ++i) {
        ans[i] = {ln_cnt, col_cnt};
        for (auto j = 0ull; j < elem_cnt; ++j) ans[i].index(j) = src[j][i];
    }
    return ans;
}

kokkoro_set<uint64_t> kokkoro_dataset_idx_init(uint64_t dataset_sz) {
    kokkoro_set<uint64_t> ans(dataset_sz);
    for (auto i = 0; i < dataset_sz; ++i) ans[i] = i;
    return ans;
}

void kokkoro_train_progress(int curr_prog, int prog, double acc, double rc, int dur) { std::printf("\r[Train][%d/%d][Acc/Rc][%.2f/%.2f][%dms]", curr_prog, prog, acc, rc, dur); }

void kokkoro_epoch_status(int epoch, double acc, double rc, int dur) {
    std::printf("\r[Epoch][%d][Acc/Rc][%.4f/%.4f][%dms]", epoch, acc, rc, dur);
    std::printf("\n");
}

kokkoro_matrix kokkoro_lbl_orgn(uint64_t lbl_val, uint64_t type_cnt) {
    kokkoro_matrix ans(type_cnt, 1);
    ans.index(lbl_val) = 1;
    return ans;
}

void kokkoro_out_acc_rc(const kokkoro_matrix &output, double train_acc, uint64_t lbl, std::atomic_uint64_t &acc_cnt, std::atomic_uint64_t &rc_cnt) {
    auto max_idx = 0;
    for (auto i = 1ull; i < output.element_count; ++i) if (output.index(i) > output.index(max_idx)) max_idx = i;
    if (max_idx == lbl) ++acc_cnt;
    // if (output.index(lbl) > 0.5) ++acc_cnt;
    if (output.index(lbl) > (1 - train_acc)) ++rc_cnt;
}

template <double rho = 0.9>
struct ada_delta final {
public:
    void delta(kokkoro_matrix &grad) {
        grad.elem_wise_mul(grad);
        if (exp_grad.verify) {
            exp_grad *= rho;
            exp_grad += grad * (1 - rho);
        } else exp_grad = grad * (1 - rho);

        auto tmp_grad = exp_grad;
        tmp_grad.broadcast_add(kokkoro_eps);
        if (exp_delta.verify) {
            auto tmp = std::move(tmp_grad);
            tmp_grad = exp_delta;
            tmp_grad.broadcast_add(kokkoro_eps);
            tmp_grad.elem_wise_div(tmp);
        } else tmp_grad.elem_wise_div<false>(kokkoro_eps);
        grad.elem_wise_mul(tmp_grad);

        if (exp_delta.verify) {
            exp_delta *= rho;
            exp_delta += grad * (1 - rho);
        } else exp_delta = grad * (1 - rho);

        grad.elem_wise_pow(0.5);
    }

    void update(kokkoro_matrix &curr_weight, kokkoro_matrix &grad) {
        delta(grad);
        curr_weight -= grad;
    }

private: kokkoro_matrix exp_grad, exp_delta;
};

template <double learn_rate = .1, double rho = 0.9>
struct ada_nesterov final {
public:
    kokkoro_matrix weight(const kokkoro_matrix &curr_weight) const {
        if (velocity.verify) return curr_weight - rho * velocity;
        else return curr_weight;
    }

    void momentum(kokkoro_matrix &curr_grad) {
        curr_grad *= learn_rate;
        if (!velocity.verify) {
            velocity = curr_grad;
            return;
        }
        velocity *= rho;
        velocity += curr_grad;
        curr_grad = velocity;
    }

    void update(kokkoro_matrix &curr_weight, kokkoro_matrix &nesterov_weight, kokkoro_matrix &grad) {
        momentum(grad);
        curr_weight    -= grad;
        nesterov_weight = weight(curr_weight);
    }

private: kokkoro_matrix velocity;
};

KOKKORO_END