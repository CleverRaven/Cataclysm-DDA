#include "item_contents.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <string>
#include <type_traits>

#include "character.h"
#include "color.h"
#include "cursesdef.h"
#include "debug.h"
#include "enum_conversions.h"
#include "enums.h"
#include "flat_set.h"
#include "input.h"
#include "inventory.h"
#include "item.h"
#include "item_category.h"
#include "item_location.h"
#include "item_pocket.h"
#include "iteminfo_query.h"
#include "itype.h"
#include "make_static.h"
#include "map.h"
#include "output.h"
#include "point.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "ui.h"
#include "units.h"

class pocket_favorite_callback : public uilist_callback
{
    private:
        std::list<item_pocket> *pockets = nullptr;
        // whitelist or blacklist, for interactions
        bool whitelist = true;
    public:
        explicit pocket_favorite_callback( std::list<item_pocket> *pockets ) : pockets( pockets ) {}
        void refresh( uilist *menu ) override;
        bool key( const input_context &, const input_event &event, int entnum, uilist *menu ) override;
};

void pocket_favorite_callback::refresh( uilist *menu )
{
    item_pocket *selected_pocket = nullptr;
    int i = 0;
    for( item_pocket &pocket : *pockets ) {
        if( !pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            continue;
        }

        if( i == menu->selected ) {
            selected_pocket = &pocket;
            break;
        }
        ++i;
    }

    if( selected_pocket != nullptr ) {
        std::vector<iteminfo> info;
        int starty = 5;
        const int startx = menu->w_width - menu->pad_right;
        const int width = menu->pad_right - 1;

        fold_and_print( menu->window, point( 2, 2 ), width,
                        c_light_gray, string_format( _( "Press a key to add to %s" ),
                                colorize( whitelist ? _( "whitelist" ) : _( "blacklist" ), c_light_blue ) ) );

        selected_pocket->general_info( info, menu->selected + 1, true );
        starty += fold_and_print( menu->window, point( startx, starty ), width,
                                  c_light_gray, format_item_info( info, {} ) ) + 1;

        info.clear();
        selected_pocket->favorite_info( info );
        starty += fold_and_print( menu->window, point( startx, starty ), width,
                                  c_light_gray, format_item_info( info, {} ) ) + 1;

        info.clear();
        selected_pocket->contents_info( info, menu->selected + 1, true );
        fold_and_print( menu->window, point( startx, starty ), width,
                        c_light_gray, format_item_info( info, {} ) );
    }

    wnoutrefresh( menu->window );
}

static std::string keys_text()
{
    return
        colorize( "p", c_light_green ) + _( " priority, " ) +
        colorize( "i", c_light_green ) + _( " item, " ) +
        colorize( "c", c_light_green ) + _( " category, " ) +
        colorize( "w", c_light_green ) + _( " whitelist, " ) +
        colorize( "b", c_light_green ) + _( " blacklist" );
}

bool pocket_favorite_callback::key( const input_context &, const input_event &event, int,
                                    uilist *menu )
{
    item_pocket *selected_pocket = nullptr;
    int i = 0;
    for( item_pocket &pocket : *pockets ) {
        if( !pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            continue;
        }

        if( i == menu->selected ) {
            selected_pocket = &pocket;
            break;
        }
        ++i;
    }
    if( selected_pocket == nullptr ) {
        return false;
    }

    const char input = event.get_first_input();
    if( input == 'w' ) {
        whitelist = true;
        return true;
    } else if( input == 'b' ) {
        whitelist = false;
        return true;
    } else if( input == 'p' ) {
        string_input_popup popup;
        popup.title( string_format( _( "Enter Priority (current priority %d)" ),
                                    selected_pocket->settings.priority() ) );
        selected_pocket->settings.set_priority( popup.query_int() );
    }

    const bool item_id = input == 'i';
    const bool cat_id = input == 'c';
    uilist selector_menu;

    const std::string remove_prefix = "<color_light_red>-</color> ";
    const std::string add_prefix = "<color_green>+</color> ";

    if( item_id ) {
        const cata::flat_set<itype_id> &listed_itypes = whitelist
                ? selected_pocket->settings.get_item_whitelist()
                : selected_pocket->settings.get_item_blacklist();
        cata::flat_set<itype_id> nearby_itypes;
        selector_menu.title = _( "Select an item from nearby" );
        for( const std::list<item> *it_list : get_player_character().crafting_inventory().const_slice() ) {
            nearby_itypes.insert( it_list->front().typeId() );
        }

        std::vector<std::pair<itype_id, std::string>> listed_names;
        std::vector<std::pair<itype_id, std::string>> nearby_names;
        listed_names.reserve( listed_itypes.size() );
        nearby_names.reserve( nearby_itypes.size() );
        for( const itype_id &id : listed_itypes ) {
            listed_names.emplace_back( id, id->nname( 1 ) );
        }
        for( const itype_id &id : nearby_itypes ) {
            if( !listed_itypes.count( id ) ) {
                nearby_names.emplace_back( id, id->nname( 1 ) );
            }
        }
        const auto &compare_names = []( const std::pair<itype_id, std::string> &lhs,
        const std::pair<itype_id, std::string> &rhs ) {
            return localized_compare( lhs.second, rhs.second );
        };
        std::sort( listed_names.begin(), listed_names.end(), compare_names );
        std::sort( nearby_names.begin(), nearby_names.end(), compare_names );

        for( const std::pair<itype_id, std::string> &it : listed_names ) {
            selector_menu.addentry( remove_prefix + it.second );
        }
        for( const std::pair<itype_id, std::string> &it : nearby_names ) {
            selector_menu.addentry( add_prefix + it.second );
        }
        selector_menu.query();

        const int selected = selector_menu.ret;
        itype_id selected_id = itype_id::NULL_ID();
        if( selected >= 0 ) {
            size_t idx = selected;
            const std::vector<std::pair<itype_id, std::string>> *names = nullptr;
            if( idx < listed_names.size() ) {
                names = &listed_names;
            } else {
                idx -= listed_names.size();
            }
            if( !names && idx < nearby_names.size() ) {
                names = &nearby_names;
            }
            if( names ) {
                selected_id = ( *names )[idx].first;
            }
        }

        if( !selected_id.is_null() ) {
            if( whitelist ) {
                selected_pocket->settings.whitelist_item( selected_id );
            } else {
                selected_pocket->settings.blacklist_item( selected_id );
            }
        }

        return true;
    } else if( cat_id ) {
        // Get all categories and sort by name
        std::vector<item_category> all_cat = item_category::get_all();
        const cata::flat_set<item_category_id> &listed_cat = whitelist
                ? selected_pocket->settings.get_category_whitelist()
                : selected_pocket->settings.get_category_blacklist();
        std::sort( all_cat.begin(), all_cat.end(), [&]( const item_category & lhs,
        const item_category & rhs ) {
            const bool lhs_in_list = listed_cat.count( lhs.get_id() );
            const bool rhs_in_list = listed_cat.count( rhs.get_id() );
            if( lhs_in_list && !rhs_in_list ) {
                return true;
            } else if( !lhs_in_list && rhs_in_list ) {
                return false;
            }
            return localized_compare( lhs.name(), rhs.name() );
        } );

        for( const item_category &cat : all_cat ) {
            const bool in_list = listed_cat.count( cat.get_id() );
            const std::string &prefix = in_list ? remove_prefix : add_prefix;
            selector_menu.addentry( prefix + cat.name() );
        }
        selector_menu.query();

        if( selector_menu.ret >= 0 ) {
            const item_category_id id( all_cat.at( selector_menu.ret ).get_id() );
            if( whitelist ) {
                selected_pocket->settings.whitelist_category( id );
            } else {
                selected_pocket->settings.blacklist_category( id );
            }
        }
        return true;
    }

    return false;
}

item_contents::item_contents( const std::vector<pocket_data> &pockets )
{

    for( const pocket_data &data : pockets ) {
        contents.emplace_back( &data );
    }
}

bool item_contents::empty_real() const
{
    return contents.empty() ||
    std::all_of( contents.begin(), contents.end(), []( const item_pocket & p ) {
        return p.is_default_state();
    } );
}

bool item_contents::empty() const
{
    if( contents.empty() ) {
        return true;
    }
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MOD ) ) {
            // item mods aren't really contents per se
            continue;
        }
        if( !pocket.empty() ) {
            return false;
        }
    }
    return true;
}

bool item_contents::empty_container() const
{
    if( contents.empty() ) {
        return true;
    }
    for( const item_pocket &pocket : contents ) {
        if( !pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            continue;
        }
        if( !pocket.empty() ) {
            return false;
        }
    }
    return true;
}

bool item_contents::full( bool allow_bucket ) const
{
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER )
            && !pocket.full( allow_bucket ) ) {
            return false;
        }
    }
    return true;
}

bool item_contents::bigger_on_the_inside( const units::volume &container_volume ) const
{
    units::volume min_logical_volume = 0_ml;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            if( pocket.rigid() ) {
                min_logical_volume += pocket.max_contains_volume();
            } else {
                min_logical_volume += pocket.magazine_well();
            }
        }
    }
    return container_volume <= min_logical_volume;
}

size_t item_contents::size() const
{
    return contents.size();
}

void item_contents::read_mods( const item_contents &read_input )
{
    for( const item_pocket &pocket : read_input.contents ) {
        if( pocket.saved_type() == item_pocket::pocket_type::MOD ) {
            for( const item *it : pocket.all_items_top() ) {
                insert_item( *it, item_pocket::pocket_type::MOD );
            }
        }
    }
}

void item_contents::combine( const item_contents &read_input, const bool convert )
{
    std::vector<item> uninserted_items;
    size_t pocket_index = 0;

    for( const item_pocket &pocket : read_input.contents ) {
        if( pocket_index < contents.size() ) {
            if( convert ) {
                if( pocket.is_type( item_pocket::pocket_type::MIGRATION ) ||
                    pocket.is_type( item_pocket::pocket_type::CORPSE ) ||
                    pocket.is_type( item_pocket::pocket_type::MAGAZINE ) ||
                    pocket.is_type( item_pocket::pocket_type::MAGAZINE_WELL ) ) {
                    ++pocket_index;
                    for( const item *it : pocket.all_items_top() ) {
                        insert_item( *it, pocket.get_pocket_data()->type );
                    }
                    continue;
                } else if( pocket.is_type( item_pocket::pocket_type::MOD ) ) {
                    // skipping mod type pocket because using combine this way requires mods to come first
                    // and to call update_mod_pockets
                    ++pocket_index;
                    continue;
                }
            } else {
                if( pocket.saved_type() == item_pocket::pocket_type::MOD ) {
                    // this is already handled in item_contents::read_mods
                    ++pocket_index;
                    continue;
                } else if( pocket.saved_type() == item_pocket::pocket_type::MIGRATION ||
                           pocket.saved_type() == item_pocket::pocket_type::CORPSE ) {
                    for( const item *it : pocket.all_items_top() ) {
                        insert_item( *it, pocket.saved_type() );
                    }
                    ++pocket_index;
                    continue;
                }
            }
            auto current_pocket_iter = contents.begin();
            std::advance( current_pocket_iter, pocket_index );

            for( const item *it : pocket.all_items_top() ) {
                const ret_val<item_pocket::contain_code> inserted = current_pocket_iter->insert_item( *it );
                if( !inserted.success() ) {
                    uninserted_items.push_back( *it );
                    debugmsg( "error: item %s cannot fit into pocket while loading: %s",
                              it->typeId().str(), inserted.str() );
                }
            }

            if( pocket.saved_sealed() ) {
                current_pocket_iter->seal();
            }
            current_pocket_iter->settings = pocket.settings;
        } else {
            for( const item *it : pocket.all_items_top() ) {
                uninserted_items.push_back( *it );
            }
        }
        ++pocket_index;
    }

    for( const item &uninserted_item : uninserted_items ) {
        insert_item( uninserted_item, item_pocket::pocket_type::MIGRATION );
    }
}

struct item_contents::item_contents_helper {
    // Static helper function to implement the const and non-const versions of
    // find_pocket_for with less code duplication
    template<typename ItemContents>
    using pocket_type = std::conditional_t <
                        std::is_const<ItemContents>::value,
                        const item_pocket,
                        item_pocket
                        >;

    template<typename ItemContents>
    static ret_val<pocket_type<ItemContents>*> find_pocket_for(
        ItemContents &contents, const item &it, item_pocket::pocket_type pk_type ) {
        using my_pocket_type = pocket_type<ItemContents>;
        static constexpr item_pocket *null_pocket = nullptr;

        std::vector<std::string> failure_messages;
        int num_pockets_of_type = 0;

        for( my_pocket_type &pocket : contents.contents ) {
            if( !pocket.is_type( pk_type ) ) {
                continue;
            }
            if( pk_type != item_pocket::pocket_type::CONTAINER &&
                pk_type != item_pocket::pocket_type::MAGAZINE &&
                pk_type != item_pocket::pocket_type::MAGAZINE_WELL &&
                pocket.is_type( pk_type ) ) {
                return ret_val<my_pocket_type *>::make_success(
                           &pocket, "special pocket type override" );
            }
            ++num_pockets_of_type;
            ret_val<item_pocket::contain_code> ret_contain = pocket.can_contain( it );
            if( ret_contain.success() ) {
                return ret_val<my_pocket_type *>::make_success( &pocket, ret_contain.str() );
            } else {
                failure_messages.push_back( ret_contain.str() );
            }
        }

        if( failure_messages.empty() ) {
            return ret_val<my_pocket_type *>::make_failure( null_pocket, _( "pocket with type (%s) not found" ),
                    io::enum_to_string( pk_type ) );
        }
        std::sort( failure_messages.begin(), failure_messages.end(), localized_compare );
        failure_messages.erase(
            std::unique( failure_messages.begin(), failure_messages.end() ),
            failure_messages.end() );
        return ret_val<my_pocket_type *>::make_failure(
                   null_pocket,
                   n_gettext( "pocket unacceptable because %s", "pockets unacceptable because %s",
                              num_pockets_of_type ),
                   enumerate_as_string( failure_messages, enumeration_conjunction::or_ ) );
    }
};

ret_val<item_pocket *> item_contents::find_pocket_for( const item &it,
        item_pocket::pocket_type pk_type )
{
    return item_contents_helper::find_pocket_for( *this, it, pk_type );
}

ret_val<const item_pocket *> item_contents::find_pocket_for( const item &it,
        item_pocket::pocket_type pk_type ) const
{
    return item_contents_helper::find_pocket_for( *this, it, pk_type );
}

int item_contents::obtain_cost( const item &it ) const
{
    for( const item_pocket &pocket : contents ) {
        const int mv = pocket.obtain_cost( it );
        if( mv != 0 ) {
            return mv;
        }
    }
    return 0;
}

int item_contents::insert_cost( const item &it ) const
{
    ret_val<const item_pocket *> pocket = find_pocket_for( it, item_pocket::pocket_type::CONTAINER );
    if( pocket.success() ) {
        return pocket.value()->moves();
    } else {
        return -1;
    }
}

ret_val<item_pocket *> item_contents::insert_item( const item &it,
        item_pocket::pocket_type pk_type )
{
    if( pk_type == item_pocket::pocket_type::LAST ) {
        // LAST is invalid, so we assume it will be a regular container
        pk_type = item_pocket::pocket_type::CONTAINER;
    }

    ret_val<item_pocket *> pocket = find_pocket_for( it, pk_type );
    if( !pocket.success() ) {
        return pocket;
    }

    ret_val<item_pocket::contain_code> pocket_contain_code = pocket.value()->insert_item( it );
    if( pocket_contain_code.success() ) {
        return pocket;
    }
    return ret_val<item_pocket *>::make_failure( nullptr, pocket_contain_code.str() );
}

void item_contents::force_insert_item( const item &it, item_pocket::pocket_type pk_type )
{
    for( item_pocket &pocket : contents ) {
        if( pocket.is_type( pk_type ) ) {
            pocket.add( it );
            return;
        }
    }
    debugmsg( "ERROR: Could not insert item %s as contents does not have pocket type", it.tname() );
}

std::pair<item_location, item_pocket *> item_contents::best_pocket( const item &it,
        item_location &parent, const item *avoid, const bool allow_sealed, const bool ignore_settings )
{
    if( !can_contain( it ).success() ) {
        return { item_location(), nullptr };
    }
    std::pair<item_location, item_pocket *> ret;
    ret.second = nullptr;
    for( item_pocket &pocket : contents ) {
        if( !pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            // best pocket is for picking stuff up.
            // containers are the only pockets that are available for such
            continue;
        }
        if( !allow_sealed && pocket.sealed() ) {
            // we don't want to unseal a pocket to put something in it automatically
            // that needs to be something a player explicitly does
            continue;
        }
        if( !pocket.can_contain( it ).success() ) {
            continue;
        }
        if( !ignore_settings && !pocket.settings.accepts_item( it ) ) {
            // Item forbidden by whitelist / blacklist
            continue;
        }
        if( ret.second == nullptr || ret.second->better_pocket( pocket, it ) ) {
            // this pocket is the new candidate for "best"
            ret.first = parent;
            ret.second = &pocket;
            // check all pockets within to see if they are better
            for( item *contained : all_items_top( item_pocket::pocket_type::CONTAINER ) ) {
                if( contained == avoid ) {
                    continue;
                }
                std::pair<item_location, item_pocket *> internal_pocket =
                    contained->best_pocket( it, parent, avoid, /*allow_sealed=*/false, /*ignore_settings=*/false );
                if( internal_pocket.second != nullptr &&
                    ret.second->better_pocket( *internal_pocket.second, it, true ) ) {
                    ret = internal_pocket;
                }
            }
        }
    }
    return ret;
}

item_pocket *item_contents::contained_where( const item &contained )
{
    for( item_pocket &pocket : contents ) {
        if( pocket.has_item( contained ) ) {
            return &pocket;
        }
    }
    return nullptr;
}

units::length item_contents::max_containable_length() const
{
    units::length ret = 0_mm;
    for( const item_pocket &pocket : contents ) {
        if( !pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            continue;
        }
        units::length candidate = pocket.max_containable_length();
        if( candidate > ret ) {
            ret = candidate;
        }
    }
    return ret;
}

std::set<flag_id> item_contents::magazine_flag_restrictions() const
{
    std::set<flag_id> ret;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MAGAZINE_WELL ) ) {
            ret = ( pocket.get_pocket_data() )->get_flag_restrictions();
        }
    }
    return ret;
}

units::volume item_contents::max_containable_volume() const
{
    units::volume ret = 0_ml;
    for( const item_pocket &pocket : contents ) {
        if( !pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            continue;
        }
        units::volume candidate = pocket.remaining_volume();
        if( candidate > ret ) {
            ret = candidate;
        }

    }
    return ret;
}

ret_val<bool> item_contents::can_contain_rigid( const item &it ) const
{
    ret_val<bool> ret = ret_val<bool>::make_failure( _( "is not a container" ) );
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MOD ) ||
            pocket.is_type( item_pocket::pocket_type::CORPSE ) ||
            pocket.is_type( item_pocket::pocket_type::MIGRATION ) ) {
            continue;
        }
        if( !pocket.rigid() ) {
            ret = ret_val<bool>::make_failure( _( "is not rigid" ) );
            continue;
        }
        const ret_val<item_pocket::contain_code> pocket_contain_code = pocket.can_contain( it );
        if( pocket_contain_code.success() ) {
            return ret_val<bool>::make_success();
        }
        ret = ret_val<bool>::make_failure( pocket_contain_code.str() );
    }
    return ret;
}

ret_val<bool> item_contents::can_contain( const item &it, const bool ignore_fullness ) const
{
    ret_val<bool> ret = ret_val<bool>::make_failure( _( "is not a container" ) );
    for( const item_pocket &pocket : contents ) {
        // mod, migration, corpse, and software aren't regular pockets.
        if( !pocket.is_standard_type() ) {
            continue;
        }
        const ret_val<item_pocket::contain_code> pocket_contain_code = pocket.can_contain( it,
                ignore_fullness );
        if( pocket_contain_code.success() ) {
            return ret_val<bool>::make_success();
        }
        ret = ret_val<bool>::make_failure( pocket_contain_code.str() );
    }
    return ret;
}

bool item_contents::can_contain_liquid( bool held_or_ground ) const
{
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) &&
            pocket.can_contain_liquid( held_or_ground ) ) {
            return true;
        }
    }
    return false;
}

bool item_contents::can_unload_liquid() const
{
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) && pocket.can_unload_liquid() ) {
            return true;
        }
    }
    return false;
}

size_t item_contents::num_item_stacks() const
{
    size_t num = 0;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MOD ) ||
            pocket.is_type( item_pocket::pocket_type::MAGAZINE_WELL ) ) {
            // mods and magazine wells aren't really a contained item, which this function gets
            continue;
        }
        num += pocket.size();
    }
    return num;
}

void item_contents::on_pickup( Character &guy )
{
    for( item_pocket &pocket : contents ) {
        if( !pocket.is_type( item_pocket::pocket_type::MOD ) ) {
            pocket.on_pickup( guy );
        }
    }
}

bool item_contents::spill_contents( const tripoint &pos )
{
    bool spilled = false;
    for( item_pocket &pocket : contents ) {
        spilled = pocket.spill_contents( pos ) || spilled;
    }
    return spilled;
}

void item_contents::overflow( const tripoint &pos )
{
    for( item_pocket &pocket : contents ) {
        pocket.overflow( pos );
    }
}

void item_contents::heat_up()
{
    for( item_pocket &pocket : contents ) {
        if( !pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            continue;
        }
        pocket.heat_up();
    }
}

int item_contents::ammo_consume( int qty, const tripoint &pos )
{
    int consumed = 0;
    for( item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MAGAZINE_WELL ) ) {
            // we are assuming only one magazine per well
            if( pocket.empty() ) {
                return 0;
            }
            // assuming only one mag
            item &mag = pocket.front();
            const int res = mag.ammo_consume( qty, pos, nullptr );
            if( res && mag.ammo_remaining() == 0 ) {
                if( mag.has_flag( STATIC( flag_id( "MAG_DESTROY" ) ) ) ) {
                    pocket.remove_item( mag );
                } else if( mag.has_flag( STATIC( flag_id( "MAG_EJECT" ) ) ) ) {
                    get_map().add_item( pos, mag );
                    pocket.remove_item( mag );
                }
            }
            qty -= res;
            consumed += res;
        } else if( pocket.is_type( item_pocket::pocket_type::MAGAZINE ) ) {
            const int res = pocket.ammo_consume( qty );
            consumed += res;
            qty -= res;
        }
    }
    return consumed;
}

item *item_contents::magazine_current()
{
    for( item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MAGAZINE_WELL ) ) {
            item *mag = pocket.magazine_current();
            if( mag != nullptr ) {
                return mag;
            }
        }
    }
    return nullptr;
}

int item_contents::ammo_capacity( const ammotype &ammo ) const
{
    int ret = 0;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MAGAZINE ) ) {
            ret += pocket.ammo_capacity( ammo );
        }
    }
    return ret;
}

std::set<ammotype> item_contents::ammo_types() const
{
    std::set<ammotype> ret;
    for( const item_pocket &pocket : contents ) {
        for( const ammotype &ammo : pocket.ammo_types() ) {
            ret.emplace( ammo );
        }
    }
    return ret;
}

item &item_contents::first_ammo()
{
    for( item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MAGAZINE_WELL ) ) {
            return pocket.front().first_ammo();
        }
        if( !pocket.is_type( item_pocket::pocket_type::MAGAZINE ) || pocket.empty() ) {
            continue;
        }
        return pocket.front();
    }
    debugmsg( "Error: Tried to get first ammo in container not containing ammo" );
    return null_item_reference();
}

const item &item_contents::first_ammo() const
{
    if( empty() ) {
        debugmsg( "Error: Contents has no pockets" );
        return null_item_reference();
    }
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MAGAZINE_WELL ) ) {
            return pocket.front().first_ammo();
        }
        if( !pocket.is_type( item_pocket::pocket_type::MAGAZINE ) || pocket.empty() ) {
            continue;
        }
        static const flag_id json_flag_CASING( "CASING" );
        if( pocket.front().has_flag( json_flag_CASING ) ) {
            for( const item *i : pocket.all_items_top() ) {
                if( !i->has_flag( json_flag_CASING ) ) {
                    return *i;
                }
            }
            continue;
        }
        return pocket.front();
    }
    return null_item_reference();
}

bool item_contents::will_spill() const
{
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) && pocket.will_spill() ) {
            return true;
        }
    }
    return false;
}

bool item_contents::will_spill_if_unsealed() const
{
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER )
            && pocket.will_spill_if_unsealed() ) {
            return true;
        }
    }
    return false;
}

bool item_contents::spill_open_pockets( Character &guy, const item *avoid )
{
    for( item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) && pocket.will_spill() ) {
            pocket.handle_liquid_or_spill( guy, avoid );
            if( !pocket.empty() ) {
                return false;
            }
        }
    }
    return true;
}

void item_contents::handle_liquid_or_spill( Character &guy, const item *const avoid )
{
    for( item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            pocket.handle_liquid_or_spill( guy, avoid );
        }
    }
}

void item_contents::casings_handle( const std::function<bool( item & )> &func )
{
    for( item_pocket &pocket : contents ) {
        pocket.casings_handle( func );
    }
}

void item_contents::clear_items()
{
    for( item_pocket &pocket : contents ) {
        pocket.clear_items();
    }
}

void item_contents::clear_magazines()
{
    for( item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MAGAZINE ) ) {
            pocket.clear_items();
        }
    }
}

void item_contents::update_open_pockets()
{
    for( item_pocket &pocket : contents ) {
        if( pocket.empty() ) {
            pocket.unseal();
        }
    }
}

void item_contents::set_item_defaults()
{
    /* For Items with a magazine or battery in its contents */
    for( item_pocket &pocket : contents ) {
        if( !pocket.is_standard_type() ) {
            continue;
        }

        pocket.set_item_defaults();
    }
}

bool item_contents::seal_all_pockets()
{
    bool any_sealed = false;
    for( item_pocket &pocket : contents ) {
        any_sealed = pocket.seal() || any_sealed;
    }
    return any_sealed;
}

bool item_contents::all_pockets_sealed() const
{

    bool all_sealed = false;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            if( !pocket.sealed() ) {
                return false;
            } else {
                all_sealed = true;
            }
        }
    }

    return all_sealed;
}

bool item_contents::any_pockets_sealed() const
{
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            if( pocket.sealed() ) {
                return true;
            }
        }
    }

    return false;
}

bool item_contents::has_pocket_type( const item_pocket::pocket_type pk_type ) const
{
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( pk_type ) ) {
            return true;
        }
    }
    return false;
}

bool item_contents::has_any_with( const std::function<bool( const item &it )> &filter,
                                  item_pocket::pocket_type pk_type ) const
{
    for( const item_pocket &pocket : contents ) {
        if( !pocket.is_type( pk_type ) ) {
            continue;
        }
        if( pocket.has_any_with( filter ) ) {
            return true;
        }
    }
    return false;
}

bool item_contents::stacks_with( const item_contents &rhs ) const
{
    if( contents.size() != rhs.contents.size() ) {
        return false;
    }
    return ( empty() && rhs.empty() ) ||
           std::equal( contents.begin(), contents.end(),
                       rhs.contents.begin(),
    []( const item_pocket & a, const item_pocket & b ) {
        return a.stacks_with( b );
    } );
}

bool item_contents::same_contents( const item_contents &rhs ) const
{
    if( contents.size() != rhs.contents.size() ) {
        return false;
    }
    return std::equal( contents.begin(), contents.end(),
                       rhs.contents.begin(), rhs.contents.end(),
    []( const item_pocket & a, const item_pocket & b ) {
        return a.same_contents( b );
    } );
}

bool item_contents::is_funnel_container( units::volume &bigger_than ) const
{
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            if( pocket.is_funnel_container( bigger_than ) ) {
                return true;
            }
        }
    }
    return false;
}

item &item_contents::only_item()
{
    if( num_item_stacks() != 1 ) {
        debugmsg( "ERROR: item_contents::only_item called with %d items contained", num_item_stacks() );
        return null_item_reference();
    }
    for( item_pocket &pocket : contents ) {
        if( pocket.empty() || !pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            continue;
        }
        // the first item we come to is the only one.
        return pocket.front();
    }
    return null_item_reference();
}

const item &item_contents::only_item() const
{
    if( num_item_stacks() != 1 ) {
        debugmsg( "ERROR: item_contents::only_item called with %d items contained", num_item_stacks() );
        return null_item_reference();
    }
    for( const item_pocket &pocket : contents ) {
        if( pocket.empty() || !( pocket.is_type( item_pocket::pocket_type::CONTAINER ) ||
                                 pocket.is_type( item_pocket::pocket_type::SOFTWARE ) ) ) {
            continue;
        }
        // the first item we come to is the only one.
        return pocket.front();
    }

    return null_item_reference();
}

item *item_contents::get_item_with( const std::function<bool( const item &it )> &filter )
{
    for( item_pocket &pocket : contents ) {
        item *it = pocket.get_item_with( filter );
        if( it != nullptr ) {
            return it;
        }
    }
    return nullptr;
}

void item_contents::remove_items_if( const std::function<bool( item & )> &filter )
{
    for( item_pocket &pocket : contents ) {
        pocket.remove_items_if( filter );
    }
}

std::list<item *> item_contents::all_items_top( const std::function<bool( item_pocket & )> &filter )
{
    std::list<item *> all_items_internal;
    for( item_pocket &pocket : contents ) {
        if( filter( pocket ) ) {
            std::list<item *> contained_items = pocket.all_items_top();
            all_items_internal.insert( all_items_internal.end(), contained_items.begin(),
                                       contained_items.end() );
        }
    }
    return all_items_internal;
}

std::list<item *> item_contents::all_items_top( item_pocket::pocket_type pk_type )
{
    return all_items_top( [pk_type]( item_pocket & pocket ) {
        return pocket.is_type( pk_type );
    } );
}

std::list<const item *> item_contents::all_items_top( const
        std::function<bool( const item_pocket & )> &filter ) const
{
    std::list<const item *> all_items_internal;
    for( const item_pocket &pocket : contents ) {
        if( filter( pocket ) ) {
            std::list<const item *> contained_items = pocket.all_items_top();
            all_items_internal.insert( all_items_internal.end(), contained_items.begin(),
                                       contained_items.end() );
        }
    }
    return all_items_internal;
}

std::list<const item *> item_contents::all_items_top( item_pocket::pocket_type pk_type ) const
{
    return all_items_top( [pk_type]( const item_pocket & pocket ) {
        return pocket.is_type( pk_type );
    } );
}

std::list<item *> item_contents::all_items_top()
{
    return all_items_top( []( const item_pocket & pocket ) {
        return pocket.is_standard_type();
    } );
}

std::list<const item *> item_contents::all_items_top() const
{
    return all_items_top( []( const item_pocket & pocket ) {
        return pocket.is_standard_type();
    } );
}

item &item_contents::legacy_front()
{
    if( empty() ) {
        debugmsg( "naively asked for first content item and will get a nullptr" );
        return null_item_reference();
    }
    return *all_items_top().front();
}

const item &item_contents::legacy_front() const
{
    if( empty() ) {
        debugmsg( "naively asked for first content item and will get a nullptr" );
        return null_item_reference();
    }
    return *all_items_top().front();
}

std::vector<item *> item_contents::gunmods()
{
    std::vector<item *> mods;
    for( item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MOD ) ) {
            std::vector<item *> internal_mods{ pocket.gunmods() };
            mods.insert( mods.end(), internal_mods.begin(), internal_mods.end() );
        }
    }
    return mods;
}

std::vector<const item *> item_contents::gunmods() const
{
    std::vector<const item *> mods;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MOD ) ) {
            std::vector<const item *> internal_mods{ pocket.gunmods() };
            mods.insert( mods.end(), internal_mods.begin(), internal_mods.end() );
        }
    }
    return mods;
}

bool item_contents::allows_speedloader( const itype_id &speedloader_id ) const
{
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MAGAZINE ) ) {
            return pocket.allows_speedloader( speedloader_id );
        }
    }
    return false;
}

std::vector<const item *> item_contents::mods() const
{
    std::vector<const item *> mods;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MOD ) ) {
            for( const item *it : pocket.all_items_top() ) {
                mods.insert( mods.end(), it );
            }
        }
    }
    return mods;
}

std::vector<const item *> item_contents::softwares() const
{
    std::vector<const item *> softwares;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::SOFTWARE ) ) {
            for( const item *it : pocket.all_items_top() ) {
                softwares.insert( softwares.end(), it );
            }
        }
    }
    return softwares;
}

std::vector<item *> item_contents::ebooks()
{
    std::vector<item *> ebooks;
    for( item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::EBOOK ) ) {
            for( item *it : pocket.all_items_top() ) {
                ebooks.emplace_back( it );
            }
        }
    }
    return ebooks;
}

std::vector<const item *> item_contents::ebooks() const
{
    std::vector<const item *> ebooks;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::EBOOK ) ) {
            for( const item *it : pocket.all_items_top() ) {
                ebooks.emplace_back( it );
            }
        }
    }
    return ebooks;
}

void item_contents::update_modified_pockets(
    const cata::optional<const pocket_data *> &mag_or_mag_well,
    std::vector<const pocket_data *> container_pockets )
{
    for( auto pocket_iter = contents.begin(); pocket_iter != contents.end(); ) {
        item_pocket &pocket = *pocket_iter;
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {

            const pocket_data *current = pocket.get_pocket_data();
            bool found = false;
            // this loop is to make sure the pockets on the current item are already here from @container_pockets,
            // so we don't need to clear them (saving the favorite data)
            for( auto container_pocket = container_pockets.begin(); container_pocket != container_pockets.end();
               ) {
                // comparing pointers because each pocket is uniquely defined in json as its own.
                if( *container_pocket == current ) {
                    container_pocket = container_pockets.erase( container_pocket );
                    found = true;
                    // there will not be more than one pocket with the same pocket_data pointer, so exit early
                    break;
                } else {
                    ++container_pocket;
                }
            }

            if( !found ) {
                if( !pocket.empty() ) {
                    // in case the debugmsg wasn't clear, this should never happen
                    debugmsg( "Oops!  deleted some items when updating pockets that were added via toolmods" );
                }
                pocket_iter = contents.erase( pocket_iter );
            } else {
                ++pocket_iter;
            }

        } else if( pocket.is_type( item_pocket::pocket_type::MAGAZINE ) ||
                   pocket.is_type( item_pocket::pocket_type::MAGAZINE_WELL ) ) {
            if( mag_or_mag_well ) {
                if( pocket.get_pocket_data() != *mag_or_mag_well ) {
                    if( !pocket.empty() ) {
                        // in case the debugmsg wasn't clear, this should never happen
                        debugmsg( "Oops!  deleted some items when updating pockets that were added via toolmods" );
                    }
                    contents.emplace_back( *mag_or_mag_well );
                    pocket_iter = contents.erase( pocket_iter );
                } else {
                    ++pocket_iter;
                }
            } else {
                // no mag or mag well, so it needs to be erased
                pocket_iter = contents.erase( pocket_iter );
            }
        } else {
            ++pocket_iter;
        }
    }

    // we've deleted all of the superfluous copies already, so time to add the new pockets
    for( const pocket_data *container_pocket : container_pockets ) {
        contents.emplace_back( container_pocket );
    }

}

std::set<itype_id> item_contents::magazine_compatible() const
{
    std::set<itype_id> ret;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MAGAZINE_WELL ) ) {
            for( const itype_id &id : pocket.item_type_restrictions() ) {
                ret.emplace( id );
            }
        }
    }
    return ret;
}

itype_id item_contents::magazine_default() const
{
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MAGAZINE_WELL ) ) {
            return pocket.magazine_default();
        }
    }
    return itype_id::NULL_ID();
}

units::mass item_contents::total_container_weight_capacity() const
{
    units::mass total_weight = 0_gram;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            total_weight += pocket.weight_capacity();
        }
    }
    return total_weight;
}

ret_val<std::vector<const item_pocket *>> item_contents::get_all_contained_pockets() const
{
    std::vector<const item_pocket *> pockets;
    bool found = false;

    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            found = true;
            pockets.push_back( &pocket );
        }
    }
    if( found ) {
        return ret_val<std::vector<const item_pocket *>>::make_success( pockets );
    } else {
        return ret_val<std::vector<const item_pocket *>>::make_failure( pockets );
    }
}

ret_val<std::vector<item_pocket *>> item_contents::get_all_contained_pockets()
{
    std::vector<item_pocket *> pockets;
    bool found = false;

    for( item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            found = true;
            pockets.push_back( &pocket );
        }
    }
    if( found ) {
        return ret_val<std::vector<item_pocket *>>::make_success( pockets );
    } else {
        return ret_val<std::vector<item_pocket *>>::make_failure( pockets );
    }
}

units::volume item_contents::total_container_capacity() const
{
    units::volume total_vol = 0_ml;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            total_vol += pocket.volume_capacity();
        }
    }
    return total_vol;
}

units::volume item_contents::total_standard_capacity() const
{
    units::volume total_vol = 0_ml;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_standard_type() ) {
            total_vol += pocket.volume_capacity();
        }
    }
    return total_vol;
}

units::volume item_contents::remaining_container_capacity() const
{
    units::volume total_vol = 0_ml;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            total_vol += pocket.remaining_volume();
        }
    }
    return total_vol;
}

units::volume item_contents::total_contained_volume() const
{
    units::volume total_vol = 0_ml;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            total_vol += pocket.contains_volume();
        }
    }
    return total_vol;
}

units::volume item_contents::get_contents_volume_with_tweaks( const std::map<const item *, int>
        &without ) const
{
    units::volume ret = 0_ml;

    for( const item_pocket *pocket : get_all_contained_pockets().value() ) {
        if( !pocket->empty() && !pocket->contains_phase( phase_id::SOLID ) ) {
            const item *it = &pocket->front();
            auto stack = without.find( it );
            if( ( stack == without.end() ) || ( stack->second != it->charges ) ) {
                ret += pocket->volume_capacity();
            }
        } else {
            for( const item *i : pocket->all_items_top() ) {
                if( i->count_by_charges() ) {
                    ret += i->volume() - i->get_selected_stack_volume( without );
                } else if( !without.count( i ) ) {
                    ret += i->volume();
                    ret -= i->get_nested_content_volume_recursive( without );
                }
            }
        }
    }

    return ret;
}

units::volume item_contents::get_nested_content_volume_recursive( const
        std::map<const item *, int> &without ) const
{
    units::volume ret = 0_ml;

    for( const item_pocket *pocket : get_all_contained_pockets().value() ) {
        if( pocket->rigid() && !pocket->empty() && !pocket->contains_phase( phase_id::SOLID ) ) {
            const item *it = &pocket->front();
            auto stack = without.find( it );
            if( ( stack != without.end() ) && ( stack->second == it->charges ) ) {
                ret += pocket->volume_capacity();
            }
        } else {
            for( const item *i : pocket->all_items_top() ) {
                if( i->count_by_charges() ) {
                    ret += i->get_selected_stack_volume( without );
                } else if( without.count( i ) ) {
                    ret += i->volume();
                } else {
                    ret += i->get_nested_content_volume_recursive( without );
                }
            }

            if( pocket->rigid() ) {
                ret += pocket->remaining_volume();
            }
        }
    }

    return ret;
}

void item_contents::remove_internal( const std::function<bool( item & )> &filter,
                                     int &count, std::list<item> &res )
{
    for( item_pocket &pocket : contents ) {
        if( pocket.remove_internal( filter, count, res ) ) {
            return;
        }
    }
}

void item_contents::process( Character *carrier, const tripoint &pos, float insulation,
                             temperature_flag flag, float spoil_multiplier_parent )
{
    for( item_pocket &pocket : contents ) {
        // no reason to check mods, they won't rot
        if( !pocket.is_type( item_pocket::pocket_type::MOD ) ) {
            pocket.process( carrier, pos, insulation, flag, spoil_multiplier_parent );
        }
    }
}

int item_contents::remaining_capacity_for_liquid( const item &liquid ) const
{
    int charges_of_liquid = 0;
    item liquid_copy = liquid;
    liquid_copy.charges = 1;
    for( const item_pocket &pocket : contents ) {
        if( !pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            continue;
        }
        charges_of_liquid += pocket.remaining_capacity_for_item( liquid );
    }
    return charges_of_liquid;
}

float item_contents::relative_encumbrance() const
{
    units::volume nonrigid_max_volume;
    units::volume nonrigid_volume;
    for( const item_pocket &pocket : contents ) {
        if( !pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            continue;
        }
        if( pocket.rigid() ) {
            continue;
        }
        nonrigid_volume += pocket.contains_volume();
        nonrigid_max_volume += pocket.max_contains_volume();
    }
    if( nonrigid_volume > nonrigid_max_volume ) {
        debugmsg( "volume exceeds capacity (%sml > %sml)",
                  to_milliliter( nonrigid_volume ), to_milliliter( nonrigid_max_volume ) );
        return 1;
    }
    if( nonrigid_max_volume == 0_ml ) {
        return 0;
    }
    return nonrigid_volume * 1.0 / nonrigid_max_volume;
}

bool item_contents::all_pockets_rigid() const
{
    for( const item_pocket &pocket : contents ) {
        if( !pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            continue;
        }
        if( !pocket.rigid() ) {
            return false;
        }
    }
    return true;
}

units::volume item_contents::item_size_modifier() const
{
    units::volume total_vol = 0_ml;
    for( const item_pocket &pocket : contents ) {
        total_vol += pocket.item_size_modifier();
    }
    return total_vol;
}

units::mass item_contents::item_weight_modifier() const
{
    units::mass total_mass = 0_gram;
    for( const item_pocket &pocket : contents ) {
        total_mass += pocket.item_weight_modifier();
    }
    return total_mass;
}

units::length item_contents::item_length_modifier() const
{
    units::length total_length = 0_mm;
    for( const item_pocket &pocket : contents ) {
        total_length = std::max( pocket.item_length_modifier(), total_length );
    }
    return total_length;
}


int item_contents::best_quality( const quality_id &id ) const
{
    int ret = 0;
    for( const item_pocket &pocket : contents ) {
        ret = std::max( pocket.best_quality( id ), ret );
    }
    return ret;
}

static void insert_separation_line( std::vector<iteminfo> &info )
{
    if( info.empty() || info.back().sName != "--" ) {
        info.emplace_back( "DESCRIPTION", "--" );
    }
}

void item_contents::info( std::vector<iteminfo> &info, const iteminfo_query *parts ) const
{
    int pocket_number = 1;
    std::vector<iteminfo> contents_info;
    std::vector<item_pocket> found_pockets;
    std::map<int, int> pocket_num; // index, amount
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            bool found = false;
            int idx = 0;
            for( const item_pocket &found_pocket : found_pockets ) {
                if( found_pocket == pocket ) {
                    found = true;
                    pocket_num[idx]++;
                }
                idx++;
            }
            if( !found ) {
                found_pockets.push_back( pocket );
                pocket_num[idx]++;
            }
            pocket.contents_info( contents_info, pocket_number++, contents.size() != 1 );
        }
    }
    if( parts->test( iteminfo_parts::DESCRIPTION_POCKETS ) ) {
        // If multiple pockets and/or multiple kinds of pocket, show total capacity section
        if( found_pockets.size() > 1 || pocket_num[0] > 1 ) {
            insert_separation_line( info );
            info.emplace_back( "CONTAINER", _( "<bold>Total capacity</bold>:" ) );
            info.push_back( vol_to_info( "CONTAINER", _( "Volume: " ), total_container_capacity(), 2, false ) );
            info.push_back( weight_to_info( "CONTAINER", _( "  Weight: " ), total_container_weight_capacity(),
                                            2, false ) );
            info.back().bNewLine = true;
        }

        int idx = 0;
        for( const item_pocket &pocket : found_pockets ) {
            insert_separation_line( info );
            // If there are multiple similar pockets, show their capacity as a set
            if( pocket_num[idx] > 1 ) {
                info.emplace_back( "DESCRIPTION", string_format( _( "<bold>%d Pockets</bold> with capacity:" ),
                                   pocket_num[idx] ) );
            } else {
                // If this is the only pocket the item has, label it "Total capacity"
                // Otherwise, give it a generic "Pocket" heading (is one of several pockets)
                bool only_one_pocket = ( found_pockets.size() == 1 && pocket_num[0] == 1 );
                if( only_one_pocket ) {
                    info.emplace_back( "DESCRIPTION", _( "<bold>Total capacity</bold>:" ) );
                } else {
                    info.emplace_back( "DESCRIPTION", _( "<bold>Pocket</bold> with capacity:" ) );
                }
            }
            idx++;
            pocket.general_info( info, idx, false );
        }
    }
    if( parts->test( iteminfo_parts::DESCRIPTION_CONTENTS ) ) {
        info.insert( info.end(), contents_info.begin(), contents_info.end() );
    }
}

void item_contents::favorite_settings_menu( const std::string &item_name )
{
    pocket_favorite_callback cb( &contents );
    int num_container_pockets = 0;
    std::map<int, std::string> pocket_name;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            pocket_name[num_container_pockets] =
                string_format( "%s/%s",
                               vol_to_info( "", "", pocket.contains_volume() ).sValue,
                               vol_to_info( "", "", pocket.max_contains_volume() ).sValue );
            num_container_pockets++;
        }
    }
    uilist pocket_selector;
    pocket_selector.title = item_name;
    pocket_selector.text = keys_text() + "\n ";
    pocket_selector.callback = &cb;
    pocket_selector.w_x_setup = 0;
    pocket_selector.w_width_setup = []() {
        return TERMX;
    };
    pocket_selector.pad_right_setup = []() {
        return std::max( TERMX / 2, TERMX - 50 );
    };
    pocket_selector.w_y_setup = 0;
    pocket_selector.w_height_setup = []() {
        return TERMY;
    };
    for( int i = 1; i <= num_container_pockets; i++ ) {
        pocket_selector.addentry( string_format( "%d - %s", i, pocket_name[i - 1] ) );
    }

    pocket_selector.query();
}
