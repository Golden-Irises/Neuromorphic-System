NEUNET_BEGIN

double sigmoid(double src) { return 1 / (1 + 1 / std::exp(src)); }

double sigmoid_dv(double src) { { return sigmoid(src) * (1 - sigmoid(src)); } }

double ReLU(double src) { return src < 0 ? 0 : src; }

double ReLU_dv(double src) { return src < 0 ? 0 : 1; }

void softmax(net_matrix &src) {
    neunet_traverse(src, std::exp);
    src.elem_wise_div(src.elem_sum());
}

void softmax_cec_grad(net_matrix &out, net_matrix &orgn) {
    orgn.elem_wise_div(out);
    double sum = 0;
    for (auto i = 0ull; i < orgn.element_count; ++i) sum += orgn.index(i) * out.index(i);
    for (auto i = 0ull; i < orgn.element_count; ++i) orgn.index(i) = sum - orgn.index(i);
    out *= sum;
}

uint64_t samp_block_cnt(uint64_t filter_dir_cnt, uint64_t dir_dilate) { return (dir_dilate + 1) * filter_dir_cnt - dir_dilate; }

uint64_t samp_trace_pos(uint64_t output_dir_pos, uint64_t filter_dir_pos, uint64_t dir_stride, uint64_t dir_dilate) { return output_dir_pos * dir_stride + filter_dir_pos * (1 + dir_dilate);}

uint64_t samp_output_dir_cnt(uint64_t input_dir_cnt, uint64_t filter_dir_cnt, uint64_t dir_stride, uint64_t dir_dilate) { return (input_dir_cnt - samp_block_cnt(filter_dir_cnt, dir_dilate)) / dir_stride + 1; }

uint64_t samp_input_dir_cnt(uint64_t output_dir_cnt, uint64_t filter_dir_cnt, uint64_t dir_stride, uint64_t dir_dilate) { return (output_dir_cnt - 1) * dir_stride + samp_block_cnt(filter_dir_cnt, dir_dilate); }

net_set<uint64_t> im2col_pad_idx(uint64_t &ans_ln_cnt, uint64_t &ans_col_cnt, const net_matrix &in, uint64_t in_ln_cnt, uint64_t in_col_cnt, uint64_t top_cnt, uint64_t right_cnt, uint64_t bottom_cnt, uint64_t left_cnt, uint64_t ln_dist, uint64_t col_dist) {
    if (!(top_cnt || right_cnt || bottom_cnt || left_cnt || ln_dist || col_dist)) {
        ans_ln_cnt  = in_ln_cnt;
        ans_col_cnt = in_col_cnt;
        return {};
    }
    if (!ans_ln_cnt) ans_ln_cnt = matrix_pad_dir_cnt(top_cnt, bottom_cnt, in_ln_cnt, ln_dist);
    if (!ans_col_cnt) ans_col_cnt = matrix_pad_dir_cnt(left_cnt, right_cnt, in_col_cnt, col_dist);
    net_set<uint64_t> ans(ans_ln_cnt * ans_col_cnt * in.column_count);
    for (auto i = 0ull; i < in_ln_cnt; ++i) for (auto j = 0ull; j < in_col_cnt; ++j) {
        auto im2col_ln = ((top_cnt + i * (ln_dist + 1)) * ans_col_cnt + left_cnt + j * (col_dist + 1)) * in.column_count;
        auto in_index  = (i * in_col_cnt + j) * in.column_count;
        for (auto k = 0ull; k < in.column_count; ++k) ans[im2col_ln + k] = in_index + k;
    }
    return ans;
}

net_set<uint64_t> im2col_crop_idx(uint64_t &ans_ln_cnt, uint64_t &ans_col_cnt, const net_matrix &in, uint64_t in_ln_cnt, uint64_t in_col_cnt, uint64_t top_cnt, uint64_t right_cnt, uint64_t bottom_cnt, uint64_t left_cnt, uint64_t ln_dist, uint64_t col_dist) {
    if (!(top_cnt || right_cnt || bottom_cnt || left_cnt || ln_dist || col_dist)) {
        ans_ln_cnt  = in_ln_cnt;
        ans_col_cnt = in_col_cnt;
        return {};
    }
    if (!ans_ln_cnt) ans_ln_cnt = matrix_crop_dir_cnt(top_cnt, bottom_cnt, in_ln_cnt, ln_dist);
    if (!ans_col_cnt) ans_col_cnt = matrix_crop_dir_cnt(left_cnt, right_cnt, in_col_cnt, col_dist);
    net_set<uint64_t> ans(ans_ln_cnt * ans_col_cnt * in.column_count);
    for (auto i = 0ull; i < ans_ln_cnt; ++i) for (auto j = 0ull; j < ans_col_cnt; ++j) {
        auto im2col_ln = (i * in_col_cnt + j) * in.column_count;
        auto in_index  = ((top_cnt + i * (ln_dist + 1)) * in_col_cnt + left_cnt + j * (col_dist + 1)) * in.column_count;
        for (auto k = 0ull; k < in.column_count; ++k) ans[im2col_ln + k] = in_index + k;
    }
    return ans;
}

net_matrix im2col_from_tensor(const net_set<net_matrix> &src) {
    net_matrix ans(src[0].element_count, src.length);
    for (auto i = 0ull; i < src.length; ++i) for (auto j = 0ull; j < src[i].element_count; ++j) ans[j][i] = src[i].index(j);
    return ans;
}
net_set<net_matrix> im2col_to_tensor(const net_matrix &src, uint64_t ln_cnt, uint64_t col_cnt) {
    auto elem_cnt = ln_cnt * col_cnt;
    if (src.line_count != elem_cnt) return {};
    net_set<net_matrix> ans(src.column_count);
    for (auto i = 0ull; i < ans.length; ++i) {
        ans[i] = {ln_cnt, col_cnt};
        for (auto j = 0ull; j < elem_cnt; ++j) ans[i].index(j) = src[j][i];
    }
    return ans;
}

void net_train_progress(int curr_prog, int prog, double acc, double rc, int dur) { std::printf("\r[Train][%d/%d][Acc/Rc][%.2f/%.2f][%dms]", curr_prog, prog, acc, rc, dur); }

void net_epoch_status(int epoch, double acc, double rc, int dur) {
    std::printf("\r[Epoch][%d][Acc/Rc][%.4f/%.4f][%dms]", epoch, acc, rc, dur);
    std::cout << std::endl;
}

net_matrix net_lbl_orgn(uint64_t lbl_val, uint64_t type_cnt) {
    net_matrix ans(type_cnt, 1);
    ans.index(lbl_val) = 1;
    return ans;
}
net_set<net_matrix> net_lbl_orgn(const net_set<uint64_t> &lbl_set, uint64_t type_cnt) {
    net_set<net_matrix> ans(lbl_set.length);
    for (auto i = 0ull; i < ans.length; ++i) ans[i] = net_lbl_orgn(lbl_set[i], type_cnt);
    return ans;
}

void net_out_acc_rc(const net_matrix &output, long double train_acc, uint64_t lbl, std::atomic_uint64_t &acc_cnt, std::atomic_uint64_t &rc_cnt) {
    if (output.index(lbl) > 0.5) ++acc_cnt;
    if (output.index(lbl) > (1 - train_acc)) ++rc_cnt;
}

struct net_counter {
    std::atomic_uint64_t cnt = 0;

    net_counter() = default;
    net_counter(const net_counter &src) { cnt = (uint64_t)src.cnt; }
    net_counter &operator=(const net_counter &src) {
        cnt = (uint64_t)src.cnt;
        return *this;
    }
};

struct ada_delta final {
public: void delta(net_matrix &grad) {
    grad.elem_wise_mul(grad);
    if (exp_grad.verify) {
        exp_grad *= rho;
        exp_grad += grad * (1 - rho);
    } else exp_grad = grad * (1 - rho);

    auto tmp_grad = exp_grad;
    tmp_grad.broadcast_add(neunet_eps);
    if (exp_delta.verify) {
        auto tmp = std::move(tmp_grad);
        tmp_grad = exp_delta;
        tmp_grad.broadcast_add(neunet_eps);
        tmp_grad.elem_wise_div(tmp);
    } else tmp_grad.elem_wise_div<false>(neunet_eps);
    grad.elem_wise_mul(tmp_grad);

    if (exp_delta.verify) {
        exp_delta *= rho;
        exp_delta += grad * (1 - rho);
    } else exp_delta = grad * (1 - rho);

    grad.elem_wise_pow(0.5);
}

private: net_matrix exp_grad, exp_delta;

public: double rho = 0.95;
};

struct ada_nesterov final {
public:
    net_matrix weight(const net_matrix &curr_weight) const {
        if (velocity.verify) return curr_weight - rho * velocity;
        else return curr_weight;
    }

    void momentum(net_matrix &curr_grad, double learn_rate) {
        curr_grad *= learn_rate;
        if (!velocity.verify) {
            velocity = curr_grad;
            return;
        }
        velocity *= rho;
        velocity += curr_grad;
        curr_grad = velocity;
    }

private: net_matrix velocity;

public: double rho = 0.9;
};

NEUNET_END