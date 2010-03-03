#!/usr/bin/python
# -*- coding: utf-8 -*-
# libavg - Media Playback Engine.
# Copyright (C) 2003-2008 Ulrich von Zadow
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Current versions can be found at www.libavg.de
#

import unittest

import platform

from libavg import avg
from testcase import *

class PluginTestCase(AVGTestCase):
    def __init__(self, testFuncName):
        AVGTestCase.__init__(self, testFuncName, 24)

    def testColorNodePlugin(self):
        def loadPlugin():
            if platform.system() == 'Windows':
                Player.loadPlugin("ColorNode")
            else:
                if os.getenv('srcdir') in ('.', None):
                    # make check or ./Test.py
                    addpth = './'
                else:
                    # make distcheck
                    addpth = '../../_build/src/test/'

                Player.pluginPath += ":"+addpth+"plugin/.libs"
                Player.loadPlugin("libColorNode")
            
        def usePlugin1():
            node = colorplugin.ColorNode(fillcolor="7f7f00", id="mynode1")
            Player.getElementByID("container").appendChild(node)
            
            mynode = Player.getElementByID("mynode1")
            self.assert_(mynode.fillcolor == "7f7f00")
 
        def usePlugin2():
            node = Player.createNode('<colornode fillcolor="0f3f7f" id="mynode2" />')
            Player.getElementByID("container").appendChild(node)

            mynode = Player.getElementByID("mynode2")
            self.assert_(mynode.fillcolor == "0f3f7f")

       
        Player.loadString("""
            <avg width="160" height="120" id="container" />""")

        self.start(None, (
            loadPlugin,
            usePlugin1,
            lambda: self.compareImage("testplugin1", False),
            usePlugin2,
            lambda: self.compareImage("testplugin2", False),
        ))

def pluginTestSuite (tests):
    availableTests = ("testColorNodePlugin",)
    return AVGTestSuite (availableTests, PluginTestCase, tests)
    
Player = avg.Player.get()
