#pragma once
#ifndef CATA_SRC_MAPGEN_POST_PROCESS_H
#define CATA_SRC_MAPGEN_POST_PROCESS_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "coords_fwd.h"
#include "type_id.h"

class JsonObject;
class JsonOut;
class JsonValue;
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

// Scope of a sub-generator decision.
// omt: fresh roll per OMT each mapgen (current behavior, not persisted).
// overmap_special: rolled once for whole special at place_special() time,
// shared across all OMTs of the special. Fixes multi-OMT and multi-z-level
// inconsistency for building-wide effects like pre_burn.
enum class pp_sub_generator_scope : int {
    omt = 0,
    overmap_special,
    last
};

template<>
struct enum_traits<pp_sub_generator_scope> {
    static constexpr pp_sub_generator_scope last = pp_sub_generator_scope::last;
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
    pp_sub_generator_scope scope = pp_sub_generator_scope::omt;

    void load( const JsonObject &jo );
    void check( const std::string &ctx ) const;
};

// Persisted decision for a single overmap_special-scoped sub-generator.
// Keyed by (type, ordinal_within_type) so stored entries survive JSON
// reorders and unrelated type swaps. pp_generator::check() enforces that
// only one entry per type exists when any entry of that type uses
// overmap_special scope, so ordinal is structurally always 0 today.
// The field exists for forward-compat if that uniqueness rule ever relaxes.
struct pp_sub_decision {
    enum class status : uint8_t { not_evaluated = 0, applied, skipped };

    sub_generator_type type = sub_generator_type::last;
    uint8_t ordinal = 0;
    status st = status::not_evaluated;
    uint32_t seed = 0;

    void serialize( JsonOut &jsout ) const;
    void deserialize( const JsonValue &jv );
};

// All persisted decisions for one pp_generator on a given overmap_special.
struct pp_resolved_generator {
    pp_generator_id generator;
    std::vector<pp_sub_decision> sub_decisions;

    void serialize( JsonOut &jsout ) const;
    void deserialize( const JsonValue &jv );
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
        // decisions: persisted gate decisions for overmap_special-scoped sub-generators,
        // or nullptr when no decisions are stored for this OMT. When nullptr, every
        // sub-generator rolls fresh per OMT.
        void execute( map &md, const tripoint_abs_omt &p,
                      std::vector<pp_sub_decision> *decisions = nullptr ) const;

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
