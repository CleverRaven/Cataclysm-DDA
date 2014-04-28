#ifndef _MESSAGES_H_
#define _MESSAGES_H_

#include <string>
#include <vector>
#include "calendar.h"
#include "output.h"

enum game_message_type {
    good,	 /* something good happend to the player character, eg. damage., decreasing in skill */
    bad,     /* something bad happened to the player character, eg. health boost, increasing in skill */
    mixed,   /* something happened to the player character which is mixed (has good and bad parts),
                eg. gaining a mutation with mixed effect*/
    warning, /* warns the player about a danger. eg. enemy appeared, an alarm sounds, noise heard. */
    info,    /* informs the player about something, eg. on examination, seeing an item,
                about how to use a certain function, etc. */
    neutral	 /* neutral or indifferent events which aren’t informational or nothing really happened eg.
                a miss, a non-critical failure. May also effect for good or bad effects which are
                just very slight to be notable. */
};

class Messages
{
public:

    Messages()
    {
        curmes = 0;
    };
    friend void add_msg(const char *msg, ...);
    friend void add_msg(game_message_type type, const char *msg, ...);
    static std::vector< std::pair<std::string, std::string> > recent_messages(const size_t count);
    static void vadd_msg(const char *msg, va_list ap);
    static void vadd_msg(game_message_type type, const char *msg, va_list ap);
    static void clear_messages();
    static size_t size();
    static bool has_undisplayed_messages();
    static void display_messages(WINDOW *ipk_target, int left, int top, int right, int bottom,
                                 size_t offset = 0, bool display_turns = false);

private:
    struct game_message {
        game_message_type type;
        calendar turn;
        int count;
        std::string message;
        game_message() {
            turn = 0;
            count = 1;
            message = "";
            type = neutral;
        };
        /* legacy */
        game_message(calendar T, std::string M) : turn(T), message(M) {
            count = 1;
        };
        game_message(calendar T, std::string M, game_message_type Y) : turn(T), message(M), type(Y) {
            count = 1;
        };
        game_message &operator= (game_message const &gm) {
            turn = gm.turn;
            count = gm.count;
            message = gm.message;
            type = gm.type;
            return *this;
        }
    };
    std::vector <struct game_message> messages;   // Messages to be printed
    void add_msg_string(const std::string &s);
    void add_msg_string(const std::string &s, game_message_type type);
    int curmes;   // The last-seen message.
    int display_single_message(WINDOW *ipk_target, int left, int top);
};

void add_msg(const char *msg, ...);
void add_msg(game_message_type type, const char *msg, ...);

#endif //_MESSAGES_H_
