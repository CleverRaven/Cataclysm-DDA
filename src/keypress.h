#ifndef _KEYPRESS_H_
#define _KEYPRESS_H_

// Simple text input--translates numpad to vikeys
long input(long ch = -1);
bool input_wait(char & ret_ch, int timeout);
long get_keypress();
long convert_to_dialog_key(const long ch);

#define CTRL(n) (n - 'A' + 1 < 1 ? n - 'a' + 1 : n - 'A' + 1)
#define KEY_ESCAPE 27
#endif
