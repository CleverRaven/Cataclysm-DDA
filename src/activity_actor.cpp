#include "activity_actor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <new>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "action.h"
#include "activity_actor_definitions.h"
#include "activity_handlers.h" // put_into_vehicle_or_drop and drop_on_map
#include "advanced_inv.h"
#include "avatar.h"
#include "avatar_action.h"
#include "bodypart.h"
#include "calendar.h"
#include "character.h"
#include "coordinates.h"
#include "craft_command.h"
#include "debug.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "flag.h"
#include "game.h"
#include "game_constants.h"
#include "gates.h"
#include "gun_mode.h"
#include "handle_liquid.h"
#include "iexamine.h"
#include "item.h"
#include "item_contents.h"
#include "item_group.h"
#include "item_location.h"
#include "itype.h"
#include "json.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "memory_fast.h"
#include "messages.h"
#include "monster.h"
#include "morale_types.h"
#include "mtype.h"
#include "npc.h"
#include "optional.h"
#include "options.h"
#include "output.h"
#include "pickup.h"
#include "pimpl.h"
#include "player.h"
#include "player_activity.h"
#include "point.h"
#include "ranged.h"
#include "recipe.h"
#include "requirements.h"
#include "ret_val.h"
#include "rng.h"
#include "shearing.h"
#include "sounds.h"
#include "string_formatter.h"
#include "timed_event.h"
#include "translations.h"
#include "ui.h"
#include "uistate.h"
#include "units.h"
#include "value_ptr.h"
#include "vehicle.h"
#include "vpart_position.h"

static const efftype_id effect_pet( "pet" );
static const efftype_id effect_sheared( "sheared" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_tied( "tied" );

static const itype_id itype_bone_human( "bone_human" );
static const itype_id itype_disassembly( "disassembly" );
static const itype_id itype_electrohack( "electrohack" );
static const itype_id itype_pseudo_bio_picklock( "pseudo_bio_picklock" );

static const skill_id skill_computer( "computer" );
static const skill_id skill_mechanics( "mechanics" );
static const skill_id skill_traps( "traps" );

static const proficiency_id proficiency_prof_lockpicking( "prof_lockpicking" );
static const proficiency_id proficiency_prof_lockpicking_expert( "prof_lockpicking_expert" );

static const mtype_id mon_zombie( "mon_zombie" );
static const mtype_id mon_zombie_fat( "mon_zombie_fat" );
static const mtype_id mon_zombie_rot( "mon_zombie_rot" );
static const mtype_id mon_skeleton( "mon_skeleton" );
static const mtype_id mon_zombie_crawler( "mon_zombie_crawler" );

static const quality_id qual_LOCKPICK( "LOCKPICK" );
static const quality_id qual_SHEAR( "SHEAR" );

std::string activity_actor::get_progress_message( const player_activity &act ) const
{
    if( act.moves_total > 0 ) {
        const int pct = ( ( act.moves_total - act.moves_left ) * 100 ) / act.moves_total;
        return string_format( "%d%%", pct );
    } else {
        return std::string();
    }
}

aim_activity_actor::aim_activity_actor()
{
    initial_view_offset = get_avatar().view_offset;
}

aim_activity_actor aim_activity_actor::use_wielded()
{
    return aim_activity_actor();
}

aim_activity_actor aim_activity_actor::use_bionic( const item &fake_gun,
        const units::energy &cost_per_shot )
{
    aim_activity_actor act = aim_activity_actor();
    act.bp_cost_per_shot = cost_per_shot;
    act.fake_weapon = fake_gun;
    return act;
}

aim_activity_actor aim_activity_actor::use_mutation( const item &fake_gun )
{
    aim_activity_actor act = aim_activity_actor();
    act.fake_weapon = fake_gun;
    return act;
}

void aim_activity_actor::start( player_activity &act, Character &/*who*/ )
{
    // Time spent on aiming is determined on the go by the player
    act.moves_total = 1;
    act.moves_left = 1;
}

void aim_activity_actor::do_turn( player_activity &act, Character &who )
{
    if( !who.is_avatar() ) {
        debugmsg( "ACT_AIM not implemented for NPCs" );
        aborted = true;
        act.moves_left = 0;
        return;
    }
    avatar &you = get_avatar();

    item *weapon = get_weapon();
    if( !weapon || !avatar_action::can_fire_weapon( you, get_map(), *weapon ) ) {
        aborted = true;
        act.moves_left = 0;
        return;
    }

    gun_mode gun = weapon->gun_current_mode();
    if( first_turn && gun->has_flag( flag_RELOAD_AND_SHOOT ) && !gun->ammo_remaining() ) {
        if( !load_RAS_weapon() ) {
            aborted = true;
            act.moves_left = 0;
            return;
        }
    }

    g->temp_exit_fullscreen();
    target_handler::trajectory trajectory = target_handler::mode_fire( you, *this );
    g->reenter_fullscreen();

    if( aborted ) {
        act.moves_left = 0;
    } else {
        if( !trajectory.empty() ) {
            fin_trajectory = trajectory;
            act.moves_left = 0;
        }
        // If aborting on the first turn, keep 'first_turn' as 'true'.
        // This allows refunding moves spent on unloading RELOAD_AND_SHOOT weapons
        // to simulate avatar not loading them in the first place
        first_turn = false;

        // Allow interrupting activity only during 'aim and fire'.
        // Prevents '.' key for 'aim for 10 turns' from conflicting with '.' key for 'interrupt activity'
        // in case of high input lag (curses, sdl sometimes...), but allows to interrupt aiming
        // if a bug happens / stars align to cause an endless aiming loop.
        act.interruptable_with_kb = action != "AIM";
    }
}

void aim_activity_actor::finish( player_activity &act, Character &who )
{
    act.set_to_null();
    restore_view();
    item *weapon = get_weapon();
    if( !weapon ) {
        return;
    }
    if( aborted ) {
        unload_RAS_weapon();
        if( reload_requested ) {
            // Reload the gun / select different arrows
            // May assign ACT_RELOAD
            g->reload_wielded( true );
        }
        return;
    }

    // Fire!
    gun_mode gun = weapon->gun_current_mode();
    int shots_fired = static_cast<player *>( &who )->fire_gun( fin_trajectory.back(), gun.qty, *gun );

    // TODO: bionic power cost of firing should be derived from a value of the relevant weapon.
    if( shots_fired && ( bp_cost_per_shot > 0_J ) ) {
        who.mod_power_level( -bp_cost_per_shot * shots_fired );
    }
}

void aim_activity_actor::canceled( player_activity &/*act*/, Character &/*who*/ )
{
    restore_view();
    unload_RAS_weapon();
}

void aim_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "fake_weapon", fake_weapon );
    jsout.member( "bp_cost_per_shot", bp_cost_per_shot );
    jsout.member( "first_turn", first_turn );
    jsout.member( "action", action );
    jsout.member( "aif_duration", aif_duration );
    jsout.member( "aiming_at_critter", aiming_at_critter );
    jsout.member( "snap_to_target", snap_to_target );
    jsout.member( "shifting_view", shifting_view );
    jsout.member( "initial_view_offset", initial_view_offset );

    jsout.end_object();
}

std::unique_ptr<activity_actor> aim_activity_actor::deserialize( JsonIn &jsin )
{
    aim_activity_actor actor = aim_activity_actor();

    JsonObject data = jsin.get_object();

    data.read( "fake_weapon", actor.fake_weapon );
    data.read( "bp_cost_per_shot", actor.bp_cost_per_shot );
    data.read( "first_turn", actor.first_turn );
    data.read( "action", actor.action );
    data.read( "aif_duration", actor.aif_duration );
    data.read( "aiming_at_critter", actor.aiming_at_critter );
    data.read( "snap_to_target", actor.snap_to_target );
    data.read( "shifting_view", actor.shifting_view );
    data.read( "initial_view_offset", actor.initial_view_offset );

    return actor.clone();
}

item *aim_activity_actor::get_weapon()
{
    if( fake_weapon.has_value() ) {
        // TODO: check if the player lost relevant bionic/mutation
        return &fake_weapon.value();
    } else {
        // Check for lost gun (e.g. yanked by zombie technician)
        // TODO: check that this is the same gun that was used to start aiming
        item *weapon = &get_player_character().weapon;
        return weapon->is_null() ? nullptr : weapon;
    }
}

void aim_activity_actor::restore_view()
{
    avatar &player_character = get_avatar();
    bool changed_z = player_character.view_offset.z != initial_view_offset.z;
    player_character.view_offset = initial_view_offset;
    if( changed_z ) {
        get_map().invalidate_map_cache( player_character.view_offset.z );
        g->invalidate_main_ui_adaptor();
    }
}

bool aim_activity_actor::load_RAS_weapon()
{
    // TODO: use activity for fetching ammo and loading weapon
    player &you = get_avatar();
    item *weapon = get_weapon();
    gun_mode gun = weapon->gun_current_mode();
    const auto ammo_location_is_valid = [&]() -> bool {
        if( !you.ammo_location )
        {
            return false;
        }
        if( !gun->can_reload_with( you.ammo_location->typeId() ) )
        {
            return false;
        }
        if( square_dist( you.pos(), you.ammo_location.position() ) > 1 )
        {
            return false;
        }
        return true;
    };
    item::reload_option opt = ammo_location_is_valid() ? item::reload_option( &you, weapon,
                              weapon, you.ammo_location ) : you.select_ammo( *gun );
    if( !opt ) {
        // Menu canceled
        return false;
    }
    int reload_time = 0;
    reload_time += opt.moves();
    if( !gun->reload( you, std::move( opt.ammo ), 1 ) ) {
        // Reload not allowed
        return false;
    }

    // Burn 0.2% max base stamina x the strength required to fire.
    you.mod_stamina( gun->get_min_str() * static_cast<int>( 0.002f *
                     get_option<int>( "PLAYER_MAX_STAMINA" ) ) );
    // At low stamina levels, firing starts getting slow.
    int sta_percent = ( 100 * you.get_stamina() ) / you.get_stamina_max();
    reload_time += ( sta_percent < 25 ) ? ( ( 25 - sta_percent ) * 2 ) : 0;

    you.moves -= reload_time;
    return true;
}

void aim_activity_actor::unload_RAS_weapon()
{
    // Unload reload-and-shoot weapons to avoid leaving bows pre-loaded with arrows
    avatar &you = get_avatar();
    item *weapon = get_weapon();
    if( !weapon ) {
        return;
    }

    gun_mode gun = weapon->gun_current_mode();
    if( gun->has_flag( flag_RELOAD_AND_SHOOT ) ) {
        int moves_before_unload = you.moves;

        // Note: this code works only for avatar
        item_location loc = item_location( you, gun.target );
        you.unload( loc, true );

        // Give back time for unloading as essentially nothing has been done.
        if( first_turn ) {
            you.moves = moves_before_unload;
        }
    }
}

void autodrive_activity_actor::start( player_activity &act, Character &who )
{
    const bool in_vehicle = who.in_vehicle && who.controlling_vehicle;
    const map &here = get_map();
    const optional_vpart_position vp = here.veh_at( who.pos() );
    if( !( vp && in_vehicle ) ) {
        who.cancel_activity();
        return;
    }

    player_vehicle = &vp->vehicle();
    act.moves_left = calendar::INDEFINITELY_LONG;
    who.moves = 0;
}

void autodrive_activity_actor::do_turn( player_activity &act, Character &who )
{
    if( who.in_vehicle && who.controlling_vehicle && player_vehicle && player_vehicle->is_autodriving &&
        !who.omt_path.empty() && !player_vehicle->omt_path.empty() ) {
        player_vehicle->do_autodrive();
        if( who.global_omt_location() == who.omt_path.back() ) {
            who.omt_path.pop_back();
        }
        who.moves = 0;
    } else {
        who.cancel_activity();
        return;
    }
    if( player_vehicle->omt_path.empty() ) {
        act.moves_left = 0;
    }
}

void autodrive_activity_actor::canceled( player_activity &act, Character &who )
{
    who.add_msg_if_player( m_info, _( "Auto-drive canceled." ) );
    if( player_vehicle && !player_vehicle->omt_path.empty() ) {
        player_vehicle->omt_path.clear();
    }
    if( !who.omt_path.empty() ) {
        who.omt_path.clear();
    }
    if( player_vehicle ) {
        player_vehicle->is_autodriving = false;
    }
    act.set_to_null();
}

void autodrive_activity_actor::finish( player_activity &act, Character &who )
{
    who.add_msg_if_player( m_info, _( "You have reached your destination." ) );
    player_vehicle->is_autodriving = false;
    act.set_to_null();
}

void autodrive_activity_actor::serialize( JsonOut &jsout ) const
{
    // Activity is not being saved but still provide some valid json if called.
    jsout.write_null();
}

std::unique_ptr<activity_actor> autodrive_activity_actor::deserialize( JsonIn & )
{
    return autodrive_activity_actor().clone();
}

void dig_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves_total;
    act.moves_left = moves_total;
}

void dig_activity_actor::do_turn( player_activity &, Character &who )
{
    sfx::play_activity_sound( "tool", "shovel", sfx::get_heard_volume( location ) );
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a shovel digging a pit at work!
        sounds::sound( location, 10, sounds::sound_t::activity, _( "hsh!" ) );
    }
    get_map().maybe_trigger_trap( location, who, true );
}

void dig_activity_actor::finish( player_activity &act, Character &who )
{
    map &here = get_map();
    const bool grave = here.ter( location ) == t_grave;

    if( grave ) {
        if( one_in( 10 ) ) {
            static const std::array<mtype_id, 5> monids = { {
                    mon_zombie, mon_zombie_fat,
                    mon_zombie_rot, mon_skeleton,
                    mon_zombie_crawler
                }
            };

            g->place_critter_at( random_entry( monids ), byproducts_location );
            here.furn_set( location, f_coffin_o );
            who.add_msg_if_player( m_warning, _( "Something crawls out of the coffin!" ) );
        } else {
            here.spawn_item( location, itype_bone_human, rng( 5, 15 ) );
            here.furn_set( location, f_coffin_c );
        }
        std::vector<item *> dropped =
            here.place_items( item_group_id( "allclothes" ), 50, location, location, false,
                              calendar::turn );
        here.place_items( item_group_id( "grave" ), 25, location, location, false,
                          calendar::turn );
        here.place_items( item_group_id( "jewelry_front" ), 20, location, location, false,
                          calendar::turn );
        for( item * const &it : dropped ) {
            if( it->is_armor() ) {
                it->set_flag( flag_FILTHY );
                it->set_damage( rng( 1, it->max_damage() - 1 ) );
            }
        }
        get_event_bus().send<event_type::exhumes_grave>( who.getID() );
    }

    here.ter_set( location, ter_id( result_terrain ) );

    for( int i = 0; i < byproducts_count; i++ ) {
        here.spawn_items( byproducts_location, item_group::items_from( byproducts_item_group,
                          calendar::turn ) );
    }

    const int helpersize = get_player_character().get_num_crafting_helpers( 3 );
    who.mod_stored_nutr( 5 - helpersize );
    who.mod_thirst( 5 - helpersize );
    who.mod_fatigue( 10 - ( helpersize * 2 ) );
    if( grave ) {
        who.add_msg_if_player( m_good, _( "You finish exhuming a grave." ) );
    } else {
        who.add_msg_if_player( m_good, _( "You finish digging the %s." ),
                               here.ter( location ).obj().name() );
    }

    act.set_to_null();
}

void dig_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves", moves_total );
    jsout.member( "location", location );
    jsout.member( "result_terrain", result_terrain );
    jsout.member( "byproducts_location", byproducts_location );
    jsout.member( "byproducts_count", byproducts_count );
    jsout.member( "byproducts_item_group", byproducts_item_group );

    jsout.end_object();
}

std::unique_ptr<activity_actor> dig_activity_actor::deserialize( JsonIn &jsin )
{
    dig_activity_actor actor( 0, tripoint_zero,
                              {}, tripoint_zero, 0, {} );

    JsonObject data = jsin.get_object();

    data.read( "moves", actor.moves_total );
    data.read( "location", actor.location );
    data.read( "result_terrain", actor.result_terrain );
    data.read( "byproducts_location", actor.byproducts_location );
    data.read( "byproducts_count", actor.byproducts_count );
    data.read( "byproducts_item_group", actor.byproducts_item_group );

    return actor.clone();
}

void dig_channel_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves_total;
    act.moves_left = moves_total;
}

void dig_channel_activity_actor::do_turn( player_activity &, Character &who )
{
    sfx::play_activity_sound( "tool", "shovel", sfx::get_heard_volume( location ) );
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a shovel digging a pit at work!
        sounds::sound( location, 10, sounds::sound_t::activity, _( "hsh!" ) );
    }
    get_map().maybe_trigger_trap( location, who, true );
}

void dig_channel_activity_actor::finish( player_activity &act, Character &who )
{
    map &here = get_map();
    here.ter_set( location, ter_id( result_terrain ) );

    for( int i = 0; i < byproducts_count; i++ ) {
        here.spawn_items( byproducts_location, item_group::items_from( byproducts_item_group,
                          calendar::turn ) );
    }

    const int helpersize = get_player_character().get_num_crafting_helpers( 3 );
    who.mod_stored_nutr( 5 - helpersize );
    who.mod_thirst( 5 - helpersize );
    who.mod_fatigue( 10 - ( helpersize * 2 ) );
    who.add_msg_if_player( m_good, _( "You finish digging up %s." ),
                           here.ter( location ).obj().name() );

    act.set_to_null();
}

void dig_channel_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves", moves_total );
    jsout.member( "location", location );
    jsout.member( "result_terrain", result_terrain );
    jsout.member( "byproducts_location", byproducts_location );
    jsout.member( "byproducts_count", byproducts_count );
    jsout.member( "byproducts_item_group", byproducts_item_group );

    jsout.end_object();
}

std::unique_ptr<activity_actor> dig_channel_activity_actor::deserialize( JsonIn &jsin )
{
    dig_channel_activity_actor actor( 0, tripoint_zero,
                                      {}, tripoint_zero, 0, {} );

    JsonObject data = jsin.get_object();

    data.read( "moves", actor.moves_total );
    data.read( "location", actor.location );
    data.read( "result_terrain", actor.result_terrain );
    data.read( "byproducts_location", actor.byproducts_location );
    data.read( "byproducts_count", actor.byproducts_count );
    data.read( "byproducts_item_group", actor.byproducts_item_group );

    return actor.clone();
}

void gunmod_remove_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves_total;
    act.moves_left = moves_total;
}

void gunmod_remove_activity_actor::finish( player_activity &act, Character &who )
{
    item *it_gun = gun.get_item();
    if( !it_gun ) {
        debugmsg( "ACT_GUNMOD_REMOVE lost target gun" );
        act.set_to_null();
        return;
    }
    item *it_mod = nullptr;
    std::vector<item *> mods = it_gun->gunmods();
    if( gunmod_idx >= 0 && mods.size() > static_cast<size_t>( gunmod_idx ) ) {
        it_mod = mods[gunmod_idx];
    } else {
        debugmsg( "ACT_GUNMOD_REMOVE lost target gunmod" );
        act.set_to_null();
        return;
    }
    act.set_to_null();
    gunmod_remove( who, *it_gun, *it_mod );
}

bool gunmod_remove_activity_actor::gunmod_unload( Character &who, item &gunmod )
{
    if( gunmod.has_flag( flag_BRASS_CATCHER ) ) {
        // Exclude brass catchers so that removing them wouldn't spill the casings
        return true;
    }
    // TODO: unloading gunmods happens instantaneously in some cases, but should take time
    item_location loc = item_location( who, &gunmod );
    return !( gunmod.ammo_remaining() && !who.as_player()->unload( loc, true ) );
}

void gunmod_remove_activity_actor::gunmod_remove( Character &who, item &gun, item &mod )
{
    if( !gunmod_unload( who, mod ) ) {
        return;
    }

    gun.gun_set_mode( gun_mode_id( "DEFAULT" ) );
    const itype *modtype = mod.type;

    who.i_add_or_drop( mod );
    gun.remove_item( mod );

    // If the removed gunmod added mod locations, check to see if any mods are in invalid locations
    if( !modtype->gunmod->add_mod.empty() ) {
        std::map<gunmod_location, int> mod_locations = gun.get_mod_locations();
        for( const auto &slot : mod_locations ) {
            int free_slots = gun.get_free_mod_locations( slot.first );

            for( item *the_mod : gun.gunmods() ) {
                if( the_mod->type->gunmod->location == slot.first && free_slots < 0 ) {
                    gunmod_remove( who, gun, *the_mod );
                    free_slots++;
                } else if( mod_locations.find( the_mod->type->gunmod->location ) ==
                           mod_locations.end() ) {
                    gunmod_remove( who, gun, *the_mod );
                }
            }
        }
    }

    //~ %1$s - gunmod, %2$s - gun.
    who.add_msg_if_player( _( "You remove your %1$s from your %2$s." ), modtype->nname( 1 ),
                           gun.tname() );
}

void gunmod_remove_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves_total", moves_total );
    jsout.member( "gun", gun );
    jsout.member( "gunmod", gunmod_idx );

    jsout.end_object();
}

std::unique_ptr<activity_actor> gunmod_remove_activity_actor::deserialize( JsonIn &jsin )
{
    gunmod_remove_activity_actor actor( 0, item_location(), -1 );

    JsonObject data = jsin.get_object();

    data.read( "moves_total", actor.moves_total );
    data.read( "gun", actor.gun );
    data.read( "gunmod", actor.gunmod_idx );

    return actor.clone();
}

void hacking_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = to_moves<int>( 5_minutes );
    act.moves_left = to_moves<int>( 5_minutes );
}

enum class hack_result : int {
    UNABLE,
    FAIL,
    NOTHING,
    SUCCESS
};

enum class hack_type : int {
    SAFE,
    DOOR,
    GAS,
    NONE
};

static int hack_level( const Character &who )
{
    ///\EFFECT_COMPUTER increases success chance of hacking card readers
    // odds go up with int>8, down with int<8
    // 4 int stat is worth 1 computer skill here
    ///\EFFECT_INT increases success chance of hacking card readers
    return who.get_skill_level( skill_computer ) + who.int_cur / 2 - 8;
}

static hack_result hack_attempt( Character &who )
{
    // TODO: Remove this once player -> Character migration is complete
    {
        player *p = dynamic_cast<player *>( &who );
        p->practice( skill_computer, 20 );
    }

    // only skilled supergenius never cause short circuits, but the odds are low for people
    // with moderate skills
    const int hack_stddev = 5;
    int success = std::ceil( normal_roll( hack_level( who ), hack_stddev ) );
    if( success < 0 ) {
        who.add_msg_if_player( _( "You cause a short circuit!" ) );
        who.use_charges( itype_electrohack, 25 );

        if( success <= -5 ) {
            who.use_charges( itype_electrohack, 50 );
        }
        return hack_result::FAIL;
    } else if( success < 6 ) {
        return hack_result::NOTHING;
    } else {
        return hack_result::SUCCESS;
    }
}

static hack_type get_hack_type( const tripoint &examp )
{
    hack_type type = hack_type::NONE;
    map &here = get_map();
    const furn_t &xfurn_t = here.furn( examp ).obj();
    const ter_t &xter_t = here.ter( examp ).obj();
    if( xter_t.has_examine( iexamine::pay_gas ) || xfurn_t.has_examine( iexamine::pay_gas ) ) {
        type = hack_type::GAS;
    } else if( xter_t.has_examine( "cardreader" ) ||
               xfurn_t.has_examine( "cardreader" ) ) {
        type = hack_type::DOOR;
    } else if( xter_t.has_examine( iexamine::gunsafe_el ) ||
               xfurn_t.has_examine( iexamine::gunsafe_el ) ) {
        type = hack_type::SAFE;
    }
    return type;
}

void hacking_activity_actor::finish( player_activity &act, Character &who )
{
    tripoint examp = act.placement;
    hack_type type = get_hack_type( examp );
    switch( hack_attempt( who ) ) {
        case hack_result::UNABLE:
            who.add_msg_if_player( _( "You cannot hack this." ) );
            break;
        case hack_result::FAIL:
            // currently all things that can be hacked have equivalent alarm failure states.
            // this may not always be the case with new hackable things.
            get_event_bus().send<event_type::triggers_alarm>( who.getID() );
            sounds::sound( who.pos(), 60, sounds::sound_t::music, _( "an alarm sound!" ), true, "environment",
                           "alarm" );
            if( examp.z > 0 && !get_timed_events().queued( timed_event_type::WANTED ) ) {
                get_timed_events().add( timed_event_type::WANTED, calendar::turn + 30_minutes, 0,
                                        who.global_sm_location() );
            }
            break;
        case hack_result::NOTHING:
            who.add_msg_if_player( _( "You fail the hack, but no alarms are triggered." ) );
            break;
        case hack_result::SUCCESS:
            map &here = get_map();
            if( type == hack_type::GAS ) {
                int tankUnits;
                std::string fuelType;
                const cata::optional<tripoint> pTank_ = iexamine::getNearFilledGasTank( examp, tankUnits,
                                                        fuelType );
                if( !pTank_ ) {
                    break;
                }
                const tripoint pTank = *pTank_;
                const cata::optional<tripoint> pGasPump = iexamine::getGasPumpByNumber( examp,
                        uistate.ags_pay_gas_selected_pump );
                if( pGasPump && iexamine::toPumpFuel( pTank, *pGasPump, tankUnits ) ) {
                    who.add_msg_if_player( _( "You hack the terminal and route all available fuel to your pump!" ) );
                    sounds::sound( examp, 6, sounds::sound_t::activity,
                                   _( "Glug Glug Glug Glug Glug Glug Glug Glug Glug" ), true, "tool", "gaspump" );
                } else {
                    who.add_msg_if_player( _( "Nothing happens." ) );
                }
            } else if( type == hack_type::SAFE ) {
                who.add_msg_if_player( m_good, _( "The door on the safe swings open." ) );
                here.furn_set( examp, furn_str_id( "f_safe_o" ) );
            } else if( type == hack_type::DOOR ) {
                who.add_msg_if_player( _( "You activate the panel!" ) );
                who.add_msg_if_player( m_good, _( "The nearby doors unlock." ) );
                here.ter_set( examp, t_card_reader_broken );
                for( const tripoint &tmp : here.points_in_radius( ( examp ), 3 ) ) {
                    if( here.ter( tmp ) == t_door_metal_locked ) {
                        here.ter_set( tmp, t_door_metal_c );
                    }
                }
            }
            break;
    }
    act.set_to_null();
}

void hacking_activity_actor::serialize( JsonOut & ) const {}

std::unique_ptr<activity_actor> hacking_activity_actor::deserialize( JsonIn & )
{
    hacking_activity_actor actor;
    return actor.clone();
}

void hotwire_car_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves_total;
    act.moves_left = moves_total;
}

void hotwire_car_activity_actor::do_turn( player_activity &act, Character & )
{
    map &here = get_map();
    if( calendar::once_every( 1_minutes ) ) {
        bool lost = !here.veh_at( here.getlocal( target ) ).has_value();
        if( lost ) {
            act.set_to_null();
            debugmsg( "Lost ACT_HOTWIRE_CAR target vehicle" );
        }
    }
}

void hotwire_car_activity_actor::finish( player_activity &act, Character &who )
{
    act.set_to_null();

    map &here = get_map();
    const optional_vpart_position vp = here.veh_at( here.getlocal( target ) );
    if( !vp ) {
        debugmsg( "Lost ACT_HOTWIRE_CAR target vehicle" );
        return;
    }
    vehicle &veh = vp->vehicle();

    int skill = who.get_skill_level( skill_mechanics );
    if( skill > rng( 1, 6 ) ) {
        // Success
        who.add_msg_if_player( _( "You found the wire that starts the engine." ) );
        veh.is_locked = false;
    } else if( skill > rng( 0, 4 ) ) {
        // Soft fail
        who.add_msg_if_player( _( "You found a wire that looks like the right one." ) );
        veh.is_alarm_on = veh.has_security_working();
        veh.is_locked = false;
    } else if( !veh.is_alarm_on ) {
        // Hard fail
        who.add_msg_if_player( _( "The red wire always starts the engine, doesn't it?" ) );
        veh.is_alarm_on = veh.has_security_working();
    } else {
        // Already failed
        who.add_msg_if_player(
            _( "By process of elimination, you found the wire that starts the engine." ) );
        veh.is_locked = false;
    }
}

void hotwire_car_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "target", target );
    jsout.member( "moves_total", moves_total );

    jsout.end_object();
}

std::unique_ptr<activity_actor> hotwire_car_activity_actor::deserialize( JsonIn &jsin )
{
    hotwire_car_activity_actor actor( 0, tripoint_zero );

    JsonObject data = jsin.get_object();

    data.read( "target", actor.target );
    data.read( "moves_total", actor.moves_total );

    return actor.clone();
}

void bikerack_racking_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves_total;
    act.moves_left = moves_total;
}

void bikerack_racking_activity_actor::finish( player_activity &act, Character & )
{
    if( parent_vehicle.try_to_rack_nearby_vehicle( parts ) ) {
        map &here = get_map();
        here.invalidate_map_cache( here.get_abs_sub().z );
        here.rebuild_vehicle_level_caches();
    } else {
        debugmsg( "Racking task failed.  Parent-Vehicle:" + parent_vehicle.name +
                  "; Found parts size:" + std::to_string( parts[0].size() ) );
    }
    act.set_to_null();
}

void bikerack_racking_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "moves_total", moves_total );
    jsout.member( "parent_vehicle", parent_vehicle );
    jsout.member( "parts", parts );
    jsout.end_object();
}

std::unique_ptr<activity_actor> bikerack_racking_activity_actor::deserialize( JsonIn &jsin )
{
    vehicle veh;
    bikerack_racking_activity_actor actor( 0, veh, std::vector<std::vector<int>>() );
    JsonObject data = jsin.get_object();
    data.read( "moves_total", actor.moves_total );
    data.read( "parent_vehicle", actor.parent_vehicle );
    data.read( "parts", actor.parts );

    return actor.clone();
}

void bikerack_unracking_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves_total;
    act.moves_left = moves_total;
}

void bikerack_unracking_activity_actor::finish( player_activity &act, Character & )
{
    if( parent_vehicle.remove_carried_vehicle( parts ) ) {
        parent_vehicle.clear_bike_racks( racks );
        map &here = get_map();
        here.invalidate_map_cache( here.get_abs_sub().z );
        here.rebuild_vehicle_level_caches();
    } else {
        debugmsg( "Unracking task failed.  Parent-Vehicle:" + parent_vehicle.name +
                  "; Found parts size:" + std::to_string( parts.size() ) );
    }
    act.set_to_null();
}

void bikerack_unracking_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "moves_total", moves_total );
    jsout.member( "parent_vehicle", parent_vehicle );
    jsout.member( "parts", parts );
    jsout.member( "racks", racks );
    jsout.end_object();
}

std::unique_ptr<activity_actor> bikerack_unracking_activity_actor::deserialize( JsonIn &jsin )
{
    vehicle veh;
    bikerack_unracking_activity_actor actor( 0, veh, std::vector<int>(), std::vector<int>() );
    JsonObject data = jsin.get_object();
    data.read( "moves_total", actor.moves_total );
    data.read( "parent_vehicle", actor.parent_vehicle );
    data.read( "parts", actor.parts );
    data.read( "racks", actor.racks );

    return actor.clone();
}

void move_items_activity_actor::do_turn( player_activity &act, Character &who )
{
    const tripoint dest = relative_destination + who.pos();

    while( who.moves > 0 && !target_items.empty() ) {
        item_location target = std::move( target_items.back() );
        const int quantity = quantities.back();
        target_items.pop_back();
        quantities.pop_back();

        if( !target ) {
            debugmsg( "Lost target item of ACT_MOVE_ITEMS" );
            continue;
        }

        // Check that we can pick it up.
        if( !target->made_of_from_type( phase_id::LIQUID ) ) {
            item &leftovers = *target;
            // Make a copy to be put in the destination location
            item newit = leftovers;

            if( newit.is_owned_by( who, true ) ) {
                newit.set_owner( who );
            } else {
                continue;
            }

            // Handle charges, quantity == 0 means move all
            if( quantity != 0 && newit.count_by_charges() ) {
                newit.charges = std::min( newit.charges, quantity );
                leftovers.charges -= quantity;
            } else {
                leftovers.charges = 0;
            }

            // This is for hauling across zlevels, remove when going up and down stairs
            // is no longer teleportation
            const tripoint src = target.position();
            const int distance = src.z == dest.z ? std::max( rl_dist( src, dest ), 1 ) : 1;
            // Yuck, I'm sticking weariness scaling based on activity level here
            const float weary_mult = who.exertion_adjusted_move_multiplier( exertion_level() );
            who.mod_moves( -Pickup::cost_to_move_item( who, newit ) * distance * weary_mult );
            if( to_vehicle ) {
                put_into_vehicle_or_drop( who, item_drop_reason::deliberate, { newit }, dest );
            } else {
                drop_on_map( who, item_drop_reason::deliberate, { newit }, dest );
            }
            // If we picked up a whole stack, remove the leftover item
            if( leftovers.charges <= 0 ) {
                target.remove_item();
            }
        }
    }

    if( target_items.empty() ) {
        // Nuke the current activity, leaving the backlog alone.
        act.set_to_null();
        if( who.is_hauling() && !get_map().has_haulable_items( who.pos() ) ) {
            who.stop_hauling();
        }
    }
}

void move_items_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "target_items", target_items );
    jsout.member( "quantities", quantities );
    jsout.member( "to_vehicle", to_vehicle );
    jsout.member( "relative_destination", relative_destination );

    jsout.end_object();
}

std::unique_ptr<activity_actor> move_items_activity_actor::deserialize( JsonIn &jsin )
{
    move_items_activity_actor actor( {}, {}, false, tripoint_zero );

    JsonObject data = jsin.get_object();

    data.read( "target_items", actor.target_items );
    data.read( "quantities", actor.quantities );
    data.read( "to_vehicle", actor.to_vehicle );
    data.read( "relative_destination", actor.relative_destination );

    return actor.clone();
}

static void cancel_pickup( Character &who )
{
    who.cancel_activity();
    if( who.is_hauling() && !get_map().has_haulable_items( who.pos() ) ) {
        who.stop_hauling();
    }
}

void pickup_activity_actor::do_turn( player_activity &, Character &who )
{
    // If we don't have target items bail out
    if( target_items.empty() ) {
        cancel_pickup( who );
        return;
    }

    // If the player moves while picking up (i.e.: in a moving vehicle) cancel
    // the activity, only populate starting_pos when grabbing from the ground
    if( starting_pos && *starting_pos != who.pos() ) {
        who.add_msg_if_player( _( "Moving canceled auto-pickup." ) );
        cancel_pickup( who );
        return;
    }

    // Auto_resume implies autopickup.
    const bool autopickup = who.activity.auto_resume;

    // False indicates that the player canceled pickup when met with some prompt
    const bool keep_going = Pickup::do_pickup( target_items, quantities, autopickup );

    // If there are items left we ran out of moves, so continue the activity
    // Otherwise, we are done.
    if( !keep_going || target_items.empty() ) {
        cancel_pickup( who );

        if( who.get_value( "THIEF_MODE_KEEP" ) != "YES" ) {
            who.set_value( "THIEF_MODE", "THIEF_ASK" );
        }

        if( !keep_going ) {
            // The user canceled the activity, so we're done
            // AIM might have more pickup activities pending, also cancel them.
            // TODO: Move this to advanced inventory instead of hacking it in here
            cancel_aim_processing();
        }
    }
}

void pickup_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "target_items", target_items );
    jsout.member( "quantities", quantities );
    jsout.member( "starting_pos", starting_pos );

    jsout.end_object();
}

std::unique_ptr<activity_actor> pickup_activity_actor::deserialize( JsonIn &jsin )
{
    pickup_activity_actor actor( {}, {}, cata::nullopt );

    JsonObject data = jsin.get_object();

    data.read( "target_items", actor.target_items );
    data.read( "quantities", actor.quantities );
    data.read( "starting_pos", actor.starting_pos );

    return actor.clone();
}

lockpick_activity_actor lockpick_activity_actor::use_item(
    int moves_total,
    const item_location &lockpick,
    const tripoint &target
)
{
    return lockpick_activity_actor {
        moves_total,
        lockpick,
        cata::nullopt,
        target
    };
}

lockpick_activity_actor lockpick_activity_actor::use_bionic(
    const tripoint &target
)
{
    return lockpick_activity_actor {
        to_moves<int>( 4_seconds ),
        cata::nullopt,
        item( itype_pseudo_bio_picklock ),
        target
    };
}

void lockpick_activity_actor::start( player_activity &act, Character & )
{
    act.moves_left = moves_total;
    act.moves_total = moves_total;

    const time_duration lockpicking_time = time_duration::from_moves( moves_total );
    add_msg_debug( debugmode::DF_ACT_LOCKPICK, "lockpicking time = %s", to_string( lockpicking_time ) );
}

void lockpick_activity_actor::finish( player_activity &act, Character &who )
{
    act.set_to_null();

    item *it;
    if( lockpick.has_value() ) {
        it = ( *lockpick ).get_item();
    } else {
        it = &*fake_lockpick;
    }

    if( !it ) {
        debugmsg( "Lost ACT_LOCKPICK item" );
        return;
    }

    map &here = get_map();
    const tripoint target = here.getlocal( this->target );
    const ter_id ter_type = here.ter( target );
    const furn_id furn_type = here.furn( target );
    ter_id new_ter_type;
    furn_id new_furn_type;
    std::string open_message = _( "The lock opens…" );

    if( here.has_furn( target ) ) {
        if( furn_type->lockpick_result.is_null() ) {
            debugmsg( "%s lockpick_result is null", furn_type.id().str() );
            return;
        }

        new_furn_type = furn_type->lockpick_result;
        if( !furn_type->lockpick_message.empty() ) {
            open_message = furn_type->lockpick_message.translated();
        }
    } else {
        if( ter_type->lockpick_result.is_null() ) {
            debugmsg( "%s lockpick_result is null", ter_type.id().str() );
            return;
        }

        new_ter_type = ter_type->lockpick_result;
        if( !ter_type->lockpick_message.empty() ) {
            open_message = ter_type->lockpick_message.translated();
        }
    }

    bool perfect = it->has_flag( flag_PERFECT_LOCKPICK );
    bool destroy = false;

    // Your devices skill is the primary skill that applies to your roll. Your mechanics skill has a little input.
    const float weighted_skill_average = ( 3.0f * who.get_skill_level(
            skill_traps ) + who.get_skill_level( skill_mechanics ) ) / 4.0f;

    // Your dexterity determines most of your stat contribution, but your intelligence and perception combined are about half as much.
    const float weighted_stat_average = ( 6.0f * who.dex_cur + 2.0f * who.per_cur +
                                          who.int_cur ) / 9.0f;

    // Get a bonus from your lockpick quality if the quality is higher than 3, or a penalty if it is lower. For a bobby pin this puts you at -2, for a locksmith kit, +2.
    const float tool_effect = ( it->get_quality( qual_LOCKPICK ) - 3 ) - ( it->damage() / 2000.0 );

    // Without at least a basic lockpick proficiency, your skill level is effectively 6 levels lower.
    int proficiency_effect = -3;
    int duration_proficiency_factor = 10;
    if( who.has_proficiency( proficiency_prof_lockpicking ) ) {
        // If you have the basic lockpick prof, negate the above penalty
        proficiency_effect = 0;
        duration_proficiency_factor = 5;
    }
    if( who.has_proficiency( proficiency_prof_lockpicking_expert ) ) {
        // If you have the locksmith proficiency, your skill level is effectively 4 levels higher.
        proficiency_effect = 3;
        duration_proficiency_factor = 1;
    }

    // We get our average roll by adding the above factors together. For a person with no skill, average stats, no proficiencies, and an improvised lockpick, mean_roll will be 2.
    const float mean_roll = weighted_skill_average + ( weighted_stat_average / 4 ) + proficiency_effect
                            + tool_effect;

    // A standard deviation of 2 means that about 2/3 of rolls will come within 2 points of your mean_roll. Lockpicking
    int pick_roll = std::round( normal_roll( mean_roll, 2 ) );

    // Lock_roll should be replaced with a flat value defined by the door, soon.
    // In the meantime, let's roll 3d5-3, giving us a range of 0-12.
    int lock_roll = rng( 0, 4 ) + rng( 0, 4 ) + rng( 0, 4 );

    add_msg_debug( debugmode::DF_ACT_LOCKPICK, _( "Rolled %i. Mean_roll %g. Difficulty %i." ),
                   pick_roll,
                   mean_roll, lock_roll );

    // Your base skill XP gain is derived from the lock difficulty (which is currently random but shouldn't be).
    int xp_gain = 3 * lock_roll;
    if( perfect || ( pick_roll >= lock_roll ) ) {
        if( !perfect ) {
            // Increase your XP if you successfully pick the lock, unless you were using a Perfect Lockpick.
            xp_gain = xp_gain * 2;
        }
        here.has_furn( target ) ?
        here.furn_set( target, new_furn_type ) :
        static_cast<void>( here.ter_set( target, new_ter_type ) );
        who.add_msg_if_player( m_good, open_message );
    } else if( furn_type == f_gunsafe_ml && lock_roll > ( 3 * pick_roll ) ) {
        who.add_msg_if_player( m_bad, _( "Your clumsy attempt jams the lock!" ) );
        here.furn_set( target, furn_str_id( "f_gunsafe_mj" ) );
    } else if( lock_roll > ( 1.5 * pick_roll ) ) {
        if( it->inc_damage() ) {
            who.add_msg_if_player( m_bad,
                                   _( "The lock stumps your efforts to pick it, and you destroy your tool." ) );
            destroy = true;
        } else {
            who.add_msg_if_player( m_bad,
                                   _( "The lock stumps your efforts to pick it, and you damage your tool." ) );
        }
    } else {
        who.add_msg_if_player( m_bad, _( "The lock stumps your efforts to pick it." ) );
    }

    if( avatar *you = dynamic_cast<avatar *>( &who ) ) {
        // Gives another boost to XP, reduced by your skill level.
        // Higher skill levels require more difficult locks to gain a meaningful amount of xp.
        // Again, we're using randomized lock_roll until a defined lock difficulty is implemented.
        if( lock_roll > you->get_skill_level( skill_traps ) ) {
            xp_gain += lock_roll + ( xp_gain / ( you->get_skill_level( skill_traps ) + 1 ) );
        } else {
            xp_gain += xp_gain / ( you->get_skill_level( skill_traps ) + 1 );
        }
        you->practice( skill_traps, xp_gain );
    }

    if( !perfect && ter_type == t_door_locked_alarm && ( lock_roll + dice( 1, 30 ) ) > pick_roll ) {
        sounds::sound( who.pos(), 40, sounds::sound_t::alarm, _( "an alarm sound!" ), true, "environment",
                       "alarm" );
        if( !get_timed_events().queued( timed_event_type::WANTED ) ) {
            get_timed_events().add( timed_event_type::WANTED, calendar::turn + 30_minutes, 0,
                                    who.global_sm_location() );
        }
    }

    if( destroy && lockpick.has_value() ) {
        ( *lockpick ).remove_item();
    }

    who.practice_proficiency( proficiency_prof_lockpicking,
                              time_duration::from_moves( act.moves_total ) / duration_proficiency_factor );
    who.practice_proficiency( proficiency_prof_lockpicking_expert,
                              time_duration::from_moves( act.moves_total ) / duration_proficiency_factor );
}

cata::optional<tripoint> lockpick_activity_actor::select_location( avatar &you )
{
    if( you.is_mounted() ) {
        you.add_msg_if_player( m_info, _( "You cannot do that while mounted." ) );
        return cata::nullopt;
    }

    const std::function<bool( const tripoint & )> is_pickable = [&you]( const tripoint & p ) {
        if( p == you.pos() ) {
            return false;
        }
        return get_map().has_flag( "PICKABLE", p );
    };

    const cata::optional<tripoint> target = choose_adjacent_highlight(
            _( "Use your lockpick where?" ), _( "There is nothing to lockpick nearby." ), is_pickable, false );
    if( !target ) {
        return cata::nullopt;
    }

    if( is_pickable( *target ) ) {
        return target;
    }

    const ter_id terr_type = get_map().ter( *target );
    if( *target == you.pos() ) {
        you.add_msg_if_player( m_info, _( "You pick your nose and your sinuses swing open." ) );
    } else if( g->critter_at<npc>( *target ) ) {
        you.add_msg_if_player( m_info,
                               _( "You can pick your friends, and you can\npick your nose, but you can't pick\nyour friend's nose." ) );
    } else if( terr_type == t_door_c ) {
        you.add_msg_if_player( m_info, _( "That door isn't locked." ) );
    } else {
        you.add_msg_if_player( m_info, _( "That cannot be picked." ) );
    }
    return cata::nullopt;
}

void lockpick_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves_total", moves_total );
    jsout.member( "lockpick", lockpick );
    jsout.member( "fake_lockpick", fake_lockpick );
    jsout.member( "target", target );

    jsout.end_object();
}

std::unique_ptr<activity_actor> lockpick_activity_actor::deserialize( JsonIn &jsin )
{
    lockpick_activity_actor actor( 0, cata::nullopt, cata::nullopt, tripoint_zero );

    JsonObject data = jsin.get_object();

    data.read( "moves_total", actor.moves_total );
    data.read( "lockpick", actor.lockpick );
    data.read( "fake_lockpick", actor.fake_lockpick );
    data.read( "target", actor.target );

    return actor.clone();
}

void migration_cancel_activity_actor::do_turn( player_activity &act, Character &who )
{
    // Stop the activity
    act.set_to_null();

    // Ensure that neither avatars nor npcs end up in an invalid state
    if( who.is_npc() ) {
        npc &npc_who = dynamic_cast<npc &>( who );
        npc_who.revert_after_activity();
    } else {
        avatar &avatar_who = dynamic_cast<avatar &>( who );
        avatar_who.clear_destination();
        avatar_who.backlog.clear();
    }
}

void migration_cancel_activity_actor::serialize( JsonOut &jsout ) const
{
    // This will probably never be called, but write null to avoid invalid json in
    // the case that it is
    jsout.write_null();
}

std::unique_ptr<activity_actor> migration_cancel_activity_actor::deserialize( JsonIn & )
{
    return migration_cancel_activity_actor().clone();
}

void open_gate_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves_total;
    act.moves_left = moves_total;
}

void open_gate_activity_actor::finish( player_activity &act, Character & )
{
    gates::open_gate( placement );
    act.set_to_null();
}

void open_gate_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves", moves_total );
    jsout.member( "placement", placement );

    jsout.end_object();
}

std::unique_ptr<activity_actor> open_gate_activity_actor::deserialize( JsonIn &jsin )
{
    open_gate_activity_actor actor( 0, tripoint_zero );

    JsonObject data = jsin.get_object();

    data.read( "moves", actor.moves_total );
    data.read( "placement", actor.placement );

    return actor.clone();
}

void consume_activity_actor::start( player_activity &act, Character &guy )
{
    int moves;
    Character &player_character = get_player_character();
    if( consume_location ) {
        ret_val<edible_rating> ret = ret_val<edible_rating>::make_success();
        if( refuel ) {
            ret = player_character.can_consume_fuel( *consume_location );
        } else {
            ret = player_character.will_eat( *consume_location, true );
        }

        if( !ret.success() ) {
            canceled = true;
            consume_menu_selections = std::vector<int>();
            consume_menu_selected_items.clear();
            consume_menu_filter.clear();
            return;
        }
        moves = to_moves<int>( guy.get_consume_time( *consume_location ) );
    } else if( !consume_item.is_null() ) {
        ret_val<edible_rating> ret = ret_val<edible_rating>::make_success();
        if( refuel ) {
            ret = player_character.can_consume_fuel( consume_item );
        } else {
            ret = player_character.will_eat( consume_item, true );
        }
        if( !ret.success() ) {
            canceled = true;
            consume_menu_selections = std::vector<int>();
            consume_menu_selected_items.clear();
            consume_menu_filter.clear();
            return;
        }
        moves = to_moves<int>( guy.get_consume_time( consume_item ) );
    } else {
        debugmsg( "Item/location to be consumed should not be null." );
        canceled = true;
        return;
    }

    act.moves_total = moves;
    act.moves_left = moves;
}

void consume_activity_actor::finish( player_activity &act, Character & )
{
    // Prevent interruptions from this point onwards, so that e.g. pain from
    // injecting serum doesn't pop up messages about cancelling consuming (it's
    // too late; we've already consumed).
    act.interruptable = false;

    // Consuming an item may cause various effects, including cancelling our activity.
    // Back up these values since this activity actor might be destroyed.
    std::vector<int> temp_selections = consume_menu_selections;
    const std::vector<item_location> temp_selected_items = consume_menu_selected_items;
    const std::string temp_filter = consume_menu_filter;
    item_location consume_loc = consume_location;
    activity_id new_act = type;

    avatar &player_character = get_avatar();
    if( !canceled ) {
        if( consume_loc ) {
            player_character.consume( consume_loc, /*force=*/true, refuel );
        } else if( !consume_item.is_null() ) {
            player_character.consume( consume_item, /*force=*/true, refuel );
        } else {
            debugmsg( "Item location/name to be consumed should not be null." );
        }
        if( player_character.get_value( "THIEF_MODE_KEEP" ) != "YES" ) {
            player_character.set_value( "THIEF_MODE", "THIEF_ASK" );
        }
    }

    if( act.id() == activity_id( "ACT_CONSUME" ) ) {
        act.set_to_null();
    }

    if( !temp_selections.empty() || !temp_selected_items.empty() || !temp_filter.empty() ) {
        if( act.is_null() ) {
            player_character.assign_activity( new_act );
            player_character.activity.values = temp_selections;
            player_character.activity.targets = temp_selected_items;
            player_character.activity.str_values = { temp_filter, "true" };
        } else {
            player_activity eat_menu( new_act );
            eat_menu.values = temp_selections;
            eat_menu.targets = temp_selected_items;
            eat_menu.str_values = { temp_filter, "true" };
            player_character.backlog.push_back( eat_menu );
        }
    }
}

void consume_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "consume_location", consume_location );
    jsout.member( "consume_item", consume_item );
    jsout.member( "consume_menu_selections", consume_menu_selections );
    jsout.member( "consume_menu_selected_items", consume_menu_selected_items );
    jsout.member( "consume_menu_filter", consume_menu_filter );
    jsout.member( "canceled", canceled );

    jsout.end_object();
}

std::unique_ptr<activity_actor> consume_activity_actor::deserialize( JsonIn &jsin )
{
    item_location null;
    consume_activity_actor actor( null );

    JsonObject data = jsin.get_object();

    data.read( "consume_location", actor.consume_location );
    data.read( "consume_item", actor.consume_item );
    data.read( "consume_menu_selections", actor.consume_menu_selections );
    data.read( "consume_menu_selected_items", actor.consume_menu_selected_items );
    data.read( "consume_menu_filter", actor.consume_menu_filter );
    data.read( "canceled", actor.canceled );

    return actor.clone();
}

void try_sleep_activity_actor::start( player_activity &act, Character &/*who*/ )
{
    act.moves_total = to_moves<int>( duration );
    act.moves_left = act.moves_total;
}

void try_sleep_activity_actor::do_turn( player_activity &act, Character &who )
{
    if( who.has_effect( effect_sleep ) ) {
        return;
    }
    if( dynamic_cast<player *>( &who )->can_sleep() ) {
        who.fall_asleep(); // calls act.set_to_null()
        if( !who.has_effect( effect_sleep ) ) {
            // Character can potentially have immunity for 'effect_sleep'
            who.add_msg_if_player(
                _( "You feel you should've fallen asleep by now, but somehow you're still awake." ) );
        }
        return;
    }
    if( one_in( 1000 ) ) {
        who.add_msg_if_player( _( "You toss and turn…" ) );
    }
    if( calendar::once_every( 30_minutes ) ) {
        query_keep_trying( act, who );
    }
}

void try_sleep_activity_actor::finish( player_activity &act, Character &who )
{
    act.set_to_null();
    if( !who.has_effect( effect_sleep ) ) {
        who.add_msg_if_player( _( "You try to sleep, but can't." ) );
    }
}

void try_sleep_activity_actor::query_keep_trying( player_activity &act, Character &who )
{
    if( disable_query || !who.is_avatar() ) {
        return;
    }

    uilist sleep_query;
    sleep_query.text = _( "You have trouble sleeping, keep trying?" );
    sleep_query.addentry( 1, true, 'S', _( "Stop trying to fall asleep and get up." ) );
    sleep_query.addentry( 2, true, 'c', _( "Continue trying to fall asleep." ) );
    sleep_query.addentry( 3, true, 'C',
                          _( "Continue trying to fall asleep and don't ask again." ) );
    sleep_query.query();
    switch( sleep_query.ret ) {
        case UILIST_CANCEL:
        case 1:
            act.set_to_null();
            break;
        case 3:
            disable_query = true;
            break;
        case 2:
        default:
            break;
    }
}

void try_sleep_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "disable_query", disable_query );
    jsout.member( "duration", duration );

    jsout.end_object();
}

std::unique_ptr<activity_actor> try_sleep_activity_actor::deserialize( JsonIn &jsin )
{
    try_sleep_activity_actor actor = try_sleep_activity_actor( 0_seconds );

    JsonObject data = jsin.get_object();

    data.read( "disable_query", actor.disable_query );
    data.read( "duration", actor.duration );

    return actor.clone();
}

void unload_activity_actor::start( player_activity &act, Character & )
{
    act.moves_left = moves_total;
    act.moves_total = moves_total;
}

void unload_activity_actor::finish( player_activity &act, Character &who )
{
    act.set_to_null();
    unload( who, target );
}

void unload_activity_actor::unload( Character &who, item_location &target )
{
    int qty = 0;
    item &it = *target.get_item();
    bool actually_unloaded = false;

    if( it.is_container() ) {
        contents_change_handler handler;
        bool changed = false;
        for( item *contained : it.all_items_top() ) {
            int old_charges = contained->charges;
            const bool consumed = who.add_or_drop_with_msg( *contained, true, &it );
            if( consumed || contained->charges != old_charges ) {
                changed = true;
                handler.unseal_pocket_containing( item_location( target, contained ) );
            }
            if( consumed ) {
                it.remove_item( *contained );
            }
        }

        if( changed ) {
            it.on_contents_changed();
            who.invalidate_weight_carried_cache();
            handler.handle_by( who );
        }
        return;
    }

    std::vector<item *> remove_contained;
    for( item *contained : it.all_items_top() ) {
        if( contained->ammo_type() == ammotype( "plutonium" ) ) {
            contained->charges /= PLUTONIUM_CHARGES;
        }
        if( who.as_player()->add_or_drop_with_msg( *contained, true, &it ) ) {
            qty += contained->charges;
            remove_contained.push_back( contained );
            actually_unloaded = true;
        }
    }
    // remove the ammo leads in the belt
    for( item *remove : remove_contained ) {
        it.remove_item( *remove );
        actually_unloaded = true;
    }

    // remove the belt linkage
    if( it.is_ammo_belt() ) {
        if( it.type->magazine->linkage ) {
            item link( *it.type->magazine->linkage, calendar::turn, qty );
            if( who.as_player()->add_or_drop_with_msg( link, true ) ) {
                actually_unloaded = true;
            }
        }
        if( actually_unloaded ) {
            who.add_msg_if_player( _( "You disassemble your %s." ), it.tname() );
        }
    } else if( actually_unloaded ) {
        who.add_msg_if_player( _( "You unload your %s." ), it.tname() );
    }

    if( it.has_flag( flag_MAG_DESTROY ) && it.ammo_remaining() == 0 ) {
        target.remove_item();
    }
}

void unload_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves_total", moves_total );
    jsout.member( "target", target );

    jsout.end_object();
}

std::unique_ptr<activity_actor> unload_activity_actor::deserialize( JsonIn &jsin )
{
    unload_activity_actor actor = unload_activity_actor( 0, item_location::nowhere );

    JsonObject data = jsin.get_object();

    data.read( "moves_total", actor.moves_total );
    data.read( "target", actor.target );

    return actor.clone();
}

craft_activity_actor::craft_activity_actor( item_location &it, const bool is_long ) :
    craft_item( it ), is_long( is_long )
{
    cached_crafting_speed = 0;
    cached_assistants = 0;
    cached_base_total_moves = 1;
    cached_cur_total_moves = 1;
}

bool craft_activity_actor::check_if_craft_okay( item_location &craft_item, Character &crafter )
{
    item *craft = craft_item.get_item();

    // item_location::get_item() will return nullptr if the item is lost
    if( !craft ) {
        crafter.add_msg_player_or_npc(
            _( "You no longer have the in progress craft in your possession.  "
               "You stop crafting.  "
               "Reactivate the in progress craft to continue crafting." ),
            _( "<npcname> no longer has the in progress craft in their possession.  "
               "<npcname> stops crafting." ) );
        return false;
    }

    if( !craft->is_craft() ) {
        debugmsg( "ACT_CRAFT target '%s' is not a craft.  Aborting ACT_CRAFT.", craft->tname() );
        return false;
    }

    if( !cached_continuation_requirements ) {
        cached_continuation_requirements = craft->get_continue_reqs();
    }
    return crafter.can_continue_craft( *craft, *cached_continuation_requirements );
}

void craft_activity_actor::start( player_activity &act, Character &crafter )
{
    if( !check_if_craft_okay( craft_item, crafter ) ) {
        act.set_to_null();
    }
    act.moves_left = calendar::INDEFINITELY_LONG;
    activity_override = craft_item.get_item()->get_making().exertion_level();
    cached_crafting_speed = 0;
}

void craft_activity_actor::do_turn( player_activity &act, Character &crafter )
{
    if( !check_if_craft_okay( craft_item, crafter ) ) {
        act.set_to_null();
        return;
    }

    // We already checked if this is nullptr above
    item &craft = *craft_item.get_item();

    const cata::optional<tripoint> location = craft_item.where() == item_location::type::character
            ? cata::optional<tripoint>() : cata::optional<tripoint>( craft_item.position() );
    const recipe &rec = craft.get_making();
    const float crafting_speed = crafter.crafting_speed_multiplier( craft, location );
    const int assistants = crafter.available_assistant_count( craft.get_making() );

    if( crafting_speed <= 0.0f ) {
        crafter.cancel_activity();
        return;
    }

    if( cached_crafting_speed != crafting_speed || cached_assistants != assistants ) {
        cached_crafting_speed = crafting_speed;
        cached_assistants = assistants;

        // Base moves for batch size with no speed modifier or assistants
        // Must ensure >= 1 so we don't divide by 0;
        cached_base_total_moves = std::max( static_cast<int64_t>( 1 ),
                                            rec.batch_time( crafter, craft.charges, 1.0f, 0 ) );
        // Current expected total moves, includes crafting speed modifiers and assistants
        cached_cur_total_moves = std::max( static_cast<int64_t>( 1 ),
                                           rec.batch_time( crafter, craft.charges, crafting_speed,
                                                   assistants ) );
    }
    const double base_total_moves = cached_base_total_moves;
    const double cur_total_moves = cached_cur_total_moves;

    // item_counter represents the percent progress relative to the base batch time
    // stored precise to 5 decimal places ( e.g. 67.32 percent would be stored as 6'732'000 )
    const int old_counter = craft.item_counter;

    // Delta progress in moves adjusted for current crafting speed /
    //crafter.exertion_adjusted_move_multiplier( exertion_level() )
    int spent_moves = crafter.get_moves() * crafter.exertion_adjusted_move_multiplier(
                          exertion_level() );
    const double delta_progress = spent_moves * base_total_moves / cur_total_moves;
    // Current progress in moves
    const double current_progress = craft.item_counter * base_total_moves / 10'000'000.0 +
                                    delta_progress;
    // Current progress as a percent of base_total_moves to 2 decimal places
    craft.item_counter = std::round( current_progress / base_total_moves * 10'000'000.0 );
    crafter.set_moves( 0 );

    // This is to ensure we don't over count skill steps
    craft.item_counter = std::min( craft.item_counter, 10'000'000 );

    // This nominal craft time is also how many practice ticks to perform
    // spread out evenly across the actual duration.
    const double total_practice_ticks = rec.time_to_craft_moves( crafter,
                                        recipe_time_flag::ignore_proficiencies ) / 100.0;

    const int ticks_per_practice = 10'000'000.0 / total_practice_ticks;
    int num_practice_ticks = craft.item_counter / ticks_per_practice -
                             old_counter / ticks_per_practice;
    if( num_practice_ticks > 0 ) {
        crafter.craft_skill_gain( craft, num_practice_ticks );
    }
    // Proficiencies and tools are gained/consumed after every 5% progress
    int five_percent_steps = craft.item_counter / 500'000 - old_counter / 500'000;
    if( five_percent_steps > 0 ) {
        // Divide by 100 for seconds, 20 for 5%
        const time_duration pct_time = time_duration::from_seconds( base_total_moves / 2000 );
        crafter.craft_proficiency_gain( craft, pct_time * five_percent_steps );
        // Invalidate the crafting time cache because proficiencies may have changed
        cached_crafting_speed = 0;
    }

    // Unlike skill, tools are consumed once at the start and should not be consumed at the end
    if( craft.item_counter >= 10'000'000 ) {
        --five_percent_steps;
    }

    if( five_percent_steps > 0 ) {
        if( !crafter.craft_consume_tools( craft, five_percent_steps, false ) ) {
            // So we don't skip over any tool comsuption
            craft.item_counter -= craft.item_counter % 500'000 + 1;
            crafter.cancel_activity();
            return;
        }
    }

    // if item_counter has reached 100% or more
    if( craft.item_counter >= 10'000'000 ) {
        item craft_copy = craft;
        craft_item.remove_item();
        // We need to cache this before we cancel the activity else we risk Use After Free
        bool will_continue = is_long;
        crafter.cancel_activity();
        crafter.complete_craft( craft_copy, location );
        if( will_continue ) {
            if( crafter.making_would_work( crafter.lastrecipe, craft_copy.charges ) ) {
                crafter.last_craft->execute( location );
            }
        }
    } else if( craft.item_counter >= craft.get_next_failure_point() ) {
        bool destroy = craft.handle_craft_failure( crafter );
        // If the craft needs to be destroyed, do it and stop crafting.
        if( destroy ) {
            crafter.add_msg_player_or_npc( _( "There is nothing left of the %s to craft from." ),
                                           _( "There is nothing left of the %s <npcname> was crafting." ), craft.tname() );
            craft_item.remove_item();
            crafter.cancel_activity();
        }
    }
}

void craft_activity_actor::finish( player_activity &act, Character & )
{
    act.set_to_null();
}

std::string craft_activity_actor::get_progress_message( const player_activity & ) const
{
    if( !craft_item ) {
        //We have somehow lost the craft item.  This will be handled in do_turn in the check_if_craft_is_ok call.
        return "";
    }
    return craft_item.get_item()->tname();
}

float craft_activity_actor::exertion_level() const
{
    return activity_override;
}

void craft_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "craft_loc", craft_item );
    jsout.member( "long", is_long );

    jsout.end_object();
}

std::unique_ptr<activity_actor> craft_activity_actor::deserialize( JsonIn &jsin )
{
    item_location tmp_item_loc;
    bool tmp_long;

    JsonObject data = jsin.get_object();

    data.read( "craft_loc", tmp_item_loc );
    data.read( "long", tmp_long );

    craft_activity_actor actor = craft_activity_actor( tmp_item_loc, tmp_long );

    return actor.clone();
}

void workout_activity_actor::start( player_activity &act, Character &who )
{
    if( who.get_fatigue() > fatigue_levels::DEAD_TIRED ) {
        who.add_msg_if_player( _( "You are too tired to exercise." ) );
        act_id = activity_id::NULL_ID();
        act.set_to_null();
        return;
    }
    if( who.get_thirst() > 240 ) {
        who.add_msg_if_player( _( "You are too dehydrated to exercise." ) );
        act_id = activity_id::NULL_ID();
        act.set_to_null();
        return;
    }
    if( who.is_armed() ) {
        who.add_msg_if_player( _( "Empty your hands first." ) );
        act_id = activity_id::NULL_ID();
        act.set_to_null();
        return;
    }
    map &here = get_map();
    // free training requires all limbs intact, but specialized workout machines
    // train upper or lower parts of body only and may permit workout with
    // broken limbs as long as they are not involved by the machine
    bool hand_equipment = here.has_flag_furn( "WORKOUT_ARMS", location );
    bool leg_equipment = here.has_flag_furn( "WORKOUT_LEGS", location );

    if( hand_equipment && ( ( who.is_limb_broken( body_part_arm_l ) ) ||
                            who.is_limb_broken( body_part_arm_r ) ) ) {
        who.add_msg_if_player( _( "You cannot train here with a broken arm." ) );
        act_id = activity_id::NULL_ID();
        act.set_to_null();
        return;
    }
    if( leg_equipment && ( ( who.is_limb_broken( body_part_leg_l ) ) ||
                           who.is_limb_broken( body_part_leg_r ) ) ) {
        who.add_msg_if_player( _( "You cannot train here with a broken leg." ) );
        act_id = activity_id::NULL_ID();
        act.set_to_null();
        return;
    }
    if( !hand_equipment && !leg_equipment &&
        ( who.is_limb_broken( body_part_arm_l ) ||
          who.is_limb_broken( body_part_arm_r ) ||
          who.is_limb_broken( body_part_leg_l ) ||
          who.is_limb_broken( body_part_leg_r ) ) ) {
        who.add_msg_if_player( _( "You cannot train freely with a broken limb." ) );
        act_id = activity_id::NULL_ID();
        act.set_to_null();
        return;
    }
    uilist workout_query;
    workout_query.desc_enabled = true;
    workout_query.text =
        _( "Physical effort determines workout efficiency, but also rate of exhaustion." );
    workout_query.title = _( "Choose training intensity:" );
    workout_query.addentry_desc( 1, true, 'l', pgettext( "training intensity", "Light" ),
                                 _( "Light exercise comparable in intensity to walking, but more focused and methodical." ) );
    workout_query.addentry_desc( 2, true, 'm', pgettext( "training intensity", "Moderate" ),
                                 _( "Moderate exercise without excessive exertion, but with enough effort to break a sweat." ) );
    workout_query.addentry_desc( 3, true, 'a', pgettext( "training intensity", "Active" ),
                                 _( "Active exercise with full involvement.  Strenuous, but in a controlled manner." ) );
    workout_query.addentry_desc( 4, true, 'h', pgettext( "training intensity", "High" ),
                                 _( "High intensity exercise with maximum effort and full power.  Exhausting in the long run." ) );
    workout_query.query();
    switch( workout_query.ret ) {
        case UILIST_CANCEL:
            act.set_to_null();
            act_id = activity_id::NULL_ID();
            return;
        case 4:
            act_id = activity_id( "ACT_WORKOUT_HARD" );
            intensity_modifier = 4;
            break;
        case 3:
            act_id = activity_id( "ACT_WORKOUT_ACTIVE" );
            intensity_modifier = 3;
            break;
        case 2:
            act_id = activity_id( "ACT_WORKOUT_MODERATE" );
            intensity_modifier = 2;
            break;
        case 1:
        default:
            act_id = activity_id( "ACT_WORKOUT_LIGHT" );
            intensity_modifier = 1;
            break;
    }
    int length;
    query_int( length, _( "Train for how long (minutes): " ) );
    if( length > 0 ) {
        duration = length * 1_minutes;
    } else {
        act_id = activity_id::NULL_ID();
        act.set_to_null();
        return;
    }
    act.moves_total = to_moves<int>( duration );
    act.moves_left = act.moves_total;
    if( who.male ) {
        sfx::play_activity_sound( "plmove", "fatigue_m_med", sfx::get_heard_volume( location ) );
    } else {
        sfx::play_activity_sound( "plmove", "fatigue_f_med", sfx::get_heard_volume( location ) );
    }
    who.add_msg_if_player( _( "You start your workout session." ) );
}

void workout_activity_actor::do_turn( player_activity &act, Character &who )
{
    if( who.get_fatigue() > fatigue_levels::DEAD_TIRED ) {
        who.add_msg_if_player( _( "You are exhausted so you finish your workout early." ) );
        act.set_to_null();
        return;
    }
    if( who.get_thirst() > 240 ) {
        who.add_msg_if_player( _( "You are dehydrated so you finish your workout early." ) );
        act.set_to_null();
        return;
    }
    if( !rest_mode && who.get_stamina() > who.get_stamina_max() / 3 ) {
        who.mod_stamina( -25 - intensity_modifier );
        if( one_in( 180 / intensity_modifier ) ) {
            who.mod_fatigue( 1 );
            who.mod_thirst( 1 );
        }
        if( calendar::once_every( 16_minutes / intensity_modifier ) ) {
            //~ heavy breathing when exercising
            std::string huff = _( "yourself huffing and puffing!" );
            sounds::sound( location + tripoint_east, 2 * intensity_modifier, sounds::sound_t::speech, huff,
                           true );
        }
        // morale bonus kicks in gradually after 5 minutes of exercise
        if( calendar::once_every( 2_minutes ) &&
            ( ( elapsed + act.moves_total - act.moves_left ) / 100 * 1_turns ) > 5_minutes ) {
            who.add_morale( MORALE_FEELING_GOOD, intensity_modifier, 20, 6_hours, 30_minutes );
        }
        if( calendar::once_every( 2_minutes ) ) {
            who.add_msg_debug_if_player( debugmode::DF_ACT_WORKOUT, who.activity_level_str() );
            who.add_msg_debug_if_player( debugmode::DF_ACT_WORKOUT, act.id().c_str() );
        }
    } else if( !rest_mode ) {
        rest_mode = true;
        who.add_msg_if_player( _( "You catch your breath for few moments." ) );
    } else if( who.get_stamina() >= who.get_stamina_max() ) {
        rest_mode = false;
        who.add_msg_if_player( _( "You get back to your training." ) );
    }
}

void workout_activity_actor::finish( player_activity &act, Character &who )
{
    if( !query_keep_training( act, who ) ) {
        act.set_to_null();
        who.add_msg_if_player( _( "You finish your workout session." ) );
    }
}

void workout_activity_actor::canceled( player_activity &/*act*/, Character &/*who*/ )
{
    stop_time = calendar::turn;
}

bool workout_activity_actor::query_keep_training( player_activity &act, Character &who )
{
    if( disable_query || !who.is_avatar() ) {
        elapsed += act.moves_total - act.moves_left;
        act.moves_total = to_moves<int>( 60_minutes );
        act.moves_left = act.moves_total;
        return true;
    }
    int length;
    uilist workout_query;
    workout_query.text = _( "You have finished your training cycle, keep training?" );
    workout_query.addentry( 1, true, 'S', _( "Stop training." ) );
    workout_query.addentry( 2, true, 'c', _( "Continue training." ) );
    workout_query.addentry( 3, true, 'C', _( "Continue training and don't ask again." ) );
    workout_query.query();
    switch( workout_query.ret ) {
        case UILIST_CANCEL:
        case 1:
            act_id = activity_id::NULL_ID();
            act.set_to_null();
            return false;
        case 3:
            disable_query = true;
            elapsed += act.moves_total - act.moves_left;
            act.moves_total = to_moves<int>( 60_minutes );
            act.moves_left = act.moves_total;
            return true;
        case 2:
        default:
            query_int( length, _( "Train for how long (minutes): " ) );
            elapsed += act.moves_total - act.moves_left;
            act.moves_total = to_moves<int>( length * 1_minutes );
            act.moves_left = act.moves_total;
            return true;
    }
}

void workout_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "disable_query", disable_query );
    jsout.member( "act_id", act_id );
    jsout.member( "duration", duration );
    jsout.member( "location", location );
    jsout.member( "stop_time", stop_time );
    jsout.member( "elapsed", elapsed );
    jsout.member( "intensity_modifier", intensity_modifier );
    jsout.member( "rest_mode", rest_mode );

    jsout.end_object();
}

std::unique_ptr<activity_actor> workout_activity_actor::deserialize( JsonIn &jsin )
{
    workout_activity_actor actor = workout_activity_actor( tripoint_zero );

    JsonObject data = jsin.get_object();

    data.read( "disable_query", actor.disable_query );
    data.read( "act_id", actor.act_id );
    data.read( "duration", actor.duration );
    data.read( "location", actor.location );
    data.read( "stop_time", actor.stop_time );
    data.read( "elapsed", actor.elapsed );
    data.read( "intensity_modifier", actor.intensity_modifier );
    data.read( "rest_mode", actor.rest_mode );

    return actor.clone();
}

// TODO: Display costs in the multidrop menu
static void debug_drop_list( const std::vector<drop_or_stash_item_info> &items )
{
    if( !debug_mode ) {
        return;
    }

    std::string res( "Items ordered to drop:\n" );
    for( const drop_or_stash_item_info &it : items ) {
        item_location loc = it.loc();
        if( !loc ) {
            // some items could have been destroyed by e.g. monster attack
            continue;
        }
        res += string_format( "Drop %d %s\n", it.count(), loc->display_name( it.count() ) );
    }
    popup( res, PF_GET_KEY );
}

// Return a list of items to be dropped by the given item-dropping activity in the current turn.
static std::list<item> obtain_activity_items(
    std::vector<drop_or_stash_item_info> &items,
    contents_change_handler &handler,
    Character &who )
{
    std::list<item> res;

    debug_drop_list( items );

    auto it = items.begin();
    for( ; it != items.end(); ++it ) {
        item_location loc = it->loc();
        if( !loc ) {
            // some items could have been destroyed by e.g. monster attack
            continue;
        }

        const int consumed_moves = loc.obtain_cost( who, it->count() );
        if( !who.is_npc() && who.moves <= 0 && consumed_moves > 0 ) {
            break;
        }

        who.mod_moves( -consumed_moves );

        // If item is inside another (container/pocket), unseal it, and update encumbrance
        if( loc.has_parent() ) {
            // Save parent items to be handled when finishing or canceling activity,
            // in case there are still other items to drop from the same container.
            // This assumes that an item and any of its parents/ancestors are not
            // dropped at the same time.
            handler.unseal_pocket_containing( loc );
        } else {
            // when parent's encumbrance cannot be marked as dirty,
            // mark character's encumbrance as dirty instead (correctness over performance)
            who.set_check_encumbrance( true );
        }

        // Take off the item or remove it from the player's inventory
        if( who.is_worn( *loc ) ) {
            who.as_player()->takeoff( loc, &res );
        } else if( loc->count_by_charges() ) {
            res.push_back( who.as_player()->reduce_charges( &*loc, it->count() ) );
        } else {
            res.push_back( who.i_rem( &*loc ) );
        }
    }

    // Remove handled items from activity
    items.erase( items.begin(), it );
    return res;
}

void drop_or_stash_item_info::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "loc", _loc );
    jsout.member( "count", _count );
    jsout.end_object();
}

void drop_or_stash_item_info::deserialize( JsonIn &jsin )
{
    JsonObject jsobj = jsin.get_object();
    jsobj.read( "loc", _loc );
    jsobj.read( "count", _count );
}

void drop_activity_actor::do_turn( player_activity &, Character &who )
{
    const tripoint pos = placement + who.pos();
    who.invalidate_weight_carried_cache();
    put_into_vehicle_or_drop( who, item_drop_reason::deliberate,
                              obtain_activity_items( items, handler, who ),
                              pos, force_ground );
    // Cancel activity if items is empty. Otherwise, we modified in place and we will continue
    // to resolve the drop next turn. This is different from the pickup logic which creates
    // a brand new activity every turn and cancels the old activity
    if( items.empty() ) {
        who.cancel_activity();
    }
}

void drop_activity_actor::canceled( player_activity &, Character &who )
{
    handler.handle_by( who );
}

void drop_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "items", items );
    jsout.member( "unhandled_containers", handler );
    jsout.member( "placement", placement );
    jsout.member( "force_ground", force_ground );
    jsout.end_object();
}

std::unique_ptr<activity_actor> drop_activity_actor::deserialize( JsonIn &jsin )
{
    drop_activity_actor actor;

    JsonObject jsobj = jsin.get_object();
    jsobj.read( "items", actor.items );
    jsobj.read( "unhandled_containers", actor.handler );
    jsobj.read( "placement", actor.placement );
    jsobj.read( "force_ground", actor.force_ground );

    return actor.clone();
}

static void stash_on_pet( const std::list<item> &items, monster &pet, Character &who )
{
    units::volume remaining_volume = pet.storage_item->get_total_capacity() - pet.get_carried_volume();
    units::mass remaining_weight = pet.weight_capacity() - pet.get_carried_weight();
    map &here = get_map();

    for( const item &it : items ) {
        if( it.volume() > remaining_volume ) {
            add_msg( m_bad, _( "%1$s did not fit and fell to the %2$s." ), it.display_name(),
                     here.name( pet.pos() ) );
            here.add_item_or_charges( pet.pos(), it );
        } else if( it.weight() > remaining_weight ) {
            add_msg( m_bad, _( "%1$s is too heavy and fell to the %2$s." ), it.display_name(),
                     here.name( pet.pos() ) );
            here.add_item_or_charges( pet.pos(), it );
        } else {
            pet.add_item( it );
            remaining_volume -= it.volume();
            remaining_weight -= it.weight();
        }
        // TODO: if NPCs can have pets or move items onto pets
        item( it ).handle_pickup_ownership( who );
    }
}

void stash_activity_actor::do_turn( player_activity &, Character &who )
{
    const tripoint pos = placement + who.pos();

    monster *pet = g->critter_at<monster>( pos );
    if( pet != nullptr && pet->has_effect( effect_pet ) ) {
        stash_on_pet( obtain_activity_items( items, handler, who ),
                      *pet, who );
        if( items.empty() ) {
            who.cancel_activity();
        }
    } else {
        who.add_msg_if_player( _( "The pet has moved somewhere else." ) );
        who.cancel_activity();
    }
}

void stash_activity_actor::canceled( player_activity &, Character &who )
{
    handler.handle_by( who );
}

void stash_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "items", items );
    jsout.member( "unhandled_containers", handler );
    jsout.member( "placement", placement );
    jsout.end_object();
}

std::unique_ptr<activity_actor> stash_activity_actor::deserialize( JsonIn &jsin )
{
    stash_activity_actor actor;

    JsonObject jsobj = jsin.get_object();
    jsobj.read( "items", actor.items );
    jsobj.read( "unhandled_containers", actor.handler );
    jsobj.read( "placement", actor.placement );

    return actor.clone();
}

void move_furniture_activity_actor::start( player_activity &act, Character & )
{
    int moves = g->grabbed_furn_move_time( dp );
    act.moves_left = moves;
    act.moves_total = moves;
}

void move_furniture_activity_actor::finish( player_activity &act, Character &who )
{
    if( !g->grabbed_furn_move( dp ) ) {
        g->walk_move( who.pos() + dp, via_ramp, true );
    }
    act.set_to_null();
}

void move_furniture_activity_actor::canceled( player_activity &, Character & )
{
    add_msg( m_warning, _( "You let go of the grabbed object." ) );
    get_avatar().grab( object_type::NONE );
}

void move_furniture_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "dp", dp );
    jsout.member( "via_ramp", via_ramp );
    jsout.end_object();
}

std::unique_ptr<activity_actor> move_furniture_activity_actor::deserialize( JsonIn &jsin )
{
    move_furniture_activity_actor actor = move_furniture_activity_actor( tripoint_zero, false );

    JsonObject data = jsin.get_object();

    data.read( "dp", actor.dp );
    data.read( "via_ramp", actor.via_ramp );

    return actor.clone();
}

static int item_move_cost( Character &who, item_location &item )
{
    // Cost to take an item from a container or map
    const int obtain_cost = item.obtain_cost( who );
    // Cost to move an item to a container, vehicle or the ground
    const int move_cost = Pickup::cost_to_move_item( who, *item );
    return obtain_cost + move_cost;
}

void insert_item_activity_actor::start( player_activity &act, Character &who )
{
    if( items.empty() ) {
        debugmsg( "ACT_INSERT_ITEM was passed an empty list" );
        act.set_to_null();
    }

    all_pockets_rigid = holster->all_pockets_rigid();

    const int total_moves = item_move_cost( who, items.front().first );
    act.moves_left = total_moves;
    act.moves_total = total_moves;
}

void insert_item_activity_actor::finish( player_activity &act, Character &who )
{
    bool success = false;
    drop_location &holstered_item = items.front();
    if( holstered_item.first ) {
        item &it = *holstered_item.first;
        if( !it.count_by_charges() ) {
            if( holster->can_contain( it ) && ( all_pockets_rigid ||
                                                holster.parents_can_contain_recursive( &it ) ) ) {

                success = holster->put_in( it, item_pocket::pocket_type::CONTAINER,
                                           /*unseal_pockets=*/true ).success();
                if( success ) {
                    //~ %1$s: item to put in the container, %2$s: container to put item in
                    who.add_msg_if_player( string_format( _( "You put your %1$s into the %2$s." ),
                                                          holstered_item.first->display_name(), holster->type->nname( 1 ) ) );
                    handler.add_unsealed( holster );
                    handler.unseal_pocket_containing( holstered_item.first );
                    holstered_item.first.remove_item();
                }

            }
        } else {
            int charges = all_pockets_rigid ? holstered_item.second : std::min( holstered_item.second,
                          holster.max_charges_by_parent_recursive( it ) );

            if( charges > 0 && holster->can_contain_partial( it ) ) {
                int result = holster->fill_with( it, charges,
                                                 /*unseal_pockets=*/true,
                                                 /*allow_sealed=*/true,
                                                 /*ignore_settings*/true );
                success = result > 0;

                if( success ) {
                    item copy( it );
                    copy.charges = result;
                    //~ %1$s: item to put in the container, %2$s: container to put item in
                    who.add_msg_if_player( string_format( _( "You put your %1$s into the %2$s." ),
                                                          copy.display_name(), holster->type->nname( 1 ) ) );
                    handler.add_unsealed( holster );
                    handler.unseal_pocket_containing( holstered_item.first );
                    it.charges -= result;
                    if( it.charges == 0 ) {
                        holstered_item.first.remove_item();
                    }
                }
            }
        }

        if( !success ) {
            who.add_msg_if_player(
                string_format(
                    _( "Could not put %1$s into %2$s, aborting." ),
                    it.tname(), holster->tname() ) );
        }
    } else {
        // item was lost, so just go to next item
        success = true;
    }

    items.pop_front();
    if( items.empty() || !success ) {
        handler.handle_by( who );
        act.set_to_null();
        return;
    }

    // Restart the activity
    const int total_moves = item_move_cost( who, items.front().first );
    act.moves_left = total_moves;
    act.moves_total = total_moves;
}

void insert_item_activity_actor::canceled( player_activity &/*act*/, Character &who )
{
    handler.handle_by( who );
}

void insert_item_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "holster", holster );
    jsout.member( "items", items );
    jsout.member( "handler", handler );
    jsout.member( "all_pockets_rigid", all_pockets_rigid );

    jsout.end_object();
}

std::unique_ptr<activity_actor> insert_item_activity_actor::deserialize( JsonIn &jsin )
{
    insert_item_activity_actor actor;

    JsonObject data = jsin.get_object();

    data.read( "holster", actor.holster );
    data.read( "items", actor.items );
    data.read( "handler", actor.handler );
    data.read( "all_pockets_rigid", actor.all_pockets_rigid );

    return actor.clone();
}

bool reload_activity_actor::can_reload() const
{
    if( reload_targets.size() != 2 || quantity <= 0 ) {
        debugmsg( "invalid arguments to ACT_RELOAD" );
        return false;
    }

    if( !reload_targets[0] ) {
        debugmsg( "reload target is null, failed to reload" );
        return false;
    }

    if( !reload_targets[1] ) {
        debugmsg( "ammo target is null, failed to reload" );
        return false;
    }

    return true;
}

void reload_activity_actor::start( player_activity &act, Character &/*who*/ )
{
    act.moves_total = moves_total;
    act.moves_left = moves_total;
}

void reload_activity_actor::make_reload_sound( Character &who, item &reloadable )
{
    if( reloadable.type->gun->reload_noise_volume > 0 ) {
        sfx::play_variant_sound( "reload", reloadable.typeId().str(),
                                 sfx::get_heard_volume( who.pos() ) );
        sounds::ambient_sound( who.pos(), reloadable.type->gun->reload_noise_volume,
                               sounds::sound_t::activity, reloadable.type->gun->reload_noise.translated() );
    }
}

void reload_activity_actor::finish( player_activity &act, Character &who )
{
    act.set_to_null();
    if( !can_reload() ) {
        return;
    }

    item &reloadable = *reload_targets[ 0 ];
    item &ammo = *reload_targets[ 1 ];
    const std::string reloadable_name = reloadable.tname();
    // cache check results because reloading deletes the ammo item
    const std::string ammo_name = ammo.tname();
    const bool ammo_is_filthy = ammo.is_filthy();
    const bool ammo_uses_speedloader = ammo.has_flag( flag_SPEEDLOADER );

    if( !reloadable.reload( who, std::move( reload_targets[ 1 ] ), quantity ) ) {
        add_msg( m_info, _( "Can't reload the %s." ), reloadable_name );
        return;
    }

    if( ammo_is_filthy ) {
        reloadable.set_flag( flag_FILTHY );
    }

    if( reloadable.get_var( "dirt", 0 ) > 7800 ) {
        add_msg( m_neutral, _( "You manage to loosen some debris and make your %s somewhat operational." ),
                 reloadable_name );
        reloadable.set_var( "dirt", ( reloadable.get_var( "dirt", 0 ) - rng( 790, 2750 ) ) );
    }

    if( reloadable.is_gun() ) {
        who.recoil = MAX_RECOIL;
        if( reloadable.has_flag( flag_RELOAD_ONE ) && !ammo_uses_speedloader ) {
            add_msg( m_neutral, _( "You insert %dx %s into the %s." ), quantity, ammo_name, reloadable_name );
        }
        make_reload_sound( who, reloadable );
    } else if( reloadable.is_watertight_container() ) {
        add_msg( m_neutral, _( "You refill the %s." ), reloadable_name );
    } else {
        add_msg( m_neutral, _( "You reload the %1$s with %2$s." ), reloadable_name, ammo_name );
    }
    item_location loc = reload_targets[0];
    // Reload may have caused the item to increase in size more than the pocket/location can contain.
    // We want to avoid this because items will be deleted on a save/load.
    if( loc.volume_capacity() < units::volume() ||
        loc.weight_capacity() < units::mass() ) {
        // In player inventory and player is wielding nothing.
        if( !who.is_armed() && loc.held_by( who ) ) {
            add_msg( m_neutral, _( "The %s no longer fits in your inventory so you wield it instead." ),
                     reloadable_name );
            who.wield( reloadable );
        } else {
            // In player inventory and player is wielding something.
            if( loc.held_by( who ) ) {
                add_msg( m_neutral, _( "The %s no longer fits in your inventory so you drop it instead." ),
                         reloadable_name );
                // Default handling message.
            } else {
                add_msg( m_neutral, _( "The %s no longer fits its location so you drop it instead." ),
                         reloadable_name );
            }
            get_map().add_item_or_charges( loc.position(), reloadable );
            loc.remove_item();
        }
    }
}

void reload_activity_actor::canceled( player_activity &act, Character &/*who*/ )
{
    act.moves_total = 0;
    act.moves_left = 0;
}

void reload_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves_total", moves_total );
    jsout.member( "qty", quantity );
    jsout.member( "reload_targets", reload_targets );

    jsout.end_object();
}

std::unique_ptr<activity_actor> reload_activity_actor::deserialize( JsonIn &jsin )
{
    reload_activity_actor actor;

    JsonObject data = jsin.get_object();

    data.read( "moves_total", actor.moves_total );
    data.read( "qty", actor.quantity );
    data.read( "reload_targets", actor.reload_targets );
    return actor.clone();
}

void milk_activity_actor::start( player_activity &act, Character &/*who*/ )
{
    act.moves_total = total_moves;
    act.moves_left = total_moves;
}

void milk_activity_actor::finish( player_activity &act, Character &who )
{
    if( monster_coords.empty() ) {
        debugmsg( "milking activity with no position of monster stored" );
        return;
    }
    map &here = get_map();
    const tripoint source_pos = here.getlocal( monster_coords.at( 0 ) );
    monster *source_mon = g->critter_at<monster>( source_pos );
    if( source_mon == nullptr ) {
        debugmsg( "could not find source creature for liquid transfer" );
        return;
    }
    auto milked_item = source_mon->ammo.find( source_mon->type->starting_ammo.begin()->first );
    if( milked_item == source_mon->ammo.end() ) {
        debugmsg( "animal has no milkable ammo type" );
        return;
    }
    if( milked_item->second <= 0 ) {
        debugmsg( "started milking but udders are now empty before milking finishes" );
        return;
    }
    item milk( milked_item->first, calendar::turn, milked_item->second );
    milk.set_item_temperature( 311.75 );
    if( liquid_handler::handle_liquid( milk, nullptr, 1, nullptr, nullptr, -1, source_mon ) ) {
        milked_item->second = 0;
        if( milk.charges > 0 ) {
            milked_item->second = milk.charges;
        } else {
            who.add_msg_if_player( _( "The %s's udders run dry." ), source_mon->get_name() );
        }
    }
    // if the monster was not manually tied up, but needed to be fixed in place temporarily then
    // remove that now.
    if( !string_values.empty() && string_values[0] == "temp_tie" ) {
        source_mon->remove_effect( effect_tied );
    }

    act.set_to_null();
}

void milk_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "total_moves", total_moves );
    jsout.member( "monster_coords", monster_coords );
    jsout.member( "string_values", string_values );

    jsout.end_object();
}

std::unique_ptr<activity_actor> milk_activity_actor::deserialize( JsonIn &jsin )
{
    milk_activity_actor actor;
    JsonObject data = jsin.get_object();

    data.read( "moves_total", actor.total_moves );
    data.read( "monster_coords", actor.monster_coords );
    data.read( "string_values", actor.string_values );

    return actor.clone();
}

void shearing_activity_actor::start( player_activity &act, Character &who )
{
    monster *mon = g->critter_at<monster>( mon_coords );
    if( !mon ) {
        debugmsg( "shearing lost monster when starting" );
        act.set_to_null();
        return;
    }

    std::string pet_name_capitalized = mon->unique_name.empty() ? mon->disp_name( false,
                                       true ) : mon->unique_name;

    if( !mon->shearable() ) {
        add_msg( _( "%1$s has nothing %2$s could shear." ), pet_name_capitalized, who.disp_name() );
        if( shearing_tie ) {
            mon->remove_effect( effect_tied );
        }
        act.set_to_null();
        return;
    }

    const int shearing_quality = who.max_quality( qual_SHEAR );
    if( !( shearing_quality > 0 ) ) {
        if( who.is_player() ) {
            add_msg( m_info, _( "%1$s don't have a shearing tool." ), who.disp_name( false, true ) );
        } else { // who.is_npc
            // npcs can't shear monsters yet, this is for when they are able to
            add_msg_if_player_sees( who, _( "%1$s doesn't have a shearing tool." ), who.disp_name(),
                                    mon->disp_name() );
        }
        if( shearing_tie ) {
            mon->remove_effect( effect_tied );
        }
        act.set_to_null();
        return;
    }

    const time_duration shearing_time = 30_minutes / shearing_quality;
    add_msg_debug( debugmode::DF_ACT_SHEARING, "shearing_time time = %s", to_string( shearing_time ) );

    if( who.is_player() ) {
        add_msg( m_info,
                 _( "%1$s start shearing %2$s." ), who.disp_name( false, true ), mon->disp_name() );
    } else { // who.is_npc
        // npcs can't shear monsters yet, this is for when they are able to
        add_msg_if_player_sees( who, _( "%1$s starts shearing %2$s." ), who.disp_name(),
                                mon->disp_name() );
    }

    act.moves_total = to_moves<int>( shearing_time );
    act.moves_left = act.moves_total;
}

void shearing_activity_actor::do_turn( player_activity &, Character &who )
{
    if( !who.has_quality( qual_SHEAR ) ) {
        if( who.is_player() ) {
            add_msg(
                m_bad,
                _( "%1$s don't have a shearing tool anymore." ),
                who.disp_name( false, true ) );
        } else {
            add_msg_if_player_sees(
                who,
                _( "%1$s doesn't have a shearing tool anymore." ),
                who.disp_name() );
        }
        who.cancel_activity();
    }
}

void shearing_activity_actor::finish( player_activity &act, Character &who )
{
    monster *mon = g->critter_at<monster>( mon_coords );
    if( !mon ) {
        debugmsg( "shearing monster at position disappeared" );
        act.set_to_null();
        return;
    }

    const shearing_data &shear_data = mon->type->shearing;
    if( !shear_data.valid() ) {
        debugmsg( "shearing monster does not have shearing_data" );
        if( shearing_tie ) {
            mon->remove_effect( effect_tied );
        }
        act.set_to_null();
        return;
    }

    map &mp = get_map();

    add_msg_if_player_sees( who,
                            string_format(
                                _( "%1$s finished shearing %2$s and got:" ),
                                who.disp_name( false, true ),
                                mon->unique_name.empty() ? mon->disp_name() : mon->unique_name ) );

    const std::vector<shearing_roll> shear_roll = shear_data.roll_all( *mon );
    for( const shearing_roll &roll : shear_roll ) {
        if( roll.result->count_by_charges() ) {
            item shear_item( roll.result, calendar::turn, roll.amount );
            mp.add_item_or_charges( who.pos(), shear_item );
            add_msg_if_player_sees( who, shear_item.display_name() );
        } else {
            item shear_item( roll.result, calendar::turn );
            for( int i = 0; i < roll.amount; ++i ) {
                mp.add_item_or_charges( who.pos(), shear_item );
            }
            add_msg_if_player_sees( who,
                                    //~ %1$s - item, %2$d - amount
                                    string_format( _( "%1$s x%2$d" ),
                                                   shear_item.display_name(), roll.amount ) );
        }
    }

    mon->add_effect( effect_sheared, calendar::season_length() );
    if( shearing_tie ) {
        mon->remove_effect( effect_tied );
    }

    act.set_to_null();
}

void shearing_activity_actor::canceled( player_activity &, Character & )
{
    monster *mon = g->critter_at<monster>( mon_coords );
    if( !mon ) {
        return;
    }

    if( shearing_tie ) {
        // free the poor critter
        mon->remove_effect( effect_tied );
    }
}

void shearing_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "mon_coords", mon_coords );
    jsout.member( "shearing_tie", shearing_tie );
    jsout.end_object();
}

std::unique_ptr<activity_actor> shearing_activity_actor::deserialize( JsonIn &jsin )
{
    shearing_activity_actor actor = shearing_activity_actor( {} );

    JsonObject data = jsin.get_object();
    data.read( "mon_coords", actor.mon_coords );
    data.read( "shearing_tie", actor.shearing_tie );
    return actor.clone();
}

static bool check_if_disassemble_okay( item_location target, Character &who )
{
    item *disassembly = target.get_item();

    // item_location::get_item() will return nullptr if the item is lost
    if( !disassembly ) {
        who.add_msg_player_or_npc(
            _( "You no longer have the in progress disassembly in your possession.  "
               "You stop disassembling.  "
               "Reactivate the in progress disassembly to continue disassembling." ),
            _( "<npcname> no longer has the in progress disassembly in their possession.  "
               "<npcname> stops disassembling." ) );
        return false;
    }
    return true;
}

void disassemble_activity_actor::start( player_activity &act, Character &who )
{
    if( act.targets.empty() ) {
        act.set_to_null();
        return;
    }

    if( act.targets.back()->typeId() != itype_disassembly ) {
        target = who.create_in_progress_disassembly( act.targets.back() );
    } else {
        target = act.targets.back();
    }
    act.targets.pop_back();

    if( !check_if_disassemble_okay( target, who ) ) {
        act.set_to_null();
        return;
    }

    // We already checked if this is nullptr above
    item *craft = target.get_item();
    double counter = craft->item_counter;

    act.moves_left = moves_total - ( counter / 10'000'000.0 ) * moves_total;
    act.moves_total = moves_total;
}

void disassemble_activity_actor::do_turn( player_activity &act, Character &who )
{
    if( !check_if_disassemble_okay( target, who ) ) {
        act.set_to_null();
        return;
    }

    // We already checked if this is nullptr above
    item *craft = target.get_item();

    double moves_left = act.moves_left;
    double moves_total = act.moves_total;
    // Current progress as a percent of base_total_moves to 2 decimal places
    craft->item_counter = std::round( ( moves_total - moves_left ) / moves_total * 10'000'000.0 );
}

void disassemble_activity_actor::finish( player_activity &act, Character &who )
{
    if( !check_if_disassemble_okay( target, who ) ) {
        act.set_to_null();
        return;
    }
    who.complete_disassemble( target );
}

void disassemble_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves_total", moves_total );

    jsout.end_object();
}

std::unique_ptr<activity_actor> disassemble_activity_actor::deserialize( JsonIn &jsin )
{
    disassemble_activity_actor actor = disassemble_activity_actor( 0 );

    JsonObject data = jsin.get_object();

    data.read( "moves_total", actor.moves_total );

    return actor.clone();
}

void tent_placement_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves_total;
    act.moves_left = moves_total;
}

void tent_placement_activity_actor::finish( player_activity &act, Character &p )
{
    map &here = get_map();
    const tripoint center = p.pos() + tripoint( ( radius + 1 ) * target.x, ( radius + 1 ) * target.y,
                            0 );

    // Make a square of floor surrounded by wall.
    for( const tripoint &dest : here.points_in_radius( center, radius ) ) {
        here.furn_set( dest, wall );
    }
    for( const tripoint &dest : here.points_in_radius( center, radius - 1 ) ) {
        here.furn_set( dest, floor );
    }
    // Place the center floor and the door.
    if( floor_center ) {
        here.furn_set( center, *floor_center );
    }
    here.furn_set( p.pos() + target, door_closed );

    add_msg( m_info, _( "You set up the %s on the ground." ), it.tname() );
    add_msg( m_info, _( "Examine the center square to pack it up again." ) );
    act.set_to_null();
}

void tent_placement_activity_actor::canceled( player_activity &, Character &p )
{
    map &here = get_map();
    here.add_item_or_charges( p.pos() + target, it, true );
}

void tent_placement_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "moves_total", moves_total );
    jsout.member( "wall", wall );
    jsout.member( "floor", floor );
    jsout.member( "floor_center", floor_center );
    jsout.member( "door_closed", door_closed );
    jsout.member( "radius", radius );
    jsout.member( "it", it );
    jsout.member( "target", target );
    jsout.end_object();
}

std::unique_ptr<activity_actor> tent_placement_activity_actor::deserialize( JsonIn &jsin )
{
    item it;
    tent_placement_activity_actor actor( 0, {}, 0, it, {}, {}, {}, {} );
    JsonObject data = jsin.get_object();
    data.read( "moves_total", actor.moves_total );
    data.read( "wall", actor.wall );
    data.read( "floor", actor.floor );
    data.read( "floor_center", actor.floor_center );
    data.read( "door_closed", actor.door_closed );
    data.read( "radius", actor.radius );
    data.read( "it", actor.it );
    data.read( "target", actor.target );
    return actor.clone();
}

void tent_deconstruct_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves_total;
    act.moves_left = moves_total;
}

void tent_deconstruct_activity_actor::finish( player_activity &act, Character & )
{
    map &here = get_map();
    for( const tripoint &pt : here.points_in_radius( target, radius ) ) {
        here.furn_set( pt, f_null );
    }
    here.add_item_or_charges( target, item( tent, calendar::turn ) );
    act.set_to_null();
}

void tent_deconstruct_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "moves_total", moves_total );
    jsout.member( "radius", radius );
    jsout.member( "target", target );
    jsout.member( "tent", tent );
    jsout.end_object();
}

std::unique_ptr<activity_actor> tent_deconstruct_activity_actor::deserialize( JsonIn &jsin )
{
    tent_deconstruct_activity_actor actor( 0, 0, {}, {} );
    JsonObject data = jsin.get_object();
    data.read( "moves_total", actor.moves_total );
    data.read( "radius", actor.radius );
    data.read( "target", actor.target );
    data.read( "tent", actor.tent );

    return actor.clone();
}

void meditate_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = to_moves<int>( 20_minutes );
    act.moves_left = act.moves_total;
}

void meditate_activity_actor::finish( player_activity &act, Character &who )
{
    who.add_msg_if_player( m_good, _( "You pause to engage in spiritual contemplation." ) );
    who.add_morale( MORALE_FEELING_GOOD, 5, 10 );
    act.set_to_null();
}

void meditate_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.write_null();
}

std::unique_ptr<activity_actor> meditate_activity_actor::deserialize( JsonIn & )
{
    return meditate_activity_actor().clone();
}

void play_with_pet_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = rng( 50, 125 ) * 100;
    act.moves_left = act.moves_total;
}

void play_with_pet_activity_actor::finish( player_activity &act, Character &who )
{
    who.add_morale( MORALE_PLAY_WITH_PET, rng( 3, 10 ), 10, 5_hours, 25_minutes );
    who.add_msg_if_player( m_good, _( "Playing with your %s has lifted your spirits a bit." ),
                           pet_name );
    act.set_to_null();
}

void play_with_pet_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "pet_name", pet_name );

    jsout.end_object();
}

std::unique_ptr<activity_actor> play_with_pet_activity_actor::deserialize( JsonIn &jsin )
{
    play_with_pet_activity_actor actor = play_with_pet_activity_actor();

    JsonObject data = jsin.get_object();

    data.read( "pet_name", actor.pet_name );
    return actor.clone();
}

void shave_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = to_moves<int>( 5_minutes );
    act.moves_left = act.moves_total;
}

void shave_activity_actor::finish( player_activity &act, Character &who )
{
    who.add_msg_if_player( _( "You open up your kit and shave." ) );
    who.add_morale( MORALE_SHAVE, 8, 8, 240_minutes, 3_minutes );
    act.set_to_null();
}

void shave_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.write_null();
}

std::unique_ptr<activity_actor> shave_activity_actor::deserialize( JsonIn & )
{
    return shave_activity_actor().clone();
}

void haircut_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = to_moves<int>( 30_minutes );
    act.moves_left = act.moves_total;
}

void haircut_activity_actor::finish( player_activity &act, Character &who )
{
    who.add_msg_if_player( _( "You give your hair a trim." ) );
    who.add_morale( MORALE_HAIRCUT, 3, 3, 480_minutes, 3_minutes );
    act.set_to_null();
}

void haircut_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.write_null();
}

std::unique_ptr<activity_actor> haircut_activity_actor::deserialize( JsonIn & )
{
    return haircut_activity_actor().clone();
}

namespace activity_actors
{

// Please keep this alphabetically sorted
const std::unordered_map<activity_id, std::unique_ptr<activity_actor>( * )( JsonIn & )>
deserialize_functions = {
    { activity_id( "ACT_AIM" ), &aim_activity_actor::deserialize },
    { activity_id( "ACT_AUTODRIVE" ), &autodrive_activity_actor::deserialize },
    { activity_id( "ACT_BIKERACK_RACKING" ), &bikerack_racking_activity_actor::deserialize },
    { activity_id( "ACT_BIKERACK_UNRACKING" ), &bikerack_unracking_activity_actor::deserialize },
    { activity_id( "ACT_CONSUME" ), &consume_activity_actor::deserialize },
    { activity_id( "ACT_CRAFT" ), &craft_activity_actor::deserialize },
    { activity_id( "ACT_DIG" ), &dig_activity_actor::deserialize },
    { activity_id( "ACT_DIG_CHANNEL" ), &dig_channel_activity_actor::deserialize },
    { activity_id( "ACT_DISASSEMBLE" ), &disassemble_activity_actor::deserialize },
    { activity_id( "ACT_DROP" ), &drop_activity_actor::deserialize },
    { activity_id( "ACT_GUNMOD_REMOVE" ), &gunmod_remove_activity_actor::deserialize },
    { activity_id( "ACT_HACKING" ), &hacking_activity_actor::deserialize },
    { activity_id( "ACT_HAIRCUT" ), &haircut_activity_actor::deserialize },
    { activity_id( "ACT_HOTWIRE_CAR" ), &hotwire_car_activity_actor::deserialize },
    { activity_id( "ACT_INSERT_ITEM" ), &insert_item_activity_actor::deserialize },
    { activity_id( "ACT_LOCKPICK" ), &lockpick_activity_actor::deserialize },
    { activity_id( "ACT_MEDITATE" ), &meditate_activity_actor::deserialize },
    { activity_id( "ACT_MIGRATION_CANCEL" ), &migration_cancel_activity_actor::deserialize },
    { activity_id( "ACT_MILK" ), &milk_activity_actor::deserialize },
    { activity_id( "ACT_MOVE_ITEMS" ), &move_items_activity_actor::deserialize },
    { activity_id( "ACT_OPEN_GATE" ), &open_gate_activity_actor::deserialize },
    { activity_id( "ACT_PICKUP" ), &pickup_activity_actor::deserialize },
    { activity_id( "ACT_PLAY_WITH_PET" ), &play_with_pet_activity_actor::deserialize },
    { activity_id( "ACT_RELOAD" ), &reload_activity_actor::deserialize },
    { activity_id( "ACT_SHAVE" ), &shave_activity_actor::deserialize },
    { activity_id( "ACT_SHEARING" ), &shearing_activity_actor::deserialize },
    { activity_id( "ACT_STASH" ), &stash_activity_actor::deserialize },
    { activity_id( "ACT_TENT_DECONSTRUCT" ), &tent_deconstruct_activity_actor::deserialize },
    { activity_id( "ACT_TENT_PLACE" ), &tent_placement_activity_actor::deserialize },
    { activity_id( "ACT_TRY_SLEEP" ), &try_sleep_activity_actor::deserialize },
    { activity_id( "ACT_UNLOAD" ), &unload_activity_actor::deserialize },
    { activity_id( "ACT_WORKOUT_HARD" ), &workout_activity_actor::deserialize },
    { activity_id( "ACT_WORKOUT_ACTIVE" ), &workout_activity_actor::deserialize },
    { activity_id( "ACT_WORKOUT_MODERATE" ), &workout_activity_actor::deserialize },
    { activity_id( "ACT_WORKOUT_LIGHT" ), &workout_activity_actor::deserialize },
    { activity_id( "ACT_FURNITURE_MOVE" ), &move_furniture_activity_actor::deserialize },
};
} // namespace activity_actors

void serialize( const cata::clone_ptr<activity_actor> &actor, JsonOut &jsout )
{
    if( !actor ) {
        jsout.write_null();
    } else {
        jsout.start_object();

        jsout.member( "actor_type", actor->get_type() );
        jsout.member( "actor_data", *actor );

        jsout.end_object();
    }
}

void deserialize( cata::clone_ptr<activity_actor> &actor, JsonIn &jsin )
{
    if( jsin.test_null() ) {
        actor = nullptr;
    } else {
        JsonObject data = jsin.get_object();
        if( data.has_member( "actor_data" ) ) {
            activity_id actor_type;
            data.read( "actor_type", actor_type );
            auto deserializer = activity_actors::deserialize_functions.find( actor_type );
            if( deserializer != activity_actors::deserialize_functions.end() ) {
                actor = deserializer->second( *data.get_raw( "actor_data" ) );
            } else {
                debugmsg( "Failed to find activity actor deserializer for type \"%s\"", actor_type.c_str() );
                actor = nullptr;
            }
        } else {
            debugmsg( "Failed to load activity actor" );
            actor = nullptr;
        }
    }
}
