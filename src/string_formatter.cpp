#include "string_formatter.h"

#include <stdexcept>
#include <exception>

#include "cata_assert.h"
#include "cata_utility.h"

char cata::string_formatter::consume_next_input()
{
    return current_index_in_format < format.size() ? format[current_index_in_format++] : '\0';
}

char cata::string_formatter::get_current_input() const
{
    return current_index_in_format < format.size() ? format[current_index_in_format] : '\0';
}

bool cata::string_formatter::consume_next_input_if( const char c )
{
    if( c == get_current_input() ) {
        consume_next_input();
        return true;
    } else {
        return false;
    }
}

void cata::string_formatter::read_flags()
{
    static const std::string flag_characters( "#0 +'I-" );
    while( flag_characters.find( get_current_input() ) != std::string::npos ) {
        current_format.push_back( consume_next_input() );
    }
}

bool cata::string_formatter::has_digit() const
{
    const char c = get_current_input();
    return c >= '0' && c <= '9';
}

std::optional<int> cata::string_formatter::read_argument_index()
{
    const char c = get_current_input();
    // can't use has_digit because '0' is not allowed as first character
    if( c >= '1' && c <= '9' ) {
        const size_t pos = format.find_first_not_of( "012345678", current_index_in_format + 1 );
        if( pos == std::string::npos || format[pos] != '$' ) {
            return std::nullopt;
        }
        const int result = parse_integer() - 1; // arguments are 1-based
        // We already know this is true because of the `find_first_not_of` check above.
        const bool had_next = consume_next_input_if( '$' );
        ( void ) had_next;
        cata_assert( had_next );
        return result;
    } else {
        return std::nullopt;
    }
}

int cata::string_formatter::parse_integer()
{
    int result = 0;
    while( has_digit() ) {
        // TODO: Check for overflow
        result = result * 10 + ( consume_next_input() - '0' );
    }
    return result;
}

std::optional<int> cata::string_formatter::read_number_or_argument_index()
{
    if( consume_next_input_if( '*' ) ) {
        if( !has_digit() ) {
            return current_argument_index++;
        }
        const int index = parse_integer() - 1; // format specifiers are 1-based
        if( !consume_next_input_if( '$' ) ) {
            throw_error( "expected '$' after field precision" );
        }
        return index;
    }
    while( has_digit() ) {
        current_format.push_back( consume_next_input() );
    }
    return std::nullopt;
}

std::optional<int> cata::string_formatter::read_width()
{
    return read_number_or_argument_index();
}

std::optional<int> cata::string_formatter::read_precision()
{
    if( !consume_next_input_if( '.' ) ) {
        return std::nullopt;
    }
    current_format.push_back( '.' );
    return read_number_or_argument_index();
}

void cata::string_formatter::throw_error( const std::string_view msg ) const
{
    throw std::runtime_error(
        str_cat( msg, " at: \"", format.substr( 0, current_index_in_format ), "|",
                 format.substr( current_index_in_format ), "\"" ) );
}

std::string cata::handle_string_format_error()
{
    try {
        throw;
    } catch( const std::exception &err ) {
        return err.what();
    }
}

void cata::string_formatter::add_long_long_length_modifier()
{
    const char *modifier;
#if !defined(__USE_MINGW_ANSI_STDIO) && (defined(__MINGW32__) || defined(__MINGW64__))
    // mingw does not support '%llu' normally (it does with __USE_MINGW_ANSI_STDIO)
    // instead one has to use '%I64u'/'I32u'
    if( sizeof( signed long long int ) == 4 ) {
        modifier = "I32";
    } else {
        static_assert( sizeof( signed long long int ) == 8,
                       "unexpected size of long long, format specifier 'I64' does not work" );
        modifier = "I64";
    }
#else
    modifier = "ll";
#endif
    current_format.insert( current_format.size() - 1, modifier );
}
