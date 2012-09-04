#ifndef _KEYPRESS_H_
#define _KEYPRESS_H_
#if (defined _WIN32 || defined WINDOWS)
	#include "catacurse.h"
#else
	#include <curses.h>
#endif

#include <string>

class game;

// Simple text input--translates numpad to vikeys
long input();
bool input_wait(char & ret_ch, int timeout);

// If ch is vikey, x & y are set to corresponding direction; ch=='y'->x=-1,y=-1
void get_direction(int &x, int &y, char ch);
// Uses the keymap to figure out direction properly
void get_direction(game *g, int &x, int &y, char ch);
std::string default_keymap_txt();

#define CTRL(n) (n - 'A' + 1 < 1 ? n - 'a' + 1 : n - 'A' + 1)
#define KEY_ESCAPE 27
#endif
