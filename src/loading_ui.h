#pragma once
#ifndef LOADING_UI_H
#define LOADING_UI_H

#include <memory>
#include <vector>

class uimenu;

class loading_ui
{
    private:
        std::unique_ptr<uimenu> menu;
        std::vector<std::string> entries;
    public:
        loading_ui( bool display );
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
         * Marks current entry as processed and scrolls down.
         */
        void proceed();
        /**
         * Shows the UI on the screen (if display is enabled).
         */
        void show();
};

#endif