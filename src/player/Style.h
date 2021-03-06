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

#ifndef _Style_H_
#define _Style_H_

#include "BoostPython.h"

#include "../api.h"

#include <boost/shared_ptr.hpp>

#include <string>
#include <vector>

namespace avg {

class Style;
typedef boost::shared_ptr<Style> StylePtr;

#ifdef _WIN32
// Silences warning about py::dict not being exported.
#pragma warning( disable: 4251 )
#endif

class AVG_API Style 
{
    public:
        Style(const py::dict& params);
        virtual ~Style();

        // python dict interface.
        py::object __getitem__(py::object& key) const;
        bool __contains__(py::object& key) const;
        py::list keys() const;
        py::list values() const;
        py::list items() const;
        int __len__() const;
        py::object __iter__() const;
        py::object iteritems() const;
        py::object iterkeys() const;
        py::object itervalues() const;
        std::string __repr__() const;

        // C++ interface
        py::dict mergeParams(const py::dict& attrs);

    private:
        const py::dict& getDict()const;

        py::dict m_Properties;
};

}

#endif

