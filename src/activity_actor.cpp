#include "activity_actor.h"

#include "activity_handlers.h" // put_into_vehicle_or_drop and drop_on_map
#include "avatar.h"
#include "character.h"
#include "item.h"
#include "item_location.h"
#include "line.h"
#include "npc.h"
#include "pickup.h"
#include "point.h"
#include "translations.h"

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

void move_items_activity_actor::finish( player_activity &, Character & )
{
    // Do nothing
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

void migration_cancel_activity_actor::finish( player_activity &, Character & )
{
    // Do nothing
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

void install_software_activity_actor::do_turn( player_activity &act, Character &who )
{
    if( who.moves > 0 && install_time_left > 0 ) {
        if( !software_source || software_source->get_contained().is_null() ) {
            act.set_to_null();
            debugmsg( "lost track of software during ACT_INSTALL_SOFTWARE" );
        }
        if( !computer ) {
            act.set_to_null();
            debugmsg( "lost track of computer during ACT_INSTALL_SOFTWARE" );
        }
        // TODO deal with suspend/cancel - here?
        int progress = std::min( who.moves, install_time_left );
        who.mod_moves( -progress );
        install_time_left -= progress;

        if( install_time_left == 0 ) {
            act.set_to_null();
        }
    }
}

void install_software_activity_actor::finish( player_activity &, Character &who )
{
    // move the original copy to the computer
    // this expects the software_source to only have one contained item
    // TODO make this smarter
    item installed_software = software_source->get_contained();
    software_source->remove_item( installed_software );
    //if( software->has_flag( "DRM" ) ) {
    who.add_msg_if_player( m_good,
                           _( "You successfully installed the %1$s on your %2$s.  It securely wipes itself from the installation media." ),
                           installed_software.tname(),
                           computer->tname() );
    computer->contents.push_back( installed_software );
    /*} else {
        // it's open source, just make a copy
        // is this the right way to clone an item? O.o
        item the_copy = * software;
        who.add_msg_if_player( m_good,
                               _( "You successfully installed a copy of the %1$s on your %2$s." ),
                               the_copy.tname(),
                               ( *computer ).tname() );
        computer->contents.push_back( the_copy );
    }*/
}

void install_software_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "computer", computer );
    jsout.member( "software_source", software_source );
    jsout.member( "install_time_left", install_time_left );

    jsout.end_object();
}

std::unique_ptr<activity_actor> install_software_activity_actor::deserialize( JsonIn &jsin )
{
    install_software_activity_actor actor( item_location::nowhere, item_location::nowhere, 0 );

    JsonObject data = jsin.get_object();
    data.read( "computer", actor.computer );
    data.read( "software_source", actor.software_source );
    data.read( "install_time_left", actor.install_time_left );

    return actor.clone();
}

namespace activity_actors
{

const std::unordered_map<activity_id, std::unique_ptr<activity_actor>( * )( JsonIn & )>
deserialize_functions = {
    { activity_id( "ACT_MIGRATION_CANCEL" ), &migration_cancel_activity_actor::deserialize },
    { activity_id( "ACT_MOVE_ITEMS" ), &move_items_activity_actor::deserialize },
    { activity_id( "ACT_INSTALL_SOFTWARE" ), &install_software_activity_actor::deserialize }
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
