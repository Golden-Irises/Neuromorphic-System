NEUNET_BEGIN

struct diagram_pt { double x = 0., y = 0.; };

diagram_pt diagram_console_rect() {
    RECT rect;
    GetWindowRect(GetConsoleWindow(), &rect);
    return {double(rect.right - rect.left), double(rect.bottom - rect.top)};
}

template<int point_count = 20>
struct diagram_scroll_info {
    int pt_front_idx = 0,
        pt_rear_idx  = 0,
        
        y_axis_len = 0,
        y_axis_dy  = 0,
        
        x_left     = 1;

    #if __POINT_COUNT__
    diagram_pt pt_arr[point_count];
    
    int y_axis[point_count];
    #else
    diagram_pt pt_arr[20];
    
    int y_axis_val[20];
    #endif

    double max_y = 0.,
           min_y = 0.;
};

template <int pt_cnt>
void diagram_scroll_add_point(diagram_scroll_info<pt_cnt> &src, net_queue<diagram_pt> &pt_que) {
    auto point = pt_que.de_queue();
    if (src.max_y < point.y) src.max_y = point.y;
    if (src.min_y > point.y) src.min_y = point.y;
    if (++src.pt_rear_idx == pt_cnt) src.pt_rear_idx = 0;
    if (src.pt_front_idx == src.pt_rear_idx) if (++src.pt_front_idx == pt_cnt) src.pt_front_idx = 0;
    src.pt_arr[src.pt_rear_idx] = std::move(point);
}

template <int pt_cnt>
bool diagram_scroll_full(const diagram_scroll_info<pt_cnt> &src) {
    auto for_rear = src.pt_rear_idx + 1;
    if (for_rear == pt_cnt) return true;
    return for_rear == src.pt_front_idx;
}

template <int pt_cnt>
int diagram_scroll_len(const diagram_scroll_info<pt_cnt> &src) { src.pt_front_idx > src.pt_rear_idx ? pt_cnt - src.pt_front_idx + src.pt_rear_idx + 1 : src.pt_rear_idx - src.pt_front_idx + 1; }

int diagram_scroll_colszx(int max_x) {
    auto ans = 0;
    while (max_x) {
        max_x /= 10;
        ++ans;
    }
    return ans;
}

void diagram_scroll_printx(int src, int buf_cnt) {
    auto src_colsz = diagram_scroll_colszx(src);
    std::cout << src;
    buf_cnt -= src_colsz;
    for (auto i = 0; i < buf_cnt; ++i) std::cout << ' ';
}

template <int pt_cnt>
int diagram_scroll_axis(diagram_scroll_info<pt_cnt> &src, int min_x = 0) {
    auto wnd_sz = diagram_console_rect();
    auto y_from = int(std::pow(10, std::log10(src.min_y))),
         y_to   = int(std::pow(10, std::log10(src.max_y)) + 10);
    if (y_from < 10) y_from = 0;
    else y_from -= 10;
    auto y_area = y_to - y_from;
    CONSOLE_FONT_INFO font_info;
    GetCurrentConsoleFont(GetStdHandle(STD_OUTPUT_HANDLE), false, &font_info);
    auto y_font_sz = font_info.dwFontSize.Y,
         x_font_sz = font_info.dwFontSize.X;
    auto y_axis    = wnd_sz.y * 0.8 / y_font_sz,
         y_buffer  = y_axis / pt_cnt,
         y_unit    = y_area / y_axis,
         y_show    = y_to * 1.;
    while (y_axis > pt_cnt) {
        ++src.y_axis_dy;
        y_axis /= (src.y_axis_dy + 1);
    }
    for (auto i = 0; i < y_axis; ++i) {
        std::cout << (int)y_show << '\n';
        src.y_axis_val[src.y_axis_len++] = y_show;
        for (auto j = 0; j < src.y_axis_dy; ++j) {
            y_show -= y_unit;
            std::cout << '\n';
        }
    }
    while (y_to) {
        std::cout << ' ';
        y_to /= 10;
        ++src.x_left;
    }
    std::cout << ' ';
    CONSOLE_SCREEN_BUFFER_INFO buffer_info;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &buffer_info);
    auto x_axis_buf = buffer_info.dwSize.X - src.x_left,
         x_buf_cnt  = x_axis_buf / pt_cnt;
    for (auto i = 0; i < pt_cnt; ++i) diagram_scroll_printx(i, x_buf_cnt);
    std::cout << '\n';
    ++src.x_left;
    return ++min_x;
}

template <int pt_cnt>
diagram_pt diagram_scroll_offset(const diagram_scroll_info<pt_cnt> &src, const diagram_pt &point) {
    return {0, 0};
}

// return next min_x
template <int pt_cnt>
int diagram_scroll_print(diagram_scroll_info<pt_cnt> &src, net_queue<diagram_pt> &pt_que, int min_x = 0) {
    std::system("cls");
    auto ans = diagram_scroll_axis(src, min_x);
    diagram_scroll_add_point(src, pt_que);
    auto pt_len = diagram_scroll_len(src);
    for (auto i = 0; i < pt_len; ++i) {
        auto idx = (src.pt_front_idx + i) % pt_cnt;
        // put point(s)
    }
    src.y_axis_dy = 0;
    src.x_left    = 1;
    if (diagram_scroll_full(src)) return ans;
    return --ans;
}

NEUNET_END