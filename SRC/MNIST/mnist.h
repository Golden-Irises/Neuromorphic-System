#pragma once
#pragma comment(lib, "gdiplus.lib")

#ifndef __MNIST__
#define __MNIST__

#define MNIST_ELEM_MAGIC_NUM    0x0803
#define MNIST_LBL_MAGIC_NUM     0x0801

#define MNIST_ORGN_SZ           0x000a

#ifndef MNIST_MSG
#define MNIST_MSG true
#endif

#include <fstream>
#include <comdef.h>
#include <gdiplus.h>

#include "../../ANN/kokkoro_set"

KOKKORO_BEGIN

typedef struct { std::ifstream elem_file, lbl_file; } mnist_stream;

typedef struct {
    kokkoro_set<uint64_t> lbl;
    kokkoro_set<kokkoro_matrix> elem;
} mnist_data;

bool mnist_open(mnist_stream *, const char *, const char *);

void mnist_close(mnist_stream *);

bool mnist_magic_verify(mnist_stream *);

bool mnist_qty_verify(mnist_stream *, mnist_data *);

int mnist_ln_cnt(mnist_stream *);

int mnist_col_cnt(mnist_stream *);

void mnist_read(mnist_stream *, mnist_data *, int, int, uint64_t, bool);

kokkoro_set<uint64_t> mnist_idx(const mnist_data *);

bool mnist_save_image(const char *, const mnist_data *, int);

KOKKORO_END

#include "mnist.hpp"

#endif