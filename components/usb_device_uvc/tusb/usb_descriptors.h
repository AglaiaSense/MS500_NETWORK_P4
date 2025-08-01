/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Jerzy Kasenbreg
 * Copyright (c) 2021 Koji KITAYAMA
 * Copyright (c) 2023 Espressif
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef _USB_DESCRIPTORS_H_
#define _USB_DESCRIPTORS_H_

#include "uvc_frame_config.h"
/* Time stamp base clock. It is a deprecated parameter. */
#define UVC_CLOCK_FREQUENCY    27000000
/* video capture path */
#define UVC_ENTITY_CAP_INPUT_TERMINAL  0x01
#define UVC_ENTITY_CAP_OUTPUT_TERMINAL 0x02

enum {
    ITF_NUM_CDC = 0,          // CDC 控制接口
    ITF_NUM_CDC_DATA,         // CDC 数据接口
    ITF_NUM_VIDEO_CONTROL,    // UVC 控制接口
    ITF_NUM_VIDEO_STREAMING,  // UVC 视频流接口
#if CONFIG_UVC_SUPPORT_TWO_CAM
    ITF_NUM_VIDEO_CONTROL_2,
    ITF_NUM_VIDEO_STREAMING_2,
#endif
    ITF_NUM_TOTAL
};

//------------- USB Endpoint numbers -------------//
enum {
    EP_EMPTY = 0,
#if CFG_TUD_CDC
    EPNUM_0_CDC_NOTIF,
    EPNUM_0_CDC,
#endif

#if CFG_TUD_MSC
    EPNUM_MSC,
#endif

#if CFG_TUD_NCM
    EPNUM_NET_NOTIF,
    EPNUM_NET_DATA,
#endif

#if CFG_TUD_VENDOR
    EPNUM_0_VENDOR,
#endif
 
};

//------------- STRID -------------//
enum {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
#if CFG_TUD_CDC
    STRID_CDC_INTERFACE,
#endif

#if CFG_TUD_MSC
    STRID_MSC_INTERFACE,
#endif

#if CFG_TUD_NCM
    STRID_NET_INTERFACE,
    STRID_MAC,
#endif

#if CFG_TUD_VENDOR
    STRID_VENDOR_INTERFACE,
#endif
};


#if (CFG_TUD_VIDEO)
#define TUD_VIDEO_CAPTURE_DESC_UNCOMPR_LEN (\
    TUD_VIDEO_DESC_IAD_LEN\
    /* control */\
    + TUD_VIDEO_DESC_STD_VC_LEN\
    + (TUD_VIDEO_DESC_CS_VC_LEN + 1/*bInCollection*/)\
    + TUD_VIDEO_DESC_CAMERA_TERM_LEN\
    + TUD_VIDEO_DESC_OUTPUT_TERM_LEN\
    /* Interface 1, Alternate 0 */\
    + TUD_VIDEO_DESC_STD_VS_LEN\
    + (TUD_VIDEO_DESC_CS_VS_IN_LEN + 1/*bNumFormats x bControlSize*/)\
    + TUD_VIDEO_DESC_CS_VS_FMT_UNCOMPR_LEN\
    + TUD_VIDEO_DESC_CS_VS_FRM_UNCOMPR_CONT_LEN\
    + TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING_LEN\
    /* Interface 1, Alternate 1 */\
    + TUD_VIDEO_DESC_STD_VS_LEN\
    + 7/* Endpoint */\
  )

#define TUD_VIDEO_CAPTURE_DESC_MJPEG_LEN (\
    TUD_VIDEO_DESC_IAD_LEN\
    /* control */\
    + TUD_VIDEO_DESC_STD_VC_LEN\
    + (TUD_VIDEO_DESC_CS_VC_LEN + 1/*bInCollection*/)\
    + TUD_VIDEO_DESC_CAMERA_TERM_LEN\
    + TUD_VIDEO_DESC_OUTPUT_TERM_LEN\
    /* Interface 1, Alternate 0 */\
    + TUD_VIDEO_DESC_STD_VS_LEN\
    + (TUD_VIDEO_DESC_CS_VS_IN_LEN + 1/*bNumFormats x bControlSize*/)\
    + TUD_VIDEO_DESC_CS_VS_FMT_MJPEG_LEN\
    + TUD_VIDEO_DESC_CS_VS_FRM_MJPEG_CONT_LEN\
    + TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING_LEN\
    /* Interface 1, Alternate 1 */\
    + TUD_VIDEO_DESC_STD_VS_LEN\
    + 7/* Endpoint */\
  )

#define TUD_VIDEO_CAPTURE_DESC_MULTI_MJPEG_LEN(n) (\
    TUD_VIDEO_DESC_IAD_LEN\
    /* control */\
    + TUD_VIDEO_DESC_STD_VC_LEN\
    + (TUD_VIDEO_DESC_CS_VC_LEN + 1/*bInCollection*/)\
    + TUD_VIDEO_DESC_CAMERA_TERM_LEN\
    + TUD_VIDEO_DESC_OUTPUT_TERM_LEN\
    /* Interface 1, Alternate 0 */\
    + TUD_VIDEO_DESC_STD_VS_LEN\
    + (TUD_VIDEO_DESC_CS_VS_IN_LEN + 1/*bNumFormats x bControlSize*/)\
    + TUD_VIDEO_DESC_CS_VS_FMT_MJPEG_LEN\
    + (n*TUD_VIDEO_DESC_CS_VS_FRM_MJPEG_CONT_LEN)\
    + TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING_LEN\
    /* Interface 1, Alternate 1 */\
    + TUD_VIDEO_DESC_STD_VS_LEN\
    + 7/* Endpoint */\
  )

#define TUD_VIDEO_CAPTURE_DESC_UNCOMPR_BULK_LEN (\
    TUD_VIDEO_DESC_IAD_LEN\
    /* control */\
    + TUD_VIDEO_DESC_STD_VC_LEN\
    + (TUD_VIDEO_DESC_CS_VC_LEN + 1/*bInCollection*/)\
    + TUD_VIDEO_DESC_CAMERA_TERM_LEN\
    + TUD_VIDEO_DESC_OUTPUT_TERM_LEN\
    /* Interface 1, Alternate 0 */\
    + TUD_VIDEO_DESC_STD_VS_LEN\
    + (TUD_VIDEO_DESC_CS_VS_IN_LEN + 1/*bNumFormats x bControlSize*/)\
    + TUD_VIDEO_DESC_CS_VS_FMT_UNCOMPR_LEN\
    + TUD_VIDEO_DESC_CS_VS_FRM_UNCOMPR_CONT_LEN\
    + TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING_LEN\
    + 7/* Endpoint */\
  )

#define TUD_VIDEO_CAPTURE_DESC_MJPEG_BULK_LEN (\
    TUD_VIDEO_DESC_IAD_LEN\
    /* control */\
    + TUD_VIDEO_DESC_STD_VC_LEN\
    + (TUD_VIDEO_DESC_CS_VC_LEN + 1/*bInCollection*/)\
    + TUD_VIDEO_DESC_CAMERA_TERM_LEN\
    + TUD_VIDEO_DESC_OUTPUT_TERM_LEN\
    /* Interface 1, Alternate 0 */\
    + TUD_VIDEO_DESC_STD_VS_LEN\
    + (TUD_VIDEO_DESC_CS_VS_IN_LEN + 1/*bNumFormats x bControlSize*/)\
    + TUD_VIDEO_DESC_CS_VS_FMT_MJPEG_LEN\
    + TUD_VIDEO_DESC_CS_VS_FRM_MJPEG_CONT_LEN\
    + TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING_LEN\
    + 7/* Endpoint */\
  )

#define TUD_VIDEO_CAPTURE_DESC_MULTI_MJPEG_BULK_LEN(n) (\
    TUD_VIDEO_DESC_IAD_LEN\
    /* control */\
    + TUD_VIDEO_DESC_STD_VC_LEN\
    + (TUD_VIDEO_DESC_CS_VC_LEN + 1/*bInCollection*/)\
    + TUD_VIDEO_DESC_CAMERA_TERM_LEN\
    + TUD_VIDEO_DESC_OUTPUT_TERM_LEN\
    /* Interface 1, Alternate 0 */\
    + TUD_VIDEO_DESC_STD_VS_LEN\
    + (TUD_VIDEO_DESC_CS_VS_IN_LEN + 1/*bNumFormats x bControlSize*/)\
    + TUD_VIDEO_DESC_CS_VS_FMT_MJPEG_LEN\
    + (n*TUD_VIDEO_DESC_CS_VS_FRM_MJPEG_CONT_LEN)\
    + TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING_LEN\
    + 7/* Endpoint */\
  )

#define TUD_VIDEO_CAPTURE_DESC_MULTI_FRAME_BASED_BULK_LEN(n) (\
    TUD_VIDEO_DESC_IAD_LEN\
    /* control */\
    + TUD_VIDEO_DESC_STD_VC_LEN\
    + (TUD_VIDEO_DESC_CS_VC_LEN + 1/*bInCollection*/)\
    + TUD_VIDEO_DESC_CAMERA_TERM_LEN\
    + TUD_VIDEO_DESC_OUTPUT_TERM_LEN\
    /* Interface 1, Alternate 0 */\
    + TUD_VIDEO_DESC_STD_VS_LEN\
    + (TUD_VIDEO_DESC_CS_VS_IN_LEN + 1/*bNumFormats x bControlSize*/)\
    + TUD_VIDEO_DESC_CS_VS_FMT_FRAME_BASED_LEN\
    + (n*TUD_VIDEO_DESC_CS_VS_FRM_FRAME_BASED_CONT_LEN)\
    + TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING_LEN\
    + 7/* Endpoint */\
  )

#define TUD_VIDEO_CAPTURE_DESC_FRAME_BASED_BULK_LEN (\
    TUD_VIDEO_DESC_IAD_LEN\
    /* control */\
    + TUD_VIDEO_DESC_STD_VC_LEN\
    + (TUD_VIDEO_DESC_CS_VC_LEN + 1/*bInCollection*/)\
    + TUD_VIDEO_DESC_CAMERA_TERM_LEN\
    + TUD_VIDEO_DESC_OUTPUT_TERM_LEN\
    /* Interface 1, Alternate 0 */\
    + TUD_VIDEO_DESC_STD_VS_LEN\
    + (TUD_VIDEO_DESC_CS_VS_IN_LEN + 1/*bNumFormats x bControlSize*/)\
    + TUD_VIDEO_DESC_CS_VS_FMT_FRAME_BASED_LEN\
    + TUD_VIDEO_DESC_CS_VS_FRM_FRAME_BASED_CONT_LEN\
    + TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING_LEN\
    + 7/* Endpoint */\
  )

#define TUD_VIDEO_CAPTURE_DESC_FRAME_BASED_LEN (\
    TUD_VIDEO_DESC_IAD_LEN\
    /* control */\
    + TUD_VIDEO_DESC_STD_VC_LEN\
    + (TUD_VIDEO_DESC_CS_VC_LEN + 1/*bInCollection*/)\
    + TUD_VIDEO_DESC_CAMERA_TERM_LEN\
    + TUD_VIDEO_DESC_OUTPUT_TERM_LEN\
    /* Interface 1, Alternate 0 */\
    + TUD_VIDEO_DESC_STD_VS_LEN\
    + (TUD_VIDEO_DESC_CS_VS_IN_LEN + 1/*bNumFormats x bControlSize*/)\
    + TUD_VIDEO_DESC_CS_VS_FMT_FRAME_BASED_LEN\
    + TUD_VIDEO_DESC_CS_VS_FRM_FRAME_BASED_CONT_LEN\
    + TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING_LEN\
    /* Interface 1, Alternate 1 */\
    + TUD_VIDEO_DESC_STD_VS_LEN\
    + 7/* Endpoint */\
  )

#define TUD_VIDEO_CAPTURE_DESC_MULTI_FRAME_BASED_LEN(n) (\
    TUD_VIDEO_DESC_IAD_LEN\
    /* control */\
    + TUD_VIDEO_DESC_STD_VC_LEN\
    + (TUD_VIDEO_DESC_CS_VC_LEN + 1/*bInCollection*/)\
    + TUD_VIDEO_DESC_CAMERA_TERM_LEN\
    + TUD_VIDEO_DESC_OUTPUT_TERM_LEN\
    /* Interface 1, Alternate 0 */\
    + TUD_VIDEO_DESC_STD_VS_LEN\
    + (TUD_VIDEO_DESC_CS_VS_IN_LEN + 1/*bNumFormats x bControlSize*/)\
    + TUD_VIDEO_DESC_CS_VS_FMT_FRAME_BASED_LEN\
    + (n*TUD_VIDEO_DESC_CS_VS_FRM_FRAME_BASED_CONT_LEN)\
    + TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING_LEN\
    /* Interface 1, Alternate 1 */\
    + TUD_VIDEO_DESC_STD_VS_LEN\
    + 7/* Endpoint */\
  )

/* Windows support YUY2 and NV12
 * https://docs.microsoft.com/en-us/windows-hardware/drivers/stream/usb-video-class-driver-overview */

#define TUD_VIDEO_DESC_CS_VS_FMT_YUY2(_fmtidx, _numfmtdesc, _frmidx, _asrx, _asry, _interlace, _cp) \
  TUD_VIDEO_DESC_CS_VS_FMT_UNCOMPR(_fmtidx, _numfmtdesc, TUD_VIDEO_GUID_YUY2, 16, _frmidx, _asrx, _asry, _interlace, _cp)
#define TUD_VIDEO_DESC_CS_VS_FMT_NV12(_fmtidx, _numfmtdesc, _frmidx, _asrx, _asry, _interlace, _cp) \
  TUD_VIDEO_DESC_CS_VS_FMT_UNCOMPR(_fmtidx, _numfmtdesc, TUD_VIDEO_GUID_NV12, 12, _frmidx, _asrx, _asry, _interlace, _cp)
#define TUD_VIDEO_DESC_CS_VS_FMT_M420(_fmtidx, _numfmtdesc, _frmidx, _asrx, _asry, _interlace, _cp) \
  TUD_VIDEO_DESC_CS_VS_FMT_UNCOMPR(_fmtidx, _numfmtdesc, TUD_VIDEO_GUID_M420, 12, _frmidx, _asrx, _asry, _interlace, _cp)
#define TUD_VIDEO_DESC_CS_VS_FMT_I420(_fmtidx, _numfmtdesc, _frmidx, _asrx, _asry, _interlace, _cp) \
  TUD_VIDEO_DESC_CS_VS_FMT_UNCOMPR(_fmtidx, _numfmtdesc, TUD_VIDEO_GUID_I420, 12, _frmidx, _asrx, _asry, _interlace, _cp)

#define TUD_VIDEO_DESC_CS_VS_FRM_MJPEG_CONT_TEMPLATE(bCamIndex ,bFrameIndex) \
        TUD_VIDEO_DESC_CS_VS_FRM_MJPEG_CONT(bFrameIndex, 0, UVC_FRAMES_INFO[bCamIndex][bFrameIndex-1].width, UVC_FRAMES_INFO[bCamIndex][bFrameIndex-1].height, \
            UVC_FRAMES_INFO[bCamIndex][bFrameIndex-1].width * UVC_FRAMES_INFO[bCamIndex][bFrameIndex-1].height * 16, UVC_FRAMES_INFO[bCamIndex][bFrameIndex-1].width * UVC_FRAMES_INFO[bCamIndex][bFrameIndex-1].height * 16 * UVC_FRAMES_INFO[bCamIndex][bFrameIndex-1].rate, \
            UVC_FRAMES_INFO[bCamIndex][bFrameIndex-1].width * UVC_FRAMES_INFO[bCamIndex][bFrameIndex-1].height * 16 / 8, \
            (10000000/UVC_FRAMES_INFO[bCamIndex][bFrameIndex-1].rate), (10000000/UVC_FRAMES_INFO[bCamIndex][bFrameIndex-1].rate), (10000000/UVC_FRAMES_INFO[bCamIndex][bFrameIndex-1].rate), (10000000/UVC_FRAMES_INFO[bCamIndex][bFrameIndex-1].rate))

#define TUD_VIDEO_DESC_CS_VS_FRM_H264_CONT_TEMPLATE(bCamIndex ,bFrameIndex) \
        TUD_VIDEO_DESC_CS_VS_FRM_FRAME_BASED_CONT(bFrameIndex, 0, UVC_FRAMES_INFO[bCamIndex][bFrameIndex-1].width, UVC_FRAMES_INFO[bCamIndex][bFrameIndex-1].height, \
            UVC_FRAMES_INFO[bCamIndex][bFrameIndex-1].width * UVC_FRAMES_INFO[bCamIndex][bFrameIndex-1].height * 16, UVC_FRAMES_INFO[bCamIndex][bFrameIndex-1].width * UVC_FRAMES_INFO[bCamIndex][bFrameIndex-1].height * 16 * UVC_FRAMES_INFO[bCamIndex][bFrameIndex-1].rate, \
            (10000000/UVC_FRAMES_INFO[bCamIndex][bFrameIndex-1].rate), /*bytesPreLine*/ 0, (10000000/UVC_FRAMES_INFO[bCamIndex][bFrameIndex-1].rate), (10000000/UVC_FRAMES_INFO[bCamIndex][bFrameIndex-1].rate), (10000000/UVC_FRAMES_INFO[bCamIndex][bFrameIndex-1].rate))

#define TUD_VIDEO_CAPTURE_DESCRIPTOR_UNCOMPR(_stridx, _itf,_epin, _width, _height, _fps, _epsize) \
  TUD_VIDEO_DESC_IAD(_itf, /* 2 Interfaces */ 0x02, _stridx), \
  /* Video control 0 */ \
  TUD_VIDEO_DESC_STD_VC(_itf, 0, _stridx), \
    TUD_VIDEO_DESC_CS_VC( /* UVC 1.5*/ 0x0150, \
         /* wTotalLength - bLength */ \
         TUD_VIDEO_DESC_CAMERA_TERM_LEN + TUD_VIDEO_DESC_OUTPUT_TERM_LEN, \
         UVC_CLOCK_FREQUENCY, _itf + 1), \
      TUD_VIDEO_DESC_CAMERA_TERM(UVC_ENTITY_CAP_INPUT_TERMINAL, 0, 0,\
                                 /*wObjectiveFocalLengthMin*/0, /*wObjectiveFocalLengthMax*/0,\
                                 /*wObjectiveFocalLength*/0, /*bmControls*/0), \
      TUD_VIDEO_DESC_OUTPUT_TERM(UVC_ENTITY_CAP_OUTPUT_TERMINAL, VIDEO_TT_STREAMING, 0, 1, 0), \
  /* Video stream alt. 0 */ \
  TUD_VIDEO_DESC_STD_VS(_itf + 1, 0, 0, _stridx), \
    /* Video stream header for without still image capture */ \
    TUD_VIDEO_DESC_CS_VS_INPUT( /*bNumFormats*/1, \
        /*wTotalLength - bLength */\
        TUD_VIDEO_DESC_CS_VS_FMT_UNCOMPR_LEN\
        + TUD_VIDEO_DESC_CS_VS_FRM_UNCOMPR_CONT_LEN\
        + TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING_LEN,\
        _epin, /*bmInfo*/0, /*bTerminalLink*/UVC_ENTITY_CAP_OUTPUT_TERMINAL, \
        /*bStillCaptureMethod*/0, /*bTriggerSupport*/0, /*bTriggerUsage*/0, \
        /*bmaControls(1)*/0), \
      /* Video stream format */ \
      TUD_VIDEO_DESC_CS_VS_FMT_YUY2(/*bFormatIndex*/1, /*bNumFrameDescriptors*/1, \
        /*bDefaultFrameIndex*/1, 0, 0, 0, /*bCopyProtect*/0), \
        /* Video stream frame format */ \
        TUD_VIDEO_DESC_CS_VS_FRM_UNCOMPR_CONT(/*bFrameIndex */1, 0, _width, _height, \
            _width * _height * 16, _width * _height * 16 * _fps, \
            _width * _height * 16, \
            (10000000/_fps), (10000000/_fps), (10000000/_fps)*_fps, (10000000/_fps)), \
        TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING(VIDEO_COLOR_PRIMARIES_BT709, VIDEO_COLOR_XFER_CH_BT709, VIDEO_COLOR_COEF_SMPTE170M), \
  /* VS alt 1 */\
  TUD_VIDEO_DESC_STD_VS(_itf + 1, 1, 1, _stridx), \
    /* EP */ \
    TUD_VIDEO_DESC_EP_ISO(_epin, _epsize, 1)

#define TUD_VIDEO_CAPTURE_DESCRIPTOR_MJPEG(_stridx, _itf, _epin, _width, _height, _fps, _epsize) \
  TUD_VIDEO_DESC_IAD(_itf, /* 2 Interfaces */ 0x02, _stridx), \
  /* Video control 0 */ \
  TUD_VIDEO_DESC_STD_VC(_itf, 0, _stridx), \
    TUD_VIDEO_DESC_CS_VC( /* UVC 1.5*/ 0x0150, \
         /* wTotalLength - bLength */ \
         TUD_VIDEO_DESC_CAMERA_TERM_LEN + TUD_VIDEO_DESC_OUTPUT_TERM_LEN, \
         UVC_CLOCK_FREQUENCY, _itf + 1), \
      TUD_VIDEO_DESC_CAMERA_TERM(UVC_ENTITY_CAP_INPUT_TERMINAL, 0, 0,\
                                 /*wObjectiveFocalLengthMin*/0, /*wObjectiveFocalLengthMax*/0,\
                                 /*wObjectiveFocalLength*/0, /*bmControls*/0), \
      TUD_VIDEO_DESC_OUTPUT_TERM(UVC_ENTITY_CAP_OUTPUT_TERMINAL, VIDEO_TT_STREAMING, 0, 1, 0), \
  /* Video stream alt. 0 */ \
  TUD_VIDEO_DESC_STD_VS(_itf + 1, 0, 0, _stridx), \
    /* Video stream header for without still image capture */ \
    TUD_VIDEO_DESC_CS_VS_INPUT( /*bNumFormats*/1, \
        /*wTotalLength - bLength */\
        TUD_VIDEO_DESC_CS_VS_FMT_MJPEG_LEN\
        + TUD_VIDEO_DESC_CS_VS_FRM_MJPEG_CONT_LEN\
        + TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING_LEN,\
        _epin, /*bmInfo*/0, /*bTerminalLink*/UVC_ENTITY_CAP_OUTPUT_TERMINAL, \
        /*bStillCaptureMethod*/0, /*bTriggerSupport*/0, /*bTriggerUsage*/0, \
        /*bmaControls(1)*/0), \
      /* Video stream format */ \
      TUD_VIDEO_DESC_CS_VS_FMT_MJPEG(/*bFormatIndex*/1, /*bNumFrameDescriptors*/1, \
        /*bmFlags*/0, /*bDefaultFrameIndex*/1, 0, 0, 0, /*bCopyProtect*/0), \
        /* Video stream frame format */ \
        TUD_VIDEO_DESC_CS_VS_FRM_MJPEG_CONT(/*bFrameIndex */1, 0, _width, _height, \
            _width * _height * 16, _width * _height * 16 * _fps, \
            _width * _height * 16 / 8, \
            (10000000/_fps), (10000000/_fps), (10000000/_fps)*_fps, (10000000/_fps)), \
        TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING(VIDEO_COLOR_PRIMARIES_BT709, VIDEO_COLOR_XFER_CH_BT709, VIDEO_COLOR_COEF_SMPTE170M), \
  /* VS alt 1 */\
  TUD_VIDEO_DESC_STD_VS(_itf + 1, 1, 1, _stridx), \
    /* EP */ \
    TUD_VIDEO_DESC_EP_ISO(_epin, _epsize, 1)

#define TUD_VIDEO_CAPTURE_DESCRIPTOR_MULTI_MJPEG(_stridx, _itf, _epin, _epsize) \
  TUD_VIDEO_DESC_IAD(_itf, /* 2 Interfaces */ 0x02, _stridx), \
  /* Video control 0 */ \
  TUD_VIDEO_DESC_STD_VC(_itf, 0, _stridx), \
    TUD_VIDEO_DESC_CS_VC( /* UVC 1.5*/ 0x0150, \
         /* wTotalLength - bLength */ \
         TUD_VIDEO_DESC_CAMERA_TERM_LEN + TUD_VIDEO_DESC_OUTPUT_TERM_LEN, \
         UVC_CLOCK_FREQUENCY, _itf + 1), \
      TUD_VIDEO_DESC_CAMERA_TERM(UVC_ENTITY_CAP_INPUT_TERMINAL, 0, 0,\
                                 /*wObjectiveFocalLengthMin*/0, /*wObjectiveFocalLengthMax*/0,\
                                 /*wObjectiveFocalLength*/0, /*bmControls*/0), \
      TUD_VIDEO_DESC_OUTPUT_TERM(UVC_ENTITY_CAP_OUTPUT_TERMINAL, VIDEO_TT_STREAMING, 0, 1, 0), \
  /* Video stream alt. 0 */ \
  TUD_VIDEO_DESC_STD_VS(_itf + 1, 0, 0, _stridx), \
    /* Video stream header for without still image capture */ \
    TUD_VIDEO_DESC_CS_VS_INPUT( /*bNumFormats*/1, \
        /*wTotalLength - bLength */\
        TUD_VIDEO_DESC_CS_VS_FMT_MJPEG_LEN\
        + (UVC_FRAME_NUM*TUD_VIDEO_DESC_CS_VS_FRM_MJPEG_CONT_LEN)\
        + TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING_LEN,\
        _epin, /*bmInfo*/0, /*bTerminalLink*/UVC_ENTITY_CAP_OUTPUT_TERMINAL, \
        /*bStillCaptureMethod*/0, /*bTriggerSupport*/0, /*bTriggerUsage*/0, \
        /*bmaControls(1)*/0), \
      /* Video stream format */ \
      TUD_VIDEO_DESC_CS_VS_FMT_MJPEG(/*bFormatIndex*/1, /*bNumFrameDescriptors*/UVC_FRAME_NUM, \
        /*bmFlags*/0, /*bDefaultFrameIndex*/1, 0, 0, 0, /*bCopyProtect*/0), \
        /* Video stream frame format */ \
        TUD_VIDEO_DESC_CS_VS_FRM_MJPEG_CONT_TEMPLATE(_itf/2, 1), \
        TUD_VIDEO_DESC_CS_VS_FRM_MJPEG_CONT_TEMPLATE(_itf/2, 2), \
        TUD_VIDEO_DESC_CS_VS_FRM_MJPEG_CONT_TEMPLATE(_itf/2, 3), \
        TUD_VIDEO_DESC_CS_VS_FRM_MJPEG_CONT_TEMPLATE(_itf/2, 4), \
        TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING(VIDEO_COLOR_PRIMARIES_BT709, VIDEO_COLOR_XFER_CH_BT709, VIDEO_COLOR_COEF_SMPTE170M), \
  /* VS alt 1 */\
  TUD_VIDEO_DESC_STD_VS(_itf + 1, 1, 1, _stridx), \
    /* EP */ \
    TUD_VIDEO_DESC_EP_ISO(_epin, _epsize, 1)

#define TUD_VIDEO_CAPTURE_DESCRIPTOR_UNCOMPR_BULK(_stridx, _itf, _epin, _width, _height, _fps, _epsize) \
  TUD_VIDEO_DESC_IAD(_itf, /* 2 Interfaces */ 0x02, _stridx), \
  /* Video control 0 */ \
  TUD_VIDEO_DESC_STD_VC(_itf, 0, _stridx), \
    TUD_VIDEO_DESC_CS_VC( /* UVC 1.5*/ 0x0150, \
         /* wTotalLength - bLength */ \
         TUD_VIDEO_DESC_CAMERA_TERM_LEN + TUD_VIDEO_DESC_OUTPUT_TERM_LEN, \
         UVC_CLOCK_FREQUENCY, _itf + 1), \
      TUD_VIDEO_DESC_CAMERA_TERM(UVC_ENTITY_CAP_INPUT_TERMINAL, 0, 0,\
                                 /*wObjectiveFocalLengthMin*/0, /*wObjectiveFocalLengthMax*/0,\
                                 /*wObjectiveFocalLength*/0, /*bmControls*/0), \
      TUD_VIDEO_DESC_OUTPUT_TERM(UVC_ENTITY_CAP_OUTPUT_TERMINAL, VIDEO_TT_STREAMING, 0, 1, 0), \
  /* Video stream alt. 0 */ \
  TUD_VIDEO_DESC_STD_VS(_itf + 1, 0, 1, _stridx), \
    /* Video stream header for without still image capture */ \
    TUD_VIDEO_DESC_CS_VS_INPUT( /*bNumFormats*/1, \
        /*wTotalLength - bLength */\
        TUD_VIDEO_DESC_CS_VS_FMT_UNCOMPR_LEN\
        + TUD_VIDEO_DESC_CS_VS_FRM_UNCOMPR_CONT_LEN\
        + TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING_LEN,\
        _epin, /*bmInfo*/0, /*bTerminalLink*/UVC_ENTITY_CAP_OUTPUT_TERMINAL, \
        /*bStillCaptureMethod*/0, /*bTriggerSupport*/0, /*bTriggerUsage*/0, \
        /*bmaControls(1)*/0), \
      /* Video stream format */ \
      TUD_VIDEO_DESC_CS_VS_FMT_YUY2(/*bFormatIndex*/1, /*bNumFrameDescriptors*/1, \
        /*bDefaultFrameIndex*/1, 0, 0, 0, /*bCopyProtect*/0), \
        /* Video stream frame format */ \
        TUD_VIDEO_DESC_CS_VS_FRM_UNCOMPR_CONT(/*bFrameIndex */1, 0, _width, _height, \
            _width * _height * 16, _width * _height * 16 * _fps, \
            _width * _height * 16, \
            (10000000/_fps), (10000000/_fps), (10000000/_fps)*_fps, (10000000/_fps)), \
        TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING(VIDEO_COLOR_PRIMARIES_BT709, VIDEO_COLOR_XFER_CH_BT709, VIDEO_COLOR_COEF_SMPTE170M), \
        TUD_VIDEO_DESC_EP_BULK(_epin, _epsize, 1)

#define TUD_VIDEO_CAPTURE_DESCRIPTOR_MJPEG_BULK(_stridx, _itf, _epin, _width, _height, _fps, _epsize) \
  TUD_VIDEO_DESC_IAD(_itf, /* 2 Interfaces */ 0x02, _stridx), \
  /* Video control 0 */ \
  TUD_VIDEO_DESC_STD_VC(_itf, 0, _stridx), \
    TUD_VIDEO_DESC_CS_VC( /* UVC 1.5*/ 0x0150, \
         /* wTotalLength - bLength */ \
         TUD_VIDEO_DESC_CAMERA_TERM_LEN + TUD_VIDEO_DESC_OUTPUT_TERM_LEN, \
         UVC_CLOCK_FREQUENCY, _itf + 1), \
      TUD_VIDEO_DESC_CAMERA_TERM(UVC_ENTITY_CAP_INPUT_TERMINAL, 0, 0,\
                                 /*wObjectiveFocalLengthMin*/0, /*wObjectiveFocalLengthMax*/0,\
                                 /*wObjectiveFocalLength*/0, /*bmControls*/0), \
      TUD_VIDEO_DESC_OUTPUT_TERM(UVC_ENTITY_CAP_OUTPUT_TERMINAL, VIDEO_TT_STREAMING, 0, 1, 0), \
  /* Video stream alt. 0 */ \
  TUD_VIDEO_DESC_STD_VS(_itf + 1, 0, 1, _stridx), \
    /* Video stream header for without still image capture */ \
    TUD_VIDEO_DESC_CS_VS_INPUT( /*bNumFormats*/1, \
        /*wTotalLength - bLength */\
        TUD_VIDEO_DESC_CS_VS_FMT_MJPEG_LEN\
        + TUD_VIDEO_DESC_CS_VS_FRM_MJPEG_CONT_LEN\
        + TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING_LEN,\
        _epin, /*bmInfo*/0, /*bTerminalLink*/UVC_ENTITY_CAP_OUTPUT_TERMINAL, \
        /*bStillCaptureMethod*/0, /*bTriggerSupport*/0, /*bTriggerUsage*/0, \
        /*bmaControls(1)*/0), \
      /* Video stream format */ \
      TUD_VIDEO_DESC_CS_VS_FMT_MJPEG(/*bFormatIndex*/1, /*bNumFrameDescriptors*/1, \
        /*bmFlags*/0, /*bDefaultFrameIndex*/1, 0, 0, 0, /*bCopyProtect*/0), \
        /* Video stream frame format */ \
        TUD_VIDEO_DESC_CS_VS_FRM_MJPEG_CONT(/*bFrameIndex */1, 0, _width, _height, \
            _width * _height * 16, _width * _height * 16 * _fps, \
            _width * _height * 16 / 8, \
            (10000000/_fps), (10000000/_fps), (10000000/_fps), (10000000/_fps)), \
        TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING(VIDEO_COLOR_PRIMARIES_BT709, VIDEO_COLOR_XFER_CH_BT709, VIDEO_COLOR_COEF_SMPTE170M), \
        /* EP */ \
        TUD_VIDEO_DESC_EP_BULK(_epin, _epsize, 1)

#define TUD_VIDEO_CAPTURE_DESCRIPTOR_MULTI_MJPEG_BULK(_stridx, _itf, _epin, _epsize) \
  TUD_VIDEO_DESC_IAD(_itf, /* 2 Interfaces */ 0x02, _stridx), \
  /* Video control 0 */ \
  TUD_VIDEO_DESC_STD_VC(_itf, 0, _stridx), \
    TUD_VIDEO_DESC_CS_VC( /* UVC 1.5*/ 0x0150, \
         /* wTotalLength - bLength */ \
         TUD_VIDEO_DESC_CAMERA_TERM_LEN + TUD_VIDEO_DESC_OUTPUT_TERM_LEN, \
         UVC_CLOCK_FREQUENCY, _itf + 1), \
      TUD_VIDEO_DESC_CAMERA_TERM(UVC_ENTITY_CAP_INPUT_TERMINAL, 0, 0,\
                                 /*wObjectiveFocalLengthMin*/0, /*wObjectiveFocalLengthMax*/0,\
                                 /*wObjectiveFocalLength*/0, /*bmControls*/0), \
      TUD_VIDEO_DESC_OUTPUT_TERM(UVC_ENTITY_CAP_OUTPUT_TERMINAL, VIDEO_TT_STREAMING, 0, 1, 0), \
  /* Video stream alt. 0 */ \
  TUD_VIDEO_DESC_STD_VS(_itf + 1, 0, 1, _stridx), \
    /* Video stream header for without still image capture */ \
    TUD_VIDEO_DESC_CS_VS_INPUT( /*bNumFormats*/1, \
        /*wTotalLength - bLength */\
        TUD_VIDEO_DESC_CS_VS_FMT_MJPEG_LEN\
        + (UVC_FRAME_NUM*TUD_VIDEO_DESC_CS_VS_FRM_MJPEG_CONT_LEN)\
        + TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING_LEN,\
        _epin, /*bmInfo*/0, /*bTerminalLink*/UVC_ENTITY_CAP_OUTPUT_TERMINAL, \
        /*bStillCaptureMethod*/0, /*bTriggerSupport*/0, /*bTriggerUsage*/0, \
        /*bmaControls(1)*/0), \
      /* Video stream format */ \
      TUD_VIDEO_DESC_CS_VS_FMT_MJPEG(/*bFormatIndex*/1, /*bNumFrameDescriptors*/UVC_FRAME_NUM, \
        /*bmFlags*/0, /*bDefaultFrameIndex*/1, 0, 0, 0, /*bCopyProtect*/0), \
        /* Video stream frame format */ \
        TUD_VIDEO_DESC_CS_VS_FRM_MJPEG_CONT_TEMPLATE(_itf/2, 1), \
        TUD_VIDEO_DESC_CS_VS_FRM_MJPEG_CONT_TEMPLATE(_itf/2, 2), \
        TUD_VIDEO_DESC_CS_VS_FRM_MJPEG_CONT_TEMPLATE(_itf/2, 3), \
        TUD_VIDEO_DESC_CS_VS_FRM_MJPEG_CONT_TEMPLATE(_itf/2, 4), \
        TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING(VIDEO_COLOR_PRIMARIES_BT709, VIDEO_COLOR_XFER_CH_BT709, VIDEO_COLOR_COEF_SMPTE170M), \
        /* EP */ \
        TUD_VIDEO_DESC_EP_BULK(_epin, _epsize, 1)

#define TUD_VIDEO_CAPTURE_DESCRIPTOR_H264(_stridx, _itf, _epin, _width, _height, _fps, _epsize) \
  TUD_VIDEO_DESC_IAD(_itf, /* 2 Interfaces */ 0x02, _stridx), \
  /* Video control 0 */ \
  TUD_VIDEO_DESC_STD_VC(_itf, 0, _stridx), \
    TUD_VIDEO_DESC_CS_VC( /* UVC 1.5*/ 0x0150, \
         /* wTotalLength - bLength */ \
         TUD_VIDEO_DESC_CAMERA_TERM_LEN + TUD_VIDEO_DESC_OUTPUT_TERM_LEN, \
         UVC_CLOCK_FREQUENCY, _itf + 1), \
      TUD_VIDEO_DESC_CAMERA_TERM(UVC_ENTITY_CAP_INPUT_TERMINAL, 0, 0,\
                                 /*wObjectiveFocalLengthMin*/0, /*wObjectiveFocalLengthMax*/0,\
                                 /*wObjectiveFocalLength*/0, /*bmControls*/0), \
      TUD_VIDEO_DESC_OUTPUT_TERM(UVC_ENTITY_CAP_OUTPUT_TERMINAL, VIDEO_TT_STREAMING, 0, 1, 0), \
  /* Video stream alt. 0 */ \
  TUD_VIDEO_DESC_STD_VS(_itf + 1, 0, 0, _stridx), \
    /* Video stream header for without still image capture */ \
    TUD_VIDEO_DESC_CS_VS_INPUT( /*bNumFormats*/1, \
        /*wTotalLength - bLength */\
        TUD_VIDEO_DESC_CS_VS_FMT_FRAME_BASED_LEN\
        + TUD_VIDEO_DESC_CS_VS_FRM_FRAME_BASED_CONT_LEN\
        + TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING_LEN,\
        _epin, /*bmInfo*/0, /*bTerminalLink*/UVC_ENTITY_CAP_OUTPUT_TERMINAL, \
        /*bStillCaptureMethod*/0, /*bTriggerSupport*/0, /*bTriggerUsage*/0, \
        /*bmaControls(1)*/0), \
      /* Video stream format */ \
      TUD_VIDEO_DESC_CS_VS_FMT_FRAME_BASED(/*bFormatIndex*/1, /*bNumFrameDescriptors*/1, \
        /*guid*/TUD_VIDEO_GUID_H264, /*bitsPerPix*/16, /*bDefaultFrameIndex*/1, 0, 0, 0, /*bCopyProtect*/0, /*variableSize*/1), \
        /* Video stream frame format */ \
        TUD_VIDEO_DESC_CS_VS_FRM_FRAME_BASED_CONT(/*bFrameIndex */1, 0, _width, _height, \
            _width * _height * 16, _width * _height * 16 * _fps, \
            (10000000/_fps), /*bytesPreLine*/ 0, \
            (10000000/_fps), (10000000/_fps)*_fps, (10000000/_fps)), \
        TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING(VIDEO_COLOR_PRIMARIES_BT709, VIDEO_COLOR_XFER_CH_BT709, VIDEO_COLOR_COEF_SMPTE170M), \
  /* VS alt 1 */\
  TUD_VIDEO_DESC_STD_VS(_itf + 1, 1, 1, _stridx), \
    /* EP */ \
    TUD_VIDEO_DESC_EP_ISO(_epin, _epsize, 1)

#define TUD_VIDEO_CAPTURE_DESCRIPTOR_H264_BULK(_stridx, _itf, _epin, _width, _height, _fps, _epsize) \
  TUD_VIDEO_DESC_IAD(_itf, /* 2 Interfaces */ 0x02, _stridx), \
  /* Video control 0 */ \
  TUD_VIDEO_DESC_STD_VC(_itf, 0, _stridx), \
    TUD_VIDEO_DESC_CS_VC( /* UVC 1.5*/ 0x0150, \
         /* wTotalLength - bLength */ \
         TUD_VIDEO_DESC_CAMERA_TERM_LEN + TUD_VIDEO_DESC_OUTPUT_TERM_LEN, \
         UVC_CLOCK_FREQUENCY, _itf + 1), \
      TUD_VIDEO_DESC_CAMERA_TERM(UVC_ENTITY_CAP_INPUT_TERMINAL, 0, 0,\
                                 /*wObjectiveFocalLengthMin*/0, /*wObjectiveFocalLengthMax*/0,\
                                 /*wObjectiveFocalLength*/0, /*bmControls*/0), \
      TUD_VIDEO_DESC_OUTPUT_TERM(UVC_ENTITY_CAP_OUTPUT_TERMINAL, VIDEO_TT_STREAMING, 0, 1, 0), \
  /* Video stream alt. 0 */ \
  TUD_VIDEO_DESC_STD_VS(_itf + 1, 0, 1, _stridx), \
    /* Video stream header for without still image capture */ \
    TUD_VIDEO_DESC_CS_VS_INPUT( /*bNumFormats*/1, \
        /*wTotalLength - bLength */\
        TUD_VIDEO_DESC_CS_VS_FMT_FRAME_BASED_LEN\
        + TUD_VIDEO_DESC_CS_VS_FRM_FRAME_BASED_CONT_LEN\
        + TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING_LEN,\
        _epin, /*bmInfo*/0, /*bTerminalLink*/UVC_ENTITY_CAP_OUTPUT_TERMINAL, \
        /*bStillCaptureMethod*/0, /*bTriggerSupport*/0, /*bTriggerUsage*/0, \
        /*bmaControls(1)*/0), \
      /* Video stream format */ \
      TUD_VIDEO_DESC_CS_VS_FMT_FRAME_BASED(/*bFormatIndex*/1, /*bNumFrameDescriptors*/1, \
        /*guid*/TUD_VIDEO_GUID_H264, /*bitsPerPix*/16, /*bDefaultFrameIndex*/1, 0, 0, 0, /*bCopyProtect*/0, /*variableSize*/1), \
        /* Video stream frame format */ \
        TUD_VIDEO_DESC_CS_VS_FRM_FRAME_BASED_CONT(/*bFrameIndex */1, 0, _width, _height, \
            _width * _height * 16, _width * _height * 16 * _fps, \
            (10000000/_fps), /*bytesPreLine*/ 0, \
            (10000000/_fps), (10000000/_fps), (10000000/_fps)), \
        TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING(VIDEO_COLOR_PRIMARIES_BT709, VIDEO_COLOR_XFER_CH_BT709, VIDEO_COLOR_COEF_SMPTE170M), \
    /* EP */ \
    TUD_VIDEO_DESC_EP_BULK(_epin, _epsize, 1)

#define TUD_VIDEO_CAPTURE_DESCRIPTOR_MULTI_H264(_stridx, _itf, _epin, _epsize) \
  TUD_VIDEO_DESC_IAD(_itf, /* 2 Interfaces */ 0x02, _stridx), \
  /* Video control 0 */ \
  TUD_VIDEO_DESC_STD_VC(_itf, 0, _stridx), \
    TUD_VIDEO_DESC_CS_VC( /* UVC 1.5*/ 0x0150, \
         /* wTotalLength - bLength */ \
         TUD_VIDEO_DESC_CAMERA_TERM_LEN + TUD_VIDEO_DESC_OUTPUT_TERM_LEN, \
         UVC_CLOCK_FREQUENCY, _itf + 1), \
      TUD_VIDEO_DESC_CAMERA_TERM(UVC_ENTITY_CAP_INPUT_TERMINAL, 0, 0,\
                                 /*wObjectiveFocalLengthMin*/0, /*wObjectiveFocalLengthMax*/0,\
                                 /*wObjectiveFocalLength*/0, /*bmControls*/0), \
      TUD_VIDEO_DESC_OUTPUT_TERM(UVC_ENTITY_CAP_OUTPUT_TERMINAL, VIDEO_TT_STREAMING, 0, 1, 0), \
  /* Video stream alt. 0 */ \
  TUD_VIDEO_DESC_STD_VS(_itf + 1, 0, 0, _stridx), \
    /* Video stream header for without still image capture */ \
    TUD_VIDEO_DESC_CS_VS_INPUT( /*bNumFormats*/1, \
        /*wTotalLength - bLength */\
        TUD_VIDEO_DESC_CS_VS_FMT_FRAME_BASED_LEN\
        + (UVC_FRAME_NUM*TUD_VIDEO_DESC_CS_VS_FRM_FRAME_BASED_CONT_LEN)\
        + TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING_LEN,\
        _epin, /*bmInfo*/0, /*bTerminalLink*/UVC_ENTITY_CAP_OUTPUT_TERMINAL, \
        /*bStillCaptureMethod*/0, /*bTriggerSupport*/0, /*bTriggerUsage*/0, \
        /*bmaControls(1)*/0), \
      /* Video stream format */ \
      TUD_VIDEO_DESC_CS_VS_FMT_FRAME_BASED(/*bFormatIndex*/1, /*bNumFrameDescriptors*/UVC_FRAME_NUM, \
        /*guid*/TUD_VIDEO_GUID_H264, /*bitsPerPix*/16, /*bDefaultFrameIndex*/1, 0, 0, 0, /*bCopyProtect*/0, /*variableSize*/1), \
        /* Video stream frame format */ \
        TUD_VIDEO_DESC_CS_VS_FRM_H264_CONT_TEMPLATE(_itf/2, 1), \
        TUD_VIDEO_DESC_CS_VS_FRM_H264_CONT_TEMPLATE(_itf/2, 2), \
        TUD_VIDEO_DESC_CS_VS_FRM_H264_CONT_TEMPLATE(_itf/2, 3), \
        TUD_VIDEO_DESC_CS_VS_FRM_H264_CONT_TEMPLATE(_itf/2, 4), \
        TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING(VIDEO_COLOR_PRIMARIES_BT709, VIDEO_COLOR_XFER_CH_BT709, VIDEO_COLOR_COEF_SMPTE170M), \
  /* VS alt 1 */\
  TUD_VIDEO_DESC_STD_VS(_itf + 1, 1, 1, _stridx), \
    /* EP */ \
    TUD_VIDEO_DESC_EP_ISO(_epin, _epsize, 1)

#define TUD_VIDEO_CAPTURE_DESCRIPTOR_MULTI_H264_BULK(_stridx, _itf, _epin, _epsize) \
  TUD_VIDEO_DESC_IAD(_itf, /* 2 Interfaces */ 0x02, _stridx), \
  /* Video control 0 */ \
  TUD_VIDEO_DESC_STD_VC(_itf, 0, _stridx), \
    TUD_VIDEO_DESC_CS_VC( /* UVC 1.5*/ 0x0150, \
         /* wTotalLength - bLength */ \
         TUD_VIDEO_DESC_CAMERA_TERM_LEN + TUD_VIDEO_DESC_OUTPUT_TERM_LEN, \
         UVC_CLOCK_FREQUENCY, _itf + 1), \
      TUD_VIDEO_DESC_CAMERA_TERM(UVC_ENTITY_CAP_INPUT_TERMINAL, 0, 0,\
                                 /*wObjectiveFocalLengthMin*/0, /*wObjectiveFocalLengthMax*/0,\
                                 /*wObjectiveFocalLength*/0, /*bmControls*/0), \
      TUD_VIDEO_DESC_OUTPUT_TERM(UVC_ENTITY_CAP_OUTPUT_TERMINAL, VIDEO_TT_STREAMING, 0, 1, 0), \
  /* Video stream alt. 0 */ \
  TUD_VIDEO_DESC_STD_VS(_itf + 1, 0, 1, _stridx), \
    /* Video stream header for without still image capture */ \
    TUD_VIDEO_DESC_CS_VS_INPUT( /*bNumFormats*/1, \
        /*wTotalLength - bLength */\
        TUD_VIDEO_DESC_CS_VS_FMT_FRAME_BASED_LEN\
        + (UVC_FRAME_NUM*TUD_VIDEO_DESC_CS_VS_FRM_FRAME_BASED_CONT_LEN)\
        + TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING_LEN,\
        _epin, /*bmInfo*/0, /*bTerminalLink*/UVC_ENTITY_CAP_OUTPUT_TERMINAL, \
        /*bStillCaptureMethod*/0, /*bTriggerSupport*/0, /*bTriggerUsage*/0, \
        /*bmaControls(1)*/0), \
      /* Video stream format */ \
      TUD_VIDEO_DESC_CS_VS_FMT_FRAME_BASED(/*bFormatIndex*/1, /*bNumFrameDescriptors*/UVC_FRAME_NUM, \
        /*guid*/TUD_VIDEO_GUID_H264, /*bitsPerPix*/16, /*bDefaultFrameIndex*/1, 0, 0, 0, /*bCopyProtect*/0, /*variableSize*/1), \
        /* Video stream frame format */ \
        TUD_VIDEO_DESC_CS_VS_FRM_H264_CONT_TEMPLATE(_itf/2, 1), \
        TUD_VIDEO_DESC_CS_VS_FRM_H264_CONT_TEMPLATE(_itf/2, 2), \
        TUD_VIDEO_DESC_CS_VS_FRM_H264_CONT_TEMPLATE(_itf/2, 3), \
        TUD_VIDEO_DESC_CS_VS_FRM_H264_CONT_TEMPLATE(_itf/2, 4), \
        TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING(VIDEO_COLOR_PRIMARIES_BT709, VIDEO_COLOR_XFER_CH_BT709, VIDEO_COLOR_COEF_SMPTE170M), \
        /* EP */ \
        TUD_VIDEO_DESC_EP_BULK(_epin, _epsize, 1)

#endif //CFG_TUD_VIDEO
#endif
