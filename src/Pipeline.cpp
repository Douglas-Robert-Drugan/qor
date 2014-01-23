#include "Pipeline.h"
#include "BasicPartitioner.h"
#include "Window.h"
#include <cassert>
#include <cmath>
#include <glm/glm.hpp>
#include "GLTask.h"
#include "Camera.h"
//#include <glm/gtc/matrix_transform.hpp>

using namespace std;

const std::vector<std::string> Pipeline :: s_TextureUniformNames = {
    "Nrm",
    "Disp",
    "Spec",
    "Occ"
};

const std::vector<std::string> Pipeline :: s_AttributeNames = {
    "Position",
    "Wrap",
    "Normal"
};

Pipeline :: Pipeline(
    Window* window,
    Cache<Resource, std::string>* cache,
    const shared_ptr<Node>& root,
    const shared_ptr<Node>& camera
):
    m_pWindow(window),
    m_pCache(cache),
    m_pRoot(root),
    m_pCamera(camera)
{
    assert(m_pWindow);
    //assert(m_pCamera.lock());
    //assert(m_pRoot.lock());

    m_pPartitioner = make_shared<BasicPartitioner>();

    m_ActiveShader = PassType::NORMAL;
    GL_TASK_START()
        
        load_shaders({"base", "basic"}); // base, basic, lit

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_MULTISAMPLE);
        //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_CULL_FACE);
        
        for(auto&& slot: m_Shaders) {
            slot->m_ModelViewProjectionID = slot->m_pShader->uniform(
                "ModelViewProjection"
            );
            slot->m_ModelViewID = slot->m_pShader->uniform(
                "ModelView"
            );
            slot->m_NormalID = slot->m_pShader->uniform(
                "NormalMatrix"
            );
            
            for(int i=0;i < s_TextureUniformNames.size() + 1;++i) {
                int tex_id = slot->m_pShader->uniform(
                    (boost::format("Texture%s")%(
                        i?
                            s_TextureUniformNames.at(i-1)
                        :
                            ""
                    )).str()
                );
                if(tex_id == -1)
                    break;
                slot->m_Textures.resize(i+1);
                slot->m_Textures.at(i) = tex_id;
            }
        }
         
        assert(glGetError() == GL_NO_ERROR);
    GL_TASK_END()
    //glEnable(GL_POLYGON_SMOOTH); // don't use this for 2D

    ortho(true);

    //float* f =  glm::value_ptr(m_ProjectionMatrix);
    //f[0] = std::round(f[i]);

    //m_ProjectionMatrix = glm::perspective(
    //    80.0f,
    //    16.0f / 9.0f,
    //    -101.0f,
    //    100.0f
    //);
}

Pipeline :: ~Pipeline()
{
    GL_TASK_START()
        // not sure if i need this
        layout(0);
        texture_slots(~0,8);
    GL_TASK_END()
}

void Pipeline :: load_shaders(vector<string> names)
{
    m_Shaders.clear();

    //const string path = "shaders/";
    //for(auto&& name: names) {
    //    std::shared_ptr<PipelineShader> s(make_shared<Program>(
    //        make_shared<Shader>(path + name + ".vp", Shader::VERTEX),
    //        make_shared<Shader>(path + name + ".fp", Shader::FRAGMENT)
    //    ));
    //    m_Shaders.push_back(std::move(s));
    //}
    for(auto&& name: names)
    {
        auto shader = m_pCache->cache_as<PipelineShader>(name+".json");
        m_Shaders.push_back(shader);
        
        unsigned layout = 0;
        unsigned i = 0;
        for(auto&& attr_name: s_AttributeNames) {
            auto attr_id = shader->m_pShader->attribute((boost::format("Vertex%s")%
                attr_name
            ).str());
            if(attr_id != (unsigned)-1) {
                //LOGf("attr: %s (%s)", attr_name % attr_id);
                shader->m_Attributes.resize(i+1);
                shader->m_Attributes.at(i) = attr_id;
                shader->m_SupportedLayout |= (1 << i);
            }
            else
                WARNINGf("missing attribute %s", attr_name);
            
            ++i;
        }
    }
    //m_Shaders.at((unsigned)m_ActiveShader)->m_pShader->use();
}

void Pipeline :: matrix(Pass* pass, const glm::mat4* m)
{
    m_ModelViewMatrix = m_ViewMatrix * *m;
    m_NormalMatrix = glm::transpose(glm::inverse(m_ModelViewMatrix));
    m_ModelViewProjectionMatrix = m_ProjectionMatrix * m_ModelViewMatrix;
    //m_ModelViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix * *m;
    
    GL_TASK_START()
        m_Shaders.at((unsigned)m_ActiveShader)->m_pShader->uniform(
            m_Shaders.at((unsigned)m_ActiveShader)->m_ModelViewProjectionID,
            m_ModelViewProjectionMatrix
        );
        m_Shaders.at((unsigned)m_ActiveShader)->m_pShader->uniform(
            m_Shaders.at((unsigned)m_ActiveShader)->m_ModelViewID,
            m_ModelViewMatrix
        );
        m_Shaders.at((unsigned)m_ActiveShader)->m_pShader->uniform(
            m_Shaders.at((unsigned)m_ActiveShader)->m_NormalID,
            m_NormalMatrix
        );
    GL_TASK_END()
}

void Pipeline :: texture(
    unsigned id, unsigned slot
){
    GL_TASK_START()
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, id);
        try{
            m_Shaders.at((unsigned)m_ActiveShader)->m_pShader->uniform(
                m_Shaders.at((unsigned)m_ActiveShader)->m_Textures.at(slot),
                (int)slot
            );
        }catch(...){}
    GL_TASK_END()
}

void Pipeline :: texture_nobind(
    unsigned slot
){
    GL_TASK_START()
        glActiveTexture(GL_TEXTURE0 + slot);
    GL_TASK_END()
}

void Pipeline :: render()
{
    assert(m_pWindow);
    if(!m_pRoot.lock())
        return;
    if(!m_pCamera.lock())
        return;

    m_ViewMatrix = glm::inverse(*m_pCamera.lock()->matrix_c(Space::WORLD));
    //m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    
    GL_TASK_START()
    std::shared_ptr<Node> root = m_pRoot.lock();
    assert(glGetError() == GL_NO_ERROR);

    // set up initial state
    //glViewport(0,0,m_pWindow->size().x/2,m_pWindow->size().y/2);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(m_BGColor.r(), m_BGColor.g(), m_BGColor.b(), 1.0f);
    glDisable(GL_BLEND);

    // RENDER ALL
    Pass pass(m_pPartitioner.get(), this, Pass::RECURSIVE | Pass::BASE);
    //m_pPartitioner->partition(root.get());
    shader(PassType::BASE);
    root->render(&pass);

    // RENDER ONLY VISIBLE
    //Pass pass(m_pPartitioner.get(), this, Pass::BASE);
    //for(auto&& node: m_pPartioner->visible_nodes())
    //    node->render_self(&pass);

    //glViewport(
    //    m_pWindow->size().x/2,m_pWindow->size().y/2,
    //    m_pWindow->size().x,m_pWindow->size().y
    //);
    // set up multi-pass state
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendFunc(GL_ONE, GL_ONE);
    glEnable(GL_BLEND);

    // render one pass (DEBUG only)
    pass.flags(pass.flags() &~Pass::BASE);
    shader(PassType::NORMAL);
    root->render(&pass);

    // render each light pass
    //if(m_pPartitioner->visible_lights().empty())
    //    root->render(&pass); // remove when you add lights
    // TODO: add this back when we impl visible_nodes
    //pass.flags(pass.flags() & ~Pass::RECURSIVE);
    //shader(PassType::NORMAL);
    //for(auto&& light: m_pPartitioner->visible_lights()) {
    //    root->render(&pass);
    //}

    GL_TASK_END()
}

void Pipeline :: ortho(bool o)
{
    auto camera = dynamic_pointer_cast<Camera>(m_pCamera.lock());
    if(o)
    {
        float aspect_ratio = static_cast<float>(m_pWindow->aspect_ratio());
        m_ProjectionMatrix = glm::ortho(
            0.0f,
            static_cast<float>(m_pWindow->size().x),
            0.0f,
            static_cast<float>(m_pWindow->size().y),
            -100.0f,
            100.0f
        );
    }
    else
    {
        float aspect_ratio = static_cast<float>(m_pWindow->aspect_ratio());
        m_ProjectionMatrix = glm::perspective(
            camera ? camera->fov() : m_DefaultFOV,
            16.0f / 9.0f,
            0.01f,
            1000.0f
        );
    }
}

void Pipeline :: shader(
    PassType style,
    std::shared_ptr<Program> shader
){
    //LOGf("style: %s", (unsigned)style);
    GL_TASK_START()
        assert(glGetError() == GL_NO_ERROR);
        m_ActiveShader = style;
        m_Shaders.at((unsigned)m_ActiveShader)->m_pShader->use();
        assert(glGetError() == GL_NO_ERROR);
    GL_TASK_END()
    
    //if(style != m_ActiveShader || (shader && shader != m_pCurrentShader)) {
    //    if(!shader)
    //        m_pCurrentShader = m_Shaders.at((unsigned)m_ActiveShader)->m_pShader;
    //    else
    //        m_pCurrentShader = shader;
    //    m_ActiveShader = style;
    //    m_OpenTextureSlots =
    //        m_Shaders.at((unsigned)m_ActiveShader).m_TextureSlots;
    //    GL_TASK_START()
    //        for(int i=0; i<m_OpenTextureSlots; ++i)
    //            texture(i, 0);
    //        m_pCurrentShader->use();
    //    GL_TASK_END()
    //}
}

void Pipeline :: shader(std::shared_ptr<Program> p)
{
    shader(m_ActiveShader, p);
}

void Pipeline :: shader(std::nullptr_t)
{
    shader(std::shared_ptr<Program>());
}

std::shared_ptr<Program> Pipeline :: shader(unsigned slot) const
{
    return m_Shaders.at(slot)->m_pShader;
}

unsigned Pipeline :: layout(unsigned attrs)
{
    auto& cur_layout = m_Shaders.at((unsigned)m_ActiveShader)->m_Layout;

    // get compatible layout
    attrs &= m_Shaders.at((unsigned)m_ActiveShader)->m_SupportedLayout;
    
    for(unsigned i=0; i < (unsigned)AttributeID::MAX; ++i)
    //for(unsigned i=0; cur_layout!=attrs; ++i)
    {
        unsigned bit = 1U << i;
        //assert(bit < (unsigned)AttributeID::MAX);
        unsigned abit = attrs & bit;
        //unsigned abit = 1;
        
        //if((cur_layout&bit) != abit)
        //{
            if(abit) {
                glEnableVertexAttribArray(i);
                //LOGf("enable: %s", i);
                cur_layout |= bit;
            } else {
                glDisableVertexAttribArray(i);
                //LOGf("disable: %s", i);
                cur_layout &= ~bit;
            }
            
            //cur_layout ^= bit;
        //}
    }
    
    return attrs;
    
    //m_Layout = attrs;
}

void Pipeline :: texture_slots(unsigned slot_flags, unsigned max_tex)
{
    assert(max_tex);
    
    auto& shader = m_Shaders.at((unsigned)m_ActiveShader);
    auto& cur_slots = shader->m_ActiveTextureSlots;
    
    GL_TASK_START()
        texture(0,0);
        //for(unsigned i = max_tex-1; cur_slots != slot_flags; --i) {
        //    const unsigned bit = 1 << i;
        //    const unsigned new_state = slot_flags & bit;
            
        //    // intended state differs?
        //    if((cur_slots & bit) != new_state) {
        //        if(!new_state)
        //            texture(0, i);
        //        //else
        //        //    texture_nobind(i);
                
        //        // update bit for slot, possible early termination
        //        cur_slots ^= bit;
        //    }
        //}
    GL_TASK_END()
}

unsigned Pipeline :: attribute_id(AttributeID id)
{
    return m_Shaders.at((unsigned)m_ActiveShader)->
        m_Attributes.at((unsigned)id);
}

