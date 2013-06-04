#ifndef _KEYPRESS_H_
#define _KEYPRESS_H_

#include <string>
#include "cursesdef.h"

class game;

// Simple text input--translates numpad to vikeys
long input(long ch = -1);
bool input_wait(char & ret_ch, int timeout);

// Uses the keymap to figure out direction properly
void get_direction(game *g, int &x, int &y, char ch);
std::string default_keymap_txt();

#define CTRL(n) (n - 'A' + 1 < 1 ? n - 'a' + 1 : n - 'A' + 1)
#define KEY_ESCAPE 27
#endif
