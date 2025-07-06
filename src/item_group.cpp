#include "item_group.h"

#include <algorithm>
#include <functional>
#include <iterator>
#include <set>
#include <string>
#include <unordered_map>

#include "calendar.h"
#include "cata_assert.h"
#include "cata_scope_helpers.h"
#include "cata_utility.h"
#include "debug.h"
#include "enum_traits.h"
#include "enums.h"
#include "flag.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "item.h"
#include "item_components.h"
#include "item_contents.h"
#include "item_factory.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "make_static.h"
#include "options.h"
#include "pocket_type.h"
#include "relic.h"
#include "ret_val.h"
#include "rng.h"
#include "string_formatter.h"
#include "type_id.h"
#include "units.h"

static const fault_id fault_gun_dirt( "fault_gun_dirt" );
static const fault_id fault_gun_unlubricated( "fault_gun_unlubricated" );

std::size_t Item_spawn_data::create( ItemList &list,
                                     const time_point &birthday, spawn_flags flags ) const
{
    RecursionList rec;
    return create( list, birthday, rec, flags );
}

item Item_spawn_data::create_single( const time_point &birthday ) const
{
    RecursionList rec;
    return create_single( birthday, rec );
}

void Item_spawn_data::check_consistency( bool actually_spawn ) const
{
    if( on_overflow != overflow_behaviour::none && !container_item ) {
        debugmsg( "item group %s specifies overflow behaviour but not container", context() );
    }
    // Spawn ourselves with all possible items being definitely spawned, so as
    // to verify e.g. that if a container item was specified it can actually
    // contain what was wanted.
    if( actually_spawn ) {
        ItemList dummy_list;
        dummy_list.reserve( 20 );
        create( dummy_list, calendar::turn_zero, spawn_flags::maximized );
    }
}

void Item_spawn_data::relic_generator::load( const JsonObject &jo )
{
    mandatory( jo, was_loaded, "rules", rules );
    mandatory( jo, was_loaded, "procgen_id", id );
}

relic Item_spawn_data::relic_generator::generate_relic( const itype_id &it_id ) const
{
    return id->generate( rules, it_id );
}

namespace io
{
template<>
std::string enum_to_string<Item_spawn_data::overflow_behaviour>(
    Item_spawn_data::overflow_behaviour data )
{
    switch( data ) {
        // *INDENT-OFF*
        case Item_spawn_data::overflow_behaviour::none: return "none";
        case Item_spawn_data::overflow_behaviour::spill: return "spill";
        case Item_spawn_data::overflow_behaviour::discard: return "discard";
        // *INDENT-ON*
        case Item_spawn_data::overflow_behaviour::last:
            break;
    }
    cata_fatal( "Invalid overflow_behaviour" );
}
} // namespace io

static pocket_type guess_pocket_for( const item &container, const item &payload )
{
    if( container.is_estorage() && payload.is_estorable() ) {
        return pocket_type::E_FILE_STORAGE;
    }
    if( ( container.is_gun() && payload.is_gunmod() ) || ( container.is_tool() &&
            payload.is_toolmod() ) ) {
        return pocket_type::MOD;
    }
    if( ( container.is_gun() || container.is_tool() ) && payload.is_magazine() ) {
        return pocket_type::MAGAZINE_WELL;
    } else if( container.is_magazine() && payload.is_ammo() ) {
        return pocket_type::MAGAZINE;
    }
    return pocket_type::CONTAINER;
}

static void put_into_container(
    Item_spawn_data::ItemList &items, std::size_t num_items,
    const std::optional<itype_id> &container_type, const std::optional<std::string> &container_variant,
    const bool sealed,
    time_point birthday, Item_spawn_data::overflow_behaviour on_overflow,
    const std::string &context )
{
    if( !container_type ) {
        return;
    }

    // Randomly permute the list of items so that when some don't fit it's
    // not always the ones at the end which are rejected.
    cata_assert( items.size() >= num_items );
    std::shuffle( items.end() - num_items, items.end(), rng_get_engine() );

    item ctr( *container_type, birthday );
    if( container_variant ) {
        ctr.set_itype_variant( *container_variant );
    }
    Item_spawn_data::ItemList excess;
    for( auto it = items.end() - num_items; it != items.end(); ++it ) {
        ret_val<void> ret = ctr.can_contain_directly( *it );
        if( ret.success() ) {
            const pocket_type pk_type = guess_pocket_for( ctr, *it );
            ctr.put_in( *it, pk_type );
        } else if( ctr.is_corpse() ) {
            const pocket_type pk_type = guess_pocket_for( ctr, *it );
            ctr.force_insert_item( *it, pk_type );
        } else {
            switch( on_overflow ) {
                case Item_spawn_data::overflow_behaviour::none:
                    debugmsg( "item %s could not be put in container %s when spawning item group %s: %s.  "
                              "This can be resolved either by changing the container or contents "
                              "to ensure that they fit, or by specifying an overflow behaviour via "
                              "\"on_overflow\" on the item group.",
                              it->typeId().str(), container_type->str(), context, ret.str() );
                    break;
                case Item_spawn_data::overflow_behaviour::spill:
                    excess.push_back( *it );
                    break;
                case Item_spawn_data::overflow_behaviour::discard:
                    break;
                case Item_spawn_data::overflow_behaviour::last:
                    debugmsg( "Invalid overflow_behaviour" );
                    break;
            }
        }
    }
    ctr.add_automatic_whitelist();
    if( sealed ) {
        ctr.seal();
    }

    items.erase( items.end() - num_items, items.end() );
    items.reserve( items.size() + excess.size() + 1 );
    items.insert( items.end(), std::make_move_iterator( excess.begin() ),
                  std::make_move_iterator( excess.end() ) );
    items.emplace_back( std::move( ctr ) );
}

Single_item_creator::Single_item_creator( const std::string &_id, Type _type, int _probability,
        const std::string &context, holiday event )
    : Item_spawn_data( _probability, context, event )
    , id( _id )
    , type( _type )
{
}
item Single_item_creator::create_single( const time_point &birthday, RecursionList &rec ) const
{
    item tmp = create_single_without_container( birthday, rec );
    if( container_item ) {
        tmp = tmp.in_container( *container_item, tmp.count(), sealed,
                                container_item_variant.value_or( "" ) );
    }
    return tmp;
}

item Single_item_creator::create_single_without_container( const time_point &birthday,
        RecursionList &rec ) const
{
    // Check direct return conditions first.
    if( type == S_NONE ) {
        return item( itype_id::NULL_ID(), birthday );
    }
    Item_spawn_data *isd = nullptr;
    if( type == S_ITEM_GROUP ) {
        item_group_id group_id( id );
        if( std::find( rec.begin(), rec.end(), group_id ) != rec.end() ) {
            debugmsg( "recursion in item spawn list %s", id.c_str() );
            return item( itype_id::NULL_ID(), birthday );
        }
        rec.push_back( group_id );
        isd = item_controller->get_group( group_id );
        if( isd == nullptr ) {
            debugmsg( "unknown item spawn list %s", id.c_str() );
            return item( itype_id::NULL_ID(), birthday );
        }
    }

    item tmp = ( [&]() -> item {
        if( isd )
        {
            on_out_of_scope scope{
                [&]{
                    rec.erase( rec.end() - 1 );
                }} ;
            return isd->create_single( birthday, rec );
        } else
        {
            cata_assert( type == S_ITEM );
            if( id == "corpse" ) {
                return item::make_corpse( mtype_id::NULL_ID(), birthday );
            } else {
                return item( itype_id( id ), birthday );
            }
        }
    } )();

    if( one_in( 3 ) && tmp.has_flag( flag_VARSIZE ) ) {
        tmp.set_flag( flag_FIT );
    }

    if( active.has_value() ) {
        tmp.active = *active;
    }

    if( components_items ) {
        for( itype_id component_id : *components_items ) {
            if( !component_id.is_valid() ) {
                debugmsg( "Invalid components item %s in %s (could not find matching itype id)",
                          component_id.c_str(), context() );
                continue;
            }
            item component = item( component_id, calendar::turn );
            tmp.components.add( component );
        }
    }
    if( modifier ) {
        modifier->modify( tmp, "modifier for " + context() );
    } else {
        int qty = tmp.count();
        if( modifier ) {
            qty = rng( modifier->charges.first, modifier->charges.second );
        } else if( tmp.made_of_from_type( phase_id::LIQUID ) ) {
            qty = item::INFINITE_CHARGES;
        }
        // TODO: change the spawn lists to contain proper references to containers
        tmp = tmp.in_its_container( qty );
    }
    if( artifact ) {
        tmp.overwrite_relic( artifact->generate_relic( tmp.typeId() ) );
    }
    return tmp;
}

std::size_t Single_item_creator::create( ItemList &list,
        const time_point &birthday, RecursionList &rec, spawn_flags flags ) const
{
    std::size_t prev_list_size = list.size();
    int cnt = 1;
    if( modifier ) {
        auto modifier_count = modifier->count;
        if( flags & spawn_flags::maximized ) {
            cnt = modifier_count.second;
        } else {
            cnt = ( modifier_count.first == modifier_count.second ) ? modifier_count.first : rng(
                      modifier_count.first, modifier_count.second );
        }
    }
    float spawn_rate = get_option<float>( "ITEM_SPAWNRATE" );
    for( ; cnt > 0; cnt-- ) {
        if( type == S_ITEM ) {
            item itm = create_single_without_container( birthday, rec );
            if( flags & spawn_flags::use_spawn_rate && !itm.has_flag( STATIC( flag_id( "MISSION_ITEM" ) ) ) &&
                rng_float( 0, 1 ) > spawn_rate ) {
                continue;
            }
            if( !itm.is_null() ) {
                list.emplace_back( std::move( itm ) );
            }
        } else {
            item_group_id group_id( id );
            if( std::find( rec.begin(), rec.end(), group_id ) != rec.end() ) {
                debugmsg( "recursion in item spawn list %s", id.c_str() );
                return list.size() - prev_list_size;
            }
            rec.push_back( group_id );
            Item_spawn_data *isd = item_controller->get_group( group_id );
            if( isd == nullptr ) {
                debugmsg( "unknown item spawn list %s", id.c_str() );
                return list.size() - prev_list_size;
            }
            std::size_t tmp_list_size = isd->create( list, birthday, rec, flags );
            cata_assert( list.size() >= tmp_list_size );
            rec.pop_back();
            if( modifier ) {
                for( auto it = list.end() - tmp_list_size; it != list.end(); ++it ) {
                    modifier->modify( *it, "modifier for " + context() );
                }
            }
        }
    }
    if( artifact ) {
        for( auto it = list.begin() + prev_list_size; it != list.end(); ++it ) {
            it->overwrite_relic( artifact->generate_relic( it->typeId() ) );
        }
    }
    const std::size_t items_created = list.size() - prev_list_size;
    put_into_container( list, items_created, container_item, container_item_variant, sealed, birthday,
                        on_overflow, context() );
    return list.size() - prev_list_size;
}

void Single_item_creator::check_consistency( bool actually_spawn ) const
{
    if( type == S_ITEM ) {
        if( !item::type_is_defined( itype_id( id ) ) ) {
            debugmsg( "item id %s is unknown (in %s)", id, context() );
        }
        if( modifier && ( modifier->charges.first != -1 || modifier->charges.second != -1 ) ) {
            itype_id content_final( id );
            if( !modifier->ammo && !content_final->can_have_charges() && !content_final->tool &&
                !content_final->gun && !content_final->magazine ) {
                debugmsg( "itemgroup entry for spawning item %s defined charges but can't have any", id );
            }
        }
    } else if( type == S_ITEM_GROUP ) {
        if( !item_group::group_is_defined( item_group_id( id ) ) ) {
            debugmsg( "item group id %s is unknown (in %s)", id, context() );
        }
    } else if( type == S_NONE ) {
        // this is okay, it will be ignored
    } else {
        debugmsg( "Unknown type of Single_item_creator: %d", static_cast<int>( type ) );
    }
    if( modifier ) {
        modifier->check_consistency( context() );
    }
    // If this is to create a group and there's no container to restrain it, no need to actually create it,
    // since groups are processed separately.
    Item_spawn_data::check_consistency( ( type == S_ITEM_GROUP &&
                                          !container_item ) ? false : actually_spawn );
}

bool Single_item_creator::remove_item( const itype_id &itemid )
{
    if( modifier ) {
        if( modifier->remove_item( itemid ) ) {
            type = S_NONE;
            return true;
        }
    }
    if( type == S_ITEM ) {
        if( itemid.str() == id ) {
            type = S_NONE;
            return true;
        }
    }
    // This part of code is currently only used in Item_factory::finalize_item_blacklist(), not during the game.
    // Else if type == S_ITEM_GROUP, the group must have been indexed in m_template_groups,
    // and finalize_item_blacklist() will deal with it, so no need to find it and call its remove_item().
    return type == S_NONE;
}

void Single_item_creator::replace_items( const std::unordered_map<itype_id, itype_id>
        &replacements )
{
    if( modifier ) {
        modifier->replace_items( replacements );
    }
    if( type == S_ITEM ) {
        auto it = replacements.find( itype_id( id ) );
        if( it != replacements.end() ) {
            id = it->second.str();
        }
    }
    // This part of code is currently only used in Item_factory::finalize_item_blacklist(), not during the game.
    // Else if type == S_ITEM_GROUP, the group must have been indexed in m_template_groups,
    // and finalize_item_blacklist() will deal with it, so no need to find it and call its replace_items().
}

bool Single_item_creator::has_item( const itype_id &itemid ) const
{
    switch( type ) {
        case S_ITEM:
            return itemid.str() == id;
        case S_ITEM_GROUP: {
            Item_spawn_data *isd = item_controller->get_group( item_group_id( id ) );
            if( isd != nullptr ) {
                return isd->has_item( itemid );
            }
            return false;
        }
        case S_NONE:
            return false;
    }
    return false;
}

std::set<const itype *> Single_item_creator::every_item() const
{
    switch( type ) {
        case S_ITEM:
            return { item::find_type( itype_id( id ) ) };
        case S_ITEM_GROUP: {
            Item_spawn_data *isd = item_controller->get_group( item_group_id( id ) );
            if( isd != nullptr ) {
                return isd->every_item();
            }
            return {};
        }
        case S_NONE:
            return {};
    }
    // NOLINTNEXTLINE(misc-static-assert,cert-dcl03-c)
    cata_fatal( "Unexpected type" );
    return {};
}

std::map<const itype *, std::pair<int, int>> Single_item_creator::every_item_min_max() const
{
    switch( type ) {
        case S_ITEM: {
            const itype *i = item::find_type( itype_id( id ) );
            if( i->count_by_charges() ) {
                // TODO: Not technically perfect for only charge_min/charge max and for ammo/liquids but Item_modifier::modify()'s logic is gross and I'd rather not try to replicate it perfectly as is
                const int min_charges = modifier->charges.first == -1 ?
                                        i->charges_default() : modifier->charges.first;
                const int max_charges = modifier->charges.second == -1 ?
                                        i->charges_default() : modifier->charges.second;
                return { std::make_pair( i, std::make_pair( modifier->count.first * min_charges, modifier->count.second * max_charges ) ) };
            }

            // since modifier is std::optional it might not be present
            // if not - we return [0,1] if item has probability to spawn less than a hundred
            // and [1,1] otherwise
            if( modifier ) {
                return { std::make_pair( i, modifier->count ) };
            } else {
                return { std::make_pair( i, std::make_pair( probability < 100 ? 0 : 1, 1 ) ) };
            }
        }
        case S_ITEM_GROUP: {
            Item_spawn_data *isd = item_controller->get_group( item_group_id( id ) );
            if( isd != nullptr ) {
                return isd->every_item_min_max();
            }
            return {};
        }
        case S_NONE:
            return {};
    }
    // NOLINTNEXTLINE(misc-static-assert,cert-dcl03-c)
    cata_fatal( "Unexpected type" );
    return {};
}

void Single_item_creator::inherit_ammo_mag_chances( const int ammo, const int mag )
{
    if( ammo != 0 || mag != 0 ) {
        if( !modifier ) {
            modifier.emplace();
        }
        modifier->with_ammo = ammo;
        modifier->with_magazine = mag;
    }
}

Item_modifier::Item_modifier()
    : damage( 0, 0 )
    , count( 1, 1 )
      // Dirt in guns is capped unless overwritten in the itemgroup
      // most guns should not be very dirty or dirty at all
    , dirt( 0, 500 )
    , charges( -1, -1 )
    , with_ammo( 0 )
    , with_magazine( 0 )
{
}

void Item_modifier::modify( item &new_item, const std::string &context ) const
{
    if( new_item.is_null() ) {
        return;
    }

    new_item.set_damage( rng( damage.first, damage.second ) );
    new_item.rand_degradation();
    // no need for dirt if it's a bow
    if( new_item.is_gun() && !new_item.has_flag( flag_PRIMITIVE_RANGED_WEAPON ) &&
        !new_item.has_flag( flag_NON_FOULING ) ) {
        int random_dirt = rng( dirt.first, dirt.second );
        // if gun RNG is dirty, must add dirt fault to allow cleaning
        if( random_dirt > 0 ) {
            new_item.set_var( "dirt", random_dirt );
            new_item.set_fault( fault_gun_dirt );
            // chance to be unlubed, but only if it's not a laser or something
        } else if( one_in( 10 ) && !new_item.has_flag( flag_NEEDS_NO_LUBE ) ) {
            new_item.faults.emplace( fault_gun_unlubricated );
        }
    }

    new_item.set_itype_variant( variant );

    {
        // create container here from modifier or from default to get max charges later
        std::optional<item> cont;
        if( container != nullptr ) {
            cont = container->create_single( new_item.birthday() );
        } else if( new_item.type->default_container.has_value() &&
                   !new_item.type->default_container->is_null() ) {
            cont = item( *new_item.type->default_container, new_item.birthday() );
            if( new_item.type->default_container_variant.has_value() ) {
                cont->set_itype_variant( *new_item.type->default_container_variant );
            }
        }

        int max_capacity = -1;

        if( charges.first != -1 && charges.second == -1 && ( new_item.is_magazine() ||
                new_item.uses_magazine() ) ) {
            int max_ammo = 0;

            if( new_item.is_magazine() ) {
                // Get the ammo capacity of the new item itself
                max_ammo = new_item.ammo_capacity( item_controller->find_template(
                                                       new_item.ammo_default() )->ammo->type );
            } else if( !new_item.magazine_default().is_null() ) {
                // Get the capacity of the item's default magazine
                max_ammo = item_controller->find_template( new_item.magazine_default() )->magazine->capacity;
            }
            // Don't change the ammo capacity from 0 if the item isn't a magazine
            // and doesn't have a default magazine with a capacity

            if( max_ammo > 0 ) {
                max_capacity = max_ammo;
            }
        }

        if( max_capacity == -1 && cont.has_value() && !cont->is_null() &&
            ( new_item.made_of( phase_id::LIQUID ) ||
              ( !new_item.is_tool() && !new_item.is_gun() && !new_item.is_magazine() ) ) ) {
            if( new_item.type->weight == 0_gram ) {
                max_capacity = new_item.charges_per_volume( cont->get_total_capacity() );
            } else {
                max_capacity = std::min( new_item.charges_per_volume( cont->get_total_capacity() ),
                                         new_item.charges_per_weight( cont->get_total_weight_capacity() ) );
            }
        }

        const bool charges_not_set = charges.first == -1 && charges.second == -1;
        int ch = -1;
        if( !charges_not_set ) {
            int charges_min = charges.first == -1 ? 0 : charges.first;
            int charges_max = charges.second == -1 ? max_capacity : charges.second;

            if( charges_min == -1 && charges_max != -1 ) {
                charges_min = 0;
            }

            if( max_capacity != -1 && ( charges_max > max_capacity || ( charges_min != 1 &&
                                        charges_max == -1 ) ) ) {
                charges_max = max_capacity;
            }

            if( charges_min > charges_max ) {
                charges_min = charges_max;
            }

            ch = charges_min == charges_max ? charges_min : rng( charges_min,
                    charges_max );
        } else if( cont.has_value() && !cont->is_null() && new_item.made_of( phase_id::LIQUID ) ) {
            new_item.charges = std::max( 1, max_capacity );
        }

        if( ch != -1 ) {
            if( new_item.count_by_charges() || new_item.made_of( phase_id::LIQUID ) ) {
                // food, ammo
                // count_by_charges requires that charges is at least 1. It makes no sense to
                // spawn a "water (0)" item.
                new_item.charges = std::max( 1, ch );
            } else if( new_item.is_tool() ) {
                if( !new_item.magazine_current() && !new_item.magazine_default().is_null() ) {
                    item mag( new_item.magazine_default() );
                    if( !mag.ammo_default().is_null() ) {
                        mag.ammo_set( mag.ammo_default(), ch );
                    }
                    new_item.put_in( mag, pocket_type::MAGAZINE_WELL );
                } else if( new_item.is_magazine() ) {
                    new_item.ammo_set( new_item.ammo_default(), ch );
                } else if( new_item.magazine_current() ) {
                    new_item.ammo_set( new_item.magazine_current()->ammo_default(), ch );
                } else {
                    debugmsg( "in %s: tried to set ammo for %s which does not have ammo or a magazine",
                              context, new_item.typeId().str() );
                }
            } else if( new_item.type->can_have_charges() ) {
                new_item.charges = ch;
            }
        }

        if( ch > 0 && ( new_item.is_gun() || new_item.is_magazine() ) ) {
            itype_id ammo_id;
            if( ammo ) {
                ammo_id = ammo->create_single( new_item.birthday() ).typeId();
            } else if( new_item.ammo_default() ) {
                ammo_id = new_item.ammo_default();
            } else if( new_item.magazine_default() && new_item.magazine_default()->magazine->default_ammo ) {
                ammo_id = new_item.magazine_default()->magazine->default_ammo;
            }
            if( ammo_id && !ammo_id.is_empty() ) {
                new_item.ammo_set( ammo_id, ch );
            } else {
                debugmsg( "tried to set ammo for %s which does not have ammo or a magazine",
                          new_item.typeId().c_str() );
            }
            // Make sure the item is in valid state
            if( new_item.magazine_integral() ) {
                new_item.charges = std::min( new_item.charges,
                                             new_item.ammo_capacity( item_controller->find_template( new_item.ammo_default() )->ammo->type ) );
            } else {
                new_item.charges = 0;
            }
        }

        if( new_item.is_magazine() ||
            new_item.has_pocket_type( pocket_type::MAGAZINE_WELL ) ) {
            bool spawn_ammo = rng( 0, 99 ) < with_ammo && new_item.ammo_remaining() == 0 && ch == -1;
            bool spawn_mag = rng( 0, 99 ) < with_magazine && !new_item.magazine_integral() &&
                             !new_item.magazine_current();

            if( spawn_mag ) {
                item mag( new_item.magazine_default(), new_item.birthday() );
                if( spawn_ammo && !mag.ammo_default().is_null() ) {
                    mag.ammo_set( mag.ammo_default() );
                }
                new_item.put_in( mag, pocket_type::MAGAZINE_WELL );
            } else if( spawn_ammo && !new_item.ammo_default().is_null() ) {
                if( ammo ) {
                    const item am = ammo->create_single( new_item.birthday() );
                    new_item.ammo_set( am.typeId() );
                } else {
                    new_item.ammo_set( new_item.ammo_default() );
                }
            }
        }

        if( cont.has_value() && !cont->is_null() ) {
            const pocket_type pk_type = guess_pocket_for( *cont, new_item );
            cont->put_in( new_item, pk_type );
            cont->add_automatic_whitelist();
            new_item = std::move( *cont );
            if( sealed ) {
                new_item.seal();
            }
        }
    }

    if( contents != nullptr ) {
        Item_spawn_data::ItemList contentitems;
        contents->create( contentitems, new_item.birthday() );
        for( const item &it : contentitems ) {
            // custom code for directly attaching pockets to MOLLE vests
            const use_function *action = new_item.get_use( "attach_molle" );
            if( action && it.can_attach_as_pocket() ) {
                const molle_attach_actor *actor = dynamic_cast<const molle_attach_actor *>
                                                  ( action->get_actor_ptr() );
                const int vacancies = actor->size - new_item.get_contents().get_additional_space_used();
                // intentionally might lose items here if they don't fit this is because it's
                // impossible to know in a spawn group how much stuff could end up in it
                // if you roll 3, 3 size items in a 3 slot vest you shouldn't get an error
                // but you should get a vest with at least the first one
                if( it.get_pocket_size() <= vacancies ) {
                    new_item.get_contents().add_pocket( it );
                }
            } else {
                const pocket_type pk_type = guess_pocket_for( new_item, it );
                new_item.put_in( it, pk_type );
            }
        }
        if( sealed ) {
            new_item.seal();
        }
    }

    for( const flag_id &flag : custom_flags ) {
        new_item.set_flag( flag );
    }

    if( !snippets.empty() ) {
        new_item.snip_id = random_entry( snippets );
    }
}

void Item_modifier::check_consistency( const std::string &context ) const
{
    if( ammo != nullptr ) {
        ammo->check_consistency( true );
    }
    if( contents != nullptr ) {
        contents->check_consistency( true );
    }
    if( container != nullptr ) {
        container->check_consistency( true );
    }
    if( with_ammo < 0 || with_ammo > 100 ) {
        debugmsg( "in %s: Item modifier's ammo chance %d is out of range", context, with_ammo );
    }
    if( with_magazine < 0 || with_magazine > 100 ) {
        debugmsg( "in %s: Item modifier's magazine chance %d is out of range",
                  context, with_magazine );
    }
}

bool Item_modifier::remove_item( const itype_id &itemid )
{
    if( ammo != nullptr ) {
        if( ammo->remove_item( itemid ) ) {
            ammo.reset();
        }
    }
    if( contents != nullptr ) {
        if( contents->remove_item( itemid ) ) {
            contents.reset();
        }
    }
    if( container != nullptr ) {
        if( container->remove_item( itemid ) ) {
            container.reset();
            return true;
        }
    }
    return false;
}

void Item_modifier::replace_items( const std::unordered_map<itype_id, itype_id> &replacements )
const
{
    if( ammo ) {
        ammo->replace_items( replacements );
    }
    if( container ) {
        container->replace_items( replacements );
    }
    if( contents ) {
        contents->replace_items( replacements );
    }
}

Item_group::Item_group( Type t, int probability, int ammo_chance, int magazine_chance,
                        const std::string &context, holiday event )
    : Item_spawn_data( probability, context, event )
    , type( t )
    , with_ammo( ammo_chance )
    , with_magazine( magazine_chance )
    , sum_prob( 0 )
{
    if( probability <= 0 || ( t != Type::G_DISTRIBUTION && probability > 100 ) ) {
        debugmsg( "Probability %d out of range", probability );
    }
    if( ammo_chance < 0 || ammo_chance > 100 ) {
        debugmsg( "Ammo chance %d is out of range.", ammo_chance );
    }
    if( magazine_chance < 0 || magazine_chance > 100 ) {
        debugmsg( "Magazine chance %d is out of range.", magazine_chance );
    }
}

void Item_group::add_item_entry( const itype_id &itemid, int probability,
                                 const std::string &variant )
{
    std::string entry_context = "item " + itemid.str() + " within " + context();
    std::unique_ptr<Single_item_creator> added = std::make_unique<Single_item_creator>(
                itemid.str(), Single_item_creator::S_ITEM, probability, entry_context );
    if( !variant.empty() ) {
        added->modifier.emplace();
        added->modifier->variant = variant;
    }
    add_entry( std::move( added ) );
}

void Item_group::add_group_entry( const item_group_id &groupid, int probability )
{
    add_entry( std::make_unique<Single_item_creator>(
                   groupid.str(), Single_item_creator::S_ITEM_GROUP, probability,
                   "item_group id " + groupid.str() ) );
}

void Item_group::add_entry( std::unique_ptr<Item_spawn_data> ptr )
{
    cata_assert( ptr.get() != nullptr );
    if( ptr->get_probability( true ) <= 0 ) {
        return;
    }
    if( type == G_COLLECTION ) {
        ptr->set_probablility( std::min( 100, ptr->get_probability( true ) ) );
    }
    sum_prob += ptr->get_probability( true );

    // Make the ammo and magazine probabilities from the outer entity apply to the nested entity:
    // If ptr is an Item_group, it already inherited its parent's ammo/magazine chances in its constructor.
    Single_item_creator *sic = dynamic_cast<Single_item_creator *>( ptr.get() );
    if( sic ) {
        sic->inherit_ammo_mag_chances( with_ammo, with_magazine );
    }
    items.push_back( std::move( ptr ) );
}

std::size_t Item_group::create( Item_spawn_data::ItemList &list,
                                const time_point &birthday, RecursionList &rec, spawn_flags flags ) const
{
    std::size_t prev_list_size = list.size();
    if( type == G_COLLECTION ) {
        for( const auto &elem : items ) {
            if( !( flags & spawn_flags::maximized ) && rng( 0, 99 ) >= elem->get_probability( false ) ) {
                continue;
            }
            elem->create( list, birthday, rec, flags );
        }
    } else if( type == G_DISTRIBUTION ) {
        int p = rng( 0, sum_prob - 1 );
        for( const auto &elem : items ) {
            bool ev_based = elem->is_event_based();
            int prob = elem->get_probability( false );
            int real_prob = elem->get_probability( true );
            p -= real_prob;
            if( ( ev_based && prob == 0 ) || p >= 0 ) {
                continue;
            }
            elem->create( list, birthday, rec, flags );
            break;
        }
    }
    const std::size_t items_created = list.size() - prev_list_size;
    put_into_container( list, items_created, container_item, container_item_variant, sealed, birthday,
                        on_overflow, context() );

    return list.size() - prev_list_size;
}

item Item_group::create_single( const time_point &birthday, RecursionList &rec ) const
{
    if( type == G_COLLECTION ) {
        for( const auto &elem : items ) {
            if( rng( 0, 99 ) >= elem->get_probability( false ) ) {
                continue;
            }
            return elem->create_single( birthday, rec );
        }
    } else if( type == G_DISTRIBUTION ) {
        int p = rng( 0, sum_prob - 1 );
        for( const auto &elem : items ) {
            bool ev_based = elem->is_event_based();
            int prob = elem->get_probability( false );
            int real_prob = elem->get_probability( true );
            p -= real_prob;
            if( ( ev_based && prob == 0 ) || p >= 0 ) {
                continue;
            }
            return elem->create_single( birthday, rec );
        }
    }
    return item( itype_id::NULL_ID(), birthday );
}

void Item_group::check_consistency( bool actually_spawn ) const
{
    // if type is collection, then spawning itself automatically spawnes all entries,
    // thus no need to spawn them individually.
    for( const auto &elem : items ) {
        // Assuming every entry needs to be spawned.
        elem->check_consistency( true );
    }
    // If this group has no container, no need to spawn it as a whole as every entry has been spawned.
    Item_spawn_data::check_consistency( container_item ? actually_spawn : false );
}

int Item_spawn_data::get_probability( bool skip_event_check ) const
{
    // Use probability as normal
    if( skip_event_check || event == holiday::none ) {
        return probability;
    }

    // Item spawn is event-based, but option is disabled
    std::string opt = get_option<std::string>( "EVENT_SPAWNS" );
    if( opt != "items" && opt != "both" ) {
        return 0;
    }

    // Use probability if the current holiday matches the item's spawn event
    if( event == get_holiday_from_time() ) {
        return probability;
    }

    // Not currently the item's holiday
    return 0;
}

bool Item_group::remove_item( const itype_id &itemid )
{
    for( prop_list::iterator a = items.begin(); a != items.end(); ) {
        if( ( *a )->remove_item( itemid ) ) {
            sum_prob -= ( *a )->get_probability( true );
            a = items.erase( a );
        } else {
            ++a;
        }
    }
    if( container_item && ( *container_item == itemid ) ) {
        container_item = std::nullopt;
        on_overflow = overflow_behaviour::none;
    }
    return items.empty();
}

void Item_group::replace_items( const std::unordered_map<itype_id, itype_id> &replacements )
{
    for( const std::unique_ptr<Item_spawn_data> &elem : items ) {
        elem->replace_items( replacements );
    }
}

bool Item_group::has_item( const itype_id &itemid ) const
{
    for( const std::unique_ptr<Item_spawn_data> &elem : items ) {
        if( elem->has_item( itemid ) ) {
            return true;
        }
    }
    return false;
}

std::set<const itype *> Item_group::every_item() const
{
    std::set<const itype *> result;
    for( const auto &spawn_data : items ) {
        std::set<const itype *> these_items = spawn_data->every_item();
        result.insert( these_items.begin(), these_items.end() );
    }
    return result;
}

std::map<const itype *, std::pair<int, int>> Item_group::every_item_min_max() const
{
    std::map<const itype *, std::pair<int, int>> result;
    for( const auto &spawn_data : items ) {
        std::map<const itype *, std::pair<int, int>> item_entries = spawn_data->every_item_min_max();
        for( const auto &item_entry : item_entries ) {
            const auto existing_item_entry = result.find( item_entry.first );
            if( existing_item_entry != result.end() ) {
                existing_item_entry->second.first += item_entry.second.first;
                existing_item_entry->second.second += item_entry.second.second;
            } else {
                result.insert( item_entry );
            }
        }
    }
    return result;
}

std::string item_group::potential_items( const item_group_id &group_id )
{
    std::string ret;
    const Item_spawn_data *spawn_data = spawn_data_from_group( group_id );
    if( spawn_data ) {
        const std::map<const itype *, std::pair<int, int>> items_min_max =
                    spawn_data->every_item_min_max();
        for( const auto &item_min_max : items_min_max ) {
            const int &min = item_min_max.second.first;
            const int &max = item_min_max.second.second;
            if( min != max ) {
                ret += string_format( "- <color_cyan>%d-%d %s</color>\n", min, max,
                                      item_min_max.first->nname( max ) );
            } else {
                ret += string_format( "- <color_cyan>%d %s</color>\n", max, item_min_max.first->nname( max ) );
            }
        }
    }
    return ret;
}

item_group::ItemList item_group::items_from( const item_group_id &group_id,
        const time_point &birthday, spawn_flags flags )
{
    const Item_spawn_data *group = item_controller->get_group( group_id );
    if( group == nullptr ) {
        return ItemList();
    }
    ItemList result;
    result.reserve( 20 );
    group->create( result, birthday, flags );
    return result;
}

item_group::ItemList item_group::items_from( const item_group_id &group_id )
{
    return items_from( group_id, calendar::turn_zero );
}

item item_group::item_from( const item_group_id &group_id, const time_point &birthday )
{
    const Item_spawn_data *group = item_controller->get_group( group_id );
    if( group == nullptr ) {
        return item();
    }
    return group->create_single( birthday );
}

item item_group::item_from( const item_group_id &group_id )
{
    return item_from( group_id, calendar::turn_zero );
}

bool item_group::group_is_defined( const item_group_id &group_id )
{
    return item_controller->get_group( group_id ) != nullptr;
}

Item_spawn_data *item_group::spawn_data_from_group( const item_group_id &group_id )
{
    return item_controller->get_group( group_id );
}

bool item_group::group_contains_item( const item_group_id &group_id, const itype_id &type_id )
{
    const Item_spawn_data *group = item_controller->get_group( group_id );
    if( group == nullptr ) {
        return false;
    }
    return group->has_item( type_id );
}

std::set<const itype *> item_group::every_possible_item_from( const item_group_id &group_id )
{
    Item_spawn_data *group = item_controller->get_group( group_id );
    if( group == nullptr ) {
        return {};
    }
    return group->every_item();
}

void item_group::load_item_group( const JsonObject &jsobj, const item_group_id &group_id,
                                  const std::string &subtype )
{
    item_controller->load_item_group( jsobj, group_id, subtype );
}

static item_group_id get_unique_group_id()
{
    // This is just a hint what id to use next. Overflow of it is defined and if the group
    // name is already used, we simply go the next id.
    static unsigned int next_id = 0;
    // Prefixing with a character outside the ASCII range, so it is hopefully unique and
    // (if actually printed somewhere) stands out. Theoretically those auto-generated group
    // names should not be seen anywhere.
    static const std::string unique_prefix = "\u01F7 ";
    while( true ) {
        item_group_id new_group( unique_prefix + std::to_string( next_id++ ) );
        if( !item_group::group_is_defined( new_group ) ) {
            return new_group;
        }
    }
}

item_group_id item_group::load_item_group( const JsonValue &value,
        const std::string &default_subtype, const std::string &context )
{
    if( value.test_string() ) {
        return item_group_id( value.get_string() );
    } else if( value.test_object() ) {
        item_group_id group = get_unique_group_id();

        JsonObject jo = value.get_object();
        const std::string subtype = jo.get_string( "subtype", default_subtype );
        item_controller->load_item_group( jo, group, subtype, context );

        return group;
    } else if( value.test_array() ) {
        item_group_id group = get_unique_group_id();

        JsonArray jarr = value.get_array();
        // load_item_group needs a bool, invalid subtypes are unexpected and most likely errors
        // from the caller of this function.
        if( default_subtype != "collection" && default_subtype != "distribution" ) {
            debugmsg( "invalid subtype for item group: %s", default_subtype.c_str() );
        }
        item_controller->load_item_group( jarr, group, default_subtype == "collection", 0, 0,
                                          context );

        return group;
    }
    value.throw_error( "invalid item group, must be string (group id) or object/array (the group data)" );
}
