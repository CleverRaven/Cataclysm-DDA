#pragma once
#ifndef CATA_SRC_FACTION_UI_H
#define CATA_SRC_FACTION_UI_H

#include <algorithm>
#include <string>

#include <imgui/imgui.h>

#include "cata_imgui.h"
#include "game_constants.h"
#include "input_context.h"
#include "translations.h"
#include "type_id.h"

class basecamp;
class faction;
class npc;

enum class tab_mode : int {
    TAB_MYFACTION = 0,
    TAB_FOLLOWERS,
    TAB_OTHERFACTIONS,
    TAB_CREATURES,
    NUM_TABS
};

class faction_ui : public cataimgui::window
{
    public:
        explicit faction_ui( ) : cataimgui::window( _( "Faction" ),
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav ) {
        };

        bool execute();

        void draw_hint_section() const;

        void draw_your_faction_tab();
        void draw_your_factions_list();
        void your_faction_display() const;

        void draw_your_followers_tab();
        void draw_your_followers_list();
        void your_follower_display();

        void draw_other_factions_tab();
        void draw_other_factions_list();
        void other_faction_display();

        // i think creature tab better be migrated to diary and adjacent UI
        void draw_creatures_tab();
        void draw_creature_list();
        void creature_display() const;

        std::string last_action;
    protected:

        void draw_controls() override;
        cataimgui::bounds get_bounds() override {

            const float window_width = std::clamp( float( str_width_to_pixels( EVEN_MINIMUM_TERM_WIDTH ) ),
                                                   ImGui::GetMainViewport()->Size.x / 2,
                                                   ImGui::GetMainViewport()->Size.x );
            const float window_height = std::clamp( float( str_height_to_pixels( EVEN_MINIMUM_TERM_HEIGHT ) ),
                                                    ImGui::GetMainViewport()->Size.y / 2,
                                                    ImGui::GetMainViewport()->Size.y );

            const cataimgui::bounds bounds{ -1.f, -1.f, window_width, window_height };
            return bounds;
        }

    private:
        input_context ctxt;
        cataimgui::scroll s = cataimgui::scroll::none;
        tab_mode selected_tab = tab_mode::TAB_MYFACTION;
        // hack, hide the window if picked talking to someone
        // remove when npc dialogue menu will be made imgui
        bool hide_ui = false;

        basecamp *picked_camp = nullptr;
        npc *picked_follower = nullptr;
        const faction *picked_faction = nullptr;
        const mtype_id *picked_creature = nullptr;
        bool interactable = false;
        bool radio_interactable = false;

        float get_table_column_width() const {
            return 260.f;
        }
};

#endif // CATA_SRC_FACTION_UI_H
