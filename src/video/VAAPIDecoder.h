//
//  libavg - Media Playback Engine.
//  Copyright (C) 2013 Pararth Shah
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//  Current versions can be found at github.com/pararthshah/libavg-vaapi
//
#ifndef VAAPIDECODER_H_
#define VAAPIDECODER_H_


#include "../avgconfigwrapper.h"
#include "../base/GLMHelper.h"

#include "WrapFFMpeg.h"

#include <va/va.h>
#include <libavcodec/vaapi.h>

namespace avg {

class VAAPIDecoder; // Forward declaration

class VAAPISurface
{
public:
    VASurfaceID  m_SurfaceID;
    int          m_RefCount;
    unsigned int m_Order;

    VAAPIDecoder* m_pDecoder;

};

class VAAPIDecoder
{
public:
	VAAPIDecoder();
    ~VAAPIDecoder();
    AVCodec* openCodec(AVCodecContext* pCodec);

    static bool isAvailable();

    VAContextID getContext();
    VAImage getImage();
    IntPoint getSize();

private:
    // Callbacks
    static int getBuffer(AVCodecContext* pContext, AVFrame* pFrame);
    static void releaseBuffer(struct AVCodecContext* pContext, AVFrame* pFrame);
    static void drawHorizBand(AVCodecContext* pContext, const AVFrame* pFrame,
            int offset[4], int y, int type, int height);
    static AVPixelFormat getFormat(AVCodecContext* pContext, const AVPixelFormat* pFmt);

    VAAPISurface* getFreeVaapiSurface();
    int getBufferInternal(AVCodecContext* pContext, AVFrame* pFrame);
    void releaseBufferInternal(struct AVCodecContext* pContext, AVFrame* pFrame);
    void render(AVCodecContext* pContext, const AVFrame* pFrame);
    void setupDecoder(AVCodecContext* pContext);
    void createSurfaces(AVCodecContext* pContext);
    void destroySurfaces();

    VAProfile m_Profile;

    VAConfigID    m_ConfigID;
    VAContextID   m_ContextID;

    struct vaapi_context m_HWCtx;

    int          m_SurfaceCount;
    unsigned int m_SurfaceOrder;
    IntPoint     m_Size;
    //vlc_fourcc_t m_SurfaceChroma;

    VAAPISurface *m_pVAAPISurface;

    VAImage      m_Image;
    //copy_cache_t m_image_cache;

    AVPixelFormat  m_PixFmt;

};

}
#endif /* VAAPIDECODER_H_ */
