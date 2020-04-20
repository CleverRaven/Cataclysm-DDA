#include "game.h" // IWYU pragma: associated

#include <chrono>
#include <cstdlib>
#include <initializer_list>
#include <set>
#include <sstream>
#include <utility>

#include "action.h"
#include "advanced_inv.h"
#include "auto_note.h"
#include "auto_pickup.h"
#include "avatar.h"
#include "avatar_action.h"
#include "bionics.h"
#include "calendar.h"
#include "catacharset.h"
#include "character.h"
#include "character_martial_arts.h"
#include "clzones.h"
#include "color.h"
#include "construction.h"
#include "cursesdef.h"
#include "damage.h"
#include "debug.h"
#include "debug_menu.h"
#include "faction.h"
#include "field.h"
#include "field_type.h"
#include "game_constants.h"
#include "game_inventory.h"
#include "gamemode.h"
#include "gates.h"
#include "gun_mode.h"
#include "help.h"
#include "input.h"
#include "int_id.h"
#include "item.h"
#include "item_contents.h"
#include "item_group.h"
#include "itype.h"
#include "iuse.h"
#include "lightmap.h"
#include "line.h"
#include "magic.h"
#include "map.h"
#include "mapdata.h"
#include "mapsharing.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "mutation.h"
#include "options.h"
#include "output.h"
#include "overmap_ui.h"
#include "panels.h"
#include "player.h"
#include "player_activity.h"
#include "popup.h"
#include "ranged.h"
#include "rng.h"
#include "safemode_ui.h"
#include "scores_ui.h"
#include "sounds.h"
#include "string_formatter.h"
#include "string_id.h"
#include "string_input_popup.h"
#include "translations.h"
#include "ui.h"
#include "units.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "weather.h"
#include "worldfactory.h"

static const activity_id ACT_FERTILIZE_PLOT( "ACT_FERTILIZE_PLOT" );
static const activity_id ACT_MOVE_LOOT( "ACT_MOVE_LOOT" );
static const activity_id ACT_MULTIPLE_BUTCHER( "ACT_MULTIPLE_BUTCHER" );
static const activity_id ACT_MULTIPLE_CHOP_PLANKS( "ACT_MULTIPLE_CHOP_PLANKS" );
static const activity_id ACT_MULTIPLE_CHOP_TREES( "ACT_MULTIPLE_CHOP_TREES" );
static const activity_id ACT_MULTIPLE_CONSTRUCTION( "ACT_MULTIPLE_CONSTRUCTION" );
static const activity_id ACT_MULTIPLE_FARM( "ACT_MULTIPLE_FARM" );
static const activity_id ACT_MULTIPLE_MINE( "ACT_MULTIPLE_MINE" );
static const activity_id ACT_PULP( "ACT_PULP" );
static const activity_id ACT_SPELLCASTING( "ACT_SPELLCASTING" );
static const activity_id ACT_VEHICLE_DECONSTRUCTION( "ACT_VEHICLE_DECONSTRUCTION" );
static const activity_id ACT_VEHICLE_REPAIR( "ACT_VEHICLE_REPAIR" );
static const activity_id ACT_WAIT( "ACT_WAIT" );
static const activity_id ACT_WAIT_STAMINA( "ACT_WAIT_STAMINA" );
static const activity_id ACT_WAIT_WEATHER( "ACT_WAIT_WEATHER" );

static const efftype_id effect_alarm_clock( "alarm_clock" );
static const efftype_id effect_laserlocked( "laserlocked" );
static const efftype_id effect_relax_gas( "relax_gas" );

static const skill_id skill_melee( "melee" );

static const quality_id qual_CUT( "CUT" );

static const bionic_id bio_remote( "bio_remote" );

static const trait_id trait_HIBERNATE( "HIBERNATE" );
static const trait_id trait_PROF_CHURL( "PROF_CHURL" );
static const trait_id trait_SHELL2( "SHELL2" );
static const trait_id trait_WAYFARER( "WAYFARER" );

static const std::string flag_LITCIG( "LITCIG" );
static const std::string flag_LOCKED( "LOCKED" );
static const std::string flag_MAGIC_FOCUS( "MAGIC_FOCUS" );
static const std::string flag_NO_QUICKDRAW( "NO_QUICKDRAW" );
static const std::string flag_REACH_ATTACK( "REACH_ATTACK" );
static const std::string flag_REACH3( "REACH3" );
static const std::string flag_RELOAD_AND_SHOOT( "RELOAD_AND_SHOOT" );
static const std::string flag_RELOAD_ONE( "RELOAD_ONE" );

static const std::string flag_SLEEP_IGNORE( "SLEEP_IGNORE" );

#define dbg(x) DebugLog((x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

#if defined(__ANDROID__)
extern std::map<std::string, std::list<input_event>> quick_shortcuts_map;
extern bool add_best_key_for_action_to_quick_shortcuts( action_id action,
        const std::string &category, bool back );
extern bool add_key_to_quick_shortcuts( int key, const std::string &category, bool back );
#endif

class user_turn
{

    private:
        std::chrono::time_point<std::chrono::steady_clock> user_turn_start;
    public:
        user_turn() {
            user_turn_start = std::chrono::steady_clock::now();
        }

        bool has_timeout_elapsed() {
            return moves_elapsed() > 100;
        }

        int moves_elapsed() {
            const float turn_duration = get_option<float>( "TURN_DURATION" );
            // Magic number 0.005 chosen due to option menu's 2 digit precision and
            // the option menu UI rounding <= 0.005 down to "0.00" in the display.
            // This conditional will catch values (e.g. 0.003) that the options menu
            // would round down to "0.00" in the options menu display. This prevents
            // the user from being surprised by floating point rounding near zero.
            if( turn_duration <= 0.005 ) {
                return 0;
            }
            auto now = std::chrono::steady_clock::now();
            std::chrono::milliseconds elapsed_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>( now - user_turn_start );
            return elapsed_ms.count() / ( 10.0 * turn_duration );
        }

};

input_context game::get_player_input( std::string &action )
{
    input_context ctxt;
    if( uquit == QUIT_WATCH ) {
        ctxt = input_context( "DEFAULTMODE" );
        ctxt.set_iso( true );
        // The list of allowed actions in death-cam mode in game::handle_action
        // *INDENT-OFF*
        for( const action_id id : {
            ACTION_TOGGLE_MAP_MEMORY,
            ACTION_CENTER,
            ACTION_SHIFT_N,
            ACTION_SHIFT_NE,
            ACTION_SHIFT_E,
            ACTION_SHIFT_SE,
            ACTION_SHIFT_S,
            ACTION_SHIFT_SW,
            ACTION_SHIFT_W,
            ACTION_SHIFT_NW,
            ACTION_LOOK,
            ACTION_KEYBINDINGS,
        } ) {
            ctxt.register_action( action_ident( id ) );
        }
        // *INDENT-ON*
        ctxt.register_action( "QUIT", to_translation( "Accept your fate" ) );
    } else {
        ctxt = get_default_mode_input_context();
    }

    m.update_visibility_cache( u.posz() );
    const visibility_variables &cache = g->m.get_visibility_variables_cache();
    const level_cache &map_cache = m.get_cache_ref( u.posz() );
    const auto &visibility_cache = map_cache.visibility_cache;

    user_turn current_turn;

    if( get_option<bool>( "ANIMATIONS" ) ) {
        const int TOTAL_VIEW = MAX_VIEW_DISTANCE * 2 + 1;
        int iStartX = ( TERRAIN_WINDOW_WIDTH > TOTAL_VIEW ) ? ( TERRAIN_WINDOW_WIDTH - TOTAL_VIEW ) / 2 : 0;
        int iStartY = ( TERRAIN_WINDOW_HEIGHT > TOTAL_VIEW ) ? ( TERRAIN_WINDOW_HEIGHT - TOTAL_VIEW ) / 2 :
                      0;
        int iEndX = ( TERRAIN_WINDOW_WIDTH > TOTAL_VIEW ) ? TERRAIN_WINDOW_WIDTH -
                    ( TERRAIN_WINDOW_WIDTH - TOTAL_VIEW ) /
                    2 :
                    TERRAIN_WINDOW_WIDTH;
        int iEndY = ( TERRAIN_WINDOW_HEIGHT > TOTAL_VIEW ) ? TERRAIN_WINDOW_HEIGHT -
                    ( TERRAIN_WINDOW_HEIGHT - TOTAL_VIEW ) /
                    2 : TERRAIN_WINDOW_HEIGHT;

        if( fullscreen ) {
            iStartX = 0;
            iStartY = 0;
            iEndX = TERMX;
            iEndY = TERMY;
        }

        //x% of the Viewport, only shown on visible areas
        const auto weather_info = get_weather_animation( weather.weather );
        int offset_x = u.posx() + u.view_offset.x - getmaxx( w_terrain ) / 2;
        int offset_y = u.posy() + u.view_offset.y - getmaxy( w_terrain ) / 2;

#if defined(TILES)
        if( tile_iso && use_tiles ) {
            iStartX = 0;
            iStartY = 0;
            iEndX = MAPSIZE_X;
            iEndY = MAPSIZE_Y;
            offset_x = 0;
            offset_y = 0;
        }
#endif //TILES

        // TODO: Move the weather calculations out of here.
        const bool bWeatherEffect = ( weather_info.glyph != '?' );
        const int dropCount = static_cast<int>( iEndX * iEndY * weather_info.factor );

        weather_printable wPrint;
        wPrint.colGlyph = weather_info.color;
        wPrint.cGlyph = weather_info.glyph;
        wPrint.wtype = weather.weather;
        wPrint.vdrops.clear();

        ctxt.set_timeout( 125 );
        bool initial_draw = true;
        do {
            if( bWeatherEffect && get_option<bool>( "ANIMATION_RAIN" ) ) {
                /*
                Location to add rain drop animation bits! Since it refreshes w_terrain it can be added to the animation section easily
                Get tile information from above's weather information:
                WEATHER_ACID_DRIZZLE | WEATHER_ACID_RAIN = "weather_acid_drop"
                WEATHER_DRIZZLE | WEATHER_LIGHT_DRIZZLE | WEATHER_RAINY | WEATHER_THUNDER | WEATHER_LIGHTNING = "weather_rain_drop"
                WEATHER_FLURRIES | WEATHER_SNOW | WEATHER_SNOWSTORM = "weather_snowflake"
                */

#if defined(TILES)
                if( !use_tiles ) {
#endif //TILES
                    //If not using tiles, erase previous drops from w_terrain
                    for( auto &elem : wPrint.vdrops ) {
                        const tripoint location( elem.first + offset_x, elem.second + offset_y, get_levz() );
                        const lit_level lighting = visibility_cache[location.x][location.y];
                        wmove( w_terrain, location.xy() + point( -offset_x, -offset_y ) );
                        if( !m.apply_vision_effects( w_terrain, m.get_visibility( lighting, cache ) ) ) {
                            m.drawsq( w_terrain, u, location, false, true,
                                      u.pos() + u.view_offset,
                                      lighting == LL_LOW, lighting == LL_BRIGHT );
                        }
                    }
#if defined(TILES)
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
#if defined(TILES)
                if( !use_tiles ) {
#endif
                    for( auto &elem : SCT.vSCT ) {
                        //Erase previous text from w_terrain
                        if( elem.getStep() > 0 ) {
                            const int width = utf8_width( elem.getText() );
                            for( int i = 0; i < width; ++i ) {
                                const tripoint location( elem.getPosX() + i, elem.getPosY(), get_levz() );
                                const lit_level lighting = visibility_cache[location.x][location.y];
                                wmove( w_terrain, location.xy() + point( -offset_x, -offset_y ) );
                                if( !m.apply_vision_effects( w_terrain, m.get_visibility( lighting, cache ) ) ) {
                                    m.drawsq( w_terrain, u, location, false, true,
                                              u.pos() + u.view_offset,
                                              lighting == LL_LOW, lighting == LL_BRIGHT );
                                }
                            }
                        }
                    }
#if defined(TILES)
                }
#endif

                SCT.advanceAllSteps();

                //Check for creatures on all drawing positions and offset if necessary
                for( auto iter = SCT.vSCT.rbegin(); iter != SCT.vSCT.rend(); ++iter ) {
                    const direction oCurDir = iter->getDirecton();
                    const int width = utf8_width( iter->getText() );
                    for( int i = 0; i < width; ++i ) {
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

            if( initial_draw ) {
                werase( w_terrain );

                draw_ter();
                initial_draw = false;
            }
            draw_weather( wPrint );

            if( uquit != QUIT_WATCH ) {
                draw_sct();
            }

            wrefresh( w_terrain );
            g->draw_panels();

            if( uquit == QUIT_WATCH ) {
                query_popup()
                .wait_message( c_red, _( "Press %s to accept your fate…" ), ctxt.get_desc( "QUIT" ) )
                .on_top( true )
                .show();
            }
        } while( handle_mouseview( ctxt, action ) && uquit != QUIT_WATCH
                 && ( action != "TIMEOUT" || !current_turn.has_timeout_elapsed() ) );
        ctxt.reset_timeout();
    } else {
        ctxt.set_timeout( 125 );
        while( handle_mouseview( ctxt, action ) ) {
            if( action == "TIMEOUT" && current_turn.has_timeout_elapsed() ) {
                break;
            }
        }
        ctxt.reset_timeout();
    }

    return ctxt;
}

inline static void rcdrive( const point &d )
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

    auto rc_pairs = m.get_rc_items( tripoint( cx, cy, cz ) );
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

    tripoint dest( cx + d.x, cy + d.y, cz );
    if( m.impassable( dest ) || !m.can_put_items_ter_furn( dest ) ||
        m.has_furn( dest ) ) {
        sounds::sound( dest, 7, sounds::sound_t::combat,
                       _( "sound of a collision with an obstacle." ), true, "misc", "rc_car_hits_obstacle" );
        return;
    } else if( !m.add_item_or_charges( dest, *rc_car ).is_null() ) {
        tripoint src( cx, cy, cz );
        //~ Sound of moving a remote controlled car
        sounds::sound( src, 6, sounds::sound_t::movement, _( "zzz…" ), true, "misc", "rc_car_drives" );
        u.moves -= 50;
        m.i_rem( src, rc_car );
        car_location_string.clear();
        car_location_string << dest.x << ' ' << dest.y << ' ' << dest.z;
        u.set_value( "remote_controlling", car_location_string.str() );
        return;
    }
}

static void pldrive( const tripoint &p )
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
        dbg( D_ERROR ) << "game::pldrive: can't find vehicle!  Drive mode is now off.";
        debugmsg( "game::pldrive error: can't find vehicle!  Drive mode is now off." );
        u.in_vehicle = false;
        return;
    }
    if( !remote ) {
        static const itype_id fuel_type_animal( "animal" );
        const bool has_animal_controls = veh->part_with_feature( part, "CONTROL_ANIMAL", true ) >= 0;
        const bool has_controls = veh->part_with_feature( part, "CONTROLS", true ) >= 0;
        const bool has_animal = veh->has_engine_type( fuel_type_animal, false ) &&
                                veh->has_harnessed_animal();
        if( !has_controls && !has_animal_controls ) {
            add_msg( m_info, _( "You can't drive the vehicle from here.  You need controls!" ) );
            u.controlling_vehicle = false;
            return;
        } else if( !has_controls && has_animal_controls && !has_animal ) {
            add_msg( m_info, _( "You can't drive this vehicle without an animal to pull it." ) );
            u.controlling_vehicle = false;
            return;
        }
    } else {
        if( empty( veh->get_avail_parts( "REMOTE_CONTROLS" ) ) ) {
            add_msg( m_info, _( "Can't drive this vehicle remotely.  It has no working controls." ) );
            return;
        }
    }
    if( p.z != 0 && !g->m.has_zlevels() ) {
        u.add_msg_if_player( m_info, _( "This vehicle doesn't look very airworthy." ) );
        return;
    }
    if( p.z == -1 ) {
        if( veh->check_heli_descend( u ) ) {
            u.add_msg_if_player( m_info, _( "You steer the vehicle into a descent." ) );
        } else {
            return;
        }
    } else if( p.z == 1 ) {
        if( veh->check_heli_ascend( u ) ) {
            u.add_msg_if_player( m_info, _( "You steer the vehicle into an ascent." ) );
        } else {
            return;
        }
    }
    veh->pldrive( p.xy(), p.z );
}

inline static void pldrive( point d )
{
    return pldrive( tripoint( d, 0 ) );
}

static void open()
{
    player &u = g->u;
    map &m = g->m;

    const cata::optional<tripoint> openp_ = choose_adjacent_highlight( _( "Open where?" ),
                                            pgettext( "no door, gate, curtain, etc.", "There is nothing that can be opened nearby." ),
                                            ACTION_OPEN, false );

    if( !openp_ ) {
        return;
    }
    const tripoint openp = *openp_;

    u.moves -= 100;

    if( const optional_vpart_position vp = m.veh_at( openp ) ) {
        vehicle *const veh = &vp->vehicle();
        int openable = veh->next_part_to_open( vp->part_index() );
        if( openable >= 0 ) {
            const vehicle *player_veh = veh_pointer_or_null( m.veh_at( u.pos() ) );
            bool outside = !player_veh || player_veh != veh;
            if( !outside ) {
                if( !veh->handle_potential_theft( dynamic_cast<player &>( g->u ) ) ) {
                    u.moves += 100;
                    return;
                } else {
                    veh->open( openable );
                }
            } else {
                // Outside means we check if there's anything in that tile outside-openable.
                // If there is, we open everything on tile. This means opening a closed,
                // curtained door from outside is possible, but it will magically open the
                // curtains as well.
                int outside_openable = veh->next_part_to_open( vp->part_index(), true );
                if( outside_openable == -1 ) {
                    const std::string name = veh->part_info( openable ).name();
                    add_msg( m_info, _( "That %s can only opened from the inside." ), name );
                    u.moves += 100;
                } else {
                    if( !veh->handle_potential_theft( dynamic_cast<player &>( g->u ) ) ) {
                        u.moves += 100;
                        return;
                    } else {
                        veh->open_all_at( openable );
                    }
                }
            }
        } else {
            // If there are any OPENABLE parts here, they must be already open
            if( const cata::optional<vpart_reference> already_open = vp.part_with_feature( "OPENABLE",
                    true ) ) {
                const std::string name = already_open->info().name();
                add_msg( m_info, _( "That %s is already open." ), name );
            }
            u.moves += 100;
        }
        return;
    }

    bool didit = m.open_door( openp, !m.is_outside( u.pos() ) );

    if( !didit ) {
        const ter_str_id tid = m.ter( openp ).id();

        if( m.has_flag( flag_LOCKED, openp ) ) {
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
    if( const cata::optional<tripoint> pnt = choose_adjacent_highlight( _( "Close where?" ),
            pgettext( "no door, gate, etc.", "There is nothing that can be closed nearby." ),
            ACTION_CLOSE, false ) ) {
        doors::close_door( g->m, g->u, *pnt );
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
    if( veh->last_turn != 0 && rng( 15, 60 ) * 100 < std::abs( veh->velocity ) ) {
        veh->skidding = true;
        add_msg( m_warning, _( "You lose control of %s." ), veh->name );
        veh->turn( veh->last_turn > 0 ? 60 : -60 );
    } else {
        int braking_power = std::abs( veh->velocity ) / 2 + 10 * 100;
        if( std::abs( veh->velocity ) < braking_power ) {
            veh->stop();
        } else {
            int sgn = veh->velocity > 0 ? 1 : -1;
            veh->velocity = sgn * ( std::abs( veh->velocity ) - braking_power );
        }
    }
    g->u.moves = 0;
}

// Establish or release a grab on a vehicle
static void grab()
{
    avatar &you = g->u;
    map &m = g->m;

    if( you.get_grab_type() != OBJECT_NONE ) {
        if( const optional_vpart_position vp = m.veh_at( you.pos() + you.grab_point ) ) {
            add_msg( _( "You release the %s." ), vp->vehicle().name );
        } else if( m.has_furn( you.pos() + you.grab_point ) ) {
            add_msg( _( "You release the %s." ), m.furnname( you.pos() + you.grab_point ) );
        }

        you.grab( OBJECT_NONE );
        return;
    }

    const cata::optional<tripoint> grabp_ = choose_adjacent( _( "Grab where?" ) );
    if( !grabp_ ) {
        add_msg( _( "Never mind." ) );
        return;
    }
    const tripoint grabp = *grabp_;

    if( grabp == you.pos() ) {
        add_msg( _( "You get a hold of yourself." ) );
        you.grab( OBJECT_NONE );
        return;
    }
    if( const optional_vpart_position vp = m.veh_at( grabp ) ) {
        if( !vp->vehicle().handle_potential_theft( dynamic_cast<player &>( g->u ) ) ) {
            return;
        }
        you.grab( OBJECT_VEHICLE, grabp - you.pos() );
        add_msg( _( "You grab the %s." ), vp->vehicle().name );
    } else if( m.has_furn( grabp ) ) { // If not, grab furniture if present
        if( !m.furn( grabp ).obj().is_movable() ) {
            add_msg( _( "You can not grab the %s" ), m.furnname( grabp ) );
            return;
        }
        you.grab( OBJECT_FURNITURE, grabp - you.pos() );
        if( !m.can_move_furniture( grabp, &you ) ) {
            add_msg( _( "You grab the %s. It feels really heavy." ), m.furnname( grabp ) );
        } else {
            add_msg( _( "You grab the %s." ), m.furnname( grabp ) );
        }
    } else { // TODO: grab mob? Captured squirrel = pet (or meat that stays fresh longer).
        add_msg( m_info, _( "There's nothing to grab there!" ) );
    }
}

static void haul()
{
    player &u = g->u;
    map &m = g->m;

    if( u.is_hauling() ) {
        u.stop_hauling();
    } else {
        if( m.veh_at( u.pos() ) ) {
            add_msg( m_info, _( "You cannot haul inside vehicles." ) );
        } else if( m.has_flag( TFLAG_DEEP_WATER, u.pos() ) ) {
            add_msg( m_info, _( "You cannot haul while in deep water." ) );
        } else if( !m.can_put_items( u.pos() ) ) {
            add_msg( m_info, _( "You cannot haul items here." ) );
        } else if( !m.has_items( u.pos() ) ) {
            add_msg( m_info, _( "There are no items to haul here." ) );
        } else {
            u.start_hauling();
        }
    }
}

static void smash()
{
    player &u = g->u;
    map &m = g->m;
    if( u.is_mounted() ) {
        auto mons = u.mounted_creature.get();
        if( mons->has_flag( MF_RIDEABLE_MECH ) ) {
            if( !mons->check_mech_powered() ) {
                add_msg( m_bad, _( "Your %s refuses to move as its batteries have been drained." ),
                         mons->get_name() );
                return;
            }
        }
    }
    const int move_cost = !u.is_armed() ? 80 : u.weapon.attack_time() * 0.8;
    bool didit = false;
    bool mech_smash = false;
    int smashskill;
    ///\EFFECT_STR increases smashing capability
    if( u.is_mounted() ) {
        auto mon = u.mounted_creature.get();
        smashskill = u.str_cur + mon->mech_str_addition() + mon->type->melee_dice *
                     mon->type->melee_sides;
        mech_smash = true;
    } else {
        smashskill = u.str_cur + u.weapon.damage_melee( DT_BASH );
    }

    const bool allow_floor_bash = debug_mode; // Should later become "true"
    const cata::optional<tripoint> smashp_ = choose_adjacent( _( "Smash where?" ), allow_floor_bash );
    if( !smashp_ ) {
        return;
    }
    tripoint smashp = *smashp_;

    bool smash_floor = false;
    if( smashp.z != u.posz() ) {
        if( smashp.z > u.posz() ) {
            // TODO: Knock on the ceiling
            return;
        }

        smashp.z = u.posz();
        smash_floor = true;
    }
    if( u.is_mounted() ) {
        monster *crit = u.mounted_creature.get();
        if( crit->has_flag( MF_RIDEABLE_MECH ) ) {
            crit->use_mech_power( -3 );
        }
    }
    for( std::pair<const field_type_id, field_entry> &fd_to_smsh : m.field_at( smashp ) ) {
        const map_bash_info &bash_info = fd_to_smsh.first->bash_info;
        if( bash_info.str_min == -1 ) {
            continue;
        }
        if( smashskill < bash_info.str_min && one_in( 10 ) ) {
            add_msg( m_neutral, _( "You don't seem to be damaging the %s." ), fd_to_smsh.first->get_name() );
            return;
        } else if( smashskill >= rng( bash_info.str_min, bash_info.str_max ) ) {
            sounds::sound( smashp, bash_info.sound_vol, sounds::sound_t::combat, bash_info.sound, true, "smash",
                           "field" );
            m.remove_field( smashp, fd_to_smsh.first );
            m.spawn_items( smashp, item_group::items_from( bash_info.drop_group, calendar::turn ) );
            u.mod_moves( - bash_info.fd_bash_move_cost );
            add_msg( m_info, bash_info.field_bash_msg_success.translated() );
            return;
        } else {
            sounds::sound( smashp, bash_info.sound_fail_vol, sounds::sound_t::combat, bash_info.sound_fail,
                           true, "smash",
                           "field" );
            return;
        }
    }

    bool should_pulp = false;
    for( const item &it : m.i_at( smashp ) ) {
        if( it.is_corpse() && it.damage() < it.max_damage() && it.can_revive() ) {
            if( it.get_mtype()->bloodType()->has_acid ) {
                if( query_yn( _( "Are you sure you want to pulp an acid filled corpse?" ) ) ) {
                    should_pulp = true;
                    break; // Don't prompt for the same thing multiple times
                } else {
                    return; // Player doesn't want an acid bath
                }
            }
            should_pulp = true; // There is at least one corpse to pulp
        }
    }

    if( should_pulp ) {
        // do activity forever. ACT_PULP stops itself
        u.assign_activity( ACT_PULP, calendar::INDEFINITELY_LONG, 0 );
        u.activity.placement = m.getabs( smashp );
        return; // don't smash terrain if we've smashed a corpse
    }

    vehicle *veh = veh_pointer_or_null( g->m.veh_at( smashp ) );
    if( veh != nullptr ) {
        if( !veh->handle_potential_theft( dynamic_cast<player &>( g->u ) ) ) {
            return;
        }
    }
    didit = m.bash( smashp, smashskill, false, false, smash_floor ).did_bash;
    if( didit ) {
        if( !mech_smash ) {
            u.handle_melee_wear( u.weapon );
            const int mod_sta = ( ( u.weapon.weight() / 10_gram ) + 200 + static_cast<int>
                                  ( get_option<float>( "PLAYER_BASE_STAMINA_REGEN_RATE" ) ) ) * -1;
            u.mod_stamina( mod_sta );
            if( u.get_skill_level( skill_melee ) == 0 ) {
                u.practice( skill_melee, rng( 0, 1 ) * rng( 0, 1 ) );
            }
            const int vol = u.weapon.volume() / units::legacy_volume_factor;
            if( u.weapon.made_of( material_id( "glass" ) ) &&
                rng( 0, vol + 3 ) < vol ) {
                add_msg( m_bad, _( "Your %s shatters!" ), u.weapon.tname() );
                u.weapon.spill_contents( u.pos() );
                sounds::sound( u.pos(), 24, sounds::sound_t::combat, "CRACK!", true, "smash", "glass" );
                u.deal_damage( nullptr, bodypart_id( "hand_r" ), damage_instance( DT_CUT, rng( 0, vol ) ) );
                if( vol > 20 ) {
                    // Hurt left arm too, if it was big
                    u.deal_damage( nullptr, bodypart_id( "hand_l" ), damage_instance( DT_CUT, rng( 0,
                                   static_cast<int>( vol * .5 ) ) ) );
                }
                u.remove_weapon();
                u.check_dead_state();
            }
        }
        u.moves -= move_cost;

        if( smashskill < m.bash_resistance( smashp ) && one_in( 10 ) ) {
            if( m.has_furn( smashp ) && m.furn( smashp ).obj().bash.str_min != -1 ) {
                // %s is the smashed furniture
                add_msg( m_neutral, _( "You don't seem to be damaging the %s." ), m.furnname( smashp ) );
            } else {
                // %s is the smashed terrain
                add_msg( m_neutral, _( "You don't seem to be damaging the %s." ), m.tername( smashp ) );
            }
        }
    } else {
        add_msg( _( "There's nothing there to smash!" ) );
    }
}

static int try_set_alarm()
{
    uilist as_m;
    const bool already_set = g->u.has_effect( effect_alarm_clock );

    as_m.text = already_set ?
                _( "You already have an alarm set.  What do you want to do?" ) :
                _( "You have an alarm clock.  What do you want to do?" );

    as_m.entries.emplace_back( 0, true, 'w', already_set ?
                               _( "Keep the alarm and wait a while" ) :
                               _( "Wait a while" ) );
    as_m.entries.emplace_back( 1, true, 'a', already_set ?
                               _( "Change your alarm" ) :
                               _( "Set an alarm for later" ) );
    as_m.query();

    return as_m.ret;
}

static void wait()
{
    std::map<int, time_duration> durations;
    uilist as_m;
    player &u = g->u;
    bool setting_alarm = false;

    if( u.controlling_vehicle && ( g->m.veh_at( u.pos() )->vehicle().velocity ||
                                   g->m.veh_at( u.pos() )->vehicle().cruise_velocity ) ) {
        popup( _( "You can't pass time while controlling a moving vehicle." ) );
        return;
    }

    if( u.has_alarm_clock() ) {
        int alarm_query = try_set_alarm();
        if( alarm_query == UILIST_CANCEL ) {
            return;
        }
        setting_alarm = alarm_query == 1;
    }

    const bool has_watch = u.has_watch() || setting_alarm;

    const auto add_menu_item = [ &as_m, &durations, has_watch ]
                               ( int retval, int hotkey, const std::string &caption = "",
    const time_duration &duration = time_duration::from_turns( calendar::INDEFINITELY_LONG ) ) {

        std::string text( caption );

        if( has_watch && duration != time_duration::from_turns( calendar::INDEFINITELY_LONG ) ) {
            const std::string dur_str( to_string( duration ) );
            text += ( text.empty() ? dur_str : string_format( " (%s)", dur_str ) );
        }
        as_m.addentry( retval, true, hotkey, text );
        durations.emplace( retval, duration );
    };

    if( setting_alarm ) {

        add_menu_item( 0, '0', "", 30_minutes );

        for( int i = 1; i <= 9; ++i ) {
            add_menu_item( i, '0' + i, "", i * 1_hours );
        }

    } else {
        if( g->u.get_stamina() < g->u.get_stamina_max() ) {
            as_m.addentry( 12, true, 'w', _( "Wait until you catch your breath" ) );
            durations.emplace( 12, 15_minutes ); // to hide it from showing
        }
        add_menu_item( 1, '1', "", 5_minutes );
        add_menu_item( 2, '2', "", 30_minutes );
        add_menu_item( 3, '3', "", 1_hours );
        add_menu_item( 4, '4', "", 2_hours );
        add_menu_item( 5, '5', "", 3_hours );
        add_menu_item( 6, '6', "", 6_hours );
        as_m.addentry( 13, true, 'c', _( "Custom input" ) );
    }

    if( g->get_levz() >= 0 || has_watch ) {
        const time_point last_midnight = calendar::turn - time_past_midnight( calendar::turn );
        const auto diurnal_time_before = []( const time_point & p ) {
            // Either the given time is in the future (e.g. waiting for sunset while it's early morning),
            // than use it directly. Otherwise (in the past), add a single day to get the same time tomorrow
            // (e.g. waiting for sunrise while it's noon).
            const time_point target_time = p > calendar::turn ? p : p + 1_days;
            return target_time - calendar::turn;
        };

        add_menu_item( 7,  'd',
                       setting_alarm ? _( "Set alarm for dawn" ) : _( "Wait till daylight" ),
                       diurnal_time_before( to_turns<int>( daylight_time( calendar::turn ) - calendar::turn_zero ) ) );
        add_menu_item( 8,  'n',
                       setting_alarm ? _( "Set alarm for noon" ) : _( "Wait till noon" ),
                       diurnal_time_before( last_midnight + 12_hours ) );
        add_menu_item( 9,  'k',
                       setting_alarm ? _( "Set alarm for dusk" ) : _( "Wait till night" ),
                       diurnal_time_before( to_turns<int>( night_time( calendar::turn ) - calendar::turn_zero ) ) );
        add_menu_item( 10, 'm',
                       setting_alarm ? _( "Set alarm for midnight" ) : _( "Wait till midnight" ),
                       diurnal_time_before( last_midnight + 0_hours ) );
        if( setting_alarm ) {
            if( u.has_effect( effect_alarm_clock ) ) {
                add_menu_item( 11, 'x', _( "Cancel the currently set alarm." ),
                               0_turns );
            }
        } else {
            add_menu_item( 11, 'W', _( "Wait till weather changes" ) );
        }
    }

    as_m.text = ( has_watch ) ? string_format( _( "It's %s now. " ),
                to_string_time_of_day( calendar::turn ) ) : "";
    as_m.text += setting_alarm ? _( "Set alarm for when?" ) : _( "Wait for how long?" );
    as_m.query(); /* calculate key and window variables, generate window, and loop until we get a valid answer */

    time_duration time_to_wait;
    if( as_m.ret == 13 ) {
        int minutes = string_input_popup()
                      .title( _( "How long? (in minutes)" ) )
                      .identifier( "wait_duration" )
                      .query_int();
        time_to_wait = minutes * 1_minutes;
    } else {

        const auto dur_iter = durations.find( as_m.ret );
        if( dur_iter == durations.end() ) {
            return;
        }
        time_to_wait = dur_iter->second;
    }

    if( setting_alarm ) {
        // Setting alarm
        u.remove_effect( effect_alarm_clock );
        if( as_m.ret == 11 ) {
            add_msg( _( "You cancel your alarm." ) );
        } else {
            u.add_effect( effect_alarm_clock, time_to_wait );
            add_msg( _( "You set your alarm." ) );
        }

    } else {
        // Waiting
        activity_id actType;
        if( as_m.ret == 11 ) {
            actType = ACT_WAIT_WEATHER;
        } else if( as_m.ret == 12 ) {
            actType = ACT_WAIT_STAMINA;
        } else {
            actType = ACT_WAIT;
        }

        player_activity new_act( actType, 100 * ( to_turns<int>( time_to_wait ) - 1 ), 0 );

        u.assign_activity( new_act, false );
    }
}

static void sleep()
{
    player &u = g->u;
    if( u.is_mounted() ) {
        u.add_msg_if_player( m_info, _( "You cannot sleep while mounted." ) );
        return;
    }
    uilist as_m;
    as_m.text = _( "<color_white>Are you sure you want to sleep?</color>" );
    // (Y)es/(S)ave before sleeping/(N)o
    as_m.entries.emplace_back( 0, true,
                               get_option<bool>( "FORCE_CAPITAL_YN" ) ? 'Y' : 'y',
                               _( "Yes." ) );
    as_m.entries.emplace_back( 1, g->get_moves_since_last_save(),
                               get_option<bool>( "FORCE_CAPITAL_YN" ) ? 'S' : 's',
                               _( "Yes, and save game before sleeping." ) );
    as_m.entries.emplace_back( 2, true,
                               get_option<bool>( "FORCE_CAPITAL_YN" ) ? 'N' : 'n',
                               _( "No." ) );

    // List all active items, bionics or mutations so player can deactivate them
    std::vector<std::string> active;
    for( auto &it : u.inv_dump() ) {
        if( it->has_flag( flag_LITCIG ) ||
            ( it->active && ( it->charges > 0 || it->units_remaining( u ) > 0 ) && it->is_tool() &&
              !it->has_flag( flag_SLEEP_IGNORE ) ) ) {
            active.push_back( it->tname() );
        }
    }
    for( int i = 0; i < g->u.num_bionics(); i++ ) {
        const bionic &bio = u.bionic_at_index( i );
        if( !bio.powered ) {
            continue;
        }

        // some bionics
        // bio_alarm is useful for waking up during sleeping
        // turning off bio_leukocyte has 'unpleasant side effects'
        if( bio.info().sleep_friendly ) {
            continue;
        }

        const auto &info = bio.info();
        if( info.power_over_time > 0_kJ ) {
            active.push_back( info.name.translated() );
        }
    }
    for( auto &mut : u.get_mutations() ) {
        const auto &mdata = mut.obj();
        if( mdata.cost > 0 && u.has_active_mutation( mut ) ) {
            active.push_back( mdata.name() );
        }
    }

    // check for deactivating any currently played music instrument.
    for( auto &item : u.inv_dump() ) {
        if( item->active && item->get_use( "musical_instrument" ) != nullptr ) {
            u.add_msg_if_player( _( "You stop playing your %s before trying to sleep." ), item->tname() );
            // deactivate instrument
            item->active = false;
        }
    }

    // ask for deactivation
    std::stringstream data;
    if( !active.empty() ) {
        as_m.selected = 2;
        data << as_m.text << std::endl;
        data << _( "You may want to extinguish or turn off:" ) << std::endl;
        data << " " << std::endl;
        for( auto &a : active ) {
            data << "<color_red>" << a << "</color>" << std::endl;
        }
        as_m.text = data.str();
    }

    /* Calculate key and window variables, generate window,
       and loop until we get a valid answer. */
    as_m.query();

    if( as_m.ret == 1 ) {
        g->quicksave();
    } else if( as_m.ret == 2 || as_m.ret < 0 ) {
        return;
    }

    time_duration try_sleep_dur = 24_hours;
    std::string deaf_text;
    if( g->u.is_deaf() ) {
        deaf_text = _( "<color_c_red> (DEAF!)</color>" );
    }
    if( u.has_alarm_clock() ) {
        /* Reuse menu to ask player whether they want to set an alarm. */
        bool can_hibernate = u.get_hunger() < -60 && u.has_active_mutation( trait_HIBERNATE );

        as_m.reset();
        as_m.text = can_hibernate ?
                    _( "You're engorged to hibernate.  The alarm would only attract attention.  "
                       "Set an alarm anyway?" ) :
                    _( "You have an alarm clock.  Set an alarm?" );
        as_m.text += deaf_text;

        as_m.entries.emplace_back( 0, true,
                                   get_option<bool>( "FORCE_CAPITAL_YN" ) ? 'N' : 'n',
                                   _( "No, don't set an alarm." ) );

        for( int i = 3; i <= 9; ++i ) {
            as_m.entries.emplace_back( i, true, '0' + i,
                                       string_format( _( "Set alarm to wake up in %i hours." ), i ) + deaf_text );
        }

        as_m.query();
        if( as_m.ret >= 3 && as_m.ret <= 9 ) {
            u.add_effect( effect_alarm_clock, 1_hours * as_m.ret );
            try_sleep_dur = 1_hours * as_m.ret + 1_turns;
        } else if( as_m.ret < 0 ) {
            return;
        }
    }

    u.moves = 0;
    u.try_to_sleep( try_sleep_dur );
}

static void loot()
{
    enum ZoneFlags {
        None = 1,
        SortLoot = 2,
        FertilizePlots = 16,
        ConstructPlots = 64,
        MultiFarmPlots = 128,
        Multichoptrees = 256,
        Multichopplanks = 512,
        Multideconvehicle = 1024,
        Multirepairvehicle = 2048,
        MultiButchery = 4096,
        MultiMining = 8192
    };

    player &u = g->u;
    int flags = 0;
    auto &mgr = zone_manager::get_manager();
    const bool has_fertilizer = u.has_item_with_flag( "FERTILIZER" );

    // Manually update vehicle cache.
    // In theory this would be handled by the related activity (activity_on_turn_move_loot())
    // but with a stale cache we never get that far.
    mgr.cache_vzones();

    flags |= g->check_near_zone( zone_type_id( "LOOT_UNSORTED" ), u.pos() ) ? SortLoot : 0;
    if( g->check_near_zone( zone_type_id( "FARM_PLOT" ), u.pos() ) ) {
        flags |= FertilizePlots;
        flags |= MultiFarmPlots;
    }
    flags |= g->check_near_zone( zone_type_id( "CONSTRUCTION_BLUEPRINT" ),
                                 u.pos() ) ? ConstructPlots : 0;

    flags |= g->check_near_zone( zone_type_id( "CHOP_TREES" ), u.pos() ) ? Multichoptrees : 0;
    flags |= g->check_near_zone( zone_type_id( "LOOT_WOOD" ), u.pos() ) ? Multichopplanks : 0;
    flags |= g->check_near_zone( zone_type_id( "VEHICLE_DECONSTRUCT" ),
                                 u.pos() ) ? Multideconvehicle : 0;
    flags |= g->check_near_zone( zone_type_id( "VEHICLE_REPAIR" ), u.pos() ) ? Multirepairvehicle : 0;
    flags |= g->check_near_zone( zone_type_id( "LOOT_CORPSE" ), u.pos() ) ? MultiButchery : 0;
    flags |= g->check_near_zone( zone_type_id( "MINING" ), u.pos() ) ? MultiMining : 0;
    if( flags == 0 ) {
        add_msg( m_info, _( "There is no compatible zone nearby." ) );
        add_msg( m_info, _( "Compatible zones are %s and %s" ),
                 mgr.get_name_from_type( zone_type_id( "LOOT_UNSORTED" ) ),
                 mgr.get_name_from_type( zone_type_id( "FARM_PLOT" ) ) );
        return;
    }

    uilist menu;
    menu.text = _( "Pick action:" );
    menu.desc_enabled = true;

    if( flags & SortLoot ) {
        menu.addentry_desc( SortLoot, true, 'o', _( "Sort out my loot" ),
                            _( "Sorts out the loot from Loot: Unsorted zone to nearby appropriate Loot zones.  Uses empty space in your inventory or utilizes a cart, if you are holding one." ) );
    }

    if( flags & FertilizePlots ) {
        menu.addentry_desc( FertilizePlots, has_fertilizer, 'f',
                            !has_fertilizer ? _( "Fertilize plots… you don't have any fertilizer" ) : _( "Fertilize plots" ),
                            _( "Fertilize any nearby Farm: Plot zones." ) );
    }

    if( flags & ConstructPlots ) {
        menu.addentry_desc( ConstructPlots, true, 'c', _( "Construct plots" ),
                            _( "Work on any nearby Blueprint: construction zones." ) );
    }
    if( flags & MultiFarmPlots ) {
        menu.addentry_desc( MultiFarmPlots, true, 'm', _( "Farm plots" ),
                            _( "Till and plant on any nearby farm plots - auto-fetch seeds and tools." ) );
    }
    if( flags & Multichoptrees ) {
        menu.addentry_desc( Multichoptrees, true, 'C', _( "Chop trees" ),
                            _( "Chop down any trees in the designated zone - auto-fetch tools." ) );
    }
    if( flags & Multichopplanks ) {
        menu.addentry_desc( Multichopplanks, true, 'P', _( "Chop planks" ),
                            _( "Auto-chop logs in wood loot zones into planks - auto-fetch tools." ) );
    }
    if( flags & Multideconvehicle ) {
        menu.addentry_desc( Multideconvehicle, true, 'v', _( "Deconstruct vehicle" ),
                            _( "Auto-deconstruct vehicle in designated zone - auto-fetch tools." ) );
    }
    if( flags & Multirepairvehicle ) {
        menu.addentry_desc( Multirepairvehicle, true, 'V', _( "Repair vehicle" ),
                            _( "Auto-repair vehicle in designated zone - auto-fetch tools." ) );
    }
    if( flags & MultiButchery ) {
        menu.addentry_desc( MultiButchery, true, 'B', _( "Butcher corpses" ),
                            _( "Auto-butcher anything in corpse loot zones - auto-fetch tools." ) );
    }
    if( flags & MultiMining ) {
        menu.addentry_desc( MultiMining, true, 'M', _( "Mine Area" ),
                            _( "Auto-mine anything in mining zone - auto-fetch tools." ) );
    }

    menu.query();
    flags = ( menu.ret >= 0 ) ? menu.ret : None;

    switch( flags ) {
        case None:
            add_msg( _( "Never mind." ) );
            break;
        case SortLoot:
            u.assign_activity( ACT_MOVE_LOOT );
            break;
        case FertilizePlots:
            u.assign_activity( ACT_FERTILIZE_PLOT );
            break;
        case ConstructPlots:
            u.assign_activity( ACT_MULTIPLE_CONSTRUCTION );
            break;
        case MultiFarmPlots:
            u.assign_activity( ACT_MULTIPLE_FARM );
            break;
        case Multichoptrees:
            u.assign_activity( ACT_MULTIPLE_CHOP_TREES );
            break;
        case Multichopplanks:
            u.assign_activity( ACT_MULTIPLE_CHOP_PLANKS );
            break;
        case Multideconvehicle:
            u.assign_activity( ACT_VEHICLE_DECONSTRUCTION );
            break;
        case Multirepairvehicle:
            u.assign_activity( ACT_VEHICLE_REPAIR );
            break;
        case MultiButchery:
            u.assign_activity( ACT_MULTIPLE_BUTCHER );
            break;
        case MultiMining:
            u.assign_activity( ACT_MULTIPLE_MINE );
            break;
        default:
            debugmsg( "Unsupported flag" );
            break;
    }
}

static void wear()
{
    avatar &u = g->u;
    item_location loc = game_menus::inv::wear( u );

    if( loc ) {
        u.wear( *loc.obtain( u ) );
    } else {
        add_msg( _( "Never mind." ) );
    }
}

static void takeoff()
{
    avatar &u = g->u;
    item_location loc = game_menus::inv::take_off( u );

    if( loc ) {
        u.takeoff( *loc.obtain( u ) );
    } else {
        add_msg( _( "Never mind." ) );
    }
}

static void read()
{
    avatar &u = g->u;
    // Can read items from inventory or within one tile (including in vehicles)
    item_location loc = game_menus::inv::read( u );

    if( loc ) {
        if( loc->type->can_use( "learn_spell" ) ) {
            item spell_book = *loc.get_item();
            spell_book.get_use( "learn_spell" )->call( u, spell_book, spell_book.active, u.pos() );
        } else {
            u.read( loc );
        }
    } else {
        add_msg( _( "Never mind." ) );
    }
}

// Perform a reach attach
// range - the range of the current weapon.
// u - player
static void reach_attack( int range, player &u )
{
    g->temp_exit_fullscreen();
    g->m.draw( g->w_terrain, u.pos() );
    std::vector<tripoint> trajectory = target_handler().target_ui( u, TARGET_MODE_REACH, &u.weapon,
                                       range );
    if( !trajectory.empty() ) {
        u.reach_attack( trajectory.back() );
    }
    g->draw_ter();
    wrefresh( g->w_terrain );
    g->draw_panels();
    g->reenter_fullscreen();
}

static void fire()
{
    player &u = g->u;

    // Use vehicle turret or draw a pistol from a holster if unarmed
    if( !u.is_armed() ) {

        const optional_vpart_position vp = g->m.veh_at( u.pos() );

        turret_data turret;
        if( vp && ( turret = vp->vehicle().turret_query( u.pos() ) ) ) {
            avatar_action::fire_turret_manual( g->u, g->m, turret );
            return;
        }

        if( vp.part_with_feature( "CONTROLS", true ) ) {
            if( vp->vehicle().turrets_aim_and_fire_all_manual() ) {
                return;
            }
        }

        std::vector<std::string> options;
        std::vector<std::function<void()>> actions;

        for( auto &w : u.worn ) {
            if( w.type->can_use( "holster" ) && !w.has_flag( flag_NO_QUICKDRAW ) &&
                !w.contents.empty() && w.contents.front().is_gun() ) {
                //~ draw (first) gun contained in holster
                //~ %1$s: weapon name, %2$s: container name, %3$d: remaining ammo count
                options.push_back( string_format( pgettext( "holster", "%1$s from %2$s (%3$d)" ),
                                                  w.contents.front().tname(),
                                                  w.type_name(),
                                                  w.contents.front().ammo_remaining() ) );

                actions.emplace_back( [&] { u.invoke_item( &w, "holster" ); } );

            } else if( w.is_gun() && w.gunmod_find( "shoulder_strap" ) ) {
                // wield item currently worn using shoulder strap
                options.push_back( w.display_name() );
                actions.emplace_back( [&] { u.wield( w ); } );
            }
        }
        if( !options.empty() ) {
            int sel = uilist( _( "Draw what?" ), options );
            if( sel >= 0 ) {
                actions[sel]();
            }
        }
    }

    if( u.weapon.is_gun() && !u.weapon.gun_current_mode().melee() ) {
        avatar_action::fire_wielded_weapon( g->u, g->m );
    } else if( u.weapon.has_flag( flag_REACH_ATTACK ) ) {
        int range = u.weapon.has_flag( flag_REACH3 ) ? 3 : 2;
        if( u.has_effect( effect_relax_gas ) ) {
            if( one_in( 8 ) ) {
                add_msg( m_good, _( "Your willpower asserts itself, and so do you!" ) );
                reach_attack( range, u );
            } else {
                u.moves -= rng( 2, 8 ) * 10;
                add_msg( m_bad, _( "You're too pacified to strike anything…" ) );
            }
        } else {
            reach_attack( range, u );
        }
    } else if( u.weapon.is_gun() && u.weapon.gun_current_mode().flags.count( flag_REACH_ATTACK ) ) {
        int range = u.weapon.gun_current_mode().qty;
        if( u.has_effect( effect_relax_gas ) ) {
            if( one_in( 8 ) ) {
                add_msg( m_good, _( "Your willpower asserts itself, and so do you!" ) );
                reach_attack( range, u );
            } else {
                u.moves -= rng( 2, 8 ) * 10;
                add_msg( m_bad, _( "You're too pacified to strike anything…" ) );
            }
        } else {
            reach_attack( range, u );
        }
    }
}

static void open_movement_mode_menu()
{
    avatar &u = g->u;
    uilist as_m;

    as_m.text = _( "Change to which movement mode?" );

    as_m.entries.emplace_back( CMM_RUN, true, 'r', _( "Run" ) );
    as_m.entries.emplace_back( CMM_WALK, true, 'w', _( "Walk" ) );
    as_m.entries.emplace_back( CMM_CROUCH, true, 'c', _( "Crouch" ) );
    as_m.entries.emplace_back( CMM_COUNT, true, '"', _( "Cycle move mode (run/walk/crouch)" ) );
    as_m.selected = 1;
    as_m.query();

    if( as_m.ret != UILIST_CANCEL ) {
        if( as_m.ret == CMM_COUNT ) {
            u.cycle_move_mode();
        } else {
            u.set_movement_mode( static_cast<character_movemode>( as_m.ret ) );
        }
    }
}

static void cast_spell()
{
    player &u = g->u;

    std::vector<spell_id> spells = u.magic.spells();

    if( spells.empty() ) {
        add_msg( game_message_params{ m_bad, gmf_bypass_cooldown },
                 _( "You don't know any spells to cast." ) );
        return;
    }

    bool can_cast_spells = false;
    for( spell_id sp : spells ) {
        spell temp_spell = u.magic.get_spell( sp );
        if( temp_spell.can_cast( u ) ) {
            can_cast_spells = true;
        }
    }

    if( !can_cast_spells ) {
        add_msg( game_message_params{ m_bad, gmf_bypass_cooldown },
                 _( "You can't cast any of the spells you know!" ) );
    }

    const int spell_index = u.magic.select_spell( u );
    if( spell_index < 0 ) {
        return;
    }

    spell &sp = *u.magic.get_spells()[spell_index];

    if( u.is_armed() && !sp.has_flag( spell_flag::NO_HANDS ) &&
        !u.weapon.has_flag( flag_MAGIC_FOCUS ) ) {
        add_msg( game_message_params{ m_bad, gmf_bypass_cooldown },
                 _( "You need your hands free to cast this spell!" ) );
        return;
    }

    if( !u.magic.has_enough_energy( u, sp ) ) {
        add_msg( game_message_params{ m_bad, gmf_bypass_cooldown },
                 _( "You don't have enough %s to cast the spell." ),
                 sp.energy_string() );
        return;
    }

    if( sp.energy_source() == hp_energy && !u.has_quality( qual_CUT ) ) {
        add_msg( game_message_params{ m_bad, gmf_bypass_cooldown },
                 _( "You cannot cast Blood Magic without a cutting implement." ) );
        return;
    }

    player_activity cast_spell( ACT_SPELLCASTING, sp.casting_time( u ) );
    // [0] this is used as a spell level override for items casting spells
    cast_spell.values.emplace_back( -1 );
    // [1] if this value is 1, the spell never fails
    cast_spell.values.emplace_back( 0 );
    // [2] this value overrides the mana cost if set to 0
    cast_spell.values.emplace_back( 1 );
    cast_spell.name = sp.id().c_str();
    if( u.magic.casting_ignore ) {
        const std::vector<distraction_type> ignored_distractions = {
            distraction_type::noise,
            distraction_type::pain,
            distraction_type::attacked,
            distraction_type::hostile_spotted_near,
            distraction_type::hostile_spotted_far,
            distraction_type::talked_to,
            distraction_type::asthma,
            distraction_type::motion_alarm,
            distraction_type::weather_change
        };
        for( const distraction_type ignored : ignored_distractions ) {
            cast_spell.ignore_distraction( ignored );
        }
    }
    u.assign_activity( cast_spell, false );
}

void game::open_consume_item_menu()
{
    uilist as_m;

    as_m.text = _( "What do you want to consume?" );

    as_m.entries.emplace_back( 0, true, 'f', _( "Food" ) );
    as_m.entries.emplace_back( 1, true, 'd', _( "Drink" ) );
    as_m.entries.emplace_back( 2, true, 'm', _( "Medication" ) );
    as_m.query();

    switch( as_m.ret ) {
        case 0:
            avatar_action::eat( u, game_menus::inv::consume_food( u ) );
            break;
        case 1:
            avatar_action::eat( u, game_menus::inv::consume_drink( u ) );
            break;
        case 2:
            avatar_action::eat( u, game_menus::inv::consume_meds( u ) );
            break;
        default:
            break;
    }
}

bool game::handle_action()
{
    std::string action;
    input_context ctxt;
    action_id act = ACTION_NULL;
    user_turn current_turn;
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
    cata::optional<tripoint> mouse_target;

    if( uquit == QUIT_WATCH && action == "QUIT" ) {
        uquit = QUIT_DIED;
        return false;
    }

    if( act == ACTION_NULL ) {
        act = look_up_action( action );

        if( act == ACTION_KEYBINDINGS ) {
            // already handled by input context
            refresh_all();
            return false;
        }

        if( act == ACTION_MAIN_MENU ) {
            if( uquit == QUIT_WATCH ) {
                return false;
            }
            // No auto-move actions have or can be set at this point.
            u.clear_destination();
            destination_preview.clear();
            act = handle_main_menu();
            if( act == ACTION_NULL ) {
                return false;
            }
        }

        if( act == ACTION_ACTIONMENU ) {
            if( uquit == QUIT_WATCH ) {
                return false;
            }
            // No auto-move actions have or can be set at this point.
            u.clear_destination();
            destination_preview.clear();
            act = handle_action_menu();
            if( act == ACTION_NULL ) {
                return false;
            }
#if defined(__ANDROID__)
            if( get_option<bool>( "ANDROID_ACTIONMENU_AUTOADD" ) && ctxt.get_category() == "DEFAULTMODE" ) {
                add_best_key_for_action_to_quick_shortcuts( act, ctxt.get_category(), false );
            }
#endif
        }

        if( act == ACTION_KEYBINDINGS ) {
            u.clear_destination();
            destination_preview.clear();
            act = ctxt.display_menu( true );
            if( act == ACTION_NULL ) {
                return false;
            }
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

            const cata::optional<tripoint> mouse_pos = ctxt.get_coordinates( w_terrain );
            if( !mouse_pos ) {
                return false;
            } else if( !u.sees( *mouse_pos ) ) {
                // Not clicked in visible terrain
                return false;
            }
            mouse_target = mouse_pos;

            if( act == ACTION_SELECT ) {
                // Note: The following has the potential side effect of
                // setting auto-move destination state in addition to setting
                // act.
                if( !try_get_left_click_action( act, *mouse_target ) ) {
                    return false;
                }
            } else if( act == ACTION_SEC_SELECT ) {
                if( !try_get_right_click_action( act, *mouse_target ) ) {
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
            const int ch = evt.get_first_input();
            const std::string &&name = inp_mngr.get_keyname( ch, evt.type, true );
            if( !get_option<bool>( "NO_UNKNOWN_COMMAND_MSG" ) ) {
                add_msg( m_info, _( "Unknown command: \"%s\" (%ld)" ), name, ch );
                if( const cata::optional<std::string> hint =
                        press_x_if_bound( ACTION_KEYBINDINGS ) ) {
                    add_msg( m_info, _( "%s at any time to see and edit keybindings relevant to "
                                        "the current context." ),
                             *hint );
                }
            }
        }
        return false;
    }

    // This has no action unless we're in a special game mode.
    gamemode->pre_action( act );

    int soffset = get_option<int>( "MOVE_VIEW_OFFSET" );

    int before_action_moves = u.moves;

    // These actions are allowed while deathcam is active. Registered in game::get_player_input
    if( uquit == QUIT_WATCH || !u.is_dead_state() ) {
        switch( act ) {
            case ACTION_TOGGLE_MAP_MEMORY:
                u.toggle_map_memory();
                break;

            case ACTION_CENTER:
                u.view_offset.x = driving_view_offset.x;
                u.view_offset.y = driving_view_offset.y;
                break;

            case ACTION_SHIFT_N:
            case ACTION_SHIFT_NE:
            case ACTION_SHIFT_E:
            case ACTION_SHIFT_SE:
            case ACTION_SHIFT_S:
            case ACTION_SHIFT_SW:
            case ACTION_SHIFT_W:
            case ACTION_SHIFT_NW: {
                static const std::map<action_id, std::pair<point, point>> shift_delta = {
                    { ACTION_SHIFT_N, { point_north, point_north_east } },
                    { ACTION_SHIFT_NE, { point_north_east, point_east } },
                    { ACTION_SHIFT_E, { point_east, point_south_east } },
                    { ACTION_SHIFT_SE, { point_south_east, point_south } },
                    { ACTION_SHIFT_S, { point_south, point_south_west } },
                    { ACTION_SHIFT_SW, { point_south_west, point_west } },
                    { ACTION_SHIFT_W, { point_west, point_north_west } },
                    { ACTION_SHIFT_NW, { point_north_west, point_north } },
                };
                u.view_offset += use_tiles && tile_iso ?
                                 shift_delta.at( act ).second * soffset : shift_delta.at( act ).first * soffset;
            }
            break;

            case ACTION_LOOK:
                look_around();
                break;

            case ACTION_KEYBINDINGS:
                // already handled by input context
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
            case ACTION_KEYBINDINGS:
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

            case ACTION_CYCLE_MOVE:
                u.cycle_move_mode();
                break;

            case ACTION_RESET_MOVE:
                u.reset_move_mode();
                break;

            case ACTION_TOGGLE_RUN:
                u.toggle_run_mode();
                break;

            case ACTION_TOGGLE_CROUCH:
                u.toggle_crouch_mode();
                break;

            case ACTION_OPEN_MOVEMENT:
                open_movement_mode_menu();
                break;

            case ACTION_MOVE_FORTH:
            case ACTION_MOVE_FORTH_RIGHT:
            case ACTION_MOVE_RIGHT:
            case ACTION_MOVE_BACK_RIGHT:
            case ACTION_MOVE_BACK:
            case ACTION_MOVE_BACK_LEFT:
            case ACTION_MOVE_LEFT:
            case ACTION_MOVE_FORTH_LEFT:
                if( !u.get_value( "remote_controlling" ).empty() &&
                    ( u.has_active_item( "radiocontrol" ) || u.has_active_bionic( bio_remote ) ) ) {
                    rcdrive( get_delta_from_movement_action( act, iso_rotate::yes ) );
                } else if( veh_ctrl ) {
                    // vehicle control uses x for steering and y for ac/deceleration,
                    // so no rotation needed
                    pldrive( get_delta_from_movement_action( act, iso_rotate::no ) );
                } else {
                    point dest_delta = get_delta_from_movement_action( act, iso_rotate::yes );
                    if( auto_travel_mode && !u.is_auto_moving() ) {
                        for( int i = 0; i < SEEX; i++ ) {
                            tripoint auto_travel_destination( u.posx() + dest_delta.x * ( SEEX - i ),
                                                              u.posy() + dest_delta.y * ( SEEX - i ),
                                                              u.posz() );
                            destination_preview = m.route( u.pos(),
                                                           auto_travel_destination,
                                                           u.get_pathfinding_settings(),
                                                           u.get_path_avoid() );
                            if( !destination_preview.empty() ) {
                                destination_preview.erase( destination_preview.begin() + 1, destination_preview.end() );
                                u.set_destination( destination_preview );
                                break;
                            }
                        }
                        act = u.get_next_auto_move_direction();
                        const point dest_next = get_delta_from_movement_action( act, iso_rotate::yes );
                        if( dest_next == point_zero ) {
                            u.clear_destination();
                        }
                        dest_delta = dest_next;
                    }
                    if( !avatar_action::move( u, m, dest_delta ) ) {
                        // auto-move should be canceled due to a failed move or obstacle
                        u.clear_destination();
                    }
                }
                break;
            case ACTION_MOVE_DOWN:
                if( u.is_mounted() ) {
                    auto mon = u.mounted_creature.get();
                    if( !mon->has_flag( MF_RIDEABLE_MECH ) ) {
                        add_msg( m_info, _( "You can't go down stairs while you're riding." ) );
                        break;
                    }
                }
                if( !u.in_vehicle ) {
                    vertical_move( -1, false );
                } else if( veh_ctrl && vp->vehicle().is_rotorcraft() ) {
                    pldrive( tripoint_below );
                }
                break;

            case ACTION_MOVE_UP:
                if( u.is_mounted() ) {
                    auto mon = u.mounted_creature.get();
                    if( !mon->has_flag( MF_RIDEABLE_MECH ) ) {
                        add_msg( m_info, _( "You can't go down stairs while you're riding." ) );
                        break;
                    }
                }
                if( !u.in_vehicle ) {
                    vertical_move( 1, false );
                } else if( veh_ctrl && vp->vehicle().is_rotorcraft() ) {
                    pldrive( tripoint_above );
                }
                break;

            case ACTION_OPEN:
                if( u.has_active_mutation( trait_SHELL2 ) ) {
                    add_msg( m_info, _( "You can't open things while you're in your shell." ) );
                } else if( u.is_mounted() ) {
                    add_msg( m_info, _( "You can't open things while you're riding." ) );
                } else {
                    open();
                }
                break;

            case ACTION_CLOSE:
                if( u.has_active_mutation( trait_SHELL2 ) ) {
                    add_msg( m_info, _( "You can't close things while you're in your shell." ) );
                } else if( u.is_mounted() ) {
                    auto mon = u.mounted_creature.get();
                    if( !mon->has_flag( MF_RIDEABLE_MECH ) ) {
                        add_msg( m_info, _( "You can't close things while you're riding." ) );
                    }
                } else if( mouse_target ) {
                    doors::close_door( m, u, *mouse_target );
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
                } else if( mouse_target ) {
                    examine( *mouse_target );
                } else {
                    examine();
                }
                break;

            case ACTION_ADVANCEDINV:
                if( u.has_active_mutation( trait_SHELL2 ) ) {
                    add_msg( m_info, _( "You can't move mass quantities while you're in your shell." ) );
                } else if( u.is_mounted() ) {
                    add_msg( m_info, _( "You can't move mass quantities while you're riding." ) );
                } else {
                    create_advanced_inv();
                }
                break;

            case ACTION_PICKUP:
                if( u.has_active_mutation( trait_SHELL2 ) ) {
                    add_msg( m_info, _( "You can't pick anything up while you're in your shell." ) );
                } else if( u.is_mounted() ) {
                    add_msg( m_info, _( "You can't pick anything up while you're riding." ) );
                } else if( mouse_target ) {
                    pickup( *mouse_target );
                } else {
                    pickup();
                }
                break;

            case ACTION_PICKUP_FEET:
                if( u.has_active_mutation( trait_SHELL2 ) ) {
                    add_msg( m_info, _( "You can't pick anything up while you're in your shell." ) );
                } else {
                    pickup_feet();
                }
                break;

            case ACTION_GRAB:
                if( u.has_active_mutation( trait_SHELL2 ) ) {
                    add_msg( m_info, _( "You can't grab things while you're in your shell." ) );
                } else if( u.is_mounted() ) {
                    add_msg( m_info, _( "You can't grab things while you're riding." ) );
                } else {
                    grab();
                }
                break;

            case ACTION_HAUL:
                if( u.has_active_mutation( trait_SHELL2 ) ) {
                    add_msg( m_info, _( "You can't haul things while you're in your shell." ) );
                } else if( u.is_mounted() ) {
                    add_msg( m_info, _( "You can't haul things while you're riding." ) );
                } else {
                    haul();
                }
                break;

            case ACTION_BUTCHER:
                if( u.has_active_mutation( trait_SHELL2 ) ) {
                    add_msg( m_info, _( "You can't butcher while you're in your shell." ) );
                } else if( u.is_mounted() ) {
                    add_msg( m_info, _( "You can't butcher while you're riding." ) );
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
                } else if( u.is_mounted() ) {
                    add_msg( m_info, _( "You can't peek around corners while you're riding." ) );
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
                game_menus::inv::compare( u, cata::nullopt );
                break;

            case ACTION_ORGANIZE:
                game_menus::inv::swap_letters( u );
                break;

            case ACTION_USE:
                // Shell-users are presumed to be able to mess with their inventories, etc
                // while in the shell.  Eating, gear-changing, and item use are OK.
                avatar_action::use_item( u );
                break;

            case ACTION_USE_WIELDED:
                u.use_wielded();
                break;

            case ACTION_WEAR:
                wear();
                break;

            case ACTION_TAKE_OFF:
                takeoff();
                break;

            case ACTION_EAT:
                if( !avatar_action::eat_here( u ) ) {
                    avatar_action::eat( u );
                }
                break;

            case ACTION_OPEN_CONSUME:
                if( !avatar_action::eat_here( u ) ) {
                    open_consume_item_menu();
                }
                break;

            case ACTION_READ:
                // Shell-users are presumed to have the book just at an opening and read it that way
                read();
                break;

            case ACTION_WIELD:
                wield();
                break;

            case ACTION_PICK_STYLE:
                u.martial_arts_data.pick_style( u );
                break;

            case ACTION_RELOAD_ITEM:
                reload_item();
                break;

            case ACTION_RELOAD_WEAPON:
                reload_weapon();
                break;

            case ACTION_RELOAD_WIELDED:
                reload_wielded();
                break;

            case ACTION_UNLOAD:
                avatar_action::unload( u );
                break;

            case ACTION_MEND:
                avatar_action::mend( g->u, item_location() );
                break;

            case ACTION_THROW: {
                item_location loc;
                avatar_action::plthrow( g->u, loc );
                break;
            }

            case ACTION_FIRE:
                fire();
                break;

            case ACTION_CAST_SPELL:
                cast_spell();
                break;

            case ACTION_FIRE_BURST: {
                gun_mode_id original_mode = u.weapon.gun_get_mode_id();
                if( u.weapon.gun_set_mode( gun_mode_id( "AUTO" ) ) ) {
                    avatar_action::fire_wielded_weapon( u, m );
                    u.weapon.gun_set_mode( original_mode );
                }
                break;
            }

            case ACTION_SELECT_FIRE_MODE:
                if( u.is_armed() ) {
                    if( u.weapon.is_gun() && !u.weapon.is_gunmod() && u.weapon.gun_all_modes().size() > 1 ) {
                        u.weapon.gun_cycle_mode();
                    } else if( u.weapon.has_flag( flag_RELOAD_ONE ) || u.weapon.has_flag( flag_RELOAD_AND_SHOOT ) ) {
                        item::reload_option opt = u.select_ammo( u.weapon, false );
                        if( !opt ) {
                            break;
                        } else if( u.ammo_location && opt.ammo == u.ammo_location ) {
                            u.ammo_location = item_location();
                        } else {
                            u.ammo_location = opt.ammo;
                        }
                    }
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
                } else if( u.is_mounted() ) {
                    add_msg( m_info, _( "You can't craft while you're riding." ) );
                } else {
                    u.craft();
                }
                break;

            case ACTION_RECRAFT:
                if( u.has_active_mutation( trait_SHELL2 ) ) {
                    add_msg( m_info, _( "You can't craft while you're in your shell." ) );
                } else if( u.is_mounted() ) {
                    add_msg( m_info, _( "You can't craft while you're riding." ) );
                } else {
                    u.recraft();
                }
                break;

            case ACTION_LONGCRAFT:
                if( u.has_active_mutation( trait_SHELL2 ) ) {
                    add_msg( m_info, _( "You can't craft while you're in your shell." ) );
                } else if( u.is_mounted() ) {
                    add_msg( m_info, _( "You can't craft while you're riding." ) );
                } else {
                    u.long_craft();
                }
                break;

            case ACTION_DISASSEMBLE:
                if( u.controlling_vehicle ) {
                    add_msg( m_info, _( "You can't disassemble items while driving." ) );
                } else if( u.is_mounted() ) {
                    add_msg( m_info, _( "You can't disassemble items while you're riding." ) );
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
                } else if( u.is_mounted() ) {
                    add_msg( m_info, _( "You can't construct while you're riding." ) );
                } else {
                    construction_menu( false );
                }
                break;

            case ACTION_SLEEP:
                if( veh_ctrl ) {
                    add_msg( m_info, _( "Vehicle control has moved, %s" ),
                             press_x( ACTION_CONTROL_VEHICLE, _( "new binding is " ),
                                      _( "new default binding is '^'." ) ) );
                } else {
                    sleep();
                }
                break;

            case ACTION_CONTROL_VEHICLE:
                if( u.has_active_mutation( trait_SHELL2 ) ) {
                    add_msg( m_info, _( "You can't operate a vehicle while you're in your shell." ) );
                } else if( u.is_mounted() ) {
                    u.dismount();
                } else if( u.has_trait( trait_WAYFARER ) ) {
                    add_msg( m_info, _( "You refuse to take control of this vehicle." ) );
                } else {
                    control_vehicle();
                }
                break;

            case ACTION_TOGGLE_AUTO_TRAVEL_MODE:
                auto_travel_mode = !auto_travel_mode;
                add_msg( m_info, auto_travel_mode ? _( "Auto travel mode ON!" ) : _( "Auto travel mode OFF!" ) );
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
                             ? _( "Safe mode OFF!  (Auto safe mode still enabled!)" ) : _( "Safe mode OFF!" ) );
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
                    for( auto &elem : u.get_mon_visible().new_seen_mon ) {
                        monster &critter = *elem;
                        critter.ignoring = rl_dist( u.pos(), critter.pos() );
                    }
                    set_safe_mode( SAFE_MODE_ON );
                } else if( u.has_effect( effect_laserlocked ) ) {
                    if( u.has_trait( trait_PROF_CHURL ) ) {
                        add_msg( m_warning, _( "You make the sign of the cross." ) );
                    } else {
                        add_msg( m_info, _( "Ignoring laser targeting!" ) );
                    }
                    u.remove_effect( effect_laserlocked );
                    safe_mode_warning_logged = false;
                }
                break;

            case ACTION_WHITELIST_ENEMY:
                if( safe_mode == SAFE_MODE_STOP && !get_safemode().empty() ) {
                    get_safemode().add_rule( get_safemode().lastmon_whitelist, Creature::A_ANY, 0, RULE_WHITELISTED );
                    add_msg( m_info, _( "Creature whitelisted: %s" ), get_safemode().lastmon_whitelist );
                    set_safe_mode( SAFE_MODE_ON );
                    mostseen = 0;
                } else {
                    get_safemode().show();
                }
                break;

            case ACTION_SUICIDE:
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
                break;

            case ACTION_MAP:
                werase( w_terrain );
                ui::omap::display();
                break;

            case ACTION_SKY:
                if( m.is_outside( u.pos() ) ) {
                    werase( w_terrain );
                    ui::omap::display_visible_weather();
                } else {
                    add_msg( m_info, _( "You can't see the sky from here." ) );
                }
                break;

            case ACTION_MISSIONS:
                list_missions();
                break;

            case ACTION_SCORES:
                show_scores_ui( *achievements_tracker_ptr, stats(), get_kill_tracker() );
                break;

            case ACTION_FACTIONS:
                faction_manager_ptr->display();
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
                get_help().display_help();
                refresh_all();
                break;

            case ACTION_OPTIONS:
                get_options().show( true );
                break;

            case ACTION_AUTOPICKUP:
                get_auto_pickup().show();
                refresh_all();
                break;

            case ACTION_AUTONOTES:
                get_auto_notes_settings().show_gui();
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
                debug_menu::debug();
                break;

            case ACTION_TOGGLE_FULLSCREEN:
                toggle_fullscreen();
                break;

            case ACTION_TOGGLE_PIXEL_MINIMAP:
                toggle_pixel_minimap();
                break;

            case ACTION_TOGGLE_PANEL_ADM:
                panel_manager::get_manager().show_adm();
                break;

            case ACTION_RELOAD_TILESET:
                reload_tileset();
                break;

            case ACTION_TOGGLE_AUTO_FEATURES:
                get_options().get_option( "AUTO_FEATURES" ).setNext();
                get_options().save();
                //~ Auto Features are now ON/OFF
                add_msg( _( "%s are now %s." ),
                         get_options().get_option( "AUTO_FEATURES" ).getMenuText(),
                         get_option<bool>( "AUTO_FEATURES" ) ? _( "ON" ) : _( "OFF" ) );
                break;

            case ACTION_TOGGLE_AUTO_PULP_BUTCHER:
                get_options().get_option( "AUTO_PULP_BUTCHER" ).setNext();
                get_options().save();
                //~ Auto Pulp/Pulp Adjacent/Butcher is now set to x
                add_msg( _( "%s is now set to %s." ),
                         get_options().get_option( "AUTO_PULP_BUTCHER" ).getMenuText(),
                         get_options().get_option( "AUTO_PULP_BUTCHER" ).getValueName() );
                break;

            case ACTION_TOGGLE_AUTO_MINING:
                get_options().get_option( "AUTO_MINING" ).setNext();
                get_options().save();
                //~ Auto Mining is now ON/OFF
                add_msg( _( "%s is now %s." ),
                         get_options().get_option( "AUTO_MINING" ).getMenuText(),
                         get_option<bool>( "AUTO_MINING" ) ? _( "ON" ) : _( "OFF" ) );
                break;

            case ACTION_TOGGLE_THIEF_MODE:
                if( g->u.get_value( "THIEF_MODE" ) == "THIEF_ASK" ) {
                    u.set_value( "THIEF_MODE", "THIEF_HONEST" );
                    u.set_value( "THIEF_MODE_KEEP", "YES" );
                    //~ Thief mode cycled between THIEF_ASK/THIEF_HONEST/THIEF_STEAL
                    add_msg( _( "You will not pick up other peoples belongings." ) );
                } else if( g->u.get_value( "THIEF_MODE" ) == "THIEF_HONEST" ) {
                    u.set_value( "THIEF_MODE", "THIEF_STEAL" );
                    u.set_value( "THIEF_MODE_KEEP", "YES" );
                    //~ Thief mode cycled between THIEF_ASK/THIEF_HONEST/THIEF_STEAL
                    add_msg( _( "You will pick up also those things that belong to others!" ) );
                } else if( g->u.get_value( "THIEF_MODE" ) == "THIEF_STEAL" ) {
                    u.set_value( "THIEF_MODE", "THIEF_ASK" );
                    u.set_value( "THIEF_MODE_KEEP", "NO" );
                    //~ Thief mode cycled between THIEF_ASK/THIEF_HONEST/THIEF_STEAL
                    add_msg( _( "You will be reminded not to steal." ) );
                } else {
                    // ERROR
                    add_msg( _( "THIEF_MODE CONTAINED BAD VALUE [ %s ]!" ), g->u.get_value( "THIEF_MODE" ) );
                }
                break;

            case ACTION_TOGGLE_AUTO_FORAGING:
                get_options().get_option( "AUTO_FORAGING" ).setNext();
                get_options().save();
                //~ Auto Foraging is now set to x
                add_msg( _( "%s is now set to %s." ),
                         get_options().get_option( "AUTO_FORAGING" ).getMenuText(),
                         get_options().get_option( "AUTO_FORAGING" ).getValueName() );
                break;

            case ACTION_TOGGLE_AUTO_PICKUP:
                get_options().get_option( "AUTO_PICKUP" ).setNext();
                get_options().save();
                //~ Auto pickup is now set to x
                add_msg( _( "%s is now set to %s." ),
                         get_options().get_option( "AUTO_PICKUP" ).getMenuText(),
                         get_options().get_option( "AUTO_PICKUP" ).getValueName() );
                break;

            case ACTION_DISPLAY_SCENT:
                if( MAP_SHARING::isCompetitive() && !MAP_SHARING::isDebugger() ) {
                    break;    //don't do anything when sharing and not debugger
                }
                display_scent();
                break;

            case ACTION_DISPLAY_SCENT_TYPE:
                if( MAP_SHARING::isCompetitive() && !MAP_SHARING::isDebugger() ) {
                    break;    //don't do anything when sharing and not debugger
                }
                display_scent();
                break;

            case ACTION_DISPLAY_TEMPERATURE:
                if( MAP_SHARING::isCompetitive() && !MAP_SHARING::isDebugger() ) {
                    break;    //don't do anything when sharing and not debugger
                }
                display_temperature();
                break;
            case ACTION_DISPLAY_VEHICLE_AI:
                if( MAP_SHARING::isCompetitive() && !MAP_SHARING::isDebugger() ) {
                    break;    //don't do anything when sharing and not debugger
                }
                display_vehicle_ai();
                break;
            case ACTION_DISPLAY_VISIBILITY:
                if( MAP_SHARING::isCompetitive() && !MAP_SHARING::isDebugger() ) {
                    break;    //don't do anything when sharing and not debugger
                }
                display_visibility();
                break;

            case ACTION_DISPLAY_LIGHTING:
                if( MAP_SHARING::isCompetitive() && !MAP_SHARING::isDebugger() ) {
                    break;    //don't do anything when sharing and not debugger
                }
                display_lighting();
                break;

            case ACTION_DISPLAY_RADIATION:
                if( MAP_SHARING::isCompetitive() && !MAP_SHARING::isDebugger() ) {
                    break;    //don't do anything when sharing and not debugger
                }
                display_radiation();
                break;

            case ACTION_TOGGLE_HOUR_TIMER:
                toggle_debug_hour_timer();
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
                avatar_action::autoattack( u, m );
                break;

            default:
                break;
        }
    }
    if( act != ACTION_TIMEOUT ) {
        u.mod_moves( -current_turn.moves_elapsed() );
    }
    gamemode->post_action( act );

    u.movecounter = ( !u.is_dead_state() ? ( before_action_moves - u.moves ) : 0 );
    dbg( D_INFO ) << string_format( "%s: [%d] %d - %d = %d", action_ident( act ),
                                    to_turn<int>( calendar::turn ), before_action_moves, u.movecounter, u.moves );
    return ( !u.is_dead_state() );
}
