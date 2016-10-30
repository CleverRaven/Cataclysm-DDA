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
        // Remnants of @opening_screen that I'll get rid of as I continue the refactoring process:
        WINDOW *w_open;
        WINDOW *w_background;
        int extra_w, iMenuOffsetX, iMenuOffsetY;
        int sel1, sel2, sel3, layer;
        std::vector<std::string> savegames, templates;
        std::vector<std::string> vMenuItems;
        std::vector< std::vector<std::string> > vMenuHotkeys;

        // functions for handling some of the tabs. Returns whether a game was started or not.
        bool new_character_tab();
        bool load_character_tab();
        bool world_tab();
        bool special_tab();
        bool settings_tab();

        // ASCII art that says "Cataclysm Dark Days Ahead"
        std::vector<std::string> mmenu_title;
        std::vector<std::string> mmenu_motd;
        std::vector<std::string> mmenu_credits;

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

        std::vector<std::string> load_file( const std::string &path, const std::string &alternative_text ) const;
        void mmenu_refresh_title();
        void mmenu_refresh_credits();
        void mmenu_refresh_motd();
        std::vector<std::string> get_hotkeys( const std::string &s );

        void display_credits();
};

#endif

