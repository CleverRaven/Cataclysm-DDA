#include <sstream>
#include "inventory.h"
#include "game.h"
#include "iuse.h"

const std::string inv_chars =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!\"#&()*+./:;=@[\\]^_{|}";

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

/*static*/ bool inventory::has_activation(const item &it, const player &u)
{
    return u.rate_action_use(&it) != HINT_CANT;
}

/*static*/ bool inventory::has_category(const item &it, item_cat cat, const player &u)
{
    switch (cat) {
    case IC_COMESTIBLE: // food
        if (it.is_food(&u) || it.is_food_container(&u)) {
            return true;
        }
        break;
    case IC_AMMO: // ammo
        if (it.is_ammo() || it.is_ammo_container()) {
            return true;
        }
        break;
    case IC_ARMOR: // armor
        if (it.is_armor()) {
            return true;
        }
        break;
    case IC_BOOK: // books
        if (it.is_book()) {
            return true;
        }
        break;
    case IC_TOOL: // tools
        if (it.is_tool()) {
            return true;
        }
        break;
    case IC_CONTAINER: // containers for liquid handling
        if (it.is_tool() || it.is_gun()) {
            if (it.ammo_type() == "gasoline") {
                return true;
            }
        } else {
            if (it.is_container()) {
                return true;
            }
        }
        break;
    case IC_GUN:
        if(it.is_gun()) {
            return true;
        }
        break;
    case IC_NULL:
        break;
    }
    return false;
}

/*static*/ bool inventory::has_capacity_for_liquid(const item &it, const item &liquid)
{
    LIQUID_FILL_ERROR error;
    return (it.get_remaining_capacity_for_liquid(liquid, error) > 0);
}

indexed_invslice inventory::slice_filter()
{
    int i = 0;
    indexed_invslice stacks;
    for( auto &elem : items ) {
        stacks.push_back( std::make_pair( &elem, i ) );
        ++i;
    }
    return stacks;
}

indexed_invslice inventory::slice_filter_by_activation(const player &u)
{
    int i = 0;
    indexed_invslice stacks;
    for( auto &elem : items ) {
        if( has_activation( elem.front(), u ) ) {
            stacks.push_back( std::make_pair( &elem, i ) );
        }
        ++i;
    }
    return stacks;
}

indexed_invslice inventory::slice_filter_by_category(item_cat cat, const player &u)
{
    int i = 0;
    indexed_invslice stacks;
    for( auto &elem : items ) {
        if( has_category( elem.front(), cat, u ) ) {
            stacks.push_back( std::make_pair( &elem, i ) );
        }
        ++i;
    }
    return stacks;
}

indexed_invslice inventory::slice_filter_by_flag(const std::string flag)
{
    int i = 0;
    indexed_invslice stacks;
    for( auto &elem : items ) {
        if( elem.front().has_flag( flag ) ) {
            stacks.push_back( std::make_pair( &elem, i ) );
        }
        ++i;
    }
    return stacks;
}

indexed_invslice inventory::slice_filter_by_capacity_for_liquid(const item &liquid)
{
    int i = 0;
    indexed_invslice stacks;
    for( auto &elem : items ) {
        if( has_capacity_for_liquid( elem.front(), liquid ) ) {
            stacks.push_back( std::make_pair( &elem, i ) );
        }
        ++i;
    }
    return stacks;
}

indexed_invslice inventory::slice_filter_by_salvageability()
{
    int i = 0;
    indexed_invslice stacks;
    for( auto &elem : items ) {
        if( iuse::valid_to_cut_up( &elem.front() ) ) {
            stacks.push_back( std::make_pair( &elem, i ) );
        }
        ++i;
    }
    return stacks;
}

void inventory::unsort()
{
    sorted = false;
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

    std::list<item> to_restack;
    int idx = 0;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter, ++idx) {
        const int ipos = p->invlet_to_position(iter->front().invlet);
        if (!iter->front().invlet_is_okay() || ( ipos != INT_MIN && ipos != idx ) ) {
            assign_empty_invlet(iter->front());
            for( std::list<item>::iterator stack_iter = iter->begin();
                 stack_iter != iter->end(); ++stack_iter ) {
                stack_iter->invlet = iter->front().invlet;
            }
        }

        // remove non-matching items, stripping off end of stack so the first item keeps the invlet.
        while( iter->size() > 1 && !iter->front().stacks_with(iter->back()) ) {
            to_restack.splice(to_restack.begin(), *iter, --iter->end());
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
}

extern long count_charges_in_list(const itype *type, const std::vector<item> &items);
void inventory::form_from_map(point origin, int range, bool assign_invlet)
{
    items.clear();
    for (int x = origin.x - range; x <= origin.x + range; x++) {
        for (int y = origin.y - range; y <= origin.y + range; y++) {
            if (g->m.has_furn(x, y) && g->m.accessable_furniture(origin.x, origin.y, x, y, range)) {
                const furn_t &f = g->m.furn_at(x, y);
                itype *type = f.crafting_pseudo_item_type();
                if (type != NULL) {
                    item furn_item(type->id, 0);
                    const itype *ammo = f.crafting_ammo_item_type();
                    if (ammo != NULL) {
                        furn_item.charges = count_charges_in_list(ammo, g->m.i_at(x, y));
                    }
                    furn_item.item_tags.insert("PSEUDO");
                    add_item(furn_item);
                }
            }
            if(g->m.accessable_items(origin.x, origin.y, x, y, range)) {
                continue;
            }
            for (auto &i : g->m.i_at(x, y)) {
                if (!i.made_of(LIQUID)) {
                    add_item(i, false, assign_invlet);
                }
            }
            // Kludges for now!
            ter_id terrain_id = g->m.ter(x, y);
            if (g->m.has_nearby_fire(x, y, 0)) {
                item fire("fire", 0);
                fire.charges = 1;
                add_item(fire);
            }
            if (terrain_id == t_water_sh || terrain_id == t_water_dp ||
                terrain_id == t_water_pool || terrain_id == t_water_pump) {
                item water("water", 0);
                water.charges = 50;
                add_item(water);
            }
            if (terrain_id == t_swater_sh || terrain_id == t_swater_dp) {
                item swater("salt_water", 0);
                swater.charges = 50;
                add_item(swater);
            }
            // add cvd forge from terrain
            if (terrain_id == t_cvdmachine) {
                item cvd_machine("cvd_machine", 0);
                cvd_machine.charges = 1;
                cvd_machine.item_tags.insert("PSEUDO");
                add_item(cvd_machine);
            }
            // kludge that can probably be done better to check specifically for toilet water to use in
            // crafting
            if (furnlist[g->m.furn(x, y)].examine == &iexamine::toilet) {
                // get water charges at location
                std::vector<item> toiletitems = g->m.i_at(x, y);
                int waterindex = -1;
                for (size_t i = 0; i < toiletitems.size(); ++i) {
                    if (toiletitems[i].typeId() == "water") {
                        waterindex = i;
                        break;
                    }
                }
                if (waterindex >= 0 && toiletitems[waterindex].charges > 0) {
                    add_item(toiletitems[waterindex]);
                }
            }

            // keg-kludge
            if (furnlist[g->m.furn(x, y)].examine == &iexamine::keg) {
                std::vector<item> liq_contained = g->m.i_at(x, y);
                for (auto &i : liq_contained)
                    if (i.made_of(LIQUID)) {
                        add_item(i);
                    }
            }

            int vpart = -1;
            vehicle *veh = g->m.veh_at(x, y, vpart);

            if (veh) {
                //Adds faucet to kitchen stuff; may be horribly wrong to do such....
                //ShouldBreak into own variable
                const int kpart = veh->part_with_feature(vpart, "KITCHEN");
                const int faupart = veh->part_with_feature(vpart, "FAUCET");
                const int weldpart = veh->part_with_feature(vpart, "WELDRIG");
                const int craftpart = veh->part_with_feature(vpart, "CRAFTRIG");
                const int forgepart = veh->part_with_feature(vpart, "FORGE");
                const int chempart = veh->part_with_feature(vpart, "CHEMLAB");
                const int cargo = veh->part_with_feature(vpart, "CARGO");

                if (cargo >= 0) {
                    *this += std::list<item>(veh->parts[cargo].items.begin(), veh->parts[cargo].items.end());
                }

                if(faupart >= 0 ) {
                    item water("water_clean", 0);
                    water.charges = veh->fuel_left("water");
                    add_item(water);
                }

                if (kpart >= 0) {
                    item hotplate("hotplate", 0);
                    hotplate.charges = veh->fuel_left("battery");
                    hotplate.item_tags.insert("PSEUDO");
                    add_item(hotplate);

                    item water("water_clean", 0);
                    water.charges = veh->fuel_left("water");
                    add_item(water);

                    item pot("pot", 0);
                    pot.item_tags.insert("PSEUDO");
                    add_item(pot);
                    item pan("pan", 0);
                    pan.item_tags.insert("PSEUDO");
                    add_item(pan);
                }
                if (weldpart >= 0) {
                    item welder("welder", 0);
                    welder.charges = veh->fuel_left("battery");
                    welder.item_tags.insert("PSEUDO");
                    add_item(welder);

                    item soldering_iron("soldering_iron", 0);
                    soldering_iron.charges = veh->fuel_left("battery");
                    soldering_iron.item_tags.insert("PSEUDO");
                    add_item(soldering_iron);
                }
                if (craftpart >= 0) {
                    item vac_sealer("vac_sealer", 0);
                    vac_sealer.charges = veh->fuel_left("battery");
                    vac_sealer.item_tags.insert("PSEUDO");
                    add_item(vac_sealer);

                    item dehydrator("dehydrator", 0);
                    dehydrator.charges = veh->fuel_left("battery");
                    dehydrator.item_tags.insert("PSEUDO");
                    add_item(dehydrator);

                    item press("press", 0);
                    press.charges = veh->fuel_left("battery");
                    press.item_tags.insert("PSEUDO");
                    add_item(press);
                }
                if (forgepart >= 0) {
                    item forge("forge", 0);
                    forge.charges = veh->fuel_left("battery");
                    forge.item_tags.insert("PSEUDO");
                    add_item(forge);
                }
                if (chempart >= 0) {
                    item hotplate("hotplate", 0);
                    hotplate.charges = veh->fuel_left("battery");
                    hotplate.item_tags.insert("PSEUDO");
                    add_item(hotplate);

                    item chemistry_set("chemistry_set", 0);
                    chemistry_set.charges = veh->fuel_left("battery");
                    chemistry_set.item_tags.insert("PSEUDO");
                    add_item(chemistry_set);
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
    auto tmp = remove_items_with( [&it] (const item &i) { return &i == it; } );
    if( !tmp.empty() ) {
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

void inventory::dump(std::vector<item *> &dest)
{
    for( auto &elem : items ) {
        for( auto &elem_stack_iter : elem ) {
            dest.push_back( &( elem_stack_iter ) );
        }
    }
}

item &inventory::find_item(int position)
{
    if (position < 0 || position >= (int)items.size()) {
        return nullitem;
    }
    invstack::iterator iter = items.begin();
    for (int j = 0; j < position; ++j) {
        ++iter;
    }
    return iter->front();
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

int inventory::position_by_item(const item *it)
{
    int i = 0;
    for( auto &elem : items ) {
        for( auto &elem_stack_iter : elem ) {
            if( it == &elem_stack_iter ) {
                return i;
            }
        }
        ++i;
    }
    return INT_MIN;
}

item &inventory::item_by_type(itype_id type)
{
    for( auto &elem : items ) {
        if( elem.front().type->id == type ) {
            return elem.front();
        }
    }
    return nullitem;
}

int inventory::position_by_type(itype_id type)
{
    int i = 0;
    for( auto &elem : items ) {
        if( elem.front().type->id == type ) {
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
            if( elem_stack_iter.type->id == type ) {
                return elem_stack_iter;
            } else if( elem_stack_iter.is_container() && !elem_stack_iter.contents.empty() ) {
                if( elem_stack_iter.contents[0].type->id == type ) {
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
            if( elem_stack_iter.type->id == type ) {
                ret.push_back( std::make_pair( &elem_stack_iter, i ) );
            }
        }
        ++i;
    }
    return ret;
}

std::vector<item *> inventory::all_ammo(const ammotype &type)
{
    std::vector<item *> ret;
    for( auto &elem : items ) {
        for( auto &elem_stack_iter : elem ) {
            if( elem_stack_iter.is_of_ammo_type_or_contains_it( type ) ) {
                ret.push_back( &elem_stack_iter );
            }
        }
    }
    return ret;
}

int inventory::amount_of(itype_id it) const
{
    return amount_of(it, true);
}

int inventory::amount_of(itype_id it, bool used_as_tool) const
{
    int count = 0;
    for( const auto &elem : items ) {
        for( const auto &elem_stack_iter : elem ) {
            count += elem_stack_iter.amount_of( it, used_as_tool );
        }
    }
    return count;
}

long inventory::charges_of(itype_id it) const
{
    int count = 0;
    for( const auto &elem : items ) {
        for( const auto &elem_stack_iter : elem ) {
            count += elem_stack_iter.charges_of( it );
        }
    }
    return count;
}

std::list<item> inventory::use_amount(itype_id it, int quantity, bool use_container)
{
    sort();
    std::list<item> ret;
    for (invstack::iterator iter = items.begin(); iter != items.end() && quantity > 0; /* noop */) {
        for (std::list<item>::iterator stack_iter = iter->begin();
             stack_iter != iter->end() && quantity > 0;
             /* noop */) {
            if (stack_iter->use_amount(it, quantity, use_container, ret)) {
                stack_iter = iter->erase(stack_iter);
            } else {
                ++stack_iter;
            }
        }
        if (iter->empty()) {
            iter = items.erase(iter);
        } else if (iter != items.end()) {
            ++iter;
        }
    }
    return ret;
}

std::list<item> inventory::use_charges(itype_id it, long quantity)
{
    sort();
    std::list<item> ret;
    for (invstack::iterator iter = items.begin(); iter != items.end() && quantity > 0; /* noop */) {
        for (std::list<item>::iterator stack_iter = iter->begin();
             stack_iter != iter->end() && quantity > 0; /* noop */) {
            if (stack_iter->use_charges(it, quantity, ret)) {
                stack_iter = iter->erase(stack_iter);
            } else {
                ++stack_iter;
            }
        }
        if (iter->empty()) {
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

bool inventory::has_amount(itype_id it, int quantity) const
{
    return has_amount(it, quantity, true);
}

bool inventory::has_amount(itype_id it, int quantity, bool used_as_tool) const
{
    return (amount_of(it, used_as_tool) >= quantity);
}

bool inventory::has_charges(itype_id it, long quantity) const
{
    return (charges_of(it) >= quantity);
}

bool inventory::has_item( const item *it ) const
{
    return has_item_with( [&it]( const item & i ) {
        return &i == it;
    } );
}

bool inventory::has_items_with_quality(std::string id, int level, int amount) const
{
    int found = 0;
    for( const auto &elem : items ) {
        for( const auto &elem_stack_iter : elem ) {
            if( !elem_stack_iter.contents.empty() && elem_stack_iter.is_container() ) {
                continue;
            }
            if( elem_stack_iter.has_quality( id, level ) ) {
                found++;
            }
        }
    }
    if(found >= amount) {
        return true;
    } else {
        return false;
    }
}

int inventory::leak_level(std::string flag) const
{
    int ret = 0;

    for( const auto &elem : items ) {
        for( const auto &elem_stack_iter : elem ) {
            if( elem_stack_iter.has_flag( flag ) ) {
                if( elem_stack_iter.has_flag( "LEAK_ALWAYS" ) ) {
                    ret += elem_stack_iter.volume();
                } else if( elem_stack_iter.has_flag( "LEAK_DAM" ) && elem_stack_iter.damage > 0 ) {
                    ret += elem_stack_iter.damage;
                }
            }
        }
    }
    return ret;
}

int inventory::butcher_factor() const
{
    int result = INT_MIN;
    for( const auto &elem : items ) {
        for( const auto &cur_item : elem ) {

            result = std::max( result, cur_item.butcher_factor() );
        }
    }
    return result;
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
        if ( (pain <= 35 && it.type->id == "aspirin") ||
             (pain >= 50 && it.type->id == "oxycodone") ||
             it.type->id == "tramadol" || it.type->id == "codeine") {
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
        itype_id type = elem.front().type->id;
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

item *inventory::best_for_melee(player *p)
{
    item *ret = &nullitem;
    int best = 0;
    for( auto &elem : items ) {
        int score = elem.front().melee_value( p );
        if (score > best) {
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
        if( elem.front().is_gun() && elem.front().charges > max ) {
            ret = &( elem.front() );
            max = ret->charges;
        }
    }
    return ret;
}

void inventory::rust_iron_items()
{
    for( auto &elem : items ) {
        for( auto &elem_stack_iter : elem ) {
            if( elem_stack_iter.made_of( "iron" ) &&
                !elem_stack_iter.has_flag( "WATERPROOF_GUN" ) &&
                !elem_stack_iter.has_flag( "WATERPROOF" ) && elem_stack_iter.damage < 5 &&
                one_in( 500 ) ) {
                elem_stack_iter.damage++;
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

int inventory::volume() const
{
    int ret = 0;
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
                  elem_stack_iter.contents[0].active ) ) {
                ret.push_back( &elem_stack_iter );
            }
        }
    }
    return ret;
}

void inventory::assign_empty_invlet(item &it, bool force)
{
    if( !OPTIONS["AUTO_INV_ASSIGN"] ) {
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
