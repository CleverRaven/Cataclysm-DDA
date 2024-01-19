#include <memory>

#include "character_id.h"
#include "item.h"
#include "itype.h"
#include "magic.h"
#include "npc.h"
#include "pimpl.h"
#include "point.h"
#include "talker_topic.h"
#include "vehicle.h"

std::string talker_topic::disp_name() const
{
    debugmsg( "This talker doesn't have a name." );
    return "";
}

int talker_topic::posx() const
{
    debugmsg( "This talker doesn't have a posx." );
    return 0;
}

int talker_topic::posy() const
{
    debugmsg( "This talker doesn't have a posy." );
    return 0;
}

int talker_topic::posz() const
{
    debugmsg( "This talker doesn't have a posz." );
    return 0;
}

tripoint talker_topic::pos() const
{
    debugmsg( "This talker doesn't have a pos." );
    return tripoint_zero;
}

tripoint_abs_ms talker_topic::global_pos() const
{
    debugmsg( "This talker doesn't have a global_pos." );
    return tripoint_abs_ms( tripoint_zero );
}

tripoint_abs_omt talker_topic::global_omt_location() const
{
    debugmsg( "This talker doesn't have a global_omt_location." );
    return tripoint_abs_omt( tripoint_zero );
}

void talker_topic::set_value( const std::string &, const std::string & )
{
    debugmsg( "This talker doesn't have values to set." );
}

void talker_topic::remove_value( const std::string & )
{
    debugmsg( "This talker doesn't have values to remove." );
}

std::vector<std::string> talker_topic::get_topics( bool )
{
    return topics;
}

bool talker_topic::will_talk_to_u( const Character &, bool )
{
    return true;
}

std::unique_ptr<talker> get_talker_for( const std::vector<std::string> &topics )
{
    return std::make_unique<talker_topic>( topics );
}
