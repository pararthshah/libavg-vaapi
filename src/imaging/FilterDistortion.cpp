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
//  Original author of this file is igor@c-base.org.
//

#include "FilterDistortion.h"

#include <iostream>
#include <math.h>

using namespace std;

namespace avg{

    FilterDistortion::FilterDistortion(const IntPoint& SrcSize,
            CoordTransformerPtr coordtrans)
      : m_SrcSize(SrcSize),
        m_pTrafo(coordtrans) 
    {
        //we use the same dimensions for both of src and dest and just crop...
        //for each pixel at (x,y) in the dest
        //m_pMap[x][y] contains a IntPoint that gives the coords in the src Bitmap 
        m_pMap = new IntPoint[m_SrcSize.y*m_SrcSize.x];
        for(int y=0; y<m_SrcSize.y; ++y) {
            for(int x=0; x<m_SrcSize.x; ++x) {
                DPoint tmp = m_pTrafo->inverse_transform_point(DPoint(int(x),int(y)));
                IntPoint tmp2(int(tmp.x+0.5),int(tmp.y+0.5));
                if(tmp2.x < m_SrcSize.x && tmp2.y < m_SrcSize.y){
                    m_pMap[y*m_SrcSize.x+x] = tmp2;
                } else {
                    m_pMap[y*m_SrcSize.y+x] = IntPoint(0,0);
                }
            }
        }
    }
    
    FilterDistortion::~FilterDistortion()
    {
        delete[] m_pMap;
    }

    BitmapPtr FilterDistortion::apply(BitmapPtr pBmpSource)
    {
        BitmapPtr res = BitmapPtr(new Bitmap(m_SrcSize, I8));
        unsigned char *p = res->getPixels();
        unsigned char *src = pBmpSource->getPixels();
        unsigned char *pLine = p;
        int destStride = res->getStride();
        int srcStride = pBmpSource->getStride();
        IntPoint *pMapPos = m_pMap;
        for(int y=0; y<m_SrcSize.y; ++y) {
            for(int x=0; x<m_SrcSize.x; ++x) {
                *pLine = src[pMapPos->x + srcStride*pMapPos->y];
                pLine++;
                pMapPos++;
            }
            p+=destStride;
            pLine = p;
        }
        return res;
    }

}
