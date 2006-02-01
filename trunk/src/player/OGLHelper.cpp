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

#include "OGLHelper.h"

#include "../base/Logger.h"
#include "../base/Exception.h"

#include "GL/gl.h"
#include "GL/glu.h"
#ifndef __APPLE__
#include "GL/glx.h"
#include <X11/Xlib.h>
#endif

#include <iostream>
#include <sstream>

using namespace std;

namespace avg {

void OGLErrorCheck(int avgcode, string where) {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        stringstream s;
        s << "OpenGL error in " << where <<": " << gluErrorString(err) 
            << " (#" << err << ") ";
        AVG_TRACE(Logger::ERROR, s.str());
        if (err != GL_INVALID_OPERATION) {
            OGLErrorCheck(avgcode, "  --");
        }
        throw Exception(avgcode, s.str());
    }
}

bool queryOGLExtension(char *extName)
{
    /*
    ** Search for extName in the extensions string. Use of strstr()
    ** is not sufficient because extension names can be prefixes of
    ** other extension names. Could use strtok() but the constant
    ** string returned by glGetString might be in read-only memory.
    */
    char *p;
    int extNameLen = strlen(extName);

    p = (char *)glGetString(GL_EXTENSIONS);
//    cout << "OpenGL extensions string: " << p << endl;
    if (NULL == p) {
        throw Exception(AVG_ERR_VIDEO_GENERAL, "Couldn't get OpenGL extension string.");
    }

    char * end = p + strlen(p);

    while (p < end) {
        int n = strcspn(p, " ");
        if ((extNameLen == n) && (strncmp(extName, p, n) == 0)) {
            return true;
        }
        p += (n + 1);
    }
    return false;
}

bool queryGLXExtension(char *extName) {
#ifdef __APPLE__
    return false;
#else
    int extNameLen = strlen(extName);

    Display * display = XOpenDisplay(0);
    char * p = (char *)glXQueryExtensionsString(display, DefaultScreen(display));
    if (NULL == p) {
        throw Exception(AVG_ERR_VIDEO_GENERAL, "Couldn't get GLX extension string.");
    }
//    cout << "GLX extensions string: " << p << endl;

    char * end = p + strlen(p);

    while (p < end) {
        int n = strcspn(p, " ");
        if ((extNameLen == n) && (strncmp(extName, p, n) == 0)) {
            return true;
        }
        p += (n + 1);
    }
    return false;
#endif
}

}
