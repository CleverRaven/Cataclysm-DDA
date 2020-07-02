#include "activity_actor.h"

#include <list>
#include <utility>

#include "activity_handlers.h" // put_into_vehicle_or_drop and drop_on_map
#include "advanced_inv.h"
#include "avatar.h"
#include "avatar_action.h"
#include "character.h"
#include "computer_session.h"
#include "debug.h"
#include "enums.h"
#include "game.h"
#include "gates.h"
#include "iexamine.h"
#include "item.h"
#include "item_group.h"
#include "item_location.h"
#include "json.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "morale_types.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "pickup.h"
#include "player.h"
#include "player_activity.h"
#include "point.h"
#include "ranged.h"
#include "timed_event.h"
#include "uistate.h"
#include "vehicle.h"
#include "vpart_position.h"

static const bionic_id bio_fingerhack( "bio_fingerhack" );

static const efftype_id effect_sleep( "sleep" );

static const itype_id itype_bone_human( "bone_human" );
static const itype_id itype_electrohack( "electrohack" );

static const skill_id skill_computer( "computer" );
static const skill_id skill_lockpick( "lockpick" );
static const skill_id skill_mechanics( "mechanics" );

static const trait_id trait_ILLITERATE( "ILLITERATE" );

static const std::string flag_PERFECT_LOCKPICK( "PERFECT_LOCKPICK" );
static const std::string flag_RELOAD_AND_SHOOT( "RELOAD_AND_SHOOT" );

static const mtype_id mon_zombie( "mon_zombie" );
static const mtype_id mon_zombie_fat( "mon_zombie_fat" );
static const mtype_id mon_zombie_rot( "mon_zombie_rot" );
static const mtype_id mon_skeleton( "mon_skeleton" );
static const mtype_id mon_zombie_crawler( "mon_zombie_crawler" );

static const quality_id qual_LOCKPICK( "LOCKPICK" );

static const activity_id ACT_EAT_MENU( "ACT_EAT_MENU" );

template<>
struct enum_traits<aim_activity_actor::WeaponSource> {
    static constexpr aim_activity_actor::WeaponSource last =
        aim_activity_actor::WeaponSource::NumWeaponSources;
};

namespace io
{
using WS = aim_activity_actor::WeaponSource;

template<>
std::string enum_to_string<WS>( WS data )
{
    switch( data ) {
            // *INDENT-OFF*
        case WS::Wielded: return "Wielded";
        case WS::Bionic: return "Bionic";
        case WS::Mutation: return "Mutation";
            // *INDENT-ON*
        case WS::NumWeaponSources:
            break;
    }
    debugmsg( "Invalid weapon source" );
    abort();
}
} // namespace io

aim_activity_actor::aim_activity_actor()
{
    initial_view_offset = g->u.view_offset;
}

aim_activity_actor aim_activity_actor::use_wielded()
{
    return aim_activity_actor();
}

aim_activity_actor aim_activity_actor::use_bionic( const item &fake_gun,
        const units::energy &cost_per_shot )
{
    aim_activity_actor act = aim_activity_actor();
    act.weapon_source = WeaponSource::Bionic;
    act.bp_cost_per_shot = cost_per_shot;
    act.fake_weapon = make_shared_fast<item>( fake_gun );
    return act;
}

aim_activity_actor aim_activity_actor::use_mutation( const item &fake_gun )
{
    aim_activity_actor act = aim_activity_actor();
    act.weapon_source = WeaponSource::Mutation;
    act.fake_weapon = make_shared_fast<item>( fake_gun );
    return act;
}

void aim_activity_actor::start( player_activity &act, Character &/*who*/ )
{
    // Time spent on aiming is determined on the go by the player
    act.moves_total = 1;
    act.moves_left = 1;
    act.interruptable_with_kb = false;
}

void aim_activity_actor::do_turn( player_activity &act, Character &who )
{
    if( aborted || finished ) {
        // A shortcut that allows terminating this activity by setting 'aborted' or 'finished'
        act.moves_left = 0;
        return;
    }
    if( !who.is_avatar() ) {
        debugmsg( "ACT_AIM not implemented for NPCs" );
        aborted = true;
        return;
    }
    avatar &you = g->u;

    item *weapon = get_weapon();
    if( !weapon || !avatar_action::can_fire_weapon( you, get_map(), *weapon ) ) {
        aborted = true;
        return;
    }

    gun_mode gun = weapon->gun_current_mode();
    if( first_turn && gun->has_flag( flag_RELOAD_AND_SHOOT ) && !gun->ammo_remaining() ) {
        if( !load_RAS_weapon() ) {
            aborted = true;
            return;
        }
    }

    g->temp_exit_fullscreen();
    target_handler::trajectory trajectory = target_handler::mode_fire( you, *this );
    g->reenter_fullscreen();

    if( !aborted ) {
        if( !trajectory.empty() ) {
            finished = true;
            fin_trajectory = trajectory;
        }
        // If aborting on the first turn, keep 'first_turn' as 'true'.
        // This allows refunding moves spent on unloading RELOAD_AND_SHOOT weapons
        // to simulate avatar not loading them in the first place
        first_turn = false;
    }
}

void aim_activity_actor::finish( player_activity &act, Character &who )
{
    act.set_to_null();
    restore_view();
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
    item *weapon = get_weapon();
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

    jsout.member( "weapon_source", weapon_source );
    if( weapon_source == WeaponSource::Bionic || weapon_source == WeaponSource::Mutation ) {
        jsout.member( "fake_weapon", *fake_weapon );
    }
    jsout.member( "bp_cost_per_shot", bp_cost_per_shot );
    jsout.member( "first_turn", first_turn );
    jsout.member( "action", action );
    jsout.member( "snap_to_target", snap_to_target );
    jsout.member( "shifting_view", shifting_view );
    jsout.member( "initial_view_offset", initial_view_offset );

    jsout.end_object();
}

std::unique_ptr<activity_actor> aim_activity_actor::deserialize( JsonIn &jsin )
{
    aim_activity_actor actor = aim_activity_actor();

    JsonObject data = jsin.get_object();

    data.read( "weapon_source", actor.weapon_source );
    if( actor.weapon_source == WeaponSource::Bionic || actor.weapon_source == WeaponSource::Mutation ) {
        actor.fake_weapon = make_shared_fast<item>();
        data.read( "fake_weapon", *actor.fake_weapon );
    }
    data.read( "bp_cost_per_shot", actor.bp_cost_per_shot );
    data.read( "first_turn", actor.first_turn );
    data.read( "action", actor.action );
    data.read( "snap_to_target", actor.snap_to_target );
    data.read( "shifting_view", actor.shifting_view );
    data.read( "initial_view_offset", actor.initial_view_offset );

    return actor.clone();
}

item *aim_activity_actor::get_weapon()
{
    switch( weapon_source ) {
        case WeaponSource::Wielded:
            // Check for lost gun (e.g. yanked by zombie technician)
            // TODO: check that this is the same gun that was used to start aiming
            return g->u.weapon.is_null() ? nullptr : &g->u.weapon;
        case WeaponSource::Bionic:
        case WeaponSource::Mutation:
            // TODO: check if the player lost relevant bionic/mutation
            return fake_weapon.get();
        default:
            debugmsg( "Invalid weapon source value" );
            return nullptr;
    }
}

void aim_activity_actor::restore_view()
{
    bool changed_z = g->u.view_offset.z != initial_view_offset.z;
    g->u.view_offset = initial_view_offset;
    if( changed_z ) {
        get_map().invalidate_map_cache( g->u.view_offset.z );
        g->invalidate_main_ui_adaptor();
    }
}

bool aim_activity_actor::load_RAS_weapon()
{
    // TODO: use activity for fetching ammo and loading weapon
    player &you = g->u;
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
    avatar &you = g->u;
    item *weapon = get_weapon();
    if( !weapon ) {
        return;
    }

    gun_mode gun = weapon->gun_current_mode();
    if( gun->has_flag( flag_RELOAD_AND_SHOOT ) ) {
        int moves_before_unload = you.moves;

        // Note: this code works only for avatar
        item_location loc = item_location( you, gun.target );
        g->unload( loc );

        // Give back time for unloading as essentially nothing has been done.
        if( first_turn ) {
            you.moves = moves_before_unload;
        }
    }
}

void dig_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves_total;
    act.moves_left = moves_total;
}

void dig_activity_actor::do_turn( player_activity &, Character & )
{
    sfx::play_activity_sound( "tool", "shovel", sfx::get_heard_volume( location ) );
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a shovel digging a pit at work!
        sounds::sound( location, 10, sounds::sound_t::activity, _( "hsh!" ) );
    }
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
        std::vector<item *> dropped = here.place_items( "allclothes", 50, location, location, false,
                                      calendar::turn );
        here.place_items( "grave", 25, location, location, false, calendar::turn );
        here.place_items( "jewelry_front", 20, location, location, false, calendar::turn );
        for( item * const &it : dropped ) {
            if( it->is_armor() ) {
                it->item_tags.insert( "FILTHY" );
                it->set_damage( rng( 1, it->max_damage() - 1 ) );
            }
        }
        g->events().send<event_type::exhumes_grave>( who.getID() );
    }

    here.ter_set( location, ter_id( result_terrain ) );

    for( int i = 0; i < byproducts_count; i++ ) {
        here.spawn_items( byproducts_location, item_group::items_from( byproducts_item_group,
                          calendar::turn ) );
    }

    const int helpersize = g->u.get_num_crafting_helpers( 3 );
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

void dig_channel_activity_actor::do_turn( player_activity &, Character & )
{
    sfx::play_activity_sound( "tool", "shovel", sfx::get_heard_volume( location ) );
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a shovel digging a pit at work!
        sounds::sound( location, 10, sounds::sound_t::activity, _( "hsh!" ) );
    }
}

void dig_channel_activity_actor::finish( player_activity &act, Character &who )
{
    map &here = get_map();
    here.ter_set( location, ter_id( result_terrain ) );

    for( int i = 0; i < byproducts_count; i++ ) {
        here.spawn_items( byproducts_location, item_group::items_from( byproducts_item_group,
                          calendar::turn ) );
    }

    const int helpersize = g->u.get_num_crafting_helpers( 3 );
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
    if( who.has_trait( trait_ILLITERATE ) ) {
        return hack_result::UNABLE;
    }
    const bool using_electrohack = who.has_charges( itype_electrohack, 25 ) &&
                                   query_yn( _( "Use electrohack?" ) );
    const bool using_fingerhack = !using_electrohack && who.has_bionic( bio_fingerhack ) &&
                                  who.get_power_level() > 24_kJ && query_yn( _( "Use fingerhack?" ) );

    if( !( using_electrohack || using_fingerhack ) ) {
        return hack_result::UNABLE;
    }

    // TODO: Remove this once player -> Character migration is complete
    {
        player *p = dynamic_cast<player *>( &who );
        p->practice( skill_computer, 20 );
    }
    if( using_fingerhack ) {
        who.mod_power_level( -25_kJ );
    } else {
        who.use_charges( itype_electrohack, 25 );
    }

    // only skilled supergenius never cause short circuits, but the odds are low for people
    // with moderate skills
    const int hack_stddev = 5;
    int success = std::ceil( normal_roll( hack_level( who ), hack_stddev ) );
    if( success < 0 ) {
        who.add_msg_if_player( _( "You cause a short circuit!" ) );
        if( using_fingerhack ) {
            who.mod_power_level( -25_kJ );
        } else {
            who.use_charges( itype_electrohack, 25 );
        }

        if( success <= -5 ) {
            if( using_electrohack ) {
                who.add_msg_if_player( m_bad, _( "Your electrohack is ruined!" ) );
                who.use_amount( itype_electrohack, 1 );
            } else {
                who.add_msg_if_player( m_bad, _( "Your power is drained!" ) );
                who.mod_power_level( units::from_kilojoule( -rng( 25,
                                     units::to_kilojoule( who.get_power_level() ) ) ) );
            }
        }
        return hack_result::FAIL;
    } else if( success < 6 ) {
        return hack_result::NOTHING;
    } else {
        return hack_result::SUCCESS;
    }
}

static hack_type get_hack_type( tripoint examp )
{
    hack_type type = hack_type::NONE;
    map &here = get_map();
    const furn_t &xfurn_t = here.furn( examp ).obj();
    const ter_t &xter_t = here.ter( examp ).obj();
    if( xter_t.examine == &iexamine::pay_gas || xfurn_t.examine == &iexamine::pay_gas ) {
        type = hack_type::GAS;
    } else if( xter_t.examine == &iexamine::cardreader || xfurn_t.examine == &iexamine::cardreader ) {
        type = hack_type::DOOR;
    } else if( xter_t.examine == &iexamine::gunsafe_el || xfurn_t.examine == &iexamine::gunsafe_el ) {
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
            g->events().send<event_type::triggers_alarm>( who.getID() );
            sounds::sound( who.pos(), 60, sounds::sound_t::music, _( "an alarm sound!" ), true, "environment",
                           "alarm" );
            if( examp.z > 0 && !g->timed_events.queued( timed_event_type::WANTED ) ) {
                g->timed_events.add( timed_event_type::WANTED, calendar::turn + 30_minutes, 0,
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

void hacking_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.write_null();
}

std::unique_ptr<activity_actor> hacking_activity_actor::deserialize( JsonIn & )
{
    return hacking_activity_actor().clone();
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

        // Don't need to make a copy here since movement can't be canceled
        item &leftovers = *target;
        // Make a copy to be put in the destination location
        item newit = leftovers;

        // Handle charges, quantity == 0 means move all
        if( quantity != 0 && newit.count_by_charges() ) {
            newit.charges = std::min( newit.charges, quantity );
            leftovers.charges -= quantity;
        } else {
            leftovers.charges = 0;
        }

        // Check that we can pick it up.
        if( !newit.made_of_from_type( phase_id::LIQUID ) ) {
            // This is for hauling across zlevels, remove when going up and down stairs
            // is no longer teleportation
            if( newit.is_owned_by( who, true ) ) {
                newit.set_owner( who );
            } else {
                continue;
            }
            const tripoint src = target.position();
            const int distance = src.z == dest.z ? std::max( rl_dist( src, dest ), 1 ) : 1;
            who.mod_moves( -Pickup::cost_to_move_item( who, newit ) * distance );
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

void pickup_activity_actor::do_turn( player_activity &, Character &who )
{
    // If we don't have target items bail out
    if( target_items.empty() ) {
        who.cancel_activity();
        return;
    }

    // If the player moves while picking up (i.e.: in a moving vehicle) cancel
    // the activity, only populate starting_pos when grabbing from the ground
    if( starting_pos && *starting_pos != who.pos() ) {
        who.cancel_activity();
        who.add_msg_if_player( _( "Moving canceled auto-pickup." ) );
        return;
    }

    // Auto_resume implies autopickup.
    const bool autopickup = who.activity.auto_resume;

    // False indicates that the player canceled pickup when met with some prompt
    const bool keep_going = Pickup::do_pickup( target_items, quantities, autopickup );

    // If there are items left we ran out of moves, so continue the activity
    // Otherwise, we are done.
    if( !keep_going || target_items.empty() ) {
        who.cancel_activity();

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

void lockpick_activity_actor::start( player_activity &act, Character & )
{
    act.moves_left = moves_total;
    act.moves_total = moves_total;
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
    std::string open_message;
    if( ter_type == t_chaingate_l ) {
        new_ter_type = t_chaingate_c;
        open_message = _( "With a satisfying click, the chain-link gate opens." );
    } else if( ter_type == t_door_locked || ter_type == t_door_locked_alarm ||
               ter_type == t_door_locked_interior ) {
        new_ter_type = t_door_c;
        open_message = _( "With a satisfying click, the lock on the door opens." );
    } else if( ter_type == t_door_locked_peep ) {
        new_ter_type = t_door_c_peep;
        open_message = _( "With a satisfying click, the lock on the door opens." );
    } else if( ter_type == t_door_metal_pickable ) {
        new_ter_type = t_door_metal_c;
        open_message = _( "With a satisfying click, the lock on the door opens." );
    } else if( ter_type == t_door_bar_locked ) {
        new_ter_type = t_door_bar_o;
        //Bar doors auto-open (and lock if closed again) so show a different message)
        open_message = _( "The door swings open…" );
    } else if( furn_type == f_gunsafe_ml ) {
        new_furn_type = f_safe_o;
        open_message = _( "With a satisfying click, the lock on the door opens." );
    }

    bool perfect = it->has_flag( flag_PERFECT_LOCKPICK );
    bool destroy = false;

    /** @EFFECT_DEX improves chances of successfully picking door lock, reduces chances of bad outcomes */
    /** @EFFECT_MECHANICS improves chances of successfully picking door lock, reduces chances of bad outcomes */
    /** @EFFECT_LOCKPICK greatly improves chances of successfully picking door lock, reduces chances of bad outcomes */
    int pick_roll = std::pow( 1.5, who.get_skill_level( skill_lockpick ) ) *
                    ( std::pow( 1.3, who.get_skill_level( skill_mechanics ) ) +
                      it->get_quality( qual_LOCKPICK ) - it->damage() / 2000.0 ) +
                    who.dex_cur / 4.0;
    int lock_roll = rng( 1, 120 );
    int xp_gain = 0;
    if( perfect || ( pick_roll >= lock_roll ) ) {
        xp_gain += lock_roll;
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
        if( !perfect ) {
            // You don't gain much skill since the item does all the hard work for you
            xp_gain += std::pow( 2, you->get_skill_level( skill_lockpick ) ) + 1;
        }
        you->practice( skill_lockpick, xp_gain );
    }

    if( !perfect && ter_type == t_door_locked_alarm && ( lock_roll + dice( 1, 30 ) ) > pick_roll ) {
        sounds::sound( who.pos(), 40, sounds::sound_t::alarm, _( "an alarm sound!" ), true, "environment",
                       "alarm" );
        if( !g->timed_events.queued( timed_event_type::WANTED ) ) {
            g->timed_events.add( timed_event_type::WANTED, calendar::turn + 30_minutes, 0,
                                 who.global_sm_location() );
        }
    }

    if( destroy && lockpick.has_value() ) {
        ( *lockpick ).remove_item();
    }
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
    if( consume_location ) {
        const auto ret = g->u.will_eat( *consume_location, true );
        if( !ret.success() ) {
            consume_menu_selections = std::vector<int>();
            return;
        } else {
            force = true;
        }
        moves = to_moves<int>( guy.get_consume_time( *consume_location ) );
    } else if( !consume_item.is_null() ) {
        const auto ret = g->u.will_eat( consume_item, true );
        if( !ret.success() ) {
            consume_menu_selections = std::vector<int>();
            return;
        } else {
            force = true;
        }
        moves = to_moves<int>( guy.get_consume_time( consume_item ) );
    } else {
        debugmsg( "Item/location to be consumed should not be null." );
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

    if( consume_location ) {
        g->u.consume( consume_location, force );
    } else if( !consume_item.is_null() ) {
        g->u.consume( consume_item, force );
    } else {
        debugmsg( "Item location/name to be consumed should not be null." );
    }
    if( g->u.get_value( "THIEF_MODE_KEEP" ) != "YES" ) {
        g->u.set_value( "THIEF_MODE", "THIEF_ASK" );
    }
    //setting act to null clears these so back them up
    std::vector<int> temp_selections = consume_menu_selections;
    act.set_to_null();
    if( !temp_selections.empty() ) {
        g->u.assign_activity( ACT_EAT_MENU );
        g->u.activity.values = temp_selections;
    }
}

void consume_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "consume_location", consume_location );
    jsout.member( "consume_item", consume_item );
    jsout.member( "consume_menu_selections", consume_menu_selections );
    jsout.member( "force", force );

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
    data.read( "force", actor.force );

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
    // free training requires all limbs intact, but specialized workout machines
    // train upper or lower parts of body only and may permit workout with
    // broken limbs as long as they are not involved by the machine
    bool hand_equipment = g->m.has_flag_furn( "WORKOUT_ARMS", location );
    bool leg_equipment = g->m.has_flag_furn( "WORKOUT_LEGS", location );
    static const bodypart_id arm_l = bodypart_id( "arm_l" );
    static const bodypart_id arm_r = bodypart_id( "arm_r" );
    static const bodypart_id leg_l = bodypart_id( "leg_l" );
    static const bodypart_id leg_r = bodypart_id( "leg_r" );
    if( hand_equipment && ( ( who.is_limb_broken( arm_l ) ) ||
                            who.is_limb_broken( arm_r ) ) ) {
        who.add_msg_if_player( _( "You cannot train here with a broken arm." ) );
        act_id = activity_id::NULL_ID();
        act.set_to_null();
        return;
    }
    if( leg_equipment && ( ( who.is_limb_broken( leg_l ) ) ||
                           who.is_limb_broken( leg_r ) ) ) {
        who.add_msg_if_player( _( "You cannot train here with a broken leg." ) );
        act_id = activity_id::NULL_ID();
        act.set_to_null();
        return;
    }
    if( !hand_equipment && !leg_equipment &&
        ( who.is_limb_broken( arm_l ) ||
          who.is_limb_broken( arm_r ) ||
          who.is_limb_broken( leg_l ) ||
          who.is_limb_broken( leg_r ) ) ) {
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
    workout_query.addentry_desc( 1, true, 'l', _( "Light" ),
                                 _( "Light excercise comparable in intensity to walking, but more focused and methodical." ) );
    workout_query.addentry_desc( 2, true, 'm', _( "Moderate" ),
                                 _( "Moderate excercise without excessive exertion, but with enough effort to break a sweat." ) );
    workout_query.addentry_desc( 3, true, 'a', _( "Active" ),
                                 _( "Active excercise with full involvement.  Strenuous, but in a controlled manner." ) );
    workout_query.addentry_desc( 4, true, 'h', _( "High" ),
                                 _( "High intensity excercise with maximum effort and full power.  Exhausting in the long run." ) );
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
            //~ heavy breathing when excercising
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
            who.add_msg_if_player( m_debug, who.activity_level_str() );
            who.add_msg_if_player( m_debug, act.id().c_str() );
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
            break;
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

namespace activity_actors
{

// Please keep this alphabetically sorted
const std::unordered_map<activity_id, std::unique_ptr<activity_actor>( * )( JsonIn & )>
deserialize_functions = {
    { activity_id( "ACT_AIM" ), &aim_activity_actor::deserialize },
    { activity_id( "ACT_CONSUME" ), &consume_activity_actor::deserialize },
    { activity_id( "ACT_DIG" ), &dig_activity_actor::deserialize },
    { activity_id( "ACT_DIG_CHANNEL" ), &dig_channel_activity_actor::deserialize },
    { activity_id( "ACT_HACKING" ), &hacking_activity_actor::deserialize },
    { activity_id( "ACT_HOTWIRE_CAR" ), &hotwire_car_activity_actor::deserialize },
    { activity_id( "ACT_LOCKPICK" ), &lockpick_activity_actor::deserialize },
    { activity_id( "ACT_MIGRATION_CANCEL" ), &migration_cancel_activity_actor::deserialize },
    { activity_id( "ACT_MOVE_ITEMS" ), &move_items_activity_actor::deserialize },
    { activity_id( "ACT_OPEN_GATE" ), &open_gate_activity_actor::deserialize },
    { activity_id( "ACT_PICKUP" ), &pickup_activity_actor::deserialize },
    { activity_id( "ACT_TRY_SLEEP" ), &try_sleep_activity_actor::deserialize },
    { activity_id( "ACT_WORKOUT_HARD" ), &workout_activity_actor::deserialize },
    { activity_id( "ACT_WORKOUT_ACTIVE" ), &workout_activity_actor::deserialize },
    { activity_id( "ACT_WORKOUT_MODERATE" ), &workout_activity_actor::deserialize },
    { activity_id( "ACT_WORKOUT_LIGHT" ), &workout_activity_actor::deserialize },
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
