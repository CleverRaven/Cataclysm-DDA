#include "game.h"
#include "gamemode.h"
#include "gates.h"
#include "action.h"
#include "input.h"
#include "output.h"
#include "player.h"
#include "messages.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_reference.h"
#include "map.h"
#include "options.h"
#include "mapsharing.h"
#include "safemode_ui.h"
#include "pickup.h"
#include "game_inventory.h"
#include "ranged.h"
#include "debug.h"
#include "worldfactory.h"
#include "faction.h"
#include "itype.h"
#include "auto_pickup.h"
#include "gun_mode.h"
#include "construction.h"
#include "bionics.h"
#include "mutation.h"
#include "monster.h"
#include "help.h"
#include "calendar.h"
#include "weather.h"
#include "sounds.h"
#include "veh_type.h"
#include "mapdata.h"

#include <chrono>

#define dbg(x) DebugLog((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

void advanced_inv();

const efftype_id effect_alarm_clock( "alarm_clock" );
const efftype_id effect_laserlocked( "laserlocked" );

static const bionic_id bio_remote( "bio_remote" );

static const trait_id trait_HIBERNATE( "HIBERNATE" );
static const trait_id trait_SHELL2( "SHELL2" );

const skill_id skill_driving( "driving" );

class user_turn
{

    private:
        std::chrono::time_point<std::chrono::steady_clock> user_turn_start;
    public:
        user_turn() {
            user_turn_start = std::chrono::steady_clock::now();
        }

        bool has_timeout_elapsed() {
            float turn_duration = get_option<float>( "TURN_DURATION" );
            // Magic number 0.005 chosen due to option menu's 2 digit precision and
            // the option menu UI rounding <= 0.005 down to "0.00" in the display.
            // This conditional will catch values (e.g. 0.003) that the options menu
            // would round down to "0.00" in the options menu display. This prevents
            // the user from being surprised by floating point rounding near zero.
            if( turn_duration <= 0.005 ) {
                return false;
            }

            auto now = std::chrono::steady_clock::now();
            std::chrono::milliseconds elapsed_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>( now - user_turn_start );
            return elapsed_ms.count() >= 1000.0 * turn_duration;
        }
};

input_context game::get_player_input( std::string &action )
{
    input_context ctxt = get_default_mode_input_context();
    // register QUIT action so it catches q/Q/etc instead of just Q
    if( uquit == QUIT_WATCH ) {
        ctxt.register_action( "QUIT" );
    }

    m.update_visibility_cache( u.posz() );
    const visibility_variables &cache = g->m.get_visibility_variables_cache();
    const level_cache &map_cache = m.get_cache_ref( u.posz() );
    const auto &visibility_cache = map_cache.visibility_cache;

    user_turn current_turn;

    if( get_option<bool>( "ANIMATIONS" ) ) {
        int iStartX = ( TERRAIN_WINDOW_WIDTH > 121 ) ? ( TERRAIN_WINDOW_WIDTH - 121 ) / 2 : 0;
        int iStartY = ( TERRAIN_WINDOW_HEIGHT > 121 ) ? ( TERRAIN_WINDOW_HEIGHT - 121 ) / 2 : 0;
        int iEndX = ( TERRAIN_WINDOW_WIDTH > 121 ) ? TERRAIN_WINDOW_WIDTH - ( TERRAIN_WINDOW_WIDTH - 121 ) /
                    2 :
                    TERRAIN_WINDOW_WIDTH;
        int iEndY = ( TERRAIN_WINDOW_HEIGHT > 121 ) ? TERRAIN_WINDOW_HEIGHT -
                    ( TERRAIN_WINDOW_HEIGHT - 121 ) /
                    2 : TERRAIN_WINDOW_HEIGHT;

        if( fullscreen ) {
            iStartX = 0;
            iStartY = 0;
            iEndX = TERMX;
            iEndY = TERMY;
        }

        //x% of the Viewport, only shown on visible areas
        auto const weather_info = get_weather_animation( weather );
        int offset_x = ( u.posx() + u.view_offset.x ) - getmaxx( w_terrain ) / 2;
        int offset_y = ( u.posy() + u.view_offset.y ) - getmaxy( w_terrain ) / 2;

#ifdef TILES
        if( tile_iso && use_tiles ) {
            iStartX = 0;
            iStartY = 0;
            iEndX = MAPSIZE * SEEX;
            iEndY = MAPSIZE * SEEY;
            offset_x = 0;
            offset_y = 0;
        }
#endif //TILES

        // TODO: Move the weather calculations out of here.
        const bool bWeatherEffect = ( weather_info.glyph != '?' );
        const int dropCount = int( iEndX * iEndY * weather_info.factor );

        weather_printable wPrint;
        wPrint.colGlyph = weather_info.color;
        wPrint.cGlyph = weather_info.glyph;
        wPrint.wtype = weather;
        wPrint.vdrops.clear();
        wPrint.startx = iStartX;
        wPrint.starty = iStartY;
        wPrint.endx = iEndX;
        wPrint.endy = iEndY;

        inp_mngr.set_timeout( 125 );
        // Force at least one animation frame if the player is dead.
        while( handle_mouseview( ctxt, action ) || uquit == QUIT_WATCH ) {
            if( action == "TIMEOUT" && current_turn.has_timeout_elapsed() ) {
                break;
            }

            if( bWeatherEffect && get_option<bool>( "ANIMATION_RAIN" ) ) {
                /*
                Location to add rain drop animation bits! Since it refreshes w_terrain it can be added to the animation section easily
                Get tile information from above's weather information:
                WEATHER_ACID_DRIZZLE | WEATHER_ACID_RAIN = "weather_acid_drop"
                WEATHER_DRIZZLE | WEATHER_RAINY | WEATHER_THUNDER | WEATHER_LIGHTNING = "weather_rain_drop"
                WEATHER_FLURRIES | WEATHER_SNOW | WEATHER_SNOWSTORM = "weather_snowflake"
                */

#ifdef TILES
                if( !use_tiles ) {
#endif //TILES
                    //If not using tiles, erase previous drops from w_terrain
                    for( auto &elem : wPrint.vdrops ) {
                        const tripoint location( elem.first + offset_x, elem.second + offset_y, get_levz() );
                        const lit_level lighting = visibility_cache[location.x][location.y];
                        wmove( w_terrain, location.y - offset_y, location.x - offset_x );
                        if( !m.apply_vision_effects( w_terrain, lighting, cache ) ) {
                            m.drawsq( w_terrain, u, location, false, true,
                                      u.pos() + u.view_offset,
                                      lighting == LL_LOW, lighting == LL_BRIGHT );
                        }
                    }
#ifdef TILES
                }
#endif //TILES
                wPrint.vdrops.clear();

                for( int i = 0; i < dropCount; i++ ) {
                    const int iRandX = rng( iStartX, iEndX - 1 );
                    const int iRandY = rng( iStartY, iEndY - 1 );
                    const int mapx = iRandX + offset_x;
                    const int mapy = iRandY + offset_y;

                    const tripoint mapp( mapx, mapy, u.posz() );

                    const lit_level lighting = visibility_cache[mapp.x][mapp.y];

                    if( m.is_outside( mapp ) && m.get_visibility( lighting, cache ) == VIS_CLEAR &&
                        !critter_at( mapp, true ) ) {
                        // Suppress if a critter is there
                        wPrint.vdrops.emplace_back( std::make_pair( iRandX, iRandY ) );
                    }
                }
            }
            // don't bother calculating SCT if we won't show it
            if( uquit != QUIT_WATCH && get_option<bool>( "ANIMATION_SCT" ) ) {
#ifdef TILES
                if( !use_tiles ) {
#endif
                    for( auto &elem : SCT.vSCT ) {
                        //Erase previous text from w_terrain
                        if( elem.getStep() > 0 ) {
                            for( size_t i = 0; i < elem.getText().length(); ++i ) {
                                const tripoint location( elem.getPosX() + i, elem.getPosY(), get_levz() );
                                const lit_level lighting = visibility_cache[location.x][location.y];
                                wmove( w_terrain, location.y - offset_y, location.x - offset_x );
                                if( !m.apply_vision_effects( w_terrain, lighting, cache ) ) {
                                    m.drawsq( w_terrain, u, location, false, true,
                                              u.pos() + u.view_offset,
                                              lighting == LL_LOW, lighting == LL_BRIGHT );
                                }
                            }
                        }
                    }
#ifdef TILES
                }
#endif

                SCT.advanceAllSteps();

                //Check for creatures on all drawing positions and offset if necessary
                for( auto iter = SCT.vSCT.rbegin(); iter != SCT.vSCT.rend(); ++iter ) {
                    const direction oCurDir = iter->getDirecton();

                    for( int i = 0; i < static_cast<int>( iter->getText().length() ); ++i ) {
                        tripoint tmp( iter->getPosX() + i, iter->getPosY(), get_levz() );
                        const Creature *critter = critter_at( tmp, true );

                        if( critter != nullptr && u.sees( *critter ) ) {
                            i = -1;
                            int iPos = iter->getStep() + iter->getStepOffset();
                            for( auto iter2 = iter; iter2 != SCT.vSCT.rend(); ++iter2 ) {
                                if( iter2->getDirecton() == oCurDir &&
                                    iter2->getStep() + iter2->getStepOffset() <= iPos ) {
                                    if( iter2->getType() == "hp" ) {
                                        iter2->advanceStepOffset();
                                    }

                                    iter2->advanceStepOffset();
                                    iPos = iter2->getStep() + iter2->getStepOffset();
                                }
                            }
                        }
                    }
                }
            }

            werase( w_terrain );

            draw_ter();
            draw_weather( wPrint );

            if( uquit != QUIT_WATCH ) {
                draw_sct();
            }

            wrefresh( w_terrain );

            if( uquit == QUIT_WATCH ) {
                draw_sidebar();

                catacurses::window popup = create_wait_popup_window(
                                               string_format( _( "Press %s to accept your fate..." ),
                                                       ctxt.get_desc( "QUIT" ).c_str() ),
                                               c_red
                                           );

                wrefresh( popup );

                break;
            }

            //updating the pixel minimap here allows red flashing indicators for enemies to actually flicker
            draw_pixel_minimap();
        }
        inp_mngr.reset_timeout();
    } else {
        inp_mngr.set_timeout( 125 );
        while( handle_mouseview( ctxt, action ) ) {
            if( action == "TIMEOUT" && current_turn.has_timeout_elapsed() ) {
                break;
            }
        }
        inp_mngr.reset_timeout();
    }

    return ctxt;
}

static void rcdrive( int dx, int dy )
{
    player &u = g->u;
    map &m = g->m;
    std::stringstream car_location_string( u.get_value( "remote_controlling" ) );

    if( car_location_string.str().empty() ) {
        //no turned radio car found
        u.add_msg_if_player( m_warning, _( "No radio car connected." ) );
        return;
    }
    int cx = 0;
    int cy = 0;
    int cz = 0;
    car_location_string >> cx >> cy >> cz;

    auto rc_pairs = m.get_rc_items( cx, cy, cz );
    auto rc_pair = rc_pairs.begin();
    for( ; rc_pair != rc_pairs.end(); ++rc_pair ) {
        if( rc_pair->second->typeId() == "radio_car_on" && rc_pair->second->active ) {
            break;
        }
    }
    if( rc_pair == rc_pairs.end() ) {
        u.add_msg_if_player( m_warning, _( "No radio car connected." ) );
        u.remove_value( "remote_controlling" );
        return;
    }
    item *rc_car = rc_pair->second;

    if( tile_iso && use_tiles ) {
        rotate_direction_cw( dx, dy );
    }

    tripoint src( cx, cy, cz );
    tripoint dest( cx + dx, cy + dy, cz );
    if( m.impassable( dest ) || !m.can_put_items_ter_furn( dest ) ||
        m.has_furn( dest ) ) {
        sounds::sound( dest, 7, _( "sound of a collision with an obstacle." ) );
        return;
    } else if( !m.add_item_or_charges( dest, *rc_car ).is_null() ) {
        //~ Sound of moving a remote controlled car
        sounds::sound( src, 6, _( "zzz..." ) );
        u.moves -= 50;
        m.i_rem( src, rc_car );
        car_location_string.clear();
        car_location_string << dest.x << ' ' << dest.y << ' ' << dest.z;
        u.set_value( "remote_controlling", car_location_string.str() );
        return;
    }
}

static void pldrive( int x, int y )
{
    if( !g->check_safe_mode_allowed() ) {
        return;
    }
    player &u = g->u;
    vehicle *veh = g->remoteveh();
    bool remote = true;
    int part = -1;
    if( !veh ) {
        if( const optional_vpart_position vp = g->m.veh_at( u.pos() ) ) {
            veh = &vp->vehicle();
            part = vp->part_index();
        }
        remote = false;
    }
    if( !veh ) {
        dbg( D_ERROR ) << "game::pldrive: can't find vehicle! Drive mode is now off.";
        debugmsg( "game::pldrive error: can't find vehicle! Drive mode is now off." );
        u.in_vehicle = false;
        return;
    }
    if( !remote ) {
        int pctr = veh->part_with_feature( part, "CONTROLS" );
        if( pctr < 0 ) {
            add_msg( m_info, _( "You can't drive the vehicle from here. You need controls!" ) );
            u.controlling_vehicle = false;
            return;
        }
    } else {
        if( veh->all_parts_with_feature( "REMOTE_CONTROLS", true ).empty() ) {
            add_msg( m_info, _( "Can't drive this vehicle remotely. It has no working controls." ) );
            return;
        }
    }

    int turn_delta = 15 * x;
    const float handling_diff = veh->handling_difficulty();
    if( turn_delta != 0 ) {
        float eff = veh->steering_effectiveness();
        if( eff < 0 ) {
            add_msg( m_info, _( "This vehicle has no steering system installed, you can't turn it." ) );
            return;
        }

        if( eff == 0 ) {
            add_msg( m_bad, _( "The steering is completely broken!" ) );
            return;
        }

        // If you've got more moves than speed, it's most likely time stop
        // Let's get rid of that
        u.moves = std::min( u.moves, u.get_speed() );

        ///\EFFECT_DEX reduces chance of losing control of vehicle when turning

        ///\EFFECT_PER reduces chance of losing control of vehicle when turning

        ///\EFFECT_DRIVING reduces chance of losing control of vehicle when turning
        float skill = std::min( 10.0f,
                                u.get_skill_level( skill_driving ) + ( u.get_dex() + u.get_per() ) / 10.0f );
        float penalty = rng_float( 0.0f, handling_diff ) - skill;
        int cost;
        if( penalty > 0.0f ) {
            // At 10 penalty (rather hard to get), we're taking 4 turns per turn
            cost = 100 * ( 1.0f + penalty / 2.5f );
        } else {
            // At 10 skill, with a perfect vehicle, we could turn up to 3 times per turn
            cost = std::max( u.get_speed(), 100 ) * ( 1.0f - ( -penalty / 10.0f ) * 2 / 3 );
        }

        if( penalty > skill || cost > 400 ) {
            add_msg( m_warning, _( "You fumble with the %s's controls." ), veh->name.c_str() );
            // Anything from a wasted attempt to 2 turns in the intended direction
            turn_delta *= rng( 0, 2 );
            // Also wastes next turn
            cost = std::max( cost, u.moves + 100 );
        } else if( one_in( 10 ) ) {
            // Don't warn all the time or it gets spammy
            if( cost >= u.get_speed() * 2 ) {
                add_msg( m_warning, _( "It takes you a very long time to steer that vehicle!" ) );
            } else if( cost >= u.get_speed() * 1.5f ) {
                add_msg( m_warning, _( "It takes you a long time to steer that vehicle!" ) );
            }
        }

        veh->turn( turn_delta );

        // At most 3 turns per turn, because otherwise it looks really weird and jumpy
        u.moves -= std::max( cost, u.get_speed() / 3 + 1 );
    }

    if( y != 0 ) {
        int thr_amount = 10 * 100;
        if( veh->cruise_on ) {
            veh->cruise_thrust( -y * thr_amount );
        } else {
            veh->thrust( -y );
            u.moves = std::min( u.moves, 0 );
        }
    }

    // @todo: Actually check if we're on land on water (or disable water-skidding)
    if( veh->skidding && ( veh->valid_wheel_config( false ) || veh->valid_wheel_config( true ) ) ) {
        ///\EFFECT_DEX increases chance of regaining control of a vehicle

        ///\EFFECT_DRIVING increases chance of regaining control of a vehicle
        if( handling_diff * rng( 1, 10 ) < u.dex_cur + u.get_skill_level( skill_driving ) * 2 ) {
            add_msg( _( "You regain control of the %s." ), veh->name.c_str() );
            u.practice( skill_driving, veh->velocity / 5 );
            veh->velocity = int( veh->forward_velocity() );
            veh->skidding = false;
            veh->move.init( veh->turn_dir );
        }
    }
}

static void open()
{
    player &u = g->u;
    map &m = g->m;

    tripoint openp;
    if( !choose_adjacent_highlight( _( "Open where?" ), openp, ACTION_OPEN ) ) {
        return;
    }

    u.moves -= 100;

    if( const optional_vpart_position vp = m.veh_at( openp ) ) {
        vehicle *const veh = &vp->vehicle();
        int openable = veh->next_part_to_open( vp->part_index() );
        if( openable >= 0 ) {
            const vehicle *player_veh = veh_pointer_or_null( m.veh_at( u.pos() ) );
            bool outside = !player_veh || player_veh != veh;
            if( !outside ) {
                veh->open( openable );
            } else {
                // Outside means we check if there's anything in that tile outside-openable.
                // If there is, we open everything on tile. This means opening a closed,
                // curtained door from outside is possible, but it will magically open the
                // curtains as well.
                int outside_openable = veh->next_part_to_open( vp->part_index(), true );
                if( outside_openable == -1 ) {
                    const std::string name = veh->part_info( openable ).name();
                    add_msg( m_info, _( "That %s can only opened from the inside." ), name.c_str() );
                    u.moves += 100;
                } else {
                    veh->open_all_at( openable );
                }
            }
        } else {
            // If there are any OPENABLE parts here, they must be already open
            if( const cata::optional<vpart_reference> already_open = vp.part_with_feature( "OPENABLE" ) ) {
                const std::string name = veh->part_info( already_open->part_index() ).name();
                add_msg( m_info, _( "That %s is already open." ), name.c_str() );
            }
            u.moves += 100;
        }
        return;
    }

    bool didit = m.open_door( openp, !m.is_outside( u.pos() ) );

    if( !didit ) {
        const ter_str_id tid = m.ter( openp ).id();

        if( m.has_flag( "LOCKED", openp ) ) {
            add_msg( m_info, _( "The door is locked!" ) );
            return;
        } else if( tid.obj().close ) {
            // if the following message appears unexpectedly, the prior check was for t_door_o
            add_msg( m_info, _( "That door is already open." ) );
            u.moves += 100;
            return;
        }
        add_msg( m_info, _( "No door there." ) );
        u.moves += 100;
    }
}

static void close()
{
    tripoint closep;
    if( choose_adjacent_highlight( _( "Close where?" ), closep, ACTION_CLOSE ) ) {
        doors::close_door( g->m, g->u, closep );
    }
}

static void handbrake()
{
    const optional_vpart_position vp = g->m.veh_at( g->u.pos() );
    if( !vp ) {
        return;
    }
    vehicle *const veh = &vp->vehicle();
    add_msg( _( "You pull a handbrake." ) );
    veh->cruise_velocity = 0;
    if( veh->last_turn != 0 && rng( 15, 60 ) * 100 < abs( veh->velocity ) ) {
        veh->skidding = true;
        add_msg( m_warning, _( "You lose control of %s." ), veh->name.c_str() );
        veh->turn( veh->last_turn > 0 ? 60 : -60 );
    } else {
        int braking_power = abs( veh->velocity ) / 2 + 10 * 100;
        if( abs( veh->velocity ) < braking_power ) {
            veh->stop();
        } else {
            int sgn = veh->velocity > 0 ? 1 : -1;
            veh->velocity = sgn * ( abs( veh->velocity ) - braking_power );
        }
    }
    g->u.moves = 0;
}

bool game::handle_action()
{
    std::string action;
    input_context ctxt;
    action_id act = ACTION_NULL;
    // Check if we have an auto-move destination
    if( u.has_destination() ) {
        act = u.get_next_auto_move_direction();
        if( act == ACTION_NULL ) {
            add_msg( m_info, _( "Auto-move canceled" ) );
            u.clear_destination();
            return false;
        }
    } else if( u.has_destination_activity() ) {
        // starts destination activity after the player successfully reached his destination
        u.start_destination_activity();
        return false;
    } else {
        // No auto-move, ask player for input
        ctxt = get_player_input( action );
    }

    const optional_vpart_position vp = m.veh_at( u.pos() );
    bool veh_ctrl = !u.is_dead_state() &&
                    ( ( vp && vp->vehicle().player_in_control( u ) ) || remoteveh() != nullptr );

    // If performing an action with right mouse button, co-ordinates
    // of location clicked.
    tripoint mouse_target = tripoint_min;

    // quit prompt check (ACTION_QUIT only grabs 'Q')
    if( uquit == QUIT_WATCH && action == "QUIT" ) {
        uquit = QUIT_DIED;
        return false;
    }

    if( act == ACTION_NULL ) {
        act = look_up_action( action );

        if( act == ACTION_MAIN_MENU ) {
            // No auto-move actions have or can be set at this point.
            u.clear_destination();
            destination_preview.clear();
            act = handle_main_menu();
            if( act == ACTION_NULL ) {
                return false;
            }
        }

        if( act == ACTION_ACTIONMENU ) {
            // No auto-move actions have or can be set at this point.
            u.clear_destination();
            destination_preview.clear();
            act = handle_action_menu();
            if( act == ACTION_NULL ) {
                return false;
            }
#ifdef __ANDROID__
            if( get_option<bool>( "ANDROID_ACTIONMENU_AUTOADD" ) && ctxt.get_category() == "DEFAULTMODE" ) {
                add_best_key_for_action_to_quick_shortcuts( act, ctxt.get_category(), false );
            }
#endif
        }

        if( can_action_change_worldstate( act ) ) {
            user_action_counter += 1;
        }

        if( act == ACTION_SELECT || act == ACTION_SEC_SELECT ) {
            // Mouse button click
            if( veh_ctrl ) {
                // No mouse use in vehicle
                return false;
            }

            if( u.is_dead_state() ) {
                // do not allow mouse actions while dead
                return false;
            }

            int mx = 0;
            int my = 0;
            if( !ctxt.get_coordinates( w_terrain, mx, my ) || !u.sees( tripoint( mx, my, u.posz() ) ) ) {
                // Not clicked in visible terrain
                return false;
            }
            mouse_target = tripoint( mx, my, u.posz() );

            if( act == ACTION_SELECT ) {
                // Note: The following has the potential side effect of
                // setting auto-move destination state in addition to setting
                // act.
                if( !try_get_left_click_action( act, mouse_target ) ) {
                    return false;
                }
            } else if( act == ACTION_SEC_SELECT ) {
                if( !try_get_right_click_action( act, mouse_target ) ) {
                    return false;
                }
            }
        } else if( act != ACTION_TIMEOUT ) {
            // act has not been set for an auto-move, so clearing possible
            // auto-move destinations. Since initializing an auto-move with
            // the mouse may span across multiple actions, we do not clear the
            // auto-move destination if the action is only a timeout, as this
            // would require the user to double click quicker than the
            // timeout delay.
            u.clear_destination();
            destination_preview.clear();
        }
    }

    if( act == ACTION_NULL ) {
        const input_event &&evt = ctxt.get_raw_input();
        if( !evt.sequence.empty() ) {
            const long ch = evt.get_first_input();
            const std::string &&name = inp_mngr.get_keyname( ch, evt.type, true );
            if( !get_option<bool>( "NO_UNKNOWN_COMMAND_MSG" ) ) {
                add_msg( m_info, _( "Unknown command: \"%s\" (%ld)" ), name, ch );
            }
        }
        return false;
    }

    // This has no action unless we're in a special game mode.
    gamemode->pre_action( act );

    int soffset = get_option<int>( "MOVE_VIEW_OFFSET" );
    int soffsetr = 0 - soffset;

    int before_action_moves = u.moves;

    // Use to track if auto-move should be canceled due to a failed
    // move or obstacle
    bool continue_auto_move = true;

    // These actions are allowed while deathcam is active.
    if( uquit == QUIT_WATCH || !u.is_dead_state() ) {
        switch( act ) {
            case ACTION_CENTER:
                u.view_offset.x = driving_view_offset.x;
                u.view_offset.y = driving_view_offset.y;
                break;

            case ACTION_SHIFT_N:
                u.view_offset.y += soffsetr;
                break;

            case ACTION_SHIFT_NE:
                u.view_offset.x += soffset;
                u.view_offset.y += soffsetr;
                break;

            case ACTION_SHIFT_E:
                u.view_offset.x += soffset;
                break;

            case ACTION_SHIFT_SE:
                u.view_offset.x += soffset;
                u.view_offset.y += soffset;
                break;

            case ACTION_SHIFT_S:
                u.view_offset.y += soffset;
                break;

            case ACTION_SHIFT_SW:
                u.view_offset.x += soffsetr;
                u.view_offset.y += soffset;
                break;

            case ACTION_SHIFT_W:
                u.view_offset.x += soffsetr;
                break;

            case ACTION_SHIFT_NW:
                u.view_offset.x += soffsetr;
                u.view_offset.y += soffsetr;
                break;

            case ACTION_LOOK:
                look_around();
                break;

            default:
                break;
        }
    }

    // actions allowed only while alive
    if( !u.is_dead_state() ) {
        switch( act ) {
            case ACTION_NULL:
            case NUM_ACTIONS:
                break; // dummy entries
            case ACTION_ACTIONMENU:
            case ACTION_MAIN_MENU:
                break; // handled above

            case ACTION_TIMEOUT:
                if( check_safe_mode_allowed( false ) ) {
                    u.pause();
                }
                break;

            case ACTION_PAUSE:
                if( check_safe_mode_allowed() ) {
                    u.pause();
                }
                break;

            case ACTION_TOGGLE_MOVE:
                u.toggle_move_mode();
                break;

            case ACTION_MOVE_N:
                if( !( u.get_value( "remote_controlling" ).empty() ) && ( ( u.has_active_item( "radiocontrol" ) ) ||
                        ( u.has_active_bionic( bio_remote ) ) ) ) {
                    rcdrive( 0, -1 );
                } else if( veh_ctrl ) {
                    pldrive( 0, -1 );
                } else {
                    continue_auto_move = plmove( 0, -1 );
                }
                break;

            case ACTION_MOVE_NE:
                if( !( u.get_value( "remote_controlling" ).empty() ) && ( ( u.has_active_item( "radiocontrol" ) ) ||
                        ( u.has_active_bionic( bio_remote ) ) ) ) {
                    rcdrive( 1, -1 );
                } else if( veh_ctrl ) {
                    pldrive( 1, -1 );
                } else {
                    continue_auto_move = plmove( 1, -1 );
                }
                break;

            case ACTION_MOVE_E:
                if( !( u.get_value( "remote_controlling" ).empty() ) && ( ( u.has_active_item( "radiocontrol" ) ) ||
                        ( u.has_active_bionic( bio_remote ) ) ) ) {
                    rcdrive( 1, 0 );
                } else if( veh_ctrl ) {
                    pldrive( 1, 0 );
                } else {
                    continue_auto_move = plmove( 1, 0 );
                }
                break;

            case ACTION_MOVE_SE:
                if( !( u.get_value( "remote_controlling" ).empty() ) && ( ( u.has_active_item( "radiocontrol" ) ) ||
                        ( u.has_active_bionic( bio_remote ) ) ) ) {
                    rcdrive( 1, 1 );
                } else if( veh_ctrl ) {
                    pldrive( 1, 1 );
                } else {
                    continue_auto_move = plmove( 1, 1 );
                }
                break;

            case ACTION_MOVE_S:
                if( !( u.get_value( "remote_controlling" ).empty() ) && ( ( u.has_active_item( "radiocontrol" ) ) ||
                        ( u.has_active_bionic( bio_remote ) ) ) ) {
                    rcdrive( 0, 1 );
                } else if( veh_ctrl ) {
                    pldrive( 0, 1 );
                } else {
                    continue_auto_move = plmove( 0, 1 );
                }
                break;

            case ACTION_MOVE_SW:
                if( !( u.get_value( "remote_controlling" ).empty() ) && ( ( u.has_active_item( "radiocontrol" ) ) ||
                        ( u.has_active_bionic( bio_remote ) ) ) ) {
                    rcdrive( -1, 1 );
                } else if( veh_ctrl ) {
                    pldrive( -1, 1 );
                } else {
                    continue_auto_move = plmove( -1, 1 );
                }
                break;

            case ACTION_MOVE_W:
                if( !( u.get_value( "remote_controlling" ).empty() ) && ( ( u.has_active_item( "radiocontrol" ) ) ||
                        ( u.has_active_bionic( bio_remote ) ) ) ) {
                    rcdrive( -1, 0 );
                } else if( veh_ctrl ) {
                    pldrive( -1, 0 );
                } else {
                    continue_auto_move = plmove( -1, 0 );
                }
                break;

            case ACTION_MOVE_NW:
                if( !( u.get_value( "remote_controlling" ).empty() ) && ( ( u.has_active_item( "radiocontrol" ) ) ||
                        ( u.has_active_bionic( bio_remote ) ) ) ) {
                    rcdrive( -1, -1 );
                } else if( veh_ctrl ) {
                    pldrive( -1, -1 );
                } else {
                    continue_auto_move = plmove( -1, -1 );
                }
                break;

            case ACTION_MOVE_DOWN:
                if( !u.in_vehicle ) {
                    vertical_move( -1, false );
                }
                break;

            case ACTION_MOVE_UP:
                if( !u.in_vehicle ) {
                    vertical_move( 1, false );
                }
                break;

            case ACTION_OPEN:
                if( u.has_active_mutation( trait_SHELL2 ) ) {
                    add_msg( m_info, _( "You can't open things while you're in your shell." ) );
                } else {
                    open();
                }
                break;

            case ACTION_CLOSE:
                if( u.has_active_mutation( trait_SHELL2 ) ) {
                    add_msg( m_info, _( "You can't close things while you're in your shell." ) );
                } else if( mouse_target != tripoint_min ) {
                    doors::close_door( m, u, mouse_target );
                } else {
                    close();
                }
                break;

            case ACTION_SMASH:
                if( veh_ctrl ) {
                    handbrake();
                } else if( u.has_active_mutation( trait_SHELL2 ) ) {
                    add_msg( m_info, _( "You can't smash things while you're in your shell." ) );
                } else {
                    smash();
                }
                break;

            case ACTION_EXAMINE:
                if( u.has_active_mutation( trait_SHELL2 ) ) {
                    add_msg( m_info, _( "You can't examine your surroundings while you're in your shell." ) );
                } else if( mouse_target != tripoint_min ) {
                    examine( mouse_target );
                } else {
                    examine();
                }
                break;

            case ACTION_ADVANCEDINV:
                if( u.has_active_mutation( trait_SHELL2 ) ) {
                    add_msg( m_info, _( "You can't move mass quantities while you're in your shell." ) );
                } else {
                    advanced_inv();
                }
                break;

            case ACTION_PICKUP:
                Pickup::pick_up( u.pos(), 1 );
                break;

            case ACTION_GRAB:
                if( u.has_active_mutation( trait_SHELL2 ) ) {
                    add_msg( m_info, _( "You can't grab things while you're in your shell." ) );
                } else {
                    grab();
                }
                break;

            case ACTION_BUTCHER:
                if( u.has_active_mutation( trait_SHELL2 ) ) {
                    add_msg( m_info, _( "You can't butcher while you're in your shell." ) );
                } else {
                    butcher();
                }
                break;

            case ACTION_CHAT:
                chat();
                break;

            case ACTION_PEEK:
                if( u.has_active_mutation( trait_SHELL2 ) ) {
                    add_msg( m_info, _( "You can't peek around corners while you're in your shell." ) );
                } else {
                    peek();
                }
                break;

            case ACTION_LIST_ITEMS:
                list_items_monsters();
                break;

            case ACTION_ZONES:
                zones_manager();
                break;

            case ACTION_LOOT:
                loot();
                break;

            case ACTION_INVENTORY:
                game_menus::inv::common( u );
                break;

            case ACTION_COMPARE:
                game_menus::inv::compare( u );
                break;

            case ACTION_ORGANIZE:
                game_menus::inv::swap_letters( u );
                break;

            case ACTION_USE:
                // Shell-users are presumed to be able to mess with their inventories, etc
                // while in the shell.  Eating, gear-changing, and item use are OK.
                use_item();
                break;

            case ACTION_USE_WIELDED:
                use_wielded_item();
                break;

            case ACTION_WEAR:
                wear();
                break;

            case ACTION_TAKE_OFF:
                takeoff();
                break;

            case ACTION_EAT:
                eat();
                break;

            case ACTION_READ:
                // Shell-users are presumed to have the book just at an opening and read it that way
                read();
                break;

            case ACTION_WIELD:
                wield();
                break;

            case ACTION_PICK_STYLE:
                u.pick_style();
                break;

            case ACTION_RELOAD:
                reload();
                break;

            case ACTION_UNLOAD:
                unload();
                break;

            case ACTION_MEND:
                mend();
                break;

            case ACTION_THROW:
                plthrow();
                break;

            case ACTION_FIRE:
                // @todo: Move handling ACTION_FIRE to a new function.
                // Use vehicle turret or draw a pistol from a holster if unarmed
                if( !u.is_armed() ) {

                    const optional_vpart_position vp = m.veh_at( u.pos() );

                    turret_data turret;
                    // @todo: move direct turret firing from ACTION_FIRE to separate function.
                    if( vp && ( turret = vp->vehicle().turret_query( u.pos() ) ) ) {
                        switch( turret.query() ) {
                            case turret_data::status::no_ammo:
                                add_msg( m_bad, _( "The %s is out of ammo." ), turret.name().c_str() );
                                break;

                            case turret_data::status::no_power:
                                add_msg( m_bad,  _( "The %s is not powered." ), turret.name().c_str() );
                                break;

                            case turret_data::status::ready: {
                                // if more than one firing mode provide callback to cycle through them
                                target_callback switch_mode;
                                if( turret.base()->gun_all_modes().size() > 1 ) {
                                    switch_mode = [&turret]( item * obj ) {
                                        obj->gun_cycle_mode();
                                        // currently gun modes do not change ammo but they may in the future
                                        return turret.ammo_current() == "null" ? nullptr :
                                               item::find_type( turret.ammo_current() );
                                    };
                                }

                                // if multiple ammo types available provide callback to cycle alternatives
                                target_callback switch_ammo;
                                if( turret.ammo_options().size() > 1 ) {
                                    switch_ammo = [&turret]( item * ) {
                                        const auto opts = turret.ammo_options();
                                        auto iter = opts.find( turret.ammo_current() );
                                        turret.ammo_select( ++iter != opts.end() ? *iter : *opts.begin() );
                                        return item::find_type( turret.ammo_current() );
                                    };
                                }

                                // callbacks for handling setup and cleanup of turret firing
                                firing_callback prepare_fire = [this, &turret]( const int ) {
                                    turret.prepare_fire( u );
                                };
                                firing_callback post_fire = [this, &turret]( const int shots ) {
                                    turret.post_fire( u, shots );
                                };

                                targeting_data args = {
                                    TARGET_MODE_TURRET_MANUAL, & *turret.base(),
                                    turret.range(), 0, false, turret.ammo_data(),
                                    switch_mode, switch_ammo, prepare_fire, post_fire
                                };
                                u.set_targeting_data( args );
                                plfire();

                                break;
                            }

                            default:
                                debugmsg( "unknown turret status" );
                                break;
                        }
                        break;
                    }

                    if( vp.part_with_feature( "CONTROLS", true ) ) {
                        if( vp->vehicle().turrets_aim_and_fire() ) {
                            break;
                        }
                    }

                    std::vector<std::string> options( 1, _( "Cancel" ) );
                    std::vector<std::function<void()>> actions( 1, [] {} );

                    for( auto &w : u.worn ) {
                        if( w.type->can_use( "holster" ) && !w.has_flag( "NO_QUICKDRAW" ) &&
                            !w.contents.empty() && w.contents.front().is_gun() ) {
                            // draw (first) gun contained in holster
                            options.push_back( string_format( _( "%s from %s (%d)" ),
                                                              w.contents.front().tname().c_str(),
                                                              w.type_name().c_str(),
                                                              w.contents.front().ammo_remaining() ) );

                            actions.emplace_back( [&] { u.invoke_item( &w, "holster" ); } );

                        } else if( w.is_gun() && w.gunmod_find( "shoulder_strap" ) ) {
                            // wield item currently worn using shoulder strap
                            options.push_back( w.display_name() );
                            actions.emplace_back( [&] { u.wield( w ); } );
                        }
                    }
                    if( options.size() > 1 ) {
                        actions[( uimenu( false, _( "Draw what?" ), options ) ) - 1 ]();
                    }
                }

                if( u.weapon.is_gun() && !u.weapon.gun_current_mode().melee() ) {
                    plfire( u.weapon );
                } else if( u.weapon.has_flag( "REACH_ATTACK" ) ) {
                    int range = u.weapon.has_flag( "REACH3" ) ? 3 : 2;
                    temp_exit_fullscreen();
                    m.draw( w_terrain, u.pos() );
                    std::vector<tripoint> trajectory;
                    trajectory = target_handler().target_ui( u, TARGET_MODE_REACH, &u.weapon, range );
                    if( !trajectory.empty() ) {
                        u.reach_attack( trajectory.back() );
                    }
                    draw_ter();
                    wrefresh( w_terrain );
                    reenter_fullscreen();
                }
                break;

            case ACTION_FIRE_BURST: {
                gun_mode_id original_mode = u.weapon.gun_get_mode_id();
                if( u.weapon.gun_set_mode( gun_mode_id( "AUTO" ) ) ) {
                    plfire( u.weapon );
                    u.weapon.gun_set_mode( original_mode );
                }
                break;
            }

            case ACTION_SELECT_FIRE_MODE:
                if( u.is_armed() ) {
                    u.weapon.gun_cycle_mode();
                }
                break;

            case ACTION_DROP:
                // You CAN drop things to your own tile while in the shell.
                drop();
                break;

            case ACTION_DIR_DROP:
                if( u.has_active_mutation( trait_SHELL2 ) ) {
                    add_msg( m_info, _( "You can't drop things to another tile while you're in your shell." ) );
                } else {
                    drop_in_direction();
                }
                break;
            case ACTION_BIONICS:
                u.power_bionics();
                refresh_all();
                break;
            case ACTION_MUTATIONS:
                u.power_mutations();
                refresh_all();
                break;

            case ACTION_SORT_ARMOR:
                u.sort_armor();
                refresh_all();
                break;

            case ACTION_WAIT:
                wait();
                break;

            case ACTION_CRAFT:
                if( u.has_active_mutation( trait_SHELL2 ) ) {
                    add_msg( m_info, _( "You can't craft while you're in your shell." ) );
                } else {
                    u.craft();
                }
                break;

            case ACTION_RECRAFT:
                if( u.has_active_mutation( trait_SHELL2 ) ) {
                    add_msg( m_info, _( "You can't craft while you're in your shell." ) );
                } else {
                    u.recraft();
                }
                break;

            case ACTION_LONGCRAFT:
                if( u.has_active_mutation( trait_SHELL2 ) ) {
                    add_msg( m_info, _( "You can't craft while you're in your shell." ) );
                } else {
                    u.long_craft();
                }
                break;

            case ACTION_DISASSEMBLE:
                if( u.controlling_vehicle ) {
                    add_msg( m_info, _( "You can't disassemble items while driving." ) );
                } else {
                    u.disassemble();
                    refresh_all();
                }
                break;

            case ACTION_CONSTRUCT:
                if( u.in_vehicle ) {
                    add_msg( m_info, _( "You can't construct while in a vehicle." ) );
                } else if( u.has_active_mutation( trait_SHELL2 ) ) {
                    add_msg( m_info, _( "You can't construct while you're in your shell." ) );
                } else {
                    construction_menu();
                }
                break;

            case ACTION_SLEEP:
                if( veh_ctrl ) {
                    add_msg( m_info, _( "Vehicle control has moved, %s" ),
                             press_x( ACTION_CONTROL_VEHICLE, _( "new binding is " ),
                                      _( "new default binding is '^'." ) ).c_str() );
                } else {
                    uimenu as_m;
                    // Only accept valid input
                    as_m.return_invalid = false;
                    as_m.text = _( "Are you sure you want to sleep?" );
                    // (Y)es/(S)ave before sleeping/(N)o
                    as_m.entries.emplace_back( uimenu_entry( 0, true,
                                               ( get_option<bool>( "FORCE_CAPITAL_YN" ) ? 'Y' : 'y' ),
                                               _( "Yes." ) ) );
                    as_m.entries.emplace_back( uimenu_entry( 1, ( moves_since_last_save ),
                                               ( get_option<bool>( "FORCE_CAPITAL_YN" ) ? 'S' : 's' ),
                                               _( "Yes, and save game before sleeping." ) ) );
                    as_m.entries.emplace_back( uimenu_entry( 2, true,
                                               ( get_option<bool>( "FORCE_CAPITAL_YN" ) ? 'N' : 'n' ),
                                               _( "No." ) ) );

                    // List all active items, bionics or mutations so player can deactivate them
                    std::vector<std::string> active;
                    for( auto &it : g->u.inv_dump() ) {
                        if( it->active && it->charges > 0 && it->is_tool_reversible() ) {
                            active.push_back( it->tname() );
                        }
                    }
                    for( int i = 0; i < g->u.num_bionics(); i++ ) {
                        bionic const &bio = g->u.bionic_at_index( i );
                        if( !bio.powered ) {
                            continue;
                        }

                        // bio_alarm is useful for waking up during sleeping
                        // turning off bio_leukocyte has 'unpleasant side effects'
                        if( bio.id == bionic_id( "bio_alarm" ) || bio.id == bionic_id( "bio_leukocyte" ) ) {
                            continue;
                        }

                        auto const &info = bio.info();
                        if( info.power_over_time > 0 ) {
                            active.push_back( info.name );
                        }
                    }
                    for( auto &mut : g->u.get_mutations() ) {
                        const auto &mdata = mut.obj();
                        if( mdata.cost > 0 && u.has_active_mutation( mut ) ) {
                            active.push_back( mdata.name );
                        }
                    }
                    std::stringstream data;
                    if( !active.empty() ) {
                        data << as_m.text << std::endl;
                        data << _( "You may want to deactivate these before you sleep." ) << std::endl;
                        data << " " << std::endl;
                        for( auto &a : active ) {
                            data << a << std::endl;
                        }
                        as_m.text = data.str();
                    }

                    /* Calculate key and window variables, generate window,
                       and loop until we get a valid answer. */
                    as_m.query();

                    if( as_m.ret == 1 ) {
                        quicksave();
                    } else if( as_m.ret == 2 ) {
                        break;
                    }

                    /* Reuse menu to ask player whether they want to set an alarm. */
                    bool can_hibernate = u.get_hunger() < -60 && u.has_active_mutation( trait_HIBERNATE );

                    as_m.reset();
                    as_m.text = can_hibernate ?
                                _( "You're engorged to hibernate. The alarm would only attract attention. Set an alarm anyway?" ) :
                                _( "You have an alarm clock. Set an alarm?" );

                    if( u.has_alarm_clock() ) {
                        as_m.entries.emplace_back( uimenu_entry( 0, true,
                                                   ( get_option<bool>( "FORCE_CAPITAL_YN" ) ? 'N' : 'n' ),
                                                   _( "No, don't set an alarm." ) ) );

                        for( int i = 3; i <= 9; ++i ) {
                            as_m.entries.emplace_back( uimenu_entry( i, true, '0' + i,
                                                       string_format( _( "Set alarm to wake up in %i hours." ), i ) ) );
                        }
                    }

                    as_m.query();
                    if( as_m.ret >= 3 && as_m.ret <= 9 ) {
                        u.add_effect( effect_alarm_clock, 1_hours * as_m.ret );
                    }

                    u.moves = 0;
                    u.try_to_sleep();
                }
                break;

            case ACTION_CONTROL_VEHICLE:
                if( u.has_active_mutation( trait_SHELL2 ) ) {
                    add_msg( m_info, _( "You can't operate a vehicle while you're in your shell." ) );
                } else {
                    control_vehicle();
                }
                break;

            case ACTION_TOGGLE_SAFEMODE:
                if( safe_mode == SAFE_MODE_OFF ) {
                    set_safe_mode( SAFE_MODE_ON );
                    mostseen = 0;
                    add_msg( m_info, _( "Safe mode ON!" ) );
                } else {
                    turnssincelastmon = 0;
                    set_safe_mode( SAFE_MODE_OFF );
                    add_msg( m_info, get_option<bool>( "AUTOSAFEMODE" )
                             ? _( "Safe mode OFF! (Auto safe mode still enabled!)" ) : _( "Safe mode OFF!" ) );
                }
                if( u.has_effect( effect_laserlocked ) ) {
                    u.remove_effect( effect_laserlocked );
                    safe_mode_warning_logged = false;
                }
                break;

            case ACTION_TOGGLE_AUTOSAFE: {
                auto &autosafemode_option = get_options().get_option( "AUTOSAFEMODE" );
                add_msg( m_info, autosafemode_option.value_as<bool>()
                         ? _( "Auto safe mode OFF!" ) : _( "Auto safe mode ON!" ) );
                autosafemode_option.setNext();
                break;
            }

            case ACTION_IGNORE_ENEMY:
                if( safe_mode == SAFE_MODE_STOP ) {
                    add_msg( m_info, _( "Ignoring enemy!" ) );
                    for( auto &elem : new_seen_mon ) {
                        monster &critter = *elem;
                        critter.ignoring = rl_dist( u.pos(), critter.pos() );
                    }
                    set_safe_mode( SAFE_MODE_ON );
                } else if( u.has_effect( effect_laserlocked ) ) {
                    add_msg( m_info, _( "Ignoring laser targeting!" ) );
                    u.remove_effect( effect_laserlocked );
                    safe_mode_warning_logged = false;
                }
                break;

            case ACTION_WHITELIST_ENEMY:
                if( safe_mode == SAFE_MODE_STOP && !get_safemode().empty() ) {
                    get_safemode().add_rule( get_safemode().lastmon_whitelist, Creature::A_ANY, 0, RULE_WHITELISTED );
                    add_msg( m_info, _( "Creature whitelisted: %s" ), get_safemode().lastmon_whitelist.c_str() );
                    set_safe_mode( SAFE_MODE_ON );
                    mostseen = 0;
                } else {
                    get_safemode().show();
                }
                break;

            case ACTION_QUIT:
                if( query_yn( _( "Commit suicide?" ) ) ) {
                    if( query_yn( _( "REALLY commit suicide?" ) ) ) {
                        u.moves = 0;
                        u.place_corpse();
                        uquit = QUIT_SUICIDE;
                    }
                }
                refresh_all();
                break;

            case ACTION_SAVE:
                if( query_yn( _( "Save and quit?" ) ) ) {
                    if( save() ) {
                        u.moves = 0;
                        uquit = QUIT_SAVED;
                    }
                }
                refresh_all();
                break;

            case ACTION_QUICKSAVE:
                quicksave();
                return false;

            case ACTION_QUICKLOAD:
                quickload();
                return false;

            case ACTION_PL_INFO:
                u.disp_info();
                refresh_all();
                break;

            case ACTION_MAP:
                werase( w_terrain );
                draw_overmap();
                break;

            case ACTION_MISSIONS:
                list_missions();
                break;

            case ACTION_KILLS:
                disp_kills();
                break;

            case ACTION_FACTIONS:
                faction_manager_ptr->display();
                refresh_all();
                break;

            case ACTION_MORALE:
                u.disp_morale();
                refresh_all();
                break;

            case ACTION_MESSAGES:
                Messages::display_messages();
                refresh_all();
                break;

            case ACTION_HELP:
                display_help();
                refresh_all();
                break;

            case ACTION_KEYBINDINGS:
                ctxt.display_menu();
                refresh_all();
                break;

            case ACTION_OPTIONS:
                get_options().show( true );
                refresh_all();
                break;

            case ACTION_AUTOPICKUP:
                get_auto_pickup().show();
                refresh_all();
                break;

            case ACTION_SAFEMODE:
                get_safemode().show();
                refresh_all();
                break;

            case ACTION_COLOR:
                all_colors.show_gui();
                refresh_all();
                break;

            case ACTION_WORLD_MODS:
                world_generator->show_active_world_mods( world_generator->active_world->active_mod_order );
                refresh_all();
                break;

            case ACTION_DEBUG:
                if( MAP_SHARING::isCompetitive() && !MAP_SHARING::isDebugger() ) {
                    break;    //don't do anything when sharing and not debugger
                }
                debug();
                refresh_all();
                break;

            case ACTION_TOGGLE_SIDEBAR_STYLE:
                toggle_sidebar_style();
                break;

            case ACTION_TOGGLE_FULLSCREEN:
                toggle_fullscreen();
                break;

            case ACTION_TOGGLE_PIXEL_MINIMAP:
                toggle_pixel_minimap();
                break;

            case ACTION_TOGGLE_AUTO_PULP_BUTCHER:
                get_options().get_option( "AUTO_PULP_BUTCHER" ).setNext();
                get_options().save();
                //~ Auto Pulp/Pulp Adjacent/Butcher is now ON/OFF
                add_msg( string_format( _( "Auto %1$s is now %2$s." ),
                                        get_options().get_option( "AUTO_PULP_BUTCHER_ACTION" ).getValueName(),
                                        get_option<bool>( "AUTO_PULP_BUTCHER" ) ? _( "ON" ) : _( "OFF" ) ).c_str() );
                break;

            case ACTION_DISPLAY_SCENT:
                if( MAP_SHARING::isCompetitive() && !MAP_SHARING::isDebugger() ) {
                    break;    //don't do anything when sharing and not debugger
                }
                display_scent();
                break;

            case ACTION_TOGGLE_DEBUG_MODE:
                if( MAP_SHARING::isCompetitive() && !MAP_SHARING::isDebugger() ) {
                    break;    //don't do anything when sharing and not debugger
                }
                debug_mode = !debug_mode;
                if( debug_mode ) {
                    add_msg( m_info, _( "Debug mode ON!" ) );
                } else {
                    add_msg( m_info, _( "Debug mode OFF!" ) );
                }
                break;

            case ACTION_ZOOM_IN:
                zoom_in();
                break;

            case ACTION_ZOOM_OUT:
                zoom_out();
                break;

            case ACTION_ITEMACTION:
                item_action_menu();
                break;

            case ACTION_AUTOATTACK:
                autoattack();
                break;

            default:
                break;
        }
    }
    if( !continue_auto_move ) {
        u.clear_destination();
    }

    gamemode->post_action( act );

    u.movecounter = ( !u.is_dead_state() ? ( before_action_moves - u.moves ) : 0 );
    dbg( D_INFO ) << string_format( "%s: [%d] %d - %d = %d", action_ident( act ).c_str(),
                                    int( calendar::turn ), before_action_moves, u.movecounter, u.moves );
    return ( !u.is_dead_state() );
}
