#include "item_factory.h"
#include "item_group.h"
#include "rng.h"
#include <map>
#include <algorithm>

Item_group::Item_group(const Item_tag id) : m_id(id), m_max_odds(0){
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

const Item_tag Item_group::get_id(std::vector<Item_tag> recursion_list){
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

const Item_tag Item_group_group::get(std::vector<Item_tag> recursion_list){
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
