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

#ifndef _AVGNode_H_
#define _AVGNode_H_

#include "DivNode.h"
#include "KeyEvent.h"
#include "../graphics/Point.h"

#include <string>

namespace avg {

class AVGNode : public DivNode
{
	public:
        AVGNode ();
        AVGNode (const xmlNodePtr xmlNode);
        virtual ~AVGNode ();

        virtual std::string getTypeStr ();

        void handleKeyEvent (KeyEvent* pEvent);
        bool getCropSetting();

        const std::string& getOnKeyUp()
        {
            return m_sKeyUpHandler;
        }
        
        const std::string& getOnKeyDown()
        {
            return m_sKeyDownHandler;
        }
        
    private:
        std::string m_sKeyUpHandler;
        std::string m_sKeyDownHandler;
        bool m_bEnableCrop;

};

}

#endif //_AVGNode_H_

