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

#include "components/component_common.h" 

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
typedef struct component_buffer_t{
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
    OMX_ERRORTYPE error;
    OMX_BUFFERHEADERTYPE* encoder_output_buffer;
    OMX_BUFFERHEADERTYPE* preview_output_buffer;
    component_t camera;
    component_t encoder;
    component_t encoder_prv;
    component_t resize;
    component_t splitter;
    component_t null_sink;
    camera.name      = "OMX.broadcom.camera";
    encoder.name     = "OMX.broadcom.video_encode";
    encoder_prv.name = "OMX.broadcom.video_encode";
    resize.name      = "OMX.broadcom.resize";
    splitter.name    = "OMX.broadcom.video_splitter";
    null_sink.name   = "OMX.broadcom.null_sink";

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

    //Initialize Broadcom's VideoCore APIs
    bcm_host_init();

    //Initialize OpenMAX IL
    if ((error = OMX_Init()))
    {
        fprintf(stderr, "error: OMX_Init: %s\n", dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    printf("--------Initialize components-------------------\n");
    //Initialize components
    init_component(&camera);
    init_component(&encoder);
    init_component(&encoder_prv);
    init_component(&resize);
    init_component(&splitter);
    init_component(&null_sink);

    printf("--------Load camera driver----------------------\n");
    //Initialize camera drivers
    load_camera_drivers(&camera);

    printf("------Set components port definition and setting\n");
    //Configure camera port definition
    set_camera_port_definition(&camera);
    //Configure camera settings
    set_camera_settings(&camera);

    //Configure H264 port definition
    set_h264_port_definition(&encoder);
    //Configure H264
    set_h264_settings(&encoder);

    //Configure H264 preview port definition
    set_h264_preview_port_definition(&encoder_prv);
    //Configure H264 preview
    set_h264_preview_settings(&encoder_prv);

    set_resize_port_definition(&resize);

    printf("---------Set Tunnels----------------------------\n");
    //Setup tunnels: camera (video) -> video_splitter -> video_encode, camera (preview port) -> null_sink
    //and video_splitter -> resize -> video_encode(for preview)
    printf("configuring tunnels\n");
    if ((error = OMX_SetupTunnel(camera.handle, 71, splitter.handle, 250)))
    {
        fprintf(stderr, "error: OMX_SetupTunnel: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }
    if ((error = OMX_SetupTunnel(splitter.handle, 251, encoder.handle, 200)))
    {
        fprintf(stderr, "error: OMX_SetupTunnel: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }
    if ((error = OMX_SetupTunnel(splitter.handle, 252, resize.handle, 60)))
    {
        fprintf(stderr, "error: OMX_SetupTunnel: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }
    if ((error = OMX_SetupTunnel(resize.handle, 61, encoder_prv.handle, 200)))
    {
        fprintf(stderr, "error: OMX_SetupTunnel: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }
    if ((error = OMX_SetupTunnel(camera.handle, 70, null_sink.handle, 240)))
    {
        fprintf(stderr, "error: OMX_SetupTunnel: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }
    
 
    printf("----------Change state to IDLE------------------\n");
    //Change state to IDLE
    change_state(&camera, OMX_StateIdle);
    //wait(&camera, EVENT_STATE_SET, 0);
    wait_state_change(&camera, OMX_StateIdle);

    change_state(&encoder, OMX_StateIdle);
    //wait(&encoder, EVENT_STATE_SET, 0);
    wait_state_change(&encoder, OMX_StateIdle);
    
    change_state(&encoder_prv, OMX_StateIdle);
    //wait(&encoder_prv, EVENT_STATE_SET, 0);
    wait_state_change(&encoder_prv, OMX_StateIdle);
    
    change_state(&resize, OMX_StateIdle);
    //wait(&resize, EVENT_STATE_SET, 0);
    wait_state_change(&resize, OMX_StateIdle);
    
    change_state(&splitter, OMX_StateIdle);
    //wait(&splitter, EVENT_STATE_SET, 0);
    wait_state_change(&splitter, OMX_StateIdle);
    
    change_state(&null_sink, OMX_StateIdle);
    //wait(&null_sink, EVENT_STATE_SET, 0);
    wait_state_change(&null_sink, OMX_StateIdle);


    printf("----------Enable the ports----------------------\n");
    //Enable the ports
    enable_port(&camera, 71);
    wait_enable_port(&camera, 71);
    enable_port(&camera, 70);
    wait_enable_port(&camera, 70);

    enable_port(&splitter, 250);
    wait_enable_port(&splitter, 250);
    enable_port(&splitter, 251);
    wait_enable_port(&splitter, 251);
    enable_port(&splitter, 252);
    wait_enable_port(&splitter, 252);
    
    enable_port(&resize, 60);
    wait_enable_port(&resize, 60);
    enable_port(&resize, 61);
    wait_enable_port(&resize, 61);

    enable_port(&encoder, 200);
    wait_enable_port(&encoder, 200);
    enable_encoder_output_port(&encoder, &encoder_output_buffer);
    
    enable_port(&encoder_prv, 200);
    wait_enable_port(&encoder_prv, 200);
    enable_encoder_output_port(&encoder_prv, &preview_output_buffer); 
    
    enable_port(&null_sink, 240);
    wait_enable_port(&null_sink, 240);


    printf("----------Change state to EXECUTING-------------\n");
    //Change state to EXECUTING
    change_state(&camera, OMX_StateExecuting);
    //wait(&camera, EVENT_STATE_SET, 0);
    wait_state_change(&camera, OMX_StateExecuting);

    change_state(&splitter, OMX_StateExecuting);
    //wait(&splitter, EVENT_STATE_SET, 0);
    wait_state_change(&splitter, OMX_StateExecuting);
    
    change_state(&resize, OMX_StateExecuting);
    //wait(&resize, EVENT_STATE_SET, 0);
    wait_state_change(&resize, OMX_StateExecuting);
    
    change_state(&encoder, OMX_StateExecuting);
    //wait(&encoder, EVENT_STATE_SET, 0);
    wait_state_change(&encoder, OMX_StateExecuting);
    wait(&encoder, EVENT_PORT_SETTINGS_CHANGED, 0);
    
    change_state(&encoder_prv, OMX_StateExecuting);
    //wait(&encoder_prv, EVENT_STATE_SET, 0);
    wait_state_change(&encoder_prv, OMX_StateExecuting);
    wait(&encoder_prv, EVENT_PORT_SETTINGS_CHANGED, 0);
    
    change_state(&null_sink, OMX_StateExecuting);
    //wait(&null_sink, EVENT_STATE_SET, 0);
    wait_state_change(&null_sink, OMX_StateExecuting);

    
    printf("---------Set camera capture Enable--------------\n");
    //Enable camera capture port. This basically says that the port 71 will be
    //used to get data from the camera. If you're capturing a still, the port 72
    //must be used
    printf("enabling %s capture port\n", camera.name);
    OMX_CONFIG_PORTBOOLEANTYPE capture_st;
    OMX_INIT_STRUCTURE(capture_st);
    capture_st.nPortIndex = 71;
    capture_st.bEnabled = OMX_TRUE;
    if ((error = OMX_SetConfig(camera.handle, OMX_IndexConfigPortCapturing,
            &capture_st)))
    {
        fprintf(stderr, "error: OMX_SetConfig: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    //signal interrupt
    signal(SIGINT,  sig_flag_set);
    signal(SIGTERM, sig_flag_set);
    signal(SIGQUIT, sig_flag_set);

    printf("---------Start Capture and Encode---------------\n");
    //Create Encoding thread
    int encode_status;
    component_buffer_t encode_cmp;
    encode_cmp.fd = &fd;
    encode_cmp.component = &encoder;
    encode_cmp.buffer = encoder_output_buffer;
    
    VCOS_THREAD_T encode_th;
    vcos_thread_create(&encode_th, "encode_thread", NULL, encoding_thread, (void*)(&encode_cmp));
    printf("encoding Thread start\n");

    //Create preview Thread
    int preview_status;
    component_buffer_t preview_cmp;
    preview_cmp.fd = &fd_prv;
    preview_cmp.component = &encoder_prv;
    preview_cmp.buffer = preview_output_buffer;

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

    //Restore signal handlers 
    signal(SIGINT,  SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);

    //Disable camera capture port
    printf("disabling %s capture port\n", camera.name);
    capture_st.bEnabled = OMX_FALSE;
    if ((error = OMX_SetConfig(camera.handle, OMX_IndexConfigPortCapturing,
            &capture_st)))
    {
        fprintf(stderr, "error: OMX_SetConfig: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    printf("-----------Disable tunnel ports-----------------\n");
    //Disable the tunnel ports
    disable_port(&camera, 71);
    //wait(&camera, EVENT_PORT_ENABLE, 0);
    wait_disable_port(&camera, 71); 

    disable_port(&camera, 70);
    //wait(&camera, EVENT_PORT_ENABLE, 0);
    wait_disable_port(&camera, 70); 
    
    disable_port(&null_sink, 240);
    //wait(&null_sink, EVENT_PORT_ENABLE, 0);
    wait_disable_port(&null_sink, 240); 
    
    disable_port(&splitter, 250);
    //wait(&splitter, EVENT_PORT_ENABLE, 0);
    wait_disable_port(&splitter, 250); 
    
    disable_port(&splitter, 251);
    //wait(&splitter, EVENT_PORT_ENABLE, 0);
    wait_disable_port(&splitter, 251); 
    
    disable_port(&splitter, 252);
    //wait(&splitter, EVENT_PORT_ENABLE, 0);
    wait_disable_port(&splitter, 252); 
    
    disable_port(&resize, 60);
    //wait(&splitter, EVENT_PORT_ENABLE, 0);
    wait_disable_port(&resize, 60); 
    
    disable_port(&resize, 61);
    //wait(&splitter, EVENT_PORT_ENABLE, 0);
    wait_disable_port(&resize, 61); 
    
    disable_port(&encoder, 200);
    wait_disable_port(&encoder, 200); 
    //wait(&encoder, EVENT_PORT_ENABLE, 0);
    disable_encoder_output_port(&encoder, encoder_output_buffer);
    
    disable_port(&encoder_prv, 200);
    //wait(&encoder_prv, EVENT_PORT_ENABLE, 0);
    wait_disable_port(&encoder_prv, 200); 
    disable_encoder_output_port(&encoder_prv, preview_output_buffer);
    
    
    printf("---------Change state to IDLE-------------------\n");
    //Change state to IDLE
    change_state(&camera, OMX_StateIdle);
    //wait(&camera, EVENT_STATE_SET, 0);
    wait_state_change(&camera, OMX_StateIdle);

    change_state(&encoder, OMX_StateIdle);
    //wait(&encoder, EVENT_STATE_SET, 0);
    wait_state_change(&encoder, OMX_StateIdle);
    
    change_state(&encoder_prv, OMX_StateIdle);
    //wait(&encoder_prv, EVENT_STATE_SET, 0);
    wait_state_change(&encoder_prv, OMX_StateIdle);
    
    change_state(&resize, OMX_StateIdle);
    //wait(&resize, EVENT_STATE_SET, 0);
    wait_state_change(&resize, OMX_StateIdle);
    
    change_state(&splitter, OMX_StateIdle);
    //wait(&splitter, EVENT_STATE_SET, 0);
    wait_state_change(&splitter, OMX_StateIdle);
    
    change_state(&null_sink, OMX_StateIdle);
    //wait(&null_sink, EVENT_STATE_SET, 0);
    wait_state_change(&null_sink, OMX_StateIdle);


    
    printf("---------Change state to LOADED-----------------\n");
    //Change state to LOADED
    change_state(&camera, OMX_StateLoaded);
    //wait(&camera, EVENT_STATE_SET, 0);
    wait_state_change(&camera, OMX_StateLoaded);

    change_state(&encoder_prv, OMX_StateLoaded);
    //wait(&encoder_prv, EVENT_STATE_SET, 0);
    wait_state_change(&encoder_prv, OMX_StateLoaded);
    
    change_state(&encoder, OMX_StateLoaded);
    //wait(&encoder, EVENT_STATE_SET, 0);
    wait_state_change(&encoder_prv, OMX_StateLoaded);
    
    change_state(&resize, OMX_StateLoaded);
    //wait(&resize, EVENT_STATE_SET, 0);
    wait_state_change(&resize, OMX_StateLoaded);
    
    change_state(&splitter, OMX_StateLoaded);
    //wait(&splitter, EVENT_STATE_SET, 0);
    wait_state_change(&splitter, OMX_StateLoaded);
    
    change_state(&null_sink, OMX_StateLoaded);
    //wait(&null_sink, EVENT_STATE_SET, 0);
    wait_state_change(&null_sink, OMX_StateLoaded);

     
    printf("--------Deinitialize components-----------------\n");
    //Deinitialize components
    deinit_component(&camera);
    deinit_component(&encoder);
    deinit_component(&encoder_prv);
    deinit_component(&resize);
    deinit_component(&splitter);
    deinit_component(&null_sink);

    //Deinitialize OpenMAX IL
    if ((error = OMX_Deinit()))
    {
        fprintf(stderr, "error: OMX_Deinit: %s\n", dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    //Deinitialize Broadcom's VideoCore APIs
    bcm_host_deinit();

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
