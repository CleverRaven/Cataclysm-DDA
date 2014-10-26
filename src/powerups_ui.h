#ifndef POWERUPS_UI_H
#define POWERUPS_UI_H

#include <string>
#include <map>

void draw_powerups_titlebar(WINDOW *window, player *p, std::string menu_mode);
void draw_exam_window(WINDOW *win, int border_line, bool examination);

#endif
