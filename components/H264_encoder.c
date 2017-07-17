#include "H264_encoder.h"

//H264 encoder port definition 
void set_h264_port_definition(component_t* encoder)
{
    //Configure encoder port definition
    printf("configuring %s port definition\n", encoder->name);
    
    OMX_ERRORTYPE error;

    OMX_PARAM_PORTDEFINITIONTYPE port_st;
    OMX_INIT_STRUCTURE(port_st);
    port_st.nPortIndex = 201;
    if ((error = OMX_GetParameter(encoder->handle, OMX_IndexParamPortDefinition,
            &port_st)))
    {
        fprintf(stderr, "error: OMX_GetParameter: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }
    port_st.format.video.nFrameWidth = CAM_WIDTH;
    port_st.format.video.nFrameHeight = CAM_HEIGHT;
    port_st.format.video.nStride = CAM_WIDTH;
    port_st.format.video.xFramerate = VIDEO_FRAMERATE << 16;
    //Despite being configured later, these two fields need to be set
    port_st.format.video.nBitrate = VIDEO_BITRATE;
    port_st.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    if ((error = OMX_SetParameter(encoder->handle, OMX_IndexParamPortDefinition,
            &port_st)))
    {
        fprintf(stderr, "error: OMX_SetParameter: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }
}

//H264 encoder component setup
void set_h264_settings(component_t* encoder)
{
    printf("configuring '%s' settings\n", encoder->name);

    OMX_ERRORTYPE error;

    //Bitrate
    OMX_VIDEO_PARAM_BITRATETYPE bitrate_st;
    OMX_INIT_STRUCTURE(bitrate_st);
    bitrate_st.eControlRate = OMX_Video_ControlRateVariable;
    bitrate_st.nTargetBitrate = VIDEO_BITRATE;
    bitrate_st.nPortIndex = 201;
    if ((error = OMX_SetParameter(encoder->handle,
            OMX_IndexParamVideoBitrate, &bitrate_st)))
    {
        fprintf(stderr, "error: OMX_SetParameter: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    //Codec
    OMX_VIDEO_PARAM_PORTFORMATTYPE format_st;
    OMX_INIT_STRUCTURE(format_st);
    format_st.nPortIndex = 201;
    //H.264/AVC
    format_st.eCompressionFormat = OMX_VIDEO_CodingAVC;
    if ((error = OMX_SetParameter(encoder->handle,
            OMX_IndexParamVideoPortFormat, &format_st)))
    {
        fprintf(stderr, "error: OMX_SetParameter: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    //Note: Motion vectors are not implemented in this program.
    //See for further details
    //https://github.com/gagle/raspberrypi-omxcam/blob/master/src/h264.c
    //https://github.com/gagle/raspberrypi-omxcam/blob/master/src/video.c
}

//H264 preview encoder port definition
void set_h264_preview_port_definition(component_t* encoder_prv)
{
    //Configure preview encoder port definition
    printf("configuring %s for preview port definition\n", encoder_prv->name);
    
    OMX_ERRORTYPE error;
    
    OMX_PARAM_PORTDEFINITIONTYPE port_st;
    OMX_INIT_STRUCTURE(port_st);
    port_st.nPortIndex = 201;
    if ((error = OMX_GetParameter(encoder_prv->handle, OMX_IndexParamPortDefinition,
            &port_st)))
    {
        fprintf(stderr, "error: OMX_GetParameter: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }
    port_st.format.video.nFrameWidth = PREVIEW_WIDTH;
    port_st.format.video.nFrameHeight = PREVIEW_HEIGHT;
    port_st.format.video.nStride = PREVIEW_WIDTH;
    port_st.format.video.xFramerate = PREVIEW_FRAMERATE << 16;
    //Despite being configured later, these two fields need to be set
    port_st.format.video.nBitrate = PREVIEW_BITRATE;
    port_st.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    
    if ((error = OMX_SetParameter(encoder_prv->handle, OMX_IndexParamPortDefinition,
            &port_st)))
    {
        fprintf(stderr, "error: OMX_SetParameter: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }
}

//H264 preview encoder component setup
void set_h264_preview_settings(component_t* encoder_prv)
{
    printf("configuring '%s' settings\n", encoder_prv->name);

    OMX_ERRORTYPE error;

    //Bitrate
    OMX_VIDEO_PARAM_BITRATETYPE bitrate_st;
    OMX_INIT_STRUCTURE(bitrate_st);
    bitrate_st.eControlRate = OMX_Video_ControlRateVariable;
    bitrate_st.nTargetBitrate = PREVIEW_BITRATE;
    bitrate_st.nPortIndex = 201;
    if ((error = OMX_SetParameter(encoder_prv->handle,
            OMX_IndexParamVideoBitrate, &bitrate_st)))
    {
        fprintf(stderr, "error: OMX_SetParameter: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    //Codec
    OMX_VIDEO_PARAM_PORTFORMATTYPE format_st;
    OMX_INIT_STRUCTURE(format_st);
    format_st.nPortIndex = 201;
    //H.264/AVC
    format_st.eCompressionFormat = OMX_VIDEO_CodingAVC;
    if ((error = OMX_SetParameter(encoder_prv->handle,
            OMX_IndexParamVideoPortFormat, &format_st)))
    {
        fprintf(stderr, "error: OMX_SetParameter: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }
    
    //SPS/PPS INLINE HEADER mode
    OMX_CONFIG_PORTBOOLEANTYPE sps_pps_st;
    OMX_INIT_STRUCTURE(sps_pps_st);
    sps_pps_st.nPortIndex = 201;
    sps_pps_st.bEnabled = PREVIEW_SPS_PPS_INLINE;
    if ((error = OMX_SetParameter(encoder_prv->handle,
           OMX_IndexParamBrcmVideoAVCInlineHeaderEnable, &sps_pps_st)))
    {
        fprintf(stderr, "error: OMX_SetParameter: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    //IDR period
    OMX_VIDEO_CONFIG_AVCINTRAPERIOD idr_st;
    OMX_INIT_STRUCTURE (idr_st);
    idr_st.nPortIndex = 201;
    if ((error = OMX_GetConfig (encoder_prv->handle,
           OMX_IndexConfigVideoAVCIntraPeriod, &idr_st)))
    {
        fprintf(stderr, "error: OMX_GetConfig: %s\n",
                 dump_OMX_ERRORTYPE (error));
        exit(1);
    }
    idr_st.nIDRPeriod = PREVIEW_IDR_PERIOD;
    if ((error = OMX_SetConfig (encoder_prv->handle,
           OMX_IndexConfigVideoAVCIntraPeriod, &idr_st)))
    {
        fprintf(stderr, "error: OMX_SetConfig: %s\n", dump_OMX_ERRORTYPE (error));
        exit(1);
    }

    //Note: Motion vectors are not implemented in this program.
    //See for further details
    //https://github.com/gagle/raspberrypi-omxcam/blob/master/src/h264.c
    //https://github.com/gagle/raspberrypi-omxcam/blob/master/src/video.c
}

//encoder output port have a buffer.
//add functions to allocate buffer of encoder
void enable_encoder_output_port(component_t* encoder,
        OMX_BUFFERHEADERTYPE** encoder_output_buffer)
{
    //The port is not enabled until the buffer is allocated
    OMX_ERRORTYPE error;

    enable_port(encoder, 201);

    OMX_PARAM_PORTDEFINITIONTYPE port_st;
    OMX_INIT_STRUCTURE(port_st);
    port_st.nPortIndex = 201;
    if ((error = OMX_GetParameter(encoder->handle, OMX_IndexParamPortDefinition,
            &port_st)))
    {
        fprintf(stderr, "error: OMX_GetParameter: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }
    printf("allocating %s output buffer\n", encoder->name);
    if ((error = OMX_AllocateBuffer(encoder->handle, encoder_output_buffer, 201,
            0, port_st.nBufferSize)))
    {
        fprintf(stderr, "error: OMX_AllocateBuffer: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    wait_enable_port(encoder, 201);
    //wait(encoder, EVENT_PORT_ENABLE, 0);
}

void disable_encoder_output_port(component_t* encoder,
        OMX_BUFFERHEADERTYPE* encoder_output_buffer)
{
    //The port is not disabled until the buffer is released
    OMX_ERRORTYPE error;

    disable_port(encoder, 201);

    //Free encoder output buffer
    printf("releasing %s output buffer\n", encoder->name);
    if ((error = OMX_FreeBuffer(encoder->handle, 201, encoder_output_buffer)))
    {
        fprintf(stderr, "error: OMX_FreeBuffer: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    wait_disable_port(encoder, 201);
    //wait(encoder, EVENT_PORT_DISABLE, 0);
}
