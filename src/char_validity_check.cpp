#include "char_validity_check.h"

/**
 * Returns whether or not the given (ASCII) character is usable.
 * Called to check player name validity and world name validity.
 * Only printable symbols not reserved by the filesystem are
 * permitted.
 * @param ch The char to check.
 * @return true if the char is allowed in a name, false if not.
 */
bool is_char_allowed(char ch) {

  //Allow everything EXCEPT the following reserved characters:
  return (ch > 31 //0-31 are control characters
          && ch < 127 //DEL character
          && ch != '/' && ch != '\\' //Path separators
          && ch != '?' && ch != '*' && ch != '%' //Wildcards
          && ch != ':' //Mount point/drive marker
          && ch != '|' //Output pipe
          && ch != '"' //Filename (with spaces) marker
          && ch != '>' && ch != '<'); //Input/output redirection

}
