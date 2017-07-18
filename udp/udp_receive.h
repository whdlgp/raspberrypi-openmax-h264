#ifndef _UDP_RECEIVE_H_
#define _UDP_RECEIVE_H_

#include "udp_common.h"

void udp_receive(char* port, char* data, int* data_size, sockaddr_in* send_addr, socklen_t* send_addr_size);

#endif
