//
// $Id$
// 

#ifndef _SDLDisplayEngine_H_
#define _SDLDisplayEngine_H_

#include "IEventSource.h"
#include "IDisplayEngine.h"
#include "VBlank.h"
#include "../graphics/Pixel32.h"

#include <SDL/SDL.h>

#include <string>
#include <vector>

namespace avg {

class OGLSurface;

class SDLDisplayEngine: public IDisplayEngine, public IEventSource
{
    public:
        SDLDisplayEngine();
        virtual ~SDLDisplayEngine();

        // From IDisplayEngine
        virtual void init(int width, int height, bool isFullscreen, int bpp,
                int WindowWidth, int WindowHeight);
        virtual void teardown();

        virtual void render(AVGNode * pRootNode, 
                FramerateManager * pFramerateManager, bool bRenderEverything);
        
        virtual void setClipRect();
        virtual bool pushClipRect(const DRect& rc, bool bClip);
        virtual void popClipRect();
        virtual const DRect& getClipRect();
        virtual void blt32(ISurface * pSurface, const DRect* pDestRect, 
                double opacity, double angle, const DPoint& pivot,
                BlendMode Mode);
        virtual void blta8(ISurface * pSurface, const DRect* pDestRect,
                double opacity, const Pixel32& color, double angle, 
                const DPoint& pivot, BlendMode Mode);

        virtual ISurface * createSurface();
        virtual void surfaceChanged(ISurface * pSurface);

        virtual int getWidth();
        virtual int getHeight();
        virtual int getBPP();

        virtual bool supportsBpp(int bpp);
        virtual bool hasRGBOrdering(); 
        virtual bool isYCbCrSupported(); 
        virtual void showCursor (bool bShow);
        virtual BitmapPtr screenshot ();

        // From IEventSource
        virtual std::vector<Event *> pollEvents();

    private:
        void initSDL(int width, int height, bool isFullscreen, int bpp);
        void initInput();
        void initTranslationTable();
        void initJoysticks();
        void logConfig(); 
        void setDirtyRect(const DRect& rc);
        virtual void swapBuffers();
        void clip();
        void setClipPlane(double Eqn[4], int WhichPlane);
        void safeSetAttribute( SDL_GLattr attr, int value);

        Event * createMouseMotionEvent 
                (Event::Type Type, const SDL_Event & SDLEvent);
        Event * createMouseButtonEvent
                (Event::Type Type, const SDL_Event & SDLEvent);
        Event * createKeyEvent
                (Event::Type Type, const SDL_Event & SDLEvent);
        
        int m_Width;
        int m_Height;
        bool m_IsFullscreen;
        int m_bpp;
        int m_WindowWidth;
        int m_WindowHeight;
        std::vector<DRect> m_ClipRects;
        DRect m_DirtyRect;
        bool m_bEnableCrop;

        SDL_Surface * m_pScreen;

        static std::vector<long> KeyCodeTranslationTable;
};

}

#endif 
