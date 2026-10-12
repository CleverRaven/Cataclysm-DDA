#include "body_status_ui.h" // IWYU pragma: associated

#include <memory>
#include <tuple>

#include "avatar_action.h"
#include "bodygraph.h"
#include "display.h"
#include "imgui/imgui.h"
#include "uilist.h"
#include "ui_manager.h"
#include "wcwidth.h"

static const bodygraph_id bodygraph_full_body( "full_body" );

//TODO: these static functions are identical to character creator's
static cataimgui::bounds uilist_reset_desired_bounds()
{
    return { 0, -1.0f, 0.33f * ImGui::GetMainViewport()->WorkSize.x, -1.0f };
}

static uilist_entry get_uilist_entry( const std::string &name )
{
    const bool screen_reader_mode = get_option<bool>( "SCREEN_READER_MODE" );
    uilist_entry cc_entry( screen_reader_mode ? std::string() : name );
    cc_entry.hotkey = input_event();
    cc_entry.text_color = c_white;

    return cc_entry;
}

// SX2: identical to CC ui
void body_status_uistate::set_description_scroll()
{
    if( scrolled_down ) {
        scrolled_down = false;
        ImGui::SetScrollY( std::clamp( ImGui::GetScrollY() + ImGui::GetContentRegionAvail().y, 0.0f,
                                       ImGui::GetScrollMaxY() ) );
    } else if( scrolled_up ) {
        scrolled_up = false;
        ImGui::SetScrollY( std::clamp( ImGui::GetScrollY() - ImGui::GetContentRegionAvail().y, 0.0f,
                                       ImGui::GetScrollMaxY() ) );
    }
}

body_status_uistate::body_status_uistate( Character &displayed_character, bodygraph_id bg_id ) :
    displayed_character( displayed_character )
{
    bodygraph_display_state = bodygraph_display( &displayed_character, bg_id );
}

void body_status_uistate::update_bodygraph_display()
{
    bodygraph_display_state.sel_part = selected_bodypart_index;
}

void body_status_uistate::init()
{
    bodygraph_display_state.prepare_partlist();
    bodygraph_display_state.prepare_infolist();
    all_bodyparts = displayed_character.get_all_body_parts( get_body_part_flags::sorted );

    const outfit &character_outfit = displayed_character.worn;

    character_worn.clear();
    character_outfit.visit_items( displayed_character,
    [&]( item_location node ) {
        if( displayed_character.is_worn( *node ) &&
            filter_worn_by == bodypart_str_id::NULL_ID() || node->covers( filter_worn_by ) ) {
            character_worn.push_back( node );
        }
        return VisitResponse::NEXT;
    } );
}

void body_status_ui::setup_input_context( input_context &bs_ictxt )
{
    bs_ictxt.register_action( "NEXT_TAB" );
    bs_ictxt.register_action( "PREV_TAB" );
    bs_ictxt.register_action( "QUIT" );
    bs_ictxt.register_action( "HELP_KEYBINDINGS" );


    bs_ictxt.register_action( "SCROLL_INFOBOX_UP" );
    bs_ictxt.register_action( "SCROLL_INFOBOX_DOWN" );

    bs_ictxt.register_action( "BODYGRAPH_DISPLAY_CHANGE" );
    bs_ictxt.register_action( "BODYGRAPH_TOGGLE_WINDOW" );
    /*
    ctxt.register_action("PAGE_UP");
    ctxt.register_action("PAGE_DOWN");
    ctxt.register_action("MOVE_ARMOR"); // s
    ctxt.register_action("CHANGE_SIDE"); // c
    ctxt.register_action("TOGGLE_CLOTH"); // H
    ctxt.register_action("ASSIGN_INVLETS"); // r
    ctxt.register_action("SORT_ARMOR"); // S
    ctxt.register_action("EQUIP_ARMOR"); // e
    ctxt.register_action("EQUIP_ARMOR_HERE"); // E
    ctxt.register_action("REMOVE_ARMOR"); // u
    ctxt.register_action("TOGGLE_FAVORITE"); // *
    ctxt.register_action("USAGE_HELP"); // F1
    */
}

void body_status_ui::display()
{

    body_status_ui_impl bsui( this );

    bs_uistate.init();

    // setup all uilists/inputs
    body_status_tab preserve_first_tab = bs_uistate.selected_tab;
    bs_uistate.selected_tab = body_status_tab::BS_TAB_SUMMARY;
    for( int i = 0; i < BODY_STATUS_UI_TAB_COUNT; i++ ) {
        setup_new_uilist();
        ++bs_uistate.selected_tab;
    }
    bs_uistate.selected_tab = preserve_first_tab;

    // set first tab
    upon_switching_tab();

    ui_manager::invalidate_all_ui_adaptors();

    while( !bs_uistate.quit_menu ) {
        ui_manager::redraw_invalidated();

        input_context &current_tab_input = get_current_tab_input();
        input_context::scoped_activation active_tab_context( current_tab_input );

        std::shared_ptr<uilist> current_tab_uilist = get_current_tab_uilist();
        if( current_tab_uilist ) {
            bs_uilist_current = current_tab_uilist->create_or_get_ui();
            if( current_tab_uilist->query_setup() ) {
                current_tab_uilist->query_once( current_tab_input, 33 );
                // if list entries need to be reset from a callback
                // SX2: build into interface
                if( reset_list_entries ) {
                    reset_list_entries = false;
                    bs_uilist_current.reset();
                    current_tab_uilist.reset();
                    bs_uilist[static_cast<int>( bs_uistate.selected_tab )].reset();
                    setup_new_uilist();
                }
            }
        } else {
            bs_uilist_current.reset();
            handle_action( current_tab_input.handle_input( 33 ) );
        }
        if( !bs_uistate.button_action.empty() ) {
            handle_action( bs_uistate.button_action );
            bs_uistate.button_action.clear();
        }
        if( reset_window_size ) {
            bsui.mark_resized();
            reset_window_size = false;
        }
    }
}

void body_status_ui::upon_switching_tab()
{
    update_uilist_entries();
}

std::shared_ptr<uilist> body_status_ui::get_current_tab_uilist()
{
    return bs_uilist[static_cast<int>( bs_uistate.selected_tab )];
}

std::shared_ptr<uilist> body_status_ui::get_tab_uilist( body_status_tab tab )
{
    return bs_uilist[static_cast<int>( tab )];
}

void body_status_ui::set_current_tab_uilist( const std::shared_ptr<uilist> &new_uilist )
{
    bs_uilist[static_cast<int>( bs_uistate.selected_tab )] = new_uilist;
}

input_context &body_status_ui::get_current_tab_input()
{
    return bs_inputs[static_cast<int>( bs_uistate.selected_tab )];
}

void body_status_ui::set_current_tab_input( const input_context &new_input )
{
    bs_inputs[static_cast<int>( bs_uistate.selected_tab )] = new_input;
}

void body_status_ui::setup_new_uilist()
{
    std::shared_ptr<uilist> new_uilist = get_current_tab_uilist();

    if( !new_uilist ) {
        input_context new_uilist_input;
        setup_input_context( new_uilist_input );

        set_current_tab_uilist( std::make_shared<uilist>() );
        new_uilist = get_current_tab_uilist();
        new_uilist->allow_additional = true;
        new_uilist->allow_cancel = false;
        new_uilist->callback = &bs_callback;
        //snap to left edge of screen, position and height updated later
        new_uilist->desired_bounds = uilist_reset_desired_bounds();
        new_uilist->force_desired_bounds = true;

        update_uilist_entries();

        new_uilist->register_uilist_inputs( new_uilist_input );
        new_uilist->setup();
        set_current_tab_input( new_uilist_input );
    }

}

bool body_status_ui::handle_action( const std::string &action )
{

    if( action == "QUIT" ) {
        // quit summary subpart list
        if( bs_uistate.selected_tab == body_status_tab::BS_TAB_SUMMARY &&
            bs_uistate.parent_bodypart.has_value() ) {
            bs_uistate.parent_bodypart.reset();
            // get nested graph of original part
            bodygraph_display &bodygraph_display_state = bs_uistate.bodygraph_display_state;
            bodygraph_display_state.id = bodygraph_full_body;
            bodygraph_display_state.prepare_partlist();
            reset_list_entries = true;
        } else {
            bs_uistate.quit_menu = true;
        }
    } else if( action == "PREV_TAB" ) {
        --bs_uistate.selected_tab;
        bs_uistate.switched_tab = bs_uistate.selected_tab;
        upon_switching_tab();
    } else if( action == "NEXT_TAB" ) {
        ++bs_uistate.selected_tab;
        bs_uistate.switched_tab = bs_uistate.selected_tab;
        upon_switching_tab();
    } else if( action == "SCROLL_INFOBOX_UP" ) {
        bs_uistate.scrolled_up = true;
    } else if( action == "SCROLL_INFOBOX_DOWN" ) {
        bs_uistate.scrolled_down = true;
    } else if( action == "BODYGRAPH_TOGGLE_WINDOW" ) {
        bs_uistate.bodygraph_collapsed = !bs_uistate.bodygraph_collapsed;
        reset_window_size = true;
    } else if( action == "BODYGRAPH_DISPLAY_CHANGE" ) {
        ++bs_uistate.bodygraph_display_state.display_mode;
    } else if( action == "APPLY" ) {
        if( bs_uistate.displayed_character.is_avatar() ) {
            avatar_action::use_item( *bs_uistate.displayed_character.as_avatar() );
        } else {
            popup( _( "Applying not implemented for NPCs." ) );
        }
    } else {
        return false;
    }
    return true;
}

void body_status_ui::update_uilist_entries()
{
    std::shared_ptr<uilist> menu = get_current_tab_uilist();

    if( menu ) {
        menu->entries.clear();
    }

    auto display_limbs = [this, &menu]() {
        for( const part_tuple &bgt : bs_uistate.bodygraph_display_state.partlist ) {
            std::string txt = !std::get<1>( bgt ) ?
                              std::get<0>( bgt )->name.translated() :
                              std::get<1>( bgt )->name.translated();
            txt = uppercase_first_letter( txt );
            if( !std::get<3>( bgt ) ) {
                txt = colorize( txt, c_dark_gray );
            }
            txt.append( !std::get<2>( bgt )->nested_graph.is_null() ?
                        colorize( " >", c_light_green ) : " " );
            nc_color clr = std::get<3>( bgt ) ? c_white : c_dark_gray;

            menu->addentry( get_uilist_entry( txt ) );
        }
    };

    switch( bs_uistate.selected_tab ) {
        case body_status_tab::BS_TAB_MEDICAL: {
            display_limbs();
            break;
        }
        case body_status_tab::BS_TAB_SUMMARY: {
            display_limbs();
            break;
        }
        case body_status_tab::BS_TAB_SORT_ARMOR: {
            for( const item_location &loc : bs_uistate.character_worn ) {
                std::string worn_armor_name = loc->tname();
                menu->addentry( get_uilist_entry( worn_armor_name ) );
            }

            break;
        }
    }
}

void body_status_ui::update_uilist_position( ImVec2 new_position )
{
    std::shared_ptr<uilist> current_uilist = get_current_tab_uilist();
    current_uilist->desired_bounds = uilist_reset_desired_bounds();
    current_uilist->desired_bounds->y = new_position.y;
    current_uilist->desired_bounds->h = ImGui::GetContentRegionAvail().y;
    current_uilist->reposition();
    if( bs_uilist_current ) {
        bs_uilist_current->mark_resized();
    }
}

bool body_status_callback::key( const input_context &ctxt, const input_event &event,
                                int, uilist * )
{
    const std::string &action = ctxt.input_to_action( event );
    if( !ui_parent->handle_action( action ) ) {
        return false;
    }
    return true;
}

void body_status_callback::confirm( uilist *menu )
{
    int uilist_returned = menu->ret;

    if( menu->ret == -1 ) {
        uilist_returned = menu->selected;
        if( uilist_returned == -1 ) {
            return;
        }
    }

    body_status_uistate &bs_uistate = ui_parent->bs_uistate;
    bodygraph_display &bodygraph_display_state = bs_uistate.bodygraph_display_state;

    switch( bs_uistate.selected_tab ) {
        case body_status_tab::BS_TAB_SUMMARY: {
            if( !bs_uistate.parent_bodypart.has_value() ) {
                // if in the top-level bodypart list, open the sub-bodypart list
                bs_uistate.parent_bodypart = bs_uistate.selected_bodypart;
                // get nested graph of original part
                bodygraph_display_state.id = std::get<2>(
                                                 bodygraph_display_state.partlist[bodygraph_display_state.sel_part] )->nested_graph;
                bodygraph_display_state.prepare_partlist();
                ui_parent->reset_list_entries = true;
            }
            break;
        }
        case body_status_tab::BS_TAB_MEDICAL: {
            bs_uistate.displayed_character.pick_wound_fix( bs_uistate.selected_bodypart );
            break;
        }
        case body_status_tab::BS_TAB_SORT_ARMOR: {
            // toggle this entry as "moving"
            break;
        }
        default:
            break;
    }
}

void body_status_callback::select( uilist *menu )
{
    body_status_uistate &bs_uistate = ui_parent->bs_uistate;
    bodygraph_display &bodygraph_display_state = bs_uistate.bodygraph_display_state;
    bodygraph_display_state.blink = 0;
    bodygraph_display_state.symbol_blinking = true;
    disp_bodygraph_cache::clear( bodygraph_display_state.id, bodygraph_display_state.display_mode );

    int menu_selected = menu->selected;
    bodypart_id selected_bp = std::get<0>( bodygraph_display_state.partlist[menu_selected] );

    auto update_selected_bodypart = [&bs_uistate, &menu_selected, &selected_bp,
                 &bodygraph_display_state]() {
        bs_uistate.selected_bodypart_index = menu_selected;
        bodygraph_display_state.sel_part = menu_selected;
        bs_uistate.selected_bodypart = selected_bp;
    };

    switch( bs_uistate.selected_tab ) {
        case body_status_tab::BS_TAB_SUMMARY: {
            update_selected_bodypart();
            bodygraph_display_state.prepare_infolist();
            break;
        }
        case body_status_tab::BS_TAB_MEDICAL: {
            update_selected_bodypart();
            break;
        }
        case body_status_tab::BS_TAB_SORT_ARMOR:
            bs_uistate.selected_armor = bs_uistate.character_worn[menu_selected];
            // much more complicated... depends on which list is up
            // needs to be able to move entries and not move entries (not including headers)
            break;
        default:
            break;
    }
    bs_uistate.update_bodygraph_display();
}

void body_status_callback::refresh( uilist *menu )
{
    menu->footer_text = !menu->filter.empty() ? string_format( "Filter: %s",
                        menu->filter ) : std::string();
}

body_status_ui_impl::body_status_ui_impl( body_status_ui *parent ) :
    cataimgui::window( "",
                       ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                       ImGuiWindowFlags_NoNav )
{
    ui_parent = parent;
}

cataimgui::bounds body_status_ui_impl::get_bounds()
{
    const ImVec2 viewport = ImGui::GetMainViewport()->WorkSize;
    return { 0, 0, ui_parent->bs_uistate.bodygraph_collapsed ? viewport.x * 0.66f : viewport.x, viewport.y};
}

void body_status_ui_impl::draw_controls()
{
    body_status_tab &selected_tab = ui_parent->bs_uistate.selected_tab;
    const ImVec2 CHILD_WINDOW_SIZE = { ImGui::GetContentRegionAvail().x *
                                       ( ui_parent->bs_uistate.bodygraph_collapsed ? 0.5f : 0.33f ), ImGui::GetContentRegionAvail().y
                                     };

    {
        ImGui::BeginChild( "WINDOW1", CHILD_WINDOW_SIZE );
        if( ImGui::BeginTabBar( "##Tabs" ) ) {
            if( cataimgui::BeginTabItem( _( "SUMMARY" ), selected_tab == body_status_tab::BS_TAB_SUMMARY ) ) {
                selected_tab = ImGui::IsItemActive() ? body_status_tab::BS_TAB_SUMMARY : selected_tab;
                ImGui::EndTabItem();
            }
            if( cataimgui::BeginTabItem( _( "MEDICAL" ), selected_tab == body_status_tab::BS_TAB_MEDICAL ) ) {
                selected_tab = ImGui::IsItemActive() ? body_status_tab::BS_TAB_MEDICAL : selected_tab;
                ImGui::EndTabItem();
            }
            if( cataimgui::BeginTabItem( _( "SORT ARMOR" ),
                                         selected_tab == body_status_tab::BS_TAB_SORT_ARMOR ) ) {
                selected_tab = ImGui::IsItemActive() ? body_status_tab::BS_TAB_SORT_ARMOR : selected_tab;
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        std::shared_ptr<uilist> current_uilist = ui_parent->get_current_tab_uilist();
        if( current_uilist ) {
            const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
            if( current_uilist->desired_bounds->y != cursor_pos.y ||
                current_uilist->desired_bounds->w != uilist_reset_desired_bounds().w ) {
                ui_parent->update_uilist_position( cursor_pos );
            }
        }

        ImGui::EndChild();
    }

    ImGui::SameLine();
    {
        ImGui::BeginChild( "WINDOW2", CHILD_WINDOW_SIZE );
        draw_description();
        ImGui::EndChild();
    }

    if( !ui_parent->bs_uistate.bodygraph_collapsed ) {
        ImGui::SameLine();
        {
            ImGui::BeginChild( "WINDOW3", CHILD_WINDOW_SIZE );
            draw_bodygraph();
            ImGui::EndChild();
        }
    }


}

void body_status_ui_impl::draw_description()
{
    body_status_uistate &bs_uistate = ui_parent->bs_uistate;

    if( ImGui::Button( _( "Toggle Bodygraph Window" ) ) ) {
        bs_uistate.button_action = "BODYGRAPH_TOGGLE_WINDOW";
    }

    switch( bs_uistate.selected_tab ) {
        case body_status_tab::BS_TAB_SUMMARY:
            bs_uistate.bodygraph_display_state.draw_info_imgui();
            break;
        case body_status_tab::BS_TAB_MEDICAL:
            medical_ui::draw_limbs_details( bs_uistate.displayed_character, bs_uistate.selected_bodypart );
            break;
        case body_status_tab::BS_TAB_SORT_ARMOR: {
            const item &selected_armor = *bs_uistate.selected_armor;
            const Character &displayed_character = bs_uistate.displayed_character;
            const bodypart_id filter_worn_by = bs_uistate.filter_worn_by;

            cataimgui::draw_colored_text( selected_armor.tname() );
            ImGui::Separator();
            cataimgui::draw_colored_text( clothing_info::clothing_layer( selected_armor ) );

            for( const std::string &lines : clothing_info::clothing_flags_description( selected_armor ) ) {
                cataimgui::draw_colored_text( lines );
            }
            ImGui::Separator();
            for( const std::string &lines : clothing_info::clothing_properties(
                     selected_armor, displayed_character, filter_worn_by ) ) {
                cataimgui::draw_colored_text( lines );
            }
            ImGui::Separator();
            for( const std::string &lines : clothing_info::clothing_protection( selected_armor,
                    filter_worn_by ) ) {
                cataimgui::draw_colored_text( lines );
            }
        }
        break;
        default:
            break;
    }
    bs_uistate.set_description_scroll();
}
void body_status_ui_impl::draw_bodygraph()
{
    body_status_uistate &bs_uistate = ui_parent->bs_uistate;
    if( bs_uistate.selected_tab != body_status_tab::BS_TAB_SORT_ARMOR ) {
        medical_ui::summary_tab( bs_uistate.displayed_character );
    } else {

    }
    ImGui::Separator();

    cataimgui::draw_colored_text( "Body Graph:" );
    if( ImGui::Button( string_format( _( "Displaying: %s" ),
                                      io::enum_to_string( bs_uistate.bodygraph_display_state.display_mode ) ).c_str() ) ) {
        bs_uistate.button_action = "BODYGRAPH_DISPLAY_CHANGE";
    }
    ImGui::NewLine();
    bs_uistate.bodygraph_display_state.draw_graph_imgui( bs_uistate.parent_bodypart.has_value() );
}
