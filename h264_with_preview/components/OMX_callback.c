#include "OMX_callback.h"

//Function that is called when a component receives an event from a secondary
//thread
OMX_ERRORTYPE event_handler (
        OMX_IN OMX_HANDLETYPE comp,
        OMX_IN OMX_PTR app_data,
        OMX_IN OMX_EVENTTYPE event,
        OMX_IN OMX_U32 data1,
        OMX_IN OMX_U32 data2,
        OMX_IN OMX_PTR event_data)
{
    component_t* component = (component_t*)app_data;

    switch (event)
    {
        case OMX_EventCmdComplete:
        switch (data1)
        {
            case OMX_CommandStateSet:
            printf ("event: %s, OMX_CommandStateSet, state: %s\n",
                    component->name, dump_OMX_STATETYPE (data2));
            wake (component, EVENT_STATE_SET);
            break;
            case OMX_CommandPortDisable:
            printf ("event: %s, OMX_CommandPortDisable, port: %d\n",
                    component->name, data2);
            wake (component, EVENT_PORT_DISABLE);
            break;
            case OMX_CommandPortEnable:
            printf ("event: %s, OMX_CommandPortEnable, port: %d\n",
                    component->name, data2);
            wake (component, EVENT_PORT_ENABLE);
            break;
            case OMX_CommandFlush:
            printf ("event: %s, OMX_CommandFlush, port: %d\n",
                    component->name, data2);
            wake (component, EVENT_FLUSH);
            break;
            case OMX_CommandMarkBuffer:
            printf ("event: %s, OMX_CommandMarkBuffer, port: %d\n",
                    component->name, data2);
            wake (component, EVENT_MARK_BUFFER);
            break;
        }
        break;
        case OMX_EventError:
        printf ("event: %s, %s\n", component->name, dump_OMX_ERRORTYPE (data1));
        wake (component, EVENT_ERROR);
        break;
        case OMX_EventMark:
        printf ("event: %s, OMX_EventMark\n", component->name);
        wake (component, EVENT_MARK);
        break;
        case OMX_EventPortSettingsChanged:
        printf ("event: %s, OMX_EventPortSettingsChanged, port: %d\n",
                component->name, data1);
        wake (component, EVENT_PORT_SETTINGS_CHANGED);
        break;
        case OMX_EventParamOrConfigChanged:
        printf ("event: %s, OMX_EventParamOrConfigChanged, data1: %d, data2: "
                "%X\n", component->name, data1, data2);
        wake (component, EVENT_PARAM_OR_CONFIG_CHANGED);
        break;
        case OMX_EventBufferFlag:
        printf ("event: %s, OMX_EventBufferFlag, port: %d\n",
                component->name, data1);
        wake (component, EVENT_BUFFER_FLAG);
        break;
        case OMX_EventResourcesAcquired:
        printf ("event: %s, OMX_EventResourcesAcquired\n", component->name);
        wake (component, EVENT_RESOURCES_ACQUIRED);
        break;
        case OMX_EventDynamicResourcesAvailable:
        printf ("event: %s, OMX_EventDynamicResourcesAvailable\n",
                component->name);
        wake (component, EVENT_DYNAMIC_RESOURCES_AVAILABLE);
        break;
        default:
        //This should never execute, just ignore
        printf ("event: unknown (%X)\n", event);
        break;
    }

    return OMX_ErrorNone;
}

//Function that is called when a component fills a buffer with data
OMX_ERRORTYPE fill_buffer_done (
        OMX_IN OMX_HANDLETYPE comp,
        OMX_IN OMX_PTR app_data,
        OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
    component_t* component = (component_t*)app_data;

    printf ("event: %s, fill_buffer_done\n", component->name);
    wake (component, EVENT_FILL_BUFFER_DONE);

    return OMX_ErrorNone;
}


