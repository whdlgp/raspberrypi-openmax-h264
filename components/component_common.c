#include "component_common.h"

//event based blocking function
void wake(component_t* component, VCOS_UNSIGNED event)
{
    vcos_event_flags_set(&component->flags, event, VCOS_OR);
}

void wait(component_t* component, VCOS_UNSIGNED events,
        VCOS_UNSIGNED* retrieved_events)
{
    VCOS_UNSIGNED set;
    if (vcos_event_flags_get(&component->flags, events | EVENT_ERROR,
            VCOS_OR_CONSUME, VCOS_SUSPEND, &set))
    {
        fprintf(stderr, "error: vcos_event_flags_get\n");
        exit(1);
    }
    if (set == EVENT_ERROR)
    {
        exit(1);
    }
    if (retrieved_events)
    {
        *retrieved_events = set;
    }
}

//non-event based blocking function
void wait_enable_port(component_t* component, OMX_U32 port)
{
    printf("Check enable port directly.\n");
    
    OMX_ERRORTYPE r;
    OMX_PARAM_PORTDEFINITIONTYPE port_st;
    OMX_INIT_STRUCTURE(port_st);
    port_st.nPortIndex = port;
    while(port_st.bEnabled != OMX_TRUE)
    {
        if((r = OMX_GetParameter(component->handle, OMX_IndexParamPortDefinition, &port_st)) != OMX_ErrorNone)
        {
            fprintf(stderr, "port enable check fail, %s, port %d, %s\n", component->name, port, dump_OMX_ERRORTYPE(r));
            exit(1);
        }
        usleep(10000);
    }
    printf("%s port %d enabled\n", component->name, port);
}

void wait_disable_port(component_t* component, OMX_U32 port)
{
    printf("Check disable port directly.\n");
    
    OMX_ERRORTYPE r;
    OMX_PARAM_PORTDEFINITIONTYPE port_st;
    OMX_INIT_STRUCTURE(port_st);
    port_st.nPortIndex = port;
    while(port_st.bEnabled != OMX_FALSE)
    {
        if((r = OMX_GetParameter(component->handle, OMX_IndexParamPortDefinition, &port_st)) != OMX_ErrorNone)
        {
            fprintf(stderr, "port disable check fail, %s, port %d, %s\n", component->name, port, dump_OMX_ERRORTYPE(r));
            exit(1);
        }
        usleep(10000);
    }
    printf("%s port %d disabled\n", component->name, port);
}

void wait_state_change(component_t* component, OMX_STATETYPE wanted_state)
{
    printf("check state change directly\n");

    OMX_STATETYPE receive_state;
    while(receive_state != wanted_state)
    {
        OMX_GetState(component->handle, &receive_state);
        usleep(10000);
    }
    printf("%s state chaneged\n", component->name);
}

void init_component(component_t* component)
{
    printf("initializing component %s\n", component->name);

    OMX_ERRORTYPE error;

    //Create the event flags
    if (vcos_event_flags_create(&component->flags, "component"))
    {
        fprintf(stderr, "error: vcos_event_flags_create\n");
        exit(1);
    }

    //Each component has an event_handler and fill_buffer_done functions
    OMX_CALLBACKTYPE callbacks_st;
    callbacks_st.EventHandler = event_handler;
    callbacks_st.FillBufferDone = fill_buffer_done;

    //Get the handle
    if ((error = OMX_GetHandle(&component->handle, component->name, component,
            &callbacks_st)))
    {
        fprintf(stderr, "error: OMX_GetHandle: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    //Disable all the ports
    OMX_INDEXTYPE types[] =
    { OMX_IndexParamAudioInit, OMX_IndexParamVideoInit, OMX_IndexParamImageInit,
            OMX_IndexParamOtherInit };
    OMX_PORT_PARAM_TYPE ports_st;
    OMX_INIT_STRUCTURE(ports_st);

    int i;
    for (i = 0; i < 4; i++)
    {
        if ((error = OMX_GetParameter(component->handle, types[i], &ports_st)))
        {
            fprintf(stderr, "error: OMX_GetParameter: %s\n",
                    dump_OMX_ERRORTYPE(error));
            exit(1);
        }

        OMX_U32 port;
        for (port = ports_st.nStartPortNumber;
                port < ports_st.nStartPortNumber + ports_st.nPorts; port++)
        {
            //Disable the port
            disable_port(component, port);
            //Wait to the event
            //wait(component, EVENT_PORT_DISABLE, 0);
            wait_disable_port(component, port);
        }
    }
}

void deinit_component(component_t* component)
{
    printf("deinitializing component %s\n", component->name);

    OMX_ERRORTYPE error;

    vcos_event_flags_delete(&component->flags);

    if ((error = OMX_FreeHandle(component->handle)))
    {
        fprintf(stderr, "error: OMX_FreeHandle: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }
}

void change_state(component_t* component, OMX_STATETYPE state)
{
    printf("changing %s state to %s\n", component->name,
            dump_OMX_STATETYPE(state));

    OMX_ERRORTYPE error;

    if ((error = OMX_SendCommand(component->handle, OMX_CommandStateSet, state,
            0)))
    {
        fprintf(stderr, "error: OMX_SendCommand: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }
}

void enable_port(component_t* component, OMX_U32 port)
{ 
    printf("enabling port %d (%s)\n", port, component->name);

    OMX_ERRORTYPE error;

    if ((error = OMX_SendCommand(component->handle, OMX_CommandPortEnable, port,
            0)))
    {
        fprintf(stderr, "error: OMX_SendCommand: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }
}

void disable_port(component_t* component, OMX_U32 port)
{
    printf("disabling port %d (%s)\n", port, component->name);

    OMX_ERRORTYPE error;

    if ((error = OMX_SendCommand(component->handle, OMX_CommandPortDisable,
            port, 0)))
    {
        fprintf(stderr, "error: OMX_SendCommand: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }
}
