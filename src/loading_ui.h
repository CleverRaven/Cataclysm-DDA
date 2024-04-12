#pragma once
#ifndef CATA_SRC_LOADING_UI_H
#define CATA_SRC_LOADING_UI_H

#include <memory>
#include <string>

class background_pane;
class ui_adaptor;
class uilist;

class loading_ui
{
    private:
        std::unique_ptr<uilist> menu;
        std::unique_ptr<ui_adaptor> ui;
        std::unique_ptr<background_pane> ui_background;

        void init();
    public:
        explicit loading_ui( bool display );
        ~loading_ui();

        /**
         * Sets the description for the menu and clears existing entries.
         */
        void new_context( const std::string &desc );
        /**
         * Adds a named entry in the current loading context.
         */
        void add_entry( const std::string &description );
        /**
         * Place the UI onto UI stack, mark current entry as processed, scroll down,
         * and redraw. (if display is enabled)
         */
        void proceed();
        /**
         * Place the UI onto UI stack and redraw it on the screen (if display is enabled).
         */
        void show();
};

#endif // CATA_SRC_LOADING_UI_H
