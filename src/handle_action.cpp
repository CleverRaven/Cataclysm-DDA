#include "game.h" // IWYU pragma: associated

#include <algorithm>
#include <chrono>
#include <cmath>
#include <initializer_list>
#include <map>
#include <sstream>
#include <string>
#include <utility>

#include "action.h"
#include "activity_actor_definitions.h"
#include "activity_type.h"
#include "advanced_inv.h"
#include "auto_note.h"
#include "auto_pickup.h"
#include "avatar.h"
#include "avatar_action.h"
#include "bionics.h"
#include "bodygraph.h"
#include "bodypart.h"
#include "cached_options.h"
#include "calendar.h"
#include "catacharset.h"
#include "character.h"
#include "character_martial_arts.h"
#include "clzones.h"
#include "colony.h"
#include "color.h"
#include "construction.h"
#include "creature_tracker.h"
#include "cursesdef.h"
#include "damage.h"
#include "debug.h"
#include "debug_menu.h"
#include "diary.h"
#include "distraction_manager.h"
#include "do_turn.h"
#include "event.h"
#include "event_bus.h"
#include "faction.h"
#include "field.h"
#include "field_type.h"
#include "flag.h"
#include "game_constants.h"
#include "game_inventory.h"
#include "gamemode.h"
#include "gates.h"
#include "gun_mode.h"
#include "help.h"
#include "input.h"
#include "item.h"
#include "item_group.h"
#include "itype.h"
#include "iuse.h"
#include "level_cache.h"
#include "lightmap.h"
#include "line.h"
#include "magic.h"
#include "make_static.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "mapsharing.h"
#include "messages.h"
#include "monster.h"
#include "move_mode.h"
#include "mtype.h"
#include "mutation.h"
#include "options.h"
#include "output.h"
#include "overmap_ui.h"
#include "panels.h"
#include "player_activity.h"
#include "popup.h"
#include "ranged.h"
#include "rng.h"
#include "safemode_ui.h"
#include "scores_ui.h"
#include "sounds.h"
#include "string_formatter.h"
#include "timed_event.h"
#include "translations.h"
#include "ui.h"
#include "ui_manager.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "weather.h"
#include "weather_type.h"
#include "worldfactory.h"

#if defined(TILES)
#include "cata_tiles.h" // all animation functions will be pushed out to a cata_tiles function in some manner
#include "sdltiles.h"
#endif

static const activity_id ACT_FERTILIZE_PLOT( "ACT_FERTILIZE_PLOT" );
static const activity_id ACT_MOVE_LOOT( "ACT_MOVE_LOOT" );
static const activity_id ACT_MULTIPLE_BUTCHER( "ACT_MULTIPLE_BUTCHER" );
static const activity_id ACT_MULTIPLE_CHOP_PLANKS( "ACT_MULTIPLE_CHOP_PLANKS" );
static const activity_id ACT_MULTIPLE_CHOP_TREES( "ACT_MULTIPLE_CHOP_TREES" );
static const activity_id ACT_MULTIPLE_CONSTRUCTION( "ACT_MULTIPLE_CONSTRUCTION" );
static const activity_id ACT_MULTIPLE_DIS( "ACT_MULTIPLE_DIS" );
static const activity_id ACT_MULTIPLE_FARM( "ACT_MULTIPLE_FARM" );
static const activity_id ACT_MULTIPLE_MINE( "ACT_MULTIPLE_MINE" );
static const activity_id ACT_MULTIPLE_MOP( "ACT_MULTIPLE_MOP" );
static const activity_id ACT_PULP( "ACT_PULP" );
static const activity_id ACT_SPELLCASTING( "ACT_SPELLCASTING" );
static const activity_id ACT_VEHICLE_DECONSTRUCTION( "ACT_VEHICLE_DECONSTRUCTION" );
static const activity_id ACT_VEHICLE_REPAIR( "ACT_VEHICLE_REPAIR" );
static const activity_id ACT_WAIT( "ACT_WAIT" );
static const activity_id ACT_WAIT_STAMINA( "ACT_WAIT_STAMINA" );
static const activity_id ACT_WAIT_WEATHER( "ACT_WAIT_WEATHER" );

static const bionic_id bio_remote( "bio_remote" );

static const damage_type_id damage_bash( "bash" );
static const damage_type_id damage_cut( "cut" );

static const efftype_id effect_alarm_clock( "alarm_clock" );
static const efftype_id effect_incorporeal( "incorporeal" );
static const efftype_id effect_laserlocked( "laserlocked" );
static const efftype_id effect_stunned( "stunned" );

static const flag_id json_flag_MOP( "MOP" );

static const gun_mode_id gun_mode_AUTO( "AUTO" );

static const itype_id fuel_type_animal( "animal" );
static const itype_id itype_radiocontrol( "radiocontrol" );

static const json_character_flag json_flag_ALARMCLOCK( "ALARMCLOCK" );
static const json_character_flag json_flag_SUBTLE_SPELL( "SUBTLE_SPELL" );

static const material_id material_glass( "glass" );

static const mon_flag_str_id mon_flag_RIDEABLE_MECH( "RIDEABLE_MECH" );

static const proficiency_id proficiency_prof_helicopter_pilot( "prof_helicopter_pilot" );

static const quality_id qual_CUT( "CUT" );

static const skill_id skill_melee( "melee" );

static const trait_id trait_BRAWLER( "BRAWLER" );
static const trait_id trait_HIBERNATE( "HIBERNATE" );
static const trait_id trait_PROF_CHURL( "PROF_CHURL" );
static const trait_id trait_SHELL2( "SHELL2" );
static const trait_id trait_SHELL3( "SHELL3" );
static const trait_id trait_WATERSLEEP( "WATERSLEEP" );
static const trait_id trait_WATERSLEEPER( "WATERSLEEPER" );
static const trait_id trait_WAYFARER( "WAYFARER" );

static const zone_type_id zone_type_CHOP_TREES( "CHOP_TREES" );
static const zone_type_id zone_type_CONSTRUCTION_BLUEPRINT( "CONSTRUCTION_BLUEPRINT" );
static const zone_type_id zone_type_FARM_PLOT( "FARM_PLOT" );
static const zone_type_id zone_type_LOOT_CORPSE( "LOOT_CORPSE" );
static const zone_type_id zone_type_LOOT_UNSORTED( "LOOT_UNSORTED" );
static const zone_type_id zone_type_LOOT_WOOD( "LOOT_WOOD" );
static const zone_type_id zone_type_MINING( "MINING" );
static const zone_type_id zone_type_MOPPING( "MOPPING" );
static const zone_type_id zone_type_VEHICLE_DECONSTRUCT( "VEHICLE_DECONSTRUCT" );
static const zone_type_id zone_type_VEHICLE_REPAIR( "VEHICLE_REPAIR" );
static const zone_type_id zone_type_zone_disassemble( "zone_disassemble" );
static const zone_type_id zone_type_zone_strip( "zone_strip" );
static const zone_type_id zone_type_zone_unload_all( "zone_unload_all" );

static const std::string flag_CANT_DRAG( "CANT_DRAG" );

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
            std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
            std::chrono::milliseconds elapsed_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>( now - user_turn_start );
            return elapsed_ms.count() / ( 10.0 * turn_duration );
        }

        bool async_anim_timeout() {
            std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
            std::chrono::milliseconds elapsed_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>( now - user_turn_start );
            return elapsed_ms.count() > get_option<int>( "ANIMATION_DELAY" );
        }

        std::chrono::steady_clock::time_point last_blink_transition = std::chrono::steady_clock::now();
        bool blink_timeout() {
            std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
            std::chrono::milliseconds elapsed_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>( now - last_blink_transition );
            if( elapsed_ms.count() > get_option<int>( "BLINK_SPEED" ) ) {
                last_blink_transition = now;
                return true;
            }
            return false;
        }

};

input_context game::get_player_input( std::string &action )
{
    input_context ctxt;
    if( uquit == QUIT_WATCH ) {
        ctxt = input_context( "DEFAULTMODE", keyboard_mode::keycode );
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
    const visibility_variables &cache = m.get_visibility_variables_cache();
    const level_cache &map_cache = m.get_cache_ref( u.posz() );
    const auto &visibility_cache = map_cache.visibility_cache;

    user_turn current_turn;

    if( get_option<bool>( "ANIMATIONS" ) ) {
        const int TOTAL_VIEW = MAX_VIEW_DISTANCE * 2 + 1;
        point iStart( ( TERRAIN_WINDOW_WIDTH > TOTAL_VIEW ) ? ( TERRAIN_WINDOW_WIDTH - TOTAL_VIEW ) / 2 : 0,
                      ( TERRAIN_WINDOW_HEIGHT > TOTAL_VIEW ) ? ( TERRAIN_WINDOW_HEIGHT - TOTAL_VIEW ) / 2 :
                      0 );
        point iEnd( ( TERRAIN_WINDOW_WIDTH > TOTAL_VIEW ) ? TERRAIN_WINDOW_WIDTH -
                    ( TERRAIN_WINDOW_WIDTH - TOTAL_VIEW ) /
                    2 :
                    TERRAIN_WINDOW_WIDTH, ( TERRAIN_WINDOW_HEIGHT > TOTAL_VIEW ) ? TERRAIN_WINDOW_HEIGHT -
                    ( TERRAIN_WINDOW_HEIGHT - TOTAL_VIEW ) /
                    2 : TERRAIN_WINDOW_HEIGHT );

        if( fullscreen ) {
            iStart.x = 0;
            iStart.y = 0;
            iEnd.x = TERMX;
            iEnd.y = TERMY;
        }

        //x% of the Viewport, only shown on visible areas
        const weather_animation_t weather_info = weather.weather_id->weather_animation;
        point offset( u.view_offset.xy() + point( -getmaxx( w_terrain ) / 2 + u.posx(),
                      -getmaxy( w_terrain ) / 2 + u.posy() ) );

#if defined(TILES)
        if( g->is_tileset_isometric() ) {
            iStart.x = 0;
            iStart.y = 0;
            iEnd.x = MAPSIZE_X;
            iEnd.y = MAPSIZE_Y;
            offset.x = 0;
            offset.y = 0;
        }
#endif //TILES

        // TODO: Move the weather calculations out of here.
        const bool bWeatherEffect = weather_info.symbol != NULL_UNICODE;
        const int dropCount = static_cast<int>( iEnd.x * iEnd.y * weather_info.factor );

        weather_printable wPrint;
        wPrint.colGlyph = weather_info.color;
        wPrint.cGlyph = weather_info.symbol;
        wPrint.wtype = weather.weather_id;
        wPrint.vdrops.clear();

        ctxt.set_timeout( 125 );

        shared_ptr_fast<game::draw_callback_t> animation_cb =
        make_shared_fast<game::draw_callback_t>( [&]() {
            draw_weather( wPrint );

            if( uquit != QUIT_WATCH ) {
                draw_sct();
            }
        } );
        add_draw_callback( animation_cb );

        creature_tracker &creatures = get_creature_tracker();
        do {
            if( bWeatherEffect && get_option<bool>( "ANIMATION_RAIN" ) ) {
                /*
                Location to add rain drop animation bits! Since it refreshes w_terrain it can be added to the animation section easily
                Get tile information from above's weather information:
                WEATHER_DRIZZLE | WEATHER_LIGHT_DRIZZLE | WEATHER_RAINY | WEATHER_RAINSTORM | WEATHER_THUNDER | WEATHER_LIGHTNING = "weather_rain_drop"
                WEATHER_FLURRIES | WEATHER_SNOW | WEATHER_SNOWSTORM = "weather_snowflake"
                */
                invalidate_main_ui_adaptor();

                wPrint.vdrops.clear();

                for( int i = 0; i < dropCount; i++ ) {
                    const point iRand( rng( iStart.x, iEnd.x - 1 ), rng( iStart.y, iEnd.y - 1 ) );
                    const point map( iRand + offset );

                    const tripoint mapp( map, u.posz() );

                    const lit_level lighting = visibility_cache[mapp.x][mapp.y];

                    if( m.is_outside( mapp ) && m.get_visibility( lighting, cache ) == visibility_type::CLEAR &&
                        !creatures.creature_at( mapp, true ) ) {
                        // Suppress if a critter is there
                        wPrint.vdrops.emplace_back( iRand.x, iRand.y );
                    }
                }
            }
            // don't bother calculating SCT if we won't show it
            if( uquit != QUIT_WATCH && get_option<bool>( "ANIMATION_SCT" ) && !SCT.vSCT.empty() ) {
                invalidate_main_ui_adaptor();

                SCT.advanceAllSteps();

                //Check for creatures on all drawing positions and offset if necessary
                for( auto iter = SCT.vSCT.rbegin(); iter != SCT.vSCT.rend(); ++iter ) {
                    const direction oCurDir = iter->getDirection();
                    const int width = utf8_width( iter->getText() );
                    for( int i = 0; i < width; ++i ) {
                        tripoint tmp( iter->getPosX() + i, iter->getPosY(), get_map().get_abs_sub().z() );
                        const Creature *critter = creatures.creature_at( tmp, true );

                        if( critter != nullptr && u.sees( *critter ) ) {
                            i = -1;
                            int iPos = iter->getStep() + iter->getStepOffset();
                            for( auto iter2 = iter; iter2 != SCT.vSCT.rend(); ++iter2 ) {
                                if( iter2->getDirection() == oCurDir &&
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

            if( pixel_minimap_option ) {
                // TODO: more granular control to only redraw pixel minimap
                invalidate_main_ui_adaptor();
            }

            std::unique_ptr<static_popup> deathcam_msg_popup;
            if( uquit == QUIT_WATCH ) {
                deathcam_msg_popup = std::make_unique<static_popup>();
                deathcam_msg_popup
                ->wait_message( c_red, _( "Press %s to accept your fate…" ), ctxt.get_desc( "QUIT" ) )
                .on_top( true );
            }

            // Remove asynchronous animations after animation delay if no input
            if( current_turn.async_anim_timeout() ) {
                g->void_async_anim_curses();
#if defined(TILES)
                tilecontext->void_async_anim();
#else
                // Curses does not redraw itself so do it here
                g->invalidate_main_ui_adaptor();
#endif
            }

            if( g->has_blink_curses() && current_turn.blink_timeout() ) {
                // Toggle blink phase and redraw
                g->blink_active_phase = !g->blink_active_phase;
                g->invalidate_main_ui_adaptor();
            }

            ui_manager::redraw_invalidated();
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

static void rcdrive( const point &d )
{
    Character &player_character = get_player_character();
    map &here = get_map();
    std::stringstream car_location_string( player_character.get_value( "remote_controlling" ) );

    if( car_location_string.str().empty() ) {
        //no turned radio car found
        add_msg( m_warning, _( "No radio car connected." ) );
        return;
    }
    tripoint c;
    car_location_string >> c.x >> c.y >> c.z;

    auto rc_pairs = here.get_rc_items( c );
    auto rc_pair = rc_pairs.begin();
    for( ; rc_pair != rc_pairs.end(); ++rc_pair ) {
        if( rc_pair->second->has_flag( flag_RADIOCAR ) && rc_pair->second->active ) {
            break;
        }
    }
    if( rc_pair == rc_pairs.end() ) {
        add_msg( m_warning, _( "No radio car connected." ) );
        player_character.remove_value( "remote_controlling" );
        return;
    }
    item *rc_car = rc_pair->second;

    tripoint dest( c + d );
    if( here.impassable( dest ) || !here.can_put_items_ter_furn( dest ) ||
        here.has_furn( dest ) ) {
        sounds::sound( dest, 7, sounds::sound_t::combat,
                       _( "sound of a collision with an obstacle." ), true, "misc", "rc_car_hits_obstacle" );
        return;
    } else if( !here.add_item_or_charges( dest, *rc_car ).is_null() ) {
        tripoint src( c );
        //~ Sound of moving a remote controlled car
        sounds::sound( src, 6, sounds::sound_t::movement, _( "zzz…" ), true, "misc", "rc_car_drives" );
        player_character.moves -= 50;
        here.i_rem( src, rc_car );
        car_location_string.clear();
        car_location_string << dest.x << ' ' << dest.y << ' ' << dest.z;
        player_character.set_value( "remote_controlling", car_location_string.str() );
        return;
    }
}

static void pldrive( const tripoint &p )
{
    if( !g->check_safe_mode_allowed() ) {
        return;
    }
    vehicle *veh = g->remoteveh();
    bool remote = true;
    int part = -1;
    Character &player_character = get_player_character();
    map &here = get_map();
    if( !veh ) {
        if( const optional_vpart_position vp = here.veh_at( player_character.pos() ) ) {
            veh = &vp->vehicle();
            part = vp->part_index();
        }
        remote = false;
    }
    if( !veh ) {
        dbg( D_ERROR ) << "game::pldrive: can't find vehicle!  Drive mode is now off.";
        debugmsg( "game::pldrive error: can't find vehicle!  Drive mode is now off." );
        player_character.in_vehicle = false;
        return;
    }
    if( veh->is_on_ramp && p.x != 0 ) {
        add_msg( m_bad, _( "You can't turn the vehicle while on a ramp." ) );
        return;
    }
    if( !remote ) {
        const vehicle_part &vp = veh->part( part );
        const bool has_animal_controls = veh->part_with_feature( vp.mount, "CONTROL_ANIMAL", true ) >= 0;
        const bool has_controls = veh->part_with_feature( vp.mount, "CONTROLS", true ) >= 0;
        const bool has_animal = veh->has_engine_type( fuel_type_animal, false ) &&
                                veh->get_harnessed_animal();
        if( !has_controls && !has_animal_controls ) {
            add_msg( m_info, _( "You can't drive the vehicle from here.  You need controls!" ) );
            player_character.controlling_vehicle = false;
            return;
        } else if( !has_controls && has_animal_controls && !has_animal ) {
            add_msg( m_info, _( "You can't drive this vehicle without an animal to pull it." ) );
            player_character.controlling_vehicle = false;
            return;
        }
    } else {
        if( empty( veh->get_avail_parts( "REMOTE_CONTROLS" ) ) ) {
            add_msg( m_info, _( "Can't drive this vehicle remotely.  It has no working controls." ) );
            return;
        }
    }
    if( p.z != 0 ) {
        if( !player_character.has_proficiency( proficiency_prof_helicopter_pilot ) ) {
            player_character.add_msg_if_player( m_info, _( "You have no idea how to make the vehicle fly." ) );
            return;
        }
        if( !veh->is_flyable() ) {
            player_character.add_msg_if_player( m_info, _( "This vehicle doesn't look very airworthy." ) );
            return;
        }
    }
    if( p.z == -1 ) {
        if( veh->check_heli_descend( player_character ) ) {
            player_character.add_msg_if_player( m_info, _( "You steer the vehicle into a descent." ) );
        } else {
            return;
        }
    } else if( p.z == 1 ) {
        if( veh->check_heli_ascend( player_character ) ) {
            player_character.add_msg_if_player( m_info, _( "You steer the vehicle into an ascent." ) );
        } else {
            return;
        }
    }
    veh->pldrive( get_avatar(), p.xy(), p.z );
}

static void pldrive( point d )
{
    return pldrive( tripoint( d, 0 ) );
}

static void open()
{
    avatar &player_character = get_avatar();
    const std::optional<tripoint> openp_ = choose_adjacent_highlight( _( "Open where?" ),
                                           pgettext( "no door, gate, curtain, etc.", "There is nothing that can be opened nearby." ),
                                           ACTION_OPEN, false );

    if( !openp_ ) {
        return;
    }
    const tripoint openp = *openp_;
    map &here = get_map();

    player_character.moves -= 100;

    // Is a vehicle part here?
    if( const optional_vpart_position vp = here.veh_at( openp ) ) {
        vehicle *const veh = &vp->vehicle();
        // Check for potential thievery, and restore moves if action is canceled
        if( !veh->handle_potential_theft( player_character ) ) {
            player_character.moves += 100;
            return;
        }
        // Check if vehicle has a part here that can be opened
        int openable = veh->next_part_to_open( vp->part_index() );
        if( openable >= 0 ) {
            // If player is inside vehicle, open the door/window/curtain
            const vehicle *player_veh = veh_pointer_or_null( here.veh_at( player_character.pos() ) );
            const std::string part_name = veh->part( openable ).name();
            bool outside = !player_veh || player_veh != veh;
            if( !outside ) {
                veh->open( openable );
                //~ %1$s - vehicle name, %2$s - part name
                player_character.add_msg_if_player( _( "You open the %1$s's %2$s." ), veh->name, part_name );
            } else {
                // Outside means we check if there's anything in that tile outside-openable.
                // If there is, we open everything on tile. This means opening a closed,
                // curtained door from outside is possible, but it will magically open the
                // curtains as well.
                int outside_openable = veh->next_part_to_open( vp->part_index(), true );
                if( outside_openable == -1 ) {
                    add_msg( m_info, _( "That %s can only be opened from the inside." ), part_name );
                    player_character.moves += 100;
                } else {
                    veh->open_all_at( openable );
                    //~ %1$s - vehicle name, %2$s - part name
                    player_character.add_msg_if_player( _( "You open the %1$s's %2$s." ), veh->name, part_name );
                }
            }
        } else {
            // If there are any OPENABLE parts here, they must be already open or locked
            if( const std::optional<vpart_reference> openable_part = vp.part_with_feature( "OPENABLE",
                    true ); openable_part.has_value() ) {
                const std::string name = openable_part->info().name();
                if( vp->vehicle().part( openable_part->part_index() ).locked ) {
                    add_msg( m_info, _( "That %s is locked." ), name );
                } else {
                    add_msg( m_info, _( "That %s is already open." ), name );
                }
            }
            player_character.moves += 100;
        }
        return;
    }
    // Not a vehicle part, just a regular door
    bool didit = here.open_door( player_character, openp, !here.is_outside( player_character.pos() ) );
    if( didit ) {
        player_character.add_msg_if_player( _( "You open the %s." ), here.name( openp ) );
    } else {
        const ter_str_id tid = here.ter( openp ).id();

        if( here.has_flag( ter_furn_flag::TFLAG_LOCKED, openp ) ) {
            add_msg( m_info, _( "The door is locked!" ) );
            return;
        } else if( tid.obj().close ) {
            // if the following message appears unexpectedly, the prior check was for t_door_o
            add_msg( m_info, _( "That door is already open." ) );
            player_character.moves += 100;
            return;
        }
        add_msg( m_info, _( "No door there." ) );
        player_character.moves += 100;
    }
}

static void close()
{
    if( const std::optional<tripoint> pnt = choose_adjacent_highlight( _( "Close where?" ),
                                            pgettext( "no door, gate, etc.", "There is nothing that can be closed nearby." ),
                                            ACTION_CLOSE, false ) ) {
        doors::close_door( get_map(), get_player_character(), *pnt );
    }
}

// Establish or release a grab on a vehicle
static void grab()
{
    avatar &you = get_avatar();
    map &here = get_map();

    if( you.get_grab_type() != object_type::NONE ) {
        if( const optional_vpart_position vp = here.veh_at( you.pos() + you.grab_point ) ) {
            add_msg( _( "You release the %s." ), vp->vehicle().name );
        } else if( here.has_furn( you.pos() + you.grab_point ) ) {
            add_msg( _( "You release the %s." ), here.furnname( you.pos() + you.grab_point ) );
        }

        you.grab( object_type::NONE );
        return;
    }

    const std::optional<tripoint> grabp_ = choose_adjacent( _( "Grab where?" ) );
    if( !grabp_ ) {
        add_msg( _( "Never mind." ) );
        return;
    }
    tripoint grabp = *grabp_;

    if( grabp == you.pos() ) {
        add_msg( _( "You get a hold of yourself." ) );
        you.grab( object_type::NONE );
        return;
    }

    // Object might not be on the same z level if on a ramp.
    if( !( here.veh_at( grabp ) || here.has_furn( grabp ) ) ) {
        if( here.has_flag( ter_furn_flag::TFLAG_RAMP_UP, grabp ) ||
            here.has_flag( ter_furn_flag::TFLAG_RAMP_UP, you.pos() ) ) {
            grabp.z += 1;
        } else if( here.has_flag( ter_furn_flag::TFLAG_RAMP_DOWN, grabp ) ||
                   here.has_flag( ter_furn_flag::TFLAG_RAMP_DOWN, you.pos() ) ) {
            grabp.z -= 1;
        }
    }

    if( const optional_vpart_position vp = here.veh_at( grabp ) ) {
        if( !vp->vehicle().handle_potential_theft( you ) ) {
            return;
        }
        if( vp->vehicle().has_tag( flag_CANT_DRAG ) ) {
            add_msg( m_info, _( "There's nothing to grab there!" ) );
            return;
        }
        you.grab( object_type::VEHICLE, grabp - you.pos() );
        add_msg( _( "You grab the %s." ), vp->vehicle().name );
    } else if( here.has_furn( grabp ) ) { // If not, grab furniture if present
        if( !here.furn( grabp ).obj().is_movable() ) {
            add_msg( _( "You can not grab the %s." ), here.furnname( grabp ) );
            return;
        }
        you.grab( object_type::FURNITURE, grabp - you.pos() );
        if( !here.can_move_furniture( grabp, &you ) ) {
            add_msg( _( "You grab the %s. It feels really heavy." ), here.furnname( grabp ) );
        } else {
            add_msg( _( "You grab the %s." ), here.furnname( grabp ) );
        }
    } else { // TODO: grab mob? Captured squirrel = pet (or meat that stays fresh longer).
        add_msg( m_info, _( "There's nothing to grab there!" ) );
    }
}

static void haul()
{
    Character &player_character = get_player_character();
    map &here = get_map();

    if( player_character.is_hauling() ) {
        player_character.stop_hauling();
    } else {
        if( here.veh_at( player_character.pos() ) ) {
            add_msg( m_info, _( "You cannot haul inside vehicles." ) );
        } else if( here.has_flag( ter_furn_flag::TFLAG_DEEP_WATER, player_character.pos() ) ) {
            add_msg( m_info, _( "You cannot haul while in deep water." ) );
        } else if( !here.can_put_items( player_character.pos() ) ) {
            add_msg( m_info, _( "You cannot haul items here." ) );
        } else if( !here.has_haulable_items( player_character.pos() ) ) {
            add_msg( m_info, _( "There are no items to haul here." ) );
        } else {
            player_character.start_hauling();
        }
    }
}

static void smash()
{
    avatar &player_character = get_avatar();
    map &here = get_map();
    if( player_character.is_mounted() ) {
        auto *mons = player_character.mounted_creature.get();
        if( mons->has_flag( mon_flag_RIDEABLE_MECH ) ) {
            if( !mons->check_mech_powered() ) {
                add_msg( m_bad, _( "Your %s refuses to move as its batteries have been drained." ),
                         mons->get_name() );
                return;
            }
        }
    }
    const int move_cost = !player_character.is_armed() ? 80 :
                          player_character.get_wielded_item()->attack_time( player_character ) *
                          0.8;
    bool mech_smash = false;
    int smashskill = player_character.get_arm_str();
    ///\EFFECT_STR increases smashing capability
    if( player_character.is_mounted() ) {
        auto *mon = player_character.mounted_creature.get();
        smashskill += mon->mech_str_addition() + mon->type->melee_dice * mon->type->melee_sides;
        mech_smash = true;
    } else if( player_character.get_wielded_item() ) {
        smashskill += player_character.get_wielded_item()->damage_melee( damage_bash );
    }
    smashskill = player_character.calculate_by_enchantment( smashskill, enchant_vals::mod::MELEE_DAMAGE,
                 true );
    smashskill = player_character.calculate_by_enchantment( smashskill,
                 enchant_vals::mod::ITEM_DAMAGE_BASH,
                 true );

    const bool allow_floor_bash = debug_mode; // Should later become "true"
    const std::optional<tripoint> smashp_ = choose_adjacent( _( "Smash where?" ), allow_floor_bash );
    if( !smashp_ ) {
        return;
    }
    tripoint smashp = *smashp_;

    bool smash_floor = false;
    if( smashp.z != player_character.posz() ) {
        if( smashp.z > player_character.posz() ) {
            // TODO: Knock on the ceiling
            return;
        }

        smashp.z = player_character.posz();
        smash_floor = true;
    }
    get_event_bus().send<event_type::character_smashes_tile>(
        player_character.getID(), here.ter( smashp ).id(), here.furn( smashp ).id() );
    if( player_character.is_mounted() ) {
        monster *crit = player_character.mounted_creature.get();
        if( crit->has_flag( mon_flag_RIDEABLE_MECH ) ) {
            crit->use_mech_power( 3_kJ );
        }
    }
    for( std::pair<const field_type_id, field_entry> &fd_to_smsh : here.field_at( smashp ) ) {
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
            here.remove_field( smashp, fd_to_smsh.first );
            here.spawn_items( smashp, item_group::items_from( bash_info.drop_group, calendar::turn ) );
            player_character.mod_moves( - bash_info.fd_bash_move_cost );
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
    for( const item &maybe_corpse : here.i_at( smashp ) ) {
        if( maybe_corpse.is_corpse() && maybe_corpse.damage() < maybe_corpse.max_damage() &&
            maybe_corpse.can_revive() ) {
            if( maybe_corpse.get_mtype()->bloodType()->has_acid &&
                !player_character.is_immune_field( fd_acid ) ) {
                if( !query_yn( _( "Are you sure you want to pulp an acid filled corpse?" ) ) ) {
                    return; // Player doesn't want an acid bath
                }
            }
            should_pulp = true; // There is at least one corpse to pulp
        }
    }

    if( should_pulp ) {
        // do activity forever. ACT_PULP stops itself
        player_character.assign_activity( ACT_PULP, calendar::INDEFINITELY_LONG, 0 );
        player_character.activity.placement = here.getglobal( smashp );
        return; // don't smash terrain if we've smashed a corpse
    }

    vehicle *veh = veh_pointer_or_null( here.veh_at( smashp ) );
    if( veh != nullptr ) {
        if( !veh->handle_potential_theft( player_character ) ) {
            return;
        }
    }

    if( !player_character.has_weapon() ) {
        const bodypart_id bp_null( "bp_null" );
        std::pair<bodypart_id, int> best_part_to_smash = {bp_null, 0};
        int tmp_bash_armor = 0;
        for( const bodypart_id &bp : player_character.get_all_body_parts() ) {
            tmp_bash_armor += player_character.worn.damage_resist( damage_bash, bp );
            for( const trait_id &mut : player_character.get_mutations() ) {
                const resistances &res = mut->damage_resistance( bp );
                tmp_bash_armor += std::floor( res.type_resist( damage_bash ) );
            }
            if( tmp_bash_armor > best_part_to_smash.second ) {
                best_part_to_smash = {bp, tmp_bash_armor};
            }
        }
        if( best_part_to_smash.first != bp_null && here.is_bashable( smashp ) ) {
            std::string name_to_bash = _( "thing" );
            if( here.is_bashable_furn( smashp ) ) {
                name_to_bash = here.furnname( smashp );
            } else if( here.is_bashable_ter( smashp ) ) {
                name_to_bash = here.tername( smashp );
            }
            if( !best_part_to_smash.first->smash_message.empty() ) {
                add_msg( best_part_to_smash.first->smash_message, name_to_bash );
            } else {
                add_msg( _( "You use your %s to smash the %s." ),
                         body_part_name_accusative( best_part_to_smash.first ), name_to_bash );
            }
        }
        const int min_smashskill = smashskill * best_part_to_smash.first->smash_efficiency;
        const int max_smashskill = smashskill * ( 1.0f + best_part_to_smash.first->smash_efficiency );
        smashskill = std::min( best_part_to_smash.second + min_smashskill, max_smashskill );
    }
    const bash_params bash_result = here.bash( smashp, smashskill, false, false, smash_floor );
    // Weariness scaling
    float weary_mult = 1.0f;
    item_location weapon = player_character.used_weapon();
    if( bash_result.did_bash ) {
        if( !mech_smash ) {
            player_character.set_activity_level( MODERATE_EXERCISE );
            player_character.handle_melee_wear( weapon );
            weary_mult = 1.0f / player_character.exertion_adjusted_move_multiplier( MODERATE_EXERCISE );

            const int mod_sta = 2 * player_character.get_standard_stamina_cost();
            player_character.mod_stamina( mod_sta );

            if( static_cast<int>( player_character.get_skill_level( skill_melee ) ) == 0 ) {
                player_character.practice( skill_melee, rng( 0, 1 ) * rng( 0, 1 ) );
            }
            if( weapon ) {
                const int glass_portion = weapon->made_of( material_glass );
                float glass_fraction = glass_portion / static_cast<float>( weapon->type->mat_portion_total );
                if( std::isnan( glass_fraction ) || glass_fraction > 1.f ) {
                    glass_fraction = 0.f;
                }
                const int vol = weapon->volume() * glass_fraction / units::legacy_volume_factor;
                if( glass_portion && rng( 0, vol + 3 ) < vol ) {
                    add_msg( m_bad, _( "Your %s shatters!" ), weapon->tname() );
                    weapon->spill_contents( player_character.pos() );
                    sounds::sound( player_character.pos(), 24, sounds::sound_t::combat, "CRACK!", true, "smash",
                                   "glass" );
                    player_character.deal_damage( nullptr, bodypart_id( "hand_r" ), damage_instance( damage_cut,
                                                  rng( 0,
                                                       vol ) ) );
                    if( vol > 20 ) {
                        // Hurt left arm too, if it was big
                        player_character.deal_damage( nullptr, bodypart_id( "hand_l" ), damage_instance( damage_cut,
                                                      rng( 0,
                                                           static_cast<int>( vol * .5 ) ) ) );
                    }
                    player_character.remove_weapon();
                    player_character.check_dead_state();
                }
            }
        }
        player_character.moves -= move_cost * weary_mult;
        player_character.recoil = MAX_RECOIL;

        if( bash_result.success ) {
            // Bash results in destruction of target
            g->draw_async_anim( smashp, "bash_complete", "X", c_light_gray );
        } else if( smashskill >= here.bash_resistance( smashp ) ) {
            // Bash effective but target not yet destroyed
            g->draw_async_anim( smashp, "bash_effective", "/", c_light_gray );
        } else {
            // Bash not effective
            g->draw_async_anim( smashp, "bash_ineffective" );
            if( one_in( 10 ) ) {
                if( here.has_furn( smashp ) && here.furn( smashp ).obj().bash.str_min != -1 ) {
                    // %s is the smashed furniture
                    add_msg( m_neutral, _( "You don't seem to be damaging the %s." ), here.furnname( smashp ) );
                } else {
                    // %s is the smashed terrain
                    add_msg( m_neutral, _( "You don't seem to be damaging the %s." ), here.tername( smashp ) );
                }
            }
        }

    } else {
        add_msg( _( "There's nothing there to smash!" ) );
    }
}

static int try_set_alarm()
{
    uilist as_m;
    const bool already_set = get_player_character().has_effect( effect_alarm_clock );

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
    Character &player_character = get_player_character();
    bool setting_alarm = false;
    map &here = get_map();

    if( player_character.controlling_vehicle ) {
        const vehicle &veh = here.veh_at( player_character.pos() )->vehicle();
        if( !veh.can_use_rails() && (   // control optional if on rails
                veh.is_flying_in_air() ||   // control required: fuel is consumed even at hover
                veh.is_falling ||           // *not* vertical_velocity, which is only used for collisions
                veh.velocity ||             // is moving
                ( veh.cruise_velocity && (  // would move if it could
                      ( veh.is_watercraft() && veh.can_float() ) || // is viable watercraft floating on water
                      veh.sufficient_wheel_config() // is viable land vehicle on ground or fording shallow water
                  ) ) ||
                ( veh.is_in_water( true ) && !veh.can_float() ) // is sinking in deep water
            ) ) {
            popup( _( "You can't pass time while controlling a moving vehicle." ) );
            return;
        }
    }

    if( player_character.has_alarm_clock() ) {
        int alarm_query = try_set_alarm();
        if( alarm_query == UILIST_CANCEL ) {
            return;
        }
        setting_alarm = alarm_query == 1;
    }

    const bool has_watch = player_character.has_watch() || setting_alarm;

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
        if( player_character.get_stamina() < player_character.get_stamina_max() ) {
            as_m.addentry( 14, true, 'w', _( "Wait until you catch your breath" ) );
            durations.emplace( 14, 15_minutes ); // to hide it from showing
        }
        add_menu_item( 1, '1', !has_watch ? _( "Wait 20 heartbeats" ) : "", 20_seconds );
        add_menu_item( 2, '2', !has_watch ? _( "Wait 60 heartbeats" ) : "", 1_minutes );
        add_menu_item( 3, '3', !has_watch ? _( "Wait 300 heartbeats" ) : "", 5_minutes );
        add_menu_item( 4, '4', !has_watch ? _( "Wait 1800 heartbeats" ) : "", 30_minutes );

        if( has_watch ) {
            add_menu_item( 5, '5', "", 1_hours );
            add_menu_item( 6, '6', "", 2_hours );
            add_menu_item( 7, '7', "", 3_hours );
            add_menu_item( 8, '8', "", 6_hours );
        }
    }

    if( here.get_abs_sub().z() >= 0 || has_watch ) {
        const time_point last_midnight = calendar::turn - time_past_midnight( calendar::turn );
        const auto diurnal_time_before = []( const time_point & p ) {
            // Either the given time is in the future (e.g. waiting for sunset while it's early morning),
            // than use it directly. Otherwise (in the past), add a single day to get the same time tomorrow
            // (e.g. waiting for sunrise while it's noon).
            const time_point target_time = p > calendar::turn ? p : p + 1_days;
            return target_time - calendar::turn;
        };

        add_menu_item( 9,  'd',
                       setting_alarm ? _( "Set alarm for dawn" ) : _( "Wait till daylight" ),
                       diurnal_time_before( daylight_time( calendar::turn ) ) );
        add_menu_item( 10,  'n',
                       setting_alarm ? _( "Set alarm for noon" ) : _( "Wait till noon" ),
                       diurnal_time_before( last_midnight + 12_hours ) );
        add_menu_item( 11,  'k',
                       setting_alarm ? _( "Set alarm for dusk" ) : _( "Wait till night" ),
                       diurnal_time_before( night_time( calendar::turn ) ) );
        add_menu_item( 12, 'm',
                       setting_alarm ? _( "Set alarm for midnight" ) : _( "Wait till midnight" ),
                       diurnal_time_before( last_midnight ) );
        if( setting_alarm ) {
            if( player_character.has_effect( effect_alarm_clock ) ) {
                add_menu_item( 13, 'x', _( "Cancel the currently set alarm." ),
                               0_turns );
            }
        } else {
            add_menu_item( 13, 'W', _( "Wait till weather changes" ) );
        }
    }

    // NOLINTNEXTLINE(cata-text-style): spaces required for concatenation
    as_m.text = has_watch ? string_format( _( "It's %s now.  " ),
                                           to_string_time_of_day( calendar::turn ) ) : "";
    as_m.text += setting_alarm ? _( "Set alarm for when?" ) : _( "Wait for how long?" );
    as_m.query(); /* calculate key and window variables, generate window, and loop until we get a valid answer */

    const auto dur_iter = durations.find( as_m.ret );
    if( dur_iter == durations.end() ) {
        return;
    }
    const time_duration time_to_wait = dur_iter->second;

    if( setting_alarm ) {
        // Setting alarm
        player_character.remove_effect( effect_alarm_clock );
        if( as_m.ret == 13 ) {
            add_msg( _( "You cancel your alarm." ) );
        } else {
            player_character.add_effect( effect_alarm_clock, time_to_wait );
            add_msg( _( "You set your alarm." ) );
        }

    } else {
        // Waiting
        activity_id actType;
        if( as_m.ret == 13 ) {
            actType = ACT_WAIT_WEATHER;
        } else if( as_m.ret == 14 ) {
            actType = ACT_WAIT_STAMINA;
        } else {
            actType = ACT_WAIT;
        }

        player_activity new_act( actType, 100 * to_turns<int>( time_to_wait ), 0 );

        player_character.assign_activity( new_act );
    }
}

static void sleep()
{
    avatar &player_character = get_avatar();
    if( player_character.is_mounted() ) {
        add_msg( m_info, _( "You cannot sleep while mounted." ) );
        return;
    }

    vehicle *const boat = veh_pointer_or_null( get_map().veh_at( player_character.pos() ) );
    if( get_map().has_flag( ter_furn_flag::TFLAG_DEEP_WATER, player_character.pos() ) &&
        !player_character.has_trait( trait_WATERSLEEPER ) &&
        !player_character.has_trait( trait_WATERSLEEP ) &&
        boat == nullptr ) {
        add_msg( m_info, _( "You cannot sleep while swimming." ) );
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
    for( item_location &it : player_character.all_items_loc() ) {
        if( it->has_flag( flag_LITCIG ) || ( it->active && it->ammo_sufficient( &player_character ) &&
                                             it->is_tool() && !it->has_flag( flag_SLEEP_IGNORE ) ) ) {
            active.push_back( it->tname() );
        }
    }
    for( int i = 0; i < player_character.num_bionics(); i++ ) {
        const bionic &bio = player_character.bionic_at_index( i );
        if( !bio.powered ) {
            continue;
        }

        // some bionics
        // bio_alarm is useful for waking up during sleeping
        if( bio.info().has_flag( STATIC( json_character_flag( "BIONIC_SLEEP_FRIENDLY" ) ) ) ) {
            continue;
        }

        const bionic_data &info = bio.info();
        if( info.power_over_time > 0_kJ ) {
            active.push_back( info.name.translated() );
        }
    }
    for( auto &mut : player_character.get_mutations() ) {
        const mutation_branch &mdata = mut.obj();
        if( mdata.cost > 0 && player_character.has_active_mutation( mut ) ) {
            active.push_back( player_character.mutation_name( mut ) );
        }
    }

    // check for deactivating any currently played music instrument.
    for( item *&item : player_character.inv_dump() ) {
        if( item->active && item->get_use( "musical_instrument" ) != nullptr ) {
            player_character.add_msg_if_player( _( "You stop playing your %s before trying to sleep." ),
                                                item->tname() );
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
    if( player_character.is_deaf() && !player_character.has_flag( json_flag_ALARMCLOCK ) ) {
        deaf_text = _( "<color_c_red> (DEAF!)</color>" );
    }
    if( player_character.has_alarm_clock() ) {
        /* Reuse menu to ask player whether they want to set an alarm. */
        bool can_hibernate = player_character.get_hunger() < -60 &&
                             player_character.has_active_mutation( trait_HIBERNATE );

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
            player_character.add_effect( effect_alarm_clock, 1_hours * as_m.ret );
            try_sleep_dur = 1_hours * as_m.ret + 1_turns;
        } else if( as_m.ret < 0 ) {
            return;
        }
    }

    player_character.moves = 0;
    player_character.try_to_sleep( try_sleep_dur );
}

static void loot()
{
    enum ZoneFlags {
        None = 1,
        SortLoot = 2,
        SortLootStatic = 4,
        SortLootPersonal = 8,
        FertilizePlots = 16,
        ConstructPlots = 64,
        MultiFarmPlots = 128,
        Multichoptrees = 256,
        Multichopplanks = 512,
        Multideconvehicle = 1024,
        Multirepairvehicle = 2048,
        MultiButchery = 4096,
        MultiMining = 8192,
        MultiDis = 16384,
        MultiMopping = 32768,
        UnloadLoot = 65536
    };

    Character &player_character = get_player_character();
    int flags = 0;
    zone_manager &mgr = zone_manager::get_manager();
    const bool has_fertilizer = player_character.has_item_with_flag( flag_FERTILIZER );

    // reset any potentially disabled zones from a past activity
    mgr.reset_disabled();

    // cache should only happen if we have personal zones defined
    if( mgr.has_personal_zones() ) {
        mgr.cache_data();
    }

    // Manually update vehicle cache.
    // In theory this would be handled by the related activity (activity_on_turn_move_loot())
    // but with a stale cache we never get that far.
    mgr.cache_vzones();

    flags |= g->check_near_zone( zone_type_LOOT_UNSORTED,
                                 player_character.pos() ) ? SortLoot : 0;
    flags |= g->check_near_zone( zone_type_zone_unload_all, player_character.pos() ) ||
             g->check_near_zone( zone_type_zone_strip, player_character.pos() ) ? UnloadLoot : 0;
    if( g->check_near_zone( zone_type_FARM_PLOT, player_character.pos() ) ) {
        flags |= FertilizePlots;
        flags |= MultiFarmPlots;
    }
    flags |= g->check_near_zone( zone_type_CONSTRUCTION_BLUEPRINT,
                                 player_character.pos() ) ? ConstructPlots : 0;

    flags |= g->check_near_zone( zone_type_CHOP_TREES,
                                 player_character.pos() ) ? Multichoptrees : 0;
    flags |= g->check_near_zone( zone_type_LOOT_WOOD,
                                 player_character.pos() ) ? Multichopplanks : 0;
    flags |= g->check_near_zone( zone_type_VEHICLE_DECONSTRUCT,
                                 player_character.pos() ) ? Multideconvehicle : 0;
    flags |= g->check_near_zone( zone_type_VEHICLE_REPAIR,
                                 player_character.pos() ) ? Multirepairvehicle : 0;
    flags |= g->check_near_zone( zone_type_LOOT_CORPSE,
                                 player_character.pos() ) ? MultiButchery : 0;
    flags |= g->check_near_zone( zone_type_MINING, player_character.pos() ) ? MultiMining : 0;
    flags |= g->check_near_zone( zone_type_zone_disassemble,
                                 player_character.pos() ) ? MultiDis : 0;
    flags |= g->check_near_zone( zone_type_MOPPING, player_character.pos() ) ? MultiMopping : 0;
    if( flags == 0 ) {
        add_msg( m_info, _( "There is no compatible zone nearby." ) );
        add_msg( m_info, _( "Compatible zones are %s and %s" ),
                 mgr.get_name_from_type( zone_type_LOOT_UNSORTED ),
                 mgr.get_name_from_type( zone_type_FARM_PLOT ) );
        return;
    }

    uilist menu;
    menu.text = _( "Pick action:" );
    menu.desc_enabled = true;

    if( flags & SortLoot ) {
        menu.addentry_desc( SortLootStatic, true, 'o', _( "Sort out my loot (static zones only)" ),
                            _( "Sorts out the loot from Loot: Unsorted zone to nearby appropriate Loot zones ignoring personal zones.  Uses empty space in your inventory or utilizes a cart, if you are holding one." ) );
    }

    if( flags & SortLoot ) {
        menu.addentry_desc( SortLootPersonal, true, 'O', _( "Sort out my loot (personal zones only)" ),
                            _( "Sorts out the loot from Loot: Unsorted zone to nearby appropriate Loot zones ignoring static zones.  Uses empty space in your inventory or utilizes a cart, if you are holding one." ) );
    }

    if( flags & SortLoot ) {
        menu.addentry_desc( SortLoot, true, 'I', _( "Sort out my loot (all)" ),
                            _( "Sorts out the loot from Loot: Unsorted zone to nearby appropriate Loot zones.  Uses empty space in your inventory or utilizes a cart, if you are holding one." ) );
    }

    if( flags & UnloadLoot ) {
        menu.addentry_desc( UnloadLoot, true, 'U', _( "Unload nearby containers" ),
                            _( "Unloads any corpses or containers that are in their respective zones." ) );
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
    if( flags & MultiDis ) {
        menu.addentry_desc( MultiDis, true, 'D', _( "Disassemble items" ),
                            _( "Auto-disassemble anything in disassembly zone - auto-fetch tools." ) );

    }
    if( flags & MultiMopping ) {
        menu.addentry_desc( MultiMopping, true, 'p', _( "Mop area" ), _( "Mop clean the area." ) );
    }

    menu.query();
    flags = ( menu.ret >= 0 ) ? menu.ret : None;
    bool recache = false;

    switch( flags ) {
        case None:
            add_msg( _( "Never mind." ) );
            break;
        case SortLoot:
            player_character.assign_activity( ACT_MOVE_LOOT );
            break;
        case SortLootStatic:
            //temporarily disable personal zones
            for( const auto &i : mgr.get_zones() ) {
                zone_data &zone = i.get();
                if( zone.get_is_personal() && zone.get_enabled() ) {
                    zone.set_enabled( false );
                    zone.set_temporary_disabled( true );
                    recache = true;
                }
            }
            if( recache ) {
                mgr.cache_data();
            }
            player_character.assign_activity( ACT_MOVE_LOOT );
            break;
        case SortLootPersonal:
            //temporarily disable non personal zones
            for( const auto &i : mgr.get_zones() ) {
                zone_data &zone = i.get();
                if( !zone.get_is_personal() && zone.get_enabled() ) {
                    zone.set_enabled( false );
                    zone.set_temporary_disabled( true );
                    recache = true;
                }
            }
            if( recache ) {
                mgr.cache_data();
            }
            player_character.assign_activity( ACT_MOVE_LOOT );
            break;
        case UnloadLoot:
            player_character.assign_activity( unload_loot_activity_actor() );
            break;
        case FertilizePlots:
            player_character.assign_activity( ACT_FERTILIZE_PLOT );
            break;
        case ConstructPlots:
            player_character.assign_activity( ACT_MULTIPLE_CONSTRUCTION );
            break;
        case MultiFarmPlots:
            player_character.assign_activity( ACT_MULTIPLE_FARM );
            break;
        case Multichoptrees:
            player_character.assign_activity( ACT_MULTIPLE_CHOP_TREES );
            break;
        case Multichopplanks:
            player_character.assign_activity( ACT_MULTIPLE_CHOP_PLANKS );
            break;
        case Multideconvehicle:
            player_character.assign_activity( ACT_VEHICLE_DECONSTRUCTION );
            break;
        case Multirepairvehicle:
            player_character.assign_activity( ACT_VEHICLE_REPAIR );
            break;
        case MultiButchery:
            player_character.assign_activity( ACT_MULTIPLE_BUTCHER );
            break;
        case MultiMining:
            player_character.assign_activity( ACT_MULTIPLE_MINE );
            break;
        case MultiDis:
            player_character.assign_activity( ACT_MULTIPLE_DIS );
            break;
        case MultiMopping:
            player_character.assign_activity( ACT_MULTIPLE_MOP );
            break;
        default:
            debugmsg( "Unsupported flag" );
            break;
    }
}

static void wear()
{
    avatar &player_character = get_avatar();
    item_location loc = game_menus::inv::wear( player_character );

    if( loc ) {
        player_character.wear( loc );
    } else {
        add_msg( _( "Never mind." ) );
    }
}

static void takeoff()
{
    avatar &player_character = get_avatar();
    item_location loc = game_menus::inv::take_off( player_character );

    if( loc ) {
        player_character.takeoff( loc.obtain( player_character ) );
    } else {
        add_msg( _( "Never mind." ) );
    }
}

static void read()
{
    avatar &player_character = get_avatar();
    // Can read items from inventory or within one tile (including in vehicles)
    item_location loc = game_menus::inv::read( player_character );

    if( loc ) {
        if( loc->type->can_use( "learn_spell" ) ) {
            item spell_book = *loc.get_item();
            spell_book.get_use( "learn_spell" )->call( &player_character, spell_book,
                    player_character.pos() );
        } else {
            loc = loc.obtain( player_character );
            player_character.read( loc );
        }
    } else {
        add_msg( _( "Never mind." ) );
    }
}

// Perform a reach attach using wielded weapon
static void reach_attack( avatar &you )
{
    g->temp_exit_fullscreen();

    target_handler::trajectory traj = target_handler::mode_reach( you, you.get_wielded_item() );

    if( !traj.empty() ) {
        you.reach_attack( traj.back() );
    }
    g->reenter_fullscreen();
}

static void fire()
{
    avatar &you = get_avatar();
    map &here = get_map();

    if( !you.try_break_relax_gas( _( "Your willpower asserts itself, and so do you!" ),
                                  _( "You're too pacified to strike anything…" ) ) ) {
        return;
    }

    const item_location weapon = you.get_wielded_item();
    // try reach weapon
    if( weapon && !weapon->is_gun() && weapon->current_reach_range( you ) > 1 ) {
        reach_attack( you );
        return;
    }
    if( you.has_trait( trait_BRAWLER ) ) {
        add_msg( m_bad, _( "You refuse to use ranged weapons." ) );
        return;
    }
    // try firing gun
    if( weapon && weapon->is_gun() && !weapon->gun_current_mode().melee() ) {
        avatar_action::fire_wielded_weapon( you );
        return;
    }
    // try firing turrets
    if( const optional_vpart_position ovp = here.veh_at( you.pos() ) ) {
        if( turret_data turret_here = ovp->vehicle().turret_query( you.pos() ) ) {
            if( avatar_action::fire_turret_manual( you, here, turret_here ) ) {
                return;
            }
        } else if( ovp.part_with_feature( VPFLAG_CONTROLS, true ) ) {
            if( ovp->vehicle().turrets_aim_and_fire_all_manual() ) {
                return;
            }
        }
    }
    // offer to draw a gun from worn holster
    std::vector<std::string> options;
    std::vector<std::function<void()>> actions;
    you.worn.fire_options( you, options, actions );
    if( !options.empty() ) {
        int sel = uilist( _( "Draw what?" ), options );
        if( sel >= 0 ) {
            actions[sel]();
            return;
        }
    }
}

static void open_movement_mode_menu()
{
    avatar &player_character = get_avatar();
    const std::vector<move_mode_id> &modes = move_modes_by_speed();
    const int cycle = 1027;
    uilist as_m;

    as_m.text = _( "Change to which movement mode?" );

    for( size_t i = 0; i < modes.size(); ++i ) {
        const move_mode_id &curr = modes[i];
        as_m.entries.emplace_back( static_cast<int>( i ), player_character.can_switch_to( curr ),
                                   curr->letter(),
                                   curr->name() );
    }
    as_m.entries.emplace_back( cycle,
                               player_character.can_switch_to( player_character.current_movement_mode()->cycle() ),
                               hotkey_for_action( ACTION_OPEN_MOVEMENT, /*maximum_modifier_count=*/1 ),
                               _( "Cycle move mode" ) );
    // This should select the middle move mode
    as_m.selected = std::floor( modes.size() / 2 );
    as_m.query();

    if( as_m.ret != UILIST_CANCEL ) {
        if( as_m.ret == cycle ) {
            player_character.cycle_move_mode();
        } else {
            player_character.set_movement_mode( modes[as_m.ret] );
        }
    }
}

static void cast_spell()
{
    Character &player_character = get_player_character();
    player_character.magic->clear_opens_spellbook_data();
    get_event_bus().send<event_type::opens_spellbook>( player_character.getID() ); // trigger EoC
    player_character.magic->evaluate_opens_spellbook_data();
    std::vector<spell_id> spells = player_character.magic->spells();

    if( spells.empty() ) {
        add_msg( game_message_params{ m_bad, gmf_bypass_cooldown },
                 _( "You don't know any spells to cast." ) );
        return;
    }

    bool can_cast_spells = false;
    for( const spell_id &sp : spells ) {
        spell &temp_spell = player_character.magic->get_spell( sp );
        if( temp_spell.can_cast( player_character ) ) {
            can_cast_spells = true;
        }
    }

    if( !can_cast_spells ) {
        add_msg( game_message_params{ m_bad, gmf_bypass_cooldown },
                 _( "You can't cast any of the spells you know!" ) );
    }

    const int spell_index = player_character.magic->select_spell( player_character );
    if( spell_index < 0 ) {
        return;
    }

    spell &sp = *player_character.magic->get_spells()[spell_index];

    player_character.cast_spell( sp, false, std::nullopt );
}

// returns true if the spell was assigned
bool Character::cast_spell( spell &sp, bool fake_spell,
                            const std::optional<tripoint> &target = std::nullopt )
{
    if( is_armed() && !sp.has_flag( spell_flag::NO_HANDS ) && !has_flag( json_flag_SUBTLE_SPELL ) &&
        !get_wielded_item()->has_flag( flag_MAGIC_FOCUS ) && !sp.check_if_component_in_hand( *this ) ) {
        add_msg( game_message_params{ m_bad, gmf_bypass_cooldown },
                 _( "You need your hands free to cast this spell!" ) );
        return false;
    }

    if( !magic->has_enough_energy( *this, sp ) ) {
        add_msg( game_message_params{ m_bad, gmf_bypass_cooldown },
                 _( "You don't have enough %s to cast the spell." ),
                 sp.energy_string() );
        return false;
    }

    if( !sp.has_flag( spell_flag::NO_HANDS ) && !has_flag( json_flag_SUBTLE_SPELL ) &&
        has_effect( effect_stunned ) ) {
        add_msg( game_message_params{ m_bad, gmf_bypass_cooldown },
                 _( "You can't focus enough to cast spell." ) );
        return false;
    }

    if( sp.energy_source() == magic_energy_type::hp && !has_quality( qual_CUT ) ) {
        add_msg( game_message_params{ m_bad, gmf_bypass_cooldown },
                 _( "You cannot cast Blood Magic without a cutting implement." ) );
        return false;
    }

    player_activity spell_act( ACT_SPELLCASTING, sp.casting_time( *this ) );
    // [0] this is used as a spell level override for items casting spells
    if( fake_spell ) {
        spell_act.values.emplace_back( sp.get_level() );
    } else {
        spell_act.values.emplace_back( -1 );
    }
    // [1] if this value is 1, the spell never fails
    spell_act.values.emplace_back( 0 );
    // [2] this value overrides the mana cost if set to 0
    spell_act.values.emplace_back( 1 );
    spell_act.name = sp.id().c_str();
    if( magic->casting_ignore ) {
        const std::vector<distraction_type> ignored_distractions = {
            distraction_type::noise,
            distraction_type::pain,
            distraction_type::attacked,
            distraction_type::hostile_spotted_near,
            distraction_type::hostile_spotted_far,
            distraction_type::talked_to,
            distraction_type::asthma,
            distraction_type::motion_alarm,
            distraction_type::weather_change,
            distraction_type::mutation
        };
        for( const distraction_type ignored : ignored_distractions ) {
            spell_act.ignore_distraction( ignored );
        }
    }
    if( target ) {
        spell_act.coords.emplace_back( get_map().getabs( *target ) );
    }
    assign_activity( spell_act );
    return true;
}

// this is here because it shares some things in common with cast_spell
bool bionic::activate_spell( Character &caster ) const
{
    if( !caster.is_avatar() || !id->spell_on_activate ) {
        // the return value tells us if the spell fails. if it has no spell it can't fail
        return true;
    }
    spell sp = id->spell_on_activate->get_spell( caster );
    return caster.cast_spell( sp, true );
}

void game::open_consume_item_menu()
{
    uilist as_m;

    as_m.text = _( "What do you want to consume?" );

    as_m.entries.emplace_back( 0, true, 'f', _( "Food" ) );
    as_m.entries.emplace_back( 1, true, 'd', _( "Drink" ) );
    as_m.entries.emplace_back( 2, true, 'm', _( "Medication" ) );
    as_m.query();

    avatar &player_character = get_avatar();
    switch( as_m.ret ) {
        case 0: {
            item_location loc = game_menus::inv::consume_food( player_character );
            avatar_action::eat( player_character, loc );
            break;
        }
        case 1: {
            item_location loc = game_menus::inv::consume_drink( player_character );
            avatar_action::eat( player_character, loc );
            break;
        }
        case 2:
            avatar_action::eat_or_use( player_character, game_menus::inv::consume_meds( player_character ) );
            break;
        default:
            break;
    }
}

static void handle_debug_mode()
{
    auto debug_mode_setup = []( uilist_entry & entry ) -> void {
        entry.txt = string_format( _( "Debug Mode (%1$s)" ), debug_mode ? _( "ON" ) : _( "OFF" ) );
        entry.text_color = debug_mode ? c_green : c_light_gray;
    };

    // returns if entry became active
    auto debugmode_entry_setup = []( uilist_entry & entry, bool active ) -> void {
        if( active )
        {
            entry.extratxt.txt = _( "A" );
            entry.extratxt.color = c_white_green;
            entry.text_color = c_green;
        } else
        {
            entry.extratxt.txt = " ";
            entry.extratxt.color = c_unset;
            entry.text_color = c_light_gray;
        }
    };

    static bool first_time = true;
    if( first_time ) {
        first_time = false;
        debugmode::enabled_filters.clear();
        for( int i = 0; i < debugmode::DF_LAST; ++i ) {
            debugmode::enabled_filters.emplace_back( static_cast<debugmode::debug_filter>( i ) );
        }
    }

    input_context ctxt( "DEFAULTMODE" );
    ctxt.register_action( "debug_mode" );

    uilist dbmenu;
    dbmenu.allow_anykey = true;
    dbmenu.title = _( "Debug Mode Filters" );
    dbmenu.text = string_format( _( "Press [%1$s] to quickly toggle debug mode." ),
                                 ctxt.get_desc( "debug_mode" ) );

    dbmenu.entries.reserve( 1 + debugmode::DF_LAST );

    dbmenu.addentry( 0, true, 'd', " " );
    debug_mode_setup( dbmenu.entries[0] );

    dbmenu.addentry( 1, true, 't', _( "Toggle all filters" ) );
    bool toggle_value = true;

    for( int i = 0; i < debugmode::DF_LAST; ++i ) {
        uilist_entry entry( i + 2, true, 0,
                            debugmode::filter_name( static_cast<debugmode::debug_filter>( i ) ) );

        entry.extratxt.left = 1;

        const bool active = std::find(
                                debugmode::enabled_filters.begin(), debugmode::enabled_filters.end(),
                                static_cast<debugmode::debug_filter>( i ) ) != debugmode::enabled_filters.end();

        if( toggle_value && active ) {
            toggle_value = false;
        }

        debugmode_entry_setup( entry, active );
        dbmenu.entries.push_back( entry );
    }

    do {
        dbmenu.query();
        if( ctxt.input_to_action( dbmenu.ret_evt ) == "debug_mode" ) {
            debug_mode = !debug_mode;
            if( debug_mode ) {
                add_msg( m_info, _( "Debug mode ON!" ) );
            } else {
                add_msg( m_info, _( "Debug mode OFF!" ) );
            }
            break;
        }

        if( dbmenu.ret == 0 ) {
            debug_mode = !debug_mode;
            debug_mode_setup( dbmenu.entries[0] );

        } else if( dbmenu.ret == 1 ) {
            debugmode::enabled_filters.clear();

            for( int i = 0; i < debugmode::DF_LAST; ++i ) {
                debugmode_entry_setup( dbmenu.entries[i + 2], toggle_value );

                if( toggle_value ) {
                    debugmode::enabled_filters.emplace_back( static_cast<debugmode::debug_filter>( i ) );
                }
            }

            toggle_value = !toggle_value;

        } else if( dbmenu.ret > 1 ) {
            uilist_entry &entry = dbmenu.entries[dbmenu.ret];

            const auto filter_iter = std::find(
                                         debugmode::enabled_filters.begin(), debugmode::enabled_filters.end(),
                                         static_cast<debugmode::debug_filter>( dbmenu.ret - 2 ) );

            const bool active = filter_iter != debugmode::enabled_filters.end();

            debugmode_entry_setup( entry, !active );

            if( active ) {
                debugmode::enabled_filters.erase( filter_iter );
            } else {
                debugmode::enabled_filters.push_back(
                    static_cast<debugmode::debug_filter>( dbmenu.ret - 2 ) );
            }
        }
    } while( dbmenu.ret != UILIST_CANCEL );
}

static bool has_vehicle_control( avatar &player_character )
{
    if( player_character.is_dead_state() ) {
        return false;
    }
    const optional_vpart_position vp = get_map().veh_at( player_character.pos() );
    if( vp && vp->vehicle().player_in_control( player_character ) ) {
        return true;
    }
    return g->remoteveh() != nullptr;
}

static void do_deathcam_action( const action_id &act, avatar &player_character )
{
    switch( act ) {
        case ACTION_TOGGLE_MAP_MEMORY:
            player_character.toggle_map_memory();
            break;

        case ACTION_CENTER:
            player_character.view_offset.x = g->driving_view_offset.x;
            player_character.view_offset.y = g->driving_view_offset.y;
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
            int soffset = get_option<int>( "MOVE_VIEW_OFFSET" );
            player_character.view_offset += g->is_tileset_isometric()
                                            ? shift_delta.at( act ).second * soffset
                                            : shift_delta.at( act ).first * soffset;
        }
        break;

        case ACTION_LOOK:
            g->look_around();
            break;

        case ACTION_KEYBINDINGS: // already handled by input context
        default:
            break;
    }
}


static std::map<action_id, std::string> get_actions_disabled_in_shell()
{
    return std::map<action_id, std::string> {
        { ACTION_OPEN,               _( "You can't open things while you're in your shell." ) },
        { ACTION_CLOSE,              _( "You can't close things while you're in your shell." ) },
        { ACTION_SMASH,              _( "You can't smash things while you're in your shell." ) },
        { ACTION_EXAMINE,            _( "You can't examine your surroundings while you're in your shell." ) },
        { ACTION_EXAMINE_AND_PICKUP, _( "You can't examine your surroundings while you're in your shell." ) },
        { ACTION_ADVANCEDINV,        _( "You can't move mass quantities while you're in your shell." ) },
        { ACTION_PICKUP,             _( "You can't pick anything up while you're in your shell." ) },
        { ACTION_PICKUP_ALL,         _( "You can't pick anything up while you're in your shell." ) },
        { ACTION_GRAB,               _( "You can't grab things while you're in your shell." ) },
        { ACTION_HAUL,               _( "You can't haul things while you're in your shell." ) },
        { ACTION_BUTCHER,            _( "You can't butcher while you're in your shell." ) },
        { ACTION_PEEK,               _( "You can't peek around corners while you're in your shell." ) },
        { ACTION_DROP,               _( "You can't drop things while you're in your shell." ) },
        { ACTION_CRAFT,              _( "You can't craft while you're in your shell." ) },
        { ACTION_RECRAFT,            _( "You can't craft while you're in your shell." ) },
        { ACTION_LONGCRAFT,          _( "You can't craft while you're in your shell." ) },
        { ACTION_DISASSEMBLE,        _( "You can't disassemble while you're in your shell." ) },
        { ACTION_CONSTRUCT,          _( "You can't construct while you're in your shell." ) },
        { ACTION_CONTROL_VEHICLE,    _( "You can't operate a vehicle while you're in your shell." ) },
    };
}

static const std::set<action_id> actions_disabled_in_incorporeal {
    ACTION_OPEN,
    ACTION_CLOSE,
    ACTION_SMASH,
    ACTION_ADVANCEDINV,
    ACTION_PICKUP,
    ACTION_PICKUP_ALL,
    ACTION_GRAB,
    ACTION_HAUL,
    ACTION_BUTCHER,
    ACTION_CRAFT,
    ACTION_RECRAFT,
    ACTION_LONGCRAFT,
    ACTION_DISASSEMBLE,
    ACTION_CONSTRUCT,
    ACTION_CONTROL_VEHICLE,
};

static std::map<action_id, std::string> get_actions_disabled_mounted()
{
    return std::map<action_id, std::string> {
        { ACTION_DISASSEMBLE,        _( "You can't disassemble items while you're riding." ) },
        { ACTION_CONSTRUCT,          _( "You can't construct while you're riding." ) },
        { ACTION_OPEN,               _( "You can't open things while you're riding." ) },
        { ACTION_ADVANCEDINV,        _( "You can't move mass quantities while you're riding." ) },
        { ACTION_PICKUP,             _( "You can't pick anything up while you're riding." ) },
        { ACTION_PICKUP_ALL,         _( "You can't pick anything up while you're riding." ) },
        { ACTION_GRAB,               _( "You can't grab things while you're riding." ) },
        { ACTION_HAUL,               _( "You can't haul things while you're riding." ) },
        { ACTION_BUTCHER,            _( "You can't butcher while you're riding." ) },
        { ACTION_PEEK,               _( "You can't peek around corners while you're riding." ) },
        { ACTION_CRAFT,              _( "You can't craft while you're riding." ) },
        { ACTION_RECRAFT,            _( "You can't craft while you're riding." ) },
        { ACTION_LONGCRAFT,          _( "You can't craft while you're riding." ) },
    };
}

bool game::do_regular_action( action_id &act, avatar &player_character,
                              const std::optional<tripoint> &mouse_target )
{
    item_location weapon = player_character.get_wielded_item();
    const bool in_shell = player_character.has_active_mutation( trait_SHELL2 )
                          || player_character.has_active_mutation( trait_SHELL3 );

    const std::map<action_id, std::string> actions_disabled_mounted = get_actions_disabled_mounted();
    const std::map<action_id, std::string> actions_disabled_in_shell = get_actions_disabled_in_shell();

    if( in_shell && actions_disabled_in_shell.count( act ) > 0 ) {
        add_msg( m_info, actions_disabled_in_shell.at( act ) );
        return true;
    }

    if( u.has_effect( effect_incorporeal ) && actions_disabled_in_incorporeal.count( act ) > 0 ) {
        add_msg( m_info, _( "You lack the substance to affect anything." ) );
        return true;
    }

    if( player_character.is_mounted() && actions_disabled_mounted.count( act ) > 0 ) {
        add_msg( m_info, actions_disabled_mounted.at( act ) );
        return true;
    }

    switch( act ) {
        case ACTION_NULL: // dummy entry
        case NUM_ACTIONS: // dummy entry
        case ACTION_ACTIONMENU: // handled above
        case ACTION_MAIN_MENU:
        case ACTION_KEYBINDINGS:
            break;

        case ACTION_TIMEOUT:
            if( check_safe_mode_allowed( false ) ) {
                player_character.pause();
            }
            break;

        case ACTION_PAUSE:
            if( check_safe_mode_allowed() ) {
                player_character.pause();
            }
            break;

        case ACTION_CYCLE_MOVE:
            player_character.cycle_move_mode();
            break;

        case ACTION_CYCLE_MOVE_REVERSE:
            player_character.cycle_move_mode_reverse();
            break;

        case ACTION_RESET_MOVE:
            player_character.reset_move_mode();
            break;

        case ACTION_TOGGLE_RUN:
            player_character.toggle_run_mode();
            break;

        case ACTION_TOGGLE_CROUCH:
            player_character.toggle_crouch_mode();
            break;

        case ACTION_TOGGLE_PRONE:
            player_character.toggle_prone_mode();
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
            if( !player_character.get_value( "remote_controlling" ).empty() &&
                ( player_character.has_active_item( itype_radiocontrol ) ||
                  player_character.has_active_bionic( bio_remote ) ) ) {
                rcdrive( get_delta_from_movement_action( act, iso_rotate::yes ) );
            } else if( has_vehicle_control( player_character ) ) {
                // vehicle control uses x for steering and y for ac/deceleration,
                // so no rotation needed
                pldrive( get_delta_from_movement_action( act, iso_rotate::no ) );
            } else {
                point dest_delta = get_delta_from_movement_action( act, iso_rotate::yes );
                if( auto_travel_mode && !player_character.is_auto_moving() ) {
                    for( int i = 0; i < SEEX; i++ ) {
                        tripoint_bub_ms auto_travel_destination =
                            player_character.pos_bub() + dest_delta * ( SEEX - i );
                        destination_preview =
                            m.route( player_character.pos_bub(), auto_travel_destination,
                                     player_character.get_pathfinding_settings(),
                                     player_character.get_path_avoid() );
                        if( !destination_preview.empty() ) {
                            destination_preview.erase(
                                destination_preview.begin() + 1, destination_preview.end() );
                            player_character.set_destination( destination_preview );
                            break;
                        }
                    }
                    act = player_character.get_next_auto_move_direction();
                    const point dest_next = get_delta_from_movement_action( act, iso_rotate::yes );
                    if( dest_next == point_zero ) {
                        player_character.clear_destination();
                    }
                    dest_delta = dest_next;
                }
                if( !avatar_action::move( player_character, m, dest_delta ) ) {
                    // auto-move should be canceled due to a failed move or obstacle
                    player_character.clear_destination();
                }

                if( get_option<bool>( "AUTO_FEATURES" ) && get_option<bool>( "AUTO_MOPPING" ) &&
                    weapon && weapon->has_flag( json_flag_MOP ) ) {
                    map &here = get_map();
                    const bool is_blind = player_character.is_blind();
                    for( const tripoint_bub_ms &point : here.points_in_radius( player_character.pos_bub(), 1 ) ) {
                        bool did_mop = false;
                        if( is_blind ) {
                            // blind character have a 1/3 chance of actually mopping
                            if( one_in( 3 ) ) {
                                did_mop = here.mop_spills( point );
                            } else {
                                did_mop = here.terrain_moppable( point );
                            }
                        } else {
                            did_mop = here.mop_spills( point );
                        }
                        // iuse::mop costs 15 moves per use
                        if( did_mop ) {
                            player_character.mod_moves( -15 );
                        }
                    }
                }
            }
            break;
        case ACTION_MOVE_DOWN: {
            if( player_character.is_mounted() ) {
                auto *mon = player_character.mounted_creature.get();
                if( !mon->has_flag( mon_flag_RIDEABLE_MECH ) ) {
                    add_msg( m_info, _( "You can't go down stairs while you're riding." ) );
                    break;
                }
            }

            if( has_vehicle_control( player_character ) ) {
                const optional_vpart_position vp = get_map().veh_at( player_character.pos() );
                if( vp->vehicle().is_rotorcraft() ) {
                    pldrive( tripoint_below );
                    break;
                }
            }

            if( !player_character.in_vehicle ) {
                // We're NOT standing on tiles with stairs, ropes, ladders etc
                if( !m.has_flag( ter_furn_flag::TFLAG_GOES_DOWN, player_character.pos() ) ) {
                    std::vector<tripoint> pts;

                    // Check tiles around player character for open air
                    for( const tripoint &p : m.points_in_radius( player_character.pos(), 1 ) ) {
                        if( m.has_flag( ter_furn_flag::TFLAG_NO_FLOOR, p ) ) {
                            pts.push_back( p );
                        }
                    }

                    // If we found tiles with open air, prompt player with query on direction they want to climb
                    if( !pts.empty() ) {
                        const std::optional<tripoint> pnt = point_selection_menu( pts, false );
                        if( !pnt ) {
                            break;
                        }

                        // If player selected direction, climb down there, and exit from the whole ACTION_MOVE_DOWN case
                        climb_down( *pnt );
                        break;
                    }
                }

                // If we're here, we might or might not be standing on tiles with stairs, ropes, ladders etc
                // In any case, attempt a descend
                vertical_move( -1, false );
            }
            break;
        }

        case ACTION_MOVE_UP:
            if( player_character.is_mounted() ) {
                auto *mon = player_character.mounted_creature.get();
                if( !mon->has_flag( mon_flag_RIDEABLE_MECH ) ) {
                    add_msg( m_info, _( "You can't go up stairs while you're riding." ) );
                    break;
                }
            }
            if( !player_character.in_vehicle ) {
                vertical_move( 1, false );
            } else if( has_vehicle_control( player_character ) ) {
                const optional_vpart_position vp = get_map().veh_at( player_character.pos() );
                if( vp->vehicle().is_rotorcraft() ) {
                    pldrive( tripoint_above );
                }
            }
            break;

        case ACTION_OPEN:
            open();
            break;

        case ACTION_CLOSE:
            if( player_character.is_mounted() ) {
                auto *mon = player_character.mounted_creature.get();
                if( !mon->has_flag( mon_flag_RIDEABLE_MECH ) ) {
                    add_msg( m_info, _( "You can't close things while you're riding." ) );
                }
            } else if( mouse_target ) {
                doors::close_door( m, player_character, *mouse_target );
            } else {
                close();
            }
            break;

        case ACTION_SMASH:
            if( has_vehicle_control( player_character ) ) {
                handbrake();
            } else {
                smash();
            }
            break;

        case ACTION_EXAMINE:
        case ACTION_EXAMINE_AND_PICKUP:
            if( mouse_target ) {
                // Examine including item pickup if ACTION_EXAMINE_AND_PICKUP is used
                examine( *mouse_target, act == ACTION_EXAMINE_AND_PICKUP );
            } else {
                examine( act == ACTION_EXAMINE_AND_PICKUP );
            }
            break;

        case ACTION_ADVANCEDINV:
            create_advanced_inv();
            break;

        case ACTION_PICKUP:
        case ACTION_PICKUP_ALL:
            if( mouse_target ) {
                pickup( *mouse_target );
            } else {
                if( act == ACTION_PICKUP_ALL ) {
                    pickup_all();
                } else {
                    pickup();
                }
            }
            break;

        case ACTION_GRAB:
            grab();
            break;

        case ACTION_HAUL:
            haul();
            break;

        case ACTION_BUTCHER:
            butcher();
            break;

        case ACTION_CHAT:
            chat();
            break;

        case ACTION_PEEK:
            peek();
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
            game_menus::inv::common( player_character );
            break;

        case ACTION_COMPARE:
            game_menus::inv::compare( player_character, std::nullopt );
            break;

        case ACTION_ORGANIZE:
            game_menus::inv::swap_letters( player_character );
            break;

        case ACTION_USE:
            // Shell-users are presumed to be able to mess with their inventories, etc
            // while in the shell.  Eating, gear-changing, and item use are OK.
            avatar_action::use_item( player_character );
            break;

        case ACTION_USE_WIELDED:
            player_character.use_wielded();
            break;

        case ACTION_WEAR:
            wear();
            break;

        case ACTION_TAKE_OFF:
            takeoff();
            break;

        case ACTION_EAT:
            if( !avatar_action::eat_here( player_character ) ) {
                avatar_action::eat_or_use( player_character, game_menus::inv::consume( player_character ) );
            }
            break;

        case ACTION_OPEN_CONSUME:
            if( !avatar_action::eat_here( player_character ) ) {
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
            player_character.martial_arts_data->pick_style( player_character );
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
            avatar_action::unload( player_character );
            break;

        case ACTION_MEND:
            avatar_action::mend( player_character, item_location() );
            break;

        case ACTION_THROW: {
            item_location loc;
            avatar_action::plthrow( player_character, loc );
            break;
        }

        case ACTION_FIRE:
            fire();
            break;

        case ACTION_CAST_SPELL:
            cast_spell();
            break;

        case ACTION_FIRE_BURST: {
            if( weapon ) {
                gun_mode_id original_mode = weapon->gun_get_mode_id();
                if( weapon->gun_set_mode( gun_mode_AUTO ) ) {
                    avatar_action::fire_wielded_weapon( player_character );
                    weapon->gun_set_mode( original_mode );
                }
            }
            break;
        }

        case ACTION_SELECT_FIRE_MODE:
            if( weapon ) {
                if( weapon->is_gun() && !weapon->is_gunmod() &&
                    weapon->gun_all_modes().size() > 1 ) {
                    weapon->gun_cycle_mode();
                } else if( weapon->has_flag( flag_RELOAD_ONE ) ||
                           weapon->has_flag( flag_RELOAD_AND_SHOOT ) ) {
                    item::reload_option opt = player_character.select_ammo( weapon, false );
                    if( !opt ) {
                        break;
                    } else if( player_character.ammo_location && opt.ammo == player_character.ammo_location ) {
                        player_character.ammo_location = item_location();
                    } else {
                        player_character.ammo_location = opt.ammo;
                    }
                }
            }
            break;

        case ACTION_UNLOAD_CONTAINER:
            // You CAN drop things to your own tile while in the shell.
            unload_container();
            break;

        case ACTION_DROP:
            drop_in_direction( player_character.pos() );
            break;
        case ACTION_DIR_DROP:
            if( const std::optional<tripoint> pnt = choose_adjacent( _( "Drop where?" ) ) ) {
                if( *pnt != player_character.pos() && in_shell ) {
                    add_msg( m_info, _( "You can't drop things to another tile while you're in your shell." ) );
                } else {
                    drop_in_direction( *pnt );
                }
            }
            break;
        case ACTION_BIONICS:
            player_character.power_bionics();
            break;
        case ACTION_MUTATIONS:
            player_character.power_mutations();
            break;

        case ACTION_SORT_ARMOR:
            player_character.worn.sort_armor( player_character );
            break;

        case ACTION_WAIT:
            wait();
            break;

        case ACTION_CRAFT:
            player_character.craft();
            break;

        case ACTION_RECRAFT:
            player_character.recraft();
            break;

        case ACTION_LONGCRAFT:
            player_character.long_craft();
            break;

        case ACTION_DISASSEMBLE:
            if( player_character.controlling_vehicle ) {
                add_msg( m_info, _( "You can't disassemble items while driving." ) );
            } else {
                player_character.disassemble();
            }
            break;

        case ACTION_CONSTRUCT:
            if( player_character.in_vehicle ) {
                add_msg( m_info, _( "You can't construct while in a vehicle." ) );
            } else {
                construction_menu( false );
            }
            break;

        case ACTION_SLEEP:
            if( has_vehicle_control( player_character ) ) {
                add_msg( m_info, _( "You can't sleep while controlling a vehicle" ) );
            } else {
                sleep();
            }
            break;

        case ACTION_CONTROL_VEHICLE:
            if( player_character.is_mounted() ) {
                player_character.dismount();
            } else if( player_character.has_trait( trait_WAYFARER ) ) {
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
                turnssincelastmon = 0_turns;
                set_safe_mode( SAFE_MODE_OFF );
                add_msg( m_info, get_option<bool>( "AUTOSAFEMODE" )
                         ? _( "Safe mode OFF!  (Auto safe mode still enabled!)" ) : _( "Safe mode OFF!" ) );
            }
            if( player_character.has_effect( effect_laserlocked ) ) {
                player_character.remove_effect( effect_laserlocked );
                safe_mode_warning_logged = false;
            }
            break;

        case ACTION_TOGGLE_AUTOSAFE: {
            options_manager::cOpt &autosafemode_option = get_options().get_option( "AUTOSAFEMODE" );
            add_msg( m_info, autosafemode_option.value_as<bool>()
                     ? _( "Auto safe mode OFF!" ) : _( "Auto safe mode ON!" ) );
            autosafemode_option.setNext();
            break;
        }

        case ACTION_IGNORE_ENEMY:
            if( safe_mode == SAFE_MODE_STOP ) {
                add_msg( m_info, _( "Ignoring enemy!" ) );
                for( auto &elem : player_character.get_mon_visible().new_seen_mon ) {
                    monster &critter = *elem;
                    critter.ignoring = rl_dist( player_character.pos(), critter.pos() );
                }
                set_safe_mode( SAFE_MODE_ON );
            } else if( player_character.has_effect( effect_laserlocked ) ) {
                if( player_character.has_trait( trait_PROF_CHURL ) ) {
                    add_msg( m_warning, _( "You make the sign of the cross." ) );
                } else {
                    add_msg( m_info, _( "Ignoring laser targeting!" ) );
                }
                player_character.remove_effect( effect_laserlocked );
                safe_mode_warning_logged = false;
            }
            break;

        case ACTION_WHITELIST_ENEMY:
            if( safe_mode == SAFE_MODE_STOP && !get_safemode().empty() ) {
                get_safemode().add_rule( get_safemode().lastmon_whitelist, Creature::Attitude::ANY, 0,
                                         rule_state::WHITELISTED );
                add_msg( m_info, _( "Creature whitelisted: %s" ), get_safemode().lastmon_whitelist );
                set_safe_mode( SAFE_MODE_ON );
                mostseen = 0;
            } else {
                get_safemode().show();
            }
            break;

        case ACTION_WORKOUT:
            if( query_yn( _( "Start workout?" ) ) ) {
                player_character.assign_activity( workout_activity_actor( player_character.pos() ) );
            }
            break;

        case ACTION_SUICIDE:
            if( query_yn( _( "Abandon this character?" ) ) ) {
                if( query_yn( _( "This will kill your character.  Continue?" ) ) ) {
                    player_character.moves = 0;
                    player_character.place_corpse();
                    uquit = QUIT_SUICIDE;
                }
            }
            break;

        case ACTION_SAVE:
            if( query_yn( _( "Save and quit?" ) ) ) {
                if( save() ) {
                    player_character.moves = 0;
                    uquit = QUIT_SAVED;
                }
            }
            break;

        case ACTION_QUICKSAVE:
            quicksave();
            return false;

        case ACTION_QUICKLOAD:
            quickload();
            return false;

        case ACTION_PL_INFO:
            player_character.disp_info( true );
            break;

        case ACTION_MAP:
            if( !m.is_outside( player_character.pos() ) ) {
                uistate.overmap_visible_weather = false;
            }
            if( !get_timed_events().get( timed_event_type::OVERRIDE_PLACE ) ) {
                ui::omap::display();
            } else {
                add_msg( m_info, _( "You have no idea where you are." ) );
            }
            break;

        case ACTION_SKY:
            if( m.is_outside( player_character.pos() ) ) {
                ui::omap::display_visible_weather();
            } else {
                add_msg( m_info, _( "You can't see the sky from here." ) );
            }
            break;

        case ACTION_MISSIONS:
            list_missions();
            break;

        case ACTION_DIARY:
            diary::show_diary_ui( u.get_avatar_diary() );
            break;

        case ACTION_FACTIONS:
            faction_manager_ptr->display();
            break;

        case ACTION_MORALE:
            player_character.disp_morale();
            break;

        case ACTION_MEDICAL:
            player_character.disp_medical();
            break;

        case ACTION_BODYSTATUS:
            display_bodygraph( get_player_character() );
            break;

        case ACTION_MESSAGES:
            Messages::display_messages();
            break;

        case ACTION_HELP:
            get_help().display_help();
            break;

        case ACTION_OPTIONS:
            get_options().show( true );
            break;

        case ACTION_AUTOPICKUP:
            get_auto_pickup().show();
            break;

        case ACTION_AUTONOTES:
            get_auto_notes_settings().show_gui();
            break;

        case ACTION_SAFEMODE:
            get_safemode().show();
            break;

        case ACTION_DISTRACTION_MANAGER:
            get_distraction_manager().show();
            break;

        case ACTION_COLOR:
            all_colors.show_gui();
            break;

        case ACTION_WORLD_MODS:
            world_generator->show_active_world_mods( world_generator->active_world->active_mod_order );
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
            if( player_character.get_value( "THIEF_MODE" ) == "THIEF_ASK" ) {
                player_character.set_value( "THIEF_MODE", "THIEF_HONEST" );
                player_character.set_value( "THIEF_MODE_KEEP", "YES" );
                //~ Thief mode cycled between THIEF_ASK/THIEF_HONEST/THIEF_STEAL
                add_msg( _( "You will not pick up other peoples belongings." ) );
            } else if( player_character.get_value( "THIEF_MODE" ) == "THIEF_HONEST" ) {
                player_character.set_value( "THIEF_MODE", "THIEF_STEAL" );
                player_character.set_value( "THIEF_MODE_KEEP", "YES" );
                //~ Thief mode cycled between THIEF_ASK/THIEF_HONEST/THIEF_STEAL
                add_msg( _( "You will pick up also those things that belong to others!" ) );
            } else if( player_character.get_value( "THIEF_MODE" ) == "THIEF_STEAL" ) {
                player_character.set_value( "THIEF_MODE", "THIEF_ASK" );
                player_character.set_value( "THIEF_MODE_KEEP", "NO" );
                //~ Thief mode cycled between THIEF_ASK/THIEF_HONEST/THIEF_STEAL
                add_msg( _( "You will be reminded not to steal." ) );
            } else {
                // ERROR
                add_msg( _( "THIEF_MODE CONTAINED BAD VALUE [ %s ]!" ),
                         player_character.get_value( "THIEF_MODE" ) );
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

        case ACTION_DISPLAY_TRANSPARENCY:
            if( MAP_SHARING::isCompetitive() && !MAP_SHARING::isDebugger() ) {
                break;    //don't do anything when sharing and not debugger
            }
            display_transparency();
            break;

        case ACTION_DISPLAY_REACHABILITY_ZONES:
            if( MAP_SHARING::isCompetitive() && !MAP_SHARING::isDebugger() ) {
                break;    //don't do anything when sharing and not debugger
            }
            display_reachability_zones();
            break;

        case ACTION_TOGGLE_DEBUG_MODE:
            if( MAP_SHARING::isCompetitive() && !MAP_SHARING::isDebugger() ) {
                break;    //don't do anything when sharing and not debugger
            }
            handle_debug_mode();
            break;

        case ACTION_TOGGLE_PREVENT_OCCLUSION:
            get_options().get_option( "PREVENT_OCCLUSION" ).setNext();
            get_options().save();
            break;

        case ACTION_ZOOM_IN:
            zoom_in();
            mark_main_ui_adaptor_resize();
            break;

        case ACTION_ZOOM_OUT:
            zoom_out();
            mark_main_ui_adaptor_resize();
            break;

        case ACTION_ITEMACTION:
            item_action_menu();
            break;

        case ACTION_AUTOATTACK:
            avatar_action::autoattack( player_character, m );
            break;

        default:
            break;
    }

    return true;
}

bool game::handle_action()
{
    std::string action;
    input_context ctxt;
    action_id act = ACTION_NULL;
    user_turn current_turn;
    avatar &player_character = get_avatar();
    // Check if we have an auto-move destination
    if( player_character.has_destination() ) {
        act = player_character.get_next_auto_move_direction();
        if( act == ACTION_NULL ) {
            add_msg( m_info, _( "Auto-move canceled" ) );
            player_character.clear_destination();
            return false;
        }
        handle_key_blocking_activity();
    } else if( player_character.has_destination_activity() ) {
        // starts destination activity after the player successfully reached his destination
        player_character.start_destination_activity();
        return false;
    } else {
        // No auto-move, ask player for input
        ctxt = get_player_input( action );
    }

    // Remove asynchronous animations if any action taken before the input timeout
    // Otherwise repeated input can cause animations to accumulate as the timeout is never reached
    g->void_async_anim_curses();
#if defined(TILES)
    tilecontext->void_async_anim();
#else
    // Curses does not redraw itself so do it here
    g->invalidate_main_ui_adaptor();
#endif

    bool veh_ctrl = has_vehicle_control( player_character );

    // If performing an action with right mouse button, co-ordinates
    // of location clicked.
    std::optional<tripoint> mouse_target;

    if( uquit == QUIT_WATCH && action == "QUIT" ) {
        uquit = QUIT_DIED;
        return false;
    }

    if( act == ACTION_NULL ) {
        act = look_up_action( action );

        if( act == ACTION_KEYBINDINGS ) {
            // already handled by input context
            return false;
        }

        if( act == ACTION_MAIN_MENU ) {
            if( uquit == QUIT_WATCH ) {
                return false;
            }
            // No auto-move actions have or can be set at this point.
            player_character.clear_destination();
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
            player_character.clear_destination();
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
            player_character.clear_destination();
            destination_preview.clear();
            act = ctxt.display_menu( true );
            if( act == ACTION_NULL ) {
                return false;
            }
        }

        if( can_action_change_worldstate( act ) ) {
            user_action_counter += 1;
        }

        if( act == ACTION_CLICK_AND_DRAG ) {
            // Need to return false to avoid disrupting actions like character mouse movement that require two clicks
            return false;
        }

        if( act == ACTION_SELECT || act == ACTION_SEC_SELECT ) {
            // Mouse button click
            if( veh_ctrl ) {
                // No mouse use in vehicle
                return false;
            }

            if( player_character.is_dead_state() ) {
                // do not allow mouse actions while dead
                return false;
            }

            const std::optional<tripoint> mouse_pos = ctxt.get_coordinates( w_terrain, ter_view_p.xy(), true );
            if( !mouse_pos ) {
                return false;
            }
            if( !player_character.sees( *mouse_pos ) ) {
                // Not clicked in visible terrain
                return false;
            }
            mouse_target = mouse_pos;

            if( act == ACTION_SELECT ) {
                // Note: The following has the potential side effect of
                // setting auto-move destination state in addition to setting
                // act.
                // TODO: fix point types
                if( !try_get_left_click_action( act, tripoint_bub_ms( *mouse_target ) ) ) {
                    return false;
                }
            } else if( act == ACTION_SEC_SELECT ) {
                // TODO: fix point types
                if( !try_get_right_click_action( act, tripoint_bub_ms( *mouse_target ) ) ) {
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
            player_character.clear_destination();
            destination_preview.clear();
        }
    }

    if( act == ACTION_NULL ) {
        const input_event &&evt = ctxt.get_raw_input();
        if( !evt.sequence.empty() ) {
            const int ch = evt.get_first_input();
            if( !get_option<bool>( "NO_UNKNOWN_COMMAND_MSG" ) ) {
                std::string msg = string_format( _( "Unknown command: \"%s\" (%ld)" ), evt.long_description(), ch );
                if( const std::optional<std::string> hint =
                        press_x_if_bound( ACTION_KEYBINDINGS ) ) {
                    msg = string_format( "%s\n%s", msg,
                                         string_format( _( "%s at any time to see and edit keybindings relevant to "
                                                           "the current context." ),
                                                        *hint ) );
                }
                add_msg( m_info, msg );
            }
        }
        return false;
    }

    // This has no action unless we're in a special game mode.
    gamemode->pre_action( act );

    int before_action_moves = player_character.moves;

    // These actions are allowed while deathcam is active. Registered in game::get_player_input
    if( uquit == QUIT_WATCH || !player_character.is_dead_state() ) {
        do_deathcam_action( act, player_character );
    }

    // actions allowed only while alive
    if( !player_character.is_dead_state() ) {
        if( !do_regular_action( act, player_character, mouse_target ) ) {
            return false;
        }
    }
    if( act != ACTION_TIMEOUT ) {
        player_character.mod_moves( -current_turn.moves_elapsed() );
    }
    gamemode->post_action( act );

    player_character.movecounter = ( !player_character.is_dead_state() ? ( before_action_moves -
                                     player_character.moves ) : 0 );
    dbg( D_INFO ) << string_format( "%s: [%d] %d - %d = %d", action_ident( act ),
                                    to_turn<int>( calendar::turn ), before_action_moves, player_character.movecounter,
                                    player_character.moves );
    return !player_character.is_dead_state();
}
