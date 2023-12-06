#include <errno.h>
#include <netinet/in.h>

#include <descriptor_table.h>
#include "__utils.h"


int tcp_connect(tcp_socket_t* socket, network_ip_socket_address_t* address)
{
    switch (socket->state.tag) {
    case TCP_SOCKET_STATE_UNBOUND:
    case TCP_SOCKET_STATE_BOUND:
        // These can initiate a connect.
        break;
    case TCP_SOCKET_STATE_CONNECTING:
        errno = EALREADY;
        return -1;
    case TCP_SOCKET_STATE_CONNECTED:
        errno = EISCONN;
        return -1;
    case TCP_SOCKET_STATE_CONNECT_FAILED: // POSIX: "If connect() fails, the state of the socket is unspecified. Conforming applications should close the file descriptor and create a new socket before attempting to reconnect."
    case TCP_SOCKET_STATE_LISTENING:
    default:
        errno = EOPNOTSUPP;
        return -1;
    }

    network_error_code_t error;
    network_borrow_network_t network_borrow = __wasi_sockets_utils__borrow_network();
    tcp_borrow_tcp_socket_t socket_borrow = tcp_borrow_tcp_socket(socket->socket);

    if (!tcp_method_tcp_socket_start_connect(socket_borrow, network_borrow, address, &error)) {
        errno = __wasi_sockets_utils__map_error(error);
        return -1;
    }

    // Connect has successfully started.
    socket->state = (tcp_socket_state_t){ .tag = TCP_SOCKET_STATE_CONNECTING, .connecting = { /* No additional state */ } };

    // Attempt to finish it:
    tcp_tuple2_own_input_stream_own_output_stream_t io;
    while (!tcp_method_tcp_socket_finish_connect(socket_borrow, &io, &error)) {
        if (error == NETWORK_ERROR_CODE_WOULD_BLOCK) {
            if (socket->blocking) {
                poll_borrow_pollable_t pollable_borrow = poll_borrow_pollable(socket->socket_pollable);
                poll_method_pollable_block(pollable_borrow);
            } else {
                errno = EINPROGRESS;
                return -1;
            }
        } else {
            socket->state.tag = TCP_SOCKET_STATE_CONNECT_FAILED;
            socket->state = (tcp_socket_state_t){ .connect_failed = {
                .error_code = error,
            } };

            errno = __wasi_sockets_utils__map_error(error);
            return -1;
        }
    }

    // Connect successful.

    streams_own_input_stream_t input = io.f0;
    streams_borrow_input_stream_t input_borrow = streams_borrow_input_stream(input);
    poll_own_pollable_t input_pollable = streams_method_input_stream_subscribe(input_borrow);

    streams_own_output_stream_t output = io.f1;
    streams_borrow_output_stream_t output_borrow = streams_borrow_output_stream(output);
    poll_own_pollable_t output_pollable = streams_method_output_stream_subscribe(output_borrow);

    socket->state = (tcp_socket_state_t){ .tag = TCP_SOCKET_STATE_CONNECTED, .connected = {
        .input = input,
        .input_pollable = input_pollable,
        .output = output,
        .output_pollable = output_pollable,
    } };
    return 0;
}

int udp_connect(udp_socket_t* socket, network_ip_socket_address_t* address)
{
    // TODO wasi-sockets: implement
    errno = EOPNOTSUPP;
    return -1;
}

int connect(int fd, const struct sockaddr* address, socklen_t len)
{
    descriptor_table_entry_t* entry;
    if (!descriptor_table_get_ref(fd, &entry)) {
        errno = EBADF;
        return -1;
    }

    network_ip_socket_address_t ip_address;
    int parse_err;
    if (!__wasi_sockets_utils__parse_address(address, len, &ip_address, &parse_err)) {
        errno = parse_err;
        return -1;
    }

    switch (entry->tag)
    {
    case DESCRIPTOR_TABLE_ENTRY_TCP_SOCKET:
        return tcp_connect(&entry->tcp_socket, &ip_address);
    case DESCRIPTOR_TABLE_ENTRY_UDP_SOCKET:
        return udp_connect(&entry->udp_socket, &ip_address);
    default:
        errno = EOPNOTSUPP;
        return -1;
    }
}
