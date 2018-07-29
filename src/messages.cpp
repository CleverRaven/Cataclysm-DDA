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

#include <deque>
#include <iterator>
#include <algorithm>

// sidebar messages flow direction
extern bool log_from_top;
extern int message_ttl;

namespace
{

// Messages object.
Messages player_messages;

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

bool message_exceeds_ttl( const game_message &message )
{
    return message_ttl > 0 &&
           message.timestamp_in_user_actions + message_ttl <= g->get_user_action_counter();
}

} //namespace

class Messages::impl_t
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

Messages::Messages() : impl_()
{
}

Messages::~Messages() = default;

std::vector<std::pair<std::string, std::string>> Messages::recent_messages( const size_t count )
{
    return player_messages.impl_->recent_messages( count );
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
    if( !json.has_member( "player_messages" ) ) {
        return;
    }

    JsonObject obj = json.get_object( "player_messages" );
    obj.read( "messages", player_messages.impl_->messages );
    obj.read( "curmes", player_messages.impl_->curmes );
}

void Messages::add_msg( std::string msg )
{
    player_messages.impl_->add_msg_string( std::move( msg ) );
}

void Messages::add_msg( const game_message_type type, std::string msg )
{
    player_messages.impl_->add_msg_string( std::move( msg ), type );
}

void Messages::clear_messages()
{
    player_messages.impl_->messages.clear();
    player_messages.impl_->active = true;
}

void Messages::deactivate()
{
    player_messages.impl_->active = false;
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
    catacurses::window w = catacurses::newwin(
                               FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                               ( TERMY > FULL_SCREEN_HEIGHT ) ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0,
                               ( TERMX > FULL_SCREEN_WIDTH ) ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0 );

    input_context ctxt( "MESSAGE_LOG" );
    ctxt.register_action( "UP", _( "Scroll up" ) );
    ctxt.register_action( "DOWN", _( "Scroll down" ) );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    /* Right-Aligning Time Epochs For Readability
     * ==========================================
     * Given display_messages(); right-aligns epochs, we must declare one quick variable first:
     * max_padlength refers to the length of the LONGEST possible unit of time returned by to_string_clipped() any language has to offer.
     * This variable is, for now, set to '10', which seems most reasonable.
     *
     * The reason right-aligned epochs don't use a "shortened version" (e.g. only showing the first letter) boils down to:
     * 1. The first letter of every time unit being unique is a property that might not carry across to other languages.
     * 2. Languages where displayed characters change depending on the characters preceding/following it will become unclear.
     * 3. Some polymorphic languages might not be able to appropriately convey meaning with one character (FRS is not a linguist- correct her on this if needed.)
     *
     * This right padlength is then incorporated into a so-called 'epoch_format' which, in turn, will be used to format the correct epoch.
     * If an external language introduces time units longer than 10 characters in size, consider altering these variables.
     * The game (likely) shan't segfault, though the text may appear a bit messed up if these variables aren't set properly.
     */
    const int max_padlength = 10;

    /* Dealing With Screen Extremities
     * ===============================
     * 'maxlength' corresponds to the most extreme length a log message may be before foldstring() wraps it around to two or more lines.
     * The numbers subtracted from FULL_SCREEN_WIDTH are - in order:
     * '-2' the characters reserved for the borders of the box, both on the left and right side.
     * '-1' the leftmost guide character that's drawn on screen.
     * '-4' the padded three-digit number each epoch starts with.
     * '-max_padlength' the characters of space that are allocated to time units (e.g. "years", "minutes", etc.)
     *
     * 'bottom' works much like 'maxlength', but instead it refers to the amount of lines that the message box may hold.
     */
    const int maxlength = FULL_SCREEN_WIDTH - 2 - 1 - 4 - max_padlength;
    const int bottom = FULL_SCREEN_HEIGHT - 2;
    const int msg_count = size();

    /* Dealing With Scroll Direction
     * =============================
     * Much like how the sidebar can have variable scroll direction, so will the message box.
     * To properly differentiate the two methods of displaying text, we will label them NEWEST-TOP, and OLDEST-TOP. This labeling should be self explanatory.
     *
     * LIMITATIONS:
     * 1. If there are multi-line messages among the oldest ones, the text cannot be fully scrolled to the top/bottom,
     * since the message box is drawn using message index rather than actual line number.
     * 2. Markers will change when skipping through multi-line messages, since `offset` is only the number of messages
     * instead of the line count of these messages.
     *
     * log_from_top: display the newest message at the top if true, bottom if false.
     * offset: index of the message from which to start displaying, from the newest to the oldest.
     * line: the current line of folded message for display, either from the top or the bottom.
     * scr_line: the current line of folded message for display, from the top.
     * retrieve_history: the current index of message for display, from the newest to the oldest.
     * folded_ind: current line index of the folded message.
     */
    for( int offset = 0;; ) {
        werase( w );
        draw_border( w );
        center_print( w, bottom + 1, c_red,
                      string_format( _( "Press %s to return" ), ctxt.get_desc( "QUIT" ).c_str() ) );
        int scroll = log_from_top ? offset : msg_count - bottom - offset;
        draw_scrollbar( w, scroll, bottom, msg_count, 1, 0, c_white, true );

        time_duration lasttime = -1_turns;
        for( int line = 0, retrieve_history = offset;
             line < bottom && retrieve_history < msg_count; ++retrieve_history ) {
            const game_message &m     = player_messages.impl_->history( retrieve_history );
            const time_duration timepassed = calendar::turn - m.timestamp_in_turns;
            std::string long_ago      = to_string_clipped( timepassed );
            nc_color col              = msgtype_to_color( m.type, false );

            auto &&folded_strings = foldstring( m.get_with_count(), maxlength );
            int folded_size( folded_strings.size() );
            int scr_line = log_from_top ? line : std::max( 0, bottom - line - folded_size );
            // Here we separate the unit and amount from one another so that they can be properly padded when they're drawn on the screen.
            // Note that the very first character of 'unit' is often a space (except for languages where the time unit directly follows the number.)
            const auto amount_len = long_ago.find_first_not_of( "0123456789" );
            std::string amount = long_ago.substr( 0, amount_len );
            std::string unit = long_ago.substr( amount_len );
            if( timepassed != lasttime ) {
                right_print( w, scr_line + 1, 2, c_light_blue, string_format( _( "%-3s%-10s" ), amount.c_str(),
                             unit.c_str() ) );
                lasttime = timepassed;
            }

            nc_color col_out = col;
            for( int folded_ind = log_from_top ? 0 : folded_size - 1;
                 folded_ind >= 0 && folded_ind < folded_size && line < bottom;
                 ( log_from_top ? ++folded_ind : --folded_ind ), ++line ) {
                scr_line = log_from_top ? line : bottom - line - 1;
                print_colored_text( w, scr_line + 1, 2, col_out, col, folded_strings[folded_ind] );

                // So-called special "markers"- alternating '=' and '-'s at the edges of te message window so players can properly make sense of which message belongs to which time interval.
                // The '+offset%4' in the calculation makes it so that the markings scroll along with the messages.
                // On lines divisible by 4, draw a dark gray '-' at both horizontal extremes of the window.
                if( ( line + offset % 4 ) % 4 == 0 ) {
                    mvwprintz( w, scr_line + 1, 1, c_dark_gray, "-" );
                    mvwprintz( w, scr_line + 1, FULL_SCREEN_WIDTH - 2, c_dark_gray, "-" );
                    // On lines divisible by 2 (but not 4), draw a light gray '=' at the horizontal extremes of the window.
                } else if( ( line + offset % 4 ) % 2 == 0 ) {
                    mvwprintz( w, scr_line + 1, 1, c_light_gray, "=" );
                    mvwprintz( w, scr_line + 1, FULL_SCREEN_WIDTH - 2, c_light_gray, "=" );
                }
            }
        }
        wrefresh( w );

        const std::string &action = ctxt.handle_input();
        if( action == "DOWN" || action == "UP" ) {
            if( ( action == "DOWN" ) ^ ( log_from_top ) ) {
                if( offset > 0 ) {
                    --offset;
                }
            } else if( offset < msg_count - bottom ) {
                ++offset;
            }
        } else if( action == "QUIT" ) {
            break;
        }
    }

    player_messages.impl_->curmes = calendar::turn;
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

            const game_message &m = player_messages.impl_->messages[i];
            if( message_exceeds_ttl( m ) ) {
                break;
            }

            const nc_color col = m.get_color( player_messages.impl_->curmes );
            std::string message_text = m.get_with_count();
            if( !m.is_recent( player_messages.impl_->curmes ) ) {
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

            const game_message &m = player_messages.impl_->messages[i];
            if( message_exceeds_ttl( m ) ) {
                break;
            }

            const nc_color col = m.get_color( player_messages.impl_->curmes );
            std::string message_text = m.get_with_count();
            if( !m.is_recent( player_messages.impl_->curmes ) ) {
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

    player_messages.impl_->curmes = calendar::turn;
}

void add_msg( std::string msg )
{
    Messages::add_msg( std::move( msg ) );
}

void add_msg( game_message_type const type, std::string msg )
{
    Messages::add_msg( type, std::move( msg ) );
}
