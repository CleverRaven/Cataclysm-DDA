#pragma once
#ifndef CATA_SRC_GAME_INVENTORY_H
#define CATA_SRC_GAME_INVENTORY_H

#include <functional>
#include <iosfwd>
#include <list>
#include <utility>

#include "inventory_ui.h"
#include "item_location.h"
#include "type_id.h"

class Character;
struct tripoint;

namespace cata
{
template<typename T>
class optional;
} // namespace cata
class avatar;
class item;
class repair_item_actor;
class salvage_actor;

using item_filter = std::function<bool( const item & )>;
using item_location_filter = std::function<bool ( const item_location & )>;
using drop_location = std::pair<item_location, int>;
using drop_locations = std::list<drop_location>;

class inventory_filter_preset : public inventory_selector_preset
{
    public:
        explicit inventory_filter_preset( const item_location_filter &filter );

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
                                  const std::string &title, int radius = -1, const std::string &none_message = "" );
item_location titled_filter_menu( const item_location_filter &filter, avatar &you,
                                  const std::string &title, int radius = -1, const std::string &none_message = "" );

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
void compare( avatar &you, const cata::optional<tripoint> &offset );
void reassign_letter( avatar &you, item &it );
void swap_letters( avatar &you );

/**
* Compares two items, if confirm_message isn't empty then it will be printed
* to the screen and function return value will be true if confirm button was
* pressed, false for "quit" input.
* @return False if confirm_message is empty or QUIT input was pressed.
*/
bool compare_items( const item &first, const item &second,
                    const std::string &confirm_message = "" );

/**
 * Select items to drop.
 * @return A list of pairs of item_location, quantity.
 */
drop_locations multidrop( avatar &you );
/**
 * Select items to pick up.
 * If target is provided, pick up items only from that tile (presumably adjacent to the avatar).
 * Otherwise, pick up items from the avatar's current location and all adjacent tiles.
 * @return A list of pairs of item_location, quantity.
 */
drop_locations pickup( avatar &you, const cata::optional<tripoint> &target = cata::nullopt,
                       const std::vector<drop_location> &selection = {} );

drop_locations smoke_food( Character &you, units::volume total_capacity,
                           units::volume used_capacity );

/**
* Consume an item via a custom menu.
* If item_location is provided then consume only from the contents of that container.
*/
item_location consume( avatar &you, const item_location &loc = item_location() );
/** Consuming a food item via a custom menu. */
item_location consume_food( avatar &you );
/** Consuming a drink item via a custom menu. */
item_location consume_drink( avatar &you );
/** Consuming a medication item via a custom menu. */
item_location consume_meds( avatar &you );
/** Choosing a container for liquid. */
item_location container_for( Character &you, const item &liquid, int radius = 0,
                             const item *avoid = nullptr );
/** Item disassembling menu. */
item_location disassemble( Character &you );
/** Gunmod installation menu. */
item_location gun_to_modify( Character &you, const item &gunmod );
/** Book reading menu. */
item_location read( Character &you );
/** eBook reading menu. */
item_location ebookread( Character &you, item_location &ereader );
/** Menu for stealing stuff. */
item_location steal( avatar &you, Character &victim );
/** Item activation menu. */
item_location use( avatar &you );
/** Item wielding/unwielding menu. */
item_location wield( avatar &you );
/** Item wielding/unwielding menu. */
drop_locations holster( avatar &you, const item_location &holster );
void insert_items( avatar &you, item_location &holster );
drop_locations unload_container( avatar &you );
/** Choosing a gun to saw down it's barrel. */
item_location saw_barrel( Character &you, item &tool );
/** Choosing a gun to saw down its barrel. */
item_location saw_stock( Character &you, item &tool );
/** Choosing an item to attach to a load bearing vest. */
item_location molle_attach( Character &you, item &tool );
/** Choose item to wear. */
item_location wear( Character &you, const bodypart_id &bp = bodypart_id( "bp_null" ) );
/** Choose item to take off. */
item_location take_off( avatar &you );
/** Item cut up menu. */
item_location salvage( Character &you, const salvage_actor *actor );
/** Repair menu. */
item_location repair( Character &you, const repair_item_actor *actor, const item *main_tool );
/** Bionic install menu. */
item_location install_bionic( Character &you, Character &patient, bool surgeon = false );
/**Autoclave sterilize menu*/
item_location sterilize_cbm( Character &you );
/*@}*/

} // namespace inv

} // namespace game_menus

#endif // CATA_SRC_GAME_INVENTORY_H
