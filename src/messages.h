#pragma once
#ifndef MESSAGES_H
#define MESSAGES_H

#include "cursesdef.h" // WINDOW
#include "printf_check.h"

#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <cstdarg>

class JsonOut;
class JsonObject;

enum game_message_type : int;

class Messages
{
    public:
        Messages();
        ~Messages();

        static std::vector<std::pair<std::string, std::string>> recent_messages( size_t count );
        static void vadd_msg( const char *msg, va_list ap );
        static void vadd_msg( game_message_type type, const char *msg, va_list ap );
        static void clear_messages();
        static size_t size();
        static bool has_undisplayed_messages();
        static void display_messages();
        static void display_messages( WINDOW *ipk_target, int left, int top, int right, int bottom );
        static void serialize( JsonOut &jsout );
        static void deserialize( JsonObject &json );
    private:
        class impl_t;
        std::unique_ptr<impl_t> impl_;
};

void add_msg( const char *msg, ... ) PRINTF_LIKE( 1, 2 );
void add_msg( game_message_type type, const char *msg, ... ) PRINTF_LIKE( 2, 3 );

#endif
