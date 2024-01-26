#include "iuse.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "action.h"
#include "activity_actor_definitions.h"
#include "activity_type.h"
#include "avatar.h"
#include "avatar_action.h"
#include "bionics.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "construction.h"
#include "character_martial_arts.h"
#include "city.h"
#include "color.h"
#include "coordinate_conversions.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "cuboid_rectangle.h"
#include "damage.h"
#include "debug.h"
#include "effect.h" // for weed_msg
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "explosion.h"
#include "field.h"
#include "field_type.h"
#include "flag.h"
#include "fungal_effects.h"
#include "game.h"
#include "game_constants.h"
#include "game_inventory.h"
#include "handle_liquid.h"
#include "harvest.h"
#include "iexamine.h"
#include "input_context.h"
#include "inventory.h"
#include "inventory_ui.h"
#include "item.h"
#include "item_location.h"
#include "item_pocket.h"
#include "iteminfo_query.h"
#include "itype.h"
#include "json.h"
#include "json_loader.h"
#include "line.h"
#include "make_static.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "martialarts.h"
#include "memorial_logger.h"
#include "memory_fast.h"
#include "messages.h"
#include "mongroup.h"
#include "monster.h"
#include "morale_types.h"
#include "mtype.h"
#include "music.h"
#include "mutation.h"
#include "npc.h"
#include "omdata.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "pimpl.h"
#include "player_activity.h"
#include "point.h"
#include "popup.h" // For play_game
#include "recipe.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "ret_val.h"
#include "rng.h"
#include "sounds.h"
#include "speech.h"
#include "stomach.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "teleport.h"
#include "text_snippets.h"
#include "translations.h"
#include "trap.h"
#include "try_parse_integer.h"
#include "type_id.h"
#include "ui.h"
#include "ui_manager.h"
#include "units_utility.h"
#include "value_ptr.h"
#include "veh_interact.h"
#include "vehicle.h"
#include "viewer.h"
#include "vitamin.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "units.h"
#include "weather.h"
#include "weather_gen.h"
#include "weather_type.h"

static const activity_id ACT_FISH( "ACT_FISH" );
static const activity_id ACT_GAME( "ACT_GAME" );
static const activity_id ACT_GENERIC_GAME( "ACT_GENERIC_GAME" );
static const activity_id ACT_HAND_CRANK( "ACT_HAND_CRANK" );
static const activity_id ACT_HEATING( "ACT_HEATING" );
static const activity_id ACT_JACKHAMMER( "ACT_JACKHAMMER" );
static const activity_id ACT_PICKAXE( "ACT_PICKAXE" );
static const activity_id ACT_ROBOT_CONTROL( "ACT_ROBOT_CONTROL" );
static const activity_id ACT_VIBE( "ACT_VIBE" );

static const addiction_id addiction_marloss_b( "marloss_b" );
static const addiction_id addiction_marloss_r( "marloss_r" );
static const addiction_id addiction_marloss_y( "marloss_y" );
static const addiction_id addiction_nicotine( "nicotine" );

static const ammotype ammo_battery( "battery" );

static const bionic_id bio_shock( "bio_shock" );
static const bionic_id bio_tools( "bio_tools" );

static const construction_str_id construction_constr_clear_rubble( "constr_clear_rubble" );
static const construction_str_id construction_constr_fill_pit( "constr_fill_pit" );
static const construction_str_id construction_constr_pit( "constr_pit" );
static const construction_str_id construction_constr_pit_shallow( "constr_pit_shallow" );
static const construction_str_id construction_constr_water_channel( "constr_water_channel" );

static const efftype_id effect_adrenaline( "adrenaline" );
static const efftype_id effect_antibiotic( "antibiotic" );
static const efftype_id effect_antibiotic_visible( "antibiotic_visible" );
static const efftype_id effect_antifungal( "antifungal" );
static const efftype_id effect_asthma( "asthma" );
static const efftype_id effect_beartrap( "beartrap" );
static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_blind( "blind" );
static const efftype_id effect_blood_spiders( "blood_spiders" );
static const efftype_id effect_bloodworms( "bloodworms" );
static const efftype_id effect_boomered( "boomered" );
static const efftype_id effect_bouldering( "bouldering" );
static const efftype_id effect_brainworms( "brainworms" );
static const efftype_id effect_cig( "cig" );
static const efftype_id effect_conjunctivitis_bacterial( "conjunctivitis_bacterial" );
static const efftype_id effect_conjunctivitis_viral( "conjunctivitis_viral" );
static const efftype_id effect_contacts( "contacts" );
static const efftype_id effect_corroding( "corroding" );
static const efftype_id effect_critter_well_fed( "critter_well_fed" );
static const efftype_id effect_crushed( "crushed" );
static const efftype_id effect_datura( "datura" );
static const efftype_id effect_dazed( "dazed" );
static const efftype_id effect_dermatik( "dermatik" );
static const efftype_id effect_docile( "docile" );
static const efftype_id effect_downed( "downed" );
static const efftype_id effect_drunk( "drunk" );
static const efftype_id effect_earphones( "earphones" );
static const efftype_id effect_flushot( "flushot" );
static const efftype_id effect_foodpoison( "foodpoison" );
static const efftype_id effect_formication( "formication" );
static const efftype_id effect_fungus( "fungus" );
static const efftype_id effect_glowing( "glowing" );
static const efftype_id effect_glowy_led( "glowy_led" );
static const efftype_id effect_hallu( "hallu" );
static const efftype_id effect_happy( "happy" );
static const efftype_id effect_harnessed( "harnessed" );
static const efftype_id effect_has_bag( "has_bag" );
static const efftype_id effect_haslight( "haslight" );
static const efftype_id effect_high( "high" );
static const efftype_id effect_in_pit( "in_pit" );
static const efftype_id effect_infected( "infected" );
static const efftype_id effect_jetinjector( "jetinjector" );
static const efftype_id effect_lack_sleep( "lack_sleep" );
static const efftype_id effect_laserlocked( "laserlocked" );
static const efftype_id effect_lying_down( "lying_down" );
static const efftype_id effect_melatonin( "melatonin" );
static const efftype_id effect_meth( "meth" );
static const efftype_id effect_monster_armor( "monster_armor" );
static const efftype_id effect_monster_saddled( "monster_saddled" );
static const efftype_id effect_music( "music" );
static const efftype_id effect_nausea( "nausea" );
static const efftype_id effect_onfire( "onfire" );
static const efftype_id effect_paincysts( "paincysts" );
static const efftype_id effect_pet( "pet" );
static const efftype_id effect_poison( "poison" );
static const efftype_id effect_pre_conjunctivitis_bacterial( "pre_conjunctivitis_bacterial" );
static const efftype_id effect_ridden( "ridden" );
static const efftype_id effect_riding( "riding" );
static const efftype_id effect_run( "run" );
static const efftype_id effect_sad( "sad" );
static const efftype_id effect_sap( "sap" );
static const efftype_id effect_shakes( "shakes" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_slimed( "slimed" );
static const efftype_id effect_smoke_lungs( "smoke_lungs" );
static const efftype_id effect_spores( "spores" );
static const efftype_id effect_stimpack( "stimpack" );
static const efftype_id effect_strong_antibiotic( "strong_antibiotic" );
static const efftype_id effect_strong_antibiotic_visible( "strong_antibiotic_visible" );
static const efftype_id effect_stunned( "stunned" );
static const efftype_id effect_tapeworm( "tapeworm" );
static const efftype_id effect_teargas( "teargas" );
static const efftype_id effect_tetanus( "tetanus" );
static const efftype_id effect_tied( "tied" );
static const efftype_id effect_took_antiasthmatic( "took_antiasthmatic" );
static const efftype_id effect_took_anticonvulsant_visible( "took_anticonvulsant_visible" );
static const efftype_id effect_took_flumed( "took_flumed" );
static const efftype_id effect_took_prozac( "took_prozac" );
static const efftype_id effect_took_prozac_bad( "took_prozac_bad" );
static const efftype_id effect_took_prozac_visible( "took_prozac_visible" );
static const efftype_id effect_took_thorazine( "took_thorazine" );
static const efftype_id effect_took_thorazine_bad( "took_thorazine_bad" );
static const efftype_id effect_took_thorazine_visible( "took_thorazine_visible" );
static const efftype_id effect_took_xanax( "took_xanax" );
static const efftype_id effect_took_xanax_visible( "took_xanax_visible" );
static const efftype_id effect_valium( "valium" );
static const efftype_id effect_visuals( "visuals" );
static const efftype_id effect_weak_antibiotic( "weak_antibiotic" );
static const efftype_id effect_weak_antibiotic_visible( "weak_antibiotic_visible" );
static const efftype_id effect_webbed( "webbed" );
static const efftype_id effect_weed_high( "weed_high" );

static const furn_str_id furn_f_translocator_buoy( "f_translocator_buoy" );

static const harvest_drop_type_id harvest_drop_blood( "blood" );

static const itype_id itype_advanced_ecig( "advanced_ecig" );
static const itype_id itype_afs_atomic_smartphone( "afs_atomic_smartphone" );
static const itype_id itype_afs_atomic_smartphone_music( "afs_atomic_smartphone_music" );
static const itype_id itype_afs_atomic_wraitheon_music( "afs_atomic_wraitheon_music" );
static const itype_id itype_afs_wraitheon_smartphone( "afs_wraitheon_smartphone" );
static const itype_id itype_apparatus( "apparatus" );
static const itype_id itype_arcade_machine( "arcade_machine" );
static const itype_id itype_atomic_coffeepot( "atomic_coffeepot" );
static const itype_id itype_barometer( "barometer" );
static const itype_id itype_battery( "battery" );
static const itype_id itype_c4armed( "c4armed" );
static const itype_id itype_canister_empty( "canister_empty" );
static const itype_id itype_cig( "cig" );
static const itype_id itype_cigar( "cigar" );
static const itype_id itype_cow_bell( "cow_bell" );
static const itype_id itype_detergent( "detergent" );
static const itype_id itype_ecig( "ecig" );
static const itype_id itype_fire( "fire" );
static const itype_id itype_firecracker_act( "firecracker_act" );
static const itype_id itype_firecracker_pack_act( "firecracker_pack_act" );
static const itype_id itype_geiger_on( "geiger_on" );
static const itype_id itype_handrolled_cig( "handrolled_cig" );
static const itype_id itype_heatpack_used( "heatpack_used" );
static const itype_id itype_hygrometer( "hygrometer" );
static const itype_id itype_joint( "joint" );
static const itype_id itype_liquid_soap( "liquid_soap" );
static const itype_id itype_log( "log" );
static const itype_id itype_mask_h20survivor_on( "mask_h20survivor_on" );
static const itype_id itype_memory_card( "memory_card" );
static const itype_id itype_mininuke_act( "mininuke_act" );
static const itype_id itype_molotov( "molotov" );
static const itype_id itype_mp3( "mp3" );
static const itype_id itype_mp3_on( "mp3_on" );
static const itype_id itype_multi_cooker( "multi_cooker" );
static const itype_id itype_multi_cooker_filled( "multi_cooker_filled" );
static const itype_id itype_nicotine_liquid( "nicotine_liquid" );
static const itype_id itype_paper( "paper" );
static const itype_id itype_radio_car( "radio_car" );
static const itype_id itype_radio_car_on( "radio_car_on" );
static const itype_id itype_radio_on( "radio_on" );
static const itype_id itype_rebreather_on( "rebreather_on" );
static const itype_id itype_rebreather_xl_on( "rebreather_xl_on" );
static const itype_id itype_shocktonfa_off( "shocktonfa_off" );
static const itype_id itype_shocktonfa_on( "shocktonfa_on" );
static const itype_id itype_smart_phone( "smart_phone" );
static const itype_id itype_smartphone_music( "smartphone_music" );
static const itype_id itype_soap( "soap" );
static const itype_id itype_soldering_iron( "soldering_iron" );
static const itype_id itype_spiral_stone( "spiral_stone" );
static const itype_id itype_towel( "towel" );
static const itype_id itype_towel_wet( "towel_wet" );
static const itype_id itype_water( "water" );
static const itype_id itype_water_clean( "water_clean" );
static const itype_id itype_wax( "wax" );
static const itype_id itype_weather_reader( "weather_reader" );

static const json_character_flag json_flag_ENHANCED_VISION( "ENHANCED_VISION" );
static const json_character_flag json_flag_HYPEROPIC( "HYPEROPIC" );
static const json_character_flag json_flag_MYOPIC( "MYOPIC" );
static const json_character_flag json_flag_MYOPIC_IN_LIGHT( "MYOPIC_IN_LIGHT" );
static const json_character_flag json_flag_PAIN_IMMUNE( "PAIN_IMMUNE" );

static const mongroup_id GROUP_FISH( "GROUP_FISH" );

static const mtype_id mon_blob( "mon_blob" );
static const mtype_id mon_dog_thing( "mon_dog_thing" );
static const mtype_id mon_hallu_multicooker( "mon_hallu_multicooker" );
static const mtype_id mon_hologram( "mon_hologram" );
static const mtype_id mon_spore( "mon_spore" );
static const mtype_id mon_vortex( "mon_vortex" );

static const mutation_category_id mutation_category_CATTLE( "CATTLE" );
static const mutation_category_id mutation_category_MYCUS( "MYCUS" );

static const proficiency_id proficiency_prof_lockpicking( "prof_lockpicking" );
static const proficiency_id proficiency_prof_lockpicking_expert( "prof_lockpicking_expert" );

static const quality_id qual_AXE( "AXE" );
static const quality_id qual_GLARE( "GLARE" );
static const quality_id qual_LOCKPICK( "LOCKPICK" );
static const quality_id qual_PRY( "PRY" );
static const quality_id qual_SCREW_FINE( "SCREW_FINE" );

static const skill_id skill_computer( "computer" );
static const skill_id skill_cooking( "cooking" );
static const skill_id skill_electronics( "electronics" );
static const skill_id skill_fabrication( "fabrication" );
static const skill_id skill_firstaid( "firstaid" );
static const skill_id skill_mechanics( "mechanics" );
static const skill_id skill_survival( "survival" );
static const skill_id skill_traps( "traps" );

static const species_id species_FUNGUS( "FUNGUS" );
static const species_id species_HALLUCINATION( "HALLUCINATION" );
static const species_id species_INSECT( "INSECT" );
static const species_id species_ROBOT( "ROBOT" );

static const ter_str_id ter_t_grave( "t_grave" );
static const ter_str_id ter_t_grave_new( "t_grave_new" );
static const ter_str_id ter_t_pit( "t_pit" );
static const ter_str_id ter_t_pit_corpsed( "t_pit_corpsed" );
static const ter_str_id ter_t_pit_covered( "t_pit_covered" );
static const ter_str_id ter_t_pit_glass( "t_pit_glass" );
static const ter_str_id ter_t_pit_shallow( "t_pit_shallow" );
static const ter_str_id ter_t_pit_spiked( "t_pit_spiked" );
static const ter_str_id ter_t_pit_spiked_covered( "t_pit_spiked_covered" );
static const ter_str_id ter_t_utility_light( "t_utility_light" );

static const trait_id trait_ACIDBLOOD( "ACIDBLOOD" );
static const trait_id trait_ACIDPROOF( "ACIDPROOF" );
static const trait_id trait_ALCMET( "ALCMET" );
static const trait_id trait_CHLOROMORPH( "CHLOROMORPH" );
static const trait_id trait_EATDEAD( "EATDEAD" );
static const trait_id trait_EATPOISON( "EATPOISON" );
static const trait_id trait_GILLS( "GILLS" );
static const trait_id trait_ILLITERATE( "ILLITERATE" );
static const trait_id trait_LIGHTWEIGHT( "LIGHTWEIGHT" );
static const trait_id trait_MARLOSS( "MARLOSS" );
static const trait_id trait_MARLOSS_AVOID( "MARLOSS_AVOID" );
static const trait_id trait_MARLOSS_BLUE( "MARLOSS_BLUE" );
static const trait_id trait_MARLOSS_YELLOW( "MARLOSS_YELLOW" );
static const trait_id trait_M_DEPENDENT( "M_DEPENDENT" );
static const trait_id trait_PSYCHOPATH( "PSYCHOPATH" );
static const trait_id trait_PYROMANIA( "PYROMANIA" );
static const trait_id trait_SPIRITUAL( "SPIRITUAL" );
static const trait_id trait_THRESH_MARLOSS( "THRESH_MARLOSS" );
static const trait_id trait_THRESH_MYCUS( "THRESH_MYCUS" );
static const trait_id trait_THRESH_PLANT( "THRESH_PLANT" );
static const trait_id trait_TOLERANCE( "TOLERANCE" );
static const trait_id trait_VAMPIRE( "VAMPIRE" );
static const trait_id trait_WAYFARER( "WAYFARER" );

static const vitamin_id vitamin_blood( "blood" );
static const vitamin_id vitamin_human_blood_vitamin( "human_blood_vitamin" );
static const vitamin_id vitamin_redcells( "redcells" );

static const weather_type_id weather_portal_storm( "portal_storm" );

// how many characters per turn of radio
static constexpr int RADIO_PER_TURN = 25;

#include "iuse_software.h"

struct object_names_collection;

static void item_save_monsters( Character &p, item &it, const std::vector<monster *> &monster_vec,
                                int photo_quality );
static bool show_photo_selection( Character &p, item &it, const std::string &var_name );

static std::string format_object_pair( const std::pair<std::string, int> &pair,
                                       const std::string &article );
static std::string format_object_pair_article( const std::pair<std::string, int> &pair );
static std::string format_object_pair_no_article( const std::pair<std::string, int> &pair );

static std::string colorized_field_description_at( const tripoint &point );
static std::string colorized_trap_name_at( const tripoint &point );
static std::string colorized_ter_name_flags_at( const tripoint &point,
        const std::vector<std::string> &flags = {}, const std::vector<ter_str_id> &ter_whitelist = {} );
static std::string colorized_feature_description_at( const tripoint &center_point, bool &item_found,
        const units::volume &min_visible_volume );

static std::string colorized_item_name( const item &item );
static std::string colorized_item_description( const item &item );
static item get_top_item_at_point( const tripoint &point,
                                   const units::volume &min_visible_volume );

static std::string effects_description_for_creature( Creature *creature, std::string &pose,
        const std::string &pronoun_gender );

static object_names_collection enumerate_objects_around_point( const tripoint &point,
        int radius, const tripoint &bounds_center_point, int bounds_radius,
        const tripoint &camera_pos, const units::volume &min_visible_volume, bool create_figure_desc,
        std::unordered_set<tripoint> &ignored_points,
        std::unordered_set<const vehicle *> &vehicles_recorded );
static item::extended_photo_def photo_def_for_camera_point( const tripoint &aim_point,
        const tripoint &camera_pos,
        std::vector<monster *> &monster_vec, std::vector<Character *> &character_vec );

static const std::vector<std::string> camera_ter_whitelist_flags = {
    "HIDE_PLACE", "FUNGUS", "TREE", "PERMEABLE", "SHRUB",
    "PLACE_ITEM", "GROWTH_HARVEST", "GROWTH_MATURE", "GOES_UP",
    "GOES_DOWN", "RAMP", "SHARP", "SIGN", "CLIMBABLE"
};
static const std::vector<ter_str_id> camera_ter_whitelist_types = {
    ter_t_pit_covered, ter_t_grave_new, ter_t_grave, ter_t_pit,
    ter_t_pit_shallow, ter_t_pit_corpsed, ter_t_pit_spiked,
    ter_t_pit_spiked_covered, ter_t_pit_glass, ter_t_pit_glass, ter_t_utility_light
};

void remove_radio_mod( item &it, Character &p )
{
    if( !it.has_flag( flag_RADIO_MOD ) ) {
        return;
    }
    p.add_msg_if_player( _( "You remove the radio modification from your %s." ), it.tname() );
    item mod( "radio_mod" );
    p.i_add_or_drop( mod, 1 );
    it.unset_flag( flag_RADIO_ACTIVATION );
    it.unset_flag( flag_RADIO_MOD );
    it.unset_flag( flag_RADIOSIGNAL_1 );
    it.unset_flag( flag_RADIOSIGNAL_2 );
    it.unset_flag( flag_RADIOSIGNAL_3 );
    it.unset_flag( flag_RADIOCARITEM );
}

// Checks that the player can smoke
std::optional<std::string> iuse::can_smoke( const Character &you )
{
    auto cigs = you.cache_get_items_with( flag_LITCIG, []( const item & it ) {
        return it.active;
    } );

    if( !cigs.empty() ) {
        return string_format( _( "You're already smoking a %s!" ), cigs[0]->tname() );
    }

    if( !you.has_charges( itype_fire, 1 ) ) {
        return _( "You don't have anything to light it with!" );
    }
    return std::nullopt;
}

std::optional<int> iuse::sewage( Character *p, item *, const tripoint & )
{
    if( !p->query_yn( _( "Are you sure you want to drink… this?" ) ) ) {
        return std::nullopt;
    }

    get_event_bus().send<event_type::eats_sewage>();
    p->vomit();
    return 1;
}

std::optional<int> iuse::honeycomb( Character *p, item *, const tripoint & )
{
    get_map().spawn_item( p->pos(), itype_wax, 2 );
    return 1;
}

std::optional<int> iuse::xanax( Character *p, item *it, const tripoint & )
{
    p->add_msg_if_player( _( "You take some %s." ), it->tname() );
    p->add_effect( effect_took_xanax, 90_minutes );
    p->add_effect( effect_took_xanax_visible, rng( 70_minutes, 110_minutes ) );
    return 1;
}

static constexpr time_duration alc_strength( const int strength, const time_duration &weak,
        const time_duration &medium, const time_duration &strong )
{
    return strength == 0 ? weak : strength == 1 ? medium : strong;
}

static int alcohol( Character &p, const item &it, const int strength )
{
    // Weaker characters are cheap drunks
    /** @EFFECT_STR_MAX reduces drunkenness duration */
    time_duration duration = alc_strength( strength, 22_minutes, 34_minutes,
                                           45_minutes ) - ( alc_strength( strength, 36_seconds, 1_minutes, 72_seconds ) * p.str_max );
    if( p.has_trait( trait_ALCMET ) ) {
        duration = alc_strength( strength, 6_minutes, 14_minutes, 18_minutes ) - ( alc_strength( strength,
                   36_seconds, 1_minutes, 1_minutes ) * p.str_max );
        // Metabolizing the booze improves the nutritional value;
        // might not be healthy, and still causes Thirst problems, though
        p.stomach.mod_nutr( -std::abs( it.get_comestible() ? it.type->comestible->stim : 0 ) );
        // Metabolizing it cancels out the depressant
        p.mod_stim( std::abs( it.get_comestible() ? it.get_comestible()->stim : 0 ) );
    } else if( p.has_trait( trait_TOLERANCE ) ) {
        duration -= alc_strength( strength, 9_minutes, 16_minutes, 24_minutes );
    } else if( p.has_trait( trait_LIGHTWEIGHT ) ) {
        duration += alc_strength( strength, 9_minutes, 16_minutes, 24_minutes );
    }
    p.add_effect( effect_drunk, duration );
    return 1;
}

std::optional<int> iuse::alcohol_weak( Character *p, item *it, const tripoint & )
{
    return alcohol( *p, *it, 0 );
}

std::optional<int> iuse::alcohol_medium( Character *p, item *it, const tripoint & )
{
    return alcohol( *p, *it, 1 );
}

std::optional<int> iuse::alcohol_strong( Character *p, item *it, const tripoint & )
{
    return alcohol( *p, *it, 2 );
}

/**
 * Entry point for intentional bodily intake of smoke via paper wrapped one
 * time use items: cigars, cigarettes, etc.
 *
 * @param p Player doing the smoking
 * @param it the item to be smoked.
 * @return Charges used in item smoked
 */
std::optional<int> iuse::smoking( Character *p, item *it, const tripoint & )
{
    std::optional<std::string> litcig = can_smoke( *p );
    if( litcig.has_value() ) {
        p->add_msg_if_player( m_info, _( litcig.value_or( "" ) ) );
        return std::nullopt;
    }

    item cig;
    if( it->typeId() == itype_cig || it->typeId() == itype_handrolled_cig ) {
        cig = item( "cig_lit", calendar::turn );
        cig.item_counter = to_turns<int>( 4_minutes );
        p->mod_hunger( -3 );
        p->mod_thirst( 2 );
    } else if( it->typeId() == itype_cigar ) {
        cig = item( "cigar_lit", calendar::turn );
        cig.item_counter = to_turns<int>( 12_minutes );
        p->mod_thirst( 3 );
        p->mod_hunger( -4 );
    } else if( it->typeId() == itype_joint ) {
        cig = item( "joint_lit", calendar::turn );
        cig.item_counter = to_turns<int>( 4_minutes );
        p->mod_hunger( 4 );
        p->mod_thirst( 6 );
        if( p->get_painkiller() < 5 ) {
            p->set_painkiller( ( p->get_painkiller() + 3 ) * 2 );
        }
    } else {
        p->add_msg_if_player( m_bad,
                              _( "Please let the devs know you should be able to smoke a %s, but the smoking code does not know how." ),
                              it->tname() );
        return std::nullopt;
    }
    // If we're here, we better have a cig to light.
    p->use_charges_if_avail( itype_fire, 1 );
    cig.active = true;
    p->inv->add_item( cig, false, true );
    p->add_msg_if_player( m_neutral, _( "You light a %s." ), cig.tname() );

    // Parting messages
    if( it->typeId() == itype_joint ) {
        // Would group with the joint, but awkward to mutter before lighting up.
        if( one_in( 5 ) ) {
            weed_msg( *p );
        }
    }
    if( p->get_effect_dur( effect_cig ) > 10_minutes * ( p->addiction_level(
                addiction_nicotine ) + 1 ) ) {
        p->add_msg_if_player( m_bad, _( "Ugh, too much smoke… you feel nasty." ) );
    }

    return 1;
}

std::optional<int> iuse::ecig( Character *p, item *it, const tripoint & )
{
    if( it->typeId() == itype_ecig ) {
        p->add_msg_if_player( m_neutral, _( "You take a puff from your electronic cigarette." ) );
    } else if( it->typeId() == itype_advanced_ecig ) {
        if( p->has_charges( itype_nicotine_liquid, 1 ) ) {
            p->add_msg_if_player( m_neutral,
                                  _( "You inhale some vapor from your advanced electronic cigarette." ) );
            p->use_charges( itype_nicotine_liquid, 1 );
            item dummy_ecig = item( "ecig", calendar::turn );
            p->consume_effects( dummy_ecig );
        } else {
            p->add_msg_if_player( m_info, _( "You don't have any nicotine liquid!" ) );
            return std::nullopt;
        }
    }

    p->mod_thirst( 1 );
    p->mod_hunger( -1 );
    p->add_effect( effect_cig, 10_minutes );
    if( p->get_effect_dur( effect_cig ) > 10_minutes * ( p->addiction_level(
                addiction_nicotine ) + 1 ) ) {
        p->add_msg_if_player( m_bad, _( "Ugh, too much nicotine… you feel nasty." ) );
    }
    return 1;
}

std::optional<int> iuse::antibiotic( Character *p, item *, const tripoint & )
{
    p->add_msg_player_or_npc( m_neutral,
                              _( "You take some antibiotics." ),
                              _( "<npcname> takes some antibiotics." ) );
    if( p->has_effect( effect_tetanus ) ) {
        if( one_in( 3 ) ) {
            p->remove_effect( effect_tetanus );
            p->add_msg_if_player( m_good, _( "The muscle spasms start to go away." ) );
        } else {
            p->add_msg_if_player( m_warning, _( "The medication does nothing to help the spasms." ) );
        }
    }
    if( p->has_effect( effect_conjunctivitis_bacterial ) ) {
        if( one_in( 2 ) ) {
            p->remove_effect( effect_conjunctivitis_bacterial );
            p->add_msg_if_player( m_good, _( "Your pinkeye seems to be clearing up." ) );
        } else {
            p->add_msg_if_player( m_warning, _( "Your pinkeye doesn't feel any better." ) );
        }
    }
    if( p->has_effect( effect_pre_conjunctivitis_bacterial ) ) {
        if( one_in( 2 ) ) {
            //There were no symptoms yet, so we skip telling the player they feel better.
            p->remove_effect( effect_pre_conjunctivitis_bacterial );
        }
    }
    if( p->has_effect( effect_conjunctivitis_viral ) ) {
        //Antibiotics don't kill viruses.
        p->add_msg_if_player( m_warning, _( "Your pinkeye doesn't feel any better." ) );
    }
    if( p->has_effect( effect_infected ) && !p->has_effect( effect_antibiotic ) ) {
        p->add_msg_if_player( m_good,
                              _( "Maybe this is just the placebo effect, but you feel a little better as the dose settles in." ) );
    }
    p->add_effect( effect_antibiotic, 12_hours );
    p->add_effect( effect_antibiotic_visible, rng( 9_hours, 15_hours ) );
    return 1;
}

std::optional<int> iuse::eyedrops( Character *p, item *it, const tripoint & )
{
    if( p->cant_do_underwater() ) {
        return std::nullopt;
    }
    if( !it->ammo_sufficient( p ) ) {
        p->add_msg_if_player( _( "You're out of %s." ), it->tname() );
        return std::nullopt;
    }
    p->add_msg_if_player( _( "You use your %s." ), it->tname() );
    p->moves -= to_moves<int>( 10_seconds );
    if( p->has_effect( effect_boomered ) ) {
        p->remove_effect( effect_boomered );
        p->add_msg_if_player( m_good, _( "You wash the slime from your eyes." ) );
    }
    return 1;
}

std::optional<int> iuse::fungicide( Character *p, item *, const tripoint & )
{
    if( p->cant_do_underwater() ) {
        return std::nullopt;
    }

    const bool has_fungus = p->has_effect( effect_fungus );
    const bool has_spores = p->has_effect( effect_spores );

    if( p->is_npc() && !has_fungus && !has_spores ) {
        return std::nullopt;
    }

    p->add_msg_player_or_npc( _( "You use your fungicide." ), _( "<npcname> uses some fungicide." ) );
    if( has_fungus && one_in( 3 ) ) {
        // this is not a medicine, the effect is shorter
        p->add_effect( effect_antifungal, 1_hours );
        if( p->has_effect( effect_fungus ) ) {
            p->add_msg_if_player( m_warning,
                                  _( "You feel a burning sensation slowly radiating throughout your skin." ) );
        }
    }
    creature_tracker &creatures = get_creature_tracker();
    if( has_spores && one_in( 2 ) ) {
        if( !p->has_effect( effect_fungus ) ) {
            p->add_msg_if_player( m_warning, _( "Your skin grows warm for a moment." ) );
        }
        p->remove_effect( effect_spores );
        int spore_count = rng( 1, 6 );
        map &here = get_map();
        for( const tripoint &dest : here.points_in_radius( p->pos(), 1 ) ) {
            if( spore_count == 0 ) {
                break;
            }
            if( dest == p->pos() ) {
                continue;
            }
            if( here.passable( dest ) && x_in_y( spore_count, 8 ) ) {
                if( monster *const mon_ptr = creatures.creature_at<monster>( dest ) ) {
                    monster &critter = *mon_ptr;
                    if( !critter.type->in_species( species_FUNGUS ) ) {
                        add_msg_if_player_sees( dest, m_warning, _( "The %s is covered in tiny spores!" ),
                                                critter.name() );
                    }
                    if( !critter.make_fungus() ) {
                        critter.die( p ); // counts as kill by player
                    }
                } else {
                    g->place_critter_at( mon_spore, dest );
                }
                spore_count--;
            }
        }
    }
    return 1;
}

std::optional<int> iuse::antifungal( Character *p, item *, const tripoint & )
{
    if( p->cant_do_underwater() ) {
        return std::nullopt;
    }
    p->add_effect( effect_antifungal, 4_hours );
    if( p->has_effect( effect_fungus ) ) {
        p->add_msg_if_player( m_warning,
                              _( "You feel a burning sensation slowly radiating throughout your skin." ) );
    }
    if( p->has_effect( effect_spores ) ) {
        if( !p->has_effect( effect_fungus ) ) {
            p->add_msg_if_player( m_warning, _( "Your skin grows warm for a moment." ) );
        }
    }
    return 1;
}

std::optional<int> iuse::antiparasitic( Character *p, item *, const tripoint & )
{
    if( p->cant_do_underwater() ) {
        return std::nullopt;
    }
    p->add_msg_if_player( _( "You take some antiparasitic medication." ) );
    if( p->has_effect( effect_dermatik ) ) {
        p->remove_effect( effect_dermatik );
        p->add_msg_if_player( m_good, _( "The itching sensation under your skin fades away." ) );
    }
    if( p->has_effect( effect_tapeworm ) ) {
        p->remove_effect( effect_tapeworm );
        p->guts.mod_nutr( -1 ); // You just digested the tapeworm.
        if( p->has_flag( json_flag_PAIN_IMMUNE ) ) {
            p->add_msg_if_player( m_good, _( "Your bowels clench as something inside them dies." ) );
        } else {
            p->add_msg_if_player( m_mixed, _( "Your bowels spasm painfully as something inside them dies." ) );
            p->mod_pain( rng( 8, 24 ) );
        }
    }
    if( p->has_effect( effect_bloodworms ) ) {
        p->remove_effect( effect_bloodworms );
        p->add_msg_if_player( _( "Your skin prickles and your veins itch for a few moments." ) );
    }
    if( p->has_effect( effect_blood_spiders ) ) {
        p->remove_effect( effect_blood_spiders );
        p->add_msg_if_player( _( "Your veins relax in a soothing wave through your body." ) );
    }
    if( p->has_effect( effect_brainworms ) ) {
        p->remove_effect( effect_brainworms );
        if( p->has_flag( json_flag_PAIN_IMMUNE ) ) {
            p->add_msg_if_player( m_good, _( "The pressure inside your head feels better already." ) );
        } else {
            p->add_msg_if_player( m_mixed,
                                  _( "Your head pounds like a sore tooth as something inside of it dies." ) );
            p->mod_pain( rng( 8, 24 ) );
        }
    }
    if( p->has_effect( effect_paincysts ) ) {
        p->remove_effect( effect_paincysts );
        if( p->has_flag( json_flag_PAIN_IMMUNE ) ) {
            p->add_msg_if_player( m_good, _( "The stiffness in your joints goes away." ) );
        } else {
            p->add_msg_if_player( m_good, _( "The pain in your joints goes away." ) );
        }
    }
    return 1;
}

std::optional<int> iuse::anticonvulsant( Character *p, item *, const tripoint & )
{
    p->add_msg_if_player( _( "You take some anticonvulsant medication." ) );
    /** @EFFECT_STR reduces duration of anticonvulsant medication */
    time_duration duration = 8_hours - p->str_cur * rng( 0_turns, 10_minutes );
    if( p->has_trait( trait_TOLERANCE ) ) {
        duration -= 1_hours;
    }
    if( p->has_trait( trait_LIGHTWEIGHT ) ) {
        duration += 2_hours;
    }
    p->add_effect( effect_valium, duration );
    p->add_effect( effect_took_anticonvulsant_visible, duration );
    p->add_effect( effect_high, duration );
    if( p->has_effect( effect_shakes ) ) {
        p->remove_effect( effect_shakes );
        p->add_msg_if_player( m_good, _( "You stop shaking." ) );
    }
    return 1;
}

std::optional<int> iuse::weed_cake( Character *p, item *, const tripoint & )
{
    p->add_msg_if_player(
        _( "You start scarfing down the delicious cake.  It tastes a little funny, though…" ) );
    time_duration duration = 12_minutes;
    if( p->has_trait( trait_TOLERANCE ) ) {
        duration = 9_minutes;
    }
    if( p->has_trait( trait_LIGHTWEIGHT ) ) {
        duration = 15_minutes;
    }
    p->mod_hunger( 2 );
    p->mod_thirst( 6 );
    if( p->get_painkiller() < 5 ) {
        p->set_painkiller( ( p->get_painkiller() + 3 ) * 2 );
    }
    p->add_effect( effect_weed_high, duration );
    p->moves -= 100;
    if( one_in( 5 ) ) {
        weed_msg( *p );
    }
    return 1;
}

std::optional<int> iuse::coke( Character *p, item *, const tripoint & )
{
    p->add_msg_if_player( _( "You snort a bump of coke." ) );
    /** @EFFECT_STR reduces duration of coke */
    time_duration duration = 20_minutes - 1_seconds * p->str_cur + rng( 0_minutes, 1_minutes );
    if( p->has_trait( trait_TOLERANCE ) ) {
        duration -= 1_minutes; // Symmetry would cause problems :-/
    }
    if( p->has_trait( trait_LIGHTWEIGHT ) ) {
        duration += 2_minutes;
    }
    p->mod_hunger( -8 );
    p->add_effect( effect_high, duration );
    return 1;
}

std::optional<int> iuse::meth( Character *p, item *, const tripoint & )
{
    /** @EFFECT_STR reduces duration of meth */
    time_duration duration = 1_minutes * ( 60 - p->str_cur );
    if( p->has_amount( itype_apparatus, 1 ) && p->use_charges_if_avail( itype_fire, 1 ) ) {
        p->add_msg_if_player( m_neutral, _( "You smoke your meth." ) );
        p->add_msg_if_player( m_good, _( "The world seems to sharpen." ) );
        p->mod_fatigue( -375 );
        if( p->has_trait( trait_TOLERANCE ) ) {
            duration *= 1.2;
        } else {
            duration *= ( p->has_trait( trait_LIGHTWEIGHT ) ? 1.8 : 1.5 );
        }
        map &here = get_map();
        // breathe out some smoke
        for( int i = 0; i < 3; i++ ) {
            point offset( rng( -2, 2 ), rng( -2, 2 ) );
            here.add_field( p->pos_bub() + offset, field_type_id( "fd_methsmoke" ), 2 );
        }
    } else {
        p->add_msg_if_player( _( "You snort some crystal meth." ) );
        p->mod_fatigue( -300 );
    }
    if( !p->has_effect( effect_meth ) ) {
        duration += 1_hours;
    }
    if( duration > 0_turns ) {
        // meth actually inhibits hunger, weaker characters benefit more
        /** @EFFECT_STR_MAX >4 experiences less hunger benefit from meth */
        int hungerpen = p->str_max < 5 ? 35 : 40 - ( 2 * p->str_max );
        if( hungerpen > 0 ) {
            p->mod_hunger( -hungerpen );
        }
        p->add_effect( effect_meth, duration );
    }
    return 1;
}

std::optional<int> iuse::flu_vaccine( Character *p, item *it, const tripoint & )
{
    p->add_msg_if_player( _( "You inject the vaccine." ) );
    time_point expiration_date = it->birthday() + 24_weeks;
    time_duration remaining_time = expiration_date - calendar::turn;
    // FIXME Removing feedback and visible status would be more realistic
    if( remaining_time > 0_turns ) {
        p->add_msg_if_player( m_good, _( "You no longer need to fear the flu, at least for some time." ) );
        p->add_effect( effect_flushot, remaining_time, false );
    } else {
        p->add_msg_if_player( m_bad,
                              _( "You notice the date on the packaging is pretty old.  It may no longer be effective." ) );
    }
    p->mod_pain( 3 );
    item syringe( "syringe", it->birthday() );
    p->i_add_or_drop( syringe );
    return 1;
}

std::optional<int> iuse::poison( Character *p, item *it, const tripoint & )
{
    if( p->has_trait( trait_EATDEAD ) ) {
        return 1;
    }

    // NPCs have a magical sense of what is inedible
    // Players can abuse the crafting menu instead...
    if( !it->has_flag( flag_HIDDEN_POISON ) &&
        ( p->is_npc() ||
          !p->query_yn( _( "Are you sure you want to eat this?  It looks poisonous…" ) ) ) ) {
        return std::nullopt;
    }
    /** @EFFECT_STR increases EATPOISON trait effectiveness (50-90%) */
    if( p->has_trait( trait_EATPOISON ) && ( !one_in( p->str_cur / 2 ) ) ) {
        return 1;
    }
    p->add_effect( effect_poison, 1_hours );
    p->add_effect( effect_foodpoison, 3_hours );
    return 1;
}

std::optional<int> iuse::meditate( Character *p, item *it, const tripoint & )
{
    if( !p ) {
        debugmsg( "%s called action meditate that requires character but no character is present",
                  it->typeId().str() );
        return std::nullopt;
    }
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    if( p->has_trait( trait_SPIRITUAL ) ) {
        p->assign_activity( meditate_activity_actor() );
    } else {
        p->add_msg_if_player( _( "This %s probably meant a lot to someone at one time." ),
                              it->tname() );
    }
    return 1;
}

std::optional<int> iuse::thorazine( Character *p, item *, const tripoint & )
{
    if( p->has_effect( effect_took_thorazine ) ) {
        p->remove_effect( effect_took_thorazine );
        p->mod_fatigue( 15 );
    }
    p->add_effect( effect_took_thorazine, 12_hours );
    p->mod_fatigue( 5 );
    p->remove_effect( effect_hallu );
    p->remove_effect( effect_visuals );
    p->remove_effect( effect_high );
    if( !p->has_effect( effect_dermatik ) ) {
        p->remove_effect( effect_formication );
    }
    if( one_in( 50 ) ) { // adverse reaction
        p->add_msg_if_player( m_bad, _( "You feel completely exhausted." ) );
        p->mod_fatigue( 15 );
        p->add_effect( effect_took_thorazine_bad, p->get_effect_dur( effect_took_thorazine ) );
    } else {
        p->add_msg_if_player( m_warning, _( "You feel a bit wobbly." ) );
    }
    p->add_effect( effect_took_thorazine_visible, rng( 9_hours, 15_hours ) );
    return 1;
}

std::optional<int> iuse::prozac( Character *p, item *, const tripoint & )
{
    if( !p->has_effect( effect_took_prozac ) ) {
        p->add_effect( effect_took_prozac, 12_hours );
    } else {
        p->mod_stim( 3 );
    }
    if( one_in( 50 ) ) { // adverse reaction, same duration as prozac effect.
        p->add_msg_if_player( m_warning, _( "You suddenly feel hollow inside." ) );
        p->add_effect( effect_took_prozac_bad, p->get_effect_dur( effect_took_prozac ) );
    }
    p->add_effect( effect_took_prozac_visible, rng( 9_hours, 15_hours ) );
    return 1;
}

std::optional<int> iuse::datura( Character *p, item *it, const tripoint & )
{
    if( p->is_npc() ) {
        return std::nullopt;
    }

    p->add_effect( effect_datura, rng( 3_hours, 13_hours ) );
    p->add_msg_if_player( _( "You eat the datura seed." ) );
    if( p->has_trait( trait_SPIRITUAL ) ) {
        p->add_morale( MORALE_FOOD_GOOD, 36, 72, 2_hours, 1_hours, false, it->type );
    }
    return 1;
}

std::optional<int> iuse::flumed( Character *p, item *it, const tripoint & )
{
    p->add_effect( effect_took_flumed, 10_hours );
    p->add_msg_if_player( _( "You take some %s." ), it->tname() );
    return 1;
}

std::optional<int> iuse::flusleep( Character *p, item *it, const tripoint & )
{
    p->add_effect( effect_took_flumed, 12_hours );
    p->mod_fatigue( 30 );
    p->add_msg_if_player( _( "You take some %s." ), it->tname() );
    p->add_msg_if_player( m_warning, _( "You feel very sleepy…" ) );
    return 1;
}

std::optional<int> iuse::inhaler( Character *p, item *, const tripoint & )
{
    p->add_msg_player_or_npc( m_neutral, _( "You take a puff from your inhaler." ),
                              _( "<npcname> takes a puff from their inhaler." ) );
    if( !p->remove_effect( effect_asthma ) ) {
        p->mod_fatigue( -3 ); // if we don't have asthma can be used as stimulant
        if( one_in( 20 ) ) {   // with a small but significant risk of adverse reaction
            p->add_effect( effect_shakes, rng( 2_minutes, 5_minutes ) );
        }
    }
    p->add_effect( effect_took_antiasthmatic, rng( 6_hours, 12_hours ) );
    p->remove_effect( effect_smoke_lungs );
    return 1;
}

std::optional<int> iuse::oxygen_bottle( Character *p, item *it, const tripoint & )
{
    p->moves -= to_moves<int>( 10_seconds );
    p->add_msg_player_or_npc( m_neutral, string_format( _( "You breathe deeply from the %s." ),
                              it->tname() ),
                              string_format( _( "<npcname> breathes from the %s." ),
                                      it->tname() ) );
    if( p->has_effect( effect_smoke_lungs ) ) {
        p->remove_effect( effect_smoke_lungs );
    } else if( p->has_effect( effect_teargas ) ) {
        p->remove_effect( effect_teargas );
    } else if( p->has_effect( effect_asthma ) ) {
        p->remove_effect( effect_asthma );
    } else if( p->get_stim() < 16 ) {
        p->mod_stim( 8 );
        p->mod_painkiller( 2 );
    }
    p->mod_painkiller( 2 );
    return 1;
}

std::optional<int> iuse::blech( Character *p, item *it, const tripoint & )
{
    // TODO: Add more effects?
    if( it->made_of( phase_id::LIQUID ) ) {
        if( !p->query_yn( _( "This looks unhealthy, sure you want to drink it?" ) ) ) {
            return std::nullopt;
        }
    } else { //Assume that if a blech consumable isn't a drink, it will be eaten.
        if( !p->query_yn( _( "This looks unhealthy, sure you want to eat it?" ) ) ) {
            return std::nullopt;
        }
    }

    if( it->has_flag( flag_ACID ) && ( p->has_trait( trait_ACIDPROOF ) ||
                                       p->has_trait( trait_ACIDBLOOD ) ) ) {
        p->add_msg_if_player( m_bad, _( "Blech, that tastes gross!" ) );
        //reverse the harmful values of drinking this acid.
        double multiplier = -1;
        p->stomach.mod_nutr( -p->nutrition_for( *it ) * multiplier );
        p->mod_thirst( -it->get_comestible()->quench * multiplier );
        p->stomach.mod_quench( 20 ); //acidproof people can drink acids like diluted water.
        p->mod_daily_health( it->get_comestible()->healthy * multiplier,
                             it->get_comestible()->healthy * multiplier );
        p->add_morale( MORALE_FOOD_BAD, it->get_comestible_fun() * multiplier, 60, 1_hours, 30_minutes,
                       false, it->type );
    } else if( it->has_flag( flag_ACID ) || it->has_flag( flag_CORROSIVE ) ) {
        p->add_msg_if_player( m_bad, _( "Blech, that burns your throat!" ) );
        p->mod_pain( rng( 32, 64 ) );
        p->add_effect( effect_poison, 1_hours );
        p->apply_damage( nullptr, bodypart_id( "torso" ), rng( 4, 12 ) );
        p->vomit();
    } else {
        p->add_msg_if_player( m_bad, _( "Blech, you don't feel you can stomach much of that." ) );
        p->add_effect( effect_nausea, 3_minutes );
    }
    return 1;
}

std::optional<int> iuse::blech_because_unclean( Character *p, item *it, const tripoint & )
{
    if( !p->is_npc() ) {
        if( it->made_of( phase_id::LIQUID ) ) {
            if( !p->query_yn( _( "This looks unclean; are you sure you want to drink it?" ) ) ) {
                return std::nullopt;
            }
        } else { //Assume that if a blech consumable isn't a drink, it will be eaten.
            if( !p->query_yn( _( "This looks unclean; are you sure you want to eat it?" ) ) ) {
                return std::nullopt;
            }
        }
    }
    return 1;
}

std::optional<int> iuse::plantblech( Character *p, item *it, const tripoint &pos )
{
    if( p->has_trait( trait_THRESH_PLANT ) ) {
        double multiplier = -1;
        if( p->has_trait( trait_CHLOROMORPH ) ) {
            multiplier = -3;
            p->add_msg_if_player( m_good, _( "The meal is revitalizing." ) );
        } else {
            p->add_msg_if_player( m_good, _( "Oddly enough, this doesn't taste so bad." ) );
        }

        //reverses the harmful values of drinking fertilizer
        p->stomach.mod_nutr( p->nutrition_for( *it ) * multiplier );
        p->mod_thirst( -it->get_comestible()->quench * multiplier );
        p->mod_daily_health( it->get_comestible()->healthy * multiplier,
                             it->get_comestible()->healthy * multiplier );
        p->add_morale( MORALE_FOOD_GOOD, -10 * multiplier, 60, 1_hours, 30_minutes, false, it->type );
        return 1;
    } else {
        return blech( p, it, pos );
    }
}

std::optional<int> iuse::chew( Character *p, item *it, const tripoint & )
{
    // TODO: Add more effects?
    p->add_msg_if_player( _( "You chew your %s." ), it->tname() );
    return 1;
}

// Helper to handle the logic of removing some random mutations.
static void do_purify( Character &p )
{
    std::vector<trait_id> valid; // Which flags the player has
    for( const mutation_branch &traits_iter : mutation_branch::get_all() ) {
        if( p.has_trait( traits_iter.id ) && !p.has_base_trait( traits_iter.id ) ) {
            //Looks for active mutation
            valid.push_back( traits_iter.id );
        }
    }
    if( valid.empty() ) {
        p.add_msg_if_player( _( "You feel cleansed." ) );
        return;
    }
    int num_cured = rng( 1, valid.size() );
    num_cured = std::min( 4, num_cured );
    for( int i = 0; i < num_cured && !valid.empty(); i++ ) {
        const trait_id id = random_entry_removed( valid );
        if( p.purifiable( id ) ) {
            p.remove_mutation( id );
        } else {
            p.add_msg_if_player( m_warning, _( "You feel a slight itching inside, but it passes." ) );
        }
    }
}

std::optional<int> iuse::purify_smart( Character *p, item *it, const tripoint & )
{
    std::vector<trait_id> valid; // Which flags the player has
    std::vector<std::string> valid_names; // Which flags the player has
    for( const mutation_branch &traits_iter : mutation_branch::get_all() ) {
        if( p->has_trait( traits_iter.id ) &&
            !p->has_base_trait( traits_iter.id ) &&
            p->purifiable( traits_iter.id ) ) {
            //Looks for active mutation
            valid.push_back( traits_iter.id );
            valid_names.push_back( p->mutation_name( traits_iter.id ) );
        }
    }
    if( valid.empty() ) {
        p->add_msg_if_player( _( "You don't have any mutations to purify." ) );
        return std::nullopt;
    }

    int mutation_index = uilist( _( "Choose a mutation to purify: " ), valid_names );
    // Because valid_names doesn't start with a space,
    // include one here to prettify the output
    if( mutation_index < 0 ) {
        return std::nullopt;
    }

    p->add_msg_if_player(
        _( "You inject the purifier.  The liquid thrashes inside the tube and goes down reluctantly." ) );

    p->remove_mutation( valid[mutation_index] );
    valid.erase( valid.begin() + mutation_index );

    // and one or two more untargeted purifications.
    if( !valid.empty() ) {
        p->remove_mutation( random_entry_removed( valid ) );
    }
    if( !valid.empty() && one_in( 2 ) ) {
        p->remove_mutation( random_entry_removed( valid ) );
    }

    p->mod_pain( 3 );

    item syringe( "syringe", it->birthday() );
    p->i_add( syringe );
    p->vitamins_mod( it->get_comestible()->default_nutrition.vitamins() );
    get_event_bus().send<event_type::administers_mutagen>( p->getID(),
            mutagen_technique::injected_smart_purifier );
    return 1;
}

static void spawn_spores( const Character &p )
{
    int spores_spawned = 0;
    map &here = get_map();
    fungal_effects fe;
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint &dest : closest_points_first( p.pos(), 4 ) ) {
        if( here.impassable( dest ) ) {
            continue;
        }
        float dist = rl_dist( dest, p.pos() );
        if( x_in_y( 1, dist ) ) {
            fe.marlossify( dest );
        }
        if( creatures.creature_at( dest ) != nullptr ) {
            continue;
        }
        if( one_in( 10 + 5 * dist ) && one_in( spores_spawned * 2 ) ) {
            if( monster *const spore = g->place_critter_at( mon_spore, dest ) ) {
                spore->friendly = -1;
                spores_spawned++;
            }
        }
    }
}

static void marloss_common( Character &p, item &it, const trait_id &current_color )
{
    static const std::map<trait_id, addiction_id> mycus_colors = {{
            { trait_MARLOSS_BLUE, addiction_marloss_b }, { trait_MARLOSS_YELLOW, addiction_marloss_y }, { trait_MARLOSS, addiction_marloss_r }
        }
    };

    if( p.has_trait( current_color ) || p.has_trait( trait_THRESH_MARLOSS ) ) {
        p.add_msg_if_player( m_good,
                             _( "As you eat the %s, you have a near-religious experience, feeling at one with your surroundings…" ),
                             it.tname() );
        p.add_morale( MORALE_MARLOSS, 100, 1000 );
        for( const std::pair<const trait_id, addiction_id> &pr : mycus_colors ) {
            if( pr.first != current_color ) {
                p.add_addiction( pr.second, 50 );
            }
        }

        p.set_hunger( -10 );
        spawn_spores( p );
        return;
    }

    int marloss_count = std::count_if( mycus_colors.begin(), mycus_colors.end(),
    [&p]( const std::pair<trait_id, addiction_id> &pr ) {
        return p.has_trait( pr.first );
    } );

    /* If we're not already carriers of current type of Marloss, roll for a random effect:
     * 1 - Mutate
     * 2 - Mutate
     * 3 - Mutate
     * 4 - Painkiller
     * 5 - Painkiller
     * 6 - Cleanse radiation + Painkiller
     * 7 - Fully satiate
     * 8 - Vomit
     * 9-12 - Give Marloss mutation
     */
    int effect = rng( 1, 12 );
    if( effect <= 3 ) {
        p.add_msg_if_player( _( "It tastes extremely strange!" ) );
        p.mutate();
        // Gruss dich, mutation drain, missed you!
        p.mod_pain( 2 * rng( 1, 5 ) );
        p.mod_stored_kcal( -87 );
        p.mod_thirst( 10 );
        p.mod_fatigue( 5 );
    } else if( effect <= 6 ) { // Radiation cleanse is below
        p.add_msg_if_player( m_good, _( "You feel better all over." ) );
        p.mod_painkiller( 30 );
        p.mod_pain( -40 );
        if( effect == 6 ) {
            p.set_rad( 0 );
        }
    } else if( effect == 7 ) {

        // previously used to set hunger to -10. with the new system, needs to do something
        // else that actually makes sense, so it is a little bit more involved.
        units::volume fulfill_vol = std::max( p.stomach.capacity( p ) / 8 - p.stomach.contains(), 0_ml );
        if( fulfill_vol != 0_ml ) {
            p.add_msg_if_player( m_good, _( "It is delicious, and very filling!" ) );
            int fulfill_cal = units::to_milliliter( fulfill_vol * 6 );
            p.stomach.mod_calories( fulfill_cal );
            p.stomach.mod_contents( fulfill_vol );
        } else {
            p.add_msg_if_player( m_bad, _( "It is delicious, but you can't eat any more." ) );
        }
    } else if( effect == 8 ) {
        p.add_msg_if_player( m_bad, _( "You take one bite, and immediately vomit!" ) );
        p.vomit();
    } else if( p.crossed_threshold() ) {
        // Mycus Rejection.  Goo already present fights off the fungus.
        p.add_msg_if_player( m_bad,
                             _( "You feel a familiar warmth, but suddenly it surges into an excruciating burn as you convulse, vomiting, and black out…" ) );
        if( p.is_avatar() ) {
            get_memorial().add(
                pgettext( "memorial_male", "Suffered Marloss Rejection." ),
                pgettext( "memorial_female", "Suffered Marloss Rejection." ) );
        }
        p.vomit();
        p.mod_pain( 90 );
        p.hurtall( rng( 40, 65 ), nullptr ); // No good way to say "lose half your current HP"
        /** @EFFECT_INT slightly reduces sleep duration when eating Mycus+goo */
        p.fall_asleep( 10_hours - p.int_cur *
                       1_minutes ); // Hope you were eating someplace safe.  Mycus v. goo in your guts is no joke.
        for( const std::pair<const trait_id, addiction_id> &pr : mycus_colors ) {
            p.unset_mutation( pr.first );
            p.rem_addiction( pr.second );
        }
        p.set_mutation(
            trait_MARLOSS_AVOID ); // And if you survive it's etched in your RNA, so you're unlikely to repeat the experiment.
    } else if( marloss_count >= 2 ) {
        p.add_msg_if_player( m_bad,
                             _( "You feel a familiar warmth, but suddenly it surges into painful burning as you convulse and collapse to the ground…" ) );
        /** @EFFECT_INT reduces sleep duration when eating wrong color Marloss */
        p.fall_asleep( 40_minutes - 1_minutes * p.int_cur / 2 );
        for( const std::pair<const trait_id, addiction_id> &pr : mycus_colors ) {
            p.unset_mutation( pr.first );
            p.rem_addiction( pr.second );
        }

        p.set_mutation( trait_THRESH_MARLOSS );
        get_map().ter_set( p.pos(), t_marloss );
        get_event_bus().send<event_type::crosses_marloss_threshold>( p.getID() );
        p.add_msg_if_player( m_good,
                             _( "You wake up in a Marloss bush.  Almost *cradled* in it, actually, as though it grew there for you." ) );
        p.add_msg_if_player( m_good,
                             //~ Beginning to hear the Mycus while conscious: that's it speaking
                             _( "unity.  together we have reached the door.  we provide the final key.  now to pass through…" ) );
    } else {
        p.add_msg_if_player( _( "You feel a strange warmth spreading throughout your body…" ) );
        p.set_mutation( current_color );
        // Give us addictions to the other two colors, but cure one for current color
        for( const std::pair<const trait_id, addiction_id> &pr : mycus_colors ) {
            if( pr.first == current_color ) {
                p.rem_addiction( pr.second );
            } else {
                p.add_addiction( pr.second, 60 );
            }
        }
    }
}

static bool marloss_prevented( const Character &p )
{
    if( p.is_npc() ) {
        return true;
    }
    if( p.has_trait( trait_MARLOSS_AVOID ) ) {
        p.add_msg_if_player( m_warning,
                             //~ "Nuh-uh" is a sound used for "nope", "no", etc.
                             _( "After what happened that last time?  Nuh-uh.  You're not eating that alien poison." ) );
        return true;
    }
    if( p.has_trait( trait_THRESH_MYCUS ) ) {
        p.add_msg_if_player( m_info,
                             _( "we no longer require this scaffolding.  we reserve it for other uses." ) );
        return true;
    }

    return false;
}

std::optional<int> iuse::marloss( Character *p, item *it, const tripoint & )
{
    if( marloss_prevented( *p ) ) {
        return std::nullopt;
    }

    get_event_bus().send<event_type::consumes_marloss_item>( p->getID(), it->typeId() );

    marloss_common( *p, *it, trait_MARLOSS );
    return 1;
}

std::optional<int> iuse::marloss_seed( Character *p, item *it, const tripoint & )
{
    if( !query_yn( _( "Are you sure you want to eat the %s?  You could plant it in a mound of dirt." ),
                   colorize( it->tname(), it->color_in_inventory() ) ) ) {
        return std::nullopt; // Save the seed for later!
    }

    if( marloss_prevented( *p ) ) {
        return std::nullopt;
    }

    get_event_bus().send<event_type::consumes_marloss_item>( p->getID(), it->typeId() );

    marloss_common( *p, *it, trait_MARLOSS_BLUE );
    return 1;
}

std::optional<int> iuse::marloss_gel( Character *p, item *it, const tripoint & )
{
    if( marloss_prevented( *p ) ) {
        return std::nullopt;
    }

    get_event_bus().send<event_type::consumes_marloss_item>( p->getID(), it->typeId() );

    marloss_common( *p, *it, trait_MARLOSS_YELLOW );
    return 1;
}

std::optional<int> iuse::mycus( Character *p, item *, const tripoint & )
{
    if( p->is_npc() ) {
        return 1;
    }
    // Welcome our guide.  Welcome.  To. The Mycus.

    // From an end-user perspective, dialogue should be presented uniformly:
    // initial caps, as in human writing, or all lowercase letters.
    // I think that all lowercase, because it contrasts with normal convention, reinforces the Mycus' alien nature

    if( p->has_trait( trait_THRESH_MARLOSS ) ) {
        get_event_bus().send<event_type::crosses_mycus_threshold>( p->getID() );
        p->add_msg_if_player( m_neutral,
                              _( "It tastes amazing, and you finish it quickly." ) );
        p->add_msg_if_player( m_good, _( "You feel better all over." ) );
        p->mod_painkiller( 30 );
        p->set_rad( 0 );
        p->healall( 4 ); // Can't make you a whole new person, but not for lack of trying
        p->add_msg_if_player( m_good,
                              _( "As it settles in, you feel ecstasy radiating through every part of your body…" ) );
        p->add_morale( MORALE_MARLOSS, 1000, 1000 ); // Last time you'll ever have it this good.  So enjoy.
        p->add_msg_if_player( m_good,
                              _( "Your eyes roll back in your head.  Everything dissolves into a blissful haze…" ) );
        /** @EFFECT_INT slightly reduces sleep duration when eating Mycus */
        p->fall_asleep( 5_hours - p->int_cur * 1_minutes );
        p->unset_mutation( trait_THRESH_MARLOSS );
        p->set_mutation( trait_THRESH_MYCUS );
        g->invalidate_main_ui_adaptor();
        //~ The Mycus does not use the term (or encourage the concept of) "you".  The PC is a local/native organism, but is now the Mycus.
        //~ It still understands the concept, but uninitelligent fungaloids and mind-bent symbiotes should not need it.
        //~ We are the Mycus.
        popup( _( "we welcome into us.  we have endured long in this forbidding world." ) );
        p->add_msg_if_player( " " );
        p->add_msg_if_player( m_good,
                              _( "A sea of white caps, waving gently.  A haze of spores wafting silently over a forest." ) );
        g->invalidate_main_ui_adaptor();
        popup( _( "the natives have a saying: \"e pluribus unum.\"  out of many, one." ) );
        p->add_msg_if_player( " " );
        p->add_msg_if_player( m_good,
                              _( "The blazing pink redness of the berry.  The juices spreading across our tongue, the warmth draping over us like a lover's embrace." ) );
        g->invalidate_main_ui_adaptor();
        popup( _( "we welcome the union of our lines in our local guide.  we will prosper, and unite this world.  even now, our fruits adapt to better serve local physiology." ) );
        p->add_msg_if_player( " " );
        p->add_msg_if_player( m_good,
                              _( "The sky-blue of the seed.  The nutty, creamy flavors intermingling with the berry, a memory that will never leave us." ) );
        g->invalidate_main_ui_adaptor();
        popup( _( "as, in time, shall we adapt to better welcome those who have not received us." ) );
        p->add_msg_if_player( " " );
        p->add_msg_if_player( m_good,
                              _( "The amber-yellow of the sap.  Feel it flowing through our veins, taking the place of the strange, thin red gruel called \"blood.\"" ) );
        g->invalidate_main_ui_adaptor();
        popup( _( "we are the Mycus." ) );
        /*p->add_msg_if_player( m_good,
                              _( "We welcome into us.  We have endured long in this forbidding world." ) );
        p->add_msg_if_player( m_good,
                              _( "The natives have a saying: \"E Pluribus Unum\"  Out of many, one." ) );
        p->add_msg_if_player( m_good,
                              _( "We welcome the union of our lines in our local guide.  We will prosper, and unite this world." ) );
        p->add_msg_if_player( m_good, _( "Even now, our fruits adapt to better serve local physiology." ) );
        p->add_msg_if_player( m_good,
                              _( "As, in time, shall we adapt to better welcome those who have not received us." ) );*/
        map &here = get_map();
        fungal_effects fe;
        for( const tripoint &nearby_pos : here.points_in_radius( p->pos(), 3 ) ) {
            if( here.move_cost( nearby_pos ) != 0 && !here.has_furn( nearby_pos ) &&
                !here.has_flag( ter_furn_flag::TFLAG_DEEP_WATER, nearby_pos ) &&
                !here.has_flag( ter_furn_flag::TFLAG_NO_FLOOR, nearby_pos ) ) {
                fe.marlossify( nearby_pos );
            }
        }
        p->rem_addiction( addiction_marloss_r );
        p->rem_addiction( addiction_marloss_b );
        p->rem_addiction( addiction_marloss_y );
    } else if( p->has_trait( trait_THRESH_MYCUS ) &&
               !p->has_trait( trait_M_DEPENDENT ) ) { // OK, now set the hook.
        if( !one_in( 3 ) ) {
            p->mutate_category( mutation_category_MYCUS, false, true );
            p->mod_stored_kcal( -87 );
            p->mod_thirst( 10 );
            p->mod_fatigue( 5 );
            p->add_morale( MORALE_MARLOSS, 25, 200 ); // still covers up mutation pain
        }
    } else if( p->has_trait( trait_THRESH_MYCUS ) ) {
        p->mod_painkiller( 5 );
        p->mod_stim( 5 );
    } else { // In case someone gets one without having been adapted first.
        // Marloss is the Mycus' method of co-opting humans.  Mycus fruit is for symbiotes' maintenance and development.
        p->add_msg_if_player(
            _( "This tastes really weird!  You're not sure it's good for you…" ) );
        p->mutate();
        p->mod_pain( 2 * rng( 1, 5 ) );
        p->mod_stored_kcal( -87 );
        p->mod_thirst( 10 );
        p->mod_fatigue( 5 );
        p->vomit(); // no hunger/quench benefit for you
        p->mod_daily_health( -8, -50 );
    }
    return 1;
}

std::optional<int> iuse::petfood( Character *p, item *it, const tripoint & )
{
    if( !it->is_comestible() ) {
        p->add_msg_if_player( _( "You doubt someone would want to eat %1$s." ), it->tname() );
        return std::nullopt;
    }

    const std::optional<tripoint> pnt = choose_adjacent( string_format(
                                            _( "Tame which animal with %s?" ),
                                            it->tname() ) );
    if( !pnt ) {
        return std::nullopt;
    }

    creature_tracker &creatures = get_creature_tracker();
    // First a check to see if we are trying to feed a NPC dog food.
    if( npc *const who = creatures.creature_at<npc>( *pnt ) ) {
        if( query_yn( _( "Are you sure you want to feed a person %1$s?" ), it->tname() ) ) {
            p->mod_moves( -to_moves<int>( 1_seconds ) );
            p->add_msg_if_player( _( "You put your %1$s into %2$s's mouth!" ),
                                  it->tname(), who->disp_name( true ) );
            if( x_in_y( 9, 10 ) || who->is_ally( *p ) ) {
                who->say(
                    _( "Okay, but please, don't give me this again.  I don't want to eat pet food in the Cataclysm all day." ) );
            } else {
                p->add_msg_if_player( _( "%s knocks it from your hand!" ), who->disp_name() );
                who->make_angry();
            }
            p->consume_charges( *it, 1 );
            return std::nullopt;
        } else {
            p->add_msg_if_player( _( "Never mind." ) );
            return std::nullopt;
        }
        // Then monsters.
    } else if( monster *const mon = creatures.creature_at<monster>( *pnt, true ) ) {
        p->mod_moves( -to_moves<int>( 1_seconds ) );

        bool can_feed = false;
        const pet_food_data &petfood = mon->type->petfood;
        const std::set<std::string> &itemfood = it->get_comestible()->petfood;
        for( const std::string &food : petfood.food ) {
            if( itemfood.find( food ) != itemfood.end() ) {
                can_feed = true;
                break;
            }
        }

        if( !can_feed ) {
            p->add_msg_if_player( _( "The %s doesn't want that kind of food." ), mon->get_name() );
            return std::nullopt;
        }

        bool halluc = mon->is_hallucination();

        if( mon->type->id == mon_dog_thing ) {
            if( !halluc ) {
                p->deal_damage( mon, bodypart_id( "hand_r" ), damage_instance( STATIC( damage_type_id( "cut" ) ),
                                rng( 1, 10 ) ) );
            }
            p->add_msg_if_player( m_bad, _( "You want to feed it the dog food, but it bites your fingers!" ) );
            if( one_in( 5 ) ) {
                p->add_msg_if_player(
                    _( "Apparently, it's more interested in your flesh than the dog food in your hand!" ) );
                if( halluc ) {
                    item drop_me = p->reduce_charges( it, 1 );
                    p->i_drop_at( drop_me );
                } else {
                    p->consume_charges( *it, 1 );
                }
            }
            return std::nullopt;
        }

        if( halluc && one_in( 4 ) ) {
            p->add_msg_if_player( _( "You try to feed the %1$s some %2$s, but it vanishes!" ),
                                  mon->type->nname(), it->tname() );
            mon->die( nullptr );
            return std::nullopt;
        }

        p->add_msg_if_player( _( "You feed your %1$s to the %2$s." ), it->tname(), mon->get_name() );
        if( mon->has_flag( mon_flag_EATS ) ) {
            int kcal = it->get_comestible()->default_nutrition.kcal();
            mon->amount_eaten += kcal;
            if( mon->amount_eaten >= mon->stomach_size ) {
                p->add_msg_if_player( _( "The %1$s seems full now." ), mon->get_name() );
            }
        } else if( !mon->has_flag( mon_flag_EATS ) ) {
            mon->add_effect( effect_critter_well_fed, 24_hours );
        }

        if( petfood.feed.empty() ) {
            p->add_msg_if_player( m_good, _( "The %1$s is your pet now!" ), mon->get_name() );
        } else {
            p->add_msg_if_player( m_good, petfood.feed, mon->get_name() );
        }

        mon->friendly = -1;
        mon->add_effect( effect_pet, 1_turns, true );
        if( halluc ) {
            item drop_me = p->reduce_charges( it, 1 );
            p->i_drop_at( drop_me );
        } else {
            p->consume_charges( *it, 1 );
        }
        return std::nullopt;
    }
    p->add_msg_if_player( _( "There is nothing to be fed here." ) );
    return std::nullopt;
}

std::optional<int> iuse::radio_mod( Character *p, item *, const tripoint & )
{
    if( p->is_npc() ) {
        // Now THAT would be kinda cruel
        return std::nullopt;
    }

    auto filter = []( const item & itm ) {
        return itm.has_flag( flag_RADIO_MODABLE );
    };

    // note: if !p->is_npc() then p is avatar
    item_location loc = game_menus::inv::titled_filter_menu(
                            filter, *p->as_avatar(), _( "Modify what?" ) );

    if( !loc ) {
        p->add_msg_if_player( _( "You don't have that item!" ) );
        return std::nullopt;
    }
    item &modded = *loc;

    int choice = uilist( _( "Which signal should activate the item?" ), {
        _( "\"Red\"" ), _( "\"Blue\"" ), _( "\"Green\"" )
    } );

    flag_id newtag;
    std::string colorname;
    switch( choice ) {
        case 0:
            newtag = flag_RADIOSIGNAL_1;
            colorname = _( "\"Red\"" );
            break;
        case 1:
            newtag = flag_RADIOSIGNAL_2;
            colorname = _( "\"Blue\"" );
            break;
        case 2:
            newtag = flag_RADIOSIGNAL_3;
            colorname = _( "\"Green\"" );
            break;
        default:
            return std::nullopt;
    }

    if( modded.has_flag( flag_RADIO_MOD ) && modded.has_flag( newtag ) ) {
        p->add_msg_if_player( _( "This item has been modified this way already." ) );
        return std::nullopt;
    }

    remove_radio_mod( modded, *p );

    p->add_msg_if_player(
        _( "You modify your %1$s to listen for the %2$s activation signal on the radio." ),
        modded.tname(), colorname );
    modded.set_flag( flag_RADIO_ACTIVATION )
    .set_flag( flag_RADIOCARITEM )
    .set_flag( flag_RADIO_MOD )
    .set_flag( newtag );
    return 1;
}

std::optional<int> iuse::remove_all_mods( Character *p, item *, const tripoint & )
{
    if( !p ) {
        return std::nullopt;
    }

    item_location loc = g->inv_map_splice( []( const item & e ) {
        for( const item *it : e.toolmods() ) {
            if( !it->is_irremovable() ) {
                return true;
            }
        }
        return false;
    },
    _( "Remove mods from tool?" ), 1,
    _( "You don't have any modified tools." ) );

    if( !loc ) {
        add_msg( m_info, _( "Never mind." ) );
        return std::nullopt;
    }

    if( !loc->ammo_remaining() || p->unload( loc ) ) {
        item *mod = loc->get_item_with(
        []( const item & e ) {
            return e.is_toolmod() && !e.is_irremovable();
        } );
        add_msg( m_info, _( "You remove the %s from the tool." ), mod->tname() );
        p->i_add_or_drop( *mod );
        loc->remove_item( *mod );
        remove_radio_mod( *loc, *p );
        loc->on_contents_changed();
    }
    return 0;
}

static bool good_fishing_spot( const tripoint &pos, Character *p )
{
    std::unordered_set<tripoint> fishable_locations = g->get_fishable_locations( 60, pos );
    std::vector<monster *> fishables = g->get_fishable_monsters( fishable_locations );
    map &here = get_map();
    // isolated little body of water with no definite fish population
    // TODO: fix point types
    const oter_id &cur_omt =
        overmap_buffer.ter( tripoint_abs_omt( ms_to_omt_copy( here.getabs( pos ) ) ) );
    std::string om_id = cur_omt.id().c_str();
    if( fishables.empty() && !here.has_flag( ter_furn_flag::TFLAG_CURRENT, pos ) &&
        // this is a ridiculous way to find a good fishing spot, but I'm just trying
        // to do oceans right now.  Maybe is_water_body() would be better?
        // if you find this comment still here and it's later than 2025, LOL.
        om_id.find( "river_" ) == std::string::npos && !cur_omt->is_lake() && !cur_omt->is_ocean() &&
        !cur_omt->is_lake_shore() && !cur_omt->is_ocean_shore() ) {
        p->add_msg_if_player( m_info, _( "You doubt you will have much luck catching fish here." ) );
        return false;
    }
    return true;
}

std::optional<int> iuse::fishing_rod( Character *p, item *it, const tripoint & )
{
    if( p->is_npc() ) {
        // Long actions - NPCs don't like those yet.
        return std::nullopt;
    }
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    map &here = get_map();
    std::optional<tripoint> found;
    for( const tripoint &pnt : here.points_in_radius( p->pos(), 1 ) ) {
        if( here.has_flag( ter_furn_flag::TFLAG_FISHABLE, pnt ) && good_fishing_spot( pnt, p ) ) {
            found = pnt;
            break;
        }
    }
    if( !found ) {
        p->add_msg_if_player( m_info, _( "You can't fish there!" ) );
        return std::nullopt;
    }
    p->add_msg_if_player( _( "You cast your line and wait to hook something…" ) );
    p->assign_activity( ACT_FISH, to_moves<int>( 5_hours ), 0, 0, it->tname() );
    p->activity.targets.emplace_back( *p, it );
    p->activity.coord_set = g->get_fishable_locations( 60, *found );
    return 0;
}

std::optional<int> iuse::fish_trap( Character *p, item *it, const tripoint & )
{
    map &here = get_map();
    // Handle deploying fish trap.
    if( it->active ) {
        it->active = false;
        return 0;
    }

    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    if( p->cant_do_underwater() ) {
        return std::nullopt;
    }

    if( it->ammo_remaining() == 0 ) {
        p->add_msg_if_player( _( "Fish aren't foolish enough to go in here without bait." ) );
        return std::nullopt;
    }

    const std::optional<tripoint> pnt_ = choose_adjacent( _( "Put fish trap where?" ) );
    if( !pnt_ ) {
        return std::nullopt;
    }
    const tripoint pnt = *pnt_;

    if( !here.has_flag( ter_furn_flag::TFLAG_FISHABLE, pnt ) ) {
        p->add_msg_if_player( m_info, _( "You can't fish there!" ) );
        return std::nullopt;
    }
    if( !good_fishing_spot( pnt, p ) ) {
        return std::nullopt;
    }
    it->active = true;
    it->set_age( 0_turns );
    here.add_item_or_charges( pnt, *it );
    p->i_rem( it );
    p->add_msg_if_player( m_info,
                          _( "You place the fish trap; in three hours or so, you may catch some fish." ) );

    return 0;

}

std::optional<int> iuse::fish_trap_tick( Character *p, item *it, const tripoint &pos )
{
    map &here = get_map();
    // Handle processing fish trap over time.
    if( it->ammo_remaining() == 0 ) {
        it->active = false;
        return 0;
    }
    if( it->age() > 3_hours ) {
        it->active = false;

        if( !here.has_flag( ter_furn_flag::TFLAG_FISHABLE, pos ) ) {
            return 0;
        }

        avatar &player = get_avatar();

        int success = -50;
        const float surv = player.get_skill_level( skill_survival );
        const int attempts = rng( it->ammo_remaining(), it->ammo_remaining() * it->ammo_remaining() );
        for( int i = 0; i < attempts; i++ ) {
            /** @EFFECT_SURVIVAL randomly increases number of fish caught in fishing trap */
            success += rng( round( surv ), round( surv * surv ) );
        }

        int bait_consumed = rng( 0, it->ammo_remaining() + 1 );
        if( bait_consumed > it->ammo_remaining() ) {
            bait_consumed = it->ammo_remaining();
        }

        int fishes = 0;

        if( success < 0 ) {
            fishes = 0;
        } else if( success < 300 ) {
            fishes = 1;
        } else if( success < 1500 ) {
            fishes = 2;
        } else {
            fishes = rng( 3, 5 );
        }

        if( fishes == 0 ) {
            it->ammo_consume( it->ammo_remaining(), pos, nullptr );
            player.practice( skill_survival, rng( 5, 15 ) );

            return 0;
        }

        //get the fishables around the trap's spot
        std::unordered_set<tripoint> fishable_locations = g->get_fishable_locations( 60, pos );
        std::vector<monster *> fishables = g->get_fishable_monsters( fishable_locations );
        for( int i = 0; i < fishes; i++ ) {
            player.practice( skill_survival, rng( 3, 10 ) );
            if( !fishables.empty() ) {
                monster *chosen_fish = random_entry( fishables );
                // reduce the abstract fish_population marker of that fish
                chosen_fish->fish_population -= 1;
                if( chosen_fish->fish_population <= 0 ) {
                    g->catch_a_monster( chosen_fish, pos, p, 300_hours ); //catch the fish!
                } else {
                    here.add_item_or_charges( pos, item::make_corpse( chosen_fish->type->id,
                                              calendar::turn + rng( 0_turns,
                                                      3_hours ) ) );
                }
            } else {
                //there will always be a chance that the player will get lucky and catch a fish
                //not existing in the fishables vector. (maybe it was in range, but wandered off)
                //lets say it is a 5% chance per fish to catch
                if( one_in( 20 ) ) {
                    const std::vector<mtype_id> fish_group = MonsterGroupManager::GetMonstersFromGroup(
                                GROUP_FISH, true );
                    const mtype_id &fish_mon = random_entry_ref( fish_group );
                    //Yes, we can put fishes in the trap like knives in the boot,
                    //and then get fishes via activation of the item,
                    //but it's not as comfortable as if you just put fishes in the same tile with the trap.
                    //Also: corpses and comestibles do not rot in containers like this, but on the ground they will rot.
                    //we don't know when it was caught so use a random turn
                    here.add_item_or_charges( pos, item::make_corpse( fish_mon, it->birthday() + rng( 0_turns,
                                              3_hours ) ) );
                    break; //this can happen only once
                }
            }
        }
        it->ammo_consume( bait_consumed, pos, nullptr );
    }
    return 0;
}

std::optional<int> iuse::extinguisher( Character *p, item *it, const tripoint & )
{
    if( !it->ammo_sufficient( p ) ) {
        return std::nullopt;
    }
    // If anyone other than the player wants to use one of these,
    // they're going to need to figure out how to aim it.
    const std::optional<tripoint> dest_ = choose_adjacent( _( "Spray where?" ) );
    if( !dest_ ) {
        return std::nullopt;
    }
    tripoint dest = *dest_;

    p->moves -= to_moves<int>( 2_seconds );

    map &here = get_map();
    // Reduce the strength of fire (if any) in the target tile.
    here.add_field( dest, fd_extinguisher, 3, 10_turns );

    // Also spray monsters in that tile.
    if( monster *const mon_ptr = get_creature_tracker().creature_at<monster>( dest, true ) ) {
        monster &critter = *mon_ptr;
        critter.moves -= to_moves<int>( 2_seconds );
        bool blind = false;
        if( one_in( 2 ) && critter.has_flag( mon_flag_SEES ) ) {
            blind = true;
            critter.add_effect( effect_blind, rng( 1_minutes, 2_minutes ) );
        }
        viewer &player_view = get_player_view();
        if( player_view.sees( critter ) ) {
            p->add_msg_if_player( _( "The %s is sprayed!" ), critter.name() );
            if( blind ) {
                p->add_msg_if_player( _( "The %s looks blinded." ), critter.name() );
            }
        }
        if( critter.made_of( phase_id::LIQUID ) ) {
            if( player_view.sees( critter ) ) {
                p->add_msg_if_player( _( "The %s is frozen!" ), critter.name() );
            }
            critter.apply_damage( p, bodypart_id( "torso" ), rng( 20, 60 ) );
            critter.set_speed_base( critter.get_speed_base() / 2 );
        }
    }

    // Slightly reduce the strength of fire immediately behind the target tile.
    if( here.passable( dest ) ) {
        dest.x += ( dest.x - p->posx() );
        dest.y += ( dest.y - p->posy() );

        here.mod_field_intensity( dest, fd_fire, std::min( 0 - rng( 0, 1 ) + rng( 0, 1 ), 0 ) );
    }

    return 1;
}

class exosuit_interact
{
    public:
        static int run( item *it ) {
            exosuit_interact menu( it );
            menu.interact_loop();
            return menu.moves;
        }

    private:
        explicit exosuit_interact( item *it ) : suit( it ), ctxt( "", keyboard_mode::keycode ) {
            ctxt.register_navigate_ui_list();
            ctxt.register_leftright();
            ctxt.register_action( "SCROLL_INFOBOX_UP" );
            ctxt.register_action( "SCROLL_INFOBOX_DOWN" );
            ctxt.register_action( "CONFIRM" );
            ctxt.register_action( "QUIT" );
            // mouse selection
            ctxt.register_action( "SELECT" );
            ctxt.register_action( "SEC_SELECT" );
            ctxt.register_action( "MOUSE_MOVE" );
            ctxt.register_action( "SCROLL_UP" );
            ctxt.register_action( "SCROLL_DOWN" );
            pocket_count = it->get_all_contained_pockets().size();
            height = std::max( pocket_count, height_default ) + 2;
            width_menu = 30;
            for( const item_pocket *pkt : it->get_all_contained_pockets() ) {
                int tmp = utf8_width( get_pocket_name( pkt ) );
                if( tmp > width_menu ) {
                    width_menu = tmp;
                }
            }
            width_menu = std::min( width_menu, 50 );
            width_info = 80 - width_menu;
            moves = 0;
        }
        ~exosuit_interact() = default;

        item *suit;
        weak_ptr_fast<ui_adaptor> ui;
        input_context ctxt;
        catacurses::window w_border;
        catacurses::window w_info;
        catacurses::window w_menu;
        std::map<int, inclusive_rectangle<point>> pkt_map;
        int moves = 0;
        int pocket_count = 0;
        int cur_pocket = 0;
        int scroll_pos = 0;
        int height = 0;
        const int height_default = 20;
        int width_info = 30;
        int width_menu = 30;
        int sel_frame = 0;

        static std::string get_pocket_name( const item_pocket *pkt ) {
            if( !pkt->get_pocket_data()->pocket_name.empty() ) {
                return pkt->get_pocket_data()->pocket_name.translated();
            }
            const std::set<flag_id> flags = pkt->get_pocket_data()->get_flag_restrictions();
            return enumerate_as_string( flags, []( const flag_id & fid ) {
                if( fid->name().empty() ) {
                    return fid.str();
                }
                return fid->name();
            } );
        }

        void init_windows() {
            const point topleft( TERMX / 2 - ( width_info + width_menu + 3 ) / 2, TERMY / 2 - height / 2 );
            //NOLINTNEXTLINE(cata-use-named-point-constants)
            w_menu = catacurses::newwin( height - 2, width_menu, topleft + point( 1, 1 ) );
            w_info = catacurses::newwin( height - 2, width_info, topleft + point( 2 + width_menu, 1 ) );
            w_border = catacurses::newwin( height, width_info + width_menu + 3, topleft );
        }

        void draw_menu() {
            pkt_map.clear();
            // info box
            pkt_map.emplace( -1, inclusive_rectangle<point>( point( 2 + width_menu, 1 ),
                             point( 2 + width_menu + width_info, height - 2 ) ) );
            werase( w_menu );
            int row = 0;
            for( const item_pocket *pkt : suit->get_all_contained_pockets() ) {
                nc_color colr = row == cur_pocket ? h_white : c_white;
                std::string txt = get_pocket_name( pkt );
                int remaining = width_menu - utf8_width( txt, true );
                if( remaining > 0 ) {
                    txt.append( remaining, ' ' );
                }
                trim_and_print( w_menu, point( 0, row ), width_menu, colr, txt );
                pkt_map.emplace( row, inclusive_rectangle<point>( point( 0, row ), point( width_menu, row ) ) );
                row++;
            }
            wnoutrefresh( w_menu );
        }

        void draw_iteminfo() {
            std::vector<iteminfo> dummy;
            std::vector<iteminfo> suitinfo;
            item_pocket *pkt = suit->get_all_contained_pockets()[cur_pocket];
            pkt->general_info( suitinfo, cur_pocket, true );
            pkt->contents_info( suitinfo, cur_pocket, true );
            item_info_data data( suit->tname(), suit->type_name(), suitinfo, dummy, scroll_pos );
            data.without_getch = true;
            data.without_border = true;
            data.scrollbar_left = false;
            data.use_full_win = true;
            data.padding = 0;
            draw_item_info( w_info, data );
        }

        shared_ptr_fast<ui_adaptor> create_or_get_ui_adaptor() {
            shared_ptr_fast<ui_adaptor> current_ui = ui.lock();
            if( !current_ui ) {
                ui = current_ui = make_shared_fast<ui_adaptor>();
                current_ui->on_screen_resize( [this]( ui_adaptor & cui ) {
                    init_windows();
                    cui.position_from_window( w_border );
                } );
                current_ui->mark_resize();
                current_ui->on_redraw( [this]( const ui_adaptor & ) {
                    draw_border( w_border, c_white, suit->tname(), c_light_green );
                    for( int i = 1; i < height - 1; i++ ) {
                        mvwputch( w_border, point( width_menu + 1, i ), c_white, LINE_XOXO );
                    }
                    mvwputch( w_border, point( width_menu + 1, height - 1 ), c_white, LINE_XXOX );
                    wnoutrefresh( w_border );
                    draw_menu();
                    draw_iteminfo();
                } );
            }
            return current_ui;
        }

        void interact_loop() {
            bool done = false;
            shared_ptr_fast<ui_adaptor> current_ui = create_or_get_ui_adaptor();
            while( !done ) {
                ui_manager::redraw();
                std::string action = ctxt.handle_input();
                if( action == "MOUSE_MOVE" || action == "SELECT" ) {
                    std::optional<point> coord = ctxt.get_coordinates_text( w_border );
                    if( !!coord ) {
                        int tmp_frame = 0;
                        run_for_point_in<int, point>( pkt_map, *coord,
                        [&tmp_frame]( const std::pair<int, inclusive_rectangle<point>> &p ) {
                            if( p.first == -1 ) {
                                tmp_frame = 1;
                            }
                        } );
                        sel_frame = tmp_frame;
                    }
                    coord = ctxt.get_coordinates_text( w_menu );
                    if( !!coord ) {
                        int tmp_pocket = cur_pocket;
                        run_for_point_in<int, point>( pkt_map, *coord,
                        [&tmp_pocket, &action]( const std::pair<int, inclusive_rectangle<point>> &p ) {
                            if( p.first >= 0 ) {
                                tmp_pocket = p.first;
                                if( action == "SELECT" ) {
                                    action = "CONFIRM";
                                }
                            }
                        } );
                        cur_pocket = tmp_pocket;
                    }
                } else if( action == "SCROLL_UP" || action == "SCROLL_DOWN" ) {
                    if( sel_frame == 0 ) {
                        action = action == "SCROLL_UP" ? "UP" : "DOWN";
                    } else {
                        action = action == "SCROLL_UP" ? "SCROLL_INFOBOX_UP" : "SCROLL_INFOBOX_DOWN";
                    }
                }
                if( action == "QUIT" || action == "SEC_SELECT" ) {
                    scroll_pos = 0;
                    done = true;
                } else if( action == "CONFIRM" ) {
                    scroll_pos = 0;
                    int nmoves = insert_replace_activate_mod(
                                     suit->get_all_contained_pockets()[cur_pocket], suit );
                    moves = moves > nmoves ? moves : nmoves;
                    if( !get_player_character().activity.is_null() ) {
                        done = true;
                    }
                } else if( navigate_ui_list( action, cur_pocket, 5, pocket_count, true ) ) {
                    scroll_pos = 0;
                    sel_frame = 0;
                } else if( action == "SCROLL_INFOBOX_UP" ) {
                    scroll_pos--;
                    sel_frame = 1;
                } else if( action == "SCROLL_INFOBOX_DOWN" ) {
                    scroll_pos++;
                    sel_frame = 1;
                }
            }
        }

        int insert_replace_activate_mod( item_pocket *pkt, item *it ) {
            Character &c = get_player_character();
            map &here = get_map();
            const std::set<flag_id> flags = pkt->get_pocket_data()->get_flag_restrictions();
            if( flags.empty() ) {
                //~ Modular exoskeletons require pocket restrictions to insert modules. %s = pocket name.
                popup( _( "%s doesn't define any restrictions for modules!" ), get_pocket_name( pkt ) );
                return 0;
            }

            // If pocket already contains a module, ask to unload or replace
            const bool not_empty = !pkt->empty();
            if( not_empty ) {
                item *mod_it = pkt->all_items_top().front();
                std::string mod_name = mod_it->tname();
                uilist amenu;
                //~ Prompt the player to handle the module inside the modular exoskeleton
                amenu.text = _( "What to do with the existing module?" );
                amenu.addentry( -1, true, MENU_AUTOASSIGN, _( "Unload everything from this %s" ),
                                get_pocket_name( pkt ) );
                amenu.addentry( -1, true, MENU_AUTOASSIGN, _( "Replace the %s" ), mod_name );
                amenu.addentry( -1, mod_it->has_relic_activation() || mod_it->type->has_use(), MENU_AUTOASSIGN,
                                mod_it->active ? _( "Deactivate the %s" ) : _( "Activate the %s" ), mod_name );
                amenu.addentry( -1, mod_it->is_reloadable() && c.can_reload( *mod_it ), MENU_AUTOASSIGN,
                                _( "Reload the %s" ), mod_name );
                amenu.addentry( -1, !mod_it->is_container_empty(), MENU_AUTOASSIGN, _( "Unload the %s" ),
                                mod_name );
                amenu.query();
                int ret = amenu.ret;
                item_location loc_it;
                item_location held = c.get_wielded_item();
                if( !!held && held->has_item( *mod_it ) ) {
                    loc_it = item_location( held, mod_it );
                } else {
                    for( const item_location &loc : c.top_items_loc() ) {
                        if( loc->has_item( *mod_it ) ) {
                            loc_it = item_location( loc, mod_it );
                            break;
                        }
                    }
                }
                if( ret < 0 || ret > 4 ) {
                    return 0;
                } else if( ret == 0 ) {
                    // Unload existing module
                    pkt->remove_items_if( [&c, &here]( const item & i ) {
                        here.add_item_or_charges( c.pos(), i );
                        return true;
                    } );
                    return to_moves<int>( 5_seconds );
                } else if( ret == 2 ) {
                    if( !!loc_it ) {
                        avatar_action::use_item( get_avatar(), loc_it );
                    }
                    return 0;
                } else if( ret == 3 ) {
                    if( !!loc_it ) {
                        g->reload( loc_it );
                    }
                    return 0;
                } else if( ret == 4 ) {
                    if( !!loc_it ) {
                        c.unload( loc_it );
                    }
                    return 0;
                }
            }

            const item_filter filter = [&flags, pkt, it]( const item & i ) {
                return i.has_any_flag( flags ) && ( pkt->empty() || !it->has_item( i ) ) &&
                       pkt->can_contain( i ).success();
            };

            std::vector<item_location> candidates;
            for( item *i : c.items_with( filter ) ) {
                candidates.emplace_back( c, i );
            }
            for( const tripoint &p : here.points_in_radius( c.pos(), PICKUP_RANGE ) ) {
                for( item &i : here.i_at( p ) ) {
                    if( filter( i ) ) {
                        candidates.emplace_back( map_cursor( p ), &i );
                    }
                }
            }
            if( candidates.empty() ) {
                //~ The player has nothing that fits in the modular exoskeleton's pocket
                popup( _( "You don't have anything compatible with this module!" ) );
                return 0;
            }

            //~ Prompt the player to select an item to attach to the modular exoskeleton's pocket (%s)
            uilist imenu( string_format( _( "Which module to attach to the %s?" ), get_pocket_name( pkt ) ), {} );
            for( const item_location &i : candidates ) {
                imenu.addentry( -1, true, MENU_AUTOASSIGN, i->tname() );
            }
            imenu.query();
            int ret = imenu.ret;
            if( ret < 0 || static_cast<size_t>( ret ) >= candidates.size() ) {
                // Cancelled
                return 0;
            }

            int moves = 0;

            // Unload existing module
            if( not_empty ) {
                pkt->remove_items_if( [&c, &here]( const item & i ) {
                    here.add_item_or_charges( c.pos(), i );
                    return true;
                } );
                moves += to_moves<int>( 5_seconds );
            }
            ret_val<item *> rval = pkt->insert_item( *candidates[ret] );
            if( rval.success() ) {
                candidates[ret].remove_item();
                moves += to_moves<int>( 5_seconds );
                return moves;
            }
            debugmsg( "Could not insert item \"%s\" into pocket \"%s\": %s",
                      candidates[ret]->type_name(), get_pocket_name( pkt ), rval.str() );
            return moves;
        }
};

std::optional<int> iuse::mace( Character *p, item *it, const tripoint & )
{
    if( !it->ammo_sufficient( p ) ) {
        return std::nullopt;
    }
    // If anyone other than the player wants to use one of these,
    // they're going to need to figure out how to aim it.
    const std::optional<tripoint> dest_ = choose_adjacent( _( "Spray where?" ) );
    if( !dest_ ) {
        return std::nullopt;
    }
    tripoint dest = *dest_;

    p->moves -= to_moves<int>( 2_seconds );

    map &here = get_map();
    here.add_field( dest, fd_tear_gas, 2, 3_turns );

    // Also spray monsters in that tile.
    if( monster *const mon_ptr = get_creature_tracker().creature_at<monster>( dest, true ) ) {
        monster &critter = *mon_ptr;
        critter.moves -= to_moves<int>( 2_seconds );
        bool blind = false;
        if( one_in( 2 ) && critter.has_flag( mon_flag_SEES ) ) {
            blind = true;
            critter.add_effect( effect_blind, rng( 1_minutes, 2_minutes ) );
        }
        // even if it's not blinded getting maced hurts a lot and stuns it
        if( !critter.has_flag( mon_flag_NO_BREATHE ) ) {
            critter.moves -= to_moves<int>( 3_seconds );
            p->add_msg_if_player( _( "The %s recoils in pain!" ), critter.name() );
        }
        viewer &player_view = get_player_view();
        if( player_view.sees( critter ) ) {
            p->add_msg_if_player( _( "The %s is sprayed!" ), critter.name() );
            if( blind ) {
                p->add_msg_if_player( _( "The %s looks blinded." ), critter.name() );
            }
        }
    }

    return 1;
}

std::optional<int> iuse::manage_exosuit( Character *p, item *it, const tripoint & )
{
    if( !p->is_avatar() ) {
        return std::nullopt;
    }
    if( it->get_all_contained_pockets().empty() ) {
        add_msg( m_warning, _( "Your %s does not have any pockets to contain modules." ), it->tname() );
        return std::nullopt;
    }
    p->moves -= exosuit_interact::run( it );
    return 0;
}

std::optional<int> iuse::rm13armor_off( Character *p, item *it, const tripoint & )
{
    // This allows it to turn on for a turn, because ammo_sufficient assumes non-tool non-weapons need zero ammo, for some reason.
    if( !it->ammo_sufficient( p ) ) {
        p->add_msg_if_player( m_info, _( "The RM13 combat armor's fuel cells are dead." ) );
        return std::nullopt;
    } else {
        std::string oname = it->typeId().str() + "_on";
        p->add_msg_if_player( _( "You activate your RM13 combat armor." ) );
        p->add_msg_if_player( _( "Rivtech Model 13 RivOS v2.19:   ONLINE." ) );
        p->add_msg_if_player( _( "CBRN defense system:            ONLINE." ) );
        p->add_msg_if_player( _( "Acoustic dampening system:      ONLINE." ) );
        p->add_msg_if_player( _( "Thermal regulation system:      ONLINE." ) );
        p->add_msg_if_player( _( "Vision enhancement system:      ONLINE." ) );
        p->add_msg_if_player( _( "Electro-reactive armor system:  ONLINE." ) );
        p->add_msg_if_player( _( "All systems nominal." ) );
        it->convert( itype_id( oname ), p ).active = true;
        p->calc_encumbrance();
        return 1;
    }
}

std::optional<int> iuse::rm13armor_on( Character *p, item *it, const tripoint & )
{
    if( !p ) { // Normal use
        debugmsg( "%s called action rm13armor_on that requires character but no character is present",
                  it->typeId().str() );
    } else { // Turning it off
        std::string oname = it->typeId().str();
        if( string_ends_with( oname, "_on" ) ) {
            oname.erase( oname.length() - 3, 3 );
        } else {
            debugmsg( "no item type to turn it into (%s)!", oname );
            return 0;
        }
        p->add_msg_if_player( _( "RivOS v2.19 shutdown sequence initiated." ) );
        p->add_msg_if_player( _( "Shutting down." ) );
        p->add_msg_if_player( _( "Your RM13 combat armor turns off." ) );
        it->convert( itype_id( oname ), p ).active = false;
        p->calc_encumbrance();
    }
    return 1;
}

std::optional<int> iuse::unpack_item( Character *p, item *it, const tripoint & )
{
    if( p->cant_do_underwater() ) {
        return std::nullopt;
    }
    std::string oname = it->typeId().str() + "_on";
    p->moves -= to_moves<int>( 10_seconds );
    p->add_msg_if_player( _( "You unpack your %s for use." ), it->tname() );
    it->convert( itype_id( oname ), p ).active = false;
    // Check if unpacking led to invalid container state
    p->invalidate_inventory_validity_cache();
    return 0;
}

std::optional<int> iuse::pack_cbm( Character *p, item *it, const tripoint & )
{
    item_location bionic = g->inv_map_splice( []( const item & e ) {
        return e.is_bionic() && e.has_flag( flag_NO_PACKED );
    }, _( "Choose CBM to pack" ), PICKUP_RANGE, _( "You don't have any CBMs." ) );

    if( !bionic ) {
        return std::nullopt;
    }
    if( !bionic.get_item()->faults.empty() ) {
        if( p->query_yn( _( "This CBM is faulty.  You should mend it first.  Do you want to try?" ) ) ) {
            p->mend_item( std::move( bionic ) );
        }
        return 0;
    }

    const int success = round( p->get_skill_level( skill_firstaid ) ) - rng( 0, 6 );
    if( success > 0 ) {
        p->add_msg_if_player( m_good, _( "You carefully prepare the CBM for sterilization." ) );
        bionic.get_item()->unset_flag( flag_NO_PACKED );
    } else {
        p->add_msg_if_player( m_bad, _( "You fail to properly prepare the CBM." ) );
    }

    std::vector<item_comp> comps;
    comps.emplace_back( it->typeId(), 1 );
    p->consume_items( comps, 1, is_crafting_component );

    return 0;
}

std::optional<int> iuse::pack_item( Character *p, item *it, const tripoint & )
{
    if( !p ) {
        debugmsg( "%s called action pack_item that requires character but no character is present",
                  it->typeId().str() );
    }
    if( p->cant_do_underwater() ) {
        return std::nullopt;
    }

    if( p->is_worn( *it ) ) {
        p->add_msg_if_player( m_info, _( "You can't pack your %s until you take it off." ),
                              it->tname() );
        return std::nullopt;
    } else { // Turning it off
        std::string oname = it->typeId().str();
        if( string_ends_with( oname, "_on" ) ) {
            oname.erase( oname.length() - 3, 3 );
        } else {
            debugmsg( "no item type to turn it into (%s)!", oname );
            return std::nullopt;
        }
        p->moves -= to_moves<int>( 10_seconds );
        p->add_msg_if_player( _( "You pack your %s for storage." ), it->tname() );
        it->convert( itype_id( oname ), p ).active = false;
    }
    return 0;
}

std::optional<int> iuse::water_purifier( Character *p, item *it, const tripoint & )
{
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    item_location obj = g->inv_map_splice( []( const item & e ) {
        return !e.empty() && e.has_item_with( []( const item & it ) {
            return it.typeId() == itype_water;
        } );
    }, _( "Purify what?" ), 1, _( "You don't have water to purify." ) );

    if( !obj ) {
        p->add_msg_if_player( m_info, _( "You don't have that item!" ) );
        return std::nullopt;
    }

    const std::vector<item *> liquids = obj->items_with( []( const item & it ) {
        return it.typeId() == itype_water;
    } );
    int charges_of_water = 0;
    for( const item *water : liquids ) {
        charges_of_water += water->charges;
    }
    if( !it->ammo_sufficient( p, charges_of_water ) ) {
        p->add_msg_if_player( m_info, _( "That volume of water is too large to purify." ) );
        return std::nullopt;
    }

    p->moves -= to_moves<int>( 2_seconds );

    for( item *water : liquids ) {
        water->convert( itype_water_clean, p ).poison = 0;
    }
    return charges_of_water;
}

std::optional<int> iuse::radio_off( Character *p, item *it, const tripoint & )
{
    if( !it->ammo_sufficient( p ) ) {
        p->add_msg_if_player( _( "It's dead." ) );
    } else {
        p->add_msg_if_player( _( "You turn the radio on." ) );
        it->convert( itype_radio_on, p ).active = true;
    }
    return 1;
}

std::optional<int> iuse::directional_antenna( Character *p, item *, const tripoint & )
{
    // Find out if we have an active radio
    auto radios = p->cache_get_items_with( itype_radio_on );
    // If we don't wield the radio, also check on the ground
    if( radios.empty() ) {
        map_stack items = get_map().i_at( p->pos() );
        for( item &an_item : items ) {
            if( an_item.typeId() == itype_radio_on ) {
                radios.push_back( &an_item );
            }
        }
    }
    if( radios.empty() ) {
        add_msg( m_info, _( "You must have an active radio to check for signal direction." ) );
        return std::nullopt;
    }
    const item radio = *radios.front();
    // Find the radio station it's tuned to (if any)
    const radio_tower_reference tref = overmap_buffer.find_radio_station( radio.frequency );
    if( !tref ) {
        p->add_msg_if_player( m_info, _( "You can't find the direction if your radio isn't tuned." ) );
        return std::nullopt;
    }
    // Report direction.
    const tripoint_abs_sm player_pos = p->global_sm_location();
    direction angle = direction_from( player_pos.xy(), tref.abs_sm_pos );
    add_msg( _( "The signal seems strongest to the %s." ), direction_name( angle ) );
    return 1;
}

// 0-100 percent chance of a character in a radio signal being obscured by static
static int radio_static_chance( const radio_tower_reference &tref )
{
    constexpr int HALF_RADIO_MIN = RADIO_MIN_STRENGTH / 2;
    const int signal_strength = tref.signal_strength;
    const int max_strength = tref.tower->strength;
    int dist = max_strength - signal_strength;
    // For towers whose strength is quite close to the min, make them act as though they are farther away
    if( RADIO_MIN_STRENGTH * 1.25 > max_strength ) {
        dist += 25;
    }
    // When we're close enough, there's no noise
    if( dist < HALF_RADIO_MIN ) {
        return 0;
    }
    // There's minimal, but increasing noise when quite close to the signal
    if( dist < RADIO_MIN_STRENGTH ) {
        return lerp( 1, 20, static_cast<float>( dist - HALF_RADIO_MIN ) / HALF_RADIO_MIN );
    }
    // Otherwise, just a rapid increase until the signal stops
    return lerp( 20, 100, static_cast<float>( dist - RADIO_MIN_STRENGTH ) /
                 ( max_strength - RADIO_MIN_STRENGTH ) );
}

std::optional<int> iuse::radio_tick( Character *, item *it, const tripoint &pos )
{
    std::string message = _( "Radio: Kssssssssssssh." );
    const radio_tower_reference tref = overmap_buffer.find_radio_station( it->frequency );
    add_msg_debug( debugmode::DF_RADIO, "Set freq: %d", it->frequency );
    if( tref ) {
        point_abs_omt dbgpos = project_to<coords::omt>( tref.abs_sm_pos );
        add_msg_debug( debugmode::DF_RADIO, "found broadcast (str %d) at (%d %d)",
                       tref.signal_strength, dbgpos.x(), dbgpos.y() );
        const radio_tower *selected_tower = tref.tower;
        if( selected_tower->type == radio_type::MESSAGE_BROADCAST ) {
            message = selected_tower->message;
        } else if( selected_tower->type == radio_type::WEATHER_RADIO ) {
            message = weather_forecast( tref.abs_sm_pos );
        }

        const city *c = overmap_buffer.closest_city( tripoint_abs_sm( tref.abs_sm_pos, 0 ) ).city;
        //~ radio noise
        const std::string cityname = c == nullptr ? _( "ksssh" ) : c->name;

        replace_city_tag( message, cityname );
        int static_chance = radio_static_chance( tref );
        add_msg_debug( debugmode::DF_RADIO, "Message: '%s' at %d%% noise", message, static_chance );
        message = obscure_message( message, [&static_chance]()->int {
            if( x_in_y( static_chance, 100 ) )
            {
                // Gradually replace random characters with noise as distance increases
                if( one_in( 3 ) && static_chance - rng( 0, 25 ) < 50 ) {
                    // Replace with random character
                    return 0;
                }
                // Replace with '#'
                return '#';
            }
            // Leave unchanged
            return -1;
        } );

        std::vector<std::string> segments = foldstring( message, RADIO_PER_TURN );
        int index = to_turn<int>( calendar::turn ) % segments.size();
        message = string_format( _( "radio: %s" ), segments[index] );
    }
    sounds::ambient_sound( pos, 6, sounds::sound_t::electronic_speech, message );
    if( !sfx::is_channel_playing( sfx::channel::radio ) ) {
        if( one_in( 10 ) ) {
            sfx::play_ambient_variant_sound( "radio", "static", 100, sfx::channel::radio, 300, -1, 0 );
        } else if( one_in( 10 ) ) {
            sfx::play_ambient_variant_sound( "radio", "inaudible_chatter", 100, sfx::channel::radio, 300, -1,
                                             0 );
        }
    }
    return 1;
}

std::optional<int> iuse::radio_on( Character *, item *it, const tripoint & )
{

    const auto tower_desc = []( const int noise ) {
        if( noise == 0 ) {
            return SNIPPET.random_from_category( "radio_station_desc_noise_0" )->translated();
        } else if( noise <= 20 ) {
            return SNIPPET.random_from_category( "radio_station_desc_noise_20" )->translated();
        } else if( noise <= 40 ) {
            return SNIPPET.random_from_category( "radio_station_desc_noise_40" )->translated();
        } else if( noise <= 60 ) {
            return SNIPPET.random_from_category( "radio_station_desc_noise_60" )->translated();
        } else if( noise <= 80 ) {
            return SNIPPET.random_from_category( "radio_station_desc_noise_80" )->translated();
        }
        return SNIPPET.random_from_category( "radio_station_desc_noise_max" )->translated();
    };

    std::vector<radio_tower_reference> options = overmap_buffer.find_all_radio_stations();
    if( options.empty() ) {
        popup( SNIPPET.random_from_category( "radio_scan_no_stations" )->translated() );
    }
    uilist scanlist;
    scanlist.title = _( "Select a station" );
    scanlist.desc_enabled = true;
    add_msg_debug( debugmode::DF_RADIO, "Radio scan:" );
    for( size_t i = 0; i < options.size(); ++i ) {
        std::string selected_text;
        const radio_tower_reference &tref = options[i];
        if( it->frequency == tref.tower->frequency ) {
            selected_text = pgettext( "radio station", " (selected)" );
        }
        //~ Selected radio station, %d is a number in sequence (1,2,3...),
        //~ %s is ' (selected)' if this is the radio station playing, else nothing
        const std::string station_name = string_format( _( "Station %d%s" ), i + 1, selected_text );
        const int noise_chance = radio_static_chance( tref );
        scanlist.addentry_desc( i, true, MENU_AUTOASSIGN, station_name, tower_desc( noise_chance ) );
        const point_abs_omt dbgpos = project_to<coords::omt>( tref.abs_sm_pos );
        add_msg_debug( debugmode::DF_RADIO, "  %d: %d at (%d %d) str [%d/%d]", i + 1, tref.tower->frequency,
                       dbgpos.x(), dbgpos.y(), tref.signal_strength, tref.tower->strength );
    }
    scanlist.query();
    const int sel = scanlist.ret;
    if( sel >= 0 && static_cast<size_t>( sel ) < options.size() ) {
        it->frequency = options[sel].tower->frequency;
    }
    return 1;
}

std::optional<int> iuse::noise_emitter_on( Character *, item *, const tripoint &pos )
{
    sounds::sound( pos, 30, sounds::sound_t::alarm, _( "KXSHHHHRRCRKLKKK!" ), true, "tool",
                   "noise_emitter" );
    return 1;
}

std::optional<int> iuse::emf_passive_on( Character *, item *, const tripoint &pos )
{
    // need to calculate distance to closest electrical thing

    // set distance as farther than the radius
    const int max = 10;
    int distance = max + 1;

    creature_tracker &creatures = get_creature_tracker();
    map &here = get_map();
    // can't get a reading during a portal storm
    if( get_weather().weather_id == weather_portal_storm ) {
        sounds::sound( pos, 6, sounds::sound_t::alarm, _( "BEEEEE-CHHHHHHH-eeEEEEEEE-CHHHHHHHHHHHH" ), true,
                       "tool", "emf_detector" );
        // skip continuing to check for locations
        return 1;
    }

    for( const tripoint &loc : closest_points_first( pos, max ) ) {
        const Creature *critter = creatures.creature_at( loc );

        // if the creature exists and is either a robot or electric
        bool found = critter != nullptr && critter->is_electrical();

        // check for an electrical field
        if( !found ) {
            for( const auto &fd : here.field_at( loc ) ) {
                if( fd.first->has_elec ) {
                    found = true;
                    break;
                }
            }
        }

        // if an electrical field or creature is nearby
        if( found ) {
            distance = rl_dist( pos, loc );
            if( distance <= 3 ) {
                sounds::sound( pos, 4, sounds::sound_t::alarm, _( "BEEEEEEP BEEEEEEP" ), true, "tool",
                               "emf_detector" );
            } else if( distance <= 7 ) {
                sounds::sound( pos, 3, sounds::sound_t::alarm, _( "BEEP BEEP" ), true, "tool",
                               "emf_detector" );
            } else if( distance <= 10 ) {
                sounds::sound( pos, 2, sounds::sound_t::alarm, _( "beep… beep" ), true, "tool",
                               "emf_detector" );
            }
            // skip continuing to check for locations
            return 1;
        }
    }
    return 1;
}

std::optional<int> iuse::ma_manual( Character *p, item *it, const tripoint & )
{
    // [CR] - should NPCs just be allowed to learn this stuff? Just like that?

    const matype_id style_to_learn = martial_art_learned_from( *it->type );

    if( !style_to_learn.is_valid() ) {
        debugmsg( "ERROR: Invalid martial art" );
        return std::nullopt;
    }

    p->martial_arts_data->learn_style( style_to_learn, p->is_avatar() );

    return 1;
}

// Remove after 0.G
std::optional<int> iuse::hammer( Character *p, item *it, const tripoint &pos )
{
    return iuse::crowbar( p, it, pos );
}

std::optional<int> iuse::crowbar_weak( Character *p, item *it, const tripoint &pos )
{
    return iuse::crowbar( p, it, pos );
}

std::optional<int> iuse::crowbar( Character *p, item *it, const tripoint &pos )
{
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }

    map &here = get_map();
    const std::function<bool( const tripoint & )> f =
    [&here, p]( const tripoint & pnt ) {
        if( pnt == p->pos() ) {
            return false;
        } else if( here.has_furn( pnt ) ) {
            return here.furn( pnt )->prying->valid();
        } else if( !here.ter( pnt )->is_null() ) {
            return here.ter( pnt )->prying->valid();
        }
        return false;
    };

    const std::optional<tripoint> pnt_ = ( pos != p->pos() ) ? pos : choose_adjacent_highlight(
            _( "Pry where?" ), _( "There is nothing to pry nearby." ), f, false );
    if( !pnt_ ) {
        return std::nullopt;
    }

    const tripoint &pnt = *pnt_;

    const pry_data *prying;
    if( here.has_furn( pnt ) ) {
        prying = &here.furn( pnt )->prying->prying_data();
    } else {
        prying = &here.ter( pnt )->prying->prying_data();
    }

    if( !f( pnt ) ) {
        if( pnt == p->pos() ) {
            if( it->typeId() == STATIC( itype_id( "hammer" ) ) ) {
                p->add_msg_if_player( m_info, _( "You try to hit yourself with the hammer." ) );
                p->add_msg_if_player( m_info, _( "But you can't touch this." ) );
            } else {
                p->add_msg_if_player( m_info, _( "You attempt to pry open your wallet, "
                                                 "but alas.  You are just too miserly." ) );
            }
        } else {
            p->add_msg_if_player( m_info, _( "You can't pry that." ) );
        }
        return std::nullopt;
    }

    // previously iuse::hammer
    if( prying->prying_nails ) {
        p->assign_activity( prying_activity_actor( pnt, item_location{*p, it} ) );
        return std::nullopt;
    }

    // Doors need PRY 2 which is on a crowbar, crates need PRY 1 which is on a crowbar
    // & a claw hammer.
    // The iexamine function for crate supplies a hammer object.
    // So this stops the player (A)ctivating a Hammer with a Crowbar in their backpack
    // then managing to open a door.
    const int pry_level = it->get_quality( qual_PRY );

    if( pry_level < prying->prying_level ) {
        // This doesn't really make it clear to the player
        // why their attempt is failing.
        p->add_msg_if_player( _( "You can't get sufficient leverage to open that with your %1$s." ),
                              it->tname() );
        p->mod_moves( -50 ); // spend a few moves trying it.
        return std::nullopt;
    }

    p->assign_activity( prying_activity_actor( pnt, item_location{*p, it} ) );

    return std::nullopt;
}

std::optional<int> iuse::makemound( Character *p, item *it, const tripoint & )
{
    if( !p ) {
        debugmsg( "%s called action makemound that requires character but no character is present",
                  it->typeId().str() );
        return std::nullopt;
    }
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    const std::optional<tripoint> pnt_ = choose_adjacent( _( "Till soil where?" ) );
    if( !pnt_ ) {
        return std::nullopt;
    }
    const tripoint pnt = *pnt_;

    if( pnt == p->pos() ) {
        p->add_msg_if_player( m_info,
                              _( "You think about jumping on a shovel, but then change your mind." ) );
        return std::nullopt;
    }

    map &here = get_map();
    if( here.has_flag( ter_furn_flag::TFLAG_PLOWABLE, pnt ) &&
        !here.has_flag( ter_furn_flag::TFLAG_PLANT, pnt ) ) {
        p->add_msg_if_player( _( "You start churning up the earth here." ) );
        p->assign_activity( churn_activity_actor( 18000, item_location( *p, it ) ) );
        p->activity.placement = here.getglobal( pnt );
        return 1;
    } else {
        p->add_msg_if_player( _( "You can't churn up this ground." ) );
        return std::nullopt;
    }
}

std::optional<int> iuse::dig( Character *p, item *it, const tripoint & )
{
    if( !p ) {
        debugmsg( "%s called action dig that requires character but no character is present",
                  it->typeId().str() );
        return std::nullopt;
    }
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }

    std::vector<construction> const &cnstr = get_constructions();
    auto const build = std::find_if( cnstr.begin(), cnstr.end(), []( const construction & it ) {
        return it.str_id == construction_constr_pit;
    } );
    auto const build_shallow = std::find_if( cnstr.begin(), cnstr.end(), []( const construction & it ) {
        return it.str_id == construction_constr_pit_shallow;
    } );

    place_construction( { build->group, build_shallow->group } );

    return 0;
}

std::optional<int> iuse::dig_channel( Character *p, item *it, const tripoint & )
{
    if( !p ) {
        debugmsg( "%s called action dig_channel that requires character but no character is present",
                  it->typeId().str() );
        return std::nullopt;
    }
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }

    std::vector<construction> const &cnstr = get_constructions();
    auto const build = std::find_if( cnstr.begin(), cnstr.end(), []( const construction & it ) {
        return it.str_id == construction_constr_water_channel;
    } );

    place_construction( { build->group } );
    return 0;
}

std::optional<int> iuse::fill_pit( Character *p, item *it, const tripoint & )
{
    if( !p ) {
        debugmsg( "%s called action fill_pit that requires character but no character is present",
                  it->typeId().str() );
        return std::nullopt;
    }
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }

    std::vector<construction> const &cnstr = get_constructions();
    auto const build = std::find_if( cnstr.begin(), cnstr.end(), []( const construction & it ) {
        return it.str_id == construction_constr_fill_pit;
    } );

    place_construction( { build->group } );
    return 0;
}

std::optional<int> iuse::clear_rubble( Character *p, item *it, const tripoint & )
{
    if( !p ) {
        debugmsg( "%s called action clear_rubble that requires character but no character is present",
                  it->typeId().str() );
        return std::nullopt;
    }
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }

    std::vector<construction> const &cnstr = get_constructions();
    auto const build = std::find_if( cnstr.begin(), cnstr.end(), []( const construction & it ) {
        return it.str_id == construction_constr_clear_rubble;
    } );

    place_construction( { build->group } );
    return 0;
}

std::optional<int> iuse::siphon( Character *p, item *, const tripoint & )
{
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    map &here = get_map();
    const std::function<bool( const tripoint & )> f = [&here]( const tripoint & pnt ) {
        const optional_vpart_position vp = here.veh_at( pnt );
        return !!vp;
    };

    vehicle *v = nullptr;
    bool found_more_than_one = false;
    for( const tripoint &pos : here.points_in_radius( p->pos(), 1 ) ) {
        const optional_vpart_position vp = here.veh_at( pos );
        if( !vp ) {
            continue;
        }
        vehicle *vfound = &vp->vehicle();
        if( v == nullptr ) {
            v = vfound;
        } else {
            //found more than one vehicle?
            if( v != vfound ) {
                v = nullptr;
                found_more_than_one = true;
                break;
            }
        }
    }
    if( found_more_than_one ) {
        std::optional<tripoint> pnt_ = choose_adjacent_highlight(
                                           _( "Siphon from where?" ), _( "There is nothing to siphon from nearby." ), f, false );
        if( !pnt_ ) {
            return std::nullopt;
        }
        const optional_vpart_position vp = here.veh_at( *pnt_ );
        if( vp ) {
            v = &vp->vehicle();
        }
    }

    if( v == nullptr ) {
        p->add_msg_if_player( m_info, _( "There's no vehicle there." ) );
        return std::nullopt;
    }
    act_vehicle_siphon( v );
    return 1;
}

std::optional<int> iuse::change_eyes( Character *p, item *, const tripoint & )
{
    if( p->is_avatar() ) {
        p->customize_appearance( customize_appearance_choice::EYES );
    }
    return std::nullopt;
}

std::optional<int> iuse::change_skin( Character *p, item *, const tripoint & )
{
    if( p->is_avatar() ) {
        p->customize_appearance( customize_appearance_choice::SKIN );
    }
    return std::nullopt;
}

static std::optional<int> dig_tool( Character *p, item *it, const tripoint &pos,
                                    const activity_id activity,
                                    const std::string &prompt, const std::string &fail, const std::string &success,
                                    int extra_moves = 0 )
{
    // use has_enough_charges to check for UPS availability
    // p is assumed to exist for iuse cases
    if( !it->ammo_sufficient( p ) ) {
        return std::nullopt;
    }
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    if( p->cant_do_underwater() ) {
        return std::nullopt;
    }

    tripoint pnt = pos;
    if( pos == p->pos() ) {
        const std::optional<tripoint> pnt_ = choose_adjacent( prompt );
        if( !pnt_ ) {
            return std::nullopt;
        }
        pnt = *pnt_;
    }

    map &here = get_map();
    const bool mineable_furn = here.has_flag_furn( ter_furn_flag::TFLAG_MINEABLE, pnt );
    const bool mineable_ter = here.has_flag_ter( ter_furn_flag::TFLAG_MINEABLE, pnt );
    const int max_mining_ability = 70;
    if( !mineable_furn && !mineable_ter ) {
        p->add_msg_if_player( m_info, fail );
        if( here.bash_resistance( pnt ) > max_mining_ability ) {
            p->add_msg_if_player( m_info,
                                  _( "The material is too hard for you to even make a dent." ) );
        }
        return std::nullopt;
    }
    if( here.veh_at( pnt ) ) {
        p->add_msg_if_player( _( "There's a vehicle in the way!" ) );
        return std::nullopt;
    }

    int moves = to_moves<int>( 30_minutes );

    const std::vector<Character *> helpers = p->get_crafting_helpers();
    const std::size_t helpersize = p->get_num_crafting_helpers( 3 );
    moves *= ( 1.0f - ( helpersize / 10.0f ) );
    for( std::size_t i = 0; i < helpersize; i++ ) {
        add_msg( m_info, _( "%s helps with this task…" ), helpers[i]->get_name() );
    }

    moves += extra_moves;

    if( here.move_cost( pnt ) == 2 ) {
        // We're breaking up some flat surface like pavement, which is much easier
        moves /= 2;
    }

    p->assign_activity( activity, moves );
    p->activity.targets.emplace_back( *p, it );
    p->activity.placement = here.getglobal( pnt );

    // You can mine either furniture or terrain, and furniture goes first,
    // according to @ref map::bash_ter_furn()
    std::string mining_what = mineable_furn ? here.furnname( pnt ) : here.tername( pnt );
    p->add_msg_if_player( success, mining_what, it->tname() );

    return 0; // handled when the activity finishes
}

std::optional<int> iuse::jackhammer( Character *p, item *it, const tripoint &pos )
{
    // use has_enough_charges to check for UPS availability
    // p is assumed to exist for iuse cases
    if( !it->ammo_sufficient( p ) ) {
        return std::nullopt;
    }

    return dig_tool( p, it, pos, ACT_JACKHAMMER,
                     _( "Drill where?" ), _( "You can't drill there." ),
                     _( "You start drilling into the %1$s with your %2$s." ) );

}

std::optional<int> iuse::pick_lock( Character *p, item *it, const tripoint &pos )
{
    if( p->is_npc() ) {
        return std::nullopt;
    }
    avatar &you = dynamic_cast<avatar &>( *p );

    std::optional<tripoint> target;
    // Prompt for a target lock to pick, or use the given tripoint
    if( pos == you.pos() ) {
        target = lockpick_activity_actor::select_location( you );
    } else {
        target = pos;
    }
    if( !target.has_value() ) {
        return std::nullopt;
    }

    int qual = it->get_quality( qual_LOCKPICK );
    if( qual < 1 ) {
        debugmsg( "Item %s with 'PICK_LOCK' use action requires LOCKPICK quality of at least 1.",
                  it->typeId().c_str() );
        qual = 1;
    }

    /** @EFFECT_DEX speeds up door lock picking */
    /** @EFFECT_LOCKPICK speeds up door lock picking */
    int duration_proficiency_factor = 10;

    if( you.has_proficiency( proficiency_prof_lockpicking ) ) {
        duration_proficiency_factor = 5;
    }
    if( you.has_proficiency( proficiency_prof_lockpicking_expert ) ) {
        duration_proficiency_factor = 1;
    }
    time_duration duration = 5_seconds;
    if( !it->has_flag( flag_PERFECT_LOCKPICK ) ) {
        duration = std::max( 30_seconds,
                             ( 10_minutes - time_duration::from_minutes( qual + static_cast<float>( you.dex_cur ) / 4.0f +
                                     you.get_skill_level( skill_traps ) ) ) * duration_proficiency_factor );
    }

    you.assign_activity( lockpick_activity_actor::use_item( to_moves<int>( duration ),
                         item_location( you, it ), get_map().getabs( *target ) ) );
    return 1;
}

std::optional<int> iuse::pickaxe( Character *p, item *it, const tripoint &pos )
{
    if( p->is_npc() ) {
        // Long action
        return std::nullopt;
    }
    /** @EFFECT_STR decreases time to dig with a pickaxe */
    int extra_moves = ( ( MAX_STAT + 4 ) -
                        std::min( p->get_arm_str(), MAX_STAT ) ) * to_moves<int>( 5_minutes );
    return dig_tool( p, it, pos, ACT_PICKAXE,
                     _( "Mine where?" ), _( "You can't mine there." ), _( "You strike the %1$s with your %2$s." ),
                     extra_moves );

}

std::optional<int> iuse::geiger( Character *p, item *it, const tripoint & )
{
    int ch = uilist( _( "Geiger counter:" ), {
        _( "Scan yourself or other person" ), _( "Scan the ground" ), _( "Turn continuous scan on" )
    } );
    creature_tracker &creatures = get_creature_tracker();
    switch( ch ) {
        case 0: {
            const std::function<bool( const tripoint & )> f = [&]( const tripoint & pnt ) {
                return creatures.creature_at<Character>( pnt ) != nullptr;
            };

            const std::optional<tripoint> pnt_ = choose_adjacent_highlight( _( "Scan whom?" ),
                                                 _( "There is no one to scan nearby." ), f, false );
            if( !pnt_ ) {
                return std::nullopt;
            }
            const tripoint &pnt = *pnt_;
            if( pnt == p->pos() ) {
                p->add_msg_if_player( m_info, _( "Your radiation level: %d mSv (%d mSv from items)" ), p->get_rad(),
                                      static_cast<int>( p->get_leak_level() ) );
                break;
            }
            if( npc *const person_ = creatures.creature_at<npc>( pnt ) ) {
                npc &person = *person_;
                p->add_msg_if_player( m_info, _( "%s's radiation level: %d mSv (%d mSv from items)" ),
                                      person.get_name(), person.get_rad(),
                                      static_cast<int>( person.get_leak_level() ) );
            }
            break;
        }
        case 1:
            p->add_msg_if_player( m_info, _( "The ground's radiation level: %d mSv/h" ),
                                  get_map().get_radiation( p->pos() ) );
            break;
        case 2:
            p->add_msg_if_player( _( "The geiger counter's scan LED turns on." ) );
            it->convert( itype_geiger_on, p ).active = true;
            break;
        default:
            return std::nullopt;
    }
    p->mod_moves( -100 );

    return 1;
}

std::optional<int> iuse::geiger_active( Character *, item *, const tripoint &pos )
{
    const int rads = get_map().get_radiation( pos );
    if( rads == 0 ) {
        return 1;
    }
    std::string description = rads > 50 ? _( "buzzing" ) :
                              rads > 25 ? _( "rapid clicking" ) : _( "clicking" );
    std::string sound_var = rads > 50 ? _( "geiger_high" ) :
                            rads > 25 ? _( "geiger_medium" ) : _( "geiger_low" );

    sounds::sound( pos, 6, sounds::sound_t::alarm, description, true, "tool", sound_var );
    if( !get_avatar().can_hear( pos, 6 ) ) {
        // can not hear it, but may have alarmed other creatures
        return 1;
    }
    if( rads > 50 ) {
        add_msg( m_warning, _( "The geiger counter buzzes intensely." ) );
    } else if( rads > 35 ) {
        add_msg( m_warning, _( "The geiger counter clicks wildly." ) );
    } else if( rads > 25 ) {
        add_msg( m_warning, _( "The geiger counter clicks rapidly." ) );
    } else if( rads > 15 ) {
        add_msg( m_warning, _( "The geiger counter clicks steadily." ) );
    } else if( rads > 8 ) {
        add_msg( m_warning, _( "The geiger counter clicks slowly." ) );
    } else if( rads > 4 ) {
        add_msg( _( "The geiger counter clicks intermittently." ) );
    } else {
        add_msg( _( "The geiger counter clicks once." ) );
    }
    return 1;
}

std::optional<int> iuse::teleport( Character *p, item *it, const tripoint & )
{
    if( p->is_npc() ) {
        // That would be evil
        return std::nullopt;
    }
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    if( !it->ammo_sufficient( p ) ) {
        return std::nullopt;
    }
    p->moves -= to_moves<int>( 1_seconds );
    teleport::teleport( *p );
    return 1;
}

std::optional<int> iuse::can_goo( Character *p, item *it, const tripoint & )
{
    it->convert( itype_canister_empty );
    int tries = 0;
    tripoint goop;
    goop.z = p->posz();
    map &here = get_map();
    do {
        goop.x = p->posx() + rng( -2, 2 );
        goop.y = p->posy() + rng( -2, 2 );
        tries++;
    } while( here.impassable( goop ) && tries < 10 );
    if( tries == 10 ) {
        return std::nullopt;
    }
    creature_tracker &creatures = get_creature_tracker();
    if( monster *const mon_ptr = creatures.creature_at<monster>( goop ) ) {
        monster &critter = *mon_ptr;
        add_msg_if_player_sees( goop, _( "Black goo emerges from the canister and envelopes the %s!" ),
                                critter.name() );
        critter.poly( mon_blob );

        critter.set_speed_base( critter.get_speed_base() - rng( 5, 25 ) );
        critter.set_hp( critter.get_speed() );
    } else {
        add_msg_if_player_sees( goop, _( "Living black goo emerges from the canister!" ) );
        if( monster *const goo = g->place_critter_at( mon_blob, goop ) ) {
            goo->friendly = -1;
        }
    }
    if( x_in_y( 3.0, 4.0 ) ) {
        tries = 0;
        bool found = false;
        do {
            goop.x = p->posx() + rng( -2, 2 );
            goop.y = p->posy() + rng( -2, 2 );
            tries++;
            found = here.passable( goop ) && here.tr_at( goop ).is_null();
        } while( !found && tries < 10 );
        if( found ) {
            add_msg_if_player_sees( goop, m_warning, _( "A nearby splatter of goo forms into a goo pit." ) );
            here.trap_set( goop, tr_goo );
        } else {
            return 0;
        }
    }
    return 1;
}

std::optional<int> iuse::granade_act( Character *, item *it, const tripoint &pos )
{
    if( pos.x == -999 || pos.y == -999 ) {
        return std::nullopt;
    }
    map &here = get_map();
    creature_tracker &creatures = get_creature_tracker();

    int explosion_radius = 3;
    int effect_roll = rng( 1, 4 );
    auto buff_stat = [&]( int &current_stat, int modify_by ) {
        int modified_stat = current_stat + modify_by;
        current_stat = std::max( current_stat, std::min( 15, modified_stat ) );
    };
    avatar &player_character = get_avatar();
    switch( effect_roll ) {
        case 1:
            sounds::sound( pos, 100, sounds::sound_t::electronic_speech, _( "BUGFIXES!" ),
                           true, "speech", it->typeId().str() );
            explosion_handler::draw_explosion( pos, explosion_radius, c_light_cyan );
            for( const tripoint &dest : here.points_in_radius( pos, explosion_radius ) ) {
                monster *const mon = creatures.creature_at<monster>( dest, true );
                if( mon && ( mon->type->in_species( species_INSECT ) || mon->is_hallucination() ) ) {
                    mon->die_in_explosion( nullptr );
                }
            }
            break;

        case 2:
            sounds::sound( pos, 100, sounds::sound_t::electronic_speech, _( "BUFFS!" ),
                           true, "speech", it->typeId().str() );
            explosion_handler::draw_explosion( pos, explosion_radius, c_green );
            for( const tripoint &dest : here.points_in_radius( pos, explosion_radius ) ) {
                if( monster *const mon_ptr = creatures.creature_at<monster>( dest ) ) {
                    monster &critter = *mon_ptr;
                    critter.set_speed_base(
                        critter.get_speed_base() * rng_float( 1.1, 2.0 ) );
                    critter.set_hp( critter.get_hp() * rng_float( 1.1, 2.0 ) );
                } else if( npc *const person = creatures.creature_at<npc>( dest ) ) {
                    /** @EFFECT_STR_MAX increases possible granade str buff for NPCs */
                    buff_stat( person->str_max, rng( 0, person->str_max / 2 ) );
                    /** @EFFECT_DEX_MAX increases possible granade dex buff for NPCs */
                    buff_stat( person->dex_max, rng( 0, person->dex_max / 2 ) );
                    /** @EFFECT_INT_MAX increases possible granade int buff for NPCs */
                    buff_stat( person->int_max, rng( 0, person->int_max / 2 ) );
                    /** @EFFECT_PER_MAX increases possible granade per buff for NPCs */
                    buff_stat( person->per_max, rng( 0, person->per_max / 2 ) );
                } else if( player_character.pos() == dest ) {
                    /** @EFFECT_STR_MAX increases possible granade str buff */
                    buff_stat( player_character.str_max, rng( 0, player_character.str_max / 2 ) );
                    /** @EFFECT_DEX_MAX increases possible granade dex buff */
                    buff_stat( player_character.dex_max, rng( 0, player_character.dex_max / 2 ) );
                    /** @EFFECT_INT_MAX increases possible granade int buff */
                    buff_stat( player_character.int_max, rng( 0, player_character.int_max / 2 ) );
                    /** @EFFECT_PER_MAX increases possible granade per buff */
                    buff_stat( player_character.per_max, rng( 0, player_character.per_max / 2 ) );
                    player_character.recalc_hp();
                    for( const bodypart_id &bp : player_character.get_all_body_parts(
                             get_body_part_flags::only_main ) ) {
                        player_character.set_part_hp_cur( bp, player_character.get_part_hp_cur( bp ) * rng_float( 1,
                                                          1.2 ) );
                        const int hp_max = player_character.get_part_hp_max( bp );
                        if( player_character.get_part_hp_cur( bp ) > hp_max ) {
                            player_character.set_part_hp_cur( bp, hp_max );
                        }
                    }
                }
            }
            break;

        case 3:
            sounds::sound( pos, 100, sounds::sound_t::electronic_speech, _( "NERFS!" ),
                           true, "speech", it->typeId().str() );
            explosion_handler::draw_explosion( pos, explosion_radius, c_red );
            for( const tripoint &dest : here.points_in_radius( pos, explosion_radius ) ) {
                if( monster *const mon_ptr = creatures.creature_at<monster>( dest ) ) {
                    monster &critter = *mon_ptr;
                    critter.set_speed_base(
                        rng( 0, critter.get_speed_base() ) );
                    critter.set_hp( rng( 1, critter.get_hp() ) );
                } else if( npc *const person = creatures.creature_at<npc>( dest ) ) {
                    /** @EFFECT_STR_MAX increases possible granade str debuff for NPCs (NEGATIVE) */
                    person->str_max -= rng( 0, person->str_max / 2 );
                    /** @EFFECT_DEX_MAX increases possible granade dex debuff for NPCs (NEGATIVE) */
                    person->dex_max -= rng( 0, person->dex_max / 2 );
                    /** @EFFECT_INT_MAX increases possible granade int debuff for NPCs (NEGATIVE) */
                    person->int_max -= rng( 0, person->int_max / 2 );
                    /** @EFFECT_PER_MAX increases possible granade per debuff for NPCs (NEGATIVE) */
                    person->per_max -= rng( 0, person->per_max / 2 );
                } else if( player_character.pos() == dest ) {
                    /** @EFFECT_STR_MAX increases possible granade str debuff (NEGATIVE) */
                    player_character.str_max -= rng( 0, player_character.str_max / 2 );
                    /** @EFFECT_DEX_MAX increases possible granade dex debuff (NEGATIVE) */
                    player_character.dex_max -= rng( 0, player_character.dex_max / 2 );
                    /** @EFFECT_INT_MAX increases possible granade int debuff (NEGATIVE) */
                    player_character.int_max -= rng( 0, player_character.int_max / 2 );
                    /** @EFFECT_PER_MAX increases possible granade per debuff (NEGATIVE) */
                    player_character.per_max -= rng( 0, player_character.per_max / 2 );
                    player_character.recalc_hp();
                    for( const bodypart_id &bp : player_character.get_all_body_parts(
                             get_body_part_flags::only_main ) ) {
                        const int hp_cur = player_character.get_part_hp_cur( bp );
                        if( hp_cur > 0 ) {
                            player_character.set_part_hp_cur( bp, rng( 1, hp_cur ) );
                        }
                    }
                }
            }
            break;

        case 4:
            sounds::sound( pos, 100, sounds::sound_t::electronic_speech, _( "REVERTS!" ),
                           true, "speech", it->typeId().str() );
            explosion_handler::draw_explosion( pos, explosion_radius, c_pink );
            for( const tripoint &dest : here.points_in_radius( pos, explosion_radius ) ) {
                if( monster *const mon_ptr = creatures.creature_at<monster>( dest ) ) {
                    monster &critter = *mon_ptr;
                    critter.set_speed_base( critter.type->speed );
                    critter.set_hp( critter.get_hp_max() );
                    critter.clear_effects();
                } else if( npc *const person = creatures.creature_at<npc>( dest ) ) {
                    person->environmental_revert_effect();
                } else if( player_character.pos() == dest ) {
                    player_character.environmental_revert_effect();
                    do_purify( player_character );
                }
            }
            break;
    }
    return 1;
}

std::optional<int> iuse::c4( Character *p, item *it, const tripoint & )
{
    int time;
    bool got_value = query_int( time, _( "Set the timer to how many seconds (0 to cancel)?" ) );
    if( !got_value || time <= 0 ) {
        p->add_msg_if_player( _( "Never mind." ) );
        return std::nullopt;
    }
    p->add_msg_if_player( n_gettext( "You set the timer to %d second.",
                                     "You set the timer to %d seconds.", time ), time );
    it->convert( itype_c4armed );
    it->countdown_point = calendar::turn + time_duration::from_seconds( time );
    it->active = true;
    return 1;
}

std::optional<int> iuse::acidbomb_act( Character *p, item *it, const tripoint &pos )
{
    if( !p ) {
        it->charges = -1;
        map &here = get_map();
        for( const tripoint &tmp : here.points_in_radius( pos, 1 ) ) {
            here.add_field( tmp, fd_acid, 3 );
        }
        return 1;
    }
    return std::nullopt;
}

std::optional<int> iuse::grenade_inc_act( Character *p, item *, const tripoint &pos )
{
    if( pos.x == -999 || pos.y == -999 ) {
        return std::nullopt;
    }

    map &here = get_map();
    int num_flames = rng( 3, 5 );
    for( int current_flame = 0; current_flame < num_flames; current_flame++ ) {
        tripoint dest( pos + point( rng( -5, 5 ), rng( -5, 5 ) ) );
        std::vector<tripoint> flames = line_to( pos, dest, 0, 0 );
        for( tripoint &flame : flames ) {
            here.add_field( flame, fd_fire, rng( 0, 2 ) );
        }
    }
    explosion_handler::explosion( p, pos, 8, 0.8, true );
    for( const tripoint &dest : here.points_in_radius( pos, 2 ) ) {
        here.add_field( dest, fd_incendiary, 3 );
    }

    avatar &player = get_avatar();
    if( player.has_trait( trait_PYROMANIA ) && player.sees( pos ) ) {
        player.add_morale( MORALE_PYROMANIA_STARTFIRE, 15, 15, 8_hours, 6_hours );
        player.rem_morale( MORALE_PYROMANIA_NOFIRE );
        add_msg( m_good, _( "Fire…  Good…" ) );
    }
    return 0;
}

std::optional<int> iuse::molotov_lit( Character *p, item *it, const tripoint &pos )
{

    if( !p ) {
        // It was thrown or dropped, so burst into flames
        map &here = get_map();
        for( const tripoint &pt : here.points_in_radius( pos, 1, 0 ) ) {
            const int intensity = 1 + one_in( 3 ) + one_in( 5 );
            here.add_field( pt, fd_fire, intensity );
        }
        avatar &player = get_avatar();
        if( player.has_trait( trait_PYROMANIA ) && player.sees( pos ) ) {
            player.add_morale( MORALE_PYROMANIA_STARTFIRE, 15, 15, 8_hours, 6_hours );
            player.rem_morale( MORALE_PYROMANIA_NOFIRE );
            add_msg( m_good, _( "Fire…  Good…" ) );
        }
        return 1;
    }

    // 20% chance of going out harmlessly.
    if( one_in( 5 ) ) {
        p->add_msg_if_player( _( "Your lit Molotov goes out." ) );
        it->convert( itype_molotov, p ).active = false;
    }
    return 0;
}

std::optional<int> iuse::firecracker_pack( Character *p, item *it, const tripoint & )
{
    if( p->cant_do_underwater() ) {
        return std::nullopt;
    }
    if( !p->has_charges( itype_fire, 1 ) ) {
        p->add_msg_if_player( m_info, _( "You need a source of fire!" ) );
        return std::nullopt;
    }
    p->add_msg_if_player( _( "You light the pack of firecrackers." ) );
    it->convert( itype_firecracker_pack_act, p );
    it->countdown_point = calendar::turn + 27_seconds;
    it->set_age( 0_turns );
    it->active = true;
    return 0; // don't use any charges at all. it has became a new item
}

std::optional<int> iuse::firecracker_pack_act( Character *, item *it, const tripoint &pos )
{
    // Two seconds of lit fuse burning
    // Followed by random number of explosions (4-6) per turn until 25 epxlosions have happened
    // Finally item despawns since countdown has ended
    if( it->age() < 2_seconds ) {
        sounds::sound( pos, 0, sounds::sound_t::alarm, _( "ssss…" ), true, "misc", "lit_fuse" );
    } else {
        // Time left to countdown_point is used to track of number of explosions
        int explosions = rng( 4, 6 );
        int i = 0;
        explosions = std::min( explosions, to_seconds<int>( it->countdown_point - calendar::turn ) );
        for( i = 0; i < explosions; i++ ) {
            sounds::sound( pos, 20, sounds::sound_t::combat, _( "Bang!" ), false, "explosion", "small" );
        }
        it->countdown_point -= ( explosions - 1 ) * 1_seconds;
    }
    return 0;
}

std::optional<int> iuse::firecracker( Character *p, item *it, const tripoint & )
{
    if( p->cant_do_underwater() ) {
        return std::nullopt;
    }
    if( !p->use_charges_if_avail( itype_fire, 1 ) ) {
        p->add_msg_if_player( m_info, _( "You need a source of fire!" ) );
        return std::nullopt;
    }
    p->add_msg_if_player( _( "You light the firecracker." ) );
    it->convert( itype_firecracker_act, p );
    it->countdown_point = calendar::turn + 2_seconds;
    it->active = true;
    return 1;
}

std::optional<int> iuse::mininuke( Character *p, item *it, const tripoint & )
{
    int time;
    bool got_value = query_int( time, _( "Set the timer to ___ turns (0 to cancel)?" ) );
    if( !got_value || time <= 0 ) {
        p->add_msg_if_player( _( "Never mind." ) );
        return std::nullopt;
    }
    p->add_msg_if_player( _( "You set the timer to %s." ),
                          to_string( time_duration::from_turns( time ) ) );
    get_event_bus().send<event_type::activates_mininuke>( p->getID() );
    it->convert( itype_mininuke_act, p );
    it->countdown_point = calendar::turn + time_duration::from_seconds( time );
    it->active = true;
    return 1;
}

std::optional<int> iuse::portal( Character *p, item *it, const tripoint & )
{
    if( !it->ammo_sufficient( p ) ) {
        return std::nullopt;
    }
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    tripoint t( p->posx() + rng( -2, 2 ), p->posy() + rng( -2, 2 ), p->posz() );
    get_map().trap_set( t, tr_portal );
    return 1;
}

std::optional<int> iuse::tazer( Character *p, item *it, const tripoint &pos )
{
    if( !it->ammo_sufficient( p ) ) {
        return std::nullopt;
    }

    tripoint pnt = pos;
    if( pos == p->pos() ) {
        const std::optional<tripoint> pnt_ = choose_adjacent( _( "Shock where?" ) );
        if( !pnt_ ) {
            return std::nullopt;
        }
        pnt = *pnt_;
    }

    if( pnt == p->pos() ) {
        p->add_msg_if_player( m_info, _( "Umm.  No." ) );
        return std::nullopt;
    }

    Creature *target = get_creature_tracker().creature_at( pnt, true );
    if( target == nullptr ) {
        p->add_msg_if_player( _( "There's nothing to zap there!" ) );
        return std::nullopt;
    }

    npc *foe = dynamic_cast<npc *>( target );
    if( foe != nullptr &&
        !foe->is_enemy() &&
        !p->query_yn( _( "Do you really want to shock %s?" ), target->disp_name() ) ) {
        return std::nullopt;
    }

    const float hit_roll = p->hit_roll();
    p->moves -= to_moves<int>( 1_seconds );

    const bool tazer_was_dodged = target->dodge_check( p->hit_roll() );
    const bool tazer_was_armored = hit_roll < target->get_armor_type( STATIC(
                                       damage_type_id( "bash" ) ), bodypart_id( "torso" ) );
    if( tazer_was_dodged ) {
        p->add_msg_player_or_npc( _( "You attempt to shock %s, but miss." ),
                                  _( "<npcname> attempts to shock %s, but misses." ),
                                  target->disp_name() );
    } else if( tazer_was_armored ) {
        p->add_msg_player_or_npc( _( "You attempt to shock %s, but are blocked by armor." ),
                                  _( "<npcname> attempts to shock %s, but is blocked by armor." ),
                                  target->disp_name() );
    } else {
        // Stun duration scales harshly inversely with big creatures
        if( target->get_size() == creature_size::tiny ) {
            target->moves -= rng( 150, 250 );
        } else if( target->get_size() == creature_size::small ) {
            target->moves -= rng( 125, 200 );
        } else if( target->get_size() == creature_size::large ) {
            target->moves -= rng( 95, 115 );
        } else if( target->get_size() == creature_size::huge ) {
            target->moves -= rng( 50, 80 );
        } else {
            target->moves -= rng( 110, 150 );
        }
        p->add_msg_player_or_npc( m_good,
                                  _( "You shock %s!" ),
                                  _( "<npcname> shocks %s!" ),
                                  target->disp_name() );
    }

    if( foe != nullptr ) {
        foe->on_attacked( *p );
    }

    return 1;
}

std::optional<int> iuse::tazer2( Character *p, item *it, const tripoint &pos )
{
    if( it->ammo_remaining( p, true ) >= 2 ) {
        // Instead of having a ctrl+c+v of the function above, spawn a fake tazer and use it
        // Ugly, but less so than copied blocks
        item fake( "tazer", calendar::turn_zero );
        fake.charges = 2;
        return tazer( p, &fake, pos );
    } else {
        p->add_msg_if_player( m_info, _( "Insufficient power" ) );
    }

    return std::nullopt;
}

std::optional<int> iuse::shocktonfa_off( Character *p, item *it, const tripoint &pos )
{
    if( !p ) {
        debugmsg( "%s called action shocktonfa_off that requires character but no character is present",
                  it->typeId().str() );
    }
    int choice = uilist( _( "tactical tonfa" ), {
        _( "Zap something" ), _( "Turn on light" )
    } );

    switch( choice ) {
        case 0: {
            return iuse::tazer2( p, it, pos );
        }
        case 1: {
            if( !it->ammo_sufficient( p ) ) {
                p->add_msg_if_player( m_info, _( "The batteries are dead." ) );
                return std::nullopt;
            } else {
                p->add_msg_if_player( _( "You turn the light on." ) );
                it->convert( itype_shocktonfa_on, p ).active = true;
                return 1;
            }
        }
    }
    return 0;
}

std::optional<int> iuse::shocktonfa_on( Character *p, item *it, const tripoint &pos )
{
    if( !p ) { // Effects while simply on
        debugmsg( "%s called action shocktonfa_on that requires character but no character is present",
                  it->typeId().str() );
    } else {
        if( !it->ammo_sufficient( p ) ) {
            p->add_msg_if_player( m_info, _( "Your tactical tonfa is out of power." ) );
            it->convert( itype_shocktonfa_off, p ).active = false;
        } else {
            int choice = uilist( _( "tactical tonfa" ), {
                _( "Zap something" ), _( "Turn off light" )
            } );

            switch( choice ) {
                case 0: {
                    return iuse::tazer2( p, it, pos );
                }
                case 1: {
                    p->add_msg_if_player( _( "You turn off the light." ) );
                    it->convert( itype_shocktonfa_off, p ).active = false;
                }
            }
        }
    }
    return 0;
}

std::optional<int> iuse::mp3( Character *p, item *it, const tripoint & )
{
    // TODO: avoid item id hardcoding to make this function usable for pure json-defined devices.
    if( !it->ammo_sufficient( p ) ) {
        p->add_msg_if_player( m_info, _( "The device's batteries are dead." ) );
    } else if( p->has_active_item( itype_mp3_on ) || p->has_active_item( itype_smartphone_music ) ||
               p->has_active_item( itype_afs_atomic_smartphone_music ) ||
               p->has_active_item( itype_afs_atomic_wraitheon_music ) ) {
        p->add_msg_if_player( m_info, _( "You are already listening to music!" ) );
    } else {
        p->add_msg_if_player( m_info, _( "You put in the earbuds and start listening to music." ) );
        if( it->typeId() == itype_mp3 ) {
            it->convert( itype_mp3_on, p ).active = true;
        } else if( it->typeId() == itype_smart_phone ) {
            it->convert( itype_smartphone_music, p ).active = true;
        } else if( it->typeId() == itype_afs_atomic_smartphone ) {
            it->convert( itype_afs_atomic_smartphone_music, p ).active = true;
        } else if( it->typeId() == itype_afs_wraitheon_smartphone ) {
            it->convert( itype_afs_atomic_wraitheon_music, p ).active = true;
        }
        p->mod_moves( -200 );
    }
    return 1;
}

static std::string get_music_description()
{
    const std::array<std::string, 5> descriptions = {{
            translate_marker( "a sweet guitar solo!" ),
            translate_marker( "a funky bassline." ),
            translate_marker( "some amazing vocals." ),
            translate_marker( "some pumping bass." ),
            translate_marker( "dramatic classical music." )
        }
    };

    if( one_in( 50 ) ) {
        return _( "some bass-heavy post-glam speed polka." );
    }

    size_t i = static_cast<size_t>( rng( 0, descriptions.size() * 2 ) );
    if( i < descriptions.size() ) {
        return _( descriptions[i] );
    }
    // Not one of the hard-coded versions, let's apply a random string made up
    // of snippets {a, b, c}, but only a 50% chance
    // Actual chance = 24.5% of being selected
    if( one_in( 2 ) ) {
        return SNIPPET.expand( SNIPPET.random_from_category( "<music_description>" ).value_or(
                                   translation() ).translated() );
    }

    return _( "a sweet guitar solo!" );
}

void iuse::play_music( Character *p, const tripoint &source, const int volume,
                       const int max_morale )
{
    // TODO: what about other "player", e.g. when a NPC is listening or when the PC is listening,
    // the other characters around should be able to profit as well.
    const bool do_effects = p && p->can_hear( source, volume ) && !p->in_sleep_state();
    std::string sound = "music";

    if( calendar::once_every( time_duration::from_minutes(
                                  get_option<int>( "DESCRIBE_MUSIC_FREQUENCY" ) ) ) ) {
        // Every X minutes, describe the music
        const std::string music = get_music_description();
        if( !music.empty() ) {
            sound = music;
            // descriptions aren't printed for sounds at our position
            if( do_effects && p->pos() == source ) {
                p->add_msg_if_player( _( "You listen to %s" ), music );
            }
        }
    }

    if( volume != 0 ) {
        sounds::ambient_sound( source, volume, sounds::sound_t::music, sound );
    }

    if( do_effects ) {
        p->add_effect( effect_music, 1_turns );
        p->add_morale( MORALE_MUSIC, 1, max_morale, 5_minutes, 2_minutes, true );
        // mp3 player reduces hearing
        if( volume == 0 ) {
            p->add_effect( effect_earphones, 1_turns );
        }
    }
}

std::optional<int> iuse::mp3_on( Character *p, item *, const tripoint &pos )
{
    // mp3 player in inventory, we can listen
    play_music( p, pos, 0, 20 );
    music::activate_music_id( music::music_id::mp3 );
    return 1;
}

std::optional<int> iuse::mp3_deactivate( Character *p, item *it, const tripoint & )
{

    if( it->typeId() == itype_mp3_on ) {
        p->add_msg_if_player( _( "The mp3 player turns off." ) );
        it->convert( itype_mp3, p ).active = false;
    } else if( it->typeId() == itype_smartphone_music ) {
        p->add_msg_if_player( _( "The phone turns off." ) );
        it->convert( itype_smart_phone, p ).active = false;
    } else if( it->typeId() == itype_afs_atomic_smartphone_music ) {
        p->add_msg_if_player( _( "The phone turns off." ) );
        it->convert( itype_afs_atomic_smartphone, p ).active = false;
    } else if( it->typeId() == itype_afs_atomic_wraitheon_music ) {
        p->add_msg_if_player( _( "The phone turns off." ) );
        it->convert( itype_afs_wraitheon_smartphone, p ).active = false;
    }
    p->mod_moves( -200 );
    music::deactivate_music_id( music::music_id::mp3 );

    return 0;

}

std::optional<int> iuse::rpgdie( Character *you, item *die, const tripoint & )
{
    if( you->cant_do_mounted() ) {
        return std::nullopt;
    }
    int num_sides = die->get_var( "die_num_sides", 0 );
    if( num_sides == 0 ) {
        const std::vector<int> sides_options = { 4, 6, 8, 10, 12, 20, 50 };
        const int sides = sides_options[ rng( 0, sides_options.size() - 1 ) ];
        num_sides = sides;
        die->set_var( "die_num_sides", sides );
    }
    const int roll = rng( 1, num_sides );
    //~ %1$d: roll number, %2$d: side number of a die, %3$s: die item name
    you->add_msg_if_player( pgettext( "dice", "You roll a %1$d on your %2$d sided %3$s" ), roll,
                            num_sides, die->tname() );
    if( roll == num_sides ) {
        add_msg( m_good, _( "Critical!" ) );
    }
    return roll;
}

std::optional<int> iuse::dive_tank( Character *p, item *it, const tripoint & )
{
    if( p && p->is_worn( *it ) ) {
        if( p->is_underwater() && p->oxygen < 10 ) {
            p->oxygen += 20;
        }
        if( one_in( 15 ) ) {
            p->add_msg_if_player( m_bad, _( "You take a deep breath from your %s." ), it->tname() );
        }
        if( it->ammo_remaining() == 0 ) {
            p->add_msg_if_player( m_bad, _( "Air in your %s runs out." ), it->tname() );
            it->erase_var( "overwrite_env_resist" );
            it->convert( *it->type->revert_to ).active = false;
        }
    } else { // not worn = off thanks to on-demand regulator
        it->erase_var( "overwrite_env_resist" );
        it->convert( *it->type->revert_to ).active = false;
    }

    return 1;
}

std::optional<int> iuse::dive_tank_activate( Character *p, item *it, const tripoint & )
{
    if( it->ammo_remaining() == 0 ) {
        p->add_msg_if_player( _( "Your %s is empty." ), it->tname() );
    } else if( it->active ) { //off
        p->add_msg_if_player( _( "You turn off the regulator and close the air valve." ) );
        it->erase_var( "overwrite_env_resist" );
        it->convert( *it->type->revert_to ).active = false;
    } else { //on
        if( !p->is_worn( *it ) ) {
            p->add_msg_if_player( _( "You should wear it first." ) );
        } else {
            p->add_msg_if_player( _( "You turn on the regulator and open the air valve." ) );
            it->set_var( "overwrite_env_resist", it->get_base_env_resist_w_filter() );
            it->convert( itype_id( it->typeId().str() + "_on" ) ).active = true;
        }
    }
    return 1;
}

std::optional<int> iuse::solarpack( Character *p, item *it, const tripoint & )
{
    if( !p ) {
        debugmsg( "%s called action solarpack that requires character but no character is present",
                  it->typeId().str() );
        return std::nullopt;
    }
    const bionic_id rem_bid = p->get_remote_fueled_bionic();
    if( rem_bid.is_empty() ) {  // Cable CBM required
        p->add_msg_if_player(
            _( "You have no cable charging system to plug it into, so you leave it alone." ) );
        return std::nullopt;
    } else if( !p->has_active_bionic( rem_bid ) ) {  // when OFF it takes no effect
        p->add_msg_if_player( _( "Activate your cable charging system to take advantage of it." ) );
    }

    if( it->is_armor() && !p->is_worn( *it ) ) {
        p->add_msg_if_player( m_neutral, _( "You need to wear the %1$s before you can unfold it." ),
                              it->tname() );
        return std::nullopt;
    }
    // no doubled sources of power
    if( p->worn_with_flag( flag_SOLARPACK_ON ) ) {
        p->add_msg_if_player( m_neutral, _( "You can't use the %1$s with another of its kind." ),
                              it->tname() );
        return std::nullopt;
    }
    p->add_msg_if_player(
        _( "You unfold the solar array from the pack.  You still need to connect it with a cable." ) );

    it->convert( itype_id( it->typeId().str() + "_on" ), p );
    return 0;
}

std::optional<int> iuse::solarpack_off( Character *p, item *it, const tripoint & )
{
    if( !p ) {
        debugmsg( "%s called action solarpack_off that requires character but no character is present",
                  it->typeId().str() );
        return std::nullopt;
    }
    if( !p->is_worn( *it ) ) {  // folding when not worn
        p->add_msg_if_player( _( "You fold your portable solar array into the pack." ) );
    } else {
        p->add_msg_if_player( _( "You unplug your portable solar array, and fold it into the pack." ) );
    }

    it->erase_var( "cable" );

    // 3 = "_on"
    it->convert( itype_id( it->typeId().str().substr( 0,
                           it->typeId().str().size() - 3 ) ), p ).active = false;
    p->process_items(); // Process carried items to disconnect any connected cables
    return 0;
}

std::optional<int> iuse::gasmask_activate( Character *p, item *it, const tripoint & )
{
    if( it->ammo_remaining() == 0 ) {
        p->add_msg_if_player( _( "Your %s doesn't have a filter." ), it->tname() );
    } else {
        p->add_msg_if_player( _( "You prepare your %s." ), it->tname() );
        it->active = true;
        it->set_var( "overwrite_env_resist", it->get_base_env_resist_w_filter() );
    }

    return 0;
}

std::optional<int> iuse::gasmask( Character *p, item *it, const tripoint &pos )
{
    if( p && p->is_worn( *it ) ) {
        // calculate amount of absorbed gas per filter charge
        const field &gasfield = get_map().field_at( pos );
        for( const auto &dfield : gasfield ) {
            const field_entry &entry = dfield.second;
            int gas_abs_factor = to_turns<int>( entry.get_field_type()->gas_absorption_factor );
            // Not set, skip this field
            if( gas_abs_factor == 0 ) {
                continue;
            }
            const field_intensity_level &int_level = entry.get_intensity_level();
            // 6000 is the amount of "gas absorbed" charges in a full 100 capacity gas mask cartridge.
            // factor/concentration gives an amount of seconds the cartidge is expected to last in current conditions.
            /// 6000/that is the amount of "gas absorbed" charges to tick up every second in order to reach that number.
            float gas_absorbed = 6000 / ( static_cast<float>( gas_abs_factor ) / static_cast<float>
                                          ( int_level.concentration ) );
            add_msg_debug( debugmode::DF_IUSE, "Absorbing %g/60 from field: 6000 / (%d * %d)", gas_absorbed,
                           gas_abs_factor, int_level.concentration );
            if( gas_absorbed > 0 ) {
                it->set_var( "gas_absorbed", it->get_var( "gas_absorbed", 0 ) + gas_absorbed );
            }
        }
        if( it->get_var( "gas_absorbed", 0 ) >= 60 ) {
            it->ammo_consume( 1, pos, p );
            it->set_var( "gas_absorbed", 0 );
            if( it->ammo_remaining() < 10 ) {
                p->add_msg_player_or_npc(
                    m_bad,
                    _( "Your %s is getting hard to breathe in!" ),
                    _( "<npcname>'s gas mask is getting hard to breathe in!" )
                    , it->tname() );
            }
        }
        if( it->ammo_remaining() == 0 ) {
            p->add_msg_player_or_npc(
                m_bad,
                _( "Your %s requires new filters!" ),
                _( "<npcname> needs new gas mask filters!" )
                , it->tname() );
        }
    }

    if( it->ammo_remaining() == 0 ) {
        it->set_var( "overwrite_env_resist", 0 );
        it->active = false;
    }
    return 0;
}

std::optional<int> iuse::portable_game( Character *p, item *it, const tripoint & )
{
    if( p->is_npc() ) {
        // Long action
        return std::nullopt;
    }
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    if( p->cant_do_underwater() ) {
        return std::nullopt;
    }
    if( p->has_trait( trait_ILLITERATE ) ) {
        p->add_msg_if_player( m_info, _( "You're illiterate!" ) );
        return std::nullopt;
    } else if( it->typeId() != itype_arcade_machine && !it->ammo_sufficient( p ) ) {
        p->add_msg_if_player( m_info, _( "The %s's batteries are dead." ), it->tname() );
        return std::nullopt;
    } else {
        std::string loaded_software = "robot_finds_kitten";
        // number of nearby friends with gaming devices
        std::vector<npc *> friends_w_game = g->get_npcs_if( [&it, p]( const npc & n ) {
            return n.is_player_ally() && p->sees( n ) &&
                   n.can_hear( p->pos(), p->get_shout_volume() ) &&
            n.has_item_with( [&it]( const item & i ) {
                return i.typeId() == it->typeId() && i.ammo_sufficient( nullptr );
            } );
        } );

        uilist as_m;
        as_m.text = _( "What do you want to play?" );
        as_m.entries.emplace_back( 1, true, '1', _( "robotfindskitten" ) );
        as_m.entries.emplace_back( 2, true, '2', _( "S N A K E" ) );
        as_m.entries.emplace_back( 3, true, '3', _( "Sokoban" ) );
        as_m.entries.emplace_back( 4, true, '4', _( "Minesweeper" ) );
        as_m.entries.emplace_back( 5, true, '5', _( "Lights on!" ) );
        if( friends_w_game.empty() ) {
            as_m.entries.emplace_back( 6, true, '6', _( "Play anything for a while" ) );
        } else {
            as_m.entries.emplace_back( 6, true, '6', _( "Play something with friends" ) );
            as_m.entries.emplace_back( 7, true, '7', _( "Play something alone" ) );
        }
        as_m.query();

        bool w_friends = false;
        switch( as_m.ret ) {
            case 1:
                loaded_software = "robot_finds_kitten";
                break;
            case 2:
                loaded_software = "snake_game";
                break;
            case 3:
                loaded_software = "sokoban_game";
                break;
            case 4:
                loaded_software = "minesweeper_game";
                break;
            case 5:
                loaded_software = "lightson_game";
                break;
            case 6:
                loaded_software = "null";
                w_friends = !friends_w_game.empty();
                break;
            case 7: {
                if( friends_w_game.empty() ) {
                    return std::nullopt;
                }
                loaded_software = "null";
            }
            break;
            default:
                //Cancel
                return std::nullopt;
        }

        //Play in 15-minute chunks
        const int moves = to_moves<int>( 15_minutes );
        size_t num_friends = w_friends ? friends_w_game.size() : 0;
        int winner = rng( 0, num_friends );
        if( winner == 0 ) {
            winner = get_player_character().getID().get_value();
        } else {
            winner = friends_w_game[winner - 1]->getID().get_value();
        }
        player_activity game_act( ACT_GENERIC_GAME, to_moves<int>( 1_hours ), num_friends,
                                  p->get_item_position( it ), w_friends ? "gaming with friends" : "gaming" );
        game_act.values.emplace_back( winner );

        if( w_friends ) {
            std::string it_name = it->type_name( num_friends + 1 );
            if( num_friends > 1 ) {
                p->add_msg_if_player( _( "You and your %1$u friends play on your %2$s for a while." ), num_friends,
                                      it_name );
            } else {
                p->add_msg_if_player( _( "You and your friend play on your %s for a while." ), it_name );
            }
            for( npc *n : friends_w_game ) {
                std::vector<item *> nit = n->cache_get_items_with( it->typeId(), []( const item & i ) {
                    return i.ammo_sufficient( nullptr );
                } );
                n->assign_activity( game_act );
                n->activity.targets.emplace_back( *n, nit.front() );
                n->activity.position = n->get_item_position( nit.front() );
            }
        } else {
            p->add_msg_if_player( _( "You play on your %s for a while." ), it->tname() );
        }
        if( loaded_software == "null" ) {
            p->assign_activity( game_act );
            p->activity.targets.emplace_back( *p, it );
            return 0;
        }
        p->assign_activity( ACT_GAME, moves, -1, 0, "gaming" );
        p->activity.targets.emplace_back( *p, it );
        std::map<std::string, std::string> game_data;
        game_data.clear();
        int game_score = 0;

        play_videogame( loaded_software, game_data, game_score );

        if( game_data.find( "end_message" ) != game_data.end() ) {
            p->add_msg_if_player( game_data["end_message"] );
        }

        if( game_score != 0 ) {
            if( game_data.find( "moraletype" ) != game_data.end() ) {
                std::string moraletype = game_data.find( "moraletype" )->second;
                if( moraletype == "MORALE_GAME_FOUND_KITTEN" ) {
                    p->add_morale( MORALE_GAME_FOUND_KITTEN, game_score, 110 );
                } /*else if ( ...*/
            } else {
                p->add_morale( MORALE_GAME, game_score, 110 );
            }
        }

    }
    return 0;
}

std::optional<int> iuse::fitness_check( Character *p, item *it, const tripoint & )
{
    if( p->has_trait( trait_ILLITERATE ) ) {
        p->add_msg_if_player( m_info, _( "You don't know what you're looking at." ) );
        return std::nullopt;
    } else {
        //What else should block using f-band?
        std::string msg;
        msg.append( "***  " );
        msg.append( string_format( _( "You check your health metrics on your %s." ), it->tname( 1,
                                   false ) ) );
        msg.append( "  ***\n\n" );
        const int bpm = p->heartrate_bpm();
        msg.append( "-> " );
        msg.append( string_format( _( "Your heart rate is %i bpm." ), bpm ) );
        if( bpm > 179 ) {
            msg.append( "\n" );
            msg.append( "-> " );
            msg.append( _( "WARNING!  Slow down!  Your pulse is getting too high, champion!" ) );
        }
        const std::string exercise = p->activity_level_str();
        msg.append( "\n" );
        msg.append( "-> " );
        if( exercise == "NO_EXERCISE" ) {
            msg.append( _( "You haven't really been active today.  Try going for a walk!" ) );
        } else if( exercise == "LIGHT_EXERCISE" ) {
            msg.append( _( "Good start!  Keep it up and move more." ) );
        } else if( exercise == "MODERATE_EXERCISE" ) {
            msg.append( _( "Doing good!  Don't stop, push the limit!" ) );
        } else if( exercise == "ACTIVE_EXERCISE" ) {
            msg.append( _( "Great job!  Take a break and don't forget about hydration!" ) );
        } else {
            msg.append( _( "You are too active!  Avoid overexertion for your safety and health." ) );
        }
        msg.append( "\n" );
        msg.append( "-> " );
        msg.append( string_format( _( "You consumed %d kcal today and %d kcal yesterday." ),
                                   p->as_avatar()->get_daily_ingested_kcal( false ),
                                   p->as_avatar()->get_daily_ingested_kcal( true ) ) );
        msg.append( "\n" );
        msg.append( "-> " );
        msg.append( string_format( _( "You burned %d kcal today and %d kcal yesterday." ),
                                   p->as_avatar()->get_daily_spent_kcal( false ),
                                   p->as_avatar()->get_daily_spent_kcal( true ) ) );
        //TODO add whatever else makes sense (steps, sleep quality, health level approximation?)
        p->add_msg_if_player( m_neutral, msg );
        popup( msg );
    }
    return 1;
}

std::optional<int> iuse::hand_crank( Character *p, item *it, const tripoint & )
{
    if( p->is_npc() ) {
        // Long action
        return std::nullopt;
    }
    if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _( "It's not waterproof enough to work underwater." ) );
        return std::nullopt;
    }
    if( p->get_fatigue() >= fatigue_levels::DEAD_TIRED ) {
        p->add_msg_if_player( m_info, _( "You're too exhausted to keep cranking." ) );
        return std::nullopt;
    }
    item *magazine = it->magazine_current();
    if( magazine && magazine->has_flag( flag_RECHARGE ) ) {
        // 1600 minutes. It shouldn't ever run this long, but it's an upper bound.
        // expectation is it runs until the player is too tired.
        int moves = to_moves<int>( 1600_minutes );
        if( it->ammo_capacity( ammo_battery ) > it->ammo_remaining() ) {
            p->add_msg_if_player( _( "You start cranking the %s to charge its %s." ), it->tname(),
                                  it->magazine_current()->tname() );
            p->assign_activity( ACT_HAND_CRANK, moves, -1, 0, "hand-cranking" );
            p->activity.targets.emplace_back( *p, it );
        } else {
            p->add_msg_if_player( _( "You could use the %s to charge its %s, but it's already charged." ),
                                  it->tname(), magazine->tname() );
        }
    } else {
        p->add_msg_if_player( m_info, _( "You need a rechargeable battery cell to charge." ) );
    }
    return 0;
}

std::optional<int> iuse::vibe( Character *p, item *it, const tripoint & )
{
    if( p->is_npc() ) {
        // Long action
        // Also, that would be creepy as fuck, seriously
        return std::nullopt;
    }
    if( p->is_mounted() ) {
        p->add_msg_if_player( m_info, _( "You can't do… that while mounted." ) );
        return std::nullopt;
    }
    if( p->is_underwater() && ( !( p->has_trait( trait_GILLS ) ||
                                   p->is_wearing( itype_rebreather_on ) ||
                                   p->is_wearing( itype_rebreather_xl_on ) ||
                                   p->is_wearing( itype_mask_h20survivor_on ) ) ) ) {
        p->add_msg_if_player( m_info, _( "It might be waterproof, but your lungs aren't." ) );
        return std::nullopt;
    }
    if( !it->ammo_sufficient( p ) ) {
        p->add_msg_if_player( m_info, _( "The %s's batteries are dead." ), it->tname() );
        return std::nullopt;
    }
    if( p->get_fatigue() >= fatigue_levels::DEAD_TIRED ) {
        p->add_msg_if_player( m_info, _( "*Your* batteries are dead." ) );
        return std::nullopt;
    } else {
        int moves = to_moves<int>( 20_minutes );
        if( it->ammo_remaining() > 0 ) {
            p->add_msg_if_player( _( "You fire up your %s and start getting the tension out." ),
                                  it->tname() );
        } else {
            p->add_msg_if_player( _( "You whip out your %s and start getting the tension out." ),
                                  it->tname() );
        }
        p->assign_activity( ACT_VIBE, moves, -1, 0, "de-stressing" );
        p->activity.targets.emplace_back( *p, it );
    }
    return 1;
}

std::optional<int> iuse::vortex( Character *p, item *it, const tripoint & )
{
    std::vector<point> spawn;
    for( int i = -3; i <= 3; i++ ) {
        spawn.emplace_back( -3, i );
        spawn.emplace_back( +3, i );
        spawn.emplace_back( i, -3 );
        spawn.emplace_back( i, +3 );
    }

    while( !spawn.empty() ) {
        const tripoint offset( random_entry_removed( spawn ), 0 );
        monster *const mon = g->place_critter_at( mon_vortex, offset + p->pos() );
        if( !mon ) {
            continue;
        }
        p->add_msg_if_player( m_warning, _( "Air swirls all over…" ) );
        p->moves -= to_moves<int>( 1_seconds );
        it->convert( itype_spiral_stone, p );
        mon->friendly = -1;
        return 1;
    }

    // Only reachable when no monster has been spawned.
    p->add_msg_if_player( m_warning, _( "Air swirls around you for a moment." ) );
    return 0;
}

std::optional<int> iuse::dog_whistle( Character *p, item *, const tripoint & )
{
    if( !p->is_avatar() ) {
        return std::nullopt;
    }
    if( p->cant_do_underwater() ) {
        return std::nullopt;
    }
    p->add_msg_if_player( _( "You blow your dog whistle." ) );

    // Can the Character hear the dog whistle?
    auto hearing_check = [p]( const Character & who ) -> bool {
        return !who.is_deaf() && p->sees( who ) &&
        who.has_trait( STATIC( trait_id( "THRESH_LUPINE" ) ) );
    };

    for( const npc &subject : g->all_npcs() ) {
        if( !( one_in( 3 ) && hearing_check( subject ) ) ) {
            continue;
        }

        std::optional<translation> npc_message;

        if( p->attitude_to( subject ) == Creature::Attitude::HOSTILE ) {
            npc_message = SNIPPET.random_from_category( "dogwhistle_message_npc_hostile" );
        } else {
            npc_message = SNIPPET.random_from_category( "dogwhistle_message_npc_not_hostile" );
        }

        if( npc_message ) {
            subject.say( npc_message.value().translated() );
        }
    }

    if( hearing_check( *p ) && one_in( 3 ) ) {
        std::optional<translation> your_message = SNIPPET.random_from_category( "dogwhistle_message_you" );
        if( your_message ) {
            p->add_msg_if_player( m_info, your_message.value().translated() );
        }
    }

    for( monster &critter : g->all_monsters() ) {
        if( critter.friendly != 0 && critter.has_flag( mon_flag_DOGFOOD ) ) {
            bool u_see = get_player_view().sees( critter );
            if( critter.has_effect( effect_docile ) ) {
                if( u_see ) {
                    p->add_msg_if_player( _( "Your %s looks ready to attack." ), critter.name() );
                }
                critter.remove_effect( effect_docile );
            } else {
                if( u_see ) {
                    p->add_msg_if_player( _( "Your %s goes docile." ), critter.name() );
                }
                critter.add_effect( effect_docile, 1_turns, true );
            }
        }
    }
    return 1;
}

std::optional<int> iuse::call_of_tindalos( Character *p, item *, const tripoint & )
{
    map &here = get_map();
    for( const tripoint &dest : here.points_in_radius( p->pos(), 12 ) ) {
        if( here.is_cornerfloor( dest ) ) {
            here.add_field( dest, fd_tindalos_rift, 3 );
            add_msg( m_info, _( "You hear a low-pitched echoing howl." ) );
        }
    }
    return 1;
}

std::optional<int> iuse::blood_draw( Character *p, item *it, const tripoint & )
{
    if( p->is_npc() ) {
        return std::nullopt;    // No NPCs for now!
    }
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    if( !it->empty() ) {
        p->add_msg_if_player( m_info, _( "That %s is full!" ), it->tname() );
        return std::nullopt;
    }

    item blood( "blood", calendar::turn );
    bool drew_blood = false;
    bool acid_blood = false;
    bool vampire = false;
    units::temperature blood_temp = units::from_kelvin( -1.0f ); //kelvins
    for( item &map_it : get_map().i_at( point( p->posx(), p->posy() ) ) ) {
        if( map_it.is_corpse() &&
            query_yn( _( "Draw blood from %s?" ),
                      colorize( map_it.tname(), map_it.color_in_inventory() ) ) ) {
            p->add_msg_if_player( m_info, _( "You drew blood from the %s…" ), map_it.tname() );
            drew_blood = true;
            blood_temp = map_it.temperature;

            field_type_id bloodtype( map_it.get_mtype()->bloodType() );
            if( bloodtype.obj().has_acid ) {
                acid_blood = true;
            } else {
                blood.set_mtype( map_it.get_mtype() );

                for( const harvest_entry &entry : map_it.get_mtype()->harvest.obj() ) {
                    if( entry.type == harvest_drop_blood ) {
                        blood.convert( itype_id( entry.drop ) );
                        break;
                    }
                }
            }
        }
    }

    if( !drew_blood && query_yn( _( "Draw your own blood?" ) ) ) {
        p->add_msg_if_player( m_info, _( "You drew your own blood…" ) );
        drew_blood = true;
        blood_temp = units::from_celsius( 37 );
        if( p->has_trait( trait_ACIDBLOOD ) ) {
            acid_blood = true;
        }
        if( p->has_trait( trait_VAMPIRE ) ) {
            vampire = true;
        }
        // From wikipedia,
        // "To compare, this (volume of blood loss that causes death) is five to eight times
        // as much blood as people usually give in a blood donation.[2]"
        // This is half a TU, hence I'm setting it to 1/10th of a lethal exsanguination.
        p->vitamin_mod( vitamin_redcells, vitamin_redcells->min() / 10 );
        p->vitamin_mod( vitamin_blood, vitamin_blood->min() / 10 );
        p->mod_pain( 3 );
    }

    if( acid_blood ) {
        item acid( "blood_acid", calendar::turn );
        acid.set_item_temperature( blood_temp );
        it->put_in( acid, pocket_type::CONTAINER );
        if( one_in( 3 ) ) {
            if( it->inc_damage() ) {
                p->add_msg_if_player( m_info, _( "…but acidic blood melts the %s, destroying it!" ),
                                      it->tname() );
                p->i_rem( it );
                return 0;
            }
            p->add_msg_if_player( m_info, _( "…but acidic blood damages the %s!" ), it->tname() );
        }
    }

    if( vampire ) {
        p->vitamin_mod( vitamin_human_blood_vitamin, -500 );
    }

    if( !drew_blood ) {
        return std::nullopt;;
    }

    blood.set_item_temperature( blood_temp );
    it->put_in( blood, pocket_type::CONTAINER );
    p->mod_moves( -to_moves<int>( 5_seconds ) );
    return 1;
}

void iuse::cut_log_into_planks( Character &p )
{
    if( p.cant_do_mounted() ) {
        return;
    }
    const int moves = to_moves<int>( 20_minutes );
    p.add_msg_if_player( _( "You cut the log into planks." ) );

    p.assign_activity( chop_planks_activity_actor( moves ) );
    p.activity.placement = get_map().getglobal( p.pos() );
}

std::optional<int> iuse::lumber( Character *p, item *it, const tripoint & )
{
    if( !p ) {
        debugmsg( "%s called action lumber that requires character but no character is present",
                  it->typeId().str() );
        return std::nullopt;
    }
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    map &here = get_map();
    // Check if player is standing on any lumber
    for( item &i : here.i_at( p->pos() ) ) {
        if( i.typeId() == itype_log ) {
            here.i_rem( p->pos(), &i );
            cut_log_into_planks( *p );
            return 1;
        }
    }

    // If the player is not standing on a log, check inventory
    avatar *you = p->as_avatar();
    item_location loc;
    auto filter = []( const item & it ) {
        return it.typeId() == itype_log;
    };
    if( you != nullptr ) {
        loc = game_menus::inv::titled_filter_menu( filter, *you, _( "Cut up what?" ) );
    }

    if( !loc ) {
        p->add_msg_if_player( m_info, _( "You don't have that item!" ) );
        return std::nullopt;
    }
    p->i_rem( &*loc );
    cut_log_into_planks( *p );
    return 1;
}

static int chop_moves( Character *p, item *it )
{
    // quality of tool
    const int quality = it->get_quality( qual_AXE );

    // attribute; regular tools - based on STR, powered tools - based on DEX
    const int attr = it->has_flag( flag_POWERED ) ? p->dex_cur : p->get_arm_str();

    int moves = to_moves<int>( time_duration::from_minutes( 60 - attr ) / std::pow( 2, quality - 1 ) );
    const int helpersize = p->get_num_crafting_helpers( 3 );
    moves *= ( 1.0f - ( helpersize / 10.0f ) );
    return moves;
}

std::optional<int> iuse::chop_tree( Character *p, item *it, const tripoint & )
{
    if( !p ) {
        debugmsg( "%s called action chop_tree that requires character but no character is present",
                  it->typeId().str() );
        return std::nullopt;
    }
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    map &here = get_map();
    const std::function<bool( const tripoint & )> f = [&here, p]( const tripoint & pnt ) {
        if( pnt == p->pos() ) {
            return false;
        }
        return here.has_flag( ter_furn_flag::TFLAG_TREE, pnt );
    };

    const std::optional<tripoint> pnt_ = choose_adjacent_highlight(
            _( "Chop down which tree?" ), _( "There is no tree to chop down nearby." ), f, false );
    if( !pnt_ ) {
        return std::nullopt;
    }
    const tripoint &pnt = *pnt_;
    if( !f( pnt ) ) {
        if( pnt == p->pos() ) {
            p->add_msg_if_player( m_info, _( "You're not stern enough to shave yourself with THIS." ) );
        } else {
            p->add_msg_if_player( m_info, _( "You can't chop down that." ) );
        }
        return std::nullopt;
    }
    int moves = chop_moves( p, it );
    const std::vector<Character *> helpers = p->get_crafting_helpers();
    for( std::size_t i = 0; i < helpers.size() && i < 3; i++ ) {
        add_msg( m_info, _( "%s helps with this task…" ), helpers[i]->get_name() );
    }
    p->assign_activity( chop_tree_activity_actor( moves, item_location( *p, it ) ) );
    p->activity.placement = here.getglobal( pnt );

    return 1;
}

std::optional<int> iuse::chop_logs( Character *p, item *it, const tripoint & )
{
    if( !p ) {
        debugmsg( "%s called action chop_logs that requires character but no character is present",
                  it->typeId().str() );
        return std::nullopt;
    }
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }

    const std::set<ter_id> allowed_ter_id {
        t_trunk,
        t_stump
    };
    map &here = get_map();
    const std::function<bool( const tripoint & )> f = [&allowed_ter_id, &here]( const tripoint & pnt ) {
        const ter_id type = here.ter( pnt );
        const bool is_allowed_terrain = allowed_ter_id.find( type ) != allowed_ter_id.end();
        return is_allowed_terrain;
    };

    const std::optional<tripoint> pnt_ = choose_adjacent_highlight(
            _( "Chop which tree trunk?" ), _( "There is no tree trunk to chop nearby." ), f, false );
    if( !pnt_ ) {
        return std::nullopt;
    }
    const tripoint &pnt = *pnt_;
    if( !f( pnt ) ) {
        p->add_msg_if_player( m_info, _( "You can't chop that." ) );
        return std::nullopt;
    }

    int moves = chop_moves( p, it );
    const std::vector<Character *> helpers = p->get_crafting_helpers();
    for( std::size_t i = 0; i < helpers.size() && i < 3; i++ ) {
        add_msg( m_info, _( "%s helps with this task…" ), helpers[i]->get_name() );
    }
    p->assign_activity( chop_logs_activity_actor( moves, item_location( *p, it ) ) );
    p->activity.placement = here.getglobal( pnt );

    return 1;
}

std::optional<int> iuse::oxytorch( Character *p, item *it, const tripoint & )
{
    if( p->is_npc() ) {
        // Long action
        return std::nullopt;
    }
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    if( !p->has_quality( qual_GLARE, 1 ) ) {
        p->add_msg_if_player( m_info, _( "You need welding goggles to do that." ) );
        return std::nullopt;
    }

    map &here = get_map();
    const std::function<bool( const tripoint & )> f =
    [&here, p]( const tripoint & pnt ) {
        if( pnt == p->pos() ) {
            return false;
        } else if( here.has_furn( pnt ) ) {
            return here.furn( pnt )->oxytorch->valid();
        } else if( !here.ter( pnt )->is_null() ) {
            return here.ter( pnt )->oxytorch->valid();
        }
        return false;
    };

    const std::optional<tripoint> pnt_ = choose_adjacent_highlight(
            _( "Cut up metal where?" ), _( "There is no metal to cut up nearby." ), f, false );
    if( !pnt_ ) {
        return std::nullopt;
    }
    const tripoint &pnt = *pnt_;
    if( !f( pnt ) ) {
        if( pnt == p->pos() ) {
            p->add_msg_if_player( m_info, _( "Yuck.  Acetylene gas smells weird." ) );
        } else {
            p->add_msg_if_player( m_info, _( "You can't cut that." ) );
        }
        return std::nullopt;
    }

    p->assign_activity( oxytorch_activity_actor( pnt, item_location{*p, it} ) );

    return std::nullopt;
}

std::optional<int> iuse::hacksaw( Character *p, item *it, const tripoint &it_pnt )
{
    if( !p ) {
        debugmsg( "%s called action hacksaw that requires character but no character is present",
                  it->typeId().str() );
        return std::nullopt;
    }
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }

    map &here = get_map();
    const std::function<bool( const tripoint & )> f =
    [&here, p]( const tripoint & pnt ) {
        if( pnt == p->pos() ) {
            return false;
        } else if( here.has_furn( pnt ) ) {
            return here.furn( pnt )->hacksaw->valid();
        } else if( !here.ter( pnt )->is_null() ) {
            return here.ter( pnt )->hacksaw->valid();
        }
        return false;
    };

    const std::optional<tripoint> pnt_ = choose_adjacent_highlight(
            _( "Cut up metal where?" ), _( "There is no metal to cut up nearby." ), f, false );
    if( !pnt_ ) {
        return std::nullopt;
    }
    const tripoint &pnt = *pnt_;
    if( !f( pnt ) ) {
        if( pnt == p->pos() ) {
            p->add_msg_if_player( m_info, _( "Why would you do that?" ) );
            p->add_msg_if_player( m_info, _( "You're not even chained to a boiler." ) );
        } else {
            p->add_msg_if_player( m_info, _( "You can't cut that." ) );
        }
        return std::nullopt;
    }
    if( p->pos() == it_pnt ) {
        p->assign_activity( hacksaw_activity_actor( pnt, item_location{ *p, it } ) );
    } else {
        p->assign_activity( hacksaw_activity_actor( pnt, it->typeId(), it_pnt ) );
    }

    return std::nullopt;
}

std::optional<int> iuse::boltcutters( Character *p, item *it, const tripoint & )
{
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }

    map &here = get_map();
    const std::function<bool( const tripoint & )> f =
    [&here, p]( const tripoint & pnt ) {
        if( pnt == p->pos() ) {
            return false;
        } else if( here.has_furn( pnt ) ) {
            return here.furn( pnt )->boltcut->valid();
        } else if( !here.ter( pnt )->is_null() ) {
            return here.ter( pnt )->boltcut->valid();
        }
        return false;
    };

    const std::optional<tripoint> pnt_ = choose_adjacent_highlight(
            _( "Cut up metal where?" ), _( "There is no metal to cut up nearby." ), f, false );
    if( !pnt_ ) {
        return std::nullopt;
    }
    const tripoint &pnt = *pnt_;
    if( !f( pnt ) ) {
        if( pnt == p->pos() ) {
            p->add_msg_if_player( m_info,
                                  _( "You neatly sever all of the veins and arteries in your body.  Oh, wait; never mind." ) );
        } else {
            p->add_msg_if_player( m_info, _( "You can't cut that." ) );
        }
        return std::nullopt;
    }

    p->assign_activity( boltcutting_activity_actor( pnt, item_location{*p, it} ) );
    return std::nullopt;
}

std::optional<int> iuse::mop( Character *p, item *, const tripoint & )
{
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    map &here = get_map();
    const std::function<bool( const tripoint & )> f = [&here]( const tripoint & pnt ) {
        // TODO: fix point types
        return here.terrain_moppable( tripoint_bub_ms( pnt ) );
    };

    const std::optional<tripoint> pnt_ = choose_adjacent_highlight(
            _( "Mop where?" ), _( "There is nothing to mop nearby." ), f, false );
    if( !pnt_ ) {
        return std::nullopt;
    }
    // TODO: fix point types
    const tripoint_bub_ms pnt( *pnt_ );
    // TODO: fix point types
    if( !f( pnt.raw() ) ) {
        if( pnt == p->pos_bub() ) {
            p->add_msg_if_player( m_info, _( "You mop yourself up." ) );
            p->add_msg_if_player( m_info, _( "The universe implodes and reforms around you." ) );
        } else {
            p->add_msg_if_player( m_bad, _( "There's nothing to mop there." ) );
        }
        return std::nullopt;
    }
    if( p->is_blind() ) {
        p->add_msg_if_player( m_info, _( "You move the mop around, unsure whether it's doing any good." ) );
        p->moves -= 15;
        if( one_in( 3 ) ) {
            here.mop_spills( pnt );
        }
    } else if( here.mop_spills( pnt ) ) {
        p->add_msg_if_player( m_info, _( "You mop up the spill." ) );
        p->moves -= 15;
    } else {
        return std::nullopt;
    }
    return 1;
}

std::optional<int> iuse::spray_can( Character *p, item *it, const tripoint & )
{
    const std::optional<tripoint> dest_ = choose_adjacent( _( "Spray where?" ) );
    if( !dest_ ) {
        return std::nullopt;
    }
    return handle_ground_graffiti( *p, it, _( "Spray what?" ), dest_.value() );
}

std::optional<int> iuse::handle_ground_graffiti( Character &p, item *it, const std::string &prefix,
        const tripoint &where )
{
    map &here = get_map();
    string_input_popup popup;
    std::string message = popup
                          .description( prefix + " " + _( "(To delete, clear the text and confirm)" ) )
                          .text( here.has_graffiti_at( where ) ? here.graffiti_at( where ) : std::string() )
                          .identifier( "graffiti" )
                          .query_string();
    if( popup.canceled() ) {
        return std::nullopt;
    }

    bool grave = here.ter( where ) == t_grave_new;
    int move_cost;
    if( message.empty() ) {
        if( here.has_graffiti_at( where ) ) {
            move_cost = 3 * here.graffiti_at( where ).length();
            here.delete_graffiti( where );
            if( grave ) {
                p.add_msg_if_player( m_info, _( "You blur the inscription on the grave." ) );
            } else {
                p.add_msg_if_player( m_info, _( "You manage to get rid of the message on the surface." ) );
            }
        } else {
            return std::nullopt;
        }
    } else {
        here.set_graffiti( where, message );
        if( grave ) {
            p.add_msg_if_player( m_info, _( "You carve an inscription on the grave." ) );
        } else {
            p.add_msg_if_player( m_info, _( "You write a message on the surface." ) );
        }
        move_cost = 2 * message.length();
    }
    p.moves -= move_cost;
    if( it != nullptr ) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * Heats up a food item.
 * @return 1 if an item was heated, false if nothing was heated.
 */
static bool heat_item( Character &p )
{
    item_location loc = g->inv_map_splice( []( const item_location & itm ) {
        return itm->has_temperature() && !itm->has_own_flag( flag_HOT ) &&
               ( !itm->made_of_from_type( phase_id::LIQUID ) ||
                 itm.where() == item_location::type::container ||
                 get_map().has_flag_furn( ter_furn_flag::TFLAG_LIQUIDCONT, itm.position() ) );
    }, _( "Heat up what?" ), 1, _( "You don't have any appropriate food to heat up." ) );

    item *heat = loc.get_item();
    if( heat == nullptr ) {
        add_msg( m_info, _( "Never mind." ) );
        return false;
    }
    // simulates heat capacity of food, more weight = longer heating time
    // this is x2 to simulate larger delta temperature of frozen food in relation to
    // heating non-frozen food (x1); no real life physics here, only approximations
    int duration = to_turns<int>( time_duration::from_seconds( to_gram( heat->weight() ) ) ) * 10;
    if( heat->has_own_flag( flag_FROZEN ) && !heat->has_flag( flag_EATEN_COLD ) ) {
        duration *= 2;
    }
    p.add_msg_if_player( m_info, _( "You start heating up the food." ) );
    p.assign_activity( ACT_HEATING, duration );
    p.activity.targets.emplace_back( p, heat );
    return true;
}

std::optional<int> iuse::heatpack( Character *p, item *it, const tripoint & )
{
    if( heat_item( *p ) ) {
        it->convert( itype_heatpack_used, p );
    }
    return 0;
}

std::optional<int> iuse::heat_food( Character *p, item *it, const tripoint & )
{
    if( get_map().has_nearby_fire( p->pos() ) ) {
        heat_item( *p );
        return 0;
    } else if( p->has_active_bionic( bio_tools ) && p->get_power_level() > 10_kJ &&
               query_yn( _( "There is no fire around; use your integrated toolset instead?" ) ) ) {
        if( heat_item( *p ) ) {
            p->mod_power_level( -10_kJ );
            return 0;
        }
    } else {
        p->add_msg_if_player( m_info,
                              _( "You need to be next to a fire to heat something up with the %s." ),
                              it->tname() );
    }
    return std::nullopt;
}

std::optional<int> iuse::hotplate( Character *p, item *it, const tripoint & )
{
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    if( !it->ammo_sufficient( p ) ) {
        p->add_msg_if_player( m_info, _( "The %s's batteries are dead." ), it->tname() );
        return std::nullopt;
    }

    if( heat_item( *p ) ) {
        return 1;
    }
    return std::nullopt;
}

std::optional<int> iuse::hotplate_atomic( Character *p, item *it, const tripoint & )
{
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    if( it->typeId() == itype_atomic_coffeepot ) {
        heat_item( *p );
    }

    return std::nullopt;
}

std::optional<int> iuse::towel( Character *p, item *it, const tripoint & )
{
    return towel_common( p, it, false );
}

int iuse::towel_common( Character *p, item *it, bool )
{
    if( !p ) {
        debugmsg( "%s called action towel that requires character but no character is present",
                  it->typeId().str() );
        return 0;
    }
    bool slime = p->has_effect( effect_slimed );
    bool boom = p->has_effect( effect_boomered );
    bool glow = p->has_effect( effect_glowing );
    int mult = slime + boom + glow; // cleaning off more than one at once makes it take longer
    bool towelUsed = false;
    const std::string name = it ? it->tname() : _( "towel" );

    // can't use an already wet towel!
    if( it && it->is_filthy() ) {
        p->add_msg_if_player( m_info, _( "That %s is too filthy to clean anything!" ),
                              it->tname() );
    } else if( it && it->has_flag( flag_WET ) ) {
        p->add_msg_if_player( m_info, _( "That %s is too wet to soak up any more liquid!" ),
                              it->tname() );
        // clean off the messes first, more important
    } else if( slime || boom || glow ) {
        p->remove_effect( effect_slimed ); // able to clean off all at once
        p->remove_effect( effect_boomered );
        p->remove_effect( effect_glowing );
        p->add_msg_if_player( _( "You use the %s to clean yourself off, saturating it with slime!" ),
                              name );

        towelUsed = true;
        if( it && it->typeId() == itype_towel ) {
            it->set_flag( flag_FILTHY );
        }

        // dry off from being wet
    } else if( p->has_atleast_one_wet_part() ) {
        p->rem_morale( MORALE_WET );
        p->set_all_parts_wetness( 0 );
        p->add_msg_if_player( _( "You use the %s to dry off, saturating it with water!" ),
                              name );

        towelUsed = true;
        if( it && it->typeId() == itype_towel ) {
            it->item_counter = to_turns<int>( 30_minutes );
            // change "towel" to a "towel_wet" (different flavor text/color)
            it->convert( itype_towel_wet, p );
        }

        // default message
    } else {
        p->add_msg_if_player( _( "You are already dry; the %s does nothing." ), name );
    }

    // towel was used
    if( towelUsed ) {
        if( mult == 0 ) {
            mult = 1;
        }
        p->moves -= 50 * mult;
        if( it ) {
            // WET, active items have their timer decremented every turn
            it->set_flag( flag_WET );
            it->active = true;
        }
    }
    return it ? 1 : 0;
}

std::optional<int> iuse::unfold_generic( Character *p, item *it, const tripoint & )
{
    p->assign_activity( vehicle_unfolding_activity_actor( *it ) );
    p->i_rem( it );
    return 0;
}

std::optional<int> iuse::adrenaline_injector( Character *p, item *it, const tripoint & )
{
    if( p->is_npc() && p->get_effect_dur( effect_adrenaline ) >= 30_minutes ) {
        return std::nullopt;
    }

    p->moves -= to_moves<int>( 1_seconds );
    p->add_msg_player_or_npc( _( "You inject yourself with adrenaline." ),
                              _( "<npcname> injects themselves with adrenaline." ) );

    item syringe( "syringe", it->birthday() );
    p->i_add( syringe );
    if( p->has_effect( effect_adrenaline ) ) {
        p->add_msg_if_player( m_bad, _( "Your heart spasms!" ) );
        // Note: not the mod, the health
        p->mod_livestyle( -20 );
    }

    p->add_effect( effect_adrenaline, 20_minutes );

    return 1;
}

std::optional<int> iuse::jet_injector( Character *p, item *it, const tripoint & )
{
    if( !it->ammo_sufficient( p ) ) {
        p->add_msg_if_player( m_info, _( "The jet injector is empty." ) );
        return std::nullopt;
    } else {
        p->add_msg_if_player( _( "You inject yourself with the jet injector." ) );
        // Intensity is 2 here because intensity = 1 is the comedown
        p->add_effect( effect_jetinjector, 20_minutes, false, 2 );
        p->mod_painkiller( 20 );
        p->mod_stim( 10 );
        p->healall( 20 );
    }

    if( p->has_effect( effect_jetinjector ) ) {
        if( p->get_effect_dur( effect_jetinjector ) > 20_minutes ) {
            p->add_msg_if_player( m_warning, _( "Your heart is beating alarmingly fast!" ) );
        }
    }
    return 1;
}

std::optional<int> iuse::stimpack( Character *p, item *it, const tripoint & )
{
    if( p->get_item_position( it ) >= -1 ) {
        p->add_msg_if_player( m_info,
                              _( "You must wear the stimulant delivery system before you can activate it." ) );
        return std::nullopt;
    }

    if( !it->ammo_sufficient( p ) ) {
        p->add_msg_if_player( m_info, _( "The stimulant delivery system is empty." ) );
        return std::nullopt;
    } else {
        p->add_msg_if_player( _( "You inject yourself with the stimulants." ) );
        // Intensity is 2 here because intensity = 1 is the comedown
        p->add_effect( effect_stimpack, 25_minutes, false, 2 );
        p->mod_painkiller( 2 );
        p->mod_stim( 20 );
        p->mod_fatigue( -100 );
        p->set_stamina( p->get_stamina_max() );
    }
    return 1;
}

std::optional<int> iuse::radglove( Character *p, item *it, const tripoint & )
{
    if( p->get_item_position( it ) >= -1 ) {
        p->add_msg_if_player( m_info,
                              _( "You must wear the radiation biomonitor before you can activate it." ) );
        return std::nullopt;
    } else if( !it->ammo_sufficient( p ) ) {
        p->add_msg_if_player( m_info, _( "The radiation biomonitor needs batteries to function." ) );
        return std::nullopt;
    } else {
        p->add_msg_if_player( _( "You activate your radiation biomonitor." ) );
        if( p->get_rad() >= 1 ) {
            p->add_msg_if_player( m_warning, _( "You are currently irradiated." ) );
            p->add_msg_player_or_say( m_info,
                                      _( "Your radiation level: %d mSv." ),
                                      _( "It says here that my radiation level is %d mSv." ),
                                      p->get_rad() );
        } else {
            p->add_msg_player_or_say( m_info,
                                      _( "You aren't currently irradiated." ),
                                      _( "It says I'm not irradiated." ) );
        }
        p->add_msg_if_player( _( "Have a nice day!" ) );
    }

    return 1;
}

std::optional<int> iuse::contacts( Character *p, item *it, const tripoint & )
{
    if( p->cant_do_underwater() ) {
        return std::nullopt;
    }
    const time_duration duration = rng( 6_days, 8_days );
    if( p->has_effect( effect_contacts ) ) {
        if( query_yn( _( "Replace your current lenses?" ) ) ) {
            p->moves -= to_moves<int>( 20_seconds );
            p->add_msg_if_player( _( "You replace your current %s." ), it->tname() );
            p->remove_effect( effect_contacts );
            p->add_effect( effect_contacts, duration );
            return 1;
        } else {
            p->add_msg_if_player( _( "You don't do anything with your %s." ), it->tname() );
            return std::nullopt;
        }
    } else if( p->has_flag( json_flag_HYPEROPIC ) || p->has_flag( json_flag_MYOPIC ) ||
               p->has_flag( json_flag_MYOPIC_IN_LIGHT ) ) {
        p->moves -= to_moves<int>( 20_seconds );
        p->add_msg_if_player( _( "You put the %s in your eyes." ), it->tname() );
        p->add_effect( effect_contacts, duration );
        return 1;
    } else {
        p->add_msg_if_player( m_info, _( "Your vision is fine already." ) );
        return std::nullopt;
    }
}

std::optional<int> iuse::talking_doll( Character *p, item *it, const tripoint & )
{
    if( !it->ammo_sufficient( p ) ) {
        p->add_msg_if_player( m_info, _( "The %s's batteries are dead." ), it->tname() );
        return std::nullopt;
    }
    p->add_msg_if_player( m_neutral, _( "You press a button on the doll to make it talk." ) );
    const SpeechBubble speech = get_speech( it->typeId().str() );

    sounds::sound( p->pos(), speech.volume, sounds::sound_t::electronic_speech,
                   speech.text.translated(), true, "speech", it->typeId().str() );

    return 1;
}

std::optional<int> iuse::gun_repair( Character *p, item *it, const tripoint & )
{
    if( !it->ammo_sufficient( p ) ) {
        return std::nullopt;
    }
    if( p->cant_do_underwater() ) {
        return std::nullopt;
    }
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    /** @EFFECT_MECHANICS >1 allows gun repair */
    if( p->get_skill_level( skill_mechanics ) < 2 ) {
        p->add_msg_if_player( m_info, _( "You need a mechanics skill of 2 to use this repair kit." ) );
        return std::nullopt;
    }
    item_location loc = game_menus::inv::titled_filter_menu( []( const item_location & loc ) {
        return loc->is_firearm() && !loc->has_flag( flag_NO_REPAIR );
    }, get_avatar(), _( "Select the firearm to repair:" ) );
    if( !loc ) {
        p->add_msg_if_player( m_info, _( "You don't have that item!" ) );
        return std::nullopt;
    }
    return ::gun_repair( p, it, loc );
}

std::optional<int> gun_repair( Character *p, item *, item_location &loc )
{
    item &fix = *loc;
    if( p->fine_detail_vision_mod() > 4 ) {
        p->add_msg_if_player( m_info, _( "You can't see to do that!" ) );
        return std::nullopt;
    }
    if( fix.damage() <= fix.degradation() ) {
        const char *msg = fix.damage_level() > 0 ?
                          _( "You can't improve your %s any more, considering the degradation." ) :
                          _( "You can't improve your %s any more this way." );
        p->add_msg_if_player( m_info, msg, fix.tname() );
        return std::nullopt;
    }
    const std::string startdurability = fix.durability_indicator( true );
    sounds::sound( p->pos(), 8, sounds::sound_t::activity, "crunch", true, "tool", "repair_kit" );
    p->practice( skill_mechanics, 10 );
    p->moves -= to_moves<int>( 20_seconds );

    fix.mod_damage( -itype::damage_scale );

    const std::string msg = fix.damage_level() == 0
                            ? _( "You repair your %s completely!  ( %s-> %s)" )
                            : _( "You repair your %s!  ( %s-> %s)" );
    p->add_msg_if_player( m_good, msg, fix.tname( 1, false ), startdurability,
                          fix.durability_indicator( true ) );
    return 1;
}

std::optional<int> iuse::gunmod_attach( Character *p, item *it, const tripoint & )
{
    if( !it || !it->is_gunmod() ) {
        debugmsg( "tried to attach non-gunmod" );
        return std::nullopt;
    }

    if( !p ) {
        return std::nullopt;
    }

    do {
        item_location loc = game_menus::inv::gun_to_modify( *p->as_character(), *it );

        if( !loc ) {
            add_msg( m_info, _( "Never mind." ) );
            return std::nullopt;
        }

        if( !loc->is_gunmod_compatible( *it ).success() ) {
            return std::nullopt;
        }

        const item mod_copy( *it );
        item modded_gun( *loc );

        modded_gun.put_in( mod_copy, pocket_type::MOD );

        if( !game_menus::inv::compare_items( *loc, modded_gun, _( "Attach modification?" ) ) ) {
            continue;
        }

        p->gunmod_add( *loc, *it );
        return 0;
    } while( true );
}

std::optional<int> iuse::toolmod_attach( Character *p, item *it, const tripoint & )
{
    if( !it || !it->is_toolmod() ) {
        debugmsg( "tried to attach non-toolmod" );
        return std::nullopt;
    }

    if( !p ) {
        return std::nullopt;
    }

    auto filter = [&it]( const item & e ) {
        // don't allow ups or bionic battery mods on a UPS or UPS-powered/bionic-powered tools
        if( ( it->has_flag( flag_USE_UPS ) || it->has_flag( flag_USES_BIONIC_POWER ) ) &&
            ( e.has_flag( flag_IS_UPS ) || e.has_flag( flag_USE_UPS ) ||
              e.has_flag( flag_USES_BIONIC_POWER ) ) ) {
            return false;
        }

        // can't mod non-tool, or a tool with existing mods, or a battery currently installed
        if( !e.is_tool() || !e.toolmods().empty() || e.magazine_current() ) {
            return false;
        }

        // can only attach to unmodified tools that use compatible ammo
        return std::any_of( it->type->mod->acceptable_ammo.begin(),
        it->type->mod->acceptable_ammo.end(), [&]( const ammotype & at ) {
            return e.type->tool->ammo_id.count( at );
        } );
    };

    item_location loc = g->inv_map_splice( filter, _( "Select tool to modify:" ), 1,
                                           _( "You don't have compatible tools." ) );

    if( !loc ) {
        add_msg( m_info, _( "Never mind." ) );
        return std::nullopt;
    }

    if( loc->ammo_remaining() ) {
        if( !p->unload( loc ) ) {
            p->add_msg_if_player( m_info, _( "You cancel unloading the tool." ) );
            return std::nullopt;
        }
    }

    p->toolmod_add( std::move( loc ), item_location( *p, it ) );
    return 0;
}

std::optional<int> iuse::bell( Character *p, item *it, const tripoint & )
{
    if( it->typeId() == itype_cow_bell ) {
        sounds::sound( p->pos(), 12, sounds::sound_t::music, _( "Clank!  Clank!" ), true, "misc",
                       "cow_bell" );
        if( !p->is_deaf() ) {
            auto cattle_level =
                p->mutation_category_level.find( mutation_category_CATTLE );
            const int cow_factor = 1 + ( cattle_level == p->mutation_category_level.end() ?
                                         0 : cattle_level->second );
            if( x_in_y( cow_factor, 1 + cow_factor ) ) {
                p->add_morale( MORALE_MUSIC, 1, std::min( cow_factor, 100 ) );
            }
        }
    } else {
        sounds::sound( p->pos(), 4, sounds::sound_t::music, _( "Ring!  Ring!" ), true, "misc", "bell" );
    }
    return 1;
}

std::optional<int> iuse::seed( Character *p, item *it, const tripoint & )
{
    if( p->is_npc() ||
        query_yn( _( "Are you sure you want to eat the %s?  You could plant it in a mound of dirt." ),
                  colorize( it->tname(), it->color_in_inventory() ) ) ) {
        return 1; //This eats the seed object.
    }
    return std::nullopt;
}

bool iuse::robotcontrol_can_target( Character *p, const monster &m )
{
    return !m.is_dead()
           && m.type->in_species( species_ROBOT )
           && m.friendly == 0
           && rl_dist( p->pos(), m.pos() ) <= 10;
}

std::optional<int> iuse::robotcontrol( Character *p, item *it, const tripoint & )
{

    bool isComputer = !it->has_flag( flag_MAGICAL );
    int choice = 0;

    if( isComputer ) {
        if( !it->ammo_sufficient( p ) ) {
            p->add_msg_if_player( _( "The %s's batteries are dead." ), it->tname() );
            return std::nullopt;
        }

        if( p->has_trait( trait_ILLITERATE ) ) {
            p->add_msg_if_player( _( "You can't read a computer screen." ) );
            return std::nullopt;
        }

        if( p->has_flag( json_flag_HYPEROPIC ) && !p->worn_with_flag( flag_FIX_FARSIGHT ) &&
            !p->has_effect( effect_contacts ) && !p->has_flag( json_flag_ENHANCED_VISION ) ) {
            p->add_msg_if_player( m_info,
                                  _( "You'll need to put on reading glasses before you can see the screen." ) );
            return std::nullopt;
        }

        choice = uilist( _( "Welcome to hackPRO!" ), {
            _( "Prepare IFF protocol override" ),
            _( "Set friendly robots to passive mode" ),
            _( "Set friendly robots to combat mode" )
        } );
    } else {
        if( !it->ammo_sufficient( p ) ) {
            p->add_msg_if_player( _( "The %s lacks charge to function." ), it->tname() );
            return std::nullopt;
        }
        choice = uilist( _( "You prepare to manipulate nearby robots!" ), {
            _( "Prepare IFF protocol override" ),
            _( "Set friendly robots to passive mode" ),
            _( "Set friendly robots to combat mode" )
        } );
    }
    switch( choice ) {
        case 0: { // attempt to make a robot friendly
            uilist pick_robot;
            pick_robot.text = _( "Choose an endpoint to hack." );
            // Build a list of all unfriendly robots in range.
            // TODO: change into vector<Creature*>
            std::vector< shared_ptr_fast< monster> > mons;
            std::vector< tripoint > locations;
            int entry_num = 0;
            for( const monster &candidate : g->all_monsters() ) {
                if( robotcontrol_can_target( p, candidate ) ) {
                    mons.push_back( g->shared_from( candidate ) );
                    pick_robot.addentry( entry_num++, true, MENU_AUTOASSIGN, candidate.name() );
                    tripoint seen_loc;
                    // Show locations of seen robots, center on player if robot is not seen
                    if( p->sees( candidate ) ) {
                        seen_loc = candidate.pos();
                    } else {
                        seen_loc = p->pos();
                    }
                    locations.push_back( seen_loc );
                }
            }
            if( mons.empty() ) {
                p->add_msg_if_player( m_info, _( "No enemy robots in range." ) );
                return 1;
            }
            pointmenu_cb callback( locations );
            pick_robot.callback = &callback;
            pick_robot.query();
            if( pick_robot.ret < 0 || static_cast<size_t>( pick_robot.ret ) >= mons.size() ) {
                p->add_msg_if_player( m_info, _( "Never mind" ) );
                return 1;
            }
            const size_t mondex = pick_robot.ret;
            shared_ptr_fast< monster > z = mons[mondex];
            p->add_msg_if_player( _( "You start reprogramming the %s into an ally." ), z->name() );

            /** @EFFECT_INT speeds up hacking preparation */
            /** @EFFECT_COMPUTER speeds up hacking preparation */
            int move_cost = std::max( 100,
                                      1000 - static_cast<int>( p->int_cur * 10 - p->get_skill_level( skill_computer ) * 10 ) );
            player_activity act( ACT_ROBOT_CONTROL, move_cost );
            act.monsters.emplace_back( z );

            p->assign_activity( act );

            return 1;
        }
        case 1: { //make all friendly robots stop their purposeless extermination of (un)life.
            p->moves -= to_moves<int>( 1_seconds );
            int f = 0; //flag to check if you have robotic allies
            for( monster &critter : g->all_monsters() ) {
                if( critter.friendly != 0 && critter.type->in_species( species_ROBOT ) ) {
                    p->add_msg_if_player( _( "A following %s goes into passive mode." ),
                                          critter.name() );
                    critter.add_effect( effect_docile, 1_turns, true );
                    f = 1;
                }
            }
            if( f == 0 ) {
                p->add_msg_if_player( _( "You aren't commanding any robots." ) );
                return std::nullopt;
            }
            return 1;
        }
        case 2: { //make all friendly robots terminate (un)life with extreme prejudice
            p->moves -= to_moves<int>( 1_seconds );
            int f = 0; //flag to check if you have robotic allies
            for( monster &critter : g->all_monsters() ) {
                if( critter.friendly != 0 && critter.has_flag( mon_flag_ELECTRONIC ) ) {
                    p->add_msg_if_player( _( "A following %s goes into combat mode." ),
                                          critter.name() );
                    critter.remove_effect( effect_docile );
                    f = 1;
                }
            }
            if( f == 0 ) {
                p->add_msg_if_player( _( "You aren't commanding any robots." ) );
                return std::nullopt;
            }
            return 1;
        }
    }
    return 0;
}

static int get_quality_from_string( const std::string_view s )
{
    const ret_val<int> try_quality = try_parse_integer<int>( s, false );
    if( try_quality.success() ) {
        return try_quality.value();
    } else {
        debugmsg( "Error parsing photo quality: %s", try_quality.str() );
        return 0;
    }
}

static std::string photo_quality_name( const int index )
{
    static const std::array<std::string, 6> names {
        {
            //~ photo quality adjective
            { translate_marker( "awful" ) }, { translate_marker( "bad" ) }, { translate_marker( "not bad" ) }, { translate_marker( "good" ) }, { translate_marker( "fine" ) }, { translate_marker( "exceptional" ) }
        }
    };
    return _( names[index] );
}

void item::extended_photo_def::deserialize( const JsonObject &obj )
{
    quality = obj.get_int( "quality" );
    name = obj.get_string( "name" );
    description = obj.get_string( "description" );
}

void item::extended_photo_def::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "quality", quality );
    jsout.member( "name", name );
    jsout.member( "description", description );
    jsout.end_object();
}

std::optional<int> iuse::epic_music( Character *p, item *it, const tripoint &pos )
{
    if( !it->get_var( "EIPC_MUSIC_ON" ).empty() &&
        it->ammo_sufficient( p ) ) {

        //the more varied music, the better max mood.
        const int songs = it->get_var( "EIPC_MUSIC", 0 );
        play_music( p, pos, 8, std::min( 25, songs ) );
    }
    return std::nullopt;
}

std::optional<int> iuse::einktabletpc( Character *p, item *it, const tripoint & )
{

    if( p->cant_do_mounted() ) {
        return std::nullopt;
    } else if( !p->is_npc() ) {

        enum {
            ei_invalid, ei_photo, ei_music, ei_recipe, ei_uploaded_photos, ei_monsters, ei_download,
        };

        if( p->cant_do_underwater() ) {
            return std::nullopt;
        }
        if( p->has_trait( trait_ILLITERATE ) ) {
            p->add_msg_if_player( m_info, _( "You can't read a computer screen." ) );
            return std::nullopt;
        }
        if( p->has_flag( json_flag_HYPEROPIC ) && !p->worn_with_flag( flag_FIX_FARSIGHT ) &&
            !p->has_effect( effect_contacts ) && !p->has_flag( json_flag_ENHANCED_VISION ) ) {
            p->add_msg_if_player( m_info,
                                  _( "You'll need to put on reading glasses before you can see the screen." ) );
            return std::nullopt;
        }

        if( !it->active ) {
            it->erase_var( "EIPC_MUSIC_ON" );
        }
        uilist amenu;

        amenu.text = _( "Choose menu option:" );

        const int photos = it->get_var( "EIPC_PHOTOS", 0 );
        if( photos > 0 ) {
            amenu.addentry( ei_photo, true, 'p', _( "Unsorted photos [%d]" ), photos );
        } else {
            amenu.addentry( ei_photo, false, 'p', _( "No photos on device" ) );
        }

        const int songs = it->get_var( "EIPC_MUSIC", 0 );
        if( songs > 0 ) {
            if( it->has_var( "EIPC_MUSIC_ON" ) ) {
                amenu.addentry( ei_music, true, 'm', _( "Turn music off" ) );
            } else {
                amenu.addentry( ei_music, true, 'm', _( "Turn music on [%d]" ), songs );
            }
        } else {
            amenu.addentry( ei_music, false, 'm', _( "No music on device" ) );
        }

        if( !it->get_saved_recipes().empty() ) {
            amenu.addentry( ei_recipe, true, 'r', _( "List stored recipes" ) );
        }

        if( !it->get_var( "EIPC_EXTENDED_PHOTOS" ).empty() ) {
            amenu.addentry( ei_uploaded_photos, true, 'l', _( "Your photos" ) );
        }

        if( !it->get_var( "EINK_MONSTER_PHOTOS" ).empty() ) {
            amenu.addentry( ei_monsters, true, 'y', _( "Your collection of monsters" ) );
        } else {
            amenu.addentry( ei_monsters, false, 'y', _( "Collection of monsters is empty" ) );
        }

        amenu.addentry( ei_download, true, 'w', _( "Download data from memory cards" ) );
        amenu.query();

        const int choice = amenu.ret;

        if( ei_photo == choice ) {

            const int photos = it->get_var( "EIPC_PHOTOS", 0 );
            const int viewed = std::min( photos, static_cast<int>( rng( 10, 30 ) ) );
            const int count = photos - viewed;
            if( count == 0 ) {
                it->erase_var( "EIPC_PHOTOS" );
            } else {
                it->set_var( "EIPC_PHOTOS", count );
            }

            p->moves -= to_moves<int>( rng( 3_seconds, 7_seconds ) );

            if( p->has_trait( trait_PSYCHOPATH ) ) {
                p->add_msg_if_player( m_info, _( "Wasted time.  These pictures do not provoke your senses." ) );
            } else {
                p->add_morale( MORALE_PHOTOS, rng( 15, 30 ), 100 );
                p->add_msg_if_player( m_good, "%s",
                                      SNIPPET.random_from_category( "examine_photo_msg" ).value_or( translation() ) );
            }

            return 1;
        }

        if( ei_music == choice ) {

            p->moves -= 30;
            // Turn on the screen before playing musics
            if( !it->active ) {
                if( it->is_transformable() ) {
                    const use_function *readinglight = it->type->get_use( "transform" );
                    if( readinglight ) {
                        readinglight->call( p, *it, p->pos() );
                    }
                }
                it->activate();
            }
            // If transformable we use transform action to turn off the device
            else if( !it->is_transformable() ) {
                it->deactivate();
            }

            if( it->has_var( "EIPC_MUSIC_ON" ) ) {
                it->erase_var( "EIPC_MUSIC_ON" );

                p->add_msg_if_player( m_info, _( "You turned off the music on your %s." ), it->tname() );
            } else {
                it->set_var( "EIPC_MUSIC_ON", "1" );
                p->add_msg_if_player( m_info, _( "You turned on the music on your %s." ), it->tname() );
            }

            return 1;
        }

        if( ei_recipe == choice ) {
            p->moves -= 50;

            uilist rmenu;
            for( const recipe_id &rid : it->get_saved_recipes() ) {
                rmenu.addentry( 0, true, 0, rid->result_name( /* decorated = */ true ) );
            }

            rmenu.text = _( "List recipes:" );
            rmenu.query();

            return 1;
        }

        if( ei_uploaded_photos == choice ) {
            show_photo_selection( *p, *it, "EIPC_EXTENDED_PHOTOS" );
            return 1;
        }

        if( ei_monsters == choice ) {

            uilist pmenu;

            pmenu.text = _( "Your collection of monsters:" );

            std::vector<mtype_id> monster_photos;

            std::istringstream f( it->get_var( "EINK_MONSTER_PHOTOS" ) );
            std::string s;
            int k = 0;
            while( getline( f, s, ',' ) ) {
                if( s.empty() ) {
                    continue;
                }
                monster_photos.emplace_back( s );
                std::string menu_str;
                const monster dummy( monster_photos.back() );
                menu_str = dummy.name();
                getline( f, s, ',' );
                const int quality = get_quality_from_string( s );
                menu_str += " [" + photo_quality_name( quality ) + "]";
                pmenu.addentry( k++, true, -1, menu_str.c_str() );
            }

            int choice;
            do {
                pmenu.query();
                choice = pmenu.ret;

                if( choice < 0 ) {
                    break;
                }

                const monster dummy( monster_photos[choice] );
                popup( dummy.type->get_description() );
            } while( true );
            return 1;
        }

        if( ei_download == choice ) {
            if( !p->has_item( *it ) ) {
                p->add_msg_if_player( m_info, _( "You don't have that item!" ) );
                return std::nullopt; // need posession of reader item for item_location to work right
            }
            if( !p->is_avatar() ) {
                return std::nullopt; // npc triggered iuse?
            }
            inventory_filter_preset preset( []( const item_location & it ) {
                return it->has_flag( flag_MC_HAS_DATA ) || it->type->memory_card_data;
            } );
            inventory_multiselector inv_s( *p, preset );

            inv_s.set_title( _( "Choose memory cards to download data from:" ) );
            inv_s.set_display_stats( true );
            inv_s.add_character_items( *p );
            inv_s.add_nearby_items( 1 );

            const std::list<std::pair<item_location, int>> locs = inv_s.execute();
            if( locs.empty() ) {
                p->add_msg_if_player( m_info, _( "Nevermind." ) );
                return std::nullopt;
            }
            std::vector<item_location> targets;
            for( const auto& [item_loc, count] : locs ) {
                targets.emplace_back( item_loc );
            }
            const item_location reader( *p, it );
            const data_handling_activity_actor actor( reader, targets );
            p->assign_activity( player_activity( actor ) );
        }
    }
    return std::nullopt;
}

static std::string colorized_trap_name_at( const tripoint &point )
{
    const trap &trap = get_map().tr_at( point );
    std::string name;
    if( trap.can_see( point, get_player_character() ) ) {
        name = colorize( trap.name(), trap.color ) + _( " on " );
    }
    return name;
}

static const std::unordered_map<description_affix, std::string> description_affixes = {
    { description_affix::DESCRIPTION_AFFIX_IN, translate_marker( " in %s" ) },
    { description_affix::DESCRIPTION_AFFIX_COVERED_IN, translate_marker( " covered in %s" ) },
    { description_affix::DESCRIPTION_AFFIX_ON, translate_marker( " on %s" ) },
    { description_affix::DESCRIPTION_AFFIX_UNDER, translate_marker( " under %s" ) },
    { description_affix::DESCRIPTION_AFFIX_ILLUMINATED_BY, translate_marker( " in %s" ) },
};

static std::string colorized_field_description_at( const tripoint &point )
{
    std::string field_text;
    const field &field = get_map().field_at( point );
    const field_entry *entry = field.find_field( field.displayed_field_type() );
    if( entry ) {
        field_text = string_format( _( description_affixes.at( field.displayed_description_affix() ) ),
                                    colorize( entry->name(), entry->color() ) );
    }
    return field_text;
}

static std::string colorized_item_name( const item &item )
{
    nc_color color = item.color_in_inventory();
    std::string damtext = item.damage() != 0 ? item.durability_indicator() : "";
    return damtext + colorize( item.tname( 1, false ), color );
}

static std::string colorized_item_description( const item &item )
{
    std::vector<iteminfo> dummy;
    iteminfo_query query = iteminfo_query(
    std::vector<iteminfo_parts> {
        iteminfo_parts::DESCRIPTION,
        iteminfo_parts::DESCRIPTION_NOTES,
        iteminfo_parts::DESCRIPTION_CONTENTS
    } );
    return item.info( dummy, &query, 1 );
}

static item get_top_item_at_point( const tripoint &point,
                                   const units::volume &min_visible_volume )
{
    map_stack items = get_map().i_at( point );
    // iterate from topmost item down to ground
    for( const item &it : items ) {
        if( it.volume() > min_visible_volume ) {
            // return top (or first big enough) item to the list
            return it;
        }
    }
    return item();
}

static std::string colorized_ter_name_flags_at( const tripoint &point,
        const std::vector<std::string> &flags, const std::vector<ter_str_id> &ter_whitelist )
{
    map &here = get_map();
    const ter_id ter = here.ter( point );
    std::string name = colorize( ter->name(), ter->color() );
    const std::string &graffiti_message = here.graffiti_at( point );

    if( !graffiti_message.empty() ) {
        name += string_format( _( " with graffiti \"%s\"" ), graffiti_message );
        return name;
    }
    if( ter_whitelist.empty() && flags.empty() ) {
        return name;
    }
    if( !ter->open.is_null() || ( ter->has_examine( iexamine::none ) &&
                                  ter->has_examine( iexamine::fungus ) &&
                                  ter->has_examine( iexamine::water_source ) &&
                                  ter->has_examine( iexamine::dirtmound ) ) ) {
        return name;
    }
    for( const ter_str_id &ter_good : ter_whitelist ) {
        if( ter->id == ter_good ) {
            return name;
        }
    }
    for( const std::string &flag : flags ) {
        if( ter->has_flag( flag ) ) {
            return name;
        }
    }

    return std::string();
}

static std::string colorized_feature_description_at( const tripoint &center_point, bool &item_found,
        const units::volume &min_visible_volume )
{
    item_found = false;
    map &here = get_map();
    const furn_id furn = here.furn( center_point );
    if( furn != f_null && furn.is_valid() ) {
        std::string furn_str = colorize( furn->name(), c_yellow );
        std::string sign_message = here.get_signage( center_point );
        if( !sign_message.empty() ) {
            furn_str += string_format( _( " with message \"%s\"" ), sign_message );
        }
        if( !furn->has_flag( ter_furn_flag::TFLAG_CONTAINER ) &&
            !furn->has_flag( ter_furn_flag::TFLAG_SEALED ) ) {
            const item item = get_top_item_at_point( center_point, min_visible_volume );
            if( !item.is_null() ) {
                furn_str += string_format( _( " with %s on it" ), colorized_item_name( item ) );
                item_found = true;
            }
        }
        return furn_str;
    }
    return std::string();
}

static std::string format_object_pair( const std::pair<std::string, int> &pair,
                                       const std::string &article )
{
    if( pair.second == 1 ) {
        return article + pair.first;
    } else if( pair.second > 1 ) {
        return string_format( "%i %s", pair.second, pair.first );
    }
    return std::string();
}
static std::string format_object_pair_article( const std::pair<std::string, int> &pair )
{
    return format_object_pair( pair, pgettext( "Article 'a', replace it with empty "
                               "string if it is not used in language", "a " ) );
}
static std::string format_object_pair_no_article( const std::pair<std::string, int> &pair )
{
    return format_object_pair( pair, "" );
}

static std::string effects_description_for_creature( Creature *const creature, std::string &pose,
        const std::string &pronoun_gender )
{
    struct ef_con { // effect constraint
        translation status;
        translation pose;
        int intensity_lower_limit;
        ef_con( const translation &status, const translation &pose, int intensity_lower_limit ) :
            status( status ), pose( pose ), intensity_lower_limit( intensity_lower_limit ) {}
        ef_con( const translation &status, const translation &pose ) :
            status( status ), pose( pose ), intensity_lower_limit( 0 ) {}
        ef_con( const translation &status, int intensity_lower_limit ) :
            status( status ), intensity_lower_limit( intensity_lower_limit ) {}
        explicit ef_con( const translation &status ) :
            status( status ), intensity_lower_limit( 0 ) {}
    };
    static const std::unordered_map<efftype_id, ef_con> vec_effect_status = {
        { effect_onfire, ef_con( to_translation( " is on <color_red>fire</color>.  " ) ) },
        { effect_bleed, ef_con( to_translation( " is <color_red>bleeding</color>.  " ), 1 ) },
        { effect_happy, ef_con( to_translation( " looks <color_green>happy</color>.  " ), 13 ) },
        { effect_downed, ef_con( translation(), to_translation( "downed" ) ) },
        { effect_in_pit, ef_con( translation(), to_translation( "stuck" ) ) },
        { effect_stunned, ef_con( to_translation( " is <color_blue>stunned</color>.  " ) ) },
        { effect_dazed, ef_con( to_translation( " is <color_blue>dazed</color>.  " ) ) },
        // NOLINTNEXTLINE(cata-text-style): spaces required for concatenation
        { effect_beartrap, ef_con( to_translation( " is stuck in beartrap.  " ) ) },
        // NOLINTNEXTLINE(cata-text-style): spaces required for concatenation
        { effect_laserlocked, ef_con( to_translation( " have tiny <color_red>red dot</color> on body.  " ) ) },
        { effect_boomered, ef_con( to_translation( " is covered in <color_magenta>bile</color>.  " ) ) },
        { effect_glowing, ef_con( to_translation( " is covered in <color_yellow>glowing goo</color>.  " ) ) },
        { effect_slimed, ef_con( to_translation( " is covered in <color_green>thick goo</color>.  " ) ) },
        { effect_corroding, ef_con( to_translation( " is covered in <color_light_green>acid</color>.  " ) ) },
        { effect_sap, ef_con( to_translation( " is coated in <color_brown>sap</color>.  " ) ) },
        { effect_webbed, ef_con( to_translation( " is covered in <color_dark_gray>webs</color>.  " ) ) },
        { effect_spores, ef_con( to_translation( " is covered in <color_green>spores</color>.  " ), 1 ) },
        { effect_crushed, ef_con( to_translation( " lies under <color_dark_gray>collapsed debris</color>.  " ), to_translation( "lies" ) ) },
        { effect_lack_sleep, ef_con( to_translation( " looks <color_dark_gray>very tired</color>.  " ) ) },
        { effect_lying_down, ef_con( to_translation( " is <color_dark_blue>sleeping</color>.  " ), to_translation( "lies" ) ) },
        { effect_sleep, ef_con( to_translation( " is <color_dark_blue>sleeping</color>.  " ), to_translation( "lies" ) ) },
        { effect_haslight, ef_con( to_translation( " is <color_yellow>lit</color>.  " ) ) },
        { effect_monster_saddled, ef_con( to_translation( " is <color_dark_gray>saddled</color>.  " ) ) },
        // NOLINTNEXTLINE(cata-text-style): spaces required for concatenation
        { effect_harnessed, ef_con( to_translation( " is being <color_dark_gray>harnessed</color> by a vehicle.  " ) ) },
        { effect_monster_armor, ef_con( to_translation( " is <color_dark_gray>wearing armor</color>.  " ) ) },
        // NOLINTNEXTLINE(cata-text-style): spaces required for concatenation
        { effect_has_bag, ef_con( to_translation( " have <color_dark_gray>bag</color> attached.  " ) ) },
        { effect_tied, ef_con( to_translation( " is <color_dark_gray>tied</color>.  " ) ) },
        { effect_bouldering, ef_con( translation(), to_translation( "balancing" ) ) }
    };

    std::string figure_effects;
    if( creature ) {
        for( const auto &pair : vec_effect_status ) {
            if( creature->get_effect_int( pair.first ) > pair.second.intensity_lower_limit ) {
                if( !pair.second.status.empty() ) {
                    figure_effects += pronoun_gender + pair.second.status;
                }
                if( !pair.second.pose.empty() ) {
                    pose = pair.second.pose.translated();
                }
            }
        }
        if( creature->has_effect( effect_sad ) ) {
            int intensity = creature->get_effect_int( effect_sad );
            if( intensity > 500 && intensity <= 950 ) {
                figure_effects += pronoun_gender + pgettext( "Someone", " looks <color_blue>sad</color>.  " );
            } else if( intensity > 950 ) {
                figure_effects += pronoun_gender + pgettext( "Someone", " looks <color_blue>depressed</color>.  " );
            }
        }
        float pain = creature->get_pain() / 10.f;
        if( pain > 3 ) {
            figure_effects += pronoun_gender + pgettext( "Someone",
                              " is writhing in <color_red>pain</color>.  " );
        }
        if( creature->has_effect( effect_riding ) ) {
            pose = _( "rides" );
            monster *const mon = get_creature_tracker().creature_at<monster>( creature->pos(), false );
            figure_effects += pronoun_gender + string_format( _( " is riding %s.  " ),
                              colorize( mon->name(), c_light_blue ) );
        }
        if( creature->has_effect( effect_glowy_led ) ) {
            figure_effects += _( "A bionic LED is <color_yellow>glowing</color> softly.  " );
        }
    }
    while( !figure_effects.empty() && figure_effects.back() == ' ' ) { // remove last spaces
        figure_effects.erase( figure_effects.end() - 1 );
    }
    return figure_effects;
}

struct object_names_collection {
    std::unordered_map<std::string, int>
    furniture,
    vehicles,
    items,
    terrain;

    std::string figure_text;
    std::string obj_nearby_text;
};

static object_names_collection enumerate_objects_around_point( const tripoint &point,
        const int radius, const tripoint &bounds_center_point, const int bounds_radius,
        const tripoint &camera_pos, const units::volume &min_visible_volume, bool create_figure_desc,
        std::unordered_set<tripoint> &ignored_points,
        std::unordered_set<const vehicle *> &vehicles_recorded )
{
    map &here = get_map();
    const tripoint_range<tripoint> bounds =
        here.points_in_radius( bounds_center_point, bounds_radius );
    const tripoint_range<tripoint> points_in_radius = here.points_in_radius( point, radius );
    int dist = rl_dist( camera_pos, point );

    bool item_found = false;
    std::unordered_set<const vehicle *> local_vehicles_recorded( vehicles_recorded );
    object_names_collection ret_obj;

    std::string description_part_on_figure;
    std::string description_furniture_on_figure;
    std::string description_terrain_on_figure;

    // store objects in radius
    for( const tripoint &point_around_figure : points_in_radius ) {
        if( !bounds.is_point_inside( point_around_figure ) ||
            !here.sees( camera_pos, point_around_figure, dist + radius ) ||
            ( ignored_points.find( point_around_figure ) != ignored_points.end() &&
              !( point_around_figure == point && create_figure_desc ) ) ) {
            continue; // disallow photos with not visible objects
        }
        units::volume volume_to_search = point_around_figure == bounds_center_point ? 0_ml :
                                         min_visible_volume;

        std::string furn_desc = colorized_feature_description_at( point_around_figure, item_found,
                                volume_to_search );

        const item item = get_top_item_at_point( point_around_figure, volume_to_search );

        const optional_vpart_position veh_part_pos = here.veh_at( point_around_figure );
        std::string unusual_ter_desc = colorized_ter_name_flags_at( point_around_figure,
                                       camera_ter_whitelist_flags,
                                       camera_ter_whitelist_types );
        std::string ter_desc = colorized_ter_name_flags_at( point_around_figure );

        const std::string trap_name = colorized_trap_name_at( point_around_figure );
        const std::string field_desc = colorized_field_description_at( point_around_figure );

        auto augment_description = [&]( std::string & desc ) {
            desc = str_cat( trap_name, desc, field_desc );
        };

        if( !furn_desc.empty() ) {
            augment_description( furn_desc );
            if( point == point_around_figure && create_figure_desc ) {
                description_furniture_on_figure = furn_desc;
            } else {
                ret_obj.furniture[ furn_desc ] ++;
            }
        } else if( veh_part_pos.has_value() ) {
            const vehicle &veh = veh_part_pos->vehicle();
            const std::string veh_name = colorize( veh.disp_name(), c_light_blue );
            const vehicle *veh_hash = &veh_part_pos->vehicle();

            if( local_vehicles_recorded.find( veh_hash ) == local_vehicles_recorded.end() &&
                point != point_around_figure ) {
                // new vehicle, point is not center
                ret_obj.vehicles[ veh_name ] ++;
            } else if( point == point_around_figure ) {
                // point is center
                //~ %1$s: vehicle part name, %2$s: vehicle name
                description_part_on_figure = string_format( pgettext( "vehicle part", "%1$s from %2$s" ),
                                             veh_part_pos.part_displayed()->part().name(), veh_name );
                if( ret_obj.vehicles.find( veh_name ) != ret_obj.vehicles.end() &&
                    local_vehicles_recorded.find( veh_hash ) != local_vehicles_recorded.end() ) {
                    // remove vehicle name only if we previously added THIS vehicle name (in case of same name)
                    ret_obj.vehicles[ veh_name ] --;
                    if( ret_obj.vehicles[ veh_name ] <= 0 ) {
                        ret_obj.vehicles.erase( veh_name );
                    }
                }
            }
            vehicles_recorded.insert( veh_hash );
            local_vehicles_recorded.insert( veh_hash );
        } else if( !item.is_null() ) {
            std::string item_name = colorized_item_name( item );
            augment_description( item_name );
            if( point == point_around_figure && create_figure_desc ) {
                //~ %1$s: terrain description, %2$s: item name
                description_terrain_on_figure = string_format( pgettext( "terrain and item", "%1$s with a %2$s" ),
                                                ter_desc, item_name );
            } else {
                ret_obj.items[ item_name ] ++;
            }
        } else if( !unusual_ter_desc.empty() ) {
            augment_description( unusual_ter_desc );
            if( point == point_around_figure && create_figure_desc ) {
                description_furniture_on_figure = unusual_ter_desc;
            } else {
                ret_obj.furniture[ unusual_ter_desc ] ++;
            }
        } else if( !ter_desc.empty() && ( !field_desc.empty() || !trap_name.empty() ) ) {
            augment_description( ter_desc );
            if( point == point_around_figure && create_figure_desc ) {
                description_terrain_on_figure = ter_desc;
            } else {
                ret_obj.terrain[ ter_desc ] ++;
            }
        } else {
            augment_description( ter_desc );
            if( point == point_around_figure && create_figure_desc ) {
                description_terrain_on_figure = ter_desc;
            }
        }
        ignored_points.insert( point_around_figure );
    }

    if( create_figure_desc ) {
        std::vector<std::string> objects_combined_desc;
        int objects_combined_num = 0;
        std::array<std::unordered_map<std::string, int>, 4> vecs_to_retrieve = {
            ret_obj.furniture, ret_obj.vehicles, ret_obj.items, ret_obj.terrain
        };

        for( int i = 0; i < 4; i++ ) {
            for( const auto &p : vecs_to_retrieve[ i ] ) {
                objects_combined_desc.push_back( i == 1 ?  // vehicle name already includes "the"
                                                 format_object_pair_no_article( p ) : format_object_pair_article( p ) );
                objects_combined_num += p.second;
            }
        }

        const char *transl_str = pgettext( "someone stands/sits *on* something", " on a %s." );
        if( !description_part_on_figure.empty() ) {
            ret_obj.figure_text = string_format( transl_str, description_part_on_figure );
        } else {
            if( !description_furniture_on_figure.empty() ) {
                ret_obj.figure_text = string_format( transl_str, description_furniture_on_figure );
            } else {
                ret_obj.figure_text = string_format( transl_str, description_terrain_on_figure );
            }
        }
        if( !objects_combined_desc.empty() ) {
            // store objects to description_figures_status
            std::string objects_text = enumerate_as_string( objects_combined_desc );
            ret_obj.obj_nearby_text = string_format( n_gettext( "Nearby is %s.", "Nearby are %s.",
                                      objects_combined_num ), objects_text );
        }
    }
    return ret_obj;
}

static item::extended_photo_def photo_def_for_camera_point( const tripoint &aim_point,
        const tripoint &camera_pos,
        std::vector<monster *> &monster_vec, std::vector<Character *> &character_vec )
{
    // look for big items on top of stacks in the background for the selfie description
    const units::volume min_visible_volume = 490_ml;

    std::unordered_set<tripoint> ignored_points;
    std::unordered_set<const vehicle *> vehicles_recorded;

    std::unordered_map<std::string, std::string> description_figures_appearance;
    std::vector<std::pair<std::string, std::string>> description_figures_status;

    std::string timestamp = to_string( time_point( calendar::turn ) );
    int dist = rl_dist( camera_pos, aim_point );
    map &here = get_map();
    const tripoint_range<tripoint> bounds = here.points_in_radius( aim_point, 2 );
    item::extended_photo_def photo;
    bool need_store_weather = false;
    int outside_tiles_num = 0;
    int total_tiles_num = 0;

    const auto map_deincrement_or_erase = []( std::unordered_map<std::string, int> &obj_map,
    const std::string & key ) {
        if( obj_map.find( key ) != obj_map.end() ) {
            obj_map[ key ] --;
            if( obj_map[ key ] <= 0 ) {
                obj_map.erase( key );
            }
        }
    };

    creature_tracker &creatures = get_creature_tracker();
    // first scan for critters and mark nearby furniture, vehicles and items
    for( const tripoint &current : bounds ) {
        if( !here.sees( camera_pos, current, dist + 3 ) ) {
            continue; // disallow photos with non-visible objects
        }
        monster *const mon = creatures.creature_at<monster>( current, false );
        Character *guy = creatures.creature_at<Character>( current );

        total_tiles_num++;
        if( here.is_outside( current ) ) {
            need_store_weather = true;
            outside_tiles_num++;
        }

        if( guy || mon ) {
            std::string figure_appearance;
            std::string figure_name;
            std::string pose;
            std::string pronoun_gender;
            std::string figure_effects;
            Creature *creature;
            if( mon && mon->has_effect( effect_ridden ) ) {
                // only player can ride, see monexamine::mount_pet
                guy = &get_avatar();
                description_figures_appearance[ mon->name() ] = "\"" + mon->type->get_description() + "\"";
            }

            if( guy ) {
                if( guy->is_hallucination() ) {
                    continue; // do not include hallucinations
                }
                if( guy->is_crouching() ) {
                    pose = _( "sits" );
                } else {
                    pose = _( "stands" );
                }
                const std::vector<std::string> vec = guy->short_description_parts();
                figure_appearance = string_join( vec, "\n\n" );
                figure_name = guy->get_name();
                pronoun_gender = guy->male ? _( "He" ) : _( "She" );
                creature = guy;
                character_vec.push_back( guy );
            } else {
                if( mon->is_hallucination() || mon->type->in_species( species_HALLUCINATION ) ) {
                    continue; // do not include hallucinations
                }
                pose = _( "stands" );
                figure_appearance = "\"" + mon->type->get_description() + "\"";
                figure_name = mon->name();
                pronoun_gender = pgettext( "Pronoun", "It" );
                creature = mon;
                monster_vec.push_back( mon );
            }

            figure_effects = effects_description_for_creature( creature, pose, pronoun_gender );
            description_figures_appearance[ figure_name ] = figure_appearance;

            object_names_collection obj_collection = enumerate_objects_around_point( current, 1, aim_point, 2,
                    camera_pos, min_visible_volume, true,
                    ignored_points, vehicles_recorded );
            std::string figure_text = pose + obj_collection.figure_text;

            if( !figure_effects.empty() ) {
                figure_text += " " + figure_effects;
            }
            if( !obj_collection.obj_nearby_text.empty() ) {
                figure_text += " " + obj_collection.obj_nearby_text;
            }
            auto name_text_pair = std::pair<std::string, std::string>( figure_name, figure_text );
            if( current == aim_point ) {
                description_figures_status.insert( description_figures_status.begin(), name_text_pair );
            } else {
                description_figures_status.push_back( name_text_pair );
            }
        }
    }

    // scan for everything NOT near critters
    object_names_collection obj_coll = enumerate_objects_around_point( aim_point, 2, aim_point, 2,
                                       camera_pos, min_visible_volume, false,
                                       ignored_points, vehicles_recorded );

    std::string photo_text = _( "This is a photo of " );

    bool found_item_aim_point;
    std::string furn_desc = colorized_feature_description_at( aim_point, found_item_aim_point,
                            0_ml );
    const item item = get_top_item_at_point( aim_point, 0_ml );
    const std::string trap_name = colorized_trap_name_at( aim_point );
    std::string ter_name = colorized_ter_name_flags_at( aim_point, {}, {} );
    const std::string field_desc = colorized_field_description_at( aim_point );

    bool found_vehicle_aim_point = here.veh_at( aim_point ).has_value();
    bool found_furniture_aim_point = !furn_desc.empty();
    // colorized_feature_description_at do not update flag if no furniture found, so need to check again
    if( !found_furniture_aim_point ) {
        found_item_aim_point = !item.is_null();
    }

    const ter_id ter_aim = here.ter( aim_point );
    const furn_id furn_aim = here.furn( aim_point );

    if( !description_figures_status.empty() ) {
        std::string names = enumerate_as_string( description_figures_status.begin(),
                            description_figures_status.end(),
        []( const std::pair<std::string, std::string> &it ) {
            return colorize( it.first, c_light_blue );
        } );

        photo.name = names;
        photo_text += names + ".";

        for( const auto &figure_status : description_figures_status ) {
            photo_text += "\n\n" + colorize( figure_status.first, c_light_blue )
                          + " " + figure_status.second;
        }
    } else if( found_vehicle_aim_point ) {
        const optional_vpart_position veh_part_pos = here.veh_at( aim_point );
        const std::string veh_name = colorize( veh_part_pos->vehicle().disp_name(), c_light_blue );
        photo.name = veh_name;
        photo_text += veh_name + ".";
        map_deincrement_or_erase( obj_coll.vehicles, veh_name );
    } else if( found_furniture_aim_point || found_item_aim_point )  {
        std::string item_name = colorized_item_name( item );
        if( found_furniture_aim_point ) {
            furn_desc = trap_name + furn_desc + field_desc;
            photo.name = furn_desc;
            photo_text += photo.name + ".";
            map_deincrement_or_erase( obj_coll.furniture, furn_desc );
        } else if( found_item_aim_point ) {
            item_name = trap_name + item_name + field_desc;
            photo.name = item_name;
            photo_text += item_name + ". " + string_format( _( "It lies on the %s." ),
                          ter_name );
            map_deincrement_or_erase( obj_coll.items, item_name );
        }
        if( found_furniture_aim_point && !furn_aim->description.empty() ) {
            photo_text += "\n\n" + colorize( furn_aim->name(), c_yellow ) + ":\n" + furn_aim->description;
        }
        if( found_item_aim_point ) {
            photo_text += "\n\n" + item_name + ":\n" + colorized_item_description( item );
        }
    } else {
        ter_name = trap_name + ter_name + field_desc;
        photo.name = ter_name;
        photo_text += photo.name + ".";
        map_deincrement_or_erase( obj_coll.terrain, ter_name );
        map_deincrement_or_erase( obj_coll.furniture, ter_name );

        if( !ter_aim->description.empty() ) {
            photo_text += "\n\n" + photo.name + ":\n" + ter_aim->description;
        }
    }

    auto num_of = []( const std::unordered_map<std::string, int> &m ) -> int {
        int ret = 0;
        for( const auto &it : m )
        {
            ret += it.second;
        }
        return ret;
    };

    if( !obj_coll.items.empty() ) {
        std::string obj_list = enumerate_as_string( obj_coll.items.begin(), obj_coll.items.end(),
                               format_object_pair_article );
        photo_text += "\n\n" + string_format( n_gettext( "There is something lying on the ground: %s.",
                                              "There are some things lying on the ground: %s.", num_of( obj_coll.items ) ),
                                              obj_list );
    }
    if( !obj_coll.furniture.empty() ) {
        std::string obj_list = enumerate_as_string( obj_coll.furniture.begin(), obj_coll.furniture.end(),
                               format_object_pair_article );
        photo_text += "\n\n" + string_format( n_gettext( "Something is visible in the background: %s.",
                                              "Some objects are visible in the background: %s.", num_of( obj_coll.furniture ) ),
                                              obj_list );
    }
    if( !obj_coll.vehicles.empty() ) {
        std::string obj_list = enumerate_as_string( obj_coll.vehicles.begin(), obj_coll.vehicles.end(),
                               format_object_pair_no_article );
        photo_text += "\n\n" + string_format( n_gettext( "There is %s parked in the background.",
                                              "There are %s parked in the background.", num_of( obj_coll.vehicles ) ),
                                              obj_list );
    }
    if( !obj_coll.terrain.empty() ) {
        std::string obj_list = enumerate_as_string( obj_coll.terrain.begin(), obj_coll.terrain.end(),
                               format_object_pair_article );
        photo_text += "\n\n" + string_format( n_gettext( "There is %s in the background.",
                                              "There are %s in the background.", num_of( obj_coll.terrain ) ),
                                              obj_list );
    }

    // TODO: fix point types
    const oter_id &cur_ter =
        overmap_buffer.ter( tripoint_abs_omt( ms_to_omt_copy( here.getabs( aim_point ) ) ) );
    std::string overmap_desc = string_format( _( "In the background you can see a %s." ),
                               colorize( cur_ter->get_name(), cur_ter->get_color() ) );
    if( outside_tiles_num == total_tiles_num ) {
        photo_text += _( "\n\nThis photo was taken <color_dark_gray>outside</color>." );
    } else if( outside_tiles_num == 0 ) {
        photo_text += _( "\n\nThis photo was taken <color_dark_gray>inside</color>." );
        overmap_desc += _( " interior" );
    } else if( outside_tiles_num < total_tiles_num / 2.0 ) {
        photo_text += _( "\n\nThis photo was taken mostly <color_dark_gray>inside</color>,"
                         " but <color_dark_gray>outside</color> can be seen." );
        overmap_desc += _( " interior" );
    } else if( outside_tiles_num >= total_tiles_num / 2.0 ) {
        photo_text += _( "\n\nThis photo was taken mostly <color_dark_gray>outside</color>,"
                         " but <color_dark_gray>inside</color> can be seen." );
    }
    photo_text += "\n" + overmap_desc + ".";

    if( get_map().get_abs_sub().z() >= 0 && need_store_weather ) {
        photo_text += "\n\n";
        if( is_dawn( calendar::turn ) ) {
            photo_text += _( "It is <color_yellow>sunrise</color>. " );
        } else if( is_dusk( calendar::turn ) ) {
            photo_text += _( "It is <color_light_red>sunset</color>. " );
        } else if( is_night( calendar::turn ) ) {
            photo_text += _( "It is <color_dark_gray>night</color>. " );
        } else {
            photo_text += _( "It is day. " );
        }
        photo_text += string_format( _( "The weather is %s." ), colorize( get_weather().weather_id->name,
                                     get_weather().weather_id->color ) );
    }

    for( const auto &figure : description_figures_appearance ) {
        photo_text += "\n\n" + string_format( _( "%s appearance:" ),
                                              colorize( figure.first, c_light_blue ) ) + "\n" + figure.second;
    }

    photo_text += "\n\n" + string_format( pgettext( "Date", "The photo was taken on %s." ),
                                          colorize( timestamp, c_light_blue ) );

    photo.description = photo_text;

    return photo;
}

static void item_save_monsters( Character &p, item &it, const std::vector<monster *> &monster_vec,
                                const int photo_quality )
{
    std::string monster_photos = it.get_var( "CAMERA_MONSTER_PHOTOS" );
    if( monster_photos.empty() ) {
        monster_photos = ",";
    }

    for( monster * const &monster_p : monster_vec ) {
        const std::string mtype = monster_p->type->id.str();
        const std::string name = monster_p->name();

        // position of <monster type string>
        const size_t mon_str_pos = monster_photos.find( "," + mtype + "," );

        // monster gets recorded by the character, add to known types
        p.set_knows_creature_type( monster_p->type->id );

        if( mon_str_pos == std::string::npos ) { // new monster
            monster_photos += string_format( "%s,%d,", mtype, photo_quality );
        } else { // replace quality character, if new photo is better
            const size_t quality_num_pos = mon_str_pos + mtype.size() + 2;
            const size_t next_comma = monster_photos.find( ',', quality_num_pos );
            const int old_quality =
                get_quality_from_string( monster_photos.substr( quality_num_pos, next_comma - quality_num_pos ) );

            if( photo_quality > old_quality ) {
                const std::string quality_s = string_format( "%d", photo_quality );
                cata_assert( quality_s.size() == 1 );
                monster_photos[quality_num_pos] = quality_s.front();
            }
            if( !p.is_blind() ) {
                if( photo_quality > old_quality ) {
                    p.add_msg_if_player( m_good, _( "The quality of %s image is better than the previous one." ),
                                         colorize( name, c_light_blue ) );
                } else if( old_quality == 5 ) {
                    p.add_msg_if_player( _( "The quality of the stored %s image is already maximally detailed." ),
                                         colorize( name, c_light_blue ) );
                } else {
                    p.add_msg_if_player( m_bad, _( "But the quality of %s image is worse than the previous one." ),
                                         colorize( name, c_light_blue ) );
                }
            }
        }
    }
    it.set_var( "CAMERA_MONSTER_PHOTOS", monster_photos );
}

// throws exception
bool item::read_extended_photos( std::vector<extended_photo_def> &extended_photos,
                                 const std::string &var_name, bool insert_at_begin ) const
{
    bool result = false;
    std::optional<JsonValue> json_opt = json_loader::from_string_opt( get_var( var_name ) );
    if( !json_opt.has_value() ) {
        return result;
    }
    JsonValue &json = *json_opt;
    if( insert_at_begin ) {
        std::vector<extended_photo_def> temp_vec;
        result = json.read( temp_vec );
        extended_photos.insert( std::begin( extended_photos ), std::begin( temp_vec ),
                                std::end( temp_vec ) );
    } else {
        result = json.read( extended_photos );
    }
    return result;
}

// throws exception
void item::write_extended_photos( const std::vector<extended_photo_def> &extended_photos,
                                  const std::string &var_name )
{
    std::ostringstream extended_photos_data;
    JsonOut json( extended_photos_data );
    json.write( extended_photos );
    set_var( var_name, extended_photos_data.str() );
}

static bool show_photo_selection( Character &p, item &it, const std::string &var_name )
{
    if( p.is_blind() ) {
        p.add_msg_if_player( _( "You can't see the camera screen, you're blind." ) );
        return false;
    }

    uilist pmenu;
    pmenu.text = _( "Photos saved on camera:" );

    std::vector<std::string> descriptions;
    std::vector<item::extended_photo_def> extended_photos;

    try {
        it.read_extended_photos( extended_photos, var_name, false );
    } catch( const JsonError &e ) {
        debugmsg( "Error reading photos: %s", e.c_str() );
    }
    try { // if there is old photos format, append them; delete old and save new
        if( it.read_extended_photos( extended_photos, "CAMERA_NPC_PHOTOS", true ) ) {
            it.erase_var( "CAMERA_NPC_PHOTOS" );
            it.write_extended_photos( extended_photos, var_name );
        }
    } catch( const JsonError &e ) {
        debugmsg( "Error migrating old photo format: %s", e.c_str() );
    }

    int k = 0;
    for( const item::extended_photo_def &extended_photo : extended_photos ) {
        std::string menu_str = extended_photo.name;

        size_t index = menu_str.find( p.name );
        if( index != std::string::npos ) {
            menu_str.replace( index, p.name.length(), _( "You" ) );
        }

        descriptions.push_back( extended_photo.description );
        menu_str += " [" + photo_quality_name( extended_photo.quality ) + "]";

        pmenu.addentry( k++, true, -1, menu_str.c_str() );
    }

    int choice;
    do {
        pmenu.query();
        choice = pmenu.ret;

        if( choice < 0 ) {
            break;
        }
        popup( "%s", descriptions[choice].c_str() );

    } while( true );
    return true;
}

std::optional<int> iuse::camera( Character *p, item *it, const tripoint & )
{
    enum {c_shot, c_photos, c_monsters, c_upload};

    // From item processing
    if( !p ) {
        debugmsg( "%s called action camera that requires character but no character is present",
                  it->typeId().str() );
        return std::nullopt;
    }

    // CAMERA_NPC_PHOTOS is old save variable
    bool found_extended_photos = !it->get_var( "CAMERA_NPC_PHOTOS" ).empty() ||
                                 !it->get_var( "CAMERA_EXTENDED_PHOTOS" ).empty();
    bool found_monster_photos = !it->get_var( "CAMERA_MONSTER_PHOTOS" ).empty();

    uilist amenu;
    amenu.text = _( "What to do with camera?" );
    amenu.addentry( c_shot, true, 't', _( "Take a photo" ) );
    if( !found_extended_photos && !found_monster_photos ) {
        amenu.addentry( c_photos, false, 'l', _( "No photos in memory" ) );
    } else {
        if( found_extended_photos ) {
            amenu.addentry( c_photos, true, 'l', _( "List photos" ) );
        }
        if( found_monster_photos ) {
            amenu.addentry( c_monsters, true, 'm', _( "Your collection of monsters" ) );
        }
        amenu.addentry( c_upload, true, 'u', _( "Upload photos to memory card" ) );
    }

    amenu.query();
    const int choice = amenu.ret;

    if( choice < 0 ) {
        return std::nullopt;
    }

    map &here = get_map();
    creature_tracker &creatures = get_creature_tracker();
    if( c_shot == choice ) {
        const std::optional<tripoint> aim_point_ = g->look_around();

        if( !aim_point_ ) {
            p->add_msg_if_player( _( "Never mind." ) );
            return std::nullopt;
        }
        tripoint aim_point = *aim_point_;
        bool incorrect_focus = false;
        tripoint_range<tripoint> aim_bounds = here.points_in_radius( aim_point, 2 );

        std::vector<tripoint> trajectory = line_to( p->pos(), aim_point, 0, 0 );
        trajectory.push_back( aim_point );

        p->moves -= 50;
        sounds::sound( p->pos(), 8, sounds::sound_t::activity, _( "Click." ), true, "tool",
                       "camera_shutter" );

        for( std::vector<tripoint>::iterator point_it = trajectory.begin();
             point_it != trajectory.end();
             ++point_it ) {
            const tripoint trajectory_point = *point_it;
            if( point_it != trajectory.end() ) {
                const tripoint next_point = *( point_it + 1 ); // Trajectory ends on last visible tile
                if( !here.sees( p->pos(), next_point, rl_dist( p->pos(), next_point ) + 3 ) ) {
                    p->add_msg_if_player( _( "You have the wrong camera focus." ) );
                    incorrect_focus = true;
                    // recalculate target point
                    aim_point = trajectory_point;
                    aim_bounds = here.points_in_radius( trajectory_point, 2 );
                }
            }

            monster *const mon = creatures.creature_at<monster>( trajectory_point, true );
            Character *const guy = creatures.creature_at<Character>( trajectory_point );
            if( mon || guy || trajectory_point == aim_point ) {
                int dist = rl_dist( p->pos(), trajectory_point );

                int camera_bonus = it->has_flag( flag_CAMERA_PRO ) ? 10 : 0;
                int photo_quality = 20 - rng( dist, dist * 2 ) * 2 + rng( camera_bonus / 2, camera_bonus );
                if( photo_quality > 5 ) {
                    photo_quality = 5;
                }
                if( photo_quality < 0 ) {
                    photo_quality = 0;
                }
                if( p->is_blind() ) {
                    photo_quality /= 2;
                }

                if( mon ) {
                    monster &z = *mon;

                    // shoot past small monsters and hallucinations
                    if( trajectory_point != aim_point && ( z.type->size <= creature_size::small ||
                                                           z.is_hallucination() ||
                                                           z.type->in_species( species_HALLUCINATION ) ) ) {
                        continue;
                    }
                    if( !aim_bounds.is_point_inside( trajectory_point ) ) {
                        // take a photo of the monster that's in the way
                        p->add_msg_if_player( m_warning, _( "A %s got in the way of your photo." ), z.name() );
                        incorrect_focus = true;
                    } else if( trajectory_point != aim_point ) { // shoot past mon that will be in photo anyway
                        continue;
                    }
                    // get a special message if the target is a hallucination
                    if( trajectory_point == aim_point && ( z.is_hallucination() ||
                                                           z.type->in_species( species_HALLUCINATION ) ) ) {
                        p->add_msg_if_player( _( "Strange… there's nothing in the center of this picture?" ) );
                    }
                } else if( guy ) {
                    if( trajectory_point == aim_point && guy->is_hallucination() ) {
                        p->add_msg_if_player( _( "Strange… %s isn't visible on the picture?" ), guy->get_name() );
                    } else if( !aim_bounds.is_point_inside( trajectory_point ) ) {
                        // take a photo of the monster that's in the way
                        p->add_msg_if_player( m_warning, _( "%s got in the way of your photo." ), guy->get_name() );
                        incorrect_focus = true;
                    } else if( trajectory_point != aim_point ) {  // shoot past guy that will be in photo anyway
                        continue;
                    }
                }
                if( incorrect_focus ) {
                    photo_quality = photo_quality == 0 ? 0 : photo_quality - 1;
                }

                std::vector<item::extended_photo_def> extended_photos;
                std::vector<monster *> monster_vec;
                std::vector<Character *> character_vec;
                item::extended_photo_def photo = photo_def_for_camera_point( trajectory_point, p->pos(),
                                                 monster_vec, character_vec );
                photo.quality = photo_quality;

                try {
                    it->read_extended_photos( extended_photos, "CAMERA_EXTENDED_PHOTOS", false );
                    extended_photos.push_back( photo );
                    it->write_extended_photos( extended_photos, "CAMERA_EXTENDED_PHOTOS" );
                } catch( const JsonError &e ) {
                    debugmsg( "Error when adding new photo (loaded photos = %i): %s", extended_photos.size(),
                              e.c_str() );
                }

                const bool selfie = std::find( character_vec.begin(), character_vec.end(),
                                               p ) != character_vec.end();

                if( selfie ) {
                    p->add_msg_if_player( _( "You took a selfie." ) );
                } else {
                    if( p->is_blind() ) {
                        p->add_msg_if_player( _( "You took a photo of %s." ), photo.name );
                    } else {
                        p->add_msg_if_player( _( "You took a photo of %1$s. It is %2$s." ), photo.name,
                                              photo_quality_name( photo_quality ) );
                    }
                    std::vector<std::string> blinded_names;
                    for( monster * const &monster_p : monster_vec ) {
                        if( dist < 4 && one_in( dist + 2 ) && monster_p->has_flag( mon_flag_SEES ) ) {
                            monster_p->add_effect( effect_blind, rng( 5_turns, 10_turns ) );
                            blinded_names.push_back( monster_p->name() );
                        }
                    }
                    for( Character * const &character_p : character_vec ) {
                        if( dist < 4 && one_in( dist + 2 ) && !character_p->is_blind() ) {
                            character_p->add_effect( effect_blind, rng( 5_turns, 10_turns ) );
                            blinded_names.push_back( character_p->get_name() );
                        }
                    }
                    if( !blinded_names.empty() ) {
                        p->add_msg_if_player( _( "%s looks blinded." ), enumerate_as_string( blinded_names.begin(),
                        blinded_names.end(), []( const std::string & it ) {
                            return colorize( it, c_light_blue );
                        } ) );
                    }
                }
                if( !monster_vec.empty() ) {
                    item_save_monsters( *p, *it, monster_vec, photo_quality );
                }
                return 1;
            }
        }
        return 1;
    }

    if( c_photos == choice ) {
        show_photo_selection( *p, *it, "CAMERA_EXTENDED_PHOTOS" );
        return 1;
    }

    if( c_monsters == choice ) {
        if( p->is_blind() ) {
            p->add_msg_if_player( _( "You can't see the camera screen, you're blind." ) );
            return 0;
        }
        uilist pmenu;

        pmenu.text = _( "Your collection of monsters:" );

        std::vector<mtype_id> monster_photos;
        std::vector<std::string> descriptions;

        std::istringstream f_mon( it->get_var( "CAMERA_MONSTER_PHOTOS" ) );
        std::string s;
        int k = 0;
        while( getline( f_mon, s, ',' ) ) {

            if( s.empty() ) {
                continue;
            }

            monster_photos.emplace_back( s );

            std::string menu_str;

            const monster dummy( monster_photos.back() );
            menu_str = dummy.name();
            descriptions.push_back( dummy.type->get_description() );

            getline( f_mon, s, ',' );
            const int quality = get_quality_from_string( s );

            menu_str += " [" + photo_quality_name( quality ) + "]";

            pmenu.addentry( k++, true, -1, menu_str.c_str() );
        }

        int choice;
        do {
            pmenu.query();
            choice = pmenu.ret;

            if( choice < 0 ) {
                break;
            }

            popup( "%s", descriptions[choice].c_str() );

        } while( true );

        return 1;
    }

    if( c_upload == choice ) {

        if( p->is_blind() ) {
            p->add_msg_if_player( _( "You can't see the camera screen, you're blind." ) );
            return std::nullopt;
        }

        p->moves -= to_moves<int>( 2_seconds );

        avatar *you = p->as_avatar();
        item_location loc;
        if( you != nullptr ) {
            loc = game_menus::inv::titled_filter_menu( []( const item & it ) {
                return it.has_flag( flag_MC_MOBILE );
            }, *you, _( "Insert memory card" ) );
        }
        if( !loc ) {
            p->add_msg_if_player( m_info, _( "You don't have that item!" ) );
            return 1;
        }
        item &mc = *loc;

        if( mc.has_flag( flag_MC_HAS_DATA ) ) {
            if( !query_yn( _( "Are you sure you want to clear the old data on the card?" ) ) ) {
                return 1;
            }
        }

        mc.convert( itype_memory_card );
        mc.clear_vars();
        mc.unset_flags();
        mc.set_flag( flag_MC_HAS_DATA );

        mc.set_var( "MC_MONSTER_PHOTOS", it->get_var( "CAMERA_MONSTER_PHOTOS" ) );
        mc.set_var( "MC_EXTENDED_PHOTOS", it->get_var( "CAMERA_EXTENDED_PHOTOS" ) );
        p->add_msg_if_player( m_info,
                              _( "You upload your photos and monster collection to the memory card." ) );

        return 1;
    }

    return 1;
}

std::optional<int> iuse::ehandcuffs_tick( Character *p, item *it, const tripoint &pos )
{

    if( get_map().has_flag( ter_furn_flag::TFLAG_SWIMMABLE, pos.xy() ) ) {
        it->unset_flag( flag_NO_UNWIELD );
        it->ammo_unset();
        it->active = false;
        add_msg( m_good, _( "%s automatically turned off!" ), it->tname() );
        return 1;
    }

    if( !p ) {
        // Active but not in use. Deactivate
        sounds::sound( pos, 2, sounds::sound_t::combat, "Click.", true, "tools", "handcuffs" );
        it->unset_flag( flag_NO_UNWIELD );
        it->active = false;
        return 1;
    }

    if( it->charges == 0 ) {
        sounds::sound( pos, 2, sounds::sound_t::combat, "Click.", true, "tools", "handcuffs" );
        it->unset_flag( flag_NO_UNWIELD );
        it->active = false;

        if( p ) {
            add_msg( m_good, _( "%s on your wrists opened!" ), it->tname() );
        }

        return 1;
    }

    if( p->has_active_bionic( bio_shock ) && p->get_power_level() >= bio_shock->power_trigger &&
        one_in( 5 ) ) {
        p->mod_power_level( -bio_shock->power_trigger );

        it->unset_flag( flag_NO_UNWIELD );
        it->charges = 0;
        it->active = false;
        add_msg( m_good, _( "The %s crackle with electricity from your bionic, then come off your hands!" ),
                 it->tname() );

        return 1;
    }

    if( calendar::once_every( 1_minutes ) ) {
        sounds::sound( pos, 10, sounds::sound_t::alarm, _( "a police siren, whoop WHOOP." ), true,
                       "environment", "police_siren" );
    }

    const point p2( it->get_var( "HANDCUFFS_X", 0 ), it->get_var( "HANDCUFFS_Y", 0 ) );

    if( ( it->ammo_remaining() > it->type->maximum_charges() - 1000 ) && ( p2.x != pos.x ||
            p2.y != pos.y ) ) {

        if( p->is_elec_immune() ) {
            if( one_in( 10 ) ) {
                add_msg( m_good, _( "The cuffs try to shock you, but you're protected from electricity." ) );
            }
        } else {
            add_msg( m_bad, _( "Ouch, the cuffs shock you!" ) );

            p->apply_damage( nullptr, bodypart_id( "arm_l" ), rng( 0, 2 ) );
            p->apply_damage( nullptr, bodypart_id( "arm_r" ), rng( 0, 2 ) );
            p->mod_pain( rng( 2, 5 ) );

        }

        it->charges -= 50;
        if( it->charges < 1 ) {
            it->charges = 1;
        }

        it->set_var( "HANDCUFFS_X", pos.x );
        it->set_var( "HANDCUFFS_Y", pos.y );

        return 1;

    }

    return 1;
}

std::optional<int> iuse::ehandcuffs( Character *, item *it, const tripoint & )
{
    if( it->active ) {
        add_msg( _( "The %s are clamped tightly on your wrists.  You can't take them off." ),
                 it->tname() );
    } else {
        add_msg( _( "The %s have discharged and can be taken off." ), it->tname() );
    }

    return 1;
}

std::optional<int> iuse::afs_translocator( Character *p, item *it, const tripoint & )
{
    if( !it->ammo_sufficient( p ) ) {
        return std::nullopt;
    }

    const std::optional<tripoint> dest_ = choose_adjacent( _( "Create buoy where?" ) );
    if( !dest_ ) {
        return std::nullopt;
    }

    tripoint dest = *dest_;

    p->moves -= to_moves<int>( 2_seconds );

    map &here = get_map();
    if( here.impassable( dest ) || here.has_flag( ter_furn_flag::TFLAG_NO_FLOOR, dest ) ||
        here.has_furn( dest ) ) {
        add_msg( m_info, _( "The %s, emits a short angry beep." ), it->tname() );
        return std::nullopt;
    } else {
        here.furn_set( dest, furn_f_translocator_buoy );
        add_msg( m_info, _( "Space warps momentarily as the %s is created." ),
                 furn_f_translocator_buoy.obj().name() );
        return  1;
    }
}

std::optional<int> iuse::foodperson_voice( Character *, item *, const tripoint &pos )
{
    if( calendar::once_every( 1_minutes ) ) {
        const SpeechBubble &speech = get_speech( "foodperson_mask" );
        sounds::sound( pos, speech.volume, sounds::sound_t::alarm, speech.text.translated(), true, "speech",
                       "foodperson_mask" );
    }
    return 0;
}

std::optional<int> iuse::foodperson( Character *p, item *it, const tripoint & )
{
    // Prevent crash if battery was somehow removed.
    if( !it->magazine_current() ) {
        return std::nullopt;
    }

    time_duration shift = time_duration::from_turns( it->magazine_current()->ammo_remaining() *
                          it->type->tool->turns_per_charge );

    p->add_msg_if_player( m_info, _( "Your HUD lights-up: \"Your shift ends in %s\"." ),
                          to_string( shift ) );
    return 0;
}

std::optional<int> iuse::radiocar( Character *p, item *it, const tripoint & )
{
    int choice = -1;
    item *bomb_it = it->get_item_with( []( const item & c ) {
        return c.has_flag( flag_RADIOCARITEM );
    } );
    if( bomb_it == nullptr ) {
        choice = uilist( _( "Using RC car:" ), {
            _( "Turn on" ), _( "Put a bomb to car" )
        } );
    } else {
        choice = uilist( _( "Using RC car:" ), {
            _( "Turn on" ), bomb_it->tname()
        } );
    }
    if( choice < 0 ) {
        return std::nullopt;
    }

    if( choice == 0 ) { //Turn car ON
        if( !it->ammo_sufficient( p ) ) {
            p->add_msg_if_player( _( "The RC car's batteries seem to be dead." ) );
            return std::nullopt;
        }

        it->convert( itype_radio_car_on, p ).active = true;

        p->add_msg_if_player(
            _( "You turned on your RC car; now place it on the ground, and use your radio control to play." ) );

        return 0;
    }

    if( choice == 1 ) {

        if( bomb_it == nullptr ) { //arming car with bomb

            avatar *you = p->as_avatar();
            item_location loc;
            if( you != nullptr ) {
                loc = game_menus::inv::titled_filter_menu( []( const item & it ) {
                    return it.has_flag( flag_RADIOCARITEM );
                }, *you, _( "Arm what?" ) );
            }
            if( !loc ) {
                p->add_msg_if_player( m_info, _( "You don't have that item!" ) );
                return 0;
            }
            item &put = *loc;

            if( put.has_flag( flag_RADIOCARITEM ) && ( put.volume() <= 1250_ml ||
                    ( put.weight() <= 2_kilogram ) ) ) {
                p->moves -= to_moves<int>( 3_seconds );
                p->add_msg_if_player( _( "You armed your RC car with %s." ),
                                      put.tname() );
                it->put_in( p->i_rem( &put ), pocket_type::CONTAINER );
            } else if( !put.has_flag( flag_RADIOCARITEM ) ) {
                p->add_msg_if_player( _( "You want to arm your RC car with %s?  But how?" ),
                                      put.tname() );
            } else {
                p->add_msg_if_player( _( "Your %s is too heavy or bulky for this RC car." ),
                                      put.tname() );
            }
        } else { // Disarm the car
            p->moves -= to_moves<int>( 2_seconds );

            p->inv->assign_empty_invlet( *bomb_it, *p, true ); // force getting an invlet.
            p->i_add( *bomb_it );
            it->remove_item( *bomb_it );

            p->add_msg_if_player( _( "You disarmed your RC car." ) );
        }
    }

    return 1;
}

std::optional<int> iuse::radiocaron( Character *p, item *it, const tripoint & )
{
    if( !it->ammo_sufficient( p ) ) {
        // Deactivate since other mode has an iuse too.
        it->convert( itype_radio_car, p ).active = false;
        return 0;
    }

    int choice = uilist( _( "What to do with your activated RC car?" ), {
        _( "Turn off" )
    } );

    if( choice < 0 ) {
        return 1;
    }

    if( choice == 0 ) {
        it->convert( itype_radio_car, p ).active = false;

        p->add_msg_if_player( _( "You turned off your RC car." ) );
        return 1;
    }

    return 1;
}

static void sendRadioSignal( Character &p, const flag_id &signal )
{
    map &here = get_map();
    for( const tripoint &loc : here.points_in_radius( p.pos(), 60 ) ) {
        for( item &it : here.i_at( loc ) ) {
            if( it.has_flag( flag_RADIO_ACTIVATION ) && it.has_flag( signal ) ) {
                sounds::sound( p.pos(), 6, sounds::sound_t::alarm, _( "beep" ), true, "misc", "beep" );
                if( it.has_flag( flag_RADIO_INVOKE_PROC ) ) {
                    // Invoke to transform a radio-modded explosive into its active form
                    // The item activation may have all kinds of requirements. Like requiring item to be wielded.
                    // We do not care. Call item action directly without checking can_use.
                    // Items where this can be a problem should not be radio moddable
                    std::map<std::string, use_function> use_methods = it.type->use_methods;
                    if( use_methods.find( "transform" ) != use_methods.end() ) {
                        it.type->get_use( "transform" )->call( &p, it, loc );
                        item_location itm_loc = item_location( map_cursor( loc ), &it );
                        here.update_lum( itm_loc, true );
                    } else {
                        it.type->get_use( it.type->use_methods.begin()->first )->call( &p, it, loc );
                    }
                }
            } else if( !it.empty_container() ) {
                item *itm = it.get_item_with( [&signal]( const item & c ) {
                    return c.has_flag( signal );
                } );

                if( itm != nullptr ) {
                    sounds::sound( p.pos(), 6, sounds::sound_t::alarm, _( "beep" ), true, "misc", "beep" );
                    // Invoke to transform a radio-modded explosive into its active form
                    if( itm->has_flag( flag_RADIO_INVOKE_PROC ) ) {
                        itm->type->invoke( &p, *itm, loc );
                        item_location itm_loc = item_location( map_cursor( loc ), itm );
                        here.update_lum( itm_loc, true );
                    }
                }
            }
        }
    }
}

std::optional<int> iuse::radiocontrol_tick( Character *p, item *it, const tripoint & )
{
    if( !p ) {
        // Player has dropped the controller
        avatar &player = get_avatar();
        it->active = false;
        player.remove_value( "remote_controlling" );
        return 1;
    }
    if( !it->ammo_sufficient( p ) ) {
        it->active = false;
        p->remove_value( "remote_controlling" );
    } else if( p->get_value( "remote_controlling" ).empty() ) {
        it->active = false;
    }

    return 1;
}

std::optional<int> iuse::radiocontrol( Character *p, item *it, const tripoint & )
{
    const char *car_action = nullptr;

    if( !it->active ) {
        car_action = _( "Take control of RC car" );
    } else {
        car_action = _( "Stop controlling RC car" );
    }

    int choice = uilist( _( "What to do with the radio control?" ), {
        car_action,
        _( "Press red button" ), _( "Press blue button" ), _( "Press green button" )
    } );

    map &here = get_map();
    if( choice < 0 ) {
        return 0;
    } else if( choice == 0 ) {
        if( it->active ) {
            it->active = false;
            p->remove_value( "remote_controlling" );
        } else {
            std::list<std::pair<tripoint, item *>> rc_pairs = here.get_rc_items();
            tripoint rc_item_location = {999, 999, 999};
            // TODO: grab the closest car or similar?
            for( auto &rc_pairs_rc_pair : rc_pairs ) {
                if( rc_pairs_rc_pair.second->has_flag( flag_RADIOCAR ) &&
                    rc_pairs_rc_pair.second->active ) {
                    rc_item_location = rc_pairs_rc_pair.first;
                }
            }
            if( rc_item_location.x == 999 ) {
                p->add_msg_if_player( _( "No active RC cars on ground and in range." ) );
                return 1;
            } else {
                std::stringstream car_location_string;
                // Populate with the point and stash it.
                car_location_string << rc_item_location.x << ' ' <<
                                    rc_item_location.y << ' ' << rc_item_location.z;
                p->add_msg_if_player( m_good, _( "You take control of the RC car." ) );

                p->set_value( "remote_controlling", car_location_string.str() );
                it->active = true;
            }
        }
    } else {
        const flag_id signal( "RADIOSIGNAL_" + std::to_string( choice ) );

        if( p->cache_has_item_with( flag_BOMB, [&p, &signal]( const item & it ) {
        if( it.has_flag( flag_RADIO_ACTIVATION ) && it.has_flag( signal ) ) {
                p->add_msg_if_player( m_warning,
                                      _( "The %s in your inventory would explode on this signal.  Place it down before sending the signal." ),
                                      it.display_name() );
                return true;
            }
            return false;
        } ) ) {
            return std::nullopt;
        }

        if( p->cache_has_item_with( flag_RADIO_CONTAINER, [&p, &signal]( const item & it ) {
        const item *rad_cont = it.get_item_with( [&signal]( const item & c ) {
            return c.has_flag( flag_BOMB ) && c.has_flag( signal );
            } );
            if( rad_cont != nullptr ) {
                p->add_msg_if_player( m_warning,
                                      _( "The %1$s in your %2$s would explode on this signal.  Place it down before sending the signal." ),
                                      rad_cont->display_name(), it.display_name() );
                return true;
            }
            return false;
        } ) ) {
            return std::nullopt;
        }

        p->add_msg_if_player( _( "Click." ) );
        sendRadioSignal( *p, signal );
        p->moves -= to_moves<int>( 2_seconds );
    }

    return 1;
}

static bool hackveh( Character &p, item &it, vehicle &veh )
{
    if( !veh.is_locked || !veh.has_security_working() ) {
        return true;
    }
    const bool advanced = !empty( veh.get_avail_parts( "REMOTE_CONTROLS" ) );
    if( advanced && veh.is_alarm_on ) {
        p.add_msg_if_player( m_bad, _( "This vehicle's security system has locked you out!" ) );
        return false;
    }

    /** @EFFECT_INT increases chance of bypassing vehicle security system */

    /** @EFFECT_COMPUTER increases chance of bypassing vehicle security system */
    int roll = dice( round( p.get_skill_level( skill_computer ) ) + 2,
                     p.int_cur ) - ( advanced ? 50 : 25 );
    int effort = 0;
    bool success = false;
    if( roll < -20 ) { // Really bad rolls will trigger the alarm before you know it exists
        effort = 1;
        p.add_msg_if_player( m_bad, _( "You trigger the alarm!" ) );
        veh.is_alarm_on = true;
    } else if( roll >= 20 ) { // Don't bother the player if it's trivial
        effort = 1;
        p.add_msg_if_player( m_good, _( "You quickly bypass the security system!" ) );
        success = true;
    }

    if( effort == 0 && !query_yn( _( "Try to hack this car's security system?" ) ) ) {
        // Scanning for security systems isn't free
        p.moves -= to_moves<int>( 1_seconds );
        it.charges -= 1;
        return false;
    }

    p.practice( skill_computer, advanced ? 10 : 3 );
    if( roll < -10 ) {
        effort = rng( 4, 8 );
        p.add_msg_if_player( m_bad, _( "You waste some time, but fail to affect the security system." ) );
    } else if( roll < 0 ) {
        effort = 1;
        p.add_msg_if_player( m_bad, _( "You fail to affect the security system." ) );
    } else if( roll < 20 ) {
        effort = rng( 2, 8 );
        p.add_msg_if_player( m_mixed,
                             _( "You take some time, but manage to bypass the security system!" ) );
        success = true;
    }

    p.moves -= to_moves<int>( time_duration::from_seconds( effort ) );
    it.charges -= effort;
    if( success && advanced ) { // Unlock controls, but only if they're drive-by-wire
        veh.is_locked = false;
    }
    return success;
}

static vehicle *pickveh( const tripoint &center, bool advanced )
{
    static const std::string ctrl = "CTRL_ELECTRONIC";
    static const std::string advctrl = "REMOTE_CONTROLS";
    uilist pmenu;
    pmenu.title = _( "Select vehicle to access" );
    std::vector< vehicle * > vehs;

    for( wrapped_vehicle &veh : get_map().get_vehicles() ) {
        vehicle *&v = veh.v;
        if( rl_dist( center, v->global_pos3() ) < 40 &&
            v->fuel_left( itype_battery ) > 0 &&
            ( !empty( v->get_avail_parts( advctrl ) ) ||
              ( !advanced && !empty( v->get_avail_parts( ctrl ) ) ) ) ) {
            vehs.push_back( v );
        }
    }
    std::vector<tripoint> locations;
    for( int i = 0; i < static_cast<int>( vehs.size() ); i++ ) {
        vehicle *veh = vehs[i];
        locations.push_back( veh->global_pos3() );
        pmenu.addentry( i, true, MENU_AUTOASSIGN, veh->name );
    }

    if( vehs.empty() ) {
        add_msg( m_bad, _( "No vehicle available." ) );
        return nullptr;
    }
    if( vehs.size() == 1 ) {
        return vehs[0];
    }

    pointmenu_cb callback( locations );
    pmenu.callback = &callback;
    pmenu.w_y_setup = 0;
    pmenu.query();

    if( pmenu.ret < 0 || pmenu.ret >= static_cast<int>( vehs.size() ) ) {
        return nullptr;
    } else {
        return vehs[pmenu.ret];
    }
}

std::optional<int> iuse::remoteveh_tick( Character *p, item *it, const tripoint & )
{
    vehicle *remote = g->remoteveh();
    bool stop = false;
    if( !it->ammo_sufficient( p ) ) {
        p->add_msg_if_player( m_bad, _( "The remote control's battery goes dead." ) );
        stop = true;
    } else if( remote == nullptr ) {
        p->add_msg_if_player( _( "Lost contact with the vehicle." ) );
        stop = true;
    } else if( remote->fuel_left( itype_battery ) == 0 ) {
        p->add_msg_if_player( m_bad, _( "The vehicle's battery died." ) );
        stop = true;
    }
    if( stop ) {
        it->active = false;
        g->setremoteveh( nullptr );
    }

    return 1;
}

std::optional<int> iuse::remoteveh( Character *p, item *it, const tripoint &pos )
{
    vehicle *remote = g->remoteveh();

    bool controlling = it->active && remote != nullptr;
    int choice = uilist( _( "What to do with the remote vehicle control:" ), {
        controlling ? _( "Stop controlling the vehicle." ) : _( "Take control of a vehicle." ),
        _( "Execute one vehicle action" )
    } );

    if( choice < 0 || choice > 1 ) {
        return std::nullopt;
    }

    if( choice == 0 && controlling ) {
        it->active = false;
        g->setremoteveh( nullptr );
        return 0;
    }

    avatar &player_character = get_avatar();
    point p2( player_character.view_offset.xy() );

    vehicle *veh = pickveh( pos, choice == 0 );

    if( veh == nullptr ) {
        return std::nullopt;
    }

    if( !hackveh( *p, *it, *veh ) ) {
        return 0;
    }

    if( choice == 0 ) {
        if( p->has_trait( trait_WAYFARER ) ) {
            add_msg( m_info,
                     _( "Despite using a controller, you still refuse to take control of this vehicle." ) );
        } else {
            it->active = true;
            g->setremoteveh( veh );
            p->add_msg_if_player( m_good, _( "You take control of the vehicle." ) );
            if( !veh->engine_on ) {
                veh->start_engines();
            }
        }
    } else if( choice == 1 ) {
        const auto rctrl_parts = veh->get_avail_parts( "REMOTE_CONTROLS" );
        const auto electronics_parts = veh->get_avail_parts( "CTRL_ELECTRONIC" );
        // Revert to original behavior if we can't find remote controls.
        if( empty( rctrl_parts ) ) {
            veh->interact_with( electronics_parts.begin()->pos() );
        } else {
            veh->interact_with( rctrl_parts.begin()->pos() );
        }
    }

    player_character.view_offset.x = p2.x;
    player_character.view_offset.y = p2.y;
    return 1;
}

static bool multicooker_hallu( Character &p )
{
    p.moves -= to_moves<int>( 2_seconds );
    const int random_hallu = rng( 1, 7 );
    switch( random_hallu ) {

        case 1:
            add_msg( m_info, _( "And when you gaze long into a screen, the screen also gazes into you." ) );
            return true;

        case 2:
            add_msg( m_bad, _( "The multi-cooker boiled your head!" ) );
            return true;

        case 3:
            add_msg( m_info, _( "The characters on the screen display an obscene joke.  Strange humor." ) );
            return true;

        case 4:
            //~ Single-spaced & lowercase are intentional, conveying hurried speech-KA101
            add_msg( m_warning, _( "Are you sure?!  the multi-cooker wants to poison your food!" ) );
            return true;

        case 5:
            add_msg( m_info,
                     _( "The multi-cooker argues with you about the taste preferences.  You don't want to deal with it." ) );
            return true;

        case 6:
            if( !one_in( 5 ) ) {
                add_msg( m_warning, _( "The multi-cooker runs away!" ) );
                if( monster *const m = g->place_critter_around( mon_hallu_multicooker, p.pos(), 1 ) ) {
                    m->hallucination = true;
                    m->add_effect( effect_run, 1_turns, true );
                }
            } else {
                p.add_msg_if_player( m_info, _( "You're surrounded by aggressive multi-cookers!" ) );

                for( const tripoint &pn : get_map().points_in_radius( p.pos(), 1 ) ) {
                    if( monster *const m = g->place_critter_at( mon_hallu_multicooker, pn ) ) {
                        m->hallucination = true;
                    }
                }
            }
            return true;

        default:
            return false;
    }

}

std::optional<int> iuse::multicooker( Character *p, item *it, const tripoint &pos )
{
    static const int charges_to_start = 50;
    const int charge_buffer = 2;

    enum {
        mc_start, mc_stop, mc_take, mc_upgrade, mc_empty
    };

    if( p->cant_do_underwater() ) {
        return std::nullopt;
    }

    if( p->has_trait( trait_ILLITERATE ) ) {
        p->add_msg_if_player( m_info,
                              _( "You can't read, and don't understand the screen or the buttons!" ) );
        return std::nullopt;
    }

    if( p->has_effect( effect_hallu ) || p->has_effect( effect_visuals ) ) {
        if( multicooker_hallu( *p ) ) {
            return 0;
        }
    }

    if( p->has_flag( json_flag_HYPEROPIC ) && !p->worn_with_flag( flag_FIX_FARSIGHT ) &&
        !p->has_effect( effect_contacts ) ) {
        p->add_msg_if_player( m_info,
                              _( "You'll need to put on reading glasses before you can see the screen." ) );
        return std::nullopt;
    }

    uilist menu;
    menu.text = _( "Welcome to the RobotChef3000.  Choose option:" );

    item *dish_it = it->get_item_with(
    []( const item & it ) {
        return !( it.is_toolmod() || it.is_magazine() );
    } );

    if( it->active ) {
        menu.addentry( mc_stop, true, 's', _( "Stop cooking" ) );
    } else {
        if( dish_it == nullptr ) {
            if( it->ammo_remaining( p, true ) < charges_to_start ) {
                p->add_msg_if_player( _( "Batteries are low." ) );
                return 0;
            }
            menu.addentry( mc_start, true, 's', _( "Start cooking" ) );

            /** @EFFECT_ELECTRONICS >3 allows multicooker upgrade */

            /** @EFFECT_FABRICATION >3 allows multicooker upgrade */
            if( p->get_skill_level( skill_electronics ) >= 4 && p->get_skill_level( skill_fabrication ) >= 4 ) {
                const std::string upgr = it->get_var( "MULTI_COOK_UPGRADE" );
                if( upgr.empty() ) {
                    menu.addentry( mc_upgrade, true, 'u', _( "Upgrade multi-cooker" ) );
                } else {
                    if( upgr == "UPGRADE" ) {
                        menu.addentry( mc_upgrade, false, 'u', _( "Multi-cooker already upgraded" ) );
                    } else {
                        menu.addentry( mc_upgrade, false, 'u', _( "Multi-cooker unable to upgrade" ) );
                    }
                }
            }
        } else {
            // Something other than a recipe item might be stored in the pocket.
            if( dish_it->typeId().str() == it->get_var( "DISH" ) ) {
                menu.addentry( mc_take, true, 't', _( "Take out dish" ) );
            } else {
                menu.addentry( mc_empty, true, 't',
                               _( "Obstruction detected.  Please remove any items lodged in the multi-cooker." ) );
            }
        }
    }

    menu.query();
    int choice = menu.ret;

    if( choice < 0 ) {
        return std::nullopt;
    }

    if( mc_stop == choice ) {
        if( query_yn( _( "Really stop cooking?" ) ) ) {
            it->active = false;
            it->erase_var( "DISH" );
            it->erase_var( "COOKTIME" );
            it->erase_var( "RECIPE" );
            it->convert( itype_multi_cooker, p );
        }
        return 0;
    }

    if( mc_take == choice ) {
        item &dish = *dish_it;
        if( dish.has_flag( flag_FROZEN ) ) {
            dish.cold_up();  //don't know how to check if the dish is frozen liquid and prevent extraction of it into inventory...
        }
        const std::string dish_name = dish.tname( dish.charges, false );
        const bool is_delicious = dish.has_flag( flag_HOT ) && dish.has_flag( flag_EATEN_HOT );
        if( dish.made_of( phase_id::LIQUID ) ) {
            if( !p->check_eligible_containers_for_crafting( *recipe_id( it->get_var( "RECIPE" ) ), 1 ) ) {
                p->add_msg_if_player( m_info, _( "You don't have a suitable container to store your %s." ),
                                      dish_name );

                return 0;
            }
            liquid_handler::handle_all_liquid( dish, PICKUP_RANGE );
        } else {
            p->i_add( dish );
        }

        it->remove_item( *dish_it );
        it->erase_var( "RECIPE" );
        if( is_delicious ) {
            p->add_msg_if_player( m_good,
                                  _( "You got the dish from the multi-cooker.  The %s smells delicious." ),
                                  dish_name );
        } else {
            p->add_msg_if_player( m_good, _( "You got the %s from the multi-cooker." ),
                                  dish_name );
        }

        return 0;
    }

    // Do nothing if there's non-dish contained in the cooker
    if( mc_empty == choice ) {
        return std::nullopt;
    }

    if( mc_start == choice ) {
        uilist dmenu;
        dmenu.text = _( "Choose desired meal:" );

        std::vector<const recipe *> dishes;

        inventory crafting_inv = p->crafting_inventory();
        // add some tools and qualities. we can't add this qualities to
        // json, because multicook must be used only by activating, not as
        // component other crafts.
        crafting_inv.push_back( item( "hotplate", calendar::turn_zero ) ); //hotplate inside
        // some recipes requires tongs
        crafting_inv.push_back( item( "tongs", calendar::turn_zero ) );
        // toolset with CUT and other qualities inside
        crafting_inv.push_back( item( "toolset", calendar::turn_zero ) );
        // good COOK, BOIL, CONTAIN qualities inside
        crafting_inv.push_back( item( "pot", calendar::turn_zero ) );

        int counter = 0;
        static const std::set<std::string> multicooked_subcats = { "CSC_FOOD_MEAT", "CSC_FOOD_VEGGI", "CSC_FOOD_PASTA" };

        for( const recipe * const &r : get_avatar().get_learned_recipes().in_category( "CC_FOOD" ) ) {
            if( multicooked_subcats.count( r->subcategory ) > 0 ) {
                dishes.push_back( r );
                const bool can_make = r->deduped_requirements().can_make_with_inventory(
                                          crafting_inv, r->get_component_filter() );

                dmenu.addentry( counter++, can_make, -1, r->result_name( /*decorated=*/true ) );
            }
        }

        dmenu.query();

        int choice = dmenu.ret;

        if( choice < 0 ) {
            return std::nullopt;
        } else {
            const recipe *meal = dishes[choice];
            int mealtime;
            if( it->get_var( "MULTI_COOK_UPGRADE" ) == "UPGRADE" ) {
                mealtime = meal->time_to_craft_moves( *p, recipe_time_flag::ignore_proficiencies );
            } else {
                mealtime = meal->time_to_craft_moves( *p, recipe_time_flag::ignore_proficiencies ) * 2;
            }

            const int all_charges = charges_to_start + mealtime / 1000 * units::to_watt(
                                        it->type->tool->power_draw ) / 1000;

            if( it->ammo_remaining( p, true ) < all_charges ) {

                p->add_msg_if_player( m_warning,
                                      _( "The multi-cooker needs %d charges to cook this dish." ),
                                      all_charges );

                return std::nullopt;
            }

            const auto filter = is_crafting_component;
            const requirement_data *reqs =
                meal->deduped_requirements().select_alternative( *p, crafting_inv, filter );
            if( !reqs ) {
                return std::nullopt;
            }

            for( const auto &component : reqs->get_components() ) {
                p->consume_items( component, 1, filter );
            }

            it->set_var( "RECIPE", meal->ident().str() );
            it->set_var( "DISH", meal->result().str() );
            it->set_var( "COOKTIME", mealtime );

            p->add_msg_if_player( m_good,
                                  _( "The screen flashes blue symbols and scales as the multi-cooker begins to shake." ) );

            it->convert( itype_multi_cooker_filled, p ).active = true;
            it->ammo_consume( charges_to_start - charge_buffer, pos, p );

            p->practice( skill_cooking, meal->difficulty * 3 ); //little bonus

            return 0;
        }
    }

    if( mc_upgrade == choice ) {

        if( !p->has_morale_to_craft() ) {
            p->add_msg_if_player( m_info, _( "Your morale is too low to craft…" ) );
            return std::nullopt;
        }

        bool has_tools = true;

        const inventory &cinv = p->crafting_inventory();

        if( !cinv.has_amount( itype_soldering_iron, 1 ) ) {
            p->add_msg_if_player( m_warning, _( "You need a %s." ),
                                  item::nname( itype_soldering_iron ) );
            has_tools = false;
        }

        if( !cinv.has_quality( qual_SCREW_FINE ) ) {
            p->add_msg_if_player( m_warning, _( "You need an item with %s of 1 or more to upgrade this." ),
                                  qual_SCREW_FINE.obj().name );
            has_tools = false;
        }

        if( !has_tools ) {
            return std::nullopt;
        }

        p->practice( skill_electronics, rng( 5, 10 ) );
        p->practice( skill_fabrication, rng( 5, 10 ) );

        p->moves -= to_moves<int>( 7_seconds );

        /** @EFFECT_INT increases chance to successfully upgrade multi-cooker */

        /** @EFFECT_ELECTRONICS increases chance to successfully upgrade multi-cooker */

        /** @EFFECT_FABRICATION increases chance to successfully upgrade multi-cooker */
        if( p->get_skill_level( skill_electronics ) + p->get_skill_level( skill_fabrication ) + p->int_cur >
            rng( 20, 35 ) ) {

            p->practice( skill_electronics, rng( 5, 20 ) );
            p->practice( skill_fabrication, rng( 5, 20 ) );

            p->add_msg_if_player( m_good,
                                  _( "You've successfully upgraded the multi-cooker, master tinkerer!  Now it cooks faster!" ) );

            it->set_var( "MULTI_COOK_UPGRADE", "UPGRADE" );

            return 0;

        } else {

            if( !one_in( 5 ) ) {
                p->add_msg_if_player( m_neutral,
                                      _( "You sagely examine and analyze the multi-cooker, but don't manage to accomplish anything." ) );
            } else {
                p->add_msg_if_player( m_bad,
                                      _( "Your tinkering nearly breaks the multi-cooker!  Fortunately, it still works, but best to stop messing with it." ) );
                it->set_var( "MULTI_COOK_UPGRADE", "DAMAGED" );
            }

            return 0;

        }

    }

    return 0;
}

std::optional<int> iuse::multicooker_tick( Character *p, item *it, const tripoint &pos )
{
    const int charge_buffer = 2;

    //stop action before power runs out and iuse deletes the cooker
    if( it->ammo_remaining( p, true ) < charge_buffer ) {
        it->active = false;
        it->erase_var( "RECIPE" );
        it->convert( itype_multi_cooker, p );
        //drain the buffer amount given at activation
        it->ammo_consume( charge_buffer, pos, p );
        p->add_msg_if_player( m_info,
                              _( "Batteries low, entering standby mode.  With a low buzzing sound the multi-cooker shuts down." ) );
        return 0;
    }

    int cooktime = it->get_var( "COOKTIME", 0 );
    cooktime -= 100;

    if( cooktime >= 300 && cooktime < 400 ) {
        //Smart or good cook or careful
        /** @EFFECT_INT increases chance of checking multi-cooker on time */

        /** @EFFECT_SURVIVAL increases chance of checking multi-cooker on time */
        avatar &player = get_avatar();
        if( player.int_cur + player.get_skill_level( skill_cooking ) + player.get_skill_level(
                skill_survival ) > 16 ) {
            add_msg( m_info, _( "The multi-cooker should be finishing shortly…" ) );
        }
    }

    if( cooktime <= 0 ) {
        item meal( it->get_var( "DISH" ), time_point( calendar::turn ), 1 );
        if( ( *recipe_id( it->get_var( "RECIPE" ) ) ).hot_result() ) {
            meal.heat_up();
        } else {
            meal.set_item_temperature( std::max( temperatures::cold,
                                                 get_weather().get_temperature( pos ) ) );
        }

        it->active = false;
        it->erase_var( "COOKTIME" );
        it->convert( itype_multi_cooker, p );
        if( it->can_contain( meal ).success() ) {
            it->put_in( meal, pocket_type::CONTAINER );
        } else {
            add_msg( m_info,
                     _( "Obstruction detected.  Please remove any items lodged in the multi-cooker." ) );
            return 0;
        }

        //~ sound of a multi-cooker finishing its cycle!
        sounds::sound( pos, 8, sounds::sound_t::alarm, _( "ding!" ), true, "misc", "ding" );

        return 0;
    } else {
        it->set_var( "COOKTIME", cooktime );
        return 0;
    }

    return 0;
}

std::optional<int> iuse::shavekit( Character *p, item *it, const tripoint & )
{
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    if( !it->ammo_sufficient( p ) ) {
        p->add_msg_if_player( _( "You need soap to use this." ) );
    } else {
        p->assign_activity( shave_activity_actor() );
    }
    return 1;
}

std::optional<int> iuse::hairkit( Character *p, item *, const tripoint & )
{
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    p->assign_activity( haircut_activity_actor() );
    return 1;
}

std::optional<int> iuse::weather_tool( Character *p, item *it, const tripoint & )
{
    weather_manager &weather = get_weather();
    const w_point weatherPoint = *weather.weather_precise;

    /* Possibly used twice. Worth spending the time to precalculate. */
    const units::temperature player_local_temp = weather.get_temperature( p->pos() );

    if( it->typeId() == itype_weather_reader ) {
        p->add_msg_if_player( m_neutral, _( "The %s's monitor slowly outputs the data…" ),
                              it->tname() );
    }
    if( it->has_flag( flag_THERMOMETER ) ) {
        std::string temperature_str;
        if( get_map().has_flag_ter( ter_furn_flag::TFLAG_DEEP_WATER, p->pos() ) ||
            get_map().has_flag_ter( ter_furn_flag::TFLAG_SHALLOW_WATER, p->pos() ) ) {
            temperature_str = print_temperature( get_weather().get_cur_weather_gen().get_water_temperature() );
        } else {
            temperature_str = print_temperature( player_local_temp );
        }
        p->add_msg_if_player( m_neutral, _( "The %1$s reads %2$s." ), it->tname(),
                              temperature_str );
    }
    if( it->has_flag( flag_HYGROMETER ) ) {
        if( it->typeId() == itype_hygrometer ) {
            p->add_msg_if_player(
                m_neutral, _( "The %1$s reads %2$s." ), it->tname(),
                print_humidity( get_local_humidity( weatherPoint.humidity, get_weather().weather_id,
                                                    g->is_sheltered( p->pos() ) ) ) );
        } else {
            p->add_msg_if_player(
                m_neutral, _( "Relative Humidity: %s." ),
                print_humidity( get_local_humidity( weatherPoint.humidity, get_weather().weather_id,
                                                    g->is_sheltered( p->pos() ) ) ) );
        }
    }
    if( it->has_flag( flag_BAROMETER ) ) {
        if( it->typeId() == itype_barometer ) {
            p->add_msg_if_player(
                m_neutral, _( "The %1$s reads %2$s." ), it->tname(),
                print_pressure( static_cast<int>( weatherPoint.pressure ) ) );
        } else {
            p->add_msg_if_player( m_neutral, _( "Pressure: %s." ),
                                  print_pressure( static_cast<int>( weatherPoint.pressure ) ) );
        }
    }

    if( it->typeId() == itype_weather_reader ) {
        int vehwindspeed = 0;
        if( optional_vpart_position vp = get_map().veh_at( p->pos() ) ) {
            vehwindspeed = std::abs( vp->vehicle().velocity / 100 ); // For mph
        }
        const oter_id &cur_om_ter = overmap_buffer.ter( p->global_omt_location() );
        const int windpower = get_local_windpower( weather.windspeed + vehwindspeed, cur_om_ter,
                              p->get_location(), weather.winddirection, g->is_sheltered( p->pos() ) );

        p->add_msg_if_player( m_neutral, _( "Wind Speed: %.1f %s." ),
                              convert_velocity( windpower * 100, VU_WIND ),
                              velocity_units( VU_WIND ) );
        p->add_msg_if_player(
            m_neutral, _( "Feels Like: %s." ),
            print_temperature( player_local_temp + get_local_windchill( weatherPoint.temperature,
                               weatherPoint.humidity, windpower ) ) );
        std::string dirstring = get_dirstring( weather.winddirection );
        p->add_msg_if_player( m_neutral, _( "Wind Direction: From the %s." ), dirstring );
    }

    return 1; //TODO check
}

std::optional<int> iuse::sextant( Character *p, item *, const tripoint &pos )
{
    const std::pair<units::angle, units::angle> sun_position = sun_azimuth_altitude( calendar::turn );
    const float altitude = to_degrees( sun_position.second );
    if( debug_mode ) {
        // Debug mode always shows all sun angles
        const float azimuth = to_degrees( sun_position.first );
        p->add_msg_if_player( m_neutral, "Sun altitude %.1f°, azimuth %.1f°", altitude, azimuth );
    } else if( g->is_sheltered( pos ) ) {
        p->add_msg_if_player( m_neutral, _( "You can't see the Sun from here." ) );
    } else if( altitude > 0 ) {
        p->add_msg_if_player( m_neutral, _( "The Sun is at an altitude of %.1f°." ), altitude );
    } else {
        p->add_msg_if_player( m_neutral, _( "The Sun is below the horizon." ) );
    }

    return 0;
}

std::optional<int> iuse::lux_meter( Character *p, item *it, const tripoint &pos )
{
    p->add_msg_if_player( m_neutral, _( "The illumination is %.1f." ),
                          g->natural_light_level( pos.z ) );

    return it->type->charges_to_use();
}

std::optional<int> iuse::dbg_lux_meter( Character *p, item *, const tripoint &pos )
{
    map &here = get_map();
    const float incident_light = incident_sunlight( current_weather( here.getglobal( pos ) ),
                                 calendar::turn );
    const float nat_light = g->natural_light_level( pos.z );
    const float sunlight = sun_light_at( calendar::turn );
    const float sun_irrad = sun_irradiance( calendar::turn );
    const float incident_irrad = incident_sun_irradiance( current_weather( here.getglobal( pos ) ),
                                 calendar::turn );
    p->add_msg_if_player( m_neutral,
                          _( "Incident light: %.1f\nNatural light: %.1f\nSunlight: %.1f\nSun irradiance: %.1f\nIncident irradiance %.1f" ),
                          incident_light, nat_light, sunlight, sun_irrad, incident_irrad );

    return 0;
}

std::optional<int> iuse::calories_intake_tracker( Character *p, item *it, const tripoint & )
{
    if( p->has_trait( trait_ILLITERATE ) ) {
        p->add_msg_if_player( m_info, _( "You don't know what you're looking at." ) );
        return std::nullopt;
    } else {
        std::string msg;
        msg.append( "***  " );
        msg.append( string_format( _( "You check your registered calories intake on your %s." ),
                                   it->tname( 1, false ) ) );
        msg.append( "  ***\n\n" );
        msg.append( "-> " );
        msg.append( string_format( _( "You consumed %d kcal today and %d kcal yesterday." ),
                                   p->as_avatar()->get_daily_ingested_kcal( false ),
                                   p->as_avatar()->get_daily_ingested_kcal( true ) ) );
        p->add_msg_if_player( m_neutral, msg );
        popup( msg );
    }
    return 1;
}

std::optional<int> iuse::directional_hologram( Character *p, item *it, const tripoint & )
{
    if( it->is_armor() &&  !p->is_worn( *it ) ) {
        p->add_msg_if_player( m_neutral, _( "You need to wear the %1$s before activating it." ),
                              it->tname() );
        return std::nullopt;
    }
    const std::optional<tripoint> posp = choose_adjacent( _( "Choose hologram direction." ) );
    if( !posp ) {
        return std::nullopt;
    }
    const tripoint delta = *posp - get_player_character().pos();

    monster *const hologram = g->place_critter_at( mon_hologram, *posp );
    if( !hologram ) {
        p->add_msg_if_player( m_info, _( "Can't create a hologram there." ) );
        return std::nullopt;
    }
    tripoint_abs_ms target = p->get_location() + delta * ( 4 * SEEX );
    hologram->friendly = -1;
    hologram->add_effect( effect_docile, 1_hours );
    hologram->wandf = -30;
    hologram->set_summon_time( 60_seconds );
    hologram->set_dest( target );
    p->mod_moves( -to_moves<int>( 1_seconds ) );
    return 1;
}

std::optional<int> iuse::capture_monster_veh( Character *p, item *it, const tripoint &pos )
{
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    if( !it->has_flag( flag_VEHICLE ) ) {
        p->add_msg_if_player( m_info, _( "The %s must be installed in a vehicle before being loaded." ),
                              it->tname() );
        return std::nullopt;
    }
    capture_monster_act( p, it, pos );
    return 0;
}

bool item::release_monster( const tripoint &target, const int radius )
{
    shared_ptr_fast<monster> new_monster = make_shared_fast<monster>();
    try {
        ::deserialize_from_string( *new_monster, get_var( "contained_json", "" ) );
    } catch( const std::exception &e ) {
        debugmsg( _( "Error restoring monster: %s" ), e.what() );
        return false;
    }
    if( !g->place_critter_around( new_monster, target, radius ) ) {
        return false;
    }
    erase_var( "contained_name" );
    erase_var( "contained_json" );
    erase_var( "name" );
    erase_var( "weight" );
    return true;
}

// didn't want to drag the monster:: definition into item.h, so just reacquire the monster
// at target
int item::contain_monster( const tripoint &target )
{
    const monster *const mon_ptr = get_creature_tracker().creature_at<monster>( target );
    if( !mon_ptr ) {
        return 0;
    }
    const monster &f = *mon_ptr;

    set_var( "contained_json", ::serialize( f ) );
    set_var( "contained_name", f.type->nname() );
    set_var( "name", string_format( _( "%s holding %s" ), type->nname( 1 ),
                                    f.type->nname() ) );
    // Need to add the weight of the empty container because item::weight uses the "weight" variable directly.
    set_var( "weight", to_milligram( type->weight + f.get_weight() ) );
    g->remove_zombie( f );
    return 0;
}

std::optional<int> iuse::capture_monster_act( Character *p, item *it, const tripoint &pos )
{
    if( p->is_mounted() ) {
        p->add_msg_if_player( m_info, _( "You can't capture a creature mounted." ) );
        return std::nullopt;
    }
    if( it->has_var( "contained_name" ) ) {
        // Remember contained_name for messages after release_monster erases it
        const std::string contained_name = it->get_var( "contained_name", "" );

        if( it->release_monster( pos ) ) {
            // It's been activated somewhere where there isn't a player or monster, good.
            return 0;
        }
        if( it->has_flag( flag_PLACE_RANDOMLY ) ) {
            if( it->release_monster( p->pos(), 1 ) ) {
                return 0;
            }
            p->add_msg_if_player( _( "There is no place to put the %s." ), contained_name );
            return std::nullopt;
        } else {
            const std::string query = string_format( _( "Place the %s where?" ), contained_name );
            const std::optional<tripoint> pos_ = choose_adjacent( query );
            if( !pos_ ) {
                return std::nullopt;
            }
            if( it->release_monster( *pos_ ) ) {
                p->add_msg_if_player( _( "You release the %s." ), contained_name );
                return 0;
            }
            p->add_msg_if_player( m_info, _( "You can't place the %s there!" ), contained_name );
            return std::nullopt;
        }
    } else {
        if( !it->has_property( "creature_size_capacity" ) ) {
            debugmsg( "%s has no creature_size_capacity.", it->tname() );
            return std::nullopt;
        }
        const std::string capacity = it->get_property_string( "creature_size_capacity" );
        if( Creature::size_map.count( capacity ) == 0 ) {
            debugmsg( "%s has invalid creature_size_capacity %s.",
                      it->tname(), capacity.c_str() );
            return std::nullopt;
        }
        const std::function<bool( const tripoint & )> adjacent_capturable = []( const tripoint & pnt ) {
            const monster *mon_ptr = get_creature_tracker().creature_at<monster>( pnt );
            return mon_ptr != nullptr;
        };
        const std::string query = string_format( _( "Grab which creature to place in the %s?" ),
                                  it->tname() );
        const std::optional<tripoint> target_ = choose_adjacent_highlight( query,
                                                _( "There is no creature nearby you can capture." ), adjacent_capturable, false );
        if( !target_ ) {
            p->add_msg_if_player( m_info, _( "You can't use a %s there." ), it->tname() );
            return std::nullopt;
        }
        const tripoint target = *target_;

        // Capture the thing, if it's on the target square.
        if( const monster *const mon_ptr = get_creature_tracker().creature_at<monster>( target ) ) {
            const monster &f = *mon_ptr;

            if( f.get_size() > Creature::size_map.find( capacity )->second ) {
                p->add_msg_if_player( m_info, _( "The %1$s is too big to put in your %2$s." ),
                                      f.type->nname(), it->tname() );
                return std::nullopt;
            }
            // TODO: replace this with some kind of melee check.
            int chance = f.hp_percentage() / 10;
            // A weaker monster is easier to capture.
            // If the monster is friendly, then put it in the item
            // without checking if it rolled a success.
            if( f.friendly != 0 || one_in( chance ) ) {
                p->add_msg_if_player( _( "You capture the %1$s in your %2$s." ),
                                      f.type->nname(), it->tname() );
                return it->contain_monster( target );
            } else {
                p->add_msg_if_player( m_bad, _( "The %1$s avoids your attempts to put it in the %2$s." ),
                                      f.type->nname(), it->type->nname( 1 ) );
            }
            p->moves -= to_moves<int>( 1_seconds );
        } else {
            add_msg( _( "The %s can't capture nothing" ), it->tname() );
            return std::nullopt;
        }
    }
    return 0;
}

washing_requirements washing_requirements_for_volume( const units::volume &vol )
{
    int water = divide_round_up( vol, 125_ml );
    int cleanser = divide_round_up( vol, 1_liter );
    int time = to_moves<int>( 10_seconds * ( vol / 250_ml ) );
    return { water, cleanser, time };
}

std::optional<int> iuse::wash_soft_items( Character *p, item *, const tripoint & )
{
    if( p->fine_detail_vision_mod() > 4 ) {
        p->add_msg_if_player( _( "You can't see to do that!" ) );
        return std::nullopt;
    }
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    // Check that player isn't over volume limit as this might cause it to break... this is a hack.
    // TODO: find a better solution.
    if( p->volume_capacity() < p->volume_carried() ) {
        p->add_msg_if_player( _( "You're carrying too much to clean anything." ) );
        return std::nullopt;
    }

    wash_items( p, true, false );
    return 0;
}

std::optional<int> iuse::wash_hard_items( Character *p, item *, const tripoint & )
{
    if( p->fine_detail_vision_mod() > 4 ) {
        p->add_msg_if_player( _( "You can't see to do that!" ) );
        return std::nullopt;
    }
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    // Check that player isn't over volume limit as this might cause it to break... this is a hack.
    // TODO: find a better solution.
    if( p->volume_capacity() < p->volume_carried() ) {
        p->add_msg_if_player( _( "You're carrying too much to clean anything." ) );
        return std::nullopt;
    }

    wash_items( p, false, true );
    return 0;
}

std::optional<int> iuse::wash_all_items( Character *p, item *, const tripoint & )
{
    if( p->fine_detail_vision_mod() > 4 ) {
        p->add_msg_if_player( _( "You can't see to do that!" ) );
        return std::nullopt;
    }

    // Check that player isn't over volume limit as this might cause it to break... this is a hack.
    // TODO: find a better solution.
    if( p->volume_capacity() < p->volume_carried() ) {
        p->add_msg_if_player( _( "You're carrying too much to clean anything." ) );
        return std::nullopt;
    }

    wash_items( p, true, true );
    return 0;
}

std::optional<int> iuse::wash_items( Character *p, bool soft_items, bool hard_items )
{
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    p->inv->restack( *p );
    const inventory &crafting_inv = p->crafting_inventory();

    auto is_liquid = []( const item & it ) {
        return it.made_of( phase_id::LIQUID );
    };
    int available_water = std::max(
                              crafting_inv.charges_of( itype_water, INT_MAX, is_liquid ),
                              crafting_inv.charges_of( itype_water_clean, INT_MAX, is_liquid )
                          );
    int available_cleanser = std::max( crafting_inv.charges_of( itype_soap ),
                                       std::max( crafting_inv.charges_of( itype_detergent ),
                                               crafting_inv.charges_of( itype_liquid_soap, INT_MAX, is_liquid ) ) );

    const inventory_filter_preset preset( [soft_items, hard_items]( const item_location & location ) {
        return location->has_flag( flag_FILTHY ) && !location->has_flag( flag_NO_CLEAN ) &&
               ( ( soft_items && location->is_soft() ) || ( hard_items && !location->is_soft() ) );
    } );
    auto make_raw_stats = [available_water,
                           available_cleanser]( const std::vector<std::pair<item_location, int>> &locs
    ) {
        units::volume total_volume = 0_ml;
        for( const auto &pair : locs ) {
            total_volume += pair.first->volume( false, true, pair.second );
        }
        washing_requirements required = washing_requirements_for_volume( total_volume );
        const std::string time = colorize( to_string( time_duration::from_moves( required.time ), true ),
                                           c_light_gray );
        auto to_string = []( int val ) -> std::string {
            if( val == INT_MAX )
            {
                return pgettext( "short for infinity", "inf" );
            }
            return string_format( "%3d", val );
        };
        const std::string water = string_join( display_stat( "", required.water, available_water,
                                               to_string ), "" );
        const std::string cleanser = string_join( display_stat( "", required.cleanser, available_cleanser,
                                     to_string ), "" );
        using stats = inventory_selector::stats;
        return stats{{
                {{ _( "Water" ), water }},
                {{ _( "Cleanser" ), cleanser }},
                {{ _( "Estimated time" ), time }}
            }};
    };
    inventory_multiselector inv_s( *p, preset, _( "ITEMS TO CLEAN" ),
                                   make_raw_stats, /*allow_select_contained=*/true );
    inv_s.add_character_items( *p );
    inv_s.add_nearby_items( PICKUP_RANGE );
    inv_s.set_title( _( "Multiclean" ) );
    inv_s.set_hint( _( "To clean x items, type a number before selecting." ) );
    if( inv_s.empty() ) {
        popup( std::string( _( "You have nothing to clean." ) ), PF_GET_KEY );
        return std::nullopt;
    }
    const drop_locations to_clean = inv_s.execute();
    if( to_clean.empty() ) {
        return std::nullopt;
    }
    // Determine if we have enough water and cleanser for all the items.
    units::volume total_volume = 0_ml;
    for( drop_location pair : to_clean ) {
        if( !pair.first ) {
            p->add_msg_if_player( m_info, _( "Never mind." ) );
            return std::nullopt;
        }
        total_volume += pair.first->volume( false, true, pair.second );
    }

    washing_requirements required = washing_requirements_for_volume( total_volume );

    if( !crafting_inv.has_charges( itype_water, required.water, is_liquid ) &&
        !crafting_inv.has_charges( itype_water_clean, required.water, is_liquid ) ) {
        p->add_msg_if_player( _( "You need %1$i charges of water or clean water to wash these items." ),
                              required.water );
        return std::nullopt;
    } else if( !crafting_inv.has_charges( itype_soap, required.cleanser ) &&
               !crafting_inv.has_charges( itype_detergent, required.cleanser ) &&
               !crafting_inv.has_charges( itype_liquid_soap, required.cleanser, is_liquid ) ) {
        p->add_msg_if_player( _( "You need %1$i charges of cleansing agent to wash these items." ),
                              required.cleanser );
        return std::nullopt;
    }
    const std::vector<Character *> helpers = p->get_crafting_helpers();
    const std::size_t helpersize = p->get_num_crafting_helpers( 3 );
    required.time *= ( 1.0f - ( helpersize / 10.0f ) );
    for( std::size_t i = 0; i < helpersize; i++ ) {
        add_msg( m_info, _( "%s helps with this task…" ), helpers[i]->get_name() );
    }
    // Assign the activity values.
    p->assign_activity( wash_activity_actor( to_clean, required ) );

    return 0;
}

std::optional<int> iuse::break_stick( Character *p, item *it, const tripoint & )
{
    p->moves -= to_moves<int>( 2_seconds );
    p->mod_stamina( static_cast<int>( 0.05f * p->get_stamina_max() ) );

    if( p->get_str() < 5 ) {
        p->add_msg_if_player( _( "You are too weak to even try." ) );
        return 0;
    } else if( p->get_str() <= rng( 5, 11 ) ) {
        p->add_msg_if_player(
            _( "You use all your strength, but the stick won't break.  Perhaps try again?" ) );
        return 0;
    }
    std::vector<item_comp> comps;
    comps.emplace_back( it->typeId(), 1 );
    p->consume_items( comps, 1, is_crafting_component );
    int chance = rng( 0, 100 );
    map &here = get_map();
    if( chance <= 20 ) {
        p->add_msg_if_player( _( "You try to break the stick in two, but it shatters into splinters." ) );
        here.spawn_item( p->pos(), "splinter", 2 );
        return 1;
    } else if( chance <= 40 ) {
        p->add_msg_if_player( _( "The stick breaks clean into two parts." ) );
        here.spawn_item( p->pos(), "stick", 2 );
        return 1;
    } else if( chance <= 100 ) {
        p->add_msg_if_player( _( "You break the stick, but one half shatters into splinters." ) );
        here.spawn_item( p->pos(), "stick", 1 );
        here.spawn_item( p->pos(), "splinter", 1 );
        return 1;
    }
    return 0;
}

std::optional<int> iuse::weak_antibiotic( Character *p, item *it, const tripoint & )
{
    p->add_msg_if_player( _( "You take some %s." ), it->tname() );
    if( p->has_effect( effect_infected ) && !p->has_effect( effect_weak_antibiotic ) ) {
        p->add_msg_if_player( m_good, _( "The throbbing of the infection diminishes.  Slightly." ) );
    }
    p->add_effect( effect_weak_antibiotic, 12_hours );
    p->add_effect( effect_weak_antibiotic_visible, rng( 9_hours, 15_hours ) );
    return 1;
}

std::optional<int> iuse::strong_antibiotic( Character *p, item *it, const tripoint & )
{
    p->add_msg_if_player( _( "You take some %s." ), it->tname() );
    if( p->has_effect( effect_infected ) && !p->has_effect( effect_strong_antibiotic ) ) {
        p->add_msg_if_player( m_good, _( "You feel much better - almost entirely." ) );
    }
    p->add_effect( effect_strong_antibiotic, 12_hours );
    p->add_effect( effect_strong_antibiotic_visible, rng( 9_hours, 15_hours ) );
    return 1;
}

static item *wield_before_use( Character *const p, item *const it, const std::string &msg )
{
    if( !p->is_wielding( *it ) ) {
        if( !p->is_armed() || query_yn( msg.c_str(), it->tname() ) ) {
            if( !p->wield( *it ) ) {
                // Will likely happen if it is too heavy, or the player is
                // wielding something that can't be unwielded
                add_msg( m_bad, "%s", p->can_wield( *it ).str() );
                return nullptr;
            }
            // `it` is no longer the item we are using (note that `player::wielded` is a value).
            return &*p->get_wielded_item();
        } else {
            return nullptr;
        }
    }
    return it;
}

std::optional<int> iuse::craft( Character *p, item *it, const tripoint & )
{
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    it = wield_before_use( p, it, _( "Wield the %s and start working?" ) );
    if( !it ) {
        return std::nullopt;
    }

    const std::string craft_name = it->tname();

    if( !it->is_craft() ) {
        debugmsg( "Attempted to start working on non craft '%s.'  Aborting.", craft_name );
        return std::nullopt;
    }

    if( !p->can_continue_craft( *it ) ) {
        return std::nullopt;
    }
    const recipe &rec = it->get_making();
    const inventory &inv = p->crafting_inventory();
    if( !p->has_recipe( &rec, inv, p->get_crafting_group() ) ) {
        p->add_msg_player_or_npc(
            _( "You don't know the recipe for the %s and can't continue crafting." ),
            _( "<npcname> doesn't know the recipe for the %s and can't continue crafting." ),
            rec.result_name() );
        return 0;
    }
    p->add_msg_player_or_npc(
        pgettext( "in progress craft", "You start working on the %s." ),
        pgettext( "in progress craft", "<npcname> starts working on the %s." ), craft_name );
    item_location craft_loc = item_location( *p, it );
    p->assign_activity( craft_activity_actor( craft_loc, false ) );

    return 0;
}

std::optional<int> iuse::disassemble( Character *p, item *it, const tripoint & )
{
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    it = wield_before_use( p, it, _( "Wield the %s and start working?" ) );
    if( !it ) {
        return std::nullopt;
    }
    if( !p->has_item( *it ) ) {
        return std::nullopt;
    }

    p->disassemble( item_location( *p, it ), false );

    return 0;
}

std::optional<int> iuse::melatonin_tablet( Character *p, item *it, const tripoint & )
{
    p->add_msg_if_player( _( "You pop a %s." ), it->tname() );
    if( p->has_effect( effect_melatonin ) ) {
        p->add_msg_if_player( m_warning,
                              _( "Simply taking more melatonin won't help.  You have to go to sleep for it to work." ) );
    }
    p->add_effect( effect_melatonin, 16_hours );
    return 1;
}

std::optional<int> iuse::coin_flip( Character *p, item *it, const tripoint & )
{
    p->add_msg_if_player( m_info, _( "You flip a %s." ), it->tname() );
    p->add_msg_if_player( m_info, one_in( 2 ) ? _( "Heads!" ) : _( "Tails!" ) );
    return 0;
}

std::optional<int> iuse::play_game( Character *p, item *it, const tripoint & )
{
    if( p->is_avatar() ) {
        std::vector<npc *> followers = g->get_npcs_if( [p]( const npc & n ) {
            return n.is_ally( *p ) && p->sees( n ) && n.can_hear( p->pos(), p->get_shout_volume() );
        } );
        int fcount = followers.size();
        if( fcount > 0 ) {
            const char *qstr = fcount > 1 ? _( "Play the %s with your friends?" ) :
                               _( "Play the %s with your friend?" );
            std::string res = query_popup()
                              .context( "FRIENDS_ME_CANCEL" )
                              .message( qstr, it->tname() )
                              .option( "FRIENDS" ).option( "ME" ).option( "CANCEL" )
                              .query().action;
            if( res == "FRIENDS" ) {
                if( fcount > 1 ) {
                    add_msg( n_gettext( "You and your %d friend start playing.",
                                        "You and your %d friends start playing.", fcount ), fcount );
                } else {
                    add_msg( _( "You and your friend start playing." ) );
                }
                p->assign_activity( ACT_GENERIC_GAME, to_moves<int>( 1_hours ), fcount,
                                    p->get_item_position( it ), "gaming with friends" );
                for( npc *n : followers ) {
                    n->assign_activity( ACT_GENERIC_GAME, to_moves<int>( 1_hours ), fcount,
                                        n->get_item_position( it ), "gaming with friends" );
                }
            } else if( res == "ME" ) {
                p->add_msg_if_player( _( "You start playing." ) );
                p->assign_activity( ACT_GENERIC_GAME, to_moves<int>( 1_hours ), -1,
                                    p->get_item_position( it ), "gaming" );
            } else {
                return std::nullopt;
            }
            return 0;
        } // else, fall through to playing alone
    }
    if( query_yn( _( "Play a game with the %s?" ), it->tname() ) ) {
        p->add_msg_if_player( _( "You start playing." ) );
        p->assign_activity( ACT_GENERIC_GAME, to_moves<int>( 1_hours ), -1,
                            p->get_item_position( it ), "gaming" );
    } else {
        return std::nullopt;
    }
    return 0;
}

std::optional<int> iuse::magic_8_ball( Character *p, item *it, const tripoint & )
{
    p->add_msg_if_player( m_info, _( "You ask the %s, then flip it." ), it->tname() );
    int rn = rng( 0, 3 );
    std::string msg_category;
    game_message_type color;
    if( rn == 0 ) {
        msg_category = "magic_8ball_bad";
        color = m_bad;
    } else if( rn == 1 ) {
        msg_category = "magic_8ball_unknown";
        color = m_info;
    } else {
        msg_category = "magic_8ball_good";
        color = m_good;
    }
    p->add_msg_if_player( color, _( "The %s says: %s" ), it->tname(),
                          SNIPPET.random_from_category( msg_category ).value_or( translation() ) );
    return 0;
}

std::optional<int> iuse::electricstorage( Character *p, item *it, const tripoint & )
{
    // From item processing
    if( !p ) {
        debugmsg( "%s called action electricstorage that requires character but no character is present",
                  it->typeId().str() );
        return std::nullopt;
    }

    if( p->is_npc() ) {
        return std::nullopt;
    }

    if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _( "Unfortunately your device is not waterproof." ) );
        return std::nullopt;
    }

    if( !it->is_ebook_storage() ) {
        debugmsg( "ELECTRICSTORAGE iuse called on item without ebook type pocket" );
        return std::nullopt;
    }

    if( p->has_flag( json_flag_HYPEROPIC ) && !p->worn_with_flag( flag_FIX_FARSIGHT ) &&
        !p->has_effect( effect_contacts ) && !p->has_flag( json_flag_ENHANCED_VISION ) ) {
        p->add_msg_if_player( m_info,
                              _( "You'll need to put on reading glasses before you can see the screen." ) );
        return std::nullopt;
    }

    auto filter = [it]( const item & itm ) {
        return !itm.is_broken() &&
               &itm != it &&
               itm.has_pocket_type( pocket_type::EBOOK );
    };

    item_location storage_card = game_menus::inv::titled_filter_menu(
                                     filter, *p->as_avatar(), _( "Use what storage device?" ),
                                     -1, _( "You don't have any empty book storage devices." ) );

    if( !storage_card ) {
        return std::nullopt;
    }

    // list of books of from_it that are not in to_it
    auto book_difference = []( const item & from_it, const item & to_it ) -> std::vector<const item *> {
        std::set<itype_id> existing_ebooks;
        for( const item *ebook : to_it.ebooks() )
        {
            if( !ebook->is_book() ) {
                debugmsg( "ebook type pocket contains non-book item %s", ebook->typeId().str() );
                continue;
            }

            existing_ebooks.insert( ebook->typeId() );
        }

        std::vector<const item *> ebooks;
        for( const item *ebook : from_it.ebooks() )
        {
            if( !ebook->is_book() ) {
                debugmsg( "ebook type pocket contains non-book item %s", ebook->typeId().str() );
                continue;
            }

            if( existing_ebooks.count( ebook->typeId() ) ) {
                continue;
            }

            ebooks.emplace_back( ebook );
        }
        return ebooks;
    };

    std::vector<const item *> to_storage = book_difference( *it, *storage_card );
    std::vector<const item *> to_device = book_difference( *storage_card, *it );

    uilist smenu;
    smenu.text = _( "What to do with your storage devices:" );

    smenu.addentry( 1, !to_device.empty(), 't', _( "Copy to device from the card" ) );
    smenu.addentry( 2, !to_storage.empty(), 'f', _( "Copy from device to the card" ) );
    smenu.addentry( 3, !storage_card->ebooks().empty(), 'v', _( "View books in the card" ) );
    smenu.query();

    // were any books moved to or from the device
    int books_moved = 0;

    auto move_books = [&books_moved]( const std::vector<const item *> &fromset, item & toit ) -> void {
        books_moved = fromset.size();
        for( const item *ebook : fromset )
        {
            toit.put_in( *ebook, pocket_type::EBOOK );
        }
    };

    if( smenu.ret == 1 ) {
        // to device
        move_books( to_device, *it );
    } else if( smenu.ret == 2 ) {
        // from device
        move_books( to_storage, *storage_card );
    } else if( smenu.ret == 3 ) {
        game_menus::inv::ebookread( *p, storage_card );
        return std::nullopt;
    } else {
        return std::nullopt;
    }

    if( books_moved > 0 ) {
        p->mod_moves( -to_moves<int>( 2_seconds ) );
        if( smenu.ret == 1 ) {
            p->add_msg_if_player( m_info,
                                  n_gettext( "Copied one book to the device.",
                                             "Copied %1$s books to the device.", books_moved ),
                                  books_moved );
        } else if( smenu.ret == 2 ) {
            p->add_msg_if_player( m_info,
                                  n_gettext( "Copied one book to the %2$s.",
                                             "Copied %1$s books to the %2$s.", books_moved ),
                                  books_moved, storage_card->tname() );
        }
    }

    return std::nullopt;
}

std::optional<int> iuse::ebooksave( Character *p, item *it, const tripoint & )
{
    if( !p ) {
        debugmsg( "%s called action ebooksave that requires character but no character is present",
                  it->typeId().str() );
        return std::nullopt;
    }
    if( !it->is_ebook_storage() ) {
        debugmsg( "EBOOKSAVE iuse called on item without ebook type pocket" );
        return std::nullopt;
    }

    if( p->is_npc() ) {
        return std::nullopt;
    }

    if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _( "Unfortunately your device is not waterproof." ) );
        return std::nullopt;
    }

    if( p->has_flag( json_flag_HYPEROPIC ) && !p->worn_with_flag( flag_FIX_FARSIGHT ) &&
        !p->has_effect( effect_contacts ) && !p->has_flag( json_flag_ENHANCED_VISION ) ) {
        p->add_msg_if_player( m_info,
                              _( "You'll need to put on reading glasses before you can see the screen." ) );
        return std::nullopt;
    }

    item_location ereader = item_location( *p, it );
    const drop_locations to_scan = game_menus::inv::ebooksave( *p, ereader );
    if( to_scan.empty() ) {
        return std::nullopt;
    }
    std::vector<item_location> books;
    for( const auto &pair : to_scan ) {
        books.push_back( pair.first );
    }
    p->assign_activity( ebooksave_activity_actor( books, ereader ) );
    return std::nullopt;
}

std::optional<int> iuse::ebookread( Character *p, item *it, const tripoint & )
{
    if( !p ) {
        debugmsg( "%s called action ebookread that requires character but no character is present",
                  it->typeId().str() );
        return std::nullopt;
    }
    if( !it->is_ebook_storage() ) {
        debugmsg( "EBOOKREAD iuse called on item without ebook type pocket" );
        return std::nullopt;
    }

    if( p->is_npc() ) {
        return std::nullopt;
    }

    if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _( "Unfortunately your device is not waterproof." ) );
        return std::nullopt;
    }

    if( p->has_flag( json_flag_HYPEROPIC ) && !p->worn_with_flag( flag_FIX_FARSIGHT ) &&
        !p->has_effect( effect_contacts ) && !p->has_flag( json_flag_ENHANCED_VISION ) ) {
        p->add_msg_if_player( m_info,
                              _( "You'll need to put on reading glasses before you can see the screen." ) );
        return std::nullopt;
    }

    // Only turn on the eBook light, if it's too dark to read
    if( p->fine_detail_vision_mod() > 4 && !it->active && it->is_transformable() ) {
        const use_function *readinglight = it->type->get_use( "transform" );
        if( readinglight ) {
            readinglight->call( p, *it, p->pos() );
        }
    }

    item_location ereader = item_location( *p, it );
    item_location book = game_menus::inv::ebookread( *p, ereader );

    if( !book ) {
        return std::nullopt;
    }

    p->as_avatar()->read( book, ereader );

    return std::nullopt;
}

std::optional<int> iuse::binder_add_recipe( Character *p, item *binder, const tripoint & )
{
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }

    if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _( "You rethink trying to write underwater." ) );
        return std::nullopt;
    }

    const inventory crafting_inv = p->crafting_inventory();
    const std::vector<const item *> writing_tools = crafting_inv.items_with( [&]( const item & it ) {
        return it.has_flag( flag_WRITE_MESSAGE ) && it.ammo_sufficient( p );
    } );

    if( writing_tools.empty() ) {
        p->add_msg_if_player( m_info, _( "You do not have anything to write with." ) );
        return std::nullopt;
    }

    if( p->fine_detail_vision_mod() > 4.0f ) {
        p->add_msg_if_player( m_info, _( "It's too dark to write!" ) );
        return std::nullopt;
    }

    const recipe_subset res = p->get_recipes_from_books( crafting_inv );
    const std::vector<const recipe *> recipes( res.begin(), res.end() );
    const auto enough_writing_tool_charges = [&writing_tools]( int pages ) {
        for( const item *it : writing_tools ) {
            if( it->ammo_required() == 0 ) {
                return true;
            }
            pages -= it->ammo_remaining() / it->ammo_required();
            if( pages <= 0 ) {
                return true;
            }
        }
        return false;
    };

    uilist menu;
    menu.text = _( "Choose recipe to copy" );
    menu.hilight_disabled = true;
    menu.desc_enabled = true;
    for( const recipe *r : recipes ) {
        std::string desc;
        const int pages = bookbinder_copy_activity_actor::pages_for_recipe( *r );
        const std::string pages_str = string_format( n_gettext( "%1$d page", "%1$d pages", pages ), pages );

        if( !p->has_charges( itype_paper, pages ) ) {
            desc = _( "You do not have enough paper to copy this recipe." );
        } else if( binder->remaining_ammo_capacity() <  pages ) {
            desc = _( "Your recipe book can not fit this recipe." );
        } else if( binder->get_saved_recipes().count( r->ident() ) != 0 ) {
            desc = string_format( _( "Your recipe book already has a recipe for %s." ), r->result_name() );
        } else if( !enough_writing_tool_charges( pages ) ) {
            desc = _( "Your writing tools do not have enough charges." );
        }
        menu.addentry_col( -1, desc.empty(), std::nullopt, r->result_name(), pages_str, desc );
    }
    if( menu.entries.empty() ) {
        p->add_msg_if_player( m_info, _( "You do not have any recipes you can copy." ) );
        return std::nullopt;
    }
    menu.query();
    if( menu.ret < 0 ) {
        return std::nullopt;
    }

    bookbinder_copy_activity_actor act( item_location( *p, binder ), recipes[menu.ret]->ident() );
    p->assign_activity( act );

    return std::nullopt;
}

std::optional<int> iuse::binder_manage_recipe( Character *p, item *binder,
        const tripoint &ipos )
{
    if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _( "Doing that would ruin the %1$s." ), binder->tname() );
        return std::nullopt;
    }

    std::set<recipe_id> binder_recipes = binder->get_saved_recipes();
    const std::vector<recipe_id> recipes( binder_recipes.begin(), binder_recipes.end() );

    if( binder_recipes.empty() ) {
        p->add_msg_if_player( m_info, _( "You have no recipes to manage." ) );
        return std::nullopt;
    }

    uilist rmenu;
    rmenu.text = _( "Manage recipes" );

    for( const recipe_id &new_recipe : recipes ) {
        std::string recipe_name = new_recipe->result_name();
        if( p->knows_recipe( &new_recipe.obj() ) ) {
            recipe_name += _( " (KNOWN)" );
        }

        const int pages = bookbinder_copy_activity_actor::pages_for_recipe( *new_recipe );
        rmenu.addentry_col( -1, true, ' ', recipe_name,
                            string_format( n_gettext( "%1$d page", "%1$d pages", pages ), pages ) );
    }

    rmenu.query();
    if( rmenu.ret < 0 ) {
        return std::nullopt;
    }
    const recipe_id &rec = recipes[rmenu.ret];
    if( !query_yn( _( "Remove the recipe for %1$s?" ), rec->result_name() ) ) {
        return std::nullopt;
    }
    binder_recipes.erase( rec );
    binder->set_saved_recipes( binder_recipes );

    const int pages = bookbinder_copy_activity_actor::pages_for_recipe( *rec );
    binder->ammo_consume( pages, ipos, p );

    return std::nullopt;
}

std::optional<int> iuse::voltmeter( Character *p, item *, const tripoint & )
{
    const std::optional<tripoint> pnt_ = choose_adjacent( _( "Check voltage where?" ) );
    if( !pnt_ ) {
        return std::nullopt;
    }

    const map &here = get_map();
    const optional_vpart_position vp = here.veh_at( *pnt_ );

    if( !vp ) {
        p->add_msg_if_player( _( "There's nothing to measure there." ) );
        return std::nullopt;
    }
    if( vp->vehicle().fuel_left( itype_battery ) ) {
        p->add_msg_if_player( _( "The %1$s has voltage." ), vp->vehicle().name );
    } else {
        p->add_msg_if_player( _( "The %1$s has no voltage." ), vp->vehicle().name );
    }
    return 1;
}

void use_function::dump_info( const item &it, std::vector<iteminfo> &dump ) const
{
    if( actor != nullptr ) {
        actor->info( it, dump );
    }
}

ret_val<void> use_function::can_call( const Character &p, const item &it,
                                      const tripoint &pos ) const
{
    if( actor == nullptr ) {
        return ret_val<void>::make_failure( _( "You can't do anything interesting with your %s." ),
                                            it.tname() );
    } else if( it.is_broken() ) {
        return ret_val<void>::make_failure( _( "Your %s is broken and won't activate." ),
                                            it.tname() );
    }

    return actor->can_use( p, it, pos );
}

std::optional<int> use_function::call( Character *p, item &it,
                                       const tripoint &pos ) const
{
    return actor->use( p, it, pos );
}
