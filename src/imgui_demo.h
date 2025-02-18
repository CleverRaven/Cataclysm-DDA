#pragma once
#ifndef CATA_SRC_IMGUI_DEMO_H
#define CATA_SRC_IMGUI_DEMO_H

#include <memory>

#include "cata_imgui.h"

namespace cataimgui
{
struct Paragraph;
}  // namespace cataimgui

class imgui_demo_ui : public cataimgui::window
{
    public:
        imgui_demo_ui();
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

#endif // CATA_SRC_IMGUI_DEMO_H
