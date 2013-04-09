#ifndef _ITEM_GROUP_H_
#define _ITEM_GROUP_H_

#include "item_manager.h"
#include <vector>

class Item_group_entry;
class Item_group_group;

class Item_group
{
public:
    Item_group(item_tag id);
    
    const item_tag get_id();
    const item_tag get_id(std::vector<item_tag> recursion_list);
    void add_entry(const item_tag item_id, int chance);
    void add_group(Item_group*, int chance);
private:
    const item_tag m_id;
    int m_max_odds;
    std::vector<Item_group_group*> m_groups;
    std::vector<Item_group_entry*> m_entries;
};

//Item group entries involve a string value, and an upper bound
//Since items beforehand have already been checked, there is no need
//to add a lower bound value.
class Item_group_entry
{
public:
    Item_group_entry(const item_tag id, int upper_bound);

    bool check(int value) const;
    const item_tag get() const;
private:
    const item_tag m_id;
    int m_upper_bound;
};

class Item_group_group
{
public:
    Item_group_group(Item_group* group, int upper_bound);

    bool check(int value) const;
    const item_tag get(std::vector<item_tag> recursion_list);
private:
    Item_group* m_group;
    int m_upper_bound;
};
#endif
