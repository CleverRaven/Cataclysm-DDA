#pragma once
#ifndef CATA_SRC_MAIN_MENU_H
#define CATA_SRC_MAIN_MENU_H

#include <cstddef>
#include <iosfwd>
#include <vector>

#include "cuboid_rectangle.h"
#include "cursesdef.h"
#include "enums.h"
#include "input.h"
#include "point.h"
#include "worldfactory.h"

class main_menu
{
    public:
        main_menu() : ctxt( "MAIN_MENU", keyboard_mode::keychar ) { }
        // Shows the main menu and returns whether a game was started or not
        bool opening_screen();

        static std::string queued_world_to_load;
        static std::string queued_save_id_to_load;
    private:
        // ASCII art that says "Cataclysm Dark Days Ahead"
        std::vector<std::string> mmenu_title;
        std::string mmenu_motd;
        std::string mmenu_credits;
        int mmenu_motd_len;
        int mmenu_credits_len;
        std::vector<std::string> vMenuItems; // MOTD, New Game, Load Game, etc.
        std::vector<std::string> vWorldSubItems;
        std::vector<std::string> vNewGameSubItems;
        std::vector<std::string> vNewGameHints;
        std::vector< std::vector<std::string> > vWorldHotkeys;
        std::vector<std::string> vSettingsSubItems;
        std::vector< std::vector<std::string> > vSettingsHotkeys;
        std::vector< std::vector<std::string> > vMenuHotkeys; // hotkeys for the vMenuItems
        std::vector< std::vector<std::string> > vNewGameHotkeys;
        std::string vdaytip; //tip of the day

        /**
         * Does what it sounds like, but this function also exists in order to gracefully handle
         * the case where the player goes to the 'Settings' tab and changes the language.
        */
        void init_strings();
        /** Helper function for @ref init_strings */
        std::vector<std::string> load_file( const std::string &path,
                                            const std::string &alt_text ) const;

        // Play a sound whenever the user moves left or right in the main menu or its tabs
        void on_move() const;

        // Play a sound *once* when an error occurs in the main menu or its tabs
        void on_error();

        // Tab functions. They return whether a game was started or not. The ones that can never
        // start a game have a void return type.
        bool load_game( std::string const &worldname, save_t const &savegame );
        bool new_character_tab();
        bool load_character_tab( const std::string &worldname );
        void world_tab( const std::string &worldname );

        /*
         * Load character templates from template folder
         */
        void load_char_templates();

        // These variables are shared between @opening_screen and the tab functions.
        // TODO: But this is an ugly short-term solution.
        input_context ctxt;
        int sel1 = 1;
        int sel2 = 1;
        size_t last_world_pos = 0;
        int sub_opt_off = 0;
        point LAST_TERM;
        catacurses::window w_open;
        point menu_offset;
        std::vector<std::string> templates;
        int extra_w = 0;
        std::vector<save_t> savegames;
        std::vector<std::pair<inclusive_rectangle<point>, std::pair<int, int>>> main_menu_sub_button_map;
        std::vector<std::pair<inclusive_rectangle<point>, int>> main_menu_button_map;

        /**
         * Prints a horizontal list of options
         *
         * @param w_in Window we are printing in
         * @param vItems Main menu items
         * @param iSel Which index of vItems is selected. This menu item will be highlighted to
         * make it stand out from the other menu items.
         * @param offset Offset of menu items
         * @param spacing: How many spaces to print between each menu item
         * @returns A list of horizontal offsets, one for each menu item
         */
        std::vector<int> print_menu_items( const catacurses::window &w_in,
                                           const std::vector<std::string> &vItems, size_t iSel,
                                           point offset, int spacing = 1, bool main = false );

        /**
         * Called by @ref opening_screen, this prints all the text that you see on the main menu
         *
         * @param w_open Window to print menu in
         * @param iSel which index in vMenuItems is selected
         * @param offset Menu location in window
         */
        void print_menu( const catacurses::window &w_open, int iSel, const point &offset, int sel_line );

        void display_text( const std::string &text, const std::string &title, int &selected );

        void display_sub_menu( int sel, const point &bottom_left, int sel_line );

        void init_windows();

        holiday get_holiday_from_time();

        holiday current_holiday = holiday::none;

        static std::string halloween_spider();
        std::string halloween_graves();
};

#endif // CATA_SRC_MAIN_MENU_H

