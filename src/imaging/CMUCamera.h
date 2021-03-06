//
//  libavg - Media Playback Engine. 
//  Copyright (C) 2003-2011 Ulrich von Zadow
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
//  Current versions can be found at www.libavg.de
//
//  Original author of this file is Nick Hebner (hebnern@gmail.com).
//

#ifndef _CMUCamera_H_
#define _CMUCamera_H_

#include "../api.h"
#include "Camera.h"

#include <string>

class C1394Camera;

namespace avg {

class AVG_API CMUCamera : public Camera {
public:
    CMUCamera(long long guid, bool bFW800, IntPoint Size, PixelFormat camPF, 
            PixelFormat destPF, float FrameRate);
    virtual ~CMUCamera();

    virtual BitmapPtr getImage(bool bWait);

    virtual const std::string& getDevice() const; 
    virtual const std::string& getDriverName() const; 
 
    virtual int getFeature(CameraFeature Feature) const;
    virtual void setFeature(CameraFeature Feature, int Value, bool bIgnoreOldValue=false);
    virtual void setFeatureOneShot(CameraFeature Feature);
    virtual int getWhitebalanceU() const;
    virtual int getWhitebalanceV() const;
    virtual void setWhitebalance(int u, int v, bool bIgnoreOldValue=false);

    static void dumpCameras();
    static int countCameras();
    static CameraInfo* getCameraInfos(int deviceNumber);

private:
    int getCamIndex(long long guid);
    void internalGetFeature(CameraFeature Feature, unsigned short* val1, 
            unsigned short* val2) const;
    void enablePtGreyBayer();
    void checkCMUError(int code, int type, const std::string& sMsg) const;
    void checkCMUWarning(bool bOk, const std::string& sMsg) const;
    std::string CMUErrorToString(int code);

    static void getCameraControls(C1394Camera* pCamera, CameraInfo* camInfo);
    static void getCameraImageFormats(C1394Camera* pCamera, CameraInfo* pCamInfo);
    static void getCameraFramerates(C1394Camera* pCamera, unsigned long videoFormat,
            unsigned long videoMode, FrameratesVector &framerates);

    std::string m_sDevice;

    mutable C1394Camera * m_pCamera; // The CMU1394 lib is not const-correct.
    FeatureMap m_Features;
    int m_WhitebalanceU;
    int m_WhitebalanceV;
};

}

#endif
