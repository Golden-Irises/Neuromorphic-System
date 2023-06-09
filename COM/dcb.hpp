bool dcb_open_port(void     *h_port,
                   int      idx                        = NULL,
                   dcbi32_t read_byte_interval_timeout = 50,
                   dcbi32_t read_byte_timeout          = 10,
                   dcbi32_t read_timeout               = 50,
                   dcbi32_t write_byte_timeout         = 10,
                   dcbi32_t write_timeout              = 50) {
    if (h_port) return false;
    // buffer
    char com_name[128];
    wsprintfA(com_name, "\\\\.\\COM%d", idx);
    // create handle of COM in Windows
    h_port = CreateFileA(com_name,
                         GENERIC_READ | GENERIC_WRITE,
                         NULL,
                         NULL,
                         OPEN_EXISTING,
                         NULL,
                         NULL);
    if (h_port == INVALID_HANDLE_VALUE) return false;

    // timeout interval for IO device (ms)
    COMMTIMEOUTS timeouts {read_byte_interval_timeout,
                           read_byte_timeout,
                           read_timeout,
                           write_byte_timeout,
                           write_timeout};
    // COM timeouts & Win32 event mask (>> get characters)
    if (!(SetCommTimeouts(h_port, &timeouts) &&
          SetCommMask(h_port, EV_RXCHAR))) return false;
    
    #if DCB_MSG
    std::printf("[Port switches on]\n");
    #endif

    return true;
}

bool dcb_close_port(void *h_port) { return CloseHandle(h_port); }

bool dcb_port_params(void *h_port,
                     int baudrate,
                     int databits,
                     int stopbits,
                     int parity = NOPARITY) {
    // Windows Device Control Block parameter for COM handle
    DCB serial_params {0};
    // DCB size of current Windows machine
    serial_params.DCBlength = sizeof(serial_params);
    // Current machine DCB status
    if (!GetCommState(h_port, &serial_params)) {
        #if DCB_MSG
        std::printf("[DCB Status is false]\n");
        #endif
        return false;
    }
    // Set COM baud rate
    serial_params.BaudRate  = baudrate;
    // Each data bits of COM IO
    serial_params.ByteSize  = databits;
    serial_params.StopBits  = stopbits;
    // Parity [NOPARITY | ODDPARITY | EVENPAERITY | MARKPARITY | SPACEPARITY]
    serial_params.Parity    = parity;
    return SetCommState(h_port, &serial_params);
}

// sizeof(char) = 1 byte
bool dcb_write(void *h_port, const char *src) {
    DWORD write_len   = std::strlen(src),
          written_len = 0;
    if (!WriteFile(h_port,
                   src,
                   write_len,
                   &written_len,
                   NULL)) {
        #if DCB_MSG
        std::printf("[DCB writes data failed]\n");
        #endif
        return false;
    }
    #if DCB_MSG
    std::printf("[Output Data][\n%s\n]", src);
    #endif
    return written_len == write_len;
}

// [dest] Buffer destination should be allocated memory with specified buffer length [buf_len]
bool dcb_read(void *h_port, char *buf_dest, int buf_len) {
    DWORD msg_mask = NULL, // Current task mask value
          data_len = NULL;
    
    // Wait for Windows message queue
    if (!WaitCommEvent(h_port, &msg_mask, NULL)) {
        #if DCB_MSG
        std::printf("Windows waiting handle is broken down\n");
        #endif
        return false;
    }

    // Read
    auto res = ReadFile(h_port, buf_dest, buf_len, &data_len, NULL);
    buf_dest[data_len] = '\0';
    return res;
}