#pragma once
#ifndef CATA_SRC_SYSTEM_LANGUAGE_H
#define CATA_SRC_SYSTEM_LANGUAGE_H

#include <string>
#include "optional.h"

// Functions to query system locale settings
namespace SystemLocale
{

// Detect system language, returns a supported game language code (eg. "fr"),
// or cata::nullopt if detection failed or system language is not supported by the game
cata::optional<std::string> Language();

// Detect whether current system locale uses metric system
// Returns cata::nullopt if detection fails
cata::optional<bool> UseMetricSystem();

} // namespace SystemLocale

#endif // CATA_SRC_SYSTEM_LANGUAGE_H
