#include "messages.h"
#include "input.h"
#include "game.h"
#include "player.h" // Only u.is_dead
#include "debug.h"
#include "compatibility.h" //to_string
#include "json.h"
#include "output.h"
#include "calendar.h"
#include "translations.h"

#include <deque>
#include <iterator>
#include <algorithm>

// sidebar messages flow direction
extern bool log_from_top;
extern int message_ttl;

namespace {

// Messages object.
Messages player_messages;

struct game_message : public JsonDeserializer, public JsonSerializer {
    std::string       message;
    calendar          timestamp_in_turns  = 0;
    int               timestamp_in_user_actions = 0;
    int               count = 1;
    game_message_type type  = m_neutral;

    game_message() = default;
    game_message(std::string &&msg, game_message_type const t) :
        message (std::move(msg)),
        timestamp_in_turns (calendar::turn),
        timestamp_in_user_actions(g->get_user_action_counter()),
        type (t)
    {
    }

    int turn() const {
        return timestamp_in_turns.get_turn();
    }

    std::string get_with_count() const {
        if (count <= 1) {
            return message;
        }
        //~ Message %s on the message log was repeated %d times, eg. "You hear a whack! x 12"
        return string_format(_("%s x %d"), message.c_str(), count);
    }

    bool is_new(int const current) const {
        return turn() >= current;
    }

    bool is_recent(int const current) const {
        return turn() + 5 >= current;
    }

    nc_color get_color(int const current) const {
        if (is_new(current)) {
            // color for new messages
            return msgtype_to_color(type, false);

        } else if (is_recent(current)) {
            // color for slightly old messages
            return msgtype_to_color(type, true);
        }

        // color for old messages
        return c_dkgray;
    }

    void deserialize(JsonIn &jsin) override {
        JsonObject obj = jsin.get_object();
        timestamp_in_turns = obj.get_int( "turn" );
        message = obj.get_string( "message" );
        count = obj.get_int( "count" );
        type = static_cast<game_message_type>( obj.get_int( "type" ) );
    }

    void serialize(JsonOut &jsout) const override {
        jsout.start_object();
        jsout.member( "turn", static_cast<int>( timestamp_in_turns ) );
        jsout.member( "message", message );
        jsout.member( "count", count );
        jsout.member( "type", static_cast<int>( type ) );
        jsout.end_object();
    }
};

bool message_exceeds_ttl(const game_message &message) {
    return message_ttl > 0 && message.timestamp_in_user_actions + message_ttl <= g->get_user_action_counter();
}

} //namespace

class Messages::impl_t {
public:
    std::deque<game_message> messages;   // Messages to be printed
    int                      curmes = 0; // The last-seen message.

    bool has_undisplayed_messages() const {
        return !messages.empty() && messages.back().turn() > curmes;
    }

    game_message const& history(int const i) const {
        return messages[messages.size() - i - 1];
    }

    // coalesce recent like messages
    bool coalesce_messages(std::string const &msg, game_message_type const type) {
        if (messages.empty()) {
            return false;
        }

        auto &last_msg = messages.back();
        if (last_msg.turn() + 3 < calendar::turn.get_turn()) {
            return false;
        }

        if (type != last_msg.type || msg != last_msg.message) {
            return false;
        }

        last_msg.count++;
        last_msg.timestamp_in_turns = calendar::turn;
        last_msg.timestamp_in_user_actions = g->get_user_action_counter();
        last_msg.type = type;

        return true;
    }

    void add_msg_string(std::string &&msg) {
        add_msg_string(std::move(msg), m_neutral);
    }

    void add_msg_string(std::string &&msg, game_message_type const type) {
        if (msg.length() == 0) {
            return;
        }
        // hide messages if dead
        if (g->u.is_dead_state()) {
            return;
        }

        if (type == m_debug && !debug_mode) {
            return;
        }

        if (coalesce_messages(msg, type)) {
            return;
        }

        while (messages.size() > 255) {
            messages.pop_front();
        }

        messages.emplace_back(std::move(msg), type);
    }

    std::vector<std::pair<std::string, std::string>> recent_messages(size_t count) const {
        count = std::min(count, messages.size());

        std::vector<std::pair<std::string, std::string>> result;
        result.reserve(count);

        const int offset = static_cast<std::ptrdiff_t>( messages.size() - count );

        std::transform(begin(messages) + offset, end(messages), back_inserter(result),
            [](game_message const& msg) {
                return std::make_pair(msg.timestamp_in_turns.print_time(),
                    msg.count ? msg.message + to_string(msg.count) : msg.message);
            }
        );

        return result;
    }
};

Messages::Messages() : impl_ {new impl_t()}
{
}

Messages::~Messages() = default;

std::vector<std::pair<std::string, std::string>> Messages::recent_messages(const size_t count)
{
    return player_messages.impl_->recent_messages(count);
}

void Messages::serialize( JsonOut &json )
{
    json.member( "player_messages" );
    json.start_object();
    json.member( "messages", player_messages.impl_->messages );
    json.member( "curmes", player_messages.impl_->curmes );
    json.end_object();
}

void Messages::deserialize( JsonObject &json )
{
    if (!json.has_member("player_messages")) {
        return;
    }

    JsonObject obj = json.get_object( "player_messages" );
    obj.read( "messages", player_messages.impl_->messages );
    obj.read( "curmes", player_messages.impl_->curmes );
}

void Messages::vadd_msg(const char *msg, va_list ap)
{
    player_messages.impl_->add_msg_string(vstring_format(msg, ap));
}

void Messages::vadd_msg(game_message_type type, const char *msg, va_list ap)
{
    player_messages.impl_->add_msg_string(vstring_format(msg, ap), type);
}

void Messages::clear_messages()
{
    player_messages.impl_->messages.clear();
}

size_t Messages::size()
{
    return player_messages.impl_->messages.size();
}

bool Messages::has_undisplayed_messages()
{
    return player_messages.impl_->has_undisplayed_messages();
}

void Messages::display_messages()
{
    WINDOW_PTR w_ptr {newwin(
        FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
        (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0,
        (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0)};

    WINDOW *const w = w_ptr.get();

    input_context ctxt("MESSAGE_LOG");
    ctxt.register_action("UP", _("Scroll up"));
    ctxt.register_action("DOWN", _("Scroll down"));
    ctxt.register_action("QUIT");
    ctxt.register_action("HELP_KEYBINDINGS");

    int offset = 0;
    const int maxlength = FULL_SCREEN_WIDTH - 2 - 1;
    const int bottom = FULL_SCREEN_HEIGHT - 2;
    const int msg_count = size();

    for (;;) {
        werase(w);
        draw_border(w);
        mvwprintz(w, bottom + 1, 32, c_red, _("Press %s to return"), ctxt.get_desc("QUIT").c_str());
        draw_scrollbar(w, offset, bottom, msg_count, 1);

        int line = 1;
        int lasttime = -1;
        for( int i = offset; i < msg_count; ++i ) {
            if (line > bottom) {
                break;
            }

            const game_message &m     = player_messages.impl_->history(i);
            const nc_color col        = msgtype_to_color( m.type, false );
            const calendar timepassed = calendar::turn - m.timestamp_in_turns;

            if (timepassed.get_turn() > lasttime) {
                mvwprintz(w, line++, 3, c_ltblue, _("%s ago:"),
                    timepassed.textify_period().c_str());
                lasttime = timepassed.get_turn();
            }

            nc_color col_out = col;
            for( const std::string &folded : foldstring(m.get_with_count(), maxlength) ) {
                if (line > bottom) {
                    break;
                }
                print_colored_text( w, line++, 1, col_out, col, folded );
            }
        }

        if (offset + 1 < msg_count) {
            mvwprintz(w, bottom + 1, 5, c_magenta, "vvv");
        }
        if (offset > 0) {
            mvwprintz(w, bottom + 1, maxlength - 3, c_magenta, "^^^");
        }
        wrefresh(w);

        const std::string &action = ctxt.handle_input();
        if (action == "DOWN" && offset + 1 < msg_count) {
            offset++;
        } else if (action == "UP" && offset > 0) {
            offset--;
        } else if (action == "QUIT") {
            break;
        }
    }

    player_messages.impl_->curmes = calendar::turn.get_turn();
}

void Messages::display_messages(WINDOW *const ipk_target, int const left, int const top,
                                int const right, int const bottom)
{
    if (!size()) {
        return;
    }

    int const maxlength = right - left;
    int line = log_from_top ? top : bottom;

    if (log_from_top) {
        for (int i = size() - 1; i >= 0; --i) {
            if (line > bottom) {
                break;
            }

            const game_message &m = player_messages.impl_->messages[i];
            if (message_exceeds_ttl(m)) {
                break;
            }

            const nc_color col = m.get_color(player_messages.impl_->curmes);
            std::string message_text = m.get_with_count();
            if (!m.is_recent(player_messages.impl_->curmes)) {
                message_text = remove_color_tags(message_text);
            }

            for( const std::string &folded : foldstring(message_text, maxlength) ) {
                if (line > bottom) {
                    break;
                }
                // Redrawing line to ensure new messages similar to previous
                // messages will not be missed by screen readers
                wredrawln(ipk_target, line, 1);
                nc_color col_out = col;
                print_colored_text( ipk_target, line++, left, col_out, col, folded );
            }
        }
    } else {
        for (int i = size() - 1; i >= 0; --i) {
            if (line < top) {
                break;
            }

            const game_message &m = player_messages.impl_->messages[i];
            if (message_exceeds_ttl(m)) {
                break;
            }

            const nc_color col = m.get_color(player_messages.impl_->curmes);
            std::string message_text = m.get_with_count();
            if (!m.is_recent(player_messages.impl_->curmes)) {
                message_text = remove_color_tags(message_text);
            }

            const auto folded_strings = foldstring(message_text, maxlength);
            const auto folded_rend = folded_strings.rend();
            for( auto string_iter = folded_strings.rbegin();
                    string_iter != folded_rend && line >= top; ++string_iter, line-- ) {
                // Redrawing line to ensure new messages similar to previous
                // messages will not be missed by screen readers
                wredrawln(ipk_target, line, 1);
                nc_color col_out = col;
                print_colored_text( ipk_target, line, left, col_out, col, *string_iter);
            }
        }
    }

    player_messages.impl_->curmes = calendar::turn.get_turn();
}

void add_msg(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    Messages::vadd_msg(msg, ap);
    va_end(ap);
}

void add_msg(game_message_type const type, const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    Messages::vadd_msg(type, msg, ap);
    va_end(ap);
}
