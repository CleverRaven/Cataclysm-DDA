#pragma once
#ifndef STRING_FORMATTER_H
#define STRING_FORMATTER_H

#include <cstddef>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>

// needed for the workaround for the std::to_string bug in some compilers
#include "compatibility.h" // IWYU pragma: keep
// TODO: replace with std::optional
#include "optional.h"

class translation;

namespace cata
{

class string_formatter;

// wrapper to allow calling string_formatter::throw_error before the definition of string_formatter
[[noreturn]]
void throw_error( const string_formatter &, const std::string & );
// wrapper to access string_formatter::temp_buffer before the definition of string_formatter
const char *string_formatter_set_temp_buffer( const string_formatter &, const std::string & );
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
using is_numeric = typename std::conditional <
                   std::is_arithmetic<typename std::decay<T>::type>::value &&
                   !std::is_same<typename std::decay<T>::type, bool>::value, std::true_type, std::false_type >::type;
// Test for integer type (not floating point, not bool).
template<typename T>
using is_integer = typename std::conditional < is_numeric<T>::value &&
                   !std::is_floating_point<typename std::decay<T>::type>::value, std::true_type,
                   std::false_type >::type;
template<typename T>
using is_char = typename
                std::conditional<std::is_same<typename std::decay<T>::type, char>::value, std::true_type, std::false_type>::type;
// Test for std::string type.
template<typename T>
using is_string = typename
                  std::conditional<std::is_same<typename std::decay<T>::type, std::string>::value, std::true_type, std::false_type>::type;
// Test for c-string type.
template<typename T>
using is_cstring = typename std::conditional <
                   std::is_same<typename std::decay<T>::type, const char *>::value ||
                   std::is_same<typename std::decay<T>::type, char *>::value, std::true_type, std::false_type >::type;
// Test for class translation
template<typename T>
using is_translation = typename std::conditional <
                       std::is_same<typename std::decay<T>::type, translation>::value, std::true_type,
                       std::false_type >::type;

template<typename RT, typename T>
inline typename std::enable_if < is_integer<RT>::value &&is_integer<T>::value,
       RT >::type convert( RT *, const string_formatter &, T &&value, int )
{
    return value;
}
template<typename RT, typename T>
inline typename std::enable_if < is_integer<RT>::value
&&std::is_enum<typename std::decay<T>::type>::value,
RT >::type convert( RT *, const string_formatter &, T &&value, int )
{
    return static_cast<RT>( value );
}
template<typename RT, typename T>
inline typename std::enable_if < std::is_floating_point<RT>::value &&is_numeric<T>::value
&&!is_integer<T>::value, RT >::type convert( RT *, const string_formatter &, T &&value, int )
{
    return value;
}
template<typename RT, typename T>
inline typename std::enable_if < std::is_same<RT, void *>::value
&&std::is_pointer<typename std::decay<T>::type>::value, void * >::type convert( RT *,
        const string_formatter &, T &&value, int )
{
    return const_cast<typename std::remove_const<typename std::remove_pointer<typename std::decay<T>::type>::type>::type *>
           ( value );
}
template<typename RT, typename T>
inline typename std::enable_if < std::is_same<RT, const char *>::value &&is_string<T>::value,
       const char * >::type convert( RT *, const string_formatter &, T &&value, int )
{
    return value.c_str();
}
template<typename RT, typename T>
inline typename std::enable_if < std::is_same<RT, const char *>::value &&is_cstring<T>::value,
       const char * >::type convert( RT *, const string_formatter &, T &&value, int )
{
    return value;
}
template<typename RT, typename T>
inline typename std::enable_if < std::is_same<RT, const char *>::value &&is_translation<T>::value,
       const char * >::type convert( RT *, const string_formatter &sf, T &&value, int )
{
    return string_formatter_set_temp_buffer( sf, value.translated() );
}
template<typename RT, typename T>
inline typename std::enable_if < std::is_same<RT, const char *>::value &&is_numeric<T>::value
&&!is_char<T>::value, const char * >::type convert( RT *, const string_formatter &sf, T &&value,
        int )
{
    return string_formatter_set_temp_buffer( sf, to_string( value ) );
}
template<typename RT, typename T>
inline typename std::enable_if < std::is_same<RT, const char *>::value &&is_numeric<T>::value
&&is_char<T>::value, const char * >::type convert( RT *, const string_formatter &sf, T &&value,
        int )
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
    static_assert( std::is_pointer<typename std::decay<T>::type>::value ||
                   is_numeric<T>::value || is_string<T>::value || is_char<T>::value ||
                   std::is_enum<typename std::decay<T>::type>::value ||
                   is_cstring<T>::value || is_translation<T>::value, "Unsupported argument type" );
    throw_error( sf, "Tried to convert argument of type " +
                 std::string( typeid( T ).name() ) + " to " +
                 std::string( typeid( RT ).name() ) + ", which is not possible" );
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
        const std::string format;
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
        cata::optional<int> read_width();
        /// See @ref read_width. This does the same, but for the precision specifier.
        cata::optional<int> read_precision();
        /// Read and return the index of the printf-argument that is to be formatted. Returns
        /// nothing if @ref format does not refer to a specific index (caller should use
        /// @ref current_argument_index).
        cata::optional<int> read_argument_index();
        // Helper for common logic in @ref read_width and @ref read_precision.
        cata::optional<int> read_number_or_argument_index();
        /// Throws an exception containing the given message and the @ref format.
        [[noreturn]]
        void throw_error( const std::string &msg ) const;
        friend void throw_error( const string_formatter &sf, const std::string &msg ) {
            sf.throw_error( msg );
        }
        mutable std::string temp_buffer;
        /// Stores the given text in @ref temp_buffer and returns `c_str()` of it. This is used
        /// for printing non-strings through "%s". It *only* works because this prints each format
        /// specifier separately, so the content of @ref temp_buffer is only used once.
        friend const char *string_formatter_set_temp_buffer( const string_formatter &sf,
                const std::string &text ) {
            sf.temp_buffer = text;
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
            throw_error( "Requested argument " + to_string( requested ) + " but input has only " + to_string(
                             current_index ) );
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
                if( consume_next_input_if( 'l' ) ) {
                }
            } else if( consume_next_input_if( 'h' ) ) {
                if( consume_next_input_if( 'h' ) ) {
                }
            } else if( consume_next_input_if( 'z' ) ) {
            } else if( consume_next_input_if( 't' ) ) {
            }
            const char c = consume_next_input();
            current_format.push_back( c );
            switch( c ) {
                case 'c':
                    return do_formating( get_nth_arg_as<int, 0>( format_arg_index, std::forward<Args>( args )... ) );
                case 'd':
                case 'i':
                    add_long_long_length_modifier();
                    return do_formating( get_nth_arg_as<signed long long int, 0>( format_arg_index,
                                         std::forward<Args>( args )... ) );
                case 'o':
                case 'u':
                case 'x':
                case 'X':
                    add_long_long_length_modifier();
                    return do_formating( get_nth_arg_as<unsigned long long int, 0>( format_arg_index,
                                         std::forward<Args>( args )... ) );
                case 'a':
                case 'A':
                case 'g':
                case 'G':
                case 'f':
                case 'F':
                case 'e':
                case 'E':
                    return do_formating( get_nth_arg_as<double, 0>( format_arg_index, std::forward<Args>( args )... ) );
                case 'p':
                    return do_formating( get_nth_arg_as<void *, 0>( format_arg_index,
                                         std::forward<Args>( args )... ) );
                case 's':
                    return do_formating( get_nth_arg_as<const char *, 0>( format_arg_index,
                                         std::forward<Args>( args )... ) );
                default:
                    throw_error( "Unsupported format conversion: " + std::string( 1, c ) );
            }
        }

        template<typename T>
        void do_formating( T &&value ) {
            output.append( raw_string_format( current_format.c_str(), value ) );
        }

    public:
        /// @param format The format string as required by `sprintf`.
        string_formatter( std::string format ) : format( std::move( format ) ) { }
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
                const cata::optional<int> format_arg_index = read_argument_index();
                read_flags();
                if( const cata::optional<int> width_argument_index = read_width() ) {
                    const int w = get_nth_arg_as<int, 0>( *width_argument_index, std::forward<Args>( args )... );
                    current_format += to_string( w );
                }
                if( const cata::optional<int> precision_argument_index = read_precision() ) {
                    const int p = get_nth_arg_as<int, 0>( *precision_argument_index, std::forward<Args>( args )... );
                    current_format += to_string( p );
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
        /**
         * Wrapper for calling @ref vsprintf - see there for documentation. Try to avoid it as it's
         * not type safe and may easily lead to undefined behavior - use @ref string_format instead.
         * @throws std::exception if the format is invalid / does not match the arguments, but that's
         * not guaranteed - technically it's undefined behaviour.
         */
        // Implemented in output.cpp
        static std::string raw_string_format( const char *format, ... ) PRINTF_LIKE( 1, 2 );
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
inline std::string string_format( std::string format, Args &&...args )
{
    try {
        cata::string_formatter formatter( std::move( format ) );
        formatter.parse( std::forward<Args>( args )... );
        return formatter.get_output();
    } catch( ... ) {
        return cata::handle_string_format_error();
    }
}
template<typename ...Args>
inline std::string string_format( const char *const format, Args &&...args )
{
    return string_format( std::string( format ), std::forward<Args>( args )... );
}
template<typename T, typename ...Args>
inline typename std::enable_if<cata::is_translation<T>::value, std::string>::type
string_format( T &&format, Args &&...args )
{
    return string_format( format.translated(), std::forward<Args>( args )... );
}
/**@}*/

#endif
