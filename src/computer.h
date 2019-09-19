#pragma once
#ifndef COMPUTER_H
#define COMPUTER_H

#include <string>
#include <vector>

#include "calendar.h"
#include "cursesdef.h"

class player;
class JsonObject;

// Don't change those! They must stay in this specific order!
// TODO: Remove this enum
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
    COMPACT_MAP_SUBWAY,
    COMPACT_MISS_LAUNCH,
    COMPACT_MISS_DISARM,
    COMPACT_LIST_BIONICS,
    COMPACT_ELEVATOR_ON,
    COMPACT_AMIGARA_LOG,
    COMPACT_AMIGARA_START,
    COMPACT_COMPLETE_DISABLE_EXTERNAL_POWER, // Completes "Disable External Power" mission
    COMPACT_REPEATER_MOD,       //Converts a terminal in a radio station into a 'repeater', locks terminal and completes mission
    COMPACT_DOWNLOAD_SOFTWARE,
    COMPACT_BLOOD_ANAL,
    COMPACT_DATA_ANAL,
    COMPACT_DISCONNECT,
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
    COMPACT_OPEN_DISARM,
    COMPACT_UNLOCK_DISARM,
    COMPACT_RELEASE_DISARM,
    COMPACT_IRRADIATOR,
    COMPACT_GEIGER,
    COMPACT_CONVEYOR,
    COMPACT_SHUTTERS,
    COMPACT_EXTRACT_RAD_SOURCE,
    COMPACT_DEACTIVATE_SHOCK_VENT,
    COMPACT_RADIO_ARCHIVE,
    NUM_COMPUTER_ACTIONS
};

// Don't change those! They must stay in this specific order!
// TODO: Remove this enum
enum computer_failure_type {
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

// TODO: Turn the enum into id, get rid of this
computer_action computer_action_from_string( const std::string &str );
computer_failure_type computer_failure_type_from_string( const std::string &str );

struct computer_option {
    std::string name;
    computer_action action;
    int security;

    computer_option();
    computer_option( const std::string &N, computer_action A, int S );

    static computer_option from_json( JsonObject &jo );
};

struct computer_failure {
    computer_failure_type type;

    computer_failure( computer_failure_type t ) : type( t ) {
    }

    static computer_failure from_json( JsonObject &jo );
};

class computer
{
    public:
        computer( const std::string &new_name, int new_security );
        computer( const computer &rhs );
        ~computer();

        computer &operator=( const computer &rhs );
        // Initialization
        void set_security( int Security );
        void add_option( const computer_option &opt );
        void add_option( const std::string &opt_name, computer_action action, int security );
        void add_failure( const computer_failure &failure );
        void add_failure( computer_failure_type failure );
        void set_warning( const std::string &new_warning );
        // Basic usage
        /** Shutdown (free w_terminal, etc.) */
        void shutdown_terminal();
        /** Handles player use of a computer */
        void use();
        /** Returns true if the player successfully hacks the computer. Security = -1 defaults to
         *  the main system security. */
        bool hack_attempt( player &p, int Security = -1 );
        // Save/load
        std::string save_data() const;
        void load_data( const std::string &data );

        std::string name; // "Jon's Computer", "Lab 6E77-B Terminal Omega"
        int mission_id; // Linked to a mission?

    private:
        // Difficulty of simply logging in
        int security;
        // Date of next attempt
        time_point next_attempt = calendar::before_time_starts;
        // Things we can do
        std::vector<computer_option> options;
        // Things that happen if we fail a hack
        std::vector<computer_failure> failures;
        // Optional message to warn the player of the consequences of a failed
        // hacking attempt (like secubots spawning)
        std::string warning;
        // Output window
        catacurses::window w_terminal;
        // Pretty border
        catacurses::window w_border;
        // Misc research notes from json
        static std::vector<std::string> lab_notes;

        // Called by use()
        void activate_function( computer_action action );
        // Generally called when we fail a hack attempt
        void activate_random_failure();
        // ...but we can also choose a specific failure.
        void activate_failure( computer_failure_type fail );

        void remove_option( computer_action action );

        void mark_refugee_center();

        // OUTPUT/INPUT:

        // Reset to a blank terminal (e.g. at start of usage loop)
        void reset_terminal();
        // Prints a line to the terminal (with printf flags)
        template<typename ...Args>
        void print_line( const char *text, Args &&... args );
        // For now, the same as print_line but in red ( TODO: change this?)
        template<typename ...Args>
        void print_error( const char *text, Args &&... args );
        // Wraps and prints a block of text with a 1-space indent
        template<typename ...Args>
        void print_text( const char *text, Args &&... args );
        // Prints code-looking gibberish
        void print_gibberish_line();
        // Prints a line and waits for Y/N/Q
        template<typename ...Args>
        char query_ynq( const char *text, Args &&... args );
        // Same as query_ynq, but returns true for y or Y
        template<typename ...Args>
        bool query_bool( const char *text, Args &&... args );
        // Simply wait for any key, returns True
        template<typename ...Args>
        bool query_any( const char *text, Args &&... args );
        // Move the cursor to the beginning of the next line
        void print_newline();
};

#endif
