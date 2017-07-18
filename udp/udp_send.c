#include "udp_common.h"

void udp_send(char* ip, char* port, char* send_m, int send_m_size)
{
    int send_sock;
    sockaddr_in rcv_addr;

    send_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if(send_sock < 0)
    {
        printf("can't get socket\n");
        exit(0);
    }

    memset(&rcv_addr, 0, sizeof(rcv_addr));
    rcv_addr.sin_family = AF_INET;
    rcv_addr.sin_addr.s_addr = inet_addr(ip);
    rcv_addr.sin_port = htons(atoi(port));

    sendto(send_sock, send_m, send_m_size, 0, (const sockaddr*)&rcv_addr, sizeof(rcv_addr));
    
    close(send_sock);
}
