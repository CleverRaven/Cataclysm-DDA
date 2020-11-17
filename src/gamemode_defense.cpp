#include "gamemode_defense.h" // IWYU pragma: associated

#include <cassert>
#include <cstddef>
#include <memory>
#include <ostream>
#include <set>

#include "action.h"
#include "avatar.h"
#include "basecamp.h"
#include "color.h"
#include "construction.h"
#include "coordinate_conversions.h"
#include "cursesdef.h"
#include "debug.h"
#include "game.h"
#include "game_constants.h"
#include "input.h"
#include "item.h"
#include "item_group.h"
#include "map.h"
#include "messages.h"
#include "mongroup.h"
#include "monster.h"
#include "monstergenerator.h"
#include "mtype.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "player.h"
#include "pldata.h"
#include "point.h"
#include "popup.h"
#include "rng.h"
#include "string_formatter.h"
#include "string_id.h"
#include "translations.h"
#include "ui_manager.h"
#include "weather.h"

static const skill_id skill_barter( "barter" );

static const mongroup_id GROUP_NETHER( "GROUP_NETHER" );
static const mongroup_id GROUP_ROBOT( "GROUP_ROBOT" );
static const mongroup_id GROUP_SPIDER( "GROUP_SPIDER" );
static const mongroup_id GROUP_TRIFFID( "GROUP_TRIFFID" );
static const mongroup_id GROUP_VANILLA( "GROUP_VANILLA" );
static const mongroup_id GROUP_ZOMBIE( "GROUP_ZOMBIE" );

#define SPECIAL_WAVE_CHANCE 5 // One in X chance of single-flavor wave
#define SPECIAL_WAVE_MIN 5 // Don't use a special wave with < X monsters

#define SELCOL(n) (selection == (n) ? c_yellow : c_blue)
#define TOGCOL(n, b) (selection == (n) ? ((b) ? c_light_green : c_yellow) :\
                      ((b) ? c_green : c_dark_gray))
#define NUMALIGN(n) ((n) >= 10000 ? 20 : ((n) >= 1000 ? 21 :\
                     ((n) >= 100 ? 22 : ((n) >= 10 ? 23 : 24))))

std::string caravan_category_name( caravan_category cat );
std::vector<itype_id> caravan_items( caravan_category cat );
std::set<m_flag> monflags_to_add;

int caravan_price( player &u, int price );

void draw_caravan_borders( const catacurses::window &w, int current_window );
void draw_caravan_categories( const catacurses::window &w, int category_selected,
                              int total_price, int cash );
void draw_caravan_items( const catacurses::window &w, std::vector<itype_id> *items,
                         std::vector<int> *counts, int offset, int item_selected );

std::string defense_style_name( defense_style style );
std::string defense_style_description( defense_style style );
std::string defense_location_name( defense_location location );
std::string defense_location_description( defense_location location );

defense_game::defense_game()
    : time_between_waves( 0_turns )
{
    current_wave = 0;
    hunger = false;
    thirst = false;
    sleep  = false;
    zombies = false;
    specials = false;
    spiders = false;
    triffids = false;
    robots = false;
    subspace = false;
    mercenaries = false;
    init_to_style( DEFENSE_EASY );
}

bool defense_game::init()
{
    calendar::turn = calendar::turn_zero + 12_hours; // Start at noon
    g->weather.temperature = 65;
    if( !g->u.create( character_type::CUSTOM ) ) {
        return false;
    }
    g->u.str_cur = g->u.str_max;
    g->u.per_cur = g->u.per_max;
    g->u.int_cur = g->u.int_max;
    g->u.dex_cur = g->u.dex_max;
    init_mtypes();
    init_constructions();
    current_wave = 0;
    hunger = false;
    thirst = false;
    sleep  = false;
    zombies = false;
    specials = false;
    spiders = false;
    triffids = false;
    robots = false;
    subspace = false;
    mercenaries = false;
    allow_save = false;
    init_to_style( DEFENSE_EASY );
    setup();
    g->u.cash = initial_cash;
    // TODO: support multiple defense games? clean up old defense game
    defloc_pos = tripoint( 50, 50, 0 );
    init_map();
    caravan();
    return true;
}

void defense_game::per_turn()
{
    if( !thirst ) {
        g->u.set_thirst( 0 );
    }
    if( !hunger ) {
        g->u.set_hunger( 0 );
    }
    if( !sleep ) {
        g->u.set_fatigue( 0 );
    }
    if( calendar::once_every( time_between_waves ) ) {
        current_wave++;
        if( current_wave > 1 && current_wave % waves_between_caravans == 0 ) {
            popup( _( "A caravan approaches!  Press spacebar…" ) );
            caravan();
        }
        spawn_wave();
    }
}

void defense_game::pre_action( action_id &act )
{
    std::string action_error_message;
    switch( act ) {
        case ACTION_SLEEP:
            if( !sleep ) {
                action_error_message = _( "You don't need to sleep!" );
            }
            break;
        case ACTION_SAVE:
        case ACTION_QUICKSAVE:
            if( !allow_save ) {
                action_error_message = _( "You cannot save in defense mode!" );
            }
            break;
        case ACTION_MOVE_FORTH:
        case ACTION_MOVE_FORTH_RIGHT:
        case ACTION_MOVE_RIGHT:
        case ACTION_MOVE_BACK_RIGHT:
        case ACTION_MOVE_BACK:
        case ACTION_MOVE_BACK_LEFT:
        case ACTION_MOVE_LEFT:
        case ACTION_MOVE_FORTH_LEFT: {
            const point delta = get_delta_from_movement_action( act, iso_rotate::yes );
            if( ( delta.y < 0 && g->u.posy() == HALF_MAPSIZE_Y && g->get_levy() <= 93 )
                || ( delta.y > 0 && g->u.posy() == HALF_MAPSIZE_Y + SEEY - 1 && g->get_levy() >= 98 )
                || ( delta.x < 0 && g->u.posx() == HALF_MAPSIZE_X && g->get_levx() <= 93 )
                || ( delta.x > 0 && g->u.posx() == HALF_MAPSIZE_X + SEEX - 1 && g->get_levx() >= 98 ) ) {
                action_error_message = string_format( _( "You cannot leave the %s behind!" ),
                                                      defense_location_name( location ) );
            }
        }
        break;
        default:
            break;
    }
    if( !action_error_message.empty() ) {
        add_msg( m_info, action_error_message );
        act = ACTION_NULL;
    }
}

void defense_game::post_action( action_id /*act*/ )
{
}

void defense_game::game_over()
{
    popup( _( "You managed to survive through wave %d!" ), current_wave );
}

void defense_game::init_mtypes()
{
    for( auto &type : MonsterGenerator::generator().get_all_mtypes() ) {
        mtype *const t = const_cast<mtype *>( &type );
        t->difficulty *= 1.5;
        t->difficulty += static_cast<int>( t->difficulty / 5 );
        t->set_flag( MF_BASHES );
        t->set_flag( MF_SMELLS );
        t->set_flag( MF_HEARS );
        t->set_flag( MF_SEES );
    }
}

void defense_game::init_constructions()
{
    standardize_construction_times( 1 ); // Everything takes 1 minute
}

void defense_game::init_map()
{
    background_pane background;
    static_popup popup;
    popup.message( _( "Please wait as the map generates [%2d%%]" ), 0 );
    ui_manager::redraw();
    refresh_display();

    auto &starting_om = overmap_buffer.get( point_zero );
    for( int x = 0; x < OMAPX; x++ ) {
        for( int y = 0; y < OMAPY; y++ ) {
            tripoint p( x, y, 0 );
            starting_om.ter_set( p, oter_id( "field" ) );
            starting_om.seen( p ) = true;
        }
    }

    switch( location ) {
        case DEFLOC_NULL:
        case NUM_DEFENSE_LOCATIONS:
            defloc_special = overmap_special_id( "house_two_story_basement" );
            DebugLog( D_ERROR, D_GAME ) << "defense location is invalid: " << location;
            break;

        case DEFLOC_HOSPITAL:
            defloc_special = overmap_special_id( "hospital" );
            break;

        case DEFLOC_WORKS:
            defloc_special = overmap_special_id( "public_works" );
            break;

        case DEFLOC_MALL:
            defloc_special = overmap_special_id( "megastore" );
            break;

        case DEFLOC_BAR:
            defloc_special = overmap_special_id( "bar" );
            break;

        case DEFLOC_MANSION:
            defloc_special = overmap_special_id( "Mansion_Wild" );
            break;
    }
    starting_om.place_special_forced( defloc_special, defloc_pos, om_direction::type::north );

    starting_om.save();

    // Init the map
    int old_percent = 0;
    for( int i = 0; i <= MAPSIZE * 2; i += 2 ) {
        for( int j = 0; j <= MAPSIZE * 2; j += 2 ) {
            int mx = 100 - MAPSIZE + i;
            int my = 100 - MAPSIZE + j;
            int percent = 100 * ( ( j / 2 + MAPSIZE * ( i / 2 ) ) ) /
                          ( ( MAPSIZE ) * ( MAPSIZE + 1 ) );
            if( percent >= old_percent + 1 ) {
                popup.message( _( "Please wait as the map generates [%2d%%]" ), percent );
                ui_manager::redraw();
                refresh_display();
                old_percent = percent;
            }
            // Round down to the nearest even number
            mx -= mx % 2;
            my -= my % 2;
            tinymap tm;
            tm.generate( tripoint( mx, my, 0 ), calendar::turn );
            tm.clear_spawns();
            tm.clear_traps();
            tm.save();
        }
    }

    g->load_map( omt_to_sm_copy( defloc_pos ) );
    g->u.setx( SEEX );
    g->u.sety( SEEY );

    g->update_map( g-> u );
    monster *const generator = g->place_critter_around( mtype_id( "mon_generator" ), g->u.pos(), 2 );
    assert( generator );
    generator->friendly = -1;
}

void defense_game::init_to_style( defense_style new_style )
{
    style = new_style;
    hunger = false;
    thirst = false;
    sleep  = false;
    zombies = false;
    specials = false;
    spiders = false;
    triffids = false;
    robots = false;
    subspace = false;
    mercenaries = false;

    switch( new_style ) {
        case NUM_DEFENSE_STYLES:
            DebugLog( D_ERROR, D_GAME ) << "invalid defense style: " << new_style;
            break;
        case DEFENSE_EASY:
        // fall through to custom
        case DEFENSE_CUSTOM:
            location = DEFLOC_HOSPITAL;
            initial_difficulty = 15;
            wave_difficulty = 10;
            time_between_waves = 30_minutes;
            waves_between_caravans = 3;
            initial_cash = 1000000;
            cash_per_wave = 100000;
            cash_increase = 30000;
            specials = true;
            spiders = true;
            triffids = true;
            mercenaries = true;
            break;

        case DEFENSE_MEDIUM:
            location = DEFLOC_MALL;
            initial_difficulty = 30;
            wave_difficulty = 15;
            time_between_waves = 20_minutes;
            waves_between_caravans = 4;
            initial_cash = 600000;
            cash_per_wave = 80000;
            cash_increase = 20000;
            specials = true;
            spiders = true;
            triffids = true;
            robots = true;
            hunger = true;
            mercenaries = true;
            break;

        case DEFENSE_HARD:
            location = DEFLOC_BAR;
            initial_difficulty = 50;
            wave_difficulty = 20;
            time_between_waves = 10_minutes;
            waves_between_caravans = 5;
            initial_cash = 200000;
            cash_per_wave = 60000;
            cash_increase = 10000;
            specials = true;
            spiders = true;
            triffids = true;
            robots = true;
            subspace = true;
            hunger = true;
            thirst = true;
            break;

        case DEFENSE_SHAUN:
            location = DEFLOC_BAR;
            initial_difficulty = 30;
            wave_difficulty = 15;
            time_between_waves = 5_minutes;
            waves_between_caravans = 6;
            initial_cash = 500000;
            cash_per_wave = 50000;
            cash_increase = 10000;
            zombies = true;
            break;

        case DEFENSE_DAWN:
            location = DEFLOC_MALL;
            initial_difficulty = 60;
            wave_difficulty = 20;
            time_between_waves = 30_minutes;
            waves_between_caravans = 4;
            initial_cash = 800000;
            cash_per_wave = 50000;
            cash_increase = 0;
            zombies = true;
            hunger = true;
            thirst = true;
            mercenaries = true;
            break;

        case DEFENSE_SPIDERS:
            location = DEFLOC_MALL;
            initial_difficulty = 60;
            wave_difficulty = 10;
            time_between_waves = 10_minutes;
            waves_between_caravans = 4;
            initial_cash = 600000;
            cash_per_wave = 50000;
            cash_increase = 10000;
            spiders = true;
            break;

        case DEFENSE_TRIFFIDS:
            location = DEFLOC_MANSION;
            initial_difficulty = 60;
            wave_difficulty = 20;
            time_between_waves = 30_minutes;
            waves_between_caravans = 2;
            initial_cash = 1000000;
            cash_per_wave = 60000;
            cash_increase = 10000;
            triffids = true;
            hunger = true;
            thirst = true;
            sleep = true;
            mercenaries = true;
            break;

        case DEFENSE_SKYNET:
            location = DEFLOC_HOSPITAL;
            initial_difficulty = 20;
            wave_difficulty = 20;
            time_between_waves = 20_minutes;
            waves_between_caravans = 6;
            initial_cash = 1200000;
            cash_per_wave = 100000;
            cash_increase = 20000;
            robots = true;
            hunger = true;
            thirst = true;
            mercenaries = true;
            break;

        case DEFENSE_LOVECRAFT:
            location = DEFLOC_MANSION;
            initial_difficulty = 20;
            wave_difficulty = 20;
            time_between_waves = 120_minutes;
            waves_between_caravans = 8;
            initial_cash = 400000;
            cash_per_wave = 100000;
            cash_increase = 10000;
            subspace = true;
            hunger = true;
            thirst = true;
            sleep = true;
            break;

    }
}

void defense_game::setup()
{
    background_pane bg_pane;

    catacurses::window w;

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        w = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                point( TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0,
                                       TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0 ) );
        ui.position_from_window( w );
    } );
    ui.mark_resize();

    int selection = 1;
    int selection_max = 20;

    ui.on_redraw( [&]( const ui_adaptor & ) {
        refresh_setup( w, selection );
    } );

    input_context ctxt( "DEFENSE_SETUP" );
    ctxt.register_action( "UP", to_translation( "Previous option" ) );
    ctxt.register_action( "DOWN", to_translation( "Next option" ) );
    ctxt.register_action( "LEFT", to_translation( "Cycle option value" ) );
    ctxt.register_action( "RIGHT", to_translation( "Cycle option value" ) );
    ctxt.register_action( "CONFIRM", to_translation( "Toggle option" ) );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "START" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    while( true ) {
        ui_manager::redraw();
        const std::string action = ctxt.handle_input();

        if( action == "START" ) {
            if( !zombies && !specials && !spiders && !triffids && !robots && !subspace ) {
                popup( _( "You must choose at least one monster group!" ) );
            } else {
                return;
            }
        } else if( action == "DOWN" ) {
            if( selection == selection_max ) {
                selection = 1;
            } else {
                selection++;
            }
        } else if( action == "UP" ) {
            if( selection == 1 ) {
                selection = selection_max;
            } else {
                selection--;
            }
        } else {
            switch( selection ) {
                case 1:
                    // Scenario selection
                    if( action == "RIGHT" ) {
                        if( style == static_cast<defense_style>( NUM_DEFENSE_STYLES - 1 ) ) {
                            style = static_cast<defense_style>( 1 );
                        } else {
                            style = static_cast<defense_style>( style + 1 );
                        }
                    }
                    if( action == "LEFT" ) {
                        if( style == static_cast<defense_style>( 1 ) ) {
                            style = static_cast<defense_style>( NUM_DEFENSE_STYLES - 1 );
                        } else {
                            style = static_cast<defense_style>( style - 1 );
                        }
                    }
                    init_to_style( style );
                    break;

                case 2:
                    // Location selection
                    if( action == "RIGHT" ) {
                        if( location == static_cast<defense_location>( NUM_DEFENSE_LOCATIONS - 1 ) ) {
                            location = static_cast<defense_location>( 1 );
                        } else {
                            location = static_cast<defense_location>( location + 1 );
                        }
                    }
                    if( action == "LEFT" ) {
                        if( location == static_cast<defense_location>( 1 ) ) {
                            location = static_cast<defense_location>( NUM_DEFENSE_LOCATIONS - 1 );
                        } else {
                            location = static_cast<defense_location>( location - 1 );
                        }
                    }
                    break;

                case 3:
                    // Difficulty of the first wave
                    if( action == "LEFT" && initial_difficulty > 10 ) {
                        initial_difficulty -= 5;
                    }
                    if( action == "RIGHT" && initial_difficulty < 995 ) {
                        initial_difficulty += 5;
                    }
                    break;

                case 4:
                    // Wave Difficulty
                    if( action == "LEFT" && wave_difficulty > 10 ) {
                        wave_difficulty -= 5;
                    }
                    if( action == "RIGHT" && wave_difficulty < 995 ) {
                        wave_difficulty += 5;
                    }
                    break;

                case 5:
                    if( action == "LEFT" && time_between_waves > 5_minutes ) {
                        time_between_waves -= 5_minutes;
                    }
                    if( action == "RIGHT" && time_between_waves < 995_minutes ) {
                        time_between_waves += 5_minutes;
                    }
                    break;

                case 6:
                    if( action == "LEFT" && waves_between_caravans > 1 ) {
                        waves_between_caravans -= 1;
                    }
                    if( action == "RIGHT" && waves_between_caravans < 50 ) {
                        waves_between_caravans += 1;
                    }
                    break;

                case 7:
                    if( action == "LEFT" && initial_cash > 0 ) {
                        initial_cash -= 100;
                    }
                    if( action == "RIGHT" && initial_cash < 1000000 ) {
                        initial_cash += 100;
                    }
                    break;

                case 8:
                    if( action == "LEFT" && cash_per_wave > 0 ) {
                        cash_per_wave -= 100;
                    }
                    if( action == "RIGHT" && cash_per_wave < 1000000 ) {
                        cash_per_wave += 100;
                    }
                    break;

                case 9:
                    if( action == "LEFT" && cash_increase > 0 ) {
                        cash_increase -= 50;
                    }
                    if( action == "RIGHT" && cash_increase < 1000000 ) {
                        cash_increase += 50;
                    }
                    break;

                case 10:
                    if( action == "CONFIRM" ) {
                        zombies = !zombies;
                        specials = false;
                    }
                    mvwprintz( w, point( 2, 18 ), ( zombies ? c_light_green : c_yellow ), "Zombies" );
                    mvwprintz( w, point( 14, 18 ), c_yellow, _( "Special Zombies" ) );
                    break;

                case 11:
                    if( action == "CONFIRM" ) {
                        specials = !specials;
                        zombies = false;
                    }
                    break;

                case 12:
                    if( action == "CONFIRM" ) {
                        spiders = !spiders;
                    }
                    break;

                case 13:
                    if( action == "CONFIRM" ) {
                        triffids = !triffids;
                    }
                    break;

                case 14:
                    if( action == "CONFIRM" ) {
                        robots = !robots;
                    }
                    break;

                case 15:
                    if( action == "CONFIRM" ) {
                        subspace = !subspace;
                    }
                    break;

                case 16:
                    if( action == "CONFIRM" ) {
                        hunger = !hunger;
                    }
                    break;

                case 17:
                    if( action == "CONFIRM" ) {
                        thirst = !thirst;
                    }
                    break;

                case 18:
                    if( action == "CONFIRM" ) {
                        sleep = !sleep;
                    }
                    break;

                case 19:
                    if( action == "CONFIRM" ) {
                        mercenaries = !mercenaries;
                    }
                    break;

                case 20:
                    if( action == "CONFIRM" ) {
                        allow_save = !allow_save;
                    }
                    break;
            }
        }
    }
}

void defense_game::refresh_setup( const catacurses::window &w, int selection )
{
    werase( w );
    draw_border( w, c_light_gray, _( "DEFENSE MODE" ), c_light_red );
    mvwprintz( w, point( 2, 1 ), c_light_red,
               _( "Press direction keys to cycle, ENTER to toggle, S to start" ) );
    mvwprintz( w, point( 2, 2 ), c_white, _( "Scenario:" ) );
    mvwprintz( w, point( 2, 3 ), SELCOL( 1 ), defense_style_name( style ) );
    mvwprintz( w, point( 28, 3 ), c_light_gray, defense_style_description( style ) );
    mvwprintz( w, point( 2, 4 ), c_white, _( "Location:" ) );
    mvwprintz( w, point( 2, 5 ), SELCOL( 2 ), defense_location_name( location ) );
    mvwprintz( w, point( 28, 5 ), c_light_gray, defense_location_description( location ) );

    mvwprintz( w, point( 2, 7 ), c_white, _( "Initial Difficulty:" ) );
    mvwprintz( w, point( NUMALIGN( initial_difficulty ), 7 ), SELCOL( 3 ), "%d", initial_difficulty );
    mvwprintz( w, point( 28, 7 ), c_light_gray, _( "The difficulty of the first wave." ) );
    mvwprintz( w, point( 2, 8 ), c_white, _( "Wave Difficulty:" ) );
    mvwprintz( w, point( NUMALIGN( wave_difficulty ), 8 ), SELCOL( 4 ), "%d", wave_difficulty );
    mvwprintz( w, point( 28, 8 ), c_light_gray, _( "The increase of difficulty with each wave." ) );

    mvwprintz( w, point( 2, 10 ), c_white, _( "Time b/w Waves:" ) );
    mvwprintz( w, point( NUMALIGN( to_minutes<int>( time_between_waves ) ), 10 ), SELCOL( 5 ),
               "%d", to_minutes<int>( time_between_waves ) );
    mvwprintz( w, point( 28, 10 ), c_light_gray, _( "The time, in minutes, between waves." ) );
    mvwprintz( w, point( 2, 11 ), c_white, _( "Waves b/w Caravans:" ) );
    mvwprintz( w, point( NUMALIGN( waves_between_caravans ), 11 ), SELCOL( 6 ), "%d",
               waves_between_caravans );
    mvwprintz( w, point( 28, 11 ), c_light_gray, _( "The number of waves in between caravans." ) );

    mvwprintz( w, point( 2, 13 ), c_white, _( "Initial Cash:" ) );
    mvwprintz( w, point( NUMALIGN( initial_cash ), 13 ), SELCOL( 7 ), "%d", initial_cash / 100 );
    mvwprintz( w, point( 28, 13 ), c_light_gray, _( "The amount of money the player starts with." ) );
    mvwprintz( w, point( 2, 14 ), c_white, _( "Cash for 1st Wave:" ) );
    mvwprintz( w, point( NUMALIGN( cash_per_wave ), 14 ), SELCOL( 8 ), "%d", cash_per_wave / 100 );
    mvwprintz( w, point( 28, 14 ), c_light_gray, _( "The cash awarded for the first wave." ) );
    mvwprintz( w, point( 2, 15 ), c_white, _( "Cash Increase:" ) );
    mvwprintz( w, point( NUMALIGN( cash_increase ), 15 ), SELCOL( 9 ), "%d", cash_increase / 100 );
    mvwprintz( w, point( 28, 15 ), c_light_gray, _( "The increase in the award each wave." ) );

    mvwprintz( w, point( 2, 17 ), c_white, _( "Enemy Selection:" ) );
    mvwprintz( w, point( 2, 18 ), TOGCOL( 10, zombies ), _( "Zombies" ) );
    mvwprintz( w, point( 14, 18 ), TOGCOL( 11, specials ), _( "Special Zombies" ) );
    mvwprintz( w, point( 34, 18 ), TOGCOL( 12, spiders ), _( "Spiders" ) );
    mvwprintz( w, point( 46, 18 ), TOGCOL( 13, triffids ), _( "Triffids" ) );
    mvwprintz( w, point( 59, 18 ), TOGCOL( 14, robots ), _( "Robots" ) );
    mvwprintz( w, point( 70, 18 ), TOGCOL( 15, subspace ), _( "Subspace" ) );

    mvwprintz( w, point( 2, 20 ), c_white, _( "Needs:" ) );
    mvwprintz( w, point( 2, 21 ), TOGCOL( 16, hunger ), _( "Food" ) );
    mvwprintz( w, point( 14, 21 ), TOGCOL( 17, thirst ), _( "Water" ) );
    mvwprintz( w, point( 34, 21 ), TOGCOL( 18, sleep ), _( "Sleep" ) );
    mvwprintz( w, point( 46, 21 ), TOGCOL( 19, mercenaries ), _( "Mercenaries" ) );
    mvwprintz( w, point( 59, 21 ), TOGCOL( 20, allow_save ), _( "Allow save" ) );
    wrefresh( w );
}

std::string defense_style_name( defense_style style )
{
    // 24 Characters Max!
    switch( style ) {
        case DEFENSE_CUSTOM:
            return _( "Custom" );
        case DEFENSE_EASY:
            return _( "Easy" );
        case DEFENSE_MEDIUM:
            return _( "Medium" );
        case DEFENSE_HARD:
            return _( "Hard" );
        case DEFENSE_SHAUN:
            return _( "Shaun of the Dead" );
        case DEFENSE_DAWN:
            return _( "Dawn of the Dead" );
        case DEFENSE_SPIDERS:
            return _( "Eight-Legged Freaks" );
        case DEFENSE_TRIFFIDS:
            return _( "Day of the Triffids" );
        case DEFENSE_SKYNET:
            return _( "Skynet" );
        case DEFENSE_LOVECRAFT:
            return _( "The Call of Cthulhu" );
        case NUM_DEFENSE_STYLES:
            break;
    }
    return "Bug!  (bug in defense.cpp:defense_style_name)";
}

std::string defense_style_description( defense_style style )
{
    // 51 Characters Max!
    switch( style ) {
        case DEFENSE_CUSTOM:
            return _( "A custom game." );
        case DEFENSE_EASY:
            return _( "Easy monsters and lots of money." );
        case DEFENSE_MEDIUM:
            return _( "Harder monsters.  You have to eat." );
        case DEFENSE_HARD:
            return _( "All monsters.  You have to eat and drink." );
        case DEFENSE_SHAUN:
            return _( "Defend a bar against classic zombies.  Easy and fun." );
        case DEFENSE_DAWN:
            return _( "Classic zombies.  Slower and more realistic." );
        case DEFENSE_SPIDERS:
            return _( "Fast-paced spider-fighting fun!" );
        case DEFENSE_TRIFFIDS:
            return _( "Defend your mansion against the triffids." );
        case DEFENSE_SKYNET:
            return _( "The robots have decided that humans are the enemy!" );
        case DEFENSE_LOVECRAFT:
            return _( "Ward off legions of eldritch horrors." );
        case NUM_DEFENSE_STYLES:
            break;
    }
    return "What the heck is this I don't even know.  (defense.cpp:defense_style_description)";
}

std::string defense_location_name( defense_location location )
{
    switch( location ) {
        case DEFLOC_NULL:
            return "Nowhere?!  (bug in defense.cpp:defense_location_name)";
        case DEFLOC_HOSPITAL:
            return _( "Hospital" );
        case DEFLOC_WORKS:
            return _( "Public Works" );
        case DEFLOC_MALL:
            return _( "Megastore" );
        case DEFLOC_BAR:
            return _( "Bar" );
        case DEFLOC_MANSION:
            return _( "Mansion" );
        case NUM_DEFENSE_LOCATIONS:
            break;
    }
    return "a ghost's house (bug in defense.cpp:defense_location_name)";
}

std::string defense_location_description( defense_location location )
{
    switch( location ) {
        case DEFLOC_NULL:
            return "NULL Bug.  (defense.cpp:defense_location_description)";
        case DEFLOC_HOSPITAL:
            return                 _( "One entrance and many rooms.  Some medical supplies." );
        case DEFLOC_WORKS:
            return                 _( "Easily fortifiable building.  Lots of useful tools." );
        case DEFLOC_MALL:
            return                 _( "A large building with various supplies." );
        case DEFLOC_BAR:
            return                 _( "A small building with plenty of alcohol." );
        case DEFLOC_MANSION:
            return                 _( "A large house with many rooms." );
        case NUM_DEFENSE_LOCATIONS:
            break;
    }
    return "Unknown data bug.  (defense.cpp:defense_location_description)";
}

void defense_game::caravan()
{
    std::vector<itype_id> items[NUM_CARAVAN_CATEGORIES];
    std::vector<int> item_count[NUM_CARAVAN_CATEGORIES];

    // Init the items for each category
    for( int i = 0; i < NUM_CARAVAN_CATEGORIES; i++ ) {
        items[i] = caravan_items( static_cast<caravan_category>( i ) );
        for( std::vector<itype_id>::iterator it = items[i].begin();
             it != items[i].end(); ) {
            if( current_wave == 0 || !one_in( 4 ) ) {
                item_count[i].push_back( 0 );  // Init counts to 0 for each item
                it++;
            } else { // Remove the item
                it = items[i].erase( it );
            }
        }
    }

    signed total_price = 0;

    background_pane bg_pane;

    catacurses::window w;
    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        const int width = FULL_SCREEN_WIDTH;
        const int height = FULL_SCREEN_HEIGHT;
        const int offsetx = std::max( 0, TERMX - FULL_SCREEN_WIDTH ) / 2;
        const int offsety = std::max( 0, TERMY - FULL_SCREEN_HEIGHT ) / 2;
        w = catacurses::newwin( height, width, point( offsetx, offsety ) );
        ui.position_from_window( w );
    } );
    ui.mark_resize();

    int offset = 0;
    int item_selected = 0;
    int category_selected = 0;

    int current_window = 0;

    ui.on_redraw( [&]( const ui_adaptor & ) {
        draw_caravan_categories( w, category_selected, total_price, g->u.cash );
        draw_caravan_items( w, &( items[category_selected] ),
                            &( item_count[category_selected] ), offset, item_selected );
        draw_caravan_borders( w, current_window );
    } );

    input_context ctxt( "CARAVAN" );
    ctxt.register_cardinal();
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "HELP" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    bool done = false;
    bool cancel = false;
    while( !done ) {
        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        if( action == "HELP" ) {
            popup_top( _( "CARAVAN:\n"
                          "Start by selecting a category using your favorite up/down keys.\n"
                          "Switch between category selection and item selecting by pressing %s.\n"
                          "Pick an item with the up/down keys, press left/right to buy 1 less/more.\n"
                          "Press %s to buy everything in your cart, %s to buy nothing." ),
                       ctxt.get_desc( "NEXT_TAB" ),
                       ctxt.get_desc( "CONFIRM" ),
                       ctxt.get_desc( "QUIT" )
                     );
        } else if( action == "DOWN" ) {
            if( current_window == 0 ) { // Categories
                category_selected++;
                if( category_selected == NUM_CARAVAN_CATEGORIES ) {
                    category_selected = CARAVAN_CART;
                }
                offset = 0;
                item_selected = 0;
            } else if( !items[category_selected].empty() ) { // Items
                if( item_selected < static_cast<int>( items[category_selected].size() ) - 1 ) {
                    item_selected++;
                } else {
                    item_selected = 0;
                    offset = 0;
                }
                if( item_selected > offset + 12 ) {
                    offset++;
                }
            }
        } else if( action == "UP" ) {
            if( current_window == 0 ) { // Categories
                if( category_selected == 0 ) {
                    category_selected = NUM_CARAVAN_CATEGORIES - 1;
                } else {
                    category_selected--;
                }
                if( category_selected == NUM_CARAVAN_CATEGORIES ) {
                    category_selected = CARAVAN_CART;
                }
                offset = 0;
                item_selected = 0;
            } else if( !items[category_selected].empty() ) { // Items
                if( item_selected > 0 ) {
                    item_selected--;
                } else {
                    item_selected = items[category_selected].size() - 1;
                    offset = item_selected - 12;
                    if( offset < 0 ) {
                        offset = 0;
                    }
                }
                if( item_selected < offset ) {
                    offset--;
                }
            }
        } else if( action == "RIGHT" ) {
            if( current_window == 1 && !items[category_selected].empty() ) {
                item_count[category_selected][item_selected]++;
                itype_id tmp_itm = items[category_selected][item_selected];
                total_price += caravan_price( g->u, item( tmp_itm, 0 ).price( false ) );
                if( category_selected == CARAVAN_CART ) { // Find the item in its category
                    for( int i = 1; i < NUM_CARAVAN_CATEGORIES; i++ ) {
                        for( size_t j = 0; j < items[i].size(); j++ ) {
                            if( items[i][j] == tmp_itm ) {
                                item_count[i][j]++;
                            }
                        }
                    }
                } else { // Add / increase the item in the shopping cart
                    bool found_item = false;
                    for( unsigned i = 0; i < items[0].size() && !found_item; i++ ) {
                        if( items[0][i] == tmp_itm ) {
                            found_item = true;
                            item_count[0][i]++;
                        }
                    }
                    if( !found_item ) {
                        items[0].push_back( items[category_selected][item_selected] );
                        item_count[0].push_back( 1 );
                    }
                }
            }
        } else if( action == "LEFT" ) {
            if( current_window == 1 && !items[category_selected].empty() &&
                item_count[category_selected][item_selected] > 0 ) {
                item_count[category_selected][item_selected]--;
                itype_id tmp_itm = items[category_selected][item_selected];
                total_price -= caravan_price( g->u, item( tmp_itm, 0 ).price( false ) );
                if( category_selected == CARAVAN_CART ) { // Find the item in its category
                    for( int i = 1; i < NUM_CARAVAN_CATEGORIES; i++ ) {
                        for( size_t j = 0; j < items[i].size(); j++ ) {
                            if( items[i][j] == tmp_itm ) {
                                item_count[i][j]--;
                            }
                        }
                    }
                } else { // Decrease / remove the item in the shopping cart
                    bool found_item = false;
                    for( unsigned i = 0; i < items[0].size() && !found_item; i++ ) {
                        if( items[0][i] == tmp_itm ) {
                            found_item = true;
                            item_count[0][i]--;
                            if( item_count[0][i] == 0 ) {
                                item_count[0].erase( item_count[0].begin() + i );
                                items[0].erase( items[0].begin() + i );
                            }
                        }
                    }
                }
            }
        } else if( action == "NEXT_TAB" ) {
            current_window = ( current_window + 1 ) % 2;
        } else if( action == "QUIT" ) {
            if( query_yn( _( "Really buy nothing?" ) ) ) {
                cancel = true;
                done = true;
            }
        } else if( action == "CONFIRM" ) {
            if( total_price > g->u.cash ) {
                popup( _( "You can't afford those items!" ) );
            } else if( ( items[0].empty() && query_yn( _( "Really buy nothing?" ) ) ) ||
                       ( !items[0].empty() &&
                         query_yn( ngettext( "Buy %d item, leaving you with %s?",
                                             "Buy %d items, leaving you with %s?",
                                             items[0].size() ),
                                   items[0].size(),
                                   format_money( static_cast<int>( g->u.cash ) - static_cast<int>( total_price ) ) ) ) ) {
                done = true;
            }
        } // "switch" on (action)

    } // while (!done)

    if( !cancel ) {
        g->u.cash -= total_price;
        bool dropped_some = false;
        for( size_t i = 0; i < items[0].size(); i++ ) {
            item tmp( items[0][i] );
            tmp = tmp.in_its_container();

            // Guns bought from the caravan should always come with an empty
            // magazine.
            if( tmp.is_gun() && !tmp.magazine_integral() ) {
                tmp.put_in( item( tmp.magazine_default() ) );
            }

            for( int j = 0; j < item_count[0][i]; j++ ) {
                if( g->u.can_pickVolume( tmp ) && g->u.can_pickWeight( tmp ) ) {
                    g->u.i_add( tmp );
                } else { // Could fit it in the inventory!
                    dropped_some = true;
                    g->m.add_item_or_charges( g->u.pos(), tmp );
                }
            }
        }
        if( dropped_some ) {
            add_msg( _( "You drop some items." ) );
        }
    }
}

std::string caravan_category_name( caravan_category cat )
{
    switch( cat ) {
        case CARAVAN_CART:
            return _( "Shopping Cart" );
        case CARAVAN_MELEE:
            return _( "Melee Weapons" );
        case CARAVAN_RANGED:
            return _( "Ranged Weapons" );
        case CARAVAN_AMMUNITION:
            return _( "Ammuniton" );
        case CARAVAN_COMPONENTS:
            return _( "Crafting & Construction Components" );
        case CARAVAN_FOOD:
            return _( "Food & Drugs" );
        case CARAVAN_CLOTHES:
            return _( "Clothing & Armor" );
        case CARAVAN_TOOLS:
            return _( "Tools, Traps & Grenades" );
        case NUM_CARAVAN_CATEGORIES:
            break; // error message below
    }
    return "BUG (defense.cpp:caravan_category_name)";
}

std::vector<itype_id> caravan_items( caravan_category cat )
{
    std::vector<itype_id> ret;
    std::string group_id;
    switch( cat ) {
        case CARAVAN_CART:
            return ret;

        case CARAVAN_MELEE:
            group_id = "defense_caravan_melee" ;
            break;

        case CARAVAN_RANGED:
            group_id = "defense_caravan_ranged" ;
            break;

        case CARAVAN_AMMUNITION:
            group_id = "defense_caravan_ammunition" ;
            break;

        case CARAVAN_COMPONENTS:
            group_id = "defense_caravan_components" ;
            break;

        case CARAVAN_FOOD:
            group_id = "defense_caravan_food" ;
            break;

        case CARAVAN_CLOTHES:
            group_id = "defense_caravan_clothes" ;
            break;

        case CARAVAN_TOOLS:
            group_id = "defense_caravan_tools" ;
            break;

        case NUM_CARAVAN_CATEGORIES:
            DebugLog( D_ERROR, D_GAME ) << "invalid caravan category: " << cat;
            return ret;
    }

    item_group::ItemList item_list = item_group::items_from( item_group_id( group_id ) );

    for( auto &it : item_list ) {
        itype_id item_type = it.typeId();
        ret.emplace_back( item_type );
        // Add the default magazine types for each gun.
        if( it.is_gun() && !it.magazine_integral() ) {
            ret.emplace_back( it.magazine_default() );
        }
    }
    return ret;
}

void draw_caravan_borders( const catacurses::window &w, int current_window )
{
    // First, do the borders for the category window
    nc_color col = c_light_gray;
    if( current_window == 0 ) {
        col = c_yellow;
    }

    mvwputch( w, point_zero, col, LINE_OXXO );
    for( int i = 1; i <= 38; i++ ) {
        mvwputch( w, point( i, 0 ), col, LINE_OXOX );
        mvwputch( w, point( i, 11 ), col, LINE_OXOX );
    }
    for( int i = 1; i <= 10; i++ ) {
        mvwputch( w, point( 0, i ), col, LINE_XOXO );
        mvwputch( w, point( 39, i ), c_yellow, LINE_XOXO ); // Shared border, always yellow
    }
    mvwputch( w, point( 0, 11 ), col, LINE_XXXO );

    // These are shared with the items window, and so are always "on"
    mvwputch( w, point( 39, 0 ), c_yellow, LINE_OXXX );
    mvwputch( w, point( 39, 11 ), c_yellow, LINE_XOXX );

    col = ( current_window == 1 ? c_yellow : c_light_gray );
    // Next, draw the borders for the item description window--always "off" & gray
    for( int i = 12; i <= 23; i++ ) {
        mvwputch( w, point( 0, i ), c_light_gray, LINE_XOXO );
        mvwputch( w, point( 39, i ), col,      LINE_XOXO );
    }
    for( int i = 1; i <= 38; i++ ) {
        mvwputch( w, point( i, FULL_SCREEN_HEIGHT - 1 ), c_light_gray, LINE_OXOX );
    }

    mvwputch( w, point( 0, FULL_SCREEN_HEIGHT - 1 ), c_light_gray, LINE_XXOO );
    mvwputch( w, point( 39, FULL_SCREEN_HEIGHT - 1 ), c_light_gray, LINE_XXOX );

    // Finally, draw the item section borders
    for( int i = 40; i <= FULL_SCREEN_WIDTH - 2; i++ ) {
        mvwputch( w, point( i, 0 ), col, LINE_OXOX );
        mvwputch( w, point( i, FULL_SCREEN_HEIGHT - 1 ), col, LINE_OXOX );
    }
    for( int i = 1; i <= FULL_SCREEN_HEIGHT - 2; i++ ) {
        mvwputch( w, point( FULL_SCREEN_WIDTH - 1, i ), col, LINE_XOXO );
    }

    mvwputch( w, point( 39, FULL_SCREEN_HEIGHT - 1 ), col, LINE_XXOX );
    mvwputch( w, point( FULL_SCREEN_WIDTH - 1, 0 ), col, LINE_OOXX );
    mvwputch( w, point( FULL_SCREEN_WIDTH - 1, FULL_SCREEN_HEIGHT - 1 ), col, LINE_XOOX );

    // Quick reminded about help.
    // NOLINTNEXTLINE(cata-text-style): literal question mark
    mvwprintz( w, point( 2, FULL_SCREEN_HEIGHT - 1 ), c_red, _( "Press ? for help." ) );
    wrefresh( w );
}

void draw_caravan_categories( const catacurses::window &w, int category_selected,
                              int total_price, int cash )
{
    // Clear the window
    for( int i = 1; i <= 10; i++ ) {
        mvwprintz( w, point( 1, i ), c_black, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" );
    }
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 1 ), c_white, _( "Your Cash: %s" ), format_money( cash ) );
    wprintz( w, c_light_gray, " -> " );
    wprintz( w, ( total_price > cash ? c_red : c_green ), "%s",
             format_money( static_cast<int>( cash ) - static_cast<int>( total_price ) ) );

    for( int i = 0; i < NUM_CARAVAN_CATEGORIES; i++ ) {
        mvwprintz( w, point( 1, i + 3 ), ( i == category_selected ? h_white : c_white ),
                   caravan_category_name( static_cast<caravan_category>( i ) ) );
    }
    wrefresh( w );
}

void draw_caravan_items( const catacurses::window &w, std::vector<itype_id> *items,
                         std::vector<int> *counts, int offset, int item_selected )
{
    // Print the item info first.  This is important, because it contains \n which
    // will corrupt the item list.

    // Actually, clear the item info first.
    for( int i = 12; i <= FULL_SCREEN_HEIGHT - 2; i++ ) {
        mvwprintz( w, point( 1, i ), c_black, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" );
    }
    // THEN print it--if item_selected is valid
    if( item_selected < static_cast<int>( items->size() ) ) {
        item tmp( ( *items )[item_selected], 0 ); // Dummy item to get info
        fold_and_print( w, point( 1, 12 ), 38, c_white, tmp.info() );
    }
    // Next, clear the item list on the right
    for( int i = 1; i <= FULL_SCREEN_HEIGHT - 2; i++ ) {
        mvwprintz( w, point( 40, i ), c_black, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" );
    }
    // Finally, print the item list on the right
    for( int i = offset; i <= offset + FULL_SCREEN_HEIGHT - 2 &&
         i < static_cast<int>( items->size() ); i++ ) {
        mvwprintz( w, point( 40, i - offset + 1 ), ( item_selected == i ? h_white : c_white ),
                   item::nname( ( *items )[i], ( *counts )[i] ) );
        wprintz( w, c_white, " x %2d", ( *counts )[i] );
        if( ( *counts )[i] > 0 ) {
            int price = caravan_price( g->u, item( ( *items )[i],
                                                   0 ).price( false ) * ( *counts )[i] );
            wprintz( w, ( price > g->u.cash ? c_red : c_green ), " (%s)", format_money( price ) );
        }
    }
    wrefresh( w );
}

int caravan_price( player &u, int price )
{
    ///\EFFECT_BARTER reduces caravan prices, 5% per point, up to 50%
    if( u.get_skill_level( skill_barter ) > 10 ) {
        return static_cast<int>( static_cast<double>( price ) * .5 );
    }
    return price * ( 1.0 - u.get_skill_level( skill_barter ) * .05 );
}

void defense_game::spawn_wave()
{
    add_msg( m_info, "********" );
    int diff = initial_difficulty + current_wave * wave_difficulty;
    bool themed_wave = one_in( SPECIAL_WAVE_CHANCE ); // All a single monster type
    g->u.cash += cash_per_wave + ( current_wave - 1 ) * cash_increase;
    std::vector<mtype_id> valid = pick_monster_wave();
    while( diff > 0 ) {
        // Clear out any monsters that exceed our remaining difficulty
        for( auto it = valid.begin(); it != valid.end(); ) {
            const mtype &mt = it->obj();
            if( mt.difficulty > diff ) {
                it = valid.erase( it );
            } else {
                it++;
            }
        }
        if( valid.empty() ) {
            add_msg( m_info, _( "Welcome to Wave %d!" ), current_wave );
            add_msg( m_info, "********" );
            return;
        }
        const mtype &type = random_entry( valid ).obj();
        if( themed_wave ) {
            int num = diff / type.difficulty;
            if( num >= SPECIAL_WAVE_MIN ) {
                // TODO: Do we want a special message here?
                for( int i = 0; i < num; i++ ) {
                    spawn_wave_monster( type.id );
                }
                add_msg( m_info,  special_wave_message( type.nname( 100 ) ) );
                add_msg( m_info, "********" );
                return;
            } else {
                themed_wave = false;    // No partially-themed waves
            }
        }
        diff -= type.difficulty;
        spawn_wave_monster( type.id );
    }
    add_msg( m_info, _( "Welcome to Wave %d!" ), current_wave );
    add_msg( m_info, "********" );
}

std::vector<mtype_id> defense_game::pick_monster_wave()
{
    std::vector<mongroup_id> valid;
    std::vector<mtype_id> ret;

    if( zombies || specials ) {
        if( specials ) {
            valid.push_back( GROUP_ZOMBIE );
        } else {
            valid.push_back( GROUP_VANILLA );
        }
    }
    if( spiders ) {
        valid.push_back( GROUP_SPIDER );
    }
    if( triffids ) {
        valid.push_back( GROUP_TRIFFID );
    }
    if( robots ) {
        valid.push_back( GROUP_ROBOT );
    }
    if( subspace ) {
        valid.push_back( GROUP_NETHER );
    }

    if( valid.empty() ) {
        debugmsg( "Couldn't find a valid monster group for defense!" );
    } else {
        ret = MonsterGroupManager::GetMonstersFromGroup( random_entry( valid ) );
    }

    return ret;
}

void defense_game::spawn_wave_monster( const mtype_id &type )
{
    for( int tries = 0; tries < 1000; tries++ ) {
        point pnt;
        if( location == DEFLOC_HOSPITAL || location == DEFLOC_MALL ) {
            // Always spawn to the north!
            pnt = point( rng( HALF_MAPSIZE_X, HALF_MAPSIZE_X + SEEX ), SEEY );
        } else if( one_in( 2 ) ) {
            pnt = point( rng( HALF_MAPSIZE_X, HALF_MAPSIZE_X + SEEX ), rng( 1, SEEY ) );
            if( one_in( 2 ) ) {
                pnt = point( pnt.x, -pnt.y ) + point( 0, MAPSIZE_Y - 1 );
            }
        } else {
            pnt = point( rng( 1, SEEX ), rng( HALF_MAPSIZE_Y, HALF_MAPSIZE_Y + SEEY ) );
            if( one_in( 2 ) ) {
                pnt = point( -pnt.x, pnt.y ) + point( MAPSIZE_X - 1, 0 );
            }
        }
        monster *const mon = g->place_critter_at( type, tripoint( pnt, g->get_levz() ) );
        if( !mon ) {
            continue;
        }
        monster &tmp = *mon;
        tmp.wander_pos = g->u.pos();
        tmp.wandf = 150;
        // We want to kill!
        tmp.anger = 100;
        tmp.morale = 100;
        return;
    }
}

std::string defense_game::special_wave_message( std::string name )
{
    std::string ret;
    ret += string_format( _( "Wave %d: " ), current_wave );

    // Capitalize
    capitalize_letter( name );
    for( size_t i = 2; i < name.size(); i++ ) {
        if( name[i - 1] == ' ' ) {
            capitalize_letter( name, i );
        }
    }

    switch( rng( 1, 8 ) ) {
        case 1:
            ret += string_format( _( "Invasion of the %s!" ), name );
            break;
        case 2:
            ret += string_format( _( "Attack of the %s!" ), name );
            break;
        case 3:
            ret += string_format( _( "%s Attack!" ), name );
            break;
        case 4:
            ret += string_format( _( "%s from Hell!" ), name );
            break;
        case 5:
            ret += string_format( _( "Beware!  %s!" ), name );
            break;
        case 6:
            ret += string_format( _( "The Day of the %s!" ), name );
            break;
        case 7:
            ret += string_format( _( "Revenge of the %s!" ), name );
            break;
        case 8:
            ret += string_format( _( "Rise of the %s!" ), name );
            break;
    }

    return ret;
}
