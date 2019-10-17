#include "player.h"

#include <cctype>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <map>
#include <string>
#include <sstream>
#include <limits>
#include <bitset>
#include <exception>
#include <tuple>

#include "action.h"
#include "activity_handlers.h"
#include "addiction.h"
#include "ammo.h"
#include "avatar.h"
#include "avatar_action.h"
#include "bionics.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "coordinate_conversions.h"
#include "craft_command.h"
#include "creature_tracker.h"
#include "cursesdef.h"
#include "debug.h"
#include "effect.h"
#include "event_bus.h"
#include "fault.h"
#include "filesystem.h"
#include "fungal_effects.h"
#include "game.h"
#include "gun_mode.h"
#include "handle_liquid.h"
#include "input.h"
#include "inventory.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "iuse_actor.h"
#include "magic.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "martialarts.h"
#include "material.h"
#include "memorial_logger.h"
#include "messages.h"
#include "monster.h"
#include "morale.h"
#include "morale_types.h"
#include "mtype.h"
#include "mutation.h"
#include "name.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "overmapbuffer.h"
#include "pickup.h"
#include "profession.h"
#include "ranged.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "skill.h"
#include "sounds.h"
#include "string_formatter.h"
#include "submap.h"
#include "text_snippets.h"
#include "translations.h"
#include "trap.h"
#include "ui.h"
#include "uistate.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vitamin.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "weather.h"
#include "weather_gen.h"
#include "field.h"
#include "fire.h"
#include "int_id.h"
#include "iuse.h"
#include "lightmap.h"
#include "line.h"
#include "math_defines.h"
#include "omdata.h"
#include "overmap_types.h"
#include "recipe.h"
#include "rng.h"
#include "units.h"
#include "visitable.h"
#include "string_id.h"
#include "colony.h"
#include "enums.h"
#include "flat_set.h"
#include "stomach.h"
#include "teleport.h"

const double MAX_RECOIL = 3000;

const mtype_id mon_player_blob( "mon_player_blob" );
const mtype_id mon_shadow_snake( "mon_shadow_snake" );

const skill_id skill_dodge( "dodge" );
const skill_id skill_gun( "gun" );
const skill_id skill_mechanics( "mechanics" );
const skill_id skill_swimming( "swimming" );
const skill_id skill_throw( "throw" );
const skill_id skill_unarmed( "unarmed" );

const efftype_id effect_adrenaline( "adrenaline" );
const efftype_id effect_alarm_clock( "alarm_clock" );
const efftype_id effect_asthma( "asthma" );
const efftype_id effect_attention( "attention" );
const efftype_id effect_bandaged( "bandaged" );
const efftype_id effect_bite( "bite" );
const efftype_id effect_blind( "blind" );
const efftype_id effect_blisters( "blisters" );
const efftype_id effect_bloodworms( "bloodworms" );
const efftype_id effect_boomered( "boomered" );
const efftype_id effect_brainworms( "brainworms" );
const efftype_id effect_cig( "cig" );
const efftype_id effect_cold( "cold" );
const efftype_id effect_common_cold( "common_cold" );
const efftype_id effect_contacts( "contacts" );
const efftype_id effect_corroding( "corroding" );
const efftype_id effect_cough_suppress( "cough_suppress" );
const efftype_id effect_darkness( "darkness" );
const efftype_id effect_datura( "datura" );
const efftype_id effect_deaf( "deaf" );
const efftype_id effect_depressants( "depressants" );
const efftype_id effect_dermatik( "dermatik" );
const efftype_id effect_disabled( "disabled" );
const efftype_id effect_disinfected( "disinfected" );
const efftype_id effect_downed( "downed" );
const efftype_id effect_drunk( "drunk" );
const efftype_id effect_earphones( "earphones" );
const efftype_id effect_evil( "evil" );
const efftype_id effect_flu( "flu" );
const efftype_id effect_foodpoison( "foodpoison" );
const efftype_id effect_formication( "formication" );
const efftype_id effect_frostbite( "frostbite" );
const efftype_id effect_frostbite_recovery( "frostbite_recovery" );
const efftype_id effect_fungus( "fungus" );
const efftype_id effect_glowing( "glowing" );
const efftype_id effect_glowy_led( "glowy_led" );
const efftype_id effect_got_checked( "got_checked" );
const efftype_id effect_grabbed( "grabbed" );
const efftype_id effect_grabbing( "grabbing" );
const efftype_id effect_hallu( "hallu" );
const efftype_id effect_happy( "happy" );
const efftype_id effect_hot( "hot" );
const efftype_id effect_infected( "infected" );
const efftype_id effect_iodine( "iodine" );
const efftype_id effect_irradiated( "irradiated" );
const efftype_id effect_jetinjector( "jetinjector" );
const efftype_id effect_lack_sleep( "lack_sleep" );
const efftype_id effect_sleep_deprived( "sleep_deprived" );
const efftype_id effect_lying_down( "lying_down" );
const efftype_id effect_mending( "mending" );
const efftype_id effect_meth( "meth" );
const efftype_id effect_narcosis( "narcosis" );
const efftype_id effect_nausea( "nausea" );
const efftype_id effect_no_sight( "no_sight" );
const efftype_id effect_onfire( "onfire" );
const efftype_id effect_paincysts( "paincysts" );
const efftype_id effect_pkill( "pkill" );
const efftype_id effect_pkill1( "pkill1" );
const efftype_id effect_pkill2( "pkill2" );
const efftype_id effect_pkill3( "pkill3" );
const efftype_id effect_recover( "recover" );
const efftype_id effect_riding( "riding" );
const efftype_id effect_sad( "sad" );
const efftype_id effect_shakes( "shakes" );
const efftype_id effect_sleep( "sleep" );
const efftype_id effect_slept_through_alarm( "slept_through_alarm" );
const efftype_id effect_spores( "spores" );
const efftype_id effect_stim( "stim" );
const efftype_id effect_stim_overdose( "stim_overdose" );
const efftype_id effect_stunned( "stunned" );
const efftype_id effect_tapeworm( "tapeworm" );
const efftype_id effect_took_prozac( "took_prozac" );
const efftype_id effect_took_thorazine( "took_thorazine" );
const efftype_id effect_took_xanax( "took_xanax" );
const efftype_id effect_valium( "valium" );
const efftype_id effect_visuals( "visuals" );
const efftype_id effect_weed_high( "weed_high" );
const efftype_id effect_winded( "winded" );
const efftype_id effect_bleed( "bleed" );
const efftype_id effect_magnesium_supplements( "magnesium" );
const efftype_id effect_pet( "pet" );

const matype_id style_none( "style_none" );
const matype_id style_kicks( "style_kicks" );

const species_id ROBOT( "ROBOT" );

static const bionic_id bio_ads( "bio_ads" );
static const bionic_id bio_advreactor( "bio_advreactor" );
static const bionic_id bio_armor_arms( "bio_armor_arms" );
static const bionic_id bio_armor_eyes( "bio_armor_eyes" );
static const bionic_id bio_armor_head( "bio_armor_head" );
static const bionic_id bio_armor_legs( "bio_armor_legs" );
static const bionic_id bio_armor_torso( "bio_armor_torso" );
static const bionic_id bio_blaster( "bio_blaster" );
static const bionic_id bio_carbon( "bio_carbon" );
static const bionic_id bio_climate( "bio_climate" );
static const bionic_id bio_cloak( "bio_cloak" );
static const bionic_id bio_cqb( "bio_cqb" );
static const bionic_id bio_dis_acid( "bio_dis_acid" );
static const bionic_id bio_dis_shock( "bio_dis_shock" );
static const bionic_id bio_drain( "bio_drain" );
static const bionic_id bio_earplugs( "bio_earplugs" );
static const bionic_id bio_ears( "bio_ears" );
static const bionic_id bio_eye_optic( "bio_eye_optic" );
static const bionic_id bio_faraday( "bio_faraday" );
static const bionic_id bio_flashlight( "bio_flashlight" );
static const bionic_id bio_tattoo_led( "bio_tattoo_led" );
static const bionic_id bio_glowy( "bio_glowy" );
static const bionic_id bio_geiger( "bio_geiger" );
static const bionic_id bio_gills( "bio_gills" );
static const bionic_id bio_ground_sonar( "bio_ground_sonar" );
static const bionic_id bio_heatsink( "bio_heatsink" );
static const bionic_id bio_itchy( "bio_itchy" );
static const bionic_id bio_jointservo( "bio_jointservo" );
static const bionic_id bio_laser( "bio_laser" );
static const bionic_id bio_leaky( "bio_leaky" );
static const bionic_id bio_lighter( "bio_lighter" );
static const bionic_id bio_membrane( "bio_membrane" );
static const bionic_id bio_memory( "bio_memory" );
static const bionic_id bio_metabolics( "bio_metabolics" );
static const bionic_id bio_noise( "bio_noise" );
static const bionic_id bio_plut_filter( "bio_plut_filter" );
static const bionic_id bio_power_weakness( "bio_power_weakness" );
static const bionic_id bio_reactor( "bio_reactor" );
static const bionic_id bio_recycler( "bio_recycler" );
static const bionic_id bio_shakes( "bio_shakes" );
static const bionic_id bio_sleepy( "bio_sleepy" );
static const bionic_id bn_bio_solar( "bn_bio_solar" );
static const bionic_id bio_soporific( "bio_soporific" );
static const bionic_id bio_spasm( "bio_spasm" );
static const bionic_id bio_speed( "bio_speed" );
static const bionic_id bio_syringe( "bio_syringe" );
static const bionic_id bio_tools( "bio_tools" );
static const bionic_id bio_trip( "bio_trip" );
static const bionic_id bio_uncanny_dodge( "bio_uncanny_dodge" );
static const bionic_id bio_ups( "bio_ups" );
static const bionic_id bio_watch( "bio_watch" );
static const bionic_id bio_synaptic_regen( "bio_synaptic_regen" );

// Aftershock stuff!
static const bionic_id afs_bio_linguistic_coprocessor( "afs_bio_linguistic_coprocessor" );

static const trait_id trait_ACIDBLOOD( "ACIDBLOOD" );
static const trait_id trait_ACIDPROOF( "ACIDPROOF" );
static const trait_id trait_ADDICTIVE( "ADDICTIVE" );
static const trait_id trait_ADRENALINE( "ADRENALINE" );
static const trait_id trait_ALBINO( "ALBINO" );
static const trait_id trait_AMPHIBIAN( "AMPHIBIAN" );
static const trait_id trait_ANTENNAE( "ANTENNAE" );
static const trait_id trait_ANTLERS( "ANTLERS" );
static const trait_id trait_ASTHMA( "ASTHMA" );
static const trait_id trait_BADBACK( "BADBACK" );
static const trait_id trait_BARK( "BARK" );
static const trait_id trait_CANNIBAL( "CANNIBAL" );
static const trait_id trait_CENOBITE( "CENOBITE" );
static const trait_id trait_CEPH_EYES( "CEPH_EYES" );
static const trait_id trait_CF_HAIR( "CF_HAIR" );
static const trait_id trait_CHAOTIC( "CHAOTIC" );
static const trait_id trait_CHAOTIC_BAD( "CHAOTIC_BAD" );
static const trait_id trait_CHEMIMBALANCE( "CHEMIMBALANCE" );
static const trait_id trait_CHITIN_FUR( "CHITIN_FUR" );
static const trait_id trait_CHITIN_FUR2( "CHITIN_FUR2" );
static const trait_id trait_CHITIN_FUR3( "CHITIN_FUR3" );
static const trait_id trait_CHLOROMORPH( "CHLOROMORPH" );
static const trait_id trait_CLUMSY( "CLUMSY" );
static const trait_id trait_COLDBLOOD4( "COLDBLOOD4" );
static const trait_id trait_DEAF( "DEAF" );
static const trait_id trait_DEFT( "DEFT" );
static const trait_id trait_DEBUG_BIONIC_POWER( "DEBUG_BIONIC_POWER" );
static const trait_id trait_DEBUG_CLOAK( "DEBUG_CLOAK" );
static const trait_id trait_DEBUG_HS( "DEBUG_HS" );
static const trait_id trait_DEBUG_LS( "DEBUG_LS" );
static const trait_id trait_DEBUG_NODMG( "DEBUG_NODMG" );
static const trait_id trait_DEBUG_NOTEMP( "DEBUG_NOTEMP" );
static const trait_id trait_DISIMMUNE( "DISIMMUNE" );
static const trait_id trait_DISRESISTANT( "DISRESISTANT" );
static const trait_id trait_DOWN( "DOWN" );
static const trait_id trait_EASYSLEEPER( "EASYSLEEPER" );
static const trait_id trait_EASYSLEEPER2( "EASYSLEEPER2" );
static const trait_id trait_ELECTRORECEPTORS( "ELECTRORECEPTORS" );
static const trait_id trait_EATHEALTH( "EATHEALTH" );
static const trait_id trait_FASTHEALER( "FASTHEALER" );
static const trait_id trait_FASTHEALER2( "FASTHEALER2" );
static const trait_id trait_FASTLEARNER( "FASTLEARNER" );
static const trait_id trait_FASTREADER( "FASTREADER" );
static const trait_id trait_FAT( "FAT" );
static const trait_id trait_FELINE_FUR( "FELINE_FUR" );
static const trait_id trait_FLOWERS( "FLOWERS" );
static const trait_id trait_FRESHWATEROSMOSIS( "FRESHWATEROSMOSIS" );
static const trait_id trait_FUR( "FUR" );
static const trait_id trait_GILLS( "GILLS" );
static const trait_id trait_GILLS_CEPH( "GILLS_CEPH" );
static const trait_id trait_HATES_BOOKS( "HATES_BOOKS" );
static const trait_id trait_HEAVYSLEEPER( "HEAVYSLEEPER" );
static const trait_id trait_HEAVYSLEEPER2( "HEAVYSLEEPER2" );
static const trait_id trait_HOARDER( "HOARDER" );
static const trait_id trait_HOLLOW_BONES( "HOLLOW_BONES" );
static const trait_id trait_HOOVES( "HOOVES" );
static const trait_id trait_HORNS_POINTED( "HORNS_POINTED" );
static const trait_id trait_HUGE( "HUGE" );
static const trait_id trait_HUGE_OK( "HUGE_OK" );
static const trait_id trait_INFIMMUNE( "INFIMMUNE" );
static const trait_id trait_INSOMNIA( "INSOMNIA" );
static const trait_id trait_INT_SLIME( "INT_SLIME" );
static const trait_id trait_JITTERY( "JITTERY" );
static const trait_id trait_LARGE( "LARGE" );
static const trait_id trait_LARGE_OK( "LARGE_OK" );
static const trait_id trait_LEAVES( "LEAVES" );
static const trait_id trait_LEAVES2( "LEAVES2" );
static const trait_id trait_LEAVES3( "LEAVES3" );
static const trait_id trait_LEG_TENTACLES( "LEG_TENTACLES" );
static const trait_id trait_LEG_TENT_BRACE( "LEG_TENT_BRACE" );
static const trait_id trait_LIGHTFUR( "LIGHTFUR" );
static const trait_id trait_LIGHTSTEP( "LIGHTSTEP" );
static const trait_id trait_LIGHT_BONES( "LIGHT_BONES" );
static const trait_id trait_LOVES_BOOKS( "LOVES_BOOKS" );
static const trait_id trait_LUPINE_FUR( "LUPINE_FUR" );
static const trait_id trait_MEMBRANE( "MEMBRANE" );
static const trait_id trait_MOODSWINGS( "MOODSWINGS" );
static const trait_id trait_MOREPAIN( "MORE_PAIN" );
static const trait_id trait_MOREPAIN2( "MORE_PAIN2" );
static const trait_id trait_MOREPAIN3( "MORE_PAIN3" );
static const trait_id trait_MYOPIC( "MYOPIC" );
static const trait_id trait_M_BLOSSOMS( "M_BLOSSOMS" );
static const trait_id trait_M_DEPENDENT( "M_DEPENDENT" );
static const trait_id trait_M_IMMUNE( "M_IMMUNE" );
static const trait_id trait_M_SKIN2( "M_SKIN2" );
static const trait_id trait_M_SKIN3( "M_SKIN3" );
static const trait_id trait_M_SPORES( "M_SPORES" );
static const trait_id trait_NARCOLEPTIC( "NARCOLEPTIC" );
static const trait_id trait_NAUSEA( "NAUSEA" );
static const trait_id trait_NOMAD( "NOMAD" );
static const trait_id trait_NOMAD2( "NOMAD2" );
static const trait_id trait_NOMAD3( "NOMAD3" );
static const trait_id trait_NONADDICTIVE( "NONADDICTIVE" );
static const trait_id trait_NOPAIN( "NOPAIN" );
static const trait_id trait_NO_THIRST( "NO_THIRST" );
static const trait_id trait_PACIFIST( "PACIFIST" );
static const trait_id trait_PADDED_FEET( "PADDED_FEET" );
static const trait_id trait_PAINREC1( "PAINREC1" );
static const trait_id trait_PAINREC2( "PAINREC2" );
static const trait_id trait_PAINREC3( "PAINREC3" );
static const trait_id trait_PAINRESIST( "PAINRESIST" );
static const trait_id trait_PAINRESIST_TROGLO( "PAINRESIST_TROGLO" );
static const trait_id trait_PARAIMMUNE( "PARAIMMUNE" );
static const trait_id trait_PARKOUR( "PARKOUR" );
static const trait_id trait_PAWS( "PAWS" );
static const trait_id trait_PAWS_LARGE( "PAWS_LARGE" );
static const trait_id trait_PER_SLIME( "PER_SLIME" );
static const trait_id trait_PER_SLIME_OK( "PER_SLIME_OK" );
static const trait_id trait_PRED2( "PRED2" );
static const trait_id trait_PRED3( "PRED3" );
static const trait_id trait_PRED4( "PRED4" );
static const trait_id trait_PROF_DICEMASTER( "PROF_DICEMASTER" );
static const trait_id trait_PROF_FOODP( "PROF_FOODP" );
static const trait_id trait_PROF_SKATER( "PROF_SKATER" );
static const trait_id trait_PSYCHOPATH( "PSYCHOPATH" );
static const trait_id trait_PYROMANIA( "PYROMANIA" );
static const trait_id trait_KILLER( "KILLER" );
static const trait_id trait_QUILLS( "QUILLS" );
static const trait_id trait_RADIOACTIVE1( "RADIOACTIVE1" );
static const trait_id trait_RADIOACTIVE2( "RADIOACTIVE2" );
static const trait_id trait_RADIOACTIVE3( "RADIOACTIVE3" );
static const trait_id trait_RADIOGENIC( "RADIOGENIC" );
static const trait_id trait_REGEN( "REGEN" );
static const trait_id trait_REGEN_LIZ( "REGEN_LIZ" );
static const trait_id trait_ROOTS2( "ROOTS2" );
static const trait_id trait_ROOTS3( "ROOTS3" );
static const trait_id trait_SAPIOVORE( "SAPIOVORE" );
static const trait_id trait_SAVANT( "SAVANT" );
static const trait_id trait_SCHIZOPHRENIC( "SCHIZOPHRENIC" );
static const trait_id trait_SEESLEEP( "SEESLEEP" );
static const trait_id trait_SELFAWARE( "SELFAWARE" );
static const trait_id trait_SHARKTEETH( "SHARKTEETH" );
static const trait_id trait_SHELL2( "SHELL2" );
static const trait_id trait_SHOUT1( "SHOUT1" );
static const trait_id trait_SHOUT2( "SHOUT2" );
static const trait_id trait_SHOUT3( "SHOUT3" );
static const trait_id trait_SLEEK_SCALES( "SLEEK_SCALES" );
static const trait_id trait_SLIMESPAWNER( "SLIMESPAWNER" );
static const trait_id trait_SLIMY( "SLIMY" );
static const trait_id trait_SLOWHEALER( "SLOWHEALER" );
static const trait_id trait_SLOWLEARNER( "SLOWLEARNER" );
static const trait_id trait_SLOWREADER( "SLOWREADER" );
static const trait_id trait_SMELLY( "SMELLY" );
static const trait_id trait_SMELLY2( "SMELLY2" );
static const trait_id trait_SORES( "SORES" );
static const trait_id trait_SPINES( "SPINES" );
static const trait_id trait_SPIRITUAL( "SPIRITUAL" );
static const trait_id trait_SQUEAMISH( "SQUEAMISH" );
static const trait_id trait_STRONGSTOMACH( "STRONGSTOMACH" );
static const trait_id trait_SUNBURN( "SUNBURN" );
static const trait_id trait_SUNLIGHT_DEPENDENT( "SUNLIGHT_DEPENDENT" );
static const trait_id trait_TAIL_FIN( "TAIL_FIN" );
static const trait_id trait_THORNS( "THORNS" );
static const trait_id trait_THRESH_SPIDER( "THRESH_SPIDER" );
static const trait_id trait_TOUGH_FEET( "TOUGH_FEET" );
static const trait_id trait_TRANSPIRATION( "TRANSPIRATION" );
static const trait_id trait_TROGLO( "TROGLO" );
static const trait_id trait_TROGLO2( "TROGLO2" );
static const trait_id trait_TROGLO3( "TROGLO3" );
static const trait_id trait_UNSTABLE( "UNSTABLE" );
static const trait_id trait_URSINE_EYE( "URSINE_EYE" );
static const trait_id trait_URSINE_FUR( "URSINE_FUR" );
static const trait_id trait_VISCOUS( "VISCOUS" );
static const trait_id trait_VOMITOUS( "VOMITOUS" );
static const trait_id trait_WATERSLEEP( "WATERSLEEP" );
static const trait_id trait_WEAKSCENT( "WEAKSCENT" );
static const trait_id trait_WEAKSTOMACH( "WEAKSTOMACH" );
static const trait_id trait_WEBBED( "WEBBED" );
static const trait_id trait_WEB_SPINNER( "WEB_SPINNER" );
static const trait_id trait_WEB_WALKER( "WEB_WALKER" );
static const trait_id trait_WEB_WEAVER( "WEB_WEAVER" );
static const trait_id trait_WOOLALLERGY( "WOOLALLERGY" );

stat_mod player::get_pain_penalty() const
{
    stat_mod ret;
    int pain = get_perceived_pain();
    if( pain <= 0 ) {
        return ret;
    }

    int stat_penalty = std::floor( std::pow( pain, 0.8f ) / 10.0f );

    bool ceno = has_trait( trait_CENOBITE );
    if( !ceno ) {
        ret.strength = stat_penalty;
        ret.dexterity = stat_penalty;
    }

    if( !has_trait( trait_INT_SLIME ) ) {
        ret.intelligence = 1 + stat_penalty;
    } else {
        ret.intelligence = 1 + pain / 5;
    }

    ret.perception = stat_penalty * 2 / 3;

    ret.speed = std::pow( pain, 0.7f );
    if( ceno ) {
        ret.speed /= 2;
    }

    ret.speed = std::min( ret.speed, 50 );
    return ret;
}

player::player() :
    next_climate_control_check( calendar::before_time_starts )
    , cached_time( calendar::before_time_starts )
{
    str_cur = 8;
    str_max = 8;
    dex_cur = 8;
    dex_max = 8;
    int_cur = 8;
    int_max = 8;
    per_cur = 8;
    per_max = 8;
    dodges_left = 1;
    blocks_left = 1;
    set_power_level( 0_kJ );
    set_max_power_level( 0_kJ );
    stamina = 10000; //Temporary value for stamina. It will be reset later from external json option.
    stim = 0;
    pkill = 0;
    radiation = 0;
    tank_plut = 0;
    reactor_plut = 0;
    slow_rad = 0;
    cash = 0;
    scent = 500;
    male = true;
    prof = profession::has_initialized() ? profession::generic() :
           nullptr; //workaround for a potential structural limitation, see player::create

    start_location = start_location_id( "shelter" );
    moves = 100;
    movecounter = 0;
    oxygen = 0;
    last_climate_control_ret = false;
    in_vehicle = false;
    controlling_vehicle = false;
    grab_point = tripoint_zero;
    hauling = false;
    style_selected = style_none;
    keep_hands_free = false;
    focus_pool = 100;
    last_item = itype_id( "null" );
    sight_max = 9999;
    last_batch = 0;
    lastconsumed = itype_id( "null" );
    next_expected_position = cata::nullopt;
    death_drops = true;

    empty_traits();

    temp_cur.fill( BODYTEMP_NORM );
    frostbite_timer.fill( 0 );
    temp_conv.fill( BODYTEMP_NORM );
    body_wetness.fill( 0 );
    nv_cached = false;
    volume = 0;

    set_value( "THIEF_MODE", "THIEF_ASK" );

    for( const auto &v : vitamin::all() ) {
        vitamin_levels[ v.first ] = 0;
    }

    drench_capacity[bp_eyes] = 1;
    drench_capacity[bp_mouth] = 1;
    drench_capacity[bp_head] = 7;
    drench_capacity[bp_leg_l] = 11;
    drench_capacity[bp_leg_r] = 11;
    drench_capacity[bp_foot_l] = 3;
    drench_capacity[bp_foot_r] = 3;
    drench_capacity[bp_arm_l] = 10;
    drench_capacity[bp_arm_r] = 10;
    drench_capacity[bp_hand_l] = 3;
    drench_capacity[bp_hand_r] = 3;
    drench_capacity[bp_torso] = 40;

    recalc_sight_limits();
    reset_encumbrance();

    ma_styles = {{
            style_none, style_kicks
        }
    };
}

player::~player() = default;
player::player( player && ) = default;
player &player::operator=( player && ) = default;

void player::normalize()
{
    Character::normalize();

    style_selected = style_none;

    recalc_hp();

    temp_conv.fill( BODYTEMP_NORM );
    stamina = get_stamina_max();
}

void player::process_turn()
{
    // Has to happen before reset_stats
    clear_miss_reasons();

    Character::process_turn();

    // If we're actively handling something we can't just drop it on the ground
    // in the middle of handling it
    if( activity.targets.empty() ) {
        drop_invalid_inventory();
    }

    // Didn't just pick something up
    last_item = itype_id( "null" );

    if( has_active_bionic( bio_metabolics ) && !is_max_power() &&
        0.8f < get_kcal_percent() && calendar::once_every( 3_turns ) ) {
        // Efficiency is approximately 25%, power output is ~60W
        mod_stored_kcal( -1 );
        mod_power_level( 1_kJ );
    }
    if( has_trait( trait_DEBUG_BIONIC_POWER ) ) {
        mod_power_level( get_max_power_level() );
    }

    visit_items( [this]( item * e ) {
        e->process_artifact( this, pos() );
        e->process_relic( this );
        return VisitResponse::NEXT;
    } );

    suffer();

    // Set our scent towards the norm
    int norm_scent = 500;
    if( has_trait( trait_WEAKSCENT ) ) {
        norm_scent = 300;
    }
    if( has_trait( trait_SMELLY ) ) {
        norm_scent = 800;
    }
    if( has_trait( trait_SMELLY2 ) ) {
        norm_scent = 1200;
    }
    // Not so much that you don't have a scent
    // but that you smell like a plant, rather than
    // a human. When was the last time you saw a critter
    // attack a bluebell or an apple tree?
    if( ( has_trait( trait_FLOWERS ) ) && ( !( has_trait( trait_CHLOROMORPH ) ) ) ) {
        norm_scent -= 200;
    }
    // You *are* a plant.  Unless someone hunts triffids by scent,
    // you don't smell like prey.
    if( has_trait( trait_CHLOROMORPH ) ) {
        norm_scent = 0;
    }

    // Scent increases fast at first, and slows down as it approaches normal levels.
    // Estimate it will take about norm_scent * 2 turns to go from 0 - norm_scent / 2
    // Without smelly trait this is about 1.5 hrs. Slows down significantly after that.
    if( scent < rng( 0, norm_scent ) ) {
        scent++;
    }

    // Unusually high scent decreases steadily until it reaches normal levels.
    if( scent > norm_scent ) {
        scent--;
    }

    for( const trait_id &mut : get_mutations() ) {
        scent *= mut.obj().scent_modifier;
    }

    // We can dodge again! Assuming we can actually move...
    if( in_sleep_state() ) {
        blocks_left = 0;
        dodges_left = 0;
    } else if( moves > 0 ) {
        blocks_left = get_num_blocks();
        dodges_left = get_num_dodges();
    }

    // auto-learning. This is here because skill-increases happens all over the place:
    // SkillLevel::readBook (has no connection to the skill or the player),
    // player::read, player::practice, ...
    // Check for spontaneous discovery of martial art styles
    for( auto &style : autolearn_martialart_types() ) {
        const matype_id &ma( style );

        if( !has_martialart( ma ) && can_autolearn( ma ) ) {
            add_martialart( ma );
            add_msg_if_player( m_info, _( "You have learned a new style: %s!" ), ma.obj().name );
        }
    }

    // Update time spent conscious in this overmap tile for the Nomad traits.
    if( ( has_trait( trait_NOMAD ) || has_trait( trait_NOMAD2 ) || has_trait( trait_NOMAD3 ) ) &&
        !has_effect( effect_sleep ) && !has_effect( effect_narcosis ) ) {
        const tripoint ompos = global_omt_location();
        const point pos = ompos.xy();
        if( overmap_time.find( pos ) == overmap_time.end() ) {
            overmap_time[pos] = 1_turns;
        } else {
            overmap_time[pos] += 1_turns;
        }
    }
    // Decay time spent in other overmap tiles.
    if( calendar::once_every( 1_hours ) ) {
        const tripoint ompos = global_omt_location();
        const time_point now = calendar::turn;
        time_duration decay_time = 0_days;
        if( has_trait( trait_NOMAD ) ) {
            decay_time = 7_days;
        } else if( has_trait( trait_NOMAD2 ) ) {
            decay_time = 14_days;
        } else if( has_trait( trait_NOMAD3 ) ) {
            decay_time = 28_days;
        }
        auto it = overmap_time.begin();
        while( it != overmap_time.end() ) {
            if( it->first.x == ompos.x && it->first.y == ompos.y ) {
                it++;
                continue;
            }
            // Find the amount of time passed since the player touched any of the overmap tile's submaps.
            const tripoint tpt = tripoint( it->first, 0 );
            const time_point last_touched = overmap_buffer.scent_at( tpt ).creation_time;
            const time_duration since_visit = now - last_touched;
            // If the player has spent little time in this overmap tile, let it decay after just an hour instead of the usual extended decay time.
            const time_duration modified_decay_time = it->second > 5_minutes ? decay_time : 1_hours;
            if( since_visit > modified_decay_time ) {
                // Reduce the tracked time spent in this overmap tile.
                const time_duration decay_amount = std::min( since_visit - modified_decay_time, 1_hours );
                const time_duration updated_value = it->second - decay_amount;
                if( updated_value <= 0_turns ) {
                    // We can stop tracking this tile if there's no longer any time recorded there.
                    it = overmap_time.erase( it );
                    continue;
                } else {
                    it->second = updated_value;
                }
            }
            it++;
        }
    }
}

void player::action_taken()
{
    nv_cached = false;
}

void player::update_morale()
{
    morale->decay( 1_minutes );
    apply_persistent_morale();
}

void player::apply_persistent_morale()
{
    // Hoarders get a morale penalty if they're not carrying a full inventory.
    if( has_trait( trait_HOARDER ) ) {
        int pen = ( volume_capacity() - volume_carried() ) / 125_ml;
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
    // Nomads get a morale penalty if they stay near the same overmap tiles too long.
    if( has_trait( trait_NOMAD ) || has_trait( trait_NOMAD2 ) || has_trait( trait_NOMAD3 ) ) {
        const tripoint ompos = global_omt_location();
        float total_time = 0;
        // Check how long we've stayed in any overmap tile within 5 of us.
        const int max_dist = 5;
        for( const tripoint &pos : points_in_radius( ompos, max_dist ) ) {
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
        const int pen = ceil( lerp_clamped( 0, max_unhappiness, t ) );
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

void player::update_bodytemp()
{
    if( has_trait( trait_DEBUG_NOTEMP ) ) {
        temp_cur.fill( BODYTEMP_NORM );
        temp_conv.fill( BODYTEMP_NORM );
        return;
    }
    /* Cache calls to g->get_temperature( player position ), used in several places in function */
    const auto player_local_temp = g->weather.get_temperature( pos() );
    // NOTE : visit weather.h for some details on the numbers used
    // Converts temperature to Celsius/10
    int Ctemperature = static_cast<int>( 100 * temp_to_celsius( player_local_temp ) );
    const w_point weather = *g->weather.weather_precise;
    int vehwindspeed = 0;
    const optional_vpart_position vp = g->m.veh_at( pos() );
    if( vp ) {
        vehwindspeed = abs( vp->vehicle().velocity / 100 ); // vehicle velocity in mph
    }
    const oter_id &cur_om_ter = overmap_buffer.ter( global_omt_location() );
    bool sheltered = g->is_sheltered( pos() );
    double total_windpower = get_local_windpower( g->weather.windspeed + vehwindspeed, cur_om_ter,
                             pos(),
                             g->weather.winddirection, sheltered );
    // Let's cache this not to check it num_bp times
    const bool has_bark = has_trait( trait_BARK );
    const bool has_sleep = has_effect( effect_sleep );
    const bool has_sleep_state = has_sleep || in_sleep_state();
    const bool has_heatsink = has_bionic( bio_heatsink ) || is_wearing( "rm13_armor_on" ) ||
                              has_trait( trait_M_SKIN2 ) || has_trait( trait_M_SKIN3 );
    const bool has_common_cold = has_effect( effect_common_cold );
    const bool has_climate_control = in_climate_control();
    const bool use_floor_warmth = can_use_floor_warmth();
    const furn_id furn_at_pos = g->m.furn( pos() );
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
    const int fatigue_warmth = has_sleep ? 0 : static_cast<int>( std::min( 0.0f,
                               -1.5f * get_fatigue() ) );

    // Sunlight
    const int sunlight_warmth = g->is_in_sunlight( pos() ) ? ( g->weather.weather == WEATHER_SUNNY ?
                                1000 :
                                500 ) : 0;
    const int best_fire = get_heat_radiation( pos(), true );

    const int lying_warmth = use_floor_warmth ? floor_warmth( pos() ) : 0;
    const int water_temperature =
        100 * temp_to_celsius( g->weather.get_cur_weather_gen().get_water_temperature() );

    // Correction of body temperature due to traits and mutations
    // Lower heat is applied always
    const int mutation_heat_low = bodytemp_modifier_traits( false );
    const int mutation_heat_high = bodytemp_modifier_traits( true );
    // Difference between high and low is the "safe" heat - one we only apply if it's beneficial
    const int mutation_heat_bonus = mutation_heat_high - mutation_heat_low;

    const int h_radiation = get_heat_radiation( pos(), false );
    // Current temperature and converging temperature calculations
    for( const body_part bp : all_body_parts ) {
        // Skip eyes
        if( bp == bp_eyes ) {
            continue;
        }

        // This adjusts the temperature scale to match the bodytemp scale,
        // it needs to be reset every iteration
        int adjusted_temp = ( Ctemperature - ambient_norm );
        int bp_windpower = total_windpower;
        // Represents the fact that the body generates heat when it is cold.
        // TODO: : should this increase hunger?
        double scaled_temperature = logarithmic_range( BODYTEMP_VERY_COLD, BODYTEMP_VERY_HOT,
                                    temp_cur[bp] );
        // Produces a smooth curve between 30.0 and 60.0.
        double homeostasis_adjustement = 30.0 * ( 1.0 + scaled_temperature );
        int clothing_warmth_adjustement = static_cast<int>( homeostasis_adjustement * warmth( bp ) );
        int clothing_warmth_adjusted_bonus = static_cast<int>( homeostasis_adjustement * bonus_item_warmth(
                bp ) );
        // WINDCHILL

        bp_windpower = static_cast<int>( static_cast<float>( bp_windpower ) * ( 1 - get_wind_resistance(
                                             bp ) / 100.0 ) );
        // Calculate windchill
        int windchill = get_local_windchill( player_local_temp,
                                             get_local_humidity( weather.humidity, g->weather.weather,
                                                     sheltered ),
                                             bp_windpower );
        // If you're standing in water, air temperature is replaced by water temperature. No wind.
        const ter_id ter_at_pos = g->m.ter( pos() );
        // Convert to 0.01C
        if( ( ter_at_pos == t_water_dp || ter_at_pos == t_water_pool || ter_at_pos == t_swater_dp ||
              ter_at_pos == t_water_moving_dp ) ||
            ( ( ter_at_pos == t_water_sh || ter_at_pos == t_swater_sh || ter_at_pos == t_sewage ||
                ter_at_pos == t_water_moving_sh ) &&
              ( bp == bp_foot_l || bp == bp_foot_r || bp == bp_leg_l || bp == bp_leg_r ) ) ) {
            adjusted_temp += water_temperature - Ctemperature; // Swap out air temp for water temp.
            windchill = 0;
        }

        // Convergent temperature is affected by ambient temperature,
        // clothing warmth, and body wetness.
        temp_conv[bp] = BODYTEMP_NORM + adjusted_temp + windchill * 100 + clothing_warmth_adjustement;
        // HUNGER / STARVATION
        temp_conv[bp] += hunger_warmth;
        // FATIGUE
        temp_conv[bp] += fatigue_warmth;
        // Mutations
        temp_conv[bp] += mutation_heat_low;
        // DIRECT HEAT SOURCES (generates body heat, helps fight frostbite)
        // Bark : lowers blister count to -5; harder to get blisters
        int blister_count = ( has_bark ? -5 : 0 ); // If the counter is high, your skin starts to burn

        if( frostbite_timer[bp] > 0 ) {
            frostbite_timer[bp] -= std::max( 5, h_radiation );
        }
        // 111F (44C) is a temperature in which proteins break down: https://en.wikipedia.org/wiki/Burn
        blister_count += h_radiation - 111 > 0 ? std::max( static_cast<int>( sqrt( h_radiation - 111 ) ),
                         0 ) : 0;

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

        temp_conv[bp] += sunlight_warmth;
        // DISEASES
        if( bp == bp_head && has_effect( effect_flu ) ) {
            temp_conv[bp] += 1500;
        }
        if( has_common_cold ) {
            temp_conv[bp] -= 750;
        }
        // Loss of blood results in loss of body heat, 1% bodyheat lost per 2% hp lost
        temp_conv[bp] -= blood_loss( bp ) * temp_conv[bp] / 200;

        // EQUALIZATION
        switch( bp ) {
            case bp_torso:
                temp_equalizer( bp_torso, bp_arm_l );
                temp_equalizer( bp_torso, bp_arm_r );
                temp_equalizer( bp_torso, bp_leg_l );
                temp_equalizer( bp_torso, bp_leg_r );
                temp_equalizer( bp_torso, bp_head );
                break;
            case bp_head:
                temp_equalizer( bp_head, bp_torso );
                temp_equalizer( bp_head, bp_mouth );
                break;
            case bp_arm_l:
                temp_equalizer( bp_arm_l, bp_torso );
                temp_equalizer( bp_arm_l, bp_hand_l );
                break;
            case bp_arm_r:
                temp_equalizer( bp_arm_r, bp_torso );
                temp_equalizer( bp_arm_r, bp_hand_r );
                break;
            case bp_leg_l:
                temp_equalizer( bp_leg_l, bp_torso );
                temp_equalizer( bp_leg_l, bp_foot_l );
                break;
            case bp_leg_r:
                temp_equalizer( bp_leg_r, bp_torso );
                temp_equalizer( bp_leg_r, bp_foot_r );
                break;
            case bp_mouth:
                temp_equalizer( bp_mouth, bp_head );
                break;
            case bp_hand_l:
                temp_equalizer( bp_hand_l, bp_arm_l );
                break;
            case bp_hand_r:
                temp_equalizer( bp_hand_r, bp_arm_r );
                break;
            case bp_foot_l:
                temp_equalizer( bp_foot_l, bp_leg_l );
                break;
            case bp_foot_r:
                temp_equalizer( bp_foot_r, bp_leg_r );
                break;
            default:
                debugmsg( "Wacky body part temperature equalization!" );
                break;
        }

        // Climate Control eases the effects of high and low ambient temps
        if( has_climate_control ) {
            temp_conv[bp] = temp_corrected_by_climate_control( temp_conv[bp] );
        }

        // FINAL CALCULATION : Increments current body temperature towards convergent.
        int bonus_fire_warmth = 0;
        if( !has_sleep_state && best_fire > 0 ) {
            // Warming up over a fire
            // Extremities are easier to extend over a fire
            switch( bp ) {
                case bp_head:
                case bp_torso:
                case bp_mouth:
                case bp_leg_l:
                case bp_leg_r:
                    bonus_fire_warmth = best_fire * best_fire * 150; // Not much
                    break;
                case bp_arm_l:
                case bp_arm_r:
                    bonus_fire_warmth = best_fire * 600; // A fair bit
                    break;
                case bp_foot_l:
                case bp_foot_r:
                    if( furn_at_pos != f_null ) {
                        // Can sit on something to lift feet up to the fire
                        bonus_fire_warmth = best_fire * furn_at_pos.obj().bonus_fire_warmth_feet;
                    } else if( boardable ) {
                        bonus_fire_warmth = best_fire * boardable->info().bonus_fire_warmth_feet;
                    } else {
                        // Has to stand
                        bonus_fire_warmth = best_fire * 300;
                    }
                    break;
                case bp_hand_l:
                case bp_hand_r:
                    bonus_fire_warmth = best_fire * 1500; // A lot
                default:
                    break;
            }
        }

        const int comfortable_warmth = bonus_fire_warmth + lying_warmth;
        const int bonus_warmth = comfortable_warmth + metabolism_warmth + mutation_heat_bonus;
        if( bonus_warmth > 0 ) {
            // Approximate temp_conv needed to reach comfortable temperature in this very turn
            // Basically inverted formula for temp_cur below
            int desired = 501 * BODYTEMP_NORM - 499 * temp_cur[bp];
            if( std::abs( BODYTEMP_NORM - desired ) < 1000 ) {
                desired = BODYTEMP_NORM; // Ensure that it converges
            } else if( desired > BODYTEMP_HOT ) {
                desired = BODYTEMP_HOT; // Cap excess at sane temperature
            }

            if( desired < temp_conv[bp] ) {
                // Too hot, can't help here
            } else if( desired < temp_conv[bp] + bonus_warmth ) {
                // Use some heat, but not all of it
                temp_conv[bp] = desired;
            } else {
                // Use all the heat
                temp_conv[bp] += bonus_warmth;
            }

            // Morale bonus for comfiness - only if actually comfy (not too warm/cold)
            // Spread the morale bonus in time.
            if( comfortable_warmth > 0 &&
                // @todo make this simpler and use time_duration/time_point
                to_turn<int>( calendar::turn ) % to_turns<int>( 1_minutes ) == to_turns<int>
                ( 1_minutes * bp ) / to_turns<int>( 1_minutes * num_bp ) &&
                get_effect_int( effect_cold, num_bp ) == 0 &&
                get_effect_int( effect_hot, num_bp ) == 0 &&
                temp_cur[bp] > BODYTEMP_COLD && temp_cur[bp] <= BODYTEMP_NORM ) {
                add_morale( MORALE_COMFY, 1, 10, 2_minutes, 1_minutes, true );
            }
        }

        int temp_before = temp_cur[bp];
        int temp_difference = temp_before - temp_conv[bp]; // Negative if the player is warming up.
        // exp(-0.001) : half life of 60 minutes, exp(-0.002) : half life of 30 minutes,
        // exp(-0.003) : half life of 20 minutes, exp(-0.004) : half life of 15 minutes
        int rounding_error = 0;
        // If temp_diff is small, the player cannot warm up due to rounding errors. This fixes that.
        if( temp_difference < 0 && temp_difference > -600 ) {
            rounding_error = 1;
        }
        if( temp_cur[bp] != temp_conv[bp] ) {
            temp_cur[bp] = static_cast<int>( temp_difference * exp( -0.002 ) + temp_conv[bp] + rounding_error );
        }
        // This statement checks if we should be wearing our bonus warmth.
        // If, after all the warmth calculations, we should be, then we have to recalculate the temperature.
        if( clothing_warmth_adjusted_bonus != 0 &&
            ( ( temp_conv[bp] + clothing_warmth_adjusted_bonus ) < BODYTEMP_HOT ||
              temp_cur[bp] < BODYTEMP_COLD ) ) {
            temp_conv[bp] += clothing_warmth_adjusted_bonus;
            rounding_error = 0;
            if( temp_difference < 0 && temp_difference > -600 ) {
                rounding_error = 1;
            }
            if( temp_before != temp_conv[bp] ) {
                temp_difference = temp_before - temp_conv[bp];
                temp_cur[bp] = static_cast<int>( temp_difference * exp( -0.002 ) + temp_conv[bp] + rounding_error );
            }
        }
        int temp_after = temp_cur[bp];
        // PENALTIES
        if( temp_cur[bp] < BODYTEMP_FREEZING ) {
            add_effect( effect_cold, 1_turns, bp, true, 3 );
        } else if( temp_cur[bp] < BODYTEMP_VERY_COLD ) {
            add_effect( effect_cold, 1_turns, bp, true, 2 );
        } else if( temp_cur[bp] < BODYTEMP_COLD ) {
            add_effect( effect_cold, 1_turns, bp, true, 1 );
        } else if( temp_cur[bp] > BODYTEMP_SCORCHING ) {
            add_effect( effect_hot, 1_turns, bp, true, 3 );
        } else if( temp_cur[bp] > BODYTEMP_VERY_HOT ) {
            add_effect( effect_hot, 1_turns, bp, true, 2 );
        } else if( temp_cur[bp] > BODYTEMP_HOT ) {
            add_effect( effect_hot, 1_turns, bp, true, 1 );
        } else {
            if( temp_cur[bp] >= BODYTEMP_COLD ) {
                remove_effect( effect_cold, bp );
            }
            if( temp_cur[bp] <= BODYTEMP_HOT ) {
                remove_effect( effect_hot, bp );
            }
        }
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

        if( bp == bp_mouth || bp == bp_foot_r || bp == bp_foot_l || bp == bp_hand_r || bp == bp_hand_l ) {
            // Handle the frostbite timer
            // Need temps in F, windPower already in mph
            int wetness_percentage = 100 * body_wetness[bp] / drench_capacity[bp]; // 0 - 100
            // Warmth gives a slight buff to temperature resistance
            // Wetness gives a heavy nerf to temperature resistance
            double adjusted_warmth = warmth( bp ) - wetness_percentage;
            int Ftemperature = static_cast<int>( player_local_temp + 0.2 * adjusted_warmth );
            // Windchill reduced by your armor
            int FBwindPower = static_cast<int>(
                                  total_windpower * ( 1 - get_wind_resistance( bp ) / 100.0 ) );

            int intense = get_effect_int( effect_frostbite, bp );

            // This has been broken down into 8 zones
            // Low risk zones (stops at frostnip)
            if( temp_cur[bp] < BODYTEMP_COLD &&
                ( ( Ftemperature < 30 && Ftemperature >= 10 ) ||
                  ( Ftemperature < 10 && Ftemperature >= -5 &&
                    FBwindPower < 20 && -4 * Ftemperature + 3 * FBwindPower - 20 >= 0 ) ) ) {
                if( frostbite_timer[bp] < 2000 ) {
                    frostbite_timer[bp] += 3;
                }
                if( one_in( 100 ) && !has_effect( effect_frostbite, bp ) ) {
                    add_msg( m_warning, _( "Your %s will be frostnipped in the next few hours." ),
                             body_part_name( bp ) );
                }
                // Medium risk zones
            } else if( temp_cur[bp] < BODYTEMP_COLD &&
                       ( ( Ftemperature < 10 && Ftemperature >= -5 && FBwindPower < 20 &&
                           -4 * Ftemperature + 3 * FBwindPower - 20 < 0 ) ||
                         ( Ftemperature < 10 && Ftemperature >= -5 && FBwindPower >= 20 ) ||
                         ( Ftemperature < -5 && FBwindPower < 10 ) ||
                         ( Ftemperature < -5 && FBwindPower >= 10 &&
                           -4 * Ftemperature + 3 * FBwindPower - 170 >= 0 ) ) ) {
                frostbite_timer[bp] += 8;
                if( one_in( 100 ) && intense < 2 ) {
                    add_msg( m_warning, _( "Your %s will be frostbitten within the hour!" ),
                             body_part_name( bp ) );
                }
                // High risk zones
            } else if( temp_cur[bp] < BODYTEMP_COLD &&
                       ( ( Ftemperature < -5 && FBwindPower >= 10 &&
                           -4 * Ftemperature + 3 * FBwindPower - 170 < 0 ) ||
                         ( Ftemperature < -35 && FBwindPower >= 10 ) ) ) {
                frostbite_timer[bp] += 72;
                if( one_in( 100 ) && intense < 2 ) {
                    add_msg( m_warning, _( "Your %s will be frostbitten any minute now!" ),
                             body_part_name( bp ) );
                }
                // Risk free, so reduce frostbite timer
            } else {
                frostbite_timer[bp] -= 3;
            }

            // Handle the bestowing of frostbite
            if( frostbite_timer[bp] < 0 ) {
                frostbite_timer[bp] = 0;
            } else if( frostbite_timer[bp] > 4200 ) {
                // This ensures that the player will recover in at most 3 hours.
                frostbite_timer[bp] = 4200;
            }
            // Frostbite, no recovery possible
            if( frostbite_timer[bp] >= 3600 ) {
                add_effect( effect_frostbite, 1_turns, bp, true, 2 );
                remove_effect( effect_frostbite_recovery, bp );
                // Else frostnip, add recovery if we were frostbitten
            } else if( frostbite_timer[bp] >= 1800 ) {
                if( intense == 2 ) {
                    add_effect( effect_frostbite_recovery, 1_turns, bp, true );
                }
                add_effect( effect_frostbite, 1_turns, bp, true, 1 );
                // Else fully recovered
            } else if( frostbite_timer[bp] == 0 ) {
                remove_effect( effect_frostbite, bp );
                remove_effect( effect_frostbite_recovery, bp );
            }
        }
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

        // Warn the player that wind is going to be a problem.
        // But only if it can be a problem, no need to spam player with "wind chills your scorching body"
        if( temp_conv[bp] <= BODYTEMP_COLD && windchill < -10 && one_in( 200 ) ) {
            add_msg( m_bad, _( "The wind is making your %s feel quite cold." ),
                     body_part_name( bp ) );
        } else if( temp_conv[bp] <= BODYTEMP_COLD && windchill < -20 && one_in( 100 ) ) {
            add_msg( m_bad,
                     _( "The wind is very strong, you should find some more wind-resistant clothing for your %s." ),
                     body_part_name( bp ) );
        } else if( temp_conv[bp] <= BODYTEMP_COLD && windchill < -30 && one_in( 50 ) ) {
            add_msg( m_bad, _( "Your clothing is not providing enough protection from the wind for your %s!" ),
                     body_part_name( bp ) );
        }
    }
}

bool player::can_use_floor_warmth() const
{
    // TODO: Reading? Waiting?
    return in_sleep_state();
}

int player::floor_bedding_warmth( const tripoint &pos )
{
    const trap &trap_at_pos = g->m.tr_at( pos );
    const ter_id ter_at_pos = g->m.ter( pos );
    const furn_id furn_at_pos = g->m.furn( pos );
    int floor_bedding_warmth = 0;

    const optional_vpart_position vp = g->m.veh_at( pos );
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

int player::floor_item_warmth( const tripoint &pos )
{
    if( !g->m.has_items( pos ) ) {
        return 0;
    }

    int item_warmth = 0;
    // Search the floor for items
    const auto floor_item = g->m.i_at( pos );
    for( const auto &elem : floor_item ) {
        if( !elem.is_armor() ) {
            continue;
        }
        // Items that are big enough and covers the torso are used to keep warm.
        // Smaller items don't do as good a job
        if( elem.volume() > 250_ml &&
            ( elem.covers( bp_torso ) || elem.covers( bp_leg_l ) ||
              elem.covers( bp_leg_r ) ) ) {
            item_warmth += 60 * elem.get_warmth() * elem.volume() / 2500_ml;
        }
    }

    return item_warmth;
}

int player::floor_warmth( const tripoint &pos ) const
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

int player::bodytemp_modifier_traits( bool overheated ) const
{
    int mod = 0;
    for( auto &iter : my_mutations ) {
        mod += overheated ? iter.first->bodytemp_min :
               iter.first->bodytemp_max;
    }
    return mod;
}

int player::bodytemp_modifier_traits_floor() const
{
    int mod = 0;
    for( auto &iter : my_mutations ) {
        mod += iter.first->bodytemp_sleep;
    }
    return mod;
}

int player::temp_corrected_by_climate_control( int temperature ) const
{
    const int variation = int( BODYTEMP_NORM * 0.5 );
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

int player::blood_loss( body_part bp ) const
{
    int hp_cur_sum = 1;
    int hp_max_sum = 1;

    if( bp == bp_leg_l || bp == bp_leg_r ) {
        hp_cur_sum = hp_cur[hp_leg_l] + hp_cur[hp_leg_r];
        hp_max_sum = hp_max[hp_leg_l] + hp_max[hp_leg_r];
    } else if( bp == bp_arm_l || bp == bp_arm_r ) {
        hp_cur_sum = hp_cur[hp_arm_l] + hp_cur[hp_arm_r];
        hp_max_sum = hp_max[hp_arm_l] + hp_max[hp_arm_r];
    } else if( bp == bp_torso ) {
        hp_cur_sum = hp_cur[hp_torso];
        hp_max_sum = hp_max[hp_torso];
    } else if( bp == bp_head ) {
        hp_cur_sum = hp_cur[hp_head];
        hp_max_sum = hp_max[hp_head];
    }

    hp_cur_sum = std::min( hp_max_sum, std::max( 0, hp_cur_sum ) );
    return 100 - ( 100 * hp_cur_sum ) / hp_max_sum;
}

void player::temp_equalizer( body_part bp1, body_part bp2 )
{
    // Body heat is moved around.
    // Shift in one direction only, will be shifted in the other direction separately.
    int diff = static_cast<int>( ( temp_cur[bp2] - temp_cur[bp1] ) *
                                 0.0001 ); // If bp1 is warmer, it will lose heat
    temp_cur[bp1] += diff;
}

int player::kcal_speed_penalty()
{
    static const std::vector<std::pair<float, float>> starv_thresholds = { {
            std::make_pair( 0.0f, -90.0f ),
            std::make_pair( character_weight_category::emaciated, -50.f ),
            std::make_pair( character_weight_category::underweight, -25.0f ),
            std::make_pair( character_weight_category::normal, 0.0f )
        }
    };
    if( get_kcal_percent() > 0.95f ) {
        // @TODO: get speed penalties for being too fat, too
        return 0;
    } else {
        return round( multi_lerp( starv_thresholds, get_bmi() ) );
    }
}

int player::thirst_speed_penalty( int thirst )
{
    // We die at 1200 thirst
    // Start by dropping speed really fast, but then level it off a bit
    static const std::vector<std::pair<float, float>> thirst_thresholds = {{
            std::make_pair( 40.0f, 0.0f ),
            std::make_pair( 300.0f, -25.0f ),
            std::make_pair( 600.0f, -50.0f ),
            std::make_pair( 1200.0f, -75.0f )
        }
    };
    return static_cast<int>( multi_lerp( thirst_thresholds, thirst ) );
}

void player::recalc_speed_bonus()
{
    // Minus some for weight...
    int carry_penalty = 0;
    if( weight_carried() > weight_capacity() && !has_trait( trait_id( "DEBUG_STORAGE" ) ) ) {
        carry_penalty = 25 * ( weight_carried() - weight_capacity() ) / ( weight_capacity() );
    }
    mod_speed_bonus( -carry_penalty );

    mod_speed_bonus( -get_pain_penalty().speed );

    if( get_thirst() > 40 ) {
        mod_speed_bonus( thirst_speed_penalty( get_thirst() ) );
    }
    // fat or underweight, you get slower. cumulative with hunger
    mod_speed_bonus( kcal_speed_penalty() );

    for( const auto &maps : *effects ) {
        for( auto i : maps.second ) {
            bool reduced = resists_effect( i.second );
            mod_speed_bonus( i.second.get_mod( "SPEED", reduced ) );
        }
    }

    // add martial arts speed bonus
    mod_speed_bonus( mabuff_speed_bonus() );

    // Not sure why Sunlight Dependent is here, but OK
    // Ectothermic/COLDBLOOD4 is intended to buff folks in the Summer
    // Threshold-crossing has its charms ;-)
    if( g != nullptr ) {
        if( has_trait( trait_SUNLIGHT_DEPENDENT ) && !g->is_in_sunlight( pos() ) ) {
            mod_speed_bonus( -( g->light_level( posz() ) >= 12 ? 5 : 10 ) );
        }
        const float temperature_speed_modifier = mutation_value( "temperature_speed_modifier" );
        if( temperature_speed_modifier != 0 ) {
            const auto player_local_temp = g->weather.get_temperature( pos() );
            if( has_trait( trait_COLDBLOOD4 ) || player_local_temp < 65 ) {
                mod_speed_bonus( ( player_local_temp - 65 ) * temperature_speed_modifier );
            }
        }
    }

    if( has_artifact_with( AEP_SPEED_UP ) ) {
        mod_speed_bonus( 20 );
    }
    if( has_artifact_with( AEP_SPEED_DOWN ) ) {
        mod_speed_bonus( -20 );
    }

    float speed_modifier = Character::mutation_value( "speed_modifier" );
    set_speed_bonus( static_cast<int>( get_speed() * speed_modifier ) - get_speed_base() );

    if( has_bionic( bio_speed ) ) { // multiply by 1.1
        set_speed_bonus( static_cast<int>( get_speed() * 1.1 ) - get_speed_base() );
    }

    // Speed cannot be less than 25% of base speed, so minimal speed bonus is -75% base speed.
    const int min_speed_bonus = static_cast<int>( -0.75 * get_speed_base() );
    if( get_speed_bonus() < min_speed_bonus ) {
        set_speed_bonus( min_speed_bonus );
    }
}

int player::run_cost( int base_cost, bool diag ) const
{
    float movecost = static_cast<float>( base_cost );
    if( diag ) {
        movecost *= 0.7071f; // because everything here assumes 100 is base
    }
    const bool flatground = movecost < 105;
    // The "FLAT" tag includes soft surfaces, so not a good fit.
    const bool on_road = flatground && g->m.has_flag( "ROAD", pos() );
    const bool on_fungus = g->m.has_flag_ter_or_furn( "FUNGUS", pos() );

    if( !is_mounted() ) {
        if( movecost > 100 ) {
            movecost *= Character::mutation_value( "movecost_obstacle_modifier" );
            if( movecost < 100 ) {
                movecost = 100;
            }
        }
        if( has_trait( trait_M_IMMUNE ) && on_fungus ) {
            if( movecost > 75 ) {
                movecost =
                    75; // Mycal characters are faster on their home territory, even through things like shrubs
            }
        }
        if( is_limb_hindered( hp_leg_l ) ) {
            movecost += 25;
        }

        if( is_limb_hindered( hp_leg_r ) ) {
            movecost += 25;
        }
        movecost *= Character::mutation_value( "movecost_modifier" );
        if( flatground ) {
            movecost *= Character::mutation_value( "movecost_flatground_modifier" );
        }
        if( has_trait( trait_PADDED_FEET ) && !footwear_factor() ) {
            movecost *= .9f;
        }
        if( has_active_bionic( bio_jointservo ) ) {
            if( move_mode == CMM_RUN ) {
                movecost *= 0.85f;
            } else {
                movecost *= 0.95f;
            }
        } else if( has_bionic( bio_jointservo ) ) {
            movecost *= 1.1f;
        }

        if( worn_with_flag( "SLOWS_MOVEMENT" ) ) {
            movecost *= 1.1f;
        }
        if( worn_with_flag( "FIN" ) ) {
            movecost *= 1.5f;
        }
        if( worn_with_flag( "ROLLER_INLINE" ) ) {
            if( on_road ) {
                movecost *= 0.5f;
            } else {
                movecost *= 1.5f;
            }
        }
        // Quad skates might be more stable than inlines,
        // but that also translates into a slower speed when on good surfaces.
        if( worn_with_flag( "ROLLER_QUAD" ) ) {
            if( on_road ) {
                movecost *= 0.7f;
            } else {
                movecost *= 1.3f;
            }
        }
        // Skates with only one wheel (roller shoes) are fairly less stable
        // and fairly slower as well
        if( worn_with_flag( "ROLLER_ONE" ) ) {
            if( on_road ) {
                movecost *= 0.85f;
            } else {
                movecost *= 1.1f;
            }
        }

        movecost +=
            ( ( encumb( bp_foot_l ) + encumb( bp_foot_r ) ) * 2.5 +
              ( encumb( bp_leg_l ) + encumb( bp_leg_r ) ) * 1.5 ) / 10;

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

        if( has_trait( trait_ROOTS3 ) && g->m.has_flag( "DIGGABLE", pos() ) ) {
            movecost += 10 * footwear_factor();
        }
        // Both walk and run speed drop to half their maximums as stamina approaches 0.
        // Convert stamina to a float first to allow for decimal place carrying
        float stamina_modifier = ( static_cast<float>( stamina ) / get_stamina_max() + 1 ) / 2;
        if( move_mode == CMM_RUN && stamina > 0 ) {
            // Rationale: Average running speed is 2x walking speed. (NOT sprinting)
            stamina_modifier *= 2.0;
        }
        if( move_mode == CMM_CROUCH ) {
            stamina_modifier *= 0.5;
        }

        movecost = calculate_by_enchantment( movecost, enchantment::mod::MOVE_COST );
        movecost /= stamina_modifier;
    }

    if( diag ) {
        movecost *= M_SQRT2;
    }

    return static_cast<int>( movecost );
}

int player::swim_speed() const
{
    int ret;
    if( is_mounted() ) {
        monster *mon = mounted_creature.get();
        // no difference in swim speed by monster type yet.
        // TODO : difference in swim speed by monster type.
        // No monsters are currently mountable and can swim, though mods may allow this.
        if( mon->has_flag( MF_SWIMS ) ) {
            ret = 25;
            ret += get_weight() / 120_gram - 50 * mon->get_size();
            return ret;
        }
    }
    const auto usable = exclusive_flag_coverage( "ALLOWS_NATURAL_ATTACKS" );
    float hand_bonus_mult = ( usable.test( bp_hand_l ) ? 0.5f : 0.0f ) +
                            ( usable.test( bp_hand_r ) ? 0.5f : 0.0f );

    if( !has_trait( trait_AMPHIBIAN ) ) {
        ret = 440 + weight_carried() / 60_gram - 50 * get_skill_level( skill_swimming );
        /** AMPHIBIAN increases base swim speed */
    } else {
        ret = 200 + weight_carried() / 120_gram - 50 * get_skill_level( skill_swimming );
    }
    /** @EFFECT_STR increases swim speed bonus from PAWS */
    if( has_trait( trait_PAWS ) ) {
        ret -= hand_bonus_mult * ( 20 + str_cur * 3 );
    }
    /** @EFFECT_STR increases swim speed bonus from PAWS_LARGE */
    if( has_trait( trait_PAWS_LARGE ) ) {
        ret -= hand_bonus_mult * ( 20 + str_cur * 4 );
    }
    /** @EFFECT_STR increases swim speed bonus from swim_fins */
    if( worn_with_flag( "FIN", bp_foot_l ) || worn_with_flag( "FIN", bp_foot_r ) ) {
        if( worn_with_flag( "FIN", bp_foot_l ) && worn_with_flag( "FIN", bp_foot_r ) ) {
            ret -= ( 15 * str_cur );
        } else {
            ret -= ( 15 * str_cur ) / 2;
        }
    }
    /** @EFFECT_STR increases swim speed bonus from WEBBED */
    if( has_trait( trait_WEBBED ) ) {
        ret -= hand_bonus_mult * ( 60 + str_cur * 5 );
    }
    /** @EFFECT_STR increases swim speed bonus from TAIL_FIN */
    if( has_trait( trait_TAIL_FIN ) ) {
        ret -= 100 + str_cur * 10;
    }
    if( has_trait( trait_SLEEK_SCALES ) ) {
        ret -= 100;
    }
    if( has_trait( trait_LEG_TENTACLES ) ) {
        ret -= 60;
    }
    if( has_trait( trait_FAT ) ) {
        ret -= 30;
    }
    /** @EFFECT_SWIMMING increases swim speed */
    ret += ( 50 - get_skill_level( skill_swimming ) * 2 ) * ( ( encumb( bp_leg_l ) + encumb(
                bp_leg_r ) ) / 10 );
    ret += ( 80 - get_skill_level( skill_swimming ) * 3 ) * ( encumb( bp_torso ) / 10 );
    if( get_skill_level( skill_swimming ) < 10 ) {
        for( auto &i : worn ) {
            ret += i.volume() / 125_ml * ( 10 - get_skill_level( skill_swimming ) );
        }
    }
    /** @EFFECT_STR increases swim speed */

    /** @EFFECT_DEX increases swim speed */
    ret -= str_cur * 6 + dex_cur * 4;
    if( worn_with_flag( "FLOTATION" ) ) {
        ret = std::min( ret, 400 );
        ret = std::max( ret, 200 );
    }
    // If (ret > 500), we can not swim; so do not apply the underwater bonus.
    if( underwater && ret < 500 ) {
        ret -= 50;
    }

    // Running movement mode while swimming means faster swim style, like crawlstroke
    if( move_mode == CMM_RUN ) {
        ret -= 80;
    }
    // Crouching movement mode while swimming means slower swim style, like breaststroke
    if( move_mode == CMM_CROUCH ) {
        ret += 50;
    }

    if( ret < 30 ) {
        ret = 30;
    }
    return ret;
}

bool player::digging() const
{
    return false;
}

bool player::is_on_ground() const
{
    return get_working_leg_count() < 2 || has_effect( effect_downed );
}

bool player::is_elec_immune() const
{
    return is_immune_damage( DT_ELECTRIC );
}

bool player::is_immune_effect( const efftype_id &eff ) const
{
    if( eff == effect_downed ) {
        return is_throw_immune() || ( has_trait( trait_LEG_TENT_BRACE ) && footwear_factor() == 0 );
    } else if( eff == effect_onfire ) {
        return is_immune_damage( DT_HEAT );
    } else if( eff == effect_deaf ) {
        return worn_with_flag( "DEAF" ) || worn_with_flag( "PARTIAL_DEAF" ) || has_bionic( bio_ears ) ||
               is_wearing( "rm13_armor_on" );
    } else if( eff == effect_corroding ) {
        return is_immune_damage( DT_ACID ) || has_trait( trait_SLIMY ) || has_trait( trait_VISCOUS );
    } else if( eff == effect_nausea ) {
        return has_trait( trait_STRONGSTOMACH );
    }

    return false;
}

float player::stability_roll() const
{
    /** @EFFECT_STR improves player stability roll */

    /** @EFFECT_PER slightly improves player stability roll */

    /** @EFFECT_DEX slightly improves player stability roll */

    /** @EFFECT_MELEE improves player stability roll */
    return get_melee() + get_str() + ( get_per() / 3.0f ) + ( get_dex() / 4.0f );
}

bool player::is_immune_damage( const damage_type dt ) const
{
    switch( dt ) {
        case DT_NULL:
            return true;
        case DT_TRUE:
            return false;
        case DT_BIOLOGICAL:
            return has_effect_with_flag( "EFFECT_BIO_IMMUNE" ) ||
                   worn_with_flag( "BIO_IMMUNE" );
        case DT_BASH:
            return has_effect_with_flag( "EFFECT_BASH_IMMUNE" ) ||
                   worn_with_flag( "BASH_IMMUNE" );
        case DT_CUT:
            return has_effect_with_flag( "EFFECT_CUT_IMMUNE" ) ||
                   worn_with_flag( "CUT_IMMUNE" );
        case DT_ACID:
            return has_trait( trait_ACIDPROOF ) ||
                   has_effect_with_flag( "EFFECT_ACID_IMMUNE" ) ||
                   worn_with_flag( "ACID_IMMUNE" );
        case DT_STAB:
            return has_effect_with_flag( "EFFECT_STAB_IMMUNE" ) ||
                   worn_with_flag( "STAB_IMMUNE" );
        case DT_HEAT:
            return has_trait( trait_M_SKIN2 ) ||
                   has_trait( trait_M_SKIN3 ) ||
                   has_effect_with_flag( "EFFECT_HEAT_IMMUNE" ) ||
                   worn_with_flag( "HEAT_IMMUNE" );
        case DT_COLD:
            return has_effect_with_flag( "EFFECT_COLD_IMMUNE" ) ||
                   worn_with_flag( "COLD_IMMUNE" );
        case DT_ELECTRIC:
            return has_active_bionic( bio_faraday ) ||
                   worn_with_flag( "ELECTRIC_IMMUNE" ) ||
                   has_artifact_with( AEP_RESIST_ELECTRICITY ) ||
                   has_effect_with_flag( "EFFECT_ELECTRIC_IMMUNE" );
        default:
            return true;
    }
}

double player::recoil_vehicle() const
{
    // TODO: vary penalty dependent upon vehicle part on which player is boarded

    if( in_vehicle ) {
        if( const optional_vpart_position vp = g->m.veh_at( pos() ) ) {
            return static_cast<double>( abs( vp->vehicle().velocity ) ) * 3 / 100;
        }
    }
    return 0;
}

double player::recoil_total() const
{
    return recoil + recoil_vehicle();
}

bool player::is_underwater() const
{
    return underwater;
}

bool player::is_hallucination() const
{
    return false;
}

void player::set_underwater( bool u )
{
    if( underwater != u ) {
        underwater = u;
        recalc_sight_limits();
    }
}

nc_color player::basic_symbol_color() const
{
    if( has_effect( effect_onfire ) ) {
        return c_red;
    }
    if( has_effect( effect_stunned ) ) {
        return c_light_blue;
    }
    if( has_effect( effect_boomered ) ) {
        return c_pink;
    }
    if( has_active_mutation( trait_id( "SHELL2" ) ) ) {
        return c_magenta;
    }
    if( underwater ) {
        return c_blue;
    }
    if( has_active_bionic( bio_cloak ) || has_artifact_with( AEP_INVISIBLE ) ||
        is_wearing_active_optcloak() || has_trait( trait_DEBUG_CLOAK ) ) {
        return c_dark_gray;
    }
    if( move_mode == CMM_RUN ) {
        return c_yellow;
    }
    if( move_mode == CMM_CROUCH ) {
        return c_light_gray;
    }
    return c_white;
}

void player::mod_stat( const std::string &stat, float modifier )
{
    if( stat == "thirst" ) {
        mod_thirst( modifier );
    } else if( stat == "fatigue" ) {
        mod_fatigue( modifier );
    } else if( stat == "oxygen" ) {
        oxygen += modifier;
    } else if( stat == "stamina" ) {
        if( stamina + modifier < 0 ) {
            add_effect( effect_winded, 10_turns );
        }
        stamina += modifier;
        stamina = std::min( stamina, get_stamina_max() );
        stamina = std::max( 0, stamina );
    } else {
        // Fall through to the creature method.
        Character::mod_stat( stat, modifier );
    }
}

time_duration player::estimate_effect_dur( const skill_id &relevant_skill,
        const efftype_id &target_effect, const time_duration &error_magnitude,
        int threshold, const Creature &target ) const
{
    const time_duration zero_duration = 0_turns;

    int skill_lvl = get_skill_level( relevant_skill );

    time_duration estimate = std::max( zero_duration, target.get_effect_dur( target_effect ) +
                                       rng( -1, 1 ) * error_magnitude *
                                       rng( 0, std::max( 0, threshold - skill_lvl ) ) );
    return estimate;
}

bool player::has_conflicting_trait( const trait_id &flag ) const
{
    return ( has_opposite_trait( flag ) || has_lower_trait( flag ) || has_higher_trait( flag ) ||
             has_same_type_trait( flag ) );
}

bool player::has_opposite_trait( const trait_id &flag ) const
{
    for( auto &i : flag->cancels ) {
        if( has_trait( i ) ) {
            return true;
        }
    }
    return false;
}

bool player::has_lower_trait( const trait_id &flag ) const
{
    for( auto &i : flag->prereqs ) {
        if( has_trait( i ) || has_lower_trait( i ) ) {
            return true;
        }
    }
    return false;
}

bool player::has_higher_trait( const trait_id &flag ) const
{
    for( auto &i : flag->replacements ) {
        if( has_trait( i ) || has_higher_trait( i ) ) {
            return true;
        }
    }
    return false;
}

bool player::has_same_type_trait( const trait_id &flag ) const
{
    for( auto &i : get_mutations_in_types( flag->types ) ) {
        if( has_trait( i ) && flag != i ) {
            return true;
        }
    }
    return false;
}

bool player::purifiable( const trait_id &flag ) const
{
    return flag->purifiable;
}

/// Returns a randomly selected dream
std::string player::get_category_dream( const std::string &cat,
                                        int strength ) const
{
    std::vector<dream> valid_dreams;
    //Pull the list of dreams
    for( auto &i : dreams ) {
        //Pick only the ones matching our desired category and strength
        if( ( i.category == cat ) && ( i.strength == strength ) ) {
            // Put the valid ones into our list
            valid_dreams.push_back( i );
        }
    }
    if( valid_dreams.empty() ) {
        return "";
    }
    const dream &selected_dream = random_entry( valid_dreams );
    return random_entry( selected_dream.messages() );
}

bool player::in_climate_control()
{
    bool regulated_area = false;
    // Check
    if( has_active_bionic( bio_climate ) ) {
        return true;
    }
    if( has_trait( trait_M_SKIN3 ) && g->m.has_flag_ter_or_furn( "FUNGUS", pos() ) &&
        in_sleep_state() ) {
        return true;
    }
    for( auto &w : worn ) {
        if( w.active && w.is_power_armor() ) {
            return true;
        }
        if( worn_with_flag( "CLIMATE_CONTROL" ) ) {
            return true;
        }
    }
    if( calendar::turn >= next_climate_control_check ) {
        // save CPU and simulate acclimation.
        next_climate_control_check = calendar::turn + 20_turns;
        if( const optional_vpart_position vp = g->m.veh_at( pos() ) ) {
            regulated_area = (
                                 vp->is_inside() &&  // Already checks for opened doors
                                 vp->vehicle().total_power_w( true ) > 0 // Out of gas? No AC for you!
                             );  // TODO: (?) Force player to scrounge together an AC unit
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

std::list<item *> player::get_radio_items()
{
    std::list<item *> rc_items;
    const invslice &stacks = inv.slice();
    for( auto &stack : stacks ) {
        item &stack_iter = stack->front();
        if( stack_iter.has_flag( "RADIO_ACTIVATION" ) ) {
            rc_items.push_back( &stack_iter );
        }
    }

    for( auto &elem : worn ) {
        if( elem.has_flag( "RADIO_ACTIVATION" ) ) {
            rc_items.push_back( &elem );
        }
    }

    if( is_armed() ) {
        if( weapon.has_flag( "RADIO_ACTIVATION" ) ) {
            rc_items.push_back( &weapon );
        }
    }
    return rc_items;
}

std::list<item *> player::get_artifact_items()
{
    std::list<item *> art_items;
    const invslice &stacks = inv.slice();
    for( auto &stack : stacks ) {
        item &stack_iter = stack->front();
        if( stack_iter.is_artifact() ) {
            art_items.push_back( &stack_iter );
        }
    }
    for( auto &elem : worn ) {
        if( elem.is_artifact() ) {
            art_items.push_back( &elem );
        }
    }
    if( is_armed() ) {
        if( weapon.is_artifact() ) {
            art_items.push_back( &weapon );
        }
    }
    return art_items;
}

/*
 * Calculate player brightness based on the brightest active item, as
 * per itype tag LIGHT_* and optional CHARGEDIM ( fade starting at 20% charge )
 * item.light.* is -unimplemented- for the moment, as it is a custom override for
 * applying light sources/arcs with specific angle and direction.
 */
float player::active_light() const
{
    float lumination = 0;

    int maxlum = 0;
    has_item_with( [&maxlum]( const item & it ) {
        const int lumit = it.getlight_emit();
        if( maxlum < lumit ) {
            maxlum = lumit;
        }
        return false; // continue search, otherwise has_item_with would cancel the search
    } );

    lumination = static_cast<float>( maxlum );

    if( lumination < 60 && has_active_bionic( bio_flashlight ) ) {
        lumination = 60;
    } else if( lumination < 25 && has_artifact_with( AEP_GLOW ) ) {
        lumination = 25;
    } else if( lumination < 5 && ( has_effect( effect_glowing ) ||
                                   ( has_active_bionic( bio_tattoo_led ) ||
                                     has_effect( effect_glowy_led ) ) ) ) {
        lumination = 5;
    }
    return lumination;
}

tripoint player::global_square_location() const
{
    return g->m.getabs( position );
}

tripoint player::global_sm_location() const
{
    return ms_to_sm_copy( global_square_location() );
}

tripoint player::global_omt_location() const
{
    return ms_to_omt_copy( global_square_location() );
}

const tripoint &player::pos() const
{
    return position;
}

int player::sight_range( int light_level ) const
{
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
    int range = static_cast<int>( -log( get_vision_threshold( static_cast<int>( g->m.ambient_light_at(
                                            pos() ) ) ) /
                                        static_cast<float>( light_level ) ) *
                                  ( 1.0 / LIGHT_TRANSPARENCY_OPEN_AIR ) );
    // int range = log(light_level * LIGHT_AMBIENT_LOW) / LIGHT_TRANSPARENCY_OPEN_AIR;

    // Clamp to [1, sight_max].
    return std::max( 1, std::min( range, sight_max ) );
}

int player::unimpaired_range() const
{
    return std::min( sight_max, 60 );
}

bool player::overmap_los( const tripoint &omt, int sight_points )
{
    const tripoint ompos = global_omt_location();
    if( omt.x < ompos.x - sight_points || omt.x > ompos.x + sight_points ||
        omt.y < ompos.y - sight_points || omt.y > ompos.y + sight_points ) {
        // Outside maximum sight range
        return false;
    }

    const std::vector<tripoint> line = line_to( ompos, omt, 0, 0 );
    for( size_t i = 0; i < line.size() && sight_points >= 0; i++ ) {
        const tripoint &pt = line[i];
        const oter_id &ter = overmap_buffer.ter( pt );
        sight_points -= static_cast<int>( ter->get_see_cost() );
        if( sight_points < 0 ) {
            return false;
        }
    }
    return true;
}

int player::overmap_sight_range( int light_level ) const
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
    sight += Character::mutation_value( "overmap_sight" );

    float multiplier = Character::mutation_value( "overmap_multiplier" );
    // Binoculars double your sight range.
    const bool has_optic = ( has_item_with_flag( "ZOOM" ) || has_bionic( bio_eye_optic ) ||
                             ( is_mounted() &&
                               mounted_creature->has_flag( MF_MECH_RECON_VISION ) ) );
    if( has_optic ) {
        multiplier += 1;
    }

    sight = round( sight * multiplier );
    return std::max( sight, 3 );
}

#define MAX_CLAIRVOYANCE 40
int player::clairvoyance() const
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

bool player::sight_impaired() const
{
    return ( ( ( has_effect( effect_boomered ) || has_effect( effect_no_sight ) ||
                 has_effect( effect_darkness ) ) &&
               ( !( has_trait( trait_PER_SLIME_OK ) ) ) ) ||
             ( underwater && !has_bionic( bio_membrane ) && !has_trait( trait_MEMBRANE ) &&
               !worn_with_flag( "SWIM_GOGGLES" ) && !has_trait( trait_PER_SLIME_OK ) &&
               !has_trait( trait_CEPH_EYES ) && !has_trait( trait_SEESLEEP ) ) ||
             ( ( has_trait( trait_MYOPIC ) || has_trait( trait_URSINE_EYE ) ) &&
               !worn_with_flag( "FIX_NEARSIGHT" ) &&
               !has_effect( effect_contacts ) &&
               !has_bionic( bio_eye_optic ) ) ||
             has_trait( trait_PER_SLIME ) );
}

bool player::avoid_trap( const tripoint &pos, const trap &tr ) const
{
    /** @EFFECT_DEX increases chance to avoid traps */

    /** @EFFECT_DODGE increases chance to avoid traps */
    int myroll = dice( 3, dex_cur + get_skill_level( skill_dodge ) * 1.5 );
    int traproll;
    if( tr.can_see( pos, *this ) ) {
        traproll = dice( 3, tr.get_avoidance() );
    } else {
        traproll = dice( 6, tr.get_avoidance() );
    }

    if( has_trait( trait_LIGHTSTEP ) ) {
        myroll += dice( 2, 6 );
    }

    if( has_trait( trait_CLUMSY ) ) {
        myroll -= dice( 2, 6 );
    }

    return myroll >= traproll;
}

bool player::has_alarm_clock() const
{
    return ( has_item_with_flag( "ALARMCLOCK", true ) ||
             ( g->m.veh_at( pos() ) &&
               !empty( g->m.veh_at( pos() )->vehicle().get_avail_parts( "ALARMCLOCK" ) ) ) ||
             has_bionic( bio_watch ) );
}

bool player::has_watch() const
{
    return ( has_item_with_flag( "WATCH", true ) ||
             ( g->m.veh_at( pos() ) &&
               !empty( g->m.veh_at( pos() )->vehicle().get_avail_parts( "WATCH" ) ) ) ||
             has_bionic( bio_watch ) );
}

void player::pause()
{
    moves = 0;
    recoil = MAX_RECOIL;

    // Train swimming if underwater
    if( !in_vehicle ) {
        if( underwater ) {
            practice( skill_swimming, 1 );
            drench( 100, { {
                    bp_leg_l, bp_leg_r, bp_torso, bp_arm_l,
                    bp_arm_r, bp_head, bp_eyes, bp_mouth,
                    bp_foot_l, bp_foot_r, bp_hand_l, bp_hand_r
                }
            }, true );
        } else if( g->m.has_flag( TFLAG_DEEP_WATER, pos() ) ) {
            practice( skill_swimming, 1 );
            // Same as above, except no head/eyes/mouth
            drench( 100, { {
                    bp_leg_l, bp_leg_r, bp_torso, bp_arm_l,
                    bp_arm_r, bp_foot_l, bp_foot_r, bp_hand_l,
                    bp_hand_r
                }
            }, true );
        } else if( g->m.has_flag( "SWIMMABLE", pos() ) ) {
            drench( 40, { { bp_foot_l, bp_foot_r, bp_leg_l, bp_leg_r } }, false );
        }
    }

    // Try to put out clothing/hair fire
    if( has_effect( effect_onfire ) ) {
        time_duration total_removed = 0_turns;
        time_duration total_left = 0_turns;
        bool on_ground = has_effect( effect_downed );
        for( const body_part bp : all_body_parts ) {
            effect &eff = get_effect( effect_onfire, bp );
            if( eff.is_null() ) {
                continue;
            }

            // TODO: Tools and skills
            total_left += eff.get_duration();
            // Being on the ground will smother the fire much faster because you can roll
            const time_duration dur_removed = on_ground ? eff.get_duration() / 2 + 2_turns : 1_turns;
            eff.mod_duration( -dur_removed );
            total_removed += dur_removed;
        }

        // Don't drop on the ground when the ground is on fire
        if( total_left > 1_minutes && !is_dangerous_fields( g->m.field_at( pos() ) ) ) {
            add_effect( effect_downed, 2_turns, num_bp, false, 0, true );
            add_msg_player_or_npc( m_warning,
                                   _( "You roll on the ground, trying to smother the fire!" ),
                                   _( "<npcname> rolls on the ground!" ) );
        } else if( total_removed > 0_turns ) {
            add_msg_player_or_npc( m_warning,
                                   _( "You attempt to put out the fire on you!" ),
                                   _( "<npcname> attempts to put out the fire on them!" ) );
        }
    }

    // on-pause effects for martial arts
    ma_onpause_effects();

    if( is_npc() ) {
        // The stuff below doesn't apply to NPCs
        // search_surroundings should eventually do, though
        return;
    }

    if( in_vehicle && one_in( 8 ) ) {
        VehicleList vehs = g->m.get_vehicles();
        vehicle *veh = nullptr;
        for( auto &v : vehs ) {
            veh = v.v;
            if( veh && veh->is_moving() && veh->player_in_control( *this ) ) {
                double exp_temp = 1 + veh->total_mass() / 400.0_kilogram +
                                  std::abs( veh->velocity / 3200.0 );
                int experience = static_cast<int>( exp_temp );
                if( exp_temp - experience > 0 && x_in_y( exp_temp - experience, 1.0 ) ) {
                    experience++;
                }
                practice( skill_id( "driving" ), experience );
                break;
            }
        }
    }

    search_surroundings();
}

void player::search_surroundings()
{
    if( controlling_vehicle ) {
        return;
    }
    // Search for traps in a larger area than before because this is the only
    // way we can "find" traps that aren't marked as visible.
    // Detection formula takes care of likelihood of seeing within this range.
    for( const tripoint &tp : g->m.points_in_radius( pos(), 5 ) ) {
        const trap &tr = g->m.tr_at( tp );
        if( tr.is_null() || tp == pos() ) {
            continue;
        }
        if( has_active_bionic( bio_ground_sonar ) && !knows_trap( tp ) &&
            ( tr.loadid == tr_beartrap_buried ||
              tr.loadid == tr_landmine_buried || tr.loadid == tr_sinkhole ) ) {
            const std::string direction = direction_name( direction_from( pos(), tp ) );
            add_msg_if_player( m_warning, _( "Your ground sonar detected a %1$s to the %2$s!" ),
                               tr.name(), direction );
            add_known_trap( tp, tr );
        }
        if( !sees( tp ) ) {
            continue;
        }
        if( tr.is_always_invisible() || tr.can_see( tp, *this ) ) {
            // Already seen, or can never be seen
            continue;
        }
        // Chance to detect traps we haven't yet seen.
        if( tr.detect_trap( tp, *this ) ) {
            if( tr.get_visibility() > 0 ) {
                // Only bug player about traps that aren't trivial to spot.
                const std::string direction = direction_name(
                                                  direction_from( pos(), tp ) );
                add_msg_if_player( _( "You've spotted a %1$s to the %2$s!" ),
                                   tr.name(), direction );
            }
            add_known_trap( tp, tr );
        }
    }
}

int player::read_speed( bool return_stat_effect ) const
{
    // Stat window shows stat effects on based on current stat
    const int intel = get_int();
    /** @EFFECT_INT increases reading speed */
    int ret = to_moves<int>( 1_minutes ) - to_moves<int>( 3_seconds ) * ( intel - 8 );

    if( has_bionic( afs_bio_linguistic_coprocessor ) ) { // Aftershock
        ret *= .85;
    }

    if( has_trait( trait_FASTREADER ) ) {
        ret *= .8;
    }

    if( has_trait( trait_PROF_DICEMASTER ) ) {
        ret *= .9;
    }

    if( has_trait( trait_SLOWREADER ) ) {
        ret *= 1.3;
    }

    if( ret < to_moves<int>( 1_seconds ) ) {
        ret = to_moves<int>( 1_seconds );
    }
    // return_stat_effect actually matters here
    return return_stat_effect ? ret : ret / 10;
}

int player::rust_rate( bool return_stat_effect ) const
{
    if( get_option<std::string>( "SKILL_RUST" ) == "off" ) {
        return 0;
    }

    // Stat window shows stat effects on based on current stat
    int intel = get_int();
    /** @EFFECT_INT reduces skill rust */
    int ret = ( ( get_option<std::string>( "SKILL_RUST" ) == "vanilla" ||
                  get_option<std::string>( "SKILL_RUST" ) == "capped" ) ? 500 : 500 - 35 * ( intel - 8 ) );

    ret *= mutation_value( "skill_rust_multiplier" );

    if( ret < 0 ) {
        ret = 0;
    }

    // return_stat_effect actually matters here
    return ( return_stat_effect ? ret : ret / 10 );
}

int player::talk_skill() const
{
    /** @EFFECT_INT slightly increases talking skill */

    /** @EFFECT_PER slightly increases talking skill */

    /** @EFFECT_SPEECH increases talking skill */
    int ret = get_int() + get_per() + get_skill_level( skill_id( "speech" ) ) * 3;
    return ret;
}

int player::intimidation() const
{
    /** @EFFECT_STR increases intimidation factor */
    int ret = get_str() * 2;
    if( weapon.is_gun() ) {
        ret += 10;
    }
    if( weapon.damage_melee( DT_BASH ) >= 12 ||
        weapon.damage_melee( DT_CUT ) >= 12 ||
        weapon.damage_melee( DT_STAB ) >= 12 ) {
        ret += 5;
    }

    if( stim > 20 ) {
        ret += 2;
    }
    if( has_effect( effect_drunk ) ) {
        ret -= 4;
    }

    return ret;
}

bool player::is_dead_state() const
{
    return hp_cur[hp_head] <= 0 || hp_cur[hp_torso] <= 0;
}

void player::on_dodge( Creature *source, float difficulty )
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

    ma_ondodge_effects();

    // For adjacent attackers check for techniques usable upon successful dodge
    if( source && square_dist( pos(), source->pos() ) == 1 ) {
        matec_id tec = pick_technique( *source, used_weapon(), false, true, false );

        if( tec != tec_none && !is_dead_state() ) {
            if( stamina < get_stamina_max() / 3 ) {
                add_msg( m_bad, _( "You try to counterattack but you are too exhausted!" ) );
            } else {
                melee_attack( *source, false, tec );
            }
        }
    }
}

void player::on_hit( Creature *source, body_part bp_hit,
                     float /*difficulty*/, dealt_projectile_attack const *const proj )
{
    check_dead_state();
    if( source == nullptr || proj != nullptr ) {
        return;
    }

    bool u_see = g->u.sees( *this );
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
        ods_shock_damage.add_damage( DT_ELECTRIC, shock * 5 );
        // Should hit body part used for attack
        source->deal_damage( this, bp_torso, ods_shock_damage );
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
        spine_damage.add_damage( DT_STAB, spine );
        source->deal_damage( this, bp_torso, spine_damage );
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
        thorn_damage.add_damage( DT_CUT, thorn );
        // In general, critters don't have separate limbs
        // so safer to target the torso
        source->deal_damage( this, bp_torso, thorn_damage );
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
    if( worn_with_flag( "REQUIRES_BALANCE" ) && !has_effect( effect_downed ) ) {
        int rolls = 4;
        if( worn_with_flag( "ROLLER_ONE" ) ) {
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
            add_effect( effect_downed, 2_turns );
        }
    }
    Character::on_hit( source, bp_hit, 0.0f, proj );
}

int player::get_lift_assist() const
{
    int result = 0;
    const std::vector<npc *> helpers = g->u.get_crafting_helpers();
    for( const npc *np : helpers ) {
        result += np->get_str();
    }
    return result;
}

int player::get_num_crafting_helpers( int max ) const
{
    std::vector<npc *> helpers = g->u.get_crafting_helpers();
    return std::min( max, static_cast<int>( helpers.size() ) );
}

bool player::immune_to( body_part bp, damage_unit dam ) const
{
    if( has_trait( trait_DEBUG_NODMG ) || is_immune_damage( dam.type ) ) {
        return true;
    }

    passive_absorb_hit( bp, dam );

    for( const item &cloth : worn ) {
        if( cloth.get_coverage() == 100 && cloth.covers( bp ) ) {
            cloth.mitigate_damage( dam );
        }
    }

    return dam.amount <= 0;
}

dealt_damage_instance player::deal_damage( Creature *source, body_part bp,
        const damage_instance &d )
{
    if( has_trait( trait_DEBUG_NODMG ) ) {
        return dealt_damage_instance();
    }

    //damage applied here
    dealt_damage_instance dealt_dams = Creature::deal_damage( source, bp, d );
    //block reduction should be by applied this point
    int dam = dealt_dams.total_damage();

    // TODO: Pre or post blit hit tile onto "this"'s location here
    if( dam > 0 && g->u.sees( pos() ) ) {
        g->draw_hit_player( *this, dam );

        if( is_player() && source ) {
            //monster hits player melee
            SCT.add( point( posx(), posy() ),
                     direction_from( point_zero, point( posx() - source->posx(), posy() - source->posy() ) ),
                     get_hp_bar( dam, get_hp_max( player::bp_to_hp( bp ) ) ).first, m_bad,
                     body_part_name( bp ), m_neutral );
        }
    }

    // handle snake artifacts
    if( has_artifact_with( AEP_SNAKES ) && dam >= 6 ) {
        const int snakes = dam / 6;
        int spawned = 0;
        for( int i = 0; i < snakes; i++ ) {
            if( monster *const snake = g->place_critter_around( mon_shadow_snake, pos(), 1 ) ) {
                snake->friendly = -1;
                spawned++;
            }
        }
        if( spawned == 1 ) {
            add_msg( m_warning, _( "A snake sprouts from your body!" ) );
        } else if( spawned >= 2 ) {
            add_msg( m_warning, _( "Some snakes sprout from your body!" ) );
        }
    }

    // And slimespawners too
    if( ( has_trait( trait_SLIMESPAWNER ) ) && ( dam >= 10 ) && one_in( 20 - dam ) ) {
        if( monster *const slime = g->place_critter_around( mon_player_blob, pos(), 1 ) ) {
            slime->friendly = -1;
            add_msg_if_player( m_warning, _( "Slime is torn from you, and moves on its own!" ) );
        }
    }

    //Acid blood effects.
    bool u_see = g->u.sees( *this );
    int cut_dam = dealt_dams.type_damage( DT_CUT );
    if( source && has_trait( trait_ACIDBLOOD ) && !one_in( 3 ) &&
        ( dam >= 4 || cut_dam > 0 ) && ( rl_dist( g->u.pos(), source->pos() ) <= 1 ) ) {
        if( is_player() ) {
            add_msg( m_good, _( "Your acidic blood splashes %s in mid-attack!" ),
                     source->disp_name() );
        } else if( u_see ) {
            add_msg( _( "%1$s's acidic blood splashes on %2$s in mid-attack!" ),
                     disp_name(), source->disp_name() );
        }
        damage_instance acidblood_damage;
        acidblood_damage.add_damage( DT_ACID, rng( 4, 16 ) );
        if( !one_in( 4 ) ) {
            source->deal_damage( this, bp_arm_l, acidblood_damage );
            source->deal_damage( this, bp_arm_r, acidblood_damage );
        } else {
            source->deal_damage( this, bp_torso, acidblood_damage );
            source->deal_damage( this, bp_head, acidblood_damage );
        }
    }

    int recoil_mul = 100;
    switch( bp ) {
        case bp_eyes:
            if( dam > 5 || cut_dam > 0 ) {
                const time_duration minblind = std::max( 1_turns, 1_turns * ( dam + cut_dam ) / 10 );
                const time_duration maxblind = std::min( 5_turns, 1_turns * ( dam + cut_dam ) / 4 );
                add_effect( effect_blind, rng( minblind, maxblind ) );
            }
            break;
        case bp_torso:
            break;
        case bp_hand_l: // Fall through to arms
        case bp_arm_l:
        // Hit to arms/hands are really bad to our aim
        case bp_hand_r: // Fall through to arms
        case bp_arm_r:
            recoil_mul = 200;
            break;
        case bp_foot_l: // Fall through to legs
        case bp_leg_l:
            break;
        case bp_foot_r: // Fall through to legs
        case bp_leg_r:
            break;
        case bp_mouth: // Fall through to head damage
        case bp_head:
            // TODO: Some daze maybe? Move drain?
            break;
        default:
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
            /** @EFFECT_DEX increases chance to avoid being grabbed, if DEX>STR */

            /** @EFFECT_STR increases chance to avoid being grabbed, if STR>DEX */
            if( has_grab_break_tec() && get_grab_resist() > 0 &&
                ( get_dex() > get_str() ? rng( 0, get_dex() ) : rng( 0, get_str() ) ) >
                rng( 0, 10 ) ) {
                if( has_effect( effect_grabbed ) ) {
                    add_msg_if_player( m_warning, _( "You are being grabbed by %s, but you bat it away!" ),
                                       source->disp_name() );
                } else {
                    add_msg_player_or_npc( m_info, _( "You are being grabbed by %s, but you break its grab!" ),
                                           _( "<npcname> are being grabbed by %s, but they break its grab!" ),
                                           source->disp_name() );
                }
            } else {
                int prev_effect = get_effect_int( effect_grabbed );
                add_effect( effect_grabbed, 2_turns, bp_torso, false, prev_effect + 2 );
                source->add_effect( effect_grabbing, 2_turns );
                add_msg_player_or_npc( m_bad, _( "You are grabbed by %s!" ), _( "<npcname> is grabbed by %s!" ),
                                       source->disp_name() );
            }
        }
    }

    if( get_option<bool>( "FILTHY_WOUNDS" ) ) {
        int sum_cover = 0;
        for( const item &i : worn ) {
            if( i.covers( bp ) && i.is_filthy() ) {
                sum_cover += i.get_coverage();
            }
        }

        // Chance of infection is damage (with cut and stab x4) * sum of coverage on affected body part, in percent.
        // i.e. if the body part has a sum of 100 coverage from filthy clothing,
        // each point of damage has a 1% change of causing infection.
        if( sum_cover > 0 ) {
            const int cut_type_dam = dealt_dams.type_damage( DT_CUT ) + dealt_dams.type_damage( DT_STAB );
            const int combined_dam = dealt_dams.type_damage( DT_BASH ) + ( cut_type_dam * 4 );
            const int infection_chance = ( combined_dam * sum_cover ) / 100;
            if( x_in_y( infection_chance, 100 ) ) {
                if( has_effect( effect_bite, bp ) ) {
                    add_effect( effect_bite, 40_minutes, bp, true );
                } else if( has_effect( effect_infected, bp ) ) {
                    add_effect( effect_infected, 25_minutes, bp, true );
                } else {
                    add_effect( effect_bite, 1_turns, bp, true );
                }
                add_msg_if_player( _( "Filth from your clothing has implanted deep in the wound." ) );
            }
        }
    }

    on_hurt( source );
    return dealt_dams;
}

void player::mod_pain( int npain )
{
    if( npain > 0 ) {
        if( has_trait( trait_NOPAIN ) || has_effect( effect_narcosis ) ) {
            return;
        }
        // always increase pain gained by one from these bad mutations
        if( has_trait( trait_MOREPAIN ) ) {
            npain += std::max( 1, roll_remainder( npain * 0.25 ) );
        } else if( has_trait( trait_MOREPAIN2 ) ) {
            npain += std::max( 1, roll_remainder( npain * 0.5 ) );
        } else if( has_trait( trait_MOREPAIN3 ) ) {
            npain += std::max( 1, roll_remainder( npain * 1.0 ) );
        }

        if( npain > 1 ) {
            // if it's 1 it'll just become 0, which is bad
            if( has_trait( trait_PAINRESIST_TROGLO ) ) {
                npain = roll_remainder( npain * 0.5 );
            } else if( has_trait( trait_PAINRESIST ) ) {
                npain = roll_remainder( npain * 0.67 );
            }
        }
    }
    Creature::mod_pain( npain );
}

void player::set_pain( int npain )
{
    const int prev_pain = get_perceived_pain();
    Creature::set_pain( npain );
    const int cur_pain = get_perceived_pain();

    if( cur_pain != prev_pain ) {
        react_to_felt_pain( cur_pain - prev_pain );
        on_stat_change( "perceived_pain", cur_pain );
    }
}

int player::get_perceived_pain() const
{
    if( get_effect_int( effect_adrenaline ) > 1 ) {
        return 0;
    }

    return std::max( get_pain() - get_painkiller(), 0 );
}

void player::mod_painkiller( int npkill )
{
    set_painkiller( pkill + npkill );
}

void player::set_painkiller( int npkill )
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

int player::get_painkiller() const
{
    return pkill;
}

void player::react_to_felt_pain( int intensity )
{
    if( intensity <= 0 ) {
        return;
    }
    if( is_player() && intensity >= 2 ) {
        g->cancel_activity_or_ignore_query( distraction_type::pain,  _( "Ouch, something hurts!" ) );
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

int player::reduce_healing_effect( const efftype_id &eff_id, int remove_med, body_part hurt )
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
            add_msg_if_player( m_bad, _( "Bandages on your %s were destroyed!" ), body_part_name( hurt ) );
        } else  if( eff_id == effect_disinfected ) {
            add_msg_if_player( m_bad, _( "Your %s is no longer disinfected!" ), body_part_name( hurt ) );
        }
    }
    e.mod_duration( -6_hours * remove_med );
    return intensity;
}

/*
    Where damage to player is actually applied to hit body parts
    Might be where to put bleed stuff rather than in player::deal_damage()
 */
void player::apply_damage( Creature *source, body_part hurt, int dam, const bool bypass_med )
{
    if( is_dead_state() || has_trait( trait_DEBUG_NODMG ) ) {
        // don't do any more damage if we're already dead
        // Or if we're debugging and don't want to die
        return;
    }

    hp_part hurtpart = bp_to_hp( hurt );
    if( hurtpart == num_hp_parts ) {
        debugmsg( "Wacky body part hurt!" );
        hurtpart = hp_torso;
    }

    mod_pain( dam / 2 );

    const int dam_to_bodypart = std::min( dam, hp_cur[hurtpart] );

    hp_cur[hurtpart] -= dam_to_bodypart;
    g->events().send<event_type::character_takes_damage>( getID(), dam_to_bodypart );

    if( hp_cur[hurtpart] <= 0 && ( source == nullptr || !source->is_hallucination() ) ) {
        if( !can_wield( weapon ).success() ) {
            put_into_vehicle_or_drop( *this, item_drop_reason::tumbling, { weapon } );
        }
        if( has_effect( effect_mending, hurt ) ) {
            effect &e = get_effect( effect_mending, hurt );
            float remove_mend = dam / 20.0f;
            e.mod_duration( -e.get_max_duration() * remove_mend );
        }
    }

    if( dam > get_painkiller() ) {
        on_hurt( source );
    }

    if( !bypass_med ) {
        // remove healing effects if damaged
        int remove_med = roll_remainder( dam / 5.0f );
        if( remove_med > 0 && has_effect( effect_bandaged, hurt ) ) {
            remove_med -= reduce_healing_effect( effect_bandaged, remove_med, hurt );
        }
        if( remove_med > 0 && has_effect( effect_disinfected, hurt ) ) {
            // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
            remove_med -= reduce_healing_effect( effect_disinfected, remove_med, hurt );
        }
    }
}

float player::fall_damage_mod() const
{
    if( has_effect_with_flag( "EFFECT_FEATHER_FALL" ) ) {
        return 0.0f;
    }
    float ret = 1.0f;

    // Ability to land properly is 2x as important as dexterity itself
    /** @EFFECT_DEX decreases damage from falling */

    /** @EFFECT_DODGE decreases damage from falling */
    float dex_dodge = dex_cur / 2.0 + get_skill_level( skill_dodge );
    // Penalize for wearing heavy stuff
    const float average_leg_encumb = ( encumb( bp_leg_l ) + encumb( bp_leg_r ) ) / 2.0;
    dex_dodge -= ( average_leg_encumb + encumb( bp_torso ) ) / 10;
    // But prevent it from increasing damage
    dex_dodge = std::max( 0.0f, dex_dodge );
    // 100% damage at 0, 75% at 10, 50% at 20 and so on
    ret *= ( 100.0f - ( dex_dodge * 4.0f ) ) / 100.0f;

    if( has_trait( trait_PARKOUR ) ) {
        ret *= 2.0f / 3.0f;
    }

    // TODO: Bonus for Judo, mutations. Penalty for heavy weight (including mutations)
    return std::max( 0.0f, ret );
}

// force is maximum damage to hp before scaling
int player::impact( const int force, const tripoint &p )
{
    // Falls over ~30m are fatal more often than not
    // But that would be quite a lot considering 21 z-levels in game
    // so let's assume 1 z-level is comparable to 30 force

    if( force <= 0 ) {
        return force;
    }

    // Damage modifier (post armor)
    float mod = 1.0f;
    int effective_force = force;
    int cut = 0;
    // Percentage armor penetration - armor won't help much here
    // TODO: Make cushioned items like bike helmets help more
    float armor_eff = 1.0f;
    // Shock Absorber CBM heavily reduces damage
    const bool shock_absorbers = has_active_bionic( bionic_id( "bio_shock_absorber" ) );

    // Being slammed against things rather than landing means we can't
    // control the impact as well
    const bool slam = p != pos();
    std::string target_name = "a swarm of bugs";
    Creature *critter = g->critter_at( p );
    if( critter != this && critter != nullptr ) {
        target_name = critter->disp_name();
        // Slamming into creatures and NPCs
        // TODO: Handle spikes/horns and hard materials
        armor_eff = 0.5f; // 2x as much as with the ground
        // TODO: Modify based on something?
        mod = 1.0f;
        effective_force = force;
    } else if( const optional_vpart_position vp = g->m.veh_at( p ) ) {
        // Slamming into vehicles
        // TODO: Integrate it with vehicle collision function somehow
        target_name = vp->vehicle().disp_name();
        if( vp.part_with_feature( "SHARP", true ) ) {
            // Now we're actually getting impaled
            cut = force; // Lots of fun
        }

        mod = slam ? 1.0f : fall_damage_mod();
        armor_eff = 0.25f; // Not much
        if( !slam && vp->part_with_feature( "ROOF", true ) ) {
            // Roof offers better landing than frame or pavement
            effective_force /= 2; // TODO: Make this not happen with heavy duty/plated roof
        }
    } else {
        // Slamming into terrain/furniture
        target_name = g->m.disp_name( p );
        int hard_ground = g->m.has_flag( TFLAG_DIGGABLE, p ) ? 0 : 3;
        armor_eff = 0.25f; // Not much
        // Get cut by stuff
        // This isn't impalement on metal wreckage, more like flying through a closed window
        cut = g->m.has_flag( TFLAG_SHARP, p ) ? 5 : 0;
        effective_force = force + hard_ground;
        mod = slam ? 1.0f : fall_damage_mod();
        if( g->m.has_furn( p ) ) {
            // TODO: Make furniture matter
        } else if( g->m.has_flag( TFLAG_SWIMMABLE, p ) ) {
            // TODO: Some formula of swimming
            effective_force /= 4;
        }
    }

    // Rescale for huge force
    // At >30 force, proper landing is impossible and armor helps way less
    if( effective_force > 30 ) {
        // Armor simply helps way less
        armor_eff *= 30.0f / effective_force;
        if( mod < 1.0f ) {
            // Everything past 30 damage gets a worse modifier
            const float scaled_mod = std::pow( mod, 30.0f / effective_force );
            const float scaled_damage = ( 30.0f * mod ) + scaled_mod * ( effective_force - 30.0f );
            mod = scaled_damage / effective_force;
        }
    }

    if( !slam && mod < 1.0f && mod * force < 5 ) {
        // Perfect landing, no damage (regardless of armor)
        add_msg_if_player( m_warning, _( "You land on %s." ), target_name );
        return 0;
    }

    // Shock absorbers kick in only when they need to, so if our other protections fail, fall back on them
    if( shock_absorbers ) {
        effective_force -= 15; // Provide a flat reduction to force
        if( mod > 0.25f ) {
            mod = 0.25f; // And provide a 75% reduction against that force if we don't have it already
        }
        if( effective_force < 0 ) {
            effective_force = 0;
        }
    }

    int total_dealt = 0;
    if( mod * effective_force >= 5 ) {
        for( int i = 0; i < num_hp_parts; i++ ) {
            const body_part bp = hp_to_bp( static_cast<hp_part>( i ) );
            const int bash = effective_force * rng( 60, 100 ) / 100;
            damage_instance di;
            di.add_damage( DT_BASH, bash, 0, armor_eff, mod );
            // No good way to land on sharp stuff, so here modifier == 1.0f
            di.add_damage( DT_CUT, cut, 0, armor_eff, 1.0f );
            total_dealt += deal_damage( nullptr, bp, di ).total_damage();
        }
    }

    if( total_dealt > 0 && is_player() ) {
        // "You slam against the dirt" is fine
        add_msg( m_bad, _( "You are slammed against %1$s for %2$d damage." ),
                 target_name, total_dealt );
    } else if( is_player() && shock_absorbers ) {
        add_msg( m_bad, _( "You are slammed against %s!" ),
                 target_name, total_dealt );
        add_msg( m_good, _( "...but your shock absorbers negate the damage!" ) );
    } else if( slam ) {
        // Only print this line if it is a slam and not a landing
        // Non-players should only get this one: player doesn't know how much damage was dealt
        // and landing messages for each slammed creature would be too much
        add_msg_player_or_npc( m_bad,
                               _( "You are slammed against %s." ),
                               _( "<npcname> is slammed against %s." ),
                               target_name );
    } else {
        // No landing message for NPCs
        add_msg_if_player( m_warning, _( "You land on %s." ), target_name );
    }

    if( x_in_y( mod, 1.0f ) ) {
        add_effect( effect_downed, rng( 1_turns, 1_turns + mod * 3_turns ) );
    }

    return total_dealt;
}

void player::knock_back_to( const tripoint &to )
{
    if( to == pos() ) {
        return;
    }

    // First, see if we hit a monster
    if( monster *const critter = g->critter_at<monster>( to ) ) {
        deal_damage( critter, bp_torso, damage_instance( DT_BASH, critter->type->size ) );
        add_effect( effect_stunned, 1_turns );
        /** @EFFECT_STR_MAX allows knocked back player to knock back, damage, stun some monsters */
        if( ( str_max - 6 ) / 4 > critter->type->size ) {
            critter->knock_back_from( pos() ); // Chain reaction!
            critter->apply_damage( this, bp_torso, ( str_max - 6 ) / 4 );
            critter->add_effect( effect_stunned, 1_turns );
        } else if( ( str_max - 6 ) / 4 == critter->type->size ) {
            critter->apply_damage( this, bp_torso, ( str_max - 6 ) / 4 );
            critter->add_effect( effect_stunned, 1_turns );
        }
        critter->check_dead_state();

        add_msg_player_or_npc( _( "You bounce off a %s!" ), _( "<npcname> bounces off a %s!" ),
                               critter->name() );
        return;
    }

    if( npc *const np = g->critter_at<npc>( to ) ) {
        deal_damage( np, bp_torso, damage_instance( DT_BASH, np->get_size() ) );
        add_effect( effect_stunned, 1_turns );
        np->deal_damage( this, bp_torso, damage_instance( DT_BASH, 3 ) );
        add_msg_player_or_npc( _( "You bounce off %s!" ), _( "<npcname> bounces off %s!" ),
                               np->name );
        np->check_dead_state();
        return;
    }

    // If we're still in the function at this point, we're actually moving a tile!
    if( g->m.has_flag( "LIQUID", to ) && g->m.has_flag( TFLAG_DEEP_WATER, to ) ) {
        if( !is_npc() ) {
            avatar_action::swim( g->m, g->u, to );
        }
        // TODO: NPCs can't swim!
    } else if( g->m.impassable( to ) ) { // Wait, it's a wall

        // It's some kind of wall.
        apply_damage( nullptr, bp_torso,
                      3 ); // TODO: who knocked us back? Maybe that creature should be the source of the damage?
        add_effect( effect_stunned, 2_turns );
        add_msg_player_or_npc( _( "You bounce off a %s!" ), _( "<npcname> bounces off a %s!" ),
                               g->m.obstacle_name( to ) );

    } else { // It's no wall
        setpos( to );
    }
}

int player::hp_percentage() const
{
    int total_cur = 0;
    int total_max = 0;
    // Head and torso HP are weighted 3x and 2x, respectively
    total_cur = hp_cur[hp_head] * 3 + hp_cur[hp_torso] * 2;
    total_max = hp_max[hp_head] * 3 + hp_max[hp_torso] * 2;
    for( int i = hp_arm_l; i < num_hp_parts; i++ ) {
        total_cur += hp_cur[i];
        total_max += hp_max[i];
    }

    return ( 100 * total_cur ) / total_max;
}

// Returns the number of multiples of tick_length we would "pass" on our way `from` to `to`
// For example, if `tick_length` is 1 hour, then going from 0:59 to 1:01 should return 1
inline int ticks_between( const time_point &from, const time_point &to,
                          const time_duration &tick_length )
{
    return ( to_turn<int>( to ) / to_turns<int>( tick_length ) ) - ( to_turn<int>
            ( from ) / to_turns<int>( tick_length ) );
}

void player::update_body()
{
    update_body( calendar::turn - 1_turns, calendar::turn );
}

void player::update_body( const time_point &from, const time_point &to )
{
    update_stamina( to_turns<int>( to - from ) );
    update_stomach( from, to );
    recalculate_enchantment_cache();
    if( ticks_between( from, to, 3_minutes ) > 0 ) {
        magic.update_mana( *this, to_turns<float>( 3_minutes ) );
    }
    const int five_mins = ticks_between( from, to, 5_minutes );
    if( five_mins > 0 ) {
        check_needs_extremes();
        update_needs( five_mins );
        regen( five_mins );
        // Note: mend ticks once per 5 minutes, but wants rate in TURNS, not 5 minute intervals
        //@todo change @ref med to take time_duration
        mend( five_mins * to_turns<int>( 5_minutes ) );
    }
    if( ticks_between( from, to, 24_hours ) > 0 ) {
        enforce_minimum_healing();
    }

    const int thirty_mins = ticks_between( from, to, 30_minutes );
    if( thirty_mins > 0 ) {
        if( activity.is_null() ) {
            reset_activity_level();
        }
        // Radiation kills health even at low doses
        update_health( has_trait( trait_RADIOGENIC ) ? 0 : -radiation );
        get_sick();
    }

    for( const auto &v : vitamin::all() ) {
        const time_duration rate = vitamin_rate( v.first );
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
}

void player::update_stomach( const time_point &from, const time_point &to )
{
    const needs_rates rates = calc_needs_rates();
    // No food/thirst/fatigue clock at all
    const bool debug_ls = has_trait( trait_DEBUG_LS );
    // No food/thirst, capped fatigue clock (only up to tired)
    const bool npc_no_food = is_npc() && get_option<bool>( "NO_NPC_FOOD" );
    const bool foodless = debug_ls || npc_no_food;
    const bool mouse = has_trait( trait_NO_THIRST );
    const bool mycus = has_trait( trait_M_DEPENDENT );
    const float kcal_per_time = get_bmr() / ( 12.0f * 24.0f );
    const int five_mins = ticks_between( from, to, 5_minutes );

    if( five_mins > 0 ) {
        stomach.absorb_water( *this, 250_ml * five_mins );
        guts.absorb_water( *this, 250_ml * five_mins );
    }
    if( ticks_between( from, to, 30_minutes ) > 0 ) {
        // the stomach does not currently have rates of absorption, but this is where it goes
        stomach.calculate_absorbed( stomach.get_absorb_rates( true, rates ) );
        guts.calculate_absorbed( guts.get_absorb_rates( false, rates ) );
        stomach.store_absorbed( *this );
        guts.store_absorbed( *this );
        guts.bowel_movement( guts.get_pass_rates( false ) );
        stomach.bowel_movement( stomach.get_pass_rates( true ), guts );
    }
    if( stomach.time_since_ate() > 10_minutes ) {
        if( stomach.contains() >= stomach.capacity() && get_hunger() > -61 ) {
            // you're engorged! your stomach is full to bursting!
            set_hunger( -61 );
        } else if( stomach.contains() >= stomach.capacity() / 2 && get_hunger() > -21 ) {
            // sated
            set_hunger( -21 );
        } else if( stomach.contains() >= stomach.capacity() / 8 && get_hunger() > -1 ) {
            // that's really all the food you need to feel full
            set_hunger( -1 );
        } else if( stomach.contains() == 0_ml ) {
            if( guts.get_calories() == 0 && guts.get_calories_absorbed() == 0 &&
                get_stored_kcal() < get_healthy_kcal() && get_hunger() < 300 ) {
                // there's no food except what you have stored in fat
                set_hunger( 300 );
            } else if( get_hunger() < 100 && ( ( guts.get_calories() == 0 &&
                                                 guts.get_calories_absorbed() == 0 &&
                                                 get_stored_kcal() >= get_healthy_kcal() ) || get_stored_kcal() < get_healthy_kcal() ) ) {
                set_hunger( 100 );
            } else if( get_hunger() < 0 ) {
                set_hunger( 0 );
            }
        }
        if( !foodless && rates.hunger > 0.0f ) {
            mod_hunger( roll_remainder( rates.hunger * five_mins ) );
            // instead of hunger keeping track of how you're living, burn calories instead
            mod_stored_kcal( -roll_remainder( five_mins * kcal_per_time ) );
        }
    } else
        // you fill up when you eat fast, but less so than if you eat slow
        // if you just ate but your stomach is still empty it will still
        // delay your filling up (drugs?)
    {
        if( stomach.contains() >= stomach.capacity() && get_hunger() > -61 ) {
            // you're engorged! your stomach is full to bursting!
            set_hunger( -61 );
        } else if( stomach.contains() >= stomach.capacity() * 3 / 4 && get_hunger() > -21 ) {
            // sated
            set_hunger( -21 );
        } else if( stomach.contains() >= stomach.capacity() / 2 && get_hunger() > -1 ) {
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
    if( mycus || mouse ) {
        set_thirst( 0 );
    }
}

void player::get_sick()
{
    // NPCs are too dumb to handle infections now
    if( is_npc() || has_trait( trait_DISIMMUNE ) ) {
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
    add_msg( m_debug, "disease_rarity = %d", disease_rarity );
    if( one_in( disease_rarity ) ) {
        if( one_in( 6 ) ) {
            // The flu typically lasts 3-10 days.
            add_env_effect( effect_flu, bp_mouth, 3, rng( 3_days, 10_days ) );
        } else {
            // A cold typically lasts 1-14 days.
            add_env_effect( effect_common_cold, bp_mouth, 3, rng( 1_days, 14_days ) );
        }
    }
}

void player::check_needs_extremes()
{
    // Check if we've overdosed... in any deadly way.
    if( stim > 250 ) {
        add_msg_if_player( m_bad, _( "You have a sudden heart attack!" ) );
        g->events().send<event_type::dies_from_drug_overdose>( getID(), efftype_id() );
        hp_cur[hp_torso] = 0;
    } else if( stim < -200 || get_painkiller() > 240 ) {
        add_msg_if_player( m_bad, _( "Your breathing stops completely." ) );
        g->events().send<event_type::dies_from_drug_overdose>( getID(), efftype_id() );
        hp_cur[hp_torso] = 0;
    } else if( has_effect( effect_jetinjector ) && get_effect_dur( effect_jetinjector ) > 40_minutes ) {
        if( !( has_trait( trait_NOPAIN ) ) ) {
            add_msg_if_player( m_bad, _( "Your heart spasms painfully and stops." ) );
        } else {
            add_msg_if_player( _( "Your heart spasms and stops." ) );
        }
        g->events().send<event_type::dies_from_drug_overdose>( getID(), effect_jetinjector );
        hp_cur[hp_torso] = 0;
    } else if( get_effect_dur( effect_adrenaline ) > 50_minutes ) {
        add_msg_if_player( m_bad, _( "Your heart spasms and stops." ) );
        g->events().send<event_type::dies_from_drug_overdose>( getID(), effect_adrenaline );
        hp_cur[hp_torso] = 0;
    } else if( get_effect_int( effect_drunk ) > 4 ) {
        add_msg_if_player( m_bad, _( "Your breathing slows down to a stop." ) );
        g->events().send<event_type::dies_from_drug_overdose>( getID(), effect_drunk );
        hp_cur[hp_torso] = 0;
    }

    // check if we've starved
    if( is_player() ) {
        if( get_stored_kcal() <= 0 ) {
            add_msg_if_player( m_bad, _( "You have starved to death." ) );
            g->events().send<event_type::dies_of_starvation>( getID() );
            hp_cur[hp_torso] = 0;
        } else {
            if( calendar::once_every( 1_hours ) ) {
                std::string message;
                if( stomach.contains() <= stomach.capacity() / 4 ) {
                    if( get_kcal_percent() < 0.1f ) {
                        message = _( "Food..." );
                    } else if( get_kcal_percent() < 0.25f ) {
                        message = _( "Due to insufficient nutrition, your body is suffering from starvation." );
                    } else if( get_kcal_percent() < 0.5f ) {
                        message = _( "Despite having something in your stomach, you still feel like you haven't eaten in days..." );
                    } else if( get_kcal_percent() < 0.8f ) {
                        message = _( "Your stomach feels so empty..." );
                    }
                } else {
                    if( get_kcal_percent() < 0.1f ) {
                        message = _( "Food..." );
                    } else if( get_kcal_percent() < 0.25f ) {
                        message = _( "You are EMACIATED!" );
                    } else if( get_kcal_percent() < 0.5f ) {
                        message = _( "You feel weak due to malnutrition." );
                    } else if( get_kcal_percent() < 0.8f ) {
                        message = _( "You feel that your body needs more nutritious food." );
                    }
                }
                add_msg_if_player( m_warning, message );
            }
        }
    }

    // Check if we're dying of thirst
    if( is_player() && get_thirst() >= 600 && ( stomach.get_water() == 0_ml ||
            guts.get_water() == 0_ml ) ) {
        if( get_thirst() >= 1200 ) {
            add_msg_if_player( m_bad, _( "You have died of dehydration." ) );
            g->events().send<event_type::dies_of_thirst>( getID() );
            hp_cur[hp_torso] = 0;
        } else if( get_thirst() >= 1000 && calendar::once_every( 30_minutes ) ) {
            add_msg_if_player( m_warning, _( "Even your eyes feel dry..." ) );
        } else if( get_thirst() >= 800 && calendar::once_every( 30_minutes ) ) {
            add_msg_if_player( m_warning, _( "You are THIRSTY!" ) );
        } else if( calendar::once_every( 30_minutes ) ) {
            add_msg_if_player( m_warning, _( "Your mouth feels so dry..." ) );
        }
    }

    // Check if we're falling asleep, unless we're sleeping
    if( get_fatigue() >= EXHAUSTED + 25 && !in_sleep_state() ) {
        if( get_fatigue() >= MASSIVE_FATIGUE ) {
            add_msg_if_player( m_bad, _( "Survivor sleep now." ) );
            g->events().send<event_type::falls_asleep_from_exhaustion>( getID() );
            mod_fatigue( -10 );
            fall_asleep();
        } else if( get_fatigue() >= 800 && calendar::once_every( 30_minutes ) ) {
            add_msg_if_player( m_warning, _( "Anywhere would be a good place to sleep..." ) );
        } else if( calendar::once_every( 30_minutes ) ) {
            add_msg_if_player( m_warning, _( "You feel like you haven't slept in days." ) );
        }
    }

    // Even if we're not Exhausted, we really should be feeling lack/sleep earlier
    // Penalties start at Dead Tired and go from there
    if( get_fatigue() >= DEAD_TIRED && !in_sleep_state() ) {
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
        } else if( get_fatigue() >= EXHAUSTED ) {
            if( calendar::once_every( 30_minutes ) ) {
                add_msg_if_player( m_warning, _( "How much longer until bedtime?" ) );
                add_effect( effect_lack_sleep, 30_minutes + 1_turns );
            }
            /** @EFFECT_INT slightly decreases occurrence of short naps when exhausted */
            if( one_in( 100 + int_cur ) ) {
                fall_asleep( 30_seconds );
            }
        } else if( get_fatigue() >= DEAD_TIRED && calendar::once_every( 30_minutes ) ) {
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
                                   _( "Your mind feels foggy from lack of good sleep, and your eyes keep trying to close against your will." ) );
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
                                   _( "You haven't slept decently for so long that your whole body is screaming for mercy.  It's a miracle that you're still awake, but it just feels like a curse now." ) );
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
            bool can_pass_out = ( stim < 30 && sleep_deprivation >= SLEEP_DEPRIVATION_MINOR ) ||
                                sleep_deprivation >= SLEEP_DEPRIVATION_MAJOR;

            if( can_pass_out && calendar::once_every( 10_minutes ) ) {
                /** @EFFECT_PER slightly increases resilience against passing out from sleep deprivation */
                if( one_in( static_cast<int>( ( 1 - sleep_deprivation_pct ) * 100 ) + per_cur ) ||
                    sleep_deprivation >= SLEEP_DEPRIVATION_MASSIVE ) {
                    add_msg_player_or_npc( m_bad,
                                           _( "Your body collapses due to sleep deprivation, your neglected fatigue rushing back all at once, and you pass out on the spot." )
                                           , _( "<npcname> collapses to the ground from exhaustion." ) ) ;
                    if( get_fatigue() < EXHAUSTED ) {
                        set_fatigue( EXHAUSTED );
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

needs_rates player::calc_needs_rates()
{
    effect &sleep = get_effect( effect_sleep );
    const bool has_recycler = has_bionic( bio_recycler );
    const bool asleep = !sleep.is_null();

    needs_rates rates;
    rates.hunger = metabolic_rate();

    // TODO: this is where calculating basal metabolic rate, in kcal per day would go
    rates.kcal = 2500.0;

    add_msg_if_player( m_debug, "Metabolic rate: %.2f", rates.hunger );

    rates.thirst = get_option< float >( "PLAYER_THIRST_RATE" );
    rates.thirst *= 1.0f +  mutation_value( "thirst_modifier" );
    if( worn_with_flag( "SLOWS_THIRST" ) ) {
        rates.thirst *= 0.7f;
    }

    rates.fatigue = get_option< float >( "PLAYER_FATIGUE_RATE" );
    rates.fatigue *= 1.0f + mutation_value( "fatigue_modifier" );

    // Note: intentionally not in metabolic rate
    if( has_recycler ) {
        // Recycler won't help much with mutant metabolism - it is intended for human one
        rates.hunger = std::min( rates.hunger, std::max( 0.5f, rates.hunger - 0.5f ) );
        rates.thirst = std::min( rates.thirst, std::max( 0.5f, rates.thirst - 0.5f ) );
    }

    if( asleep ) {
        rates.recovery = 1.0f + mutation_value( "fatigue_regen_modifier" );
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

    if( has_activity( activity_id( "ACT_TREE_COMMUNION" ) ) ) {
        // Much of the body's needs are taken care of by the trees.
        // Hair Roots dont provide any bodily needs.
        if( has_trait( trait_ROOTS2 ) || has_trait( trait_ROOTS3 ) ) {
            rates.hunger *= 0.5f;
            rates.thirst *= 0.5f;
            rates.fatigue *= 0.5f;
        }
    }

    if( has_trait( trait_TRANSPIRATION ) ) {
        // Transpiration, the act of moving nutrients with evaporating water, can take a very heavy toll on your thirst when it's really hot.
        rates.thirst *= ( ( g->weather.get_temperature( pos() ) - 32.5f ) / 40.0f );
    }

    if( is_npc() ) {
        rates.hunger *= 0.25f;
        rates.thirst *= 0.25f;
    }

    return rates;
}

void player::update_needs( int rate_multiplier )
{
    // Hunger, thirst, & fatigue up every 5 minutes
    effect &sleep = get_effect( effect_sleep );
    // No food/thirst/fatigue clock at all
    const bool debug_ls = has_trait( trait_DEBUG_LS );
    // No food/thirst, capped fatigue clock (only up to tired)
    const bool npc_no_food = is_npc() && get_option<bool>( "NO_NPC_FOOD" );
    const bool asleep = !sleep.is_null();
    const bool lying = asleep || has_effect( effect_lying_down ) ||
                       activity.id() == "ACT_TRY_SLEEP";

    needs_rates rates = calc_needs_rates();

    const bool wasnt_fatigued = get_fatigue() <= DEAD_TIRED;
    // Don't increase fatigue if sleeping or trying to sleep or if we're at the cap.
    if( get_fatigue() < 1050 && !asleep && !debug_ls ) {
        if( rates.fatigue > 0.0f ) {
            int fatigue_roll = roll_remainder( rates.fatigue * rate_multiplier );
            mod_fatigue( fatigue_roll );

            if( get_option< bool >( "SLEEP_DEPRIVATION" ) ) {
                // Synaptic regen bionic stops SD while awake and boosts it while sleeping
                if( !has_active_bionic( bio_synaptic_regen ) ) {
                    // fatigue_roll should be around 1 - so the counter increases by 1 every minute on average,
                    // but characters who need less sleep will also get less sleep deprived, and vice-versa.

                    // Note: Since needs are updated in 5-minute increments, we have to multiply the roll again by
                    // 5. If rate_multiplier is > 1, fatigue_roll will be higher and this will work out.
                    mod_sleep_deprivation( fatigue_roll * 5 );
                }
            }

            if( npc_no_food && get_fatigue() > TIRED ) {
                set_fatigue( TIRED );
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
                mod_fatigue( -recovered );
                if( get_option< bool >( "SLEEP_DEPRIVATION" ) ) {
                    // Sleeping on the ground, no bionic = 1x rest_modifier
                    // Sleeping on a bed, no bionic      = 2x rest_modifier
                    // Sleeping on a comfy bed, no bionic= 3x rest_modifier

                    // Sleeping on the ground, bionic    = 3x rest_modifier
                    // Sleeping on a bed, bionic         = 6x rest_modifier
                    // Sleeping on a comfy bed, bionic   = 9x rest_modifier
                    float rest_modifier = ( has_active_bionic( bio_synaptic_regen ) ? 3 : 1 );
                    // Magnesium supplements also add a flat bonus to recovery speed
                    if( has_effect( effect_magnesium_supplements ) ) {
                        rest_modifier += 1;
                    }

                    comfort_level comfort = base_comfort_value( pos() );

                    if( comfort >= comfort_level::very_comfortable ) {
                        rest_modifier *= 3;
                    } else  if( comfort >= comfort_level::comfortable ) {
                        rest_modifier *= 2.5;
                    } else if( comfort >= comfort_level::slightly_comfortable ) {
                        rest_modifier *= 2;
                    }

                    // If we're just tired, we'll get a decent boost to our sleep quality.
                    // The opposite is true for very tired characters.
                    if( get_fatigue() < DEAD_TIRED ) {
                        rest_modifier += 2;
                    } else if( get_fatigue() >= EXHAUSTED ) {
                        rest_modifier = ( rest_modifier > 2 ) ? rest_modifier - 2 : 1;
                    }

                    // Recovered is multiplied by 2 as well, since we spend 1/3 of the day sleeping
                    mod_sleep_deprivation( -rest_modifier * ( recovered * 2 ) );
                }
            }
        }
    }
    if( is_player() && wasnt_fatigued && get_fatigue() > DEAD_TIRED && !lying ) {
        if( !activity ) {
            add_msg_if_player( m_warning, _( "You're feeling tired.  %s to lie down for sleep." ),
                               press_x( ACTION_SLEEP ) );
        } else {
            g->cancel_activity_query( _( "You're feeling tired." ) );
        }
    }

    if( stim < 0 ) {
        stim = std::min( stim + rate_multiplier, 0 );
    } else if( stim > 0 ) {
        stim = std::max( stim - rate_multiplier, 0 );
    }

    if( get_painkiller() > 0 ) {
        mod_painkiller( -std::min( get_painkiller(), rate_multiplier ) );
    }

    if( g->is_in_sunlight( pos() ) ) {
        if( has_bionic( bn_bio_solar ) ) {
            mod_power_level( units::from_kilojoule( rate_multiplier * 25 ) );
        }
    }

    // Huge folks take penalties for cramming themselves in vehicles
    if( in_vehicle && ( has_trait( trait_HUGE ) || has_trait( trait_HUGE_OK ) ) ) {
        vehicle *veh = veh_pointer_or_null( g->m.veh_at( pos() ) );
        // it's painful to work the controls, but passengers in open topped vehicles are fine
        if( veh && ( veh->enclosed_at( pos() ) || veh->player_in_control( *this ) ) ) {
            add_msg_if_player( m_bad,
                               _( "You're cramping up from stuffing yourself in this vehicle." ) );
            if( is_npc() ) {
                npc &as_npc = dynamic_cast<npc &>( *this );
                as_npc.complain_about( "cramped_vehicle", 1_hours, "<cramped_vehicle>", false );
            }
            mod_pain_noresist( 2 * rng( 2, 3 ) );
            focus_pool -= 1;
        }
    }
}

void player::regen( int rate_multiplier )
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
    for( int i = 0; i < num_hp_parts; i++ ) {
        body_part bp = hp_to_bp( static_cast<hp_part>( i ) );
        float healing = healing_rate_medicine( rest, bp ) * to_turns<int>( 5_minutes );

        int healing_apply = roll_remainder( healing );
        healed_bp( i, healing_apply );
        heal( bp, healing_apply );
        if( damage_bandaged[i] > 0 ) {
            damage_bandaged[i] -= healing_apply;
            if( damage_bandaged[i] <= 0 ) {
                damage_bandaged[i] = 0;
                remove_effect( effect_bandaged, bp );
                add_msg_if_player( _( "Bandaged wounds on your %s was healed." ), body_part_name( bp ) );
            }
        }
        if( damage_disinfected[i] > 0 ) {
            damage_disinfected[i] -= healing_apply;
            if( damage_disinfected[i] <= 0 ) {
                damage_disinfected[i] = 0;
                remove_effect( effect_disinfected, bp );
                add_msg_if_player( _( "Disinfected wounds on your %s was healed." ), body_part_name( bp ) );
            }
        }

        // remove effects if the limb was healed by other way
        if( has_effect( effect_bandaged, bp ) && ( hp_cur[i] == hp_max[i] ) ) {
            damage_bandaged[i] = 0;
            remove_effect( effect_bandaged, bp );
            add_msg_if_player( _( "Bandaged wounds on your %s was healed." ), body_part_name( bp ) );
        }
        if( has_effect( effect_disinfected, bp ) && ( hp_cur[i] == hp_max[i] ) ) {
            damage_disinfected[i] = 0;
            remove_effect( effect_disinfected, bp );
            add_msg_if_player( _( "Disinfected wounds on your %s was healed." ), body_part_name( bp ) );
        }
    }

    if( radiation > 0 ) {
        radiation = std::max( 0, radiation - roll_remainder( rate_multiplier / 50.0f ) );
    }
}

void player::update_stamina( int turns )
{
    float stamina_recovery = 0.0f;
    // Recover some stamina every turn.
    // Mutated stamina works even when winded
    float stamina_multiplier = ( !has_effect( effect_winded ) ? 1.0f : 0.1f ) +
                               mutation_value( "stamina_regen_modifier" );
    // But mouth encumbrance interferes, even with mutated stamina.
    stamina_recovery += stamina_multiplier * std::max( 1.0f,
                        get_option<float>( "PLAYER_BASE_STAMINA_REGEN_RATE" ) - ( encumb( bp_mouth ) / 5.0f ) );
    // TODO: recovering stamina causes hunger/thirst/fatigue.
    // TODO: Tiredness slowing recovery

    // stim recovers stamina (or impairs recovery)
    if( stim > 0 ) {
        // TODO: Make stamina recovery with stims cost health
        stamina_recovery += std::min( 5.0f, stim / 15.0f );
    } else if( stim < 0 ) {
        // Affect it less near 0 and more near full
        // Negative stim kill at -200
        // At -100 stim it inflicts -20 malus to regen at 100%  stamina,
        // effectivly countering stamina gain of default 20,
        // at 50% stamina its -10 (50%), cuts by 25% at 25% stamina
        stamina_recovery += stim / 5.0f * stamina / get_stamina_max() ;
    }

    const int max_stam = get_stamina_max();
    if( get_power_level() >= 3_kJ && has_active_bionic( bio_gills ) ) {
        int bonus = std::min<int>( units::to_kilojoule( get_power_level() ) / 3,
                                   max_stam - stamina - stamina_recovery * turns );
        // so the effective recovery is up to 5x default
        bonus = std::min( bonus, 4 * static_cast<int>
                          ( get_option<float>( "PLAYER_BASE_STAMINA_REGEN_RATE" ) ) );
        if( bonus > 0 ) {
            stamina_recovery += bonus;
            bonus /= 10;
            bonus = std::max( bonus, 1 );
            mod_power_level( units::from_kilojoule( -bonus ) );
        }
    }

    stamina += roll_remainder( stamina_recovery * turns );
    add_msg( m_debug, "Stamina recovery: %d", roll_remainder( stamina_recovery * turns ) );
    // Cap at max
    stamina = std::min( std::max( stamina, 0 ), max_stam );
}

bool player::is_hibernating() const
{
    // Hibernating only kicks in whilst Engorged; separate tracking for hunger/thirst here
    // as a safety catch.  One test subject managed to get two Colds during hibernation;
    // since those add fatigue and dry out the character, the subject went for the full 10 days plus
    // a little, and came out of it well into Parched.  Hibernating shouldn't endanger your
    // life like that--but since there's much less fluid reserve than food reserve,
    // simply using the same numbers won't work.
    return has_effect( effect_sleep ) && get_kcal_percent() > 0.8f &&
           get_thirst() <= 80 && has_active_mutation( trait_id( "HIBERNATE" ) );
}

void player::add_addiction( add_type type, int strength )
{
    if( type == ADD_NULL ) {
        return;
    }
    time_duration timer = 2_hours;
    if( has_trait( trait_ADDICTIVE ) ) {
        strength *= 2;
        timer = 1_hours;
    } else if( has_trait( trait_NONADDICTIVE ) ) {
        strength /= 2;
        timer = 6_hours;
    }
    //Update existing addiction
    for( auto &i : addictions ) {
        if( i.type != type ) {
            continue;
        }

        if( i.sated < 0_turns ) {
            i.sated = timer;
        } else if( i.sated < 10_minutes ) {
            i.sated += timer; // TODO: Make this variable?
        } else {
            i.sated += timer / 2;
        }
        if( i.intensity < MAX_ADDICTION_LEVEL && strength > i.intensity * rng( 2, 5 ) ) {
            i.intensity++;
        }

        add_msg( m_debug, "Updating addiction: %d intensity, %d sated",
                 i.intensity, to_turns<int>( i.sated ) );

        return;
    }

    // Add a new addiction
    const int roll = rng( 0, 100 );
    add_msg( m_debug, "Addiction: roll %d vs strength %d", roll, strength );
    if( roll < strength ) {
        const std::string &type_name = addiction_type_name( type );
        add_msg( m_debug, "%s got addicted to %s", disp_name(), type_name );
        addictions.emplace_back( type, 1 );
        g->events().send<event_type::gains_addiction>( getID(), type );
    }
}

bool player::has_addiction( add_type type ) const
{
    return std::any_of( addictions.begin(), addictions.end(),
    [type]( const addiction & ad ) {
        return ad.type == type && ad.intensity >= MIN_ADDICTION_LEVEL;
    } );
}

void player::rem_addiction( add_type type )
{
    auto iter = std::find_if( addictions.begin(), addictions.end(),
    [type]( const addiction & ad ) {
        return ad.type == type;
    } );

    if( iter != addictions.end() ) {
        addictions.erase( iter );
        g->events().send<event_type::loses_addiction>( getID(), type );
    }
}

int player::addiction_level( add_type type ) const
{
    auto iter = std::find_if( addictions.begin(), addictions.end(),
    [type]( const addiction & ad ) {
        return ad.type == type;
    } );
    return iter != addictions.end() ? iter->intensity : 0;
}

void player::siphon( vehicle &veh, const itype_id &desired_liquid )
{
    auto qty = veh.fuel_left( desired_liquid );
    if( qty <= 0 ) {
        add_msg( m_bad, _( "There is not enough %s left to siphon it." ),
                 item::nname( desired_liquid ) );
        return;
    }

    item liquid( desired_liquid, calendar::turn, qty );
    if( liquid.is_food() ) {
        liquid.set_item_specific_energy( veh.fuel_specific_energy( desired_liquid ) );
    }
    if( liquid_handler::handle_liquid( liquid, nullptr, 1, nullptr, &veh ) ) {
        veh.drain( desired_liquid, qty - liquid.charges );
    }
}

void player::cough( bool harmful, int loudness )
{
    if( harmful ) {
        const int stam = stamina;
        const int malus = get_stamina_max() * 0.05; // 5% max stamina
        mod_stat( "stamina", -malus );
        if( stam < malus && x_in_y( malus - stam, malus ) && one_in( 6 ) ) {
            apply_damage( nullptr, bp_torso, 1 );
        }
    }

    if( has_effect( effect_cough_suppress ) ) {
        return;
    }

    if( !is_npc() ) {
        add_msg( m_bad, _( "You cough heavily." ) );
    }
    sounds::sound( pos(), loudness, sounds::sound_t::speech, _( "a hacking cough." ), false, "misc",
                   "cough" );

    moves -= 80;

    if( has_effect( effect_sleep ) && !has_effect( effect_narcosis ) &&
        ( ( harmful && one_in( 3 ) ) || one_in( 10 ) ) ) {
        wake_up();
    }
}

void player::add_pain_msg( int val, body_part bp ) const
{
    if( has_trait( trait_NOPAIN ) ) {
        return;
    }
    if( bp == num_bp ) {
        if( val > 20 ) {
            add_msg_if_player( _( "Your body is wracked with excruciating pain!" ) );
        } else if( val > 10 ) {
            add_msg_if_player( _( "Your body is wracked with terrible pain!" ) );
        } else if( val > 5 ) {
            add_msg_if_player( _( "Your body is wracked with pain!" ) );
        } else if( val > 1 ) {
            add_msg_if_player( _( "Your body pains you!" ) );
        } else {
            add_msg_if_player( _( "Your body aches." ) );
        }
    } else {
        if( val > 20 ) {
            add_msg_if_player( _( "Your %s is wracked with excruciating pain!" ),
                               body_part_name_accusative( bp ) );
        } else if( val > 10 ) {
            add_msg_if_player( _( "Your %s is wracked with terrible pain!" ),
                               body_part_name_accusative( bp ) );
        } else if( val > 5 ) {
            add_msg_if_player( _( "Your %s is wracked with pain!" ),
                               body_part_name_accusative( bp ) );
        } else if( val > 1 ) {
            add_msg_if_player( _( "Your %s pains you!" ),
                               body_part_name_accusative( bp ) );
        } else {
            add_msg_if_player( _( "Your %s aches." ),
                               body_part_name_accusative( bp ) );
        }
    }
}

void player::print_health() const
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
        const std::string &msg = SNIPPET.random_from_category( iter->second );
        add_msg_if_player( current_health > 0 ? m_good : m_bad, msg );
    }
}

void player::process_one_effect( effect &it, bool is_new )
{
    bool reduced = resists_effect( it );
    double mod = 1;
    body_part bp = it.get_bp();
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
    auto msgs = it.get_miss_msgs();
    if( !msgs.empty() ) {
        for( const auto &i : msgs ) {
            add_miss_reason( _( i.first ), static_cast<unsigned>( i.second ) );
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
            stim += bound_mod_to_vals( stim, val, it.get_max_val( "STIM", reduced ),
                                       it.get_min_val( "STIM", reduced ) );
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
            radiation += bound_mod_to_vals( radiation, val, it.get_max_val( "RAD", reduced ), 0 );
            // Radiation can't go negative
            if( radiation < 0 ) {
                radiation = 0;
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
            if( has_trait( trait_LARGE ) || has_trait( trait_LARGE_OK ) ) {
                mod *= 2;
            }
            if( has_trait( trait_HUGE ) || has_trait( trait_HUGE_OK ) ) {
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
            if( has_trait( trait_LARGE ) || has_trait( trait_LARGE_OK ) ) {
                mod *= 2;
            }
            if( has_trait( trait_HUGE ) || has_trait( trait_HUGE_OK ) ) {
                mod *= 3;
            }
        }
        if( is_new || it.activated( calendar::turn, "HURT", val, reduced, mod ) ) {
            if( bp == num_bp ) {
                if( val > 5 ) {
                    add_msg_if_player( _( "Your %s HURTS!" ), body_part_name_accusative( bp_torso ) );
                } else {
                    add_msg_if_player( _( "Your %s hurts!" ), body_part_name_accusative( bp_torso ) );
                }
                apply_damage( nullptr, bp_torso, val, true );
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
        mod = it.get_addict_mod( "PKILL", addiction_level( ADD_PKILLER ) );
        if( is_new || it.activated( calendar::turn, "PKILL", val, reduced, mod ) ) {
            mod_painkiller( bound_mod_to_vals( pkill, val, it.get_max_val( "PKILL", reduced ), 0 ) );
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
            mod_stat( "stamina", bound_mod_to_vals( stamina, val,
                                                    it.get_max_val( "STAMINA", reduced ),
                                                    it.get_min_val( "STAMINA", reduced ) ) );
        }
    }

    // Speed and stats are handled in recalc_speed_bonus and reset_stats respectively
}

void player::process_effects()
{
    //Special Removals
    if( has_effect( effect_darkness ) && g->is_in_sunlight( pos() ) ) {
        remove_effect( effect_darkness );
    }
    if( has_trait( trait_M_IMMUNE ) && has_effect( effect_fungus ) ) {
        vomit();
        remove_effect( effect_fungus );
        add_msg_if_player( m_bad,  _( "We have mistakenly colonized a local guide!  Purging now." ) );
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
        add_msg_if_player( m_good, _( "Something writhes and inside of you as it dies." ) );
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

double player::vomit_mod()
{
    double mod = 1;
    if( has_effect( effect_weed_high ) ) {
        mod *= .1;
    }
    if( has_trait( trait_STRONGSTOMACH ) ) {
        mod *= .5;
    }
    if( has_trait( trait_WEAKSTOMACH ) ) {
        mod *= 2;
    }
    if( has_trait( trait_NAUSEA ) ) {
        mod *= 3;
    }
    if( has_trait( trait_VOMITOUS ) ) {
        mod *= 3;
    }
    // If you're already nauseous, any food in your stomach greatly
    // increases chance of vomiting. Liquids don't provoke vomiting, though.
    if( stomach.contains() != 0_ml && has_effect( effect_nausea ) ) {
        mod *= 5 * get_effect_int( effect_nausea );
    }
    return mod;
}

void player::suffer()
{
    // TODO: Remove this section and encapsulate hp_cur
    for( int i = 0; i < num_hp_parts; i++ ) {
        body_part bp = hp_to_bp( static_cast<hp_part>( i ) );
        if( hp_cur[i] <= 0 ) {
            add_effect( effect_disabled, 1_turns, bp, true );
        }
    }

    for( size_t i = 0; i < my_bionics->size(); i++ ) {
        if( ( *my_bionics )[i].powered ) {
            process_bionic( i );
        }
    }

    for( auto &mut : my_mutations ) {
        auto &tdata = mut.second;
        if( !tdata.powered ) {
            continue;
        }
        const auto &mdata = mut.first.obj();
        if( tdata.powered && tdata.charge > 0 ) {
            // Already-on units just lose a bit of charge
            tdata.charge--;
        } else {
            // Not-on units, or those with zero charge, have to pay the power cost
            if( mdata.cooldown > 0 ) {
                tdata.powered = true;
                tdata.charge = mdata.cooldown - 1;
            }
            if( mdata.hunger ) {
                // does not directly modify hunger, but burns kcal
                mod_stored_nutr( mdata.cost );
                if( get_bmi() < character_weight_category::underweight ) {
                    add_msg_if_player( m_warning, _( "You're too malnourished to keep your %s going." ), mdata.name() );
                    tdata.powered = false;
                }
            }
            if( mdata.thirst ) {
                mod_thirst( mdata.cost );
                if( get_thirst() >= 260 ) { // Well into Dehydrated
                    add_msg_if_player( m_warning, _( "You're too dehydrated to keep your %s going." ), mdata.name() );
                    tdata.powered = false;
                }
            }
            if( mdata.fatigue ) {
                mod_fatigue( mdata.cost );
                if( get_fatigue() >= EXHAUSTED ) { // Exhausted
                    add_msg_if_player( m_warning, _( "You're too exhausted to keep your %s going." ), mdata.name() );
                    tdata.powered = false;
                }
            }

            if( !tdata.powered ) {
                apply_mods( mut.first, false );
            }
        }
    }

    if( underwater ) {
        if( !has_trait( trait_GILLS ) && !has_trait( trait_GILLS_CEPH ) ) {
            oxygen--;
        }
        if( oxygen < 12 && worn_with_flag( "REBREATHER" ) ) {
            oxygen += 12;
        }
        if( oxygen <= 5 ) {
            if( has_bionic( bio_gills ) && get_power_level() >= 25_kJ ) {
                oxygen += 5;
                mod_power_level( -25_kJ );
            } else {
                add_msg_if_player( m_bad, _( "You're drowning!" ) );
                apply_damage( nullptr, bp_torso, rng( 1, 4 ) );
            }
        }
        if( has_trait( trait_FRESHWATEROSMOSIS ) && !g->m.has_flag_ter( "SALT_WATER", pos() ) &&
            get_thirst() > -60 ) {
            mod_thirst( -1 );
        }
    }

    if( has_trait( trait_SHARKTEETH ) && one_turn_in( 24_hours ) ) {
        add_msg_if_player( m_neutral, _( "You shed a tooth!" ) );
        g->m.spawn_item( pos(), "bone", 1 );
    }

    if( has_active_mutation( trait_id( "WINGS_INSECT" ) ) ) {
        //~Sound of buzzing Insect Wings
        sounds::sound( pos(), 10, sounds::sound_t::movement, _( "BZZZZZ" ), false, "misc", "insect_wings" );
    }

    bool wearing_shoes = is_wearing_shoes( side::LEFT ) || is_wearing_shoes( side::RIGHT );
    int root_vitamins = 0;
    int root_water = 0;
    if( has_trait( trait_ROOTS3 ) && g->m.has_flag( "PLOWABLE", pos() ) && !wearing_shoes ) {
        root_vitamins += 1;
        if( get_thirst() <= -2000 ) {
            root_water += 51;
        }
    }

    if( x_in_y( root_vitamins, 576 ) ) {
        vitamin_mod( vitamin_id( "iron" ), 1, true );
        vitamin_mod( vitamin_id( "calcium" ), 1, true );
        mod_healthy_mod( 5, 50 );
    }

    if( x_in_y( root_water, 2550 ) ) {
        // Plants draw some crazy amounts of water from the ground in real life,
        // so these numbers try to reflect that uncertain but large amount
        // this should take 12 hours to meet your daily needs with ROOTS2, and 8 with ROOTS3
        mod_thirst( -1 );
    }

    time_duration timer = -6_hours;
    if( has_trait( trait_ADDICTIVE ) ) {
        timer = -10_hours;
    } else if( has_trait( trait_NONADDICTIVE ) ) {
        timer = -3_hours;
    }
    for( auto &cur_addiction : addictions ) {
        if( cur_addiction.sated <= 0_turns &&
            cur_addiction.intensity >= MIN_ADDICTION_LEVEL ) {
            addict_effect( *this, cur_addiction );
        }
        cur_addiction.sated -= 1_turns;
        // Higher intensity addictions heal faster
        if( cur_addiction.sated - 10_minutes * cur_addiction.intensity < timer ) {
            if( cur_addiction.intensity <= 2 ) {
                rem_addiction( cur_addiction.type );
                break;
            } else {
                cur_addiction.intensity--;
                cur_addiction.sated = 0_turns;
            }
        }
    }

    if( !in_sleep_state() ) {
        if( !has_trait( trait_id( "DEBUG_STORAGE" ) ) && ( weight_carried() > 4 * weight_capacity() ) ) {
            if( has_effect( effect_downed ) ) {
                add_effect( effect_downed, 1_turns, num_bp, false, 0, true );
            } else {
                add_effect( effect_downed, 2_turns, num_bp, false, 0, true );
            }
        }
        if( has_trait( trait_CHEMIMBALANCE ) ) {
            if( one_turn_in( 6_hours ) && !has_trait( trait_NOPAIN ) ) {
                add_msg_if_player( m_bad, _( "You suddenly feel sharp pain for no reason." ) );
                mod_pain( 3 * rng( 1, 3 ) );
            }
            if( one_turn_in( 6_hours ) ) {
                int pkilladd = 5 * rng( -1, 2 );
                if( pkilladd > 0 ) {
                    add_msg_if_player( m_bad, _( "You suddenly feel numb." ) );
                } else if( ( pkilladd < 0 ) && ( !( has_trait( trait_NOPAIN ) ) ) ) {
                    add_msg_if_player( m_bad, _( "You suddenly ache." ) );
                }
                mod_painkiller( pkilladd );
            }
            if( one_turn_in( 6_hours ) && !has_effect( effect_sleep ) ) {
                add_msg_if_player( m_bad, _( "You feel dizzy for a moment." ) );
                moves -= rng( 10, 30 );
            }
            if( one_turn_in( 6_hours ) ) {
                int hungadd = 5 * rng( -1, 3 );
                if( hungadd > 0 ) {
                    add_msg_if_player( m_bad, _( "You suddenly feel hungry." ) );
                } else {
                    add_msg_if_player( m_good, _( "You suddenly feel a little full." ) );
                }
                mod_hunger( hungadd );
            }
            if( one_turn_in( 6_hours ) ) {
                add_msg_if_player( m_bad, _( "You suddenly feel thirsty." ) );
                mod_thirst( 5 * rng( 1, 3 ) );
            }
            if( one_turn_in( 6_hours ) ) {
                add_msg_if_player( m_good, _( "You feel fatigued all of a sudden." ) );
                mod_fatigue( 10 * rng( 2, 4 ) );
            }
            if( one_turn_in( 8_hours ) ) {
                if( one_in( 3 ) ) {
                    add_morale( MORALE_FEELING_GOOD, 20, 100 );
                } else {
                    add_morale( MORALE_FEELING_BAD, -20, -100 );
                }
            }
            if( one_turn_in( 6_hours ) ) {
                if( one_in( 3 ) ) {
                    add_msg_if_player( m_bad, _( "You suddenly feel very cold." ) );
                    temp_cur.fill( BODYTEMP_VERY_COLD );
                } else {
                    add_msg_if_player( m_bad, _( "You suddenly feel cold." ) );
                    temp_cur.fill( BODYTEMP_COLD );
                }
            }
            if( one_turn_in( 6_hours ) ) {
                if( one_in( 3 ) ) {
                    add_msg_if_player( m_bad, _( "You suddenly feel very hot." ) );
                    temp_cur.fill( BODYTEMP_VERY_HOT );
                } else {
                    add_msg_if_player( m_bad, _( "You suddenly feel hot." ) );
                    temp_cur.fill( BODYTEMP_HOT );
                }
            }
        }
        if( ( has_trait( trait_SCHIZOPHRENIC ) || has_artifact_with( AEP_SCHIZO ) ) &&
            !has_effect( effect_took_thorazine ) ) {
            if( is_player() ) {
                bool done_effect = false;
                // Sound
                if( one_turn_in( 4_hours ) ) {
                    sound_hallu();
                }

                // Follower turns hostile
                if( !done_effect && one_turn_in( 4_hours ) ) {
                    std::vector<std::shared_ptr<npc>> followers = overmap_buffer.get_npcs_near_player( 12 );

                    std::string who_gets_angry = name;
                    if( !followers.empty() ) {
                        who_gets_angry = random_entry_ref( followers )->name;
                    }
                    add_msg( m_bad, _( "%1$s gets angry!" ), who_gets_angry );
                    done_effect = true;
                }

                // Monster dies
                if( !done_effect && one_turn_in( 6_hours ) ) {

                    // TODO: move to monster group json
                    static const mtype_id mon_zombie( "mon_zombie" );
                    static const mtype_id mon_zombie_fat( "mon_zombie_fat" );
                    static const mtype_id mon_zombie_fireman( "mon_zombie_fireman" );
                    static const mtype_id mon_zombie_cop( "mon_zombie_cop" );
                    static const mtype_id mon_zombie_soldier( "mon_zombie_soldier" );
                    static const std::array<mtype_id, 5> monsters = { {
                            mon_zombie, mon_zombie_fat, mon_zombie_fireman, mon_zombie_cop, mon_zombie_soldier
                        }
                    };
                    add_msg( _( "%s dies!" ), random_entry_ref( monsters )->nname() );
                    done_effect = true;
                }

                // Limb Breaks
                if( !done_effect && one_turn_in( 4_hours ) ) {
                    std::string snip = SNIPPET.random_from_category( "broken_limb" );
                    add_msg( m_bad, snip );
                    done_effect = true;
                }

                // NPC chat
                if( !done_effect && one_turn_in( 4_hours ) ) {
                    std::string i_name = Name::generate( one_in( 2 ) );

                    std::string i_talk = SNIPPET.random_from_category( "<lets_talk>" );
                    parse_tags( i_talk, *this, *this );

                    add_msg( _( "%1$s says: \"%2$s\"" ), i_name, i_talk );
                    done_effect = true;
                }

                // Skill raise
                if( !done_effect && one_turn_in( 12_hours ) ) {
                    skill_id raised_skill = Skill::random_skill();
                    add_msg( m_good, _( "You increase %1$s to level %2$d." ), raised_skill.obj().name(),
                             get_skill_level( raised_skill ) + 1 );
                    done_effect = true;
                }

                // Talk to self
                if( !done_effect && one_turn_in( 4_hours ) ) {
                    std::string snip = SNIPPET.random_from_category( "schizo_self_talk" );
                    add_msg( _( "%1$s says: \"%2$s\"" ), name, snip );
                    done_effect = true;
                }

                // Talking weapon
                if( !done_effect && !weapon.is_null() ) {
                    // If player has a weapon, picks a message from said weapon
                    // Weapon tells player to kill a monster if any are nearby
                    // Weapon is concerned for player if bleeding
                    // Weapon is concerned for itself if damaged
                    // Otherwise random chit-chat

                    std::string i_name_w = weapon.has_var( "item_label" ) ?
                                           weapon.get_var( "item_label" ) :
                                           //~ %1$s: weapon name
                                           string_format( _( "Your %1$s" ), weapon.type_name() );

                    std::vector<std::weak_ptr<monster>> mons = g->all_monsters().items;

                    std::string i_talk_w;
                    bool does_talk = false;
                    if( !mons.empty() && one_turn_in( 12_minutes ) ) {
                        std::vector<std::string> seen_mons;
                        for( auto &n : mons ) {
                            if( sees( *n.lock() ) ) {
                                seen_mons.emplace_back( n.lock()->get_name() );
                            }
                        }
                        if( !seen_mons.empty() ) {
                            std::string talk_w = SNIPPET.random_from_category( "schizo_weapon_talk_monster" );
                            i_talk_w = string_format( talk_w, random_entry_ref( seen_mons ) );
                            does_talk = true;
                        }
                    }
                    if( !does_talk && has_effect( effect_bleed ) && one_turn_in( 5_minutes ) ) {
                        i_talk_w = SNIPPET.random_from_category( "schizo_weapon_talk_bleeding" );
                        does_talk = true;
                    } else if( weapon.damage() >= weapon.max_damage() / 3 && one_turn_in( 1_hours ) ) {
                        i_talk_w = SNIPPET.random_from_category( "schizo_weapon_talk_damaged" );
                        does_talk = true;
                    } else if( one_turn_in( 4_hours ) ) {
                        i_talk_w = SNIPPET.random_from_category( "schizo_weapon_talk_misc" );
                        does_talk = true;
                    }
                    if( does_talk ) {
                        add_msg( _( "%1$s says: \"%2$s\"" ), i_name_w, i_talk_w );
                        done_effect = true;
                    }
                }
                // Delusions
                if( !done_effect && one_turn_in( 8_hours ) ) {
                    if( rng( 1, 20 ) > 5 ) {  // 75% chance
                        std::string snip = SNIPPET.random_from_category( "schizo_delusion_paranoid" );
                        add_msg( m_warning, snip );
                        add_morale( MORALE_FEELING_BAD, -20, -100 );
                    } else { // 25% chance
                        std::string snip = SNIPPET.random_from_category( "schizo_delusion_grandiose" );
                        add_msg( m_good, snip );
                        add_morale( MORALE_FEELING_GOOD, 20, 100 );
                    }
                    done_effect = true;
                }
                // Formication
                if( !done_effect && one_turn_in( 6_hours ) ) {
                    std::string snip = SNIPPET.random_from_category( "schizo_formication" );
                    body_part bp = random_body_part( true );
                    add_effect( effect_formication, 45_minutes, bp );
                    add_msg( m_bad, snip );
                    done_effect = true;
                }
                // Numbness
                if( !done_effect && one_turn_in( 4_hours ) ) {
                    add_msg( m_bad, _( "You suddenly feel so numb..." ) );
                    mod_painkiller( 25 );
                    done_effect = true;
                }
                // Hallucination
                if( !done_effect && one_turn_in( 6_hours ) ) {
                    add_effect( effect_hallu, 6_hours );
                    done_effect = true;
                }
                // Visuals
                if( !done_effect && one_turn_in( 2_hours ) ) {
                    add_effect( effect_visuals, rng( 15_turns, 60_turns ) );
                    done_effect = true;
                }
                // Shaking
                if( !done_effect && !has_effect( effect_valium ) && one_turn_in( 4_hours ) ) {
                    add_msg( m_bad, _( "You start to shake uncontrollably." ) );
                    add_effect( effect_shakes, rng( 2_minutes, 5_minutes ) );
                    done_effect = true;
                }
                // Shout
                if( !done_effect && one_turn_in( 4_hours ) ) {
                    std::string snip = SNIPPET.random_from_category( "schizo_self_shout" );
                    shout( string_format( _( "yourself shout, %s" ), snip ) );
                    done_effect = true;
                }
                // Drop weapon
                if( !done_effect && one_turn_in( 2_days ) ) {
                    if( !weapon.is_null() ) {
                        std::string i_name_w = weapon.has_var( "item_label" ) ?
                                               weapon.get_var( "item_label" ) :
                                               //~ %1$s: weapon name
                                               string_format( _( "your %1$s" ), weapon.type_name() );

                        std::string snip = SNIPPET.random_from_category( "schizo_weapon_drop" );
                        std::string str = string_format( snip, i_name_w );
                        str[0] = toupper( str[0] );

                        add_msg( m_bad, str );
                        drop( get_item_position( &weapon ), pos() );
                        // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
                        done_effect = true;
                    }
                }
            }
        }

        if( ( has_trait( trait_NARCOLEPTIC ) || has_artifact_with( AEP_SCHIZO ) ) ) {
            if( one_turn_in( 8_hours ) ) {
                add_msg( m_bad, _( "You're suddenly overcome with the urge to sleep and you pass out." ) );
                add_effect( effect_lying_down, 20_minutes );
            }
        }

        if( has_trait( trait_JITTERY ) && !has_effect( effect_shakes ) ) {
            if( stim > 50 && one_in( to_turns<int>( 30_minutes ) - ( stim * 6 ) ) ) {
                add_effect( effect_shakes, 30_minutes + 1_turns * stim );
            } else if( ( get_hunger() > 80 || get_kcal_percent() < 1.0f ) && get_hunger() > 0 &&
                       one_in( to_turns<int>( 50_minutes ) - ( get_hunger() * 6 ) ) ) {
                add_effect( effect_shakes, 40_minutes );
            }
        }

        if( has_trait( trait_MOODSWINGS ) && one_turn_in( 6_hours ) ) {
            if( rng( 1, 20 ) > 9 ) { // 55% chance
                add_morale( MORALE_MOODSWING, -100, -500 );
            } else {  // 45% chance
                add_morale( MORALE_MOODSWING, 100, 500 );
            }
        }

        if( has_trait( trait_VOMITOUS ) && one_turn_in( 7_hours ) ) {
            vomit();
        }

        if( has_trait( trait_SHOUT1 ) && one_turn_in( 6_hours ) ) {
            shout();
        }
        if( has_trait( trait_SHOUT2 ) && one_turn_in( 4_hours ) ) {
            shout();
        }
        if( has_trait( trait_SHOUT3 ) && one_turn_in( 3_hours ) ) {
            shout();
        }
        if( has_trait( trait_M_SPORES ) && one_turn_in( 4_hours ) ) {
            spores();
        }
        if( has_trait( trait_M_BLOSSOMS ) && one_turn_in( 3_hours ) ) {
            blossoms();
        }
    } // Done with while-awake-only effects

    if( has_trait( trait_ASTHMA ) && !has_effect( effect_adrenaline ) &&
        !has_effect( effect_datura ) &&
        one_in( ( to_turns<int>( 6_hours ) - stim * 300 ) *
                ( has_effect( effect_sleep ) ? 10 : 1 ) ) ) {
        bool auto_use = has_charges( "inhaler", 1 ) || has_charges( "oxygen_tank", 1 ) ||
                        has_charges( "smoxygen_tank", 1 );
        bool oxygenator = has_bionic( bio_gills ) && get_power_level() >= 3_kJ;
        if( underwater ) {
            oxygen = oxygen / 2;
            auto_use = false;
        }

        add_msg_player_or_npc( m_bad, _( "You have an asthma attack!" ),
                               "<npcname> starts wheezing and coughing." );

        if( in_sleep_state() && !has_effect( effect_narcosis ) ) {
            inventory map_inv;
            map_inv.form_from_map( g->u.pos(), 2, &g->u );
            // check if an inhaler is somewhere near
            bool nearby_use = auto_use || oxygenator || map_inv.has_charges( "inhaler", 1 ) ||
                              map_inv.has_charges( "oxygen_tank", 1 ) ||
                              map_inv.has_charges( "smoxygen_tank", 1 );
            // check if character has an oxygenator first
            if( oxygenator ) {
                mod_power_level( -3_kJ );
                add_msg_if_player( m_info, _( "You use your Oxygenator to clear it up, "
                                              "then go back to sleep." ) );
            } else if( auto_use ) {
                if( use_charges_if_avail( "inhaler", 1 ) ) {
                    add_msg_if_player( m_info, _( "You use your inhaler and go back to sleep." ) );
                } else if( use_charges_if_avail( "oxygen_tank", 1 ) ||
                           use_charges_if_avail( "smoxygen_tank", 1 ) ) {
                    add_msg_if_player( m_info, _( "You take a deep breath from your oxygen tank "
                                                  "and go back to sleep." ) );
                }
            } else if( nearby_use ) {
                // create new variable to resolve a reference issue
                int amount = 1;
                if( !g->m.use_charges( g->u.pos(), 2, "inhaler", amount ).empty() ) {
                    add_msg_if_player( m_info, _( "You use your inhaler and go back to sleep." ) );
                } else if( !g->m.use_charges( g->u.pos(), 2, "oxygen_tank", amount ).empty() ||
                           !g->m.use_charges( g->u.pos(), 2, "smoxygen_tank", amount ).empty() ) {
                    add_msg_if_player( m_info, _( "You take a deep breath from your oxygen tank "
                                                  "and go back to sleep." ) );
                }
            } else {
                add_effect( effect_asthma, rng( 5_minutes, 20_minutes ) );
                if( has_effect( effect_sleep ) ) {
                    wake_up();
                } else {
                    if( !is_npc() ) {
                        g->cancel_activity_or_ignore_query( distraction_type::asthma,
                                                            _( "You can't focus while choking!" ) );
                    }
                }
            }
        } else if( auto_use ) {
            int charges = 0;
            if( use_charges_if_avail( "inhaler", 1 ) ) {
                moves -= 40;
                charges = charges_of( "inhaler" );
                if( charges == 0 ) {
                    add_msg_if_player( m_bad, _( "You use your last inhaler charge." ) );
                } else {
                    add_msg_if_player( m_info, ngettext( "You use your inhaler, "
                                                         "only %d charge left.",
                                                         "You use your inhaler, "
                                                         "only %d charges left.", charges ),
                                       charges );
                }
            } else if( use_charges_if_avail( "oxygen_tank", 1 ) ||
                       use_charges_if_avail( "smoxygen_tank", 1 ) ) {
                moves -= 500; // synched with use action
                charges = charges_of( "oxygen_tank" ) + charges_of( "smoxygen_tank" );
                if( charges == 0 ) {
                    add_msg_if_player( m_bad, _( "You breathe in last bit of oxygen "
                                                 "from the tank." ) );
                } else {
                    add_msg_if_player( m_info, ngettext( "You take a deep breath from your oxygen "
                                                         "tank, only %d charge left.",
                                                         "You take a deep breath from your oxygen "
                                                         "tank, only %d charges left.", charges ),
                                       charges );
                }
            }
        } else {
            add_effect( effect_asthma, rng( 5_minutes, 20_minutes ) );
            if( !is_npc() ) {
                g->cancel_activity_or_ignore_query( distraction_type::asthma,
                                                    _( "You can't focus while choking!" ) );
            }
        }
    }

    double sleeve_factor = armwear_factor();
    const bool has_hat = wearing_something_on( bp_head );
    const bool leafy = has_trait( trait_LEAVES ) || has_trait( trait_LEAVES2 ) ||
                       has_trait( trait_LEAVES3 );
    const bool leafier = has_trait( trait_LEAVES2 ) || has_trait( trait_LEAVES3 );
    const bool leafiest = has_trait( trait_LEAVES3 );
    int sunlight_nutrition = 0;
    if( leafy && g->m.is_outside( pos() ) && ( g->light_level( pos().z ) >= 40 ) ) {
        const float weather_factor = ( g->weather.weather == WEATHER_CLEAR ||
                                       g->weather.weather == WEATHER_SUNNY ) ? 1.0 : 0.5;
        const int player_local_temp = g->weather.get_temperature( pos() );
        int flux = ( player_local_temp - 65 ) / 2;
        if( !has_hat ) {
            sunlight_nutrition += ( 100 + flux ) * weather_factor;
        }
        if( leafier ) {
            int rate = ( ( 100 * sleeve_factor ) + flux ) * 2;
            sunlight_nutrition += ( rate * ( leafiest ? 2 : 1 ) ) * weather_factor;
        }
    }

    if( x_in_y( sunlight_nutrition, 18000 ) ) {
        vitamin_mod( vitamin_id( "vitA" ), 1, true );
        vitamin_mod( vitamin_id( "vitC" ), 1, true );
    }

    if( x_in_y( sunlight_nutrition, 12000 ) ) {
        mod_hunger( -1 );
        // photosynthesis absorbs kcal directly
        mod_stored_nutr( -1 );
        stomach.ate();
    }

    if( get_pain() > 0 ) {
        if( has_trait( trait_PAINREC1 ) && one_turn_in( 1_hours ) ) {
            mod_pain( -1 );
        }
        if( has_trait( trait_PAINREC2 ) && one_turn_in( 30_minutes ) ) {
            mod_pain( -1 );
        }
        if( has_trait( trait_PAINREC3 ) && one_turn_in( 15_minutes ) ) {
            mod_pain( -1 );
        }
    }

    if( ( has_trait( trait_ALBINO ) || has_effect( effect_datura ) ) &&
        g->is_in_sunlight( pos() ) && one_turn_in( 1_minutes ) ) {
        // Umbrellas can keep the sun off the skin and sunglasses - off the eyes.
        if( !weapon.has_flag( "RAIN_PROTECT" ) ) {
            //calculate total coverage of skin
            body_part_set affected_bp { {
                    bp_leg_l, bp_leg_r, bp_torso, bp_head, bp_mouth, bp_arm_l,
                    bp_arm_r, bp_foot_l, bp_foot_r, bp_hand_l, bp_hand_r
                }
            };
            //pecentage of "open skin" by body part
            std::map<body_part, float> open_percent;
            //initialize coverage
            for( const body_part &bp : all_body_parts ) {
                if( affected_bp.test( bp ) ) {
                    open_percent[bp] = 1.0;
                }
            }
            //calculate coverage for every body part
            for( const item &i : worn ) {
                body_part_set covered = i.get_covered_body_parts();
                for( const body_part &bp : all_body_parts )  {
                    if( !affected_bp.test( bp ) || !covered.test( bp ) ) {
                        continue;
                    }
                    //percent of "not covered skin"
                    float p = 1.0 - i.get_coverage() / 100.0;
                    open_percent[bp] = open_percent[bp] * p;
                }
            }

            const float COVERAGE_LIMIT = 0.01;
            body_part max_affected_bp = num_bp;
            float max_affected_bp_percent = 0;
            int count_affected_bp = 0;
            for( auto &it : open_percent ) {
                const body_part &bp = it.first;
                const float &p = it.second;

                if( p <= COVERAGE_LIMIT ) {
                    continue;
                }
                ++count_affected_bp;
                if( max_affected_bp_percent < p ) {
                    max_affected_bp_percent = p;
                    max_affected_bp = bp;
                }
            }
            if( count_affected_bp > 0 && max_affected_bp != num_bp ) {
                //Check if both arms/legs are affected
                int parts_count = 1;
                body_part other_bp = static_cast<body_part>( bp_aiOther[max_affected_bp] );
                body_part other_bp_rev = static_cast<body_part>( bp_aiOther[other_bp] );
                if( other_bp != other_bp_rev ) {
                    const auto found = open_percent.find( other_bp );
                    if( found != open_percent.end() && found->second > COVERAGE_LIMIT ) {
                        ++parts_count;
                    }
                }
                std::string bp_name = body_part_name( max_affected_bp, parts_count );
                if( count_affected_bp > parts_count ) {
                    bp_name = string_format( _( "%s and other body parts" ), bp_name );
                }
                add_msg_if_player( m_bad, _( "The sunlight is really irritating your %s." ), bp_name );

                //apply effects
                if( has_effect( effect_sleep ) && !has_effect( effect_narcosis ) ) {
                    wake_up();
                }
                if( one_turn_in( 1_minutes ) ) {
                    mod_pain( 1 );
                } else {
                    focus_pool --;
                }
            }
        }
        if( !( ( ( worn_with_flag( "SUN_GLASSES" ) ) || worn_with_flag( "BLIND" ) ) &&
               ( wearing_something_on( bp_eyes ) ) )
            && !has_bionic( bionic_id( "bio_sunglasses" ) ) ) {
            add_msg_if_player( m_bad, _( "The sunlight is really irritating your eyes." ) );
            if( one_turn_in( 1_minutes ) ) {
                mod_pain( 1 );
            } else {
                focus_pool --;
            }
        }
    }
    for( const auto &m : my_mutations ) {
        const mutation_branch &mdata = m.first.obj();
        for( const body_part bp : all_body_parts ) {
            if( calendar::once_every( 1_minutes ) ) {
                // 0.0 - 1.0
                const float wetness_percentage =  static_cast<float>( body_wetness[bp] ) / drench_capacity[bp];
                const int dmg = mdata.weakness_to_water * wetness_percentage;
                if( dmg > 0 ) {
                    apply_damage( nullptr, bp, dmg );
                    add_msg_player_or_npc( m_bad, _( "Your %s is damaged by the water." ),
                                           _( "<npcname>'s %s is damaged by the water." ), body_part_name( bp ) );
                } else if( dmg < 0 && hp_cur[bp_to_hp( bp )] != hp_max[bp_to_hp( bp )] ) {
                    heal( bp, abs( dmg ) );
                    add_msg_player_or_npc( m_good, _( "Your %s is healed by the water." ),
                                           _( "<npcname>'s %s is healed by the water." ), body_part_name( bp ) );
                }
            }
        }
    }

    if( has_trait( trait_SUNBURN ) && g->is_in_sunlight( pos() ) && one_in( 10 ) ) {
        if( !( weapon.has_flag( "RAIN_PROTECT" ) ) ) {
            add_msg_if_player( m_bad, _( "The sunlight burns your skin!" ) );
            if( has_effect( effect_sleep ) && !has_effect( effect_narcosis ) ) {
                wake_up();
            }
            mod_pain( 1 );
            hurtall( 1, nullptr );
        }
    }

    if( ( has_trait( trait_TROGLO ) || has_trait( trait_TROGLO2 ) ) &&
        g->is_in_sunlight( pos() ) && g->weather.weather == WEATHER_SUNNY ) {
        mod_str_bonus( -1 );
        mod_dex_bonus( -1 );
        add_miss_reason( _( "The sunlight distracts you." ), 1 );
        mod_int_bonus( -1 );
        mod_per_bonus( -1 );
    }
    if( has_trait( trait_TROGLO2 ) && g->is_in_sunlight( pos() ) ) {
        mod_str_bonus( -1 );
        mod_dex_bonus( -1 );
        add_miss_reason( _( "The sunlight distracts you." ), 1 );
        mod_int_bonus( -1 );
        mod_per_bonus( -1 );
    }
    if( has_trait( trait_TROGLO3 ) && g->is_in_sunlight( pos() ) ) {
        mod_str_bonus( -4 );
        mod_dex_bonus( -4 );
        add_miss_reason( _( "You can't stand the sunlight!" ), 4 );
        mod_int_bonus( -4 );
        mod_per_bonus( -4 );
    }

    if( has_trait( trait_SORES ) ) {
        for( const body_part bp : all_body_parts ) {
            if( bp == bp_head ) {
                continue;
            }
            int sores_pain = 5 + 0.4 * abs( encumb( bp ) );
            if( get_pain() < sores_pain ) {
                set_pain( sores_pain );
            }
        }
    }
    //Web Weavers...weave web
    if( has_active_mutation( trait_WEB_WEAVER ) && !in_vehicle ) {
        // this adds intensity to if its not already there.
        g->m.add_field( pos(), fd_web, 1 );

    }

    // Blind/Deaf for brief periods about once an hour,
    // and visuals about once every 30 min.
    if( has_trait( trait_PER_SLIME ) ) {
        if( one_turn_in( 1_hours ) && !has_effect( effect_deaf ) ) {
            add_msg_if_player( m_bad, _( "Suddenly, you can't hear anything!" ) );
            add_effect( effect_deaf, rng( 20_minutes, 60_minutes ) );
        }
        if( one_turn_in( 1_hours ) && !( has_effect( effect_blind ) ) ) {
            add_msg_if_player( m_bad, _( "Suddenly, your eyes stop working!" ) );
            add_effect( effect_blind, rng( 2_minutes, 6_minutes ) );
        }
        // Yes, you can be blind and hallucinate at the same time.
        // Your post-human biology is truly remarkable.
        if( one_turn_in( 30_minutes ) && !( has_effect( effect_visuals ) ) ) {
            add_msg_if_player( m_bad, _( "Your visual centers must be acting up..." ) );
            add_effect( effect_visuals, rng( 36_minutes, 72_minutes ) );
        }
    }

    if( has_trait( trait_WEB_SPINNER ) && !in_vehicle && one_in( 3 ) ) {
        // this adds intensity to if its not already there.
        g->m.add_field( pos(), fd_web, 1 );
    }

    if( has_trait( trait_UNSTABLE ) && !has_trait( trait_CHAOTIC_BAD ) && one_turn_in( 48_hours ) ) {
        mutate();
    }
    if( ( has_trait( trait_CHAOTIC ) || has_trait( trait_CHAOTIC_BAD ) ) && one_turn_in( 12_hours ) ) {
        mutate();
    }
    if( has_artifact_with( AEP_MUTAGENIC ) && one_turn_in( 48_hours ) ) {
        mutate();
    }
    if( has_artifact_with( AEP_FORCE_TELEPORT ) && one_turn_in( 1_hours ) ) {
        teleport::teleport( *this );
    }
    const bool needs_fire = !has_morale( MORALE_PYROMANIA_NEARFIRE ) &&
                            !has_morale( MORALE_PYROMANIA_STARTFIRE );
    if( has_trait( trait_PYROMANIA ) && needs_fire && !in_sleep_state() &&
        calendar::once_every( 2_hours ) ) {
        add_morale( MORALE_PYROMANIA_NOFIRE, -1, -30, 24_hours, 24_hours, true );
        if( calendar::once_every( 4_hours ) ) {
            std::string smokin_hot_fiyah = SNIPPET.random_from_category( "pyromania_withdrawal" );
            add_msg_if_player( m_bad, _( smokin_hot_fiyah ) );
        }
    }
    if( has_trait( trait_KILLER ) && !has_morale( MORALE_KILLER_HAS_KILLED ) &&
        calendar::once_every( 2_hours ) ) {
        add_morale( MORALE_KILLER_NEED_TO_KILL, -1, -30, 24_hours, 24_hours );
        if( calendar::once_every( 4_hours ) ) {
            std::string snip = SNIPPET.random_from_category( "killer_withdrawal" );
            add_msg_if_player( m_bad, _( snip ) );
        }
    }

    // checking for radioactive items in inventory
    const int item_radiation = leak_level( "RADIOACTIVE" );

    const int map_radiation = g->m.get_radiation( pos() );

    float rads = map_radiation / 100.0f + item_radiation / 10.0f;

    int rad_mut = 0;
    if( has_trait( trait_RADIOACTIVE3 ) ) {
        rad_mut = 3;
    } else if( has_trait( trait_RADIOACTIVE2 ) ) {
        rad_mut = 2;
    } else if( has_trait( trait_RADIOACTIVE1 ) ) {
        rad_mut = 1;
    }

    // Spread less radiation when sleeping (slower metabolism etc.)
    // Otherwise it can quickly get to the point where you simply can't sleep at all
    const bool rad_mut_proc = rad_mut > 0 &&
                              x_in_y( rad_mut, to_turns<int>( in_sleep_state() ? 3_hours : 30_minutes ) );

    bool has_helmet = false;
    const bool power_armored = is_wearing_power_armor( &has_helmet );
    const bool rad_resist = power_armored || worn_with_flag( "RAD_RESIST" );

    if( rad_mut > 0 ) {
        const bool kept_in = is_rad_immune() || ( rad_resist && !one_in( 4 ) );
        if( kept_in ) {
            // As if standing on a map tile with radiation level equal to rad_mut
            rads += rad_mut / 100.0f;
        }

        if( rad_mut_proc && !kept_in ) {
            // Irradiate a random nearby point
            // If you can't, irradiate the player instead
            tripoint rad_point = pos() + point( rng( -3, 3 ), rng( -3, 3 ) );
            // TODO: Radioactive vehicles?
            if( g->m.get_radiation( rad_point ) < rad_mut ) {
                g->m.adjust_radiation( rad_point, 1 );
            } else {
                rads += rad_mut;
            }
        }
    }

    // Used to control vomiting from radiation to make it not-annoying
    bool radiation_increasing = irradiate( rads );

    if( radiation_increasing && calendar::once_every( 3_minutes ) && has_bionic( bio_geiger ) ) {
        add_msg_if_player( m_warning,
                           _( "You feel an anomalous sensation coming from your radiation sensors." ) );
    }

    if( calendar::once_every( 15_minutes ) ) {
        if( radiation < 0 ) {
            radiation = 0;
        } else if( radiation > 2000 ) {
            radiation = 2000;
        }
        if( get_option<bool>( "RAD_MUTATION" ) && rng( 100, 10000 ) < radiation ) {
            mutate();
            radiation -= 50;
        } else if( radiation > 50 && rng( 1, 3000 ) < radiation && ( stomach.contains() > 0_ml ||
                   radiation_increasing || !in_sleep_state() ) ) {
            vomit();
            radiation -= 1;
        }
    }

    const bool radiogenic = has_trait( trait_RADIOGENIC );
    if( radiogenic && calendar::once_every( 30_minutes ) && radiation > 0 ) {
        // At 200 irradiation, twice as fast as REGEN
        if( x_in_y( radiation, 200 ) ) {
            healall( 1 );
            if( rad_mut == 0 ) {
                // Don't heal radiation if we're generating it naturally
                // That would counter the main downside of radioactivity
                radiation -= 5;
            }
        }
    }

    if( !radiogenic && radiation > 0 ) {
        // Even if you heal the radiation itself, the damage is done.
        const int hmod = get_healthy_mod();
        if( hmod > 200 - radiation ) {
            set_healthy_mod( std::max( -200, 200 - radiation ) );
        }
    }

    if( radiation > 200 && calendar::once_every( 10_minutes ) && x_in_y( radiation, 1000 ) ) {
        hurtall( 1, nullptr );
        radiation -= 5;
    }

    if( reactor_plut || tank_plut || slow_rad ) {
        // Microreactor CBM and supporting bionics
        if( has_bionic( bio_reactor ) || has_bionic( bio_advreactor ) ) {
            //first do the filtering of plutonium from storage to reactor
            if( tank_plut > 0 ) {
                int plut_trans;
                if( has_active_bionic( bio_plut_filter ) ) {
                    plut_trans = tank_plut * 0.025;
                } else {
                    plut_trans = tank_plut * 0.005;
                }
                if( plut_trans < 1 ) {
                    plut_trans = 1;
                }
                tank_plut -= plut_trans;
                reactor_plut += plut_trans;
            }
            //leaking radiation, reactor is unshielded, but still better than a simple tank
            slow_rad += ( ( tank_plut * 0.1 ) + ( reactor_plut * 0.01 ) );
            //begin power generation
            if( reactor_plut > 0 ) {
                int power_gen = 0;
                if( has_bionic( bio_advreactor ) ) {
                    if( ( reactor_plut * 0.05 ) > 2000 ) {
                        power_gen = 2000;
                    } else {
                        power_gen = reactor_plut * 0.05;
                        if( power_gen < 1 ) {
                            power_gen = 1;
                        }
                    }
                    slow_rad += ( power_gen * 3 );
                    while( slow_rad >= 50 ) {
                        if( power_gen >= 1 ) {
                            slow_rad -= 50;
                            power_gen -= 1;
                            reactor_plut -= 1;
                        } else {
                            break;
                        }
                    }
                } else if( has_bionic( bio_reactor ) ) {
                    if( ( reactor_plut * 0.025 ) > 500 ) {
                        power_gen = 500;
                    } else {
                        power_gen = reactor_plut * 0.025;
                        if( power_gen < 1 ) {
                            power_gen = 1;
                        }
                    }
                    slow_rad += ( power_gen * 3 );
                }
                reactor_plut -= power_gen;
                while( power_gen >= 250 ) {
                    apply_damage( nullptr, bp_torso, 1 );
                    mod_pain( 1 );
                    add_msg_if_player( m_bad, _( "Your chest burns as your power systems overload!" ) );
                    mod_power_level( 50_kJ );
                    power_gen -= 60; // ten units of power lost due to short-circuiting into you
                }
                mod_power_level( units::from_kilojoule( power_gen ) );
            }
        } else {
            slow_rad += ( ( ( reactor_plut * 0.4 ) + ( tank_plut * 0.4 ) ) * 100 );
            //plutonium in body without any kind of container.  Not good at all.
            reactor_plut *= 0.6;
            tank_plut *= 0.6;
        }
        while( slow_rad >= 1000 ) {
            radiation += 1;
            slow_rad -= 1000;
        }
    }

    // Negative bionics effects
    if( has_bionic( bio_dis_shock ) && get_power_level() > 9_kJ && one_turn_in( 2_hours ) &&
        !has_effect( effect_narcosis ) ) {
        add_msg_if_player( m_bad, _( "You suffer a painful electrical discharge!" ) );
        mod_pain( 1 );
        moves -= 150;
        mod_power_level( -10_kJ );

        if( weapon.typeId() == "e_handcuffs" && weapon.charges > 0 ) {
            weapon.charges -= rng( 1, 3 ) * 50;
            if( weapon.charges < 1 ) {
                weapon.charges = 1;
            }

            add_msg_if_player( m_good, _( "The %s seems to be affected by the discharge." ),
                               weapon.tname() );
        }
        sfx::play_variant_sound( "bionics", "elec_discharge", 100 );
    }
    if( has_bionic( bio_dis_acid ) && one_turn_in( 150_minutes ) ) {
        add_msg_if_player( m_bad, _( "You suffer a burning acidic discharge!" ) );
        hurtall( 1, nullptr );
        sfx::play_variant_sound( "bionics", "acid_discharge", 100 );
        sfx::do_player_death_hurt( g->u, false );
    }
    if( has_bionic( bio_drain ) && get_power_level() > 24_kJ && one_turn_in( 1_hours ) ) {
        add_msg_if_player( m_bad, _( "Your batteries discharge slightly." ) );
        mod_power_level( -25_kJ );
        sfx::play_variant_sound( "bionics", "elec_crackle_low", 100 );
    }
    if( has_bionic( bio_noise ) && one_turn_in( 50_minutes ) &&
        !has_effect( effect_narcosis ) ) {
        // TODO: NPCs with said bionic
        if( !is_deaf() ) {
            add_msg( m_bad, _( "A bionic emits a crackle of noise!" ) );
            sfx::play_variant_sound( "bionics", "elec_blast", 100 );
        } else {
            add_msg( m_bad, _( "You feel your faulty bionic shuddering." ) );
            sfx::play_variant_sound( "bionics", "elec_blast_muffled", 100 );
        }
        sounds::sound( pos(), 60, sounds::sound_t::movement, _( "Crackle!" ) ); //sfx above
    }
    if( has_bionic( bio_power_weakness ) && has_max_power() &&
        get_power_level() >= get_max_power_level() * .75 ) {
        mod_str_bonus( -3 );
    }
    if( has_bionic( bio_trip ) && one_turn_in( 50_minutes ) &&
        !has_effect( effect_visuals ) &&
        !has_effect( effect_narcosis ) && !in_sleep_state() ) {
        add_msg_if_player( m_bad, _( "Your vision pixelates!" ) );
        add_effect( effect_visuals, 10_minutes );
        sfx::play_variant_sound( "bionics", "pixelated", 100 );
    }
    if( has_bionic( bio_spasm ) && one_turn_in( 5_hours ) && !has_effect( effect_downed ) &&
        !has_effect( effect_narcosis ) ) {
        add_msg_if_player( m_bad,
                           _( "Your malfunctioning bionic causes you to spasm and fall to the floor!" ) );
        mod_pain( 1 );
        add_effect( effect_stunned, 1_turns );
        add_effect( effect_downed, 1_turns, num_bp, false, 0, true );
        sfx::play_variant_sound( "bionics", "elec_crackle_high", 100 );
    }
    if( has_bionic( bio_shakes ) && get_power_level() > 24_kJ && one_turn_in( 2_hours ) ) {
        add_msg_if_player( m_bad, _( "Your bionics short-circuit, causing you to tremble and shiver." ) );
        mod_power_level( -25_kJ );
        add_effect( effect_shakes, 5_minutes );
        sfx::play_variant_sound( "bionics", "elec_crackle_med", 100 );
    }
    if( has_bionic( bio_leaky ) && one_turn_in( 6_minutes ) ) {
        mod_healthy_mod( -1, -200 );
    }
    if( has_bionic( bio_sleepy ) && one_turn_in( 50_minutes ) && !in_sleep_state() ) {
        mod_fatigue( 1 );
    }
    if( has_bionic( bio_itchy ) && one_turn_in( 50_minutes ) && !has_effect( effect_formication ) &&
        !has_effect( effect_narcosis ) ) {
        add_msg_if_player( m_bad, _( "Your malfunctioning bionic itches!" ) );
        body_part bp = random_body_part( true );
        add_effect( effect_formication, 10_minutes, bp );
    }
    if( has_bionic( bio_glowy ) && !has_effect( effect_glowy_led ) && one_turn_in( 50_minutes ) &&
        get_power_level() > 1_kJ ) {
        add_msg_if_player( m_bad, _( "Your malfunctioning bionic starts to glow!" ) );
        add_effect( effect_glowy_led, 5_minutes );
        mod_power_level( -1_kJ );
    }

    // Artifact effects
    if( has_artifact_with( AEP_ATTENTION ) ) {
        add_effect( effect_attention, 3_turns );
    }

    if( has_artifact_with( AEP_BAD_WEATHER ) && calendar::once_every( 1_minutes ) &&
        g->weather.weather != WEATHER_SNOWSTORM ) {
        g->weather.weather_override = WEATHER_SNOWSTORM;
        g->weather.set_nextweather( calendar::turn );
    }

    // Stim +250 kills
    if( stim > 210 ) {
        if( one_turn_in( 2_minutes ) && !has_effect( effect_downed ) ) {
            add_msg_if_player( m_bad, _( "Your muscles spasm!" ) );
            if( !has_effect( effect_downed ) ) {
                add_msg_if_player( m_bad, _( "You fall to the ground!" ) );
                add_effect( effect_downed, rng( 6_turns, 20_turns ) );
            }
        }
    }
    if( stim > 170 ) {
        if( !has_effect( effect_winded ) && calendar::once_every( 10_minutes ) ) {
            add_msg( m_bad, _( "You feel short of breath." ) );
            add_effect( effect_winded, 10_minutes + 1_turns );
        }
    }
    if( stim > 110 ) {
        if( !has_effect( effect_shakes ) && calendar::once_every( 10_minutes ) ) {
            add_msg( _( "You shake uncontrollably." ) );
            add_effect( effect_shakes, 15_minutes + 1_turns );
        }
    }
    if( stim > 75 ) {
        if( !one_turn_in( 2_minutes ) && !has_effect( effect_nausea ) ) {
            add_msg( _( "You feel nauseous..." ) );
            add_effect( effect_nausea, 5_minutes );
        }
    }

    // Stim -200 or painkillers 240 kills
    if( stim < -160 || pkill > 200 ) {
        if( one_turn_in( 3_minutes ) && !in_sleep_state() ) {
            add_msg_if_player( m_bad, _( "You black out!" ) );
            const time_duration dur = rng( 30_minutes, 60_minutes );
            add_effect( effect_downed, dur );
            add_effect( effect_blind, dur );
            fall_asleep( dur );
        }
    }
    if( stim < -120 || pkill > 160 ) {
        if( !has_effect( effect_winded ) && calendar::once_every( 10_minutes ) ) {
            add_msg( m_bad, _( "Your breathing slows down." ) );
            add_effect( effect_winded, 10_minutes + 1_turns );
        }
    }
    if( stim < -85 || pkill > 145 ) {
        if( one_turn_in( 15_seconds ) && !has_effect( effect_sleep ) ) {
            add_msg_if_player( m_bad, _( "You feel dizzy for a moment." ) );
            mod_moves( -rng( 10, 30 ) );
            if( one_in( 3 ) && !has_effect( effect_downed ) ) {
                add_msg_if_player( m_bad, _( "You stumble and fall over!" ) );
                add_effect( effect_downed, rng( 3_turns, 10_turns ) );
            }
        }
    }
    if( stim < -60 || pkill > 130 ) {
        if( calendar::once_every( 10_minutes ) ) {
            add_msg( m_warning, _( "You feel tired..." ) );
            mod_fatigue( rng( 1, 2 ) );
        }
    }

    int sleep_deprivation = !in_sleep_state() ? get_sleep_deprivation() : 0;
    // Stimulants can lessen the PERCEIVED effects of sleep deprivation, but
    // they do nothing to cure it. As such, abuse is even more dangerous now.
    if( stim > 0 ) {
        // 100% of blacking out = 20160sd ; Max. stim modifier = 12500sd @ 250stim
        // Note: Very high stim already has its own slew of bad effects,
        // so the "useful" part of this bonus is actually lower.
        sleep_deprivation -= stim * 50;
    }

    // Harmless warnings
    if( sleep_deprivation >= SLEEP_DEPRIVATION_HARMLESS ) {
        if( one_turn_in( 50_minutes ) ) {
            switch( dice( 1, 4 ) ) {
                default:
                case 1:
                    add_msg_player_or_npc( m_warning, _( "You tiredly rub your eyes." ),
                                           _( "<npcname> tiredly rubs their eyes." ) );
                    break;
                case 2:
                    add_msg_player_or_npc( m_warning, _( "You let out a small yawn." ),
                                           _( "<npcname> lets out a small yawn." ) );
                    break;
                case 3:
                    add_msg_player_or_npc( m_warning, _( "You stretch your back." ),
                                           _( "<npcname> streches their back." ) );
                    break;
                case 4:
                    add_msg_player_or_npc( m_warning, _( "You feel mentally tired." ),
                                           _( "<npcname> lets out a huge yawn." ) );
                    break;
            }
        }
    }
    // Minor discomfort
    if( sleep_deprivation >= SLEEP_DEPRIVATION_MINOR ) {
        if( one_turn_in( 75_minutes ) ) {
            add_msg_if_player( m_warning, _( "You feel lightheaded for a moment." ) );
            moves -= 10;
        }
        if( one_turn_in( 100_minutes ) ) {
            add_msg_if_player( m_warning, _( "Your muscles spasm uncomfortably." ) );
            mod_pain( 2 );
        }
        if( !has_effect( effect_visuals ) && one_turn_in( 150_minutes ) ) {
            add_msg_if_player( m_warning, _( "Your vision blurs a little." ) );
            add_effect( effect_visuals, rng( 1_minutes, 5_minutes ) );
        }
    }
    // Slight disability
    if( sleep_deprivation >= SLEEP_DEPRIVATION_SERIOUS ) {
        if( one_turn_in( 75_minutes ) ) {
            add_msg_if_player( m_bad, _( "Your mind lapses into unawareness briefly." ) );
            moves -= rng( 20, 80 );
        }
        if( one_turn_in( 125_minutes ) ) {
            add_msg_if_player( m_bad, _( "Your muscles ache in stressfully unpredictable ways." ) );
            mod_pain( rng( 2, 10 ) );
        }
        if( one_turn_in( 5_hours ) ) {
            add_msg_if_player( m_bad, _( "You have a distractingly painful headache." ) );
            mod_pain( rng( 10, 25 ) );
        }
    }
    // Major disability, high chance of passing out also relevant
    if( sleep_deprivation >= SLEEP_DEPRIVATION_MAJOR ) {
        if( !has_effect( effect_nausea ) && one_turn_in( 500_minutes ) ) {
            add_msg_if_player( m_bad, _( "You feel heartburn and an acid taste in your mouth." ) );
            mod_pain( 5 );
            add_effect( effect_nausea, rng( 5_minutes, 30_minutes ) );
        }
        if( one_turn_in( 5_hours ) ) {
            add_msg_if_player( m_bad,
                               _( "Your mind is so tired that you feel you can't trust your eyes anymore." ) );
            add_effect( effect_hallu, rng( 5_minutes, 60_minutes ) );
        }
        if( !has_effect( effect_shakes ) && one_turn_in( 425_minutes ) ) {
            add_msg_if_player( m_bad,
                               _( "Your muscles spasm uncontrollably, and you have trouble keeping your balance." ) );
            add_effect( effect_shakes, 15_minutes );
        } else if( has_effect( effect_shakes ) && one_turn_in( 75_seconds ) ) {
            moves -= 10;
            add_msg_player_or_npc( m_warning, _( "Your shaking legs make you stumble." ),
                                   _( "<npcname> stumbles." ) );
            if( !has_effect( effect_downed ) && one_in( 10 ) ) {
                add_msg_player_or_npc( m_bad, _( "You fall over!" ), _( "<npcname> falls over!" ) );
                add_effect( effect_downed, rng( 3_turns, 10_turns ) );
            }
        }
    }
}

bool player::irradiate( float rads, bool bypass )
{
    int rad_mut = 0;
    if( has_trait( trait_RADIOACTIVE3 ) ) {
        rad_mut = 3;
    } else if( has_trait( trait_RADIOACTIVE2 ) ) {
        rad_mut = 2;
    } else if( has_trait( trait_RADIOACTIVE1 ) ) {
        rad_mut = 1;
    }

    if( rads > 0 ) {
        bool has_helmet = false;
        const bool power_armored = is_wearing_power_armor( &has_helmet );
        const bool rad_resist = power_armored || worn_with_flag( "RAD_RESIST" );

        if( is_rad_immune() && !bypass ) {
            // Power armor and some high-tech gear protects completely from radiation
            rads = 0.0f;
        } else if( rad_resist && !bypass ) {
            rads /= 4.0f;
        }

        if( has_effect( effect_iodine ) ) {
            // Radioactive mutation makes iodine less efficient (but more useful)
            rads *= 0.3f + 0.1f * rad_mut;
        }

        int rads_max = roll_remainder( rads );
        radiation += rng( 0, rads_max );

        // Apply rads to any radiation badges.
        for( item *const it : inv_dump() ) {
            if( it->typeId() != "rad_badge" ) {
                continue;
            }

            // Actual irradiation levels of badges and the player aren't precisely matched.
            // This is intentional.
            const int before = it->irradiation;

            const int delta = rng( 0, rads_max );
            if( delta == 0 ) {
                continue;
            }

            it->irradiation += delta;

            // If in inventory (not worn), don't print anything.
            if( inv.has_item( *it ) ) {
                continue;
            }

            // If the color hasn't changed, don't print anything.
            const std::string &col_before = rad_badge_color( before );
            const std::string &col_after = rad_badge_color( it->irradiation );
            if( col_before == col_after ) {
                continue;
            }

            add_msg_if_player( m_warning, _( "Your radiation badge changes from %1$s to %2$s!" ),
                               col_before, col_after );
        }

        if( rads > 0.0f ) {
            return true;
        }
    }
    return false;
}

// At minimum level, return at_min, at maximum at_max
static float addiction_scaling( float at_min, float at_max, float add_lvl )
{
    // Not addicted
    if( add_lvl < MIN_ADDICTION_LEVEL ) {
        return 1.0f;
    }

    return lerp( at_min, at_max, ( add_lvl - MIN_ADDICTION_LEVEL ) / MAX_ADDICTION_LEVEL );
}

void player::mend( int rate_multiplier )
{
    // Wearing splints can slowly mend a broken limb back to 1 hp.
    bool any_broken = false;
    for( int i = 0; i < num_hp_parts; i++ ) {
        if( hp_cur[i] <= 0 ) {
            any_broken = true;
            break;
        }
    }

    if( !any_broken ) {
        return;
    }

    double healing_factor = 1.0;
    // Studies have shown that alcohol and tobacco use delay fracture healing time
    // Being under effect is 50% slowdown
    // Being addicted but not under effect scales from 25% slowdown to 75% slowdown
    // The improvement from being intoxicated over withdrawal is intended
    if( has_effect( effect_cig ) ) {
        healing_factor *= 0.5;
    } else {
        healing_factor *= addiction_scaling( 0.25f, 0.75f, addiction_level( ADD_CIG ) );
    }

    if( has_effect( effect_drunk ) ) {
        healing_factor *= 0.5;
    } else {
        healing_factor *= addiction_scaling( 0.25f, 0.75f, addiction_level( ADD_ALCOHOL ) );
    }

    if( radiation > 0 && !has_trait( trait_RADIOGENIC ) ) {
        healing_factor *= clamp( ( 1000.0f - radiation ) / 1000.0f, 0.0f, 1.0f );
    }

    // Bed rest speeds up mending
    if( has_effect( effect_sleep ) ) {
        healing_factor *= 4.0;
    } else if( get_fatigue() > DEAD_TIRED ) {
        // but being dead tired does not...
        healing_factor *= 0.75;
    } else {
        // If not dead tired, resting without sleep also helps
        healing_factor *= 1.0f + rest_quality();
    }

    // Being healthy helps.
    healing_factor *= 1.0f + get_healthy() / 200.0f;

    // Very hungry starts lowering the chance
    // square rooting the value makes the numbers drop off faster when below 1
    healing_factor *= sqrt( static_cast<float>( get_stored_kcal() ) / static_cast<float>
                            ( get_healthy_kcal() ) );
    // Similar for thirst - starts at very thirsty, drops to 0 ~halfway between two last statuses
    healing_factor *= 1.0f - clamp( ( get_thirst() - 80.0f ) / 300.0f, 0.0f, 1.0f );

    // Mutagenic healing factor!
    bool needs_splint = true;
    if( has_trait( trait_REGEN_LIZ ) ) {
        healing_factor *= 20.0;
        needs_splint = false;
    } else if( has_trait( trait_REGEN ) ) {
        healing_factor *= 16.0;
    } else if( has_trait( trait_FASTHEALER2 ) ) {
        healing_factor *= 4.0;
    } else if( has_trait( trait_FASTHEALER ) ) {
        healing_factor *= 2.0;
    } else if( has_trait( trait_SLOWHEALER ) ) {
        healing_factor *= 0.5;
    }

    add_msg( m_debug, "Limb mend healing factor: %.2f", healing_factor );
    if( healing_factor <= 0.0f ) {
        // The section below assumes positive healing rate
        return;
    }

    for( int i = 0; i < num_hp_parts; i++ ) {
        const bool broken = ( hp_cur[i] <= 0 );
        if( !broken ) {
            continue;
        }

        body_part part = hp_to_bp( static_cast<hp_part>( i ) );
        if( needs_splint && !worn_with_flag( "SPLINT", part ) ) {
            continue;
        }

        const time_duration dur_inc = 1_turns * roll_remainder( rate_multiplier * healing_factor );
        auto &eff = get_effect( effect_mending, part );
        if( eff.is_null() ) {
            add_effect( effect_mending, dur_inc, part, true );
            continue;
        }

        eff.set_duration( eff.get_duration() + dur_inc );

        if( eff.get_duration() >= eff.get_max_duration() ) {
            hp_cur[i] = 1;
            remove_effect( effect_mending, part );
            g->events().send<event_type::broken_bone_mends>( getID(), part );
            //~ %s is bodypart
            add_msg_if_player( m_good, _( "Your %s has started to mend!" ),
                               body_part_name( part ) );
        }
    }
}

void player::sound_hallu()
{
    // Random 'dangerous' sound from a random direction
    // 1/5 chance to be a loud sound
    std::vector<std::string> dir{ "north",
                                  "northeast",
                                  "northwest",
                                  "south",
                                  "southeast",
                                  "southwest",
                                  "east",
                                  "west" };

    std::vector<std::string> dirz{ "and above you ", "and below you " };

    std::vector<std::tuple<std::string, std::string, std::string>> desc{
        std::make_tuple( "whump!", "smash_fail", "t_door_c" ),
        std::make_tuple( "crash!", "smash_success", "t_door_c" ),
        std::make_tuple( "glass breaking!", "smash_success", "t_window_domestic" ) };

    std::vector<std::tuple<std::string, std::string, std::string>> desc_big{
        std::make_tuple( "huge explosion!", "explosion", "default" ),
        std::make_tuple( "bang!", "fire_gun", "glock_19" ),
        std::make_tuple( "blam!", "fire_gun", "mossberg_500" ),
        std::make_tuple( "crash!", "smash_success", "t_wall" ),
        std::make_tuple( "SMASH!", "smash_success", "t_wall" ) };

    std::string i_dir = dir[rng( 0, dir.size() - 1 )];

    if( one_in( 10 ) ) {
        i_dir += " " + dirz[rng( 0, dirz.size() - 1 )];
    }

    std::string i_desc;
    std::pair<std::string, std::string> i_sound;
    if( one_in( 5 ) ) {
        int r_int = rng( 0, desc_big.size() - 1 );
        i_desc = std::get<0>( desc_big[r_int] );
        i_sound = std::make_pair( std::get<1>( desc_big[r_int] ), std::get<2>( desc_big[r_int] ) );
    } else {
        int r_int = rng( 0, desc.size() - 1 );
        i_desc = std::get<0>( desc[r_int] );
        i_sound = std::make_pair( std::get<1>( desc[r_int] ), std::get<2>( desc[r_int] ) );
    }

    add_msg( m_warning, _( "From the %1$s you hear %2$s" ), i_dir, i_desc );
    sfx::play_variant_sound( i_sound.first, i_sound.second, rng( 20, 80 ) );
}

void player::drench( int saturation, const body_part_set &flags, bool ignore_waterproof )
{
    if( saturation < 1 ) {
        return;
    }

    // OK, water gets in your AEP suit or whatever.  It wasn't built to keep you dry.
    if( has_trait( trait_DEBUG_NOTEMP ) || has_active_mutation( trait_id( "SHELL2" ) ) ||
        ( !ignore_waterproof && is_waterproof( flags ) ) ) {
        return;
    }

    // Make the body wet
    for( const body_part bp : all_body_parts ) {
        // Different body parts have different size, they can only store so much water
        int bp_wetness_max = 0;
        if( flags.test( bp ) ) {
            bp_wetness_max = drench_capacity[bp];
        }

        if( bp_wetness_max == 0 ) {
            continue;
        }
        // Different sources will only make the bodypart wet to a limit
        int source_wet_max = saturation * bp_wetness_max * 2 / 100;
        int wetness_increment = ignore_waterproof ? 100 : 2;
        // Respect maximums
        const int wetness_max = std::min( source_wet_max, bp_wetness_max );
        if( body_wetness[bp] < wetness_max ) {
            body_wetness[bp] = std::min( wetness_max, body_wetness[bp] + wetness_increment );
        }
    }

    if( is_weak_to_water() ) {
        add_msg_if_player( m_bad, _( "You feel the water burning your skin." ) );
    }

    // Remove onfire effect
    if( saturation > 10 || x_in_y( saturation, 10 ) ) {
        remove_effect( effect_onfire );
    }
}

void player::apply_wetness_morale( int temperature )
{
    // First, a quick check if we have any wetness to calculate morale from
    // Faster than checking all worn items for friendliness
    if( !std::any_of( body_wetness.begin(), body_wetness.end(),
    []( const int w ) {
    return w != 0;
} ) ) {
        return;
    }

    // Normalize temperature to [-1.0,1.0]
    temperature = std::max( 0, std::min( 100, temperature ) );
    const double global_temperature_mod = -1.0 + ( 2.0 * temperature / 100.0 );

    int total_morale = 0;
    const auto wet_friendliness = exclusive_flag_coverage( "WATER_FRIENDLY" );
    for( const body_part bp : all_body_parts ) {
        // Sum of body wetness can go up to 103
        const int part_drench = body_wetness[bp];
        if( part_drench == 0 ) {
            continue;
        }

        const auto &part_arr = mut_drench[bp];
        const int part_ignored = part_arr[WT_IGNORED];
        const int part_neutral = part_arr[WT_NEUTRAL];
        const int part_good    = part_arr[WT_GOOD];

        if( part_ignored >= part_drench ) {
            continue;
        }

        int bp_morale = 0;
        const bool is_friendly = wet_friendliness.test( bp );
        const int effective_drench = part_drench - part_ignored;
        if( is_friendly ) {
            // Using entire bonus from mutations and then some "human" bonus
            bp_morale = std::min( part_good, effective_drench ) + effective_drench / 2;
        } else if( effective_drench < part_good ) {
            // Positive or 0
            // Won't go higher than part_good / 2
            // Wet slime/scale doesn't feel as good when covered by wet rags/fur/kevlar
            bp_morale = std::min( effective_drench, part_good - effective_drench );
        } else if( effective_drench > part_good + part_neutral ) {
            // This one will be negative
            bp_morale = part_good + part_neutral - effective_drench;
        }

        // Clamp to [COLD,HOT] and cast to double
        const double part_temperature =
            std::min( BODYTEMP_HOT, std::max( BODYTEMP_COLD, temp_cur[bp] ) );
        // 0.0 at COLD, 1.0 at HOT
        const double part_mod = ( part_temperature - BODYTEMP_COLD ) /
                                ( BODYTEMP_HOT - BODYTEMP_COLD );
        // Average of global and part temperature modifiers, each in range [-1.0, 1.0]
        double scaled_temperature = ( global_temperature_mod + part_mod ) / 2;

        if( bp_morale < 0 ) {
            // Damp, hot clothing on hot skin feels bad
            scaled_temperature = fabs( scaled_temperature );
        }

        // For an unmutated human swimming in deep water, this will add up to:
        // +51 when hot in 100% water friendly clothing
        // -103 when cold/hot in 100% unfriendly clothing
        total_morale += static_cast<int>( bp_morale * ( 1.0 + scaled_temperature ) / 2.0 );
    }

    if( total_morale == 0 ) {
        return;
    }

    int morale_effect = total_morale / 8;
    if( morale_effect == 0 ) {
        if( total_morale > 0 ) {
            morale_effect = 1;
        } else {
            morale_effect = -1;
        }
    }
    // 61_seconds because decay is applied in 1_minutes increments
    add_morale( MORALE_WET, morale_effect, total_morale, 61_seconds, 61_seconds, true );
}

void player::update_body_wetness( const w_point &weather )
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

    for( const body_part bp : all_body_parts ) {
        if( body_wetness[bp] == 0 ) {
            continue;
        }
        // This is to normalize drying times
        int drying_chance = drench_capacity[bp];
        // Body temperature affects duration of wetness
        // Note: Using temp_conv rather than temp_cur, to better approximate environment
        if( temp_conv[bp] >= BODYTEMP_SCORCHING ) {
            drying_chance *= 2;
        } else if( temp_conv[bp] >= BODYTEMP_VERY_HOT ) {
            drying_chance = drying_chance * 3 / 2;
        } else if( temp_conv[bp] >= BODYTEMP_HOT ) {
            drying_chance = drying_chance * 4 / 3;
        } else if( temp_conv[bp] > BODYTEMP_COLD ) {
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
            body_wetness[bp] -= 1;
            if( body_wetness[bp] < 0 ) {
                body_wetness[bp] = 0;
            }
        }
    }
    // TODO: Make clothing slow down drying
}

int player::get_morale_level() const
{
    return morale->get_level();
}

void player::add_morale( const morale_type &type, int bonus, int max_bonus,
                         const time_duration &duration, const time_duration &decay_start,
                         bool capped, const itype *item_type )
{
    morale->add( type, bonus, max_bonus, duration, decay_start, capped, item_type );
}

int player::has_morale( const morale_type &type ) const
{
    return morale->has( type );
}

void player::rem_morale( const morale_type &type, const itype *item_type )
{
    morale->remove( type, item_type );
}

void player::clear_morale()
{
    morale->clear();
}

bool player::has_morale_to_read() const
{
    return get_morale_level() >= -40;
}

void player::check_and_recover_morale()
{
    player_morale test_morale;

    for( const auto &wit : worn ) {
        test_morale.on_item_wear( wit );
    }

    for( const auto &mut : my_mutations ) {
        test_morale.on_mutation_gain( mut.first );
    }

    for( auto &elem : *effects ) {
        for( auto &_effect_it : elem.second ) {
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
        add_msg( m_debug, "%s morale was recovered.", disp_name( true ) );
    }
}

void player::on_worn_item_transform( const item &old_it, const item &new_it )
{
    morale->on_worn_item_transform( old_it, new_it );
}

void player::process_active_items()
{
    if( weapon.needs_processing() && weapon.process( this, pos(), false ) ) {
        weapon = item();
    }

    std::vector<item *> inv_active = inv.active_items();
    for( item *tmp_it : inv_active ) {
        if( tmp_it->process( this, pos(), false ) ) {
            inv.remove_item( tmp_it );
        }
    }

    // worn items
    remove_worn_items_with( [this]( item & itm ) {
        return itm.needs_processing() && itm.process( this, pos(), false );
    } );

    // Active item processing done, now we're recharging.
    item *cloak = nullptr;
    item *power_armor = nullptr;
    std::vector<item *> active_worn_items;
    bool weapon_active = weapon.has_flag( "USE_UPS" ) &&
                         weapon.charges < weapon.type->maximum_charges();
    // Manual iteration because we only care about *worn* active items.
    for( item &w : worn ) {
        if( w.has_flag( "USE_UPS" ) &&
            w.charges < w.type->maximum_charges() ) {
            active_worn_items.push_back( &w );
        }
        if( !w.active ) {
            continue;
        }
        if( cloak == nullptr && w.has_flag( "ACTIVE_CLOAKING" ) ) {
            cloak = &w;
        }
        // Only the main power armor item can be active, the other ones (hauling frame, helmet) aren't.
        if( power_armor == nullptr && w.is_power_armor() ) {
            power_armor = &w;
        }
    }
    std::vector<size_t> active_held_items;
    int ch_UPS = 0;
    for( size_t index = 0; index < inv.size(); index++ ) {
        item &it = inv.find_item( index );
        itype_id identifier = it.type->get_id();
        if( identifier == "UPS_off" ) {
            ch_UPS += it.ammo_remaining();
        } else if( identifier == "adv_UPS_off" ) {
            ch_UPS += it.ammo_remaining() / 0.6;
        }
        if( it.has_flag( "USE_UPS" ) && it.charges < it.type->maximum_charges() ) {
            active_held_items.push_back( index );
        }
    }
    // Necessary for UPS in Aftershock - check worn items for charge
    for( const item &it : worn ) {
        itype_id identifier = it.type->get_id();
        if( identifier == "UPS_off" ) {
            ch_UPS += it.ammo_remaining();
        } else if( identifier == "adv_UPS_off" ) {
            ch_UPS += it.ammo_remaining() / 0.6;
        }
    }
    if( has_active_bionic( bionic_id( "bio_ups" ) ) ) {
        ch_UPS += units::to_kilojoule( get_power_level() );
    }
    int ch_UPS_used = 0;
    if( cloak != nullptr ) {
        if( ch_UPS >= 20 ) {
            use_charges( "UPS", 20 );
            ch_UPS -= 20;
            if( ch_UPS < 200 && one_in( 3 ) ) {
                add_msg_if_player( m_warning, _( "Your cloaking flickers for a moment!" ) );
            }
        } else if( ch_UPS > 0 ) {
            use_charges( "UPS", ch_UPS );
            return;
        } else {
            add_msg_if_player( m_bad,
                               _( "Your cloaking flickers and becomes opaque." ) );
            // Bypass the "you deactivate the ..." message
            cloak->active = false;
            return;
        }
    }

    // For powered armor, an armor-powering bionic should always be preferred over UPS usage.
    if( power_armor != nullptr ) {
        const int power_cost = 4;
        bool bio_powered = can_interface_armor() && has_power();
        // Bionic power costs are handled elsewhere.
        if( !bio_powered ) {
            if( ch_UPS >= power_cost ) {
                use_charges( "UPS", power_cost );
                ch_UPS -= power_cost;
            } else {
                // Deactivate armor here, bypassing the usual deactivation message.
                add_msg_if_player( m_warning, _( "Your power armor disengages." ) );
                power_armor->active = false;
            }
        }
    }

    // Load all items that use the UPS to their minimal functional charge,
    // The tool is not really useful if its charges are below charges_to_use
    for( size_t index : active_held_items ) {
        if( ch_UPS_used >= ch_UPS ) {
            break;
        }
        item &it = inv.find_item( index );
        ch_UPS_used++;
        it.charges++;
    }
    if( weapon_active && ch_UPS_used < ch_UPS ) {
        ch_UPS_used++;
        weapon.charges++;
    }
    for( item *worn_item : active_worn_items ) {
        if( ch_UPS_used >= ch_UPS ) {
            break;
        }
        ch_UPS_used++;
        worn_item->charges++;
    }
    if( ch_UPS_used > 0 ) {
        use_charges( "UPS", ch_UPS_used );
    }
}

item player::reduce_charges( int position, int quantity )
{
    item &it = i_at( position );
    if( it.is_null() ) {
        debugmsg( "invalid item position %d for reduce_charges", position );
        return item();
    }
    if( it.charges <= quantity ) {
        return i_rem( position );
    }
    it.mod_charges( -quantity );
    item tmp( it );
    tmp.charges = quantity;
    return tmp;
}

item player::reduce_charges( item *it, int quantity )
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

int player::invlet_to_position( const int linvlet ) const
{
    // Invlets may come from curses, which may also return any kind of key codes, those being
    // of type int and they can become valid, but different characters when casted to char.
    // Example: KEY_NPAGE (returned when the player presses the page-down key) is 0x152,
    // casted to char would yield 0x52, which happens to be 'R', a valid invlet.
    if( linvlet > std::numeric_limits<char>::max() || linvlet < std::numeric_limits<char>::min() ) {
        return INT_MIN;
    }
    const char invlet = static_cast<char>( linvlet );
    if( is_npc() ) {
        DebugLog( D_WARNING,  D_GAME ) << "Why do you need to call player::invlet_to_position on npc " <<
                                       name;
    }
    if( weapon.invlet == invlet ) {
        return -1;
    }
    auto iter = worn.begin();
    for( size_t i = 0; i < worn.size(); i++, iter++ ) {
        if( iter->invlet == invlet ) {
            return worn_position_to_index( i );
        }
    }
    return inv.invlet_to_position( invlet );
}

bool player::can_interface_armor() const
{
    bool okay = std::any_of( my_bionics->begin(), my_bionics->end(),
    []( const bionic & b ) {
        return b.powered && b.info().armor_interface;
    } );
    return okay;
}

const martialart &player::get_combat_style() const
{
    return style_selected.obj();
}

std::vector<item *> player::inv_dump()
{
    std::vector<item *> ret;
    if( is_armed() && can_unwield( weapon ).success() ) {
        ret.push_back( &weapon );
    }
    for( auto &i : worn ) {
        ret.push_back( &i );
    }
    inv.dump( ret );
    return ret;
}

std::list<item> player::use_amount( itype_id it, int quantity,
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
    std::list<item> tmp = inv.use_amount( it, quantity, filter );
    ret.splice( ret.end(), tmp );
    return ret;
}

bool player::use_charges_if_avail( const itype_id &it, int quantity )
{
    if( has_charges( it, quantity ) ) {
        use_charges( it, quantity );
        return true;
    }
    return false;
}

bool player::has_fire( const int quantity ) const
{
    // TODO: Replace this with a "tool produces fire" flag.

    if( g->m.has_nearby_fire( pos() ) ) {
        return true;
    } else if( has_item_with_flag( "FIRE" ) ) {
        return true;
    } else if( has_item_with_flag( "FIRESTARTER" ) ) {
        auto firestarters = all_items_with_flag( "FIRESTARTER" );
        for( auto &i : firestarters ) {
            if( has_charges( i->typeId(), quantity ) ) {
                return true;
            }
        }
    } else if( has_active_bionic( bio_tools ) && get_power_level() > quantity * 5_kJ ) {
        return true;
    } else if( has_bionic( bio_lighter ) && get_power_level() > quantity * 5_kJ ) {
        return true;
    } else if( has_bionic( bio_laser ) && get_power_level() > quantity * 5_kJ ) {
        return true;
    } else if( is_npc() ) {
        // A hack to make NPCs use their Molotovs
        return true;
    }
    return false;
}

void player::use_fire( const int quantity )
{
    //Okay, so checks for nearby fires first,
    //then held lit torch or candle, bionic tool/lighter/laser
    //tries to use 1 charge of lighters, matches, flame throwers
    //If there is enough power, will use power of one activation of the bio_lighter, bio_tools and bio_laser
    // (home made, military), hotplate, welder in that order.
    // bio_lighter, bio_laser, bio_tools, has_active_bionic("bio_tools"

    if( g->m.has_nearby_fire( pos() ) ) {
        return;
    } else if( has_item_with_flag( "FIRE" ) ) {
        return;
    } else if( has_item_with_flag( "FIRESTARTER" ) ) {
        auto firestarters = all_items_with_flag( "FIRESTARTER" );
        for( auto &i : firestarters ) {
            if( has_charges( i->typeId(), quantity ) ) {
                use_charges( i->typeId(), quantity );
                return;
            }
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

std::list<item> player::use_charges( const itype_id &what, int qty,
                                     const std::function<bool( const item & )> &filter )
{
    std::list<item> res;

    if( qty <= 0 ) {
        return res;

    } else if( what == "toolset" ) {
        mod_power_level( units::from_kilojoule( -qty ) );
        return res;

    } else if( what == "fire" ) {
        use_fire( qty );
        return res;

    } else if( what == "UPS" ) {
        if( is_mounted() && mounted_creature.get()->has_flag( MF_RIDEABLE_MECH ) &&
            mounted_creature.get()->battery_item ) {
            auto mons = mounted_creature.get();
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

        int adv = charges_of( "adv_UPS_off", static_cast<int>( ceil( qty * 0.6 ) ) );
        if( adv > 0 ) {
            std::list<item> found = use_charges( "adv_UPS_off", adv );
            res.splice( res.end(), found );
            qty -= std::min( qty, static_cast<int>( adv / 0.6 ) );
        }

        int ups = charges_of( "UPS_off", qty );
        if( ups > 0 ) {
            std::list<item> found = use_charges( "UPS_off", ups );
            res.splice( res.end(), found );
            qty -= std::min( qty, ups );
        }
        return res;
    }

    std::vector<item *> del;

    bool has_tool_with_UPS = false;
    visit_items( [this, &what, &qty, &res, &del, &has_tool_with_UPS, &filter]( item * e ) {
        if( e->use_charges( what, qty, res, pos(), filter ) ) {
            del.push_back( e );
        }
        if( filter( *e ) && e->typeId() == what && e->has_flag( "USE_UPS" ) ) {
            has_tool_with_UPS = true;
        }
        return qty > 0 ? VisitResponse::SKIP : VisitResponse::ABORT;
    } );

    for( auto e : del ) {
        remove_item( *e );
    }

    if( has_tool_with_UPS ) {
        use_charges( "UPS", qty );
    }

    return res;
}

bool player::covered_with_flag( const std::string &flag, const body_part_set &parts ) const
{
    if( parts.none() ) {
        return true;
    }

    body_part_set to_cover( parts );

    for( const auto &elem : worn ) {
        if( !elem.has_flag( flag ) ) {
            continue;
        }

        to_cover &= ~elem.get_covered_body_parts();

        if( to_cover.none() ) {
            return true;    // Allows early exit.
        }
    }

    return to_cover.none();
}

bool player::is_waterproof( const body_part_set &parts ) const
{
    return covered_with_flag( "WATERPROOF", parts );
}

int player::amount_worn( const itype_id &id ) const
{
    int amount = 0;
    for( auto &elem : worn ) {
        if( elem.typeId() == id ) {
            ++amount;
        }
    }
    return amount;
}

bool player::has_charges( const itype_id &it, int quantity,
                          const std::function<bool( const item & )> &filter ) const
{
    if( it == "fire" || it == "apparatus" ) {
        return has_fire( quantity );
    }
    if( it == "UPS" && is_mounted() &&
        mounted_creature.get()->has_flag( MF_RIDEABLE_MECH ) ) {
        auto mons = mounted_creature.get();
        return quantity <= mons->battery_item->ammo_remaining();
    }
    return charges_of( it, quantity, filter ) == quantity;
}

int  player::leak_level( const std::string &flag ) const
{
    int leak_level = 0;
    leak_level = inv.leak_level( flag );
    return leak_level;
}

bool player::has_mission_item( int mission_id ) const
{
    return mission_id != -1 && has_item_with( has_mission_item_filter{ mission_id } );
}

//Returns the amount of charges that were consumed by the player
int player::drink_from_hands( item &water )
{
    int charges_consumed = 0;
    if( query_yn( _( "Drink %s from your hands?" ),
                  colorize( water.type_name(), water.color_in_inventory() ) ) ) {
        // Create a dose of water no greater than the amount of water remaining.
        item water_temp( water );
        // If player is slaked water might not get consumed.
        consume_item( water_temp );
        charges_consumed = water.charges - water_temp.charges;
        if( charges_consumed > 0 ) {
            moves -= 350;
        }
    }

    return charges_consumed;
}

// TODO: Properly split medications and food instead of hacking around
bool player::consume_med( item &target )
{
    if( !target.is_medication() ) {
        return false;
    }

    const itype_id tool_type = target.get_comestible()->tool;
    const auto req_tool = item::find_type( tool_type );
    bool tool_override = false;
    if( tool_type == "syringe" && has_bionic( bio_syringe ) ) {
        tool_override = true;
    }
    if( req_tool->tool ) {
        if( !( has_amount( tool_type, 1 ) && has_charges( tool_type, req_tool->tool->charges_per_use ) ) &&
            !tool_override ) {
            add_msg_if_player( m_info, _( "You need a %s to consume that!" ), req_tool->nname( 1 ) );
            return false;
        }
        use_charges( tool_type, req_tool->tool->charges_per_use );
    }

    int amount_used = 1;
    if( target.type->has_use() ) {
        amount_used = target.type->invoke( *this, target, pos() );
        if( amount_used <= 0 ) {
            return false;
        }
    }

    // TODO: Get the target it was used on
    // Otherwise injecting someone will give us addictions etc.
    if( target.has_flag( "NO_INGEST" ) ) {
        const auto &comest = *target.get_comestible();
        // Assume that parenteral meds don't spoil, so don't apply rot
        modify_health( comest );
        modify_stimulation( comest );
        modify_addiction( comest );
        modify_morale( target );
    } else {
        // Take by mouth
        consume_effects( target );
    }

    mod_moves( -250 );
    target.charges -= amount_used;
    return target.charges <= 0;
}

static bool query_consume_ownership( item &target, player &p )
{
    if( !target.is_owned_by( p, true ) ) {
        bool choice = true;
        if( p.get_value( "THIEF_MODE" ) == "THIEF_ASK" ) {
            choice = Pickup::query_thief();
        }
        if( p.get_value( "THIEF_MODE" ) == "THIEF_HONEST" || !choice ) {
            return false;
        }
        std::vector<npc *> witnesses;
        for( npc &elem : g->all_npcs() ) {
            if( rl_dist( elem.pos(), p.pos() ) < MAX_VIEW_DISTANCE && elem.sees( p.pos() ) ) {
                witnesses.push_back( &elem );
            }
        }
        for( npc *elem : witnesses ) {
            elem->say( "<witnessed_thievery>", 7 );
        }
        if( !witnesses.empty() && target.is_owned_by( p, true ) ) {
            if( g->u.add_faction_warning( target.get_owner() ) ) {
                for( npc *elem : witnesses ) {
                    elem->make_angry();
                }
            }
        }
    }
    return true;
}

bool player::consume_item( item &target )
{
    if( target.is_null() ) {
        add_msg_if_player( m_info, _( "You do not have that item." ) );
        return false;
    }
    if( is_underwater() && !has_trait( trait_WATERSLEEP ) ) {
        add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return false;
    }

    item &comest = get_consumable_from( target );

    if( comest.is_null() || target.is_craft() ) {
        add_msg_if_player( m_info, _( "You can't eat your %s." ), target.tname() );
        if( is_npc() ) {
            debugmsg( "%s tried to eat a %s", name, target.tname() );
        }
        return false;
    }
    if( is_player() && !query_consume_ownership( target, *this ) ) {
        return false;
    }
    if( consume_med( comest ) ||
        eat( comest ) ||
        feed_battery_with( comest ) ||
        feed_reactor_with( comest ) ||
        feed_furnace_with( comest ) || fuel_bionic_with( comest ) ) {

        if( target.is_container() ) {
            target.on_contents_changed();
        }

        return comest.charges <= 0;
    }

    return false;
}

bool player::consume( int target_position )
{
    auto &target = i_at( target_position );
    if( consume_item( target ) ) {

        const bool was_in_container = !can_consume_as_is( target );

        if( was_in_container ) {
            i_rem( &target.contents.front() );
        } else {
            i_rem( &target );
        }

        //Restack and sort so that we don't lie about target's invlet
        if( target_position >= 0 ) {
            inv.restack( *this );
        }

        if( was_in_container && target_position == -1 ) {
            add_msg_if_player( _( "You are now wielding an empty %s." ), weapon.tname() );
        } else if( was_in_container && target_position < -1 ) {
            add_msg_if_player( _( "You are now wearing an empty %s." ), target.tname() );
        } else if( was_in_container && !is_npc() ) {
            bool drop_it = false;
            if( get_option<std::string>( "DROP_EMPTY" ) == "no" ) {
                drop_it = false;
            } else if( get_option<std::string>( "DROP_EMPTY" ) == "watertight" ) {
                drop_it = !target.is_watertight_container();
            } else if( get_option<std::string>( "DROP_EMPTY" ) == "all" ) {
                drop_it = true;
            }
            if( drop_it ) {
                add_msg( _( "You drop the empty %s." ), target.tname() );
                put_into_vehicle_or_drop( *this, item_drop_reason::deliberate, { inv.remove_item( &target ) } );
            } else {
                int quantity = inv.const_stack( inv.position_by_item( &target ) ).size();
                char letter = target.invlet ? target.invlet : ' ';
                add_msg( m_info, _( "%c - %d empty %s" ), letter, quantity, target.tname( quantity ) );
            }
        }
    } else if( target_position >= 0 ) {
        if( Pickup::handle_spillable_contents( *this, target, g->m ) ) {
            i_rem( &target );
        }
        inv.restack( *this );
        inv.unsort();
    }

    return true;
}

bool player::add_faction_warning( const faction_id &id )
{
    const auto it = warning_record.find( id );
    if( it != warning_record.end() ) {
        it->second.first += 1;
        if( it->second.second - calendar::turn > 5_minutes ) {
            it->second.first -= 1;
        }
        it->second.second = calendar::turn;
        if( it->second.first > 3 ) {
            return true;
        }
    } else {
        warning_record[id] = std::make_pair( 1, calendar::turn );
    }
    faction *fac = g->faction_manager_ptr->get( id );
    if( fac != nullptr && is_player() && fac->id != faction_id( "no_faction" ) ) {
        fac->likes_u -= 1;
        fac->respects_u -= 1;
    }
    return false;
}

int player::current_warnings_fac( const faction_id &id )
{
    const auto it = warning_record.find( id );
    if( it != warning_record.end() ) {
        if( it->second.second - calendar::turn > 5_minutes ) {
            it->second.first = std::max( 0,
                                         it->second.first - 1 );
        }
        return it->second.first;
    }
    return 0;
}

bool player::beyond_final_warning( const faction_id &id )
{
    const auto it = warning_record.find( id );
    if( it != warning_record.end() ) {
        if( it->second.second - calendar::turn > 5_minutes ) {
            it->second.first = std::max( 0,
                                         it->second.first - 1 );
        }
        return it->second.first > 3;
    }
    return false;
}

item::reload_option player::select_ammo( const item &base,
        std::vector<item::reload_option> opts ) const
{
    if( opts.empty() ) {
        add_msg_if_player( m_info, _( "Never mind." ) );
        return item::reload_option();
    }

    uilist menu;
    menu.text = string_format( base.is_watertight_container() ? _( "Refill %s" ) :
                               base.has_flag( "RELOAD_AND_SHOOT" ) ? _( "Select ammo for %s" ) : _( "Reload %s" ),
                               base.tname() );
    menu.w_width = -1;
    menu.w_height = -1;

    // Construct item names
    std::vector<std::string> names;
    std::transform( opts.begin(), opts.end(),
    std::back_inserter( names ), [&]( const item::reload_option & e ) {
        if( e.ammo->is_magazine() && e.ammo->ammo_data() ) {
            if( e.ammo->ammo_current() == "battery" ) {
                // This battery ammo is not a real object that can be recovered but pseudo-object that represents charge
                //~ battery storage (charges)
                return string_format( pgettext( "magazine", "%1$s (%2$d)" ), e.ammo->type_name(),
                                      e.ammo->ammo_remaining() );
            } else {
                //~ magazine with ammo (count)
                return string_format( pgettext( "magazine", "%1$s with %2$s (%3$d)" ), e.ammo->type_name(),
                                      e.ammo->ammo_data()->nname( e.ammo->ammo_remaining() ), e.ammo->ammo_remaining() );
            }
        } else if( e.ammo->is_watertight_container() ||
                   ( e.ammo->is_ammo_container() && g->u.is_worn( *e.ammo ) ) ) {
            // worn ammo containers should be named by their contents with their location also updated below
            return e.ammo->contents.front().display_name();

        } else {
            return ( ammo_location && ammo_location == e.ammo ? "* " : "" ) + e.ammo->display_name();
        }
    } );

    // Get location descriptions
    std::vector<std::string> where;
    std::transform( opts.begin(), opts.end(),
    std::back_inserter( where ), []( const item::reload_option & e ) {
        bool is_ammo_container = e.ammo->is_ammo_container();
        if( is_ammo_container || e.ammo->is_container() ) {
            if( is_ammo_container && g->u.is_worn( *e.ammo ) ) {
                return e.ammo->type_name();
            }
            return string_format( _( "%s, %s" ), e.ammo->type_name(), e.ammo.describe( &g->u ) );
        }
        return e.ammo.describe( &g->u );
    } );

    // Pads elements to match longest member and return length
    auto pad = []( std::vector<std::string> &vec, int n, int t ) -> int {
        for( const auto &e : vec )
        {
            n = std::max( n, utf8_width( e, true ) + t );
        }
        for( auto &e : vec )
        {
            e += std::string( n - utf8_width( e, true ), ' ' );
        }
        return n;
    };

    // Pad the first column including 4 trailing spaces
    int w = pad( names, utf8_width( menu.text, true ), 6 );
    menu.text.insert( 0, 2, ' ' ); // add space for UI hotkeys
    menu.text += std::string( w + 2 - utf8_width( menu.text, true ), ' ' );

    // Pad the location similarly (excludes leading "| " and trailing " ")
    w = pad( where, utf8_width( _( "| Location " ) ) - 3, 6 );
    menu.text += _( "| Location " );
    menu.text += std::string( w + 3 - utf8_width( _( "| Location " ) ), ' ' );

    menu.text += _( "| Amount  " );
    menu.text += _( "| Moves   " );

    // We only show ammo statistics for guns and magazines
    if( base.is_gun() || base.is_magazine() ) {
        menu.text += _( "| Damage  | Pierce  " );
    }

    auto draw_row = [&]( int idx ) {
        const auto &sel = opts[ idx ];
        std::string row = string_format( "%s| %s |", names[ idx ], where[ idx ] );
        row += string_format( ( sel.ammo->is_ammo() ||
                                sel.ammo->is_ammo_container() ) ? " %-7d |" : "         |", sel.qty() );
        row += string_format( " %-7d ", sel.moves() );

        if( base.is_gun() || base.is_magazine() ) {
            const itype *ammo = sel.ammo->is_ammo_container() ? sel.ammo->contents.front().ammo_data() :
                                sel.ammo->ammo_data();
            if( ammo ) {
                if( ammo->ammo->prop_damage ) {
                    row += string_format( "| *%-6.2f | %-7d", static_cast<float>( *ammo->ammo->prop_damage ),
                                          ammo->ammo->legacy_pierce );
                } else {
                    const damage_instance &dam = ammo->ammo->damage;
                    row += string_format( "| %-7d | %-7d", static_cast<int>( dam.total_damage() ),
                                          static_cast<int>( dam.empty() ? 0.0f : ( *dam.begin() ).res_pen ) );
                }
            } else {
                row += "|         |         ";
            }
        }
        return row;
    };

    itype_id last = uistate.lastreload[ ammotype( base.ammo_default() ) ];
    // We keep the last key so that pressing the key twice (for example, r-r for reload)
    // will always pick the first option on the list.
    int last_key = inp_mngr.get_previously_pressed_key();
    bool last_key_bound = false;
    // This is the entry that has out default
    int default_to = 0;

    // If last_key is RETURN, don't use that to override hotkey
    if( last_key == '\n' ) {
        last_key_bound = true;
        default_to = -1;
    }

    for( auto i = 0; i < static_cast<int>( opts.size() ); ++i ) {
        const item &ammo = opts[ i ].ammo->is_ammo_container() ? opts[ i ].ammo->contents.front() :
                           *opts[ i ].ammo;

        char hotkey = -1;
        if( g->u.has_item( ammo ) ) {
            // if ammo in player possession and either it or any container has a valid invlet use this
            if( ammo.invlet ) {
                hotkey = ammo.invlet;
            } else {
                for( const auto obj : g->u.parents( ammo ) ) {
                    if( obj->invlet ) {
                        hotkey = obj->invlet;
                        break;
                    }
                }
            }
        }
        if( last == ammo.typeId() ) {
            if( !last_key_bound && hotkey == -1 ) {
                // If this is the first occurrence of the most recently used type of ammo and the hotkey
                // was not already set above then set it to the keypress that opened this prompt
                hotkey = last_key;
                last_key_bound = true;
            }
            if( !last_key_bound ) {
                // Pressing the last key defaults to the first entry of compatible type
                default_to = i;
                last_key_bound = true;
            }
        }
        if( hotkey == last_key ) {
            last_key_bound = true;
            // Prevent the default from being used: key is bound to something already
            default_to = -1;
        }

        menu.addentry( i, true, hotkey, draw_row( i ) );
    }

    struct reload_callback : public uilist_callback {
        public:
            std::vector<item::reload_option> &opts;
            const std::function<std::string( int )> draw_row;
            int last_key;
            const int default_to;
            const bool can_partial_reload;

            reload_callback( std::vector<item::reload_option> &_opts,
                             std::function<std::string( int )> _draw_row,
                             int _last_key, int _default_to, bool _can_partial_reload ) :
                opts( _opts ), draw_row( _draw_row ),
                last_key( _last_key ), default_to( _default_to ),
                can_partial_reload( _can_partial_reload )
            {}

            bool key( const input_context &, const input_event &event, int idx, uilist *menu ) override {
                auto cur_key = event.get_first_input();
                if( default_to != -1 && cur_key == last_key ) {
                    // Select the first entry on the list
                    menu->ret = default_to;
                    return true;
                }
                if( idx < 0 || idx >= static_cast<int>( opts.size() ) ) {
                    return false;
                }
                auto &sel = opts[ idx ];
                switch( cur_key ) {
                    case KEY_LEFT:
                        if( can_partial_reload ) {
                            sel.qty( sel.qty() - 1 );
                            menu->entries[ idx ].txt = draw_row( idx );
                        }
                        return true;

                    case KEY_RIGHT:
                        if( can_partial_reload ) {
                            sel.qty( sel.qty() + 1 );
                            menu->entries[ idx ].txt = draw_row( idx );
                        }
                        return true;
                }
                return false;
            }
    } cb( opts, draw_row, last_key, default_to, !base.has_flag( "RELOAD_ONE" ) );
    menu.callback = &cb;

    menu.query();
    if( menu.ret < 0 || static_cast<size_t>( menu.ret ) >= opts.size() ) {
        add_msg_if_player( m_info, _( "Never mind." ) );
        return item::reload_option();
    }

    const item_location &sel = opts[ menu.ret ].ammo;
    uistate.lastreload[ ammotype( base.ammo_default() ) ] = sel->is_ammo_container() ?
            sel->contents.front().typeId() :
            sel->typeId();
    return opts[ menu.ret ] ;
}

bool player::list_ammo( const item &base, std::vector<item::reload_option> &ammo_list,
                        bool empty ) const
{
    auto opts = base.gunmods();
    opts.push_back( &base );

    if( base.magazine_current() ) {
        opts.push_back( base.magazine_current() );
    }

    for( const auto mod : base.gunmods() ) {
        if( mod->magazine_current() ) {
            opts.push_back( mod->magazine_current() );
        }
    }

    bool ammo_match_found = false;
    for( const auto e : opts ) {
        for( item_location &ammo : find_ammo( *e, empty ) ) {
            // don't try to unload frozen liquids
            if( ammo->is_watertight_container() && ammo->contents_made_of( SOLID ) ) {
                continue;
            }
            auto id = ( ammo->is_ammo_container() || ammo->is_container() )
                      ? ammo->contents.front().typeId()
                      : ammo->typeId();
            if( e->can_reload_with( id ) ) {
                // Speedloaders require an empty target.
                if( !ammo->has_flag( "SPEEDLOADER" ) || e->ammo_remaining() < 1 ) {
                    ammo_match_found = true;
                }
            }
            if( can_reload( *e, id ) || e->has_flag( "RELOAD_AND_SHOOT" ) ) {
                ammo_list.emplace_back( this, e, &base, std::move( ammo ) );
            }
        }
    }
    return ammo_match_found;
}

item::reload_option player::select_ammo( const item &base, bool prompt, bool empty ) const
{
    std::vector<item::reload_option> ammo_list;
    bool ammo_match_found = list_ammo( base, ammo_list, empty );

    if( ammo_list.empty() ) {
        if( !base.is_magazine() && !base.magazine_integral() && !base.magazine_current() ) {
            add_msg_if_player( m_info, _( "You need a compatible magazine to reload the %s!" ),
                               base.tname() );

        } else if( ammo_match_found ) {
            add_msg_if_player( m_info, _( "Nothing to reload!" ) );
        } else {
            std::string name;
            if( base.ammo_data() ) {
                name = base.ammo_data()->nname( 1 );
            } else if( base.is_watertight_container() ) {
                name = base.is_container_empty() ? "liquid" : base.contents.front().tname();
            } else {
                name = enumerate_as_string( base.ammo_types().begin(),
                base.ammo_types().end(), []( const ammotype & at ) {
                    return at->name();
                }, enumeration_conjunction::none );
            }
            add_msg_if_player( m_info, _( "You don't have any %s to reload your %s!" ),
                               name, base.tname() );
        }
        return item::reload_option();
    }

    // sort in order of move cost (ascending), then remaining ammo (descending) with empty magazines always last
    std::stable_sort( ammo_list.begin(), ammo_list.end(), []( const item::reload_option & lhs,
    const item::reload_option & rhs ) {
        return lhs.ammo->ammo_remaining() > rhs.ammo->ammo_remaining();
    } );
    std::stable_sort( ammo_list.begin(), ammo_list.end(), []( const item::reload_option & lhs,
    const item::reload_option & rhs ) {
        return lhs.moves() < rhs.moves();
    } );
    std::stable_sort( ammo_list.begin(), ammo_list.end(), []( const item::reload_option & lhs,
    const item::reload_option & rhs ) {
        return ( lhs.ammo->ammo_remaining() != 0 ) > ( rhs.ammo->ammo_remaining() != 0 );
    } );

    if( is_npc() ) {
        return ammo_list[ 0 ] ;
    }

    if( !prompt && ammo_list.size() == 1 ) {
        // unconditionally suppress the prompt if there's only one option
        return ammo_list[ 0 ] ;
    }

    return select_ammo( base, std::move( ammo_list ) );
}

ret_val<bool> player::can_wear( const item &it ) const
{
    if( !it.is_armor() ) {
        return ret_val<bool>::make_failure( _( "Putting on a %s would be tricky." ), it.tname() );
    }

    if( it.is_power_armor() ) {
        for( auto &elem : worn ) {
            if( ( elem.get_covered_body_parts() & it.get_covered_body_parts() ).any() ) {
                return ret_val<bool>::make_failure( _( "Can't wear power armor over other gear!" ) );
            }
        }
        if( !it.covers( bp_torso ) ) {
            bool power_armor = false;
            if( !worn.empty() ) {
                for( auto &elem : worn ) {
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

        for( auto &i : worn ) {
            if( i.is_power_armor() && i.typeId() == it.typeId() ) {
                return ret_val<bool>::make_failure( _( "Can't wear more than one %s!" ), it.tname() );
            }
        }
    } else {
        // Only headgear can be worn with power armor, except other power armor components.
        // You can't wear headgear if power armor helmet is already sitting on your head.
        bool has_helmet = false;
        if( is_wearing_power_armor( &has_helmet ) &&
            ( has_helmet || !( it.covers( bp_head ) || it.covers( bp_mouth ) || it.covers( bp_eyes ) ) ) ) {
            return ret_val<bool>::make_failure( _( "Can't wear %s with power armor!" ), it.tname() );
        }
    }

    // Check if we don't have both hands available before wearing a briefcase, shield, etc. Also occurs if we're already wearing one.
    if( it.has_flag( "RESTRICT_HANDS" ) && ( !has_two_arms() || worn_with_flag( "RESTRICT_HANDS" ) ||
            weapon.is_two_handed( *this ) ) ) {
        return ret_val<bool>::make_failure( ( is_player() ? _( "You don't have a hand free to wear that." )
                                              : string_format( _( "%s doesn't have a hand free to wear that." ), name ) ) );
    }

    for( auto &i : worn ) {
        if( i.has_flag( "ONLY_ONE" ) && i.typeId() == it.typeId() ) {
            return ret_val<bool>::make_failure( _( "Can't wear more than one %s!" ), it.tname() );
        }
    }

    if( amount_worn( it.typeId() ) >= MAX_WORN_PER_TYPE ) {
        return ret_val<bool>::make_failure( _( "Can't wear %i or more %s at once." ),
                                            MAX_WORN_PER_TYPE + 1, it.tname( MAX_WORN_PER_TYPE + 1 ) );
    }

    if( ( ( it.covers( bp_foot_l ) && is_wearing_shoes( side::LEFT ) ) ||
          ( it.covers( bp_foot_r ) && is_wearing_shoes( side::RIGHT ) ) ) &&
        ( !it.has_flag( "OVERSIZE" ) || !it.has_flag( "OUTER" ) ) && !it.has_flag( "SKINTIGHT" ) &&
        !it.has_flag( "BELTED" ) && !it.has_flag( "PERSONAL" ) && !it.has_flag( "AURA" ) &&
        !it.has_flag( "SEMITANGIBLE" ) ) {
        // Checks to see if the player is wearing shoes
        return ret_val<bool>::make_failure( ( is_player() ? _( "You're already wearing footwear!" )
                                              : string_format( _( "%s is already wearing footwear!" ), name ) ) );
    }

    if( it.covers( bp_head ) &&
        !it.has_flag( "HELMET_COMPAT" ) && !it.has_flag( "SKINTIGHT" ) && !it.has_flag( "PERSONAL" ) &&
        !it.has_flag( "AURA" ) && !it.has_flag( "SEMITANGIBLE" ) && !it.has_flag( "OVERSIZE" ) &&
        is_wearing_helmet() ) {
        return ret_val<bool>::make_failure( wearing_something_on( bp_head ),
                                            ( is_player() ? _( "You can't wear that with other headgear!" )
                                              : string_format( _( "%s can't wear that with other headgear!" ), name ) ) );
    }

    if( it.covers( bp_head ) && !it.has_flag( "SEMITANGIBLE" ) &&
        ( it.has_flag( "SKINTIGHT" ) || it.has_flag( "HELMET_COMPAT" ) ) &&
        ( head_cloth_encumbrance() + it.get_encumber( *this ) > 40 ) ) {
        return ret_val<bool>::make_failure( ( is_player() ? _( "You can't wear that much on your head!" )
                                              : string_format( _( "%s can't wear that much on their head!" ), name ) ) );
    }

    if( has_trait( trait_WOOLALLERGY ) && ( it.made_of( material_id( "wool" ) ) ||
                                            it.item_tags.count( "wooled" ) ) ) {
        return ret_val<bool>::make_failure( _( "Can't wear that, it's made of wool!" ) );
    }

    if( it.is_filthy() && has_trait( trait_SQUEAMISH ) ) {
        return ret_val<bool>::make_failure( _( "Can't wear that, it's filthy!" ) );
    }

    if( !it.has_flag( "OVERSIZE" ) && !it.has_flag( "SEMITANGIBLE" ) ) {
        for( const trait_id &mut : get_mutations() ) {
            const auto &branch = mut.obj();
            if( branch.conflicts_with_item( it ) ) {
                return ret_val<bool>::make_failure( _( "Your %s mutation prevents you from wearing your %s." ),
                                                    branch.name(), it.type_name() );
            }
        }
        if( it.covers( bp_head ) && !it.has_flag( "SEMITANGIBLE" ) &&
            !it.made_of( material_id( "wool" ) ) && !it.made_of( material_id( "cotton" ) ) &&
            !it.made_of( material_id( "nomex" ) ) && !it.made_of( material_id( "leather" ) ) &&
            ( has_trait( trait_HORNS_POINTED ) || has_trait( trait_ANTENNAE ) ||
              has_trait( trait_ANTLERS ) ) ) {
            return ret_val<bool>::make_failure( _( "Cannot wear a helmet over %s." ),
                                                ( has_trait( trait_HORNS_POINTED ) ? _( "horns" ) :
                                                  ( has_trait( trait_ANTENNAE ) ? _( "antennae" ) : _( "antlers" ) ) ) );
        }
    }

    return ret_val<bool>::make_success();
}

ret_val<bool> player::can_wield( const item &it ) const
{
    if( it.made_of_from_type( LIQUID ) ) {
        return ret_val<bool>::make_failure( _( "Can't wield spilt liquids." ) );
    }

    if( get_working_arm_count() <= 0 ) {
        return ret_val<bool>::make_failure(
                   _( "You need at least one arm to even consider wielding something." ) );
    }

    if( it.is_two_handed( *this ) && ( !has_two_arms() || worn_with_flag( "RESTRICT_HANDS" ) ) ) {
        if( worn_with_flag( "RESTRICT_HANDS" ) ) {
            return ret_val<bool>::make_failure(
                       _( "Something you are wearing hinders the use of both hands." ) );
        } else if( it.has_flag( "ALWAYS_TWOHAND" ) ) {
            return ret_val<bool>::make_failure( _( "The %s can't be wielded with only one arm." ),
                                                it.tname() );
        } else {
            return ret_val<bool>::make_failure( _( "You are too weak to wield %s with only one arm." ),
                                                it.tname() );
        }
    }

    return ret_val<bool>::make_success();
}

ret_val<bool> player::can_unwield( const item &it ) const
{
    if( it.has_flag( "NO_UNWIELD" ) ) {
        return ret_val<bool>::make_failure( _( "You cannot unwield your %s." ), it.tname() );
    }

    return ret_val<bool>::make_success();
}

bool player::is_wielding( const item &target ) const
{
    return &weapon == &target;
}

bool player::unwield()
{
    if( weapon.is_null() ) {
        return true;
    }

    if( !can_unwield( weapon ).success() ) {
        return false;
    }

    const std::string query = string_format( _( "Stop wielding %s?" ), weapon.tname() );

    if( !dispose_item( item_location( *this, &weapon ), query ) ) {
        return false;
    }

    inv.unsort();

    return true;
}

// ids of martial art styles that are available with the bio_cqb bionic.
static const std::vector<matype_id> bio_cqb_styles{ {
        matype_id{ "style_aikido" },
        matype_id{ "style_biojutsu" },
        matype_id{ "style_boxing" },
        matype_id{ "style_capoeira" },
        matype_id{ "style_crane" },
        matype_id{ "style_dragon" },
        matype_id{ "style_judo" },
        matype_id{ "style_karate" },
        matype_id{ "style_krav_maga" },
        matype_id{ "style_leopard" },
        matype_id{ "style_muay_thai" },
        matype_id{ "style_ninjutsu" },
        matype_id{ "style_pankration" },
        matype_id{ "style_snake" },
        matype_id{ "style_taekwondo" },
        matype_id{ "style_tai_chi" },
        matype_id{ "style_tiger" },
        matype_id{ "style_wingchun" },
        matype_id{ "style_zui_quan" }
    }};

bool player::pick_style() // Style selection menu
{
    enum style_selection {
        KEEP_HANDS_FREE = 0,
        STYLE_OFFSET
    };

    // If there are style already, cursor starts there
    // if no selected styles, cursor starts from no-style

    // Any other keys quit the menu
    const std::vector<matype_id> &selectable_styles = has_active_bionic( bio_cqb ) ? bio_cqb_styles :
            ma_styles;

    input_context ctxt( "MELEE_STYLE_PICKER" );
    ctxt.register_action( "SHOW_DESCRIPTION" );

    uilist kmenu;
    kmenu.text = string_format( _( "Select a style.  (press %s for more info)" ),
                                ctxt.get_desc( "SHOW_DESCRIPTION" ) );
    ma_style_callback callback( static_cast<size_t>( STYLE_OFFSET ), selectable_styles );
    kmenu.callback = &callback;
    kmenu.input_category = "MELEE_STYLE_PICKER";
    kmenu.additional_actions.emplace_back( "SHOW_DESCRIPTION", "" );
    kmenu.desc_enabled = true;
    kmenu.addentry_desc( KEEP_HANDS_FREE, true, 'h',
                         keep_hands_free ? _( "Keep hands free (on)" ) : _( "Keep hands free (off)" ),
                         _( "When this is enabled, player won't wield things unless explicitly told to." ) );

    kmenu.selected = STYLE_OFFSET;

    for( size_t i = 0; i < selectable_styles.size(); i++ ) {
        auto &style = selectable_styles[i].obj();
        //Check if this style is currently selected
        const bool selected = selectable_styles[i] == style_selected;
        std::string entry_text = style.name.translated();
        if( selected ) {
            kmenu.selected = i + STYLE_OFFSET;
            entry_text = colorize( entry_text, c_pink );
        }
        kmenu.addentry_desc( i + STYLE_OFFSET, true, -1, entry_text, style.description.translated() );
    }

    kmenu.query();
    int selection = kmenu.ret;

    if( selection >= STYLE_OFFSET ) {
        style_selected = selectable_styles[selection - STYLE_OFFSET];
        martialart_use_message();
    } else if( selection == KEEP_HANDS_FREE ) {
        keep_hands_free = !keep_hands_free;
    } else {
        return false;
    }

    return true;
}

hint_rating player::rate_action_wear( const item &it ) const
{
    // TODO: flag already-worn items as HINT_IFFY

    if( !it.is_armor() ) {
        return HINT_CANT;
    }

    return can_wear( it ).success() ? HINT_GOOD : HINT_IFFY;
}

hint_rating player::rate_action_change_side( const item &it ) const
{
    if( !is_worn( it ) ) {
        return HINT_IFFY;
    }

    if( !it.is_sided() ) {
        return HINT_CANT;
    }

    return HINT_GOOD;
}

bool player::can_reload( const item &it, const itype_id &ammo ) const
{
    if( !it.is_reloadable_with( ammo ) ) {
        return false;
    }

    if( it.is_ammo_belt() ) {
        const auto &linkage = it.type->magazine->linkage;
        if( linkage && !has_charges( *linkage, 1 ) ) {
            return false;
        }
    }

    return true;
}

bool player::dispose_item( item_location &&obj, const std::string &prompt )
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

    const bool bucket = obj->is_bucket_nonempty();

    opts.emplace_back( dispose_option {
        bucket ? _( "Spill contents and store in inventory" ) : _( "Store in inventory" ),
        volume_carried() + obj->volume() <= volume_capacity(), '1',
        item_handling_cost( *obj ),
        [this, bucket, &obj] {
            if( bucket && !obj->spill_contents( *this ) )
            {
                return false;
            }

            moves -= item_handling_cost( *obj );
            inv.add_item_keep_invlet( *obj );
            inv.unsort();
            obj.remove_item();
            return true;
        }
    } );

    opts.emplace_back( dispose_option {
        _( "Drop item" ), true, '2', 0, [this, &obj] {
            put_into_vehicle_or_drop( *this, item_drop_reason::deliberate, { *obj } );
            obj.remove_item();
            return true;
        }
    } );

    opts.emplace_back( dispose_option {
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
            auto ptr = dynamic_cast<const holster_actor *>( e.type->get_use( "holster" )->get_actor_ptr() );
            opts.emplace_back( dispose_option {
                string_format( _( "Store in %s" ), e.tname() ), true, e.invlet,
                item_store_cost( *obj, e, false, ptr->draw_cost ),
                [this, ptr, &e, &obj]{
                    return ptr->store( *this, e, *obj );
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
        return opts[ menu.ret ].action();
    }
    return false;
}

void player::mend_item( item_location &&obj, bool interactive )
{
    if( g->u.has_trait( trait_DEBUG_HS ) ) {
        uilist menu;
        menu.text = _( "Toggle which fault?" );
        std::vector<std::pair<fault_id, bool>> opts;
        for( const auto &f : obj->faults_potential() ) {
            opts.emplace_back( f, !!obj->faults.count( f ) );
            menu.addentry( -1, true, -1, string_format(
                               opts.back().second ? pgettext( "fault", "Mend: %s" ) : pgettext( "fault", "Set: %s" ),
                               f.obj().name() ) );
        }
        if( opts.empty() ) {
            add_msg( m_info, _( "The %s doesn't have any faults to toggle." ), obj->tname() );
            return;
        }
        menu.query();
        if( menu.ret >= 0 ) {
            if( opts[ menu.ret ].second ) {
                obj->faults.erase( opts[ menu.ret ].first );
            } else {
                obj->faults.insert( opts[ menu.ret ].first );
            }
        }
        return;
    }

    std::vector<std::pair<const fault *, bool>> faults;
    std::transform( obj->faults.begin(), obj->faults.end(),
    std::back_inserter( faults ), []( const fault_id & e ) {
        return std::make_pair<const fault *, bool>( &e.obj(), false );
    } );

    if( faults.empty() ) {
        if( interactive ) {
            add_msg( m_info, _( "The %s doesn't have any faults to mend." ), obj->tname() );
        }
        return;
    }

    auto inv = crafting_inventory();

    for( auto &f : faults ) {
        f.second = f.first->requirements().can_make_with_inventory( inv, is_crafting_component );
    }

    int sel = 0;
    if( interactive ) {
        uilist menu;
        menu.text = _( "Mend which fault?" );
        menu.desc_enabled = true;
        menu.desc_lines = 0; // Let uilist handle description height

        int w = 80;

        for( auto &f : faults ) {
            auto reqs = f.first->requirements();
            auto tools = reqs.get_folded_tools_list( w, c_white, inv );
            auto comps = reqs.get_folded_components_list( w, c_white, inv, is_crafting_component );

            std::ostringstream descr;
            descr << _( "<color_white>Time required:</color>\n" );
            // TODO: better have a from_moves function
            descr << "> " << to_string_approx( time_duration::from_turns( f.first->time() / 100 ) ) << "\n";
            descr << _( "<color_white>Skills:</color>\n" );
            for( const auto &e : f.first->skills() ) {
                bool hasSkill = get_skill_level( e.first ) >= e.second;
                if( !hasSkill && f.second ) {
                    f.second = false;
                }
                //~ %1$s represents the internal color name which shouldn't be translated, %2$s is skill name, and %3$i is skill level
                descr << string_format( _( "> <color_%1$s>%2$s %3$i</color>\n" ), hasSkill ? "c_green" : "c_red",
                                        e.first.obj().name(), e.second );
            }

            std::copy( tools.begin(), tools.end(), std::ostream_iterator<std::string>( descr, "\n" ) );
            std::copy( comps.begin(), comps.end(), std::ostream_iterator<std::string>( descr, "\n" ) );

            menu.addentry_desc( -1, true, -1, f.first->name(), descr.str() );
        }
        menu.query();
        if( menu.ret < 0 ) {
            add_msg( _( "Never mind." ) );
            return;
        }
        sel = menu.ret;
    }

    if( sel >= 0 ) {
        if( !faults[ sel ].second ) {
            if( interactive ) {
                add_msg( m_info, _( "You are currently unable to mend the %s." ), obj->tname() );
            }
            return;
        }

        assign_activity( activity_id( "ACT_MEND_ITEM" ), faults[ sel ].first->time() );
        activity.name = faults[ sel ].first->id().str();
        activity.targets.push_back( std::move( obj ) );
    }
}

int player::item_reload_cost( const item &it, const item &ammo, int qty ) const
{
    if( ammo.is_ammo() || ammo.is_frozen_liquid() ) {
        qty = std::max( std::min( ammo.charges, qty ), 1 );
    } else if( ammo.is_ammo_container() || ammo.is_container() ) {
        qty = std::max( std::min( ammo.contents.front().charges, qty ), 1 );
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

    if( ammo.has_flag( "MAG_BULKY" ) ) {
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

    int cost = ( it.is_gun() ? it.get_reload_time() : it.type->magazine->reload_time ) * qty;

    skill_id sk = it.is_gun() ? it.type->gun->skill_used : skill_gun;
    mv += cost / ( 1.0f + std::min( get_skill_level( sk ) * 0.1f, 1.0f ) );

    if( it.has_flag( "STR_RELOAD" ) ) {
        /** @EFFECT_STR reduces reload time of some weapons */
        mv -= get_str() * 20;
    }

    return std::max( mv, 25 );
}

int player::item_wear_cost( const item &it ) const
{
    double mv = item_handling_cost( it );

    switch( it.get_layer() ) {
        case PERSONAL_LAYER:
            break;

        case UNDERWEAR_LAYER:
            mv *= 1.5;
            break;

        case REGULAR_LAYER:
            break;

        case WAIST_LAYER:
        case OUTER_LAYER:
            mv /= 1.5;
            break;

        case BELTED_LAYER:
            mv /= 2.0;
            break;

        case AURA_LAYER:
            break;

        default:
            break;
    }

    mv *= std::max( it.get_encumber( *this ) / 10.0, 1.0 );

    return mv;
}

cata::optional<std::list<item>::iterator>
player::wear( int pos, bool interactive )
{
    return wear( i_at( pos ), interactive );
}

cata::optional<std::list<item>::iterator>
player::wear( item &to_wear, bool interactive )
{
    if( is_worn( to_wear ) ) {
        if( interactive ) {
            add_msg_player_or_npc( m_info,
                                   _( "You are already wearing that." ),
                                   _( "<npcname> is already wearing that." )
                                 );
        }
        return cata::nullopt;
    }
    if( to_wear.is_null() ) {
        if( interactive ) {
            add_msg_player_or_npc( m_info,
                                   _( "You don't have that item." ),
                                   _( "<npcname> doesn't have that item." ) );
        }
        return cata::nullopt;
    }

    bool was_weapon;
    item to_wear_copy( to_wear );
    if( &to_wear == &weapon ) {
        weapon = item();
        was_weapon = true;
    } else {
        inv.remove_item( &to_wear );
        inv.restack( *this );
        was_weapon = false;
    }

    auto result = wear_item( to_wear_copy, interactive );
    if( !result ) {
        if( was_weapon ) {
            weapon = to_wear_copy;
        } else {
            inv.add_item( to_wear_copy, true );
        }
        return cata::nullopt;
    }

    return result;
}

cata::optional<std::list<item>::iterator>
player::wear_item( const item &to_wear, bool interactive )
{
    const auto ret = can_wear( to_wear );
    if( !ret.success() ) {
        if( interactive ) {
            add_msg_if_player( m_info, "%s", ret.c_str() );
        }
        return cata::nullopt;
    }

    const bool was_deaf = is_deaf();
    const bool supertinymouse = g->u.has_trait( trait_id( "SMALL2" ) ) ||
                                g->u.has_trait( trait_id( "SMALL_OK" ) );
    last_item = to_wear.typeId();

    std::list<item>::iterator position = position_to_wear_new_item( to_wear );
    std::list<item>::iterator new_item_it = worn.insert( position, to_wear );

    if( interactive ) {
        add_msg_player_or_npc(
            _( "You put on your %s." ),
            _( "<npcname> puts on their %s." ),
            to_wear.tname() );
        moves -= item_wear_cost( to_wear );

        for( const body_part bp : all_body_parts ) {
            if( to_wear.covers( bp ) && encumb( bp ) >= 40 ) {
                add_msg_if_player( m_warning,
                                   bp == bp_eyes ?
                                   _( "Your %s are very encumbered!  %s" ) : _( "Your %s is very encumbered!  %s" ),
                                   body_part_name( bp ), encumb_text( bp ) );
            }
        }
        if( !was_deaf && is_deaf() ) {
            add_msg_if_player( m_info, _( "You're deafened!" ) );
        }
        if( supertinymouse && !to_wear.has_flag( "UNDERSIZE" ) ) {
            add_msg_if_player( m_warning,
                               _( "This %s is too big to wear comfortably!  Maybe it could be refitted..." ),
                               to_wear.tname() );
        } else if( to_wear.has_flag( "UNDERSIZE" ) ) {
            add_msg_if_player( m_warning,
                               _( "This %s is too small to wear comfortably!  Maybe it could be refitted..." ),
                               to_wear.tname() );
        }
    } else {
        add_msg_if_npc( _( "<npcname> puts on their %s." ), to_wear.tname() );
    }

    new_item_it->on_wear( *this );

    inv.update_invlet( *new_item_it );
    inv.update_cache_with_item( *new_item_it );

    recalc_sight_limits();
    reset_encumbrance();

    return new_item_it;
}

bool player::change_side( item &it, bool interactive )
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
    reset_encumbrance();

    return true;
}

bool player::change_side( int pos, bool interactive )
{
    item &it( i_at( pos ) );

    if( !is_worn( it ) ) {
        if( interactive ) {
            add_msg_player_or_npc( m_info,
                                   _( "You are not wearing that item." ),
                                   _( "<npcname> isn't wearing that item." ) );
        }
        return false;
    }

    return change_side( it, interactive );
}

hint_rating player::rate_action_takeoff( const item &it ) const
{
    if( !it.is_armor() ) {
        return HINT_CANT;
    }

    if( is_worn( it ) ) {
        return HINT_GOOD;
    }

    return HINT_IFFY;
}

std::list<const item *> player::get_dependent_worn_items( const item &it ) const
{
    std::list<const item *> dependent;
    // Adds dependent worn items recursively
    const std::function<void( const item &it )> add_dependent = [ & ]( const item & it ) {
        for( const auto &wit : worn ) {
            if( &wit == &it || !wit.is_worn_only_with( it ) ) {
                continue;
            }
            const auto iter = std::find_if( dependent.begin(), dependent.end(),
            [ &wit ]( const item * dit ) {
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

ret_val<bool> player::can_takeoff( const item &it, const std::list<item> *res ) const
{
    auto iter = std::find_if( worn.begin(), worn.end(), [ &it ]( const item & wit ) {
        return &it == &wit;
    } );

    if( iter == worn.end() ) {
        return ret_val<bool>::make_failure( !is_npc() ? _( "You are not wearing that item." ) :
                                            _( "<npcname> is not wearing that item." ) );
    }

    if( res == nullptr && !get_dependent_worn_items( it ).empty() ) {
        return ret_val<bool>::make_failure( !is_npc() ?
                                            _( "You can't take off power armor while wearing other power armor components." ) :
                                            _( "<npcname> can't take off power armor while wearing other power armor components." ) );
    }
    if( it.has_flag( "NO_TAKEOFF" ) ) {
        return ret_val<bool>::make_failure( !is_npc() ?
                                            _( "You can't take that item off." ) :
                                            _( "<npcname> can't take that item off." ) );
    }
    return ret_val<bool>::make_success();
}

bool player::takeoff( const item &it, std::list<item> *res )
{
    const auto ret = can_takeoff( it, res );
    if( !ret.success() ) {
        add_msg( m_info, "%s", ret.c_str() );
        return false;
    }

    auto iter = std::find_if( worn.begin(), worn.end(), [ &it ]( const item & wit ) {
        return &it == &wit;
    } );

    if( res == nullptr ) {
        if( volume_carried() + it.volume() > volume_capacity_reduced_by( it.get_storage() ) ) {
            if( is_npc() || query_yn( _( "No room in inventory for your %s.  Drop it?" ),
                                      colorize( it.tname(), it.color_in_inventory() ) ) ) {
                drop( get_item_position( &it ), pos() );
                return true; // the drop activity ends up taking off the item anyway so shouldn't try to do it again here
            } else {
                return false;
            }
        }
        iter->on_takeoff( *this );
        inv.add_item_keep_invlet( it );
    } else {
        iter->on_takeoff( *this );
        res->push_back( it );
    }

    add_msg_player_or_npc( _( "You take off your %s." ),
                           _( "<npcname> takes off their %s." ),
                           it.tname() );

    mod_moves( -250 );    // TODO: Make this variable
    worn.erase( iter );

    recalc_sight_limits();
    reset_encumbrance();

    return true;
}

bool player::takeoff( int pos )
{
    return takeoff( i_at( pos ) );
}

void player::drop( int pos, const tripoint &where )
{
    const item &it = i_at( pos );
    const int count = it.count();

    drop( { std::make_pair( pos, count ) }, where );
}

void player::drop( const std::list<std::pair<int, int>> &what, const tripoint &target, bool stash )
{
    const activity_id type( stash ? "ACT_STASH" : "ACT_DROP" );

    if( what.empty() ) {
        return;
    }

    if( rl_dist( pos(), target ) > 1 || !( stash || g->m.can_put_items( target ) ) ) {
        add_msg_player_or_npc( m_info, _( "You can't place items here!" ),
                               _( "<npcname> can't place items here!" ) );
        return;
    }

    assign_activity( type );
    activity.placement = target - pos();

    for( auto item_pair : what ) {
        if( can_unwield( i_at( item_pair.first ) ).success() ) {
            activity.values.push_back( item_pair.first );
            activity.values.push_back( item_pair.second );
        }
    }
    // TODO: Remove the hack. Its here because npcs don't process activities
    if( is_npc() ) {
        activity.do_turn( *this );
    }
}

bool player::add_or_drop_with_msg( item &it, const bool unloading )
{
    if( it.made_of( LIQUID ) ) {
        liquid_handler::consume_liquid( it, 1 );
        return it.charges <= 0;
    }
    it.charges = this->i_add_to_container( it, unloading );
    if( it.is_ammo() && it.charges == 0 ) {
        return true;
    } else if( !this->can_pickVolume( it ) ) {
        put_into_vehicle_or_drop( *this, item_drop_reason::too_large, { it } );
    } else if( !this->can_pickWeight( it, !get_option<bool>( "DANGEROUS_PICKUPS" ) ) ) {
        put_into_vehicle_or_drop( *this, item_drop_reason::too_heavy, { it } );
    } else {
        auto &ni = this->i_add( it );
        add_msg( _( "You put the %s in your inventory." ), ni.tname() );
        add_msg( m_info, "%c - %s", ni.invlet == 0 ? ' ' : ni.invlet, ni.tname() );
    }
    return true;
}

bool player::unload( item &it )
{
    // Unload a container consuming moves per item successfully removed
    if( it.is_container() || it.is_bandolier() ) {
        if( it.contents.empty() ) {
            add_msg( m_info, _( "The %s is already empty!" ), it.tname() );
            return false;
        }
        if( !it.can_unload_liquid() ) {
            add_msg( m_info, _( "The liquid can't be unloaded in its current state!" ) );
            return false;
        }

        bool changed = false;
        it.contents.erase( std::remove_if( it.contents.begin(), it.contents.end(), [this,
        &changed]( item & e ) {
            int old_charges = e.charges;
            const bool consumed = this->add_or_drop_with_msg( e, true );
            changed = changed || consumed || e.charges != old_charges;
            if( consumed ) {
                this->mod_moves( -this->item_handling_cost( e ) );
            }
            return consumed;
        } ), it.contents.end() );
        if( changed ) {
            it.on_contents_changed();
        }
        return true;
    }

    // If item can be unloaded more than once (currently only guns) prompt user to choose
    std::vector<std::string> msgs( 1, it.tname() );
    std::vector<item *> opts( 1, &it );

    for( auto e : it.gunmods() ) {
        if( e->is_gun() && !e->has_flag( "NO_UNLOAD" ) &&
            ( e->magazine_current() || e->ammo_remaining() > 0 || e->casings_count() > 0 ) ) {
            msgs.emplace_back( e->tname() );
            opts.emplace_back( e );
        }
    }

    item *target = nullptr;
    if( opts.size() > 1 ) {
        const int ret = uilist( _( "Unload what?" ), msgs );
        if( ret >= 0 ) {
            target = opts[ret];
        }
    } else {
        target = &it;
    }

    if( target == nullptr ) {
        return false;
    }

    // Next check for any reasons why the item cannot be unloaded
    if( target->ammo_types().empty() || target->ammo_capacity() <= 0 ) {
        add_msg( m_info, _( "You can't unload a %s!" ), target->tname() );
        return false;
    }

    if( target->has_flag( "NO_UNLOAD" ) ) {
        if( target->has_flag( "RECHARGE" ) || target->has_flag( "USE_UPS" ) ) {
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

    target->casings_handle( [&]( item & e ) {
        return this->i_add_or_drop( e );
    } );

    if( target->is_magazine() ) {
        player_activity unload_mag_act( activity_id( "ACT_UNLOAD_MAG" ) );
        g->u.assign_activity( unload_mag_act );
        g->u.activity.targets.emplace_back( item_location( *this, target ) );

        // Calculate the time to remove the contained ammo (consuming half as much time as required to load the magazine)
        int mv = 0;
        for( auto &content : target->contents ) {
            mv += this->item_reload_cost( it, content, content.charges ) / 2;
        }
        g->u.activity.moves_left += mv;

        // I think this means if unload is not done on ammo-belt, it takes as long as it takes to reload a mag.
        if( !it.is_ammo_belt() ) {
            g->u.activity.moves_left += mv;
        }
        g->u.activity.auto_resume = true;

        return true;

    } else if( target->magazine_current() ) {
        if( !this->add_or_drop_with_msg( *target->magazine_current(), true ) ) {
            return false;
        }
        // Eject magazine consuming half as much time as required to insert it
        this->moves -= this->item_reload_cost( *target, *target->magazine_current(), -1 ) / 2;

        target->contents.remove_if( [&target]( const item & e ) {
            return target->magazine_current() == &e;
        } );

    } else if( target->ammo_remaining() ) {
        int qty = target->ammo_remaining();

        if( target->ammo_current() == "plut_cell" ) {
            qty = target->ammo_remaining() / PLUTONIUM_CHARGES;
            if( qty > 0 ) {
                add_msg( _( "You recover %i unused plutonium." ), qty );
            } else {
                add_msg( m_info, _( "You can't remove partially depleted plutonium!" ) );
                return false;
            }
        }

        // Construct a new ammo item and try to drop it
        item ammo( target->ammo_current(), calendar::turn, qty );

        if( ammo.made_of_from_type( LIQUID ) ) {
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

        if( target->ammo_current() == "plut_cell" ) {
            qty *= PLUTONIUM_CHARGES;
        }

        target->ammo_set( target->ammo_current(), target->ammo_remaining() - qty );
    }

    // Turn off any active tools
    if( target->is_tool() && target->active && target->ammo_remaining() == 0 ) {
        target->type->invoke( *this, *target, this->pos() );
    }

    add_msg( _( "You unload your %s." ), target->tname() );
    return true;
}

void player::use_wielded()
{
    use( -1 );
}

hint_rating player::rate_action_reload( const item &it ) const
{
    hint_rating res = HINT_CANT;

    // Guns may contain additional reloadable mods so check these first
    for( const auto mod : it.gunmods() ) {
        switch( rate_action_reload( *mod ) ) {
            case HINT_GOOD:
                return HINT_GOOD;

            case HINT_CANT:
                continue;

            case HINT_IFFY:
                res = HINT_IFFY;
        }
    }

    if( !it.is_reloadable() ) {
        return res;
    }

    return can_reload( it ) ? HINT_GOOD : HINT_IFFY;
}

hint_rating player::rate_action_unload( const item &it ) const
{
    if( ( it.is_container() || it.is_bandolier() ) && !it.contents.empty() &&
        it.can_unload_liquid() ) {
        return HINT_GOOD;
    }

    if( it.has_flag( "NO_UNLOAD" ) ) {
        return HINT_CANT;
    }

    if( it.magazine_current() ) {
        return HINT_GOOD;
    }

    for( auto e : it.gunmods() ) {
        if( e->is_gun() && !e->has_flag( "NO_UNLOAD" ) &&
            ( e->magazine_current() || e->ammo_remaining() > 0 || e->casings_count() > 0 ) ) {
            return HINT_GOOD;
        }
    }

    if( it.ammo_types().empty() ) {
        return HINT_CANT;
    }

    if( it.ammo_remaining() > 0 || it.casings_count() > 0 ) {
        return HINT_GOOD;
    }

    if( it.ammo_capacity() > 0 ) {
        return HINT_IFFY;
    }

    return HINT_CANT;
}

hint_rating player::rate_action_mend( const item &it ) const
{
    // TODO: check also if item damage could be repaired via a tool
    if( !it.faults.empty() ) {
        return HINT_GOOD;
    }
    return it.faults_potential().empty() ? HINT_CANT : HINT_IFFY;
}

hint_rating player::rate_action_disassemble( const item &it )
{
    if( can_disassemble( it, crafting_inventory() ).success() ) {
        return HINT_GOOD; // possible

    } else if( recipe_dictionary::get_uncraft( it.typeId() ) ) {
        return HINT_IFFY; // potentially possible but we currently lack requirements

    } else {
        return HINT_CANT; // never possible
    }
}

hint_rating player::rate_action_use( const item &it ) const
{
    if( it.is_tool() ) {
        return it.ammo_sufficient() ? HINT_GOOD : HINT_IFFY;

    } else if( it.is_gunmod() ) {
        /** @EFFECT_GUN >0 allows rating estimates for gun modifications */
        if( get_skill_level( skill_gun ) == 0 ) {
            return HINT_IFFY;
        } else {
            return HINT_GOOD;
        }
    } else if( it.is_food() || it.is_medication() || it.is_book() || it.is_armor() ) {
        return HINT_IFFY; //the rating is subjective, could be argued as HINT_CANT or HINT_GOOD as well
    } else if( it.type->has_use() ) {
        return HINT_GOOD;
    } else if( !it.is_container_empty() ) {
        return rate_action_use( it.get_contained() );
    }

    return HINT_CANT;
}

bool player::has_enough_charges( const item &it, bool show_msg ) const
{
    if( !it.is_tool() || !it.ammo_required() ) {
        return true;
    }
    if( it.has_flag( "USE_UPS" ) ) {
        if( has_charges( "UPS", it.ammo_required() ) || it.ammo_sufficient() ) {
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
                               ngettext( "Your %s has %d charge but needs %d.",
                                         "Your %s has %d charges but needs %d.",
                                         it.ammo_remaining() ),
                               it.tname(), it.ammo_remaining(), it.ammo_required() );
        }
        return false;
    }
    return true;
}

bool player::consume_charges( item &used, int qty )
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
        i_rem( &used );
        return true;
    }

    // USE_UPS never occurs on base items but is instead added by the UPS tool mod
    if( used.has_flag( "USE_UPS" ) ) {
        // With the new UPS system, we'll want to use any charges built up in the tool before pulling from the UPS
        // The usage of the item was already approved, so drain item if possible, otherwise use UPS
        if( used.charges >= qty ) {
            used.ammo_consume( qty, pos() );
        } else {
            use_charges( "UPS", qty );
        }
    } else {
        used.ammo_consume( std::min( qty, used.ammo_remaining() ), pos() );
    }
    return false;
}

void player::use( int inventory_position )
{
    item &used = i_at( inventory_position );
    auto loc = item_location( *this, &used );

    use( loc );
}

void player::use( item_location loc )
{
    item &used = *loc.get_item();
    int inventory_position = loc.where() == item_location::type::character ?
                             this->get_item_position( &used ) : INT_MIN;

    if( used.is_null() ) {
        add_msg( m_info, _( "You do not have that item." ) );
        return;
    }

    last_item = used.typeId();

    if( used.is_tool() ) {
        if( !used.type->has_use() ) {
            add_msg_if_player( _( "You can't do anything interesting with your %s." ), used.tname() );
            return;
        }
        invoke_item( &used, loc.position() );

    } else if( used.type->can_use( "DOGFOOD" ) ||
               used.type->can_use( "CATFOOD" ) ||
               used.type->can_use( "BIRDFOOD" ) ||
               used.type->can_use( "CATTLEFODDER" ) ) {
        invoke_item( &used, loc.position() );

    } else if( !used.is_craft() && ( used.is_food() ||
                                     used.is_medication() ||
                                     used.get_contained().is_food() ||
                                     used.get_contained().is_medication() ) ) {
        consume( inventory_position );

    } else if( used.is_book() ) {
        // TODO: Remove this nasty cast once this and related functions are migrated to avatar
        if( avatar *u = dynamic_cast<avatar *>( this ) ) {
            u->read( inventory_position );
        }
    } else if( used.type->has_use() ) {
        invoke_item( &used, loc.position() );

    } else {
        add_msg( m_info, _( "You can't do anything interesting with your %s." ),
                 used.tname() );
    }
}

bool player::invoke_item( item *used )
{
    return invoke_item( used, pos() );
}

bool player::invoke_item( item *used, const tripoint &pt )
{
    const auto &use_methods = used->type->use_methods;

    if( use_methods.empty() ) {
        return false;
    } else if( use_methods.size() == 1 ) {
        return invoke_item( used, use_methods.begin()->first, pt );
    }

    uilist umenu;

    umenu.text = string_format( _( "What to do with your %s?" ), used->tname() );
    umenu.hilight_disabled = true;

    for( const auto &e : use_methods ) {
        const auto res = e.second.can_call( *this, *used, false, pt );
        umenu.addentry_desc( MENU_AUTOASSIGN, res.success(), MENU_AUTOASSIGN, e.second.get_name(),
                             res.str() );
    }

    umenu.desc_enabled = std::any_of( umenu.entries.begin(),
    umenu.entries.end(), []( const uilist_entry & elem ) {
        return !elem.desc.empty();
    } );

    umenu.query();

    int choice = umenu.ret;
    if( choice < 0 || choice >= static_cast<int>( use_methods.size() ) ) {
        return false;
    }

    const std::string &method = std::next( use_methods.begin(), choice )->first;

    return invoke_item( used, method, pt );
}

bool player::invoke_item( item *used, const std::string &method )
{
    return invoke_item( used, method, pos() );
}

bool player::invoke_item( item *used, const std::string &method, const tripoint &pt )
{
    if( !has_enough_charges( *used, true ) ) {
        return false;
    }

    item *actually_used = used->get_usable_item( method );
    if( actually_used == nullptr ) {
        debugmsg( "Tried to invoke a method %s on item %s, which doesn't have this method",
                  method.c_str(), used->tname() );
        return false;
    }

    int charges_used = actually_used->type->invoke( *this, *actually_used, pt, method );
    if( charges_used == 0 ) {
        return false;
    }
    // Prevent accessing the item as it may have been deleted by the invoked iuse function.

    if( used->is_tool() || used->is_medication() || used->get_contained().is_medication() ) {
        return consume_charges( *actually_used, charges_used );
    } else if( used->is_bionic() || used->is_deployable() || method == "place_trap" ) {
        i_rem( used );
        return true;
    }

    return false;
}

void player::reassign_item( item &it, int invlet )
{
    bool remove_old = true;
    if( invlet ) {
        item &prev = i_at( invlet_to_position( invlet ) );
        if( !prev.is_null() ) {
            remove_old = it.typeId() != prev.typeId();
            inv.reassign_item( prev, it.invlet, remove_old );
        }
    }

    if( !invlet || inv_chars.valid( invlet ) ) {
        const auto iter = inv.assigned_invlet.find( it.invlet );
        bool found = iter != inv.assigned_invlet.end();
        if( found ) {
            inv.assigned_invlet.erase( iter );
        }
        if( invlet && ( !found || it.invlet != invlet ) ) {
            inv.assigned_invlet[invlet] = it.typeId();
        }
        inv.reassign_item( it, invlet, remove_old );
    }
}

bool player::gunmod_remove( item &gun, item &mod )
{
    auto iter = std::find_if( gun.contents.begin(), gun.contents.end(), [&mod]( const item & e ) {
        return &mod == &e;
    } );
    if( iter == gun.contents.end() ) {
        debugmsg( "Cannot remove non-existent gunmod" );
        return false;
    }
    if( mod.ammo_remaining() && !g->unload( mod ) ) {
        return false;
    }

    gun.gun_set_mode( gun_mode_id( "DEFAULT" ) );
    moves -= mod.type->gunmod->install_time / 2;

    if( mod.typeId() == "brass_catcher" ) {
        gun.casings_handle( [&]( item & e ) {
            return i_add_or_drop( e );
        } );
    }

    const itype *modtype = mod.type;

    i_add_or_drop( mod );
    gun.contents.erase( iter );

    //If the removed gunmod added mod locations, check to see if any mods are in invalid locations
    if( !modtype->gunmod->add_mod.empty() ) {
        std::map<gunmod_location, int> mod_locations = gun.get_mod_locations();
        for( const auto &slot : mod_locations ) {
            int free_slots = gun.get_free_mod_locations( slot.first );

            for( auto the_mod : gun.gunmods() ) {
                if( the_mod->type->gunmod->location == slot.first && free_slots < 0 ) {
                    gunmod_remove( gun, *the_mod );
                    free_slots++;
                } else if( mod_locations.find( the_mod->type->gunmod->location ) ==
                           mod_locations.end() ) {
                    gunmod_remove( gun, *the_mod );
                }
            }
        }
    }

    //~ %1$s - gunmod, %2$s - gun.
    add_msg_if_player( _( "You remove your %1$s from your %2$s." ), modtype->nname( 1 ),
                       gun.tname() );

    return true;
}

std::pair<int, int> player::gunmod_installation_odds( const item &gun, const item &mod ) const
{
    // Mods with INSTALL_DIFFICULT have a chance to fail, potentially damaging the gun
    if( !mod.has_flag( "INSTALL_DIFFICULT" ) || has_trait( trait_DEBUG_HS ) ) {
        return std::make_pair( 100, 0 );
    }

    int roll = 100; // chance of success (%)
    int risk = 0;   // chance of failure (%)
    int chances = 1; // start with 1 in 6 (~17% chance)

    for( const auto &e : mod.type->min_skills ) {
        // gain an additional chance for every level above the minimum requirement
        skill_id sk = e.first == "weapon" ? gun.gun_skill() : skill_id( e.first );
        chances += std::max( get_skill_level( sk ) - e.second, 0 );
    }
    // cap success from skill alone to 1 in 5 (~83% chance)
    roll = std::min( static_cast<double>( chances ), 5.0 ) / 6.0 * 100;
    // focus is either a penalty or bonus of at most +/-10%
    roll += ( std::min( std::max( focus_pool, 140 ), 60 ) - 100 ) / 4;
    // dexterity and intelligence give +/-2% for each point above or below 12
    roll += ( get_dex() - 12 ) * 2;
    roll += ( get_int() - 12 ) * 2;
    // each level of damage to the base gun reduces success by 10%
    roll -= std::max( gun.damage_level( 4 ), 0 ) * 10;
    roll = std::min( std::max( roll, 0 ), 100 );

    // risk of causing damage on failure increases with less durable guns
    risk = ( 100 - roll ) * ( ( 10.0 - std::min( gun.type->gun->durability, 9 ) ) / 10.0 );

    return std::make_pair( roll, risk );
}

void player::gunmod_add( item &gun, item &mod )
{
    if( !gun.is_gunmod_compatible( mod ).success() ) {
        debugmsg( "Tried to add incompatible gunmod" );
        return;
    }

    if( !has_item( gun ) && !has_item( mod ) ) {
        debugmsg( "Tried gunmod installation but mod/gun not in player possession" );
        return;
    }

    // first check at least the minimum requirements are met
    if( !has_trait( trait_DEBUG_HS ) && !can_use( mod, gun ) ) {
        return;
    }

    // any (optional) tool charges that are used during installation
    auto odds = gunmod_installation_odds( gun, mod );
    int roll = odds.first;
    int risk = odds.second;

    std::string tool;
    int qty = 0;

    if( mod.is_irremovable() ) {
        if( !query_yn( _( "Permanently install your %1$s in your %2$s?" ),
                       colorize( mod.tname(), mod.color_in_inventory() ),
                       colorize( gun.tname(), gun.color_in_inventory() ) ) ) {
            add_msg_if_player( _( "Never mind." ) );
            return; // player canceled installation
        }
    }

    // if chance of success <100% prompt user to continue
    if( roll < 100 ) {
        uilist prompt;
        prompt.text = string_format( _( "Attach your %1$s to your %2$s?" ), mod.tname(),
                                     gun.tname() );

        std::vector<std::function<void()>> actions;

        prompt.addentry( -1, true, 'w',
                         string_format( _( "Try without tools (%i%%) risking damage (%i%%)" ), roll, risk ) );
        actions.emplace_back( [&] {} );

        prompt.addentry( -1, has_charges( "small_repairkit", 100 ), 'f',
                         string_format( _( "Use 100 charges of firearm repair kit (%i%%)" ), std::min( roll * 2, 100 ) ) );

        actions.emplace_back( [&] {
            tool = "small_repairkit";
            qty = 100;
            roll *= 2; // firearm repair kit improves success...
            risk /= 2; // ...and reduces the risk of damage upon failure
        } );

        prompt.addentry( -1, has_charges( "large_repairkit", 25 ), 'g',
                         string_format( _( "Use 25 charges of gunsmith repair kit (%i%%)" ), std::min( roll * 3, 100 ) ) );

        actions.emplace_back( [&] {
            tool = "large_repairkit";
            qty = 25;
            roll *= 3; // gunsmith repair kit improves success markedly...
            risk = 0;  // ...and entirely prevents damage upon failure
        } );

        prompt.query();
        if( prompt.ret < 0 ) {
            add_msg_if_player( _( "Never mind." ) );
            return; // player canceled installation
        }
        actions[ prompt.ret ]();
    }

    const int turns = !has_trait( trait_DEBUG_HS ) ? mod.type->gunmod->install_time : 0;
    const int moves = to_moves<int>( time_duration::from_turns( turns ) );

    assign_activity( activity_id( "ACT_GUNMOD_ADD" ), moves, -1, get_item_position( &gun ), tool );
    activity.values.push_back( get_item_position( &mod ) );
    activity.values.push_back( roll ); // chance of success (%)
    activity.values.push_back( risk ); // chance of damage (%)
    activity.values.push_back( qty ); // tool charges
}

void player::toolmod_add( item_location tool, item_location mod )
{
    if( !tool && !mod ) {
        debugmsg( "Tried toolmod installation but mod/tool not available" );
        return;
    }
    // first check at least the minimum requirements are met
    if( !has_trait( trait_DEBUG_HS ) && !can_use( *mod, *tool ) ) {
        return;
    }

    if( !query_yn( _( "Permanently install your %1$s in your %2$s?" ),
                   colorize( mod->tname(), mod->color_in_inventory() ),
                   colorize( tool->tname(), tool->color_in_inventory() ) ) ) {
        add_msg_if_player( _( "Never mind." ) );
        return; // player canceled installation
    }

    assign_activity( activity_id( "ACT_TOOLMOD_ADD" ), 1, -1 );
    activity.targets.emplace_back( std::move( tool ) );
    activity.targets.emplace_back( std::move( mod ) );
}

bool player::fun_to_read( const item &book ) const
{
    // If you don't have a problem with eating humans, To Serve Man becomes rewarding
    if( ( has_trait( trait_CANNIBAL ) || has_trait( trait_PSYCHOPATH ) ||
          has_trait( trait_SAPIOVORE ) ) &&
        book.typeId() == "cookbook_human" ) {
        return true;
    } else if( has_trait( trait_SPIRITUAL ) && book.has_flag( "INSPIRATIONAL" ) ) {
        return true;
    } else {
        return book_fun_for( book, *this ) > 0;
    }
}

int player::book_fun_for( const item &book, const player &p ) const
{
    int fun_bonus = book.type->book->fun;
    if( !book.is_book() ) {
        debugmsg( "called avatar::book_fun_for with non-book" );
        return 0;
    }

    // If you don't have a problem with eating humans, To Serve Man becomes rewarding
    if( ( p.has_trait( trait_CANNIBAL ) || p.has_trait( trait_PSYCHOPATH ) ||
          p.has_trait( trait_SAPIOVORE ) ) &&
        book.typeId() == "cookbook_human" ) {
        fun_bonus = abs( fun_bonus );
    } else if( p.has_trait( trait_SPIRITUAL ) && book.has_flag( "INSPIRATIONAL" ) ) {
        fun_bonus = abs( fun_bonus * 3 );
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

    return fun_bonus;
}

bool player::studied_all_recipes( const itype &book ) const
{
    if( !book.book ) {
        return true;
    }
    for( auto &elem : book.book->recipes ) {
        if( !knows_recipe( elem.recipe ) ) {
            return false;
        }
    }
    return true;
}

const recipe_subset &player::get_learned_recipes() const
{
    // Cache validity check
    if( *_skills != *valid_autolearn_skills ) {
        for( const auto &r : recipe_dict.all_autolearn() ) {
            if( meets_skill_requirements( r->autolearn_requirements ) ) {
                learned_recipes->include( r );
            }
        }
        *valid_autolearn_skills = *_skills; // Reassign the validity stamp
    }

    return *learned_recipes;
}

recipe_subset player::get_recipes_from_books( const inventory &crafting_inv ) const
{
    recipe_subset res;

    for( const auto &stack : crafting_inv.const_slice() ) {
        const item &candidate = stack->front();

        for( std::pair<const recipe *, int> recipe_entry :
             candidate.get_available_recipes( *this ) ) {
            res.include( recipe_entry.first, recipe_entry.second );
        }
    }

    return res;
}

std::set<itype_id> player::get_books_for_recipe( const inventory &crafting_inv,
        const recipe *r ) const
{
    std::set<itype_id> book_ids;
    const int skill_level = get_skill_level( r->skill_used );
    for( auto &book_lvl : r->booksets ) {
        itype_id book_id = book_lvl.first;
        int required_skill_level = book_lvl.second;
        // NPCs don't need to identify books
        if( !has_identified( book_id ) ) {
            continue;
        }

        if( skill_level >= required_skill_level && crafting_inv.amount_of( book_id ) > 0 ) {
            book_ids.insert( book_id );
        }
    }
    return book_ids;
}

recipe_subset player::get_available_recipes( const inventory &crafting_inv,
        const std::vector<npc *> *helpers ) const
{
    recipe_subset res( get_learned_recipes() );

    res.include( get_recipes_from_books( crafting_inv ) );

    if( helpers != nullptr ) {
        for( npc *np : *helpers ) {
            // Directly form the helper's inventory
            res.include( get_recipes_from_books( np->inv ) );
            // Being told what to do
            res.include_if( np->get_learned_recipes(), [ this ]( const recipe & r ) {
                return get_skill_level( r.skill_used ) >= static_cast<int>( r.difficulty *
                        0.8f ); // Skilled enough to understand
            } );
        }
    }

    return res;
}

void player::try_to_sleep( const time_duration &dur )
{
    const optional_vpart_position vp = g->m.veh_at( pos() );
    const trap &trap_at_pos = g->m.tr_at( pos() );
    const ter_id ter_at_pos = g->m.ter( pos() );
    const furn_id furn_at_pos = g->m.furn( pos() );
    bool plantsleep = false;
    bool fungaloid_cosplay = false;
    bool websleep = false;
    bool webforce = false;
    bool websleeping = false;
    bool in_shell = false;
    bool watersleep = false;
    if( has_trait( trait_CHLOROMORPH ) ) {
        plantsleep = true;
        if( ( ter_at_pos == t_dirt || ter_at_pos == t_pit ||
              ter_at_pos == t_dirtmound || ter_at_pos == t_pit_shallow ||
              ter_at_pos == t_grass ) && !vp &&
            furn_at_pos == f_null ) {
            add_msg_if_player( m_good, _( "You relax as your roots embrace the soil." ) );
        } else if( vp ) {
            add_msg_if_player( m_bad, _( "It's impossible to sleep in this wheeled pot!" ) );
        } else if( furn_at_pos != f_null ) {
            add_msg_if_player( m_bad,
                               _( "The humans' furniture blocks your roots.  You can't get comfortable." ) );
        } else { // Floor problems
            add_msg_if_player( m_bad, _( "Your roots scrabble ineffectively at the unyielding surface." ) );
        }
    } else if( has_trait( trait_M_SKIN3 ) ) {
        fungaloid_cosplay = true;
        if( g->m.has_flag_ter_or_furn( "FUNGUS", pos() ) ) {
            add_msg_if_player( m_good,
                               _( "Our fibers meld with the ground beneath us.  The gills on our neck begin to seed the air with spores as our awareness fades." ) );
        }
    }
    if( has_trait( trait_WEB_WALKER ) ) {
        websleep = true;
    }
    // Not sure how one would get Arachnid w/o web-making, but Just In Case
    if( has_trait( trait_THRESH_SPIDER ) && ( has_trait( trait_WEB_SPINNER ) ||
            ( has_trait( trait_WEB_WEAVER ) ) ) ) {
        webforce = true;
    }
    if( websleep || webforce ) {
        int web = g->m.get_field_intensity( pos(), fd_web );
        if( !webforce ) {
            // At this point, it's kinda weird, but surprisingly comfy...
            if( web >= 3 ) {
                add_msg_if_player( m_good,
                                   _( "These thick webs support your weight, and are strangely comfortable..." ) );
                websleeping = true;
            } else if( web > 0 ) {
                add_msg_if_player( m_info,
                                   _( "You try to sleep, but the webs get in the way.  You brush them aside." ) );
                g->m.remove_field( pos(), fd_web );
            }
        } else {
            // Here, you're just not comfortable outside a nice thick web.
            if( web >= 3 ) {
                add_msg_if_player( m_good, _( "You relax into your web." ) );
                websleeping = true;
            } else {
                add_msg_if_player( m_bad,
                                   _( "You try to sleep, but you feel exposed and your spinnerets keep twitching." ) );
                add_msg_if_player( m_info, _( "Maybe a nice thick web would help you sleep." ) );
            }
        }
    }
    if( has_active_mutation( trait_SHELL2 ) ) {
        // Your shell's interior is a comfortable place to sleep.
        in_shell = true;
    }
    if( has_trait( trait_WATERSLEEP ) ) {
        if( underwater ) {
            add_msg_if_player( m_good,
                               _( "You lay beneath the waves' embrace, gazing up through the water's surface..." ) );
            watersleep = true;
        } else if( g->m.has_flag_ter( "SWIMMABLE", pos() ) ) {
            add_msg_if_player( m_good, _( "You settle into the water and begin to drowse..." ) );
            watersleep = true;
        }
    }
    if( !plantsleep && ( furn_at_pos.obj().comfort > static_cast<int>( comfort_level::neutral ) ||
                         ter_at_pos == t_improvised_shelter ||
                         trap_at_pos.comfort > static_cast<int>( comfort_level::neutral ) ||
                         in_shell || websleeping || watersleep ||
                         vp.part_with_feature( "SEAT", true ) ||
                         vp.part_with_feature( "BED", true ) ) ) {
        add_msg_if_player( m_good, _( "This is a comfortable place to sleep." ) );
    } else if( !plantsleep && !fungaloid_cosplay && !watersleep ) {
        if( !vp && ter_at_pos != t_floor ) {
            add_msg_if_player( ter_at_pos.obj().movecost <= 2 ?
                               _( "It's a little hard to get to sleep on this %s." ) :
                               _( "It's hard to get to sleep on this %s." ),
                               ter_at_pos.obj().name() );
        } else if( vp ) {
            if( vp->part_with_feature( VPFLAG_AISLE, true ) ) {
                add_msg_if_player(
                    //~ %1$s: vehicle name, %2$s: vehicle part name
                    _( "It's a little hard to get to sleep on this %2$s in %1$s." ),
                    vp->vehicle().disp_name(),
                    vp->part_with_feature( VPFLAG_AISLE, true )->part().name( false ) );
            } else {
                add_msg_if_player(
                    //~ %1$s: vehicle name
                    _( "It's hard to get to sleep in %1$s." ),
                    vp->vehicle().disp_name() );
            }
        }
    }
    add_msg_if_player( _( "You start trying to fall asleep." ) );
    if( has_active_bionic( bio_soporific ) ) {
        bio_soporific_powered_at_last_sleep_check = has_power();
        if( bio_soporific_powered_at_last_sleep_check ) {
            // The actual bonus is applied in sleep_spot( p ).
            add_msg_if_player( m_good, _( "Your soporific inducer starts working its magic." ) );
        } else {
            add_msg_if_player( m_bad, _( "Your soporific inducer doesn't have enough power to operate." ) );
        }
    }
    assign_activity( activity_id( "ACT_TRY_SLEEP" ), to_moves<int>( dur ) );
}

comfort_level player::base_comfort_value( const tripoint &p ) const
{
    // Comfort of sleeping spots is "objective", while sleep_spot( p ) is "subjective"
    // As in the latter also checks for fatigue and other variables while this function
    // only looks at the base comfyness of something. It's still subjective, in a sense,
    // as arachnids who sleep in webs will find most places comfortable for instance.
    int comfort = 0;

    bool plantsleep = has_trait( trait_CHLOROMORPH );
    bool fungaloid_cosplay = has_trait( trait_M_SKIN3 );
    bool websleep = has_trait( trait_WEB_WALKER );
    bool webforce = has_trait( trait_THRESH_SPIDER ) && ( has_trait( trait_WEB_SPINNER ) ||
                    ( has_trait( trait_WEB_WEAVER ) ) );
    bool in_shell = has_active_mutation( trait_SHELL2 );
    bool watersleep = has_trait( trait_WATERSLEEP );

    const optional_vpart_position vp = g->m.veh_at( p );
    const maptile tile = g->m.maptile_at( p );
    const trap &trap_at_pos = tile.get_trap_t();
    const ter_id ter_at_pos = tile.get_ter();
    const furn_id furn_at_pos = tile.get_furn();

    int web = g->m.get_field_intensity( p, fd_web );

    // Some mutants have different comfort needs
    if( !plantsleep && !webforce ) {
        if( in_shell ) {
            comfort += 1 + static_cast<int>( comfort_level::slightly_comfortable );
            // Note: shelled individuals can still use sleeping aids!
        } else if( vp ) {
            vehicle &veh = vp->vehicle();
            const cata::optional<vpart_reference> carg = vp.part_with_feature( "CARGO", false );
            const cata::optional<vpart_reference> board = vp.part_with_feature( "BOARDABLE", true );
            if( carg ) {
                vehicle_stack items = veh.get_items( carg->part_index() );
                for( auto &items_it : items ) {
                    if( items_it.has_flag( "SLEEP_AID" ) ) {
                        // Note: BED + SLEEP_AID = 9 pts, or 1 pt below very_comfortable
                        comfort += 1 + static_cast<int>( comfort_level::slightly_comfortable );
                        add_msg_if_player( m_info, _( "You use your %s for comfort." ), items_it.tname() );
                        break; // prevents using more than 1 sleep aid
                    }
                }
            }
            if( board ) {
                comfort += board->info().comfort;
            } else {
                comfort -= g->m.move_cost( p );
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
            comfort -= g->m.move_cost( p );
        }

        auto items = g->m.i_at( p );
        for( auto &items_it : items ) {
            if( items_it.has_flag( "SLEEP_AID" ) ) {
                // Note: BED + SLEEP_AID = 9 pts, or 1 pt below very_comfortable
                comfort += 1 + static_cast<int>( comfort_level::slightly_comfortable );
                add_msg_if_player( m_info, _( "You use your %s for comfort." ), items_it.tname() );
                break; // prevents using more than 1 sleep aid
            }
        }

        if( fungaloid_cosplay && g->m.has_flag_ter_or_furn( "FUNGUS", pos() ) ) {
            comfort += static_cast<int>( comfort_level::very_comfortable );
        } else if( watersleep && g->m.has_flag_ter( "SWIMMABLE", pos() ) ) {
            comfort += static_cast<int>( comfort_level::very_comfortable );
        }
    } else if( plantsleep ) {
        if( vp || furn_at_pos != f_null ) {
            // Sleep ain't happening in a vehicle or on furniture
            comfort = static_cast<int>( comfort_level::uncomfortable );
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
                comfort = static_cast<int>( comfort_level::uncomfortable );
            }
        }
        // Has webforce
    } else {
        if( web >= 3 ) {
            // Thick Web and you're good to go
            comfort += static_cast<int>( comfort_level::very_comfortable );
        } else {
            comfort = static_cast<int>( comfort_level::uncomfortable );
        }
    }

    if( comfort > static_cast<int>( comfort_level::comfortable ) ) {
        return comfort_level::very_comfortable;
    } else if( comfort > static_cast<int>( comfort_level::slightly_comfortable ) ) {
        return comfort_level::comfortable;
    } else if( comfort > static_cast<int>( comfort_level::neutral ) ) {
        return comfort_level::slightly_comfortable;
    } else if( comfort == static_cast<int>( comfort_level::neutral ) ) {
        return comfort_level::neutral;
    } else {
        return comfort_level::uncomfortable;
    }
}

int player::sleep_spot( const tripoint &p ) const
{
    comfort_level base_level = base_comfort_value( p );
    int sleepy = static_cast<int>( base_level );
    bool watersleep = has_trait( trait_WATERSLEEP );

    if( has_addiction( ADD_SLEEP ) ) {
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
    if( watersleep && g->m.has_flag_ter( "SWIMMABLE", pos() ) ) {
        sleepy += 10; //comfy water!
    }

    if( get_fatigue() < TIRED + 1 ) {
        sleepy -= static_cast<int>( ( TIRED + 1 - get_fatigue() ) / 4 );
    } else {
        sleepy += static_cast<int>( ( get_fatigue() - TIRED + 1 ) / 16 );
    }

    if( stim > 0 || !has_trait( trait_INSOMNIA ) ) {
        sleepy -= 2 * stim;
    } else {
        // Make it harder for insomniac to get around the trait
        sleepy -= stim;
    }

    return sleepy;
}

bool player::can_sleep()
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

void player::fall_asleep()
{
    // Communicate to the player that he is using items on the floor
    std::string item_name = is_snuggling();
    if( item_name == "many" ) {
        if( one_in( 15 ) ) {
            add_msg_if_player( _( "You nestle your pile of clothes for warmth." ) );
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
    if( has_active_mutation( trait_id( "HIBERNATE" ) ) &&
        get_kcal_percent() > 0.8f ) {
        if( is_avatar() ) {
            g->memorial().add( pgettext( "memorial_male", "Entered hibernation." ),
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

void player::fall_asleep( const time_duration &duration )
{
    if( activity ) {
        if( activity.id() == "ACT_TRY_SLEEP" ) {
            activity.set_to_null();
        } else {
            cancel_activity();
        }
    }
    add_effect( effect_sleep, duration );
}

std::string player::is_snuggling() const
{
    auto begin = g->m.i_at( pos() ).begin();
    auto end = g->m.i_at( pos() ).end();

    if( in_vehicle ) {
        if( const cata::optional<vpart_reference> vp = g->m.veh_at( pos() ).part_with_feature( VPFLAG_CARGO,
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
                   ( candidate->covers( bp_torso ) || candidate->covers( bp_leg_l ) ||
                     candidate->covers( bp_leg_r ) ) ) {
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

// Returned values range from 1.0 (unimpeded vision) to 11.0 (totally blind).
//  1.0 is LIGHT_AMBIENT_LIT or brighter
//  4.0 is a dark clear night, barely bright enough for reading and crafting
//  6.0 is LIGHT_AMBIENT_DIM
//  7.3 is LIGHT_AMBIENT_MINIMAL, a dark cloudy night, unlit indoors
// 11.0 is zero light or blindness
float player::fine_detail_vision_mod( const tripoint &p ) const
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
    float own_light = std::max( 1.0, LIGHT_AMBIENT_LIT - active_light() - 2 );

    // Same calculation as above, but with a result 3 lower.
    float ambient_light = std::max( 1.0,
                                    LIGHT_AMBIENT_LIT - g->m.ambient_light_at( p == tripoint_zero ? pos() : p ) + 1.0 );

    return std::min( own_light, ambient_light );
}

int player::get_wind_resistance( body_part bp ) const
{
    int coverage = 0;
    float totalExposed = 1.0;
    int totalCoverage = 0;
    int penalty = 100;

    for( auto &i : worn ) {
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

            coverage = std::max( 0, i.get_coverage() - penalty );
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

int player::warmth( body_part bp ) const
{
    int ret = 0;
    int warmth = 0;

    for( auto &i : worn ) {
        if( i.covers( bp ) ) {
            warmth = i.get_warmth();
            // Wool items do not lose their warmth due to being wet.
            // Warmth is reduced by 0 - 66% based on wetness.
            if( !i.made_of( material_id( "wool" ) ) ) {
                warmth *= 1.0 - 0.66 * body_wetness[bp] / drench_capacity[bp];
            }
            ret += warmth;
        }
    }
    ret += get_effect_int( efftype_id( "heating_bionic" ), bp );
    return ret;
}

static int bestwarmth( const std::list< item > &its, const std::string &flag )
{
    int best = 0;
    for( auto &w : its ) {
        if( w.has_flag( flag ) && w.get_warmth() > best ) {
            best = w.get_warmth();
        }
    }
    return best;
}

int player::bonus_item_warmth( body_part bp ) const
{
    int ret = 0;

    // If the player is not wielding anything big, check if hands can be put in pockets
    if( ( bp == bp_hand_l || bp == bp_hand_r ) && weapon.volume() < 500_ml ) {
        ret += bestwarmth( worn, "POCKETS" );
    }

    // If the player's head is not encumbered, check if hood can be put up
    if( bp == bp_head && encumb( bp_head ) < 10 ) {
        ret += bestwarmth( worn, "HOOD" );
    }

    // If the player's mouth is not encumbered, check if collar can be put up
    if( bp == bp_mouth && encumb( bp_mouth ) < 10 ) {
        ret += bestwarmth( worn, "COLLAR" );
    }

    return ret;
}

int player::get_armor_bash( body_part bp ) const
{
    return get_armor_bash_base( bp ) + armor_bash_bonus;
}

int player::get_armor_cut( body_part bp ) const
{
    return get_armor_cut_base( bp ) + armor_cut_bonus;
}

int player::get_armor_type( damage_type dt, body_part bp ) const
{
    switch( dt ) {
        case DT_TRUE:
        case DT_BIOLOGICAL:
            return 0;
        case DT_BASH:
            return get_armor_bash( bp );
        case DT_CUT:
            return get_armor_cut( bp );
        case DT_STAB:
            return get_armor_cut( bp ) * 0.8f;
        case DT_ACID:
        case DT_HEAT:
        case DT_COLD:
        case DT_ELECTRIC: {
            int ret = 0;
            for( auto &i : worn ) {
                if( i.covers( bp ) ) {
                    ret += i.damage_resist( dt );
                }
            }

            ret += mutation_armor( bp, dt );
            return ret;
        }
        case DT_NULL:
        case NUM_DT:
            // Let it error below
            break;
    }

    debugmsg( "Invalid damage type: %d", dt );
    return 0;
}

int player::get_armor_bash_base( body_part bp ) const
{
    int ret = 0;
    for( auto &i : worn ) {
        if( i.covers( bp ) ) {
            ret += i.bash_resist();
        }
    }
    if( has_bionic( bio_carbon ) ) {
        ret += 2;
    }
    if( bp == bp_head && has_bionic( bio_armor_head ) ) {
        ret += 3;
    }
    if( ( bp == bp_arm_l || bp == bp_arm_r ) && has_bionic( bio_armor_arms ) ) {
        ret += 3;
    }
    if( bp == bp_torso && has_bionic( bio_armor_torso ) ) {
        ret += 3;
    }
    if( ( bp == bp_leg_l || bp == bp_leg_r ) && has_bionic( bio_armor_legs ) ) {
        ret += 3;
    }
    if( bp == bp_eyes && has_bionic( bio_armor_eyes ) ) {
        ret += 3;
    }

    ret += mutation_armor( bp, DT_BASH );
    return ret;
}

int player::get_armor_cut_base( body_part bp ) const
{
    int ret = 0;
    for( auto &i : worn ) {
        if( i.covers( bp ) ) {
            ret += i.cut_resist();
        }
    }
    if( has_bionic( bio_carbon ) ) {
        ret += 4;
    }
    if( bp == bp_head && has_bionic( bio_armor_head ) ) {
        ret += 3;
    } else if( ( bp == bp_arm_l || bp == bp_arm_r ) && has_bionic( bio_armor_arms ) ) {
        ret += 3;
    } else if( bp == bp_torso && has_bionic( bio_armor_torso ) ) {
        ret += 3;
    } else if( ( bp == bp_leg_l || bp == bp_leg_r ) && has_bionic( bio_armor_legs ) ) {
        ret += 3;
    } else if( bp == bp_eyes && has_bionic( bio_armor_eyes ) ) {
        ret += 3;
    }

    ret += mutation_armor( bp, DT_CUT );
    return ret;
}

int player::get_armor_acid( body_part bp ) const
{
    return get_armor_type( DT_ACID, bp );
}

int player::get_armor_fire( body_part bp ) const
{
    return get_armor_type( DT_HEAT, bp );
}

int player::get_env_resist( body_part bp ) const
{
    int ret = 0;
    for( auto &i : worn ) {
        // Head protection works on eyes too (e.g. baseball cap)
        if( i.covers( bp ) || ( bp == bp_eyes && i.covers( bp_head ) ) ) {
            ret += i.get_env_resist();
        }
    }

    for( const bionic &bio : *my_bionics ) {
        const auto EP = bio.info().env_protec.find( bp );
        if( EP != bio.info().env_protec.end() ) {
            ret += EP->second;
        }
    }

    if( bp == bp_eyes && has_trait( trait_SEESLEEP ) ) {
        ret += 8;
    }
    return ret;
}

bool player::natural_attack_restricted_on( body_part bp ) const
{
    for( auto &i : worn ) {
        if( i.covers( bp ) && !i.has_flag( "ALLOWS_NATURAL_ATTACKS" ) && !i.has_flag( "SEMITANGIBLE" ) &&
            !i.has_flag( "PERSONAL" ) && !i.has_flag( "AURA" ) ) {
            return true;
        }
    }
    return false;
}

bool player::is_wearing_helmet() const
{
    for( const item &i : worn ) {
        if( i.covers( bp_head ) && !i.has_flag( "HELMET_COMPAT" ) && !i.has_flag( "SKINTIGHT" ) &&
            !i.has_flag( "PERSONAL" ) && !i.has_flag( "AURA" ) && !i.has_flag( "SEMITANGIBLE" ) &&
            !i.has_flag( "OVERSIZE" ) ) {
            return true;
        }
    }
    return false;
}

int player::head_cloth_encumbrance() const
{
    int ret = 0;
    for( auto &i : worn ) {
        const item *worn_item = &i;
        if( i.covers( bp_head ) && !i.has_flag( "SEMITANGIBLE" ) &&
            ( worn_item->has_flag( "HELMET_COMPAT" ) || worn_item->has_flag( "SKINTIGHT" ) ) ) {
            ret += worn_item->get_encumber( *this );
        }
    }
    return ret;
}

double player::armwear_factor() const
{
    double ret = 0;
    if( wearing_something_on( bp_arm_l ) ) {
        ret += .5;
    }
    if( wearing_something_on( bp_arm_r ) ) {
        ret += .5;
    }
    return ret;
}

int player::shoe_type_count( const itype_id &it ) const
{
    int ret = 0;
    if( is_wearing_on_bp( it, bp_foot_l ) ) {
        ret++;
    }
    if( is_wearing_on_bp( it, bp_foot_r ) ) {
        ret++;
    }
    return ret;
}

int player::adjust_for_focus( int amount ) const
{
    int effective_focus = focus_pool;
    if( has_trait( trait_FASTLEARNER ) ) {
        effective_focus += 15;
    }
    if( has_active_bionic( bio_memory ) ) {
        effective_focus += 10;
    }
    if( has_trait( trait_SLOWLEARNER ) ) {
        effective_focus -= 15;
    }
    double tmp = amount * ( effective_focus / 100.0 );
    return roll_remainder( tmp );
}

void player::practice( const skill_id &id, int amount, int cap, bool suppress_warning )
{
    SkillLevel &level = get_skill_level_object( id );
    const Skill &skill = id.obj();
    std::string skill_name = skill.name();

    if( !level.can_train() ) {
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
    if( has_trait( trait_PRED2 ) && skill.is_combat_skill() ) {
        if( one_in( 3 ) ) {
            amount *= 2;
        }
    }
    if( has_trait( trait_PRED3 ) && skill.is_combat_skill() ) {
        amount *= 2;
    }

    if( has_trait( trait_PRED4 ) && skill.is_combat_skill() ) {
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
        if( is_player() && newLevel > oldLevel ) {
            add_msg( m_good, _( "Your skill in %s has increased to %d!" ), skill_name, newLevel );
        }
        if( is_player() && newLevel > cap ) {
            //inform player immediately that the current recipe can't be used to train further
            add_msg( m_info, _( "You feel that %s tasks of this level are becoming trivial." ),
                     skill_name );
        }

        int chance_to_drop = focus_pool;
        focus_pool -= chance_to_drop / 100;
        // Apex Predators don't think about much other than killing.
        // They don't lose Focus when practicing combat skills.
        if( ( rng( 1, 100 ) <= ( chance_to_drop % 100 ) ) && ( !( has_trait( trait_PRED4 ) &&
                skill.is_combat_skill() ) ) ) {
            focus_pool--;
        }
    }

    get_skill_level_object( id ).practice();
}

void player::handle_skill_warning( const skill_id &id, bool force_warning )
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

int player::exceeds_recipe_requirements( const recipe &rec ) const
{
    return get_all_skills().exceeds_recipe_requirements( rec );
}

bool player::has_recipe_requirements( const recipe &rec ) const
{
    return get_all_skills().has_recipe_requirements( rec );
}

bool player::can_decomp_learn( const recipe &rec ) const
{
    return !rec.learn_by_disassembly.empty() &&
           meets_skill_requirements( rec.learn_by_disassembly );
}

bool player::knows_recipe( const recipe *rec ) const
{
    return get_learned_recipes().contains( rec );
}

int player::has_recipe( const recipe *r, const inventory &crafting_inv,
                        const std::vector<npc *> &helpers ) const
{
    if( !r->skill_used ) {
        return 0;
    }

    if( knows_recipe( r ) ) {
        return r->difficulty;
    }

    const auto available = get_available_recipes( crafting_inv, &helpers );
    return available.contains( r ) ? available.get_custom_difficulty( r ) : -1;
}

void player::learn_recipe( const recipe *const rec )
{
    if( rec->never_learn ) {
        return;
    }
    learned_recipes->include( rec );
}

void player::assign_activity( const activity_id &type, int moves, int index, int pos,
                              const std::string &name )
{
    assign_activity( player_activity( type, moves, index, pos, name ) );
}

void player::assign_activity( const player_activity &act, bool allow_resume )
{
    if( allow_resume && !backlog.empty() && backlog.front().can_resume_with( act, *this ) ) {
        add_msg_if_player( _( "You resume your task." ) );
        activity = backlog.front();
        backlog.pop_front();
    } else {
        if( activity ) {
            backlog.push_front( activity );
        }

        activity = act;
    }

    if( activity.rooted() ) {
        rooted_message();
    }
    if( is_npc() ) {
        npc *guy = dynamic_cast<npc *>( this );
        guy->set_attitude( NPCATT_ACTIVITY );
        guy->set_mission( NPC_MISSION_ACTIVITY );
        guy->current_activity_id = activity.id();
    }
}

bool player::has_activity( const activity_id &type ) const
{
    return activity.id() == type;
}

bool player::has_activity( const std::vector<activity_id> &types ) const
{
    return std::find( types.begin(), types.end(), activity.id() ) != types.end() ;
}

void player::cancel_activity()
{
    if( has_activity( activity_id( "ACT_MOVE_ITEMS" ) ) && is_hauling() ) {
        stop_hauling();
    }
    if( has_activity( activity_id( "ACT_TRY_SLEEP" ) ) ) {
        remove_value( "sleep_query" );
    }
    // Clear any backlog items that aren't auto-resume.
    for( auto backlog_item = backlog.begin(); backlog_item != backlog.end(); ) {
        if( backlog_item->auto_resume ) {
            backlog_item++;
        } else {
            backlog_item = backlog.erase( backlog_item );
        }
    }
    if( activity && activity.is_suspendable() ) {
        backlog.push_front( activity );
    }
    sfx::end_activity_sounds(); // kill activity sounds when canceled
    activity = player_activity();
}

void player::resume_backlog_activity()
{
    if( !backlog.empty() && backlog.front().auto_resume ) {
        activity = backlog.front();
        backlog.pop_front();
    }
}

bool player::has_gun_for_ammo( const ammotype &at ) const
{
    return has_item_with( [at]( const item & it ) {
        // item::ammo_type considers the active gunmod.
        return it.is_gun() && it.ammo_types().count( at );
    } );
}

bool player::has_magazine_for_ammo( const ammotype &at ) const
{
    return has_item_with( [&at]( const item & it ) {
        return !it.has_flag( "NO_RELOAD" ) &&
               ( ( it.is_magazine() && it.ammo_types().count( at ) ) ||
                 ( it.is_gun() && it.magazine_integral() && it.ammo_types().count( at ) ) ||
                 ( it.is_gun() && it.magazine_current() != nullptr &&
                   it.magazine_current()->ammo_types().count( at ) ) );
    } );
}

// mytest return weapon name to display in sidebar
std::string player::weapname( unsigned int truncate ) const
{
    if( weapon.is_gun() ) {
        std::string str = string_format( "(%s) %s", weapon.gun_current_mode().tname(), weapon.type_name() );

        // Is either the base item or at least one auxiliary gunmod loaded (includes empty magazines)
        bool base = weapon.ammo_capacity() > 0 && !weapon.has_flag( "RELOAD_AND_SHOOT" );

        const auto mods = weapon.gunmods();
        bool aux = std::any_of( mods.begin(), mods.end(), [&]( const item * e ) {
            return e->is_gun() && e->ammo_capacity() > 0 && !e->has_flag( "RELOAD_AND_SHOOT" );
        } );

        if( base || aux ) {
            str += " (";
            if( base ) {
                str += std::to_string( weapon.ammo_remaining() );
                if( weapon.magazine_integral() ) {
                    str += "/" + std::to_string( weapon.ammo_capacity() );
                }
            } else {
                str += "---";
            }
            str += ")";

            for( auto e : mods ) {
                if( e->is_gun() && e->ammo_capacity() > 0 && !e->has_flag( "RELOAD_AND_SHOOT" ) ) {
                    str += " (" + std::to_string( e->ammo_remaining() );
                    if( e->magazine_integral() ) {
                        str += "/" + std::to_string( e->ammo_capacity() );
                    }
                    str += ")";
                }
            }
        }
        return str;

    } else if( weapon.is_container() && weapon.contents.size() == 1 ) {
        return string_format( "%s (%d)", weapon.tname(),
                              weapon.contents.front().charges );

    } else if( !is_armed() ) {
        return _( "fists" );

    } else {
        return weapon.tname( 1, true, truncate );
    }
}

bool player::wield_contents( item &container, int pos, bool penalties, int base_cost )
{
    // if index not specified and container has multiple items then ask the player to choose one
    if( pos < 0 ) {
        std::vector<std::string> opts;
        std::transform( container.contents.begin(), container.contents.end(),
        std::back_inserter( opts ), []( const item & elem ) {
            return elem.display_name();
        } );
        if( opts.size() > 1 ) {
            pos = uilist( _( "Wield what?" ), opts );
            if( pos < 0 ) {
                return false;
            }
        } else {
            pos = 0;
        }
    }

    if( pos >= static_cast<int>( container.contents.size() ) ) {
        debugmsg( "Tried to wield non-existent item from container (player::wield_contents)" );
        return false;
    }

    auto target = std::next( container.contents.begin(), pos );
    const auto ret = can_wield( *target );
    if( !ret.success() ) {
        add_msg_if_player( m_info, "%s", ret.c_str() );
        return false;
    }

    int mv = 0;

    if( is_armed() ) {
        if( !unwield() ) {
            return false;
        }
        inv.unsort();
    }

    weapon = std::move( *target );
    container.contents.erase( target );
    container.on_contents_changed();

    inv.update_invlet( weapon );
    inv.update_cache_with_item( weapon );
    last_item = weapon.typeId();

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
    int lvl = get_skill_level( weapon.is_gun() ? weapon.gun_skill() : weapon.melee_skill() );
    mv += item_handling_cost( weapon, penalties, base_cost ) / ( ( lvl + 10.0f ) / 10.0f );

    moves -= mv;

    weapon.on_wield( *this, mv );

    return true;
}

void player::store( item &container, item &put, bool penalties, int base_cost )
{
    moves -= item_store_cost( put, container, penalties, base_cost );
    container.put_in( i_rem( &put ) );
    reset_encumbrance();
}

nc_color encumb_color( int level )
{
    if( level < 0 ) {
        return c_green;
    }
    if( level < 10 ) {
        return c_light_gray;
    }
    if( level < 40 ) {
        return c_yellow;
    }
    if( level < 70 ) {
        return c_light_red;
    }
    return c_red;
}

float player::get_melee() const
{
    return get_skill_level( skill_id( "melee" ) );
}

bool player::uncanny_dodge()
{
    bool is_u = this == &g->u;
    bool seen = g->u.sees( *this );
    if( this->get_power_level() < 74_kJ || !this->has_active_bionic( bio_uncanny_dodge ) ) {
        return false;
    }
    tripoint adjacent = adjacent_tile();
    mod_power_level( -75_kJ );
    if( adjacent.x != posx() || adjacent.y != posy() ) {
        position.x = adjacent.x;
        position.y = adjacent.y;
        if( is_u ) {
            add_msg( _( "Time seems to slow down and you instinctively dodge!" ) );
        } else if( seen ) {
            add_msg( _( "%s dodges... so fast!" ), this->disp_name() );

        }
        return true;
    }
    if( is_u ) {
        add_msg( _( "You try to dodge but there's no room!" ) );
    } else if( seen ) {
        add_msg( _( "%s tries to dodge but there's no room!" ), this->disp_name() );
    }
    return false;
}

int player::climbing_cost( const tripoint &from, const tripoint &to ) const
{
    if( !g->m.valid_move( from, to, false, true ) ) {
        return 0;
    }

    const int diff = g->m.climb_difficulty( from );

    if( diff > 5 ) {
        return 0;
    }

    return 50 + diff * 100;
    // TODO: All sorts of mutations, equipment weight etc.
}

void player::environmental_revert_effect()
{
    addictions.clear();
    morale->clear();

    for( int part = 0; part < num_hp_parts; part++ ) {
        hp_cur[part] = hp_max[part];
    }
    set_hunger( 0 );
    set_thirst( 0 );
    set_fatigue( 0 );
    set_healthy( 0 );
    set_healthy_mod( 0 );
    stim = 0;
    set_pain( 0 );
    set_painkiller( 0 );
    radiation = 0;

    recalc_sight_limits();
    reset_encumbrance();
}

void player::set_destination( const std::vector<tripoint> &route,
                              const player_activity &destination_activity )
{
    auto_move_route = route;
    this->destination_activity = destination_activity;
    destination_point.emplace( g->m.getabs( route.back() ) );
}

void player::clear_destination()
{
    auto_move_route.clear();
    destination_activity = player_activity();
    destination_point = cata::nullopt;
    next_expected_position = cata::nullopt;
}

bool player::has_distant_destination() const
{
    return has_destination() && !destination_activity.is_null() &&
           destination_activity.id() == "ACT_TRAVELLING" && !omt_path.empty();
}

bool player::has_destination() const
{
    return !auto_move_route.empty();
}

bool player::has_destination_activity() const
{
    return !destination_activity.is_null() && destination_point &&
           position == g->m.getlocal( *destination_point );
}

void player::start_destination_activity()
{
    if( !has_destination_activity() ) {
        debugmsg( "Tried to start invalid destination activity" );
        return;
    }

    assign_activity( destination_activity );
    clear_destination();
}

std::vector<tripoint> &player::get_auto_move_route()
{
    return auto_move_route;
}

action_id player::get_next_auto_move_direction()
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
    if( abs( dp.x ) > 1 || abs( dp.y ) > 1 || abs( dp.z ) > 1 ||
        ( abs( dp.z ) != 0 && ( abs( dp.x ) != 0 || abs( dp.y ) != 0 ) ) ) {
        // Should never happen, but check just in case
        return ACTION_NULL;
    }
    return get_movement_direction_from_delta( dp );
}

bool player::defer_move( const tripoint &next )
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

void player::shift_destination( const point &shift )
{
    if( next_expected_position ) {
        *next_expected_position += shift;
    }

    for( auto &elem : auto_move_route ) {
        elem += shift;
    }
}

void player::start_hauling()
{
    add_msg( _( "You start hauling items along the ground." ) );
    if( is_armed() ) {
        add_msg( m_warning, _( "Your hands are not free, which makes hauling slower." ) );
    }
    hauling = true;
}

void player::stop_hauling()
{
    add_msg( _( "You stop hauling items." ) );
    hauling = false;
    if( has_activity( activity_id( "ACT_MOVE_ITEMS" ) ) ) {
        cancel_activity();
    }
}

bool player::is_hauling() const
{
    return hauling;
}

bool player::has_weapon() const
{
    return !unarmed_attack();
}

m_size player::get_size() const
{
    if( has_trait( trait_id( "SMALL2" ) ) || has_trait( trait_id( "SMALL_OK" ) ) ||
        has_trait( trait_id( "SMALL" ) ) ) {
        return MS_SMALL;
    } else if( has_trait( trait_LARGE ) || has_trait( trait_LARGE_OK ) ) {
        return MS_LARGE;
    } else if( has_trait( trait_HUGE ) || has_trait( trait_HUGE_OK ) ) {
        return MS_HUGE;
    }
    return MS_MEDIUM;
}

int player::get_hp() const
{
    return get_hp( num_hp_parts );
}

int player::get_hp( hp_part bp ) const
{
    if( bp < num_hp_parts ) {
        return hp_cur[bp];
    }
    int hp_total = 0;
    for( int i = 0; i < num_hp_parts; ++i ) {
        hp_total += hp_cur[i];
    }
    return hp_total;
}

int player::get_hp_max() const
{
    return get_hp_max( num_hp_parts );
}

int player::get_hp_max( hp_part bp ) const
{
    if( bp < num_hp_parts ) {
        return hp_max[bp];
    }
    int hp_total = 0;
    for( int i = 0; i < num_hp_parts; ++i ) {
        hp_total += hp_max[i];
    }
    return hp_total;
}

int player::get_stamina_max() const
{
    int maxStamina = get_option< int >( "PLAYER_MAX_STAMINA" );
    maxStamina *= Character::mutation_value( "max_stamina_modifier" );
    return maxStamina;
}

void player::burn_move_stamina( int moves )
{
    int overburden_percentage = 0;
    units::mass current_weight = weight_carried();
    units::mass max_weight = weight_capacity();
    if( current_weight > max_weight ) {
        overburden_percentage = ( current_weight - max_weight ) * 100 / max_weight;
    }

    int burn_ratio = get_option<int>( "PLAYER_BASE_STAMINA_BURN_RATE" );
    if( g->u.has_active_bionic( bionic_id( "bio_torsionratchet" ) ) ) {
        burn_ratio = burn_ratio * 2 - 3;
    }
    for( const bionic_id &bid : get_bionic_fueled_with( item( "muscle" ) ) ) {
        if( has_active_bionic( bid ) ) {
            burn_ratio = burn_ratio * 2 - 3;
        }
    }
    burn_ratio += overburden_percentage;
    if( move_mode == CMM_RUN ) {
        burn_ratio = burn_ratio * 7;
    }
    mod_stat( "stamina", -( ( moves * burn_ratio ) / 100.0 ) );
    add_msg( m_debug, "Stamina burn: %d", -( ( moves * burn_ratio ) / 100 ) );
    // Chance to suffer pain if overburden and stamina runs out or has trait BADBACK
    // Starts at 1 in 25, goes down by 5 for every 50% more carried
    if( ( current_weight > max_weight ) && ( has_trait( trait_BADBACK ) || stamina == 0 ) &&
        one_in( 35 - 5 * current_weight / ( max_weight / 2 ) ) ) {
        add_msg_if_player( m_bad, _( "Your body strains under the weight!" ) );
        // 1 more pain for every 800 grams more (5 per extra STR needed)
        if( ( ( current_weight - max_weight ) / 800_gram > get_pain() && get_pain() < 100 ) ) {
            mod_pain( 1 );
        }
    }
}

Creature::Attitude player::attitude_to( const Creature &other ) const
{
    const auto m = dynamic_cast<const monster *>( &other );
    if( m != nullptr ) {
        if( m->friendly != 0 ) {
            return A_FRIENDLY;
        }
        switch( m->attitude( const_cast<player *>( this ) ) ) {
            // player probably does not want to harm them, but doesn't care much at all.
            case MATT_FOLLOW:
            case MATT_FPASSIVE:
            case MATT_IGNORE:
            case MATT_FLEE:
                return A_NEUTRAL;
            // player does not want to harm those.
            case MATT_FRIEND:
            case MATT_ZLAVE: // Don't want to harm your zlave!
                return A_FRIENDLY;
            case MATT_ATTACK:
                return A_HOSTILE;
            case MATT_NULL:
            case NUM_MONSTER_ATTITUDES:
                break;
        }

        return A_NEUTRAL;
    }

    const auto p = dynamic_cast<const npc *>( &other );
    if( p != nullptr ) {
        if( p->is_enemy() ) {
            return A_HOSTILE;
        } else if( p->is_player_ally() ) {
            return A_FRIENDLY;
        } else {
            return A_NEUTRAL;
        }
    } else if( &other == this ) {
        return A_FRIENDLY;
    }

    return A_NEUTRAL;
}

bool player::sees( const tripoint &t, bool, int ) const
{
    static const bionic_id str_bio_night( "bio_night" );
    const int wanted_range = rl_dist( pos(), t );
    bool can_see = is_player() ? g->m.pl_sees( t, wanted_range ) :
                   Creature::sees( t );
    // Clairvoyance is now pretty cheap, so we can check it early
    if( wanted_range < MAX_CLAIRVOYANCE && wanted_range < clairvoyance() ) {
        return true;
    }
    // Only check if we need to override if we already came to the opposite conclusion.
    if( can_see && wanted_range < 15 && wanted_range > sight_range( 1 ) &&
        has_active_bionic( str_bio_night ) ) {
        can_see = false;
    }
    if( can_see && wanted_range > unimpaired_range() ) {
        can_see = false;
    }

    return can_see;
}

bool player::sees( const Creature &critter ) const
{
    // This handles only the player/npc specific stuff (monsters don't have traits or bionics).
    const int dist = rl_dist( pos(), critter.pos() );
    if( dist <= 3 && has_active_mutation( trait_ANTENNAE ) ) {
        return true;
    }
    if( critter.digging() && has_active_bionic( bio_ground_sonar ) ) {
        // Bypass the check below, the bionic sonar also bypasses the sees(point) check because
        // walls don't block sonar which is transmitted in the ground, not the air.
        // TODO: this might need checks whether the player is in the air, or otherwise not connected
        // to the ground. It also might need a range check.
        return true;
    }
    return Creature::sees( critter );
}

nc_color player::bodytemp_color( int bp ) const
{
    nc_color color = c_light_gray; // default
    if( bp == bp_eyes ) {
        color = c_light_gray;    // Eyes don't count towards warmth
    } else if( temp_conv[bp]  > BODYTEMP_SCORCHING ) {
        color = c_red;
    } else if( temp_conv[bp]  > BODYTEMP_VERY_HOT ) {
        color = c_light_red;
    } else if( temp_conv[bp]  > BODYTEMP_HOT ) {
        color = c_yellow;
    } else if( temp_conv[bp]  > BODYTEMP_COLD ) {
        color = c_green;
    } else if( temp_conv[bp]  > BODYTEMP_VERY_COLD ) {
        color = c_light_blue;
    } else if( temp_conv[bp]  > BODYTEMP_FREEZING ) {
        color = c_cyan;
    } else if( temp_conv[bp] <= BODYTEMP_FREEZING ) {
        color = c_blue;
    }
    return color;
}

//message related stuff
void player::add_msg_if_player( const std::string &msg ) const
{
    Messages::add_msg( msg );
}

void player::add_msg_player_or_npc( const std::string &player_msg,
                                    const std::string &/*npc_msg*/ ) const
{
    Messages::add_msg( player_msg );
}

void player::add_msg_if_player( const game_message_type type, const std::string &msg ) const
{
    Messages::add_msg( type, msg );
}

void player::add_msg_player_or_npc( const game_message_type type, const std::string &player_msg,
                                    const std::string &/*npc_msg*/ ) const
{
    Messages::add_msg( type, player_msg );
}

void player::add_msg_player_or_say( const std::string &player_msg,
                                    const std::string &/*npc_speech*/ ) const
{
    Messages::add_msg( player_msg );
}

void player::add_msg_player_or_say( const game_message_type type, const std::string &player_msg,
                                    const std::string &/*npc_speech*/ ) const
{
    Messages::add_msg( type, player_msg );
}

bool player::knows_trap( const tripoint &pos ) const
{
    const tripoint p = g->m.getabs( pos );
    return known_traps.count( p ) > 0;
}

void player::add_known_trap( const tripoint &pos, const trap &t )
{
    const tripoint p = g->m.getabs( pos );
    if( t.is_null() ) {
        known_traps.erase( p );
    } else {
        // TODO: known_traps should map to a trap_str_id
        known_traps[p] = t.id.str();
    }
}

bool player::is_deaf() const
{
    return get_effect_int( effect_deaf ) > 2 || worn_with_flag( "DEAF" ) || has_trait( trait_DEAF ) ||
           ( has_active_bionic( bio_earplugs ) && !has_active_bionic( bio_ears ) ) ||
           ( has_trait( trait_M_SKIN3 ) && g->m.has_flag_ter_or_furn( "FUNGUS", pos() ) && in_sleep_state() );
}

bool player::can_hear( const tripoint &source, const int volume ) const
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
    return ( volume - weather::sound_attn( g->weather.weather ) ) * volume_multiplier >= dist;
}

float player::hearing_ability() const
{
    float volume_multiplier = 1.0;

    // Mutation/Bionic volume modifiers
    if( has_active_bionic( bio_ears ) && !has_active_bionic( bio_earplugs ) ) {
        volume_multiplier *= 3.5;
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
        volume_multiplier *= .25;
    }

    return volume_multiplier;
}

std::string player::visible_mutations( const int visibility_cap ) const
{
    const std::string trait_str = enumerate_as_string( my_mutations.begin(), my_mutations.end(),
    [visibility_cap ]( const std::pair<trait_id, trait_data> &pr ) -> std::string {
        const auto &mut_branch = pr.first.obj();
        // Finally some use for visibility trait of mutations
        if( mut_branch.visibility > 0 && mut_branch.visibility >= visibility_cap )
        {
            return colorize( mut_branch.name(), mut_branch.get_display_color() );
        }

        return std::string();
    } );
    return trait_str;
}

std::vector<std::string> player::short_description_parts() const
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

std::string player::short_description() const
{
    return join( short_description_parts(), ";   " );
}

int player::print_info( const catacurses::window &w, int vStart, int, int column ) const
{
    mvwprintw( w, point( column, vStart++ ), _( "You (%s)" ), name );
    return vStart;
}

bool player::is_visible_in_range( const Creature &critter, const int range ) const
{
    return sees( critter ) && rl_dist( pos(), critter.pos() ) <= range;
}

std::vector<Creature *> player::get_visible_creatures( const int range ) const
{
    return g->get_creatures_if( [this, range]( const Creature & critter ) -> bool {
        return this != &critter && pos() != critter.pos() && // TODO: get rid of fake npcs (pos() check)
        rl_dist( pos(), critter.pos() ) <= range && sees( critter );
    } );
}

std::vector<Creature *> player::get_targetable_creatures( const int range ) const
{
    return g->get_creatures_if( [this, range]( const Creature & critter ) -> bool {
        return this != &critter && pos() != critter.pos() && // TODO: get rid of fake npcs (pos() check)
        round( rl_dist_exact( pos(), critter.pos() ) ) <= range &&
        ( sees( critter ) || sees_with_infrared( critter ) );
    } );
}

std::vector<Creature *> player::get_hostile_creatures( int range ) const
{
    return g->get_creatures_if( [this, range]( const Creature & critter ) -> bool {
        float dist_to_creature;
        // Fixes circular distance range for ranged attacks
        dist_to_creature = round( rl_dist_exact( pos(), critter.pos() ) );
        return this != &critter && pos() != critter.pos() && // TODO: get rid of fake npcs (pos() check)
        dist_to_creature <= range && critter.attitude_to( *this ) == A_HOSTILE
        && sees( critter );
    } );
}

void player::place_corpse()
{
    //If the character/NPC is on a distant mission, don't drop their their gear when they die since they still have a local pos
    if( !death_drops ) {
        return;
    }
    std::vector<item *> tmp = inv_dump();
    item body = item::make_corpse( mtype_id::NULL_ID(), calendar::turn, name );
    body.set_item_temperature( 310.15 );
    for( auto itm : tmp ) {
        g->m.add_item_or_charges( pos(), *itm );
    }
    for( auto &bio : *my_bionics ) {
        if( item::type_is_defined( bio.id.str() ) ) {
            item cbm( bio.id.str(), calendar::turn );
            cbm.set_flag( "FILTHY" );
            cbm.set_flag( "NO_STERILE" );
            cbm.set_flag( "NO_PACKED" );
            cbm.faults.emplace( fault_id( "fault_bionic_salvaged" ) );
            body.put_in( cbm );
        }
    }

    // Restore amount of installed pseudo-modules of Power Storage Units
    std::pair<int, int> storage_modules = amount_of_storage_bionics();
    for( int i = 0; i < storage_modules.first; ++i ) {
        body.emplace_back( "bio_power_storage" );
    }
    for( int i = 0; i < storage_modules.second; ++i ) {
        body.emplace_back( "bio_power_storage_mkII" );
    }
    g->m.add_item_or_charges( pos(), body );
}

void player::place_corpse( const tripoint &om_target )
{
    tinymap bay;
    bay.load( tripoint( om_target.x * 2, om_target.y * 2, om_target.z ), false );
    int finX = rng( 1, SEEX * 2 - 2 );
    int finY = rng( 1, SEEX * 2 - 2 );
    // This makes no sense at all. It may find a random tile without furniture, but
    // if the first try to find one fails, it will go through all tiles of the map
    // and essentially select the last one that has no furniture.
    // Q: Why check for furniture? (Check for passable or can-place-items seems more useful.)
    // Q: Why not grep a random point out of all the possible points (e.g. via random_entry)?
    // Q: Why use furn_str_id instead of f_null?
    // @todo fix it, see above.
    if( bay.furn( point( finX, finY ) ) != furn_str_id( "f_null" ) ) {
        for( const tripoint &p : bay.points_on_zlevel() ) {
            if( bay.furn( p ) == furn_str_id( "f_null" ) ) {
                finX = p.x;
                finY = p.y;
            }
        }
    }

    std::vector<item *> tmp = inv_dump();
    item body = item::make_corpse( mtype_id::NULL_ID(), calendar::turn, name );
    for( auto itm : tmp ) {
        bay.add_item_or_charges( point( finX, finY ), *itm );
    }
    for( auto &bio : *my_bionics ) {
        if( item::type_is_defined( bio.id.str() ) ) {
            body.put_in( item( bio.id.str(), calendar::turn ) );
        }
    }

    // Restore amount of installed pseudo-modules of Power Storage Units
    std::pair<int, int> storage_modules = amount_of_storage_bionics();
    for( int i = 0; i < storage_modules.first; ++i ) {
        body.emplace_back( "bio_power_storage" );
    }
    for( int i = 0; i < storage_modules.second; ++i ) {
        body.emplace_back( "bio_power_storage_mkII" );
    }
    bay.add_item_or_charges( point( finX, finY ), body );
}

bool player::sees_with_infrared( const Creature &critter ) const
{
    // electroreceptors grants vision of robots and electric monsters through walls
    if( has_trait( trait_ELECTRORECEPTORS ) &&
        ( critter.in_species( ROBOT ) || critter.has_flag( MF_ELECTRIC ) ) ) {
        return true;
    }

    if( !vision_mode_cache[IR_VISION] || !critter.is_warm() ) {
        return false;
    }

    if( is_player() || critter.is_player() ) {
        // Players should not use map::sees
        // Likewise, players should not be "looked at" with map::sees, not to break symmetry
        return g->m.pl_line_of_sight( critter.pos(),
                                      sight_range( current_daylight_level( calendar::turn ) ) );
    }

    return g->m.sees( pos(), critter.pos(), sight_range( current_daylight_level( calendar::turn ) ) );
}

float player::power_rating() const
{
    int dmg = std::max( { weapon.damage_melee( DT_BASH ),
                          weapon.damage_melee( DT_CUT ),
                          weapon.damage_melee( DT_STAB )
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
    if( has_trait( trait_HUGE ) || has_trait( trait_HUGE_OK ) ) {
        ret += 1;
    }
    if( is_wearing_power_armor( nullptr ) ) {
        ret = 5; // No mercy!
    }
    return ret;
}

float player::speed_rating() const
{
    float ret = get_speed() / 100.0f;
    ret *= 100.0f / run_cost( 100, false );
    // Adjustment for player being able to run, but not doing so at the moment
    if( move_mode != CMM_RUN ) {
        ret *= 1.0f + ( static_cast<float>( stamina ) / static_cast<float>( get_stamina_max() ) );
    }
    return ret;
}

std::vector<const item *> player::all_items_with_flag( const std::string &flag ) const
{
    return items_with( [&flag]( const item & it ) {
        return it.has_flag( flag );
    } );
}

item &player::item_with_best_of_quality( const quality_id &qid )
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

bool player::crush_frozen_liquid( item_location loc )
{

    player &u = g->u;

    if( u.has_quality( quality_id( "HAMMER" ) ) ) {
        item hammering_item = u.item_with_best_of_quality( quality_id( "HAMMER" ) );
        //~ %1$s: item to be crushed, %2$s: hammer name
        if( query_yn( _( "Do you want to crush up %1$s with your %2$s?\n"
                         "<color_red>Be wary of fragile items nearby!</color>" ),
                      loc.get_item()->display_name(), hammering_item.tname() ) ) {

            //Risk smashing tile with hammering tool, risk is lower with higher dex, damage lower with lower strength
            if( one_in( 1 + u.dex_cur / 4 ) ) {
                add_msg_if_player( colorize( _( "You swing your %s wildly!" ), c_red ),
                                   hammering_item.tname() );
                int smashskill = u.str_cur + hammering_item.damage_melee( DT_BASH );
                g->m.bash( loc.position(), smashskill );
            }
            add_msg_if_player( _( "You crush up and gather %s" ), loc.get_item()->display_name() );
            return true;
        }
    } else {
        popup( _( "You need a hammering tool to crush up frozen liquids!" ) );
    }
    return false;
}

bool player::has_item_with_flag( const std::string &flag, bool need_charges ) const
{
    return has_item_with( [&flag, &need_charges]( const item & it ) {
        if( it.is_tool() && need_charges ) {
            return it.has_flag( flag ) && it.type->tool->max_charges ? it.charges > 0 : it.has_flag( flag );
        }
        return it.has_flag( flag );
    } );
}

void player::on_mutation_gain( const trait_id &mid )
{
    morale->on_mutation_gain( mid );
    magic.on_mutation_gain( mid, *this );
}

void player::on_mutation_loss( const trait_id &mid )
{
    morale->on_mutation_loss( mid );
    magic.on_mutation_loss( mid );
}

void player::on_stat_change( const std::string &stat, int value )
{
    morale->on_stat_change( stat, value );
}

void player::on_item_wear( const item &it )
{
    morale->on_item_wear( it );
}

void player::on_item_takeoff( const item &it )
{
    morale->on_item_takeoff( it );
}

void player::on_worn_item_washed( const item &it )
{
    if( is_worn( it ) ) {
        morale->on_worn_item_washed( it );
    }
}

void player::on_effect_int_change( const efftype_id &eid, int intensity, body_part bp )
{
    // Adrenaline can reduce perceived pain (or increase it when you enter comedown).
    // See @ref get_perceived_pain()
    if( eid == effect_adrenaline ) {
        // Note that calling this does no harm if it wasn't changed.
        on_stat_change( "perceived_pain", get_perceived_pain() );
    }

    morale->on_effect_int_change( eid, intensity, bp );
}

const targeting_data &player::get_targeting_data()
{
    if( tdata == nullptr ) {
        debugmsg( "Tried to get targeting data before setting it" );
        tdata.reset( new targeting_data() );
        tdata->relevant = nullptr;
        g->u.cancel_activity();
    }

    return *tdata;
}

void player::set_targeting_data( const targeting_data &td )
{
    tdata.reset( new targeting_data( td ) );
}

bool player::query_yn( const std::string &mes ) const
{
    return ::query_yn( mes );
}

const pathfinding_settings &player::get_pathfinding_settings() const
{
    return *path_settings;
}

std::set<tripoint> player::get_path_avoid() const
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

bool player::is_rad_immune() const
{
    bool has_helmet = false;
    return ( is_wearing_power_armor( &has_helmet ) && has_helmet ) || worn_with_flag( "RAD_PROOF" );
}

void player::do_skill_rust()
{
    const int rate = rust_rate();
    if( rate <= 0 ) {
        return;
    }
    for( auto &pair : *_skills ) {
        if( rate <= rng( 0, 1000 ) ) {
            continue;
        }

        const Skill &aSkill = *pair.first;
        SkillLevel &skill_level_obj = pair.second;

        if( aSkill.is_combat_skill() &&
            ( ( has_trait( trait_PRED2 ) && one_in( 4 ) ) ||
              ( has_trait( trait_PRED3 ) && one_in( 2 ) ) ||
              ( has_trait( trait_PRED4 ) && x_in_y( 2, 3 ) ) ) ) {
            // Their brain is optimized to remember this
            if( one_in( 15600 ) ) {
                // They've already passed the roll to avoid rust at
                // this point, but print a message about it now and
                // then.
                //
                // 13 combat skills, 600 turns/hr, 7800 tests/hr.
                // This means PRED2/PRED3/PRED4 think of hunting on
                // average every 8/4/3 hours, enough for immersion
                // without becoming an annoyance.
                //
                add_msg_if_player( _( "Your heart races as you recall your most recent hunt." ) );
                stim++;
            }
            continue;
        }

        const bool charged_bio_mem = get_power_level() > 25_kJ && has_active_bionic( bio_memory );
        const int oldSkillLevel = skill_level_obj.level();
        if( skill_level_obj.rust( charged_bio_mem ) ) {
            add_msg_if_player( m_warning,
                               _( "Your knowledge of %s begins to fade, but your memory banks retain it!" ), aSkill.name() );
            mod_power_level( -25_kJ );
        }
        const int newSkill = skill_level_obj.level();
        if( newSkill < oldSkillLevel ) {
            add_msg_if_player( m_bad, _( "Your skill in %s has reduced to %d!" ), aSkill.name(), newSkill );
        }
    }
}

std::pair<std::string, nc_color> player::get_hunger_description() const
{
    const bool calorie_deficit = get_bmi() < character_weight_category::normal;
    const units::volume contains = stomach.contains();
    const units::volume cap = stomach.capacity();
    std::string hunger_string;
    nc_color hunger_color = c_white;
    // i ate just now!
    const bool just_ate = stomach.time_since_ate() < 15_minutes;
    // i ate a meal recently enough that i shouldn't need another meal
    const bool recently_ate = stomach.time_since_ate() < 3_hours;
    if( calorie_deficit ) {
        if( contains >= cap ) {
            hunger_string = _( "Engorged" );
            hunger_color = c_green;
        } else if( contains > cap * 3 / 4 ) {
            hunger_string = _( "Sated" );
            hunger_color = c_green;
        } else if( just_ate && contains > cap / 2 ) {
            hunger_string = _( "Full" );
            hunger_color = c_green;
        } else if( just_ate ) {
            hunger_string = _( "Hungry" );
            hunger_color = c_yellow;
        } else if( recently_ate ) {
            hunger_string = _( "Very Hungry" );
            hunger_color = c_yellow;
        } else if( get_bmi() < character_weight_category::emaciated ) {
            hunger_string = _( "Starving!" );
            hunger_color = c_red;
        } else if( get_bmi() < character_weight_category::underweight ) {
            hunger_string = _( "Near starving" );
            hunger_color = c_red;
        } else {
            hunger_string = _( "Famished" );
            hunger_color = c_light_red;
        }
    } else {
        if( contains >= cap * 5 / 6 ) {
            hunger_string = _( "Engorged" );
            hunger_color = c_green;
        } else if( contains > cap * 11 / 20 ) {
            hunger_string = _( "Sated" );
            hunger_color = c_green;
        } else if( recently_ate && contains >= cap * 3 / 8 ) {
            hunger_string = _( "Full" );
            hunger_color = c_green;
        } else if( ( stomach.time_since_ate() > 90_minutes && contains < cap / 8 && recently_ate ) ||
                   ( just_ate && contains > 0_ml && contains < cap * 3 / 8 ) ) {
            hunger_string = _( "Peckish" );
            hunger_color = c_dark_gray;
        } else if( !just_ate && ( recently_ate || contains > 0_ml ) ) {
            hunger_string.clear();
        } else {
            if( get_bmi() > character_weight_category::overweight ) {
                hunger_string = _( "Hungry" );
            } else {
                hunger_string = _( "Very Hungry" );
            }
            hunger_color = c_yellow;
        }
    }

    return std::make_pair( hunger_string, hunger_color );
}

std::pair<std::string, nc_color> player::get_pain_description() const
{
    auto pain = Creature::get_pain_description();
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

void player::enforce_minimum_healing()
{
    for( int i = 0; i < num_hp_parts; i++ ) {
        if( healed_total[i] <= 0 ) {
            heal( static_cast<hp_part>( i ), 1 );
        }
        healed_total[i] = 0;
    }
}
