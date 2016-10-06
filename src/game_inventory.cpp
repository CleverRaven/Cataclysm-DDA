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

static item_location inv_internal( player &u, const inventory_selector_preset &preset,
                                   const std::string &title, int radius,
                                   const std::string &none_message )
{
    u.inv.restack( &u );
    u.inv.sort();

    inventory_pick_selector inv_s( u, preset );

    inv_s.set_title( title );
    inv_s.set_display_stats( false );

    inv_s.add_character_items( u );
    inv_s.add_nearby_items( radius );

    if( inv_s.empty() ) {
        const std::string msg = none_message.empty()
                                ? _( "You don't have the necessary item at hand." )
                                : none_message;
        popup( msg, PF_GET_KEY );
        return item_location();
    }

    return inv_s.execute();
}

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
        inv_s.set_hint( string_format(
                            _( "Item hotkeys assigned: <color_ltgray>%d</color>/<color_ltgray>%d</color>" ),
                            u.allocated_invlets().size(), inv_chars.size() - u.allocated_invlets().size() ) );
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
    return u.get_item_position( inv_map_splice( filter, title, -1, none_message ).get_item() );
}

int game::inv_for_all( const std::string &title, const std::string &none_message )
{
    const std::string msg = ( none_message.empty() ) ? _( "Your inventory is empty." ) : none_message;
    return u.get_item_position( inv_internal( u, inventory_selector_preset(),
                                title, -1, none_message ).get_item() );
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
    return inv_internal( u, inventory_filter_preset( convert_filter( filter ) ),
                         title, radius, none_message );
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

    return inv_internal( u, inventory_filter_preset( filter ), title, radius,
                         string_format( _( "You don't have a suitable container for carrying %s." ),
                                        liquid.tname().c_str() ) ).get_item();
}

class gunmod_inventory_preset : public inventory_selector_preset
{
    public:
        gunmod_inventory_preset( const player &p, const item &gunmod ) : p( p ), gunmod( gunmod ) {
            append_cell( [ this ]( const item_location & loc ) {
                const auto odds = get_odds( loc );

                if( odds.first >= 100 ) {
                    return string_format( "<color_ltgreen>%s</color>", _( "always" ) );
                }

                return string_format( "<color_ltgreen>%d%%</color>", odds.first );
            }, _( "SUCCESS CHANCE" ) );

            append_cell( [ this ]( const item_location & loc ) {
                const auto odds = get_odds( loc );
                return odds.second > 0 ? string_format( "<color_red>%d%%</color>", odds.second ) : std::string();
            }, _( "DAMAGE RISK" ) );
        }

        bool is_shown( const item_location &loc ) const override {
            return loc->is_gun() && !loc->is_gunmod();
        }

        std::string get_denial( const item_location &loc ) const override {
            std::string incompatability;

            if( !loc->gunmod_compatible( gunmod, &incompatability ) ) {
                return incompatability;
            }

            if( !p.meets_requirements( gunmod, *loc ) ) {
                return string_format( _( "requires at least %s" ),
                                      p.enumerate_unmet_requirements( gunmod, *loc ).c_str() );
            }

            if( get_odds( loc ).first <= 0 ) {
                return _( "is too difficult for you to modify" );
            }

            return std::string();
        }

        bool sort_compare( const item_location &lhs, const item_location &rhs ) const override {
            const auto a = get_odds( lhs );
            const auto b = get_odds( rhs );

            if( a.first > b.first || ( a.first == b.first && a.second < b.second ) ) {
                return true;
            }

            return inventory_selector_preset::sort_compare( lhs, rhs );
        }

    protected:
        /** @return Odds for successful installation (pair.first) and gunmod damage (pair.second) */
        std::pair<int, int> get_odds( const item_location &gun ) const {
            return p.gunmod_installation_odds( *gun, gunmod );
        }

    private:
        const player &p;
        const item &gunmod;
};

item_location game::inv_for_gunmod( const item &gunmod, const std::string &title )
{
    return inv_internal( u, gunmod_inventory_preset( u, gunmod ),
                         title, -1, _( "You don't have any guns to modify." ) );
}

class read_inventory_preset: public inventory_selector_preset
{
    public:
        read_inventory_preset( const player &p ) : p( p ) {
            static const std::string unknown( _( "<color_dkgray>?</color>" ) );
            static const std::string martial_arts( _( "martial arts" ) );

            append_cell( [ this, &p ]( const item_location & loc ) -> std::string {
                if( loc->type->can_use( "MA_MANUAL" ) ) {
                    return martial_arts;
                }
                if( !is_known( loc ) ) {
                    return unknown;
                }
                const auto &book = get_book( loc );
                if( book.skill && p.get_skill_level( book.skill ).can_train() ) {
                    return string_format( _( "%s to %d" ), book.skill->name().c_str(), book.level );
                }
                return std::string();
            }, _( "TRAINS" ), unknown );

            append_cell( [ this ]( const item_location & loc ) -> std::string {
                if( !is_known( loc ) ) {
                    return unknown;
                }
                const auto &book = get_book( loc );
                const int unlearned = book.recipes.size() - get_known_recipes( book );

                return unlearned > 0 ? std::to_string( unlearned ) : std::string();
            }, _( "RECIPES" ), unknown );

            append_cell( [ this ]( const item_location & loc ) -> std::string {
                if( !is_known( loc ) ) {
                    return unknown;
                }
                const int fun = get_book( loc ).fun;
                if( fun > 0 ) {
                    return string_format( "<good>+%d</good>", fun );
                } else if( fun < 0 ) {
                    return string_format( "<bad>%d</bad>", fun );
                }
                return std::string();
            }, _( "FUN" ), unknown );

            append_cell( [ this, &p ]( const item_location & loc ) -> std::string {
                if( !is_known( loc ) ) {
                    return unknown;
                }
                std::vector<std::string> dummy;
                const player *reader = p.get_book_reader( *loc, dummy );
                if( reader == nullptr ) {
                    return std::string();  // Just to make sure
                }

                const int actual_time = p.time_to_read( *loc, *reader ) / MOVES( 1 );
                const int normal_time = get_book( loc ).time * reader->read_speed() / MOVES( 1 );
                const bool complicated_stuff = actual_time > normal_time;

                const std::string duration = calendar( actual_time ).textify_period();

                return complicated_stuff ? string_format( "<color_ltred>%s</color>", duration.c_str() ) : duration;
            }, _( "CHAPTER IN" ), unknown );
        }

        bool is_shown( const item_location &loc ) const override {
            return loc->is_book();
        }

        std::string get_denial( const item_location &loc ) const override {
            std::vector<std::string> denials;
            if( p.get_book_reader( *loc, denials ) == nullptr && !denials.empty() ) {
                return denials.front();
            }
            return std::string();
        }

    private:
        const islot_book &get_book( const item_location &loc ) const {
            return *loc->type->book.get();
        }

        bool is_known( const item_location &loc ) const {
            return p.has_identified( loc->typeId() );
        }

        int get_known_recipes( const islot_book &book ) const {
            int res = 0;
            for( auto const &elem : book.recipes ) {
                if( p.knows_recipe( elem.recipe ) ) {
                    ++res; // If the player knows it, they recognize it even if it's not clearly stated.
                }
            }
            return res;
        }

        const player &p;
};

item_location game::inv_for_books( const std::string &title )
{
    return inv_internal( u, read_inventory_preset( u ),
                         title, 1, _( "You have nothing to read." ) );
}

std::list<std::pair<int, int>> game::multidrop()
{
    u.inv.restack( &u );
    u.inv.sort();

    const inventory_filter_preset preset( [ this ]( const item_location & location ) {
        return u.can_unwield( *location, false );
    } );

    inventory_drop_selector inv_s( u, preset );

    inv_s.add_character_items( u );
    inv_s.set_title( _( "Multidrop:" ) );
    inv_s.set_hint( _( "To drop x items, type a number before selecting." ) );

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
    inv_s.set_hint( _( "Select two items to compare them." ) );

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
