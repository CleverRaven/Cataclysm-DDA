#pragma once
#ifndef CATA_SRC_DEBUG_IMGUI_DEMO_H
#define CATA_SRC_DEBUG_IMGUI_DEMO_H

#include "cata_imgui.h"

class debug_imgui_demo_ui : public cataimgui::window
{
    public:
        debug_imgui_demo_ui();
        void init();
        void run();

    protected:
        void draw_controls() override;
        cataimgui::bounds get_bounds() override;
        void on_resized() override {
            init();
        };
    private:
        std::shared_ptr<cataimgui::Paragraph> stuff;
};

#endif // CATA_SRC_DEBUG_IMGUI_DEMO_H
