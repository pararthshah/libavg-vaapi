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

#include "MeshNode.h"

#include "../wrapper/WrapHelper.h"
#include "../base/Logger.h"
#include "../base/Exception.h"

#include "NodeDefinition.h"
#include "VectorNode.h"
#include "Player.h"

#include <cstdlib>
#include <string>
#include <iostream>

using namespace std;
using namespace boost::python;

namespace avg {

NodeDefinition MeshNode::createDefinition()
{
    vector<DPoint> vVert;
    vector<DPoint> vTex;
    vector<IntTriple> vTriangle;

    return NodeDefinition("mesh", (NodeBuilder)MeshNode::buildNode<MeshNode>)
        .extendDefinition(VectorNode::createDefinition())
        .addArg(Arg<vector<DPoint> >("vertexcoords", vVert, false, offsetof(MeshNode, m_VertexCoords)))
        .addArg(Arg<vector<DPoint> >("texcoords", vTex, false, offsetof(MeshNode, m_TexCoords)))
        .addArg(Arg<vector<IntTriple> >("triangles", vTriangle, false, offsetof(MeshNode, m_Triangles)))
        ;
}

MeshNode::MeshNode(const ArgList& Args)
    : VectorNode(Args)
{
    Args.setMembers(this);
    isValid(m_TexCoords);
}

MeshNode::~MeshNode()
{
}

void MeshNode::isValid(const vector<DPoint>& coords)
{
    if (coords.size() != m_VertexCoords.size()) {
        throw(Exception(AVG_ERR_OUT_OF_RANGE,
                "Coordinates Out of Range"));
    }
}

const vector<DPoint>& MeshNode::getVertexCoords() const
{
    return m_VertexCoords;
}

void MeshNode::setVertexCoords(const vector<DPoint>& coords)
{
    isValid(coords);
    m_VertexCoords = coords;
    setDrawNeeded(true);
}

const vector<DPoint>& MeshNode::getTexCoords() const
{
    return m_TexCoords;
}

void MeshNode::setTexCoords(const vector<DPoint>& coords)
{
    isValid(coords);
    m_TexCoords = coords;
    setDrawNeeded(true);
}

const vector<IntTriple>& MeshNode::getTriangles() const
{
    return m_Triangles; 
}


void MeshNode::setTriangles(const vector<IntTriple>& triangles)
{
    for (unsigned int i = 0; i < triangles.size(); i++) {
        
        if( static_cast<unsigned int>(triangles[i].x) < 0 ||
            static_cast<unsigned int>(triangles[i].y) < 0 || 
            static_cast<unsigned int>(triangles[i].z) < 0){
            throw(Exception(AVG_ERR_OUT_OF_RANGE,
                "Triangle Index Out of Range < 0"));
        }
        
        if( static_cast<unsigned int>(triangles[i].x) > m_VertexCoords.size() || 
            static_cast<unsigned int>(triangles[i].y) > m_VertexCoords.size() ||
            static_cast<unsigned int>(triangles[i].z) > m_VertexCoords.size()){
            throw(Exception(AVG_ERR_OUT_OF_RANGE,
                "Triangle Index Out of Range > max triangles"));
        }
    }   
    m_Triangles = triangles;
    setDrawNeeded(true);
}

int MeshNode::getNumVertexes()
{
    return m_VertexCoords.size();
}

int MeshNode::getNumIndexes()
{
    int numTriangles = m_Triangles.size()*3;
    return numTriangles;
}

void MeshNode::calcVertexes(VertexArrayPtr& pVertexArray, Pixel32 color)
{
    for (unsigned int i = 0; i < m_VertexCoords.size(); i++) {
        pVertexArray->appendPos(m_VertexCoords[i],m_TexCoords[i], color);
    }

    for (unsigned int i = 0; i < m_Triangles.size(); i++) {
        pVertexArray->appendTriIndexes(m_Triangles[i].x, m_Triangles[i].y, m_Triangles[i].z);
    }
}

}