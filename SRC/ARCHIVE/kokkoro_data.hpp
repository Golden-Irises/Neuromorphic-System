KOKKORO_BEGIN

kokkoro_array kokkoro_data_transfer_core(char raw_msg[kokkoro_data_segcnt]) {
    kokkoro_array ans;
    for (auto i = 0; i < kokkoro_data_segcnt; ++i) for (auto j = 0; j < kokkoro_data_segcnt; ++j) {
        ans.sen_arr[j + i * kokkoro_data_segcnt] = raw_msg[i] & kokkoro_data_segcnt;
        raw_msg[i]                             >>= kokkoro_data_bitsz;
    }
    return ans;
}

kokkoro_array kokkoro_data_transfer(char raw_msg[kokkoro_data_segcnt]) {
    #if kokkoro_dcb_msg
    for (auto i = 0; i < kokkoro_data_segcnt; ++i) std::cout << '[' << std::bitset<8>(raw_msg[i]) << ']'; std::cout << std::endl;
    #endif
    auto ans = kokkoro_data_transfer_core(raw_msg);
    #if kokkoro_dcb_msg
    for (auto i = 0; i < kokkoro_data_arrsz; i += 3) {
        std::cout << "[  ";
        for (auto j = kokkoro_data_segcnt; j; --j) std::cout << ' ' << ans.sen_arr[i + j - 1];
        std::cout << ']';
    }
    std::cout << std::endl;
    #endif
    return ans;
}

bool kokkoro_array_verify(const kokkoro_array &src) {
    for (auto i = 0; i < kokkoro_data_arrsz; ++i) if (src.sen_arr[i]) return true;
    return false;
}

/* port index     = 3
 * baudrate       = CBR_2400
 * interval in ms = 400
 * IO buffer size = 1024
 * data bits      = 6
 * stop bits      = ONESTOPBIT
 * parity         = NOPARITY
 * async mode     = false
 */
struct kokkoro_array_handle {
    // DCB parameter area
    int port_idx = 3,
        baudrate = CBR_2400,
        intrv_ms = 400,
        iobuf_sz = 1024,

        databits = 6,
        stopbits = ONESTOPBIT,
        parity   = NOPARITY;

    bool async_mode = false;

    dcb_hdl h_port;

    int iobat_sz = intrv_ms * baudrate / 1000 / kokkoro_data_bitcnt,
        start_pt = 0;

    std::atomic_char key_ch = 0;

    // buffer reading area

    kokkoro_queue<kokkoro_array> data_que;

    std::atomic_bool read_stop = false,
                     reset_sgn = true;

    // array saving area

    std::ofstream save_ofs;

    // control area

    async_pool ctrl_pool {kokkoro_data_bitsz};
    std::atomic_size_t ctrl_sz = 0;

    #if kokkoro_dcb_msg
    std::atomic_bool ctrl_msg = true;
    #endif
};

bool kokkoro_array_shutdown(kokkoro_array_handle &kokkoro_handle) {
    while (kokkoro_handle.ctrl_sz < kokkoro_data_bitsz) _sleep(kokkoro_sleep_ms);
    kokkoro_handle.save_ofs.close();
    return dcb_shutdown(kokkoro_handle.h_port);
}

bool kokkoro_array_startup(kokkoro_array_handle &kokkoro_handle, const std::string &file_name, const std::string &file_dir = "SRC\\ARCHIVE\\") {
    kokkoro_handle.save_ofs.open(file_dir + file_name + ".csv", std::ios::out | std::ios::trunc); 
    return dcb_startup(kokkoro_handle.h_port, kokkoro_handle.port_idx, kokkoro_handle.baudrate, kokkoro_handle.databits, kokkoro_handle.stopbits, kokkoro_handle.parity, kokkoro_handle.async_mode, kokkoro_handle.iobuf_sz, kokkoro_handle.iobuf_sz) && kokkoro_handle.save_ofs.is_open();
}

void kokkoro_array_read_thread(kokkoro_array_handle &kokkoro_handle) { kokkoro_handle.ctrl_pool.add_task([&kokkoro_handle] { kokkoro_loop {
    if (kokkoro_handle.read_stop) {
        ++kokkoro_handle.ctrl_sz;
        return;
    }
    if (kokkoro_handle.reset_sgn) {
        kokkoro_handle.reset_sgn = false;
        kokkoro_handle.start_pt  = 0;
    }
    if (kokkoro_handle.start_pt < kokkoro_data_segcnt) {
        char start_c = 0;
        auto buf_len = dcb_read(kokkoro_handle.h_port, &start_c, 1, kokkoro_handle.async_mode);
        if (!buf_len) continue;
        if (start_c == kokkoro_dcb_start) ++kokkoro_handle.start_pt;
        else kokkoro_handle.start_pt = 0;
        continue;
    }
    char buf_tmp[kokkoro_data_segcnt] = {0};
    auto buf_len = dcb_read(kokkoro_handle.h_port, buf_tmp, kokkoro_data_segcnt, kokkoro_handle.async_mode);
    if (!buf_len) continue;
    #if kokkoro_dcb_msg
    while (!kokkoro_handle.ctrl_msg) _sleep(kokkoro_sleep_ms);
    #endif
    kokkoro_handle.data_que.en_queue(kokkoro_data_transfer(buf_tmp));
} } ); }

void kokkoro_array_save_thread(kokkoro_array_handle &kokkoro_handle, bool peak_cnt = false) { kokkoro_handle.ctrl_pool.add_task([&kokkoro_handle, peak_cnt](bool zero_arr = true, char syb_num = csv_ch0) { kokkoro_loop {
    int max_tmp[kokkoro_data_arrsz] = {0},
        dif_tmp[kokkoro_data_arrsz] = {0},
        pre_tmp[kokkoro_data_arrsz] = {0},
        peak_pt[kokkoro_data_arrsz] = {0};
    
    for (auto i = 0; i < kokkoro_handle.iobat_sz; ++i) {
        // get max message
        auto arr_tmp = kokkoro_handle.data_que.de_queue();
        if (kokkoro_handle.read_stop) {
            ++kokkoro_handle.ctrl_sz;
            return;
        }
        if (kokkoro_array_verify(arr_tmp)) { if (zero_arr) {
            zero_arr = false;
            
            #if kokkoro_dcb_msg
            kokkoro_handle.ctrl_msg = false;
            #endif

            kokkoro_msg_print(kokkoro_msg_data_save_syb_select);
            while (!kokkoro_syb_check(kokkoro_handle.key_ch)) _sleep(kokkoro_sleep_ms);
            syb_num = kokkoro_handle.key_ch;

            #if kokkoro_dcb_msg
            kokkoro_handle.ctrl_msg = true;
            #endif
            if (syb_num == kokkoro_key_exit || syb_num == kokkoro_key_reset) break;
        } } else if (!zero_arr) {
            kokkoro_handle.key_ch = 0;
            zero_arr              = true;
            syb_num               = csv_ch0;
        }

        // get peaks amount
        for (auto j = 0; j < kokkoro_data_arrsz; ++j) {
            if (arr_tmp.sen_arr[j] > max_tmp[j]) max_tmp[j] = arr_tmp.sen_arr[j];
            if (!peak_cnt) continue;
            auto curr_dif = arr_tmp.sen_arr[j] - pre_tmp[j];
            if (curr_dif ^ dif_tmp[j] && curr_dif < 0) ++peak_pt[j];
            pre_tmp[j] = arr_tmp.sen_arr[j];
            dif_tmp[j] = curr_dif;
        }
    }
    // check inner io message
    if (syb_num == kokkoro_key_exit || syb_num == kokkoro_key_reset) continue;

    // write to file stream
    for (auto j = 0; j < kokkoro_data_arrsz; ++j) kokkoro_handle.save_ofs << std::to_string(max_tmp[j]) << csv_comma;
    kokkoro_handle.save_ofs << syb_num << csv_enter;

    // print peak
    if (!peak_cnt) continue;
    kokkoro_msg_print(kokkoro_msg_peak_begin);
    for (auto i = 0; i < kokkoro_data_arrsz; ++i) {
        kokkoro_msg_print(kokkoro_msg_mask_int, peak_pt[i]);
        if (i + 1 < kokkoro_data_arrsz) kokkoro_msg_print(kokkoro_msg_mask_space);
    }
    kokkoro_msg_print(kokkoro_msg_peak_end);
} } ); }

void kokkoro_array_control_thread(kokkoro_array_handle &kokkoro_handle) { kokkoro_loop {
    kokkoro_handle.key_ch = _getch();
    switch (kokkoro_handle.key_ch) {
    case kokkoro_key_exit:
        kokkoro_handle.read_stop = true;
        kokkoro_handle.data_que.reset();
        return;
    case kokkoro_key_reset: kokkoro_handle.reset_sgn = true; break;
    default: kokkoro_msg_print(kokkoro_msg_mask_charn, char(kokkoro_handle.key_ch)); break;
    }
} }

KOKKORO_END