KOKKORO_BEGIN

bool kokkoro_data_blank_verify(char raw_msg[kokkoro_data_segcnt]) {
    for (auto i = 0; i < kokkoro_data_segcnt; ++i) if (raw_msg[i]) return false;
    #if kokkoro_dcb_msg
    std::printf("[00000000][00000000][00000000]\n");
    #endif
    return true;
}

kokkoro_sensor kokkoro_data_transfer_core(char raw_msg[kokkoro_data_segcnt]) {
    kokkoro_sensor ans;
    for (auto i = 0; i < kokkoro_data_segcnt; ++i) for (auto j = 0; j < kokkoro_data_segcnt; ++j) {
        ans.sen_arr[j + i * kokkoro_data_segcnt] = raw_msg[i] & kokkoro_data_segcnt;
        raw_msg[i]                             >>= kokkoro_data_bitsz;
    }
    return ans;
}

kokkoro_sensor kokkoro_data_transfer(char raw_msg[kokkoro_data_segcnt]) {
    #if kokkoro_dcb_msg
    for (auto i = 0; i < kokkoro_data_segcnt; ++i) std::cout << '[' << std::bitset<8>(raw_msg[i]) << ']'; std::cout << std::endl;
    #endif
    auto ans = kokkoro_data_transfer_core(raw_msg);
    #if kokkoro_dcb_msg
    for (auto i = 0; i < kokkoro_data_sensz; i += 3) {
        std::cout << "[  ";
        for (auto j = kokkoro_data_segcnt; j; --j) std::cout << ' ' << ans.sen_arr[i + j - 1];
        std::cout << ']';
    }
    std::cout << std::endl;
    #endif
    return ans;
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
struct kokkoro_dcb_handle {
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

    kokkoro_queue<kokkoro_sensor> data_que;

    std::atomic_bool read_stop = false;
};

bool kokkoro_dcb_startup(kokkoro_dcb_handle &kokkoro_handle) { return dcb_startup(kokkoro_handle.h_port, kokkoro_handle.port_idx, kokkoro_handle.baudrate, kokkoro_handle.databits, kokkoro_handle.stopbits, kokkoro_handle.parity, kokkoro_handle.async_mode, kokkoro_handle.iobuf_sz, kokkoro_handle.iobuf_sz); }

bool kokkoro_dcb_shutdown(kokkoro_dcb_handle &kokkoro_handle) { return dcb_shutdown(kokkoro_handle.h_port); }

void kokkoro_array_read(kokkoro_dcb_handle &kokkoro_handle) { kokkoro_loop {
    if (kokkoro_handle.read_stop) break;
    auto buf_len = 1;
    char ch_tmp  = 0,
         *p_buf  = &ch_tmp,
         buf_tmp[kokkoro_data_segcnt] = {0};
    if (kokkoro_handle.start_pt == kokkoro_data_segcnt) {
        // normal condition
        p_buf   = buf_tmp;
        buf_len = kokkoro_data_segcnt;
    }
    buf_len = dcb_read(kokkoro_handle.h_port, p_buf, buf_len, kokkoro_handle.async_mode);
    if (!buf_len) continue;
    if (kokkoro_handle.start_pt < kokkoro_data_segcnt) {
        // get [3f 3f 3f]
        if (ch_tmp == kokkoro_dcb_start) ++kokkoro_handle.start_pt;
        else kokkoro_handle.start_pt = 0;
        continue;
    }
    if (kokkoro_data_blank_verify(p_buf)) continue;
    kokkoro_handle.data_que.en_queue(kokkoro_data_transfer(buf_tmp));
} }

KOKKORO_END