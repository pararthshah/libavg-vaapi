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

#include "BitmapManager.h"

#ifdef WIN32
#include  <io.h>
#endif
#include  <stdio.h>
#include  <stdlib.h>

#include "../base/OSHelper.h"


namespace avg {

BitmapManager * BitmapManager::s_pBitmapManager=0;

BitmapManager::BitmapManager()
{
    if (s_pBitmapManager) {
        throw Exception(AVG_ERR_UNKNOWN, "BitmapMananger has already been instantiated.");
    }
    
    m_pCmdQueue = BitmapManagerThread::CQueuePtr(new BitmapManagerThread::CQueue);
    m_pMsgQueue = BitmapManagerMsgQueuePtr(new BitmapManagerMsgQueue(8));
    m_pBitmapManagerThread = new boost::thread(
            BitmapManagerThread(*m_pCmdQueue, *m_pMsgQueue));
    
    s_pBitmapManager = this;
}

BitmapManager::~BitmapManager()
{
    while (!m_pCmdQueue->empty()) {
        m_pCmdQueue->pop();
    }
    while (!m_pMsgQueue->empty()) {
        m_pMsgQueue->pop();
    }
    m_pCmdQueue->pushCmd(boost::bind(&BitmapManagerThread::stop, _1));
    if (m_pBitmapManagerThread) {
        m_pBitmapManagerThread->join();
        delete m_pBitmapManagerThread;
    }

    s_pBitmapManager = 0;
}

BitmapManager* BitmapManager::get()
{
    if (!s_pBitmapManager) {
        s_pBitmapManager = new BitmapManager();
    }
    return s_pBitmapManager;
}

void BitmapManager::loadBitmap(const UTF8String& sUtf8FileName,
        const boost::python::object& pyFunc)
{
    std::string sFileName = convertUTF8ToFilename(sUtf8FileName);

#ifdef WIN32
    int rc = _access(sFileName.c_str(), 04);
#else
    int rc = access(sFileName.c_str(), R_OK);
#endif

    BitmapManagerMsgPtr msg = BitmapManagerMsgPtr(new BitmapManagerMsg());
    msg->setRequest(sUtf8FileName, pyFunc);

    if (rc != 0) {
        msg->setError(Exception(AVG_ERR_FILEIO, 
                std::string("BitmapManager can't open output file '") +
                sFileName + "'. Reason: " +
                strerror(errno)));
        m_pMsgQueue->push(msg);
    } else {
        m_pCmdQueue->pushCmd(boost::bind(&BitmapManagerThread::loadBitmap, _1, msg));
    }
}

void BitmapManager::onFrameEnd()
{
    while (!m_pMsgQueue->empty()) {
        BitmapManagerMsgPtr pMsg = m_pMsgQueue->pop();
        pMsg->executeCallback();
    }
}


}
