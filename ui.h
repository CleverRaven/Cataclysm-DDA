#ifndef _UI_H_
#define _UI_H_

#include "output.h"
#include <sstream>
#include <stdlib.h>
#include "cursesdef.h"

const int UIMENU_INVALID=-1024;
const nc_color C_UNSET_MASK=c_red_red;
const int MENU_ALIGN_LEFT=-1;
const int MENU_ALIGN_CENTER=0;
const int MENU_ALIGN_RIGHT=1;
const int MENU_WIDTH_ENTRIES=-2;
const int MENU_AUTOASSIGN=-1;

struct mvwzstr {
    int left;
    nc_color color;
    std::string txt;
};

struct uimenu_entry {
    int retval;           // return this int
    bool enabled;         // darken, and forbid scrolling if hilight_disabled is false
    int hotkey;           // keycode from (int)getch(). -1: automagically pick first free character: 1-9 a-z A-Z
    std::string txt;      // what it says on the tin
    nc_color hotkey_color;
    nc_color text_color;
    mvwzstr extratxt;
    uimenu_entry(std::string T) { retval = -1; enabled=true; hotkey=-1; txt=T;text_color=C_UNSET_MASK;};
    uimenu_entry(std::string T, int K) { retval = -1; enabled=true; hotkey=K; txt=T; text_color=C_UNSET_MASK; };
    uimenu_entry(int R, bool E, int K, std::string T) : retval(R), enabled(E), hotkey(K), txt(T) {text_color=C_UNSET_MASK;};
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
    std::vector<std::string> textformatted;
    int textwidth;
    int textalign;
    std::string title;
    std::vector<uimenu_entry> entries;
    std::map<int, int> keymap;    
    bool border;
    nc_color border_color;
    nc_color text_color;
    nc_color title_color;
    nc_color hilight_color;
    nc_color hotkey_color;
    nc_color disabled_color;
    int pad_left; int pad_right;
    bool return_invalid;
    bool hilight_disabled;
    bool hilight_full;
    int shift_retval;
    int vshift;
    int vmax;
    uimenu(); // bare init

    uimenu(bool cancancel, const char * message, ...); // legacy menu()
    uimenu(bool cancelable, const char *mes, std::vector<std::string> options); // legacy menu_vec

    uimenu (int startx, int width, int starty, std::string title, std::vector<uimenu_entry> ents);

    void init();
    void setup();
    void show();
    void query(bool loop=true);
    void addentry(std::string str);
    void addentry(const char *format, ...);
    void addentry(int r, bool e, int k, std::string str);
    void addentry(int r, bool e, int k, const char *format, ...);
    void settext(std::string str);
    void settext(const char *format, ...);
    ~uimenu ();
    operator int() const;

  private:
    bool started;
};



#endif
