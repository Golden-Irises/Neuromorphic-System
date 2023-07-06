KOKKORO_BEGIN

uint64_t kokkoro_lbl_no(char syb) { switch (syb) {
    case '+': return 0x0000;
    case '-': return 0x0001;
    case 'x': return 0x0002;
    case '/': return 0x0003;
    default: return kokkoro_array_sz;
} }

struct kokkoro_data{
    kokkoro_set<kokkoro_matrix> elem;
    kokkoro_set<uint64_t> lbl;
};

kokkoro_data kokkoro_train_data(const std::string &path) {
    auto data_raw = csv_in(path);
    kokkoro_data ans;
    if (!data_raw.length < 1) return ans;
    ans.elem.init(data_raw.length - 1);
    ans.lbl.init(data_raw.length - 1);
    for (auto i = 0ull; i < ans.lbl.length; ++i) {
        ans.lbl[i]  = std::atof(data_raw[i + 1][kokkoro_array_sz].c_str());
        ans.elem[i] = {kokkoro_array_sz, 1};
        for (auto j = 0; j < kokkoro_array_sz; ++j) ans.elem[i].index(j) = kokkoro_lbl_no(data_raw[i + 1][j][0]);
    }
    return ans;
}

/* get data from dcb */
struct kokkoro_dcb {
    kokkoro_queue<std::string> elem_raw;
    kokkoro_queue<kokkoro_matrix> elem_vect;
    async_pool dcb_pool;
    std::atomic_bool stop = false;
};

bool kokkoro_dcb_startup(kokkoro_dcb &dcb_info, int port_no = 3, int baudrate = 19200) {
    kokkoro_handle h_dcb = nullptr;
    if (!(dcb_open_port(h_dcb, port_no, 50, 10, 50, 10, 50) && 
          dcb_port_params(h_dcb, baudrate, 20, NOPARITY))) return false;
    dcb_info.dcb_pool.add_task([&dcb_info, h_dcb]{ while (true) {
        char buffer [DCB_BUF_LEN] = {0};
        if (!dcb_read(h_dcb, buffer, DCB_BUF_LEN)) return false;
        dcb_info.elem_raw.en_queue(buffer);
        if (dcb_info.stop) {
            if (!dcb_close_port(h_dcb)) return false;
            break;
        }
    } });
    return true;
}

void kokkoro_dcb_thread(kokkoro_dcb &dcb_info) { dcb_info.dcb_pool.add_task([&dcb_info]{ while (true) {
    kokkoro_matrix elem {kokkoro_array_sz, 1};
    auto raw_str = dcb_info.elem_raw.de_queue();
    // TODO: get matrix data from raw string value
    dcb_info.elem_vect.en_queue(std::move(elem));
    if (dcb_info.stop) break;
} }); }

void kokkoro_dcb_shutdown(kokkoro_dcb &dcb_info) {
    dcb_info.stop = true;
    dcb_info.elem_raw.reset();
    dcb_info.elem_vect.reset();
}

KOKKORO_END