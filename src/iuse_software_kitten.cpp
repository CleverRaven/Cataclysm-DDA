#include "iuse_software_kitten.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>  // Needed for rand()
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "cuboid_rectangle.h"
#include "input.h"
#include "input_context.h"
#include "output.h"
#include "rng.h"
#include "text_snippets.h"
#include "translations.h"
#include "ui_manager.h"

static constexpr int EMPTY = -1;
static constexpr int ROBOT = 0;
static constexpr int KITTEN = 1;

robot_finds_kitten::robot_finds_kitten()
{
    ret = false;
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    char ktile[83] =
        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!#&()*+./:;=?![]{|}y";

    nummessages = 201;
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    empty.pos = point( -1, -1 );
    empty.color = nc_color();
    empty.character = ' ';
    rfkscreen.fill( EMPTY );
    /* Create an array to ensure we don't get duplicate messages. */
    for( int c = 0; c < nummessages; c++ ) {
        bogus[c] = empty;
    }
    /* Now we initialize the various game OBJECTs.
       * Assign a position to the player. */
    robot.pos.x = rng( 0, rfkCOLS - 1 );
    robot.pos.y = rng( 0, rfkLINES - 3 - 1 ) + 3;
    robot.character = '#';
    robot.color = c_white;
    rfkscreen[robot.pos.x][robot.pos.y] = ROBOT;

    /* Assign the kitten a unique position. */
    do {
        kitten.pos.x = rng( 0, rfkCOLS - 1 );
        kitten.pos.y = rng( 0, rfkLINES - 3 - 1 ) + 3;
    } while( rfkscreen[kitten.pos.x][kitten.pos.y] != EMPTY );

    /* Assign the kitten a character and a color. */
    do {
        kitten.character = ktile[rng( 0, 81 )];
    } while( kitten.character == '#' || kitten.character == ' ' );

    do {
        kitten.color = all_colors.get_random();
    } while( kitten.color == c_black );

    rfkscreen[kitten.pos.x][kitten.pos.y] = KITTEN;

    /* Now, initialize non-kitten OBJECTs. */
    for( int c = 0; c < numbogus; c++ ) {
        /* Assign a unique position. */
        do {
            bogus[c].pos.x = rng( 0, rfkCOLS - 1 );
            bogus[c].pos.y = rng( 0, rfkLINES - 3 - 1 ) + 3;
        } while( rfkscreen[bogus[c].pos.x][bogus[c].pos.y] != EMPTY );
        rfkscreen[bogus[c].pos.x][bogus[c].pos.y] = c + 2;

        /* Assign a character. */
        do {
            bogus[c].character = ktile[rng( 0, 81 )];
        } while( bogus[c].character == '#' || bogus[c].character == ' ' );

        do {
            bogus[c].color = all_colors.get_random();
        } while( bogus[c].color == c_black );

        /* Assign a unique message. */
        bool got_nki = false;
        while( !got_nki ) {
            std::string nki = SNIPPET.random_from_category( "non_kitten_items" ).value_or(
                                  translation() ).translated();
            // Add NKI if it does not already exist in the list
            if( std::find( bogus_messages.begin(), bogus_messages.end(), nki ) == bogus_messages.end() ) {
                bogus_messages.push_back( nki );
                got_nki = true;
            }
        }
    }

    ui_adaptor ui;
    ui.on_screen_resize( [this]( ui_adaptor & ui ) {
        bkatwin = catacurses::newwin( rfkLINES + 2, rfkCOLS + 2,
                                      point( ( TERMX - rfkCOLS - 2 ) / 2, ( TERMY - rfkLINES - 2 ) / 2 ) );
        w = catacurses::newwin( rfkLINES, rfkCOLS,
                                point( ( TERMX - rfkCOLS ) / 2, ( TERMY - rfkLINES ) / 2 ) );
        ui.position_from_window( bkatwin );
    } );
    ui.mark_resize();

    ui.on_redraw( [this]( const ui_adaptor & ) {
        show();
    } );

    /* Now the fun begins. */
    current_ui_state = ui_state::instructions;
    while( current_ui_state != ui_state::exit ) {
        ui_manager::redraw();
        process_input();
    }
}

void robot_finds_kitten::show() const
{
    input_context ctxt( "IUSE_SOFTWARE_KITTEN" );

    draw_border( bkatwin );
    wnoutrefresh( bkatwin );

    werase( w );
    if( current_ui_state != ui_state::instructions ) {
        for( int c = 0; c < rfkCOLS; c++ ) {
            mvwputch( w, point( c, 2 ), BORDER_COLOR, '_' );
        }
        wmove( w, kitten.pos );
        draw_kitten();

        for( int c = 0; c < numbogus; c++ ) {
            mvwputch( w, bogus[c].pos, bogus[c].color, bogus[c].character );
        }

        wmove( w, robot.pos );
        draw_robot();
    }
    switch( current_ui_state ) {
        case ui_state::instructions: {
            int pos = 1;
            // NOLINTNEXTLINE(cata-use-named-point-constants)
            pos += fold_and_print( w, point( 1, 0 ), getmaxx( w ) - 4, c_light_gray,
                                   _( "robotfindskitten v22July2008" ) );
            pos += 1 + fold_and_print( w, point( 1, pos ), getmaxx( w ) - 4, c_light_gray,
                                       _( "Originally by the illustrious Leonard Richardson, "
                                          "rewritten in PDCurses by Joseph Larson, "
                                          "ported to CDDA gaming system by a nutcase." ) );

            pos += 1 + fold_and_print( w, point( 1, pos ), getmaxx( w ) - 4, c_light_gray,
                                       _( "In this game, you are robot (" ) );
            draw_robot();
            wprintz( w, c_light_gray, _( ")." ) );
            pos += 1 + fold_and_print( w, point( 1, pos ), getmaxx( w ) - 4, c_light_gray,
                                       _( "Your job is to find kitten.  This task is complicated by the existence of various things "
                                          "which are not kitten.  Robot must touch items to determine if they are kitten or not.  "
                                          "The game ends when robot finds kitten.  Alternatively, you may end the game by hitting %s." ),
                                       ctxt.get_desc( "QUIT" ) );
            fold_and_print( w, point( 1, pos ), getmaxx( w ) - 4, c_light_gray,
                            _( "Press any key to start." ) );
            break;
        }
        case ui_state::main:
            mvwprintz( w, point_zero, c_white, _( "robotfindskitten v22July2008 - press %s to quit." ),
                       ctxt.get_desc( "QUIT" ) );
            break;
        case ui_state::invalid_input:
            mvwprintz( w, point_zero, c_white, _( "Invalid command: Use direction keys or press %s to quit." ),
                       ctxt.get_desc( "QUIT" ) );
            break;
        case ui_state::bogus_message: {
            std::vector<std::string> bogusvstr = foldstring( this_bogus_message, rfkCOLS );
            for( size_t c = 0; c < bogusvstr.size(); c++ ) {
                mvwprintz( w, point( 0, c ), c_white, bogusvstr[c] );
            }
            break;
        }
        case ui_state::end_animation: {
            /* The grand cinema scene. */
            const int c = std::min( end_animation_frame, 3 );
            wmove( w, point( rfkCOLS / 2 - 4 + c, 1 ) );
            if( end_animation_last_input_left_or_up ) {
                draw_kitten();
            } else {
                draw_robot();
            }
            wmove( w, point( rfkCOLS / 2 + 3 - c, 1 ) );
            if( end_animation_last_input_left_or_up ) {
                draw_robot();
            } else {
                draw_kitten();
            }
            if( end_animation_frame >= 4 ) {
                /* They're in love! */
                mvwprintz( w, point( ( rfkCOLS - 6 ) / 2 - 1, 0 ), c_light_red, "<3<3<3" );
            }
            if( end_animation_frame >= 5 ) {
                mvwprintz( w, point_zero, c_white, _( "You found kitten!  Way to go, robot!" ) );
            }
            break;
        }
        case ui_state::exit:
            break;
    }
    wnoutrefresh( w );
}

void robot_finds_kitten::process_input()
{
    input_context ctxt( "IUSE_SOFTWARE_KITTEN" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "ANY_INPUT" );
    switch( current_ui_state ) {
        case ui_state::instructions: {
            const std::string action = ctxt.handle_input();
            if( action == "QUIT" ) {
                current_ui_state = ui_state::exit;
            } else if( action == "ANY_INPUT" && !ctxt.get_raw_input().sequence.empty() ) {
                current_ui_state = ui_state::main;
            }
            break;
        }
        case ui_state::main:
        case ui_state::invalid_input:
        case ui_state::bogus_message: {
            ctxt.register_cardinal();
            const std::string action = ctxt.handle_input();
            if( action == "QUIT" ) {
                current_ui_state = ui_state::exit;
            } else if( action == "ANY_INPUT" ) {
                if( !ctxt.get_raw_input().sequence.empty() ) {
                    current_ui_state = ui_state::invalid_input;
                }
            } else if( action != "HELP_KEYBINDINGS" ) {
                current_ui_state = ui_state::main;
                point check = robot.pos;

                if( action == "UP" ) {
                    check.y--;
                } else if( action == "DOWN" ) {
                    check.y++;
                } else if( action == "LEFT" ) {
                    check.x--;
                } else if( action == "RIGHT" ) {
                    check.x++;
                }

                constexpr half_open_rectangle<point> bounds(
                    point( 0, 3 ), point( rfkCOLS, rfkLINES ) );
                if( !bounds.contains( check ) ) {
                    /* Can't move past edge */
                } else if( rfkscreen[check.x][check.y] != EMPTY ) {
                    switch( rfkscreen[check.x][check.y] ) {
                        case ROBOT:
                            /* We didn't move. */
                            break;
                        case KITTEN:
                            /* Found it! */
                            ret = true;
                            current_ui_state = ui_state::end_animation;
                            end_animation_frame = 0;
                            end_animation_last_input_left_or_up = action == "UP" || action == "LEFT";
                            break;
                        default:
                            current_ui_state = ui_state::bogus_message;
                            this_bogus_message = bogus_messages.at( rfkscreen[check.x][check.y] - 2 );
                            break;
                    }
                } else {
                    /* Otherwise, move the robot. */
                    rfkscreen[robot.pos.x][robot.pos.y] = EMPTY;
                    robot.pos = check;
                    rfkscreen[robot.pos.x][robot.pos.y] = ROBOT;
                }
            }
            break;
        }
        case ui_state::end_animation:
            if( end_animation_frame + 1 == num_end_animation_frames ) {
                const std::string action = ctxt.handle_input();
                if( action != "HELP_KEYBINDINGS" ) {
                    current_ui_state = ui_state::exit;
                }
            } else {
                refresh_display();
                // Sleep for 1 s
                const auto sleep_till = std::chrono::steady_clock::now()
                                        + std::chrono::nanoseconds( 1'000'000'000 );
                do {
                    const auto sleep_for = std::min( sleep_till - std::chrono::steady_clock::now(),
                                                     // Pump events every 100 ms
                                                     std::chrono::nanoseconds( 100'000'000 ) );
                    if( sleep_for > std::chrono::nanoseconds( 0 ) ) {
                        std::this_thread::sleep_for( sleep_for );
                        inp_mngr.pump_events();
                    } else {
                        break;
                    }
                } while( true );
                end_animation_frame++;
            }
            break;
        case ui_state::exit:
            break;
    }
}

/* Draws robot at current position */
void robot_finds_kitten::draw_robot() const
{
    wputch( w, robot.color, robot.character );
}

/* Draws kitten at current position */
void robot_finds_kitten::draw_kitten() const
{
    wputch( w, kitten.color, kitten.character );
}
