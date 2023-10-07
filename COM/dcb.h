#pragma once
#pragma comment(lib, "user32.lib")

#ifndef __DCB_H__
#define __DCB_H__

#include <windows.h>
#include "../ANN/kokkoro_set"

#ifndef KOKKORO_DCB_BUF_SZ
#define KOKKORO_DCB_BUF_SZ 0x0010
#endif

#ifndef KOKKORO_DCB_HDL_SZ
#define KOKKORO_DCB_HDL_SZ 0x0010
#endif

KOKKORO_BEGIN 

// DCB handle
typedef int dcb_hdl[KOKKORO_DCB_HDL_SZ];

/**
 * @brief COM connection startup for DCB tansformation
 * @param h_port [in] DCB handle pointer
 * @param port_idx [in] DCB port index
 * @param baudrate [in] Baudrate index [CBR_110 | CBR_300 | CBR_600 | CBR_1200 | CBR_2400 | CBR_4800 | CBR_9600 | CBR_14400 | CBR_19200 | CBR_38400 | CBR_56000 | CBR_57600 | CBR_115200 | CBR_128000 | CBR_256000]
 * @param databits [in] Uplaoding bit count of each byte [5, 8]
 * @param stopbits [in] Stop bit count [ONESTOPBIT | ONE5STOPBITS | TWOSTOPBITS]
 * @param parity [in] Odd & even parity [NOPARITY | ODDPARITY | EVENPARITY | MARKPARITY | SPACEPARITY]
 * @param async_mode [in] Asynchronous mode switch [true - on | false - off]
 * @param in_buf_sz [in] DCB read stream buffer size
 * @param out_buf_sz [in] DCB write stream buffer size
 * @return Starup status
 * @retval true DCB startup successfully
 * @retval false DCB startup failed
 */
bool dcb_startup(dcb_hdl, int, int, int, int, int, bool, int, int);

/**
 * @brief COM connection shutdown
 * @param h_port [in] DCB handle pointer
 * @return Shutdown status
 * @retval true DCB shutdown successfully
 * @retval false DCB shutdown failed
 */
bool dcb_shutdown(dcb_hdl);

/**
 * @brief DCB output stream
 * @param h_port [in] DCB handle pointer
 * @param buf [in] Output data string buffer
 * @param buf_len [in] Output string length
 * @param async_mode [in] Asynchronous mode flag
 * @param pending_ms [in] Asynchronous IO pending time (ms)
 * @return Output status
 * @retval true Write successfully
 * @retval false Write startup failed
 */
bool dcb_write(dcb_hdl, const char *, int, bool, int);

/**
 * @brief DCB input stream
 * @param h_port [in] DCB handle pointer
 * @param buf_dest [out] Input data destination string pointer
 * @param buf_max_len [in] Input data buffer max length
 * @param async_mode [in] Asynchronous mode flag
 * @return Input data buffer length
 */
int dcb_read(dcb_hdl, char *, int, bool);

KOKKORO_END

#include "dcb.hpp"

#endif