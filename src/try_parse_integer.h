#pragma once
#ifndef CATA_SRC_TRY_PARSE_INTEGER_H
#define CATA_SRC_TRY_PARSE_INTEGER_H

#include "ret_val.h"

/**
 * Convert a string to an integer or provide an error message indicating what
 * went wrong in trying to do so.
 *
 * @param use_locale Whether to use a number-parsing function that might permit
 * formats specific to the user's locale.  Generally would be true for
 * player-input values, and false for values from e.g. game data files.
 */
template<typename T>
ret_val<T> try_parse_integer( const std::string &, bool use_locale );

extern template ret_val<int> try_parse_integer<int>( const std::string &, bool use_locale );
extern template ret_val<long> try_parse_integer<long>( const std::string &, bool use_locale );
extern template ret_val<long long> try_parse_integer<long long>(
    const std::string &, bool use_locale );

#endif // CATA_SRC_TRY_PARSE_INTEGER_H
