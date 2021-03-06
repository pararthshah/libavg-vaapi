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
//  Original author of this file is igor@c-base.org 
//

#ifndef _ConnectedComps_H_
#define _ConnectedComps_H_

#include "../api.h"
#include "Run.h"

#include "../graphics/Bitmap.h"
#include "../graphics/Pixel32.h"

#include "../base/GLMHelper.h"

#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

namespace avg {

class Blob;
typedef boost::shared_ptr<class Blob> BlobPtr;
typedef boost::weak_ptr<class Blob> BlobWeakPtr;
typedef std::vector<BlobPtr> BlobVector;
typedef std::vector<BlobWeakPtr> BlobWeakPtrVector;
typedef boost::shared_ptr<BlobVector> BlobVectorPtr;
typedef std::vector<IntPoint> ContourSeq;

class AVG_API Blob
{
    public:
        Blob(const Run& run);
        ~Blob();

        void addRun(const Run& run);
        void merge(const BlobPtr& other);
        RunArray* getRuns();
        void render(BitmapPtr pSrcBmp, BitmapPtr pDestBmp, Pixel32 Color, 
                int Min, int Max, bool bFinger, bool bMarkCenter, 
                Pixel32 CenterColor= Pixel32(0x00, 0x00, 0xFF, 0xFF));
        bool contains(IntPoint pt);

        void calcStats();
        void calcContour(int Precision);
        ContourSeq getContour();

        const glm::vec2& getCenter() const;
        const glm::vec2& getEstimatedNextCenter() const;
        float getArea() const;
        const IntRect & getBoundingBox() const;
        float getEccentricity() const;
        float getInertia() const;
        float getOrientation() const;
        const glm::vec2& getScaledBasis(int i) const;
        const glm::vec2& getEigenVector(int i) const;
        const glm::vec2& getEigenValues() const;

        void calcNextCenter(glm::vec2 oldCenter);
        void clearRelated();
        void addRelated(BlobPtr pBlob);
        const BlobPtr getFirstRelated(); 

        BlobPtr m_pParent;

    private:
        Blob(const Blob &);
        glm::vec2 calcCenter();
        IntRect calcBBox();
        int calcArea();
        void initRowPositions();
        IntPoint findNeighborInside(const IntPoint& Pt, int& Dir);
        bool ptIsInBlob(const IntPoint& Pt);

        RunArray m_Runs; // This array is unsorted until contours are calculated.
        std::vector<RunArray::iterator> m_RowPositions;
        BlobWeakPtrVector m_RelatedBlobs; // For fingers, this contains the hand.
                                          // For hands, this contains the fingers.

        bool m_bStatsAvailable;
        glm::vec2 m_EstimatedNextCenter;
        glm::vec2 m_Center;
        float m_Area;
        IntRect m_BoundingBox;
        float m_Eccentricity;
        float m_Inertia;
        float m_Orientation;
        glm::vec2 m_ScaledBasis[2];
        glm::vec2 m_EigenVector[2];
        glm::vec2 m_EigenValues;

        ContourSeq m_Contour;
};

BlobVectorPtr AVG_API findConnectedComponents(BitmapPtr pBmp, 
        unsigned char threshold);

}

#endif
