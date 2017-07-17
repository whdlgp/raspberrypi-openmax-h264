#ifndef _COMPONENT_COMMON_H_
#define _COMPONENT_COMMON_H_

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include <bcm_host.h>
#include <interface/vcos/vcos.h>
#include <IL/OMX_Broadcom.h>

#include "../dump/dump.h"
#include "OMX_callback.h"

#define OMX_INIT_STRUCTURE(x) \
  memset (&(x), 0, sizeof (x)); \
  (x).nSize = sizeof (x); \
  (x).nVersion.nVersion = OMX_VERSION; \
  (x).nVersion.s.nVersionMajor = OMX_VERSION_MAJOR; \
  (x).nVersion.s.nVersionMinor = OMX_VERSION_MINOR; \
  (x).nVersion.s.nRevision = OMX_VERSION_REVISION; \
  (x).nVersion.s.nStep = OMX_VERSION_STEP

//Encoding setting
#define VIDEO_FRAMERATE 25
#define VIDEO_BITRATE 10000000

//Preview Resizing and Encoding setting
#define PREVIEW_FRAMERATE 25
#define PREVIEW_BITRATE 500000
#define PREVIEW_WIDTH 320
#define PREVIEW_HEIGHT 240
#define PREVIEW_SPS_PPS_INLINE OMX_TRUE
#define PREVIEW_IDR_PERIOD 5

//Camera component port setting
//Some settings doesn't work well
#define CAM_WIDTH 1920
#define CAM_HEIGHT 1080

//Camera component setting
#define CAM_SHARPNESS 0 //-100 .. 100
#define CAM_CONTRAST 0 //-100 .. 100
#define CAM_BRIGHTNESS 50 //0 .. 100
#define CAM_SATURATION 0 //-100 .. 100
#define CAM_SHUTTER_SPEED_AUTO OMX_TRUE
#define CAM_SHUTTER_SPEED 1.0/8.0
#define CAM_ISO_AUTO OMX_TRUE
#define CAM_ISO 100 //100 .. 800
#define CAM_EXPOSURE OMX_ExposureControlAuto
#define CAM_EXPOSURE_COMPENSATION 0 //-24 .. 24
#define CAM_MIRROR OMX_MirrorNone
#define CAM_ROTATION 0 //0 90 180 270
#define CAM_COLOR_ENABLE OMX_FALSE
#define CAM_COLOR_U 128 //0 .. 255
#define CAM_COLOR_V 128 //0 .. 255
#define CAM_NOISE_REDUCTION OMX_TRUE
#define CAM_FRAME_STABILIZATION OMX_FALSE
#define CAM_METERING OMX_MeteringModeAverage
#define CAM_WHITE_BALANCE OMX_WhiteBalControlAuto
//The gains are used if the white balance is set to off
#define CAM_WHITE_BALANCE_RED_GAIN 1000 //0 ..
#define CAM_WHITE_BALANCE_BLUE_GAIN 1000 //0 ..
#define CAM_IMAGE_FILTER OMX_ImageFilterNone
#define CAM_ROI_TOP 0 //0 .. 100
#define CAM_ROI_LEFT 0 //0 .. 100
#define CAM_ROI_WIDTH 100 //0 .. 100
#define CAM_ROI_HEIGHT 100 //0 .. 100
#define CAM_DRC OMX_DynRangeExpOff

/*
 Possible values:

 CAM_EXPOSURE
 OMX_ExposureControlOff
 OMX_ExposureControlAuto
 OMX_ExposureControlNight
 OMX_ExposureControlBackLight
 OMX_ExposureControlSpotlight
 OMX_ExposureControlSports
 OMX_ExposureControlSnow
 OMX_ExposureControlBeach
 OMX_ExposureControlLargeAperture
 OMX_ExposureControlSmallAperture
 OMX_ExposureControlVeryLong
 OMX_ExposureControlFixedFps
 OMX_ExposureControlNightWithPreview
 OMX_ExposureControlAntishake
 OMX_ExposureControlFireworks

 CAM_IMAGE_FILTER
 OMX_ImageFilterNone
 OMX_ImageFilterEmboss
 OMX_ImageFilterNegative
 OMX_ImageFilterSketch
 OMX_ImageFilterOilPaint
 OMX_ImageFilterHatch
 OMX_ImageFilterGpen
 OMX_ImageFilterSolarize
 OMX_ImageFilterWatercolor
 OMX_ImageFilterPastel
 OMX_ImageFilterFilm
 OMX_ImageFilterBlur
 OMX_ImageFilterColourSwap
 OMX_ImageFilterWashedOut
 OMX_ImageFilterColourPoint
 OMX_ImageFilterPosterise
 OMX_ImageFilterColourBalance
 OMX_ImageFilterCartoon

 CAM_METERING
 OMX_MeteringModeAverage
 OMX_MeteringModeSpot
 OMX_MeteringModeMatrix
 OMX_MeteringModeBacklit

 CAM_MIRROR
 OMX_MirrorNone
 OMX_MirrorHorizontal
 OMX_MirrorVertical
 OMX_MirrorBoth

 CAM_WHITE_BALANCE
 OMX_WhiteBalControlOff
 OMX_WhiteBalControlAuto
 OMX_WhiteBalControlSunLight
 OMX_WhiteBalControlCloudy
 OMX_WhiteBalControlShade
 OMX_WhiteBalControlTungsten
 OMX_WhiteBalControlFluorescent
 OMX_WhiteBalControlIncandescent
 OMX_WhiteBalControlFlash
 OMX_WhiteBalControlHorizon

 CAM_DRC
 OMX_DynRangeExpOff
 OMX_DynRangeExpLow
 OMX_DynRangeExpMedium
 OMX_DynRangeExpHigh

 VIDEO_PROFILE
 OMX_VIDEO_AVCProfileHigh
 OMX_VIDEO_AVCProfileBaseline
 OMX_VIDEO_AVCProfileMain
 */

//Data of each component
typedef struct
{
    //The handle is obtained with OMX_GetHandle() and is used on every function
    //that needs to manipulate a component. It is released with OMX_FreeHandle()
    OMX_HANDLETYPE handle;
    //Bitwise OR of flags. Used for blocking the current thread and waiting an
    //event. Used with vcos_event_flags_get() and vcos_event_flags_set()
    VCOS_EVENT_FLAGS_T flags;
    //The fullname of the component
    OMX_STRING name;
} component_t;

//Prototypes
void wake(component_t* component, VCOS_UNSIGNED event);
void wait(component_t* component, VCOS_UNSIGNED events,
        VCOS_UNSIGNED* retrieved_events);
void wait_enable_port(component_t* component, OMX_U32 port);
void wait_disable_port(component_t* component, OMX_U32 port);
void wait_state_change(component_t* component, OMX_STATETYPE wanted_state);

void init_component(component_t* component);
void deinit_component(component_t* component);
void change_state(component_t* component, OMX_STATETYPE state);
void enable_port(component_t* component, OMX_U32 port);
void disable_port(component_t* component, OMX_U32 port);

#endif
