NEUNET_BEGIN

bool diagram_buffer_sz(int &x, int &y) {
    // getting console buffer info status
    auto s = true;
    CONSOLE_SCREEN_BUFFER_INFO console_info;
    s = GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &console_info);
    x = console_info.srWindow.Right - console_info.srWindow.Left;
    y = console_info.srWindow.Bottom - console_info.srWindow.Top;
    return s;
}

int diagram_num_buffer_len(int src) {
    auto ans = 0;
    if (src <= 0) {
        ++ans;
        src *= -1;
    }
    for (auto i = src; i; i /= 10) ++ans;
    return ans;
}

template<int point_cnt = __POINT_COUNT__, char point_show = __POINT_PATTERN__>
struct diagram_scroll_info {
    int point_que_front   = 0,// y_axis_point_dist = 1, 
        point_que_rear    = 0,// y_axis_point_cnt  = 2

        y_axis_point_cnt  = 0,// 20_    *     *
        y_axis_point_dist = 1,// |  
                              // 10_*
        x_axis_begin      = 1,// |                  x_axis_begin    = 3
        x_axis_block_sz   = 1,// ___0____10___20... x_axis_block_sz = 5

        min_x             = 0;

    double max_y       = 0.,
           min_y       = 0.,

           y_unit      = 0;

    bool empty = true;

    #if __POINT_COUNT__
    
    double y_points[__POINT_COUNT__] = {0.};

    int y_axis_points[__POINT_COUNT__] = {0};
    
    #else
    
    double y_points[point_cnt] = {0.};
    
    int y_axis_points[point_cnt] = {0};
    
    #endif
};

#if __POINT_COUNT__
#define point_cnt  __POINT_COUNT__
#define point_show __POINT_PATTERN__
#else
template<int point_cnt, char point_show>
#endif
void diagram_scroll_flush(diagram_scroll_info<point_cnt, point_show> &info) {
    info.x_axis_begin      = 1;
    info.x_axis_block_sz   = 1;
    info.y_axis_point_cnt  = 0;
    info.y_axis_point_dist = 1;
    info.y_unit            = 0.;
    info.max_y             = 0.;
    info.min_y             = 0.;
    std::system("cls");
}

#if !__POINT_COUNT__
template<int point_cnt, char point_show>
#endif
void diagram_scroll_add_point(diagram_scroll_info<point_cnt, point_show> &info, net_queue<double> &points_que) {
    auto y_point = points_que.de_queue();
    if (info.empty) {
        info.empty = false;
        goto add_point;
    }
    if (++info.point_que_rear == point_cnt) info.point_que_rear = 0;
    if (info.point_que_rear == info.point_que_front) {
        if (++info.point_que_front == point_cnt) info.point_que_front = 0;
        ++info.min_x;
    }
    add_point: info.y_points[info.point_que_rear] = y_point;
    info.max_y = *std::max_element(info.y_points, info.y_points + point_cnt);
    info.min_y = *std::min_element(info.y_points, info.y_points + point_cnt);
}

#if !__POINT_COUNT__
template<int point_cnt, char point_show>
#endif
int diagram_scroll_points_cnt(const diagram_scroll_info<point_cnt, point_show> &info) {
    if (info.point_que_rear != info.point_que_front) return info.point_que_rear > info.point_que_front ? info.point_que_rear - info.point_que_front + 1 : info.point_que_rear + 1 + point_cnt - info.point_que_front;
    if (info.empty) return 0;
    return 1;
}

#if !__POINT_COUNT__
template<int point_cnt, char point_show>
#endif
bool diagram_scroll_update_axis(diagram_scroll_info<point_cnt, point_show> &info) {
    auto y_axis_min = std::pow(10, std::log10(std::abs(info.min_y))),
         y_axis_max = std::pow(10, std::log10(std::abs(info.max_y)));
    if (info.min_y < 0) y_axis_min *= -1;
    if (info.max_y < 0) y_axis_max *= -1;
    y_axis_max      += 10;
    y_axis_min      -= 1;
    auto y_axis_area = std::abs(y_axis_max - y_axis_min);
    auto y_buf_sz    = 0,
         x_buf_sz    = 0,
         x_max_bfsz  = diagram_num_buffer_len(info.min_x + point_cnt - 1);
    diagram_scroll_flush(info);
    info.x_axis_begin   += (std::max)(diagram_num_buffer_len(y_axis_min), diagram_num_buffer_len(y_axis_max));
    auto x_min_buffer_sz = (x_max_bfsz + 1) * (point_cnt - 1);
    if (!diagram_buffer_sz(x_buf_sz, y_buf_sz)) return false;
    auto x_axis_buffer_sz = ++x_buf_sz - info.x_axis_begin - x_max_bfsz,
         y_axis_point_cnt = (y_buf_sz - 1) / (info.y_axis_point_dist + 1);
    if (x_axis_buffer_sz < x_min_buffer_sz) {
        info.x_axis_block_sz = x_max_bfsz + 1;
        x_axis_buffer_sz     = info.x_axis_block_sz * (point_cnt - 1) + info.x_axis_begin + x_max_bfsz;
        SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), {short(x_axis_buffer_sz), short(y_buf_sz)});
    } else info.x_axis_block_sz = x_axis_buffer_sz / (point_cnt - 1);
    while (y_axis_point_cnt > point_cnt - 1) y_axis_point_cnt = (y_buf_sz - 1) / (++info.y_axis_point_dist + 1);
    info.y_unit           = y_axis_area / y_axis_point_cnt;
    auto y_axis_point_max = y_axis_max;
    if (info.y_unit < 1) info.y_unit = 1;
    else while (y_axis_point_max - y_axis_point_cnt * info.y_unit < y_axis_min) {
        auto next_y_axis_point_dist = 1 + info.y_axis_point_dist,
             next_y_axis_point_cnt  = (y_buf_sz - 1) / (next_y_axis_point_dist + 1);
        if (y_axis_point_max - next_y_axis_point_cnt * info.y_unit > y_axis_min) break;
        info.y_axis_point_dist = next_y_axis_point_dist;
        y_axis_point_cnt       = next_y_axis_point_cnt;
    }
    ++y_axis_point_cnt;
    for (auto i = 0; i < y_axis_point_cnt; ++i) {
        auto y_axis_point = y_axis_point_max + (y_axis_point_max < 0 ? -.5 : .5);
        if (int(y_axis_point) == info.y_axis_points[info.y_axis_point_cnt - 1]) continue;
        std::cout << int(y_axis_point) << '\n';
        info.y_axis_points[info.y_axis_point_cnt++] = y_axis_point;
        y_axis_point_max -= info.y_unit;
        if (i + 1 == y_axis_point_cnt) break;
        for (auto j = 0; j < info.y_axis_point_dist; ++j) std::cout << '\n';
    }
    for (auto i = 0; i < info.x_axis_begin; ++i) std::cout << ' ';
    for (auto i = 0; ; ++i) {
        auto x_axis_point = info.min_x + i;
        std::cout << x_axis_point;
        if (i + 1 == point_cnt) break;
        auto x_point_rem_buf_sz = info.x_axis_block_sz - diagram_num_buffer_len(x_axis_point);
        for (auto j = 0; j < x_point_rem_buf_sz; ++j) std::cout << ' ';
    }
    return true;
}

#if !__POINT_COUNT__
template<int point_cnt, char point_show>
#endif
bool diagram_scroll_offset(const diagram_scroll_info<point_cnt, point_show> &info, int y_point_idx) {
    double y_point = info.y_points[(info.point_que_front + y_point_idx) % point_cnt];
    short y_offset = 0;
    for (auto i = 0; i < info.y_axis_point_cnt; ++i) {
        auto y_axis_point = info.y_axis_points[i] * 1.;
        if (y_point < y_axis_point) continue;
        y_offset = i * (info.y_axis_point_dist + 1);
        if (y_point == y_axis_point) break;
        auto y_unit_dist = (info.y_axis_points[i - 1] - y_axis_point) / (info.y_axis_point_dist + 1);
        if (y_axis_point > 0) y_axis_point += y_unit_dist;
        while (y_point >= y_axis_point) {
            --y_offset;
            y_axis_point += y_unit_dist;
        }
        break;
    }
    short x_offset = y_point_idx * info.x_axis_block_sz + info.x_axis_begin;
    return SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), {short(x_offset), short(y_offset)});
}

#if !__POINT_COUNT__
template<int point_cnt, char point_show>
#endif
bool diagram_scroll_update_point(diagram_scroll_info<point_cnt, point_show> &info, net_queue<double> &points_que) {
    auto points_cnt = diagram_scroll_points_cnt(info);
    for (auto i = 0; i < points_cnt; ++i) {
        CONSOLE_CURSOR_INFO cursor_info {1, false};
        SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursor_info);
        diagram_scroll_offset(info, i);
        std::cout << point_show;
    }
    return true;
}

NEUNET_END