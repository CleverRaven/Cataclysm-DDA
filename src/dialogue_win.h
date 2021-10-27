#pragma once
#ifndef CATA_SRC_DIALOGUE_WIN_H
#define CATA_SRC_DIALOGUE_WIN_H

#include <cstddef>
#include <iosfwd>
#include <vector>

#include "color.h"
#include "cursesdef.h"

struct talk_data {
    nc_color color;
    std::string hotkey_desc;
    std::string text;
};

class ui_adaptor;

class dialogue_window
{
    public:
        dialogue_window() = default;
        void resize_dialogue( ui_adaptor &ui );
        void print_header( const std::string &name );

        bool text_only = false;

        void clear_window_texts();
        void handle_scrolling( const std::string &action );
        void display_responses( int highlight_lines, const std::vector<talk_data> &responses );
        void refresh_response_display();
        /**
         * Adds a message to the conversation history.
         *
         * Returns the number of added lines.
         */
        size_t add_to_history( const std::string &text );
        /**
         * Adds a message to the conversation history for a given speaker.
         *
         * Returns the number of added lines.
         */
        size_t add_to_history( const std::string &text, const std::string &speaker_name,
                               nc_color speaker_color );
        void add_history_separator();

    private:
        size_t add_to_history( const std::string &text, nc_color color );

        catacurses::window d_win;
        struct history_message {
            inline history_message( nc_color c, const std::string &t ) : color( c ), text( t ) {}

            nc_color color; // Text color when highlighted
            std::string text;
        };
        /**
         * This contains the exchanged words, it is basically like the global message log.
         * Each responses of the player character and the NPC are added as are information about
         * what each of them does (e.g. the npc drops their weapon).
         * This will be displayed in the dialog window and should already be translated.
         */
        std::vector<history_message> history;
        // yoffset of the current response window
        int scroll_yoffset = 0;
        bool can_scroll_up = false;
        bool can_scroll_down = false;

        void print_history( size_t highlight_lines );
        bool print_responses( const std::vector<talk_data> &responses );

        std::string npc_name;
};
#endif // CATA_SRC_DIALOGUE_WIN_H

