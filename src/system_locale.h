#pragma once
#ifndef CATA_SRC_SYSTEM_LANGUAGE_H
#define CATA_SRC_SYSTEM_LANGUAGE_H

#include <optional>
#include <string>

// Functions to query system locale settings
namespace SystemLocale
{

// Detect system language, returns a supported game language code (eg. "fr"),
// or std::nullopt if detection failed or system language is not supported by the game
std::optional<std::string> Language();

// Detect whether current system locale uses metric system
// Returns std::nullopt if detection fails
std::optional<bool> UseMetricSystem();

} // namespace SystemLocale

#endif // CATA_SRC_SYSTEM_LANGUAGE_H
