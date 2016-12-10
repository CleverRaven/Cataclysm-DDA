#include <sstream>
#include "inventory.h"
#include "game.h"
#include "map.h"
#include "debug.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "options.h"
#include "npc.h"
#include "itype.h"
#include "vehicle.h"
#include "veh_type.h"
#include "mapdata.h"
#include "map_iterator.h"
#include <algorithm>

const invlet_wrapper inv_chars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!\"#&()*+./:;=@[\\]^_{|}");

bool invlet_wrapper::valid( const long invlet ) const
{
    if( invlet > std::numeric_limits<char>::max() || invlet < std::numeric_limits<char>::min() ) {
        return false;
    }
    return find( static_cast<char>( invlet ) ) != std::string::npos;
}

inventory::inventory()
: nullitem()
, nullstack()
, invlet_cache()
, items()
, sorted(false)
{
}

invslice inventory::slice()
{
    invslice stacks;
    for( auto &elem : items ) {
        stacks.push_back( &elem );
    }
    return stacks;
}

const_invslice inventory::const_slice() const
{
    const_invslice stacks;
    for( auto iter = items.cbegin(); iter != items.cend(); ++iter) {
        stacks.push_back(&*iter);
    }
    return stacks;
}

const std::list<item> &inventory::const_stack(int i) const
{
    if (i < 0 || i >= (int)items.size()) {
        debugmsg("Attempted to access stack %d in an inventory (size %d)", i, items.size());
        return nullstack;
    }

    invstack::const_iterator iter = items.begin();
    for (int j = 0; j < i; ++j) {
        ++iter;
    }
    return *iter;
}

size_t inventory::size() const
{
    return items.size();
}

int inventory::num_items() const
{
    int ret = 0;
    for( const auto &elem : items ) {
        ret += elem.size();
    }
    return ret;
}

bool inventory::is_sorted() const
{
    return sorted;
}

inventory &inventory::operator+= (const inventory &rhs)
{
    for (size_t i = 0; i < rhs.size(); i++) {
        add_stack(rhs.const_stack(i));
    }
    return *this;
}

inventory &inventory::operator+= (const std::list<item> &rhs)
{
    for( const auto &rh : rhs ) {
        add_item( rh, false, false );
    }
    return *this;
}

inventory &inventory::operator+= (const std::vector<item> &rhs)
{
    for( const auto &rh : rhs ) {
        add_item( rh, true );
    }
    return *this;
}

inventory &inventory::operator+= (const item &rhs)
{
    add_item(rhs);
    return *this;
}

inventory inventory::operator+ (const inventory &rhs)
{
    return inventory(*this) += rhs;
}

inventory inventory::operator+ (const std::list<item> &rhs)
{
    return inventory(*this) += rhs;
}

inventory inventory::operator+ (const item &rhs)
{
    return inventory(*this) += rhs;
}

void inventory::unsort()
{
    sorted = false;
    binned = false;
}

bool stack_compare(const std::list<item> &lhs, const std::list<item> &rhs)
{
    return lhs.front() < rhs.front();
}

void inventory::sort()
{
    items.sort(stack_compare);
    sorted = true;
}

void inventory::clear()
{
    items.clear();
    binned = false;
}

void inventory::add_stack(const std::list<item> newits)
{
    for( const auto &newit : newits ) {
        add_item( newit, true );
    }
}

/*
 *  Bypass troublesome add_item for situations where we want an -exact- copy.
 */
void inventory::clone_stack (const std::list<item> &rhs)
{
    std::list<item> newstack;
    for( const auto &rh : rhs ) {
        newstack.push_back( rh );
    }
    items.push_back(newstack);
    binned = false;
}

void inventory::push_back(std::list<item> newits)
{
    add_stack(newits);
}

// This function keeps the invlet cache updated when a new item is added.
void inventory::update_cache_with_item(item &newit)
{
    // This function does two things:
    // 1. It adds newit's invlet to the list of favorite letters for newit's item type.
    // 2. It removes newit's invlet from the list of favorite letters for all other item types.

    // no invlet item, just return.
    // TODO: Should we instead remember that the invlet was cleared?
    if (newit.invlet == 0) {
        return;
    }
    // Iterator over all the keys of the map.
    std::map<std::string, std::vector<char> >::iterator i;
    for(i = invlet_cache.begin(); i != invlet_cache.end(); i++) {
        std::string type = i->first;
        std::vector<char> &preferred_invlets = i->second;

        if( newit.typeId() != type) {
            // Erase the used invlet from all caches.
            for( size_t ind = 0; ind < preferred_invlets.size(); ++ind ) {
                if(preferred_invlets[ind] == newit.invlet) {
                    preferred_invlets.erase(preferred_invlets.begin() + ind);
                    ind--;
                }
            }
        }
    }

    // Append the selected invlet to the list of preferred invlets of this item type.
    std::vector<char> &pref = invlet_cache[newit.typeId()];
    if( std::find(pref.begin(), pref.end(), newit.invlet) == pref.end() ) {
        pref.push_back(newit.invlet);
    }
}

char inventory::find_usable_cached_invlet(const std::string &item_type)
{
    if( ! invlet_cache.count(item_type) ) {
        return 0;
    }

    // Some of our preferred letters might already be used.
    for( auto invlet : invlet_cache[item_type] ) {
        // Don't overwrite user assignments.
        if( g->u.assigned_invlet.count(invlet) ) {
            continue;
        }
        if( g->u.weapon.invlet == invlet ) {
            continue;
        }
        // Check if anything is using this invlet.
        bool invlet_is_used = false;
        for( auto &elem : items ) {
            if( elem.front().invlet == invlet ) {
                invlet_is_used = true;
                break;
            }
        }
        if( !invlet_is_used ) {
            return invlet;
        }
    }

    return 0;
}

item &inventory::add_item(item newit, bool keep_invlet, bool assign_invlet)
{
    binned = false;
    bool reuse_cached_letter = false;

    // Avoid letters that have been manually assigned to other things.
    if( !keep_invlet && g->u.assigned_invlet.count(newit.invlet) ) {
        newit.invlet = '\0';
    }

    // Check how many stacks of this type already are in our inventory.
    if(!keep_invlet && assign_invlet) {
        // Do we have this item in our inventory favourites cache?
        char temp_invlet = find_usable_cached_invlet(newit.typeId());
        if( temp_invlet != 0 ) {
            newit.invlet = temp_invlet;
            reuse_cached_letter = true;
        }

        // If it's not in our cache and not a lowercase letter, try to give it a low letter.
        if(!reuse_cached_letter && (newit.invlet < 'a' || newit.invlet > 'z')) {
            assign_empty_invlet(newit);
        }

        // Make sure the assigned invlet doesn't exist already.
        if(this == &g->u.inv && g->u.invlet_to_position(newit.invlet) != INT_MIN) {
            assign_empty_invlet(newit);
        }
    }


    // See if we can't stack this item.
    for( auto &elem : items ) {
        std::list<item>::iterator it_ref = elem.begin();
        if( it_ref->stacks_with( newit ) ) {
            if( it_ref->merge_charges( newit ) ) {
                return *it_ref;
            }
            newit.invlet = it_ref->invlet;
            elem.push_back( newit );
            return elem.back();
        } else if( keep_invlet && assign_invlet && it_ref->invlet == newit.invlet ) {
            // If keep_invlet is true, we'll be forcing other items out of their current invlet.
            assign_empty_invlet(*it_ref);
        }
    }

    // Couldn't stack the item, proceed.
    if(!reuse_cached_letter) {
        update_cache_with_item(newit);
    }

    std::list<item> newstack;
    newstack.push_back(newit);
    items.push_back(newstack);
    return items.back().back();
}

void inventory::add_item_keep_invlet(item newit)
{
    add_item(newit, true);
}

void inventory::push_back(item newit)
{
    add_item(newit);
}


void inventory::restack(player *p)
{
    // tasks that the old restack seemed to do:
    // 1. reassign inventory letters
    // 2. remove items from non-matching stacks
    // 3. combine matching stacks

    if (!p) {
        return;
    }

    binned = false;
    std::list<item> to_restack;
    int idx = 0;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter, ++idx) {
        std::list<item> &stack = *iter;
        item &topmost = stack.front();

        const int ipos = p->invlet_to_position(topmost.invlet);
        if( !inv_chars.valid( topmost.invlet ) || ( ipos != INT_MIN && ipos != idx ) ) {
            assign_empty_invlet(topmost);
            for( std::list<item>::iterator stack_iter = stack.begin();
                 stack_iter != stack.end(); ++stack_iter ) {
                stack_iter->invlet = topmost.invlet;
            }
        }

        // remove non-matching items, stripping off end of stack so the first item keeps the invlet.
        while( stack.size() > 1 && !topmost.stacks_with(stack.back()) ) {
            to_restack.splice(to_restack.begin(), *iter, --stack.end());
        }
    }

    // combine matching stacks
    // separate loop to ensure that ALL stacks are homogeneous
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter) {
        for (invstack::iterator other = iter; other != items.end(); ++other) {
            if (iter != other && iter->front().stacks_with( other->front() ) ) {
                if( other->front().count_by_charges() ) {
                    iter->front().charges += other->front().charges;
                } else {
                    iter->splice(iter->begin(), *other);
                }
                other = items.erase(other);
                --other;
            }
        }
    }

    //re-add non-matching items
    for( auto &elem : to_restack ) {
        add_item( elem );
    }

    //Ensure that all items in the same stack have the same invlet.
    for( std::list< item > &outer : items ) {
        for( item &inner : outer ) {
            inner.invlet = outer.front().invlet;
        }
    }
}

static long count_charges_in_list(const itype *type, const map_stack &items)
{
    for( const auto &candidate : items ) {
        if( candidate.type == type ) {
            return candidate.charges;
        }
    }
    return 0;
}

void inventory::form_from_map( const tripoint &origin, int range, bool assign_invlet )
{
    items.clear();

    std::set<vehicle *> vehs;

    for( const tripoint &p : g->m.points_in_radius( origin, range ) ) {
        if (g->m.has_furn( p ) && g->m.accessible_furniture( origin, p, range )) {
            const furn_t &f = g->m.furn( p ).obj();
            const itype *type = f.crafting_pseudo_item_type();
            if (type != NULL) {
                const itype *ammo = f.crafting_ammo_item_type();
                item furn_item( type, calendar::turn, ammo ? count_charges_in_list( ammo, g->m.i_at( p ) ) : 0 );
                furn_item.item_tags.insert("PSEUDO");
                add_item(furn_item);
            }
        }
        if( !g->m.accessible_items( origin, p, range ) ) {
            continue;
        }
        for (auto &i : g->m.i_at( p )) {
            if (!i.made_of(LIQUID)) {
                add_item(i, false, assign_invlet);
            }
        }
        // Kludges for now!
        if (g->m.has_nearby_fire( p, 0 )) {
            item fire("fire", 0);
            fire.charges = 1;
            add_item(fire);
        }
        // Handle any water from infinite map sources.
        item water = g->m.water_from( p );
        if( !water.is_null() ) {
            add_item( water );
        }
        // kludge that can probably be done better to check specifically for toilet water to use in
        // crafting
        if (g->m.furn( p ).obj().examine == &iexamine::toilet) {
            // get water charges at location
            auto toilet = g->m.i_at( p );
            auto water = toilet.end();
            for( auto candidate = toilet.begin(); candidate != toilet.end(); ++candidate ) {
                if( candidate->typeId() == "water" ) {
                    water = candidate;
                    break;
                }
            }
            if( water != toilet.end() && water->charges > 0) {
                add_item( *water );
            }
        }

        // keg-kludge
        if (g->m.furn( p ).obj().examine == &iexamine::keg) {
            auto liq_contained = g->m.i_at( p );
            for( auto &i : liq_contained ) {
                if( i.made_of(LIQUID) ) {
                    add_item(i);
                }
            }
        }

        int vpart = -1;
        vehicle *veh = g->m.veh_at( p, vpart );

        if( veh == nullptr ) {
            continue;
        }

        vehs.insert( veh );

        const int cargo = veh->part_with_feature(vpart, "CARGO");
        if (cargo >= 0) {
            *this += std::list<item>( veh->get_items(cargo).begin(),
                                      veh->get_items(cargo).end() );
        }

        for( const auto pt : veh->get_parts( p ) ) {
            // does any part on this tile provide any pseudo-tools?
            for( const auto &e : pt->info().tools ) {
                item obj( e );
                obj.ammo_set( obj.ammo_default(), veh->fuel_left( obj.ammo_default() ) );
                add_item( obj.set_flag( "PSEUDO" ) );
            }
        }
    }

    for( const vehicle *v : vehs ) {
        // if vehicle has FAUCET can use contents from any of the tanks
        if( v->has_part( "FAUCET" ) ) {
            for( const auto &pt : v->parts ) {
                if( pt.is_tank() ) {
                    for( const auto &obj : pt.contents() ) {
                        add_item( obj );
                    }
                }
            }
        }
    }
}

template<typename Locator>
std::list<item> inventory::reduce_stack_internal(const Locator &locator, int quantity)
{
    int pos = 0;
    std::list<item> ret;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter) {
        if (item_matches_locator(iter->front(), locator, pos)) {
            binned = false;
            if(quantity >= (int)iter->size() || quantity < 0) {
                ret = *iter;
                items.erase(iter);
            } else {
                for(int i = 0 ; i < quantity ; i++) {
                    ret.push_back(remove_item(&iter->front()));
                }
            }
            break;
        }
        ++pos;
    }
    return ret;
}

// Instantiate for each type of Locator.
std::list<item> inventory::reduce_stack(int position, int quantity)
{
    return reduce_stack_internal(position, quantity);
}

std::list<item> inventory::reduce_stack(const itype_id &type, int quantity)
{
    return reduce_stack_internal(type, quantity);
}

item inventory::remove_item(const item *it)
{
    auto tmp = remove_items_with( [&it](const item& i) { return &i == it; }, 1 );
    if( !tmp.empty() ) {
        binned = false;
        return tmp.front();
    }
    debugmsg("Tried to remove a item not in inventory (name: %s)", it->tname().c_str());
    return nullitem;
}

template<typename Locator>
item inventory::remove_item_internal(const Locator &locator)
{
    int pos = 0;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter) {
        if (item_matches_locator(iter->front(), locator, pos)) {
            binned = false;
            if (iter->size() > 1) {
                std::list<item>::iterator stack_member = iter->begin();
                char invlet = stack_member->invlet;
                ++stack_member;
                stack_member->invlet = invlet;
            }
            item ret = iter->front();
            iter->erase(iter->begin());
            if (iter->empty()) {
                items.erase(iter);
            }
            return ret;
        }
        ++pos;
    }

    return nullitem;
}

// Instantiate for each type of Locator.
item inventory::remove_item(int position)
{
    return remove_item_internal(position);
}

std::list<item> inventory::remove_randomly_by_volume( const units::volume &volume )
{
    std::list<item> result;
    units::volume volume_dropped = 0;
    while( volume_dropped < volume ) {
        units::volume cumulative_volume = 0;
        auto chosen_stack = items.begin();
        auto chosen_item = chosen_stack->begin();
        for( auto stack = items.begin(); stack != items.end(); ++stack ) {
            for( auto stack_it = stack->begin(); stack_it != stack->end(); ++stack_it ) {
                cumulative_volume += stack_it->volume();
                if( x_in_y( stack_it->volume().value(), cumulative_volume.value() ) ) {
                    chosen_item = stack_it;
                    chosen_stack = stack;
                }
            }
        }
        volume_dropped += chosen_item->volume();
        result.push_back( std::move( *chosen_item ) );
        chosen_item = chosen_stack->erase( chosen_item );
        if( chosen_item == chosen_stack->begin() && !chosen_stack->empty() ) {
            // preserve the invlet when removing the first item of a stack
            chosen_item->invlet = result.back().invlet;
        }
        if( chosen_stack->empty() ) {
            binned = false;
            items.erase( chosen_stack );
        }
    }
    return result;
}

void inventory::dump(std::vector<item *> &dest)
{
    for( auto &elem : items ) {
        for( auto &elem_stack_iter : elem ) {
            dest.push_back( &( elem_stack_iter ) );
        }
    }
}

const item &inventory::find_item(int position) const
{
    if (position < 0 || position >= (int)items.size()) {
        return nullitem;
    }
    invstack::const_iterator iter = items.begin();
    for (int j = 0; j < position; ++j) {
        ++iter;
    }
    return iter->front();
}

item &inventory::find_item(int position)
{
    return const_cast<item&>( const_cast<const inventory*>(this)->find_item( position ) );
}

int inventory::invlet_to_position( char invlet ) const
{
    int i = 0;
    for( const auto &elem : items ) {
        if( elem.begin()->invlet == invlet ) {
            return i;
        }
        ++i;
    }
    return INT_MIN;
}

int inventory::position_by_item( const item *it ) const
{
    int p = 0;
    for( const auto &stack : items ) {
        for( const auto &e : stack ) {
            if( e.has_item( *it ) ) {
                return p;
            }
        }
        p++;
    }
    return INT_MIN;
}

item &inventory::item_by_type(itype_id type)
{
    for( auto &elem : items ) {
        if( elem.front().typeId() == type ) {
            return elem.front();
        }
    }
    return nullitem;
}

int inventory::position_by_type(itype_id type)
{
    int i = 0;
    for( auto &elem : items ) {
        if( elem.front().typeId() == type ) {
            return i;
        }
        ++i;
    }
    return INT_MIN;
}
item &inventory::item_or_container(itype_id type)
{
    for( auto &elem : items ) {
        for( auto &elem_stack_iter : elem ) {
            if( elem_stack_iter.typeId() == type ) {
                return elem_stack_iter;
            } else if( elem_stack_iter.is_container() && !elem_stack_iter.contents.empty() ) {
                if( elem_stack_iter.contents.front().typeId() == type ) {
                    return elem_stack_iter;
                }
            }
        }
    }

    return nullitem;
}

std::vector<std::pair<item *, int> > inventory::all_items_by_type(itype_id type)
{
    std::vector<std::pair<item *, int> > ret;
    int i = 0;
    for( auto &elem : items ) {
        for( auto &elem_stack_iter : elem ) {
            if( elem_stack_iter.typeId() == type ) {
                ret.push_back( std::make_pair( &elem_stack_iter, i ) );
            }
        }
        ++i;
    }
    return ret;
}

std::list<item> inventory::use_amount(itype_id it, int _quantity)
{
    long quantity = _quantity; // Don't wanny change the function signature right now
    sort();
    std::list<item> ret;
    for (invstack::iterator iter = items.begin(); iter != items.end() && quantity > 0; /* noop */) {
        for (std::list<item>::iterator stack_iter = iter->begin();
             stack_iter != iter->end() && quantity > 0;
             /* noop */) {
            if (stack_iter->use_amount(it, quantity, ret)) {
                stack_iter = iter->erase(stack_iter);
            } else {
                ++stack_iter;
            }
        }
        if (iter->empty()) {
            binned = false;
            iter = items.erase(iter);
        } else if (iter != items.end()) {
            ++iter;
        }
    }
    return ret;
}

bool inventory::has_tools(itype_id it, int quantity) const
{
    return has_amount(it, quantity, true);
}

bool inventory::has_components(itype_id it, int quantity) const
{
    return has_amount(it, quantity, false);
}

bool inventory::has_charges(itype_id it, long quantity) const
{
    return (charges_of(it) >= quantity);
}

int inventory::leak_level(std::string flag) const
{
    int ret = 0;

    for( const auto &elem : items ) {
        for( const auto &elem_stack_iter : elem ) {
            if( elem_stack_iter.has_flag( flag ) ) {
                if( elem_stack_iter.has_flag( "LEAK_ALWAYS" ) ) {
                    ret += elem_stack_iter.volume() / units::legacy_volume_factor;
                } else if( elem_stack_iter.has_flag( "LEAK_DAM" ) && elem_stack_iter.damage() > 0 ) {
                    ret += elem_stack_iter.damage();
                }
            }
        }
    }
    return ret;
}

int inventory::worst_item_value(npc *p) const
{
    int worst = 99999;
    for( const auto &elem : items ) {
        const item &it = elem.front();
        int val = p->value(it);
        if (val < worst) {
            worst = val;
        }
    }
    return worst;
}

bool inventory::has_enough_painkiller(int pain) const
{
    for( const auto &elem : items ) {
        const item &it = elem.front();
        if ( (pain <= 35 && it.typeId() == "aspirin") ||
             (pain >= 50 && it.typeId() == "oxycodone") ||
             it.typeId() == "tramadol" || it.typeId() == "codeine") {
            return true;
        }
    }
    return false;
}

item *inventory::most_appropriate_painkiller(int pain)
{
    int difference = 9999;
    item *ret = &nullitem;
    for( auto &elem : items ) {
        int diff = 9999;
        itype_id type = elem.front().typeId();
        if (type == "aspirin") {
            diff = abs(pain - 15);
        } else if (type == "codeine") {
            diff = abs(pain - 30);
        } else if (type == "oxycodone") {
            diff = abs(pain - 60);
        } else if (type == "heroin") {
            diff = abs(pain - 100);
        } else if (type == "tramadol") {
            diff = abs(pain - 40) / 2; // Bonus since it's long-acting
        }

        if (diff < difference) {
            difference = diff;
            ret = &( elem.front() );
        }
    }
    return ret;
}

item *inventory::best_for_melee( player &p, double &best )
{
    item *ret = &nullitem;
    for( auto &elem : items ) {
        auto score = p.melee_value( elem.front() );
        if( score > best ) {
            best = score;
            ret = &( elem.front() );
        }
    }

    return ret;
}

item *inventory::most_loaded_gun()
{
    item *ret = &nullitem;
    int max = 0;
    for( auto &elem : items ) {
        item &gun = elem.front();
        if( !gun.is_gun() ) {
            continue;
        }

        const auto required = gun.ammo_required();
        int cur = 0;
        if( required <= 0 ) {
            // Arbitrary
            cur = 5;
        } else {
            cur = gun.ammo_remaining() / required;
        }

        if( cur > max ) {
            ret = &gun;
            max = cur;
        }
    }
    return ret;
}

void inventory::rust_iron_items()
{
    for( auto &elem : items ) {
        for( auto &elem_stack_iter : elem ) {
            if( elem_stack_iter.made_of( material_id( "iron" ) ) &&
                !elem_stack_iter.has_flag( "WATERPROOF_GUN" ) &&
                !elem_stack_iter.has_flag( "WATERPROOF" ) && elem_stack_iter.damage() < elem_stack_iter.max_damage() &&
                one_in( 500 ) ) {
                elem_stack_iter.inc_damage( DT_ACID ); // rusting never completely destroys an item
            }
        }
    }
}

int inventory::weight() const
{
    int ret = 0;
    for( const auto &elem : items ) {
        for( const auto &elem_stack_iter : elem ) {
            ret += elem_stack_iter.weight();
        }
    }
    return ret;
}

units::volume inventory::volume() const
{
    units::volume ret = 0;
    for( const auto &elem : items ) {
        for( const auto &elem_stack_iter : elem ) {
            ret += elem_stack_iter.volume();
        }
    }
    return ret;
}

std::vector<item *> inventory::active_items()
{
    std::vector<item *> ret;
    for( auto &elem : items ) {
        for( auto &elem_stack_iter : elem ) {
            if( ( elem_stack_iter.is_artifact() && elem_stack_iter.is_tool() ) ||
                elem_stack_iter.active ||
                ( elem_stack_iter.is_container() && !elem_stack_iter.contents.empty() &&
                  elem_stack_iter.contents.front().active ) ) {
                ret.push_back( &elem_stack_iter );
            }
        }
    }
    return ret;
}

void inventory::assign_empty_invlet(item &it, bool force)
{
    if( !get_option<bool>( "AUTO_INV_ASSIGN" ) ) {
        return;
    }

    player *p = &(g->u);
    std::set<char> cur_inv = p->allocated_invlets();
    itype_id target_type = it.typeId();
    for (auto iter : p->assigned_invlet) {
        if (iter.second == target_type && !cur_inv.count(iter.first)) {
            it.invlet = iter.first;
            return;
        }
    }
    if (cur_inv.size() < inv_chars.size()) {
        for( const auto &inv_char : inv_chars ) {
            if( p->assigned_invlet.count(inv_char) ) {
                // don't overwrite assigned keys
                continue;
            }
            if( cur_inv.find( inv_char ) == cur_inv.end() ) {
                it.invlet = inv_char;
                return;
            }
        }
    }
    if (!force) {
        it.invlet = 0;
        return;
    }
    // No free hotkey exist, re-use some of the existing ones
    for( auto &elem : items ) {
        item &o = elem.front();
        if (o.invlet != 0) {
            it.invlet = o.invlet;
            o.invlet = 0;
            return;
        }
    }
    debugmsg("could not find a hotkey for %s", it.tname().c_str());
}

std::set<char> inventory::allocated_invlets() const
{
    std::set<char> invlets;
    for( const auto &stack : items ) {
        const char invlet = stack.front().invlet;
        if( invlet != 0 ) {
            invlets.insert( invlet );
        }
    }
    return invlets;
}

const itype_bin &inventory::get_binned_items() const
{
    if( binned ) {
        return binned_items;
    }

    binned_items.clear();

    // Hack warning
    inventory *this_nonconst = const_cast<inventory *>( this );
    this_nonconst->visit_items( [ this ]( item *e ) {
        binned_items[ e->typeId() ].push_back( e );
        return VisitResponse::NEXT;
    } );

    binned = true;
    return binned_items;
}
