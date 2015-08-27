#include "item_factory.h"
#include "item_group.h"
#include "rng.h"
#include "item.h"
#include "debug.h"
#include "itype.h"
#include <map>
#include <algorithm>
#include <cassert>

static const std::string null_item_id("null");

Item_spawn_data::ItemList Item_spawn_data::create( int birthday ) const
{
    RecursionList rec;
    return create( birthday, rec );
}

item Item_spawn_data::create_single( int birthday ) const
{
    RecursionList rec;
    return create_single( birthday, rec );
}

Single_item_creator::Single_item_creator(const std::string &_id, Type _type, int _probability)
    : Item_spawn_data(_probability)
    , id(_id)
    , type(_type)
    , modifier()
{
}

Single_item_creator::~Single_item_creator()
{
}

item Single_item_creator::create_single(int birthday, RecursionList &rec) const
{
    item tmp;
    if (type == S_ITEM) {
        if (id == "corpse") {
            tmp.make_corpse( NULL_ID, birthday );
        } else {
            tmp = item(id, birthday);
        }
    } else if (type == S_ITEM_GROUP) {
        if (std::find(rec.begin(), rec.end(), id) != rec.end()) {
            debugmsg("recursion in item spawn list %s", id.c_str());
            return item(null_item_id, birthday);
        }
        rec.push_back(id);
        Item_spawn_data *isd = item_controller->get_group(id);
        if (isd == NULL) {
            debugmsg("unknown item spawn list %s", id.c_str());
            return item(null_item_id, birthday);
        }
        tmp = isd->create_single(birthday, rec);
        rec.erase( rec.end() - 1 );
    } else if (type == S_NONE) {
        return item(null_item_id, birthday);
    }
    if( one_in( 3 ) && tmp.has_flag( "VARSIZE" ) ) {
        tmp.item_tags.insert( "FIT" );
    }
    if (modifier.get() != NULL) {
        modifier->modify(tmp);
    }
    // TODO: change the spawn lists to contain proper references to containers
    tmp = tmp.in_its_container();
    return tmp;
}

Item_spawn_data::ItemList Single_item_creator::create(int birthday, RecursionList &rec) const
{
    ItemList result;
    int cnt = 1;
    if (modifier.get() != NULL) {
        cnt = (modifier->count.first == modifier->count.second) ? modifier->count.first : rng(
                  modifier->count.first, modifier->count.second);
    }
    for( ; cnt > 0; cnt--) {
        if (type == S_ITEM) {
            const auto itm = create_single( birthday, rec );
            if( !itm.is_null() ) {
                result.push_back( itm );
            }
        } else {
            if (std::find(rec.begin(), rec.end(), id) != rec.end()) {
                debugmsg("recursion in item spawn list %s", id.c_str());
                return result;
            }
            rec.push_back(id);
            Item_spawn_data *isd = item_controller->get_group(id);
            if (isd == NULL) {
                debugmsg("unknown item spawn list %s", id.c_str());
                return result;
            }
            ItemList tmplist = isd->create(birthday, rec);
            rec.erase( rec.end() - 1 );
            if (modifier.get() != NULL) {
                for( auto &elem : tmplist ) {
                    modifier->modify( elem );
                }
            }
            result.insert(result.end(), tmplist.begin(), tmplist.end());
        }
    }
    return result;
}

void Single_item_creator::check_consistency() const
{
    if (type == S_ITEM) {
        if( !item::type_is_defined( id ) ) {
            debugmsg("item id %s is unknown", id.c_str());
        }
    } else if (type == S_ITEM_GROUP) {
        if (!item_group::group_is_defined(id)) {
            debugmsg("item group id %s is unknown", id.c_str());
        }
    } else if (type == S_NONE) {
        // this is ok, it will be ignored
    } else {
        debugmsg("Unknown type of Single_item_creator: %d", (int) type);
    }
    if (modifier.get() != NULL) {
        modifier->check_consistency();
    }
}

bool Single_item_creator::remove_item(const Item_tag &itemid)
{
    if (modifier.get() != NULL) {
        if (modifier->remove_item(itemid)) {
            type = S_NONE;
            return true;
        }
    }
    if (type == S_ITEM) {
        if (itemid == id) {
            type = S_NONE;
            return true;
        }
    } else if (type == S_ITEM_GROUP) {
        Item_spawn_data *isd = item_controller->get_group(id);
        if (isd != NULL) {
            isd->remove_item(itemid);
        }
    }
    return type == S_NONE;
}

bool Single_item_creator::has_item(const Item_tag &itemid) const
{
    return type == S_ITEM && itemid == id;
}



Item_modifier::Item_modifier()
    : damage(0, 0)
    , count(1, 1)
    , charges(-1, -1)
    , ammo()
    , container()
{
}

Item_modifier::~Item_modifier()
{
}

void Item_modifier::modify(item &new_item) const
{

    if(new_item.is_null()) {
        return;
    }
    int dm = (damage.first == damage.second) ? damage.first : rng(damage.first, damage.second);
    if(dm >= -1 && dm <= 4) {
        new_item.damage = dm;
    }
    long ch = (charges.first == charges.second) ? charges.first : rng(charges.first, charges.second);
    const auto g = new_item.type->gun.get();
    const auto t = dynamic_cast<const it_tool *>(new_item.type);
   
    if(ch != -1) {
        if( new_item.count_by_charges() || new_item.made_of( LIQUID ) ) {
            // food, ammo
            // count_by_charges requires that charges is at least 1. It makes no sense to
            // spawn a "water (0)" item.
            new_item.charges = std::max( 1l, ch );
        } else if(t != NULL) {
            new_item.charges = std::min(ch, t->max_charges);
        } else if (g == nullptr){
            //not gun, food, ammo or tool. 
            new_item.charges = ch;
        }
    }
    
    if( g != nullptr && ( ammo.get() != nullptr || ch > 0 ) ) {
        if( ammo.get() == nullptr ) {
            // In case there is no explicit ammo item defined, use the default ammo
            const auto ammoid = default_ammo( g->ammo );
            if ( !ammoid.empty() ) {
                new_item.set_curammo( ammoid );
                new_item.charges = ch;
            }
        } else {
            const item am = ammo->create_single( new_item.bday );
            new_item.set_curammo( am );
            // Prefer explicit charges of the gun, else take the charges of the ammo item,
            // Gun charges are easier to define: {"item":"gun","charge":10,"ammo-item":"ammo"}
            if( ch > 0 ) {
                new_item.charges = ch;
            } else {
                new_item.charges = am.charges;
            }
        }
        // Make sure the item is in a valid state curammo==0 <=> charges==0 and respect clip size
        if( !new_item.has_curammo() ) {
            new_item.charges = 0;
        } else {
            new_item.charges = std::min<long>( new_item.charges, new_item.clip_size() );
        }
    }
    if(container.get() != NULL) {
        item cont = container->create_single(new_item.bday);
        if (!cont.is_null()) {
            if (new_item.made_of(LIQUID)) {
                long rc = cont.get_remaining_capacity_for_liquid(new_item);
                if(rc > 0 && (new_item.charges > rc || ch == -1)) {
                    // make sure the container is not over-full.
                    // fill up the container (if using default charges)
                    new_item.charges = rc;
                }
            }
            cont.put_in(new_item);
            new_item = cont;
        }
    }
    if (contents.get() != NULL) {
        Item_spawn_data::ItemList contentitems = contents->create(new_item.bday);
        new_item.contents.insert(new_item.contents.end(), contentitems.begin(), contentitems.end());
    }
}

void Item_modifier::check_consistency() const
{
    if (ammo.get() != NULL) {
        ammo->check_consistency();
    }
    if (container.get() != NULL) {
        container->check_consistency();
    }
}

bool Item_modifier::remove_item(const Item_tag &itemid)
{
    if (ammo.get() != NULL) {
        if (ammo->remove_item(itemid)) {
            ammo.reset();
        }
    }
    if (container.get() != NULL) {
        if (container->remove_item(itemid)) {
            container.reset();
            return true;
        }
    }
    return false;
}



Item_group::Item_group(Type t, int probability)
    : Item_spawn_data(probability)
    , type(t)
    , sum_prob(0)
    , items()
    , with_ammo(false)
{
}

Item_group::~Item_group()
{
    for( auto &elem : items ) {
        delete elem;
    }
    items.clear();
}

void Item_group::add_item_entry(const Item_tag &itemid, int probability)
{
    std::unique_ptr<Item_spawn_data> ptr(new Single_item_creator(itemid, Single_item_creator::S_ITEM,
                                         probability));
    add_entry(ptr);
}

void Item_group::add_group_entry(const Group_tag &groupid, int probability)
{
    std::unique_ptr<Item_spawn_data> ptr(new Single_item_creator(groupid,
                                         Single_item_creator::S_ITEM_GROUP, probability));
    add_entry(ptr);
}

void Item_group::add_entry(std::unique_ptr<Item_spawn_data> &ptr)
{
    assert(ptr.get() != NULL);
    if (ptr->probability <= 0) {
        return;
    }
    if (type == G_COLLECTION) {
        ptr->probability = std::min(100, ptr->probability);
    }
    items.push_back(ptr.get());
    sum_prob += ptr->probability;
    ptr.release();
}

Item_spawn_data::ItemList Item_group::create(int birthday, RecursionList &rec) const
{
    ItemList result;
    if (type == G_COLLECTION) {
        for( const auto &elem : items ) {
            if( rng( 0, 99 ) >= ( elem )->probability ) {
                continue;
            }
            ItemList tmp = ( elem )->create( birthday, rec );
            result.insert(result.end(), tmp.begin(), tmp.end());
        }
    } else if (type == G_DISTRIBUTION) {
        int p = rng(0, sum_prob - 1);
        for( const auto &elem : items ) {
            p -= ( elem )->probability;
            if (p >= 0) {
                continue;
            }
            ItemList tmp = ( elem )->create( birthday, rec );
            result.insert(result.end(), tmp.begin(), tmp.end());
            break;
        }
    }
    if (with_ammo && !result.empty()) {
        const auto t = result.front().type;
        if( t->gun ) {
            const std::string ammoid = default_ammo( t->gun->ammo );
            if ( !ammoid.empty() ) {
                item ammo( ammoid, birthday );
                // TODO: change the spawn lists to contain proper references to containers
                ammo = ammo.in_its_container();
                result.push_back( ammo );
            }
        }
    }
    return result;
}

item Item_group::create_single(int birthday, RecursionList &rec) const
{
    if (type == G_COLLECTION) {
        for( const auto &elem : items ) {
            if( rng( 0, 99 ) >= ( elem )->probability ) {
                continue;
            }
            return ( elem )->create_single( birthday, rec );
        }
    } else if (type == G_DISTRIBUTION) {
        int p = rng(0, sum_prob - 1);
        for( const auto &elem : items ) {
            p -= ( elem )->probability;
            if (p >= 0) {
                continue;
            }
            return ( elem )->create_single( birthday, rec );
        }
    }
    return item(null_item_id, birthday);
}

void Item_group::check_consistency() const
{
    for( const auto &elem : items ) {
        ( elem )->check_consistency();
    }
}

bool Item_group::remove_item(const Item_tag &itemid)
{
    for(prop_list::iterator a = items.begin(); a != items.end(); ) {
        if ((*a)->remove_item(itemid)) {
            sum_prob -= (*a)->probability;
            delete *a;
            a = items.erase(a);
        } else {
            ++a;
        }
    }
    return items.empty();
}

bool Item_group::has_item(const Item_tag &itemid) const
{
    for( const auto &elem : items ) {
        if( ( elem )->has_item( itemid ) ) {
            return true;
        }
    }
    return false;
}

item_group::ItemList item_group::items_from( const Group_tag &group_id, int birthday )
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

item item_group::item_from( const Group_tag &group_id, int birthday )
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

void item_group::load_item_group( JsonObject &jsobj, const Group_tag &group_id,
                                  const std::string &subtype )
{
    item_controller->load_item_group( jsobj, group_id, subtype );
}

Group_tag get_unique_group_id()
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

Group_tag item_group::load_item_group( JsonIn& stream, const std::string& default_subtype )
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
        item_controller->load_item_group( jarr, group, default_subtype == "collection" );

        return group;
    } else {
        stream.error( "invalid item group, must be string (group id) or object/array (the group data)" );
        // stream.error always throws, this is here to prevent a warning
        return Group_tag{};
    }
}
