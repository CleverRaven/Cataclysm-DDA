#ifndef _ITEM_GROUP_H_
#define _ITEM_GROUP_H_

#include <vector>

typedef std::string Item_tag;

class Item_group_entry;
class Item_group_group;

class Item_group
{
public:
    Item_group(Item_tag id);

    const Item_tag get_id();
    const Item_tag get_id(std::vector<Item_tag> recursion_list);
    void add_entry(const Item_tag item_id, int chance);
    void add_group(Item_group*, int chance);

    // Does this item group contain the given item?
    bool has_item(const Item_tag item_id);
    // Check that all items referenced here do actually exist (are defined)
    // this is not recursive
    void check_items_exist() const;
private:
    const Item_tag m_id;
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
    Item_group_entry(const Item_tag id, int upper_bound);

    bool check(int value) const;
    const Item_tag get() const;
private:
    const Item_tag m_id;
    int m_upper_bound;
};

class Item_group_group
{
public:
    Item_group_group(Item_group* group, int upper_bound);

    bool check(int value) const;
    const Item_tag get(std::vector<Item_tag> recursion_list);
private:
    Item_group* m_group;
    int m_upper_bound;
};
#endif
