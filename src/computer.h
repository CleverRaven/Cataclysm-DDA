#pragma once
#ifndef COMPUTER_H
#define COMPUTER_H

#include "cursesdef.h" // WINDOW
#include "printf_check.h"
#include <vector>
#include <string>

class game;
class player;
class JsonObject;

enum computer_action {
    COMPACT_NULL = 0,
    COMPACT_OPEN,
    COMPACT_LOCK,
    COMPACT_UNLOCK,
    COMPACT_TOLL,
    COMPACT_SAMPLE,
    COMPACT_RELEASE,
    COMPACT_RELEASE_BIONICS,
    COMPACT_TERMINATE,
    COMPACT_PORTAL,
    COMPACT_CASCADE,
    COMPACT_RESEARCH,
    COMPACT_MAPS,
    COMPACT_MAP_SEWER,
    COMPACT_MISS_LAUNCH,
    COMPACT_MISS_DISARM,
    COMPACT_LIST_BIONICS,
    COMPACT_ELEVATOR_ON,
    COMPACT_AMIGARA_LOG,
    COMPACT_AMIGARA_START,
    COMPACT_COMPLETE_MISSION,   //Completes the mission that has the same name as the computer action
    COMPACT_REPEATER_MOD,       //Converts a terminal in a radio station into a 'repeater', locks terminal and completes mission
    COMPACT_DOWNLOAD_SOFTWARE,
    COMPACT_BLOOD_ANAL,
    COMPACT_DATA_ANAL,
    COMPACT_DISCONNECT,
    COMPACT_STEMCELL_TREATMENT,
    COMPACT_EMERG_MESS,
    COMPACT_EMERG_REF_CENTER,   //Points to the refugee center
    COMPACT_TOWER_UNRESPONSIVE,
    COMPACT_SR1_MESS,           //Security Reminders for Hazardous Waste Sarcophagus (SRCF)
    COMPACT_SR2_MESS,
    COMPACT_SR3_MESS,
    COMPACT_SR4_MESS,
    COMPACT_SRCF_1_MESS,
    COMPACT_SRCF_2_MESS,
    COMPACT_SRCF_3_MESS,
    COMPACT_SRCF_SEAL_ORDER,
    COMPACT_SRCF_SEAL,
    COMPACT_SRCF_ELEVATOR,
    NUM_COMPUTER_ACTIONS
};

enum computer_failure {
    COMPFAIL_NULL = 0,
    COMPFAIL_SHUTDOWN,
    COMPFAIL_ALARM,
    COMPFAIL_MANHACKS,
    COMPFAIL_SECUBOTS,
    COMPFAIL_DAMAGE,
    COMPFAIL_PUMP_EXPLODE,
    COMPFAIL_PUMP_LEAK,
    COMPFAIL_AMIGARA,
    COMPFAIL_DESTROY_BLOOD,
    COMPFAIL_DESTROY_DATA,
    NUM_COMPUTER_FAILURES
};

struct computer_option {
    std::string name;
    computer_action action;
    int security;

    computer_option() {
        name = "Unknown", action = COMPACT_NULL, security = 0;
    };
    computer_option( std::string N, computer_action A, int S ) :
        name( N ), action( A ), security( S ) {};
};

class computer
{
    public:
        computer( const std::string &name, int Security );
        ~computer();

        computer &operator=( const computer &rhs );
        // Initialization
        void set_security( int Security );
        void add_option( std::string opt_name, computer_action action, int security );
        void add_failure( computer_failure failure );
        // Basic usage
        /** Shutdown (free w_terminal, etc.) */
        void shutdown_terminal();
        /** Handles player use of a computer */
        void use();
        /** Returns true if the player successfully hacks the computer. Security = -1 defaults to
         *  the main system security. */
        bool hack_attempt( player *p, int Security = -1 );
        // Save/load
        std::string save_data();
        void load_data( std::string data );

        std::string name; // "Jon's Computer", "Lab 6E77-B Terminal Omega"
        int mission_id; // Linked to a mission?

    private:
        // Difficulty of simply logging in
        int security;
        // Date of next attempt
        int next_attempt = 0;
        // Things we can do
        std::vector<computer_option> options;
        // Things that happen if we fail a hack
        std::vector<computer_failure> failures;
        // Output window
        WINDOW *w_terminal;
        // Pretty border
        WINDOW *w_border;
        // Misc research notes from json
        static std::vector<std::string> lab_notes;

        // Called by use()
        void activate_function( computer_action action, char ch );
        // Generally called when we fail a hack attempt
        void activate_random_failure();
        // ...but we can also choose a specific failure.
        void activate_failure( computer_failure fail );

        void remove_option( computer_action action );

        void mark_refugee_center();

        // OUTPUT/INPUT:

        // Reset to a blank terminal (e.g. at start of usage loop)
        void reset_terminal();
        // Prints a line to the terminal (with printf flags)
        void print_line( const char *text, ... ) PRINTF_LIKE( 2, 3 );
        // For now, the same as print_line but in red (TODO: change this?)
        void print_error( const char *text, ... ) PRINTF_LIKE( 2, 3 );
        // Wraps and prints a block of text with a 1-space indent
        void print_text( const char *text, ... ) PRINTF_LIKE( 2, 3 );
        // Prints code-looking gibberish
        void print_gibberish_line();
        // Prints a line and waits for Y/N/Q
        char query_ynq( const char *text, ... ) PRINTF_LIKE( 2, 3 );
        // Same as query_ynq, but returns true for y or Y
        bool query_bool( const char *text, ... ) PRINTF_LIKE( 2, 3 );
        // Simply wait for any key, returns True
        bool query_any( const char *text, ... ) PRINTF_LIKE( 2, 3 );
        // Move the cursor to the beginning of the next line
        void print_newline();
};

#endif
