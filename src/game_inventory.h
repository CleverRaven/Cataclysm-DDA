#ifndef GAME_INVENTORY_H
#define GAME_INVENTORY_H

#include "enums.h"

#include <list>
#include <string>

class item;
class item_location;
class player;

namespace game_menus
{

namespace inv
{

/**
* @name Customized inventory menus
*
* The functions here execute customized inventory menus for specific game situations.
* Each menu displays only related inventory (or nearby) items along with context dependent information.
* More functions will follow. @todo update all 'inv_for...()' functions to return @ref item_location instead of @ref int and move them here.
* @return Either location of the selected item or null location if none was selected.
*/
/*@{*/

void common( player &p );
void compare( player &p, const tripoint &offset = tripoint_min );
void reassign_letter( player &p, item &it );
void swap_letters( player &p );

/** Todo: let them return item_location */
int take_off( player &p );
int wear( player &p );

/**
 * Select items to drop.
 * @return A list of pairs of position, quantity.
 */
std::list<std::pair<int, int>> multidrop( player &p );

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
/*@}*/

}

}

#endif // GAME_INVENTORY_H
