#pragma once
#ifndef CATA_SRC_NPCTALK_RULES_H
#define CATA_SRC_NPCTALK_RULES_H

#include <algorithm>
#include <map>
#include <string>

#include "cata_imgui.h"
#include "game_constants.h"
#include "imgui/imgui.h"
#include "translations.h"

class input_context;
class npc;
struct input_event;

class follower_rules_ui
{
        friend class follower_rules_ui_impl;
    public:
        void draw_follower_rules_ui( npc *guy );
};

class follower_rules_ui_impl : public cataimgui::window
{
    public:
        std::string last_action;
        input_context *input_ptr = nullptr;
        explicit follower_rules_ui_impl() : cataimgui::window( _( "Rules for your follower" ),
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ) {
        }

        void set_npc_pointer_to( npc *new_guy );

    private:
        npc *guy = nullptr;
        std::string get_parsed( std::string initial_string );
        void print_hotkey( input_event &hotkey );
        void rules_transfer_popup( bool &exporting_rules, bool &still_in_popup );
        bool setup_button( int &button_num, std::string &label, bool should_color );
        bool setup_table_button( int &button_num, std::string &label, bool should_color );

        float window_width = std::clamp( float( str_width_to_pixels( EVEN_MINIMUM_TERM_WIDTH ) ),
                                         ImGui::GetMainViewport()->Size.x / 2,
                                         ImGui::GetMainViewport()->Size.x );
        float window_height = std::clamp( float( str_height_to_pixels( EVEN_MINIMUM_TERM_HEIGHT ) ),
                                          ImGui::GetMainViewport()->Size.y / 2,
                                          ImGui::GetMainViewport()->Size.y );

        // makes a checkbox for the rule
        template<typename T>
        void checkbox( int rule_number, const T &this_rule, input_event &assigned_hotkey,
                       const input_event &pressed_key );

        // makes one radio button per option in the map
        template<typename T>
        void radio_group( std::string header_id, const char *title, T *rule,
                          std::map<T, std::string> &values, input_event &assigned_hotkey, const input_event &pressed_key );

        // Prepares for a rule option with multiple valid selections. Advances and wraps through
        // those options as the hotkey is pressed.
        template<typename T>
        void multi_rule_header( const std::string &id, T &rule, const std::map<T, std::string> &rule_map,
                                bool should_advance );

    protected:
        void draw_controls() override;
};

#endif // CATA_SRC_NPCTALK_RULES_H
