#include <memory>

#include "talker_topic.h"

std::vector<std::string> talker_topic_const::get_topics( bool ) const
{
    return topics;
}

std::unique_ptr<talker> get_talker_for( const std::vector<std::string> &topics )
{
    return std::make_unique<talker_topic>( topics );
}
