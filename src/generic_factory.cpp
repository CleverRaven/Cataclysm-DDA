#include "generic_factory.h"

#include "catacharset.h"
#include "output.h"
#include "wcwidth.h"

bool one_char_symbol_reader( const JsonObject &jo, std::string_view member_name, int &sym,
                             bool )
{
    std::string sym_as_string;
    if( !jo.read( member_name, sym_as_string ) ) {
        return false;
    }
    if( sym_as_string.size() != 1 ) {
        jo.throw_error_at(
            member_name,
            string_format( "%s must be exactly one ASCII character but was %zu bytes",
                           member_name, sym_as_string.size() ) );
    }
    uint8_t c = sym_as_string.front();
    if( c > 127 ) {
        jo.throw_error_at(
            member_name,
            string_format( "%s must be exactly one ASCII character but was non-ASCII (%u)",
                           member_name, c ) );
    }
    sym = c;
    return true;
}

bool unicode_codepoint_from_symbol_reader( const JsonObject &jo,
        std::string_view member_name, uint32_t &member, bool )
{
    int sym_as_int;
    std::string sym_as_string;
    if( !jo.read( member_name, sym_as_string, false ) ) {
        // Legacy fallback to integer `sym`.
        if( !jo.read( member_name, sym_as_int ) ) {
            return false;
        } else {
            sym_as_string = string_from_int( sym_as_int );
        }
    }
    uint32_t sym_as_codepoint = UTF8_getch( sym_as_string );
    if( mk_wcwidth( sym_as_codepoint ) != 1 ) {
        jo.throw_error_at(
            member_name, str_cat( member_name, " must be exactly one console cell wide" ) );
    }
    member = sym_as_codepoint;
    return true;
}

float read_proportional_entry( const JsonObject &jo, std::string_view key )
{
    if( jo.has_float( key ) ) {
        float scalar = jo.get_float( key );
        if( scalar == 1 || scalar < 0 ) {
            jo.throw_error_at( key, "Proportional multiplier must be a positive number other than 1.0" );
        }
        return scalar;
    }
    return 1.0f;
}
