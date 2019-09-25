#pragma once
#ifndef CRAFTING_H
#define CRAFTING_H

#include <list>
#include <string>

class item;
class player;
class recipe;

// removes any (removable) ammo from the item and stores it in the
// players inventory.
void remove_ammo( item &dis_item, player &p );
// same as above but for each item in the list
void remove_ammo( std::list<item> &dis_items, player &p );

const recipe *select_crafting_recipe( int &batch_size, const std::string &filter = "" );

#endif
