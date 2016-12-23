#ifndef MAIN_MENU_H
#define MAIN_MENU_H

class player;

#include <string>
#include <vector>

#include "cursesdef.h"
#include "input.h"

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


        // Play a sound whenver the user moves left or right in the main menu or its tabs
        void on_move() const;

        // Tab functions. They return whether a game was started or not.
        bool new_character_tab();
        bool load_character_tab();

        // These variables are shared between @opening_screen and the tab functions.
        // TODO: But this is an ugly short-term solution.
        input_context ctxt;
        int sel1 = 1, sel2 = 1, sel3 = 1, layer = 1;
        WINDOW *w_open;
        WINDOW *w_background;
        int iMenuOffsetX = 0;
        int iMenuOffsetY;
        std::vector<std::string> templates;
        int extra_w;
        std::vector<std::string> savegames;

        /**
         * Prints a horizontal list of options
         *
         * @param iSel: Which index of vItems is selected. This menu item will be highlighted to
         *              make it stand out from the other menu items.
         * @param spacing: How many spaces to print between each menu item
        */
        void print_menu_items( WINDOW *w_in, std::vector<std::string> vItems, size_t iSel,
                               int iOffsetY, int iOffsetX, int spacing = 1 );

        /**
         * Called by @opening_screen, this prints all the text that you see on the main menu
         *
         * @param iSel: which index in @vMenuItems is selected
        */
        void print_menu( WINDOW *w_open, int iSel, const int iMenuOffsetX, int iMenuOffsetY,
                         bool bShowDDA = true );

        void display_credits();
};

#endif

