KOKKORO_BEGIN

kokkoro_array kokkoro_data_transfer(char raw_msg[kokkoro_data_segcnt]
                                    #if kokkoro_dcb_msg
                                    , std::string &msg_out
                                    #endif
                                    ) {
    #if kokkoro_dcb_msg // print current message
    msg_out.clear();
    for (auto i = 0; i < kokkoro_data_segcnt; ++i) msg_out += '[' + std::bitset<8>(raw_msg[i]).to_string() + ']';
    msg_out += '\n';
    #endif

    kokkoro_array ans;
    for (auto i = 0; i < kokkoro_data_segcnt; ++i) for (auto j = 0; j < kokkoro_data_segcnt; ++j) {
        ans.sen_arr[j + i * kokkoro_data_segcnt] = raw_msg[i] & kokkoro_data_segcnt;
        raw_msg[i]                             >>= kokkoro_data_bitsz;
    }

    #if kokkoro_dcb_msg // print sensor value
    for (auto i = 0; i < kokkoro_data_arrsz; i += 3) {
        msg_out += "[  ";
        for (auto j = kokkoro_data_segcnt; j; --j) msg_out += ' ' + std::to_string(ans.sen_arr[i + j - 1]);
        msg_out += ']';
    }
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

    dcb_hdl h_port {0};

    int iobat_sz = intrv_ms * baudrate / 1000 / kokkoro_data_bitcnt,
        start_pt = 0;

    // buffer reading area

    kokkoro_queue<kokkoro_array> data_que;

    std::atomic_bool read_stop = false,
                     reset_sgn = true;

    #if kokkoro_dcb_msg // message queue
    kokkoro_queue<std::string> msg_que;
    #endif

    #if kokkoro_data_save
    // array saving area
    std::ofstream save_data_ofs, save_lbl_ofs;
    #else
    // array max area
    kokkoro_queue<kokkoro_array> arr_que;
    #endif

    #if kokkoro_dcb_peak
    kokkoro_queue<kokkoro_array> peak_cnt_que;
    #endif

    // control area

    async_pool ctrl_pool {kokkoro_data_thdsz};

    std::atomic_bool read_end_sgn = false;
};

bool kokkoro_array_shutdown(kokkoro_array_handle &kokkoro_handle) {
    #if kokkoro_data_save
    kokkoro_handle.save_data_ofs.close();
    kokkoro_handle.save_lbl_ofs.close();
    #endif

    return dcb_shutdown(kokkoro_handle.h_port);
}

bool kokkoro_array_startup(kokkoro_array_handle &kokkoro_handle
                           #if kokkoro_data_save
                           ,const std::string &data_file_name
                           ,const std::string &lbl_file_name
                           ,const std::string &file_dir = "SRC\\ARCHIVE\\"
                           #endif
                          ) {
    #if kokkoro_data_save
    kokkoro_handle.save_data_ofs.open(file_dir + data_file_name + ".csv", std::ios::out | std::ios::trunc);
    kokkoro_handle.save_lbl_ofs.open(file_dir + lbl_file_name + ".csv", std::ios::out | std::ios::trunc);
    #endif

    return dcb_startup(kokkoro_handle.h_port, kokkoro_handle.port_idx, kokkoro_handle.baudrate, kokkoro_handle.databits, kokkoro_handle.stopbits, kokkoro_handle.parity, kokkoro_handle.async_mode, kokkoro_handle.iobuf_sz, kokkoro_handle.iobuf_sz)
    
    #if kokkoro_data_save
    && kokkoro_handle.save_data_ofs.is_open()
    && kokkoro_handle.save_lbl_ofs.is_open()
    #endif

    ;
}

void kokkoro_array_read_thread(kokkoro_array_handle &kokkoro_handle) { kokkoro_handle.ctrl_pool.add_task([&kokkoro_handle] { kokkoro_loop {
    if (kokkoro_handle.read_stop) {
        kokkoro_handle.read_end_sgn = true;
        return;
    }
    if (kokkoro_handle.reset_sgn) {
        kokkoro_handle.reset_sgn = false;
        kokkoro_handle.start_pt  = 0;
        kokkoro_handle.data_que.clear();
    }
    if (kokkoro_handle.start_pt < kokkoro_data_segcnt) {
        char start_c = 0;
        if (dcb_read(kokkoro_handle.h_port, &start_c, 1, kokkoro_handle.async_mode) && start_c == kokkoro_dcb_start) ++kokkoro_handle.start_pt;
        else kokkoro_handle.start_pt = 0;
        continue;
    }
    char buf_tmp[kokkoro_data_segcnt] = {0};
    if (kokkoro_data_segcnt != dcb_read(kokkoro_handle.h_port, buf_tmp, kokkoro_data_segcnt, kokkoro_handle.async_mode)) continue;
    #if kokkoro_dcb_msg
    std::string msg_out {};
    kokkoro_handle.data_que.en_queue(kokkoro_data_transfer(buf_tmp, msg_out));
    kokkoro_handle.msg_que.en_queue(std::move(msg_out));
    #else
    kokkoro_handle.data_que.en_queue(kokkoro_data_transfer(buf_tmp));
    #endif
} } ); }

void kokkoro_array_save_thread(kokkoro_array_handle &kokkoro_handle, bool zero_arr = true, char ctrl_key = 0) { kokkoro_loop {
    kokkoro_array max_tmp;

    #if kokkoro_dcb_peak
    kokkoro_array peak_cnt;
    int dif_tmp[kokkoro_data_arrsz] = {0},
        pre_tmp[kokkoro_data_arrsz] = {0};
    #endif

    for (auto i = 0; i < kokkoro_handle.iobat_sz; ++i) {
        auto arr_tmp = kokkoro_handle.data_que.de_queue();

        if (kokkoro_array_verify(arr_tmp)) { if (zero_arr) {
            zero_arr = false;
            
            #if kokkoro_data_save
            std::cout << "[Symbol][+(1) -(2) x(3) /(4)][Action][exit(5) reset(6)]: ";
            std::cin >> ctrl_key;
            #endif

        } } else if (zero_arr) {
            --i;
            continue;
        } else {
            zero_arr = true;
            i        = kokkoro_handle.iobat_sz;
        }
        if (ctrl_key == kokkoro_key_reset) kokkoro_handle.reset_sgn = true;
        if (ctrl_key == kokkoro_key_exit) {
            kokkoro_handle.read_stop = true;
            while (!kokkoro_handle.read_end_sgn) _sleep(kokkoro_sleep_ms);
            return;
        }

        #if kokkoro_dcb_msg
        std::cout << kokkoro_handle.msg_que.de_queue() << std::endl;
        #endif

        // get max message
        for (auto j = 0; j < kokkoro_data_arrsz; ++j) {
            if (arr_tmp.sen_arr[j] > max_tmp.sen_arr[j]) max_tmp.sen_arr[j] = arr_tmp.sen_arr[j];

            #if kokkoro_dcb_peak
            auto curr_dif = arr_tmp.sen_arr[j] - pre_tmp[j];
            if (curr_dif ^ dif_tmp[j] && curr_dif < 0) ++peak_cnt.sen_arr[j];
            pre_tmp[j] = arr_tmp.sen_arr[j];
            dif_tmp[j] = curr_dif;
            #endif
        }
    }

    #if kokkoro_data_save
    // write to file stream
    for (auto j = 0; j < kokkoro_data_arrsz; ++j) {
        if (j) kokkoro_handle.save_data_ofs << csv_comma;
        kokkoro_handle.save_data_ofs << std::to_string(max_tmp.sen_arr[j]);
    }
    kokkoro_handle.save_data_ofs << csv_enter;
    kokkoro_handle.save_lbl_ofs << ctrl_key << csv_enter;
    #else
    kokkoro_handle.arr_que.en_queue(std::move(max_tmp));
    if (zero_arr) kokkoro_handle.arr_que.en_queue();
    #endif

    #if kokkoro_dcb_peak
    // print peak
    kokkoro_handle.peak_cnt_que.en_queue(peak_cnt);
    #endif
}}

bool kokkoro_csv_data_load(kokkoro_set<kokkoro_matrix> &data_set, kokkoro_set<uint64_t> &lbl_set, const std::string &csv_data_path, const std::string &csv_lbl_path) {
    auto data_tab = csv_in(csv_data_path),
         lbl_tab  = csv_in(csv_lbl_path);
    if (data_tab.length != lbl_tab.length) return false;
    data_set.init(data_tab.length);
    lbl_set.init(lbl_tab.length);
    for (auto i = 0ull; i < data_set.length; ++i) {
        data_set[i] = {kokkoro_data_arrsz, 1};
        for (auto j = 0; j < kokkoro_data_arrsz; ++j) data_set[i].index(j) = std::atoi(data_tab[i][j].c_str());
        lbl_set[i] = std::atoi(lbl_tab[i][0].c_str());
    }
    return true;
}

KOKKORO_END