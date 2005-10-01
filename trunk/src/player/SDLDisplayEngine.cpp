//
// $Id$
//

#include "SDLDisplayEngine.h"
#include "Region.h"
#include "Player.h"
#include "Node.h"
#include "AVGNode.h"
#include "FramerateManager.h"

#include "Event.h"
#include "MouseEvent.h"
#include "KeyEvent.h"

#include "OGLSurface.h"
#include "OGLHelper.h"

#include "../base/Exception.h"
#include "../base/Logger.h"
#include "../base/ScopeTimer.h"

#include "../graphics/Filterflip.h"
#include "../graphics/Filterfliprgb.h"

#define XMD_H 1
#include "GL/gl.h"
#include "GL/glu.h"

#include <sys/ioctl.h>
#include <sys/fcntl.h>

#include <signal.h>
#include <iostream>
#include <sstream>
#include <unistd.h>

using namespace std;

namespace avg {

void dumpSDLGLParams() {
    int value;
    cerr << "SDL display parameters used: "<< endl;
    SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &value);
    fprintf(stderr,"  SDL_GL_RED_SIZE = %d\n",value);
    SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE,  &value);
    fprintf(stderr,"  SDL_GL_GREEN_SIZE = %d\n",value);
    SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE , &value);
    fprintf(stderr,"  SDL_GL_BLUE_SIZE = %d\n",value);
    SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE , &value);
    fprintf(stderr,"  SDL_GL_ALPHA_SIZE = %d\n",value);
    SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE , &value);
    fprintf(stderr,"  SDL_GL_DEPTH_SIZE = %d\n",value);
    SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER , &value);
    fprintf(stderr,"  SDL_GL_DOUBLEBUFFER = %d\n",value);
    SDL_GL_GetAttribute(SDL_GL_BUFFER_SIZE , &value);
    fprintf(stderr,"  SDL_GL_BUFFER_SIZE = %d\n",value);
    SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE , &value);
    fprintf(stderr,"  SDL_GL_STENCIL_SIZE = %d\n",value);
}

SDLDisplayEngine::SDLDisplayEngine()
    : m_pScreen(0)
{
    if (SDL_InitSubSystem(SDL_INIT_VIDEO)==-1) {
        AVG_TRACE(Logger::ERROR, "Can't init SDL display subsystem.");
        exit(-1);
    }
    initTranslationTable();
}

SDLDisplayEngine::~SDLDisplayEngine()
{
    if (m_pScreen) {
        teardown();
    }
}

void SDLDisplayEngine::init(int width, int height, bool isFullscreen, 
        int bpp, int WindowWidth, int WindowHeight)
{
    double AspectRatio = double(width)/double(height);
    if (WindowWidth == 0 && WindowHeight == 0) {
        m_WindowWidth = width;
        m_WindowHeight = height;
    } else if (WindowWidth == 0) {
        m_WindowWidth = int(WindowHeight*AspectRatio);
        m_WindowHeight = WindowHeight;
    } else {
        m_WindowWidth = WindowWidth;
        m_WindowHeight = int(WindowWidth/AspectRatio);
    }
    
    switch (bpp) {
        case 32:
            safeSetAttribute( SDL_GL_RED_SIZE, 8 );
            safeSetAttribute( SDL_GL_GREEN_SIZE, 8 );
            safeSetAttribute( SDL_GL_BLUE_SIZE, 8 );
            safeSetAttribute( SDL_GL_BUFFER_SIZE, 32 );
            break;
        case 24:
            safeSetAttribute( SDL_GL_RED_SIZE, 8 );
            safeSetAttribute( SDL_GL_GREEN_SIZE, 8 );
            safeSetAttribute( SDL_GL_BLUE_SIZE, 8 );
            safeSetAttribute( SDL_GL_BUFFER_SIZE, 24 );
            break;
        case 16:
            safeSetAttribute( SDL_GL_RED_SIZE, 5 );
            safeSetAttribute( SDL_GL_GREEN_SIZE, 6 );
            safeSetAttribute( SDL_GL_BLUE_SIZE, 5 );
            safeSetAttribute( SDL_GL_BUFFER_SIZE, 16 );
            break;
        case 15:
            safeSetAttribute( SDL_GL_RED_SIZE, 5 );
            safeSetAttribute( SDL_GL_GREEN_SIZE, 5 );
            safeSetAttribute( SDL_GL_BLUE_SIZE, 5 );
            safeSetAttribute( SDL_GL_BUFFER_SIZE, 15 );
            break;
        default:
            AVG_TRACE(Logger::ERROR, "Unsupported bpp " << bpp <<
                    "in SDLDisplayEngine::init()");
            exit(-1);
    }
    safeSetAttribute( SDL_GL_DEPTH_SIZE, 0 );
    safeSetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

    unsigned int Flags = SDL_OPENGL;
    if (isFullscreen) {
        Flags |= SDL_FULLSCREEN;
    }
    m_pScreen = SDL_SetVideoMode(m_WindowWidth, m_WindowHeight, bpp, Flags);
    if (!m_pScreen) {
        AVG_TRACE(Logger::ERROR, "Setting SDL video mode failed: " 
                << SDL_GetError() <<". (width=" << m_WindowWidth << ", height=" 
                << m_WindowHeight << ", bpp=" << bpp << ").");
        exit(-1);
    }   
    SDL_WM_SetCaption("AVG Renderer", 0);

    glEnable(GL_BLEND);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, "init: glEnable(GL_BLEND)");
    glShadeModel(GL_FLAT);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, "init: glShadeModel()");
    glDisable(GL_DEPTH_TEST);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, "init: glDisable(GL_DEPTH_TEST)");
    int TexMode = OGLSurface::getTextureMode();
    glEnable(TexMode);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, "init: glEnable(TexMode);");

    m_Width = width;
    m_Height = height;
    initInput();
    // SDL sets up a signal handler we really don't want.
//    signal(SIGSEGV, SIG_DFL);
    m_VBlank.init();
    
    logConfig();
}

void SDLDisplayEngine::teardown()
{
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void SDLDisplayEngine::logConfig() 
{
    AVG_TRACE(Logger::CONFIG, "OpenGL version: " << glGetString(GL_VERSION));
    AVG_TRACE(Logger::CONFIG, "OpenGL vendor: " << glGetString(GL_VENDOR));
    AVG_TRACE(Logger::CONFIG, "OpenGL renderer: " << glGetString(GL_RENDERER));
}

void SDLDisplayEngine::render(AVGNode * pRootNode, 
        FramerateManager * pFramerateManager, bool bRenderEverything)
{
    m_bEnableCrop = pRootNode->getCropSetting();
    pRootNode->prepareRender(0, pRootNode->getAbsViewport());
    glClearColor(0.0, 0.0, 0.0, 0.0); 
    glClear(GL_COLOR_BUFFER_BIT);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, 
            "SDLDisplayEngine::render::glClear()");

    glViewport(0, 0, m_WindowWidth, m_WindowHeight);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, 
            "SDLDisplayEngine::render: glViewport()");
    glMatrixMode(GL_PROJECTION);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, 
            "SDLDisplayEngine::render: glMatrixMode()");
    glLoadIdentity();
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, 
            "SDLDisplayEngine::render: glLoadIdentity()");
    gluOrtho2D(0, m_Width, m_Height, 0);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, 
            "SDLDisplayEngine::render: gluOrtho2D()");
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); 
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, 
            "SDLDisplayEngine::render: glTexEnvf()");
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, 
            "SDLDisplayEngine::render: glBlendFunc()");
    
    const DRect rc(0,0, m_Width, m_Height);
    glMatrixMode(GL_MODELVIEW);
    pRootNode->maybeRender(rc);
    pFramerateManager->FrameWait();
    swapBuffers();
    pFramerateManager->CheckJitter();
}
 
void SDLDisplayEngine::setClipRect()
{
    m_ClipRects.clear();
    m_ClipRects.push_back(DRect(0, 0, m_Width, m_Height));
}

bool SDLDisplayEngine::pushClipRect(const DRect& rc, bool bClip)
{
    m_ClipRects.push_back(rc);

    glPushMatrix();
    AVG_TRACE(Logger::BLTS, "Clip set to " << 
            rc.tl.x << "x" << rc.tl.y << 
            ", width: " << rc.Width() << ", height: " << 
            rc.Height());
    if (bClip) {
        clip();
    }
    return true;
}

void SDLDisplayEngine::popClipRect()
{
    glPopMatrix();
    m_ClipRects.pop_back();
    clip();
}

const DRect& SDLDisplayEngine::getClipRect() 
{
    return m_ClipRects.back();
}

void SDLDisplayEngine::blt32(ISurface * pSurface, const DRect* pDestRect, 
        double opacity, double angle, const DPoint& pivot, BlendMode Mode)
{
    OGLSurface * pOGLSurface = dynamic_cast<OGLSurface*>(pSurface);
    glColor4f(1.0f, 1.0f, 1.0f, opacity);
    pOGLSurface->blt(pDestRect, opacity, angle, pivot, Mode);
}

void SDLDisplayEngine::blta8(ISurface * pSurface, 
        const DRect* pDestRect, double opacity, 
        const Pixel32& color, double angle, 
        const DPoint& pivot, BlendMode Mode)
{
    OGLSurface * pOGLSurface = dynamic_cast<OGLSurface*>(pSurface);
    glColor4f(float(color.getR())/256, float(color.getG())/256, 
            float(color.getB())/256, opacity);
    pOGLSurface->blt(pDestRect, opacity, angle, pivot, Mode);
}


void SDLDisplayEngine::setClipPlane(double Eqn[4], int WhichPlane)
{
    glEnable (WhichPlane);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, 
            "setClipPlane: glEnable()");
    glClipPlane(WhichPlane, Eqn);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, 
            "setClipPlane: glClipPlane()");
}

void SDLDisplayEngine::clip()
{
    
    DRect rc = m_ClipRects.back();
    if (m_bEnableCrop) {       
        double yEqn[4] = {0.0, 1.0, 0.0, -rc.tl.y};
        //    yEqn[3] = -rc.tl.y;
        setClipPlane(yEqn, GL_CLIP_PLANE0);

        yEqn[0] = 0;
        yEqn[1] = -1.0;
        yEqn[2] = 0;
        yEqn[3] = rc.br.y;
        setClipPlane(yEqn, GL_CLIP_PLANE1);

        double xEqn[4] = {1.0, 0.0, 0.0, 0.0};
        xEqn[3] = -rc.tl.x;
        setClipPlane(xEqn, GL_CLIP_PLANE2);

        xEqn[0] = -1;
        xEqn[1] = 0.0;
        xEqn[2] = 0.0;
        xEqn[3] = rc.br.x;
        setClipPlane(xEqn, GL_CLIP_PLANE3);
    }
}

void SDLDisplayEngine::setDirtyRect(const DRect& rc) 
{
    m_DirtyRect = rc;
    
    AVG_TRACE(Logger::BLTS, "Dirty rect: " << m_DirtyRect.tl.x << "x" << 
            m_DirtyRect.tl.y << ", width: " << m_DirtyRect.Width() << 
            ", height: " << m_DirtyRect.Height());
}

static ProfilingZone VBlankProfilingZone("  Render - VBlank wait");
static ProfilingZone SwapBufferProfilingZone("  Render - swap buffers");

void SDLDisplayEngine::swapBuffers()
{
    {   
        ScopeTimer VBlankTimer(VBlankProfilingZone);
        m_VBlank.wait();
    }
    {
        ScopeTimer Timer(SwapBufferProfilingZone);
        SDL_GL_SwapBuffers();
        AVG_TRACE(Logger::BLTS, "GL SwapBuffers");
        OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, "swapBuffers()");
    }
}

ISurface * SDLDisplayEngine::createSurface()
{
    return new OGLSurface;
}

void SDLDisplayEngine::surfaceChanged(ISurface * pSurface) 
{
    OGLSurface * pOGLSurface = dynamic_cast<OGLSurface *>(pSurface);
    pOGLSurface->bind();
}

bool SDLDisplayEngine::supportsBpp(int bpp)
{
    return (bpp == 24 || bpp == 32);
}

bool SDLDisplayEngine::hasRGBOrdering()
{
    return true;
}

void SDLDisplayEngine::showCursor (bool bShow)
{
    if (bShow) {
        SDL_ShowCursor(SDL_ENABLE);
    } else {
        SDL_ShowCursor(SDL_DISABLE);
    }
}

BitmapPtr SDLDisplayEngine::screenshot ()
{
    BitmapPtr pBmp (new Bitmap(IntPoint(m_Width, m_Height), R8G8B8X8, "screenshot"));
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, m_Width, m_Height, GL_RGBA, GL_UNSIGNED_BYTE, 
            pBmp->getPixels());
    FilterFlipRGB().applyInPlace(pBmp);
    FilterFlip().applyInPlace(pBmp);
    return pBmp;
}

int SDLDisplayEngine::getWidth()
{
    return m_Width;
}

int SDLDisplayEngine::getHeight()
{
    return m_Height;
}

int SDLDisplayEngine::getBPP()
{
    return m_bpp;
}


vector<long> SDLDisplayEngine::KeyCodeTranslationTable(SDLK_LAST, key::KEY_UNKNOWN);

void SDLDisplayEngine::initInput()
{
    initJoysticks();
}

vector<Event *> SDLDisplayEngine::pollEvents()
{
    SDL_Event sdlEvent;
    vector<Event *> Events;

    while(SDL_PollEvent(&sdlEvent)){
        switch(sdlEvent.type){
            case SDL_MOUSEMOTION:
                Events.push_back
                        (createMouseMotionEvent(Event::MOUSEMOTION, sdlEvent));
                break;
            case SDL_MOUSEBUTTONDOWN:
                Events.push_back
                        (createMouseButtonEvent(Event::MOUSEBUTTONDOWN, sdlEvent));
                break;
            case SDL_MOUSEBUTTONUP:
                Events.push_back
                        (createMouseButtonEvent(Event::MOUSEBUTTONUP, sdlEvent));
                break;
            case SDL_JOYAXISMOTION:
//                Events.push_back(createAxisEvent(sdlEvent));
                break;
            case SDL_JOYBUTTONDOWN:
//                Events.push_back(createButtonEvent(Event::BUTTON_DOWN, sdlEvent));
                break;
            case SDL_JOYBUTTONUP:
//                Events.push_back(createButtonEvent(Event::BUTTON_UP, sdlEvent));
                break;
            case SDL_KEYDOWN:
                Events.push_back(createKeyEvent(Event::KEYDOWN, sdlEvent));
                break;
            case SDL_KEYUP:
                Events.push_back(createKeyEvent(Event::KEYUP, sdlEvent));
                break;
            case SDL_QUIT:
                {
                    Event * pEvent = new Event(Event::QUIT);
                    Events.push_back(pEvent);
                }
                break;
            case SDL_VIDEORESIZE:
                break;
            default:
                // Ignore unknown events.
                break;
        }
    }
    return Events;
}

Event * SDLDisplayEngine::createMouseMotionEvent
        (Event::Type Type, const SDL_Event & SDLEvent)
{
    int x = int((SDLEvent.motion.x*m_Width)/m_WindowWidth);
    int y = int((SDLEvent.motion.y*m_Height)/m_WindowHeight);
    MouseEvent * pEvent = new MouseEvent (Type, 
            SDLEvent.motion.state & SDL_BUTTON(1),
            SDLEvent.motion.state & SDL_BUTTON(3), 
            SDLEvent.motion.state & SDL_BUTTON(2),
            x, y, 
            MouseEvent::NO_BUTTON);
    return pEvent;
}

Event * SDLDisplayEngine::createMouseButtonEvent
        (Event::Type Type, const SDL_Event & SDLEvent) 
{
    long Button = 0;
    switch (SDLEvent.button.button) {
        case SDL_BUTTON_LEFT:
            Button = MouseEvent::LEFT_BUTTON;
            break;
        case SDL_BUTTON_MIDDLE:
            Button = MouseEvent::MIDDLE_BUTTON;
            break;
        case SDL_BUTTON_RIGHT:
            Button = MouseEvent::RIGHT_BUTTON;
            break;
    }
    int x,y;
    SDL_GetMouseState(&x, &y);
    x = int((x*m_Width)/m_WindowWidth);
    y = int((y*m_Height)/m_WindowHeight);
    MouseEvent * pEvent = new MouseEvent(Type, 
            SDLEvent.button.button == SDL_BUTTON_LEFT,
            SDLEvent.button.button == SDL_BUTTON_MIDDLE, 
            SDLEvent.button.button == SDL_BUTTON_RIGHT,
            x, y, Button);
    return pEvent; 
}

/*
Event * SDLDisplayEngine::createAxisEvent(const SDL_Event & SDLEvent)
{
    return new AxisEvent(SDLEvent.jaxis.which, SDLEvent.jaxis.axis,
                SDLEvent.jaxis.value);
}


Event * SDLDisplayEngine::createButtonEvent
        (Event::Type Type, const SDL_Event & SDLEvent) 
{
    return new ButtonEvent(Type, SDLEvent.jbutton.which,
                SDLEvent.jbutton.button));
}
*/

Event * SDLDisplayEngine::createKeyEvent
        (Event::Type Type, const SDL_Event & SDLEvent)
{
    long KeyCode = KeyCodeTranslationTable[SDLEvent.key.keysym.sym];
    unsigned int Modifiers = key::KEYMOD_NONE;

    if (SDLEvent.key.keysym.mod & KMOD_LSHIFT) 
        { Modifiers |= key::KEYMOD_LSHIFT; }
    if (SDLEvent.key.keysym.mod & KMOD_RSHIFT) 
        { Modifiers |= key::KEYMOD_RSHIFT; }
    if (SDLEvent.key.keysym.mod & KMOD_LCTRL) 
        { Modifiers |= key::KEYMOD_LCTRL; }
    if (SDLEvent.key.keysym.mod & KMOD_RCTRL) 
        { Modifiers |= key::KEYMOD_RCTRL; }
    if (SDLEvent.key.keysym.mod & KMOD_LALT) 
        { Modifiers |= key::KEYMOD_LALT; }
    if (SDLEvent.key.keysym.mod & KMOD_RALT) 
        { Modifiers |= key::KEYMOD_RALT; }
    if (SDLEvent.key.keysym.mod & KMOD_LMETA) 
        { Modifiers |= key::KEYMOD_LMETA; }
    if (SDLEvent.key.keysym.mod & KMOD_RMETA) 
        { Modifiers |= key::KEYMOD_RMETA; }
    if (SDLEvent.key.keysym.mod & KMOD_NUM) 
        { Modifiers |= key::KEYMOD_NUM; }
    if (SDLEvent.key.keysym.mod & KMOD_CAPS) 
        { Modifiers |= key::KEYMOD_CAPS; }
    if (SDLEvent.key.keysym.mod & KMOD_MODE) 
        { Modifiers |= key::KEYMOD_MODE; }
    if (SDLEvent.key.keysym.mod & KMOD_RESERVED) 
        { Modifiers |= key::KEYMOD_RESERVED; }

    KeyEvent * pEvent = new KeyEvent(Type, 
            SDLEvent.key.keysym.scancode, KeyCode,
            SDL_GetKeyName(SDLEvent.key.keysym.sym), Modifiers);
    return pEvent;
}

void SDLDisplayEngine::initJoysticks() 
{
/*
    SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    SDL_JoystickEventState(SDL_ENABLE);
    cerr << "**** Number of joysticks: " << SDL_NumJoysticks() << endl;
    for (int i=0; i<SDL_NumJoysticks(); i++) {
        SDL_Joystick * pJoystick = SDL_JoystickOpen(i);
        if (!pJoystick) {
            cerr << "Warning: could not open joystick # " << i << "of"
                << SDL_NumJoysticks() << endl;
        } else {
            printf("Opened Joystick %i\n", i);
            printf("Name: %s\n", SDL_JoystickName(i));
            printf("Number of Axes: %d\n", SDL_JoystickNumAxes(pJoystick));
            printf("Number of Buttons: %d\n", SDL_JoystickNumButtons(pJoystick));
            printf("Number of Balls: %d\n", SDL_JoystickNumBalls(pJoystick));
        }
    }
*/
}

void SDLDisplayEngine::safeSetAttribute( SDL_GLattr attr, int value) {
    int err = SDL_GL_SetAttribute(attr, value);
    if (err == -1) {
        throw Exception(AVG_ERR_VIDEO_GENERAL, SDL_GetError());
    }
}

void SDLDisplayEngine::initTranslationTable()
{
#define TRANSLATION_ENTRY(x) KeyCodeTranslationTable[SDLK_##x] = key::KEY_##x;

    TRANSLATION_ENTRY(UNKNOWN);
    TRANSLATION_ENTRY(BACKSPACE);
    TRANSLATION_ENTRY(TAB);
    TRANSLATION_ENTRY(CLEAR);
    TRANSLATION_ENTRY(RETURN);
    TRANSLATION_ENTRY(PAUSE);
    TRANSLATION_ENTRY(ESCAPE);
    TRANSLATION_ENTRY(SPACE);
    TRANSLATION_ENTRY(EXCLAIM);
    TRANSLATION_ENTRY(QUOTEDBL);
    TRANSLATION_ENTRY(HASH);
    TRANSLATION_ENTRY(DOLLAR);
    TRANSLATION_ENTRY(AMPERSAND);
    TRANSLATION_ENTRY(QUOTE);
    TRANSLATION_ENTRY(LEFTPAREN);
    TRANSLATION_ENTRY(RIGHTPAREN);
    TRANSLATION_ENTRY(ASTERISK);
    TRANSLATION_ENTRY(PLUS);
    TRANSLATION_ENTRY(COMMA);
    TRANSLATION_ENTRY(MINUS);
    TRANSLATION_ENTRY(PERIOD);
    TRANSLATION_ENTRY(SLASH);
    TRANSLATION_ENTRY(0);
    TRANSLATION_ENTRY(1);
    TRANSLATION_ENTRY(2);
    TRANSLATION_ENTRY(3);
    TRANSLATION_ENTRY(4);
    TRANSLATION_ENTRY(5);
    TRANSLATION_ENTRY(6);
    TRANSLATION_ENTRY(7);
    TRANSLATION_ENTRY(8);
    TRANSLATION_ENTRY(9);
    TRANSLATION_ENTRY(COLON);
    TRANSLATION_ENTRY(SEMICOLON);
    TRANSLATION_ENTRY(LESS);
    TRANSLATION_ENTRY(EQUALS);
    TRANSLATION_ENTRY(GREATER);
    TRANSLATION_ENTRY(QUESTION);
    TRANSLATION_ENTRY(AT);
    TRANSLATION_ENTRY(LEFTBRACKET);
    TRANSLATION_ENTRY(BACKSLASH);
    TRANSLATION_ENTRY(RIGHTBRACKET);
    TRANSLATION_ENTRY(CARET);
    TRANSLATION_ENTRY(UNDERSCORE);
    TRANSLATION_ENTRY(BACKQUOTE);
    TRANSLATION_ENTRY(a);
    TRANSLATION_ENTRY(b);
    TRANSLATION_ENTRY(c);
    TRANSLATION_ENTRY(d);
    TRANSLATION_ENTRY(e);
    TRANSLATION_ENTRY(f);
    TRANSLATION_ENTRY(g);
    TRANSLATION_ENTRY(h);
    TRANSLATION_ENTRY(i);
    TRANSLATION_ENTRY(j);
    TRANSLATION_ENTRY(k);
    TRANSLATION_ENTRY(l);
    TRANSLATION_ENTRY(m);
    TRANSLATION_ENTRY(n);
    TRANSLATION_ENTRY(o);
    TRANSLATION_ENTRY(p);
    TRANSLATION_ENTRY(q);
    TRANSLATION_ENTRY(r);
    TRANSLATION_ENTRY(s);
    TRANSLATION_ENTRY(t);
    TRANSLATION_ENTRY(u);
    TRANSLATION_ENTRY(v);
    TRANSLATION_ENTRY(w);
    TRANSLATION_ENTRY(x);
    TRANSLATION_ENTRY(y);
    TRANSLATION_ENTRY(z);
    TRANSLATION_ENTRY(DELETE);
    TRANSLATION_ENTRY(WORLD_0);
    TRANSLATION_ENTRY(WORLD_1);
    TRANSLATION_ENTRY(WORLD_2);
    TRANSLATION_ENTRY(WORLD_3);
    TRANSLATION_ENTRY(WORLD_4);
    TRANSLATION_ENTRY(WORLD_5);
    TRANSLATION_ENTRY(WORLD_6);
    TRANSLATION_ENTRY(WORLD_7);
    TRANSLATION_ENTRY(WORLD_8);
    TRANSLATION_ENTRY(WORLD_9);
    TRANSLATION_ENTRY(WORLD_10);
    TRANSLATION_ENTRY(WORLD_11);
    TRANSLATION_ENTRY(WORLD_12);
    TRANSLATION_ENTRY(WORLD_13);
    TRANSLATION_ENTRY(WORLD_14);
    TRANSLATION_ENTRY(WORLD_15);
    TRANSLATION_ENTRY(WORLD_16);
    TRANSLATION_ENTRY(WORLD_17);
    TRANSLATION_ENTRY(WORLD_18);
    TRANSLATION_ENTRY(WORLD_19);
    TRANSLATION_ENTRY(WORLD_20);
    TRANSLATION_ENTRY(WORLD_21);
    TRANSLATION_ENTRY(WORLD_22);
    TRANSLATION_ENTRY(WORLD_23);
    TRANSLATION_ENTRY(WORLD_24);
    TRANSLATION_ENTRY(WORLD_25);
    TRANSLATION_ENTRY(WORLD_26);
    TRANSLATION_ENTRY(WORLD_27);
    TRANSLATION_ENTRY(WORLD_28);
    TRANSLATION_ENTRY(WORLD_29);
    TRANSLATION_ENTRY(WORLD_30);
    TRANSLATION_ENTRY(WORLD_31);
    TRANSLATION_ENTRY(WORLD_32);
    TRANSLATION_ENTRY(WORLD_33);
    TRANSLATION_ENTRY(WORLD_34);
    TRANSLATION_ENTRY(WORLD_35);
    TRANSLATION_ENTRY(WORLD_36);
    TRANSLATION_ENTRY(WORLD_37);
    TRANSLATION_ENTRY(WORLD_38);
    TRANSLATION_ENTRY(WORLD_39);
    TRANSLATION_ENTRY(WORLD_40);
    TRANSLATION_ENTRY(WORLD_41);
    TRANSLATION_ENTRY(WORLD_42);
    TRANSLATION_ENTRY(WORLD_43);
    TRANSLATION_ENTRY(WORLD_44);
    TRANSLATION_ENTRY(WORLD_45);
    TRANSLATION_ENTRY(WORLD_46);
    TRANSLATION_ENTRY(WORLD_47);
    TRANSLATION_ENTRY(WORLD_48);
    TRANSLATION_ENTRY(WORLD_49);
    TRANSLATION_ENTRY(WORLD_50);
    TRANSLATION_ENTRY(WORLD_51);
    TRANSLATION_ENTRY(WORLD_52);
    TRANSLATION_ENTRY(WORLD_53);
    TRANSLATION_ENTRY(WORLD_54);
    TRANSLATION_ENTRY(WORLD_55);
    TRANSLATION_ENTRY(WORLD_56);
    TRANSLATION_ENTRY(WORLD_57);
    TRANSLATION_ENTRY(WORLD_58);
    TRANSLATION_ENTRY(WORLD_59);
    TRANSLATION_ENTRY(WORLD_60);
    TRANSLATION_ENTRY(WORLD_61);
    TRANSLATION_ENTRY(WORLD_62);
    TRANSLATION_ENTRY(WORLD_63);
    TRANSLATION_ENTRY(WORLD_64);
    TRANSLATION_ENTRY(WORLD_65);
    TRANSLATION_ENTRY(WORLD_66);
    TRANSLATION_ENTRY(WORLD_67);
    TRANSLATION_ENTRY(WORLD_68);
    TRANSLATION_ENTRY(WORLD_69);
    TRANSLATION_ENTRY(WORLD_70);
    TRANSLATION_ENTRY(WORLD_71);
    TRANSLATION_ENTRY(WORLD_72);
    TRANSLATION_ENTRY(WORLD_73);
    TRANSLATION_ENTRY(WORLD_74);
    TRANSLATION_ENTRY(WORLD_75);
    TRANSLATION_ENTRY(WORLD_76);
    TRANSLATION_ENTRY(WORLD_77);
    TRANSLATION_ENTRY(WORLD_78);
    TRANSLATION_ENTRY(WORLD_79);
    TRANSLATION_ENTRY(WORLD_80);
    TRANSLATION_ENTRY(WORLD_81);
    TRANSLATION_ENTRY(WORLD_82);
    TRANSLATION_ENTRY(WORLD_83);
    TRANSLATION_ENTRY(WORLD_84);
    TRANSLATION_ENTRY(WORLD_85);
    TRANSLATION_ENTRY(WORLD_86);
    TRANSLATION_ENTRY(WORLD_87);
    TRANSLATION_ENTRY(WORLD_88);
    TRANSLATION_ENTRY(WORLD_89);
    TRANSLATION_ENTRY(WORLD_90);
    TRANSLATION_ENTRY(WORLD_91);
    TRANSLATION_ENTRY(WORLD_92);
    TRANSLATION_ENTRY(WORLD_93);
    TRANSLATION_ENTRY(WORLD_94);
    TRANSLATION_ENTRY(WORLD_95);
    TRANSLATION_ENTRY(KP0);
    TRANSLATION_ENTRY(KP1);
    TRANSLATION_ENTRY(KP2);
    TRANSLATION_ENTRY(KP3);
    TRANSLATION_ENTRY(KP4);
    TRANSLATION_ENTRY(KP5);
    TRANSLATION_ENTRY(KP6);
    TRANSLATION_ENTRY(KP7);
    TRANSLATION_ENTRY(KP8);
    TRANSLATION_ENTRY(KP9);
    TRANSLATION_ENTRY(KP_PERIOD);
    TRANSLATION_ENTRY(KP_DIVIDE);
    TRANSLATION_ENTRY(KP_MULTIPLY);
    TRANSLATION_ENTRY(KP_MINUS);
    TRANSLATION_ENTRY(KP_PLUS);
    TRANSLATION_ENTRY(KP_ENTER);
    TRANSLATION_ENTRY(KP_EQUALS);
    TRANSLATION_ENTRY(UP);
    TRANSLATION_ENTRY(DOWN);
    TRANSLATION_ENTRY(RIGHT);
    TRANSLATION_ENTRY(LEFT);
    TRANSLATION_ENTRY(INSERT);
    TRANSLATION_ENTRY(HOME);
    TRANSLATION_ENTRY(END);
    TRANSLATION_ENTRY(PAGEUP);
    TRANSLATION_ENTRY(PAGEDOWN);
    TRANSLATION_ENTRY(F1);
    TRANSLATION_ENTRY(F2);
    TRANSLATION_ENTRY(F3);
    TRANSLATION_ENTRY(F4);
    TRANSLATION_ENTRY(F5);
    TRANSLATION_ENTRY(F6);
    TRANSLATION_ENTRY(F7);
    TRANSLATION_ENTRY(F8);
    TRANSLATION_ENTRY(F9);
    TRANSLATION_ENTRY(F10);
    TRANSLATION_ENTRY(F11);
    TRANSLATION_ENTRY(F12);
    TRANSLATION_ENTRY(F13);
    TRANSLATION_ENTRY(F14);
    TRANSLATION_ENTRY(F15);
    TRANSLATION_ENTRY(NUMLOCK);
    TRANSLATION_ENTRY(CAPSLOCK);
    TRANSLATION_ENTRY(SCROLLOCK);
    TRANSLATION_ENTRY(RSHIFT);
    TRANSLATION_ENTRY(LSHIFT);
    TRANSLATION_ENTRY(RCTRL);
    TRANSLATION_ENTRY(LCTRL);
    TRANSLATION_ENTRY(RALT);
    TRANSLATION_ENTRY(LALT);
    TRANSLATION_ENTRY(RMETA);
    TRANSLATION_ENTRY(LMETA);
    TRANSLATION_ENTRY(LSUPER);
    TRANSLATION_ENTRY(RSUPER);
    TRANSLATION_ENTRY(MODE);
    TRANSLATION_ENTRY(COMPOSE);
    TRANSLATION_ENTRY(HELP);
    TRANSLATION_ENTRY(PRINT);
    TRANSLATION_ENTRY(SYSREQ);
    TRANSLATION_ENTRY(BREAK);
    TRANSLATION_ENTRY(MENU);
    TRANSLATION_ENTRY(POWER);
    TRANSLATION_ENTRY(EURO);
    TRANSLATION_ENTRY(UNDO);
}

}
