// Copyright (c) 2015-2017 Nuxi, https://nuxi.nl/
//
// SPDX-License-Identifier: BSD-2-Clause

#include <sys/socket.h>

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <wasi/api.h>

#include <descriptor_table.h>

static_assert(MSG_PEEK == __WASI_RIFLAGS_RECV_PEEK, "Value mismatch");
static_assert(MSG_WAITALL == __WASI_RIFLAGS_RECV_WAITALL, "Value mismatch");

ssize_t recv(int socket, void* restrict buffer, size_t length, int flags)
{
#if 0
    // Validate flags.
    if ((flags & ~(MSG_PEEK | MSG_WAITALL)) != 0) {
        errno = EOPNOTSUPP;
        return -1;
    }

    // Prepare input parameters.
    __wasi_iovec_t iov = { .buf = buffer, .buf_len = length };
    __wasi_iovec_t* ri_data = &iov;
    size_t ri_data_len = 1;
    __wasi_riflags_t ri_flags = flags;

    // Perform system call.
    size_t ro_datalen;
    __wasi_roflags_t ro_flags;
    __wasi_errno_t error = __wasi_sock_recv(socket,
        ri_data, ri_data_len, ri_flags,
        &ro_datalen,
        &ro_flags);
    if (error != 0) {
        errno = error;
        return -1;
    }
    return ro_datalen;
#else
    // TODO: what flags do we support?
    if (flags != 0) {
        errno = EOPNOTSUPP;
        return -1;
    }

    descriptor_table_variant_t variant;
    if (!descriptor_table_get(socket, &variant)) {
        errno = EBADF;
        return -1;
    }

    bool blocking;
    reactor_own_input_stream_t rx;
    switch (variant.tag) {
    case DESCRIPTOR_TABLE_VARIANT_TCP_CONNECTED:
        blocking = variant.value.tcp_connected.socket.blocking;
        rx = variant.value.tcp_connected.rx;
        break;

    default:
        errno = ENOTCONN;
        return -1;
    }

    reactor_borrow_input_stream_t rx_borrow = wasi_io_0_2_0_rc_2023_10_18_streams_borrow_input_stream(rx);
    while (true) {
        reactor_list_u8_t result;
        wasi_io_0_2_0_rc_2023_10_18_streams_stream_error_t error;
        if (!wasi_io_0_2_0_rc_2023_10_18_streams_method_input_stream_read(rx_borrow, length, &result, &error)) {
            // TODO: map errors appropriately
            errno = EBADF;
            return -1;
        }

        if (result.len) {
            memcpy(buffer, result.ptr, result.len);
            reactor_list_u8_free(&result);
            return result.len;
        } else if (blocking) {
            reactor_own_pollable_t pollable = wasi_io_0_2_0_rc_2023_10_18_streams_method_input_stream_subscribe(rx_borrow);
            reactor_borrow_pollable_t pollable_borrow = wasi_io_0_2_0_rc_2023_10_18_poll_borrow_pollable(pollable);
            wasi_io_0_2_0_rc_2023_10_18_poll_poll_one(pollable_borrow);
            wasi_io_0_2_0_rc_2023_10_18_poll_pollable_drop_own(pollable);
        } else {
            errno = EAGAIN;
            return -1;
        }
    }
#endif
}
