#include "camera.h"

void load_camera_drivers(component_t* component)
{
    /*
     This is a specific behaviour of the Broadcom's Raspberry Pi OpenMAX IL
     implementation module because the OMX_SetConfig() and OMX_SetParameter() are
     blocking functions but the drivers are loaded asynchronously, that is, an
     event is fired to signal the completion. Basically, what you're saying is:

     "When the parameter with index OMX_IndexParamCameraDeviceNumber is set, load
     the camera drivers and emit an OMX_EventParamOrConfigChanged event"

     The red LED of the camera will be turned on after this call.
     */

    printf("loading camera drivers\n");

    OMX_ERRORTYPE error;

    OMX_CONFIG_REQUESTCALLBACKTYPE cbs_st;
    OMX_INIT_STRUCTURE(cbs_st);
    cbs_st.nPortIndex = OMX_ALL;
    cbs_st.nIndex = OMX_IndexParamCameraDeviceNumber;
    cbs_st.bEnable = OMX_TRUE;
    if ((error = OMX_SetConfig(component->handle,
            OMX_IndexConfigRequestCallback, &cbs_st)))
    {
        fprintf(stderr, "error: OMX_SetConfig: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    OMX_PARAM_U32TYPE dev_st;
    OMX_INIT_STRUCTURE(dev_st);
    dev_st.nPortIndex = OMX_ALL;
    //ID for the camera device
    dev_st.nU32 = 0;
    if ((error = OMX_SetParameter(component->handle,
            OMX_IndexParamCameraDeviceNumber, &dev_st)))
    {
        fprintf(stderr, "error: OMX_SetParameter: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    wait(component, EVENT_PARAM_OR_CONFIG_CHANGED, 0);
}

void set_camera_port_definition(component_t* camera)
{
    //Configure camera port definition
    
    OMX_ERRORTYPE error;
    
    printf("configuring %s port definition\n", camera->name);
    OMX_PARAM_PORTDEFINITIONTYPE port_st;
    OMX_INIT_STRUCTURE(port_st);
    port_st.nPortIndex = 71;
    if ((error = OMX_GetParameter(camera->handle, OMX_IndexParamPortDefinition,
            &port_st)))
    {
        fprintf(stderr, "error: OMX_GetParameter: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    port_st.format.video.nFrameWidth = CAM_WIDTH;
    port_st.format.video.nFrameHeight = CAM_HEIGHT;
    port_st.format.video.nStride = CAM_WIDTH;
    port_st.format.video.xFramerate = VIDEO_FRAMERATE << 16;
    port_st.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    port_st.format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
    if ((error = OMX_SetParameter(camera->handle, OMX_IndexParamPortDefinition,
            &port_st)))
    {
        fprintf(stderr, "error: OMX_SetParameter: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    //Preview port
    port_st.nPortIndex = 70;
    if ((error = OMX_SetParameter(camera->handle, OMX_IndexParamPortDefinition,
            &port_st)))
    {
        fprintf(stderr, "error: OMX_SetParameter: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    //Configure framerate of camera, use for encoder?
    printf("configuring %s framerate\n", camera->name);
    OMX_CONFIG_FRAMERATETYPE framerate_st;
    OMX_INIT_STRUCTURE(framerate_st);
    framerate_st.nPortIndex = 71;
    framerate_st.xEncodeFramerate = port_st.format.video.xFramerate;
    if ((error = OMX_SetConfig(camera->handle, OMX_IndexConfigVideoFramerate,
            &framerate_st)))
    {
        fprintf(stderr, "error: OMX_SetConfig: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    //Preview port
    framerate_st.nPortIndex = 70;
    if ((error = OMX_SetConfig(camera->handle, OMX_IndexConfigVideoFramerate,
            &framerate_st)))
    {
        fprintf(stderr, "error: OMX_SetConfig: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }
}

void set_camera_settings(component_t* camera)
{
    printf("configuring '%s' settings\n", camera->name);

    OMX_ERRORTYPE error;

    //Sharpness
    OMX_CONFIG_SHARPNESSTYPE sharpness_st;
    OMX_INIT_STRUCTURE(sharpness_st);
    sharpness_st.nPortIndex = OMX_ALL;
    sharpness_st.nSharpness = CAM_SHARPNESS;
    if ((error = OMX_SetConfig(camera->handle, OMX_IndexConfigCommonSharpness,
            &sharpness_st)))
    {
        fprintf(stderr, "error: OMX_SetConfig: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    //Contrast
    OMX_CONFIG_CONTRASTTYPE contrast_st;
    OMX_INIT_STRUCTURE(contrast_st);
    contrast_st.nPortIndex = OMX_ALL;
    contrast_st.nContrast = CAM_CONTRAST;
    if ((error = OMX_SetConfig(camera->handle, OMX_IndexConfigCommonContrast,
            &contrast_st)))
    {
        fprintf(stderr, "error: OMX_SetConfig: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    //Saturation
    OMX_CONFIG_SATURATIONTYPE saturation_st;
    OMX_INIT_STRUCTURE(saturation_st);
    saturation_st.nPortIndex = OMX_ALL;
    saturation_st.nSaturation = CAM_SATURATION;
    if ((error = OMX_SetConfig(camera->handle, OMX_IndexConfigCommonSaturation,
            &saturation_st)))
    {
        fprintf(stderr, "error: OMX_SetConfig: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    //Brightness
    OMX_CONFIG_BRIGHTNESSTYPE brightness_st;
    OMX_INIT_STRUCTURE(brightness_st);
    brightness_st.nPortIndex = OMX_ALL;
    brightness_st.nBrightness = CAM_BRIGHTNESS;
    if ((error = OMX_SetConfig(camera->handle, OMX_IndexConfigCommonBrightness,
            &brightness_st)))
    {
        fprintf(stderr, "error: OMX_SetConfig: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    //Exposure value
    OMX_CONFIG_EXPOSUREVALUETYPE exposure_value_st;
    OMX_INIT_STRUCTURE(exposure_value_st);
    exposure_value_st.nPortIndex = OMX_ALL;
    exposure_value_st.eMetering = CAM_METERING;
    exposure_value_st.xEVCompensation = (OMX_S32)(
            (CAM_EXPOSURE_COMPENSATION << 16) / 6.0);
    exposure_value_st.nShutterSpeedMsec = (OMX_U32)((CAM_SHUTTER_SPEED) * 1e6);
    exposure_value_st.bAutoShutterSpeed = CAM_SHUTTER_SPEED_AUTO;
    exposure_value_st.nSensitivity = CAM_ISO;
    exposure_value_st.bAutoSensitivity = CAM_ISO_AUTO;
    if ((error = OMX_SetConfig(camera->handle,
            OMX_IndexConfigCommonExposureValue, &exposure_value_st)))
    {
        fprintf(stderr, "error: OMX_SetConfig: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    //Exposure control
    OMX_CONFIG_EXPOSURECONTROLTYPE exposure_control_st;
    OMX_INIT_STRUCTURE(exposure_control_st);
    exposure_control_st.nPortIndex = OMX_ALL;
    exposure_control_st.eExposureControl = CAM_EXPOSURE;
    if ((error = OMX_SetConfig(camera->handle, OMX_IndexConfigCommonExposure,
            &exposure_control_st)))
    {
        fprintf(stderr, "error: OMX_SetConfig: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    //Frame stabilisation
    OMX_CONFIG_FRAMESTABTYPE frame_stabilisation_st;
    OMX_INIT_STRUCTURE(frame_stabilisation_st);
    frame_stabilisation_st.nPortIndex = OMX_ALL;
    frame_stabilisation_st.bStab = CAM_FRAME_STABILIZATION;
    if ((error = OMX_SetConfig(camera->handle,
            OMX_IndexConfigCommonFrameStabilisation, &frame_stabilisation_st)))
    {
        fprintf(stderr, "error: OMX_SetConfig: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    //White balance
    OMX_CONFIG_WHITEBALCONTROLTYPE white_balance_st;
    OMX_INIT_STRUCTURE(white_balance_st);
    white_balance_st.nPortIndex = OMX_ALL;
    white_balance_st.eWhiteBalControl = CAM_WHITE_BALANCE;
    if ((error = OMX_SetConfig(camera->handle,
            OMX_IndexConfigCommonWhiteBalance, &white_balance_st)))
    {
        fprintf(stderr, "error: OMX_SetConfig: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    //White balance gains (if white balance is set to off)
    if (!CAM_WHITE_BALANCE)
    {
        OMX_CONFIG_CUSTOMAWBGAINSTYPE white_balance_gains_st;
        OMX_INIT_STRUCTURE(white_balance_gains_st);
        white_balance_gains_st.xGainR = (CAM_WHITE_BALANCE_RED_GAIN << 16)
                / 1000;
        white_balance_gains_st.xGainB = (CAM_WHITE_BALANCE_BLUE_GAIN << 16)
                / 1000;
        if ((error = OMX_SetConfig(camera->handle,
                OMX_IndexConfigCustomAwbGains, &white_balance_gains_st)))
        {
            fprintf(stderr, "error: OMX_SetConfig: %s\n",
                    dump_OMX_ERRORTYPE(error));
            exit(1);
        }
    }

    //Image filter
    OMX_CONFIG_IMAGEFILTERTYPE image_filter_st;
    OMX_INIT_STRUCTURE(image_filter_st);
    image_filter_st.nPortIndex = OMX_ALL;
    image_filter_st.eImageFilter = CAM_IMAGE_FILTER;
    if ((error = OMX_SetConfig(camera->handle, OMX_IndexConfigCommonImageFilter,
            &image_filter_st)))
    {
        fprintf(stderr, "error: OMX_SetConfig: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    //Mirror
    OMX_CONFIG_MIRRORTYPE mirror_st;
    OMX_INIT_STRUCTURE(mirror_st);
    mirror_st.nPortIndex = 71;
    mirror_st.eMirror = CAM_MIRROR;
    if ((error = OMX_SetConfig(camera->handle, OMX_IndexConfigCommonMirror,
            &mirror_st)))
    {
        fprintf(stderr, "error: OMX_SetConfig: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    //Rotation
    OMX_CONFIG_ROTATIONTYPE rotation_st;
    OMX_INIT_STRUCTURE(rotation_st);
    rotation_st.nPortIndex = 71;
    rotation_st.nRotation = CAM_ROTATION;
    if ((error = OMX_SetConfig(camera->handle, OMX_IndexConfigCommonRotate,
            &rotation_st)))
    {
        fprintf(stderr, "error: OMX_SetConfig: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    //Color enhancement
    OMX_CONFIG_COLORENHANCEMENTTYPE color_enhancement_st;
    OMX_INIT_STRUCTURE(color_enhancement_st);
    color_enhancement_st.nPortIndex = OMX_ALL;
    color_enhancement_st.bColorEnhancement = CAM_COLOR_ENABLE;
    color_enhancement_st.nCustomizedU = CAM_COLOR_U;
    color_enhancement_st.nCustomizedV = CAM_COLOR_V;
    if ((error = OMX_SetConfig(camera->handle,
            OMX_IndexConfigCommonColorEnhancement, &color_enhancement_st)))
    {
        fprintf(stderr, "error: OMX_SetConfig: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    //Denoise
    OMX_CONFIG_BOOLEANTYPE denoise_st;
    OMX_INIT_STRUCTURE(denoise_st);
    denoise_st.bEnabled = CAM_NOISE_REDUCTION;
    if ((error = OMX_SetConfig(camera->handle,
            OMX_IndexConfigStillColourDenoiseEnable, &denoise_st)))
    {
        fprintf(stderr, "error: OMX_SetConfig: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    //ROI
    OMX_CONFIG_INPUTCROPTYPE roi_st;
    OMX_INIT_STRUCTURE(roi_st);
    roi_st.nPortIndex = OMX_ALL;
    roi_st.xLeft = (CAM_ROI_LEFT << 16) / 100;
    roi_st.xTop = (CAM_ROI_TOP << 16) / 100;
    roi_st.xWidth = (CAM_ROI_WIDTH << 16) / 100;
    roi_st.xHeight = (CAM_ROI_HEIGHT << 16) / 100;
    if ((error = OMX_SetConfig(camera->handle,
            OMX_IndexConfigInputCropPercentages, &roi_st)))
    {
        fprintf(stderr, "error: OMX_SetConfig: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }

    //DRC
    OMX_CONFIG_DYNAMICRANGEEXPANSIONTYPE drc_st;
    OMX_INIT_STRUCTURE(drc_st);
    drc_st.eMode = CAM_DRC;
    if ((error = OMX_SetConfig(camera->handle,
            OMX_IndexConfigDynamicRangeExpansion, &drc_st)))
    {
        fprintf(stderr, "error: OMX_SetConfig: %s\n",
                dump_OMX_ERRORTYPE(error));
        exit(1);
    }
}
