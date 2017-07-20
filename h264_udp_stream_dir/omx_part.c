#include "omx_part.h"

//Variable, handlers for OMX components
static OMX_ERRORTYPE error;
static OMX_BUFFERHEADERTYPE* encoder_output_buffer;
static OMX_BUFFERHEADERTYPE* preview_output_buffer;
static component_t camera;
static component_t encoder;
static component_t encoder_prv;
static component_t resize;
static component_t splitter;
static component_t null_sink;

//It looks good to use structures to easily share components and buffers with the outside world.
components_n_buffers cmp_buf;

void rpiomx_open()
{ 
    camera.name      = "OMX.broadcom.camera";
    encoder.name     = "OMX.broadcom.video_encode";
    encoder_prv.name = "OMX.broadcom.video_encode";
    resize.name      = "OMX.broadcom.resize";
    splitter.name    = "OMX.broadcom.video_splitter";
    null_sink.name   = "OMX.broadcom.null_sink";

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

    //make it easier to share handlers and buffers when more components are available
    cmp_buf.camera      = &camera;
    cmp_buf.encoder     = &encoder;
    cmp_buf.encoder_prv = &encoder_prv;
    cmp_buf.resize      = &resize;
    cmp_buf.splitter    = &splitter;
    cmp_buf.null_sink   = &null_sink;
    cmp_buf.encoder_output_buffer = encoder_output_buffer;
    cmp_buf.preview_output_buffer = preview_output_buffer;
}

void rpiomx_close()
{
    //Disable camera capture port
    printf("disabling %s capture port\n", camera.name);
    OMX_CONFIG_PORTBOOLEANTYPE capture_st;
    OMX_INIT_STRUCTURE(capture_st);
    capture_st.nPortIndex = 71;
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
}
