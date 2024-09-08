#include "json.h"

#include <clocale>
#include <algorithm>
#include <bitset>
#include <cmath> // IWYU pragma: keep
#include <cstdint>
#include <cstdio>
#include <cstring> // strcmp
#include <exception>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <set>
#include <sstream> // IWYU pragma: keep
#include <string>
#include <utility>
#include <vector>

#include "cached_options.h"
#include "cata_scope_helpers.h"
#include "cata_utility.h"
#include "wcwidth.h"
#include "debug.h"
#include "output.h"
#include "string_formatter.h"

json_error_output_colors_t json_error_output_colors = json_error_output_colors_t::unset;

// JSON parsing and serialization tools for Cataclysm-DDA.
// For documentation, see the included header, json.h.

static bool is_whitespace( char ch )
{
    // These are all the valid whitespace characters allowed by RFC 4627.
    return ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r';
}

// Thw following function would fit more logically in catacharset.cpp, but it's
// needed for the json formatter and we can't easily include that file in that
// binary.
std::string utf32_to_utf8( uint32_t ch )
{
    std::array<char, 5> out;
    char *buf = out.data();
    static constexpr std::array<unsigned char, 7> utf8FirstByte = {
        0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC
    };
    int utf8Bytes;
    if( ch < 0x80 ) {
        utf8Bytes = 1;
    } else if( ch < 0x800 ) {
        utf8Bytes = 2;
    } else if( ch < 0x10000 ) {
        utf8Bytes = 3;
    } else if( ch <= 0x10FFFF ) {
        utf8Bytes = 4;
    } else {
        utf8Bytes = 3;
        ch = UNKNOWN_UNICODE;
    }

    buf += utf8Bytes;
    switch( utf8Bytes ) {
        case 4: // NOLINT(bugprone-branch-clone)
            *--buf = ( ch | 0x80 ) & 0xBF;
            ch >>= 6;
        /* fallthrough */
        case 3:
            *--buf = ( ch | 0x80 ) & 0xBF;
            ch >>= 6;
        /* fallthrough */
        case 2:
            *--buf = ( ch | 0x80 ) & 0xBF;
            ch >>= 6;
        /* fallthrough */
        case 1:
            *--buf = ch | utf8FirstByte[utf8Bytes];
    }
    out[utf8Bytes] = '\0';
    return out.data();
}

void TextJsonValue::string_error( const std::string &err ) const
{
    string_error( 0, err );
}

void TextJsonValue::string_error( int offset, const std::string &err ) const
{
    seek().string_error( offset, err );
}

void TextJsonValue::throw_error( const std::string &err ) const
{
    throw_error( 0, err );
}

void TextJsonValue::throw_error( int offset, const std::string &err ) const
{
    seek().error( offset, err );
}

void TextJsonValue::throw_error_after( const std::string &err ) const
{
    seek().skip_value();
    jsin_.error( err );
}

/* class TextJsonObject
 * represents a JSON object,
 * providing access to the underlying data.
 */
TextJsonObject::TextJsonObject( TextJsonIn &j )
{
    jsin = &j;
    start = jsin->tell();
    // cache the position of the value for each member
    jsin->start_object();
    while( !jsin->end_object() ) {
        std::string n = jsin->get_member_name();
        int p = jsin->tell();
        if( positions.count( n ) > 0 ) {
            j.error( "duplicate entry in json object" );
        }
        positions[n] = p;
        jsin->skip_value();
    }
    end_ = jsin->tell();
    final_separator = jsin->get_ate_separator();
}

void TextJsonObject::mark_visited( const std::string_view name ) const
{
#ifndef CATA_IN_TOOL
    visited_members.emplace( name );
#else
    static_cast<void>( name );
#endif
}

void TextJsonObject::report_unvisited() const
{
#ifndef CATA_IN_TOOL
    if( report_unvisited_members && !reported_unvisited_members &&
        !std::uncaught_exceptions() ) {
        reported_unvisited_members = true;
        for( const std::pair<const std::string, int> &p : positions ) {
            const std::string &name = p.first;
            if( !visited_members.count( name ) ) {
                try {
                    if( !string_starts_with( name, "//" ) ) {
                        throw_error_at(
                            name,
                            string_format( "Invalid or misplaced field name \"%s\" in JSON data, or "
                                           "value in unexpected format.", name ) );
                    } else if( name == "//~" ) {
                        throw_error_at(
                            name,
                            "\"//~\" should be within a text object and contain comments for translators." );
                    }
                } catch( const JsonError &e ) {
                    debugmsg( "(json-error)\n%s", e.what() );
                }
            }
        }
    }
#endif
}

void TextJsonObject::finish()
{
    report_unvisited();
    if( jsin && jsin->good() ) {
        jsin->seek( end_ );
        jsin->set_ate_separator( final_separator );
    }
}

size_t TextJsonObject::size() const
{
    return positions.size();
}
bool TextJsonObject::empty() const
{
    return positions.empty();
}

void TextJsonObject::allow_omitted_members() const
{
#ifndef CATA_IN_TOOL
    report_unvisited_members = false;
#endif
}

void TextJsonObject::copy_visited_members( const TextJsonObject &rhs ) const
{
#ifndef CATA_IN_TOOL
    visited_members = rhs.visited_members;
#else
    static_cast<void>( rhs );
#endif
}

int TextJsonObject::verify_position( const std::string_view name,
                                     const bool throw_exception ) const
{
    if( !jsin ) {
        if( throw_exception ) {
            throw JsonError( str_cat( "member lookup on empty object: ", name ) );
        }
        // 0 is always before the opening brace,
        // so it will never indicate a valid member position
        return 0;
    }
    const auto iter = positions.find( name );
    if( iter == positions.end() ) {
        if( throw_exception ) {
            jsin->seek( start );
            jsin->error( str_cat( "member not found: ", name ) );
        }
        // 0 is always before the opening brace,
        // so it will never indicate a valid member position
        return 0;
    }
    return iter->second;
}

bool TextJsonObject::has_member( const std::string &name ) const
{
    return positions.count( name ) > 0;
}

std::string TextJsonObject::line_number() const
{
    jsin->seek( start );
    return jsin->line_number();
}

std::string TextJsonObject::str() const
{
    // If we're getting the string form, we might be re-parsing later, so don't
    // complain about unvisited members.
    allow_omitted_members();

    if( jsin && end_ >= start ) {
        return jsin->substr( start, end_ - start );
    } else {
        return "{}";
    }
}

void TextJsonObject::throw_error_at( const std::string_view name, const std::string &err ) const
{
    mark_visited( name );
    if( !jsin ) {
        throw JsonError( err );
    }
    const int pos = verify_position( name, false );
    if( pos ) {
        jsin->seek( pos );
    }
    jsin->error( err );
}

void TextJsonArray::throw_error( const std::string &err ) const
{
    if( !jsin ) {
        throw JsonError( err );
    }
    jsin->error( err );
}

void TextJsonArray::throw_error( int idx, const std::string &err ) const
{
    if( !jsin ) {
        throw JsonError( err );
    }
    if( idx >= 0 && static_cast<size_t>( idx ) < positions.size() ) {
        jsin->seek( positions[idx] );
    }
    jsin->error( err );
}

void TextJsonArray::string_error( const int idx, const int offset, const std::string &err ) const
{
    if( jsin && idx >= 0 && static_cast<size_t>( idx ) < positions.size() ) {
        jsin->seek( positions[idx] );
        jsin->string_error( offset, err );
    } else {
        throw_error( err );
    }
}

void TextJsonObject::throw_error( const std::string &err ) const
{
    if( !jsin ) {
        throw JsonError( err );
    }
    jsin->error( err );
}

TextJsonIn *TextJsonObject::get_raw( const std::string_view name ) const
{
    int pos = verify_position( name );
    mark_visited( name );
    jsin->seek( pos );
    return jsin;
}

json_source_location TextJsonObject::get_source_location() const
{
    if( !jsin ) {
        throw JsonError( "TextJsonObject::get_source_location called when stream is null" );
    }
    json_source_location loc;
    loc.path = jsin->get_path();
    if( !loc.path ) {
        jsin->seek( start );
        jsin->error( "TextJsonObject::get_source_location called but the path is unknown" );
    }
    loc.offset = start;
    return loc;
}

/* returning values by name */

bool TextJsonObject::get_bool( const std::string_view name ) const
{
    return get_member( name ).get_bool();
}

bool TextJsonObject::get_bool( const std::string_view name, const bool fallback ) const
{
    int pos = verify_position( name, false );
    if( !pos ) {
        return fallback;
    }
    mark_visited( name );
    jsin->seek( pos );
    return jsin->get_bool();
}

int TextJsonObject::get_int( const std::string_view name ) const
{
    return get_member( name ).get_int();
}

int TextJsonObject::get_int( const std::string_view name, const int fallback ) const
{
    int pos = verify_position( name, false );
    if( !pos ) {
        return fallback;
    }
    mark_visited( name );
    jsin->seek( pos );
    return jsin->get_int();
}

double TextJsonObject::get_float( const std::string_view name ) const
{
    return get_member( name ).get_float();
}

double TextJsonObject::get_float( const std::string_view name, const double fallback ) const
{
    int pos = verify_position( name, false );
    if( !pos ) {
        return fallback;
    }
    mark_visited( name );
    jsin->seek( pos );
    return jsin->get_float();
}

std::string TextJsonObject::get_string( const std::string_view name ) const
{
    return get_member( name ).get_string();
}

std::string TextJsonObject::get_string( const std::string_view name,
                                        const std::string &fallback ) const
{
    int pos = verify_position( name, false );
    if( !pos ) {
        return fallback;
    }
    mark_visited( name );
    jsin->seek( pos );
    return jsin->get_string();
}

/* returning containers by name */

TextJsonArray TextJsonObject::get_array( const std::string_view name ) const
{
    int pos = verify_position( name, false );
    if( !pos ) {
        return TextJsonArray();
    }
    mark_visited( name );
    jsin->seek( pos );
    return TextJsonArray( *jsin );
}

std::vector<int> TextJsonObject::get_int_array( const std::string_view name ) const
{
    std::vector<int> ret;
    for( const int entry : get_array( name ) ) {
        ret.push_back( entry );
    }
    return ret;
}

std::vector<std::string> TextJsonObject::get_string_array( const std::string_view name ) const
{
    std::vector<std::string> ret;
    for( const std::string entry : get_array( name ) ) {
        ret.push_back( entry );
    }
    return ret;
}

std::vector<std::string> TextJsonObject::get_as_string_array( const std::string_view name ) const
{
    std::vector<std::string> ret;
    if( has_array( name ) ) {
        for( const std::string entry : get_array( name ) ) {
            ret.push_back( entry );
        }
    } else {
        ret.push_back( get_string( name ) );
    }
    return ret;
}

TextJsonObject TextJsonObject::get_object( const std::string_view name ) const
{
    int pos = verify_position( name, false );
    if( !pos ) {
        return TextJsonObject();
    }
    mark_visited( name );
    jsin->seek( pos );
    return jsin->get_object();
}

/* non-fatal member existence and type testing */

bool TextJsonObject::has_null( const std::string_view name ) const
{
    int pos = verify_position( name, false );
    if( !pos ) {
        return false;
    }
    mark_visited( name );
    jsin->seek( pos );
    return jsin->test_null();
}

bool TextJsonObject::has_bool( const std::string_view name ) const
{
    int pos = verify_position( name, false );
    if( !pos ) {
        return false;
    }
    jsin->seek( pos );
    return jsin->test_bool();
}

bool TextJsonObject::has_number( const std::string_view name ) const
{
    int pos = verify_position( name, false );
    if( !pos ) {
        return false;
    }
    jsin->seek( pos );
    return jsin->test_number();
}

bool TextJsonObject::has_string( const std::string_view name ) const
{
    int pos = verify_position( name, false );
    if( !pos ) {
        return false;
    }
    jsin->seek( pos );
    return jsin->test_string();
}

bool TextJsonObject::has_array( const std::string_view name ) const
{
    int pos = verify_position( name, false );
    if( !pos ) {
        return false;
    }
    jsin->seek( pos );
    return jsin->test_array();
}

bool TextJsonObject::has_object( const std::string_view name ) const
{
    int pos = verify_position( name, false );
    if( !pos ) {
        return false;
    }
    jsin->seek( pos );
    return jsin->test_object();
}

/* class TextJsonArray
 * represents a JSON array,
 * providing access to the underlying data.
 */
TextJsonArray::TextJsonArray( TextJsonIn &j )
{
    jsin = &j;
    start = jsin->tell();
    index = 0;
    // cache the position of each element
    jsin->start_array();
    while( !jsin->end_array() ) {
        positions.push_back( jsin->tell() );
        jsin->skip_value();
    }
    end_ = jsin->tell();
    final_separator = jsin->get_ate_separator();
}

TextJsonArray::TextJsonArray( const TextJsonArray &ja )
{
    *this = ja;
}

// NOLINTNEXTLINE(bugprone-unhandled-self-assignment,cert-oop54-cpp)
TextJsonArray &TextJsonArray::operator=( const TextJsonArray &ja )
{
    jsin = ja.jsin;
    start = ja.start;
    index = 0;
    positions = ja.positions;
    end_ = ja.end_;
    final_separator = ja.final_separator;

    return *this;
}

void TextJsonArray::finish()
{
    if( jsin && jsin->good() ) {
        jsin->seek( end_ );
        jsin->set_ate_separator( final_separator );
    }
}

bool TextJsonArray::has_more() const
{
    return index < positions.size();
}
size_t TextJsonArray::size() const
{
    return positions.size();
}
bool TextJsonArray::empty()
{
    return positions.empty();
}

std::string TextJsonArray::str()
{
    if( jsin ) {
        return jsin->substr( start, end_ - start );
    } else {
        return "[]";
    }
}

void TextJsonArray::verify_index( const size_t i ) const
{
    if( !jsin ) {
        throw JsonError( "tried to access empty array." );
    } else if( i >= positions.size() ) {
        jsin->seek( start );
        std::stringstream err;
        err << "bad index value: " << i;
        jsin->error( err.str() );
    }
}

/* iterative access */

TextJsonValue TextJsonArray::next_value()
{
    verify_index( index );
    jsin->seek( positions[index++] );
    return jsin->get_value();
}

bool TextJsonArray::next_bool()
{
    verify_index( index );
    jsin->seek( positions[index++] );
    return jsin->get_bool();
}

int TextJsonArray::next_int()
{
    verify_index( index );
    jsin->seek( positions[index++] );
    return jsin->get_int();
}

double TextJsonArray::next_float()
{
    verify_index( index );
    jsin->seek( positions[index++] );
    return jsin->get_float();
}

std::string TextJsonArray::next_string()
{
    verify_index( index );
    jsin->seek( positions[index++] );
    return jsin->get_string();
}

TextJsonArray TextJsonArray::next_array()
{
    verify_index( index );
    jsin->seek( positions[index++] );
    return jsin->get_array();
}

TextJsonObject TextJsonArray::next_object()
{
    verify_index( index );
    jsin->seek( positions[index++] );
    return jsin->get_object();
}

void TextJsonArray::skip_value()
{
    verify_index( index );
    ++index;
}

/* static access */

bool TextJsonArray::get_bool( const size_t i ) const
{
    verify_index( i );
    jsin->seek( positions[i] );
    return jsin->get_bool();
}

int TextJsonArray::get_int( const size_t i ) const
{
    verify_index( i );
    jsin->seek( positions[i] );
    return jsin->get_int();
}

double TextJsonArray::get_float( const size_t i ) const
{
    verify_index( i );
    jsin->seek( positions[i] );
    return jsin->get_float();
}

std::string TextJsonArray::get_string( const size_t i ) const
{
    verify_index( i );
    jsin->seek( positions[i] );
    return jsin->get_string();
}

TextJsonArray TextJsonArray::get_array( const size_t i ) const
{
    verify_index( i );
    jsin->seek( positions[i] );
    return jsin->get_array();
}

TextJsonObject TextJsonArray::get_object( const size_t i ) const
{
    verify_index( i );
    jsin->seek( positions[i] );
    return jsin->get_object();
}

/* iterative type checking */

bool TextJsonArray::test_null() const
{
    if( !has_more() ) {
        return false;
    }
    jsin->seek( positions[index] );
    return jsin->test_null();
}

bool TextJsonArray::test_bool() const
{
    if( !has_more() ) {
        return false;
    }
    jsin->seek( positions[index] );
    return jsin->test_bool();
}

bool TextJsonArray::test_number() const
{
    if( !has_more() ) {
        return false;
    }
    jsin->seek( positions[index] );
    return jsin->test_number();
}

bool TextJsonArray::test_string() const
{
    if( !has_more() ) {
        return false;
    }
    jsin->seek( positions[index] );
    return jsin->test_string();
}

bool TextJsonArray::test_bitset() const
{
    if( !has_more() ) {
        return false;
    }
    jsin->seek( positions[index] );
    return jsin->test_bitset();
}

bool TextJsonArray::test_array() const
{
    if( !has_more() ) {
        return false;
    }
    jsin->seek( positions[index] );
    return jsin->test_array();
}

bool TextJsonArray::test_object() const
{
    if( !has_more() ) {
        return false;
    }
    jsin->seek( positions[index] );
    return jsin->test_object();
}

/* random-access type checking */

bool TextJsonArray::has_null( const size_t i ) const
{
    verify_index( i );
    jsin->seek( positions[i] );
    return jsin->test_null();
}

bool TextJsonArray::has_bool( const size_t i ) const
{
    verify_index( i );
    jsin->seek( positions[i] );
    return jsin->test_bool();
}

bool TextJsonArray::has_number( const size_t i ) const
{
    verify_index( i );
    jsin->seek( positions[i] );
    return jsin->test_number();
}

bool TextJsonArray::has_string( const size_t i ) const
{
    verify_index( i );
    jsin->seek( positions[i] );
    return jsin->test_string();
}

bool TextJsonArray::has_array( const size_t i ) const
{
    verify_index( i );
    jsin->seek( positions[i] );
    return jsin->test_array();
}

bool TextJsonArray::has_object( const size_t i ) const
{
    verify_index( i );
    jsin->seek( positions[i] );
    return jsin->test_object();
}

void add_array_to_set( std::set<std::string> &s, const TextJsonObject &json,
                       const std::string_view name )
{
    for( const std::string line : json.get_array( name ) ) {
        s.insert( line );
    }
}

TextJsonIn::TextJsonIn( std::istream &s ) : stream( &s )
{
    sanity_check_stream();
}

TextJsonIn::TextJsonIn( std::istream &s, const std::string &path )
    : stream( &s )
    , path( make_shared_fast<std::string>( path ) )
{
    sanity_check_stream();
}

TextJsonIn::TextJsonIn( std::istream &s, const json_source_location &loc )
    : stream( &s ), path( loc.path )
{
    seek( loc.offset );
    sanity_check_stream();
}

void TextJsonIn::sanity_check_stream()
{
    char c = stream->peek();
    if( c == '\xef' ) {
        error( _( "This JSON file looks like it starts with a Byte Order Mark (BOM) or is otherwise corrupted.  This can happen if you edit files in Windows Notepad.  See doc/CONTRIBUTING.md for more advice." ) );
    }
}

int TextJsonIn::tell()
{
    return stream->tellg();
}
char TextJsonIn::peek()
{
    return static_cast<char>( stream->peek() );
}
bool TextJsonIn::good()
{
    return stream->good();
}

void TextJsonIn::seek( int pos )
{
    stream->clear();
    stream->seekg( pos );
    ate_separator = false;
}

void TextJsonIn::eat_whitespace()
{
    while( is_whitespace( peek() ) ) {
        stream->get();
    }
}

void TextJsonIn::uneat_whitespace()
{
    while( tell() > 0 ) {
        stream->seekg( -1, std::istream::cur );
        if( !is_whitespace( peek() ) ) {
            break;
        }
    }
}

void TextJsonIn::end_value()
{
    ate_separator = false;
    skip_separator();
}

void TextJsonIn::skip_member()
{
    skip_string();
    skip_pair_separator();
    skip_value();
}

void TextJsonIn::skip_separator()
{
    eat_whitespace();
    signed char ch = peek();
    if( ch == ',' ) {
        if( ate_separator ) {
            error( "duplicate comma" );
        }
        stream->get();
        ate_separator = true;
    } else if( ch == ']' || ch == '}' || ch == ':' ) {
        // okay
        if( ate_separator ) {
            std::stringstream err;
            err << "comma should not be found before '" << ch << "'";
            uneat_whitespace();
            error( err.str() );
        }
        ate_separator = false;
    } else if( ch == EOF ) {
        // that's okay too... probably
        if( ate_separator ) {
            uneat_whitespace();
            error( "comma at end of file not allowed" );
        }
        ate_separator = false;
    } else {
        // not okay >:(
        uneat_whitespace();
        error( 1, "missing comma" );
    }
}

void TextJsonIn::skip_pair_separator()
{
    char ch;
    eat_whitespace();
    stream->get( ch );
    if( ch != ':' ) {
        std::stringstream err;
        err << "expected pair separator ':', not '" << ch << "'";
        error( -1, err.str() );
    } else if( ate_separator ) {
        error( -1, "duplicate pair separator ':' not allowed" );
    }
    ate_separator = true;
}

void TextJsonIn::skip_string()
{
    char ch;
    eat_whitespace();
    stream->get( ch );
    if( ch != '"' ) {
        std::stringstream err;
        err << "expecting string but found '" << ch << "'";
        error( -1, err.str() );
    }
    while( stream->good() ) {
        stream->get( ch );
        if( ch == '\\' ) {
            stream->get( ch );
            continue;
        } else if( ch == '"' ) {
            break;
        } else if( ch == '\r' || ch == '\n' ) {
            error( -1, "string not closed before end of line" );
        }
    }
    end_value();
}

void TextJsonIn::skip_value()
{
    eat_whitespace();
    char ch = peek();
    // it's either a string '"'
    if( ch == '"' ) {
        skip_string();
        // or an object '{'
    } else if( ch == '{' ) {
        skip_object();
        // or an array '['
    } else if( ch == '[' ) {
        skip_array();
        // or a number (-0123456789)
    } else if( ch == '-' || ( ch >= '0' && ch <= '9' ) ) {
        skip_number();
        // or "true", "false" or "null"
    } else if( ch == 't' ) {
        skip_true();
    } else if( ch == 'f' ) {
        skip_false();
    } else if( ch == 'n' ) {
        skip_null();
        // or an error.
    } else {
        std::stringstream err;
        err << "expected JSON value but got '" << ch << "'";
        error( err.str() );
    }
    // skip_* end value automatically
}

void TextJsonIn::skip_object()
{
    start_object();
    while( !end_object() ) {
        skip_member();
    }
    // end_value called by end_object
}

void TextJsonIn::skip_array()
{
    start_array();
    while( !end_array() ) {
        skip_value();
    }
    // end_value called by end_array
}

void TextJsonIn::skip_true()
{
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    char text[5];
    eat_whitespace();
    stream->get( text, 5 );
    if( strcmp( text, "true" ) != 0 ) {
        std::stringstream err;
        err << R"(expected "true", but found ")" << text << "\"";
        error( -4, err.str() );
    }
    end_value();
}

void TextJsonIn::skip_false()
{
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    char text[6];
    eat_whitespace();
    stream->get( text, 6 );
    if( strcmp( text, "false" ) != 0 ) {
        std::stringstream err;
        err << R"(expected "false", but found ")" << text << "\"";
        error( -5, err.str() );
    }
    end_value();
}

void TextJsonIn::skip_null()
{
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    char text[5];
    eat_whitespace();
    stream->get( text, 5 );
    if( strcmp( text, "null" ) != 0 ) {
        std::stringstream err;
        err << R"(expected "null", but found ")" << text << "\"";
        error( -4, err.str() );
    }
    end_value();
}

void TextJsonIn::skip_number()
{
    char ch;
    eat_whitespace();
    // skip all of (+-0123456789.eE)
    while( stream->good() ) {
        stream->get( ch );
        if( ch != '+' && ch != '-' && ( ch < '0' || ch > '9' ) &&
            ch != 'e' && ch != 'E' && ch != '.' ) {
            stream->unget();
            break;
        }
    }
    end_value();
}

std::string TextJsonIn::get_member_name()
{
    std::string s = get_string();
    skip_pair_separator();
    return s;
}

static bool get_escaped_or_unicode( std::istream &stream, std::string &s, std::string &err )
{
    if( !stream.good() ) {
        err = "stream not good";
        return false;
    }
    char ch;
    stream.get( ch );
    if( !stream.good() ) {
        err = "read operation failed";
        return false;
    }
    if( ch == '\\' ) {
        // converting \", \\, \/, \b, \f, \n, \r, \t and \uxxxx according to JSON spec.
        stream.get( ch );
        if( !stream.good() ) {
            err = "read operation failed";
            return false;
        }
        switch( ch ) {
            case '\\':
                s += '\\';
                break;
            case '"':
                s += '"';
                break;
            case '/':
                s += '/';
                break;
            case 'b':
                s += '\b';
                break;
            case 'f':
                s += '\f';
                break;
            case 'n':
                s += '\n';
                break;
            case 'r':
                s += '\r';
                break;
            case 't':
                s += '\t';
                break;
            case 'u': {
                    uint32_t u = 0;
                    for( int i = 0; i < 4; ++i ) {
                        stream.get( ch );
                        if( !stream.good() ) {
                            err = "read operation failed";
                            return false;
                        }
                        if( ch >= '0' && ch <= '9' ) {
                            u = ( u << 4 ) | ( ch - '0' );
                        } else if( ch >= 'a' && ch <= 'f' ) {
                            u = ( u << 4 ) | ( ch - 'a' + 10 );
                        } else if( ch >= 'A' && ch <= 'F' ) {
                            u = ( u << 4 ) | ( ch - 'A' + 10 );
                        } else {
                            err = "expected hex digit";
                            return false;
                        }
                    }
                    s += utf32_to_utf8( u );
                }
                break;
            default:
                err = "invalid escape sequence";
                return false;
        }
    } else if( ch == '\r' || ch == '\n' ) {
        err = "reached end of line without closing string";
        return false;
    } else if( ch == '"' ) {
        // the caller is supposed to handle the ending quote
        err = "unexpected ending quote";
        return false;
    } else if( static_cast<unsigned char>( ch ) < 0x20 ) {
        err = "invalid character inside string";
        return false;
    } else {
        unsigned char uc = static_cast<unsigned char>( ch );
        uint32_t unicode = 0;
        int n = 0;
        if( uc >= 0xFC ) {
            unicode = uc & 0x01;
            n = 5;
        } else if( uc >= 0xF8 ) {
            unicode = uc & 0x03;
            n = 4;
        } else if( uc >= 0xF0 ) {
            unicode = uc & 0x07;
            n = 3;
        } else if( uc >= 0xE0 ) {
            unicode = uc & 0x0f;
            n = 2;
        } else if( uc >= 0xC0 ) {
            unicode = uc & 0x1f;
            n = 1;
        } else if( uc >= 0x80 ) {
            err = "invalid utf8 sequence";
            return false;
        } else {
            unicode = uc;
            n = 0;
        }
        s += ch;
        for( ; n > 0; --n ) {
            stream.get( ch );
            if( !stream.good() ) {
                err = "read operation failed";
                return false;
            }
            uc = static_cast<unsigned char>( ch );
            if( uc < 0x80 || uc >= 0xC0 ) {
                err = "invalid utf8 sequence";
                return false;
            }
            unicode = ( unicode << 6 ) | ( uc & 0x3f );
            s += ch;
        }
        if( unicode > 0x10FFFF ) {
            err = "invalid unicode codepoint";
            return false;
        }
    }
    return true;
}

std::string TextJsonIn::get_string()
{
    eat_whitespace();
    std::string s;
    char ch;
    std::string err;
    bool success = false;
    do {
        // the first character had better be a '"'
        stream->get( ch );
        if( !stream->good() ) {
            err = "read operation failed";
            break;
        }
        if( ch != '"' ) {
            err = "expected string but got '" + std::string( 1, ch ) + "'";
            break;
        }
        // add chars to the string, one at a time
        do {
            ch = stream->peek();
            if( !stream->good() ) {
                err = "read operation failed";
                break;
            }
            if( ch == '"' ) {
                stream->ignore();
                success = true;
                break;
            }
            if( !get_escaped_or_unicode( *stream, s, err ) ) {
                break;
            }
        } while( stream->good() );
    } while( false );
    if( success ) {
        end_value();
        return s;
    }
    if( stream->eof() ) {
        error( "couldn't find end of string, reached EOF." );
    } else if( stream->fail() ) {
        error( "stream failure while reading string." );
    } else {
        error( -1, err );
    }
}

// These functions get -INT_MIN and -INT64_MIN while very carefully avoiding any overflow.
constexpr static uint64_t neg_INT_MIN()
{
    static_assert( sizeof( int ) <= sizeof( int64_t ),
                   "neg_INT_MIN() assumed sizeof( int ) <= sizeof( int64_t )" );
    constexpr int x = std::numeric_limits<int>::min() + std::numeric_limits<int>::max();
    static_assert( x >= 0 || x + std::numeric_limits<int>::max() >= 0,
                   "neg_INT_MIN assumed INT_MIN + INT_MAX >= -INT_MAX" );
    if( x < 0 ) {
        return static_cast<uint64_t>( std::numeric_limits<int>::max() ) + static_cast<uint64_t>( -x );
    } else {
        return static_cast<uint64_t>( std::numeric_limits<int>::max() ) - static_cast<uint64_t>( x );
    }
}
constexpr static uint64_t neg_INT64_MIN()
{
    constexpr int64_t x = std::numeric_limits<int64_t>::min() + std::numeric_limits<int64_t>::max();
    static_assert( x >= 0 || x + std::numeric_limits<int64_t>::max() >= 0,
                   "neg_INT64_MIN assumed INT64_MIN + INT64_MAX >= -INT64_MAX" );
    if( x < 0 ) {
        return static_cast<uint64_t>( std::numeric_limits<int64_t>::max() ) + static_cast<uint64_t>( -x );
    } else {
        return static_cast<uint64_t>( std::numeric_limits<int64_t>::max() ) - static_cast<uint64_t>( x );
    }
}

number_sci_notation TextJsonIn::get_any_int()
{
    number_sci_notation n = get_any_number();
    if( n.exp < 0 ) {
        error( "Integers cannot have a decimal point or negative order of magnitude." );
    }
    // Manually apply scientific notation, since std::pow converts to double under the hood.
    for( int64_t i = 0; i < n.exp; i++ ) {
        if( n.number > std::numeric_limits<uint64_t>::max() / 10ULL ) {
            error( "Specified order of magnitude too large -- encountered overflow applying it." );
        }
        n.number *= 10ULL;
    }
    n.exp = 0;
    return n;
}

int TextJsonIn::get_int()
{
    static_assert( sizeof( int ) <= sizeof( int64_t ),
                   "TextJsonIn::get_int() assumed sizeof( int ) <= sizeof( int64_t )" );
    number_sci_notation n = get_any_int();
    if( !n.negative && n.number > static_cast<uint64_t>( std::numeric_limits<int>::max() ) ) {
        error( "Found a number greater than " + std::to_string( std::numeric_limits<int>::max() ) +
               " which is unsupported in this context." );
    } else if( n.negative && n.number > neg_INT_MIN() ) {
        error( "Found a number less than " + std::to_string( std::numeric_limits<int>::min() ) +
               " which is unsupported in this context." );
    }
    if( n.negative ) {
        static_assert( neg_INT_MIN() <= static_cast<uint64_t>( std::numeric_limits<int>::max() )
                       || neg_INT_MIN() - static_cast<uint64_t>( std::numeric_limits<int>::max() )
                       <= static_cast<uint64_t>( std::numeric_limits<int>::max() ),
                       "TextJsonIn::get_int() assumed -INT_MIN - INT_MAX <= INT_MAX" );
        if( n.number > static_cast<uint64_t>( std::numeric_limits<int>::max() ) ) {
            const uint64_t x = n.number - static_cast<uint64_t>( std::numeric_limits<int>::max() );
            return -std::numeric_limits<int>::max() - static_cast<int>( x );
        } else {
            return -static_cast<int>( n.number );
        }
    } else {
        return static_cast<int>( n.number );
    }
}

unsigned int TextJsonIn::get_uint()
{
    number_sci_notation n = get_any_int();
    if( n.number > std::numeric_limits<unsigned int>::max() ) {
        error( "Found a number greater than " +
               std::to_string( std::numeric_limits<unsigned int>::max() ) +
               " which is unsupported in this context." );
    }
    if( n.negative ) {
        error( "Unsigned integers cannot have a negative sign." );
    }
    return static_cast<unsigned int>( n.number );
}

int64_t TextJsonIn::get_int64()
{
    number_sci_notation n = get_any_int();
    if( !n.negative && n.number > static_cast<uint64_t>( std::numeric_limits<int64_t>::max() ) ) {
        error( "Signed integers greater than " +
               std::to_string( std::numeric_limits<int64_t>::max() ) + " not supported." );
    } else if( n.negative && n.number > neg_INT64_MIN() ) {
        error( "Integers less than "
               + std::to_string( std::numeric_limits<int64_t>::min() ) + " not supported." );
    }
    if( n.negative ) {
        static_assert( neg_INT64_MIN() <= static_cast<uint64_t>( std::numeric_limits<int64_t>::max() )
                       || neg_INT64_MIN() - static_cast<uint64_t>( std::numeric_limits<int64_t>::max() )
                       <= static_cast<uint64_t>( std::numeric_limits<int64_t>::max() ),
                       "TextJsonIn::get_int64() assumed -INT64_MIN - INT64_MAX <= INT64_MAX" );
        if( n.number > static_cast<uint64_t>( std::numeric_limits<int64_t>::max() ) ) {
            const uint64_t x = n.number - static_cast<uint64_t>( std::numeric_limits<int64_t>::max() );
            return -std::numeric_limits<int64_t>::max() - static_cast<int64_t>( x );
        } else {
            return -static_cast<int64_t>( n.number );
        }
    } else {
        return static_cast<int64_t>( n.number );
    }
}

uint64_t TextJsonIn::get_uint64()
{
    number_sci_notation n = get_any_int();
    if( n.negative ) {
        error( "Unsigned integers cannot have a negative sign." );
    }
    return n.number;
}

double TextJsonIn::get_float()
{
    number_sci_notation n = get_any_number();
    return n.number * std::pow( 10.0f, n.exp ) * ( n.negative ? -1.f : 1.f );
}

number_sci_notation TextJsonIn::get_any_number()
{
    // this could maybe be prettier?
    char ch;
    number_sci_notation ret;
    int mod_e = 0;
    eat_whitespace();
    if( !stream->get( ch ) ) {
        error( "unexpected end of input" );
    }
    if( ( ret.negative = ch == '-' ) ) {
        if( !stream->get( ch ) ) {
            error( "unexpected end of input" );
        }
    } else if( ch != '.' && ( ch < '0' || ch > '9' ) ) {
        // not a valid float
        std::stringstream err;
        err << "expecting number but found '" << ch << "'";
        error( -1, err.str() );
    }
    if( ch == '0' ) {
        // allow a single leading zero in front of a '.' or 'e'/'E'
        stream->get( ch );
        if( ch >= '0' && ch <= '9' ) {
            error( -1, "leading zeros not allowed" );
        }
    }
    while( ch >= '0' && ch <= '9' ) {
        ret.number *= 10;
        ret.number += ( ch - '0' );
        if( !stream->get( ch ) ) {
            break;
        }
    }
    if( ch == '.' ) {
        while( stream->get( ch ) && ch >= '0' && ch <= '9' ) {
            ret.number *= 10;
            ret.number += ( ch - '0' );
            mod_e -= 1;
        }
    }
    if( stream && ( ch == 'e' || ch == 'E' ) ) {
        if( !stream->get( ch ) ) {
            error( "unexpected end of input" );
        }
        bool neg;
        if( ( neg = ch == '-' ) || ch == '+' ) {
            if( !stream->get( ch ) ) {
                error( "unexpected end of input" );
            }
        }
        while( ch >= '0' && ch <= '9' ) {
            ret.exp *= 10;
            ret.exp += ( ch - '0' );
            if( !stream->get( ch ) ) {
                break;
            }
        }
        if( neg ) {
            ret.exp *= -1;
        }
    }
    // unget the final non-number character (probably a separator)
    if( stream ) {
        stream->unget();
    }
    end_value();
    ret.exp += mod_e;
    return ret;
}

bool TextJsonIn::get_bool()
{
    char ch;
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    char text[5];
    std::stringstream err;
    eat_whitespace();
    stream->get( ch );
    if( ch == 't' ) {
        stream->get( text, 4 );
        if( strcmp( text, "rue" ) == 0 ) {
            end_value();
            return true;
        } else {
            err << R"(not a boolean.  expected "true", but got ")";
            err << ch << text << "\"";
            error( -4, err.str() );
        }
    } else if( ch == 'f' ) {
        stream->get( text, 5 );
        if( strcmp( text, "alse" ) == 0 ) {
            end_value();
            return false;
        } else {
            err << R"(not a boolean.  expected "false", but got ")";
            err << ch << text << "\"";
            error( -5, err.str() );
        }
    }
    err << "not a boolean value!  expected 't' or 'f' but got '" << ch << "'";
    error( -1, err.str() );
}

TextJsonObject TextJsonIn::get_object()
{
    return TextJsonObject( *this );
}
TextJsonArray TextJsonIn::get_array()
{
    return TextJsonArray( *this );
}
TextJsonValue TextJsonIn::get_value()
{
    return TextJsonValue( *this, tell() );
}

void TextJsonIn::start_array()
{
    eat_whitespace();
    if( peek() == '[' ) {
        stream->get();
        ate_separator = false;
        return;
    } else {
        // expecting an array, so this is an error
        std::stringstream err;
        err << "tried to start array, but found '";
        err << peek() << "', not '['";
        error( err.str() );
    }
}

bool TextJsonIn::end_array()
{
    eat_whitespace();
    if( peek() == ']' ) {
        if( ate_separator ) {
            uneat_whitespace();
            error( "comma not allowed at end of array" );
        }
        stream->get();
        end_value();
        return true;
    } else {
        // not the end yet, so just return false?
        return false;
    }
}

void TextJsonIn::start_object()
{
    eat_whitespace();
    if( peek() == '{' ) {
        stream->get();
        ate_separator = false; // not that we want to
        return;
    } else {
        // expecting an object, so fail loudly
        std::stringstream err;
        err << "tried to start object, but found '";
        err << peek() << "', not '{'";
        error( err.str() );
    }
}

bool TextJsonIn::end_object()
{
    eat_whitespace();
    if( peek() == '}' ) {
        if( ate_separator ) {
            uneat_whitespace();
            error( "comma not allowed at end of object" );
        }
        stream->get();
        end_value();
        return true;
    } else {
        // not the end yet, so just return false?
        return false;
    }
}

bool TextJsonIn::test_null()
{
    eat_whitespace();
    return peek() == 'n';
}

bool TextJsonIn::test_bool()
{
    eat_whitespace();
    const char ch = peek();
    return ch == 't' || ch == 'f';
}

bool TextJsonIn::test_number()
{
    eat_whitespace();
    const char ch = peek();
    return ch == '-' || ch == '+' || ch == '.' || ( ch >= '0' && ch <= '9' );
}

bool TextJsonIn::test_string()
{
    eat_whitespace();
    return peek() == '"';
}

bool TextJsonIn::test_bitset()
{
    eat_whitespace();
    return peek() == '"';
}

bool TextJsonIn::test_array()
{
    eat_whitespace();
    return peek() == '[';
}

bool TextJsonIn::test_object()
{
    eat_whitespace();
    return peek() == '{';
}

/* non-fatal value setting by reference */

bool TextJsonIn::read_null( bool throw_on_error )
{
    if( !test_null() ) {
        return error_or_false( throw_on_error, "Expected null" );
    }
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    char text[5];
    if( !stream->get( text, 5 ) ) {
        error( "Unexpected end of stream reading null" );
    }
    if( 0 != strcmp( text, "null" ) ) {
        error( -4, std::string( "Expected 'null', got '" ) + text + "'" );
    }
    end_value();
    return true;
}

bool TextJsonIn::read( bool &b, bool throw_on_error )
{
    if( !test_bool() ) {
        return error_or_false( throw_on_error, "Expected bool" );
    }
    b = get_bool();
    return true;
}

bool TextJsonIn::read( char &c, bool throw_on_error )
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    c = get_int();
    return true;
}

bool TextJsonIn::read( signed char &c, bool throw_on_error )
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    // TODO: test for overflow
    c = get_int();
    return true;
}

bool TextJsonIn::read( unsigned char &c, bool throw_on_error )
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    // TODO: test for overflow
    c = get_int();
    return true;
}

bool TextJsonIn::read( short unsigned int &s, bool throw_on_error )
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    // TODO: test for overflow
    s = get_int();
    return true;
}

bool TextJsonIn::read( short int &s, bool throw_on_error )
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    // TODO: test for overflow
    s = get_int();
    return true;
}

bool TextJsonIn::read( int &i, bool throw_on_error )
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    i = get_int();
    return true;
}

bool TextJsonIn::read( std::int64_t &i, bool throw_on_error )
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    i = get_int64();
    return true;
}

bool TextJsonIn::read( std::uint64_t &i, bool throw_on_error )
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    i = get_uint64();
    return true;
}

bool TextJsonIn::read( unsigned int &u, bool throw_on_error )
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    u = get_uint();
    return true;
}

bool TextJsonIn::read( float &f, bool throw_on_error )
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    f = get_float();
    return true;
}

bool TextJsonIn::read( double &d, bool throw_on_error )
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    d = get_float();
    return true;
}

bool TextJsonIn::read( std::string &s, bool throw_on_error )
{
    if( !test_string() ) {
        return error_or_false( throw_on_error, "Expected string" );
    }
    s = get_string();
    return true;
}

template<size_t N>
bool TextJsonIn::read( std::bitset<N> &b, bool throw_on_error )
{
    if( !test_bitset() ) {
        return error_or_false( throw_on_error, "Expected bitset" );
    }
    std::string tmp_string = get_string();
    if( tmp_string.length() > N ) {
        // If the loaded string contains more bits than expected, skip the most significant bits
        tmp_string.erase( 0, tmp_string.length() - N );
    }
    b = std::bitset<N> ( tmp_string );
    return true;
}

/**
 * Get the normal form of a relative path. Does not work on absolute paths.
 * Slash and backslash are both treated as path separators and replaced with
 * slash. Trailing slashes are always removed.
 */
static std::string normalize_relative_path( const std::string &path )
{
    if( path.empty() ) {
        // normal form of an empty path is an empty path
        return path;
    }
    std::vector<std::string> names;
    for( size_t name_start = 0; name_start < path.size(); ) {
        const size_t name_end = std::min( path.find_first_of( "\\/", name_start ),
                                          path.size() );
        if( name_start < name_end ) {
            const std::string name = path.substr( name_start, name_end - name_start );
            if( name == "." ) {
                // do nothing
            } else if( name == ".." ) {
                if( names.empty() || names.back() == ".." ) {
                    names.emplace_back( name );
                } else {
                    names.pop_back();
                }
            } else {
                names.emplace_back( name );
            }
        }
        name_start = std::min( path.find_first_not_of( "\\/", name_end ),
                               path.size() );
    }
    if( names.empty() ) {
        return ".";
    } else {
        std::string normpath;
        for( auto it = names.begin(); it != names.end(); ++it ) {
            if( it != names.begin() ) {
                normpath += "/";
            }
            normpath += *it;
        }
        return normpath;
    }
}

/**
 * Escape special chars in github action command properties.
 * See https://github.com/actions/toolkit/blob/main/packages/core/src/command.ts
 */
static std::string escape_property( std::string str )
{
    switch( error_log_format ) {
        case error_log_format_t::human_readable:
            break;
        case error_log_format_t::github_action:
            replace_substring( str, "%", "%25", true );
            replace_substring( str, "\r", "%0D", true ); // NOLINT(cata-text-style)
            replace_substring( str, "\n", "%0A", true );
            replace_substring( str, ":", "%3A", true );
            replace_substring( str, ",", "%2C", true );
            break;
    }
    return str;
}

/**
 * Escape special chars in github action command messages.
 * See https://github.com/actions/toolkit/blob/main/packages/core/src/command.ts
 */
static std::string escape_data( std::string str )
{
    switch( error_log_format ) {
        case error_log_format_t::human_readable:
            break;
        case error_log_format_t::github_action:
            replace_substring( str, "%", "%25", true );
            replace_substring( str, "\r", "%0D", true ); // NOLINT(cata-text-style)
            replace_substring( str, "\n", "%0A", true );
            break;
    }
    return str;
}

/* error display */

// WARNING: for occasional use only.
std::string TextJsonIn::line_number( int offset_modifier )
{
    const std::string name = escape_property( path ? normalize_relative_path( *path )
                             : "<unknown source file>" );
    if( stream && stream->eof() ) {
        switch( error_log_format ) {
            case error_log_format_t::human_readable:
                return name + ":EOF";
            case error_log_format_t::github_action:
                return "file=" + name + ",line=EOF";
        }
    } else if( !stream || stream->fail() ) {
        switch( error_log_format ) {
            case error_log_format_t::human_readable:
                return name + ":???";
            case error_log_format_t::github_action:
                return "file=" + name + ",line=???";
        }
    } // else stream is fine
    int pos = tell();
    int line = 1;
    int offset = 1;
    char ch;
    seek( 0 );
    for( int i = 0; i < pos + offset_modifier; ++i ) {
        stream->get( ch );
        if( !stream->good() ) {
            break;
        }
        if( ch == '\r' ) {
            offset = 1;
            ++line;
            if( peek() == '\n' ) {
                stream->get();
                ++i;
            }
        } else if( ch == '\n' ) {
            offset = 1;
            ++line;
        } else {
            ++offset;
        }
    }
    seek( pos );
    std::stringstream ret;
    switch( error_log_format ) {
        case error_log_format_t::human_readable:
            ret << name << ":" << line << ":" << offset;
            break;
        case error_log_format_t::github_action:
            ret.imbue( std::locale::classic() );
            ret << "file=" << name << ",line=" << line << ",col=" << offset;
            break;
    }
    return ret.str();
}

// see note at utf8_width_raw(...
static uint32_t UTF8_getch_raw( const char **src, int *srclen )
{
    const unsigned char *p = *reinterpret_cast<const unsigned char **>( src );
    int left = 0;
    bool overlong = false;
    bool underflow = false;
    uint32_t ch = UNKNOWN_UNICODE;

    if( *srclen == 0 ) {
        return UNKNOWN_UNICODE;
    }
    if( p[0] >= 0xFC ) {
        if( ( p[0] & 0xFE ) == 0xFC ) {
            if( p[0] == 0xFC && ( p[1] & 0xFC ) == 0x80 ) {
                overlong = true;
            }
            ch = static_cast<uint32_t>( p[0] & 0x01 );
            left = 5;
        }
    } else if( p[0] >= 0xF8 ) {
        if( ( p[0] & 0xFC ) == 0xF8 ) {
            if( p[0] == 0xF8 && ( p[1] & 0xF8 ) == 0x80 ) {
                overlong = true;
            }
            ch = static_cast<uint32_t>( p[0] & 0x03 );
            left = 4;
        }
    } else if( p[0] >= 0xF0 ) {
        if( ( p[0] & 0xF8 ) == 0xF0 ) {
            if( p[0] == 0xF0 && ( p[1] & 0xF0 ) == 0x80 ) {
                overlong = true;
            }
            ch = static_cast<uint32_t>( p[0] & 0x07 );
            left = 3;
        }
    } else if( p[0] >= 0xE0 ) {
        if( ( p[0] & 0xF0 ) == 0xE0 ) {
            if( p[0] == 0xE0 && ( p[1] & 0xE0 ) == 0x80 ) {
                overlong = true;
            }
            ch = static_cast<uint32_t>( p[0] & 0x0F );
            left = 2;
        }
    } else if( p[0] >= 0xC0 ) {
        if( ( p[0] & 0xE0 ) == 0xC0 ) {
            if( ( p[0] & 0xDE ) == 0xC0 ) {
                overlong = true;
            }
            ch = static_cast<uint32_t>( p[0] & 0x1F );
            left = 1;
        }
    } else {
        if( ( p[0] & 0x80 ) == 0x00 ) {
            ch = static_cast<uint32_t>( p[0] );
        }
    }
    ++*src;
    --*srclen;
    while( left > 0 && *srclen > 0 ) {
        ++p;
        if( ( p[0] & 0xC0 ) != 0x80 ) {
            ch = UNKNOWN_UNICODE;
            break;
        }
        ch <<= 6;
        ch |= ( p[0] & 0x3F );
        ++*src;
        --*srclen;
        --left;
    }
    if( left > 0 ) {
        underflow = true;
    }
    if( overlong || underflow ||
        ( ch >= 0xD800 && ch <= 0xDFFF ) ||
        ( ch == 0xFFFE || ch == 0xFFFF ) || ch > 0x10FFFF ) {
        ch = UNKNOWN_UNICODE;
    }
    return ch;
}

//Calculate width of a Unicode string
//Latin characters have a width of 1
//CJK characters have a width of 2, etc
// This is a copy of the function in catacharset.cpp but without
// dependencies on catacharset.h => output.h => more dependencies...
// TODO: untangle the dependencies and remove this copy and UTF8_getch_raw
static int utf8_width_raw( const std::string_view s )
{
    int len = s.size();
    const char *ptr = s.data();
    int w = 0;
    while( len > 0 ) {
        uint32_t ch = UTF8_getch_raw( &ptr, &len );
        if( ch == UNKNOWN_UNICODE ) {
            continue;
        }
        w += mk_wcwidth( ch );
    }
    return w;
}

void TextJsonIn::error( const std::string &message )
{
    error( 0, message );
}

void TextJsonIn::error( int offset, const std::string &message )
{
    std::ostringstream err_header;
    switch( error_log_format ) {
        case error_log_format_t::human_readable:
            err_header << "Json error: " << line_number( offset ) << ": ";
            break;
        case error_log_format_t::github_action:
            err_header << "::error " << line_number( offset ) << "::";
            break;
    }

    std::string color_normal;
    std::string color_error;
    std::string color_highlight;
    std::string color_end;
    switch( json_error_output_colors ) {
        case json_error_output_colors_t::color_tags:
            color_normal = "<color_white>";
            color_error = "<color_light_red>";
            color_highlight = "<color_cyan>";
            color_end = "</color>";
            break;
        case json_error_output_colors_t::ansi_escapes:
            color_normal = "\033[0m";
            color_error = "\033[0;31m";
            color_highlight = "\033[0;36m";
            color_end = "\033[0m";
            break;
        case json_error_output_colors_t::unset:
            err_header << "( json_error_output_colors is unset, defaulting to no colors ) ";
            break;
        default:
            // leave color strings empty
            break;
    }

    // if we can't get more info from the stream don't try
    if( !stream->good() ) {
        throw JsonError( err_header.str() + escape_data( message ) );
    }
    // Seek to eof after throwing to avoid continue reading from the incorrect
    // location. The calling code of json error methods is supposed to restore
    // the stream location if it wishes to recover from the error.
    on_out_of_scope seek_to_eof( [this]() {
        stream->seekg( 0, std::istream::end );
    } );
    std::ostringstream err;
    err << color_normal << color_highlight << message << color_end << "\n\n";
    const int max_context_lines = 3;   // limits context length to this many lines
    const int max_context_chars = 480; // limits context length to this many chars
    stream->seekg( offset, std::istream::cur );

    // if offset points to middle of a codepoint then rewind stream to codepoint start
    // this is so utf8_width below has correct utf-8 to count length.
    // pattern of 10xxxxxx means the byte is utf-8 continuation byte.
    while( ( stream->tellg() > 0 ) && ( ( stream->peek() & 0xC0 ) == 0x80 ) ) {
        stream->seekg( -1, std::istream::cur );
    }

    // remember positions of several places to print a few lines of context
    const size_t cursor_pos = tell();               // exact position of error
    rewind( 1, max_context_chars );                 // rewind to start of line
    const size_t start_of_line = tell();            // start of error line
    rewind( max_context_lines, max_context_chars ); // rewind a couple lines to show context
    const size_t start_of_context = tell();         // start of context (couple lines above)
    size_t end_of_line;                             // end of error line (without \n)

    {
        // find end of line and store in end_of_line
        seek( cursor_pos );
        end_of_line = tell();
        while( stream->peek() != EOF ) {
            if( stream->peek() == '\n' ) {
                break;
            }
            stream->get();
            end_of_line = tell();
        }
    }
    {
        // print context lines to start of error line
        seek( start_of_context );
        std::string buffer( start_of_line - start_of_context, '\0' );
        stream->read( buffer.data(), start_of_line - start_of_context );
        err << buffer;
    }
    {
        // print error line
        std::string buffer( end_of_line - start_of_line, '\0' );
        stream->read( buffer.data(), end_of_line - start_of_line );
        err << color_error << buffer << color_end << "\n";
    }
    {
        seek( start_of_line );
        std::string buffer( cursor_pos - start_of_line, '\0' );
        stream->read( buffer.data(), cursor_pos - start_of_line );

        // display a cursor at the position if possible, or at start of line
        if( const int padding = std::max( 0, utf8_width_raw( buffer ) - 1 ); padding > 0 ) {
            err << std::string( padding, ' ' );
        }
        err << color_highlight << "▲▲▲" << color_end;
    }
    seek( end_of_line );
    // print the next couple lines as well
    int line_count = 0;
    char ch = 0;
    for( int i = 0; line_count < max_context_lines && stream->good() && i < max_context_chars; ++i ) {
        stream->get( ch );
        if( !stream->good() ) {
            break;
        }
        if( ch == '\r' ) {
            ch = '\n';
            ++line_count;
            if( stream->peek() == '\n' ) {
                stream->get( ch );
            }
        } else if( ch == '\n' ) {
            ++line_count;
        }
        err << ch;
    }
    err << color_end << "\n";
    throw JsonError( err_header.str() + escape_data( err.str() ) );
}

void TextJsonIn::string_error( const int offset, const std::string &message )
{
    if( test_string() ) {
        // skip quote mark
        stream->ignore();
        std::string s;
        std::string err;
        for( int i = 0; i < offset; ++i ) {
            if( !get_escaped_or_unicode( *stream, s, err ) ) {
                break;
            }
        }
    }
    error( -1, message );
}

bool TextJsonIn::error_or_false( bool throw_, const std::string &message )
{
    return error_or_false( throw_, 0, message );
}

bool TextJsonIn::error_or_false( bool throw_, int offset, const std::string &message )
{
    if( throw_ ) {
        error( offset, message );
    }
    return false;
}

void TextJsonIn::rewind( int max_lines, int max_chars )
{
    if( max_lines < 0 && max_chars < 0 ) {
        // just rewind to the beginning i guess
        seek( 0 );
        return;
    }
    if( tell() == 0 ) {
        return;
    }
    int lines_found = 0;
    stream->seekg( -1, std::istream::cur );
    for( int i = 0; i < max_chars; ++i ) {
        size_t tellpos = tell();
        if( peek() == '\n' ) {
            ++lines_found;
            if( tellpos > 0 ) {
                stream->seekg( -1, std::istream::cur );
                if( peek() != '\r' ) {
                    stream->seekg( 1, std::istream::cur );
                } else {
                    --tellpos;
                }
            }
        } else if( peek() == '\r' ) {
            ++lines_found;
        }
        if( lines_found == max_lines ) {
            // don't include the last \n or \r
            if( peek() == '\n' ) {
                stream->seekg( 1, std::istream::cur );
            } else if( peek() == '\r' ) {
                stream->seekg( 1, std::istream::cur );
                if( peek() == '\n' ) {
                    stream->seekg( 1, std::istream::cur );
                }
            }
            break;
        } else if( tellpos == 0 ) {
            break;
        }
        stream->seekg( -1, std::istream::cur );
    }
}

std::string TextJsonIn::substr( size_t pos, size_t len )
{
    std::string ret;
    if( len == std::string::npos ) {
        stream->seekg( 0, std::istream::end );
        size_t end = tell();
        len = end - pos;
    }
    ret.resize( len );
    stream->seekg( pos );
    stream->read( ret.data(), len );
    return ret;
}

JsonOut::JsonOut( std::ostream &s, bool pretty, int depth ) :
    stream( &s ), pretty_print( pretty ), indent_level( depth )
{
    // ensure consistent and locale-independent formatting of numerals
    stream->imbue( std::locale::classic() );
    stream->setf( std::ios_base::showpoint );
    stream->setf( std::ios_base::dec, std::ostream::basefield );
    stream->setf( std::ios_base::fixed, std::ostream::floatfield );

    // automatically stringify bool to "true" or "false"
    stream->setf( std::ios_base::boolalpha );
}

int JsonOut::tell()
{
    return stream->tellp();
}

void JsonOut::seek( int pos )
{
    stream->clear();
    stream->seekp( pos );
    need_separator = false;
}

void JsonOut::write_indent()
{
    std::fill_n( std::ostream_iterator<char>( *stream ), indent_level * 2, ' ' );
}

void JsonOut::write_separator()
{
    if( !need_separator ) {
        return;
    }
    stream->put( ',' );
    if( pretty_print ) {
        // Wrap after separator between objects and between members of top-level objects.
        if( indent_level < 2 || need_wrap.back() ) {
            stream->put( '\n' );
            write_indent();
        } else {
            // Otherwise pad after commas.
            stream->put( ' ' );
        }
    }
    need_separator = false;
}

void JsonOut::write_member_separator()
{
    if( pretty_print ) {
        stream->write( ": ", 2 );
    } else {
        stream->put( ':' );
    }
    need_separator = false;
}

void JsonOut::start_pretty()
{
    if( pretty_print ) {
        indent_level += 1;
        // Wrap after top level object and array opening.
        if( indent_level < 2 || need_wrap.back() ) {
            stream->put( '\n' );
            write_indent();
        } else {
            // Otherwise pad after opening.
            stream->put( ' ' );
        }
    }
}

void JsonOut::end_pretty()
{
    if( pretty_print ) {
        indent_level -= 1;
        // Wrap after ending top level array and object.
        // Also wrap in the special case of exiting an array containing an object.
        if( indent_level < 1 || need_wrap.back() ) {
            stream->put( '\n' );
            write_indent();
        } else {
            // Otherwise pad after ending.
            stream->put( ' ' );
        }
    }
}

void JsonOut::start_object( bool wrap )
{
    if( need_separator ) {
        write_separator();
    }
    stream->put( '{' );
    need_wrap.push_back( wrap );
    start_pretty();
    need_separator = false;
}

void JsonOut::end_object()
{
    end_pretty();
    need_wrap.pop_back();
    stream->put( '}' );
    need_separator = true;
}

void JsonOut::start_array( bool wrap )
{
    if( need_separator ) {
        write_separator();
    }
    stream->put( '[' );
    need_wrap.push_back( wrap );
    start_pretty();
    need_separator = false;
}

void JsonOut::end_array()
{
    end_pretty();
    need_wrap.pop_back();
    stream->put( ']' );
    need_separator = true;
}

void JsonOut::write_null()
{
    if( need_separator ) {
        write_separator();
    }
    stream->write( "null", 4 );
    need_separator = true;
}

void JsonOut::write( const std::string_view val )
{
    if( need_separator ) {
        write_separator();
    }
    stream->put( '"' );
    for( const char &i : val ) {
        unsigned char ch = i;
        if( ch == '"' ) {
            stream->write( "\\\"", 2 );
        } else if( ch == '\\' ) {
            stream->write( "\\\\", 2 );
        } else if( ch == '/' ) {
            // don't technically need to escape this
            stream->put( '/' );
        } else if( ch == '\b' ) {
            stream->write( "\\b", 2 );
        } else if( ch == '\f' ) {
            stream->write( "\\f", 2 );
        } else if( ch == '\n' ) {
            stream->write( "\\n", 2 );
        } else if( ch == '\r' ) {
            stream->write( "\\r", 2 );
        } else if( ch == '\t' ) {
            stream->write( "\\t", 2 );
        } else if( ch < 0x20 ) {
            // convert to "\uxxxx" unicode escape
            stream->write( "\\u00", 4 );
            stream->put( ( ch < 0x10 ) ? '0' : '1' );
            char remainder = ch & 0x0F;
            if( remainder < 0x0A ) {
                stream->put( '0' + remainder );
            } else {
                stream->put( 'A' + ( remainder - 0x0A ) );
            }
        } else {
            stream->put( ch );
        }
    }
    stream->put( '"' );
    need_separator = true;
}

template<size_t N>
void JsonOut::write( const std::bitset<N> &b )
{
    if( need_separator ) {
        write_separator();
    }
    std::string converted = b.to_string();
    stream->put( '"' );
    for( char &i : converted ) {
        unsigned char ch = i;
        stream->put( ch );
    }
    stream->put( '"' );
    need_separator = true;
}

void JsonOut::member( const std::string_view name )
{
    write( name );
    write_member_separator();
}

void JsonOut::null_member( const std::string_view name )
{
    member( name );
    write_null();
}

JsonError::JsonError( const std::string &msg )
    : std::runtime_error( msg )
{
}

std::ostream &operator<<( std::ostream &stream, const JsonError &err )
{
    return stream << err.what();
}

// Need to instantiate those template to make them available for other compilation units.
// Currently only bitsets of size 12 are loaded / stored, if you need other sizes, either explicitly
// instantiate them here, or move the templated read/write functions into the header.
template void JsonOut::write<12>( const std::bitset<12> & );
template bool TextJsonIn::read<12>( std::bitset<12> &, bool throw_on_error );

TextJsonIn &TextJsonValue::seek() const
{
    jsin_.seek( pos_ );
    return jsin_;
}

TextJsonValue TextJsonObject::get_member( const std::string_view name ) const
{
    const auto iter = positions.find( name );
    if( !jsin || iter == positions.end() ) {
        throw_error( str_cat( "missing required field \"", name, "\" in object: ", str() ) );
    }
    mark_visited( name );
    return TextJsonValue( *jsin, iter->second );
}
