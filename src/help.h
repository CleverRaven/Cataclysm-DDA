#pragma once
#ifndef HELP_H
#define HELP_H

#include <map>
#include <string>
#include <vector>

#include "cursesdef.h"
#include "input.h"

class JsonIn;

class help
{
    public:
        void load();
        void display_help();

    private:
        void deserialize( JsonIn &jsin );
        void draw_menu( const catacurses::window &win );
        std::string get_note_colors();
        std::string get_dir_grid();

        std::map<int, std::pair<std::string, std::vector<std::string> > > help_texts;
        std::vector< std::vector<std::string> > hotkeys;

        input_context ctxt;
};

help &get_help();

std::string get_hint();

#endif
