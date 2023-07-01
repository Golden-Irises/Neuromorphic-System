NEUNET_BEGIN

bool diagram_console_sz(short width, short height) {
    auto h_console = GetStdHandle(STD_OUTPUT_HANDLE);
    auto co_ord    = COORD {width, height};
    auto win_rect  = SMALL_RECT{0, 0, --width, --height};
    return SetConsoleWindowInfo(h_console, true, &win_rect) &&
           SetConsoleScreenBufferSize(h_console, co_ord);
}

struct diagram_info {
    uint64_t console_wid = 1280,
             console_hgt = 720,
             pt_show_cnt = 20,
             x_intvl_cnt = 0,
             y_intvl_cnt = 0;
};

NEUNET_END