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

/**
 * NPC conversation dialogue window.
 */
class dialogue_window
{
    public:
        dialogue_window() = default;
        void resize( ui_adaptor &ui );
        void draw( const std::string &npc_name, const std::vector<talk_data> &responses );

        void handle_scrolling( const std::string &action, int num_responses );

        void refresh_response_display();
        /** Adds a message to the conversation history. */
        void add_to_history( const std::string &text );
        /** Adds a message to the conversation history for a given speaker. */
        void add_to_history( const std::string &text, const std::string &speaker_name,
                             nc_color speaker_color );
        /** Adds a separator to the conversation history. */
        void add_history_separator();

        /** Unhighlights all messages. */
        void clear_history_highlights();
        bool is_computer = false;
        bool is_not_conversation = false;
        int sel_response = 0;
    private:
        catacurses::window d_win;

        struct history_message {
            inline history_message( nc_color c, const std::string &t ) : color( c ), text( t ) {}

            nc_color color; // Text color when highlighted
            std::string text;
        };

        void add_to_history( const std::string &text, nc_color color );
        void add_to_folded_history( const history_message &message, bool is_highlighted );
        void rebuild_folded_history();

        /**
         * This contains the exchanged words, it is basically like the global message log.
         *
         * Each responses of the player character and the NPC are added as are information about
         * what each of them does (e.g. the npc drops their weapon).
         * This will be displayed in the dialog window and should already be translated.
         */
        std::vector<history_message> history;
        /** Number of history messages to highlight. */
        int num_lines_highlighted;
        /** Cache of folded history text */
        std::vector<history_message> history_folded;  // Cache of folded history text
        /** Number of folded history messages to highlight. */
        int num_folded_lines_highlighted;
        /** Stored responses (hotkey, lines) */
        std::vector<std::tuple<std::string, std::vector<std::string>>> folded_txt;
        std::vector<int> folded_heights;

        // yoffset of the current response window
        int scroll_yoffset = 0;
        bool can_scroll_up = false;
        bool can_scroll_down = false;
        nc_color default_color() const;
        void print_header( const std::string &name );
        void print_history();
        bool print_responses( const std::vector<talk_data> &responses );
};
#endif // CATA_SRC_DIALOGUE_WIN_H

