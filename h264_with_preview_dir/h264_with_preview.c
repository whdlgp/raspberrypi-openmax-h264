/*
 For the sake of simplicity, this example exits on error.

 Very quick OpenMAX IL explanation:

 - There are components. Each component performs an action. For example, the
 OMX.broadcom.camera module captures images and videos and the
 OMX.broadcom.image_encoder module encodes raw data from an image into multiple
 formats. Each component has input and output ports and receives and sends
 buffers with data. The main goal is to join these components to form a
 pipeline and do complex tasks.
 - There are two ways to connect components: with tunnels or manually. The
 non-tunneled ports need to manually allocate the buffers with
 OMX_AllocateBuffer() and free them with OMX_FreeBuffer().
 - The components have states.
 - There are at least two threads: the thread that uses the application (CPU) and
 the thread that is used internally by OMX to execute the components (GPU).
 - There are two types of functions: blocking and non-blocking. The blocking
 functions are synchronous and the non-blocking are asynchronous. Being
 asynchronous means that the function returns immediately but the result is
 returned in a later time, so you need to wait until you receive an event. This
 example uses two non-blocking functions: OMX_SendCommand and
 OMX_FillThisBuffer.

 Note: The camera component has two video ports: "preview" and "video". The
 "preview" port must be enabled even if you're not using it (tunnel it to the
 null_sink component) because it is used to run AGC (automatic gain control) and
 AWB (auto white balance) algorithms.
 */

#include "omx_part.h"

#define FILENAME "video.h264"
#define PREVIEW_NAME "preview.h264"

//Signal flags for user interrupt
//e.g : ctrl + c
int signal_flag;
static void sig_flag_set(int signal)
{
    signal_flag = 1;
}
int signal_flag_check(void)
{
    return signal_flag;
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
        if(signal_flag_check())
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

//Thread for preview, write resized video to preview.h264
void* preview_thread(void* arg)
{
    component_buffer_t* cmp = (component_buffer_t*)arg;

    OMX_ERRORTYPE error;

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
        if(signal_flag_check())
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

int main()
{

    //Open the file
    //main file 
    int fd = open(FILENAME, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
    if (fd == -1)
    {
        fprintf(stderr, "error: open main video file\n");
        exit(1);
    }
    //preview file
    int fd_prv = open(PREVIEW_NAME, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
    if (fd_prv == -1)
    {
        fprintf(stderr, "error: open preview file\n");
        exit(1);
    }

    //initialize OpenMAX component's
    rpiomx_open();

    //signal interrupt
    signal(SIGINT,  sig_flag_set);
    signal(SIGTERM, sig_flag_set);
    signal(SIGQUIT, sig_flag_set);

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
    preview_cmp.fd = &fd_prv;
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

    //Close OpenMAX components
    rpiomx_close();
    
    //Close the file
    if (close(fd))
    {
        fprintf(stderr, "error: close\n");
        exit(1);
    }
    if (close(fd_prv))
    {
        fprintf(stderr, "error: close\n");
        exit(1);
    }
    printf("ok\n");

    return 0;
}
