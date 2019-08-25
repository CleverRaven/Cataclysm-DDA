#pragma once
#ifndef JSON_H
#define JSON_H

#include <cstddef>
#include <type_traits>
#include <iostream>
#include <string>
#include <vector>
#include <bitset>
#include <array>
#include <map>
#include <set>
#include <stdexcept>

#include "colony.h"
#include "enum_conversions.h"
#include "optional.h"

/* Cataclysm-DDA homegrown JSON tools
 * copyright CC-BY-SA-3.0 2013 CleverRaven
 *
 * Consists of six JSON manipulation tools:
 * JsonIn - for low-level parsing of an input JSON stream
 * JsonOut - for outputting JSON
 * JsonObject - convenience-wrapper for reading JSON objects from a JsonIn
 * JsonArray - convenience-wrapper for reading JSON arrays from a JsonIn
 * JsonSerializer - inheritable interface for custom datatype serialization
 * JsonDeserializer - inheritable interface for custom datatype deserialization
 *
 * Further documentation can be found below.
 */

class JsonObject;
class JsonArray;
class JsonSerializer;
class JsonDeserializer;

template<typename T>
class string_id;

class JsonError : public std::runtime_error
{
    public:
        JsonError( const std::string &msg );
        const char *c_str() const noexcept {
            return what();
        }
};

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

template<typename Enum>
struct key_from_json_string<Enum, std::enable_if_t<std::is_enum<Enum>::value>> {
    Enum operator()( const std::string &s ) {
        return io::string_to_enum<Enum>( s );
    }
};

/* JsonIn
 * ======
 *
 * The JsonIn class provides a wrapper around a std::istream,
 * with methods for reading JSON data directly from the stream.
 *
 * JsonObject and JsonArray provide higher-level wrappers,
 * and are a little easier to use in most cases,
 * but have the small overhead of indexing the members or elements before use.
 *
 * Typical JsonIn usage might be something like the following:
 *
 *     JsonIn jsin(myistream);
 *     // expecting an array of objects
 *     jsin.start_array(); // throws JsonError if array not found
 *     while (!jsin.end_array()) { // end_array returns false if not the end
 *         JsonObject jo = jsin.get_object();
 *         ... // load object using JsonObject methods
 *     }
 *
 * The array could have been loaded into a JsonArray for convenience.
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
 * A JsonIn can also be used for single-pass loading,
 * by passing off members as they arrive according to their names.
 *
 * Typical usage might be:
 *
 *     JsonIn jsin(&myistream);
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
 * so preindexing as a JsonObject is safer, as well as tidier.
 */
class JsonIn
{
    private:
        std::istream *stream;
        bool ate_separator = false;

        void skip_separator();
        void skip_pair_separator();
        void end_value();

    public:
        JsonIn( std::istream &s ) : stream( &s ) {}
        JsonIn( const JsonIn & ) = delete;
        JsonIn &operator=( const JsonIn & ) = delete;

        bool get_ate_separator() {
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
        std::int64_t get_int64(); // get the next value as an int64
        bool get_bool(); // get the next value as a bool
        double get_float(); // get the next value as a double
        std::string get_member_name(); // also strips the ':'
        JsonObject get_object();
        JsonArray get_array();

        template<typename E, typename = typename std::enable_if<std::is_enum<E>::value>::type>
        E get_enum_value() {
            const auto old_offset = tell();
            try {
                return io::string_to_enum<E>( get_string() );
            } catch( const io::InvalidEnumString & ) {
                seek( old_offset ); // so the error message points to the correct place.
                error( "invalid enumeration value" );
                throw; // ^^ error already throws, but the compiler doesn't know that )-:
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

        // non-fatal reading into values by reference
        // returns true if the data was read successfully, false otherwise
        bool read( bool &b );
        bool read( char &c );
        bool read( signed char &c );
        bool read( unsigned char &c );
        bool read( short unsigned int &s );
        bool read( short int &s );
        bool read( int &i );
        bool read( std::int64_t &i );
        bool read( unsigned int &u );
        bool read( float &f );
        bool read( double &d );
        bool read( std::string &s );
        template<size_t N>
        bool read( std::bitset<N> &b );
        bool read( JsonDeserializer &j );
        // This is for the string_id type
        template <typename T>
        auto read( T &thing ) -> decltype( thing.str(), true ) {
            std::string tmp;
            if( !read( tmp ) ) {
                return false;
            }
            thing = T( tmp );
            return true;
        }

        /// Overload that calls a global function `deserialize(T&,JsonIn&)`, if available.
        template<typename T>
        auto read( T &v ) -> decltype( deserialize( v, *this ), true ) {
            try {
                deserialize( v, *this );
                return true;
            } catch( const JsonError & ) {
                return false;
            }
        }

        /// Overload that calls a member function `T::deserialize(JsonIn&)`, if available.
        template<typename T>
        auto read( T &v ) -> decltype( v.deserialize( *this ), true ) {
            try {
                v.deserialize( *this );
                return true;
            } catch( const JsonError & ) {
                return false;
            }
        }

        template<typename T, std::enable_if_t<std::is_enum<T>::value, int> = 0>
        bool read( T &val ) {
            int i;
            if( read( i ) ) {
                val = static_cast<T>( i );
                return true;
            }
            std::string s;
            if( read( s ) ) {
                val = io::string_to_enum<T>( s );
                return true;
            }
            return false;
        }

        /// Overload for std::pair
        template<typename T, typename U>
        bool read( std::pair<T, U> &p ) {
            if( !test_array() ) {
                return false;
            }
            try {
                start_array();
                return !end_array() &&
                       read( p.first ) &&
                       !end_array() &&
                       read( p.second ) &&
                       end_array();
            } catch( const JsonError & ) {
                return false;
            }
        }

        /// Overload for std::pair
        template<typename T, typename U>
        bool read( std::pair<T, U> &p ) {
            if( !test_array() ) {
                return false;
            }
            try {
                start_array();
                return !end_array() &&
                       read( p.first ) &&
                       !end_array() &&
                       read( p.second ) &&
                       end_array();
            } catch( const JsonError & ) {
                return false;
            }
        }

        // array ~> vector, deque, list
        template < typename T, typename std::enable_if <
                       !std::is_same<void, typename T::value_type>::value >::type * = nullptr
                   >
        auto read( T &v ) -> decltype( v.front(), true ) {
            if( !test_array() ) {
                return false;
            }
            try {
                start_array();
                v.clear();
                while( !end_array() ) {
                    typename T::value_type element;
                    if( read( element ) ) {
                        v.push_back( std::move( element ) );
                    } else {
                        skip_value();
                    }
                }
            } catch( const JsonError & ) {
                return false;
            }

            return true;
        }

        // array ~> array
        template <typename T, size_t N> bool read( std::array<T, N> &v ) {
            if( !test_array() ) {
                return false;
            }
            try {
                start_array();
                for( size_t i = 0; i < N; ++i ) {
                    if( end_array() ) {
                        return false; // json array is too small
                    }
                    if( !read( v[i] ) ) {
                        return false; // invalid entry
                    }
                }
                return end_array(); // false if json array is too big
            } catch( const JsonError & ) {
                return false;
            }
        }

        // object ~> containers with matching key_type and value_type
        // set, unordered_set ~> object
        template <typename T, typename std::enable_if<
                      std::is_same<typename T::key_type, typename T::value_type>::value>::type * = nullptr
                  >
        bool read( T &v ) {
            if( !test_array() ) {
                return false;
            }
            try {
                start_array();
                v.clear();
                while( !end_array() ) {
                    typename T::value_type element;
                    if( read( element ) ) {
                        v.insert( std::move( element ) );
                    } else {
                        skip_value();
                    }
                }
            } catch( const JsonError & ) {
                return false;
            }

            return true;
        }

        // special case for colony as it uses `insert()` instead of `push_back()`
        // and therefore doesn't fit with vector/deque/list
        template <typename T>
        bool read( cata::colony<T> &v ) {
            if( !test_array() ) {
                return false;
            }
            try {
                start_array();
                v.clear();
                while( !end_array() ) {
                    T element;
                    if( read( element ) ) {
                        v.insert( std::move( element ) );
                    } else {
                        skip_value();
                    }
                }
            } catch( const JsonError & ) {
                return false;
            }

            return true;
        }

        // object ~> containers with unmatching key_type and value_type
        // map, unordered_map ~> object
        template < typename T, typename std::enable_if <
                       !std::is_same<typename T::key_type, typename T::value_type>::value >::type * = nullptr
                   >
        bool read( T &m ) {
            if( !test_object() ) {
                return false;
            }
            try {
                start_object();
                m.clear();
                while( !end_object() ) {
                    using key_type = typename T::key_type;
                    key_type key = key_from_json_string<key_type>()( get_member_name() );
                    typename T::mapped_type element;
                    if( read( element ) ) {
                        m[std::move( key )] = std::move( element );
                    } else {
                        skip_value();
                    }
                }
            } catch( const JsonError & ) {
                return false;
            }

            return true;
        }

        // error messages
        std::string line_number( int offset_modifier = 0 ); // for occasional use only
        [[noreturn]] void error( const std::string &message, int offset = 0 ); // ditto
        void rewind( int max_lines = -1, int max_chars = -1 );
        std::string substr( size_t pos, size_t len = std::string::npos );
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
 * as well as anything inheriting the JsonSerializer interface,
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
        JsonOut( std::ostream &stream, bool pretty_print = false, int depth = 0 );
        JsonOut( const JsonOut & ) = delete;
        JsonOut &operator=( const JsonOut & ) = delete;

        // punctuation
        void write_indent();
        void write_separator();
        void write_member_separator();
        bool get_need_separator() {
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

        void write( const JsonSerializer &thing );
        // This is for the string_id type
        template <typename T>
        auto write( const T &thing ) -> decltype( thing.str(), ( void )0 ) {
            write( thing.str() );
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

        // special case for colony, since it doesn't fit in other categories
        template <typename T>
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
        void member( const std::string &name ); // TODO: enforce value after
        void null_member( const std::string &name );
        template <typename T> void member( const std::string &name, const T &value ) {
            member( name );
            write( value );
        }
};

/* JsonObject
 * ==========
 *
 * The JsonObject class provides easy access to incoming JSON object data.
 *
 * JsonObject maps member names to the byte offset of the paired value,
 * given an underlying JsonIn stream.
 *
 * It provides data by seeking the stream to the relevant position,
 * and calling the correct JsonIn method to read the value from the stream.
 *
 *
 * General Usage
 * -------------
 *
 *     JsonObject jo(jsin);
 *     std::string id = jo.get_string("id");
 *     std::string name = _(jo.get_string("name"));
 *     std::string description = _(jo.get_string("description"));
 *     int points = jo.get_int("points", 0);
 *     std::set<std::string> tags = jo.get_tags("flags");
 *     my_object_type myobject(id, name, description, points, tags);
 *
 * Here the "id", "name" and "description" members are required.
 * JsonObject will throw a JsonError if they are not found,
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
 * including sets, vectors, maps, and any class inheriting JsonDeserializer,
 * using read(name, value).
 *
 * read() returns true on success, false on failure.
 *
 *     JsonObject jo(jsin);
 *     std::vector<std::string> messages;
 *     if (!jo.read("messages", messages)) {
 *         DebugLog() << "No messages.";
 *     }
 */
class JsonObject
{
    private:
        std::map<std::string, int> positions;
        int start;
        int end;
        bool final_separator;
        JsonIn *jsin;
        int verify_position( const std::string &name,
                             bool throw_exception = true );

    public:
        JsonObject( JsonIn &jsin );
        JsonObject( const JsonObject &jo );
        JsonObject() : start( 0 ), end( 0 ), jsin( nullptr ) {}
        ~JsonObject() {
            finish();
        }
        JsonObject &operator=( const JsonObject & );

        void finish(); // moves the stream to the end of the object
        size_t size();
        bool empty();

        bool has_member( const std::string &name ); // true iff named member exists
        std::set<std::string> get_member_names();
        std::string str(); // copy object json as string
        [[noreturn]] void throw_error( std::string err );
        [[noreturn]] void throw_error( std::string err, const std::string &name );
        // seek to a value and return a pointer to the JsonIn (member must exist)
        JsonIn *get_raw( const std::string &name );

        // values by name
        // variants with no fallback throw an error if the name is not found.
        // variants with a fallback return the fallback value in stead.
        bool get_bool( const std::string &name );
        bool get_bool( const std::string &name, bool fallback );
        int get_int( const std::string &name );
        int get_int( const std::string &name, int fallback );
        double get_float( const std::string &name );
        double get_float( const std::string &name, double fallback );
        std::string get_string( const std::string &name );
        std::string get_string( const std::string &name, const std::string &fallback );

        template<typename E, typename = typename std::enable_if<std::is_enum<E>::value>::type>
        E get_enum_value( const std::string &name, const E fallback ) {
            if( !has_member( name ) ) {
                return fallback;
            }
            jsin->seek( verify_position( name ) );
            return jsin->get_enum_value<E>();
        }
        template<typename E, typename = typename std::enable_if<std::is_enum<E>::value>::type>
        E get_enum_value( const std::string &name ) {
            jsin->seek( verify_position( name ) );
            return jsin->get_enum_value<E>();
        }

        // containers by name
        // get_array returns empty array if the member is not found
        JsonArray get_array( const std::string &name );
        std::vector<int> get_int_array( const std::string &name );
        std::vector<std::string> get_string_array( const std::string &name );
        // get_object returns empty object if not found
        JsonObject get_object( const std::string &name );

        // get_tags returns empty set if none found
        template <typename T = std::string>
        std::set<T> get_tags( const std::string &name );

        // TODO: some sort of get_map(), maybe

        // type checking
        bool has_null( const std::string &name );
        bool has_bool( const std::string &name );
        bool has_number( const std::string &name );
        bool has_int( const std::string &name ) {
            return has_number( name );
        }
        bool has_float( const std::string &name ) {
            return has_number( name );
        }
        bool has_string( const std::string &name );
        bool has_array( const std::string &name );
        bool has_object( const std::string &name );

        // non-fatally read values by reference
        // return true if the value was set, false otherwise.
        // return false if the member is not found.
        template <typename T> bool read( const std::string &name, T &t ) {
            int pos = positions[name];
            if( pos <= start ) {
                return false;
            }
            jsin->seek( pos );
            return jsin->read( t );
        }

        // useful debug info
        std::string line_number(); // for occasional use only
};

/* JsonArray
 * =========
 *
 * The JsonArray class provides easy access to incoming JSON array data.
 *
 * JsonArray stores only the byte offset of each element in the JsonIn stream,
 * and iterates or accesses according to these offsets.
 *
 * It provides data by seeking the stream to the relevant position,
 * and calling the correct JsonIn method to read the value from the stream.
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
 *     JsonArray ja = jo.get_array("some_array_member");
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
 * JsonArray will throw a JsonError indicating the problem,
 * and the position in the input stream.
 *
 * To handle arrays with elements of indeterminate type,
 * the test_* methods can be used, calling next_* according to the result.
 *
 * Note that this style of iterative access requires an element to be read,
 * or else the index will not be incremented, resulting in an infinite loop.
 * Unwanted elements can be skipped with JsonArray::skip_value().
 *
 *
 * Positional Access
 * -----------------
 *
 *     JsonArray ja = jo.get_array("xydata");
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
 * such as maps, sets, vectors, and classes implementing JsonDeserializer,
 * using the read_next() and read() methods.
 *
 *     JsonArray ja = jo.get_array("custom_datatype_array");
 *     while (ja.has_more()) {
 *         MyDataType mydata; // MyDataType implementing JsonDeserializer
 *         ja.read_next(mydata);
 *         process(mydata);
 *     }
 */
class JsonArray
{
    private:
        std::vector<int> positions;
        int start;
        int index;
        int end;
        bool final_separator;
        JsonIn *jsin;
        void verify_index( int i );

    public:
        JsonArray( JsonIn &jsin );
        JsonArray( const JsonArray &ja );
        JsonArray() : start( 0 ), index( 0 ), end( 0 ), jsin( nullptr ) {}
        ~JsonArray() {
            finish();
        }
        JsonArray &operator=( const JsonArray & );

        void finish(); // move the stream position to the end of the array

        bool has_more(); // true iff more elements may be retrieved with next_*
        size_t size() const;
        bool empty();
        std::string str(); // copy array json as string
        void throw_error( std::string err );
        void throw_error( std::string err, int idx );

        // iterative access
        bool next_bool();
        int next_int();
        double next_float();
        std::string next_string();
        JsonArray next_array();
        JsonObject next_object();
        void skip_value(); // ignore whatever is next

        // static access
        bool get_bool( int index );
        int get_int( int index );
        double get_float( int index );
        std::string get_string( int index );
        JsonArray get_array( int index );
        JsonObject get_object( int index );

        // get_tags returns empty set if none found
        template <typename T = std::string>
        std::set<T> get_tags( int index );

        // iterative type checking
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

        // random-access type checking
        bool has_null( int index );
        bool has_bool( int index );
        bool has_number( int index );
        bool has_int( int index ) {
            return has_number( index );
        }
        bool has_float( int index ) {
            return has_number( index );
        }
        bool has_string( int index );
        bool has_array( int index );
        bool has_object( int index );

        // iteratively read values by reference
        template <typename T> bool read_next( T &t ) {
            verify_index( index );
            jsin->seek( positions[index++] );
            return jsin->read( t );
        }
        // random-access read values by reference
        template <typename T> bool read( int i, T &t ) {
            verify_index( i );
            jsin->seek( positions[i] );
            return jsin->read( t );
        }
};

template <typename T>
std::set<T> JsonArray::get_tags( int index )
{
    std::set<T> res;

    verify_index( index );
    jsin->seek( positions[ index ] );

    // allow single string as tag
    if( jsin->test_string() ) {
        res.insert( T( jsin->get_string() ) );
        return res;
    }

    JsonArray jsarr = jsin->get_array();
    while( jsarr.has_more() ) {
        res.insert( T( jsarr.next_string() ) );
    }

    return res;
}

template <typename T>
std::set<T> JsonObject::get_tags( const std::string &name )
{
    std::set<T> res;
    int pos = positions[ name ];
    if( pos <= start ) {
        return res;
    }
    jsin->seek( pos );

    // allow single string as tag
    if( jsin->test_string() ) {
        res.insert( T( jsin->get_string() ) );
        return res;
    }

    // otherwise assume it's an array and error if it isn't.
    JsonArray jsarr = jsin->get_array();
    while( jsarr.has_more() ) {
        res.insert( T( jsarr.next_string() ) );
    }

    return res;
}

/**
 * Get an array member from json with name name.  For each element of that
 * array (which should be a string) add it to the given set.
 */
void add_array_to_set( std::set<std::string> &, JsonObject &json, const std::string &name );

/* JsonSerializer
 * ==============
 *
 * JsonSerializer is an inheritable interface,
 * allowing classes to define how they are to be serialized,
 * and then treated as a basic type for serialization purposes.
 *
 * All a class must to do satisfy this interface,
 * is define a `void serialize(JsonOut&) const` method,
 * which should use the provided JsonOut to write its data as JSON.
 *
 *     class point : public JsonSerializer {
 *         int x, y;
 *         void serialize(JsonOut &jsout) const {
 *             jsout.start_array();
 *             jsout.write(x);
 *             jsout.write(y);
 *             jsout.end_array();
 *         }
 *     }
 */
class JsonSerializer
{
    public:
        virtual ~JsonSerializer() = default;
        virtual void serialize( JsonOut &jsout ) const = 0;
        JsonSerializer() = default;
        JsonSerializer( JsonSerializer && ) = default;
        JsonSerializer( const JsonSerializer & ) = default;
        JsonSerializer &operator=( JsonSerializer && ) = default;
        JsonSerializer &operator=( const JsonSerializer & ) = default;
};

/* JsonDeserializer
 * ==============
 *
 * JsonDeserializer is an inheritable interface,
 * allowing classes to define how they are to be deserialized,
 * and then treated as a basic type for deserialization purposes.
 *
 * All a class must to do satisfy this interface,
 * is define a `void deserialize(JsonIn&)` method,
 * which should read its data from the provided JsonIn,
 * assuming it to be in the correct form.
 *
 *     class point : public JsonDeserializer {
 *         int x, y;
 *         void deserialize(JsonIn &jsin) {
 *             JsonArray ja = jsin.get_array();
 *             x = ja.get_int(0);
 *             y = ja.get_int(1);
 *         }
 *     }
 */
class JsonDeserializer
{
    public:
        virtual ~JsonDeserializer() = default;
        virtual void deserialize( JsonIn &jsin ) = 0;
        JsonDeserializer() = default;
        JsonDeserializer( JsonDeserializer && ) = default;
        JsonDeserializer( const JsonDeserializer & ) = default;
        JsonDeserializer &operator=( JsonDeserializer && ) = default;
        JsonDeserializer &operator=( const JsonDeserializer & ) = default;
};

std::ostream &operator<<( std::ostream &stream, const JsonError &err );

template<typename T>
void serialize( const cata::optional<T> &obj, JsonOut &jsout )
{
    if( obj ) {
        jsout.write( *obj );
    } else {
        jsout.write_null();
    }
}

template<typename T>
void deserialize( cata::optional<T> &obj, JsonIn &jsin )
{
    if( jsin.test_null() ) {
        obj.reset();
    } else {
        obj.emplace();
        jsin.read( *obj );
    }
}

#endif
