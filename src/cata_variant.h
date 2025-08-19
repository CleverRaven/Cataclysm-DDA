#pragma once
#ifndef CATA_SRC_CATA_VARIANT_H
#define CATA_SRC_CATA_VARIANT_H

#include <array>
#include <chrono>
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "character_id.h"
#include "debug.h"
#include "enum_conversions.h"
#include "hash_utils.h"
#include "point.h"
#include "to_string_id.h"
#include "type_id.h"  // IWYU pragma: keep

class JsonOut;
class JsonValue;
enum class mutagen_technique : int;
template <typename E> struct enum_traits;

namespace debug_menu
{
enum class debug_menu_index : int;
} // namespace debug_menu

// cata_variant is a variant-like type that stores a variety of different cata
// types.  All types are stored by converting them to a string.

enum class cata_variant_type : int {
    void_, // Special type for empty variants
    achievement_id,
    activity_id,
    addiction_id,
    bionic_id,
    body_part,
    bool_,
    character_id,
    chrono_seconds,
    debug_menu_index,
    efftype_id,
    faction_id,
    field_type_id,
    field_type_str_id,
    furn_id,
    furn_str_id,
    flag_id,
    int_,
    item_group_id,
    itype_id,
    matype_id,
    mtype_id,
    mongroup_id,
    move_mode_id,
    mutagen_technique,
    mutation_category_id,
    nested_mapgen_id,
    npc_template_id,
    oter_id,
    oter_type_str_id,
    overmap_special_id,
    palette_id,
    point,
    profession_id,
    proficiency_id,
    skill_id,
    species_id,
    spell_id,
    string,
    ter_id,
    ter_furn_transform_id,
    ter_str_id,
    trait_id,
    trap_id,
    trap_str_id,
    tripoint,
    vgroup_id,
    widget_id,
    zone_type_id,
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
            std::is_same_v<T, typename convert<static_cast<cata_variant_type>( I )>::type>...
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

// Inherit from this struct to easily implement convert specializations for any
// string type
template<typename T>
struct convert_string {
    using type = T;
    static_assert( std::is_same_v<T, std::string>,
                   "Intended for use only with string typedefs" );
    static std::string to_string( const T &v ) {
        return v;
    }
    static T from_string( const std::string &v ) {
        return v;
    }
    static bool is_valid( const std::string_view ) {
        return true;
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
    static bool is_valid( const std::string &v ) {
        return from_string( v ).is_valid();
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
    static bool is_valid( const std::string &v ) {
        return to_string_id_t<T>( v ).is_valid();
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
    static bool is_valid( const std::string &v ) {
        return io::enum_is_valid<T>( v );
    }
};

// These are the specializations of convert for each value type.
static_assert( static_cast<int>( cata_variant_type::num_types ) == 49,
               "This assert is a reminder to add conversion support for any new types to the "
               "below specializations" );

template<>
struct convert<cata_variant_type::void_> {
    using type = void;
    static bool is_valid( const std::string_view s ) {
        return s.empty();
    }
};

template<>
struct convert<cata_variant_type::achievement_id> : convert_string_id<achievement_id> {};

template<>
struct convert<cata_variant_type::activity_id> : convert_string_id<activity_id> {};

template<>
struct convert<cata_variant_type::addiction_id> : convert_string_id<addiction_id> {};

template<>
struct convert<cata_variant_type::bionic_id> : convert_string_id<bionic_id> {};

template<>
struct convert<cata_variant_type::body_part> : convert_int_id<bodypart_id> {};

template<>
struct convert<cata_variant_type::bool_> {
    using type = bool;
    static std::string to_string( const bool v ) {
        return std::to_string( static_cast<int>( v ) );
    }
    static bool from_string( const std::string &v ) {
        return std::stoi( v );
    }
    static bool is_valid( const std::string &v ) {
        // TODO: check for int-ness
        int i = std::stoi( v );
        return i >= 0 && i <= 1;
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
    static bool is_valid( const std::string_view ) {
        // TODO: check for int-ness
        return true;
    }
};

template<>
struct convert<cata_variant_type::chrono_seconds> {
    using type = std::chrono::seconds;
    static std::string to_string( const std::chrono::seconds &v ) {
        return std::to_string( v.count() );
    }
    static std::chrono::seconds from_string( const std::string &v ) {
        return std::chrono::seconds( std::stoll( v ) );
    }
    static bool is_valid( const std::string_view ) {
        // TODO: check for int-ness
        return true;
    }
};

template<>
struct convert<cata_variant_type::debug_menu_index> : convert_enum<debug_menu::debug_menu_index> {};

template<>
struct convert<cata_variant_type::move_mode_id> : convert_string_id<move_mode_id> {};

template<>
struct convert<cata_variant_type::efftype_id> : convert_string_id<efftype_id> {};

template<>
struct convert<cata_variant_type::faction_id> : convert_string_id<faction_id> {};

template<>
struct convert<cata_variant_type::field_type_id> : convert_int_id<field_type_id> {};

template<>
struct convert<cata_variant_type::field_type_str_id> : convert_string_id<field_type_str_id> {};

template<>
struct convert<cata_variant_type::furn_id> : convert_int_id<furn_id> {};

template<>
struct convert<cata_variant_type::furn_str_id> : convert_string_id<furn_str_id> {};

template<>
struct convert<cata_variant_type::flag_id> : convert_string_id<flag_id> {};

template<>
struct convert<cata_variant_type::int_> {
    using type = int;
    static std::string to_string( const int v ) {
        return std::to_string( v );
    }
    static int from_string( const std::string &v ) {
        return std::stoi( v );
    }
    static bool is_valid( const std::string_view ) {
        // TODO: check for int-ness
        return true;
    }
};

template<>
struct convert<cata_variant_type::item_group_id> : convert_string_id<item_group_id> {};

template<>
struct convert<cata_variant_type::itype_id> : convert_string_id<itype_id> {};

template<>
struct convert<cata_variant_type::matype_id> : convert_string_id<matype_id> {};

template<>
struct convert<cata_variant_type::mtype_id> : convert_string_id<mtype_id> {};

template<>
struct convert<cata_variant_type::mongroup_id> : convert_string_id<mongroup_id> {};

template<>
struct convert<cata_variant_type::mutagen_technique> : convert_enum<mutagen_technique> {};

template<>
struct convert<cata_variant_type::mutation_category_id> :
    convert_string_id<mutation_category_id> {};

template<>
struct convert<cata_variant_type::nested_mapgen_id> : convert_string_id<nested_mapgen_id> {};

template<>
struct convert<cata_variant_type::npc_template_id> : convert_string_id<npc_template_id> {};

template<>
struct convert<cata_variant_type::oter_id> : convert_int_id<oter_id> {};

template<>
struct convert<cata_variant_type::oter_type_str_id> : convert_string_id<oter_type_str_id> {};

template<>
struct convert<cata_variant_type::overmap_special_id> : convert_string_id<overmap_special_id> {};

template<>
struct convert<cata_variant_type::palette_id> : convert_string_id<palette_id> {};

template<>
struct convert<cata_variant_type::point> {
    using type = point;
    static std::string to_string( const point &v ) {
        return v.to_string();
    }
    static point from_string( const std::string &v ) {
        return point::from_string( v );
    }
    static bool is_valid( const std::string_view ) {
        // TODO: check for point-ness
        return true;
    }
};

template<>
struct convert<cata_variant_type::profession_id> : convert_string_id<profession_id> {};

template<>
struct convert<cata_variant_type::proficiency_id> : convert_string_id<proficiency_id> {};

template<>
struct convert<cata_variant_type::skill_id> : convert_string_id<skill_id> {};

template<>
struct convert<cata_variant_type::species_id> : convert_string_id<species_id> {};

template<>
struct convert<cata_variant_type::spell_id> : convert_string_id<spell_id> {};

template<>
struct convert<cata_variant_type::string> : convert_string<std::string> {};

template<>
struct convert<cata_variant_type::ter_id> : convert_int_id<ter_id> {};

template<>
struct convert<cata_variant_type::ter_furn_transform_id> :
    convert_string_id<ter_furn_transform_id> {};

template<>
struct convert<cata_variant_type::ter_str_id> : convert_string_id<ter_str_id> {};

template<>
struct convert<cata_variant_type::trait_id> : convert_string_id<trait_id> {};

template<>
struct convert<cata_variant_type::trap_id> : convert_int_id<trap_id> {};

template<>
struct convert<cata_variant_type::trap_str_id> : convert_string_id<trap_str_id> {};

template<>
struct convert<cata_variant_type::tripoint> {
    using type = tripoint;
    static std::string to_string( const tripoint &v ) {
        return v.to_string();
    }
    static tripoint from_string( const std::string &v ) {
        return tripoint::from_string( v );
    }
    static bool is_valid( const std::string_view ) {
        // TODO: check for tripoint-ness
        return true;
    }
};

template<>
struct convert<cata_variant_type::vgroup_id> : convert_string_id<vgroup_id> {};

template<>
struct convert<cata_variant_type::widget_id> : convert_string_id<widget_id> {};

template<>
struct convert<cata_variant_type::zone_type_id> : convert_string_id<zone_type_id> {};

} // namespace cata_variant_detail

template<typename T>
constexpr cata_variant_type cata_variant_type_for()
{
    constexpr size_t num_types = static_cast<size_t>( cata_variant_type::num_types );
    using SimpleT = std::remove_cv_t<std::remove_reference_t<T>>;
    return cata_variant_detail::type_for_impl<SimpleT>( std::make_index_sequence<num_types> {} );
}

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
                           cata_variant_type_for<Value>() < cata_variant_type::num_types )>>
        explicit cata_variant( Value && value ) {
            constexpr cata_variant_type Type = cata_variant_type_for<Value>();
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

        // Call this to construct from a type + string.  This should rarely be
        // necessary, so think twice before using it (that's why the equivalent
        // constructor is private).
        static cata_variant from_string( cata_variant_type t, std::string &&v ) {
            return cata_variant( t, std::move( v ) );
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
            return get<cata_variant_type_for<T>()>();
        }

        const std::string &get_string() const {
            return value_;
        }

        bool is_valid() const;

        std::pair<cata_variant_type, std::string> as_pair() const {
            return std::make_pair( type_, value_ );
        }

        void serialize( JsonOut & ) const;
        void deserialize( const JsonValue &jsin );

#define CATA_VARIANT_OPERATOR(op) \
    friend bool operator op( const cata_variant &l, const cata_variant &r ) { \
        return l.as_pair() op r.as_pair(); \
    }
        CATA_VARIANT_OPERATOR( == )
        CATA_VARIANT_OPERATOR( != )
        CATA_VARIANT_OPERATOR( < ) // NOLINT( cata-use-localized-sorting )
        CATA_VARIANT_OPERATOR( <= ) // NOLINT( cata-use-localized-sorting )
        CATA_VARIANT_OPERATOR( > ) // NOLINT( cata-use-localized-sorting )
        CATA_VARIANT_OPERATOR( >= ) // NOLINT( cata-use-localized-sorting )
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

#endif // CATA_SRC_CATA_VARIANT_H
