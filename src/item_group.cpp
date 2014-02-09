#include "item_factory.h"
#include "item_group.h"
#include "rng.h"
#include <map>
#include <algorithm>

static const std::string null_item_id("null");

Item_group::Item_group(const Item_tag id)
: m_id(id)
, m_max_odds(0)
, m_guns_have_ammo(false)
{
}

void Item_group::check_items_exist() const {
    for(std::vector<Item_group_entry*>::const_iterator iter = m_entries.begin(); iter != m_entries.end(); ++iter){
        const Item_tag itag = (*iter)->get();
        if(!item_controller->has_template(itag)) {
            debugmsg("%s in group %s is not a valid item type", itag.c_str(), m_id.c_str());
        }
    }
}

// When selecting an id from this group, the value returned is determined based on the odds
// given when inserted into the group.
const Item_tag Item_group::get_id(){
    //Create a list of visited groups - in case of recursion, this will be needed to throw an error.
    std::vector<Item_tag> recursion_list;
    return get_id(recursion_list);
}

// true if guns in this group and subgroups have ammo. (Non recursively recursive). Yet_another_taglist_one_day
bool Item_group::guns_have_ammo() {
    return (m_guns_have_ammo == true);
}

const Item_tag Item_group::get_id(std::vector<Item_tag> &recursion_list){
    if (m_max_odds == 0) {
        // No items in this group
        return null_item_id;
    }
    int rolled_value = rng(1,m_max_odds)-1;

    //Insure we haven't already visited this group
    if(std::find(recursion_list.begin(), recursion_list.end(), m_id) != recursion_list.end()){
        std::string error_message = "Recursion loop occured in retrieval from item group "+m_id+". Recursion backtrace follows:\n";
        for(std::vector<Item_tag>::iterator iter = recursion_list.begin(); iter != recursion_list.end(); ++iter){
            error_message+=*iter;
        }
        debugmsg(error_message.c_str());
        return "MISSING_ITEM";
    }
    //So we don't visit this item group again
    recursion_list.push_back(m_id);

    //First, check through groups.
    //Groups are assigned first, and will get the lower value odds.
    for(std::vector<Item_group_group*>::iterator iter = m_groups.begin(); iter != m_groups.end(); ++iter){
        if((*iter)->check(rolled_value)){
            return (*iter)->get(recursion_list);
        }
    }

    //If no match was found in groups, check in entries
    for(std::vector<Item_group_entry*>::iterator iter = m_entries.begin(); iter != m_entries.end(); ++iter){
        if((*iter)->check(rolled_value)){
            return (*iter)->get();
        }
    }

    debugmsg("There was an unknown error in Item_group::get_id.");
    return "MISSING_ITEM";
}


void Item_group::add_entry(const Item_tag id, int chance){
    m_max_odds = m_max_odds+chance;
    Item_group_entry* new_entry = new Item_group_entry(id, m_max_odds);
    m_entries.push_back(new_entry);
}

void Item_group::add_group(Item_group* group, int chance){
    m_max_odds = m_max_odds+chance;
    Item_group_group* new_group = new Item_group_group(group, m_max_odds);
    m_groups.push_back(new_group);
}


//Item_group_entry definition
Item_group_entry::Item_group_entry(const Item_tag id, int upper_bound): m_id(id), m_upper_bound(upper_bound){
}

bool Item_group_entry::check(int value) const{
    return (value < m_upper_bound);
}

const Item_tag Item_group_entry::get() const{
    return m_id;
}

//Item_group_group definitions
Item_group_group::Item_group_group(Item_group* group, int upper_bound): m_group(group), m_upper_bound(upper_bound){
}

bool Item_group_group::check(int value) const{
    return (value < m_upper_bound);
}

const Item_tag Item_group_group::get(std::vector<Item_tag> &recursion_list){
    return m_group->get_id(recursion_list);
}

bool Item_group::has_item(const Item_tag item_id) {
    for(int i=0; i<m_entries.size(); i++) {
        if(m_entries[i]->get() == item_id) {
            return 1;
        }
    }

    return 0;
}

void Item_group::remove_item(const Item_tag &item_id) {
    std::set<std::string> rec;
    remove_item(item_id, rec);
}

void Item_group::remove_item(const Item_tag &item_id, std::set<std::string> &rec) {
    if(rec.count(m_id) > 0) {
        return;
    }
    rec.insert(m_id);
    // If this removes an item/group, m_max_odds must be decreased
    // by the chance of the removed item/group. But the chance of
    // that is not directly stored, but as offset from the previous
    // item/group (see add_entry/add_group)
    int delta_max_odds = 0;
    int prev_upper_bound = 0;
    for(size_t i = 0; i < m_groups.size(); i++) {
        m_groups[i]->m_group->remove_item(item_id, rec);
        if(m_groups[i]->m_group->m_max_odds == 0) {
            delta_max_odds += (m_groups[i]->m_upper_bound - prev_upper_bound);
            delete m_groups[i];
            m_groups.erase(m_groups.begin() + i);
            i--;
        } else {
            m_groups[i]->m_upper_bound -= delta_max_odds;
            prev_upper_bound = m_groups[i]->m_upper_bound;
        }
    }
    for(size_t i = 0; i < m_entries.size(); i++) {
        if(m_entries[i]->m_id == item_id) {
            delta_max_odds += (m_entries[i]->m_upper_bound - prev_upper_bound);
            delete m_entries[i];
            m_entries.erase(m_entries.begin() + i);
            i--;
        } else {
            m_entries[i]->m_upper_bound -= delta_max_odds;
            prev_upper_bound = m_entries[i]->m_upper_bound;
        }
    }
    m_max_odds -= delta_max_odds;
}
