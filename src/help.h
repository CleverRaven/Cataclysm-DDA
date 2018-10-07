#pragma once
#ifndef HELP_H
#define HELP_H

#include <string>
#include <map>
#include <vector>
#include "cursesdef.h"
#include "input.h"

class JsonIn;

class help
{
    public:
        help() {};

        void load();
        void draw_menu( const catacurses::window &win );
        void display_help();

    private:
        void deserialize( JsonIn &jsin );
        std::string get_note_colors();

        std::map<int, std::pair<std::string, std::vector<std::string> > > help_texts;
        std::vector< std::vector<std::string> > hotkeys;

        input_context ctxt;
};

help &get_help();

std::string get_hint();

#endif
