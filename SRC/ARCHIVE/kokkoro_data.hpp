KOKKORO_BEGIN

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

    // buffer reading area

    kokkoro_queue<kokkoro_array> data_que;

    std::atomic_bool read_stop = false,
                     reset_sgn = true;

    #if kokkoro_dcb_msg // message queue
    kokkoro_queue<std::string> msg_que;
    #endif

    // array saving area

    std::ofstream save_ofs;

    // control area

    async_pool ctrl_pool {kokkoro_data_thdsz};

    std::atomic_size_t ctrl_sz = 0;

    std::atomic_char ctrl_key = 0;
};

kokkoro_array kokkoro_data_transfer(kokkoro_array_handle &kokkoro_handle, char raw_msg[kokkoro_data_segcnt]) {
    #if kokkoro_dcb_msg // print current message
    std::string msg_elem;
    for (auto i = 0; i < kokkoro_data_segcnt; ++i) msg_elem += '[' + std::bitset<8>(raw_msg[i]).to_string() + ']';
    msg_elem += '\n';
    #endif

    kokkoro_array ans;
    for (auto i = 0; i < kokkoro_data_segcnt; ++i) for (auto j = 0; j < kokkoro_data_segcnt; ++j) {
        ans.sen_arr[j + i * kokkoro_data_segcnt] = raw_msg[i] & kokkoro_data_segcnt;
        raw_msg[i]                             >>= kokkoro_data_bitsz;
    }

    #if kokkoro_dcb_msg // print sensor value
    for (auto i = 0; i < kokkoro_data_arrsz; i += 3) {
        msg_elem += "[  ";
        for (auto j = kokkoro_data_segcnt; j; --j) msg_elem += ' ' + std::to_string(ans.sen_arr[i + j - 1]);
        msg_elem += ']';
    }
    kokkoro_handle.msg_que.en_queue(std::move(msg_elem));
    #endif

    return ans;
}

bool kokkoro_array_verify(const kokkoro_array &src) {
    for (auto i = 0; i < kokkoro_data_arrsz; ++i) if (src.sen_arr[i]) return true;
    return false;
}

bool kokkoro_array_shutdown(kokkoro_array_handle &kokkoro_handle) {
    kokkoro_handle.save_ofs.close();
    return dcb_shutdown(kokkoro_handle.h_port);
}

bool kokkoro_array_startup(kokkoro_array_handle &kokkoro_handle, const std::string &file_name, const std::string &file_dir = "SRC\\ARCHIVE\\") {
    kokkoro_handle.save_ofs.open(file_dir + file_name + ".csv", std::ios::out | std::ios::trunc); 
    return dcb_startup(kokkoro_handle.h_port, kokkoro_handle.port_idx, kokkoro_handle.baudrate, kokkoro_handle.databits, kokkoro_handle.stopbits, kokkoro_handle.parity, kokkoro_handle.async_mode, kokkoro_handle.iobuf_sz, kokkoro_handle.iobuf_sz) && kokkoro_handle.save_ofs.is_open();
}

bool kokkoro_array_read_stop(kokkoro_array_handle &kokkoro_handle) {
    if (kokkoro_handle.read_stop) {
        ++kokkoro_handle.ctrl_sz;
        return true;
    }
    return false;
}

bool kokkoro_array_save_stop(kokkoro_array_handle &kokkoro_handle) {
    if (kokkoro_handle.read_stop || kokkoro_handle.ctrl_key == kokkoro_key_exit) {
        ++kokkoro_handle.ctrl_sz;
        return true;
    }
    return false;
}

void kokkoro_array_read_thread(kokkoro_array_handle &kokkoro_handle) { kokkoro_handle.ctrl_pool.add_task([&kokkoro_handle] { kokkoro_loop {
    if (kokkoro_array_read_stop(kokkoro_handle)) return;
    if (kokkoro_handle.reset_sgn) {
        kokkoro_handle.reset_sgn = false;
        kokkoro_handle.start_pt  = 0;
    }
    if (kokkoro_handle.start_pt < kokkoro_data_segcnt) {
        char start_c = 0;
        if (dcb_read(kokkoro_handle.h_port, &start_c, 1, kokkoro_handle.async_mode) && start_c == kokkoro_dcb_start) ++kokkoro_handle.start_pt;
        else kokkoro_handle.start_pt = 0;
        continue;
    }
    char buf_tmp[kokkoro_data_segcnt] = {0};
    if (!dcb_read(kokkoro_handle.h_port, buf_tmp, kokkoro_data_segcnt, kokkoro_handle.async_mode)) continue;
    kokkoro_handle.data_que.en_queue(kokkoro_data_transfer(kokkoro_handle, buf_tmp));
} } ); }

void kokkoro_array_save_thread(kokkoro_array_handle &kokkoro_handle, bool peak_cnt = false) { kokkoro_handle.ctrl_pool.add_task([&kokkoro_handle, peak_cnt](bool zero_arr = true) { kokkoro_loop {
    int max_tmp[kokkoro_data_arrsz] = {0},
        dif_tmp[kokkoro_data_arrsz] = {0},
        pre_tmp[kokkoro_data_arrsz] = {0},
        peak_pt[kokkoro_data_arrsz] = {0};

    for (auto i = 0; i < kokkoro_handle.iobat_sz; ++i) {
        // get data label
        auto arr_tmp = kokkoro_handle.data_que.de_queue();
        if (kokkoro_array_verify(arr_tmp)) { if (zero_arr) {
            zero_arr = false;
            std::cout << "[Symbol][+(1) -(2) x(3) /(4)]: ";
            while (!(kokkoro_syb_check(kokkoro_handle.ctrl_key) ||
                    kokkoro_handle.ctrl_key == kokkoro_key_exit)) _sleep(kokkoro_sleep_ms);
            std::cout << kokkoro_handle.ctrl_key << std::endl;
        } } else if (!zero_arr) {
            kokkoro_handle.ctrl_key = 0;
            zero_arr                = true;
        }

        if (kokkoro_array_save_stop(kokkoro_handle)) return;

        #if kokkoro_dcb_msg
        std::cout << kokkoro_handle.msg_que.de_queue() << std::endl;
        #endif

        // get max message
        for (auto j = 0; j < kokkoro_data_arrsz; ++j) {
            if (arr_tmp.sen_arr[j] > max_tmp[j]) max_tmp[j] = arr_tmp.sen_arr[j];
            if (!peak_cnt) continue;
            auto curr_dif = arr_tmp.sen_arr[j] - pre_tmp[j];
            if (curr_dif ^ dif_tmp[j] && curr_dif < 0) ++peak_pt[j];
            pre_tmp[j] = arr_tmp.sen_arr[j];
            dif_tmp[j] = curr_dif;
        }
    }

    // write to file stream
    for (auto j = 0; j < kokkoro_data_arrsz; ++j) kokkoro_handle.save_ofs << std::to_string(max_tmp[j]) << csv_comma;
    kokkoro_handle.save_ofs << kokkoro_handle.ctrl_key << csv_enter;

    // print peak
    if (!peak_cnt) continue;
    std::cout << "[Peak Count][";
    for (auto i = 0; i < kokkoro_data_arrsz; ++i) {
        std::cout << peak_pt[i];
        if (i + 1 < kokkoro_data_arrsz) std::cout << ' ';
    }
    std::cout << ']' << std::endl;
} } ); }

void kokkoro_array_control_thread(kokkoro_array_handle &kokkoro_handle) { kokkoro_loop {
    kokkoro_handle.ctrl_key = getch();
    switch(kokkoro_handle.ctrl_key) {
    case kokkoro_key_exit: // exit
        kokkoro_handle.read_stop = true;
        while (kokkoro_handle.ctrl_sz < kokkoro_data_thdsz) _sleep(kokkoro_sleep_ms);
        return;
    case kokkoro_key_reset: kokkoro_handle.reset_sgn = true; break; // reset
    default: break;
    }
} }

KOKKORO_END