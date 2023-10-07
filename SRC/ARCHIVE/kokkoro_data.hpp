KOKKORO_BEGIN

uint64_t kokkoro_lbl_mask(char syb) { switch (syb) {
    case '+': return 0x0000;
    case '-': return 0x0001;
    case 'x': return 0x0002;
    case '/': return 0x0003;
    default: return 9;
} }

struct kokkoro_data {
    kokkoro_set<kokkoro_matrix> elem;
    kokkoro_set<uint64_t> lbl;
};

kokkoro_data kokkoro_train_data(const std::string &csv_load_path) {
    auto data_raw = csv_in(csv_load_path);
    kokkoro_data ans;
    if (!data_raw.length < 1) return ans;
    ans.elem.init(data_raw.length - 1);
    ans.lbl.init(data_raw.length - 1);
    for (auto i = 0ull; i < ans.lbl.length; ++i) {
        ans.lbl[i]  = std::atof(data_raw[i + 1][9].c_str());
        ans.elem[i] = {9, 1};
        for (auto j = 0; j < 9; ++j) ans.elem[i].index(j) = kokkoro_lbl_mask(data_raw[i + 1][j][0]);
    }
    return ans;
}

void kokkoro_dcb_vect_mask(int ans[9], char src[3]) { for (auto i = 0; i < 3; ++i) {
    ans[i]     = (src[i] & kokkoro_dcb_mask);
    ans[i + 3] = (src[i] & kokkoro_dcb_mask);
    ans[i + 6] = (src[i] & kokkoro_dcb_mask);
    src[i]   >>= kokkoro_dcb_bitsz;
    src[i]   >>= kokkoro_dcb_bitsz;
    src[i]   >>= kokkoro_dcb_bitsz;
} }

template <typename = std::enable_if_t<kokkoro_dcb_bufsz % kokkoro_dcb_thdsz == 0>>
bool kokkoro_dcb_read_thread(dcb_hdl dcb_port, int baudrate = CBR_9600, int buf_sz = 1024) {
    if (!dcb_startup(dcb_port, 3, baudrate, 6, ONESTOPBIT, NOPARITY, false, buf_sz, buf_sz)) return false;
    constexpr auto proc_sz = kokkoro_dcb_bufsz / kokkoro_dcb_thdsz;
    #if kokkoro_dcb_msg
    std::printf("[Startup DCB port.]");
    #endif
    char dcb_buffer[kokkoro_dcb_bufsz] = {0};
    auto buffer_len = 0;
    do {
        buffer_len = dcb_read(dcb_port, dcb_buffer, kokkoro_dcb_bufsz);
        if (!buffer_len) return false;
        
    } while (buffer_len);
    return true;
}

KOKKORO_END