KOKKORO_BEGIN

void kokkoro_dcb_vect_mask(kokkoro_set<int> &ans, char *src) { for (auto i = 0; i < kokkoro_dcb_mask; ++i) for (auto j = 0; j < kokkoro_dcb_mask; ++j) {
    ans[i + j * kokkoro_dcb_mask] = src[j] & kokkoro_dcb_mask;
    src[j]                      >>= kokkoro_dcb_bitsz;
} }

// int ans[9], char src[4] = {00, 00, 00, 3f}
void kokkoro_dcb_vect(kokkoro_set<int> &ans, char *src) {
    #if kokkoro_dcb_msg
    for (auto i = 0; i < kokkoro_dcb_mask; ++i) std::cout << '[' << std::bitset<8>(src[i]) << ']'; std::cout << std::endl;
    #endif
    kokkoro_dcb_vect_mask(ans, src);
    #if kokkoro_dcb_msg
    for (auto i = 0; i < kokkoro_dcb_arraysz; i += 3) {
        std::cout << "[  ";
        for (auto j = kokkoro_dcb_mask; j; --j) std::cout << ' ' << ans[i + j - 1];
        std::cout << ']';
    }
    std::cout << std::endl;
    #endif
}

int kokkoro_dcb_interval_data_cnt(int baudrate, int interval_ms) { return interval_ms * baudrate / 1000 / kokkoro_dcb_unitsz; }

struct kokkoro_dcb_handle {
    int port_idx = 3,
        baudrate = CBR_9600,
        inter_ms = 400,
        buf_sz   = 1024,
        data_cnt = inter_ms * baudrate / 1000 / kokkoro_dcb_unitsz;

    dcb_hdl dcb_port;
    
    kokkoro_queue<kokkoro_set<int>> raw_data, max_elem;
};

bool kokkoro_dcb_data_read(kokkoro_dcb_handle &kokkoro_data) {
    if (!dcb_startup(kokkoro_data.dcb_port, kokkoro_data.port_idx, kokkoro_data.baudrate, 6, ONESTOPBIT, NOPARITY, false, kokkoro_data.buf_sz, kokkoro_data.buf_sz)) return false;
    #if kokkoro_dcb_msg
    std::printf("[Startup DCB port.]");
    #endif
    char data_buf[kokkoro_dcb_bytecnt] = {0};
    kokkoro_set<int> data_tmp(kokkoro_dcb_arraysz);
    kokkoro_loop {
        auto data_buf_len = dcb_read(kokkoro_data.dcb_port, data_buf, kokkoro_dcb_bytecnt);
        if (!data_buf_len) break;
        kokkoro_dcb_vect(data_tmp, data_buf);
        kokkoro_data.raw_data.en_queue(data_tmp);
    }
    return true;
}

void kokkoro_dcb_data_save(kokkoro_dcb_handle &kokkoro_data, bool peak_stat = false) { kokkoro_loop {
    kokkoro_set<int> max_num(kokkoro_dcb_arraysz), prev_num(kokkoro_dcb_arraysz),
                     dif_num(kokkoro_dcb_arraysz), peak_cnt(kokkoro_dcb_arraysz);
    for (auto i = 0; i < kokkoro_data.data_cnt; ++i) {
        auto curr_num = kokkoro_data.raw_data.de_queue();
        auto curr_dif = 0;
        for (auto j = 0; j < kokkoro_dcb_arraysz; ++j) {
            if (curr_num[j] > max_num[j]) max_num[j] = curr_num[j];
            curr_dif = curr_num[j] - prev_num[i];
            if (curr_dif ^ dif_num[j] && curr_dif < 0) ++peak_cnt[j];
            prev_num[j] = curr_num[j];
            dif_num [j] = curr_dif;
        }
    }
    if (!kokkoro_data.raw_data.size()) break;
    if (peak_stat) {
        std::cout << "[Peak Count][";
        for (auto i = 0; i < kokkoro_dcb_arraysz; ++i) {
            std::cout << peak_cnt[i];
            if (i + 1 < kokkoro_dcb_arraysz) std::cout << ' ';
        }
        std::cout << ']' << std::endl;
    }
    kokkoro_data.max_elem.en_queue(std::move(max_num));
} }

KOKKORO_END