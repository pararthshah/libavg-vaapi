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

#include "OGLSurface.h"

#include "../base/MathHelper.h"
#include "../base/Logger.h"
#include "../base/Exception.h"
#include "../base/ScopeTimer.h"
#include "../base/ObjectCounter.h"

#include "../graphics/ShaderRegistry.h"
#include "../graphics/GLContext.h"

#include <iostream>
#include <sstream>

using namespace std;

#define COLORSPACE_SHADER "COLORSPACE"

static float yuvCoeff[3][4] = 
{
    {1.0f,   0.0f,   1.40f,  0.0f},
    {1.0f, -0.34f,  -0.71f,  0.0f},
    {1.0f,  1.77f,    0.0f,  0.0f},
};

namespace avg {

OGLSurface::OGLSurface(const MaterialInfo& material)
    : m_Size(-1,-1),
      m_bUseForeignTexture(false),
      m_Material(material),
      m_Gamma(1,1,1),
      m_Brightness(1,1,1),
      m_Contrast(1,1,1),
      m_bIsDirty(true)
{
    ObjectCounter::get()->incRef(&typeid(*this));
}

OGLSurface::~OGLSurface()
{
    ObjectCounter::get()->decRef(&typeid(*this));
}

void OGLSurface::attach()
{
    m_MemoryMode = GLContext::getCurrent()->getMemoryModeSupported();
    if (!GLContext::getCurrent()->isUsingShaders()) {
        if (m_Material.getHasMask()) {
            throw Exception(AVG_ERR_VIDEO_GENERAL,
                    "Can't set mask bitmap since shader support is disabled.");
        }
        if (gammaIsModified() || colorIsModified()) {
            throw Exception(AVG_ERR_VIDEO_GENERAL,
                    "Can't use color correction (gamma, brightness, contrast) since shader support is disabled.");
        }
    }
}

void OGLSurface::create(const IntPoint& size, PixelFormat pf)
{
    AVG_ASSERT(GLContext::getCurrent());
    if (m_pTextures[0] && m_Size == size && m_pf == pf) {
        // If nothing's changed, we can ignore everything.
        return;
    }
    m_Size = size;
    m_pf = pf;

    if (pixelFormatIsPlanar(m_pf)) {
        m_pTextures[0] = PBOTexturePtr(new PBOTexture(size, I8, m_Material,
                m_MemoryMode));
        IntPoint halfSize(size.x/2, size.y/2);
        m_pTextures[1] = PBOTexturePtr(new PBOTexture(halfSize, I8, m_Material,
                m_MemoryMode));
        m_pTextures[2] = PBOTexturePtr(new PBOTexture(halfSize, I8, m_Material,
                m_MemoryMode));
        if (pixelFormatHasAlpha(m_pf)) {
            m_pTextures[3] = PBOTexturePtr(new PBOTexture(size, I8, m_Material,
                m_MemoryMode));
        }
    } else {
        m_pTextures[0] = PBOTexturePtr(new PBOTexture(size, m_pf, m_Material,
                m_MemoryMode));
    }
    m_bUseForeignTexture = false;
    m_bIsDirty = true;
}

void OGLSurface::createMask(const IntPoint& size)
{
    AVG_ASSERT(m_Material.getHasMask());
    m_pMaskTexture = PBOTexturePtr(new PBOTexture(size, I8, m_Material, m_MemoryMode));
    m_bIsDirty = true;
}

void OGLSurface::destroy()
{
    m_bUseForeignTexture = false;
    m_pTextures[0] = PBOTexturePtr();
    m_pTextures[1] = PBOTexturePtr();
    m_pTextures[2] = PBOTexturePtr();
    m_pTextures[3] = PBOTexturePtr();
}

void OGLSurface::activate(const IntPoint& logicalSize, bool bPremultipliedAlpha) const
{
    if (useShader()) {
        OGLShaderPtr pShader = getShader(COLORSPACE_SHADER);
        pShader->activate();
        OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, "OGLSurface::activate()");
        switch (m_pf) {
            case YCbCr420p:
            case YCbCrJ420p:
                pShader->setUniformIntParam("colorModel", 1);
                break;
            case YCbCrA420p:
                pShader->setUniformIntParam("colorModel", 3);
                break;
            case A8:
                pShader->setUniformIntParam("colorModel", 2);
                break;
            default:
                pShader->setUniformIntParam("colorModel", 0);
        }

        m_pTextures[0]->activate(GL_TEXTURE0);
        pShader->setUniformIntParam("texture", 0);
        
        if (pixelFormatIsPlanar(m_pf)) {
            m_pTextures[1]->activate(GL_TEXTURE1);
            pShader->setUniformIntParam("cbTexture", 1);
            m_pTextures[2]->activate(GL_TEXTURE2);
            pShader->setUniformIntParam("crTexture", 2);
            if (m_pf == YCbCrA420p) {
                m_pTextures[3]->activate(GL_TEXTURE3);
                pShader->setUniformIntParam("aTexture", 3);
            }
        }
        if (pixelFormatIsPlanar(m_pf) || colorIsModified()) {
            Matrix3x4 mat = calcColorspaceMatrix();
            pShader->setUniformVec4fParam("colorCoeff0", 
                    mat.val[0][0], mat.val[1][0], mat.val[2][0], 0);
            pShader->setUniformVec4fParam("colorCoeff1", 
                    mat.val[0][1], mat.val[1][1], mat.val[2][1], 0);
            pShader->setUniformVec4fParam("colorCoeff2", 
                    mat.val[0][2], mat.val[1][2], mat.val[2][2], 0);
            pShader->setUniformVec4fParam("colorCoeff3", 
                    mat.val[0][3], mat.val[1][3], mat.val[2][3], 1);
        }

        pShader->setUniformVec4fParam("gamma", float(1/m_Gamma.x), float(1/m_Gamma.y), 
                float(1/m_Gamma.z), 1.0);
        pShader->setUniformIntParam("bUseColorCoeff", colorIsModified());

        pShader->setUniformIntParam("bPremultipliedAlpha", 
                m_bUseForeignTexture || bPremultipliedAlpha);
        pShader->setUniformIntParam("bUseMask", m_Material.getHasMask());
        if (m_Material.getHasMask()) {
            m_pMaskTexture->activate(GL_TEXTURE4);
            pShader->setUniformIntParam("maskTexture", 4);
            pShader->setUniformDPointParam("maskPos", m_Material.getMaskPos());
            // maskScale is (1,1) for everything excepting words nodes.
            DPoint maskScale(1,1);
            if (logicalSize != IntPoint(0,0)) {
                maskScale = DPoint((double)logicalSize.x/m_Size.x, 
                        (double)logicalSize.y/m_Size.y);
            }
            pShader->setUniformDPointParam("maskSize", 
                    m_Material.getMaskSize()*maskScale);
        }

        OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, "OGLSurface::activate: params");
    } else {
        m_pTextures[0]->activate(GL_TEXTURE0);
        if (GLContext::getCurrent()->isUsingShaders()) {
            glproc::UseProgramObject(0);
        }
        for (int i=1; i<5; ++i) {
            glproc::ActiveTexture(GL_TEXTURE0 + i);
            glDisable(GL_TEXTURE_2D);
        }
        glproc::ActiveTexture(GL_TEXTURE0);
        OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, "OGLSurface::activate: fixed function");
    }
}

BitmapPtr OGLSurface::lockBmp(int i)
{
    return m_pTextures[i]->lockBmp();
}

void OGLSurface::unlockBmps()
{
    for (unsigned i = 0; i < getNumPixelFormatPlanes(m_pf); ++i) {
        m_pTextures[i]->unlockBmp();
    }
}

BitmapPtr OGLSurface::readbackBmp()
{
    return m_pTextures[0]->readbackBmp();
}

void OGLSurface::setTex(GLTexturePtr pTex)
{
    m_bUseForeignTexture = true;
    m_pTextures[0]->setTex(pTex);
    m_bIsDirty = true;
}

BitmapPtr OGLSurface::lockMaskBmp()
{
    AVG_ASSERT(m_Material.getHasMask());
    return m_pMaskTexture->lockBmp();
}

void OGLSurface::unlockMaskBmp()
{
    m_pMaskTexture->unlockBmp();
}

const MaterialInfo& OGLSurface::getMaterial() const
{
    return m_Material;
}

void OGLSurface::setMaterial(const MaterialInfo& material)
{
    GLContext* pContext = GLContext::getCurrent();
    if (pContext && (material.getHasMask() && !pContext->isUsingShaders())) 
    {
        throw Exception(AVG_ERR_VIDEO_GENERAL,
                "Can't set mask bitmap since shader support is disabled.");
    }
    bool bOldHasMask = m_Material.getHasMask();
    m_Material = material;
    if (m_pTextures[0]) {
        for (unsigned i = 0; i < getNumPixelFormatPlanes(m_pf); ++i) {
            m_pTextures[i]->setMaterial(material);
        }
    }
    if (bOldHasMask && !m_Material.getHasMask()) {
        m_pMaskTexture = PBOTexturePtr();
    }
    if (!bOldHasMask && m_Material.getHasMask() && m_pMaskTexture) {
        m_pMaskTexture = PBOTexturePtr(new PBOTexture(IntPoint(m_Material.getMaskSize()),
                I8, m_Material, m_MemoryMode));
    }
    m_bIsDirty = true;
}

void OGLSurface::downloadTexture()
{
    if (m_pTextures[0] && !m_bUseForeignTexture) {
        m_bIsDirty = true;
        m_pTextures[0]->download();
        if (pixelFormatIsPlanar(m_pf)) {
            m_pTextures[1]->download();
            m_pTextures[2]->download();
            if (m_pf == YCbCrA420p) {
                m_pTextures[3]->download();
            }
        }
    }
}

void OGLSurface::downloadMaskTexture()
{
    if (m_Material.getHasMask()) {
        m_bIsDirty = true;
        m_pMaskTexture->download();
    }
}

PixelFormat OGLSurface::getPixelFormat()
{
    return m_pf;
}
        
IntPoint OGLSurface::getSize()
{
    return m_Size;
}

IntPoint OGLSurface::getTextureSize()
{
    return m_pTextures[0]->getTextureSize();
}

bool OGLSurface::isCreated() const
{
    return m_pTextures[0];
}

void OGLSurface::setColorParams(const DTriple& gamma, const DTriple& brightness,
            const DTriple& contrast)
{
    m_Gamma = gamma;
    m_Brightness = brightness;
    m_Contrast = contrast;
    if (!GLContext::getCurrent()->isUsingShaders() &&
            (gammaIsModified() || colorIsModified())) 
    {
        throw Exception(AVG_ERR_VIDEO_GENERAL,
                "Can't use color correction (gamma, brightness, contrast) since shader support is disabled.");
    }
    m_bIsDirty = true;
}

void OGLSurface::createShader()
{
    string sProgram =
        "uniform sampler2D texture;\n"
        "uniform sampler2D yTexture;\n"
        "uniform sampler2D cbTexture;\n"
        "uniform sampler2D crTexture;\n"
        "uniform sampler2D aTexture;\n"
        "uniform sampler2D maskTexture;\n"
        "uniform int colorModel;  // 0=rgb, 1=yuv, 2=greyscale, 3=yuva\n"
        "uniform vec4 colorCoeff0;\n"
        "uniform vec4 colorCoeff1;\n"
        "uniform vec4 colorCoeff2;\n"
        "uniform vec4 colorCoeff3;\n"
        "uniform bool bUseColorCoeff;\n"
        "uniform vec4 gamma;\n"
        "uniform bool bPremultipliedAlpha;\n"
        "uniform bool bUseMask;\n"
        "uniform vec2 maskPos;\n"
        "uniform vec2 maskSize;\n"
        "\n"
        "vec4 convertYCbCr(mat4 colorCoeff)\n"
        "{\n"
        "    vec4 yuv;\n"
        "    yuv = vec4(texture2D(texture, gl_TexCoord[0].st).r,\n"
        "               texture2D(cbTexture, (gl_TexCoord[0].st)).r,\n"
        "               texture2D(crTexture, (gl_TexCoord[0].st)).r,\n"
        "               1.0);\n"
        "    vec4 rgb;\n"
        "    rgb = colorCoeff*yuv;\n"
        "    return vec4(rgb.rgb, gl_Color.a);\n"
        "}\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    vec4 rgba;\n"
        "    mat4 colorCoeff;\n"
        "    colorCoeff[0] = colorCoeff0;\n"
        "    colorCoeff[1] = colorCoeff1;\n"
        "    colorCoeff[2] = colorCoeff2;\n"
        "    colorCoeff[3] = colorCoeff3;\n"
        "    if (colorModel == 0) {\n"
        "        rgba = texture2D(texture, gl_TexCoord[0].st);\n"
        "        if (bUseColorCoeff) {\n"
        "           rgba = colorCoeff*rgba;\n"
        "        };\n"
        "        rgba.a *= gl_Color.a;\n"
        "    } else if (colorModel == 1) {\n"
        "        rgba = convertYCbCr(colorCoeff);\n"
        "    } else if (colorModel == 2) {\n"
        "        rgba = gl_Color;\n"
        "        if (bUseColorCoeff) {\n"
        "           rgba = colorCoeff*rgba;\n"
        "        };\n"
        "        rgba.a *= texture2D(texture, gl_TexCoord[0].st).a;\n"
        "    } else if (colorModel == 3) {\n"
        "        rgba = convertYCbCr(colorCoeff);\n"
        "        rgba.a *= texture2D(aTexture, gl_TexCoord[0].st).r;\n"
        "    } else {\n"
        "        rgba = vec4(1,1,1,1);\n"
        "    }\n"
        "    rgba = pow(rgba, gamma);\n"
        "    if (bUseMask) {\n"
        "        if (bPremultipliedAlpha) {\n"
        "            rgba.rgb *= texture2D(maskTexture,\n"
        "                    (gl_TexCoord[0].st/maskSize)-maskPos).r;\n"
        "        }\n"
        "        rgba.a *= texture2D(maskTexture,\n"
        "                (gl_TexCoord[0].st/maskSize)-maskPos).r;\n"
        "    }\n"
        "    gl_FragColor = rgba;\n"
        "}\n";
    getOrCreateShader(COLORSPACE_SHADER, sProgram);
}

bool OGLSurface::isDirty() const
{
    return m_bIsDirty;
}

void OGLSurface::resetDirty()
{
    m_bIsDirty = false;
}

bool OGLSurface::useShader() const
{
    return GLContext::getCurrent()->isUsingShaders() && 
            (m_Material.getHasMask() || pixelFormatIsPlanar(m_pf) || gammaIsModified() || 
                    colorIsModified());
}

Matrix3x4 OGLSurface::calcColorspaceMatrix() const
{
    Matrix3x4 mat;
    if (colorIsModified()) {
        mat *= Matrix3x4::createScale(m_Brightness);
        mat *= Matrix3x4::createTranslate(float(0.5-m_Contrast.x/2), 
                float(0.5-m_Contrast.y/2), float(0.5-m_Contrast.z/2));
        mat *= Matrix3x4::createScale(m_Contrast);
    }
    if (m_pf == YCbCr420p || m_pf == YCbCrJ420p || m_pf == YCbCrA420p) {
        mat *= Matrix3x4(*yuvCoeff);
        mat *= Matrix3x4::createTranslate(0.0, -0.5, -0.5);
        if (m_pf == YCbCr420p || m_pf == YCbCrA420p) {
            mat *= Matrix3x4::createScale(255.0f/(235-16), 255.0f/(240-16) , 
                    255.0f/(240-16));
            mat *= Matrix3x4::createTranslate(-16.0f/255, -16.0f/255, -16.0f/255);
        }
    }
    return mat;
}

bool OGLSurface::gammaIsModified() const
{
    return (fabs(m_Gamma.x-1.0) > 0.00001 || fabs(m_Gamma.y-1.0) > 0.00001 ||
           fabs(m_Gamma.z-1.0) > 0.00001);
}

bool OGLSurface::colorIsModified() const
{
    return (fabs(m_Brightness.x-1.0) > 0.00001 || fabs(m_Brightness.y-1.0) > 0.00001 ||
           fabs(m_Brightness.z-1.0) > 0.00001 || fabs(m_Contrast.x-1.0) > 0.00001 ||
           fabs(m_Contrast.y-1.0) > 0.00001 || fabs(m_Contrast.z-1.0) > 0.00001);
}

}
