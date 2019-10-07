#include "json.h"

#include <cstdint>
#include <cstdio>
#include <cmath> // pow
#include <cstdlib> // strtoul
#include <cstring> // strcmp
#include <locale> // ensure user's locale doesn't interfere with output
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <bitset>
#include <iterator>
#include <algorithm>
#include <exception>
#include <utility>

// JSON parsing and serialization tools for Cataclysm-DDA.
// For documentation, see the included header, json.h.

static bool is_whitespace( char ch )
{
    // These are all the valid whitespace characters allowed by RFC 4627.
    return ( ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r' );
}

// for parsing \uxxxx escapes
static std::string utf16_to_utf8( uint32_t ch )
{
    char out[5];
    char *buf = out;
    static const unsigned char utf8FirstByte[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
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
        std::stringstream err;
        err << "unknown unicode: " << ch;
        throw std::runtime_error( err.str() );
    }

    buf += utf8Bytes;
    switch( utf8Bytes ) {
        case 4:
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
    return out;
}

/* class JsonObject
 * represents a JSON object,
 * providing access to the underlying data.
 */
JsonObject::JsonObject( JsonIn &j )
{
    jsin = &j;
    start = jsin->tell();
    // cache the position of the value for each member
    jsin->start_object();
    while( !jsin->end_object() ) {
        std::string n = jsin->get_member_name();
        int p = jsin->tell();
        if( n != "//" && n != "comment" && positions.count( n ) > 0 ) {
            // members with name "//" or "comment" are used for comments and
            // should be ignored anyway.
            j.error( "duplicate entry in json object" );
        }
        positions[n] = p;
        jsin->skip_value();
    }
    end = jsin->tell();
    final_separator = jsin->get_ate_separator();
}

JsonObject::JsonObject( const JsonObject &jo )
{
    jsin = jo.jsin;
    start = jo.start;
    positions = jo.positions;
    end = jo.end;
    final_separator = jo.final_separator;
}

JsonObject &JsonObject::operator=( const JsonObject &jo ) = default;

void JsonObject::finish()
{
    if( jsin && jsin->good() ) {
        jsin->seek( end );
        jsin->set_ate_separator( final_separator );
    }
}

size_t JsonObject::size()
{
    return positions.size();
}
bool JsonObject::empty()
{
    return positions.empty();
}

int JsonObject::verify_position( const std::string &name,
                                 const bool throw_exception )
{
    int pos = positions[name]; // initialized to 0 if it doesn't exist
    if( pos > start ) {
        return pos;
    } else if( throw_exception && !jsin ) {
        throw JsonError( std::string( "member lookup on empty object: " ) + name );
    } else if( throw_exception ) {
        jsin->seek( start );
        jsin->error( "member not found: " + name );
    }
    // 0 is always the opening brace,
    // so it will never indicate a valid member position
    return 0;
}

bool JsonObject::has_member( const std::string &name )
{
    return static_cast<bool>( verify_position( name, false ) );
}

std::set<std::string> JsonObject::get_member_names()
{
    std::set<std::string> ret;
    for( auto &elem : positions ) {
        ret.insert( elem.first );
    }
    return ret;
}

std::string JsonObject::line_number()
{
    jsin->seek( start );
    return jsin->line_number();
}

std::string JsonObject::str()
{
    if( jsin ) {
        return jsin->substr( start, end - start );
    } else {
        return "{}";
    }
}

void JsonObject::throw_error( std::string err, const std::string &name )
{
    jsin->seek( verify_position( name, false ) );
    jsin->error( err );
}

void JsonArray::throw_error( std::string err )
{
    jsin->error( err );
}

void JsonArray::throw_error( std::string err, int idx )
{
    if( idx >= 0 && size_t( idx ) < positions.size() ) {
        jsin->seek( positions[idx] );
    }
    jsin->error( err );
}

void JsonObject::throw_error( std::string err )
{
    jsin->error( err );
}

JsonIn *JsonObject::get_raw( const std::string &name )
{
    int pos = verify_position( name );
    jsin->seek( pos );
    return jsin;
}

/* returning values by name */

bool JsonObject::get_bool( const std::string &name )
{
    int pos = verify_position( name );
    jsin->seek( pos );
    return jsin->get_bool();
}

bool JsonObject::get_bool( const std::string &name, const bool fallback )
{
    int pos = positions[name];
    if( pos <= start ) {
        return fallback;
    }
    jsin->seek( pos );
    return jsin->get_bool();
}

int JsonObject::get_int( const std::string &name )
{
    int pos = verify_position( name );
    jsin->seek( pos );
    return jsin->get_int();
}

int JsonObject::get_int( const std::string &name, const int fallback )
{
    int pos = positions[name];
    if( pos <= start ) {
        return fallback;
    }
    jsin->seek( pos );
    return jsin->get_int();
}

double JsonObject::get_float( const std::string &name )
{
    int pos = verify_position( name );
    jsin->seek( pos );
    return jsin->get_float();
}

double JsonObject::get_float( const std::string &name, const double fallback )
{
    int pos = positions[name];
    if( pos <= start ) {
        return fallback;
    }
    jsin->seek( pos );
    return jsin->get_float();
}

std::string JsonObject::get_string( const std::string &name )
{
    int pos = verify_position( name );
    jsin->seek( pos );
    return jsin->get_string();
}

std::string JsonObject::get_string( const std::string &name, const std::string &fallback )
{
    int pos = positions[name];
    if( pos <= start ) {
        return fallback;
    }
    jsin->seek( pos );
    return jsin->get_string();
}

/* returning containers by name */

JsonArray JsonObject::get_array( const std::string &name )
{
    int pos = positions[name];
    if( pos <= start ) {
        return JsonArray(); // empty array
    }
    jsin->seek( pos );
    return JsonArray( *jsin );
}

std::vector<int> JsonObject::get_int_array( const std::string &name )
{
    JsonArray ja = get_array( name );
    std::vector<int> ret;
    while( ja.has_more() ) {
        ret.push_back( ja.next_int() );
    }
    return ret;
}

std::vector<std::string> JsonObject::get_string_array( const std::string &name )
{
    JsonArray ja = get_array( name );
    std::vector<std::string> ret;
    while( ja.has_more() ) {
        ret.push_back( ja.next_string() );
    }
    return ret;
}

JsonObject JsonObject::get_object( const std::string &name )
{
    int pos = positions[name];
    if( pos <= start ) {
        return JsonObject(); // empty object
    }
    jsin->seek( pos );
    return jsin->get_object();
}

/* non-fatal member existence and type testing */

bool JsonObject::has_null( const std::string &name )
{
    int pos = verify_position( name, false );
    if( !pos ) {
        return false;
    }
    jsin->seek( pos );
    return jsin->test_null();
}

bool JsonObject::has_bool( const std::string &name )
{
    int pos = verify_position( name, false );
    if( !pos ) {
        return false;
    }
    jsin->seek( pos );
    return jsin->test_bool();
}

bool JsonObject::has_number( const std::string &name )
{
    int pos = verify_position( name, false );
    if( !pos ) {
        return false;
    }
    jsin->seek( pos );
    return jsin->test_number();
}

bool JsonObject::has_string( const std::string &name )
{
    int pos = verify_position( name, false );
    if( !pos ) {
        return false;
    }
    jsin->seek( pos );
    return jsin->test_string();
}

bool JsonObject::has_array( const std::string &name )
{
    int pos = verify_position( name, false );
    if( !pos ) {
        return false;
    }
    jsin->seek( pos );
    return jsin->test_array();
}

bool JsonObject::has_object( const std::string &name )
{
    int pos = verify_position( name, false );
    if( !pos ) {
        return false;
    }
    jsin->seek( pos );
    return jsin->test_object();
}

/* class JsonArray
 * represents a JSON array,
 * providing access to the underlying data.
 */
JsonArray::JsonArray( JsonIn &j )
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
    end = jsin->tell();
    final_separator = jsin->get_ate_separator();
}

JsonArray::JsonArray( const JsonArray &ja )
{
    jsin = ja.jsin;
    start = ja.start;
    index = 0;
    positions = ja.positions;
    end = ja.end;
    final_separator = ja.final_separator;
}

JsonArray &JsonArray::operator=( const JsonArray &ja )
{
    jsin = ja.jsin;
    start = ja.start;
    index = 0;
    positions = ja.positions;
    end = ja.end;
    final_separator = ja.final_separator;

    return *this;
}

void JsonArray::finish()
{
    if( jsin && jsin->good() ) {
        jsin->seek( end );
        jsin->set_ate_separator( final_separator );
    }
}

bool JsonArray::has_more()
{
    return ( index >= 0 && size_t( index ) < positions.size() );
}
size_t JsonArray::size() const
{
    return positions.size();
}
bool JsonArray::empty()
{
    return positions.empty();
}

std::string JsonArray::str()
{
    if( jsin ) {
        return jsin->substr( start, end - start );
    } else {
        return "[]";
    }
}

void JsonArray::verify_index( int i )
{
    if( !jsin ) {
        throw JsonError( "tried to access empty array." );
    } else if( i < 0 || size_t( i ) >= positions.size() ) {
        jsin->seek( start );
        std::stringstream err;
        err << "bad index value: " << i;
        jsin->error( err.str() );
    }
}

/* iterative access */

bool JsonArray::next_bool()
{
    verify_index( index );
    jsin->seek( positions[index++] );
    return jsin->get_bool();
}

int JsonArray::next_int()
{
    verify_index( index );
    jsin->seek( positions[index++] );
    return jsin->get_int();
}

double JsonArray::next_float()
{
    verify_index( index );
    jsin->seek( positions[index++] );
    return jsin->get_float();
}

std::string JsonArray::next_string()
{
    verify_index( index );
    jsin->seek( positions[index++] );
    return jsin->get_string();
}

JsonArray JsonArray::next_array()
{
    verify_index( index );
    jsin->seek( positions[index++] );
    return jsin->get_array();
}

JsonObject JsonArray::next_object()
{
    verify_index( index );
    jsin->seek( positions[index++] );
    return jsin->get_object();
}

void JsonArray::skip_value()
{
    verify_index( index );
    ++index;
}

/* static access */

bool JsonArray::get_bool( int i )
{
    verify_index( i );
    jsin->seek( positions[i] );
    return jsin->get_bool();
}

int JsonArray::get_int( int i )
{
    verify_index( i );
    jsin->seek( positions[i] );
    return jsin->get_int();
}

double JsonArray::get_float( int i )
{
    verify_index( i );
    jsin->seek( positions[i] );
    return jsin->get_float();
}

std::string JsonArray::get_string( int i )
{
    verify_index( i );
    jsin->seek( positions[i] );
    return jsin->get_string();
}

JsonArray JsonArray::get_array( int i )
{
    verify_index( i );
    jsin->seek( positions[i] );
    return jsin->get_array();
}

JsonObject JsonArray::get_object( int i )
{
    verify_index( i );
    jsin->seek( positions[i] );
    return jsin->get_object();
}

/* iterative type checking */

bool JsonArray::test_null()
{
    if( !has_more() ) {
        return false;
    }
    jsin->seek( positions[index] );
    return jsin->test_null();
}

bool JsonArray::test_bool()
{
    if( !has_more() ) {
        return false;
    }
    jsin->seek( positions[index] );
    return jsin->test_bool();
}

bool JsonArray::test_number()
{
    if( !has_more() ) {
        return false;
    }
    jsin->seek( positions[index] );
    return jsin->test_number();
}

bool JsonArray::test_string()
{
    if( !has_more() ) {
        return false;
    }
    jsin->seek( positions[index] );
    return jsin->test_string();
}

bool JsonArray::test_bitset()
{
    if( !has_more() ) {
        return false;
    }
    jsin->seek( positions[index] );
    return jsin->test_bitset();
}

bool JsonArray::test_array()
{
    if( !has_more() ) {
        return false;
    }
    jsin->seek( positions[index] );
    return jsin->test_array();
}

bool JsonArray::test_object()
{
    if( !has_more() ) {
        return false;
    }
    jsin->seek( positions[index] );
    return jsin->test_object();
}

/* random-access type checking */

bool JsonArray::has_null( int i )
{
    verify_index( i );
    jsin->seek( positions[i] );
    return jsin->test_null();
}

bool JsonArray::has_bool( int i )
{
    verify_index( i );
    jsin->seek( positions[i] );
    return jsin->test_bool();
}

bool JsonArray::has_number( int i )
{
    verify_index( i );
    jsin->seek( positions[i] );
    return jsin->test_number();
}

bool JsonArray::has_string( int i )
{
    verify_index( i );
    jsin->seek( positions[i] );
    return jsin->test_string();
}

bool JsonArray::has_array( int i )
{
    verify_index( i );
    jsin->seek( positions[i] );
    return jsin->test_array();
}

bool JsonArray::has_object( int i )
{
    verify_index( i );
    jsin->seek( positions[i] );
    return jsin->test_object();
}

void add_array_to_set( std::set<std::string> &s, JsonObject &json, const std::string &name )
{
    JsonArray jarr = json.get_array( name );
    while( jarr.has_more() ) {
        s.insert( jarr.next_string() );
    }
}

int JsonIn::tell()
{
    return stream->tellg();
}
char JsonIn::peek()
{
    return static_cast<char>( stream->peek() );
}
bool JsonIn::good()
{
    return stream->good();
}

void JsonIn::seek( int pos )
{
    stream->clear();
    stream->seekg( pos );
    ate_separator = false;
}

void JsonIn::eat_whitespace()
{
    while( is_whitespace( peek() ) ) {
        stream->get();
    }
}

void JsonIn::uneat_whitespace()
{
    while( tell() > 0 ) {
        stream->seekg( -1, std::istream::cur );
        if( !is_whitespace( peek() ) ) {
            break;
        }
    }
}

void JsonIn::end_value()
{
    ate_separator = false;
    skip_separator();
}

void JsonIn::skip_member()
{
    skip_string();
    skip_pair_separator();
    skip_value();
}

void JsonIn::skip_separator()
{
    eat_whitespace();
    signed char ch = peek();
    if( ch == ',' ) {
        if( ate_separator ) {
            error( "duplicate separator" );
        }
        stream->get();
        ate_separator = true;
    } else if( ch == ']' || ch == '}' || ch == ':' ) {
        // okay
        if( ate_separator ) {
            std::stringstream err;
            err << "separator should not be found before '" << ch << "'";
            uneat_whitespace();
            error( err.str() );
        }
        ate_separator = false;
    } else if( ch == EOF ) {
        // that's okay too... probably
        if( ate_separator ) {
            uneat_whitespace();
            error( "separator at end of file not strictly allowed" );
        }
        ate_separator = false;
    } else {
        // not okay >:(
        uneat_whitespace();
        error( "missing separator", 1 );
    }
}

void JsonIn::skip_pair_separator()
{
    char ch;
    eat_whitespace();
    stream->get( ch );
    if( ch != ':' ) {
        std::stringstream err;
        err << "expected pair separator ':', not '" << ch << "'";
        error( err.str(), -1 );
    } else if( ate_separator ) {
        error( "duplicate separator not strictly allowed", -1 );
    }
    ate_separator = true;
}

void JsonIn::skip_string()
{
    char ch;
    eat_whitespace();
    stream->get( ch );
    if( ch != '"' ) {
        std::stringstream err;
        err << "expecting string but found '" << ch << "'";
        error( err.str(), -1 );
    }
    while( stream->good() ) {
        stream->get( ch );
        if( ch == '\\' ) {
            stream->get( ch );
            continue;
        } else if( ch == '"' ) {
            break;
        } else if( ch == '\r' || ch == '\n' ) {
            error( "string not closed before end of line", -1 );
        }
    }
    end_value();
}

void JsonIn::skip_value()
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

void JsonIn::skip_object()
{
    start_object();
    while( !end_object() ) {
        skip_member();
    }
    // end_value called by end_object
}

void JsonIn::skip_array()
{
    start_array();
    while( !end_array() ) {
        skip_value();
    }
    // end_value called by end_array
}

void JsonIn::skip_true()
{
    char text[5];
    eat_whitespace();
    stream->get( text, 5 );
    if( strcmp( text, "true" ) != 0 ) {
        std::stringstream err;
        err << R"(expected "true", but found ")" << text << "\"";
        error( err.str(), -4 );
    }
    end_value();
}

void JsonIn::skip_false()
{
    char text[6];
    eat_whitespace();
    stream->get( text, 6 );
    if( strcmp( text, "false" ) != 0 ) {
        std::stringstream err;
        err << R"(expected "false", but found ")" << text << "\"";
        error( err.str(), -5 );
    }
    end_value();
}

void JsonIn::skip_null()
{
    char text[5];
    eat_whitespace();
    stream->get( text, 5 );
    if( strcmp( text, "null" ) != 0 ) {
        std::stringstream err;
        err << R"(expected "null", but found ")" << text << "\"";
        error( err.str(), -4 );
    }
    end_value();
}

void JsonIn::skip_number()
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

std::string JsonIn::get_member_name()
{
    std::string s = get_string();
    skip_pair_separator();
    return s;
}

std::string JsonIn::get_string()
{
    std::string s;
    char ch;
    bool backslash = false;
    char unihex[5] = "0000";
    eat_whitespace();
    int startpos = tell();
    // the first character had better be a '"'
    stream->get( ch );
    if( ch != '"' ) {
        std::stringstream err;
        err << "expecting string but got '" << ch << "'";
        error( err.str(), -1 );
    }
    // add chars to the string, one at a time, converting:
    // \", \\, \/, \b, \f, \n, \r, \t and \uxxxx according to JSON spec.
    while( stream->good() ) {
        stream->get( ch );
        if( ch == '\\' ) {
            if( backslash ) {
                s += '\\';
                backslash = false;
            } else {
                backslash = true;
                continue;
            }
        } else if( backslash ) {
            backslash = false;
            if( ch == '"' ) {
                s += '"';
            } else if( ch == '/' ) {
                s += '/';
            } else if( ch == 'b' ) {
                s += '\b';
            } else if( ch == 'f' ) {
                s += '\f';
            } else if( ch == 'n' ) {
                s += '\n';
            } else if( ch == 'r' ) {
                s += '\r';
            } else if( ch == 't' ) {
                s += '\t';
            } else if( ch == 'u' ) {
                // get the next four characters as hexadecimal
                stream->get( unihex, 5 );
                // insert the appropriate unicode character in utf8
                // TODO: verify that unihex is in fact 4 hex digits.
                char **endptr = nullptr;
                uint32_t u = static_cast<uint32_t>( strtoul( unihex, endptr, 16 ) );
                try {
                    s += utf16_to_utf8( u );
                } catch( const std::exception &err ) {
                    error( err.what() );
                }
            } else {
                // for anything else, just add the character, i suppose
                s += ch;
            }
        } else if( ch == '"' ) {
            // end of the string
            end_value();
            return s;
        } else if( ch == '\r' || ch == '\n' ) {
            error( "reached end of line without closing string", -1 );
        } else if( static_cast<unsigned char>( ch ) < 0x20 ) {
            error( "invalid character inside string", -1 );
        } else {
            s += ch;
        }
    }
    // if we get to here, probably hit a premature EOF?
    if( stream->eof() ) {
        stream->clear();
        seek( startpos );
        error( "couldn't find end of string, reached EOF." );
    } else if( stream->fail() ) {
        throw JsonError( "stream failure while reading string." );
    }
    throw JsonError( "something went wrong D:" );
}

int JsonIn::get_int()
{
    // get float value and then convert to int,
    // because "1.359e3" is technically a valid integer.
    return static_cast<int>( get_float() );
}

std::int64_t JsonIn::get_int64()
{
    // get float value and then convert to int,
    // because "1.359e3" is technically a valid integer.
    return static_cast<int64_t>( get_float() );
}

double JsonIn::get_float()
{
    // this could maybe be prettier?
    char ch;
    bool neg = false;
    int i = 0;
    int e = 0;
    int mod_e = 0;
    eat_whitespace();
    stream->get( ch );
    if( ch == '-' ) {
        neg = true;
        stream->get( ch );
    } else if( ch != '.' && ( ch < '0' || ch > '9' ) ) {
        // not a valid float
        std::stringstream err;
        err << "expecting number but found '" << ch << "'";
        error( err.str(), -1 );
    }
    if( ch == '0' ) {
        // allow a single leading zero in front of a '.' or 'e'/'E'
        stream->get( ch );
        if( ch >= '0' && ch <= '9' ) {
            error( "leading zeros not strictly allowed", -1 );
        }
    }
    while( ch >= '0' && ch <= '9' ) {
        i *= 10;
        i += ( ch - '0' );
        stream->get( ch );
    }
    if( ch == '.' ) {
        stream->get( ch );
        while( ch >= '0' && ch <= '9' ) {
            i *= 10;
            i += ( ch - '0' );
            mod_e -= 1;
            stream->get( ch );
        }
    }
    if( neg ) {
        i *= -1;
    }
    if( ch == 'e' || ch == 'E' ) {
        stream->get( ch );
        neg = false;
        if( ch == '-' ) {
            neg = true;
            stream->get( ch );
        } else if( ch == '+' ) {
            stream->get( ch );
        }
        while( ch >= '0' && ch <= '9' ) {
            e *= 10;
            e += ( ch - '0' );
            stream->get( ch );
        }
        if( neg ) {
            e *= -1;
        }
    }
    // unget the final non-number character (probably a separator)
    stream->unget();
    end_value();
    // now put it all together!
    return i * std::pow( 10.0f, e + mod_e );
}

bool JsonIn::get_bool()
{
    char ch;
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
            err << R"(not a boolean. expected "true", but got ")";
            err << ch << text << "\"";
            error( err.str(), -4 );
        }
    } else if( ch == 'f' ) {
        stream->get( text, 5 );
        if( strcmp( text, "alse" ) == 0 ) {
            end_value();
            return false;
        } else {
            err << R"(not a boolean. expected "false", but got ")";
            err << ch << text << "\"";
            error( err.str(), -5 );
        }
    }
    err << "not a boolean value! expected 't' or 'f' but got '" << ch << "'";
    error( err.str(), -1 );
    throw JsonError( "warnings are silly" );
}

JsonObject JsonIn::get_object()
{
    return JsonObject( *this );
}
JsonArray JsonIn::get_array()
{
    return JsonArray( *this );
}

void JsonIn::start_array()
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

bool JsonIn::end_array()
{
    eat_whitespace();
    if( peek() == ']' ) {
        if( ate_separator ) {
            uneat_whitespace();
            error( "separator not strictly allowed at end of array" );
        }
        stream->get();
        end_value();
        return true;
    } else {
        // not the end yet, so just return false?
        return false;
    }
}

void JsonIn::start_object()
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

bool JsonIn::end_object()
{
    eat_whitespace();
    if( peek() == '}' ) {
        if( ate_separator ) {
            uneat_whitespace();
            error( "separator not strictly allowed at end of object" );
        }
        stream->get();
        end_value();
        return true;
    } else {
        // not the end yet, so just return false?
        return false;
    }
}

bool JsonIn::test_null()
{
    eat_whitespace();
    return peek() == 'n';
}

bool JsonIn::test_bool()
{
    eat_whitespace();
    const char ch = peek();
    return ch == 't' || ch == 'f';
}

bool JsonIn::test_number()
{
    eat_whitespace();
    const char ch = peek();
    return ch == '-' || ch == '+' || ch == '.' || ( ch >= '0' && ch <= '9' );
}

bool JsonIn::test_string()
{
    eat_whitespace();
    return peek() == '"';
}

bool JsonIn::test_bitset()
{
    eat_whitespace();
    return peek() == '"';
}

bool JsonIn::test_array()
{
    eat_whitespace();
    return peek() == '[';
}

bool JsonIn::test_object()
{
    eat_whitespace();
    return peek() == '{';
}

/* non-fatal value setting by reference */

bool JsonIn::read( bool &b, bool throw_on_error )
{
    if( !test_bool() ) {
        return error_or_false( throw_on_error, "Expected bool" );
    }
    b = get_bool();
    return true;
}

bool JsonIn::read( char &c, bool throw_on_error )
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    c = get_int();
    return true;
}

bool JsonIn::read( signed char &c, bool throw_on_error )
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    // TODO: test for overflow
    c = get_int();
    return true;
}

bool JsonIn::read( unsigned char &c, bool throw_on_error )
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    // TODO: test for overflow
    c = get_int();
    return true;
}

bool JsonIn::read( short unsigned int &s, bool throw_on_error )
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    // TODO: test for overflow
    s = get_int();
    return true;
}

bool JsonIn::read( short int &s, bool throw_on_error )
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    // TODO: test for overflow
    s = get_int();
    return true;
}

bool JsonIn::read( int &i, bool throw_on_error )
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    i = get_int();
    return true;
}

bool JsonIn::read( std::int64_t &i, bool throw_on_error )
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    i = get_int64();
    return true;
}

bool JsonIn::read( unsigned int &u, bool throw_on_error )
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    u = get_int();
    return true;
}

bool JsonIn::read( float &f, bool throw_on_error )
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    f = get_float();
    return true;
}

bool JsonIn::read( double &d, bool throw_on_error )
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    d = get_float();
    return true;
}

bool JsonIn::read( std::string &s, bool throw_on_error )
{
    if( !test_string() ) {
        return error_or_false( throw_on_error, "Expected string" );
    }
    s = get_string();
    return true;
}

template<size_t N>
bool JsonIn::read( std::bitset<N> &b, bool throw_on_error )
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

bool JsonIn::read( JsonDeserializer &j, bool throw_on_error )
{
    // can't know what type of json object it will deserialize from,
    // so just try to deserialize, catching any error.
    // TODO: non-verbose flag for JsonIn errors so try/catch is faster here
    try {
        j.deserialize( *this );
        return true;
    } catch( const JsonError & ) {
        if( throw_on_error ) {
            throw;
        }
        return false;
    }
}

/* error display */

// WARNING: for occasional use only.
std::string JsonIn::line_number( int offset_modifier )
{
    if( stream->eof() ) {
        return "EOF";
    } else if( stream->fail() ) {
        return "???";
    } // else stream is fine
    int pos = tell();
    int line = 1;
    int offset = 1;
    char ch;
    seek( 0 );
    for( int i = 0; i < pos; ++i ) {
        stream->get( ch );
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
    std::stringstream ret;
    ret << "line " << line << ":" << ( offset + offset_modifier );
    return ret.str();
}

void JsonIn::error( const std::string &message, int offset )
{
    std::ostringstream err;
    err << line_number( offset ) << ": " << message;
    // if we can't get more info from the stream don't try
    if( !stream->good() ) {
        throw JsonError( err.str() );
    }
    // also print surrounding few lines of context, if not too large
    err << "\n\n";
    stream->seekg( offset, std::istream::cur );
    size_t pos = tell();
    rewind( 3, 240 );
    size_t startpos = tell();
    std::string buffer( pos - startpos, '\0' );
    stream->read( &buffer[0], pos - startpos );
    err << buffer;
    if( !is_whitespace( peek() ) ) {
        err << peek();
    }
    // display a pointer to the position
    rewind( 1, 240 );
    startpos = tell();
    err << '\n';
    if( pos > startpos ) {
        err << std::string( pos - startpos - 1, ' ' );
    }
    err << "^\n";
    seek( pos );
    // if that wasn't the end of the line, continue underneath pointer
    char ch = stream->get();
    if( ch == '\r' ) {
        if( peek() == '\n' ) {
            stream->get();
        }
    } else if( ch == '\n' ) {
        // pass
    } else if( peek() != '\r' && peek() != '\n' ) {
        for( size_t i = 0; i < pos - startpos; ++i ) {
            err << ' ';
        }
    }
    // print the next couple lines as well
    int line_count = 0;
    for( int i = 0; i < 240; ++i ) {
        stream->get( ch );
        err << ch;
        if( ch == '\r' ) {
            ++line_count;
            if( peek() == '\n' ) {
                err << stream->get();
            }
        } else if( ch == '\n' ) {
            ++line_count;
        }
        if( line_count > 2 ) {
            break;
        }
    }
    throw JsonError( err.str() );
}

bool JsonIn::error_or_false( bool throw_, const std::string &message, int offset )
{
    if( throw_ ) {
        error( message, offset );
    }
    return false;
}

void JsonIn::rewind( int max_lines, int max_chars )
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
                // note: does not update tellpos or count a character
                if( peek() != '\r' ) {
                    continue;
                }
            }
        } else if( peek() == '\r' ) {
            ++lines_found;
        }
        if( tellpos == 0 ) {
            break;
        } else if( lines_found == max_lines ) {
            // don't include the last \n or \r
            stream->seekg( 1, std::istream::cur );
            break;
        }
        stream->seekg( -1, std::istream::cur );
    }
}

std::string JsonIn::substr( size_t pos, size_t len )
{
    std::string ret;
    if( len == std::string::npos ) {
        stream->seekg( 0, std::istream::end );
        size_t end = tell();
        len = end - pos;
    }
    ret.resize( len );
    stream->seekg( pos );
    stream->read( &ret[0], len );
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
        // Wrap after seperator between objects and between members of top-level objects.
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
        if( indent_level < 2 || need_wrap.back() ) {
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

void JsonOut::write( const std::string &val )
{
    if( need_separator ) {
        write_separator();
    }
    stream->put( '"' );
    for( const auto &i : val ) {
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
    for( auto &i : converted ) {
        unsigned char ch = i;
        stream->put( ch );
    }
    stream->put( '"' );
    need_separator = true;
}

void JsonOut::write( const JsonSerializer &thing )
{
    if( need_separator ) {
        write_separator();
    }
    thing.serialize( *this );
    need_separator = true;
}

void JsonOut::member( const std::string &name )
{
    write( name );
    write_member_separator();
}

void JsonOut::null_member( const std::string &name )
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
template bool JsonIn::read<12>( std::bitset<12> &, bool throw_on_error );
