#pragma once
#ifndef GAME_INVENTORY_H
#define GAME_INVENTORY_H

#include "enums.h"
#include "inventory_ui.h"

#include <list>
#include <string>

class item;
class item_location;
class player;
typedef std::function<bool( const item_location & )> item_location_filter;

class inventory_filter_preset : public inventory_selector_preset
{
    public:
        inventory_filter_preset( const item_location_filter &filter );

        bool is_shown( const item_location &location ) const override;

    private:
        item_location_filter filter;
};
namespace game_menus
{

namespace inv
{

/**
* @name Customized inventory menus
*
* The functions here execute customized inventory menus for specific game situations.
* Each menu displays only related inventory (or nearby) items along with context dependent information.
* More functions will follow. @todo: update all 'inv_for...()' functions to return @ref item_location instead of
* plain int and move them here.
* @return Either location of the selected item or null location if none was selected.
*/
/*@{*/

void common( player &p );
void compare( player &p, const tripoint &offset = tripoint_min );
void reassign_letter( player &p, item &it );
void swap_letters( player &p );

/**
 * Select items to drop.
 * @return A list of pairs of position, quantity.
 */
std::list<std::pair<int, int>> multidrop( player &p );

/** Consuming an item. */
item_location consume( player &p );
/** Choosing a container for liquid. */
item_location container_for( player &p, const item &liquid, int radius = 0 );
/** Item disassembling menu. */
item_location disassemble( player &p );
/** Gunmod installation menu. */
item_location gun_to_modify( player &p, const item &gunmod );
/** Book reading menu. */
item_location read( player &p );
/** Menu for stealing stuff. */
item_location steal( player &p, player &victim );
/** Item activation menu. */
item_location use( player &p );
/** Item wielding/unwielding menu. */
item_location wield( player &p );
/** Item wielding/unwielding menu. */
item_location holster( player &p, item &holster );
/** Choosing a gun to saw down it's barrel. */
item_location saw_barrel( player &p, item &tool );
/** Choose item to wear. */
item_location wear( player &p );
/** Choose item to take off. */
item_location take_off( player &p );
/*@}*/

}

}

#endif // GAME_INVENTORY_H
