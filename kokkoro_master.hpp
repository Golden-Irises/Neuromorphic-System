KOKKORO_BEGIN

int MachineBatteryPercent() {
    // powercfg /batteryreport /output "..\html_file_path.html"
    SYSTEM_POWER_STATUS power_stat;
    GetSystemPowerStatus(&power_stat);
    return power_stat.BatteryLifePercent;
}



KOKKORO_END