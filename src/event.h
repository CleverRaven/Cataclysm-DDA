#pragma once
#ifndef CATA_SRC_EVENT_H
#define CATA_SRC_EVENT_H

#include <array>
#include <cstddef>
#include <cstdlib>
#include <iosfwd>
#include <map>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "calendar.h"
#include "cata_variant.h"
#include "debug.h"

template <typename E> struct enum_traits;

// An event is something to be passed via the event_bus to subscribers
// interested in being notified about events.
//
// Each event is of a specific type, taken from the event_type enum.

enum class event_type : int {
    activates_artifact,
    activates_mininuke,
    administers_mutagen,
    angers_amigara_horrors,
    avatar_enters_omt,
    avatar_moves,
    avatar_dies,
    awakes_dark_wyrms,
    becomes_wanted,
    broken_bone,
    broken_bone_mends,
    buries_corpse,
    causes_resonance_cascade,
    // Eating is always consuming, but consuming also covers medication and
    // fueling bionics
    character_casts_spell,
    character_consumes_item,
    character_eats_item,
    character_finished_activity,
    character_forgets_spell,
    character_gains_effect,
    character_gets_headshot,
    character_heals_damage,
    character_kills_character,
    character_kills_monster,
    character_learns_spell,
    character_loses_effect,
    character_melee_attacks_character,
    character_melee_attacks_monster,
    character_ranged_attacks_character,
    character_ranged_attacks_monster,
    character_smashes_tile,
    character_starts_activity,
    character_takes_damage,
    character_triggers_trap,
    character_wakes_up,
    character_wields_item,
    character_wears_item,
    consumes_marloss_item,
    crosses_marloss_threshold,
    crosses_mutation_threshold,
    crosses_mycus_threshold,
    cuts_tree,
    dermatik_eggs_hatch,
    dermatik_eggs_injected,
    destroys_triffid_grove,
    dies_from_asthma_attack,
    dies_from_drug_overdose,
    dies_from_bleeding,
    dies_from_hypovolemia,
    dies_from_redcells_loss,
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
    game_avatar_death,
    game_avatar_new,
    game_load,
    game_begin,
    game_over,
    game_save,
    game_start,
    installs_cbm,
    installs_faulty_cbm,
    learns_martial_art,
    loses_addiction,
    npc_becomes_hostile,
    opens_portal,
    opens_spellbook,
    opens_temple,
    player_fails_conduct,
    player_gets_achievement,
    player_levels_spell,
    reads_book,
    releases_subspace_specimens,
    removes_cbm,
    seals_hazardous_material_sarcophagus,
    spellcasting_finish,
    telefrags_creature,
    teleglow_teleports,
    teleports_into_wall,
    terminates_subspace_specimens,
    throws_up,
    triggers_alarm,
    uses_debug_menu,
    u_var_changed,
    vehicle_moves,
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

struct event_spec_character_item {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "character", cata_variant_type::character_id },
            { "itype", cata_variant_type::itype_id },
        }
    };
};

static_assert( static_cast<int>( event_type::num_event_types ) == 96,
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
struct event_spec<event_type::avatar_enters_omt> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "pos", cata_variant_type::tripoint },
            { "oter_id", cata_variant_type::oter_id },
        }
    };
};

template<>
struct event_spec<event_type::avatar_moves> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 5> fields = {{
            { "mount", cata_variant_type::mtype_id },
            { "terrain", cata_variant_type::ter_id },
            { "movement_mode", cata_variant_type::move_mode_id },
            { "underwater", cata_variant_type::bool_ },
            { "z", cata_variant_type::int_ },
        }
    };
};

template<>
struct event_spec<event_type::avatar_dies> : event_spec_empty {};

template<>
struct event_spec<event_type::awakes_dark_wyrms> : event_spec_empty {};

template<>
struct event_spec<event_type::becomes_wanted> : event_spec_character {};

template<>
struct event_spec<event_type::broken_bone> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "character", cata_variant_type::character_id },
            { "part", cata_variant_type::body_part },
        }
    };
};

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
struct event_spec<event_type::character_consumes_item> : event_spec_character_item {};

template<>
struct event_spec<event_type::character_eats_item> : event_spec_character_item {};

template<>
struct event_spec<event_type::character_casts_spell> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 6> fields = { {
            { "character", cata_variant_type::character_id },
            { "spell", cata_variant_type::spell_id },
            { "difficulty", cata_variant_type::int_},
            { "cost", cata_variant_type::int_},
            { "cast_time", cata_variant_type::int_},
            { "damage", cata_variant_type::int_}

        }
    };
};

template<>
struct event_spec<event_type::character_finished_activity> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 3> fields = { {
            { "character", cata_variant_type::character_id },
            { "activity", cata_variant_type::activity_id },
            { "canceled", cata_variant_type::bool_ }
        }
    };
};

template<>
struct event_spec<event_type::character_forgets_spell> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = { {
            { "character", cata_variant_type::character_id },
            { "spell", cata_variant_type::spell_id }
        }
    };
};

template<>
struct event_spec<event_type::character_gains_effect> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 3> fields = {{
            { "character", cata_variant_type::character_id },
            { "bodypart", cata_variant_type::body_part},
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
struct event_spec<event_type::character_learns_spell> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = { {
            { "character", cata_variant_type::character_id },
            { "spell", cata_variant_type::spell_id }
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
struct event_spec<event_type::character_melee_attacks_character> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 5> fields = {{
            { "attacker", cata_variant_type::character_id },
            { "weapon", cata_variant_type::itype_id },
            { "hits", cata_variant_type::bool_ },
            { "victim", cata_variant_type::character_id },
            { "victim_name", cata_variant_type::string },
        }
    };
};

template<>
struct event_spec<event_type::character_melee_attacks_monster> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 4> fields = {{
            { "attacker", cata_variant_type::character_id },
            { "weapon", cata_variant_type::itype_id },
            { "hits", cata_variant_type::bool_ },
            { "victim_type", cata_variant_type::mtype_id },
        }
    };
};

template<>
struct event_spec<event_type::character_ranged_attacks_character> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 4> fields = {{
            { "attacker", cata_variant_type::character_id },
            { "weapon", cata_variant_type::itype_id },
            { "victim", cata_variant_type::character_id },
            { "victim_name", cata_variant_type::string },
        }
    };
};

template<>
struct event_spec<event_type::character_ranged_attacks_monster> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 3> fields = {{
            { "attacker", cata_variant_type::character_id },
            { "weapon", cata_variant_type::itype_id },
            { "victim_type", cata_variant_type::mtype_id },
        }
    };
};

template<>
struct event_spec<event_type::character_smashes_tile> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 3> fields = {{
            { "character", cata_variant_type::character_id },
            { "terrain", cata_variant_type::ter_str_id },
            { "furniture", cata_variant_type::furn_str_id },
        }
    };
};

template<>
struct event_spec<event_type::character_starts_activity> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 3> fields = { {
            { "character", cata_variant_type::character_id },
            { "activity", cata_variant_type::activity_id },
            { "resume", cata_variant_type::bool_ }
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
struct event_spec<event_type::character_wakes_up> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 1> fields = {{
            { "character", cata_variant_type::character_id },
        }
    };
};

template<>
struct event_spec<event_type::character_wears_item> : event_spec_character_item {};

template<>
struct event_spec<event_type::character_wields_item> : event_spec_character_item {};

template<>
struct event_spec<event_type::consumes_marloss_item> : event_spec_character_item {};

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
struct event_spec<event_type::cuts_tree> : event_spec_character {};

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
struct event_spec<event_type::dies_from_bleeding> : event_spec_character {};

template<>
struct event_spec<event_type::dies_from_hypovolemia> : event_spec_character {};

template<>
struct event_spec<event_type::dies_from_redcells_loss> : event_spec_character {};

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
            { "add_type", cata_variant_type::addiction_id },
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
struct event_spec<event_type::game_avatar_death> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 5> fields = {{
            { "avatar_id", cata_variant_type::character_id },
            { "avatar_name", cata_variant_type::string },
            { "avatar_is_male", cata_variant_type::bool_ },
            { "is_suicide", cata_variant_type::bool_ },
            { "last_words", cata_variant_type::string },
        }
    };
};

template<>
struct event_spec<event_type::game_avatar_new> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 7> fields = {{
            { "is_new_game", cata_variant_type::bool_ },
            { "is_debug", cata_variant_type::bool_ },
            { "avatar_id", cata_variant_type::character_id },
            { "avatar_name", cata_variant_type::string },
            { "avatar_is_male", cata_variant_type::bool_ },
            { "avatar_profession", cata_variant_type::profession_id },
            { "avatar_custom_profession", cata_variant_type::string },
        }
    };
};

template<>
struct event_spec<event_type::game_load> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 1> fields = {{
            { "cdda_version", cata_variant_type::string },
        }
    };
};

template<>
struct event_spec<event_type::game_begin> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 1> fields = { {
            { "cdda_version", cata_variant_type::string },
        }
    };
};

template<>
struct event_spec<event_type::game_over> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 1> fields = {{
            { "total_time_played", cata_variant_type::chrono_seconds },
        }
    };
};

template<>
struct event_spec<event_type::game_save> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "time_since_load", cata_variant_type::chrono_seconds },
            { "total_time_played", cata_variant_type::chrono_seconds },
        }
    };
};

template<>
struct event_spec<event_type::game_start> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 1> fields = {{
            { "game_version", cata_variant_type::string },
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
            { "add_type", cata_variant_type::addiction_id },
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
struct event_spec<event_type::opens_spellbook> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 1> fields = { {
            { "character", cata_variant_type::character_id }
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
struct event_spec<event_type::player_fails_conduct> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "conduct", cata_variant_type::achievement_id },
            { "achievements_enabled", cata_variant_type::bool_ },
        }
    };
};

template<>
struct event_spec<event_type::player_gets_achievement> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "achievement", cata_variant_type::achievement_id },
            { "achievements_enabled", cata_variant_type::bool_ },
        }
    };
};

template<>
struct event_spec<event_type::player_levels_spell> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 3> fields = {{
            { "character", cata_variant_type::character_id },
            { "spell", cata_variant_type::spell_id },
            { "new_level", cata_variant_type::int_ },
        }
    };
};

template<>
struct event_spec<event_type::reads_book> : event_spec_character_item {};

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
struct event_spec<event_type::spellcasting_finish> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 3> fields = { {
            { "character", cata_variant_type::character_id },
            { "spell", cata_variant_type::spell_id },
            { "school", cata_variant_type::trait_id }
        }
    };
};

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

template<>
struct event_spec<event_type::uses_debug_menu> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 1> fields = {{
            { "debug_menu_option", cata_variant_type::debug_menu_index },
        }
    };
};

template<>
struct event_spec<event_type::u_var_changed> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = { {
            { "var", cata_variant_type::string },
            { "value", cata_variant_type::string },
        }
    };
};

template<>
struct event_spec<event_type::vehicle_moves> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 11> fields = {{
            { "avatar_on_board", cata_variant_type::bool_ },
            { "avatar_is_driving", cata_variant_type::bool_ }, // non-remote-control
            { "avatar_remote_control", cata_variant_type::bool_ },
            { "is_flying_aircraft", cata_variant_type::bool_ }, // actual viable aircraft
            { "is_floating_watercraft", cata_variant_type::bool_ }, // actual viable boat
            { "is_on_rails", cata_variant_type::bool_ }, // railway vehicle on rails
            { "is_falling", cata_variant_type::bool_ }, // not an aircraft, just getting air time
            { "is_sinking", cata_variant_type::bool_ }, // sinking in water
            { "is_skidding", cata_variant_type::bool_ },
            { "velocity", cata_variant_type::int_ }, // vehicle current velocity, mph * 100
            { "z", cata_variant_type::int_ },
        }
    };
};

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

        using fields_type = std::unordered_map<std::string, cata_variant_type>;
        static fields_type get_fields( event_type );

        event_type type() const {
            return type_;
        }
        time_point time() const {
            return time_;
        }

        cata_variant get_variant( const std::string &key ) const;
        cata_variant get_variant_or_void( const std::string &key ) const;

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

#endif // CATA_SRC_EVENT_H
