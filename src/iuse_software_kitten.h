#pragma once
#ifndef CATA_SRC_IUSE_SOFTWARE_KITTEN_H
#define CATA_SRC_IUSE_SOFTWARE_KITTEN_H

#include <string>

#include "color.h"
#include "cursesdef.h"
#include "point.h"

class ui_adaptor;

struct kobject {
    point pos;
    nc_color color;
    int character = 0;
};

static constexpr int MAXMESSAGES = 1200;

class robot_finds_kitten
{
    public:
        bool ret = false;
        robot_finds_kitten();
    private:
        std::string getmessage( int idx ) const;
        void draw_robot() const;
        void draw_kitten() const;
        void show() const;
        void process_input();
        catacurses::window bkatwin;
        catacurses::window w;
        kobject robot;
        kobject kitten;
        kobject empty;
        static constexpr int numbogus = 20;
        kobject bogus[MAXMESSAGES];
        std::vector<std::string> bogus_messages;
        static constexpr int rfkLINES = 20;
        static constexpr int rfkCOLS = 60;
        int rfkscreen[rfkCOLS][rfkLINES];
        int nummessages = 0;

        enum class ui_state : int {
            instructions,
            main,
            invalid_input,
            bogus_message,
            end_animation,
            exit,
        };
        ui_state current_ui_state = ui_state::instructions;

        std::string this_bogus_message;
        int end_animation_frame = 0;
        static constexpr int num_end_animation_frames = 6;
        bool end_animation_last_input_left_or_up = false;
};

#endif // CATA_SRC_IUSE_SOFTWARE_KITTEN_H
