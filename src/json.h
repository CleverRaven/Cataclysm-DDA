#pragma once
#ifndef CATA_SRC_JSON_H
#define CATA_SRC_JSON_H

#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "colony.h"
#include "enum_conversions.h"
#include "json_error.h"
#include "int_id.h"
#include "memory_fast.h"
#include "string_id.h"

/* Cataclysm-DDA homegrown JSON tools
 * copyright CC-BY-SA-3.0 2013 CleverRaven
 *
 * Consists of four JSON manipulation tools:
 * TextJsonIn - for low-level parsing of an input JSON stream
 * JsonOut - for outputting JSON
 * TextJsonObject - convenience-wrapper for reading JSON objects from a TextJsonIn
 * TextJsonArray - convenience-wrapper for reading JSON arrays from a TextJsonIn
 *
 * Further documentation can be found below.
 */

template<typename E>
class enum_bitset;
class TextJsonArray;
class TextJsonIn;
class TextJsonObject;
class TextJsonValue;
class item;

namespace cata
{
template<typename T>
class optional;
} // namespace cata

template<typename T, typename Enable = void>
struct key_from_json_string;

template<>
struct key_from_json_string<std::string, void> {
    std::string operator()( const std::string &s ) {
        return s;
    }
};

template<typename T>
struct key_from_json_string<string_id<T>, void> {
    string_id<T> operator()( const std::string &s ) {
        return string_id<T>( s );
    }
};

template<typename T>
struct key_from_json_string<int_id<T>, void> {
    int_id<T> operator()( const std::string &s ) {
        return int_id<T>( s );
    }
};

template<typename Enum>
struct key_from_json_string<Enum, std::enable_if_t<std::is_enum<Enum>::value>> {
    Enum operator()( const std::string &s ) {
        return io::string_to_enum<Enum>( s );
    }
};

struct number_sci_notation {
    bool negative = false;
    // AKA the significand
    uint64_t number = 0;
    // AKA the order of magnitude
    int64_t exp = 0;
};

struct json_source_location {
    shared_ptr_fast<std::string> path;
    int offset = 0;
};

class TextJsonValue
{
    private:
        TextJsonIn &jsin_;
        int pos_;

        TextJsonIn &seek() const;

    public:
        TextJsonValue( TextJsonIn &jsin, int pos ) : jsin_( jsin ), pos_( pos ) { }

        // NOLINTNEXTLINE(google-explicit-constructor)
        operator std::string() const;
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator int() const;
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator bool() const;
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator double() const;
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator TextJsonObject() const;
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator TextJsonArray() const;
        template<typename T>
        bool read( T &t, bool throw_on_error = false ) const;

        bool test_string() const;
        bool test_int() const;
        bool test_bool() const;
        bool test_float() const;
        bool test_object() const;
        bool test_array() const;
        bool test_null() const;

        std::string get_string() const;
        int get_int() const;
        bool get_bool() const;
        double get_float() const;
        TextJsonObject get_object() const;
        TextJsonArray get_array() const;
        // Consumes null value in the underlying JsonIn.
        void get_null() const;

        [[noreturn]] void string_error( const std::string &err ) const;
        [[noreturn]] void string_error( int offset, const std::string &err ) const;
        [[noreturn]] void throw_error( const std::string &err ) const;
        [[noreturn]] void throw_error( int offset, const std::string &err ) const;
        [[noreturn]] void throw_error_after( const std::string &err ) const;
};

/* TextJsonIn
 * ======
 *
 * The TextJsonIn class provides a wrapper around a std::istream,
 * with methods for reading JSON data directly from the stream.
 *
 * TextJsonObject and TextJsonArray provide higher-level wrappers,
 * and are a little easier to use in most cases,
 * but have the small overhead of indexing the members or elements before use.
 *
 * Typical TextJsonIn usage might be something like the following:
 *
 *     TextJsonIn jsin(myistream);
 *     // expecting an array of objects
 *     jsin.start_array(); // throws JsonError if array not found
 *     while (!jsin.end_array()) { // end_array returns false if not the end
 *         TextJsonObject jo = jsin.get_object();
 *         ... // load object using TextJsonObject methods
 *     }
 *
 * The array could have been loaded into a TextJsonArray for convenience.
 * Not doing so saves one full pass of the data,
 * as the element positions don't have to be read beforehand.
 *
 * If the JSON structure is not as expected,
 * verbose error messages are provided, indicating the problem,
 * and the exact line number and byte offset within the istream.
 *
 *
 * Single-Pass Loading
 * -------------------
 *
 * A TextJsonIn can also be used for single-pass loading,
 * by passing off members as they arrive according to their names.
 *
 * Typical usage might be:
 *
 *     TextJsonIn jsin(&myistream);
 *     // expecting an array of objects, to be mapped by id
 *     std::map<std::string,my_data_type> myobjects;
 *     jsin.start_array();
 *     while (!jsin.end_array()) {
 *         my_data_type myobject;
 *         jsin.start_object();
 *         while (!jsin.end_object()) {
 *             std::string name = jsin.get_member_name();
 *             if (name == "id") {
 *                 myobject.id = jsin.get_string();
 *             } else if (name == "name") {
 *                 myobject.name = _(jsin.get_string());
 *             } else if (name == "description") {
 *                 myobject.description = _(jsin.get_string());
 *             } else if (name == "points") {
 *                 myobject.points = jsin.get_int();
 *             } else if (name == "flags") {
 *                 if (jsin.test_string()) { // load single string tag
 *                     myobject.tags.insert(jsin.get_string());
 *                 } else { // load tag array
 *                     jsin.start_array();
 *                     while (!jsin.end_array()) {
 *                         myobject.tags.insert(jsin.get_string());
 *                     }
 *                 }
 *             } else {
 *                 jsin.skip_value();
 *             }
 *         }
 *         myobjects[myobject.id] = myobject;
 *     }
 *
 * The get_* methods automatically verify and skip separators and whitespace.
 *
 * Using this method, unrecognized members are silently ignored,
 * allowing for things like "comment" members,
 * but the user must remember to provide an "else" clause,
 * explicitly skipping them.
 *
 * If an if;else if;... is missing the "else", it /will/ cause bugs,
 * so preindexing as a TextJsonObject is safer, as well as tidier.
 */
class TextJsonIn
{
    private:
        std::istream *stream;
        shared_ptr_fast<std::string> path;
        bool ate_separator = false;

        void sanity_check_stream();
        void skip_separator();
        void skip_pair_separator();
        void end_value();

    public:
        explicit TextJsonIn( std::istream &s );
        TextJsonIn( std::istream &s, const std::string &path );
        TextJsonIn( std::istream &s, const json_source_location &loc );
        TextJsonIn( const TextJsonIn & ) = delete;
        TextJsonIn &operator=( const TextJsonIn & ) = delete;

        shared_ptr_fast<std::string> get_path() const {
            return path;
        }

        bool get_ate_separator() const {
            return ate_separator;
        }
        void set_ate_separator( bool s ) {
            ate_separator = s;
        }

        int tell(); // get current stream position
        void seek( int pos ); // seek to specified stream position
        char peek(); // what's the next char gonna be?
        bool good(); // whether stream is ok

        // advance seek head to the next non-whitespace character
        void eat_whitespace();
        // or rewind to the previous one
        void uneat_whitespace();

        // quick skipping for when values don't have to be parsed
        void skip_member();
        void skip_string();
        void skip_value();
        void skip_object();
        void skip_array();
        void skip_true();
        void skip_false();
        void skip_null();
        void skip_number();

        // data parsing
        std::string get_string(); // get the next value as a string
        int get_int(); // get the next value as an int
        unsigned int get_uint(); // get the next value as an unsigned int
        int64_t get_int64(); // get the next value as an int64
        uint64_t get_uint64(); // get the next value as a uint64
        bool get_bool(); // get the next value as a bool
        double get_float(); // get the next value as a double
        std::string get_member_name(); // also strips the ':'
        TextJsonObject get_object();
        TextJsonArray get_array();
        TextJsonValue get_value(); // just returns a TextJsonValue at the current position.

        template<typename E, typename = typename std::enable_if<std::is_enum<E>::value>::type>
        E get_enum_value() {
            const int old_offset = tell();
            try {
                return io::string_to_enum<E>( get_string() );
            } catch( const io::InvalidEnumString & ) {
                seek( old_offset ); // so the error message points to the correct place.
                error( "invalid enumeration value" );
            }
        }

        // container control and iteration
        void start_array(); // verify array start
        bool end_array(); // returns false if it's not the end
        void start_object();
        bool end_object(); // returns false if it's not the end

        // type testing
        bool test_null();
        bool test_bool();
        bool test_number();
        bool test_int() {
            return test_number();
        }
        bool test_float() {
            return test_number();
        }
        bool test_string();
        bool test_bitset();
        bool test_array();
        bool test_object();

        // optionally-fatal reading into values by reference
        // returns true if the data was read successfully, false otherwise
        // if throw_on_error then throws JsonError rather than returning false.
        bool read_null( bool throw_on_error = false );
        bool read( bool &b, bool throw_on_error = false );
        bool read( char &c, bool throw_on_error = false );
        bool read( signed char &c, bool throw_on_error = false );
        bool read( unsigned char &c, bool throw_on_error = false );
        bool read( short unsigned int &s, bool throw_on_error = false );
        bool read( short int &s, bool throw_on_error = false );
        bool read( int &i, bool throw_on_error = false );
        bool read( int64_t &i, bool throw_on_error = false );
        bool read( uint64_t &i, bool throw_on_error = false );
        bool read( unsigned int &u, bool throw_on_error = false );
        bool read( float &f, bool throw_on_error = false );
        bool read( double &d, bool throw_on_error = false );
        bool read( std::string &s, bool throw_on_error = false );
        template<size_t N>
        bool read( std::bitset<N> &b, bool throw_on_error = false );

        template <typename T>
        auto read( string_id<T> &thing, bool throw_on_error = false ) -> bool {
            std::string tmp;
            if( !read( tmp, throw_on_error ) ) {
                return false;
            }
            thing = string_id<T>( tmp );
            return true;
        }

        // This is for the int_id type
        template <typename T>
        auto read( int_id<T> &thing, bool throw_on_error = false ) -> bool {
            std::string tmp;
            if( !read( tmp, throw_on_error ) ) {
                return false;
            }
            thing = int_id<T>( tmp );
            return true;
        }

        /// Overload that calls a global function `deserialize(T&, const TextJsonValue&)`, if available.
        template<typename T>
        auto read( T &v, bool throw_on_error = false ) ->
        decltype( deserialize( v, std::declval<const TextJsonValue &>() ), true ) {
            try {
                deserialize( v, this->get_value() );
                return true;
            } catch( const JsonError & ) {
                if( throw_on_error ) {
                    throw;
                }
                return false;
            }
        }

        /// Overload that calls a member function `T::deserialize(const TextJsonValue&)`, if available.
        template<typename T>
        auto read( T &v, bool throw_on_error = false ) -> decltype( v.deserialize(
                    std::declval<const TextJsonValue &>() ), true ) {
            try {
                v.deserialize( this->get_value() );
                return true;
            } catch( const JsonError & ) {
                if( throw_on_error ) {
                    throw;
                }
                return false;
            }
        }

        template<typename T, std::enable_if_t<std::is_enum<T>::value, int> = 0>
        bool read( T &val, bool throw_on_error = false ) {
            int i;
            if( read( i, false ) ) {
                val = static_cast<T>( i );
                return true;
            }
            std::string s;
            if( read( s, throw_on_error ) ) {
                val = io::string_to_enum<T>( s );
                return true;
            }
            return false;
        }

        /// Overload for std::pair
        template<typename T, typename U>
        bool read( std::pair<T, U> &p, bool throw_on_error = false ) {
            if( !test_array() ) {
                return error_or_false( throw_on_error, "Expected json array encoding pair" );
            }
            try {
                start_array();
                bool result = !end_array() &&
                              read( p.first, throw_on_error ) &&
                              !end_array() &&
                              read( p.second, throw_on_error ) &&
                              end_array();
                if( !result && throw_on_error ) {
                    error( "Array had wrong number of elements for pair" );
                }
                return result;
            } catch( const JsonError & ) {
                if( throw_on_error ) {
                    throw;
                }
                return false;
            }
        }

        // array ~> vector, deque, list
        template < typename T, typename std::enable_if <
                       !std::is_same<void, typename T::value_type>::value >::type * = nullptr
                   >
        auto read( T &v, bool throw_on_error = false ) -> decltype( v.front(), true ) {
            if( !test_array() ) {
                return error_or_false( throw_on_error, "Expected json array" );
            }
            try {
                start_array();
                v.clear();
                while( !end_array() ) {
                    typename T::value_type element;
                    if( read( element, throw_on_error ) ) {
                        v.push_back( std::move( element ) );
                    } else {
                        skip_value();
                    }
                }
            } catch( const JsonError & ) {
                if( throw_on_error ) {
                    throw;
                }
                return false;
            }

            return true;
        }

        // array ~> array
        template <typename T, size_t N>
        bool read( std::array<T, N> &v, bool throw_on_error = false ) {
            if( !test_array() ) {
                return error_or_false( throw_on_error, "Expected json array" );
            }
            try {
                start_array();
                for( size_t i = 0; i < N; ++i ) {
                    if( end_array() ) {
                        if( throw_on_error ) {
                            error( "Json array is too short" );
                        }
                        return false; // json array is too small
                    }
                    if( !read( v[i], throw_on_error ) ) {
                        return false; // invalid entry
                    }
                }
                bool result = end_array();
                if( !result && throw_on_error ) {
                    error( "Array had too many elements" );
                }
                return result;
            } catch( const JsonError & ) {
                if( throw_on_error ) {
                    throw;
                }
                return false;
            }
        }

        // object ~> containers with matching key_type and value_type
        // set, unordered_set ~> object
        template <typename T, typename std::enable_if<
                      std::is_same<typename T::key_type, typename T::value_type>::value>::type * = nullptr
                  >
        bool read( T &v, bool throw_on_error = false ) {
            if( !test_array() ) {
                return error_or_false( throw_on_error, "Expected json array" );
            }
            try {
                start_array();
                v.clear();
                while( !end_array() ) {
                    typename T::value_type element;
                    if( read( element, throw_on_error ) ) {
                        if( !v.insert( std::move( element ) ).second ) {
                            return error_or_false(
                                       throw_on_error,
                                       "Duplicate entry in set defined by json array" );
                        }
                    } else {
                        skip_value();
                    }
                }
            } catch( const JsonError & ) {
                if( throw_on_error ) {
                    throw;
                }
                return false;
            }

            return true;
        }

        // special case for enum_bitset
        template <typename T>
        bool read( enum_bitset<T> &v, bool throw_on_error = false ) {
            if( !test_array() ) {
                return error_or_false( throw_on_error, "Expected json array" );
            }
            try {
                start_array();
                v = {};
                while( !end_array() ) {
                    T element;
                    if( read( element, throw_on_error ) ) {
                        if( v.test( element ) ) {
                            return error_or_false(
                                       throw_on_error,
                                       "Duplicate entry in set defined by json array" );
                        }
                        v.set( element );
                    } else {
                        skip_value();
                    }
                }
            } catch( const JsonError & ) {
                if( throw_on_error ) {
                    throw;
                }
                return false;
            }

            return true;
        }

        // special case for colony<item> as it supports RLE
        // see corresponding `write` for details
        template <typename T, std::enable_if_t<std::is_same<T, item>::value> * = nullptr >
        bool read( cata::colony<T> &v, bool throw_on_error = false ) {
            if( !test_array() ) {
                return error_or_false( throw_on_error, "Expected json array" );
            }
            try {
                start_array();
                v.clear();
                while( !end_array() ) {
                    T element;
                    const int prev_pos = tell();
                    if( test_array() ) {
                        start_array();
                        int run_l;
                        if( read( element, throw_on_error ) &&
                            read( run_l, throw_on_error ) &&
                            end_array()
                          ) { // all is good
                            // first insert (run_l-1) elements
                            for( int i = 0; i < run_l - 1; i++ ) {
                                v.insert( element );
                            }
                            // micro-optimization, move last element
                            v.insert( std::move( element ) );
                        } else { // array is malformed, skipping it entirely
                            error_or_false( throw_on_error, "Expected end of array" );
                            seek( prev_pos );
                            skip_array();
                        }
                    } else {
                        if( read( element, throw_on_error ) ) {
                            v.insert( std::move( element ) );
                        } else {
                            skip_value();
                        }
                    }
                }
            } catch( const JsonError & ) {
                if( throw_on_error ) {
                    throw;
                }
                return false;
            }

            return true;
        }

        // special case for colony as it uses `insert()` instead of `push_back()`
        // and therefore doesn't fit with vector/deque/list
        // for colony of items there is another specialization with RLE
        template < typename T, std::enable_if_t < !std::is_same<T, item>::value > * = nullptr >
        bool read( cata::colony<T> &v, bool throw_on_error = false ) {
            if( !test_array() ) {
                return error_or_false( throw_on_error, "Expected json array" );
            }
            try {
                start_array();
                v.clear();
                while( !end_array() ) {
                    T element;
                    if( read( element, throw_on_error ) ) {
                        v.insert( std::move( element ) );
                    } else {
                        skip_value();
                    }
                }
            } catch( const JsonError & ) {
                if( throw_on_error ) {
                    throw;
                }
                return false;
            }

            return true;
        }

        // object ~> containers with unmatching key_type and value_type
        // map, unordered_map ~> object
        template < typename T, typename std::enable_if <
                       !std::is_same<typename T::key_type, typename T::value_type>::value >::type * = nullptr
                   >
        bool read( T &m, bool throw_on_error = true ) {
            if( !test_object() ) {
                return error_or_false( throw_on_error, "Expected json object" );
            }
            try {
                start_object();
                m.clear();
                while( !end_object() ) {
                    using key_type = typename T::key_type;
                    key_type key = key_from_json_string<key_type>()( get_member_name() );
                    typename T::mapped_type element;
                    if( read( element, throw_on_error ) ) {
                        m[std::move( key )] = std::move( element );
                    } else {
                        skip_value();
                    }
                }
            } catch( const JsonError & ) {
                if( throw_on_error ) {
                    throw;
                }
                return false;
            }

            return true;
        }

        // error messages
        std::string line_number( int offset_modifier = 0 ); // for occasional use only
        [[noreturn]] void error( const std::string &message ); // ditto
        [[noreturn]] void error( int offset, const std::string &message ); // ditto
        // if the next element is a string, throw error after the `offset`th unicode
        // character in the parsed string. if `offset` is 0, throw error right after
        // the starting quotation mark.
        [[noreturn]] void string_error( int offset, const std::string &message );

        // If throw_, then call error( message, offset ), otherwise return
        // false
        bool error_or_false( bool throw_, const std::string &message );
        bool error_or_false( bool throw_, int offset, const std::string &message );
        void rewind( int max_lines = -1, int max_chars = -1 );
        std::string substr( size_t pos, size_t len = std::string::npos );
    private:
        // This should be used to get any and all numerical data types.
        number_sci_notation get_any_number();
        // Calls get_any_number() then applies operations common to all integer types.
        number_sci_notation get_any_int();
};

/* JsonOut
 * =======
 *
 * The JsonOut class provides a straightforward interface for outputting JSON.
 *
 * It wraps a std::ostream, providing methods for writing JSON data directly.
 *
 * Typical usage might be as follows:
 *
 *     JsonOut jsout(myostream);
 *     jsout.start_object();          // {
 *     jsout.member("type", "point"); // "type":"point"
 *     jsout.member("data");          // ,"data":
 *     jsout.start_array();           // [
 *     jsout.write(5)                 // 5
 *     jsout.write(9)                 // ,9
 *     jsout.end_array();             // ]
 *     jsout.end_object();            // }
 *
 * which writes {"type":"point","data":[5,9]} to myostream.
 *
 * Separators are handled automatically,
 * and the constructor also has an option for crude pretty-printing,
 * which inserts newlines and whitespace liberally, if turned on.
 *
 * Basic containers such as maps, sets and vectors,
 * can be serialized automatically by write() and member().
 */
class JsonOut
{
    private:
        std::ostream *stream;
        bool pretty_print;
        std::vector<bool> need_wrap;
        int indent_level = 0;
        bool need_separator = false;

    public:
        explicit JsonOut( std::ostream &stream, bool pretty_print = false, int depth = 0 );
        JsonOut( const JsonOut & ) = delete;
        JsonOut &operator=( const JsonOut & ) = delete;

        // punctuation
        void write_indent();
        void write_separator();
        void write_member_separator();
        bool get_need_separator() const {
            return need_separator;
        }
        void set_need_separator() {
            need_separator = true;
        }
        std::ostream *get_stream() {
            return stream;
        }
        int tell();
        void seek( int pos );
        void start_pretty();
        void end_pretty();

        void start_object( bool wrap = false );
        void end_object();
        void start_array( bool wrap = false );
        void end_array();

        // write data to the output stream as JSON
        void write_null();

        template <typename T, typename std::enable_if<std::is_fundamental<T>::value, int>::type = 0>
        void write( T val ) {
            if( need_separator ) {
                write_separator();
            }
            *stream << val;
            need_separator = true;
        }

        /// Overload that calls a global function `serialize(const T&,JsonOut&)`, if available.
        template<typename T>
        auto write( const T &v ) -> decltype( serialize( v, *this ), void() ) {
            serialize( v, *this );
        }

        /// Overload that calls a member function `T::serialize(JsonOut&) const`, if available.
        template<typename T>
        auto write( const T &v ) -> decltype( v.serialize( *this ), void() ) {
            v.serialize( *this );
        }

        template <typename T, typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
        void write( T val ) {
            write( static_cast<typename std::underlying_type<T>::type>( val ) );
        }

        // strings need escaping and quoting
        void write( const std::string &val );
        void write( const char *val ) {
            write( std::string( val ) );
        }

        // char should always be written as an unquoted numeral
        void write( char val ) {
            write( static_cast<int>( val ) );
        }
        void write( signed char val ) {
            write( static_cast<int>( val ) );
        }
        void write( unsigned char val ) {
            write( static_cast<int>( val ) );
        }

        template<size_t N>
        void write( const std::bitset<N> &b );

        template <typename T>
        auto write( const string_id<T> &thing ) {
            write( thing.str() );
        }

        template <typename T>
        auto write( const int_id<T> &thing ) {
            write( thing.id().str() );
        }

        // enum ~> string
        template <typename E, typename std::enable_if<std::is_enum<E>::value>::type * = nullptr>
        void write_as_string( const E value ) {
            write( io::enum_to_string<E>( value ) );
        }

        void write_as_string( const std::string &s ) {
            write( s );
        }

        template<typename T>
        void write_as_string( const string_id<T> &s ) {
            write( s );
        }

        template<typename T>
        void write_as_string( const int_id<T> &s ) {
            write( s.id() );
        }

        template<typename T, typename U>
        void write( const std::pair<T, U> &p ) {
            start_array();
            write( p.first );
            write( p.second );
            end_array();
        }

        template <typename T>
        void write_as_array( const T &container ) {
            start_array();
            for( const auto &e : container ) {
                write( e );
            }
            end_array();
        }

        // containers with front() ~> array
        // vector, deque, forward_list, list
        template < typename T, typename std::enable_if <
                       !std::is_same<void, typename T::value_type>::value >::type * = nullptr
                   >
        auto write( const T &container ) -> decltype( container.front(), ( void )0 ) {
            write_as_array( container );
        }

        // containers with matching key_type and value_type ~> array
        // set, unordered_set
        template <typename T, typename std::enable_if<
                      std::is_same<typename T::key_type, typename T::value_type>::value>::type * = nullptr
                  >
        void write( const T &container ) {
            write_as_array( container );
        }

        // special case for item colony, adds simple RLE-based compression
        template <typename T, std::enable_if_t<std::is_same<T, item>::value> * = nullptr>
        void write( const cata::colony<T> &container ) {
            start_array();
            // simple RLE implementation:
            // run_l is the length of the "running sequence" of identical items, ending with the current item.
            // when sequence ends, it's written as either the single item object (run_l==1) or as a two-element
            // array (run_l > 1), where the first element is the item and second element is the size of the sequence.
            int run_l = 1;
            for( auto it = container.begin(); it != container.end(); ++it ) {
                const auto nxt = std::next( it );
                if( nxt == container.end() || !it->same_for_rle( *nxt ) ) {
                    if( run_l == 1 ) {
                        write( *it );
                    } else {
                        start_array();
                        write( *it );
                        write( run_l );
                        end_array();
                    }
                    run_l = 1;
                } else {
                    run_l++;
                }
            }
            end_array();
        }

        // special case for colony, since it doesn't fit in other categories
        // for colony of items there is another specialization with RLE
        template < typename T, std::enable_if_t < !std::is_same<T, item>::value > * = nullptr >
        void write( const cata::colony<T> &container ) {
            write_as_array( container );
        }

        // containers with unmatching key_type and value_type ~> object
        // map, unordered_map ~> object
        template < typename T, typename std::enable_if <
                       !std::is_same<typename T::key_type, typename T::value_type>::value >::type * = nullptr
                   >
        void write( const T &map ) {
            start_object();
            for( const auto &it : map ) {
                write_as_string( it.first );
                write_member_separator();
                write( it.second );
            }
            end_object();
        }

        // convenience methods for writing named object members
        // TODO: enforce value after
        void member( const std::string &name );
        void null_member( const std::string &name );
        template <typename T> void member( const std::string &name, const T &value ) {
            member( name );
            write( value );
        }
        template <typename T> void member( const std::string &name, const T &value,
                                           const T &value_default ) {
            if( value != value_default ) {
                member( name, value );
            }
        }
        template <typename T> void member_as_string( const std::string &name, const T &value ) {
            member( name );
            write_as_string( value );
        }
};

/* TextJsonObject
 * ==========
 *
 * The TextJsonObject class provides easy access to incoming JSON object data.
 *
 * TextJsonObject maps member names to the byte offset of the paired value,
 * given an underlying TextJsonIn stream.
 *
 * It provides data by seeking the stream to the relevant position,
 * and calling the correct TextJsonIn method to read the value from the stream.
 *
 *
 * General Usage
 * -------------
 *
 *     TextJsonObject jo(jsin);
 *     std::string id = jo.get_string("id");
 *     std::string name = _(jo.get_string("name"));
 *     std::string description = _(jo.get_string("description"));
 *     int points = jo.get_int("points", 0);
 *     std::set<std::string> tags = jo.get_tags("flags");
 *     my_object_type myobject(id, name, description, points, tags);
 *
 * Here the "id", "name" and "description" members are required.
 * TextJsonObject will throw a JsonError if they are not found,
 * identifying the problem and the current position in the input stream.
 *
 * Note that "name" and "description" are passed to gettext for translating.
 * Any members with string information that is displayed directly to the user
 * will need similar treatment, and may also need to be explicitly handled
 * in lang/extract_json_strings.py to be translatable.
 *
 * The "points" member here is not required.
 * If it is not found in the incoming JSON object,
 * it will be initialized with the default value given as the second parameter,
 * in this case, 0.
 *
 * get_tags() always returns an empty set if the member is not found,
 * and so does not require a default value to be specified.
 *
 *
 * Member Testing and Automatic Deserialization
 * --------------------------------------------
 *
 * JsonObjects can test for member type with has_int(name) etc.,
 * and for member existence with has_member(name).
 *
 * They can also read directly into compatible data structures,
 * including sets, vectors, maps, and any class with a compatible
 * deserialize() routine.
 *
 * read() returns true on success, false on failure.
 *
 *     TextJsonObject jo(jsin);
 *     std::vector<std::string> messages;
 *     if (!jo.read("messages", messages)) {
 *         DebugLog() << "No messages.";
 *     }
 *
 *
 * Automatic error checking
 * ------------------------
 *
 * By default, when a TextJsonObject is destroyed (or when you call finish) it will
 * check to see whether every member of the object was referenced in some way
 * (even simply checking for the existence of the member is sufficient).
 *
 * If not all the members were referenced, then an error will be written to the
 * log (which in particular will cause the tests to fail).
 *
 * If you don't want this behavior, then call allow_omitted_members() before
 * the TextJsonObject is destroyed.  Calling str() also suppresses it (on the basis
 * that you may be intending to re-parse that string later).
 */
class TextJsonObject
{
    private:
        std::map<std::string, int, std::less<>> positions;
        int start;
        int end_;
        bool final_separator;
#ifndef CATA_IN_TOOL
        mutable std::set<std::string> visited_members;
        mutable bool report_unvisited_members = true;
        mutable bool reported_unvisited_members = false;
#endif
        void mark_visited( std::string_view name ) const;
        void report_unvisited() const;

        TextJsonIn *jsin;
        int verify_position( std::string_view name,
                             bool throw_exception = true ) const;

    public:
        explicit TextJsonObject( TextJsonIn &jsin );
        TextJsonObject() :
            start( 0 ), end_( 0 ), final_separator( false ), jsin( nullptr ) {}
        TextJsonObject( const TextJsonObject & ) = default;
        TextJsonObject( TextJsonObject && ) = default;
        TextJsonObject &operator=( const TextJsonObject & ) = default;
        TextJsonObject &operator=( TextJsonObject && ) = default;
        ~TextJsonObject() {
            finish();
        }

        class const_iterator;

        friend const_iterator;

        const_iterator begin() const;
        const_iterator end() const;

        void finish(); // moves the stream to the end of the object
        size_t size() const;
        bool empty() const;

        void allow_omitted_members() const;
        // If we're making a copy of the TextJsonObject (because it is required) to pass to a function,
        // use this to count the members visited on that one as visited on this one
        // See item::deserialize for a use case
        void copy_visited_members( const TextJsonObject &rhs ) const;
        bool has_member( const std::string &name ) const; // true iff named member exists
        std::string str() const; // copy object json as string
        [[noreturn]] void throw_error( const std::string &err ) const;
        [[noreturn]] void throw_error_at( const std::string &name, const std::string &err ) const;
        // seek to a value and return a pointer to the TextJsonIn (member must exist)
        TextJsonIn *get_raw( std::string_view name ) const;
        TextJsonValue get_member( std::string_view name ) const;
        json_source_location get_source_location() const;

        // values by name
        // variants with no fallback throw an error if the name is not found.
        // variants with a fallback return the fallback value in stead.
        bool get_bool( const std::string &name ) const;
        bool get_bool( const std::string &name, bool fallback ) const;
        int get_int( const std::string &name ) const;
        int get_int( const std::string &name, int fallback ) const;
        double get_float( const std::string &name ) const;
        double get_float( const std::string &name, double fallback ) const;
        std::string get_string( const std::string &name ) const;
        std::string get_string( const std::string &name, const std::string &fallback ) const;

        template<typename E, typename = typename std::enable_if<std::is_enum<E>::value>::type>
        E get_enum_value( const std::string &name, const E fallback ) const {
            if( !has_member( name ) ) {
                return fallback;
            }
            mark_visited( name );
            jsin->seek( verify_position( name ) );
            return jsin->get_enum_value<E>();
        }
        template<typename E, typename = typename std::enable_if<std::is_enum<E>::value>::type>
        E get_enum_value( const std::string_view name ) const {
            mark_visited( name );
            jsin->seek( verify_position( name ) );
            return jsin->get_enum_value<E>();
        }

        // containers by name
        // get_array returns empty array if the member is not found
        TextJsonArray get_array( const std::string &name ) const;
        std::vector<int> get_int_array( const std::string &name ) const;
        std::vector<std::string> get_string_array( const std::string &name ) const;
        // returns a single element array if the sype is string instead of array
        std::vector<std::string> get_as_string_array( const std::string &name ) const;
        // get_object returns empty object if not found
        TextJsonObject get_object( const std::string &name ) const;

        // get_tags returns empty set if none found
        template<typename T = std::string, typename Res = std::set<T>>
        Res get_tags( std::string_view name ) const;

        // TODO: some sort of get_map(), maybe

        // type checking
        bool has_null( const std::string &name ) const;
        bool has_bool( const std::string &name ) const;
        bool has_number( const std::string &name ) const;
        bool has_int( const std::string &name ) const {
            return has_number( name );
        }
        bool has_float( const std::string &name ) const {
            return has_number( name );
        }
        bool has_string( const std::string &name ) const;
        bool has_array( const std::string &name ) const;
        bool has_object( const std::string &name ) const;

        // non-fatally read values by reference
        // return true if the value was set.
        // return false if the member is not found.
        // throw_on_error dictates the behavior when the member was present
        // but the read fails.
        template <typename T>
        bool read( const std::string_view name, T &t, bool throw_on_error = true ) const {
            int pos = verify_position( name, false );
            if( !pos ) {
                return false;
            }
            mark_visited( name );
            jsin->seek( pos );
            return jsin->read( t, throw_on_error );
        }

        // useful debug info
        std::string line_number() const; // for occasional use only
};

/* TextJsonArray
 * =========
 *
 * The TextJsonArray class provides easy access to incoming JSON array data.
 *
 * TextJsonArray stores only the byte offset of each element in the TextJsonIn stream,
 * and iterates or accesses according to these offsets.
 *
 * It provides data by seeking the stream to the relevant position,
 * and calling the correct TextJsonIn method to read the value from the stream.
 *
 * Arrays can be iterated over,
 * or accessed directly by element index.
 *
 * Elements can be returned as standard data types,
 * or automatically read into a compatible container.
 *
 *
 * Iterative Access
 * ----------------
 *
 *     TextJsonArray ja = jo.get_array("some_array_member");
 *     std::vector<int> myarray;
 *     while (ja.has_more()) {
 *         myarray.push_back(ja.next_int());
 *     }
 *
 * If the array member is not found, get_array() returns an empty array,
 * and has_more() is always false, so myarray will remain empty.
 *
 * next_int() and the other next_* methods advance the array iterator,
 * so after the final element has been read,
 * has_more() will return false and the loop will terminate.
 *
 * If the next element is not an integer,
 * TextJsonArray will throw a JsonError indicating the problem,
 * and the position in the input stream.
 *
 * To handle arrays with elements of indeterminate type,
 * the test_* methods can be used, calling next_* according to the result.
 *
 * Note that this style of iterative access requires an element to be read,
 * or else the index will not be incremented, resulting in an infinite loop.
 * Unwanted elements can be skipped with TextJsonArray::skip_value().
 *
 *
 * Positional Access
 * -----------------
 *
 *     TextJsonArray ja = jo.get_array("xydata");
 *     point xydata(ja.get_int(0), ja.get_int(1));
 *
 * Arrays also provide has_int(index) etc., for positional type testing,
 * and read(index, &container) for automatic deserialization.
 *
 *
 * Automatic Deserialization
 * -------------------------
 *
 * Elements can also be automatically read into compatible containers,
 * such as maps, sets, vectors, and classes using a deserialize routine,
 * using the read_next() and read() methods.
 *
 *     TextJsonArray ja = jo.get_array("custom_datatype_array");
 *     while (ja.has_more()) {
 *         MyDataType mydata;
 *         ja.read_next(mydata);
 *         process(mydata);
 *     }
 */
class TextJsonArray
{
    private:
        std::vector<size_t> positions;
        int start;
        size_t index;
        int end_;
        bool final_separator;
        TextJsonIn *jsin;
        void verify_index( size_t i ) const;

    public:
        explicit TextJsonArray( TextJsonIn &jsin );
        TextJsonArray( const TextJsonArray &ja );
        TextJsonArray() : start( 0 ), index( 0 ), end_( 0 ), final_separator( false ), jsin( nullptr ) {}
        ~TextJsonArray() {
            finish();
        }
        TextJsonArray &operator=( const TextJsonArray & );

        void finish(); // move the stream position to the end of the array

        bool has_more() const; // true iff more elements may be retrieved with next_*
        size_t size() const;
        bool empty();
        std::string str(); // copy array json as string
        [[noreturn]] void throw_error( const std::string &err ) const;
        [[noreturn]] void throw_error( int idx, const std::string &err ) const;
        // See JsonIn::string_error
        [[noreturn]] void string_error( int idx, int offset, const std::string &err ) const;

        // iterative access
        TextJsonValue next_value();
        bool next_bool();
        int next_int();
        double next_float();
        std::string next_string();
        TextJsonArray next_array();
        TextJsonObject next_object();
        void skip_value(); // ignore whatever is next

        // static access
        bool get_bool( size_t index ) const;
        int get_int( size_t index ) const;
        double get_float( size_t index ) const;
        std::string get_string( size_t index ) const;
        TextJsonArray get_array( size_t index ) const;
        TextJsonObject get_object( size_t index ) const;

        // get_tags returns empty set if none found
        template<typename T = std::string, typename Res = std::set<T>>
        Res get_tags( size_t index ) const;

        class const_iterator;

        const_iterator begin() const;
        const_iterator end() const;

        // iterative type checking
        bool test_null() const;
        bool test_bool() const;
        bool test_number() const;
        bool test_int() const {
            return test_number();
        }
        bool test_float() const {
            return test_number();
        }
        bool test_string() const;
        bool test_bitset() const;
        bool test_array() const;
        bool test_object() const;

        // random-access type checking
        bool has_null( size_t index ) const;
        bool has_bool( size_t index ) const;
        bool has_number( size_t index ) const;
        bool has_int( size_t index ) const {
            return has_number( index );
        }
        bool has_float( size_t index ) const {
            return has_number( index );
        }
        bool has_string( size_t index ) const;
        bool has_array( size_t index ) const;
        bool has_object( size_t index ) const;

        // iteratively read values by reference
        template <typename T> bool read_next( T &t ) {
            verify_index( index );
            jsin->seek( positions[index++] );
            return jsin->read( t );
        }
        // random-access read values by reference
        template <typename T> bool read( size_t i, T &t, bool throw_on_error = false ) const {
            verify_index( i );
            jsin->seek( positions[i] );
            return jsin->read( t, throw_on_error );
        }
};

// NOLINTNEXTLINE(google-explicit-constructor)
inline TextJsonValue::operator std::string() const
{
    return seek().get_string();
}
// NOLINTNEXTLINE(google-explicit-constructor)
inline TextJsonValue::operator int() const
{
    return seek().get_int();
}
// NOLINTNEXTLINE(google-explicit-constructor)
inline TextJsonValue::operator bool() const
{
    return seek().get_bool();
}
// NOLINTNEXTLINE(google-explicit-constructor)
inline TextJsonValue::operator double() const
{
    return seek().get_float();
}
// NOLINTNEXTLINE(google-explicit-constructor)
inline TextJsonValue::operator TextJsonObject() const
{
    return seek().get_object();
}
// NOLINTNEXTLINE(google-explicit-constructor)
inline TextJsonValue::operator TextJsonArray() const
{
    return seek().get_array();
}
template<typename T>
inline bool TextJsonValue::read( T &t, bool throw_on_error ) const
{
    return seek().read( t, throw_on_error );
}

inline bool TextJsonValue::test_string() const
{
    return seek().test_string();
}
inline bool TextJsonValue::test_int() const
{
    return seek().test_int();
}
inline bool TextJsonValue::test_bool() const
{
    return seek().test_bool();
}
inline bool TextJsonValue::test_float() const
{
    return seek().test_float();
}
inline bool TextJsonValue::test_object() const
{
    return seek().test_object();
}
inline bool TextJsonValue::test_array() const
{
    return seek().test_array();
}
inline bool TextJsonValue::test_null() const
{
    return seek().test_null();
}

inline std::string TextJsonValue::get_string() const
{
    return seek().get_string();
}
inline int TextJsonValue::get_int() const
{
    return seek().get_int();
}
inline bool TextJsonValue::get_bool() const
{
    return seek().get_bool();
}
inline double TextJsonValue::get_float() const
{
    return seek().get_float();
}
inline TextJsonObject TextJsonValue::get_object() const
{
    return seek().get_object();
}
inline TextJsonArray TextJsonValue::get_array() const
{
    return seek().get_array();
}
inline void TextJsonValue::get_null() const
{
    seek().skip_null();
}

class TextJsonArray::const_iterator
{
    private:
        TextJsonArray array_;
        size_t index_;

    public:
        const_iterator( const TextJsonArray &array, size_t index ) : array_( array ), index_( index ) { }

        const_iterator &operator++() {
            index_++;
            return *this;
        }
        TextJsonValue operator*() const {
            array_.verify_index( index_ );
            return TextJsonValue( *array_.jsin, array_.positions[index_] );
        }

        friend bool operator==( const const_iterator &lhs, const const_iterator &rhs ) {
            return lhs.index_ == rhs.index_;
        }
        friend bool operator!=( const const_iterator &lhs, const const_iterator &rhs ) {
            return !operator==( lhs, rhs );
        }
};

inline TextJsonArray::const_iterator TextJsonArray::begin() const
{
    return const_iterator( *this, 0 );
}

inline TextJsonArray::const_iterator TextJsonArray::end() const
{
    return const_iterator( *this, size() );
}
/**
 * Represents a member of a @ref TextJsonObject. This is returned when one iterates over
 * a TextJsonObject.
 * It *is* @ref TextJsonValue, which is the value of the member, which allows one to write:
<code>
for( const TextJsonMember &member : some_json_object )
    TextJsonArray array = member.get_array();
}
</code>
 */
class TextJsonMember : public TextJsonValue
{
    private:
        const std::string &name_;

    public:
        TextJsonMember( const std::string &name, const TextJsonValue &value ) : TextJsonValue( value ),
            name_( name ) { }

        const std::string &name() const {
            return name_;
        }
        /**
         * @returns Whether this member is considered a comment.
         * Comments should generally be ignored by game, but they should be kept
         * when this class is used within a generic JSON tool.
         */
        bool is_comment() const {
            return name_ == "//";
        }
};

class TextJsonObject::const_iterator
{
    private:
        const TextJsonObject &object_;
        decltype( TextJsonObject::positions )::const_iterator iter_;

    public:
        const_iterator( const TextJsonObject &object, const decltype( iter_ ) &iter ) : object_( object ),
            iter_( iter ) { }

        const_iterator &operator++() {
            iter_++;
            return *this;
        }
        TextJsonMember operator*() const {
            object_.mark_visited( iter_->first );
            return TextJsonMember( iter_->first, TextJsonValue( *object_.jsin, iter_->second ) );
        }

        friend bool operator==( const const_iterator &lhs, const const_iterator &rhs ) {
            return lhs.iter_ == rhs.iter_;
        }
        friend bool operator!=( const const_iterator &lhs, const const_iterator &rhs ) {
            return !operator==( lhs, rhs );
        }
};

inline TextJsonObject::const_iterator TextJsonObject::begin() const
{
    return const_iterator( *this, positions.begin() );
}

inline TextJsonObject::const_iterator TextJsonObject::end() const
{
    return const_iterator( *this, positions.end() );
}

template <typename T, typename Res>
Res TextJsonArray::get_tags( const size_t index ) const
{
    Res res;

    verify_index( index );
    jsin->seek( positions[ index ] );

    // allow single string as tag
    if( jsin->test_string() ) {
        res.insert( T( jsin->get_string() ) );
        return res;
    }

    for( const std::string line : jsin->get_array() ) {
        if( !res.insert( T( line ) ).second ) {
            jsin->error( "duplicate item in set defined by json array" );
        }
    }

    return res;
}

template <typename T, typename Res>
Res TextJsonObject::get_tags( const std::string_view name ) const
{
    Res res;
    int pos = verify_position( name, false );
    if( !pos ) {
        return res;
    }
    mark_visited( name );
    jsin->seek( pos );

    // allow single string as tag
    if( jsin->test_string() ) {
        res.insert( T( jsin->get_string() ) );
        return res;
    }

    // otherwise assume it's an array and error if it isn't.
    for( const std::string line : jsin->get_array() ) {
        if( !res.insert( T( line ) ).second ) {
            jsin->error( "duplicate item in set defined by json array" );
        }
    }

    return res;
}

/**
 * Get an array member from json with name name.  For each element of that
 * array (which should be a string) add it to the given set.
 */
void add_array_to_set( std::set<std::string> &, const TextJsonObject &json,
                       const std::string &name );

std::ostream &operator<<( std::ostream &stream, const JsonError &err );

template<typename T>
void serialize( const std::optional<T> &obj, JsonOut &jsout )
{
    if( obj ) {
        jsout.write( *obj );
    } else {
        jsout.write_null();
    }
}

template<typename T>
void deserialize( std::optional<T> &obj, const TextJsonValue &jsin )
{
    if( jsin.test_null() ) {
        obj.reset();
        // Temporary to manage internal mutable state correctly.
        jsin.get_null();
    } else {
        obj.emplace();
        jsin.read( *obj, true );
    }
}

#include "flexbuffer_json.h"

#endif // CATA_SRC_JSON_H
