KOKKORO_BEGIN

class KokkoroCore : Kokkoro {
public:
    static int MasterMachineBattery(const std::string &sBatteryReportSavePath = "") {
        // powercfg /batteryreport /output "..\sBatteryReportSavePath.html"
        if (sBatteryReportSavePath.length()) WinExec(("powercfg /batteryreport /output \"" + sBatteryReportSavePath + ".html\"").c_str(), NULL);
        SYSTEM_POWER_STATUS power_stat;
        GetSystemPowerStatus(&power_stat);
        return power_stat.BatteryLifePercent;
    }

protected:
    // COM DCB handle - No.3 port, 2400 baudrate, 400ms for each sampling, 6 databots, 1 stopbit, no parity
    kokkoro_array_handle array_handle {3, CBR_2400, 400, 1024, 6, ONESTOPBIT, NOPARITY};

public:    
};

KOKKORO_END