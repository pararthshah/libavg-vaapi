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
#include "VAAPIDecoder.h"
#include "VAAPIHelper.h"

#include "../base/Exception.h"
#include "../base/Logger.h"

#include <iostream>

using namespace std;

namespace avg {

VAAPIDecoder::VAAPIDecoder()
    : m_SurfaceOrder(0),
      m_Size(-1,-1),
      m_pVAAPISurface(NULL),
      m_PixFmt(PIX_FMT_NONE)
{
	m_HWCtx = NULL;
}

VAAPIDecoder::~VAAPIDecoder()
{
	if(m_Size.x || m_Size.y)
        destroySurfaces();

    if(m_ConfigID != VA_INVALID_ID)
        vaDestroyConfig(getVAAPIDisplay(), m_ConfigID);

    if(getVAAPIDisplay())
        vaTerminate(getVAAPIDisplay());

    //if(x11_display)
    //    XCloseDisplay(display);
}

AVCodec* VAAPIDecoder::openCodec(AVCodecContext* pContext)
{
    if (!isAvailable()) {
        return 0;
    }

    m_ConfigID  = VA_INVALID_ID;
    m_ContextID = VA_INVALID_ID;
    m_Image.image_id = VA_INVALID_ID;
    m_PixFmt = PIX_FMT_VAAPI_VLD; // Only VLD supported

    VAProfile *pProfilesList;
    bool bSupportedProfile = false;
    int numProfiles = 0;

    AVCodec* pCodec = 0;
    switch (pContext->codec_id) {
        case CODEC_ID_MPEG1VIDEO:
        case CODEC_ID_MPEG2VIDEO:
        	m_Profile = VAProfileMPEG2Main;
        	m_SurfaceCount = 2+1;
            //pCodec = avcodec_find_decoder_by_name("mpeg2_vaapi");
            pContext->codec_id = CODEC_ID_MPEG2VIDEO;
            break;
        case CODEC_ID_MPEG4:
        	m_Profile = VAProfileMPEG4AdvancedSimple;
        	m_SurfaceCount = 2+1;
            //pCodec = avcodec_find_decoder_by_name("mpeg4_vaapi");
            break;
        case CODEC_ID_H263:
        	m_Profile = VAProfileH264Baseline;
        	m_SurfaceCount = 16+1;
            //pCodec = avcodec_find_decoder_by_name("h264_vaapi");
            break;
        case CODEC_ID_H264:
        	m_Profile = VAProfileH264High;
        	m_SurfaceCount = 16+1;
            //pCodec = avcodec_find_decoder_by_name("h264_vaapi");
            break;
        case CODEC_ID_WMV3:
        	m_Profile = VAProfileVC1Main;
        	m_SurfaceCount = 2+1;
            //pCodec = avcodec_find_decoder_by_name("wmv3_vaapi");
            break;
        case CODEC_ID_VC1:
        	m_Profile = VAProfileVC1Advanced;
        	m_SurfaceCount = 2+1;
            //pCodec = avcodec_find_decoder_by_name("vc1_vaapi");
            break;
        default:
            pCodec = 0;
    }
    //if (pCodec) {
	m_SurfaceCount = 300;
        pContext->get_buffer = VAAPIDecoder::getBuffer;
        pContext->release_buffer = VAAPIDecoder::releaseBuffer;
        pContext->draw_horiz_band = NULL;
        pContext->get_format = VAAPIDecoder::getFormat;
        pContext->slice_flags = SLICE_FLAG_CODED_ORDER | SLICE_FLAG_ALLOW_FIELD;
	//pContext->bits_per_coded_sample = 8; //hardcoded
	pContext->pix_fmt = m_PixFmt;
    //}

    numProfiles = vaMaxNumProfiles(getVAAPIDisplay());
    AVG_TRACE(Logger::category::PLAYER, Logger::severity::INFO, "maxNumProfiles: " << numProfiles);
    pProfilesList = new VAProfile[numProfiles];
    if(!pProfilesList)
    	return 0;

    VAStatus status = vaQueryConfigProfiles(getVAAPIDisplay(),
    		pProfilesList, &numProfiles);
    if (status == VA_STATUS_SUCCESS) {
    	for( int i = 0; i < numProfiles; i++ ) {
		AVG_TRACE(Logger::category::PLAYER, Logger::severity::INFO, "profile[" << i << "]: " << (unsigned int) pProfilesList[i]);
    		if (pProfilesList[i] == m_Profile) {
    			bSupportedProfile = true;
    			break;
    		}
    	}
    } else {
	AVG_ASSERT_MSG(false,
    		       "vaQueryConfigProfiles return error");
    	return 0;
    }
    delete[] pProfilesList;
    if (!bSupportedProfile) {
	AVG_TRACE(Logger::category::PLAYER, Logger::severity::INFO, "profile: " << (unsigned int) m_Profile);
    	AVG_ASSERT_MSG(false,
    			"Codec and profile not supported by the hardware");
    	return 0;
    }

    if (m_pVAAPISurface == NULL) setupDecoder(pContext);

    return pCodec;
}

bool VAAPIDecoder::isAvailable()
{
#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(53, 34, 0)
    return getVAAPIDisplay() != 0;
#else
    return false;
#endif
}

VAContextID VAAPIDecoder::getContext()
{
	return m_ContextID;
}

VAImage VAAPIDecoder::getImage()
{
	return m_Image;
}

IntPoint VAAPIDecoder::getSize()
{
	return m_Size;
}

int VAAPIDecoder::getBuffer(AVCodecContext* pContext, AVFrame* pFrame)
{
    VAAPIDecoder* pVAAPIDecoder = (VAAPIDecoder*)pContext->opaque;
    return pVAAPIDecoder->getBufferInternal(pContext, pFrame);
}

void VAAPIDecoder::releaseBuffer(struct AVCodecContext* pContext, AVFrame* pFrame)
{
	//VAAPIDecoder* pVAAPIDecoder = (VAAPIDecoder*)pContext->opaque;
	//pVAAPIDecoder->releaseBufferInternal(pContext, pFrame);
	pFrame->data[0] = NULL;
	pFrame->data[3] = NULL;
	pFrame->opaque = NULL;
	//AVG_TRACE(Logger::category::PLAYER, Logger::severity::INFO, "releaseBuffer: " << i_surface->m_SurfaceID);
}

// main rendering routine
void VAAPIDecoder::drawHorizBand(struct AVCodecContext* pContext,
		const AVFrame* src, int offset[4], int y, int type, int height)
{
    VAAPIDecoder* pVAAPIDecoder = (VAAPIDecoder*)pContext->opaque;
    pVAAPIDecoder->render(pContext, src);
}

AVPixelFormat VAAPIDecoder::getFormat(AVCodecContext* pContext,
		const AVPixelFormat* pFmt)
{
    AVG_TRACE(Logger::category::PLAYER, Logger::severity::INFO, "getFormat.");
    for (int i = 0; pFmt[i] != PIX_FMT_NONE; i++) {
	AVG_TRACE(Logger::category::PLAYER, Logger::severity::INFO, "getFormat: " << pFmt[i]);
	if (pFmt[i] == PIX_FMT_VAAPI_VLD)
	    return pFmt[i];
    }
    AVG_ASSERT(false);
    return PIX_FMT_VAAPI_VLD;
    switch (pContext->codec_id) {
        case CODEC_ID_H264:
        case CODEC_ID_MPEG1VIDEO:
        case CODEC_ID_MPEG2VIDEO:
        case CODEC_ID_VC1:
            return PIX_FMT_VAAPI_VLD;
        default:
            return pFmt[0];
    }
}

VAAPISurface* VAAPIDecoder::getFreeVaapiSurface()
{
	//AVG_TRACE(Logger::category::PLAYER, Logger::severity::INFO, "getFreeVaapiSurface.");
	/* Grab an unused surface, in case none are, try the oldest
	 * XXX using the oldest is a workaround in case a problem happens with ffmpeg */
	int i = 0, i_old = 0;
	for(; i < m_SurfaceCount; i++) {
		VAAPISurface *p_surface = &m_pVAAPISurface[i];
		if(!p_surface->m_RefCount)
			break;

		if(p_surface->m_Order < m_pVAAPISurface[i_old].m_Order)
			i_old = i;
	}
	if( i >= m_SurfaceCount){
		AVG_TRACE(Logger::category::PLAYER, Logger::severity::INFO, "i >= surfaceCount");
		i = i_old;
	}

    return &m_pVAAPISurface[i];
}

int VAAPIDecoder::getBufferInternal(AVCodecContext* pContext, AVFrame* pFrame)
{
	VAAPISurface* pVaapiSurface = getFreeVaapiSurface();

	AVG_TRACE(Logger::category::PLAYER, Logger::severity::INFO, "getBufferInternal wait: " << pVaapiSurface->m_SurfaceID);

#if VA_CHECK_VERSION(0,31,0)
	if(vaSyncSurface(getVAAPIDisplay(), pVaapiSurface->m_SurfaceID)) {
#else
	if(vaSyncSurface(getVAAPIDisplay(), m_ContextID,
			pVaapiSurface->m_SurfaceID)) {
#endif
		AVG_ASSERT(false);
	}
	AVG_TRACE(Logger::category::PLAYER, Logger::severity::INFO, "getBufferInternal: " << pVaapiSurface->m_SurfaceID);

	VASurfaceStatus s_status;
	if(vaQuerySurfaceStatus(getVAAPIDisplay(), pVaapiSurface->m_SurfaceID, &s_status)) {
		AVG_ASSERT(false);
	}
	AVG_TRACE(Logger::category::PLAYER, Logger::severity::INFO, "getBufferInternal s_status: " << s_status);

	pVaapiSurface->m_RefCount++;
	pVaapiSurface->m_Order = m_SurfaceOrder++;

	for(int i = 0; i < 4; i++) {
		pFrame->data[i] = NULL;
		pFrame->linesize[i] = 0;

		if(i == 0 || i == 3)
			pFrame->data[i] = (uint8_t*)m_HWCtx;
	}

	pFrame->opaque = (void*)pVaapiSurface;
	pFrame->type = FF_BUFFER_TYPE_USER;
	//pFrame->age = 1;
	//AVG_TRACE(Logger::category::PLAYER, Logger::severity::INFO, "getBufferInternal finish.");
	return 0;
}

void VAAPIDecoder::unlockSurface(VAAPISurface* pVaapiSurface)
{
    VASurfaceID surface_id = pVaapiSurface->m_SurfaceID;
    AVG_TRACE(Logger::category::PLAYER, Logger::severity::INFO, "unlockSurface: " << surface_id);

    for(int i = 0; i < m_SurfaceCount; i++ ) {
	VAAPISurface *p_surface = &m_pVAAPISurface[i];

	if(p_surface->m_SurfaceID == surface_id){
		if (p_surface->m_RefCount > 0)
			p_surface->m_RefCount--;
	}
    }	
}

void VAAPIDecoder::render(AVCodecContext* pContext, const AVFrame* pFrame)
{

}

void VAAPIDecoder::setupDecoder(AVCodecContext* pContext)
{
    AVG_TRACE(Logger::category::PLAYER, Logger::severity::INFO, "setupDecoder.");
    if(m_Size.x == pContext->width && m_Size.y == pContext->height) {
	pContext->hwaccel_context = m_HWCtx;
	//*pi_chroma = p_va->i_surface_chroma;
	return;
    }

    // Create a VA configuration
    VAConfigAttrib attrib;
    memset(&attrib, 0, sizeof(attrib));
    attrib.type = VAConfigAttribRTFormat;
    if(vaGetConfigAttributes(getVAAPIDisplay(), m_Profile, VAEntrypointVLD,
    		&attrib, 1)) {
    	AVG_ASSERT_MSG(false, "vaGetConfigAttributes returned error");
    	return;
    }

    /* Not sure what to do if not, I don't have a way to test */
    if((attrib.value & VA_RT_FORMAT_YUV420) == 0) {
    	AVG_ASSERT_MSG(false, "Attribute format not supported");
    	return;
    }

    if(vaCreateConfig(getVAAPIDisplay(), m_Profile, VAEntrypointVLD, &attrib, 1,
    		&m_ConfigID)) {
    	m_ConfigID = VA_INVALID_ID;
    	AVG_ASSERT_MSG(false, "vaCreateConfig returned error");
    	return;
    }

    pContext->hwaccel_context = NULL;
    //*pi_chroma = 0;
    /*if( m_Size.x || m_Size.y)
    	destroySurfaces();*/

    //m_Size = IntPoint(pContext->width, pContext->height);

    if(pContext->width > 0 && pContext->height > 0)
    	createSurfaces(pContext);
    else {
    	AVG_ASSERT_MSG(false, "Empty width and height");
    	return;
    }

    AVG_TRACE(Logger::category::PLAYER, Logger::severity::INFO, "setupDecoder finish.");
}

void VAAPIDecoder::createSurfaces(AVCodecContext* pContext)
{
    AVG_TRACE(Logger::category::PLAYER, Logger::severity::INFO, "createSurfaces.");
	AVG_ASSERT(pContext->width > 0 && pContext->height > 0);

	m_Size = IntPoint(pContext->width, pContext->height);
	m_pVAAPISurface = new VAAPISurface[m_SurfaceCount];
	if(!m_pVAAPISurface) {
		AVG_ASSERT_MSG(false, "Cannot allocate mem for surfaces");
		return;
	}

	m_Image.image_id = VA_INVALID_ID;
	m_ContextID      = VA_INVALID_ID;

	// Create surfaces
	VASurfaceID surface_id[m_SurfaceCount];
	if(vaCreateSurfaces(getVAAPIDisplay(), m_Size.x, m_Size.y,
			VA_RT_FORMAT_YUV420, m_SurfaceCount, surface_id)) {
		for( int i = 0; i < m_SurfaceCount; i++ )
			m_pVAAPISurface[i].m_SurfaceID = VA_INVALID_SURFACE;
		destroySurfaces();
		AVG_ASSERT_MSG(false, "vaCreateSurfaces returned error");
		return;
	}

	for(int i = 0; i < m_SurfaceCount; i++) {
		VAAPISurface *p_surface = &m_pVAAPISurface[i];

		p_surface->m_SurfaceID = surface_id[i];
		p_surface->m_RefCount = 0;
		p_surface->m_Order = 0;
		p_surface->m_Image.image_id = VA_INVALID_ID;
		p_surface->m_pDecoder = this;
	}

	// Create a context
	if(vaCreateContext(getVAAPIDisplay(), m_ConfigID, m_Size.x, m_Size.y,
			VA_PROGRESSIVE, surface_id, m_SurfaceCount, &m_ContextID)) {
		m_ContextID = VA_INVALID_ID;
		destroySurfaces();
		AVG_ASSERT_MSG(false, "vaCreateContext returned error");
		return;
	}

	// Find and create a supported image chroma
	int fmtCount = vaMaxNumImageFormats(getVAAPIDisplay());
	VAImageFormat *p_fmt = new VAImageFormat[fmtCount];
	if(!p_fmt) {
		destroySurfaces();
		AVG_ASSERT_MSG(false, "Cannot allocate mem for images");
		return;
	}

	if(vaQueryImageFormats(getVAAPIDisplay(), p_fmt, &fmtCount)) {
		delete[] p_fmt;
		destroySurfaces();
		AVG_ASSERT_MSG(false, "vaQueryImageFormats returned error");
		return;
	}

	//vlc_fourcc_t  i_chroma = 0;
	VAImageFormat fmt;
	int i = 0;
	for(; i < fmtCount; i++) {
		if(p_fmt[i].fourcc == VA_FOURCC( 'Y', 'V', '1', '2' ) ||
				p_fmt[i].fourcc == VA_FOURCC( 'I', '4', '2', '0' ) ||
				p_fmt[i].fourcc == VA_FOURCC( 'N', 'V', '1', '2' )) {
			if(vaCreateImage(getVAAPIDisplay(), &p_fmt[i],
					m_Size.x, m_Size.y, &m_Image)) {
				m_Image.image_id = VA_INVALID_ID;
				continue;
			}
			// Validate that vaGetImage works with this format
			if(vaGetImage(getVAAPIDisplay(), surface_id[0], 0, 0,
					m_Size.x, m_Size.y,	m_Image.image_id)) {
				vaDestroyImage(getVAAPIDisplay(), m_Image.image_id);
				m_Image.image_id = VA_INVALID_ID;
				AVG_ASSERT(false);
				continue;
			}

			fmt = p_fmt[i];
			vaDestroyImage(getVAAPIDisplay(), m_Image.image_id);
			m_Image.image_id = VA_INVALID_ID;
			break;
		}
	}
	delete[] p_fmt;
	if (i >= fmtCount)
		AVG_ASSERT(false);

	for(int i = 0; i < m_SurfaceCount && m_pVAAPISurface; i++) {
	    VAAPISurface *pVAAPISurface = &m_pVAAPISurface[i];
            pVAAPISurface->m_bBound = 0;
            VAStatus status = vaDeriveImage(getVAAPIDisplay(), pVAAPISurface->m_SurfaceID, &(pVAAPISurface->m_Image));
            if (status == VA_STATUS_SUCCESS) {
                /* vaDeriveImage() is supported, check format */
                if (pVAAPISurface->m_Image.format.fourcc != fmt.fourcc) {
                    vaDestroyImage(getVAAPIDisplay(), pVAAPISurface->m_Image.image_id);
		    pVAAPISurface->m_Image.image_id = VA_INVALID_ID;
		    AVG_ASSERT(false);
                    return;
                }
                if (pVAAPISurface->m_Image.width == m_Size.x && pVAAPISurface->m_Image.height == m_Size.y) {
                    pVAAPISurface->m_bBound = 1;
		    AVG_TRACE(Logger::category::PLAYER, Logger::severity::INFO, "using DeriveImage");
                }
                else {
                    vaDestroyImage(getVAAPIDisplay(), pVAAPISurface->m_Image.image_id);
		    pVAAPISurface->m_Image.image_id = VA_INVALID_ID;
                    status = VA_STATUS_ERROR_OPERATION_FAILED;
                }
                
            }
            if (status != VA_STATUS_SUCCESS) {
                status = vaCreateImage(getVAAPIDisplay(), &fmt,
                                       m_Size.x, m_Size.y, &(pVAAPISurface->m_Image));
                if (status != VA_STATUS_SUCCESS){
		    AVG_ASSERT(false);
                    return;
		}
            }
        }

	// Setup the ffmpeg hardware context
	m_HWCtx = new struct vaapi_context;
	pContext->hwaccel_context = m_HWCtx;
	    AVG_TRACE(Logger::category::PLAYER, Logger::severity::INFO, "hwaccel_context: " << pContext->hwaccel_context);
	memset(m_HWCtx, 0, sizeof(struct vaapi_context));
	m_HWCtx->display    = getVAAPIDisplay();
	m_HWCtx->config_id  = m_ConfigID;
	m_HWCtx->context_id = m_ContextID;

        AVG_TRACE(Logger::category::PLAYER, Logger::severity::INFO, "createSurfaces finish.");
}

void VAAPIDecoder::destroySurfaces()
{
	if(m_Image.image_id != VA_INVALID_ID) {
		vaDestroyImage(getVAAPIDisplay(), m_Image.image_id);
	}

	if(m_ContextID != VA_INVALID_ID)
		vaDestroyContext(getVAAPIDisplay(), m_ContextID);

	for(int i = 0; i < m_SurfaceCount && m_pVAAPISurface; i++) {
		VAAPISurface *pVAAPISurface = &m_pVAAPISurface[i];

		if(pVAAPISurface->m_Image.image_id != VA_INVALID_ID) {
			vaDestroyImage(getVAAPIDisplay(), pVAAPISurface->m_Image.image_id);
		}
		if(pVAAPISurface->m_SurfaceID != VA_INVALID_SURFACE)
			vaDestroySurfaces(getVAAPIDisplay(),
					  &pVAAPISurface->m_SurfaceID, 1);
	}
	delete[] m_pVAAPISurface;
	
	if (m_HWCtx != NULL)
		delete m_HWCtx;
	m_HWCtx = NULL;
	m_Image.image_id = VA_INVALID_ID;
	m_ContextID = VA_INVALID_ID;
	m_pVAAPISurface = NULL;
	m_Size = IntPoint(0,0);
}

}



