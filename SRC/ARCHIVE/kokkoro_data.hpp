KOKKORO_BEGIN

// unsafe
void kokkoro_dcb_vect_mask(int *ans, char *src) { for (auto i = 0; i < kokkoro_dcb_mask; ++i) for (auto j = 0; j < kokkoro_dcb_mask; ++j) {
    ans[i + j * kokkoro_dcb_mask] = src[j] & kokkoro_dcb_mask;
    src[j]                      >>= kokkoro_dcb_bitsz;
} }

bool kokkoro_dcb_read_thread(dcb_hdl dcb_port, int baudrate = CBR_9600, int buf_sz = 1024) {
    if (!dcb_startup(dcb_port, 3, baudrate, 6, ONESTOPBIT, NOPARITY, false, buf_sz, buf_sz)) return false;
    #if kokkoro_dcb_msg
    std::printf("[Startup DCB port.]");
    #endif
    char dcb_buf[4]  = {0};
    int  buffer_len  = 0,
         data_tmp[9] = {0};
    do {
        buffer_len = dcb_read(dcb_port, dcb_buf, 4);
        if (!buffer_len || (*dcb_buf) == kokkoro_dcb_idle) return false;
        // #if kokkoro_dcb_msg
        char bi_str[10] = {0};
        for (auto i = 0; i < 4; ++i) {
            itoa(dcb_buf[i], bi_str, 2);
            std::printf("[00%s]", bi_str);
        }
        // #endif
        kokkoro_dcb_vect_mask(data_tmp, dcb_buf);
    } while (buffer_len);
    return true;
}

KOKKORO_END