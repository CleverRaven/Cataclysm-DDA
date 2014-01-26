#ifndef _HELP_H_
#define _HELP_H_
#include "output.h"
#include "json.h"
#include <string>
#include <vector>

void help_main     (WINDOW *win);
void help_movement (WINDOW *win);
void help_driving  (WINDOW *win);
void help_map      (WINDOW *win);

std::vector<std::string> text_introduction();
std::vector<std::string> text_viewing();
std::vector<std::string> text_hunger();
std::vector<std::string> text_pain();
std::vector<std::string> text_addiction();
std::vector<std::string> text_morale();
std::vector<std::string> text_mutation();
std::vector<std::string> text_bionics();
std::vector<std::string> text_crafting();
std::vector<std::string> text_traps();
std::vector<std::string> text_items();
std::vector<std::string> text_combat();
std::vector<std::string> text_styles();
std::vector<std::string> text_tips();
std::vector<std::string> text_types();
std::vector<std::string> text_guns();
std::vector<std::string> text_faq();

void display_help();

void load_hint(JsonObject &jsobj);
void clear_hints();
std::string get_hint(); // return a random hint about the game

#endif // _HELP_H_
