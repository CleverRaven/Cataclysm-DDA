#pragma once
#ifndef COMPUTER_H
#define COMPUTER_H

#include <string>
#include <vector>

#include "calendar.h"

class JsonIn;
class JsonOut;
class JsonObject;

enum computer_action {
    COMPACT_NULL = 0,
    COMPACT_AMIGARA_LOG,
    COMPACT_AMIGARA_START,
    COMPACT_BLOOD_ANAL,
    COMPACT_CASCADE,
    COMPACT_COMPLETE_DISABLE_EXTERNAL_POWER, // Completes "Disable External Power" mission
    COMPACT_CONVEYOR,
    COMPACT_DATA_ANAL,
    COMPACT_DEACTIVATE_SHOCK_VENT,
    COMPACT_DISCONNECT,
    COMPACT_DOWNLOAD_SOFTWARE,
    COMPACT_ELEVATOR_ON,
    COMPACT_EMERG_MESS,
    COMPACT_EMERG_REF_CENTER,   //Points to the refugee center
    COMPACT_EXTRACT_RAD_SOURCE,
    COMPACT_GEIGER,
    COMPACT_IRRADIATOR,
    COMPACT_LIST_BIONICS,
    COMPACT_LOCK,
    COMPACT_MAP_SEWER,
    COMPACT_MAP_SUBWAY,
    COMPACT_MAPS,
    COMPACT_MISS_DISARM,
    COMPACT_OPEN,
    COMPACT_OPEN_DISARM,
    COMPACT_PORTAL,
    COMPACT_RADIO_ARCHIVE,
    COMPACT_RELEASE,
    COMPACT_RELEASE_BIONICS,
    COMPACT_RELEASE_DISARM,
    COMPACT_REPEATER_MOD,       //Converts a terminal in a radio station into a 'repeater', locks terminal and completes mission
    COMPACT_RESEARCH,
    COMPACT_SAMPLE,
    COMPACT_SHUTTERS,
    COMPACT_SR1_MESS,           //Security Reminders for Hazardous Waste Sarcophagus (SRCF)
    COMPACT_SR2_MESS,
    COMPACT_SR3_MESS,
    COMPACT_SR4_MESS,
    COMPACT_SRCF_1_MESS,
    COMPACT_SRCF_2_MESS,
    COMPACT_SRCF_3_MESS,
    COMPACT_SRCF_ELEVATOR,
    COMPACT_SRCF_SEAL,
    COMPACT_SRCF_SEAL_ORDER,
    COMPACT_TERMINATE,
    COMPACT_TOLL,
    COMPACT_TOWER_UNRESPONSIVE,
    COMPACT_UNLOCK,
    COMPACT_UNLOCK_DISARM,
    NUM_COMPUTER_ACTIONS
};

enum computer_failure_type {
    COMPFAIL_NULL = 0,
    COMPFAIL_ALARM,
    COMPFAIL_AMIGARA,
    COMPFAIL_DAMAGE,
    COMPFAIL_DESTROY_BLOOD,
    COMPFAIL_DESTROY_DATA,
    COMPFAIL_MANHACKS,
    COMPFAIL_PUMP_EXPLODE,
    COMPFAIL_PUMP_LEAK,
    COMPFAIL_SECUBOTS,
    COMPFAIL_SHUTDOWN,
    NUM_COMPUTER_FAILURES
};

struct computer_option {
    std::string name;
    computer_action action;
    int security;

    computer_option();
    computer_option( const std::string &N, computer_action A, int S );
    // Save to/load from saves
    void serialize( JsonOut &jout ) const;
    void deserialize( JsonIn &jin );
    // Load from data files
    static computer_option from_json( const JsonObject &jo );
};

struct computer_failure {
    computer_failure_type type;

    computer_failure();
    computer_failure( computer_failure_type t ) : type( t ) {
    }
    // Save to/load from saves
    void serialize( JsonOut &jout ) const;
    void deserialize( JsonIn &jin );
    // Load from data files
    static computer_failure from_json( const JsonObject &jo );
};

class computer
{
    public:
        computer( const std::string &new_name, int new_security );

        // Initialization
        void set_security( int Security );
        void add_option( const computer_option &opt );
        void add_option( const std::string &opt_name, computer_action action, int security );
        void add_failure( const computer_failure &failure );
        void add_failure( computer_failure_type failure );
        void set_access_denied_msg( const std::string &new_msg );
        void set_mission( int id );
        // Save/load
        void load_legacy_data( const std::string &data );
        void serialize( JsonOut &jout ) const;
        void deserialize( JsonIn &jin );

        friend class computer_session;
    private:
        // "Jon's Computer", "Lab 6E77-B Terminal Omega"
        std::string name;
        // Linked to a mission?
        int mission_id;
        // Difficulty of simply logging in
        int security;
        // Number of times security is tripped
        int alerts;
        // Date of next attempt
        time_point next_attempt;
        // Things we can do
        std::vector<computer_option> options;
        // Things that happen if we fail a hack
        std::vector<computer_failure> failures;
        // Message displayed when the computer is secured and initial login fails.
        // Can be customized to for example warn the player of potentially lethal
        // consequences like secubots spawning.
        std::string access_denied;

        void remove_option( computer_action action );
};

#endif
