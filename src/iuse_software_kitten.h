#pragma once
#ifndef SOFTWARE_KITTEN_H
#define SOFTWARE_KITTEN_H

#include <string>

#include "color.h"

namespace catacurses
{
class window;
} // namespace catacurses

struct kobject {
    int x;
    int y;
    nc_color color;
    int character;
};

#define MAXMESSAGES 1200

class robot_finds_kitten
{
    public:
        bool ret;
        std::string getmessage( int idx );
        robot_finds_kitten( const catacurses::window &w );
        void instructions( const catacurses::window &w );
        void draw_robot( const catacurses::window &w );
        void draw_kitten( const catacurses::window &w );
        void process_input( int input, const catacurses::window &w );
        kobject robot;
        kobject kitten;
        kobject empty;
        kobject bogus[MAXMESSAGES];
        int rfkscreen[60][20];
        int nummessages;
        int bogus_messages[MAXMESSAGES];
        int rfkLINES;
        int rfkCOLS;
};

#endif
