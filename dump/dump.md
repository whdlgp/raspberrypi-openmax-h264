# dump

Setting up OpenMAX is a bit complicated. It is difficult to find the proper setting at once, and it is inconvenient to debug using GDB, etc., so it is necessary to check how each component is set and operated.

To accomplish this, we have collected simple utilities that can be used with each print function.

```c
const char* dump_OMX_COLOR_FORMATTYPE (OMX_COLOR_FORMATTYPE color);
const char* dump_OMX_OTHER_FORMATTYPE (OMX_OTHER_FORMATTYPE format);
const char* dump_OMX_AUDIO_CODINGTYPE (OMX_AUDIO_CODINGTYPE coding);
const char* dump_OMX_VIDEO_CODINGTYPE (OMX_VIDEO_CODINGTYPE coding);
const char* dump_OMX_IMAGE_CODINGTYPE (OMX_IMAGE_CODINGTYPE coding);
const char* dump_OMX_STATETYPE (OMX_STATETYPE state);
const char* dump_OMX_ERRORTYPE (OMX_ERRORTYPE error);
const char* dump_OMX_EVENTTYPE (OMX_EVENTTYPE event);
const char* dump_OMX_INDEXTYPE (OMX_INDEXTYPE type);
void dump_OMX_PARAM_PORTDEFINITIONTYPE (OMX_PARAM_PORTDEFINITIONTYPE* port);
void dump_OMX_IMAGE_PARAM_PORTFORMATTYPE (OMX_IMAGE_PARAM_PORTFORMATTYPE* port);
void dump_OMX_BUFFERHEADERTYPE (OMX_BUFFERHEADERTYPE* header);
int printNALFrame(unsigned char *frame, int len);
```
These are prints to check various states of OpenMAX.

```c
#define DECLARE_TIME(timeval)
#define START_TIME(timeval)
#define STOP_TIME(timeval)
#define PRINT_EXECUTION_TIME(timeval)

uint64_t GetTimeStamp();
```

It is a function that can be used to partially check execution time.

