#include "calendar_ui.h"

#include "string_formatter.h"
#include "string_input_popup.h"
#include "ui.h"
#include "ui_manager.h"

time_point calendar_ui::select_time_point( time_point initial_value, std::string_view title,
        calendar_ui::granularity granularity_level )
{
    time_point return_value = initial_value;
    auto set_turn = [&]( const int initial, const time_duration & factor, const char *const msg,
    int auto_value = 0 ) {
        int new_value = initial + auto_value;
        if( new_value == initial ) {
            string_input_popup pop;
            new_value = pop
                        .title( msg )
                        .width( 20 )
                        .text( std::to_string( initial ) )
                        .only_digits( true )
                        .query_int();
            if( pop.canceled() ) {
                return;
            }
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
    int iSel = smenu.ret;
    do {
        smenu.reset();
        smenu.title = title;
        smenu.text += string_format( "Old date: %1$s\nNew date: %2$s",
                                     colorize( to_string( initial_value ), c_light_red ),
                                     colorize( to_string( return_value ), c_light_cyan ) );
        smenu.desc_enabled = true;
        smenu.allow_additional = true;
        smenu.input_category = "CALENDAR_UI";
        smenu.additional_actions = {
            { "LEFT", to_translation( "Decrease value" ) },
            { "RIGHT", to_translation( "Increase value" ) },
            { "RESET", to_translation( "Reset value" ) }
        };
        smenu.footer_text += string_format( _( "Press <color_light_green>%s</color> to decrease value.\n" ),
                                            input_context( smenu.input_category ).get_desc( "LEFT" ) );
        smenu.footer_text += string_format( _( "Press <color_light_green>%s</color> to increase value.\n" ),
                                            input_context( smenu.input_category ).get_desc( "RIGHT" ) );
        smenu.footer_text += string_format( _( "Press <color_light_green>%s</color> to reset value.\n" ),
                                            input_context( smenu.input_category ).get_desc( "RESET" ) );
        smenu.footer_text += string_format(
                                 _( "Press <color_light_green>%s</color> when done selecting time point." ),
                                 input_context( smenu.input_category ).get_desc( "UILIST.QUIT" ) );

        smenu.addentry( static_cast<int>( calendar_ui::granularity::year ), true,
                        'y', "%s: %d", _( "year" ), years( return_value ) + 1 );
        smenu.addentry( static_cast<int>( calendar_ui::granularity::season ), true,
                        's', "%s: %s", _( "season" ), calendar::name_season( season_of_year( return_value ) ) );
        smenu.addentry( static_cast<int>( calendar_ui::granularity::day ), true,
                        'd', "%s: %d", _( "day" ), day_of_season<int>( return_value ) + 1 );
        smenu.addentry( static_cast<int>( calendar_ui::granularity::hour ), true,
                        'h', "%s: %d", _( "hour" ), hour_of_day<int>( return_value ) );
        smenu.addentry( static_cast<int>( calendar_ui::granularity::minute ), true,
                        'm', "%s: %d", _( "minute" ), minute_of_hour<int>( return_value ) );
        smenu.addentry( static_cast<int>( calendar_ui::granularity::turn ), true,
                        't', "%s: %d", _( "turn" ), to_turns<int>( return_value - calendar::turn_zero ) );

        for( uilist_entry &entry : smenu.entries ) {
            if( entry.retval > static_cast<int>( granularity_level ) ) {
                entry.enabled = false;
            }
        }
        smenu.selected = iSel;
        smenu.query();

        int auto_value = 0;
        switch( smenu.ret ) {
            case static_cast<int>( calendar_ui::granularity::year ):
                set_turn( years( return_value ) + 1, calendar::year_length(), _( "Set year to?" ) );
                break;
            case static_cast<int>( calendar_ui::granularity::season ):
                set_turn( static_cast<int>( season_of_year( return_value ) ), calendar::season_length(),
                          _( "Set season to?  (0 = spring)" ) );
                break;
            case static_cast<int>( calendar_ui::granularity::day ):
                set_turn( day_of_season<int>( return_value ) + 1, 1_days, _( "Set days to?" ) );
                break;
            case static_cast<int>( calendar_ui::granularity::hour ):
                set_turn( hour_of_day<int>( return_value ), 1_hours, _( "Set hour to?" ) );
                break;
            case static_cast<int>( calendar_ui::granularity::minute ):
                set_turn( minute_of_hour<int>( return_value ), 1_minutes, _( "Set minute to?" ) );
                break;
            case static_cast<int>( calendar_ui::granularity::turn ):
                set_turn( to_turns<int>( return_value - calendar::turn_zero ), 1_turns,
                          string_format( _( "Set turn to?  (One day is %i turns)" ), to_turns<int>( 1_days ) ).c_str() );
                break;
            case UILIST_ADDITIONAL:
                if( smenu.ret_act == "LEFT" ) {
                    auto_value = -1;
                } else if( smenu.ret_act == "RIGHT" ) {
                    auto_value = 1;
                } else if( smenu.ret_act == "RESET" ) {
                    auto_value = 0;
                    return_value = initial_value;
                }
                if( auto_value != 0 ) {
                    switch( smenu.selected ) {
                        case static_cast<int>( calendar_ui::granularity::year ):
                            set_turn( years( return_value ) + 1, calendar::year_length(), "", auto_value );
                            break;
                        case static_cast<int>( calendar_ui::granularity::season ):
                            set_turn( static_cast<int>( season_of_year( return_value ) ), calendar::season_length(), "",
                                      auto_value );
                            break;
                        case static_cast<int>( calendar_ui::granularity::day ):
                            set_turn( day_of_season<int>( return_value ) + 1, 1_days, "", auto_value );
                            break;
                        case static_cast<int>( calendar_ui::granularity::hour ):
                            set_turn( hour_of_day<int>( return_value ), 1_hours, "", auto_value );
                            break;
                        case static_cast<int>( calendar_ui::granularity::minute ):
                            set_turn( minute_of_hour<int>( return_value ), 1_minutes, "", auto_value );
                            break;
                        case static_cast<int>( calendar_ui::granularity::turn ):
                            set_turn( to_turns<int>( return_value - calendar::turn_zero ), 1_turns, "", auto_value );
                            break;
                        default:
                            break;
                    }
                }
                break;
            default:
                break;
        }
        iSel = smenu.selected;
    } while( smenu.ret != UILIST_CANCEL );
    return return_value;
}
