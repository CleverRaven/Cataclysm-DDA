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
bool is_char_allowed( int ch )
{
#if defined(_MSC_VER)
    // This hardcoded check and rejection of \t is in here because
    // std::isprint on Windows with MSVC has a bug where it considers
    // it a printable character.
    //
    // A Microsoft representative has this to say:
    // "Tab being reported as a print character by isprint() is a regression
    // in the Universal C Runtime that started appearing with the update
    // KB4338819. It will be fixed in a future servicing update, as
    // well as will be fixed in the next major version of Windows."
    //
    // That was from July 27, 2018. As of April 8, 2020 this is still
    // not functional.
    if( ch == 9 ) {
        return false;
    }
#endif

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
