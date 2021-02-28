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

bool init_language_system();
void prompt_select_lang_on_startup();
const std::vector<language_info> &list_available_languages();
void set_language();
const language_info &get_language();

#endif // CATA_SRC_LANGUAGE_H
