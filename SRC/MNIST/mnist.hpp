int mnist_num_rev(int src) {
    uint8_t tmp_0, tmp_1, tmp_2, tmp_3;
    tmp_0 = src & 0x00ff;
    tmp_1 = (src >> 0x0008) & 0x00ff;
    tmp_2 = (src >> 0x0010) & 0x00ff;
    tmp_3 = (src >> 0x0018) & 0x00ff;
    return ((int)tmp_0 << 0x0018) + ((int)tmp_1 << 0x0010) + ((int)tmp_2 << 0x0008) + tmp_3;
}

bool mnist_open(mnist_stream *stream, const char *elem_path, const char *lbl_path) {
    stream->elem_file = std::ifstream {elem_path, std::ios::binary};
    stream->lbl_file  = std::ifstream {lbl_path, std::ios::binary};
    return stream->elem_file.is_open() && stream->lbl_file.is_open();
}

void mnist_close(mnist_stream *stream) {
    stream->elem_file.close();
    stream->lbl_file.close();
}

bool mnist_magic_verify(mnist_stream *stream) {
    auto elem_magic = 0,
         lbl_magic  = 0;
    stream->elem_file.read((char *)&elem_magic, sizeof(int));
    stream->lbl_file.read((char *)&lbl_magic, sizeof(int));
    return mnist_num_rev(elem_magic) == MNIST_ELEM_MAGIC_NUM && mnist_num_rev(lbl_magic) == MNIST_LBL_MAGIC_NUM;
}

bool mnist_qty_verify(mnist_stream *stream, mnist_data *src) {
    int elem_cnt = 0,
        lbl_cnt  = 0;
    stream->elem_file.read((char *)&elem_cnt, sizeof(int));
    stream->lbl_file.read((char *)&lbl_cnt, sizeof(int));
    elem_cnt = mnist_num_rev(elem_cnt);
    lbl_cnt  = mnist_num_rev(lbl_cnt);
    if (elem_cnt != lbl_cnt) return false;
    src->elem.init(elem_cnt, false);
    src->lbl.init(lbl_cnt, false);
    return true;
}

int mnist_ln_cnt(mnist_stream *stream) {
    auto ans = 0;
    stream->elem_file.read((char *)&ans, sizeof(int));
    return mnist_num_rev(ans);
}

int mnist_col_cnt(mnist_stream *stream) { return mnist_ln_cnt(stream); }

void mnist_read(mnist_stream *stream, mnist_data *src, int ln_cnt, int col_cnt, uint64_t padding = 0, bool im2col = false) {
    auto elem_cnt = ln_cnt * col_cnt;
    for (auto i = 0; i < src->elem.length; ++i) {
        char tmp = 0;
        stream->lbl_file.read(&tmp, 1);
        src->lbl[i] = tmp;
        if (im2col) src->elem[i] = {uint64_t(elem_cnt), 1};
        else src->elem[i] = {(uint64_t)ln_cnt, (uint64_t)col_cnt};
        for (auto j = 0; j < elem_cnt; ++j) {
            stream->elem_file.read(&tmp, 1);
            if (im2col) src->elem[i].index(j) = tmp;
            else if (tmp) src->elem[i].index(j) = 0x00ff;
        }
    }
}

void mnist_clsid(CLSID *src) {
    // Number of image encoders
    uint32_t encode_num = 0,
    // Size of the image encoder array in bytes
             _size      = 0;
    Gdiplus::GetImageEncodersSize(&encode_num, &_size);
    if (!_size) return;
    auto img_code_info_ptr = new Gdiplus::ImageCodecInfo[_size];
    if (!img_code_info_ptr) return;
    Gdiplus::GetImageEncoders(encode_num, _size, img_code_info_ptr);
    for (auto i = 0ull; i < encode_num; ++i) {
        if (!std::wcscmp(img_code_info_ptr[i].MimeType, L"image/png")) {
            *src = img_code_info_ptr[i].Clsid;
            break;
        }
    }
    while (img_code_info_ptr) {
        delete [] img_code_info_ptr;
        img_code_info_ptr = nullptr;
    }
}

std::wstring mnist_s2ws(const std::string &src) {
    auto p_str = new wchar_t[src.length()];
    MultiByteToWideChar(0, 0, src.c_str(), src.length(), p_str, src.length());
    std::wstring ans {p_str};
    while (p_str) {
        delete [] p_str;
        p_str = nullptr;
    }
    return ans;
}

neunet::net_set<uint64_t> mnist_idx(const mnist_data *src) {
    neunet::net_set<uint64_t> ans(src->elem.length);
    for (auto i = 0ull; i < ans.length; ++i) ans[i] = i;
    ans.shuffle();
    return ans;
}

bool mnist_save_image(const char *dir, const mnist_data *src, int expe_save_qty = 0) {
    if (!(src->elem.length && src->elem.length == src->lbl.length)) return false;
    std::string path {dir};
    int ln_cnt  = src->elem[0].line_count,
        col_cnt = src->elem[0].column_count;
    if (!expe_save_qty) expe_save_qty = src->elem.length;
    uint64_t gph_token {0};
    Gdiplus::Status status = Gdiplus::Ok;
    Gdiplus::GdiplusStartupInput st_gph;
    if (Gdiplus::GdiplusStartup(&gph_token, &st_gph, nullptr) != Gdiplus::Ok) return false;
    CLSID clsid {0};
    mnist_clsid(&clsid);
    status_ok: {
    Gdiplus::Bitmap bitmap {col_cnt, ln_cnt, PixelFormat32bppRGB};
    Gdiplus::Graphics gph_img {&bitmap};
    for (auto i = 0; i < expe_save_qty; ++i) {
        for (auto j = 0; j < ln_cnt; ++j) for (auto k = 0; k < col_cnt; ++k) {
            uint8_t color_tmp = src->elem[i][j][k];
            Gdiplus::Pen curr_pen {Gdiplus::Color{color_tmp, color_tmp, color_tmp}};
            status = gph_img.DrawLine(&curr_pen, int(k), int(j), int(k + 1), int(j + 1));
            if (status != Gdiplus::Ok) goto status_err;
        }
        auto path_tmp = mnist_s2ws(path + "\\[" + std::to_string(i) + ']' + std::to_string(src->lbl[i]) + ".png");
        #if MNIST_MSG
        std::wcout << mnist_s2ws(path_tmp) << std::endl;
        #endif
        status = bitmap.Save(path_tmp.c_str(), &clsid);
        if (status != Gdiplus::Ok) break;
    } } status_err: Gdiplus::GdiplusShutdown(gph_token);
    return status == Gdiplus::Ok;
}
