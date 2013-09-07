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

#include "keypress.h"
#include "output.h"
#include "catacharset.h"
#include "options.h"
#include "debug.h"
#include "iuse_software_kitten.h"

#define EMPTY -1
#define ROBOT 0
#define KITTEN 1
#define MAXMESSAGES 1200
std::string robot_finds_kitten::getmessage(int idx) {
char rfimessages[MAXMESSAGES][81] =
{
    "This is not kitten.",
    "That's just an old tin can.",
    "It's an altar to the horse god.",
    "A mere collection of pixels.",
    "A box of fumigation pellets.",
    "More grist for the mill.",
    "It's a square.",
    "Run away!  Run away!",
    "The rothe hits!  The rothe hits!",
    "This place is called Antarctica. There is no kitten here.",
    "It's a copy of \"Zen and The Art of Robot Maintenance\".",
    "\"Yes!\" says the bit.",
    "\"No!\" says the bit.",
    "A robot comedian. You feel amused.",
    "A forgotten telephone switchboard.",
    "It's a desperate plug for Cymon's Games, http://www.cymonsgames.com/",
    "The letters O and R.",
    "\"Blup, blup, blup\" says the mud pot.",
    "Another rabbit? That's three today!",
    "Thar's Mobius Dick, the convoluted whale. Arrr!",
    "This object here appears to be Louis Farrakhan's bow tie.",
    "...thingy???",
    "Pumpkin pie spice.",
    "Chewing gum and baling wire.",
    "It's the crusty exoskeleton of an arthropod!",
  };
  if (idx < 0 || idx >= nummessages) {
     return std::string("It is SOFTWARE BUG.");
  } else {
   return std::string(rfimessages[idx]);
  }
};

robot_finds_kitten::robot_finds_kitten(WINDOW *w)
{
    ret = false;
    char ktile[83] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!#&()*+./:;=?![]{|}y";
    int used_messages[MAXMESSAGES];

    LINES = 20;
    COLS = 60;

    const int numbogus = 20;
    const int maxcolor = 15;
    nummessages = 25;
    empty.x = -1;
    empty.y = -1;
    empty.color = (nc_color)0;
    empty.character = ' ';
    for (int c = 0; c < COLS; c++) {
        for (int c2 = 0; c2 < LINES; c2++) {
            rfkscreen[c][c2] = EMPTY;
        }
    }
    /* Create an array to ensure we don't get duplicate messages. */
    for (int c = 0; c < nummessages; c++) {
        used_messages[c] = 0;
        bogus_messages[c] = 0;
        bogus[c] = empty;
    }
    /* Now we initialize the various game OBJECTs.
       * Assign a position to the player. */
    robot.x = rand() % COLS;
    robot.y = rand() % (LINES - 3) + 3;
    robot.character = '#';
    robot.color = int_to_color(1);
    rfkscreen[robot.x][robot.y] = ROBOT;

    /* Assign the kitten a unique position. */
    do {
        kitten.x = rand() % COLS;
        kitten.y = rand() % (LINES - 3) + 3;
    } while (rfkscreen[kitten.x][kitten.y] != EMPTY);

    /* Assign the kitten a character and a color. */
    do {
        kitten.character = ktile[rand() % 82];
    } while (kitten.character == '#' || kitten.character == ' ');
    kitten.color = int_to_color( ( rand() % (maxcolor - 2) ) + 2);
    rfkscreen[kitten.x][kitten.y] = KITTEN;

    /* Now, initialize non-kitten OBJECTs. */
    for (int c = 0; c < numbogus; c++) {
        /* Assign a unique position. */
        do {
            bogus[c].x = rand() % COLS;
            bogus[c].y = (rand() % (LINES - 3)) + 3;
        } while (rfkscreen[bogus[c].x][bogus[c].y] != EMPTY);
        rfkscreen[bogus[c].x][bogus[c].y] = c + 2;

        /* Assign a character. */
        do {
            bogus[c].character = ktile[rand() % 82];
        } while (bogus[c].character == '#' || bogus[c].character == ' ');
        bogus[c].color = int_to_color((rand() % (maxcolor - 2)) + 2);

        /* Assign a unique message. */
        int index = 0;
        do {
            index = rand() % nummessages;
        } while (used_messages[index] != 0);
        bogus_messages[c] = index;
        used_messages[index] = 1;
    }

    instructions(w);

    werase(w);
    mvwprintz (w, 0, 0, c_white, "robotfindskitten v22July2008");
    for (int c = 0; c < COLS; c++) {
        mvwputch (w, 2, c, c_white, '_');
    }
    wmove(w, kitten.y, kitten.x);
    draw_kitten(w);

    for (int c = 0; c < numbogus; c++) {
        mvwputch(w, bogus[c].y, bogus[c].x, bogus[c].color, bogus[c].character);
    }

    wmove(w, robot.y, robot.x);
    draw_robot(w);
    int old_x = robot.x;
    int old_y = robot.y;

    wrefresh(w);
    /* Now the fun begins. */
    int input = '.';
    input = getch();

    while (input != 'q') {
        process_input(input, w);
        if(ret == true) {
            break;
        }
        /* Redraw robot, where avaliable */
        if (!(old_x == robot.x && old_y == robot.y)) {
            wmove(w, old_y, old_x);
            wputch(w, c_white, ' ');
            wmove(w, robot.y, robot.x);
            draw_robot(w);
            rfkscreen[old_x][old_y] = EMPTY;
            rfkscreen[robot.x][robot.y] = ROBOT;
            old_x = robot.x;
            old_y = robot.y;
            wrefresh(w);
        }
        input = getch();
    }
}
void robot_finds_kitten::instructions(WINDOW *w) {
    	mvwprintw (w,0,0,"robotfindskitten v22July2008\n"
    "Originally by the illustrious Leonard Richardson 1997\n"
    "ReWritten in PDCurses by Joseph Larson,\n"
    "Ported to CDDA gaming system by a nutcase.\n"
    "   In this game, you are robot (");
	draw_robot(w);
	wprintw (w,"). Your job is to find kitten. This task\n"
    "is complicated by the existance of various things which are not kitten.\n"
    "Robot must touch items to determine if they are kitten or not. The game\n"
    "ends when robotfindskitten. Alternatively, you may end the game\n"
    "by hitting 'q'\n"
    "   Press any key to start.\n");
    wrefresh(w);
	getch();

}

void robot_finds_kitten::process_input(int input, WINDOW *w)

{
    timespec ts;
    ts.tv_sec = 1;
    ts.tv_nsec = 0;

    int check_x = robot.x;
    int check_y = robot.y;

    switch (input) {
        case KEY_UP: /* up */
            check_y--;
            break;
        case KEY_DOWN: /* down */
            check_y++;
            break;
        case KEY_LEFT: /* left */
            check_x--;
            break;
        case KEY_RIGHT: /* right */
            check_x++;
            break;
        case 0:
            break;
        default: { /* invalid command */
            for (int c = 0; c < COLS; c++) {
                mvwputch (w, 0, c, c_white, ' ');
                mvwputch (w, 1, c, c_white, ' ');
            }
            mvwprintz (w, 0, 0, c_white, "Invalid command: Use direction keys or Press 'q'.");
            return;
        }
    }

    if (check_y < 3 || check_y > LINES - 1 || check_x < 0 || check_x > COLS - 1) {
        return;
    }

    if (rfkscreen[check_x][check_y] != EMPTY) {
        switch (rfkscreen[check_x][check_y]) {
            case ROBOT:
                /* We didn't move. */
                break;
            case KITTEN: {/* Found it! */
                for (int c = 0; c < COLS; c++) {
                    mvwputch (w, 0, c, c_white, ' ');
                }

                /* The grand cinema scene. */
                for (int c = 0; c <= 3; c++) {

                    wmove(w, 1, (COLS / 2) - 5 + c);
                    wputch(w, c_white, ' ');
                    wmove(w, 1, (COLS / 2) + 4 - c);
                    wputch(w, c_white, ' ');
                    wmove(w, 1, (COLS / 2) - 4 + c);
                    if (input == KEY_LEFT || input == KEY_UP) {
                        draw_kitten(w);
                    } else {
                        draw_robot(w);
                    }
                    wmove(w, 1, (COLS / 2) + 3 - c);
                    if (input == KEY_LEFT || input == KEY_UP) {
                        draw_robot(w);
                    } else {
                        draw_kitten(w);
                    }
                    wrefresh (w);
                    nanosleep(&ts, NULL);
                }

                /* They're in love! */
                mvwprintz(w, 0, ((COLS - 6) / 2) - 1, c_ltred, "<3<3<3");
                wrefresh(w);
                nanosleep(&ts, NULL);
                for (int c = 0; c < COLS; c++) {
                    mvwputch (w, 0, c, c_white, ' ');
                    mvwputch (w, 1, c, c_white, ' ');
                }
                mvwprintz (w, 0, 0, c_white, "You found kitten! Way to go, robot!");
                wrefresh(w);
                ret = true;
                getch ();
            }
            break;

            default: {
                for (int c = 0; c < COLS; c++) {
                    mvwputch (w, 0, c, c_white, ' ');
                    mvwputch (w, 1, c, c_white, ' ');
                }
                mvwprintw (w, 0, 0, "%s", getmessage(bogus_messages[rfkscreen[check_x][check_y] - 2]).c_str());
                wrefresh(w);
            }
            break;
        }
        wmove(w, 2, 0);
        return;
    }
    /* Otherwise, move the robot. */
    robot.x = check_x;
    robot.y = check_y;
}
/////////////////////////////////
void robot_finds_kitten::draw_robot(WINDOW *w)  /* Draws robot at current position */
{
    wputch(w, robot.color, robot.character);
}

void robot_finds_kitten::draw_kitten(WINDOW *w)  /* Draws kitten at current position */
{
    wputch(w, kitten.color, kitten.character);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
