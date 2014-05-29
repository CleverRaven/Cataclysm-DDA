#include "item_factory.h"
#include "item_group.h"
#include "monstergenerator.h"
#include "rng.h"
#include "output.h"
#include <map>
#include <algorithm>
#include <cassert>

static const std::string null_item_id("null");

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

item Single_item_creator::create_single(int birthday, RecursionList &rec) const {
    item tmp;
    if (type == S_ITEM) {
        if (id == "corpse") {
            tmp.make_corpse("corpse", GetMType("mon_null"), birthday);
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
    } else if (type == S_NONE) {
        return item(null_item_id, birthday);
    }
    if (modifier.get() != NULL) {
        modifier->modify(tmp);
    }
    // TODO: change the spawn lists to contain proper references to containers
    tmp = tmp.in_its_container();
    return tmp;
}

Item_spawn_data::ItemList Single_item_creator::create(int birthday, RecursionList &rec) const {
    ItemList result;
    int cnt = 1;
    if (modifier.get() != NULL) {
        cnt = (modifier->count.first == modifier->count.second) ? modifier->count.first : rng(modifier->count.first, modifier->count.second);
    }
    for( ; cnt > 0; cnt--) {
        if (type == S_ITEM) {
            result.push_back(create_single(birthday, rec));
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
            if (modifier.get() != NULL) {
                for(ItemList::iterator a = tmplist.begin(); a != tmplist.end(); ++a) {
                    modifier->modify(*a);
                }
            }
            result.insert(result.end(), tmplist.begin(), tmplist.end());
        }
    }
    return result;
}

void Single_item_creator::check_consistency() const {
    if (type == S_ITEM) {
        if (!item_controller->has_template(id)) {
            debugmsg("item id %s is unknown", id.c_str());
        }
    } else if (type == S_ITEM_GROUP) {
        if (!item_controller->has_group(id)) {
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

Item_modifier::~Item_modifier() {
}

void Item_modifier::modify(item &new_item) const {
    if(new_item.is_null()) {
        return;
    }
    int dm = (damage.first == damage.second) ? damage.first : rng(damage.first, damage.second);
    if(dm >= -1 && dm <= 4) {
        new_item.damage = dm;
    }
    long ch = (charges.first == charges.second) ? charges.first : rng(charges.first, charges.second);
    if(ch != -1) {
        it_tool *t = dynamic_cast<it_tool*>(new_item.type);
        it_gun *g = dynamic_cast<it_gun*>(new_item.type);
        if(new_item.count_by_charges()) {
            // food, ammo
            new_item.charges = ch;
        } else if(t != NULL) {
            new_item.charges = std::min(ch, t->max_charges);
        } else if(g != NULL && ammo.get() != NULL) {
            item am = ammo->create_single(new_item.bday);
            it_ammo *a = dynamic_cast<it_ammo*>(am.type);
            if(!am.is_null() && a != NULL) {
                new_item.curammo = a;
                new_item.charges = std::min<long>(am.charges, new_item.clip_size());
            }
        }
    }
    if(container.get() != NULL) {
        item cont = container->create_single(new_item.bday);
        if (!cont.is_null()) {
            if (new_item.made_of(LIQUID)) {
                LIQUID_FILL_ERROR err;
                int rc = cont.get_remaining_capacity_for_liquid(new_item, err);
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

void Item_modifier::check_consistency() const {
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

Item_group::~Item_group() {
    for(prop_list::iterator a = items.begin(); a != items.end(); ++a) {
        delete *a;
    }
    items.clear();
}

void Item_group::add_item_entry(const Item_tag &itemid, int probability)
{
    std::unique_ptr<Item_spawn_data> ptr(new Single_item_creator(itemid, Single_item_creator::S_ITEM, probability));
    add_entry(ptr);
}

void Item_group::add_group_entry(const Group_tag &groupid, int probability)
{
    std::unique_ptr<Item_spawn_data> ptr(new Single_item_creator(groupid, Single_item_creator::S_ITEM_GROUP, probability));
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
        for(prop_list::const_iterator a = items.begin(); a != items.end(); ++a) {
            if(rng(0, 99) >= (*a)->probability) {
                continue;
            }
            ItemList tmp = (*a)->create(birthday, rec);
            result.insert(result.end(), tmp.begin(), tmp.end());
        }
    } else if (type == G_DISTRIBUTION) {
        int p = rng(0, sum_prob - 1);
        for(prop_list::const_iterator a = items.begin(); a != items.end(); ++a) {
            p -= (*a)->probability;
            if (p >= 0) {
                continue;
            }
            ItemList tmp = (*a)->create(birthday, rec);
            result.insert(result.end(), tmp.begin(), tmp.end());
            break;
        }
    }
    if (with_ammo && !result.empty()) {
        it_gun *maybe_gun = dynamic_cast<it_gun *>(result.front().type);
        if (maybe_gun != NULL) {
            item ammo(default_ammo(maybe_gun->ammo), birthday);
            // TODO: change the spawn lists to contain proper references to containers
            ammo = ammo.in_its_container();
            result.push_back(ammo);
        }
    }
    return result;
}

item Item_group::create_single(int birthday, RecursionList &rec) const
{
    if (type == G_COLLECTION) {
        for(prop_list::const_iterator a = items.begin(); a != items.end(); ++a) {
            if(rng(0, 99) >= (*a)->probability) {
                continue;
            }
            return (*a)->create_single(birthday, rec);
        }
    } else if (type == G_DISTRIBUTION) {
        int p = rng(0, sum_prob - 1);
        for(prop_list::const_iterator a = items.begin(); a != items.end(); ++a) {
            p -= (*a)->probability;
            if (p >= 0) {
                continue;
            }
            return (*a)->create_single(birthday, rec);
        }
    }
    return item(null_item_id, birthday);
}

void Item_group::check_consistency() const
{
    for(prop_list::const_iterator a = items.begin(); a != items.end(); ++a) {
        (*a)->check_consistency();
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
    for(prop_list::const_iterator a = items.begin(); a != items.end(); ++a) {
        if ((*a)->has_item(itemid)) {
            return true;
        }
    }
    return false;
}
