#ifndef DUMP_H
#define DUMP_H

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <IL/OMX_Broadcom.h>

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

//time stamp for checking latancy of video
#define TIME_STAMP

#ifdef TIME_STAMP

#define DECLARE_TIME(timeval) uint64_t (timeval) = 0;
#define START_TIME(timeval) (timeval) = GetTimeStamp();
#define STOP_TIME(timeval) (timeval) = GetTimeStamp() - (timeval);
#define PRINT_EXECUTION_TIME(timeval) printf(#timeval" execution time : %" PRIu64 " us\n", (timeval));

#else

#define DECLARE_TIME(timeval)
#define START_TIME(timeval)
#define STOP_TIME(timeval)
#define PRINT_EXECUTION_TIME(timeval)

#endif

uint64_t GetTimeStamp();

#endif
