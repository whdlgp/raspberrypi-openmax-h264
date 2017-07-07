#include "resize.h"

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

