//
// $Id$
// 

#include "../avgconfig.h"

#include "RasterNode.h"

#ifdef AVG_ENABLE_GL
#include "OGLSurface.h"
#endif
#include "MathHelper.h"
#include "IDisplayEngine.h"

#include "../base/Logger.h"
#include "../base/XMLHelper.h"

using namespace std;

namespace avg {

RasterNode::RasterNode ()
    : m_pSurface(0),
      m_Angle(0),
      m_Pivot(-32767, -32767),
      m_MaxTileSize(IntPoint(-1,-1)),
      m_sBlendMode("blend")
{
}

RasterNode::RasterNode (const xmlNodePtr xmlNode, Container * pParent)
    : Node(xmlNode, pParent),
      m_pSurface(0),
      m_Angle(0),
      m_Pivot(-32767, -32767),
      m_MaxTileSize(IntPoint(-1,-1)),
      m_sBlendMode("blend")
{
    m_Angle = getDefaultedDoubleAttr (xmlNode, "angle", 0);
    m_Pivot.x = getDefaultedDoubleAttr (xmlNode, "pivotx", -32767);
    m_Pivot.y = getDefaultedDoubleAttr (xmlNode, "pivoty", -32767);
    m_MaxTileSize.x = getDefaultedIntAttr (xmlNode, "maxtilewidth", -1);
    m_MaxTileSize.y = getDefaultedIntAttr (xmlNode, "maxtileheight", -1);
    setBlendModeStr(getDefaultedStringAttr (xmlNode, "blendmode", "blend"));
}

RasterNode::~RasterNode()
{
    delete m_pSurface;
}

void RasterNode::initVisible()
{
    Node::initVisible();

    m_bHasCustomPivot = ((m_Pivot.x != -32767) && (m_Pivot.y != -32767));

    if (m_MaxTileSize != IntPoint(-1, -1)) {
#ifdef AVG_ENABLE_GL        
        OGLSurface * pOGLSurface = 
            dynamic_cast<OGLSurface*>(m_pSurface);
        if (!pOGLSurface) {
            AVG_TRACE(Logger::WARNING, 
                    "Node "+getID()+":"
                    "Custom tile sizes are only allowed when "
                    "the display engine is OpenGL. Ignoring.");
        } else {
            pOGLSurface->setMaxTileSize(m_MaxTileSize);
        }
#else
            AVG_TRACE(Logger::WARNING, 
                    "Node "+getID()+":"
                    "Custom tile sizes are only allowed when "
                    "the display engine is OpenGL. Ignoring.");
#endif        
    }
    setBlendModeStr(m_sBlendMode);
}

int RasterNode::getNumVerticesX()
{
#ifdef AVG_ENABLE_GL
    OGLSurface * pOGLSurface = getOGLSurface();
    return pOGLSurface->getNumVerticesX(); 
#else
    return 1;
#endif
}

int RasterNode::getNumVerticesY()
{
#ifdef AVG_ENABLE_GL
    OGLSurface * pOGLSurface = getOGLSurface();
    return pOGLSurface->getNumVerticesY(); 
#else
    return 1;
#endif
}

DPoint RasterNode::getOrigVertexCoord(int x, int y)
{
#ifdef AVG_ENABLE_GL
    OGLSurface * pOGLSurface = getOGLSurface();
    return pOGLSurface->getOrigVertexCoord(x, y);
#else
    return DPoint(0,0);
#endif
}

DPoint RasterNode::getWarpedVertexCoord(int x, int y) 
{
#ifdef AVG_ENABLE_GL
    OGLSurface * pOGLSurface = getOGLSurface();
    return pOGLSurface->getWarpedVertexCoord(x, y);
#else
    return DPoint(0,0);
#endif
}

void RasterNode::setWarpedVertexCoord(int x, int y, const DPoint& Vertex)
{
#ifdef AVG_ENABLE_GL
    OGLSurface * pOGLSurface = getOGLSurface();
    pOGLSurface->setWarpedVertexCoord(x, y, Vertex);
#endif
}

double RasterNode::getAngle() const
{
    return m_Angle;
}

void RasterNode::setAngle(double Angle)
{
    m_Angle = fmod(Angle, 2*PI);
    invalidate();
}

double RasterNode::getPivotX() const
{
    return m_Pivot.x;
}

void RasterNode::setPivotX(double Pivotx)
{
    m_Pivot = getPivot();
    m_Pivot.x = Pivotx;
    m_bHasCustomPivot = true;
}

double RasterNode::getPivotY() const
{
    return m_Pivot.y;
}

void RasterNode::setPivotY(double Pivoty)
{
    m_Pivot = getPivot();
    m_Pivot.y = Pivoty;
    m_bHasCustomPivot = true;
}

const std::string& RasterNode::getBlendModeStr() const
{
    return m_sBlendMode;
}

void RasterNode::setBlendModeStr(const std::string& sBlendMode)
{
    m_sBlendMode = sBlendMode;
    if (m_sBlendMode == "blend") {
        m_BlendMode = IDisplayEngine::BLEND_BLEND;
    } else if (m_sBlendMode == "add") {
        m_BlendMode = IDisplayEngine::BLEND_ADD;
    } else if (m_sBlendMode == "min") {
        m_BlendMode = IDisplayEngine::BLEND_MIN;
    } else if (m_sBlendMode == "max") {
        m_BlendMode = IDisplayEngine::BLEND_MAX;
    } else {
        // TODO: throw exception here
    }
}

Node * RasterNode::getElementByPos (const DPoint & pos)
{
    // Node isn't pickable if it's tilted or warped.
    if (fabs(m_Angle)<0.0001 && m_MaxTileSize == IntPoint(-1, -1)) {
        return Node::getElementByPos(pos);
    } else {
        return 0;
    }
}

DPoint RasterNode::getPivot()
{
    if (m_bHasCustomPivot) {
        return m_Pivot;
    } else {
        const DRect& vpt = getRelViewport();
        return DPoint (vpt.Width()/2, vpt.Height()/2);
    }
}

#ifdef AVG_ENABLE_GL
OGLSurface * RasterNode::getOGLSurface()
{
    OGLSurface * pOGLSurface = dynamic_cast<OGLSurface *>(m_pSurface);
    if (pOGLSurface) {
        return pOGLSurface; 
    } else {
        AVG_TRACE(Logger::ERROR, 
                "OpenGL display engine needed for node " << getID() <<
                ". Aborting.");
        exit(-1);
    }
}
#endif

IDisplayEngine::BlendMode RasterNode::getBlendMode() const
{
    return m_BlendMode;
}

ISurface * RasterNode::getSurface()
{
    if (!m_pSurface) {
        m_pSurface = getEngine()->createSurface();
    }
    return m_pSurface;
}

string RasterNode::getTypeStr ()
{
    return "RasterNode";
}

}

