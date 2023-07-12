#include "item_contents.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <string>
#include <type_traits>

#include "avatar.h"
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
#include "item_factory.h"
#include "item_location.h"
#include "item_pocket.h"
#include "iteminfo_query.h"
#include "itype.h"
#include "localized_comparator.h"
#include "make_static.h"
#include "map.h"
#include "output.h"
#include "point.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "ui.h"
#include "units.h"

static const flag_id json_flag_CASING( "CASING" );

class pocket_favorite_callback : public uilist_callback
{
    private:
        std::vector<std::tuple<item_pocket *, int, uilist_entry *>> saved_pockets;
        // whitelist or blacklist, for interactions
        bool whitelist = true;
        std::pair<item *, item_pocket *> item_to_move = { nullptr, nullptr };

        bool needs_to_refresh = false;

        std::string uilist_text;

        // items to create pockets for
        std::vector<item *> to_organize;

        void move_item( uilist *menu, item_pocket *selected_pocket );

        void refresh_columns( uilist *menu );

        void add_pockets( item &i, uilist &pocket_selector, const std::string &depth );
    public:
        explicit pocket_favorite_callback( const std::string &uilist_text,
                                           const std::vector<item *> &to_organize,
                                           uilist &pocket_selector );
        void refresh( uilist *menu ) override;
        bool key( const input_context &, const input_event &event, int entnum, uilist *menu ) override;
};

void pocket_favorite_callback::refresh( uilist *menu )
{
    if( needs_to_refresh ) {
        refresh_columns( menu );
        needs_to_refresh = false;
    }

    item_pocket *selected_pocket = nullptr;
    int i = 0;
    int pocket_num = 0;
    for( std::tuple<item_pocket *, int, uilist_entry *> &pocket_val : saved_pockets ) {
        item_pocket *pocket = std::get<0>( pocket_val );
        if( pocket == nullptr || ( pocket->get_pocket_data()  &&
                                   !pocket->is_type( item_pocket::pocket_type::CONTAINER ) ) ) {
            ++i;
            continue;
        }

        if( i == menu->selected ) {
            selected_pocket = pocket;
            pocket_num = std::get<1>( pocket_val ) + 1;
            break;
        }
        ++i;
    }

    if( selected_pocket != nullptr && !selected_pocket->is_forbidden() ) {
        std::vector<iteminfo> info;
        int starty = 5;
        const int startx = menu->w_width - menu->pad_right;
        const int width = menu->pad_right - 1;

        fold_and_print( menu->window, point( 2, 2 ), width,
                        c_light_gray, string_format( _( "Currently modifying %s" ),
                                colorize( whitelist ? _( "whitelist" ) : _( "blacklist" ), c_light_blue ) ) );

        selected_pocket->general_info( info, pocket_num, true );
        starty += fold_and_print( menu->window, point( startx, starty ), width,
                                  c_light_gray, format_item_info( info, {} ) ) + 1;

        info.clear();
        selected_pocket->favorite_info( info );
        starty += fold_and_print( menu->window, point( startx, starty ), width,
                                  c_light_gray, format_item_info( info, {} ) ) + 1;

        info.clear();
        selected_pocket->contents_info( info, pocket_num, true );
        fold_and_print( menu->window, point( startx, starty ), width,
                        c_light_gray, format_item_info( info, {} ) );
    }

    wnoutrefresh( menu->window );
}

pocket_favorite_callback::pocket_favorite_callback( const std::string &uilist_text,
        const std::vector<item *> &to_organize, uilist &pocket_selector )
    : uilist_text( uilist_text ), to_organize( to_organize )
{
    for( item *i : to_organize ) {
        add_pockets( *i, pocket_selector, "" );
    }
}

void pocket_favorite_callback::add_pockets( item &i, uilist &pocket_selector,
        const std::string &depth )
{
    if( i.get_all_contained_pockets().empty() ) {
        // if it doesn't have pockets skip it
        return;
    }

    pocket_selector.addentry( -1, false, '\0', string_format( "%s%s", depth, i.display_name() ) );
    // pad list with empty entries for the items themselves
    saved_pockets.emplace_back( nullptr, 0, nullptr );
    if( ( i.is_collapsed() && to_organize.size() != 1 ) || i.all_pockets_sealed() ) {
        //is collapsed, or all pockets are sealed skip its nested pockets
        // only skip collapsed items if doing multi menu, for a single item this wouldn't make sense
        return;
    }

    uilist_entry *item_entry = &pocket_selector.entries.back();
    int pocket_num = 1;
    for( item_pocket *it_pocket : i.get_all_contained_pockets() ) {
        std::string temp = string_format( "%d -", pocket_num );

        pocket_selector.addentry( 0, true, '\0', string_format( "%s%s %s/%s",
                                  depth,
                                  temp,
                                  vol_to_info( "", "", it_pocket->contains_volume() ).sValue,
                                  vol_to_info( "", "", it_pocket->max_contains_volume() ).sValue ) );
        // pocket number is displayed from 1 stored from 0
        saved_pockets.emplace_back( it_pocket, pocket_num - 1, item_entry );
        pocket_num++;

        // display the items
        for( item *it : it_pocket->all_items_top() ) {
            // check for pockets in that pocket
            add_pockets( *it, pocket_selector, depth + "  " );
        }
    }
}

void pocket_favorite_callback::refresh_columns( uilist *menu )
{
    // rebuild the list of rows can't fully clear or the menu will close
    // clear all but one entry repopulate then delete that initial
    menu->entries.clear();
    saved_pockets.clear();
    for( item *i : to_organize ) {
        add_pockets( *i, *menu, "" );
    }
    // there might be a better way to refresh this
    menu->setup();
}

void pocket_favorite_callback::move_item( uilist *menu, item_pocket *selected_pocket )
{
    uilist selector_menu;

    if( item_to_move.second == nullptr ) {
        selector_menu.title = _( "Select an item from the pocket" );
        std::vector<item *> item_list;
        for( item *it_in : selected_pocket->all_items_top() ) {
            item_list.emplace_back( it_in );
        }

        std::sort( item_list.begin(), item_list.end() );

        for( const item *it : item_list ) {
            selector_menu.addentry( it->display_name() );
        }

        if( selector_menu.entries.empty() ) {
            popup( std::string( _( "No items in the pocket." ) ), PF_GET_KEY );
        } else {
            selector_menu.query();
        }

        if( selector_menu.ret >= 0 ) {
            item_to_move = { item_list[selector_menu.ret], selected_pocket };
        }

        if( item_to_move.first != nullptr ) {
            menu->settext( string_format( "%s: %s", _( "Moving" ), item_to_move.first->display_name() ) );
            refresh_columns( menu );

            // if we have an item already selected for moving update some info
            auto itt = saved_pockets.begin();
            for( uilist_entry &entry : menu->entries ) {
                if( entry.enabled && !std::get<0>( *itt )->can_contain( *item_to_move.first ).success() ) {
                    entry.enabled = false;
                }

                // make sure we dont try to put anything in itself
                if( entry.enabled ) {
                    for( item_pocket *pocket : item_to_move.first->get_all_standard_pockets() ) {
                        if( std::get<0>( *itt ) == pocket ) {
                            entry.enabled = false;
                        }
                    }
                }

                // move through the pockets as you process entries
                ++itt;
            }
        }
    } else {
        // If no pockets are enabled, uilist allows scrolling through them, so we need to recheck
        ret_val<item_pocket::contain_code> contain = selected_pocket->can_contain( *item_to_move.first );
        if( contain.success() ) {
            // storage should mimick character inserting
            get_avatar().as_character()->store( selected_pocket, *item_to_move.first );
        } else {
            const std::string base_string = _( "Cannot put item in pocket because %s" );
            popup( string_format( base_string, contain.str() ) );
        }
        // reset the moved item
        item_to_move = { nullptr, nullptr };

        menu->settext( uilist_text );

        refresh_columns( menu );
    }
}

bool pocket_favorite_callback::key( const input_context &ctxt, const input_event &event, int,
                                    uilist *menu )
{
    item_pocket *selected_pocket = nullptr;
    int i = 0;
    int pocket_num = 0;
    for( std::tuple<item_pocket *, int, uilist_entry *> &pocket_val : saved_pockets ) {
        item_pocket *pocket = std::get<0>( pocket_val );
        if( pocket == nullptr || ( pocket->get_pocket_data()  &&
                                   !pocket->is_type( item_pocket::pocket_type::CONTAINER ) ) ) {
            ++i;
            continue;
        }

        if( i == menu->selected ) {
            selected_pocket = pocket;
            pocket_num = std::get<1>( pocket_val ) + 1;
            break;
        }
        ++i;
    }
    if( selected_pocket == nullptr ) {
        return false;
    }

    const std::string remove_prefix = "<color_light_red>-</color> ";
    const std::string add_prefix = "<color_green>+</color> ";
    const std::string &action = ctxt.input_to_action( event );
    if( action == "FAV_WHITELIST" ) {
        whitelist = true;
        return true;
    } else if( action == "FAV_BLACKLIST" ) {
        whitelist = false;
        return true;
    } else if( action == "FAV_PRIORITY" ) {
        string_input_popup popup;
        popup.title( string_format( _( "Enter Priority (current priority %d)" ),
                                    selected_pocket->settings.priority() ) );
        const int ret = popup.query_int();
        if( popup.confirmed() ) {
            selected_pocket->settings.set_priority( ret );
            selected_pocket->settings.set_was_edited();
        }
        return true;
    } else if( action == "FAV_AUTO_PICKUP" ) {
        selected_pocket->settings.set_disabled( !selected_pocket->settings.is_disabled() );
        selected_pocket->settings.set_was_edited();
        return true;
    } else if( action == "FAV_AUTO_UNLOAD" ) {
        selected_pocket->settings.set_unloadable( !selected_pocket->settings.is_unloadable() );
        selected_pocket->settings.set_was_edited();
        return true;
    } else if( action == "FAV_MOVE_ITEM" ) {
        move_item( menu, selected_pocket );
        selected_pocket->settings.set_was_edited();
        return true;
    } else if( action == "FAV_ITEM" ) {
        const cata::flat_set<itype_id> &listed_itypes = whitelist
                ? selected_pocket->settings.get_item_whitelist()
                : selected_pocket->settings.get_item_blacklist();
        cata::flat_set<itype_id> nearby_itypes;
        uilist selector_menu;
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
            if( !listed_itypes.count( id ) && selected_pocket->is_compatible( item( id ) ).success() ) {
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
        if( selector_menu.entries.empty() ) {
            popup( std::string( _( "No nearby items would fit here." ) ), PF_GET_KEY );
        } else {
            selector_menu.query();
        }

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
            selected_pocket->settings.set_was_edited();
        }

        return true;
    } else if( action == "FAV_CATEGORY" ) {
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

        uilist selector_menu;
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
            selected_pocket->settings.set_was_edited();
        }
        return true;
    } else if( action == "FAV_SAVE_PRESET" ) {
        string_input_popup custom_preset_popup;
        custom_preset_popup
        .title( _( "Enter a preset name:" ) )
        .width( 25 );
        if( selected_pocket->settings.get_preset_name().has_value() ) {
            custom_preset_popup.text( selected_pocket->settings.get_preset_name().value() );
        }
        custom_preset_popup.query_string();
        if( !custom_preset_popup.canceled() ) {
            const std::string &rval = custom_preset_popup.text();
            // Check if already exists
            item_pocket::load_presets();
            if( item_pocket::has_preset( rval ) ) {
                if( query_yn( _( "Preset already exists, overwrite?" ) ) ) {
                    item_pocket::delete_preset( item_pocket::find_preset( rval ) );
                    selected_pocket->settings.set_preset_name( rval );
                    item_pocket::add_preset( selected_pocket->settings );
                }
            } else {
                selected_pocket->settings.set_preset_name( rval );
                item_pocket::add_preset( selected_pocket->settings );
            }
        }
        return true;
    } else if( action == "FAV_APPLY_PRESET" ) {
        item_pocket::load_presets();
        uilist selector_menu;
        selector_menu.title = _( "Select a preset" );
        for( const item_pocket::favorite_settings &preset : item_pocket::pocket_presets ) {
            selector_menu.addentry( preset.get_preset_name().value() );
        }
        selector_menu.query();

        if( selector_menu.ret >= 0 ) {
            selected_pocket->settings = item_pocket::pocket_presets[selector_menu.ret];
        }
        return true;
    } else if( action == "FAV_DEL_PRESET" ) {
        item_pocket::load_presets();
        uilist selector_menu;
        for( const item_pocket::favorite_settings &preset : item_pocket::pocket_presets ) {
            selector_menu.addentry( preset.get_preset_name().value() );
        }
        selector_menu.query();

        if( selector_menu.ret >= 0 ) {
            if( query_yn( _( "Are you sure you wish to delete preset %s?" ),
                          item_pocket::pocket_presets[selector_menu.ret].get_preset_name().value() ) ) {
                item_pocket::delete_preset( item_pocket::pocket_presets.begin() + selector_menu.ret );
            }
        }
        return true;
    } else if( action == "FAV_CLEAR" ) {
        if( query_yn( _( "Are you sure you want to clear settings for pocket %d?" ), pocket_num ) ) {
            selected_pocket->settings.clear();
            selected_pocket->settings.set_was_edited();
        }
        return true;
    } else if( action == "FAV_CONTEXT_MENU" ) {
        uilist cmenu( _( "Action to take on this pocket" ), {} );
        cmenu.addentry( 0, true, inp_mngr.get_first_char_for_action( "FAV_MOVE_ITEM", "INVENTORY" ),
                        ctxt.get_action_name( "FAV_MOVE_ITEM" ) );
        cmenu.addentry( 1, true, inp_mngr.get_first_char_for_action( "FAV_ITEM", "INVENTORY" ),
                        ctxt.get_action_name( "FAV_ITEM" ) );
        cmenu.addentry( 2, true, inp_mngr.get_first_char_for_action( "FAV_CATEGORY", "INVENTORY" ),
                        ctxt.get_action_name( "FAV_CATEGORY" ) );
        cmenu.addentry( 3, true, inp_mngr.get_first_char_for_action( "FAV_WHITELIST", "INVENTORY" ),
                        ctxt.get_action_name( "FAV_WHITELIST" ) );
        cmenu.addentry( 4, true, inp_mngr.get_first_char_for_action( "FAV_BLACKLIST", "INVENTORY" ),
                        ctxt.get_action_name( "FAV_BLACKLIST" ) );
        cmenu.addentry( 5, true, inp_mngr.get_first_char_for_action( "FAV_PRIORITY", "INVENTORY" ),
                        ctxt.get_action_name( "FAV_PRIORITY" ) );
        cmenu.addentry( 6, true, inp_mngr.get_first_char_for_action( "FAV_AUTO_PICKUP", "INVENTORY" ),
                        ctxt.get_action_name( "FAV_AUTO_PICKUP" ) );
        cmenu.addentry( 7, true, inp_mngr.get_first_char_for_action( "FAV_AUTO_UNLOAD", "INVENTORY" ),
                        ctxt.get_action_name( "FAV_AUTO_UNLOAD" ) );
        cmenu.addentry( 8, true, inp_mngr.get_first_char_for_action( "FAV_SAVE_PRESET", "INVENTORY" ),
                        ctxt.get_action_name( "FAV_SAVE_PRESET" ) );
        cmenu.addentry( 9, true, inp_mngr.get_first_char_for_action( "FAV_APPLY_PRESET", "INVENTORY" ),
                        ctxt.get_action_name( "FAV_APPLY_PRESET" ) );
        cmenu.addentry( 10, true, inp_mngr.get_first_char_for_action( "FAV_DEL_PRESET", "INVENTORY" ),
                        ctxt.get_action_name( "FAV_DEL_PRESET" ) );
        cmenu.addentry( 11, true, inp_mngr.get_first_char_for_action( "FAV_CLEAR", "INVENTORY" ),
                        ctxt.get_action_name( "FAV_CLEAR" ) );
        cmenu.query();

        std::string ev;
        switch( cmenu.ret ) {
            case 0:
                ev = "FAV_MOVE_ITEM";
                break;
            case 1:
                ev = "FAV_ITEM";
                break;
            case 2:
                ev = "FAV_CATEGORY";
                break;
            case 3:
                ev = "FAV_WHITELIST";
                break;
            case 4:
                ev = "FAV_BLACKLIST";
                break;
            case 5:
                ev = "FAV_PRIORITY";
                break;
            case 6:
                ev = "FAV_AUTO_PICKUP";
                break;
            case 7:
                ev = "FAV_AUTO_UNLOAD";
                break;
            case 8:
                ev = "FAV_SAVE_PRESET";
                break;
            case 9:
                ev = "FAV_APPLY_PRESET";
                break;
            case 10:
                ev = "FAV_DEL_PRESET";
                break;
            case 11:
                ev = "FAV_CLEAR";
                break;
            default:
                break;

        }
        const std::vector<input_event> evlist = inp_mngr.get_input_for_action( ev, "INVENTORY" );
        if( cmenu.ret >= 0 && cmenu.ret <= 11 && !evlist.empty() ) {
            return key( ctxt, evlist.front(), -1, menu );
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

bool item_contents::empty_with_no_mods() const
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

bool item_contents::is_magazine_full() const
{
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MAGAZINE )
            && pocket.full( false ) ) {
            return true;
        }
    }
    return false;
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

void item_contents::combine( const item_contents &read_input, const bool convert,
                             const bool into_bottom, bool restack_charges )
{
    std::vector<item> uninserted_items;
    size_t pocket_index = 0;

    for( const item &pocket : read_input.additional_pockets ) {
        add_pocket( pocket );
    }

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
                const ret_val<item_pocket::contain_code> inserted = current_pocket_iter->insert_item( *it,
                        into_bottom, restack_charges );
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
    if( pocket.value()->is_forbidden() ) {
        return ret_val<item_pocket *>::make_failure( nullptr, _( "Can't store anything in this." ) );
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
        item_location &this_loc, const item *avoid, const bool allow_sealed, const bool ignore_settings,
        const bool nested, bool ignore_rigidity )
{
    // @TODO: this could be made better by doing a plain preliminary volume check.
    // if the total volume of the parent is not sufficient, a child won't have enough either.
    std::pair<item_location, item_pocket *> ret = { this_loc, nullptr };
    std::vector<item_pocket *> valid_pockets;
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
        valid_pockets.emplace_back( &pocket );
        if( !ignore_settings && !pocket.settings.accepts_item( it ) ) {
            // Item forbidden by whitelist / blacklist
            continue;
        }
        if( !pocket.rigid() && (
                !pocket.settings.get_item_whitelist().empty() ||
                !pocket.settings.get_category_whitelist().empty() ||
                pocket.settings.priority() > 0 ) ) {
            ignore_rigidity = true;
        }
        if( !pocket.can_contain( it ).success() || ( !ignore_rigidity && nested && !pocket.rigid() ) ) {
            // non-rigid nested pocket makes no sense, item should also be able to fit in parent.
            continue;
        }
        if( ret.second == nullptr || ret.second->better_pocket( pocket, it, /*nested=*/nested ) ) {
            ret.second = &pocket;
        }
    }
    const bool parent_pkt_selected = !!ret.second;
    for( item_pocket *pocket : valid_pockets ) {
        std::pair<item_location, item_pocket *const> nested_content_pocket =
            pocket->best_pocket_in_contents( this_loc, it, avoid, allow_sealed, ignore_settings );
        if( !nested_content_pocket.second ||
            ( !nested_content_pocket.second->rigid() && pocket->remaining_volume() < it.volume() ) ) {
            // no nested pocket found, or the nested pocket is soft and the parent is full
            continue;
        }
        if( parent_pkt_selected ) {
            if( ret.second->better_pocket( *nested_content_pocket.second, it, true ) ) {
                // item is whitelisted in nested pocket, prefer that over parent pocket
                ret = nested_content_pocket;
            }
            continue;
        }
        ret = nested_content_pocket;
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

units::length item_contents::max_containable_length( const bool unrestricted_pockets_only ) const
{
    units::length ret = 0_mm;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_forbidden() ) {
            continue;
        }
        bool restriction_condition = !pocket.is_type( item_pocket::pocket_type::CONTAINER ) ||
                                     pocket.is_ablative() || pocket.holster_full();
        if( unrestricted_pockets_only ) {
            restriction_condition = restriction_condition && pocket.is_restricted();
        }
        if( restriction_condition ) {
            continue;
        }
        units::length candidate = pocket.max_containable_length();
        if( candidate > ret ) {
            ret = candidate;
        }
    }
    return ret;
}

units::length item_contents::min_containable_length() const
{
    units::length ret = 0_mm;
    for( const item_pocket &pocket : contents ) {
        if( !pocket.is_type( item_pocket::pocket_type::CONTAINER ) || pocket.is_ablative() ||
            pocket.holster_full() ) {
            continue;
        }
        units::length candidate = pocket.min_containable_length();
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
            ret = pocket.get_pocket_data()->get_flag_restrictions();
        }
    }
    return ret;
}

units::volume item_contents::max_containable_volume( const bool unrestricted_pockets_only ) const
{
    units::volume ret = 0_ml;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_forbidden() ) {
            continue;
        }
        bool restriction_condition = !pocket.is_type( item_pocket::pocket_type::CONTAINER ) ||
                                     pocket.is_ablative() || pocket.holster_full() ||
                                     pocket.volume_capacity() >= pocket_data::max_volume_for_container;
        if( unrestricted_pockets_only ) {
            restriction_condition = restriction_condition && pocket.is_restricted();
        }
        if( restriction_condition ) {
            continue;
        }
        units::volume candidate = pocket.remaining_volume();
        if( candidate > ret ) {
            ret = candidate;
        }

    }
    return ret;
}

ret_val<void> item_contents::is_compatible( const item &it ) const
{
    ret_val<void> ret = ret_val<void>::make_failure( _( "is not a container" ) );
    for( const item_pocket &pocket : contents ) {
        // mod, migration, corpse, and software aren't regular pockets.
        if( !pocket.is_standard_type() ) {
            continue;
        }
        const ret_val<item_pocket::contain_code> pocket_contain_code = pocket.is_compatible( it );
        if( pocket_contain_code.success() ) {
            return ret_val<void>::make_success();
        }
        ret = ret_val<void>::make_failure( pocket_contain_code.str() );
    }
    return ret;
}

ret_val<void> item_contents::can_contain_rigid( const item &it,
        const bool ignore_pkt_settings ) const
{
    ret_val<void> ret = ret_val<void>::make_failure( _( "is not a container" ) );
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MOD ) ||
            pocket.is_type( item_pocket::pocket_type::CORPSE ) ||
            pocket.is_type( item_pocket::pocket_type::MIGRATION ) ) {
            continue;
        }
        if( !pocket.rigid() ) {
            ret = ret_val<void>::make_failure( _( "is not rigid" ) );
            continue;
        }
        if( !ignore_pkt_settings && !pocket.settings.accepts_item( it ) ) {
            ret = ret_val<void>::make_failure( _( "denied by pocket auto insert settings" ) );
            continue;
        }
        const ret_val<item_pocket::contain_code> pocket_contain_code = pocket.can_contain( it );
        if( pocket_contain_code.success() ) {
            return ret_val<void>::make_success();
        }
        ret = ret_val<void>::make_failure( pocket_contain_code.str() );
    }
    return ret;
}

ret_val<void> item_contents::can_contain( const item &it, const bool ignore_pkt_settings,
        units::volume remaining_parent_volume ) const
{
    ret_val<void> ret = ret_val<void>::make_failure( _( "is not a container" ) );
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_forbidden() ) {
            continue;
        }
        // mod, migration, corpse, and software aren't regular pockets.
        if( !pocket.is_standard_type() ) {
            continue;
        }
        if( !ignore_pkt_settings && !pocket.settings.accepts_item( it ) ) {
            ret = ret_val<void>::make_failure( _( "denied by pocket auto insert settings" ) );
            continue;
        }
        if( !pocket.rigid() && it.volume() > remaining_parent_volume ) {
            continue;
        }
        const ret_val<item_pocket::contain_code> pocket_contain_code = pocket.can_contain( it );
        if( pocket_contain_code.success() ) {
            return ret_val<void>::make_success();
        }
        ret = ret_val<void>::make_failure( pocket_contain_code.str() );
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

bool item_contents::contains_no_solids() const
{
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) &&
            pocket.contains_phase( phase_id::SOLID ) ) {
            return false;
        }
    }

    return true;
}

bool item_contents::can_reload_with( const item &ammo, const bool now ) const
{
    for( const item_pocket *pocket : get_all_reloadable_pockets() ) {
        if( pocket->can_reload_with( ammo, now ) ) {
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
            pocket.is_type( item_pocket::pocket_type::MAGAZINE_WELL ) ||
            pocket.is_type( item_pocket::pocket_type::CORPSE ) ) {
            // mods and magazine wells aren't really a contained item, which this function gets
            continue;
        }
        num += pocket.size();
    }
    return num;
}

void item_contents::on_pickup( Character &guy, item *avoid )
{
    for( item_pocket &pocket : contents ) {
        if( !pocket.is_type( item_pocket::pocket_type::MOD ) ) {
            pocket.on_pickup( guy, avoid );
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

void item_contents::overflow( const tripoint &pos, const item_location &loc )
{
    for( item_pocket &pocket : contents ) {
        pocket.overflow( pos, loc );
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

int item_contents::ammo_consume( int qty, const tripoint &pos, float fuel_efficiency )
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
            if( !pocket.empty() && pocket.front().is_fuel() && fuel_efficiency >= 0 ) {
                // if using fuel instead of battery, everything is in kJ
                // charges is going to be the energy needed over the energy in 1 unit of fuel * the efficiency of the generator
                int charges_used = ceil( static_cast<float>( units::from_kilojoule( qty ).value() ) / (
                                             static_cast<float>( pocket.front().fuel_energy().value() ) * fuel_efficiency ) );

                const int res = pocket.ammo_consume( charges_used );
                //calculate the ammount of energy generated
                int energy_generated = res * units::to_kilojoule( pocket.front().fuel_energy() );
                consumed += energy_generated;
                qty -= energy_generated;
                qty = std::max( 0, qty );
            } else {
                const int res = pocket.ammo_consume( qty );
                consumed += res;
                qty -= res;
            }
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
    if( empty() ) {
        debugmsg( "Error: Contents has no pockets" );
        return null_item_reference();
    }
    for( item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MAGAZINE_WELL ) ) {
            return pocket.front().first_ammo();
        }
        if( !pocket.is_type( item_pocket::pocket_type::MAGAZINE ) || pocket.empty() ) {
            continue;
        }
        if( pocket.front().has_flag( json_flag_CASING ) ) {
            for( item *i : pocket.all_items_top() ) {
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

void item_contents::clear_pockets_if( const std::function<bool( item_pocket const & )> &filter )
{
    for( item_pocket &pocket : contents ) {
        if( filter( pocket ) ) {
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

bool item_contents::has_unrestricted_pockets() const
{
    int restricted_pockets_qty = 0;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_restricted() ) {
            restricted_pockets_qty++;
        }
    }
    return restricted_pockets_qty < static_cast <int>( get_all_contained_pockets().size() );
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

bool item_contents::stacks_with( const item_contents &rhs, int depth, int maxdepth ) const
{
    if( contents.size() != rhs.contents.size() ) {
        return false;
    }
    return ( empty() && rhs.empty() ) ||
           std::equal( contents.begin(), contents.end(),
                       rhs.contents.begin(),
    [depth, maxdepth]( const item_pocket & a, const item_pocket & b ) {
        return a.stacks_with( b, depth, maxdepth );
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

bool item_contents::is_restricted_container() const
{
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_restricted() ) {
            return true;
        }
    }
    return false;
}

bool item_contents::is_single_restricted_container() const
{
    std::vector<const item_pocket *> const contained_pockets = get_all_contained_pockets();
    return contained_pockets.size() == 1 && contained_pockets[0]->is_restricted();
}

item &item_contents::first_item()
{
    for( item_pocket &pocket : contents ) {
        if( pocket.empty() || !pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            continue;
        }
        // the first item we come to is the only one.
        return pocket.front();
    }
    return null_item_reference();
}

item &item_contents::only_item()
{
    if( num_item_stacks() != 1 ) {
        debugmsg( "ERROR: item_contents::only_item called with %d items contained", num_item_stacks() );
        return null_item_reference();
    }
    return first_item();
}

const item &item_contents::first_item() const
{
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

const item &item_contents::only_item() const
{
    if( num_item_stacks() != 1 ) {
        debugmsg( "ERROR: item_contents::only_item called with %d items contained", num_item_stacks() );
        return null_item_reference();
    }
    return first_item();
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
            all_items_internal.splice( all_items_internal.end(), std::move( contained_items ) );
        }
    }
    return all_items_internal;
}

std::list<item *> item_contents::all_items_top( item_pocket::pocket_type pk_type,
        bool unloading )
{
    if( unloading ) {
        return all_items_top( [pk_type]( item_pocket & pocket ) {
            return pocket.is_type( pk_type ) && pocket.settings.is_unloadable();
        } );
    }
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
            all_items_internal.splice( all_items_internal.end(), std::move( contained_items ) );
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

std::list<item *> item_contents::all_known_contents()
{
    return all_items_top( []( const item_pocket & pocket ) {
        return pocket.is_standard_type() && pocket.transparent();
    } );
}

std::list<const item *> item_contents::all_known_contents() const
{
    return all_items_top( []( const item_pocket & pocket ) {
        return pocket.is_standard_type() && pocket.transparent();
    } );
}

std::list<item *> item_contents::all_ablative_armor()
{
    return all_items_top( []( const item_pocket & pocket ) {
        return pocket.is_ablative();
    } );
}

std::list<const item *> item_contents::all_ablative_armor() const
{
    return all_items_top( []( const item_pocket & pocket ) {
        return pocket.is_ablative();
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

std::vector<item *> item_contents::cables()
{
    std::vector<item *> cables;
    for( item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CABLE ) ) {
            for( item *it : pocket.all_items_top() ) {
                cables.emplace_back( it );
            }
        }
    }
    return cables;
}

std::vector<const item *> item_contents::cables() const
{
    std::vector<const item *> cables;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CABLE ) ) {
            for( const item *it : pocket.all_items_top() ) {
                cables.emplace_back( it );
            }
        }
    }
    return cables;
}

void item_contents::update_modified_pockets(
    const std::optional<const pocket_data *> &mag_or_mag_well,
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
                if( item_is_blacklisted( id ) ) {
                    continue;
                }
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

units::mass item_contents::total_container_weight_capacity( const bool unrestricted_pockets_only )
const
{
    units::mass total_weight = 0_gram;

    for( const item_pocket &pocket : contents ) {
        if( pocket.is_forbidden() ) {
            continue;
        }
        bool restriction_condition = pocket.is_type( item_pocket::pocket_type::CONTAINER ) &&
                                     !pocket.is_ablative() && pocket.weight_capacity() < pocket_data::max_weight_for_container;
        if( unrestricted_pockets_only ) {
            restriction_condition = restriction_condition && !pocket.is_restricted();
        }
        if( restriction_condition ) {
            total_weight += pocket.weight_capacity();
        }
    }
    return total_weight;
}

std::vector<const item_pocket *> item_contents::get_pockets( const
        std::function<bool( item_pocket const & )> &filter ) const
{
    std::vector<const item_pocket *> pockets;

    for( const item_pocket &pocket : contents ) {
        if( filter( pocket ) ) {
            pockets.push_back( &pocket );
        }
    }
    return pockets;
}

std::vector<item_pocket *> item_contents::get_pockets( const
        std::function<bool( item_pocket const & )> &filter )
{
    std::vector<item_pocket *> pockets;

    for( item_pocket &pocket : contents ) {
        if( filter( pocket ) ) {
            pockets.push_back( &pocket );
        }
    }
    return pockets;
}

std::vector<const item_pocket *> item_contents::get_all_contained_pockets() const
{
    return get_pockets( []( item_pocket const & pocket ) {
        return pocket.is_type( item_pocket::pocket_type::CONTAINER );
    } );
}

std::vector<item_pocket *> item_contents::get_all_contained_pockets()
{
    return get_pockets( []( item_pocket const & pocket ) {
        return pocket.is_type( item_pocket::pocket_type::CONTAINER );
    } );
}

std::vector<const item_pocket *> item_contents::get_all_standard_pockets() const
{
    return get_pockets( []( item_pocket const & pocket ) {
        return pocket.is_standard_type();
    } );
}

std::vector<item_pocket *> item_contents::get_all_standard_pockets()
{
    return get_pockets( []( item_pocket const & pocket ) {
        return pocket.is_standard_type();
    } );
}

std::vector<const item_pocket *> item_contents::get_all_ablative_pockets() const
{
    return get_pockets( []( item_pocket const & pocket ) {
        return pocket.is_ablative();
    } );
}

std::vector<item_pocket *> item_contents::get_all_ablative_pockets()
{
    return get_pockets( []( item_pocket const & pocket ) {
        return pocket.is_ablative();
    } );
}

std::vector<const item *> item_contents::get_added_pockets() const
{
    std::vector<const item *> items_added;

    items_added.reserve( additional_pockets.size() );
    for( const item &it : additional_pockets ) {
        items_added.push_back( &it );
    }

    return items_added;
}

void item_contents::add_pocket( const item &pocket_item )
{
    units::volume total_nonrigid_volume = 0_ml;
    for( const item_pocket *i_pocket : pocket_item.get_all_contained_pockets() ) {

        // need to insert before the end since the final pocket is the migration pocket
        contents.insert( --contents.end(), *i_pocket );
        // these pockets should fallback to using the item name as a description
        // need to update it once it's stored in the contents list
        ( ++contents.rbegin() )->name_as_description = true;
        total_nonrigid_volume += i_pocket->max_contains_volume();
    }
    additional_pockets_volume += total_nonrigid_volume;
    additional_pockets_space_used += pocket_item.get_pocket_size();
    additional_pockets.push_back( pocket_item );
    additional_pockets.back().clear_items();

}

item item_contents::remove_pocket( int index )
{
    // start at the first pocket
    auto rit = contents.rbegin();

    // find the pockets to remove from the item
    for( int i = additional_pockets.size() - 1; i >= index; --i ) {
        // move the iterator past all the pockets we aren't removing
        std::advance( rit, additional_pockets[i].get_all_contained_pockets().size() );
    }

    // at this point reversed past the pockets we want to get rid of so now start going forward
    auto it = std::next( rit ).base();
    units::volume total_nonrigid_volume = 0_ml;
    for( item_pocket *i_pocket : additional_pockets[index].get_all_contained_pockets() ) {
        total_nonrigid_volume += i_pocket->max_contains_volume();

        // move items from the consolidated pockets to the item that will be returned
        for( const item *item_to_move : it->all_items_top() ) {
            i_pocket->add( *item_to_move );
        }

        // finally remove the pocket data
        contents.erase( it++ );
    }
    additional_pockets_volume -= total_nonrigid_volume;
    additional_pockets_space_used -= additional_pockets[index].get_pocket_size();

    // create a copy of the item to return and delete the old items entry
    item it_return( additional_pockets[index] );
    additional_pockets.erase( additional_pockets.begin() + index );

    return it_return;
}

const item_pocket *item_contents::get_added_pocket( int index ) const
{
    if( additional_pockets.empty() || index >= static_cast<int>( additional_pockets.size() ) ) {
        return nullptr;
    }

    // start at the first pocket
    auto rit = contents.rbegin();

    // find the pockets to remove from the item
    for( int i = additional_pockets.size() - 1; i >= index; --i ) {
        // move the iterator past all the pockets we aren't removing
        std::advance( rit, additional_pockets[i].get_all_contained_pockets().size() );
    }

    // at this point reversed past the pockets we want to get rid of so now start going forward
    auto it = std::next( rit ).base();

    return &*it;
}

bool item_contents::has_additional_pockets() const
{
    // if there are additional pockets return true
    return !additional_pockets.empty();
}

int item_contents::get_additional_pocket_encumbrance( float mod ) const
{
    return additional_pockets_volume * mod / 250_ml;
}

int item_contents::get_additional_space_used() const
{
    return additional_pockets_space_used;
}

units::mass item_contents::get_additional_weight() const
{
    units::mass ret = 0_kilogram;
    for( const item &it : additional_pockets ) {
        ret += it.weight( false );
    }
    return ret;
}

units::volume item_contents::get_additional_volume() const
{
    units::volume ret = 0_ml;
    for( const item &it : additional_pockets ) {
        ret += it.volume( false, true );
    }
    return ret;
}

std::vector< const item_pocket *> item_contents::get_all_reloadable_pockets() const
{
    std::vector<const item_pocket *> pockets;

    for( const item_pocket &pocket : contents ) {
        if( ( pocket.is_type( item_pocket::pocket_type::CONTAINER ) && pocket.watertight() ) ||
            pocket.is_type( item_pocket::pocket_type::MAGAZINE ) ||
            pocket.is_type( item_pocket::pocket_type::MAGAZINE_WELL ) ) {
            pockets.push_back( &pocket );
        }
    }
    return pockets;
}

int item_contents::get_used_holsters() const
{
    int holsters = 0;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) && pocket.holster_full() ) {
            holsters++;
        }
    }
    return holsters;
}

int item_contents::get_total_holsters() const
{
    int holsters = 0;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) && pocket.is_holster() ) {
            holsters++;
        }
    }
    return holsters;
}

units::volume item_contents::get_used_holster_volume() const
{
    units::volume holster_volume = 0_ml;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) && pocket.holster_full() ) {
            holster_volume += pocket.volume_capacity();
        }
    }
    return holster_volume;
}

units::volume item_contents::get_total_holster_volume() const
{
    units::volume holster_volume = 0_ml;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) && pocket.is_holster() ) {
            holster_volume += pocket.volume_capacity();
        }
    }
    return holster_volume;
}

units::mass item_contents::get_used_holster_weight() const
{
    units::mass holster_weight = units::mass();
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) && pocket.is_holster() ) {
            holster_weight += pocket.contains_weight();
        }
    }
    return holster_weight;
}

units::mass item_contents::get_total_holster_weight() const
{
    units::mass holster_weight = units::mass();
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) && pocket.is_holster() ) {
            holster_weight += pocket.weight_capacity();
        }
    }
    return holster_weight;
}

units::volume item_contents::total_container_capacity( const bool unrestricted_pockets_only ) const
{
    units::volume total_vol = 0_ml;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_forbidden() ) {
            continue;
        }
        bool restriction_condition = pocket.is_type( item_pocket::pocket_type::CONTAINER );
        if( unrestricted_pockets_only ) {
            restriction_condition = restriction_condition && !pocket.is_restricted();
        }
        if( restriction_condition ) {
            // if the pocket has default volume or is a holster that has an
            // item in it or is a pocket that has normal pickup disabled
            // instead of returning the volume return the volume of things contained
            if( pocket.volume_capacity() >= pocket_data::max_volume_for_container ||
                pocket.settings.is_disabled() || pocket.holster_full() ) {
                total_vol += pocket.contains_volume();
            } else {
                total_vol += pocket.volume_capacity();
            }
        }
    }
    return total_vol;
}

units::volume item_contents::total_standard_capacity( const bool unrestricted_pockets_only ) const
{
    units::volume total_vol = 0_ml;
    for( const item_pocket &pocket : contents ) {
        bool restriction_condition = pocket.is_standard_type();
        if( unrestricted_pockets_only ) {
            restriction_condition = restriction_condition && !pocket.is_restricted();
        }
        if( restriction_condition ) {
            total_vol += pocket.volume_capacity();
        }
    }
    return total_vol;
}

units::volume item_contents::remaining_container_capacity( const bool unrestricted_pockets_only )
const
{
    units::volume total_vol = 0_ml;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_forbidden() ) {
            continue;
        }
        bool restriction_condition = pocket.is_type( item_pocket::pocket_type::CONTAINER );
        if( unrestricted_pockets_only ) {
            restriction_condition = restriction_condition && !pocket.is_restricted();
        }
        if( restriction_condition ) {
            total_vol += pocket.remaining_volume();
        }
    }
    return total_vol;
}

units::volume item_contents::total_contained_volume( const bool unrestricted_pockets_only ) const
{
    units::volume total_vol = 0_ml;
    for( const item_pocket &pocket : contents ) {
        bool restriction_condition = pocket.is_type( item_pocket::pocket_type::CONTAINER );
        if( unrestricted_pockets_only ) {
            restriction_condition = restriction_condition && !pocket.is_restricted();
        }
        if( restriction_condition ) {
            total_vol += pocket.contains_volume();
        }
    }
    return total_vol;
}

units::mass item_contents::remaining_container_capacity_weight( const bool
        unrestricted_pockets_only ) const
{
    units::mass total_weight = 0_gram;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_forbidden() ) {
            continue;
        }
        bool restriction_condition = pocket.is_type( item_pocket::pocket_type::CONTAINER );
        if( unrestricted_pockets_only ) {
            restriction_condition = restriction_condition && !pocket.is_restricted();
        }
        if( restriction_condition ) {
            total_weight += pocket.remaining_weight();
        }
    }
    return total_weight;
}

units::mass item_contents::total_contained_weight( const bool unrestricted_pockets_only ) const
{
    units::mass total_weight = 0_gram;
    for( const item_pocket &pocket : contents ) {
        bool restriction_condition = pocket.is_type( item_pocket::pocket_type::CONTAINER );
        if( unrestricted_pockets_only ) {
            restriction_condition = restriction_condition && !pocket.is_restricted();
        }
        if( restriction_condition ) {
            total_weight += pocket.contains_weight();
        }
    }
    return total_weight;
}

units::volume item_contents::get_contents_volume_with_tweaks( const std::map<const item *, int>
        &without ) const
{
    units::volume ret = 0_ml;

    for( const item_pocket *pocket : get_all_contained_pockets() ) {
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

    for( const item_pocket *pocket : get_all_contained_pockets() ) {
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

void item_contents::process( map &here, Character *carrier, const tripoint &pos, float insulation,
                             temperature_flag flag, float spoil_multiplier_parent )
{
    for( item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            pocket.process( here, carrier, pos, insulation, flag, spoil_multiplier_parent );
        }
    }
}

void item_contents::leak( map &here, Character *carrier, const tripoint &pos, item_pocket *pocke )
{
    for( item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            pocket.leak( here, carrier, pos, pocke );
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
        // need to modify by pockets volume encumbrance modifier since some pockets may have less effect than others
        float modifier = pocket.get_pocket_data()->volume_encumber_modifier;
        nonrigid_volume += pocket.contains_volume() * modifier;
        nonrigid_max_volume += pocket.max_contains_volume() * modifier;
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
        // start by saying what items are attached to this directly
        if( !additional_pockets.empty() ) {
            insert_separation_line( info );
            info.emplace_back( "CONTAINER", _( "<bold>This item incorporates</bold>:" ) );
            for( const item &it : additional_pockets ) {
                info.emplace_back( "CONTAINER", string_format( _( "%s." ),
                                   it.display_name() ) );
            }
            info.back().bNewLine = true;
        }

        // If multiple pockets and/or multiple kinds of pocket, show total capacity section
        units::volume capacity = total_container_capacity();
        units::mass weight = total_container_weight_capacity();
        if( ( found_pockets.size() > 1 || pocket_num[0] > 1 ) && capacity > 0_ml && weight > 0_gram ) {
            insert_separation_line( info );
            info.emplace_back( "CONTAINER", _( "<bold>Total capacity</bold>:" ) );
            info.push_back( vol_to_info( "CONTAINER", _( "Volume: " ), capacity, 2, false ) );
            info.push_back( weight_to_info( "CONTAINER", _( "  Weight: " ), weight,
                                            2, false ) );
            info.back().bNewLine = true;
        }

        int idx = 0;
        for( const item_pocket &pocket : found_pockets ) {
            if( pocket.is_forbidden() ) {
                continue;
            }
            insert_separation_line( info );
            // If there are multiple similar pockets, show their capacity as a set
            if( pocket_num[idx] > 1 ) {
                info.emplace_back( "DESCRIPTION", string_format( _( "<bold>%d pockets</bold> with capacity:" ),
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

void item_contents::favorite_settings_menu( item *i )
{
    std::vector<item *> to_organize;
    to_organize.push_back( i );
    pocket_management_menu( remove_color_tags( i->display_name() ), to_organize );
}

void pocket_management_menu( const std::string &title, const std::vector<item *> &to_organize )
{
    static const std::string input_category = "INVENTORY";
    input_context ctxt( input_category );
    const std::string uilist_text = string_format(
                                        _( "Modify pocket settings and move items between pockets.  [<color_yellow>%s</color>] Context menu" ),
                                        ctxt.get_desc( "FAV_CONTEXT_MENU", 1 ) );
    uilist pocket_selector;
    pocket_favorite_callback cb( uilist_text, to_organize, pocket_selector );
    pocket_selector.title = title;
    pocket_selector.text = uilist_text;
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
    pocket_selector.input_category = input_category;
    pocket_selector.additional_actions = {
        { "FAV_PRIORITY", translation() },
        { "FAV_AUTO_PICKUP", translation() },
        { "FAV_AUTO_UNLOAD", translation() },
        { "FAV_ITEM", translation() },
        { "FAV_CATEGORY", translation() },
        { "FAV_WHITELIST", translation() },
        { "FAV_BLACKLIST", translation() },
        { "FAV_CLEAR", translation() },
        { "FAV_MOVE_ITEM", translation() },
        { "FAV_CONTEXT_MENU", translation() },
        { "FAV_SAVE_PRESET", translation() },
        { "FAV_APPLY_PRESET", translation() },
        { "FAV_DEL_PRESET", translation() }
    };
    // Override CONFIRM with FAV_CONTEXT_MENU
    pocket_selector.allow_confirm = false;
    pocket_selector.allow_additional = true;

    pocket_selector.query();
}
