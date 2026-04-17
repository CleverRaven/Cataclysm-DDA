#pragma once
#ifndef CATA_SRC_MAPGEN_POST_PROCESS_H
#define CATA_SRC_MAPGEN_POST_PROCESS_H

#include <string>
#include <string_view>
#include <vector>

#include "coords_fwd.h"
#include "type_id.h"

class JsonObject;
class map;
template<typename E> struct enum_traits;
template<typename T> class generic_factory;

// Sub-generator type enum: each value corresponds to a C++ executor function.
enum class sub_generator_type : int {
    bash_damage = 0,
    move_items,
    add_fire,
    pre_burn,
    place_blood,
    aftershock_ruin,
    last
};

template<>
struct enum_traits<sub_generator_type> {
    static constexpr sub_generator_type last = sub_generator_type::last;
};

// A single sub-generator entry within a pp_generator.
// Not a generic_factory type -- loaded as an embedded sub-object of pp_generator.
struct pp_sub_generator {
    sub_generator_type type = sub_generator_type::bash_damage;
    int attempts = 0;
    int chance = 0;           // per-attempt probability; meaning is type-dependent
    // (percent [0-100] for bash/move, permille [0-1000] for blood,
    // unused/0 for add_fire/pre_burn/aftershock_ruin)
    int min_intensity = 0;
    int max_intensity = 0;
    int scaling_days_start = 0;
    int scaling_days_end = 0;

    void load( const JsonObject &jo );
    void check( const std::string &ctx ) const;
};

// A pp_generator: a named collection of sub-generators applied as a group.
// Registered via generic_factory.
class pp_generator
{
    public:
        pp_generator_id id = pp_generator_id::NULL_ID();

        const std::vector<pp_sub_generator> &sub_generators() const {
            return sub_generators_;
        }

        // Execute this generator on the given map at the given overmap position.
        void execute( map &md, const tripoint_abs_omt &p ) const;

    private:
        void load( const JsonObject &jo, std::string_view src );
        void check() const;
        bool was_loaded = false;
        friend class generic_factory<pp_generator>;

        std::vector<pp_sub_generator> sub_generators_;
};

namespace pp_generators
{
void load( const JsonObject &jo, const std::string &src );
void reset();
void finalize();
void check_consistency();
} // namespace pp_generators

#endif // CATA_SRC_MAPGEN_POST_PROCESS_H
