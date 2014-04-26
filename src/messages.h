#ifndef _MESSAGES_H_
#define _MESSAGES_H_

#include <string>
#include <vector>
#include "calendar.h"
#include "output.h"

class Messages
{
public:

    Messages()
    {
        curmes = 0;
    };

    friend void add_msg(const char *msg, ...);
    static std::vector< std::pair<std::string, std::string> > recent_messages(const size_t count);
    static void vadd_msg(const char *msg, va_list ap);
    static void clear_messages();
    static size_t size();
    static bool has_undisplayed_messages();
    static void display_messages(WINDOW *ipk_target, int left, int top, int right, int bottom,
                                 size_t offset = 0, bool display_turns = false);

private:
    struct game_message {
        calendar turn;
        int count;
        std::string message;
        game_message() {
            turn = 0;
            count = 1;
            message = "";
        };
        game_message(calendar T, std::string M) : turn(T), message(M) {
            count = 1;
        };
        game_message &operator= (game_message const &gm) {
            turn = gm.turn;
            count = gm.count;
            message = gm.message;
            return *this;
        }
    };
    std::vector <struct game_message> messages;   // Messages to be printed
    void add_msg_string(const std::string &s);
    int curmes;   // The last-seen message.
};

void add_msg(const char *msg, ...);


#endif //_MESSAGES_H_
