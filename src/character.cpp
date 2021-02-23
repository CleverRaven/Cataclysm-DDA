#include "character.h"

#include <cctype>
#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <numeric>
#include <ostream>
#include <tuple>
#include <type_traits>

#include "action.h"
#include "activity_actor_definitions.h"
#include "activity_handlers.h"
#include "anatomy.h"
#include "avatar.h"
#include "bionics.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character_martial_arts.h"
#include "clzones.h"
#include "colony.h"
#include "color.h"
#include "construction.h"
#include "coordinate_conversions.h"
#include "coordinates.h"
#include "cursesdef.h"
#include "debug.h"
#include "disease.h"
#include "effect.h"
#include "effect_source.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "field.h"
#include "field_type.h"
#include "fire.h"
#include "flag.h"
#include "fungal_effects.h"
#include "game.h"
#include "game_constants.h"
#include "gun_mode.h"
#include "handle_liquid.h"
#include "input.h"
#include "inventory.h"
#include "item_contents.h"
#include "item_location.h"
#include "item_pocket.h"
#include "item_stack.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "json.h"
#include "lightmap.h"
#include "line.h"
#include "magic.h"
#include "make_static.h"
#include "map.h"
#include "map_iterator.h"
#include "map_selector.h"
#include "mapdata.h"
#include "material.h"
#include "math_defines.h"
#include "memorial_logger.h"
#include "messages.h"
#include "mission.h"
#include "monster.h"
#include "morale.h"
#include "morale_types.h"
#include "move_mode.h"
#include "mtype.h"
#include "mutation.h"
#include "npc.h"
#include "omdata.h"
#include "options.h"
#include "output.h"
#include "overlay_ordering.h"
#include "overmapbuffer.h"
#include "pathfinding.h"
#include "player.h"
#include "proficiency.h"
#include "recipe_dictionary.h"
#include "ret_val.h"
#include "rng.h"
#include "scent_map.h"
#include "skill.h"
#include "skill_boost.h"
#include "sounds.h"
#include "stomach.h"
#include "string_formatter.h"
#include "submap.h"
#include "text_snippets.h"
#include "translations.h"
#include "trap.h"
#include "ui.h"
#include "ui_manager.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "viewer.h"
#include "vitamin.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "weather.h"
#include "weather_gen.h"
#include "weather_type.h"

struct dealt_projectile_attack;

static const activity_id ACT_MOVE_ITEMS( "ACT_MOVE_ITEMS" );
static const activity_id ACT_TRAVELLING( "ACT_TRAVELLING" );
static const activity_id ACT_TREE_COMMUNION( "ACT_TREE_COMMUNION" );
static const activity_id ACT_TRY_SLEEP( "ACT_TRY_SLEEP" );
static const activity_id ACT_WAIT_STAMINA( "ACT_WAIT_STAMINA" );

static const bionic_id bio_soporific( "bio_soporific" );
static const bionic_id bio_uncanny_dodge( "bio_uncanny_dodge" );

static const efftype_id effect_adrenaline( "adrenaline" );
static const efftype_id effect_alarm_clock( "alarm_clock" );
static const efftype_id effect_bandaged( "bandaged" );
static const efftype_id effect_beartrap( "beartrap" );
static const efftype_id effect_bite( "bite" );
static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_blind( "blind" );
static const efftype_id effect_blisters( "blisters" );
static const efftype_id effect_bloodworms( "bloodworms" );
static const efftype_id effect_boomered( "boomered" );
static const efftype_id effect_brainworms( "brainworms" );
static const efftype_id effect_cig( "cig" );
static const efftype_id effect_cold( "cold" );
static const efftype_id effect_common_cold( "common_cold" );
static const efftype_id effect_contacts( "contacts" );
static const efftype_id effect_controlled( "controlled" );
static const efftype_id effect_corroding( "corroding" );
static const efftype_id effect_cough_suppress( "cough_suppress" );
static const efftype_id effect_crushed( "crushed" );
static const efftype_id effect_darkness( "darkness" );
static const efftype_id effect_deaf( "deaf" );
static const efftype_id effect_dermatik( "dermatik" );
static const efftype_id effect_mute( "mute" );
static const efftype_id effect_disinfected( "disinfected" );
static const efftype_id effect_disrupted_sleep( "disrupted_sleep" );
static const efftype_id effect_downed( "downed" );
static const efftype_id effect_drunk( "drunk" );
static const efftype_id effect_earphones( "earphones" );
static const efftype_id effect_flu( "flu" );
static const efftype_id effect_foodpoison( "foodpoison" );
static const efftype_id effect_frostbite( "frostbite" );
static const efftype_id effect_frostbite_recovery( "frostbite_recovery" );
static const efftype_id effect_fungus( "fungus" );
static const efftype_id effect_glowing( "glowing" );
static const efftype_id effect_glowy_led( "glowy_led" );
static const efftype_id effect_got_checked( "got_checked" );
static const efftype_id effect_grabbed( "grabbed" );
static const efftype_id effect_grabbing( "grabbing" );
static const efftype_id effect_harnessed( "harnessed" );
static const efftype_id effect_heating_bionic( "heating_bionic" );
static const efftype_id effect_heavysnare( "heavysnare" );
static const efftype_id effect_hot( "hot" );
static const efftype_id effect_hot_speed( "hot_speed" );
static const efftype_id effect_hunger_blank( "hunger_blank" );
static const efftype_id effect_hunger_engorged( "hunger_engorged" );
static const efftype_id effect_hunger_famished( "hunger_famished" );
static const efftype_id effect_hunger_full( "hunger_full" );
static const efftype_id effect_hunger_hungry( "hunger_hungry" );
static const efftype_id effect_hunger_near_starving( "hunger_near_starving" );
static const efftype_id effect_hunger_satisfied( "hunger_satisfied" );
static const efftype_id effect_hunger_starving( "hunger_starving" );
static const efftype_id effect_hunger_very_hungry( "hunger_very_hungry" );
static const efftype_id effect_hypovolemia( "hypovolemia" );
static const efftype_id effect_in_pit( "in_pit" );
static const efftype_id effect_incorporeal( "incorporeal" );
static const efftype_id effect_infected( "infected" );
static const efftype_id effect_jetinjector( "jetinjector" );
static const efftype_id effect_lack_sleep( "lack_sleep" );
static const efftype_id effect_lightsnare( "lightsnare" );
static const efftype_id effect_lying_down( "lying_down" );
static const efftype_id effect_masked_scent( "masked_scent" );
static const efftype_id effect_melatonin( "melatonin" );
static const efftype_id effect_mending( "mending" );
static const efftype_id effect_meth( "meth" );
static const efftype_id effect_narcosis( "narcosis" );
static const efftype_id effect_nausea( "nausea" );
static const efftype_id effect_nightmares( "nightmares" );
static const efftype_id effect_no_sight( "no_sight" );
static const efftype_id effect_onfire( "onfire" );
static const efftype_id effect_paincysts( "paincysts" );
static const efftype_id effect_pkill1( "pkill1" );
static const efftype_id effect_pkill2( "pkill2" );
static const efftype_id effect_pkill3( "pkill3" );
static const efftype_id effect_recently_coughed( "recently_coughed" );
static const efftype_id effect_recover( "recover" );
static const efftype_id effect_ridden( "ridden" );
static const efftype_id effect_riding( "riding" );
static const efftype_id effect_monster_saddled( "monster_saddled" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_slept_through_alarm( "slept_through_alarm" );
static const efftype_id effect_stunned( "stunned" );
static const efftype_id effect_tapeworm( "tapeworm" );
static const efftype_id effect_tied( "tied" );
static const efftype_id effect_took_prozac( "took_prozac" );
static const efftype_id effect_took_xanax( "took_xanax" );
static const efftype_id effect_webbed( "webbed" );
static const efftype_id effect_weed_high( "weed_high" );
static const efftype_id effect_winded( "winded" );

static const field_type_str_id field_fd_clairvoyant( "fd_clairvoyant" );

static const itype_id itype_adv_UPS_off( "adv_UPS_off" );
static const itype_id itype_apparatus( "apparatus" );
static const itype_id itype_beartrap( "beartrap" );
static const itype_id itype_e_handcuffs( "e_handcuffs" );
static const itype_id itype_fire( "fire" );
static const itype_id itype_rm13_armor_on( "rm13_armor_on" );
static const itype_id itype_rope_6( "rope_6" );
static const itype_id itype_snare_trigger( "snare_trigger" );
static const itype_id itype_string_36( "string_36" );
static const itype_id itype_toolset( "toolset" );
static const itype_id itype_UPS( "UPS" );
static const itype_id itype_UPS_off( "UPS_off" );

static const skill_id skill_archery( "archery" );
static const skill_id skill_dodge( "dodge" );
static const skill_id skill_gun( "gun" );
static const skill_id skill_pistol( "pistol" );
static const skill_id skill_rifle( "rifle" );
static const skill_id skill_shotgun( "shotgun" );
static const skill_id skill_smg( "smg" );
static const skill_id skill_swimming( "swimming" );
static const skill_id skill_throw( "throw" );

static const species_id species_HUMAN( "HUMAN" );
static const species_id species_ROBOT( "ROBOT" );

static const trait_id trait_ACIDBLOOD( "ACIDBLOOD" );
static const trait_id trait_ADRENALINE( "ADRENALINE" );
static const trait_id trait_ANTENNAE( "ANTENNAE" );
static const trait_id trait_ANTLERS( "ANTLERS" );
static const trait_id trait_BADBACK( "BADBACK" );
static const trait_id trait_CF_HAIR( "CF_HAIR" );
static const trait_id trait_CHITIN_FUR( "CHITIN_FUR" );
static const trait_id trait_CHITIN_FUR2( "CHITIN_FUR2" );
static const trait_id trait_CHITIN_FUR3( "CHITIN_FUR3" );
static const trait_id trait_CLUMSY( "CLUMSY" );
static const trait_id trait_DEBUG_NODMG( "DEBUG_NODMG" );
static const trait_id trait_DEFT( "DEFT" );
static const trait_id trait_EASYSLEEPER( "EASYSLEEPER" );
static const trait_id trait_EASYSLEEPER2( "EASYSLEEPER2" );
static const trait_id trait_EATHEALTH( "EATHEALTH" );
static const trait_id trait_FAT( "FAT" );
static const trait_id trait_FELINE_FUR( "FELINE_FUR" );
static const trait_id trait_FUR( "FUR" );
static const trait_id trait_INFIMMUNE( "INFIMMUNE" );
static const trait_id trait_INSOMNIA( "INSOMNIA" );
static const trait_id trait_LIGHTFUR( "LIGHTFUR" );
static const trait_id trait_LUPINE_FUR( "LUPINE_FUR" );
static const trait_id trait_PACIFIST( "PACIFIST" );
static const trait_id trait_PARAIMMUNE( "PARAIMMUNE" );
static const trait_id trait_PROF_SKATER( "PROF_SKATER" );
static const trait_id trait_QUILLS( "QUILLS" );
static const trait_id trait_SAVANT( "SAVANT" );
static const trait_id trait_SPINES( "SPINES" );
static const trait_id trait_SQUEAMISH( "SQUEAMISH" );
static const trait_id trait_THORNS( "THORNS" );
static const trait_id trait_URSINE_FUR( "URSINE_FUR" );
static const trait_id trait_WOOLALLERGY( "WOOLALLERGY" );

static const bionic_id bio_ads( "bio_ads" );
static const bionic_id bio_blaster( "bio_blaster" );
static const bionic_id bio_climate( "bio_climate" );
static const bionic_id bio_voice( "bio_voice" );
static const bionic_id bio_flashlight( "bio_flashlight" );
static const bionic_id bio_gills( "bio_gills" );
static const bionic_id bio_ground_sonar( "bio_ground_sonar" );
static const bionic_id bio_heatsink( "bio_heatsink" );
static const bionic_id bio_hydraulics( "bio_hydraulics" );
static const bionic_id bio_jointservo( "bio_jointservo" );
static const bionic_id bio_laser( "bio_laser" );
static const bionic_id bio_leukocyte( "bio_leukocyte" );
static const bionic_id bio_lighter( "bio_lighter" );
static const bionic_id bio_memory( "bio_memory" );
static const bionic_id bio_railgun( "bio_railgun" );
static const bionic_id bio_recycler( "bio_recycler" );
static const bionic_id bio_shock_absorber( "bio_shock_absorber" );
static const bionic_id bio_tattoo_led( "bio_tattoo_led" );
static const bionic_id bio_tools( "bio_tools" );
static const bionic_id bio_ups( "bio_ups" );
// Aftershock stuff!
static const bionic_id afs_bio_linguistic_coprocessor( "afs_bio_linguistic_coprocessor" );

static const trait_id trait_BADTEMPER( "BADTEMPER" );
static const trait_id trait_BARK( "BARK" );
static const trait_id trait_BIRD_EYE( "BIRD_EYE" );
static const trait_id trait_CEPH_VISION( "CEPH_VISION" );
static const trait_id trait_CHEMIMBALANCE( "CHEMIMBALANCE" );
static const trait_id trait_CHLOROMORPH( "CHLOROMORPH" );
static const trait_id trait_COLDBLOOD( "COLDBLOOD" );
static const trait_id trait_COLDBLOOD2( "COLDBLOOD2" );
static const trait_id trait_COLDBLOOD3( "COLDBLOOD3" );
static const trait_id trait_COLDBLOOD4( "COLDBLOOD4" );
static const trait_id trait_MUTE( "MUTE" );
static const trait_id trait_DEBUG_CLOAK( "DEBUG_CLOAK" );
static const trait_id trait_DEBUG_LS( "DEBUG_LS" );
static const trait_id trait_DEBUG_NIGHTVISION( "DEBUG_NIGHTVISION" );
static const trait_id trait_DEBUG_NOTEMP( "DEBUG_NOTEMP" );
static const trait_id trait_DISRESISTANT( "DISRESISTANT" );
static const trait_id trait_DOWN( "DOWN" );
static const trait_id trait_ELECTRORECEPTORS( "ELECTRORECEPTORS" );
static const trait_id trait_ELFA_FNV( "ELFA_FNV" );
static const trait_id trait_ELFA_NV( "ELFA_NV" );
static const trait_id trait_FASTLEARNER( "FASTLEARNER" );
static const trait_id trait_FAST_REFLEXES( "FAST_REFLEXES" );
static const trait_id trait_FEL_NV( "FEL_NV" );
static const trait_id trait_GILLS( "GILLS" );
static const trait_id trait_GILLS_CEPH( "GILLS_CEPH" );
static const trait_id trait_HEAVYSLEEPER( "HEAVYSLEEPER" );
static const trait_id trait_HEAVYSLEEPER2( "HEAVYSLEEPER2" );
static const trait_id trait_HIBERNATE( "HIBERNATE" );
static const trait_id trait_HOARDER( "HOARDER" );
static const trait_id trait_HOLLOW_BONES( "HOLLOW_BONES" );
static const trait_id trait_HOOVES( "HOOVES" );
static const trait_id trait_HORNS_POINTED( "HORNS_POINTED" );
static const trait_id trait_LEG_TENT_BRACE( "LEG_TENT_BRACE" );
static const trait_id trait_LEG_TENTACLES( "LEG_TENTACLES" );
static const trait_id trait_LIGHT_BONES( "LIGHT_BONES" );
static const trait_id trait_M_DEPENDENT( "M_DEPENDENT" );
static const trait_id trait_M_IMMUNE( "M_IMMUNE" );
static const trait_id trait_M_SKIN3( "M_SKIN3" );
static const trait_id trait_MYOPIC( "MYOPIC" );
static const trait_id trait_NIGHTVISION( "NIGHTVISION" );
static const trait_id trait_NIGHTVISION2( "NIGHTVISION2" );
static const trait_id trait_NIGHTVISION3( "NIGHTVISION3" );
static const trait_id trait_NOMAD( "NOMAD" );
static const trait_id trait_NOMAD2( "NOMAD2" );
static const trait_id trait_NOMAD3( "NOMAD3" );
static const trait_id trait_NOPAIN( "NOPAIN" );
static const trait_id trait_PADDED_FEET( "PADDED_FEET" );
static const trait_id trait_PAWS( "PAWS" );
static const trait_id trait_PAWS_LARGE( "PAWS_LARGE" );
static const trait_id trait_PER_SLIME( "PER_SLIME" );
static const trait_id trait_PER_SLIME_OK( "PER_SLIME_OK" );
static const trait_id trait_PROF_FOODP( "PROF_FOODP" );
static const trait_id trait_QUICK( "QUICK" );
static const trait_id trait_PYROMANIA( "PYROMANIA" );
static const trait_id trait_RADIOGENIC( "RADIOGENIC" );
static const trait_id trait_ROOTS2( "ROOTS2" );
static const trait_id trait_ROOTS3( "ROOTS3" );
static const trait_id trait_SEESLEEP( "SEESLEEP" );
static const trait_id trait_SELFAWARE( "SELFAWARE" );
static const trait_id trait_SHELL2( "SHELL2" );
static const trait_id trait_SHOUT2( "SHOUT2" );
static const trait_id trait_SHOUT3( "SHOUT3" );
static const trait_id trait_SLIMESPAWNER( "SLIMESPAWNER" );
static const trait_id trait_SLIMY( "SLIMY" );
static const trait_id trait_SLOWLEARNER( "SLOWLEARNER" );
static const trait_id trait_STRONGSTOMACH( "STRONGSTOMACH" );
static const trait_id trait_THRESH_CEPHALOPOD( "THRESH_CEPHALOPOD" );
static const trait_id trait_THRESH_INSECT( "THRESH_INSECT" );
static const trait_id trait_THRESH_PLANT( "THRESH_PLANT" );
static const trait_id trait_THRESH_SPIDER( "THRESH_SPIDER" );
static const trait_id trait_TOUGH_FEET( "TOUGH_FEET" );
static const trait_id trait_TRANSPIRATION( "TRANSPIRATION" );
static const trait_id trait_URSINE_EYE( "URSINE_EYE" );
static const trait_id trait_VISCOUS( "VISCOUS" );
static const trait_id trait_WATERSLEEP( "WATERSLEEP" );
static const trait_id trait_WEBBED( "WEBBED" );
static const trait_id trait_WEB_SPINNER( "WEB_SPINNER" );
static const trait_id trait_WEB_WALKER( "WEB_WALKER" );
static const trait_id trait_WEB_WEAVER( "WEB_WEAVER" );

static const std::string flag_PLOWABLE( "PLOWABLE" );

static const json_character_flag json_flag_ALARMCLOCK( "ALARMCLOCK" );
static const json_character_flag json_flag_ACID_IMMUNE( "ACID_IMMUNE" );
static const json_character_flag json_flag_BASH_IMMUNE( "BASH_IMMUNE" );
static const json_character_flag json_flag_BIO_IMMUNE( "BIO_IMMUNE" );
static const json_character_flag json_flag_BLIND( "BLIND" );
static const json_character_flag json_flag_BULLET_IMMUNE( "BULLET_IMMUNE" );
static const json_character_flag json_flag_CLAIRVOYANCE( "CLAIRVOYANCE" );
static const json_character_flag json_flag_CLAIRVOYANCE_PLUS( "CLAIRVOYANCE_PLUS" );
static const json_character_flag json_flag_COLD_IMMUNE( "COLD_IMMUNE" );
static const json_character_flag json_flag_CUT_IMMUNE( "CUT_IMMUNE" );
static const json_character_flag json_flag_DEAF( "DEAF" );
static const json_character_flag json_flag_ELECTRIC_IMMUNE( "ELECTRIC_IMMUNE" );
static const json_character_flag json_flag_ENHANCED_VISION( "ENHANCED_VISION" );
static const json_character_flag json_flag_EYE_MEMBRANE( "EYE_MEMBRANE" );
static const json_character_flag json_flag_HEATPROOF( "HEATPROOF" );
static const json_character_flag json_flag_IMMUNE_HEARING_DAMAGE( "IMMUNE_HEARING_DAMAGE" );
static const json_character_flag json_flag_INFRARED( "INFRARED" );
static const json_character_flag json_flag_NIGHT_VISION( "NIGHT_VISION" );
static const json_character_flag json_flag_NO_DISEASE( "NO_DISEASE" );
static const json_character_flag json_flag_NO_MINIMAL_HEALING( "NO_MINIMAL_HEALING" );
static const json_character_flag json_flag_NO_RADIATION( "NO_RADIATION" );
static const json_character_flag json_flag_NO_THIRST( "NO_THIRST" );
static const json_character_flag json_flag_NON_THRESH( "NON_THRESH" );
static const json_character_flag json_flag_PRED2( "PRED2" );
static const json_character_flag json_flag_PRED3( "PRED3" );
static const json_character_flag json_flag_PRED4( "PRED4" );
static const json_character_flag json_flag_STAB_IMMUNE( "STAB_IMMUNE" );
static const json_character_flag json_flag_STEADY( "STEADY" );
static const json_character_flag json_flag_STOP_SLEEP_DEPRIVATION( "STOP_SLEEP_DEPRIVATION" );
static const json_character_flag json_flag_SUPER_CLAIRVOYANCE( "SUPER_CLAIRVOYANCE" );
static const json_character_flag json_flag_SUPER_HEARING( "SUPER_HEARING" );
static const json_character_flag json_flag_UNCANNY_DODGE( "UNCANNY_DODGE" );
static const json_character_flag json_flag_WATCH( "WATCH" );

static const mtype_id mon_player_blob( "mon_player_blob" );

static const vitamin_id vitamin_blood( "blood" );
static const morale_type morale_nightmare( "morale_nightmare" );

namespace io
{

template<>
std::string enum_to_string<blood_type>( blood_type data )
{
    switch( data ) {
            // *INDENT-OFF*
        case blood_type::blood_O: return "O";
        case blood_type::blood_A: return "A";
        case blood_type::blood_B: return "B";
        case blood_type::blood_AB: return "AB";
            // *INDENT-ON*
        case blood_type::num_bt:
            break;
    }
    debugmsg( "Invalid blood_type" );
    abort();
}

} // namespace io

// *INDENT-OFF*
Character::Character() :
    cached_time( calendar::before_time_starts ),
    id( -1 ),
    next_climate_control_check( calendar::before_time_starts ),
    last_climate_control_ret( false )
{
    randomize_blood();
    str_max = 0;
    dex_max = 0;
    per_max = 0;
    int_max = 0;
    str_cur = 0;
    dex_cur = 0;
    per_cur = 0;
    int_cur = 0;
    str_bonus = 0;
    dex_bonus = 0;
    per_bonus = 0;
    int_bonus = 0;
    healthy = 0;
    healthy_mod = 0;
    hunger = 0;
    thirst = 0;
    fatigue = 0;
    sleep_deprivation = 0;
    set_rad( 0 );
    slow_rad = 0;
    set_stim( 0 );
    set_stamina( 10000 ); //Temporary value for stamina. It will be reset later from external json option.
    set_anatomy( anatomy_id("human_anatomy") );
    update_type_of_scent( true );
    pkill = 0;
    // 45 days to starve to death
    // 55 Mcal or 55k kcal
    healthy_calories = 55'000'000;
    stored_calories = healthy_calories;
    initialize_stomach_contents();

    name.clear();
    custom_profession.clear();

    *path_settings = pathfinding_settings{ 0, 1000, 1000, 0, true, true, true, false, true };

    move_mode = move_mode_id( "walk" );
    next_expected_position = cata::nullopt;
}
// *INDENT-ON*

Character::~Character() = default;
Character::Character( Character && ) = default;
Character &Character::operator=( Character && ) = default;

void Character::setID( character_id i, bool force )
{
    if( id.is_valid() && !force ) {
        debugmsg( "tried to set id of a npc/player, but has already a id: %d", id.get_value() );
    } else if( !i.is_valid() && !force ) {
        debugmsg( "tried to set invalid id of a npc/player: %d", i.get_value() );
    } else {
        id = i;
    }
}

character_id Character::getID() const
{
    return this->id;
}

void Character::randomize_blood()
{
    // Blood type distribution data is taken from this study on blood types of
    // COVID-19 patients presented to five major hospitals in Massachusetts:
    // https://www.ncbi.nlm.nih.gov/pmc/articles/PMC7354354/
    // and is adjusted for racial/ethnic distribution in game, which is defined in
    // data/json/npcs/appearance_trait_groups.json
    static const std::array<std::tuple<double, blood_type, bool>, 8> blood_type_distribution = {{
            std::make_tuple( 0.3821, blood_type::blood_O, true ),
            std::make_tuple( 0.0387, blood_type::blood_O, false ),
            std::make_tuple( 0.3380, blood_type::blood_A, true ),
            std::make_tuple( 0.0414, blood_type::blood_A, false ),
            std::make_tuple( 0.1361, blood_type::blood_B, true ),
            std::make_tuple( 0.0134, blood_type::blood_B, false ),
            std::make_tuple( 0.0437, blood_type::blood_AB, true ),
            std::make_tuple( 0.0066, blood_type::blood_AB, false )
        }
    };
    const double x = rng_float( 0.0, 1.0 );
    double cumulative_prob = 0.0;
    for( const std::tuple<double, blood_type, bool> &type : blood_type_distribution ) {
        cumulative_prob += std::get<0>( type );
        if( x <= cumulative_prob ) {
            my_blood_type = std::get<1>( type );
            blood_rh_factor = std::get<2>( type );
            return;
        }
    }
    my_blood_type = blood_type::blood_AB;
    blood_rh_factor = false;
}

field_type_id Character::bloodType() const
{
    if( has_trait( trait_ACIDBLOOD ) ) {
        return fd_acid;
    }
    if( has_trait( trait_THRESH_PLANT ) ) {
        return fd_blood_veggy;
    }
    if( has_trait( trait_THRESH_INSECT ) || has_trait( trait_THRESH_SPIDER ) ) {
        return fd_blood_insect;
    }
    if( has_trait( trait_THRESH_CEPHALOPOD ) ) {
        return fd_blood_invertebrate;
    }
    return fd_blood;
}
field_type_id Character::gibType() const
{
    return fd_gibs_flesh;
}

bool Character::in_species( const species_id &spec ) const
{
    return spec == species_HUMAN;
}

bool Character::is_warm() const
{
    // TODO: is there a mutation (plant?) that makes a npc not warm blooded?
    return true;
}

const std::string &Character::symbol() const
{
    static const std::string character_symbol( "@" );
    return character_symbol;
}

void Character::mod_stat( const std::string &stat, float modifier )
{
    if( stat == "str" ) {
        mod_str_bonus( modifier );
    } else if( stat == "dex" ) {
        mod_dex_bonus( modifier );
    } else if( stat == "per" ) {
        mod_per_bonus( modifier );
    } else if( stat == "int" ) {
        mod_int_bonus( modifier );
    } else if( stat == "healthy" ) {
        mod_healthy( modifier );
    } else if( stat == "hunger" ) {
        mod_hunger( modifier );
    } else {
        Creature::mod_stat( stat, modifier );
    }
}

int Character::get_fat_to_hp() const
{
    float mut_fat_hp = 0.0f;
    for( const trait_id &mut : get_mutations() ) {
        mut_fat_hp += mut.obj().fat_to_max_hp;
    }

    return mut_fat_hp * ( get_bmi() - character_weight_category::normal );
}

creature_size Character::get_size() const
{
    return size_class;
}

std::string Character::disp_name( bool possessive, bool capitalize_first ) const
{
    if( !possessive ) {
        if( is_player() ) {
            return capitalize_first ? _( "You" ) : _( "you" );
        }
        return name;
    } else {
        if( is_player() ) {
            return capitalize_first ? _( "Your" ) : _( "your" );
        }
        return string_format( _( "%s's" ), name );
    }
}

std::string Character::skin_name() const
{
    // TODO: Return actual deflecting layer name
    return _( "armor" );
}

int Character::effective_dispersion( int dispersion ) const
{
    /** @EFFECT_PER penalizes sight dispersion when low. */
    dispersion += ranged_per_mod();

    dispersion += encumb( body_part_eyes ) / 2;

    return std::max( dispersion, 0 );
}

std::pair<int, int> Character::get_fastest_sight( const item &gun, double recoil ) const
{
    // Get fastest sight that can be used to improve aim further below @ref recoil.
    int sight_speed_modifier = INT_MIN;
    int limit = 0;
    if( effective_dispersion( gun.type->gun->sight_dispersion ) < recoil ) {
        sight_speed_modifier = gun.has_flag( flag_DISABLE_SIGHTS ) ? 0 : 6;
        limit = effective_dispersion( gun.type->gun->sight_dispersion );
    }

    for( const item *e : gun.gunmods() ) {
        const islot_gunmod &mod = *e->type->gunmod;
        if( mod.sight_dispersion < 0 || mod.aim_speed < 0 ) {
            continue; // skip gunmods which don't provide a sight
        }
        if( effective_dispersion( mod.sight_dispersion ) < recoil &&
            mod.aim_speed > sight_speed_modifier ) {
            sight_speed_modifier = mod.aim_speed;
            limit = effective_dispersion( mod.sight_dispersion );
        }
    }
    return std::make_pair( sight_speed_modifier, limit );
}

int Character::get_most_accurate_sight( const item &gun ) const
{
    if( !gun.is_gun() ) {
        return 0;
    }

    int limit = effective_dispersion( gun.type->gun->sight_dispersion );
    for( const item *e : gun.gunmods() ) {
        const islot_gunmod &mod = *e->type->gunmod;
        if( mod.aim_speed >= 0 ) {
            limit = std::min( limit, effective_dispersion( mod.sight_dispersion ) );
        }
    }

    return limit;
}

double Character::aim_speed_skill_modifier( const skill_id &gun_skill ) const
{
    double skill_mult = 1.0;
    if( gun_skill == skill_pistol ) {
        skill_mult = 2.0;
    } else if( gun_skill == skill_rifle ) {
        skill_mult = 0.9;
    }
    /** @EFFECT_PISTOL increases aiming speed for pistols */
    /** @EFFECT_SMG increases aiming speed for SMGs */
    /** @EFFECT_RIFLE increases aiming speed for rifles */
    /** @EFFECT_SHOTGUN increases aiming speed for shotguns */
    /** @EFFECT_LAUNCHER increases aiming speed for launchers */
    return skill_mult * std::min( MAX_SKILL, get_skill_level( gun_skill ) );
}

double Character::aim_speed_dex_modifier() const
{
    return get_dex() - 8;
}

double Character::aim_speed_encumbrance_modifier() const
{
    return ( encumb( body_part_hand_l ) + encumb( body_part_hand_r ) ) / 10.0;
}

double Character::aim_cap_from_volume( const item &gun ) const
{
    skill_id gun_skill = gun.gun_skill();

    units::volume wielded_volume = gun.volume();
    if( gun.has_flag( flag_COLLAPSIBLE_STOCK ) ) {
        // use the unfolded volume
        wielded_volume += gun.collapsed_volume_delta();
    }

    double aim_cap = std::min( 49.0, 49.0 - static_cast<float>( wielded_volume / 75_ml ) );
    // TODO: also scale with skill level.
    if( gun_skill == skill_smg ) {
        aim_cap = std::max( 12.0, aim_cap );
    } else if( gun_skill == skill_shotgun ) {
        aim_cap = std::max( 12.0, aim_cap );
    } else if( gun_skill == skill_pistol ) {
        aim_cap = std::max( 15.0, aim_cap * 1.25 );
    } else if( gun_skill == skill_rifle ) {
        aim_cap = std::max( 7.0, aim_cap - 7.0 );
    } else if( gun_skill == skill_archery ) {
        aim_cap = std::max( 13.0, aim_cap );
    } else { // Launchers, etc.
        aim_cap = std::max( 10.0, aim_cap );
    }
    return aim_cap;
}

double Character::aim_per_move( const item &gun, double recoil ) const
{
    if( !gun.is_gun() ) {
        return 0.0;
    }

    std::pair<int, int> best_sight = get_fastest_sight( gun, recoil );
    int sight_speed_modifier = best_sight.first;
    int limit = best_sight.second;
    if( sight_speed_modifier == INT_MIN ) {
        // No suitable sights (already at maximum aim).
        return 0;
    }

    // Overall strategy for determining aim speed is to sum the factors that contribute to it,
    // then scale that speed by current recoil level.
    // Player capabilities make aiming faster, and aim speed slows down as it approaches 0.
    // Base speed is non-zero to prevent extreme rate changes as aim speed approaches 0.
    double aim_speed = 10.0;

    skill_id gun_skill = gun.gun_skill();
    // Ranges [0 - 10]
    aim_speed += aim_speed_skill_modifier( gun_skill );

    // Range [0 - 12]
    /** @EFFECT_DEX increases aiming speed */
    aim_speed += aim_speed_dex_modifier();

    // Range [0 - 10]
    aim_speed += sight_speed_modifier;

    // Each 5 points (combined) of hand encumbrance decreases aim speed by one unit.
    aim_speed -= aim_speed_encumbrance_modifier();

    aim_speed = std::min( aim_speed, aim_cap_from_volume( gun ) );

    // Just a raw scaling factor.
    aim_speed *= 6.5;

    // Scale rate logistically as recoil goes from MAX_RECOIL to 0.
    aim_speed *= 1.0 - logarithmic_range( 0, MAX_RECOIL, recoil );

    // Minimum improvement is 5MoA.  This mostly puts a cap on how long aiming for sniping takes.
    aim_speed = std::max( aim_speed, 5.0 );

    // Never improve by more than the currently used sights permit.
    return std::min( aim_speed, recoil - limit );
}

const tripoint &Character::pos() const
{
    return position;
}

int Character::sight_range( int light_level ) const
{
    if( light_level == 0 ) {
        return 1;
    }
    /* Via Beer-Lambert we have:
     * light_level * (1 / exp( LIGHT_TRANSPARENCY_OPEN_AIR * distance) ) <= LIGHT_AMBIENT_LOW
     * Solving for distance:
     * 1 / exp( LIGHT_TRANSPARENCY_OPEN_AIR * distance ) <= LIGHT_AMBIENT_LOW / light_level
     * 1 <= exp( LIGHT_TRANSPARENCY_OPEN_AIR * distance ) * LIGHT_AMBIENT_LOW / light_level
     * light_level <= exp( LIGHT_TRANSPARENCY_OPEN_AIR * distance ) * LIGHT_AMBIENT_LOW
     * log(light_level) <= LIGHT_TRANSPARENCY_OPEN_AIR * distance + log(LIGHT_AMBIENT_LOW)
     * log(light_level) - log(LIGHT_AMBIENT_LOW) <= LIGHT_TRANSPARENCY_OPEN_AIR * distance
     * log(LIGHT_AMBIENT_LOW / light_level) <= LIGHT_TRANSPARENCY_OPEN_AIR * distance
     * log(LIGHT_AMBIENT_LOW / light_level) * (1 / LIGHT_TRANSPARENCY_OPEN_AIR) <= distance
     */
    int range = static_cast<int>( -std::log( get_vision_threshold( static_cast<int>
                                  ( get_map().ambient_light_at( pos() ) ) ) / static_cast<float>( light_level ) ) *
                                  ( 1.0 / LIGHT_TRANSPARENCY_OPEN_AIR ) );

    // Clamp to [1, sight_max].
    return clamp( range, 1, sight_max );
}

int Character::unimpaired_range() const
{
    return std::min( sight_max, 60 );
}

bool Character::overmap_los( const tripoint_abs_omt &omt, int sight_points )
{
    const tripoint_abs_omt ompos = global_omt_location();
    const point_rel_omt offset = omt.xy() - ompos.xy();
    if( offset.x() < -sight_points || offset.x() > sight_points ||
        offset.y() < -sight_points || offset.y() > sight_points ) {
        // Outside maximum sight range
        return false;
    }

    // TODO: fix point types
    const std::vector<tripoint> line = line_to( ompos.raw(), omt.raw(), 0, 0 );
    for( size_t i = 0; i < line.size() && sight_points >= 0; i++ ) {
        const tripoint &pt = line[i];
        const oter_id &ter = overmap_buffer.ter( tripoint_abs_omt( pt ) );
        sight_points -= static_cast<int>( ter->get_see_cost() );
        if( sight_points < 0 ) {
            return false;
        }
    }
    return true;
}

int Character::overmap_sight_range( int light_level ) const
{
    int sight = sight_range( light_level );
    if( sight < SEEX ) {
        return 0;
    }
    if( sight <= SEEX * 4 ) {
        return ( sight / ( SEEX / 2 ) );
    }

    sight = 6;
    // The higher your perception, the farther you can see.
    sight += static_cast<int>( get_per() / 2 );
    // The higher up you are, the farther you can see.
    sight += std::max( 0, posz() ) * 2;
    // Mutations like Scout and Topographagnosia affect how far you can see.
    sight += mutation_value( "overmap_sight" );

    float multiplier = mutation_value( "overmap_multiplier" );
    // Binoculars double your sight range.
    const bool has_optic = ( has_item_with_flag( flag_ZOOM ) || has_flag( json_flag_ENHANCED_VISION ) ||
                             ( is_mounted() &&
                               mounted_creature->has_flag( MF_MECH_RECON_VISION ) ) );
    if( has_optic ) {
        multiplier += 1;
    }

    sight = std::round( sight * multiplier );
    return std::max( sight, 3 );
}

int Character::clairvoyance() const
{
    if( vision_mode_cache[VISION_CLAIRVOYANCE_SUPER] ) {
        return MAX_CLAIRVOYANCE;
    }

    if( vision_mode_cache[VISION_CLAIRVOYANCE_PLUS] ) {
        return 8;
    }

    if( vision_mode_cache[VISION_CLAIRVOYANCE] ) {
        return 3;
    }

    return 0;
}

bool Character::sight_impaired() const
{
    const bool in_light = get_map().ambient_light_at( pos() ) > LIGHT_AMBIENT_LIT;
    return ( ( ( has_effect( effect_boomered ) || has_effect( effect_no_sight ) ||
                 has_effect( effect_darkness ) ) &&
               ( !( has_trait( trait_PER_SLIME_OK ) ) ) ) ||
             ( underwater && !has_flag( json_flag_EYE_MEMBRANE ) &&
               !worn_with_flag( flag_SWIM_GOGGLES ) ) ||
             ( ( has_trait( trait_MYOPIC ) || ( in_light && has_trait( trait_URSINE_EYE ) ) ) &&
               !worn_with_flag( flag_FIX_NEARSIGHT ) &&
               !has_effect( effect_contacts ) &&
               !has_flag( json_flag_ENHANCED_VISION ) ) ||
             has_trait( trait_PER_SLIME ) || is_blind() );
}

bool Character::has_alarm_clock() const
{
    map &here = get_map();
    return ( has_item_with_flag( flag_ALARMCLOCK, true ) ||
             ( here.veh_at( pos() ) &&
               !empty( here.veh_at( pos() )->vehicle().get_avail_parts( "ALARMCLOCK" ) ) ) ||
             has_flag( json_flag_ALARMCLOCK ) );
}

bool Character::has_watch() const
{
    map &here = get_map();
    return ( has_item_with_flag( flag_WATCH, true ) ||
             ( here.veh_at( pos() ) &&
               !empty( here.veh_at( pos() )->vehicle().get_avail_parts( "WATCH" ) ) ) ||
             has_flag( json_flag_WATCH ) );
}

void Character::react_to_felt_pain( int intensity )
{
    if( intensity <= 0 ) {
        return;
    }
    if( is_player() && intensity >= 2 ) {
        g->cancel_activity_or_ignore_query( distraction_type::pain, _( "Ouch, something hurts!" ) );
    }
    // Only a large pain burst will actually wake people while sleeping.
    if( has_effect( effect_sleep ) && !has_effect( effect_narcosis ) ) {
        int pain_thresh = rng( 3, 5 );

        if( has_trait( trait_HEAVYSLEEPER ) ) {
            pain_thresh += 2;
        } else if( has_trait( trait_HEAVYSLEEPER2 ) ) {
            pain_thresh += 5;
        }

        if( intensity >= pain_thresh ) {
            wake_up();
        }
    }
}

void Character::action_taken()
{
    nv_cached = false;
}

int Character::swim_speed() const
{
    int ret;
    if( is_mounted() ) {
        monster *mon = mounted_creature.get();
        // no difference in swim speed by monster type yet.
        // TODO: difference in swim speed by monster type.
        // No monsters are currently mountable and can swim, though mods may allow this.
        if( mon->swims() ) {
            ret = 25;
            ret += get_weight() / 120_gram - 50 * ( mon->get_size() - 1 );
            return ret;
        }
    }
    const body_part_set usable = exclusive_flag_coverage( flag_ALLOWS_NATURAL_ATTACKS );
    float hand_bonus_mult = ( usable.test( body_part_hand_l ) ? 0.5f : 0.0f ) +
                            ( usable.test( body_part_hand_r ) ? 0.5f : 0.0f );

    // base swim speed.
    ret = ( 440 * mutation_value( "movecost_swim_modifier" ) ) + weight_carried() /
          ( 60_gram / mutation_value( "movecost_swim_modifier" ) ) - 50 * get_skill_level( skill_swimming );
    /** @EFFECT_STR increases swim speed bonus from PAWS */
    if( has_trait( trait_PAWS ) ) {
        ret -= hand_bonus_mult * ( 20 + str_cur * 3 );
    }
    /** @EFFECT_STR increases swim speed bonus from PAWS_LARGE */
    if( has_trait( trait_PAWS_LARGE ) ) {
        ret -= hand_bonus_mult * ( 20 + str_cur * 4 );
    }
    /** @EFFECT_STR increases swim speed bonus from swim_fins */
    if( worn_with_flag( flag_FIN, body_part_foot_l ) ||
        worn_with_flag( flag_FIN, body_part_foot_r ) ) {
        if( worn_with_flag( flag_FIN, body_part_foot_l ) &&
            worn_with_flag( flag_FIN, body_part_foot_r ) ) {
            ret -= ( 15 * str_cur );
        } else {
            ret -= ( 15 * str_cur ) / 2;
        }
    }
    /** @EFFECT_STR increases swim speed bonus from WEBBED */
    if( has_trait( trait_WEBBED ) ) {
        ret -= hand_bonus_mult * ( 60 + str_cur * 5 );
    }
    /** @EFFECT_SWIMMING increases swim speed */
    ret += ( 50 - get_skill_level( skill_swimming ) * 2 ) * ( ( encumb( body_part_leg_l ) +
            encumb( body_part_leg_r ) ) / 10 );
    ret += ( 80 - get_skill_level( skill_swimming ) * 3 ) * ( encumb( body_part_torso ) / 10 );
    if( get_skill_level( skill_swimming ) < 10 ) {
        for( const item &i : worn ) {
            ret += i.volume() / 125_ml * ( 10 - get_skill_level( skill_swimming ) );
        }
    }
    /** @EFFECT_STR increases swim speed */

    /** @EFFECT_DEX increases swim speed */
    ret -= str_cur * 6 + dex_cur * 4;
    if( worn_with_flag( flag_FLOTATION ) ) {
        ret = std::min( ret, 400 );
        ret = std::max( ret, 200 );
    }
    // If (ret > 500), we can not swim; so do not apply the underwater bonus.
    if( underwater && ret < 500 ) {
        ret -= 50;
    }

    ret += move_mode->swim_speed_mod();

    if( ret < 30 ) {
        ret = 30;
    }
    return ret;
}

bool Character::is_on_ground() const
{
    return get_working_leg_count() < 2 || has_effect( effect_downed );
}

bool Character::can_stash( const item &it )
{
    return best_pocket( it, nullptr ).second != nullptr;
}

bool Character::can_stash_partial( const item &it )
{
    item copy = it;
    if( it.count_by_charges() ) {
        copy.charges = 1;
    }

    return can_stash( copy );
}

void Character::cancel_stashed_activity()
{
    stashed_outbounds_activity = player_activity();
    stashed_outbounds_backlog = player_activity();
}

player_activity Character::get_stashed_activity() const
{
    return stashed_outbounds_activity;
}

void Character::set_stashed_activity( const player_activity &act, const player_activity &act_back )
{
    stashed_outbounds_activity = act;
    stashed_outbounds_backlog = act_back;
}

bool Character::has_stashed_activity() const
{
    return static_cast<bool>( stashed_outbounds_activity );
}

void Character::assign_stashed_activity()
{
    activity = stashed_outbounds_activity;
    backlog.push_front( stashed_outbounds_backlog );
    cancel_stashed_activity();
}

bool Character::check_outbounds_activity( const player_activity &act, bool check_only )
{
    map &here = get_map();
    if( ( act.placement != tripoint_zero && act.placement != tripoint_min &&
          !here.inbounds( here.getlocal( act.placement ) ) ) || ( !act.coords.empty() &&
                  !here.inbounds( here.getlocal( act.coords.back() ) ) ) ) {
        if( is_npc() && !check_only ) {
            // stash activity for when reloaded.
            stashed_outbounds_activity = act;
            if( !backlog.empty() ) {
                stashed_outbounds_backlog = backlog.front();
            }
            activity = player_activity();
        }
        add_msg_debug(
            "npc %s at pos %d %d, activity target is not inbounds at %d %d therefore activity was stashed",
            disp_name(), pos().x, pos().y, act.placement.x, act.placement.y );
        return true;
    }
    return false;
}

void Character::set_destination_activity( const player_activity &new_destination_activity )
{
    destination_activity = new_destination_activity;
}

void Character::clear_destination_activity()
{
    destination_activity = player_activity();
}

player_activity Character::get_destination_activity() const
{
    return destination_activity;
}

void Character::mount_creature( monster &z )
{
    tripoint pnt = z.pos();
    shared_ptr_fast<monster> mons = g->shared_from( z );
    if( mons == nullptr ) {
        add_msg_debug( "mount_creature(): monster not found in critter_tracker" );
        return;
    }
    add_effect( effect_riding, 1_turns, true );
    z.add_effect( effect_ridden, 1_turns, true );
    if( z.has_effect( effect_tied ) ) {
        z.remove_effect( effect_tied );
        if( z.tied_item ) {
            i_add( *z.tied_item );
            z.tied_item.reset();
        }
    }
    z.mounted_player_id = getID();
    if( z.has_effect( effect_harnessed ) ) {
        z.remove_effect( effect_harnessed );
        add_msg_if_player( m_info, _( "You remove the %s's harness." ), z.get_name() );
    }
    mounted_creature = mons;
    mons->mounted_player = this;
    if( is_avatar() ) {
        avatar &player_character = get_avatar();
        if( player_character.is_hauling() ) {
            player_character.stop_hauling();
        }
        if( player_character.get_grab_type() != object_type::NONE ) {
            add_msg( m_warning, _( "You let go of the grabbed object." ) );
            player_character.grab( object_type::NONE );
        }
        g->place_player( pnt );
    } else {
        npc &guy = dynamic_cast<npc &>( *this );
        guy.setpos( pnt );
    }
    z.facing = facing;
    add_msg_if_player( m_good, _( "You climb on the %s." ), z.get_name() );
    if( z.has_flag( MF_RIDEABLE_MECH ) ) {
        if( !z.type->mech_weapon.is_empty() ) {
            item mechwep = item( z.type->mech_weapon );
            wield( mechwep );
        }
        add_msg_if_player( m_good, _( "You hear your %s whir to life." ), z.get_name() );
    }
    // some rideable mechs have night-vision
    recalc_sight_limits();
    mod_moves( -100 );
}

bool Character::check_mount_will_move( const tripoint &dest_loc )
{
    if( !is_mounted() ) {
        return true;
    }
    if( mounted_creature && mounted_creature->type->has_fear_trigger( mon_trigger::HOSTILE_CLOSE ) ) {
        for( const monster &critter : g->all_monsters() ) {
            Attitude att = critter.attitude_to( *this );
            if( att == Attitude::HOSTILE && sees( critter ) && rl_dist( pos(), critter.pos() ) <= 15 &&
                rl_dist( dest_loc, critter.pos() ) < rl_dist( pos(), critter.pos() ) ) {
                add_msg_if_player( _( "You fail to budge your %s!" ), mounted_creature->get_name() );
                return false;
            }
        }
    }
    return true;
}

bool Character::check_mount_is_spooked()
{
    if( !is_mounted() ) {
        return false;
    }
    // chance to spook per monster nearby:
    // base 1% per turn.
    // + 1% per square closer than 15 distance. (1% - 15%)
    // * 2 if hostile monster is bigger than or same size as mounted creature.
    // -0.25% per point of dexterity (low -1%, average -2%, high -3%, extreme -3.5%)
    // -0.1% per point of strength ( low -0.4%, average -0.8%, high -1.2%, extreme -1.4% )
    // / 2 if horse has full tack and saddle.
    // Monster in spear reach monster and average stat (8) player on saddled horse, 14% -2% -0.8% / 2 = ~5%
    if( mounted_creature && mounted_creature->type->has_fear_trigger( mon_trigger::HOSTILE_CLOSE ) ) {
        const creature_size mount_size = mounted_creature->get_size();
        const bool saddled = mounted_creature->has_effect( effect_monster_saddled );
        for( const monster &critter : g->all_monsters() ) {
            double chance = 1.0;
            Attitude att = critter.attitude_to( *this );
            // actually too close now - horse might spook.
            if( att == Attitude::HOSTILE && sees( critter ) && rl_dist( pos(), critter.pos() ) <= 10 ) {
                chance += 10 - rl_dist( pos(), critter.pos() );
                if( critter.get_size() >= mount_size ) {
                    chance *= 2;
                }
                chance -= 0.25 * get_dex();
                chance -= 0.1 * get_str();
                if( saddled ) {
                    chance /= 2;
                }
                chance = std::max( 1.0, chance );
                if( x_in_y( chance, 100.0 ) ) {
                    forced_dismount();
                    return true;
                }
            }
        }
    }
    return false;
}

bool Character::is_mounted() const
{
    return has_effect( effect_riding ) && mounted_creature;
}

void Character::forced_dismount()
{
    remove_effect( effect_riding );
    bool mech = false;
    if( mounted_creature ) {
        auto *mon = mounted_creature.get();
        if( mon->has_flag( MF_RIDEABLE_MECH ) && !mon->type->mech_weapon.is_empty() ) {
            mech = true;
            remove_item( weapon );
        }
        mon->mounted_player_id = character_id();
        mon->remove_effect( effect_ridden );
        mon->add_effect( effect_controlled, 5_turns );
        mounted_creature = nullptr;
        mon->mounted_player = nullptr;
    }
    std::vector<tripoint> valid;
    for( const tripoint &jk : get_map().points_in_radius( pos(), 1 ) ) {
        if( g->is_empty( jk ) ) {
            valid.push_back( jk );
        }
    }
    if( !valid.empty() ) {
        setpos( random_entry( valid ) );
        if( mech ) {
            add_msg_player_or_npc( m_bad, _( "You are ejected from your mech!" ),
                                   _( "<npcname> is ejected from their mech!" ) );
        } else {
            add_msg_player_or_npc( m_bad, _( "You fall off your mount!" ),
                                   _( "<npcname> falls off their mount!" ) );
        }
        const int dodge = get_dodge();
        const int damage = std::max( 0, rng( 1, 20 ) - rng( dodge, dodge * 2 ) );
        bodypart_id hit( "bp_null" );
        switch( rng( 1, 10 ) ) {
            case  1:
                if( one_in( 2 ) ) {
                    hit = body_part_foot_l;
                } else {
                    hit = body_part_foot_r;
                }
                break;
            case  2:
            case  3:
            case  4:
                if( one_in( 2 ) ) {
                    hit = body_part_leg_l;
                } else {
                    hit = body_part_leg_r;
                }
                break;
            case  5:
            case  6:
            case  7:
                if( one_in( 2 ) ) {
                    hit = body_part_arm_l;
                } else {
                    hit = body_part_arm_r;
                }
                break;
            case  8:
            case  9:
                hit = body_part_torso;
                break;
            case 10:
                hit = body_part_head;
                break;
        }
        if( damage > 0 ) {
            add_msg_if_player( m_bad, _( "You hurt yourself!" ) );
            deal_damage( nullptr, hit, damage_instance( damage_type::BASH, damage ) );
            if( is_avatar() ) {
                get_memorial().add(
                    pgettext( "memorial_male", "Fell off a mount." ),
                    pgettext( "memorial_female", "Fell off a mount." ) );
            }
            check_dead_state();
        }
        add_effect( effect_downed, 5_turns, true );
    } else {
        add_msg_debug( "Forced_dismount could not find a square to deposit player" );
    }
    if( is_avatar() ) {
        avatar &player_character = get_avatar();
        if( player_character.get_grab_type() != object_type::NONE ) {
            add_msg( m_warning, _( "You let go of the grabbed object." ) );
            player_character.grab( object_type::NONE );
        }
        set_movement_mode( move_mode_id( "walk" ) );
        if( player_character.is_auto_moving() || player_character.has_destination() ||
            player_character.has_destination_activity() ) {
            player_character.clear_destination();
        }
        g->update_map( player_character );
    }
    if( activity ) {
        cancel_activity();
    }
    moves -= 150;
}

void Character::dismount()
{
    if( !is_mounted() ) {
        add_msg_debug( "dismount called when not riding" );
        return;
    }
    if( const cata::optional<tripoint> pnt = choose_adjacent( _( "Dismount where?" ) ) ) {
        if( !g->is_empty( *pnt ) ) {
            add_msg( m_warning, _( "You cannot dismount there!" ) );
            return;
        }
        remove_effect( effect_riding );
        monster *critter = mounted_creature.get();
        critter->mounted_player_id = character_id();
        if( critter->has_flag( MF_RIDEABLE_MECH ) && !critter->type->mech_weapon.is_empty() &&
            weapon.typeId() == critter->type->mech_weapon ) {
            remove_item( weapon );
        }
        avatar &player_character = get_avatar();
        if( is_avatar() && player_character.get_grab_type() != object_type::NONE ) {
            add_msg( m_warning, _( "You let go of the grabbed object." ) );
            player_character.grab( object_type::NONE );
        }
        critter->remove_effect( effect_ridden );
        critter->add_effect( effect_controlled, 5_turns );
        mounted_creature = nullptr;
        critter->mounted_player = nullptr;
        setpos( *pnt );
        mod_moves( -100 );
        set_movement_mode( move_mode_id( "walk" ) );
    }
}

float Character::stability_roll() const
{
    /** @EFFECT_STR improves player stability roll */

    /** @EFFECT_PER slightly improves player stability roll */

    /** @EFFECT_DEX slightly improves player stability roll */

    /** @EFFECT_MELEE improves player stability roll */
    return get_melee() + get_str() + ( get_per() / 3.0f ) + ( get_dex() / 4.0f );
}

bool Character::is_dead_state() const
{
    return get_part_hp_cur( body_part_head ) <= 0 ||
           get_part_hp_cur( body_part_torso ) <= 0;
}

void Character::on_dodge( Creature *source, float difficulty )
{
    static const matec_id tec_none( "tec_none" );

    // Each avoided hit consumes an available dodge
    // When no more available we are likely to fail player::dodge_roll
    dodges_left--;

    // dodging throws of our aim unless we are either skilled at dodging or using a small weapon
    if( is_armed() && weapon.is_gun() ) {
        recoil += std::max( weapon.volume() / 250_ml - get_skill_level( skill_dodge ), 0 ) * rng( 0, 100 );
        recoil = std::min( MAX_RECOIL, recoil );
    }

    // Even if we are not to train still call practice to prevent skill rust
    difficulty = std::max( difficulty, 0.0f );
    practice( skill_dodge, difficulty * 2, difficulty );

    martial_arts_data->ma_ondodge_effects( *this );

    // For adjacent attackers check for techniques usable upon successful dodge
    if( source && square_dist( pos(), source->pos() ) == 1 ) {
        matec_id tec = pick_technique( *source, used_weapon(), false, true, false );

        if( tec != tec_none && !is_dead_state() ) {
            if( get_stamina() < get_stamina_max() / 3 ) {
                add_msg( m_bad, _( "You try to counterattack, but you are too exhausted!" ) );
            } else {
                melee_attack( *source, false, tec );
            }
        }
    }
}

float Character::get_melee() const
{
    return get_skill_level( skill_id( "melee" ) );
}

bool Character::uncanny_dodge()
{

    bool is_u = is_avatar();
    bool seen = get_player_view().sees( *this );

    const bool can_dodge_bio = get_power_level() >= 75_kJ && has_active_bionic( bio_uncanny_dodge );
    const bool can_dodge_mut = get_stamina() >= 300 && has_trait_flag( json_flag_UNCANNY_DODGE );
    const bool can_dodge_both = get_power_level() >= 37500_J &&
                                has_active_bionic( bio_uncanny_dodge ) &&
                                get_stamina() >= 150 && has_trait_flag( json_flag_UNCANNY_DODGE );

    if( !( can_dodge_bio || can_dodge_mut || can_dodge_both ) ) {
        return false;
    }
    tripoint adjacent = adjacent_tile();

    if( can_dodge_both ) {
        mod_power_level( -37500_J );
        mod_stamina( -150 );
    } else if( can_dodge_bio ) {
        mod_power_level( -75_kJ );
    } else if( can_dodge_mut ) {
        mod_stamina( -300 );
    }

    map &here = get_map();
    const optional_vpart_position veh_part = here.veh_at( position );
    if( in_vehicle && veh_part && veh_part.avail_part_with_feature( "SEATBELT" ) ) {
        return false;
    }

    //uncanny dodge triggered in car and wasn't secured by seatbelt
    if( in_vehicle && veh_part ) {
        here.unboard_vehicle( pos() );
    }
    if( adjacent.x != posx() || adjacent.y != posy() ) {
        position.x = adjacent.x;
        position.y = adjacent.y;

        //landed in a vehicle tile
        if( here.veh_at( position ) ) {
            here.board_vehicle( pos(), this );
        }

        if( is_u ) {
            add_msg( _( "Time seems to slow down, and you instinctively dodge!" ) );
        } else if( seen ) {
            add_msg( _( "%s dodges… so fast!" ), this->disp_name() );

        }
        return true;
    }
    if( is_u ) {
        add_msg( _( "You try to dodge, but there's no room!" ) );
    } else if( seen ) {
        add_msg( _( "%s tries to dodge, but there's no room!" ), this->disp_name() );
    }
    return false;
}

void Character::handle_skill_warning( const skill_id &id, bool force_warning )
{
    //remind the player intermittently that no skill gain takes place
    if( is_player() && ( force_warning || one_in( 5 ) ) ) {
        SkillLevel &level = get_skill_level_object( id );

        const Skill &skill = id.obj();
        std::string skill_name = skill.name();
        int curLevel = level.level();

        add_msg( m_info, _( "This task is too simple to train your %s beyond %d." ),
                 skill_name, curLevel );
    }
}

/** Returns true if the character has two functioning arms */
bool Character::has_two_arms() const
{
    return get_working_arm_count() >= 2;
}

// working is defined here as not disabled which means arms can be not broken
// and still not count if they have low enough hitpoints
int Character::get_working_arm_count() const
{
    if( has_active_mutation( trait_SHELL2 ) ) {
        return 0;
    }

    int limb_count = 0;
    if( !is_limb_disabled( body_part_arm_l ) ) {
        limb_count++;
    }
    if( !is_limb_disabled( body_part_arm_r ) ) {
        limb_count++;
    }
    if( has_bionic( bio_blaster ) && limb_count > 0 ) {
        limb_count--;
    }

    return limb_count;
}

// working is defined here as not broken
int Character::get_working_leg_count() const
{
    int limb_count = 0;
    if( !is_limb_broken( body_part_leg_l ) ) {
        limb_count++;
    }
    if( !is_limb_broken( body_part_leg_r ) ) {
        limb_count++;
    }
    return limb_count;
}

bool Character::is_limb_disabled( const bodypart_id &limb ) const
{
    return get_part_hp_cur( limb ) <= get_part_hp_max( limb ) * .125;
}

// this is the source of truth on if a limb is broken so all code to determine
// if a limb is broken should point here to make any future changes to breaking easier
bool Character::is_limb_broken( const bodypart_id &limb ) const
{
    return get_part_hp_cur( limb ) == 0;
}

bool Character::can_run() const
{
    return get_stamina() > 0 && !has_effect( effect_winded ) && get_working_leg_count() >= 2;
}

void Character::try_remove_downed()
{

    /** @EFFECT_DEX increases chance to stand up when knocked down */

    /** @EFFECT_STR increases chance to stand up when knocked down, slightly */
    if( rng( 0, 40 ) > get_dex() + get_str() / 2 ) {
        add_msg_if_player( _( "You struggle to stand." ) );
    } else {
        add_msg_player_or_npc( m_good, _( "You stand up." ),
                               _( "<npcname> stands up." ) );
        remove_effect( effect_downed );
    }
}

void Character::try_remove_bear_trap()
{
    /* Real bear traps can't be removed without the proper tools or immense strength; eventually this should
       allow normal players two options: removal of the limb or removal of the trap from the ground
       (at which point the player could later remove it from the leg with the right tools).
       As such we are currently making it a bit easier for players and NPC's to get out of bear traps.
    */
    /** @EFFECT_STR increases chance to escape bear trap */
    // If is riding, then despite the character having the effect, it is the mounted creature that escapes.
    map &here = get_map();
    if( is_player() && is_mounted() ) {
        auto *mon = mounted_creature.get();
        if( mon->type->melee_dice * mon->type->melee_sides >= 18 ) {
            if( x_in_y( mon->type->melee_dice * mon->type->melee_sides, 200 ) ) {
                mon->remove_effect( effect_beartrap );
                remove_effect( effect_beartrap );
                here.spawn_item( pos(), itype_beartrap );
                add_msg( _( "The %s escapes the bear trap!" ), mon->get_name() );
            } else {
                add_msg_if_player( m_bad,
                                   _( "Your %s tries to free itself from the bear trap, but can't get loose!" ), mon->get_name() );
            }
        }
    } else {
        if( x_in_y( get_str(), 100 ) ) {
            remove_effect( effect_beartrap );
            add_msg_player_or_npc( m_good, _( "You free yourself from the bear trap!" ),
                                   _( "<npcname> frees themselves from the bear trap!" ) );
            item beartrap( "beartrap", calendar::turn );
            here.add_item_or_charges( pos(), beartrap );
        } else {
            add_msg_if_player( m_bad,
                               _( "You try to free yourself from the bear trap, but can't get loose!" ) );
        }
    }
}

void Character::try_remove_lightsnare()
{
    map &here = get_map();
    if( is_mounted() ) {
        auto *mon = mounted_creature.get();
        if( x_in_y( mon->type->melee_dice * mon->type->melee_sides, 12 ) ) {
            mon->remove_effect( effect_lightsnare );
            remove_effect( effect_lightsnare );
            here.spawn_item( pos(), itype_string_36 );
            here.spawn_item( pos(), itype_snare_trigger );
            add_msg( _( "The %s escapes the light snare!" ), mon->get_name() );
        }
    } else {
        /** @EFFECT_STR increases chance to escape light snare */

        /** @EFFECT_DEX increases chance to escape light snare */
        if( x_in_y( get_str(), 12 ) || x_in_y( get_dex(), 8 ) ) {
            remove_effect( effect_lightsnare );
            add_msg_player_or_npc( m_good, _( "You free yourself from the light snare!" ),
                                   _( "<npcname> frees themselves from the light snare!" ) );
            item string( "string_36", calendar::turn );
            item snare( "snare_trigger", calendar::turn );
            here.add_item_or_charges( pos(), string );
            here.add_item_or_charges( pos(), snare );
        } else {
            add_msg_if_player( m_bad,
                               _( "You try to free yourself from the light snare, but can't get loose!" ) );
        }
    }
}

void Character::try_remove_heavysnare()
{
    map &here = get_map();
    if( is_mounted() ) {
        auto *mon = mounted_creature.get();
        if( mon->type->melee_dice * mon->type->melee_sides >= 7 ) {
            if( x_in_y( mon->type->melee_dice * mon->type->melee_sides, 32 ) ) {
                mon->remove_effect( effect_heavysnare );
                remove_effect( effect_heavysnare );
                here.spawn_item( pos(), itype_rope_6 );
                here.spawn_item( pos(), itype_snare_trigger );
                add_msg( _( "The %s escapes the heavy snare!" ), mon->get_name() );
            }
        }
    } else {
        /** @EFFECT_STR increases chance to escape heavy snare, slightly */

        /** @EFFECT_DEX increases chance to escape light snare */
        if( x_in_y( get_str(), 32 ) || x_in_y( get_dex(), 16 ) ) {
            remove_effect( effect_heavysnare );
            add_msg_player_or_npc( m_good, _( "You free yourself from the heavy snare!" ),
                                   _( "<npcname> frees themselves from the heavy snare!" ) );
            item rope( "rope_6", calendar::turn );
            item snare( "snare_trigger", calendar::turn );
            here.add_item_or_charges( pos(), rope );
            here.add_item_or_charges( pos(), snare );
        } else {
            add_msg_if_player( m_bad,
                               _( "You try to free yourself from the heavy snare, but can't get loose!" ) );
        }
    }
}

void Character::try_remove_crushed()
{
    /** @EFFECT_STR increases chance to escape crushing rubble */

    /** @EFFECT_DEX increases chance to escape crushing rubble, slightly */
    if( x_in_y( get_str() + get_dex() / 4.0, 100 ) ) {
        remove_effect( effect_crushed );
        add_msg_player_or_npc( m_good, _( "You free yourself from the rubble!" ),
                               _( "<npcname> frees themselves from the rubble!" ) );
    } else {
        add_msg_if_player( m_bad, _( "You try to free yourself from the rubble, but can't get loose!" ) );
    }
}

bool Character::try_remove_grab()
{
    int zed_number = 0;
    if( is_mounted() ) {
        auto *mon = mounted_creature.get();
        if( mon->has_effect( effect_grabbed ) ) {
            if( ( dice( mon->type->melee_dice + mon->type->melee_sides,
                        3 ) < get_effect_int( effect_grabbed ) ) ||
                !one_in( 4 ) ) {
                add_msg( m_bad, _( "Your %s tries to break free, but fails!" ), mon->get_name() );
                return false;
            } else {
                add_msg( m_good, _( "Your %s breaks free from the grab!" ), mon->get_name() );
                remove_effect( effect_grabbed );
                mon->remove_effect( effect_grabbed );
            }
        } else {
            if( one_in( 4 ) ) {
                add_msg( m_bad, _( "You are pulled from your %s!" ), mon->get_name() );
                remove_effect( effect_grabbed );
                forced_dismount();
            }
        }
    } else {
        map &here = get_map();
        for( auto&& dest : here.points_in_radius( pos(), 1, 0 ) ) { // *NOPAD*
            const monster *const mon = g->critter_at<monster>( dest );
            if( mon && mon->has_effect( effect_grabbing ) ) {
                zed_number += mon->get_grab_strength();
            }
        }
        if( zed_number == 0 ) {
            add_msg_player_or_npc( m_good, _( "You find yourself no longer grabbed." ),
                                   _( "<npcname> finds themselves no longer grabbed." ) );
            remove_effect( effect_grabbed );

            /** @EFFECT_STR increases chance to escape grab */
        } else if( rng( 0, get_str() ) < rng( get_effect_int( effect_grabbed, body_part_torso ),
                                              8 ) ) {
            add_msg_player_or_npc( m_bad, _( "You try break out of the grab, but fail!" ),
                                   _( "<npcname> tries to break out of the grab, but fails!" ) );
            return false;
        } else {
            add_msg_player_or_npc( m_good, _( "You break out of the grab!" ),
                                   _( "<npcname> breaks out of the grab!" ) );
            remove_effect( effect_grabbed );
            for( auto&& dest : here.points_in_radius( pos(), 1, 0 ) ) { // *NOPAD*
                monster *mon = g->critter_at<monster>( dest );
                if( mon && mon->has_effect( effect_grabbing ) ) {
                    mon->remove_effect( effect_grabbing );
                }
            }
        }
    }
    return true;
}

void Character::try_remove_webs()
{
    if( is_mounted() ) {
        auto *mon = mounted_creature.get();
        if( x_in_y( mon->type->melee_dice * mon->type->melee_sides,
                    6 * get_effect_int( effect_webbed ) ) ) {
            add_msg( _( "The %s breaks free of the webs!" ), mon->get_name() );
            mon->remove_effect( effect_webbed );
            remove_effect( effect_webbed );
        }
        /** @EFFECT_STR increases chance to escape webs */
    } else if( x_in_y( get_str(), 6 * get_effect_int( effect_webbed ) ) ) {
        add_msg_player_or_npc( m_good, _( "You free yourself from the webs!" ),
                               _( "<npcname> frees themselves from the webs!" ) );
        remove_effect( effect_webbed );
    } else {
        add_msg_if_player( _( "You try to free yourself from the webs, but can't get loose!" ) );
    }
}

void Character::try_remove_impeding_effect()
{
    for( const effect &eff : get_effects_with_flag( flag_EFFECT_IMPEDING ) ) {
        const efftype_id &eff_id = eff.get_id();
        if( is_mounted() ) {
            auto *mon = mounted_creature.get();
            if( x_in_y( mon->type->melee_dice * mon->type->melee_sides,
                        6 * get_effect_int( eff_id ) ) ) {
                add_msg( _( "The %s breaks free!" ), mon->get_name() );
                mon->remove_effect( eff_id );
                remove_effect( eff_id );
            }
            /** @EFFECT_STR increases chance to escape webs */
        } else if( x_in_y( get_str(), 6 * get_effect_int( eff_id ) ) ) {
            add_msg_player_or_npc( m_good, _( "You free yourself!" ),
                                   _( "<npcname> frees themselves!" ) );
            remove_effect( eff_id );
        } else {
            add_msg_if_player( _( "You try to free yourself, but can't!" ) );
        }
    }
}

bool Character::move_effects( bool attacking )
{
    if( has_effect( effect_downed ) ) {
        try_remove_downed();
        return false;
    }
    if( has_effect( effect_webbed ) ) {
        try_remove_webs();
        return false;
    }
    if( has_effect( effect_lightsnare ) ) {
        try_remove_lightsnare();
        return false;

    }
    if( has_effect( effect_heavysnare ) ) {
        try_remove_heavysnare();
        return false;
    }
    if( has_effect( effect_beartrap ) ) {
        try_remove_bear_trap();
        return false;
    }
    if( has_effect( effect_crushed ) ) {
        try_remove_crushed();
        return false;
    }
    if( has_effect_with_flag( flag_EFFECT_IMPEDING ) ) {
        try_remove_impeding_effect();
        return false;
    }

    // Below this point are things that allow for movement if they succeed

    // Currently we only have one thing that forces movement if you succeed, should we get more
    // than this will need to be reworked to only have success effects if /all/ checks succeed
    if( has_effect( effect_in_pit ) ) {
        /** @EFFECT_STR increases chance to escape pit */

        /** @EFFECT_DEX increases chance to escape pit, slightly */
        if( rng( 0, 40 ) > get_str() + get_dex() / 2 ) {
            add_msg_if_player( m_bad, _( "You try to escape the pit, but slip back in." ) );
            return false;
        } else {
            add_msg_player_or_npc( m_good, _( "You escape the pit!" ),
                                   _( "<npcname> escapes the pit!" ) );
            remove_effect( effect_in_pit );
        }
    }
    return !has_effect( effect_grabbed ) || attacking || try_remove_grab();
}

void Character::wait_effects( bool attacking )
{
    if( has_effect( effect_downed ) ) {
        try_remove_downed();
        return;
    }
    if( has_effect( effect_beartrap ) ) {
        try_remove_bear_trap();
        return;
    }
    if( has_effect( effect_lightsnare ) ) {
        try_remove_lightsnare();
        return;
    }
    if( has_effect( effect_heavysnare ) ) {
        try_remove_heavysnare();
        return;
    }
    if( has_effect( effect_webbed ) ) {
        try_remove_webs();
        return;
    }
    if( has_effect_with_flag( flag_EFFECT_IMPEDING ) ) {
        try_remove_impeding_effect();
        return;
    }
    if( has_effect( effect_grabbed ) && !attacking && !try_remove_grab() ) {
        return;
    }
}

move_mode_id Character::current_movement_mode() const
{
    return move_mode;
}

bool Character::movement_mode_is( const move_mode_id &mode ) const
{
    return move_mode == mode;
}

bool Character::is_running() const
{
    return move_mode->type() == move_mode_type::RUNNING;
}

bool Character::is_walking() const
{
    return move_mode->type() == move_mode_type::WALKING;
}

bool Character::is_crouching() const
{
    return move_mode->type() == move_mode_type::CROUCHING;
}

steed_type Character::get_steed_type() const
{
    steed_type steed;
    if( is_mounted() ) {
        if( mounted_creature->has_flag( MF_RIDEABLE_MECH ) ) {
            steed = steed_type::MECH;
        } else {
            steed = steed_type::ANIMAL;
        }
    } else {
        steed = steed_type::NONE;
    }
    return steed;
}

bool Character::can_switch_to( const move_mode_id &mode ) const
{
    // Only running modes are restricted at the moment and only when its your legs doing the running
    return get_steed_type() != steed_type::NONE || mode->type() != move_mode_type::RUNNING || can_run();
}

void Character::expose_to_disease( const diseasetype_id &dis_type )
{
    const cata::optional<int> &healt_thresh = dis_type->health_threshold;
    if( healt_thresh && healt_thresh.value() < get_healthy() ) {
        return;
    }
    const std::set<bodypart_str_id> &bps = dis_type->affected_bodyparts;
    if( !bps.empty() ) {
        for( const bodypart_str_id &bp : bps ) {
            add_effect( dis_type->symptoms, rng( dis_type->min_duration, dis_type->max_duration ), bp.id(),
                        false,
                        rng( dis_type->min_intensity, dis_type->max_intensity ) );
        }
    } else {
        add_effect( dis_type->symptoms, rng( dis_type->min_duration, dis_type->max_duration ),
                    bodypart_str_id::NULL_ID(),
                    false,
                    rng( dis_type->min_intensity, dis_type->max_intensity ) );
    }
}

void Character::process_turn()
{
    migrate_items_to_storage( false );

    for( bionic &i : *my_bionics ) {
        if( i.incapacitated_time > 0_turns ) {
            i.incapacitated_time -= 1_turns;
            if( i.incapacitated_time == 0_turns ) {
                add_msg_if_player( m_bad, _( "Your %s bionic comes back online." ), i.info().name );
            }
        }
    }

    for( const trait_id &mut : get_mutations() ) {
        mutation_reflex_trigger( mut );
    }

    Creature::process_turn();
}

void Character::recalc_hp()
{
    int str_boost_val = 0;
    cata::optional<skill_boost> str_boost = skill_boost::get( "str" );
    if( str_boost ) {
        int skill_total = 0;
        for( const std::string &skill_str : str_boost->skills() ) {
            skill_total += get_skill_level( skill_id( skill_str ) );
        }
        str_boost_val = str_boost->calc_bonus( skill_total );
    }
    // Mutated toughness stacks with starting, by design.
    float hp_mod = 1.0f + mutation_value( "hp_modifier" ) + mutation_value( "hp_modifier_secondary" );
    float hp_adjustment = mutation_value( "hp_adjustment" ) + ( str_boost_val * 3 );
    calc_all_parts_hp( hp_mod, hp_adjustment, get_str_base(), get_dex_base(), get_per_base(),
                       get_int_base(), get_healthy(), get_fat_to_hp() );
}

int Character::get_part_hp_max( const bodypart_id &id ) const
{
    return enchantment_cache->modify_value( enchant_vals::mod::MAX_HP,
                                            Creature::get_part_hp_max( id ) );
}

void Character::update_body_wetness( const w_point &weather )
{
    // Average number of turns to go from completely soaked to fully dry
    // assuming average temperature and humidity
    constexpr time_duration average_drying = 2_hours;

    // A modifier on drying time
    double delay = 1.0;
    // Weather slows down drying
    delay += ( ( weather.humidity - 66 ) - ( weather.temperature - 65 ) ) / 100;
    delay = std::max( 0.1, delay );
    // Fur/slime retains moisture
    if( has_trait( trait_LIGHTFUR ) || has_trait( trait_FUR ) || has_trait( trait_FELINE_FUR ) ||
        has_trait( trait_LUPINE_FUR ) || has_trait( trait_CHITIN_FUR ) || has_trait( trait_CHITIN_FUR2 ) ||
        has_trait( trait_CHITIN_FUR3 ) ) {
        delay = delay * 6 / 5;
    }
    if( has_trait( trait_URSINE_FUR ) || has_trait( trait_SLIMY ) ) {
        delay *= 1.5;
    }

    if( !x_in_y( 1, to_turns<int>( average_drying * delay / 100.0 ) ) ) {
        // No drying this turn
        return;
    }

    // Now per-body-part stuff
    // To make drying uniform, make just one roll and reuse it
    const int drying_roll = rng( 1, 80 );

    for( const bodypart_id &bp : get_all_body_parts() ) {
        if( get_part_wetness( bp ) == 0 ) {
            continue;
        }
        // This is to normalize drying times
        int drying_chance = get_part_drench_capacity( bp );
        const int temp_conv = get_part_temp_conv( bp );
        // Body temperature affects duration of wetness
        // Note: Using temp_conv rather than temp_cur, to better approximate environment
        if( temp_conv >= BODYTEMP_SCORCHING ) {
            drying_chance *= 2;
        } else if( temp_conv >= BODYTEMP_VERY_HOT ) {
            drying_chance = drying_chance * 3 / 2;
        } else if( temp_conv >= BODYTEMP_HOT ) {
            drying_chance = drying_chance * 4 / 3;
        } else if( temp_conv > BODYTEMP_COLD ) {
            // Comfortable, doesn't need any changes
        } else {
            // Evaporation doesn't change that much at lower temp
            drying_chance = drying_chance * 3 / 4;
        }

        if( drying_chance < 1 ) {
            drying_chance = 1;
        }

        // TODO: Make evaporation reduce body heat
        if( drying_chance >= drying_roll ) {
            mod_part_wetness( bp, -1 );
            if( get_part_wetness( bp ) < 0 ) {
                set_part_wetness( bp, 0 );
            }
        }
        // Safety measure to keep wetness within bounds
        if( get_part_wetness( bp ) > get_part_drench_capacity( bp ) ) {
            set_part_wetness( bp, get_part_drench_capacity( bp ) );
        }
    }
    // TODO: Make clothing slow down drying
}

// This must be called when any of the following change:
// - effects
// - bionics
// - traits
// - underwater
// - clothes
// With the exception of clothes, all changes to these character attributes must
// occur through a function in this class which calls this function. Clothes are
// typically added/removed with wear() and takeoff(), but direct access to the
// 'wears' vector is still allowed due to refactor exhaustion.
void Character::recalc_sight_limits()
{
    sight_max = 9999;
    vision_mode_cache.reset();
    const bool in_light = get_map().ambient_light_at( pos() ) > LIGHT_AMBIENT_LIT;

    // Set sight_max.
    if( is_blind() || ( in_sleep_state() && !has_trait( trait_SEESLEEP ) && is_player() ) ||
        has_effect( effect_narcosis ) ) {
        sight_max = 0;
    } else if( has_effect( effect_boomered ) && ( !( has_trait( trait_PER_SLIME_OK ) ) ) ) {
        sight_max = 1;
        vision_mode_cache.set( BOOMERED );
    } else if( has_effect( effect_in_pit ) || has_effect( effect_no_sight ) ||
               ( underwater && !has_flag( json_flag_EYE_MEMBRANE ) && !worn_with_flag( flag_SWIM_GOGGLES ) ) ) {
        sight_max = 1;
    } else if( has_active_mutation( trait_SHELL2 ) ) {
        // You can kinda see out a bit.
        sight_max = 2;
    } else if( ( has_trait( trait_MYOPIC ) || ( in_light && has_trait( trait_URSINE_EYE ) ) ) &&
               !worn_with_flag( flag_FIX_NEARSIGHT ) && !has_effect( effect_contacts ) ) {
        sight_max = 4;
    } else if( has_trait( trait_PER_SLIME ) ) {
        sight_max = 6;
    } else if( has_effect( effect_darkness ) ) {
        vision_mode_cache.set( DARKNESS );
        sight_max = 10;
    }

    sight_max = enchantment_cache->modify_value( enchant_vals::mod::SIGHT_RANGE, sight_max );

    // Debug-only NV, by vache's request
    if( has_trait( trait_DEBUG_NIGHTVISION ) ) {
        vision_mode_cache.set( DEBUG_NIGHTVISION );
    }
    if( has_nv() ) {
        vision_mode_cache.set( NV_GOGGLES );
    }
    if( has_active_mutation( trait_NIGHTVISION3 ) || is_wearing( itype_rm13_armor_on ) ||
        ( is_mounted() && mounted_creature->has_flag( MF_MECH_RECON_VISION ) ) ) {
        vision_mode_cache.set( NIGHTVISION_3 );
    }
    if( has_active_mutation( trait_ELFA_FNV ) ) {
        vision_mode_cache.set( FULL_ELFA_VISION );
    }
    if( has_active_mutation( trait_CEPH_VISION ) ) {
        vision_mode_cache.set( CEPH_VISION );
    }
    if( has_active_mutation( trait_ELFA_NV ) ) {
        vision_mode_cache.set( ELFA_VISION );
    }
    if( has_active_mutation( trait_NIGHTVISION2 ) ) {
        vision_mode_cache.set( NIGHTVISION_2 );
    }
    if( has_active_mutation( trait_FEL_NV ) ) {
        vision_mode_cache.set( FELINE_VISION );
    }
    if( has_active_mutation( trait_URSINE_EYE ) ) {
        vision_mode_cache.set( URSINE_VISION );
    }
    if( has_active_mutation( trait_NIGHTVISION ) ) {
        vision_mode_cache.set( NIGHTVISION_1 );
    }
    if( has_trait( trait_BIRD_EYE ) ) {
        vision_mode_cache.set( BIRD_EYE );
    }

    // Not exactly a sight limit thing, but related enough
    if( has_flag( json_flag_INFRARED ) ||
        worn_with_flag( flag_IR_EFFECT ) || ( is_mounted() &&
                mounted_creature->has_flag( MF_MECH_RECON_VISION ) ) ) {
        vision_mode_cache.set( IR_VISION );
    }

    if( has_trait_flag( json_flag_SUPER_CLAIRVOYANCE ) ) {
        vision_mode_cache.set( VISION_CLAIRVOYANCE_SUPER );
    } else if( has_trait_flag( json_flag_CLAIRVOYANCE_PLUS ) ) {
        vision_mode_cache.set( VISION_CLAIRVOYANCE_PLUS );
    } else if( has_trait_flag( json_flag_CLAIRVOYANCE ) ) {
        vision_mode_cache.set( VISION_CLAIRVOYANCE );
    }
}

static float threshold_for_range( float range )
{
    constexpr float epsilon = 0.01f;
    return LIGHT_AMBIENT_MINIMAL / std::exp( range * LIGHT_TRANSPARENCY_OPEN_AIR ) - epsilon;
}

float Character::get_vision_threshold( float light_level ) const
{
    if( vision_mode_cache[DEBUG_NIGHTVISION] ) {
        // Debug vision always works with absurdly little light.
        return 0.01;
    }

    // As light_level goes from LIGHT_AMBIENT_MINIMAL to LIGHT_AMBIENT_LIT,
    // dimming goes from 1.0 to 2.0.
    const float dimming_from_light = 1.0 + ( ( static_cast<float>( light_level ) -
                                     LIGHT_AMBIENT_MINIMAL ) /
                                     ( LIGHT_AMBIENT_LIT - LIGHT_AMBIENT_MINIMAL ) );

    int eyes_encumb = 0;
    if( has_part( body_part_eyes ) ) {
        eyes_encumb = encumb( body_part_eyes );
    }

    float range = get_per() / 3.0f - eyes_encumb / 10.0f;
    if( vision_mode_cache[NV_GOGGLES] || vision_mode_cache[NIGHTVISION_3] ||
        vision_mode_cache[FULL_ELFA_VISION] || vision_mode_cache[CEPH_VISION] ) {
        range += 10;
    } else if( vision_mode_cache[NIGHTVISION_2] || vision_mode_cache[FELINE_VISION] ||
               vision_mode_cache[URSINE_VISION] || vision_mode_cache[ELFA_VISION] ) {
        range += 4.5;
    } else if( vision_mode_cache[NIGHTVISION_1] ) {
        range += 2;
    }

    if( vision_mode_cache[BIRD_EYE] ) {
        range++;
    }

    // Clamp range to 1+, so that we can always see where we are
    range = std::max( 1.0f, range );

    return std::min( static_cast<float>( LIGHT_AMBIENT_LOW ),
                     threshold_for_range( range ) * dimming_from_light );
}

void Character::flag_encumbrance()
{
    check_encumbrance = true;
}

void Character::check_item_encumbrance_flag()
{
    bool update_required = check_encumbrance;
    for( auto &i : worn ) {
        if( !update_required && i.encumbrance_update_ ) {
            update_required = true;
        }
        i.encumbrance_update_ = false;
    }

    if( update_required ) {
        calc_encumbrance();
    }
}

bool Character::natural_attack_restricted_on( const bodypart_id &bp ) const
{
    for( const item &i : worn ) {
        if( i.covers( bp ) && !i.has_flag( flag_ALLOWS_NATURAL_ATTACKS ) &&
            !i.has_flag( flag_SEMITANGIBLE ) &&
            !i.has_flag( flag_PERSONAL ) && !i.has_flag( flag_AURA ) ) {
            return true;
        }
    }
    return false;
}

std::vector<bionic_id> Character::get_bionics() const
{
    std::vector<bionic_id> result;
    for( const bionic &b : *my_bionics ) {
        result.push_back( b.id );
    }
    return result;
}

bool Character::has_bionic( const bionic_id &b ) const
{
    for( const bionic_id &bid : get_bionics() ) {
        if( bid == b ) {
            return true;
        }
    }
    return false;
}

bool Character::has_active_bionic( const bionic_id &b ) const
{
    for( const bionic &i : *my_bionics ) {
        if( i.id == b ) {
            return ( i.powered && i.incapacitated_time == 0_turns );
        }
    }
    return false;
}

bool Character::has_any_bionic() const
{
    return !get_bionics().empty();
}

bionic_id Character::get_remote_fueled_bionic() const
{
    for( const bionic_id &bid : get_bionics() ) {
        if( bid->is_remote_fueled ) {
            return bid;
        }
    }
    return bionic_id();
}

bool Character::can_fuel_bionic_with( const item &it ) const
{
    if( ( !it.is_fuel() && !it.type->magazine && !it.flammable() ) || it.is_comestible() ) {
        return false;
    }

    for( const bionic_id &bid : get_bionics() ) {
        for( const material_id &fuel : bid->fuel_opts ) {
            if( fuel == it.get_base_material().id ) {
                return true;
            } else if( it.type->magazine && fuel == item( it.ammo_current() ).get_base_material().id ) {
                return true;
            }
        }

    }
    return false;
}

std::vector<bionic_id> Character::get_bionic_fueled_with( const item &it ) const
{
    std::vector<bionic_id> bionics;

    for( const bionic_id &bid : get_bionics() ) {
        for( const material_id &fuel : bid->fuel_opts ) {
            if( fuel == it.get_base_material().id ) {
                bionics.emplace_back( bid );
            } else if( it.type->magazine && fuel == item( it.ammo_current() ).get_base_material().id ) {
                bionics.emplace_back( bid );
            }
        }
    }

    return bionics;
}

std::vector<bionic_id> Character::get_bionic_fueled_with( const material_id &mat ) const
{
    std::vector<bionic_id> bionics;

    for( const bionic_id &bid : get_bionics() ) {
        for( const material_id &fuel : bid->fuel_opts ) {
            if( fuel == mat ) {
                bionics.emplace_back( bid );
            }
        }
    }

    return bionics;
}

std::vector<bionic_id> Character::get_fueled_bionics() const
{
    std::vector<bionic_id> bionics;
    for( const bionic_id &bid : get_bionics() ) {
        if( !bid->fuel_opts.empty() ) {
            bionics.emplace_back( bid );
        }
    }
    return bionics;
}

bionic_id Character::get_most_efficient_bionic( const std::vector<bionic_id> &bids ) const
{
    float temp_eff = 0.0f;
    bionic_id bio( "null" );
    for( const bionic_id &bid : bids ) {
        if( bid->fuel_efficiency > temp_eff ) {
            temp_eff = bid->fuel_efficiency;
            bio = bid;
        }
    }
    return bio;
}

void Character::practice( const skill_id &id, int amount, int cap, bool suppress_warning )
{
    static const int INTMAX_SQRT = std::floor( std::sqrt( std::numeric_limits<int>::max() ) );
    SkillLevel &level = get_skill_level_object( id );
    const Skill &skill = id.obj();
    if( !level.can_train() && !in_sleep_state() ) {
        // If leveling is disabled, don't train, don't drain focus, don't print anything
        // Leaving as a skill method rather than global for possible future skill cap setting
        return;
    }

    const auto highest_skill = [&]() {
        std::pair<skill_id, int> result( skill_id::NULL_ID(), -1 );
        for( const auto &pair : *_skills ) {
            const SkillLevel &lobj = pair.second;
            if( lobj.level() > result.second ) {
                result = std::make_pair( pair.first, lobj.level() );
            }
        }
        return result.first;
    };

    const bool isSavant = has_trait( trait_SAVANT );
    const skill_id savantSkill = isSavant ? highest_skill() : skill_id::NULL_ID();

    amount = adjust_for_focus( amount );

    if( has_trait( trait_PACIFIST ) && skill.is_combat_skill() ) {
        if( !one_in( 3 ) ) {
            amount = 0;
        }
    }
    if( has_trait_flag( json_flag_PRED2 ) && skill.is_combat_skill() ) {
        if( one_in( 3 ) ) {
            amount *= 2;
        }
    }
    if( has_trait_flag( json_flag_PRED3 ) && skill.is_combat_skill() ) {
        amount *= 2;
    }

    if( has_trait_flag( json_flag_PRED4 ) && skill.is_combat_skill() ) {
        amount *= 3;
    }

    if( isSavant && id != savantSkill ) {
        amount /= 2;
    }

    if( amount > 0 && get_skill_level( id ) > cap ) { //blunt grinding cap implementation for crafting
        amount = 0;
        if( !suppress_warning ) {
            handle_skill_warning( id, false );
        }
    }
    if( amount > 0 && level.isTraining() ) {
        int oldLevel = get_skill_level( id );
        get_skill_level_object( id ).train( amount );
        int newLevel = get_skill_level( id );
        std::string skill_name = skill.name();
        if( newLevel > oldLevel ) {
            get_event_bus().send<event_type::gains_skill_level>( getID(), id, newLevel );
        }
        if( is_player() && newLevel > oldLevel ) {
            add_msg( m_good, _( "Your skill in %s has increased to %d!" ), skill_name, newLevel );
        }
        if( is_player() && newLevel > cap ) {
            //inform player immediately that the current recipe can't be used to train further
            add_msg( m_info, _( "You feel that %s tasks of this level are becoming trivial." ),
                     skill_name );
        }

        // Apex Predators don't think about much other than killing.
        // They don't lose Focus when practicing combat skills.
        if( !( has_trait_flag( json_flag_PRED4 ) && skill.is_combat_skill() ) ) {
            // Base reduction on the larger of 1% of total, or practice amount.
            // The latter kicks in when long actions like crafting
            // apply many turns of gains at once.
            int focus_drain = std::max( focus_pool / 100, amount );
            // For large values of amount, amount^2 can exceed INT_MAX.
            // We're going to be draining all of the focus if it gets that large, so cap it at a safe value
            focus_drain = std::min( focus_drain, INTMAX_SQRT );
            // The purpose of having this squared is that it makes focus drain dramatically slower
            // as it approaches zero.
            focus_pool -= ( focus_drain * focus_drain ) / 1000;
        }
        focus_pool = std::max( focus_pool, 0 );
    }

    get_skill_level_object( id ).practice();
}

// Returned values range from 1.0 (unimpeded vision) to 11.0 (totally blind).
//  1.0 is LIGHT_AMBIENT_LIT or brighter
//  4.0 is a dark clear night, barely bright enough for reading and crafting
//  6.0 is LIGHT_AMBIENT_DIM
//  7.3 is LIGHT_AMBIENT_MINIMAL, a dark cloudy night, unlit indoors
// 11.0 is zero light or blindness
float Character::fine_detail_vision_mod( const tripoint &p ) const
{
    // PER_SLIME_OK implies you can get enough eyes around the bile
    // that you can generally see.  There still will be the haze, but
    // it's annoying rather than limiting.
    if( is_blind() ||
        ( ( has_effect( effect_boomered ) || has_effect( effect_darkness ) ) &&
          !has_trait( trait_PER_SLIME_OK ) ) ) {
        return 11.0;
    }
    // Scale linearly as light level approaches LIGHT_AMBIENT_LIT.
    // If we're actually a source of light, assume we can direct it where we need it.
    // Therefore give a hefty bonus relative to ambient light.
    float own_light = std::max( 1.0f, LIGHT_AMBIENT_LIT - active_light() - 2.0f );

    // Same calculation as above, but with a result 3 lower.
    float ambient_light = std::max( 1.0f,
                                    LIGHT_AMBIENT_LIT - get_map().ambient_light_at( p == tripoint_zero ? pos() : p ) + 1.0f );

    return std::min( own_light, ambient_light );
}

units::energy Character::get_power_level() const
{
    return power_level;
}

units::energy Character::get_max_power_level() const
{
    return enchantment_cache->modify_value( enchant_vals::mod::BIONIC_POWER, max_power_level );
}

void Character::set_power_level( const units::energy &npower )
{
    power_level = std::min( npower, get_max_power_level() );
}

void Character::set_max_power_level( const units::energy &npower_max )
{
    max_power_level = npower_max;
}

void Character::mod_power_level( const units::energy &npower )
{
    // units::energy is an int, so avoid overflow by converting it to a int64_t, then adding them
    // If the result is greater than the max power level, set power to max
    int64_t power = static_cast<int64_t>( units::to_millijoule( get_power_level() ) ) +
                    static_cast<int64_t>( units::to_millijoule( npower ) );
    units::energy new_power;
    if( power > units::to_millijoule( get_max_power_level() ) ) {
        new_power = get_max_power_level();
    } else {
        new_power = get_power_level() + npower;
    }
    set_power_level( clamp( new_power, 0_kJ, get_max_power_level() ) );
}

void Character::mod_max_power_level( const units::energy &npower_max )
{
    max_power_level += npower_max;
}

bool Character::is_max_power() const
{
    return get_power_level() >= get_max_power_level();
}

bool Character::has_power() const
{
    return get_power_level() > 0_kJ;
}

bool Character::has_max_power() const
{
    return get_max_power_level() > 0_kJ;
}

bool Character::enough_power_for( const bionic_id &bid ) const
{
    return get_power_level() >= bid->power_activate;
}

void Character::conduct_blood_analysis()
{
    std::vector<std::string> effect_descriptions;
    std::vector<nc_color> colors;

    for( auto &elem : *effects ) {
        if( elem.first->get_blood_analysis_description().empty() ) {
            continue;
        }
        effect_descriptions.emplace_back( elem.first->get_blood_analysis_description() );
        colors.emplace_back( elem.first->get_rating() == e_good ? c_green : c_red );
    }

    const int win_w = 46;
    size_t win_h = 0;
    catacurses::window w;
    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        win_h = std::min( static_cast<size_t>( TERMY ),
                          std::max<size_t>( 1, effect_descriptions.size() ) + 2 );
        w = catacurses::newwin( win_h, win_w,
                                point( ( TERMX - win_w ) / 2, ( TERMY - win_h ) / 2 ) );
        ui.position_from_window( w );
    } );
    ui.mark_resize();
    ui.on_redraw( [&]( const ui_adaptor & ) {
        draw_border( w, c_red, string_format( " %s ", _( "Blood Test Results" ) ) );
        if( effect_descriptions.empty() ) {
            trim_and_print( w, point( 2, 1 ), win_w - 3, c_white, _( "No effects." ) );
        } else {
            for( size_t line = 1; line < ( win_h - 1 ) && line <= effect_descriptions.size(); ++line ) {
                trim_and_print( w, point( 2, line ), win_w - 3, colors[line - 1], effect_descriptions[line - 1] );
            }
        }
        wnoutrefresh( w );
    } );
    input_context ctxt( "BLOOD_TEST_RESULTS" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    bool stop = false;
    // Display new messages
    g->invalidate_main_ui_adaptor();
    while( !stop ) {
        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        if( action == "CONFIRM" || action == "QUIT" ) {
            stop = true;
        }
    }
}

std::vector<material_id> Character::get_fuel_available( const bionic_id &bio ) const
{
    std::vector<material_id> stored_fuels;
    for( const material_id &fuel : bio->fuel_opts ) {
        if( !get_value( fuel.str() ).empty() || fuel->get_fuel_data().is_perpetual_fuel ) {
            stored_fuels.emplace_back( fuel );
        }
    }
    return stored_fuels;
}

int Character::get_fuel_capacity( const material_id &fuel ) const
{
    int amount_stored = 0;
    if( !get_value( fuel.str() ).empty() ) {
        amount_stored = std::stoi( get_value( fuel.str() ) );
    }
    int capacity = 0;
    for( const bionic_id &bid : get_bionics() ) {
        for( const material_id &fl : bid->fuel_opts ) {
            if( get_value( bid.str() ).empty() || get_value( bid.str() ) == fl.str() ) {
                if( fl == fuel ) {
                    capacity += bid->fuel_capacity;
                }
            }
        }
    }
    return capacity - amount_stored;
}

int Character::get_total_fuel_capacity( const material_id &fuel ) const
{
    int capacity = 0;
    for( const bionic_id &bid : get_bionics() ) {
        for( const material_id &fl : bid->fuel_opts ) {
            if( get_value( bid.str() ).empty() || get_value( bid.str() ) == fl.str() ) {
                if( fl == fuel ) {
                    capacity += bid->fuel_capacity;
                }
            }
        }
    }
    return capacity;
}

void Character::update_fuel_storage( const material_id &fuel )
{

    if( get_value( fuel.str() ).empty() ) {
        for( const bionic_id &bid : get_bionic_fueled_with( fuel ) ) {
            remove_value( bid.c_str() );
        }
        return;
    }

    std::vector<bionic_id> bids = get_bionic_fueled_with( fuel );
    if( bids.empty() ) {
        return;
    }
    int amount_fuel_loaded = std::stoi( get_value( fuel.str() ) );
    std::vector<bionic_id> loaded_bio;

    // Sort bionic in order of decreasing capacity
    // To fill the bigger ones firts.
    bool swap = true;
    while( swap ) {
        swap = false;
        for( size_t i = 0; i < bids.size() - 1; i++ ) {
            if( bids[i + 1]->fuel_capacity > bids[i]->fuel_capacity ) {
                std::swap( bids[i + 1], bids[i] );
                swap = true;
            }
        }
    }

    for( const bionic_id &bid : bids ) {
        remove_value( bid.c_str() );
        if( bid->fuel_capacity <= amount_fuel_loaded ) {
            amount_fuel_loaded -= bid->fuel_capacity;
            loaded_bio.emplace_back( bid );
        } else if( amount_fuel_loaded != 0 ) {
            loaded_bio.emplace_back( bid );
            break;
        }
    }

    for( const bionic_id &bd : loaded_bio ) {
        set_value( bd.str(), fuel.str() );
    }

}

int Character::get_mod_stat_from_bionic( const character_stat &Stat ) const
{
    int ret = 0;
    for( const bionic_id &bid : get_bionics() ) {
        const auto St_bn = bid->stat_bonus.find( Stat );
        if( St_bn != bid->stat_bonus.end() ) {
            ret += St_bn->second;
        }
    }
    return ret;
}

int Character::get_standard_stamina_cost( const item *thrown_item ) const
{
    // Previously calculated as 2_gram * std::max( 1, str_cur )
    // using 16_gram normalizes it to 8 str. Same effort expenditure
    // for each strike, regardless of weight. This is compensated
    // for by the additional move cost as weapon weight increases
    //If the item is thrown, override with the thrown item instead.
    const int weight_cost = ( thrown_item == nullptr ) ? this->weapon.weight() /
                            ( 16_gram ) : thrown_item->weight() / ( 16_gram );
    const int encumbrance_cost = this->encumb( body_part_arm_l ) + this->encumb(
                                     body_part_arm_r );
    return ( weight_cost + encumbrance_cost + 50 ) * -1;
}

cata::optional<std::list<item>::iterator> Character::wear_item( const item &to_wear,
        bool interactive, bool do_calc_encumbrance )
{
    invalidate_inventory_validity_cache();
    const auto ret = can_wear( to_wear );
    if( !ret.success() ) {
        if( interactive ) {
            add_msg_if_player( m_info, "%s", ret.c_str() );
        }
        return cata::nullopt;
    }

    const bool was_deaf = is_deaf();
    const bool supertinymouse = get_size() == creature_size::tiny;
    last_item = to_wear.typeId();

    std::list<item>::iterator position = position_to_wear_new_item( to_wear );
    std::list<item>::iterator new_item_it = worn.insert( position, to_wear );

    get_event_bus().send<event_type::character_wears_item>( getID(), last_item );

    if( interactive ) {
        add_msg_player_or_npc(
            _( "You put on your %s." ),
            _( "<npcname> puts on their %s." ),
            to_wear.tname() );
        moves -= item_wear_cost( to_wear );

        for( const bodypart_id &bp : get_all_body_parts() ) {
            if( to_wear.covers( bp ) && encumb( bp ) >= 40 ) {
                add_msg_if_player( m_warning,
                                   bp == body_part_eyes ?
                                   _( "Your %s are very encumbered!  %s" ) : _( "Your %s is very encumbered!  %s" ),
                                   body_part_name( bp ), encumb_text( bp ) );
            }
        }
        if( !was_deaf && is_deaf() ) {
            add_msg_if_player( m_info, _( "You're deafened!" ) );
        }
        if( supertinymouse && !to_wear.has_flag( flag_UNDERSIZE ) ) {
            add_msg_if_player( m_warning,
                               _( "This %s is too big to wear comfortably!  Maybe it could be refitted." ),
                               to_wear.tname() );
        } else if( !supertinymouse && to_wear.has_flag( flag_UNDERSIZE ) ) {
            add_msg_if_player( m_warning,
                               _( "This %s is too small to wear comfortably!  Maybe it could be refitted." ),
                               to_wear.tname() );
        }
    } else if( is_npc() && get_player_view().sees( *this ) ) {
        add_msg_if_npc( _( "<npcname> puts on their %s." ), to_wear.tname() );
    }

    new_item_it->on_wear( *this );

    inv->update_invlet( *new_item_it );
    inv->update_cache_with_item( *new_item_it );

    if( do_calc_encumbrance ) {
        recalc_sight_limits();
        calc_encumbrance();
    }

    return new_item_it;
}

int Character::amount_worn( const itype_id &id ) const
{
    int amount = 0;
    for( const item &elem : worn ) {
        if( elem.typeId() == id ) {
            ++amount;
        }
    }
    return amount;
}

int Character::count_softwares( const itype_id &id )
{
    int count = 0;
    for( const item_location &it_loc : all_items_loc() ) {
        if( it_loc->is_software_storage() ) {
            for( const item *soft : it_loc->softwares() ) {
                if( soft->typeId() == id ) {
                    count++;
                }
            }
        }
    }
    return count;
}

std::vector<item_location> Character::nearby( const
        std::function<bool( const item *, const item * )> &func, int radius ) const
{
    std::vector<item_location> res;

    visit_items( [&]( const item * e, const item * parent ) {
        if( func( e, parent ) ) {
            res.emplace_back( const_cast<Character &>( *this ), const_cast<item *>( e ) );
        }
        return VisitResponse::NEXT;
    } );

    for( const auto &cur : map_selector( pos(), radius ) ) {
        cur.visit_items( [&]( const item * e, const item * parent ) {
            if( func( e, parent ) ) {
                res.emplace_back( cur, const_cast<item *>( e ) );
            }
            return VisitResponse::NEXT;
        } );
    }

    for( const auto &cur : vehicle_selector( pos(), radius ) ) {
        cur.visit_items( [&]( const item * e, const item * parent ) {
            if( func( e, parent ) ) {
                res.emplace_back( cur, const_cast<item *>( e ) );
            }
            return VisitResponse::NEXT;
        } );
    }

    return res;
}

units::length Character::max_single_item_length() const
{
    units::length ret = weapon.max_containable_length();

    for( const item &worn_it : worn ) {
        units::length candidate = worn_it.max_containable_length();
        if( candidate > ret ) {
            ret = candidate;
        }
    }
    return ret;
}

units::volume Character::max_single_item_volume() const
{
    units::volume ret = weapon.max_containable_volume();

    for( const item &worn_it : worn ) {
        units::volume candidate = worn_it.max_containable_volume();
        if( candidate > ret ) {
            ret = candidate;
        }
    }
    return ret;
}

std::pair<item_location, item_pocket *> Character::best_pocket( const item &it, const item *avoid )
{
    item_location weapon_loc( *this, &weapon );
    std::pair<item_location, item_pocket *> ret = std::make_pair( item_location(), nullptr );
    if( &weapon != &it && &weapon != avoid ) {
        ret = weapon.best_pocket( it, weapon_loc );
    }
    for( item &worn_it : worn ) {
        if( &worn_it == &it || &worn_it == avoid ) {
            continue;
        }
        item_location loc( *this, &worn_it );
        std::pair<item_location, item_pocket *> internal_pocket = worn_it.best_pocket( it, loc );
        if( internal_pocket.second != nullptr &&
            ( ret.second == nullptr || ret.second->better_pocket( *internal_pocket.second, it ) ) ) {
            ret = internal_pocket;
        }
    }
    return ret;
}

item *Character::try_add( item it, const item *avoid, const bool allow_wield )
{
    invalidate_inventory_validity_cache();
    itype_id item_type_id = it.typeId();
    last_item = item_type_id;

    // if there's a desired invlet for this item type, try to use it
    bool keep_invlet = false;
    const invlets_bitset cur_inv = allocated_invlets();
    for( const auto &iter : inv->assigned_invlet ) {
        if( iter.second == item_type_id && !cur_inv[iter.first] ) {
            it.invlet = iter.first;
            keep_invlet = true;
            break;
        }
    }
    std::pair<item_location, item_pocket *> pocket = best_pocket( it, avoid );
    item *ret = nullptr;
    if( pocket.second == nullptr ) {
        if( !has_weapon() && allow_wield && wield( it ) ) {
            ret = &weapon;
        } else {
            return nullptr;
        }
    } else {
        // this will set ret to either it, or to stack where it was placed
        pocket.second->add( it, &ret );
        pocket.first.on_contents_changed();
        pocket.second->on_contents_changed();
    }

    if( keep_invlet ) {
        ret->invlet = it.invlet;
    }
    ret->on_pickup( *this );
    cached_info.erase( "reloadables" );
    return ret;
}

item &Character::i_add( item it, bool /* should_stack */, const item *avoid, const bool allow_drop,
                        const bool allow_wield )
{
    invalidate_inventory_validity_cache();
    item *added = try_add( it, avoid, /*allow_wield=*/allow_wield );
    if( added == nullptr ) {
        if( !allow_wield || !wield( it ) ) {
            if( allow_drop ) {
                return get_map().add_item_or_charges( pos(), it );
            } else {
                return null_item_reference();
            }
        } else {
            return weapon;
        }
    } else {
        return *added;
    }
}

std::list<item> Character::remove_worn_items_with( const std::function<bool( item & )> &filter )
{
    invalidate_inventory_validity_cache();
    std::list<item> result;
    for( auto iter = worn.begin(); iter != worn.end(); ) {
        if( filter( *iter ) ) {
            iter->on_takeoff( *this );
            result.splice( result.begin(), worn, iter++ );
        } else {
            ++iter;
        }
    }
    return result;
}

static void recur_internal_locations( item_location parent, std::vector<item_location> &list )
{
    for( item *it : parent->contents.all_items_top( item_pocket::pocket_type::CONTAINER ) ) {
        item_location child( parent, it );
        recur_internal_locations( child, list );
    }
    list.push_back( parent );
}

std::vector<item_location> Character::all_items_loc()
{
    std::vector<item_location> ret;
    item_location weap_loc( *this, &weapon );
    std::vector<item_location> weapon_internal_items;
    recur_internal_locations( weap_loc, weapon_internal_items );
    ret.insert( ret.end(), weapon_internal_items.begin(), weapon_internal_items.end() );
    for( item &worn_it : worn ) {
        item_location worn_loc( *this, &worn_it );
        std::vector<item_location> worn_internal_items;
        recur_internal_locations( worn_loc, worn_internal_items );
        ret.insert( ret.end(), worn_internal_items.begin(), worn_internal_items.end() );
    }
    return ret;
}

std::vector<item_location> Character::top_items_loc()
{
    std::vector<item_location> ret;
    for( item &worn_it : worn ) {
        item_location worn_loc( *this, &worn_it );
        ret.push_back( worn_loc );
    }
    return ret;
}

item *Character::invlet_to_item( const int linvlet )
{
    // Invlets may come from curses, which may also return any kind of key codes, those being
    // of type int and they can become valid, but different characters when casted to char.
    // Example: KEY_NPAGE (returned when the player presses the page-down key) is 0x152,
    // casted to char would yield 0x52, which happens to be 'R', a valid invlet.
    if( linvlet > std::numeric_limits<char>::max() || linvlet < std::numeric_limits<char>::min() ) {
        return nullptr;
    }
    const char invlet = static_cast<char>( linvlet );
    if( is_npc() ) {
        DebugLog( D_WARNING, D_GAME ) << "Why do you need to call Character::invlet_to_position on npc " <<
                                      name;
    }
    item *invlet_item = nullptr;
    visit_items( [&invlet, &invlet_item]( item * it, item * ) {
        if( it->invlet == invlet ) {
            invlet_item = it;
            return VisitResponse::ABORT;
        }
        return VisitResponse::NEXT;
    } );
    return invlet_item;
}

// Negative positions indicate weapon/clothing, 0 & positive indicate inventory
const item &Character::i_at( int position ) const
{
    if( position == -1 ) {
        return weapon;
    }
    if( position < -1 ) {
        int worn_index = worn_position_to_index( position );
        if( static_cast<size_t>( worn_index ) < worn.size() ) {
            auto iter = worn.begin();
            std::advance( iter, worn_index );
            return *iter;
        }
    }

    return inv->find_item( position );
}

item &Character::i_at( int position )
{
    return const_cast<item &>( const_cast<const Character *>( this )->i_at( position ) );
}

int Character::get_item_position( const item *it ) const
{
    if( weapon.has_item( *it ) ) {
        return -1;
    }

    int p = 0;
    for( const auto &e : worn ) {
        if( e.has_item( *it ) ) {
            return worn_position_to_index( p );
        }
        p++;
    }

    return inv->position_by_item( it );
}

item Character::i_rem( const item *it )
{
    auto tmp = remove_items_with( [&it]( const item & i ) {
        return &i == it;
    }, 1 );
    if( tmp.empty() ) {
        debugmsg( "did not found item %s to remove it!", it->tname() );
        return item();
    }
    return tmp.front();
}

void Character::i_rem_keep_contents( const item *const it )
{
    i_rem( it ).spill_contents( pos() );
}

bool Character::i_add_or_drop( item &it, int qty, const item *avoid )
{
    bool retval = true;
    bool drop = it.made_of( phase_id::LIQUID );
    bool add = it.is_gun() || !it.is_irremovable();
    inv->assign_empty_invlet( it, *this );
    map &here = get_map();
    for( int i = 0; i < qty; ++i ) {
        drop |= !can_pickWeight( it, !get_option<bool>( "DANGEROUS_PICKUPS" ) ) || !can_pickVolume( it );
        if( drop ) {
            retval &= !here.add_item_or_charges( pos(), it ).is_null();
        } else if( add ) {
            i_add( it, true, avoid, /*allow_drop=*/true, /*allow_wield=*/!has_wield_conflicts( it ) );
        }
    }

    return retval;
}

void Character::handle_contents_changed( const std::vector<item_location> &containers )
{
    if( containers.empty() ) {
        return;
    }

    class item_loc_with_depth
    {
        public:
            // NOLINTNEXTLINE(google-explicit-constructor)
            item_loc_with_depth( const item_location &_loc )
                : _loc( _loc ) {
                item_location ancestor = _loc;
                while( ancestor.has_parent() ) {
                    ++_depth;
                    ancestor = ancestor.parent_item();
                }
            }

            const item_location &loc() const {
                return _loc;
            }

            int depth() const {
                return _depth;
            }

        private:
            item_location _loc;
            int _depth = 0;
    };

    class sort_by_depth
    {
        public:
            bool operator()( const item_loc_with_depth &lhs, const item_loc_with_depth &rhs ) const {
                return lhs.depth() < rhs.depth();
            }
    };

    std::multiset<item_loc_with_depth, sort_by_depth> sorted_containers(
        containers.begin(), containers.end() );
    map &m = get_map();

    // unseal and handle containers, from innermost (max depth) to outermost (min depth)
    // so inner containers are always handled before outer containers are possibly removed.
    while( !sorted_containers.empty() ) {
        item_location loc = std::prev( sorted_containers.end() )->loc();
        sorted_containers.erase( std::prev( sorted_containers.end() ) );
        if( !loc ) {
            debugmsg( "invalid item location" );
            continue;
        }
        loc->on_contents_changed();
        const bool handle_drop = loc.where() != item_location::type::map && !is_wielding( *loc );
        bool drop_unhandled = false;
        for( item_pocket *const pocket : loc->contents.get_all_contained_pockets().value() ) {
            if( pocket && !pocket->sealed() ) {
                // pockets are unsealed but on_contents_changed is not called
                // in contents_change_handler::unseal_pocket_containing
                pocket->on_contents_changed();
            }
            if( pocket && handle_drop && pocket->will_spill() ) {
                // the pocket's contents (with a larger depth value) are not
                // inside `sorted_containers` and can be safely disposed of.
                pocket->handle_liquid_or_spill( *this, /*avoid=*/&*loc );
                // drop the container instead if canceled.
                if( !pocket->empty() ) {
                    // drop later since we still need to access the container item
                    drop_unhandled = true;
                    // canceling one pocket cancels spilling for the whole container
                    break;
                }
            }
        }

        if( loc.has_parent() ) {
            item_location parent_loc = loc.parent_item();
            item_loc_with_depth parent( parent_loc );
            item_pocket *const pocket = parent_loc->contained_where( *loc );
            pocket->unseal();
            bool exists = false;
            auto it = sorted_containers.lower_bound( parent );
            for( ; it != sorted_containers.end() && it->depth() == parent.depth(); ++it ) {
                if( it->loc() == parent_loc ) {
                    exists = true;
                    break;
                }
            }
            if( !exists ) {
                sorted_containers.emplace_hint( it, parent );
            }
        }

        if( drop_unhandled ) {
            // We can drop the unhandled container now since the container and
            // its contents (with a larger depth) are not inside `sorted_containers`.
            add_msg_player_or_npc(
                _( "To avoid spilling its contents, you set your %1$s on the %2$s." ),
                _( "To avoid spilling its contents, <npcname> sets their %1$s on the %2$s." ),
                loc->display_name(), m.name( pos() )
            );
            item it_copy( *loc );
            loc.remove_item();
            // target item of `loc` is invalidated and should not be used from now on
            m.add_item_or_charges( pos(), it_copy );
        }
    }
}

void contents_change_handler::add_unsealed( const item_location &loc )
{
    if( std::find( unsealed.begin(), unsealed.end(), loc ) == unsealed.end() ) {
        unsealed.emplace_back( loc );
    }
}

void contents_change_handler::unseal_pocket_containing( const item_location &loc )
{
    if( loc.has_parent() ) {
        item_location parent = loc.parent_item();
        item_pocket *const pocket = parent->contained_where( *loc );
        if( pocket ) {
            // on_contents_changed restacks the pocket and should be called later
            // in Character::handle_contents_changed
            pocket->unseal();
        } else {
            debugmsg( "parent container does not contain item" );
        }
        parent.on_contents_changed();
        add_unsealed( parent );
    }
}

void contents_change_handler::handle_by( Character &guy )
{
    // some containers could have been destroyed by e.g. monster attack
    auto it = std::remove_if( unsealed.begin(), unsealed.end(),
    []( const item_location & loc ) -> bool {
        return !loc;
    } );
    unsealed.erase( it, unsealed.end() );
    guy.handle_contents_changed( unsealed );
    unsealed.clear();
}

void contents_change_handler::serialize( JsonOut &jsout ) const
{
    jsout.write( unsealed );
}

void contents_change_handler::deserialize( JsonIn &jsin )
{
    jsin.read( unsealed );
}

std::list<item *> Character::get_dependent_worn_items( const item &it )
{
    std::list<item *> dependent;
    // Adds dependent worn items recursively
    const std::function<void( const item &it )> add_dependent = [&]( const item & it ) {
        for( item &wit : worn ) {
            if( &wit == &it || !wit.is_worn_only_with( it ) ) {
                continue;
            }
            const auto iter = std::find_if( dependent.begin(), dependent.end(),
            [&wit]( const item * dit ) {
                return &wit == dit;
            } );
            if( iter == dependent.end() ) { // Not in the list yet
                add_dependent( wit );
                dependent.push_back( &wit );
            }
        }
    };

    if( is_worn( it ) ) {
        add_dependent( it );
    }

    return dependent;
}

void Character::drop( item_location loc, const tripoint &where )
{
    drop( { std::make_pair( loc, loc->count() ) }, where );
    invalidate_inventory_validity_cache();
}

void Character::drop( const drop_locations &what, const tripoint &target,
                      bool stash )
{
    if( what.empty() ) {
        return;
    }

    if( rl_dist( pos(), target ) > 1 || !( stash || get_map().can_put_items( target ) ) ) {
        add_msg_player_or_npc( m_info, _( "You can't place items here!" ),
                               _( "<npcname> can't place items here!" ) );
        return;
    }

    const tripoint placement = target - pos();
    std::vector<drop_or_stash_item_info> items;
    for( drop_location item_pair : what ) {
        if( can_drop( *item_pair.first ).success() ) {
            items.emplace_back( item_pair.first, item_pair.second );
        }
    }
    if( stash ) {
        assign_activity( player_activity( stash_activity_actor(
                                              items, placement
                                          ) ) );
    } else {
        assign_activity( player_activity( drop_activity_actor(
                                              items, placement, /*force_ground=*/false
                                          ) ) );
    }
}

invlets_bitset Character::allocated_invlets() const
{
    invlets_bitset invlets = inv->allocated_invlets();

    invlets.set( weapon.invlet );
    for( const auto &w : worn ) {
        invlets.set( w.invlet );
    }

    invlets[0] = false;

    return invlets;
}

bool Character::has_active_item( const itype_id &id ) const
{
    return has_item_with( [id]( const item & it ) {
        return it.active && it.typeId() == id;
    } );
}

item Character::remove_weapon()
{
    item tmp = weapon;
    weapon = item();
    get_event_bus().send<event_type::character_wields_item>( getID(), weapon.typeId() );
    cached_info.erase( "weapon_value" );
    return tmp;
}

void Character::remove_mission_items( int mission_id )
{
    if( mission_id == -1 ) {
        return;
    }
    remove_items_with( has_mission_item_filter { mission_id } );
}

std::vector<const item *> Character::get_ammo( const ammotype &at ) const
{
    return items_with( [at]( const item & it ) {
        return it.ammo_type() == at;
    } );
}

template <typename T, typename Output>
void find_ammo_helper( T &src, const item &obj, bool empty, Output out, bool nested )
{
    if( obj.is_watertight_container() ) {
        if( !obj.is_container_empty() ) {

            // Look for containers with the same type of liquid as that already in our container
            src.visit_items( [&src, &nested, &out, &obj]( item * node, item * parent ) {
                if( node == &obj ) {
                    // This stops containers and magazines counting *themselves* as ammo sources
                    return VisitResponse::SKIP;
                }
                // Prevents reloading with items frozen in watertight containers.
                if( parent != nullptr && parent->is_watertight_container() && node->is_frozen_liquid() ) {
                    return VisitResponse::SKIP;
                }

                // Liquids not in a watertight container are skipped.
                if( parent != nullptr && !parent->is_watertight_container() &&
                    node->made_of( phase_id::LIQUID ) ) {
                    return VisitResponse::SKIP;
                }

                // Spills have no parent.
                if( parent == nullptr && node->made_of_from_type( phase_id::LIQUID ) ) {
                    return VisitResponse::SKIP;
                }

                if( !nested && node->is_container() && parent != nullptr && parent->is_container() ) {
                    return VisitResponse::SKIP;
                }

                if( node->made_of_from_type( phase_id::LIQUID ) ) {
                    out = item_location( item_location( src, parent ), node );
                }

                return VisitResponse::NEXT;
            } );
        } else {
            // Look for containers with any liquid and loose frozen liquids
            src.visit_items( [&src, &nested, &out]( item * node, item * parent ) {
                // Prevents reloading with items frozen in watertight containers.
                if( parent != nullptr && parent->is_watertight_container() && node->is_frozen_liquid() ) {
                    return VisitResponse::SKIP;
                }

                // Liquids not in a watertight container are skipped.
                if( parent != nullptr && !parent->is_watertight_container() &&
                    node->made_of( phase_id::LIQUID ) ) {
                    return VisitResponse::SKIP;
                }

                // Spills have no parent.
                if( parent == nullptr && node->made_of_from_type( phase_id::LIQUID ) ) {
                    return VisitResponse::SKIP;
                }

                if( !nested && node->is_container() && parent != nullptr && parent->is_container() ) {
                    return VisitResponse::SKIP;
                }

                if( node->made_of_from_type( phase_id::LIQUID ) ) {
                    out = item_location( item_location( src, parent ), node );
                }

                return VisitResponse::NEXT;
            } );
        }
    }
    if( obj.magazine_integral() ) {
        // find suitable ammo excluding that already loaded in magazines
        std::set<ammotype> ammo = obj.ammo_types();

        src.visit_items( [&src, &nested, &out, ammo]( item * node, item * parent ) {
            if( !node->made_of_from_type( phase_id::SOLID ) && parent == nullptr ) {
                // some liquids are ammo but we can't reload with them unless within a container or frozen
                return VisitResponse::SKIP;
            }
            if( !node->made_of( phase_id::SOLID ) && parent != nullptr ) {
                for( const ammotype &at : ammo ) {
                    if( node->ammo_type() == at ) {
                        out = item_location( src, node );
                    }
                }
                return VisitResponse::SKIP;
            }

            // Solid ammo gets skipped earlier than non-solid because it does not need a container.
            if( !nested && parent != nullptr && parent->is_container() &&
                !node->made_of_from_type( phase_id::LIQUID ) && !node->made_of( phase_id::GAS ) ) {
                return VisitResponse::SKIP;
            }

            if( !nested && node->is_container() && parent != nullptr && parent->is_container() ) {
                return VisitResponse::SKIP;
            }

            for( const ammotype &at : ammo ) {
                if( node->ammo_type() == at ) {
                    out = item_location( src, node );
                }
            }
            if( node->is_magazine() &&
                ( parent == nullptr || node != parent->magazine_current() ) &&
                node->has_flag( flag_SPEEDLOADER ) ) {
                if( node->ammo_remaining() ) {
                    out = item_location( src, node );
                }
            }
            return VisitResponse::NEXT;
        } );
    } else {
        // find compatible magazines excluding those already loaded in tools/guns
        const auto mags = obj.magazine_compatible();

        src.visit_items( [&src, &nested, &out, mags, empty]( item * node, item * parent ) {
            if( node->is_magazine() &&
                ( parent == nullptr || node != parent->magazine_current() ) ) {
                if( mags.count( node->typeId() ) && ( node->ammo_remaining() || empty ) ) {
                    out = item_location( src, node );
                }
                return VisitResponse::SKIP;
            }
            return nested ? VisitResponse::NEXT : VisitResponse::SKIP;
        } );
    }
}

std::vector<item_location> Character::find_ammo( const item &obj, bool empty, int radius ) const
{
    std::vector<item_location> res;

    find_ammo_helper( const_cast<Character &>( *this ), obj, empty, std::back_inserter( res ), true );

    if( radius >= 0 ) {
        for( auto &cursor : map_selector( pos(), radius ) ) {
            find_ammo_helper( cursor, obj, empty, std::back_inserter( res ), false );
        }
        for( auto &cursor : vehicle_selector( pos(), radius ) ) {
            find_ammo_helper( cursor, obj, empty, std::back_inserter( res ), false );
        }
    }

    return res;
}

std::vector<item_location> Character::find_reloadables()
{
    std::vector<item_location> reloadables;

    visit_items( [this, &reloadables]( item * node, item * ) {
        if( !node->is_gun() && !node->is_magazine() ) {
            return VisitResponse::NEXT;
        }
        bool reloadable = false;
        if( node->is_gun() && !node->magazine_compatible().empty() ) {
            reloadable = node->magazine_current() == nullptr ||
                         node->remaining_ammo_capacity() > 0;
        } else {
            reloadable = ( node->is_magazine() ||
                           ( node->is_gun() && node->magazine_integral() ) ) &&
                         node->remaining_ammo_capacity() > 0;
        }
        if( reloadable ) {
            reloadables.push_back( item_location( *this, node ) );
        }
        return VisitResponse::NEXT;
    } );
    return reloadables;
}

units::mass Character::weight_carried() const
{
    if( cached_weight_carried ) {
        return *cached_weight_carried;
    }
    cached_weight_carried = weight_carried_with_tweaks( item_tweaks() );
    return *cached_weight_carried;
}

void Character::invalidate_weight_carried_cache()
{
    cached_weight_carried = cata::nullopt;
}

units::mass Character::best_nearby_lifting_assist() const
{
    return best_nearby_lifting_assist( this->pos() );
}

units::mass Character::best_nearby_lifting_assist( const tripoint &world_pos ) const
{
    const quality_id LIFT( "LIFT" );
    int mech_lift = 0;
    if( is_mounted() ) {
        auto *mons = mounted_creature.get();
        if( mons->has_flag( MF_RIDEABLE_MECH ) ) {
            mech_lift = mons->mech_str_addition() + 10;
        }
    }
    int lift_quality = std::max( { this->max_quality( LIFT ), mech_lift,
                                   map_selector( this->pos(), PICKUP_RANGE ).max_quality( LIFT ),
                                   vehicle_selector( world_pos, 4, true, true ).max_quality( LIFT )
                                 } );
    return lifting_quality_to_mass( lift_quality );
}

units::mass Character::weight_carried_with_tweaks( const std::vector<std::pair<item_location, int>>
        &locations ) const
{
    std::map<const item *, int> dropping;
    for( const std::pair<item_location, int> &location_pair : locations ) {
        dropping.emplace( location_pair.first.get_item(), location_pair.second );
    }
    return weight_carried_with_tweaks( item_tweaks( { dropping } ) );
}

units::mass Character::weight_carried_with_tweaks( const item_tweaks &tweaks ) const
{
    const std::map<const item *, int> empty;
    const std::map<const item *, int> &without = tweaks.without_items ? tweaks.without_items->get() :
            empty;

    // Worn items
    units::mass ret = 0_gram;
    for( const item &i : worn ) {
        if( !without.count( &i ) ) {
            for( const item *j : i.contents.all_items_ptr( item_pocket::pocket_type::CONTAINER ) ) {
                if( j->count_by_charges() ) {
                    ret -= get_selected_stack_weight( j, without );
                } else if( without.count( j ) ) {
                    ret -= j->weight();
                }
            }
            ret += i.weight();
        }
    }

    // Wielded item
    units::mass weaponweight = 0_gram;
    if( !without.count( &weapon ) ) {
        weaponweight += weapon.weight();
        for( const item *i : weapon.contents.all_items_ptr( item_pocket::pocket_type::CONTAINER ) ) {
            if( i->count_by_charges() ) {
                weaponweight -= get_selected_stack_weight( i, without );
            } else if( without.count( i ) ) {
                weaponweight -= i->weight();
            }
        }
    } else if( weapon.count_by_charges() ) {
        weaponweight += weapon.weight() - get_selected_stack_weight( &weapon, without );
    }

    // Exclude wielded item if using lifting tool
    if( ( weaponweight + ret <= weight_capacity() ) || ( g->new_game ||
            best_nearby_lifting_assist() < weaponweight ) ) {
        ret += weaponweight;
    }

    return ret;
}

units::mass Character::get_selected_stack_weight( const item *i,
        const std::map<const item *, int> &without ) const
{
    auto stack = without.find( i );
    if( stack != without.end() ) {
        int selected = stack->second;
        item copy = *i;
        copy.charges = selected;
        return copy.weight();
    }

    return 0_gram;
}

units::volume Character::volume_carried_with_tweaks( const
        std::vector<std::pair<item_location, int>>
        &locations ) const
{
    std::map<const item *, int> dropping;
    for( const std::pair<item_location, int> &location_pair : locations ) {
        dropping.emplace( location_pair.first.get_item(), location_pair.second );
    }
    return volume_carried_with_tweaks( item_tweaks( { dropping } ) );
}

units::volume Character::volume_carried_with_tweaks( const item_tweaks &tweaks ) const
{
    const std::map<const item *, int> empty;
    const std::map<const item *, int> &without = tweaks.without_items ? tweaks.without_items->get() :
            empty;

    // Worn items
    units::volume ret = 0_ml;
    for( const item &i : worn ) {
        if( !without.count( &i ) ) {
            ret += i.contents.get_contents_volume_with_tweaks( without );
        }
    }

    // Wielded item
    if( !without.count( &weapon ) ) {
        ret += weapon.contents.get_contents_volume_with_tweaks( without );
    }

    return ret;
}

units::mass Character::weight_capacity() const
{
    // Get base capacity from creature,
    // then apply player-only mutation and trait effects.
    units::mass ret = Creature::weight_capacity();
    /** @EFFECT_STR increases carrying capacity */
    ret += get_str() * 4_kilogram;
    ret *= mutation_value( "weight_capacity_modifier" );

    units::mass worn_weight_bonus = 0_gram;
    for( const item &it : worn ) {
        ret *= it.get_weight_capacity_modifier();
        worn_weight_bonus += it.get_weight_capacity_bonus();
    }

    units::mass bio_weight_bonus = 0_gram;
    for( const bionic_id &bid : get_bionics() ) {
        ret *= bid->weight_capacity_modifier;
        bio_weight_bonus +=  bid->weight_capacity_bonus;
    }

    ret += bio_weight_bonus + worn_weight_bonus;

    ret = enchantment_cache->modify_value( enchant_vals::mod::CARRY_WEIGHT, ret );

    if( ret < 0_gram ) {
        ret = 0_gram;
    }
    if( is_mounted() ) {
        auto *mons = mounted_creature.get();
        // the mech has an effective strength for other purposes, like hitting.
        // but for lifting, its effective strength is even higher, due to its sturdy construction, leverage,
        // and being built entirely for that purpose with hydraulics etc.
        ret = mons->mech_str_addition() == 0 ? ret : ( mons->mech_str_addition() + 10 ) * 4_kilogram;
    }
    return ret;
}

bool Character::can_pickVolume( const item &it, bool ) const
{
    if( weapon.can_contain( it ) ) {
        return true;
    }
    for( const item &w : worn ) {
        if( w.can_contain( it ) ) {
            return true;
        }
    }
    return false;
}

bool Character::can_pickWeight( const item &it, bool safe ) const
{
    if( !safe ) {
        // Character can carry up to four times their maximum weight
        return ( weight_carried() + it.weight() <= weight_capacity() * 4 );
    } else {
        return ( weight_carried() + it.weight() <= weight_capacity() );
    }
}

bool Character::can_pickWeight_partial( const item &it, bool safe ) const
{
    item copy = it;
    if( it.count_by_charges() ) {
        copy.charges = 1;
    }

    return can_pickWeight( copy, safe );
}

bool Character::can_use( const item &it, const item &context ) const
{
    if( has_effect( effect_incorporeal ) ) {
        add_msg_player_or_npc( m_bad, _( "You can't use anything while incorporeal." ),
                               _( "<npcname> can't use anything while incorporeal." ) );
        return false;
    }
    const auto &ctx = !context.is_null() ? context : it;

    if( !meets_requirements( it, ctx ) ) {
        const std::string unmet( enumerate_unmet_requirements( it, ctx ) );

        if( &it == &ctx ) {
            //~ %1$s - list of unmet requirements, %2$s - item name.
            add_msg_player_or_npc( m_bad, _( "You need at least %1$s to use this %2$s." ),
                                   _( "<npcname> needs at least %1$s to use this %2$s." ),
                                   unmet, it.tname() );
        } else {
            //~ %1$s - list of unmet requirements, %2$s - item name, %3$s - indirect item name.
            add_msg_player_or_npc( m_bad, _( "You need at least %1$s to use this %2$s with your %3$s." ),
                                   _( "<npcname> needs at least %1$s to use this %2$s with their %3$s." ),
                                   unmet, it.tname(), ctx.tname() );
        }

        return false;
    }

    return true;
}

ret_val<bool> Character::can_wear( const item &it, bool with_equip_change ) const
{
    if( has_effect( effect_incorporeal ) ) {
        return ret_val<bool>::make_failure( _( "You can't wear anything while incorporeal." ) );
    }
    if( !it.is_armor() ) {
        return ret_val<bool>::make_failure( _( "Putting on a %s would be tricky." ), it.tname() );
    }

    if( has_trait( trait_WOOLALLERGY ) && ( it.made_of( material_id( "wool" ) ) ||
                                            it.has_own_flag( flag_wooled ) ) ) {
        return ret_val<bool>::make_failure( _( "Can't wear that, it's made of wool!" ) );
    }

    if( it.is_filthy() && has_trait( trait_SQUEAMISH ) ) {
        return ret_val<bool>::make_failure( _( "Can't wear that, it's filthy!" ) );
    }

    if( !it.has_flag( flag_OVERSIZE ) && !it.has_flag( flag_SEMITANGIBLE ) ) {
        for( const trait_id &mut : get_mutations() ) {
            const auto &branch = mut.obj();
            if( branch.conflicts_with_item( it ) ) {
                return ret_val<bool>::make_failure( is_player() ?
                                                    _( "Your %s mutation prevents you from wearing your %s." ) :
                                                    _( "My %s mutation prevents me from wearing this %s." ), branch.name(),
                                                    it.type_name() );
            }
        }
        if( it.covers( body_part_head ) && !it.has_flag( flag_SEMITANGIBLE ) &&
            !it.made_of( material_id( "wool" ) ) && !it.made_of( material_id( "cotton" ) ) &&
            !it.made_of( material_id( "nomex" ) ) && !it.made_of( material_id( "leather" ) ) &&
            ( has_trait( trait_HORNS_POINTED ) || has_trait( trait_ANTENNAE ) ||
              has_trait( trait_ANTLERS ) ) ) {
            return ret_val<bool>::make_failure( _( "Cannot wear a helmet over %s." ),
                                                ( has_trait( trait_HORNS_POINTED ) ? _( "horns" ) :
                                                  ( has_trait( trait_ANTENNAE ) ? _( "antennae" ) : _( "antlers" ) ) ) );
        }
    }

    if( it.has_flag( flag_SPLINT ) ) {
        bool need_splint = false;
        for( const bodypart_id &bp : get_all_body_parts() ) {
            if( !it.covers( bp ) ) {
                continue;
            }
            if( is_limb_broken( bp ) && !worn_with_flag( flag_SPLINT, bp ) ) {
                need_splint = true;
                break;
            }
        }
        if( !need_splint ) {
            return ret_val<bool>::make_failure( is_player() ?
                                                _( "You don't have any broken limbs this could help." )
                                                : _( "%s doesn't have any broken limbs this could help." ), name );
        }
    }

    if( it.has_flag( flag_TOURNIQUET ) ) {
        bool need_tourniquet = false;
        for( const bodypart_id &bp : get_all_body_parts() ) {
            if( !it.covers( bp ) ) {
                continue;
            }
            effect e = get_effect( effect_bleed, bp );
            if( !e.is_null() && e.get_intensity() > e.get_max_intensity() / 4 &&
                !worn_with_flag( flag_TOURNIQUET, bp ) ) {
                need_tourniquet = true;
                break;
            }
        }
        if( !need_tourniquet ) {
            std::string msg;
            if( is_player() ) {
                msg = _( "You don't need a tourniquet to stop the bleeding." );
            } else {
                msg = string_format( _( "%s doesn't need a tourniquet to stop the bleeding." ), name );
            }
            return ret_val<bool>::make_failure( msg );
        }
    }

    if( it.has_flag( flag_RESTRICT_HANDS ) && !has_two_arms() ) {
        return ret_val<bool>::make_failure( ( is_player() ? _( "You don't have enough arms to wear that." )
                                              : string_format( _( "%s doesn't have enough arms to wear that." ), name ) ) );
    }

    //Everything checked after here should be something that could be solved by changing equipment
    if( with_equip_change ) {
        return ret_val<bool>::make_success();
    }

    if( it.is_power_armor() ) {
        for( const item &elem : worn ) {
            if( elem.get_covered_body_parts().make_intersection( it.get_covered_body_parts() ).any() &&
                !elem.has_flag( flag_POWERARMOR_COMPATIBLE ) ) {
                return ret_val<bool>::make_failure( _( "Can't wear power armor over other gear!" ) );
            }
        }
        if( !it.covers( body_part_torso ) ) {
            bool power_armor = false;
            if( !worn.empty() ) {
                for( const item &elem : worn ) {
                    if( elem.is_power_armor() ) {
                        power_armor = true;
                        break;
                    }
                }
            }
            if( !power_armor ) {
                return ret_val<bool>::make_failure(
                           _( "You can only wear power armor components with power armor!" ) );
            }
        }

        for( const item &i : worn ) {
            if( i.is_power_armor() && i.typeId() == it.typeId() ) {
                return ret_val<bool>::make_failure( _( "Can't wear more than one %s!" ), it.tname() );
            }
        }
    } else {
        // Only headgear can be worn with power armor, except other power armor components.
        // You can't wear headgear if power armor helmet is already sitting on your head.
        bool has_helmet = false;
        if( !it.has_flag( flag_POWERARMOR_COMPATIBLE ) && ( ( is_wearing_power_armor( &has_helmet ) &&
                ( has_helmet || !( it.covers( body_part_head ) || it.covers( body_part_mouth ) ||
                                   it.covers( body_part_eyes ) ) ) ) ) ) {
            return ret_val<bool>::make_failure( _( "Can't wear %s with power armor!" ), it.tname() );
        }
    }

    // Check if we don't have both hands available before wearing a briefcase, shield, etc. Also occurs if we're already wearing one.
    if( it.has_flag( flag_RESTRICT_HANDS ) && ( worn_with_flag( flag_RESTRICT_HANDS ) ||
            weapon.is_two_handed( *this ) ) ) {
        return ret_val<bool>::make_failure( ( is_player() ? _( "You don't have a hand free to wear that." )
                                              : string_format( _( "%s doesn't have a hand free to wear that." ), name ) ) );
    }

    for( const item &i : worn ) {
        if( i.has_flag( flag_ONLY_ONE ) && i.typeId() == it.typeId() ) {
            return ret_val<bool>::make_failure( _( "Can't wear more than one %s!" ), it.tname() );
        }
    }

    if( amount_worn( it.typeId() ) >= MAX_WORN_PER_TYPE ) {
        return ret_val<bool>::make_failure( _( "Can't wear %i or more %s at once." ),
                                            MAX_WORN_PER_TYPE + 1, it.tname( MAX_WORN_PER_TYPE + 1 ) );
    }

    if( ( ( it.covers( body_part_foot_l ) && is_wearing_shoes( side::LEFT ) ) ||
          ( it.covers( body_part_foot_r ) && is_wearing_shoes( side::RIGHT ) ) ) &&
        ( !it.has_flag( flag_OVERSIZE ) || !it.has_flag( flag_OUTER ) ) && !it.has_flag( flag_SKINTIGHT ) &&
        !it.has_flag( flag_BELTED ) && !it.has_flag( flag_PERSONAL ) && !it.has_flag( flag_AURA ) &&
        !it.has_flag( flag_SEMITANGIBLE ) ) {
        // Checks to see if the player is wearing shoes
        return ret_val<bool>::make_failure( ( is_player() ? _( "You're already wearing footwear!" )
                                              : string_format( _( "%s is already wearing footwear!" ), name ) ) );
    }

    if( it.covers( body_part_head ) &&
        !it.has_flag( flag_HELMET_COMPAT ) && !it.has_flag( flag_SKINTIGHT ) &&
        !it.has_flag( flag_PERSONAL ) &&
        !it.has_flag( flag_AURA ) && !it.has_flag( flag_SEMITANGIBLE ) && !it.has_flag( flag_OVERSIZE ) &&
        is_wearing_helmet() ) {
        return ret_val<bool>::make_failure( wearing_something_on( body_part_head ),
                                            ( is_player() ? _( "You can't wear that with other headgear!" )
                                              : string_format( _( "%s can't wear that with other headgear!" ), name ) ) );
    }

    if( it.covers( body_part_head ) && !it.has_flag( flag_SEMITANGIBLE ) &&
        ( it.has_flag( flag_SKINTIGHT ) || it.has_flag( flag_HELMET_COMPAT ) ) &&
        ( head_cloth_encumbrance() + it.get_encumber( *this, body_part_head ) > 40 ) ) {
        return ret_val<bool>::make_failure( ( is_player() ? _( "You can't wear that much on your head!" )
                                              : string_format( _( "%s can't wear that much on their head!" ), name ) ) );
    }

    return ret_val<bool>::make_success();
}

ret_val<bool> Character::can_unwield( const item &it ) const
{
    if( it.has_flag( flag_NO_UNWIELD ) ) {
        cata::optional<int> wi;
        // check if "it" is currently wielded fake bionic weapon that can be deactivated
        if( !( is_wielding( it ) && ( wi = active_bionic_weapon_index() ) &&
               can_deactivate_bionic( *wi ).success() ) ) {
            return ret_val<bool>::make_failure( _( "You cannot unwield your %s." ), it.tname() );
        }
    }

    return ret_val<bool>::make_success();
}

ret_val<bool> Character::can_drop( const item &it ) const
{
    if( it.has_flag( flag_NO_UNWIELD ) ) {
        return ret_val<bool>::make_failure( _( "You cannot drop your %s." ), it.tname() );
    }
    return ret_val<bool>::make_success();
}

void Character::invalidate_inventory_validity_cache()
{
    cache_inventory_is_valid = false;
}

void Character::drop_invalid_inventory()
{
    if( cache_inventory_is_valid ) {
        return;
    }
    bool dropped_liquid = false;
    for( const std::list<item> *stack : inv->const_slice() ) {
        const item &it = stack->front();
        if( it.made_of( phase_id::LIQUID ) ) {
            dropped_liquid = true;
            get_map().add_item_or_charges( pos(), it );
            // must be last
            i_rem( &it );
        }
    }
    if( dropped_liquid ) {
        add_msg_if_player( m_bad, _( "Liquid from your inventory has leaked onto the ground." ) );
    }

    weapon.contents.overflow( pos() );
    for( item &w : worn ) {
        w.contents.overflow( pos() );
    }

    cache_inventory_is_valid = true;
}

bool Character::is_wielding( const item &target ) const
{
    return &weapon == &target;
}

bool Character::is_wearing( const itype_id &it ) const
{
    for( const item &i : worn ) {
        if( i.typeId() == it ) {
            return true;
        }
    }
    return false;
}

bool Character::is_wearing_on_bp( const itype_id &it, const bodypart_id &bp ) const
{
    for( const item &i : worn ) {
        if( i.typeId() == it && i.covers( bp ) ) {
            return true;
        }
    }
    return false;
}

bool Character::worn_with_flag( const flag_id &f, const bodypart_id &bp ) const
{
    return std::any_of( worn.begin(), worn.end(), [&f, bp]( const item & it ) {
        return it.has_flag( f ) && ( bp == bodypart_str_id::NULL_ID() || it.covers( bp ) );
    } );
}

bool Character::worn_with_flag( const flag_id &f ) const
{
    return std::any_of( worn.begin(), worn.end(), [&f]( const item & it ) {
        return it.has_flag( f ) ;
    } );
}

item Character::item_worn_with_flag( const flag_id &f, const bodypart_id &bp ) const
{
    item it_with_flag;
    for( const item &it : worn ) {
        if( it.has_flag( f ) && ( bp == bodypart_str_id::NULL_ID() || it.covers( bp ) ) ) {
            it_with_flag = it;
            break;
        }
    }
    return it_with_flag;
}

item Character::item_worn_with_flag( const flag_id &f ) const
{
    item it_with_flag;
    for( const item &it : worn ) {
        if( it.has_flag( f ) ) {
            it_with_flag = it;
            break;
        }
    }
    return it_with_flag;
}

std::vector<std::string> Character::get_overlay_ids() const
{
    std::vector<std::string> rval;
    std::multimap<int, std::string> mutation_sorting;
    int order;
    std::string overlay_id;

    // first get effects
    for( const auto &eff_pr : *effects ) {
        rval.push_back( "effect_" + eff_pr.first.str() );
    }

    // then get mutations
    for( const std::pair<const trait_id, trait_data> &mut : my_mutations ) {
        overlay_id = ( mut.second.powered ? "active_" : "" ) + mut.first.str();
        order = get_overlay_order_of_mutation( overlay_id );
        mutation_sorting.insert( std::pair<int, std::string>( order, overlay_id ) );
    }

    // then get bionics
    for( const bionic &bio : *my_bionics ) {
        overlay_id = ( bio.powered ? "active_" : "" ) + bio.id.str();
        order = get_overlay_order_of_mutation( overlay_id );
        mutation_sorting.insert( std::pair<int, std::string>( order, overlay_id ) );
    }

    for( auto &mutorder : mutation_sorting ) {
        rval.push_back( "mutation_" + mutorder.second );
    }

    // next clothing
    // TODO: worry about correct order of clothing overlays
    for( const item &worn_item : worn ) {
        rval.push_back( "worn_" + worn_item.typeId().str() );
    }

    // last weapon
    // TODO: might there be clothing that covers the weapon?
    if( is_armed() ) {
        rval.push_back( "wielded_" + weapon.typeId().str() );
    }

    if( !is_walking() ) {
        rval.push_back( move_mode.str() );
    }

    return rval;
}

const SkillLevelMap &Character::get_all_skills() const
{
    return *_skills;
}

const SkillLevel &Character::get_skill_level_object( const skill_id &ident ) const
{
    return _skills->get_skill_level_object( ident );
}

SkillLevel &Character::get_skill_level_object( const skill_id &ident )
{
    return _skills->get_skill_level_object( ident );
}

int Character::get_skill_level( const skill_id &ident ) const
{
    return _skills->get_skill_level( ident );
}

int Character::get_skill_level( const skill_id &ident, const item &context ) const
{
    return _skills->get_skill_level( ident, context );
}

void Character::set_skill_level( const skill_id &ident, const int level )
{
    get_skill_level_object( ident ).level( level );
}

void Character::mod_skill_level( const skill_id &ident, const int delta )
{
    _skills->mod_skill_level( ident, delta );
}

std::string Character::enumerate_unmet_requirements( const item &it, const item &context ) const
{
    std::vector<std::string> unmet_reqs;

    const auto check_req = [ &unmet_reqs ]( const std::string & name, int cur, int req ) {
        if( cur < req ) {
            unmet_reqs.push_back( string_format( "%s %d", name, req ) );
        }
    };

    check_req( _( "strength" ),     get_str(), it.get_min_str() );
    check_req( _( "dexterity" ),    get_dex(), it.type->min_dex );
    check_req( _( "intelligence" ), get_int(), it.type->min_int );
    check_req( _( "perception" ),   get_per(), it.type->min_per );

    for( const auto &elem : it.type->min_skills ) {
        check_req( context.contextualize_skill( elem.first )->name(),
                   get_skill_level( elem.first, context ),
                   elem.second );
    }

    return enumerate_as_string( unmet_reqs );
}

int Character::rust_rate() const
{
    const std::string &rate_option = get_option<std::string>( "SKILL_RUST" );
    if( rate_option == "off" ) {
        return 0;
    }

    // Stat window shows stat effects on based on current stat
    int intel = get_int();
    /** @EFFECT_INT increases skill rust delay by 10% per level above 8 */
    int ret = ( ( rate_option == "vanilla" || rate_option == "capped" ) ?
                100 : 100 + 10 * ( intel - 8 ) );

    ret *= mutation_value( "skill_rust_multiplier" );

    if( ret < 0 ) {
        ret = 0;
    }

    return ret;
}

int Character::read_speed( bool return_stat_effect ) const
{
    // Stat window shows stat effects on based on current stat
    const int intel = get_int();
    /** @EFFECT_INT increases reading speed by 3s per level above 8*/
    int ret = to_moves<int>( 1_minutes ) - to_moves<int>( 3_seconds ) * ( intel - 8 );

    if( has_bionic( afs_bio_linguistic_coprocessor ) ) { // Aftershock
        ret *= .85;
    }

    ret *= mutation_value( "reading_speed_multiplier" );

    if( ret < to_moves<int>( 1_seconds ) ) {
        ret = to_moves<int>( 1_seconds );
    }
    // return_stat_effect actually matters here
    return return_stat_effect ? ret : ret * 100 / to_moves<int>( 1_minutes );
}

bool Character::meets_skill_requirements( const std::map<skill_id, int> &req,
        const item &context ) const
{
    return _skills->meets_skill_requirements( req, context );
}

bool Character::meets_skill_requirements( const construction &con ) const
{
    return std::all_of( con.required_skills.begin(), con.required_skills.end(),
    [&]( const std::pair<skill_id, int> &pr ) {
        return get_skill_level( pr.first ) >= pr.second;
    } );
}

bool Character::meets_stat_requirements( const item &it ) const
{
    return get_str() >= it.get_min_str() &&
           get_dex() >= it.type->min_dex &&
           get_int() >= it.type->min_int &&
           get_per() >= it.type->min_per;
}

bool Character::meets_requirements( const item &it, const item &context ) const
{
    const auto &ctx = !context.is_null() ? context : it;
    return meets_stat_requirements( it ) && meets_skill_requirements( it.type->min_skills, ctx );
}

void Character::make_bleed( const effect_source &source, const bodypart_id &bp,
                            time_duration duration, int intensity, bool permanent, bool force, bool defferred )
{
    int b_resist = 0;
    for( const trait_id &mut : get_mutations() ) {
        b_resist += mut.obj().bleed_resist;
    }

    if( b_resist > intensity ) {
        return;
    }

    add_effect( source, effect_bleed, duration, bp, permanent, intensity, force, defferred );
}

void Character::normalize()
{
    Creature::normalize();

    weary.clear();
    martial_arts_data->reset_style();
    weapon = item( "null", calendar::turn_zero );

    set_body();
    recalc_hp();
}

// Actual player death is mostly handled in game::is_game_over
void Character::die( Creature *nkiller )
{
    g->set_critter_died();
    set_killer( nkiller );
    set_time_died( calendar::turn );
    if( has_effect( effect_lightsnare ) ) {
        inv->add_item( item( "string_36", calendar::turn_zero ) );
        inv->add_item( item( "snare_trigger", calendar::turn_zero ) );
    }
    if( has_effect( effect_heavysnare ) ) {
        inv->add_item( item( "rope_6", calendar::turn_zero ) );
        inv->add_item( item( "snare_trigger", calendar::turn_zero ) );
    }
    if( has_effect( effect_beartrap ) ) {
        inv->add_item( item( "beartrap", calendar::turn_zero ) );
    }
    mission::on_creature_death( *this );
}

void Character::apply_skill_boost()
{
    for( const skill_boost &boost : skill_boost::get_all() ) {
        int skill_total = 0;
        for( const std::string &skill_str : boost.skills() ) {
            skill_total += get_skill_level( skill_id( skill_str ) );
        }
        mod_stat( boost.stat(), boost.calc_bonus( skill_total ) );
        if( boost.stat() == "str" ) {
            recalc_hp();
        }
    }
}

void Character::do_skill_rust()
{
    const int rust_rate_tmp = rust_rate();
    for( std::pair<const skill_id, SkillLevel> &pair : *_skills ) {
        const Skill &aSkill = *pair.first;
        SkillLevel &skill_level_obj = pair.second;

        if( aSkill.is_combat_skill() &&
            ( ( has_trait_flag( json_flag_PRED2 ) && calendar::once_every( 8_hours ) ) ||
              ( has_trait_flag( json_flag_PRED3 ) && calendar::once_every( 4_hours ) ) ||
              ( has_trait_flag( json_flag_PRED4 ) && calendar::once_every( 3_hours ) ) ) ) {
            // Their brain is optimized to remember this
            if( one_in( 13 ) ) {
                // They've already passed the roll to avoid rust at
                // this point, but print a message about it now and
                // then.
                //
                // 13 combat skills.
                // This means PRED2/PRED3/PRED4 think of hunting on
                // average every 8/4/3 hours, enough for immersion
                // without becoming an annoyance.
                //
                add_msg_if_player( _( "Your heart races as you recall your most recent hunt." ) );
                mod_stim( 1 );
            }
            continue;
        }

        const bool charged_bio_mem = get_power_level() > 25_J && has_active_bionic( bio_memory );
        const int oldSkillLevel = skill_level_obj.level();
        if( skill_level_obj.rust( charged_bio_mem, rust_rate_tmp ) ) {
            add_msg_if_player( m_warning,
                               _( "Your knowledge of %s begins to fade, but your memory banks retain it!" ), aSkill.name() );
            mod_power_level( -25_J );
        }
        const int newSkill = skill_level_obj.level();
        if( newSkill < oldSkillLevel ) {
            add_msg_if_player( m_bad, _( "Your skill in %s has reduced to %d!" ), aSkill.name(), newSkill );
        }
    }
}

void Character::reset_stats()
{
    // Bionic buffs
    if( has_active_bionic( bio_hydraulics ) ) {
        mod_str_bonus( 20 );
    }

    mod_str_bonus( get_mod_stat_from_bionic( character_stat::STRENGTH ) );
    mod_dex_bonus( get_mod_stat_from_bionic( character_stat::DEXTERITY ) );
    mod_per_bonus( get_mod_stat_from_bionic( character_stat::PERCEPTION ) );
    mod_int_bonus( get_mod_stat_from_bionic( character_stat::INTELLIGENCE ) );

    // Trait / mutation buffs
    mod_str_bonus( std::floor( mutation_value( "str_modifier" ) ) );
    mod_dodge_bonus( std::floor( mutation_value( "dodge_modifier" ) ) );

    /** @EFFECT_STR_MAX above 15 decreases Dodge bonus by 1 (NEGATIVE) */
    if( str_max >= 16 ) {
        mod_dodge_bonus( -1 );   // Penalty if we're huge
    }
    /** @EFFECT_STR_MAX below 6 increases Dodge bonus by 1 */
    else if( str_max <= 5 ) {
        mod_dodge_bonus( 1 );   // Bonus if we're small
    }

    apply_skill_boost();

    nv_cached = false;

    // Reset our stats to normal levels
    // Any persistent buffs/debuffs will take place in effects,
    // player::suffer(), etc.

    // repopulate the stat fields
    str_cur = str_max + get_str_bonus();
    dex_cur = dex_max + get_dex_bonus();
    per_cur = per_max + get_per_bonus();
    int_cur = int_max + get_int_bonus();

    // Floor for our stats.  No stat changes should occur after this!
    if( dex_cur < 0 ) {
        dex_cur = 0;
    }
    if( str_cur < 0 ) {
        str_cur = 0;
    }
    if( per_cur < 0 ) {
        per_cur = 0;
    }
    if( int_cur < 0 ) {
        int_cur = 0;
    }
}

void Character::reset()
{
    // TODO: Move reset_stats here, remove it from Creature
    Creature::reset();
}

bool Character::has_nv()
{
    static bool nv = false;

    if( !nv_cached ) {
        nv_cached = true;
        nv = ( worn_with_flag( flag_GNV_EFFECT ) ||
               has_flag( json_flag_NIGHT_VISION ) ||
               has_effect_with_flag( flag_EFFECT_NIGHT_VISION ) );
    }

    return nv;
}

void Character::calc_encumbrance()
{
    calc_encumbrance( item() );
}

void Character::calc_encumbrance( const item &new_item )
{

    std::map<bodypart_id, encumbrance_data> enc;
    item_encumb( enc, new_item );
    mut_cbm_encumb( enc );

    for( const std::pair<const bodypart_id, encumbrance_data> &elem : enc ) {
        set_part_encumbrance_data( elem.first, elem.second );
    }

}

units::mass Character::get_weight() const
{
    units::mass ret = 0_gram;
    units::mass wornWeight = std::accumulate( worn.begin(), worn.end(), 0_gram,
    []( units::mass sum, const item & itm ) {
        return sum + itm.weight();
    } );

    ret += bodyweight();       // The base weight of the player's body
    ret += inv->weight();           // Weight of the stored inventory
    ret += wornWeight;             // Weight of worn items
    ret += weapon.weight();        // Weight of wielded item
    ret += bionics_weight();       // Weight of installed bionics
    return ret;
}

bool Character::change_side( item &it, bool interactive )
{
    if( !it.swap_side() ) {
        if( interactive ) {
            add_msg_player_or_npc( m_info,
                                   _( "You cannot swap the side on which your %s is worn." ),
                                   _( "<npcname> cannot swap the side on which their %s is worn." ),
                                   it.tname() );
        }
        return false;
    }

    if( interactive ) {
        add_msg_player_or_npc( m_info, _( "You swap the side on which your %s is worn." ),
                               _( "<npcname> swaps the side on which their %s is worn." ),
                               it.tname() );
    }

    mod_moves( -250 );
    calc_encumbrance();

    return true;
}

bool Character::change_side( item_location &loc, bool interactive )
{
    if( !loc || !is_worn( *loc ) ) {
        if( interactive ) {
            add_msg_player_or_npc( m_info,
                                   _( "You are not wearing that item." ),
                                   _( "<npcname> isn't wearing that item." ) );
        }
        return false;
    }

    return change_side( *loc, interactive );
}

static void layer_item( std::map<bodypart_id, encumbrance_data> &vals, const item &it,
                        std::map<bodypart_id, layer_level> &highest_layer_so_far, bool power_armor, const Character &c )
{
    body_part_set covered_parts = it.get_covered_body_parts();
    for( const bodypart_id &bp : c.get_all_body_parts() ) {
        if( !covered_parts.test( bp.id() ) ) {
            continue;
        }

        const layer_level item_layer = it.get_layer();
        int encumber_val = it.get_encumber( c, bp.id() );
        int layering_encumbrance = clamp( encumber_val, 2, 10 );

        /*
         * Setting layering_encumbrance to 0 at this point makes the item cease to exist
         * for the purposes of the layer penalty system. (normally an item has a minimum
         * layering_encumbrance of 2 )
         */
        if( it.has_flag( flag_SEMITANGIBLE ) ) {
            encumber_val = 0;
            layering_encumbrance = 0;
        }

        const int armorenc = !power_armor || !it.is_power_armor() ?
                             encumber_val : std::max( 0, encumber_val - 40 );

        highest_layer_so_far[bp] = std::max( highest_layer_so_far[bp], item_layer );

        // Apply layering penalty to this layer, as well as any layer worn
        // within it that would normally be worn outside of it.
        for( layer_level penalty_layer = item_layer;
             penalty_layer <= highest_layer_so_far[bp]; ++penalty_layer ) {
            vals[bp].layer( penalty_layer, layering_encumbrance );
        }

        vals[bp].armor_encumbrance += armorenc;
    }
}

bool Character::is_wearing_power_armor( bool *hasHelmet ) const
{
    bool result = false;
    for( const item &elem : worn ) {
        if( !elem.is_power_armor() ) {
            continue;
        }
        if( hasHelmet == nullptr ) {
            // found power armor, helmet not requested, cancel loop
            return true;
        }
        // found power armor, continue search for helmet
        result = true;
        if( elem.covers( body_part_head ) ) {
            *hasHelmet = true;
            return true;
        }
    }
    return result;
}

bool Character::is_wearing_active_power_armor() const
{
    for( const item &w : worn ) {
        if( w.is_power_armor() && w.active ) {
            return true;
        }
    }
    return false;
}

bool Character::is_wearing_active_optcloak() const
{
    for( const item &w : worn ) {
        if( w.active && w.has_flag( flag_ACTIVE_CLOAKING ) ) {
            return true;
        }
    }
    return false;
}

bool Character::in_climate_control()
{
    bool regulated_area = false;
    // Check
    if( has_active_bionic( bio_climate ) ) {
        return true;
    }
    map &here = get_map();
    if( has_trait( trait_M_SKIN3 ) && here.has_flag_ter_or_furn( "FUNGUS", pos() ) &&
        in_sleep_state() ) {
        return true;
    }
    for( const item &w : worn ) {
        if( w.active && w.is_power_armor() ) {
            return true;
        }
        if( worn_with_flag( flag_CLIMATE_CONTROL ) ) {
            return true;
        }
    }
    if( calendar::turn >= next_climate_control_check ) {
        // save CPU and simulate acclimation.
        next_climate_control_check = calendar::turn + 20_turns;
        if( const optional_vpart_position vp = here.veh_at( pos() ) ) {
            // TODO: (?) Force player to scrounge together an AC unit
            regulated_area = (
                                 vp->is_inside() &&  // Already checks for opened doors
                                 vp->vehicle().total_power_w( true ) > 0 // Out of gas? No AC for you!
                             );
        }
        // TODO: AC check for when building power is implemented
        last_climate_control_ret = regulated_area;
        if( !regulated_area ) {
            // Takes longer to cool down / warm up with AC, than it does to step outside and feel cruddy.
            next_climate_control_check += 40_turns;
        }
    } else {
        return last_climate_control_ret;
    }
    return regulated_area;
}

int Character::get_wind_resistance( const bodypart_id &bp ) const
{
    int coverage = 0;
    float totalExposed = 1.0f;
    int totalCoverage = 0;
    int penalty = 100;

    for( const item &i : worn ) {
        if( i.covers( bp ) ) {
            if( i.made_of( material_id( "leather" ) ) || i.made_of( material_id( "plastic" ) ) ||
                i.made_of( material_id( "bone" ) ) ||
                i.made_of( material_id( "chitin" ) ) || i.made_of( material_id( "nomex" ) ) ) {
                penalty = 10; // 90% effective
            } else if( i.made_of( material_id( "cotton" ) ) ) {
                penalty = 30;
            } else if( i.made_of( material_id( "wool" ) ) ) {
                penalty = 40;
            } else {
                penalty = 1; // 99% effective
            }

            coverage = std::max( 0, i.get_coverage( bp ) - penalty );
            totalExposed *= ( 1.0 - coverage / 100.0 ); // Coverage is between 0 and 1?
        }
    }

    // Your shell provides complete wind protection if you're inside it
    if( has_active_mutation( trait_SHELL2 ) ) {
        totalCoverage = 100;
        return totalCoverage;
    }

    totalCoverage = 100 - totalExposed * 100;

    return totalCoverage;
}

void layer_details::reset()
{
    *this = layer_details();
}

// The stacking penalty applies by doubling the encumbrance of
// each item except the highest encumbrance one.
// So we add them together and then subtract out the highest.
int layer_details::layer( const int encumbrance )
{
    /*
     * We should only get to this point with an encumbrance value of 0
     * if the item is 'semitangible'. A normal item has a minimum of
     * 2 encumbrance for layer penalty purposes.
     * ( even if normally its encumbrance is 0 )
     */
    if( encumbrance == 0 ) {
        return total; // skip over the other logic because this item doesn't count
    }

    pieces.push_back( encumbrance );

    int current = total;
    if( encumbrance > max ) {
        total += max;   // *now* the old max is counted, just ignore the new max
        max = encumbrance;
    } else {
        total += encumbrance;
    }
    return total - current;
}

std::list<item>::iterator Character::position_to_wear_new_item( const item &new_item )
{
    // By default we put this item on after the last item on the same or any
    // lower layer.
    return std::find_if(
               worn.rbegin(), worn.rend(),
    [&]( const item & w ) {
        return w.get_layer() <= new_item.get_layer();
    }
           ).base();
}

/*
 * Encumbrance logic:
 * Some clothing is intrinsically encumbering, such as heavy jackets, backpacks, body armor, etc.
 * These simply add their encumbrance value to each body part they cover.
 * In addition, each article of clothing after the first in a layer imposes an additional penalty.
 * e.g. one shirt will not encumber you, but two is tight and starts to restrict movement.
 * Clothes on separate layers don't interact, so if you wear e.g. a light jacket over a shirt,
 * they're intended to be worn that way, and don't impose a penalty.
 * The default is to assume that clothes do not fit, clothes that are "fitted" either
 * reduce the encumbrance penalty by ten, or if that is already 0, they reduce the layering effect.
 *
 * Use cases:
 * What would typically be considered normal "street clothes" should not be considered encumbering.
 * T-shirt, shirt, jacket on torso/arms, underwear and pants on legs, socks and shoes on feet.
 * This is currently handled by each of these articles of clothing
 * being on a different layer and/or body part, therefore accumulating no encumbrance.
 */
void Character::item_encumb( std::map<bodypart_id, encumbrance_data> &vals,
                             const item &new_item ) const
{

    // reset all layer data
    vals = std::map<bodypart_id, encumbrance_data>();

    // Figure out where new_item would be worn
    std::list<item>::const_iterator new_item_position = worn.end();
    if( !new_item.is_null() ) {
        // const_cast required to work around g++-4.8 library bug
        // see the commit that added this comment to understand why
        new_item_position =
            const_cast<Character *>( this )->position_to_wear_new_item( new_item );
    }

    // Track highest layer observed so far so we can penalize out-of-order
    // items
    std::map<bodypart_id, layer_level> highest_layer_so_far;

    const bool power_armored = is_wearing_active_power_armor();
    for( auto w_it = worn.begin(); w_it != worn.end(); ++w_it ) {
        if( w_it == new_item_position ) {
            layer_item( vals, new_item, highest_layer_so_far, power_armored, *this );
        }
        layer_item( vals, *w_it, highest_layer_so_far, power_armored, *this );
    }

    if( worn.end() == new_item_position && !new_item.is_null() ) {
        layer_item( vals, new_item, highest_layer_so_far, power_armored, *this );
    }

    // make sure values are sane
    for( const bodypart_id &bp : get_all_body_parts() ) {
        encumbrance_data &elem = vals[bp];

        elem.armor_encumbrance = std::max( 0, elem.armor_encumbrance );

        // Add armor and layering penalties for the final values
        elem.encumbrance += elem.armor_encumbrance + elem.layer_penalty;
    }
}

int Character::encumb( const bodypart_id &bp ) const
{
    return get_part_encumbrance_data( bp ).encumbrance;
}

void Character::apply_mut_encumbrance( std::map<bodypart_id, encumbrance_data> &vals ) const
{
    const std::vector<trait_id> all_muts = get_mutations();
    std::map<bodypart_str_id, float> total_enc;

    // Lower penalty for bps covered only by XL armor
    // Initialized on demand for performance reasons:
    // (calculation is costly, most of players and npcs are don't have encumbering mutations)
    cata::optional<body_part_set> oversize;

    for( const trait_id &mut : all_muts ) {
        for( const std::pair<const bodypart_str_id, int> &enc : mut->encumbrance_always ) {
            total_enc[enc.first] += enc.second;
        }
        for( const std::pair<const bodypart_str_id, int> &enc : mut->encumbrance_covered ) {
            if( !oversize ) {
                // initialize on demand
                oversize = exclusive_flag_coverage( flag_OVERSIZE );
            }
            if( !oversize->test( enc.first ) ) {
                total_enc[enc.first] += enc.second;
            }
        }
    }

    for( const trait_id &mut : all_muts ) {
        for( const std::pair<const bodypart_str_id, float> &enc : mut->encumbrance_multiplier_always ) {
            total_enc[enc.first] *= enc.second;
        }
    }

    for( const std::pair<const bodypart_str_id, float> &enc : total_enc ) {
        vals[enc.first.id()].encumbrance += enc.second;
    }
}

void Character::mut_cbm_encumb( std::map<bodypart_id, encumbrance_data> &vals ) const
{

    for( const bionic_id &bid : get_bionics() ) {
        for( const std::pair<const bodypart_str_id, int> &element : bid->encumbrance ) {
            vals[element.first.id()].encumbrance += element.second;
        }
    }

    if( has_active_bionic( bio_shock_absorber ) ) {
        for( std::pair<const bodypart_id, encumbrance_data> &val : vals ) {
            val.second.encumbrance += 3; // Slight encumbrance to all parts except eyes
        }
        vals[body_part_eyes].encumbrance -= 3;
    }

    apply_mut_encumbrance( vals );
}

body_part_set Character::exclusive_flag_coverage( const flag_id &flag ) const
{
    body_part_set ret;
    ret.fill( get_all_body_parts() );

    for( const item &elem : worn ) {
        if( !elem.has_flag( flag ) ) {
            // Unset the parts covered by this item
            ret.substract_set( elem.get_covered_body_parts() );
        }
    }

    return ret;
}

/*
 * Innate stats getters
 */

// get_stat() always gets total (current) value, NEVER just the base
// get_stat_bonus() is always just the bonus amount
int Character::get_str() const
{
    return std::max( 0, get_str_base() + str_bonus );
}
int Character::get_dex() const
{
    return std::max( 0, get_dex_base() + dex_bonus );
}
int Character::get_per() const
{
    return std::max( 0, get_per_base() + per_bonus );
}
int Character::get_int() const
{
    return std::max( 0, get_int_base() + int_bonus );
}

int Character::get_str_base() const
{
    return str_max;
}
int Character::get_dex_base() const
{
    return dex_max;
}
int Character::get_per_base() const
{
    return per_max;
}
int Character::get_int_base() const
{
    return int_max;
}

int Character::get_str_bonus() const
{
    return str_bonus;
}
int Character::get_dex_bonus() const
{
    return dex_bonus;
}
int Character::get_per_bonus() const
{
    return per_bonus;
}
int Character::get_int_bonus() const
{
    return int_bonus;
}

static int get_speedydex_bonus( const int dex )
{
    static const std::string speedydex_min_dex( "SPEEDYDEX_MIN_DEX" );
    static const std::string speedydex_dex_speed( "SPEEDYDEX_DEX_SPEED" );
    // this is the number to be multiplied by the increment
    const int modified_dex = std::max( dex - get_option<int>( speedydex_min_dex ), 0 );
    return modified_dex * get_option<int>( speedydex_dex_speed );
}

int Character::get_speed() const
{
    if( has_trait_flag( json_flag_STEADY ) ) {
        return get_speed_base() + std::max( 0, get_speed_bonus() ) + std::max( 0,
                get_speedydex_bonus( get_dex() ) );
    }
    return Creature::get_speed() + get_speedydex_bonus( get_dex() );
}

int Character::ranged_dex_mod() const
{
    ///\EFFECT_DEX <20 increases ranged penalty
    return std::max( ( 20.0 - get_dex() ) * 0.5, 0.0 );
}

int Character::ranged_per_mod() const
{
    ///\EFFECT_PER <20 increases ranged aiming penalty.
    return std::max( ( 20.0 - get_per() ) * 1.2, 0.0 );
}

int Character::get_healthy() const
{
    return healthy;
}
int Character::get_healthy_mod() const
{
    return healthy_mod;
}

/*
 * Innate stats setters
 */

void Character::set_str_bonus( int nstr )
{
    str_bonus = nstr;
    str_cur = std::max( 0, str_max + str_bonus );
}
void Character::set_dex_bonus( int ndex )
{
    dex_bonus = ndex;
    dex_cur = std::max( 0, dex_max + dex_bonus );
}
void Character::set_per_bonus( int nper )
{
    per_bonus = nper;
    per_cur = std::max( 0, per_max + per_bonus );
}
void Character::set_int_bonus( int nint )
{
    int_bonus = nint;
    int_cur = std::max( 0, int_max + int_bonus );
}
void Character::mod_str_bonus( int nstr )
{
    str_bonus += nstr;
    str_cur = std::max( 0, str_max + str_bonus );
}
void Character::mod_dex_bonus( int ndex )
{
    dex_bonus += ndex;
    dex_cur = std::max( 0, dex_max + dex_bonus );
}
void Character::mod_per_bonus( int nper )
{
    per_bonus += nper;
    per_cur = std::max( 0, per_max + per_bonus );
}
void Character::mod_int_bonus( int nint )
{
    int_bonus += nint;
    int_cur = std::max( 0, int_max + int_bonus );
}

void Character::print_health() const
{
    if( !is_player() ) {
        return;
    }
    int current_health = get_healthy();
    if( has_trait( trait_SELFAWARE ) ) {
        add_msg_if_player( _( "Your current health value is %d." ), current_health );
    }

    if( current_health > 0 &&
        ( has_effect( effect_common_cold ) || has_effect( effect_flu ) ) ) {
        return;
    }

    static const std::map<int, std::string> msg_categories = {
        { -100, "health_horrible" },
        { -50, "health_very_bad" },
        { -10, "health_bad" },
        { 10, "" },
        { 50, "health_good" },
        { 100, "health_very_good" },
        { INT_MAX, "health_great" }
    };

    auto iter = msg_categories.lower_bound( current_health );
    if( iter != msg_categories.end() && !iter->second.empty() ) {
        const translation msg = SNIPPET.random_from_category( iter->second ).value_or( translation() );
        add_msg_if_player( current_health > 0 ? m_good : m_bad, "%s", msg );
    }
}

namespace io
{
template<>
std::string enum_to_string<character_stat>( character_stat data )
{
    switch( data ) {
        // *INDENT-OFF*
    case character_stat::STRENGTH:     return "STR";
    case character_stat::DEXTERITY:    return "DEX";
    case character_stat::INTELLIGENCE: return "INT";
    case character_stat::PERCEPTION:   return "PER";

        // *INDENT-ON*
        case character_stat::DUMMY_STAT:
            break;
    }
    abort();
}
} // namespace io

void Character::set_healthy( int nhealthy )
{
    healthy = nhealthy;
}
void Character::mod_healthy( int nhealthy )
{
    float mut_rate = 1.0f;
    for( const trait_id &mut : get_mutations() ) {
        mut_rate *= mut.obj().healthy_rate;
    }
    healthy += nhealthy * mut_rate;
}
void Character::set_healthy_mod( int nhealthy_mod )
{
    healthy_mod = nhealthy_mod;
}
void Character::mod_healthy_mod( int nhealthy_mod, int cap )
{
    // TODO: This really should be a full morale-like system, with per-effect caps
    //       and durations.  This version prevents any single effect from exceeding its
    //       intended ceiling, but multiple effects will overlap instead of adding.

    // Cap indicates how far the mod is allowed to shift in this direction.
    // It can have a different sign to the mod, e.g. for items that treat
    // extremely low health, but can't make you healthy.
    if( nhealthy_mod == 0 || cap == 0 ) {
        return;
    }
    int low_cap;
    int high_cap;
    if( nhealthy_mod < 0 ) {
        low_cap = cap;
        high_cap = 200;
    } else {
        low_cap = -200;
        high_cap = cap;
    }

    // If we're already out-of-bounds, we don't need to do anything.
    if( ( healthy_mod <= low_cap && nhealthy_mod < 0 ) ||
        ( healthy_mod >= high_cap && nhealthy_mod > 0 ) ) {
        return;
    }

    healthy_mod += nhealthy_mod;

    // Since we already bailed out if we were out-of-bounds, we can
    // just clamp to the boundaries here.
    healthy_mod = std::min( healthy_mod, high_cap );
    healthy_mod = std::max( healthy_mod, low_cap );
}

int Character::get_stored_kcal() const
{
    return stored_calories / 1000;
}

static std::string exert_lvl_to_str( float level )
{
    if( level <= NO_EXERCISE ) {
        return _( "NO_EXERCISE" );
    } else if( level <= LIGHT_EXERCISE ) {
        return _( "LIGHT_EXERCISE" );
    } else if( level <= MODERATE_EXERCISE ) {
        return _( "MODERATE_EXERCISE" );
    } else if( level <= BRISK_EXERCISE ) {
        return _( "BRISK_EXERCISE" );
    } else if( level <= ACTIVE_EXERCISE ) {
        return _( "ACTIVE_EXERCISE" );
    } else {
        return _( "EXTRA_EXERCISE" );
    }
}
std::string Character::debug_weary_info() const
{
    int amt = weariness();
    std::string max_act = exert_lvl_to_str( maximum_exertion_level() );
    float move_mult = exertion_adjusted_move_multiplier( EXTRA_EXERCISE );

    int bmr = base_bmr();
    int intake = weary.intake;
    int input = weary.tracker;
    int thresh = weary_threshold();
    int current = weariness_level();
    int morale = get_morale_level();
    int weight = units::to_gram<int>( bodyweight() );
    float bmi = get_bmi();

    return string_format( "Weariness: %s Max Full Exert: %s Mult: %g\nBMR: %d Intake: %d Tracker: %d Thresh: %d At: %d\nCal: %d/%d Fatigue: %d Morale: %d Wgt: %d (BMI %.1f)",
                          amt, max_act, move_mult, bmr, intake, input, thresh, current, get_stored_kcal(),
                          get_healthy_kcal(), fatigue, morale, weight, bmi );
}

void weariness_tracker::clear()
{
    tracker = 0;
    intake = 0;
    low_activity_ticks = 0;
    tick_counter = 0;
}

void Character::mod_stored_kcal( int nkcal, const bool ignore_weariness )
{
    const bool npc_no_food = is_npc() && get_option<bool>( "NO_NPC_FOOD" );
    if( !npc_no_food ) {
        mod_stored_calories( nkcal * 1000, ignore_weariness );
    }
}

void Character::mod_stored_calories( int ncal, const bool ignore_weariness )
{
    int nkcal = ncal / 1000;
    if( nkcal > 0 ) {
        add_gained_calories( nkcal );
        if( !ignore_weariness ) {
            weary.intake += nkcal;
        }
    } else {
        add_spent_calories( -nkcal );
        // nkcal is negative, we need positive
        if( !ignore_weariness ) {
            weary.tracker -= nkcal;
        }
    }
    set_stored_calories( stored_calories + ncal );
}

void Character::mod_stored_nutr( int nnutr )
{
    // nutr is legacy type code, this function simply converts old nutrition to new kcal
    mod_stored_kcal( -1 * std::round( nnutr * 2500.0f / ( 12 * 24 ) ) );
}

void Character::set_stored_kcal( int kcal )
{
    set_stored_calories( kcal * 1000 );
}

void Character::set_stored_calories( int cal )
{
    if( stored_calories != cal ) {
        stored_calories = cal;

        //some mutant change their max_hp according to their bmi
        recalc_hp();
    }
}

int Character::get_healthy_kcal() const
{
    return healthy_calories / 1000;
}

float Character::get_kcal_percent() const
{
    return static_cast<float>( get_stored_kcal() ) / static_cast<float>( get_healthy_kcal() );
}

int Character::get_hunger() const
{
    return hunger;
}

void Character::mod_hunger( int nhunger )
{
    const bool npc_no_food = is_npc() && get_option<bool>( "NO_NPC_FOOD" );
    if( !npc_no_food ) {
        set_hunger( hunger + nhunger );
    }
}

void Character::set_hunger( int nhunger )
{
    if( hunger != nhunger ) {
        // cap hunger at 300, just below famished
        hunger = std::min( 300, nhunger );
        on_stat_change( "hunger", hunger );
    }
}

// this is a translation from a legacy value
int Character::get_starvation() const
{
    static const std::vector<std::pair<float, float>> starv_thresholds = { {
            std::make_pair( 0.0f, 6000.0f ),
            std::make_pair( 0.8f, 300.0f ),
            std::make_pair( 0.95f, 100.0f )
        }
    };
    if( get_kcal_percent() < 0.95f ) {
        return std::round( multi_lerp( starv_thresholds, get_kcal_percent() ) );
    }
    return 0;
}

int Character::get_thirst() const
{
    return thirst;
}

std::pair<std::string, nc_color> Character::get_thirst_description() const
{
    // some delay from water in stomach is desired, but there needs to be some visceral response
    int thirst = get_thirst() - ( std::max( units::to_milliliter<int>( stomach.get_water() ) / 10,
                                            0 ) );
    std::string hydration_string;
    nc_color hydration_color = c_white;
    if( thirst > 520 ) {
        hydration_color = c_light_red;
        hydration_string = translate_marker( "Parched" );
    } else if( thirst > 240 ) {
        hydration_color = c_light_red;
        hydration_string = translate_marker( "Dehydrated" );
    } else if( thirst > 80 ) {
        hydration_color = c_yellow;
        hydration_string = translate_marker( "Very thirsty" );
    } else if( thirst > 40 ) {
        hydration_color = c_yellow;
        hydration_string = translate_marker( "Thirsty" );
    } else if( thirst < -60 ) {
        hydration_color = c_green;
        hydration_string = translate_marker( "Turgid" );
    } else if( thirst < -20 ) {
        hydration_color = c_green;
        hydration_string = translate_marker( "Hydrated" );
    } else if( thirst < 0 ) {
        hydration_color = c_green;
        hydration_string = translate_marker( "Slaked" );
    }
    return std::make_pair( _( hydration_string ), hydration_color );
}

std::pair<std::string, nc_color> Character::get_hunger_description() const
{
    // clang 3.8 has some sort of issue where if the initializer list contains const arguments,
    // like all of the effect_* string_id variables which are const string_id, then it fails to
    // initialize the array with tuples successfully complaining that
    // "chosen constructor is explicit in copy-initialization". Using std::forward_as_tuple
    // returns a tuple consisting of correctly implcitly copyable types.
    static const std::array<std::tuple<const efftype_id &, const char *, nc_color>, 9> hunger_states{ {
            std::forward_as_tuple( effect_hunger_engorged, translate_marker( "Engorged" ), c_red ),
            std::forward_as_tuple( effect_hunger_full, translate_marker( "Full" ), c_yellow ),
            std::forward_as_tuple( effect_hunger_satisfied, translate_marker( "Satisfied" ), c_green ),
            std::forward_as_tuple( effect_hunger_blank, "", c_white ),
            std::forward_as_tuple( effect_hunger_hungry, translate_marker( "Hungry" ), c_yellow ),
            std::forward_as_tuple( effect_hunger_very_hungry, translate_marker( "Very Hungry" ), c_yellow ),
            std::forward_as_tuple( effect_hunger_near_starving, translate_marker( "Near starving" ), c_red ),
            std::forward_as_tuple( effect_hunger_starving, translate_marker( "Starving!" ), c_red ),
            std::forward_as_tuple( effect_hunger_famished, translate_marker( "Famished" ), c_light_red )
        }
    };
    for( auto &hunger_state : hunger_states ) {
        if( has_effect( std::get<0>( hunger_state ) ) ) {
            return std::make_pair( _( std::get<1>( hunger_state ) ), std::get<2>( hunger_state ) );
        }
    }
    return std::make_pair( _( "ERROR!" ), c_light_red );
}

std::pair<std::string, nc_color> Character::get_fatigue_description() const
{
    int fatigue = get_fatigue();
    std::string fatigue_string;
    nc_color fatigue_color = c_white;
    if( fatigue > fatigue_levels::EXHAUSTED ) {
        fatigue_color = c_red;
        fatigue_string = translate_marker( "Exhausted" );
    } else if( fatigue > fatigue_levels::DEAD_TIRED ) {
        fatigue_color = c_light_red;
        fatigue_string = translate_marker( "Dead Tired" );
    } else if( fatigue > fatigue_levels::TIRED ) {
        fatigue_color = c_yellow;
        fatigue_string = translate_marker( "Tired" );
    }
    return std::make_pair( _( fatigue_string ), fatigue_color );
}

void Character::mod_thirst( int nthirst )
{
    if( has_flag( json_flag_NO_THIRST ) || ( is_npc() && get_option<bool>( "NO_NPC_FOOD" ) ) ) {
        return;
    }
    set_thirst( std::max( -100, thirst + nthirst ) );
}

void Character::set_thirst( int nthirst )
{
    if( thirst != nthirst ) {
        thirst = nthirst;
        on_stat_change( "thirst", thirst );
    }
}

void Character::mod_fatigue( int nfatigue )
{
    set_fatigue( fatigue + nfatigue );
}

void Character::mod_sleep_deprivation( int nsleep_deprivation )
{
    set_sleep_deprivation( sleep_deprivation + nsleep_deprivation );
}

void Character::set_fatigue( int nfatigue )
{
    nfatigue = std::max( nfatigue, -1000 );
    if( fatigue != nfatigue ) {
        fatigue = nfatigue;
        on_stat_change( "fatigue", fatigue );
    }
}

void Character::set_fatigue( fatigue_levels nfatigue )
{
    set_fatigue( static_cast<int>( nfatigue ) );
}

void Character::set_sleep_deprivation( int nsleep_deprivation )
{
    sleep_deprivation = std::min( static_cast< int >( SLEEP_DEPRIVATION_MASSIVE ), std::max( 0,
                                  nsleep_deprivation ) );
}

int Character::get_fatigue() const
{
    return fatigue;
}

int Character::get_sleep_deprivation() const
{
    return sleep_deprivation;
}

std::pair<std::string, nc_color> Character::get_pain_description() const
{
    const std::pair<std::string, nc_color> pain = Creature::get_pain_description();
    nc_color pain_color = pain.second;
    std::string pain_string;
    // get pain color
    if( get_perceived_pain() >= 60 ) {
        pain_color = c_red;
    } else if( get_perceived_pain() >= 40 ) {
        pain_color = c_light_red;
    }
    // get pain string
    if( ( has_trait( trait_SELFAWARE ) || has_effect( effect_got_checked ) ) &&
        get_perceived_pain() > 0 ) {
        pain_string = string_format( "%s %d", _( "Pain " ), get_perceived_pain() );
    } else if( get_perceived_pain() > 0 ) {
        pain_string = pain.first;
    }
    return std::make_pair( pain_string, pain_color );
}

bool Character::is_deaf() const
{
    return get_effect_int( effect_deaf ) > 2 || worn_with_flag( flag_DEAF ) ||
           has_flag( json_flag_DEAF ) ||
           ( has_trait( trait_M_SKIN3 ) && get_map().has_flag_ter_or_furn( "FUNGUS", pos() )
             && in_sleep_state() );
}

bool Character::is_mute() const
{
    return get_effect_int( effect_mute ) || worn_with_flag( flag_MUTE ) ||
           ( has_trait( trait_PROF_FOODP ) && !( is_wearing( itype_id( "foodperson_mask" ) ) ||
                   is_wearing( itype_id( "foodperson_mask_on" ) ) ) ) ||
           has_trait( trait_MUTE );
}
void Character::on_damage_of_type( int adjusted_damage, damage_type type, const bodypart_id &bp )
{
    // Electrical damage has a chance to temporarily incapacitate bionics in the damaged body_part.
    if( type == damage_type::ELECTRIC ) {
        const time_duration min_disable_time = 10_turns * adjusted_damage;
        for( bionic &i : *my_bionics ) {
            if( !i.powered ) {
                // Unpowered bionics are protected from power surges.
                continue;
            }
            const auto &info = i.info();
            if( info.has_flag( STATIC( json_character_flag( "BIONIC_SHOCKPROOF" ) ) ) ||
                info.has_flag( STATIC( json_character_flag( "BIONIC_FAULTY" ) ) ) ) {
                continue;
            }
            const std::map<bodypart_str_id, size_t> &bodyparts = info.occupied_bodyparts;
            if( bodyparts.find( bp.id() ) != bodyparts.end() ) {
                const int bp_hp = get_part_hp_cur( bp );
                // The chance to incapacitate is as high as 50% if the attack deals damage equal to one third of the body part's current health.
                if( x_in_y( adjusted_damage * 3, bp_hp ) && one_in( 2 ) ) {
                    if( i.incapacitated_time == 0_turns ) {
                        add_msg_if_player( m_bad, _( "Your %s bionic shorts out!" ), info.name );
                    }
                    i.incapacitated_time += rng( min_disable_time, 10 * min_disable_time );
                }
            }
        }
    }
}

void Character::reset_bonuses()
{
    // Reset all bonuses to 0 and multipliers to 1.0
    str_bonus = 0;
    dex_bonus = 0;
    per_bonus = 0;
    int_bonus = 0;

    Creature::reset_bonuses();
}

int Character::get_max_healthy() const
{
    const float bmi = get_bmi();
    return clamp( static_cast<int>( std::round( -3 * ( bmi - character_weight_category::normal ) *
                                    ( bmi - character_weight_category::overweight ) + 200 ) ), -200, 200 );
}

void Character::regen( int rate_multiplier )
{
    int pain_ticks = rate_multiplier;
    while( get_pain() > 0 && pain_ticks-- > 0 ) {
        mod_pain( -roll_remainder( 0.2f + get_pain() / 50.0f ) );
    }

    float rest = rest_quality();
    float heal_rate = healing_rate( rest ) * to_turns<int>( 5_minutes );
    if( heal_rate > 0.0f ) {
        healall( roll_remainder( rate_multiplier * heal_rate ) );
    } else if( heal_rate < 0.0f ) {
        int rot_rate = roll_remainder( rate_multiplier * -heal_rate );
        // Has to be in loop because some effects depend on rounding
        while( rot_rate-- > 0 ) {
            hurtall( 1, nullptr, false );
        }
    }

    // include healing effects
    for( const bodypart_id &bp : get_all_body_parts( get_body_part_flags::only_main ) ) {
        float healing = healing_rate_medicine( rest, bp ) * to_turns<int>( 5_minutes );
        int healing_apply = roll_remainder( healing );
        mod_part_healed_total( bp, healing_apply );
        heal( bp, healing_apply );
        if( get_part_damage_bandaged( bp ) > 0 ) {
            mod_part_damage_bandaged( bp, -healing_apply );
            if( get_part_damage_bandaged( bp ) <= 0 ) {
                set_part_damage_bandaged( bp, 0 );
                remove_effect( effect_bandaged, bp );
                add_msg_if_player( _( "Bandaged wounds on your %s healed." ), body_part_name( bp ) );
            }
        }
        if( get_part_damage_disinfected( bp ) > 0 ) {
            mod_part_damage_disinfected( bp, -healing_apply );
            if( get_part_damage_disinfected( bp ) <= 0 ) {
                set_part_damage_disinfected( bp, 0 );
                remove_effect( effect_disinfected, bp );
                add_msg_if_player( _( "Disinfected wounds on your %s healed." ), body_part_name( bp ) );
            }
        }

        // remove effects if the limb was healed by other way
        if( has_effect( effect_bandaged, bp.id() ) && ( is_part_at_max_hp( bp ) ) ) {
            set_part_damage_bandaged( bp, 0 );
            remove_effect( effect_bandaged, bp );
            add_msg_if_player( _( "Bandaged wounds on your %s healed." ), body_part_name( bp ) );
        }
        if( has_effect( effect_disinfected, bp.id() ) && ( is_part_at_max_hp( bp ) ) ) {
            set_part_damage_disinfected( bp, 0 );
            remove_effect( effect_disinfected, bp );
            add_msg_if_player( _( "Disinfected wounds on your %s healed." ), body_part_name( bp ) );
        }
    }

    if( get_rad() > 0 ) {
        mod_rad( -roll_remainder( rate_multiplier / 50.0f ) );
    }
}

void Character::enforce_minimum_healing()
{
    for( const bodypart_id &bp : get_all_body_parts() ) {
        if( get_part_healed_total( bp ) <= 0 ) {
            heal( bp, 1 );
        }
        set_part_healed_total( bp, 0 );
    }
}

void Character::update_health( int external_modifiers )
{
    // Limit healthy_mod to [-200, 200].
    // This also sets approximate bounds for the character's health.
    if( get_healthy_mod() > get_max_healthy() ) {
        set_healthy_mod( get_max_healthy() );
    } else if( get_healthy_mod() < -200 ) {
        set_healthy_mod( -200 );
    }

    // Active leukocyte breeder will keep your health near 100
    int effective_healthy_mod = get_healthy_mod();
    if( has_active_bionic( bio_leukocyte ) ) {
        // Side effect: dependency
        mod_healthy_mod( -50, -200 );
        effective_healthy_mod = 100;
    }

    // Health tends toward healthy_mod.
    // For small differences, it changes 4 points per day
    // For large ones, up to ~40% of the difference per day
    int health_change = effective_healthy_mod - get_healthy() + external_modifiers;
    mod_healthy( sgn( health_change ) * std::max( 1, std::abs( health_change ) / 10 ) );

    // And healthy_mod decays over time.
    // Slowly near 0, but it's hard to overpower it near +/-100
    set_healthy_mod( std::round( get_healthy_mod() * 0.95f ) );

    add_msg_debug( "Health: %d, Health mod: %d", get_healthy(), get_healthy_mod() );
}

// Returns the number of multiples of tick_length we would "pass" on our way `from` to `to`
// For example, if `tick_length` is 1 hour, then going from 0:59 to 1:01 should return 1
static inline int ticks_between( const time_point &from, const time_point &to,
                                 const time_duration &tick_length )
{
    return ( to_turn<int>( to ) / to_turns<int>( tick_length ) ) - ( to_turn<int>
            ( from ) / to_turns<int>( tick_length ) );
}

void Character::update_body()
{
    update_body( calendar::turn - 1_turns, calendar::turn );
}

void Character::update_body( const time_point &from, const time_point &to )
{
    if( !is_npc() ) {
        update_stamina( to_turns<int>( to - from ) );
    }
    update_stomach( from, to );
    recalculate_enchantment_cache();
    if( ticks_between( from, to, 3_minutes ) > 0 ) {
        magic->update_mana( *this->as_player(), to_turns<float>( 3_minutes ) );
    }
    const int five_mins = ticks_between( from, to, 5_minutes );
    if( five_mins > 0 ) {
        try_reduce_weariness( attempted_activity_level );
        if( !activity.is_null() ) {
            decrease_activity_level( activity.exertion_level() );
        } else {
            reset_activity_level();
        }
        check_needs_extremes();
        update_needs( five_mins );
        regen( five_mins );
        // Note: mend ticks once per 5 minutes, but wants rate in TURNS, not 5 minute intervals
        // TODO: change @ref med to take time_duration
        mend( five_mins * to_turns<int>( 5_minutes ) );
    }
    if( ticks_between( from, to, 24_hours ) > 0 && !has_flag( json_flag_NO_MINIMAL_HEALING ) ) {
        enforce_minimum_healing();
    }

    const int thirty_mins = ticks_between( from, to, 30_minutes );
    if( thirty_mins > 0 ) {
        // Radiation kills health even at low doses
        update_health( has_trait( trait_RADIOGENIC ) ? 0 : -get_rad() );
        get_sick();
    }

    for( const auto &v : vitamin::all() ) {
        const time_duration rate = vitamin_rate( v.first );

        // No blood volume regeneration if body lacks fluids
        if( v.first == vitamin_blood && has_effect( effect_hypovolemia ) && get_thirst() > 240 ) {
            continue;
        }

        if( rate > 0_turns ) {
            int qty = ticks_between( from, to, rate );
            if( qty > 0 ) {
                vitamin_mod( v.first, 0 - qty );
            }

        } else if( rate < 0_turns ) {
            // mutations can result in vitamins being generated (but never accumulated)
            int qty = ticks_between( from, to, -rate );
            if( qty > 0 ) {
                vitamin_mod( v.first, qty );
            }
        }
    }

    if( is_avatar() && ticks_between( from, to, 24_hours ) > 0 ) {
        as_avatar()->advance_daily_calories();
    }

    do_skill_rust();
}

item *Character::best_quality_item( const quality_id &qual )
{
    std::vector<item *> qual_inv = items_with( [qual]( const item & itm ) {
        return itm.has_quality( qual );
    } );
    item *best_qual = random_entry( qual_inv );
    for( item *elem : qual_inv ) {
        if( elem->get_quality( qual ) > best_qual->get_quality( qual ) ) {
            best_qual = elem;
        }
    }
    return best_qual;
}

int Character::weary_threshold() const
{
    const int bmr = base_bmr();
    int threshold = bmr * get_option<float>( "WEARY_BMR_MULT" );
    // reduce by 1% per 14 points of fatigue after 150 points
    threshold *= 1.0f - ( ( std::max( fatigue, -20 ) - 150 ) / 1400.0f );
    // Each 2 points of morale increase or decrease by 1%
    threshold *= 1.0f + ( get_morale_level() / 200.0f );
    // TODO: Hunger effects this

    return std::max( threshold, bmr / 10 );
}

int Character::weariness() const
{
    if( weary.intake > weary.tracker ) {
        return weary.tracker * 0.5;
    }
    return weary.tracker - weary.intake * 0.5;
}

std::pair<int, int> Character::weariness_transition_progress() const
{
    // Mostly a duplicate of the below function. No real way to clean this up
    int amount = weariness();
    int threshold = weary_threshold();
    amount -= threshold * get_option<float>( "WEARY_INITIAL_STEP" );
    while( amount >= 0 ) {
        amount -= threshold;
        if( threshold > 20 ) {
            threshold *= get_option<float>( "WEARY_THRESH_SCALING" );
        }
    }

    // If we return the absolute value of the amount, it will work better
    // Because as it decreases, we will approach a transition
    return {std::abs( amount ), threshold};
}

int Character::weariness_level() const
{
    int amount = weariness();
    int threshold = weary_threshold();
    int level = 0;
    amount -= threshold * get_option<float>( "WEARY_INITIAL_STEP" );
    while( amount >= 0 ) {
        amount -= threshold;
        if( threshold > 20 ) {
            threshold *= get_option<float>( "WEARY_THRESH_SCALING" );
        }
        ++level;
    }

    return level;
}

float Character::maximum_exertion_level() const
{
    switch( weariness_level() ) {
        case 0:
            return EXTRA_EXERCISE;
        case 1:
            return ACTIVE_EXERCISE;
        case 2:
            return BRISK_EXERCISE;
        case 3:
            return MODERATE_EXERCISE;
        case 4:
            return LIGHT_EXERCISE;
        case 5:
        default:
            return NO_EXERCISE;
    }
}

float Character::exertion_adjusted_move_multiplier( float level ) const
{
    // The default value for level is -1.0
    // And any values we get that are negative or 0
    // will cause incorrect behavior
    if( level <= 0 ) {
        level = attempted_activity_level;
    }
    const float max = maximum_exertion_level();
    if( level < max ) {
        return 1.0f;
    }
    return max / level;
}

// Called every 5 minutes, when activity level is logged
void Character::try_reduce_weariness( const float exertion )
{
    weary.tick_counter++;
    if( exertion == NO_EXERCISE ) {
        weary.low_activity_ticks++;
        // Recover twice as fast at rest
        if( in_sleep_state() ) {
            weary.low_activity_ticks++;
        }
    }

    const float recovery_mult = get_option<float>( "WEARY_RECOVERY_MULT" );

    if( weary.low_activity_ticks >= 6 ) {
        int reduction = weary.tracker;
        const int bmr = base_bmr();
        // 1/20 of whichever's bigger
        if( bmr > reduction ) {
            reduction = bmr * recovery_mult;
        } else {
            reduction *= recovery_mult;
        }
        weary.low_activity_ticks = 0;

        weary.tracker -= reduction;
    }

    if( weary.tick_counter >= 12 ) {
        weary.intake *= 1 - recovery_mult;
        weary.tick_counter = 0;
    }

    // Normalize values, make sure we stay above 0
    weary.intake = std::max( weary.intake, 0 );
    weary.tracker = std::max( weary.tracker, 0 );
    weary.tick_counter = std::max( weary.tick_counter, 0 );
    weary.low_activity_ticks = std::max( weary.low_activity_ticks, 0 );
}

// Remove all this instantaneous stuff when activity tracking moves to per turn
float Character::instantaneous_activity_level() const
{
    // As this is for display purposes, we want to show last turn's activity.
    if( calendar::turn > act_turn ) {
        return act_cursor;
    } else {
        return last_act;
    }
}

// Basically, advance one turn
void Character::reset_activity_cursor()
{

    if( calendar::turn > act_turn ) {
        last_act = act_cursor;
        act_cursor = NO_EXERCISE;
        act_turn = calendar::turn;
    } else {
        act_cursor = NO_EXERCISE;
    }
}

// Log the highest activity level for this turn, and advance one turn if needed
void Character::log_instant_activity( float level )
{
    if( calendar::turn > act_turn ) {
        reset_activity_cursor();
        act_cursor = level;
    } else if( level > act_cursor ) {
        act_cursor = level;
    }
}

float Character::activity_level() const
{
    float max = maximum_exertion_level();
    if( attempted_activity_level > max ) {
        return max;
    }
    return attempted_activity_level;
}

void Character::update_stomach( const time_point &from, const time_point &to )
{
    const needs_rates rates = calc_needs_rates();
    // No food/thirst/fatigue clock at all
    const bool debug_ls = has_trait( trait_DEBUG_LS );
    // No food/thirst, capped fatigue clock (only up to tired)
    const bool npc_no_food = is_npc() && get_option<bool>( "NO_NPC_FOOD" );
    const bool foodless = debug_ls || npc_no_food;
    const bool no_thirst = has_flag( json_flag_NO_THIRST );
    const bool mycus = has_trait( trait_M_DEPENDENT );
    const float kcal_per_time = get_bmr() / ( 12.0f * 24.0f );
    const int five_mins = ticks_between( from, to, 5_minutes );
    const int half_hours = ticks_between( from, to, 30_minutes );
    const units::volume stomach_capacity = stomach.capacity( *this );

    if( five_mins > 0 ) {
        // Digest nutrients in stomach, they are destined for the guts (except water)
        food_summary digested_to_guts = stomach.digest( *this, rates, five_mins, half_hours );
        // Digest nutrients in guts, they will be distributed to needs levels
        food_summary digested_to_body = guts.digest( *this, rates, five_mins, half_hours );
        // Water from stomach skips guts and gets absorbed by body
        mod_thirst( -units::to_milliliter<int>( digested_to_guts.water ) / 5 );
        guts.ingest( digested_to_guts );

        mod_stored_kcal( digested_to_body.nutr.kcal() );
        vitamins_mod( digested_to_body.nutr.vitamins, false );
        log_activity_level( activity_level() );

        if( !foodless && rates.hunger > 0.0f ) {
            mod_hunger( roll_remainder( rates.hunger * five_mins ) );
            // instead of hunger keeping track of how you're living, burn calories instead
            // Explicitly floor it here, the int cast will do so anyways
            mod_stored_calories( -std::floor( five_mins * kcal_per_time * 1000 ) );
        }
    }
    // if npc_no_food no need to calc hunger, and set hunger_effect
    if( npc_no_food ) {
        return;
    }
    if( stomach.time_since_ate() > 10_minutes ) {
        if( stomach.contains() >= stomach_capacity && get_hunger() > -61 ) {
            // you're engorged! your stomach is full to bursting!
            set_hunger( -61 );
        } else if( stomach.contains() >= stomach_capacity / 2 && get_hunger() > -21 ) {
            // full
            set_hunger( -21 );
        } else if( stomach.contains() >= stomach_capacity / 8 && get_hunger() > -1 ) {
            // that's really all the food you need to feel full
            set_hunger( -1 );
        } else if( stomach.contains() == 0_ml ) {
            if( guts.get_calories() == 0 && get_stored_kcal() < get_healthy_kcal() && get_hunger() < 300 ) {
                // there's no food except what you have stored in fat
                set_hunger( 300 );
            } else if( get_hunger() < 100 && ( ( guts.get_calories() == 0 &&
                                                 get_stored_kcal() >= get_healthy_kcal() ) || get_stored_kcal() < get_healthy_kcal() ) ) {
                set_hunger( 100 );
            } else if( get_hunger() < 0 ) {
                set_hunger( 0 );
            }
        }
    } else
        // you fill up when you eat fast, but less so than if you eat slow
        // if you just ate but your stomach is still empty it will still
        // delay your filling up (drugs?)
    {
        if( stomach.contains() >= stomach_capacity && get_hunger() > -61 ) {
            // you're engorged! your stomach is full to bursting!
            set_hunger( -61 );
        } else if( stomach.contains() >= stomach_capacity * 3 / 4 && get_hunger() > -21 ) {
            // full
            set_hunger( -21 );
        } else if( stomach.contains() >= stomach_capacity / 2 && get_hunger() > -1 ) {
            // that's really all the food you need to feel full
            set_hunger( -1 );
        } else if( stomach.contains() > 0_ml && get_kcal_percent() > 0.95 ) {
            // usually eating something cools your hunger
            set_hunger( 0 );
        }
    }

    if( !foodless && rates.thirst > 0.0f ) {
        mod_thirst( roll_remainder( rates.thirst * five_mins ) );
    }
    // Mycus and Metabolic Rehydration makes thirst unnecessary
    // since water is not limited by intake but by absorption, we can just set thirst to zero
    if( mycus || no_thirst ) {
        set_thirst( 0 );
    }

    const bool calorie_deficit = get_bmi() < character_weight_category::normal;
    const units::volume contains = stomach.contains();
    const units::volume cap = stomach.capacity( *this );

    efftype_id hunger_effect;
    // i ate just now!
    const bool just_ate = stomach.time_since_ate() < 15_minutes;
    // i ate a meal recently enough that i shouldn't need another meal
    const bool recently_ate = stomach.time_since_ate() < 3_hours;
    // Hunger effect should intensify whenever stomach contents decreases, last eaten time increases, or calorie deficit intensifies.
    if( calorie_deficit ) {
        //              just_ate    recently_ate
        //              <15 min     <3 hrs      >=3 hrs
        // >= cap       engorged    engorged    engorged
        // > 3/4 cap    full        full        full
        // > 1/2 cap    satisfied   v. hungry   famished/(near)starving
        // <= 1/2 cap   hungry      v. hungry   famished/(near)starving
        if( contains >= cap ) {
            hunger_effect = effect_hunger_engorged;
        } else if( contains > cap * 3 / 4 ) {
            hunger_effect = effect_hunger_full;
        } else if( just_ate && contains > cap / 2 ) {
            hunger_effect = effect_hunger_satisfied;
        } else if( just_ate ) {
            hunger_effect = effect_hunger_hungry;
        } else if( recently_ate ) {
            hunger_effect = effect_hunger_very_hungry;
        } else if( get_bmi() < character_weight_category::underweight ) {
            hunger_effect = effect_hunger_near_starving;
        } else if( get_bmi() < character_weight_category::emaciated ) {
            hunger_effect = effect_hunger_starving;
        } else {
            hunger_effect = effect_hunger_famished;
        }
    } else {
        //              just_ate    recently_ate
        //              <15 min     <3 hrs      >=3 hrs
        // >= 5/6 cap   engorged    engorged    engorged
        // > 11/20 cap  full        full        full
        // >= 3/8 cap   satisfied   satisfied   blank
        // > 0          blank       blank       blank
        // 0            blank       blank       (v.) hungry
        if( contains >= cap * 5 / 6 ) {
            hunger_effect = effect_hunger_engorged;
        } else if( contains > cap * 11 / 20 ) {
            hunger_effect = effect_hunger_full;
        } else if( recently_ate && contains >= cap * 3 / 8 ) {
            hunger_effect = effect_hunger_satisfied;
        } else if( recently_ate || contains > 0_ml ) {
            hunger_effect = effect_hunger_blank;
        } else if( get_bmi() > character_weight_category::overweight ) {
            hunger_effect = effect_hunger_hungry;
        } else {
            hunger_effect = effect_hunger_very_hungry;
        }
    }
    if( !has_effect( hunger_effect ) ) {
        remove_effect( effect_hunger_engorged );
        remove_effect( effect_hunger_full );
        remove_effect( effect_hunger_satisfied );
        remove_effect( effect_hunger_hungry );
        remove_effect( effect_hunger_very_hungry );
        remove_effect( effect_hunger_near_starving );
        remove_effect( effect_hunger_starving );
        remove_effect( effect_hunger_famished );
        remove_effect( effect_hunger_blank );
        add_effect( hunger_effect, 24_hours, true );
    }
}

void Character::update_needs( int rate_multiplier )
{
    const int current_stim = get_stim();
    // Hunger, thirst, & fatigue up every 5 minutes
    effect &sleep = get_effect( effect_sleep );
    // No food/thirst/fatigue clock at all
    const bool debug_ls = has_trait( trait_DEBUG_LS );
    // No food/thirst, capped fatigue clock (only up to tired)
    const bool npc_no_food = is_npc() && get_option<bool>( "NO_NPC_FOOD" );
    const bool asleep = !sleep.is_null();
    const bool lying = asleep || has_effect( effect_lying_down ) ||
                       activity.id() == ACT_TRY_SLEEP;

    needs_rates rates = calc_needs_rates();

    const bool wasnt_fatigued = get_fatigue() <= fatigue_levels::DEAD_TIRED;
    // Don't increase fatigue if sleeping or trying to sleep or if we're at the cap.
    if( get_fatigue() < 1050 && !asleep && !debug_ls ) {
        if( rates.fatigue > 0.0f ) {
            int fatigue_roll = roll_remainder( rates.fatigue * rate_multiplier );
            mod_fatigue( fatigue_roll );

            // Synaptic regen bionic stops SD while awake and boosts it while sleeping
            if( !has_flag( json_flag_STOP_SLEEP_DEPRIVATION ) ) {
                // fatigue_roll should be around 1 - so the counter increases by 1 every minute on average,
                // but characters who need less sleep will also get less sleep deprived, and vice-versa.

                // Note: Since needs are updated in 5-minute increments, we have to multiply the roll again by
                // 5. If rate_multiplier is > 1, fatigue_roll will be higher and this will work out.
                mod_sleep_deprivation( fatigue_roll * 5 );
            }

            if( npc_no_food && get_fatigue() > fatigue_levels::TIRED ) {
                set_fatigue( fatigue_levels::TIRED );
                set_sleep_deprivation( 0 );
            }
        }
    } else if( asleep ) {
        if( rates.recovery > 0.0f ) {
            int recovered = roll_remainder( rates.recovery * rate_multiplier );
            if( get_fatigue() - recovered < -20 ) {
                // Should be wake up, but that could prevent some retroactive regeneration
                sleep.set_duration( 1_turns );
                mod_fatigue( -25 );
            } else {
                if( has_effect( effect_disrupted_sleep ) || has_effect( effect_recently_coughed ) ) {
                    recovered *= .5;
                }
                mod_fatigue( -recovered );

                // Sleeping on the ground, no bionic = 1x rest_modifier
                // Sleeping on a bed, no bionic      = 2x rest_modifier
                // Sleeping on a comfy bed, no bionic= 3x rest_modifier

                // Sleeping on the ground, bionic    = 3x rest_modifier
                // Sleeping on a bed, bionic         = 6x rest_modifier
                // Sleeping on a comfy bed, bionic   = 9x rest_modifier
                float rest_modifier = ( has_flag( json_flag_STOP_SLEEP_DEPRIVATION ) ? 3 : 1 );
                // Melatonin supplements also add a flat bonus to recovery speed
                if( has_effect( effect_melatonin ) ) {
                    rest_modifier += 1;
                }

                const comfort_level comfort = base_comfort_value( pos() ).level;

                if( comfort >= comfort_level::very_comfortable ) {
                    rest_modifier *= 3;
                } else  if( comfort >= comfort_level::comfortable ) {
                    rest_modifier *= 2.5;
                } else if( comfort >= comfort_level::slightly_comfortable ) {
                    rest_modifier *= 2;
                }

                // If we're just tired, we'll get a decent boost to our sleep quality.
                // The opposite is true for very tired characters.
                if( get_fatigue() < fatigue_levels::DEAD_TIRED ) {
                    rest_modifier += 2;
                } else if( get_fatigue() >= fatigue_levels::EXHAUSTED ) {
                    rest_modifier = ( rest_modifier > 2 ) ? rest_modifier - 2 : 1;
                }

                // Recovered is multiplied by 2 as well, since we spend 1/3 of the day sleeping
                mod_sleep_deprivation( -rest_modifier * ( recovered * 2 ) );

            }
        }
    }
    if( is_player() && wasnt_fatigued && get_fatigue() > fatigue_levels::DEAD_TIRED && !lying ) {
        if( !activity ) {
            add_msg_if_player( m_warning, _( "You're feeling tired.  %s to lie down for sleep." ),
                               press_x( ACTION_SLEEP ) );
        } else {
            g->cancel_activity_query( _( "You're feeling tired." ) );
        }
    }

    if( current_stim < 0 ) {
        set_stim( std::min( current_stim + rate_multiplier, 0 ) );
    } else if( current_stim > 0 ) {
        set_stim( std::max( current_stim - rate_multiplier, 0 ) );
    }

    if( get_painkiller() > 0 ) {
        mod_painkiller( -std::min( get_painkiller(), rate_multiplier ) );
    }

    // Huge folks take penalties for cramming themselves in vehicles
    if( in_vehicle && get_size() == creature_size::huge &&
        !( has_trait( trait_NOPAIN ) || has_effect( effect_narcosis ) ) ) {
        vehicle *veh = veh_pointer_or_null( get_map().veh_at( pos() ) );
        // it's painful to work the controls, but passengers in open topped vehicles are fine
        if( veh && ( veh->enclosed_at( pos() ) || veh->player_in_control( *this->as_player() ) ) ) {
            add_msg_if_player( m_bad,
                               _( "You're cramping up from stuffing yourself in this vehicle." ) );
            if( is_npc() ) {
                npc &as_npc = dynamic_cast<npc &>( *this );
                as_npc.complain_about( "cramped_vehicle", 1_hours, "<cramped_vehicle>", false );
            }

            mod_pain( rng( 4, 6 ) );
            mod_focus( -1 );
        }
    }
}
needs_rates Character::calc_needs_rates() const
{
    const effect &sleep = get_effect( effect_sleep );
    const bool has_recycler = has_bionic( bio_recycler );
    const bool asleep = !sleep.is_null();

    needs_rates rates;
    rates.hunger = metabolic_rate();

    rates.kcal = get_bmr();

    add_msg_if_player( m_debug, "Metabolic rate: %.2f", rates.hunger );

    static const std::string player_thirst_rate( "PLAYER_THIRST_RATE" );
    rates.thirst = get_option< float >( player_thirst_rate );
    static const std::string thirst_modifier( "thirst_modifier" );
    rates.thirst *= 1.0f + mutation_value( thirst_modifier );
    if( worn_with_flag( flag_SLOWS_THIRST ) ) {
        rates.thirst *= 0.7f;
    }

    static const std::string player_fatigue_rate( "PLAYER_FATIGUE_RATE" );
    rates.fatigue = get_option< float >( player_fatigue_rate );
    static const std::string fatigue_modifier( "fatigue_modifier" );
    rates.fatigue *= 1.0f + mutation_value( fatigue_modifier );

    // Note: intentionally not in metabolic rate
    if( has_recycler ) {
        // Recycler won't help much with mutant metabolism - it is intended for human one
        rates.hunger = std::min( rates.hunger, std::max( 0.5f, rates.hunger - 0.5f ) );
        rates.thirst = std::min( rates.thirst, std::max( 0.5f, rates.thirst - 0.5f ) );
    }

    if( asleep ) {
        static const std::string fatigue_regen_modifier( "fatigue_regen_modifier" );
        rates.recovery = 1.0f + mutation_value( fatigue_regen_modifier );
        if( !is_hibernating() ) {
            // Hunger and thirst advance more slowly while we sleep. This is the standard rate.
            rates.hunger *= 0.5f;
            rates.thirst *= 0.5f;
            const int intense = sleep.is_null() ? 0 : sleep.get_intensity();
            // Accelerated recovery capped to 2x over 2 hours
            // After 16 hours of activity, equal to 7.25 hours of rest
            const int accelerated_recovery_chance = 24 - intense + 1;
            const float accelerated_recovery_rate = 1.0f / accelerated_recovery_chance;
            rates.recovery += accelerated_recovery_rate;
        } else {
            // Hunger and thirst advance *much* more slowly whilst we hibernate.
            rates.hunger *= ( 2.0f / 7.0f );
            rates.thirst *= ( 2.0f / 7.0f );
        }
        rates.recovery -= static_cast<float>( get_perceived_pain() ) / 60;

    } else {
        rates.recovery = 0;
    }

    if( has_activity( ACT_TREE_COMMUNION ) ) {
        // Much of the body's needs are taken care of by the trees.
        // Hair Roots don't provide any bodily needs.
        if( has_trait( trait_ROOTS2 ) || has_trait( trait_ROOTS3 ) ) {
            rates.hunger *= 0.5f;
            rates.thirst *= 0.5f;
            rates.fatigue *= 0.5f;
        }
    }

    if( has_trait( trait_TRANSPIRATION ) ) {
        // Transpiration, the act of moving nutrients with evaporating water, can take a very heavy toll on your thirst when it's really hot.
        rates.thirst *= ( ( get_weather().get_temperature( pos() ) - 32.5f ) / 40.0f );
    }

    if( is_npc() ) {
        rates.hunger *= 0.25f;
        rates.thirst *= 0.25f;
    }

    rates.fatigue = enchantment_cache->modify_value( enchant_vals::mod::FATIGUE, rates.fatigue );
    rates.thirst = enchantment_cache->modify_value( enchant_vals::mod::THIRST, rates.thirst );

    return rates;
}

item Character::reduce_charges( item *it, int quantity )
{
    if( !has_item( *it ) ) {
        debugmsg( "invalid item (name %s) for reduce_charges", it->tname() );
        return item();
    }
    if( it->charges <= quantity ) {
        return i_rem( it );
    }
    it->mod_charges( -quantity );
    item result( *it );
    result.charges = quantity;
    return result;
}

bool Character::can_interface_armor() const
{
    bool okay = std::any_of( my_bionics->begin(), my_bionics->end(),
    []( const bionic & b ) {
        return b.powered && b.info().has_flag( STATIC( json_character_flag( "BIONIC_ARMOR_INTERFACE" ) ) );
    } );
    return okay;
}

bool Character::has_mission_item( int mission_id ) const
{
    return mission_id != -1 && has_item_with( has_mission_item_filter{ mission_id } );
}

bool Character::has_gun_for_ammo( const ammotype &at ) const
{
    return has_item_with( [at]( const item & it ) {
        // item::ammo_type considers the active gunmod.
        return it.is_gun() && it.ammo_types().count( at );
    } );
}

bool Character::has_magazine_for_ammo( const ammotype &at ) const
{
    return has_item_with( [&at]( const item & it ) {
        return !it.has_flag( flag_NO_RELOAD ) &&
               ( ( it.is_magazine() && it.ammo_types().count( at ) ) ||
                 ( it.is_gun() && it.magazine_integral() && it.ammo_types().count( at ) ) ||
                 ( it.is_gun() && it.magazine_current() != nullptr &&
                   it.magazine_current()->ammo_types().count( at ) ) );
    } );
}

void Character::check_needs_extremes()
{
    // Check if we've overdosed... in any deadly way.
    if( get_stim() > 250 ) {
        add_msg_player_or_npc( m_bad,
                               _( "You have a sudden heart attack!" ),
                               _( "<npcname> has a sudden heart attack!" ) );
        get_event_bus().send<event_type::dies_from_drug_overdose>( getID(), efftype_id() );
        set_part_hp_cur( body_part_torso, 0 );
    } else if( get_stim() < -200 || get_painkiller() > 240 ) {
        add_msg_player_or_npc( m_bad,
                               _( "Your breathing stops completely." ),
                               _( "<npcname>'s breathing stops completely." ) );
        get_event_bus().send<event_type::dies_from_drug_overdose>( getID(), efftype_id() );
        set_part_hp_cur( body_part_torso, 0 );
    } else if( has_effect( effect_jetinjector ) && get_effect_dur( effect_jetinjector ) > 40_minutes ) {
        if( !( has_trait( trait_NOPAIN ) ) ) {
            add_msg_player_or_npc( m_bad,
                                   _( "Your heart spasms painfully and stops." ),
                                   _( "<npcname>'s heart spasms painfully and stops." ) );
        } else {
            add_msg_player_or_npc( _( "Your heart spasms and stops." ),
                                   _( "<npcname>'s heart spasms and stops." ) );
        }
        get_event_bus().send<event_type::dies_from_drug_overdose>( getID(), effect_jetinjector );
        set_part_hp_cur( body_part_torso, 0 );
    } else if( get_effect_dur( effect_adrenaline ) > 50_minutes ) {
        add_msg_player_or_npc( m_bad,
                               _( "Your heart spasms and stops." ),
                               _( "<npcname>'s heart spasms and stops." ) );
        get_event_bus().send<event_type::dies_from_drug_overdose>( getID(), effect_adrenaline );
        set_part_hp_cur( body_part_torso, 0 );
    } else if( get_effect_int( effect_drunk ) > 4 ) {
        add_msg_player_or_npc( m_bad,
                               _( "Your breathing slows down to a stop." ),
                               _( "<npcname>'s breathing slows down to a stop." ) );
        get_event_bus().send<event_type::dies_from_drug_overdose>( getID(), effect_drunk );
        set_part_hp_cur( body_part_torso, 0 );
    }

    // check if we've starved
    if( is_player() ) {
        if( get_stored_kcal() <= 0 ) {
            add_msg_if_player( m_bad, _( "You have starved to death." ) );
            get_event_bus().send<event_type::dies_of_starvation>( getID() );
            set_part_hp_cur( body_part_torso, 0 );
        } else {
            if( calendar::once_every( 12_hours ) ) {
                std::string category;
                if( stomach.contains() <= stomach.capacity( *this ) / 4 ) {
                    if( get_kcal_percent() < 0.1f ) {
                        category = "starving";
                    } else if( get_kcal_percent() < 0.25f ) {
                        category = "emaciated";
                    } else if( get_kcal_percent() < 0.5f ) {
                        category = "malnutrition";
                    } else if( get_kcal_percent() < 0.8f ) {
                        category = "low_cal";
                    }
                } else {
                    if( get_kcal_percent() < 0.1f ) {
                        category = "empty_starving";
                    } else if( get_kcal_percent() < 0.25f ) {
                        category = "empty_emaciated";
                    } else if( get_kcal_percent() < 0.5f ) {
                        category = "empty_malnutrition";
                    } else if( get_kcal_percent() < 0.8f ) {
                        category = "empty_low_cal";
                    }
                }
                if( !category.empty() ) {
                    const translation message = SNIPPET.random_from_category( category ).value_or( translation() );
                    add_msg_if_player( m_warning, message );
                }

            }
        }
    }

    // Check if we're dying of thirst
    if( is_player() && get_thirst() >= 600 && ( stomach.get_water() == 0_ml ||
            guts.get_water() == 0_ml ) ) {
        if( get_thirst() >= 1200 ) {
            add_msg_if_player( m_bad, _( "You have died of dehydration." ) );
            get_event_bus().send<event_type::dies_of_thirst>( getID() );
            set_part_hp_cur( body_part_torso, 0 );
        } else if( get_thirst() >= 1000 && calendar::once_every( 30_minutes ) ) {
            add_msg_if_player( m_warning, _( "Even your eyes feel dry…" ) );
        } else if( get_thirst() >= 800 && calendar::once_every( 30_minutes ) ) {
            add_msg_if_player( m_warning, _( "You are THIRSTY!" ) );
        } else if( calendar::once_every( 30_minutes ) ) {
            add_msg_if_player( m_warning, _( "Your mouth feels so dry…" ) );
        }
    }

    // Check if we're falling asleep, unless we're sleeping
    if( get_fatigue() >= fatigue_levels::EXHAUSTED + 25 && !in_sleep_state() ) {
        if( get_fatigue() >= fatigue_levels::MASSIVE_FATIGUE ) {
            add_msg_if_player( m_bad, _( "Survivor sleep now." ) );
            get_event_bus().send<event_type::falls_asleep_from_exhaustion>( getID() );
            mod_fatigue( -10 );
            fall_asleep();
        } else if( get_fatigue() >= 800 && calendar::once_every( 30_minutes ) ) {
            add_msg_if_player( m_warning, _( "Anywhere would be a good place to sleep…" ) );
        } else if( calendar::once_every( 30_minutes ) ) {
            add_msg_if_player( m_warning, _( "You feel like you haven't slept in days." ) );
        }
    }

    // Even if we're not Exhausted, we really should be feeling lack/sleep earlier
    // Penalties start at Dead Tired and go from there
    if( get_fatigue() >= fatigue_levels::DEAD_TIRED && !in_sleep_state() ) {
        if( get_fatigue() >= 700 ) {
            if( calendar::once_every( 30_minutes ) ) {
                add_msg_if_player( m_warning, _( "You're too physically tired to stop yawning." ) );
                add_effect( effect_lack_sleep, 30_minutes + 1_turns );
            }
            /** @EFFECT_INT slightly decreases occurrence of short naps when dead tired */
            if( one_in( 50 + int_cur ) ) {
                // Rivet's idea: look out for microsleeps!
                fall_asleep( 30_seconds );
            }
        } else if( get_fatigue() >= fatigue_levels::EXHAUSTED ) {
            if( calendar::once_every( 30_minutes ) ) {
                add_msg_if_player( m_warning, _( "How much longer until bedtime?" ) );
                add_effect( effect_lack_sleep, 30_minutes + 1_turns );
            }
            /** @EFFECT_INT slightly decreases occurrence of short naps when exhausted */
            if( one_in( 100 + int_cur ) ) {
                fall_asleep( 30_seconds );
            }
        } else if( get_fatigue() >= fatigue_levels::DEAD_TIRED && calendar::once_every( 30_minutes ) ) {
            add_msg_if_player( m_warning, _( "*yawn* You should really get some sleep." ) );
            add_effect( effect_lack_sleep, 30_minutes + 1_turns );
        }
    }

    // Sleep deprivation kicks in if lack of sleep is avoided with stimulants or otherwise for long periods of time
    int sleep_deprivation = get_sleep_deprivation();
    float sleep_deprivation_pct = sleep_deprivation / static_cast<float>( SLEEP_DEPRIVATION_MASSIVE );

    if( sleep_deprivation >= SLEEP_DEPRIVATION_HARMLESS && !in_sleep_state() ) {
        if( calendar::once_every( 60_minutes ) ) {
            if( sleep_deprivation < SLEEP_DEPRIVATION_MINOR ) {
                add_msg_if_player( m_warning,
                                   _( "Your mind feels tired.  It's been a while since you've slept well." ) );
                mod_fatigue( 1 );
            } else if( sleep_deprivation < SLEEP_DEPRIVATION_SERIOUS ) {
                add_msg_if_player( m_bad,
                                   _( "Your mind feels foggy from a lack of good sleep, and your eyes keep trying to close against your will." ) );
                mod_fatigue( 5 );

                if( one_in( 10 ) ) {
                    mod_healthy_mod( -1, 0 );
                }
            } else if( sleep_deprivation < SLEEP_DEPRIVATION_MAJOR ) {
                add_msg_if_player( m_bad,
                                   _( "Your mind feels weary, and you dread every wakeful minute that passes.  You crave sleep, and feel like you're about to collapse." ) );
                mod_fatigue( 10 );

                if( one_in( 5 ) ) {
                    mod_healthy_mod( -2, 0 );
                }
            } else if( sleep_deprivation < SLEEP_DEPRIVATION_MASSIVE ) {
                add_msg_if_player( m_bad,
                                   _( "You haven't slept decently for so long that your whole body is screaming for mercy.  It's a miracle that you're still awake, but it feels more like a curse now." ) );
                mod_fatigue( 40 );

                mod_healthy_mod( -5, 0 );
            }
            // else you pass out for 20 hours, guaranteed

            // Microsleeps are slightly worse if you're sleep deprived, but not by much. (chance: 1 in (75 + int_cur) at lethal sleep deprivation)
            // Note: these can coexist with fatigue-related microsleeps
            /** @EFFECT_INT slightly decreases occurrence of short naps when sleep deprived */
            if( one_in( static_cast<int>( sleep_deprivation_pct * 75 ) + int_cur ) ) {
                fall_asleep( 30_seconds );
            }

            // Stimulants can be used to stay awake a while longer, but after a while you'll just collapse.
            bool can_pass_out = ( get_stim() < 30 && sleep_deprivation >= SLEEP_DEPRIVATION_MINOR ) ||
                                sleep_deprivation >= SLEEP_DEPRIVATION_MAJOR;

            if( can_pass_out && calendar::once_every( 10_minutes ) ) {
                /** @EFFECT_PER slightly increases resilience against passing out from sleep deprivation */
                if( one_in( static_cast<int>( ( 1 - sleep_deprivation_pct ) * 100 ) + per_cur ) ||
                    sleep_deprivation >= SLEEP_DEPRIVATION_MASSIVE ) {
                    add_msg_player_or_npc( m_bad,
                                           _( "Your body collapses due to sleep deprivation, your neglected fatigue rushing back all at once, and you pass out on the spot." )
                                           , _( "<npcname> collapses to the ground from exhaustion." ) );
                    if( get_fatigue() < fatigue_levels::EXHAUSTED ) {
                        set_fatigue( fatigue_levels::EXHAUSTED );
                    }

                    if( sleep_deprivation >= SLEEP_DEPRIVATION_MAJOR ) {
                        fall_asleep( 20_hours );
                    } else if( sleep_deprivation >= SLEEP_DEPRIVATION_SERIOUS ) {
                        fall_asleep( 16_hours );
                    } else {
                        fall_asleep( 12_hours );
                    }
                }
            }

        }
    }
}

void Character::get_sick()
{
    // NPCs are too dumb to handle infections now
    if( is_npc() || has_flag( json_flag_NO_DISEASE ) ) {
        // In a shocking twist, disease immunity prevents diseases.
        return;
    }

    if( has_effect( effect_flu ) || has_effect( effect_common_cold ) ) {
        // While it's certainly possible to get sick when you already are,
        // it wouldn't be very fun.
        return;
    }

    // Normal people get sick about 2-4 times/year.
    int base_diseases_per_year = 3;
    if( has_trait( trait_DISRESISTANT ) ) {
        // Disease resistant people only get sick once a year.
        base_diseases_per_year = 1;
    }

    // This check runs once every 30 minutes, so double to get hours, *24 to get days.
    const int checks_per_year = 2 * 24 * 365;

    // Health is in the range [-200,200].
    // Diseases are half as common for every 50 health you gain.
    float health_factor = std::pow( 2.0f, get_healthy() / 50.0f );

    int disease_rarity = static_cast<int>( checks_per_year * health_factor / base_diseases_per_year );
    add_msg_debug( "disease_rarity = %d", disease_rarity );
    if( one_in( disease_rarity ) ) {
        if( one_in( 6 ) ) {
            // The flu typically lasts 3-10 days.
            add_env_effect( effect_flu, body_part_mouth, 3, rng( 3_days, 10_days ) );
        } else {
            // A cold typically lasts 1-14 days.
            add_env_effect( effect_common_cold, body_part_mouth, 3, rng( 1_days, 14_days ) );
        }
    }
}

bool Character::is_hibernating() const
{
    // Hibernating only kicks in whilst Engorged; separate tracking for hunger/thirst here
    // as a safety catch.  One test subject managed to get two Colds during hibernation;
    // since those add fatigue and dry out the character, the subject went for the full 10 days plus
    // a little, and came out of it well into Parched.  Hibernating shouldn't endanger your
    // life like that--but since there's much less fluid reserve than food reserve,
    // simply using the same numbers won't work.
    return has_effect( effect_sleep ) && get_kcal_percent() > 0.8f &&
           get_thirst() <= 80 && has_active_mutation( trait_HIBERNATE );
}

/* Here lies the intended effects of body temperature

Assumption 1 : a naked person is comfortable at 19C/66.2F (31C/87.8F at rest).
Assumption 2 : a "lightly clothed" person is comfortable at 13C/55.4F (25C/77F at rest).
Assumption 3 : the player is always running, thus generating more heat.
Assumption 4 : frostbite cannot happen above 0C temperature.*
* In the current model, a naked person can get frostbite at 1C. This isn't true, but it's a compromise with using nice whole numbers.

Here is a list of warmth values and the corresponding temperatures in which the player is comfortable, and in which the player is very cold.

Warmth  Temperature (Comfortable)    Temperature (Very cold)    Notes
  0       19C /  66.2F               -11C /  12.2F               * Naked
 10       13C /  55.4F               -17C /   1.4F               * Lightly clothed
 20        7C /  44.6F               -23C /  -9.4F
 30        1C /  33.8F               -29C / -20.2F
 40       -5C /  23.0F               -35C / -31.0F
 50      -11C /  12.2F               -41C / -41.8F
 60      -17C /   1.4F               -47C / -52.6F
 70      -23C /  -9.4F               -53C / -63.4F
 80      -29C / -20.2F               -59C / -74.2F
 90      -35C / -31.0F               -65C / -85.0F
100      -41C / -41.8F               -71C / -95.8F

WIND POWER
Except for the last entry, pressures are sort of made up...

Breeze : 5mph (1015 hPa)
Strong Breeze : 20 mph (1000 hPa)
Moderate Gale : 30 mph (990 hPa)
Storm : 50 mph (970 hPa)
Hurricane : 100 mph (920 hPa)
HURRICANE : 185 mph (880 hPa) [Ref: Hurricane Wilma]
*/

void Character::update_bodytemp()
{
    if( has_trait( trait_DEBUG_NOTEMP ) ) {
        set_all_parts_temp_conv( BODYTEMP_NORM );
        set_all_parts_temp_cur( BODYTEMP_NORM );
        return;
    }
    /* Cache calls to g->get_temperature( player position ), used in several places in function */
    const int player_local_temp = g->weather.get_temperature( pos() );
    // NOTE : visit weather.h for some details on the numbers used
    // Converts temperature to Celsius/10
    int Ctemperature = static_cast<int>( 100 * temp_to_celsius( player_local_temp ) );
    const w_point weather = *g->weather.weather_precise;
    int vehwindspeed = 0;
    map &here = get_map();
    const optional_vpart_position vp = here.veh_at( pos() );
    if( vp ) {
        vehwindspeed = std::abs( vp->vehicle().velocity / 100 ); // vehicle velocity in mph
    }
    const oter_id &cur_om_ter = overmap_buffer.ter( global_omt_location() );
    bool sheltered = g->is_sheltered( pos() );
    double total_windpower = get_local_windpower( g->weather.windspeed + vehwindspeed, cur_om_ter,
                             pos(),
                             g->weather.winddirection, sheltered );
    // Let's cache this not to check it for every bodyparts
    const bool has_bark = has_trait( trait_BARK );
    const bool has_sleep = has_effect( effect_sleep );
    const bool has_sleep_state = has_sleep || in_sleep_state();
    const bool heat_immune = has_flag( json_flag_HEATPROOF );
    const bool has_heatsink = has_bionic( bio_heatsink ) || is_wearing( itype_rm13_armor_on ) ||
                              heat_immune;
    const bool has_common_cold = has_effect( effect_common_cold );
    const bool has_climate_control = in_climate_control();
    const bool use_floor_warmth = can_use_floor_warmth();
    const furn_id furn_at_pos = here.furn( pos() );
    const cata::optional<vpart_reference> boardable = vp.part_with_feature( "BOARDABLE", true );
    // Temperature norms
    // Ambient normal temperature is lower while asleep
    const int ambient_norm = has_sleep ? 3100 : 1900;

    /**
     * Calculations that affect all body parts equally go here, not in the loop
     */
    // Hunger / Starvation
    // -1000 when about to starve to death
    // -1333 when starving with light eater
    // -2000 if you managed to get 0 metabolism rate somehow
    const float met_rate = metabolic_rate();
    const int hunger_warmth = static_cast<int>( 2000 * std::min( met_rate, 1.0f ) - 2000 );
    // Give SOME bonus to those living furnaces with extreme metabolism
    const int metabolism_warmth = static_cast<int>( std::max( 0.0f, met_rate - 1.0f ) * 1000 );
    // Fatigue
    // ~-900 when exhausted
    const int fatigue_warmth = has_sleep ? 0 : static_cast<int>( clamp( -1.5f * get_fatigue(), -1350.0f,
                               0.0f ) );

    // Sunlight
    const int sunlight_warmth = g->is_in_sunlight( pos() ) ?
                                ( get_weather().weather_id->sun_intensity ==
                                  sun_intensity_type::high ?
                                  1000 :
                                  500 ) : 0;
    const int best_fire = get_heat_radiation( pos(), true );

    const int lying_warmth = use_floor_warmth ? floor_warmth( pos() ) : 0;
    const int water_temperature =
        100 * temp_to_celsius( get_weather().get_cur_weather_gen().get_water_temperature() );

    // Correction of body temperature due to traits and mutations
    // Lower heat is applied always
    const int mutation_heat_low = bodytemp_modifier_traits( false );
    const int mutation_heat_high = bodytemp_modifier_traits( true );
    // Difference between high and low is the "safe" heat - one we only apply if it's beneficial
    const int mutation_heat_bonus = mutation_heat_high - mutation_heat_low;

    const int h_radiation = get_heat_radiation( pos(), false );
    // Current temperature and converging temperature calculations
    for( const bodypart_id &bp : get_all_body_parts() ) {

        if( bp->has_flag( "IGNORE_TEMP" ) ) {
            continue;
        }

        // This adjusts the temperature scale to match the bodytemp scale,
        // it needs to be reset every iteration
        int adjusted_temp = ( Ctemperature - ambient_norm );
        int bp_windpower = total_windpower;
        // Represents the fact that the body generates heat when it is cold.
        // TODO: : should this increase hunger?
        double scaled_temperature = logarithmic_range( BODYTEMP_VERY_COLD, BODYTEMP_VERY_HOT,
                                    get_part_temp_cur( bp ) );
        // Produces a smooth curve between 30.0 and 60.0.
        double homeostasis_adjustment = 30.0 * ( 1.0 + scaled_temperature );
        int clothing_warmth_adjustment = static_cast<int>( homeostasis_adjustment * warmth( bp ) );
        int clothing_warmth_adjusted_bonus = static_cast<int>( homeostasis_adjustment * bonus_item_warmth(
                bp ) );
        // WINDCHILL

        bp_windpower = static_cast<int>( static_cast<float>( bp_windpower ) * ( 1 - get_wind_resistance(
                                             bp ) / 100.0 ) );
        // Calculate windchill
        int windchill = get_local_windchill( player_local_temp,
                                             get_local_humidity( weather.humidity, get_weather().weather_id, sheltered ),
                                             bp_windpower );

        static const auto is_lower = []( const bodypart_id & bp ) {
            return bp == body_part_foot_l  ||
                   bp ==  body_part_foot_r  ||
                   bp ==  body_part_leg_l  ||
                   bp ==  body_part_leg_r ;
        };

        // If you're standing in water, air temperature is replaced by water temperature. No wind.
        // Convert to 0.01C
        if( here.has_flag_ter( TFLAG_DEEP_WATER, pos() ) ||
            ( here.has_flag_ter( TFLAG_SHALLOW_WATER, pos() ) && is_lower( bp ) ) ) {
            adjusted_temp += water_temperature - Ctemperature; // Swap out air temp for water temp.
            windchill = 0;
        }

        // Convergent temperature is affected by ambient temperature,
        // clothing warmth, and body wetness.
        set_part_temp_conv( bp, BODYTEMP_NORM + adjusted_temp + windchill * 100 +
                            clothing_warmth_adjustment );
        // HUNGER / STARVATION
        mod_part_temp_conv( bp, hunger_warmth );
        // FATIGUE
        mod_part_temp_conv( bp, fatigue_warmth );
        // Mutations
        mod_part_temp_conv( bp, mutation_heat_low );
        // DIRECT HEAT SOURCES (generates body heat, helps fight frostbite)
        // Bark : lowers blister count to -5; harder to get blisters
        int blister_count = ( has_bark ? -5 : 0 ); // If the counter is high, your skin starts to burn

        if( get_part_frostbite_timer( bp ) > 0 ) {
            mod_part_frostbite_timer( bp, -std::max( 5, h_radiation ) );
        }
        // 111F (44C) is a temperature in which proteins break down: https://en.wikipedia.org/wiki/Burn
        blister_count += h_radiation - 111 > 0 ?
                         std::max( static_cast<int>( std::sqrt( h_radiation - 111 ) ), 0 ) : 0;

        const bool pyromania = has_trait( trait_PYROMANIA );
        // BLISTERS : Skin gets blisters from intense heat exposure.
        // Fire protection protects from blisters.
        // Heatsinks give near-immunity.
        if( blister_count - get_armor_fire( bp ) - ( has_heatsink ? 20 : 0 ) > 0 ) {
            add_effect( effect_blisters, 1_turns, bp );
            if( pyromania ) {
                add_morale( MORALE_PYROMANIA_NEARFIRE, 10, 10, 1_hours,
                            30_minutes ); // Proximity that's close enough to harm us gives us a bit of a thrill
                rem_morale( MORALE_PYROMANIA_NOFIRE );
            }
        } else if( pyromania && best_fire >= 1 ) { // Only give us fire bonus if there's actually fire
            add_morale( MORALE_PYROMANIA_NEARFIRE, 5, 5, 30_minutes,
                        15_minutes ); // Gain a much smaller mood boost even if it doesn't hurt us
            rem_morale( MORALE_PYROMANIA_NOFIRE );
        }

        mod_part_temp_conv( bp, sunlight_warmth );
        // DISEASES
        if( bp == body_part_head && has_effect( effect_flu ) ) {
            mod_part_temp_conv( bp, 1500 );
        }
        if( has_common_cold ) {
            mod_part_temp_conv( bp, -750 );
        }
        // Loss of blood results in loss of body heat, 1% bodyheat lost per 2% hp lost
        mod_part_temp_conv( bp, - blood_loss( bp ) * get_part_temp_conv( bp ) / 200 );

        // EQUALIZATION
        static const std::pair<bodypart_str_id, bodypart_str_id> connections[] {
            {body_part_torso, body_part_arm_l },
            {body_part_torso, body_part_arm_r },
            {body_part_torso, body_part_leg_l },
            {body_part_torso, body_part_leg_r },
            {body_part_torso, body_part_head },
            {body_part_head, body_part_mouth },
            {body_part_arm_l, body_part_hand_l },
            {body_part_arm_r, body_part_hand_r },
            {body_part_leg_l, body_part_foot_l },
            {body_part_leg_r, body_part_foot_r }
        };

        bool equalized = false;
        for( const auto &conn : connections ) {
            if( bp == conn.first ) {
                temp_equalizer( bp, conn.second );
                equalized = true;
            }
            // connections are defined in one-direction only, but should work in both direction
            if( bp == conn.second ) {
                temp_equalizer( conn.second, bp );
                equalized = true;
            }
        }
        if( !equalized ) {
            debugmsg( "Wacky body part temperature equalization!  Body part is not handled: %s",
                      bp.id().str() );
        }

        // Climate Control eases the effects of high and low ambient temps
        if( has_climate_control ) {
            set_part_temp_conv( bp, temp_corrected_by_climate_control( get_part_temp_conv( bp ) ) );
        }

        // FINAL CALCULATION : Increments current body temperature towards convergent.
        int bonus_fire_warmth = 0;
        if( !has_sleep_state && best_fire > 0 ) {
            // Warming up over a fire
            if( bp == body_part_foot_l || bp == body_part_foot_r ) {
                if( furn_at_pos != f_null ) {
                    // Can sit on something to lift feet up to the fire
                    bonus_fire_warmth = best_fire * furn_at_pos.obj().bonus_fire_warmth_feet;
                } else if( boardable ) {
                    bonus_fire_warmth = best_fire * boardable->info().bonus_fire_warmth_feet;
                } else {
                    // Has to stand
                    bonus_fire_warmth = best_fire * bp->fire_warmth_bonus;
                }
            } else {
                bonus_fire_warmth = best_fire * bp->fire_warmth_bonus;
            }

        }

        const int comfortable_warmth = bonus_fire_warmth + lying_warmth;
        const int bonus_warmth = comfortable_warmth + metabolism_warmth + mutation_heat_bonus;
        if( bonus_warmth > 0 ) {
            // Approximate temp_conv needed to reach comfortable temperature in this very turn
            // Basically inverted formula for temp_cur below
            int desired = 501 * BODYTEMP_NORM - 499 * get_part_temp_conv( bp );
            if( std::abs( BODYTEMP_NORM - desired ) < 1000 ) {
                desired = BODYTEMP_NORM; // Ensure that it converges
            } else if( desired > BODYTEMP_HOT ) {
                desired = BODYTEMP_HOT; // Cap excess at sane temperature
            }

            if( desired < get_part_temp_conv( bp ) ) {
                // Too hot, can't help here
            } else if( desired < get_part_temp_conv( bp ) + bonus_warmth ) {
                // Use some heat, but not all of it
                set_part_temp_conv( bp, desired );
            } else {
                // Use all the heat
                mod_part_temp_conv( bp, bonus_warmth );
            }

            // Morale bonus for comfiness - only if actually comfy (not too warm/cold)
            // Spread the morale bonus in time.
            if( comfortable_warmth > 0 &&
                calendar::once_every( 1_minutes ) && get_effect_int( effect_cold ) == 0 &&
                get_effect_int( effect_hot ) == 0 &&
                get_part_temp_conv( bp ) > BODYTEMP_COLD && get_part_temp_conv( bp ) <= BODYTEMP_NORM ) {
                add_morale( MORALE_COMFY, 1, 10, 2_minutes, 1_minutes, true );
            }
        }

        const int temp_before = get_part_temp_cur( bp );
        const int cur_temp_conv = get_part_temp_conv( bp );
        int temp_difference = temp_before - cur_temp_conv; // Negative if the player is warming up.
        // exp(-0.001) : half life of 60 minutes, exp(-0.002) : half life of 30 minutes,
        // exp(-0.003) : half life of 20 minutes, exp(-0.004) : half life of 15 minutes
        int rounding_error = 0;
        // If temp_diff is small, the player cannot warm up due to rounding errors. This fixes that.
        if( temp_difference < 0 && temp_difference > -600 ) {
            rounding_error = 1;
        }
        if( temp_before != cur_temp_conv ) {
            set_part_temp_cur( bp, static_cast<int>( temp_difference * std::exp( -0.002 ) + cur_temp_conv +
                               rounding_error ) );
        }
        // This statement checks if we should be wearing our bonus warmth.
        // If, after all the warmth calculations, we should be, then we have to recalculate the temperature.
        if( clothing_warmth_adjusted_bonus != 0 &&
            ( ( cur_temp_conv + clothing_warmth_adjusted_bonus ) < BODYTEMP_HOT ||
              get_part_temp_cur( bp ) < BODYTEMP_COLD ) ) {
            mod_part_temp_conv( bp, clothing_warmth_adjusted_bonus );
            rounding_error = 0;
            if( temp_difference < 0 && temp_difference > -600 ) {
                rounding_error = 1;
            }
            const int new_temp_conv = get_part_temp_conv( bp );
            if( temp_before != new_temp_conv ) {
                temp_difference = get_part_temp_cur( bp ) - new_temp_conv;
                set_part_temp_cur( bp, static_cast<int>( temp_difference * std::exp( -0.002 ) + new_temp_conv +
                                   rounding_error ) );
            }
        }
        const int temp_after = get_part_temp_cur( bp );
        // PENALTIES
        if( temp_after < BODYTEMP_FREEZING ) {
            add_effect( effect_cold, 1_turns, bp, true, 3 );
        } else if( temp_after < BODYTEMP_VERY_COLD ) {
            add_effect( effect_cold, 1_turns, bp, true, 2 );
        } else if( temp_after < BODYTEMP_COLD ) {
            add_effect( effect_cold, 1_turns, bp, true, 1 );
        } else if( temp_after > BODYTEMP_SCORCHING && !heat_immune ) {
            add_effect( effect_hot, 1_turns, bp, true, 3 );
            if( bp->main_part == bp.id() ) {
                add_effect( effect_hot_speed, 1_turns, bp, true, 3 );
            }
        } else if( temp_after > BODYTEMP_VERY_HOT && !heat_immune ) {
            add_effect( effect_hot, 1_turns, bp, true, 2 );
            if( bp->main_part == bp.id() ) {
                add_effect( effect_hot_speed, 1_turns, bp, true, 2 );
            }
        } else if( temp_after > BODYTEMP_HOT && !heat_immune ) {
            add_effect( effect_hot, 1_turns, bp, true, 1 );
            if( bp->main_part == bp.id() ) {
                add_effect( effect_hot_speed, 1_turns, bp, true, 1 );
            }
        } else {
            remove_effect( effect_cold, bp );
            remove_effect( effect_hot, bp );
            remove_effect( effect_hot_speed, bp );
        }

        update_frostbite( bp, bp_windpower );

        // Warn the player if condition worsens
        if( temp_before > BODYTEMP_FREEZING && temp_after < BODYTEMP_FREEZING ) {
            //~ %s is bodypart
            add_msg( m_warning, _( "You feel your %s beginning to go numb from the cold!" ),
                     body_part_name( bp ) );
        } else if( temp_before > BODYTEMP_VERY_COLD && temp_after < BODYTEMP_VERY_COLD ) {
            //~ %s is bodypart
            add_msg( m_warning, _( "You feel your %s getting very cold." ),
                     body_part_name( bp ) );
        } else if( temp_before > BODYTEMP_COLD && temp_after < BODYTEMP_COLD ) {
            //~ %s is bodypart
            add_msg( m_warning, _( "You feel your %s getting chilly." ),
                     body_part_name( bp ) );
        } else if( temp_before < BODYTEMP_SCORCHING && temp_after > BODYTEMP_SCORCHING ) {
            //~ %s is bodypart
            add_msg( m_bad, _( "You feel your %s getting red hot from the heat!" ),
                     body_part_name( bp ) );
        } else if( temp_before < BODYTEMP_VERY_HOT && temp_after > BODYTEMP_VERY_HOT ) {
            //~ %s is bodypart
            add_msg( m_warning, _( "You feel your %s getting very hot." ),
                     body_part_name( bp ) );
        } else if( temp_before < BODYTEMP_HOT && temp_after > BODYTEMP_HOT ) {
            //~ %s is bodypart
            add_msg( m_warning, _( "You feel your %s getting warm." ),
                     body_part_name( bp ) );
        }

        // Note: Numbers are based off of BODYTEMP at the top of weather.h
        // If torso is BODYTEMP_COLD which is 34C, the early stages of hypothermia begin
        // constant shivering will prevent the player from falling asleep.
        // Otherwise, if any other body part is BODYTEMP_VERY_COLD, or 31C
        // AND you have frostbite, then that also prevents you from sleeping
        if( in_sleep_state() && !has_effect( effect_narcosis ) ) {
            if( bp == body_part_torso && temp_after <= BODYTEMP_COLD ) {
                add_msg( m_warning, _( "Your shivering prevents you from sleeping." ) );
                wake_up();
            } else if( bp != body_part_torso && temp_after <= BODYTEMP_VERY_COLD &&
                       has_effect( effect_frostbite ) ) {
                add_msg( m_warning, _( "You are too cold.  Your frostbite prevents you from sleeping." ) );
                wake_up();
            }
        }

        const int conv_temp = get_part_temp_conv( bp );
        // Warn the player that wind is going to be a problem.
        // But only if it can be a problem, no need to spam player with "wind chills your scorching body"
        if( conv_temp <= BODYTEMP_COLD && windchill < -10 && one_in( 200 ) ) {
            add_msg( m_bad, _( "The wind is making your %s feel quite cold." ),
                     body_part_name( bp ) );
        } else if( conv_temp <= BODYTEMP_COLD && windchill < -20 && one_in( 100 ) ) {
            add_msg( m_bad,
                     _( "The wind is very strong; you should find some more wind-resistant clothing for your %s." ),
                     body_part_name( bp ) );
        } else if( conv_temp <= BODYTEMP_COLD && windchill < -30 && one_in( 50 ) ) {
            add_msg( m_bad, _( "Your clothing is not providing enough protection from the wind for your %s!" ),
                     body_part_name( bp ) );
        }
    }
}

void Character::update_frostbite( const bodypart_id &bp, const int FBwindPower )
{
    // FROSTBITE - only occurs to hands, feet, face
    /**

    Source : http://www.atc.army.mil/weather/windchill.pdf

    Temperature and wind chill are main factors, mitigated by clothing warmth. Each 10 warmth protects against 2C of cold.

    1200 turns in low risk, + 3 tics
    450 turns in moderate risk, + 8 tics
    50 turns in high risk, +72 tics

    Let's say frostnip @ 1800 tics, frostbite @ 3600 tics

    >> Chunked into 8 parts (http://imgur.com/xlTPmJF)
    -- 2 hour risk --
    Between 30F and 10F
    Between 10F and -5F, less than 20mph, -4x + 3y - 20 > 0, x : F, y : mph
    -- 45 minute risk --
    Between 10F and -5F, less than 20mph, -4x + 3y - 20 < 0, x : F, y : mph
    Between 10F and -5F, greater than 20mph
    Less than -5F, less than 10 mph
    Less than -5F, more than 10 mph, -4x + 3y - 170 > 0, x : F, y : mph
    -- 5 minute risk --
    Less than -5F, more than 10 mph, -4x + 3y - 170 < 0, x : F, y : mph
    Less than -35F, more than 10 mp
    **/

    const int player_local_temp = g->weather.get_temperature( pos() );
    const int temp_after = get_part_temp_cur( bp );

    if( bp == body_part_mouth || bp == body_part_foot_r ||
        bp == body_part_foot_l || bp == body_part_hand_r || bp == body_part_hand_l ) {
        // Handle the frostbite timer
        // Need temps in F, windPower already in mph
        int wetness_percentage = 100 * get_part_wetness_percentage( bp ); // 0 - 100
        // Warmth gives a slight buff to temperature resistance
        // Wetness gives a heavy nerf to temperature resistance
        double adjusted_warmth = warmth( bp ) - wetness_percentage;
        int Ftemperature = static_cast<int>( player_local_temp + 0.2 * adjusted_warmth );

        int intense = get_effect_int( effect_frostbite, bp );

        // This has been broken down into 8 zones
        // Low risk zones (stops at frostnip)
        if( temp_after < BODYTEMP_COLD && ( ( Ftemperature < 30 && Ftemperature >= 10 ) ||
                                            ( Ftemperature < 10 && Ftemperature >= -5 && FBwindPower < 20 &&
                                              -4 * Ftemperature + 3 * FBwindPower - 20 >= 0 ) ) ) {
            if( get_part_frostbite_timer( bp ) < 2000 ) {
                mod_part_frostbite_timer( bp, 3 );
            }
            if( one_in( 100 ) && !has_effect( effect_frostbite, bp.id() ) ) {
                add_msg( m_warning, _( "Your %s will be frostnipped in the next few hours." ),
                         body_part_name( bp ) );
            }
            // Medium risk zones
        } else if( temp_after < BODYTEMP_COLD &&
                   ( ( Ftemperature < 10 && Ftemperature >= -5 && FBwindPower < 20 &&
                       -4 * Ftemperature + 3 * FBwindPower - 20 < 0 ) ||
                     ( Ftemperature < 10 && Ftemperature >= -5 && FBwindPower >= 20 ) ||
                     ( Ftemperature < -5 && FBwindPower < 10 ) ||
                     ( Ftemperature < -5 && FBwindPower >= 10 &&
                       -4 * Ftemperature + 3 * FBwindPower - 170 >= 0 ) ) ) {
            mod_part_frostbite_timer( bp, 8 );
            if( one_in( 100 ) && intense < 2 ) {
                add_msg( m_warning, _( "Your %s will be frostbitten within the hour!" ),
                         body_part_name( bp ) );
            }
            // High risk zones
        } else if( temp_after < BODYTEMP_COLD &&
                   ( ( Ftemperature < -5 && FBwindPower >= 10 &&
                       -4 * Ftemperature + 3 * FBwindPower - 170 < 0 ) ||
                     ( Ftemperature < -35 && FBwindPower >= 10 ) ) ) {
            mod_part_frostbite_timer( bp, 72 );
            if( one_in( 100 ) && intense < 2 ) {
                add_msg( m_warning, _( "Your %s will be frostbitten any minute now!" ),
                         body_part_name( bp ) );
            }
            // Risk free, so reduce frostbite timer
        } else {
            mod_part_frostbite_timer( bp, -3 );
        }

        int frostbite_timer = get_part_frostbite_timer( bp );
        // Handle the bestowing of frostbite
        if( frostbite_timer < 0 ) {
            set_part_frostbite_timer( bp, 0 );
        } else if( frostbite_timer > 4200 ) {
            // This ensures that the player will recover in at most 3 hours.
            set_part_frostbite_timer( bp, 4200 );
        }
        frostbite_timer = get_part_frostbite_timer( bp );
        // Frostbite, no recovery possible
        if( frostbite_timer >= 3600 ) {
            add_effect( effect_frostbite, 1_turns, bp, true, 2 );
            remove_effect( effect_frostbite_recovery, bp );
            // Else frostnip, add recovery if we were frostbitten
        } else if( frostbite_timer >= 1800 ) {
            if( intense == 2 ) {
                add_effect( effect_frostbite_recovery, 1_turns, bp, true );
            }
            add_effect( effect_frostbite, 1_turns, bp, true, 1 );
            // Else fully recovered
        } else if( frostbite_timer == 0 ) {
            remove_effect( effect_frostbite, bp );
            remove_effect( effect_frostbite_recovery, bp );
        }
    }
}

void Character::temp_equalizer( const bodypart_id &bp1, const bodypart_id &bp2 )
{
    // Body heat is moved around.
    // Shift in one direction only, will be shifted in the other direction separately.
    int diff = static_cast<int>( ( get_part_temp_cur( bp2 ) - get_part_temp_cur( bp1 ) ) *
                                 0.0001 ); // If bp1 is warmer, it will lose heat
    mod_part_temp_cur( bp1, diff );
}

Character::comfort_response_t Character::base_comfort_value( const tripoint &p ) const
{
    // Comfort of sleeping spots is "objective", while sleep_spot( p ) is "subjective"
    // As in the latter also checks for fatigue and other variables while this function
    // only looks at the base comfyness of something. It's still subjective, in a sense,
    // as arachnids who sleep in webs will find most places comfortable for instance.
    int comfort = 0;

    comfort_response_t comfort_response;

    bool plantsleep = has_trait( trait_CHLOROMORPH );
    bool fungaloid_cosplay = has_trait( trait_M_SKIN3 );
    bool websleep = has_trait( trait_WEB_WALKER );
    bool webforce = has_trait( trait_THRESH_SPIDER ) && ( has_trait( trait_WEB_SPINNER ) ||
                    ( has_trait( trait_WEB_WEAVER ) ) );
    bool in_shell = has_active_mutation( trait_SHELL2 );
    bool watersleep = has_trait( trait_WATERSLEEP );

    map &here = get_map();
    const optional_vpart_position vp = here.veh_at( p );
    const maptile tile = here.maptile_at( p );
    const trap &trap_at_pos = tile.get_trap_t();
    const ter_id ter_at_pos = tile.get_ter();
    const furn_id furn_at_pos = tile.get_furn();

    int web = here.get_field_intensity( p, fd_web );

    // Some mutants have different comfort needs
    if( !plantsleep && !webforce ) {
        if( in_shell ) {
            comfort += 1 + static_cast<int>( comfort_level::slightly_comfortable );
            // Note: shelled individuals can still use sleeping aids!
        } else if( vp ) {
            const cata::optional<vpart_reference> carg = vp.part_with_feature( "CARGO", false );
            const cata::optional<vpart_reference> board = vp.part_with_feature( "BOARDABLE", true );
            if( carg ) {
                const vehicle_stack items = vp->vehicle().get_items( carg->part_index() );
                for( const item &items_it : items ) {
                    if( items_it.has_flag( flag_SLEEP_AID ) ) {
                        // Note: BED + SLEEP_AID = 9 pts, or 1 pt below very_comfortable
                        comfort += 1 + static_cast<int>( comfort_level::slightly_comfortable );
                        comfort_response.aid = &items_it;
                        break; // prevents using more than 1 sleep aid
                    }
                    if( items_it.has_flag( flag_SLEEP_AID_CONTAINER ) ) {
                        bool found = false;
                        if( items_it.contents.size() > 1 ) {
                            // Only one item is allowed, so we don't fill our pillowcase with nails
                            continue;
                        }
                        for( const item *it : items_it.contents.all_items_top() ) {
                            if( it->has_flag( flag_SLEEP_AID ) ) {
                                // Note: BED + SLEEP_AID = 9 pts, or 1 pt below very_comfortable
                                comfort += 1 + static_cast<int>( comfort_level::slightly_comfortable );
                                comfort_response.aid = &items_it;
                                found = true;
                                break; // prevents using more than 1 sleep aid
                            }
                        }
                        // Only 1 sleep aid
                        if( found ) {
                            break;
                        }
                    }
                }
            }
            if( board ) {
                comfort += board->info().comfort;
            } else {
                comfort -= here.move_cost( p );
            }
        }
        // Not in a vehicle, start checking furniture/terrain/traps at this point in decreasing order
        else if( furn_at_pos != f_null ) {
            comfort += 0 + furn_at_pos.obj().comfort;
        }
        // Web sleepers can use their webs if better furniture isn't available
        else if( websleep && web >= 3 ) {
            comfort += 1 + static_cast<int>( comfort_level::slightly_comfortable );
        } else if( ter_at_pos == t_improvised_shelter ) {
            comfort += 0 + static_cast<int>( comfort_level::slightly_comfortable );
        } else if( ter_at_pos == t_floor || ter_at_pos == t_floor_waxed ||
                   ter_at_pos == t_carpet_red || ter_at_pos == t_carpet_yellow ||
                   ter_at_pos == t_carpet_green || ter_at_pos == t_carpet_purple ) {
            comfort += 1 + static_cast<int>( comfort_level::neutral );
        } else if( !trap_at_pos.is_null() ) {
            comfort += 0 + trap_at_pos.comfort;
        } else {
            // Not a comfortable sleeping spot
            comfort -= here.move_cost( p );
        }

        if( comfort_response.aid == nullptr ) {
            const map_stack items = here.i_at( p );
            for( const item &items_it : items ) {
                if( items_it.has_flag( flag_SLEEP_AID ) ) {
                    // Note: BED + SLEEP_AID = 9 pts, or 1 pt below very_comfortable
                    comfort += 1 + static_cast<int>( comfort_level::slightly_comfortable );
                    comfort_response.aid = &items_it;
                    break; // prevents using more than 1 sleep aid
                }
                if( items_it.has_flag( flag_SLEEP_AID_CONTAINER ) ) {
                    bool found = false;
                    if( items_it.contents.size() > 1 ) {
                        // Only one item is allowed, so we don't fill our pillowcase with nails
                        continue;
                    }
                    for( const item *it : items_it.contents.all_items_top() ) {
                        if( it->has_flag( flag_SLEEP_AID ) ) {
                            // Note: BED + SLEEP_AID = 9 pts, or 1 pt below very_comfortable
                            comfort += 1 + static_cast<int>( comfort_level::slightly_comfortable );
                            comfort_response.aid = &items_it;
                            found = true;
                            break; // prevents using more than 1 sleep aid
                        }
                    }
                    // Only 1 sleep aid
                    if( found ) {
                        break;
                    }
                }
            }
        }
        if( fungaloid_cosplay && here.has_flag_ter_or_furn( "FUNGUS", pos() ) ) {
            comfort += static_cast<int>( comfort_level::very_comfortable );
        } else if( watersleep && here.has_flag_ter( "SWIMMABLE", pos() ) ) {
            comfort += static_cast<int>( comfort_level::very_comfortable );
        }
    } else if( plantsleep ) {
        if( vp || furn_at_pos != f_null ) {
            // Sleep ain't happening in a vehicle or on furniture
            comfort = static_cast<int>( comfort_level::impossible );
        } else {
            // It's very easy for Chloromorphs to get to sleep on soil!
            if( ter_at_pos == t_dirt || ter_at_pos == t_pit || ter_at_pos == t_dirtmound ||
                ter_at_pos == t_pit_shallow ) {
                comfort += static_cast<int>( comfort_level::very_comfortable );
            }
            // Not as much if you have to dig through stuff first
            else if( ter_at_pos == t_grass ) {
                comfort += static_cast<int>( comfort_level::comfortable );
            }
            // Sleep ain't happening
            else {
                comfort = static_cast<int>( comfort_level::impossible );
            }
        }
        // Has webforce
    } else {
        if( web >= 3 ) {
            // Thick Web and you're good to go
            comfort += static_cast<int>( comfort_level::very_comfortable );
        } else {
            comfort = static_cast<int>( comfort_level::impossible );
        }
    }

    if( comfort > static_cast<int>( comfort_level::comfortable ) ) {
        comfort_response.level = comfort_level::very_comfortable;
    } else if( comfort > static_cast<int>( comfort_level::slightly_comfortable ) ) {
        comfort_response.level = comfort_level::comfortable;
    } else if( comfort > static_cast<int>( comfort_level::neutral ) ) {
        comfort_response.level = comfort_level::slightly_comfortable;
    } else if( comfort == static_cast<int>( comfort_level::neutral ) ) {
        comfort_response.level = comfort_level::neutral;
    } else {
        comfort_response.level = comfort_level::uncomfortable;
    }
    return comfort_response;
}

int Character::blood_loss( const bodypart_id &bp ) const
{
    int hp_cur_sum = get_part_hp_cur( bp );
    int hp_max_sum = get_part_hp_max( bp );

    if( bp == body_part_leg_l || bp == body_part_leg_r ) {
        hp_cur_sum = get_part_hp_cur( body_part_leg_l ) + get_part_hp_cur( body_part_leg_r );
        hp_max_sum = get_part_hp_max( body_part_leg_l ) + get_part_hp_max( body_part_leg_r );
    } else if( bp == body_part_arm_l || bp == body_part_arm_r ) {
        hp_cur_sum = get_part_hp_cur( body_part_arm_l ) + get_part_hp_cur( body_part_arm_r );
        hp_max_sum = get_part_hp_max( body_part_arm_l ) + get_part_hp_max( body_part_arm_r );
    }

    hp_cur_sum = std::min( hp_max_sum, std::max( 0, hp_cur_sum ) );
    hp_max_sum = std::max( hp_max_sum, 1 );
    return 100 - ( 100 * hp_cur_sum ) / hp_max_sum;
}

float Character::get_dodge_base() const
{
    /** @EFFECT_DEX increases dodge base */
    /** @EFFECT_DODGE increases dodge_base */
    return get_dex() / 2.0f + get_skill_level( skill_dodge );
}
float Character::get_hit_base() const
{
    /** @EFFECT_DEX increases hit base, slightly */
    return get_dex() / 4.0f;
}

bodypart_id Character::body_window( const std::string &menu_header,
                                    bool show_all, bool precise,
                                    int normal_bonus, int head_bonus, int torso_bonus,
                                    int bleed, float bite, float infect, float bandage_power, float disinfectant_power ) const
{
    /* This struct establishes some kind of connection between the hp_part (which can be healed and
     * have HP) and the body_part. Note that there are more body_parts than hp_parts. For example:
     * Damage to bp_head, bp_eyes and bp_mouth is all applied on the HP of hp_head. */
    struct healable_bp {
        mutable bool allowed;
        bodypart_id bp;
        std::string name; // Translated name as it appears in the menu.
        int bonus;
    };
    /* The array of the menu entries show to the player. The entries are displayed in this order,
     * it may be changed here. */
    std::array<healable_bp, 6> parts = { {
            { false, body_part_head,  _( "Head" ), head_bonus },
            { false, body_part_torso, _( "Torso" ), torso_bonus },
            { false, body_part_arm_l, _( "Left Arm" ), normal_bonus },
            { false, body_part_arm_r,  _( "Right Arm" ), normal_bonus },
            { false, body_part_leg_l,  _( "Left Leg" ), normal_bonus },
            { false, body_part_leg_r,  _( "Right Leg" ), normal_bonus },
        }
    };

    int max_bp_name_len = 0;
    for( const healable_bp &e : parts ) {
        max_bp_name_len = std::max( max_bp_name_len, utf8_width( e.name ) );
    }

    uilist bmenu;
    bmenu.desc_enabled = true;
    bmenu.text = menu_header;
    bmenu.textwidth = 60;

    bmenu.hilight_disabled = true;
    bool is_valid_choice = false;

    // If this is an NPC, the player is the one examining them and so the fact
    // that they can't self-diagnose effectively doesn't matter
    bool no_feeling = is_player() && has_trait( trait_NOPAIN );

    for( size_t i = 0; i < parts.size(); i++ ) {
        const healable_bp &e = parts[i];
        const bodypart_id &bp = e.bp;
        const int maximal_hp = get_part_hp_max( bp );
        const int current_hp = get_part_hp_cur( bp );
        // This will c_light_gray if the part does not have any effects cured by the item/effect
        // (e.g. it cures only bites, but the part does not have a bite effect)
        const nc_color state_col = limb_color( bp, bleed > 0, bite > 0.0f, infect > 0.0f );
        const bool has_curable_effect = state_col != c_light_gray;
        // The same as in the main UI sidebar. Independent of the capability of the healing item/effect!
        const nc_color all_state_col = limb_color( bp, true, true, true );
        // Broken means no HP can be restored, it requires surgical attention.
        const bool limb_is_broken = is_limb_broken( bp );
        const bool limb_is_mending = worn_with_flag( flag_SPLINT, bp );

        if( show_all ) {
            e.allowed = true;
        } else if( has_curable_effect ) {
            e.allowed = true;
        } else if( limb_is_broken ) {
            e.allowed = false;
        } else if( current_hp < maximal_hp && ( e.bonus != 0 || bandage_power > 0.0f  ||
                                                disinfectant_power > 0.0f ) ) {
            e.allowed = true;
        } else {
            e.allowed = false;
        }

        std::string msg;
        std::string desc;
        bool bleeding = has_effect( effect_bleed, bp.id() );
        bool bitten = has_effect( effect_bite, bp.id() );
        bool infected = has_effect( effect_infected, bp.id() );
        bool bandaged = has_effect( effect_bandaged, bp.id() );
        const int b_power = get_effect_int( effect_bandaged, bp );
        const int d_power = get_effect_int( effect_disinfected, bp );
        int new_b_power = static_cast<int>( std::floor( bandage_power ) );
        if( bandaged ) {
            const effect &eff = get_effect( effect_bandaged, bp );
            if( new_b_power > eff.get_max_intensity() ) {
                new_b_power = eff.get_max_intensity();
            }

        }
        int new_d_power = static_cast<int>( std::floor( disinfectant_power ) );

        const auto &aligned_name = std::string( max_bp_name_len - utf8_width( e.name ), ' ' ) + e.name;
        std::string hp_str;
        if( limb_is_mending ) {
            desc += colorize( _( "It is broken, but has been set, and just needs time to heal." ),
                              c_blue ) + "\n";
            if( no_feeling ) {
                hp_str = colorize( "==%==", c_blue );
            } else {
                const auto &eff = get_effect( effect_mending, bp );
                const int mend_perc = eff.is_null() ? 0.0 : 100 * eff.get_duration() / eff.get_max_duration();

                const int num = mend_perc / 20;
                hp_str = colorize( std::string( num, '#' ) + std::string( 5 - num, '=' ), c_blue );
                if( precise ) {
                    hp_str = string_format( "%s %3d%%", hp_str, mend_perc );
                }
            }
        } else if( limb_is_broken ) {
            desc += colorize( _( "It is broken.  It needs a splint or surgical attention." ), c_red ) + "\n";
            hp_str = "==%==";
        } else if( no_feeling ) {
            if( current_hp < maximal_hp * 0.25 ) {
                hp_str = colorize( _( "Very Bad" ), c_red );
            } else if( current_hp < maximal_hp * 0.5 ) {
                hp_str = colorize( _( "Bad" ), c_light_red );
            } else if( current_hp < maximal_hp * 0.75 ) {
                hp_str = colorize( _( "Okay" ), c_light_green );
            } else {
                hp_str = colorize( _( "Good" ), c_green );
            }
        } else {
            std::pair<std::string, nc_color> h_bar = get_hp_bar( current_hp, maximal_hp, false );
            hp_str = colorize( h_bar.first, h_bar.second ) +
                     colorize( std::string( 5 - utf8_width( h_bar.first ), '.' ), c_white );

            if( precise ) {
                hp_str = string_format( "%s %3d/%d", hp_str, current_hp, maximal_hp );
            }
        }
        msg += colorize( aligned_name, all_state_col ) + " " + hp_str;

        // BLEEDING block
        if( bleeding ) {
            desc += string_format( _( "Bleeding: %s" ),
                                   colorize( get_effect( effect_bleed, bp ).get_speed_name(),
                                             colorize_bleeding_intensity( get_effect_int( effect_bleed, bp ) ) ) );
            if( bleed > 0 ) {
                int percent = static_cast<int>( bleed * 100 / get_effect_int( effect_bleed, bp ) );
                percent = std::min( percent, 100 );
                desc += " -> " + colorize( string_format( _( "%d %% improvement" ), percent ), c_green );
            }
            desc += "\n";
        }

        // BANDAGE block
        if( e.allowed && ( new_b_power > 0 || b_power > 0 ) ) {
            desc += string_format( _( "Bandaged: %s" ), texitify_healing_power( b_power ) );
            if( new_b_power > 0 ) {
                desc += string_format( " -> %s", texitify_healing_power( new_b_power ) );
                if( new_b_power <= b_power ) {
                    desc += _( " (no improvement)" );
                }
            }
            desc += "\n";
        }

        // DISINFECTANT block
        if( e.allowed && ( d_power > 0 || new_d_power > 0 ) ) {
            desc += string_format( _( "Disinfected: %s" ), texitify_healing_power( d_power ) );
            if( new_d_power > 0 ) {
                desc += string_format( " -> %s",  texitify_healing_power( new_d_power ) );
                if( new_d_power <= d_power ) {
                    desc += _( " (no improvement)" );
                }
            }
            desc += "\n";
        }

        // BITTEN block
        if( bitten ) {
            desc += string_format( "%s: ", get_effect( effect_bite, bp ).get_speed_name() );
            if( bite > 0 ) {
                desc += colorize( string_format( _( "Chance to clean and disinfect: %d %%" ),
                                                 static_cast<int>( bite * 100 ) ), c_light_green );
            } else {
                desc += colorize( _( "It has a deep bite wound that needs cleaning." ), c_red );
            }
            desc += "\n";
        }

        // INFECTED block
        if( infected ) {
            desc += string_format( "%s: ", get_effect( effect_infected, bp ).get_speed_name() );
            if( infect > 0 ) {
                desc += colorize( string_format( _( "Chance to cure infection: %d %%" ),
                                                 static_cast<int>( infect * 100 ) ), c_light_green ) + "\n";
            } else {
                desc += colorize( _( "It has a deep wound that looks infected.  Antibiotics might be required." ),
                                  c_red );
            }
            desc += "\n";
        }
        // END of blocks

        if( ( !e.allowed && !limb_is_broken ) || ( show_all && current_hp == maximal_hp &&
                !limb_is_broken && !bitten && !infected && !bleeding ) ) {
            desc += colorize( _( "Healthy." ), c_green ) + "\n";
        }
        if( !e.allowed ) {
            desc += colorize( _( "You don't expect any effect from using this." ), c_yellow );
        } else {
            is_valid_choice = true;
        }
        bmenu.addentry_desc( i, e.allowed, MENU_AUTOASSIGN, msg, desc );
    }

    if( !is_valid_choice ) { // no body part can be chosen for this item/effect
        bmenu.init();
        bmenu.desc_enabled = false;
        bmenu.text = _( "No limb would benefit from it." );
        bmenu.addentry( parts.size(), true, 'q', "%s", _( "Cancel" ) );
    }

    bmenu.query();
    if( bmenu.ret >= 0 && static_cast<size_t>( bmenu.ret ) < parts.size() &&
        parts[bmenu.ret].allowed ) {
        return parts[bmenu.ret].bp;
    } else {
        return bodypart_str_id::NULL_ID();
    }
}

nc_color Character::limb_color( const bodypart_id &bp, bool bleed, bool bite, bool infect ) const
{
    if( bp == bodypart_str_id::NULL_ID() ) {
        return c_light_gray;
    }
    int color_bit = 0;
    nc_color i_color = c_light_gray;
    const int intense = get_effect_int( effect_bleed, bp );
    if( bleed && intense > 0 ) {
        color_bit += 1;
    }
    if( bite && has_effect( effect_bite, bp.id() ) ) {
        color_bit += 10;
    }
    if( infect && has_effect( effect_infected, bp.id() ) ) {
        color_bit += 100;
    }
    switch( color_bit ) {
        case 1:
            i_color = colorize_bleeding_intensity( intense );
            break;
        case 10:
            i_color = c_blue;
            break;
        case 100:
            i_color = c_green;
            break;
        case 11:
            if( intense < 21 ) {
                i_color = c_magenta;
            } else {
                i_color = c_magenta_red;
            }
            break;
        case 101:
            if( intense < 21 ) {
                i_color = c_yellow;
            } else {
                i_color = c_yellow_red;
            }
            break;
    }

    return i_color;
}

std::string Character::get_name() const
{
    return name;
}

std::vector<std::string> Character::get_grammatical_genders() const
{
    if( male ) {
        return { "m" };
    } else {
        return { "f" };
    }
}

nc_color Character::symbol_color() const
{
    nc_color basic = basic_symbol_color();

    if( has_effect( effect_downed ) ) {
        return hilite( basic );
    } else if( has_effect( effect_grabbed ) ) {
        return cyan_background( basic );
    }

    const auto &fields = get_map().field_at( pos() );

    // Priority: electricity, fire, acid, gases
    bool has_elec = false;
    bool has_fire = false;
    bool has_acid = false;
    bool has_fume = false;
    for( const auto &field : fields ) {
        has_elec = field.first.obj().has_elec;
        if( has_elec ) {
            return hilite( basic );
        }
        has_fire = field.first.obj().has_fire;
        has_acid = field.first.obj().has_acid;
        has_fume = field.first.obj().has_fume;
    }
    if( has_fire ) {
        return red_background( basic );
    }
    if( has_acid ) {
        return green_background( basic );
    }
    if( has_fume ) {
        return white_background( basic );
    }
    if( in_sleep_state() ) {
        return hilite( basic );
    }
    return basic;
}

bool Character::is_immune_field( const field_type_id &fid ) const
{
    // Obviously this makes us invincible
    if( has_trait( trait_DEBUG_NODMG ) || has_effect( effect_incorporeal ) ) {
        return true;
    }
    // Check to see if we are immune
    const field_type &ft = fid.obj();
    for( const trait_id &t : ft.immunity_data_traits ) {
        if( has_trait( t ) ) {
            return true;
        }
    }
    bool immune_by_body_part_resistance = !ft.immunity_data_body_part_env_resistance.empty();
    for( const std::pair<bodypart_str_id, int> &fide : ft.immunity_data_body_part_env_resistance ) {
        immune_by_body_part_resistance = immune_by_body_part_resistance &&
                                         get_env_resist( fide.first.id() ) >= fide.second;
    }
    if( immune_by_body_part_resistance ) {
        return true;
    }
    if( ft.has_elec ) {
        return is_elec_immune();
    }
    if( ft.has_fire ) {
        return has_active_bionic( bio_heatsink ) || is_wearing( itype_rm13_armor_on );
    }
    if( ft.has_acid ) {
        return !is_on_ground() && get_env_resist( body_part_foot_l ) >= 15 &&
               get_env_resist( body_part_foot_r ) >= 15 &&
               get_env_resist( body_part_leg_l ) >= 15 &&
               get_env_resist( body_part_leg_r ) >= 15 &&
               get_armor_type( damage_type::ACID, body_part_foot_l ) >= 5 &&
               get_armor_type( damage_type::ACID, body_part_foot_r ) >= 5 &&
               get_armor_type( damage_type::ACID, body_part_leg_l ) >= 5 &&
               get_armor_type( damage_type::ACID, body_part_leg_r ) >= 5;
    }
    // If we haven't found immunity yet fall up to the next level
    return Creature::is_immune_field( fid );
}

bool Character::is_elec_immune() const
{
    return is_immune_damage( damage_type::ELECTRIC );
}

bool Character::is_immune_effect( const efftype_id &eff ) const
{
    if( eff == effect_downed ) {
        return is_throw_immune() || ( has_trait( trait_LEG_TENT_BRACE ) && footwear_factor() == 0 );
    } else if( eff == effect_onfire ) {
        return is_immune_damage( damage_type::HEAT );
    } else if( eff == effect_deaf ) {
        return worn_with_flag( flag_DEAF ) || worn_with_flag( flag_PARTIAL_DEAF ) ||
               has_flag( json_flag_IMMUNE_HEARING_DAMAGE ) ||
               is_wearing( itype_rm13_armor_on );
    } else if( eff == effect_mute ) {
        return has_bionic( bio_voice );
    } else if( eff == effect_corroding ) {
        return is_immune_damage( damage_type::ACID ) || has_trait( trait_SLIMY ) ||
               has_trait( trait_VISCOUS );
    } else if( eff == effect_nausea ) {
        return has_trait( trait_STRONGSTOMACH );
    }

    return false;
}

bool Character::is_immune_damage( const damage_type dt ) const
{
    switch( dt ) {
        case damage_type::NONE:
            return true;
        case damage_type::PURE:
            return false;
        case damage_type::BIOLOGICAL:
            return has_flag( json_flag_BIO_IMMUNE ) ||
                   has_effect_with_flag( flag_EFFECT_BIO_IMMUNE ) ||
                   worn_with_flag( flag_BIO_IMMUNE );
        case damage_type::BASH:
            return has_flag( json_flag_BASH_IMMUNE ) ||
                   has_effect_with_flag( flag_EFFECT_BASH_IMMUNE ) ||
                   worn_with_flag( flag_BASH_IMMUNE );
        case damage_type::CUT:
            return has_flag( json_flag_CUT_IMMUNE ) ||
                   has_effect_with_flag( flag_EFFECT_CUT_IMMUNE ) ||
                   worn_with_flag( flag_CUT_IMMUNE );
        case damage_type::ACID:
            return has_flag( json_flag_ACID_IMMUNE ) ||
                   has_effect_with_flag( flag_EFFECT_ACID_IMMUNE ) ||
                   worn_with_flag( flag_ACID_IMMUNE );
        case damage_type::STAB:
            return has_flag( json_flag_STAB_IMMUNE ) ||
                   has_effect_with_flag( flag_EFFECT_STAB_IMMUNE ) ||
                   worn_with_flag( flag_STAB_IMMUNE );
        case damage_type::BULLET:
            return has_flag( json_flag_BULLET_IMMUNE ) ||
                   has_effect_with_flag( flag_EFFECT_BULLET_IMMUNE ) ||
                   worn_with_flag( flag_BULLET_IMMUNE );
        case damage_type::HEAT:
            return has_flag( json_flag_HEATPROOF ) ||
                   has_effect_with_flag( flag_EFFECT_HEAT_IMMUNE ) ||
                   worn_with_flag( flag_HEAT_IMMUNE );
        case damage_type::COLD:
            return has_flag( json_flag_COLD_IMMUNE ) ||
                   has_effect_with_flag( flag_EFFECT_COLD_IMMUNE ) ||
                   worn_with_flag( flag_COLD_IMMUNE );
        case damage_type::ELECTRIC:
            return has_flag( json_flag_ELECTRIC_IMMUNE ) ||
                   worn_with_flag( flag_ELECTRIC_IMMUNE ) ||
                   has_effect_with_flag( flag_EFFECT_ELECTRIC_IMMUNE );
        default:
            return true;
    }
}

bool Character::is_rad_immune() const
{
    bool has_helmet = false;
    return ( is_wearing_power_armor( &has_helmet ) && has_helmet ) || worn_with_flag( flag_RAD_PROOF );
}

int Character::throw_range( const item &it ) const
{
    if( it.is_null() ) {
        return -1;
    }

    item tmp = it;

    if( tmp.count_by_charges() && tmp.charges > 1 ) {
        tmp.charges = 1;
    }

    /** @EFFECT_STR determines maximum weight that can be thrown */
    if( ( tmp.weight() / 113_gram ) > static_cast<int>( str_cur * 15 ) ) {
        return 0;
    }
    // Increases as weight decreases until 150 g, then decreases again
    /** @EFFECT_STR increases throwing range, vs item weight (high or low) */
    int str_override = str_cur;
    if( is_mounted() ) {
        auto *mons = mounted_creature.get();
        str_override = mons->mech_str_addition() != 0 ? mons->mech_str_addition() : str_cur;
    }
    int ret = ( str_override * 10 ) / ( tmp.weight() >= 150_gram ? tmp.weight() / 113_gram : 10 -
                                        static_cast<int>(
                                            tmp.weight() / 15_gram ) );
    ret -= tmp.volume() / 1_liter;
    static const std::set<material_id> affected_materials = { material_id( "iron" ), material_id( "steel" ) };
    if( has_active_bionic( bio_railgun ) && tmp.made_of_any( affected_materials ) ) {
        ret *= 2;
    }
    if( ret < 1 ) {
        return 1;
    }
    // Cap at double our strength + skill
    /** @EFFECT_STR caps throwing range */

    /** @EFFECT_THROW caps throwing range */
    if( ret > str_override * 3 + get_skill_level( skill_throw ) ) {
        return str_override * 3 + get_skill_level( skill_throw );
    }

    return ret;
}

const std::vector<material_id> Character::fleshy = { material_id( "flesh" ), material_id( "hflesh" ) };
bool Character::made_of( const material_id &m ) const
{
    // TODO: check for mutations that change this.
    return std::find( fleshy.begin(), fleshy.end(), m ) != fleshy.end();
}
bool Character::made_of_any( const std::set<material_id> &ms ) const
{
    // TODO: check for mutations that change this.
    return std::any_of( fleshy.begin(), fleshy.end(), [&ms]( const material_id & e ) {
        return ms.count( e );
    } );
}

tripoint Character::global_square_location() const
{
    return get_map().getabs( position );
}

tripoint Character::global_sm_location() const
{
    return ms_to_sm_copy( global_square_location() );
}

tripoint_abs_omt Character::global_omt_location() const
{
    // TODO: fix point types
    return tripoint_abs_omt( ms_to_omt_copy( global_square_location() ) );
}

bool Character::is_blind() const
{
    return ( worn_with_flag( flag_BLIND ) ||
             has_effect( effect_blind ) ||
             has_flag( json_flag_BLIND ) );
}

bool Character::is_invisible() const
{
    return (
               has_effect_with_flag( flag_EFFECT_INVISIBLE ) ||
               is_wearing_active_optcloak() ||
               has_trait( trait_DEBUG_CLOAK )
           );
}

int Character::visibility( bool, int ) const
{
    // 0-100 %
    if( is_invisible() ) {
        return 0;
    }
    // TODO:
    // if ( dark_clothing() && light check ...
    int stealth_modifier = std::floor( mutation_value( "stealth_modifier" ) );
    return clamp( 100 - stealth_modifier, 40, 160 );
}

/*
 * Calculate player brightness based on the brightest active item, as
 * per itype tag LIGHT_* and optional CHARGEDIM ( fade starting at 20% charge )
 * item.light.* is -unimplemented- for the moment, as it is a custom override for
 * applying light sources/arcs with specific angle and direction.
 */
float Character::active_light() const
{
    float lumination = 0.0f;

    int maxlum = 0;
    has_item_with( [&maxlum]( const item & it ) {
        const int lumit = it.getlight_emit();
        if( maxlum < lumit ) {
            maxlum = lumit;
        }
        return false; // continue search, otherwise has_item_with would cancel the search
    } );

    lumination = static_cast<float>( maxlum );

    float mut_lum = 0.0f;
    for( const trait_id &mut : get_mutations() ) {
        float curr_lum = 0.0f;
        for( const std::pair<const bodypart_str_id, float> &elem : mut->lumination ) {
            int coverage = 0;
            for( const item &i : worn ) {
                if( i.covers( elem.first.id() ) && !i.has_flag( flag_ALLOWS_NATURAL_ATTACKS ) &&
                    !i.has_flag( flag_SEMITANGIBLE ) &&
                    !i.has_flag( flag_PERSONAL ) && !i.has_flag( flag_AURA ) ) {
                    coverage += i.get_coverage( elem.first.id() );
                }
            }
            curr_lum += elem.second * ( 1 - ( coverage / 100.0f ) );
        }
        mut_lum += curr_lum;
    }

    lumination = std::max( lumination, mut_lum );

    if( lumination < 60 && has_active_bionic( bio_flashlight ) ) {
        lumination = 60;
    } else if( lumination < 5 && ( has_effect( effect_glowing ) ||
                                   ( has_active_bionic( bio_tattoo_led ) ||
                                     has_effect( effect_glowy_led ) ) ) ) {
        lumination = 5;
    }
    return lumination;
}

bool Character::sees_with_specials( const Creature &critter ) const
{
    // electroreceptors grants vision of robots and electric monsters through walls
    if( has_trait( trait_ELECTRORECEPTORS ) &&
        ( critter.in_species( species_ROBOT ) || critter.has_flag( MF_ELECTRIC ) ) ) {
        return true;
    }

    if( critter.digging() && has_active_bionic( bio_ground_sonar ) ) {
        // Bypass the check below, the bionic sonar also bypasses the sees(point) check because
        // walls don't block sonar which is transmitted in the ground, not the air.
        // TODO: this might need checks whether the player is in the air, or otherwise not connected
        // to the ground. It also might need a range check.
        return true;
    }

    return false;
}

bool Character::pour_into( item &container, item &liquid )
{
    std::string err;
    const int amount = container.get_remaining_capacity_for_liquid( liquid, *this, &err );

    if( !err.empty() ) {
        if( !container.has_item_with( [&liquid]( const item & it ) {
        return it.typeId() == liquid.typeId();
        } ) ) {
            add_msg_if_player( m_bad, err );
        } else {
            //~ you filled <container> to the brim with <liquid>
            add_msg_if_player( _( "You filled %1$s to the brim with %2$s." ), container.tname(),
                               liquid.tname() );
        }
        return false;
    }

    add_msg_if_player( _( "You pour %1$s into the %2$s." ), liquid.tname(), container.tname() );

    liquid.charges -= container.fill_with( liquid, amount );
    inv->unsort();

    if( liquid.charges > 0 ) {
        add_msg_if_player( _( "There's some left over!" ) );
    }

    return true;
}

bool Character::pour_into( const vpart_reference &vp, item &liquid ) const
{
    if( !vp.part().fill_with( liquid ) ) {
        return false;
    }

    //~ $1 - vehicle name, $2 - part name, $3 - liquid type
    add_msg_if_player( _( "You refill the %1$s's %2$s with %3$s." ),
                       vp.vehicle().name, vp.part().name(), liquid.type_name() );

    if( liquid.charges > 0 ) {
        add_msg_if_player( _( "There's some left over!" ) );
    }
    return true;
}

resistances Character::mutation_armor( bodypart_id bp ) const
{
    resistances res;
    for( const trait_id &iter : get_mutations() ) {
        res += iter->damage_resistance( bp );
    }

    return res;
}

float Character::mutation_armor( bodypart_id bp, damage_type dt ) const
{
    return mutation_armor( bp ).type_resist( dt );
}

float Character::mutation_armor( bodypart_id bp, const damage_unit &du ) const
{
    return mutation_armor( bp ).get_effective_resist( du );
}

int Character::ammo_count_for( const item &gun )
{
    int ret = item::INFINITE_CHARGES;
    if( !gun.is_gun() ) {
        return ret;
    }

    int required = gun.ammo_required();

    if( required > 0 ) {
        int total_ammo = 0;
        total_ammo += gun.ammo_remaining();

        bool has_mag = gun.magazine_integral();

        const auto found_ammo = find_ammo( gun, true, -1 );
        int loose_ammo = 0;
        for( const auto &ammo : found_ammo ) {
            if( ammo->is_magazine() ) {
                has_mag = true;
                total_ammo += ammo->ammo_remaining();
            } else if( ammo->is_ammo() ) {
                loose_ammo += ammo->charges;
            }
        }

        if( has_mag ) {
            total_ammo += loose_ammo;
        }

        ret = std::min( ret, total_ammo / required );
    }

    int ups_drain = gun.get_gun_ups_drain();
    if( ups_drain > 0 ) {
        ret = std::min( ret, charges_of( itype_UPS ) / ups_drain );
    }

    return ret;
}

bool Character::can_reload( const item &it, const itype_id &ammo ) const
{
    if( !it.is_reloadable_with( ammo ) ) {
        return false;
    }

    if( it.is_ammo_belt() ) {
        const cata::optional<itype_id> &linkage = it.type->magazine->linkage;
        if( linkage && !has_charges( *linkage, 1 ) ) {
            return false;
        }
    }

    return true;
}

hint_rating Character::rate_action_reload( const item &it ) const
{
    hint_rating res = hint_rating::cant;

    // Guns may contain additional reloadable mods so check these first
    for( const item *mod : it.gunmods() ) {
        switch( rate_action_reload( *mod ) ) {
            case hint_rating::good:
                return hint_rating::good;

            case hint_rating::cant:
                continue;

            case hint_rating::iffy:
                res = hint_rating::iffy;
        }
    }

    if( !it.is_reloadable() ) {
        return res;
    }

    return can_reload( it ) ? hint_rating::good : hint_rating::iffy;
}

hint_rating Character::rate_action_unload( const item &it ) const
{
    if( it.is_container() && !it.contents.empty() &&
        it.can_unload_liquid() ) {
        return hint_rating::good;
    }

    if( it.has_flag( flag_NO_UNLOAD ) ) {
        return hint_rating::cant;
    }

    if( it.magazine_current() ) {
        return hint_rating::good;
    }

    for( const item *e : it.gunmods() ) {
        if( ( e->is_gun() && !e->has_flag( flag_NO_UNLOAD ) &&
              ( e->magazine_current() || e->ammo_remaining() > 0 || e->casings_count() > 0 ) ) ||
            ( e->has_flag( flag_BRASS_CATCHER ) && !e->is_container_empty() ) ) {
            return hint_rating::good;
        }
    }

    if( it.ammo_types().empty() ) {
        return hint_rating::cant;
    }

    if( it.ammo_remaining() > 0 || it.casings_count() > 0 ) {
        return hint_rating::good;
    }

    return hint_rating::iffy;
}

float Character::rest_quality() const
{
    // Just a placeholder for now.
    // TODO: Waiting/reading/being unconscious on bed/sofa/grass
    return has_effect( effect_sleep ) ? 1.0f : 0.0f;
}

std::string Character::extended_description() const
{
    std::string ss;
    if( is_player() ) {
        // <bad>This is me, <player_name>.</bad>
        ss += string_format( _( "This is you - %s." ), name );
    } else {
        ss += string_format( _( "This is %s." ), name );
    }

    ss += "\n--\n";

    const std::vector<bodypart_id> &bps = get_all_body_parts( get_body_part_flags::only_main );
    // Find length of bp names, to align
    // accumulate looks weird here, any better function?
    int longest = std::accumulate( bps.begin(), bps.end(), 0,
    []( int m, bodypart_id bp ) {
        return std::max( m, utf8_width( body_part_name_as_heading( bp, 1 ) ) );
    } );

    // This is a stripped-down version of the body_window function
    // This should be extracted into a separate function later on
    for( const bodypart_id &bp : bps ) {
        const std::string &bp_heading = body_part_name_as_heading( bp, 1 );

        const nc_color state_col = limb_color( bp, true, true, true );
        nc_color name_color = state_col;
        std::pair<std::string, nc_color> hp_bar = get_hp_bar( get_part_hp_cur( bp ), get_part_hp_max( bp ),
                false );

        ss += colorize( left_justify( bp_heading, longest ), name_color );
        ss += colorize( hp_bar.first, hp_bar.second );
        // Trailing bars. UGLY!
        // TODO: Integrate into get_hp_bar somehow
        ss += colorize( std::string( 5 - utf8_width( hp_bar.first ), '.' ), c_white );
        ss += "\n";
    }

    ss += "--\n";
    ss += _( "Wielding:" ) + std::string( " " );
    if( weapon.is_null() ) {
        ss += _( "Nothing" );
    } else {
        ss += weapon.tname();
    }

    ss += "\n";
    ss += _( "Wearing:" ) + std::string( " " );
    ss += enumerate_as_string( worn.begin(), worn.end(), []( const item & it ) {
        return it.tname();
    } );

    return replace_colors( ss );
}

social_modifiers Character::get_mutation_bionic_social_mods() const
{
    social_modifiers mods;
    for( const mutation_branch *mut : cached_mutations ) {
        mods += mut->social_mods;
    }
    for( const bionic &bio : *my_bionics ) {
        mods += bio.info().social_mods;
    }
    return mods;
}

template <cata::optional<float> mutation_branch::*member>
float calc_mutation_value( const std::vector<const mutation_branch *> &mutations )
{
    float lowest = 0.0f;
    float highest = 0.0f;
    for( const mutation_branch *mut : mutations ) {
        if( ( mut->*member ).has_value() ) {
            float val = ( mut->*member ).value();
            lowest = std::min( lowest, val );
            highest = std::max( highest, val );
        }
    }

    return std::min( 0.0f, lowest ) + std::max( 0.0f, highest );
}

template <cata::optional<float> mutation_branch::*member>
float calc_mutation_value_additive( const std::vector<const mutation_branch *> &mutations )
{
    float ret = 0.0f;
    for( const mutation_branch *mut : mutations ) {
        if( ( mut->*member ).has_value() ) {
            ret += ( mut->*member ).value();
        }
    }
    return ret;
}

template <cata::optional<float> mutation_branch::*member>
float calc_mutation_value_multiplicative( const std::vector<const mutation_branch *> &mutations )
{
    float ret = 1.0f;
    for( const mutation_branch *mut : mutations ) {
        if( ( mut->*member ).has_value() ) {
            ret *= ( mut->*member ).value();
        }
    }
    return ret;
}

static const std::map<std::string, std::function <float( std::vector<const mutation_branch *> )>>
mutation_value_map = {
    { "healing_awake", calc_mutation_value<&mutation_branch::healing_awake> },
    { "healing_resting", calc_mutation_value<&mutation_branch::healing_resting> },
    { "mending_modifier", calc_mutation_value_multiplicative<&mutation_branch::mending_modifier> },
    { "hp_modifier", calc_mutation_value<&mutation_branch::hp_modifier> },
    { "hp_modifier_secondary", calc_mutation_value<&mutation_branch::hp_modifier_secondary> },
    { "hp_adjustment", calc_mutation_value<&mutation_branch::hp_adjustment> },
    { "temperature_speed_modifier", calc_mutation_value<&mutation_branch::temperature_speed_modifier> },
    { "metabolism_modifier", calc_mutation_value<&mutation_branch::metabolism_modifier> },
    { "thirst_modifier", calc_mutation_value<&mutation_branch::thirst_modifier> },
    { "fatigue_regen_modifier", calc_mutation_value<&mutation_branch::fatigue_regen_modifier> },
    { "fatigue_modifier", calc_mutation_value<&mutation_branch::fatigue_modifier> },
    { "stamina_regen_modifier", calc_mutation_value<&mutation_branch::stamina_regen_modifier> },
    { "stealth_modifier", calc_mutation_value<&mutation_branch::stealth_modifier> },
    { "str_modifier", calc_mutation_value<&mutation_branch::str_modifier> },
    { "dodge_modifier", calc_mutation_value_additive<&mutation_branch::dodge_modifier> },
    { "mana_modifier", calc_mutation_value_additive<&mutation_branch::mana_modifier> },
    { "mana_multiplier", calc_mutation_value_multiplicative<&mutation_branch::mana_multiplier> },
    { "mana_regen_multiplier", calc_mutation_value_multiplicative<&mutation_branch::mana_regen_multiplier> },
    { "bionic_mana_penalty", calc_mutation_value_multiplicative<&mutation_branch::bionic_mana_penalty> },
    { "casting_time_multiplier", calc_mutation_value_multiplicative<&mutation_branch::casting_time_multiplier> },
    { "speed_modifier", calc_mutation_value_multiplicative<&mutation_branch::speed_modifier> },
    { "movecost_modifier", calc_mutation_value_multiplicative<&mutation_branch::movecost_modifier> },
    { "movecost_flatground_modifier", calc_mutation_value_multiplicative<&mutation_branch::movecost_flatground_modifier> },
    { "movecost_obstacle_modifier", calc_mutation_value_multiplicative<&mutation_branch::movecost_obstacle_modifier> },
    { "attackcost_modifier", calc_mutation_value_multiplicative<&mutation_branch::attackcost_modifier> },
    { "max_stamina_modifier", calc_mutation_value_multiplicative<&mutation_branch::max_stamina_modifier> },
    { "weight_capacity_modifier", calc_mutation_value_multiplicative<&mutation_branch::weight_capacity_modifier> },
    { "hearing_modifier", calc_mutation_value_multiplicative<&mutation_branch::hearing_modifier> },
    { "movecost_swim_modifier", calc_mutation_value_multiplicative<&mutation_branch::movecost_swim_modifier> },
    { "noise_modifier", calc_mutation_value_multiplicative<&mutation_branch::noise_modifier> },
    { "overmap_sight", calc_mutation_value_additive<&mutation_branch::overmap_sight> },
    { "overmap_multiplier", calc_mutation_value_multiplicative<&mutation_branch::overmap_multiplier> },
    { "map_memory_capacity_multiplier", calc_mutation_value_multiplicative<&mutation_branch::map_memory_capacity_multiplier> },
    { "reading_speed_multiplier", calc_mutation_value_multiplicative<&mutation_branch::reading_speed_multiplier> },
    { "skill_rust_multiplier", calc_mutation_value_multiplicative<&mutation_branch::skill_rust_multiplier> },
    { "crafting_speed_multiplier", calc_mutation_value_multiplicative<&mutation_branch::crafting_speed_multiplier> },
    { "obtain_cost_multiplier", calc_mutation_value_multiplicative<&mutation_branch::obtain_cost_multiplier> },
    { "stomach_size_multiplier", calc_mutation_value_multiplicative<&mutation_branch::stomach_size_multiplier> },
    { "vomit_multiplier", calc_mutation_value_multiplicative<&mutation_branch::vomit_multiplier> },
    { "consume_time_modifier", calc_mutation_value_multiplicative<&mutation_branch::consume_time_modifier> }
};

float Character::mutation_value( const std::string &val ) const
{
    // Syntax similar to tuple get<n>()
    const auto found = mutation_value_map.find( val );

    if( found == mutation_value_map.end() ) {
        debugmsg( "Invalid mutation value name %s", val );
        return 0.0f;
    } else {
        return found->second( cached_mutations );
    }
}

float Character::healing_rate( float at_rest_quality ) const
{
    // TODO: Cache
    float heal_rate;
    if( !is_npc() ) {
        heal_rate = get_option< float >( "PLAYER_HEALING_RATE" );
    } else {
        heal_rate = get_option< float >( "NPC_HEALING_RATE" );
    }
    float awake_rate = heal_rate * mutation_value( "healing_awake" );
    float final_rate = 0.0f;
    if( awake_rate > 0.0f ) {
        final_rate += awake_rate;
    } else if( at_rest_quality < 1.0f ) {
        // Resting protects from rot
        final_rate += ( 1.0f - at_rest_quality ) * awake_rate;
    }
    float asleep_rate = 0.0f;
    if( at_rest_quality > 0.0f ) {
        asleep_rate = at_rest_quality * heal_rate * ( 1.0f + mutation_value( "healing_resting" ) );
    }
    if( asleep_rate > 0.0f ) {
        final_rate += asleep_rate * ( 1.0f + get_healthy() / 200.0f );
    }

    // Most common case: awake player with no regenerative abilities
    // ~7e-5 is 1 hp per day, anything less than that is totally negligible
    static constexpr float eps = 0.000007f;
    add_msg_debug( "%s healing: %.6f", name, final_rate );
    if( std::abs( final_rate ) < eps ) {
        return 0.0f;
    }

    float primary_hp_mod = mutation_value( "hp_modifier" );
    if( primary_hp_mod < 0.0f ) {
        // HP mod can't get below -1.0
        final_rate *= 1.0f + primary_hp_mod;
    }

    return enchantment_cache->modify_value( enchant_vals::mod::REGEN_HP, final_rate );
}

float Character::healing_rate_medicine( float at_rest_quality, const bodypart_id &bp ) const
{
    float rate_medicine = 0.0f;

    for( const auto &elem : *effects ) {
        for( const std::pair<const bodypart_id, effect> &i : elem.second ) {
            const effect &eff = i.second;
            float tmp_rate = static_cast<float>( eff.get_amount( "HEAL_RATE" ) ) / to_turns<int>
                             ( 24_hours );

            if( bp == body_part_head ) {
                tmp_rate *= eff.get_amount( "HEAL_HEAD" ) / 100.0f;
            }
            if( bp == body_part_torso ) {
                tmp_rate *= eff.get_amount( "HEAL_TORSO" ) / 100.0f;
            }
            rate_medicine += tmp_rate;
        }
    }

    rate_medicine *= 1.0f + mutation_value( "healing_resting" );
    rate_medicine *= 1.0f + at_rest_quality;

    // increase healing if character has both effects
    if( has_effect( effect_bandaged ) && has_effect( effect_disinfected ) ) {
        rate_medicine *= 1.25;
    }

    if( get_healthy() > 0.0f ) {
        rate_medicine *= 1.0f + get_healthy() / 200.0f;
    } else {
        rate_medicine *= 1.0f + get_healthy() / 400.0f;
    }
    float primary_hp_mod = mutation_value( "hp_modifier" );
    if( primary_hp_mod < 0.0f ) {
        // HP mod can't get below -1.0
        rate_medicine *= 1.0f + primary_hp_mod;
    }
    return rate_medicine;
}

float Character::get_bmi() const
{
    return 12 * get_kcal_percent() + 13;
}

std::string Character::get_weight_string() const
{
    std::pair<std::string, nc_color> weight_pair = get_weight_description();
    return colorize( weight_pair.first, weight_pair.second );
}

std::pair<std::string, nc_color> Character::get_weight_description() const
{
    const float bmi = get_bmi();
    std::string weight_string;
    nc_color weight_color = c_light_gray;
    if( get_option<bool>( "CRAZY" ) ) {
        if( bmi > character_weight_category::morbidly_obese + 10.0f ) {
            weight_string = translate_marker( "AW HELL NAH" );
            weight_color = c_red;
        } else if( bmi > character_weight_category::morbidly_obese + 5.0f ) {
            weight_string = translate_marker( "DAYUM" );
            weight_color = c_red;
        } else if( bmi > character_weight_category::morbidly_obese ) {
            weight_string = translate_marker( "Fluffy" );
            weight_color = c_red;
        } else if( bmi > character_weight_category::very_obese ) {
            weight_string = translate_marker( "Husky" );
            weight_color = c_red;
        } else if( bmi > character_weight_category::obese ) {
            weight_string = translate_marker( "Healthy" );
            weight_color = c_light_red;
        } else if( bmi > character_weight_category::overweight ) {
            weight_string = translate_marker( "Big" );
            weight_color = c_yellow;
        } else if( bmi > character_weight_category::normal ) {
            weight_string = translate_marker( "Normal" );
            weight_color = c_light_gray;
        } else if( bmi > character_weight_category::underweight ) {
            weight_string = translate_marker( "Bean Pole" );
            weight_color = c_yellow;
        } else if( bmi > character_weight_category::emaciated ) {
            weight_string = translate_marker( "Emaciated" );
            weight_color = c_light_red;
        } else {
            weight_string = translate_marker( "Spooky Scary Skeleton" );
            weight_color = c_red;
        }
    } else {
        if( bmi > character_weight_category::morbidly_obese ) {
            weight_string = translate_marker( "Morbidly Obese" );
            weight_color = c_red;
        } else if( bmi > character_weight_category::very_obese ) {
            weight_string = translate_marker( "Very Obese" );
            weight_color = c_red;
        } else if( bmi > character_weight_category::obese ) {
            weight_string = translate_marker( "Obese" );
            weight_color = c_light_red;
        } else if( bmi > character_weight_category::overweight ) {
            weight_string = translate_marker( "Overweight" );
            weight_color = c_yellow;
        } else if( bmi > character_weight_category::normal ) {
            weight_string = translate_marker( "Normal" );
            weight_color = c_light_gray;
        } else if( bmi > character_weight_category::underweight ) {
            weight_string = translate_marker( "Underweight" );
            weight_color = c_yellow;
        } else if( bmi > character_weight_category::emaciated ) {
            weight_string = translate_marker( "Emaciated" );
            weight_color = c_light_red;
        } else {
            weight_string = translate_marker( "Skeletal" );
            weight_color = c_red;
        }
    }
    return std::make_pair( _( weight_string ), weight_color );
}

std::string Character::get_weight_long_description() const
{
    const float bmi = get_bmi();
    if( bmi > character_weight_category::morbidly_obese ) {
        return _( "You have far more fat than is healthy or useful.  It is causing you major problems." );
    } else if( bmi > character_weight_category::very_obese ) {
        return _( "You have too much fat.  It impacts your day-to-day health and wellness." );
    } else if( bmi > character_weight_category::obese ) {
        return _( "You've definitely put on a lot of extra weight.  Although helpful in times of famine, this is too much and is impacting your health." );
    } else if( bmi > character_weight_category::overweight ) {
        return _( "You've put on some extra pounds.  Nothing too excessive, but it's starting to impact your health and waistline a bit." );
    } else if( bmi > character_weight_category::normal ) {
        return _( "You look to be a pretty healthy weight, with some fat to last you through the winter, but nothing excessive." );
    } else if( bmi > character_weight_category::underweight ) {
        return _( "You are thin, thinner than is healthy.  You are less resilient to going without food." );
    } else if( bmi > character_weight_category::emaciated ) {
        return _( "You are very unhealthily underweight, nearing starvation." );
    } else {
        return _( "You have very little meat left on your bones.  You appear to be starving." );
    }
}

units::mass Character::bodyweight() const
{
    return units::from_kilogram( get_bmi() * std::pow( height() / 100.0f, 2 ) );
}

units::mass Character::bionics_weight() const
{
    units::mass bio_weight = 0_gram;
    for( const bionic_id &bid : get_bionics() ) {
        if( !bid->included ) {
            bio_weight += item::find_type( bid->itype() )->weight;
        }
    }
    return bio_weight;
}

void Character::reset_chargen_attributes()
{
    init_age = 25;
    init_height = 175;
}

int Character::base_age() const
{
    return init_age;
}

void Character::set_base_age( int age )
{
    init_age = age;
}

void Character::mod_base_age( int mod )
{
    init_age += mod;
}

int Character::age() const
{
    int years_since_cataclysm = to_turns<int>( calendar::turn - calendar::turn_zero ) /
                                to_turns<int>( calendar::year_length() );
    return init_age + years_since_cataclysm;
}

std::string Character::age_string() const
{
    //~ how old the character is in years. try to limit number of characters to fit on the screen
    std::string unformatted = _( "%d years" );
    return string_format( unformatted, age() );
}

int Character::base_height() const
{
    return init_height;
}

void Character::set_base_height( int height )
{
    init_height = height;
}

void Character::mod_base_height( int mod )
{
    init_height += mod;
}

std::string Character::height_string() const
{
    const bool metric = get_option<std::string>( "DISTANCE_UNITS" ) == "metric";

    if( metric ) {
        std::string metric_string = _( "%d cm" );
        return string_format( metric_string, height() );
    }

    int total_inches = std::round( height() / 2.54 );
    int feet = std::floor( total_inches / 12 );
    int remainder_inches = total_inches % 12;
    return string_format( "%d\'%d\"", feet, remainder_inches );
}

int Character::height() const
{
    switch( get_size() ) {
        case creature_size::tiny:
            return init_height - 100;
        case creature_size::small:
            return init_height - 50;
        case creature_size::medium:
            return init_height;
        case creature_size::large:
            return init_height + 50;
        case creature_size::huge:
            return init_height + 100;
        case creature_size::num_sizes:
            debugmsg( "ERROR: Character has invalid size class." );
            return 0;
    }

    debugmsg( "Invalid size class" );
    abort();
}

int Character::base_bmr() const
{
    /**
    Values are for males, and average!
    */
    const int equation_constant = 5;
    const int weight_factor = units::to_gram<int>( bodyweight() / 100.0 );
    const int height_factor = 6.25 * height();
    const int age_factor = 5 * age();
    return metabolic_rate_base() * ( weight_factor + height_factor - age_factor + equation_constant );
}

int Character::get_bmr() const
{
    int base_bmr_calc = base_bmr();
    base_bmr_calc *= activity_level();
    return std::ceil( enchantment_cache->modify_value( enchant_vals::mod::METABOLISM, base_bmr_calc ) );
}

void Character::increase_activity_level( float new_level )
{
    if( attempted_activity_level < new_level ) {
        attempted_activity_level = new_level;
    }
    log_instant_activity( new_level );
}

void Character::decrease_activity_level( float new_level )
{
    if( attempted_activity_level > new_level ) {
        attempted_activity_level = new_level;
    }
    log_instant_activity( new_level );
}
void Character::reset_activity_level()
{
    attempted_activity_level = NO_EXERCISE;
    reset_activity_cursor();
}

std::string Character::activity_level_str() const
{
    if( attempted_activity_level <= NO_EXERCISE ) {
        return _( "NO_EXERCISE" );
    } else if( attempted_activity_level <= LIGHT_EXERCISE ) {
        return _( "LIGHT_EXERCISE" );
    } else if( attempted_activity_level <= MODERATE_EXERCISE ) {
        return _( "MODERATE_EXERCISE" );
    } else if( attempted_activity_level <= BRISK_EXERCISE ) {
        return _( "BRISK_EXERCISE" );
    } else if( attempted_activity_level <= ACTIVE_EXERCISE ) {
        return _( "ACTIVE_EXERCISE" );
    } else {
        return _( "EXTRA_EXERCISE" );
    }
}

int Character::get_armor_bash( bodypart_id bp ) const
{
    return get_armor_bash_base( bp ) + armor_bash_bonus;
}

int Character::get_armor_cut( bodypart_id bp ) const
{
    return get_armor_cut_base( bp ) + armor_cut_bonus;
}

int Character::get_armor_bullet( bodypart_id bp ) const
{
    return get_armor_bullet_base( bp ) + armor_bullet_bonus;
}

int Character::get_armor_type( damage_type dt, bodypart_id bp ) const
{
    switch( dt ) {
        case damage_type::PURE:
        case damage_type::BIOLOGICAL:
            return 0;
        case damage_type::BASH:
            return get_armor_bash( bp );
        case damage_type::CUT:
            return get_armor_cut( bp );
        case damage_type::STAB:
            return get_armor_cut( bp ) * 0.8f;
        case damage_type::BULLET:
            return get_armor_bullet( bp );
        case damage_type::ACID:
        case damage_type::HEAT:
        case damage_type::COLD:
        case damage_type::ELECTRIC: {
            int ret = 0;
            for( const item &i : worn ) {
                if( i.covers( bp ) ) {
                    ret += i.damage_resist( dt );
                }
            }

            ret += mutation_armor( bp, dt );
            return ret;
        }
        case damage_type::NONE:
        case damage_type::NUM:
            // Let it error below
            break;
    }

    debugmsg( "Invalid damage type: %d", dt );
    return 0;
}

int Character::get_armor_bash_base( bodypart_id bp ) const
{
    float ret = 0;
    for( const item &i : worn ) {
        if( i.covers( bp ) ) {
            ret += i.bash_resist();
        }
    }
    for( const bionic_id &bid : get_bionics() ) {
        const auto bash_prot = bid->bash_protec.find( bp.id() );
        if( bash_prot != bid->bash_protec.end() ) {
            ret += bash_prot->second;
        }
    }

    ret += mutation_armor( bp, damage_type::BASH );
    return ret;
}

int Character::get_armor_cut_base( bodypart_id bp ) const
{
    float ret = 0;
    for( const item &i : worn ) {
        if( i.covers( bp ) ) {
            ret += i.cut_resist();
        }
    }
    for( const bionic_id &bid : get_bionics() ) {
        const auto cut_prot = bid->cut_protec.find( bp.id() );
        if( cut_prot != bid->cut_protec.end() ) {
            ret += cut_prot->second;
        }
    }

    ret += mutation_armor( bp, damage_type::CUT );
    return ret;
}

int Character::get_armor_bullet_base( bodypart_id bp ) const
{
    float ret = 0;
    for( const item &i : worn ) {
        if( i.covers( bp ) ) {
            ret += i.bullet_resist();
        }
    }

    for( const bionic_id &bid : get_bionics() ) {
        const auto bullet_prot = bid->bullet_protec.find( bp.id() );
        if( bullet_prot != bid->bullet_protec.end() ) {
            ret += bullet_prot->second;
        }
    }

    ret += mutation_armor( bp, damage_type::BULLET );
    return ret;
}

int Character::get_env_resist( bodypart_id bp ) const
{
    float ret = 0;
    for( const item &i : worn ) {
        // Head protection works on eyes too (e.g. baseball cap)
        if( i.covers( bp ) || ( bp == body_part_eyes && i.covers( body_part_head ) ) ) {
            ret += i.get_env_resist();
        }
    }

    for( const bionic_id &bid : get_bionics() ) {
        const auto EP = bid->env_protec.find( bp.id() );
        if( EP != bid->env_protec.end() ) {
            ret += EP->second;
        }
    }

    if( bp == body_part_eyes && has_trait( trait_SEESLEEP ) ) {
        ret += 8;
    }
    return ret;
}

int Character::get_armor_acid( bodypart_id bp ) const
{
    return get_armor_type( damage_type::ACID, bp );
}

int Character::get_stim() const
{
    return stim;
}

void Character::set_stim( int new_stim )
{
    stim = new_stim;
}

void Character::mod_stim( int mod )
{
    stim += mod;
}

int Character::get_rad() const
{
    return radiation;
}

void Character::set_rad( int new_rad )
{
    radiation = new_rad;
}

void Character::mod_rad( int mod )
{
    if( has_flag( json_flag_NO_RADIATION ) ) {
        return;
    }
    set_rad( std::max( 0, get_rad() + mod ) );
}

int Character::get_stamina() const
{
    return stamina;
}

int Character::get_stamina_max() const
{
    static const std::string player_max_stamina( "PLAYER_MAX_STAMINA" );
    static const std::string max_stamina_modifier( "max_stamina_modifier" );
    int maxStamina = get_option< int >( player_max_stamina );
    maxStamina *= Character::mutation_value( max_stamina_modifier );
    maxStamina = enchantment_cache->modify_value( enchant_vals::mod::MAX_STAMINA, maxStamina );
    return maxStamina;
}

void Character::set_stamina( int new_stamina )
{
    stamina = new_stamina;
}

void Character::mod_stamina( int mod )
{
    // TODO: Make NPCs smart enough to use stamina
    if( is_npc() ) {
        return;
    }
    stamina += mod;
    if( stamina < 0 ) {
        add_effect( effect_winded, 10_turns );
    }
    stamina = clamp( stamina, 0, get_stamina_max() );
}

void Character::burn_move_stamina( int moves )
{
    int overburden_percentage = 0;
    units::mass current_weight = weight_carried();
    // Make it at least 1 gram to avoid divide-by-zero warning
    units::mass max_weight = std::max( weight_capacity(), 1_gram );
    if( current_weight > max_weight ) {
        overburden_percentage = ( current_weight - max_weight ) * 100 / max_weight;
    }

    int burn_ratio = get_option<int>( "PLAYER_BASE_STAMINA_BURN_RATE" );
    for( const bionic_id &bid : get_bionic_fueled_with( item( "muscle" ) ) ) {
        if( has_active_bionic( bid ) ) {
            burn_ratio = burn_ratio * 2 - 3;
        }
    }
    burn_ratio += overburden_percentage;
    burn_ratio *= move_mode->stamina_mult();
    mod_stamina( -( ( moves * burn_ratio ) / 100.0 ) * stamina_move_cost_modifier() );
    add_msg_debug( "Stamina burn: %d", -( ( moves * burn_ratio ) / 100 ) );
    // Chance to suffer pain if overburden and stamina runs out or has trait BADBACK
    // Starts at 1 in 25, goes down by 5 for every 50% more carried
    if( ( current_weight > max_weight ) && ( has_trait( trait_BADBACK ) || get_stamina() == 0 ) &&
        one_in( 35 - 5 * current_weight / ( max_weight / 2 ) ) ) {
        add_msg_if_player( m_bad, _( "Your body strains under the weight!" ) );
        // 1 more pain for every 800 grams more (5 per extra STR needed)
        if( ( ( current_weight - max_weight ) / 800_gram > get_pain() && get_pain() < 100 ) ) {
            mod_pain( 1 );
        }
    }
}

float Character::stamina_move_cost_modifier() const
{
    // Both walk and run speed drop to half their maximums as stamina approaches 0.
    // Convert stamina to a float first to allow for decimal place carrying
    float stamina_modifier = ( static_cast<float>( get_stamina() ) / get_stamina_max() + 1 ) / 2;
    return stamina_modifier * move_mode->move_speed_mult();
}

void Character::update_stamina( int turns )
{
    static const std::string player_base_stamina_regen_rate( "PLAYER_BASE_STAMINA_REGEN_RATE" );
    static const std::string stamina_regen_modifier( "stamina_regen_modifier" );
    const float base_regen_rate = get_option<float>( player_base_stamina_regen_rate );
    const int current_stim = get_stim();
    float stamina_recovery = 0.0f;
    // Recover some stamina every turn.
    // Mutated stamina works even when winded
    // max stamina modifers from mutation also affect stamina multi
    float stamina_multiplier = std::max<float>( 0.1f, ( !has_effect( effect_winded ) ? 1.0f : 0.1f ) +
                               mutation_value( stamina_regen_modifier ) + ( mutation_value( "max_stamina_modifier" ) - 1.0f ) );
    // But mouth encumbrance interferes, even with mutated stamina.
    stamina_recovery += stamina_multiplier * std::max( 1.0f,
                        base_regen_rate - ( encumb( body_part_mouth ) / 5.0f ) );
    stamina_recovery = enchantment_cache->modify_value( enchant_vals::mod::REGEN_STAMINA,
                       stamina_recovery );
    // TODO: recovering stamina causes hunger/thirst/fatigue.
    // TODO: Tiredness slowing recovery

    // stim recovers stamina (or impairs recovery)
    if( current_stim > 0 ) {
        // TODO: Make stamina recovery with stims cost health
        stamina_recovery += std::min( 5.0f, current_stim / 15.0f );
    } else if( current_stim < 0 ) {
        // Affect it less near 0 and more near full
        // Negative stim kill at -200
        // At -100 stim it inflicts -20 malus to regen at 100%  stamina,
        // effectivly countering stamina gain of default 20,
        // at 50% stamina its -10 (50%), cuts by 25% at 25% stamina
        stamina_recovery += current_stim / 5.0f * get_stamina() / get_stamina_max();
    }

    const int max_stam = get_stamina_max();
    if( get_power_level() >= 3_kJ && has_active_bionic( bio_gills ) ) {
        int bonus = std::min<int>( units::to_kilojoule( get_power_level() ) / 3,
                                   max_stam - get_stamina() - stamina_recovery * turns );
        // so the effective recovery is up to 5x default
        bonus = std::min( bonus, 4 * static_cast<int>( base_regen_rate ) );
        if( bonus > 0 ) {
            stamina_recovery += bonus;
            bonus /= 10;
            bonus = std::max( bonus, 1 );
            mod_power_level( units::from_kilojoule( -bonus ) );
        }
    }

    mod_stamina( roll_remainder( stamina_recovery * turns ) );
    add_msg_debug( "Stamina recovery: %d", roll_remainder( stamina_recovery * turns ) );
    // Cap at max
    set_stamina( std::min( std::max( get_stamina(), 0 ), max_stam ) );
}

bool Character::invoke_item( item *used )
{
    return invoke_item( used, pos() );
}

bool Character::invoke_item( item *, const tripoint &, int )
{
    return false;
}

bool Character::invoke_item( item *used, const std::string &method )
{
    return invoke_item( used, method, pos() );
}

bool Character::invoke_item( item *used, const std::string &method, const tripoint &pt,
                             int pre_obtain_moves )
{
    if( !has_enough_charges( *used, true ) ) {
        moves = pre_obtain_moves;
        return false;
    }
    if( used->is_medication() && !can_use_heal_item( *used ) ) {
        add_msg_if_player( m_bad, _( "Your biology is not compatible with that healing item." ) );
        moves = pre_obtain_moves;
        return false;
    }

    item *actually_used = used->get_usable_item( method );
    if( actually_used == nullptr ) {
        debugmsg( "Tried to invoke a method %s on item %s, which doesn't have this method",
                  method.c_str(), used->tname() );
        moves = pre_obtain_moves;
        return false;
    }

    cata::optional<int> charges_used = actually_used->type->invoke( *this->as_player(), *actually_used,
                                       pt, method );
    if( !charges_used.has_value() ) {
        moves = pre_obtain_moves;
        return false;
    }
    if( charges_used.value() == 0 ) {
        return false;
    }
    // Prevent accessing the item as it may have been deleted by the invoked iuse function.
    if( used->is_tool() || actually_used->is_medication() ) {
        return consume_charges( *actually_used, charges_used.value() );
    } else if( used->is_bionic() || used->is_deployable() || method == "place_trap" ) {
        i_rem( used );
        return true;
    } else if( used->is_comestible() ) {
        const bool ret = consume_effects( *used );
        consume_charges( *used, charges_used.value() );
        return ret;
    }

    return false;
}

bool Character::dispose_item( item_location &&obj, const std::string &prompt )
{
    uilist menu;
    menu.text = prompt.empty() ? string_format( _( "Dispose of %s" ), obj->tname() ) : prompt;

    using dispose_option = struct {
        std::string prompt;
        bool enabled;
        char invlet;
        int moves;
        std::function<bool()> action;
    };

    std::vector<dispose_option> opts;

    const bool bucket = obj->will_spill() && !obj->is_container_empty();

    opts.emplace_back( dispose_option{
        bucket ? _( "Spill contents and store in inventory" ) : _( "Store in inventory" ),
        can_stash( *obj ), '1',
        item_handling_cost( *obj ),
        [this, bucket, &obj] {
            if( bucket && !obj->contents.spill_open_pockets( *this, obj.get_item() ) )
            {
                return false;
            }

            moves -= item_handling_cost( *obj );
            this->i_add( *obj, true, &*obj );
            obj.remove_item();
            return true;
        }
    } );

    opts.emplace_back( dispose_option{
        _( "Drop item" ), true, '2', 0, [this, &obj] {
            put_into_vehicle_or_drop( *this, item_drop_reason::deliberate, { *obj } );
            obj.remove_item();
            return true;
        }
    } );

    opts.emplace_back( dispose_option{
        bucket ? _( "Spill contents and wear item" ) : _( "Wear item" ),
        can_wear( *obj ).success(), '3', item_wear_cost( *obj ),
        [this, bucket, &obj] {
            if( bucket && !obj->spill_contents( *this ) )
            {
                return false;
            }

            item it = *obj;
            obj.remove_item();
            return !!wear_item( it );
        }
    } );

    for( auto &e : worn ) {
        if( e.can_holster( *obj ) ) {
            const holster_actor *ptr = dynamic_cast<const holster_actor *>
                                       ( e.type->get_use( "holster" )->get_actor_ptr() );
            opts.emplace_back( dispose_option{
                string_format( _( "Store in %s" ), e.tname() ), true, e.invlet,
                item_store_cost( *obj, e, false, e.contents.insert_cost( *obj ) ),
                [this, ptr, &e, &obj] {
                    return ptr->store( *this->as_player(), e, *obj );
                }
            } );
        }
    }

    int w = utf8_width( menu.text, true ) + 4;
    for( const auto &e : opts ) {
        w = std::max( w, utf8_width( e.prompt, true ) + 4 );
    }
    for( auto &e : opts ) {
        e.prompt += std::string( w - utf8_width( e.prompt, true ), ' ' );
    }

    menu.text.insert( 0, 2, ' ' ); // add space for UI hotkeys
    menu.text += std::string( w + 2 - utf8_width( menu.text, true ), ' ' );
    menu.text += _( " | Moves  " );

    for( const auto &e : opts ) {
        menu.addentry( -1, e.enabled, e.invlet, string_format( e.enabled ? "%s | %-7d" : "%s |",
                       e.prompt, e.moves ) );
    }

    menu.query();
    if( menu.ret >= 0 ) {
        return opts[menu.ret].action();
    }
    return false;
}

bool Character::has_enough_charges( const item &it, bool show_msg ) const
{
    if( !it.is_tool() || !it.ammo_required() ) {
        return true;
    }
    if( it.has_flag( flag_USE_UPS ) ) {
        if( has_charges( itype_UPS, it.ammo_required() ) || it.ammo_sufficient() ) {
            return true;
        }
        if( show_msg ) {
            add_msg_if_player( m_info,
                               ngettext( "Your %s needs %d charge from some UPS.",
                                         "Your %s needs %d charges from some UPS.",
                                         it.ammo_required() ),
                               it.tname(), it.ammo_required() );
        }
        return false;
    } else if( !it.ammo_sufficient() ) {
        if( show_msg ) {
            add_msg_if_player( m_info,
                               ngettext( "Your %s has %d charge, but needs %d.",
                                         "Your %s has %d charges, but needs %d.",
                                         it.ammo_remaining() ),
                               it.tname(), it.ammo_remaining(), it.ammo_required() );
        }
        return false;
    }
    return true;
}

bool Character::consume_charges( item &used, int qty )
{
    if( qty < 0 ) {
        debugmsg( "Tried to consume negative charges" );
        return false;
    }

    if( qty == 0 ) {
        return false;
    }

    if( !used.is_tool() && !used.is_food() && !used.is_medication() ) {
        debugmsg( "Tried to consume charges for non-tool, non-food, non-med item" );
        return false;
    }

    // Consume comestibles destroying them if no charges remain
    if( used.is_food() || used.is_medication() ) {
        used.charges -= qty;
        if( used.charges <= 0 ) {
            i_rem( &used );
            return true;
        }
        return false;
    }

    // Tools which don't require ammo are instead destroyed
    if( used.is_tool() && !used.ammo_required() ) {
        if( has_item( used ) ) {
            i_rem( &used );
        } else {
            map_stack items = get_map().i_at( pos() );
            for( item_stack::iterator iter = items.begin(); iter != items.end(); iter++ ) {
                if( &( *iter ) == &used ) {
                    iter = items.erase( iter );
                    break;
                }
            }
        }
        return true;
    }

    // USE_UPS never occurs on base items but is instead added by the UPS tool mod
    if( used.has_flag( flag_USE_UPS ) ) {
        // With the new UPS system, we'll want to use any charges built up in the tool before pulling from the UPS
        // The usage of the item was already approved, so drain item if possible, otherwise use UPS
        if( used.charges >= qty || ( used.magazine_integral() &&
                                     !used.has_flag( flag_id( "USES_BIONIC_POWER" ) ) && used.ammo_remaining() >= qty ) ) {
            used.ammo_consume( qty, pos() );
        } else {
            use_charges( itype_UPS, qty );
        }
    } else {
        used.ammo_consume( std::min( qty, used.ammo_remaining() ), pos() );
    }
    return false;
}

int Character::item_handling_cost( const item &it, bool penalties, int base_cost ) const
{
    int mv = base_cost;
    if( penalties ) {
        // 40 moves per liter, up to 200 at 5 liters
        mv += std::min( 200, it.volume() / 20_ml );
    }

    if( weapon.typeId() == itype_e_handcuffs ) {
        mv *= 4;
    } else if( penalties && has_effect( effect_grabbed ) ) {
        mv *= 2;
    }

    // For single handed items use the least encumbered hand
    if( it.is_two_handed( *this ) ) {
        mv += encumb( body_part_hand_l ) + encumb( body_part_hand_r );
    } else {
        mv += std::min( encumb( body_part_hand_l ), encumb( body_part_hand_r ) );
    }

    return std::max( mv, 0 );
}

int Character::item_store_cost( const item &it, const item & /* container */, bool penalties,
                                int base_cost ) const
{
    /** @EFFECT_PISTOL decreases time taken to store a pistol */
    /** @EFFECT_SMG decreases time taken to store an SMG */
    /** @EFFECT_RIFLE decreases time taken to store a rifle */
    /** @EFFECT_SHOTGUN decreases time taken to store a shotgun */
    /** @EFFECT_LAUNCHER decreases time taken to store a launcher */
    /** @EFFECT_STABBING decreases time taken to store a stabbing weapon */
    /** @EFFECT_CUTTING decreases time taken to store a cutting weapon */
    /** @EFFECT_BASHING decreases time taken to store a bashing weapon */
    int lvl = get_skill_level( it.is_gun() ? it.gun_skill() : it.melee_skill() );
    return item_handling_cost( it, penalties, base_cost ) / ( ( lvl + 10.0f ) / 10.0f );
}

int Character::item_retrieve_cost( const item &it, const item &container, bool penalties,
                                   int base_cost ) const
{
    // Drawing from an holster use the same formula as storing an item for now
    /**
         * @EFFECT_PISTOL decreases time taken to draw pistols from holsters
         * @EFFECT_SMG decreases time taken to draw smgs from holsters
         * @EFFECT_RIFLE decreases time taken to draw rifles from holsters
         * @EFFECT_SHOTGUN decreases time taken to draw shotguns from holsters
         * @EFFECT_LAUNCHER decreases time taken to draw launchers from holsters
         * @EFFECT_STABBING decreases time taken to draw stabbing weapons from sheathes
         * @EFFECT_CUTTING decreases time taken to draw cutting weapons from scabbards
         * @EFFECT_BASHING decreases time taken to draw bashing weapons from holsters
         */
    return item_store_cost( it, container, penalties, base_cost );
}

int Character::item_wear_cost( const item &it ) const
{
    double mv = item_handling_cost( it );

    switch( it.get_layer() ) {
        case layer_level::PERSONAL:
            break;

        case layer_level::UNDERWEAR:
            mv *= 1.5;
            break;

        case layer_level::REGULAR:
            break;

        case layer_level::WAIST:
        case layer_level::OUTER:
            mv /= 1.5;
            break;

        case layer_level::BELTED:
            mv /= 2.0;
            break;

        case layer_level::AURA:
            break;

        default:
            break;
    }

    mv *= std::max( it.get_avg_encumber( *this ) / 10.0, 1.0 );

    return mv;
}

void Character::cough( bool harmful, int loudness )
{
    if( has_effect( effect_cough_suppress ) ) {
        return;
    }

    if( harmful ) {
        const int stam = get_stamina();
        const int malus = get_stamina_max() * 0.05; // 5% max stamina
        mod_stamina( -malus );
        if( stam < malus && x_in_y( malus - stam, malus ) && one_in( 6 ) ) {
            apply_damage( nullptr, body_part_torso, 1 );
        }
    }

    if( !is_npc() ) {
        add_msg( m_bad, _( "You cough heavily." ) );
    }
    sounds::sound( pos(), loudness, sounds::sound_t::speech, _( "a hacking cough." ), false, "misc",
                   "cough" );

    moves -= 80;

    add_effect( effect_disrupted_sleep, 5_minutes );
}

void Character::wake_up()
{
    //Can't wake up if under anesthesia
    if( has_effect( effect_narcosis ) ) {
        return;
    }

    // Do not remove effect_sleep or effect_alarm_clock now otherwise it invalidates an effect
    // iterator in player::process_effects().
    // We just set it for later removal (also happening in player::process_effects(), so no side
    // effects) with a duration of 0 turns.

    if( has_effect( effect_sleep ) ) {
        get_event_bus().send<event_type::character_wakes_up>( getID() );
        get_effect( effect_sleep ).set_duration( 0_turns );
    }
    remove_effect( effect_slept_through_alarm );
    remove_effect( effect_lying_down );
    if( has_effect( effect_alarm_clock ) ) {
        get_effect( effect_alarm_clock ).set_duration( 0_turns );
    }
    recalc_sight_limits();

    if( has_effect( effect_nightmares ) ) {
        add_msg_if_player( m_bad, "%s",
                           SNIPPET.random_from_category( "nightmares" ).value_or( translation() ) );
        add_morale( morale_nightmare, -15, -30, 30_minutes );
    }
}

int Character::get_shout_volume() const
{
    int base = 10;
    int shout_multiplier = 2;

    // Mutations make shouting louder, they also define the default message
    if( has_trait( trait_SHOUT3 ) ) {
        shout_multiplier = 4;
        base = 20;
    } else if( has_trait( trait_SHOUT2 ) ) {
        base = 15;
        shout_multiplier = 3;
    }

    // You can't shout without your face
    if( has_trait( trait_PROF_FOODP ) && !( is_wearing( itype_id( "foodperson_mask" ) ) ||
                                            is_wearing( itype_id( "foodperson_mask_on" ) ) ) ) {
        base = 0;
        shout_multiplier = 0;
    }

    // Masks and such dampen the sound
    // Balanced around whisper for wearing bondage mask
    // and noise ~= 10 (door smashing) for wearing dust mask for character with strength = 8
    /** @EFFECT_STR increases shouting volume */
    const int penalty = encumb( body_part_mouth ) * 3 / 2;
    int noise = base + str_cur * shout_multiplier - penalty;

    // Minimum noise volume possible after all reductions.
    // Volume 1 can't be heard even by player
    constexpr int minimum_noise = 2;

    if( noise <= base ) {
        noise = std::max( minimum_noise, noise );
    }

    noise = enchantment_cache->modify_value( enchant_vals::mod::SHOUT_NOISE, noise );

    // Screaming underwater is not good for oxygen and harder to do overall
    if( underwater ) {
        noise = std::max( minimum_noise, noise / 2 );
    }
    return noise;
}

void Character::shout( std::string msg, bool order )
{
    int base = 10;
    std::string shout;

    // You can't shout without your face
    if( has_trait( trait_PROF_FOODP ) && !( is_wearing( itype_id( "foodperson_mask" ) ) ||
                                            is_wearing( itype_id( "foodperson_mask_on" ) ) ) ) {
        add_msg_if_player( m_warning, _( "You try to shout, but you have no face!" ) );
        return;
    }

    // Mutations make shouting louder, they also define the default message
    if( has_trait( trait_SHOUT3 ) ) {
        base = 20;
        if( msg.empty() ) {
            msg = is_player() ? _( "yourself let out a piercing howl!" ) : _( "a piercing howl!" );
            shout = "howl";
        }
    } else if( has_trait( trait_SHOUT2 ) ) {
        base = 15;
        if( msg.empty() ) {
            msg = is_player() ? _( "yourself scream loudly!" ) : _( "a loud scream!" );
            shout = "scream";
        }
    }

    if( msg.empty() ) {
        msg = is_player() ? _( "yourself shout loudly!" ) : _( "a loud shout!" );
        shout = "default";
    }
    int noise = get_shout_volume();

    // Minimum noise volume possible after all reductions.
    // Volume 1 can't be heard even by player
    constexpr int minimum_noise = 2;

    if( noise <= base ) {
        std::string dampened_shout;
        std::transform( msg.begin(), msg.end(), std::back_inserter( dampened_shout ), tolower );
        msg = std::move( dampened_shout );
    }

    // Screaming underwater is not good for oxygen and harder to do overall
    if( underwater ) {
        if( !has_trait( trait_GILLS ) && !has_trait( trait_GILLS_CEPH ) ) {
            mod_stat( "oxygen", -noise );
        }
    }

    const int penalty = encumb( body_part_mouth ) * 3 / 2;
    // TODO: indistinct noise descriptions should be handled in the sounds code
    if( noise <= minimum_noise ) {
        add_msg_if_player( m_warning,
                           _( "The sound of your voice is almost completely muffled!" ) );
        msg = is_player() ? _( "your muffled shout" ) : _( "an indistinct voice" );
    } else if( noise * 2 <= noise + penalty ) {
        // The shout's volume is 1/2 or lower of what it would be without the penalty
        add_msg_if_player( m_warning, _( "The sound of your voice is significantly muffled!" ) );
    }

    sounds::sound( pos(), noise, order ? sounds::sound_t::order : sounds::sound_t::alert, msg, false,
                   "shout", shout );
}

void Character::vomit()
{
    get_event_bus().send<event_type::throws_up>( getID() );

    if( stomach.contains() != 0_ml ) {
        stomach.empty();
        get_map().add_field( adjacent_tile(), fd_bile, 1 );
        add_msg_player_or_npc( m_bad, _( "You throw up heavily!" ), _( "<npcname> throws up heavily!" ) );
    }

    if( !has_effect( effect_nausea ) ) {  // Prevents never-ending nausea
        const effect dummy_nausea( effect_source( this ), &effect_nausea.obj(), 0_turns,
                                   bodypart_str_id::NULL_ID(), false, 1, calendar::turn );
        add_effect( effect_nausea, std::max( dummy_nausea.get_max_duration() * units::to_milliliter(
                stomach.contains() ) / 21, dummy_nausea.get_int_dur_factor() ) );
    }

    mod_moves( -100 );
    for( auto &elem : *effects ) {
        for( auto &_effect_it : elem.second ) {
            auto &it = _effect_it.second;
            if( it.get_id() == effect_foodpoison ) {
                it.mod_duration( -30_minutes );
            } else if( it.get_id() == effect_drunk ) {
                it.mod_duration( rng( -10_minutes, -50_minutes ) );
            }
        }
    }
    remove_effect( effect_pkill1 );
    remove_effect( effect_pkill2 );
    remove_effect( effect_pkill3 );
    // Don't wake up when just retching
    if( stomach.contains() > 0_ml ) {
        wake_up();
    }
}

// adjacent_tile() returns a safe, unoccupied adjacent tile. If there are no such tiles, returns player position instead.
tripoint Character::adjacent_tile() const
{
    std::vector<tripoint> ret;
    int dangerous_fields = 0;
    map &here = get_map();
    for( const tripoint &p : here.points_in_radius( pos(), 1 ) ) {
        if( p == pos() ) {
            // Don't consider player position
            continue;
        }
        if( g->critter_at( p ) != nullptr ) {
            continue;
        }
        if( here.impassable( p ) ) {
            continue;
        }
        const trap &curtrap = here.tr_at( p );
        // If we don't known a trap here, the spot "appears" to be good, so consider it.
        // Same if we know a benign trap (as it's not dangerous).
        if( curtrap.can_see( p, *this ) && !curtrap.is_benign() ) {
            continue;
        }
        // Only consider tile if unoccupied, passable and has no traps
        dangerous_fields = 0;
        auto &tmpfld = here.field_at( p );
        for( auto &fld : tmpfld ) {
            const field_entry &cur = fld.second;
            if( cur.is_dangerous() ) {
                dangerous_fields++;
            }
        }

        if( dangerous_fields == 0 ) {
            ret.push_back( p );
        }
    }

    return random_entry( ret, pos() ); // player position if no valid adjacent tiles
}

void Character::set_fac_id( const std::string &my_fac_id )
{
    fac_id = faction_id( my_fac_id );
}

std::string get_stat_name( character_stat Stat )
{
    switch( Stat ) {
        // *INDENT-OFF*
    case character_stat::STRENGTH:     return pgettext( "strength stat", "STR" );
    case character_stat::DEXTERITY:    return pgettext( "dexterity stat", "DEX" );
    case character_stat::INTELLIGENCE: return pgettext( "intelligence stat", "INT" );
    case character_stat::PERCEPTION:   return pgettext( "perception stat", "PER" );
        // *INDENT-ON*
        default:
            break;
    }
    return pgettext( "fake stat there's an error", "ERR" );
}

void Character::build_mut_dependency_map( const trait_id &mut,
        std::unordered_map<trait_id, int> &dependency_map, int distance )
{
    // Skip base traits and traits we've seen with a lower distance
    const auto lowest_distance = dependency_map.find( mut );
    if( !has_base_trait( mut ) && ( lowest_distance == dependency_map.end() ||
                                    distance < lowest_distance->second ) ) {
        dependency_map[mut] = distance;
        // Recurse over all prerequisite and replacement mutations
        const mutation_branch &mdata = mut.obj();
        for( const trait_id &i : mdata.prereqs ) {
            build_mut_dependency_map( i, dependency_map, distance + 1 );
        }
        for( const trait_id &i : mdata.prereqs2 ) {
            build_mut_dependency_map( i, dependency_map, distance + 1 );
        }
        for( const trait_id &i : mdata.replacements ) {
            build_mut_dependency_map( i, dependency_map, distance + 1 );
        }
    }
}

void Character::set_highest_cat_level()
{
    mutation_category_level.clear();

    // For each of our mutations...
    for( const trait_id &mut : get_mutations() ) {
        // ...build up a map of all prerequisite/replacement mutations along the tree, along with their distance from the current mutation
        std::unordered_map<trait_id, int> dependency_map;
        build_mut_dependency_map( mut, dependency_map, 0 );

        // Then use the map to set the category levels
        for( const std::pair<const trait_id, int> &i : dependency_map ) {
            const mutation_branch &mdata = i.first.obj();
            if( !mdata.flags.count( json_flag_NON_THRESH ) ) {
                for( const mutation_category_id &cat : mdata.category ) {
                    // Decay category strength based on how far it is from the current mutation
                    mutation_category_level[cat] += 8 / static_cast<int>( std::pow( 2, i.second ) );
                }
            }
        }
    }
}

void Character::drench_mut_calc()
{
    for( const bodypart_id &bp : get_all_body_parts() ) {
        int ignored = 0;
        int neutral = 0;
        int good = 0;

        for( const trait_id &iter : get_mutations() ) {
            const mutation_branch &mdata = iter.obj();
            const auto wp_iter = mdata.protection.find( bp.id() );
            if( wp_iter != mdata.protection.end() ) {
                ignored += wp_iter->second.x;
                neutral += wp_iter->second.y;
                good += wp_iter->second.z;
            }
        }
        set_part_mut_drench( bp, { WT_GOOD, good } );
        set_part_mut_drench( bp, { WT_NEUTRAL, neutral } );
        set_part_mut_drench( bp, { WT_IGNORED, ignored } );
    }
}

/// Returns the mutation category with the highest strength
mutation_category_id Character::get_highest_category() const
{
    int iLevel = 0;
    mutation_category_id sMaxCat;

    for( const std::pair<const mutation_category_id, int> &elem : mutation_category_level ) {
        if( elem.second > iLevel ) {
            sMaxCat = elem.first;
            iLevel = elem.second;
        } else if( elem.second == iLevel ) {
            sMaxCat = mutation_category_id();  // no category on ties
        }
    }
    return sMaxCat;
}

void Character::recalculate_enchantment_cache()
{
    // start by resetting the cache to all inventory items
    *enchantment_cache = inv->get_active_enchantment_cache( *this );

    visit_items( [&]( const item * it, item * ) {
        for( const enchantment &ench : it->get_enchantments() ) {
            if( ench.is_active( *this, *it ) ) {
                enchantment_cache->force_add( ench );
            }
        }
        return VisitResponse::NEXT;
    } );

    // get from traits/ mutations
    for( const std::pair<const trait_id, trait_data> &mut_map : my_mutations ) {
        const mutation_branch &mut = mut_map.first.obj();

        for( const enchantment_id &ench_id : mut.enchantments ) {
            const enchantment &ench = ench_id.obj();
            if( ench.is_active( *this, mut.activated && mut_map.second.powered ) ) {
                enchantment_cache->force_add( ench );
            }
        }
    }

    for( const bionic &bio : *my_bionics ) {
        const bionic_id &bid = bio.id;

        for( const enchantment_id &ench_id : bid->enchantments ) {
            const enchantment &ench = ench_id.obj();
            if( ench.is_active( *this, bio.powered &&
                                bid->has_flag( STATIC( json_character_flag( "BIONIC_TOGGLED" ) ) ) ) ) {
                enchantment_cache->force_add( ench );
            }
        }
    }
}

double Character::calculate_by_enchantment( double modify, enchant_vals::mod value,
        bool round_output ) const
{
    modify += enchantment_cache->get_value_add( value );
    modify *= 1.0 + enchantment_cache->get_value_multiply( value );
    if( round_output ) {
        modify = std::round( modify );
    }
    return modify;
}

void Character::passive_absorb_hit( const bodypart_id &bp, damage_unit &du ) const
{
    // >0 check because some mutations provide negative armor
    // Thin skin check goes before subdermal armor plates because SUBdermal
    if( du.amount > 0.0f ) {
        // HACK: Get rid of this as soon as CUT and STAB are split
        if( du.type == damage_type::STAB ) {
            damage_unit du_copy = du;
            du_copy.type = damage_type::CUT;
            du.amount -= mutation_armor( bp, du_copy );
        } else {
            du.amount -= mutation_armor( bp, du );
        }
    }
    du.amount -= bionic_armor_bonus( bp, du.type ); //Check for passive armor bionics
    du.amount -= mabuff_armor_bonus( du.type );
    du.amount = std::max( 0.0f, du.amount );
}

static void destroyed_armor_msg( Character &who, const std::string &pre_damage_name )
{
    if( who.is_avatar() ) {
        get_memorial().add(
            //~ %s is armor name
            pgettext( "memorial_male", "Worn %s was completely destroyed." ),
            pgettext( "memorial_female", "Worn %s was completely destroyed." ),
            pre_damage_name );
    }
    who.add_msg_player_or_npc( m_bad, _( "Your %s is completely destroyed!" ),
                               _( "<npcname>'s %s is completely destroyed!" ),
                               pre_damage_name );
}

static void item_armor_enchantment_adjust( Character &guy, damage_unit &du, item &armor )
{
    switch( du.type ) {
        case damage_type::ACID:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_ACID );
            break;
        case damage_type::BASH:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_BASH );
            break;
        case damage_type::BIOLOGICAL:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_BIO );
            break;
        case damage_type::COLD:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_COLD );
            break;
        case damage_type::CUT:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_CUT );
            break;
        case damage_type::ELECTRIC:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_ELEC );
            break;
        case damage_type::HEAT:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_HEAT );
            break;
        case damage_type::STAB:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_STAB );
            break;
        case damage_type::BULLET:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_BULLET );
            break;
        default:
            return;
    }
    du.amount = std::max( 0.0f, du.amount );
}

// adjusts damage unit depending on type by enchantments.
// the ITEM_ enchantments only affect the damage resistance for that one item, while the others affect all of them
static void armor_enchantment_adjust( Character &guy, damage_unit &du )
{
    switch( du.type ) {
        case damage_type::ACID:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_ACID );
            break;
        case damage_type::BASH:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_BASH );
            break;
        case damage_type::BIOLOGICAL:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_BIO );
            break;
        case damage_type::COLD:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_COLD );
            break;
        case damage_type::CUT:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_CUT );
            break;
        case damage_type::ELECTRIC:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_ELEC );
            break;
        case damage_type::HEAT:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_HEAT );
            break;
        case damage_type::STAB:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_STAB );
            break;
        case damage_type::BULLET:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_BULLET );
            break;
        default:
            return;
    }
    du.amount = std::max( 0.0f, du.amount );
}

void Character::absorb_hit( const bodypart_id &bp, damage_instance &dam )
{
    std::list<item> worn_remains;
    bool armor_destroyed = false;

    for( damage_unit &elem : dam.damage_units ) {
        if( elem.amount < 0 ) {
            // Prevents 0 damage hits (like from hallucinations) from ripping armor
            elem.amount = 0;
            continue;
        }

        // The bio_ads CBM absorbs damage before hitting armor
        if( has_active_bionic( bio_ads ) ) {
            if( elem.amount > 0 && get_power_level() > 24_kJ ) {
                if( elem.type == damage_type::BASH ) {
                    elem.amount -= rng( 1, 2 );
                } else if( elem.type == damage_type::CUT ) {
                    elem.amount -= rng( 1, 4 );
                } else if( elem.type == damage_type::STAB || elem.type == damage_type::BULLET ) {
                    elem.amount -= rng( 1, 8 );
                }
                mod_power_level( -25_kJ );
            }
            if( elem.amount < 0 ) {
                elem.amount = 0;
            }
        }

        armor_enchantment_adjust( *this, elem );

        // Only the outermost armor can be set on fire
        bool outermost = true;
        // The worn vector has the innermost item first, so
        // iterate reverse to damage the outermost (last in worn vector) first.
        for( auto iter = worn.rbegin(); iter != worn.rend(); ) {
            item &armor = *iter;

            if( !armor.covers( bp ) ) {
                ++iter;
                continue;
            }

            const std::string pre_damage_name = armor.tname();
            bool destroy = false;

            item_armor_enchantment_adjust( *this, elem, armor );
            // Heat damage can set armor on fire
            // Even though it doesn't cause direct physical damage to it
            if( outermost && elem.type == damage_type::HEAT && elem.amount >= 1.0f ) {
                // TODO: Different fire intensity values based on damage
                fire_data frd{ 2 };
                destroy = armor.burn( frd );
                int fuel = roll_remainder( frd.fuel_produced );
                if( fuel > 0 ) {
                    add_effect( effect_onfire, time_duration::from_turns( fuel + 1 ), bp, false, 0, false,
                                true );
                }
            }

            if( !destroy ) {
                destroy = armor_absorb( elem, armor, bp );
            }

            if( destroy ) {
                if( get_player_view().sees( *this ) ) {
                    SCT.add( point( posx(), posy() ), direction::NORTH, remove_color_tags( pre_damage_name ),
                             m_neutral, _( "destroyed" ), m_info );
                }
                destroyed_armor_msg( *this, pre_damage_name );
                armor_destroyed = true;
                armor.on_takeoff( *this );
                for( const item *it : armor.contents.all_items_top( item_pocket::pocket_type::CONTAINER ) ) {
                    worn_remains.push_back( *it );
                }
                // decltype is the type name of the iterator, note that reverse_iterator::base returns the
                // iterator to the next element, not the one the revers_iterator points to.
                // http://stackoverflow.com/questions/1830158/how-to-call-erase-with-a-reverse-iterator
                iter = decltype( iter )( worn.erase( --( iter.base() ) ) );
            } else {
                ++iter;
                outermost = false;
            }
        }

        passive_absorb_hit( bp, elem );

        if( elem.type == damage_type::BASH ) {
            if( has_trait( trait_LIGHT_BONES ) ) {
                elem.amount *= 1.4;
            }
            if( has_trait( trait_HOLLOW_BONES ) ) {
                elem.amount *= 1.8;
            }
        }

        elem.amount = std::max( elem.amount, 0.0f );
    }
    map &here = get_map();
    for( item &remain : worn_remains ) {
        here.add_item_or_charges( pos(), remain );
    }
    if( armor_destroyed ) {
        drop_invalid_inventory();
    }
}

bool Character::armor_absorb( damage_unit &du, item &armor, const bodypart_id &bp )
{
    if( rng( 1, 100 ) > armor.get_coverage( bp ) ) {
        return false;
    }

    // TODO: add some check for power armor
    armor.mitigate_damage( du );

    // We want armor's own resistance to this type, not the resistance it grants
    const float armors_own_resist = armor.damage_resist( du.type, true );
    if( armors_own_resist > 1000.0f ) {
        // This is some weird type that doesn't damage armors
        return false;
    }

    // Scale chance of article taking damage based on the number of parts it covers.
    // This represents large articles being able to take more punishment
    // before becoming ineffective or being destroyed.
    const int num_parts_covered = armor.get_covered_body_parts().count();
    if( !one_in( num_parts_covered ) ) {
        return false;
    }

    // Don't damage armor as much when bypassed by armor piercing
    // Most armor piercing damage comes from bypassing armor, not forcing through
    const float raw_dmg = du.amount;
    if( raw_dmg > armors_own_resist ) {
        // If damage is above armor value, the chance to avoid armor damage is
        // 50% + 50% * 1/dmg
        if( one_in( raw_dmg ) || one_in( 2 ) ) {
            return false;
        }
    } else {
        // Sturdy items and power armors never take chip damage.
        // Other armors have 0.5% of getting damaged from hits below their armor value.
        if( armor.has_flag( flag_STURDY ) || armor.is_power_armor() || !one_in( 200 ) ) {
            return false;
        }
    }

    const material_type &material = armor.get_random_material();
    std::string damage_verb = ( du.type == damage_type::BASH ) ? material.bash_dmg_verb() :
                              material.cut_dmg_verb();

    const std::string pre_damage_name = armor.tname();
    const std::string pre_damage_adj = armor.get_base_material().dmg_adj( armor.damage_level() );

    // add "further" if the damage adjective and verb are the same
    std::string format_string = ( pre_damage_adj == damage_verb ) ?
                                _( "Your %1$s is %2$s further!" ) : _( "Your %1$s is %2$s!" );
    add_msg_if_player( m_bad, format_string, pre_damage_name, damage_verb );
    //item is damaged
    if( is_player() ) {
        SCT.add( point( posx(), posy() ), direction::NORTH, remove_color_tags( pre_damage_name ), m_neutral,
                 damage_verb,
                 m_info );
    }

    return armor.mod_damage( armor.has_flag( flag_FRAGILE ) ?
                             rng( 2 * itype::damage_scale, 3 * itype::damage_scale ) : itype::damage_scale, du.type );
}

float Character::bionic_armor_bonus( const bodypart_id &bp, damage_type dt ) const
{
    float result = 0.0f;
    if( dt == damage_type::CUT || dt == damage_type::STAB ) {
        for( const bionic_id &bid : get_bionics() ) {
            const auto cut_prot = bid->cut_protec.find( bp.id() );
            if( cut_prot != bid->cut_protec.end() ) {
                result += cut_prot->second;
            }
        }
    } else if( dt == damage_type::BASH ) {
        for( const bionic_id &bid : get_bionics() ) {
            const auto bash_prot = bid->bash_protec.find( bp.id() );
            if( bash_prot != bid->bash_protec.end() ) {
                result += bash_prot->second;
            }
        }
    } else if( dt == damage_type::BULLET ) {
        for( const bionic_id &bid : get_bionics() ) {
            const auto bullet_prot = bid->bullet_protec.find( bp.id() );
            if( bullet_prot != bid->bullet_protec.end() ) {
                result += bullet_prot->second;
            }
        }
    }

    return result;
}

int Character::get_armor_fire( const bodypart_id &bp ) const
{
    return get_armor_type( damage_type::HEAT, bp );
}

void Character::did_hit( Creature &target )
{
    enchantment_cache->cast_hit_you( *this, target );
}

ret_val<bool> Character::can_wield( const item &it ) const
{
    if( has_effect( effect_incorporeal ) ) {
        return ret_val<bool>::make_failure( _( "You can't wield anything while incorporeal." ) );
    }
    if( get_working_arm_count() <= 0 ) {
        return ret_val<bool>::make_failure(
                   _( "You need at least one arm to even consider wielding something." ) );
    }
    if( it.made_of_from_type( phase_id::LIQUID ) ) {
        return ret_val<bool>::make_failure( _( "Can't wield spilt liquids." ) );
    }

    if( is_armed() && !can_unwield( weapon ).success() ) {
        return ret_val<bool>::make_failure( _( "The %s is preventing you from wielding the %s." ),
                                            weapname(), it.tname() );
    }
    monster *mount = mounted_creature.get();
    if( it.is_two_handed( *this ) && ( !has_two_arms() || worn_with_flag( flag_RESTRICT_HANDS ) ) &&
        !( is_mounted() && mount->has_flag( MF_RIDEABLE_MECH ) &&
           mount->type->mech_weapon && it.typeId() == mount->type->mech_weapon ) ) {
        if( worn_with_flag( flag_RESTRICT_HANDS ) ) {
            return ret_val<bool>::make_failure(
                       _( "Something you are wearing hinders the use of both hands." ) );
        } else if( it.has_flag( flag_ALWAYS_TWOHAND ) ) {
            return ret_val<bool>::make_failure( _( "The %s can't be wielded with only one arm." ),
                                                it.tname() );
        } else {
            return ret_val<bool>::make_failure( _( "You are too weak to wield %s with only one arm." ),
                                                it.tname() );
        }
    }
    if( is_mounted() && mount->has_flag( MF_RIDEABLE_MECH ) &&
        mount->type->mech_weapon && it.typeId() != mount->type->mech_weapon ) {
        return ret_val<bool>::make_failure( _( "You cannot wield anything while piloting a mech." ) );
    }

    return ret_val<bool>::make_success();
}

bool Character::has_wield_conflicts( const item &it ) const
{
    return is_wielding( it ) || ( is_armed() && !it.can_combine( weapon ) );
}

bool Character::unwield()
{
    if( weapon.is_null() ) {
        return true;
    }

    if( !can_unwield( weapon ).success() ) {
        return false;
    }

    // currently the only way to unwield NO_UNWIELD weapon is if it's a bionic that can be deactivated
    if( weapon.has_flag( flag_NO_UNWIELD ) ) {
        cata::optional<int> wi = active_bionic_weapon_index();
        return wi && deactivate_bionic( *wi );
    }

    const std::string query = string_format( _( "Stop wielding %s?" ), weapon.tname() );

    if( !dispose_item( item_location( *this, &weapon ), query ) ) {
        return false;
    }

    inv->unsort();

    return true;
}

std::string Character::weapname() const
{
    if( weapon.is_gun() ) {
        std::string gunmode;
        // only required for empty mags and empty guns
        std::string mag_ammo;
        if( weapon.gun_all_modes().size() > 1 ) {
            gunmode = weapon.gun_current_mode().tname();
        }

        if( weapon.ammo_remaining() == 0 ) {
            if( weapon.magazine_current() != nullptr ) {
                const item *mag = weapon.magazine_current();
                mag_ammo = string_format( " (0/%d)",
                                          mag->ammo_capacity( item( mag->ammo_default() ).ammo_type() ) );
            } else {
                mag_ammo = _( " (empty)" );
            }
        }

        return string_format( "%s%s%s", gunmode, weapon.display_name(), mag_ammo );

    } else if( !is_armed() ) {
        return _( "fists" );

    } else {
        return weapon.tname();
    }
}

void Character::on_hit( Creature *source, bodypart_id bp_hit,
                        float /*difficulty*/, dealt_projectile_attack const *const proj )
{
    check_dead_state();
    if( source == nullptr || proj != nullptr ) {
        return;
    }

    bool u_see = get_player_view().sees( *this );
    if( has_active_bionic( bionic_id( "bio_ods" ) ) && get_power_level() > 5_kJ ) {
        if( is_player() ) {
            add_msg( m_good, _( "Your offensive defense system shocks %s in mid-attack!" ),
                     source->disp_name() );
        } else if( u_see ) {
            add_msg( _( "%1$s's offensive defense system shocks %2$s in mid-attack!" ),
                     disp_name(),
                     source->disp_name() );
        }
        int shock = rng( 1, 4 );
        mod_power_level( units::from_kilojoule( -shock ) );
        damage_instance ods_shock_damage;
        ods_shock_damage.add_damage( damage_type::ELECTRIC, shock * 5 );
        // Should hit body part used for attack
        source->deal_damage( this, body_part_torso, ods_shock_damage );
    }
    if( !wearing_something_on( bp_hit ) &&
        ( has_trait( trait_SPINES ) || has_trait( trait_QUILLS ) ) ) {
        int spine = rng( 1, has_trait( trait_QUILLS ) ? 20 : 8 );
        if( !is_player() ) {
            if( u_see ) {
                add_msg( _( "%1$s's %2$s puncture %3$s in mid-attack!" ), name,
                         ( has_trait( trait_QUILLS ) ? _( "quills" ) : _( "spines" ) ),
                         source->disp_name() );
            }
        } else {
            add_msg( m_good, _( "Your %1$s puncture %2$s in mid-attack!" ),
                     ( has_trait( trait_QUILLS ) ? _( "quills" ) : _( "spines" ) ),
                     source->disp_name() );
        }
        damage_instance spine_damage;
        spine_damage.add_damage( damage_type::STAB, spine );
        source->deal_damage( this, body_part_torso, spine_damage );
    }
    if( ( !( wearing_something_on( bp_hit ) ) ) && ( has_trait( trait_THORNS ) ) &&
        ( !( source->has_weapon() ) ) ) {
        if( !is_player() ) {
            if( u_see ) {
                add_msg( _( "%1$s's %2$s scrape %3$s in mid-attack!" ), name,
                         _( "thorns" ), source->disp_name() );
            }
        } else {
            add_msg( m_good, _( "Your thorns scrape %s in mid-attack!" ), source->disp_name() );
        }
        int thorn = rng( 1, 4 );
        damage_instance thorn_damage;
        thorn_damage.add_damage( damage_type::CUT, thorn );
        // In general, critters don't have separate limbs
        // so safer to target the torso
        source->deal_damage( this, body_part_torso, thorn_damage );
    }
    if( ( !( wearing_something_on( bp_hit ) ) ) && ( has_trait( trait_CF_HAIR ) ) ) {
        if( !is_player() ) {
            if( u_see ) {
                add_msg( _( "%1$s gets a load of %2$s's %3$s stuck in!" ), source->disp_name(),
                         name, ( _( "hair" ) ) );
            }
        } else {
            add_msg( m_good, _( "Your hairs detach into %s!" ), source->disp_name() );
        }
        source->add_effect( effect_stunned, 2_turns );
        if( one_in( 3 ) ) { // In the eyes!
            source->add_effect( effect_blind, 2_turns );
        }
    }
    if( worn_with_flag( flag_REQUIRES_BALANCE ) && !has_effect( effect_downed ) ) {
        int rolls = 4;
        if( worn_with_flag( flag_ROLLER_ONE ) ) {
            rolls += 2;
        }
        if( has_trait( trait_PROF_SKATER ) ) {
            rolls--;
        }
        if( has_trait( trait_DEFT ) ) {
            rolls--;
        }
        if( has_trait( trait_CLUMSY ) ) {
            rolls++;
        }

        if( stability_roll() < dice( rolls, 10 ) ) {
            if( !is_player() ) {
                if( u_see ) {
                    add_msg( _( "%1$s loses their balance while being hit!" ), name );
                }
            } else {
                add_msg( m_bad, _( "You lose your balance while being hit!" ) );
            }
            // This kind of downing is not subject to immunity.
            add_effect( effect_downed, 2_turns, false, 0, true );
        }
    }

    enchantment_cache->cast_hit_me( *this, source );
}

/*
    Where damage to character is actually applied to hit body parts
    Might be where to put bleed stuff rather than in player::deal_damage()
 */
void Character::apply_damage( Creature *source, bodypart_id hurt, int dam, const bool bypass_med )
{
    if( is_dead_state() || has_trait( trait_DEBUG_NODMG ) || has_effect( effect_incorporeal ) ) {
        // don't do any more damage if we're already dead
        // Or if we're debugging and don't want to die
        // Or we are intangible
        return;
    }

    if( hurt == bodypart_str_id::NULL_ID() ) {
        debugmsg( "Wacky body part hurt!" );
        hurt = body_part_torso;
    }

    mod_pain( dam / 2 );

    const bodypart_id &part_to_damage = hurt->main_part;

    const int dam_to_bodypart = std::min( dam, get_part_hp_cur( part_to_damage ) );

    mod_part_hp_cur( part_to_damage, - dam_to_bodypart );
    get_event_bus().send<event_type::character_takes_damage>( getID(), dam_to_bodypart );

    if( !weapon.is_null() && !as_player()->can_wield( weapon ).success() &&
        can_drop( weapon ).success() ) {
        add_msg_if_player( _( "You are no longer able to wield your %s and drop it!" ),
                           weapon.display_name() );
        put_into_vehicle_or_drop( *this, item_drop_reason::tumbling, { weapon } );
        i_rem( &weapon );
    }
    if( has_effect( effect_mending, part_to_damage.id() ) && ( source == nullptr ||
            !source->is_hallucination() ) ) {
        effect &e = get_effect( effect_mending, part_to_damage );
        float remove_mend = dam / 20.0f;
        e.mod_duration( -e.get_max_duration() * remove_mend );
    }

    if( dam > get_painkiller() ) {
        on_hurt( source );
    }

    if( !bypass_med ) {
        // remove healing effects if damaged
        int remove_med = roll_remainder( dam / 5.0f );
        if( remove_med > 0 && has_effect( effect_bandaged, part_to_damage.id() ) ) {
            remove_med -= reduce_healing_effect( effect_bandaged, remove_med, part_to_damage );
        }
        if( remove_med > 0 && has_effect( effect_disinfected, part_to_damage.id() ) ) {
            reduce_healing_effect( effect_disinfected, remove_med, part_to_damage );
        }
    }
}

dealt_damage_instance Character::deal_damage( Creature *source, bodypart_id bp,
        const damage_instance &d )
{
    if( has_trait( trait_DEBUG_NODMG ) || has_effect( effect_incorporeal ) ) {
        return dealt_damage_instance();
    }

    //damage applied here
    dealt_damage_instance dealt_dams = Creature::deal_damage( source, bp, d );
    //block reduction should be by applied this point
    int dam = dealt_dams.total_damage();

    // TODO: Pre or post blit hit tile onto "this"'s location here
    if( dam > 0 && get_player_view().sees( pos() ) ) {
        g->draw_hit_player( *this, dam );

        if( is_player() && source ) {
            //monster hits player melee
            SCT.add( point( posx(), posy() ),
                     direction_from( point_zero, point( posx() - source->posx(), posy() - source->posy() ) ),
                     get_hp_bar( dam, get_hp_max( bp ) ).first, m_bad, body_part_name( bp ), m_neutral );
        }
    }

    // And slimespawners too
    if( ( has_trait( trait_SLIMESPAWNER ) ) && ( dam >= 10 ) && one_in( 20 - dam ) ) {
        if( monster *const slime = g->place_critter_around( mon_player_blob, pos(), 1 ) ) {
            slime->friendly = -1;
            add_msg_if_player( m_warning, _( "A mass of slime is torn from you, and moves on its own!" ) );
        }
    }

    Character &player_character = get_player_character();
    //Acid blood effects.
    bool u_see = player_character.sees( *this );
    int cut_dam = dealt_dams.type_damage( damage_type::CUT );
    if( source && has_trait( trait_ACIDBLOOD ) && !one_in( 3 ) &&
        ( dam >= 4 || cut_dam > 0 ) && ( rl_dist( player_character.pos(), source->pos() ) <= 1 ) ) {
        if( is_player() ) {
            add_msg( m_good, _( "Your acidic blood splashes %s in mid-attack!" ),
                     source->disp_name() );
        } else if( u_see ) {
            add_msg( _( "%1$s's acidic blood splashes on %2$s in mid-attack!" ),
                     disp_name(), source->disp_name() );
        }
        damage_instance acidblood_damage;
        acidblood_damage.add_damage( damage_type::ACID, rng( 4, 16 ) );
        if( !one_in( 4 ) ) {
            source->deal_damage( this, body_part_arm_l, acidblood_damage );
            source->deal_damage( this, body_part_arm_r, acidblood_damage );
        } else {
            source->deal_damage( this, body_part_torso, acidblood_damage );
            source->deal_damage( this, body_part_head, acidblood_damage );
        }
    }

    int recoil_mul = 100;

    if( bp == body_part_eyes ) {
        if( dam > 5 || cut_dam > 0 ) {
            const time_duration minblind = std::max( 1_turns, 1_turns * ( dam + cut_dam ) / 10 );
            const time_duration maxblind = std::min( 5_turns, 1_turns * ( dam + cut_dam ) / 4 );
            add_effect( effect_blind, rng( minblind, maxblind ) );
        }
    } else if( bp == body_part_hand_l || bp == body_part_arm_l ||
               bp == body_part_hand_r || bp == body_part_arm_r ) {
        recoil_mul = 200;
    } else if( bp == bodypart_str_id::NULL_ID() ) {
        debugmsg( "Wacky body part hit!" );
    }

    // TODO: Scale with damage in a way that makes sense for power armors, plate armor and naked skin.
    recoil += recoil_mul * weapon.volume() / 250_ml;
    recoil = std::min( MAX_RECOIL, recoil );
    //looks like this should be based off of dealt damages, not d as d has no damage reduction applied.
    // Skip all this if the damage isn't from a creature. e.g. an explosion.
    if( source != nullptr ) {
        if( source->has_flag( MF_GRABS ) && !source->is_hallucination() &&
            !source->has_effect( effect_grabbing ) ) {
            /** @EFFECT_DEX increases chance to avoid being grabbed */

            int reflex_mod = has_trait( trait_FAST_REFLEXES ) ? 2 : 1;
            const bool dodged_grab = rng( 0, reflex_mod * get_dex() ) > rng( 0, 10 );

            if( has_grab_break_tec() && dodged_grab ) {
                if( has_effect( effect_grabbed ) ) {
                    add_msg_if_player( m_warning, _( "The %s tries to grab you as well, but you bat it away!" ),
                                       source->disp_name() );
                } else {
                    add_msg_player_or_npc( m_info, _( "The %s tries to grab you, but you break its grab!" ),
                                           _( "The %s tries to grab <npcname>, but they break its grab!" ),
                                           source->disp_name() );
                }
            } else {
                int prev_effect = get_effect_int( effect_grabbed );
                add_effect( effect_grabbed, 2_turns,  body_part_torso, false, prev_effect + 2 );
                source->add_effect( effect_grabbing, 2_turns );
                add_msg_player_or_npc( m_bad, _( "You are grabbed by %s!" ), _( "<npcname> is grabbed by %s!" ),
                                       source->disp_name() );
            }
        }
    }

    int sum_cover = 0;
    for( const item &i : worn ) {
        if( i.covers( bp ) && i.is_filthy() ) {
            sum_cover += i.get_coverage( bp );
        }
    }

    // Chance of infection is damage (with cut and stab x4) * sum of coverage on affected body part, in percent.
    // i.e. if the body part has a sum of 100 coverage from filthy clothing,
    // each point of damage has a 1% change of causing infection.
    if( sum_cover > 0 ) {
        const int cut_type_dam = dealt_dams.type_damage( damage_type::CUT ) + dealt_dams.type_damage(
                                     damage_type::STAB );
        const int combined_dam = dealt_dams.type_damage( damage_type::BASH ) + ( cut_type_dam * 4 );
        const int infection_chance = ( combined_dam * sum_cover ) / 100;
        if( x_in_y( infection_chance, 100 ) ) {
            if( has_effect( effect_bite, bp.id() ) ) {
                add_effect( effect_bite, 40_minutes, bp, true );
            } else if( has_effect( effect_infected, bp.id() ) ) {
                add_effect( effect_infected, 25_minutes, bp, true );
            } else {
                add_effect( effect_bite, 1_turns, bp, true );
            }
            add_msg_if_player( _( "Filth from your clothing has been embedded deep in the wound." ) );
        }
    }

    on_hurt( source );
    return dealt_dams;
}

int Character::reduce_healing_effect( const efftype_id &eff_id, int remove_med,
                                      const bodypart_id &hurt )
{
    effect &e = get_effect( eff_id, hurt );
    int intensity = e.get_intensity();
    if( remove_med < intensity ) {
        if( eff_id == effect_bandaged ) {
            add_msg_if_player( m_bad, _( "Bandages on your %s were damaged!" ), body_part_name( hurt ) );
        } else  if( eff_id == effect_disinfected ) {
            add_msg_if_player( m_bad, _( "You got some filth on your disinfected %s!" ),
                               body_part_name( hurt ) );
        }
    } else {
        if( eff_id == effect_bandaged ) {
            add_msg_if_player( m_bad, _( "Bandages on your %s were destroyed!" ),
                               body_part_name( hurt ) );
        } else  if( eff_id == effect_disinfected ) {
            add_msg_if_player( m_bad, _( "Your %s is no longer disinfected!" ), body_part_name( hurt ) );
        }
    }
    e.mod_duration( -6_hours * remove_med );
    return intensity;
}

void Character::heal_bp( bodypart_id bp, int dam )
{
    heal( bp, dam );
}

void Character::heal( const bodypart_id &healed, int dam )
{
    if( !is_limb_broken( healed ) ) {
        int effective_heal = std::min( dam, get_part_hp_max( healed ) - get_part_hp_cur( healed ) );
        mod_part_hp_cur( healed, effective_heal );
        get_event_bus().send<event_type::character_heals_damage>( getID(), effective_heal );
    }
}

void Character::healall( int dam )
{
    for( const bodypart_id &bp : get_all_body_parts() ) {
        heal( bp, dam );
        mod_part_healed_total( bp, dam );
    }
}

void Character::hurtall( int dam, Creature *source, bool disturb /*= true*/ )
{
    if( is_dead_state() || has_trait( trait_DEBUG_NODMG ) || has_effect( effect_incorporeal ) ||
        dam <= 0 ) {
        return;
    }

    for( const bodypart_id &bp : get_all_body_parts( get_body_part_flags::only_main ) ) {
        // Don't use apply_damage here or it will annoy the player with 6 queries
        const int dam_to_bodypart = std::min( dam, get_part_hp_cur( bp ) );
        mod_part_hp_cur( bp, - dam_to_bodypart );
        get_event_bus().send<event_type::character_takes_damage>( getID(), dam_to_bodypart );
    }

    // Low pain: damage is spread all over the body, so not as painful as 6 hits in one part
    mod_pain( dam );
    on_hurt( source, disturb );
}

int Character::hitall( int dam, int vary, Creature *source )
{
    int damage_taken = 0;
    for( const bodypart_id &bp : get_all_body_parts( get_body_part_flags::only_main ) ) {
        int ddam = vary ? dam * rng( 100 - vary, 100 ) / 100 : dam;
        int cut = 0;
        damage_instance damage = damage_instance::physical( ddam, cut, 0 );
        damage_taken += deal_damage( source, bp, damage ).total_damage();
    }
    return damage_taken;
}

void Character::on_hurt( Creature *source, bool disturb /*= true*/ )
{
    if( has_trait( trait_ADRENALINE ) && !has_effect( effect_adrenaline ) &&
        ( get_part_hp_cur( body_part_head ) < 25 ||
          get_part_hp_cur( body_part_torso ) < 15 ) ) {
        add_effect( effect_adrenaline, 20_minutes );
    }

    if( disturb ) {
        if( has_effect( effect_sleep ) ) {
            wake_up();
        }
        if( !is_npc() && !has_effect( effect_narcosis ) ) {
            if( source != nullptr ) {
                if( sees( *source ) ) {
                    g->cancel_activity_or_ignore_query( distraction_type::attacked,
                                                        string_format( _( "You were attacked by %s!" ), source->disp_name() ) );
                } else {
                    g->cancel_activity_or_ignore_query( distraction_type::attacked,
                                                        _( "You were attacked by something you can't see!" ) );
                }
            } else {
                g->cancel_activity_or_ignore_query( distraction_type::attacked, _( "You were hurt!" ) );
            }
        }
    }

    if( is_dead_state() ) {
        set_killer( source );
    }
}

bool Character::crossed_threshold() const
{
    for( const trait_id &mut : get_mutations() ) {
        if( mut->threshold ) {
            return true;
        }
    }
    return false;
}

void Character::update_type_of_scent( bool init )
{
    scenttype_id new_scent = scenttype_id( "sc_human" );
    for( const trait_id &mut : get_mutations() ) {
        if( mut.obj().scent_typeid ) {
            new_scent = mut.obj().scent_typeid.value();
        }
    }

    if( !init && new_scent != get_type_of_scent() ) {
        get_scent().reset();
    }
    set_type_of_scent( new_scent );
}

void Character::update_type_of_scent( const trait_id &mut, bool gain )
{
    const cata::optional<scenttype_id> &mut_scent = mut->scent_typeid;
    if( mut_scent ) {
        if( gain && mut_scent.value() != get_type_of_scent() ) {
            set_type_of_scent( mut_scent.value() );
            get_scent().reset();
        } else {
            update_type_of_scent();
        }
    }
}

void Character::set_type_of_scent( const scenttype_id &id )
{
    type_of_scent = id;
}

scenttype_id Character::get_type_of_scent() const
{
    return type_of_scent;
}

void Character::restore_scent()
{
    const std::string prev_scent = get_value( "prev_scent" );
    if( !prev_scent.empty() ) {
        remove_effect( effect_masked_scent );
        set_type_of_scent( scenttype_id( prev_scent ) );
        remove_value( "prev_scent" );
        remove_value( "waterproof_scent" );
        add_msg_if_player( m_info, _( "You smell like yourself again." ) );
    }
}

void Character::spores()
{
    map &here = get_map();
    fungal_effects fe( *g, here );
    //~spore-release sound
    sounds::sound( pos(), 10, sounds::sound_t::combat, _( "Pouf!" ), false, "misc", "puff" );
    for( const tripoint &sporep : here.points_in_radius( pos(), 1 ) ) {
        if( sporep == pos() ) {
            continue;
        }
        fe.fungalize( sporep, this, 0.25 );
    }
}

void Character::blossoms()
{
    // Player blossoms are shorter-ranged, but you can fire much more frequently if you like.
    sounds::sound( pos(), 10, sounds::sound_t::combat, _( "Pouf!" ), false, "misc", "puff" );
    map &here = get_map();
    for( const tripoint &tmp : here.points_in_radius( pos(), 2 ) ) {
        here.add_field( tmp, fd_fungal_haze, rng( 1, 2 ) );
    }
}

void Character::update_vitamins( const vitamin_id &vit )
{
    if( is_npc() && vit->type() != vitamin_type::COUNTER ) {
        return; // NPCs cannot develop vitamin diseases, bypass for special
    }

    efftype_id def = vit.obj().deficiency();
    efftype_id exc = vit.obj().excess();

    int lvl = vit.obj().severity( vitamin_get( vit ) );
    if( lvl <= 0 ) {
        remove_effect( def );
    }
    if( lvl >= 0 ) {
        remove_effect( exc );
    }
    if( lvl > 0 ) {
        if( has_effect( def ) ) {
            get_effect( def ).set_intensity( lvl, true );
        } else {
            add_effect( def, 1_turns, true, lvl );
        }
    }
    if( lvl < 0 ) {
        if( has_effect( exc ) ) {
            get_effect( exc ).set_intensity( -lvl, true );
        } else {
            add_effect( exc, 1_turns, true, -lvl );
        }
    }
}

void Character::rooted_message() const
{
    bool wearing_shoes = footwear_factor() == 1.0;
    if( ( has_trait( trait_ROOTS2 ) || has_trait( trait_ROOTS3 ) ) &&
        get_map().has_flag( flag_PLOWABLE, pos() ) &&
        !wearing_shoes ) {
        add_msg( m_info, _( "You sink your roots into the soil." ) );
    }
}

void Character::rooted()
// This assumes that daily Iron, Calcium, and Thirst needs should be filled at the same rate.
// Baseline humans need 96 Iron and Calcium, and 288 Thirst per day.
// Thirst level -40 puts it right in the middle of the 'Hydrated' zone.
// TODO: The rates for iron, calcium, and thirst should probably be pulled from the nutritional data rather than being hardcoded here, so that future balance changes don't break this.
{
    double shoe_factor = footwear_factor();
    if( ( has_trait( trait_ROOTS2 ) || has_trait( trait_ROOTS3 ) ) &&
        get_map().has_flag( flag_PLOWABLE, pos() ) && shoe_factor != 1.0 ) {
        int time_to_full = 43200; // 12 hours
        if( has_trait( trait_ROOTS3 ) ) {
            time_to_full += -14400;    // -4 hours
        }
        if( x_in_y( 96, time_to_full ) ) {
            vitamin_mod( vitamin_id( "iron" ), 1, true );
            vitamin_mod( vitamin_id( "calcium" ), 1, true );
            mod_healthy_mod( 5, 50 );
        }
        if( get_thirst() > -40 && x_in_y( 288, time_to_full ) ) {
            mod_thirst( -1 );
        }
    }
}

bool Character::wearing_something_on( const bodypart_id &bp ) const
{
    for( const item &i : worn ) {
        if( i.covers( bp ) ) {
            return true;
        }
    }
    return false;
}

bool Character::is_wearing_shoes( const side &which_side ) const
{
    bool left = true;
    bool right = true;
    if( which_side == side::LEFT || which_side == side::BOTH ) {
        left = false;
        for( const item &worn_item : worn ) {
            if( worn_item.covers( body_part_foot_l ) && !worn_item.has_flag( flag_BELTED ) &&
                !worn_item.has_flag( flag_PERSONAL ) && !worn_item.has_flag( flag_AURA ) &&
                !worn_item.has_flag( flag_SEMITANGIBLE ) && !worn_item.has_flag( flag_SKINTIGHT ) ) {
                left = true;
                break;
            }
        }
    }
    if( which_side == side::RIGHT || which_side == side::BOTH ) {
        right = false;
        for( const item &worn_item : worn ) {
            if( worn_item.covers( body_part_foot_r ) && !worn_item.has_flag( flag_BELTED ) &&
                !worn_item.has_flag( flag_PERSONAL ) && !worn_item.has_flag( flag_AURA ) &&
                !worn_item.has_flag( flag_SEMITANGIBLE ) && !worn_item.has_flag( flag_SKINTIGHT ) ) {
                right = true;
                break;
            }
        }
    }
    return ( left && right );
}

bool Character::is_wearing_helmet() const
{
    for( const item &i : worn ) {
        if( i.covers( body_part_head ) && !i.has_flag( flag_HELMET_COMPAT ) &&
            !i.has_flag( flag_SKINTIGHT ) &&
            !i.has_flag( flag_PERSONAL ) && !i.has_flag( flag_AURA ) && !i.has_flag( flag_SEMITANGIBLE ) &&
            !i.has_flag( flag_OVERSIZE ) ) {
            return true;
        }
    }
    return false;
}

int Character::head_cloth_encumbrance() const
{
    int ret = 0;
    for( const item &i : worn ) {
        const item *worn_item = &i;
        if( i.covers( body_part_head ) && !i.has_flag( flag_SEMITANGIBLE ) &&
            ( worn_item->has_flag( flag_HELMET_COMPAT ) || worn_item->has_flag( flag_SKINTIGHT ) ) ) {
            ret += worn_item->get_encumber( *this, body_part_head );
        }
    }
    return ret;
}

double Character::armwear_factor() const
{
    double ret = 0;
    if( wearing_something_on( body_part_arm_l ) ) {
        ret += .5;
    }
    if( wearing_something_on( body_part_arm_r ) ) {
        ret += .5;
    }
    return ret;
}

double Character::footwear_factor() const
{
    double ret = 0;
    for( const item &i : worn ) {
        if( i.covers( body_part_foot_l ) && !i.has_flag( flag_NOT_FOOTWEAR ) ) {
            ret += 0.5f;
            break;
        }
    }
    for( const item &i : worn ) {
        if( i.covers( body_part_foot_r ) && !i.has_flag( flag_NOT_FOOTWEAR ) ) {
            ret += 0.5f;
            break;
        }
    }
    return ret;
}

int Character::shoe_type_count( const itype_id &it ) const
{
    int ret = 0;
    if( is_wearing_on_bp( it, body_part_foot_l ) ) {
        ret++;
    }
    if( is_wearing_on_bp( it, body_part_foot_r ) ) {
        ret++;
    }
    return ret;
}

std::vector<item *> Character::inv_dump()
{
    std::vector<item *> ret;
    if( is_armed() && can_drop( weapon ).success() ) {
        ret.push_back( &weapon );
    }
    for( item &i : worn ) {
        ret.push_back( &i );
    }
    inv->dump( ret );
    return ret;
}

bool Character::covered_with_flag( const flag_id &f, const body_part_set &parts ) const
{
    if( parts.none() ) {
        return true;
    }

    body_part_set to_cover( parts );

    for( const auto &elem : worn ) {
        if( !elem.has_flag( f ) ) {
            continue;
        }

        to_cover.substract_set( elem.get_covered_body_parts() );

        if( to_cover.none() ) {
            return true;    // Allows early exit.
        }
    }

    return to_cover.none();
}

bool Character::is_waterproof( const body_part_set &parts ) const
{
    return covered_with_flag( flag_WATERPROOF, parts );
}

void Character::update_morale()
{
    morale->decay( 1_minutes );
    apply_persistent_morale();
}

units::volume Character::free_space() const
{
    units::volume volume_capacity = 0_ml;
    volume_capacity += weapon.get_total_capacity();
    for( const item_pocket *pocket : weapon.contents.get_all_contained_pockets().value() ) {
        if( pocket->contains_phase( phase_id::SOLID ) ) {
            for( const item *it : pocket->all_items_top() ) {
                volume_capacity -= it->volume();
            }
        } else if( !pocket->empty() ) {
            volume_capacity -= pocket->volume_capacity();
        }
    }
    volume_capacity += weapon.check_for_free_space();
    for( const item &w : worn ) {
        volume_capacity += w.get_total_capacity();
        for( const item_pocket *pocket : w.contents.get_all_contained_pockets().value() ) {
            if( pocket->contains_phase( phase_id::SOLID ) ) {
                for( const item *it : pocket->all_items_top() ) {
                    volume_capacity -= it->volume();
                }
            } else if( !pocket->empty() ) {
                volume_capacity -= pocket->volume_capacity();
            }
        }
        volume_capacity += w.check_for_free_space();
    }
    return volume_capacity;
}

units::volume Character::volume_capacity() const
{
    units::volume volume_capacity = 0_ml;
    volume_capacity += weapon.contents.total_container_capacity();
    for( const item &w : worn ) {
        volume_capacity += w.contents.total_container_capacity();
    }
    return volume_capacity;
}

units::volume Character::volume_capacity_with_tweaks( const
        std::vector<std::pair<item_location, int>>
        &locations ) const
{
    std::map<const item *, int> dropping;
    for( const std::pair<item_location, int> &location_pair : locations ) {
        dropping.emplace( location_pair.first.get_item(), location_pair.second );
    }
    return volume_capacity_with_tweaks( item_tweaks( { dropping } ) );
}

units::volume Character::volume_capacity_with_tweaks( const item_tweaks &tweaks ) const
{
    const std::map<const item *, int> empty;
    const std::map<const item *, int> &without = tweaks.without_items ? tweaks.without_items->get() :
            empty;

    units::volume volume_capacity = 0_ml;

    if( !without.count( &weapon ) ) {
        volume_capacity += weapon.get_total_capacity();
    }

    for( const item &i : worn ) {
        if( !without.count( &i ) ) {
            volume_capacity += i.get_total_capacity();
        }
    }

    return volume_capacity;
}

units::volume Character::volume_carried() const
{
    return volume_capacity() - free_space();
}

void Character::hoarder_morale_penalty()
{
    int pen = free_space() / 125_ml;
    if( pen > 70 ) {
        pen = 70;
    }
    if( pen <= 0 ) {
        pen = 0;
    }
    if( has_effect( effect_took_xanax ) ) {
        pen = pen / 7;
    } else if( has_effect( effect_took_prozac ) ) {
        pen = pen / 2;
    }
    if( pen > 0 ) {
        add_morale( MORALE_PERM_HOARDER, -pen, -pen, 1_minutes, 1_minutes, true );
    }
}

void Character::apply_persistent_morale()
{
    // Hoarders get a morale penalty if they're not carrying a full inventory.
    if( has_trait( trait_HOARDER ) ) {
        hoarder_morale_penalty();
    }
    // Nomads get a morale penalty if they stay near the same overmap tiles too long.
    if( has_trait( trait_NOMAD ) || has_trait( trait_NOMAD2 ) || has_trait( trait_NOMAD3 ) ) {
        const tripoint_abs_omt ompos = global_omt_location();
        float total_time = 0.0f;
        // Check how long we've stayed in any overmap tile within 5 of us.
        const int max_dist = 5;
        for( const tripoint_abs_omt &pos : points_in_radius( ompos, max_dist ) ) {
            const float dist = rl_dist( ompos, pos );
            if( dist > max_dist ) {
                continue;
            }
            const auto iter = overmap_time.find( pos.xy() );
            if( iter == overmap_time.end() ) {
                continue;
            }
            // Count time in own tile fully, tiles one away as 4/5, tiles two away as 3/5, etc.
            total_time += to_moves<float>( iter->second ) * ( max_dist - dist ) / max_dist;
        }
        // Characters with higher tiers of Nomad suffer worse morale penalties, faster.
        int max_unhappiness;
        float min_time, max_time;
        if( has_trait( trait_NOMAD ) ) {
            max_unhappiness = 20;
            min_time = to_moves<float>( 12_hours );
            max_time = to_moves<float>( 1_days );
        } else if( has_trait( trait_NOMAD2 ) ) {
            max_unhappiness = 40;
            min_time = to_moves<float>( 4_hours );
            max_time = to_moves<float>( 8_hours );
        } else { // traid_NOMAD3
            max_unhappiness = 60;
            min_time = to_moves<float>( 1_hours );
            max_time = to_moves<float>( 2_hours );
        }
        // The penalty starts at 1 at min_time and scales up to max_unhappiness at max_time.
        const float t = ( total_time - min_time ) / ( max_time - min_time );
        const int pen = std::ceil( lerp_clamped( 0, max_unhappiness, t ) );
        if( pen > 0 ) {
            add_morale( MORALE_PERM_NOMAD, -pen, -pen, 1_minutes, 1_minutes, true );
        }
    }

    if( has_trait( trait_PROF_FOODP ) ) {
        // Loosing your face is distressing
        if( !( is_wearing( itype_id( "foodperson_mask" ) ) ||
               is_wearing( itype_id( "foodperson_mask_on" ) ) ) ) {
            add_morale( MORALE_PERM_NOFACE, -20, -20, 1_minutes, 1_minutes, true );
        } else if( is_wearing( itype_id( "foodperson_mask" ) ) ||
                   is_wearing( itype_id( "foodperson_mask_on" ) ) ) {
            rem_morale( MORALE_PERM_NOFACE );
        }

        if( is_wearing( itype_id( "foodperson_mask_on" ) ) ) {
            add_morale( MORALE_PERM_FPMODE_ON, 10, 10, 1_minutes, 1_minutes, true );
        } else {
            rem_morale( MORALE_PERM_FPMODE_ON );
        }
    }
}

int Character::get_morale_level() const
{
    return morale->get_level();
}

void Character::add_morale( const morale_type &type, int bonus, int max_bonus,
                            const time_duration &duration, const time_duration &decay_start,
                            bool capped, const itype *item_type )
{
    morale->add( type, bonus, max_bonus, duration, decay_start, capped, item_type );
}

int Character::has_morale( const morale_type &type ) const
{
    return morale->has( type );
}

void Character::rem_morale( const morale_type &type, const itype *item_type )
{
    morale->remove( type, item_type );
}

void Character::clear_morale()
{
    morale->clear();
}

bool Character::has_morale_to_read() const
{
    return get_morale_level() >= -40;
}

void Character::check_and_recover_morale()
{
    player_morale test_morale;

    for( const item &wit : worn ) {
        test_morale.on_item_wear( wit );
    }

    for( const trait_id &mut : get_mutations() ) {
        test_morale.on_mutation_gain( mut );
    }

    for( auto &elem : *effects ) {
        for( std::pair<const bodypart_id, effect> &_effect_it : elem.second ) {
            const effect &e = _effect_it.second;
            test_morale.on_effect_int_change( e.get_id(), e.get_intensity(), e.get_bp() );
        }
    }

    test_morale.on_stat_change( "hunger", get_hunger() );
    test_morale.on_stat_change( "thirst", get_thirst() );
    test_morale.on_stat_change( "fatigue", get_fatigue() );
    test_morale.on_stat_change( "pain", get_pain() );
    test_morale.on_stat_change( "pkill", get_painkiller() );
    test_morale.on_stat_change( "perceived_pain", get_perceived_pain() );

    apply_persistent_morale();

    if( !morale->consistent_with( test_morale ) ) {
        *morale = player_morale( test_morale ); // Recover consistency
        add_msg_debug( "%s morale was recovered.", disp_name( true ) );
    }
}

void Character::start_hauling()
{
    add_msg( _( "You start hauling items along the ground." ) );
    if( is_armed() ) {
        add_msg( m_warning, _( "Your hands are not free, which makes hauling slower." ) );
    }
    hauling = true;
}

void Character::stop_hauling()
{
    if( hauling ) {
        add_msg( _( "You stop hauling items." ) );
        hauling = false;
    }
    if( has_activity( ACT_MOVE_ITEMS ) ) {
        cancel_activity();
    }
}

bool Character::is_hauling() const
{
    return hauling;
}

void Character::assign_activity( const activity_id &type, int moves, int index, int pos,
                                 const std::string &name )
{
    assign_activity( player_activity( type, moves, index, pos, name ) );
}

void Character::assign_activity( const player_activity &act, bool allow_resume )
{
    bool resuming = false;
    if( allow_resume && !backlog.empty() && backlog.front().can_resume_with( act, *this ) ) {
        resuming = true;
        add_msg_if_player( _( "You resume your task." ) );
        activity = backlog.front();
        backlog.pop_front();
    } else {
        if( activity ) {
            backlog.push_front( activity );
        }

        activity = act;
    }

    activity.start_or_resume( *this, resuming );

    if( is_npc() ) {
        cancel_stashed_activity();
        npc *guy = dynamic_cast<npc *>( this );
        guy->set_attitude( NPCATT_ACTIVITY );
        guy->set_mission( NPC_MISSION_ACTIVITY );
        guy->current_activity_id = activity.id();
    }
}

bool Character::has_activity( const activity_id &type ) const
{
    return activity.id() == type;
}

bool Character::has_activity( const std::vector<activity_id> &types ) const
{
    return std::find( types.begin(), types.end(), activity.id() ) != types.end();
}

void Character::cancel_activity()
{
    activity.canceled( *this );
    if( has_activity( ACT_MOVE_ITEMS ) && is_hauling() ) {
        stop_hauling();
    }
    // Clear any backlog items that aren't auto-resume.
    for( auto backlog_item = backlog.begin(); backlog_item != backlog.end(); ) {
        if( backlog_item->auto_resume ) {
            backlog_item++;
        } else {
            backlog_item = backlog.erase( backlog_item );
        }
    }
    // act wait stamina interrupts an ongoing activity.
    // and automatically puts auto_resume = true on it
    // we don't want that to persist if there is another interruption.
    // and player moves elsewhere.
    if( has_activity( ACT_WAIT_STAMINA ) && !backlog.empty() &&
        backlog.front().auto_resume ) {
        backlog.front().auto_resume = false;
    }
    if( activity && activity.is_suspendable() ) {
        activity.allow_distractions();
        backlog.push_front( activity );
    }
    sfx::end_activity_sounds(); // kill activity sounds when canceled
    activity = player_activity();
}

void Character::resume_backlog_activity()
{
    if( !backlog.empty() && backlog.front().auto_resume ) {
        activity = backlog.front();
        activity.auto_resume = false;
        activity.allow_distractions();
        backlog.pop_front();
    }
}

void Character::fall_asleep()
{
    // Communicate to the player that he is using items on the floor
    std::string item_name = is_snuggling();
    if( item_name == "many" ) {
        if( one_in( 15 ) ) {
            add_msg_if_player( _( "You nestle into your pile of clothes for warmth." ) );
        } else {
            add_msg_if_player( _( "You use your pile of clothes for warmth." ) );
        }
    } else if( item_name != "nothing" ) {
        if( one_in( 15 ) ) {
            add_msg_if_player( _( "You snuggle your %s to keep warm." ), item_name );
        } else {
            add_msg_if_player( _( "You use your %s to keep warm." ), item_name );
        }
    }
    if( has_active_mutation( trait_HIBERNATE ) &&
        get_kcal_percent() > 0.8f ) {
        if( is_avatar() ) {
            get_memorial().add( pgettext( "memorial_male", "Entered hibernation." ),
                                pgettext( "memorial_female", "Entered hibernation." ) );
        }
        // some days worth of round-the-clock Snooze.  Cata seasons default to 91 days.
        fall_asleep( 10_days );
        // If you're not fatigued enough for 10 days, you won't sleep the whole thing.
        // In practice, the fatigue from filling the tank from (no msg) to Time For Bed
        // will last about 8 days.
    }

    fall_asleep( 10_hours ); // default max sleep time.
}

void Character::fall_asleep( const time_duration &duration )
{
    if( activity ) {
        if( activity.id() == ACT_TRY_SLEEP ) {
            activity.set_to_null();
        } else {
            cancel_activity();
        }
    }
    add_effect( effect_sleep, duration );
}

void Character::migrate_items_to_storage( bool disintegrate )
{
    inv->visit_items( [&]( const item * it, item * ) {
        if( disintegrate ) {
            if( try_add( *it ) == nullptr ) {
                debugmsg( "ERROR: Could not put %s into inventory.  Check if the profession has enough space.",
                          it->tname() );
                return VisitResponse::ABORT;
            }
        } else {
            item &added = i_add( *it, true, /*avoid=*/nullptr,
                                 /*allow_drop=*/false, /*allow_wield=*/!has_wield_conflicts( *it ) );
            if( added.is_null() ) {
                put_into_vehicle_or_drop( *this, item_drop_reason::tumbling, { *it } );
            }
        }
        return VisitResponse::SKIP;
    } );
    inv->clear();
}

std::string Character::is_snuggling() const
{
    map &here = get_map();
    auto begin = here.i_at( pos() ).begin();
    auto end = here.i_at( pos() ).end();

    if( in_vehicle ) {
        if( const cata::optional<vpart_reference> vp = here.veh_at( pos() ).part_with_feature( VPFLAG_CARGO,
                false ) ) {
            vehicle *const veh = &vp->vehicle();
            const int cargo = vp->part_index();
            if( !veh->get_items( cargo ).empty() ) {
                begin = veh->get_items( cargo ).begin();
                end = veh->get_items( cargo ).end();
            }
        }
    }
    const item *floor_armor = nullptr;
    int ticker = 0;

    // If there are no items on the floor, return nothing
    if( begin == end ) {
        return "nothing";
    }

    for( auto candidate = begin; candidate != end; ++candidate ) {
        if( !candidate->is_armor() ) {
            continue;
        } else if( candidate->volume() > 250_ml && candidate->get_warmth() > 0 &&
                   ( candidate->covers( body_part_torso ) || candidate->covers( body_part_leg_l ) ||
                     candidate->covers( body_part_leg_r ) ) ) {
            floor_armor = &*candidate;
            ticker++;
        }
    }

    if( ticker == 0 ) {
        return "nothing";
    } else if( ticker == 1 ) {
        return floor_armor->type_name();
    } else if( ticker > 1 ) {
        return "many";
    }

    return "nothing";
}

int Character::warmth( const bodypart_id &bp ) const
{
    int ret = 0;
    double warmth = 0.0;

    for( const item &i : worn ) {
        if( i.covers( bp ) ) {
            warmth = i.get_warmth();
            // Wool items do not lose their warmth due to being wet.
            // Warmth is reduced by 0 - 66% based on wetness.
            if( !i.made_of( material_id( "wool" ) ) ) {
                warmth *= 1.0 - 0.66 * get_part_wetness_percentage( bp );
            }
            ret += std::round( warmth );
        }
    }
    ret += get_effect_int( effect_heating_bionic, bp );
    return ret;
}

static int bestwarmth( const std::list< item > &its, const flag_id &flag )
{
    int best = 0;
    for( const item &w : its ) {
        if( w.has_flag( flag ) && w.get_warmth() > best ) {
            best = w.get_warmth();
        }
    }
    return best;
}

int Character::bonus_item_warmth( const bodypart_id &bp ) const
{
    int ret = 0;

    // If the player is not wielding anything big, check if hands can be put in pockets
    if( ( bp == body_part_hand_l || bp == body_part_hand_r ) &&
        weapon.volume() < 500_ml ) {
        ret += bestwarmth( worn, flag_POCKETS );
    }

    // If the player's head is not encumbered, check if hood can be put up
    if( bp == body_part_head && encumb( body_part_head ) < 10 ) {
        ret += bestwarmth( worn, flag_HOOD );
    }

    // If the player's mouth is not encumbered, check if collar can be put up
    if( bp == body_part_mouth && encumb( body_part_mouth ) < 10 ) {
        ret += bestwarmth( worn, flag_COLLAR );
    }

    return ret;
}

bool Character::can_use_floor_warmth() const
{
    return in_sleep_state() ||
           has_activity( activity_id( "ACT_WAIT" ) ) ||
           has_activity( activity_id( "ACT_WAIT_NPC" ) ) ||
           has_activity( activity_id( "ACT_WAIT_STAMINA" ) ) ||
           has_activity( activity_id( "ACT_AUTODRIVE" ) ) ||
           has_activity( activity_id( "ACT_READ" ) ) ||
           has_activity( activity_id( "ACT_SOCIALIZE" ) ) ||
           has_activity( activity_id( "ACT_MEDITATE" ) ) ||
           has_activity( activity_id( "ACT_FISH" ) ) ||
           has_activity( activity_id( "ACT_GAME" ) ) ||
           has_activity( activity_id( "ACT_HAND_CRANK" ) ) ||
           has_activity( activity_id( "ACT_HEATING" ) ) ||
           has_activity( activity_id( "ACT_VIBE" ) ) ||
           has_activity( activity_id( "ACT_TRY_SLEEP" ) ) ||
           has_activity( activity_id( "ACT_OPERATION" ) ) ||
           has_activity( activity_id( "ACT_TREE_COMMUNION" ) ) ||
           has_activity( activity_id( "ACT_EAT_MENU" ) ) ||
           has_activity( activity_id( "ACT_CONSUME_FOOD_MENU" ) ) ||
           has_activity( activity_id( "ACT_CONSUME_DRINK_MENU" ) ) ||
           has_activity( activity_id( "ACT_CONSUME_MEDS_MENU" ) ) ||
           has_activity( activity_id( "ACT_STUDY_SPELL" ) );
}

int Character::floor_bedding_warmth( const tripoint &pos )
{
    map &here = get_map();
    const trap &trap_at_pos = here.tr_at( pos );
    const ter_id ter_at_pos = here.ter( pos );
    const furn_id furn_at_pos = here.furn( pos );
    int floor_bedding_warmth = 0;

    const optional_vpart_position vp = here.veh_at( pos );
    const cata::optional<vpart_reference> boardable = vp.part_with_feature( "BOARDABLE", true );
    // Search the floor for bedding
    if( furn_at_pos != f_null ) {
        floor_bedding_warmth += furn_at_pos.obj().floor_bedding_warmth;
    } else if( !trap_at_pos.is_null() ) {
        floor_bedding_warmth += trap_at_pos.floor_bedding_warmth;
    } else if( boardable ) {
        floor_bedding_warmth += boardable->info().floor_bedding_warmth;
    } else if( ter_at_pos == t_improvised_shelter ) {
        floor_bedding_warmth -= 500;
    } else {
        floor_bedding_warmth -= 2000;
    }

    return floor_bedding_warmth;
}

int Character::floor_item_warmth( const tripoint &pos )
{
    int item_warmth = 0;

    const auto warm = [&item_warmth]( const auto & stack ) {
        for( const item &elem : stack ) {
            if( !elem.is_armor() ) {
                continue;
            }
            // Items that are big enough and covers the torso are used to keep warm.
            // Smaller items don't do as good a job
            if( elem.volume() > 250_ml &&
                ( elem.covers( body_part_torso ) || elem.covers( body_part_leg_l ) ||
                  elem.covers( body_part_leg_r ) ) ) {
                item_warmth += 60 * elem.get_warmth() * elem.volume() / 2500_ml;
            }
        }
    };

    map &here = get_map();

    if( !!here.veh_at( pos ) ) {
        if( const cata::optional<vpart_reference> vp = here.veh_at( pos ).part_with_feature( VPFLAG_CARGO,
                false ) ) {
            vehicle *const veh = &vp->vehicle();
            const int cargo = vp->part_index();
            vehicle_stack vehicle_items = veh->get_items( cargo );
            warm( vehicle_items );
        }
        return item_warmth;
    }
    map_stack floor_items = here.i_at( pos );
    warm( floor_items );
    return item_warmth;
}

int Character::floor_warmth( const tripoint &pos ) const
{
    const int item_warmth = floor_item_warmth( pos );
    int bedding_warmth = floor_bedding_warmth( pos );

    // If the PC has fur, etc, that will apply too
    int floor_mut_warmth = bodytemp_modifier_traits_floor();
    // DOWN does not provide floor insulation, though.
    // Better-than-light fur or being in one's shell does.
    if( ( !( has_trait( trait_DOWN ) ) ) && ( floor_mut_warmth >= 200 ) ) {
        bedding_warmth = std::max( 0, bedding_warmth );
    }
    return ( item_warmth + bedding_warmth + floor_mut_warmth );
}

int Character::bodytemp_modifier_traits( bool overheated ) const
{
    int mod = 0;
    for( const trait_id &iter : get_mutations() ) {
        mod += overheated ? iter->bodytemp_min : iter->bodytemp_max;
    }
    return mod;
}

int Character::bodytemp_modifier_traits_floor() const
{
    int mod = 0;
    for( const trait_id &iter : get_mutations() ) {
        mod += iter->bodytemp_sleep;
    }
    return mod;
}

int Character::temp_corrected_by_climate_control( int temperature ) const
{
    const int variation = static_cast<int>( BODYTEMP_NORM * 0.5 );
    if( temperature < BODYTEMP_SCORCHING + variation &&
        temperature > BODYTEMP_FREEZING - variation ) {
        if( temperature > BODYTEMP_SCORCHING ) {
            temperature = BODYTEMP_VERY_HOT;
        } else if( temperature > BODYTEMP_VERY_HOT ) {
            temperature = BODYTEMP_HOT;
        } else if( temperature > BODYTEMP_HOT ) {
            temperature = BODYTEMP_NORM;
        } else if( temperature < BODYTEMP_FREEZING ) {
            temperature = BODYTEMP_VERY_COLD;
        } else if( temperature < BODYTEMP_VERY_COLD ) {
            temperature = BODYTEMP_COLD;
        } else if( temperature < BODYTEMP_COLD ) {
            temperature = BODYTEMP_NORM;
        }
    }
    return temperature;
}

bool Character::in_sleep_state() const
{
    return Creature::in_sleep_state() || activity.id() == ACT_TRY_SLEEP;
}

bool Character::has_item_with_flag( const flag_id &flag, bool need_charges ) const
{
    return has_item_with( [&flag, &need_charges]( const item & it ) {
        if( it.is_tool() && need_charges ) {
            return it.has_flag( flag ) && it.type->tool->max_charges ? it.charges > 0 : it.has_flag( flag );
        }
        return it.has_flag( flag );
    } );
}

std::vector<const item *> Character::all_items_with_flag( const flag_id &flag ) const
{
    return items_with( [&flag]( const item & it ) {
        return it.has_flag( flag );
    } );
}

bool Character::has_charges( const itype_id &it, int quantity,
                             const std::function<bool( const item & )> &filter ) const
{
    if( it == itype_fire || it == itype_apparatus ) {
        return has_fire( quantity );
    }
    if( it == itype_UPS && is_mounted() &&
        mounted_creature.get()->has_flag( MF_RIDEABLE_MECH ) ) {
        auto *mons = mounted_creature.get();
        return quantity <= mons->battery_item->ammo_remaining();
    }
    return charges_of( it, quantity, filter ) == quantity;
}

std::list<item> Character::use_amount( const itype_id &it, int quantity,
                                       const std::function<bool( const item & )> &filter )
{
    std::list<item> ret;
    if( weapon.use_amount( it, quantity, ret ) ) {
        remove_weapon();
    }
    for( auto a = worn.begin(); a != worn.end() && quantity > 0; ) {
        if( a->use_amount( it, quantity, ret, filter ) ) {
            a->on_takeoff( *this );
            a = worn.erase( a );
        } else {
            ++a;
        }
    }
    if( quantity <= 0 ) {
        return ret;
    }
    std::list<item> tmp = inv->use_amount( it, quantity, filter );
    ret.splice( ret.end(), tmp );
    return ret;
}

bool Character::use_charges_if_avail( const itype_id &it, int quantity )
{
    if( has_charges( it, quantity ) ) {
        use_charges( it, quantity );
        return true;
    }
    return false;
}

std::list<item> Character::use_charges( const itype_id &what, int qty, const int radius,
                                        const std::function<bool( const item & )> &filter )
{
    std::list<item> res;
    inventory inv = crafting_inventory( pos(), radius, true );

    if( qty <= 0 ) {
        return res;

    } else if( what == itype_toolset ) {
        mod_power_level( units::from_kilojoule( -qty ) );
        return res;

    } else if( what == itype_fire ) {
        use_fire( qty );
        return res;

    } else if( what == itype_UPS ) {
        if( is_mounted() && mounted_creature.get()->has_flag( MF_RIDEABLE_MECH ) &&
            mounted_creature.get()->battery_item ) {
            auto *mons = mounted_creature.get();
            int power_drain = std::min( mons->battery_item->ammo_remaining(), qty );
            mons->use_mech_power( -power_drain );
            qty -= std::min( qty, power_drain );
            return res;
        }
        if( has_power() && has_active_bionic( bio_ups ) ) {
            int bio = std::min( units::to_kilojoule( get_power_level() ), qty );
            mod_power_level( units::from_kilojoule( -bio ) );
            qty -= std::min( qty, bio );
        }

        int adv = inv.charges_of( itype_adv_UPS_off, static_cast<int>( std::ceil( qty * 0.6 ) ) );
        if( adv > 0 ) {
            std::list<item> found = use_charges( itype_adv_UPS_off, adv, radius );
            res.splice( res.end(), found );
            qty -= std::min( qty, static_cast<int>( adv / 0.6 ) );
        }

        int ups = inv.charges_of( itype_UPS_off, qty );
        if( ups > 0 ) {
            std::list<item> found = use_charges( itype_UPS_off, ups, radius );
            res.splice( res.end(), found );
            qty -= std::min( qty, ups );
        }
        return res;
    }

    std::vector<item *> del;

    bool has_tool_with_UPS = false;
    // Detection of UPS tool
    inv.visit_items( [ &what, &qty, &has_tool_with_UPS, &filter]( item * e, item * ) {
        if( filter( *e ) && e->typeId() == what && e->has_flag( flag_USE_UPS ) ) {
            has_tool_with_UPS = true;
            return VisitResponse::ABORT;
        }
        return qty > 0 ? VisitResponse::NEXT : VisitResponse::ABORT;
    } );

    if( radius >= 0 ) {
        get_map().use_charges( pos(), radius, what, qty, return_true<item> );
    }
    if( qty > 0 ) {
        visit_items( [this, &what, &qty, &res, &del, &filter]( item * e, item * ) {
            if( e->use_charges( what, qty, res, pos(), filter ) ) {
                del.push_back( e );
            }
            return qty > 0 ? VisitResponse::NEXT : VisitResponse::ABORT;
        } );
    }

    for( item *e : del ) {
        remove_item( *e );
    }

    if( has_tool_with_UPS ) {
        use_charges( itype_UPS, qty, radius );
    }

    return res;
}

std::list<item> Character::use_charges( const itype_id &what, int qty,
                                        const std::function<bool( const item & )> &filter )
{
    return use_charges( what, qty, -1, filter );
}

const item *Character::find_firestarter_with_charges( const int quantity ) const
{
    for( const item *i : all_items_with_flag( flag_FIRESTARTER ) ) {
        if( !i->typeId()->can_have_charges() ) {
            const use_function *usef = i->type->get_use( "firestarter" );
            const firestarter_actor *actor = dynamic_cast<const firestarter_actor *>( usef->get_actor_ptr() );
            if( actor->can_use( *this->as_character(), *i, false, tripoint_zero ).success() ) {
                return i;
            }
        } else if( has_charges( i->typeId(), quantity ) ) {
            return i;
        }
    }

    return nullptr;
}

bool Character::has_fire( const int quantity ) const
{
    // TODO: Replace this with a "tool produces fire" flag.

    if( get_map().has_nearby_fire( pos() ) ) {
        return true;
    } else if( has_item_with_flag( flag_FIRE ) ) {
        return true;
    } else if( find_firestarter_with_charges( quantity ) ) {
        return true;
    } else if( has_active_bionic( bio_tools ) && get_power_level() > quantity * 5_kJ ) {
        return true;
    } else if( has_bionic( bio_lighter ) && get_power_level() > quantity * 5_kJ ) {
        return true;
    } else if( has_bionic( bio_laser ) && get_power_level() > quantity * 5_kJ ) {
        return true;
    } else if( is_npc() ) {
        // HACK: A hack to make NPCs use their Molotovs
        return true;
    }

    return false;
}

void Character::mod_painkiller( int npkill )
{
    set_painkiller( pkill + npkill );
}

void Character::set_painkiller( int npkill )
{
    npkill = std::max( npkill, 0 );
    if( pkill != npkill ) {
        const int prev_pain = get_perceived_pain();
        pkill = npkill;
        on_stat_change( "pkill", pkill );
        const int cur_pain = get_perceived_pain();

        if( cur_pain != prev_pain ) {
            react_to_felt_pain( cur_pain - prev_pain );
            on_stat_change( "perceived_pain", cur_pain );
        }
    }
}

int Character::get_painkiller() const
{
    return pkill;
}

void Character::use_fire( const int quantity )
{
    //Okay, so checks for nearby fires first,
    //then held lit torch or candle, bionic tool/lighter/laser
    //tries to use 1 charge of lighters, matches, flame throwers
    //If there is enough power, will use power of one activation of the bio_lighter, bio_tools and bio_laser
    // (home made, military), hotplate, welder in that order.
    // bio_lighter, bio_laser, bio_tools, has_active_bionic("bio_tools"

    if( get_map().has_nearby_fire( pos() ) ) {
        return;
    } else if( has_item_with_flag( flag_FIRE ) ) {
        return;
    } else if( const item *firestarter = find_firestarter_with_charges( quantity ) ) {
        if( firestarter->typeId()->can_have_charges() ) {
            use_charges( firestarter->typeId(), quantity );
            return;
        }
    } else if( has_active_bionic( bio_tools ) && get_power_level() > quantity * 5_kJ ) {
        mod_power_level( -quantity * 5_kJ );
        return;
    } else if( has_bionic( bio_lighter ) && get_power_level() > quantity * 5_kJ ) {
        mod_power_level( -quantity * 5_kJ );
        return;
    } else if( has_bionic( bio_laser ) && get_power_level() > quantity * 5_kJ ) {
        mod_power_level( -quantity * 5_kJ );
        return;
    }
}

int Character::heartrate_bpm() const
{
    //Dead have no heartbeat usually and no heartbeat in omnicell
    if( is_dead_state() || has_trait( trait_SLIMESPAWNER ) ) {
        return 0;
    }
    //This function returns heartrate in BPM basing of health, physical state, tiredness,
    //moral effects, stimulators and anything that should fit here.
    //Some values are picked to make sense from math point of view
    //and seem correct but effects may vary in real life.
    //This needs more attention from experienced contributors to work more smooth.
    //Average healthy bpm is 60-80. That's a simple imitation of normal distribution.
    //Must a better way to do that. Possibly this value should be generated with player creation.
    int average_heartbeat = 70 + rng( -5, 5 ) + rng( -5, 5 );
    //Chemical imbalance makes this less predictable. It's possible this range needs tweaking
    if( has_trait( trait_CHEMIMBALANCE ) ) {
        average_heartbeat += rng( -15, 15 );
    }
    //Quick also raises basic BPM
    if( has_trait( trait_QUICK ) ) {
        average_heartbeat *= 1.1;
    }
    //Badtemper makes your BPM raise from anger
    if( has_trait( trait_BADTEMPER ) ) {
        average_heartbeat *= 1.1;
    }
    //COLDBLOOD dependencies, works almost same way as temperature effect for speed.
    const int player_local_temp = get_weather().get_temperature( pos() );
    float temperature_modifier = 0.0f;
    if( has_trait( trait_COLDBLOOD ) ) {
        temperature_modifier = 0.002f;
    }
    if( has_trait( trait_COLDBLOOD2 ) ) {
        temperature_modifier = 0.00333f;
    }
    if( has_trait( trait_COLDBLOOD3 ) || has_trait( trait_COLDBLOOD4 ) ) {
        temperature_modifier = 0.005f;
    }
    average_heartbeat *= 1 + ( ( player_local_temp - 65 ) * temperature_modifier );
    //Limit avg from below with 20, arbitrary
    average_heartbeat = std::max( 20, average_heartbeat );
    const float stamina_level = static_cast<float>( get_stamina() ) / get_stamina_max();
    float stamina_effect = 0.0f;
    if( stamina_level >= 0.9f ) {
        stamina_effect = 0.0f;
    } else if( stamina_level >= 0.8f ) {
        stamina_effect = 0.2f;
    } else if( stamina_level >= 0.6f ) {
        stamina_effect = 0.5f;
    } else if( stamina_level >= 0.4f ) {
        stamina_effect = 1.0f;
    } else if( stamina_level >= 0.2f ) {
        stamina_effect = 1.5f;
    } else {
        stamina_effect = 2.0f;
    }
    //can triple heartrate
    int heartbeat = average_heartbeat * ( 1 + stamina_effect );
    const int stim_level = get_stim();
    int stim_modifer = 0;
    if( stim_level > 0 ) {
        //that's asymptotical function that is equal to 1 at around 30 stim level
        //and slows down all the time almost reaching 2.
        //Tweaking x*x multiplier will accordingly change effect accumulation
        stim_modifer = 2 - 2 / ( 1 + 0.001 * stim_level * stim_level );
    }
    heartbeat += average_heartbeat * stim_modifer;
    if( get_effect_dur( effect_cig ) > 0_turns ) {
        //Nicotine-induced tachycardia
        if( get_effect_dur( effect_cig ) > 10_minutes * ( addiction_level( add_type::CIG ) + 1 ) ) {
            heartbeat += average_heartbeat * 0.4;
        } else {
            heartbeat += average_heartbeat * 0.1;
        }
    }
    //health effect that can make things better or worse is applied in the end.
    //Based on get_max_healthy that already has bmi factored
    const int healthy = get_max_healthy();
    //a bit arbitrary formula that can use some love
    float healthy_modifier = -0.05f * std::round( healthy / 20.0f );
    heartbeat += average_heartbeat * healthy_modifier;
    //Pain simply adds 2% per point after it reaches 5 (that's arbitrary)
    const int cur_pain = get_perceived_pain();
    float pain_modifier = 0.0f;
    if( cur_pain > 5 ) {
        pain_modifier = 0.02 * ( cur_pain - 5 );
    }
    heartbeat += average_heartbeat * pain_modifier;
    //if BPM raised at least by 20% for a player with ADRENALINE, it adds 20% of avg to result
    if( has_trait( trait_ADRENALINE ) && heartbeat > average_heartbeat * 1.2 ) {
        heartbeat += average_heartbeat * 0.2;
    }
    //Happy get it bit faster and miserable some more.
    //Morale effects might need more consideration
    const int morale_level = get_morale_level();
    if( morale_level >= 20 ) {
        heartbeat += average_heartbeat * 0.1;
    }
    if( morale_level <= -20 ) {
        heartbeat += average_heartbeat * 0.2;
    }
    //add fear?
    //A single clamp in the end should be enough
    const int max_heartbeat = average_heartbeat * 3.5;
    heartbeat = clamp( heartbeat, average_heartbeat, max_heartbeat );
    return heartbeat;
}

void Character::on_worn_item_washed( const item &it )
{
    if( is_worn( it ) ) {
        morale->on_worn_item_washed( it );
    }
}

void Character::on_item_wear( const item &it )
{
    invalidate_inventory_validity_cache();
    for( const trait_id &mut : it.mutations_from_wearing( *this ) ) {
        mutation_effect( mut, true );
        recalc_sight_limits();
        calc_encumbrance();

        // If the stamina is higher than the max (Languorous), set it back to max
        if( get_stamina() > get_stamina_max() ) {
            set_stamina( get_stamina_max() );
        }
    }
    morale->on_item_wear( it );
}

void Character::on_item_takeoff( const item &it )
{
    invalidate_inventory_validity_cache();
    for( const trait_id &mut : it.mutations_from_wearing( *this ) ) {
        mutation_loss_effect( mut );
        recalc_sight_limits();
        calc_encumbrance();
        if( get_stamina() > get_stamina_max() ) {
            set_stamina( get_stamina_max() );
        }
    }
    morale->on_item_takeoff( it );
}

void Character::on_effect_int_change( const efftype_id &eid, int intensity, const bodypart_id &bp )
{
    // Adrenaline can reduce perceived pain (or increase it when you enter comedown).
    // See @ref get_perceived_pain()
    if( eid == effect_adrenaline ) {
        // Note that calling this does no harm if it wasn't changed.
        on_stat_change( "perceived_pain", get_perceived_pain() );
    }

    morale->on_effect_int_change( eid, intensity, bp );
}

void Character::on_mutation_gain( const trait_id &mid )
{
    morale->on_mutation_gain( mid );
    magic->on_mutation_gain( mid, *this );
    update_type_of_scent( mid );
    recalculate_enchantment_cache(); // mutations can have enchantments
}

void Character::on_mutation_loss( const trait_id &mid )
{
    morale->on_mutation_loss( mid );
    magic->on_mutation_loss( mid );
    update_type_of_scent( mid, false );
    recalculate_enchantment_cache(); // mutations can have enchantments
}

void Character::on_stat_change( const std::string &stat, int value )
{
    morale->on_stat_change( stat, value );
}

bool Character::has_opposite_trait( const trait_id &flag ) const
{
    for( const trait_id &i : flag->cancels ) {
        if( has_trait( i ) ) {
            return true;
        }
    }
    for( const std::pair<const trait_id, trait_data> &mut : my_mutations ) {
        for( const trait_id &canceled_trait : mut.first->cancels ) {
            if( canceled_trait == flag ) {
                return true;
            }
        }
    }
    return false;
}

int Character::adjust_for_focus( int amount ) const
{
    int effective_focus = get_focus();
    if( has_trait( trait_FASTLEARNER ) ) {
        effective_focus += 15;
    }
    if( has_active_bionic( bio_memory ) ) {
        effective_focus += 10;
    }
    if( has_trait( trait_SLOWLEARNER ) ) {
        effective_focus -= 15;
    }
    effective_focus += ( get_int() - get_option<int>( "INT_BASED_LEARNING_BASE_VALUE" ) ) *
                       get_option<int>( "INT_BASED_LEARNING_FOCUS_ADJUSTMENT" );
    effective_focus = std::max( effective_focus, 1 );
    return effective_focus * std::min( amount, 1000 );
}

std::set<tripoint> Character::get_path_avoid() const
{
    std::set<tripoint> ret;
    for( npc &guy : g->all_npcs() ) {
        if( sees( guy ) ) {
            ret.insert( guy.pos() );
        }
    }

    // TODO: Add known traps in a way that doesn't destroy performance

    return ret;
}

const pathfinding_settings &Character::get_pathfinding_settings() const
{
    return *path_settings;
}

bool Character::crush_frozen_liquid( item_location loc )
{
    if( has_quality( quality_id( "HAMMER" ) ) ) {
        item hammering_item = item_with_best_of_quality( quality_id( "HAMMER" ) );
        //~ %1$s: item to be crushed, %2$s: hammer name
        if( query_yn( _( "Do you want to crush up %1$s with your %2$s?\n"
                         "<color_red>Be wary of fragile items nearby!</color>" ),
                      loc.get_item()->display_name(), hammering_item.tname() ) ) {

            //Risk smashing tile with hammering tool, risk is lower with higher dex, damage lower with lower strength
            if( one_in( 1 + dex_cur / 4 ) ) {
                add_msg_if_player( colorize( _( "You swing your %s wildly!" ), c_red ),
                                   hammering_item.tname() );
                int smashskill = str_cur + hammering_item.damage_melee( damage_type::BASH );
                get_map().bash( loc.position(), smashskill );
            }
            add_msg_if_player( _( "You crush up and gather %s" ), loc.get_item()->display_name() );
            return true;
        }
    } else {
        popup( _( "You need a hammering tool to crush up frozen liquids!" ) );
    }
    return false;
}

float Character::power_rating() const
{
    int dmg = std::max( { weapon.damage_melee( damage_type::BASH ),
                          weapon.damage_melee( damage_type::CUT ),
                          weapon.damage_melee( damage_type::STAB )
                        } );

    int ret = 2;
    // Small guns can be easily hidden from view
    if( weapon.volume() <= 250_ml ) {
        ret = 2;
    } else if( weapon.is_gun() ) {
        ret = 4;
    } else if( dmg > 12 ) {
        ret = 3; // Melee weapon or weapon-y tool
    }
    if( get_size() == creature_size::huge ) {
        ret += 1;
    }
    if( is_wearing_power_armor( nullptr ) ) {
        ret = 5; // No mercy!
    }
    return ret;
}

float Character::speed_rating() const
{
    float ret = get_speed() / 100.0f;
    ret *= 100.0f / run_cost( 100, false );
    // Adjustment for player being able to run, but not doing so at the moment
    if( !is_running() ) {
        ret *= 1.0f + ( static_cast<float>( get_stamina() ) / static_cast<float>( get_stamina_max() ) );
    }
    return ret;
}

item &Character::item_with_best_of_quality( const quality_id &qid )
{
    int maxq = max_quality( qid );
    auto items_with_quality = items_with( [qid]( const item & it ) {
        return it.has_quality( qid );
    } );
    for( item *it : items_with_quality ) {
        if( it->get_quality( qid ) == maxq ) {
            return *it;
        }
    }
    return null_item_reference();
}

int Character::run_cost( int base_cost, bool diag ) const
{
    float movecost = static_cast<float>( base_cost );
    if( diag ) {
        movecost *= 0.7071f; // because everything here assumes 100 is base
    }
    const bool flatground = movecost < 105;
    map &here = get_map();
    // The "FLAT" tag includes soft surfaces, so not a good fit.
    const bool on_road = flatground && here.has_flag( STATIC( "ROAD" ), pos() );
    const bool on_fungus = here.has_flag_ter_or_furn( STATIC( "FUNGUS" ), pos() );

    if( !is_mounted() ) {
        if( movecost > 100 ) {
            movecost *= mutation_value( "movecost_obstacle_modifier" );
            if( movecost < 100 ) {
                movecost = 100;
            }
        }
        if( has_trait( trait_M_IMMUNE ) && on_fungus ) {
            if( movecost > 75 ) {
                // Mycal characters are faster on their home territory, even through things like shrubs
                movecost = 75;
            }
        }

        // Linearly increase move cost relative to individual leg hp.
        movecost += 50 * ( 1 - std::sqrt( static_cast<float>( get_part_hp_cur( body_part_leg_l ) ) /
                                          static_cast<float>( get_part_hp_max( body_part_leg_l ) ) ) );
        movecost += 50 * ( 1 - std::sqrt( static_cast<float>( get_part_hp_cur( body_part_leg_r ) ) /
                                          static_cast<float>( get_part_hp_max( body_part_leg_r ) ) ) );

        movecost *= mutation_value( "movecost_modifier" );
        if( flatground ) {
            movecost *= mutation_value( "movecost_flatground_modifier" );
        }
        if( has_trait( trait_PADDED_FEET ) && !footwear_factor() ) {
            movecost *= .9f;
        }
        if( has_active_bionic( bio_jointservo ) ) {
            if( is_running() ) {
                movecost *= 0.85f;
            } else {
                movecost *= 0.95f;
            }
        } else if( has_bionic( bio_jointservo ) ) {
            movecost *= 1.1f;
        }

        if( worn_with_flag( flag_SLOWS_MOVEMENT ) ) {
            movecost *= 1.1f;
        }
        if( worn_with_flag( flag_FIN ) ) {
            movecost *= 1.5f;
        }
        if( worn_with_flag( flag_ROLLER_INLINE ) ) {
            if( on_road ) {
                movecost *= 0.5f;
            } else {
                movecost *= 1.5f;
            }
        }
        // Quad skates might be more stable than inlines,
        // but that also translates into a slower speed when on good surfaces.
        if( worn_with_flag( flag_ROLLER_QUAD ) ) {
            if( on_road ) {
                movecost *= 0.7f;
            } else {
                movecost *= 1.3f;
            }
        }
        // Skates with only one wheel (roller shoes) are fairly less stable
        // and fairly slower as well
        if( worn_with_flag( flag_ROLLER_ONE ) ) {
            if( on_road ) {
                movecost *= 0.85f;
            } else {
                movecost *= 1.1f;
            }
        }

        movecost +=
            ( ( encumb( body_part_foot_l ) + encumb( body_part_foot_r ) ) * 2.5 +
              ( encumb( body_part_leg_l ) + encumb( body_part_leg_r ) ) * 1.5 ) / 10;

        // ROOTS3 does slow you down as your roots are probing around for nutrients,
        // whether you want them to or not.  ROOTS1 is just too squiggly without shoes
        // to give you some stability.  Plants are a bit of a slow-mover.  Deal.
        const bool mutfeet = has_trait( trait_LEG_TENTACLES ) || has_trait( trait_PADDED_FEET ) ||
                             has_trait( trait_HOOVES ) || has_trait( trait_TOUGH_FEET ) || has_trait( trait_ROOTS2 );
        if( !is_wearing_shoes( side::LEFT ) && !mutfeet ) {
            movecost += 8;
        }
        if( !is_wearing_shoes( side::RIGHT ) && !mutfeet ) {
            movecost += 8;
        }

        if( has_trait( trait_ROOTS3 ) && here.has_flag( STATIC( "DIGGABLE" ), pos() ) ) {
            movecost += 10 * footwear_factor();
        }

        movecost = calculate_by_enchantment( movecost, enchant_vals::mod::MOVE_COST );
        movecost /= stamina_move_cost_modifier();
    }

    if( diag ) {
        movecost *= M_SQRT2;
    }

    return static_cast<int>( movecost );
}

void Character::place_corpse()
{
    //If the character/NPC is on a distant mission, don't drop their their gear when they die since they still have a local pos
    if( !death_drops ) {
        return;
    }
    std::vector<item *> tmp = inv_dump();
    item body = item::make_corpse( mtype_id::NULL_ID(), calendar::turn, name );
    body.set_item_temperature( 310.15 );
    map &here = get_map();
    for( item *itm : tmp ) {
        here.add_item_or_charges( pos(), *itm );
    }
    for( const bionic &bio : *my_bionics ) {
        if( item::type_is_defined( bio.info().itype() ) ) {
            item cbm( bio.id.str(), calendar::turn );
            cbm.set_flag( flag_FILTHY );
            cbm.set_flag( flag_NO_STERILE );
            cbm.set_flag( flag_NO_PACKED );
            cbm.faults.emplace( fault_id( "fault_bionic_salvaged" ) );
            body.put_in( cbm, item_pocket::pocket_type::CORPSE );
        }
    }

    // Restore amount of installed pseudo-modules of Power Storage Units
    std::pair<int, int> storage_modules = amount_of_storage_bionics();
    for( int i = 0; i < storage_modules.first; ++i ) {
        body.put_in( item( "bio_power_storage" ), item_pocket::pocket_type::CORPSE );
    }
    for( int i = 0; i < storage_modules.second; ++i ) {
        body.put_in( item( "bio_power_storage_mkII" ), item_pocket::pocket_type::CORPSE );
    }
    here.add_item_or_charges( pos(), body );
}

void Character::place_corpse( const tripoint_abs_omt &om_target )
{
    tinymap bay;
    bay.load( project_to<coords::sm>( om_target ), false );
    point fin( rng( 1, SEEX * 2 - 2 ), rng( 1, SEEX * 2 - 2 ) );
    // This makes no sense at all. It may find a random tile without furniture, but
    // if the first try to find one fails, it will go through all tiles of the map
    // and essentially select the last one that has no furniture.
    // Q: Why check for furniture? (Check for passable or can-place-items seems more useful.)
    // Q: Why not grep a random point out of all the possible points (e.g. via random_entry)?
    // Q: Why use furn_str_id instead of f_null?
    // TODO: fix it, see above.
    if( bay.furn( fin ) != furn_str_id( "f_null" ) ) {
        for( const tripoint &p : bay.points_on_zlevel() ) {
            if( bay.furn( p ) == furn_str_id( "f_null" ) ) {
                fin.x = p.x;
                fin.y = p.y;
            }
        }
    }

    std::vector<item *> tmp = inv_dump();
    item body = item::make_corpse( mtype_id::NULL_ID(), calendar::turn, name );
    for( item *itm : tmp ) {
        bay.add_item_or_charges( fin, *itm );
    }
    for( const bionic &bio : *my_bionics ) {
        if( item::type_is_defined( bio.info().itype() ) ) {
            body.put_in( item( bio.id.str(), calendar::turn ), item_pocket::pocket_type::CORPSE );
        }
    }

    // Restore amount of installed pseudo-modules of Power Storage Units
    std::pair<int, int> storage_modules = amount_of_storage_bionics();
    for( int i = 0; i < storage_modules.first; ++i ) {
        body.put_in( item( "bio_power_storage" ), item_pocket::pocket_type::CORPSE );
    }
    for( int i = 0; i < storage_modules.second; ++i ) {
        body.put_in( item( "bio_power_storage_mkII" ), item_pocket::pocket_type::CORPSE );
    }
    bay.add_item_or_charges( fin, body );
}

bool Character::sees_with_infrared( const Creature &critter ) const
{
    if( !vision_mode_cache[IR_VISION] || !critter.is_warm() ) {
        return false;
    }

    map &here = get_map();
    if( is_player() || critter.is_player() ) {
        // Players should not use map::sees
        // Likewise, players should not be "looked at" with map::sees, not to break symmetry
        return here.pl_line_of_sight( critter.pos(),
                                      sight_range( current_daylight_level( calendar::turn ) ) );
    }

    return here.sees( pos(), critter.pos(), sight_range( current_daylight_level( calendar::turn ) ) );
}

bool Character::is_visible_in_range( const Creature &critter, const int range ) const
{
    return sees( critter ) && rl_dist( pos(), critter.pos() ) <= range;
}

std::vector<Creature *> Character::get_visible_creatures( const int range ) const
{
    return g->get_creatures_if( [this, range]( const Creature & critter ) -> bool {
        return this != &critter && pos() != critter.pos() && // TODO: get rid of fake npcs (pos() check)
        rl_dist( pos(), critter.pos() ) <= range && sees( critter );
    } );
}

std::vector<Creature *> Character::get_targetable_creatures( const int range, bool melee ) const
{
    map &here = get_map();
    return g->get_creatures_if( [this, range, melee, &here]( const Creature & critter ) -> bool {
        //the call to map.sees is to make sure that even if we can see it through walls
        //via a mutation or cbm we only attack targets with a line of sight
        bool can_see = ( ( sees( critter ) || sees_with_infrared( critter ) ) && here.sees( pos(), critter.pos(), 100 ) );
        if( can_see && melee )  //handles the case where we can see something with glass in the way for melee attacks
        {
            std::vector<tripoint> path = here.find_clear_path( pos(), critter.pos() );
            for( const tripoint &point : path ) {
                if( here.impassable( point ) &&
                    !( weapon.has_flag( flag_SPEAR ) && // Fences etc. Spears can stab through those
                       here.has_flag( STATIC( "THIN_OBSTACLE" ),
                                      point ) ) ) { //this mirrors melee.cpp function reach_attack
                    can_see = false;
                    break;
                }
            }
        }
        bool in_range = std::round( rl_dist_exact( pos(), critter.pos() ) ) <= range;
        // TODO: get rid of fake npcs (pos() check)
        bool valid_target = this != &critter && pos() != critter.pos() && attitude_to( critter ) != Creature::Attitude::FRIENDLY;
        return valid_target && in_range && can_see;
    } );
}

std::vector<Creature *> Character::get_hostile_creatures( int range ) const
{
    return g->get_creatures_if( [this, range]( const Creature & critter ) -> bool {
        // Fixes circular distance range for ranged attacks
        float dist_to_creature = std::round( rl_dist_exact( pos(), critter.pos() ) );
        return this != &critter && pos() != critter.pos() && // TODO: get rid of fake npcs (pos() check)
        dist_to_creature <= range && critter.attitude_to( *this ) == Creature::Attitude::HOSTILE
        && sees( critter );
    } );
}

bool Character::knows_trap( const tripoint &pos ) const
{
    const tripoint p = get_map().getabs( pos );
    return known_traps.count( p ) > 0;
}

void Character::add_known_trap( const tripoint &pos, const trap &t )
{
    const tripoint p = get_map().getabs( pos );
    if( t.is_null() ) {
        known_traps.erase( p );
    } else {
        // TODO: known_traps should map to a trap_str_id
        known_traps[p] = t.id.str();
    }
}

bool Character::can_hear( const tripoint &source, const int volume ) const
{
    if( is_deaf() ) {
        return false;
    }

    // source is in-ear and at our square, we can hear it
    if( source == pos() && volume == 0 ) {
        return true;
    }
    const int dist = rl_dist( source, pos() );
    const float volume_multiplier = hearing_ability();
    return ( volume - get_weather().weather_id->sound_attn ) * volume_multiplier >= dist;
}

float Character::hearing_ability() const
{
    float volume_multiplier = 1.0f;

    // Mutation/Bionic volume modifiers
    if( has_flag( json_flag_SUPER_HEARING ) ) {
        volume_multiplier *= 3.5f;
    }
    if( has_trait( trait_PER_SLIME ) ) {
        // Random hearing :-/
        // (when it's working at all, see player.cpp)
        // changed from 0.5 to fix Mac compiling error
        volume_multiplier *= ( rng( 1, 2 ) );
    }

    volume_multiplier *= Character::mutation_value( "hearing_modifier" );

    if( has_effect( effect_deaf ) ) {
        // Scale linearly up to 30 minutes
        volume_multiplier *= ( 30_minutes - get_effect_dur( effect_deaf ) ) / 30_minutes;
    }

    if( has_effect( effect_earphones ) ) {
        volume_multiplier *= 0.25f;
    }

    return volume_multiplier;
}

std::vector<std::string> Character::short_description_parts() const
{
    std::vector<std::string> result;

    if( is_armed() ) {
        result.push_back( _( "Wielding: " ) + weapon.tname() );
    }
    const std::string worn_str = enumerate_as_string( worn.begin(), worn.end(),
    []( const item & it ) {
        return it.tname();
    } );
    if( !worn_str.empty() ) {
        result.push_back( _( "Wearing: " ) + worn_str );
    }
    const int visibility_cap = 0; // no cap
    const auto trait_str = visible_mutations( visibility_cap );
    if( !trait_str.empty() ) {
        result.push_back( _( "Traits: " ) + trait_str );
    }
    return result;
}

std::string Character::short_description() const
{
    return join( short_description_parts(), ";   " );
}

void Character::process_one_effect( effect &it, bool is_new )
{
    bool reduced = resists_effect( it );
    double mod = 1;
    const bodypart_id &bp = it.get_bp();
    int val = 0;

    // Still hardcoded stuff, do this first since some modify their other traits
    hardcoded_effects( it );

    const auto get_effect = [&it, is_new]( const std::string & arg, bool reduced ) {
        if( is_new ) {
            return it.get_amount( arg, reduced );
        }
        return it.get_mod( arg, reduced );
    };

    // Handle miss messages
    const std::vector<std::pair<translation, int>> &msgs = it.get_miss_msgs();
    if( !msgs.empty() ) {
        for( const auto &i : msgs ) {
            add_miss_reason( i.first.translated(), static_cast<unsigned>( i.second ) );
        }
    }

    // Handle health mod
    val = get_effect( "H_MOD", reduced );
    if( val != 0 ) {
        mod = 1;
        if( is_new || it.activated( calendar::turn, "H_MOD", val, reduced, mod ) ) {
            int bounded = bound_mod_to_vals(
                              get_healthy_mod(), val, it.get_max_val( "H_MOD", reduced ),
                              it.get_min_val( "H_MOD", reduced ) );
            // This already applies bounds, so we pass them through.
            mod_healthy_mod( bounded, get_healthy_mod() + bounded );
        }
    }

    // Handle health
    val = get_effect( "HEALTH", reduced );
    if( val != 0 ) {
        mod = 1;
        if( is_new || it.activated( calendar::turn, "HEALTH", val, reduced, mod ) ) {
            mod_healthy( bound_mod_to_vals( get_healthy(), val,
                                            it.get_max_val( "HEALTH", reduced ), it.get_min_val( "HEALTH", reduced ) ) );
        }
    }

    // Handle stim
    val = get_effect( "STIM", reduced );
    if( val != 0 ) {
        mod = 1;
        if( is_new || it.activated( calendar::turn, "STIM", val, reduced, mod ) ) {
            mod_stim( bound_mod_to_vals( get_stim(), val, it.get_max_val( "STIM", reduced ),
                                         it.get_min_val( "STIM", reduced ) ) );
        }
    }

    // Handle hunger
    val = get_effect( "HUNGER", reduced );
    if( val != 0 ) {
        mod = 1;
        if( is_new || it.activated( calendar::turn, "HUNGER", val, reduced, mod ) ) {
            mod_hunger( bound_mod_to_vals( get_hunger(), val, it.get_max_val( "HUNGER", reduced ),
                                           it.get_min_val( "HUNGER", reduced ) ) );
        }
    }

    // Handle thirst
    val = get_effect( "THIRST", reduced );
    if( val != 0 ) {
        mod = 1;
        if( is_new || it.activated( calendar::turn, "THIRST", val, reduced, mod ) ) {
            mod_thirst( bound_mod_to_vals( get_thirst(), val, it.get_max_val( "THIRST", reduced ),
                                           it.get_min_val( "THIRST", reduced ) ) );
        }
    }

    // Handle fatigue
    val = get_effect( "FATIGUE", reduced );
    // Prevent ongoing fatigue effects while asleep.
    // These are meant to change how fast you get tired, not how long you sleep.
    if( val != 0 && !in_sleep_state() ) {
        mod = 1;
        if( is_new || it.activated( calendar::turn, "FATIGUE", val, reduced, mod ) ) {
            mod_fatigue( bound_mod_to_vals( get_fatigue(), val, it.get_max_val( "FATIGUE", reduced ),
                                            it.get_min_val( "FATIGUE", reduced ) ) );
        }
    }

    // Handle Radiation
    val = get_effect( "RAD", reduced );
    if( val != 0 ) {
        mod = 1;
        if( is_new || it.activated( calendar::turn, "RAD", val, reduced, mod ) ) {
            mod_rad( bound_mod_to_vals( get_rad(), val, it.get_max_val( "RAD", reduced ), 0 ) );
            // Radiation can't go negative
            if( get_rad() < 0 ) {
                set_rad( 0 );
            }
        }
    }

    // Handle Pain
    val = get_effect( "PAIN", reduced );
    if( val != 0 ) {
        mod = 1;
        if( it.get_sizing( "PAIN" ) ) {
            if( has_trait( trait_FAT ) ) {
                mod *= 1.5;
            }
            if( get_size() == creature_size::large ) {
                mod *= 2;
            }
            if( get_size() == creature_size::huge ) {
                mod *= 3;
            }
        }
        if( is_new || it.activated( calendar::turn, "PAIN", val, reduced, mod ) ) {
            int pain_inc = bound_mod_to_vals( get_pain(), val, it.get_max_val( "PAIN", reduced ), 0 );
            mod_pain( pain_inc );
            if( pain_inc > 0 ) {
                add_pain_msg( val, bp );
            }
        }
    }

    // Handle Damage
    val = get_effect( "HURT", reduced );
    if( val != 0 ) {
        mod = 1;
        if( it.get_sizing( "HURT" ) ) {
            if( has_trait( trait_FAT ) ) {
                mod *= 1.5;
            }
            if( get_size() == creature_size::large ) {
                mod *= 2;
            }
            if( get_size() == creature_size::huge ) {
                mod *= 3;
            }
        }
        if( is_new || it.activated( calendar::turn, "HURT", val, reduced, mod ) ) {
            if( bp == bodypart_str_id::NULL_ID() ) {
                if( val > 5 ) {
                    add_msg_if_player( _( "Your %s HURTS!" ), body_part_name_accusative( body_part_torso ) );
                } else {
                    add_msg_if_player( _( "Your %s hurts!" ), body_part_name_accusative( body_part_torso ) );
                }
                apply_damage( nullptr, body_part_torso, val, true );
            } else {
                if( val > 5 ) {
                    add_msg_if_player( _( "Your %s HURTS!" ), body_part_name_accusative( bp ) );
                } else {
                    add_msg_if_player( _( "Your %s hurts!" ), body_part_name_accusative( bp ) );
                }
                apply_damage( nullptr, bp, val, true );
            }
        }
    }

    // Handle Sleep
    val = get_effect( "SLEEP", reduced );
    if( val != 0 ) {
        mod = 1;
        if( ( is_new || it.activated( calendar::turn, "SLEEP", val, reduced, mod ) ) &&
            !has_effect( efftype_id( "sleep" ) ) ) {
            add_msg_if_player( _( "You pass out!" ) );
            fall_asleep( time_duration::from_turns( val ) );
        }
    }

    // Handle painkillers
    val = get_effect( "PKILL", reduced );
    if( val != 0 ) {
        mod = it.get_addict_mod( "PKILL", addiction_level( add_type::PKILLER ) );
        if( is_new || it.activated( calendar::turn, "PKILL", val, reduced, mod ) ) {
            mod_painkiller( bound_mod_to_vals( get_painkiller(), val, it.get_max_val( "PKILL", reduced ), 0 ) );
        }
    }

    // Handle coughing
    mod = 1;
    val = 0;
    if( it.activated( calendar::turn, "COUGH", val, reduced, mod ) ) {
        cough( it.get_harmful_cough() );
    }

    // Handle vomiting
    mod = vomit_mod();
    val = 0;
    if( it.activated( calendar::turn, "VOMIT", val, reduced, mod ) ) {
        vomit();
    }

    // Handle stamina
    val = get_effect( "STAMINA", reduced );
    if( val != 0 ) {
        mod = 1;
        if( is_new || it.activated( calendar::turn, "STAMINA", val, reduced, mod ) ) {
            mod_stamina( bound_mod_to_vals( get_stamina(), val,
                                            it.get_max_val( "STAMINA", reduced ),
                                            it.get_min_val( "STAMINA", reduced ) ) );
        }
    }

    // Speed and stats are handled in recalc_speed_bonus and reset_stats respectively
}

void Character::process_effects()
{
    //Special Removals
    if( has_effect( effect_darkness ) && g->is_in_sunlight( pos() ) ) {
        remove_effect( effect_darkness );
    }
    if( has_trait( trait_M_IMMUNE ) && has_effect( effect_fungus ) ) {
        vomit();
        remove_effect( effect_fungus );
        add_msg_if_player( m_bad, _( "We have mistakenly colonized a local guide!  Purging now." ) );
    }
    if( has_trait( trait_PARAIMMUNE ) && ( has_effect( effect_dermatik ) ||
                                           has_effect( effect_tapeworm ) ||
                                           has_effect( effect_bloodworms ) || has_effect( effect_brainworms ) ||
                                           has_effect( effect_paincysts ) ) ) {
        remove_effect( effect_dermatik );
        remove_effect( effect_tapeworm );
        remove_effect( effect_bloodworms );
        remove_effect( effect_brainworms );
        remove_effect( effect_paincysts );
        add_msg_if_player( m_good, _( "Something writhes inside of you as it dies." ) );
    }
    if( has_trait( trait_ACIDBLOOD ) && ( has_effect( effect_dermatik ) ||
                                          has_effect( effect_bloodworms ) ||
                                          has_effect( effect_brainworms ) ) ) {
        remove_effect( effect_dermatik );
        remove_effect( effect_bloodworms );
        remove_effect( effect_brainworms );
    }
    if( has_trait( trait_EATHEALTH ) && has_effect( effect_tapeworm ) ) {
        remove_effect( effect_tapeworm );
        add_msg_if_player( m_good, _( "Your bowels gurgle as something inside them dies." ) );
    }
    if( has_trait( trait_INFIMMUNE ) && ( has_effect( effect_bite ) || has_effect( effect_infected ) ||
                                          has_effect( effect_recover ) ) ) {
        remove_effect( effect_bite );
        remove_effect( effect_infected );
        remove_effect( effect_recover );
    }

    //Human only effects
    for( auto &elem : *effects ) {
        for( auto &_effect_it : elem.second ) {
            process_one_effect( _effect_it.second, false );
        }
    }

    Creature::process_effects();
}

double Character::vomit_mod()
{
    double mod = mutation_value( "vomit_multiplier" );
    if( has_effect( effect_weed_high ) ) {
        mod *= .1;
    }
    // If you're already nauseous, any food in your stomach greatly
    // increases chance of vomiting. Liquids don't provoke vomiting, though.
    if( stomach.contains() != 0_ml && has_effect( effect_nausea ) ) {
        mod *= 5 * get_effect_int( effect_nausea );
    }
    return mod;
}

int Character::sleep_spot( const tripoint &p ) const
{
    const int current_stim = get_stim();
    const comfort_response_t comfort_info = base_comfort_value( p );
    if( comfort_info.aid != nullptr ) {
        add_msg_if_player( m_info, _( "You use your %s for comfort." ), comfort_info.aid->tname() );
    }

    int sleepy = static_cast<int>( comfort_info.level );
    bool watersleep = has_trait( trait_WATERSLEEP );

    if( has_addiction( add_type::SLEEP ) ) {
        sleepy -= 4;
    }
    if( has_trait( trait_INSOMNIA ) ) {
        // 12.5 points is the difference between "tired" and "dead tired"
        sleepy -= 12;
    }
    if( has_trait( trait_EASYSLEEPER ) ) {
        // Low fatigue (being rested) has a much stronger effect than high fatigue
        // so it's OK for the value to be that much higher
        sleepy += 24;
    }
    if( has_active_bionic( bio_soporific ) ) {
        sleepy += 30;
    }
    if( has_trait( trait_EASYSLEEPER2 ) ) {
        // Mousefolk can sleep just about anywhere.
        sleepy += 40;
    }
    if( watersleep && get_map().has_flag_ter( "SWIMMABLE", pos() ) ) {
        sleepy += 10; //comfy water!
    }

    if( get_fatigue() < fatigue_levels::TIRED + 1 ) {
        sleepy -= static_cast<int>( ( fatigue_levels::TIRED + 1 - get_fatigue() ) / 4 );
    } else {
        sleepy += static_cast<int>( ( get_fatigue() - fatigue_levels::TIRED + 1 ) / 16 );
    }

    if( current_stim > 0 || !has_trait( trait_INSOMNIA ) ) {
        sleepy -= 2 * current_stim;
    } else {
        // Make it harder for insomniac to get around the trait
        sleepy -= current_stim;
    }

    return sleepy;
}

bool Character::can_sleep()
{
    if( has_effect( effect_meth ) ) {
        // Sleep ain't happening until that meth wears off completely.
        return false;
    }

    // Since there's a bit of randomness to falling asleep, we want to
    // prevent exploiting this if can_sleep() gets called over and over.
    // Only actually check if we can fall asleep no more frequently than
    // every 30 minutes.  We're assuming that if we return true, we'll
    // immediately be falling asleep after that.
    //
    // Also if player debug menu'd time backwards this breaks, just do the
    // check anyway, this will reset the timer if 'dur' is negative.
    const time_point now = calendar::turn;
    const time_duration dur = now - last_sleep_check;
    if( dur >= 0_turns && dur < 30_minutes ) {
        return false;
    }
    last_sleep_check = now;

    int sleepy = sleep_spot( pos() );
    sleepy += rng( -8, 8 );
    bool result = sleepy > 0;

    if( has_active_bionic( bio_soporific ) ) {
        if( bio_soporific_powered_at_last_sleep_check && !has_power() ) {
            add_msg_if_player( m_bad, _( "Your soporific inducer runs out of power!" ) );
        } else if( !bio_soporific_powered_at_last_sleep_check && has_power() ) {
            add_msg_if_player( m_good, _( "Your soporific inducer starts back up." ) );
        }
        bio_soporific_powered_at_last_sleep_check = has_power();
    }

    return result;
}

void Character::shift_destination( const point &shift )
{
    if( next_expected_position ) {
        *next_expected_position += shift;
    }

    for( auto &elem : auto_move_route ) {
        elem += shift;
    }
}

bool Character::has_weapon() const
{
    return !unarmed_attack();
}

int Character::get_lowest_hp() const
{
    // Set lowest_hp to an arbitrarily large number.
    int lowest_hp = 999;
    for( const std::pair<const bodypart_str_id, bodypart> &elem : get_body() ) {
        const int cur_hp = elem.second.get_hp_cur();
        if( cur_hp < lowest_hp ) {
            lowest_hp = cur_hp;
        }
    }
    return lowest_hp;
}

Creature::Attitude Character::attitude_to( const Creature &other ) const
{
    const monster *m = dynamic_cast<const monster *>( &other );
    if( m != nullptr ) {
        if( m->friendly != 0 ) {
            return Attitude::FRIENDLY;
        }
        switch( m->attitude( const_cast<Character *>( this ) ) ) {
            // player probably does not want to harm them, but doesn't care much at all.
            case MATT_FOLLOW:
            case MATT_FPASSIVE:
            case MATT_IGNORE:
            case MATT_FLEE:
                return Attitude::NEUTRAL;
            // player does not want to harm those.
            case MATT_FRIEND:
                return Attitude::FRIENDLY;
            case MATT_ATTACK:
                return Attitude::HOSTILE;
            case MATT_NULL:
            case NUM_MONSTER_ATTITUDES:
                break;
        }

        return Attitude::NEUTRAL;
    }

    const npc *p = dynamic_cast<const npc *>( &other );
    if( p != nullptr ) {
        if( p->is_enemy() ) {
            return Attitude::HOSTILE;
        } else if( p->is_player_ally() ) {
            return Attitude::FRIENDLY;
        } else {
            return Attitude::NEUTRAL;
        }
    } else if( &other == this ) {
        return Attitude::FRIENDLY;
    }

    return Attitude::NEUTRAL;
}

npc_attitude Character::get_attitude() const
{
    return NPCATT_NULL;
}

bool Character::sees( const tripoint &t, bool, int ) const
{
    const int wanted_range = rl_dist( pos(), t );
    bool can_see = is_player() ? get_map().pl_sees( t, wanted_range ) :
                   Creature::sees( t );
    // Clairvoyance is now pretty cheap, so we can check it early
    if( wanted_range < MAX_CLAIRVOYANCE && wanted_range < clairvoyance() ) {
        return true;
    }

    if( can_see && wanted_range > unimpaired_range() ) {
        can_see = false;
    }

    return can_see;
}

bool Character::sees( const Creature &critter ) const
{
    // This handles only the player/npc specific stuff (monsters don't have traits or bionics).
    const int dist = rl_dist( pos(), critter.pos() );
    if( dist <= 3 && has_active_mutation( trait_ANTENNAE ) ) {
        return true;
    }
    if( dist < MAX_CLAIRVOYANCE && dist < clairvoyance() ) {
        return true;
    }
    if( field_fd_clairvoyant.is_valid() &&
        get_map().get_field( critter.pos(), field_fd_clairvoyant ) ) {
        return true;
    }
    return Creature::sees( critter );
}

nc_color Character::bodytemp_color( const bodypart_id &bp ) const
{
    nc_color color = c_light_gray; // default
    const int temp_conv = get_part_temp_conv( bp );
    if( bp == body_part_eyes ) {
        color = c_light_gray;    // Eyes don't count towards warmth
    } else if( temp_conv  > BODYTEMP_SCORCHING ) {
        color = c_red;
    } else if( temp_conv  > BODYTEMP_VERY_HOT ) {
        color = c_light_red;
    } else if( temp_conv  > BODYTEMP_HOT ) {
        color = c_yellow;
    } else if( temp_conv  > BODYTEMP_COLD ) {
        color = c_green;
    } else if( temp_conv  > BODYTEMP_VERY_COLD ) {
        color = c_light_blue;
    } else if( temp_conv  > BODYTEMP_FREEZING ) {
        color = c_cyan;
    } else {
        color = c_blue;
    }
    return color;
}

void Character::set_destination( const std::vector<tripoint> &route,
                                 const player_activity &new_destination_activity )
{
    auto_move_route = route;
    set_destination_activity( new_destination_activity );
    destination_point.emplace( get_map().getabs( route.back() ) );
}

void Character::clear_destination()
{
    auto_move_route.clear();
    clear_destination_activity();
    destination_point = cata::nullopt;
    next_expected_position = cata::nullopt;
}

bool Character::has_distant_destination() const
{
    return has_destination() && !get_destination_activity().is_null() &&
           get_destination_activity().id() == ACT_TRAVELLING && !omt_path.empty();
}

bool Character::is_auto_moving() const
{
    return destination_point.has_value();
}

bool Character::has_destination() const
{
    return !auto_move_route.empty();
}

bool Character::has_destination_activity() const
{
    return !get_destination_activity().is_null() && destination_point &&
           position == get_map().getlocal( *destination_point );
}

void Character::start_destination_activity()
{
    if( !has_destination_activity() ) {
        debugmsg( "Tried to start invalid destination activity" );
        return;
    }

    assign_activity( get_destination_activity() );
    clear_destination();
}

std::vector<tripoint> &Character::get_auto_move_route()
{
    return auto_move_route;
}

action_id Character::get_next_auto_move_direction()
{
    if( !has_destination() ) {
        return ACTION_NULL;
    }

    if( next_expected_position ) {
        if( pos() != *next_expected_position ) {
            // We're off course, possibly stumbling or stuck, cancel auto move
            return ACTION_NULL;
        }
    }

    next_expected_position.emplace( auto_move_route.front() );
    auto_move_route.erase( auto_move_route.begin() );

    tripoint dp = *next_expected_position - pos();

    // Make sure the direction is just one step and that
    // all diagonal moves have 0 z component
    if( std::abs( dp.x ) > 1 || std::abs( dp.y ) > 1 || std::abs( dp.z ) > 1 ||
        ( std::abs( dp.z ) != 0 && ( std::abs( dp.x ) != 0 || std::abs( dp.y ) != 0 ) ) ) {
        // Should never happen, but check just in case
        return ACTION_NULL;
    }
    return get_movement_action_from_delta( dp, iso_rotate::yes );
}

int Character::talk_skill() const
{
    /** @EFFECT_INT slightly increases talking skill */

    /** @EFFECT_PER slightly increases talking skill */

    /** @EFFECT_SPEECH increases talking skill */
    int ret = get_int() + get_per() + get_skill_level( skill_id( "speech" ) ) * 3;
    return ret;
}

int Character::intimidation() const
{
    /** @EFFECT_STR increases intimidation factor */
    int ret = get_str() * 2;
    if( weapon.is_gun() ) {
        ret += 10;
    }
    if( weapon.damage_melee( damage_type::BASH ) >= 12 ||
        weapon.damage_melee( damage_type::CUT ) >= 12 ||
        weapon.damage_melee( damage_type::STAB ) >= 12 ) {
        ret += 5;
    }

    if( get_stim() > 20 ) {
        ret += 2;
    }
    if( has_effect( effect_drunk ) ) {
        ret -= 4;
    }
    ret = enchantment_cache->modify_value( enchant_vals::mod::SOCIAL_INTIMIDATE, ret );
    return ret;
}

bool Character::has_proficiency( const proficiency_id &prof ) const
{
    return _proficiencies->has_learned( prof );
}

bool Character::has_prof_prereqs( const proficiency_id &prof ) const
{
    return _proficiencies->has_prereqs( prof );
}

void Character::add_proficiency( const proficiency_id &prof, bool ignore_requirements )
{
    if( ignore_requirements ) {
        _proficiencies->direct_learn( prof );
        return;
    }
    _proficiencies->learn( prof );
}

void Character::lose_proficiency( const proficiency_id &prof, bool ignore_requirements )
{
    if( ignore_requirements ) {
        _proficiencies->direct_remove( prof );
        return;
    }
    _proficiencies->remove( prof );
}

std::vector<display_proficiency> Character::display_proficiencies() const
{
    return _proficiencies->display();
}

void Character::practice_proficiency( const proficiency_id &prof, const time_duration &amount,
                                      const cata::optional<time_duration> &max )
{
    int amt = to_seconds<int>( amount );
    const float pct_before = _proficiencies->pct_practiced( prof );
    time_duration focused_amount = time_duration::from_seconds( adjust_for_focus( amt ) / 100 );
    const bool learned = _proficiencies->practice( prof, focused_amount, max );
    const float pct_after = _proficiencies->pct_practiced( prof );

    if( pct_after > pct_before ) {
        focus_pool -= focus_pool / 100;
    }

    if( learned ) {
        add_msg_if_player( m_good, _( "You are now proficient in %s!" ), prof->name() );
    }
}

time_duration Character::proficiency_training_needed( const proficiency_id &prof ) const
{
    return _proficiencies->training_time_needed( prof );
}

std::vector<proficiency_id> Character::known_proficiencies() const
{
    return _proficiencies->known_profs();
}

std::vector<proficiency_id> Character::learning_proficiencies() const
{
    return _proficiencies->learning_profs();
}

bool Character::defer_move( const tripoint &next )
{
    // next must be adjacent to current pos
    if( square_dist( next, pos() ) != 1 ) {
        return false;
    }
    // next must be adjacent to subsequent move in any preexisting automove route
    if( has_destination() && square_dist( auto_move_route.front(), next ) != 1 ) {
        return false;
    }
    auto_move_route.insert( auto_move_route.begin(), next );
    next_expected_position = pos();
    return true;
}

bool Character::add_or_drop_with_msg( item &it, const bool /*unloading*/, const item *avoid )
{
    if( it.made_of( phase_id::LIQUID ) ) {
        liquid_handler::consume_liquid( it, 1, avoid );
        return it.charges <= 0;
    }
    if( !this->can_pickVolume( it ) ) {
        put_into_vehicle_or_drop( *this, item_drop_reason::too_large, { it } );
    } else if( !this->can_pickWeight( it, !get_option<bool>( "DANGEROUS_PICKUPS" ) ) ) {
        put_into_vehicle_or_drop( *this, item_drop_reason::too_heavy, { it } );
    } else {
        const bool allow_wield = !weapon.has_item( it ) && weapon.magazine_current() != &it;
        const int prev_charges = it.charges;
        auto &ni = this->i_add( it, true, avoid, /*allow_drop=*/false, /*allow_wield=*/allow_wield );
        if( ni.is_null() ) {
            // failed to add
            put_into_vehicle_or_drop( *this, item_drop_reason::tumbling, { it } );
        } else if( &ni == &it ) {
            // merged into the original stack, restore original charges
            it.charges = prev_charges;
            put_into_vehicle_or_drop( *this, item_drop_reason::tumbling, { it } );
        } else {
            // successfully added
            add_msg( _( "You put the %s in your inventory." ), ni.tname() );
            add_msg( m_info, "%c - %s", ni.invlet == 0 ? ' ' : ni.invlet, ni.tname() );
        }
    }
    return true;
}

bool Character::unload( item_location &loc, bool bypass_activity )
{
    item &it = *loc;
    // Unload a container consuming moves per item successfully removed
    if( it.is_container() ) {
        if( it.contents.empty() ) {
            add_msg( m_info, _( "The %s is already empty!" ), it.tname() );
            return false;
        }
        if( !it.can_unload_liquid() ) {
            add_msg( m_info, _( "The liquid can't be unloaded in its current state!" ) );
            return false;
        }

        int moves = 0;
        for( item *contained : it.contents.all_items_top() ) {
            moves += this->item_handling_cost( *contained );
        }
        assign_activity( player_activity( unload_activity_actor( moves, loc ) ) );

        return true;
    }

    // If item can be unloaded more than once (currently only guns) prompt user to choose
    std::vector<std::string> msgs( 1, it.tname() );
    std::vector<item *> opts( 1, &it );

    for( item *e : it.gunmods() ) {
        if( ( e->is_gun() && !e->has_flag( flag_NO_UNLOAD ) &&
              ( e->magazine_current() || e->ammo_remaining() > 0 || e->casings_count() > 0 ) ) ||
            ( e->has_flag( flag_BRASS_CATCHER ) && !e->is_container_empty() ) ) {
            msgs.emplace_back( e->tname() );
            opts.emplace_back( e );
        }
    }

    item *target = nullptr;
    item_location targloc;
    if( opts.size() > 1 ) {
        const int ret = uilist( _( "Unload what?" ), msgs );
        if( ret >= 0 ) {
            target = opts[ret];
            targloc = item_location( loc, opts[ret] );
        }
    } else {
        target = &it;
        targloc = loc;
    }

    if( target == nullptr ) {
        return false;
    }

    // Next check for any reasons why the item cannot be unloaded
    if( !target->has_flag( flag_BRASS_CATCHER ) ) {
        if( target->ammo_types().empty() && target->magazine_compatible().empty() ) {
            add_msg( m_info, _( "You can't unload a %s!" ), target->tname() );
            return false;
        }

        if( target->has_flag( flag_NO_UNLOAD ) ) {
            if( target->has_flag( flag_RECHARGE ) || target->has_flag( flag_USE_UPS ) ) {
                add_msg( m_info, _( "You can't unload a rechargeable %s!" ), target->tname() );
            } else {
                add_msg( m_info, _( "You can't unload a %s!" ), target->tname() );
            }
            return false;
        }

        if( !target->magazine_current() && target->ammo_remaining() <= 0 && target->casings_count() <= 0 ) {
            if( target->is_tool() ) {
                add_msg( m_info, _( "Your %s isn't charged." ), target->tname() );
            } else {
                add_msg( m_info, _( "Your %s isn't loaded." ), target->tname() );
            }
            return false;
        }
    }

    target->casings_handle( [&]( item & e ) {
        return this->i_add_or_drop( e );
    } );

    if( target->is_magazine() ) {
        if( bypass_activity ) {
            unload_activity_actor::unload( *this, targloc );
        } else {
            int mv = 0;
            for( const item *content : target->contents.all_items_top() ) {
                // We use the same cost for both reloading and unloading
                mv += this->item_reload_cost( it, *content, content->charges );
            }
            if( it.is_ammo_belt() ) {
                // Disassembling ammo belts is easier than assembling them
                mv /= 2;
            }
            assign_activity( player_activity( unload_activity_actor( mv, targloc ) ) );
        }
        return true;

    } else if( target->magazine_current() ) {
        if( !this->add_or_drop_with_msg( *target->magazine_current(), true ) ) {
            return false;
        }
        // Eject magazine consuming half as much time as required to insert it
        this->moves -= this->item_reload_cost( *target, *target->magazine_current(), -1 ) / 2;

        target->remove_items_with( [&target]( const item & e ) {
            return target->magazine_current() == &e;
        } );

    } else if( target->ammo_remaining() ) {
        int qty = target->ammo_remaining();

        // Construct a new ammo item and try to drop it
        item ammo( target->ammo_current(), calendar::turn, qty );
        if( target->is_filthy() ) {
            ammo.set_flag( flag_FILTHY );
        }

        if( ammo.made_of_from_type( phase_id::LIQUID ) ) {
            if( !this->add_or_drop_with_msg( ammo ) ) {
                qty -= ammo.charges; // only handled part (or none) of the liquid
            }
            if( qty <= 0 ) {
                return false; // no liquid was moved
            }

        } else if( !this->add_or_drop_with_msg( ammo, qty > 1 ) ) {
            return false;
        }

        // If successful remove appropriate qty of ammo consuming half as much time as required to load it
        this->moves -= this->item_reload_cost( *target, ammo, qty ) / 2;

        target->ammo_set( target->ammo_current(), target->ammo_remaining() - qty );
    } else if( target->has_flag( flag_BRASS_CATCHER ) ) {
        target->spill_contents( get_player_character() );
    }

    // Turn off any active tools
    if( target->is_tool() && target->active && target->ammo_remaining() == 0 ) {
        target->type->invoke( *this->as_player(), *target, this->pos() );
    }

    add_msg( _( "You unload your %s." ), target->tname() );

    if( it.has_flag( flag_MAG_DESTROY ) && it.ammo_remaining() == 0 ) {
        loc.remove_item();
    }

    return true;
}

int Character::item_reload_cost( const item &it, const item &ammo, int qty ) const
{
    if( ammo.is_ammo() || ammo.is_frozen_liquid() || ammo.made_of_from_type( phase_id::LIQUID ) ) {
        qty = std::max( std::min( ammo.charges, qty ), 1 );
    } else if( ammo.is_ammo_container() ) {
        int min_clamp = 0;
        // find the first ammo in the container to get its charges
        ammo.visit_items( [&min_clamp]( const item * it, item * ) {
            if( it->is_ammo() ) {
                min_clamp = it->charges;
                return VisitResponse::ABORT;
            }
            return VisitResponse::NEXT;
        } );

        qty = clamp( qty, min_clamp, 1 );
    } else if( ammo.is_magazine() ) {
        qty = 1;
    } else {
        debugmsg( "cannot determine reload cost as %s is neither ammo or magazine", ammo.tname() );
        return 0;
    }

    // If necessary create duplicate with appropriate number of charges
    item obj = ammo;
    obj = obj.split( qty );
    if( obj.is_null() ) {
        obj = ammo;
    }
    // No base cost for handling ammo - that's already included in obtain cost
    // We have the ammo in our hands right now
    int mv = item_handling_cost( obj, true, 0 );

    if( ammo.has_flag( flag_MAG_BULKY ) ) {
        mv *= 1.5; // bulky magazines take longer to insert
    }

    if( !it.is_gun() && !it.is_magazine() ) {
        return mv + 100; // reload a tool or sealable container
    }

    /** @EFFECT_GUN decreases the time taken to reload a magazine */
    /** @EFFECT_PISTOL decreases time taken to reload a pistol */
    /** @EFFECT_SMG decreases time taken to reload an SMG */
    /** @EFFECT_RIFLE decreases time taken to reload a rifle */
    /** @EFFECT_SHOTGUN decreases time taken to reload a shotgun */
    /** @EFFECT_LAUNCHER decreases time taken to reload a launcher */

    int cost = 0;
    if( it.is_gun() ) {
        cost = it.get_reload_time();
    } else if( it.type->magazine ) {
        cost = it.type->magazine->reload_time * qty;
    } else {
        cost = it.contents.obtain_cost( ammo );
    }

    skill_id sk = it.is_gun() ? it.type->gun->skill_used : skill_gun;
    mv += cost / ( 1.0f + std::min( get_skill_level( sk ) * 0.1f, 1.0f ) );

    if( it.has_flag( flag_STR_RELOAD ) ) {
        /** @EFFECT_STR reduces reload time of some weapons */
        mv -= get_str() * 20;
    }

    return std::max( mv, 25 );
}

book_mastery Character::get_book_mastery( const item &book ) const
{
    if( !book.is_book() || !has_identified( book.typeId() ) ) {
        return book_mastery::CANT_DETERMINE;
    }
    // TODO: add illiterate check?

    const cata::value_ptr<islot_book> &type = book.type->book;
    const skill_id &skill = type->skill;

    if( !skill ) {
        // book gives no skill
        return book_mastery::MASTERED;
    }

    const int skill_level = get_skill_level( skill );
    const int skill_requirement = type->req;
    const int max_skill_learnable = type->level;

    if( skill_requirement > skill_level ) {
        return book_mastery::CANT_UNDERSTAND;
    }
    if( skill_level >= max_skill_learnable ) {
        return book_mastery::MASTERED;
    }
    return book_mastery::LEARNING;
}

static const itype_id itype_cookbook_human( "cookbook_human" );
static const trait_id trait_HATES_BOOKS( "HATES_BOOKS" );
static const trait_id trait_LOVES_BOOKS( "LOVES_BOOKS" );
static const trait_id trait_CANNIBAL( "CANNIBAL" );
static const trait_id trait_PSYCHOPATH( "PSYCHOPATH" );
static const trait_id trait_SAPIOVORE( "SAPIOVORE" );
static const trait_id trait_SPIRITUAL( "SPIRITUAL" );

bool Character::fun_to_read( const item &book ) const
{
    return book_fun_for( book, *this ) > 0;
}

int Character::book_fun_for( const item &book, const Character &p ) const
{
    int fun_bonus = book.type->book->fun;
    if( !book.is_book() ) {
        debugmsg( "called avatar::book_fun_for with non-book" );
        return 0;
    }

    // If you don't have a problem with eating humans, To Serve Man becomes rewarding
    if( ( p.has_trait( trait_CANNIBAL ) || p.has_trait( trait_PSYCHOPATH ) ||
          p.has_trait( trait_SAPIOVORE ) ) &&
        book.typeId() == itype_cookbook_human ) {
        fun_bonus = std::abs( fun_bonus );
    } else if( p.has_trait( trait_SPIRITUAL ) && book.has_flag( flag_INSPIRATIONAL ) ) {
        fun_bonus = std::abs( fun_bonus * 3 );
    }

    if( has_trait( trait_LOVES_BOOKS ) ) {
        fun_bonus++;
    } else if( has_trait( trait_HATES_BOOKS ) ) {
        if( book.type->book->fun > 0 ) {
            fun_bonus = 0;
        } else {
            fun_bonus--;
        }
    }

    if( fun_bonus > 1 && book.get_chapters() > 0 && book.get_remaining_chapters( p ) == 0 ) {
        fun_bonus /= 2;
    }

    return fun_bonus;
}

bool Character::has_bionic_with_flag( const json_character_flag &flag ) const
{
    for( const bionic &bio : *my_bionics ) {
        if( bio.info().has_flag( flag ) ) {
            return true;
        }
        if( bio.info().activated ) {
            if( ( bio.info().has_active_flag( flag ) && has_active_bionic( bio.id ) ) ||
                ( bio.info().has_inactive_flag( flag ) && !has_active_bionic( bio.id ) ) ) {
                return true;
            }
        }
    }

    return false;
}

bool Character::has_flag( const json_character_flag &flag ) const
{
    // If this is a performance problem create a map of flags stored for a character and updated on trait, mutation, bionic add/remove, activate/deactivate
    return has_trait_flag( flag ) || has_bionic_with_flag( flag );
}
