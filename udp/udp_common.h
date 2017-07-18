#ifndef _UDP_COMMON_H_
#define _UDP_COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <memory.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <stdarg.h>

#include <unistd.h>

#ifndef boolean
#define boolean
typedef enum { false, true } bool;
#endif

typedef struct sockaddr_in  sockaddr_in;
typedef struct sockaddr     sockaddr;

#define UDP_BUF_SIZE 61440

typedef struct Packet {
    char data[UDP_BUF_SIZE];
    int data_size;
    int data_seq;
    bool is_finish;
} Packet;

#endif
