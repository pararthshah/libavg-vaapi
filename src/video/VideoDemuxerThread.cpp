//
//  libavg - Media Playback Engine. 
//  Copyright (C) 2003-2008 Ulrich von Zadow
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

#include "VideoDemuxerThread.h"
#include "FrameVideoMsg.h"
#include "ErrorVideoMsg.h"
#include "EOFVideoMsg.h"

#include "../base/Logger.h"
#include "../base/TimeSource.h"

#include <climits>

using namespace std;

namespace avg {

VideoDemuxerThread::VideoDemuxerThread(CmdQueue& CmdQ, 
                AVFormatContext * pFormatContext)
    : WorkerThread<VideoDemuxerThread>("VideoDemuxer", CmdQ),
      m_bEOF(false),
      m_pFormatContext(pFormatContext),
      m_pDemuxer()

{
}

VideoDemuxerThread::~VideoDemuxerThread()
{
}

bool VideoDemuxerThread::init()
{
    m_pDemuxer = FFMpegDemuxerPtr(new FFMpegDemuxer(m_pFormatContext));
    return true;
};

bool VideoDemuxerThread::work() 
{
    if (m_PacketQs.empty() || m_bEOF) {
        // replace this with waitForMessage()
        msleep(10);
    } else {

        map<int, VideoPacketQueuePtr>::iterator it;
        int ShortestQ = -1;
        int ShortestLength = INT_MAX;
        for (it=m_PacketQs.begin(); it != m_PacketQs.end(); it++) {
            if (it->second->size() < ShortestLength && 
                    it->second->size() < it->second->getMaxSize() &&
                    !m_PacketQbEOF[it->first])
            {
                ShortestLength = it->second->size();
                ShortestQ = it->first;
            }
        }
        
        if (ShortestQ < 0) {
            // All queues are at their max capacity. Take a nap and try again later.
            msleep(10);
            return true;
        }
        
        AVPacket * pPacket = m_pDemuxer->getPacket(ShortestQ);
        if (pPacket == 0) {
            onStreamEOF(ShortestQ);
        }
       
        // On EOF, we send a message which has pPacket=0
        m_PacketQs[ShortestQ]->push(PacketVideoMsgPtr(new PacketVideoMsg(pPacket, false)));
        msleep(0);
    }
    return true;
}

void VideoDemuxerThread::deinit()
{
}

void VideoDemuxerThread::enableStream(VideoPacketQueuePtr pPacketQ, int StreamIndex)
{
    m_PacketQs[StreamIndex] = pPacketQ;
    m_PacketQbEOF[StreamIndex] = false;
    m_pDemuxer->enableStream(StreamIndex);
}

void VideoDemuxerThread::seek(long long DestTime)
{
    map<int, VideoPacketQueuePtr>::iterator it;
    m_pDemuxer->seek(DestTime);
    for (it=m_PacketQs.begin(); it != m_PacketQs.end(); it++) {
        VideoPacketQueuePtr pPacketQ = it->second;
        pPacketQ->push(PacketVideoMsgPtr(new PacketVideoMsg(0, true)));
        m_PacketQbEOF[it->first] = false;
    }
    m_bEOF = false;
}

void VideoDemuxerThread::onStreamEOF(int StreamIndex)
{
    m_PacketQbEOF[StreamIndex] = true;
                
    m_bEOF = true;
    map<int, bool>::iterator it;
    for (it=m_PacketQbEOF.begin(); it != m_PacketQbEOF.end(); it++) {
        if (!it->second) {
            m_bEOF = false;
            break;
        }
    }
}

}
