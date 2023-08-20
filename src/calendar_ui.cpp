#include "calendar_ui.h"

#include "string_formatter.h"
#include "string_input_popup.h"
#include "ui.h"
#include "ui_manager.h"

time_point calendar_ui::select_time_point( time_point initial_value, std::string_view title )
{
    time_point return_value = initial_value;
    auto set_turn = [&]( const int initial, const time_duration & factor, const char *const msg ) {
        string_input_popup pop;
        const int new_value = pop
                              .title( msg )
                              .width( 20 )
                              .text( std::to_string( initial ) )
                              .only_digits( true )
                              .query_int();
        if( pop.canceled() ) {
            return;
        }
        const time_duration offset = ( new_value - initial ) * factor;
        // Arbitrary maximal value.
        const time_point max = calendar::turn_zero + time_duration::from_turns(
                                   std::numeric_limits<int>::max() / 2 );
        return_value = std::max( std::min( max, return_value + offset ), calendar::turn_zero );
    };

    uilist smenu;
    static const auto years = []( const time_point & p ) {
        return static_cast<int>( ( p - calendar::turn_zero ) / calendar::year_length() );
    };
    do {
        const int iSel = smenu.ret;
        smenu.reset();
        smenu.title = title;
        smenu.text += string_format( "Old date: %1$s\nNew date: %2$s",
                                     colorize( to_string( initial_value ), c_light_red ),
                                     colorize( to_string( return_value ), c_light_cyan ) );
        smenu.desc_enabled = true;
        smenu.footer_text = string_format( _( "Press <color_light_green>%s</color> when done" ),
                                           input_context( smenu.input_category ).get_desc( "UILIST.QUIT" ) );
        smenu.addentry( 0, true, 'y', "%s: %d", _( "year" ), years( return_value ) + 1 );
        smenu.addentry( 1, !calendar::eternal_season(), 's', "%s: %d",
                        _( "season" ), static_cast<int>( season_of_year( return_value ) ) );
        smenu.addentry( 2, true, 'd', "%s: %d", _( "day" ), day_of_season<int>( return_value ) + 1 );
        smenu.addentry( 3, true, 'h', "%s: %d", _( "hour" ), hour_of_day<int>( return_value ) );
        smenu.addentry( 4, true, 'm', "%s: %d", _( "minute" ), minute_of_hour<int>( return_value ) );
        smenu.addentry( 5, true, 't', "%s: %d", _( "turn" ),
                        to_turns<int>( return_value - calendar::turn_zero ) );
        smenu.selected = iSel;
        smenu.query();

        switch( smenu.ret ) {
            case 0:
                set_turn( years( return_value ) + 1, calendar::year_length(), _( "Set year to?" ) );
                break;
            case 1:
                set_turn( static_cast<int>( season_of_year( return_value ) ), calendar::season_length(),
                          _( "Set season to?  (0 = spring)" ) );
                break;
            case 2:
                set_turn( day_of_season<int>( return_value ) + 1, 1_days, _( "Set days to?" ) );
                break;
            case 3:
                set_turn( hour_of_day<int>( return_value ), 1_hours, _( "Set hour to?" ) );
                break;
            case 4:
                set_turn( minute_of_hour<int>( return_value ), 1_minutes, _( "Set minute to?" ) );
                break;
            case 5:
                set_turn( to_turns<int>( return_value - calendar::turn_zero ), 1_turns,
                          string_format( _( "Set turn to?  (One day is %i turns)" ), to_turns<int>( 1_days ) ).c_str() );
                break;
            default:
                break;
        }
    } while( smenu.ret != UILIST_CANCEL );
    return return_value;
}
