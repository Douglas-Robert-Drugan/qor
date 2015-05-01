#include "Sound.h"
#include "Resource.h"
using namespace std;

Sound :: Sound(const std::string& fn, Cache<Resource, std::string>* cache):
    Node(fn),
    m_pResources(cache)
{
    //auto vid_section = std::make_shared<MetaBase<kit::optional_mutex<std::recursive_mutex>>>();
    //m_pResources->config()->mutex().mutex = vid_section->mutex().mutex;
    //auto video_cfg = m_pResources->config()->ensure(
    //    "audio",  vid_section
    //);

    if(Filesystem::getExtension(fn) == "json")
    {
        m_bStream = m_pConfig->at<bool>("stream", false);
        //m_bAmbient = m_pConfig->at<bool>("ambient", false);
        //m_bAutoplay = m_pConfig->at<bool>("autoplay", false);
    }
    else if(Filesystem::getExtension(fn) == "wav")
        m_bStream = false;
    else if(Filesystem::getExtension(fn) == "ogg")
        m_bStream = true;
    else
        ERRORf(GENERAL,
            "Unable to recognize extension for \"%s\"",
            Filesystem::getFileName(fn)
        );
    
    if(m_bStream){
        m_pSource = cache->cache_as<Audio::Stream>(fn);
        //m_pSource->refresh();
    }else{
        m_pBuffer = cache->cache_as<Audio::Buffer>(fn);
        m_pSource = std::make_shared<Audio::Source>();
        m_pSource->bind(m_pBuffer.get());
        //m_pSource->refresh();
    }
    //if(m_bAutoplay)
    //    source()->play();
    if(m_pSource)
        m_pSource->refresh();
    
    string vol = m_bStream ? "music-volume" : "sound-volume";
    auto vol_cb = [this, vol] {
        int g = m_pResources->config()->meta("audio")->at<int>("volume", 100);
        int v = m_pResources->config()->meta("audio")->at<int>(vol, 100);
        float val = (g / 100.0f) * (v / 100.0f);
        m_pSource->gain = val;
    };
    m_VolCon = m_pResources->config()->meta("audio")->on_change(vol, vol_cb);
    m_MasterVolCon = m_pResources->config()->meta("audio")->on_change("volume", vol_cb);
}

Sound :: ~Sound()
{
}

void Sound :: logic_self(Freq::Time t)
{
    if(m_pSource)
    {
        m_pSource->pos = position(Space::WORLD);
        m_pSource->refresh(); // TODO: put this in Node::on_move
        m_pSource->update();
    }
}

void Sound :: play()
{
    if(m_pSource)
    {
        m_pSource->pos = position(Space::WORLD);
        m_pSource->refresh(); // TODO: put this in Node::on_move
        m_pSource->update();
        m_pSource->play();
    }
}

void Sound :: pause() { if(m_pSource) m_pSource->pause(); }
void Sound :: stop() { if(m_pSource) m_pSource->stop(); }

void Sound :: play(Node* parent, const std::string& fn, Cache<Resource, std::string>* resources)
{
    shared_ptr<Sound> snd;
    try{
        snd = make_shared<Sound>(fn, resources);
    }catch(...){
        WARNINGf("missing sound: %s", fn);
        return; // TEMP: ignore missing sounds
    }
    parent->add(snd);
    snd->play();
    auto sndptr = snd.get();
    snd->on_tick.connect([sndptr](Freq::Time t){
        if(not sndptr->source()->playing())
            sndptr->detach();
    });
}


