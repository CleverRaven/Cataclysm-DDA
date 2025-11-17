#include "item_transformation.h"

#include <algorithm>

#include "character.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "item.h"
#include "item_group.h"
#include "item_location.h"
#include "itype.h"
#include "pocket_type.h"
#include "ret_val.h"
#include "rng.h"

static const flag_id flag_BOMB( "BOMB" );
static const flag_id flag_RADIO_ACTIVATION( "RADIO_ACTIVATION" );

void item_transformation::deserialize( const JsonObject &jo )
{
    optional( jo, false, "target", target );
    optional( jo, false, "target_group", target_group );

    if( !target.is_empty() && !target_group.is_empty() ) {
        jo.throw_error_at( "target_group", "Cannot use both target and target_group at once" );
    }

    optional( jo, false, "variant_type", variant_type, "<any>" );
    optional( jo, false, "container", container );
    optional( jo, false, "sealed", sealed, true );
    if( jo.has_member( "target_charges" ) && jo.has_member( "rand_target_charges" ) ) {
        jo.throw_error_at( "target_charges",
                           "Transform actor specified both fixed and random target charges" );
    }
    optional( jo, false, "target_charges", ammo_qty, -1 );
    optional( jo, false, "rand_target_charges", random_ammo_qty );
    if( random_ammo_qty.size() == 1 ) {
        jo.throw_error_at( "rand_target_charges",
                           "You must specify two or more values to choose between" );
    }
    optional( jo, false, "target_ammo", ammo_type );

    optional( jo, false, "target_timer", target_timer, 0_seconds );

    if( !ammo_type.is_empty() && !container.is_empty() ) {
        jo.throw_error_at( "target_ammo", "Transform actor specified both ammo type and container type" );
    }

    optional( jo, false, "active", active, false );
}

void item_transformation::transform( Character *carrier, item &it, bool dont_take_off ) const
{
    item obj_copy = it;

    if( container.is_empty() ) {
        if( !target_group.is_empty() ) {
            it.convert( item_group::item_from( target_group ).typeId(), carrier );
        } else {
            it.convert( target );
            it.set_itype_variant( variant_type );
        }
        if( ammo_qty >= 0 || !random_ammo_qty.empty() ) {
            int qty;
            if( !random_ammo_qty.empty() ) {
                const int index = rng( 1, random_ammo_qty.size() - 1 );
                qty = rng( random_ammo_qty[index - 1], random_ammo_qty[index] );
            } else {
                qty = ammo_qty;
            }
            if( !ammo_type.is_empty() ) {
                it.ammo_set( ammo_type, qty );
            } else if( it.is_ammo() ) {
                it.charges = qty;
            } else if( !it.ammo_current().is_null() ) {
                it.ammo_set( it.ammo_current(), qty );
            } else if( it.has_flag( flag_RADIO_ACTIVATION ) && it.has_flag( flag_BOMB ) ) {
                it.set_countdown( 1 );
            } else {
                it.set_countdown( qty );
            }
        }

        if( target_timer > 0_seconds ) {
            it.countdown_point = calendar::turn + target_timer;
        }

        it.active = active || it.has_temperature() || target_timer > 0_seconds;

    } else {
        item content = it;
        it.convert( container, carrier );
        int count = std::max( ammo_qty, 1 );

        content.set_itype_variant( variant_type );

        if( !target_group.is_empty() ) {
            content.convert( item_group::item_from( target_group ).typeId(), carrier );
        } else {
            content.convert( target, carrier );

            if( target->count_by_charges() ) {
                content.charges = count;
                count = 1;
            }
        }

        if( target_timer > 0_seconds ) {
            content.countdown_point = calendar::turn + target_timer;
        }

        content.active = active || content.has_temperature() || target_timer > 0_seconds;

        for( int i = 0; i < count; i++ ) {
            if( !it.put_in( content, pocket_type::CONTAINER ).success() ) {
                it.put_in( content, pocket_type::MIGRATION );
            }
        }

        if( sealed ) {
            it.seal();
        }
    }

    // TODO: Get rid of dont_take_off when/if the takeoff operation moves
    // the item rather than creating a copy.
    if( !dont_take_off && carrier && carrier->is_worn( it ) ) {
        if( !it.is_armor() ) {
            item_location il = item_location( *carrier, &it );
            carrier->takeoff( il );

        } else {
            carrier->calc_encumbrance();
            carrier->update_bodytemp();
            carrier->on_worn_item_transform( obj_copy, it );
        }
    }

    carrier->clear_inventory_search_cache();
}
