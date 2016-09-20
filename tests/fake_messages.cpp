#include "messages.h"

/**
 * Stubs to turn all Messages calls into no-ops for unit testing.
 */

class Messages::impl_t{};

Messages::Messages(){}
Messages::~Messages(){}

std::vector<std::pair<std::string, std::string>> Messages::recent_messages(size_t)
{
    return std::vector<std::pair<std::string, std::string>>();
}
void Messages::vadd_msg(const char *, va_list){}
void Messages::vadd_msg(game_message_type, const char *, va_list){}
void Messages::clear_messages(){}
size_t Messages::size()
{
    return 0;
}
bool Messages::has_undisplayed_messages()
{
    return false;
}
void Messages::display_messages(){}
void Messages::display_messages(WINDOW *, int, int, int, int){}
void Messages::serialize(JsonOut &){}
void Messages::deserialize(JsonObject &){}

void add_msg(const char *, ...){}

void add_msg(game_message_type, const char *, ...){}
