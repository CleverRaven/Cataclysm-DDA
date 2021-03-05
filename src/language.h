#pragma once
#ifndef CATA_SRC_LANGUAGE_H
#define CATA_SRC_LANGUAGE_H

#include <string>
#include <vector>

/**
 * Contains information on a language supported by the game.
 *
 * Fields here mirror JSON fields in the language definitions file.
 */
struct language_info {
    /**
     * Language id.
     *
     * Usually in form of `ll` or `ll_CC`
     * (ll - language, CC - country code).
     *
     * Used for autodetection on Linux, and also for resolving paths to
     * localized files (title screen ascii art, lisst of character/town names).
     */
    std::string id;

    /**
     * Native language name.
     */
    std::string name;

    /**
     * Locale to use on non-Windows platforms.
     *
     * If missing, falls back to `en_US.UTF-8`, then to user locale, then to C.
     * If this language is used as 'System language', this field is ignored
     * and the game uses user locale (or, if failed, C).
     *
     * On Windows, always uses user locale with code page 1252.
     */
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

    /**
     * OSX autodetection correction (optional field).
     *
     * Allows the code to handle special case for Simplified/Traditional Chinese.
     * Simplified/Traditional is actually denoted by the region code in older
     * iterations of the language codes, whereas now (at least on OS X)
     * region is distinct. That is, the game expects 'zh_CN' but OS X might
     * give 'zh-Hans-CN'.
     */
    std::string osx;

    /**
     * List of language code identifiers (LCIDs) for Windows lang autodetection.
     *
     * See `Windows Language Code Identifier (LCID) Reference`
     * https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-lcid/70feba9f-294e-491e-b6eb-56532684c37f
     */
    std::vector<int> lcids;
};

/**
 * Initialize language system (detect system UI language, load definitions).
 * Does not change current language/locale.
 */
bool init_language_system();

/**
 * Prompt for explicit language selection.
 * The prompt is skipped if using lang other than 'System language',
 * or if lang autodetection succeeded.
 */
void prompt_select_lang_on_startup();

/**
 * List of loaded language definitions.
 * Always contains at least 1 language (English).
 */
const std::vector<language_info> &list_available_languages();

/**
 * Set language and locale to current value of USE_LANG option.
 * If the value is empty, uses autodetected system language
 * (or English, if autodetection failed) and system locale.
 */
void set_language();

/**
 * Current game language. May differ from USE_LANG option value
 * if the option has been changed without subsequent `set_language()` call.
 * If lang system has not been initialized, falls back to English.
 */
const language_info &get_language();

#endif // CATA_SRC_LANGUAGE_H
