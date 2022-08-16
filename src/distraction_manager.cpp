#include "distraction_manager.h"

#include <functional>
#include <string>

#include "color.h"
#include "cursesdef.h"
#include "output.h"
#include "point.h"
#include "translations.h"
#include "ui.h"
#include "ui_manager.h"
#include "uistate.h"

namespace distraction_manager
{

static const std::vector<std::pair<std::string, std::string>> configurable_distractions = {
    {translate_marker( "Noise" ),                        translate_marker( "This distraction will interrupt your activity when you hear a noise." )},
    {translate_marker( "Pain" ),                         translate_marker( "This distraction will interrupt your activity when you feel pain." )},
    {translate_marker( "Attack" ),                       translate_marker( "This distraction will interrupt your activity when you're attacked." )},
    {translate_marker( "Hostile is dangerously close" ), translate_marker( "This distraction will interrupt your activity when a hostile comes within 5 tiles from you." )},
    {translate_marker( "Hostile spotted" ),              translate_marker( "This distraction will interrupt your activity when you spot a hostile." )},
    {translate_marker( "Conversation" ),                 translate_marker( "This distraction will interrupt your activity when someone starts a conversation with you." )},
    {translate_marker( "Asthma" ),                       translate_marker( "This distraction will interrupt your activity when you suffer an asthma attack." )},
    {translate_marker( "Dangerous field" ),              translate_marker( "This distraction will interrupt your activity when you're standing in a dangerous field." )},
    {translate_marker( "Weather change" ),               translate_marker( "This distraction will interrupt your activity when weather changes to dangerous." )},
    {translate_marker( "Hunger" ),                       translate_marker( "This distraction will interrupt your activity when you're at risk of starving." )},
    {translate_marker( "Thirst" ),                       translate_marker( "This distraction will interrupt your activity when you're dangerously dehydrated." )},
};

void distraction_manager_gui::show()
{
    const int iHeaderHeight = 4;
    int iContentHeight = 0;
    const int number_of_distractions = configurable_distractions.size();
    catacurses::window w_border;
    catacurses::window w_header;
    catacurses::window w;

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        iContentHeight = FULL_SCREEN_HEIGHT - 2 - iHeaderHeight;

        const point iOffset( TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0,
                             TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0 );

        w_border = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                       iOffset );

        w_header = catacurses::newwin( iHeaderHeight, FULL_SCREEN_WIDTH - 2,
                                       iOffset + point_south_east );

        w = catacurses::newwin( iContentHeight, FULL_SCREEN_WIDTH - 2,
                                iOffset + point( 1, iHeaderHeight + 1 ) );

        ui.position_from_window( w_border );
    } );
    ui.mark_resize();

    int currentLine = 0;
    int startPosition = 0;

    std::vector<bool> distractions_status = {
        uistate.distraction_noise,
        uistate.distraction_pain,
        uistate.distraction_attack,
        uistate.distraction_hostile_close,
        uistate.distraction_hostile_spotted,
        uistate.distraction_conversation,
        uistate.distraction_asthma,
        uistate.distraction_dangerous_field,
        uistate.distraction_weather_change,
        uistate.distraction_hunger,
        uistate.distraction_thirst,
    };

    input_context ctx{ "DISTRACTION_MANAGER" };
    ctx.register_action( "QUIT" );
    ctx.register_action( "HELP_KEYBINDINGS" );
    ctx.register_cardinal();
    ctx.register_action( "CONFIRM" );

    ui.on_redraw( [&]( const ui_adaptor & ) {
        // Draw border
        draw_border( w_border, BORDER_COLOR, _( "Distractions manager" ) );
        mvwputch( w_border, point( 0, iHeaderHeight - 1 ), c_light_gray, LINE_XXXO );
        mvwputch( w_border, point( 79, iHeaderHeight - 1 ), c_light_gray, LINE_XOXX );
        mvwputch( w_border, point( 61, FULL_SCREEN_HEIGHT - 1 ), c_light_gray, LINE_XXOX );
        wnoutrefresh( w_border );

        // Draw header
        werase( w_header );
        fold_and_print( w_header, point_zero, getmaxx( w_header ), c_white,
                        _( configurable_distractions[currentLine].second.c_str() ) );

        // Draw horizontal line and corner pieces of the table
        for( int x = 0; x < 78; x++ ) {
            if( x == 60 ) {
                mvwputch( w_header, point( x, iHeaderHeight - 2 ), c_light_gray, LINE_OXXX );
                mvwputch( w_header, point( x, iHeaderHeight - 1 ), c_light_gray, LINE_XOXO );
            } else {
                mvwputch( w_header, point( x, iHeaderHeight - 2 ), c_light_gray, LINE_OXOX );
            }
        }

        wnoutrefresh( w_header );

        // Clear table
        for( int y = 0; y < iContentHeight; y++ ) {
            for( int x = 0; x < 79; x++ ) {
                if( x == 60 ) {
                    mvwputch( w, point( x, y ), c_light_gray, LINE_XOXO );
                } else {
                    mvwputch( w, point( x, y ), c_black, ' ' );
                }
            }
        }

        draw_scrollbar( w_border, currentLine, iContentHeight, number_of_distractions, point( 0,
                        iHeaderHeight + 1 ) );

        calcStartPos( startPosition, currentLine, iContentHeight, number_of_distractions );

        for( int i = startPosition; i < number_of_distractions; ++i ) {
            const nc_color line_color = i == currentLine ? hilite( c_white ) : c_white;
            const nc_color status_color = distractions_status[i] ? c_light_green : c_red;
            const std::string status_string = distractions_status[i] ? _( "Enabled" ) : _( "Disabled" );

            // Print distraction types
            mvwprintz( w, point( 1, i - startPosition ), line_color, "%s",
                       _( configurable_distractions[i].first.c_str() ) );

            // Print "Enabled/Disabled" text
            mvwprintz( w, point( 62, i - startPosition ), status_color, "%s", status_string );
        }

        wnoutrefresh( w_header );
        wnoutrefresh( w_border );
        wnoutrefresh( w );
    } );

    while( true ) {
        ui_manager::redraw();

        const std::string currentAction = ctx.handle_input();

        if( currentAction == "QUIT" ) {
            break;
        }

        if( currentAction == "UP" ) {
            if( currentLine > 0 ) {
                --currentLine;
            } else {
                currentLine = number_of_distractions - 1;
            }
        } else if( currentAction == "DOWN" ) {
            if( currentLine == number_of_distractions - 1 ) {
                currentLine = 0;
            } else {
                ++currentLine;
            }
        } else if( currentAction == "CONFIRM" ) {
            // This will change status color and status text
            distractions_status[currentLine] = !distractions_status[currentLine];

            // This will actually toggle enabled/disabled status of distractions
            switch( currentLine ) {
                case 0:
                    uistate.distraction_noise = !uistate.distraction_noise;
                    break;
                case 1:
                    uistate.distraction_pain = !uistate.distraction_pain;
                    break;
                case 2:
                    uistate.distraction_attack = !uistate.distraction_attack;
                    break;
                case 3:
                    uistate.distraction_hostile_close = !uistate.distraction_hostile_close;
                    break;
                case 4:
                    uistate.distraction_hostile_spotted = !uistate.distraction_hostile_spotted;
                    break;
                case 5:
                    uistate.distraction_conversation = !uistate.distraction_conversation;
                    break;
                case 6:
                    uistate.distraction_asthma = !uistate.distraction_asthma;
                    break;
                case 7:
                    uistate.distraction_dangerous_field = !uistate.distraction_dangerous_field;
                    break;
                case 8:
                    uistate.distraction_weather_change = !uistate.distraction_weather_change;
                    break;
                case 9:
                    uistate.distraction_hunger = !uistate.distraction_hunger;
                    break;
                case 10:
                    uistate.distraction_thirst = !uistate.distraction_thirst;
                    break;
                default:
                    return;
            }
        }
    }
}

} // namespace distraction_manager

distraction_manager::distraction_manager_gui &get_distraction_manager()
{
    static distraction_manager::distraction_manager_gui staticSettings;
    return staticSettings;
}
