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

#include "VAAPIHelper.h"

#include "../base/Exception.h"
#include "../base/ConfigMgr.h"

#include "../graphics/Bitmap.h"

#include <dlfcn.h>

using namespace std;

namespace avg {

VADisplay getVAAPIDisplay()
{
    static VADisplay vaDisplay = 0;
    static bool bInitFailed = false;
    static int majorVersion = 0;
    static int minorVersion = 0;

    if (vaDisplay) {
        return vaDisplay;
    }

    if (bInitFailed) {
        return 0;
    }

    Display* pXDisplay = XOpenDisplay(NULL);
    AVG_ASSERT(pXDisplay);

    if (!(ConfigMgr::get()->getBoolOption("scr", "videoaccel", true))) {
        bInitFailed = true;
        return 0;
    }

    vaDisplay = vaGetDisplay(pXDisplay);
    if (!vaDisplay) {
    	bInitFailed = true;
    	return 0;
    }

    VAStatus status = vaInitialize(vaDisplay, &majorVersion, &minorVersion);
    if (status != VA_STATUS_SUCCESS) {
    	bInitFailed = true;
    	return 0;
    }

    return vaDisplay;
}

void getPlanesFromVAAPI(VAAPISurface* pVaapiSurface,
		BitmapPtr pBmpY, BitmapPtr pBmpU, BitmapPtr pBmpV)
{
	VAAPIDecoder* pVaapiDecoder = pVaapiSurface->m_pDecoder;
	VASurfaceID surface_id = pVaapiSurface->m_SurfaceID;

#if VA_CHECK_VERSION(0,31,0)
	if(vaSyncSurface(getVAAPIDisplay(), pVaapiSurface->m_SurfaceID)) {
#else
	if(vaSyncSurface(getVAAPIDisplay(), pHWCtx->context_id,
			pVaapiSurface->m_SurfaceID)) {
#endif
		AVG_ASSERT_MSG(false, "vaSyncSurface returned error");
		return;
	}

	/* XXX vaDeriveImage may be better but it is not supported by
	 * my setup.
	 */

	if(vaGetImage(getVAAPIDisplay(), surface_id, 0, 0, pVaapiDecoder->m_Size.x,
			pVaapiDecoder->m_Size.y, pVaapiDecoder->m_Image.image_id) ) {
		AVG_ASSERT_MSG(false, "vaGetImage returned error");
		return;
	}

	void *p_base;
	if(vaMapBuffer(getVAAPIDisplay(), pVaapiDecoder->m_Image.buf,
			&p_base)) {
		AVG_ASSERT_MSG(false, "vaMapBuffer returned error");
		return;
	}

	const uint32_t i_fourcc = pVaapiDecoder->m_Image.format.fourcc;
	AVG_ASSERT(i_fourcc == VA_FOURCC('Y','V','1','2'))

	bool b_swap_uv = i_fourcc == VA_FOURCC('I','4','2','0');
	uint8_t *pp_plane[3];
	size_t  pi_pitch[3];

	for( int i = 0; i < 3; i++ ) {
		pp_plane[i] = (uint8_t*)p_base + pVaapiDecoder->m_Image.offsets[i];
		pi_pitch[i] = pVaapiDecoder->m_Image.pitches[i];
	}

	uint8_t *dest[3] = {
			pBmpY->getPixels(),
			pBmpV->getPixels(),
			pBmpU->getPixels()
	};
	uint32_t pitches[3] = {
			pBmpY->getStride(),
			pBmpV->getStride(),
			pBmpU->getStride()
	};

	CopyPlane(dest[0], pitches[0], pp_plane[0], pi_pitch[0],
			pVaapiDecoder->m_Size.x, pVaapiDecoder->m_Size.y);
	CopyPlane(dest[1], pitches[1], pp_plane[1], pi_pitch[1],
			pVaapiDecoder->m_Size.x/2, pVaapiDecoder->m_Size.y/2);
	CopyPlane(dest[2], pitches[2], pp_plane[2], pi_pitch[2],
			pVaapiDecoder->m_Size.x/2, pVaapiDecoder->m_Size.y/2);

	if(vaUnmapBuffer(getVAAPIDisplay(), pVaapiDecoder->m_Image.buf)) {
		AVG_ASSERT_MSG(false, "vaUnmapBuffer returned error");
		return;
	}

    //unlockVAAPISurface(pVaapiSurface);
}

void getBitmapFromVAAPI(VAAPISurface* pVaapiSurface, BitmapPtr pBmpDest)
{
    IntPoint YSize = pBmpDest->getSize();
    IntPoint UVSize(YSize.x/2, YSize.y/2);
    BitmapPtr pBmpY(new Bitmap(YSize, I8));
    BitmapPtr pBmpU(new Bitmap(UVSize, I8));
    BitmapPtr pBmpV(new Bitmap(UVSize, I8));
    getPlanesFromVAAPI(pVaapiSurface, pBmpY, pBmpU, pBmpV);
    pBmpDest->copyYUVPixels(*pBmpY, *pBmpU, *pBmpV, false);
}

void CopyPlane(uint8_t *dst, size_t dst_pitch, const uint8_t *src,
		size_t src_pitch, unsigned width, unsigned height) {
	for (unsigned y = 0; y < height; y++) {
		memcpy(dst, src, width);
		src += src_pitch;
		dst += dst_pitch;
	}
}

/*void unlockVAAPISurface(VAAPISurface* pVaapiSurface)
{
    pRenderState->state &= ~FF_VDPAU_STATE_USED_FOR_REFERENCE;
}*/

}


