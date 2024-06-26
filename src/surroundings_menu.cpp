#include "surroundings_menu.h"

#include "game.h"
#include "game_inventory.h"
#include "item_search.h"
#include "messages.h"
#include "monster.h"
#include "npc.h"
#include "options.h"
#include "safemode_ui.h"
#include "ui_manager.h"
#include "imgui/imgui.h"

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

//static std::string list_items_filter_history_help()
//{
//    return colorize(_("UP: history, CTRL-U: clear line, ESC: abort, ENTER: save"), c_green);
//}

tab_data::tab_data( const std::string &title ) : title( title )
{
    ctxt = input_context( "LIST_SURROUNDINGS" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "UP" );
    ctxt.register_action( "DOWN" );
    ctxt.register_action( "LEFT" );
    ctxt.register_action( "RIGHT" );
    ctxt.register_action( "look" );
    ctxt.register_action( "zoom_in" );
    ctxt.register_action( "zoom_out" );
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_action( "SCROLL_SURROUNDINGS_INFO_UP" );
    ctxt.register_action( "SCROLL_SURROUNDINGS_INFO_DOWN" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "RESET_FILTER" );
    ctxt.register_action( "SORT" );
    ctxt.register_action( "TRAVEL_TO" );

    // arbitrary timeout to fix imgui input weirdness
    ctxt.set_timeout( 10 );
}

item_tab_data::item_tab_data( const Character &you, map &m,
                              const std::string &title ) : tab_data( title )
{
    ctxt.register_action( "EXAMINE" );
    ctxt.register_action( "COMPARE" );
    ctxt.register_action( "PRIORITY_INCREASE" );
    ctxt.register_action( "PRIORITY_DECREASE" );

    find_nearby_items( you, m );
    filtered_list = item_list;
    selected_entry = filtered_list.front();
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
            you.sees( points_p_it ) && m.sees_some_items( points_p_it.raw(), you ) ) {

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

void item_tab_data::apply_filter( std::string filter )
{
    this->filter = filter;

    filtered_list.clear();

    auto z = item_filter_from_string( filter );
    std::copy_if( item_list.begin(), item_list.end(),
    std::back_inserter( filtered_list ), [z]( const map_entity_stack<item> *a ) {
        return z( *a->get_selected_entity() );
    } );
}

monster_tab_data::monster_tab_data( const Character &you,
                                    const std::string &title ) : tab_data( title )
{
    ctxt.register_action( "fire" );
    ctxt.register_action( "SAFEMODE_BLACKLIST_TOGGLE" );

    find_nearby_monsters( you );
    filtered_list = monster_list;
    selected_entry = filtered_list.front();
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

void monster_tab_data::apply_filter( std::string filter )
{
    this->filter = filter;

    filtered_list.clear();

    // todo: filter_from_string<Creature>
    // for now just matching creature name
    // auto z = item_filter_from_string( filter );
    std::copy_if( monster_list.begin(), monster_list.end(),
    std::back_inserter( filtered_list ), [filter]( const map_entity_stack<Creature> *a ) {
        return lcmatch( remove_color_tags( a->get_selected_entity()->disp_name() ), filter );
    } );
}

terfurn_tab_data::terfurn_tab_data( const Character &you, map &m,
                                    const std::string &title ) : tab_data( title )
{
    find_nearby_terfurn( you, m );
    filtered_list = terfurn_list;
    selected_entry = filtered_list.front();
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
            you.sees( points_p_it ) ) {
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

void terfurn_tab_data::apply_filter( std::string filter )
{
    this->filter = filter;

    filtered_list.clear();

    // todo: filter_from_string<map_data_common_t>
    // for now just matching creature name
    //auto z = item_filter_from_string( filter );
    std::copy_if( terfurn_list.begin(), terfurn_list.end(),
    std::back_inserter( filtered_list ), [filter]( const map_entity_stack<map_data_common_t> *a ) {
        return lcmatch( remove_color_tags( a->get_selected_entity()->name() ), filter );
    } );
}

//static text_block get_distance_text_block( const tripoint_rel_ms &p1, const tripoint_rel_ms &p2 )
//{
//    ui_element_style style( c_light_gray, c_light_green );
//    const int dist = rl_dist( p1, p2 );
//    const int num_width = dist > 9 ? 2 : 1;
//
//    std::string text = string_format( "%*d %s", num_width, dist,
//                                      direction_name_short( direction_from( p1, p2 ) ) );
//    return text_block( text, style );
//
//}
//
//static shared_ptr_fast<ui_element> draw_item( const map_entity_stack<item> *mstack,
//        const int width )
//{
//    absolute_layout layout( point_zero, width, 1 );
//
//    const item *it = mstack->get_selected_entity();
//    if( it ) {
//        // distance string length: 1 space, 2 distance number, 1 space, 5 direction (UP_NE) = 9
//        const int dist_width = 9;
//        const int name_width = width - dist_width;
//        std::string text;
//        if( mstack->entities.size() > 1 ) {
//            text = string_format( "[%d/%d] (%d) ", mstack->get_selected_index() + 1, mstack->entities.size(),
//                                  mstack->totalcount );
//        }
//
//        const int count = mstack->get_selected_count();
//        if( count > 1 ) {
//            text += string_format( "%d ", count );
//        }
//
//        text += it->display_name( count );
//
//        ui_element_style style( it->color_in_inventory(), hilite( it->color_in_inventory() ) );
//
//        shared_ptr_fast<ui_element> name = make_shared_fast<text_block>( text, style, point_east,
//                                           name_width, 1 );
//        layout.add_element( name );
//
//        text_block dist = get_distance_text_block( tripoint_rel_ms(), *mstack->get_selected_pos() );
//        dist.pos = point( width - dist.text.size(), 0 );
//        dist.width = dist.text.size();
//        dist.height = 1;
//
//        shared_ptr_fast<ui_element> dist_ptr = make_shared_fast<text_block>( dist );
//        layout.add_element( dist_ptr );
//    }
//
//    return make_shared_fast<absolute_layout>( layout );
//}
//
//static shared_ptr_fast<ui_element> draw_monster( const map_entity_stack<Creature> *mstack,
//        const int width )
//{
//    absolute_layout layout( point_zero, width, 1 );
//
//    const Creature *mon = mstack->get_selected_entity();
//    if( mon ) {
//        const int hp_width = 5;
//        const int att_width = 18;
//        const int dist_width = 9;
//        const int name_width = width - hp_width - att_width - dist_width;
//        int cur_pos = 1;
//
//        const Character &you = get_player_character();
//        const bool inattentive = you.has_trait( trait_INATTENTIVE );
//
//        if( mon->sees( you ) && !inattentive ) {
//            shared_ptr_fast<ui_element> sees = make_shared_fast<text_block>( "!", ui_element_style( c_yellow ),
//                                               point_zero, 1, 1 );
//            layout.add_element( sees );
//        }
//
//        std::string text;
//        if( mon->is_monster() ) {
//            text = mon->as_monster()->name();
//        } else {
//            text = mon->disp_name();
//        }
//
//        ui_element_style style( mon->basic_symbol_color(), hilite( mon->basic_symbol_color() ) );
//        shared_ptr_fast<ui_element> name = make_shared_fast<text_block>( text, style, point_east,
//                                           name_width, 1 );
//        layout.add_element( name );
//
//        cur_pos += name_width;
//
//        nc_color hp_color = c_white;
//        std::string hp_text;
//
//        if( mon->is_monster() ) {
//            mon->as_monster()->get_HP_Bar( hp_color, hp_text );
//        } else {
//            std::tie( hp_text, hp_color ) = get_hp_bar( mon->as_npc()->hp_percentage(), 100 );
//        }
//        const int bar_max_width = 5;
//        const int bar_width = utf8_width( hp_text );
//        if( bar_width < bar_max_width ) {
//            hp_text += "<color_c_white>";
//            for( int i = bar_max_width - bar_width; i > 0; i-- ) {
//                hp_text += ".";
//            }
//            hp_text += "</color>";
//        }
//        shared_ptr_fast<ui_element> hp = make_shared_fast<text_block>( hp_text,
//                                         ui_element_style( hp_color ), point( cur_pos, 0 ), hp_width, 1 );
//        layout.add_element( hp );
//        cur_pos += hp_width;
//
//        nc_color att_color;
//        std::string att_text;
//
//        if( inattentive ) {
//            att_text = _( "Unknown" );
//            att_color = c_yellow;
//        } else if( mon->is_monster() ) {
//            const std::pair<std::string, nc_color> att = mon->as_monster()->get_attitude();
//            att_text = att.first;
//            att_color = att.second;
//        } else if( mon->is_npc() ) {
//            att_text = npc_attitude_name( mon->as_npc()->get_attitude() );
//            att_color = mon->symbol_color();
//        }
//        shared_ptr_fast<ui_element> att = make_shared_fast<text_block>( att_text,
//                                          ui_element_style( att_color ), point( cur_pos, 0 ), att_width, 1 );
//        layout.add_element( att );
//
//        text_block dist = get_distance_text_block( tripoint_rel_ms(), *mstack->get_selected_pos() );
//        dist.pos = point( width - dist.text.size(), 0 );
//        dist.width = dist.text.size();
//        dist.height = 1;
//        shared_ptr_fast<ui_element> dist_ptr = make_shared_fast<text_block>( dist );
//        layout.add_element( dist_ptr );
//    }
//
//    return make_shared_fast<absolute_layout>( layout );
//}
//
//static shared_ptr_fast<ui_element> draw_terfurn( const map_entity_stack<map_data_common_t> *mstack,
//        const int width )
//{
//    absolute_layout layout( point_zero, width, 1 );
//
//    const map_data_common_t *terfurn = mstack->get_selected_entity();
//    if( terfurn ) {
//        // distance string length: 1 space, 2 distance number, 1 space, 5 direction (UP_NE) = 9
//        const int dist_width = 9;
//        const int name_width = width - dist_width;
//        std::string text;
//        if( mstack->entities.size() > 1 ) {
//            text = string_format( "[%d/%d] ", mstack->get_selected_index() + 1, mstack->entities.size() );
//        }
//        text += terfurn->name();
//
//        ui_element_style style( terfurn->color(), hilite( terfurn->color() ) );
//
//        shared_ptr_fast<ui_element> name = make_shared_fast<text_block>( text, style, point_east,
//                                           name_width, 1 );
//        layout.add_element( name );
//
//        text_block dist = get_distance_text_block( tripoint_rel_ms(), *mstack->get_selected_pos() );
//        dist.pos = point( width - dist.text.size(), 0 );
//        dist.width = dist.text.size();
//        dist.height = 1;
//        shared_ptr_fast<ui_element> dist_ptr = make_shared_fast<text_block>( dist );
//        layout.add_element( dist_ptr );
//    }
//    return make_shared_fast<absolute_layout>( layout );
//}

surroundings_menu::surroundings_menu( avatar &you, map &m,
                                      std::optional<tripoint> &path_end, int width ) :
    cataimgui::window( "SURROUNDINGS", ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                       ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoNavInputs ),
    you( you ),
    m( m ),
    path_end( path_end ),
    stored_view_offset( you.view_offset ),
    item_data( you, m ),
    monster_data( you ),
    terfurn_data( you, m ),
    width( width ),
    info_height( std::min( 25, TERMY / 2 ) )
{
}

cataimgui::bounds surroundings_menu::get_bounds()
{
    info_height = std::min( 25, TERMY / 2 );
    return { static_cast<float>( str_width_to_pixels( TERMX - width ) ), 0.f, static_cast<float>( str_width_to_pixels( width ) ), static_cast<float>( str_height_to_pixels( TERMY ) ) };
}

void surroundings_menu::draw_controls()
{
    if( ImGui::BeginTabBar( "surroundings tabs" ) ) {
        draw_item_tab();
        draw_monster_tab();
        draw_terfurn_tab();
        ImGui::EndTabBar();
    }
}

static std::string get_distance_string( tripoint_rel_ms p1, tripoint_rel_ms p2 )
{
    const int dist = rl_dist( p1, p2 );

    std::string text = string_format( "%*d %s", 2, dist,
                                      direction_name_short( direction_from( p1, p2 ) ) );

    return text;
}

// todo: get ImGui::SeparatorText to draw on the entire row
void surroundings_menu::draw_category_separator( const std::string &category,
        std::string &last_category, int target_col )
{
    if( !category.empty() && ( category != last_category ) ) {
        last_category = category;
        do {
            ImGui::TableNextColumn();
            target_col--;
        } while( target_col >= 0 );
        draw_colored_text( category, c_magenta );

    }
    ImGui::TableNextRow();
}

void surroundings_menu::draw_item_tab()
{
    ImGuiTabItemFlags_ flags = ImGuiTabItemFlags_None;
    if( switch_tab == surroundings_menu_tab_enum::items ) {
        flags = ImGuiTabItemFlags_SetSelected;
        switch_tab = surroundings_menu_tab_enum::num_tabs;
    }
    if( ImGui::BeginTabItem( item_data.title.c_str(), nullptr, flags ) ) {
        selected_tab = surroundings_menu_tab_enum::items;
        if( ImGui::BeginTable( "items", 2, ImGuiTableFlags_ScrollY,
                               ImVec2( 0.0f, str_height_to_pixels( TERMY - info_height ) ) ) ) {
            ImGui::TableSetupColumn( "it_name", ImGuiTableColumnFlags_NoDirectResize_,
                                     str_width_to_pixels( width - dist_width - 3 ) );
            ImGui::TableSetupColumn( "it_distance", ImGuiTableColumnFlags_NoDirectResize_,
                                     str_width_to_pixels( dist_width ) );
            if( !item_data.selected_entry ||
                std::find( item_data.filtered_list.begin(), item_data.filtered_list.end(),
                           item_data.selected_entry ) == item_data.filtered_list.end() ) {
                item_data.selected_entry = item_data.filtered_list.front();
            }
            std::string last_category;
            int entry_no = 0;
            for( map_entity_stack<item> *it : item_data.filtered_list ) {
                if( item_data.draw_categories ) {
                    draw_category_separator( it->get_category(), last_category, 0 );
                }
                bool is_selected = it == item_data.selected_entry;
                ImGui::TableNextColumn();
                ImGui::PushID( entry_no++ );
                ImGui::Selectable( "", &is_selected,
                                   ImGuiSelectableFlags_AllowItemOverlap | ImGuiSelectableFlags_SpanAllColumns, ImVec2( 0, 0 ) );
                if( is_selected ) {
                    if( scroll ) {
                        ImGui::SetScrollHereY();
                        scroll = false;
                    }
                    item_data.selected_entry = it;
                }
                ImGui::SameLine( 0, 0 );
                ImGui::PopID();
                const item *itm = it->get_selected_entity();
                std::string text;
                if( it->entities.size() > 1 ) {
                    text = string_format( "[%d/%d] (%d) ", it->get_selected_index() + 1, it->entities.size(),
                                          it->totalcount );
                }

                const int count = it->get_selected_count();
                if( count > 1 ) {
                    text += string_format( "%d ", count );
                }

                text += itm->display_name( count );
                nc_color color = it->get_selected_entity()->color_in_inventory();
                draw_colored_text( text, color );

                ImGui::TableNextColumn();
                std::string dist_text = get_distance_string( tripoint_rel_ms(), *it->get_selected_pos() );
                draw_colored_text( dist_text, c_light_gray );
            }
            ImGui::EndTable();
        }
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
    if( ImGui::BeginTabItem( monster_data.title.c_str(), nullptr, flags ) ) {
        selected_tab = surroundings_menu_tab_enum::monsters;
        if( ImGui::BeginTable( "monsters", 5, ImGuiTableFlags_ScrollY,
                               ImVec2( 0.0f, str_height_to_pixels( TERMY - info_height ) ) ) ) {
            ImGui::TableSetupColumn( "mon_sees", ImGuiTableColumnFlags_NoDirectResize_,
                                     str_width_to_pixels( 1 ) );
            ImGui::TableSetupColumn( "mon_name", ImGuiTableColumnFlags_NoDirectResize_,
                                     str_width_to_pixels( width - dist_width - 26 ) );
            ImGui::TableSetupColumn( "mon_health", ImGuiTableColumnFlags_NoDirectResize_,
                                     str_width_to_pixels( 5 ) );
            ImGui::TableSetupColumn( "mon_attitude", ImGuiTableColumnFlags_NoDirectResize_,
                                     str_width_to_pixels( 18 ) );
            ImGui::TableSetupColumn( "mon_distance", ImGuiTableColumnFlags_NoDirectResize_,
                                     str_width_to_pixels( dist_width ) );
            if( !monster_data.selected_entry ||
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
                    if( scroll ) {
                        ImGui::SetScrollHereY();
                        scroll = false;
                    }
                    monster_data.selected_entry = it;
                }
                ImGui::SameLine( 0, 0 );
                ImGui::PopID();
                const Creature *mon = it->get_selected_entity();
                const Character &you = get_player_character();
                const bool inattentive = you.has_trait( trait_INATTENTIVE );
                bool sees = mon->sees( you ) && !inattentive;
                draw_colored_text( sees ? "!" : " ", c_yellow );

                ImGui::TableNextColumn();
                std::string mon_name;
                if( mon->is_monster() ) {
                    mon_name = mon->as_monster()->name();
                } else {
                    mon_name = mon->disp_name();
                }
                nc_color mon_color = mon->basic_symbol_color();
                draw_colored_text( mon_name, mon_color );
                if( is_selected ) {
                    ImGui::SetScrollHereY();
                }

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
                draw_colored_text( hp_text, hp_color );

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
                draw_colored_text( att_text, att_color );

                ImGui::TableNextColumn();
                std::string dist_text = get_distance_string( tripoint_rel_ms(), *it->get_selected_pos() );
                draw_colored_text( dist_text, c_light_gray );
            }
            ImGui::EndTable();
        }
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
    if( ImGui::BeginTabItem( terfurn_data.title.c_str(), nullptr, flags ) ) {
        selected_tab = surroundings_menu_tab_enum::terfurn;
        if( ImGui::BeginTable( "terfurn", 2, ImGuiTableFlags_ScrollY,
                               ImVec2( 0.0f, str_height_to_pixels( TERMY - info_height ) ) ) ) {
            ImGui::TableSetupColumn( "tf_name", ImGuiTableColumnFlags_NoDirectResize_,
                                     str_width_to_pixels( width - dist_width - 3 ) );
            ImGui::TableSetupColumn( "tf_distance", ImGuiTableColumnFlags_NoDirectResize_,
                                     str_width_to_pixels( dist_width ) );
            if( !terfurn_data.selected_entry ||
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
                    if( scroll ) {
                        ImGui::SetScrollHereY();
                        scroll = false;
                    }
                    terfurn_data.selected_entry = it;
                }
                ImGui::SameLine( 0, 0 );
                ImGui::PopID();
                const map_data_common_t *terfurn = it->get_selected_entity();
                std::string text;
                if( it->entities.size() > 1 ) {
                    text = string_format( "[%d/%d] ", it->get_selected_index() + 1, it->entities.size() );
                }
                text += terfurn->name();
                nc_color color = terfurn->color();
                draw_colored_text( text, color );

                ImGui::TableNextColumn();
                std::string dist_text = get_distance_string( tripoint_rel_ms(), *it->get_selected_pos() );
                draw_colored_text( dist_text, c_light_gray );
            }
            ImGui::EndTable();
        }
        ImGui::EndTabItem();
    }
}

void surroundings_menu::draw_examine_info()
{
    switch( selected_tab ) {
        case surroundings_menu_tab_enum::items:
            break;
        case surroundings_menu_tab_enum::monsters:
            break;
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
        std::string action = get_selected_data()->ctxt.handle_input();

        if( action == "UP" ) {
            select_prev();
        } else if( action == "DOWN" ) {
            select_next();
        } else if( action == "PAGE_UP" ) {
            select_page_up();
        } else if( action == "PAGE_DOWN" ) {
            select_page_down();
        } else if( action == "NEXT_TAB" ) {
            switch_tab = selected_tab;
            ++switch_tab;
        } else if( action == "PREV_TAB" ) {
            switch_tab = selected_tab;
            --switch_tab;
        } else if( action == "fire" ) {
            // todo: maybe refactor to not close the menu here, obsoletes the whole enum
            //return surroundings_menu_ret::fire;
        } else if( action == "QUIT" ) {
            return;
        } else if( action == "TRAVEL_TO" && path_end ) {
            you.view_offset = stored_view_offset;
            if( !you.sees( *path_end ) ) {
                add_msg( _( "You can't see that destination." ) );
            }
            // try finding route to the tile, or one tile away from it
            const std::optional<std::vector<tripoint_bub_ms>> try_route =
            g->safe_route_to( you, tripoint_bub_ms( *path_end ), 1, []( const std::string & msg ) {
                popup( msg );
            } );
            if( try_route.has_value() ) {
                you.set_destination( *try_route );
                break;
            }
        } else if( action == "COMPARE" ) {
            game_menus::inv::compare( you, path_end );
        } else if( action == "SORT" ) {
            change_selected_tab_sorting();
        } else if( action == "FILTER" ) {
            get_filter();
        } else if( action == "RESET_FILTER" ) {
            reset_filter();
        } else {
            handle_list_input( action );
        }
    }
}

void surroundings_menu::get_filter()
{
    switch( selected_tab ) {
        case surroundings_menu_tab_enum::items:
            break;
        case surroundings_menu_tab_enum::monsters:
            break;
        case surroundings_menu_tab_enum::terfurn:
            break;
        default:
            debugmsg( "invalid tab selected" );
            break;
    }
}

void surroundings_menu::reset_filter()
{
    switch( selected_tab ) {
        case surroundings_menu_tab_enum::items:
            item_data.reset_filter();
            break;
        case surroundings_menu_tab_enum::monsters:
            monster_data.reset_filter();
            break;
        case surroundings_menu_tab_enum::terfurn:
            terfurn_data.reset_filter();
            break;
        default:
            debugmsg( "invalid tab selected" );
            break;
    }
}

//else if( action == "PRIORITY_INCREASE" )
//{
//    get_string_input( _( "High Priority:" ), width, list_item_upvote, "list_item_priority" );
//} else if( action == "PRIORITY_DECREASE" )
//{
//    get_string_input( _( "Low Priority:" ), width, list_item_downvote, "list_item_downvote" );
//} else if( action == "EXAMINE" ) {
//    catacurses::window temp_info = catacurses::newwin(TERMY, width - 5, point_zero);
//    int scroll = 0;
//    draw_info(temp_info, scroll, true);

//}

void surroundings_menu::update_path_end()
{
    std::optional<tripoint_rel_ms> p = get_selected_pos();

    if( p ) {
        path_end = you.pos() + p->raw();
    } else {
        path_end = std::nullopt;
    }
}

void surroundings_menu::toggle_safemode_entry( const map_entity_stack<Creature> *mstack )
{
    safemode &sm = get_safemode();
    if( mstack && !sm.empty() ) {
        const Creature *mon = mstack->get_selected_entity();
        if( mon ) {
            std::string mon_name = "human";
            if( mon->is_monster() ) {
                mon_name = mon->as_monster()->name();
            }

            if( sm.has_rule( mon_name, Creature::Attitude::ANY ) ) {
                sm.remove_rule( mon_name, Creature::Attitude::ANY );
            } else {
                sm.add_rule( mon_name, Creature::Attitude::ANY, get_option<int>( "SAFEMODEPROXIMITY" ),
                             rule_state::BLACKLISTED );
            }
        }
    }
}

std::optional<tripoint_rel_ms> surroundings_menu::get_selected_pos()
{
    switch( selected_tab ) {
        case surroundings_menu_tab_enum::items:
            return item_data.selected_entry->get_selected_pos();
        case surroundings_menu_tab_enum::monsters:
            return monster_data.selected_entry->get_selected_pos();
        case surroundings_menu_tab_enum::terfurn:
            return terfurn_data.selected_entry->get_selected_pos();
        default:
            debugmsg( "invalid tab selected" );
            return std::nullopt;
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
    } else if( action == "SCROLL_SURROUNDINGS_INFO_UP" ) {
        //info_scroll_pos -= catacurses::getmaxy( info_window ) - 4;
    } else if( action == "SCROLL_SURROUNDINGS_INFO_DOWN" ) {
        //info_scroll_pos += catacurses::getmaxy( info_window ) - 4;
    } else if( action == "SAFEMODE_BLACKLIST_TOGGLE" &&
               selected_tab == surroundings_menu_tab_enum::monsters ) {
        toggle_safemode_entry( monster_data.selected_entry );
    }
}

tab_data *surroundings_menu::get_selected_data()
{
    switch( selected_tab ) {
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

template<typename T>
static void select_prev_generic( T &data )
{
    auto it = std::find( data.filtered_list.begin(), data.filtered_list.end(), data.selected_entry );
    if( it == data.filtered_list.begin() ) {
        data.selected_entry = *data.filtered_list.rbegin();
    } else if( it == data.filtered_list.end() ) {
        data.selected_entry = *data.filtered_list.begin();
    } else {
        data.selected_entry = *--it;
    }
}

template<typename T>
static void select_next_generic( T &data )
{
    auto it = std::find( data.filtered_list.begin(), data.filtered_list.end(), data.selected_entry );
    if( it == data.filtered_list.end() ) {
        data.selected_entry = *data.filtered_list.begin();
    } else if( ++it == data.filtered_list.end() ) {
        data.selected_entry = *data.filtered_list.begin();
    } else {
        data.selected_entry = *it;
    }
}

void surroundings_menu::select_prev()
{
    scroll = true;
    switch( selected_tab ) {
        case surroundings_menu_tab_enum::items:
            select_prev_generic( item_data );
            break;
        case surroundings_menu_tab_enum::monsters:
            select_prev_generic( monster_data );
            break;
        case surroundings_menu_tab_enum::terfurn:
            select_prev_generic( terfurn_data );
            break;
        default:
            debugmsg( "invalid tab selected" );
    }
}

void surroundings_menu::select_next()
{
    scroll = true;
    switch( selected_tab ) {
        case surroundings_menu_tab_enum::items:
            select_next_generic( item_data );
            break;
        case surroundings_menu_tab_enum::monsters:
            select_next_generic( monster_data );
            break;
        case surroundings_menu_tab_enum::terfurn:
            select_next_generic( terfurn_data );
            break;
        default:
            debugmsg( "invalid tab selected" );
    }
}

template<typename T>
void surroundings_menu::select_page_up_generic( T &data )
{
    auto it = std::find( data.filtered_list.begin(), data.filtered_list.end(), data.selected_entry );
    if( it == data.filtered_list.begin() ) {
        data.selected_entry = *data.filtered_list.rbegin();
    } else if( it == data.filtered_list.end() ||
               std::distance( data.filtered_list.begin(), it ) <= page_scroll ) {
        data.selected_entry = *data.filtered_list.begin();
    } else {
        std::advance( it, -page_scroll );
        data.selected_entry = *it;
    }
}

template<typename T>
void surroundings_menu::select_page_down_generic( T &data )
{
    auto it = std::find( data.filtered_list.begin(), data.filtered_list.end(), data.selected_entry );
    if( it == data.filtered_list.end() || it == data.filtered_list.end() - 1 ) {
        data.selected_entry = *data.filtered_list.begin();
    } else if( std::distance( it, data.filtered_list.end() ) <= page_scroll ) {
        data.selected_entry = *data.filtered_list.rbegin();
    } else {
        std::advance( it, page_scroll );
        data.selected_entry = *it;
    }
}

void surroundings_menu::select_page_up()
{
    scroll = true;
    switch( selected_tab ) {
        case surroundings_menu_tab_enum::items:
            select_page_up_generic( item_data );
            break;
        case surroundings_menu_tab_enum::monsters:
            select_page_up_generic( monster_data );
            break;
        case surroundings_menu_tab_enum::terfurn:
            select_page_up_generic( terfurn_data );
            break;
        default:
            debugmsg( "invalid tab selected" );
    }
}

void surroundings_menu::select_page_down()
{
    scroll = true;
    switch( selected_tab ) {
        case surroundings_menu_tab_enum::items:
            select_page_down_generic( item_data );
            break;
        case surroundings_menu_tab_enum::monsters:
            select_page_down_generic( monster_data );
            break;
        case surroundings_menu_tab_enum::terfurn:
            select_page_down_generic( terfurn_data );
            break;
        default:
            debugmsg( "invalid tab selected" );
    }
}

void surroundings_menu::select_prev_internal()
{
    switch( selected_tab ) {
        case surroundings_menu_tab_enum::items:
            item_data.selected_entry->select_prev();
            break;
        case surroundings_menu_tab_enum::monsters:
            monster_data.selected_entry->select_prev();
            break;
        case surroundings_menu_tab_enum::terfurn:
            terfurn_data.selected_entry->select_prev();
            break;
        default:
            debugmsg( "invalid tab selected" );
    }
}

void surroundings_menu::select_next_internal()
{
    switch( selected_tab ) {
        case surroundings_menu_tab_enum::items:
            item_data.selected_entry->select_next();
            break;
        case surroundings_menu_tab_enum::monsters:
            monster_data.selected_entry->select_next();
            break;
        case surroundings_menu_tab_enum::terfurn:
            terfurn_data.selected_entry->select_next();
            break;
        default:
            debugmsg( "invalid tab selected" );
    }
}

void surroundings_menu::change_selected_tab_sorting()
{

    switch( selected_tab ) {
        case surroundings_menu_tab_enum::items:
            item_data.draw_categories = !item_data.draw_categories;
            std::sort( item_data.filtered_list.begin(), item_data.filtered_list.end(),
            [&]( const map_entity_stack<item> *lhs, const map_entity_stack<item> *rhs ) {
                return lhs->compare( *rhs, item_data.draw_categories );
            } );
            break;
        case surroundings_menu_tab_enum::monsters:
            monster_data.draw_categories = !monster_data.draw_categories;
            std::sort( monster_data.filtered_list.begin(), monster_data.filtered_list.end(),
            [&]( const map_entity_stack<Creature> *lhs, const map_entity_stack<Creature> *rhs ) {
                return lhs->compare( *rhs, monster_data.draw_categories );
            } );
            break;
        case surroundings_menu_tab_enum::terfurn:
            terfurn_data.draw_categories = !terfurn_data.draw_categories;
            std::sort( terfurn_data.filtered_list.begin(), terfurn_data.filtered_list.end(),
                       [&]( const map_entity_stack<map_data_common_t> *lhs,
            const map_entity_stack<map_data_common_t> *rhs ) {
                return lhs->compare( *rhs, terfurn_data.draw_categories );
            } );
            break;
        default:
            debugmsg( "invalid tab selected" );
    }
}
