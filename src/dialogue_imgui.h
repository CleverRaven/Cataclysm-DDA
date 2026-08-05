#pragma once
#ifndef CATA_SRC_DIALOGUE_IMGUI_H
#define CATA_SRC_DIALOGUE_IMGUI_H

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "cata_imgui.h"
#include "color.h"
#include "cursesdef.h"
#include "imgui/imgui.h"
#include "output.h"

class input_context;

class dialogue_imgui
{
        friend class dialogue_imgui_impl;
    public:
        void draw_dialogue_imgui();
};

class dialogue_imgui_impl : public cataimgui::window
{
    public:
        std::string last_action;
        explicit dialogue_imgui_impl() : cataimgui::window( _( "Dialogue" ),
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav |
                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar ) {
        }

    private:
        float window_width = std::clamp( float( str_width_to_pixels( EVEN_MINIMUM_TERM_WIDTH ) ),
                                         ImGui::GetMainViewport()->Size.x / 2,
                                         ImGui::GetMainViewport()->Size.x );
        float window_height = std::clamp( float( str_height_to_pixels( EVEN_MINIMUM_TERM_HEIGHT ) ),
                                          ImGui::GetMainViewport()->Size.y / 2,
                                          ImGui::GetMainViewport()->Size.y );

        void draw_dialogue_sidebar() const;
        void draw_dialogue_history() const;
        void draw_dialogue_responses() const;

    protected:
        void draw_controls() override;
};
#endif // CATA_SRC_DIALOGUE_IMGUI_H
