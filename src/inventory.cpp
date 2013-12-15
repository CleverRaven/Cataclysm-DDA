#include <sstream>
#include "inventory.h"
#include "game.h"
#include "keypress.h"
#include "mapdata.h"
#include "item_factory.h"

const std::string inv_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!\"#&()*+./:;=?@[\\]^_{|}";

invslice inventory::slice() {
    invslice stacks;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        stacks.push_back(&*iter);
    }
    return stacks;
}

inventory inventory::subset(std::map<int, int> chosen) const
{
    int i = 0;
    inventory ret;
    for (invstack::const_iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        // don't need to worry about auto-creation of entries, as long as default == 0
        int count = chosen[i];
        if (count != 0)
        {
            if (iter->front().count_by_charges())
            {
                item tmp = iter->front();
                if (count <= tmp.charges && count >= 0) // -1 is used to mean "all" sometimes
                {
                    tmp.charges = count;
                }
                ret.add_item(tmp);
            }
            else
            {
                for (std::list<item>::const_iterator stack_iter = iter->begin(); stack_iter != iter->end(); ++stack_iter)
                {
                    ret.add_item(*stack_iter);
                    --count;
                    if (count <= 0)
                    {
                        break;
                    }
                }
            }
        }
        ++i;
    }
    return ret;
}

std::list<item>& inventory::stack_by_letter(char ch)
{
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        if (iter->begin()->invlet == ch)
        {
            return *iter;
        }
    }
    return nullstack;
}

const std::list<item>& inventory::const_stack(int i) const
{
    if (i < 0 || i >= items.size())
    {
        debugmsg("Attempted to access stack %d in an inventory (size %d)",
                 i, items.size());
        return nullstack;
    }

    invstack::const_iterator iter = items.begin();
    for (int j = 0; j < i; ++j)
    {
        ++iter;
    }
    return *iter;
}

int inventory::size() const
{
 return items.size();
}

int inventory::num_items() const
{
    int ret = 0;
    for (invstack::const_iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        ret += iter->size();
    }

    return ret;
}

bool inventory::is_sorted() const
{
    return sorted;
}

inventory& inventory::operator= (inventory &rhs)
{
 if (this == &rhs)
  return *this; // No self-assignment

 clear();
 for (int i = 0; i < rhs.size(); i++)
  items.push_back(rhs.const_stack(i));
 return *this;
}

inventory& inventory::operator= (const inventory &rhs)
{
 if (this == &rhs)
  return *this; // No self-assignment

 clear();
 for (int i = 0; i < rhs.size(); i++)
  items.push_back(rhs.const_stack(i));
 return *this;
}

inventory& inventory::operator+= (const inventory &rhs)
{
 for (int i = 0; i < rhs.size(); i++)
  add_stack(rhs.const_stack(i));
 return *this;
}

inventory& inventory::operator+= (const std::list<item> &rhs)
{
    for (std::list<item>::const_iterator iter = rhs.begin(); iter != rhs.end(); ++iter)
    {

        add_item(*iter, true);
    }
    return *this;
}

inventory& inventory::operator+= (const item &rhs)
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

/*static*/ bool inventory::has_activation(const item& it, const player& u) {
    return u.rate_action_use(&it) != HINT_CANT;
}

/*static*/ bool inventory::has_category(const item& it, item_cat cat, const player& u) {
    switch (cat)
    {
    case IC_COMESTIBLE: // food
        if (it.is_food(&u) || it.is_food_container(&u))
        {
             return true;
        }
        break;
    case IC_AMMO: // ammo
        if (it.is_ammo() || it.is_ammo_container())
        {
             return true;
        }
        break;
    case IC_ARMOR: // armour
        if (it.is_armor())
        {
             return true;
        }
        break;
    case IC_BOOK: // books
        if (it.is_book())
        {
             return true;
        }
        break;
    case IC_TOOL: // tools
        if (it.is_tool())
        {
             return true;
        }
        break;
    case IC_CONTAINER: // containers for liquid handling
        if (it.is_tool() || it.is_gun())
        {
            if (it.ammo_type() == "gasoline")
            {
                return true;
            }
        }
        else
        {
            if (it.is_container())
            {
                return true;
            }
        }
        break;
    }
    return false;
}

/*static*/ bool inventory::has_capacity_for_liquid(const item& it, const item& liquid) {
    LIQUID_FILL_ERROR error;
    return (it.get_remaining_capacity_for_liquid(liquid, error) > 0);
}

indexed_invslice inventory::slice_filter() {
    int i = 0;
    indexed_invslice stacks;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        stacks.push_back(std::make_pair(&*iter, i));
        ++i;
    }
    return stacks;
}

indexed_invslice inventory::slice_filter_by_activation(const player& u) {
    int i = 0;
    indexed_invslice stacks;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        if (has_activation(iter->front(), u)) {
            stacks.push_back(std::make_pair(&*iter, i));
        }
        ++i;
    }
    return stacks;
}

indexed_invslice inventory::slice_filter_by_category(item_cat cat, const player& u) {
    int i = 0;
    indexed_invslice stacks;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        if (has_category(iter->front(), cat, u)) {
            stacks.push_back(std::make_pair(&*iter, i));
        }
        ++i;
    }
    return stacks;
}

indexed_invslice inventory::slice_filter_by_capacity_for_liquid(const item &liquid) {
    int i = 0;
    indexed_invslice stacks;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter) {
        if (has_capacity_for_liquid(iter->front(), liquid)) {
            stacks.push_back(std::make_pair(&*iter, i));
        }
        ++i;
    }
    return stacks;
}

void inventory::unsort()
{
    sorted = false;
}

bool stack_compare(const std::list<item>& lhs, const std::list<item>& rhs)
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
    for (std::list<item>::const_iterator iter = newits.begin(); iter != newits.end(); ++iter)
    {
        add_item(*iter, true);
    }
}

/*
 *  Bypass troublesome add_item for situations where we want an -exact- copy.
 */
void inventory::clone_stack (const std::list<item> & rhs) {
    std::list<item> newstack;
    for (std::list<item>::const_iterator iter = rhs.begin(); iter != rhs.end(); ++iter) {
       newstack.push_back(*iter);
    }
    items.push_back(newstack);
//    return *this;
}

void inventory::push_back(std::list<item> newits)
{
 add_stack(newits);
}

// This function keeps the invlet cache updated when a new item is added.
void inventory::update_cache_with_item(item& newit) {
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
    for(i=invlet_cache.begin(); i!=invlet_cache.end(); i++) {
        std::string type = i->first;
        std::vector<char>& preferred_invlets = i->second;

        if( newit.typeId() != type){
            // Erase the used invlet from all caches.
            for(int ind=0; ind < preferred_invlets.size(); ind++) {
                if(preferred_invlets[ind] == newit.invlet) {
                    preferred_invlets.erase(preferred_invlets.begin()+ind);
                    ind--;
                }
            }
        }
    }

    // Append the selected invlet to the list of preferred invlets of this item type.
    std::vector<char>& preferred_invlets = invlet_cache[newit.typeId()];
    preferred_invlets.push_back(newit.invlet);
}

char inventory::get_invlet_for_item( std::string item_type ) {
    char candidate_invlet = 0;

    if( invlet_cache.count( item_type ) ) {
        std::vector<char>& preferred_invlets = invlet_cache[ item_type ];

        // Some of our preferred letters might already be used.
        int first_free_invlet = -1;
        for(int invlets_index = 0; invlets_index < preferred_invlets.size(); invlets_index++) {
            bool invlet_is_used = false; // Check if anything is using this invlet.
            if( g->u.weapon.invlet == preferred_invlets[ invlets_index ] ) {
                continue;
            }
            for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter) {
                if( iter->front().invlet == preferred_invlets[ invlets_index ] ) {
                    invlet_is_used = true;
                    break;
                }
            }

            // If we found one that isn't used, we're done iterating.
            if( !invlet_is_used ) {
                first_free_invlet = invlets_index;
                break;
            }
        }

        if( first_free_invlet != -1 ) {
            candidate_invlet = preferred_invlets[first_free_invlet];
        }
    }
    return candidate_invlet;
}


item& inventory::add_item(item newit, bool keep_invlet, bool assign_invlet)
{
//dprint("inv.add_item(%d): [%c] %s", keep_invlet, newit.invlet, newit.typeId().c_str()  );

    bool reuse_cached_letter = false;

    // Check how many stacks of this type already are in our inventory.

    if(!keep_invlet && assign_invlet) {
        // Do we have this item in our inventory favourites cache?
        char temp_invlet = get_invlet_for_item( newit.typeId() );
        if( temp_invlet != 0 ) {
            newit.invlet = temp_invlet;
            reuse_cached_letter = true;
        }

        // If it's not in our cache and not a lowercase letter, try to give it a low letter.
        if(!reuse_cached_letter && (newit.invlet < 'a' || newit.invlet > 'z')) {
            assign_empty_invlet(newit);
        }

        // Make sure the assigned invlet doesn't exist already.
        if(g->u.has_item(newit.invlet)) {
            assign_empty_invlet(newit);
        }
    }


    // See if we can't stack this item.
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        std::list<item>::iterator it_ref = iter->begin();
        if (it_ref->type->id == newit.type->id)
        {
            if (newit.charges != -1 && (newit.is_food() || newit.is_ammo()))
            {
                it_ref->charges += newit.charges;
                return *it_ref;
            }
            else if (it_ref->stacks_with(newit))
            {
                if (it_ref->is_food() && it_ref->has_flag("HOT"))
                {
                    int tmpcounter = (it_ref->item_counter + newit.item_counter) / 2;
                    it_ref->item_counter = tmpcounter;
                    newit.item_counter = tmpcounter;
                }
                newit.invlet = it_ref->invlet;
                iter->push_back(newit);
                return iter->back();
            }
            else if (keep_invlet && assign_invlet && it_ref->invlet == newit.invlet)
            {
                assign_empty_invlet(*it_ref);
            }
        }
        // If keep_invlet is true, we'll be forcing other items out of their current invlet.
        else if (keep_invlet && assign_invlet && it_ref->invlet == newit.invlet)
        {
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

void inventory::add_item_by_type(itype_id type, int count, int charges)
{
    // TODO add proper birthday
    while (count > 0)
    {
        item tmp = item_controller->create(type, 0);
        if (charges != -1)
        {
            tmp.charges = charges;
        }
        add_item(tmp);
        count--;
    }
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
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter) {
        if (!iter->front().invlet_is_okay() || p->has_weapon_or_armor(iter->front().invlet)) {
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
            if (iter != other && iter->front().type->id == other->front().type->id) {
                if (other->front().charges != -1 && (other->front().is_food() ||
                                                     other->front().is_ammo())) {
                    iter->front().charges += other->front().charges;
                } else if (iter->front().stacks_with(other->front())) {
                    iter->splice(iter->begin(), *other);
                } else {
                    continue;
                }
                other = items.erase(other);
                --other;
            }
        }
    }

    //re-add non-matching items
    for (std::list<item>::iterator iter = to_restack.begin(); iter != to_restack.end(); ++iter)
    {
        add_item(*iter);
    }
}

void inventory::form_from_map(game *g, point origin, int range, bool assign_invlet)
{
 items.clear();
 for (int x = origin.x - range; x <= origin.x + range; x++) {
  for (int y = origin.y - range; y <= origin.y + range; y++) {
   int junk;
   if (g->m.has_flag("SEALED", x, y) ||
       ((origin.x != x || origin.y != y) &&
        !g->m.clear_path( origin.x, origin.y, x, y, range, 1, 100, junk ) ) ) {
     continue;
   }
   for (int i = 0; i < g->m.i_at(x, y).size(); i++)
    if (!g->m.i_at(x, y)[i].made_of(LIQUID))
     add_item(g->m.i_at(x, y)[i], false, assign_invlet);
// Kludges for now!
   ter_id terrain_id = g->m.ter(x, y);
   if (g->m.has_nearby_fire(x, y, 0)) {
    item fire(itypes["fire"], 0);
    fire.charges = 1;
    add_item(fire);
   }
   if (terrain_id == t_water_sh || terrain_id == t_water_dp){
    item water(itypes["water"], 0);
    water.charges = 50;
    add_item(water);
   }
   // kludge that can probably be done better to check specifically for toilet water to use in
   // crafting
   if (furnlist[g->m.furn(x,y)].examine == &iexamine::toilet){
    // get water charges at location
     std::vector<item> toiletitems = g->m.i_at(x,y);
     int waterindex = -1;
     for (int i = 0; i < toiletitems.size(); ++i){
        if (toiletitems[i].typeId() == "water"){
            waterindex = i;
            break;
        }
     }
     if (waterindex >= 0 && toiletitems[waterindex].charges > 0){
        add_item(toiletitems[waterindex]);
     }

   }

   int vpart = -1;
   vehicle *veh = g->m.veh_at(x, y, vpart);

   if (veh) {
     const int kpart = veh->part_with_feature(vpart, "KITCHEN");
     const int weldpart = veh->part_with_feature(vpart, "WELDRIG");
     const int craftpart = veh->part_with_feature(vpart, "CRAFTRIG");
     const int forgepart = veh->part_with_feature(vpart, "FORGE");
     const int chempart = veh->part_with_feature(vpart, "CHEMLAB");

     if (kpart >= 0) {
       item hotplate(itypes["hotplate"], 0);
       hotplate.charges = veh->fuel_left("battery", true);
       add_item(hotplate);

       item water(itypes["water_clean"], 0);
       water.charges = veh->fuel_left("water");
       add_item(water);

       item pot(itypes["pot"], 0);
       add_item(pot);
       item pan(itypes["pan"], 0);
       add_item(pan);
       }
     if (weldpart >= 0) {
       item welder(itypes["welder"], 0);
       welder.charges = veh->fuel_left("battery", true);
       add_item(welder);

       item soldering_iron(itypes["soldering_iron"], 0);
       soldering_iron.charges = veh->fuel_left("battery", true);
       add_item(soldering_iron);
       }
     if (craftpart >= 0) {
       item vac_sealer(itypes["vac_sealer"], 0);
       vac_sealer.charges = veh->fuel_left("battery", true);
       add_item(vac_sealer);

       item dehydrator(itypes["dehydrator"], 0);
       dehydrator.charges = veh->fuel_left("battery", true);
       add_item(dehydrator);

       item press(itypes["press"], 0);
       press.charges = veh->fuel_left("battery", true);
       add_item(press);
       }
     if (forgepart >= 0) {
       item forge(itypes["forge"], 0);
       forge.charges = veh->fuel_left("battery", true);
       add_item(forge);
       }
     if (chempart >= 0) {
       item hotplate(itypes["hotplate"], 0);
       hotplate.charges = veh->fuel_left("battery", true);
       add_item(hotplate);

       item chemistry_set(itypes["chemistry_set"], 0);
       chemistry_set.charges = veh->fuel_left("battery", true);
       add_item(chemistry_set);
       }
     }
   }

  }
 }

template<typename Locator>
std::list<item> inventory::reduce_stack_internal(const Locator& locator, int quantity)
{
    int pos = 0;
    std::list<item> ret;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        if (item_matches_locator(iter->front(), locator, pos))
        {
            if(quantity >= iter->size() || quantity < 0)
            {
                ret = *iter;
                items.erase(iter);
            }
            else
            {
                for(int i = 0 ; i < quantity ; i++)
                {
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
std::list<item> inventory::reduce_stack(int position, int quantity) {
    return reduce_stack_internal(position, quantity);
}
std::list<item> inventory::reduce_stack(char ch, int quantity) {
    return reduce_stack_internal(ch, quantity);
}
std::list<item> inventory::reduce_stack(const itype_id& type, int quantity) {
    return reduce_stack_internal(type, quantity);
}

item inventory::remove_item(item* it)
{
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        for (std::list<item>::iterator stack_iter = iter->begin(); stack_iter != iter->end(); ++stack_iter)
        {
            if (it == &*stack_iter)
            {
                item tmp = *stack_iter;
                iter->erase(stack_iter);
                if (iter->size() <= 0)
                {
                    items.erase(iter);
                }
                return tmp;
            }
        }
    }

    debugmsg("Tried to remove a item not in inventory (name: %s)", it->type->name.c_str());
    return nullitem;
}

template<typename Locator>
item inventory::remove_item_internal(const Locator& locator) {
    int pos = 0;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        if (item_matches_locator(iter->front(), locator, pos))
        {
            if (iter->size() > 1)
            {
                std::list<item>::iterator stack_member = iter->begin();
                char invlet = stack_member->invlet;
                ++stack_member;
                stack_member->invlet = invlet;
            }
            item ret = iter->front();
            iter->erase(iter->begin());
            if (iter->empty())
            {
                items.erase(iter);
            }
            return ret;
        }
        ++pos;
    }

    return nullitem;
}

// Instantiate for each type of Locator.
item inventory::remove_item(int position) {
    return remove_item_internal(position);
}
item inventory::remove_item(char ch) {
    return remove_item_internal(ch);
}
item inventory::remove_item(const itype_id& type) {
    return remove_item_internal(type);
}

// using this assumes the item has charges
template<typename Locator>
item inventory::reduce_charges_internal(const Locator& locator, int quantity)
{
    int pos = 0;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        if (item_matches_locator(iter->front(), locator, pos))
        {
            if (!iter->front().count_by_charges())
            {
                debugmsg("Tried to remove %s by charges, but item is not counted by charges", iter->front().type->name.c_str());
            }
            item ret = iter->front();
            if (quantity > iter->front().charges)
            {
                debugmsg("Charges: Tried to remove charges that does not exist, \
                          removing maximum available charges instead");
                quantity = iter->front().charges;
            }
            ret.charges = quantity;
            iter->front().charges -= quantity;
            if (iter->front().charges <= 0)
            {
                items.erase(iter);
            }
            return ret;
        }
        ++pos;
    }
    debugmsg("Tried to reduce charges but could not find item.");
    return nullitem;
}

// Instantiate for each type of Locator.
item inventory::reduce_charges(int position, int quantity) {
    return reduce_charges_internal(position, quantity);
}
item inventory::reduce_charges(char ch, int quantity) {
    return reduce_charges_internal(ch, quantity);
}
item inventory::reduce_charges(const itype_id& type, int quantity) {
    return reduce_charges_internal(type, quantity);
}

std::vector<item> inventory::remove_mission_items(int mission_id)
{
    std::vector<item> ret;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        for (std::list<item>::iterator stack_iter = iter->begin();
             stack_iter != iter->end(); ++stack_iter)
        {
            if (stack_iter->mission_id == mission_id)
            {
                ret.push_back(remove_item(&*stack_iter));
                stack_iter = iter->begin();
            }
            else
            {
                for (int k = 0; k < stack_iter->contents.size() && stack_iter != iter->end(); ++k)
                {
                    if (stack_iter->contents[k].mission_id == mission_id)
                    {
                        ret.push_back(remove_item(&*stack_iter));
                        stack_iter = iter->begin();
                    }
                }
            }
        }
        if (ret.size())
        {
            return ret;
        }
    }
    return ret;
}

void inventory::dump(std::vector<item *>& dest)
{
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        for (std::list<item>::iterator stack_iter = iter->begin();
             stack_iter != iter->end(); ++stack_iter)
        {
            dest.push_back(&(*stack_iter));
        }
    }
}

item& inventory::find_item(int position) {
    if (position < 0 || position >= items.size())
        return nullitem;
    invstack::iterator iter = items.begin();
    for (int j = 0; j < position; ++j)
    {
        ++iter;
    }
    return iter->front();
}

int inventory::position_by_letter(char ch) {
    int i = 0;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter) {
        if (iter->begin()->invlet == ch) {
            return i;
        }
        ++i;
    }
    return INT_MIN;
}

int inventory::position_by_item(item* it) {
    int i = 0;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter) {
        for (std::list<item>::iterator stack_iter = iter->begin(); stack_iter != iter->end(); ++stack_iter) {
            if (it == &*stack_iter) {
                return i;
            }
        }
        ++i;
    }
    return INT_MIN;
}

item& inventory::item_by_letter(char ch)
{
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        if (iter->begin()->invlet == ch)
        {
            return *iter->begin();
        }
    }
    return nullitem;
}

item& inventory::item_by_type(itype_id type)
{
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        if (iter->front().type->id == type)
        {
            return iter->front();
        }
    }
    return nullitem;
}

int inventory::position_by_type(itype_id type)
{
    int i = 0;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        if (iter->front().type->id == type)
        {
            return i;
        }
        ++i;
    }
    return INT_MIN;
}
item& inventory::item_or_container(itype_id type)
{
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        for (std::list<item>::iterator stack_iter = iter->begin();
             stack_iter != iter->end(); ++stack_iter)
        {
            if (stack_iter->type->id == type)
            {
                return *stack_iter;
            }
            else if (stack_iter->is_container() && stack_iter->contents.size() > 0)
            {
                if (stack_iter->contents[0].type->id == type)
                {
                    return *stack_iter;
                }
            }
        }
    }

    return nullitem;
}

std::vector<std::pair<item*, int> > inventory::all_items_by_type(itype_id type)
{
    std::vector<std::pair<item*, int> > ret;
    int i = 0;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        for (std::list<item>::iterator stack_iter = iter->begin();
             stack_iter != iter->end();
             ++stack_iter)
        {
            if (stack_iter->type->id == type)
            {
                ret.push_back(std::make_pair(&*stack_iter, i));
            }
        }
        ++i;
    }
    return ret;
}

std::vector<item*> inventory::all_items_with_flag( const std::string flag ) {
  std::vector<item*> ret;

  for (invstack::iterator istack = items.begin(); istack != items.end(); ++istack) {
    for (std::list<item>::iterator iitem = istack->begin(); iitem != istack->end(); ++iitem) {
      if (iitem->has_flag(flag)) {
        ret.push_back(&*iitem);
      }
    }
  }

  return ret;
}


std::vector<item*> inventory::all_ammo(ammotype type)
{
    std::vector<item*> ret;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        for (std::list<item>::iterator stack_iter = iter->begin();
             stack_iter != iter->end();
             ++stack_iter)
        {
            if (stack_iter->is_ammo() && dynamic_cast<it_ammo*>(stack_iter->type)->type == type)
            {
                ret.push_back(&*stack_iter);

            }
            // Handle gasoline nested in containers
            else if (type == "gasoline" && stack_iter->is_container() &&
                     !stack_iter->contents.empty() && stack_iter->contents[0].is_ammo() &&
                     dynamic_cast<it_ammo*>(stack_iter->contents[0].type)->type == type)
            {
                ret.push_back(&*stack_iter);
                return ret;
            }
        }
    }
    return ret;
}

int inventory::amount_of(itype_id it) const
{
    int count = 0;
    for (invstack::const_iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        for (std::list<item>::const_iterator stack_iter = iter->begin();
             stack_iter != iter->end();
             ++stack_iter)
        {
            if (stack_iter->type->id == it)
            {
                // check if it's a container, if so, it should be empty
                if (stack_iter->type->is_container())
                {
                    if (stack_iter->contents.empty())
                    {
                        count++;
                    }
                }
                else
                {
                    count++;
                }
            }
            for (int k = 0; k < stack_iter->contents.size(); k++)
            {
                if (stack_iter->contents[k].type->id == it)
                {
                    count++;
                }
            }
        }
    }
    return count;
}

int inventory::charges_of(itype_id it) const
{
    int count = 0;
    for (invstack::const_iterator iter = items.begin(); iter != items.end(); ++iter) {
        for (std::list<item>::const_iterator stack_iter = iter->begin();
             stack_iter != iter->end(); ++stack_iter) {
            if (stack_iter->type->id == it || stack_iter->ammo_type() == it) {
                // If we're specifically looking for a container, only say we have it if it's empty.
                if( stack_iter->contents.size() == 0 ) {
                    if (stack_iter->charges < 0) {
                        count++;
                    } else {
                        count += stack_iter->charges;
                    }
                }
            }
            for (int k = 0; k < stack_iter->contents.size(); k++) {
                if (stack_iter->contents[k].type->id == it) {
                    if (stack_iter->contents[k].charges < 0) {
                        count++;
                    } else {
                        count += stack_iter->contents[k].charges;
                    }
                }
            }
        }
    }
    return count;
}

std::list<item> inventory::use_amount(itype_id it, int quantity, bool use_container)
{
    sort();
    std::list<item> ret;
    for (invstack::iterator iter = items.begin(); iter != items.end() && quantity > 0; ++iter)
    {
        for (std::list<item>::iterator stack_iter = iter->begin();
             stack_iter != iter->end() && quantity > 0;
             ++stack_iter)
        {
            // First, check contents
            bool used_item_contents = false;
            for (int k = 0; k < stack_iter->contents.size() && quantity > 0; k++)
            {
                if (stack_iter->contents[k].type->id == it)
                {
                    ret.push_back(stack_iter->contents[k]);
                    quantity--;
                    stack_iter->contents.erase(stack_iter->contents.begin() + k);
                    k--;
                    used_item_contents = true;
                }
            }
            // Now check the item itself
            if (use_container && used_item_contents)
            {
                stack_iter = iter->erase(stack_iter);
                if (iter->empty())
                {
                    iter = items.erase(iter);
                    --iter;
                    break;
                }
                else
                {
                    --stack_iter;
                }
            }
            else if (stack_iter->type->id == it && quantity > 0 && stack_iter->contents.size() == 0)
            {
                ret.push_back(*stack_iter);
                quantity--;
                stack_iter = iter->erase(stack_iter);
                if (iter->empty())
                {
                    iter = items.erase(iter);
                    --iter;
                    break;
                }
                else
                {
                    --stack_iter;
                }
            }
        }
    }
    return ret;
}

std::list<item> inventory::use_charges(itype_id it, int quantity)
{
    sort();
    std::list<item> ret;
    for (invstack::iterator iter = items.begin(); iter != items.end() && quantity > 0; ++iter)
    {
        for (std::list<item>::iterator stack_iter = iter->begin();
             stack_iter != iter->end() && quantity > 0;
             ++stack_iter)
        {
            // First, check contents
            for (int k = 0; k < stack_iter->contents.size() && quantity > 0; k++)
            {
                if (stack_iter->contents[k].type->id == it)
                {
                    if (stack_iter->contents[k].charges <= quantity)
                    {
                        ret.push_back(stack_iter->contents[k]);
                        quantity -= stack_iter->contents[k].charges;
                        if (stack_iter->contents[k].destroyed_at_zero_charges())
                        {
                            stack_iter->contents.erase(stack_iter->contents.begin() + k);
                            k--;
                        }
                        else
                        {
                            stack_iter->contents[k].charges = 0;
                        }
                    }
                    else
                    {
                        item tmp = stack_iter->contents[k];
                        tmp.charges = quantity;
                        ret.push_back(tmp);
                        stack_iter->contents[k].charges -= quantity;
                        return ret;
                    }
                }
            }

            // Now check the item itself
            if (stack_iter->type->id == it || stack_iter->ammo_type() == it)
            {
                if (stack_iter->charges <= quantity)
                {
                    ret.push_back(*stack_iter);
                    if (stack_iter->charges < 0) {
                        quantity--;
                    } else {
                        quantity -= stack_iter->charges;
                    }
                    if (stack_iter->destroyed_at_zero_charges())
                    {
                        stack_iter = iter->erase(stack_iter);
                        if (iter->empty())
                        {
                            iter = items.erase(iter);
                            --iter;
                            break;
                        }
                        else
                        {
                            --stack_iter;
                        }
                    }
                    else
                    {
                        stack_iter->charges = 0;
                    }
                }
                else
                {
                    item tmp = *stack_iter;
                    tmp.charges = quantity;
                    ret.push_back(tmp);
                    stack_iter->charges -= quantity;
                    return ret;
                }
            }
        }
    }
    return ret;
}

bool inventory::has_amount(itype_id it, int quantity) const
{
 return (amount_of(it) >= quantity);
}

bool inventory::has_charges(itype_id it, int quantity) const
{
 return (charges_of(it) >= quantity);
}

bool inventory::has_flag(std::string flag) const
{
    for (invstack::const_iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        for (std::list<item>::const_iterator stack_iter = iter->begin(); stack_iter != iter->end(); ++stack_iter)
        {
            if (stack_iter->has_flag(flag))
            {
                return true;
            }
        }
    }
    return false;
}

bool inventory::has_item(item *it) const
{
    for (invstack::const_iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        for (std::list<item>::const_iterator stack_iter = iter->begin(); stack_iter != iter->end(); ++stack_iter)
        {
            if (it == &(*stack_iter))
            {
                return true;
            }
            for (int k = 0; k < stack_iter->contents.size(); k++)
            {
                if (it == &(stack_iter->contents[k]))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

bool inventory::has_items_with_quality(std::string id, int level, int amount) const
{
    int found = 0;
    for (invstack::const_iterator iter = items.begin(); iter != items.end(); ++iter){
        for(std::list<item>::const_iterator stack_iter = iter->begin(); stack_iter != iter->end(); ++stack_iter){
            std::map<std::string,int> qualities = stack_iter->type->qualities;
            std::map<std::string,int>::const_iterator quality_iter = qualities.find(id);
            if(quality_iter!=qualities.end() && level >= quality_iter->second){
              found++;
            }
        }
    }
    if(found >= amount){
      return true;
    } else {
      return false;
    }
}

bool inventory::has_gun_for_ammo(ammotype type) const
{
    for (invstack::const_iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        for (std::list<item>::const_iterator stack_iter = iter->begin(); stack_iter != iter->end(); ++stack_iter)
        {
            if (stack_iter->is_gun() && stack_iter->ammo_type() == type)
            {
                return true;
            }
        }
    }
    return false;
}

bool inventory::has_active_item(itype_id type) const
{
    for (invstack::const_iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        for (std::list<item>::const_iterator stack_iter = iter->begin(); stack_iter != iter->end(); ++stack_iter)
        {
            if (stack_iter->type->id == type && stack_iter->active)
            {
                return true;
            }
        }
    }
    return false;
}

bool inventory::has_mission_item(int mission_id) const
{
    for (invstack::const_iterator stack = items.begin(); stack != items.end(); ++stack)
    {
        for (std::list<item>::const_iterator iter = stack->begin(); iter != stack->end(); ++iter)
        {
            if (iter->mission_id == mission_id)
            {
                return true;
            }
            for (int k = 0; k < iter->contents.size(); k++)
            {
                if (iter->contents[k].mission_id == mission_id)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

int inventory::butcher_factor() const
{
    int lowest_factor = 999;
    for (invstack::const_iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        for (std::list<item>::const_iterator stack_iter = iter->begin();
             stack_iter != iter->end();
             ++stack_iter)
        {
            const item& cur_item = *stack_iter;
            if (cur_item.has_quality("CUT") && !cur_item.has_flag("SPEAR"))
            {
                int factor = cur_item.volume() * 5 - cur_item.weight() / 75 -
                             cur_item.damage_cut();
                if (cur_item.damage_cut() <= 20)
                {
                    factor *= 2;
                }
                if (factor < lowest_factor)
                {
                    lowest_factor = factor;
                }
            }
        }
    }
    return lowest_factor;
}

bool inventory::has_artifact_with(art_effect_passive effect) const
{
    for (invstack::const_iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        const item& it = iter->front();
        if (it.is_artifact() && it.is_tool()) {
            it_artifact_tool *tool = dynamic_cast<it_artifact_tool*>(it.type);
            for (std::vector<art_effect_passive>::const_iterator ef_iter = tool->effects_carried.begin();
                 ef_iter != tool->effects_carried.end(); ++ef_iter)
            {
                if (*ef_iter == effect)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

bool inventory::has_liquid(itype_id type) const
{
    for (invstack::const_iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        const item& it = iter->front();
        if (it.is_container() && !it.contents.empty())
        {
            if (it.contents[0].type->id == type)
            {
                // liquid matches
                it_container* container = dynamic_cast<it_container*>(it.type);
                int holding_container_charges;

                if (it.contents[0].type->is_food())
                {
                    it_comest* tmp_comest = dynamic_cast<it_comest*>(it.contents[0].type);

                    if (tmp_comest->add == ADD_ALCOHOL) // 1 contains = 20 alcohol charges
                    {
                        holding_container_charges = container->contains * 20;
                    }
                    else
                    {
                        holding_container_charges = container->contains;
                    }
                }
                else if (it.contents[0].type->is_ammo())
                {
                    // gasoline?
                    holding_container_charges = container->contains * 200;
                }
                else
                {
                    holding_container_charges = container->contains;
                }

                if (it.contents[0].charges < holding_container_charges)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

item& inventory::watertight_container()
{
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        item& it = iter->front();
        if (it.is_container() && it.contents.empty())
        {
            if (it.has_flag("WATERTIGHT") && it.has_flag("SEALS"))
            {
                return it;
            }
        }
    }
    return nullitem;
}

int inventory::worst_item_value(npc* p) const
{
    int worst = 99999;
    for (invstack::const_iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        const item& it = iter->front();
        int val = p->value(it);
        if (val < worst)
        {
            worst = val;
        }
    }
    return worst;
}

bool inventory::has_enough_painkiller(int pain) const
{
    for (invstack::const_iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        const item& it = iter->front();
        if ( (pain <= 35 && it.type->id == "aspirin") ||
             (pain >= 50 && it.type->id == "oxycodone") ||
             it.type->id == "tramadol" || it.type->id == "codeine")
        {
            return true;
        }
    }
    return false;
}

item& inventory::most_appropriate_painkiller(int pain)
{
    int difference = 9999;
    item& ret = nullitem;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        int diff = 9999;
        itype_id type = iter->front().type->id;
        if (type == "aspirin")
        {
            diff = abs(pain - 15);
        }
        else if (type == "codeine")
        {
            diff = abs(pain - 30);
        }
        else if (type == "oxycodone")
        {
            diff = abs(pain - 60);
        }
        else if (type == "heroin")
        {
            diff = abs(pain - 100);
        }
        else if (type == "tramadol")
        {
            diff = abs(pain - 40) / 2; // Bonus since it's long-acting
        }

        if (diff < difference)
        {
            difference = diff;
            ret = iter->front();
        }
    }
    return ret;
}

item& inventory::best_for_melee(player *p)
{
    item& ret = nullitem;
    int best = 0;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        int score = iter->front().melee_value(p);
        if (score > best)
        {
            best = score;
            ret = iter->front();
        }
    }
    return ret;
}

item& inventory::most_loaded_gun()
{
    item& ret = nullitem;
    int max = 0;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        if (iter->front().is_gun() && iter->front().charges > max)
        {
            ret = iter->front();
            max = ret.charges;
        }
    }
    return ret;
}

void inventory::rust_iron_items()
{
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        for (std::list<item>::iterator stack_iter = iter->begin();
             stack_iter != iter->end();
             ++stack_iter)
        {
            if (stack_iter->type->m1 == "iron" && !stack_iter->has_flag("WATERPROOF_GUN") &&
            !stack_iter->has_flag("WATERPROOF") &&stack_iter->damage < 5 && one_in(500))
            {
                stack_iter->damage++;
            }
        }
    }
}

int inventory::weight() const
{
    int ret = 0;
    for (invstack::const_iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        for (std::list<item>::const_iterator stack_iter = iter->begin();
             stack_iter != iter->end();
             ++stack_iter)
        {
            ret += stack_iter->weight();
        }
    }
    return ret;
}

int inventory::volume() const
{
    int ret = 0;
    for (invstack::const_iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        for (std::list<item>::const_iterator stack_iter = iter->begin();
             stack_iter != iter->end();
             ++stack_iter)
        {
            ret += stack_iter->volume();
        }
    }
    return ret;
}

int inventory::max_active_item_charges(itype_id id) const
{
    int max = 0;
    for (invstack::const_iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        for (std::list<item>::const_iterator stack_iter = iter->begin();
             stack_iter != iter->end();
             ++stack_iter)
        {
            if (stack_iter->type->id == id && stack_iter->active &&
                stack_iter->charges > max)
            {
                max = stack_iter->charges;
            }
        }
    }
    return max;
}

std::vector<item*> inventory::active_items()
{
    std::vector<item*> ret;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        for (std::list<item>::iterator stack_iter = iter->begin();
             stack_iter != iter->end();
             ++stack_iter)
        {
            if ( (stack_iter->is_artifact() && stack_iter->is_tool()) ||
                stack_iter->active ||
                (stack_iter->is_container() && stack_iter->contents.size() > 0 && stack_iter->contents[0].active))
            {
                ret.push_back(&*stack_iter);
            }
        }
    }
    return ret;
}

void inventory::assign_empty_invlet(item &it, bool force)
{
  player *p = &(g->u);
  for (std::string::const_iterator newinvlet = inv_chars.begin();
       newinvlet != inv_chars.end();
       newinvlet++) {
   if (!p->has_item(*newinvlet) && (!p || !p->has_weapon_or_armor(*newinvlet))) {
    it.invlet = *newinvlet;
    return;
   }
  }
  it.invlet = force ? '`' : 0;
}
