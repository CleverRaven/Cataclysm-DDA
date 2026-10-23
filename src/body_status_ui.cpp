#include "body_status_ui.h" // IWYU pragma: associated

#include <functional>
#include <memory>

#include "avatar_action.h"
#include "bodygraph.h"
#include "character.h"
#include "character_attire.h"
#include "color.h"
#include "creature.h"
#include "display.h"
#include "enum_traits.h"
#include "flag.h"
#include "imgui/imgui.h"
#include "item.h"
#include "options.h"
#include "output.h"
#include "string_formatter.h"
#include "translation.h"
#include "translations.h"
#include "ui_manager.h"
#include "uilist.h"
#include "uistate.h"
#include "units.h"
#include "units_utility.h"

static const bodygraph_id bodygraph_full_body( "full_body" );

static const flag_id json_flag_HIDDEN( "HIDDEN" );

static void refresh_header_footer_text( uilist *new_uilist, const body_status_uistate &bs_uistate )
{

    if( bs_uistate.selected_tab == body_status_tab::BS_TAB_SORT_ARMOR ) {
        std::string bodypart_category = bs_uistate.sort_armor_category_index == 0 ?
                                        _( "ALL" ) : bs_uistate.sort_armor_category->name_as_heading.translated();
        bool no_armor_found = bs_uistate.worn_subset.empty();
        std::string header_label = no_armor_found ?
                                   colorize( _( "No armor on this limb!" ), c_yellow ) :
                                   colorize( _( "Innermost" ), c_light_gray );

        new_uilist->text = string_format( "%s\n%s",
                                          colorize( string_format( "<< %s >>", bodypart_category ), c_light_green ), header_label );

        if( no_armor_found ) {
            new_uilist->footer_text = new_uilist->text;
        } else {
            new_uilist->footer_text = colorize( _( "Outermost" ), c_light_gray );
        }
    } else {
        new_uilist->text.clear();
        new_uilist->footer_text.clear();
    }
}

body_status_uistate::body_status_uistate( Character &displayed_character, bodygraph_id bg_id,
        body_status_tab initial_tab ) :
    displayed_character( displayed_character ), initial_tab( initial_tab )
{
    if( bg_id == bodygraph_id::NULL_ID() ) {
        displayed_graph_id = bodygraph_full_body;
    }
}

void body_status_uistate::update_blink()
{
    std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
    if( now > last_blink + std::chrono::milliseconds( get_option<int>( "BLINK_SPEED" ) ) ) {
        last_blink = now;
        symbol_blinking = !symbol_blinking;
        disp_bodygraph_cache::clear_all();
    }
}

std::string body_status_uistate::prepare_bodygraph_text( const std::optional<std::string> &
        bodypart_or_subpart_id ) const
{
    int bg_height = 20;
    return display::colorized_bodygraph_text( displayed_character, displayed_graph_id.str(),
            display_mode, 40, 20, bg_height,
            symbol_blinking, bodypart_or_subpart_id );
}

const bodygraph_part_info &body_status_uistate::get_selected_partlist() const
{
    return partlist[selected_index];
}

std::string bodygraph_part_info::get_part() const
{
    const sub_bodypart_str_id selected_subpart = sbp_id;
    return !sbp_id.is_null() ? selected_subpart->id.str() :
           bp_id.id().str();
}

void body_status_uistate::init()
{
    last_blink = std::chrono::steady_clock::now();
    bodygraph_collapsed = uistate.body_status_bodygraph_collapsed;
    info_collapsed = uistate.body_status_info_collapsed;
    prepare_partlist();
    prepare_infolist();
    all_bodyparts = displayed_character.get_all_body_parts( get_body_part_flags::sorted );
    update_sorted_armor();
}

void body_status_uistate::update_sorted_armor()
{
    sort_armor_categories.clear();
    sort_armor_categories.emplace_back( bodypart_str_id::NULL_ID() );
    for( bodypart_id bp_id : all_bodyparts ) {
        sort_armor_categories.emplace_back( bp_id );
    }
    sort_armor_category = sort_armor_categories[sort_armor_category_index];

    worn_subset = displayed_character.worn.filter_by_bodypart( sort_armor_category );
    displayed_character.calc_encumbrance();
    prepare_bodyparts_compact();
    prepare_worn_items_by_limb();
}

void body_status_uistate::prepare_bodyparts_compact()
{

    all_bodyparts_compact.clear();
    for( const bodypart_id &cover : all_bodyparts ) {
        if( cover == bodypart_str_id::NULL_ID() ) {
            continue;
        }
        if( !displayed_character.worn.items_cover_opposite_bp( displayed_character, cover ) ||
            all_bodyparts_compact.count( cover.obj().opposite_part ) == 0 ) {
            all_bodyparts_compact.insert( cover );
        }
    }
}

void body_status_uistate::prepare_worn_items_by_limb()
{
    const outfit &worn = displayed_character.worn;

    worn_items_by_limb_txt.clear();
    for( const bodypart_id &cover : all_bodyparts_compact ) {
        armor_text_entry header_entry;
        header_entry.header = string_format( "%s:", colorize( body_part_name_as_heading( cover,
                                             worn.items_cover_opposite_bp( displayed_character, cover ) ? 2 : 1 ),
                                             ( cover == sort_armor_category ? c_yellow : c_white ) ) );
        worn_items_by_limb_txt.emplace_back( header_entry );

        for( layering_item_info &elem : worn.items_cover_bp( displayed_character, cover ) ) {
            armor_text_entry item_entry;
            nc_color color = elem.penalties.color_for_stacking_badness();
            char plus = elem.penalties.badness() > 0 ? '+' : ' ';
            std::string encumbrance_string = string_format( "%d%c", elem.encumber, plus );
            item_entry.item_name = colorize( elem.name, color );
            item_entry.encumbrance = colorize( encumbrance_string, c_light_gray );
            item_entry.hidden = colorize( ( elem.hidden ? "H" : "" ), c_light_gray );
            item_entry.storage = colorize( string_format( "%.2f",
                                           convert_volume( units::to_milliliter( elem.storage ) ) ), c_light_gray );
            worn_items_by_limb_txt.emplace_back( item_entry );
        }
    }
}

void body_status_ui::setup_input_context( input_context &bs_ictxt )
{
    bs_ictxt.register_action( "NEXT_TAB" );
    bs_ictxt.register_action( "PREV_TAB" );
    bs_ictxt.register_action( "QUIT" );
    bs_ictxt.register_action( "HELP_KEYBINDINGS" );


    bs_ictxt.register_action( "SCROLL_INFOBOX_UP" );
    bs_ictxt.register_action( "SCROLL_INFOBOX_DOWN" );
    bs_ictxt.register_action( "SCROLL_BODYGRAPH_UP" );
    bs_ictxt.register_action( "SCROLL_BODYGRAPH_DOWN" );

    bs_ictxt.register_action( "BODYGRAPH_DISPLAY_CHANGE" );
    bs_ictxt.register_action( "INFO_TOGGLE_WINDOW" );
    bs_ictxt.register_action( "BODYGRAPH_TOGGLE_WINDOW" );

    if( bs_uistate.selected_tab == body_status_tab::BS_TAB_MEDICAL ) {
        //bs_ictxt.register_action("APPLY", to_translation("Use item"));
    } else if( bs_uistate.selected_tab == body_status_tab::BS_TAB_SORT_ARMOR ) {
        bs_ictxt.register_leftright();
        bs_ictxt.register_action( "MOVE_ARMOR_IN" );
        bs_ictxt.register_action( "MOVE_ARMOR_OUT" );
        bs_ictxt.register_action( "CHANGE_SIDE" );
        bs_ictxt.register_action( "TOGGLE_CLOTH" );
        bs_ictxt.register_action( "ASSIGN_INVLETS" );
        bs_ictxt.register_action( "SORT_ARMOR" );
        //bs_ictxt.register_action( "EQUIP_ARMOR" );
        //bs_ictxt.register_action( "EQUIP_ARMOR_HERE" );
        bs_ictxt.register_action( "REMOVE_ARMOR" );
        bs_ictxt.register_action( "TOGGLE_FAVORITE" );
    }
}

bool body_status_ui::display()
{

    body_status_ui_impl bsui( this );

    bs_uistate.init();

    // setup all uilists/inputs
    body_status_tab preserve_first_tab = bs_uistate.initial_tab;
    bs_uistate.selected_tab = body_status_tab::BS_TAB_SUMMARY;
    for( int i = 0; i < BODY_STATUS_UI_TAB_COUNT; i++ ) {
        setup_new_uilist();
        ++bs_uistate.selected_tab;
    }
    bs_uistate.selected_tab = preserve_first_tab;

    // TODO: move to interface

    // set first tab
    upon_switching_tab();

    ui_manager::invalidate_all_ui_adaptors();

    while( !bs_uistate.quit_menu ) {
        ui_manager::redraw_invalidated();

        bs_uistate.update_blink();
        input_context &current_tab_input = get_current_tab_input();
        input_context::scoped_activation active_tab_context( current_tab_input );

        std::shared_ptr<uilist> current_tab_uilist = get_current_tab_uilist();
        if( current_tab_uilist ) {
            if( current_tab_uilist->query_setup() ) {
                current_tab_uilist->query_once( current_tab_input, 33 );
                // if list entries need to be reset from a callback
                if( reset_list_entries ) {
                    reset_list_entries = false;
                    current_tab_uilist.reset();
                    bs_uilist[static_cast<int>( bs_uistate.selected_tab )].reset();
                    setup_new_uilist();
                }
            }
        } else {
            handle_action( current_tab_input.handle_input( 33 ) );
        }
        if( !bs_uistate.button_action.empty() ) {
            handle_action( bs_uistate.button_action );
            bs_uistate.button_action.clear();
        }
        if( bs_uistate.switched_tab != body_status_tab::BS_TAB_last ) {
            bs_uistate.selected_tab = bs_uistate.switched_tab;
            bs_uistate.switched_tab = body_status_tab::BS_TAB_last;
            upon_switching_tab();
        }
        if( reset_window_size ) {
            bsui.mark_resized();
            reset_window_size = false;
        }
    }

    uistate.body_status_bodygraph_collapsed = bs_uistate.bodygraph_collapsed;
    uistate.body_status_info_collapsed = bs_uistate.info_collapsed;
    return bs_uistate.wound_fixed;
}

void body_status_ui::upon_switching_tab()
{
    update_uilist_entries();
    get_current_tab_uilist()->set_selected( 0 );
    bs_uistate.update_selected( 0 );

    switch( bs_uistate.selected_tab ) {
        case body_status_tab::BS_TAB_SUMMARY: {
            if( !bs_uistate.parent_bodypart.has_value() ) {
                bs_uistate.displayed_graph_id = bodygraph_full_body;
            } else {
                const std::string bpid_str = bs_uistate.parent_bodypart->id().str();
                bodygraph_id nested_bodygraph = bodygraph_id( bpid_str );
                bs_uistate.displayed_graph_id = nested_bodygraph.is_valid() ? nested_bodygraph :
                                                bodygraph_full_body;
                bs_uistate.prepare_partlist();
            }
            break;
        }
        case body_status_tab::BS_TAB_MEDICAL:
        case body_status_tab::BS_TAB_SORT_ARMOR: {
            bs_uistate.displayed_graph_id = bodygraph_full_body;
            break;
        }
        default:
            break;
    }
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
        input_context new_uilist_input( "BODY_STATUS_UI" );
        setup_input_context( new_uilist_input );

        set_current_tab_uilist( std::make_shared<uilist>() );
        new_uilist = get_current_tab_uilist();
        new_uilist->allow_additional = true;
        new_uilist->allow_cancel = false;
        new_uilist->callback = &bs_callback;
        //snap to left edge of screen, position and height updated later
        new_uilist->desired_bounds = uilist_reset_desired_bounds();
        new_uilist->force_desired_bounds = true;
        new_uilist->desc_enabled = false;

        update_uilist_entries();

        if( bs_uistate.selected_tab == body_status_tab::BS_TAB_SORT_ARMOR ) {
            new_uilist->desc_enabled = true;
            refresh_header_footer_text( new_uilist.get(), bs_uistate );
        }

        new_uilist->register_uilist_inputs( new_uilist_input );
        new_uilist->setup();
        set_current_tab_input( new_uilist_input );

        if( bs_uistate.selected_tab == body_status_tab::BS_TAB_SORT_ARMOR ) {
            if( bs_uistate.selected_index_set > -1 &&
                bs_uistate.selected_index_set < static_cast<int>( bs_uistate.worn_subset.size() ) ) {
                new_uilist->scrollby( uilist::scroll_amount{
                    uilist::scroll_amount::absolute, bs_uistate.selected_index_set } );
            } else {
                bs_uistate.selected_index_set = -1;
            }
        }
    }

}

bool body_status_ui::handle_action( const std::string &action )
{
    auto do_return_entry = []() {
        uistate.open_menu = []() {
            body_status_ui bsui( get_player_character(), body_status_tab::BS_TAB_SORT_ARMOR );
            bsui.display();
        };
    };

    auto splice_worn_item = [this]( std::list<item>::iterator selected_item,
                                    std::list<item>::iterator target_item,
    int target_item_index, bool outer ) {
        if( !selected_item->has_flag( flag_INTEGRATED ) && !target_item->has_flag( flag_INTEGRATED ) ) {
            // move this item below the inner item in `worn`
            displayed_character.worn.splice( outer ? ++target_item : target_item, selected_item );
            bs_uistate.update_sorted_armor();
            bs_uistate.selected_index_set = target_item_index;
            reset_list_entries = true;
        } else {
            popup( _( "Can't move your integrated armor!" ) );
        }
    };

    if( action == "QUIT" ) {
        // quit summary subpart list
        if( bs_uistate.selected_tab == body_status_tab::BS_TAB_SUMMARY &&
            bs_uistate.parent_bodypart.has_value() ) {
            bs_uistate.parent_bodypart.reset();
            // get nested graph of original part
            bs_uistate.displayed_graph_id = bodygraph_full_body;
            bs_uistate.prepare_partlist();
            reset_list_entries = true;
        } else {
            bs_uistate.quit_menu = true;
        }
    } else if( action == "PREV_TAB" ) {
        bs_uistate.switched_tab = bs_uistate.selected_tab;
        --bs_uistate.switched_tab;
    } else if( action == "NEXT_TAB" ) {
        bs_uistate.switched_tab = bs_uistate.selected_tab;
        ++bs_uistate.switched_tab;
    } else if( action == "SCROLL_INFOBOX_UP" ) {
        bs_uistate.info_scrolled_up = true;
    } else if( action == "SCROLL_INFOBOX_DOWN" ) {
        bs_uistate.info_scrolled_down = true;
    } else if( action == "SCROLL_BODYGRAPH_UP" ) {
        bs_uistate.bodygraph_scrolled_up = true;
    } else if( action == "SCROLL_BODYGRAPH_DOWN" ) {
        bs_uistate.bodygraph_scrolled_down = true;
    } else if( action == "BODYGRAPH_TOGGLE_WINDOW" ) {
        bs_uistate.bodygraph_collapsed = !bs_uistate.bodygraph_collapsed;
        reset_window_size = true;
    } else if( action == "INFO_TOGGLE_WINDOW" ) {
        bs_uistate.info_collapsed = !bs_uistate.info_collapsed;
        reset_window_size = true;
    } else if( action == "BODYGRAPH_DISPLAY_CHANGE" ) {
        ++bs_uistate.display_mode;
    } else if( action == "APPLY" ) {
        if( bs_uistate.displayed_character.is_avatar() ) {
            avatar_action::use_item( *bs_uistate.displayed_character.as_avatar() );
        } else {
            popup( _( "Applying not implemented for NPCs." ) );
        }
    } else if( action == "LEFT" ) {
        int &sort_armor_category_index = bs_uistate.sort_armor_category_index;
        sort_armor_category_index--;
        if( sort_armor_category_index < 0 ) {
            sort_armor_category_index = bs_uistate.sort_armor_categories.size() - 1;
        }
        bs_uistate.update_sorted_armor();
        update_uilist_entries();
        reset_list_entries = true;
    } else if( action == "RIGHT" ) {
        int &sort_armor_category_index = bs_uistate.sort_armor_category_index;
        sort_armor_category_index++;
        if( sort_armor_category_index == static_cast<int>( bs_uistate.sort_armor_categories.size() ) ) {
            sort_armor_category_index = 0;
        }
        bs_uistate.update_sorted_armor();
        update_uilist_entries();
        reset_list_entries = true;
    } else if( action == "MOVE_ARMOR_IN" ) {
        /*
        this process requires finding the index in `Character::worn` of the
        * selected item in the filtered list and the above item in the filtered list
        * and moving the selected item inwards in `Character::worn`
        */
        int inner_item_index = bs_uistate.selected_index - 1;
        if( inner_item_index >= 0 ) {
            std::list<item>::iterator selected_item = bs_uistate.worn_subset[bs_uistate.selected_index];
            std::list<item>::iterator inner_item = bs_uistate.worn_subset[inner_item_index];
            splice_worn_item( selected_item, inner_item, inner_item_index, false );
        }
    } else if( action == "MOVE_ARMOR_OUT" ) {
        int outer_item_index = bs_uistate.selected_index + 1;
        if( outer_item_index < static_cast<int>( bs_uistate.worn_subset.size() ) ) {
            std::list<item>::iterator selected_item = bs_uistate.worn_subset[bs_uistate.selected_index];
            std::list<item>::iterator outer_item = bs_uistate.worn_subset[outer_item_index];
            splice_worn_item( selected_item, outer_item, outer_item_index, true );
        }
    } else if( action == "CHANGE_SIDE" ) {
        item_location &changed_armor = bs_uistate.selected_armor;
        if( changed_armor.valid() ) {
            if( !changed_armor->is_sided() ) {
                popup( _( "This armor isn't one-sided!" ) );
            } else if( displayed_character.query_yn( _( "Swap side for %s?" ),
                       changed_armor->tname() ) ) {
                displayed_character.change_side( *changed_armor );
                bs_uistate.update_sorted_armor();
            }
        }
    } else if( action == "TOGGLE_CLOTH" ) {
        if( bs_uistate.selected_armor.valid() ) {
            if( !bs_uistate.selected_armor->has_flag( json_flag_HIDDEN ) ) {
                bs_uistate.selected_armor->set_flag( json_flag_HIDDEN );
            } else {
                bs_uistate.selected_armor->unset_flag( json_flag_HIDDEN );
            }
            bs_uistate.update_sorted_armor();
        }
    } else if( action == "SORT_ARMOR" ) {
        displayed_character.worn.sort();
        bs_uistate.update_sorted_armor();
        reset_list_entries = true;
    }
    /*
    else if( action == "EQUIP_ARMOR" ) {

    } else if( action == "EQUIP_ARMOR_HERE" ) {

    }
    */
    else if( action == "REMOVE_ARMOR" ) {
        // query (for now)
        if( bs_uistate.selected_armor.valid() ) {
            if( displayed_character.query_yn( _( "Remove selected armor?" ) ) ) {
                do_return_entry();

                // remove the item, asking to drop it if necessary
                displayed_character.takeoff( bs_uistate.selected_armor );
                bs_uistate.update_sorted_armor();
                bs_uistate.quit_menu = true;
            }
        }
    } else if( action == "TOGGLE_FAVORITE" ) {
        if( bs_uistate.selected_armor.valid() ) {
            item &item_to_toggle = *bs_uistate.selected_armor;
            item_to_toggle.set_favorite( !item_to_toggle.is_favorite );
            bs_uistate.update_sorted_armor();
            reset_list_entries = true;
        }
    } else if( action == "ASSIGN_INVLETS" ) {
        displayed_character.worn.assign_invlets( displayed_character );
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

    switch( bs_uistate.selected_tab ) {
        case body_status_tab::BS_TAB_MEDICAL: {
            bs_uistate.all_bodyparts =
                displayed_character.get_all_body_parts( get_body_part_flags::sorted );
            for( bodypart_id bp_id : bs_uistate.all_bodyparts ) {
                uilist_entry new_entry = get_uilist_entry(
                                             medical_ui::get_modified_limb_name( displayed_character, bp_id ) );
                new_entry.text_color = c_white;
                menu->addentry( new_entry );
            }
            break;
        }
        case body_status_tab::BS_TAB_SUMMARY: {
            for( const bodygraph_part_info &bgt : bs_uistate.partlist ) {
                std::string txt = bgt.sbp_id.is_null() ?
                                  bgt.bp_id->name.translated() :
                                  bgt.sbp_id->name.translated();
                txt = uppercase_first_letter( txt );
                bool present = bgt.present_on_character;
                if( !present ) {
                    txt = colorize( txt, c_dark_gray );
                }
                txt.append( !bgt.bg_part->nested_graph.is_null() ?
                            colorize( " >", c_light_green ) : " " );
                uilist_entry new_entry = get_uilist_entry( txt );
                new_entry.text_color = present ? c_white : c_dark_gray;
                menu->addentry( new_entry );
            }
            break;
        }
        case body_status_tab::BS_TAB_SORT_ARMOR: {
            int armor_count = 0;
            for( const std::list<item>::iterator &it : bs_uistate.worn_subset ) {
                std::string worn_armor_name = it->tname();
                uilist_entry new_entry = get_uilist_entry( worn_armor_name );
                new_entry.retval = armor_count;

                menu->addentry( new_entry );
                armor_count++;
            }
            if( armor_count == 0 ) {
                menu->addentry( get_uilist_entry( _( "No armor found!" ) ) );
            }

            break;
        }
        default:
            break;
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

    select( menu );
    switch( bs_uistate.selected_tab ) {
        case body_status_tab::BS_TAB_SUMMARY: {
            if( !bs_uistate.parent_bodypart.has_value() ) {
                // if in the top-level bodypart list, open the sub-bodypart list
                bs_uistate.parent_bodypart = bs_uistate.selected_bodypart;
                // get nested graph of original part
                bs_uistate.displayed_graph_id = bs_uistate.get_selected_partlist().bg_part->nested_graph;
                bs_uistate.prepare_partlist();
                ui_parent->reset_list_entries = true;
            }
            break;
        }
        case body_status_tab::BS_TAB_MEDICAL: {
            if( bs_uistate.displayed_character.pick_wound_fix( bs_uistate.selected_bodypart ) ) {
                bs_uistate.wound_fixed = true;
                bs_uistate.quit_menu = true;
            }
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
    bs_uistate.last_blink = std::chrono::steady_clock::now();
    bs_uistate.symbol_blinking = true;
    disp_bodygraph_cache::clear( bs_uistate.displayed_graph_id, bs_uistate.display_mode );

    int menu_selected = menu->selected;
    bs_uistate.update_selected( menu_selected );
}

void body_status_uistate::update_selected( int index )
{
    selected_index = index;
    switch( selected_tab ) {
        case body_status_tab::BS_TAB_SUMMARY: {
            bodypart_id selected_bp = get_selected_partlist().bp_id;
            selected_bodypart = selected_bp;
            prepare_infolist();
            break;
        }
        case body_status_tab::BS_TAB_MEDICAL: {
            selected_bodypart = all_bodyparts[index];
            break;
        }
        case body_status_tab::BS_TAB_SORT_ARMOR:
            selected_armor = index > -1 && !worn_subset.empty() ?
                             item_location( displayed_character, &*worn_subset[index] )
                             : item_location::nowhere;
            break;
        default:
            break;
    }
}

void body_status_callback::refresh( uilist *menu )
{
    refresh_header_footer_text( menu, ui_parent->bs_uistate );
}

body_status_ui_impl::body_status_ui_impl( body_status_ui *parent ) :
    cataimgui::window( "", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                       ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav )
{
    ui_parent = parent;
}

cataimgui::bounds body_status_ui_impl::get_bounds()
{
    const body_status_uistate &bs_uistate = ui_parent->bs_uistate;
    const ImVec2 viewport = ImGui::GetMainViewport()->WorkSize;
    const float WINDOW_THIRD = viewport.x * 0.333f;
    float displayed_windows = 1 + static_cast<int>( !bs_uistate.bodygraph_collapsed ) +
                              static_cast<int>( !bs_uistate.info_collapsed );
    return { 0, 0, displayed_windows == 3 ? viewport.x : displayed_windows * WINDOW_THIRD, viewport.y};
}

void body_status_ui_impl::draw_controls()
{
    body_status_uistate &bs_uistate = ui_parent->bs_uistate;
    const ImVec2 viewport = ImGui::GetMainViewport()->WorkSize;
    const float WINDOW_THIRD = viewport.x * 0.333f;

    const ImVec2 CHILD_WINDOW_SIZE = { WINDOW_THIRD, ImGui::GetContentRegionAvail().y };

    {
        ImGui::BeginChild( "WINDOW_UILIST", CHILD_WINDOW_SIZE, 0, window_flags );
        cataimgui::draw_colored_text( string_format( _( "Status of: %s" ),
                                      bs_uistate.displayed_character.name ) );
        if( ImGui::Button( _( "Toggle Info Window" ) ) ) {
            bs_uistate.button_action = "INFO_TOGGLE_WINDOW";
        }
        ImGui::SameLine();
        if( ImGui::Button( _( "Toggle Bodygraph Window" ) ) ) {
            bs_uistate.button_action = "BODYGRAPH_TOGGLE_WINDOW";
        }

        auto check_new_tab = [&bs_uistate]( body_status_tab new_tab ) {
            if( ImGui::IsItemActive() ) {
                bs_uistate.switched_tab = new_tab;
            }
        };

        const body_status_tab selected_tab = bs_uistate.selected_tab;
        if( ImGui::BeginTabBar( "##Tabs" ) ) {
            if( cataimgui::BeginTabItem( _( "SUMMARY" ), selected_tab == body_status_tab::BS_TAB_SUMMARY ) ) {
                check_new_tab( body_status_tab::BS_TAB_SUMMARY );
                ImGui::EndTabItem();
            }
            if( cataimgui::BeginTabItem( _( "MEDICAL" ), selected_tab == body_status_tab::BS_TAB_MEDICAL ) ) {
                check_new_tab( body_status_tab::BS_TAB_MEDICAL );
                ImGui::EndTabItem();
            }
            if( cataimgui::BeginTabItem( _( "SORT ARMOR" ),
                                         selected_tab == body_status_tab::BS_TAB_SORT_ARMOR ) ) {
                check_new_tab( body_status_tab::BS_TAB_SORT_ARMOR );
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        std::shared_ptr<uilist> current_uilist = ui_parent->get_current_tab_uilist();
        if( current_uilist ) {
            uilist::draw_controls( *current_uilist );
        }

        ImGui::EndChild();
    }

    if( !bs_uistate.info_collapsed ) {
        ImGui::SameLine();
        {
            ImGui::BeginChild( "WINDOW_INFO", CHILD_WINDOW_SIZE, 0, window_flags );
            draw_info();
            ImGui::EndChild();
        }
    }

    if( !bs_uistate.bodygraph_collapsed ) {
        ImGui::SameLine();
        {
            ImGui::BeginChild( "WINDOW_BODYGRAPH", CHILD_WINDOW_SIZE, 0, window_flags );
            draw_bodygraph();
            ImGui::EndChild();
        }
    }


}

void body_status_ui_impl::draw_info()
{
    body_status_uistate &bs_uistate = ui_parent->bs_uistate;

    switch( bs_uistate.selected_tab ) {
        case body_status_tab::BS_TAB_SUMMARY:
            if( bs_uistate.get_selected_partlist().present_on_character ) {
                draw_info_imgui( bs_uistate.info_txt );
            }
            break;
        case body_status_tab::BS_TAB_MEDICAL:
            medical_ui::draw_limbs_details( bs_uistate.displayed_character, bs_uistate.selected_bodypart );
            break;
        case body_status_tab::BS_TAB_SORT_ARMOR: {

            if( bs_uistate.selected_armor ) {
                const item &selected_armor = *bs_uistate.selected_armor;
                const bodypart_id sort_armor_category = bs_uistate.sort_armor_category;

                cataimgui::draw_colored_text( selected_armor.tname() );
                ImGui::Separator();
                cataimgui::draw_colored_text( clothing_info::clothing_layer( selected_armor ) );

                for( const std::string &lines : clothing_info::clothing_flags_description( selected_armor ) ) {
                    cataimgui::draw_colored_text( lines );
                }
                ImGui::Separator();
                for( const std::string &lines : clothing_info::clothing_properties(
                         selected_armor, bs_uistate.displayed_character, sort_armor_category ) ) {
                    cataimgui::draw_colored_text( lines );
                }
                ImGui::Separator();
                for( const std::string &lines : clothing_info::clothing_protection( selected_armor,
                        sort_armor_category ) ) {
                    cataimgui::draw_colored_text( lines );
                }
            } else {
                cataimgui::draw_colored_text( colorize( _( "Select a worn item to view its properties." ),
                                                        c_yellow ) );
            }
            break;
        }
        default:
            break;
    }
    tabbed_listinfo_ui::set_info_scroll( bs_uistate.info_scrolled_down, bs_uistate.info_scrolled_up );
}


void body_status_ui_impl::draw_worn_items_by_limb()
{
    const body_status_uistate &bs_uistate = ui_parent->bs_uistate;

    if( ImGui::BeginTable( "##Limbs", 4, ImGuiTableFlags_SizingStretchProp ) ) {
        ImGui::TableSetupColumn( _( "Armor by Limb" ) );
        ImGui::TableSetupColumn( _( "Enc." ) );
        ImGui::TableSetupColumn( _( "Vol." ) );
        ImGui::TableSetupColumn( _( "Hid." ) );
        ImGui::TableHeadersRow();
        for( const armor_text_entry &entry : bs_uistate.worn_items_by_limb_txt ) {
            ImGui::TableNextColumn();
            if( entry.header.has_value() ) {
                cataimgui::draw_colored_text( *entry.header );
            } else {
                cataimgui::draw_colored_text( entry.item_name );
                ImGui::TableNextColumn();
                if( !entry.encumbrance.empty() ) {
                    cataimgui::draw_colored_text( entry.encumbrance );
                }
                ImGui::TableNextColumn();
                if( !entry.storage.empty() ) {
                    cataimgui::draw_colored_text( entry.storage );
                }
                ImGui::TableNextColumn();
                if( !entry.hidden.empty() ) {
                    cataimgui::draw_colored_text( entry.hidden );
                }
            }
            ImGui::TableNextRow();
        }
        ImGui::EndTable();
    }
}

void body_status_ui_impl::draw_bodygraph()
{
    body_status_uistate &bs_uistate = ui_parent->bs_uistate;
    if( bs_uistate.selected_tab == body_status_tab::BS_TAB_SORT_ARMOR ) {
        draw_worn_items_by_limb();
    } else {
        medical_ui::summary_tab( bs_uistate.displayed_character );
    }
    ImGui::Separator();

    cataimgui::draw_colored_text( _( "Body Graph:" ) );
    if( ImGui::Button( string_format( _( "Displaying: %s" ),
                                      io::enum_to_string( bs_uistate.display_mode ) ).c_str() ) ) {
        bs_uistate.button_action = "BODYGRAPH_DISPLAY_CHANGE";
    }
    ImGui::NewLine();

    std::optional<std::string> bodypart_or_subpart_id;

    switch( bs_uistate.selected_tab ) {
        case body_status_tab::BS_TAB_SUMMARY:
            bodypart_or_subpart_id = bs_uistate.parent_bodypart.has_value() ?
                                     bs_uistate.get_selected_partlist().sbp_id.str() :
                                     bs_uistate.selected_bodypart->id.str();
            break;
        case body_status_tab::BS_TAB_MEDICAL:
            bodypart_or_subpart_id = bs_uistate.selected_bodypart.id().str();
            break;
        case body_status_tab::BS_TAB_SORT_ARMOR:
            // if a specific limb is selected
            bodypart_or_subpart_id = bs_uistate.sort_armor_category.id().str();
            break;
        default:
            break;
    }

    if( bodypart_or_subpart_id.has_value() ) {
        draw_graph_imgui( bodypart_or_subpart_id );
    }
    tabbed_listinfo_ui::set_info_scroll( bs_uistate.bodygraph_scrolled_down,
                                         bs_uistate.bodygraph_scrolled_up );
}
