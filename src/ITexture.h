#ifndef _ITEXTURE_H_KRCF74DT
#define _ITEXTURE_H_KRCF74DT

#include "Resource.h"

class Pass;
class ITexture:
    public Resource
{
    public:
        virtual ~ITexture() {}
        //virtual unsigned int id(Pass* pass = nullptr) const {
        //    return 0;
        //}
        virtual void bind(Pass* pass, unsigned slot=0) const {}
        virtual operator bool() const = 0;
    private:
};

#endif

