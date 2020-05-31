#pragma once
#ifndef CATA_SRC_GAME_INVENTORY_H
#define CATA_SRC_GAME_INVENTORY_H

#include <functional>
#include <list>
#include <string>
#include <utility>

#include "inventory_ui.h"
#include "item_location.h"

struct tripoint;

namespace cata
{
template<typename T>
class optional;
} // namespace cata
class avatar;
class item;
class player;
class repair_item_actor;
class salvage_actor;

using item_filter = std::function<bool( const item & )>;
using item_location_filter = std::function<bool ( const item_location & )>;
using drop_location = std::pair<item_location, int>;
using drop_locations = std::list<drop_location>;

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
// item selector for all items in @you's inventory.
item_location titled_menu( avatar &you, const std::string &title,
                           const std::string &none_message = "" );
// item selector for items in @you's inventory with a filter
item_location titled_filter_menu( const item_filter &filter, avatar &you,
                                  const std::string &title, const std::string &none_message = "" );
item_location titled_filter_menu( const item_location_filter &filter, avatar &you,
                                  const std::string &title, const std::string &none_message = "" );

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
void common( item_location &loc, avatar &you );
void compare( player &p, const cata::optional<tripoint> &offset );
void reassign_letter( player &p, item &it );
void swap_letters( player &p );

/**
 * Select items to drop.
 * @return A list of pairs of item_location, quantity.
 */
drop_locations multidrop( player &p );

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
item_location read( player &pl );
/** Menu for stealing stuff. */
item_location steal( avatar &you, player &victim );
/** Item activation menu. */
item_location use( avatar &you );
/** Item wielding/unwielding menu. */
item_location wield( avatar &you );
/** Item wielding/unwielding menu. */
drop_locations holster( player &p, const item_location &holster );
void insert_items( avatar &you, item_location &holster );
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

#endif // CATA_SRC_GAME_INVENTORY_H
