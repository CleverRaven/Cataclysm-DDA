#pragma once
#ifndef CATA_SRC_LANGUAGE_H
#define CATA_SRC_LANGUAGE_H

#include <string>
#include <vector>

struct language_info {
    std::string id;
    std::string name;
    std::string locale;

    /**
     * List of grammatical genders. Default should be first.
     *
     * Use short names and try to be consistent between languages as far as
     * possible.  Current choices are m (male), f (female), n (neuter).
     * As appropriate we might add e.g. a (animate) or c (common).
     * New genders must be added to all_genders in lang/extract_json_strings.py
     * and src/language.cpp.
     * The primary purpose of this is for NPC dialogue which might depend on
     * gender.  Only add genders to the extent needed by such translations.
     * They are likely only needed if they affect the first and second
     * person.  For example, one gender suffices for English even though
     * third person pronouns differ.
     *
     * Defaults to `[ "n" ]` if left emty.
     */
    std::vector<std::string> genders;
    std::string osx;
    std::vector<int> lcids;
};

bool init_language_system();
void prompt_select_lang_on_startup();
const std::vector<language_info> &list_available_languages();
void set_language();
const language_info &get_language();

#endif // CATA_SRC_LANGUAGE_H
