#include "npctalk_rules.h"

#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "auto_pickup.h"
#include "cata_imgui.h"
#include "character.h"
#include "character_id.h"
#include "color.h"
#include "debug.h"
#include "game.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "input_context.h"
#include "input_enums.h"
#include "npc.h"
#include "pimpl.h"
#include "string_formatter.h"
#include "translation.h"
#include "ui_manager.h"

static std::map<cbm_recharge_rule, std::string> recharge_map = {
    {cbm_recharge_rule::CBM_RECHARGE_ALL, "<ally_rule_cbm_recharge_all_text>" },
    {cbm_recharge_rule::CBM_RECHARGE_MOST, "<ally_rule_cbm_recharge_most_text>" },
    {cbm_recharge_rule::CBM_RECHARGE_SOME, "<ally_rule_cbm_recharge_some_text>" },
    {cbm_recharge_rule::CBM_RECHARGE_LITTLE, "<ally_rule_cbm_recharge_little_text>" },
    {cbm_recharge_rule::CBM_RECHARGE_NONE, "<ally_rule_cbm_recharge_none_text>" },
};

static std::map<cbm_reserve_rule, std::string> reserve_map = {
    {cbm_reserve_rule::CBM_RESERVE_ALL, "<ally_rule_cbm_reserve_all_text>" },
    {cbm_reserve_rule::CBM_RESERVE_MOST, "<ally_rule_cbm_reserve_most_text>" },
    {cbm_reserve_rule::CBM_RESERVE_SOME, "<ally_rule_cbm_reserve_some_text>" },
    {cbm_reserve_rule::CBM_RESERVE_LITTLE, "<ally_rule_cbm_reserve_little_text>" },
    {cbm_reserve_rule::CBM_RESERVE_NONE, "<ally_rule_cbm_reserve_none_text>" },
};

static std::map<combat_engagement, std::string> engagement_rules = {
    {combat_engagement::NONE, "<ally_rule_engagement_none>" },
    {combat_engagement::CLOSE, "<ally_rule_engagement_close>" },
    {combat_engagement::WEAK, "<ally_rule_engagement_weak>" },
    {combat_engagement::HIT, "<ally_rule_engagement_hit>" },
    {combat_engagement::ALL, "<ally_rule_engagement_all>" },
    {combat_engagement::FREE_FIRE, "<ally_rule_engagement_free_fire>" },
    {combat_engagement::NO_MOVE, "<ally_rule_engagement_no_move>" },
};

static std::map<aim_rule, std::string> aim_rule_map = {
    {aim_rule::WHEN_CONVENIENT, "<ally_rule_aim_when_convenient>" },
    {aim_rule::SPRAY, "<ally_rule_aim_spray>" },
    {aim_rule::PRECISE, "<ally_rule_aim_precise>" },
    {aim_rule::STRICTLY_PRECISE, "<ally_rule_aim_strictly_precise>" },
};

void follower_rules_ui::draw_follower_rules_ui( npc *guy )
{
    input_context ctxt;
    follower_rules_ui_impl p_impl;
    p_impl.set_npc_pointer_to( guy );
    p_impl.input_ptr = &ctxt;

    ctxt.register_navigate_ui_list();
    ctxt.register_leftright();
    ctxt.register_action( "MOUSE_MOVE" );
    ctxt.register_action( "CONFIRM", to_translation( "Set, toggle, or reset selected rule" ) );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "ANY_INPUT" );
    // This is still bizarrely necessary for imgui
    ctxt.set_timeout( 10 );

    while( true ) {
        ui_manager::redraw_invalidated();


        p_impl.last_action = ctxt.handle_input();

        if( p_impl.last_action == "QUIT" || !p_impl.get_is_open() ) {
            break;
        }
    }
}

template<typename T>
void follower_rules_ui_impl::multi_rule_header( const std::string &id, T &rule,
        const std::map<T, std::string> &rule_map, bool should_advance )
{
    if( ImGui::InvisibleButton( id.c_str(), ImVec2( 1.0, 1.0 ) ) || should_advance ) {
        if( rule_map.upper_bound( rule ) == rule_map.end() ) {
            // Then we have the *last* entry the map, so wrap to the first element
            rule = rule_map.begin()->first;
        } else {
            // Assign the next possible value contained in the map.
            rule = rule_map.upper_bound( rule )->first;
        }
        int offset = distance( rule_map.begin(), rule_map.find( rule ) );
        ImGui::SetKeyboardFocusHere( offset );
    }
}

bool follower_rules_ui_impl::setup_button( int &button_num, std::string &label, bool should_color )
{
    bool was_clicked = false;
    ImGui::PushID( button_num );
    button_num++;
    if( should_color ) {
        ImGui::PushStyleColor( ImGuiCol_Button, c_green );
    }
    if( ImGui::Button( label.c_str() ) ) {
        was_clicked = true;
    }
    ImGui::PopID();
    if( should_color ) {
        ImGui::PopStyleColor();
    }
    return was_clicked;
}

bool follower_rules_ui_impl::setup_table_button( int &button_num, std::string &label,
        bool should_color )
{
    ImGui::TableNextColumn();
    return setup_button( button_num, label, should_color );
}

void follower_rules_ui_impl::set_npc_pointer_to( npc *new_guy )
{
    guy = new_guy;
}

std::string follower_rules_ui_impl::get_parsed( std::string initial_string )
{
    std::string string_reference = std::move( initial_string );
    parse_tags( string_reference, get_player_character(), *guy );
    return string_reference;

}

void follower_rules_ui_impl::print_hotkey( input_event &hotkey )
{
    // Padding spaces intentional, so it's obvious that the fake "Hotkey:" header refers to these.
    // TODO: Just reimplement everything as a table...? Would avoid this sort of thing.
    // But surely not *everything* needs to be a table...
    cataimgui::draw_colored_text( string_format( "  %s  ",
                                  static_cast<char>( hotkey.sequence.front() ) ),
                                  c_green );
    ImGui::SameLine();
}

void follower_rules_ui_impl::rules_transfer_popup( bool &exporting_rules, bool &still_in_popup )
{
    std::string toggle_label;
    if( exporting_rules ) {
        cataimgui::draw_colored_text( string_format( _( "Exporting rules from <color_white>%s</color>" ),
                                      guy->disp_name() ), c_blue );
        //~Label for a button used to copy NPC follower rules to another NPC
        toggle_label = _( "Export" );
    } else {
        cataimgui::draw_colored_text( string_format( _( "Importing rules to <color_white>%s</color>" ),
                                      guy->disp_name() ), c_yellow );
        //~Label for a button used to copy NPC follower rules from another NPC
        toggle_label = _( "Import" );
    }
    // Not ideal but leaves plenty of room for even verbose languages. Really anywhere top-right-ish is fine
    ImGui::SameLine( ImGui::GetWindowWidth() * 0.7 );
    if( ImGui::Button( _( "Return to rules settings" ) ) ) {
        still_in_popup = false;
        return;
    }
    if( g->get_follower_list().size() < 2 ) {
        cataimgui::draw_colored_text( string_format(
                                          _( "Functions unavailable.  %s is your only follower." ),
                                          guy->disp_name() ), c_red );
        return;
    }
    if( ImGui::Button( _( "Toggle export/import" ) ) ) {
        exporting_rules = !exporting_rules;
    }
    ImGui::NewLine();
    cataimgui::draw_colored_text( string_format(
                                      _( "Individual settings are colored if they already match to %s." ),
                                      guy->disp_name() ) );
    if( ImGui::BeginTable( "##SETTINGS_SWAP_TABLE", 8, ImGuiTableFlags_None,
                           ImVec2( window_width, window_height ) ) ) {
        ImGui::TableSetupColumn( _( "Name" ), ImGuiTableColumnFlags_WidthStretch,
                                 static_cast<float>( window_width / 8.0f ) );
        ImGui::TableSetupColumn( _( "Behaviors" ), ImGuiTableColumnFlags_WidthStretch,
                                 static_cast<float>( window_width / 8.0f ) );
        ImGui::TableSetupColumn( _( "Aiming" ), ImGuiTableColumnFlags_WidthStretch,
                                 static_cast<float>( window_width / 8.0f ) );
        ImGui::TableSetupColumn( _( "Engagement" ), ImGuiTableColumnFlags_WidthStretch,
                                 static_cast<float>( window_width / 8.0f ) );
        ImGui::TableSetupColumn( _( "CBM recharge" ), ImGuiTableColumnFlags_WidthStretch,
                                 static_cast<float>( window_width / 8.0f ) );
        ImGui::TableSetupColumn( _( "CBM reserve" ), ImGuiTableColumnFlags_WidthStretch,
                                 static_cast<float>( window_width / 8.0f ) );
        ImGui::TableSetupColumn( _( "Pickup list" ), ImGuiTableColumnFlags_WidthStretch,
                                 static_cast<float>( window_width / 8.0f ) );
        ImGui::TableSetupColumn( _( "ALL" ), ImGuiTableColumnFlags_WidthStretch,
                                 static_cast<float>( window_width / 8.0f ) );
        ImGui::TableHeadersRow();
        int button_number = 0;
        for( npc &role_model : g->all_npcs() ) {
            if( &role_model == guy ) {
                continue; // No copying settings to/from yourself
            }
            if( role_model.is_player_ally() ) {
                bool do_flags_overrides = false;
                bool do_aim = false;
                bool do_engagement = false;
                bool do_cbm_recharge = false;
                bool do_cbm_reserve = false;
                bool do_pickup = false;
                bool do_all = false;
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                cataimgui::draw_colored_text( role_model.disp_name() );
                /*Flags and overrides*/
                do_flags_overrides = setup_table_button( button_number, toggle_label,
                                     role_model.rules.flags == guy->rules.flags );
                /*Aim rule*/
                do_aim = setup_table_button( button_number, toggle_label,
                                             role_model.rules.aim == guy->rules.aim );
                /*Engagement rule*/
                do_engagement = setup_table_button( button_number, toggle_label,
                                                    role_model.rules.engagement == guy->rules.engagement );
                /*CBM recharge rule*/
                do_cbm_recharge = setup_table_button( button_number, toggle_label,
                                                      role_model.rules.cbm_recharge == guy->rules.cbm_recharge );
                /*CBM reserve rule*/
                do_cbm_reserve = setup_table_button( button_number, toggle_label,
                                                     role_model.rules.cbm_reserve == guy->rules.cbm_reserve );
                /*Pickup whitelist rule*/

                // This is not correct if both have rules and both have *different* rules, but comparison is expensive
                // if( role_model.rules.pickup_whitelist->empty() == guy->rules.pickup_whitelist->empty() )
                do_pickup = setup_table_button( button_number, toggle_label,
                                                role_model.rules.pickup_whitelist->empty() == guy->rules.pickup_whitelist->empty() );
                /*The ALL button*/
                do_all = setup_table_button( button_number, toggle_label,
                                             role_model.rules.flags == guy->rules.flags &&
                                             role_model.rules.aim == guy->rules.aim &&
                                             role_model.rules.engagement == guy->rules.engagement &&
                                             role_model.rules.cbm_recharge == guy->rules.cbm_recharge &&
                                             role_model.rules.cbm_reserve == guy->rules.cbm_reserve &&
                                             role_model.rules.pickup_whitelist->empty() == guy->rules.pickup_whitelist->empty() );
                /*The great swap*/
                // To make this slightly easier...
                npc *guy_with_same = exporting_rules ? guy : &role_model;
                npc *guy_with_new = exporting_rules ? &role_model : guy;
                if( do_flags_overrides || do_all ) {
                    guy_with_new->rules.flags = guy_with_same->rules.flags;
                    guy_with_new->rules.overrides = guy_with_same->rules.overrides;
                    guy_with_new->rules.override_enable = guy_with_same->rules.override_enable;
                }
                if( do_aim || do_all ) {
                    guy_with_new->rules.aim = guy_with_same->rules.aim;
                }
                if( do_engagement || do_all ) {
                    guy_with_new->rules.engagement = guy_with_same->rules.engagement;
                }
                if( do_cbm_recharge || do_all ) {
                    guy_with_new->rules.cbm_recharge = guy_with_same->rules.cbm_recharge;
                }
                if( do_cbm_reserve || do_all ) {
                    guy_with_new->rules.cbm_reserve = guy_with_same->rules.cbm_reserve;
                }
                if( do_pickup || do_all ) {
                    guy_with_new->rules.pickup_whitelist = guy_with_same->rules.pickup_whitelist;
                }
            }
        }
        ImGui::EndTable();
    }
}

template<typename T>
void follower_rules_ui_impl::checkbox( int rule_number, const T &this_rule,
                                       input_event &assigned_hotkey, const input_event &pressed_key )
{
    ImGui::PushID( rule_number );
    print_hotkey( assigned_hotkey );
    bool rule_enabled = guy->rules.has_flag( this_rule.rule );
    std::string rules_text;
    if( rule_enabled ) {
        rules_text = this_rule.rule_true_text;
    } else {
        rules_text = this_rule.rule_false_text;
    }
    rules_text = string_format( "%s###CHECK", get_parsed( rules_text ) );
    bool clicked = ImGui::Checkbox( rules_text.c_str(), &rule_enabled );
    bool typed = pressed_key == assigned_hotkey;
    bool changed = typed || clicked;
    if( typed ) {
        ImGui::SetKeyboardFocusHere( -1 );
        rule_enabled = !rule_enabled;
    }
    if( changed ) {
        guy->rules.toggle_flag( this_rule.rule );
        guy->rules.toggle_specific_override_state( this_rule.rule, rule_enabled );
    }
    ImGui::SameLine();
    auto label = _( "Default" );
    auto x = ImGui::GetWindowWidth() - ImGui::GetStyle().ScrollbarSize -
             ImGui::GetStyle().WindowPadding.x - ImGui::CalcTextSize( label, nullptr, true ).x;
    ImGui::SetCursorPosX( x );
    if( ImGui::Button( label ) ) {
        guy->rules.clear_flag( this_rule.rule );
        guy->rules.clear_override( this_rule.rule );
    }
    ImGui::PopID();
}

template<typename T>
void follower_rules_ui_impl::radio_group( const std::string header_id, const char *title, T *rule,
        std::map<T, std::string> &values, input_event &assigned_hotkey, const input_event &pressed_key )
{
    ImGui::Separator();
    ImGui::NewLine();
    multi_rule_header( header_id, * &*rule, values,
                       pressed_key == assigned_hotkey );
    print_hotkey( assigned_hotkey );
    ImGui::SameLine();
    float x = ImGui::GetCursorPosX();
    cataimgui::draw_colored_text( title, c_white );
    for( const std::pair<const T, std::string> &value : values ) {
        std::string rule_text = get_parsed( value.second );
        ImGui::SetCursorPosX( x );
        if( ImGui::RadioButton( rule_text.c_str(), *rule == value.first ) ) {
            *rule = value.first;
        }
    }
}

void follower_rules_ui_impl::draw_controls()
{
    if( !guy ) {
        debugmsg( "Something has gone very wrong, can't find NPC to set rules for.  Aborting." );
        last_action = "QUIT";
        return;
    }

    // ImGui only captures arrow keys by default, we want to also capture the numpad and hjkl
    if( last_action == "QUIT" ) {
        return;
    } else if( last_action == "UP" ) {
        ImGui::NavMoveRequestSubmit( ImGuiDir_Up, ImGuiDir_Up, ImGuiNavMoveFlags_None,
                                     ImGuiScrollFlags_None );
    } else if( last_action == "DOWN" ) {
        ImGui::NavMoveRequestSubmit( ImGuiDir_Down, ImGuiDir_Down, ImGuiNavMoveFlags_None,
                                     ImGuiScrollFlags_None );
    } else if( last_action == "LEFT" ) {
        ImGui::NavMoveRequestSubmit( ImGuiDir_Left, ImGuiDir_Left, ImGuiNavMoveFlags_None,
                                     ImGuiScrollFlags_None );
    } else if( last_action == "RIGHT" ) {
        ImGui::NavMoveRequestSubmit( ImGuiDir_Right, ImGuiDir_Right, ImGuiNavMoveFlags_None,
                                     ImGuiScrollFlags_None );
    } else if( last_action == "CONFIRM" ) {
        ImGui::ActivateItemByID( ImGui::GetFocusID() );
    }

    ImGui::SetWindowSize( ImVec2( window_width, window_height ), ImGuiCond_Once );

    static bool exporting_rules = false;
    static bool in_popup = false;
    if( in_popup ) {
        rules_transfer_popup( exporting_rules, in_popup );
        return;
    }

    ImGui::InvisibleButton( "TOP_OF_WINDOW_KB_SCROLL_SELECTABLE", ImVec2( 1.0, 1.0 ) );

    const hotkey_queue &hotkeys = hotkey_queue::alphabets();
    input_event assigned_hotkey = input_ptr->first_unassigned_hotkey( hotkeys );
    input_event pressed_key = input_ptr->get_raw_input();

    cataimgui::draw_colored_text( string_format( _( "Rules for your follower, %s" ),
                                  guy->disp_name() ) );
    ImGui::Separator();

    if( ImGui::Button( _( "Import settings from follower" ) ) ) {
        exporting_rules = false;
        in_popup = true;
        return;
    }
    if( ImGui::Button( _( "Export settings to follower" ) ) ) {
        exporting_rules = true;
        in_popup = true;
        return;
    }

    cataimgui::draw_colored_text( _( "Hotkey:" ) );
    ImGui::NewLine();


    print_hotkey( assigned_hotkey );
    if( ImGui::Button( _( "Default ALL" ) ) || pressed_key == assigned_hotkey ) {
        ImGui::SetKeyboardFocusHere( -1 );
        // TODO: use query_yn here as a safeguard against fatfingering. Can't use it right now,
        // it destructs the imgui instance(?!) which obviously causes us to crash
        npc_follower_rules default_values; //Call the constructor and let *it* tell us what the default is
        guy->rules = default_values;
    }
    ImGui::NewLine();

    int rule_number = 0;
    /* Handle all of our regular, boolean rules */
    for( const std::pair<const std::string, ally_rule_data> &rule_data : ally_rule_strs ) {
        assigned_hotkey = input_ptr->next_unassigned_hotkey( hotkeys, assigned_hotkey );
        checkbox( rule_number, rule_data.second, assigned_hotkey, pressed_key );
        rule_number += 1;
    }

    // Engagement rules require their own set of buttons, each instruction is unique
    assigned_hotkey = input_ptr->next_unassigned_hotkey( hotkeys, assigned_hotkey );
    radio_group( "ENGAGEMENT_RULES", _( "Engagement rules:" ), &guy->rules.engagement, engagement_rules,
                 assigned_hotkey, pressed_key );

    // Aiming rule also has a non-boolean set of values
    assigned_hotkey = input_ptr->next_unassigned_hotkey( hotkeys, assigned_hotkey );
    radio_group( "AIMING_RULES", _( "Aiming rules:" ), &guy->rules.aim, aim_rule_map, assigned_hotkey,
                 pressed_key );

    /* Shows CBM rules, but only if the character has bionics. Must be last because it will
    only appear sometimes and we don't want hotkeys to be different depending on whether
    the character has bionics. That's bad for muscle memory! */
    if( !guy->get_bionics().empty() ) {
        assigned_hotkey = input_ptr->next_unassigned_hotkey( hotkeys, assigned_hotkey );
        radio_group( "RECHARGE_RULES", _( "CBM recharging rules:" ), &guy->rules.cbm_recharge, recharge_map,
                     assigned_hotkey, pressed_key );

        assigned_hotkey = input_ptr->next_unassigned_hotkey( hotkeys, assigned_hotkey );
        radio_group( "RESERVE_RULES", _( "CBM reserve rules" ), &guy->rules.cbm_reserve, reserve_map,
                     assigned_hotkey, pressed_key );
    }

    ImGui::InvisibleButton( "BOTTOM_OF_WINDOW_KB_SCROLL_SELECTABLE", ImVec2( 1.0, 1.0 ) );
}
