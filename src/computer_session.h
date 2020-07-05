#pragma once
#ifndef CATA_SRC_COMPUTER_SESSION_H
#define CATA_SRC_COMPUTER_SESSION_H

#include <map>
#include <string>
#include <utility>
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
        // Output window. This class assumes win's size does not change.
        catacurses::window win;
        const int left;
        const int top;
        const int width;
        const int height;
        std::vector<std::pair<int, std::string>> lines;

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
        enum class ynq : int {
            yes,
            no,
            quit,
        };
        template<typename ...Args>
        ynq query_ynq( const std::string &text, Args &&... args );
        // Same as query_ynq, but returns true for y or Y
        template<typename ...Args>
        bool query_bool( const std::string &text, Args &&... args );
        // Simply wait for any key, returns True
        template<typename ...Args>
        bool query_any( const std::string &text, Args &&... args );
        // Wait for any key without new output
        bool query_any();

        void action_amigara_log();
        void action_amigara_start();
        void action_blood_anal();
        void action_cascade();
        void action_complete_disable_external_power();
        void action_conveyor();
        void action_data_anal();
        void action_deactivate_shock_vent();
        void action_disconnect();
        void action_download_software();
        void action_elevator_on();
        void action_emerg_mess();
        void action_emerg_ref_center();
        void action_extract_rad_source();
        void action_geiger();
        void action_irradiator();
        void action_list_bionics();
        void action_lock();
        void action_map_sewer();
        void action_map_subway();
        void action_maps();
        void action_miss_disarm();
        void action_open();
        void action_open_disarm();
        void action_portal();
        void action_radio_archive();
        void action_release();
        void action_release_bionics();
        void action_release_disarm();
        void action_repeater_mod();
        void action_research();
        void action_sample();
        void action_shutters();
        void action_sr1_mess();
        void action_sr2_mess();
        void action_sr3_mess();
        void action_sr4_mess();
        void action_srcf_1_mess();
        void action_srcf_2_mess();
        void action_srcf_3_mess();
        void action_srcf_elevator();
        void action_srcf_seal();
        void action_srcf_seal_order();
        void action_terminate();
        void action_toll();
        void action_tower_unresponsive();
        void action_unlock();
        void action_unlock_disarm();
        static const std::map<computer_action, void( computer_session::* )()> computer_action_functions;

        void failure_alarm();
        void failure_amigara();
        void failure_damage();
        void failure_destroy_blood();
        void failure_destroy_data();
        void failure_manhacks();
        void failure_pump_explode();
        void failure_pump_leak();
        void failure_secubots();
        void failure_shutdown();
        static const std::map<computer_failure_type, void( computer_session::* )()>
        computer_failure_functions;
};

#endif // CATA_SRC_COMPUTER_SESSION_H
