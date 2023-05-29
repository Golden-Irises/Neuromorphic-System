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

NEUNET_END