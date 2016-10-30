#ifndef MAIN_MENU_H
#define MAIN_MENU_H

class player;

#include <string>
#include <vector>

#include "cursesdef.h"

class main_menu
{
    public:
        // Shows the main menu and returns whether a game was started or not
        bool opening_screen();

    private:
        // ASCII art that says "Cataclysm Dark Days Ahead"
        std::vector<std::string> mmenu_title;
        std::vector<std::string> mmenu_motd;
        std::vector<std::string> mmenu_credits;
        std::vector<std::string> vMenuItems; // MOTD, New Game, Load Game, etc.
        std::vector< std::vector<std::string> > vMenuHotkeys; // hotkeys for the vMenuItems

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

        std::vector<std::string> load_file( const std::string &path,
                                            const std::string &alt_text ) const;
        void mmenu_refresh_title();
        void mmenu_refresh_credits();
        void mmenu_refresh_motd();
        std::vector<std::string> get_hotkeys( const std::string &s );

        void display_credits();
};

#endif

