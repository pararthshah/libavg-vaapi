//
//  libavg - Media Playback Engine. 
//  Copyright (C) 2003-2006 Ulrich von Zadow
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
//  Original author of this file is igor@c-base.org 
//

#ifndef _ConnectedComps_H_
#define _ConnectedComps_H_

#include "BlobInfo.h"
#include "Run.h"

#include "../graphics/Bitmap.h"
#include "../graphics/Pixel32.h"

#include "../base/Point.h"

#include <assert.h>
#include <list>
#include <vector>
#include <map>

namespace avg {

class Blob;
typedef boost::shared_ptr<class Blob> BlobPtr;
typedef boost::weak_ptr<class Blob> BlobWeakPtr;
typedef std::vector<BlobPtr> BlobVector;
typedef boost::shared_ptr<BlobVector> BlobVectorPtr;

class Blob
{
    public:
        Blob(const RunPtr& pRun);
        ~Blob();

        void addRun(const RunPtr& pRun);
        BlobInfoPtr getInfo();
        void merge(const BlobPtr& other);
        RunArray* getRuns();
        void render(BitmapPtr pSrcBmp, BitmapPtr pDestBmp, Pixel32 Color, 
                int Min, int Max, bool bFinger, bool bMarkCenter, 
                Pixel32 CenterColor= Pixel32(0x00, 0x00, 0xFF, 0xFF));
        bool contains(IntPoint pt);
        void clearRelatedBlobs();
        void addRelatedBlob(BlobPtr pBlob);
        std::vector<BlobWeakPtr> getRelatedBlobs();

        BlobPtr m_pParent;

    private:
        Blob(const Blob &);

        RunArray *m_pRuns;

        BlobInfoPtr m_pBlobInfo;
        boost::shared_ptr<DPoint> m_pCenter;
        std::vector<BlobWeakPtr> m_RelatedBlobs; // For fingers, this contains the hand.
                                   // For hands, this contains the fingers.
};

BlobVectorPtr connected_components(BitmapPtr image, unsigned char object_threshold);

}

#endif
