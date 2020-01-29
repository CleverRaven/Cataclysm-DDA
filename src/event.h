#pragma once
#ifndef CATA_EVENT_H
#define CATA_EVENT_H

#include <map>
#include <utility>

#include "calendar.h"
#include "cata_variant.h"
#include "enum_conversions.h"
#include "type_id.h"

using itype_id = std::string;

// An event is something to be passed via the event_bus to subscribers
// interested in being notified about events.
//
// Each event is of a specific type, taken from the event_type enum.

enum class event_type {
    activates_artifact,
    activates_mininuke,
    administers_mutagen,
    angers_amigara_horrors,
    avatar_moves,
    awakes_dark_wyrms,
    becomes_wanted,
    broken_bone_mends,
    buries_corpse,
    causes_resonance_cascade,
    character_gains_effect,
    character_gets_headshot,
    character_heals_damage,
    character_kills_character,
    character_kills_monster,
    character_loses_effect,
    character_takes_damage,
    character_triggers_trap,
    consumes_marloss_item,
    crosses_marloss_threshold,
    crosses_mutation_threshold,
    crosses_mycus_threshold,
    dermatik_eggs_hatch,
    dermatik_eggs_injected,
    destroys_triffid_grove,
    dies_from_asthma_attack,
    dies_from_drug_overdose,
    dies_of_infection,
    dies_of_starvation,
    dies_of_thirst,
    digs_into_lava,
    disarms_nuke,
    eats_sewage,
    evolves_mutation,
    exhumes_grave,
    fails_to_install_cbm,
    fails_to_remove_cbm,
    falls_asleep_from_exhaustion,
    fuel_tank_explodes,
    gains_addiction,
    gains_mutation,
    gains_skill_level,
    game_over,
    game_start,
    installs_cbm,
    installs_faulty_cbm,
    learns_martial_art,
    loses_addiction,
    npc_becomes_hostile,
    opens_portal,
    opens_temple,
    player_levels_spell,
    releases_subspace_specimens,
    removes_cbm,
    seals_hazardous_material_sarcophagus,
    telefrags_creature,
    teleglow_teleports,
    teleports_into_wall,
    terminates_subspace_specimens,
    throws_up,
    triggers_alarm,
    num_event_types // last
};

template<>
struct enum_traits<event_type> {
    static constexpr event_type last = event_type::num_event_types;
};

namespace io
{

template<>
std::string enum_to_string<event_type>( event_type data );

} // namespace io

namespace std
{

template<>
struct hash<event_type> {
    size_t operator()( const event_type v ) const noexcept {
        return static_cast<size_t>( v );
    }
};

} // namespace std

namespace cata
{

class event;

namespace event_detail
{

// An event has various data stored in a map keyed by strings.  The specific
// keys and corresponding data types are specified in a specialization of
// event_spec.

template<event_type Type>
struct event_spec;

struct event_spec_empty {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 0> fields = {};
};

struct event_spec_character {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 1> fields = {{
            { "character", cata_variant_type::character_id },
        }
    };
};

static_assert( static_cast<int>( event_type::num_event_types ) == 61,
               "This static_assert is to remind you to add a specialization for your new "
               "event_type below" );

template<>
struct event_spec<event_type::activates_artifact> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "character", cata_variant_type::character_id },
            { "item_name", cata_variant_type::string },
        }
    };
};

template<>
struct event_spec<event_type::activates_mininuke> : event_spec_character {};

template<>
struct event_spec<event_type::administers_mutagen> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "character", cata_variant_type::character_id },
            { "technique", cata_variant_type::mutagen_technique },
        }
    };
};

template<>
struct event_spec<event_type::angers_amigara_horrors> : event_spec_empty {};

template<>
struct event_spec<event_type::avatar_moves> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 1> fields = {{
            { "mount", cata_variant_type::mtype_id },
        }
    };
};

template<>
struct event_spec<event_type::awakes_dark_wyrms> : event_spec_empty {};

template<>
struct event_spec<event_type::becomes_wanted> : event_spec_character {};

template<>
struct event_spec<event_type::broken_bone_mends> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "character", cata_variant_type::character_id },
            { "part", cata_variant_type::body_part },
        }
    };
};

template<>
struct event_spec<event_type::buries_corpse> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 3> fields = {{
            { "character", cata_variant_type::character_id },
            { "corpse_type", cata_variant_type::mtype_id },
            { "corpse_name", cata_variant_type::string },
        }
    };
};

template<>
struct event_spec<event_type::causes_resonance_cascade> : event_spec_empty {};

template<>
struct event_spec<event_type::character_gains_effect> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "character", cata_variant_type::character_id },
            { "effect", cata_variant_type::efftype_id },
        }
    };
};

template<>
struct event_spec<event_type::character_gets_headshot> : event_spec_character {};

template<>
struct event_spec<event_type::character_heals_damage> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "character", cata_variant_type::character_id },
            { "damage", cata_variant_type::int_ },
        }
    };
};

template<>
struct event_spec<event_type::character_kills_monster> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "killer", cata_variant_type::character_id },
            { "victim_type", cata_variant_type::mtype_id },
        }
    };
};

template<>
struct event_spec<event_type::character_kills_character> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 3> fields = {{
            { "killer", cata_variant_type::character_id },
            { "victim", cata_variant_type::character_id },
            { "victim_name", cata_variant_type::string },
        }
    };
};

template<>
struct event_spec<event_type::character_loses_effect> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "character", cata_variant_type::character_id },
            { "effect", cata_variant_type::efftype_id },
        }
    };
};

template<>
struct event_spec<event_type::character_takes_damage> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "character", cata_variant_type::character_id },
            { "damage", cata_variant_type::int_ },
        }
    };
};

template<>
struct event_spec<event_type::character_triggers_trap> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "character", cata_variant_type::character_id },
            { "trap", cata_variant_type::trap_str_id },
        }
    };
};

template<>
struct event_spec<event_type::consumes_marloss_item> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "character", cata_variant_type::character_id },
            { "itype", cata_variant_type::itype_id },
        }
    };
};

template<>
struct event_spec<event_type::crosses_marloss_threshold> : event_spec_character {};

template<>
struct event_spec<event_type::crosses_mutation_threshold> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "character", cata_variant_type::character_id },
            { "category", cata_variant_type::mutation_category_id },
        }
    };
};

template<>
struct event_spec<event_type::crosses_mycus_threshold> : event_spec_character {};

template<>
struct event_spec<event_type::dermatik_eggs_hatch> : event_spec_character {};

template<>
struct event_spec<event_type::dermatik_eggs_injected> : event_spec_character {};

template<>
struct event_spec<event_type::destroys_triffid_grove> : event_spec_empty {};

template<>
struct event_spec<event_type::dies_from_asthma_attack> : event_spec_character {};

template<>
struct event_spec<event_type::dies_from_drug_overdose> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "character", cata_variant_type::character_id },
            { "effect", cata_variant_type::efftype_id },
        }
    };
};

template<>
struct event_spec<event_type::dies_of_infection> : event_spec_character {};

template<>
struct event_spec<event_type::dies_of_starvation> : event_spec_character {};

template<>
struct event_spec<event_type::dies_of_thirst> : event_spec_character {};

template<>
struct event_spec<event_type::digs_into_lava> : event_spec_empty {};

template<>
struct event_spec<event_type::disarms_nuke> : event_spec_empty {};

template<>
struct event_spec<event_type::eats_sewage> : event_spec_empty {};

template<>
struct event_spec<event_type::evolves_mutation> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 3> fields = {{
            { "character", cata_variant_type::character_id },
            { "from_trait", cata_variant_type::trait_id },
            { "to_trait", cata_variant_type::trait_id },
        }
    };
};

template<>
struct event_spec<event_type::exhumes_grave> : event_spec_character {};

template<>
struct event_spec<event_type::fails_to_install_cbm> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "character", cata_variant_type::character_id },
            { "bionic", cata_variant_type::bionic_id },
        }
    };
};

template<>
struct event_spec<event_type::fails_to_remove_cbm> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "character", cata_variant_type::character_id },
            { "bionic", cata_variant_type::bionic_id },
        }
    };
};

template<>
struct event_spec<event_type::falls_asleep_from_exhaustion> : event_spec_character {};

template<>
struct event_spec<event_type::fuel_tank_explodes> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 1> fields = {{
            { "vehicle_name", cata_variant_type::string },
        }
    };
};

template<>
struct event_spec<event_type::gains_addiction> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "character", cata_variant_type::character_id },
            { "add_type", cata_variant_type::add_type },
        }
    };
};

template<>
struct event_spec<event_type::gains_mutation> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "character", cata_variant_type::character_id },
            { "trait", cata_variant_type::trait_id },
        }
    };
};

template<>
struct event_spec<event_type::gains_skill_level> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 3> fields = {{
            { "character", cata_variant_type::character_id },
            { "skill", cata_variant_type::skill_id },
            { "new_level", cata_variant_type::int_ },
        }
    };
};

template<>
struct event_spec<event_type::game_over> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "is_suicide", cata_variant_type::bool_ },
            { "last_words", cata_variant_type::string },
        }
    };
};

template<>
struct event_spec<event_type::game_start> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 1> fields = {{
            { "avatar_id", cata_variant_type::character_id },
        }
    };
};

template<>
struct event_spec<event_type::installs_cbm> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "character", cata_variant_type::character_id },
            { "bionic", cata_variant_type::bionic_id },
        }
    };
};

template<>
struct event_spec<event_type::installs_faulty_cbm> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "character", cata_variant_type::character_id },
            { "bionic", cata_variant_type::bionic_id },
        }
    };
};

template<>
struct event_spec<event_type::learns_martial_art> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "character", cata_variant_type::character_id },
            { "martial_art", cata_variant_type::matype_id },
        }
    };
};

template<>
struct event_spec<event_type::loses_addiction> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "character", cata_variant_type::character_id },
            { "add_type", cata_variant_type::add_type },
        }
    };
};

template<>
struct event_spec<event_type::npc_becomes_hostile> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "npc", cata_variant_type::character_id },
            { "npc_name", cata_variant_type::string },
        }
    };
};

template<>
struct event_spec<event_type::opens_portal> : event_spec_empty {};

template<>
struct event_spec<event_type::opens_temple> : event_spec_empty {};

template<>
struct event_spec<event_type::releases_subspace_specimens> : event_spec_empty {};

template<>
struct event_spec<event_type::player_levels_spell> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "spell", cata_variant_type::spell_id },
            { "new_level", cata_variant_type::int_ },
        }
    };
};

template<>
struct event_spec<event_type::removes_cbm> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "character", cata_variant_type::character_id },
            { "bionic", cata_variant_type::bionic_id },
        }
    };
};

template<>
struct event_spec<event_type::seals_hazardous_material_sarcophagus> : event_spec_empty {};

template<>
struct event_spec<event_type::telefrags_creature> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "character", cata_variant_type::character_id },
            { "victim_name", cata_variant_type::string },
        }
    };
};

template<>
struct event_spec<event_type::teleglow_teleports> : event_spec_character {};

template<>
struct event_spec<event_type::teleports_into_wall> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "character", cata_variant_type::character_id },
            { "obstacle_name", cata_variant_type::string },
        }
    };
};

template<>
struct event_spec<event_type::terminates_subspace_specimens> : event_spec_empty {};

template<>
struct event_spec<event_type::throws_up> : event_spec_character {};

template<>
struct event_spec<event_type::triggers_alarm> : event_spec_character {};

template<event_type Type, typename IndexSequence>
struct make_event_helper;

} // namespace event_detail

class event
{
    public:
        using data_type = std::map<std::string, cata_variant>;

        event( event_type type, time_point time, data_type &&data )
            : type_( type )
            , time_( time )
            , data_( std::move( data ) )
        {}

        // Call this to construct an event in a type-safe manner.  It will
        // verify that the types you pass match the expected types for the
        // event_type you pass as a template parameter.
        template<event_type Type, typename... Args>
        static event make( Args &&... args ) {
            using Spec = event_detail::event_spec<Type>;
            // Using is_empty mostly just to verify that the type is defined at
            // all, but it so happens that it ought to be empty too.
            static_assert( std::is_empty<Spec>::value,
                           "spec for this event type must be defined and empty" );
            static_assert( sizeof...( Args ) == Spec::fields.size(),
                           "wrong number of arguments for event type" );

            return event_detail::make_event_helper <
                   Type, std::make_index_sequence<sizeof...( Args )>
                   > ()( calendar::turn, std::forward<Args>( args )... );
        }

        static std::map<std::string, cata_variant_type> get_fields( event_type );

        event_type type() const {
            return type_;
        }
        time_point time() const {
            return time_;
        }

        cata_variant get_variant( const std::string &key ) const {
            auto it = data_.find( key );
            if( it == data_.end() ) {
                debugmsg( "No such key %s in event of type %s", key,
                          io::enum_to_string( type_ ) );
                abort();
            }
            return it->second;
        }

        template<cata_variant_type Type>
        auto get( const std::string &key ) const {
            return get_variant( key ).get<Type>();
        }

        template<typename T>
        auto get( const std::string &key ) const {
            return get_variant( key ).get<T>();
        }

        const data_type &data() const {
            return data_;
        }
    private:
        event_type type_;
        time_point time_;
        data_type data_;
};

namespace event_detail
{

template<event_type Type, size_t... I>
struct make_event_helper<Type, std::index_sequence<I...>> {
    using Spec = event_spec<Type>;

    template<typename... Args>
    event operator()( time_point time, Args &&... args ) {
        return event(
                   Type,
                   time,
        std::map<std::string, cata_variant> { {
                Spec::fields[I].first,
                cata_variant::make<Spec::fields[I].second>( args )
            } ...
        } );
    }
};

} // namespace event_detail

} // namespace cata

#endif // CATA_EVENT_H
