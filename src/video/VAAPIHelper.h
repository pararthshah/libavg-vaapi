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
#ifndef VAAPIHELPER_H_
#define VAAPIHELPER_H_

#include "../avgconfigwrapper.h"

#include <va/va.h>
#include <va/va_x11.h>
#include <libavcodec/vaapi.h>
#include <boost/shared_ptr.hpp>

#include "VAAPIDecoder.h"

namespace avg {

class VAAPISurface;
class Bitmap;
typedef boost::shared_ptr<Bitmap> BitmapPtr;

VADisplay getVAAPIDisplay();

void getPlanesFromVAAPI(VAAPISurface* pVaapiSurface, BitmapPtr pBmpY,
        BitmapPtr pBmpU, BitmapPtr pBmpV);
void getBitmapFromVAAPI(VAAPISurface* pVaapiSurface, BitmapPtr pBmpDest);
//void unlockVAAPISurface(VAAPISurface* pVaapiSurface);

}
#endif /* VAAPIHELPER_H_ */
