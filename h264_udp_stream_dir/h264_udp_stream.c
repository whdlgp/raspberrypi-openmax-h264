//for OMX components
#include "omx_part.h"

//for UDP and TCP
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h> /* close() */
#include <string.h> /* memset() */
#include <pthread.h>

//Save High resolution video to file
#define FILENAME "video.h264" 

//UDP and TCP definition
#define LOCAL_SERVER_PORT  1500
#define STREAM_CLIENT_PORT 1501   

#define MAX_UDP_SIZE 512       //
#define MAX_PAYLOAD_SIZE 508   // 4 bytes header 

//Signal flags for user interrupt and for save end
//e.g : ctrl + c, client send quit message
int signal_flag = 0;
int quit_flag = 0;
static void sig_flag_set(int signal)
{
    signal_flag = 1;
}
int signal_flag_check(void)
{
    return signal_flag;
}

//global variables for UDP
//udpsock be used for sending low resolution video 
//also used for checking "Keep alive", periodically received message
static int udpsock = -1;  // init with invalid
static struct sockaddr_in cliAddr; // make a copy for modified
static int nframe = 0;

static void send_data(unsigned char *pBuf, int len)
{
    int n;
    int flags = 0;
    int cliLen = sizeof(struct sockaddr_in);
    //int _len = len;
    unsigned char nalType = pBuf[7] & 0x1F;

    nframe++;
    int nfragment = 0;

    while (len > 0)
    {
        /* 1. add header */
        if (nfragment == 0)
        { // use the space of '0001'
            ;
        }
        else
        { // use the previous fragement area for header
            pBuf -= 4, len += 4;
        }
        *((unsigned short *) pBuf) = nframe;
        pBuf[2] = nfragment;
        pBuf[3] = nalType;

        /* 2. send one fragment */
        n = (len > MAX_UDP_SIZE) ? MAX_UDP_SIZE : len;
        n = sendto(udpsock, pBuf, n, flags, (struct sockaddr *) &cliAddr, cliLen);
        if (n <= 0)
        {
            fprintf(stderr, "cannot send all data (%d) to client\n", n);
            break;
        }
        /*
        fprintf(stdout, "fn=%d(%d),fragment=%d(%d), nal=%d\n", nframe, _len,
                nfragment, n, nalType); // to check
        */
        len -= n;
        pBuf += n;  // @TODO header size ?
        nfragment++;
    }
}

static int open_listenfd(short portNum)
{
    int sock, rc;
    struct sockaddr_in servAddr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        fprintf(stderr, "Error: cannot open socket (%d) \n", portNum);
        return -1;
    }

    /* bind local server port */
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(portNum);
    rc = bind(sock, (struct sockaddr *) &servAddr, sizeof(servAddr));
    if (rc < 0)
    {
        fprintf(stderr, "Error: cannot bind port number %d \n", portNum);
        return -1;
    }

    return sock;
}

// definition for check period of "Keep alive" message from client
#define KEEP_ALIVE_INTERVAL    2500  //in ms
static struct timespec latestKeepAlive;

//for check "Keep alive", update the time 
static void updateKeepAlive()
{
    clock_gettime(CLOCK_MONOTONIC, &latestKeepAlive);
}

//for check "Keep alive", get the difference between the previous time and the present time.
static long elapsedtimeKeepAlive()
{
    long elapsed_in_ms;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    elapsed_in_ms = (now.tv_sec * 1000 + now.tv_nsec / 1.0e6)
            - (latestKeepAlive.tv_sec * 1000 + latestKeepAlive.tv_nsec / 1.0e6);

    return elapsed_in_ms;
}

static int waitEvent(int fd1, int fd2, int time)
{
    int nselected;
    struct timeval timeout;
    fd_set readfds; //, writefds;
    int max_fd;

    timeout.tv_sec = 0;
    timeout.tv_usec = time * 1000L; // ms to us

    FD_ZERO(&readfds);
    if (fd1 < 0)
        return -1;
    FD_SET(fd1, &readfds);
    if (fd2 >= 0)
        FD_SET(fd2, &readfds);
    max_fd = (fd1 > fd2) ? fd1 : fd2;

    nselected = select(max_fd + 1, &readfds, (fd_set *) NULL, (fd_set *) NULL,
            &timeout);

    if (nselected == 0)
        return 0;
    else if (nselected > 0)
    {
        if (FD_ISSET(fd1, &readfds))
            return 1;
        else if (FD_ISSET(fd2, &readfds))
            return 2;
        else
            return -1;
    }
    else
        return -1;
}

//Informations to pass to the thread as an argument
typedef struct component_buffer_t {
    int* fd;
    component_t* component;
    OMX_BUFFERHEADERTYPE * buffer;
} component_buffer_t;

//Thread for encode and write to video.h264
void* encoding_thread(void* arg)
{
    component_buffer_t* cmp = (component_buffer_t*)arg;

    OMX_ERRORTYPE error;

    printf("Encoding thread will write to video.h264 file\n");
    while (1)
    {
        //Get the buffer data
        if ((error = OMX_FillThisBuffer(cmp->component->handle, cmp->buffer)))
        {
            fprintf(stderr, "error: OMX_FillThisBuffer: %s\n",
                    dump_OMX_ERRORTYPE(error));
            vcos_thread_exit((void*)1);
        }

        //Wait until it's filled
        wait(cmp->component, EVENT_FILL_BUFFER_DONE, 0);

        //check if user press "ctrl c" or other interrupt occured
        if(signal_flag_check() || quit_flag)
        {
            printf("encoding : Termination by user detected\n");
            //signal interrupt detected
            //wait the key frame for check the boundry of video and exit

            //wait until find I frame(syncframe)
            if(cmp->buffer->nFlags & OMX_BUFFERFLAG_SYNCFRAME)
            {
                printf("encoding : SyncFrame found, It will be finished in a moment.\n");
                break;
            }
        }

        //Append the buffer into the file
        if (pwrite(*(cmp->fd), cmp->buffer->pBuffer,
                cmp->buffer->nFilledLen,
                cmp->buffer->nOffset) == -1)
        {
            fprintf(stderr, "error: pwrite\n");
            vcos_thread_exit((void*)1);
        }
    }

    vcos_thread_exit((void*)0);

    return NULL;
}

enum NAL_TYPE
{
    POB = 1,
    PAT = 4,
    IDR = 5,
    SEI = 6,
    SPS = 7,
    PPS = 8,
};

int get_NAL_type(unsigned char* frame, int len)
{
    return frame[4] & 0x1f;
}

//Thread for preview, write resized video to preview.h264
void* preview_thread(void* arg)
{
    component_buffer_t* cmp = (component_buffer_t*)arg;

    OMX_ERRORTYPE error;

    //declare time stamp variable
    DECLARE_TIME(sending_period)

    printf("preview thread will write to preview.h264 file\n");
    while (1)
    {
        //Get the buffer data
        if ((error = OMX_FillThisBuffer(cmp->component->handle, cmp->buffer)))
        {
            fprintf(stderr, "error: OMX_FillThisBuffer: %s\n",
                    dump_OMX_ERRORTYPE(error));
            vcos_thread_exit((void*)1);
        }

        //Wait until it's filled
        wait(cmp->component, EVENT_FILL_BUFFER_DONE, 0);

        //check if user press "ctrl c" or other interrupt occured
        if(signal_flag_check() || quit_flag)
        {
            printf("preview : Termination by user detected\n");
            //signal interrupt detected
            //wait the key frame for check the boundry of video and exit

            //wait until find I frame(syncframe)
            if(cmp->buffer->nFlags & OMX_BUFFERFLAG_SYNCFRAME)
            {
                printf("preview : SyncFrame found, It will be finished in a moment.\n");
                break;
            }
        }
        
        //print type of NAL header
        //printNALFrame(cmp->buffer->pBuffer, cmp->buffer->nFilledLen);

        ////Write buffer to UDP
        //only send IDR slice or SPS/PPS
        int nal_type = get_NAL_type(cmp->buffer->pBuffer, cmp->buffer->nFilledLen);
        if((nal_type == IDR) 
            || (nal_type == SPS)
            || (nal_type == PPS))
        {
            STOP_TIME(sending_period)
            PRINT_EXECUTION_TIME(sending_period)

            send_data(cmp->buffer->pBuffer, cmp->buffer->nFilledLen);
        
            START_TIME(sending_period)
        }
    }

    vcos_thread_exit((void*)0);

    return NULL;
}

static void *stream_loop(void *arg)
{
    // socket related
    int rc;
    struct sockaddr_in servAddr;
    short localport = LOCAL_SERVER_PORT + rand() % 1000;
    cliAddr = *(struct sockaddr_in *) arg; // make a copy for modified
    
    int fd = open(FILENAME, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
    if (fd == -1)
    {
        fprintf(stderr, "error: open main video file\n");
        exit(1);
    }

    //frame count initialise
    nframe = 0;

    // 1.  create omx grpah  
    rpiomx_open();

    //signal interrupt
    signal(SIGINT,  sig_flag_set);
    signal(SIGTERM, sig_flag_set);
    signal(SIGQUIT, sig_flag_set);

    /* 1. socket creation */
    udpsock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpsock < 0)
    {
        fprintf(stderr, "Error:cannot open udp socket\n");
        pthread_exit((void *) -1);
    }

    /* 2. bind local server port */
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(STREAM_CLIENT_PORT - 1); // can use any not conflicting
    rc = bind(udpsock, (struct sockaddr *) &servAddr, sizeof(servAddr));
    if (rc < 0)
    {
        fprintf(stderr, "Error: cannot bind port number %d\n", localport);
        pthread_exit((void *) -1);
    }

    /* 3. prepare destination address */
    //cliAddr.sin_family = AF_INET;
    //cliAddr.sin_addr.s_addr = htonl(); // same destination as contoller 
    cliAddr.sin_port = htons(1501);      // different port 

    /* 4. infinite loop */
    printf("---------Start Capture and Encode---------------\n");
    //Create Encoding thread
    int encode_status;
    component_buffer_t encode_cmp;
    encode_cmp.fd = &fd;
    encode_cmp.component = cmp_buf.encoder;
    encode_cmp.buffer = cmp_buf.encoder_output_buffer;
    
    VCOS_THREAD_T encode_th;
    vcos_thread_create(&encode_th, "encode_thread", NULL, encoding_thread, (void*)(&encode_cmp));
    printf("encoding Thread start\n");

    //Create preview Thread
    int preview_status;
    component_buffer_t preview_cmp;
    preview_cmp.component = cmp_buf.encoder_prv;
    preview_cmp.buffer = cmp_buf.preview_output_buffer;

    VCOS_THREAD_T preview_th;
    vcos_thread_create(&preview_th, "preview_thread", NULL, preview_thread, (void*)(&preview_cmp));
    printf("preview Thread start\n");

    //wait join of threads
    printf("Wait encoding thread join\n");
    vcos_thread_join(&encode_th, (void*)&encode_status);
    if(encode_status != 0)
        fprintf(stderr, "unexpected exit occurred inside the encoding thread\n");
    else
        printf("encoding thread exit successfully\n");
    
    printf("Wait preview thread join\n");
    vcos_thread_join(&preview_th, (void*)&preview_status);
    if(preview_status != 0)
        fprintf(stderr, "unexpected exit occurred inside the encoding thread\n");
    else
        printf("encoding thread exit successfully\n");
    
    
    printf("------------------------------------------------\n");

    // Restore signal handlers
    signal(SIGINT,  SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);

    // 3. destroy the context
    rpiomx_close();

    close(fd);
    close(udpsock);
    udpsock = -1;  // mark it invalid
    pthread_exit((void *) 0); // user-requested-stop
}

static int stream_control(int sock, struct sockaddr_in *pCliAddr)
{
    size_t n;
    int r;
    pthread_t tid;
    int retval;
    unsigned char rxbuf[128]; /* one byte only used */
    unsigned char txbuf[128]; /* one byte only used */
    int flags = 0;
    int event = 0;

    while (1)
    {
        //select TCP socket or UDP socket. 
        //it will return available socket
        event = waitEvent(sock, udpsock, 1000);

        printf("==> event: %d\n", event);
        if (event == 0)
        {
            if (udpsock != -1)
            {
                if (elapsedtimeKeepAlive() > 2 * KEEP_ALIVE_INTERVAL)
                {
                    fprintf(stderr, "Time-OUTED\n");
                }
            }
            continue; 
            //@TODO Is this the correct place to use "continue"?
            //@TODO Is the above condition a problem?
        }
        else if (event == 2)
        {
            updateKeepAlive();
            n = 128;
            n = recvfrom(udpsock, rxbuf, n, 0, NULL, 0); // don't care of address
            if (n < 0)
            {
                fprintf(stderr, " Ooops, Error in reading udp socket...\n");
            }
            else
            {
                rxbuf[n] = 0;
                fprintf(stdout, "===>KEEP-ALIVE: %s (%d)\n", rxbuf, n);
            }
            continue;
        }
        else if (event != 1)
        {
            break;
        } // if event == 1

        // 1. get commands from client
        n = recv(sock, rxbuf, 128, flags);

        // 2. prorocol error check
        if (n <= 0)
        {
            fprintf(stderr, "read error: connection closed\n");
            quit_flag = 1;
            r = pthread_join(tid, (void **) &retval);
            quit_flag = 0;
            return -1;  // abnormal finish
        }
        else if (n > 1)
        {
            fprintf(stderr, "protocol error: too big msg\n");
            txbuf[0] = 'n'; // nack
            write(sock, txbuf, 1);
            continue;
        }
        // 3. protocol handle
        else
        {
            switch (rxbuf[0])
            {
            case 's':
                updateKeepAlive();

                r = pthread_create(&tid, NULL, stream_loop, (void *) pCliAddr);
                if (r != 0)
                {
                    fprintf(stderr, "ERROR:pthread_create\n");
                    txbuf[0] = 'n'; // ack
                    write(sock, txbuf, 1);
                }
                else
                {
                    txbuf[0] = 'a'; // ack
                    write(sock, txbuf, 1);
                }

                break;
            case 'c': // finish streaming
                quit_flag = 1;
                r = pthread_join(tid, (void **) &retval); // @TODO: check it run successfully
                quit_flag = 0;
                if (r != 0)
                {
                    fprintf(stderr, "ERROR:pthread_join\n");
                    quit_flag = 0;
                    txbuf[0] = 'n'; // ack
                    write(sock, txbuf, 1);

                }
                else
                {
                    txbuf[0] = 'a'; // ack
                    write(sock, txbuf, 1);
                    return 0; // normal  finish
                }

            default:
                txbuf[0] = 'n'; // nack
                write(sock, txbuf, 1);
                break;
            }
        }
    }

    return -1;
}

int main(int argc, char **argv)
{
    int listenfd, connfd, port;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    //struct hostent *hp;
    //char *haddrp;
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    port = atoi(argv[1]);

    printf("get user input %d\n", port);

    listenfd = open_listenfd(port);
    listen(listenfd, 1);

    printf("now listen something\n");

    clientlen = sizeof(clientaddr);

    while (1) // to stop CTRL-C or kill me 
    {
        connfd = accept(listenfd, (struct sockaddr *) &clientaddr, &clientlen);
        
        printf("Accept client\n");

        /* determine the domain name and IP address of the client */
        //hp = gethostbyaddr(&clientaddr.sin_addr, sizeof(clientaddr.sin_addr), AF_INET);
        //haddrp = inet_ntoa(clientaddr.sin_addr);
        //fprintf(stderr, "CNTL> new client %s (%s) connected\n", hp->h_name, haddrp);
        
        printf("go to stream process\n");
        stream_control(connfd, &clientaddr);

        //fprintf(stderr, "CTNL> connection %s (%s) lost\n", hp->h_name, haddrp);

        close(connfd);
    }
    return 0;
}
