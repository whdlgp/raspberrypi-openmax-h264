#ifndef OMX_PART_H
#define OMX_PART_H

#include "../components/component_common.h" 

#include "../components/camera.h"
#include "../components/resize.h"
#include "../components/H264_encoder.h"

void rpiomx_open();
void rpiomx_close();

typedef struct components_n_buffers
{
    component_t* camera;
    component_t* encoder;
    component_t* resize;
    component_t* splitter;
    component_t* null_sink;

    OMX_BUFFERHEADERTYPE* encoder_output_buffer;
    OMX_BUFFERHEADERTYPE* resize_output_buffer;
} components_n_buffers;

extern components_n_buffers cmp_buf;

#endif

