#ifndef __wasi_sockets_utils_h
#define __wasi_sockets_utils_h

#include <netinet/in.h>

#include <descriptor_table.h>

reactor_borrow_network_t __wasi_sockets_utils__borrow_network();
int __wasi_sockets_utils__map_error(wasi_sockets_0_2_0_rc_2023_10_18_network_error_code_t wasi_error);
bool __wasi_sockets_utils__parse_address(const struct sockaddr* address, socklen_t len, wasi_sockets_0_2_0_rc_2023_10_18_network_ip_socket_address_t* output, int* error);
bool __wasi_sockets_utils__format_address(const wasi_sockets_0_2_0_rc_2023_10_18_network_ip_socket_address_t* address, struct sockaddr* output_addr, socklen_t* output_addrlen, int* error);
int __wasi_sockets_utils__posix_family(wasi_sockets_0_2_0_rc_2023_10_18_network_ip_address_family_t wasi_family);

#endif