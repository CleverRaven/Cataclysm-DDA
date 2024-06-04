#pragma once
#ifndef CATA_SRC_NPCTALK_RULES_H
#define CATA_SRC_NPCTALK_RULES_H

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
        explicit follower_rules_ui_impl() : cataimgui::window( _( "Rules for your follower" ),
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ) {
        }

        void set_npc_pointer_to( npc *new_guy );

    private:
        npc *guy = nullptr;
        std::string get_parsed( std::string initial_string );

        size_t window_width = str_width_to_pixels( TERMX ) / 2;
        size_t window_height = str_height_to_pixels( TERMY ) / 2;
        size_t table_column_width = window_width / 2;

    protected:
        void draw_controls() override;
};

#endif // CATA_SRC_NPCTALK_RULES_H