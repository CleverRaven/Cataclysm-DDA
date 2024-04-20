#pragma once
#ifndef CATA_SRC_DIALOGUE_WIN_H
#define CATA_SRC_DIALOGUE_WIN_H

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "color.h"
#include "cursesdef.h"
#include "output.h"

class input_context;
class ui_adaptor;

struct talk_data {
    nc_color color;
    std::string hotkey_desc;
    std::string text;
    multiline_list_entry get_entry() const;
};

/**
 * NPC conversation dialogue window.
 */
class dialogue_window
{
    public:
        dialogue_window();
        void resize( ui_adaptor &ui );
        void draw( const std::string &npc_name );

        void handle_scrolling( std::string &action, input_context &ctxt );

        /** Adds a message to the conversation history. */
        void add_to_history( const std::string &text );
        /** Adds a message to the conversation history for a given speaker. */
        void add_to_history( const std::string &text, const std::string &speaker_name,
                             nc_color speaker_color );
        /** Adds a separator to the conversation history. */
        void add_history_separator();

        void set_responses( const std::vector<talk_data> &responses );

        void set_up_scrolling( input_context &ctxt ) const;

        /** Unhighlights all messages. */
        void clear_history_highlights();
        bool is_computer = false;
        bool is_not_conversation = false;
        int sel_response = 0;
    private:
        catacurses::window d_win;
        catacurses::window history_win;
        catacurses::window resp_win;

        struct history_message {
            inline history_message( nc_color c, const std::string &t ) : color( c ), text( t ) {}

            nc_color color; // Text color when highlighted
            std::string text;
        };

        void add_to_history( const std::string &text, nc_color color );

        /**
         * This contains the exchanged words, it is basically like the global message log.
         *
         * Each responses of the player character and the NPC are added as are information about
         * what each of them does (e.g. the npc drops their weapon).
         * This will be displayed in the dialog window and should already be translated.
         */
        std::vector<history_message> history;
        std::unique_ptr<scrolling_text_view> history_view;
        bool update_history_view = true;
        /** Number of history messages to highlight. */
        int num_lines_highlighted;
        /** Stored responses (hotkey, lines) */
        std::vector<std::tuple<std::string, std::vector<std::string>>> folded_txt;
        std::vector<int> folded_heights;
        std::unique_ptr<multiline_list> responses_list;

        nc_color default_color() const;
        void print_header( const std::string &name ) const;
};
#endif // CATA_SRC_DIALOGUE_WIN_H

