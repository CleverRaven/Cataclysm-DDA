#pragma once
#ifndef CATA_SRC_VEH_APPLIANCE_H
#define CATA_SRC_VEH_APPLIANCE_H

#include "input.h"
#include "player_activity.h"

class vehicle;
class ui_adaptor;
struct point;

class veh_app_interact
{
    public:
        static player_activity run( vehicle &veh, const point &p = point_zero );

    private:
        explicit veh_app_interact( vehicle &veh, const point &p );
        ~veh_app_interact() = default;

        point a_point = point_zero;
        vehicle *veh;
        input_context ctxt;
        weak_ptr_fast<ui_adaptor> ui;
        player_activity act;

        // Actions for input window
        std::vector<std::function<void()>> app_actions;

        // Curses windows
        catacurses::window w_border;
        catacurses::window w_info;
        // Input menu
        uilist imenu;

        void init_ui_windows();
        void draw_info();
        void populate_app_actions();
        shared_ptr_fast<ui_adaptor> create_or_get_ui_adaptor();

        bool can_refill();
        bool can_siphon();

        void refill();
        void siphon();
        void rename();

        void app_loop();
};

#endif // CATA_SRC_VEH_APPLIANCE_H