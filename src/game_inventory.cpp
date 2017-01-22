#include "game_inventory.h"

#include "game.h"
#include "inventory_ui.h"
#include "options.h"
#include "player.h"
#include "crafting.h"
#include "recipe_dictionary.h"
#include "item.h"
#include "itype.h"

#include <functional>

typedef std::function<bool( const item & )> item_filter;
typedef std::function<bool( const item_location & )> item_location_filter;

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

item_location_filter convert_filter( const item_filter &filter )
{
    return [ &filter ]( const item_location & loc ) {
        return filter( *loc );
    };
}

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

void game_menus::inv::common( player &p )
{
    static const std::set<int> allowed_selections = { { ' ', '.', 'q', '=', '\n', KEY_LEFT, KEY_ESCAPE } };

    p.inv.restack( &p );
    p.inv.sort();

    inventory_pick_selector inv_s( p );

    inv_s.add_character_items( p );
    inv_s.set_title( _( "Inventory" ) );

    int res;
    do {
        inv_s.set_hint( string_format(
                            _( "Item hotkeys assigned: <color_ltgray>%d</color>/<color_ltgray>%d</color>" ),
                            p.allocated_invlets().size(), inv_chars.size() - p.allocated_invlets().size() ) );
        const item_location &location = inv_s.execute();
        if( location == item_location::nowhere ) {
            break;
        }
        g->refresh_all();
        res = g->inventory_item_menu( p.get_item_position( location.get_item() ) );
        g->refresh_all();
    } while( allowed_selections.count( res ) != 0 );
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

int game_menus::inv::take_off( player &p )
{
    return g->inv_for_filter( _( "Take off item" ), [ &p ]( const item & it ) {
        return p.is_worn( it );
    }, _( "You don't wear anything." ) );
}

int game_menus::inv::wear( player &p )
{
    return g->inv_for_filter( _( "Wear item" ), [ &p ]( const item & it ) {
        return it.is_armor() && !p.is_worn( it );
    }, _( "You don't have any items to wear." ) );
}

item_location game::inv_map_splice( item_filter filter, const std::string &title, int radius,
                                    const std::string &none_message )
{
    return inv_internal( u, inventory_filter_preset( convert_filter( filter ) ),
                         title, radius, none_message );
}

item_location game_menus::inv::container_for( player &p, const item &liquid, int radius )
{
    const auto filter = [ &liquid ]( const item_location & location ) {
        if( location.where() == item_location::type::character ) {
            Character *character = dynamic_cast<Character *>( g->critter_at( location.position() ) );
            if( character == nullptr ) {
                debugmsg( "Invalid location supplied to the liquid filter: no character found." );
                return false;
            }
            return location->get_remaining_capacity_for_liquid( liquid, *character ) > 0;
        }

        const bool allow_buckets = location.where() == item_location::type::map;
        return location->get_remaining_capacity_for_liquid( liquid, allow_buckets ) > 0;
    };

    return inv_internal( p, inventory_filter_preset( filter ),
                         string_format( _( "Container for %s" ), liquid.display_name( liquid.charges ).c_str() ), radius,
                         string_format( _( "You don't have a suitable container for carrying %s." ),
                                        liquid.tname().c_str() ) );
}

class pickup_inventory_preset : public inventory_selector_preset
{
    public:
        pickup_inventory_preset( const player &p ) : p( p ) {}

        std::string get_denial( const item_location &loc ) const override {
            if( !p.has_item( *loc ) ) {
                if( loc->made_of( LIQUID ) ) {
                    return _( "Can't pick up liquids" );
                } else if( !p.can_pickVolume( *loc ) ) {
                    return _( "Too big to pick up" );
                } else if( !p.can_pickWeight( *loc ) ) {
                    return _( "Too heavy to pick up" );
                }
            }

            return std::string();
        }

    private:
        const player &p;
};

class disassemble_inventory_preset : public pickup_inventory_preset
{
    public:
        disassemble_inventory_preset( const player &p, const inventory &inv ) :
            pickup_inventory_preset( p ), p( p ), inv( inv ) {

            append_cell( [ this ]( const item_location & loc ) {
                const auto &req = get_recipe( loc ).disassembly_requirements();
                if( req.is_empty() ) {
                    return std::string();
                }
                const auto components = req.get_components();
                return enumerate_as_string( components.begin(), components.end(),
                []( const decltype( components )::value_type & comps ) {
                    return comps.front().to_string();
                } );
            }, _( "YIELD" ) );

            append_cell( [ this ]( const item_location & loc ) {
                return calendar( get_recipe( loc ).time / 100 ).textify_period();
            }, _( "TIME" ) );
        }

        bool is_shown( const item_location &loc ) const override {
            return get_recipe( loc );
        }

        std::string get_denial( const item_location &loc ) const override {
            std::string denial;
            if( !p.can_disassemble( *loc, inv, &denial ) ) {
                return denial;
            }
            return pickup_inventory_preset::get_denial( loc );
        }

    protected:
        const recipe &get_recipe( const item_location &loc ) const {
            return recipe_dictionary::get_uncraft( loc->typeId() );
        }

    private:
        const player &p;
        const inventory inv;
};

item_location game_menus::inv::disassemble( player &p )
{
    return inv_internal( p, disassemble_inventory_preset( p, p.crafting_inventory() ),
                         _( "Disassemble item" ), 1,
                         _( "You don't have any items you could disassemble." ) );
}

class activatable_inventory_preset : public pickup_inventory_preset
{
    public:
        activatable_inventory_preset( const player &p ) : pickup_inventory_preset( p ), p( p ) {
            if( get_option<bool>( "INV_USE_ACTION_NAMES" ) ) {
                append_cell( [ this ]( const item_location & loc ) {
                    return string_format( "<color_ltgreen>%s</color>", get_action_name( *loc ).c_str() );
                }, _( "ACTION" ) );
            }
        }

        bool is_shown( const item_location &loc ) const override {
            return p.rate_action_use( *loc ) != HINT_CANT && !get_action_name( *loc ).empty();
        }

        std::string get_denial( const item_location &loc ) const override {
            if( !p.has_enough_charges( *loc, false ) ) {
                return string_format(
                           ngettext( _( "Needs at least %d charge" ),
                                     _( "Needs at least %d charges" ), loc->ammo_required() ),
                           loc->ammo_required() );
            }

            return pickup_inventory_preset::get_denial( loc );
        }

    protected:
        std::string get_action_name( const item &it ) const {
            const auto &uses = it.type->use_methods;

            if( uses.empty() ) {
                if( it.is_food() || it.is_medication() ) {
                    return _( "Consume" );
                } else if( it.is_book() ) {
                    return _( "Read" );
                } else if( it.is_bionic() ) {
                    return _( "Install bionic" );
                }
            } else if( uses.size() == 1 ) {
                return uses.begin()->second.get_name();
            } else {
                return _( "..." );
            }

            if( !it.is_container_empty() ) {
                return get_action_name( it.get_contained() );
            }

            return std::string();
        }

    private:
        const player &p;
};

item_location game_menus::inv::use( player &p )
{
    return inv_internal( p, activatable_inventory_preset( p ),
                         _( "Use item" ), 1,
                         _( "You don't have any items you can use." ) );
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

item_location game_menus::inv::gun_to_modify( player &p, const item &gunmod )
{
    return inv_internal( p, gunmod_inventory_preset( p, gunmod ),
                         _( "Select gun to modify" ), -1,
                         _( "You don't have any guns to modify." ) );
}

class read_inventory_preset: public pickup_inventory_preset
{
    public:
        read_inventory_preset( const player &p ) : pickup_inventory_preset( p ), p( p ) {
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

                return unlearned > 0 ? to_string( unlearned ) : std::string();
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
                // Actual reading time (in turns). Can be penalized.
                const int actual_turns = p.time_to_read( *loc, *reader ) / MOVES( 1 );
                // Theoretical reading time (in turns) based on the reader speed. Free of penalties.
                const int normal_turns = get_book( loc ).time * reader->read_speed() / MOVES( 1 );
                const std::string duration = calendar::print_approx_duration( actual_turns, false );

                if( actual_turns > normal_turns ) { // Longer - complicated stuff.
                    return string_format( "<color_ltred>%s</color>", duration.c_str() );
                }

                return duration; // Normal speed.
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
            return pickup_inventory_preset::get_denial( loc );
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

item_location game_menus::inv::read( player &p )
{
    return inv_internal( p, read_inventory_preset( p ),
                         _( "Read" ), 1,
                         _( "You have nothing to read." ) );
}

class steal_inventory_preset : public pickup_inventory_preset
{
    public:
        steal_inventory_preset( const player &p, const player &victim ) :
            pickup_inventory_preset( p ), victim( victim ) {}

        bool is_shown( const item_location &loc ) const override {
            return !victim.is_worn( *loc ) && &victim.weapon != &( *loc );
        }

    private:
        const player &victim;
};

item_location game_menus::inv::steal( player &p, player &victim )
{
    return inv_internal( victim, steal_inventory_preset( p, victim ),
                         string_format( _( "Steal from %s" ), victim.name.c_str() ), -1,
                         string_format( _( "%s's inventory is empty." ), victim.name.c_str() ) );
}

std::list<std::pair<int, int>> game_menus::inv::multidrop( player &p )
{
    p.inv.restack( &p );
    p.inv.sort();

    const inventory_filter_preset preset( [ &p ]( const item_location & location ) {
        return p.can_unwield( *location, false );
    } );

    inventory_drop_selector inv_s( p, preset );

    inv_s.add_character_items( p );
    inv_s.set_title( _( "Multidrop" ) );
    inv_s.set_hint( _( "To drop x items, type a number before selecting." ) );

    if( inv_s.empty() ) {
        popup( std::string( _( "You have nothing to drop." ) ), PF_GET_KEY );
        return std::list<std::pair<int, int> >();
    }

    return inv_s.execute();
}

void game_menus::inv::compare( player &p, const tripoint &offset )
{
    p.inv.restack( &p );
    p.inv.sort();

    inventory_compare_selector inv_s( p );

    inv_s.add_character_items( p );
    inv_s.set_title( _( "Compare" ) );
    inv_s.set_hint( _( "Select two items to compare them." ) );

    if( offset != tripoint_min ) {
        inv_s.add_map_items( p.pos() + offset );
        inv_s.add_vehicle_items( p.pos() + offset );
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
                                 0, TERMY - VIEW_OFFSET_Y * 2, sItemCh, sItemTn, vItemCh, vItemLastCh,
                                 iScrollPos ).get_first_input();

            if( ch == KEY_PPAGE ) {
                iScrollPos--;
                iScrollPosLast--;
            } else if( ch == KEY_NPAGE ) {
                iScrollPos++;
                iScrollPosLast++;
            }

            g->refresh_all();
        } while( ch == KEY_PPAGE || ch == KEY_NPAGE );
    } while( true );
}

void game_menus::inv::reassign_letter( player &p, item &it )
{
    while( true ) {
        const long invlet = popup_getkey(
                                _( "Enter new letter (press SPACE for none, ESCAPE to cancel)." ) );

        if( invlet == KEY_ESCAPE ) {
            break;
        } else if( invlet == ' ' ) {
            p.reassign_item( it, 0 );
            break;
        } else if( inv_chars.valid( invlet ) ) {
            p.reassign_item( it, invlet );
            break;
        }
    }
}

void game_menus::inv::swap_letters( player &p )
{
    p.inv.restack( &p );
    p.inv.sort();

    inventory_pick_selector inv_s( p );

    inv_s.add_character_items( p );
    inv_s.set_title( _( "Swap Inventory Letters" ) );
    inv_s.set_display_stats( false );

    if( inv_s.empty() ) {
        popup( std::string( _( "Your inventory is empty." ) ), PF_GET_KEY );
        return;
    }

    while( true ) {
        const std::string invlets = colorize_symbols( inv_chars.get_allowed_chars(),
        [ &p ]( const std::string::value_type & elem ) {
            if( p.assigned_invlet.count( elem ) ) {
                return c_yellow;
            } else if( p.invlet_to_position( elem ) != INT_MIN ) {
                return c_white;
            } else {
                return c_dkgray;
            }
        } );

        inv_s.set_hint( invlets );

        auto loc = inv_s.execute();

        if( !loc ) {
            break;
        }

        reassign_letter( p, *loc );
        g->refresh_all();
    }
}
