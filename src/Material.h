#ifndef _MATERIAL_H
#define _MATERIAL_H

#include <vector>
#include "Texture.h"
#include "kit/cache/cache.h"
#include "Graphics.h"

class Material:
    public ITexture
{
    public:
        
        Material(
            const std::string& fn,
            Cache<Resource, std::string>* cache = nullptr
        );
        Material(const std::tuple<std::string, ICache*>& args):
            Material(
                std::get<0>(args),
                (Cache<Resource, std::string>*) std::get<1>(args)
            )
        {}
        virtual ~Material();
        //virtual unsigned int id(Pass* pass = nullptr) const override;
        virtual void bind(Pass* pass, unsigned slot = 0) const override;

        static bool supported(
            std::string fn,
            Cache<Resource, std::string>* cache
        );

        enum ExtraMap {
            //DIFF = 0,
            NRM,
            DISP,
            SPEC,
            OCC
        };
        
        virtual operator bool() const override {
            return true; // TODO: check deeper?
        }
        
        virtual glm::uvec2 size() const override { return m_Textures.at(0)->size(); }
        virtual void size(unsigned w, unsigned h) override { m_Textures.at(0)->size(w,h); }
        virtual glm::uvec2 center() const override { return m_Textures.at(0)->center(); }
        
    private:

        const static std::vector<std::string> s_ExtraMapNames;
        
        void load_json(std::string fn);
        void load_mtllib(std::string fn, std::string emb);
        
        Cache<Resource, std::string>* m_pCache = nullptr;
        
        std::string m_Filename;
        std::vector<std::shared_ptr<ITexture>> m_Textures;
        bool m_bComposite = false;

        Color m_Ambient = Color(1.0f);
        Color m_Diffuse = Color(1.0f);
        Color m_Specular = Color(1.0f);

        glm::uvec2 m_Size;
};

#endif

