#include "Sound.h"
#include "Resource.h"
using namespace std;

Sound :: Sound(const std::string& fn, Cache<Resource, std::string>* cache):
    Node(fn)
{
    if(Filesystem::getExtension(fn) == "json")
        m_bStream = m_pConfig->at<bool>("stream", true);
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
        m_pSource->refresh();
    }else{
        m_pBuffer = cache->cache_as<Audio::Buffer>(fn);
        m_pSource = std::make_shared<Audio::Source>();
        m_pSource->bind(m_pBuffer.get());
        m_pSource->refresh();
    }
}

void Sound :: logic_self(Freq::Time t)
{
    if(m_pSource)
        m_pSource->update();
}

