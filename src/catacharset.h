#pragma once
#ifndef CATA_SRC_CATACHARSET_H
#define CATA_SRC_CATACHARSET_H

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

constexpr int ANY_LENGTH = 5;
constexpr int NULL_UNICODE = 0x0000;
constexpr int PERCENT_SIGN_UNICODE = 0x0025;
constexpr int UNKNOWN_UNICODE = 0xFFFD;

class utf8_wrapper;

// get a Unicode character from a utf8 string
uint32_t UTF8_getch( const char **src, int *srclen );
inline uint32_t UTF8_getch( const std::string &str )
{
    const char *utf8str = str.c_str();
    int len = str.length();
    return UTF8_getch( &utf8str, &len );
}
// convert cursorx value to byte position
int cursorx_to_position( const char *line, int cursorx, int *prevpos = nullptr, int maxlen = -1 );
int utf8_width( std::string_view str, bool ignore_tags = false );
int utf8_width( const utf8_wrapper &str, bool ignore_tags = false );

std::string left_justify( const std::string &str, int width, bool ignore_tags = false );
std::string right_justify( const std::string &str, int width, bool ignore_tags = false );
std::string utf8_justify( const std::string &str, int width, bool ignore_tags = false );

/**
 * Center text inside whole line.
 * @param text to be centered.
 * @param start_pos printable position on line.
 * @param end_pos printable position on line.
 * @return First char position of centered text or start_pos if text is too big.
*/
int center_text_pos( const char *text, int start_pos, int end_pos );
int center_text_pos( const std::string &text, int start_pos, int end_pos );
int center_text_pos( const utf8_wrapper &text, int start_pos, int end_pos );
std::string utf32_to_utf8( uint32_t ch );
std::string utf8_truncate( const std::string &s, size_t length );

std::string base64_encode( const std::string &str );
std::string base64_decode( const std::string &str );

std::wstring utf8_to_wstr( const std::string &str );
std::string wstr_to_utf8( const std::wstring &wstr );

std::string wstr_to_native( const std::wstring &wstr );

std::string utf32_to_utf8( std::u32string_view str );
std::u32string utf8_to_utf32( std::string_view str );

// Split the given string into displayed characters.  Each element of the returned vector
// contains one 'regular' codepoint and all subsequent combining characters.
std::vector<std::string> utf8_display_split( const std::string & );
void utf8_display_split_into( const std::string &, std::vector<std::string_view> & );

/**
 * UTF8-Wrapper over std::string.
 * It looks and feels like a std::string, but uses code points counts
 * as index, not bytes.
 * A multi-byte Unicode character might be represented
 * as 3 bytes in UTF8, this class will see these 3 bytes as 1 character.
 * It will never separate them. It will however split between code points
 * which might be problematic when containing combination characters.
 * In this case use the *_display functions. They operate on the display width.
 * Code points with a zero width are considered to belong to the previous code
 * point and are not split from that.
 * Having a string with like [letter0][letter1][combination-mark][letter2]
 * (assuming each letter has a width of 1) will return a display_width of 3.
 * substr_display(1, 2) returns [letter1][combination-mark][letter2],
 * substr_display(1, 1) returns [letter1][combination-mark]
 * substr_display(2, 1) returns [letter2]
 *
 * Note: functions use code points, not bytes, for counting/indexing!
 * Functions with the _display suffix use display width for counting/indexing!
 * Protected functions might behave different
 *
 * For function documentation see std::string, the functions here
 * mimic the behavior of the equally named std::string function.
 */
class utf8_wrapper
{
    public:
        utf8_wrapper() : _length( 0 ), _display_width( 0 ) { }
        explicit utf8_wrapper( const std::string &d );
        explicit utf8_wrapper( const char *d );

        void insert( size_t start, const utf8_wrapper &other );
        utf8_wrapper substr( size_t start, size_t length ) const;
        utf8_wrapper substr( size_t start ) const {
            return substr( start, _length - start );
        }
        void erase( size_t start, size_t length );
        void erase( size_t start ) {
            erase( start, _length - start );
        }
        void append( const utf8_wrapper &other );
        utf8_wrapper &replace_all( const utf8_wrapper &search, const utf8_wrapper &replace );
        /**
         * Returns a substring based on the display width, not the number of
         * code points (as the other substr function does).
         * @param start Start the returned substring with the character that is
         * at that position when this string would be have been printed (rounded down).
         * E.g. a string "a´a´" (where a is a normal character, and ` is a combination
         * code point) would be displayed as two cells: "áá".
         * substr_display(0,2) would return the whole string, substr_display(0,1)
         * would return the first two code points, substr_display(1,1) would return the
         * last two code points.
         * @param length Display length of the returned string, the returned string can
         * have a shorter display length (especially if the last character is a multi-cell
         * character and including it would exceed the length parameter).
         */
        utf8_wrapper substr_display( size_t start, size_t length ) const;
        utf8_wrapper substr_display( size_t start ) const {
            return substr_display( start, _length - start );
        }

        utf8_wrapper &operator=( const std::string &d ) {
            *this = utf8_wrapper( d );
            return *this;
        }
        const std::string &str() const {
            return _data;
        }

        // Returns Unicode character at position start
        uint32_t at( size_t start ) const;

        // Returns number of Unicode characters
        size_t size() const {
            return _length;
        }
        size_t length() const {
            return size();
        }
        bool empty() const {
            return size() == 0;
        }
        // Display size might be different from length, as some characters
        // are displayed as 2 chars in a terminal
        size_t display_width() const {
            return _display_width;
        }
        const char *c_str() const {
            return _data.c_str();
        }
        /**
         * Return a substring at most maxlength width (display width).
         * If the string had to shortened, an ellipsis (...) is added. The
         * string with the ellipsis will be exactly maxlength displayed
         * characters.
         */
        std::string shorten( size_t maxlength ) const;
    protected:
        std::string _data;
        size_t _length;
        size_t _display_width;
        // Byte offset into @ref _data for codepoint at index start.
        // bstart is a initial offset (in bytes!). The function operates on
        // _data.substr(bstart), it ignores everything before bstart.
        size_t byte_start( size_t bstart, size_t start ) const;
        // Byte offset into @ref _date for the codepoint starting at displayed cell start,
        // if the first character occupies two cells, than byte_start_display(2)
        // would return the byte offset of the second codepoint
        // byte_start_display(1) and byte_start_display(0) would return 0
        size_t byte_start_display( size_t bstart, size_t start ) const;
        // Same as @ref substr, but with a byte index as start
        utf8_wrapper substr_byte( size_t bytestart, size_t length, bool use_display_width ) const;
        void init_utf8_wrapper();
};

/* A range that iterates through Unicode code points in a UTF-8 encoded string
 * without incurring dynamic memory allocation.
 *
 * Example:
 *   for( char32_t c : utf8_view( "..." ) ) {
 *       do_something_with( c );
 *   }
 */
class utf8_view
{
    private:
        const char *buffer;
        std::size_t length;

        class iterator
        {
            public:
                using iterator_category = std::input_iterator_tag;
                using difference_type = std::ptrdiff_t;
                using value_type = char32_t;
                using pointer = value_type*;
                using reference = value_type&;
            private:
                const char *ptr;
                const char *next_ptr;
                int remaining;
                int next_remaining;
                char32_t unicode;

                void decode() {
                    if( remaining > 0 ) {
                        unicode = UTF8_getch( &next_ptr, &next_remaining );
                    } else {
                        next_ptr = nullptr;
                        next_remaining = 0;
                        unicode = 0;
                    }
                }
            public:
                explicit iterator( const char *ptr, int remaining ) : ptr( ptr ), remaining( remaining ) {
                    next_ptr = ptr;
                    next_remaining = remaining;
                    decode();
                }
                bool operator != ( const iterator &rhs ) const noexcept {
                    return this->ptr != rhs.ptr;
                }
                const iterator &operator++() noexcept {
                    ptr = next_ptr;
                    remaining = next_remaining;
                    decode();
                    return *this;
                }
                char32_t operator*() const noexcept {
                    return unicode;
                }
        };

    public:
        explicit utf8_view( const std::string &str ) : buffer( str.c_str() ), length( str.length() ) {}

        iterator begin() const noexcept {
            return iterator( buffer, length );
        }

        iterator end() const noexcept {
            return iterator( buffer + length, 0 );
        }
};

#endif // CATA_SRC_CATACHARSET_H
