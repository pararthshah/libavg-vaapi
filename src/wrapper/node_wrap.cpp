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

void export_bitmap();
void export_raster();
void export_event();
#ifndef WIN32
void export_devices();
#endif

#include "WrapHelper.h"

#include "../base/Logger.h"
#include "../base/Exception.h"
#include "../player/Player.h"
#include "../player/AVGNode.h"
#include "../player/DivNode.h"
#include "../player/PanoImage.h"
#include "../player/Sound.h"
#include "../player/LineNode.h"
#include "../player/RectNode.h"
#include "../player/CurveNode.h"
#include "../player/PolyLineNode.h"
#include "../player/PolygonNode.h"
#include "../player/CircleNode.h"

#include <boost/version.hpp>
#include <boost/shared_ptr.hpp>

using namespace boost::python;
using namespace avg;

void export_node()
{
    class_<Node, boost::shared_ptr<Node>, boost::noncopyable>("Node",
            "Base class for all elements in the avg tree.",
            no_init)
        .def(self == self)
        .def(self != self)
        .def("__hash__", &Node::getHash)
        .def("getParent", &Node::getParent,
                "getParent() -> node\n"
                "Returns the container (AVGNode or DivNode) the node is in. For\n"
                "the root node, returns None.\n")
        .def("unlink", &Node::unlink,
                "unlink() -> None\n"
                "Removes a node from it's parent container. Equivalent to\n"
                "node.getParent().removeChild(node.getParent().indexOf(node)).")
        .def("setEventCapture", &Node::setMouseEventCapture,
                "setEventCapture(cursorid)\n"
                "Sets up event capturing so that cursor events are sent to this node\n"
                "regardless of the cursor position. cursorid is optional; if left out,\n"
                "the mouse cursor is captured. If not, events from a specific tracker\n"
                "cursor are captured. If the node doesn't handle the event, it\n"
                "propagates to its parent normally. This function is useful for the\n"
                "implementation of user interface elements such as scroll bars. Only one\n"
                "node can capture a cursor at any one time. Normal operation can\n"
                "be restored by calling releaseEventCapture().\n"
                "@param cursorid: The id of the tracker cursor to capture (optional).\n")
        .def("setEventCapture", &Node::setEventCapture)
        .def("releaseEventCapture", &Node::releaseMouseEventCapture,
                "releaseEventCapture(cursorid)\n"
                "Restores normal mouse operation after a call to setEventCapture().\n"
                "@param cursorid: The id of the tracker cursor to release (optional).\n")
        .def("releaseEventCapture", &Node::releaseEventCapture)
        .def("setEventHandler", &Node::setEventHandler,
                "setEventHandler(type, source, pyfunc)\n"
                "Sets a callback function that is invoked whenever an event of the\n"
                "specified type from the specified source occurs. This function is\n"
                "similar to the event handler node attributes (e.g. oncursordown).\n"
                "It is more specific since it takes the event source as a parameter\n"
                "and allows the use of any python callable as callback function.\n"
                "@param type: One of the event types KEYUP, KEYDOWN, CURSORMOTION,\n"
                "CURSORUP, CURSORDOWN, CURSOROVER, CURSOROUT, RESIZE or QUIT.\n"
                "@param source: MOUSE for mouse events, TOUCH for multitouch touch\n"
                "events, TRACK for multitouch track events or other tracking, \n"
                "NONE for keyboard events. Sources can be or'ed together to set a\n"
                "handler for several sources at once.\n"
                "@param pyfunc: The python callable to invoke.\n")
        .def("getAbsPos", &Node::getAbsPos,
                "getAbsPos(relpos) -> abspos\n"
                "Transforms a position in coordinates relative to the node to a\n"
                "position in window coordinates.\n"
                "@param relpos: Relative coordinate to transform.")
        .def("getRelPos", &Node::getRelPos,
                "getRelPos(abspos) -> relpos\n"
                "Transforms a position in window coordinates to a position\n"
                "in coordinates relative to the node.\n"
                "@param abspos: Absolute coordinate to transform.")
        .add_property("id", make_function(&Node::getID,
                return_value_policy<copy_const_reference>()), &Node::setID,
                "A unique identifier that can be used to reference the node.\n")
        .add_property("active", &Node::getActive, &Node::setActive,
                      "If this attribute is true, the node behaves as usual. If not, it\n"
                      "is neither drawn nor does it react to events. Videos are paused.\n")
        .add_property("sensitive", &Node::getSensitive, &Node::setSensitive,
                      "A node only reacts to events if sensitive is true.")
        .add_property("opacity", &Node::getOpacity, &Node::setOpacity,
                      "A measure of the node's transparency. 0.0 is completely\n"
                      "transparent, 1.0 is completely opaque. Opacity is relative to\n"
                      "the parent node's opacity.\n");

    class_<AreaNode, boost::shared_ptr<AreaNode>, bases<Node>, boost::noncopyable>(
            "AreaNode", 
            "Base class for elements in the avg tree that define an area on the screen.\n"
            "Is responsible for coordinate transformations and event handling.\n",
            no_init)
        .def("getElementByPos", &AreaNode::getElementByPos,
                "getElementByPos(pos) -> AreaNode\n"
                "Returns the topmost child node that is at the position given. pos\n"
                "is in coordinates relative to the called node. The algorithm used\n"
                "is the same as the cursor hit test algorithm used for events.\n")
        .def("getMediaSize", &AreaNode::getMediaSize,
                "getMediaSize() -> mediasize\n"
                "Returns the size in pixels of the media in the node. Image nodes\n"
                "return the bitmap size, Camera nodes\n"
                "the size of a camera frame and Words nodes the amount of space\n"
                "the text takes. Video nodes the video size if decoding has started\n"
                "or (0,0) if not.")
        .def("setFragmentShader", &AreaNode::setFragmentShaderProgram,
                "setFragmentShader(sProgram)\n"
                "Replace the fragment shader in use for this node and all its decendants"
                "The argument specifies the source of the program in the OpenGL Shader Language")
        .def("setVertexShader", &AreaNode::setVertexShaderProgram,
                "setVertexShader(sProgram)\n"
                "Replace the vertex shader in use for this node and all its decendants"
                "The argument specifies the source of the program in the OpenGL Shader Language")
        .add_property("x", &AreaNode::getX, &AreaNode::setX,
                "The position of the node's left edge relative to it's parent node.\n")
        .add_property("y", &AreaNode::getY, &AreaNode::setY,
                "The position of the node's top edge relative to it's parent node.\n")
        .add_property("pos", make_function(&AreaNode::getPos,
                return_value_policy<copy_const_reference>()), &AreaNode::setPos,
                "The position of the node's top left corner relative to it's parent node.\n")
        .add_property("width", &AreaNode::getWidth, &AreaNode::setWidth)
        .add_property("height", &AreaNode::getHeight, &AreaNode::setHeight)
        .add_property("angle", &AreaNode::getAngle, &AreaNode::setAngle,
                "The angle that the node is rotated to in radians. 0 is\n"
                "unchanged, 3.14 is upside-down.\n")
        .add_property("size", &AreaNode::getSize, &AreaNode::setSize)
        .add_property("pivotx", &AreaNode::getPivotX, &AreaNode::setPivotX,
                "x coordinate of the point that the node is rotated around.\n"
                "Default is the center of the node.\n")
        .add_property("pivoty", &AreaNode::getPivotY, &AreaNode::setPivotY,
                "y coordinate of the point that the node is rotated around.\n"
                "Default is the center of the node.\n")
        .add_property("pivot",  &AreaNode::getPivot, &AreaNode::setPivot,
                "The position of the point that the node is rotated around.\n"
                "Default is the center of the node.\n")
    ;

    export_bitmap();
    export_raster();
  
    class_<DivNode, bases<AreaNode>, boost::noncopyable>("DivNode", 
            "A div node is a node that groups other nodes logically and visually.\n"
            "Its upper left corner is used as point of origin for the coordinates\n"
            "of its child nodes. Its extents are used to clip the children. Its\n"
            "opacity is used as base opacity for the child nodes' opacities.\n"
            "The children of a div node are drawn in the order they are found\n"
            "in the avg file.",
            no_init)
        .add_property("crop", &DivNode::getCrop, &DivNode::setCrop,
                "Turns clipping on or off. Default is True.\n")
        .add_property("elementoutlinecolor",
                make_function(&DivNode::getElementOutlineColor,
                        return_value_policy<copy_const_reference>()),
                make_function(&DivNode::setElementOutlineColor,
                        return_value_policy<copy_const_reference>()),
                "Allows debugging of div node nesting by rendering the outlines of\n"
                "this div and all its div children in the specified color. Turn off\n"
                "by setting the color to \"\".\n")
        .def("getNumChildren", &DivNode::getNumChildren,
                "getNumChildren() -> numchildren\n"
                "Returns the number of immediate children that this div contains.")
        .def("getChild", make_function(&DivNode::getChild,
                return_value_policy<copy_const_reference>()),
                "getChild(pos) -> node\n"
                "Returns the child at position pos.")
        .def("appendChild", &DivNode::appendChild,
                "appendChild(node)\n"
                "Adds a new child to the container behind the last existing child.")
        .def("insertChildBefore", &DivNode::insertChildBefore,
                "insertChildBefore(newNode, oldChild)\n"
                "Adds a new child to the container in front of the existing node oldChild.")
        .def("insertChild", &DivNode::insertChild,
                "insertChild(node, pos)\n"
                "Adds a new child to the container at position pos.")
        .def("removeChild", (void (DivNode::*)(NodePtr))(&DivNode::removeChild),
                "removeChild(node)\n"
                "Removes the child given by pNode.")
        .def("removeChild", (void (DivNode::*)(unsigned))(&DivNode::removeChild),
                "removeChild(pos)\n"
                "Removes the child at index pos.")
        .def("reorderChild", (void (DivNode::*)(unsigned, unsigned))(&DivNode::reorderChild),
                "reorderChild(oldPos, newPos)\n"
                "Moves the child at index pos so it's at index newPos. This function\n"
                "can be used to change the order in which the children are drawn.")
        .def("reorderChild", (void (DivNode::*)(NodePtr, unsigned))(&DivNode::reorderChild),
                "reorderChild(node, newPos)\n"
                "Moves the child node so it's at index newPos. This function\n"
                "can be used to change the order in which the children are drawn.")
        .def("indexOf", &DivNode::indexOf,
                "indexOf(childnode)\n"
                "Returns the index of the child given or -1 if childnode isn't a\n"
                "child of the container. This function does a linear search through\n"
                "the list of children until the child is found.")
        .add_property("mediadir", make_function(&DivNode::getMediaDir,
                return_value_policy<copy_const_reference>()), &DivNode::setMediaDir,
                "The directory that the media files for the children of this node are in.\n")
    ;

    class_<AVGNode, bases<DivNode> >("AVGNode",
            "Root node of any avg tree. Defines the properties of the display and\n"
            "handles key press events. The AVGNode's width and height define the\n"
            "coordinate system for the display and are the default for the window\n"
            "size used (i.e. by default, the coordinate system is pixel-based.)\n",
            no_init)
        .def("getCropSetting", &AVGNode::getCropSetting,
                "getCropSetting() -> isCropActive\n\n"
                "Returns true if cropping is active. Cropping can be turned off globally\n"
                "in the avg file. (Deprecated. This attribute is only nessesary because\n"
                "of certain buggy display drivers that don't work with cropping.)")
    ;

    class_<Sound, bases<AreaNode> >("Sound",
            "A sound played from a file.\n",
            no_init)
        .def("play", &Sound::play,
                "play()\n"
                "Starts audio playback.")
        .def("stop", &Sound::stop,
                "stop()\n"
                "Stops audio playback. Closes the object and 'rewinds' the playback\n"
                "cursor.")
        .def("pause", &Sound::pause,
                "pause()\n"
                "Stops audio playback but doesn't close the object. The playback\n"
                "cursor stays at the same position.")
        .def("setEOFCallback", &Sound::setEOFCallback,
                "setEOFCallback(pyfunc)\n"
                "Sets a python callable to be invoked when the video reaches end of file.")
        .add_property("href", make_function(&Sound::getHRef, 
                return_value_policy<copy_const_reference>()), &Sound::setHRef,
                "The source filename of the sound.\n")
        .add_property("loop", &Sound::getLoop,
                "Whether to start the sound again when it has ended (ro).\n")
        .add_property("duration", &Sound::getDuration,
                "The duration of the sound file in milliseconds (ro).\n")
        .add_property("volume", &Sound::getVolume, &Sound::setVolume,
                "Audio playback volume for this sound. 0 is silence, 1 passes media\n"
                "file volume through unchanged. Values higher than 1 can be used to\n"
                "amplify sound if the sound file doesn't use the complete dynamic\n"
                "range.\n")
    ;

    class_<PanoImage, bases<AreaNode> >("PanoImage",
            "A panorama image displayed in cylindrical projection.\n",
            no_init)
        .def("getScreenPosFromPanoPos", &PanoImage::getScreenPosFromPanoPos,
                "getScreenPosFromPanoPos(panoPos) -> pos\n"
                "Converts a position in panorama image pixels to pixels in coordinates\n"
                "relative to the node, taking into account the current rotation angle.\n")
        .def("getScreenPosFromAngle", &PanoImage::getScreenPosFromAngle,
                "getScreenPosFromAngle(angle) -> pos\n"
                "Converts panorama angle to pixels in coordinates\n"
                "relative to the node, taking into account the current rotation angle.\n")
        .add_property("href", make_function(&PanoImage::getHRef, 
                return_value_policy<copy_const_reference>()), &PanoImage::setHRef,
                "The source filename of the image.\n")
        .add_property("sensorwidth", &PanoImage::getSensorWidth, 
                &PanoImage::setSensorWidth,
                "The width of the sensor used to make the image. This value\n"
                "is used together with sensorheight and focallength to\n"
                "determine the projection to use.\n")
        .add_property("sensorheight", &PanoImage::getSensorHeight, 
                &PanoImage::setSensorHeight,
                "The height of the sensor used to make the image.\n")
        .add_property("focallength", &PanoImage::getFocalLength, 
                &PanoImage::setFocalLength,
                "The focal length of the lens in millimeters.\n")
        .add_property("rotation", &PanoImage::getRotation, &PanoImage::setRotation,
                "The current angle the viewer is looking at in radians.\n")
        .add_property("maxrotation", &PanoImage::getMaxRotation,
                "The maximum angle the viewer can look at.\n")
    ;

    class_<VectorNode, bases<Node>, boost::noncopyable>("VectorNode", 
            no_init)
        .add_property("strokewidth", &VectorNode::getStrokeWidth, 
                &VectorNode::setStrokeWidth,
                "The width of the strokes in the vector. For lines, this is the line\n"
                "width. For rectangles, it is the width of the outline, etc.\n")
        .add_property("color", make_function(&VectorNode::getColor,
                return_value_policy<copy_const_reference>()), &VectorNode::setColor,
                "The color of the strokes in standard html color notation:\n" 
                "FF0000 is red, 00FF00 green, etc.\n")
        .add_property("texhref", make_function(&VectorNode::getTexHRef,
                return_value_policy<copy_const_reference>()), &VectorNode::setTexHRef,
                "An image file to use as a texture for the node.\n")
        .add_property("blendmode", 
                make_function(&VectorNode::getBlendModeStr, 
                        return_value_policy<copy_const_reference>()),
                &VectorNode::setBlendModeStr,
                "The method of compositing the node with the nodes under\n"
                "it. Valid values are 'blend', 'add', 'min' and 'max'.\n")
        .def("setBitmap", &VectorNode::setBitmap, 
                "setBitmap(bitmap)\n"
                "Sets a bitmap to use as a texture. Sets texhref to an empty string.\n"
                "@param bitmap: A libavg bitmap object to use.")
    ;

    class_<FilledVectorNode, bases<VectorNode>, boost::noncopyable>("FilledVectorNode", 
            no_init)
        .add_property("filltexhref", make_function(&FilledVectorNode::getFillTexHRef,
                return_value_policy<copy_const_reference>()), 
                &FilledVectorNode::setFillTexHRef,
                "An image file to use as a texture for the area of the node.\n")
        .add_property("fillcolor", make_function(&FilledVectorNode::getFillColor,
                return_value_policy<copy_const_reference>()), 
                &FilledVectorNode::setFillColor)
        .add_property("fillopacity", &FilledVectorNode::getFillOpacity, 
                &FilledVectorNode::setFillOpacity)
        .add_property("filltexcoord1", make_function(&FilledVectorNode::getFillTexCoord1,
               return_value_policy<copy_const_reference>()), 
               &FilledVectorNode::setFillTexCoord1)
        .add_property("filltexcoord2", make_function(&FilledVectorNode::getFillTexCoord2,
               return_value_policy<copy_const_reference>()), 
               &FilledVectorNode::setFillTexCoord2)
        .def("setFillBitmap", &FilledVectorNode::setFillBitmap, 
                "setFillBitmap(bitmap)\n"
                "Sets a bitmap to use as a fill texture. Sets filltexhref to an empty\n"
                "string.\n"
                "@param bitmap: A libavg bitmap object to use.")
    ;

    class_<LineNode, bases<VectorNode>, boost::noncopyable>("LineNode", 
            no_init)
        .add_property("x1", &LineNode::getX1, &LineNode::setX1)
        .add_property("y1", &LineNode::getY1, &LineNode::setY1)
        .add_property("pos1", make_function(&LineNode::getPos1,
               return_value_policy<copy_const_reference>()), &LineNode::setPos1)
        .add_property("x2", &LineNode::getX2, &LineNode::setX2)
        .add_property("y2", &LineNode::getY2, &LineNode::setY2)
        .add_property("pos2", make_function(&LineNode::getPos2,
               return_value_policy<copy_const_reference>()), &LineNode::setPos2)
        .add_property("texcoord1", &LineNode::getTexCoord1, &LineNode::setTexCoord1)
        .add_property("texcoord2", &LineNode::getTexCoord2, &LineNode::setTexCoord2)
    ;

    class_<RectNode, bases<FilledVectorNode>, boost::noncopyable>("RectNode", 
            no_init)
        .add_property("x", &RectNode::getX, &RectNode::setX)
        .add_property("y", &RectNode::getY, &RectNode::setY)
        .add_property("pos", make_function(&RectNode::getPos,
                return_value_policy<copy_const_reference>()), &RectNode::setPos)
        .add_property("width", &RectNode::getWidth, &RectNode::setWidth)
        .add_property("height", &RectNode::getHeight, &RectNode::setHeight)
        .add_property("size", &RectNode::getSize, &RectNode::setSize)
        .add_property("texcoords", make_function(&RectNode::getTexCoords, 
                return_value_policy<copy_const_reference>()), &RectNode::setTexCoords)
        .add_property("angle", &RectNode::getAngle, &RectNode::setAngle,
                "The angle that the rectangle is rotated to in radians. 0 is\n"
                "unchanged, 3.14 is upside-down. The rectangle is rotated around it's\n"
                "center\n")
    ;
    
    class_<CurveNode, bases<VectorNode>, boost::noncopyable>("CurveNode", 
            no_init)
        .add_property("x1", &CurveNode::getX1, &CurveNode::setX1)
        .add_property("y1", &CurveNode::getY1, &CurveNode::setY1)
        .add_property("pos1", make_function(&CurveNode::getPos1,
               return_value_policy<copy_const_reference>()), &CurveNode::setPos1)
        .add_property("x2", &CurveNode::getX2, &CurveNode::setX2)
        .add_property("y2", &CurveNode::getY2, &CurveNode::setY2)
        .add_property("pos2", make_function(&CurveNode::getPos2,
               return_value_policy<copy_const_reference>()), &CurveNode::setPos2)
        .add_property("x3", &CurveNode::getX3, &CurveNode::setX3)
        .add_property("y3", &CurveNode::getY3, &CurveNode::setY3)
        .add_property("pos3", make_function(&CurveNode::getPos3,
               return_value_policy<copy_const_reference>()), &CurveNode::setPos3)
        .add_property("x4", &CurveNode::getX4, &CurveNode::setX4)
        .add_property("y4", &CurveNode::getY4, &CurveNode::setY4)
        .add_property("pos4", make_function(&CurveNode::getPos4,
               return_value_policy<copy_const_reference>()), &CurveNode::setPos4)
        .add_property("texcoord1", &CurveNode::getTexCoord1, &CurveNode::setTexCoord1)
        .add_property("texcoord2", &CurveNode::getTexCoord2, &CurveNode::setTexCoord2)
    ;

    class_<PolyLineNode, bases<VectorNode>, boost::noncopyable>("PolyLineNode", no_init)
        .add_property("pos", make_function(&PolyLineNode::getPos, 
                return_value_policy<copy_const_reference>()), &PolyLineNode::setPos)
        .add_property("texcoords", make_function(&PolyLineNode::getTexCoords, 
                return_value_policy<copy_const_reference>()), &PolyLineNode::setTexCoords)
        .add_property("linejoin", &PolyLineNode::getLineJoin, &PolyLineNode::setLineJoin)
    ;

    class_<PolygonNode, bases<FilledVectorNode>, boost::noncopyable>("PolygonNode", 
            no_init)
        .add_property("pos", make_function(&PolygonNode::getPos, 
                return_value_policy<copy_const_reference>()), &PolygonNode::setPos)
        .add_property("texcoords", make_function(&PolygonNode::getTexCoords, 
                return_value_policy<copy_const_reference>()), &PolygonNode::setTexCoords)
        .add_property("linejoin", &PolygonNode::getLineJoin, &PolygonNode::setLineJoin)
    ;

    class_<CircleNode, bases<FilledVectorNode>, boost::noncopyable>("CircleNode", 
            no_init)
        .add_property("x", &CircleNode::getX, &CircleNode::setX)
        .add_property("y", &CircleNode::getY, &CircleNode::setY)
        .add_property("pos", make_function(&CircleNode::getPos,
               return_value_policy<copy_const_reference>()), &CircleNode::setPos)
        .add_property("r", &CircleNode::getR, &CircleNode::setR)
        .add_property("texcoord1", &CircleNode::getTexCoord1, &CircleNode::setTexCoord1)
        .add_property("texcoord2", &CircleNode::getTexCoord2, &CircleNode::setTexCoord2)
    ;
}
