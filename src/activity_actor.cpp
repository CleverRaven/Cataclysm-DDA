#include "activity_actor.h"

#include <list>
#include <utility>

#include "activity_handlers.h" // put_into_vehicle_or_drop and drop_on_map
#include "avatar.h"
#include "character.h"
#include "computer_session.h"
#include "debug.h"
#include "enums.h"
#include "game.h"
#include "iexamine.h"
#include "item.h"
#include "item_location.h"
#include "json.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "npc.h"
#include "output.h"
#include "pickup.h"
#include "player_activity.h"
#include "point.h"
#include "timed_event.h"
#include "uistate.h"

static const bionic_id bio_fingerhack( "bio_fingerhack" );

static const skill_id skill_computer( "computer" );

static const trait_id trait_ILLITERATE( "ILLITERATE" );

void hacking_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = to_moves<int>( 5_minutes );
    act.moves_left = to_moves<int>( 5_minutes );
}

enum hack_result {
    HACK_UNABLE,
    HACK_FAIL,
    HACK_NOTHING,
    HACK_SUCCESS
};

enum hack_type {
    HACK_SAFE,
    HACK_DOOR,
    HACK_GAS,
    HACK_NULL
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
        return HACK_UNABLE;
    }
    const bool using_electrohack = who.has_charges( "electrohack", 25 ) &&
                                   query_yn( _( "Use electrohack?" ) );
    const bool using_fingerhack = !using_electrohack && who.has_bionic( bio_fingerhack ) &&
                                  who.get_power_level() > 24_kJ && query_yn( _( "Use fingerhack?" ) );

    if( !( using_electrohack || using_fingerhack ) ) {
        return HACK_UNABLE;
    }

    // TODO: Remove this once player -> Character migration is complete
    {
        player *p = dynamic_cast<player *>( &who );
        p->practice( skill_computer, 20 );
    }
    if( using_fingerhack ) {
        who.mod_power_level( -25_kJ );
    } else {
        who.use_charges( "electrohack", 25 );
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
            who.use_charges( "electrohack", 25 );
        }

        if( success <= -5 ) {
            if( using_electrohack ) {
                who.add_msg_if_player( m_bad, _( "Your electrohack is ruined!" ) );
                who.use_amount( "electrohack", 1 );
            } else {
                who.add_msg_if_player( m_bad, _( "Your power is drained!" ) );
                who.mod_power_level( units::from_kilojoule( -rng( 25,
                                     units::to_kilojoule( who.get_power_level() ) ) ) );
            }
        }
        return HACK_FAIL;
    } else if( success < 6 ) {
        return HACK_NOTHING;
    } else {
        return HACK_SUCCESS;
    }
}

static hack_type get_hack_type( tripoint examp )
{
    hack_type type = HACK_NULL;
    const furn_t &xfurn_t = g->m.furn( examp ).obj();
    const ter_t &xter_t = g->m.ter( examp ).obj();
    if( xter_t.examine == &iexamine::pay_gas || xfurn_t.examine == &iexamine::pay_gas ) {
        type = HACK_GAS;
    } else if( xter_t.examine == &iexamine::cardreader || xfurn_t.examine == &iexamine::cardreader ) {
        type = HACK_DOOR;
    } else if( xter_t.examine == &iexamine::gunsafe_el || xfurn_t.examine == &iexamine::gunsafe_el ) {
        type = HACK_SAFE;
    }
    return type;
}

void hacking_activity_actor::finish( player_activity &act, Character &who )
{
    tripoint examp = act.placement;
    hack_type type = get_hack_type( examp );
    switch( hack_attempt( who ) ) {
        case HACK_UNABLE:
            who.add_msg_if_player( _( "You cannot hack this." ) );
            break;
        case HACK_FAIL:
            // currently all things that can be hacked have equivalent alarm failure states.
            // this may not always be the case with new hackable things.
            g->events().send<event_type::triggers_alarm>( who.getID() );
            sounds::sound( who.pos(), 60, sounds::sound_t::music, _( "an alarm sound!" ), true, "environment",
                           "alarm" );
            if( examp.z > 0 && !g->timed_events.queued( TIMED_EVENT_WANTED ) ) {
                g->timed_events.add( TIMED_EVENT_WANTED, calendar::turn + 30_minutes, 0,
                                     who.global_sm_location() );
            }
            break;
        case HACK_NOTHING:
            who.add_msg_if_player( _( "You fail the hack, but no alarms are triggered." ) );
            break;
        case HACK_SUCCESS:
            if( type == HACK_GAS ) {
                int tankGasUnits;
                const cata::optional<tripoint> pTank_ = iexamine::getNearFilledGasTank( examp, tankGasUnits );
                if( !pTank_ ) {
                    break;
                }
                const tripoint pTank = *pTank_;
                const cata::optional<tripoint> pGasPump = iexamine::getGasPumpByNumber( examp,
                        uistate.ags_pay_gas_selected_pump );
                if( pGasPump && iexamine::toPumpFuel( pTank, *pGasPump, tankGasUnits ) ) {
                    who.add_msg_if_player( _( "You hack the terminal and route all available fuel to your pump!" ) );
                    sounds::sound( examp, 6, sounds::sound_t::activity,
                                   _( "Glug Glug Glug Glug Glug Glug Glug Glug Glug" ), true, "tool", "gaspump" );
                } else {
                    who.add_msg_if_player( _( "Nothing happens." ) );
                }
            } else if( type == HACK_SAFE ) {
                who.add_msg_if_player( m_good, _( "The door on the safe swings open." ) );
                g->m.furn_set( examp, furn_str_id( "f_safe_o" ) );
            } else if( type == HACK_DOOR ) {
                who.add_msg_if_player( _( "You activate the panel!" ) );
                who.add_msg_if_player( m_good, _( "The nearby doors unlock." ) );
                g->m.ter_set( examp, t_card_reader_broken );
                for( const tripoint &tmp : g->m.points_in_radius( ( examp ), 3 ) ) {
                    if( g->m.ter( tmp ) == t_door_metal_locked ) {
                        g->m.ter_set( tmp, t_door_metal_c );
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
        if( !newit.made_of_from_type( LIQUID ) ) {
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

namespace activity_actors
{

// Please keep this alphabetically sorted
const std::unordered_map<activity_id, std::unique_ptr<activity_actor>( * )( JsonIn & )>
deserialize_functions = {
    { activity_id( "ACT_HACKING" ), &hacking_activity_actor::deserialize },
    { activity_id( "ACT_MIGRATION_CANCEL" ), &migration_cancel_activity_actor::deserialize },
    { activity_id( "ACT_MOVE_ITEMS" ), &move_items_activity_actor::deserialize },
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
