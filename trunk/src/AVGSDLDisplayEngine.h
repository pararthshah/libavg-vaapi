//
// $Id$
// 

#ifndef _AVGSDLDisplayEngine_H_
#define _AVGSDLDisplayEngine_H_

#include "AVGSDLFontManager.h"
#include "IAVGEventSource.h"
#include "IAVGDisplayEngine.h"

#include <paintlib/plrect.h>

#include <SDL/SDL.h>

#include <string>

class AVGOGLBmp;

class AVGSDLDisplayEngine: public IAVGDisplayEngine, public IAVGEventSource
{
    public:
        AVGSDLDisplayEngine();
        virtual ~AVGSDLDisplayEngine();

        // From IAVGDisplayEngine
        virtual void init(int width, int height, bool isFullscreen, int bpp);
        virtual void teardown();

        virtual void render(AVGNode * pRootNode, 
                AVGFramerateManager * pFramerateManager, bool bRenderEverything);
        
        virtual void setClipRect();
        virtual bool pushClipRect(const AVGDRect& rc, bool bClip);
        virtual void popClipRect();
        virtual const AVGDRect& getClipRect();
        virtual void blt32(PLBmp * pBmp, const AVGDRect* pDestRect, 
                double opacity, double angle, const AVGDPoint& pivot);
        virtual void blta8(PLBmp * pBmp, const AVGDRect* pDestRect,
                double opacity, const PLPixel32& color, double angle, 
                const AVGDPoint& pivot);

        virtual PLBmp * createSurface();
        virtual void surfaceChanged(PLBmp* pBmp);

        virtual int getWidth();
        virtual int getHeight();
        virtual int getBPP();

        virtual AVGFontManager * getFontManager();
        virtual bool hasYUVSupport();

        // From IAVGEventSource
        virtual std::vector<AVGEvent *> pollEvents();

    private:
        void initSDL(int width, int height, bool isFullscreen, int bpp);
        void initInput();
        void initTranslationTable();
        void initJoysticks();
        void setDirtyRect(const AVGDRect& rc);
        void bltTexture(AVGOGLBmp * pOGLBmp, const AVGDRect* pDestRect,
                float Width, float Height, double angle, const AVGDPoint& pivot);
        virtual void swapBuffers();
        void clip();

        AVGEvent * createMouseMotionEvent 
                (int Type, const SDL_Event & SDLEvent);
        AVGEvent * createMouseButtonEvent
                (int Type, const SDL_Event & SDLEvent);
        AVGEvent * createKeyEvent
                (int Type, const SDL_Event & SDLEvent);

        int m_Width;
        int m_Height;
        bool m_IsFullscreen;
        int m_bpp;
        std::vector<AVGDRect> m_ClipRects;
        AVGDRect m_DirtyRect;

        SDL_Surface * m_pScreen;

        static std::vector<long> KeyCodeTranslationTable;

        AVGSDLFontManager *m_pFontManager;
};

#endif 
