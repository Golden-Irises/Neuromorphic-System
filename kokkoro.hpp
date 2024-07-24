KOKKORO_BEGIN

class KokkoroCore : public Kokkoro {
public: static int MasterMachineBattery(const std::string &sBatteryReportSavePath = "") {
    // powercfg /batteryreport /output "..\sBatteryReportSavePath.html"
    if (sBatteryReportSavePath.length()) WinExec(("powercfg /batteryreport /output \"" + sBatteryReportSavePath + ".html\"").c_str(), NULL);
    SYSTEM_POWER_STATUS power_stat;
    GetSystemPowerStatus(&power_stat);
    return power_stat.BatteryLifePercent;
}

protected: static char DeduceResult(const kokkoro_matrix &vecOut) {
    auto iMaxIdx = 0ull;
    for (auto i = 1ull; i < vecOut.element_count; ++i) if (vecOut.index(i) > vecOut.index(iMaxIdx)) iMaxIdx = i;
    switch (iMaxIdx) {
    case 1: return '+';
    case 2: return '-';
    case 3: return 'x';
    case 4: return '/';
    default: return ' ';
    }
}

public:
    // COM DCB handle - No.3 port, 2400 baudrate, 400ms for each sampling, 6 databots, 1 stopbit, no parity
    KokkoroCore(int port_idx = 3, int baudrate = CBR_2400, int intrv_ms = 400, int iobuf_sz = 1024, int databits = 6, int stopbits = ONESTOPBIT, int parity = NOPARITY) {
        hArrayHandle.port_idx = port_idx;
        hArrayHandle.baudrate = baudrate;
        hArrayHandle.intrv_ms = intrv_ms;
        hArrayHandle.iobuf_sz = iobuf_sz;
        hArrayHandle.databits = databits;
        hArrayHandle.stopbits = stopbits;
        hArrayHandle.parity   = parity;
        kokkoro_array_startup(hArrayHandle);
    }

    void Run() {
        kokkoro_array_read_thread(hArrayHandle);
        kokkoro_array_save_thread(hArrayHandle);
        kokkoro_loop {
            kokkoro_matrix vecIn {hArrayHandle.arr_que.de_queue().sen_arr, kokkoro_data_arrsz, 1};
            std::cout << "[Input][";
            for (auto i = 0ull; i < kokkoro_data_arrsz; ++i) {
                std::cout << vecIn.index(i);
                if (i + 1 < kokkoro_data_arrsz) std::cout << ' ';
            }

            for (auto i = 0ull; i < iLayersCnt; ++i) arrLayers[i]->Deduce(vecIn);
            auto iPeakCnt = hArrayHandle.peak_cnt_que.de_queue();
            // output result & peak count

            std::cout << "][Peak Count][";
            for (auto i = 0; i < kokkoro_data_arrsz; ++i) {
                std::cout << iPeakCnt.sen_arr[i];
                if (i + 1 < kokkoro_data_arrsz) std::cout << ' ';
            }
            // std::cout << ']' << std::endl;
            std::cout << "][Symbol][" << DeduceResult(vecIn) << ']' << std::endl;
        }
    }

    ~KokkoroCore() { kokkoro_array_shutdown(hArrayHandle); }

protected: kokkoro_array_handle hArrayHandle {};
};

KOKKORO_END