#pragma once
#ifndef CATA_SRC_LANGUAGE_H
#define CATA_SRC_LANGUAGE_H

#include <string>
#include <vector>

struct language_info {
    std::string id;
    std::string name;
    std::string locale;
};

const std::vector<language_info> &list_available_languages();
bool isValidLanguage( const std::string &lang );
std::string getLangFromLCID( const int &lcid );
void select_language();
void set_language();
void update_global_locale();

#endif // CATA_SRC_LANGUAGE_H
