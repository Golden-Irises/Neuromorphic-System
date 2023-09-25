#pragma once
#pragma comment(lib, "user32.lib")

#ifndef __DCB_H__
#define __DCB_H__

#include <windows.h>
#include "../ANN/kokkoro_set"

#define dcbi32_t unsigned long

#ifndef DCB_BUF_LEN
#define DCB_BUF_LEN 0x0010
#endif

KOKKORO_BEGIN

/**
 * @brief Open DCB port for hardware device, reading timeout = bytes_count * (read_byte_timeout + max{read_byte_interval_timeout}) + read_timeout
 * @param h_port DCB port handle
 * @param idx DCB port index
 * @param read_byte_interval_timeout Max timeout for reading each byte data
 * @param read_byte_timeout Timeout for reading each byte data
 * @param read_timeout Total timeout after all bytes read
 * @param write_byte_timeout Timeout for writing each byte data
 * @param write_timeout Total timeout after all bytes write
 * @return Open port verification
 * @retval [true] Open port successfully
 * @retval [false] Open port failed
 */
bool dcb_open_port(void *, int, dcbi32_t, dcbi32_t, dcbi32_t, dcbi32_t, dcbi32_t, dcbi32_t, dcbi32_t);

bool dcb_close_port(void *);

/**
 * @brief Set DCB port parameters for protocol
 * @param h_port DCB port handle
 * @param baudrate Baudrate
 * @param databits Data bit count
 * @param stopbits Stop bits
 * @param parity Parity method [NOPARITY | ODDPARITY | EVENPAERITY | MARKPARITY | SPACEPARITY]
 * @return Setting port parameter verification
 * @retval [true] Parameter setting successfully
 * @retval [false] Parameter setting failed
 */
bool dcb_port_params(void *, int, int, int, int);

bool dcb_write(void *, const char *);

bool dcb_read(void *, char *, int);

KOKKORO_END

#include "dcb.hpp"

#endif