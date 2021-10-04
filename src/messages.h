#pragma once
#ifndef CATA_SRC_MESSAGES_H
#define CATA_SRC_MESSAGES_H

#include <cstddef>
#include <iosfwd>
#include <type_traits>
#include <utility>
#include <vector>

#include "debug.h"
#include "enums.h"
#include "string_formatter.h"

class Creature;
class JsonObject;
class JsonOut;
class translation;
struct tripoint;

namespace catacurses
{
class window;
} // namespace catacurses

namespace Messages
{
std::vector<std::pair<std::string, std::string>> recent_messages( size_t count );
void add_msg( std::string msg );
void add_msg( const game_message_params &params, std::string msg );
void add_msg_debug( debugmode::debug_filter type, std::string msg );
void clear_messages();
void deactivate();
size_t size();
bool has_undisplayed_messages();
void display_messages();
void display_messages( const catacurses::window &ipk_target, int left, int top, int right,
                       int bottom );
void serialize( JsonOut &json );
void deserialize( const JsonObject &json );
} // namespace Messages

void add_msg( std::string msg );
template<typename ...Args>
inline void add_msg( const std::string &msg, Args &&... args )
{
    return add_msg( string_format( msg, std::forward<Args>( args )... ) );
}
template<typename ...Args>
inline void add_msg( const char *const msg, Args &&... args )
{
    return add_msg( string_format( msg, std::forward<Args>( args )... ) );
}
template<typename ...Args>
inline void add_msg( const translation &msg, Args &&... args )
{
    return add_msg( string_format( msg, std::forward<Args>( args )... ) );
}

void add_msg( const game_message_params &params, std::string msg );
template<typename ...Args>
inline void add_msg( const game_message_params &params, const std::string &msg, Args &&... args )
{
    if( params.type == m_debug && !debug_mode ) {
        return;
    }
    return add_msg( params, string_format( msg, std::forward<Args>( args )... ) );
}
template<typename ...Args>
inline void add_msg( const game_message_params &params, const char *const msg, Args &&... args )
{
    if( params.type == m_debug && !debug_mode ) {
        return;
    }
    return add_msg( params, string_format( msg, std::forward<Args>( args )... ) );
}

void add_msg_debug( debugmode::debug_filter type, std::string msg );
template<typename ...Args>
inline void add_msg_debug( debugmode::debug_filter type, const std::string &msg, Args &&... args )
{
    // expanding for string formatting can be expensive
    if( debug_mode ) {
        return add_msg_debug( type, string_format( msg, std::forward<Args>( args )... ) );
    }
}
template<typename ...Args>
inline void add_msg_debug( debugmode::debug_filter type, const char *const msg, Args &&... args )
{
    if( debug_mode ) {
        return add_msg_debug( type, string_format( msg, std::forward<Args>( args )... ) );
    }
}

void add_msg_if_player_sees( const tripoint &target, std::string msg );
void add_msg_if_player_sees( const Creature &target, std::string msg );
template<typename ...Args>
inline void add_msg_if_player_sees( const tripoint &target, const std::string &msg,
                                    Args &&... args )
{
    return add_msg_if_player_sees( target, string_format( msg, std::forward<Args>( args )... ) );
}
template<typename ...Args>
inline void add_msg_if_player_sees( const Creature &target, const std::string &msg,
                                    Args &&... args )
{
    return add_msg_if_player_sees( target, string_format( msg, std::forward<Args>( args )... ) );
}
template<typename ...Args>
inline void add_msg_if_player_sees( const tripoint &target, const char *const msg, Args &&... args )
{
    return add_msg_if_player_sees( target, string_format( msg, std::forward<Args>( args )... ) );
}
template<typename ...Args>
inline void add_msg_if_player_sees( const Creature &target, const char *const msg, Args &&... args )
{
    return add_msg_if_player_sees( target, string_format( msg, std::forward<Args>( args )... ) );
}
template<typename ...Args>
inline void add_msg_if_player_sees( const tripoint &target, const translation &msg,
                                    Args &&... args )
{
    return add_msg_if_player_sees( target, string_format( msg, std::forward<Args>( args )... ) );
}
template<typename ...Args>
inline void add_msg_if_player_sees( const Creature &target, const translation &msg,
                                    Args &&... args )
{
    return add_msg_if_player_sees( target, string_format( msg, std::forward<Args>( args )... ) );
}

void add_msg_if_player_sees( const tripoint &target, const game_message_params &params,
                             std::string msg );
void add_msg_if_player_sees( const Creature &target, const game_message_params &params,
                             std::string msg );
template<typename ...Args>
inline void add_msg_if_player_sees( const tripoint &target, const game_message_params &params,
                                    const std::string &msg, Args &&... args )
{
    if( params.type == m_debug && !debug_mode ) {
        return;
    }
    return add_msg_if_player_sees( target, params, string_format( msg,
                                   std::forward<Args>( args )... ) );
}
template<typename ...Args>
inline void add_msg_if_player_sees( const Creature &target, const game_message_params &params,
                                    const std::string &msg, Args &&... args )
{
    if( params.type == m_debug && !debug_mode ) {
        return;
    }
    return add_msg_if_player_sees( target, params, string_format( msg,
                                   std::forward<Args>( args )... ) );
}
template<typename ...Args>
inline void add_msg_if_player_sees( const tripoint &target, const game_message_params &params,
                                    const char *const msg, Args &&... args )
{
    if( params.type == m_debug && !debug_mode ) {
        return;
    }
    return add_msg_if_player_sees( target, params, string_format( msg,
                                   std::forward<Args>( args )... ) );
}
template<typename ...Args>
inline void add_msg_if_player_sees( const Creature &target, const game_message_params &params,
                                    const char *const msg, Args &&... args )
{
    if( params.type == m_debug && !debug_mode ) {
        return;
    }
    return add_msg_if_player_sees( target, params, string_format( msg,
                                   std::forward<Args>( args )... ) );
}

void add_msg_debug_if_player_sees( const tripoint &target, debugmode::debug_filter type,
                                   std::string msg );
void add_msg_debug_if_player_sees( const Creature &target, debugmode::debug_filter type,
                                   std::string msg );
template<typename ...Args>
inline void add_msg_debug_if_player_sees( const tripoint &target, debugmode::debug_filter type,
        const std::string &msg, Args &&... args )
{
    // expanding for string formatting can be expensive
    if( debug_mode ) {
        return add_msg_debug_if_player_sees( target, type, string_format( msg,
                                             std::forward<Args>( args )... ) );
    }
}
template<typename ...Args>
inline void add_msg_debug_if_player_sees( const Creature &target, debugmode::debug_filter type,
        const std::string &msg, Args &&... args )
{
    if( debug_mode ) {
        return add_msg_debug_if_player_sees( target, type, string_format( msg,
                                             std::forward<Args>( args )... ) );
    }
}
template<typename ...Args>
inline void add_msg_debug_if_player_sees( const tripoint &target, debugmode::debug_filter type,
        const char *const msg, Args &&... args )
{
    if( debug_mode ) {
        return add_msg_debug_if_player_sees( target, type, string_format( msg,
                                             std::forward<Args>( args )... ) );
    }
}
template<typename ...Args>
inline void add_msg_debug_if_player_sees( const Creature &target, debugmode::debug_filter type,
        const char *const msg, Args &&... args )
{
    if( debug_mode ) {
        return add_msg_debug_if_player_sees( target, type, string_format( msg,
                                             std::forward<Args>( args )... ) );
    }
}

#endif // CATA_SRC_MESSAGES_H
