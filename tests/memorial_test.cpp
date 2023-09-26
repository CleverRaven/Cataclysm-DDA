#include <algorithm>
#include <chrono>
#include <iterator>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include "achievement.h"
#include "avatar.h"
#include "bodypart.h"
#include "cata_catch.h"
#include "character_id.h"
#include "debug_menu.h"
#include "event.h"
#include "event_bus.h"
#include "filesystem.h"
#include "make_static.h"
#include "memorial_logger.h"
#include "mutation.h"
#include "npc.h"
#include "output.h"
#include "player_helpers.h"
#include "profession.h"
#include "stats_tracker.h"
#include "type_id.h"

static const matype_id style_aikido( "style_aikido" );

static const mutation_category_id mutation_category_URSINE( "URSINE" );

static const skill_id skill_driving( "driving" );

static const spell_id spell_pain_damage( "pain_damage" );

static const trap_str_id tr_pit( "tr_pit" );

template<event_type Type, typename... Args>
void check_memorial( memorial_logger &m, event_bus &b, const std::string &ref, Args... args )
{
    CAPTURE( io::enum_to_string( Type ) );
    CAPTURE( ref );
    m.clear();
    b.send( cata::event::make<Type>( args... ) );

    std::string result = m.dump();
    CAPTURE( result );
    std::vector<std::string> result_lines = string_split( result, '\n' );
    REQUIRE( !result_lines.empty() );
    CHECK( result_lines.back().empty() );
    result_lines.pop_back();

    // Remove expected results by matching them against the strings we encounter.
    std::vector<std::string> ref_lines = string_split( ref, '\n' );
    for( const std::string &result_string : result_lines ) {
        std::string message = string_split( result_string, '|' ).back();
        if( !message.empty() && message.front() == ' ' ) {
            message.erase( message.begin() );
        }
        // Remove trailing carriage return, if any
        while( !message.empty() && *message.rbegin() == '\r' ) {
            message.erase( message.end() - 1 );
        }
        ref_lines.erase( std::remove( ref_lines.begin(), ref_lines.end(), message ),
                         ref_lines.end() );
    }
    std::string unmatched_results;
    INFO( std::accumulate( begin( ref_lines ), end( ref_lines ), unmatched_results ) );
    CHECK( ref_lines.empty() );
}

TEST_CASE( "memorials", "[memorial]" )
{
    memorial_logger &m = get_memorial();
    m.clear();
    clear_avatar();
    get_stats().clear();
    get_achievements().clear();

    event_bus &b = get_event_bus();

    avatar &player_character = get_avatar();
    player_character.male = false;
    character_id ch = player_character.getID();
    std::string u_name = player_character.name;
    character_id ch2 = character_id( ch.get_value() + 1 );
    mutagen_technique mutagen = mutagen_technique::injected_purifier;
    mtype_id mon( "mon_zombie_kevlar_2" );
    efftype_id eff( "onfire" );
    itype_id it( "marloss_seed" );
    trait_id mut( "CARNIVORE" );
    trait_id mut2( "SAPROPHAGE" );
    bionic_id cbm( "bio_alarm" );

    check_memorial<event_type::activates_artifact>(
        m, b, "Activated the art_name.", ch, "art_name" );

    check_memorial<event_type::activates_mininuke>(
        m, b, "Activated a mininuke.", ch );

    check_memorial<event_type::administers_mutagen>(
        m, b, "Injected purifier.", ch, mutagen );

    check_memorial<event_type::angers_amigara_horrors>(
        m, b, "Angered a group of amigara horrors!" );

    check_memorial<event_type::awakes_dark_wyrms>(
        m, b, "Awoke a group of dark wyrms!" );

    check_memorial<event_type::becomes_wanted>(
        m, b, "Became wanted by the police!", ch );

    check_memorial<event_type::broken_bone>(
        m, b, "Broke her right arm.", ch, bodypart_id( "arm_r" ) );

    check_memorial<event_type::broken_bone_mends>(
        m, b, "Broken right arm began to mend.", ch, bodypart_id( "arm_r" ) );

    check_memorial<event_type::buries_corpse>(
        m, b, "You buried monster_name.", ch, mon, "monster_name" );

    check_memorial<event_type::causes_resonance_cascade>(
        m, b, "Caused a resonance cascade." );

    check_memorial<event_type::character_gains_effect>(
        m, b, "Caught on fire.", ch, bodypart_id( "arm_r" ), eff );

    check_memorial<event_type::character_kills_character>(
        m, b, "Killed an innocent person, victim_name, in cold blood and felt terrible "
        "afterwards.", ch, ch2, "victim_name" );

    check_memorial<event_type::character_kills_monster>(
        m, b, "Killed a Kevlar hulk.", ch, mon );

    check_memorial<event_type::character_loses_effect>(
        m, b, "Put out the fire.", ch, eff );

    check_memorial<event_type::character_triggers_trap>(
        m, b, "Fell in a pit.", ch, tr_pit );

    check_memorial<event_type::consumes_marloss_item>(
        m, b, "Consumed a Marloss seed.", ch, it );

    check_memorial<event_type::crosses_marloss_threshold>(
        m, b, "Opened the Marloss Gateway.", ch );

    check_memorial<event_type::crosses_mutation_threshold>(
        m, b, "Became one with the bears.", ch, mutation_category_URSINE );

    check_memorial<event_type::crosses_mycus_threshold>(
        m, b, "Became one with the Mycus.", ch );

    check_memorial<event_type::dermatik_eggs_hatch>(
        m, b, "Dermatik eggs hatched.", ch );

    check_memorial<event_type::dermatik_eggs_injected>(
        m, b, "Injected with dermatik eggs.", ch );

    check_memorial<event_type::destroys_triffid_grove>(
        m, b, "Destroyed a triffid grove." );

    check_memorial<event_type::dies_from_asthma_attack>(
        m, b, "Succumbed to an asthma attack.", ch );

    check_memorial<event_type::dies_from_drug_overdose>(
        m, b, "Died of a drug overdose.", ch, eff );

    check_memorial<event_type::dies_from_bleeding>(
        m, b, "Bled to death.", ch );

    check_memorial<event_type::dies_from_hypovolemia>(
        m, b, "Died of hypovolemic shock.", ch );

    check_memorial<event_type::dies_from_redcells_loss>(
        m, b, "Died from loss of red blood cells.", ch );

    check_memorial<event_type::dies_of_infection>(
        m, b, "Succumbed to the infection.", ch );

    check_memorial<event_type::dies_of_starvation>(
        m, b, "Died of starvation.", ch );

    check_memorial<event_type::dies_of_thirst>(
        m, b, "Died of thirst.", ch );

    check_memorial<event_type::digs_into_lava>(
        m, b, "Dug a shaft into lava." );

    check_memorial<event_type::disarms_nuke>(
        m, b, "Disarmed a nuclear missile." );

    check_memorial<event_type::eats_sewage>(
        m, b, "Ate a sewage sample." );

    check_memorial<event_type::evolves_mutation>(
        m, b, "'Carnivore' mutation turned into 'Saprophage'", ch, mut, mut2 );

    check_memorial<event_type::exhumes_grave>(
        m, b, "Exhumed a grave.", ch );

    check_memorial<event_type::fails_to_install_cbm>(
        m, b, "Failed install of bionic: Alarm System.", ch, cbm );

    check_memorial<event_type::fails_to_remove_cbm>(
        m, b, "Failed to remove bionic: Alarm System.", ch, cbm );

    check_memorial<event_type::falls_asleep_from_exhaustion>(
        m, b, "Succumbed to lack of sleep.", ch );

    check_memorial<event_type::fuel_tank_explodes>(
        m, b, "The fuel tank of the vehicle_name exploded!", "vehicle_name" );

    check_memorial<event_type::gains_addiction>(
        m, b, "Became addicted to alcohol.", ch, STATIC( addiction_id( "alcohol" ) ) );

    check_memorial<event_type::gains_mutation>(
        m, b, "Gained the mutation 'Carnivore'.", ch, mut );

    check_memorial<event_type::gains_skill_level>(
        m, b, "Reached skill level 8 in vehicles.", ch, skill_driving, 8 );

    check_memorial<event_type::game_avatar_death>(
        m, b, u_name + " was killed.\nLast words: last_words", ch, u_name, player_character.male, false,
        "last_words" );

    check_memorial<event_type::game_avatar_new>(
        m, b, u_name + " began their journey into the Cataclysm.", true, false, ch, u_name,
        player_character.male, player_character.prof->ident(), player_character.custom_profession );

    check_memorial<event_type::installs_cbm>(
        m, b, "Installed bionic: Alarm System.", ch, cbm );

    check_memorial<event_type::installs_faulty_cbm>(
        m, b, "Installed bad bionic: Alarm System.", ch, cbm );

    check_memorial<event_type::learns_martial_art>(
        m, b, "Learned Aikido.", ch, style_aikido );

    check_memorial<event_type::loses_addiction>(
        m, b, "Overcame addiction to alcohol.", ch, STATIC( addiction_id( "alcohol" ) ) );

    check_memorial<event_type::npc_becomes_hostile>(
        m, b, "npc_name became hostile.", ch2, "npc_name" );

    check_memorial<event_type::opens_portal>(
        m, b, "Opened a portal." );

    check_memorial<event_type::opens_temple>(
        m, b, "Opened a strange temple." );

    check_memorial<event_type::character_forgets_spell>(
        m, b, "Forgot the spell Pain.", ch, spell_pain_damage );

    check_memorial<event_type::character_learns_spell>(
        m, b, "Learned the spell Pain.", ch, spell_pain_damage );

    check_memorial<event_type::player_levels_spell>(
        m, b, "Gained a spell level on Pain.", ch, spell_pain_damage, 5 );

    check_memorial<event_type::releases_subspace_specimens>(
        m, b, "Released subspace specimens." );

    check_memorial<event_type::removes_cbm>(
        m, b, "Removed bionic: Alarm System.", ch, cbm );

    check_memorial<event_type::seals_hazardous_material_sarcophagus>(
        m, b, "Sealed a Hazardous Material Sarcophagus." );

    check_memorial<event_type::telefrags_creature>(
        m, b, "Telefragged a telefragged_name.", ch, "telefragged_name" );

    check_memorial<event_type::teleglow_teleports>(
        m, b, "Spontaneous teleport.", ch );

    check_memorial<event_type::teleports_into_wall>(
        m, b, "Teleported into a obstacle_name.", ch, "obstacle_name" );

    check_memorial<event_type::terminates_subspace_specimens>(
        m, b, "Terminated subspace specimens." );

    check_memorial<event_type::throws_up>(
        m, b, "Threw up.", ch );

    check_memorial<event_type::triggers_alarm>(
        m, b, "Set off an alarm.", ch );

    check_memorial<event_type::uses_debug_menu>(
        m, b, "Used the debug menu (WISH).", debug_menu::debug_menu_index::WISH );
}

TEST_CASE( "convert_legacy_memorial_log", "[memorial]" )
{
    std::string eol = cata_files::eol();

    // Verify that the old format can be transformed into the new format
    const std::string input =
        "| Year 1, Spring, day 0 0800.00 | prison | "
        "Hubert 'Daffy' Mullin began their journey into the Cataclysm." + eol +
        "| Year 1, Spring, day 0 0800.05 | prison | Gained the mutation 'Debug Invincibility'." +
        eol;
    const std::string json_value =
        R"([{"preformatted":"| Year 1, Spring, day 0 0800.00 | prison | Hubert 'Daffy' Mullin began their journey into the Cataclysm."},{"preformatted":"| Year 1, Spring, day 0 0800.05 | prison | Gained the mutation 'Debug Invincibility'."}])";

    memorial_logger logger;
    {
        std::istringstream is( input );
        logger.load( is );
        std::ostringstream os;
        logger.save( os );
        CHECK( os.str() == json_value );
    }

    // Then verify that the new format is unchanged
    {
        std::istringstream is( json_value );
        logger.load( is );
        std::ostringstream os;
        logger.save( os );
        CHECK( os.str() == json_value );
    }

    // Finally, verify that dump matches legacy input
    CHECK( logger.dump() == input );
}

TEST_CASE( "memorial_log_dumping", "[memorial]" )
{
    std::string eol = cata_files::eol();

    // An example log file with one legacy and one "modern" entry
    const std::string json_value =
        R"([{"preformatted":"| Year 1, Spring, day 0 0800.00 | refugee center | Apolonia Trout began their journey into the Cataclysm."},{"time":15614,"oter_id":"cabin_isherwood","oter_name":"forest","message":"Used the debug menu (ENABLE_ACHIEVEMENTS)."}])";
    const std::string expected_output =
        "| Year 1, Spring, day 0 0800.00 | refugee center | "
        "Apolonia Trout began their journey into the Cataclysm." + eol +
        "| Year 1, Spring, day 1 4:20:14 AM | forest | Used the debug menu (ENABLE_ACHIEVEMENTS)."
        + eol;

    memorial_logger logger;
    std::istringstream is( json_value );
    logger.load( is );
    CHECK( logger.dump() == expected_output );
}
