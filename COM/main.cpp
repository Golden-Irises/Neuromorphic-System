#include <iostream>
#include <Windows.h>

#define NS_NAME     nms
#define NS_BEGIN    namespace NS_NAME {
#define NS_END      }
#define NS_MSG      false

NS_BEGIN

bool dcb_open_port(HANDLE h_port, int idx = NULL) {
    if (h_port) return false;
    // buffer
    wchar_t com_name[128];
    wsprintf(com_name, TEXT("\\\\.\\COM%d"), idx);
    // create handle of COM in Windows
    h_port = CreateFile(com_name,
                        GENERIC_READ | GENERIC_WRITE,
                        NULL,
                        NULL,
                        OPEN_EXISTING,
                        NULL,
                        NULL);
    if (h_port == INVALID_HANDLE_VALUE) return false;

    // timeout interval for IO
    COMMTIMEOUTS timeouts {0};
    timeouts.ReadIntervalTimeout         = 50;
    timeouts.ReadTotalTimeoutConstant    = 50;
    timeouts.ReadTotalTimeoutMultiplier  = 10;
    timeouts.WriteTotalTimeoutConstant   = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    // COM timeouts & Win32 event mask (>> get characters)
    if (!(SetCommTimeouts(h_port, &timeouts) &&
          SetCommMask(h_port, EV_RXCHAR))) return false;
    
    #if NS_MSG
    std::printf("[Port switches on]\n");
    #endif

    return true;
}

bool dcb_close_port(HANDLE h_port) { return CloseHandle(h_port); }

bool dcb_port_params(HANDLE h_port,
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
        #if NS_MSG
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
bool dcb_write(HANDLE h_port, const char *src) {
    DWORD write_len   = std::strlen(src),
          written_len = 0;
    if (!WriteFile(h_port,
                   src,
                   write_len,
                   &written_len,
                   NULL)) {
        #if NS_MSG
        std::printf("[DCB writes data failed]\n");
        #endif
        return false;
    }
    #if NS_MSG
    std::printf("[Output Data][\n%s\n]", src);
    #endif
    return written_len == write_len;
}

// [dest] Buffer destination should be allocated memory with specified buffer length [buf_len]
bool dcb_read(HANDLE h_port, char *buf_dest, int buf_len) {
    DWORD msg_mask = NULL, // Current task mask value
          data_len = NULL;
    
    // Wait for Windows message queue
    if (!WaitCommEvent(h_port, &msg_mask, NULL)) {
        #if NS_MSG
        std::printf("Windows waiting handle is broken down\n");
        #endif
        return false;
    }

    // Read
    auto res = ReadFile(h_port, buf_dest, buf_len, &data_len, NULL);
    buf_dest[data_len] = '\0';
    return res;
}

NS_END

using namespace std;

int main(int argc, char *argv[], char *envp[]) {
    cout << "hello, world." << endl;
    return EXIT_SUCCESS;
}