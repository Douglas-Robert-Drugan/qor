#ifndef _ITEXTURE_H_KRCF74DT
#define _ITEXTURE_H_KRCF74DT

#include "Resource.h"
#include "Graphics.h"

class Pass;
class ITexture:
    public Resource
{
    public:
        ITexture() {}
        ITexture(const std::string& fn):
            Resource(fn)
        {}
        virtual ~ITexture() {}
        //virtual unsigned int id(Pass* pass = nullptr) const {
        //    return 0;
        //}
        virtual void bind(Pass* pass, unsigned slot=0) const {}
        virtual void unbind(Pass* pass) const {}
        virtual void bind_nomaterial(Pass* pass, unsigned slot=0) const {}
        virtual operator bool() const = 0;
        
        virtual glm::uvec2 size() const = 0;
        virtual void size(unsigned w, unsigned h) = 0;
        virtual glm::uvec2 center() const = 0;

        virtual std::string name() const = 0;
        virtual std::string filename() const = 0;

        virtual Color ambient() = 0;
        virtual Color diffuse() = 0;
        virtual Color specular() = 0;
        virtual Color emissive() = 0;

    private:
};

#endif

