#pragma once
#ifndef CATA_SRC_UI_EXTENDED_DESCRIPTION_H
#define CATA_SRC_UI_EXTENDED_DESCRIPTION_H

#include <cstdint>
#include <string>
#include <vector>

#include "cata_imgui.h"
#include "coordinates.h"
#include "input_context.h"
#include "output.h"

enum class description_target : int {
    creature = 0,
    furniture,
    terrain,
    vehicle,
    num_targets
};

void draw_extended_description( const std::vector<std::string> &description, uint64_t width );

class extended_description_window : public cataimgui::window
{
    public:
        explicit extended_description_window( tripoint_bub_ms &p );

        void show();

    protected:
        void draw_controls() override;
        cataimgui::bounds get_bounds() override;

    private:
        void draw_creature_tab();
        void draw_furniture_tab();
        void draw_terrain_tab();
        void draw_vehicle_tab();

        input_context ctxt;
        description_target cur_target = description_target::terrain;
        description_target switch_target = description_target::terrain;
        tripoint_bub_ms p;
        cataimgui::bounds bounds{ 0.0f, 0.0f, static_cast<float>( str_width_to_pixels( TERMX ) ), static_cast<float>( str_height_to_pixels( TERMY ) ) };

        std::vector<std::string> creature_description;
        std::vector<std::string> furniture_description;
        std::vector<std::string> terrain_description;
        std::vector<std::string> veh_app_description;
};

#endif // CATA_SRC_UI_EXTENDED_DESCRIPTION_H
