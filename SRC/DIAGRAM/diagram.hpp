NEUNET_BEGIN

bool diagram_rect(int &width, int &height) {
    RECT rect;
    auto ans = GetWindowRect(GetConsoleWindow(), &rect);
    width    = rect.right - rect.left;
    height   = rect.bottom - rect.top;
    return ans;
}

bool diagram_font_sz(int &x, int &y) {
    CONSOLE_FONT_INFO font_info;
    auto ans = GetCurrentConsoleFont(GetStdHandle(STD_OUTPUT_HANDLE), false, &font_info);
    y        = font_info.dwFontSize.Y;
    x        = font_info.dwFontSize.X;
    return ans;
}

bool diagram_buffer_axis_sz(int &x, int &y) {
    CONSOLE_SCREEN_BUFFER_INFO buffer_info;
    auto ans = GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &buffer_info);
    x        = buffer_info.dwSize.X;
    y        = buffer_info.dwSize.Y;
    return ans;
}

template<typename fn>
int diagram_axis_point_buffer_sz(int src, fn &&os_s_operate = []{}) {
    auto ans = 0;
    for (auto i = src; src; src /= 10) {
        ++ans;
        os_s_operate();
    }
    return ans;
}

template<int point_cnt = __POINT_COUNT__, char point_show = '*'>
struct diagram_scroll_info {
    int point_que_front   = 0,// y_axis_point_dist = 1, 
        point_que_rear    = 0,// y_axis_point_cnt  = 2

        y_axis_point_cnt  = 0,// 20_    *     *
        y_axis_point_dist = 0,// |  
                              // 10_*
        x_axis_begin      = 1,// |                x_axis_begin    = 3
        x_axis_block_sz   = 1,// ___0____10___20... x_axis_block_sz = 5

        min_x = 0;

    double max_y = 0.,
           min_y = 0.;

    bool empty = true;

    #if __POINT_COUNT__
    
    double y_points[__POINT_COUNT__] = {0.};

    int y_axis_points[__POINT_COUNT__ * 2] = {0};
    
    #else
    
    double y_points[point_cnt] = {0.};
    
    int y_axis_points[point_cnt * 2] = {0};
    
    #endif
};

template<int point_cnt, char point_show>
void diagram_scroll_flush(diagram_scroll_info<point_cnt, point_show> &info) {
    info.x_axis_begin      = 1;
    info.x_axis_block_sz   = 1;
    info.y_axis_point_cnt  = 0;
    info.y_axis_point_dist = 0;
    std::system("cls");
}

template<int point_cnt, char point_show>
void diagram_scroll_add_point(diagram_scroll_info<point_cnt, point_show> &info, net_queue<double> &points_que) {
    auto y_point = points_que.de_queue();
    // if (info.empty) {
    //     info.empty = false;
    //     goto add_point;
    // }
    if (++info.point_que_rear == point_cnt) info.point_que_rear = 0;
    if (info.point_que_rear == info.point_que_front) {
        if (++info.point_que_front == point_cnt) info.point_que_front = 0;
        ++info.min_x;
    }
    add_point: info.y_points[info.point_que_rear] = y_point;
    info.max_y = *std::max_element(info.y_points, info.y_points + point_cnt);
    info.min_y = *std::min_element(info.y_points, info.y_points + point_cnt);
    if (info.min_y < 0)
    auto pause = true;
}

template<int point_cnt, char point_show>
int diagram_scroll_points_cnt(const diagram_scroll_info<point_cnt, point_show> &info) { return info.point_que_rear > info.point_que_front ? info.point_que_rear - info.point_que_front + 1 : info.point_que_rear + 1 + point_cnt - info.point_que_front; }

template<int point_cnt, char point_show>
bool diagram_scroll_update_axis(diagram_scroll_info<point_cnt, point_show> &info, int width, int height) {
    diagram_scroll_flush(info);
    auto y_axis_sgn = info.min_y < 0;
    if (y_axis_sgn) info.min_y *= (-1);
    auto y_axis_from = int(std::pow(10, std::log10(info.min_y))),
         y_axis_to   = int(std::pow(10, std::log10(info.max_y)) + 10);
    if (y_axis_sgn) {
        y_axis_from *= (-1);
        info.min_y  *= (-1);
    }
    if (y_axis_from > 10) y_axis_from -= 10;
    auto y_axis_area = y_axis_to - y_axis_from;
    auto font_y_sz   = 0;
    if (!diagram_font_sz(y_axis_from, font_y_sz)) return false;
    auto y_axis_height = height * 0.8,
         y_point_cnt   = y_axis_height / font_y_sz;
    while (y_point_cnt > point_cnt) y_point_cnt = y_axis_height / (font_y_sz * (++info.y_axis_point_dist + 1) + 1);
    auto y_axis_max  = y_axis_to * 1.,
         y_axis_unit = y_axis_area * 1. / y_point_cnt;
    for (auto i = 0; i < y_point_cnt; ++i) {
        std::cout << int(y_axis_max) << '\n';
        info.y_axis_points[info.y_axis_point_cnt++] = y_axis_max;
        y_axis_max -= y_axis_unit;
        for (auto j = 0; j < info.y_axis_point_dist; ++j) std::cout << '\n';
    }
    diagram_axis_point_buffer_sz(y_axis_to, [&info]{ std::cout << ' '; ++info.x_axis_begin; });
    std::cout << ' ';
    auto x_bufsz = 0;
    if (!diagram_buffer_axis_sz(x_bufsz, y_axis_from)) return false;
    x_bufsz             -= info.x_axis_begin;
    info.x_axis_block_sz = x_bufsz / point_cnt;
    for (auto i = 0; i < point_cnt; ++i) {
        auto x_axis_point = info.min_x + i;
        std::cout << x_axis_point;
        auto blank_bufsz = info.x_axis_block_sz - diagram_axis_point_buffer_sz(x_axis_point, []{});
        if (i + 1 < point_cnt) for (auto j = 0; j < blank_bufsz; ++j) std::cout << ' ';
    }
    return true;
}

template<int point_cnt, char point_show>
bool diagram_scroll_offset(const diagram_scroll_info<point_cnt, point_show> &info, int x_point, double y_point) {
    auto prev_tmp = 0,
         y_offset = 0;
    for (auto i = 0; i < info.y_axis_point_cnt; ++i) {
        auto y_axis_point = info.y_axis_points[i];
        if (y_point < y_axis_point) {
            prev_tmp = y_axis_point;
            if (i + 1 == info.y_axis_point_cnt) {
                y_offset = info.y_axis_point_cnt * (info.y_axis_point_dist + 1) - 1;
                break;
            }
            continue;
        }
        y_offset += i * (info.y_axis_point_dist + 1);
        if (y_point == y_axis_point || !info.y_axis_point_dist) break;
        auto y_axis_point_dif = (prev_tmp - y_axis_point) * 1. / (info.y_axis_point_dist + 1);
        for (auto j = y_axis_point + y_axis_point_dif; j < prev_tmp; j += y_axis_point_dif) {
            if (j > y_point) break;
            --y_offset;
        }
        break;
    }
    auto x_offset = x_point - info.min_x;
    x_offset     *= info.x_axis_block_sz;
    x_offset     += info.x_axis_begin;
    return SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), {short(x_offset), short(y_offset)});
}

template<int point_cnt, char point_show>
bool diagram_scroll_update_point(diagram_scroll_info<point_cnt, point_show> &info, net_queue<double> &points_que) {
    auto points_cnt = diagram_scroll_points_cnt(info);
    for (auto i = 0; i < points_cnt; ++i) {
        auto idx = (info.point_que_front + i) % point_cnt;
        diagram_scroll_offset(info, i + info.min_x, info.y_points[idx]);
        std::cout << point_show;
    }
    return true;
}

NEUNET_END