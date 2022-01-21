#pragma once
#ifndef CATA_SRC_SYSTEM_LANGUAGE_H
#define CATA_SRC_SYSTEM_LANGUAGE_H

#include <string>

// Detect system language, returns a supported game language code (eg. "fr"),
// or empty string if detection failed or system language is not supported by the game
std::string getSystemLanguage();

// Same as above but returns "en" in the case that the above one returns empty string
std::string getSystemLanguageOrEnglish();

#endif // CATA_SRC_SYSTEM_LANGUAGE_H
