#include "gamemode_tutorial.h" // IWYU pragma: associated

#include <memory>
#include <optional>

#include "action.h"
#include "avatar.h"
#include "calendar.h"
#include "character.h"
#include "coordinates.h"
#include "debug.h"
#include "game.h"
#include "item.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "point.h"
#include "profession.h"
#include "scent_map.h"
#include "text_snippets.h"
#include "translations.h"
#include "trap.h"
#include "type_id.h"
#include "weather.h"

static const furn_str_id furn_f_rack( "f_rack" );

static const itype_id itype_cig( "cig" );
static const itype_id itype_codeine( "codeine" );
static const itype_id itype_flashlight( "flashlight" );
static const itype_id itype_flashlight_on( "flashlight_on" );
static const itype_id itype_grenade_act( "grenade_act" );
static const itype_id itype_water_clean( "water_clean" );

static const overmap_special_id overmap_special_tutorial( "tutorial" );

static const skill_id skill_gun( "gun" );
static const skill_id skill_melee( "melee" );
static const skill_id skill_throwing( "throwing" );

static const ter_str_id ter_t_door_c( "t_door_c" );
static const ter_str_id ter_t_door_locked_interior( "t_door_locked_interior" );
static const ter_str_id ter_t_door_o( "t_door_o" );
static const ter_str_id ter_t_stairs_down( "t_stairs_down" );
static const ter_str_id ter_t_water_dispenser( "t_water_dispenser" );
static const ter_str_id ter_t_window( "t_window" );

static const trap_str_id tr_bubblewrap( "tr_bubblewrap" );
static const trap_str_id tr_tutorial_1( "tr_tutorial_1" );
static const trap_str_id tr_tutorial_10( "tr_tutorial_10" );
static const trap_str_id tr_tutorial_11( "tr_tutorial_11" );
static const trap_str_id tr_tutorial_12( "tr_tutorial_12" );
static const trap_str_id tr_tutorial_13( "tr_tutorial_13" );
static const trap_str_id tr_tutorial_14( "tr_tutorial_14" );
static const trap_str_id tr_tutorial_15( "tr_tutorial_15" );
static const trap_str_id tr_tutorial_2( "tr_tutorial_2" );
static const trap_str_id tr_tutorial_3( "tr_tutorial_3" );
static const trap_str_id tr_tutorial_4( "tr_tutorial_4" );
static const trap_str_id tr_tutorial_5( "tr_tutorial_5" );
static const trap_str_id tr_tutorial_6( "tr_tutorial_6" );
static const trap_str_id tr_tutorial_7( "tr_tutorial_7" );
static const trap_str_id tr_tutorial_8( "tr_tutorial_8" );
static const trap_str_id tr_tutorial_9( "tr_tutorial_9" );

namespace io
{

template<>
std::string enum_to_string<tut_lesson>( tut_lesson data )
{
    switch( data ) {
            // *INDENT-OFF*
        case tut_lesson::LESSON_INTRO: return "LESSON_INTRO";
        case tut_lesson::LESSON_MOVE: return "LESSON_MOVE";
        case tut_lesson::LESSON_MOVEMENT_MODES: return "LESSON_MOVEMENT_MODES";
        case tut_lesson::LESSON_LOOK: return "LESSON_LOOK";
        case tut_lesson::LESSON_OPEN: return "LESSON_OPEN";
        case tut_lesson::LESSON_CLOSE: return "LESSON_CLOSE";
        case tut_lesson::LESSON_SMASH: return "LESSON_SMASH";
        case tut_lesson::LESSON_WINDOW: return "LESSON_WINDOW";
        case tut_lesson::LESSON_PICKUP: return "LESSON_PICKUP";
        case tut_lesson::LESSON_EXAMINE: return "LESSON_EXAMINE";
        case tut_lesson::LESSON_INTERACT: return "LESSON_INTERACT";
        case tut_lesson::LESSON_GOT_ARMOR: return "LESSON_GOT_ARMOR";
        case tut_lesson::LESSON_GOT_WEAPON: return "LESSON_GOT_WEAPON";
        case tut_lesson::LESSON_GOT_FOOD: return "LESSON_GOT_FOOD";
        case tut_lesson::LESSON_GOT_TOOL: return "LESSON_GOT_TOOL";
        case tut_lesson::LESSON_GOT_GUN: return "LESSON_GOT_GUN";
        case tut_lesson::LESSON_GOT_AMMO: return "LESSON_GOT_AMMO";
        case tut_lesson::LESSON_WORE_ARMOR: return "LESSON_WORE_ARMOR";
        case tut_lesson::LESSON_WORE_STORAGE: return "LESSON_WORE_STORAGE";
        case tut_lesson::LESSON_WORE_MASK: return "LESSON_WORE_MASK";
        case tut_lesson::LESSON_WEAPON_INFO: return "LESSON_WEAPON_INFO";
        case tut_lesson::LESSON_HIT_MONSTER: return "LESSON_HIT_MONSTER";
        case tut_lesson::LESSON_PAIN: return "LESSON_PAIN";
        case tut_lesson::LESSON_BUTCHER: return "LESSON_BUTCHER";
        case tut_lesson::LESSON_TOOK_PAINKILLER: return "LESSON_TOOK_PAINKILLER";
        case tut_lesson::LESSON_TOOK_CIG: return "LESSON_TOOK_CIG";
        case tut_lesson::LESSON_DRANK_WATER: return "LESSON_DRANK_WATER";
        case tut_lesson::LESSON_ACT_GRENADE: return "LESSON_ACT_GRENADE";
        case tut_lesson::LESSON_ACT_BUBBLEWRAP: return "LESSON_ACT_BUBBLEWRAP";
        case tut_lesson::LESSON_GUN_LOAD: return "LESSON_GUN_LOAD";
        case tut_lesson::LESSON_GUN_FIRE: return "LESSON_GUN_FIRE";
        case tut_lesson::LESSON_RECOIL: return "LESSON_RECOIL";
        case tut_lesson::LESSON_STAIRS: return "LESSON_STAIRS";
        case tut_lesson::LESSON_DARK_NO_FLASH: return "LESSON_DARK_NO_FLASH";
        case tut_lesson::LESSON_DARK: return "LESSON_DARK";
        case tut_lesson::LESSON_PICKUP_WATER: return "LESSON_PICKUP_WATER";
        case tut_lesson::LESSON_MONSTER_SIGHTED: return "LESSON_MONSTER_SIGHTED";
        case tut_lesson::LESSON_LOCKED_DOOR: return "LESSON_LOCKED_DOOR";
        case tut_lesson::LESSON_RESTORE_STAMINA: return "LESSON_RESTORE_STAMINA";
        case tut_lesson::LESSON_REACH_ATTACK: return "LESSON_REACH_ATTACK";
        case tut_lesson::LESSON_HOLSTERS_WEAR: return "LESSON_HOLSTERS_WEAR";
        case tut_lesson::LESSON_HOLSTERS_ACTIVATE: return "LESSON_HOLSTERS_ACTIVATE";
        case tut_lesson::LESSON_INVENTORY: return "LESSON_INVENTORY";
        case tut_lesson::LESSON_FLASHLIGHT: return "LESSON_FLASHLIGHT";
        case tut_lesson::LESSON_REMOTE_USE: return "LESSON_REMOTE_USE";
        case tut_lesson::LESSON_CRAFTING_FOOD: return "LESSON_CRAFTING_FOOD";
        case tut_lesson::LESSON_CONSTRUCTION: return "LESSON_CONSTRUCTION";
        case tut_lesson::LESSON_THROWING: return "LESSON_THROWING";
        case tut_lesson::LESSON_FINALE: return "LESSON_FINALE";
            // *INDENT-ON*
        case tut_lesson::NUM_LESSONS:
            break;
    }
    cata_fatal( "Invalid tut_lesson" );
}
} // namespace io

bool tutorial_game::init()
{
    // TODO: clean up old tutorial

    // Start at noon at the end of the spring and set a fixed temperature of 20 degrees to prevent freezing
    calendar::turn = calendar::start_of_cataclysm + 8_weeks + 12_hours;
    get_weather().forced_temperature = units::from_celsius( 20 );

    tutorials_seen.clear();
    get_scent().reset();
    // We use a Z-factor of 10 so that we don't plop down tutorial rooms in the
    // middle of the "real" game world
    avatar &player_character = get_avatar();
    player_character.normalize();
    player_character.str_cur = player_character.str_max;
    player_character.per_cur = player_character.per_max;
    player_character.int_cur = player_character.int_max;
    player_character.dex_cur = player_character.dex_max;

    player_character.set_all_parts_hp_to_max();

    //~ default name for the tutorial
    player_character.name = _( "John Smith" );
    player_character.prof = profession::generic();
    // overmap terrain coordinates
    const tripoint_om_omt lp( 50, 50, 0 );
    // Assume overmap zero
    const tripoint_abs_omt lp_abs = project_combine( point_abs_om(), lp );
    overmap &starting_om = overmap_buffer.get( point_abs_om() );
    starting_om.place_special_forced( overmap_special_tutorial, lp, om_direction::type::north );
    starting_om.clear_mon_groups();

    player_character.wear_item( item( "boxer_shorts" ), false );
    player_character.wear_item( item( "jeans" ), false );
    player_character.wear_item( item( "longshirt" ), false );
    player_character.wear_item( item( "socks" ), false );
    player_character.wear_item( item( "sneakers" ), false );

    player_character.set_skill_level( skill_gun, 5 );
    player_character.set_skill_level( skill_melee, 5 );
    player_character.set_skill_level( skill_throwing, 5 );
    g->load_map( project_to<coords::sm>( lp_abs ) );
    player_character.move_to( project_to<coords::ms>( lp_abs ) + point( 2, 2 ) );

    // This shifts the view to center the players pos
    g->update_map( player_character );
    return true;
}

void tutorial_game::per_turn()
{
    // note that add_message does nothing if the message was already shown
    add_message( tut_lesson::LESSON_INTRO );
    add_message( tut_lesson::LESSON_MOVE );

    Character &player_character = get_player_character();
    if( g->light_level( player_character.posz() ) == 1 ) {
        if( player_character.has_amount( itype_flashlight, 1 ) ||
            player_character.has_amount( itype_flashlight_on, 1 ) ) {
            add_message( tut_lesson::LESSON_DARK );
        } else {
            add_message( tut_lesson::LESSON_DARK_NO_FLASH );
        }
    }

    if( player_character.get_wielded_item() &&
        player_character.get_wielded_item()->ammo_remaining( &player_character ) > 0 ) {
        add_message( tut_lesson::LESSON_GUN_FIRE );
    }

    if( player_character.get_pain() > 0 ) {
        add_message( tut_lesson::LESSON_PAIN );
    }

    if( player_character.get_stamina() <= 8000 ) {
        add_message( tut_lesson::LESSON_RESTORE_STAMINA );
    }

    map &here = get_map();
    if( !tutorials_seen[tut_lesson::LESSON_BUTCHER] ) {
        for( const item &it : here.i_at( player_character.pos().xy() ) ) {
            if( it.is_corpse() ) {
                add_message( tut_lesson::LESSON_BUTCHER );
                break;
            }
        }
    }

    for( const tripoint &p : here.points_in_radius( player_character.pos(), 1 ) ) {
        if( here.ter( p ) == ter_t_door_c ) {
            add_message( tut_lesson::LESSON_OPEN );
            break;
        } else if( here.ter( p ) == ter_t_door_o ) {
            add_message( tut_lesson::LESSON_CLOSE );
            break;
        } else if( here.ter( p ) == ter_t_door_locked_interior ) {
            add_message( tut_lesson::LESSON_LOCKED_DOOR );
            break;
        } else if( here.ter( p ) == ter_t_window ) {
            add_message( tut_lesson::LESSON_WINDOW );
            break;
        } else if( here.furn( p ) == furn_f_rack ) {
            add_message( tut_lesson::LESSON_EXAMINE );
            break;
        } else if( here.ter( p ) == ter_t_stairs_down ) {
            add_message( tut_lesson::LESSON_STAIRS );
            break;
        } else if( here.ter( p ) == ter_t_water_dispenser ) {
            add_message( tut_lesson::LESSON_PICKUP_WATER );
            break;
        } else if( here.tr_at( p ).id == tr_bubblewrap ) {
            add_message( tut_lesson::LESSON_ACT_BUBBLEWRAP );
            break;
        }
    }

    if( !here.i_at( point( player_character.posx(), player_character.posy() ) ).empty() ) {
        add_message( tut_lesson::LESSON_PICKUP );
    }

    if( here.tr_at( player_character.pos() ) == tr_tutorial_1 ) {
        add_message( tut_lesson::LESSON_LOOK );
    } else if( here.tr_at( player_character.pos() ) == tr_tutorial_2 ) {
        add_message( tut_lesson::LESSON_MOVEMENT_MODES );
    } else if( here.tr_at( player_character.pos() ) == tr_tutorial_3 ) {
        add_message( tut_lesson::LESSON_MONSTER_SIGHTED );
    } else if( here.tr_at( player_character.pos() ) == tr_tutorial_4 ) {
        add_message( tut_lesson::LESSON_REACH_ATTACK );
    } else if( here.tr_at( player_character.pos() ) == tr_tutorial_5 ) {
        add_message( tut_lesson::LESSON_HOLSTERS_WEAR );
    } else if( here.tr_at( player_character.pos() ) == tr_tutorial_6 ) {
        add_message( tut_lesson::LESSON_GUN_LOAD );
    } else if( here.tr_at( player_character.pos() ) == tr_tutorial_7 ) {
        add_message( tut_lesson::LESSON_INVENTORY );
    } else if( here.tr_at( player_character.pos() ) == tr_tutorial_8 ) {
        add_message( tut_lesson::LESSON_FLASHLIGHT );
    } else if( here.tr_at( player_character.pos() ) == tr_tutorial_9 ) {
        add_message( tut_lesson::LESSON_INTERACT );
    } else if( here.tr_at( player_character.pos() ) == tr_tutorial_10 ) {
        add_message( tut_lesson::LESSON_REMOTE_USE );
    } else if( here.tr_at( player_character.pos() ) == tr_tutorial_11 ) {
        player_character.set_hunger( 100 );
        player_character.stomach.empty();
        add_message( tut_lesson::LESSON_CRAFTING_FOOD );
    } else if( here.tr_at( player_character.pos() ) == tr_tutorial_12 ) {
        add_message( tut_lesson::LESSON_CONSTRUCTION );
    } else if( here.tr_at( player_character.pos() ) == tr_tutorial_13 ) {
        player_character.set_pain( 20 );
    } else if( here.tr_at( player_character.pos() ) == tr_tutorial_14 ) {
        add_message( tut_lesson::LESSON_THROWING );
    } else if( here.tr_at( player_character.pos() ) == tr_tutorial_15 ) {
        add_message( tut_lesson::LESSON_FINALE );
    }
}

void tutorial_game::pre_action( action_id &act )
{
    switch( act ) {
        case ACTION_SAVE:
        case ACTION_QUICKSAVE:
            popup( _( "You're saving a tutorial - the tutorial world lacks certain features of normal worlds.  "
                      "Weird things might happen when you load this save.  You have been warned." ) );
            break;
        default:
            // Other actions are fine.
            break;
    }
}

void tutorial_game::post_action( action_id act )
{
    Character &player_character = get_player_character();
    switch( act ) {
        case ACTION_OPEN:
            add_message( tut_lesson::LESSON_CLOSE );
            break;

        case ACTION_FIRE:
            if( player_character.get_wielded_item() && player_character.get_wielded_item()->is_gun() ) {
                add_message( tut_lesson::LESSON_RECOIL );
            }
            break;

        case ACTION_CLOSE:
            add_message( tut_lesson::LESSON_SMASH );
            break;

        case ACTION_USE: {
            if( player_character.has_amount( itype_grenade_act, 1 ) ) {
                add_message( tut_lesson::LESSON_ACT_GRENADE );
            }

            if( player_character.last_item == itype_codeine ) {
                add_message( tut_lesson::LESSON_TOOK_PAINKILLER );
            }
        }
        break;

        case ACTION_EAT:
            if( player_character.last_item == itype_cig ) {
                add_message( tut_lesson::LESSON_TOOK_CIG );
            } else if( player_character.last_item == itype_water_clean ) {
                add_message( tut_lesson::LESSON_DRANK_WATER );
            }
            break;

        case ACTION_WEAR: {
            item it( player_character.last_item, calendar::turn_zero );
            if( it.is_holster() ) {
                add_message( tut_lesson::LESSON_HOLSTERS_ACTIVATE );
            } else if( it.has_pocket_type( pocket_type::CONTAINER ) ) {
                add_message( tut_lesson::LESSON_WORE_STORAGE );
            } else if( it.is_armor() ) {
                if( it.get_env_resist() >= 2 ) {
                    add_message( tut_lesson::LESSON_WORE_MASK );
                } else if( it.get_avg_coverage() >= 2 || it.get_thickness() >= 2 ) {
                    add_message( tut_lesson::LESSON_WORE_ARMOR );
                }

            }
        }
        break;

        /* fallthrough */
        case ACTION_PICKUP: {
            item it( player_character.last_item, calendar::turn_zero );
            if( it.is_armor() ) {
                add_message( tut_lesson::LESSON_GOT_ARMOR );
            } else if( it.is_gun() ) {
                add_message( tut_lesson::LESSON_GOT_GUN );
            } else if( it.is_ammo() ) {
                add_message( tut_lesson::LESSON_GOT_AMMO );
            } else if( it.is_tool() ) {
                add_message( tut_lesson::LESSON_GOT_TOOL );
            } else if( it.is_food() ) {
                add_message( tut_lesson::LESSON_GOT_FOOD );
            } else if( it.is_melee() ) {
                add_message( tut_lesson::LESSON_GOT_WEAPON );
            }

        }
        break;

        default:
            // TODO: add more actions here
            break;

    }
}

void tutorial_game::add_message( tut_lesson lesson )
{
    if( tutorials_seen[lesson] ) {
        return;
    }
    tutorials_seen[lesson] = true;
    g->invalidate_main_ui_adaptor();
    std::string translated_lesson = SNIPPET.get_snippet_by_id( snippet_id(
                                        io::enum_to_string<tut_lesson>( lesson ) ) ).value_or( translation() ).translated();
    replace_keybind_tag( translated_lesson );
    popup( translated_lesson, PF_ON_TOP );
}
