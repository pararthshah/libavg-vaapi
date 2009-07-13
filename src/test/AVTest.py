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

import sys, os, platform

# Import the correct version of libavg. Since it should be possible to
# run the tests without installing libavg, we add the location of the 
# uninstalled libavg to the path.
# TODO: This is a mess.
sys.path += ['../wrapper/.libs', '../python']
if platform.system() == 'Darwin':
    sys.path += ['../..']     # Location of libavg in a mac installation. 

if platform.system() == 'Windows':
    from libavg import avg    # Under windows, there is no uninstalled version.
else:
    import avg

from testcase import *

class AVTestCase(AVGTestCase):
    def __init__(self, testFuncName):
        AVGTestCase.__init__(self, testFuncName, 24)
    def testEOF(self, node):
        def onEOF():
            Player.stop()
        def onNoEOF():
            self.assert_(False)
        self._loadEmpty()
        Player.getRootNode().appendChild(node)
        node.play()
        node.setEOFCallback(onEOF)
        Player.setTimeout(10000, onNoEOF)
        Player.play()

    def testVideoFiles(self):
        def testVideoFile(filename, isThreaded):
            def checkImage(filename):
                if not(isThreaded):
                    self.compareImage("testVideo-"+filename+"1", False)
            self._loadEmpty()
            node = Player.createNode("video",
                {"href": "../video/testfiles/"+filename, "threaded": isThreaded})
            Player.getRootNode().appendChild(node)
            self.start(None,
                    (lambda: node.play(),
                     lambda: checkImage(filename),
                     lambda: node.stop()
                    ))
        Player.setFakeFPS(25)
        for filename in ["mjpeg-48x48.avi", "mpeg1-48x48.mpg", "mpeg1-48x48-sound.avi", 
                "rgba-48x48.mov", "h264-48x48.h264"]:
            for isThreaded in [False, True]:
                testVideoFile(filename, isThreaded)

    def testVideo(self):
        def testGetMediaSize():
            self.assert_(Player.getElementByID("clogo2").getMediaSize() == (48, 48))
        def newHRef():
            node = Player.getElementByID("clogo2")
            node.href = "../video/testfiles/h264-48x48.h264"
            node.play()
        def move():
            node = Player.getElementByID("clogo2")
            node.x += 30
        def activateclogo():
            Player.getElementByID('clogo').active=1
        def deactivateclogo():
            Player.getElementByID('clogo').active=0
        Player.setFakeFPS(25)
        self.start("video.avg",
                (lambda: self.compareImage("testVideo1", False),
                 lambda: Player.getElementByID("clogo2").play(),
                 lambda: self.compareImage("testVideo2", False),
                 lambda: Player.getElementByID("clogo2").pause(),
                 lambda: self.compareImage("testVideo3", False),
                 lambda: Player.getElementByID("clogo2").play(),
                 lambda: self.compareImage("testVideo4", False),
                 newHRef,
                 lambda: Player.getElementByID("clogo1").play(),
                 lambda: self.compareImage("testVideo5", False),
                 move,
                 lambda: Player.getElementByID("clogo").pause(),
                 lambda: self.compareImage("testVideo6", False),
                 deactivateclogo,
                 lambda: self.compareImage("testVideo7", False),
                 activateclogo,
                 lambda: self.compareImage("testVideo8", False),
                 testGetMediaSize,
                 lambda: Player.getElementByID("clogo").stop(),
                 lambda: self.compareImage("testVideo9", False)
                ))

    def testVideoOpacity(self):
        def testWithFile(filename, testImgName):
            def hide():
                Player.getElementByID("video").opacity=0
            def show():
                Player.getElementByID("video").opacity=1
            Player.setFakeFPS(25)
            Player.loadString("""
                <avg width="160" height="120">
                    <video id="video" x="0" y="0" opacity="1" loop="true" threaded="false"/>
                </avg>""")
            Player.getElementByID("video").href=filename
            self.start(None,
                    (lambda: Player.getElementByID("video").play(),
                     lambda: self.compareImage(testImgName+"1", False),
                     hide,
                     None,
                     None,
                     show,
                     lambda: self.compareImage(testImgName+"2", False)
                    ))
        testWithFile("../video/testfiles/rgba-48x48.mov", "testVideoOpacityRGBA")
        testWithFile("../video/testfiles/mpeg1-48x48.mpg", "testVideoOpacityYUV")

    def testVideoSeek(self):
        def seek(frame):
            videoNode.seekToFrame(frame)
        for useCustomFPS in [False, True]:
            Player.setFakeFPS(25)
            self._loadEmpty()
            if useCustomFPS:
                videoNode = Player.createNode("video", 
                        {"loop":True, "fps":25, "threaded":False, 
                         "href":"../video/testfiles/mjpeg-48x48.avi"})
            else:
                videoNode = Player.createNode("video",
                        {"loop":True, "href":"../video/testfiles/mjpeg-48x48.avi",
                         "threaded":False})
            Player.getRootNode().appendChild(videoNode)
            self.start(None,
                    (lambda: videoNode.play(),
                     lambda: seek(100),
                     lambda: self.compareImage("testVideoSeek1", False),
                     lambda: videoNode.pause(),
                     lambda: seek(26),
                     None,
                     lambda: self.compareImage("testVideoSeek2", False),
                     lambda: videoNode.play(),
                     None,
                     lambda: self.compareImage("testVideoSeek3", False)
                    ))

    def testVideoFPS(self):
        Player.setFakeFPS(25)
        self.start("videofps.avg",
                (lambda: Player.getElementByID("video").play(),
                 None,
                 lambda: self.compareImage("testVideoFPS", False)
                ))

    def testVideoMask(self):
        def testWithFile(filename, testImgName):
            def setMask(href):
                video.maskhref = href
            def setOpacity():
                video.opacity = 0.5
            Player.setFakeFPS(25)
            Player.loadString("""
                <avg width="160" height="120">
                    <video id="video" x="0" y="0" opacity="1" threaded="false"
                            maskhref="mask.png"/>
                </avg>""")
            video = Player.getElementByID("video")
            video.href = filename
            video.play()
            self.start(None,
                    [lambda: self.compareImage(testImgName+"1", False),
                     lambda: video.seekToFrame(10),
                     lambda: setMask(""),
                     lambda: self.compareImage(testImgName+"2", False),
                     lambda: setMask("mask2.png"),
                     lambda: self.compareImage(testImgName+"3", False),
                     setOpacity,
                     lambda: self.compareImage(testImgName+"4", False),
                    ])
        testWithFile("../video/testfiles/mpeg1-48x48.mpg", "testVideoMaskYUV")
        testWithFile("../video/testfiles/mjpeg-48x48.avi", "testVideoMaskYUVJ")
        testWithFile("../video/testfiles/rgba-48x48.mov", "testVideoMaskRGBA")

    def testVideoEOF(self):
        Player.setFakeFPS(25)
        for filename in ["mpeg1-48x48.mpg", "mpeg1-48x48-sound.avi"]:
            node = Player.createNode("video",
                    {"href": "../video/testfiles/"+filename})
            self.testEOF(node)

    def testSound(self):
        def testSoundFile(filename):
            def setVolume(volume):
                node.volume = volume
            def testGetVolume(volume):
                self.assert_(node.volume == volume)
            self._loadEmpty()
            node = Player.createNode("sound",
                    {"href": "../video/testfiles/"+filename})
            Player.getRootNode().appendChild(node)
            self.start(None,
                    (
                     lambda: setVolume(0.5),
                     lambda: testGetVolume(0.5),
                     lambda: node.play(),
                     None,
                     lambda: node.stop(),
                     lambda: node.play(),
                     lambda: node.pause(),
                     lambda: node.play(),
                     lambda: setVolume(0.5),
                     lambda: testGetVolume(0.5),
                     lambda: node.pause(),
                     lambda: node.stop(),
                     lambda: setVolume(0.3),
                     lambda: testGetVolume(0.3),
                     lambda: node.pause()
                    ))
        Player.setFakeFPS(-1)
        Player.volume = 0 
        for filename in ["22.050Hz_16bit_mono.wav", "44.1kHz_16bit_stereo.aif", 
                "44.1kHz_16bit_stereo.wav", "44.1kHz_mono.ogg", "44.1kHz_stereo.mp3", 
                "48kHz_24bit_stereo.wav"]:
            testSoundFile(filename)

    def testBrokenSound(self):
        def openSound():
            node = Player.createNode("sound",
                    {"href": "../video/testfiles/44.1kHz_16bit_6Chan.ogg"})
            Player.getRootNode().appendChild(node)
            self.assertException(node.play)
        self._loadEmpty()
        self.start(None,
                [openSound])

    def testSoundEOF(self):
        Player.setFakeFPS(-1)
        Player.volume = 0 
        node = Player.createNode("sound",
                {"href": "../video/testfiles/44.1kHz_16bit_mono.wav"})
        self.testEOF(node)


def AVTestSuite(tests):
    availableTests = (
            'testSound',
            'testBrokenSound',
            'testSoundEOF',
            "testVideoFiles",
#            "testVideo",
            "testVideoOpacity",
            "testVideoSeek",
            "testVideoFPS",
            "testVideoMask",
            "testVideoEOF",
            )
    return AVGTestSuite(availableTests, AVTestCase, tests)

Player = avg.Player.get()

if __name__ == '__main__':
    runStandaloneTest(AVTestSuite)

