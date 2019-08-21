#pragma once
#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#include <cstddef>
#include <string>
#include <vector>

#include "cursesdef.h"
#include "input.h"
#include "worldfactory.h"

class main_menu
{
    public:
        main_menu() : ctxt( "MAIN_MENU" ) { }
        // Shows the main menu and returns whether a game was started or not
        bool opening_screen();

    private:
        // ASCII art that says "Cataclysm Dark Days Ahead"
        std::vector<std::string> mmenu_title;
        std::string mmenu_motd;
        std::string mmenu_credits;
        std::vector<std::string> vMenuItems; // MOTD, New Game, Load Game, etc.
        std::vector<std::string> vWorldSubItems;
        std::vector< std::vector<std::string> > vWorldHotkeys;
        std::vector<std::string> vSettingsSubItems;
        std::vector< std::vector<std::string> > vSettingsHotkeys;
        std::vector< std::vector<std::string> > vMenuHotkeys; // hotkeys for the vMenuItems
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

        // Flag to be set when first entering an error condition, cleared when leaving it
        // Used to prevent error sound from playing repeatedly at input polling rate
        bool errflag = false;
        // Play a sound *once* when an error occurs in the main menu or its tabs; sets errflag
        void on_error();
        // Clears errflag
        void clear_error();

        // Tab functions. They return whether a game was started or not. The ones that can never
        // start a game have a void return type.
        bool new_character_tab();
        bool load_character_tab();
        void world_tab();

        /*
         * Load character templates from template folder
         */
        void load_char_templates();

        // These variables are shared between @opening_screen and the tab functions.
        // TODO: But this is an ugly short-term solution.
        input_context ctxt;
        int sel1 = 1;
        int sel2 = 1;
        int sel3 = 1;
        int layer = 1;
        int LAST_TERMX = 0;
        int LAST_TERMY = 0;
        catacurses::window w_open;
        catacurses::window w_background;
        point menu_offset;
        std::vector<std::string> templates;
        int extra_w;
        std::vector<save_t> savegames;

        /**
         * Prints a horizontal list of options
         *
         * @param w_in Window we are printing in
         * @param vItems Main menu items
         * @param iSel Which index of vItems is selected. This menu item will be highlighted to
         * make it stand out from the other menu items.
         * @param offset Offset of menu items
         * @param spacing: How many spaces to print between each menu item
         */
        void print_menu_items( const catacurses::window &w_in,
                               const std::vector<std::string> &vItems, size_t iSel,
                               point offset, int spacing = 1 );

        /**
         * Called by @ref opening_screen, this prints all the text that you see on the main menu
         *
         * @param w_open Window to print menu in
         * @param iSel which index in vMenuItems is selected
         * @param offset Menu location in window
         */
        void print_menu( const catacurses::window &w_open, int iSel, const point &offset );

        void display_text( const std::string &text, const std::string &title, int &selected );

        void init_windows();
        std::string handle_input_timeout( input_context &ctxt );

        std::string halloween_spider();
        std::string halloween_graves();
};

#endif

