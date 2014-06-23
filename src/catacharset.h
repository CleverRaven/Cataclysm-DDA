#ifndef CATACHARSET_H
#define CATACHARSET_H
#include <stdint.h>
#include <string>
#define ANY_LENGTH 5
#define UNKNOWN_UNICODE 0xFFFD

// get a unicode character from a utf8 string
unsigned UTF8_getch(const char **src, int *srclen);
// from wcwidth.c, return "cell" width of a unicode char
int mk_wcwidth(uint32_t ucs);
// convert cursorx value to byte position
int cursorx_to_position(const char *line, int cursorx, int *prevppos = NULL, int maxlen = -1);
//erease for characters insertion
int erease_utf8_by_cw( char *t, int cw, int len, int maxlen);
int utf8_width(const char *s);
std::string utf8_substr(std::string s, int start, int size = -1);
std::string utf32_to_utf8(unsigned ch);
std::string utf8_truncate(std::string s, size_t length);

std::string base64_encode(std::string str);
std::string base64_decode(std::string str);

/**
 * UTF8-Wrapper over std::string.
 * It looks and feels like a std::string, but uses character counts
 * as index, not bytes.
 * A multi-byte Unicode character might by represented
 * as 3 bytes in UTF8, this class will see these 3 bytes as 1 character.
 * It will never separate them.
 *
 * Note: all functions use character counts, not byte counts!
 *
 * For function documentation see std::string, the functions here
 * mimic the behavior of the equally named std::string function.
 */
class utf8_wrapper {
public:
    utf8_wrapper() : _data(), _length(0), _display_width(0) { }
    utf8_wrapper(const std::string &d);
    utf8_wrapper(const char *d) : utf8_wrapper(std::string(d)) { }

    void insert(size_t start, const utf8_wrapper &other);
    utf8_wrapper substr(size_t start, size_t length) const;
    utf8_wrapper substr(size_t start) const { return substr(start, _length - start); }
    void erase(size_t start, size_t length);
    void erase(size_t start) { erase(start, _length - start); }
    void append(const utf8_wrapper &other);

    utf8_wrapper &operator=(const std::string &d) { return *this = utf8_wrapper(d); }
    const std::string &str() const { return _data; }

    // Returns unicode character at position start
    long at(size_t start) const;

    // Returns number of unicode characters
    size_t size() const { return _length; }
    size_t length() const { return size(); }
    bool empty() const { return size() == 0; }
    // Display size might be different from length, as some characters
    // are displayed as 2 chars in a terminal
    size_t display_width() const { return _display_width; }
    const char *c_str() const { return _data.c_str(); }
    /**
     * Return a substring at most maxlength width (display width).
     * If the string had to shortened, an ellipsis (...) is added. The
     * string with the ellipsis will be exactly maxlength displayed
     * characters.
     */
    std::string shorten(size_t maxlength) const;

    /**
     * Insert a single utf8-character ch at given position into the
     * string. Uses repeated calls to @ref getch if the input ch indicates
     * a multibyte character.
     * This is supposed to be used like this:
     * <code>
     * utf8_wrapper name(...);
     * ... handle the other input of some_input_context ...
     * if (action == "ANY_INPUT) {
     *   long ch = some_input_context.get_raw_input().get_first_input();
     *   name.insert_from_getch(start, ch);
     * }
     * </code>
     * @param ch The initial result from @ref getch.
     * @return false if the initial input ch is a non-printable control
     * character (< 32, except tab, newline and carriage return) or if
     * the input (including the result of further calls to getch) forms
     * an invalid UTF-8 sequence. Otherwise true.
     * In any a return value of true means a character has been added to
     * the string, a value of false means no character has been added.
     */
    bool insert_from_getch(size_t start, long ch);
protected:
    std::string _data;
    size_t _length;
    size_t _display_width;
    // Byte offset into @ref _data for unicode character at index start.
    size_t byte_start(size_t start) const;
    // Same as @ref substr, but with a byte index as start
    utf8_wrapper substr_byte(size_t bytestart, size_t length) const;
};

#endif
