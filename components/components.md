# components

It is a simple abstraction and collection of settings for OpenMAX components.

## component_common

A collection of commonly used util functions, commonly used settings, etc....

It is a collection of commonly used functions and commonly used settings.

In these settings, there is a preview setting beside the general camera setting. 
In the case of encoding using two encoders at the same time, the preview encoders are set separately in addition to the main encoders.

## OMX_callback

When using OpenMAX, there is a separate thread to process the abstraction layer.  
When the task is finished, it calls the callback function (which may be called an interrupt) with a task completion flag and flags to know the status of each task.

It is up to the user who develops the application how to handle events that indicate that the operation is completed (or something is wrong), 
and this source provides a simple print and blocking function to handle each event generically.

## camera

One of the OMX components has camera-related settings.
The settings that can be set are stored in the component_common side.

```c
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
```

```c
void load_camera_drivers(component_t* component);
void set_camera_port_definition(component_t* camera);
void set_camera_settings(component_t* camera);
```

## resize

One of the OMX components, it is a component for changing between resolutions. 
It shares many configurations with many components.

```c
void set_resize_port_definition(component_t* resize);

void enable_resize_output_port(component_t* resize,
        OMX_BUFFERHEADERTYPE** resize_output_buffer);
void disable_resize_output_port(component_t* resize,
        OMX_BUFFERHEADERTYPE* resize_output_buffer);
```

## H264_encoder

This section is for setting related to OMX component encoder.
There are two settings for the encoder, one for encoding the normal high-quality image and the other for the low-quality image.

By using the above two encoders, two images can be encoded and stored in different ways or at the same time....

```c

void set_h264_port_definition(component_t* encoder);
void set_h264_settings(component_t* encoder);

void set_h264_preview_port_definition(component_t* encoder_prv);
void set_h264_preview_settings(component_t* encoder_prv);

void enable_encoder_output_port(component_t* encoder,
        OMX_BUFFERHEADERTYPE** encoder_output_buffer);
void disable_encoder_output_port(component_t* encoder,
        OMX_BUFFERHEADERTYPE* encoder_output_buffer);
```

## Other components

As you can see from the other sources, other OMX components are being used in addition to the sources mentioned above. Examples are splitter and null sink.

The reason why I did not create the configuration of the above components separately is that OpenMax tunneling function takes the settings of other connected components and uses them.

Likewise, you can see that none of the above-described settings are set for the input port, because the tunneling function takes care of setting the output port of the other component connected.
