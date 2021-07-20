#include "item_group.h"

#include <algorithm>
#include <cstdlib>
#include <new>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "calendar.h"
#include "cata_assert.h"
#include "debug.h"
#include "enum_traits.h"
#include "enums.h"
#include "flag.h"
#include "generic_factory.h"
#include "item.h"
#include "item_contents.h"
#include "item_factory.h"
#include "item_pocket.h"
#include "itype.h"
#include "json.h"
#include "make_static.h"
#include "options.h"
#include "relic.h"
#include "ret_val.h"
#include "rng.h"
#include "type_id.h"
#include "units.h"

static const std::string null_item_id( "null" );

Item_spawn_data::ItemList Item_spawn_data::create(
    const time_point &birthday, spawn_flags flags ) const
{
    RecursionList rec;
    return create( birthday, rec, flags );
}

item Item_spawn_data::create_single( const time_point &birthday ) const
{
    RecursionList rec;
    return create_single( birthday, rec );
}

void Item_spawn_data::check_consistency() const
{
    if( on_overflow != overflow_behaviour::none && !container_item ) {
        debugmsg( "item group %s specifies overflow behaviour but not container", context() );
    }
    // Spawn ourselves with all possible items being definitely spawned, so as
    // to verify e.g. that if a container item was specified it can actually
    // contain what was wanted.
    create( calendar::turn_zero, spawn_flags::maximized );
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
    debugmsg( "Invalid overflow_behaviour" );
    abort();
}
} // namespace io

static item_pocket::pocket_type guess_pocket_for( const item &container, const item &payload )
{
    if( ( container.is_gun() && payload.is_gunmod() ) || ( container.is_tool() &&
            payload.is_toolmod() ) ) {
        return item_pocket::pocket_type::MOD;
    }
    if( container.is_software_storage() && payload.is_software() ) {
        return item_pocket::pocket_type::SOFTWARE;
    }
    if( ( container.is_gun() || container.is_tool() ) && payload.is_magazine() ) {
        return item_pocket::pocket_type::MAGAZINE_WELL;
    } else if( ( container.is_magazine() ) && payload.is_ammo() ) {
        return item_pocket::pocket_type::MAGAZINE;
    }
    return item_pocket::pocket_type::CONTAINER;
}

static void put_into_container(
    Item_spawn_data::ItemList &items, const cata::optional<itype_id> &container_type,
    time_point birthday, Item_spawn_data::overflow_behaviour on_overflow,
    const std::string &context )
{
    if( !container_type ) {
        return;
    }

    // Randomly permute the list of items so that when some don't fit it's
    // not always the ones at the end which are rejected.
    std::shuffle( items.begin(), items.end(), rng_get_engine() );

    item ctr( *container_type, birthday );
    Item_spawn_data::ItemList excess;
    for( const item &it : items ) {
        if( ctr.can_contain( it ) ) {
            const item_pocket::pocket_type pk_type = guess_pocket_for( ctr, it );
            ctr.put_in( it, pk_type );
        } else {
            switch( on_overflow ) {
                case Item_spawn_data::overflow_behaviour::none:
                    debugmsg( "item %s does not fit in container %s when spawning item group %s.  "
                              "This can be resolved either by changing the container or contents "
                              "to ensure that they fit, or by specifying an overflow behaviour via "
                              "\"on_overflow\" on the item group.",
                              it.typeId().str(), container_type->str(), context );
                    break;
                case Item_spawn_data::overflow_behaviour::spill:
                    excess.push_back( it );
                    break;
                case Item_spawn_data::overflow_behaviour::discard:
                    break;
                case Item_spawn_data::overflow_behaviour::last:
                    debugmsg( "Invalid overflow_behaviour" );
                    break;
            }
        }
    }
    excess.push_back( ctr );
    items = std::move( excess );
}

Single_item_creator::Single_item_creator( const std::string &_id, Type _type, int _probability,
        const std::string &context )
    : Item_spawn_data( _probability, context )
    , id( _id )
    , type( _type )
{
}

item Single_item_creator::create_single( const time_point &birthday, RecursionList &rec ) const
{
    item tmp;
    if( type == S_ITEM ) {
        if( id == "corpse" ) {
            tmp = item::make_corpse( mtype_id::NULL_ID(), birthday );
        } else {
            tmp = item( id, birthday );
        }
    } else if( type == S_ITEM_GROUP ) {
        item_group_id group_id( id );
        if( std::find( rec.begin(), rec.end(), group_id ) != rec.end() ) {
            debugmsg( "recursion in item spawn list %s", id.c_str() );
            return item( null_item_id, birthday );
        }
        rec.push_back( group_id );
        Item_spawn_data *isd = item_controller->get_group( group_id );
        if( isd == nullptr ) {
            debugmsg( "unknown item spawn list %s", id.c_str() );
            return item( null_item_id, birthday );
        }
        tmp = isd->create_single( birthday, rec );
        rec.erase( rec.end() - 1 );
    } else if( type == S_NONE ) {
        return item( null_item_id, birthday );
    }
    if( one_in( 3 ) && tmp.has_flag( flag_VARSIZE ) ) {
        tmp.set_flag( flag_FIT );
    }
    if( modifier ) {
        modifier->modify( tmp, "modifier for " + context() );
    } else {
        int qty = tmp.charges;
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
    if( container_item ) {
        tmp = tmp.in_container( *container_item, tmp.charges, sealed );
    }
    return tmp;
}

Item_spawn_data::ItemList Single_item_creator::create(
    const time_point &birthday, RecursionList &rec, spawn_flags flags ) const
{
    ItemList result;
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
            const item itm = create_single( birthday, rec );
            if( flags & spawn_flags::use_spawn_rate && !itm.has_flag( STATIC( flag_id( "MISSION_ITEM" ) ) ) &&
                rng_float( 0, 1 ) > spawn_rate ) {
                continue;
            }
            if( !itm.is_null() ) {
                result.push_back( itm );
            }
        } else {
            item_group_id group_id( id );
            if( std::find( rec.begin(), rec.end(), group_id ) != rec.end() ) {
                debugmsg( "recursion in item spawn list %s", id.c_str() );
                return result;
            }
            rec.push_back( group_id );
            Item_spawn_data *isd = item_controller->get_group( group_id );
            if( isd == nullptr ) {
                debugmsg( "unknown item spawn list %s", id.c_str() );
                return result;
            }
            ItemList tmplist = isd->create( birthday, rec, flags );
            rec.erase( rec.end() - 1 );
            if( modifier ) {
                for( auto &elem : tmplist ) {
                    modifier->modify( elem, "modifier for " + context() );
                }
            }
            result.insert( result.end(), tmplist.begin(), tmplist.end() );
        }
    }
    if( artifact ) {
        for( item &it : result ) {
            it.overwrite_relic( artifact->generate_relic( it.typeId() ) );
        }
    }
    put_into_container( result, container_item, birthday, on_overflow, context() );
    return result;
}

void Single_item_creator::check_consistency() const
{
    if( type == S_ITEM ) {
        if( !item::type_is_defined( itype_id( id ) ) ) {
            debugmsg( "item id %s is unknown (in %s)", id, context() );
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
    Item_spawn_data::check_consistency();
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
    } else if( type == S_ITEM_GROUP ) {
        Item_spawn_data *isd = item_controller->get_group( item_group_id( id ) );
        if( isd != nullptr ) {
            isd->remove_item( itemid );
        }
    }
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
    } else if( type == S_ITEM_GROUP ) {
        Item_spawn_data *isd = item_controller->get_group( item_group_id( id ) );
        if( isd != nullptr ) {
            isd->replace_items( replacements );
        }
    }
}

bool Single_item_creator::has_item( const itype_id &itemid ) const
{
    return type == S_ITEM && itemid.str() == id;
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
    cata_assert( !"Unexpected type" );
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
    // no need for dirt if it's a bow
    if( new_item.is_gun() && !new_item.has_flag( flag_PRIMITIVE_RANGED_WEAPON ) &&
        !new_item.has_flag( flag_NON_FOULING ) ) {
        int random_dirt = rng( dirt.first, dirt.second );
        // if gun RNG is dirty, must add dirt fault to allow cleaning
        if( random_dirt > 0 ) {
            new_item.set_var( "dirt", random_dirt );
            new_item.faults.emplace( "fault_gun_dirt" );
            // chance to be unlubed, but only if it's not a laser or something
        } else if( one_in( 10 ) && !new_item.has_flag( flag_NEEDS_NO_LUBE ) ) {
            new_item.faults.emplace( "fault_gun_unlubricated" );
        }
    }

    new_item.set_gun_variant( variant );

    // create container here from modifier or from default to get max charges later
    item cont;
    if( container != nullptr ) {
        cont = container->create_single( new_item.birthday() );
    }
    if( cont.is_null() && new_item.type->default_container.has_value() ) {
        const itype_id &cont_value = new_item.type->default_container.value_or( "null" );
        if( !cont_value.is_null() ) {
            cont = item( cont_value, new_item.birthday() );
        }
    }

    int max_capacity = -1;
    if( charges.first != -1 && charges.second == -1 && new_item.is_magazine() ) {
        const int max_ammo = new_item.ammo_capacity( item_controller->find_template(
                                 new_item.ammo_default() )->ammo->type );
        if( max_ammo > 0 ) {
            max_capacity = max_ammo;
        }
    }

    if( max_capacity == -1 && !cont.is_null() && ( new_item.made_of( phase_id::LIQUID ) ||
            ( !new_item.is_tool() && !new_item.is_gun() && !new_item.is_magazine() ) ) ) {
        if( new_item.type->weight == 0_gram ) {
            max_capacity = new_item.charges_per_volume( cont.get_total_capacity() );
        } else {
            max_capacity = std::min( new_item.charges_per_volume( cont.get_total_capacity() ),
                                     new_item.charges_per_weight( cont.get_total_weight_capacity() ) );
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
    } else if( !cont.is_null() && new_item.made_of( phase_id::LIQUID ) ) {
        new_item.charges = std::max( 1, max_capacity );
    }

    if( ch != -1 ) {
        if( new_item.count_by_charges() || new_item.made_of( phase_id::LIQUID ) ) {
            // food, ammo
            // count_by_charges requires that charges is at least 1. It makes no sense to
            // spawn a "water (0)" item.
            new_item.charges = std::max( 1, ch );
        } else if( new_item.is_tool() ) {
            if( !new_item.magazine_default().is_null() ) {
                item mag( new_item.magazine_default() );
                mag.ammo_set( mag.ammo_default(), ch );
                new_item.put_in( mag, item_pocket::pocket_type::MAGAZINE_WELL );
            } else if( new_item.is_magazine() ) {
                new_item.ammo_set( new_item.ammo_default(), ch );
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
        new_item.has_pocket_type( item_pocket::pocket_type::MAGAZINE_WELL ) ) {
        bool spawn_ammo = rng( 0, 99 ) < with_ammo && new_item.ammo_remaining() == 0 && ch == -1 &&
                          ( !new_item.is_tool() || new_item.type->tool->rand_charges.empty() );
        bool spawn_mag  = rng( 0, 99 ) < with_magazine && !new_item.magazine_integral() &&
                          !new_item.magazine_current();

        if( spawn_mag ) {
            item mag( new_item.magazine_default(), new_item.birthday() );
            if( spawn_ammo ) {
                mag.ammo_set( mag.ammo_default() );
            }
            new_item.put_in( mag, item_pocket::pocket_type::MAGAZINE_WELL );
        } else if( spawn_ammo && !new_item.ammo_default().is_null() ) {
            if( ammo ) {
                const item am = ammo->create_single( new_item.birthday() );
                new_item.ammo_set( am.typeId() );
            } else {
                new_item.ammo_set( new_item.ammo_default() );
            }
        }
    }

    if( !cont.is_null() ) {
        const item_pocket::pocket_type pk_type = guess_pocket_for( cont, new_item );
        cont.put_in( new_item, pk_type );
        new_item = cont;
        if( sealed ) {
            new_item.seal();
        }
    }

    if( contents != nullptr ) {
        Item_spawn_data::ItemList contentitems = contents->create( new_item.birthday() );
        for( const item &it : contentitems ) {
            const item_pocket::pocket_type pk_type = guess_pocket_for( new_item, it );
            new_item.put_in( it, pk_type );
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
        ammo->check_consistency();
    }
    if( container != nullptr ) {
        container->check_consistency();
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
    if( container != nullptr ) {
        if( container->remove_item( itemid ) ) {
            container.reset();
            return true;
        }
    }
    return false;
}

void Item_modifier::replace_items( const std::unordered_map<itype_id, itype_id> &replacements )
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
                        const std::string &context )
    : Item_spawn_data( probability, context )
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
    if( ptr->probability <= 0 ) {
        return;
    }
    if( type == G_COLLECTION ) {
        ptr->probability = std::min( 100, ptr->probability );
    }
    sum_prob += ptr->probability;

    // Make the ammo and magazine probabilities from the outer entity apply to the nested entity:
    // If ptr is an Item_group, it already inherited its parent's ammo/magazine chances in its constructor.
    Single_item_creator *sic = dynamic_cast<Single_item_creator *>( ptr.get() );
    if( sic ) {
        sic->inherit_ammo_mag_chances( with_ammo, with_magazine );
    }
    items.push_back( std::move( ptr ) );
}

Item_spawn_data::ItemList Item_group::create(
    const time_point &birthday, RecursionList &rec, spawn_flags flags ) const
{
    ItemList result;
    if( type == G_COLLECTION ) {
        for( const auto &elem : items ) {
            if( !( flags & spawn_flags::maximized ) && rng( 0, 99 ) >= ( elem )->probability ) {
                continue;
            }
            ItemList tmp = ( elem )->create( birthday, rec, flags );
            result.insert( result.end(), tmp.begin(), tmp.end() );
        }
    } else if( type == G_DISTRIBUTION ) {
        int p = rng( 0, sum_prob - 1 );
        for( const auto &elem : items ) {
            p -= ( elem )->probability;
            if( p >= 0 ) {
                continue;
            }
            ItemList tmp = ( elem )->create( birthday, rec, flags );
            result.insert( result.end(), tmp.begin(), tmp.end() );
            break;
        }
    }
    put_into_container( result, container_item, birthday, on_overflow, context() );

    return result;
}

item Item_group::create_single( const time_point &birthday, RecursionList &rec ) const
{
    if( type == G_COLLECTION ) {
        for( const auto &elem : items ) {
            if( rng( 0, 99 ) >= ( elem )->probability ) {
                continue;
            }
            return ( elem )->create_single( birthday, rec );
        }
    } else if( type == G_DISTRIBUTION ) {
        int p = rng( 0, sum_prob - 1 );
        for( const auto &elem : items ) {
            p -= ( elem )->probability;
            if( p >= 0 ) {
                continue;
            }
            return ( elem )->create_single( birthday, rec );
        }
    }
    return item( null_item_id, birthday );
}

void Item_group::check_consistency() const
{
    for( const auto &elem : items ) {
        ( elem )->check_consistency();
    }
    Item_spawn_data::check_consistency();
}

void Item_spawn_data::set_container_item( const itype_id &container )
{
    container_item = container;
}

bool Item_group::remove_item( const itype_id &itemid )
{
    for( prop_list::iterator a = items.begin(); a != items.end(); ) {
        if( ( *a )->remove_item( itemid ) ) {
            sum_prob -= ( *a )->probability;
            a = items.erase( a );
        } else {
            ++a;
        }
    }
    return items.empty();
}

void Item_group::replace_items( const std::unordered_map<itype_id, itype_id> &replacements )
{
    for( const std::unique_ptr<Item_spawn_data> &elem : items ) {
        ( elem )->replace_items( replacements );
    }
}

bool Item_group::has_item( const itype_id &itemid ) const
{
    for( const std::unique_ptr<Item_spawn_data> &elem : items ) {
        if( ( elem )->has_item( itemid ) ) {
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

item_group::ItemList item_group::items_from( const item_group_id &group_id,
        const time_point &birthday, spawn_flags flags )
{
    const Item_spawn_data *group = item_controller->get_group( group_id );
    if( group == nullptr ) {
        return ItemList();
    }
    return group->create( birthday, flags );
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
        const item_group_id new_group( unique_prefix + std::to_string( next_id++ ) );
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
        const item_group_id group = get_unique_group_id();

        JsonObject jo = value.get_object();
        const std::string subtype = jo.get_string( "subtype", default_subtype );
        item_controller->load_item_group( jo, group, subtype, context );

        return group;
    } else if( value.test_array() ) {
        const item_group_id group = get_unique_group_id();

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
