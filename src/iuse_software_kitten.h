#ifndef SOFTWARE_KITTEN_H
#define SOFTWARE_KITTEN_H

#include "cursesdef.h"
#include "color.h"
#include <string>

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
        robot_finds_kitten( WINDOW *w );
        void instructions( WINDOW *w );
        void draw_robot( WINDOW *w );
        void draw_kitten( WINDOW *w );
        void process_input( int input, WINDOW *w );
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
