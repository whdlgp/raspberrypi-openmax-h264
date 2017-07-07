#ifndef OMX_CALLBACK_H
#define OMX_CALLBACK_H

#include "component_common.h"

//Events used with vcos_event_flags_get() and vcos_event_flags_set()
typedef enum
{
    EVENT_ERROR = 0x1,
    EVENT_PORT_ENABLE = 0x2,
    EVENT_PORT_DISABLE = 0x4,
    EVENT_STATE_SET = 0x8,
    EVENT_FLUSH = 0x10,
    EVENT_MARK_BUFFER = 0x20,
    EVENT_MARK = 0x40,
    EVENT_PORT_SETTINGS_CHANGED = 0x80,
    EVENT_PARAM_OR_CONFIG_CHANGED = 0x100,
    EVENT_BUFFER_FLAG = 0x200,
    EVENT_RESOURCES_ACQUIRED = 0x400,
    EVENT_DYNAMIC_RESOURCES_AVAILABLE = 0x800,
    EVENT_FILL_BUFFER_DONE = 0x1000,
    EVENT_EMPTY_BUFFER_DONE = 0x2000,
} component_event;


OMX_ERRORTYPE event_handler (
        OMX_IN OMX_HANDLETYPE comp,
        OMX_IN OMX_PTR app_data,
        OMX_IN OMX_EVENTTYPE event,
        OMX_IN OMX_U32 data1,
        OMX_IN OMX_U32 data2,
        OMX_IN OMX_PTR event_data);
OMX_ERRORTYPE fill_buffer_done (
        OMX_IN OMX_HANDLETYPE comp,
        OMX_IN OMX_PTR app_data,
        OMX_IN OMX_BUFFERHEADERTYPE* buffer);

#endif
