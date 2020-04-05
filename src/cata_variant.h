#pragma once
#ifndef CATA_VARIANT_H
#define CATA_VARIANT_H

#include <array>
#include <cassert>
#include <utility>

#include "character_id.h"
#include "debug.h"
#include "enum_conversions.h"
#include "enum_traits.h"
#include "hash_utils.h"
#include "type_id.h"

enum add_type : int;
enum body_part : int;
enum class mutagen_technique : int;
enum hp_part : int;

using itype_id = std::string;

// cata_variant is a variant-like type that stores a variety of different cata
// types.  All types are stored by converting them to a string.

enum class cata_variant_type : int {
    void_, // Special type for empty variants
    add_type,
    bionic_id,
    body_part,
    bool_,
    character_id,
    efftype_id,
    hp_part,
    int_,
    itype_id,
    matype_id,
    mtype_id,
    mutagen_technique,
    mutation_category_id,
    oter_id,
    skill_id,
    species_id,
    spell_id,
    string,
    trait_id,
    trap_str_id,
    num_types, // last
};

template<>
struct enum_traits<cata_variant_type> {
    static constexpr cata_variant_type last = cata_variant_type::num_types;
};

// Here follows various implementation details.  Skip to the bottom of the file
// to see cata_variant itself.

namespace cata_variant_detail
{

// The convert struct is specialized for each cata_variant_type to provide the
// code for converting that type to or from a string.
template<cata_variant_type Type>
struct convert;

// type_for<T>() returns the cata_variant_type enum value corresponding to the
// given value type.  e.g. type_for<mtype_id>() == cata_variant_type::mtype_id.

template<typename T, size_t... I>
constexpr cata_variant_type type_for_impl( std::index_sequence<I...> )
{
    constexpr size_t num_types = static_cast<size_t>( cata_variant_type::num_types );
    constexpr std::array<bool, num_types> matches = {{
            std::is_same<T, typename convert<static_cast<cata_variant_type>( I )>::type>::value...
        }
    };
    for( size_t i = 0; i < num_types; ++i ) {
        if( matches[i] ) {
            return static_cast<cata_variant_type>( i );
        }
    }
    // No match
    return cata_variant_type::num_types;
}

template<typename T>
constexpr cata_variant_type type_for()
{
    constexpr size_t num_types = static_cast<size_t>( cata_variant_type::num_types );
    using SimpleT = std::remove_cv_t<std::remove_reference_t<T>>;
    return type_for_impl<SimpleT>( std::make_index_sequence<num_types> {} );
}

// Inherit from this struct to easily implement convert specializations for any
// string type
template<typename T>
struct convert_string {
    using type = T;
    static_assert( std::is_same<T, std::string>::value,
                   "Intended for use only with string typedefs" );
    static std::string to_string( const T &v ) {
        return v;
    }
    static T from_string( const std::string &v ) {
        return v;
    }
};

// Inherit from this struct to easily implement convert specializations for any
// string_id type
template<typename T>
struct convert_string_id {
    using type = T;
    static std::string to_string( const T &v ) {
        return v.str();
    }
    static T from_string( const std::string &v ) {
        return T( v );
    }
};

// Inherit from this struct to easily implement convert specializations for any
// int_id type
template<typename T>
struct convert_int_id {
    using type = T;
    static std::string to_string( const T &v ) {
        return v.id().str();
    }
    static T from_string( const std::string &v ) {
        return T( v );
    }
};

// Inherit from this struct to easily implement convert specializations for any
// enum type
template<typename T>
struct convert_enum {
    using type = T;
    static std::string to_string( const T &v ) {
        return io::enum_to_string( v );
    }
    static T from_string( const std::string &v ) {
        return io::string_to_enum<T>( v );
    }
};

// These are the specializations of convert for each value type.
static_assert( static_cast<int>( cata_variant_type::num_types ) == 21,
               "This assert is a reminder to add conversion support for any new types to the "
               "below specializations" );

template<>
struct convert<cata_variant_type::void_> {
    using type = void;
};

template<>
struct convert<cata_variant_type::add_type> : convert_enum<add_type> {};

template<>
struct convert<cata_variant_type::bionic_id> : convert_string_id<bionic_id> {};

template<>
struct convert<cata_variant_type::body_part> : convert_enum<body_part> {};

template<>
struct convert<cata_variant_type::bool_> {
    using type = bool;
    static std::string to_string( const bool v ) {
        return std::to_string( static_cast<int>( v ) );
    }
    static bool from_string( const std::string &v ) {
        return std::stoi( v );
    }
};

template<>
struct convert<cata_variant_type::character_id> {
    using type = character_id;
    static std::string to_string( const character_id &v ) {
        return std::to_string( v.get_value() );
    }
    static character_id from_string( const std::string &v ) {
        return character_id( std::stoi( v ) );
    }
};

template<>
struct convert<cata_variant_type::efftype_id> : convert_string_id<efftype_id> {};

template<>
struct convert<cata_variant_type::hp_part> : convert_enum<hp_part> {};

template<>
struct convert<cata_variant_type::int_> {
    using type = int;
    static std::string to_string( const int v ) {
        return std::to_string( v );
    }
    static int from_string( const std::string &v ) {
        return std::stoi( v );
    }
};

template<>
struct convert<cata_variant_type::itype_id> : convert_string<itype_id> {};

template<>
struct convert<cata_variant_type::matype_id> : convert_string_id<matype_id> {};

template<>
struct convert<cata_variant_type::mtype_id> : convert_string_id<mtype_id> {};

template<>
struct convert<cata_variant_type::mutagen_technique> : convert_enum<mutagen_technique> {};

template<>
struct convert<cata_variant_type::mutation_category_id> : convert_string<std::string> {};

template<>
struct convert<cata_variant_type::oter_id> : convert_int_id<oter_id> {};

template<>
struct convert<cata_variant_type::skill_id> : convert_string_id<skill_id> {};

template<>
struct convert<cata_variant_type::species_id> : convert_string_id<species_id> {};

template<>
struct convert<cata_variant_type::spell_id> : convert_string_id<spell_id> {};

template<>
struct convert<cata_variant_type::string> : convert_string<std::string> {};

template<>
struct convert<cata_variant_type::trait_id> : convert_string_id<trait_id> {};

template<>
struct convert<cata_variant_type::trap_str_id> : convert_string_id<trap_str_id> {};

} // namespace cata_variant_detail

class cata_variant
{
    public:
        // Default constructor for an 'empty' variant (you can't get a value
        // from it).
        cata_variant() : type_( cata_variant_type::void_ ) {}

        // Constructor that attempts to infer the type from the type of the
        // value passed.
        template < typename Value,
                   typename = std::enable_if_t <(
                           cata_variant_detail::type_for<Value>() < cata_variant_type::num_types )>>
        explicit cata_variant( Value && value ) {
            constexpr cata_variant_type Type = cata_variant_detail::type_for<Value>();
            type_ = Type;
            value_ =
                cata_variant_detail::convert<Type>::to_string( std::forward<Value>( value ) );
        }

        // Call this function to unambiguously construct a cata_variant instance,
        // pass the type as a template parameter.
        // In cases where the value type alone is unambiguous, the above
        // simpler constructor call can be used.
        template<cata_variant_type Type, typename Value>
        static cata_variant make( Value &&value ) {
            return cata_variant(
                Type, cata_variant_detail::convert<Type>::to_string(
                    std::forward<Value>( value ) ) );
        }

        cata_variant_type type() const {
            return type_;
        }

        template<cata_variant_type Type>
        auto get() const -> typename cata_variant_detail::convert<Type>::type {
            if( type_ != Type ) {
                debugmsg( "Tried to extract type %s from cata_variant which contained %s",
                          io::enum_to_string( Type ),
                          io::enum_to_string( type_ ) );
                return {};
            }
            return cata_variant_detail::convert<Type>::from_string( value_ );
        }

        template<typename T>
        T get() const {
            return get<cata_variant_detail::type_for<T>()>();
        }

        const std::string &get_string() const {
            return value_;
        }

        std::pair<cata_variant_type, std::string> as_pair() const {
            return std::make_pair( type_, value_ );
        }

        void serialize( JsonOut & ) const;
        void deserialize( JsonIn & );

#define CATA_VARIANT_OPERATOR(op) \
    friend bool operator op( const cata_variant &l, const cata_variant &r ) { \
        return l.as_pair() op r.as_pair(); \
    }
        CATA_VARIANT_OPERATOR( == );
        CATA_VARIANT_OPERATOR( != );
        CATA_VARIANT_OPERATOR( < );
        CATA_VARIANT_OPERATOR( <= );
        CATA_VARIANT_OPERATOR( > );
        CATA_VARIANT_OPERATOR( >= );
#undef CATA_VARIANT_OPERATOR
    private:
        explicit cata_variant( cata_variant_type t, std::string &&v )
            : type_( t )
            , value_( std::move( v ) )
        {}

        cata_variant_type type_;
        std::string value_;
};

namespace std
{

template<>
struct hash<cata_variant_type> {
    size_t operator()( const cata_variant_type v ) const noexcept {
        return static_cast<size_t>( v );
    }
};

template<>
struct hash<cata_variant> {
    size_t operator()( const cata_variant &v ) const noexcept {
        return cata::tuple_hash()( v.as_pair() );
    }
};

} // namespace std

#endif // CATA_VARIANT_H
