#include "npctalk_rules.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cata_imgui.h"
#include "dialogue.h"
#include "npctalk.h"
#include "ui_manager.h"
#include "imgui/imgui.h"

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
void follower_rules_ui_impl::multi_rule_header( std::string id, T &rule,
        std::map<T, std::string> rule_map, bool should_advance )
{
    if( ImGui::InvisibleButton( id.c_str(), ImVec2() ) || should_advance ) {
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
    draw_colored_text( string_format( "  %s  ", static_cast<char>( hotkey.sequence.front() ) ),
                       c_green );
    ImGui::SameLine();
}

void follower_rules_ui_impl::draw_controls()
{
    if( !guy ) {
        debugmsg( "Something has gone very wrong, can't find NPC to set rules for.  Aborting." );
        last_action = "QUIT";
        return;
    }

    ImGui::SetWindowSize( ImVec2( window_width, window_height ), ImGuiCond_Once );

    ImGui::InvisibleButton( "TOP_OF_WINDOW_KB_SCROLL_SELECTABLE", ImVec2() );

    const hotkey_queue &hotkeys = hotkey_queue::alphabets();
    input_event assigned_hotkey = input_ptr->first_unassigned_hotkey( hotkeys );
    input_event pressed_key = input_ptr->get_raw_input();

    draw_colored_text( string_format( _( "Rules for your follower, %s" ), guy->disp_name() ) );
    ImGui::Separator();
    draw_colored_text( _( "Hotkey:" ) );
    ImGui::NewLine();

    print_hotkey( assigned_hotkey );
    if( ImGui::Button( _( "Default ALL" ) ) || pressed_key == assigned_hotkey ) {
        ImGui::SetKeyboardFocusHere( -1 );
        // TODO: use query_yn here as a safeguard against fatfingering. Can't use it right now,
        // it destructs the imgui instance(?!) which obviously causes us to crash
        npc_follower_rules default_values; //Call the constructor and let *it* tell us what the default is
        guy->rules = default_values;
    }

    int rule_number = 0;
    /* Handle all of our regular, boolean rules */
    for( const std::pair<const std::string, ally_rule_data> &rule_data : ally_rule_strs ) {
        ImGui::PushID( rule_number );
        assigned_hotkey = input_ptr->next_unassigned_hotkey( hotkeys, assigned_hotkey );
        rule_number++;
        ImGui::NewLine();
        print_hotkey( assigned_hotkey );
        const ally_rule_data &this_rule = rule_data.second;
        bool rule_enabled = false;
        std::string rules_text;
        if( guy->rules.has_flag( this_rule.rule ) ) {
            rules_text = string_format( "%s", get_parsed( this_rule.rule_true_text ) );
            rule_enabled = true;
        } else {
            rules_text = string_format( "%s", get_parsed( this_rule.rule_false_text ) );
        }
        if( rule_enabled ) {
            ImGui::PushStyleColor( ImGuiCol_Button, c_green );
        }
        if( ImGui::Button( _( "Toggle" ) ) || pressed_key == assigned_hotkey ) {
            ImGui::SetKeyboardFocusHere( -1 );
            guy->rules.toggle_flag( this_rule.rule );
            guy->rules.toggle_specific_override_state( this_rule.rule, !rule_enabled );
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();
        if( ImGui::Button( _( "Default" ) ) ) {
            guy->rules.clear_flag( this_rule.rule );
            guy->rules.clear_override( this_rule.rule );
        }
        ImGui::SameLine();
        draw_colored_text( rules_text );
        ImGui::PopID();
    }

    // Engagement rules require their own set of buttons, each instruction is unique
    ImGui::Separator();
    ImGui::NewLine();
    assigned_hotkey = input_ptr->next_unassigned_hotkey( hotkeys, assigned_hotkey );
    print_hotkey( assigned_hotkey );
    multi_rule_header( "ENGAGEMENT_RULES", guy->rules.engagement, engagement_rules,
                       pressed_key == assigned_hotkey );
    int engagement_rule_number = 0;
    for( const std::pair<const combat_engagement, std::string> &engagement_rule : engagement_rules ) {
        ImGui::PushID( rule_number );
        rule_number++;
        engagement_rule_number++;
        // Could use a better label for these...
        std::string button_label = std::to_string( engagement_rule_number );
        ImGui::SameLine();
        if( guy->rules.engagement == engagement_rule.first ) {
            ImGui::PushStyleColor( ImGuiCol_Button, c_green );
        }
        if( ImGui::Button( button_label.c_str() ) ) {
            guy->rules.engagement = engagement_rule.first;
            ImGui::SetKeyboardFocusHere( -1 );
        }
        ImGui::PopStyleColor();
        ImGui::PopID();
    }
    ImGui::SameLine();
    draw_colored_text( _( "Engagement rules" ), c_white );
    ImGui::NewLine();
    draw_colored_text( string_format( "%s", get_parsed( engagement_rules[guy->rules.engagement] ) ) );

    // Aiming rule also has a non-boolean set of values
    ImGui::Separator();
    ImGui::NewLine();
    assigned_hotkey = input_ptr->next_unassigned_hotkey( hotkeys, assigned_hotkey );
    print_hotkey( assigned_hotkey );
    multi_rule_header( "AIMING_RULES", guy->rules.aim, aim_rule_map, pressed_key == assigned_hotkey );
    int aim_rule_number = 0;
    for( const std::pair<const aim_rule, std::string> &aiming_rule : aim_rule_map ) {
        ImGui::PushID( rule_number );
        rule_number++;
        aim_rule_number++;
        // Could use a better label for these...
        std::string button_label = std::to_string( aim_rule_number );
        ImGui::SameLine();
        if( guy->rules.aim == aiming_rule.first ) {
            ImGui::PushStyleColor( ImGuiCol_Button, c_green );
        }
        if( ImGui::Button( button_label.c_str() ) ) {
            guy->rules.aim = aiming_rule.first;
            ImGui::SetKeyboardFocusHere( -1 );
        }
        ImGui::PopStyleColor();
        ImGui::PopID();
    }
    ImGui::SameLine();
    draw_colored_text( _( "Aiming rules" ), c_white );
    ImGui::NewLine();
    draw_colored_text( string_format( "%s", get_parsed( aim_rule_map[guy->rules.aim] ) ) );

    /* Shows CBM rules, but only if the character has bionics. Must be last because it will
    only appear sometimes and we don't want hotkeys to be different depending on whether
    the character has bionics. That's bad for muscle memory! */
    if( !guy->get_bionics().empty() ) {
        ImGui::Separator();
        ImGui::NewLine();
        assigned_hotkey = input_ptr->next_unassigned_hotkey( hotkeys, assigned_hotkey );
        print_hotkey( assigned_hotkey );
        multi_rule_header( "RECHARGE_RULES", guy->rules.cbm_recharge, recharge_map,
                           pressed_key == assigned_hotkey );
        for( const std::pair<const cbm_recharge_rule, std::string> &recharge_rule : recharge_map ) {
            ImGui::PushID( rule_number );
            rule_number++;
            int percent = static_cast<int>( recharge_rule.first );
            std::string button_label = std::to_string( percent ) + "%";
            ImGui::SameLine();
            if( guy->rules.cbm_recharge == recharge_rule.first ) {
                ImGui::PushStyleColor( ImGuiCol_Button, c_green );
            }
            if( ImGui::Button( button_label.c_str() ) ) {
                guy->rules.cbm_recharge = recharge_rule.first;
                ImGui::SetKeyboardFocusHere( -1 );
            }
            ImGui::PopStyleColor();
            ImGui::PopID();
        }
        ImGui::SameLine();
        draw_colored_text( _( "CBM recharging rules" ), c_white );
        ImGui::NewLine();
        draw_colored_text( string_format( "%s", get_parsed( recharge_map[guy->rules.cbm_recharge] ) ) );

        ImGui::Separator();
        ImGui::NewLine();
        assigned_hotkey = input_ptr->next_unassigned_hotkey( hotkeys, assigned_hotkey );
        print_hotkey( assigned_hotkey );
        multi_rule_header( "RESERVE_RULES", guy->rules.cbm_reserve, reserve_map,
                           pressed_key == assigned_hotkey );
        for( const std::pair<const cbm_reserve_rule, std::string> &reserve_rule : reserve_map ) {
            ImGui::PushID( rule_number );
            rule_number++;
            int percent = static_cast<int>( reserve_rule.first );
            std::string button_label = std::to_string( percent ) + "%";
            ImGui::SameLine();
            if( guy->rules.cbm_reserve == reserve_rule.first ) {
                ImGui::PushStyleColor( ImGuiCol_Button, c_green );
            }
            if( ImGui::Button( button_label.c_str() ) ) {
                guy->rules.cbm_reserve = reserve_rule.first;
                ImGui::SetKeyboardFocusHere( -1 );
            }
            ImGui::PopStyleColor();
            ImGui::PopID();
        }
        ImGui::SameLine();
        draw_colored_text( _( "CBM reserve rules" ), c_white );
        ImGui::NewLine();
        draw_colored_text( string_format( "%s", get_parsed( reserve_map[guy->rules.cbm_reserve] ) ) );
    }

    ImGui::InvisibleButton( "BOTTOM_OF_WINDOW_KB_SCROLL_SELECTABLE", ImVec2() );
}
