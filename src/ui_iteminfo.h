#pragma once
#ifndef CATA_SRC_UI_ITEMINFO_H
#define CATA_SRC_UI_ITEMINFO_H

#include "cata_imgui.h"
#include "imgui/imgui.h"
#include "input_context.h"
#include "output.h"
#include "point.h"

class iteminfo_window : public cataimgui::window
{
    public:
        iteminfo_window( item_info_data &info, point pos, int width, int height,
                         ImGuiWindowFlags flags = ImGuiWindowFlags_None );
        void execute();

    protected:
        void draw_controls() override;
        cataimgui::bounds get_bounds() override;

    private:
        item_info_data data;
        point pos;
        int width;
        int height;
        input_context ctxt;

        cataimgui::scroll s;
};

#endif // CATA_SRC_UI_ITEMINFO_H
