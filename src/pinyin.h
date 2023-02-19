#pragma once
#ifndef CATA_SRC_PINYIN_H
#define CATA_SRC_PINYIN_H

#include <iostream>

namespace pinyin
{

/**
 * Convert a string to possible pinyins
 *
 * @param str A u32string of (possibly Chinese) characters
 * @param dest A vector to put the resulting pinyin combinations to
 */
void chinese_to_pinyin( const std::u32string &str, std::vector<std::u32string> &dest );

/**
 * Match a search string with pinyin of a given string
 *
 * @param str A u32string of (possibly Chinese) characters, to be matched against
 * @param qry A u32string of characters, the searching query
 *
 * @returns True if at least one possible pinyin combination of str contains qry
 */
bool pinyin_match( const std::u32string &str, const std::u32string &qry );

}
#endif //CATA_SRC_PINYIN_H
