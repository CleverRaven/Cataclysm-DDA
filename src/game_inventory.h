#pragma once
#ifndef GAME_INVENTORY_H
#define GAME_INVENTORY_H

#include <list>
#include <functional>
#include <utility>

#include "inventory_ui.h"

struct tripoint;

namespace cata
{
template<typename T>
class optional;
} // namespace cata
class avatar;
class item;
class item_location;
class player;
class salvage_actor;
class repair_item_actor;

using item_location_filter = std::function<bool ( const item_location & )>;

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
* More functions will follow. TODO: update all 'inv_for...()' functions to return @ref item_location instead of
* plain int and move them here.
* @return Either location of the selected item or null location if none was selected.
*/
/*@{*/

void common( avatar &you );
void compare( player &p, const cata::optional<tripoint> &offset );
void reassign_letter( player &p, item &it );
void swap_letters( player &p );

/**
 * Select items to drop.
 * @return A list of pairs of position, quantity.
 */
std::list<std::pair<int, int>> multidrop( player &p );

/** Consuming an item. */
item_location consume( player &p );
/** Consuming a food item via a custom menu. */
item_location consume_food( player &p );
/** Consuming a drink item via a custom menu. */
item_location consume_drink( player &p );
/** Consuming a medication item via a custom menu. */
item_location consume_meds( player &p );
/** Choosing a container for liquid. */
item_location container_for( avatar &you, const item &liquid, int radius = 0 );
/** Item disassembling menu. */
item_location disassemble( player &p );
/** Gunmod installation menu. */
item_location gun_to_modify( player &p, const item &gunmod );
/** Book reading menu. */
item_location read( avatar &you );
/** Menu for stealing stuff. */
item_location steal( avatar &you, player &victim );
/** Item activation menu. */
item_location use( avatar &you );
/** Item wielding/unwielding menu. */
item_location wield( avatar &you );
/** Item wielding/unwielding menu. */
item_location holster( player &p, item &holster );
/** Choosing a gun to saw down it's barrel. */
item_location saw_barrel( player &p, item &tool );
/** Choose item to wear. */
item_location wear( player &p );
/** Choose item to take off. */
item_location take_off( avatar &you );
/** Item cut up menu. */
item_location salvage( player &p, const salvage_actor *actor );
/** Repair menu. */
item_location repair( player &p, const repair_item_actor *actor, const item *main_tool );
/** Bionic install menu. */
item_location install_bionic( player &p, player &patient, bool surgeon = false );
/** Bionic uninstall menu. */
item_location uninstall_bionic( player &p, player &patient );
/**Autoclave sterilize menu*/
item_location sterilize_cbm( player &p );
/*@}*/

} // namespace inv

} // namespace game_menus

#endif // GAME_INVENTORY_H
