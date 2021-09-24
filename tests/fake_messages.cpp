#include <cstddef>
#include <iosfwd>
#include <utility>
#include <vector>

#include "calendar.h"
#include "enums.h"
#include "messages.h"

class Creature;
class JsonObject;
class JsonOut;
struct tripoint;

namespace catacurses
{
class window;
}  // namespace catacurses

/**
 * For unit testing we just store all messages so they can be dumped in the
 * event of a test failure.
 */

static std::vector<std::pair<std::string, std::string>> messages;

std::vector<std::pair<std::string, std::string>> Messages::recent_messages( size_t )
{
    return messages;
}
void Messages::add_msg( std::string m )
{
    messages.emplace_back( to_string_time_of_day( calendar::turn ), m );
}
void Messages::add_msg( const game_message_params &, std::string m )
{
    add_msg( m );
}
void Messages::add_msg_debug( debugmode::debug_filter, std::string m )
{
    // cata_test does not need filters
    add_msg( m );
}
void Messages::clear_messages()
{
    messages.clear();
}
void Messages::deactivate() {}
size_t Messages::size()
{
    return messages.size();
}
bool Messages::has_undisplayed_messages()
{
    return false;
}
void Messages::display_messages() {}
void Messages::display_messages( const catacurses::window &, int, int, int, int ) {}
void Messages::serialize( JsonOut & ) {}
void Messages::deserialize( const JsonObject & ) {}

void add_msg( std::string m )
{
    Messages::add_msg( m );
}
void add_msg( const game_message_params &, std::string m )
{
    Messages::add_msg( m );
}
void add_msg_debug( debugmode::debug_filter, std::string m )
{
    Messages::add_msg( m );
}
void add_msg_if_player_sees( const tripoint &, std::string m )
{
    Messages::add_msg( m );
}
void add_msg_if_player_sees( const Creature &, std::string m )
{
    Messages::add_msg( m );
}
void add_msg_if_player_sees( const tripoint &, const game_message_params &, std::string m )
{
    Messages::add_msg( m );
}
void add_msg_if_player_sees( const Creature &, const game_message_params &, std::string m )
{
    Messages::add_msg( m );
}
void add_msg_debug_if_player_sees( const tripoint &, debugmode::debug_filter,
                                   std::string m )
{
    Messages::add_msg( m );
}
void add_msg_debug_if_player_sees( const Creature &, debugmode::debug_filter,
                                   std::string m )
{
    Messages::add_msg( m );
}
