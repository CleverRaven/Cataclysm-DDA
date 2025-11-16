#include <algorithm>
#include <iterator>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include "achievement.h"
#include "avatar.h"
#include "bodypart.h"
#include "cata_catch.h"
#include "cata_utility.h"
#include "character.h"
#include "character_id.h"
#include "coordinates.h"
#include "debug_menu.h"
#include "event.h"
#include "event_bus.h"
#include "filesystem.h"
#include "map_helpers.h"
#include "memorial_logger.h"
#include "monster.h"
#include "mutation.h"
#include "player_helpers.h"
#include "point.h"
#include "stats_tracker.h"
#include "type_id.h"

static const addiction_id addiction_alcohol( "alcohol" );

static const matype_id style_aikido( "style_aikido" );

static const mutation_category_id mutation_category_URSINE( "URSINE" );

static const skill_id skill_driving( "driving" );

static const spell_id spell_pain_damage( "pain_damage" );

static const trap_str_id tr_pit( "tr_pit" );

// Requires a custom sender for eg. using event types that need to be sent differently for EOCS to work
template<event_type Type>
void check_memorial_custom_sender(
    memorial_logger &m,
    const std::string &ref,
    std::function<void()> &&sender
)
{
    CAPTURE( io::enum_to_string( Type ) );
    CAPTURE( ref );
    m.clear();
    sender();

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

// Send the event via event_bus::send as a sane default
template<event_type Type, typename... Args>
void check_memorial(
    memorial_logger &m,
    const std::string &ref,
    Args... args
)
{
    check_memorial_custom_sender<Type>(
        m,
        ref,
        [&] { get_event_bus().send( cata::event::make<Type>( args... ) ); }
    );
}

TEST_CASE( "memorials", "[memorial]" )
{
    memorial_logger &m = get_memorial();
    m.clear();
    clear_avatar();
    get_stats().clear();
    get_achievements().clear();

    avatar &player_character = get_avatar();
    player_character.male = false;
    character_id ch = player_character.getID();
    std::string u_name = player_character.name;
    character_id ch2 = character_id( ch.get_value() + 1 );
    mutagen_technique mutagen = mutagen_technique::injected_purifier;
    mtype_id mon( "mon_zombie_kevlar_2" );
    tripoint_bub_ms loc = get_avatar().pos_bub() + tripoint::east;
    monster &dummy_monster = spawn_test_monster( "mon_dragon_dummy", loc );
    efftype_id eff( "onfire" );
    itype_id it( "marloss_seed" );
    trait_id mut( "CARNIVORE" );
    trait_id mut2( "SAPROPHAGE" );
    trait_id mut3( "NONE" );
    bionic_id cbm( "bio_alarm" );
    proficiency_id prof( "prof_wound_care" );

    check_memorial<event_type::activates_artifact>(
        m, "Activated the art_name.", ch, "art_name" );

    check_memorial<event_type::activates_mininuke>(
        m, "Activated a mininuke.", ch );

    check_memorial<event_type::administers_mutagen>(
        m,  "Injected purifier.", ch, mutagen );

    check_memorial<event_type::angers_amigara_horrors>(
        m,  "Angered a group of amigara horrors!" );

    check_memorial<event_type::awakes_dark_wyrms>(
        m,  "Awoke a group of dark wyrms!" );

    check_memorial<event_type::becomes_wanted>(
        m,  "Became wanted by the police!", ch );

    check_memorial<event_type::broken_bone>(
        m,  "Broke her right arm.", ch, bodypart_id( "arm_r" ) );

    check_memorial<event_type::broken_bone_mends>(
        m,  "Broken right arm began to mend.", ch, bodypart_id( "arm_r" ) );

    check_memorial<event_type::buries_corpse>(
        m,  "You buried monster_name.", ch, mon, "monster_name" );

    check_memorial<event_type::causes_resonance_cascade>(
        m,  "Caused a resonance cascade." );

    check_memorial<event_type::character_gains_effect>(
        m,  "Caught on fire.", ch, bodypart_id( "arm_r" ), eff, 1 );

    check_memorial<event_type::character_kills_character>(
        m,  "Killed an innocent person, victim_name, in cold blood and felt terrible "
        "afterwards.", ch, ch2, "victim_name" );

    check_memorial_custom_sender<event_type::character_kills_monster>(
        m,  "Killed a Kevlar hulk.",
    [&] {
        const cata::event e = cata::event::make<event_type::character_kills_monster>( ch, mon, 0 );
        get_event_bus().send_with_talker( player_character.as_character(), dummy_monster.as_monster(), e );
    } );

    check_memorial<event_type::character_loses_effect>(
        m,  "Put out the fire.", ch, bodypart_id( "arm_r" ), eff );

    check_memorial<event_type::character_triggers_trap>(
        m,  "Fell in a pit.", ch, tr_pit );

    check_memorial<event_type::consumes_marloss_item>(
        m,  "Consumed a Marloss seed.", ch, it );

    check_memorial<event_type::crosses_marloss_threshold>(
        m,  "Opened the Marloss Gateway.", ch );

    check_memorial<event_type::crosses_mutation_threshold>(
        m,  "Became one with the bears.", ch, mutation_category_URSINE );

    check_memorial<event_type::crosses_mycus_threshold>(
        m,  "Became one with the Mycus.", ch );

    check_memorial<event_type::dermatik_eggs_hatch>(
        m,  "Dermatik eggs hatched.", ch );

    check_memorial<event_type::dermatik_eggs_injected>(
        m,  "Injected with dermatik eggs.", ch );

    check_memorial<event_type::destroys_triffid_grove>(
        m,  "Destroyed a triffid grove." );

    check_memorial<event_type::dies_from_asthma_attack>(
        m,  "Succumbed to an asthma attack.", ch );

    check_memorial<event_type::dies_from_drug_overdose>(
        m,  "Died of a drug overdose.", ch, eff );

    check_memorial<event_type::dies_from_bleeding>(
        m,  "Bled to death.", ch );

    check_memorial<event_type::dies_from_hypovolemia>(
        m,  "Died of hypovolemic shock.", ch );

    check_memorial<event_type::dies_from_redcells_loss>(
        m,  "Died from loss of red blood cells.", ch );

    check_memorial<event_type::dies_of_infection>(
        m,  "Succumbed to the infection.", ch );

    check_memorial<event_type::dies_of_starvation>(
        m,  "Died of starvation.", ch );

    check_memorial<event_type::dies_of_thirst>(
        m,  "Died of thirst.", ch );

    check_memorial<event_type::digs_into_lava>(
        m,  "Dug a shaft into lava." );

    check_memorial<event_type::disarms_nuke>(
        m,  "Disarmed a nuclear missile." );

    check_memorial<event_type::eats_sewage>(
        m,  "Ate a sewage sample." );

    check_memorial<event_type::evolves_mutation>(
        m,  "'Carnivore' mutation turned into 'Saprophage'", ch, mut, mut2 );

    check_memorial<event_type::exhumes_grave>(
        m,  "Exhumed a grave.", ch );

    check_memorial<event_type::fails_to_install_cbm>(
        m,  "Failed install of bionic: Alarm System.", ch, cbm );

    check_memorial<event_type::fails_to_remove_cbm>(
        m,  "Failed to remove bionic: Alarm System.", ch, cbm );

    check_memorial<event_type::falls_asleep_from_exhaustion>(
        m,  "Succumbed to lack of sleep.", ch );

    check_memorial<event_type::fuel_tank_explodes>(
        m,  "The fuel tank of the vehicle_name exploded!", "vehicle_name" );

    check_memorial<event_type::gains_addiction>(
        m,  "Became addicted to alcohol.", ch, addiction_alcohol );

    check_memorial<event_type::gains_mutation>(
        m,  "Gained the mutation 'Carnivore'.", ch, mut );

    check_memorial<event_type::gains_proficiency>(
        m,  "Gained the proficiency 'Wound Care'.", ch, prof );

    check_memorial<event_type::gains_skill_level>(
        m,  "Reached skill level 8 in vehicles.", ch, skill_driving, 8 );

    check_memorial<event_type::game_avatar_death>(
        m,  u_name + " was killed.\nLast words: last_words", ch, u_name, false, "last_words" );

    check_memorial<event_type::game_avatar_new>(
        m,  u_name + " began their journey into the Cataclysm.", true, false, ch, u_name,
        player_character.custom_profession );

    check_memorial<event_type::installs_cbm>(
        m,  "Installed bionic: Alarm System.", ch, cbm );

    check_memorial<event_type::installs_faulty_cbm>(
        m,  "Installed bad bionic: Alarm System.", ch, cbm );

    check_memorial<event_type::learns_martial_art>(
        m,  "Learned Aikido.", ch, style_aikido );

    check_memorial<event_type::loses_addiction>(
        m,  "Overcame addiction to alcohol.", ch, addiction_alcohol );

    check_memorial<event_type::npc_becomes_hostile>(
        m,  "npc_name became hostile.", ch2, "npc_name" );

    check_memorial<event_type::opens_portal>(
        m,  "Opened a portal." );

    check_memorial<event_type::opens_temple>(
        m,  "Opened a strange temple." );

    check_memorial<event_type::character_forgets_spell>(
        m,  "Forgot the spell Pain.", ch, spell_pain_damage );

    check_memorial<event_type::character_learns_spell>(
        m,  "Learned the spell Pain.", ch, spell_pain_damage );

    check_memorial<event_type::player_levels_spell>(
        m,  "Gained a spell level on Pain.", ch, spell_pain_damage, 5, mut3 );

    check_memorial<event_type::releases_subspace_specimens>(
        m,  "Released subspace specimens." );

    check_memorial<event_type::removes_cbm>(
        m,  "Removed bionic: Alarm System.", ch, cbm );

    check_memorial<event_type::seals_hazardous_material_sarcophagus>(
        m,  "Sealed a Hazardous Material Sarcophagus." );

    check_memorial<event_type::telefrags_creature>(
        m,  "Telefragged a telefragged_name.", ch, "telefragged_name" );

    check_memorial<event_type::teleglow_teleports>(
        m,  "Spontaneous teleport.", ch );

    check_memorial<event_type::teleports_into_wall>(
        m,  "Teleported into a obstacle_name.", ch, "obstacle_name" );

    check_memorial<event_type::terminates_subspace_specimens>(
        m,  "Terminated subspace specimens." );

    check_memorial<event_type::throws_up>(
        m,  "Threw up.", ch );

    check_memorial<event_type::triggers_alarm>(
        m,  "Set off an alarm.", ch );

    check_memorial<event_type::uses_debug_menu>(
        m,  "Used the debug menu (WISH).", debug_menu::debug_menu_index::WISH );
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
        "| Year 1, Mar 21 4:20:14 AM | forest | Used the debug menu (ENABLE_ACHIEVEMENTS)."
        + eol;

    memorial_logger logger;
    std::istringstream is( json_value );
    logger.load( is );
    CHECK( logger.dump() == expected_output );
}
