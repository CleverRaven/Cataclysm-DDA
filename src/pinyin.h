#pragma once
#ifndef CATA_SRC_PINYIN_H
#define CATA_SRC_PINYIN_H

#include <string>

namespace pinyin
{

/**
 * Match a search string with pinyin of a given string
 *
 * @param str A u32string of (possibly Chinese) characters, to be matched against
 * @param qry A u32string of characters, the searching query
 *
 * @returns True if at least one possible pinyin combination of str contains qry
 */
bool pinyin_match( const std::u32string &str, const std::u32string &qry );

} // namespace pinyin
#endif // CATA_SRC_PINYIN_H
