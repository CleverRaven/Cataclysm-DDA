#ifndef _MESSAGES_H_
#define _MESSAGES_H_

#include <string>
#include <vector>
#include "calendar.h"
#include "output.h"

class Messages
{
    public:
        struct game_message {
            calendar turn;
            int count;
            std::string message;
            game_message()
            {
                turn = 0;
                count = 1;
                message = "";
            };
            game_message(calendar T, std::string M) : turn(T), message(M)
            {
                count = 1;
            };
            game_message &operator= (game_message const &gm)
            {
                turn = gm.turn;
                count = gm.count;
                message = gm.message;
                return *this;
            }
        };

        Messages()
        {
            curmes = 0;
        };

        // AO:only used once, should be generalized and game_message become private
        static std::vector<game_message> recent_messages(const int count);
        static void add_msg(const char *msg, ...);
        static void vadd_msg(const char *msg, va_list ap);
        static void clear_messages();
        static size_t size();
        static bool has_undisplayed_messages();
        static void display_messages(WINDOW *ipk_target, int left, int top, int right, int bottom,
                                     int offset = 0, bool display_turns = false);

    private:
        std::vector <game_message> messages;   // Messages to be printed
        void add_msg_string(const std::string &s);
        int curmes;   // The last-seen message.
        int display_single_message(WINDOW *ipk_target, int left, int top);
};


#endif //_MESSAGES_H_
