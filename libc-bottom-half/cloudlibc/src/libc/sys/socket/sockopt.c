// Copyright (c) 2015-2016 Nuxi, https://nuxi.nl/
//
// SPDX-License-Identifier: BSD-2-Clause

#include <sys/socket.h>
#include <netinet/in.h>

#include <descriptor_table.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <wasi/api.h>
#include "__utils.h"

int tcp_getsockopt(tcp_socket_t* socket, int level, int optname,
    void* restrict optval, socklen_t* restrict optlen)
{
    int value = 0;

    wasi_sockets_0_2_0_rc_2023_10_18_network_error_code_t error;
    reactor_borrow_tcp_socket_t socket_borrow = wasi_sockets_0_2_0_rc_2023_10_18_tcp_borrow_tcp_socket(socket->socket);

    switch (level)
    {
    case SOL_SOCKET:
        switch (optname) {
        case SO_TYPE: {
            value = SOCK_STREAM;
            break;
        }
        case SO_PROTOCOL: {
            value = IPPROTO_TCP;
            break;
        }
        case SO_DOMAIN: {
            value = __wasi_sockets_utils__posix_family(
                wasi_sockets_0_2_0_rc_2023_10_18_tcp_method_tcp_socket_address_family(socket_borrow)
            );
            break;
        }
        case SO_ERROR: {
            if (socket->state_tag == TCP_SOCKET_STATE_CONNECT_FAILED) {
                value = __wasi_sockets_utils__map_error(socket->state.connect_failed.error_code);
                socket->state.connect_failed.error_code = 0;
            } else {
                value = 0;
            }
            break;
        }
        case SO_ACCEPTCONN: {
            // TODO: use 2023-11-10 snapshot to call out to tcp-socket::is-listening ?
            value = socket->state_tag == TCP_SOCKET_STATE_LISTENING;
            break;
        }
        case SO_KEEPALIVE: {
            bool result;
            if (!wasi_sockets_0_2_0_rc_2023_10_18_tcp_method_tcp_socket_keep_alive(socket_borrow, &result, &error)) {
                errno = __wasi_sockets_utils__map_error(error);
                return -1;
            }

            value = result;
            break;
        }
        case SO_RCVBUF: {
            uint64_t result;
            if (!wasi_sockets_0_2_0_rc_2023_10_18_tcp_method_tcp_socket_receive_buffer_size(socket_borrow, &result, &error)) {
                errno = __wasi_sockets_utils__map_error(error);
                return -1;
            }

            if (result > INT_MAX) {
                abort();
            }

            value = result;
            break;
        }
        case SO_SNDBUF: {
            uint64_t result;
            if (!wasi_sockets_0_2_0_rc_2023_10_18_tcp_method_tcp_socket_send_buffer_size(socket_borrow, &result, &error)) {
                errno = __wasi_sockets_utils__map_error(error);
                return -1;
            }

            if (result > INT_MAX) {
                abort();
            }

            value = result;
            break;
        }
        case SO_RCVTIMEO: // TODO: emulate in wasi-libc itself
        case SO_SNDTIMEO: // TODO: emulate in wasi-libc itself
        default:
            errno = ENOPROTOOPT;
            return -1;
        }
        break;


    case SOL_IP:
        switch (optname) {
        case IP_TTL: {
            if (wasi_sockets_0_2_0_rc_2023_10_18_tcp_method_tcp_socket_address_family(socket_borrow)
                != WASI_SOCKETS_0_2_0_RC_2023_10_18_NETWORK_IP_ADDRESS_FAMILY_IPV4)
            {
                errno = EAFNOSUPPORT;
                return -1;
            }

            uint8_t result;
            if (!wasi_sockets_0_2_0_rc_2023_10_18_tcp_method_tcp_socket_unicast_hop_limit(socket_borrow, &result, &error)) {
                errno = __wasi_sockets_utils__map_error(error);
                return -1;
            }

            value = result;
            break;
        }
        default:
            errno = ENOPROTOOPT;
            return -1;
        }
        break;


    case SOL_IPV6:
        switch (optname) {
        case IPV6_UNICAST_HOPS: {
            if (wasi_sockets_0_2_0_rc_2023_10_18_tcp_method_tcp_socket_address_family(socket_borrow)
                != WASI_SOCKETS_0_2_0_RC_2023_10_18_NETWORK_IP_ADDRESS_FAMILY_IPV6)
            {
                errno = EAFNOSUPPORT;
                return -1;
            }

            uint8_t result;
            if (!wasi_sockets_0_2_0_rc_2023_10_18_tcp_method_tcp_socket_unicast_hop_limit(socket_borrow, &result, &error)) {
                errno = __wasi_sockets_utils__map_error(error);
                return -1;
            }

            value = result;
            break;
        }
        case IPV6_V6ONLY: {
            bool result;
            if (!wasi_sockets_0_2_0_rc_2023_10_18_tcp_method_tcp_socket_ipv6_only(socket_borrow, &result, &error)) {
                errno = __wasi_sockets_utils__map_error(error);
                return -1;
            }

            value = result;
            break;
        }
        default:
            errno = ENOPROTOOPT;
            return -1;
        }
        break;


    case SOL_TCP:
        switch (optname) {
        case TCP_KEEPIDLE: // TODO: implement using the 2023-11-10 preview2 snapshot.
        case TCP_KEEPINTVL: // TODO: implement using the 2023-11-10 preview2 snapshot.
        case TCP_KEEPCNT: // TODO: implement using the 2023-11-10 preview2 snapshot.
        default:
            errno = ENOPROTOOPT;
            return -1;
        }
        break;


    default:
        errno = ENOPROTOOPT;
        return -1;
    }

    // Copy out integer value.
    memcpy(optval, &value,
        *optlen < sizeof(int) ? *optlen : sizeof(int));
    *optlen = sizeof(int);
    return 0;
}

int tcp_setsockopt(tcp_socket_t* socket, int level, int optname, const void* optval, socklen_t optlen) {
    
    int intval = *(int*)optval;
    
    wasi_sockets_0_2_0_rc_2023_10_18_network_error_code_t error;
    reactor_borrow_tcp_socket_t socket_borrow = wasi_sockets_0_2_0_rc_2023_10_18_tcp_borrow_tcp_socket(socket->socket);

    switch (level)
    {
    case SOL_SOCKET:
        switch (optname) {
        case SO_KEEPALIVE: {
            if (!wasi_sockets_0_2_0_rc_2023_10_18_tcp_method_tcp_socket_set_keep_alive(socket_borrow, intval != 0, &error)) {
                errno = __wasi_sockets_utils__map_error(error);
                return -1;
            }

            return 0;
        }
        case SO_RCVBUF: {
            if (!wasi_sockets_0_2_0_rc_2023_10_18_tcp_method_tcp_socket_set_receive_buffer_size(socket_borrow, intval, &error)) {
                errno = __wasi_sockets_utils__map_error(error);
                return -1;
            }

            return 0;
        }
        case SO_SNDBUF: {
            if (!wasi_sockets_0_2_0_rc_2023_10_18_tcp_method_tcp_socket_set_send_buffer_size(socket_borrow, intval, &error)) {
                errno = __wasi_sockets_utils__map_error(error);
                return -1;
            }

            return 0;
        }
        case SO_RCVTIMEO: // TODO: emulate in wasi-libc itself
        case SO_SNDTIMEO: // TODO: emulate in wasi-libc itself
        default:
            errno = ENOPROTOOPT;
            return -1;
        }
        break;


    case SOL_IP:
        switch (optname) {
        case IP_TTL: {
            if (wasi_sockets_0_2_0_rc_2023_10_18_tcp_method_tcp_socket_address_family(socket_borrow)
                != WASI_SOCKETS_0_2_0_RC_2023_10_18_NETWORK_IP_ADDRESS_FAMILY_IPV4)
            {
                errno = EAFNOSUPPORT;
                return -1;
            }

            if (intval < 0 || intval > UINT8_MAX) {
                errno = EINVAL;
                return -1;
            }

            if (!wasi_sockets_0_2_0_rc_2023_10_18_tcp_method_tcp_socket_set_unicast_hop_limit(socket_borrow, intval, &error)) {
                errno = __wasi_sockets_utils__map_error(error);
                return -1;
            }

            return 0;
        }
        default:
            errno = ENOPROTOOPT;
            return -1;
        }
        break;


    case SOL_IPV6:
        switch (optname) {
        case IPV6_UNICAST_HOPS: {
            if (wasi_sockets_0_2_0_rc_2023_10_18_tcp_method_tcp_socket_address_family(socket_borrow)
                != WASI_SOCKETS_0_2_0_RC_2023_10_18_NETWORK_IP_ADDRESS_FAMILY_IPV6)
            {
                errno = EAFNOSUPPORT;
                return -1;
            }

            if (intval < 0 || intval > UINT8_MAX) {
                errno = EINVAL;
                return -1;
            }

            if (!wasi_sockets_0_2_0_rc_2023_10_18_tcp_method_tcp_socket_set_unicast_hop_limit(socket_borrow, intval, &error)) {
                errno = __wasi_sockets_utils__map_error(error);
                return -1;
            }

            return 0;
        }
        case IPV6_V6ONLY: {
            if (!wasi_sockets_0_2_0_rc_2023_10_18_tcp_method_tcp_socket_set_ipv6_only(socket_borrow, intval != 0, &error)) {
                errno = __wasi_sockets_utils__map_error(error);
                return -1;
            }

            return 0;
        }
        default:
            errno = ENOPROTOOPT;
            return -1;
        }
        break;


    case SOL_TCP:
        switch (optname) {
        case TCP_KEEPIDLE: // TODO: implement using the 2023-11-10 preview2 snapshot.
        case TCP_KEEPINTVL: // TODO: implement using the 2023-11-10 preview2 snapshot.
        case TCP_KEEPCNT: // TODO: implement using the 2023-11-10 preview2 snapshot.
        default:
            errno = ENOPROTOOPT;
            return -1;
        }
        break;


    default:
        errno = ENOPROTOOPT;
        return -1;
    }
}

int udp_getsockopt(udp_socket_t* socket, int level, int optname,
    void* restrict optval, socklen_t* restrict optlen)
{
    int value;

    switch (level)
    {
    case SOL_SOCKET:
        switch (optname) {
        case SO_TYPE:
            value = SOCK_STREAM;
            break;
        case SO_PROTOCOL:
            value = IPPROTO_TCP;
            break;
        case SO_DOMAIN: // TODO: implement
        case SO_RCVBUF: // TODO: implement
        case SO_SNDBUF: // TODO: implement
        case SO_RCVTIMEO: // TODO: emulate in wasi-libc itself
        case SO_SNDTIMEO: // TODO: emulate in wasi-libc itself
        default:
            errno = ENOPROTOOPT;
            return -1;
        }
        break;


    case SOL_IP:
        switch (optname) {
        case IP_TTL: // TODO: implement
        default:
            errno = ENOPROTOOPT;
            return -1;
        }
        break;


    case SOL_IPV6:
        switch (optname) {
        case IPV6_V6ONLY: // TODO: implement
        case IPV6_UNICAST_HOPS: // TODO: implement
        default:
            errno = ENOPROTOOPT;
            return -1;
        }
        break;


    default:
        errno = ENOPROTOOPT;
        return -1;
    }

    // Copy out integer value.
    memcpy(optval, &value,
        *optlen < sizeof(int) ? *optlen : sizeof(int));
    *optlen = sizeof(int);
    return 0;
}

int udp_setsockopt(udp_socket_t* socket, int level, int optname, const void* optval, socklen_t optlen) {
    switch (level)
    {
    case SOL_SOCKET:
        switch (optname) {
        case SO_RCVBUF: // TODO: implement
        case SO_SNDBUF: // TODO: implement
        case SO_RCVTIMEO: // TODO: emulate in wasi-libc itself
        case SO_SNDTIMEO: // TODO: emulate in wasi-libc itself
        default:
            errno = ENOPROTOOPT;
            return -1;
        }
        break;


    case SOL_IP:
        switch (optname) {
        case IP_TTL: // TODO: implement
        default:
            errno = ENOPROTOOPT;
            return -1;
        }
        break;


    case SOL_IPV6:
        switch (optname) {
        case IPV6_V6ONLY: // TODO: implement
        case IPV6_UNICAST_HOPS: // TODO: implement
        default:
            errno = ENOPROTOOPT;
            return -1;
        }
        break;


    default:
        errno = ENOPROTOOPT;
        return -1;
    }
}

int getsockopt(int sockfd, int level, int optname,
    void* restrict optval, socklen_t* restrict optlen)
{
    descriptor_table_entry_t* entry;
    if (!descriptor_table_get_ref(sockfd, &entry)) {
        errno = EBADF;
        return -1;
    }

    if (optval == NULL || optlen == NULL || *optlen < sizeof(int)) {
        // FYI, the protocol-specific implementations implicitly depend on these checks.
        errno = EINVAL;
        return -1;
    }

    switch (entry->tag)
    {
    case DESCRIPTOR_TABLE_ENTRY_TCP_SOCKET:
        return tcp_getsockopt(&entry->value.tcp_socket, level, optname, optval, optlen);
    case DESCRIPTOR_TABLE_ENTRY_UDP_SOCKET:
        return udp_getsockopt(&entry->value.udp_socket, level, optname, optval, optlen);
    default:
        errno = ENOPROTOOPT;
        return -1;
    }
}

int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen) {

    descriptor_table_entry_t* entry;
    if (!descriptor_table_get_ref(sockfd, &entry)) {
        errno = EBADF;
        return -1;
    }

    if (optval == NULL || optlen < sizeof(int)) {
        // FYI, the protocol-specific implementations implicitly depend on these checks.
        errno = EINVAL;
        return -1;
    }

    switch (entry->tag)
    {
    case DESCRIPTOR_TABLE_ENTRY_TCP_SOCKET:
        return tcp_setsockopt(&entry->value.tcp_socket, level, optname, optval, optlen);
    case DESCRIPTOR_TABLE_ENTRY_UDP_SOCKET:
        return udp_setsockopt(&entry->value.udp_socket, level, optname, optval, optlen);
    default:
        errno = ENOPROTOOPT;
        return -1;
    }
}