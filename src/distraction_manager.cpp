#include "distraction_manager.h"

#include <string>
#include <vector>

#include "color.h"
#include "cursesdef.h"
#include "input_context.h"
#include "output.h"
#include "point.h"
#include "translations.h"
#include "uilist.h"
#include "ui_manager.h"
#include "uistate.h"

namespace distraction_manager
{

struct configurable_distraction {
    bool *state;
    std::string name;
    std::string description;
    bool is_toggle = false;
};

static const std::vector<configurable_distraction> &get_configurable_distractions()
{
    static const std::vector<configurable_distraction> configurable_distractions = {
        {&uistate.distraction_noise,           translate_marker( "Noise" ),                        translate_marker( "This distraction will interrupt your activity when you hear a noise." )},
        {&uistate.distraction_pain,            translate_marker( "Pain" ),                         translate_marker( "This distraction will interrupt your activity when you feel pain." )},
        {&uistate.distraction_attack,          translate_marker( "Attack" ),                       translate_marker( "This distraction will interrupt your activity when you're attacked." )},
        {&uistate.distraction_hostile_close,   translate_marker( "Hostile is dangerously close" ), translate_marker( "This distraction will interrupt your activity when a hostile comes within 5 tiles from you." )},
        {&uistate.distraction_hostile_spotted, translate_marker( "Hostile spotted" ),              translate_marker( "This distraction will interrupt your activity when you spot a hostile." )},
        {&uistate.distraction_conversation,    translate_marker( "Conversation" ),                 translate_marker( "This distraction will interrupt your activity when someone starts a conversation with you." )},
        {&uistate.distraction_asthma,          translate_marker( "Asthma" ),                       translate_marker( "This distraction will interrupt your activity when you suffer an asthma attack." )},
        {&uistate.distraction_dangerous_field, translate_marker( "Dangerous field" ),              translate_marker( "This distraction will interrupt your activity when you're standing in a dangerous field." )},
        {&uistate.distraction_weather_change,  translate_marker( "Weather change" ),               translate_marker( "This distraction will interrupt your activity when weather changes to dangerous." )},
        {&uistate.distraction_hunger,          translate_marker( "Hunger" ),                       translate_marker( "This distraction will interrupt your activity when you're at risk of starving." )},
        {&uistate.distraction_thirst,          translate_marker( "Thirst" ),                       translate_marker( "This distraction will interrupt your activity when you're dangerously dehydrated." )},
        {&uistate.distraction_temperature,     translate_marker( "Temperature" ),                  translate_marker( "This distraction will interrupt your activity when your temperature is very high or very low." )},
        {&uistate.distraction_mutation,        translate_marker( "Mutation" ),                     translate_marker( "This distraction will interrupt your activity when you gain or lose a mutation." )},
        {&uistate.distraction_oxygen,          translate_marker( "Asphyxiation" ),                 translate_marker( "This distraction will interrupt your activity when you can't breathe." )},
        {&uistate.distraction_withdrawal,      translate_marker( "Withdrawal" ),                  translate_marker( "This distraction will interrupt your activity when you have withdrawals." )},
        {&uistate.distraction_all,             translate_marker( "Toggle all" ),                   translate_marker( "Toggle all distractions" ), true }
    };
    return configurable_distractions;
}

void distraction_manager_gui::show()
{
    const int iHeaderHeight = 4;
    int iContentHeight = 0;
    const int number_of_distractions = get_configurable_distractions().size();
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
                                       iOffset + point::south_east );

        w = catacurses::newwin( iContentHeight, FULL_SCREEN_WIDTH - 2,
                                iOffset + point( 1, iHeaderHeight + 1 ) );

        ui.position_from_window( w_border );
    } );
    ui.mark_resize();

    int currentLine = 0;
    int startPosition = 0;

    input_context ctxt{ "DISTRACTION_MANAGER" };
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_leftright();
    ctxt.register_navigate_ui_list();

    ui.on_redraw( [&]( const ui_adaptor & ) {
        // Draw border
        draw_border( w_border, BORDER_COLOR, _( "Distractions manager" ) );
        wattron( w_border, c_light_gray );
        mvwaddch( w_border, point( 0, iHeaderHeight - 1 ), LINE_XXXO );
        mvwaddch( w_border, point( 79, iHeaderHeight - 1 ), LINE_XOXX );
        mvwaddch( w_border, point( 61, FULL_SCREEN_HEIGHT - 1 ), LINE_XXOX );
        wattroff( w_border, c_light_gray );
        wnoutrefresh( w_border );

        // Draw header
        werase( w_header );
        fold_and_print( w_header, point::zero, getmaxx( w_header ), c_white,
                        _( get_configurable_distractions()[currentLine].description.c_str() ) );

        // Draw horizontal line and corner pieces of the table
        wattron( w_header, c_light_gray );
        mvwhline( w_header, point( 0, iHeaderHeight - 2 ), LINE_OXOX, 78 );
        mvwaddch( w_header, point( 60, iHeaderHeight - 2 ), LINE_OXXX );
        mvwaddch( w_header, point( 60, iHeaderHeight - 1 ), LINE_XOXO );
        wattroff( w_header, c_light_gray );

        wnoutrefresh( w_header );

        // Clear table
        mvwrectf( w, point::zero, c_black, ' ', 79, iContentHeight );
        mvwvline( w, point( 60, 0 ), c_light_gray, LINE_XOXO, iContentHeight ) ;

        draw_scrollbar( w_border, currentLine, iContentHeight, number_of_distractions, point( 0,
                        iHeaderHeight + 1 ) );

        calcStartPos( startPosition, currentLine, iContentHeight, number_of_distractions );

        for( int i = startPosition; i < number_of_distractions; ++i ) {
            const nc_color line_color = i == currentLine ? hilite( c_white ) : c_white;
            const nc_color status_color = *( get_configurable_distractions()[i].state ) ? c_light_green :
                                          c_red;
            const std::string status_string = *( get_configurable_distractions()[i].state ) ?
                                              _( "Enabled" ) :
                                              _( "Disabled" );

            // Print distraction types
            mvwprintz( w, point( 1, i - startPosition ), line_color, "%s",
                       _( get_configurable_distractions()[i].name.c_str() ) );

            // Print "Enabled/Disabled" text
            mvwprintz( w, point( 62, i - startPosition ), status_color, "%s", status_string );
        }

        wnoutrefresh( w_header );
        wnoutrefresh( w_border );
        wnoutrefresh( w );
    } );

    while( true ) {
        ui_manager::redraw();

        const std::string action = ctxt.handle_input();

        if( action == "QUIT" ) {
            break;
        }

        bool toggle_state;
        if( navigate_ui_list( action, currentLine, 5, number_of_distractions, true ) ) {
        } else if( action == "CONFIRM" || action == "LEFT" || action == "RIGHT" ) {
            *( get_configurable_distractions()[currentLine].state ) ^= true;
            if( get_configurable_distractions()[currentLine].is_toggle ) {
                toggle_state = uistate.distraction_all;
                uistate.distraction_noise = toggle_state;
                uistate.distraction_pain = toggle_state;
                uistate.distraction_attack = toggle_state;
                uistate.distraction_hostile_close = toggle_state;
                uistate.distraction_hostile_spotted = toggle_state;
                uistate.distraction_conversation = toggle_state;
                uistate.distraction_asthma = toggle_state;
                uistate.distraction_dangerous_field = toggle_state;
                uistate.distraction_weather_change = toggle_state;
                uistate.distraction_hunger = toggle_state;
                uistate.distraction_thirst = toggle_state;
                uistate.distraction_temperature = toggle_state;
                uistate.distraction_mutation = toggle_state;
                uistate.distraction_oxygen = toggle_state;
                uistate.distraction_withdrawal = toggle_state;
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
