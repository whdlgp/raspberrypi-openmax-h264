#include "udp_receive.h"

void udp_receive(char* port, char* data, int* data_size, sockaddr_in* send_addr, socklen_t* send_addr_size)
{
    int rcv_sock;
    sockaddr_in rcv_addr;

    rcv_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if(rcv_sock < 0)
    {
        printf("can't get socket\n");
        exit(0);
    }

    memset(&rcv_addr, 0, sizeof(rcv_addr));
    rcv_addr.sin_family = AF_INET;
    rcv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    rcv_addr.sin_port = htons(atoi(port));

    int bind_ok;

    bind_ok = bind(rcv_sock, (const sockaddr*)&rcv_addr, sizeof(rcv_addr));
    if(bind_ok < 0)
    {
        printf("can't bind socket\n");
        exit(0);
    }

    *data_size = recvfrom(rcv_sock, data, UDP_BUF_SIZE, 0, (sockaddr*)send_addr, send_addr_size);

    close(rcv_sock);
}
