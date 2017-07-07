#ifndef H264_ENCODER_H
#define H264_ENCODER_H

#include "component_common.h"

void set_h264_port_definition(component_t* encoder);
void set_h264_settings(component_t* encoder);

void set_h264_preview_port_definition(component_t* encoder_prv);
void set_h264_preview_settings(component_t* encoder_prv);

void enable_encoder_output_port(component_t* encoder,
        OMX_BUFFERHEADERTYPE** encoder_output_buffer);
void disable_encoder_output_port(component_t* encoder,
        OMX_BUFFERHEADERTYPE* encoder_output_buffer);

#endif
