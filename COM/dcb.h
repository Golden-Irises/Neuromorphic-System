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

typedef int dcb_hdl[KOKKORO_DCB_HDL_SZ];

bool dcb_startup(dcb_hdl, int, int, int, int, int, bool, int, int);

bool dcb_shutdown(dcb_hdl);

bool dcb_write(dcb_hdl, const char *, int, bool, int);

int dcb_read(dcb_hdl, char *, int, bool);

KOKKORO_END

#include "dcb.hpp"

#endif