KOKKORO_BEGIN

bool dcb_startup(dcb_hdl h_port,
                 int     port_idx   = 3,
                 int     baudrate   = CBR_9600,
                 int     databits   = 8,
                 int     stopbits   = ONESTOPBIT,
                 int     parity     = NOPARITY,
                 bool    async_mode = false,
                 int     in_buf_sz  = 1024,
                 int     out_buf_sz = 1024
                 ) {
    char port_name[KOKKORO_DCB_HDL_SZ] = {0};
    wsprintfA(port_name, "\\\\.\\COM%d", port_idx);

    auto h_tmp = CreateFileA(port_name, GENERIC_READ | GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, async_mode ? FILE_FLAG_OVERLAPPED : NULL, NULL);
    if (h_tmp == INVALID_HANDLE_VALUE) return false;

    DCB dcb_param;
    std::memset(&dcb_param, 0, sizeof(dcb_param));
    dcb_param.DCBlength = sizeof(dcb_param);
    if (!(SetupComm(h_tmp, in_buf_sz, out_buf_sz) &&
          GetCommState(h_tmp, &dcb_param))) return false;
    dcb_param.BaudRate = baudrate;
    dcb_param.ByteSize = databits;
    dcb_param.Parity   = parity;
    dcb_param.StopBits = stopbits;
    COMMTIMEOUTS dcb_timeouts {MAXDWORD, NULL, NULL, NULL, NULL};
    if (!(SetCommState(h_tmp, &dcb_param) &&
          SetCommTimeouts(h_tmp, &dcb_timeouts))) return false;

    std::memmove(h_port, &h_tmp, sizeof(h_tmp));
    return true;
}

bool dcb_refresh(dcb_hdl h_port) {
    auto h_tmp = *(HANDLE*)h_port;
    return PurgeComm(h_tmp, PURGE_TXCLEAR | PURGE_RXCLEAR) &&
           FlushFileBuffers(h_tmp);
}

bool dcb_shutdown(dcb_hdl h_port) {
    auto h_tmp = *(HANDLE*)h_port;
    return dcb_refresh(h_port) && CloseHandle(h_tmp);
}

bool dcb_write(dcb_hdl     h_port,
               const char *buf,
               int         buf_len,
               bool        async_mode = false,
               int         pending_ms = 1000) {
    DWORD buf_sz = buf_len;
    auto  h_tmp  = *(HANDLE*)h_port;
    if (!async_mode) return WriteFile(h_tmp, buf, buf_sz, &buf_sz, NULL);

    OVERLAPPED async_io;
    std::memset(&async_io, 0, sizeof(async_io));
    async_io.hEvent = CreateEventA(NULL, true, false, "WriteEvent");
    if (async_io.hEvent == INVALID_HANDLE_VALUE) return false;

    auto write_stat = WriteFile(h_tmp, buf, buf_sz, &buf_sz, &async_io);
    if (write_stat) return true;
    if (GetLastError() == ERROR_IO_PENDING) WaitForSingleObject(async_io.hEvent, pending_ms);
    
    COMSTAT dcb_stat;
    DWORD   dcb_err;
    ClearCommError(h_tmp, &dcb_err, &dcb_stat);
    return false;
}

int dcb_read(dcb_hdl h_port,
              char  *buf_dest,
              int    buf_max_len,
              bool   async_mode = false) {
    DWORD buf_sz = buf_max_len;
    auto  h_tmp  = *(HANDLE*)h_port;
    if (!async_mode) {
        if (ReadFile(h_tmp, buf_dest, buf_sz, &buf_sz, NULL)) return buf_sz;
        return 0;
    }

    OVERLAPPED async_io;
    async_io.hEvent = CreateEventA(NULL, true, false, "ReadEvent");
    if (async_io.hEvent == INVALID_HANDLE_VALUE) return 0;

    COMSTAT dcb_stat;
    DWORD   dcb_err;
    ClearCommError(h_tmp, &dcb_err, &dcb_stat);
	if (!dcb_stat.cbInQue) return false;

    auto read_stat = ReadFile(h_tmp, buf_dest, buf_sz, &buf_sz, &async_io);
    if (read_stat) {
        buf_dest[buf_sz - 1] = '\0';
        return buf_sz;
    }
    if (GetLastError() == ERROR_IO_PENDING) {
        GetOverlappedResult(h_tmp, &async_io, &buf_sz, true);
        return buf_sz;
    }
    ClearCommError(h_tmp, &dcb_err, &dcb_stat);
    return 0;
}

KOKKORO_END