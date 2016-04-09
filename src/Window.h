#ifndef _WINDOW_H_HOFP8EQ7
#define _WINDOW_H_HOFP8EQ7

#include "Common.h"
#include "Shader.h"
#include "Pipeline.h"
#include "kit/args/args.h"
#include "kit/meta/meta.h"
#include <boost/optional.hpp>
#include "Resource.h"
#include "Headless.h"

class Window
{
    public:
        Window(
            const Args& args,
            Cache<Resource, std::string>* resources
        );
        virtual ~Window();
        void destroy();
        void render() const;

        float aspect_ratio() {
            glm::ivec2 r;
            if(not Headless::enabled())
                SDL_GetWindowSize(m_pWindow,&r[0],&r[1]);
            else
                return 1.0f;
            return (1.0f * r.x) / (1.0f * r.y);
        }

        glm::ivec2 size() const {
            glm::ivec2 r(0,0);
            if(not Headless::enabled())
                SDL_GetWindowSize(m_pWindow,&r[0],&r[1]);
            return r;
        }

        glm::ivec2 center() const {
            return size() / 2;
        }

        SDL_Window* sdl_window() {
            return m_pWindow;
        }
        
        boost::signals2::connection on_resize(const boost::signals2::signal<void()>::slot_type& cb);
        void resize(glm::ivec2);
        
    private:

        Cache<Resource, std::string>* m_pResources;
        SDL_Window* m_pWindow = nullptr;
        boost::optional<SDL_GLContext> m_GLContext;
        std::string m_Title;
};

#endif

