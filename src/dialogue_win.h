#pragma once
#ifndef DIALOGUE_WIN_H
#define DIALOGUE_WIN_H

#include <cstddef>
#include <vector>
#include <string>
#include <utility>

#include "color.h"
#include "cursesdef.h"

using talk_data = std::pair<nc_color, std::string>;

class dialogue_window
{
    public:
        dialogue_window() = default;
        void open_dialogue( bool text_only = false );
        void print_header( const std::string &name );

        bool text_only = false;

        void clear_window_texts();
        void display_responses( int hilight_lines, const std::vector<talk_data> &responses,
                                const int &ch );
        void refresh_response_display();
        /**
         * Folds and adds the folded text to @ref history. Returns the number of added lines.
         */
        size_t add_to_history( const std::string &text );
        void add_history_separator();

    private:
        catacurses::window d_win;
        /**
         * This contains the exchanged words, it is basically like the global message log.
         * Each responses of the player character and the NPC are added as are information about
         * what each of them does (e.g. the npc drops their weapon).
         * This will be displayed in the dialog window and should already be translated.
         */
        std::vector<std::string> history;
        // yoffset of the current response window
        int yoffset;
        bool can_scroll_up;
        bool can_scroll_down;

        void print_history( size_t hilight_lines );
        bool print_responses( int yoffset, const std::vector<talk_data> &responses );

        std::string npc_name;
};
#endif

