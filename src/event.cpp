#include "event.h"

#include <string>

#include "debug.h"

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
        case event_type::avatar_enters_omt: return "avatar_enters_omt";
        case event_type::avatar_moves: return "avatar_moves";
        case event_type::avatar_dies: return "avatar_dies";
        case event_type::awakes_dark_wyrms: return "awakes_dark_wyrms";
        case event_type::becomes_wanted: return "becomes_wanted";
        case event_type::broken_bone: return "broken_bone";
        case event_type::broken_bone_mends: return "broken_bone_mends";
        case event_type::buries_corpse: return "buries_corpse";
        case event_type::causes_resonance_cascade: return "causes_resonance_cascade";
        case event_type::character_consumes_item: return "character_consumes_item";
        case event_type::character_dies: return "character_dies";
        case event_type::character_eats_item: return "character_eats_item";
        case event_type::character_casts_spell: return "character_casts_spell";
        case event_type::character_finished_activity: return "character_finished_activity";
        case event_type::character_forgets_spell: return "character_forgets_spell";
        case event_type::character_gains_effect: return "character_gains_effect";
        case event_type::character_gets_headshot: return "character_gets_headshot";
        case event_type::character_heals_damage: return "character_heals_damage";
        case event_type::character_kills_character: return "character_kills_character";
        case event_type::character_kills_monster: return "character_kills_monster";
        case event_type::character_learns_spell: return "character_learns_spell";
        case event_type::character_loses_effect: return "character_loses_effect";
        case event_type::character_melee_attacks_character:
                                                 return "character_melee_attacks_character";
        case event_type::character_melee_attacks_monster:
                                                 return "character_melee_attacks_monster";
        case event_type::character_ranged_attacks_character:
                                                 return "character_ranged_attacks_character";
        case event_type::character_ranged_attacks_monster:
                                                 return "character_ranged_attacks_monster";
        case event_type::character_smashes_tile: return "character_smashes_tile";
        case event_type::character_starts_activity: return "character_starts_activity";
        case event_type::character_takes_damage: return "character_takes_damage";
        case event_type::character_triggers_trap: return "character_triggers_trap";
        case event_type::character_falls_asleep: return "character_falls_asleep";
        case event_type::character_attempt_to_fall_asleep: return "character_attempt_to_fall_asleep";
        case event_type::character_wakes_up: return "character_wakes_up";
        case event_type::character_wears_item: return "character_wears_item";
        case event_type::character_wields_item: return "character_wields_item";
        case event_type::consumes_marloss_item: return "consumes_marloss_item";
        case event_type::crosses_marloss_threshold: return "crosses_marloss_threshold";
        case event_type::crosses_mutation_threshold: return "crosses_mutation_threshold";
        case event_type::crosses_mycus_threshold: return "crosses_mycus_threshold";
        case event_type::cuts_tree: return "cuts_tree";
        case event_type::dermatik_eggs_hatch: return "dermatik_eggs_hatch";
        case event_type::dermatik_eggs_injected: return "dermatik_eggs_injected";
        case event_type::destroys_triffid_grove: return "destroys_triffid_grove";
        case event_type::dies_from_asthma_attack: return "dies_from_asthma_attack";
        case event_type::dies_from_drug_overdose: return "dies_from_drug_overdose";
        case event_type::dies_from_bleeding: return "dies_from_bleeding";
        case event_type::dies_from_hypovolemia: return "dies_from_hypovolemia";
        case event_type::dies_from_redcells_loss: return "dies_from_redcells_loss";
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
        case event_type::gains_proficiency: return "gains_proficiency";
        case event_type::gains_skill_level: return "gains_skill_level";
        case event_type::game_avatar_death: return "game_avatar_death";
        case event_type::game_avatar_new: return "game_avatar_new";
        case event_type::game_load: return "game_load";
        case event_type::game_begin: return "game_begin";
        case event_type::game_over: return "game_over";
        case event_type::game_save: return "game_save";
        case event_type::game_start: return "game_start";
        case event_type::installs_cbm: return "installs_cbm";
        case event_type::installs_faulty_cbm: return "installs_faulty_cbm";
        case event_type::learns_martial_art: return "learns_martial_art";
        case event_type::loses_addiction: return "loses_addiction";
        case event_type::loses_mutation: return "loses_mutation";
        case event_type::npc_becomes_hostile: return "npc_becomes_hostile";
        case event_type::opens_portal: return "opens_portal";
        case event_type::opens_spellbook: return "opens_spellbook";
        case event_type::opens_temple: return "opens_temple";
        case event_type::player_fails_conduct: return "player_fails_conduct";
        case event_type::player_gets_achievement: return "player_gets_achievement";
        case event_type::player_levels_spell: return "player_levels_spell";
        case event_type::reads_book: return "reads_book";
        case event_type::releases_subspace_specimens: return "releases_subspace_specimens";
        case event_type::removes_cbm: return "removes_cbm";
        case event_type::seals_hazardous_material_sarcophagus: return "seals_hazardous_material_sarcophagus";
        case event_type::spellcasting_finish: return "spellcasting_finish";
        case event_type::telefrags_creature: return "telefrags_creature";
        case event_type::teleglow_teleports: return "teleglow_teleports";
        case event_type::teleports_into_wall: return "teleports_into_wall";
        case event_type::terminates_subspace_specimens: return "terminates_subspace_specimens";
        case event_type::throws_up: return "throws_up";
        case event_type::triggers_alarm: return "triggers_alarm";
        case event_type::uses_debug_menu: return "uses_debug_menu";
        case event_type::u_var_changed: return "u_var_changed";
        case event_type::vehicle_moves: return "vehicle_moves";
        case event_type::character_butchered_corpse: return "character_butchered_corpse";
        // *INDENT-ON*
        case event_type::num_event_types:
            break;
    }
    cata_fatal( "Invalid event_type" );
}

} // namespace io

namespace cata
{

namespace event_detail
{

#define DEFINE_EVENT_HELPER_FIELDS(type) \
    constexpr std::array<std::pair<const char *, cata_variant_type>, \
    type::fields.size()> type::fields;

DEFINE_EVENT_HELPER_FIELDS( event_spec_empty )
DEFINE_EVENT_HELPER_FIELDS( event_spec_character )
DEFINE_EVENT_HELPER_FIELDS( event_spec_character_item )

static_assert( static_cast<int>( event_type::num_event_types ) == 102,
               "This static_assert is a reminder to add a definition below when you add a new "
               "event_type.  If your event_spec specialization inherits from another struct for "
               "its fields definition then you probably don't need a definition here." );

#define DEFINE_EVENT_FIELDS(type) \
    constexpr std::array<std::pair<const char *, cata_variant_type>, \
    event_spec<event_type::type>::fields.size()> \
    event_spec<event_type::type>::fields;

DEFINE_EVENT_FIELDS( activates_artifact )
DEFINE_EVENT_FIELDS( administers_mutagen )
DEFINE_EVENT_FIELDS( avatar_enters_omt )
DEFINE_EVENT_FIELDS( avatar_moves )
DEFINE_EVENT_FIELDS( broken_bone )
DEFINE_EVENT_FIELDS( broken_bone_mends )
DEFINE_EVENT_FIELDS( buries_corpse )
DEFINE_EVENT_FIELDS( character_finished_activity )
DEFINE_EVENT_FIELDS( character_forgets_spell )
DEFINE_EVENT_FIELDS( character_casts_spell )
DEFINE_EVENT_FIELDS( character_dies )
DEFINE_EVENT_FIELDS( character_gains_effect )
DEFINE_EVENT_FIELDS( character_heals_damage )
DEFINE_EVENT_FIELDS( character_kills_character )
DEFINE_EVENT_FIELDS( character_kills_monster )
DEFINE_EVENT_FIELDS( character_learns_spell )
DEFINE_EVENT_FIELDS( character_loses_effect )
DEFINE_EVENT_FIELDS( character_melee_attacks_character )
DEFINE_EVENT_FIELDS( character_melee_attacks_monster )
DEFINE_EVENT_FIELDS( character_ranged_attacks_character )
DEFINE_EVENT_FIELDS( character_ranged_attacks_monster )
DEFINE_EVENT_FIELDS( character_smashes_tile )
DEFINE_EVENT_FIELDS( character_starts_activity )
DEFINE_EVENT_FIELDS( character_takes_damage )
DEFINE_EVENT_FIELDS( character_triggers_trap )
DEFINE_EVENT_FIELDS( character_falls_asleep )
DEFINE_EVENT_FIELDS( character_attempt_to_fall_asleep )
DEFINE_EVENT_FIELDS( character_wakes_up )
DEFINE_EVENT_FIELDS( crosses_mutation_threshold )
DEFINE_EVENT_FIELDS( dies_from_drug_overdose )
DEFINE_EVENT_FIELDS( evolves_mutation )
DEFINE_EVENT_FIELDS( fails_to_install_cbm )
DEFINE_EVENT_FIELDS( fails_to_remove_cbm )
DEFINE_EVENT_FIELDS( fuel_tank_explodes )
DEFINE_EVENT_FIELDS( gains_addiction )
DEFINE_EVENT_FIELDS( gains_mutation )
DEFINE_EVENT_FIELDS( gains_proficiency )
DEFINE_EVENT_FIELDS( gains_skill_level )
DEFINE_EVENT_FIELDS( game_avatar_death )
DEFINE_EVENT_FIELDS( game_avatar_new )
DEFINE_EVENT_FIELDS( game_load )
DEFINE_EVENT_FIELDS( game_begin )
DEFINE_EVENT_FIELDS( game_over )
DEFINE_EVENT_FIELDS( game_save )
DEFINE_EVENT_FIELDS( game_start )
DEFINE_EVENT_FIELDS( installs_cbm )
DEFINE_EVENT_FIELDS( installs_faulty_cbm )
DEFINE_EVENT_FIELDS( learns_martial_art )
DEFINE_EVENT_FIELDS( loses_addiction )
DEFINE_EVENT_FIELDS( loses_mutation )
DEFINE_EVENT_FIELDS( npc_becomes_hostile )
DEFINE_EVENT_FIELDS( opens_spellbook )
DEFINE_EVENT_FIELDS( player_fails_conduct )
DEFINE_EVENT_FIELDS( player_gets_achievement )
DEFINE_EVENT_FIELDS( player_levels_spell )
DEFINE_EVENT_FIELDS( removes_cbm )
DEFINE_EVENT_FIELDS( spellcasting_finish )
DEFINE_EVENT_FIELDS( telefrags_creature )
DEFINE_EVENT_FIELDS( teleports_into_wall )
DEFINE_EVENT_FIELDS( uses_debug_menu )
DEFINE_EVENT_FIELDS( u_var_changed )
DEFINE_EVENT_FIELDS( vehicle_moves )
DEFINE_EVENT_FIELDS( character_butchered_corpse )

} // namespace event_detail

template<event_type Type>
static void get_fields_if_match( event_type type, event::fields_type &out )
{
    if( Type == type ) {
        out = { event_detail::event_spec<Type>::fields.begin(),
                event_detail::event_spec<Type>::fields.end()
              };
    }
}

template<int... I>
static event::fields_type
get_fields_helper( event_type type, std::integer_sequence<int, I...> )
{
    event::fields_type result;
    std::array<bool, sizeof...( I )> discard = {
        ( get_fields_if_match<static_cast<event_type>( I )>( type, result ), true )...
    };
    ( void ) discard;
    return result;
}

event::fields_type event::get_fields( event_type type )
{
    return get_fields_helper(
               type, std::make_integer_sequence<int, static_cast<int>( event_type::num_event_types )> {} );
}

cata_variant event::get_variant( const std::string &key ) const
{
    auto it = data_.find( key );
    if( it == data_.end() ) {
        cata_fatal( "No such key %s in event of type %s", key,
                    io::enum_to_string( type_ ) );
    }
    return it->second;
}

cata_variant event::get_variant_or_void( const std::string &key ) const
{
    auto it = data_.find( key );
    if( it == data_.end() ) {
        return cata_variant();
    }
    return it->second;
}

} // namespace cata
