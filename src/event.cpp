#include "event.h"

namespace io
{

template<>
std::string enum_to_string<event_type>( event_type data )
{
    switch( data ) {
        // *INDENT-OFF*
        case event_type::activates_artifact: return "activates_artifact";
        case event_type::activates_mininuke: return "activates_mininuke";
        case event_type::administers_mutagen: return "administers_mutagen";
        case event_type::angers_amigara_horrors: return "angers_amigara_horrors";
        case event_type::avatar_moves: return "avatar_moves";
        case event_type::awakes_dark_wyrms: return "awakes_dark_wyrms";
        case event_type::becomes_wanted: return "becomes_wanted";
        case event_type::broken_bone_mends: return "broken_bone_mends";
        case event_type::buries_corpse: return "buries_corpse";
        case event_type::causes_resonance_cascade: return "causes_resonance_cascade";
        case event_type::character_gains_effect: return "character_gains_effect";
        case event_type::character_gets_headshot: return "character_gets_headshot";
        case event_type::character_heals_damage: return "character_heals_damage";
        case event_type::character_kills_character: return "character_kills_character";
        case event_type::character_kills_monster: return "character_kills_monster";
        case event_type::character_loses_effect: return "character_loses_effect";
        case event_type::character_takes_damage: return "character_takes_damage";
        case event_type::character_triggers_trap: return "character_triggers_trap";
        case event_type::consumes_marloss_item: return "consumes_marloss_item";
        case event_type::crosses_marloss_threshold: return "crosses_marloss_threshold";
        case event_type::crosses_mutation_threshold: return "crosses_mutation_threshold";
        case event_type::crosses_mycus_threshold: return "crosses_mycus_threshold";
        case event_type::dermatik_eggs_hatch: return "dermatik_eggs_hatch";
        case event_type::dermatik_eggs_injected: return "dermatik_eggs_injected";
        case event_type::destroys_triffid_grove: return "destroys_triffid_grove";
        case event_type::dies_from_asthma_attack: return "dies_from_asthma_attack";
        case event_type::dies_from_drug_overdose: return "dies_from_drug_overdose";
        case event_type::dies_of_infection: return "dies_of_infection";
        case event_type::dies_of_starvation: return "dies_of_starvation";
        case event_type::dies_of_thirst: return "dies_of_thirst";
        case event_type::digs_into_lava: return "digs_into_lava";
        case event_type::disarms_nuke: return "disarms_nuke";
        case event_type::eats_sewage: return "eats_sewage";
        case event_type::evolves_mutation: return "evolves_mutation";
        case event_type::exhumes_grave: return "exhumes_grave";
        case event_type::fails_to_install_cbm: return "fails_to_install_cbm";
        case event_type::fails_to_remove_cbm: return "fails_to_remove_cbm";
        case event_type::falls_asleep_from_exhaustion: return "falls_asleep_from_exhaustion";
        case event_type::fuel_tank_explodes: return "fuel_tank_explodes";
        case event_type::gains_addiction: return "gains_addiction";
        case event_type::gains_mutation: return "gains_mutation";
        case event_type::gains_skill_level: return "gains_skill_level";
        case event_type::game_over: return "game_over";
        case event_type::game_start: return "game_start";
        case event_type::installs_cbm: return "installs_cbm";
        case event_type::installs_faulty_cbm: return "installs_faulty_cbm";
        case event_type::launches_nuke: return "launches_nuke";
        case event_type::learns_martial_art: return "learns_martial_art";
        case event_type::loses_addiction: return "loses_addiction";
        case event_type::npc_becomes_hostile: return "npc_becomes_hostile";
        case event_type::opens_portal: return "opens_portal";
        case event_type::opens_temple: return "opens_temple";
        case event_type::releases_subspace_specimens: return "releases_subspace_specimens";
        case event_type::removes_cbm: return "removes_cbm";
        case event_type::seals_hazardous_material_sarcophagus: return "seals_hazardous_material_sarcophagus";
        case event_type::telefrags_creature: return "telefrags_creature";
        case event_type::teleglow_teleports: return "teleglow_teleports";
        case event_type::teleports_into_wall: return "teleports_into_wall";
        case event_type::terminates_subspace_specimens: return "terminates_subspace_specimens";
        case event_type::throws_up: return "throws_up";
        case event_type::triggers_alarm: return "triggers_alarm";
        // *INDENT-ON*
        case event_type::num_event_types:
            break;
    }
    debugmsg( "Invalid event_type" );
    abort();
}

} // namespace io

namespace cata
{

namespace event_detail
{

constexpr std::array<std::pair<const char *, cata_variant_type>,
          event_spec_empty::fields.size()> event_spec_empty::fields;

constexpr std::array<std::pair<const char *, cata_variant_type>,
          event_spec_character::fields.size()> event_spec_character::fields;

static_assert( static_cast<int>( event_type::num_event_types ) == 61,
               "This static_assert is a reminder to add a definition below when you add a new "
               "event_type.  If your event_spec specialization inherits from another struct for "
               "its fields definition then you probably don't need a definition here." );

#define DEFINE_EVENT_FIELDS(type) \
    constexpr std::array<std::pair<const char *, cata_variant_type>, \
    event_spec<event_type::type>::fields.size()> \
    event_spec<event_type::type>::fields;

DEFINE_EVENT_FIELDS( activates_artifact )
DEFINE_EVENT_FIELDS( administers_mutagen )
DEFINE_EVENT_FIELDS( avatar_moves )
DEFINE_EVENT_FIELDS( broken_bone_mends )
DEFINE_EVENT_FIELDS( buries_corpse )
DEFINE_EVENT_FIELDS( character_gains_effect )
DEFINE_EVENT_FIELDS( character_heals_damage )
DEFINE_EVENT_FIELDS( character_kills_character )
DEFINE_EVENT_FIELDS( character_kills_monster )
DEFINE_EVENT_FIELDS( character_loses_effect )
DEFINE_EVENT_FIELDS( character_takes_damage )
DEFINE_EVENT_FIELDS( character_triggers_trap )
DEFINE_EVENT_FIELDS( consumes_marloss_item )
DEFINE_EVENT_FIELDS( crosses_mutation_threshold )
DEFINE_EVENT_FIELDS( dies_from_drug_overdose )
DEFINE_EVENT_FIELDS( evolves_mutation )
DEFINE_EVENT_FIELDS( fails_to_install_cbm )
DEFINE_EVENT_FIELDS( fails_to_remove_cbm )
DEFINE_EVENT_FIELDS( fuel_tank_explodes )
DEFINE_EVENT_FIELDS( gains_addiction )
DEFINE_EVENT_FIELDS( gains_mutation )
DEFINE_EVENT_FIELDS( gains_skill_level )
DEFINE_EVENT_FIELDS( game_over )
DEFINE_EVENT_FIELDS( installs_cbm )
DEFINE_EVENT_FIELDS( installs_faulty_cbm )
DEFINE_EVENT_FIELDS( launches_nuke )
DEFINE_EVENT_FIELDS( learns_martial_art )
DEFINE_EVENT_FIELDS( loses_addiction )
DEFINE_EVENT_FIELDS( npc_becomes_hostile )
DEFINE_EVENT_FIELDS( removes_cbm )
DEFINE_EVENT_FIELDS( telefrags_creature )
DEFINE_EVENT_FIELDS( teleports_into_wall )

} // namespace event_detail

template<event_type Type>
static void get_fields_if_match( event_type type, std::map<std::string, cata_variant_type> &out )
{
    if( Type == type ) {
        out = { event_detail::event_spec<Type>::fields.begin(),
                event_detail::event_spec<Type>::fields.end()
              };
    }
}

template<int... I>
static std::map<std::string, cata_variant_type>
get_fields_helper( event_type type, std::integer_sequence<int, I...> )
{
    std::map<std::string, cata_variant_type> result;
    bool discard[] = {
        ( get_fields_if_match<static_cast<event_type>( I )>( type, result ), true )...
    };
    ( void ) discard;
    return result;
}

std::map<std::string, cata_variant_type> event::get_fields( event_type type )
{
    return get_fields_helper(
               type, std::make_integer_sequence<int, static_cast<int>( event_type::num_event_types )> {} );
}

} // namespace cata
