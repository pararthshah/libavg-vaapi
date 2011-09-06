# -*- coding: utf-8 -*-
# libavg - Media Playback Engine.
# Copyright (C) 2003-2011 Ulrich von Zadow
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

from libavg import avg, statemachine

from helper import *

from math import *

g_Player = avg.Player.get()

MAX_TAP_DISTANCE_IN_MM = 8
MAX_TAP_TIME = 500
MAX_DOUBLETAP_TIME = 300

class ContactData:

    def __init__(self, listenerid):
        self.listenerid = listenerid


class Recognizer(object):

    def __init__(self, node, eventSource, maxContacts, initialEvent):
        self._node = node
        self.__eventSource = eventSource
       
        self.__setEventHandler() 
        self.__isEnabled = True
        self.__maxContacts = maxContacts
        self._contacts = {}
        self.__dirty = False
        if initialEvent:
            self._onDown(initialEvent)

    def enable(self, isEnabled):
        if isEnabled != self.__isEnabled:
            self.__isEnabled = isEnabled
            if isEnabled:
                self.__setEventHandler()
            else:
                if self._contacts != {}:
                    self._abort()
                self._node.disconnectEventHandler(self)

    def _onDown(self, event):
        if self.__maxContacts == None or len(self._contacts) < self.__maxContacts:
            listenerid = event.contact.connectListener(self._onMotion, self._onUp)
            self._contacts[event.contact] = ContactData(listenerid)
            if len(self._contacts) == 1:
                self.__frameHandlerID = g_Player.setOnFrameHandler(self._onFrame)
            self.__dirty = True
            return self._handleDown(event)

    def _onMotion(self, event):
        self.__dirty = True
        self._handleMove(event)

    def _onUp(self, event):
        self.__dirty = True
        listenerid = self._contacts[event.contact].listenerid
        del self._contacts[event.contact]
        event.contact.disconnectListener(listenerid)
        if self._contacts == {}:
            g_Player.clearInterval(self.__frameHandlerID)
        self._handleUp(event)

    def _abort(self):
        for contact, contactData in self._contacts.iteritems():
            contact.disconnectListener(contactData.listenerid)
        self._contacts = {}
        g_Player.clearInterval(self.__frameHandlerID)

    def _handleDown(self, event):
        pass

    def _handleMove(self, event):
        pass

    def _handleUp(self, event):
        pass

    def _handleChange(self):
        pass

    def _onFrame(self):
        if self.__dirty:
            self._handleChange()
            self.__dirty = False

    def __setEventHandler(self):
        self._node.connectEventHandler(avg.CURSORDOWN, self.__eventSource, self, 
                self._onDown)


class HoldRecognizer(Recognizer):

    # States
    UP = 0          # No action pending
    DOWN = 1        # Down, but <  holdDelay
    HOLDING = 2     # Down, > holdDelay, < activateDelay
    ACTIVE = 3      # > activateDelay

    def __init__(self, node, holdDelay, activateDelay, eventSource=avg.TOUCH | avg.MOUSE, 
            startHandler=None, holdHandler=None, activateHandler=None, stopHandler=None,
            initialEvent=None):
        self.__startHandler = optionalCallback(startHandler, lambda pos:None)
        self.__holdHandler = optionalCallback(holdHandler, lambda t:None)
        self.__activateHandler = optionalCallback(activateHandler, lambda:None)
        self.__stopHandler = optionalCallback(stopHandler, lambda:None)

        self.__holdDelay = holdDelay
        self.__activateDelay = activateDelay

        self.__frameHandlerID = None
        self.__state = HoldRecognizer.UP

        self.__relTime = 0
        self.__lastEvent = None
        Recognizer.__init__(self, node, eventSource, 1, initialEvent)

    def abort(self):
        self._onUp(self.__lastEvent)

    def getRelTime(self):
        return self.__relTime

    def getLastEvent(self):
        return self.__lastEvent

    def _handleDown(self, event):
        self.__startPos = event.pos
        self.__startTime = g_Player.getFrameTime()
        self.__lastEvent = event
        self.__changeState(HoldRecognizer.DOWN)
        self.__frameHandlerID = g_Player.setOnFrameHandler(self.__onFrame)

    def _handleMove(self, event):
        self.__lastEvent = event
        if event.contact.distancefromstart > 8:
            self.__startPos = event.pos
            self.__startTime = g_Player.getFrameTime()
            if self.__state != HoldRecognizer.DOWN:
                self.__stopHandler()
                self.__changeState(HoldRecognizer.DOWN)

    def __onFrame(self):
        self.__relTime = g_Player.getFrameTime() - self.__startTime
        if self.__state == HoldRecognizer.DOWN:
            if self.__relTime > self.__holdDelay:
                holdOk = self.__startHandler(self.__startPos)
                if holdOk:
                    self.__changeState(HoldRecognizer.HOLDING)
        if self.__state == HoldRecognizer.HOLDING:
            if self.__relTime > self.__activateDelay:
                self.__changeState(HoldRecognizer.ACTIVE)
                self.__activateHandler()
            else:
                self.__holdHandler(float(self.__relTime-self.__holdDelay)/
                        (self.__activateDelay-self.__holdDelay))

    def _handleUp(self, event):
        g_Player.clearInterval(self.__frameHandlerID)
        self.__frameHandlerID = None
        self.__lastEvent = None
        if self.__state != HoldRecognizer.DOWN:
            self.__stopHandler()
        self.__changeState(HoldRecognizer.UP)
        self.__relTime = 0

    def __changeState(self, newState):
        self.__state = newState


class TapRecognizer(Recognizer):

    def __init__(self, node, eventSource=avg.TOUCH | avg.MOUSE, startHandler=None,
            tapHandler=None, failHandler=None, maxTime=MAX_TAP_TIME, initialEvent=None):
        self.__startHandler = optionalCallback(startHandler, lambda:None)
        self.__tapHandler = optionalCallback(tapHandler, lambda:None)
        self.__failHandler = optionalCallback(failHandler, lambda:None)
        self.__maxTime = maxTime

        self.__stateMachine = statemachine.StateMachine("TapRecognizer", "IDLE")
        self.__stateMachine.addState("IDLE", ("HOLDING",))
        self.__stateMachine.addState("HOLDING", ("IDLE",))

        self.__maxDistance = (
                MAX_TAP_DISTANCE_IN_MM*g_Player.getPixelsPerMM())
        Recognizer.__init__(self, node, eventSource, 1, initialEvent)

    def _handleDown(self, event):
        self.__stateMachine.changeState("HOLDING")
        self.__startHandler()
        self.__startTime = g_Player.getFrameTime()

    def _handleMove(self, event):
        if self.__stateMachine.state != "IDLE": 
            if event.contact.distancefromstart > self.__maxDistance:
                self.__stateMachine.changeState("IDLE")
                self.__failHandler()

    def _handleUp(self, event):
        if self.__stateMachine.state == "HOLDING":
            if event.contact.distancefromstart > self.__maxDistance:
                self.__failHandler()
            else:
                self.__tapHandler()
            self.__stateMachine.changeState("IDLE")

    def _onFrame(self):
        downTime = g_Player.getFrameTime() - self.__startTime
        if self.__stateMachine.state == "HOLDING":
            if downTime > self.__maxTime:
                self.__stateMachine.changeState("IDLE")
                self.__failHandler()
        Recognizer._onFrame(self)


class DoubletapRecognizer(Recognizer):
    
    def __init__(self, node, eventSource=avg.TOUCH | avg.MOUSE, startHandler=None,
            tapHandler=None, failHandler=None, maxTime=MAX_DOUBLETAP_TIME, 
            initialEvent=None):
        self.__startHandler = optionalCallback(startHandler, lambda:None)
        self.__tapHandler = optionalCallback(tapHandler, lambda:None)
        self.__failHandler = optionalCallback(failHandler, lambda:None)
        self.__maxTime = maxTime

        self.__stateMachine = statemachine.StateMachine("TapRecognizer", "IDLE")
        self.__stateMachine.addState("IDLE", ("DOWN1",), enterFunc=self.__enterIdle)
        self.__stateMachine.addState("DOWN1", ("UP1", "IDLE"))
        self.__stateMachine.addState("UP1", ("DOWN2", "IDLE"))
        self.__stateMachine.addState("DOWN2", ("IDLE",))

        self.__frameHandlerID = None
        self.__maxDistance = MAX_TAP_DISTANCE_IN_MM*g_Player.getPixelsPerMM()
        Recognizer.__init__(self, node, eventSource, 1, initialEvent)
   
    def _handleDown(self, event):
        self.__startTime = g_Player.getFrameTime()
        if self.__stateMachine.state == "IDLE":
            self.__frameHandlerID = g_Player.setOnFrameHandler(self.__onFrame)
            self.__stateMachine.changeState("DOWN1")
            self.__startPos = event.pos
            self.__startHandler()
        elif self.__stateMachine.state == "UP1":
            if (event.pos - self.__startPos).getNorm() > self.__maxDistance:
                self.__stateMachine.changeState("IDLE")
                self.__failHandler()
            else:
                self.__stateMachine.changeState("DOWN2")
        else:
            assert(False)

    def _handleMove(self, event):
        if self.__stateMachine.state != "IDLE": 
            if (event.pos - self.__startPos).getNorm() > self.__maxDistance:
                self.__stateMachine.changeState("IDLE")
                self.__failHandler()

    def _handleUp(self, event):
        if self.__stateMachine.state == "DOWN1":
            self.__startTime = g_Player.getFrameTime()
            self.__stateMachine.changeState("UP1")
        elif self.__stateMachine.state == "DOWN2":
            if (event.pos - self.__startPos).getNorm() > self.__maxDistance:
                self.__failHandler()
            else:
                self.__tapHandler()
            self.__stateMachine.changeState("IDLE")
        elif self.__stateMachine.state == "IDLE":
            pass
        else:
            assert(False)

    def __onFrame(self):
        downTime = g_Player.getFrameTime() - self.__startTime
        if downTime > self.__maxTime:
            self.__failHandler()
            self.__stateMachine.changeState("IDLE")

    def __enterIdle(self):
        g_Player.clearInterval(self.__frameHandlerID)


class DragRecognizer(Recognizer):

    def __init__(self, eventNode, coordSysNode=None, eventSource=avg.TOUCH | avg.MOUSE, 
            startHandler=None, moveHandler=None, upHandler=None, stopHandler=None, 
            initialEvent=None, friction=-1):
        if coordSysNode != None:
            self.__coordSysNode = coordSysNode
        else:
            self.__coordSysNode = eventNode
        self.__startHandler = optionalCallback(startHandler, lambda event:None)
        self.__moveHandler = optionalCallback(moveHandler, lambda event,offset:None)
        self.__stopHandler = optionalCallback(stopHandler, lambda:None)
        self.__upHandler = optionalCallback(upHandler, lambda event,offset:None)
        self.__friction = friction

        self.__stateMachine = statemachine.StateMachine("DragRecognizer", "IDLE")
        self.__stateMachine.addState("IDLE", ("DRAG",))
        if friction == -1:
            self.__stateMachine.addState("DRAG", ("IDLE",))
        else:
            self.__stateMachine.addState("DRAG", ("SLIDE",))
            self.__stateMachine.addState("SLIDE", ("IDLE", "DRAG"))

        self.__inertiaHandler = None
        Recognizer.__init__(self, eventNode, eventSource, 1, initialEvent)

    def abortInertia(self):
        if self.__stateMachine.state == "SLIDE":
            self.__inertiaHandler.abort()

    def _handleDown(self, event):
        self.__stateMachine.changeState("DRAG")
        if self.__inertiaHandler:
            self.__inertiaHandler.abort()
        pos = self.__relEventPos(event)
        if self.__friction != -1:
            self.__inertiaHandler = InertiaHandler(self.__friction, self.__onInertiaMove,
                    self.__onInertiaStop)
        self.__dragStartPos = pos
        self.__lastPos = pos
        self.__startHandler(event)

    def _handleMove(self, event):
        pos = self.__relEventPos(event)
        offset = pos - self.__dragStartPos
        self.__moveHandler(event, offset)
        if self.__friction != -1:
            self.__inertiaHandler.onDrag(Transform(pos - self.__lastPos))
        self.__lastPos = pos

    def _handleUp(self, event):
        pos = self.__relEventPos(event)
        self.__offset = pos - self.__dragStartPos
        self.__upHandler(event, self.__offset)
        if self.__friction != -1:
            self.__stateMachine.changeState("SLIDE")
            self.__inertiaHandler.onDrag(Transform(pos - self.__lastPos))
            self.__inertiaHandler.onUp()
        else:
            self.__stateMachine.changeState("IDLE")
            self.__stopHandler()

    def __onInertiaMove(self, transform):
        self.__offset += transform.trans 
        if self.__moveHandler:
            self.__moveHandler(None, self.__offset)
   
    def __onInertiaStop(self):
        self.__stateMachine.changeState("IDLE")
        self.__stop()

    def __stop(self):
        self.__stopHandler()
        self.__inertiaHandler = None

    def __relEventPos(self, event):
        return self.__coordSysNode.getParent().getRelPos(event.pos)


class Mat3x3:
    # Internal class. Will be removed again.

    def __init__(self, row0=(1,0,0), row1=(0,1,0), row2=(0,0,1)):
        self.m = [row0, row1, row2]

    @classmethod
    def translate(cls, t):
        return Mat3x3([1, 0, t[0]],
                      [0, 1, t[1]])

    @classmethod
    def rotate(cls, a):
        return Mat3x3([cos(a), -sin(a), 0],
                      [sin(a), cos(a), 0])

    @classmethod
    def pivotRotate(cls, t, a):
        rot = Mat3x3.rotate(a)
        trans = Mat3x3.translate(t)
        return trans.applyMat(rot.applyMat(trans.inverse()))

    @classmethod
    def scale(cls, s):
        return Mat3x3([s[0], 0, 0],
                      [0, s[1], 0])

    @classmethod
    def fromNode(cls, node):
        return Mat3x3.translate(node.pos).applyMat(
                Mat3x3.translate(node.pivot).applyMat(
                Mat3x3.rotate(node.angle).applyMat(
                Mat3x3.translate(-node.pivot).applyMat(
                Mat3x3.scale(node.size)))))
      
    def setNodeTransform(self, node):
        v = self.applyVec([1,0,0])
        rot = avg.Point2D(v[0], v[1]).getAngle()
        node.angle = rot
        node.size = self.getScale()
        node.pivot = node.size/2 
        v = self.applyVec([0,0,1])
        node.pos = (avg.Point2D(v[0], v[1]) + (node.pivot).getRotated(node.angle) - 
                node.pivot)

    def getScale(self):
        v = self.applyVec([1,0,0])
        xscale = avg.Point2D(v[0], v[1]).getNorm()
        v = self.applyVec([0,1,0])
        yscale = avg.Point2D(v[0], v[1]).getNorm()
        return avg.Point2D(xscale, yscale)

    def __str__(self):
        return self.m.__str__()

    def applyVec(self, v):
        m = self.m
        v1 = []
        for i in range(3):
            v1.append(m[i][0]*v[0] + m[i][1]*v[1] + m[i][2]*v[2])
        return v1

    def applyMat(self, m1):
        m0 = self.m
        result = Mat3x3() 
        for i in range(3):
            v = []
            for j in range(3):
                v.append(m0[i][0]*m1.m[0][j] + m0[i][1]*m1.m[1][j] + m0[i][2]*m1.m[2][j])
            result.m[i] = v
        return result

    def det(self):
        m = self.m
        return float( m[0][0] * (m[2][2]*m[1][1]-m[2][1]*m[1][2])
                     -m[1][0] * (m[2][2]*m[0][1]-m[2][1]*m[0][2])
                     +m[2][0] * (m[1][2]*m[0][1]-m[1][1]*m[0][2]))

    def scalarMult(self, s):
        m = self.m
        result = Mat3x3()
        for i in range(3):
            v = []
            for j in range(3):
                v.append(m[i][j]*s)
            result.m[i] = v
        return result

    def inverse(self):
        m = self.m
        temp = Mat3x3([  m[2][2]*m[1][1]-m[2][1]*m[1][2],  -(m[2][2]*m[0][1]-m[2][1]*m[0][2]),   m[1][2]*m[0][1]-m[1][1]*m[0][2] ],
                      [-(m[2][2]*m[1][0]-m[2][0]*m[1][2]),   m[2][2]*m[0][0]-m[2][0]*m[0][2] , -(m[1][2]*m[0][0]-m[1][0]*m[0][2])],
                      [  m[2][1]*m[1][0]-m[2][0]*m[1][1],  -(m[2][1]*m[0][0]-m[2][0]*m[0][1]),   m[1][1]*m[0][0]-m[1][0]*m[0][1] ])
        return temp.scalarMult(1/self.det())


def getCentroid(indexes, pts):
    c = avg.Point2D(0, 0)
    for i in indexes:
        c += pts[i]
    return c/len(indexes)

def calcKMeans(pts):
    
    # in: List of points
    # out: Two lists, each containing indexes into the input list
    assert(len(pts) > 1)
    done = False
    p1 = pts[0]
    p2 = pts[1]
    oldP1 = None
    oldP2 = None
    while not(p1 == oldP1 and p2 == oldP2):
        l1 = []
        l2 = []
        # Group points
        for i, pt in enumerate(pts):
            dist1 = (pt-p1).getNorm()
            dist2 = (pt-p2).getNorm()
            if dist1 < dist2:
                l1.append(i)
            else:
                l2.append(i)
        oldP1 = p1
        oldP2 = p2
        p1 = getCentroid(l1, pts)
        p2 = getCentroid(l2, pts)
    return l1, l2
        

class Transform():
    def __init__(self, trans, rot=0, scale=1, pivot=(0,0)):
        self.trans = avg.Point2D(trans)
        self.rot = rot
        self.scale = scale
        self.pivot = avg.Point2D(pivot)

    def moveNode(self, node):
        transMat =  Mat3x3.translate(self.trans)
        rotMat = Mat3x3.rotate(self.rot)
        scaleMat = Mat3x3.scale((self.scale, self.scale))
        pivotMat = Mat3x3.translate(self.pivot)
        invPivotMat = pivotMat.inverse()
        startTransform = Mat3x3.fromNode(node)
        newTransform = pivotMat.applyMat(
                rotMat.applyMat(
                scaleMat.applyMat(
                invPivotMat.applyMat(
                transMat.applyMat(
                startTransform)))))
        newTransform.setNodeTransform(node)

    def __repr__(self):
        return "Transform"+str((self.trans, self.rot, self.scale, self.pivot))


class TransformRecognizer(Recognizer):

    def __init__(self, eventNode, coordSysNode=None, eventSource=avg.TOUCH, 
            startHandler=None, moveHandler=None, upHandler=None, stopHandler=None, 
            initialEvent=None, friction=-1):
        if coordSysNode != None:
            self.__coordSysNode = coordSysNode
        else:
            self.__coordSysNode = eventNode
        self.__startHandler = optionalCallback(startHandler, lambda:None)
        self.__moveHandler = optionalCallback(moveHandler, 
                lambda transform:None)
        self.__upHandler = optionalCallback(upHandler, 
                lambda transform:None)
        self.__stopHandler = optionalCallback(stopHandler, lambda:None)
        self.__friction = friction

        self.__baseTransform = Mat3x3()
        self.__lastPosns = []
        self.__posns = []
        self.__inertiaHandler = None
        Recognizer.__init__(self, eventNode, eventSource, None, initialEvent)

    def _handleDown(self, event):
        numContacts = len(self._contacts)
        self.__newPhase()
        if numContacts == 1:
            self.__startHandler()
            if self.__inertiaHandler:
                self.__inertiaHandler.abort()
            if self.__friction != -1:
                self.__inertiaHandler = InertiaHandler(self.__friction, 
                        self.__onInertiaMove, self.__onInertiaStop)

    def _handleUp(self, event):
        numContacts = len(self._contacts)
        if numContacts == 0:
            contact = event.contact
            transform = Transform(self.__relContactPos(contact) - self.__lastPosns[0])
            self.__upHandler(transform)
            if self.__friction != -1:
                self.__inertiaHandler.onDrag(transform)
                self.__inertiaHandler.onUp()
        elif numContacts == 1:
            self.__newPhase()
        else:
            self.__newPhase()

    def _handleChange(self):
        numContacts = len(self._contacts)
        if numContacts == 1:
            contact = self._contacts.keys()[0]
            transform = Transform(self.__relContactPos(contact) - self.__lastPosns[0])
            if self.__friction != -1:
                self.__inertiaHandler.onDrag(transform)
            self.__moveHandler(transform)
            self.__lastPosns = [self.__relContactPos(contact)]
        else:
            contactPosns = [self.__relContactPos(contact)
                    for contact in self._contacts.keys()]
            if numContacts == 2:
                self.__posns = contactPosns
            else:
                self.__posns = [getCentroid(self.__clusters[i], contactPosns) for
                        i in range(2)]

            startDelta = self.__lastPosns[1]-self.__lastPosns[0]
            curDelta = self.__posns[1]-self.__posns[0]

            pivot = (self.__posns[0]+self.__posns[1])/2

            rot = avg.Point2D.angle(curDelta, startDelta)
            
            if self.__lastPosns[0] == self.__lastPosns[1]:
                scale = 1
            else:
                scale = ((self.__posns[0]-self.__posns[1]).getNorm() / 
                        (self.__lastPosns[0]-self.__lastPosns[1]).getNorm())
            
            trans = ((self.__posns[0]+self.__posns[1])/2 - 
                    (self.__lastPosns[0]+self.__lastPosns[1])/2)
            transform = Transform(trans, rot, scale, pivot)
            if self.__friction != -1:
                self.__inertiaHandler.onDrag(transform)
            self.__moveHandler(transform)
            self.__lastPosns = self.__posns


    def __newPhase(self):
        self.__lastPosns = []
        numContacts = len(self._contacts)
        if numContacts == 1:
            contact = self._contacts.keys()[0]
            self.__lastPosns.append(self.__relContactPos(contact))
        else:
            contactPosns = [self.__relContactPos(contact) 
                    for contact in self._contacts.keys()]
            if numContacts == 2:
                self.__lastPosns = contactPosns
            else:
                self.__clusters = calcKMeans(contactPosns)
                self.__lastPosns = [getCentroid(self.__clusters[i], contactPosns) for
                        i in range(2)]

    def __onInertiaMove(self, transform):
        self.__moveHandler(transform)

    def __onInertiaStop(self):
        self.__stopHandler()
        self.__inertiaHandler = None

    def __relContactPos(self, contact):
        return self.__coordSysNode.getParent().getRelPos(contact.events[-1].pos)


class InertiaHandler():
    def __init__(self, friction, moveHandler, stopHandler):
        self.__friction = friction
        self.__moveHandler = moveHandler
        self.__stopHandler = stopHandler

        self.__transVel = avg.Point2D(0, 0)
        self.__curPivot = avg.Point2D(0, 0)
        self.__angVel = 0
        self.__sizeVel = avg.Point2D(0, 0)
        self.__frameHandlerID = g_Player.setOnFrameHandler(self.__onDragFrame)

    def abort(self):
        self.__stop() 

    def onDrag(self, transform):
        frameDuration = g_Player.getFrameDuration()
        self.__transVel += 0.1*transform.trans/frameDuration
        if transform.pivot != avg.Point2D(0,0):
            self.__curPivot = transform.pivot
        if transform.rot > pi:
            transform.rot -= 2*pi
        self.__angVel += 0.1*transform.rot/frameDuration

    def onUp(self):
        g_Player.clearInterval(self.__frameHandlerID)
        self.__frameHandlerID = g_Player.setOnFrameHandler(self.__onInertiaFrame)

    def __onDragFrame(self):
        self.__transVel *= 0.9
        self.__angVel *= 0.9

    def __onInertiaFrame(self):
        transNorm = self.__transVel.getNorm()
        if transNorm - self.__friction > 0:
            direction = self.__transVel.getNormalized()
            self.__transVel = direction * (transNorm-self.__friction)
            curTrans = self.__transVel * g_Player.getFrameDuration()
        else:
            curTrans = avg.Point2D(0, 0)

        if self.__angVel != 0:
            angSign = self.__angVel/fabs(self.__angVel)
            self.__angVel = self.__angVel - angSign*self.__friction/200
            newAngSign = self.__angVel/fabs(self.__angVel)
            if newAngSign != angSign:
                self.__angVel = 0
        curAng = self.__angVel * g_Player.getFrameDuration()
        self.__curPivot += curTrans

        if transNorm - self.__friction > 0 or self.__angVel != 0:
            if self.__moveHandler:
                self.__moveHandler(Transform(curTrans,  curAng, 1, self.__curPivot))
        else:
            self.__stop()

    def __stop(self):
        g_Player.clearInterval(self.__frameHandlerID)
        self.__stopHandler()
        self.__stopHandler = None
        self.__moveHandler = None
