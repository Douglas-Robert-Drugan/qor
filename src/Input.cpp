#include "Common.h"
#include "Input.h"
#include "Window.h"
//#include <CEGUI/System.h>
//#include <CEGUI/GUIContext.h>
//#include <CEGUI/InjectedInputReceiver.h>

Input :: Input(Window* window):
    m_pWindow(window)
{
    m_DummySwitch.make_dummy();
    
    m_bRelMouse = !m_bRelMouse; // undo initial state
    relative_mouse(!m_bRelMouse); // redo initial state change
    
    //SDL_SetRelativeMouseMode(SDL_TRUE);
    //SDL_SetWindowGrab(SDL_GL_GetCurrentWindow(), SDL_TRUE);
}

void Input :: Switch :: trigger()
{
    // TODO: add flag to method for overwriting history or modifying front?
    m_Records.push_back(Record());
    //m_Records.emplace_back();

    for(auto& c: m_Controllers)
        if(!c.expired())
        {
            auto s = c.lock();
            if(s)
                s->trigger();
            else
                c.reset();
        }
}

void Input :: logic(Freq::Time t)
{
    SDL_Event ev;
    //auto& gui = CEGUI::System::getSingleton().getDefaultGUIContext();
    //CEGUI::System::getSingleton().injectTimePulse(t.s());
    //gui.injectTimePulse(t.s());
    m_MouseRel = glm::ivec2();
    
    while(SDL_PollEvent(&ev))
    {
        switch(ev.type)
        {
            case SDL_QUIT:
                m_bQuit = true;
                break;

            case SDL_WINDOWEVENT:
                if(ev.window.event == SDL_WINDOWEVENT_RESIZED)
                    m_pWindow->on_resize(glm::ivec2(ev.window.data1, ev.window.data2));
                break;

            case SDL_KEYDOWN:
                m_Keys[ev.key.keysym.sym] = true;
                //gui.injectKeyDown((CEGUI::Key::Scan)ev.key.keysym.scancode);
                break;

            case SDL_KEYUP:
                m_Keys[ev.key.keysym.sym] = false;
                //gui.injectKeyUp((CEGUI::Key::Scan)ev.key.keysym.scancode);
                break;

            case SDL_TEXTINPUT:
            {
                unsigned idx=0;
                //while(ev.text.text[idx++])
                    //gui.injectChar(ev.text.text[idx]);
                break;
            }
            case SDL_MOUSEMOTION:
                if(m_bRelMouse)
                    m_MouseRel += glm::vec2(ev.motion.xrel, ev.motion.yrel);
                else
                    m_MouseRel = m_MousePos;
                
                m_MousePos = glm::vec2(ev.motion.x, ev.motion.y);
                
                if(!m_bRelMouse)
                    m_MouseRel -= m_MousePos; // simulate mouse rel
                
                //gui.injectMousePosition(
                //    static_cast<float>(ev.motion.x),
                //    static_cast<float>(ev.motion.y)
                //);
                break;

            case SDL_MOUSEBUTTONDOWN:
            {
                if(ev.button.button == SDL_BUTTON_LEFT) {
                    m_Mouse[0] = true;
                    //gui.injectMouseButtonDown(CEGUI::MouseButton::LeftButton);
                }
                else if(ev.button.button == SDL_BUTTON_RIGHT){
                    m_Mouse[1] = true;
                    //gui.injectMouseButtonDown(CEGUI::MouseButton::RightButton);
                }
                else if(ev.button.button == SDL_BUTTON_MIDDLE){
                    m_Mouse[2] = true;
                    //gui.injectMouseButtonDown(CEGUI::MouseButton::MiddleButton);
                }

                break;
            }

            case SDL_MOUSEBUTTONUP:
            {
                if(ev.button.button == SDL_BUTTON_LEFT){
                    m_Mouse[0] = false;
                    //gui.injectMouseButtonUp(CEGUI::MouseButton::LeftButton);
                }
                else if(ev.button.button == SDL_BUTTON_RIGHT){
                    m_Mouse[1] = false;
                    //gui.injectMouseButtonUp(CEGUI::MouseButton::RightButton);
                }
                else if(ev.button.button == SDL_BUTTON_MIDDLE){
                    m_Mouse[2] = false;
                    //gui.injectMouseButtonUp(CEGUI::MouseButton::MiddleButton);
                }
                break;
            }
            //case SDL_WINDOWEVENT_RESIZED:
            //    break;
        }
    }

    for(auto& c: m_Controllers)
    {
        if(c.second->triggered())
        {
            c.second->event();
            c.second->untrigger();
        }
        c.second->logic(t);
    }
    
    if(m_bRelMouse)
        SDL_WarpMouseInWindow(
            m_pWindow->sdl_window(),
            m_pWindow->center().x/2,
            m_pWindow->center().y/2
        );
}

void Controller :: rumble(float magnitude, Freq::Time t)
{
    // TODO: add rumble support
    // http://wiki.libsdl.org/moin.cgi/CategoryForceFeedback

    //SDL_HapticRumblePlay(m_pHaptic, magnitude, t.ms());
}

