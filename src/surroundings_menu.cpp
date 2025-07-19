#include "surroundings_menu.h"

#include <algorithm>
#include <cfloat>
#include <functional>
#include <imgui/imgui_internal.h>
#include <iterator>
#include <list>
#include <memory>
#include <set>
#include <tuple>
#include <utility>

#include "avatar.h"
#include "avatar_action.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character.h"
#include "creature.h"
#include "debug.h"
#include "enums.h"
#include "game.h"
#include "game_constants.h"
#include "game_inventory.h"
#include "input_popup.h"
#include "item.h"
#include "item_contents.h"
#include "item_location.h"
#include "item_search.h"
#include "map.h"
#include "mapdata.h"
#include "messages.h"
#include "monster.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "point.h"
#include "safemode_ui.h"
#include "string_formatter.h"
#include "text.h"
#include "translation.h"
#include "type_id.h"
#include "uistate.h"
#include "ui_extended_description.h"
#include "ui_iteminfo.h"
#include "ui_manager.h"

static const trait_id trait_INATTENTIVE( "INATTENTIVE" );

static surroundings_menu_tab_enum &operator++( surroundings_menu_tab_enum &c )
{
    c = static_cast<surroundings_menu_tab_enum>( static_cast<int>( c ) + 1 );
    if( c == surroundings_menu_tab_enum::num_tabs ) {
        c = static_cast<surroundings_menu_tab_enum>( 0 );
    }
    return c;
}

static surroundings_menu_tab_enum &operator--( surroundings_menu_tab_enum &c )
{
    if( c == static_cast<surroundings_menu_tab_enum>( 0 ) ) {
        c = surroundings_menu_tab_enum::num_tabs;
    }
    c = static_cast<surroundings_menu_tab_enum>( static_cast<int>( c ) - 1 );
    return c;
}

template<>
std::string io::enum_to_string<surroundings_menu_tab_enum>( surroundings_menu_tab_enum data )
{
    switch( data ) {
        case surroundings_menu_tab_enum::num_tabs:
        case surroundings_menu_tab_enum::items:
            return "items";
        case surroundings_menu_tab_enum::monsters:
            return "monsters";
        case surroundings_menu_tab_enum::terfurn:
            return "terfurn";
    }
    cata_fatal( "Invalid surroundings menu tab" );
}

static std::string list_items_filter_history_help()
{
    return colorize( _( "UP: history, CTRL-U: clear line, ESC: abort, ENTER: save" ), c_green );
}

template<typename T>
static void move_selection( T &data, const int amount )
{
    if( amount == 0 ) {
        return;
    }

    if( data.filtered_list.empty() ) {
        data.selected_entry = nullptr;
        return;
    }

    auto it = std::find( data.filtered_list.begin(), data.filtered_list.end(), data.selected_entry );

    if( ( amount < 0 && it == data.filtered_list.begin() ) || ( amount > 0 &&
            amount > std::distance( it, data.filtered_list.end() ) ) ) {
        data.selected_entry = *data.filtered_list.rbegin();
    } else if( ( amount < 0 && amount < std::distance( it, data.filtered_list.begin() ) ) ||
               ( amount > 0 && it == data.filtered_list.end() - 1 ) ) {
        data.selected_entry = *data.filtered_list.begin();
    } else {
        std::advance( it, amount );
        data.selected_entry = *it;
    }
}

tab_data::tab_data( const std::string &title ) : title( title )
{
    ctxt = input_context( "LIST_SURROUNDINGS" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "UP", to_translation( "Move cursor up" ) );
    ctxt.register_action( "DOWN", to_translation( "Move cursor down" ) );
    ctxt.register_action( "PAGE_UP", to_translation( "Fast scroll up" ) );
    ctxt.register_action( "PAGE_DOWN", to_translation( "Fast scroll down" ) );
    ctxt.register_action( "look" );
    ctxt.register_action( "zoom_in" );
    ctxt.register_action( "zoom_out" );
    ctxt.register_action( "SCROLL_ITEM_INFO_UP" );
    ctxt.register_action( "SCROLL_ITEM_INFO_DOWN" );
    ctxt.register_action( "RESET_FILTER" );
    ctxt.register_action( "SORT" );
    ctxt.register_action( "TRAVEL_TO" );

    // arbitrary timeout to fix imgui input weirdness
    ctxt.set_timeout( 10 );
}

void tab_data::filter_popup()
{
    string_input_popup_imgui popup( 70, filter, filter_title );
    popup.set_description( filter_description, c_white, true );
    popup.set_label( filter_label );
    popup.set_identifier( filter_identifier );

    filter = popup.query();
    set_filter_active( !filter.empty() );
}

bool tab_data::has_filter() const
{
    return !filter.empty();
}

item_tab_data::item_tab_data( const std::string &title ) : tab_data( title )
{
    ctxt.register_action( "EXAMINE" );
    ctxt.register_action( "COMPARE" );
    ctxt.register_action( "PRIORITY_INCREASE" );
    ctxt.register_action( "PRIORITY_DECREASE" );
    ctxt.register_action( "LEFT", to_translation( "Previous item" ) );
    ctxt.register_action( "RIGHT", to_translation( "Next item" ) );
    ctxt.register_action( "FILTER" );

    filter_title = _( "Filter items" );
    filter_description = item_filter_rule_string( item_filter_type::FILTER ) + "\n\n" +
                         list_items_filter_history_help();
    filter_identifier = "item_filter";

    hotkey_groups = {
        { "EXAMINE", "COMPARE", "TRAVEL_TO" },
        { "FILTER", "RESET_FILTER", "PRIORITY_INCREASE", "PRIORITY_DECREASE" }
    };
}

void item_tab_data::init( const Character &you, map &m )
{
    find_nearby_items( you, m );
    if( uistate.list_item_filter_active ) {
        filter = uistate.list_item_filter;
    }
    if( uistate.list_item_priority_active ) {
        filter_priority_plus = uistate.list_item_priority;
    }
    if( uistate.list_item_downvote_active ) {
        filter_priority_minus = uistate.list_item_downvote;
    }
    apply_filter();
    if( !filtered_list.empty() ) {
        selected_entry = filtered_list.front();
    }
}

void item_tab_data::add_item_recursive( std::vector<std::string> &item_order, const item *it,
                                        const tripoint_rel_ms &relative_pos )
{
    const std::string name = it->tname();

    if( std::find( item_order.begin(), item_order.end(), name ) == item_order.end() ) {
        item_order.push_back( name );

        items[name] = map_entity_stack<item>( it, relative_pos, it->count() );
    } else {
        items[name].add_at_pos( it, relative_pos, it->count() );
    }

    for( const item *content : it->all_known_contents() ) {
        add_item_recursive( item_order, content, relative_pos );
    }
}

void item_tab_data::find_nearby_items( const Character &you, map &m )
{
    std::vector<std::string> item_order;

    if( you.is_blind() ) {
        return;
    }

    tripoint_bub_ms pos = you.pos_bub();

    for( tripoint_bub_ms &points_p_it : closest_points_first( pos, radius ) ) {
        if( points_p_it.y() >= pos.y() - radius && points_p_it.y() <= pos.y() + radius &&
            you.sees( get_map(), points_p_it ) && m.sees_some_items( points_p_it, you ) ) {

            for( item &elem : m.i_at( points_p_it ) ) {
                const tripoint_rel_ms relative_pos = points_p_it - pos;
                add_item_recursive( item_order, &elem, relative_pos );
            }
        }
    }

    for( const std::string &elem : item_order ) {
        // todo: remove this extra sorting when closest_points_first supports circular distances
        std::sort( items[elem].entities.begin(), items[elem].entities.end(),
        []( const auto & lhs, const auto & rhs ) {
            return rl_dist( tripoint_rel_ms(), lhs.pos ) < rl_dist( tripoint_rel_ms(), rhs.pos );
        } );
        item_list.push_back( &items[elem] );
    }

    // todo: remove this extra sorting when closest_points_first supports circular distances
    std::sort( item_list.begin(), item_list.end(), []( const map_entity_stack<item> *lhs,
    const map_entity_stack<item> *rhs ) {
        return lhs->compare( *rhs );
    } );
}

void item_tab_data::reset_filter()
{
    filter = std::string();
    uistate.list_item_filter = std::string();
    uistate.list_item_filter_active = false;
    filtered_list = item_list;
}

void item_tab_data::apply_filter()
{
    filtered_list.clear();

    uistate.list_item_filter = filter;
    uistate.list_item_priority = filter_priority_plus;
    uistate.list_item_downvote = filter_priority_minus;

    auto filter_function = item_filter_from_string( uistate.list_item_filter );

    // first apply regular filter
    std::copy_if( item_list.begin(), item_list.end(),
    std::back_inserter( filtered_list ), [filter_function]( const map_entity_stack<item> *item_stack ) {
        return filter_function( *item_stack->get_selected_entity() );
    } );

    // then apply regular sorting
    std::sort( filtered_list.begin(), filtered_list.end(),
    [&]( const map_entity_stack<item> *lhs, const map_entity_stack<item> *rhs ) {
        return lhs->compare( *rhs, draw_categories );
    } );

    // then apply priorities
    priority_plus_end = list_filter_high_priority( filtered_list, uistate.list_item_priority );
    priority_minus_start = list_filter_low_priority( filtered_list, priority_plus_end + 1,
                           uistate.list_item_downvote );
}

int item_tab_data::get_max_entity_width()
{
    int max_width = 0;
    for( auto &it : item_list ) {
        max_width = std::max( max_width,
                              utf8_width( remove_color_tags( it->get_selected_entity()->display_name() ) ) );
    }
    return max_width;
}

std::optional<tripoint_rel_ms> item_tab_data::get_selected_pos()
{
    if( !selected_entry ) {
        return std::nullopt;
    }

    return selected_entry->get_selected_pos();
}

void item_tab_data::select_prev_internal()
{
    if( !selected_entry ) {
        return;
    }

    selected_entry->select_prev();
}

void item_tab_data::select_next_internal()
{
    if( !selected_entry ) {
        return;
    }

    selected_entry->select_next();
}

void item_tab_data::move_selection( const int amount )
{
    ::move_selection( *this, amount );
}

void item_tab_data::priority_popup( bool high )
{
    std::string &prio_filter = high ? filter_priority_plus : filter_priority_minus;
    std::string prio_title = high ? _( "High priority" ) : _( "Low priority" );
    item_filter_type prio_type = high ? item_filter_type::HIGH_PRIORITY :
                                 item_filter_type::LOW_PRIORITY;
    std::string prio_description = item_filter_rule_string( prio_type ) + "\n\n" +
                                   list_items_filter_history_help();
    std::string prio_id = high ? "list_item_priority" : "list_item_downvote";

    string_input_popup_imgui popup( 70, prio_filter, prio_title );
    popup.set_description( prio_description, c_white, true );
    popup.set_label( _( "Filter: " ) );
    popup.set_identifier( prio_id );

    prio_filter = popup.query();

    if( high ) {
        uistate.list_item_priority_active = !prio_filter.empty();
    } else {
        uistate.list_item_downvote_active = !prio_filter.empty();
    }
}

void item_tab_data::set_filter_active( bool active ) const
{
    uistate.list_item_filter_active = active;
}

monster_tab_data::monster_tab_data( const std::string &title ) :
    tab_data( title )
{
    ctxt.register_action( "fire" );
    ctxt.register_action( "SAFEMODE_BLACKLIST_ADD" );
    ctxt.register_action( "SAFEMODE_BLACKLIST_REMOVE" );
    ctxt.register_action( "FILTER_MONSTERS" );

    filter_title = _( "Filter monsters" );
    filter_description = _( "Type part of a monster's name to filter it." );
    filter_identifier = "monster_filter";

    hotkey_groups = {
        { "fire", "TRAVEL_TO", "SAFEMODE_BLACKLIST_ADD", "SAFEMODE_BLACKLIST_REMOVE" },
        { "FILTER_MONSTERS", "RESET_FILTER" }
    };
}

void monster_tab_data::init( const Character &you )
{
    find_nearby_monsters( you );
    if( uistate.list_monster_filter_active ) {
        filter = uistate.monster_filter;
    }
    apply_filter();
    if( !filtered_list.empty() ) {
        selected_entry = filtered_list.front();
    }
}

void monster_tab_data::find_nearby_monsters( const Character &you )
{
    std::vector<Creature *> mons = you.get_visible_creatures( radius );

    for( Creature *mon : mons ) {
        tripoint_rel_ms pos = mon->pos_bub() - you.pos_bub();
        map_entity_stack<Creature> mstack( mon, pos );
        monsters.push_back( mstack );
    }
    for( map_entity_stack<Creature> &mon : monsters ) {
        // no stack internal sorting because monsters currently don't stack
        monster_list.push_back( &mon );
    }

    std::sort( monster_list.begin(), monster_list.end(), []( const map_entity_stack<Creature> *lhs,
    const map_entity_stack<Creature> *rhs ) {
        return lhs->compare( *rhs );
    } );
}

void monster_tab_data::reset_filter()
{
    filter = std::string();
    uistate.monster_filter = std::string();
    uistate.list_monster_filter_active = false;
    filtered_list = monster_list;
}

void monster_tab_data::apply_filter()
{
    filtered_list.clear();
    uistate.monster_filter = filter;

    // todo: filter_from_string<Creature>
    // for now just matching creature name
    // auto z = item_filter_from_string( filter );

    // first apply regular filter
    std::copy_if( monster_list.begin(), monster_list.end(),
    std::back_inserter( filtered_list ), []( const map_entity_stack<Creature> *a ) {
        return lcmatch( remove_color_tags( a->get_selected_entity()->disp_name() ),
                        uistate.monster_filter );
    } );

    // then apply regular sorting
    std::sort( filtered_list.begin(), filtered_list.end(),
    [&]( const map_entity_stack<Creature> *lhs, const map_entity_stack<Creature> *rhs ) {
        return lhs->compare( *rhs, draw_categories );
    } );
}

int monster_tab_data::get_max_entity_width()
{
    int max_width = 0;
    for( auto &it : monster_list ) {
        max_width = std::max( max_width,
                              utf8_width( remove_color_tags( it->get_selected_entity()->disp_name() ) ) );
    }
    return max_width;
}

std::optional<tripoint_rel_ms> monster_tab_data::get_selected_pos()
{
    if( !selected_entry ) {
        return std::nullopt;
    }

    return selected_entry->get_selected_pos();
}

void monster_tab_data::select_prev_internal()
{
    if( !selected_entry ) {
        return;
    }

    selected_entry->select_prev();
}

void monster_tab_data::select_next_internal()
{
    if( !selected_entry ) {
        return;
    }

    selected_entry->select_next();
}

void monster_tab_data::move_selection( const int amount )
{
    ::move_selection( *this, amount );
}

void monster_tab_data::set_filter_active( bool active ) const
{
    uistate.list_monster_filter_active = active;
}

bool monster_tab_data::can_fire_at_selected( avatar &you, int max_gun_range ) const
{
    if( !selected_entry ) {
        return false;
    }
    const Creature *critter = selected_entry->get_selected_entity();

    return critter && rl_dist( you.pos_bub(), critter->pos_bub() ) <= max_gun_range;
}

bool monster_tab_data::fire_at_selected( avatar &you, int max_gun_range,
        const tripoint_rel_ms &stored_view_offset ) const
{
    if( !can_fire_at_selected( you, max_gun_range ) ) {
        return false;
    }

    you.last_target = g->shared_from( *selected_entry->get_selected_entity() );
    you.recoil = MAX_RECOIL;
    you.view_offset = stored_view_offset;
    avatar_action::fire_wielded_weapon( you );

    return true;
}

terfurn_tab_data::terfurn_tab_data( const std::string &title ) :
    tab_data( title )
{
    ctxt.register_action( "LEFT", to_translation( "Previous terrain/furniture" ) );
    ctxt.register_action( "RIGHT", to_translation( "Next terrain/furniture" ) );
    ctxt.register_action( "FILTER" );

    filter_title = _( "Filter terrain and furniture" );
    filter_description = _( "Type part of a terrain's or furniture's name to filter it." );
    filter_identifier = "terfurn_filter";

    // only kept as a single group because of the low number
    hotkey_groups = {
        { "FILTER", "RESET_FILTER", "TRAVEL_TO" }
    };
}

void terfurn_tab_data::init( const Character &you, map &m )
{
    find_nearby_terfurn( you, m );
    if( uistate.list_terfurn_filter_active ) {
        filter = uistate.terfurn_filter;
    }
    apply_filter();
    if( !filtered_list.empty() ) {
        selected_entry = filtered_list.front();
    }
}

void terfurn_tab_data::add_terfurn( std::vector<std::string> &item_order,
                                    const map_data_common_t *terfurn, const tripoint_rel_ms &relative_pos )
{
    const std::string name = terfurn->name();

    if( std::find( item_order.begin(), item_order.end(), name ) == item_order.end() ) {
        item_order.push_back( name );

        terfurns[name] = map_entity_stack<map_data_common_t>( terfurn, relative_pos );
    } else {
        terfurns[name].add_at_pos( terfurn, relative_pos );
    }
}

void terfurn_tab_data::find_nearby_terfurn( const Character &you, map &m )
{
    std::vector<std::string> item_order;

    if( you.is_blind() ) {
        return;
    }

    tripoint_bub_ms pos = you.pos_bub();

    for( tripoint_bub_ms &points_p_it : closest_points_first( pos, radius ) ) {
        if( points_p_it.y() >= pos.y() - radius && points_p_it.y() <= pos.y() + radius &&
            you.sees( get_map(), points_p_it ) ) {
            const tripoint_rel_ms relative_pos = points_p_it - pos;

            ter_id ter = m.ter( points_p_it );
            add_terfurn( item_order, &*ter, relative_pos );

            if( m.has_furn( points_p_it ) ) {
                furn_id furn = m.furn( points_p_it );
                add_terfurn( item_order, &*furn, relative_pos );
            }
        }
    }

    for( const std::string &elem : item_order ) {
        // todo: remove this extra sorting when closest_points_first supports circular distances
        std::sort( terfurns[elem].entities.begin(), terfurns[elem].entities.end(),
        []( const auto & lhs, const auto & rhs ) {
            return rl_dist( tripoint_rel_ms(), lhs.pos ) < rl_dist( tripoint_rel_ms(), rhs.pos );
        } );
        terfurn_list.push_back( &terfurns[elem] );
    }

    // todo: remove this extra sorting when closest_points_first supports circular distances
    std::sort( terfurn_list.begin(),
               terfurn_list.end(), []( const map_entity_stack<map_data_common_t> *lhs,
    const map_entity_stack<map_data_common_t> *rhs ) {
        return lhs->compare( *rhs );
    } );
}

void terfurn_tab_data::reset_filter()
{
    filter = std::string();
    uistate.terfurn_filter = std::string();
    uistate.list_terfurn_filter_active = false;
    filtered_list = terfurn_list;
}

void terfurn_tab_data::apply_filter()
{
    filtered_list.clear();
    uistate.terfurn_filter = filter;

    // todo: filter_from_string<map_data_common_t>
    // for now just matching creature name
    // auto z = item_filter_from_string( filter );

    // first apply regular filter
    std::copy_if( terfurn_list.begin(), terfurn_list.end(),
    std::back_inserter( filtered_list ), []( const map_entity_stack<map_data_common_t> *a ) {
        return lcmatch( remove_color_tags( a->get_selected_entity()->name() ), uistate.terfurn_filter );
    } );

    // then apply regular sorting
    std::sort( filtered_list.begin(), filtered_list.end(),
               [&]( const map_entity_stack<map_data_common_t> *lhs,
    const map_entity_stack<map_data_common_t> *rhs ) {
        return lhs->compare( *rhs, draw_categories );
    } );
}

int terfurn_tab_data::get_max_entity_width()
{
    int max_width = 0;
    for( auto &it : terfurn_list ) {
        max_width = std::max( max_width,
                              utf8_width( remove_color_tags( it->get_selected_entity()->name() ) ) );
    }
    return max_width;
}

std::optional<tripoint_rel_ms> terfurn_tab_data::get_selected_pos()
{
    if( !selected_entry ) {
        return std::nullopt;
    }

    return selected_entry->get_selected_pos();
}

void terfurn_tab_data::select_prev_internal()
{
    if( !selected_entry ) {
        return;
    }

    selected_entry->select_prev();
}

void terfurn_tab_data::select_next_internal()
{
    if( !selected_entry ) {
        return;
    }

    selected_entry->select_next();
}

void terfurn_tab_data::move_selection( const int amount )
{
    ::move_selection( *this, amount );
}

void terfurn_tab_data::set_filter_active( bool active ) const
{
    uistate.list_terfurn_filter_active = active;
}

surroundings_menu::surroundings_menu( avatar &you, map &m, std::optional<tripoint_bub_ms> &path_end,
                                      int min_width ) :
    cataimgui::window( "SURROUNDINGS", ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                       ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoNavInputs ),
    you( you ),
    path_end( path_end ),
    stored_view_offset( you.view_offset ),
    info_height( std::min( 25, TERMY / 2 ) )
{
    item_data.init( you, m );
    monster_data.init( you );
    terfurn_data.init( you, m );

    // todo: add distance width, calculate correct creature text width
    int wanted_width = std::max( { item_data.get_max_entity_width(),
                                   monster_data.get_max_entity_width(),
                                   terfurn_data.get_max_entity_width() } );
    width = clamp( wanted_width, min_width, TERMX / 3 );
    highlight_new = get_option<bool>( "HIGHLIGHT_UNREAD_ITEMS" );
    switch_tab = uistate.vmenu_tab;
    if( you.get_wielded_item() ) {
        max_gun_range = you.get_wielded_item()->gun_range( &you );
    }
}

cataimgui::bounds surroundings_menu::get_bounds()
{
    info_height = std::min( 25, TERMY / 2 );
    return { static_cast<float>( str_width_to_pixels( TERMX - width ) ), 0.f, static_cast<float>( str_width_to_pixels( width ) ), static_cast<float>( str_height_to_pixels( TERMY ) ) };
}

void surroundings_menu::draw_controls()
{
    if( hide_ui ) {
        ImGuiWindow *w = ImGui::GetCurrentWindowRead();
        ImGui::SetWindowHiddenAndSkipItemsForCurrentFrame( w );
    }
    if( ImGui::BeginTabBar( "surroundings tabs" ) ) {
        draw_item_tab();
        draw_monster_tab();
        draw_terfurn_tab();
        ImGui::EndTabBar();
    }
}

// todo: get ImGui::SeparatorText to draw on the entire row of a table
void surroundings_menu::draw_category_separator( const std::string &category,
        std::string &last_category, int target_col )
{
    if( !category.empty() && ( category != last_category ) ) {
        last_category = category;
        do {
            ImGui::TableNextColumn();
            target_col--;
        } while( target_col >= 0 );
        cataimgui::draw_colored_text( category, c_magenta );

    }
    ImGui::TableNextRow();
}

// return content_newness based on if item is known and nested items are known
static content_newness check_items_newness( const item *itm )
{
    if( !uistate.read_items.count( itm->typeId() ) ) {
        return content_newness::NEW;
    }
    return itm->get_contents().get_content_newness( uistate.read_items );
}

// add item and all visible nested items inside to known items
static void mark_items_read_rec( const item *itm )
{
    uistate.read_items.insert( itm->typeId() );
    for( const item *child : itm->all_known_contents() ) {
        mark_items_read_rec( child );
    }
}

void surroundings_menu::draw_item_tab()
{
    ImGuiTabItemFlags_ flags = ImGuiTabItemFlags_None;
    if( switch_tab == surroundings_menu_tab_enum::items ) {
        flags = ImGuiTabItemFlags_SetSelected;
        switch_tab = surroundings_menu_tab_enum::num_tabs;
    }

    std::string title = string_format( "%s (%d)", item_data.title, item_data.filtered_list.size() );

    if( ImGui::BeginTabItem( title.c_str(), nullptr, flags ) ) {
        selected_tab = surroundings_menu_tab_enum::items;

        // list of nearby items
        float list_height = str_height_to_pixels( TERMY - info_height ) -
                            get_hotkey_buttons_height( &item_data ) - magic_number_other_elements_height;
        if( ImGui::BeginTable( "items", 2, ImGuiTableFlags_ScrollY, ImVec2( 0.0f, list_height ) ) ) {
            float name_col_width = str_width_to_pixels( width - dist_width - 3 );
            float dir_col_width = str_width_to_pixels( dist_width );
            ImGui::TableSetupColumn( "it_name",
                                     ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, name_col_width );
            ImGui::TableSetupColumn( "it_distance",
                                     ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, dir_col_width );
            if( item_data.item_list.empty() ) {
                item_data.selected_entry = nullptr;
                ImGui::TableNextColumn();
                cataimgui::draw_colored_text( _( "You don't see any items around you!" ), c_white );
            } else {
                if( item_data.filtered_list.empty() ) {
                    item_data.selected_entry = nullptr;
                } else if( !item_data.selected_entry ||
                           std::find( item_data.filtered_list.begin(), item_data.filtered_list.end(),
                                      item_data.selected_entry ) == item_data.filtered_list.end() ) {
                    item_data.selected_entry = item_data.filtered_list.front();
                }
                std::string last_category;
                int entry_no = 0;
                for( map_entity_stack<item> *it : item_data.filtered_list ) {
                    bool prio_plus = entry_no <= item_data.priority_plus_end;
                    bool prio_minus = entry_no >= item_data.priority_minus_start;
                    if( item_data.draw_categories ) {
                        std::string cat = prio_plus ? _( "HIGH PRIORITY" ) : prio_minus ? _( "LOW PRIORITY" ) :
                                          it->get_category();
                        draw_category_separator( cat, last_category, 0 );
                    }
                    bool is_selected = it == item_data.selected_entry;
                    const item *itm = it->get_selected_entity();

                    ImGui::TableNextColumn();
                    ImGui::PushID( entry_no++ );
                    ImGui::Selectable( "", &is_selected,
                                       ImGuiSelectableFlags_AllowItemOverlap | ImGuiSelectableFlags_SpanAllColumns, ImVec2( 0, 0 ) );
                    if( is_selected ) {
                        if( auto_scroll || item_data.selected_entry != it ) {
                            // auto_scroll marks keyboard selected items
                            // the other condition marks mouse selected items
                            mark_items_read_rec( itm );
                        }
                        if( auto_scroll ) {
                            ImGui::SetScrollHereY();
                            auto_scroll = false;
                        }
                        item_data.selected_entry = it;
                    }
                    ImGui::SameLine( 0, 0 );
                    ImGui::PopID();
                    nc_color color = prio_plus ? c_yellow : prio_minus ? c_red : itm->color_in_inventory();
                    std::string newness_str;
                    nc_color newness_color;
                    if( highlight_new ) {
                        switch( check_items_newness( itm ) ) {
                            case content_newness::NEW:
                                newness_str = item_new_str;
                                newness_color = item_new_col;
                                break;
                            case content_newness::MIGHT_BE_HIDDEN:
                                newness_str = item_maybe_new_str;
                                newness_color = item_maybe_new_col;
                                break;
                            default:
                                break;
                        }
                    }

                    // FIXME: these width calculations somehow work for variable-width but not for fixed-width fonts
                    ImFont *font = ImGui::GetFont();
                    float newness_str_width = font->CalcTextSizeA( font->FontSize, FLT_MAX, 0.f,
                                              newness_str.c_str() ).x + ImGui::GetStyle().ItemSpacing.x;
                    float wrap_width_subtrahend = ImGui::GetStyle().ItemSpacing.x;
                    if( highlight_new && !newness_str.empty() ) {
                        wrap_width_subtrahend += newness_str_width;
                    }

                    cataimgui::TextColoredTrimmed( it->get_selected_name(), color,
                                                   name_col_width - wrap_width_subtrahend );

                    if( highlight_new && !newness_str.empty() ) {
                        ImGui::SameLine( name_col_width - newness_str_width, -1.0f );
                        cataimgui::draw_colored_text( newness_str, newness_color );
                    }

                    ImGui::TableNextColumn();
                    cataimgui::draw_colored_text( it->get_selected_distance_string(), c_light_gray );
                }
            }
            ImGui::EndTable();
        }

        draw_hotkey_buttons( &item_data );

        if( ImGui::BeginChild( "info", ImVec2( 0.0f, str_height_to_pixels( info_height ) ) ) ) {
            if( item_data.selected_entry ) {
                const item *selected_it = item_data.selected_entry->get_selected_entity();
                // using table so we get a (pre-styled) header
                if( ImGui::BeginTable( "iteminfo", 1, ImGuiTableFlags_None, ImVec2( 0.0f, 0.0f ) ) ) {
                    ImGui::TableSetupColumn( remove_color_tags( selected_it->display_name() ).c_str() );
                    ImGui::TableHeadersRow();
                    ImGui::TableNextColumn();
                    // embedded info for selected item
                    std::vector<iteminfo> selected_info;
                    std::vector<iteminfo> dummy_info;
                    selected_it->info( true, selected_info );
                    item_info_data dummy( "", "", selected_info, dummy_info );
                    dummy.without_getch = true;
                    dummy.without_border = true;
                    cataimgui::set_scroll( info_scroll );
                    display_item_info( dummy.get_item_display(), dummy.get_item_compare() );
                    ImGui::EndTable();
                }
            }
        }
        ImGui::EndChild();
        ImGui::EndTabItem();
    }
}

void surroundings_menu::draw_monster_tab()
{
    ImGuiTabItemFlags_ flags = ImGuiTabItemFlags_None;
    if( switch_tab == surroundings_menu_tab_enum::monsters ) {
        flags = ImGuiTabItemFlags_SetSelected;
        switch_tab = surroundings_menu_tab_enum::num_tabs;
    }

    std::string title = string_format( "%s (%d)", monster_data.title,
                                       monster_data.filtered_list.size() );

    if( ImGui::BeginTabItem( title.c_str(), nullptr, flags ) ) {
        selected_tab = surroundings_menu_tab_enum::monsters;

        // list of nearby monsters
        float list_height = str_height_to_pixels( TERMY - info_height ) -
                            get_hotkey_buttons_height( &monster_data ) - magic_number_other_elements_height;
        if( ImGui::BeginTable( "monsters", 5, ImGuiTableFlags_ScrollY, ImVec2( 0.0f, list_height ) ) ) {
            ImGui::TableSetupColumn( "mon_sees",
                                     ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize,
                                     str_width_to_pixels( 1 ) );
            ImGui::TableSetupColumn( "mon_name",
                                     ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize,
                                     str_width_to_pixels( width - dist_width - 26 ) );
            ImGui::TableSetupColumn( "mon_health",
                                     ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize,
                                     str_width_to_pixels( 5 ) );
            ImGui::TableSetupColumn( "mon_attitude",
                                     ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize,
                                     str_width_to_pixels( 18 ) );
            ImGui::TableSetupColumn( "mon_distance",
                                     ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize,
                                     str_width_to_pixels( dist_width ) );
            if( monster_data.filtered_list.empty() ) {
                monster_data.selected_entry = nullptr;
            } else if( !monster_data.selected_entry ||
                       std::find( monster_data.filtered_list.begin(), monster_data.filtered_list.end(),
                                  monster_data.selected_entry ) == monster_data.filtered_list.end() ) {
                monster_data.selected_entry = monster_data.filtered_list.front();
            }
            std::string last_category;
            int entry_no = 0;
            for( map_entity_stack<Creature> *it : monster_data.filtered_list ) {
                if( monster_data.draw_categories ) {
                    draw_category_separator( it->get_category(), last_category, 1 );
                }
                bool is_selected = it == monster_data.selected_entry;
                ImGui::TableNextColumn();
                ImGui::PushID( entry_no++ );
                ImGui::Selectable( "", &is_selected,
                                   ImGuiSelectableFlags_AllowItemOverlap | ImGuiSelectableFlags_SpanAllColumns, ImVec2( 0, 0 ) );
                if( is_selected ) {
                    if( auto_scroll ) {
                        ImGui::SetScrollHereY();
                        auto_scroll = false;
                    }
                    monster_data.selected_entry = it;
                }
                ImGui::SameLine( 0, 0 );
                ImGui::PopID();
                const Creature *mon = it->get_selected_entity();
                const Character &you = get_player_character();
                const bool inattentive = you.has_trait( trait_INATTENTIVE );
                bool sees = mon->sees( get_map(), you ) && !inattentive;
                cataimgui::draw_colored_text( sees ? "!" : " ", c_yellow );

                ImGui::TableNextColumn();
                nc_color mon_color = mon->basic_symbol_color();
                cataimgui::TextColoredTrimmed( it->get_selected_name(), mon_color );

                ImGui::TableNextColumn();
                nc_color hp_color = c_white;
                std::string hp_text;

                if( mon->is_monster() ) {
                    mon->as_monster()->get_HP_Bar( hp_color, hp_text );
                } else {
                    std::tie( hp_text, hp_color ) = get_hp_bar( mon->as_npc()->hp_percentage(), 100 );
                }
                const int bar_max_width = 5;
                const int bar_width = utf8_width( hp_text );
                if( bar_width < bar_max_width ) {
                    hp_text += "<color_c_white>";
                    for( int i = bar_max_width - bar_width; i > 0; i-- ) {
                        hp_text += ".";
                    }
                    hp_text += "</color>";
                }
                cataimgui::draw_colored_text( hp_text, hp_color );

                ImGui::TableNextColumn();
                nc_color att_color;
                std::string att_text;

                if( inattentive ) {
                    att_text = _( "Unknown" );
                    att_color = c_yellow;
                } else if( mon->is_monster() ) {
                    const std::pair<std::string, nc_color> att = mon->as_monster()->get_attitude();
                    att_text = att.first;
                    att_color = att.second;
                } else if( mon->is_npc() ) {
                    att_text = npc_attitude_name( mon->as_npc()->get_attitude() );
                    att_color = mon->symbol_color();
                }
                cataimgui::draw_colored_text( att_text, att_color );

                ImGui::TableNextColumn();
                cataimgui::draw_colored_text( it->get_selected_distance_string(), c_light_gray );
            }
            ImGui::EndTable();
        }

        draw_hotkey_buttons( &monster_data );

        // embedded info for selected monster
        if( ImGui::BeginChild( "info", ImVec2( 0.0f, str_height_to_pixels( info_height ) ) ) ) {
            if( monster_data.selected_entry ) {
                draw_extended_description(
                    monster_data.selected_entry->get_selected_entity()->extended_description(),
                    str_width_to_pixels( width ), info_scroll );
            }
        }
        ImGui::EndChild();
        ImGui::EndTabItem();
    }
}

void surroundings_menu::draw_terfurn_tab()
{
    ImGuiTabItemFlags_ flags = ImGuiTabItemFlags_None;
    if( switch_tab == surroundings_menu_tab_enum::terfurn ) {
        flags = ImGuiTabItemFlags_SetSelected;
        switch_tab = surroundings_menu_tab_enum::num_tabs;
    }

    std::string title = string_format( "%s (%d)", terfurn_data.title,
                                       terfurn_data.filtered_list.size() );

    if( ImGui::BeginTabItem( title.c_str(), nullptr, flags ) ) {
        selected_tab = surroundings_menu_tab_enum::terfurn;

        float list_height = str_height_to_pixels( TERMY - info_height ) -
                            get_hotkey_buttons_height( &terfurn_data ) - magic_number_other_elements_height;
        if( ImGui::BeginTable( "terfurn", 2, ImGuiTableFlags_ScrollY, ImVec2( 0.0f, list_height ) ) ) {
            ImGui::TableSetupColumn( "tf_name",
                                     ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize,
                                     str_width_to_pixels( width - dist_width - 3 ) );
            ImGui::TableSetupColumn( "tf_distance",
                                     ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize,
                                     str_width_to_pixels( dist_width ) );
            if( terfurn_data.filtered_list.empty() ) {
                terfurn_data.selected_entry = nullptr;
            } else if( !terfurn_data.selected_entry ||
                       std::find( terfurn_data.filtered_list.begin(), terfurn_data.filtered_list.end(),
                                  terfurn_data.selected_entry ) == terfurn_data.filtered_list.end() ) {
                terfurn_data.selected_entry = terfurn_data.filtered_list.front();
            }
            std::string last_category;
            int entry_no = 0;
            for( map_entity_stack<map_data_common_t> *it : terfurn_data.filtered_list ) {
                if( terfurn_data.draw_categories ) {
                    draw_category_separator( it->get_category(), last_category, 0 );
                }
                bool is_selected = it == terfurn_data.selected_entry;
                ImGui::TableNextColumn();
                ImGui::PushID( entry_no++ );
                ImGui::Selectable( "", &is_selected,
                                   ImGuiSelectableFlags_AllowItemOverlap | ImGuiSelectableFlags_SpanAllColumns, ImVec2( 0, 0 ) );
                if( is_selected ) {
                    if( auto_scroll ) {
                        ImGui::SetScrollHereY();
                        auto_scroll = false;
                    }
                    terfurn_data.selected_entry = it;
                }
                ImGui::SameLine( 0, 0 );
                ImGui::PopID();
                const map_data_common_t *terfurn = it->get_selected_entity();
                nc_color color = terfurn->color();
                cataimgui::TextColoredTrimmed( it->get_selected_name(), color );

                ImGui::TableNextColumn();
                cataimgui::draw_colored_text( it->get_selected_distance_string(), c_light_gray );
            }
            ImGui::EndTable();
        }

        draw_hotkey_buttons( &terfurn_data );

        // embedded info for selected terrain/furniture
        if( ImGui::BeginChild( "info", ImVec2( 0.0f, str_height_to_pixels( info_height ) ) ) ) {
            if( terfurn_data.selected_entry ) {
                draw_extended_description(
                    terfurn_data.selected_entry->get_selected_entity()->extended_description(),
                    str_width_to_pixels( width ), info_scroll );
            }
        }
        ImGui::EndChild();
        ImGui::EndTabItem();
    }
}

void surroundings_menu::draw_examine_info()
{
    switch( selected_tab ) {
        case surroundings_menu_tab_enum::items: {
            std::vector<iteminfo> selected_info;
            std::vector<iteminfo> dummy_info;
            const item *selected_item = item_data.selected_entry->get_selected_entity();
            selected_item->info( true, selected_info );
            item_info_data dummy( selected_item->tname(), selected_item->type_name(), selected_info,
                                  dummy_info );
            dummy.handle_scrolling = true;
            dummy.arrow_scrolling = true;
            iteminfo_window info( dummy, point::zero, width - 5, TERMY );
            info.execute();
        }
        break;
        case surroundings_menu_tab_enum::monsters:
        case surroundings_menu_tab_enum::terfurn:
            break;
        default:
            debugmsg( "invalid tab selected" );
            break;
    }
}

void surroundings_menu::execute()
{
    while( true ) {
        update_path_end();
        g->invalidate_main_ui_adaptor();

        ui_manager::redraw();

        std::string action;
        if( has_button_action() ) {
            action = get_button_action();
        } else {
            action = get_selected_data()->ctxt.handle_input();
        }

        if( action == "UP" ) {
            select_prev();
        } else if( action == "DOWN" ) {
            select_next();
        } else if( action == "PAGE_UP" ) {
            select_page_up();
        } else if( action == "PAGE_DOWN" ) {
            select_page_down();
        } else if( action == "SCROLL_ITEM_INFO_UP" ) {
            info_scroll = cataimgui::scroll::page_up;
        } else if( action == "SCROLL_ITEM_INFO_DOWN" ) {
            info_scroll = cataimgui::scroll::page_down;
        } else if( action == "NEXT_TAB" ) {
            switch_tab = selected_tab;
            ++switch_tab;
        } else if( action == "PREV_TAB" ) {
            switch_tab = selected_tab;
            --switch_tab;
        } else if( action == "fire" ) {
            tab_data *data = get_selected_data();
            if( data->fire_at_selected( you, max_gun_range, stored_view_offset ) ) {
                break;
            }
        } else if( action == "QUIT" ) {
            break;
        } else if( action == "TRAVEL_TO" && path_end ) {
            you.view_offset = stored_view_offset;
            if( !you.sees( get_map(), *path_end ) ) {
                add_msg( _( "You can't see that destination." ) );
            }
            // try finding route to the tile, or one tile away from it
            const std::optional<std::vector<tripoint_bub_ms>> try_route =
            g->safe_route_to( you, *path_end, 1, []( const std::string & msg ) {
                popup( msg );
            } );
            if( try_route.has_value() ) {
                you.set_destination( *try_route );
                break;
            }
        } else if( action == "EXAMINE" ) {
            draw_examine_info();
        } else if( action == "COMPARE" ) {
            hide_ui = true;
            std::optional<tripoint_rel_ms> rel_pos;
            if( path_end ) {
                rel_pos = ( *path_end - you.pos_bub() );
            }
            game_menus::inv::compare( rel_pos );
            hide_ui = false;
        } else if( action == "SORT" ) {
            change_selected_tab_sorting();
        } else if( action == "FILTER" || action == "FILTER_MONSTERS" ) {
            get_filter();
        } else if( action == "RESET_FILTER" ) {
            reset_filter();
        } else if( action == "PRIORITY_INCREASE" ) {
            get_filter_priority( true );
        } else if( action == "PRIORITY_DECREASE" ) {
            get_filter_priority( false );
        } else {
            handle_list_input( action );
        }
    }

    uistate.vmenu_tab = selected_tab;
}

void surroundings_menu::get_filter()
{
    tab_data *data = get_selected_data();
    data->filter_popup();
    data->apply_filter();
    // FIXME: hack to prevent tab switching after filtering
    switch_tab = selected_tab;
}

void surroundings_menu::reset_filter()
{
    tab_data *data = get_selected_data();
    data->reset_filter();
    data->apply_filter();
    // FIXME: hack to prevent tab switching after filtering
    switch_tab = selected_tab;
}

void surroundings_menu::get_filter_priority( bool high )
{
    switch( selected_tab ) {
        case surroundings_menu_tab_enum::items: {
            tab_data *data = get_selected_data();
            data->priority_popup( high );
            data->apply_filter();
            break;
        }
        default:
            //priority only implemented for items
            break;
    }
}

void surroundings_menu::update_path_end()
{
    std::optional<tripoint_rel_ms> p = get_selected_pos();

    if( p ) {
        path_end = you.pos_bub() + *p;
    } else {
        path_end = std::nullopt;
    }
}

std::optional<tripoint_rel_ms> surroundings_menu::get_selected_pos()
{
    tab_data *data = get_selected_data();
    return data->get_selected_pos();
}

static void toggle_safemode_entry( const map_entity_stack<Creature> *mstack, bool add )
{
    safemode &sm = get_safemode();
    if( !mstack || sm.empty() ) {
        return;
    }
    const Creature *mon = mstack->get_selected_entity();
    if( !mon ) {
        return;
    }
    std::string mon_name = "human";
    if( mon->is_monster() ) {
        mon_name = mon->as_monster()->name();
    }

    if( add ) {
        sm.add_rule( mon_name, Creature::Attitude::ANY, get_option<int>( "SAFEMODEPROXIMITY" ),
                     rule_state::BLACKLISTED );
    } else if( get_safemode().has_rule( mon_name, Creature::Attitude::ANY ) ) {
        get_safemode().remove_rule( mon_name, Creature::Attitude::ANY );
    }
}

void surroundings_menu::handle_list_input( const std::string &action )
{
    if( action == "LEFT" ) {
        select_prev_internal();
    } else if( action == "RIGHT" ) {
        select_next_internal();
    } else if( action == "look" ) {
        hide_ui = true;
        g->look_around();
        hide_ui = false;
    } else if( action == "zoom_in" ) {
        g->zoom_in();
        g->mark_main_ui_adaptor_resize();
    } else if( action == "zoom_out" ) {
        g->zoom_out();
        g->mark_main_ui_adaptor_resize();
    } else if( action == "SAFEMODE_BLACKLIST_ADD" &&
               selected_tab == surroundings_menu_tab_enum::monsters ) {
        toggle_safemode_entry( monster_data.selected_entry, true );
    } else if( action == "SAFEMODE_BLACKLIST_REMOVE" &&
               selected_tab == surroundings_menu_tab_enum::monsters ) {
        toggle_safemode_entry( monster_data.selected_entry, false );
    }
}

tab_data *surroundings_menu::get_tab_data( surroundings_menu_tab_enum tab )
{
    switch( tab ) {
        case surroundings_menu_tab_enum::items:
            return &item_data;
        case surroundings_menu_tab_enum::monsters:
            return &monster_data;
        case surroundings_menu_tab_enum::terfurn:
            return &terfurn_data;
        default:
            debugmsg( "invalid tab selected, defaulting to items" );
            return &item_data;
    }
}

tab_data *surroundings_menu::get_selected_data()
{
    return get_tab_data( selected_tab );
}

void surroundings_menu::select_prev()
{
    auto_scroll = true;
    tab_data *selected = get_selected_data();
    selected->move_selection( -1 );
    info_scroll = cataimgui::scroll::begin;
}

void surroundings_menu::select_next()
{
    auto_scroll = true;
    tab_data *selected = get_selected_data();
    selected->move_selection( 1 );
    info_scroll = cataimgui::scroll::begin;
}

void surroundings_menu::select_page_up()
{
    auto_scroll = true;
    tab_data *selected = get_selected_data();
    selected->move_selection( -page_scroll );
    info_scroll = cataimgui::scroll::begin;
}

void surroundings_menu::select_page_down()
{
    auto_scroll = true;
    tab_data *selected = get_selected_data();
    selected->move_selection( page_scroll );
    info_scroll = cataimgui::scroll::begin;
}

void surroundings_menu::select_prev_internal()
{
    tab_data *selected = get_selected_data();
    selected->select_prev_internal();
}

void surroundings_menu::select_next_internal()
{
    tab_data *selected = get_selected_data();
    selected->select_next_internal();
}

void surroundings_menu::change_selected_tab_sorting()
{
    auto_scroll = true;
    tab_data *data = get_selected_data();
    data->draw_categories = !data->draw_categories;
    data->apply_filter();
}

void surroundings_menu::draw_hotkey_buttons( const tab_data *tab )
{
    float pos = ImGui::GetStyle().WindowPadding.x;
    float button_inner_padding = ImGui::GetStyle().FramePadding.x;
    float posy = ImGui::GetCursorPosY();
    float line_height = str_height_to_pixels( 1 ) + ImGui::GetStyle().FramePadding.y +
                        ImGui::GetStyle().ItemSpacing.y;
    for( const std::unordered_set<std::string> &group : tab->hotkey_groups ) {
        for( const std::string &button : group ) {
            // todo: don't just hardcode this
            if( button == "RESET_FILTER" && !tab->has_filter() ) {
                continue;
            }
            if( tab->is_monster_tab() ) {
                const monster_tab_data *mdata = static_cast<const monster_tab_data *>( tab );
                map_entity_stack<Creature> *mstack = mdata->selected_entry;
                if( mstack ) {
                    const Creature *critter = mstack->get_selected_entity();
                    if( critter && critter->is_monster() ) {
                        std::string mon_name = critter->as_monster()->name();
                        safemode sm = get_safemode();
                        if( get_safemode().has_rule( mon_name, Creature::Attitude::ANY ) ) {
                            if( button == "SAFEMODE_BLACKLIST_ADD" ) {
                                continue;
                            }
                        } else if( button == "SAFEMODE_BLACKLIST_REMOVE" ) {
                            continue;
                        }
                    }
                    if( button == "fire" && !tab->can_fire_at_selected( you, max_gun_range ) ) {
                        continue;
                    }
                }
            }

            std::string buttontxt = tab->ctxt.get_button_text( button );
            float next_pos = get_text_width( buttontxt ) + button_inner_padding +
                             ImGui::GetStyle().ItemSpacing.x;

            if( pos + next_pos > str_width_to_pixels( width ) ) {
                posy += line_height;
                pos = ImGui::GetStyle().WindowPadding.x;
            }

            ImGui::SetCursorPosX( pos );
            ImGui::SetCursorPosY( posy );
            action_button( button, buttontxt );

            pos += next_pos;
        }
        pos = ImGui::GetStyle().WindowPadding.x;
        posy += line_height;
    }
}

float surroundings_menu::get_hotkey_buttons_height( const tab_data *tab )
{
    if( tab->hotkey_groups.empty() ) {
        return 0;
    }

    int pos = 0;
    int lines = 1;
    for( const std::unordered_set<std::string> &group : tab->hotkey_groups ) {
        for( const std::string &button : group ) {
            pos += ImGui::GetStyle().FramePadding.x;

            std::string buttontxt = tab->ctxt.get_button_text( button );
            float next_pos = get_text_width( buttontxt ) + ImGui::GetStyle().FramePadding.x +
                             ImGui::GetStyle().ItemSpacing.x;

            if( pos + next_pos > str_width_to_pixels( width ) ) {
                lines++;
                pos = ImGui::GetStyle().FramePadding.x;
            }

            pos += next_pos;
        }
        lines++;
    }

    float line_height = str_height_to_pixels( 1 ) + ImGui::GetStyle().FramePadding.y +
                        ImGui::GetStyle().ItemSpacing.y;

    return lines * line_height;
}
