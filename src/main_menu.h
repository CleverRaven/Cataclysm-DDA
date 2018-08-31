#pragma once
#ifndef MAIN_MENU_H
#define MAIN_MENU_H

class player;

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
        std::vector<std::string> mmenu_motd;
        std::vector<std::string> mmenu_credits;
        std::vector<std::string> vMenuItems; // MOTD, New Game, Load Game, etc.
        std::vector<std::string> vWorldSubItems;
        std::vector< std::vector<std::string> > vWorldHotkeys;
        std::vector<std::string> vSettingsSubItems;
        std::vector< std::vector<std::string> > vSettingsHotkeys;
        std::vector< std::vector<std::string> > vMenuHotkeys; // hotkeys for the vMenuItems

        /**
         * Does what it sounds like, but this function also exists in order to gracefully handle
         * the case where the player goes to the 'Settings' tab and changes the language.
        */
        void init_strings();
        /** Helper function for @ref init_strings */
        std::vector<std::string> load_file( const std::string &path,
                                            const std::string &alt_text ) const;
        /** Another helper function for @ref init_strings */
        std::vector<std::string> get_hotkeys( const std::string &s );


        // Play a sound whenever the user moves left or right in the main menu or its tabs
        void on_move() const;

        // Tab functions. They return whether a game was started or not. The ones that can never
        // start a game have a void return type.
        bool new_character_tab();
        bool load_character_tab();
        void world_tab();

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
        int iMenuOffsetX = 0;
        int iMenuOffsetY = 0;
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
         * @param iOffsetY Offset of menu items, y coordinate
         * @param iOffsetX Offset of menu items, x coordinate
         * @param spacing: How many spaces to print between each menu item
         */
        void print_menu_items( const catacurses::window &w_in, std::vector<std::string> vItems, size_t iSel,
                               int iOffsetY, int iOffsetX, int spacing = 1 );

        /**
         * Called by @ref opening_screen, this prints all the text that you see on the main menu
         *
         * @param w_open Window to print menu in
         * @param iSel which index in vMenuItems is selected
         * @param iMenuOffsetX Menu location in window, x coordinate
         * @param iMenuOffsetY Menu location in window, y coordinate
         * @param bShowDDA Whether to show "Dark Days Ahead" banner
         */
        void print_menu( const catacurses::window &w_open, int iSel, const int iMenuOffsetX,
                         int iMenuOffsetY, bool bShowDDA = true );

        void display_credits();

        void init_windows();
        std::string handle_input_timeout( input_context &ctxt );
};

#endif

