#pragma once
#ifndef CATA_SRC_STRING_FORMATTER_H
#define CATA_SRC_STRING_FORMATTER_H

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <utility>

#include "demangle.h"

class cata_path;
class translation;

namespace cata
{

class string_formatter;

// wrapper to allow calling string_formatter::throw_error before the definition of string_formatter
[[noreturn]]
void throw_error( const string_formatter &, std::string_view );
// wrapper to access string_formatter::temp_buffer before the definition of string_formatter
const char *string_formatter_set_temp_buffer( const string_formatter &, std::string && );
// Handle currently active exception from string_formatter and return it as string
std::string handle_string_format_error();

/**
 * @defgroup string_formatter_convert Convert functions for @ref string_formatter
 *
 * The `convert` functions here are used to convert the input value of
 * @ref string_formatter::parse into the requested type, as defined by the format specifiers.
 *
 * @tparam T the input type, as given by the call to `string_format`.
 * @tparam RT the requested type. The `convert` functions return such a value or they throw
 * an exception via @ref throw_error.
 *
 * Each function has the same parameters:
 * First parameter defined the requested type. The value of the pointer is ignored, callers
 * should use a (properly casted) `nullptr`. It is required to "simulate" overloading the
 * return value. E.g. `long convert(long*, int)` and `short convert(short*, int)` both convert
 * a input value of type `int`, but the first converts to `long` and the second converts to
 * `short`. Without the first parameters their signature would be identical.
 * The second parameter is used to call @ref throw_error / @ref string_formatter_set_temp_buffer.
 * The third parameter is the input value that is to be converted.
 * The fourth parameter is a dummy value, it is always ignored, callers should use `0` here.
 * It is used so the fallback with the variadic arguments is *only* chosen when no other
 * overload matches.
 */
/**@{*/
// Test for arithmetic type, *excluding* bool. printf can not handle bool, so can't we.
template<typename T>
constexpr bool is_numeric =
    std::is_arithmetic_v<std::decay_t<T>>
    && !std::is_same_v<std::decay_t<T>, bool>;
// Test for integer type (not floating point, not bool).
template<typename T>
constexpr bool is_integer =
    is_numeric<T>
    && !std::is_floating_point_v<std::decay_t<T>>;
template<typename T>
constexpr bool is_char = std::is_same_v<std::decay_t<T>, char>;
// Test for std::string type.
template<typename T>
constexpr bool is_string = std::is_same_v<std::decay_t<T>, std::string>;
// Test for std::string_view type.
template<typename T>
constexpr bool is_string_view = std::is_same_v<std::decay_t<T>, std::string_view>;
// Test for c-string type.
template<typename T>
constexpr bool is_cstring =
    std::is_same_v<std::decay_t<T>, const char *> || std::is_same_v<std::decay_t<T>, char *>;
// Test for class translation
template<typename T>
constexpr bool is_translation = std::is_same_v<std::decay_t<T>, translation>;
// Test for class cata_path
template<typename T>
constexpr bool is_cata_path = std::is_same_v<std::decay_t<T>, cata_path>;

template<typename RT, typename T>
inline std::enable_if_t < is_integer<RT> &&is_integer<T>, RT >
convert( RT *, const string_formatter &, T &&value, int )
{
    return value;
}
template<typename RT, typename T>
inline std::enable_if_t < is_integer<RT> &&std::is_enum_v<std::decay_t<T>>, RT >
        convert( RT *, const string_formatter &, T &&value, int )
{
    return static_cast<RT>( value );
}
template<typename RT, typename T>
inline std::enable_if_t < std::is_floating_point_v<RT> &&is_numeric<T>
&& !is_integer<T>, RT >
convert( RT *, const string_formatter &, T &&value, int )
{
    return value;
}
template<typename RT, typename T>
inline std::enable_if_t < std::is_same_v<RT, void *>
&&std::is_pointer_v<std::decay_t<T>>, void * >
                                  convert( RT *, const string_formatter &, T &&value, int )
{
    return const_cast<std::remove_const_t<std::remove_pointer_t<std::decay_t<T>>> *>( value );
}
template<typename RT, typename T>
inline std::enable_if_t < std::is_same_v<RT, const char *> &&is_string<T>, const char * >
convert( RT *, const string_formatter &, T &&value, int )
{
    return value.c_str();
}
template<typename RT, typename T>
inline std::enable_if_t < std::is_same_v<RT, const char *> &&is_string_view<T>, const char * >
convert( RT *, const string_formatter &sf, T &&value, int )
{
    return string_formatter_set_temp_buffer( sf, std::string( value ) );
}
template<typename RT, typename T>
inline std::enable_if_t < std::is_same_v<RT, const char *> &&is_cstring<T>, const char * >
convert( RT *, const string_formatter &, T &&value, int )
{
    return value;
}
template<typename RT, typename T>
inline std::enable_if_t < std::is_same_v<RT, const char *> &&is_translation<T>, const char * >
convert( RT *, const string_formatter &sf, T &&value, int )
{
    return string_formatter_set_temp_buffer( sf, value.translated() );
}
template<typename RT, typename T>
inline std::enable_if_t < std::is_same_v<RT, const char *> &&is_cata_path<T>, const char * >
convert( RT *, const string_formatter &sf, T &&value, int )
{
    return string_formatter_set_temp_buffer( sf, value.generic_u8string() );
}
template<typename RT, typename T>
inline std::enable_if_t <
std::is_same_v<RT, const char *> &&is_numeric<T> &&
!is_char<T>, const char * >
convert( RT *, const string_formatter &sf, T &&value, int )
{
    return string_formatter_set_temp_buffer( sf, std::to_string( value ) );
}
template<typename RT, typename T>
inline std::enable_if_t < std::is_same_v<RT, const char *> &&is_char<T>, const char * >
convert( RT *, const string_formatter &sf, T &&value, int )
{
    return string_formatter_set_temp_buffer( sf, std::string( 1, value ) );
}
// Catch all remaining conversions (the '...' makes this the lowest overload priority).
// The static_assert is used to restrict the input type to those that can actually be printed,
// calling `string_format` with an unknown type will trigger a compile error because no other
// `convert` function will match, while this one will give a static_assert error.
template<typename RT, typename T>
// NOLINTNEXTLINE(cert-dcl50-cpp)
inline RT convert( RT *, const string_formatter &sf, T &&, ... )
{
    static_assert( std::is_pointer_v<std::decay_t<T>> ||
                   is_numeric<T> || is_string<T> || is_string_view<T> ||
                   is_char<T> || std::is_enum_v<std::decay_t<T>> ||
                   is_cstring<T> || is_translation<T> || is_cata_path<T>, "Unsupported argument type" );
    throw_error( sf, "Tried to convert argument of type " +
                 demangle( typeid( T ).name() ) + " to " +
                 demangle( typeid( RT ).name() ) + ", which is not possible" );
}
/**@}*/

/**
 * Type-safe and undefined-behavior free wrapper over `sprintf`.
 * See @ref string_format for usage.
 * Basically it extracts the format specifiers and calls `sprintf` for each one separately
 * and with proper conversion of the input type.
 * For example `printf("%f", 7)` would yield undefined behavior as "%f" requires a `double`
 * as argument. This class detects the format specifier and converts the input to `double`
 * before calling `sprintf`. Similar for `printf("%d", "foo")` (yields UB again), but this
 * class will just throw an exception.
 */
// Note: argument index is always 0-based *in this code*, but `printf` has 1-based arguments.
class string_formatter
{
    private:
        /// Complete format string, including all format specifiers (the string passed
        /// to @ref printf).
        const std::string_view format;
        /// Used during parsing to denote the *next* character in @ref format to be
        /// parsed.
        size_t current_index_in_format = 0;
        /// The formatted output string, filled during parsing of @ref format,
        /// so it's only valid after the parsing has completed.
        std::string output;
        /// The *currently parsed format specifiers. This is extracted from @ref format
        /// during parsing and given to @ref sprintf (along with the actual argument).
        /// It is filled and reset during parsing for each format specifier in @ref format.
        std::string current_format;
        /// The *index* (not number) of the next argument to be formatted via @ref current_format.
        int current_argument_index = 0;
        /// Return the next character from @ref format and increment @ref current_index_in_format.
        /// Returns a null-character when the end of the @ref format has been reached (and does not
        /// change @ref current_index_in_format).
        char consume_next_input();
        /// Returns (like @ref consume_next_input) the next character from @ref format, but
        /// does *not* change @ref current_index_in_format.
        char get_current_input() const;
        /// If the next character to read from @ref format is the given character, consume it
        /// (like @ref consume_next_input) and return `true`. Otherwise don't do anything at all
        /// and return `false`.
        bool consume_next_input_if( char c );
        /// Return whether @ref get_current_input has a decimal digit ('0'...'9').
        bool has_digit() const;
        /// Consume decimal digits, interpret them as integer and return it.
        /// A starting '0' is allowed. Leaves @ref format at the first non-digit
        /// character (or the end). Returns 0 if the first character is not a digit.
        int parse_integer();
        /// Read and consume format flag characters and append them to @ref current_format.
        /// Leaves @ref format at the first character that is not a flag (or the end).
        void read_flags();
        /// Read and forward to @ref current_format any width specifier from @ref format.
        /// Returns nothing if the width is not specified or if it is specified as fixed number,
        /// otherwise returns the index of the printf-argument to be used for the width.
        std::optional<int> read_width();
        /// See @ref read_width. This does the same, but for the precision specifier.
        std::optional<int> read_precision();
        /// Read and return the index of the printf-argument that is to be formatted. Returns
        /// nothing if @ref format does not refer to a specific index (caller should use
        /// @ref current_argument_index).
        std::optional<int> read_argument_index();
        // Helper for common logic in @ref read_width and @ref read_precision.
        std::optional<int> read_number_or_argument_index();
        /// Throws an exception containing the given message and the @ref format.
        [[noreturn]]
        void throw_error( std::string_view msg ) const;
        friend void throw_error( const string_formatter &sf, std::string_view msg ) {
            sf.throw_error( msg );
        }
        mutable std::string temp_buffer;
        /// Stores the given text in @ref temp_buffer and returns `c_str()` of it. This is used
        /// for printing non-strings through "%s". It *only* works because this prints each format
        /// specifier separately, so the content of @ref temp_buffer is only used once.
        friend const char *string_formatter_set_temp_buffer( const string_formatter &sf,
                std::string &&text ) {
            sf.temp_buffer = std::move( text );
            return sf.temp_buffer.c_str();
        }
        /**
         * Extracts a printf argument from the argument list and converts it to the requested type.
         * @tparam RT The type that the argument should be converted to.
         * @tparam current_index The index of the first of the supplied arguments.
         * @throws If there is no argument with the given index, or if the argument can not be
         * converted to the requested type (via @ref convert).
         */
        /**@{*/
        template<typename RT, unsigned int current_index>
        RT get_nth_arg_as( const unsigned int requested ) const {
            throw_error( "Requested argument " + std::to_string( requested ) + " but input has only " +
                         std::to_string( current_index ) );
        }
        template<typename RT, unsigned int current_index, typename T, typename ...Args>
        RT get_nth_arg_as( const unsigned int requested, T &&head, Args &&... args ) const {
            if( requested > current_index ) {
                return get_nth_arg_as < RT, current_index + 1 > ( requested, std::forward<Args>( args )... );
            } else {
                return convert( static_cast<RT *>( nullptr ), *this, std::forward<T>( head ), 0 );
            }
        }
        /**@}*/

        void add_long_long_length_modifier();

        template<typename ...Args>
        void read_conversion( const int format_arg_index, Args &&... args ) {
            // Removes the prefix "ll", "l", "h" and "hh", "z", and "t".
            // We later add "ll" again and that
            // would interfere with the existing prefix. We convert *all* input to (un)signed
            // long long int and use the "ll" modifier all the time. This will print the
            // expected value all the time, even when the original modifier did not match.
            if( consume_next_input_if( 'l' ) ) {
                consume_next_input_if( 'l' );
            } else if( consume_next_input_if( 'h' ) ) {
                consume_next_input_if( 'h' );
            } else if( consume_next_input_if( 'z' ) ) { // NOLINT(bugprone-branch-clone)
                // done with it
            } else if( consume_next_input_if( 't' ) ) {
                // done with it
            }
            const char c = consume_next_input();
            current_format.push_back( c );
            switch( c ) {
                case 'c':
                    return do_formatting( get_nth_arg_as<int, 0>( format_arg_index, std::forward<Args>( args )... ) );
                case 'd':
                case 'i':
                    add_long_long_length_modifier();
                    return do_formatting( get_nth_arg_as<signed long long int, 0>( format_arg_index,
                                          std::forward<Args>( args )... ) );
                case 'o':
                case 'u':
                case 'x':
                case 'X':
                    add_long_long_length_modifier();
                    return do_formatting( get_nth_arg_as<unsigned long long int, 0>( format_arg_index,
                                          std::forward<Args>( args )... ) );
                case 'a':
                case 'A':
                case 'g':
                case 'G':
                case 'f':
                case 'F':
                case 'e':
                case 'E':
                    return do_formatting( get_nth_arg_as<double, 0>( format_arg_index,
                                          std::forward<Args>( args )... ) );
                case 'p':
                    return do_formatting( get_nth_arg_as<void *, 0>( format_arg_index,
                                          std::forward<Args>( args )... ) );
                case 's':
                    return do_formatting( get_nth_arg_as<const char *, 0>( format_arg_index,
                                          std::forward<Args>( args )... ) );
                default:
                    throw_error( "Unsupported format conversion: " + std::string( 1, c ) );
            }
        }

        template<typename T>
        void do_formatting( T &&value ) {
            output.append( raw_string_format( current_format.c_str(), value ) );
        }

    public:
        /// @param format The format string as required by `sprintf`.
        explicit string_formatter( std::string_view format ) : format( format ) { }
        /// Does the actual `sprintf`. It uses @ref format and puts the formatted
        /// string into @ref output.
        /// Note: use @ref get_output to get the formatted string after a successful
        /// call to this function.
        /// @throws Exceptions when the arguments do not match the format specifiers,
        /// see @ref get_nth_arg_as, or when the format is invalid for whatever reason.
        /// Note: @ref string_format is a wrapper that handles those exceptions.
        template<typename ...Args>
        void parse( Args &&... args ) {
            output.reserve( format.size() );
            output.resize( 0 );
            current_index_in_format = 0;
            current_argument_index = 0;
            while( const char c = consume_next_input() ) {
                if( c != '%' ) {
                    output.push_back( c );
                    continue;
                }
                if( consume_next_input_if( '%' ) ) {
                    output.push_back( '%' );
                    continue;
                }
                current_format = "%";
                const std::optional<int> format_arg_index = read_argument_index();
                read_flags();
                if( const std::optional<int> width_argument_index = read_width() ) {
                    const int w = get_nth_arg_as<int, 0>( *width_argument_index, std::forward<Args>( args )... );
                    current_format += std::to_string( w );
                }
                if( const std::optional<int> precision_argument_index = read_precision() ) {
                    const int p = get_nth_arg_as<int, 0>( *precision_argument_index, std::forward<Args>( args )... );
                    current_format += std::to_string( p );
                }
                const int arg = format_arg_index ? *format_arg_index : current_argument_index++;
                read_conversion( arg, std::forward<Args>( args )... );
            }
        }
        std::string get_output() const {
            return output;
        }
#if defined(__clang__)
#define PRINTF_LIKE(a,b) __attribute__((format(printf,a,b)))
#elif defined(__GNUC__)
#define PRINTF_LIKE(a,b) __attribute__((format(gnu_printf,a,b)))
#else
#define PRINTF_LIKE(a,b)
#endif

        // A stupid thing happens in certain situations. On Windows, when using clang, the PRINTF_LIKE
        // macro expands to something containing the token printf, which might be defined to libintl_printf,
        // which is not a valid __attribute__ name. To prevent that we use an *MSVC* pragma which gcc and clang
        // support to temporarily suppress expanding printf to libintl_printf so the attribute applies correctly.
#pragma push_macro("printf")
#undef printf
        /**
         * Wrapper for calling @ref vsprintf - see there for documentation. Try to avoid it as it's
         * not type safe and may easily lead to undefined behavior - use @ref string_format instead.
         * @throws std::exception if the format is invalid / does not match the arguments, but that's
         * not guaranteed - technically it's undefined behavior.
         */
        // Implemented in output.cpp
        static std::string raw_string_format( const char *format, ... ) PRINTF_LIKE( 1, 2 );
#pragma pop_macro("printf")

#undef PRINTF_LIKE
};

} // namespace cata

/**
 * Simple wrapper over @ref string_formatter::parse. It catches any exceptions and returns
 * some error string. Otherwise it just returns the formatted string.
 *
 * These functions perform string formatting according to the rules of the `printf` function,
 * see `man 3 printf` or any other documentation.
 *
 * In short: the \p format parameter is a string with optional placeholders, which will be
 * replaced with formatted data from the further arguments. The further arguments must have
 * a type that matches the type expected by the placeholder.
 * The placeholders look like this:
 * - `%s` expects an argument of type `const char*` or `std::string` or numeric (which is
 *   converted to a string via `to_string`), which is inserted as is.
 * - `%d` expects an argument of an integer type (int, short, ...), which is formatted as
 *   decimal number.
 * - `%f` expects a numeric argument (integer / floating point), which is formatted as
 *   decimal number.
 *
 * There are more placeholders and options to them (see documentation of `printf`).
 * Note that this wrapper (via @ref string_formatter) automatically converts the arguments
 * to match the given format specifier (if possible) - see @ref string_formatter_convert.
 */
/**@{*/
template<typename ...Args>
inline std::string string_format( std::string_view format, Args &&...args )
{
    try {
        cata::string_formatter formatter( format );
        formatter.parse( std::forward<Args>( args )... );
        return formatter.get_output();
    } catch( ... ) {
        return cata::handle_string_format_error();
    }
}
template<typename T, typename ...Args>
inline std::enable_if_t<cata::is_translation<T>, std::string>
string_format( T &&format, Args &&...args )
{
    return string_format( format.translated(), std::forward<Args>( args )... );
}
/**@}*/

#endif // CATA_SRC_STRING_FORMATTER_H
