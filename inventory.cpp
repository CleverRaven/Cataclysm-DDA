#include <sstream>
#include "inventory.h"
#include "game.h"
#include "keypress.h"
#include "mapdata.h"

const std::string inv_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!\"#&()*+./:;=?@[\\]^_{|}";

item& inventory::operator[] (int i)
{
    if (i < 0 || i > items.size())
    {
        debugmsg("Attempted to access item %d in an inventory (size %d)",
                 i, items.size());
        return nullitem;
    }

    invstack::iterator iter = items.begin();
    for (int j = 0; j < i; ++j)
    {
        ++iter;
    }

    return iter->front();
}

// TODO: make the two slice methods complain (debugmsg?) if length == size()
// after all, if we're asking for the *entire* inventory,
// that's probably a sign of a partial legacy conversion

invslice inventory::slice(int start, int length)
{
    invslice stacks;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        if (start <= 0)
        {
            stacks.push_back(&*iter);
            --length;
        }
        if (length <= 0)
        {
            break;
        }
        --start;
    }
    
    return stacks;
}

invslice inventory::slice(const std::list<item>* start, int length)
{
    invslice stacks;
    bool found_start = false;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        if (!found_start && start == &(*iter))
        {
            found_start = true;
        }
        if (found_start)
        {
            stacks.push_back(&*iter);
            --length;
        }
        if (length <= 0)
        {
            break;
        }
    }
    
    return stacks;
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

std::list<item> inventory::const_stack(int i) const
{
    if (i < 0 || i > items.size())
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
        add_item(*iter);
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

void inventory::push_back(std::list<item> newits)
{
 add_stack(newits);
}

void inventory::add_item(item newit, bool keep_invlet)
{
    if (keep_invlet && !newit.invlet_is_okay())
    {
        assign_empty_invlet(newit); // Keep invlet is true, but invlet is invalid!
    }

    if (newit.is_style())
    {
        return; // Styles never belong in our inventory.
    }
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        std::list<item>::iterator it_ref = iter->begin();
        if (it_ref->stacks_with(newit))
        {
		    if (it_ref->is_food() && it_ref->has_flag(IF_HOT))
		    {
			    int tmpcounter = (it_ref->item_counter + newit.item_counter) / 2;
			    it_ref->item_counter = tmpcounter;
			    newit.item_counter = tmpcounter;
		    }
            newit.invlet = it_ref->invlet;
            iter->push_back(newit);
            return;
        }
        else if (keep_invlet && it_ref->invlet == newit.invlet)
        {
            assign_empty_invlet(*it_ref);
        }
    }
    if (!newit.invlet_is_okay() || !item_by_letter(newit.invlet).is_null())
    {
        assign_empty_invlet(newit);
    }

    std::list<item> newstack;
    newstack.push_back(newit);
    items.push_back(newstack);
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

    if (!p)
    {
        return;
    }
    
    std::list<item> to_restack;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        if (!iter->front().invlet_is_okay() || p->has_weapon_or_armor(iter->front().invlet))
        {
            assign_empty_invlet(iter->front(), p);
            for (std::list<item>::iterator stack_iter = iter->begin();
                 stack_iter != iter->end();
                 ++stack_iter)
            {
                stack_iter->invlet = iter->front().invlet;
            }
        }

        // remove non-matching items
        while (iter->size() > 0 && !iter->front().stacks_with(iter->back()))
        {
            to_restack.splice(to_restack.begin(), *iter, iter->begin());
        }
        if (iter->size() <= 0)
        {
            iter = items.erase(iter);
            --iter;
            continue;
        }
    }

    // combine matching stacks
    // separate loop to ensure that ALL stacks are homogeneous
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        for (invstack::iterator other = iter; other != items.end(); ++other)
        {
            if (iter != other && iter->front().stacks_with(other->front()))
            {
                iter->splice(iter->begin(), *other);
                other = items.erase(other);
                --other;
                continue;
            }
        }
    }

    //re-add non-matching items
    for (std::list<item>::iterator iter = to_restack.begin(); iter != to_restack.end(); ++iter)
    {
        add_item(*iter);
    }
}

void inventory::form_from_map(game *g, point origin, int range)
{
 items.clear();
 for (int x = origin.x - range; x <= origin.x + range; x++) {
  for (int y = origin.y - range; y <= origin.y + range; y++) {
   for (int i = 0; i < g->m.i_at(x, y).size(); i++)
    if (!g->m.i_at(x, y)[i].made_of(LIQUID))
     add_item(g->m.i_at(x, y)[i]);
// Kludges for now!
   if (g->m.field_at(x, y).type == fd_fire) {
    item fire(g->itypes["fire"], 0);
    fire.charges = 1;
    add_item(fire);
   }
   ter_id terrain_id = g->m.ter(x, y);
   if (terrain_id == t_toilet || terrain_id == t_water_sh || terrain_id == t_water_dp){
    item water(g->itypes["water"], 0);
    water.charges = 50;
    add_item(water);
   }

   int vpart = -1;
   vehicle *veh = g->m.veh_at(x, y, vpart);

   if (veh) {
     const int kpart = veh->part_with_feature(vpart, vpf_kitchen);

     if (kpart >= 0) {
       item hotplate(g->itypes["hotplate"], 0);
       hotplate.charges = veh->fuel_left(AT_BATT, true);
       add_item(hotplate);

       item water(g->itypes["water_clean"], 0);
       water.charges = veh->fuel_left(AT_WATER);
       add_item(water);

       item pot(g->itypes["pot"], 0);
       add_item(pot);
       item pan(g->itypes["pan"], 0);
       add_item(pan);
     }
   }

  }
 }
}

std::list<item> inventory::remove_stack(int index)
{
    if (index < 0 || index >= items.size())
    {
        debugmsg("Tried to remove_stack(%d) from an inventory (size %d)",
                 index, items.size());
        std::list<item> nullvector;
        return nullvector;
    }
    invstack::iterator iter = items.begin();
    for (int i = 0; i < index; ++i)
    {
        ++iter;
    }
    std::list<item> ret = *iter;
    items.erase(iter);
    return ret;
}

std::list<item> inventory::remove_stack(int index,int amount)
{
    if (index < 0 || index >= items.size())
    {
        debugmsg("Tried to remove_stack(%d) from an inventory (size %d)",
               index, items.size());
        std::list<item> nullvector;
        return nullvector;
    }
    invstack::iterator iter = items.begin();
    for (int i = 0; i < index; ++i)
    {
        ++iter;
    }
    std::list<item> ret;
    if(amount > iter->size())
    {
        std::list<item> ret = *iter;
        items.erase(iter);
    }
    else
    {
        for(int i = 0 ; i < amount ; i++)
        {
            ret.push_back(remove_item(&iter->front()));
        }
    }
    return ret;
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

item inventory::remove_item(invstack::iterator iter)
{
    item ret = iter->front();
    iter->erase(iter->begin());
    if (iter->empty())
    {
        items.erase(iter);
    }
    return ret;
}

item inventory::remove_item(int stack, int index)
{
    if (stack < 0 || stack >= items.size())
    {
        debugmsg("Tried to remove_item(%d, %d) from an inventory (size %d)",
                 stack, index, items.size());
        return nullitem;
    }

    invstack::iterator iter = items.begin();
    for (int i = 0; i < stack; ++i)
    {
        ++iter;
    }
    if (index < 0 || index >= iter->size())
    {
        debugmsg("Tried to remove_item(%d, %d) from an inventory (stack is size %d)",
                 stack, index, iter->size());
        return nullitem;
    }

    std::list<item>& stack_ref = *iter;
    std::list<item>::iterator it_iter = stack_ref.begin();
    for (int i = 0; i < index; ++i)
    {
        ++it_iter;
    }
    item ret = *it_iter;
    stack_ref.erase(it_iter);
    if (stack_ref.empty())
    {
        remove_stack(stack);
    }

    return ret;
}

item inventory::remove_item_by_type(itype_id type)
{
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        if (iter->front().type->id == type)
        {
            return remove_item(iter);
        }
    }
    return nullitem;
}

item inventory::remove_item_by_letter(char ch)
{
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        if (iter->begin()->invlet == ch)
        {
            if (iter->size() > 1)
            {
                std::list<item>::iterator stack_member = iter->begin();
                ++stack_member;
                stack_member->invlet = ch;
            }
            return remove_item(iter);
        }
    }

    return nullitem;
}

item inventory::remove_item_by_letter_and_quantity(char ch, int quantity)
{
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        if (iter->begin()->invlet == ch)
        {
            item ret = iter->front();
            if (quantity > iter->front().charges)
            {
                debugmsg("Charges: Tried to remove charges that does not exist, \
                          removing maximum available charges instead");
                quantity = iter->front().charges;
            }
            ret.charges = quantity;
            iter->front().charges -= quantity;
            return ret;
        }
    }

    debugmsg("Tried to remove item with invlet %c by quantity, no such item", ch);
    return nullitem;
}

item inventory::remove_item_by_quantity(int index, int quantity)
{
    // using this assumes the item has charges
    if (index < 0 || index >= items.size()) {
        debugmsg("Quantity: Tried to remove_item(%d) from an inventory (size %d)",
               index, items.size());
        return nullitem;
    }

    invstack::iterator iter = items.begin();
    for (int i = 0; i < index; ++i)
    {
        ++iter;
    }
    std::list<item>& stack = *iter;
    item ret = *(stack.begin());
    if(quantity > stack.begin()->charges)
    {
        debugmsg("Charges: Tried to remove charges that does not exist, \
                removing maximum available charges instead");
        quantity = stack.begin()->charges;
    }
    ret.charges = quantity;
    stack.begin()->charges -= quantity;
    return ret;
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
                for (int k = 0; k < stack_iter->contents.size(); ++k)
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

void inventory::dump(std::vector<item>& dest)
{
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        for (std::list<item>::iterator stack_iter = iter->begin();
             stack_iter != iter->end(); ++stack_iter)
        {
            dest.push_back(*stack_iter);
        }
    }
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

// TODO: remove this function
int inventory::index_by_letter(char ch)
{
     if (ch == KEY_ESCAPE)
     {
         return -1;
     }
     int i = 0;
     for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
     {
         if (iter->begin()->invlet == ch)
         {
             return i;
         }
         ++i;
     }
     return -1;
}

int inventory::amount_of(itype_id it)
{
    int count = 0;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        for (std::list<item>::iterator stack_iter = iter->begin();
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

int inventory::charges_of(itype_id it)
{
    int count = 0;
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        for (std::list<item>::iterator stack_iter = iter->begin();
             stack_iter != iter->end();
             ++stack_iter)
        {
            if (stack_iter->type->id == it)
            {
                if (stack_iter->charges < 0)
                {
                    count++;
                }
                else
                {
                    count += stack_iter->charges;
                }
            }
            for (int k = 0; k < stack_iter->contents.size(); k++)
            {
                if (stack_iter->contents[k].type->id == it)
                {
                    if (stack_iter->contents[k].charges < 0)
                    {
                        count++;
                    }
                    else
                    {
                        count += stack_iter->contents[k].charges;
                    }
                }
            }
        }
    }
    return count;
}

void inventory::use_amount(itype_id it, int quantity, bool use_container)
{
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
                --stack_iter;
                if (iter->empty())
                {
                    iter = items.erase(iter);
                    --iter;
                    stack_iter = iter->begin();
                }
            }
            else if (stack_iter->type->id == it && quantity > 0)
            {
                quantity--;
                stack_iter = iter->erase(stack_iter);
                --stack_iter;
                if (iter->empty())
                {
                    iter = items.erase(iter);
                    --iter;
                }
            }
        }
    }
}

void inventory::use_charges(itype_id it, int quantity)
{
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
                        stack_iter->contents[k].charges -= quantity;
                        return;
                    }
                }
            }

            // Now check the item itself
            if (stack_iter->type->id == it)
            {
                if (stack_iter->charges <= quantity)
                {
                    quantity -= stack_iter->charges;
                    if (stack_iter->destroyed_at_zero_charges())
                    {
                        stack_iter = iter->erase(stack_iter);
                        --stack_iter;
                        if (iter->empty())
                        {
                            iter = items.erase(iter);
                            --iter;
                            stack_iter = iter->begin();
                        }
                    }
                    else
                    {
                        stack_iter->charges = 0;
                    }
                }
                else
                {
                    stack_iter->charges -= quantity;
                    return;
                }
            }
        }
    }
}

bool inventory::has_amount(itype_id it, int quantity)
{
 return (amount_of(it) >= quantity);
}

bool inventory::has_charges(itype_id it, int quantity)
{
 return (charges_of(it) >= quantity);
}

bool inventory::has_item(item *it)
{
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        for (std::list<item>::iterator stack_iter = iter->begin(); stack_iter != iter->end(); ++stack_iter)
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
            if (cur_item.damage_cut() >= 10 && !cur_item.has_flag(IF_SPEAR))
            {
                int factor = cur_item.volume() * 5 - cur_item.weight() * 1.5 -
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

void inventory::assign_empty_invlet(item &it, player *p)
{
  for (std::string::const_iterator newinvlet = inv_chars.begin();
       newinvlet != inv_chars.end();
       newinvlet++) {
   if (item_by_letter(*newinvlet).is_null() && (!p || !p->has_weapon_or_armor(*newinvlet))) {
    it.invlet = *newinvlet;
    return;
   }
  }
  it.invlet = '`';
  //debugmsg("Couldn't find empty invlet");
}

std::string inventory::save_str_no_quant() const
{
    std::stringstream dump;
    for (invstack::const_iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        for (std::list<item>::const_iterator stack_iter = iter->begin();
             stack_iter != iter->end();
             ++stack_iter)
        {
            dump << "I " << stack_iter->save_info() << std::endl;
            for (int k = 0; k < stack_iter->contents.size(); k++)
            {
                dump << "C " << stack_iter->contents[k].save_info() << std::endl;
            }
        }
    }
    return dump.str();
}
