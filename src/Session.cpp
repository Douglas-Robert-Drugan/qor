#include "Session.h"
#include "kit/log/log.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
using namespace boost;
using namespace boost::filesystem;

Session :: Session(Input* input):
    m_pInput(input)
{
    assert(m_pInput);

    // auto-intialize profile if there's only one
    auto profiles = saved_profiles();
    if(profiles.size()==1)
        plug(profiles[0]);
}

std::vector<std::string> Session :: saved_profiles() const
{
    std::vector<std::string> profiles;
    path profile_dir("profiles");

    for(directory_iterator itr(profile_dir), e;
        itr != e;
        ++itr)
    {
        auto profile = itr->path();
        //LOG(profile.string());
        if(is_regular_file(profile) &&
            ends_with(to_lower_copy(profile.string()), ".json"))
        {
            profiles.push_back(profile.string());
        }
    } 
    return profiles;
}

