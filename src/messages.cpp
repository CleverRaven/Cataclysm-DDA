#include "messages.h"
#include "input.h"
#include "game.h"
#include "debug.h"
#include <sstream>

// Messages object.
static Messages player_messages;

std::vector< std::pair<std::string, std::string> > Messages::recent_messages(const size_t count)
{
    const int tmp_count = std::min( size(), count );

    std::vector< std::pair<std::string, std::string> > recent_messages;
    for( size_t i = size() - tmp_count; i < size(); ++i ) {
        std::stringstream message_with_count;
        message_with_count << player_messages.messages[i].message;
        if( player_messages.messages[i].count > 1 ) {
            message_with_count << " x" << player_messages.messages[i].count;
        }

        recent_messages.push_back( std::make_pair( player_messages.messages[i].turn.print_time(),
                                   message_with_count.str() ) );
    }
    return recent_messages;
}

void Messages::serialize( JsonOut &json )
{
    json.member( "player_messages" );
    json.start_object();
    json.member( "messages", player_messages.messages );
    json.member( "curmes", player_messages.curmes );
    json.end_object();
}

void Messages::deserialize( JsonObject &json )
{
    if( json.has_member( "player_messages" ) ) {
        JsonObject obj = json.get_object( "player_messages" );
        obj.read( "messages", player_messages.messages );
        obj.read( "curmes", player_messages.curmes );
    }
}

void Messages::game_message::deserialize( JsonIn &jsin )
{
    JsonObject obj = jsin.get_object();
    turn = obj.get_int( "turn" );
    message = obj.get_string( "message" );
    count = obj.get_int( "count" );
    type = static_cast<game_message_type>( obj.get_int( "type" ) );
}

void Messages::game_message::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "turn", static_cast<int>( turn ) );
    jsout.member( "message", message );
    jsout.member( "count", count );
    jsout.member( "type", static_cast<int>( type ) );
    jsout.end_object();
}

void Messages::add_msg_string(const std::string &s)
{
    add_msg_string(s, m_neutral);
}

void Messages::add_msg_string(const std::string &s, game_message_type type)
{
    if (s.length() == 0) {
        return;
    }
    // hide messages if dead
    if(g->u.is_dead_state()) {
        return;
    }
    if( type == m_debug && !debug_mode ) {
        return;
    }
    if (!player_messages.messages.empty() &&
        int(player_messages.messages.back().turn) + 3 >= int(calendar::turn) &&
        s == player_messages.messages.back().message) {
        player_messages.messages.back().count++;
        player_messages.messages.back().turn = calendar::turn;
        player_messages.messages.back().type = type;
        return;
    }

    while( size() >= 256 ) {
        player_messages.messages.erase(player_messages.messages.begin());
    }

    player_messages.messages.push_back(game_message(calendar::turn, s, type));
}

void Messages::vadd_msg(const char *msg, va_list ap)
{
    player_messages.add_msg_string(vstring_format(msg, ap));
}

void Messages::vadd_msg(game_message_type type, const char *msg, va_list ap)
{
    player_messages.add_msg_string(vstring_format(msg, ap), type);
}

void add_msg(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    Messages::vadd_msg(msg, ap);
    va_end(ap);
}

void add_msg(game_message_type type, const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    Messages::vadd_msg(type, msg, ap);
    va_end(ap);
}

void Messages::clear_messages()
{
    player_messages.messages.clear();
}

size_t Messages::size()
{
    return player_messages.messages.size();
}

bool Messages::has_undisplayed_messages()
{
    return size() && (player_messages.messages.back().turn > player_messages.curmes);
}

std::string Messages::game_message::get_with_count() const
{
    if (count <= 1) {
        return message;
    }
    //~ Message %s on the message log was repeated %d times, eg. "You hear a whack! x 12"
    return string_format(_("%s x %d"), message.c_str(), count);
}

nc_color Messages::game_message::get_color() const
{
    if (int(turn) >= player_messages.curmes) {
        // color for new messages
        return msgtype_to_color(type, false);

    } else if (int(turn) + 5 >= player_messages.curmes) {
        // color for slightly old messages
        return msgtype_to_color(type, true);
    }

    // color for old messages
    return c_dkgray;
}

void Messages::display_messages()
{
    WINDOW *w = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                       (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0,
                       (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0);

    size_t offset = 0;
    const int maxlength = FULL_SCREEN_WIDTH - 2 - 1;
    const int bottom = FULL_SCREEN_HEIGHT - 2;
    input_context ctxt("MESSAGE_LOG");
    ctxt.register_action("UP", _("Scroll up"));
    ctxt.register_action("DOWN", _("Scroll down"));
    ctxt.register_action("QUIT");
    ctxt.register_action("HELP_KEYBINDINGS");
    while(true) {
        werase(w);
        draw_border(w);
        mvwprintz(w, FULL_SCREEN_HEIGHT - 1, 32, c_red, _("Press %s to return"),
                  ctxt.get_desc("QUIT").c_str());

        draw_scrollbar(w, offset, FULL_SCREEN_HEIGHT - 2, size(), 1);

        int line = 1;
        int lasttime = -1;
        for (size_t i = offset; i < size() && line <= bottom; i++) {
            const game_message &m = player_messages.messages[size() - i - 1];
            const std::string mstr = remove_color_tags( m.get_with_count() );
            const nc_color col = m.get_color();
            calendar timepassed = calendar::turn - m.turn;
            if (int(timepassed) > lasttime) {
                mvwprintz(w, line, 3, c_ltblue, _("%s ago:"),
                          timepassed.textify_period().c_str());
                line++;
                lasttime = int(timepassed);
            }
            std::vector<std::string> folded = foldstring(mstr, maxlength);
            for (size_t j = 0; j < folded.size() && line <= bottom; j++, line++) {
                mvwprintz(w, line, 1, col, "%s", folded[j].c_str());
            }
        }

        if (offset + 1 < size()) {
            mvwprintz(w, FULL_SCREEN_HEIGHT - 1, 5, c_magenta, "vvv");
        }
        if (offset > 0) {
            mvwprintz(w, FULL_SCREEN_HEIGHT - 1, FULL_SCREEN_WIDTH - 6, c_magenta, "^^^");
        }
        wrefresh(w);

        const std::string action = ctxt.handle_input();
        if (action == "DOWN" && offset + 1 < size()) {
            offset++;
        } else if (action == "UP" && offset > 0) {
            offset--;
        } else if (action == "QUIT") {
            break;
        }
    }
    player_messages.curmes = int(calendar::turn);
    werase(w);
    delwin(w);
}

void Messages::display_messages(WINDOW *ipk_target, int left, int top, int right, int bottom)
{
    if (!size()) {
        return;
    }
    const int maxlength = right - left;
    int line = bottom;
    for (int i = size() - 1; i >= 0 && line >= top; i--) {
        const game_message &m = player_messages.messages[i];
        const std::string mstr = remove_color_tags( m.get_with_count() );
        const nc_color col = m.get_color();
        std::vector<std::string> folded = foldstring(mstr, maxlength);
        for (int j = folded.size() - 1; j >= 0 && line >= top; j--, line--) {
            mvwprintz(ipk_target, line, left, col, "%s", folded[j].c_str());
        }
    }
    player_messages.curmes = int(calendar::turn);
}
