#ifndef RESIZE_H
#define RESIZE_H

#include "component_common.h"

void set_resize_port_definition(component_t* resize);

void enable_resize_output_port(component_t* resize,
        OMX_BUFFERHEADERTYPE** resize_output_buffer);
void disable_resize_output_port(component_t* resize,
        OMX_BUFFERHEADERTYPE* resize_output_buffer);

#endif
