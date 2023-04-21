#pragma once
#ifndef CATA_SRC_FLEXBUFFER_JSON_H
#define CATA_SRC_FLEXBUFFER_JSON_H

#include <optional>
#include <string>
#include <type_traits>

#include <flatbuffers/flexbuffers.h>

#include "cata_bitset.h"
#include "cata_small_literal_vector.h"
#include "cata_utility.h"
#include "flexbuffer_cache.h"
#include "json.h"
#include "json_error.h"
#include "int_id.h"
#include "memory_fast.h"
#include "string_id.h"

// Represents a 'path' in a json object, a series of object keys or indices, that when accessed from the root get you to some element in the json structure.
struct JsonPath {
        // Flexbuffer structures are either arrays or objects, where objects are just
        // arrays with an extra 'key vector'. As such, we can store a jsonpath as a series
        // of numeric indices and not store any string pointer values at all. This lets us
        // compress the datastructure to a series of shorts (at least one vector has >1k
        // elements in it). How this index should be interpreted depends on the type of
        // the json value at that location. Arrays as native indices, objects as an index
        // into the object's corresponding key vector.

        using index_type = uint32_t;

        JsonPath() = default;
        JsonPath( const JsonPath & ) = default;
        JsonPath( JsonPath && ) = default;

        JsonPath &operator=( const JsonPath & ) = default;
        JsonPath &operator=( JsonPath && ) noexcept = default;

        JsonPath &push_back( size_t idx ) {
            if( idx < std::numeric_limits<index_type>::max() ) {
                path_.push_back( static_cast<index_type>( idx ) );
            } else {
                throw std::runtime_error( "Json index out of range of uint32_t" );
            }
            return *this;
        }

        uint8_t size() const {
            return path_.size();
        }

        void pop_back() {
            path_.pop_back();
        }

        index_type &last() {
            return *path_.back();
        }

        index_type const *begin() const {
            return path_.begin();
        }
        index_type const *end() const {
            return path_.end();
        }

        friend JsonPath operator+( const JsonPath &lhs, size_t idx ) {
            JsonPath ret( lhs );
            ret.push_back( idx );
            return ret;
        }

    private:
        static constexpr size_t kInlinePathSegments = 7;
        small_literal_vector<index_type, kInlinePathSegments> path_;
};

class JsonObject;
class JsonArray;
class JsonValue;
class JsonMember;

inline flexbuffers::Reference flexbuffer_root_from_storage(
    const std::shared_ptr<flexbuffer_storage> &storage )
{
    return flexbuffers::GetRoot( storage->data(), storage->size() );
}

class Json
{
    public:
        Json() = delete;
        using flexbuffer = flexbuffers::Reference;

        std::string str() const;
        [[noreturn]] void throw_error( const JsonPath &path, int offset, const std::string &message ) const;
        [[noreturn]] void throw_error_after( const JsonPath &path, const std::string &message ) const;
        [[noreturn]] void string_error( const JsonPath &path, int offset,
                                        const std::string &message ) const;

        static const std::string &flexbuffer_type_to_string( flexbuffers::Type t );

    protected:
        Json( std::shared_ptr<parsed_flexbuffer> root, flexbuffer json ) : root_{ std::move( root ) },
            json_ { json } {}

        ~Json() noexcept = default;

        Json( const Json & ) = default;
        Json &operator=( const Json & ) = default;

        Json( Json && ) noexcept = default;
        Json &operator=( Json && ) noexcept = default;

        // Keeps the backing memory alive, also contains the root reference and source file for error logging.
        std::shared_ptr<parsed_flexbuffer> root_;
        // The actual thing we are pointing to with this Json instance.
        flexbuffer json_;
};

class JsonWithPath : protected Json
{
    public:
        [[noreturn]] void throw_error( const std::string &message ) const noexcept( false ) {
            throw_error( 0, message );
        }
        [[noreturn]] void throw_error( int offset, const std::string &message ) const noexcept( false ) {
            Json::throw_error( path_, offset, message );
        }
        [[noreturn]] void throw_error_after( const std::string &message ) const noexcept( false ) {
            Json::throw_error_after( path_, message );
        }

    protected:
        JsonWithPath( std::shared_ptr<parsed_flexbuffer> root, flexbuffer json,
                      JsonPath &&path ) : Json( std::move( root ), json ), path_{ std::move( path ) } {}

        JsonWithPath( const JsonWithPath & ) = default;
        JsonWithPath &operator=( const JsonWithPath & ) = default;

        JsonWithPath( JsonWithPath && ) noexcept = default;
        JsonWithPath &operator=( JsonWithPath && ) noexcept = default;

        bool error_or_false( bool throw_on_error,
                             const std::string &message ) const noexcept( false ) {
            if( throw_on_error ) {
                throw_error( 0, message );
            }
            return false;
        }

        JsonPath path_;
};

// A single value wrapper. Convertible to any actual value or array/object.
// To keep creating these as trivial as possible, these do not contain a JsonPath. The typical
// pattern with JsonValue's is to immediately cast to the desired type, and generally are
// retrieved while iterating members of a JsonObject or values in a JsonArray. This
// means the parent_path will outlive the JsonValue. But we have no real way to guarantee
// this without resorting to moving the JsonPath into a heap allocation with shared_ptr.
// We still aren't totally trivial because Json has a shared_ptr member, but it's close.
class JsonValue : Json
{
    public:
        JsonValue( std::shared_ptr<parsed_flexbuffer> root, flexbuffer json,
                   JsonPath const *parent_path, size_t path_index ) : Json(
                           std::move( root ),
                           json ), parent_path_{ parent_path }, path_index_{ path_index } {}

        JsonValue( const JsonValue & ) = default;
        JsonValue &operator=( const JsonValue & ) = default;

        JsonValue( JsonValue && ) noexcept = default;
        JsonValue &operator=( JsonValue && ) noexcept = default;

        // NOLINTNEXTLINE(google-explicit-constructor)
        operator std::string() const;
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator bool() const;
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator int() const;
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator unsigned() const;
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator int64_t() const;
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator uint64_t() const;
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator double() const;
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator JsonObject() const;
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator JsonArray() const;

        bool test_string() const;
        bool test_bool() const;
        bool test_number() const;
        bool test_int() const;
        bool test_float() const;
        bool test_object() const;
        bool test_array() const;
        bool test_null() const;

        std::string get_string() const;
        bool get_bool() const;
        int get_int() const;
        unsigned int get_uint() const;
        int64_t get_int64() const;
        uint64_t get_uint64() const;
        double get_float() const;
        JsonArray get_array() const;
        JsonObject get_object() const;

        using Json::throw_error;
        using Json::throw_error_after;
        using Json::string_error;

        // optionally-fatal reading into values by reference
        // returns true if the data was read successfully, false otherwise
        // if throw_on_error then throws JsonError rather than returning false.
        bool read( bool &b, bool throw_on_error = false ) const;
        bool read( char &c, bool throw_on_error = false ) const;
        bool read( signed char &c, bool throw_on_error = false ) const;
        bool read( unsigned char &c, bool throw_on_error = false ) const;
        bool read( short unsigned int &s, bool throw_on_error = false ) const;
        bool read( short int &s, bool throw_on_error = false ) const;
        bool read( int &i, bool throw_on_error = false ) const;
        bool read( int64_t &i, bool throw_on_error = false ) const;
        bool read( uint64_t &i, bool throw_on_error = false ) const;
        bool read( unsigned int &u, bool throw_on_error = false ) const;
        bool read( float &f, bool throw_on_error = false ) const;
        bool read( double &d, bool throw_on_error = false ) const;
        bool read( std::string &s, bool throw_on_error = false ) const;

        // This is for the string_id type
        template <typename T>
        auto read( string_id<T> &thing, bool throw_on_error = false ) const -> bool;

        // This is for the int_id type
        template <typename T>
        auto read( int_id<T> &thing, bool throw_on_error = false ) const -> bool;

        /// Overload that calls a global function `deserialize(T&,JsonIn&)`, if available.
        template<typename T>
        auto read( T &v, bool throw_on_error = false ) const ->
        decltype( deserialize( v, *this ), true );

        /// Overload that calls a member function `T::deserialize(JsonIn&)`, if available.
        template<typename T>
        auto read( T &v, bool throw_on_error = false ) const -> decltype( v.deserialize( *this ), true );

        template<typename T, std::enable_if_t<std::is_enum<T>::value, int> = 0>
        bool read( T &val, bool throw_on_error = false ) const;

        /// Overload for std::pair
        template<typename T, typename U>
        bool read( std::pair<T, U> &p, bool throw_on_error = false ) const;

        // array ~> vector, deque, list
        template < typename T, typename std::enable_if <
                       !std::is_same<void, typename T::value_type>::value >::type * = nullptr
                   >
        auto read( T &v, bool throw_on_error = false ) const -> decltype( v.front(), true );

        // array ~> array
        template <typename T, size_t N>
        bool read( std::array<T, N> &v, bool throw_on_error = false ) const;

        // object ~> containers with matching key_type and value_type
        // set, unordered_set ~> object
        template <typename T, typename std::enable_if<
                      std::is_same<typename T::key_type, typename T::value_type>::value>::type * = nullptr
                  >
        bool read( T &v, bool throw_on_error = false ) const;

        // special case for enum_bitset
        template <typename T>
        bool read( enum_bitset<T> &v, bool throw_on_error = false ) const;

        // special case for colony<item> as it supports RLE
        // see corresponding `write` for details
        template <typename T, std::enable_if_t<std::is_same<T, item>::value>* = nullptr >
        bool read( cata::colony<T> &v, bool throw_on_error = false ) const;

        // special case for colony as it uses `insert()` instead of `push_back()`
        // and therefore doesn't fit with vector/deque/list
        // for colony of items there is another specialization with RLE
        template < typename T, std::enable_if_t < !std::is_same<T, item>::value > * = nullptr >
        bool read( cata::colony<T> &v, bool throw_on_error = false ) const;

        // object ~> containers with unmatching key_type and value_type
        // map, unordered_map ~> object
        template < typename T, typename std::enable_if <
                       !std::is_same<typename T::key_type, typename T::value_type>::value >::type * = nullptr
                   >
        bool read( T &m, bool throw_on_error = true ) const;

        [[noreturn]] void string_error( const std::string &message ) const noexcept( false ) {
            string_error( 0, message );
        }

        [[noreturn]] void string_error( int offset, const std::string &message ) const noexcept( false ) {
            JsonPath p;
            if( parent_path_ ) {
                p = *parent_path_ + path_index_;
            }
            string_error( p, offset, message );
        }

        [[noreturn]] void throw_error( const std::string &message ) const noexcept( false ) {
            throw_error( 0, message );
        }

        [[noreturn]] void throw_error( int offset, const std::string &message ) const noexcept( false ) {
            JsonPath p;
            if( parent_path_ ) {
                p = *parent_path_ + path_index_;
            }
            throw_error( p, offset, message );
        }

        [[noreturn]] void throw_error_after( const std::string &message ) const noexcept( false ) {
            JsonPath p;
            if( parent_path_ ) {
                p = *parent_path_ + path_index_;
            }
            throw_error_after( p, message );
        }
    private:
        JsonPath const *parent_path_;
        size_t path_index_;

        // Putting this in the header allows inlining and eliding a bunch of code with constant folding.
        bool error_or_false( bool throw_on_error, const std::string &message ) const noexcept( false ) {
            if( throw_on_error ) {
                throw_error( 0, message );
            }
            return false;
        }
};

class JsonArray : JsonWithPath
{
        static const auto &empty_array_() {
            // NOLINTNEXTLINE(cata-almost-never-auto)
            static auto empty_array = flexbuffer_cache::parse_buffer( "[]" );
            return empty_array;
        }

    public:
        JsonArray() : JsonWithPath( empty_array_(),
                                        flexbuffer_root_from_storage( empty_array_()->get_storage() ), {} ) {
            init( json_ );
        }

        JsonArray(
            std::shared_ptr<parsed_flexbuffer> root,
            flexbuffer json,
            JsonPath &&path )
            : JsonWithPath( std::move( root ), json, std::move( path ) ) {
            init( json_ );
        }

        JsonArray( const JsonArray &rhs ) : JsonWithPath( rhs ) {
            init( json_ );
        }

        JsonArray &operator=( const JsonArray &rhs ) {
            JsonWithPath::operator=( rhs );
            init( json_ );
            return *this;
        }

        JsonArray( JsonArray &&rhs ) noexcept :
            JsonWithPath( std::move( static_cast<JsonWithPath &>( rhs ) ) ) {
            init( json_, &rhs.visited_fields_bitset_ );
            rhs.visited_fields_bitset_.set_all();
        }

        JsonArray &operator=( JsonArray &&rhs ) noexcept {
            JsonWithPath::operator=( std::move( static_cast<JsonWithPath &>( rhs ) ) );
            init( json_, &rhs.visited_fields_bitset_ );
            rhs.visited_fields_bitset_.set_all();
            return *this;
        }

        class const_iterator;
        friend const_iterator;

        // Iterates the values in this array.
        const_iterator begin() const;
        const_iterator end() const;

        size_t size() const {
            return size_;
        }

        bool empty() const {
            return size() == 0;
        }

        JsonValue operator[]( size_t idx ) const;

        std::string get_string( size_t idx ) const;
        int get_int( size_t idx ) const;
        double get_float( size_t idx ) const;

        JsonArray get_array( size_t idx ) const;
        JsonObject get_object( size_t idx ) const;

        bool has_string( size_t idx ) const;
        bool has_bool( size_t idx ) const;
        bool has_int( size_t idx ) const;
        bool has_float( size_t idx ) const;
        bool has_array( size_t idx ) const;
        bool has_object( size_t idx ) const;

        bool test_string() const;
        std::string next_string();

        bool test_bool() const;
        bool next_bool();

        bool test_int() const;
        int next_int();

        bool test_float() const;
        double next_float();

        bool test_array() const;

        JsonArray next_array();

        bool test_object() const;
        JsonObject next_object();

        JsonValue next_value();

        template<typename E, typename = typename std::enable_if<std::is_enum<E>::value>::type>
        E next_enum_value() ;

        // array ~> vector, deque, list
        template < typename T, typename std::enable_if <
                       !std::is_same<void, typename T::value_type>::value >::type * = nullptr
                   >
        auto read( T &v, bool throw_on_error = false ) const -> decltype( v.front(), true );

        // random-access read values by reference
        template <typename T> bool read_next( T &t, bool throw_on_error = false );

        // random-access read values by reference
        template <typename T> bool read( size_t idx, T &t, bool throw_on_error = false ) const;

        template <typename T = std::string, typename Res = std::set<T>>
        Res get_tags( size_t idx ) const;

        [[noreturn]] void string_error( size_t idx, int offset, const std::string &message ) const;

        bool has_more() const {
            return next_ < size_;
        }

        using JsonWithPath::throw_error;
        using JsonWithPath::throw_error_after;
        using JsonWithPath::error_or_false;

    private:
        JsonValue get_next();

        size_t size_ = 0;
        size_t next_ = 0;

        // NOLINTNEXTLINE(cata-large-inline-function)
        void init( const flexbuffer &json, tiny_bitset *moved_visited_fields = nullptr ) noexcept {
            if( json.IsFixedTypedVector() ) {
                size_ = json.AsFixedTypedVector().size();
            } else if( json.IsTypedVector() ) {
                size_ = json.AsTypedVector().size();
            } else {
                size_ = json.AsVector().size();
            }
            if( moved_visited_fields ) {
                using namespace std;
                swap( visited_fields_bitset_, *moved_visited_fields );
            } else {
                visited_fields_bitset_.resize( size_ );
            }
        }

        mutable tiny_bitset visited_fields_bitset_;

        void mark_visited( size_t idx ) const {
            visited_fields_bitset_.set( idx );
        }
};

class JsonMember : public JsonValue
{
    public:
        JsonMember( flexbuffers::String name,
                    JsonValue value ) : JsonValue( std::move( value ) ),
            name_( name ) { }

        JsonMember( const JsonMember & ) = default;
        JsonMember &operator=( const JsonMember & ) = default;

        JsonMember( JsonMember && ) = default;
        JsonMember &operator=( JsonMember && ) = default;

        std::string name() const {
            return name_.str();
        }

        bool is_comment() const {
            return strncmp( name_.c_str(), "//", 2 ) == 0;
        }

    private:
        flexbuffers::String name_;
};

class JsonObject : JsonWithPath
{
    protected:
        flexbuffers::TypedVector keys_ = flexbuffers::TypedVector::EmptyTypedVector();
        flexbuffers::Vector values_ = flexbuffers::Vector::EmptyVector();
        mutable tiny_bitset visited_fields_bitset_;

        static const auto &empty_object_() {
            // NOLINTNEXTLINE(cata-almost-never-auto)
            static auto empty_object = flexbuffer_cache::parse_buffer( "{}" );
            return empty_object;
        }

    public:
        JsonObject() : JsonWithPath( empty_object_(),
                                         flexbuffer_root_from_storage( empty_object_()->get_storage() ), {} ) {
            init( json_ );
        }

        JsonObject(
            std::shared_ptr<parsed_flexbuffer> root,
            flexbuffer json,
            JsonPath &&path
        )
            : JsonWithPath( std::move( root ), json, std::move( path ) ) {
            init( json_ );
        }

        JsonObject( const JsonObject &rhs ) : JsonWithPath( rhs ) {
            init( json_ );
        }

        JsonObject &operator=( const JsonObject &rhs ) {
            JsonWithPath::operator=( rhs );
            init( json_ );
            // Copying an object resets visited fields.
            visited_fields_bitset_.clear_all();
            return *this;
        }

        JsonObject( JsonObject &&rhs ) noexcept :
            JsonWithPath( std::move( static_cast<JsonWithPath &>( rhs ) ) )  {
            init( json_, &rhs.visited_fields_bitset_ );
            rhs.visited_fields_bitset_.set_all();
        }

        JsonObject &operator=( JsonObject &&rhs ) noexcept {
            JsonWithPath::operator=( std::move( static_cast<JsonWithPath &>( rhs ) ) );
            init( json_, &rhs.visited_fields_bitset_ );
            rhs.visited_fields_bitset_.set_all();
            return *this;
        }

        ~JsonObject();

    private:
        void init( const flexbuffer &json, tiny_bitset *moved_visited_fields = nullptr ) {
            flexbuffers::Map json_map = json.AsMap();
            keys_ = json_map.Keys();
            values_ = json_map.Values();
            if( moved_visited_fields ) {
                using namespace std;
                swap( visited_fields_bitset_, *moved_visited_fields );
            } else {
                visited_fields_bitset_.resize( keys_.size() );
            }
        }

    public:
        size_t size() const {
            return keys_.size();
        }

        std::string get_string( const std::string &key ) const;
        std::string get_string( const char *key ) const;

        template<typename T, typename std::enable_if_t<std::is_convertible<T, std::string>::value>* = nullptr>
        std::string get_string( const std::string &key, T && fallback ) const;

        template<typename T, typename std::enable_if_t<std::is_convertible<T, std::string>::value>* = nullptr>
        std::string get_string( const char *key, T && fallback ) const;

        // Vanilla accessors. Just return the named member and use it's conversion function.
        bool get_bool( std::string_view key ) const;
        int get_int( std::string_view key ) const;
        double get_float( std::string_view key ) const;
        JsonArray get_array( std::string_view key ) const;
        JsonObject get_object( std::string_view key ) const;

        template<typename E, typename = typename std::enable_if<std::is_enum<E>::value>::type>
        E get_enum_value( const std::string &name ) const;
        template<typename E, typename = typename std::enable_if<std::is_enum<E>::value>::type>
        E get_enum_value( const char *name ) const;

        template<typename E, typename = typename std::enable_if<std::is_enum<E>::value>::type>
        E get_enum_value( const std::string &name, E fallback ) const;
        template<typename E, typename = typename std::enable_if<std::is_enum<E>::value>::type>
        E get_enum_value( const char *name, E fallback ) const;

        // Sigh.
        std::vector<int> get_int_array( std::string_view name ) const;
        std::vector<std::string> get_string_array( std::string_view name ) const;
        std::vector<std::string> get_as_string_array( const std::string &name ) const;

        bool has_member( std::string_view key ) const;
        bool has_null( std::string_view key ) const;
        bool has_string( std::string_view key ) const;
        bool has_bool( std::string_view key ) const;
        bool has_number( std::string_view key ) const;
        bool has_int( std::string_view key ) const;
        bool has_float( std::string_view key ) const;
        bool has_array( std::string_view key ) const;
        bool has_object( std::string_view key ) const;

        // Fallback accessors. Test if the named member exists, and if yes, return it,
        // else will return the fallback value. Does *not* test the member is the type
        // being requested.
        bool get_bool( std::string_view key, bool fallback ) const;
        int get_int( std::string_view key, int fallback ) const;
        double get_float( std::string_view key, double fallback ) const;

        // Tries to get the member, and if found, calls it visited.
        std::optional<JsonValue> get_member_opt( std::string_view key ) const;
        JsonValue get_member( std::string_view key ) const;
        JsonValue operator[]( std::string_view key ) const;

        // Schwillions of read overloads
        template <typename T>
        bool read( std::string_view name, T &t, bool throw_on_error = true ) const;

        template <typename T = std::string, typename Res = std::set<T>>
        Res get_tags( std::string_view name ) const;

        [[noreturn]] void throw_error( const std::string &err ) const;
        [[noreturn]] void throw_error_at( std::string_view member, const std::string &err ) const;

        void allow_omitted_members() const {
            visited_fields_bitset_.set_all();
        }

        void copy_visited_members( const JsonObject &rhs ) const {
            visited_fields_bitset_ = rhs.visited_fields_bitset_;
        }

        using Json::str;

        class const_iterator;
        friend const_iterator;

        const_iterator begin() const;
        const_iterator end() const;

    private:
        // Only called by the iterator which can't be manually constructed.
        JsonValue operator[]( size_t idx ) const;

        // NOLINTNEXTLINE(cata-large-inline-function)
        flexbuffers::Reference find_value_ref( const std::string_view key ) const {
            size_t idx = 0;
            bool found = find_map_key_idx( key, keys_, idx );
            if( found ) {
                return values_[ idx ];
            }
            return flexbuffers::Reference();
        }

        // NOLINTNEXTLINE(cata-large-inline-function)
        static bool find_map_key_idx( const std::string_view key, const flexbuffers::TypedVector &keys,
                                      size_t &idx ) {
            // Handlrolled binary search because the STL does not provide a version that just uses indexes.
            typename std::make_signed<size_t>::type low = 0;
            typename std::make_signed<size_t>::type high = keys.size() - 1;
            while( low <= high ) {
                std::make_signed_t<size_t> mid = ( high - low ) / 2 + low;

                const std::string_view test_key = keys[ mid ].AsKey();
                int res = string_view_cmp( test_key, key );

                if( res == 0 ) {
                    idx = mid;
                    return true;
                } else if( res < 0 ) {
                    low = mid + 1;
                } else {
                    high = mid - 1;
                }
            }
            return false;
        }

        void mark_visited( size_t idx ) const {
            visited_fields_bitset_.set( idx );
        }

        void report_unvisited() const;

        // Reports an error via JsonObject at this location.
        [[noreturn]] void error_no_member( std::string_view member ) const;

        // debugmsg prints all the skipped members.
        void error_skipped_members( const std::vector<size_t> &skipped_members ) const;
};

// Having this uncommented causes unable to match definition errors for msvc.
// The implementation still exists in the -inl header but has to come after all
// the definitions for JsonValue::read().
//template<typename T>
//void deserialize( std::optional<T> &obj, const JsonValue &jsin );

void add_array_to_set( std::set<std::string> &s, const JsonObject &json,
                       std::string_view name );

#include "flexbuffer_json-inl.h"

#endif // CATA_SRC_FLEXBUFFER_JSON_H
