#pragma once
#ifndef HELP_H
#define HELP_H

#include <string>

class JsonObject;

struct help_category {
    std::string name;
    std::string hotkey;

    help_category( const std::string &p_name = "", const std::string &p_hotkey = "" ) {
        name = p_name;
        hotkey = p_hotkey;
    }
};

void load_help_category( JsonObject &jsobj );
void load_help_text( JsonObject &jsobj );

void display_help();

std::string get_hint(); // return a random hint about the game

#endif
