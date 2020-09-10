#pragma once
#ifndef CATA_SRC_SMART_CONTROLLER_UI_H
#define CATA_SRC_SMART_CONTROLLER_UI_H

#include "cursesdef.h"
#include "input.h"

/**
 * Configurable settings for the Smart Controller.
 * Fields are references to the actual variables that have to be changed.
 */
struct smart_controller_settings {
    bool &enabled;
    int &battery_lo;
    int &battery_hi;

    smart_controller_settings( bool &enabled, int &battery_lo, int &battery_hi );
};

// UI for the Smart Engine Controller
class smart_controller_ui
{
    public:
        static const int WIDTH = 52;
        static const int HEIGHT = 36;

        smart_controller_ui( smart_controller_settings initial_settings );

        // open UI and allow user to interact with it
        void control();

    private:

        static const int LEFT_MARGIN = 6;

        static const int MENU_ITEM_HEIGHT = 5;
        static const int MENU_ITEMS_N = 3;

        static const int SLIDER_W = 40;

        // Output window. This class assumes win's size does not change.
        catacurses::window win;
        input_context input_ctx;
        // current state of settings
        const smart_controller_settings settings;

        // selected menu row
        int selection = 0;
        // selected slider (0 or 1)
        int slider = 0;
        // draws the window's content
        void refresh();
};

#endif // CATA_SRC_SMART_CONTROLLER_UI_H
