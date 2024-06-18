#pragma once
#ifndef CATA_SRC_UNICODE_H
#define CATA_SRC_UNICODE_H

#include <cstdint>

/**
 * Used temporarily for determining line breaking opportunities. Ultimatedly this
 * should be changed to use the data from https://unicode.org/Public/UNIDATA/LineBreak.txt
 * for strict unicode conformance.
 */
bool is_cjk_or_emoji( uint32_t ch );

/*
 * Convert a Unicode code point to lowercase in-place, e.g. Ö -> ö
 *
 * Cannot rely on std::towlower(), because on some platforms it only converts A-Z to a-z and does nothing for
 * accented latin or cyrillic letters, so we have to rebuild the wheel on our own.
 *
 * This ad-hoc implementation is not complete, it only covers some of the latin and cyrillic character ranges.
 * TODO: Remove this if adding icu4c dependency is allowed at some point in the future.
 */
void u32_to_lowercase( char32_t &ch );

// Remove accent from a letter in-place, e.g. ö -> o
// TODO: Remove this if adding icu4c dependency is allowed at some point in the future.
void remove_accent( char32_t &ch );

#endif // CATA_SRC_UNICODE_H
