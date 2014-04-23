#include "messages.h"
#include <sstream>

// Messages object.
Messages    Messages::player_messages;

std::vector<Messages::game_message> Messages::recent_messages(const int count)
{
    int tmp_count = count;
    if (tmp_count > messages.size()) {
        tmp_count = messages.size();
    }
    return std::vector<game_message>(messages.end() - tmp_count, messages.end());
}; // AO:only used once, should be generalized and game_message become private

void Messages::add_msg_string(const std::string &s)
{
    if (s.length() == 0) {
        return;
    }
    if (!messages.empty() && int(messages.back().turn) + 3 >= int(calendar::turn) &&
        s == messages.back().message) {
        messages.back().count++;
        messages.back().turn = calendar::turn;
        return;
    }

    while (messages.size() >= 256) {
        messages.erase(messages.begin());
    }

    messages.push_back(game_message(calendar::turn, s));
}

void Messages::vadd_msg(const char *msg, va_list ap)
{
    add_msg_string(vstring_format(msg, ap));
}

void Messages::add_msg(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    vadd_msg(msg, ap);
    va_end(ap);
}

void Messages::display_messages(WINDOW *ipk_target, int left, int top, int right, int bottom,
                                int offset, bool display_turns)
{

    //from  game
    if (!messages.size()) {
        return;
    }
    int line = bottom;
    int maxlength = right - left;
    if (offset < 0) {
        offset = 0;
    }
    if (offset >= messages.size()) {
        offset = messages.size();
    }

    int lasttime = -1;
    for (int i = messages.size() - offset - 1; i >= 0 && line >= top; i--) {
        game_message &m = messages[i];
        std::string mstr = m.message;
        if (m.count > 1) {
            std::stringstream mesSS;
            //~ Message %s on the message log was repeated %d times, eg. “You hear a whack! x 12”
            mesSS << string_format(_("%s x %d"), mstr.c_str(), m.count);
            mstr = mesSS.str();
        }
        // Split the message into many if we must!
        nc_color col = c_dkgray;
        if (int(m.turn) >= curmes) {
            col = c_ltred;
        } else if (int(m.turn) + 5 >= curmes) {
            col = c_ltgray;
        }
        std::vector<std::string> folded = foldstring(mstr, maxlength);
        for (int j = folded.size() - 1; j >= 0 && line >= top; j--, line--) {
            mvwprintz(ipk_target, line, left, col, "%s", folded[j].c_str());
        }
        if (display_turns) {
            calendar timepassed = calendar::turn - m.turn;

            if (int(timepassed) > lasttime) {
                mvwprintz(ipk_target, line, 3, c_ltblue, _("%s ago:"),
                          timepassed.textify_period().c_str());
                line--;
                lasttime = int(timepassed);
            }
        }
    }
    curmes = int(calendar::turn);
};
