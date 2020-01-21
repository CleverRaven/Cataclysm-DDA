#pragma once
#ifndef COMPUTER_H
#define COMPUTER_H

#include <string>
#include <vector>

#include "calendar.h"

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
    COMPACT_OBSOLETE, // No longer used
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

struct computer_option {
    std::string name;
    computer_action action;
    int security;

    computer_option();
    computer_option( const std::string &N, computer_action A, int S );

    static computer_option from_json( const JsonObject &jo );
};

struct computer_failure {
    computer_failure_type type;

    computer_failure( computer_failure_type t ) : type( t ) {
    }

    static computer_failure from_json( const JsonObject &jo );
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
        void set_access_denied_msg( const std::string &new_msg );
        // Save/load
        std::string save_data() const;
        void load_data( const std::string &data );

        std::string name; // "Jon's Computer", "Lab 6E77-B Terminal Omega"
        int mission_id; // Linked to a mission?

        friend class computer_session;
    private:
        // Difficulty of simply logging in
        int security;
        // Date of next attempt
        time_point next_attempt = calendar::before_time_starts;
        // Things we can do
        std::vector<computer_option> options;
        // Things that happen if we fail a hack
        std::vector<computer_failure> failures;
        // Message displayed when the computer is secured and initial login fails.
        // Can be customized to for example warn the player of potentially lethal
        // consequences like secubots spawning.
        std::string access_denied;
        // Misc research notes from json
        static std::vector<std::string> lab_notes;

        void remove_option( computer_action action );
};

#endif
