#include "item_group.h"

#include <algorithm>
#include <cassert>
#include <list>
#include <set>

#include "debug.h"
#include "item.h"
#include "item_factory.h"
#include "itype.h"
#include "json.h"
#include "rng.h"
#include "calendar.h"
#include "compatibility.h"
#include "enums.h"
#include "type_id.h"
#include "flat_set.h"

static const std::string null_item_id( "null" );

Item_spawn_data::ItemList Item_spawn_data::create( const time_point &birthday ) const
{
    RecursionList rec;
    return create( birthday, rec );
}

item Item_spawn_data::create_single( const time_point &birthday ) const
{
    RecursionList rec;
    return create_single( birthday, rec );
}

Single_item_creator::Single_item_creator( const std::string &_id, Type _type, int _probability )
    : Item_spawn_data( _probability )
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
        if( std::find( rec.begin(), rec.end(), id ) != rec.end() ) {
            debugmsg( "recursion in item spawn list %s", id.c_str() );
            return item( null_item_id, birthday );
        }
        rec.push_back( id );
        Item_spawn_data *isd = item_controller->get_group( id );
        if( isd == nullptr ) {
            debugmsg( "unknown item spawn list %s", id.c_str() );
            return item( null_item_id, birthday );
        }
        tmp = isd->create_single( birthday, rec );
        rec.erase( rec.end() - 1 );
    } else if( type == S_NONE ) {
        return item( null_item_id, birthday );
    }
    if( one_in( 3 ) && tmp.has_flag( "VARSIZE" ) ) {
        tmp.item_tags.insert( "FIT" );
    }
    if( modifier ) {
        modifier->modify( tmp );
    } else {
        tmp = tmp.in_its_container();
    }
    // TODO: change the spawn lists to contain proper references to containers
    return tmp;
}

Item_spawn_data::ItemList Single_item_creator::create( const time_point &birthday,
        RecursionList &rec ) const
{
    ItemList result;
    int cnt = 1;
    if( modifier ) {
        auto modifier_count = modifier->count;
        cnt = ( modifier_count.first == modifier_count.second ) ? modifier_count.first : rng(
                  modifier_count.first, modifier_count.second );
    }
    for( ; cnt > 0; cnt-- ) {
        if( type == S_ITEM ) {
            const auto itm = create_single( birthday, rec );
            if( !itm.is_null() ) {
                result.push_back( itm );
            }
        } else {
            if( std::find( rec.begin(), rec.end(), id ) != rec.end() ) {
                debugmsg( "recursion in item spawn list %s", id.c_str() );
                return result;
            }
            rec.push_back( id );
            Item_spawn_data *isd = item_controller->get_group( id );
            if( isd == nullptr ) {
                debugmsg( "unknown item spawn list %s", id.c_str() );
                return result;
            }
            ItemList tmplist = isd->create( birthday, rec );
            rec.erase( rec.end() - 1 );
            if( modifier ) {
                for( auto &elem : tmplist ) {
                    modifier->modify( elem );
                }
            }
            result.insert( result.end(), tmplist.begin(), tmplist.end() );
        }
    }
    return result;
}

void Single_item_creator::check_consistency( const std::string &context ) const
{
    if( type == S_ITEM ) {
        if( !item::type_is_defined( id ) ) {
            debugmsg( "item id %s is unknown (in %s)", id, context );
        }
    } else if( type == S_ITEM_GROUP ) {
        if( !item_group::group_is_defined( id ) ) {
            debugmsg( "item group id %s is unknown (in %s)", id, context );
        }
    } else if( type == S_NONE ) {
        // this is okay, it will be ignored
    } else {
        debugmsg( "Unknown type of Single_item_creator: %d", static_cast<int>( type ) );
    }
    if( modifier ) {
        modifier->check_consistency( context );
    }
}

bool Single_item_creator::remove_item( const Item_tag &itemid )
{
    if( modifier ) {
        if( modifier->remove_item( itemid ) ) {
            type = S_NONE;
            return true;
        }
    }
    if( type == S_ITEM ) {
        if( itemid == id ) {
            type = S_NONE;
            return true;
        }
    } else if( type == S_ITEM_GROUP ) {
        Item_spawn_data *isd = item_controller->get_group( id );
        if( isd != nullptr ) {
            isd->remove_item( itemid );
        }
    }
    return type == S_NONE;
}

bool Single_item_creator::has_item( const Item_tag &itemid ) const
{
    return type == S_ITEM && itemid == id;
}

std::set<const itype *> Single_item_creator::every_item() const
{
    switch( type ) {
        case S_ITEM:
            return { item::find_type( id ) };
        case S_ITEM_GROUP: {
            Item_spawn_data *isd = item_controller->get_group( id );
            if( isd != nullptr ) {
                return isd->every_item();
            }
            return {};
        }
        case S_NONE:
            return {};
    }
    assert( !"Unexpected type" );
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
    , dirt( 0, 500 )
    , charges( -1, -1 )
    , with_ammo( 0 )
    , with_magazine( 0 )
{
}

void Item_modifier::modify( item &new_item ) const
{
    if( new_item.is_null() ) {
        return;
    }

    new_item.set_damage( rng( damage.first, damage.second ) );
    if( new_item.is_gun() && !new_item.has_flag( "PRIMITIVE_RANGED_WEAPON" ) ) {
        int random_dirt = rng( dirt.first, dirt.second );
        if( random_dirt > 0 ) {
            new_item.set_var( "dirt", random_dirt );
            new_item.faults.emplace( "fault_gun_dirt" );
        }
    }

    // create container here from modifier or from default to get max charges later
    item cont;
    if( container != nullptr ) {
        cont = container->create_single( new_item.birthday() );
    }
    if( cont.is_null() && new_item.type->default_container.has_value() ) {
        const itype_id &cont_value = new_item.type->default_container.value_or( "null" );
        if( cont_value != "null" ) {
            cont = item( cont_value, new_item.birthday() );
        }
    }

    int max_capacity = -1;
    if( charges.first != -1 && charges.second == -1 ) {
        int max_ammo = new_item.ammo_capacity();
        if( max_ammo > 0 ) {
            max_capacity = max_ammo;
        }
    }

    if( max_capacity == -1 && !cont.is_null() ) {
        if( new_item.made_of( LIQUID ) ) {
            max_capacity = cont.get_remaining_capacity_for_liquid( new_item );
        } else if( !new_item.is_tool() && !new_item.is_gun() && !new_item.is_magazine() ) {
            max_capacity = new_item.charges_per_volume( cont.get_container_capacity() );
        }
    }

    bool charges_not_set = charges.first == -1 && charges.second == -1;
    int ch = -1;
    if( !charges_not_set ) {
        int charges_min = charges.first;
        int charges_max = charges.second;

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

        ch = ( charges_min == charges_max ) ? charges_min : rng( charges_min,
                charges_max );
    } else if( !cont.is_null() && new_item.made_of( LIQUID ) ) {
        new_item.charges = max_capacity;
    }

    if( ch != -1 ) {
        if( new_item.count_by_charges() || new_item.made_of( LIQUID ) ) {
            // food, ammo
            // count_by_charges requires that charges is at least 1. It makes no sense to
            // spawn a "water (0)" item.
            new_item.charges = std::max( 1, ch );
        } else if( new_item.is_tool() ) {
            const int qty = std::min( ch, new_item.ammo_capacity() );
            new_item.charges = qty;
            if( !new_item.ammo_types().empty() && qty > 0 ) {
                new_item.ammo_set( new_item.ammo_default(), qty );
            }
        } else if( new_item.type->can_have_charges() ) {
            new_item.charges = ch;
        }
    }

    if( ch > 0 && ( new_item.is_gun() || new_item.is_magazine() ) ) {
        if( ammo == nullptr ) {
            // In case there is no explicit ammo item defined, use the default ammo
            if( !new_item.ammo_types().empty() ) {
                new_item.ammo_set( new_item.ammo_default(), ch );
            }
        } else {
            const item am = ammo->create_single( new_item.birthday() );
            new_item.ammo_set( am.typeId(), ch );
        }
        // Make sure the item is in valid state
        if( new_item.ammo_data() && new_item.magazine_integral() ) {
            new_item.charges = std::min( new_item.charges, new_item.ammo_capacity() );
        } else {
            new_item.charges = 0;
        }
    }

    if( new_item.is_tool() || new_item.is_gun() || new_item.is_magazine() ) {
        bool spawn_ammo = rng( 0, 99 ) < with_ammo && new_item.ammo_remaining() == 0 && ch == -1 &&
                          ( !new_item.is_tool() || new_item.type->tool->rand_charges.empty() );
        bool spawn_mag  = rng( 0, 99 ) < with_magazine && !new_item.magazine_integral() &&
                          !new_item.magazine_current();

        if( spawn_mag ) {
            new_item.contents.emplace_back( new_item.magazine_default(), new_item.birthday() );
        }

        if( spawn_ammo ) {
            if( ammo ) {
                const item am = ammo->create_single( new_item.birthday() );
                new_item.ammo_set( am.typeId() );
            } else {
                new_item.ammo_set( new_item.ammo_default() );
            }
        }
    }

    if( !cont.is_null() ) {
        cont.put_in( new_item );
        new_item = cont;
    }

    if( contents != nullptr ) {
        Item_spawn_data::ItemList contentitems = contents->create( new_item.birthday() );
        new_item.contents.insert( new_item.contents.end(), contentitems.begin(), contentitems.end() );
    }

    for( auto &flag : custom_flags ) {
        new_item.set_flag( flag );
    }
}

void Item_modifier::check_consistency( const std::string &context ) const
{
    if( ammo != nullptr ) {
        ammo->check_consistency( "ammo of " + context );
    }
    if( container != nullptr ) {
        container->check_consistency( "container of " + context );
    }
    if( with_ammo < 0 || with_ammo > 100 ) {
        debugmsg( "Item modifier's ammo chance %d is out of range", with_ammo );
    }
    if( with_magazine < 0 || with_magazine > 100 ) {
        debugmsg( "Item modifier's magazine chance %d is out of range", with_magazine );
    }
}

bool Item_modifier::remove_item( const Item_tag &itemid )
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

Item_group::Item_group( Type t, int probability, int ammo_chance, int magazine_chance )
    : Item_spawn_data( probability )
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

void Item_group::add_item_entry( const Item_tag &itemid, int probability )
{
    add_entry( std::make_unique<Single_item_creator>(
                   itemid, Single_item_creator::S_ITEM, probability ) );
}

void Item_group::add_group_entry( const Group_tag &groupid, int probability )
{
    add_entry( std::make_unique<Single_item_creator>(
                   groupid, Single_item_creator::S_ITEM_GROUP, probability ) );
}

void Item_group::add_entry( std::unique_ptr<Item_spawn_data> ptr )
{
    assert( ptr.get() != nullptr );
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

Item_spawn_data::ItemList Item_group::create( const time_point &birthday, RecursionList &rec ) const
{
    ItemList result;
    if( type == G_COLLECTION ) {
        for( const auto &elem : items ) {
            if( rng( 0, 99 ) >= ( elem )->probability ) {
                continue;
            }
            ItemList tmp = ( elem )->create( birthday, rec );
            result.insert( result.end(), tmp.begin(), tmp.end() );
        }
    } else if( type == G_DISTRIBUTION ) {
        int p = rng( 0, sum_prob - 1 );
        for( const auto &elem : items ) {
            p -= ( elem )->probability;
            if( p >= 0 ) {
                continue;
            }
            ItemList tmp = ( elem )->create( birthday, rec );
            result.insert( result.end(), tmp.begin(), tmp.end() );
            break;
        }
    }

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

void Item_group::check_consistency( const std::string &context ) const
{
    for( const auto &elem : items ) {
        ( elem )->check_consistency( "item in " + context );
    }
}

bool Item_group::remove_item( const Item_tag &itemid )
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

bool Item_group::has_item( const Item_tag &itemid ) const
{
    for( const auto &elem : items ) {
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

item_group::ItemList item_group::items_from( const Group_tag &group_id, const time_point &birthday )
{
    const auto group = item_controller->get_group( group_id );
    if( group == nullptr ) {
        return ItemList();
    }
    return group->create( birthday );
}

item_group::ItemList item_group::items_from( const Group_tag &group_id )
{
    return items_from( group_id, 0 );
}

item item_group::item_from( const Group_tag &group_id, const time_point &birthday )
{
    const auto group = item_controller->get_group( group_id );
    if( group == nullptr ) {
        return item();
    }
    return group->create_single( birthday );
}

item item_group::item_from( const Group_tag &group_id )
{
    return item_from( group_id, 0 );
}

bool item_group::group_is_defined( const Group_tag &group_id )
{
    return item_controller->get_group( group_id ) != nullptr;
}

bool item_group::group_contains_item( const Group_tag &group_id, const itype_id &type_id )
{
    const auto group = item_controller->get_group( group_id );
    if( group == nullptr ) {
        return false;
    }
    return group->has_item( type_id );
}

std::set<const itype *> item_group::every_possible_item_from( const Group_tag &group_id )
{
    Item_spawn_data *group = item_controller->get_group( group_id );
    if( group == nullptr ) {
        return {};
    }
    return group->every_item();
}

void item_group::load_item_group( JsonObject &jsobj, const Group_tag &group_id,
                                  const std::string &subtype )
{
    item_controller->load_item_group( jsobj, group_id, subtype );
}

static Group_tag get_unique_group_id()
{
    // This is just a hint what id to use next. Overflow of it is defined and if the group
    // name is already used, we simply go the next id.
    static unsigned int next_id = 0;
    // Prefixing with a character outside the ASCII range, so it is hopefully unique and
    // (if actually printed somewhere) stands out. Theoretically those auto-generated group
    // names should not be seen anywhere.
    static const std::string unique_prefix = "\u01F7 ";
    while( true ) {
        const Group_tag new_group = unique_prefix + to_string( next_id++ );
        if( !item_group::group_is_defined( new_group ) ) {
            return new_group;
        }
    }
}

Group_tag item_group::load_item_group( JsonIn &stream, const std::string &default_subtype )
{
    if( stream.test_string() ) {
        return stream.get_string();
    } else if( stream.test_object() ) {
        const Group_tag group = get_unique_group_id();

        JsonObject jo = stream.get_object();
        const std::string subtype = jo.get_string( "subtype", default_subtype );
        item_controller->load_item_group( jo, group, subtype );

        return group;
    } else if( stream.test_array() ) {
        const Group_tag group = get_unique_group_id();

        JsonArray jarr = stream.get_array();
        // load_item_group needs a bool, invalid subtypes are unexpected and most likely errors
        // from the caller of this function.
        if( default_subtype != "collection" && default_subtype != "distribution" ) {
            debugmsg( "invalid subtype for item group: %s", default_subtype.c_str() );
        }
        item_controller->load_item_group( jarr, group, default_subtype == "collection", 0, 0 );

        return group;
    } else {
        stream.error( "invalid item group, must be string (group id) or object/array (the group data)" );
        // stream.error always throws, this is here to prevent a warning
        return Group_tag{};
    }
}
