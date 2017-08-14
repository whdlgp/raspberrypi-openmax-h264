#include "resize.h"

/*--------------------------------------------------------------------- 
   set the output port (61) with PREVIEW_WIDTH, PREVIEW_HEIGHT 
                                 YUV420PackedPlannar (?) 
   NB:nothing on the input port
----------------------------------------------------------------------*/
void set_resize_port_definition(component_t* resize)
{
    //Configure resize component port definition
    printf("configuring %s for preview port definition\n", resize->name);
    
    OMX_ERRORTYPE error;
    
    OMX_PARAM_PORTDEFINITIONTYPE port_st;
    OMX_INIT_STRUCTURE(port_st);
    port_st.nPortIndex = 61;
    if ((error = OMX_GetParameter(resize->handle, OMX_IndexParamPortDefinition,
            &port_st)))
    {
        fprintf(stderr, "error: OMX_GetParameter: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }
    port_st.format.image.nFrameWidth = PREVIEW_WIDTH;
    port_st.format.image.nFrameHeight = PREVIEW_HEIGHT;
    port_st.format.image.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
    port_st.format.image.nSliceHeight = 0;
    port_st.format.image.nStride = 0;

    if ((error = OMX_SetParameter(resize->handle, OMX_IndexParamPortDefinition,
            &port_st)))
    {
        fprintf(stderr, "error: OMX_SetParameter: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

}

/*-------------------------------------------------------------------
   non-tunneling output setting
   enable the resize components and allocate output buffer
---------------------------------------------------------------------*/
void enable_resize_output_port(component_t* cmp,
        OMX_BUFFERHEADERTYPE** output_buffer)
{
    //The port is not enabled until the buffer is allocated
    OMX_ERRORTYPE error;

    enable_port(cmp, 61);

    OMX_PARAM_PORTDEFINITIONTYPE port_st;
    OMX_INIT_STRUCTURE(port_st);
    port_st.nPortIndex = 61;
    if ((error = OMX_GetParameter(cmp->handle, OMX_IndexParamPortDefinition,
            &port_st)))
    {
        fprintf(stderr, "error: OMX_GetParameter: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }
    /* Heejune tested the nBuffersize is one-frame of YUV 420, but not sure it is YUVPlannar*/ 
    printf("allocating %s output buffer, size = %d\n", cmp->name, port_st.nBufferSize);
    if ((error = OMX_AllocateBuffer(cmp->handle, output_buffer, 61,
            0, port_st.nBufferSize)))
    {
        fprintf(stderr, "error: OMX_AllocateBuffer: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    wait_enable_port(cmp, 61); // @TODO for consistency move this outside
}


/*------------------------------------------------------------------------ 
   non-tunneling output setting
   disable and deallocate buffer 
-------------------------------------------------------------------------*/
void disable_resize_output_port(component_t* cmp,
        OMX_BUFFERHEADERTYPE* output_buffer)
{
    //The port is not disabled until the buffer is released
    OMX_ERRORTYPE error;

    disable_port(cmp, 61);

    //Free output buffer
    printf("releasing %s output buffer\n", cmp->name);
    if ((error = OMX_FreeBuffer(cmp->handle, 61, output_buffer)))
    {
        fprintf(stderr, "error: OMX_FreeBuffer: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    wait_disable_port(cmp, 61); // @TODO for consistency move this outside
}



