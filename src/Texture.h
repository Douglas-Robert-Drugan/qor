#ifndef _TEXTURE_H
#define _TEXTURE_H

#include "Common.h"
#include <glm/glm.hpp>
#include <string>
#include "kit/kit.h"
#include "kit/log/errors.h"
#include "Filesystem.h"
#include "ITexture.h"
#include "Graphics.h"
#include "Pass.h"
#include "kit/cache/icache.h"
#include "Headless.h"

class Texture:
    public ITexture
{
    public:

        enum Flags {
            TRANS = kit::bit(0),
            MIPMAP  = kit::bit(1),
            CLAMP = kit::bit(2),
            FILTER = kit::bit(3)
            //FLIP = BIT(3)
        };

        static unsigned DEFAULT_FLAGS;
        static float ANISOTROPY;

        Texture(unsigned int _m_ID = 0):
            m_ID(_m_ID) {}
        
        Texture(Texture&& t):
            m_ID(t.leak()),
            m_Filename(std::move(t.m_Filename))
        {}
        Texture(const Texture&) = delete;
        Texture& operator=(Texture&& t) {
            unload();
            m_ID = t.leak();
            m_Filename = std::move(t.m_Filename);
            m_Size = t.m_Size;
            return *this;
        }
        Texture& operator=(const Texture&) = delete;

        Texture(const std::string& fn, unsigned int flags = DEFAULT_FLAGS);
        Texture(const std::tuple<std::string, ICache*>& args):
            Texture(std::get<0>(args))
        {}
        void unload();
        virtual ~Texture();

        /*
         * Returns true if texture id is non-zero
         * Should only false after Texture has been leak()ed
         */
        bool good() const { return m_ID || Headless::enabled(); }
        virtual operator bool() const override { return m_ID || Headless::enabled(); }

        /*
         * Return OpenGL Texture ID for the given pass
         */
        //virtual unsigned int id(Pass* pass = nullptr) const override {
        //    return m_ID;
        //}
        virtual void bind(Pass* pass, unsigned slot=0) const override {
            if(Headless::enabled())
                return;
            pass->texture(m_ID, slot);
        }
        virtual void bind_nomaterial(Pass* pass, unsigned slot=0) const override {
            if(Headless::enabled())
                return;
            pass->texture(m_ID, slot);
        }

        virtual unsigned int& id_ref() {
            return m_ID;
        }

        /*
         * Release the responsibility of the ID
         */
        unsigned int leak() {
            unsigned int id = m_ID;
            m_ID = 0;
            return id;
        }

        virtual glm::uvec2 size() const override { return m_Size; }
        virtual void size(unsigned w, unsigned h) override { m_Size=glm::uvec2(w,h); }
        virtual glm::uvec2 center() const override { return m_Size/2u; }

        virtual std::string filename() const override {
            return m_Filename;
        }
        virtual std::string name() const override {
            return m_Filename;
        }

        static void set_default_flags(unsigned);
        static void set_anisotropy(float a);

        virtual Color ambient() override { return Color::white(1.0f); }
        virtual Color diffuse() override { return Color::white(1.0f); }
        virtual Color specular() override { return Color::white(1.0f); }
        virtual Color emissive() override { return Color::white(0.0f); }
        
    protected:
        
        unsigned int load(std::string fn, unsigned int flags = DEFAULT_FLAGS);
        unsigned int m_ID = 0;
        std::string m_Filename;
        //ResourceCache<Texture>* m_Cache = nullptr;
        glm::uvec2 m_Size;
};

#endif

