#pragma once
#ifndef CATA_SRC_TRANSLATION_GENDERED_H
#define CATA_SRC_TRANSLATION_GENDERED_H

#include <map>
#include <string>
#include <vector>

void reset_sanity_check_genders();

using GenderMap = std::map<std::string, std::vector<std::string>>;
/**
 * Translation with a gendered context
 *
 * Similar to pgettext, but the context is a collection of genders.
 * @param genders A map where each key is a subject name (a string which should
 * make sense to the translator in the context of the line to be translated)
 * and the corresponding value is a list of potential genders for that subject.
 * The first gender from the list of genders for the current language will be
 * chosen for each subject (or the language default if there are no genders in
 * common).
 */
std::string gettext_gendered( const GenderMap &genders, const std::string &msg );

#endif // CATA_SRC_TRANSLATION_GENDERED_H
