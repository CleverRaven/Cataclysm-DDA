#pragma once
#ifndef MESSAGES_H
#define MESSAGES_H

#include "string_formatter.h"
#include "pimpl.h"

#include <string>
#include <vector>
#include <utility>

class JsonOut;
class JsonObject;

enum game_message_type : int;
namespace catacurses
{
class window;
} // namespace catacurses

class Messages
{
    public:
        Messages();
        ~Messages();

        static std::vector<std::pair<std::string, std::string>> recent_messages( size_t count );
        static void add_msg( std::string msg );
        static void add_msg( game_message_type type, std::string msg );
        static void clear_messages();
        static void deactivate();
        static size_t size();
        static bool has_undisplayed_messages();
        static void display_messages();
        static void display_messages( const catacurses::window &ipk_target, int left, int top, int right,
                                      int bottom );
        static void serialize( JsonOut &jsout );
        static void deserialize( JsonObject &json );
    private:
        class impl_t;
        pimpl<impl_t> impl_;
};

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
