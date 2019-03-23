#include "char_validity_check.h"

#include <cctype>

/**
 * Returns whether or not the given (ASCII) character is usable.
 * Called to check player name validity and world name validity.
 * Only printable symbols not reserved by the filesystem are
 * permitted.
 * @param ch The char to check.
 * @return true if the char is allowed in a name, false if not.
 */
bool is_char_allowed( long ch )
{
    if( !std::isprint( ch ) && ch <= 127 ) {
        // above 127 are non-ASCII, therefore Unicode, therefore OK
        return false;
    }
    if( ch == '\\' || ch == '/' ) {
        // not valid in file names
        return false;
    }
    return true;
}
