#ifndef _CAMERA_H_K8EZAF7N
#define _CAMERA_H_K8EZAF7N

#include "Tracker.h"
#include "kit/kit.h"
#include "kit/freq/animation.h"
#include "kit/factory/factory.h"
#include "kit/cache/cache.h"
#include <boost/signals2.hpp>

class Window;
class Camera:
    public Tracker
{
    public:

        Camera(const std::string& fn, IFactory* factory, ICache* cache);
        Camera(const std::tuple<std::string, IFactory*, ICache*>& args):
            Camera(std::get<0>(args), std::get<1>(args), std::get<2>(args))
        {
            init();
        }

        Camera() {init();}
        Camera(Window* w){
            init();
            window(w);
        }
        Camera(
            const std::shared_ptr<Node>& target
        ):
            Tracker(target)
        {
            init();
        }

        void ortho(bool origin_bottom = false);
        void perspective(float fov = 80.0f);
        void window(Window* window);

        virtual ~Camera() {}

        virtual void logic_self(Freq::Time t) override {
            Tracker::logic_self(t);
        }

        void fov(float f) {
            m_FOV=f;
            recalculate_projection();
        }
        float fov() const {
            return m_FOV;
        }

        bool in_frustum(const Box& box) const;
        bool in_frustum(glm::vec3 point) const;

        const glm::mat4& projection() const;
        const glm::mat4& view() const;
        
        enum Flag {
            ORTHO = kit::bit(0),
            BOTTOM_ORIGIN = kit::bit(1)
        };
        
        const unsigned DEFAULT_FLAGS = ORTHO;
        
    private:
        
        unsigned m_Flags = DEFAULT_FLAGS;

        bool m_bInited = false;

        void init();
        
        //void view_update() const;
        //void frustum_update() const;
        
        void recalculate_projection();
        
        float m_FOV = 80.0f;
        
        mutable glm::mat4 m_ProjectionMatrix;
        
        // TODO: cache and update on_pend
        mutable kit::lazy<glm::mat4> m_ViewMatrix;
        
        boost::signals2::scoped_connection m_WindowResize;

        //float m_ZNear;
        //float m_ZFar;

        bool m_bOrtho = true;
        bool m_bBottomOrigin = true;
        glm::ivec2 m_Size;
        //bool m_bWindingCW = false;

        mutable kit::lazy<Box> m_OrthoFrustum;
};

#endif

