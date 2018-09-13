#include "messages.h"
#include "input.h"
#include "game.h"
#include "debug.h"
#include "compatibility.h" //to_string
#include "json.h"
#include "output.h"
#include "calendar.h"
#include "translations.h"
#include "string_formatter.h"
#include "string_input_popup.h"

#include <deque>
#include <iterator>
#include <algorithm>

// sidebar messages flow direction
extern bool log_from_top;
extern int message_ttl;

namespace
{

struct game_message : public JsonDeserializer, public JsonSerializer {
    std::string       message;
    time_point timestamp_in_turns  = 0;
    int               timestamp_in_user_actions = 0;
    int               count = 1;
    game_message_type type  = m_neutral;

    game_message() = default;
    game_message( std::string &&msg, game_message_type const t ) :
        message( std::move( msg ) ),
        timestamp_in_turns( calendar::turn ),
        timestamp_in_user_actions( g->get_user_action_counter() ),
        type( t ) {
    }

    const time_point &turn() const {
        return timestamp_in_turns;
    }

    std::string get_with_count() const {
        if( count <= 1 ) {
            return message;
        }
        //~ Message %s on the message log was repeated %d times, e.g. "You hear a whack! x 12"
        return string_format( _( "%s x %d" ), message.c_str(), count );
    }

    bool is_new( const time_point &current ) const {
        return turn() >= current;
    }

    bool is_recent( const time_point &current ) const {
        return turn() + 5_turns >= current;
    }

    nc_color get_color( const time_point &current ) const {
        if( is_new( current ) ) {
            // color for new messages
            return msgtype_to_color( type, false );

        } else if( is_recent( current ) ) {
            // color for slightly old messages
            return msgtype_to_color( type, true );
        }

        // color for old messages
        return c_dark_gray;
    }

    void deserialize( JsonIn &jsin ) override {
        JsonObject obj = jsin.get_object();
        obj.read( "turn", timestamp_in_turns );
        message = obj.get_string( "message" );
        count = obj.get_int( "count" );
        type = static_cast<game_message_type>( obj.get_int( "type" ) );
    }

    void serialize( JsonOut &jsout ) const override {
        jsout.start_object();
        jsout.member( "turn", timestamp_in_turns );
        jsout.member( "message", message );
        jsout.member( "count", count );
        jsout.member( "type", static_cast<int>( type ) );
        jsout.end_object();
    }
};

class messages_impl
{
    public:
        std::deque<game_message> messages;   // Messages to be printed
        time_point curmes = 0; // The last-seen message.
        bool active = true;

        bool has_undisplayed_messages() const {
            return !messages.empty() && messages.back().turn() > curmes;
        }

        game_message const &history( int const i ) const {
            return messages[messages.size() - i - 1];
        }

        // coalesce recent like messages
        bool coalesce_messages( std::string const &msg, game_message_type const type ) {
            if( messages.empty() ) {
                return false;
            }

            auto &last_msg = messages.back();
            if( last_msg.turn() + 3_turns < calendar::turn ) {
                return false;
            }

            if( type != last_msg.type || msg != last_msg.message ) {
                return false;
            }

            last_msg.count++;
            last_msg.timestamp_in_turns = calendar::turn;
            last_msg.timestamp_in_user_actions = g->get_user_action_counter();
            last_msg.type = type;

            return true;
        }

        void add_msg_string( std::string &&msg ) {
            add_msg_string( std::move( msg ), m_neutral );
        }

        void add_msg_string( std::string &&msg, game_message_type const type ) {
            if( msg.length() == 0 || !active ) {
                return;
            }

            if( type == m_debug && !debug_mode ) {
                return;
            }

            if( coalesce_messages( msg, type ) ) {
                return;
            }

            while( messages.size() > 255 ) {
                messages.pop_front();
            }

            messages.emplace_back( std::move( msg ), type );
        }

        std::vector<std::pair<std::string, std::string>> recent_messages( size_t count ) const {
            count = std::min( count, messages.size() );

            std::vector<std::pair<std::string, std::string>> result;
            result.reserve( count );

            const int offset = static_cast<std::ptrdiff_t>( messages.size() - count );

            std::transform( begin( messages ) + offset, end( messages ), back_inserter( result ),
            []( game_message const & msg ) {
                return std::make_pair( to_string_time_of_day( msg.timestamp_in_turns ),
                                       msg.count ? msg.message + to_string( msg.count ) : msg.message );
            }
                          );

            return result;
        }
};

// Messages object.
messages_impl player_messages;

bool message_exceeds_ttl( const game_message &message )
{
    return message_ttl > 0 &&
           message.timestamp_in_user_actions + message_ttl <= g->get_user_action_counter();
}

} //namespace

std::vector<std::pair<std::string, std::string>> Messages::recent_messages( const size_t count )
{
    return player_messages.recent_messages( count );
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
    if( !json.has_member( "player_messages" ) ) {
        return;
    }

    JsonObject obj = json.get_object( "player_messages" );
    obj.read( "messages", player_messages.messages );
    obj.read( "curmes", player_messages.curmes );
}

void Messages::add_msg( std::string msg )
{
    player_messages.add_msg_string( std::move( msg ) );
}

void Messages::add_msg( const game_message_type type, std::string msg )
{
    player_messages.add_msg_string( std::move( msg ), type );
}

void Messages::clear_messages()
{
    player_messages.messages.clear();
    player_messages.active = true;
}

void Messages::deactivate()
{
    player_messages.active = false;
}

size_t Messages::size()
{
    return player_messages.messages.size();
}

bool Messages::has_undisplayed_messages()
{
    return player_messages.has_undisplayed_messages();
}

void Messages::display_messages()
{
    const int w_width = std::min( TERMX, FULL_SCREEN_WIDTH );
    const int w_height = std::min( TERMY, FULL_SCREEN_HEIGHT );
    const int w_x = ( TERMX - w_width ) / 2;
    const int w_y = ( TERMY - w_height ) / 2;

    auto w = catacurses::newwin( w_height, w_width, w_y, w_x );

    input_context ctxt( "MESSAGE_LOG" );
    ctxt.register_action( "UP", _( "Scroll up" ) );
    ctxt.register_action( "DOWN", _( "Scroll down" ) );
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    const nc_color border_color = BORDER_COLOR;
    const nc_color filter_color = c_white;

    // border_width                       border_width
    //      v                                  v
    //
    //      | 12 seconds  Never mind. x 2      |
    //
    //       '-----v-----''---------v---------'
    //        time_width        msg_width
    constexpr int border_width = 1;

    // string_format does not handle padding of utf8 strings correctly, so we have to hack it here
    //~ Change '7' in %7s to the maximum display width of all time units in the target language.
    const auto &unit_fmt = pgettext( "message log", "%7s" );
    //~ Time format in the message log window, %3d is the time number, %s is the (already aligned) time unit.
    const auto &time_fmt = pgettext( "message log", "%3d %s " );

    // Use unit_fmt and time_fmt to determine time string width
    const int unit_width = utf8_width( string_format( unit_fmt, "" ) );
    const int time_width = utf8_width( string_format( time_fmt, 0, string_format( unit_fmt, "" ) ) );

    if( border_width * 2 + time_width >= w_width ||
        border_width * 2 >= w_height ) {

        debugmsg( "No enough space for the message window" );
        return;
    }
    const int msg_width = w_width - border_width * 2 - time_width;
    const size_t max_lines = static_cast<size_t>( w_height - border_width * 2 );
    const size_t msg_count = size();

    // message indices and folded strings
    std::vector<std::pair<size_t, std::string>> folded_all;
    // indices of filtered messages
    std::vector<size_t> folded_filtered;
    for( size_t ind = 0; ind < msg_count; ++ind ) {
        size_t msg_ind = log_from_top ? ind : msg_count - 1 - ind;
        const game_message &msg = player_messages.history( msg_ind );
        const auto &folded = foldstring( msg.get_with_count(), msg_width );
        for( auto it = folded.begin(); it != folded.end(); ++it ) {
            folded_filtered.emplace_back( folded_all.size() );
            folded_all.emplace_back( msg_ind, *it );
        }
    }

    size_t offset;
    if( log_from_top || max_lines > folded_filtered.size() ) {
        offset = 0;
    } else {
        offset = folded_filtered.size() - max_lines;
    }
    string_input_popup filter;
    filter.window( w, border_width + 2, w_height - 1, w_width - border_width - 2 );
    bool filtering = false;
    std::string filter_str;

    // main UI loop
    while( true ) {
        size_t line_from = 0, line_to;
        if( offset < folded_filtered.size() ) {
            line_to = std::min( max_lines, folded_filtered.size() - offset );
        } else {
            line_to = 0;
        }

        werase( w );
        draw_border( w, border_color );
        draw_scrollbar( w, offset, max_lines, folded_filtered.size(), border_width, 0, c_white, true );

        if( !log_from_top ) {
            // Always print from new to old
            std::swap( line_from, line_to );
        }
        std::string prev_time_str;
        bool printing_range = false;
        for( size_t line = line_from; line != line_to; ) {
            // Decrement here if printing from bottom to get the correct line number
            if( !log_from_top ) {
                --line;
            }

            const size_t folded_ind = offset + line;
            const size_t msg_ind = folded_all[folded_filtered[folded_ind]].first;
            const game_message &msg = player_messages.history( msg_ind );

            const nc_color col = msgtype_to_color( msg.type, false );
            nc_color col_out = col;

            print_colored_text( w, border_width + line, border_width + time_width, col_out, col,
                                folded_all[folded_filtered[folded_ind]].second );

            const time_point msg_time = msg.timestamp_in_turns;
            const auto &num_n_unit = to_num_and_unit( calendar::turn - msg_time );
            const auto u8w = utf8_width( num_n_unit.second );
            std::string time_str;

            if( num_n_unit.first.has_value() ) {
                if( u8w < unit_width ) {
                    // string_format does not handle utf8 string width correctly, we have to insert our own padding
                    time_str = string_format( time_fmt, num_n_unit.first.value(),
                                              std::string( unit_width - u8w, ' ' ) + num_n_unit.second );
                } else {
                    time_str = string_format( time_fmt, num_n_unit.first.value(), num_n_unit.second );
                }
            } else {
                // not likely, but just in case
                if( u8w < time_width ) {
                    time_str = std::string( time_width - u8w, ' ' ) + num_n_unit.second;
                } else {
                    time_str = num_n_unit.second;
                }
            }
            if( time_str != prev_time_str ) {
                prev_time_str = time_str;
                right_print( w, border_width + line, border_width + msg_width, c_light_blue, time_str );
                printing_range = false;
            } else {
                if( printing_range ) {
                    const size_t last_line = log_from_top ? line - 1 : line + 1;
                    wattron( w, c_dark_gray );
                    mvwaddch( w, border_width + last_line, border_width + time_width - 2, LINE_XOXO );
                    wattroff( w, c_dark_gray );
                }
                wattron( w, c_dark_gray );
                mvwaddch( w, border_width + line, border_width + time_width - 2,
                          log_from_top ? LINE_XXOO : LINE_OXXO );
                wattroff( w, c_dark_gray );
                printing_range = true;
            }

            // Decrement for !log_from_top is done at the beginning
            if( log_from_top ) {
                ++line;
            }
        }

        if( filtering ) {
            mvwprintz( w, w_height - 1, border_width, border_color, "< " );
            mvwprintz( w, w_height - 1, w_width - border_width - 2, border_color, " >" );
            filter.query( false );
            if( filter.confirmed() || filter.canceled() ) {
                filtering = false;
            }
            const std::string &new_filter_str = filter.text();
            if( new_filter_str != filter_str ) {
                filter_str = new_filter_str;
                folded_filtered.clear();
                for( size_t folded_ind = 0; folded_ind < folded_all.size(); ) {
                    const size_t msg_ind = folded_all[folded_ind].first;
                    const game_message &msg = player_messages.history( msg_ind );
                    bool match = ci_find_substr( msg.get_with_count(), filter_str ) >= 0;
                    for( ; folded_ind < folded_all.size() && folded_all[folded_ind].first == msg_ind; ++folded_ind ) {
                        if( match ) {
                            folded_filtered.emplace_back( folded_ind );
                        }
                    }
                }
                // @todo fix position
                if( max_lines > folded_filtered.size() ) {
                    offset = 0;
                } else if( offset + max_lines > folded_filtered.size() ) {
                    offset = folded_filtered.size() - max_lines;
                }
            }
        } else {
            if( filter_str.empty() ) {
                mvwprintz( w, w_height - 1, border_width, border_color, _( "< Press %s to filter >" ),
                           ctxt.get_desc( "FILTER" ) );
            } else {
                mvwprintz( w, w_height - 1, border_width, border_color, "< %s >", filter_str );
                mvwprintz( w, w_height - 1, border_width + 2, filter_color, "%s", filter_str );
            }
            wrefresh( w );
            const std::string &action = ctxt.handle_input();
            if( action == "DOWN" && offset + max_lines < folded_filtered.size() ) {
                ++offset;
            } else if( action == "UP" && offset > 0 ) {
                --offset;
            } else if( action == "PAGE_DOWN" ) {
                if( offset + max_lines * 2 <= folded_filtered.size() ) {
                    offset += max_lines;
                } else if( max_lines <= folded_filtered.size() ) {
                    offset = folded_filtered.size() - max_lines;
                } else {
                    offset = 0;
                }
            } else if( action == "PAGE_UP" ) {
                if( offset >= max_lines ) {
                    offset -= max_lines;
                } else {
                    offset = 0;
                }
            } else if( action == "FILTER" ) {
                filtering = true;
            } else if( action == "QUIT" ) {
                break;
            }
        }
    }

    player_messages.curmes = calendar::turn;
}

void Messages::display_messages( const catacurses::window &ipk_target, int const left,
                                 int const top, int const right, int const bottom )
{
    if( !size() ) {
        return;
    }

    int const maxlength = right - left;
    int line = log_from_top ? top : bottom;

    if( log_from_top ) {
        for( int i = size() - 1; i >= 0; --i ) {
            if( line > bottom ) {
                break;
            }

            const game_message &m = player_messages.messages[i];
            if( message_exceeds_ttl( m ) ) {
                break;
            }

            const nc_color col = m.get_color( player_messages.curmes );
            std::string message_text = m.get_with_count();
            if( !m.is_recent( player_messages.curmes ) ) {
                message_text = remove_color_tags( message_text );
            }

            for( const std::string &folded : foldstring( message_text, maxlength ) ) {
                if( line > bottom ) {
                    break;
                }
                // Redrawing line to ensure new messages similar to previous
                // messages will not be missed by screen readers
                wredrawln( ipk_target, line, 1 );
                nc_color col_out = col;
                print_colored_text( ipk_target, line++, left, col_out, col, folded );
            }
        }
    } else {
        for( int i = size() - 1; i >= 0; --i ) {
            if( line < top ) {
                break;
            }

            const game_message &m = player_messages.messages[i];
            if( message_exceeds_ttl( m ) ) {
                break;
            }

            const nc_color col = m.get_color( player_messages.curmes );
            std::string message_text = m.get_with_count();
            if( !m.is_recent( player_messages.curmes ) ) {
                message_text = remove_color_tags( message_text );
            }

            const auto folded_strings = foldstring( message_text, maxlength );
            const auto folded_rend = folded_strings.rend();
            for( auto string_iter = folded_strings.rbegin();
                 string_iter != folded_rend && line >= top; ++string_iter, line-- ) {
                // Redrawing line to ensure new messages similar to previous
                // messages will not be missed by screen readers
                wredrawln( ipk_target, line, 1 );
                nc_color col_out = col;
                print_colored_text( ipk_target, line, left, col_out, col, *string_iter );
            }
        }
    }

    player_messages.curmes = calendar::turn;
}

void add_msg( std::string msg )
{
    Messages::add_msg( std::move( msg ) );
}

void add_msg( game_message_type const type, std::string msg )
{
    Messages::add_msg( type, std::move( msg ) );
}
