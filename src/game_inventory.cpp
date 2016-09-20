#include "game.h"

#include "inventory_ui.h"
#include "player.h"
#include "item.h"
#include "itype.h"

class inventory_filter_preset : public inventory_selector_preset
{
    public:
        inventory_filter_preset( const item_location_filter &filter ) : filter( filter ) {}

        bool is_shown( const item_location &location ) const override {
            return filter( location );
        }

    private:
        item_location_filter filter;
};

void game::interactive_inv()
{
    static const std::set<int> allowed_selections = { { ' ', '.', 'q', '=', '\n', KEY_LEFT, KEY_ESCAPE } };

    u.inv.restack( &u );
    u.inv.sort();

    inventory_pick_selector inv_s( u );

    inv_s.add_character_items( u );
    inv_s.set_title( _( "Inventory:" ) );

    int res;
    do {
        const item_location &location = inv_s.execute();
        if( location == item_location::nowhere ) {
            break;
        }
        refresh_all();
        res = inventory_item_menu( u.get_item_position( location.get_item() ) );
    } while( allowed_selections.count( res ) != 0 );
}

item_location_filter convert_filter( const item_filter &filter )
{
    return [ &filter ]( const item_location & loc ) {
        return filter( *loc );
    };
}

int game::inv_for_filter( const std::string &title, item_filter filter,
                          const std::string &none_message )
{
    return inv_for_filter( title, convert_filter( filter ), none_message );
}

int game::inv_for_filter( const std::string &title, item_location_filter filter,
                          const std::string &none_message )
{
    return u.get_item_position( inv_map_splice( filter, title, -1, none_message ).get_item() );
}

int game::inv_for_all( const std::string &title, const std::string &none_message )
{
    const std::string msg = ( none_message.empty() ) ? _( "Your inventory is empty." ) : none_message;
    return inv_for_filter( title, allow_all_items, msg );
}

int game::inv_for_activatables( const player &p, const std::string &title )
{
    return inv_for_filter( title, [ &p ]( const item & it ) {
        return p.rate_action_use( it ) != HINT_CANT;
    }, _( "You don't have any items you can use." ) );
}

int game::inv_for_flag( const std::string &flag, const std::string &title )
{
    return inv_for_filter( title, [ &flag ]( const item & it ) {
        return it.has_flag( flag );
    } );
}

int game::inv_for_id( const itype_id &id, const std::string &title )
{
    return inv_for_filter( title, [ &id ]( const item & it ) {
        return it.typeId() == id;
    }, string_format( _( "You don't have a %s." ), item::nname( id ).c_str() ) );
}

int game::inv_for_tools_powered_by( const ammotype &battery_id, const std::string &title )
{
    return inv_for_filter( title, [ &battery_id ]( const item & it ) {
        return it.is_tool() && it.ammo_type() == battery_id;
    }, string_format( _( "You don't have %s-powered tools." ), ammo_name( battery_id ).c_str() ) );
}

int game::inv_for_equipped( const std::string &title )
{
    return inv_for_filter( title, [ this ]( const item & it ) {
        return u.is_worn( it );
    }, _( "You don't wear anything." ) );
}

int game::inv_for_unequipped( const std::string &title )
{
    return inv_for_filter( title, [ this ]( const item & it ) {
        return it.is_armor() && !u.is_worn( it );
    }, _( "You don't have any items to wear." ) );
}

item_location game::inv_map_splice( item_filter filter, const std::string &title, int radius,
                                    const std::string &none_message )
{
    return inv_map_splice( convert_filter( filter ), title, radius, none_message );
}

item_location game::inv_map_splice( item_location_filter filter, const std::string &title,
                                    int radius,
                                    const std::string &none_message )
{
    u.inv.restack( &u );
    u.inv.sort();

    inventory_filter_preset preset( filter );
    inventory_pick_selector inv_s( u, preset );

    inv_s.add_character_items( u );
    inv_s.add_nearby_items( radius );
    inv_s.set_title( title );

    if( inv_s.empty() ) {
        const std::string msg = ( none_message.empty() ) ? _( "You don't have the necessary item at hand." )
                                : none_message;
        popup( msg, PF_GET_KEY );
        return item_location();
    }

    return inv_s.execute();
}

item *game::inv_map_for_liquid( const item &liquid, const std::string &title, int radius )
{
    const auto filter = [ this, &liquid ]( const item_location & location ) {
        if( location.where() == item_location::type::character ) {
            Character *character = dynamic_cast<Character *>( critter_at( location.position() ) );
            if( character == nullptr ) {
                debugmsg( "Invalid location supplied to the liquid filter: no character found." );
                return false;
            }
            return location->get_remaining_capacity_for_liquid( liquid, *character ) > 0;
        }

        const bool allow_buckets = location.where() == item_location::type::map;
        return location->get_remaining_capacity_for_liquid( liquid, allow_buckets ) > 0;
    };

    return inv_map_splice( filter, title, radius,
                           string_format( _( "You don't have a suitable container for carrying %s." ),
                                          liquid.type_name( 1 ).c_str() ) ).get_item();
}

std::list<std::pair<int, int>> game::multidrop()
{
    u.inv.restack( &u );
    u.inv.sort();

    inventory_filter_preset preset(
    [ this ]( const item_location & location ) -> bool {
        return u.can_unwield( *location, false );
    } );
    inventory_drop_selector inv_s( u, preset );

    inv_s.add_character_items( u );
    inv_s.set_title( _( "Multidrop:" ) );

    if( inv_s.empty() ) {
        popup( std::string( _( "You have nothing to drop." ) ), PF_GET_KEY );
        return std::list<std::pair<int, int> >();
    }
    return inv_s.execute();
}

void game::compare( const tripoint &offset )
{
    u.inv.restack( &u );
    u.inv.sort();

    inventory_compare_selector inv_s( u );

    inv_s.add_character_items( u );
    inv_s.set_title( _( "Compare:" ) );

    if( offset != tripoint_min ) {
        inv_s.add_map_items( u.pos() + offset );
        inv_s.add_vehicle_items( u.pos() + offset );
    } else {
        inv_s.add_nearby_items();
    }

    if( inv_s.empty() ) {
        popup( std::string( _( "There are no items to compare." ) ), PF_GET_KEY );
        return;
    }

    do {
        const auto to_compare = inv_s.execute();

        if( to_compare.first == nullptr || to_compare.second == nullptr ) {
            break;
        }

        std::vector<iteminfo> vItemLastCh, vItemCh;
        std::string sItemLastCh, sItemCh, sItemLastTn, sItemTn;

        to_compare.first->info( true, vItemCh );
        sItemCh = to_compare.first->tname();
        sItemTn = to_compare.first->type_name();

        to_compare.second->info( true, vItemLastCh );
        sItemLastCh = to_compare.second->tname();
        sItemLastTn = to_compare.second->type_name();

        int iScrollPos = 0;
        int iScrollPosLast = 0;
        int ch = ( int ) ' ';

        do {
            draw_item_info( 0, ( TERMX - VIEW_OFFSET_X * 2 ) / 2, 0, TERMY - VIEW_OFFSET_Y * 2,
                            sItemLastCh, sItemLastTn, vItemLastCh, vItemCh, iScrollPosLast, true ); //without getch(
            ch = draw_item_info( ( TERMX - VIEW_OFFSET_X * 2 ) / 2, ( TERMX - VIEW_OFFSET_X * 2 ) / 2,
                                 0, TERMY - VIEW_OFFSET_Y * 2, sItemCh, sItemTn, vItemCh, vItemLastCh, iScrollPos );

            if( ch == KEY_PPAGE ) {
                iScrollPos--;
                iScrollPosLast--;
            } else if( ch == KEY_NPAGE ) {
                iScrollPos++;
                iScrollPosLast++;
            }
        } while( ch == KEY_PPAGE || ch == KEY_NPAGE );
    } while( true );
}
