#pragma once
#ifndef CATA_SRC_MEDICAL_UI_H
#define CATA_SRC_MEDICAL_UI_H

#include "game_constants.h"
#include "cata_imgui.h"
#include "ui_manager.h"
#include "input_context.h"

class medical_ui : public cataimgui::window
{
    public:
        medical_ui() : cataimgui::window( "Imgui medical ui" ) {
            ctxt = input_context();
        };
        void execute();
    protected:
        void draw_controls() override;
    private:
        input_context ctxt;
        float window_width = std::clamp( float( str_width_to_pixels( EVEN_MINIMUM_TERM_WIDTH ) ),
                                         ImGui::GetMainViewport()->Size.x / 2,
                                         ImGui::GetMainViewport()->Size.x );
        float window_height = std::clamp( float( str_height_to_pixels( EVEN_MINIMUM_TERM_HEIGHT ) ),
                                          ImGui::GetMainViewport()->Size.y / 2,
                                          ImGui::GetMainViewport()->Size.y );
        float table_column_width = window_width / 2;
};

#endif // CATA_SRC_MEDICAL_UI_H
