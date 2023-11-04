KOKKORO_BEGIN

class KokkoroCore : Kokkoro {
public:
    KokkoroCore() { kokkoro_array_startup(hArrayHandle); }

    void Run() {
        kokkoro_array_read_thread(hArrayHandle);
        kokkoro_array_save_thread(hArrayHandle);
        std::thread tdCtrl(kokkoro_array_control_thread, std::ref(hArrayHandle));
        kokkoro_loop {
            kokkoro_matrix vecIn {hArrayHandle.arr_que.de_queue().sen_arr, kokkoro_data_arrsz, 1};
            for (auto i = 0ull; i < iLayersCnt; ++i) arrLayers[i]->Deduce(vecIn);
            auto iPeakCnt = hArrayHandle.peak_cnt_que.de_queue();
            // TODO: output result & peak count
        }
        if (tdCtrl.joinable()) tdCtrl.join();
    }

    ~KokkoroCore() { kokkoro_array_shutdown(hArrayHandle); }

// COM DCB handle - No.3 port, 2400 baudrate, 400ms for each sampling, 6 databots, 1 stopbit, no parity
protected: kokkoro_array_handle hArrayHandle {3, CBR_2400, 400, 1024, 6, ONESTOPBIT, NOPARITY};

public:
    static int MasterMachineBattery(const std::string &sBatteryReportSavePath = "") {
        // powercfg /batteryreport /output "..\sBatteryReportSavePath.html"
        if (sBatteryReportSavePath.length()) WinExec(("powercfg /batteryreport /output \"" + sBatteryReportSavePath + ".html\"").c_str(), NULL);
        SYSTEM_POWER_STATUS power_stat;
        GetSystemPowerStatus(&power_stat);
        return power_stat.BatteryLifePercent;
    }

protected:
    static char DeduceResult(const kokkoro_matrix &vecOut) {
        auto iMaxIdx = 0ull;
        for (auto i = 1ull; i < vecOut.element_count; ++i) if (vecOut.index(i) > iMaxIdx) iMaxIdx = i;
        switch (iMaxIdx) {
        case 1: return '+';
        case 2: return '-';
        case 3: return 'x';
        case 4: return '/';
        default: return ' ';
        }
    }
};

KOKKORO_END