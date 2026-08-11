#pragma once
#ifndef CATA_SRC_DIALOGUE_IMGUI_H
#define CATA_SRC_DIALOGUE_IMGUI_H

#include <string>
#include <vector>

#include "cata_imgui.h"
#include "color.h"
#include "dialogue_win.h"
#include "imgui/imgui.h"
#include "panels.h"
#include "translations.h"

struct dialogue;

class dialogue_imgui
{
        friend class dialogue_imgui_impl;
    public:
        explicit dialogue_imgui( dialogue *conversation ) : conversation( conversation ) {
        }
        void draw_dialogue_imgui( bool is_computer, bool is_not_conversation,
                                  const std::string &remote_name );
    private:
        dialogue *conversation;
};

class dialogue_imgui_impl : public cataimgui::window
{
    public:
        std::string last_action;
        explicit dialogue_imgui_impl( dialogue *conversation, bool is_computer, bool is_not_conversation,
                                      const std::string &remote_name ) : cataimgui::window( _( "Dialogue" ),
                                                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav |
                                                  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar ),
            is_computer( is_computer ),
            is_not_conversation( is_not_conversation ),
            remote_name( remote_name ),
            conversation( conversation ) {
            if( !remote_name.empty() ) {
                is_remote = true;
            }
        }

    private:
        float window_width = ImGui::GetMainViewport()->WorkSize.x - float( str_width_to_pixels(
                                 panel_manager::get_manager().get_width_right() + panel_manager::get_manager().get_width_left() ) );
        float window_height = ImGui::GetMainViewport()->WorkSize.y;

        struct history_message {
            inline history_message( nc_color c, const std::string &t ) : color( c ), text( t ) {}

            nc_color color; // Text color when highlighted
            std::string text;
        };

        std::vector<history_message> history;

        std::vector<talk_data> response_list;

        // Public for now, access should be set properly later (friend class dialogue?)
    public:
        /** Adds a message to the conversation history. */
        void add_to_history( const std::string &text );
        /** Adds a message to the conversation history for a given speaker. */
        void add_to_history( const std::string &text, const std::string &speaker_name,
                             nc_color speaker_color );
        void add_to_history( const std::string &text, nc_color color );

        bool is_computer = false;
        bool is_not_conversation = false;
        // Remote conversation (intercom, radio). Hides physical-presence
        // actions (Look at, Assess, etc.) but keeps normal dialogue style.
        // If remote_name is set, it replaces the NPC name in the header.
        bool is_remote = false;
        std::string remote_name;
        void set_responses( const std::vector<talk_data> &responses );
    private:
        void draw_history() const;
        void draw_responses() const;
        nc_color default_color() const;

        void draw_dialogue_sidebar() const;
        void draw_dialogue_history() const;
        void draw_dialogue_responses() const;
        dialogue *conversation;

    protected:
		cataimgui::bounds get_bounds() override;
        void draw_controls() override;
};
#endif // CATA_SRC_DIALOGUE_IMGUI_H
