#include <cstddef>
#include <string>
#include <utility>
#include <vector>

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
 * Stubs to turn all Messages calls into no-ops for unit testing.
 */

std::vector<std::pair<std::string, std::string>> Messages::recent_messages( size_t )
{
    return std::vector<std::pair<std::string, std::string>>();
}
void Messages::add_msg( std::string ) {}
void Messages::add_msg( const game_message_params &, std::string ) {}
void Messages::clear_messages() {}
void Messages::deactivate() {}
size_t Messages::size()
{
    return 0;
}
bool Messages::has_undisplayed_messages()
{
    return false;
}
void Messages::display_messages() {}
void Messages::display_messages( const catacurses::window &, int, int, int, int ) {}
void Messages::serialize( JsonOut & ) {}
void Messages::deserialize( const JsonObject & ) {}

void add_msg( std::string ) {}
void add_msg( const game_message_params &, std::string ) {}
void add_msg_if_player_sees( const tripoint &, std::string ) {}
void add_msg_if_player_sees( const Creature &, std::string ) {}
void add_msg_if_player_sees( const tripoint &, const game_message_params &, std::string ) {}
void add_msg_if_player_sees( const Creature &, const game_message_params &, std::string ) {}
