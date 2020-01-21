#pragma once
#ifndef COMPUTER_SESSION_H
#define COMPUTER_SESSION_H

#include <vector>

#include "computer.h"
#include "cursesdef.h"

class player;

class computer_session
{
    public:
        computer_session( computer &comp );

        /** Handles player use of a computer */
        void use();

    private:
        computer &comp;
        // Output window. This class assumes w's dimensions do not change.
        const catacurses::window w;
        const int left;
        const int top;
        const int width;
        const int height;
        std::vector<std::pair<int, std::string>> lines;

        /** Shutdown (free w_terminal, etc.) */
        void shutdown_terminal();
        /** Returns true if the player successfully hacks the computer. Security = -1 defaults to
         *  the main system security. */
        bool hack_attempt( player &p, int Security = -1 );

        // Called by use()
        void activate_function( computer_action action );
        // ...but we can also choose a specific failure.
        void activate_failure( computer_failure_type fail );
        // Generally called when we fail a hack attempt
        void activate_random_failure();

        // Reset to a blank terminal (e.g. at start of usage loop)
        void reset_terminal();
        // Actually displaying the terminal window
        void refresh();
        // Move the cursor to the beginning of the next line
        void print_newline();
        // Do the actual printing
        template<typename ...Args>
        void print_indented_line( int indent, int text_width, const std::string &text, Args &&... args );
        // Prints code-looking gibberish
        void print_gibberish_line();
        // Prints a line to the terminal (with printf flags)
        template<typename ...Args>
        void print_line( const std::string &text, Args &&... args );
        // For now, the same as print_line but in red ( TODO: change this?)
        template<typename ...Args>
        void print_error( const std::string &text, Args &&... args );
        // Wraps and prints a block of text with a 1-space indent
        template<typename ...Args>
        void print_text( const std::string &text, Args &&... args );
        // Prints a line and waits for Y/N/Q
        template<typename ...Args>
        char query_ynq( const std::string &text, Args &&... args );
        // Same as query_ynq, but returns true for y or Y
        template<typename ...Args>
        bool query_bool( const std::string &text, Args &&... args );
        // Simply wait for any key, returns True
        template<typename ...Args>
        bool query_any( const std::string &text, Args &&... args );

        void mark_refugee_center();
};

#endif
