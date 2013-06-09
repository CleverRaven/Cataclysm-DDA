#ifndef _UI_H_
#define _UI_H_

#include "map.h"
#include "output.h"
#include "game.h"
#include "item.h"
#include <sstream>
#include <stdlib.h>
#include "cursesdef.h"

const int UIMENU_INVALID=-1024;

struct uimenu_entry {
  int retval;           // return this int
  bool enabled;         // darken, and forbid scrolling if hilight_disabled is false
  int hotkey;           // keycode from (int)getch(). -1: automagically pick first free character: 1-9 a-z A-Z
  std::string txt;      // what it says on the tin
  uimenu_entry(std::string T) { retval = -1; enabled=true; hotkey=-1; txt=T;};
  uimenu_entry(int R, bool E, int K, std::string T) : retval(R), enabled(E), hotkey(K), txt(T) {};
};

class uimenu {
  public:
    int w_x;
    int w_y;
    int w_width;
    int w_height;
    WINDOW *window;
    int ret;
    int selected;
    int keypress;
    std::string text;
    std::vector<uimenu_entry> entries;
    std::map<int, int> keymap;    
    bool border;
    nc_color border_color;
    nc_color text_color;
    nc_color hilight_color;
    nc_color hotkey_color;
    nc_color disabled_color;
    int pad_left; int pad_right;
    bool return_invalid;
    bool hilight_disabled;
    bool hilight_full;

    uimenu();
    uimenu(bool cancancel, const char * message, ...);
    uimenu (int startx, int width, int starty, std::string title, std::vector<uimenu_entry> ents);
    void init();
    void show();
    void query(bool loop=true);
    ~uimenu ();
    operator int() const;

  private:
    bool started;
};



#endif
