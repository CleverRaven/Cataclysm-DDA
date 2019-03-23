#pragma once
#ifndef MESSAGES_H
#define MESSAGES_H

#include <string>
#include <utility>
#include <vector>

#include "string_formatter.h"

class JsonOut;
class JsonObject;

enum game_message_type : int;
namespace catacurses
{
class window;
} // namespace catacurses

namespace Messages
{
std::vector<std::pair<std::string, std::string>> recent_messages( size_t count );
void add_msg( std::string msg );
void add_msg( game_message_type type, std::string msg );
void clear_messages();
void deactivate();
size_t size();
bool has_undisplayed_messages();
void display_messages();
void display_messages( const catacurses::window &ipk_target, int left, int top, int right,
                       int bottom );
void serialize( JsonOut &json );
void deserialize( JsonObject &json );
} // namespace Messages

void add_msg( std::string msg );
template<typename ...Args>
inline void add_msg( const char *const msg, Args &&... args )
{
    return add_msg( string_format( msg, std::forward<Args>( args )... ) );
}

void add_msg( game_message_type type, std::string msg );
template<typename ...Args>
inline void add_msg( const game_message_type type, const char *const msg, Args &&... args )
{
    return add_msg( type, string_format( msg, std::forward<Args>( args )... ) );
}

#endif
