#pragma once
#ifndef CATA_SRC_DIALOGUE_WIN_H
#define CATA_SRC_DIALOGUE_WIN_H

#include <cstddef>
#include <vector>
#include <string>
#include <utility>

#include "color.h"
#include "cursesdef.h"

/** Represents avatar response */
struct talk_data {
    char letter;
    nc_color col;
    std::string text;
};

class ui_adaptor;

class dialogue_window
{
    public:
        dialogue_window() = default;
        void resize_dialogue( ui_adaptor &ui );
        void print_header( const std::string &name );

        void clear_window_texts();
        void handle_scrolling( int ch );
        void display_responses( const std::vector<talk_data> &responses );
        void refresh_response_display();
        /** Adds message to history. It must be already translated. */
        void add_to_history( const std::string &msg );

    private:
        catacurses::window d_win;
        /**
         * This contains the exchanged words, it is basically like the global message log.
         * Each responses of the player character and the NPC are added as are information about
         * what each of them does (e.g. the npc drops their weapon).
         */
        std::vector<std::string> history;
        /**
         * Drawing cache: basically all entries from @ref history, but folded to current
         * window width and with separators between. Used for rendering, recalculated each time window size changes.
         */
        std::vector<std::pair<std::string, size_t>> draw_cache;
        /** Scroll position in response window (page number) */
        size_t curr_page = 0;
        bool can_scroll_up = false;
        bool can_scroll_down = false;

        // Prints history. Automatically highlighting last message.
        void print_history();

        /**
         * Folds given message to current window width and adds it to drawing cache.
         * Also adds a separator (empty line).
         * idx is this message's position within @ref history_raw
         */
        void cache_msg( const std::string &msg, size_t idx );

        std::string npc_name;
};
#endif // CATA_SRC_DIALOGUE_WIN_H

