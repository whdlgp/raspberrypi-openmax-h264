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
#include "ffh264enc.h"   // wrapper for libavcodec 

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

//flag that encoding thread find key frame and will exit
int encoding_thread_exit_flag;

//Thread for encode and write to video.h264
void* encoding_thread(void* arg)
{
    component_buffer_t* cmp = (component_buffer_t*)arg;

    OMX_ERRORTYPE error;

    //for calculate actual frame rate
    uint64_t pre_time = 0;
    uint64_t currunt_time = 0;
    uint64_t time_gap = 0;
    int frame_count = 0;
    float frame_rate = 0;

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
        
        //for calculate actual frame rate
        pre_time = currunt_time;
        currunt_time = GetTimeStamp();
        time_gap = currunt_time - pre_time;
        frame_rate = (double)1000000/(double)time_gap;
        frame_count++;
        printf("encoding_thread\nframecount : %d\nframerate : %f\n\n", frame_count, frame_rate);
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
                encoding_thread_exit_flag = 1;
                break;
            }
        }

        //Append the buffer into the file
        if (write(*(cmp->fd)
                    , cmp->buffer->pBuffer
                    , cmp->buffer->nFilledLen) == -1)
        {
            fprintf(stderr, "error: write\n");
            vcos_thread_exit((void*)1);
        }
    }

    vcos_thread_exit((void*)0);

    return NULL;
}

//Thread for preview, write resized video to preview.h264
// using ffmpeg sw codec 
// arg : YUV video source component (hopefully one frame) 
//         
void* preview_thread(void* arg)
{
    component_buffer_t* cmp = (component_buffer_t*)arg;

    OMX_ERRORTYPE error;

    //for calculate actual frame rate
    uint64_t pre_time = 0;
    uint64_t currunt_time = 0;
    uint64_t time_gap = 0;
    int frame_count = 0;
    float frame_rate = 0;

    printf("preview thread will write to preview.h264 file\n");

    // init software codec
    int width = PREVIEW_WIDTH, height= PREVIEW_HEIGHT, bitrate = PREVIEW_BITRATE, fps = PREVIEW_FRAMERATE; 
    ffh264_enc_open(width, height, bitrate, fps);

    // get SPS/PPS data directly.
    unsigned char extradata[100] = {0,};
    int extradata_size = 0;
    ffh264_get_global_header(&extradata_size, extradata);

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
            
            //If do not receive the resize buffer, will not see the high resolution encoder side buffer. 
            //Wait until the keyframe appears in the high resolution encoder.
            if(encoding_thread_exit_flag == 1)
            {
                break;
            }
        }


	// Encoding
        unsigned char *pBuffer;
        int n = ffh264_enc_encode(cmp->buffer->pBuffer, &pBuffer);
        if (n < 0)
        { // errror in encoding
            fprintf(stderr, "error: encoding\n");
            vcos_thread_exit((void*) 1);
        }
        else if (n > 0)
        {
            // write SPS/PPS data
            printf("first write\n");
            if (write(*(cmp->fd)
                        , extradata
                        , extradata_size) == -1)
            {
                fprintf(stderr, "error: write\n");
                vcos_thread_exit((void*) 1);
            }
            // write frame data
            if (write(*(cmp->fd)
                        , pBuffer
                        , n) == -1)
            {
                fprintf(stderr, "error: write\n");
                vcos_thread_exit((void*) 1);
            }
        }
        else if (n == 0) // encoding ok but no data to give
            continue;

        pre_time = currunt_time;
        currunt_time = GetTimeStamp();
        time_gap = currunt_time - pre_time;
        frame_rate = (double) 1000000 / (double) time_gap;
        frame_count++;
        printf("preview_thread\nframecount : %d\nframerate : %f\n\n", frame_count, frame_rate);
    
    } // while loop

    ffh264_enc_close();

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

    // 2. run encoders  
    printf("---------Start Capture and Encode---------------\n");
    // 2.1 Create Encoding thread
    int encode_status;
    component_buffer_t encode_cmp;
    encode_cmp.fd = &fd;
    encode_cmp.component = cmp_buf.encoder;
    encode_cmp.buffer = cmp_buf.encoder_output_buffer;
    
    VCOS_THREAD_T encode_th;
    vcos_thread_create(&encode_th, "encode_thread", NULL, encoding_thread, (void*)(&encode_cmp));
    printf("encoding Thread start\n");

    // 2.1 Create preview Thread
    int preview_status;
    component_buffer_t preview_cmp;
    preview_cmp.fd = &fd_prv;
    //preview_cmp.component = cmp_buf.encoder_prv;
    //preview_cmp.buffer = cmp_buf.preview_output_buffer;
    preview_cmp.component = cmp_buf.resize;
    preview_cmp.buffer = cmp_buf.resize_output_buffer;

    VCOS_THREAD_T preview_th;
    vcos_thread_create(&preview_th, "preview_thread", NULL, preview_thread, (void*)(&preview_cmp));
    printf("preview Thread start\n");

    // 3. wait join of threads
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
