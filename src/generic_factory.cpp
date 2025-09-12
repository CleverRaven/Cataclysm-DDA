#include "generic_factory.h"

#include "catacharset.h"
#include "color.h"
#include "game_constants.h"
#include "output.h"
#include "wcwidth.h"

void warn_disabled_feature( const JsonObject &jo, const std::string_view feature,
                            const std::string_view member, const std::string_view reason )
{
    if( !jo.has_member( feature ) ) {
        return;
    }
    JsonObject feat = jo.get_object( feature );
    feat.allow_omitted_members();
    if( feat.has_member( member ) ) {
        jo.throw_error( string_format( "Using '%s' on '%s' not permitted in this instance: %s",
                                       feature, member, reason ) );
    }
}

bool one_char_symbol_reader( const JsonObject &jo, std::string_view member_name, int &sym,
                             bool )
{
    warn_disabled_feature( jo, "extend", member_name, "disabled for one_char_symbol_reader" );
    warn_disabled_feature( jo, "delete", member_name, "disabled for one_char_symbol_reader" );
    warn_disabled_feature( jo, "relative", member_name, "disabled for one_char_symbol_reader" );
    warn_disabled_feature( jo, "proportional", member_name, "disabled for one_char_symbol_reader" );
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
    warn_disabled_feature( jo, "extend", member_name,
                           "disabled for unicode_codepoint_from_symbol_reader" );
    warn_disabled_feature( jo, "delete", member_name,
                           "disabled for unicode_codepoint_from_symbol_reader" );
    warn_disabled_feature( jo, "relative", member_name,
                           "disabled for unicode_codepoint_from_symbol_reader" );
    warn_disabled_feature( jo, "proportional", member_name,
                           "disabled for unicode_codepoint_from_symbol_reader" );
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

bool unicode_symbol_reader( const JsonObject &jo, const std::string_view member_name,
                            std::string &member, bool was_loaded )
{
    uint32_t codepoint;
    bool read = unicode_codepoint_from_symbol_reader( jo, member_name, codepoint, was_loaded );
    if( read ) {
        member = utf32_to_utf8( codepoint );
        return true;
    }
    return false;
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

nc_color nc_color_reader::get_next( const JsonValue &jv ) const
{
    if( !jv.test_string() ) {
        jv.throw_error( "invalid format for nc_color" );
    }
    return color_from_string( jv.get_string() );
}

float activity_level_reader::get_next( const JsonValue &jv ) const
{
    if( !jv.test_string() ) {
        jv.throw_error( "Invalid activity level" );
    }
    auto it = activity_levels_map.find( jv.get_string() );
    if( it == activity_levels_map.end() ) {
        jv.throw_error( "Invalid activity level" );
    }
    return it->second;
}
