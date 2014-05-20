#ifndef _MESSAGES_H_
#define _MESSAGES_H_

#include <string>
#include <vector>
#include "calendar.h"
#include "output.h"
#include "json.h"

enum game_message_type {
    m_good,    /* something good happend to the player character, eg. damage., decreasing in skill */
    m_bad,      /* something bad happened to the player character, eg. health boost, increasing in skill */
    m_mixed,   /* something happened to the player character which is mixed (has good and bad parts),
                  eg. gaining a mutation with mixed effect*/
    m_warning, /* warns the player about a danger. eg. enemy appeared, an alarm sounds, noise heard. */
    m_info,    /* informs the player about something, eg. on examination, seeing an item,
                  about how to use a certain function, etc. */
    m_neutral  /* neutral or indifferent events which aren’t informational or nothing really happened eg.
                  a miss, a non-critical failure. May also effect for good or bad effects which are
                  just very slight to be notable. This is the default message type. */
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
    static void display_messages();
    static void display_messages(WINDOW *ipk_target, int left, int top, int right, int bottom);
    static void serialize(JsonOut &jsout);
    static void deserialize(JsonObject &json);

private:
    struct game_message : public JsonDeserializer, public JsonSerializer {
        calendar turn;
        int count;
        std::string message;
        game_message_type type;
        game_message() {
            turn = 0;
            count = 1;
            message = "";
            type = m_neutral;
        };
        game_message(calendar T, std::string M) : turn(T), message(M) {
            count = 1;
            type = m_neutral;
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
        std::string get_with_count() const;
        nc_color get_color() const;
        virtual void deserialize(JsonIn &jsin);
        virtual void serialize(JsonOut &jsout) const;
    };
    std::vector <struct game_message> messages;   // Messages to be printed
    void add_msg_string(const std::string &s);
    void add_msg_string(const std::string &s, game_message_type type);
    int curmes;   // The last-seen message.
};

void add_msg(const char *msg, ...);
void add_msg(game_message_type type, const char *msg, ...);

#endif //_MESSAGES_H_
