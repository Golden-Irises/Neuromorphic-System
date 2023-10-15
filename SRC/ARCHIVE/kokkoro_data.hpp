KOKKORO_BEGIN

void kokkoro_dcb_vect_mask(int *ans, char *src) { for (auto i = 0; i < kokkoro_dcb_mask; ++i) for (auto j = 0; j < kokkoro_dcb_mask; ++j) {
    ans[i + j * kokkoro_dcb_mask] = src[j] & kokkoro_dcb_mask;
    src[j]                      >>= kokkoro_dcb_bitsz;
} }

// int ans[9], char src[4] = {00, 00, 00, 3f}
void kokkoro_dcb_vect(int *ans, char *src) {
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

struct kokkoro_dcb_handle {
    int port_idx = 3,
        baudrate = CBR_9600,
        inter_ms = 400,
        buf_sz   = 1024,
        bat_sz   = inter_ms * baudrate / 1000 / kokkoro_dcb_unitsz;

    dcb_hdl dcb_port;
    
    kokkoro_queue<kokkoro_sensor> raw_data;

    // read process
    #if kokkoro_dcb_read_async

    async_pool read_proc = {kokkoro_dcb_arraysz};

    std::atomic_int read_proc_cnt = kokkoro_dcb_arraysz;

    std::atomic_bool read_proc_stop = false;

    async_controller read_curr_ctrl, read_max_ctrl;

    #endif

    kokkoro_sensor read_buff_tmp, read_curr_tmp, read_max_tmp;

    char read_buf[kokkoro_dcb_bytecnt] = {0};

    int read_data[kokkoro_dcb_arraysz] = {0},
        read_prev[kokkoro_dcb_arraysz] = {0},
        read_diff[kokkoro_dcb_arraysz] = {0},
        read_peak[kokkoro_dcb_arraysz] = {0};
};

// unsafe
void kokkoro_dcb_array_process(kokkoro_dcb_handle &kokkoro_handle, int arr_addr) {
    auto curr_tmp = &kokkoro_handle.read_curr_tmp.val_0,
         max_tmp  = &kokkoro_handle.read_max_tmp.val_0;
    if (curr_tmp[arr_addr] > max_tmp[arr_addr]) max_tmp[arr_addr] = curr_tmp[arr_addr];
    auto curr_dif = curr_tmp[arr_addr] - kokkoro_handle.read_prev[arr_addr];
    if (curr_dif ^ kokkoro_handle.read_diff[arr_addr] && curr_dif < 0) ++kokkoro_handle.read_peak[arr_addr];
    kokkoro_handle.read_prev[arr_addr] = curr_tmp[arr_addr];
    kokkoro_handle.read_diff[arr_addr] = curr_dif;
}

bool kokkoro_dcb_array_verify(const kokkoro_sensor &src) { return
    src.val_2 || src.val_1 || src.val_0 ||
    src.val_5 || src.val_4 || src.val_3 ||
    src.val_8 || src.val_7 || src.val_6; }

bool kokkoro_dcb_startup(kokkoro_dcb_handle &kokkoro_handle) {
    #if kokkoro_dcb_read_async

    for (auto i = 0; i < kokkoro_dcb_arraysz; ++i) kokkoro_handle.read_proc.add_task([i, &kokkoro_handle]{
        kokkoro_handle.read_max_ctrl.thread_sleep();
        if (kokkoro_handle.read_proc_stop) return;
        kokkoro_dcb_array_process(kokkoro_handle, i);
        if(++kokkoro_handle.read_proc_cnt == kokkoro_dcb_arraysz) kokkoro_handle.read_curr_ctrl.thread_wake_one();
    });

    #endif

    return dcb_startup(kokkoro_handle.dcb_port, kokkoro_handle.port_idx, kokkoro_handle.baudrate, kokkoro_dcb_databits, kokkoro_dcb_stopbits, kokkoro_dcb_parity, false, kokkoro_handle.buf_sz, kokkoro_handle.buf_sz);
}

bool kokkoro_dcb_shutdown(kokkoro_dcb_handle &kokkoro_handle) {
    #if kokkoro_dcb_read_async
    kokkoro_handle.read_proc_stop = true;
    kokkoro_handle.read_max_ctrl.thread_wake_all();
    #endif

    return dcb_shutdown(kokkoro_handle.dcb_port);
}

void kokkoro_dcb_array_read(kokkoro_dcb_handle &kokkoro_handle) { kokkoro_loop {
    auto data_buf_len = dcb_read(kokkoro_handle.dcb_port, kokkoro_handle.read_buf, kokkoro_dcb_bytecnt);
    /*if (!data_buf_len){
        //_sleep(1000);
        printf("0x0");
        break;
    }*/
    kokkoro_dcb_vect(kokkoro_handle.read_data, kokkoro_handle.read_buf);
    std::memmove(&kokkoro_handle.read_buff_tmp, kokkoro_handle.read_data, kokkoro_dcb_arraysz * sizeof(int));
    kokkoro_handle.raw_data.en_queue(kokkoro_handle.read_buff_tmp);
} }

void kokkoro_dcb_array_save(kokkoro_dcb_handle &kokkoro_handle) { for (auto i = 0; i < kokkoro_handle.bat_sz; ++i) {
    kokkoro_handle.read_curr_tmp = kokkoro_handle.raw_data.de_queue();
    if (!kokkoro_handle.raw_data.size()) return;

    #if kokkoro_dcb_read_async

    if (kokkoro_handle.read_proc_cnt < kokkoro_dcb_arraysz) kokkoro_handle.read_curr_ctrl.thread_sleep();
    kokkoro_handle.read_proc_cnt = 0;
    kokkoro_handle.read_max_ctrl.thread_wake_all();

    #else
    for (auto i = 0; i < kokkoro_dcb_arraysz; ++i) kokkoro_dcb_array_process(kokkoro_handle, i);
    #endif
} }

bool kokkoro_dcb_data_save(kokkoro_dcb_handle &kokkoro_handle, const std::string &csv_save_path, bool peak_stat = false) {
    std::ofstream of_file;
    of_file.open(csv_save_path, std::ios::out | std::ios::trunc);
    if (!(kokkoro_dcb_startup(kokkoro_handle) && of_file.is_open())) {
        of_file.close();
        kokkoro_dcb_shutdown(kokkoro_handle);
        return false;
    }
    char syb_num = 0;
    auto data_ok = false;
    async_pool data_read_task(1);
    data_read_task.add_task(kokkoro_dcb_array_read, std::ref(kokkoro_handle));
    kokkoro_loop {
        kokkoro_dcb_array_save(kokkoro_handle);

        // save to csv
        if (kokkoro_dcb_array_verify(kokkoro_handle.read_max_tmp)) { if (!data_ok) {
            data_ok = true;
            std::printf("[Symbol][+ -> 1 | - -> 2 | x -> 3 | / -> 4] ");
            std::scanf("%c", &syb_num);
        } } else if (data_ok) {
            data_ok = false;
            syb_num = '0';
        }
        auto p_data_tmp = &kokkoro_handle.read_max_tmp.val_0;
        for (auto i = 0; i < kokkoro_dcb_arraysz; ++i) {
            of_file << std::to_string(p_data_tmp[i]);
            if (i + 1 < kokkoro_dcb_arraysz) of_file << ',';
        }
        of_file << syb_num << std::endl;
        
        
        // print peak
        if (!peak_stat) continue;
        std::cout << "[Peak Count][";
        for (auto i = 0; i < kokkoro_dcb_arraysz; ++i) {
            std::cout << kokkoro_handle.read_peak[i];
            if (i + 1 < kokkoro_dcb_arraysz) std::cout << ' ';
        }
        std::cout << ']' << std::endl;
    }
    of_file.close();
    return kokkoro_dcb_shutdown(kokkoro_handle);
}

KOKKORO_END