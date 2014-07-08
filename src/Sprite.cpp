#include "Sprite.h"
#include "kit/log/errors.h"
#include "Filesystem.h"
#include "kit/log/log.h"
#include <memory>
using namespace std;
using namespace glm;

Sprite :: Sprite(
    const string& fn,
    Cache<Resource, std::string>* resources,
    const string& skin,
    glm::vec3 pos
):
    m_sPath(fn),
    m_pResources(resources),
    m_sMeshMaterial(skin),
    Node(fn)
{
    position(pos);
    
    auto exts = vector<string> {
        // add extensions to Qor::resolve_resource() if you want them supported through factory
        "png",
        //"bmp",
        //"tga"
    };
    if(Filesystem::hasExtension(fn, "json"))
    {
        load_as_json(fn, resources);
        return;
    }
    
    bool done = false;
    for(auto&& ext: exts)
    {
        if(Filesystem::hasExtension(fn, ext)) // case insens
        {
            load_as_image(fn, resources);
            done = true;
            break;
        }
    }
    
    if(!done)
        ERRORf(READ,
            "%s has invalid sprite extension",
            Filesystem::getFileName(fn)
        );
}

// TODO: add config file lookup to get default skin (if no name is provided)
void Sprite :: reskin(const string& skin/* = string()*/)
{
    // TODO: check that this is a valid skin listed in config file?
    m_pResources->cache(Filesystem::getPath(m_sPath)+skin+".png");
    m_sMeshMaterial = skin;
}

void Sprite :: load_as_json(
    const string& fn,
    Cache<Resource, std::string>* resources
){
    assert(resources);

    auto buf = Filesystem::file_to_buffer(fn);
    if(buf.empty())
        ERROR(READ, fn);
    Json::Value root;
    Json::Reader reader;
    if(!reader.parse(&buf[0], root))
        ERROR(PARSE, fn);
    if(!root.isObject())
        ERROR(PARSE, fn);

    //TODO: load color key before loading texture
    //Json::Value colors = root.get();

    //TODO: if there's a skins array, ensure provided skin is valid or blank
    //  If blank, pick the first listed skin

    if(m_sMeshMaterial.empty())
        m_pTexture = resources->cache_as<Texture>(Filesystem::cutExtension(fn)+".png");
    else
        m_pTexture = resources->cache_as<Texture>(Filesystem::getPath(fn)+m_sMeshMaterial+".png");

    load_mesh();

    Json::Value size = root.get("size",Json::Value());
    // TODO: if size is null, just use the texture size
    if(!size.isNull())
    {
        if(size.isArray()) {
            auto scale = vec3(
                size.get((unsigned int)0, 16).asDouble(),
                size.get((unsigned int)1, 16).asDouble(),
                1.0f
            );
            *m_pMesh->matrix() = glm::scale(*m_pMesh->matrix(),scale);
            //pend(); // automatic
        }
        else
            ERROR(PARSE, fn);
    }
    else
        m_Size = m_pTexture->size();

    size = root.get("tile-size",Json::Value());
    if(!size.isNull())
    {
        if(!size.isArray())
            ERROR(PARSE, fn);

        // overwrite m_Size to be tile size
        m_Size = glm::uvec2(
            size.get((unsigned int)0, 16).asUInt(),
            size.get((unsigned int)1, 16).asUInt()
        );

        load_type(fn, root.get("type", Json::Value()));
        load_animation(fn, root.get("animation", Json::Value()));
    }
    else
    {
        ERROR(PARSE, fn);
    }

    // do this regardless of tileset or plain image sprites
    load_cycles();
    center_mesh();
}

void Sprite :: load_as_image(
    const string& fn,
    Cache<Resource, std::string>* resources
){
    if(m_sMeshMaterial.empty())
        m_pTexture = resources->cache_as<Texture>(fn);
    else
        m_pTexture = resources->cache_as<Texture>(Filesystem::getPath(fn)+m_sMeshMaterial+".png");

    load_mesh();
    m_Size = m_pTexture->size(); // use full image
    *m_pMesh->matrix() = glm::scale(*m_pMesh->matrix(),
        vec3(1.0f*m_Size.x, 1.0f*m_Size.y, 1.0f)
    );
    load_cycles(); // load base wrap
    center_mesh();
}

void Sprite :: load_mesh()
{
    m_pMesh = make_shared<Mesh>(
        make_shared<MeshGeometry>(Prefab::quad()),
        vector<shared_ptr<IMeshModifier>>{
            make_shared<Wrap>(Prefab::quad_wrap())
        },
        make_shared<MeshMaterial>(m_pTexture)
    );
    add(m_pMesh);
}

void Sprite :: load_type(const string& fn, const Json::Value& type)
{
    if(type.isNull())
        return;
}

void Sprite :: load_animation(
    const string& fn,
    const Json::Value& animation
){
    if(animation.isNull())
        return;

    m_AnimationSpeed = animation.get("speed", 1.0f).asDouble();
    if(floatcmp(m_AnimationSpeed, 0.0f))
        ERRORf(PARSE, "%s speed value is invalid", fn);
        //ERROR(PARSE, fn);

    const Json::Value frames = animation.get("frames", Json::Value());
    if(frames.isNull())
        return;

    // read all children first, then add states
    vector<unsigned int> states;
    load_frames(fn, states, frames);
}

void Sprite :: load_frames(
    const string& fn,
    vector<unsigned int>& states,
    const Json::Value& frames
){
    if(frames.isNull())
        ERROR(PARSE, fn);

    if(frames.isArray())
    {
        Hints hints;
        vector<unsigned int> sorted_states = states;
        sort(sorted_states.begin(), sorted_states.end());
        // below this, don't use *states*
        
        //for(auto frame = frames.begin();
        //    frame != frames.end();
        //    ++frame)
        for(unsigned int i = 0; i < frames.size(); ++i)
        {
            const auto& frame = frames[i];
            if(frame.isInt())
            {
                //LOGf("%s", frame.asInt());
                m_Cycles[sorted_states].frames.emplace_back(
                    frame.asInt(), hints
                );
            }
            else if(frame.isString())
            {
                // TODO: read in hints and add them to "hints"
                // TODO: read in empty object as reset
                if(frame.asString() == "hflip")
                    hints.hflip = true;
                else if(frame.asString() == "vflip")
                    hints.vflip = true;
            }
            else if(frame.isObject())
            {
                // TODO: check for speed hint (object {"speed": 0.5f})
            }
            else
                ERROR(PARSE, fn);
        }
        return;
    }

    for(auto frame = frames.begin();
        frame != frames.end();
        ++frame)
    {
        //if(frame.isNull() || !frame.isObject())
        //    ERROR(PARSE, fn);

        unsigned int id;
        try{
            // exist state?
            id = state_id(frame.key().asString());
        }catch(const out_of_range&){
            // new state
            m_Names.push_back(frame.key().asString());
            id = m_Names.size()-1;
            if(m_CategoryNames.find(states.size()) == m_CategoryNames.end())
                m_CategoryNames[states.size()] = string();
            m_StateCategory[id] = states.size();
        }

        //LOGf("%s: (%s)", frame.key().asString() % id);

        states.push_back(id);
        load_frames(fn, states, *frame);
        states.pop_back();
    }
}

void Sprite :: load_cycles()
{
    if(m_Cycles.empty())
    {
        // TODO: load default Wrap (use entire image as sprite)
        m_pMesh->add_modifier(make_shared<Wrap>(Prefab::quad_wrap()));
    }
    else
    {
        for(auto& c: m_Cycles)
            for(auto& f: c.second.frames)
            {
                //f.wrap = make_shared<Wrap>(Prefab::quad_wrap());
                uint32_t flags = 0;
                if(f.hints.hflip)
                    flags |= Prefab::H_FLIP;

                f.wrap = make_shared<Wrap>(Prefab::tile_wrap(
                    m_Size,
                    m_pTexture->size(),
                    f.state,
                    flags
                ));
            }
    }

    m_Viewer.alarm = Freq::Alarm(Freq::Time(1), &m_Viewer.timeline);
}

void Sprite :: logic_self(Freq::Time t)
{
    if(!m_Viewer.cycle)
        return;
    
    m_Viewer.timeline.speed(m_AnimationSpeed * m_PlaySpeed);
    m_Viewer.timeline.logic(t);

    unsigned int count = 0;
    while(m_Viewer.alarm->elapsed())
    {
        assert(count < 2); // sprite animation too fast for frame rate
        
        //LOGf("frame: %s", m_Viewer.frame);
        if(++m_Viewer.frame >= m_Viewer.cycle->frames.size())
            m_Viewer.frame = 0;
        Freq::Time excess = m_Viewer.alarm->excess();
        reset_cycle(m_Viewer.frame);
        m_Viewer.alarm->delay(Freq::Time::seconds(1));
        *m_Viewer.alarm += excess; // roll over excess time

        ++count;
    }
}

