#ifndef _SOFTWARE_KITTEN_H_
#define _SOFTWARE_KITTEN_H_

#include <string>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator>
#include <map>
#include <vector>

struct kobject
{
    int x;
    int y;
    nc_color color;
    int character;
};

#define MAXMESSAGES 1200

class robot_finds_kitten {
public:
    bool ret;
    std::string getmessage(int idx);
    robot_finds_kitten(WINDOW * w);
    void instructions(WINDOW * w);
    void draw_robot(WINDOW * w);
    void draw_kitten(WINDOW * w);
    void process_input(int input, WINDOW * w);
    kobject robot;
    kobject kitten;
    kobject empty;
    kobject bogus[1200];
    int rfkscreen[60][20];
    int nummessages;
int bogus_messages[1200];
int rfkLINES;
int rfkCOLS;
};
#endif
