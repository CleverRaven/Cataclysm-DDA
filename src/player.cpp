#include "player.h"

#include "action.h"
#include "coordinate_conversions.h"
#include "profession.h"
#include "bionics.h"
#include "mission.h"
#include "game.h"
#include "map.h"
#include "filesystem.h"
#include "fungal_effects.h"
#include "debug.h"
#include "addiction.h"
#include "inventory.h"
#include "options.h"
#include "weather.h"
#include "item.h"
#include "material.h"
#include "translations.h"
#include "name.h"
#include "cursesdef.h"
#include "catacharset.h"
#include "get_version.h"
#include "crafting.h"
#include "craft_command.h"
#include "requirements.h"
#include "monstergenerator.h"
#include "help.h" // get_hint
#include "martialarts.h"
#include "output.h"
#include "overmapbuffer.h"
#include "messages.h"
#include "sounds.h"
#include "item_action.h"
#include "mongroup.h"
#include "morale.h"
#include "morale_types.h"
#include "input.h"
#include "veh_type.h"
#include "overmap.h"
#include "vehicle.h"
#include "trap.h"
#include "text_snippets.h"
#include "mutation.h"
#include "ui.h"
#include "uistate.h"
#include "trap.h"
#include "map_iterator.h"
#include "submap.h"
#include "mtype.h"
#include "weather_gen.h"
#include "cata_utility.h"
#include "iuse_actor.h"
#include "catalua.h"
#include "npc.h"
#include "cata_utility.h"
#include "overlay_ordering.h"
#include "vitamin.h"
#include "fault.h"
#include "recipe_dictionary.h"
#include "ranged.h"

#include <map>
#include <iterator>

#ifdef TILES
#include "SDL.h"
#endif // TILES

//Used for e^(x) functions
#include <stdio.h>
#include <math.h>

#include <ctime>
#include <algorithm>
#include <numeric>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <limits>

const double MIN_RECOIL = 600;

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
const efftype_id effect_darkness( "darkness" );
const efftype_id effect_datura( "datura" );
const efftype_id effect_deaf( "deaf" );
const efftype_id effect_dermatik( "dermatik" );
const efftype_id effect_disabled( "disabled" );
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
const efftype_id effect_grabbed( "grabbed" );
const efftype_id effect_hallu( "hallu" );
const efftype_id effect_hot( "hot" );
const efftype_id effect_infected( "infected" );
const efftype_id effect_iodine( "iodine" );
const efftype_id effect_jetinjector( "jetinjector" );
const efftype_id effect_lack_sleep( "lack_sleep" );
const efftype_id effect_lying_down( "lying_down" );
const efftype_id effect_mending( "mending" );
const efftype_id effect_meth( "meth" );
const efftype_id effect_onfire( "onfire" );
const efftype_id effect_paincysts( "paincysts" );
const efftype_id effect_pkill1( "pkill1" );
const efftype_id effect_pkill2( "pkill2" );
const efftype_id effect_pkill3( "pkill3" );
const efftype_id effect_recover( "recover" );
const efftype_id effect_shakes( "shakes" );
const efftype_id effect_sleep( "sleep" );
const efftype_id effect_spores( "spores" );
const efftype_id effect_stunned( "stunned" );
const efftype_id effect_tapeworm( "tapeworm" );
const efftype_id effect_took_prozac( "took_prozac" );
const efftype_id effect_took_xanax( "took_xanax" );
const efftype_id effect_visuals( "visuals" );
const efftype_id effect_weed_high( "weed_high" );
const efftype_id effect_winded( "winded" );
const efftype_id effect_nausea( "nausea" );
const efftype_id effect_cough_suppress( "cough_suppress" );

const matype_id style_none( "style_none" );

static const trait_id trait_ACIDBLOOD( "ACIDBLOOD" );
static const trait_id trait_ACIDPROOF( "ACIDPROOF" );
static const trait_id trait_ADDICTIVE( "ADDICTIVE" );
static const trait_id trait_ADRENALINE( "ADRENALINE" );
static const trait_id trait_ALBINO( "ALBINO" );
static const trait_id trait_AMORPHOUS( "AMORPHOUS" );
static const trait_id trait_ANTENNAE( "ANTENNAE" );
static const trait_id trait_ANTIFRUIT( "ANTIFRUIT" );
static const trait_id trait_ANTIJUNK( "ANTIJUNK" );
static const trait_id trait_ANTIWHEAT( "ANTIWHEAT" );
static const trait_id trait_ANTLERS( "ANTLERS" );
static const trait_id trait_ARACHNID_ARMS( "ARACHNID_ARMS" );
static const trait_id trait_ARACHNID_ARMS_OK( "ARACHNID_ARMS_OK" );
static const trait_id trait_ARM_FEATHERS( "ARM_FEATHERS" );
static const trait_id trait_ARM_TENTACLES( "ARM_TENTACLES" );
static const trait_id trait_ARM_TENTACLES_4( "ARM_TENTACLES_4" );
static const trait_id trait_ARM_TENTACLES_8( "ARM_TENTACLES_8" );
static const trait_id trait_ASTHMA( "ASTHMA" );
static const trait_id trait_BADBACK( "BADBACK" );
static const trait_id trait_BADCARDIO( "BADCARDIO" );
static const trait_id trait_BADHEARING( "BADHEARING" );
static const trait_id trait_BADKNEES( "BADKNEES" );
static const trait_id trait_BADTEMPER( "BADTEMPER" );
static const trait_id trait_BARK( "BARK" );
static const trait_id trait_BEAK( "BEAK" );
static const trait_id trait_BEAK_HUM( "BEAK_HUM" );
static const trait_id trait_BEAK_PECK( "BEAK_PECK" );
static const trait_id trait_BEAUTIFUL( "BEAUTIFUL" );
static const trait_id trait_BEAUTIFUL2( "BEAUTIFUL2" );
static const trait_id trait_BEAUTIFUL3( "BEAUTIFUL3" );
static const trait_id trait_BIRD_EYE( "BIRD_EYE" );
static const trait_id trait_CANINE_EARS( "CANINE_EARS" );
static const trait_id trait_CANNIBAL( "CANNIBAL" );
static const trait_id trait_CARNIVORE( "CARNIVORE" );
static const trait_id trait_CENOBITE( "CENOBITE" );
static const trait_id trait_CEPH_EYES( "CEPH_EYES" );
static const trait_id trait_CF_HAIR( "CF_HAIR" );
static const trait_id trait_CHAOTIC( "CHAOTIC" );
static const trait_id trait_CHEMIMBALANCE( "CHEMIMBALANCE" );
static const trait_id trait_CHITIN( "CHITIN" );
static const trait_id trait_CHITIN2( "CHITIN2" );
static const trait_id trait_CHITIN3( "CHITIN3" );
static const trait_id trait_CHITIN_FUR( "CHITIN_FUR" );
static const trait_id trait_CHITIN_FUR2( "CHITIN_FUR2" );
static const trait_id trait_CHITIN_FUR3( "CHITIN_FUR3" );
static const trait_id trait_CHLOROMORPH( "CHLOROMORPH" );
static const trait_id trait_CLAWS( "CLAWS" );
static const trait_id trait_CLAWS_RAT( "CLAWS_RAT" );
static const trait_id trait_CLAWS_RETRACT( "CLAWS_RETRACT" );
static const trait_id trait_CLAWS_ST( "CLAWS_ST" );
static const trait_id trait_CLAWS_TENTACLE( "CLAWS_TENTACLE" );
static const trait_id trait_CLUMSY( "CLUMSY" );
static const trait_id trait_COLDBLOOD( "COLDBLOOD" );
static const trait_id trait_COLDBLOOD2( "COLDBLOOD2" );
static const trait_id trait_COLDBLOOD3( "COLDBLOOD3" );
static const trait_id trait_COLDBLOOD4( "COLDBLOOD4" );
static const trait_id trait_COMPOUND_EYES( "COMPOUND_EYES" );
static const trait_id trait_DEBUG_CLOAK( "DEBUG_CLOAK" );
static const trait_id trait_DEBUG_HS( "DEBUG_HS" );
static const trait_id trait_DEBUG_LS( "DEBUG_LS" );
static const trait_id trait_DEBUG_NODMG( "DEBUG_NODMG" );
static const trait_id trait_DEBUG_NOTEMP( "DEBUG_NOTEMP" );
static const trait_id trait_DEFORMED( "DEFORMED" );
static const trait_id trait_DEFORMED2( "DEFORMED2" );
static const trait_id trait_DEFORMED3( "DEFORMED3" );
static const trait_id trait_DISIMMUNE( "DISIMMUNE" );
static const trait_id trait_DISRESISTANT( "DISRESISTANT" );
static const trait_id trait_DOWN( "DOWN" );
static const trait_id trait_EAGLEEYED( "EAGLEEYED" );
static const trait_id trait_EASYSLEEPER( "EASYSLEEPER" );
static const trait_id trait_EATDEAD( "EATDEAD" );
static const trait_id trait_EATHEALTH( "EATHEALTH" );
static const trait_id trait_EATPOISON( "EATPOISON" );
static const trait_id trait_FASTHEALER( "FASTHEALER" );
static const trait_id trait_FASTHEALER2( "FASTHEALER2" );
static const trait_id trait_FASTLEARNER( "FASTLEARNER" );
static const trait_id trait_FASTREADER( "FASTREADER" );
static const trait_id trait_FAT( "FAT" );
static const trait_id trait_FEATHERS( "FEATHERS" );
static const trait_id trait_FELINE_EARS( "FELINE_EARS" );
static const trait_id trait_FELINE_FUR( "FELINE_FUR" );
static const trait_id trait_FLEET( "FLEET" );
static const trait_id trait_FLEET2( "FLEET2" );
static const trait_id trait_FLIMSY( "FLIMSY" );
static const trait_id trait_FLIMSY2( "FLIMSY2" );
static const trait_id trait_FLIMSY3( "FLIMSY3" );
static const trait_id trait_FLOWERS( "FLOWERS" );
static const trait_id trait_FORGETFUL( "FORGETFUL" );
static const trait_id trait_FUR( "FUR" );
static const trait_id trait_GILLS( "GILLS" );
static const trait_id trait_GILLS_CEPH( "GILLS_CEPH" );
static const trait_id trait_GIZZARD( "GIZZARD" );
static const trait_id trait_GOODCARDIO( "GOODCARDIO" );
static const trait_id trait_GOODHEARING( "GOODHEARING" );
static const trait_id trait_GOODMEMORY( "GOODMEMORY" );
static const trait_id trait_GOURMAND( "GOURMAND" );
static const trait_id trait_HEAVYSLEEPER( "HEAVYSLEEPER" );
static const trait_id trait_HEAVYSLEEPER2( "HEAVYSLEEPER2" );
static const trait_id trait_HERBIVORE( "HERBIVORE" );
static const trait_id trait_HIBERNATE( "HIBERNATE" );
static const trait_id trait_HOARDER( "HOARDER" );
static const trait_id trait_HOLLOW_BONES( "HOLLOW_BONES" );
static const trait_id trait_HOOVES( "HOOVES" );
static const trait_id trait_HORNS_CURLED( "HORNS_CURLED" );
static const trait_id trait_HORNS_POINTED( "HORNS_POINTED" );
static const trait_id trait_HUGE( "HUGE" );
static const trait_id trait_HUGE_OK( "HUGE_OK" );
static const trait_id trait_HUNGER( "HUNGER" );
static const trait_id trait_HUNGER2( "HUNGER2" );
static const trait_id trait_HUNGER3( "HUNGER3" );
static const trait_id trait_HYPEROPIC( "HYPEROPIC" );
static const trait_id trait_ILLITERATE( "ILLITERATE" );
static const trait_id trait_INFIMMUNE( "INFIMMUNE" );
static const trait_id trait_INFRESIST( "INFRESIST" );
static const trait_id trait_INSECT_ARMS( "INSECT_ARMS" );
static const trait_id trait_INSECT_ARMS_OK( "INSECT_ARMS_OK" );
static const trait_id trait_INSOMNIA( "INSOMNIA" );
static const trait_id trait_INT_SLIME( "INT_SLIME" );
static const trait_id trait_JITTERY( "JITTERY" );
static const trait_id trait_LACTOSE( "LACTOSE" );
static const trait_id trait_LARGE( "LARGE" );
static const trait_id trait_LARGE_OK( "LARGE_OK" );
static const trait_id trait_LEAVES( "LEAVES" );
static const trait_id trait_LEG_TENTACLES( "LEG_TENTACLES" );
static const trait_id trait_LEG_TENT_BRACE( "LEG_TENT_BRACE" );
static const trait_id trait_LIGHTEATER( "LIGHTEATER" );
static const trait_id trait_LIGHTFUR( "LIGHTFUR" );
static const trait_id trait_LIGHTSTEP( "LIGHTSTEP" );
static const trait_id trait_LIGHT_BONES( "LIGHT_BONES" );
static const trait_id trait_LUPINE_EARS( "LUPINE_EARS" );
static const trait_id trait_LUPINE_FUR( "LUPINE_FUR" );
static const trait_id trait_LYNX_FUR( "LYNX_FUR" );
static const trait_id trait_MANDIBLES( "MANDIBLES" );
static const trait_id trait_MASOCHIST( "MASOCHIST" );
static const trait_id trait_MASOCHIST_MED( "MASOCHIST_MED" );
static const trait_id trait_MEATARIAN( "MEATARIAN" );
static const trait_id trait_MEMBRANE( "MEMBRANE" );
static const trait_id trait_MET_RAT( "MET_RAT" );
static const trait_id trait_MINOTAUR( "MINOTAUR" );
static const trait_id trait_MOODSWINGS( "MOODSWINGS" );
static const trait_id trait_MOUTH_TENTACLES( "MOUTH_TENTACLES" );
static const trait_id trait_MUZZLE( "MUZZLE" );
static const trait_id trait_MUZZLE_BEAR( "MUZZLE_BEAR" );
static const trait_id trait_MUZZLE_LONG( "MUZZLE_LONG" );
static const trait_id trait_MUZZLE_RAT( "MUZZLE_RAT" );
static const trait_id trait_MYOPIC( "MYOPIC" );
static const trait_id trait_M_BLOSSOMS( "M_BLOSSOMS" );
static const trait_id trait_M_DEPENDENT( "M_DEPENDENT" );
static const trait_id trait_M_IMMUNE( "M_IMMUNE" );
static const trait_id trait_M_SKIN( "M_SKIN" );
static const trait_id trait_M_SKIN2( "M_SKIN2" );
static const trait_id trait_M_SPORES( "M_SPORES" );
static const trait_id trait_NAUSEA( "NAUSEA" );
static const trait_id trait_NONADDICTIVE( "NONADDICTIVE" );
static const trait_id trait_NOPAIN( "NOPAIN" );
static const trait_id trait_OPTIMISTIC( "OPTIMISTIC" );
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
static const trait_id trait_PLANTSKIN( "PLANTSKIN" );
static const trait_id trait_PONDEROUS1( "PONDEROUS1" );
static const trait_id trait_PONDEROUS2( "PONDEROUS2" );
static const trait_id trait_PONDEROUS3( "PONDEROUS3" );
static const trait_id trait_PRED2( "PRED2" );
static const trait_id trait_PRED3( "PRED3" );
static const trait_id trait_PRED4( "PRED4" );
static const trait_id trait_PRETTY( "PRETTY" );
static const trait_id trait_PROBOSCIS( "PROBOSCIS" );
static const trait_id trait_PSYCHOPATH( "PSYCHOPATH" );
static const trait_id trait_QUICK( "QUICK" );
static const trait_id trait_QUILLS( "QUILLS" );
static const trait_id trait_RADIOACTIVE1( "RADIOACTIVE1" );
static const trait_id trait_RADIOACTIVE2( "RADIOACTIVE2" );
static const trait_id trait_RADIOACTIVE3( "RADIOACTIVE3" );
static const trait_id trait_RADIOGENIC( "RADIOGENIC" );
static const trait_id trait_RAP_TALONS( "RAP_TALONS" );
static const trait_id trait_REGEN( "REGEN" );
static const trait_id trait_REGEN_LIZ( "REGEN_LIZ" );
static const trait_id trait_ROOTS2( "ROOTS2" );
static const trait_id trait_ROOTS3( "ROOTS3" );
static const trait_id trait_ROT2( "ROT2" );
static const trait_id trait_ROT3( "ROT3" );
static const trait_id trait_RUMINANT( "RUMINANT" );
static const trait_id trait_SABER_TEETH( "SABER_TEETH" );
static const trait_id trait_SAPIOVORE( "SAPIOVORE" );
static const trait_id trait_SAPROPHAGE( "SAPROPHAGE" );
static const trait_id trait_SAPROVORE( "SAPROVORE" );
static const trait_id trait_SAVANT( "SAVANT" );
static const trait_id trait_SCALES( "SCALES" );
static const trait_id trait_SCHIZOPHRENIC( "SCHIZOPHRENIC" );
static const trait_id trait_SELFAWARE( "SELFAWARE" );
static const trait_id trait_SHELL( "SHELL" );
static const trait_id trait_SHELL2( "SHELL2" );
static const trait_id trait_SHOUT1( "SHOUT1" );
static const trait_id trait_SHOUT2( "SHOUT2" );
static const trait_id trait_SHOUT3( "SHOUT3" );
static const trait_id trait_SLEEK_SCALES( "SLEEK_SCALES" );
static const trait_id trait_SLEEPY( "SLEEPY" );
static const trait_id trait_SLEEPY2( "SLEEPY2" );
static const trait_id trait_SLIMESPAWNER( "SLIMESPAWNER" );
static const trait_id trait_SLIMY( "SLIMY" );
static const trait_id trait_SLIT_NOSTRILS( "SLIT_NOSTRILS" );
static const trait_id trait_SLOWHEALER( "SLOWHEALER" );
static const trait_id trait_SLOWLEARNER( "SLOWLEARNER" );
static const trait_id trait_SLOWREADER( "SLOWREADER" );
static const trait_id trait_SLOWRUNNER( "SLOWRUNNER" );
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
static const trait_id trait_TALONS( "TALONS" );
static const trait_id trait_THICKSKIN( "THICKSKIN" );
static const trait_id trait_THICK_SCALES( "THICK_SCALES" );
static const trait_id trait_THINSKIN( "THINSKIN" );
static const trait_id trait_THIRST( "THIRST" );
static const trait_id trait_THIRST2( "THIRST2" );
static const trait_id trait_THIRST3( "THIRST3" );
static const trait_id trait_THORNS( "THORNS" );
static const trait_id trait_THRESH_CEPHALOPOD( "THRESH_CEPHALOPOD" );
static const trait_id trait_THRESH_FELINE( "THRESH_FELINE" );
static const trait_id trait_THRESH_INSECT( "THRESH_INSECT" );
static const trait_id trait_THRESH_MYCUS( "THRESH_MYCUS" );
static const trait_id trait_THRESH_PLANT( "THRESH_PLANT" );
static const trait_id trait_THRESH_SPIDER( "THRESH_SPIDER" );
static const trait_id trait_THRESH_URSINE( "THRESH_URSINE" );
static const trait_id trait_TOUGH_FEET( "TOUGH_FEET" );
static const trait_id trait_TROGLO( "TROGLO" );
static const trait_id trait_TROGLO2( "TROGLO2" );
static const trait_id trait_TROGLO3( "TROGLO3" );
static const trait_id trait_UGLY( "UGLY" );
static const trait_id trait_UNSTABLE( "UNSTABLE" );
static const trait_id trait_URSINE_EARS( "URSINE_EARS" );
static const trait_id trait_URSINE_EYE( "URSINE_EYE" );
static const trait_id trait_URSINE_FUR( "URSINE_FUR" );
static const trait_id trait_VEGETARIAN( "VEGETARIAN" );
static const trait_id trait_VISCOUS( "VISCOUS" );
static const trait_id trait_VOMITOUS( "VOMITOUS" );
static const trait_id trait_WAKEFUL( "WAKEFUL" );
static const trait_id trait_WAKEFUL2( "WAKEFUL2" );
static const trait_id trait_WAKEFUL3( "WAKEFUL3" );
static const trait_id trait_WEAKSCENT( "WEAKSCENT" );
static const trait_id trait_WEAKSTOMACH( "WEAKSTOMACH" );
static const trait_id trait_WEBBED( "WEBBED" );
static const trait_id trait_WEB_SPINNER( "WEB_SPINNER" );
static const trait_id trait_WEB_WALKER( "WEB_WALKER" );
static const trait_id trait_WEB_WEAVER( "WEB_WEAVER" );
static const trait_id trait_WHISKERS( "WHISKERS" );
static const trait_id trait_WHISKERS_RAT( "WHISKERS_RAT" );
static const trait_id trait_WINGS_BUTTERFLY( "WINGS_BUTTERFLY" );
static const trait_id trait_WINGS_INSECT( "WINGS_INSECT" );
static const trait_id trait_WOOLALLERGY( "WOOLALLERGY" );

stats player_stats;

static const itype_id OPTICAL_CLOAK_ITEM_ID( "optical_cloak" );

player_morale_ptr::player_morale_ptr( const player_morale_ptr &rhs ) :
    std::unique_ptr<player_morale>( rhs ? new player_morale( *rhs ) : nullptr )
{
}

player_morale_ptr::player_morale_ptr( player_morale_ptr &&rhs ) :
    std::unique_ptr<player_morale>( rhs ? rhs.release() : nullptr )
{
}

player_morale_ptr &player_morale_ptr::operator = ( const player_morale_ptr &rhs )
{
    if( this != &rhs ) {
        reset( rhs ? new player_morale( *rhs ) : nullptr );
    }
    return *this;
}

player_morale_ptr &player_morale_ptr::operator = ( player_morale_ptr &&rhs )
{
    if( this != &rhs ) {
        reset( rhs ? rhs.release() : nullptr );
    }
    return *this;
}

player_morale_ptr::~player_morale_ptr()
{
}

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

player::player() : Character()
{
    id = -1; // -1 is invalid
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
    power_level = 0;
    max_power_level = 0;
    stamina = get_stamina_max();
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
           NULL; //workaround for a potential structural limitation, see player::create

    start_location = start_location_id( "shelter" );
    moves = 100;
    movecounter = 0;
    cached_turn = -1;
    oxygen = 0;
    next_climate_control_check = 0;
    last_climate_control_ret = false;
    active_mission = nullptr;
    in_vehicle = false;
    controlling_vehicle = false;
    grab_point = {0, 0, 0};
    grab_type = OBJECT_NONE;
    move_mode = "walk";
    style_selected = matype_id( "style_none" );
    keep_hands_free = false;
    focus_pool = 100;
    last_item = itype_id( "null" );
    sight_max = 9999;
    last_batch = 0;
    lastconsumed = itype_id( "null" );
    next_expected_position = tripoint_min;

    empty_traits();

    for( int i = 0; i < num_bp; i++ ) {
        temp_cur[i] = BODYTEMP_NORM;
        frostbite_timer[i] = 0;
        temp_conv[i] = BODYTEMP_NORM;
        body_wetness[i] = 0;
    }
    nv_cached = false;
    volume = 0;

    for( const auto &v : vitamin::all() ) {
        vitamin_levels[ v.first ] = 0;
    }

    memorial_log.clear();
    player_stats.reset();

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

    morale.reset( new player_morale() );
    last_craft.reset( new craft_command() );
}

player::~player() = default;
player::player(const player &) = default;
player::player(player &&) = default;
player &player::operator=(const player &) = default;
player &player::operator=(player &&) = default;

void player::normalize()
{
    Character::normalize();

    style_selected = matype_id( "style_none" );

    recalc_hp();

    for( int i = 0 ; i < num_bp; i++ ) {
        temp_conv[i] = BODYTEMP_NORM;
    }
    stamina = get_stamina_max();
}

std::string player::disp_name( bool possessive ) const
{
    if( !possessive ) {
        if( is_player() ) {
            return _( "you" );
        }
        return name;
    } else {
        if( is_player() ) {
            return _( "your" );
        }
        return string_format( _( "%s's" ), name.c_str() );
    }
}

std::string player::skin_name() const
{
    //TODO: Return actual deflecting layer name
    return _( "armor" );
}

void player::reset_stats()
{
    clear_miss_reasons();

    // Trait / mutation buffs
    if( has_trait( trait_THICK_SCALES ) ) {
        add_miss_reason( _( "Your thick scales get in the way." ), 2 );
    }
    if( has_trait( trait_CHITIN2 ) || has_trait( trait_CHITIN3 ) || has_trait( trait_CHITIN_FUR3 ) ) {
        add_miss_reason( _( "Your chitin gets in the way." ), 1 );
    }
    if( has_trait( trait_COMPOUND_EYES ) && !wearing_something_on( bp_eyes ) ) {
        mod_per_bonus( 1 );
    }
    if( has_trait( trait_INSECT_ARMS ) ) {
        add_miss_reason( _( "Your insect limbs get in the way." ), 2 );
    }
    if( has_trait( trait_INSECT_ARMS_OK ) ) {
        if( !wearing_something_on( bp_torso ) ) {
            mod_dex_bonus( 1 );
        } else {
            mod_dex_bonus( -1 );
            add_miss_reason( _( "Your clothing restricts your insect arms." ), 1 );
        }
    }
    if( has_trait( trait_WEBBED ) ) {
        add_miss_reason( _( "Your webbed hands get in the way." ), 1 );
    }
    if( has_trait( trait_ARACHNID_ARMS ) ) {
        add_miss_reason( _( "Your arachnid limbs get in the way." ), 4 );
    }
    if( has_trait( trait_ARACHNID_ARMS_OK ) ) {
        if( !wearing_something_on( bp_torso ) ) {
            mod_dex_bonus( 2 );
        } else if( !exclusive_flag_coverage( "OVERSIZE" )[bp_torso] ) {
            mod_dex_bonus( -2 );
            add_miss_reason( _( "Your clothing constricts your arachnid limbs." ), 2 );
        }
    }

    // Pain
    if( get_perceived_pain() > 0 ) {
        const auto ppen = get_pain_penalty();
        mod_str_bonus( -ppen.strength );
        mod_dex_bonus( -ppen.dexterity );
        mod_int_bonus( -ppen.intelligence );
        mod_per_bonus( -ppen.perception );
        if( ppen.dexterity > 0 ) {
            add_miss_reason( _( "Your pain distracts you!" ), unsigned( ppen.dexterity ) );
        }
    }
    // Morale
    if( abs( get_morale_level() ) >= 100 ) {
        mod_str_bonus( get_morale_level() / 180 );
        int dex_mod = get_morale_level() / 200;
        mod_dex_bonus( dex_mod );
        if( dex_mod < 0 ) {
            add_miss_reason( _( "What's the point of fighting?" ), unsigned( -dex_mod ) );
        }
        mod_per_bonus( get_morale_level() / 125 );
        mod_int_bonus( get_morale_level() / 100 );
    }
    // Radiation
    if( radiation > 0 ) {
        mod_str_bonus( -radiation / 80 );
        int dex_mod = -radiation / 110;
        mod_dex_bonus( dex_mod );
        if( dex_mod < 0 ) {
            add_miss_reason( _( "Radiation weakens you." ), unsigned( -dex_mod ) );
        }
        mod_per_bonus( -radiation / 100 );
        mod_int_bonus( -radiation / 120 );
    }
    // Stimulants
    mod_dex_bonus( stim / 10 );
    mod_per_bonus( stim /  7 );
    mod_int_bonus( stim /  6 );
    if( stim >= 30 ) {
        int dex_mod = -1 * abs( stim - 15 ) / 8;
        mod_dex_bonus( dex_mod );
        add_miss_reason( _( "You shake with the excess stimulation." ), unsigned( -dex_mod ) );
        mod_per_bonus( -1 * abs( stim - 15 ) / 12 );
        mod_int_bonus( -1 * abs( stim - 15 ) / 14 );
    } else if( stim < 0 ) {
        add_miss_reason( _( "You feel woozy." ), unsigned( -stim / 10 ) );
    }
    // Hunger
    if( get_hunger() >= 500 ) {
        // We die at 6000
        const int dex_mod = -get_hunger() / 1000;
        add_miss_reason( _( "You're weak from hunger." ), unsigned( -dex_mod ) );
        mod_str_bonus( -get_hunger() / 500 );
        mod_dex_bonus( dex_mod );
        mod_int_bonus( -get_hunger() / 1000 );
    }
    // Thirst
    if( get_thirst() >= 200 ) {
        // We die at 1200
        const int dex_mod = -get_thirst() / 200;
        add_miss_reason( _( "You're weak from thirst." ), unsigned( -dex_mod ) );
        mod_str_bonus( -get_thirst() / 200 );
        mod_dex_bonus( dex_mod );
        mod_int_bonus( -get_thirst() / 200 );
        mod_per_bonus( -get_thirst() / 200 );
    }

    // Dodge-related effects
    mod_dodge_bonus( mabuff_dodge_bonus() -
                     ( encumb( bp_leg_l ) + encumb( bp_leg_r ) ) / 20.0f -
                     ( encumb( bp_torso ) / 10.0f ) );
    // Whiskers don't work so well if they're covered
    if( has_trait( trait_WHISKERS ) && !wearing_something_on( bp_mouth ) ) {
        mod_dodge_bonus( 1 );
    }
    if( has_trait( trait_WHISKERS_RAT ) && !wearing_something_on( bp_mouth ) ) {
        mod_dodge_bonus( 2 );
    }
    // Spider hair is basically a full-body set of whiskers, once you get the brain for it
    if( has_trait( trait_CHITIN_FUR3 ) ) {
        static const std::array<body_part, 5> parts {{bp_head, bp_arm_r, bp_arm_l, bp_leg_r, bp_leg_l}};
        for( auto bp : parts ) {
            if( !wearing_something_on( bp ) ) {
                mod_dodge_bonus( +1 );
            }
        }
        // Torso handled separately, bigger bonus
        if( !wearing_something_on( bp_torso ) ) {
            mod_dodge_bonus( 4 );
        }
    }

    // Hit-related effects
    mod_hit_bonus( mabuff_tohit_bonus() + weapon.type->m_to_hit );

    // Apply static martial arts buffs
    ma_static_effects();

    if( calendar::once_every( MINUTES( 1 ) ) ) {
        update_mental_focus();
    }

    recalc_sight_limits();
    recalc_speed_bonus();

    // Effects
    for( auto maps : effects ) {
        for( auto i : maps.second ) {
            const auto &it = i.second;
            bool reduced = resists_effect( it );
            mod_str_bonus( it.get_mod( "STR", reduced ) );
            mod_dex_bonus( it.get_mod( "DEX", reduced ) );
            mod_per_bonus( it.get_mod( "PER", reduced ) );
            mod_int_bonus( it.get_mod( "INT", reduced ) );
        }
    }

    Character::reset_stats();
}

void player::process_turn()
{
    Character::process_turn();

    // Didn't just pick something up
    last_item = itype_id( "null" );

    if( has_active_bionic( "bio_metabolics" ) && power_level + 25 <= max_power_level &&
        get_hunger() < 100 && calendar::once_every( 5 ) ) {
        mod_hunger( 2 );
        charge_power( 25 );
    }

    visit_items( [this]( item * e ) {
        e->process_artifact( this, pos() );
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

    // We can dodge again! Assuming we can actually move...
    if( !in_sleep_state() ) {
        blocks_left = get_num_blocks();
        dodges_left = get_num_dodges();
    } else {
        blocks_left = 0;
        dodges_left = 0;
    }

    // auto-learning. This is here because skill-increases happens all over the place:
    // SkillLevel::readBook (has no connection to the skill or the player),
    // player::read, player::practice, ...
    /** @EFFECT_UNARMED >1 allows spontaneous discovery of brawling martial art style */
    if( get_skill_level( skill_unarmed ) >= 2 ) {
        const matype_id brawling( "style_brawling" );
        if( !has_martialart( brawling ) ) {
            add_martialart( brawling );
            add_msg_if_player( m_info, _( "You learned a new style." ) );
        }
    }
}

void player::action_taken()
{
    nv_cached = false;
}

void player::update_morale()
{
    morale->decay( 1 );
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
            add_morale( MORALE_PERM_HOARDER, -pen, -pen, 5, 5, true );
        }
    }
}

void player::update_mental_focus()
{
    int focus_gain_rate = calc_focus_equilibrium() - focus_pool;

    // handle negative gain rates in a symmetric manner
    int base_change = 1;
    if( focus_gain_rate < 0 ) {
        base_change = -1;
        focus_gain_rate = -focus_gain_rate;
    }

    // for every 100 points, we have a flat gain of 1 focus.
    // for every n points left over, we have an n% chance of 1 focus
    int gain = focus_gain_rate / 100;
    if( rng( 1, 100 ) <= ( focus_gain_rate % 100 ) ) {
        gain++;
    }

    focus_pool += ( gain * base_change );

    // Fatigue should at least prevent high focus
    // This caps focus gain at 60(arbitrary value) if you're Dead Tired
    if( get_fatigue() >= DEAD_TIRED && focus_pool > 60 ) {
        focus_pool = 60;
    }

    // Moved from calc_focus_equilibrium, because it is now const
    if( activity.id() == activity_id( "ACT_READ" ) ) {
        const item *book = activity.targets[0].get_item();
        if( get_item_position( book ) == INT_MIN || !book->is_book() ) {
            add_msg_if_player( m_bad, _( "You lost your book! You stop reading." ) );
            activity.set_to_null();
        }
    }
}

// written mostly by FunnyMan3595 in Github issue #613 (DarklingWolf's repo),
// with some small edits/corrections by Soron
int player::calc_focus_equilibrium() const
{
    int focus_gain_rate = 100;

    if( activity.id() == activity_id( "ACT_READ" ) ) {
        const item &book = *activity.targets[0].get_item();
        if( book.is_book() && get_item_position( &book ) != INT_MIN ) {
            auto &bt = *book.type->book;
            // apply a penalty when we're actually learning something
            const auto &skill_level = get_skill_level( bt.skill );
            if( skill_level.can_train() && skill_level < bt.level ) {
                focus_gain_rate -= 50;
            }
        }
    }

    int eff_morale = get_morale_level();
    // Factor in perceived pain, since it's harder to rest your mind while your body hurts.
    // Cenobites don't mind, though
    if( !has_trait( trait_CENOBITE ) ) {
        eff_morale = eff_morale - get_perceived_pain();
    }

    if( eff_morale < -99 ) {
        // At very low morale, focus goes up at 1% of the normal rate.
        focus_gain_rate = 1;
    } else if( eff_morale <= 50 ) {
        // At -99 to +50 morale, each point of morale gives 1% of the normal rate.
        focus_gain_rate += eff_morale;
    } else {
        /* Above 50 morale, we apply strong diminishing returns.
         * Each block of 50% takes twice as many morale points as the previous one:
         * 150% focus gain at 50 morale (as before)
         * 200% focus gain at 150 morale (100 more morale)
         * 250% focus gain at 350 morale (200 more morale)
         * ...
         * Cap out at 400% focus gain with 3,150+ morale, mostly as a sanity check.
         */

        int block_multiplier = 1;
        int morale_left = eff_morale;
        while( focus_gain_rate < 400 ) {
            if( morale_left > 50 * block_multiplier ) {
                // We can afford the entire block.  Get it and continue.
                morale_left -= 50 * block_multiplier;
                focus_gain_rate += 50;
                block_multiplier *= 2;
            } else {
                // We can't afford the entire block.  Each block_multiplier morale
                // points give 1% focus gain, and then we're done.
                focus_gain_rate += morale_left / block_multiplier;
                break;
            }
        }
    }

    // This should be redundant, but just in case...
    if( focus_gain_rate < 1 ) {
        focus_gain_rate = 1;
    } else if( focus_gain_rate > 400 ) {
        focus_gain_rate = 400;
    }

    return focus_gain_rate;
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
        for( int i = 0 ; i < num_bp ; i++ ) {
            temp_cur[i] = BODYTEMP_NORM;
            temp_conv[i] = BODYTEMP_NORM;
        }
        return;
    }
    // NOTE : visit weather.h for some details on the numbers used
    // Converts temperature to Celsius/10
    int Ctemperature = int( 100 * temp_to_celsius( g->get_temperature() ) );
    w_point const weather = *g->weather_precise;
    int vpart = -1;
    vehicle *veh = g->m.veh_at( pos(), vpart );
    int vehwindspeed = 0;
    if( veh != nullptr ) {
        vehwindspeed = abs( veh->velocity / 100 ); // vehicle velocity in mph
    }
    const oter_id &cur_om_ter = overmap_buffer.ter( global_omt_location() );
    bool sheltered = g->is_sheltered( pos() );
    int total_windpower = get_local_windpower( weather.windpower + vehwindspeed, cur_om_ter->get_name(), sheltered );

    // Let's cache this not to check it num_bp times
    const bool has_bark = has_trait( trait_BARK );
    const bool has_sleep = has_effect( effect_sleep );
    const bool has_sleep_state = has_sleep || in_sleep_state();
    const bool has_heatsink = has_bionic( "bio_heatsink" ) || is_wearing( "rm13_armor_on" );
    const bool has_common_cold = has_effect( effect_common_cold );
    const bool has_climate_control = in_climate_control();
    const bool use_floor_warmth = can_use_floor_warmth();
    const furn_id furn_at_pos = g->m.furn( pos() );
    // Temperature norms
    // Ambient normal temperature is lower while asleep
    const int ambient_norm = has_sleep ? 3100 : 1900;

    /**
     * Calculations that affect all body parts equally go here, not in the loop
     */
    // Hunger
    // -1000 when about to starve to death
    // -1333 when starving with light eater
    // -2000 if you managed to get 0 metabolism rate somehow
    const float met_rate = metabolic_rate();
    const int hunger_warmth = int( 2000 * std::min( met_rate, 1.0f ) - 2000 );
    // Give SOME bonus to those living furnaces with extreme metabolism
    const int metabolism_warmth = int( std::max( 0.0f, met_rate - 1.0f ) * 1000 );
    // Fatigue
    // ~-900 when exhausted
    const int fatigue_warmth = has_sleep ? 0 : int( std::min( 0.0f, -1.5f * get_fatigue() ) );

    // Sunlight
    const int sunlight_warmth = g->is_in_sunlight( pos() ) ? 0 :
                                ( g->weather == WEATHER_SUNNY ? 1000 : 500 );
    // Fire at our tile
    const int fire_warmth = bodytemp_modifier_fire();

    // Cache fires to avoid scanning the map around us bp times
    // Stored as intensity-distance pairs
    std::vector<std::pair<int, int>> fires;
    fires.reserve( 13 * 13 );
    int best_fire = 0;
    for( int j = -6 ; j <= 6 ; j++ ) {
        for( int k = -6 ; k <= 6 ; k++ ) {
            tripoint dest( posx() + j, posy() + k, posz() );
            int heat_intensity = 0;

            int ffire = g->m.get_field_strength( dest, fd_fire );
            if( ffire > 0 ) {
                heat_intensity = ffire;
            } else if( g->m.tr_at( dest ).loadid == tr_lava ) {
                heat_intensity = 3;
            }
            if( heat_intensity == 0 || !g->m.sees( pos(), dest, -1 ) ) {
                // No heat source here
                continue;
            }
            // Ensure fire_dist >= 1 to avoid divide-by-zero errors.
            const int fire_dist = std::max( 1, std::max( std::abs( j ), std::abs( k ) ) );
            fires.push_back( std::make_pair( heat_intensity, fire_dist ) );
            if( fire_dist <= 1 ) {
                // Extend limbs/lean over a single adjacent fire to warm up
                best_fire = std::max( best_fire, heat_intensity );
            }
        }
    }

    const int lying_warmth = use_floor_warmth ? floor_warmth( pos() ) : 0;
    const int water_temperature = 100 * temp_to_celsius( g->get_cur_weather_gen().get_water_temperature() );

    // Current temperature and converging temperature calculations
    for( int i = 0 ; i < num_bp; i++ ) {
        // Skip eyes
        if( i == bp_eyes ) {
            continue;
        }

        // This adjusts the temperature scale to match the bodytemp scale,
        // it needs to be reset every iteration
        int adjusted_temp = ( Ctemperature - ambient_norm );
        int bp_windpower = total_windpower;
        // Represents the fact that the body generates heat when it is cold.
        // TODO : should this increase hunger?
        double scaled_temperature = logarithmic_range( BODYTEMP_VERY_COLD, BODYTEMP_VERY_HOT,
                                    temp_cur[i] );
        // Produces a smooth curve between 30.0 and 60.0.
        double homeostasis_adjustement = 30.0 * ( 1.0 + scaled_temperature );
        int clothing_warmth_adjustement = int( homeostasis_adjustement * warmth( body_part( i ) ) );
        int clothing_warmth_adjusted_bonus = int( homeostasis_adjustement * bonus_item_warmth(
                body_part( i ) ) );
        // WINDCHILL

        bp_windpower = int( ( float )bp_windpower * ( 1 - get_wind_resistance( body_part( i ) ) / 100.0 ) );
        // Calculate windchill
        int windchill = get_local_windchill( g->get_temperature(),
                                             get_local_humidity( weather.humidity, g->weather,
                                                     sheltered ),
                                             bp_windpower );
        // If you're standing in water, air temperature is replaced by water temperature. No wind.
        const ter_id ter_at_pos = g->m.ter( pos() );
        // Convert to 0.01C
        if( ( ter_at_pos == t_water_dp || ter_at_pos == t_water_pool || ter_at_pos == t_swater_dp ) ||
            ( ( ter_at_pos == t_water_sh || ter_at_pos == t_swater_sh || ter_at_pos == t_sewage ) &&
              ( i == bp_foot_l || i == bp_foot_r || i == bp_leg_l || i == bp_leg_r ) ) ) {
            adjusted_temp += water_temperature - Ctemperature; // Swap out air temp for water temp.
            windchill = 0;
        }

        // Convergent temperature is affected by ambient temperature,
        // clothing warmth, and body wetness.
        temp_conv[i] = BODYTEMP_NORM + adjusted_temp + windchill * 100 + clothing_warmth_adjustement;
        // HUNGER
        temp_conv[i] += hunger_warmth;
        // FATIGUE
        temp_conv[i] += fatigue_warmth;
        // CONVECTION HEAT SOURCES (generates body heat, helps fight frostbite)
        // Bark : lowers blister count to -10; harder to get blisters
        int blister_count = ( has_bark ? -10 : 0 ); // If the counter is high, your skin starts to burn
        for( const auto &intensity_dist : fires ) {
            const int intensity = intensity_dist.first;
            const int distance = intensity_dist.second;
            if( frostbite_timer[i] > 0 ) {
                frostbite_timer[i] -= std::max( 0, intensity - distance / 2 );
            }
            const int heat_here = intensity * intensity / distance;
            temp_conv[i] += 300 * heat_here;
            blister_count += heat_here;
        }
        // Bionic "Thermal Dissipation" says it prevents fire damage up to 2000F.
        // But it's kinda hard to get the balance right, let's go with 20 blisters
        if( has_heatsink ) {
            blister_count -= 20;
        }
        // BLISTERS : Skin gets blisters from intense heat exposure.
        if( blister_count - get_env_resist( body_part( i ) ) > 10 ) {
            add_effect( effect_blisters, 1, ( body_part )i );
        }

        temp_conv[i] += fire_warmth;
        temp_conv[i] += sunlight_warmth;
        // DISEASES
        if( i == bp_head && has_effect( effect_flu ) ) {
            temp_conv[i] += 1500;
        }
        if( has_common_cold ) {
            temp_conv[i] -= 750;
        }
        // Loss of blood results in loss of body heat, 1% bodyheat lost per 2% hp lost
        temp_conv[i] -= blood_loss( body_part( i ) ) * temp_conv[i] / 200;

        // EQUALIZATION
        switch( i ) {
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

        // Correction of body temperature due to traits and mutations
        temp_conv[i] += bodytemp_modifier_traits( temp_cur[i] > BODYTEMP_NORM );

        // Climate Control eases the effects of high and low ambient temps
        if( has_climate_control ) {
            temp_conv[i] = temp_corrected_by_climate_control( temp_conv[i] );
        }

        // FINAL CALCULATION : Increments current body temperature towards convergent.
        int bonus_fire_warmth = 0;
        if( !has_sleep_state && best_fire > 0 ) {
            // Warming up over a fire
            // Extremities are easier to extend over a fire
            switch( i ) {
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
                    if( furn_at_pos == f_armchair || furn_at_pos == f_chair || furn_at_pos == f_bench ) {
                        // Can sit on something to lift feet up to the fire
                        bonus_fire_warmth = best_fire * 1000;
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
        const int bonus_warmth = comfortable_warmth + metabolism_warmth;
        if( bonus_warmth > 0 ) {
            // Approximate temp_conv needed to reach comfortable temperature in this very turn
            // Basically inverted formula for temp_cur below
            int desired = 501 * BODYTEMP_NORM - 499 * temp_cur[i];
            if( std::abs( BODYTEMP_NORM - desired ) < 1000 ) {
                desired = BODYTEMP_NORM; // Ensure that it converges
            } else if( desired > BODYTEMP_HOT ) {
                desired = BODYTEMP_HOT; // Cap excess at sane temperature
            }

            if( desired < temp_conv[i] ) {
                // Too hot, can't help here
            } else if( desired < temp_conv[i] + bonus_warmth ) {
                // Use some heat, but not all of it
                temp_conv[i] = desired;
            } else {
                // Use all the heat
                temp_conv[i] += bonus_warmth;
            }

            // Morale bonus for comfiness - only if actually comfy (not too warm/cold)
            // Spread the morale bonus in time.
            if( comfortable_warmth > 0 &&
                calendar::turn % MINUTES( 1 ) == ( MINUTES( i ) / MINUTES( num_bp ) ) &&
                get_effect_int( effect_cold, num_bp ) == 0 &&
                get_effect_int( effect_hot, num_bp ) == 0 &&
                temp_cur[i] > BODYTEMP_COLD && temp_cur[i] <= BODYTEMP_NORM ) {
                add_morale( MORALE_COMFY, 1, 10, 20, 10, true );
            }
        }

        int temp_before = temp_cur[i];
        int temp_difference = temp_before - temp_conv[i]; // Negative if the player is warming up.
        // exp(-0.001) : half life of 60 minutes, exp(-0.002) : half life of 30 minutes,
        // exp(-0.003) : half life of 20 minutes, exp(-0.004) : half life of 15 minutes
        int rounding_error = 0;
        // If temp_diff is small, the player cannot warm up due to rounding errors. This fixes that.
        if( temp_difference < 0 && temp_difference > -600 ) {
            rounding_error = 1;
        }
        if( temp_cur[i] != temp_conv[i] ) {
            temp_cur[i] = int( temp_difference * exp( -0.002 ) + temp_conv[i] + rounding_error );
        }
        // This statement checks if we should be wearing our bonus warmth.
        // If, after all the warmth calculations, we should be, then we have to recalculate the temperature.
        if( clothing_warmth_adjusted_bonus != 0 &&
            ( ( temp_conv[i] + clothing_warmth_adjusted_bonus ) < BODYTEMP_HOT ||
              temp_cur[i] < BODYTEMP_COLD ) ) {
            temp_conv[i] += clothing_warmth_adjusted_bonus;
            rounding_error = 0;
            if( temp_difference < 0 && temp_difference > -600 ) {
                rounding_error = 1;
            }
            if( temp_before != temp_conv[i] ) {
                temp_difference = temp_before - temp_conv[i];
                temp_cur[i] = int( temp_difference * exp( -0.002 ) + temp_conv[i] + rounding_error );
            }
        }
        int temp_after = temp_cur[i];
        // PENALTIES
        if( temp_cur[i] < BODYTEMP_FREEZING ) {
            add_effect( effect_cold, 1, ( body_part )i, true, 3 );
        } else if( temp_cur[i] < BODYTEMP_VERY_COLD ) {
            add_effect( effect_cold, 1, ( body_part )i, true, 2 );
        } else if( temp_cur[i] < BODYTEMP_COLD ) {
            add_effect( effect_cold, 1, ( body_part )i, true, 1 );
        } else if( temp_cur[i] > BODYTEMP_SCORCHING ) {
            add_effect( effect_hot, 1, ( body_part )i, true, 3 );
        } else if( temp_cur[i] > BODYTEMP_VERY_HOT ) {
            add_effect( effect_hot, 1, ( body_part )i, true, 2 );
        } else if( temp_cur[i] > BODYTEMP_HOT ) {
            add_effect( effect_hot, 1, ( body_part )i, true, 1 );
        } else {
            if( temp_cur[i] >= BODYTEMP_COLD ) {
                remove_effect( effect_cold, ( body_part )i );
            }
            if( temp_cur[i] <= BODYTEMP_HOT ) {
                remove_effect( effect_hot, ( body_part )i );
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

        if( i == bp_mouth || i == bp_foot_r || i == bp_foot_l || i == bp_hand_r || i == bp_hand_l ) {
            // Handle the frostbite timer
            // Need temps in F, windPower already in mph
            int wetness_percentage = 100 * body_wetness[i] / drench_capacity[i]; // 0 - 100
            // Warmth gives a slight buff to temperature resistance
            // Wetness gives a heavy nerf to temperature resistance
            int Ftemperature = int( g->get_temperature() +
                                    warmth( ( body_part )i ) * 0.2 - 20 * wetness_percentage / 100 );
            // Windchill reduced by your armor
            int FBwindPower = int( total_windpower * ( 1 - get_wind_resistance( body_part( i ) ) / 100.0 ) );

            int intense = get_effect_int( effect_frostbite, ( body_part )i );

            // This has been broken down into 8 zones
            // Low risk zones (stops at frostnip)
            if( temp_cur[i] < BODYTEMP_COLD &&
                ( ( Ftemperature < 30 && Ftemperature >= 10 ) ||
                  ( Ftemperature < 10 && Ftemperature >= -5 &&
                    FBwindPower < 20 && -4 * Ftemperature + 3 * FBwindPower - 20 >= 0 ) ) ) {
                if( frostbite_timer[i] < 2000 ) {
                    frostbite_timer[i] += 3;
                }
                if( one_in( 100 ) && !has_effect( effect_frostbite, ( body_part )i ) ) {
                    add_msg( m_warning, _( "Your %s will be frostnipped in the next few hours." ),
                             body_part_name( body_part( i ) ).c_str() );
                }
                // Medium risk zones
            } else if( temp_cur[i] < BODYTEMP_COLD &&
                       ( ( Ftemperature < 10 && Ftemperature >= -5 && FBwindPower < 20 &&
                           -4 * Ftemperature + 3 * FBwindPower - 20 < 0 ) ||
                         ( Ftemperature < 10 && Ftemperature >= -5 && FBwindPower >= 20 ) ||
                         ( Ftemperature < -5 && FBwindPower < 10 ) ||
                         ( Ftemperature < -5 && FBwindPower >= 10 &&
                           -4 * Ftemperature + 3 * FBwindPower - 170 >= 0 ) ) ) {
                frostbite_timer[i] += 8;
                if( one_in( 100 ) && intense < 2 ) {
                    add_msg( m_warning, _( "Your %s will be frostbitten within the hour!" ),
                             body_part_name( body_part( i ) ).c_str() );
                }
                // High risk zones
            } else if( temp_cur[i] < BODYTEMP_COLD &&
                       ( ( Ftemperature < -5 && FBwindPower >= 10 &&
                           -4 * Ftemperature + 3 * FBwindPower - 170 < 0 ) ||
                         ( Ftemperature < -35 && FBwindPower >= 10 ) ) ) {
                frostbite_timer[i] += 72;
                if( one_in( 100 ) && intense < 2 ) {
                    add_msg( m_warning, _( "Your %s will be frostbitten any minute now!!" ),
                             body_part_name( body_part( i ) ).c_str() );
                }
                // Risk free, so reduce frostbite timer
            } else {
                frostbite_timer[i] -= 3;
            }

            // Handle the bestowing of frostbite
            if( frostbite_timer[i] < 0 ) {
                frostbite_timer[i] = 0;
            } else if( frostbite_timer[i] > 4200 ) {
                // This ensures that the player will recover in at most 3 hours.
                frostbite_timer[i] = 4200;
            }
            // Frostbite, no recovery possible
            if( frostbite_timer[i] >= 3600 ) {
                add_effect( effect_frostbite, 1, ( body_part )i, true, 2 );
                remove_effect( effect_frostbite_recovery, ( body_part )i );
                // Else frostnip, add recovery if we were frostbitten
            } else if( frostbite_timer[i] >= 1800 ) {
                if( intense == 2 ) {
                    add_effect( effect_frostbite_recovery, 1, ( body_part )i, true );
                }
                add_effect( effect_frostbite, 1, ( body_part )i, true, 1 );
                // Else fully recovered
            } else if( frostbite_timer[i] == 0 ) {
                remove_effect( effect_frostbite, ( body_part )i );
                remove_effect( effect_frostbite_recovery, ( body_part )i );
            }
        }
        // Warn the player if condition worsens
        if( temp_before > BODYTEMP_FREEZING && temp_after < BODYTEMP_FREEZING ) {
            //~ %s is bodypart
            add_msg( m_warning, _( "You feel your %s beginning to go numb from the cold!" ),
                     body_part_name( body_part( i ) ).c_str() );
        } else if( temp_before > BODYTEMP_VERY_COLD && temp_after < BODYTEMP_VERY_COLD ) {
            //~ %s is bodypart
            add_msg( m_warning, _( "You feel your %s getting very cold." ),
                     body_part_name( body_part( i ) ).c_str() );
        } else if( temp_before > BODYTEMP_COLD && temp_after < BODYTEMP_COLD ) {
            //~ %s is bodypart
            add_msg( m_warning, _( "You feel your %s getting chilly." ),
                     body_part_name( body_part( i ) ).c_str() );
        } else if( temp_before < BODYTEMP_SCORCHING && temp_after > BODYTEMP_SCORCHING ) {
            //~ %s is bodypart
            add_msg( m_bad, _( "You feel your %s getting red hot from the heat!" ),
                     body_part_name( body_part( i ) ).c_str() );
        } else if( temp_before < BODYTEMP_VERY_HOT && temp_after > BODYTEMP_VERY_HOT ) {
            //~ %s is bodypart
            add_msg( m_warning, _( "You feel your %s getting very hot." ),
                     body_part_name( body_part( i ) ).c_str() );
        } else if( temp_before < BODYTEMP_HOT && temp_after > BODYTEMP_HOT ) {
            //~ %s is bodypart
            add_msg( m_warning, _( "You feel your %s getting warm." ),
                     body_part_name( body_part( i ) ).c_str() );
        }

        // Warn the player that wind is going to be a problem.
        // But only if it can be a problem, no need to spam player with "wind chills your scorching body"
        if( temp_conv[i] <= BODYTEMP_COLD && windchill < -10 && one_in( 200 ) ) {
            add_msg( m_bad, _( "The wind is making your %s feel quite cold." ),
                     body_part_name( body_part( i ) ).c_str() );
        } else if( temp_conv[i] <= BODYTEMP_COLD && windchill < -20 && one_in( 100 ) ) {
            add_msg( m_bad,
                     _( "The wind is very strong, you should find some more wind-resistant clothing for your %s." ),
                     body_part_name( body_part( i ) ).c_str() );
        } else if( temp_conv[i] <= BODYTEMP_COLD && windchill < -30 && one_in( 50 ) ) {
            add_msg( m_bad, _( "Your clothing is not providing enough protection from the wind for your %s!" ),
                     body_part_name( body_part( i ) ).c_str() );
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


    int vpart = -1;
    vehicle *veh = g->m.veh_at( pos, vpart );
    bool veh_bed = ( veh != nullptr && veh->part_with_feature( vpart, "BED" ) >= 0 );
    bool veh_seat = ( veh != nullptr && veh->part_with_feature( vpart, "SEAT" ) >= 0 );

    // Search the floor for bedding
    if( furn_at_pos == f_bed ) {
        floor_bedding_warmth += 1000;
    } else if( furn_at_pos == f_makeshift_bed || furn_at_pos == f_armchair ||
               furn_at_pos == f_sofa ) {
        floor_bedding_warmth += 500;
    } else if( veh_bed && veh_seat ) {
        // BED+SEAT is intentionally worse than just BED
        floor_bedding_warmth += 250;
    } else if( veh_bed ) {
        floor_bedding_warmth += 300;
    } else if( veh_seat ) {
        floor_bedding_warmth += 200;
    } else if( furn_at_pos == f_straw_bed ) {
        floor_bedding_warmth += 200;
    } else if( trap_at_pos.loadid == tr_fur_rollmat || furn_at_pos == f_hay ) {
        floor_bedding_warmth += 0;
    } else if( trap_at_pos.loadid == tr_cot || ter_at_pos == t_improvised_shelter ||
               furn_at_pos == f_tatami ) {
        floor_bedding_warmth -= 500;
    } else if( trap_at_pos.loadid == tr_rollmat ) {
        floor_bedding_warmth -= 1000;
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

    // If the PC has fur, etc, that'll apply too
    int floor_mut_warmth = bodytemp_modifier_traits_floor();
    // DOWN doesn't provide floor insulation, though.
    // Better-than-light fur or being in one's shell does.
    if( ( !( has_trait( trait_DOWN ) ) ) && ( floor_mut_warmth >= 200 ) ) {
        bedding_warmth = std::max( 0, bedding_warmth );
    }
    return ( item_warmth + bedding_warmth + floor_mut_warmth );
}

int player::bodytemp_modifier_fire() const
{
    int temp_conv = 0;
    // Being on fire increases very intensely the convergent temperature.
    if( has_effect( effect_onfire ) ) {
        temp_conv += 15000;
    }

    const trap &trap_at_pos = g->m.tr_at( pos() );
    // Same with standing on fire.
    int tile_strength = g->m.get_field_strength( pos(), fd_fire );
    if( tile_strength > 2 || trap_at_pos.loadid == tr_lava ) {
        temp_conv += 15000;
    }
    // Standing in the hot air of a fire is nice.
    tile_strength = g->m.get_field_strength( pos(), fd_hot_air1 );
    switch( tile_strength ) {
        case 3:
            temp_conv += 500;
            break;
        case 2:
            temp_conv += 300;
            break;
        case 1:
            temp_conv += 100;
            break;
        default:
            break;
    }
    tile_strength = g->m.get_field_strength( pos(), fd_hot_air2 );
    switch( tile_strength ) {
        case 3:
            temp_conv += 1000;
            break;
        case 2:
            temp_conv += 800;
            break;
        case 1:
            temp_conv += 300;
            break;
        default:
            break;
    }
    tile_strength = g->m.get_field_strength( pos(), fd_hot_air3 );
    switch( tile_strength ) {
        case 3:
            temp_conv += 3500;
            break;
        case 2:
            temp_conv += 2000;
            break;
        case 1:
            temp_conv += 800;
            break;
        default:
            break;
    }
    tile_strength = g->m.get_field_strength( pos(), fd_hot_air4 );
    switch( tile_strength ) {
        case 3:
            temp_conv += 8000;
            break;
        case 2:
            temp_conv += 5000;
            break;
        case 1:
            temp_conv += 3500;
            break;
        default:
            break;
    }
    return temp_conv;
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
    int blood_loss = 0;
    if( bp == bp_leg_l || bp == bp_leg_r ) {
        blood_loss = ( 100 - 100 * ( hp_cur[hp_leg_l] + hp_cur[hp_leg_r] ) /
                       ( hp_max[hp_leg_l] + hp_max[hp_leg_r] ) );
    } else if( bp == bp_arm_l || bp == bp_arm_r ) {
        blood_loss = ( 100 - 100 * ( hp_cur[hp_arm_l] + hp_cur[hp_arm_r] ) /
                       ( hp_max[hp_arm_l] + hp_max[hp_arm_r] ) );
    } else if( bp == bp_torso ) {
        blood_loss = ( 100 - 100 * hp_cur[hp_torso] / hp_max[hp_torso] );
    } else if( bp == bp_head ) {
        blood_loss = ( 100 - 100 * hp_cur[hp_head] / hp_max[hp_head] );
    }
    return blood_loss;
}

void player::temp_equalizer( body_part bp1, body_part bp2 )
{
    // Body heat is moved around.
    // Shift in one direction only, will be shifted in the other direction separately.
    int diff = int( ( temp_cur[bp2] - temp_cur[bp1] ) * 0.0001 ); // If bp1 is warmer, it will lose heat
    temp_cur[bp1] += diff;
}

int player::hunger_speed_penalty( int hunger )
{
    // We die at 6000 hunger
    // Hunger hits speed less hard than thirst does
    static const std::vector<std::pair<float, float>> hunger_thresholds = {{
            std::make_pair( 100.0f, 0.0f ),
            std::make_pair( 300.0f, -15.0f ),
            std::make_pair( 1000.0f, -40.0f ),
            std::make_pair( 6000.0f, -75.0f )
        }
    };
    return ( int )multi_lerp( hunger_thresholds, hunger );
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
    return ( int )multi_lerp( thirst_thresholds, thirst );
}

void player::recalc_speed_bonus()
{
    // Minus some for weight...
    int carry_penalty = 0;
    if( weight_carried() > weight_capacity() ) {
        carry_penalty = 25 * ( weight_carried() - weight_capacity() ) / ( weight_capacity() );
    }
    mod_speed_bonus( -carry_penalty );

    mod_speed_bonus( -get_pain_penalty().speed );

    if( get_painkiller() >= 10 ) {
        int pkill_penalty = int( get_painkiller() * .1 );
        if( pkill_penalty > 30 ) {
            pkill_penalty = 30;
        }
        mod_speed_bonus( -pkill_penalty );
    }

    if( abs( get_morale_level() ) >= 100 ) {
        int morale_bonus = get_morale_level() / 25;
        if( morale_bonus < -10 ) {
            morale_bonus = -10;
        } else if( morale_bonus > 10 ) {
            morale_bonus = 10;
        }
        mod_speed_bonus( morale_bonus );
    }

    if( radiation >= 40 ) {
        int rad_penalty = radiation / 40;
        if( rad_penalty > 20 ) {
            rad_penalty = 20;
        }
        mod_speed_bonus( -rad_penalty );
    }

    if( get_thirst() > 40 ) {
        mod_speed_bonus( thirst_speed_penalty( get_thirst() ) );
    }
    if( get_hunger() > 100 ) {
        mod_speed_bonus( hunger_speed_penalty( get_hunger() ) );
    }

    mod_speed_bonus( stim > 10 ? 10 : stim / 4 );

    for( auto maps : effects ) {
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
    if( g != NULL ) {
        if( has_trait( trait_SUNLIGHT_DEPENDENT ) && !g->is_in_sunlight( pos() ) ) {
            mod_speed_bonus( -( g->light_level( posz() ) >= 12 ? 5 : 10 ) );
        }
        if( has_trait( trait_COLDBLOOD4 ) || ( has_trait( trait_COLDBLOOD3 ) && g->get_temperature() < 65 ) ) {
            mod_speed_bonus( ( g->get_temperature() - 65 ) / 2 );
        } else if( has_trait( trait_COLDBLOOD2 ) && g->get_temperature() < 65 ) {
            mod_speed_bonus( ( g->get_temperature() - 65 ) / 3 );
        } else if( has_trait( trait_COLDBLOOD ) && g->get_temperature() < 65 ) {
            mod_speed_bonus( ( g->get_temperature() - 65 ) / 5 );
        }
    }

    if( has_trait( trait_M_SKIN2 ) ) {
        mod_speed_bonus( -20 ); // Could be worse--you've got the armor from a (sessile!) Spire
    }

    if( has_artifact_with( AEP_SPEED_UP ) ) {
        mod_speed_bonus( 20 );
    }
    if( has_artifact_with( AEP_SPEED_DOWN ) ) {
        mod_speed_bonus( -20 );
    }

    if( has_trait( trait_QUICK ) ) { // multiply by 1.1
        set_speed_bonus( int( get_speed() * 1.1 ) - get_speed_base() );
    }
    if( has_bionic( "bio_speed" ) ) { // multiply by 1.1
        set_speed_bonus( int( get_speed() * 1.1 ) - get_speed_base() );
    }

    // Speed cannot be less than 25% of base speed, so minimal speed bonus is -75% base speed.
    const int min_speed_bonus = int( -0.75 * get_speed_base() );
    if( get_speed_bonus() < min_speed_bonus ) {
        set_speed_bonus( min_speed_bonus );
    }
}

int player::run_cost( int base_cost, bool diag ) const
{
    float movecost = float( base_cost );
    if( diag ) {
        movecost *= 0.7071f; // because everything here assumes 100 is base
    }

    const bool flatground = movecost < 105;
    // The "FLAT" tag includes soft surfaces, so not a good fit.
    const bool on_road = flatground && g->m.has_flag( "ROAD", pos() );

    if( has_trait( trait_PARKOUR ) && movecost > 100 ) {
        movecost *= .5f;
        if( movecost < 100 ) {
            movecost = 100;
        }
    }
    if( has_trait( trait_BADKNEES ) && movecost > 100 ) {
        movecost *= 1.25f;
        if( movecost < 100 ) {
            movecost = 100;
        }
    }

    if( hp_cur[hp_leg_l] == 0 ) {
        movecost += 50;
    } else if( hp_cur[hp_leg_l] < hp_max[hp_leg_l] * .40 ) {
        movecost += 25;
    }

    if( hp_cur[hp_leg_r] == 0 ) {
        movecost += 50;
    } else if( hp_cur[hp_leg_r] < hp_max[hp_leg_r] * .40 ) {
        movecost += 25;
    }

    if( has_trait( trait_FLEET ) && flatground ) {
        movecost *= .85f;
    }
    if( has_trait( trait_FLEET2 ) && flatground ) {
        movecost *= .7f;
    }
    if( has_trait( trait_SLOWRUNNER ) && flatground ) {
        movecost *= 1.15f;
    }
    if( has_trait( trait_PADDED_FEET ) && !footwear_factor() ) {
        movecost *= .9f;
    }
    if( has_trait( trait_LIGHT_BONES ) ) {
        movecost *= .9f;
    }
    if( has_trait( trait_HOLLOW_BONES ) ) {
        movecost *= .8f;
    }
    if( has_active_mutation( trait_id( "WINGS_INSECT" ) ) ) {
        movecost *= .75f;
    }
    if( has_trait( trait_WINGS_BUTTERFLY ) ) {
        movecost -= 10; // You can't fly, but you can make life easier on your legs
    }
    if( has_trait( trait_LEG_TENTACLES ) ) {
        movecost += 20;
    }
    if( has_trait( trait_FAT ) ) {
        movecost *= 1.05f;
    }
    if( has_trait( trait_PONDEROUS1 ) ) {
        movecost *= 1.1f;
    }
    if( has_trait( trait_PONDEROUS2 ) ) {
        movecost *= 1.2f;
    }
    if( has_trait( trait_AMORPHOUS ) ) {
        movecost *= 1.25f;
    }
    if( has_trait( trait_PONDEROUS3 ) ) {
        movecost *= 1.3f;
    }
    if( is_wearing( "stillsuit" ) ) {
        movecost *= 1.1f;
    }
    if( is_wearing( "swim_fins" ) ) {
        movecost *= 1.5f;
    }
    if( is_wearing( "roller_blades" ) ) {
        if( on_road ) {
            movecost *= 0.5f;
        } else {
            movecost *= 1.5f;
        }
    }
    // Quad skates might be more stable than inlines,
    // but that also translates into a slower speed when on good surfaces.
    if( is_wearing( "rollerskates" ) ) {
        if( on_road ) {
            movecost *= 0.7f;
        } else {
            movecost *= 1.3f;
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
    if( !is_wearing_shoes( "left" ) && !mutfeet ) {
        movecost += 8;
    }
    if( !is_wearing_shoes( "right" ) && !mutfeet ) {
        movecost += 8;
    }

    if( !footwear_factor() && has_trait( trait_ROOTS3 ) &&
        g->m.has_flag( "DIGGABLE", pos() ) ) {
        movecost += 10 * footwear_factor();
    }

    // Both walk and run speed drop to half their maximums as stamina approaches 0.
    // Convert stamina to a float first to allow for decimal place carrying
    float stamina_modifier = ( float( stamina ) / get_stamina_max() + 1 ) / 2;
    if( move_mode == "run" && stamina > 0 ) {
        // Rationale: Average running speed is 2x walking speed. (NOT sprinting)
        stamina_modifier *= 2.0;
    }
    movecost /= stamina_modifier;

    if( diag ) {
        movecost *= 1.4142;
    }

    return int( movecost );
}

int player::swim_speed() const
{
    int ret = 440 + weight_carried() / 60 - 50 * get_skill_level( skill_swimming );
    const auto usable = exclusive_flag_coverage( "ALLOWS_NATURAL_ATTACKS" );
    float hand_bonus_mult = ( usable.test( bp_hand_l ) ? 0.5f : 0.0f ) +
                            ( usable.test( bp_hand_r ) ? 0.5f : 0.0f );
    /** @EFFECT_STR increases swim speed bonus from PAWS */
    if( has_trait( trait_PAWS ) ) {
        ret -= hand_bonus_mult * ( 20 + str_cur * 3 );
    }
    /** @EFFECT_STR increases swim speed bonus from PAWS_LARGE */
    if( has_trait( trait_PAWS_LARGE ) ) {
        ret -= hand_bonus_mult * ( 20 + str_cur * 4 );
    }
    /** @EFFECT_STR increases swim speed bonus from swim_fins */
    if( is_wearing( "swim_fins" ) ) {
        ret -= ( 15 * str_cur ) / ( 3 - shoe_type_count( "swim_fins" ) );
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
    return hp_cur[hp_leg_l] == 0 || hp_cur[hp_leg_r] == 0 || has_effect( effect_downed );
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
        return worn_with_flag( "DEAF" ) || worn_with_flag( "PARTIAL_DEAF" ) || has_bionic( "bio_ears" ) || is_wearing( "rm13_armor_on" );
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
            return false;
        case DT_BASH:
            return false;
        case DT_CUT:
            return false;
        case DT_ACID:
            return has_trait( trait_ACIDPROOF );
        case DT_STAB:
            return false;
        case DT_HEAT:
            return has_trait( trait_M_SKIN2 );
        case DT_COLD:
            return false;
        case DT_ELECTRIC:
            return has_active_bionic( "bio_faraday" ) ||
                   worn_with_flag( "ELECTRIC_IMMUNE" ) ||
                   has_artifact_with( AEP_RESIST_ELECTRICITY );
        default:
            return true;
    }
}

double player::recoil_vehicle() const
{
    // @todo vary penalty dependent upon vehicle part on which player is boarded

    if( in_vehicle ) {
        vehicle *veh = g->m.veh_at( pos() );
        if( veh ) {
            return double( abs( veh->velocity ) ) * 3 / 100;
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
        return c_ltblue;
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
    if( has_active_bionic( "bio_cloak" ) || has_artifact_with( AEP_INVISIBLE ) ||
        has_active_optcloak() || has_trait( trait_DEBUG_CLOAK ) ) {
        return c_dkgray;
    }
    return c_white;
}

void player::load_info( std::string data )
{
    std::stringstream dump;
    dump << data;

    JsonIn jsin( dump );
    try {
        deserialize( jsin );
    } catch( const JsonError &jsonerr ) {
        debugmsg( "Bad player json\n%s", jsonerr.c_str() );
    }
}

std::string player::save_info() const
{
    std::stringstream dump;
    dump << serialize(); // saves contents
    dump << std::endl;
    dump << dump_memorial();
    return dump.str();
}

void player::memorial( std::ostream &memorial_file, std::string epitaph )
{
    static const char *eol = cata_files::eol();

    //Size of indents in the memorial file
    const std::string indent = "  ";

    const std::string pronoun = male ? _( "He" ) : _( "She" );

    //Avoid saying "a male unemployed" or similar
    std::string profession_name;
    if( prof == prof->generic() ) {
        if( male ) {
            profession_name = _( "an unemployed male" );
        } else {
            profession_name = _( "an unemployed female" );
        }
    } else {
        profession_name = string_format( _( "a %s" ), prof->gender_appropriate_name( male ).c_str() );
    }

    //Figure out the location
    const oter_id &cur_ter = overmap_buffer.ter( global_omt_location() );
    const std::string &tername = cur_ter->get_name();

    //Were they in a town, or out in the wilderness?
    const auto global_sm_pos = global_sm_location();
    const auto closest_city = overmap_buffer.closest_city( global_sm_pos );
    std::string kill_place;
    if( !closest_city ) {
        //~ First parameter is a pronoun ("He"/"She"), second parameter is a terrain name.
        kill_place = string_format( _( "%1$s was killed in a %2$s in the middle of nowhere." ),
                                    pronoun.c_str(), tername.c_str() );
    } else {
        const auto &nearest_city = *closest_city.city;
        //Give slightly different messages based on how far we are from the middle
        const int distance_from_city = closest_city.distance - nearest_city.s;
        if( distance_from_city > nearest_city.s + 4 ) {
            //~ First parameter is a pronoun ("He"/"She"), second parameter is a terrain name.
            kill_place = string_format( _( "%1$s was killed in a %2$s in the wilderness." ),
                                        pronoun.c_str(), tername.c_str() );

        } else if( distance_from_city >= nearest_city.s ) {
            //~ First parameter is a pronoun ("He"/"She"), second parameter is a terrain name, third parameter is a city name.
            kill_place = string_format( _( "%1$s was killed in a %2$s on the outskirts of %3$s." ),
                                        pronoun.c_str(), tername.c_str(), nearest_city.name.c_str() );
        } else {
            //~ First parameter is a pronoun ("He"/"She"), second parameter is a terrain name, third parameter is a city name.
            kill_place = string_format( _( "%1$s was killed in a %2$s in %3$s." ),
                                        pronoun.c_str(), tername.c_str(), nearest_city.name.c_str() );
        }
    }

    //Header
    std::string version = string_format( "%s", getVersionString() );
    memorial_file << string_format( _( "Cataclysm - Dark Days Ahead version %s memorial file" ),
                                    version.c_str() ) << eol;
    memorial_file << eol;
    memorial_file << string_format( _( "In memory of: %s" ), name.c_str() ) << eol;
    if( epitaph.length() > 0 ) { //Don't record empty epitaphs
        //~ The "%s" will be replaced by an epitaph as displyed in the memorial files. Replace the quotation marks as appropriate for your language.
        memorial_file << string_format( pgettext( "epitaph", "\"%s\"" ), epitaph.c_str() ) << eol << eol;
    }
    //~ First parameter: Pronoun, second parameter: a profession name (with article)
    memorial_file << string_format( _( "%1$s was %2$s when the apocalypse began." ),
                                    pronoun.c_str(), profession_name.c_str() ) << eol;
    memorial_file << string_format( _( "%1$s died on %2$s of year %3$d, day %4$d, at %5$s." ),
                                    pronoun.c_str(), season_name_upper( calendar::turn.get_season() ).c_str(),
                                    ( calendar::turn.years() + 1 ),
                                    ( calendar::turn.days() + 1 ), calendar::turn.print_time().c_str() ) << eol;
    memorial_file << kill_place << eol;
    memorial_file << eol;

    //Misc
    memorial_file << string_format( _( "Cash on hand: $%d" ), cash ) << eol;
    memorial_file << eol;

    //HP

    const auto limb_hp =
    [this, &memorial_file, &indent]( const std::string & desc, const hp_part bp ) {
        memorial_file << indent << string_format( desc, get_hp( bp ), get_hp_max( bp ) ) << eol;
    };

    memorial_file << _( "Final HP:" ) << eol;
    limb_hp( _( " Head: %d/%d" ), hp_head );
    limb_hp( _( "Torso: %d/%d" ), hp_torso );
    limb_hp( _( "L Arm: %d/%d" ), hp_arm_l );
    limb_hp( _( "R Arm: %d/%d" ), hp_arm_r );
    limb_hp( _( "L Leg: %d/%d" ), hp_leg_l );
    limb_hp( _( "R Leg: %d/%d" ), hp_leg_r );
    memorial_file << eol;

    //Stats
    memorial_file << _( "Final Stats:" ) << eol;
    memorial_file << indent << string_format( _( "Str %d" ), str_cur )
                  << indent << string_format( _( "Dex %d" ), dex_cur )
                  << indent << string_format( _( "Int %d" ), int_cur )
                  << indent << string_format( _( "Per %d" ), per_cur ) << eol;
    memorial_file << _( "Base Stats:" ) << eol;
    memorial_file << indent << string_format( _( "Str %d" ), str_max )
                  << indent << string_format( _( "Dex %d" ), dex_max )
                  << indent << string_format( _( "Int %d" ), int_max )
                  << indent << string_format( _( "Per %d" ), per_max ) << eol;
    memorial_file << eol;

    //Last 20 messages
    memorial_file << _( "Final Messages:" ) << eol;
    std::vector<std::pair<std::string, std::string> > recent_messages = Messages::recent_messages( 20 );
    for( auto &recent_message : recent_messages ) {
        memorial_file << indent << recent_message.first << " " << recent_message.second;
        memorial_file << eol;
    }
    memorial_file << eol;

    //Kill list
    memorial_file << _( "Kills:" ) << eol;

    int total_kills = 0;

    std::map<std::tuple<std::string, std::string>, int> kill_counts;

    // map <name, sym> to kill count
    for( const auto &type : MonsterGenerator::generator().get_all_mtypes() ) {
        if( g->kill_count( type.id ) > 0 ) {
            kill_counts[std::tuple<std::string, std::string>(
                            type.nname(),
                            type.sym
                        )] += g->kill_count( type.id );
            total_kills += g->kill_count( type.id );
        }
    }

    for( const auto entry : kill_counts ) {
        memorial_file << "  " << std::get<1>( entry.first ) << " - "
                      << string_format( "%4d", entry.second ) << " "
                      << std::get<0>( entry.first ) << eol;
    }

    if( total_kills == 0 ) {
        memorial_file << indent << _( "No monsters were killed." ) << eol;
    } else {
        memorial_file << string_format( _( "Total kills: %d" ), total_kills ) << eol;
    }
    memorial_file << eol;

    //Skills
    memorial_file << _( "Skills:" ) << eol;
    for( auto &skill : Skill::skills ) {
        SkillLevel next_skill_level = get_skill_level( skill.ident() );
        memorial_file << indent << skill.name() << ": " << next_skill_level.level() << " ("
                      << next_skill_level.exercise() << "%)" << eol;
    }
    memorial_file << eol;

    //Traits
    memorial_file << _( "Traits:" ) << eol;
    for( auto &iter : my_mutations ) {
        memorial_file << indent << mutation_branch::get_name( iter.first ) << eol;
    }
    if( !my_mutations.empty() ) {
        memorial_file << indent << _( "(None)" ) << eol;
    }
    memorial_file << eol;

    //Effects (illnesses)
    memorial_file << _( "Ongoing Effects:" ) << eol;
    bool had_effect = false;
    if( get_morale_level() >= 100 ) {
        had_effect = true;
        memorial_file << indent << _( "Elated" ) << eol;
    }
    if( get_morale_level() <= -100 ) {
        had_effect = true;
        memorial_file << indent << _( "Depressed" ) << eol;
    }
    if( get_perceived_pain() > 0 ) {
        had_effect = true;
        memorial_file << indent << _( "Pain" ) << " (" << get_perceived_pain() << ")";
    }
    if( stim > 0 ) {
        had_effect = true;
        int dexbonus = stim / 10;
        if( abs( stim ) >= 30 ) {
            dexbonus -= abs( stim - 15 ) /  8;
        }
        if( dexbonus < 0 ) {
            memorial_file << indent << _( "Stimulant Overdose" ) << eol;
        } else {
            memorial_file << indent << _( "Stimulant" ) << eol;
        }
    } else if( stim < 0 ) {
        had_effect = true;
        memorial_file << indent << _( "Depressants" ) << eol;
    }
    if( !had_effect ) {
        memorial_file << indent << _( "(None)" ) << eol;
    }
    memorial_file << eol;

    //Bionics
    memorial_file << _( "Bionics:" ) << eol;
    int total_bionics = 0;
    for( size_t i = 0; i < my_bionics.size(); ++i ) {
        memorial_file << indent << ( i + 1 ) << ": " << bionic_info( my_bionics[i].id ).name << eol;
        total_bionics++;
    }
    if( total_bionics == 0 ) {
        memorial_file << indent << _( "No bionics were installed." ) << eol;
    } else {
        memorial_file << string_format( _( "Total bionics: %d" ), total_bionics ) << eol;
    }
    memorial_file << string_format( _( "Power: %d/%d" ), power_level,  max_power_level ) << eol;
    memorial_file << eol;

    //Equipment
    memorial_file << _( "Weapon:" ) << eol;
    memorial_file << indent << weapon.invlet << " - " << weapon.tname( 1, false ) << eol;
    memorial_file << eol;

    memorial_file << _( "Equipment:" ) << eol;
    for( auto &elem : worn ) {
        item next_item = elem;
        memorial_file << indent << next_item.invlet << " - " << next_item.tname( 1, false );
        if( next_item.charges > 0 ) {
            memorial_file << " (" << next_item.charges << ")";
        } else if( next_item.contents.size() == 1 && next_item.contents.front().charges > 0 ) {
            memorial_file << " (" << next_item.contents.front().charges << ")";
        }
        memorial_file << eol;
    }
    memorial_file << eol;

    //Inventory
    memorial_file << _( "Inventory:" ) << eol;
    inv.restack( this );
    inv.sort();
    invslice slice = inv.slice();
    for( auto &elem : slice ) {
        item &next_item = elem->front();
        memorial_file << indent << next_item.invlet << " - " <<
                      next_item.tname( unsigned( elem->size() ), false );
        if( elem->size() > 1 ) {
            memorial_file << " [" << elem->size() << "]";
        }
        if( next_item.charges > 0 ) {
            memorial_file << " (" << next_item.charges << ")";
        } else if( next_item.contents.size() == 1 && next_item.contents.front().charges > 0 ) {
            memorial_file << " (" << next_item.contents.front().charges << ")";
        }
        memorial_file << eol;
    }
    memorial_file << eol;

    //Lifetime stats
    memorial_file << _( "Lifetime Stats" ) << eol;
    memorial_file << indent << string_format( _( "Distance walked: %d squares" ),
                  player_stats.squares_walked ) << eol;
    memorial_file << indent << string_format( _( "Damage taken: %d damage" ),
                  player_stats.damage_taken ) << eol;
    memorial_file << indent << string_format( _( "Damage healed: %d damage" ),
                  player_stats.damage_healed ) << eol;
    memorial_file << indent << string_format( _( "Headshots: %d" ),
                  player_stats.headshots ) << eol;
    memorial_file << eol;

    //History
    memorial_file << _( "Game History" ) << eol;
    memorial_file << dump_memorial();

}

/**
 * Adds an event to the memorial log, to be written to the memorial file when
 * the character dies. The message should contain only the informational string,
 * as the timestamp and location will be automatically prepended.
 */
void player::add_memorial_log( const char *male_msg, const char *female_msg, ... )
{

    va_list ap;

    va_start( ap, female_msg );
    std::string msg;
    if( this->male ) {
        msg = vstring_format( male_msg, ap );
    } else {
        msg = vstring_format( female_msg, ap );
    }
    va_end( ap );

    if( msg.empty() ) {
        return;
    }

    std::stringstream timestamp;
    //~ A timestamp. Parameters from left to right: Year, season, day, time
    timestamp << string_format( _( "Year %1$d, %2$s %3$d, %4$s" ), calendar::turn.years() + 1,
                                season_name_upper( calendar::turn.get_season() ).c_str(),
                                calendar::turn.days() + 1, calendar::turn.print_time().c_str()
                              );

    const oter_id &cur_ter = overmap_buffer.ter( global_omt_location() );
    const std::string &location = cur_ter->get_name();

    std::stringstream log_message;
    log_message << "| " << timestamp.str() << " | " << location.c_str() << " | " << msg;

    memorial_log.push_back( log_message.str() );

}

/**
 * Loads the data in a memorial file from the given ifstream. All the memorial
 * entry lines begin with a pipe (|).
 * @param fin The ifstream to read the memorial entries from.
 */
void player::load_memorial_file( std::istream &fin )
{
    std::string entry;
    memorial_log.clear();
    while( fin.peek() == '|' ) {
        getline( fin, entry );
        memorial_log.push_back( entry );
    }
}

/**
 * Concatenates all of the memorial log entries, delimiting them with newlines,
 * and returns the resulting string. Used for saving and for writing out to the
 * memorial file.
 */
std::string player::dump_memorial() const
{
    static const char *eol = cata_files::eol();
    std::stringstream output;

    for( auto &elem : memorial_log ) {
        output << elem << eol;
    }

    return output.str();
}

/**
 * Returns a pointer to the stat-tracking struct. Its fields should be edited
 * as necessary to track ongoing counters, which will be added to the memorial
 * file. For single events, rather than cumulative counters, see
 * add_memorial_log.
 * @return A pointer to the stats struct being used to track this player's
 *         lifetime stats.
 */
stats *player::lifetime_stats()
{
    return &player_stats;
}

// copy of stats, for saving
stats player::get_stats() const
{
    return player_stats;
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
        stamina += modifier;
        stamina = std::min( stamina, get_stamina_max() );
        stamina = std::max( 0, stamina );
    } else {
        // Fall through to the creature method.
        Character::mod_stat( stat, modifier );
    }
}

void player::disp_morale()
{
    morale->display( ( calc_focus_equilibrium() - focus_pool ) / 100.0 );
}

static std::string print_gun_mode( const player &p )
{
    auto m = p.weapon.gun_current_mode();
    if( m ) {
        if( m.melee() || !m->is_gunmod() ) {
            return string_format( m.mode.empty() ? "%s" : "%s (%s)",
                                  p.weapname().c_str(), _( m.mode.c_str() ) );
        } else {
            return string_format( "%s (%i/%i)", m->tname().c_str(),
                                  m->ammo_remaining(), m->ammo_capacity() );
        }
    } else {
        return p.weapname();
    }
}

void player::print_stamina_bar( WINDOW *w ) const
{
    std::string sta_bar = "";
    nc_color sta_color;
    std::tie( sta_bar, sta_color ) = get_hp_bar( stamina ,  get_stamina_max() );
    wprintz( w, sta_color, sta_bar.c_str() );
}

void player::disp_status( WINDOW *w, WINDOW *w2 )
{
    bool sideStyle = use_narrow_sidebar();
    WINDOW *weapwin = sideStyle ? w2 : w;

    {
        const int y = sideStyle ? 1 : 0;
        const int wn = getmaxx( weapwin );
        trim_and_print( weapwin, y, 0, wn, c_ltgray, "%s", print_gun_mode( *this ).c_str() );
    }

    // Print currently used style or weapon mode.
    std::string style = "";
    const auto style_color = is_armed() ? c_red : c_blue;
    const auto &cur_style = style_selected.obj();
    if( cur_style.weapon_valid( weapon ) ) {
        style = cur_style.name;
    } else if( is_armed() ) {
        style = _( "Normal" );
    } else {
        style = _( "No Style" );
    }

    if( !style.empty() ) {
        int x = sideStyle ? ( getmaxx( weapwin ) - 13 ) : 0;
        mvwprintz( weapwin, 1, x, style_color, style.c_str() );
    }

    wmove( w, sideStyle ? 1 : 2, 0 );
    if( get_hunger() > 2800 ) {
        wprintz( w, c_red,    _( "Starving!" ) );
    } else if( get_hunger() > 1400 ) {
        wprintz( w, c_ltred,  _( "Near starving" ) );
    } else if( get_hunger() > 300 ) {
        wprintz( w, c_ltred,  _( "Famished" ) );
    } else if( get_hunger() > 100 ) {
        wprintz( w, c_yellow, _( "Very hungry" ) );
    } else if( get_hunger() > 40 ) {
        wprintz( w, c_yellow, _( "Hungry" ) );
    } else if( get_hunger() < 0 ) {
        wprintz( w, c_green,  _( "Full" ) );
    } else if( get_hunger() < -20 ) {
        wprintz( w, c_green,  _( "Sated" ) );
    } else if( get_hunger() < -60 ) {
        wprintz( w, c_green,  _( "Engorged" ) );
    }

    /// Find hottest/coldest bodypart
    // Calculate the most extreme body temperatures
    int current_bp_extreme = 0, conv_bp_extreme = 0;
    for( int i = 0; i < num_bp ; i++ ) {
        if( abs( temp_cur[i] - BODYTEMP_NORM ) > abs( temp_cur[current_bp_extreme] - BODYTEMP_NORM ) ) {
            current_bp_extreme = i;
        }
        if( abs( temp_conv[i] - BODYTEMP_NORM ) > abs( temp_conv[conv_bp_extreme] - BODYTEMP_NORM ) ) {
            conv_bp_extreme = i;
        }
    }

    // Assign zones for comparisons
    int cur_zone = 0, conv_zone = 0;
    if( temp_cur[current_bp_extreme] >  BODYTEMP_SCORCHING ) {
        cur_zone = 7;
    } else if( temp_cur[current_bp_extreme] >  BODYTEMP_VERY_HOT ) {
        cur_zone = 6;
    } else if( temp_cur[current_bp_extreme] >  BODYTEMP_HOT ) {
        cur_zone = 5;
    } else if( temp_cur[current_bp_extreme] >  BODYTEMP_COLD ) {
        cur_zone = 4;
    } else if( temp_cur[current_bp_extreme] >  BODYTEMP_VERY_COLD ) {
        cur_zone = 3;
    } else if( temp_cur[current_bp_extreme] >  BODYTEMP_FREEZING ) {
        cur_zone = 2;
    } else if( temp_cur[current_bp_extreme] <= BODYTEMP_FREEZING ) {
        cur_zone = 1;
    }

    if( temp_conv[conv_bp_extreme] >  BODYTEMP_SCORCHING ) {
        conv_zone = 7;
    } else if( temp_conv[conv_bp_extreme] >  BODYTEMP_VERY_HOT ) {
        conv_zone = 6;
    } else if( temp_conv[conv_bp_extreme] >  BODYTEMP_HOT ) {
        conv_zone = 5;
    } else if( temp_conv[conv_bp_extreme] >  BODYTEMP_COLD ) {
        conv_zone = 4;
    } else if( temp_conv[conv_bp_extreme] >  BODYTEMP_VERY_COLD ) {
        conv_zone = 3;
    } else if( temp_conv[conv_bp_extreme] >  BODYTEMP_FREEZING ) {
        conv_zone = 2;
    } else if( temp_conv[conv_bp_extreme] <= BODYTEMP_FREEZING ) {
        conv_zone = 1;
    }

    // delta will be positive if temp_cur is rising
    int delta = conv_zone - cur_zone;
    // Decide if temp_cur is rising or falling
    const char *temp_message = "Error";
    if( delta >   2 ) {
        temp_message = _( " (Rising!!)" );
    } else if( delta ==  2 ) {
        temp_message = _( " (Rising!)" );
    } else if( delta ==  1 ) {
        temp_message = _( " (Rising)" );
    } else if( delta ==  0 ) {
        temp_message = "";
    } else if( delta == -1 ) {
        temp_message = _( " (Falling)" );
    } else if( delta == -2 ) {
        temp_message = _( " (Falling!)" );
    } else if( delta <  -2 ) {
        temp_message = _( " (Falling!!)" );
    }

    // printCur the hottest/coldest bodypart, and if it is rising or falling in temperature
    wmove( w, sideStyle ? 6 : 1, sideStyle ? 0 : 9 );
    if( temp_cur[current_bp_extreme] >  BODYTEMP_SCORCHING ) {
        wprintz( w, c_red,   _( "Scorching!%s" ), temp_message );
    } else if( temp_cur[current_bp_extreme] >  BODYTEMP_VERY_HOT ) {
        wprintz( w, c_ltred, _( "Very hot!%s" ), temp_message );
    } else if( temp_cur[current_bp_extreme] >  BODYTEMP_HOT ) {
        wprintz( w, c_yellow, _( "Warm%s" ), temp_message );
    } else if( temp_cur[current_bp_extreme] >
               BODYTEMP_COLD ) { // If you're warmer than cold, you are comfortable
        wprintz( w, c_green, _( "Comfortable%s" ), temp_message );
    } else if( temp_cur[current_bp_extreme] >  BODYTEMP_VERY_COLD ) {
        wprintz( w, c_ltblue, _( "Chilly%s" ), temp_message );
    } else if( temp_cur[current_bp_extreme] >  BODYTEMP_FREEZING ) {
        wprintz( w, c_cyan,  _( "Very cold!%s" ), temp_message );
    } else if( temp_cur[current_bp_extreme] <= BODYTEMP_FREEZING ) {
        wprintz( w, c_blue,  _( "Freezing!%s" ), temp_message );
    }

    int x = 32;
    int y = sideStyle ?  0 :  1;
    if( is_deaf() ) {
        mvwprintz( sideStyle ? w2 : w, y, x, c_red, _( "Deaf!" ) );
    } else {
        mvwprintz( sideStyle ? w2 : w, y, x, c_yellow, _( "Sound %d" ), volume );
    }
    volume = 0;

    wmove( w, 2, sideStyle ? 0 : 15 );
    if( get_thirst() > 520 ) {
        wprintz( w, c_ltred,  _( "Parched" ) );
    } else if( get_thirst() > 240 ) {
        wprintz( w, c_ltred,  _( "Dehydrated" ) );
    } else if( get_thirst() > 80 ) {
        wprintz( w, c_yellow, _( "Very thirsty" ) );
    } else if( get_thirst() > 40 ) {
        wprintz( w, c_yellow, _( "Thirsty" ) );
    } else if( get_thirst() < 0 ) {
        wprintz( w, c_green,  _( "Slaked" ) );
    } else if( get_thirst() < -20 ) {
        wprintz( w, c_green,  _( "Hydrated" ) );
    } else if( get_thirst() < -60 ) {
        wprintz( w, c_green,  _( "Turgid" ) );
    }

    wmove( w, sideStyle ? 3 : 2, sideStyle ? 0 : 30 );
    if( get_fatigue() > EXHAUSTED ) {
        wprintz( w, c_red,    _( "Exhausted" ) );
    } else if( get_fatigue() > DEAD_TIRED ) {
        wprintz( w, c_ltred,  _( "Dead tired" ) );
    } else if( get_fatigue() > TIRED ) {
        wprintz( w, c_yellow, _( "Tired" ) );
    }

    wmove( w, sideStyle ? 4 : 2, sideStyle ? 0 : 41 );
    wprintz( w, c_white, _( "Focus" ) );
    nc_color col_xp = c_dkgray;
    if( focus_pool >= 100 ) {
        col_xp = c_white;
    } else if( focus_pool >  0 ) {
        col_xp = c_ltgray;
    }
    wprintz( w, col_xp, " %d", focus_pool );

    nc_color col_pain = c_yellow;
    if( get_perceived_pain() >= 60 ) {
        col_pain = c_red;
    } else if( get_perceived_pain() >= 40 ) {
        col_pain = c_ltred;
    }
    if( get_perceived_pain() > 0 ) {
        mvwprintz( w, sideStyle ? 0 : 3, 0, col_pain, _( "Pain %d" ), get_perceived_pain() );
    }

    int morale_cur = get_morale_level();
    nc_color col_morale = c_white;
    if( morale_cur >= 10 ) {
        col_morale = c_green;
    } else if( morale_cur <= -10 ) {
        col_morale = c_red;
    }
    const char *morale_str;
    if( morale_cur >= 200 ) {
        morale_str = "8D";
    } else if( morale_cur >= 100 ) {
        morale_str = ":D";
    } else if( has_trait( trait_THRESH_FELINE ) && morale_cur >= 10 ) {
        morale_str = ":3";
    } else if( !has_trait( trait_THRESH_FELINE ) && morale_cur >= 10 ) {
        morale_str = ":)";
    } else if( morale_cur > -10 ) {
        morale_str = ":|";
    } else if( morale_cur > -100 ) {
        morale_str = "):";
    } else if( morale_cur > -200 ) {
        morale_str = "D:";
    } else {
        morale_str = "D8";
    }
    mvwprintz( w, sideStyle ? 0 : 3, sideStyle ? 11 : 9, col_morale, morale_str );

    vehicle *veh = g->remoteveh();
    if( veh == nullptr && in_vehicle ) {
        veh = g->m.veh_at( pos() );
    }
    if( veh ) {
        veh->print_fuel_indicators( w, sideStyle ? 2 : 3, sideStyle ? getmaxx( w ) - 5 : 49 );
        nc_color col_indf1 = c_ltgray;

        float strain = veh->strain();
        nc_color col_vel = strain <= 0 ? c_ltblue :
                           ( strain <= 0.2 ? c_yellow :
                             ( strain <= 0.4 ? c_ltred : c_red ) );

        //
        // Draw the speedometer.
        //

        int speedox = sideStyle ? 0 : 33;
        int speedoy = sideStyle ? 5 :  3;

        bool metric = get_option<std::string>( "USE_METRIC_SPEEDS" ) == "km/h";
        // Logic below is not applicable to translated units and should be changed
        int velx    = metric ? 4 : 3; // strlen(units) + 1
        int cruisex = metric ? 9 : 8; // strlen(units) + 6

        if( !sideStyle ) {
            if( !veh->cruise_on ) {
                speedox += 2;
            }
            if( !metric ) {
                speedox++;
            }
        }

        const char *speedo = veh->cruise_on ? "%s....>...." : "%s....";
        mvwprintz( w, speedoy, speedox,        col_indf1, speedo, velocity_units( VU_VEHICLE ) );
        mvwprintz( w, speedoy, speedox + velx, col_vel,   "%4d",
                   int( convert_velocity( veh->velocity, VU_VEHICLE ) ) );
        if( veh->cruise_on ) {
            mvwprintz( w, speedoy, speedox + cruisex, c_ltgreen, "%4d",
                       int( convert_velocity( veh->cruise_velocity, VU_VEHICLE ) ) );
        }
        if( veh->velocity != 0 ) {
            const int offset_from_screen_edge = sideStyle ? 13 : 8;
            nc_color col_indc = veh->skidding ? c_red : c_green;
            int dfm = veh->face.dir() - veh->move.dir();
            wmove( w, sideStyle ? 4 : 3, getmaxx( w ) - offset_from_screen_edge );
            if( dfm == 0 ) {
                wprintz( w, col_indc, "^" );
            } else if( dfm < 0 ) {
                wprintz( w, col_indc, "<" );
            } else {
                wprintz( w, col_indc, ">" );
            }
        }

        if( sideStyle ) {
            // Make sure this is left-aligned.
            mvwprintz( w, speedoy, getmaxx( w ) - 9, c_white, "%s", _( "Stm " ) );
            print_stamina_bar( w );
        }
    } else {  // Not in vehicle
        nc_color col_str = c_white, col_dex = c_white, col_int = c_white,
                 col_per = c_white, col_spd = c_white, col_time = c_white;
        int str_bonus = get_str_bonus();
        int dex_bonus = get_dex_bonus();
        int int_bonus = get_int_bonus();
        int per_bonus = get_per_bonus();
        int spd_bonus = get_speed_bonus();
        if( str_bonus < 0 ) {
            col_str = c_red;
        }
        if( str_bonus > 0 ) {
            col_str = c_green;
        }
        if( dex_bonus  < 0 ) {
            col_dex = c_red;
        }
        if( dex_bonus  > 0 ) {
            col_dex = c_green;
        }
        if( int_bonus  < 0 ) {
            col_int = c_red;
        }
        if( int_bonus  > 0 ) {
            col_int = c_green;
        }
        if( per_bonus  < 0 ) {
            col_per = c_red;
        }
        if( per_bonus  > 0 ) {
            col_per = c_green;
        }
        if( spd_bonus < 0 ) {
            col_spd = c_red;
        }
        if( spd_bonus > 0 ) {
            col_spd = c_green;
        }

        int wx  = sideStyle ? 18 : 12;
        int wy  = sideStyle ?  0 :  3;
        int dx = sideStyle ?  0 :  7;
        int dy = sideStyle ?  1 :  0;
        mvwprintz( w, wy + dy * 0, wx + dx * 0, col_str, _( "Str %d" ), str_cur );
        mvwprintz( w, wy + dy * 1, wx + dx * 1, col_dex, _( "Dex %d" ), dex_cur );
        mvwprintz( w, wy + dy * 2, wx + dx * 2, col_int, _( "Int %d" ), int_cur );
        mvwprintz( w, wy + dy * 3, wx + dx * 3, col_per, _( "Per %d" ), per_cur );

        int spdx = sideStyle ?  0 : wx + dx * 4 + 1;
        int spdy = sideStyle ?  5 : wy + dy * 4;
        mvwprintz( w, spdy, spdx, col_spd, _( "Spd %d" ), get_speed() );
        if( this->weight_carried() > this->weight_capacity() ) {
            col_time = h_black;
        }
        if( this->volume_carried() > this->volume_capacity() ) {
            if( this->weight_carried() > this->weight_capacity() ) {
                col_time = c_dkgray_magenta;
            } else {
                col_time = c_dkgray_red;
            }
        }
        wprintz( w, col_time, " %d", movecounter );

        //~ Movement type: "walking". Max string length: one letter.
        const auto str_walk = pgettext( "movement-type", "W" );
        //~ Movement type: "running". Max string length: one letter.
        const auto str_run = pgettext( "movement-type", "R" );
        wprintz( w, c_white, " %s", move_mode == "walk" ? str_walk : str_run );
        if( sideStyle ) {
            mvwprintz( w, spdy, wx + dx * 4 - 3, c_white, _( "Stm " ) );
            print_stamina_bar( w );
        }
    }
}

bool player::has_conflicting_trait( const trait_id &flag ) const
{
    return ( has_opposite_trait( flag ) || has_lower_trait( flag ) || has_higher_trait( flag ) );
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

bool player::crossed_threshold() const
{
    for( auto &mut : my_mutations ) {
        if( mut.first->threshold ) {
            return true;
        }
    }
    return false;
}

bool player::purifiable( const trait_id &flag ) const
{
    return flag->purifiable;
}

void player::set_cat_level_rec( const trait_id &sMut )
{
    if( !has_base_trait( sMut ) ) { //Skip base traits
        const auto &mdata = sMut.obj();
        for( auto &elem : mdata.category ) {
            mutation_category_level[elem] += 8;
        }

        for( auto &i : mdata.prereqs ) {
            set_cat_level_rec( i );
        }

        for( auto &i : mdata.prereqs2 ) {
            set_cat_level_rec( i );
        }
    }
}

void player::set_highest_cat_level()
{
    mutation_category_level.clear();

    // Loop through our mutations
    for( auto &mut : my_mutations ) {
        set_cat_level_rec( mut.first );
    }
}

/// Returns the mutation category with the highest strength
std::string player::get_highest_category() const
{
    int iLevel = 0;
    std::string sMaxCat = "";

    for( const auto &elem : mutation_category_level ) {
        if( elem.second > iLevel ) {
            sMaxCat = elem.first;
            iLevel = elem.second;
        } else if( elem.second == iLevel ) {
            sMaxCat = "";  // no category on ties
        }
    }
    return sMaxCat;
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
    return random_entry( selected_dream.messages );
}

bool player::in_climate_control()
{
    bool regulated_area = false;
    // Check
    if( has_active_bionic( "bio_climate" ) ) {
        return true;
    }
    for( auto &w : worn ) {
        if( w.typeId() == "rm13_armor_on" ) {
            return true;
        }
        if( w.active && w.is_power_armor() ) {
            return true;
        }
    }
    if( int( calendar::turn ) >= next_climate_control_check ) {
        // save cpu and simulate acclimation.
        next_climate_control_check = int( calendar::turn ) + 20;
        int vpart = -1;
        vehicle *veh = g->m.veh_at( pos(), vpart );
        if( veh ) {
            regulated_area = (
                                 veh->is_inside( vpart ) &&  // Already checks for opened doors
                                 veh->total_power( true ) > 0 // Out of gas? No AC for you!
                             );  // TODO: (?) Force player to scrounge together an AC unit
        }
        // TODO: AC check for when building power is implemented
        last_climate_control_ret = regulated_area;
        if( !regulated_area ) {
            // Takes longer to cool down / warm up with AC, than it does to step outside and feel cruddy.
            next_climate_control_check += 40;
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
        item &itemit = stack->front();
        item *stack_iter = &itemit;
        if( stack_iter->has_flag( "RADIO_ACTIVATION" ) ) {
            rc_items.push_back( stack_iter );
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

bool player::has_active_optcloak() const
{
    for( auto &w : worn ) {
        if( w.active && w.typeId() == OPTICAL_CLOAK_ITEM_ID ) {
            return true;
        }
    }
    return false;
}

void player::charge_power( int amount )
{
    power_level += amount;
    if( power_level > max_power_level ) {
        power_level = max_power_level;
    }
    if( power_level < 0 ) {
        power_level = 0;
    }
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

    lumination = ( float )maxlum;

    if( lumination < 60 && has_active_bionic( "bio_flashlight" ) ) {
        lumination = 60;
    } else if( lumination < 25 && has_artifact_with( AEP_GLOW ) ) {
        lumination = 25;
    } else if( lumination < 5 && has_effect( effect_glowing ) ) {
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
    int range = int( -log( get_vision_threshold( int( g->m.ambient_light_at( pos() ) ) ) /
                           ( float )light_level ) *
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
        sight_points -= int( ter->get_see_cost() );
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
    sight = has_trait( trait_BIRD_EYE ) ? 15 : 10;
    bool has_optic = ( has_item_with_flag( "ZOOM" ) || has_bionic( "bio_eye_optic" ) );
    if( has_optic && has_trait( trait_EAGLEEYED ) ) {
        sight += 15;
    } else if( has_optic != has_trait( trait_EAGLEEYED ) ) {
        sight += 10;
    }
    return sight;
}

#define MAX_CLAIRVOYANCE 40
int player::clairvoyance() const
{
    if( vision_mode_cache[VISION_CLAIRVOYANCE_SUPER] ) {
        return MAX_CLAIRVOYANCE;
    }

    if( vision_mode_cache[VISION_CLAIRVOYANCE] ) {
        return 3;
    }

    return 0;
}

bool player::sight_impaired() const
{
    return ( ( ( has_effect( effect_boomered ) || has_effect( effect_darkness ) ) &&
               ( !( has_trait( trait_PER_SLIME_OK ) ) ) ) ||
             ( underwater && !has_bionic( "bio_membrane" ) && !has_trait( trait_MEMBRANE ) &&
               !worn_with_flag( "SWIM_GOGGLES" ) && !has_trait( trait_PER_SLIME_OK ) &&
               !has_trait( trait_CEPH_EYES ) ) ||
             ( ( has_trait( trait_MYOPIC ) || has_trait( trait_URSINE_EYE ) ) &&
               !is_wearing( "glasses_eye" ) &&
               !is_wearing( "glasses_monocle" ) &&
               !is_wearing( "glasses_bifocal" ) &&
               !has_effect( effect_contacts ) &&
               !has_bionic( "bio_eye_optic") ) ||
                has_trait( trait_PER_SLIME ) );
}

bool player::has_two_arms() const
{
    // If you've got a blaster arm, low hp arm, or you're inside a shell then you don't have two
    // arms to use.
    return !( ( has_bionic( "bio_blaster" ) || hp_cur[hp_arm_l] < 10 || hp_cur[hp_arm_r] < 10 ) ||
              has_active_mutation( trait_id( "SHELL2" ) ) );
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
    return ( has_item_with_flag( "ALARMCLOCK" ) ||
             (
                 ( g->m.veh_at( pos() ) != nullptr ) &&
                 !g->m.veh_at( pos() )->all_parts_with_feature( "ALARMCLOCK", true ).empty()
             ) ||
             has_bionic( "bio_watch" )
           );
}

bool player::has_watch() const
{
    return ( has_item_with_flag( "WATCH" ) ||
             (
                 ( g->m.veh_at( pos() ) != nullptr ) &&
                 !g->m.veh_at( pos() )->all_parts_with_feature( "WATCH", true ).empty()
             ) ||
             has_bionic( "bio_watch" )
           );
}

void player::pause()
{
    moves = 0;
    /** @EFFECT_STR increases recoil recovery speed */

    /** @EFFECT_GUN increases recoil recovery speed */
    recoil -= str_cur + 2 * get_skill_level( skill_gun );
    recoil = std::max( MIN_RECOIL * 2, recoil );
    recoil = recoil / 2;

    // Train swimming if underwater
    if( !in_vehicle ) {
        if( underwater ) {
            practice( skill_swimming, 1 );
            drench( 100, mfb( bp_leg_l ) | mfb( bp_leg_r ) | mfb( bp_torso ) | mfb( bp_arm_l ) |
                         mfb( bp_arm_r ) | mfb( bp_head ) | mfb( bp_eyes ) | mfb( bp_mouth ) |
                         mfb( bp_foot_l ) | mfb( bp_foot_r ) | mfb( bp_hand_l ) | mfb( bp_hand_r ), true );
        } else if( g->m.has_flag( TFLAG_DEEP_WATER, pos() ) ) {
            practice( skill_swimming, 1 );
            // Same as above, except no head/eyes/mouth
            drench( 100, mfb( bp_leg_l ) | mfb( bp_leg_r ) | mfb( bp_torso ) | mfb( bp_arm_l ) |
                         mfb( bp_arm_r ) | mfb( bp_foot_l ) | mfb( bp_foot_r ) | mfb( bp_hand_l ) |
                         mfb( bp_hand_r ), true );
        } else if( g->m.has_flag( "SWIMMABLE", pos() ) ) {
            drench( 40, mfb( bp_foot_l ) | mfb( bp_foot_r ) | mfb( bp_leg_l ) | mfb( bp_leg_r ), false );
        }
    }

    // Try to put out clothing/hair fire
    if( has_effect( effect_onfire ) ) {
        int total_removed = 0;
        int total_left = 0;
        bool on_ground = has_effect( effect_downed );
        for( size_t i = 0; i < num_bp; i++ ) {
            body_part bp = body_part( i );
            effect &eff = get_effect( effect_onfire, bp );
            if( eff.is_null() ) {
                continue;
            }

            // @todo Tools and skills
            total_left += eff.get_duration();
            // Being on the ground will smother the fire much faster because you can roll
            int dur_removed = on_ground ? eff.get_duration() / 2 + 2 : 1;
            eff.mod_duration( -dur_removed );
            total_removed += dur_removed;
        }

        // Don't drop on the ground when the ground is on fire
        if( total_left > 10 && !is_dangerous_fields( g->m.field_at( pos() ) ) ) {
            add_effect( effect_downed, 2, num_bp, false, 0, true );
            add_msg_player_or_npc( m_warning,
                                   _( "You roll on the ground, trying to smother the fire!" ),
                                   _( "<npcname> rolls on the ground!" ) );
        } else if( total_removed > 0 ) {
            add_msg_player_or_npc( m_warning,
                                   _( "You attempt to put out the fire on you!" ),
                                   _( "<npcname> attempts to put out the fire on them!" ) );
        }
    }

    if( is_npc() ) {
        // The stuff below doesn't apply to NPCs
        // search_surroundings should eventually do, though
        return;
    }

    VehicleList vehs = g->m.get_vehicles();
    vehicle *veh = NULL;
    for( auto &v : vehs ) {
        veh = v.v;
        if( veh && veh->velocity != 0 && veh->player_in_control( *this ) ) {
            if( one_in( 8 ) ) {
                double exp_temp = 1 + veh->total_mass() / 400.0 + std::abs( veh->velocity / 3200.0 );
                int experience = int( exp_temp );
                if( exp_temp - experience > 0 && x_in_y( exp_temp - experience, 1.0 ) ) {
                    experience++;
                }
                practice( skill_id( "driving" ), experience );
            }
            break;
        }
    }

    search_surroundings();
}

void player::shout( std::string msg )
{
    int base = 10;
    int shout_multiplier = 2;

    // Mutations make shouting louder, they also define the defualt message
    if ( has_trait( trait_SHOUT2 ) ) {
        base = 15;
        shout_multiplier = 3;
        if ( msg.empty() ) {
            msg = _("yourself scream loudly!");
        }
    }

    if ( has_trait( trait_SHOUT3 ) ) {
        shout_multiplier = 4;
        base = 20;
        if ( msg.empty() ) {
            msg = _("yourself let out a piercing howl!");
        }
    }

    if ( msg.empty() ) {
        msg = _("yourself shout loudly!");
    }
    // Masks and such dampen the sound
    // Balanced around  whisper for wearing bondage mask
    // and noise ~= 10(door smashing) for wearing dust mask for character with strength = 8
    /** @EFFECT_STR increases shouting volume */
    const int penalty = encumb( bp_mouth ) * 3 / 2;
    int noise = base + str_cur * shout_multiplier - penalty;

    // Minimum noise volume possible after all reductions.
    // Volume 1 can't be heard even by player
    constexpr int minimum_noise = 2;

    if ( noise <= base ) {
        std::string dampened_shout;
        std::transform( msg.begin(), msg.end(), std::back_inserter(dampened_shout), tolower );
        noise = std::max( minimum_noise, noise );
        msg = std::move( dampened_shout );
    }

    // Screaming underwater is not good for oxygen and harder to do overall
    if ( underwater ) {
        if ( !has_trait( trait_GILLS ) && !has_trait( trait_GILLS_CEPH ) ) {
            mod_stat( "oxygen", -noise );
        }

        noise = std::max(minimum_noise, noise / 2);
    }

    if( noise <= minimum_noise ) {
        add_msg( m_warning, _( "The sound of your voice is almost completely muffled!" ) );
        msg.clear();
    } else if( noise * 2 <= noise + penalty ) {
        // The shout's volume is 1/2 or lower of what it would be without the penalty
        add_msg( m_warning, _( "The sound of your voice is significantly muffled!" ) );
    }
    sounds::sound( pos(), noise, msg );
}

void player::toggle_move_mode()
{
    if( move_mode == "walk" ) {
        if( stamina > 0 && !has_effect( effect_winded ) ) {
            move_mode = "run";
            add_msg(_("You start running."));
        } else {
            add_msg(m_bad, _("You're too tired to run."));
        }
    } else if( move_mode == "run" ) {
        move_mode = "walk";
        add_msg(_("You slow to a walk."));
    }
}

void player::search_surroundings()
{
    if (controlling_vehicle) {
        return;
    }
    // Search for traps in a larger area than before because this is the only
    // way we can "find" traps that aren't marked as visible.
    // Detection formula takes care of likelihood of seeing within this range.
    const int z = posz();
    for( int xd = -5; xd <= 5; xd++ ) {
        for( int yd = -5; yd <= 5; yd++ ) {
            const int x = posx() + xd;
            const int y = posy() + yd;
            const trap &tr = g->m.tr_at( tripoint( x, y, z ) );
            if( tr.is_null() || (x == posx() && y == posy() ) ) {
                continue;
            }
            if( !sees( x, y ) ) {
                continue;
            }
            if( tr.name.empty() || tr.can_see( tripoint( x, y, z ), *this ) ) {
                // Already seen, or has no name -> can never be seen
                continue;
            }
            // Chance to detect traps we haven't yet seen.
            if (tr.detect_trap( tripoint( x, y, z ), *this )) {
                if( tr.get_visibility() > 0 ) {
                    // Only bug player about traps that aren't trivial to spot.
                    const std::string direction = direction_name(
                        direction_from(posx(), posy(), x, y));
                    add_msg_if_player(_("You've spotted a %1$s to the %2$s!"),
                                      tr.name.c_str(), direction.c_str());
                }
                add_known_trap( tripoint( x, y, z ), tr);
            }
        }
    }
}

int player::read_speed(bool return_stat_effect) const
{
    // Stat window shows stat effects on based on current stat
    const int intel = get_int();
    /** @EFFECT_INT increases reading speed */
    int ret = 1000 - 50 * (intel - 8);
    if( has_trait( trait_FASTREADER ) ) {
        ret *= .8;
    }

    if( has_trait( trait_SLOWREADER ) ) {
        ret *= 1.3;
    }

    if( ret < 100 ) {
        ret = 100;
    }
    // return_stat_effect actually matters here
    return (return_stat_effect ? ret : ret / 10);
}

int player::rust_rate(bool return_stat_effect) const
{
    if (get_option<std::string>( "SKILL_RUST" ) == "off") {
        return 0;
    }

    // Stat window shows stat effects on based on current stat
    int intel = get_int();
    /** @EFFECT_INT reduces skill rust */
    int ret = ((get_option<std::string>( "SKILL_RUST" ) == "vanilla" || get_option<std::string>( "SKILL_RUST" ) == "capped") ? 500 : 500 - 35 * (intel - 8));

    if (has_trait( trait_FORGETFUL )) {
        ret *= 1.33;
    }

    if (has_trait( trait_GOODMEMORY )) {
        ret *= .66;
    }

    if (ret < 0) {
        ret = 0;
    }

    // return_stat_effect actually matters here
    return (return_stat_effect ? ret : ret / 10);
}

int player::talk_skill() const
{
    /** @EFFECT_INT slightly increases talking skill */

    /** @EFFECT_PER slightly increases talking skill */

    /** @EFFECT_SPEECH increases talking skill */
    int ret = get_int() + get_per() + get_skill_level( skill_id( "speech" ) ) * 3;
    if (has_trait( trait_SAPIOVORE )) {
        ret -= 20; // Friendly convo with your prey? unlikely
    } else if (has_trait( trait_UGLY )) {
        ret -= 3;
    } else if (has_trait( trait_DEFORMED )) {
        ret -= 6;
    } else if (has_trait( trait_DEFORMED2 )) {
        ret -= 12;
    } else if (has_trait( trait_DEFORMED3 )) {
        ret -= 18;
    } else if (has_trait( trait_PRETTY )) {
        ret += 1;
    } else if (has_trait( trait_BEAUTIFUL )) {
        ret += 2;
    } else if (has_trait( trait_BEAUTIFUL2 )) {
        ret += 4;
    } else if (has_trait( trait_BEAUTIFUL3 )) {
        ret += 6;
    }
    return ret;
}

int player::intimidation() const
{
    /** @EFFECT_STR increases intimidation factor */
    int ret = get_str() * 2;
    if (weapon.is_gun()) {
        ret += 10;
    }
    if( weapon.damage_melee( DT_BASH ) >= 12 ||
        weapon.damage_melee( DT_CUT  ) >= 12 ||
        weapon.damage_melee( DT_STAB ) >= 12 ) {
        ret += 5;
    }
    if (has_trait( trait_SAPIOVORE )) {
        ret += 5; // Scaring one's prey, on the other claw...
    } else if (has_trait( trait_DEFORMED2 )) {
        ret += 3;
    } else if (has_trait( trait_DEFORMED3 )) {
        ret += 6;
    } else if (has_trait( trait_PRETTY )) {
        ret -= 1;
    } else if (has_trait( trait_BEAUTIFUL ) || has_trait( trait_BEAUTIFUL2 ) || has_trait( trait_BEAUTIFUL3 )) {
        ret -= 4;
    }
    if (stim > 20) {
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
    }

    // Even if we are not to train still call practice to prevent skill rust
    difficulty = std::max( difficulty, 0.0f );
    practice( skill_dodge, difficulty * 2, difficulty );

    ma_ondodge_effects();

    // For adjacent attackers check for techniques usable upon successful dodge
    if( source && square_dist( pos(), source->pos() ) == 1 ) {
        matec_id tec = pick_technique( *source, false, true, false );
        if( tec != tec_none ) {
            melee_attack( *source, false, tec );
        }
    }
}

void player::on_hit( Creature *source, body_part bp_hit,
                     float /*difficulty*/ , dealt_projectile_attack const* const proj ) {
    check_dead_state();
    if( source == nullptr || proj != nullptr ) {
        return;
    }

    bool u_see = g->u.sees( *this );
    if (has_active_bionic("bio_ods")) {
        if (is_player()) {
            add_msg(m_good, _("Your offensive defense system shocks %s in mid-attack!"),
                            source->disp_name().c_str());
        } else if (u_see) {
            add_msg(_("%1$s's offensive defense system shocks %2$s in mid-attack!"),
                        disp_name().c_str(),
                        source->disp_name().c_str());
        }
        damage_instance ods_shock_damage;
        ods_shock_damage.add_damage(DT_ELECTRIC, rng(10,40));
        source->deal_damage(this, bp_torso, ods_shock_damage);
    }
    if ((!(wearing_something_on(bp_hit))) && (has_trait( trait_SPINES ) || has_trait( trait_QUILLS ))) {
        int spine = rng(1, (has_trait( trait_QUILLS ) ? 20 : 8));
        if (!is_player()) {
            if( u_see ) {
                add_msg(_("%1$s's %2$s puncture %3$s in mid-attack!"), name.c_str(),
                            (has_trait( trait_QUILLS ) ? _("quills") : _("spines")),
                            source->disp_name().c_str());
            }
        } else {
            add_msg(m_good, _("Your %1$s puncture %2$s in mid-attack!"),
                            (has_trait( trait_QUILLS ) ? _("quills") : _("spines")),
                            source->disp_name().c_str());
        }
        damage_instance spine_damage;
        spine_damage.add_damage(DT_STAB, spine);
        source->deal_damage(this, bp_torso, spine_damage);
    }
    if ((!(wearing_something_on(bp_hit))) && (has_trait( trait_THORNS )) && (!(source->has_weapon()))) {
        if (!is_player()) {
            if( u_see ) {
                add_msg(_("%1$s's %2$s scrape %3$s in mid-attack!"), name.c_str(),
                            _("thorns"), source->disp_name().c_str());
            }
        } else {
            add_msg(m_good, _("Your thorns scrape %s in mid-attack!"), source->disp_name().c_str());
        }
        int thorn = rng(1, 4);
        damage_instance thorn_damage;
        thorn_damage.add_damage(DT_CUT, thorn);
        // In general, critters don't have separate limbs
        // so safer to target the torso
        source->deal_damage(this, bp_torso, thorn_damage);
    }
    if ((!(wearing_something_on(bp_hit))) && (has_trait( trait_CF_HAIR ))) {
        if (!is_player()) {
            if( u_see ) {
                add_msg(_("%1$s gets a load of %2$s's %3$s stuck in!"), source->disp_name().c_str(),
                  name.c_str(), (_("hair")));
            }
        } else {
            add_msg(m_good, _("Your hairs detach into %s!"), source->disp_name().c_str());
        }
        source->add_effect( effect_stunned, 2 );
        if (one_in(3)) { // In the eyes!
            source->add_effect( effect_blind, 2 );
        }
    }
}

void player::on_hurt( Creature *source, bool disturb /*= true*/ )
{
    if( has_trait( trait_ADRENALINE ) && !has_effect( effect_adrenaline ) &&
        (hp_cur[hp_head] < 25 || hp_cur[hp_torso] < 15) ) {
        add_effect( effect_adrenaline, 200 );
    }

    if( disturb ) {
        if( in_sleep_state() ) {
            wake_up();
        }
        if( !is_npc() ) {
            if( source != nullptr ) {
                g->cancel_activity_query(_("You were attacked by %s!"), source->disp_name().c_str());
            } else {
                g->cancel_activity_query(_("You were hurt!"));
            }
        }
    }

    if( is_dead_state() ) {
        set_killer( source );
    }
}

bool player::immune_to( body_part bp, damage_unit dam ) const
{
    if( dam.type == DT_HEAT ) {
        return false; // No one is immune to fire
    }
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

dealt_damage_instance player::deal_damage( Creature* source, body_part bp,
                                           const damage_instance& d )
{
    if( has_trait( trait_DEBUG_NODMG ) ) {
        return dealt_damage_instance();
    }

    //damage applied here
    dealt_damage_instance dealt_dams = Creature::deal_damage( source, bp, d );
    //block reduction should be by applied this point
    int dam = dealt_dams.total_damage();

    // TODO: Pre or post blit hit tile onto "this"'s location here
    if( g->u.sees( pos() ) ) {
        g->draw_hit_player( *this, dam );

        if( dam > 0 && is_player() && source ) {
            //monster hits player melee
            SCT.add( posx(), posy(),
                     direction_from( 0, 0, posx() - source->posx(), posy() - source->posy() ),
                     get_hp_bar( dam, get_hp_max( player::bp_to_hp( bp ) ) ).first, m_bad,
                     body_part_name( bp ), m_neutral );
        }
    }

    // handle snake artifacts
    if( has_artifact_with( AEP_SNAKES ) && dam >= 6 ) {
        int snakes = dam / 6;
        std::vector<tripoint> valid;
        for( int x = posx() - 1; x <= posx() + 1; x++ ) {
            for( int y = posy() - 1; y <= posy() + 1; y++ ) {
                tripoint dest( x, y, posz() );
                if( g->is_empty( dest ) ) {
                    valid.push_back( dest );
                }
            }
        }
        if( snakes > int( valid.size() ) ) {
            snakes = int( valid.size() );
        }
        if( snakes == 1 ) {
            add_msg( m_warning, _( "A snake sprouts from your body!" ) );
        } else if( snakes >= 2 ) {
            add_msg( m_warning, _( "Some snakes sprout from your body!" ) );
        }
        for( int i = 0; i < snakes && !valid.empty(); i++ ) {
            const tripoint target = random_entry_removed( valid );
            if( g->summon_mon( mon_shadow_snake, target ) ) {
                monster *snake = g->monster_at( target );
                snake->friendly = -1;
            }
        }
    }

    // And slimespawners too
    if( ( has_trait( trait_SLIMESPAWNER ) ) && ( dam >= 10 ) && one_in( 20 - dam ) ) {
        std::vector<tripoint> valid;
        for( int x = posx() - 1; x <= posx() + 1; x++ ) {
            for( int y = posy() - 1; y <= posy() + 1; y++ ) {
                tripoint dest( x, y, posz() );
                if( g->is_empty( dest ) ) {
                    valid.push_back( dest );
                }
            }
        }
        add_msg( m_warning, _( "Slime is torn from you, and moves on its own!" ) );
        int numslime = 1;
        for( int i = 0; i < numslime && !valid.empty(); i++ ) {
            const tripoint target = random_entry_removed( valid );
            if( g->summon_mon( mon_player_blob, target ) ) {
                monster *slime = g->monster_at( target );
                slime->friendly = -1;
            }
        }
    }

    //Acid blood effects.
    bool u_see = g->u.sees( *this );
    int cut_dam = dealt_dams.type_damage( DT_CUT );
    if( source && has_trait( trait_ACIDBLOOD ) && !one_in( 3 ) &&
        ( dam >= 4 || cut_dam > 0 ) && ( rl_dist( g->u.pos(), source->pos() ) <= 1) ) {
        if( is_player() ) {
            add_msg( m_good, _( "Your acidic blood splashes %s in mid-attack!" ),
                     source->disp_name().c_str() );
        } else if( u_see ) {
            add_msg( _( "%1$s's acidic blood splashes on %2$s in mid-attack!" ),
                     disp_name().c_str(), source->disp_name().c_str() );
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
                int minblind = ( dam + cut_dam ) / 10;
                if( minblind < 1 ) {
                    minblind = 1;
                }
                int maxblind = ( dam + cut_dam ) /  4;
                if( maxblind > 5 ) {
                    maxblind = 5;
                }
                add_effect( effect_blind, rng( minblind, maxblind ) );
            }
            break;
        case bp_torso:
            break;
        case bp_hand_l: // Fall through to arms
        case bp_arm_l:
            // Hit to arms/hands are really bad to our aim
            recoil_mul = 200;
            break;
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
            // @todo Some daze maybe? Move drain?
            break;
        default:
            debugmsg( "Wacky body part hit!" );
    }

    // @todo Scale with damage in a way that makes sense for power armors, plate armor and naked skin.
    recoil += recoil_mul * weapon.volume() / 250_ml;
    //looks like this should be based off of dealtdams, not d as d has no damage reduction applied.
    // Skip all this if the damage isn't from a creature. e.g. an explosion.
    if( source != nullptr ) {
        if ( source->has_flag( MF_GRABS ) && !source->is_hallucination() ) {
            /** @EFFECT_DEX increases chance to avoid being grabbed, if DEX>STR */

            /** @EFFECT_STR increases chance to avoid being grabbed, if STR>DEX */
            if( has_grab_break_tec() && get_grab_resist() > 0 &&
                ( get_dex() > get_str() ? rng( 0, get_dex() ) : rng( 0, get_str() ) ) >
                    rng( 0, 10 ) ) {
                if( has_effect( effect_grabbed ) ) {
                    add_msg_if_player( m_warning ,_("You are being grabbed by %s, but you bat it away!"),
                                       source->disp_name().c_str() );
                } else {
                    add_msg_player_or_npc( m_info, _("You are being grabbed by %s, but you break its grab!"),
                                           _("<npcname> are being grabbed by %s, but they break its grab!"),
                                           source->disp_name().c_str() );
                }
            } else {
                int prev_effect = get_effect_int( effect_grabbed );
                add_effect( effect_grabbed, 2, bp_torso, false, prev_effect + 2 );
                add_msg_player_or_npc(m_bad, _("You are grabbed by %s!"), _("<npcname> is grabbed by %s!"),
                                source->disp_name().c_str());
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
        const int cut_type_dam = dealt_dams.type_damage( DT_CUT ) + dealt_dams.type_damage( DT_STAB );
        const int combined_dam = dealt_dams.type_damage( DT_BASH ) + ( cut_type_dam * 4 );
        const int infection_chance = ( combined_dam * sum_cover ) / 100;
        if( x_in_y( infection_chance, 100 ) ) {
            if( has_effect( effect_bite, bp ) ) {
                add_effect( effect_bite, 400, bp, true );
            } else if( has_effect( effect_infected, bp ) ) {
                add_effect( effect_infected, 250, bp, true );
            } else {
                add_effect( effect_bite, 1, bp, true );
            }
            add_msg_if_player( "Filth from your clothing has implanted deep in the wound." );
        } 
    }

    on_hurt( source );
    return dealt_dams;
}

void player::mod_pain(int npain) {
    if( npain > 0 ) {
        if( has_trait( trait_NOPAIN ) ) {
            return;
        }
        if( npain > 1 ) {
            // if it's 1 it'll just become 0, which is bad
            if( has_trait( trait_PAINRESIST_TROGLO ) ) {
                npain = roll_remainder( npain * 0.5f );
            } else if( has_trait( trait_PAINRESIST ) ) {
                npain = roll_remainder( npain * 0.67f );
            }
        }
    }
    Creature::mod_pain( npain );
}

void player::set_pain(int npain)
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

void player::mod_painkiller(int npkill)
{
    set_painkiller( pkill + npkill );
}

void player::set_painkiller(int npkill)
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
        g->cancel_activity_query( _( "Ouch, something hurts!" ) );
    }
    // Only a large pain burst will actually wake people while sleeping.
    if( in_sleep_state() ) {
        int pain_thresh = rng( 3, 5 );

        if( has_trait( trait_HEAVYSLEEPER ) ) {
            pain_thresh += 2;
        } else if ( has_trait( trait_HEAVYSLEEPER2 ) ) {
            pain_thresh += 5;
        }

        if( intensity >= pain_thresh ) {
            wake_up();
        }
    }
}

/*
    Where damage to player is actually applied to hit body parts
    Might be where to put bleed stuff rather than in player::deal_damage()
 */
void player::apply_damage(Creature *source, body_part hurt, int dam)
{
    if( is_dead_state() || has_trait( trait_DEBUG_NODMG ) ) {
        // don't do any more damage if we're already dead
        // Or if we're debugging and don't want to die
        return;
    }

    hp_part hurtpart = bp_to_hp( hurt );
    if( hurtpart == num_hp_parts ) {
        debugmsg("Wacky body part hurt!");
        hurtpart = hp_torso;
    }

    mod_pain( dam / 2 );

    hp_cur[hurtpart] -= dam;
    if (hp_cur[hurtpart] < 0) {
        lifetime_stats()->damage_taken += hp_cur[hurtpart];
        hp_cur[hurtpart] = 0;
    }

    if( hp_cur[hurtpart] <= 0 ) {
        remove_effect( effect_mending, hurt );
        add_effect( effect_disabled, 1, hurt, true );
    }

    lifetime_stats()->damage_taken += dam;
    if( dam > get_painkiller() ) {
        on_hurt( source );
    }
}

void player::heal(body_part healed, int dam)
{
    hp_part healpart;
    switch (healed) {
        case bp_eyes: // Fall through to head damage
        case bp_mouth: // Fall through to head damage
        case bp_head:
            healpart = hp_head;
            break;
        case bp_torso:
            healpart = hp_torso;
            break;
        case bp_hand_l:
            // Shouldn't happen, but fall through to arms
            debugmsg("Heal against hands!");
        case bp_arm_l:
            healpart = hp_arm_l;
            break;
        case bp_hand_r:
            // Shouldn't happen, but fall through to arms
            debugmsg("Heal against hands!");
        case bp_arm_r:
            healpart = hp_arm_r;
            break;
        case bp_foot_l:
            // Shouldn't happen, but fall through to legs
            debugmsg("Heal against feet!");
        case bp_leg_l:
            healpart = hp_leg_l;
            break;
        case bp_foot_r:
            // Shouldn't happen, but fall through to legs
            debugmsg("Heal against feet!");
        case bp_leg_r:
            healpart = hp_leg_r;
            break;
        default:
            debugmsg("Wacky body part healed!");
            healpart = hp_torso;
    }
    heal( healpart, dam );
}

void player::heal(hp_part healed, int dam)
{
    if (hp_cur[healed] > 0) {
        hp_cur[healed] += dam;
        if (hp_cur[healed] > hp_max[healed]) {
            lifetime_stats()->damage_healed -= hp_cur[healed] - hp_max[healed];
            hp_cur[healed] = hp_max[healed];
        }
        lifetime_stats()->damage_healed += dam;
    }
}

void player::healall(int dam)
{
    for( int healed_part = 0; healed_part < num_hp_parts; healed_part++) {
        heal( (hp_part)healed_part, dam );
    }
}

void player::hurtall(int dam, Creature *source, bool disturb /*= true*/)
{
    if( is_dead_state() || has_trait( trait_DEBUG_NODMG ) || dam <= 0 ) {
        return;
    }

    for( int i = 0; i < num_hp_parts; i++ ) {
        const hp_part bp = static_cast<hp_part>( i );
        // Don't use apply_damage here or it will annoy the player with 6 queries
        hp_cur[bp] -= dam;
        lifetime_stats()->damage_taken += dam;
        if( hp_cur[bp] < 0 ) {
            lifetime_stats()->damage_taken += hp_cur[bp];
            hp_cur[bp] = 0;
        }
    }

    // Low pain: damage is spread all over the body, so not as painful as 6 hits in one part
    mod_pain( dam );
    on_hurt( source, disturb );
}

int player::hitall(int dam, int vary, Creature *source)
{
    int damage_taken = 0;
    for (int i = 0; i < num_hp_parts; i++) {
        const body_part bp = hp_to_bp( static_cast<hp_part>( i ) );
        int ddam = vary ? dam * rng (100 - vary, 100) / 100 : dam;
        int cut = 0;
        auto damage = damage_instance::physical(ddam, cut, 0);
        damage_taken += deal_damage( source, bp, damage ).total_damage();
    }
    return damage_taken;
}

float player::fall_damage_mod() const
{
    float ret = 1.0f;

    // Ability to land properly is 2x as important as dexterity itself
    /** @EFFECT_DEX decreases damage from falling */

    /** @EFFECT_DODGE decreases damage from falling */
    float dex_dodge = dex_cur / 2 + get_skill_level( skill_dodge );
    // Penalize for wearing heavy stuff
    dex_dodge -= ( ( ( encumb(bp_leg_l) + encumb(bp_leg_r) ) / 2 ) + ( encumb(bp_torso) / 1 ) ) / 10;
    // But prevent it from increasing damage
    dex_dodge = std::max( 0.0f, dex_dodge );
    // 100% damage at 0, 75% at 10, 50% at 20 and so on
    ret *= (100.0f - (dex_dodge * 4.0f)) / 100.0f;

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
    // Percentage arpen - armor won't help much here
    // TODO: Make cushioned items like bike helmets help more
    float armor_eff = 1.0f;

    // Being slammed against things rather than landing means we can't
    // control the impact as well
    const bool slam = p != pos();
    std::string target_name = "a swarm of bugs";
    Creature *critter = g->critter_at( p );
    int part_num = -1;
    vehicle *veh = g->m.veh_at( p, part_num );
    if( critter != this && critter != nullptr ) {
        target_name = critter->disp_name();
        // Slamming into creatures and NPCs
        // TODO: Handle spikes/horns and hard materials
        armor_eff = 0.5f; // 2x as much as with the ground
        // TODO: Modify based on something?
        mod = 1.0f;
        effective_force = force;
    } else if( veh != nullptr ) {
        // Slamming into vehicles
        // TODO: Integrate it with vehicle collision function somehow
        target_name = veh->disp_name();
        if( veh->part_with_feature( part_num, "SHARP" ) != -1 ) {
            // Now we're actually getting impaled
            cut = force; // Lots of fun
        }

        mod = slam ? 1.0f : fall_damage_mod();
        armor_eff = 0.25f; // Not much
        if( !slam && veh->part_with_feature( part_num, "ROOF" ) ) {
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
        add_msg_if_player( m_warning, _("You land on %s."), target_name.c_str() );
        return 0;
    }

    int total_dealt = 0;
    for( int i = 0; i < num_hp_parts; i++ ) {
        const body_part bp = hp_to_bp( static_cast<hp_part>( i ) );
        int bash = ( effective_force * rng(60, 100) / 100 );
        damage_instance di;
        di.add_damage( DT_BASH, bash, 0, armor_eff, mod );
        // No good way to land on sharp stuff, so here modifier == 1.0f
        di.add_damage( DT_CUT,  cut,  0, armor_eff, 1.0f );
        total_dealt += deal_damage( nullptr, bp, di ).total_damage();
    }

    if( total_dealt > 0 && is_player() ) {
        // "You slam against the dirt" is fine
        add_msg( m_bad, _("You are slammed against %s for %d damage."),
                 target_name.c_str(), total_dealt );
    } else if( slam ) {
        // Only print this line if it is a slam and not a landing
        // Non-players should only get this one: player doesn't know how much damage was dealt
        // and landing messages for each slammed creature would be too much
        add_msg_player_or_npc( m_bad,
                               _("You are slammed against %s."),
                               _("<npcname> is slammed against %s."),
                               target_name.c_str() );
    } else {
        // No landing message for NPCs
        add_msg_if_player( m_warning, _("You land on %s."), target_name.c_str() );
    }

    if( x_in_y( mod, 1.0f ) ) {
        add_effect( effect_downed, 1 + rng( 0, int(mod * 3) ) );
    }

    return total_dealt;
}

void player::knock_back_from( const tripoint &p )
{
    if( p == pos() ) {
        return;
    }

    tripoint to = pos();
    const tripoint dp = pos() - p;
    to.x += sgn( dp.x );
    to.y += sgn( dp.y );

    // First, see if we hit a monster
    int mondex = g->mon_at( to );
    if (mondex != -1) {
        monster *critter = &(g->zombie(mondex));
        deal_damage( critter, bp_torso, damage_instance( DT_BASH, critter->type->size ) );
        add_effect( effect_stunned, 1 );
        /** @EFFECT_STR_MAX allows knocked back player to knock back, damage, stun some monsters */
        if ((str_max - 6) / 4 > critter->type->size) {
            critter->knock_back_from(pos()); // Chain reaction!
            critter->apply_damage( this, bp_torso, (str_max - 6) / 4);
            critter->add_effect( effect_stunned, 1 );
        } else if ((str_max - 6) / 4 == critter->type->size) {
            critter->apply_damage( this, bp_torso, (str_max - 6) / 4);
            critter->add_effect( effect_stunned, 1 );
        }
        critter->check_dead_state();

        add_msg_player_or_npc(_("You bounce off a %s!"), _("<npcname> bounces off a %s!"),
                              critter->name().c_str() );
        return;
    }

    int npcdex = g->npc_at( to );
    if (npcdex != -1) {
        npc *np = g->active_npc[npcdex];
        deal_damage( np, bp_torso, damage_instance( DT_BASH, np->get_size() ) );
        add_effect( effect_stunned, 1 );
        np->deal_damage( this, bp_torso, damage_instance( DT_BASH, 3 ) );
        add_msg_player_or_npc( _("You bounce off %s!"), _("<npcname> bounces off %s!"), np->name.c_str() );
        np->check_dead_state();
        return;
    }

    // If we're still in the function at this point, we're actually moving a tile!
    if (g->m.has_flag( "LIQUID", to ) && g->m.has_flag( TFLAG_DEEP_WATER, to )) {
        if (!is_npc()) {
            g->plswim( to );
        }
        // TODO: NPCs can't swim!
    } else if (g->m.impassable( to )) { // Wait, it's a wall

        // It's some kind of wall.
        apply_damage( nullptr, bp_torso, 3 ); // TODO: who knocked us back? Maybe that creature should be the source of the damage?
        add_effect( effect_stunned, 2 );
        add_msg_player_or_npc( _("You bounce off a %s!"), _("<npcname> bounces off a %s!"),
                               g->m.obstacle_name( to ).c_str() );

    } else { // It's no wall
        setpos( to );
    }
}

int player::hp_percentage() const
{
    int total_cur = 0, total_max = 0;
    // Head and torso HP are weighted 3x and 2x, respectively
    total_cur = hp_cur[hp_head] * 3 + hp_cur[hp_torso] * 2;
    total_max = hp_max[hp_head] * 3 + hp_max[hp_torso] * 2;
    for (int i = hp_arm_l; i < num_hp_parts; i++) {
        total_cur += hp_cur[i];
        total_max += hp_max[i];
    }

    return (100 * total_cur) / total_max;
}

// Returns the number of multiples of tick_length we would "pass" on our way `from` to `to`
// For example, if `tick_length` is 1 hour, then going from 0:59 to 1:01 should return 1
inline int ticks_between( int from, int to, int tick_length )
{
    return (to / tick_length) - (from / tick_length);
}

void player::update_body()
{
    const int now = calendar::turn;
    update_body( now - 1, now );
}

void player::update_body( int from, int to )
{
    update_stamina( to - from );
    const int five_mins = ticks_between( from, to, MINUTES(5) );
    if( five_mins > 0 ) {
        check_needs_extremes();
        update_needs( five_mins );
        regen( five_mins );
        // Note: mend ticks once per 5 mins, but wants rate in TURNS, not 5 min intervals
        mend( five_mins * MINUTES( 5 ) );
    }

    const int thirty_mins = ticks_between( from, to, MINUTES(30) );
    if( thirty_mins > 0 ) {
        get_sick();
    }

    for( const auto& v : vitamin::all() ) {
        int rate = vitamin_rate( v.first );
        if( rate > 0 ) {
            int qty = ticks_between( from, to, rate );
            if( qty > 0 ) {
                vitamin_mod( v.first, 0 - qty );
            }

        } else if ( rate < 0 ) {
            // mutations can result in vitamins being generated (but never accumulated)
            int qty = ticks_between( from, to, std::abs( rate ) );
            if( qty > 0 ) {
                vitamin_mod( v.first, qty );
            }
        }
    }

    if( ticks_between( from, to, HOURS(6) ) ) {
        // Radiation kills health even at low doses
        update_health( has_trait( trait_RADIOGENIC ) ? 0 : -radiation );
    }
}

void player::update_vitamins( const vitamin_id& vit )
{
    if( is_npc() ) {
        return; // NPCs cannot develop vitamin diseases
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
        if( has_effect( def, num_bp ) ) {
            get_effect( def, num_bp ).set_intensity( lvl, true );
        } else {
            add_effect( def, 1, num_bp, true, lvl );
        }
    }
    if( lvl < 0 ) {
        if( has_effect( exc, num_bp ) ) {
            get_effect( exc, num_bp ).set_intensity( lvl, true );
        } else {
            add_effect( exc, 1, num_bp, true, lvl );
        }
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
    if (has_trait( trait_DISRESISTANT )) {
        // Disease resistant people only get sick once a year.
        base_diseases_per_year = 1;
    }

    // This check runs once every 30 minutes, so double to get hours, *24 to get days.
    const int checks_per_year = 2 * 24 * 365;

    // Health is in the range [-200,200].
    // Diseases are half as common for every 50 health you gain.
    float health_factor = std::pow(2.0f, get_healthy() / 50.0f);

    int disease_rarity = (int) (checks_per_year * health_factor / base_diseases_per_year);
    add_msg( m_debug, "disease_rarity = %d", disease_rarity);
    if (one_in(disease_rarity)) {
        if (one_in(6)) {
            // The flu typically lasts 3-10 days.
            const int short_flu = DAYS(3);
            const int long_flu = DAYS(10);
            const int duration = rng(short_flu, long_flu);
            add_env_effect( effect_flu, bp_mouth, 3, duration );
        } else {
            // A cold typically lasts 1-14 days.
            int short_cold = DAYS(1);
            int long_cold = DAYS(14);
            int duration = rng(short_cold, long_cold);
            add_env_effect( effect_common_cold, bp_mouth, 3, duration );
        }
    }
}

void player::update_health(int external_modifiers)
{
    if( has_artifact_with( AEP_SICK ) ) {
        // Carrying a sickness artifact makes your health 50 points worse on average
        external_modifiers -= 50;
    }
    Character::update_health( external_modifiers );
}

void player::check_needs_extremes()
{
    // Check if we've overdosed... in any deadly way.
    if (stim > 250) {
        add_msg_if_player(m_bad, _("You have a sudden heart attack!"));
        add_memorial_log(pgettext("memorial_male", "Died of a drug overdose."),
                           pgettext("memorial_female", "Died of a drug overdose."));
        hp_cur[hp_torso] = 0;
    } else if (stim < -200 || get_painkiller() > 240) {
        add_msg_if_player(m_bad, _("Your breathing stops completely."));
        add_memorial_log(pgettext("memorial_male", "Died of a drug overdose."),
                           pgettext("memorial_female", "Died of a drug overdose."));
        hp_cur[hp_torso] = 0;
    } else if( has_effect( effect_jetinjector ) && get_effect_dur( effect_jetinjector ) > 400 ) {
        if (!(has_trait( trait_NOPAIN ))) {
            add_msg_if_player(m_bad, _("Your heart spasms painfully and stops."));
        } else {
            add_msg_if_player(_("Your heart spasms and stops."));
        }
        add_memorial_log(pgettext("memorial_male", "Died of a healing stimulant overdose."),
                           pgettext("memorial_female", "Died of a healing stimulant overdose."));
        hp_cur[hp_torso] = 0;
    } else if( get_effect_dur( effect_adrenaline ) > 500 ) {
        add_msg_if_player( m_bad, _("Your heart spasms and stops.") );
        add_memorial_log( pgettext("memorial_male", "Died of adrenaline overdose."),
                          pgettext("memorial_female", "Died of adrenaline overdose.") );
        hp_cur[hp_torso] = 0;
    }

    // Check if we're starving or have starved
    if( is_player() && get_hunger() >= 3000 ) {
        if (get_hunger() >= 6000) {
            add_msg_if_player(m_bad, _("You have starved to death."));
            add_memorial_log(pgettext("memorial_male", "Died of starvation."),
                               pgettext("memorial_female", "Died of starvation."));
            hp_cur[hp_torso] = 0;
        } else if( get_hunger() >= 5000 && calendar::once_every(HOURS(1)) ) {
            add_msg_if_player(m_warning, _("Food..."));
        } else if( get_hunger() >= 4000 && calendar::once_every(HOURS(1)) ) {
            add_msg_if_player(m_warning, _("You are STARVING!"));
        } else if( calendar::once_every(HOURS(1)) ) {
            add_msg_if_player(m_warning, _("Your stomach feels so empty..."));
        }
    }

    // Check if we're dying of thirst
    if( is_player() && get_thirst() >= 600 ) {
        if( get_thirst() >= 1200 ) {
            add_msg_if_player(m_bad, _("You have died of dehydration."));
            add_memorial_log(pgettext("memorial_male", "Died of thirst."),
                               pgettext("memorial_female", "Died of thirst."));
            hp_cur[hp_torso] = 0;
        } else if( get_thirst() >= 1000 && calendar::once_every(MINUTES(30)) ) {
            add_msg_if_player(m_warning, _("Even your eyes feel dry..."));
        } else if( get_thirst() >= 800 && calendar::once_every(MINUTES(30)) ) {
            add_msg_if_player(m_warning, _("You are THIRSTY!"));
        } else if( calendar::once_every(MINUTES(30)) ) {
            add_msg_if_player(m_warning, _("Your mouth feels so dry..."));
        }
    }

    // Check if we're falling asleep, unless we're sleeping
    if( get_fatigue() >= EXHAUSTED + 25 && !in_sleep_state() ) {
        if( get_fatigue() >= MASSIVE_FATIGUE ) {
            add_msg_if_player(m_bad, _("Survivor sleep now."));
            add_memorial_log(pgettext("memorial_male", "Succumbed to lack of sleep."),
                               pgettext("memorial_female", "Succumbed to lack of sleep."));
            mod_fatigue(-10);
            try_to_sleep();
        } else if( get_fatigue() >= 800 && calendar::once_every(MINUTES(30)) ) {
            add_msg_if_player(m_warning, _("Anywhere would be a good place to sleep..."));
        } else if( calendar::once_every(MINUTES(30)) ) {
            add_msg_if_player(m_warning, _("You feel like you haven't slept in days."));
        }
    }

    // Even if we're not Exhausted, we really should be feeling lack/sleep earlier
    // Penalties start at Dead Tired and go from there
    if( get_fatigue() >= DEAD_TIRED && !in_sleep_state() ) {
        if( get_fatigue() >= 700 ) {
            if( calendar::once_every(MINUTES(30)) ) {
                add_msg_if_player( m_warning, _("You're too tired to stop yawning.") );
                add_effect( effect_lack_sleep, MINUTES(30) + 1 );
            }
            /** @EFFECT_INT slightly decreases occurrence of short naps when dead tired */
            if( one_in(50 + int_cur) ) {
                // Rivet's idea: look out for microsleeps!
                fall_asleep(5);
            }
        } else if( get_fatigue() >= EXHAUSTED ) {
            if( calendar::once_every( MINUTES( 30 ) ) ) {
                add_msg_if_player( m_warning, _("How much longer until bedtime?") );
                add_effect( effect_lack_sleep, MINUTES( 30 ) + 1 );
            }
            /** @EFFECT_INT slightly decreases occurrence of short naps when exhausted */
            if (one_in(100 + int_cur)) {
                fall_asleep(5);
            }
        } else if( get_fatigue() >= DEAD_TIRED && calendar::once_every( MINUTES( 30 ) ) ) {
            add_msg_if_player( m_warning, _("*yawn* You should really get some sleep.") );
            add_effect( effect_lack_sleep, MINUTES( 30 ) + 1 );
        }
    }
}

void player::update_needs( int rate_multiplier )
{
    // Hunger, thirst, & fatigue up every 5 minutes
    effect &sleep = get_effect( effect_sleep );
    // No food/thirst/fatigue clock at all
    const bool debug_ls = has_trait( trait_DEBUG_LS );
    // No food/thirst, capped fatigue clock (only up to tired)
    const bool npc_no_food = is_npc() && get_option<bool>( "NO_NPC_FOOD" );
    const bool foodless = debug_ls || npc_no_food;
    const bool has_recycler = has_bionic("bio_recycler");
    const bool asleep = !sleep.is_null();
    const bool lying = asleep || has_effect( effect_lying_down );
    const bool hibernating = asleep && is_hibernating();
    float hunger_rate = metabolic_rate();
    add_msg_if_player( m_debug, "Metabolic rate: %.2f", hunger_rate );

    float thirst_rate = 1.0f + mutation_value( "thirst_modifier" );
    if( is_wearing("stillsuit") ) {
        thirst_rate -= 0.3f;
    }

    // Note: intentionally not in metabolic rate
    if( has_recycler ) {
        hunger_rate *= 0.5f;
        thirst_rate = std::min( thirst_rate, std::max( 0.5f, thirst_rate * 0.5f ) );
    }

    if( asleep && !hibernating ) {
        // Hunger and thirst advance more slowly while we sleep. This is the standard rate.
        hunger_rate *= 0.5f;
        thirst_rate *= 0.5f;
    } else if( asleep && hibernating ) {
        // Hunger and thirst advance *much* more slowly whilst we hibernate.
        hunger_rate *= (2.0f / 7.0f);
        thirst_rate *= (2.0f / 7.0f);
    }

    if( is_npc() ) {
        hunger_rate *= 0.25f;
        thirst_rate *= 0.25f;
    }

    if( !foodless && hunger_rate > 0.0f ) {
        const int rolled_hunger = divide_roll_remainder( hunger_rate * rate_multiplier, 1.0 );
        mod_hunger( rolled_hunger );
    }

    if( !foodless && thirst_rate > 0.0f ) {
        mod_thirst( divide_roll_remainder( thirst_rate * rate_multiplier, 1.0 ) );
    }

    const bool wasnt_fatigued = get_fatigue() <= DEAD_TIRED;
    // Don't increase fatigue if sleeping or trying to sleep or if we're at the cap.
    if( get_fatigue() < 1050 && !asleep && !debug_ls ) {
        float fatigue_rate = 1.0f + mutation_value( "fatigue_modifier" );

        if( !debug_ls && fatigue_rate > 0.0f ) {
            mod_fatigue( divide_roll_remainder( fatigue_rate * rate_multiplier, 1.0 ) );
            if( npc_no_food && get_fatigue() > TIRED ) {
                set_fatigue( TIRED );
            }
        }
    } else if( asleep ) {
        float recovery_rate = 1.0f + mutation_value( "fatigue_regen_modifier" );
        if( !hibernating ) {
            const int intense = sleep.is_null() ? 0 : sleep.get_intensity();
            // Accelerated recovery capped to 2x over 2 hours
            // After 16 hours of activity, equal to 7.25 hours of rest
            const int accelerated_recovery_chance = 24 - intense + 1;
            const float accelerated_recovery_rate = 1.0f / accelerated_recovery_chance;
            recovery_rate += accelerated_recovery_rate;
        }

        // Untreated pain causes a flat penalty to fatigue reduction
        recovery_rate -= float(get_perceived_pain()) / 60;

        if( recovery_rate > 0.0f ) {
            int recovered = divide_roll_remainder( recovery_rate * rate_multiplier, 1.0 );
            if( get_fatigue() - recovered < -20 ) {
                // Should be wake up, but that could prevent some retroactive regeneration
                sleep.set_duration( 1 );
                mod_fatigue(-25);
            } else {
                mod_fatigue(-recovered);
            }
        }
    }
    if( is_player() && wasnt_fatigued && get_fatigue() > DEAD_TIRED && !lying ) {
        if( !activity ) {
            add_msg_if_player(m_warning, _("You're feeling tired.  %s to lie down for sleep."),
                press_x(ACTION_SLEEP).c_str());
        } else {
            g->cancel_activity_query(_("You're feeling tired."));
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

    if( has_bionic("bio_solar") && g->is_in_sunlight( pos() ) ) {
        charge_power( rate_multiplier * 25 );
    }

    // Huge folks take penalties for cramming themselves in vehicles
    if( in_vehicle && (has_trait( trait_HUGE ) || has_trait( trait_HUGE_OK )) ) {
        // TODO: Make NPCs complain
        add_msg_if_player(m_bad, _("You're cramping up from stuffing yourself in this vehicle."));
        mod_pain_noresist( 2 * rng(2, 3) );
        focus_pool -= 1;
    }

    int dec_stom_food = int(get_stomach_food() * 0.2);
    int dec_stom_water = int(get_stomach_water() * 0.2);
    dec_stom_food = dec_stom_food < 10 ? 10 : dec_stom_food;
    dec_stom_water = dec_stom_water < 10 ? 10 : dec_stom_water;
    mod_stomach_food(-dec_stom_food);
    mod_stomach_water(-dec_stom_water);
}

void player::regen( int rate_multiplier )
{
    int pain_ticks = rate_multiplier;
    while( get_pain() > 0 && pain_ticks-- > 0 ) {
        mod_pain( -roll_remainder( 0.2f + get_pain() / 50.0f ) );
    }

    float rest = rest_quality();
    float heal_rate = healing_rate( rest ) * MINUTES(5);
    if( heal_rate > 0.0f ) {
        healall( roll_remainder( rate_multiplier * heal_rate ) );
    } else if( heal_rate < 0.0f ) {
        int rot_rate = roll_remainder( rate_multiplier * -heal_rate );
        // Has to be in loop because some effects depend on rounding
        while( rot_rate-- > 0 ) {
            hurtall( 1, nullptr, false );
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
    if( !has_effect( effect_winded ) ) {
        // But mouth encumberance interferes.
        stamina_recovery += std::max( 1.0f, 10.0f - ( encumb( bp_mouth ) / 10.0f ) );
        // TODO: recovering stamina causes hunger/thirst/fatigue.
        // TODO: Tiredness slowing recovery
    }

    // stim recovers stamina (or impairs recovery)
    if( stim > 0 ) {
        // TODO: Make stamina recovery with stims cost health
        stamina_recovery += std::min( 5.0f, stim / 20.0f );
    } else if( stim < 0 ) {
        // Affect it less near 0 and more near full
        // Stamina maxes out around 1000, stims kill at -200
        // At -100 stim fully counters regular regeneration at 500 stamina,
        // halves at 250 stamina, cuts by 25% at 125 stamina
        // At -50 stim fully counters at 1000, halves at 500
        stamina_recovery += stim * stamina / 1000.0f / 5.0f;
    }

    const int max_stam = get_stamina_max();
    if( power_level >= 3 && has_active_bionic( "bio_gills" ) ) {
        int bonus = std::min<int>( power_level / 3, max_stam - stamina - stamina_recovery * turns );
        bonus = std::min( bonus, 3 );
        if( bonus > 0 ) {
            charge_power( -3 * bonus );
            stamina_recovery += bonus;
        }
    }

    stamina = roll_remainder( stamina + stamina_recovery * turns );

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
    return has_effect( effect_sleep ) && get_hunger() <= -60 && get_thirst() <= 80 && has_active_mutation( trait_id( "HIBERNATE" ) );
}

void player::add_addiction(add_type type, int strength)
{
    if( type == ADD_NULL ) {
        return;
    }
    int timer = HOURS( 2 );
    if( has_trait( trait_ADDICTIVE ) ) {
        strength *= 2;
        timer = HOURS( 1 );
    } else if( has_trait( trait_NONADDICTIVE ) ) {
        strength /= 2;
        timer = HOURS( 6 );
    }
    //Update existing addiction
    for( auto &i : addictions ) {
        if( i.type != type ) {
            continue;
        }

        if( i.sated < 0 ) {
            i.sated = timer;
        } else if( i.sated < MINUTES(10) ) {
            i.sated += timer; // TODO: Make this variable?
        } else {
            i.sated += timer / 2;
        }
        if( i.intensity < MAX_ADDICTION_LEVEL && strength > i.intensity * rng( 2, 5 ) ) {
            i.intensity++;
        }

        add_msg( m_debug, "Updating addiction: %d intensity, %d sated",
                 i.intensity, i.sated );

        return;
    }

    // Add a new addiction
    const int roll = rng( 0, 100 );
    add_msg( m_debug, "Addiction: roll %d vs strength %d", roll, strength );
    if( roll < strength ) {
        //~ %s is addiction name
        const std::string &type_name = addiction_type_name( type );
        add_memorial_log( pgettext("memorial_male", "Became addicted to %s."),
                          pgettext("memorial_female", "Became addicted to %s."),
                          type_name.c_str() );
        add_msg( m_debug, "%s got addicted to %s", disp_name().c_str(), type_name.c_str() );
        addictions.emplace_back( type, 1 );
    }
}

bool player::has_addiction(add_type type) const
{
    return std::any_of( addictions.begin(), addictions.end(),
        [type]( const addiction &ad ) {
        return ad.type == type && ad.intensity >= MIN_ADDICTION_LEVEL;
    } );
}

void player::rem_addiction( add_type type )
{
    auto iter = std::find_if( addictions.begin(), addictions.end(),
        [type]( const addiction &ad ) {
        return ad.type == type;
    } );

    if( iter != addictions.end() ) {
        //~ %s is addiction name
        add_memorial_log(pgettext("memorial_male", "Overcame addiction to %s."),
                    pgettext("memorial_female", "Overcame addiction to %s."),
                    addiction_type_name(type).c_str());
        addictions.erase( iter );
    }
}

int player::addiction_level( add_type type ) const
{
    auto iter = std::find_if( addictions.begin(), addictions.end(),
        [type]( const addiction &ad ) {
        return ad.type == type;
    } );
    return iter != addictions.end() ? iter->intensity : 0;
}

void player::siphon( vehicle &veh, const itype_id &type )
{
    auto qty = veh.fuel_left( type );
    if( qty <= 0 ) {
        add_msg( m_bad, _( "There is not enough %s left to siphon it." ), item::nname( type ).c_str() );
        return;
    }

    item liquid( type, calendar::turn, qty );
    if( g->handle_liquid( liquid, nullptr, 1, nullptr, &veh ) ) {
        veh.drain( type, qty - liquid.charges );
    }
}

void player::cough(bool harmful, int loudness)
{
    if( harmful ) {
        const int stam = stamina;
        mod_stat( "stamina", -100 );
        if( stam < 100 && x_in_y( 100 - stam, 100 ) ) {
            apply_damage( nullptr, bp_torso, 1 );
        }
    }

    if( has_effect( effect_cough_suppress ) ) {
        return;
    }

    if( !is_npc() ) {
        add_msg(m_bad, _("You cough heavily."));
        sounds::sound(pos(), loudness, "");
    } else {
        sounds::sound(pos(), loudness, _("a hacking cough."));
    }

    moves -= 80;

    if( has_effect( effect_sleep ) && ((harmful && one_in(3)) || one_in(10)) ) {
        wake_up();
    }
}

void player::add_pain_msg(int val, body_part bp) const
{
    if (has_trait( trait_NOPAIN )) {
        return;
    }
    if (bp == num_bp) {
        if (val > 20) {
            add_msg_if_player(_("Your body is wracked with excruciating pain!"));
        } else if (val > 10) {
            add_msg_if_player(_("Your body is wracked with terrible pain!"));
        } else if (val > 5) {
            add_msg_if_player(_("Your body is wracked with pain!"));
        } else if (val > 1) {
            add_msg_if_player(_("Your body pains you!"));
        } else {
            add_msg_if_player(_("Your body aches."));
        }
    } else {
        if (val > 20) {
            add_msg_if_player(_("Your %s is wracked with excruciating pain!"),
                                body_part_name_accusative(bp).c_str());
        } else if (val > 10) {
            add_msg_if_player(_("Your %s is wracked with terrible pain!"),
                                body_part_name_accusative(bp).c_str());
        } else if (val > 5) {
            add_msg_if_player(_("Your %s is wracked with pain!"),
                                body_part_name_accusative(bp).c_str());
        } else if (val > 1) {
            add_msg_if_player(_("Your %s pains you!"),
                                body_part_name_accusative(bp).c_str());
        } else {
            add_msg_if_player(_("Your %s aches."),
                                body_part_name_accusative(bp).c_str());
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

    static const std::map<int, std::string> msg_categories = {{
        { -100, "health_horrible" },
        { -50, "health_very_bad" },
        { -10, "health_bad" },
        { 10, "" },
        { 50, "health_good" },
        { 100, "health_very_good" },
        { INT_MAX, "health_great" },
    }};

    auto iter = msg_categories.lower_bound( current_health );
    if( iter != msg_categories.end() && !iter->second.empty() ) {
        const std::string &msg = SNIPPET.random_from_category( iter->second );
        add_msg_if_player( current_health > 0 ? m_good : m_bad, msg.c_str() );
    }
}

void player::add_eff_effects(effect e, bool reduced)
{
    body_part bp = e.get_bp();
    // Add hurt
    if (e.get_amount("HURT", reduced) > 0) {
        if (bp == num_bp) {
            add_msg_if_player(_("Your %s hurts!"), body_part_name_accusative(bp_torso).c_str());
            apply_damage(nullptr, bp_torso, e.get_mod("HURT"));
        } else {
            add_msg_if_player(_("Your %s hurts!"), body_part_name_accusative(bp).c_str());
            apply_damage(nullptr, bp, e.get_mod("HURT"));
        }
    }
    // Add sleep
    if (e.get_amount("SLEEP", reduced) > 0) {
        add_msg_if_player(_("You pass out!"));
        fall_asleep(e.get_amount("SLEEP"));
    }
    // Add pkill
    if (e.get_amount("PKILL", reduced) > 0) {
        mod_painkiller(bound_mod_to_vals(pkill, e.get_amount("PKILL", reduced),
                        e.get_max_val("PKILL", reduced), 0));
    }

    // Add radiation
    if (e.get_amount("RAD", reduced) > 0) {
        radiation += bound_mod_to_vals(radiation, e.get_amount("RAD", reduced),
                        e.get_max_val("RAD", reduced), 0);
    }
    // Add health mod
    if (e.get_amount("H_MOD", reduced) > 0) {
        int bounded = bound_mod_to_vals(get_healthy_mod(), e.get_amount("H_MOD", reduced),
                e.get_max_val("H_MOD", reduced), e.get_min_val("H_MOD", reduced));
        // This already applies bounds, so we pass them through.
        mod_healthy_mod(bounded, get_healthy_mod() + bounded);
    }
    // Add health
    if (e.get_amount("HEALTH", reduced) > 0) {
        mod_healthy(bound_mod_to_vals(get_healthy(), e.get_amount("HEALTH", reduced),
                        e.get_max_val("HEALTH", reduced), e.get_min_val("HEALTH", reduced)));
    }
    // Add stim
    if (e.get_amount("STIM", reduced) > 0) {
        stim += bound_mod_to_vals(stim, e.get_amount("STIM", reduced),
                        e.get_max_val("STIM", reduced), e.get_min_val("STIM", reduced));
    }
    // Add hunger
    if (e.get_amount("HUNGER", reduced) > 0) {
        mod_hunger(bound_mod_to_vals(get_hunger(), e.get_amount("HUNGER", reduced),
                        e.get_max_val("HUNGER", reduced), e.get_min_val("HUNGER", reduced)));
    }
    // Add thirst
    if (e.get_amount("THIRST", reduced) > 0) {
        mod_thirst(bound_mod_to_vals(get_thirst(), e.get_amount("THIRST", reduced),
                        e.get_max_val("THIRST", reduced), e.get_min_val("THIRST", reduced)));
    }
    // Add fatigue
    if (e.get_amount("FATIGUE", reduced) > 0) {
        mod_fatigue(bound_mod_to_vals(get_fatigue(), e.get_amount("FATIGUE", reduced),
                        e.get_max_val("FATIGUE", reduced), e.get_min_val("FATIGUE", reduced)));
    }
    // Add pain
    if (e.get_amount("PAIN", reduced) > 0) {
        int pain_inc = bound_mod_to_vals(get_pain(), e.get_amount("PAIN", reduced),
                        e.get_max_val("PAIN", reduced), 0);
        mod_pain(pain_inc);
        if (pain_inc > 0) {
            add_pain_msg(e.get_amount("PAIN", reduced), bp);
        }
    }
    Creature::add_eff_effects(e, reduced);
}

void player::process_effects() {
    //Special Removals
    if (has_effect( effect_darkness ) && g->is_in_sunlight(pos())) {
        remove_effect( effect_darkness );
    }
    if (has_trait( trait_M_IMMUNE ) && has_effect( effect_fungus )) {
        vomit();
        remove_effect( effect_fungus );
        add_msg_if_player(m_bad,  _("We have mistakenly colonized a local guide!  Purging now."));
    }
    if (has_trait( trait_PARAIMMUNE ) && (has_effect( effect_dermatik ) || has_effect( effect_tapeworm ) ||
          has_effect( effect_bloodworms ) || has_effect( effect_brainworms ) || has_effect( effect_paincysts )) ) {
        remove_effect( effect_dermatik );
        remove_effect( effect_tapeworm );
        remove_effect( effect_bloodworms );
        remove_effect( effect_brainworms );
        remove_effect( effect_paincysts );
        add_msg_if_player(m_good, _("Something writhes and inside of you as it dies."));
    }
    if (has_trait( trait_ACIDBLOOD ) && (has_effect( effect_dermatik ) || has_effect( effect_bloodworms ) ||
          has_effect( effect_brainworms ))) {
        remove_effect( effect_dermatik );
        remove_effect( effect_bloodworms );
        remove_effect( effect_brainworms );
    }
    if (has_trait( trait_EATHEALTH ) && has_effect( effect_tapeworm ) ) {
        remove_effect( effect_tapeworm );
        add_msg_if_player(m_good, _("Your bowels gurgle as something inside them dies."));
    }
    if (has_trait( trait_INFIMMUNE ) && (has_effect( effect_bite ) || has_effect( effect_infected ) ||
          has_effect( effect_recover ) ) ) {
        remove_effect( effect_bite );
        remove_effect( effect_infected );
        remove_effect( effect_recover );
    }
    if (!(in_sleep_state()) && has_effect( effect_alarm_clock ) ) {
        remove_effect( effect_alarm_clock );
    }

    //Human only effects
    for( auto &elem : effects ) {
        for( auto &_effect_it : elem.second ) {
            auto &it = _effect_it.second;
            bool reduced = resists_effect(it);
            double mod = 1;
            body_part bp = it.get_bp();
            int val = 0;

            // Still hardcoded stuff, do this first since some modify their other traits
            hardcoded_effects(it);

            // Handle miss messages
            auto msgs = it.get_miss_msgs();
            if (!msgs.empty()) {
                for (auto i : msgs) {
                    add_miss_reason(_(i.first.c_str()), unsigned(i.second));
                }
            }

            // Handle health mod
            val = it.get_mod("H_MOD", reduced);
            if (val != 0) {
                mod = 1;
                if(it.activated(calendar::turn, "H_MOD", val, reduced, mod)) {
                    int bounded = bound_mod_to_vals(
                            get_healthy_mod(), val, it.get_max_val("H_MOD", reduced),
                            it.get_min_val("H_MOD", reduced));
                    // This already applies bounds, so we pass them through.
                    mod_healthy_mod(bounded, get_healthy_mod() + bounded);
                }
            }

            // Handle health
            val = it.get_mod("HEALTH", reduced);
            if (val != 0) {
                mod = 1;
                if(it.activated(calendar::turn, "HEALTH", val, reduced, mod)) {
                    mod_healthy(bound_mod_to_vals(get_healthy(), val,
                                it.get_max_val("HEALTH", reduced), it.get_min_val("HEALTH", reduced)));
                }
            }

            // Handle stim
            val = it.get_mod("STIM", reduced);
            if (val != 0) {
                mod = 1;
                if(it.activated(calendar::turn, "STIM", val, reduced, mod)) {
                    stim += bound_mod_to_vals(stim, val, it.get_max_val("STIM", reduced),
                                                it.get_min_val("STIM", reduced));
                }
            }

            // Handle hunger
            val = it.get_mod("HUNGER", reduced);
            if (val != 0) {
                mod = 1;
                if(it.activated(calendar::turn, "HUNGER", val, reduced, mod)) {
                    mod_hunger(bound_mod_to_vals(get_hunger(), val, it.get_max_val("HUNGER", reduced),
                                                it.get_min_val("HUNGER", reduced)));
                }
            }

            // Handle thirst
            val = it.get_mod("THIRST", reduced);
            if (val != 0) {
                mod = 1;
                if(it.activated(calendar::turn, "THIRST", val, reduced, mod)) {
                    mod_thirst(bound_mod_to_vals(get_thirst(), val, it.get_max_val("THIRST", reduced),
                                                it.get_min_val("THIRST", reduced)));
                }
            }

            // Handle fatigue
            val = it.get_mod("FATIGUE", reduced);
            // Prevent ongoing fatigue effects while asleep.
            // These are meant to change how fast you get tired, not how long you sleep.
            if (val != 0 && !in_sleep_state()) {
                mod = 1;
                if(it.activated(calendar::turn, "FATIGUE", val, reduced, mod)) {
                    mod_fatigue(bound_mod_to_vals(get_fatigue(), val, it.get_max_val("FATIGUE", reduced),
                                                it.get_min_val("FATIGUE", reduced)));
                }
            }

            // Handle Radiation
            val = it.get_mod("RAD", reduced);
            if (val != 0) {
                mod = 1;
                if(it.activated(calendar::turn, "RAD", val, reduced, mod)) {
                    radiation += bound_mod_to_vals(radiation, val, it.get_max_val("RAD", reduced), 0);
                    // Radiation can't go negative
                    if (radiation < 0) {
                        radiation = 0;
                    }
                }
            }

            // Handle Pain
            val = it.get_mod("PAIN", reduced);
            if (val != 0) {
                mod = 1;
                if (it.get_sizing("PAIN")) {
                    if (has_trait( trait_FAT )) {
                        mod *= 1.5;
                    }
                    if (has_trait( trait_LARGE ) || has_trait( trait_LARGE_OK )) {
                        mod *= 2;
                    }
                    if (has_trait( trait_HUGE ) || has_trait( trait_HUGE_OK )) {
                        mod *= 3;
                    }
                }
                if(it.activated(calendar::turn, "PAIN", val, reduced, mod)) {
                    int pain_inc = bound_mod_to_vals(get_pain(), val, it.get_max_val("PAIN", reduced), 0);
                    mod_pain(pain_inc);
                    if (pain_inc > 0) {
                        add_pain_msg(val, bp);
                    }
                }
            }

            // Handle Damage
            val = it.get_mod("HURT", reduced);
            if (val != 0) {
                mod = 1;
                if (it.get_sizing("HURT")) {
                    if (has_trait( trait_FAT )) {
                        mod *= 1.5;
                    }
                    if (has_trait( trait_LARGE ) || has_trait( trait_LARGE_OK )) {
                        mod *= 2;
                    }
                    if (has_trait( trait_HUGE ) || has_trait( trait_HUGE_OK )) {
                        mod *= 3;
                    }
                }
                if(it.activated(calendar::turn, "HURT", val, reduced, mod)) {
                    if (bp == num_bp) {
                        if (val > 5) {
                            add_msg_if_player(_("Your %s HURTS!"), body_part_name_accusative(bp_torso).c_str());
                        } else {
                            add_msg_if_player(_("Your %s hurts!"), body_part_name_accusative(bp_torso).c_str());
                        }
                        apply_damage(nullptr, bp_torso, val);
                    } else {
                        if (val > 5) {
                            add_msg_if_player(_("Your %s HURTS!"), body_part_name_accusative(bp).c_str());
                        } else {
                            add_msg_if_player(_("Your %s hurts!"), body_part_name_accusative(bp).c_str());
                        }
                        apply_damage(nullptr, bp, val);
                    }
                }
            }

            // Handle Sleep
            val = it.get_mod("SLEEP", reduced);
            if (val != 0) {
                mod = 1;
                if(it.activated(calendar::turn, "SLEEP", val, reduced, mod)) {
                    add_msg_if_player(_("You pass out!"));
                    fall_asleep(val);
                }
            }

            // Handle painkillers
            val = it.get_mod("PKILL", reduced);
            if (val != 0) {
                mod = it.get_addict_mod("PKILL", addiction_level(ADD_PKILLER));
                if(it.activated(calendar::turn, "PKILL", val, reduced, mod)) {
                    mod_painkiller(bound_mod_to_vals(pkill, val, it.get_max_val("PKILL", reduced), 0));
                }
            }

            // Handle coughing
            mod = 1;
            val = 0;
            if (it.activated(calendar::turn, "COUGH", val, reduced, mod)) {
                cough(it.get_harmful_cough());
            }

            // Handle vomiting
            mod = vomit_mod();
            val = 0;
            if (it.activated(calendar::turn, "VOMIT", val, reduced, mod)) {
                vomit();
            }

            // Handle stamina
            val = it.get_mod("STAMINA", reduced);
            if (val != 0) {
                mod = 1;
                if(it.activated(calendar::turn, "STAMINA", val, reduced, mod)) {
                    stamina += bound_mod_to_vals( stamina, val,
                                                  it.get_max_val("STAMINA", reduced),
                                                  it.get_min_val("STAMINA", reduced) );
                    if( stamina < 0 ) {
                        // TODO: Make it drain fatigue and/or oxygen?
                        stamina = 0;
                    } else if( stamina > get_stamina_max() ) {
                        stamina = get_stamina_max();
                    }
                }
            }

            // Speed and stats are handled in recalc_speed_bonus and reset_stats respectively
        }
    }

    Creature::process_effects();
}

double player::vomit_mod()
{
    double mod = 1;
    if (has_effect( effect_weed_high )) {
        mod *= .1;
    }
    if (has_trait( trait_STRONGSTOMACH )) {
        mod *= .5;
    }
    if (has_trait( trait_WEAKSTOMACH )) {
        mod *= 2;
    }
    if (has_trait( trait_NAUSEA )) {
        mod *= 3;
    }
    if (has_trait( trait_VOMITOUS )) {
        mod *= 3;
    }
    // If you're already nauseous, any food in your stomach greatly
    // increases chance of vomiting. Liquids don't provoke vomiting, though.
    if( get_stomach_food() != 0 && has_effect( effect_nausea ) ) {
        mod *= 5 * get_effect_int( effect_nausea );
    }
    return mod;
}

void player::suffer()
{
    // @todo Remove this section and encapsulate hp_cur
    for( int i = 0; i < num_hp_parts; i++ ) {
        body_part bp = hp_to_bp( static_cast<hp_part>( i ) );
        if( hp_cur[i] <= 0 ) {
            add_effect( effect_disabled, 1, bp, true );
        }
    }

    for (size_t i = 0; i < my_bionics.size(); i++) {
        if (my_bionics[i].powered) {
            process_bionic(i);
        }
    }

    for( auto &mut : my_mutations ) {
        auto &tdata = mut.second;
        if (!tdata.powered ) {
            continue;
        }
        const auto &mdata = mut.first.obj();
        if (tdata.powered && tdata.charge > 0) {
        // Already-on units just lose a bit of charge
        tdata.charge--;
        } else {
            // Not-on units, or those with zero charge, have to pay the power cost
            if (mdata.cooldown > 0) {
                tdata.powered = true;
                tdata.charge = mdata.cooldown - 1;
            }
            if (mdata.hunger){
                mod_hunger(mdata.cost);
                if (get_hunger() >= 700) { // Well into Famished
                    add_msg_if_player(m_warning, _("You're too famished to keep your %s going."), mdata.name.c_str());
                    tdata.powered = false;
                }
            }
            if (mdata.thirst){
                mod_thirst(mdata.cost);
                if (get_thirst() >= 260) { // Well into Dehydrated
                    add_msg_if_player(m_warning, _("You're too dehydrated to keep your %s going."), mdata.name.c_str());
                    tdata.powered = false;
                }
            }
            if (mdata.fatigue){
                mod_fatigue(mdata.cost);
                if (get_fatigue() >= EXHAUSTED) { // Exhausted
                    add_msg_if_player(m_warning, _("You're too exhausted to keep your %s going."), mdata.name.c_str());
                    tdata.powered = false;
                }
            }

            if (tdata.powered == false) {
                apply_mods(mut.first, false);
            }
        }
    }

    if (underwater) {
        if (!has_trait( trait_GILLS ) && !has_trait( trait_GILLS_CEPH )) {
            oxygen--;
        }
        if (oxygen < 12 && worn_with_flag("REBREATHER")) {
                oxygen += 12;
            }
        if (oxygen <= 5) {
            if (has_bionic("bio_gills") && power_level >= 25) {
                oxygen += 5;
                charge_power(-25);
            } else {
                add_msg_if_player(m_bad, _("You're drowning!"));
                apply_damage( nullptr, bp_torso, rng( 1, 4 ) );
            }
        }
    }

    if( has_active_mutation( trait_id( "WINGS_INSECT" ) ) ) {
        //~Sound of buzzing Insect Wings
        sounds::sound( pos(), 10, _("BZZZZZ"));
    }

    double shoe_factor = footwear_factor();
    if( has_trait( trait_ROOTS3 ) && g->m.has_flag("DIGGABLE", pos()) && !shoe_factor) {
        if (one_in(100)) {
            add_msg_if_player(m_good, _("This soil is delicious!"));
            if (get_hunger() > -20) {
                mod_hunger(-2);
            }
            if (get_thirst() > -20) {
                mod_thirst(-2);
            }
            mod_healthy_mod(10, 50);
            // No losing oneself in the fertile embrace of rich
            // New England loam.  But it can be a near thing.
            /** @EFFECT_INT decreases chance of losing focus points while eating soil with ROOTS3 */
            if ( (one_in(int_cur)) && (focus_pool >= 25) ) {
                focus_pool--;
            }
        } else if (one_in(50)){
            if (get_hunger() > -20) {
                mod_hunger(-1);
            }
            if (get_thirst() > -20) {
                mod_thirst(-1);
            }
            mod_healthy_mod(5, 50);
        }
    }

    if (!in_sleep_state()) {
        if (weight_carried() > 4 * weight_capacity()) {
            if (has_effect( effect_downed )) {
                add_effect( effect_downed, 1, num_bp, false, 0, true );
            } else {
                add_effect( effect_downed, 2, num_bp, false, 0, true );
            }
        }
        int timer = -HOURS( 6 );
        if( has_trait( trait_ADDICTIVE ) ) {
            timer = -HOURS( 10 );
        } else if( has_trait( trait_NONADDICTIVE ) ) {
            timer = -HOURS( 3 );
        }
        for( size_t i = 0; i < addictions.size(); i++ ) {
            auto &cur_addiction = addictions[i];
            if( cur_addiction.sated <= 0 &&
                cur_addiction.intensity >= MIN_ADDICTION_LEVEL ) {
                addict_effect( *this, cur_addiction );
            }
            cur_addiction.sated--;
            // Higher intensity addictions heal faster
            if( cur_addiction.sated - 100 * cur_addiction.intensity < timer ) {
                if( cur_addiction.intensity <= 2 ) {
                    rem_addiction( cur_addiction.type );
                    break;
                } else {
                    cur_addiction.intensity--;
                    cur_addiction.sated = 0;
                }
            }
        }
        if (has_trait( trait_CHEMIMBALANCE )) {
            if (one_in(3600) && (!(has_trait( trait_NOPAIN )))) {
                add_msg_if_player(m_bad, _("You suddenly feel sharp pain for no reason."));
                mod_pain( 3 * rng(1, 3) );
            }
            if (one_in(3600)) {
                int pkilladd = 5 * rng(-1, 2);
                if (pkilladd > 0) {
                    add_msg_if_player(m_bad, _("You suddenly feel numb."));
                } else if ((pkilladd < 0) && (!(has_trait( trait_NOPAIN )))) {
                    add_msg_if_player(m_bad, _("You suddenly ache."));
                }
                mod_painkiller(pkilladd);
            }
            if (one_in(3600)) {
                add_msg_if_player(m_bad, _("You feel dizzy for a moment."));
                moves -= rng(10, 30);
            }
            if (one_in(3600)) {
                int hungadd = 5 * rng(-1, 3);
                if (hungadd > 0) {
                    add_msg_if_player(m_bad, _("You suddenly feel hungry."));
                } else {
                    add_msg_if_player(m_good, _("You suddenly feel a little full."));
                }
                mod_hunger(hungadd);
            }
            if (one_in(3600)) {
                add_msg_if_player(m_bad, _("You suddenly feel thirsty."));
                mod_thirst(5 * rng(1, 3));
            }
            if (one_in(3600)) {
                add_msg_if_player(m_good, _("You feel fatigued all of a sudden."));
                mod_fatigue(10 * rng(2, 4));
            }
            if (one_in(4800)) {
                if (one_in(3)) {
                    add_morale(MORALE_FEELING_GOOD, 20, 100);
                } else {
                    add_morale(MORALE_FEELING_BAD, -20, -100);
                }
            }
            if (one_in(3600)) {
                if (one_in(3)) {
                    add_msg_if_player(m_bad, _("You suddenly feel very cold."));
                    for (int i = 0 ; i < num_bp ; i++) {
                        temp_cur[i] = BODYTEMP_VERY_COLD;
                    }
                } else {
                    add_msg_if_player(m_bad, _("You suddenly feel cold."));
                    for (int i = 0 ; i < num_bp ; i++) {
                        temp_cur[i] = BODYTEMP_COLD;
                    }
                }
            }
            if (one_in(3600)) {
                if (one_in(3)) {
                    add_msg_if_player(m_bad, _("You suddenly feel very hot."));
                    for (int i = 0 ; i < num_bp ; i++) {
                        temp_cur[i] = BODYTEMP_VERY_HOT;
                    }
                } else {
                    add_msg_if_player(m_bad, _("You suddenly feel hot."));
                    for (int i = 0 ; i < num_bp ; i++) {
                        temp_cur[i] = BODYTEMP_HOT;
                    }
                }
            }
        }
        if ((has_trait( trait_SCHIZOPHRENIC ) || has_artifact_with(AEP_SCHIZO)) &&
            one_in(2400)) { // Every 4 hours or so
            monster phantasm;
            int i;
            switch(rng(0, 11)) {
                case 0:
                    add_effect( effect_hallu, 3600 );
                    break;
                case 1:
                    add_effect( effect_visuals, rng( 15, 60 ) );
                    break;
                case 2:
                    add_msg_if_player(m_warning, _("From the south you hear glass breaking."));
                    break;
                case 3:
                    add_msg_if_player(m_warning, _("YOU SHOULD QUIT THE GAME IMMEDIATELY."));
                    add_morale(MORALE_FEELING_BAD, -50, -150);
                    break;
                case 4:
                    for (i = 0; i < 10; i++) {
                        add_msg("XXXXXXXXXXXXXXXXXXXXXXXXXXX");
                    }
                    break;
                case 5:
                    add_msg_if_player(m_bad, _("You suddenly feel so numb..."));
                    mod_painkiller(25);
                    break;
                case 6:
                    add_msg_if_player(m_bad, _("You start to shake uncontrollably."));
                    add_effect( effect_shakes, 10 * rng( 2, 5 ) );
                    break;
                case 7:
                    for (i = 0; i < 10; i++) {
                        g->spawn_hallucination();
                    }
                    break;
                case 8:
                    add_msg_if_player(m_bad, _("It's a good time to lie down and sleep."));
                    add_effect( effect_lying_down, 200);
                    break;
                case 9:
                    add_msg_if_player(m_bad, _("You have the sudden urge to SCREAM!"));
                    shout(_("AHHHHHHH!"));
                    break;
                case 10:
                    add_msg(std::string(name + name + name + name + name + name + name +
                        name + name + name + name + name + name + name +
                        name + name + name + name + name + name).c_str());
                    break;
                case 11:
                    body_part bp = random_body_part(true);
                    add_effect( effect_formication, 600, bp );
                    break;
            }
        }
        if (has_trait( trait_JITTERY ) && !has_effect( effect_shakes )) {
            if (stim > 50 && one_in(300 - stim)) {
                add_effect( effect_shakes, 300 + stim );
            } else if (get_hunger() > 80 && one_in(500 - get_hunger())) {
                add_effect( effect_shakes, 400 );
            }
        }

        if (has_trait( trait_MOODSWINGS ) && one_in(3600)) {
            if (rng(1, 20) > 9) { // 55% chance
                add_morale(MORALE_MOODSWING, -100, -500);
            } else {  // 45% chance
                add_morale(MORALE_MOODSWING, 100, 500);
            }
        }

        if (has_trait( trait_VOMITOUS ) && one_in(4200)) {
            vomit();
        }

        if (has_trait( trait_SHOUT1 ) && one_in(3600)) {
            shout();
        }
        if (has_trait( trait_SHOUT2 ) && one_in(2400)) {
            shout();
        }
        if (has_trait( trait_SHOUT3 ) && one_in(1800)) {
            shout();
        }
        if (has_trait( trait_M_SPORES ) && one_in(2400)) {
            spores();
        }
        if (has_trait( trait_M_BLOSSOMS ) && one_in(1800)) {
            blossoms();
        }
    } // Done with while-awake-only effects

    if( has_trait( trait_ASTHMA ) && one_in(3600 - stim * 50) &&
        !has_effect( effect_adrenaline ) & !has_effect( effect_datura ) ) {
        bool auto_use = has_charges("inhaler", 1);
        if (underwater) {
            oxygen = int(oxygen / 2);
            auto_use = false;
        }

        if( has_effect( effect_sleep ) ) {
            add_msg_if_player(_("You have an asthma attack!"));
            wake_up();
            auto_use = false;
        } else {
            add_msg_if_player( m_bad, _( "You have an asthma attack!" ) );
        }

        if (auto_use) {
            use_charges("inhaler", 1);
            moves -= 40;
            const auto charges = charges_of( "inhaler" );
            if( charges == 0 ) {
                add_msg_if_player( m_bad, _( "You use your last inhaler charge." ) );
            } else {
                add_msg_if_player( m_info, ngettext( "You use your inhaler, only %d charge left.",
                                                     "You use your inhaler, only %d charges left.", charges ),
                                   charges );
            }
        } else {
            add_effect( effect_asthma, 50 * rng( 1, 4 ) );
            if (!is_npc()) {
                g->cancel_activity_query(_("You have an asthma attack!"));
            }
        }
    }

    if (has_trait( trait_LEAVES ) && g->is_in_sunlight(pos()) && one_in(600)) {
        mod_hunger(-1);
    }

    if (get_pain() > 0) {
        if (has_trait( trait_PAINREC1 ) && one_in(600)) {
            mod_pain( -1 );
        }
        if (has_trait( trait_PAINREC2 ) && one_in(300)) {
            mod_pain( -1 );
        }
        if (has_trait( trait_PAINREC3 ) && one_in(150)) {
            mod_pain( -1 );
        }
    }

    if( ( has_trait( trait_ALBINO ) || has_effect( effect_datura ) ) &&
        g->is_in_sunlight( pos() ) && one_in(10) ) {
        // Umbrellas can keep the sun off the skin and sunglasses - off the eyes.
        if( !weapon.has_flag( "RAIN_PROTECT" ) ) {
            add_msg( m_bad, _( "The sunlight is really irritating your skin." ) );
            if( in_sleep_state() ) {
                wake_up();
            }
            if( one_in(10) ) {
                mod_pain(1);
            }
            else focus_pool --;
        }
        if( !( ( (worn_with_flag( "SUN_GLASSES" ) ) || worn_with_flag( "BLIND" ) ) && ( wearing_something_on( bp_eyes ) ) ) ) {
            add_msg( m_bad, _( "The sunlight is really irritating your eyes." ) );
            if( one_in(10) ) {
                mod_pain(1);
            }
            else focus_pool --;
        }
    }

    if (has_trait( trait_SUNBURN ) && g->is_in_sunlight(pos()) && one_in(10)) {
        if( !( weapon.has_flag( "RAIN_PROTECT" ) ) ) {
        add_msg(m_bad, _("The sunlight burns your skin!"));
        if (in_sleep_state()) {
            wake_up();
        }
        mod_pain(1);
        hurtall(1, nullptr);
        }
    }

    if((has_trait( trait_TROGLO ) || has_trait( trait_TROGLO2 )) &&
        g->is_in_sunlight(pos()) && g->weather == WEATHER_SUNNY) {
        mod_str_bonus(-1);
        mod_dex_bonus(-1);
        add_miss_reason(_("The sunlight distracts you."), 1);
        mod_int_bonus(-1);
        mod_per_bonus(-1);
    }
    if (has_trait( trait_TROGLO2 ) && g->is_in_sunlight(pos())) {
        mod_str_bonus(-1);
        mod_dex_bonus(-1);
        add_miss_reason(_("The sunlight distracts you."), 1);
        mod_int_bonus(-1);
        mod_per_bonus(-1);
    }
    if (has_trait( trait_TROGLO3 ) && g->is_in_sunlight(pos())) {
        mod_str_bonus(-4);
        mod_dex_bonus(-4);
        add_miss_reason(_("You can't stand the sunlight!"), 4);
        mod_int_bonus(-4);
        mod_per_bonus(-4);
    }

    if (has_trait( trait_SORES )) {
        for (int i = bp_head; i < num_bp; i++) {
            int sores_pain = 5 + (int)(0.4 * abs( encumb( body_part( i ) ) ) );
            if (get_pain() < sores_pain) {
                set_pain( sores_pain );
            }
        }
    }
        //Web Weavers...weave web
    if (has_active_mutation( trait_WEB_WEAVER ) && !in_vehicle) {
      g->m.add_field( pos(), fd_web, 1, 0 ); //this adds density to if its not already there.

     }

    // Blind/Deaf for brief periods about once an hour,
    // and visuals about once every 30 min.
    if (has_trait( trait_PER_SLIME )) {
        if (one_in(600) && !has_effect( effect_deaf )) {
            add_msg_if_player(m_bad, _("Suddenly, you can't hear anything!"));
            add_effect( effect_deaf, 100 * rng ( 2, 6 ) ) ;
        }
        if (one_in(600) && !(has_effect( effect_blind ))) {
            add_msg_if_player(m_bad, _("Suddenly, your eyes stop working!"));
            add_effect( effect_blind, 10 * rng ( 2, 6 ) ) ;
        }
        // Yes, you can be blind and hallucinate at the same time.
        // Your post-human biology is truly remarkable.
        if (one_in(300) && !(has_effect( effect_visuals ))) {
            add_msg_if_player(m_bad, _("Your visual centers must be acting up..."));
            add_effect( effect_visuals, 120 * rng ( 3, 6 ) ) ;
        }
    }

    if (has_trait( trait_WEB_SPINNER ) && !in_vehicle && one_in(3)) {
        g->m.add_field( pos(), fd_web, 1, 0 ); //this adds density to if its not already there.
    }

    if (has_trait( trait_UNSTABLE ) && one_in(28800)) { // Average once per 2 days
        mutate();
    }
    if (has_trait( trait_CHAOTIC ) && one_in(7200)) { // Should be once every 12 hours
        mutate();
    }
    if (has_artifact_with(AEP_MUTAGENIC) && one_in(28800)) {
        mutate();
    }
    if (has_artifact_with(AEP_FORCE_TELEPORT) && one_in(600)) {
        g->teleport(this);
    }

    // checking for radioactive items in inventory
    const int item_radiation = leak_level("RADIOACTIVE");

    const int map_radiation = g->m.get_radiation( pos() );

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
    const bool rad_mut_proc = rad_mut > 0 && x_in_y( rad_mut, in_sleep_state() ? HOURS(3) : MINUTES(30) );

    // Used to control vomiting from radiation to make it not-annoying
    bool radiation_increasing = false;

    if( item_radiation > 0 || map_radiation > 0 || rad_mut > 0 ) {
        bool has_helmet = false;
        const bool power_armored = is_wearing_power_armor(&has_helmet);
        const bool rad_immune = (power_armored && has_helmet) || worn_with_flag("RAD_PROOF");
        const bool rad_resist = rad_immune || power_armored || worn_with_flag("RAD_RESIST");

        float rads;
        if( rad_immune ) {
            // Power armor protects completely from radiation
            rads = 0.0f;
        } else if( rad_resist ) {
            rads = map_radiation / 400.0f + item_radiation / 40.0f;
        } else {
            rads = map_radiation / 100.0f + item_radiation / 10.0f;
        }

        if( rad_mut > 0 ) {
            const bool kept_in = rad_immune || (rad_resist && !one_in( 4 ));
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

        if( has_effect( effect_iodine ) ) {
            // Radioactive mutation makes iodine less efficient (but more useful)
            rads *= 0.3f + 0.1f * rad_mut;
        }

        if( rads > 0.0f && calendar::once_every(MINUTES(3)) && has_bionic("bio_geiger") ) {
            add_msg_if_player(m_warning, _("You feel an anomalous sensation coming from your radiation sensors."));
        }

        int rads_max = roll_remainder( rads );
        radiation += rng( 0, rads_max );

        if( rads > 0.0f ) {
            radiation_increasing = true;
        }

        // Apply rads to any radiation badges.
        for (item *const it : inv_dump()) {
            if (it->typeId() != "rad_badge") {
                continue;
            }

            // Actual irradiation levels of badges and the player aren't precisely matched.
            // This is intentional.
            int const before = it->irridation;

            const int delta = rng( 0, rads_max );
            if (delta == 0) {
                continue;
            }

            it->irridation += static_cast<int>(delta);

            // If in inventory (not worn), don't print anything.
            if( inv.has_item( *it ) ) {
                continue;
            }

            // If the color hasn't changed, don't print anything.
            const std::string &col_before = rad_badge_color(before);
            const std::string &col_after  = rad_badge_color(it->irridation);
            if (col_before == col_after) {
                continue;
            }

            add_msg_if_player( m_warning, _("Your radiation badge changes from %1$s to %2$s!"),
                               col_before.c_str(), col_after.c_str() );
        }
    }

    if( calendar::once_every(MINUTES(15)) ) {
        if (radiation < 0) {
            radiation = 0;
        } else if (radiation > 2000) {
            radiation = 2000;
        }
        if( get_option<bool>( "RAD_MUTATION" ) && rng(100, 10000) < radiation ) {
            mutate();
            radiation -= 50;
        } else if( radiation > 50 && rng(1, 3000) < radiation &&
                   ( get_stomach_food() > 0 || get_stomach_water() > 0 ||
                     radiation_increasing || !in_sleep_state() ) ) {
            vomit();
            radiation -= 1;
        }
    }

    const bool radiogenic = has_trait( trait_RADIOGENIC );
    if( radiogenic && int(calendar::turn) % MINUTES(30) == 0 && radiation > 0 ) {
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

    if( radiation > 200 && ( int(calendar::turn) % MINUTES(10) == 0 ) && x_in_y( radiation, 1000 ) ) {
        hurtall( 1, nullptr );
        radiation -= 5;
    }

    if (reactor_plut || tank_plut || slow_rad) {
        // Microreactor CBM and supporting bionics
        if (has_bionic("bio_reactor") || has_bionic("bio_advreactor")) {
            //first do the filtering of plutonium from storage to reactor
            int plut_trans;
            plut_trans = 0;
            if (tank_plut > 0) {
                if (has_active_bionic("bio_plut_filter")) {
                    plut_trans = (tank_plut * 0.025);
                } else {
                    plut_trans = (tank_plut * 0.005);
                }
                if (plut_trans < 1) {
                    plut_trans = 1;
                }
                tank_plut -= plut_trans;
                reactor_plut += plut_trans;
            }
            //leaking radiation, reactor is unshielded, but still better than a simple tank
            slow_rad += ((tank_plut * 0.1) + (reactor_plut * 0.01));
            //begin power generation
            if (reactor_plut > 0) {
                int power_gen;
                power_gen = 0;
                if (has_bionic("bio_advreactor")){
                    if ((reactor_plut * 0.05) > 2000){
                        power_gen = 2000;
                    } else {
                        power_gen = reactor_plut * 0.05;
                        if (power_gen < 1) {
                            power_gen = 1;
                        }
                    }
                    slow_rad += (power_gen * 3);
                    while (slow_rad >= 50) {
                        if (power_gen >= 1) {
                            slow_rad -= 50;
                            power_gen -= 1;
                            reactor_plut -= 1;
                        } else {
                        break;
                        }
                    }
                } else if (has_bionic("bio_reactor")) {
                    if ((reactor_plut * 0.025) > 500){
                        power_gen = 500;
                    } else {
                        power_gen = reactor_plut * 0.025;
                        if (power_gen < 1) {
                            power_gen = 1;
                        }
                    }
                    slow_rad += (power_gen * 3);
                }
                reactor_plut -= power_gen;
                while (power_gen >= 250) {
                    apply_damage( nullptr, bp_torso, 1);
                    mod_pain(1);
                    add_msg_if_player(m_bad, _("Your chest burns as your power systems overload!"));
                    charge_power(50);
                    power_gen -= 60; // ten units of power lost due to short-circuiting into you
                }
                charge_power(power_gen);
            }
        } else {
            slow_rad += (((reactor_plut * 0.4) + (tank_plut * 0.4)) * 100);
            //plutonium in body without any kind of container.  Not good at all.
            reactor_plut *= 0.6;
            tank_plut *= 0.6;
        }
        while (slow_rad >= 1000) {
            radiation += 1;
            slow_rad -= 1000;
        }
    }

    // Negative bionics effects
    if (has_bionic("bio_dis_shock") && one_in(1200)) {
        add_msg_if_player(m_bad, _("You suffer a painful electrical discharge!"));
        mod_pain(1);
        moves -= 150;

        if (weapon.typeId() == "e_handcuffs" && weapon.charges > 0) {
            weapon.charges -= rng(1, 3) * 50;
            if (weapon.charges < 1) {
                weapon.charges = 1;
            }

            add_msg_if_player(m_good, _("The %s seems to be affected by the discharge."), weapon.tname().c_str());
        }
        sfx::play_variant_sound( "bionics", "elec_discharge", 100 );
    }
    if (has_bionic("bio_dis_acid") && one_in(1500)) {
        add_msg_if_player(m_bad, _("You suffer a burning acidic discharge!"));
        hurtall(1, nullptr);
        sfx::play_variant_sound( "bionics", "acid_discharge", 100 );
        sfx::do_player_death_hurt( g->u, 0 );
    }
    if (has_bionic("bio_drain") && power_level > 24 && one_in(600)) {
        add_msg_if_player(m_bad, _("Your batteries discharge slightly."));
        charge_power(-25);
        sfx::play_variant_sound( "bionics", "elec_crackle_low", 100 );
    }
    if (has_bionic("bio_noise") && one_in(500)) {
        // TODO: NPCs with said bionic
        if(!is_deaf()) {
            add_msg(m_bad, _("A bionic emits a crackle of noise!"));
            sfx::play_variant_sound( "bionics", "elec_blast", 100 );
        } else {
            add_msg(m_bad, _("You feel your faulty bionic shuddering."));
            sfx::play_variant_sound( "bionics", "elec_blast_muffled", 100 );
        }
        sounds::sound( pos(), 60, "");
    }
    if (has_bionic("bio_power_weakness") && max_power_level > 0 &&
        power_level >= max_power_level * .75) {
        mod_str_bonus(-3);
    }
    if (has_bionic("bio_trip") && one_in(500) && !has_effect( effect_visuals )) {
        add_msg_if_player(m_bad, _("Your vision pixelates!"));
        add_effect( effect_visuals, 100 );
        sfx::play_variant_sound( "bionics", "pixelated", 100 );
    }
    if (has_bionic("bio_spasm") && one_in(3000) && !has_effect( effect_downed )) {
        add_msg_if_player(m_bad, _("Your malfunctioning bionic causes you to spasm and fall to the floor!"));
        mod_pain(1);
        add_effect( effect_stunned, 1);
        add_effect( effect_downed, 1, num_bp, false, 0, true );
        sfx::play_variant_sound( "bionics", "elec_crackle_high", 100 );
    }
    if (has_bionic("bio_shakes") && power_level > 24 && one_in(1200)) {
        add_msg_if_player(m_bad, _("Your bionics short-circuit, causing you to tremble and shiver."));
        charge_power(-25);
        add_effect( effect_shakes, 50 );
        sfx::play_variant_sound( "bionics", "elec_crackle_med", 100 );
    }
    if (has_bionic("bio_leaky") && one_in(500)) {
        mod_healthy_mod(-50, -200);
    }
    if (has_bionic("bio_sleepy") && one_in(500) && !in_sleep_state()) {
        mod_fatigue(1);
    }
    if (has_bionic("bio_itchy") && one_in(500) && !has_effect( effect_formication )) {
        add_msg_if_player(m_bad, _("Your malfunctioning bionic itches!"));
        body_part bp = random_body_part(true);
        add_effect( effect_formication, 100, bp );
    }

    // Artifact effects
    if (has_artifact_with(AEP_ATTENTION)) {
        add_effect( effect_attention, 3 );
    }
}

// At minimum level, return at_min, at maximum at_max
float addiction_scaling( float at_min, float at_max, float add_lvl )
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
    // Healing stops completely halfway between near starving and starving
    healing_factor *= 1.0f - clamp( ( get_hunger() - 100.0f ) / 2000.0f, 0.0f, 1.0f );
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
        const bool broken = (hp_cur[i] <= 0);
        if( !broken ) {
            continue;
        }

        body_part part = hp_to_bp( static_cast<hp_part>( i ) );
        if( needs_splint && !worn_with_flag( "SPLINT", part ) ) {
            continue;
        }

        int dur_inc = roll_remainder( rate_multiplier * healing_factor );
        auto &eff = get_effect( effect_mending, part );
        if( eff.is_null() ) {
            add_effect( effect_mending, dur_inc, part, true );
            continue;
        }

        eff.set_duration( eff.get_duration() + dur_inc );

        if( eff.get_duration() >= eff.get_max_duration() ) {
            hp_cur[i] = 1;
            remove_effect( effect_mending, part );
            //~ %s is bodypart
            add_memorial_log( pgettext("memorial_male", "Broken %s began to mend."),
                              pgettext("memorial_female", "Broken %s began to mend."),
                              body_part_name( part ).c_str() );
            //~ %s is bodypart
            add_msg_if_player( m_good, _("Your %s has started to mend!"),
                               body_part_name( part ).c_str() );
        }
    }
}

void player::vomit()
{
    add_memorial_log(pgettext("memorial_male", "Threw up."),
                     pgettext("memorial_female", "Threw up."));

    const int stomach_contents = get_stomach_food() + get_stomach_water();
    if( stomach_contents != 0 ) {
        mod_hunger(get_stomach_food());
        mod_thirst(get_stomach_water());

        set_stomach_food(0);
        set_stomach_water(0);
        // Remove all joy form previously eaten food and apply the penalty
        rem_morale( MORALE_FOOD_GOOD );
        rem_morale( MORALE_FOOD_HOT );
        rem_morale( MORALE_HONEY ); // bears must suffer too
        add_morale( MORALE_VOMITED, -2 * stomach_contents, -40, 90, 45, false ); // 1.5 times longer

        g->m.add_field( adjacent_tile(), fd_bile, 1, 0 );

        add_msg_player_or_npc( m_bad, _("You throw up heavily!"), _("<npcname> throws up heavily!") );
    } else {
        add_msg_if_player( m_warning, _( "You retched, but your stomach is empty." ) );
    }

    if( !has_effect( effect_nausea ) ) { // Prevents never-ending nausea
        const effect dummy_nausea( &effect_nausea.obj(), 0, num_bp, false, 1, 0 );
        add_effect( effect_nausea, std::max( dummy_nausea.get_max_duration() * stomach_contents / 21,
                                             dummy_nausea.get_int_dur_factor() ) );
    }

    moves -= 100;
    for( auto &elem : effects ) {
        for( auto &_effect_it : elem.second ) {
            auto &it = _effect_it.second;
            if( it.get_id() == effect_foodpoison ) {
                it.mod_duration(-300);
            } else if( it.get_id() == effect_drunk ) {
                it.mod_duration(rng(-100, -500));
            }
        }
    }
    remove_effect( effect_pkill1 );
    remove_effect( effect_pkill2 );
    remove_effect( effect_pkill3 );
    // Don't wake up when just retching
    if( stomach_contents != 0 ) {
        wake_up();
    }
}

void player::drench( int saturation, int flags, bool ignore_waterproof )
{
    if( saturation < 1 ) {
        return;
    }

    // OK, water gets in your AEP suit or whatever.  It wasn't built to keep you dry.
    if( has_trait( trait_DEBUG_NOTEMP ) || has_active_mutation( trait_id( "SHELL2" ) ) ||
        ( !ignore_waterproof && is_waterproof(flags) ) ) {
        return;
    }

    // Make the body wet
    for( int i = bp_torso; i < num_bp; ++i ) {
        // Different body parts have different size, they can only store so much water
        int bp_wetness_max = 0;
        if( mfb(i) & flags ){
            bp_wetness_max = drench_capacity[i];
        }

        if( bp_wetness_max == 0 ){
            continue;
        }
        // Different sources will only make the bodypart wet to a limit
        int source_wet_max = saturation * bp_wetness_max * 2 / 100;
        int wetness_increment = ignore_waterproof ? 100 : 2;
        // Respect maximums
        const int wetness_max = std::min( source_wet_max, bp_wetness_max );
        if( body_wetness[i] < wetness_max ){
            body_wetness[i] = std::min( wetness_max, body_wetness[i] + wetness_increment );
        }
    }

    // Remove onfire effect
    if( saturation > 10 || x_in_y( saturation, 10 ) ) {
        remove_effect( effect_onfire );
    }
}

void player::drench_mut_calc()
{
    for( size_t i = 0; i < num_bp; i++ ) {
        body_part bp = static_cast<body_part>( i );
        int ignored = 0;
        int neutral = 0;
        int good = 0;

        for( const auto &iter : my_mutations ) {
            const mutation_branch &mdata = iter.first.obj();
            const auto wp_iter = mdata.protection.find( bp );
            if( wp_iter != mdata.protection.end() ) {
                ignored += wp_iter->second.x;
                neutral += wp_iter->second.y;
                good += wp_iter->second.z;
            }
        }

        mut_drench[i][WT_GOOD] = good;
        mut_drench[i][WT_NEUTRAL] = neutral;
        mut_drench[i][WT_IGNORED] = ignored;
    }
}

void player::apply_wetness_morale( int temperature )
{
    // First, a quick check if we have any wetness to calculate morale from
    // Faster than checking all worn items for friendliness
    if( !std::any_of( body_wetness.begin(), body_wetness.end(),
        []( const int w ) { return w != 0; } ) ) {
        return;
    }

    // Normalize temperature to [-1.0,1.0]
    temperature = std::max( 0, std::min( 100, temperature ) );
    const double global_temperature_mod = -1.0 + ( 2.0 * temperature / 100.0 );

    int total_morale = 0;
    const auto wet_friendliness = exclusive_flag_coverage( "WATER_FRIENDLY" );
    for( int i = bp_torso; i < num_bp; ++i ) {
        // Sum of body wetness can go up to 103
        const int part_drench = body_wetness[i];
        if( part_drench == 0 ) {
            continue;
        }

        const auto &part_arr = mut_drench[i];
        const int part_ignored = part_arr[WT_IGNORED];
        const int part_neutral = part_arr[WT_NEUTRAL];
        const int part_good    = part_arr[WT_GOOD];

        if( part_ignored >= part_drench ) {
            continue;
        }

        int bp_morale = 0;
        const bool is_friendly = wet_friendliness[i];
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
            std::min( BODYTEMP_HOT, std::max( BODYTEMP_COLD, temp_cur[i] ) );
        // 0.0 at COLD, 1.0 at HOT
        const double part_mod = (part_temperature - BODYTEMP_COLD) /
                                (BODYTEMP_HOT - BODYTEMP_COLD);
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

    add_morale( MORALE_WET, morale_effect, total_morale, 5, 5, true );
}

void player::update_body_wetness( const w_point &weather )
{
    // Average number of turns to go from completely soaked to fully dry
    // assuming average temperature and humidity
    constexpr int average_drying = HOURS(2);

    // A modifier on drying time
    double delay = 1.0;
    // Weather slows down drying
    delay -= ( weather.temperature - 65 ) / 100.0;
    delay += ( weather.humidity - 66 ) / 100.0;
    delay = std::max( 0.1, delay );
    // Fur/slime retains moisture
    if( has_trait( trait_LIGHTFUR ) || has_trait( trait_FUR ) || has_trait( trait_FELINE_FUR ) ||
        has_trait( trait_LUPINE_FUR ) || has_trait( trait_CHITIN_FUR ) || has_trait( trait_CHITIN_FUR2 ) ||
        has_trait( trait_CHITIN_FUR3 )) {
        delay = delay * 6 / 5;
    }
    if( has_trait( trait_URSINE_FUR ) || has_trait( trait_SLIMY ) ) {
        delay = delay * 3 / 2;
    }

    if( !one_in_improved( average_drying * delay / 100.0 ) ) {
        // No drying this turn
        return;
    }

    // Now per-body-part stuff
    // To make drying uniform, make just one roll and reuse it
    const int drying_roll = rng( 1, 100 );
    if( drying_roll > 40 ) {
        // Wouldn't affect anything
        return;
    }

    for( int i = 0; i < num_bp; ++i ) {
        if( body_wetness[i] == 0 ) {
            continue;
        }
        // This is to normalize drying times
        int drying_chance = drench_capacity[i];
        // Body temperature affects duration of wetness
        // Note: Using temp_conv rather than temp_cur, to better approximate environment
        if( temp_conv[i] >= BODYTEMP_SCORCHING ) {
            drying_chance *= 2;
        } else if( temp_conv[i] >=  BODYTEMP_VERY_HOT ) {
            drying_chance = drying_chance * 3 / 2;
        } else if( temp_conv[i] >= BODYTEMP_HOT ) {
            drying_chance = drying_chance * 4 / 3;
        } else if( temp_conv[i] > BODYTEMP_COLD ) {
            // Comfortable, doesn't need any changes
        } else {
            // Evaporation doesn't change that much at lower temp
            drying_chance = drying_chance * 3 / 4;
        }

        if( drying_chance < 1 ) {
            drying_chance = 1;
        }

        // @todo Make evaporation reduce body heat
        if( drying_chance >= drying_roll ) {
            body_wetness[i] -= 1;
            if( body_wetness[i] < 0 ) {
                body_wetness[i] = 0;
            }
        }
    }
    // TODO: Make clothing slow down drying
}

int player::get_morale_level() const
{
    return morale->get_level();
}

void player::add_morale(morale_type type, int bonus, int max_bonus,
                        int duration, int decay_start,
                        bool capped, const itype* item_type)
{
    morale->add( type, bonus, max_bonus, duration, decay_start, capped, item_type );
}

int player::has_morale( morale_type type ) const
{
    return morale->has( type );
}

void player::rem_morale(morale_type type, const itype* item_type)
{
    morale->remove( type, item_type );
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

    for( auto &elem : effects ) {
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
        morale.reset( new player_morale( test_morale ) ); // Recover consistency
        add_msg( m_debug, "%s morale was recovered.", disp_name( true ).c_str() );
    }
}

void player::process_active_items()
{
    if( weapon.needs_processing() && weapon.process( this, pos(), false ) ) {
        weapon = ret_null;
    }

    std::vector<item *> inv_active = inv.active_items();
    for( auto tmp_it : inv_active ) {

        if( tmp_it->process( this, pos(), false ) ) {
            inv.remove_item(tmp_it);
        }
    }

    // worn items
    remove_worn_items_with( [this]( item &itm ) {
        return itm.needs_processing() && itm.process( this, pos(), false );
    } );

    long ch_UPS = charges_of( "UPS" );
    item *cloak = nullptr;
    item *power_armor = nullptr;
    // Manual iteration because we only care about *worn* active items.
    for( auto &w : worn ) {
        if( !w.active ) {
            continue;
        }
        if( w.typeId() == OPTICAL_CLOAK_ITEM_ID ) {
            cloak = &w;
        }
        // Only the main power armor item can be active, the other ones (hauling frame, helmet) aren't.
        if( power_armor == nullptr && w.is_power_armor() ) {
            power_armor = &w;
        }
    }
    if( cloak != nullptr ) {
        if( ch_UPS >= 40 ) {
            use_charges( "UPS", 40 );
            if( ch_UPS < 200 && one_in( 3 ) ) {
                add_msg_if_player( m_warning, _( "Your optical cloak flickers for a moment!" ) );
            }
        } else if( ch_UPS > 0 ) {
            use_charges( "UPS", ch_UPS );
        } else {
            add_msg_if_player( m_warning, _( "Your optical cloak flickers for a moment as it becomes opaque." ) );
            // Bypass the "you deactivate the ..." message
            cloak->active = false;
        }
    }

    // For powered armor, an armor-powering bionic should always be preferred over UPS usage.
    if( power_armor != nullptr ) {
        const int power_cost = 4;
        bool bio_powered = can_interface_armor() && power_level > 0;
        // Bionic power costs are handled elsewhere.
        if( !bio_powered ) {
            if( ch_UPS >= power_cost ) {
                use_charges( "UPS", power_cost );
            } else {
                // Deactivate armor here, bypassing the usual deactivation message.
                add_msg_if_player( m_warning, _( "Your power armor disengages." ) );
                power_armor->active = false;
            }
        }
    }

    // Load all items that use the UPS to their minimal functional charge,
    // The tool is not really useful if its charges are below charges_to_use
    ch_UPS = charges_of( "UPS" ); // might have been changed by cloak
    long ch_UPS_used = 0;
    for( size_t i = 0; i < inv.size() && ch_UPS_used < ch_UPS; i++ ) {
        item &it = inv.find_item(i);
        if( !it.has_flag( "USE_UPS" ) ) {
            continue;
        }
        if( it.charges < it.type->maximum_charges() ) {
            ch_UPS_used++;
            it.charges++;
        }
    }
    if( weapon.has_flag( "USE_UPS" ) &&  ch_UPS_used < ch_UPS &&
        weapon.charges < weapon.type->maximum_charges() ) {
        ch_UPS_used++;
        weapon.charges++;
    }

    for( item& worn_item : worn ) {
        if( ch_UPS_used >= ch_UPS ) {
            break;
        }
        if( !worn_item.has_flag( "USE_UPS" ) ) {
            continue;
        }
        if( worn_item.charges < worn_item.type->maximum_charges() ) {
            ch_UPS_used++;
            worn_item.charges++;
        }
    }
    if( ch_UPS_used > 0 ) {
        use_charges( "UPS", ch_UPS_used );
    }
}

item player::reduce_charges( int position, long quantity )
{
    item &it = i_at( position );
    if( it.is_null() ) {
        debugmsg( "invalid item position %d for reduce_charges", position );
        return ret_null;
    }
    if( it.charges <= quantity ) {
        return i_rem( position );
    }
    it.mod_charges( -quantity );
    item tmp( it );
    tmp.charges = quantity;
    return tmp;
}

item player::reduce_charges( item *it, long quantity )
{
    if( !has_item( *it ) ) {
        debugmsg( "invalid item (name %s) for reduce_charges", it->tname().c_str() );
        return ret_null;
    }
    if( it->charges <= quantity ) {
        return i_rem( it );
    }
    it->mod_charges( -quantity );
    item result( *it );
    result.charges = quantity;
    return result;
}

int player::invlet_to_position( const long linvlet ) const
{
    // Invlets may come from curses, which may also return any kind of key codes, those being
    // of type long and they can become valid, but different characters when casted to char.
    // Example: KEY_NPAGE (returned when the player presses the page-down key) is 0x152,
    // casted to char would yield 0x52, which happesn to be 'R', a valid invlet.
    if( linvlet > std::numeric_limits<char>::max() || linvlet < std::numeric_limits<char>::min() ) {
        return INT_MIN;
    }
    const char invlet = static_cast<char>( linvlet );
    if( is_npc() ) {
        DebugLog( D_WARNING,  D_GAME ) << "Why do you need to call player::invlet_to_position on npc " << name;
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

bool player::can_interface_armor() const {
    bool okay = std::any_of( my_bionics.begin(), my_bionics.end(), 
        []( const bionic &b ) { return b.powered && b.info().armor_interface; } );
    return okay;
}

const martialart &player::get_combat_style() const
{
    return style_selected.obj();
}

std::vector<item *> player::inv_dump()
{
    std::vector<item *> ret;
    if( is_armed() && can_unwield( weapon, false ) ) {
        ret.push_back(&weapon);
    }
    for (auto &i : worn) {
        ret.push_back(&i);
    }
    inv.dump(ret);
    return ret;
}

std::list<item> player::use_amount(itype_id it, int _quantity)
{
    std::list<item> ret;
    long quantity = _quantity; // Don't wanny change the function signature right now
    if (weapon.use_amount(it, quantity, ret)) {
        remove_weapon();
    }
    for( auto a = worn.begin(); a != worn.end() && quantity > 0; ) {
        if( a->use_amount( it, quantity, ret ) ) {
            a->on_takeoff( *this );
            a = worn.erase( a );
        } else {
            ++a;
        }
    }
    if (quantity <= 0) {
        return ret;
    }
    std::list<item> tmp = inv.use_amount(it, quantity);
    ret.splice(ret.end(), tmp);
    return ret;
}

bool player::use_charges_if_avail(itype_id it, long quantity)
{
    if (has_charges(it, quantity))
    {
        use_charges(it, quantity);
        return true;
    }
    return false;
}

bool player::has_fire(const int quantity) const
{
// TODO: Replace this with a "tool produces fire" flag.

    if( g->m.has_nearby_fire( pos() ) ) {
        return true;
    } else if (has_charges("torch_lit", 1)) {
        return true;
    } else if (has_charges("battletorch_lit", quantity)) {
        return true;
    } else if (has_charges("handflare_lit", 1)) {
        return true;
    } else if (has_charges("candle_lit", 1)) {
        return true;
    } else if (has_charges("ref_lighter", quantity)) {
        return true;
    } else if (has_charges("matches", quantity)) {
        return true;
    } else if (has_charges("lighter", quantity)) {
        return true;
    } else if (has_charges("flamethrower", quantity)) {
        return true;
    } else if (has_charges("flamethrower_simple", quantity)) {
        return true;
    } else if (has_charges("hotplate", quantity)) {
        return true;
    } else if (has_charges("welder", quantity)) {
        return true;
    } else if (has_charges("welder_crude", quantity)) {
        return true;
    } else if (has_charges("shishkebab_on", quantity)) {
        return true;
    } else if (has_charges("firemachete_on", quantity)) {
        return true;
    } else if (has_charges("broadfire_on", quantity)) {
        return true;
    } else if (has_charges("firekatana_on", quantity)) {
        return true;
    } else if (has_charges("zweifire_on", quantity)) {
        return true;
    } else if (has_active_bionic("bio_tools") && power_level > quantity * 5 ) {
        return true;
    } else if (has_bionic("bio_lighter") && power_level > quantity * 5 ) {
        return true;
    } else if (has_bionic("bio_laser") && power_level > quantity * 5 ) {
        return true;
    } else if( is_npc() ) {
        // A hack to make NPCs use their molotovs
        return true;
    }
    return false;
}

void player::use_fire(const int quantity)
{
//Ok, so checks for nearby fires first,
//then held lit torch or candle, bio tool/lighter/laser
//tries to use 1 charge of lighters, matches, flame throwers
//If there is enough power, will use power of one activation of the bio_lighter, bio_tools and bio_laser
// (home made, military), hotplate, welder in that order.
// bio_lighter, bio_laser, bio_tools, has_active_bionic("bio_tools"

    if( g->m.has_nearby_fire( pos() ) ) {
        return;
    } else if (has_charges("torch_lit", 1)) {
        return;
    } else if (has_charges("battletorch_lit", 1)) {
        return;
    } else if (has_charges("handflare_lit", 1)) {
        return;
    } else if (has_charges("candle_lit", 1)) {
        return;
    } else if (has_charges("shishkebab_on", quantity)) {
        return;
    } else if (has_charges("firemachete_on", quantity)) {
        return;
    } else if (has_charges("broadfire_on", quantity)) {
        return;
    } else if (has_charges("firekatana_on", quantity)) {
        return;
    } else if (has_charges("zweifire_on", quantity)) {
        return;
    } else if (has_charges("ref_lighter", quantity)) {
        use_charges("ref_lighter", quantity);
        return;
    } else if (has_charges("matches", quantity)) {
        use_charges("matches", quantity);
        return;
    } else if (has_charges("lighter", quantity)) {
        use_charges("lighter", quantity);
        return;
    } else if (has_charges("flamethrower", quantity)) {
        use_charges("flamethrower", quantity);
        return;
    } else if (has_charges("flamethrower_simple", quantity)) {
        use_charges("flamethrower_simple", quantity);
        return;
    } else if (has_charges("hotplate", quantity)) {
        use_charges("hotplate", quantity);
        return;
    } else if (has_charges("welder", quantity)) {
        use_charges("welder", quantity);
        return;
    } else if (has_charges("welder_crude", quantity)) {
        use_charges("welder_crude", quantity);
        return;
    } else if (has_charges("shishkebab_off", quantity)) {
        use_charges("shishkebab_off", quantity);
        return;
    } else if (has_charges("firemachete_off", quantity)) {
        use_charges("firemachete_off", quantity);
        return;
    } else if (has_charges("broadfire_off", quantity)) {
        use_charges("broadfire_off", quantity);
        return;
    } else if (has_charges("firekatana_off", quantity)) {
        use_charges("firekatana_off", quantity);
        return;
    } else if (has_charges("zweifire_off", quantity)) {
        use_charges("zweifire_off", quantity);
        return;
    } else if (has_active_bionic("bio_tools") && power_level > quantity * 5 ) {
        charge_power( -quantity * 5 );
        return;
    } else if (has_bionic("bio_lighter") && power_level > quantity * 5 ) {
        charge_power( -quantity * 5 );
        return;
    } else if (has_bionic("bio_laser") && power_level > quantity * 5 ) {
        charge_power( -quantity * 5 );
        return;
    }
}

std::list<item> player::use_charges( const itype_id& what, long qty )
{
    std::list<item> res;

    if( qty <= 0 ) {
        return res;

    } else if( what == "toolset" ) {
        charge_power( -qty );
        return res;

    } else if( what == "fire" ) {
        use_fire( qty );
        return res;

    } else if( what == "UPS" ) {
        if( power_level > 0 && has_active_bionic( "bio_ups" ) ) {
            auto bio = std::min( long( power_level ), qty );
            charge_power( -bio );
            qty -= std::min( qty, bio );
        }

        auto adv = charges_of( "adv_UPS_off", ceil( qty * 0.6 ) );
        if( adv > 0 ) {
            auto found = use_charges( "adv_UPS_off", adv );
            res.splice( res.end(), found );
            qty -= std::min( qty, long( adv / 0.6 ) );
        }

        auto ups = charges_of( "UPS_off", qty );
        if( ups > 0 ) {
            auto found = use_charges( "UPS_off", ups );
            res.splice( res.end(), found );
            qty -= std::min( qty, ups );
        }
    }

    std::vector<item *> del;

    visit_items( [this, &what, &qty, &res, &del]( item *e ) {
        if( e->use_charges( what, qty, res, pos() ) ) {
            del.push_back( e );
        }
        return qty > 0 ? VisitResponse::SKIP : VisitResponse::ABORT;
    } );

    for( auto e : del ) {
        remove_item( *e );
    }

    return res;
}

item* player::pick_usb()
{
    std::vector<std::pair<item*, int> > drives = inv.all_items_by_type("usb_drive");

    if (drives.empty()) {
        return NULL; // None available!
    }

    std::vector<std::string> selections;
    for (size_t i = 0; i < drives.size() && i < 9; i++) {
        selections.push_back( drives[i].first->tname() );
    }

    int select = menu_vec(false, _("Choose drive:"), selections);

    return drives[ select - 1 ].first;
}

bool player::covered_with_flag( const std::string &flag, const std::bitset<num_bp> &parts ) const
{
    std::bitset<num_bp> covered = 0;

    for (auto armorPiece = worn.rbegin(); armorPiece != worn.rend(); ++armorPiece) {
        std::bitset<num_bp> cover = armorPiece->get_covered_body_parts() & parts;

        if (cover.none()) {
            continue; // For our purposes, this piece covers nothing.
        }
        if ((cover & covered).any()) {
            continue; // the body part(s) is already covered.
        }

        bool hasFlag = armorPiece->has_flag(flag);
        if (!hasFlag) {
            return false; // The item is the top layer on a relevant body part, and isn't tagged, so we fail.
        } else {
            covered |= cover; // The item is the top layer on a relevant body part, and is tagged.
        }
    }

    return (covered == parts);
}

bool player::is_waterproof( const std::bitset<num_bp> &parts ) const
{
    return covered_with_flag("WATERPROOF", parts);
}

int player::amount_worn(const itype_id &id) const
{
    int amount = 0;
    for(auto &elem : worn) {
        if(elem.typeId() == id) {
            ++amount;
        }
    }
    return amount;
}

bool player::has_charges(const itype_id &it, long quantity) const
{
    if (it == "fire" || it == "apparatus") {
        return has_fire(quantity);
    }
    return charges_of( it, quantity ) == quantity;
}

int  player::leak_level( std::string flag ) const
{
    int leak_level = 0;
    leak_level = inv.leak_level(flag);
    return leak_level;
}

bool player::has_mission_item(int mission_id) const
{
    return mission_id != -1 && has_item_with( has_mission_item_filter{ mission_id } );
}

//Returns the amount of charges that were consumed by the player
int player::drink_from_hands(item& water) {
    int charges_consumed = 0;
    if( query_yn( _("Drink %s from your hands?"), water.type_name().c_str() ) )
    {
        // Create a dose of water no greater than the amount of water remaining.
        item water_temp( water );
        // If player is slaked water might not get consumed.
        consume_item( water_temp );
        charges_consumed = water.charges - water_temp.charges;
        if( charges_consumed > 0 )
        {
            moves -= 350;
        }
    }

    return charges_consumed;
}

// @todo Properly split meds and food instead of hacking around
bool player::consume_med( item &target )
{
    if( !target.is_medication() ) {
        return false;
    }

    const itype_id tool_type = target.type->comestible->tool;
    const auto req_tool = item::find_type( tool_type );
    if( req_tool->tool ) {
        if( !( has_amount( tool_type, 1 ) && has_charges( tool_type, req_tool->tool->charges_per_use ) ) ) {
            add_msg_if_player( m_info, _( "You need a %s to consume that!" ), req_tool->nname( 1 ).c_str() );
            return false;
        }
        use_charges( tool_type, req_tool->tool->charges_per_use );
    }

    long amount_used = 1;
    if( target.type->has_use() ) {
        amount_used = target.type->invoke( this, &target, pos() );
        if( amount_used <= 0 ) {
            return false;
        }
    }

    // @todo Get the target it was used on
    // Otherwise injecting someone will give us addictions etc.
    consume_effects( target );
    mod_moves( -250 );
    target.charges -= amount_used;
    return target.charges <= 0;
}

bool player::consume_item( item &target )
{
    if( target.is_null() ) {
        add_msg_if_player( m_info, _( "You do not have that item." ) );
        return false;
    }
    if( is_underwater() ) {
        add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return false;
    }

    item &comest = get_comestible_from( target );

    if( comest.is_null() ) {
        add_msg_if_player( m_info, _( "You can't eat your %s." ), target.tname().c_str() );
        if(is_npc()) {
            debugmsg("%s tried to eat a %s", name.c_str(), target.tname().c_str());
        }
        return false;
    }

    if( consume_med( comest ) ||
        eat( comest ) ||
        feed_battery_with( comest ) ||
        feed_reactor_with( comest ) ||
        feed_furnace_with( comest ) ) {

        if( target.is_container() ) {
            target.on_contents_changed();
        }

        return comest.charges <= 0;
    }

    return false;
}

bool player::consume(int target_position)
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
            inv.restack( this );
            inv.sort();
        }

        if( was_in_container && target_position == -1 ) {
            add_msg_if_player(_("You are now wielding an empty %s."), weapon.tname().c_str());
        } else if( was_in_container && target_position < -1 ) {
            add_msg_if_player(_("You are now wearing an empty %s."), target.tname().c_str());
        } else if( was_in_container && !is_npc() ) {
            bool drop_it = false;
            if (get_option<std::string>( "DROP_EMPTY" ) == "no") {
                drop_it = false;
            } else if (get_option<std::string>( "DROP_EMPTY" ) == "watertight") {
                drop_it = !target.is_watertight_container();
            } else if (get_option<std::string>( "DROP_EMPTY" ) == "all") {
                drop_it = true;
            }
            if (drop_it) {
                add_msg(_("You drop the empty %s."), target.tname().c_str());
                g->m.add_item_or_charges( pos(), inv.remove_item(&target) );
            } else {
                int quantity = inv.const_stack( inv.position_by_item( &target ) ).size();
                char letter = target.invlet ? target.invlet : ' ';
                add_msg( m_info, _( "%c - %d empty %s" ), letter, quantity, target.tname( quantity ).c_str() );
            }
        }
    } else if( target_position >= 0 ) {
        inv.restack( this );
        inv.unsort();
    }

    return true;
}

void player::rooted_message() const
{
    if( (has_trait( trait_ROOTS2 ) || has_trait( trait_ROOTS3 ) ) &&
        g->m.has_flag("DIGGABLE", pos()) &&
        !footwear_factor() ) {
        add_msg(m_info, _("You sink your roots into the soil."));
    }
}

void player::rooted()
// Should average a point every two minutes or so; ground isn't uniformly fertile
// Overfiling triggered hibernation checks, so capping.
{
    double shoe_factor = footwear_factor();
    if( (has_trait( trait_ROOTS2 ) || has_trait( trait_ROOTS3 )) &&
        g->m.has_flag("DIGGABLE", pos()) && shoe_factor != 1.0 ) {
        if( one_in(20.0 / (1.0 - shoe_factor)) ) {
            if (get_hunger() > -20) {
                mod_hunger(-1);
            }
            if (get_thirst() > -20) {
                mod_thirst(-1);
            }
            mod_healthy_mod(5, 50);
        }
    }
}

item::reload_option player::select_ammo( const item &base, const std::vector<item::reload_option>& opts ) const
{
    using reload_option = item::reload_option;

    if( opts.empty() ) {
        add_msg_if_player( m_info, _( "Never mind." ) );
        return reload_option();
    }

    uimenu menu;
    menu.text = string_format( _("Reload %s" ), base.tname().c_str() );
    menu.return_invalid = true;
    menu.w_width = -1;
    menu.w_height = -1;

    // Construct item names
    std::vector<std::string> names;
    std::transform( opts.begin(), opts.end(), std::back_inserter( names ), []( const reload_option& e ) {
        if( e.ammo->is_magazine() && e.ammo->ammo_data() ) {
            if( e.ammo->ammo_current() == "battery" ) {
                // This battery ammo is not a real object that can be recovered but pseudo-object that represents charge
                //~ magazine with ammo count
                return string_format( _( "%s (%d)" ), e.ammo->type_name().c_str(), e.ammo->ammo_remaining() );
            } else {
                //~ magazine with ammo (count)
                return string_format( _( "%s with %s (%d)" ), e.ammo->type_name().c_str(),
                                      e.ammo->ammo_data()->nname( e.ammo->ammo_remaining() ).c_str(), e.ammo->ammo_remaining() );
            }

        } else if( e.ammo->is_ammo_container() && g->u.is_worn( *e.ammo ) ) {
            // worn ammo containers should be named by their contents with their location also updated below
            return e.ammo->contents.front().display_name();

        } else {
            return e.ammo->display_name();
        }
    } );

    // Get location descriptions
    std::vector<std::string> where;
    std::transform( opts.begin(), opts.end(), std::back_inserter( where ), []( const reload_option& e ) {
        if( e.ammo->is_ammo_container() && g->u.is_worn( *e.ammo ) ) {
            return e.ammo->type_name();
        }
        return e.ammo.describe( &g->u );
    } );

    // Pads elements to match longest member and return length
    auto pad = []( std::vector<std::string>& vec, int n, int t ) -> int {
        for( const auto& e : vec ) {
            n = std::max( n, utf8_width( e, true ) + t );
        };
        for( auto& e : vec ) {
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
    menu.text += _("| Location " );
    menu.text += std::string( w + 3 - utf8_width( _( "| Location " ) ), ' ' );

    menu.text += _( "| Amount  " );
    menu.text += _( "| Moves   " );

    // We only show ammo statistics for guns and magazines
    if( base.is_gun() || base.is_magazine() ) {
        menu.text += _( "| Damage  | Pierce  " );
    }

    auto draw_row = [&]( int idx ) {
        const auto& sel = opts[ idx ];
        std::string row = string_format( "%s| %s |", names[ idx ].c_str(), where[ idx ].c_str() );
        row += string_format( ( sel.ammo->is_ammo() || sel.ammo->is_ammo_container() ) ? " %-7d |" : "         |", sel.qty() );
        row += string_format( " %-7d ", sel.moves() );

        if( base.is_gun() || base.is_magazine() ) {
            const itype *ammo = sel.ammo->is_ammo_container() ? sel.ammo->contents.front().ammo_data() : sel.ammo->ammo_data();
            if( ammo ) {
                row += string_format( "| %-7d | %-7d", ammo->ammo->damage, ammo->ammo->pierce );
            } else {
                row += "|         |         ";
            }
        }
        return row;
    };

    itype_id last = uistate.lastreload[ base.ammo_type() ];
    // We keep the last key so that pressing the key twice (for example, r-r for reload)
    // will always pick the first option on the list.
    int last_key = inp_mngr.get_previously_pressed_key();
    bool last_key_bound = false;
    // This is the entry that has out default
    int default_to = 0;

    for( auto i = 0; i != ( int )opts.size(); ++i ) {
        const item& ammo = opts[ i ].ammo->is_ammo_container() ? opts[ i ].ammo->contents.front() : *opts[ i ].ammo;

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

    struct reload_callback : public uimenu_callback {
        public:
            std::vector<item::reload_option> &opts;
            const std::function<std::string( int )> &draw_row;
            int last_key;
            int default_to;

            reload_callback( std::vector<item::reload_option> &_opts,
                             const std::function<std::string( int )> &_draw_row,
                             int _last_key, int _default_to ) :
                           opts( _opts ), draw_row( _draw_row ),
                           last_key( _last_key ), default_to( _default_to )
            {}

            bool key( const input_event &event, int idx, uimenu * menu ) override {
                auto cur_key = event.get_first_input();
                if( default_to != -1 && cur_key == last_key ) {
                    // Select the first entry on the list
                    menu->ret = default_to;
                    return true;
                }
                if( idx < 0 || idx > (int)opts.size() - 1 ) {
                    return false;
                }
                auto &sel = opts[ idx ];
                switch( cur_key ) {
                    case KEY_LEFT:
                        sel.qty( sel.qty() - 1 );
                        menu->entries[ idx ].txt = draw_row( idx );
                        return true;

                    case KEY_RIGHT:
                        sel.qty( sel.qty() + 1 );
                        menu->entries[ idx ].txt = draw_row( idx );
                        return true;
                }
                return false;
            }
        // @todo Get rid of the const cast - it's fucking bullshit
    } cb( const_cast<std::vector<item::reload_option> &>( opts ), draw_row, last_key, default_to );
    menu.callback = &cb;

    menu.query();
    if( menu.ret < 0 || menu.ret >= ( int ) opts.size() ) {
        add_msg_if_player( m_info, _( "Never mind." ) );
        return reload_option();
    }

    const item_location& sel = opts[ menu.ret ].ammo;
    uistate.lastreload[ base.ammo_type() ] = sel->is_ammo_container() ? sel->contents.front().typeId() : sel->typeId();
    return std::move( opts[ menu.ret ] );
}

item::reload_option player::select_ammo( const item& base, bool prompt ) const
{
    using reload_option = item::reload_option;
    std::vector<reload_option> ammo_list;

    auto opts = base.gunmods();
    opts.push_back( &base );

    if( base.magazine_current() ) {
        opts.push_back( base.magazine_current() );
    }

    for( const auto e : opts ) {
        for( item_location& ammo : find_ammo( *e ) ) {
            auto id = ammo->is_ammo_container() ? ammo->contents.front().typeId() : ammo->typeId();
            if( can_reload( *e, id ) || e->has_flag( "RELOAD_AND_SHOOT" ) ) {
                ammo_list.emplace_back( this, e, &base, std::move( ammo ) );
            }
        }
    }

    if( ammo_list.empty() ) {
        if( !base.is_magazine() && !base.magazine_integral() && !base.magazine_current() ) {
            add_msg_if_player( m_info, _( "You need a compatible magazine to reload the %s!" ), base.tname().c_str() );

        } else {
            auto name = base.ammo_data() ? base.ammo_data()->nname( 1 ) : ammo_name( base.ammo_type() );
            add_msg_if_player( m_info, _( "Out of %s!" ), name.c_str() );
        }
        return reload_option();
    }

    // sort in order of move cost (ascending), then remaining ammo (descending) with empty magazines always last
    std::stable_sort( ammo_list.begin(), ammo_list.end(), []( const reload_option& lhs, const reload_option& rhs ) {
        return lhs.ammo->ammo_remaining() > rhs.ammo->ammo_remaining();
    } );
    std::stable_sort( ammo_list.begin(), ammo_list.end(), []( const reload_option& lhs, const reload_option& rhs ) {
        return lhs.moves() < rhs.moves();
    } );
    std::stable_sort( ammo_list.begin(), ammo_list.end(), []( const reload_option& lhs, const reload_option& rhs ) {
        return ( lhs.ammo->ammo_remaining() != 0 ) > ( rhs.ammo->ammo_remaining() != 0 );
    } );

    if( is_npc() ) {
        return std::move( ammo_list[ 0 ] );
    }

    if( !prompt && ammo_list.size() == 1 ) {
        // Suppress display of reload prompt when...
        if( !base.is_gun() ) {
            return std::move( ammo_list[ 0 ]); // reloading tools

        } else if( base.magazine_integral() && base.ammo_remaining() > 0 ) {
            return std::move( ammo_list[ 0 ] ); // adding to partially filled integral magazines

        } else if( base.has_flag( "RELOAD_AND_SHOOT" ) && has_item( *ammo_list[ 0 ].ammo ) ) {
            return std::move( ammo_list[ 0 ] ); // using bows etc and ammo is already in player possession
        }
    }

    return std::move( select_ammo( base, ammo_list ) );
}

bool player::can_wear( const item& it, bool alert ) const
{
    if( !it.is_armor() ) {
        if( alert ) {
            add_msg_if_player( m_info, _( "Putting on a %s would be tricky." ), it.tname().c_str() );
        }
        return false;
    }

    if( it.is_power_armor() ) {
        for( auto &elem : worn ) {
            if( ( elem.get_covered_body_parts() & it.get_covered_body_parts() ).any() ) {
                if( alert ) {
                    add_msg_if_player( m_info, _( "You can't wear power armor over other gear!" ) );
                }
                return false;
            }
        }
        if( !it.covers( bp_torso ) ) {
            bool power_armor = false;
            if( worn.size() ) {
                for( auto &elem : worn ) {
                    if( elem.is_power_armor() ) {
                        power_armor = true;
                        break;
                    }
                }
            }
            if( !power_armor ) {
                if( alert ) {
                    add_msg_if_player( m_info, _( "You can only wear power armor components with power armor!" ) );
                }
                return false;
            }
        }

        for( auto &i : worn ) {
            if( i.is_power_armor() && i.typeId() == it.typeId() ) {
                if( alert ) {
                    add_msg_if_player( m_info, _( "You cannot wear more than one %s!" ),
                                       it.tname().c_str() );
                }
                return false;
            }
        }
    } else {
        // Only headgear can be worn with power armor, except other power armor components.
        // You can't wear headgear if power armor helmet is already sitting on your head.
        bool has_helmet = false;
        if( ( is_wearing_power_armor( &has_helmet ) && has_helmet ) &&
            ( !it.covers( bp_head ) || !it.covers( bp_mouth ) || !it.covers( bp_eyes ) ) ) {
            for( auto &i : worn ) {
                if( i.is_power_armor() ) {
                    if( alert ) {
                        add_msg_if_player( m_info, _( "You can't wear %s with power armor!" ), it.tname().c_str() );
                    }
                    return false;
                }
            }
        }
    }

    // Check if we don't have both hands available before wearing a briefcase, shield, etc. Also occurs if we're already wearing one.
    if( it.has_flag( "RESTRICT_HANDS" ) && ( !has_two_arms() || worn_with_flag( "RESTRICT_HANDS" ) || weapon.is_two_handed( *this ) ) ) {
        if(alert) {
            add_msg_if_player( m_info, _("You don't have a hand free to wear that.") );
        }
        return false;
    }

    if( amount_worn( it.typeId() ) >= MAX_WORN_PER_TYPE ) {
        if( alert ) {
            //~ always plural form
            add_msg_if_player( m_info, _( "You can't wear %i or more %s at once." ),
                               MAX_WORN_PER_TYPE + 1, it.tname( MAX_WORN_PER_TYPE + 1 ).c_str() );
        }
        return false;
    }

    if( ( ( it.covers( bp_foot_l ) && is_wearing_shoes( "left" ) ) ||
          ( it.covers( bp_foot_r ) && is_wearing_shoes( "right") ) ) &&
          ( !it.has_flag( "OVERSIZE" ) || !it.has_flag( "OUTER" ) ) &&
          !it.has_flag( "SKINTIGHT" ) && !it.has_flag( "BELTED" ) ) {
        // Checks to see if the player is wearing shoes
        if( alert ) {
            add_msg_if_player( m_info, _( "You're already wearing footwear!" ) );
        }
        return false;
    }

    if( it.covers( bp_head ) && ( encumb( bp_head ) > 10) && ( !( it.get_encumber() < 9) ) ) {
        if(alert) {
            add_msg_if_player( m_info, wearing_something_on( bp_head ) ?
                            _( "You can't wear another helmet!" ) : _( "You can't wear a helmet!" ) );
        }
        return false;
    }

    if( has_trait( trait_WOOLALLERGY ) && ( it.made_of( material_id( "wool" ) ) || it.item_tags.count( "wooled" ) ) ) {
        if( alert ) {
            add_msg_if_player( m_info, _( "You can't wear that, it's made of wool!" ) );
        }
        return false;
    }

    if( it.is_filthy() && has_trait( trait_SQUEAMISH ) ) {
        if( alert ) {
            add_msg_if_player( m_info, _( "You can't wear that, it's filthy!" ) );
        }
        return false;
    }

    if( !it.has_flag( "OVERSIZE" ) ) {
        for( const trait_id &mut : get_mutations() ) {
            const auto &branch = mut.obj();
            if( branch.conflicts_with_item( it ) ) {
                if( alert ) {
                    add_msg( m_info, _( "Your mutation %s prevents you from wearing that %s." ),
                             branch.name.c_str(), it.type_name().c_str() );
                }

                return false;
            }
        }
        if( it.covers(bp_head) &&
            !it.made_of( material_id( "wool" ) ) && !it.made_of( material_id( "cotton" ) ) &&
            !it.made_of( material_id( "nomex" ) ) && !it.made_of( material_id( "leather" ) ) &&
            ( has_trait( trait_HORNS_POINTED ) || has_trait( trait_ANTENNAE ) || has_trait( trait_ANTLERS ) ) ) {
            if( alert ) {
                add_msg_if_player( m_info, _( "You cannot wear a helmet over your %s." ),
                            ( has_trait( trait_HORNS_POINTED ) ? _( "horns" ) :
                            ( has_trait( trait_ANTENNAE ) ? _( "antennae" ) : _( "antlers" ) ) ) );
            }
            return false;
        }
    }
    return true;
}

bool player::can_wield( const item &it, bool alert ) const
{
    if( it.is_two_handed(*this) && ( !has_two_arms() || worn_with_flag("RESTRICT_HANDS") ) ) {
        if( it.has_flag("ALWAYS_TWOHAND") ) {
            if( alert ) {
                add_msg( m_info, _("The %s can't be wielded with only one arm."), it.tname().c_str() );
            }
            if( worn_with_flag("RESTRICT_HANDS") ) {
                add_msg( m_info, _("Something you are wearing hinders the use of both hands.") );
            }
        } else {
            if( alert ) {
                add_msg( m_info, _("You are too weak to wield %s with only one arm."),
                         it.tname().c_str() );
            }
            if( worn_with_flag("RESTRICT_HANDS") ) {
                add_msg( m_info, _("Something you are wearing hinders the use of both hands.") );
            }
        }
        return false;
    }
    return true;
}

bool player::can_unwield( const item& it, bool alert ) const
{
    if( it.has_flag( "NO_UNWIELD" ) ) {
        if( alert ) {
            add_msg( m_info, _( "You cannot unwield your %s" ), it.tname().c_str() );
        }
        return false;
    }

    return true;
}

bool player::wield( item& target )
{
    if( !can_unwield( weapon ) || !can_wield( target ) ) {
        return false;
    }

    if( target.is_null() ) {
        return dispose_item( item_location( *this, &weapon ),
                             string_format( _( "Stop wielding %s?" ), weapon.tname().c_str() ) );
    }

    if( &weapon == &target ) {
        add_msg( m_info, _( "You're already wielding that!" ) );
        return false;
    }

    int mv = 0;

    if( is_armed() ) {
        if( !wield( ret_null ) ) {
            return false;
        }
        inv.unsort();
    }

    // Wielding from inventory is relatively slow and does not improve with increasing weapon skill.
    // Worn items (including guns with shoulder straps) are faster but still slower
    // than a skilled player with a holster.
    // There is an additional penalty when wielding items from the inventory whilst currently grabbed.

    bool worn = is_worn( target );
    mv += item_handling_cost( target, true, worn ? INVENTORY_HANDLING_PENALTY / 2 : INVENTORY_HANDLING_PENALTY );

    if( worn ) {
        target.on_takeoff( *this );
    }

    add_msg( m_debug, "wielding took %d moves", mv );
    moves -= mv;

    if( has_item( target ) ) {
        weapon = i_rem( &target );
    } else {
        weapon = target;
    }

    last_item = weapon.typeId();

    weapon.on_wield( *this, mv );

    return true;
}

// ids of martial art styles that are available with the bio_cqb bionic.
static const std::array<matype_id, 4> bio_cqb_styles {{
    matype_id{ "style_karate" }, matype_id{ "style_judo" }, matype_id{ "style_muay_thai" }, matype_id{ "style_biojutsu" }
}};

class ma_style_callback : public uimenu_callback
{
public:
    bool key(const input_event &event, int entnum, uimenu *menu) override {
        if( event.get_first_input() != '?' ) {
            return false;
        }
        matype_id style_selected;
        const size_t index = entnum;
        if( g->u.has_active_bionic( "bio_cqb" ) && index < menu->entries.size() ) {
            const size_t id = menu->entries[index].retval - 2;
            if( id < bio_cqb_styles.size() ) {
                style_selected = bio_cqb_styles[id];
            }
        } else if( index >= 3 && index - 3 < g->u.ma_styles.size() ) {
            style_selected = g->u.ma_styles[index - 3];
        }
        if( !style_selected.str().empty() ) {
            const martialart &ma = style_selected.obj();
            std::ostringstream buffer;
            buffer << ma.name << "\n\n \n\n";
            if( !ma.techniques.empty() ) {
                buffer << ngettext( "Technique:", "Techniques:", ma.techniques.size() ) << " ";
                buffer << enumerate_as_string( ma.techniques.begin(), ma.techniques.end(), []( const matec_id &mid ) {
                    return mid.obj().name;
                } );
            }
            if( !ma.weapons.empty() ) {
                buffer << "\n\n \n\n";
                buffer << ngettext( "Weapon:", "Weapons:", ma.weapons.size() ) << " ";
                buffer << enumerate_as_string( ma.weapons.begin(), ma.weapons.end(), []( const std::string &wid ) {
                    return item::nname( wid );
                } );
            }
            popup(buffer.str(), PF_NONE);
            menu->redraw();
        }
        return true;
    }
    ~ma_style_callback() override { }
};

bool player::pick_style() // Style selection menu
{
    //Create menu
    // Entries:
    // 0: Cancel
    // 1: No style
    // x: dynamic list of selectable styles

    //If there are style already, cursor starts there
    // if no selected styles, cursor starts from no-style

    // Any other keys quit the menu

    uimenu kmenu;
    kmenu.text = _("Select a style (press ? for more info)");
    std::unique_ptr<ma_style_callback> ma_style_info(new ma_style_callback());
    kmenu.callback = ma_style_info.get();
    kmenu.desc_enabled = true;
    kmenu.addentry( 0, true, 'c', _("Cancel") );
    if (keep_hands_free) {
      kmenu.addentry_desc( 1, true, 'h', _("Keep hands free (on)"), _("When this is enabled, player won't wield things unless explicitly told to."));
    }
    else {
      kmenu.addentry_desc( 1, true, 'h', _("Keep hands free (off)"), _("When this is enabled, player won't wield things unless explicitly told to."));
    }

    if (has_active_bionic("bio_cqb")) {
        for(size_t i = 0; i < bio_cqb_styles.size(); i++) {
            if( bio_cqb_styles[i].is_valid() ) {
                auto &style = bio_cqb_styles[i].obj();
                kmenu.addentry_desc( i + 2, true, -1, style.name, style.description );
            }
        }

        kmenu.query();
        const size_t selection = kmenu.ret;
        if ( selection >= 2 && selection - 2 < bio_cqb_styles.size() ) {
            style_selected = bio_cqb_styles[selection - 2];
        }
        else if ( selection == 1 ) {
            keep_hands_free = !keep_hands_free;
        } else {
            return false;
        }
    }
    else {
        kmenu.addentry( 2, true, 'n', _("No style") );
        kmenu.selected = 2;
        kmenu.return_invalid = true; //cancel with any other keys

        for (size_t i = 0; i < ma_styles.size(); i++) {
            auto &style = ma_styles[i].obj();
                //Check if this style is currently selected
                if( ma_styles[i] == style_selected ) {
                    kmenu.selected =i+3; //+3 because there are "cancel", "keep hands free" and "no style" first in the list
                }
                kmenu.addentry_desc( i+3, true, -1, style.name , style.description );
        }

        kmenu.query();
        int selection = kmenu.ret;

        //debugmsg("selected %d",choice);
        if (selection >= 3)
            style_selected = ma_styles[selection - 3];
        else if (selection == 2)
            style_selected = matype_id( "style_none" );
        else if (selection == 1)
            keep_hands_free = !keep_hands_free;
        else
            return false;
    }

    return true;
}

hint_rating player::rate_action_wear( const item &it ) const
{
    //TODO flag already-worn items as HINT_IFFY

    if (!it.is_armor()) {
        return HINT_CANT;
    }

    return can_wear( it, false ) ? HINT_GOOD : HINT_IFFY;
}


hint_rating player::rate_action_change_side( const item &it ) const {
   if (!is_worn(it)) {
      return HINT_IFFY;
   }

    if (!it.is_sided()) {
        return HINT_CANT;
    }

    return HINT_GOOD;
}

bool player::can_reload( const item& it, const itype_id& ammo ) const {

    if( !it.is_reloadable_with( ammo ) ) {
        return false;
    }

    if( it.is_ammo_belt() ) {
        auto linkage = it.type->magazine->linkage;
        if( linkage != "NULL" && !has_charges( linkage, 1 ) ) {
            return false;
        }
    }

    return true;
}

bool player::dispose_item( item_location &&obj, const std::string& prompt )
{
    uimenu menu;
    menu.text = prompt.empty() ? string_format( _( "Dispose of %s" ), obj->tname().c_str() ) : prompt;
    menu.return_invalid = true;

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
            if( bucket && !obj->spill_contents( *this ) ) {
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
            g->m.add_item_or_charges( pos(), *obj );
            obj.remove_item();
            return true;
        }
    } );

    opts.emplace_back( dispose_option {
        bucket ? _( "Spill contents and wear item" ) : _( "Wear item" ),
        can_wear( *obj, false ), '3', item_wear_cost( *obj ),
        [this, bucket, &obj] {
            if( bucket && !obj->spill_contents( *this ) ) {
                return false;
            }

            auto res = wear_item( *obj );
            obj.remove_item();
            return res;
        }
    } );

    for( auto& e : worn ) {
        if( e.can_holster( *obj ) ) {
            auto ptr = dynamic_cast<const holster_actor *>( e.type->get_use( "holster" )->get_actor_ptr() );
            opts.emplace_back( dispose_option {
                string_format( _( "Store in %s" ), e.tname().c_str() ), true, e.invlet,
                item_store_cost( *obj, e, false, ptr->draw_cost ),
                [this, ptr, &e, &obj]{
                    return ptr->store( *this, e, *obj );
                }
            } );
        }
    }

    int w = utf8_width( menu.text, true ) + 4;
    for( const auto& e : opts ) {
        w = std::max( w, utf8_width( e.prompt, true ) + 4 );
    };
    for( auto& e : opts ) {
        e.prompt += std::string( w - utf8_width( e.prompt, true ), ' ' );
    }

    menu.text.insert( 0, 2, ' ' ); // add space for UI hotkeys
    menu.text += std::string( w + 2 - utf8_width( menu.text, true ), ' ' );
    menu.text += _( " | Moves  " );

    for( const auto& e : opts ) {
        menu.addentry( -1, e.enabled, e.invlet, string_format( e.enabled ? "%s | %-7d" : "%s |", e.prompt.c_str(), e.moves ) );
    }

    menu.query();
    if( menu.ret >= 0 ) {
        return opts[ menu.ret ].action();
    }
    return false;
}

void player::mend_item( item_location&& obj, bool interactive )
{
    if( g->u.has_trait( trait_DEBUG_HS ) ) {
        uimenu menu( true, _( "Toggle which fault?" ) );
        std::vector<std::pair<fault_id, bool>> opts;
        for( const auto& f : obj->faults_potential() ) {
            opts.emplace_back( f, obj->faults.count( f ) );
            menu.addentry( -1, true, -1, string_format( "%s %s", opts.back().second ? _( "Mend" ) : _( "Break" ),
                                                        f.obj().name().c_str() ) );
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
    std::transform( obj->faults.begin(), obj->faults.end(), std::back_inserter( faults ), []( const fault_id& e ) {
        return std::make_pair<const fault *, bool>( &e.obj(), false );
    } );

    if( faults.empty() ) {
        if( interactive ) {
            add_msg( m_info, _( "The %s doesn't have any faults to mend." ), obj->tname().c_str() );
        }
        return;
    }

    auto inv = crafting_inventory();

    for( auto& f : faults ) {
        f.second = f.first->requirements().can_make_with_inventory( inv );
    }

    int sel = 0;
    if( interactive ) {
        uimenu menu;
        menu.text = _( "Mend which fault?" );
        menu.return_invalid = true;
        menu.desc_enabled = true;
        menu.desc_lines = 12;

        int w = 80;

        for( auto& f : faults ) {
            auto reqs = f.first->requirements();
            auto tools = reqs.get_folded_tools_list( w, c_white, inv );
            auto comps = reqs.get_folded_components_list( w, c_white, inv );

            std::ostringstream descr;
            descr << _( "<color_white>Time required:</color>\n" );
            descr << "> " << calendar::print_approx_duration( f.first->time() / 100 ) << "\n";
            descr << _( "<color_white>Skills:</color>\n" );
            for( const auto& e : f.first->skills() ) {
                bool hasSkill = get_skill_level( e.first ) >= e.second;
                if ( !hasSkill && f.second ) {
                    f.second = false;
                }
                //~ %1$s represents the internal color name which shouldn't be translated, %2$s is skill name, and %3$i is skill level
                descr << string_format( _( "> <color_%1$s>%2$s %3$i</color>\n" ), hasSkill ? "c_green" : "c_red",
                                        e.first.obj().name().c_str(), e.second );
            }

            std::copy( tools.begin(), tools.end(), std::ostream_iterator<std::string>( descr, "\n" ) );
            std::copy( comps.begin(), comps.end(), std::ostream_iterator<std::string>( descr, "\n" ) );

            auto name = f.first->name();
            name += std::string( std::max( w - utf8_width( name, true ), 0 ), ' ' );

            menu.addentry_desc( -1, true, -1, name, descr.str() );
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
                add_msg( m_info, _( "You are currently unable to mend the %s." ), obj->tname().c_str() );
            }
            return;
        }

        assign_activity( activity_id( "ACT_MEND_ITEM" ), faults[ sel ].first->time() );
        activity.name = faults[ sel ].first->id().str();
        activity.targets.push_back( std::move( obj ) );
    }
}

int player::item_handling_cost( const item& it, bool penalties, int base_cost ) const
{
    int mv = base_cost;
    if( penalties ) {
        // 40 moves per liter, up to 200 at 5 liters
        mv += std::min( 200, it.volume() / 20_ml );
    }

    if( weapon.typeId() == "e_handcuffs" ) {
        mv *= 4;
    } else if( penalties && has_effect( effect_grabbed ) ) {
        mv *= 2;
    }

    // For single handed items use the least encumbered hand
    if( it.is_two_handed( *this ) ) {
        mv += encumb( bp_hand_l ) + encumb( bp_hand_r );
    } else {
        mv += std::min( encumb( bp_hand_l ), encumb( bp_hand_r ) );
    }

    return std::min( std::max( mv, 0 ), MAX_HANDLING_COST );
}

int player::item_store_cost( const item& it, const item& /* container */, bool penalties, int base_cost ) const
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

int player::item_reload_cost( const item& it, const item& ammo, long qty ) const
{
    if( ammo.is_ammo() ) {
        qty = std::max( std::min( ammo.charges, qty ), 1L );
    } else if( ammo.is_ammo_container() ) {
        qty = std::max( std::min( ammo.contents.front().charges, qty ), 1L );
    } else if( ammo.is_magazine() ) {
        qty = 1;
    } else {
        debugmsg( "cannot determine reload cost as %s is neither ammo or magazine", ammo.tname().c_str() );
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
        return mv + 100; // reload a tool
    }

    /** @EFFECT_GUN decreases the time taken to reload a magazine */
    /** @EFFECT_PISTOL decreases time taken to reload a pistol */
    /** @EFFECT_SMG decreases time taken to reload an SMG */
    /** @EFFECT_RIFLE decreases time taken to reload a rifle */
    /** @EFFECT_SHOTGUN decreases time taken to reload a shotgun */
    /** @EFFECT_LAUNCHER decreases time taken to reload a launcher */

    int cost = ( it.is_gun() ? it.type->gun->reload_time : it.type->magazine->reload_time ) * qty;

    skill_id sk = it.is_gun() ? it.type->gun->skill_used : skill_gun;
    mv += cost / ( 1.0f + std::min( float( get_skill_level( sk ) ) * 0.1f, 1.0f ) );

    if( it.has_flag( "STR_RELOAD" ) ) {
        /** @EFFECT_STR reduces reload time of some weapons */
        mv -= get_str() * 20;
    }

    return std::max( mv, 25 );
}

int player::item_wear_cost( const item& it ) const
{
    double mv = item_handling_cost( it );

    switch( it.get_layer() ) {
        case UNDERWEAR:
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
    }

    mv *= std::max( it.get_encumber() / 10.0, 1.0 );

    return mv;
}

bool player::wear( int pos, bool interactive )
{
    item& to_wear = i_at( pos );

    if( is_worn( to_wear ) ) {
        if( interactive ) {
            add_msg( m_info, _( "You are already wearing that." ) );
        }
        return false;
    }
    if( to_wear.is_null() ) {
        if( interactive ) {
            add_msg( m_info, _( "You don't have that item. ") );
        }
        return false;
    }

    if( !wear_item( to_wear, interactive ) ) {
        return false;
    }

    if( &to_wear == &weapon ) {
        weapon = ret_null;
    } else {
        // it has been copied into worn vector, but assigned an invlet,
        // in case it's a stack, reset the invlet to avoid duplicates
        to_wear.invlet = 0;
        inv.remove_item( &to_wear );
        inv.restack( this );
    }

    return true;
}

bool player::wear_item( const item &to_wear, bool interactive )
{
    if( !can_wear( to_wear, interactive ) ) {
        return false;
    }

    const bool was_deaf = is_deaf();
    last_item = to_wear.typeId();
    worn.push_back(to_wear);

    if( interactive ) {
        add_msg( _("You put on your %s."), to_wear.tname().c_str() );
        moves -= item_wear_cost( to_wear );

        for (body_part i = bp_head; i < num_bp; i = body_part(i + 1))
        {
            if (to_wear.covers(i) && encumb(i) >= 40)
            {
                add_msg_if_player(m_warning,
                    i == bp_eyes ?
                    _("Your %s are very encumbered! %s"):_("Your %s is very encumbered! %s"),
                    body_part_name(body_part(i)).c_str(), encumb_text(body_part(i)).c_str());
            }
        }
        if( !was_deaf && is_deaf() ) {
            add_msg_if_player( m_info, _( "You're deafened!" ) );
        }
    } else {
        add_msg_if_npc( _("<npcname> puts on their %s."), to_wear.tname().c_str() );
    }

    item &new_item = worn.back();
    new_item.on_wear( *this );
    if( new_item.invlet == 0 ) {
        inv.assign_empty_invlet( new_item, false );
    }

    recalc_sight_limits();
    reset_encumbrance();

    return true;
}

bool player::change_side( item &it, bool interactive )
{
    if( !it.swap_side() ) {
        if( interactive ) {
            add_msg_player_or_npc( m_info,
                                   _( "You cannot swap the side on which your %s is worn." ),
                                   _( "<npcname> cannot swap the side on which their %s is worn." ),
                                   it.tname().c_str() );
        }
        return false;
    }

    if( interactive ) {
        add_msg_player_or_npc( m_info, _( "You swap the side on which your %s is worn." ),
                                       _( "<npcname> swaps the side on which their %s is worn." ),
                                       it.tname().c_str() );
    }

    mod_moves( -250 );
    reset_encumbrance();

    return true;
}

bool player::change_side (int pos, bool interactive) {
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
    if (!it.is_armor()) {
        return HINT_CANT;
    }

    if (is_worn(it)) {
      return HINT_GOOD;
    }

    return HINT_IFFY;
}

std::list<const item *> player::get_dependent_worn_items( const item &it ) const
{
    std::list<const item *> dependent;
    // Adds dependent worn items recursively
    const std::function<void( const item &it )> add_dependent = [ & ]( const item &it ) {
        for( const auto &wit : worn ) {
            if( &wit == &it || !wit.is_worn_only_with( it ) ) {
                continue;
            }
            const auto iter = std::find_if( dependent.begin(), dependent.end(),
                [ &wit ]( const item *dit ) {
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

bool player::takeoff( const item &it, std::list<item> *res )
{
    auto iter = std::find_if( worn.begin(), worn.end(), [ &it ]( const item &wit ) {
        return &it == &wit;
    } );

    if( iter == worn.end() ) {
        add_msg_player_or_npc( m_info,
            _( "You are not wearing that item." ),
            _( "<npcname> is not wearing that item." ) );
        return false;
    }

    const auto dependent = get_dependent_worn_items( it );
    if( res == nullptr && !dependent.empty() ) {
        add_msg_player_or_npc( m_info,
                               _( "You can't take off power armor while wearing other power armor components." ),
                               _( "<npcname> can't take off power armor while wearing other power armor components." ) );
        return false;
    }

    for( const auto dep_it : dependent ) {
        if( !takeoff( *dep_it, res ) ) {
            return false; // Failed to takeoff a dependent item
        }
    }

    if( res == nullptr ) {
        if( volume_carried() + it.volume() > volume_capacity_reduced_by( it.get_storage() ) ) {
            if( is_npc() || query_yn( _( "No room in inventory for your %s.  Drop it?" ), it.tname().c_str() ) ) {
                drop( get_item_position( &it ) );
                return true;
            }
            return false;
        }
        iter->on_takeoff( *this );
        inv.add_item_keep_invlet( it );
    } else {
        iter->on_takeoff( *this );
        res->push_back( it );
    }

    add_msg_player_or_npc( _( "You take off your %s." ),
                           _( "<npcname> takes off their %s." ),
                           it.tname().c_str() );

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
    const int count = it.count_by_charges() ? it.charges : 1;

    drop( { std::make_pair( pos, count ) }, where );
}

void player::drop( const std::list<std::pair<int, int>> &what, const tripoint &where, bool stash )
{
    const activity_id type( stash ? "ACT_STASH" : "ACT_DROP" );

    if( what.empty() ) {
        return;
    }

    const tripoint target = ( where != tripoint_min ) ? where : pos();
    if( rl_dist( pos(), target ) > 1 || !( stash || g->m.can_put_items( target ) ) ) {
        add_msg_player_or_npc( m_info, _( "You can't place items here!" ),
                                       _( "<npcname> can't place items here!" ) );
        return;
    }

    assign_activity( type );
    activity.placement = target - pos();

    for( auto item_pair : what ) {
        if( can_unwield( i_at( item_pair.first ) ) ) {
            activity.values.push_back( item_pair.first );
            activity.values.push_back( item_pair.second );
        }
    }
    // @todo Remove the hack. Its here because npcs don't process activities
    if( is_npc() ) {
        activity.do_turn( this );
    }
}

void player::use_wielded() {
    use(-1);
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
    if( ( it.is_container() || it.is_gun() || it.is_bandolier() ) && !it.contents.empty() ) {
        // gunmods can also be unloaded
        return HINT_GOOD;
    }

    if( it.ammo_type().is_null() || it.has_flag("NO_UNLOAD") ) {
        return HINT_CANT;
    }

    if( it.ammo_remaining() > 0 || it.casings_count() > 0 ) {
        return HINT_GOOD;
    }

    if ( it.ammo_capacity() > 0 ) {
        return HINT_IFFY;
    }

    return HINT_CANT;
}

hint_rating player::rate_action_mend( const item &it ) const
{
    // @todo check also if item damage could be repaired via a tool
    if( !it.faults.empty() ) {
        return HINT_GOOD;
    }
    return it.faults_potential().empty() ? HINT_CANT : HINT_IFFY;
}

hint_rating player::rate_action_disassemble( const item &it )
{
    if( can_disassemble( it, crafting_inventory() ) ) {
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

    } else if (it.is_gunmod()) {
        /** @EFFECT_GUN >0 allows rating estimates for gun modifications */
        if (get_skill_level( skill_gun ) == 0) {
            return HINT_IFFY;
        } else {
            return HINT_GOOD;
        }
    } else if (it.is_bionic()) {
        return HINT_GOOD;
    } else if( it.is_food() || it.is_medication() || it.is_book() || it.is_armor() ) {
        return HINT_IFFY; //the rating is subjective, could be argued as HINT_CANT or HINT_GOOD as well
    } else if (it.is_gun()) {
        return it.is_container_empty() ? HINT_IFFY : HINT_GOOD;
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
                    it.tname().c_str(), it.ammo_required() );
        }
        return false;
    } else if( !it.ammo_sufficient() ) {
        if( show_msg ) {
            add_msg_if_player( m_info,
                    ngettext( "Your %s has %d charge but needs %d.",
                              "Your %s has %d charges but needs %d.",
                              it.ammo_remaining() ),
                    it.tname().c_str(), it.ammo_remaining(), it.ammo_required() );
        }
        return false;
    }
    return true;
}

bool player::consume_charges( item& used, long qty )
{
    if( qty < 0 ) {
        debugmsg( "Tried to consume negative charges" );
        return false;
    }

    if( !used.is_tool() && !used.is_food() && !used.is_medication() ) {
        debugmsg( "Tried to consume charges for non-tool, non-food, non-med item" );
        return false;
    }

    if( qty == 0 ) {
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
    item *used = &i_at( inventory_position );
    item copy;

    if( used->is_null() ) {
        add_msg( m_info, _( "You do not have that item." ) );
        return;
    }

    last_item = used->typeId();

    if( used->is_tool() ) {
        if( !used->type->has_use() ) {
            add_msg_if_player( _( "You can't do anything interesting with your %s." ), used->tname().c_str() );
            return;
        }
        invoke_item( used );

    } else if( used->is_bionic() ) {
        if( install_bionics( *used->type ) ) {
            i_rem( inventory_position );
        }

    } else if( used->is_food() ||
               used->is_medication() ||
               used->get_contained().is_food() ||
               used->get_contained().is_medication() ) {
        consume( inventory_position );

    } else if( used->is_book() ) {
        read( inventory_position );

    } else if ( used->type->has_use() ) {
        invoke_item( used );

    } else {
        add_msg( m_info, _( "You can't do anything interesting with your %s." ),
                 used->tname().c_str() );
    }
}

bool player::invoke_item( item* used )
{
    return invoke_item( used, pos() );
}

bool player::invoke_item( item* used, const tripoint &pt )
{
    if( !has_enough_charges( *used, true ) ) {
        return false;
    }

    if( used->type->use_methods.size() < 2 ) {
        const long charges_used = used->type->invoke( this, used, pt );
        return used->is_tool() && consume_charges( *used, charges_used );
    }

    uimenu umenu;
    umenu.text = string_format( _("What to do with your %s?"), used->tname().c_str() );
    umenu.return_invalid = true;
    for( const auto &e : used->type->use_methods ) {
        umenu.addentry( MENU_AUTOASSIGN, e.second.can_call( this, used, false, pt ),
                        MENU_AUTOASSIGN, e.second.get_name() );
    }

    umenu.query();
    int choice = umenu.ret;
    if( choice < 0 || choice >= static_cast<int>( used->type->use_methods.size() ) ) {
        return false;
    }

    const std::string &method = std::next( used->type->use_methods.begin(), choice )->first;
    long charges_used = used->type->invoke( this, used, pt, method );

    return ( used->is_tool() || used->is_medication() || used->is_container() ) && consume_charges( *used, charges_used );
}

bool player::invoke_item( item* used, const std::string &method )
{
    return invoke_item( used, method, pos() );
}

bool player::invoke_item( item* used, const std::string &method, const tripoint &pt )
{
    if( !has_enough_charges( *used, true ) ) {
        return false;
    }

    item *actually_used = used->get_usable_item( method );
    if( actually_used == nullptr ) {
        debugmsg( "Tried to invoke a method %s on item %s, which doesn't have this method",
                  method.c_str(), used->tname().c_str() );
    }

    long charges_used = actually_used->type->invoke( this, actually_used, pt, method );
    return ( used->is_tool() || used->is_medication() || used->is_container() ) && consume_charges( *actually_used, charges_used );
}

void player::reassign_item( item &it, long invlet )
{
    if( invlet ) {
        item &prev = i_at( invlet_to_position( invlet ) );
        if( !prev.is_null() ) {
            inv.reassign_item( prev, it.invlet );
        }
    }

    if( !invlet || inv_chars.valid( invlet ) ) {
        const auto iter = assigned_invlet.find( it.invlet );
        bool found = iter != assigned_invlet.end();
        if( found ) {
            assigned_invlet.erase( iter );
        }
        if( invlet && ( !found || it.invlet != invlet ) ) {
            assigned_invlet[invlet] = it.typeId();
        }
        inv.reassign_item( it, invlet );
    }
}

bool player::gunmod_remove( item &gun, item& mod )
{
    auto iter = std::find_if( gun.contents.begin(), gun.contents.end(), [&mod]( const item& e ) {
        return &mod == &e;
    } );
    if( iter == gun.contents.end() ) {
        debugmsg( "Cannot remove non-existent gunmod" );
        return false;
    }
    if( mod.ammo_remaining() && !g->unload( mod ) ) {
        return false;
    }

    gun.gun_set_mode( "DEFAULT" );
    moves -= mod.type->gunmod->install_time / 2;

    if( mod.typeId() == "brass_catcher" ) {
        gun.casings_handle( [&]( item &e ) {
            return i_add_or_drop( e );
        } );
    }

    i_add_or_drop( mod );
    gun.contents.erase( iter );

    return true;
}

std::pair<int, int> player::gunmod_installation_odds( const item& gun, const item& mod ) const
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
    roll = std::min( double( chances ), 5.0 ) / 6.0 * 100;
    // focus is either a penalty or bonus of at most +/-10%
    roll += ( std::min( std::max( focus_pool, 140 ), 60 ) - 100 ) / 4;
    // dexterity and intelligence give +/-2% for each point above or below 12
    roll += ( get_dex() - 12 ) * 2;
    roll += ( get_int() - 12 ) * 2;
    // each point of damage to the base gun reduces success by 10%
    roll -= std::min( gun.damage(), 0 ) * 10;
    roll = std::min( std::max( roll, 0 ), 100 );

    // risk of causing damage on failure increases with less durable guns
    risk = ( 100 - roll ) * ( ( 10.0 - std::min( gun.type->gun->durability, 9 ) ) / 10.0 );

    return std::make_pair( roll, risk );
}

void player::gunmod_add( item &gun, item &mod )
{
    if( !gun.gunmod_compatible( mod ) ) {
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

    if( mod.has_flag( "IRREMOVABLE" ) ) {
        if( !query_yn( _( "Permanently install your %1$s in your %2$s?" ), mod.tname().c_str(),
                       gun.tname().c_str() ) ) {
            add_msg_if_player( _( "Never mind." ) );
            return; // player cancelled installation
        }
    }

    // if chance of success <100% prompt user to continue
    if( roll < 100 ) {
        uimenu prompt;
        prompt.return_invalid = true;
        prompt.text = string_format( _( "Attach your %1$s to your %2$s?" ), mod.tname().c_str(),
                                     gun.tname().c_str() );

        std::vector<std::function<void()>> actions;

        prompt.addentry( -1, true, 'w',
                         string_format( _( "Try without tools (%i%%) risking damage (%i%%)" ), roll, risk ) );
        actions.push_back( [&] {} );

        prompt.addentry( -1, has_charges( "small_repairkit", 100 ), 'f',
                         string_format( _( "Use 100 charges of firearm repair kit (%i%%)" ), std::min( roll * 2, 100 ) ) );

        actions.push_back( [&] {
            tool = "small_repairkit";
            qty = 100;
            roll *= 2; // firearm repair kit improves success...
            risk /= 2; // ...and reduces the risk of damage upon failure
        } );

        prompt.addentry( -1, has_charges( "large_repairkit", 25 ), 'g',
                         string_format( _( "Use 25 charges of gunsmith repair kit (%i%%)" ), std::min( roll * 3, 100 ) ) );

        actions.push_back( [&] {
            tool = "large_repairkit";
            qty = 25;
            roll *= 3; // gunsmith repair kit improves success markedly...
            risk = 0;  // ...and entirely prevents damage upon failure
        } );

        prompt.query();
        if( prompt.ret < 0 ) {
            add_msg_if_player( _( "Never mind." ) );
            return; // player cancelled installation
        }
        actions[ prompt.ret ]();
    }

    int turns = !has_trait( trait_DEBUG_HS ) ? mod.type->gunmod->install_time : 0;

    assign_activity( activity_id( "ACT_GUNMOD_ADD" ), turns, -1, get_item_position( &gun ), tool );
    activity.values.push_back( get_item_position( &mod ) );
    activity.values.push_back( roll ); // chance of success (%)
    activity.values.push_back( risk ); // chance of damage (%)
    activity.values.push_back( qty ); // tool charges
}

hint_rating player::rate_action_read( const item &it ) const
{
    if( !it.is_book() ) {
        return HINT_CANT;
    }

    std::vector<std::string> dummy;
    return get_book_reader( it, dummy ) == nullptr ? HINT_IFFY : HINT_GOOD;
}

const player *player::get_book_reader( const item &book, std::vector<std::string> &reasons ) const
{
    const player *reader = nullptr;
    if( !book.is_book() ) {
        reasons.push_back( string_format( _( "Your %s is not good reading material." ),
                                          book.tname().c_str() ) );
        return nullptr;
    }

    // Check for conditions that immediately disqualify the player from reading:
    const vehicle *veh = g->m.veh_at( pos() );
    if( veh != nullptr && veh->player_in_control( *this ) ) {
        reasons.push_back( _( "It's a bad idea to read while driving!" ) );
        return nullptr;
    }
    auto type = book.type->book.get();
    if( !fun_to_read( book ) && !has_morale_to_read() && has_identified( book.typeId() ) ) {
        // Low morale still permits skimming
        reasons.push_back( _( "What's the point of studying?  (Your morale is too low!)" ) );
        return nullptr;
    }
    const skill_id &skill = type->skill;
    const auto &skill_level = get_skill_level( skill );
    if( skill && skill_level < type->req && has_identified( book.typeId() ) ) {
        reasons.push_back( string_format( _( "You don't know enough about %s to understand the jargon!" ),
                                          skill.obj().name().c_str() ) );
        return nullptr;
    }

    // Check for conditions tha disqualify us only if no NPCs can read to us
    if( type->intel > 0 && has_trait( trait_ILLITERATE ) ) {
        reasons.push_back( _( "You're illiterate!" ) );
    } else if( has_trait( trait_HYPEROPIC ) && !is_wearing( "glasses_reading" ) &&
               !is_wearing( "glasses_bifocal" ) && !has_effect( effect_contacts ) && !has_bionic( "bio_eye_optic") ) {
        reasons.push_back( _( "Your eyes won't focus without reading glasses." ) );
    } else if( fine_detail_vision_mod() > 4 ) {
        // Too dark to read only applies if the player can read to himself
        reasons.push_back( _( "It's too dark to read!" ) );
        return nullptr;
    } else {
        return this;
    }

    //Check for NPCs to read for you, negates Illiterate and Far Sighted
    //The fastest-reading NPC is chosen
    if( is_deaf() ) {
        reasons.push_back( _( "Maybe someone could read that to you, but you're deaf!" ) );
        return nullptr;
    }

    int time_taken = INT_MAX;
    auto candidates = get_crafting_helpers();

    for( const npc *elem : candidates ) {
        // Check for disqualifying factors:
        if( type->intel > 0 && elem->has_trait( trait_ILLITERATE ) ) {
            reasons.push_back( string_format( _( "%s is illiterate!" ),
                                              elem->disp_name().c_str() ) );
        } else if( skill && elem->get_skill_level( skill ) < type->req &&
                   has_identified( book.typeId() ) ) {
            reasons.push_back( string_format( _( "%s doesn't know enough about %s to understand the jargon!" ),
                                              elem->disp_name().c_str(), skill.obj().name().c_str() ) );
        } else if( elem->has_trait( trait_HYPEROPIC ) && !elem->is_wearing( "glasses_reading" ) &&
                   !elem->is_wearing( "glasses_bifocal" ) && !elem->has_effect( effect_contacts ) ) {
            reasons.push_back( string_format( _( "%s needs reading glasses!" ),
                                              elem->disp_name().c_str() ) );
        } else if( std::min( fine_detail_vision_mod(), elem->fine_detail_vision_mod() ) > 4 ) {
            reasons.push_back( string_format(
                                   _( "It's too dark for %s to read!" ),
                                   elem->disp_name().c_str() ) );
        } else if( !elem->sees( *this ) ) {
            reasons.push_back( string_format( _( "%s could read that to you, but they can't see you." ),
                                              elem->disp_name().c_str() ) );
        } else if( !elem->fun_to_read( book ) && !elem->has_morale_to_read() &&
                   has_identified( book.typeId() ) ) {
            // Low morale still permits skimming
            reasons.push_back( string_format( _( "%s morale is too low!" ), elem->disp_name( true ).c_str() ) );
        } else {
            int proj_time = time_to_read( book, *elem );
            if( proj_time < time_taken ) {
                reader = elem;
                time_taken = proj_time;
            }
        }
    } //end for all candidates
    return reader;
}

int player::time_to_read( const item &book, const player &reader, const player *learner ) const
{
    auto type = book.type->book.get();
    const skill_id &skill = type->skill;
    // The reader's reading speed has an effect only if they're trying to understand the book as they read it
    // Reading speed is assumed to be how well you learn from books (as opposed to hands-on experience)
    const bool try_understand = reader.fun_to_read( book ) ||
                                reader.get_skill_level( skill ) < type->level;
    int reading_speed = try_understand ? std::max( reader.read_speed(), read_speed() ) : read_speed();
    if( learner ) {
        reading_speed = std::max( reading_speed, learner->read_speed() );
    }

    int retval = type->time * reading_speed;
    retval *= std::min( fine_detail_vision_mod(), reader.fine_detail_vision_mod() );

    const int effective_int = std::min( {int_cur, reader.get_int(), learner ? learner->get_int() : INT_MAX } );
    if( type->intel > effective_int ) {
        retval += type->time * ( type->intel - effective_int ) * 100;
    }
    if( !has_identified( book.typeId() ) ) {
        retval /= 10; //skimming
    }
    return retval;
}

bool player::fun_to_read( const item &book ) const
{
    // If you don't have a problem with eating humans, To Serve Man becomes rewarding
    if( ( has_trait( trait_CANNIBAL ) || has_trait( trait_PSYCHOPATH ) || has_trait( trait_SAPIOVORE ) ) &&
        book.typeId() == "cookbook_human" ) {
        return true;
    } else if( has_trait( trait_SPIRITUAL ) && book.has_flag( "INSPIRATIONAL" ) ) {
        return true;
    } else {
        return book.type->book.get()->fun > 0;
    }
}

/**
 * Explanation of ACT_READ activity values:
 *
 * position: ID of the reader
 * targets: 1-element vector with the item_location (always in inventory/wielded) of the book being read
 * index: We are studying until the player with this ID gains a level; 0 indicates reading once
 * values: IDs of the NPCs who will learn something
 * str_values: Parallel to values, these contain the learning penalties (as doubles in string form) as follows:
 *             Experience gained = Experience normally gained * penalty
 */

bool player::read( int inventory_position, const bool continuous )
{
    item &it = i_at( inventory_position );
    if( it.is_null() ) {
        add_msg( m_info, _( "Never mind." ) );
        return false;
    }
    std::vector<std::string> fail_messages;
    const player *reader = get_book_reader( it, fail_messages );
    if( reader == nullptr ) {
        // We can't read, and neither can our followers
        for( const std::string &reason : fail_messages ) {
            add_msg( m_bad, "%s", reason.c_str() );
        }
        return false;
    }
    const int time_taken = time_to_read( it, *reader );

    add_msg( m_debug, "player::read: time_taken = %d", time_taken );
    player_activity act( activity_id( "ACT_READ" ), time_taken, continuous ? activity.index : 0, reader->getID() );
    act.targets.push_back( item_location( *this, &it ) );

    // If the player hasn't read this book before, skim it to get an idea of what's in it.
    if( !has_identified( it.typeId() ) ) {
        if( reader != this ) {
            add_msg( m_info, "%s", fail_messages[0].c_str() );
            add_msg( m_info, _( "%s reads aloud..." ), reader->disp_name().c_str() );
        }
        assign_activity( act );
        return true;
    }

    if( it.typeId() == "guidebook" ) {
        // special guidebook effect: print a misc. hint when read
        if( reader != this ) {
            add_msg( m_info, "%s", fail_messages[0].c_str() );
            dynamic_cast<const npc *>( reader )->say( get_hint() );
        } else {
            add_msg( m_info, get_hint().c_str() );
        }
        mod_moves( -100 );
        return false;
    }

    auto type = it.type->book.get();
    const skill_id &skill = type->skill;
    const std::string skill_name = skill ? skill.obj().name() : "";

    // Find NPCs to join the study session:
    std::map<npc *, std::string> learners;
    std::map<npc *, std::string> fun_learners; //reading only for fun
    std::map<npc *, std::string> nonlearners;
    auto candidates = get_crafting_helpers();
    for( npc *elem : candidates ) {
        const int lvl = elem->get_skill_level( skill );
        const bool skill_req = ( elem->fun_to_read( it ) && ( !skill || lvl >= type->req ) ) ||
                               ( skill && lvl < type->level && lvl >= type->req );
        const bool morale_req = elem->fun_to_read( it ) || elem->has_morale_to_read();

        if( !skill_req && elem != reader ) {
            if( skill && lvl < type->req ) {
                nonlearners.insert( { elem, string_format( _( " (needs %d %s)" ), type->req, skill_name.c_str() ) } );
            } else if( skill ) {
                nonlearners.insert( { elem, string_format( _( " (already has %d %s)" ), type->level, skill_name.c_str() ) } );
            } else {
                nonlearners.insert( { elem, _( " (uninterested)" ) } );
            }
        } else if( elem->is_deaf() && reader != elem ) {
            nonlearners.insert( { elem, _( " (deaf)" ) } );
        } else if( !morale_req ) {
            nonlearners.insert( { elem, _( " (too sad)" ) } );
        } else if( skill && lvl < type->level ) {
            const double penalty = ( double )time_taken / time_to_read( it, *reader, elem );
            learners.insert( {elem, elem == reader ? _( " (reading aloud to you)" ) : ""} );
            act.values.push_back( elem->getID() );
            act.str_values.push_back( to_string( penalty ) );
        } else {
            fun_learners.insert( {elem, elem == reader ? _( " (reading aloud to you)" ) : "" } );
            act.values.push_back( elem->getID() );
            act.str_values.push_back( "1" );
        }
    }

    if( !continuous ) {
        //only show the menu if there's useful information or multiple options
        if( skill || !nonlearners.empty() || !fun_learners.empty() ) {
            uimenu menu;

            // Some helpers to reduce repetition:
            auto length = []( const std::pair<npc *, std::string> &elem ) {
                return elem.first->disp_name().size() + elem.second.size();
            };

            auto max_length = [&length]( const std::map<npc *, std::string> &m ) {
                auto max_ele = std::max_element( m.begin(), m.end(), [&length]( std::pair<npc *, std::string> left,
                std::pair<npc *, std::string> right ) {
                    return length( left ) < length( right );
                } );
                return max_ele == m.end() ? 0 : length( *max_ele );
            };

            auto get_text =
                [&]( const std::map<npc *, std::string> &m, const std::pair<npc *, std::string> &elem )
            {
                const int lvl = elem.first->get_skill_level( skill );
                const std::string lvl_text = skill ? string_format( _( " | current level: %d" ), lvl ) : "";
                const std::string name_text = elem.first->disp_name() + elem.second;
                return string_format( ( "%-*s%s" ), max_length( m ), name_text.c_str(), lvl_text.c_str() );
            };

            auto add_header = [&menu]( const std::string & str ) {
                menu.addentry( -1, false, -1, "" );
                uimenu_entry header( -1, false, -1, str , c_yellow, c_yellow );
                header.force_color = true;
                menu.entries.push_back( header );
            };

            menu.return_invalid = true;
            menu.title = !skill ? string_format( _( "Reading %s" ), it.type_name().c_str() ) :
                         string_format( _( "Reading %s (can train %s from %d to %d)" ), it.type_name().c_str(),
                                        skill_name.c_str(), type->req, type->level );

            if( skill ) {
                const int lvl = ( int )get_skill_level( skill );
                menu.addentry( getID(), lvl < type->level, '0',
                               string_format( _( "Read until you gain a level | current level: %d" ), lvl ) );
            } else {
                menu.addentry( -1, false, '0', _( "Read until you gain a level" ) );
            }
            menu.addentry( 0, true, '1', _( "Read once" ) );

            if( skill && !learners.empty() ) {
                add_header( _( "Read until this NPC gains a level:" ) );
                for( const auto &elem : learners ) {
                    menu.addentry( elem.first->getID(), true, -1, get_text( learners, elem ) );
                }
            }
            if( !fun_learners.empty() ) {
                add_header( _( "Reading for fun:" ) );
                for( const auto &elem : fun_learners ) {
                    menu.addentry( -1, false, -1, get_text( fun_learners, elem ) );
                }
            }
            if( !nonlearners.empty() ) {
                add_header( _( "Not participating:" ) );
                for( const auto &elem : nonlearners ) {
                    menu.addentry( -1, false, -1, get_text( nonlearners, elem ) );
                }
            }

            menu.query( true );
            if( menu.ret == UIMENU_INVALID ) {
                add_msg( m_info, _( "Never mind." ) );
                return false;
            }
            act.index = menu.ret;
        }
        add_msg( m_info, _( "Now reading %s, %s to stop early." ),
                 it.type_name().c_str(), press_x( ACTION_PAUSE ).c_str() );
    }

    // Print some informational messages, but only the first time or if the information changes

    if( !continuous || activity.position != act.position ) {
        if( reader != this ) {
            add_msg( m_info, "%s", fail_messages[0].c_str() );
            add_msg( m_info, _( "%s reads aloud..." ), reader->disp_name().c_str() );
        } else if( !learners.empty() || !fun_learners.empty() ) {
            add_msg( m_info, _( "You read aloud..." ) );
        }
    }

    if( !continuous ||
    !std::all_of( learners.begin(), learners.end(), [&]( std::pair<npc *, std::string> elem ) {
    return std::count( activity.values.begin(), activity.values.end(), elem.first->getID() ) != 0;
    } ) ||
    !std::all_of( activity.values.begin(), activity.values.end(), [&]( int elem ) {
        return learners.find( g->find_npc( elem ) ) != learners.end();
    } ) ) {

        if( learners.size() == 1 ) {
            add_msg( m_info, _( "%s studies with you." ), learners.begin()->first->disp_name().c_str() );
        } else if( !learners.empty() ) {
            const std::string them = enumerate_as_string( learners.begin(),
            learners.end(), [&]( std::pair<npc *, std::string> elem ) {
                return elem.first->disp_name();
            } );
            add_msg( m_info, _( "%s study with you." ), them.c_str() );
        }

        // Don't include the reader as it would be too redundant.
        std::set<std::string> readers;
        for( const auto &elem : fun_learners ) {
            if( elem.first != reader ) {
                readers.insert( elem.first->disp_name() );
            }
        }
        if( readers.size() == 1 ) {
            add_msg( m_info, _( "%s reads with you for fun." ), readers.begin()->c_str() );
        } else if( !readers.empty() ) {
            const std::string them = enumerate_as_string( readers );
            add_msg( m_info, _( "%s read with you for fun." ), them.c_str() );
        }
    }

    if( std::min( fine_detail_vision_mod(), reader->fine_detail_vision_mod() ) > 1.0 ) {
        add_msg( m_warning,
                 _( "It's difficult for %s to see fine details right now. Reading will take longer than usual." ),
                 reader->disp_name().c_str() );
    }

    const bool complex_penalty = type->intel > std::min( int_cur, reader->get_int() );
    const player *complex_player = reader->get_int() < int_cur ? reader : this;
    if( complex_penalty && !continuous ) {
        add_msg( m_warning,
                 _( "This book is too complex for %s to easily understand. It will take longer to read." ),
                 complex_player->disp_name().c_str() );
    }

    assign_activity( act );

    // Reinforce any existing morale bonus/penalty, so it doesn't decay
    // away while you read more.
    const int minutes = time_taken / 1000;
    std::set<player *> apply_morale = { this };
    for( const auto &elem : learners ) {
        apply_morale.insert( elem.first );
    }
    for( const auto &elem : fun_learners ) {
        apply_morale.insert( elem.first );
    }
    for( player *elem : apply_morale ) {
        // If you don't have a problem with eating humans, To Serve Man becomes rewarding
        if( ( elem->has_trait( trait_CANNIBAL ) || elem->has_trait( trait_PSYCHOPATH ) ||
              elem->has_trait( trait_SAPIOVORE ) ) &&
            it.typeId() == "cookbook_human" ) {
            elem->add_morale( MORALE_BOOK, 0, 75, minutes + 30, minutes, false, it.type );
        } else if( elem->has_trait( trait_SPIRITUAL ) && it.has_flag( "INSPIRATIONAL" ) ) {
            elem->add_morale( MORALE_BOOK, 15, 90, minutes + 60, minutes, false, it.type );
        } else {
            elem->add_morale( MORALE_BOOK, 0, type->fun * 15, minutes + 30, minutes, false, it.type );
        }
    }

    return true;
}

void player::do_read( item *book )
{
    auto reading = book->type->book.get();
    if( reading == nullptr ) {
        activity.set_to_null();
        return;
    }
    const skill_id &skill = reading->skill;

    if( !has_identified( book->typeId() ) ) {
        // Note that we've read the book.
        items_identified.insert( book->typeId() );

        add_msg(_("You skim %s to find out what's in it."), book->type_name().c_str());
        if( skill && get_skill_level( skill ).can_train() ) {
            add_msg(m_info, _("Can bring your %s skill to %d."),
                    skill.obj().name().c_str(), reading->level);
            if( reading->req != 0 ){
                add_msg(m_info, _("Requires %s level %d to understand."),
                        skill.obj().name().c_str(), reading->req);
            }
        }

        add_msg(m_info, _("Requires intelligence of %d to easily read."), reading->intel);
        if (reading->fun != 0) {
            add_msg(m_info, _("Reading this book affects your morale by %d"), reading->fun);
        }
        add_msg(m_info, ngettext("A chapter of this book takes %d minute to read.",
                         "A chapter of this book takes %d minutes to read.", reading->time),
                reading->time );

        std::vector<std::string> recipe_list;
        for( auto const & elem : reading->recipes ) {
            // If the player knows it, they recognize it even if it's not clearly stated.
            if( elem.is_hidden() && !knows_recipe( elem.recipe ) ) {
                continue;
            }
            recipe_list.push_back( elem.name );
        }
        if( !recipe_list.empty() ) {
            std::string recipe_line = string_format(
                ngettext("This book contains %1$d crafting recipe: %2$s",
                         "This book contains %1$d crafting recipes: %2$s", recipe_list.size()),
                recipe_list.size(), enumerate_as_string( recipe_list ).c_str());
            add_msg(m_info, "%s", recipe_line.c_str());
        }
        if( recipe_list.size() != reading->recipes.size() ) {
            add_msg( m_info, _( "It might help you figuring out some more recipes." ) );
        }
        activity.set_to_null();
        return;
    }

    std::vector<std::pair<player *, double>> learners; //learners and their penalties
    for( size_t i = 0; i < activity.values.size(); i++ ) {
        player *n = g->find_npc( activity.values[i] );
        if( n != nullptr ) {
            const std::string &s = activity.get_str_value( i, "1" );
            learners.push_back( { n, strtod( s.c_str(), nullptr ) } );
        }
        // Otherwise they must have died/teleported or something
    }
    learners.push_back( { this, 1.0 } );
    bool continuous = false; //whether to continue reading or not
    std::set<std::string> little_learned; // NPCs who learned a little about the skill
    std::set<std::string> cant_learn;
    std::list<std::string> out_of_chapters;

    for( auto &elem : learners ) {
        player *learner = elem.first;

        if( reading->fun != 0 ) {
            int fun_bonus = 0;
            const int chapters = book->get_chapters();
            const int remain = book->get_remaining_chapters( *this );
            if( chapters > 0 && remain == 0 ) {
                //Book is out of chapters -> re-reading old book, less fun
                if( learner->is_player() ) {
                    // This goes in the front because "It isn't as much fun for Jim and you"
                    // sounds weird compared to "It isn't as much fun for you and Jim"
                    out_of_chapters.push_front( learner->disp_name() );
                } else {
                    out_of_chapters.push_back( learner->disp_name() );
                }
                //50% penalty
                fun_bonus = ( reading->fun * 5 ) / 2;
            } else {
                fun_bonus = reading->fun * 5;
            }
            // If you don't have a problem with eating humans, To Serve Man becomes rewarding
            if( ( learner->has_trait( trait_CANNIBAL ) || learner->has_trait( trait_PSYCHOPATH ) ||
                  learner->has_trait( trait_SAPIOVORE ) ) &&
                book->typeId() == "cookbook_human" ) {
                fun_bonus = 25;
                learner->add_morale( MORALE_BOOK, fun_bonus, fun_bonus * 3, 60, 30, true, book->type );
            } else if( learner->has_trait( trait_SPIRITUAL ) && book->has_flag( "INSPIRATIONAL" ) ) {
                fun_bonus = 15;
                learner->add_morale( MORALE_BOOK, fun_bonus, fun_bonus * 5, 90, 90, true, book->type );
            } else {
                learner->add_morale( MORALE_BOOK, fun_bonus, reading->fun * 15, 60, 30, true, book->type );
            }
        }

        book->mark_chapter_as_read( *learner );

        if( skill && learner->get_skill_level( skill ) < reading->level &&
            learner->get_skill_level( skill ).can_train() ) {
            auto &skill_level = learner->get_skill_level( skill );
            const int originalSkillLevel = skill_level;

            // Calculate experience gained
            /** @EFFECT_INT increases reading comprehension */
            int min_ex = std::max( 1, reading->time / 10 + learner->get_int() / 4 );
            int max_ex = reading->time /  5 + learner->get_int() / 2 - originalSkillLevel;
            if( max_ex < 2 ) {
                max_ex = 2;
            }
            if( max_ex > 10 ) {
                max_ex = 10;
            }
            if( max_ex < min_ex ) {
                max_ex = min_ex;
            }

            min_ex *= ( originalSkillLevel + 1 ) * elem.second;
            min_ex = std::max( min_ex, 1 );
            max_ex *= ( originalSkillLevel + 1 ) * elem.second;
            max_ex = std::max( min_ex, max_ex );

            skill_level.readBook( min_ex, max_ex, reading->level );

            if( skill_level != originalSkillLevel ) {
                if( learner->is_player() ) {
                    add_msg( m_good, _( "You increase %s to level %d." ), skill.obj().name().c_str(),
                             originalSkillLevel + 1 );
                    if( skill_level % 4 == 0 ) {
                        //~ %s is skill name. %d is skill level
                        add_memorial_log( pgettext( "memorial_male", "Reached skill level %1$d in %2$s." ),
                                          pgettext( "memorial_female", "Reached skill level %1$d in %2$s." ),
                                          ( int )skill_level, skill.obj().name().c_str() );
                    }
                    lua_callback( "on_skill_increased" );
                } else {
                    add_msg( m_good, _( "%s increases their %s level." ), learner->disp_name().c_str(),
                             skill.obj().name().c_str() );
                }
            } else {
                //skill_level == originalSkillLevel
                if( activity.index == learner->getID() ) {
                    continuous = true;
                }
                if( learner->is_player() ) {
                    add_msg( m_info, _( "You learn a little about %s! (%d%%)" ), skill.obj().name().c_str(),
                             skill_level.exercise() );
                } else {
                    little_learned.insert( learner->disp_name() );
                }
            }

            if( skill_level == reading->level || !skill_level.can_train() ) {
                if( learner->is_player() ) {
                    add_msg( m_info, _( "You can no longer learn from %s." ), book->type_name().c_str() );
                } else {
                    cant_learn.insert( learner->disp_name() );
                }
            }
        } else if( skill ) {
            if( learner->is_player() ) {
                add_msg( m_info, _( "You can no longer learn from %s." ), book->type_name().c_str() );
            } else {
                cant_learn.insert( learner->disp_name() );
            }
        }
    } //end for all learners

    if( little_learned.size() == 1 ) {
        add_msg( m_info, _( "%s learns a little about %s!" ), little_learned.begin()->c_str(),
                 skill.obj().name().c_str() );
    } else if( !little_learned.empty() ) {
        const std::string little_learned_msg = enumerate_as_string( little_learned );
        add_msg( m_info, _( "%s learn a little about %s!" ), little_learned_msg.c_str(),
                 skill.obj().name().c_str() );
    }

    if( !cant_learn.empty() ) {
        const std::string names = enumerate_as_string( cant_learn );
        add_msg( m_info, _( "%s can no longer learn from %s." ), names.c_str(), book->type_name().c_str() );
    }
    if( !out_of_chapters.empty() ) {
        const std::string names = enumerate_as_string( out_of_chapters );
        add_msg( m_info, _( "Rereading the %s isn't as much fun for %s." ),
                 book->type_name().c_str(), names.c_str() );
        if( out_of_chapters.front() == disp_name() && one_in( 6 ) ) {
            add_msg( m_info, _( "Maybe you should find something new to read..." ) );
        }
    }

    if( continuous ) {
        activity.set_to_null();
        read( get_item_position( book ), true );
        if( activity ) {
            return;
        }
    }

    // NPCs can't learn martial arts from manuals (yet).
    auto m = book->type->use_methods.find( "MA_MANUAL" );
    if( m != book->type->use_methods.end() ) {
        m->second.call( this, book, false, pos() );
    }

    activity.set_to_null();
}

bool player::has_identified( std::string item_id ) const
{
    return items_identified.count( item_id ) > 0;
}

bool player::studied_all_recipes(const itype &book) const
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
    if( _skills != valid_autolearn_skills ) {
        for( const auto &r : recipe_dict.all_autolearn() ) {
            if( meets_skill_requirements( r->autolearn_requirements ) ) {
                learned_recipes.include( r );
            }
        }
        valid_autolearn_skills = _skills; // Reassign the validity stamp
    }

    return learned_recipes;
}

const recipe_subset player::get_recipes_from_books( const inventory &crafting_inv ) const
{
    recipe_subset res;

    for( const auto &stack : crafting_inv.const_slice() ) {
        const item &candidate = stack->front();

        if( !candidate.is_book() ) {
            continue;
        }
        // NPCs don't need to identify books
        if( is_player() && !items_identified.count( candidate.typeId() ) ) {
            continue;
        }

        for( auto const &elem : candidate.type->book->recipes ) {
            if( get_skill_level( elem.recipe->skill_used ) >= elem.skill_level ) {
                res.include( elem.recipe, elem.skill_level );
            }
        }
    }

    return res;
}

const recipe_subset player::get_available_recipes( const inventory &crafting_inv, const std::vector<npc *> *helpers ) const
{
    recipe_subset res( get_learned_recipes() );

    res.include( get_recipes_from_books( crafting_inv ) );

    if( helpers != nullptr ) {
        for( npc *np : *helpers ) {
            // Directly form the helper's inventory
            res.include( get_recipes_from_books( np->inv ) );
            // Being told what to do
            res.include_if( np->get_learned_recipes(), [ this ]( const recipe &r ) {
                return get_skill_level( r.skill_used ) >= int( r.difficulty * 0.8f ); // Skilled enough to understand
            } );
        }
    }

    return res;
}

void player::try_to_sleep()
{
    int vpart = -1;
    vehicle *veh = g->m.veh_at( pos(), vpart );
    const trap &trap_at_pos = g->m.tr_at(pos());
    const ter_id ter_at_pos = g->m.ter(pos());
    const furn_id furn_at_pos = g->m.furn(pos());
    bool plantsleep = false;
    bool websleep = false;
    bool webforce = false;
    bool websleeping = false;
    bool in_shell = false;
    if (has_trait( trait_CHLOROMORPH )) {
        plantsleep = true;
        if( (ter_at_pos == t_dirt || ter_at_pos == t_pit ||
              ter_at_pos == t_dirtmound || ter_at_pos == t_pit_shallow ||
              ter_at_pos == t_grass) && !veh &&
              furn_at_pos == f_null ) {
            add_msg_if_player(m_good, _("You relax as your roots embrace the soil."));
        } else if (veh) {
            add_msg_if_player(m_bad, _("It's impossible to sleep in this wheeled pot!"));
        } else if (furn_at_pos != f_null) {
            add_msg_if_player(m_bad, _("The humans' furniture blocks your roots. You can't get comfortable."));
        } else { // Floor problems
            add_msg_if_player(m_bad, _("Your roots scrabble ineffectively at the unyielding surface."));
        }
    }
    if (has_trait( trait_WEB_WALKER )) {
        websleep = true;
    }
    // Not sure how one would get Arachnid w/o web-making, but Just In Case
    if (has_trait( trait_THRESH_SPIDER ) && (has_trait( trait_WEB_SPINNER ) || (has_trait( trait_WEB_WEAVER ))) ) {
        webforce = true;
    }
    if (websleep || webforce) {
        int web = g->m.get_field_strength( pos(), fd_web );
            if (!webforce) {
            // At this point, it's kinda weird, but surprisingly comfy...
            if (web >= 3) {
                add_msg_if_player(m_good, _("These thick webs support your weight, and are strangely comfortable..."));
                websleeping = true;
            } else if( web > 0 ) {
                add_msg_if_player(m_info, _("You try to sleep, but the webs get in the way.  You brush them aside."));
                g->m.remove_field( pos(), fd_web );
            }
        } else {
            // Here, you're just not comfortable outside a nice thick web.
            if (web >= 3) {
                add_msg_if_player(m_good, _("You relax into your web."));
                websleeping = true;
            } else {
                add_msg_if_player(m_bad, _("You try to sleep, but you feel exposed and your spinnerets keep twitching."));
                add_msg_if_player(m_info, _("Maybe a nice thick web would help you sleep."));
            }
        }
    }
    if (has_active_mutation( trait_SHELL2 )) {
        // Your shell's interior is a comfortable place to sleep.
        in_shell = true;
    }
    if(!plantsleep && (furn_at_pos == f_bed || furn_at_pos == f_makeshift_bed ||
         trap_at_pos.loadid == tr_cot || trap_at_pos.loadid == tr_rollmat ||
         trap_at_pos.loadid == tr_fur_rollmat || furn_at_pos == f_armchair ||
         furn_at_pos == f_sofa || furn_at_pos == f_hay || furn_at_pos == f_straw_bed ||
         ter_at_pos == t_improvised_shelter || (in_shell) || (websleeping) ||
         (veh && veh->part_with_feature (vpart, "SEAT") >= 0) ||
         (veh && veh->part_with_feature (vpart, "BED") >= 0)) ) {
        add_msg_if_player(m_good, _("This is a comfortable place to sleep."));
    } else if (ter_at_pos != t_floor && !plantsleep) {
        add_msg_if_player( ter_at_pos.obj().movecost <= 2 ?
                 _("It's a little hard to get to sleep on this %s.") :
                 _("It's hard to get to sleep on this %s."),
                 ter_at_pos.obj().name.c_str() );
    }
    add_effect( effect_lying_down, 300);
}

int player::sleep_spot( const tripoint &p ) const
{
    int sleepy = 0;
    bool plantsleep = false;
    bool websleep = false;
    bool webforce = false;
    bool in_shell = false;
    if (has_addiction(ADD_SLEEP)) {
        sleepy -= 4;
    }
    if (has_trait( trait_INSOMNIA )) {
        // 12.5 points is the difference between "tired" and "dead tired"
        sleepy -= 12;
    }
    if (has_trait( trait_EASYSLEEPER )) {
        // Low fatigue (being rested) has a much stronger effect than high fatigue
        // so it's OK for the value to be that much higher
        sleepy += 24;
    }
    if (has_trait( trait_CHLOROMORPH )) {
        plantsleep = true;
    }
    if (has_trait( trait_WEB_WALKER )) {
        websleep = true;
    }
    // Not sure how one would get Arachnid w/o web-making, but Just In Case
    if (has_trait( trait_THRESH_SPIDER ) && (has_trait( trait_WEB_SPINNER ) || (has_trait( trait_WEB_WEAVER ))) ) {
        webforce = true;
    }
    if (has_active_mutation( trait_SHELL2 )) {
        // Your shell's interior is a comfortable place to sleep.
        in_shell = true;
    }
    int vpart = -1;
    vehicle *veh = g->m.veh_at(p, vpart);
    const maptile tile = g->m.maptile_at( p );
    const trap &trap_at_pos = tile.get_trap_t();
    const ter_id ter_at_pos = tile.get_ter();
    const furn_id furn_at_pos = tile.get_furn();
    int web = g->m.get_field_strength( p, fd_web );
    // Plant sleepers use a different method to determine how comfortable something is
    // Web-spinning Arachnids do too
    if (!plantsleep && !webforce) {
        // Shells are comfortable and get used anywhere
        if (in_shell) {
            sleepy += 4;
        // Else use the vehicle tile if we are in one
        } else if (veh) {
            if (veh->part_with_feature (vpart, "BED") >= 0) {
                sleepy += 4;
            } else if (veh->part_with_feature (vpart, "SEAT") >= 0) {
                sleepy += 3;
            } else {
                // Sleeping elsewhere is uncomfortable
                sleepy -= g->m.move_cost(p);
            }
        // Not in a vehicle, start checking furniture/terrain/traps at this point in decreasing order
        } else if (furn_at_pos == f_bed) {
            sleepy += 5;
        } else if (furn_at_pos == f_makeshift_bed || trap_at_pos.loadid == tr_cot ||
                    furn_at_pos == f_sofa) {
            sleepy += 4;
        } else if (websleep && web >= 3) {
            sleepy += 4;
        } else if (trap_at_pos.loadid == tr_rollmat || trap_at_pos.loadid == tr_fur_rollmat ||
              furn_at_pos == f_armchair || ter_at_pos == t_improvised_shelter) {
            sleepy += 3;
        } else if (furn_at_pos == f_straw_bed || furn_at_pos == f_hay || furn_at_pos == f_tatami) {
            sleepy += 2;
        } else if (ter_at_pos == t_floor || ter_at_pos == t_floor_waxed ||
                    ter_at_pos == t_carpet_red || ter_at_pos == t_carpet_yellow ||
                    ter_at_pos == t_carpet_green || ter_at_pos == t_carpet_purple) {
            sleepy += 1;
        } else {
            // Not a comfortable sleeping spot
            sleepy -= g->m.move_cost(p);
        }
    // Has plantsleep
    } else if (plantsleep) {
        if (veh || furn_at_pos != f_null) {
            // Sleep ain't happening in a vehicle or on furniture
            sleepy -= 999;
        } else {
            // It's very easy for Chloromorphs to get to sleep on soil!
            if (ter_at_pos == t_dirt || ter_at_pos == t_pit || ter_at_pos == t_dirtmound ||
                  ter_at_pos == t_pit_shallow) {
                sleepy += 10;
            // Not as much if you have to dig through stuff first
            } else if (ter_at_pos == t_grass) {
                sleepy += 5;
            // Sleep ain't happening
            } else {
                sleepy -= 999;
            }
        }
    // Has webforce
    } else {
        if (web >= 3) {
            // Thick Web and you're good to go
            sleepy += 10;
        }
        else {
            sleepy -= 999;
        }
    }
    if (get_fatigue() < TIRED + 1) {
        sleepy -= int( (TIRED + 1 - get_fatigue()) / 4);
    } else {
        sleepy += int((get_fatigue() - TIRED + 1) / 16);
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
    int sleepy = sleep_spot( pos() );
    sleepy += rng( -8, 8 );
    if( sleepy > 0 ) {
        return true;
    }
    return false;
}

void player::fall_asleep(int duration)
{
    if( activity ) {
        cancel_activity();
    }
    add_effect( effect_sleep, duration );
}

void player::wake_up()
{
    if( has_effect( effect_sleep ) ) {
        if(calendar::turn - get_effect( effect_sleep ).get_start_turn() > HOURS(2) ) {
            print_health();
        }
    }

    remove_effect( effect_sleep );
    remove_effect( effect_lying_down );
}

std::string player::is_snuggling() const
{
    auto begin = g->m.i_at( pos() ).begin();
    auto end = g->m.i_at( pos() ).end();

    if( in_vehicle ) {
        int vpart;
        vehicle *veh = g->m.veh_at( pos(), vpart );
        if( veh != nullptr ) {
            int cargo = veh->part_with_feature( vpart, VPFLAG_CARGO, false );
            if( cargo >= 0 ) {
                if( !veh->get_items(cargo).empty() ) {
                    begin = veh->get_items(cargo).begin();
                    end = veh->get_items(cargo).end();
                }
            }
        }
    }
    const item* floor_armor = NULL;
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

    if ( ticker == 0 ) {
        return "nothing";
    }
    else if ( ticker == 1 ) {
        return floor_armor->type_name();
    }
    else if ( ticker > 1 ) {
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
float player::fine_detail_vision_mod() const
{
    // PER_SLIME_OK implies you can get enough eyes around the bile
    // that you can generaly see.  There'll still be the haze, but
    // it's annoying rather than limiting.
    if( is_blind() ||
         ( ( has_effect( effect_boomered ) || has_effect( effect_darkness ) ) && !has_trait( trait_PER_SLIME_OK ) ) ) {
        return 11.0;
    }
    // Scale linearly as light level approaches LIGHT_AMBIENT_LIT.
    // If we're actually a source of light, assume we can direct it where we need it.
    // Therefore give a hefty bonus relative to ambient light.
    float own_light = std::max( 1.0, LIGHT_AMBIENT_LIT - active_light() - 2 );

    // Same calculation as above, but with a result 3 lower.
    float ambient_light = std::max( 1.0, LIGHT_AMBIENT_LIT - g->m.ambient_light_at( pos() ) + 1.0 );

    return std::min( own_light, ambient_light );
}

int player::get_wind_resistance(body_part bp) const
{
    int coverage = 0;
    float totalExposed = 1.0;
    int totalCoverage = 0;
    int penalty = 100;

    for( auto &i : worn ) {
        if( i.covers(bp) ) {
            if( i.made_of( material_id( "leather" ) ) || i.made_of( material_id( "plastic" ) ) || i.made_of( material_id( "bone" ) ) ||
                i.made_of( material_id( "chitin" ) ) || i.made_of( material_id( "nomex" ) ) ) {
                penalty = 10; // 90% effective
            } else if( i.made_of( material_id( "cotton" ) ) ) {
                penalty = 30;
            } else if( i.made_of( material_id( "wool" ) ) ) {
                penalty = 40;
            } else {
                penalty = 1; // 99% effective
            }

            coverage = std::max(0, i.get_coverage() - penalty);
            totalExposed *= (1.0 - coverage/100.0); // Coverage is between 0 and 1?
        }
    }

    // Your shell provides complete wind protection if you're inside it
    if (has_active_mutation( trait_SHELL2 )) {
        totalCoverage = 100;
        return totalCoverage;
    }

    totalCoverage = 100 - totalExposed*100;

    return totalCoverage;
}

int player::warmth(body_part bp) const
{
    int ret = 0, warmth = 0;

    for (auto &i : worn) {
        if( i.covers( bp ) ) {
            warmth = i.get_warmth();
            // Wool items do not lose their warmth due to being wet.
            // Warmth is reduced by 0 - 66% based on wetness.
            if (!i.made_of( material_id( "wool" ) ))
            {
                warmth *= 1.0 - 0.66 * body_wetness[bp] / drench_capacity[bp];
            }
            ret += warmth;
        }
    }
    return ret;
}

int bestwarmth( const std::list< item > &its, const std::string &flag )
{
    int best = 0;
    for( auto &w : its ) {
        if( w.has_flag( flag ) && w.get_warmth() > best ) {
            best = w.get_warmth();
        }
    }
    return best;
}

int player::bonus_item_warmth(body_part bp) const
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

int player::get_armor_bash(body_part bp) const
{
    return get_armor_bash_base(bp) + armor_bash_bonus;
}

int player::get_armor_cut(body_part bp) const
{
    return get_armor_cut_base(bp) + armor_cut_bonus;
}

int player::get_armor_type( damage_type dt, body_part bp ) const
{
    switch( dt ) {
        case DT_TRUE:
            return 0;
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
        case DT_ELECTRIC:
        {
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

int player::get_armor_bash_base(body_part bp) const
{
    int ret = 0;
    for (auto &i : worn) {
        if (i.covers(bp)) {
            ret += i.bash_resist();
        }
    }
    if (has_bionic("bio_carbon")) {
        ret += 2;
    }
    if (bp == bp_head && has_bionic("bio_armor_head")) {
        ret += 3;
    }
    if ((bp == bp_arm_l || bp == bp_arm_r) && has_bionic("bio_armor_arms")) {
        ret += 3;
    }
    if (bp == bp_torso && has_bionic("bio_armor_torso")) {
        ret += 3;
    }
    if ((bp == bp_leg_l || bp == bp_leg_r) && has_bionic("bio_armor_legs")) {
        ret += 3;
    }
    if (bp == bp_eyes && has_bionic("bio_armor_eyes")) {
        ret += 3;
    }

    ret += mutation_armor( bp, DT_BASH );
    return ret;
}

int player::get_armor_cut_base(body_part bp) const
{
    int ret = 0;
    for (auto &i : worn) {
        if (i.covers(bp)) {
            ret += i.cut_resist();
        }
    }
    if (has_bionic("bio_carbon")) {
        ret += 4;
    }
    if (bp == bp_head && has_bionic("bio_armor_head")) {
        ret += 3;
    } else if ((bp == bp_arm_l || bp == bp_arm_r) && has_bionic("bio_armor_arms")) {
        ret += 3;
    } else if (bp == bp_torso && has_bionic("bio_armor_torso")) {
        ret += 3;
    } else if ((bp == bp_leg_l || bp == bp_leg_r) && has_bionic("bio_armor_legs")) {
        ret += 3;
    } else if (bp == bp_eyes && has_bionic("bio_armor_eyes")) {
        ret += 3;
    }

    ret += mutation_armor( bp, DT_CUT );
    return ret;
}

int player::get_armor_acid(body_part bp) const
{
    return get_armor_type( DT_ACID, bp );
}

int player::get_armor_fire(body_part bp) const
{
    return get_armor_type( DT_HEAT, bp );
}

void destroyed_armor_msg( Character &who, const std::string &pre_damage_name )
{
    //~ %s is armor name
    who.add_memorial_log( pgettext("memorial_male", "Worn %s was completely destroyed."),
                          pgettext("memorial_female", "Worn %s was completely destroyed."),
                          pre_damage_name.c_str() );
    who.add_msg_player_or_npc( m_bad, _("Your %s is completely destroyed!"),
                               _("<npcname>'s %s is completely destroyed!"),
                               pre_damage_name.c_str() );
}

bool player::armor_absorb( damage_unit& du, item& armor )
{
    if( rng( 1, 100 ) > armor.get_coverage() ) {
        return false;
    }

    // TODO: add some check for power armor
    armor.mitigate_damage( du );

    // We want armor's own resistance to this type, not the resistance it grants
    const int armors_own_resist = armor.damage_resist( du.type, true );
    if( armors_own_resist > 1000 ) {
        // This is some weird type that doesn't damage armors
        return false;
    }

    // Scale chance of article taking damage based on the number of parts it covers.
    // This represents large articles being able to take more punishment
    // before becoming inneffective or being destroyed.
    const int num_parts_covered = armor.get_covered_body_parts().count();
    if( !one_in( num_parts_covered ) ) {
        return false;
    }

    // Don't damage armor as much when bypassed by armor piercing
    // Most armor piercing damage comes from bypassing armor, not forcing through
    const int raw_dmg = du.amount;
    if( raw_dmg > armors_own_resist ) {
        // If damage is above armor value, the chance to avoid armor damage is
        // 50% + 50% * 1/dmg
        if( one_in( raw_dmg ) || one_in( 2 ) ) {
            return false;
        }
    } else {
        // Sturdy items and power armors never take chip damage.
        // Other armors have 0.5% of getting damaged from hits below their armor value.
        if( armor.has_flag("STURDY") || armor.is_power_armor() || !one_in( 200 ) ) {
            return false;
        }
    }

    auto &material = armor.get_random_material();
    std::string damage_verb = ( du.type == DT_BASH ) ?
        material.bash_dmg_verb() : material.cut_dmg_verb();

    const std::string pre_damage_name = armor.tname();
    const std::string pre_damage_adj = armor.get_base_material().dmg_adj( armor.damage() );

    // add "further" if the damage adjective and verb are the same
    std::string format_string = ( pre_damage_adj == damage_verb ) ?
        _("Your %1$s is %2$s further!") : _("Your %1$s is %2$s!");
    add_msg_if_player( m_bad, format_string.c_str(), pre_damage_name.c_str(),
                       damage_verb.c_str());
    //item is damaged
    if( is_player() ) {
        SCT.add(posx(), posy(), NORTH, remove_color_tags( pre_damage_name ),
                m_neutral, damage_verb, m_info);
    }

    return armor.mod_damage( armor.has_flag( "FRAGILE" ) ? rng( 2, 3 ) : 1, du.type );
}

float player::bionic_armor_bonus( body_part bp, damage_type dt ) const
{
    float result = 0.0f;
    // We only check the passive bionics
    if( has_bionic( "bio_carbon" ) ) {
        if( dt == DT_BASH ) {
            result += 2;
        } else if( dt == DT_CUT ) {
            result += 4;
        } else if( dt == DT_STAB ) {
            result += 3.2;
        }
    }
    //all the other bionic armors reduce bash/cut/stab by 3/3/2.4
    // Map body parts to a set of bionics that protect it
    // @todo: JSONize passive bionic armor instead of hardcoding it
    static const std::map< body_part, std::string > armor_bionics = {
    { bp_head, { "bio_armor_head" } },
    { bp_arm_l, { "bio_armor_arms" } },
    { bp_arm_r, { "bio_armor_arms" } },
    { bp_torso, { "bio_armor_torso" } },
    { bp_leg_l, { "bio_armor_legs" } },
    { bp_leg_r, { "bio_armor_legs" } },
    { bp_eyes, { "bio_armor_eyes" } }
    };
    auto iter = armor_bionics.find( bp );
    if( iter != armor_bionics.end() ) {
        if( has_bionic( iter->second ) ) {
            if( dt == DT_BASH || dt == DT_CUT ) {
                result += 3;
            } else if( dt == DT_STAB ) {
                result += 2.4;
            }
        }
    }
    return result;
}

void player::passive_absorb_hit( body_part bp, damage_unit &du ) const
{
    du.amount -= bionic_armor_bonus( bp, du.type ); //Check for passive armor bionics
    // >0 check because some mutations provide negative armor
        if( du.amount > 0.0f ) {
            // Horrible hack warning!
            // Get rid of this as soon as CUT and STAB are split
            if( du.type == DT_STAB ) {
                damage_unit du_copy = du;
                du_copy.type = DT_CUT;
                du.amount -= 0.8f * mutation_armor( bp, du_copy );
            } else {
                du.amount -= mutation_armor( bp, du );
            }
        }
        du.amount -= mabuff_armor_bonus( du.type );
        du.amount = std::max( 0.0f, du.amount );
}

void player::absorb_hit(body_part bp, damage_instance &dam) {
    std::list<item> worn_remains;
    bool armor_destroyed = false;

    for( auto &elem : dam.damage_units ) {
        if( elem.amount < 0 ) {
            // Prevents 0 damage hits (like from hallucinations) from ripping armor
            elem.amount = 0;
            continue;
        }

        // The bio_ads CBM absorbs damage before hitting armor
        if( has_active_bionic("bio_ads") ) {
            if( elem.amount > 0 && power_level > 24 ) {
                if( elem.type == DT_BASH ) {
                    elem.amount -= rng( 1, 8 );
                } else if( elem.type == DT_CUT ) {
                    elem.amount -= rng( 1, 4 );
                } else if( elem.type == DT_STAB ) {
                    elem.amount -= rng( 1, 2 );
                }
                charge_power(-25);
            }
            if( elem.amount < 0 ) {
                elem.amount = 0;
            }
        }

        // Only the outermost armor can be set on fire
        bool outermost = true;
        // The worn vector has the innermost item first, so
        // iterate reverse to damage the outermost (last in worn vector) first.
        for( auto iter = worn.rbegin(); iter != worn.rend(); ) {
            item& armor = *iter;

            if( !armor.covers( bp ) ) {
                ++iter;
                continue;
            }

            const std::string pre_damage_name = armor.tname();
            bool destroy = false;

            // Heat damage can set armor on fire
            // Even though it doesn't cause direct physical damage to it
            if( outermost && elem.type == DT_HEAT && elem.amount >= 1.0f ) {
                // @todo Different fire intensity values based on damage
                fire_data frd{ 2, 0.0f, 0.0f };
                destroy = armor.burn( frd );
                int fuel = roll_remainder( frd.fuel_produced );
                if( fuel > 0 ) {
                    add_effect( effect_onfire, fuel + 1, bp );
                }
            }

            if( !destroy ) {
                destroy = armor_absorb( elem, armor );
            }

            if( destroy ) {
                if( g->u.sees( *this ) ) {
                    SCT.add( posx(), posy(), NORTH, remove_color_tags( pre_damage_name ),
                             m_neutral, _( "destroyed" ), m_info);
                }
                destroyed_armor_msg( *this, pre_damage_name );
                armor_destroyed = true;
                armor.on_takeoff( *this );
                worn_remains.insert( worn_remains.end(), armor.contents.begin(), armor.contents.end() );
                // decltype is the typename of the iterator, ote that reverse_iterator::base returns the
                // iterator to the next element, not the one the revers_iterator points to.
                // http://stackoverflow.com/questions/1830158/how-to-call-erase-with-a-reverse-iterator
                iter = decltype(iter)( worn.erase( --iter.base() ) );
            } else {
                ++iter;
                outermost = false;
            }
        }

        passive_absorb_hit( bp, elem );

        if( elem.type == DT_BASH ) {
            if( has_trait( trait_LIGHT_BONES ) ) {
                elem.amount *= 1.4;
            }
            if( has_trait( trait_HOLLOW_BONES ) ) {
                elem.amount *= 1.8;
            }
        }

        elem.amount = std::max( elem.amount, 0.0f );
    }
    for( item& remain : worn_remains ) {
        g->m.add_item_or_charges( pos(), remain );
    }
    if( armor_destroyed ) {
        drop_inventory_overflow();
    }
}

int player::get_env_resist(body_part bp) const
{
    int ret = 0;
    for (auto &i : worn) {
        // Head protection works on eyes too (e.g. baseball cap)
        if (i.covers(bp) || (bp == bp_eyes && i.covers(bp_head))) {
            ret += i.get_env_resist();
        }
    }

    if (bp == bp_mouth && has_bionic("bio_purifier") && ret < 5) {
        ret += 2;
        if (ret > 5) {
            ret = 5;
        }
    }

    if (bp == bp_eyes && has_bionic("bio_armor_eyes") && ret < 5) {
        ret += 2;
        if (ret > 5) {
            ret = 5;
        }
    }
    return ret;
}

bool player::wearing_something_on(body_part bp) const
{
    for (auto &i : worn) {
        if (i.covers(bp))
            return true;
    }
    return false;
}


bool player::natural_attack_restricted_on( body_part bp ) const
{
    for( auto &i : worn ) {
        if( i.covers( bp ) && !i.has_flag( "ALLOWS_NATURAL_ATTACKS" ) ) {
            return true;
        }
    }
    return false;
}

bool player::is_wearing_shoes(std::string side) const
{
    bool left = true;
    bool right = true;
    if (side == "left" || side == "both") {
        left = false;
        for (auto &i : worn) {
            const item *worn_item = &i;
            if (i.covers(bp_foot_l) &&
                !worn_item->has_flag("BELTED") &&
                !worn_item->has_flag("SKINTIGHT")) {
                left = true;
                break;
            }
        }
    }
    if (side == "right" || side == "both") {
        right = false;
        for (auto &i : worn) {
            const item *worn_item = &i;
            if (i.covers(bp_foot_r) &&
                !worn_item->has_flag("BELTED") &&
                !worn_item->has_flag("SKINTIGHT")) {
                right = true;
                break;
            }
        }
    }
    return (left && right);
}

double player::footwear_factor() const
{
    double ret = 0;
    if (wearing_something_on(bp_foot_l)) {
        ret += .5;
    }
    if (wearing_something_on(bp_foot_r)) {
        ret += .5;
    }
    return ret;
}

int player::shoe_type_count(const itype_id & it) const
{
    int ret = 0;
    if (is_wearing_on_bp(it, bp_foot_l)) {
        ret++;
    }
    if (is_wearing_on_bp(it, bp_foot_r)) {
        ret++;
    }
    return ret;
}

bool player::is_wearing_power_armor(bool *hasHelmet) const {
    bool result = false;
    for( auto &elem : worn ) {
        if( !elem.is_power_armor() ) {
            continue;
        }
        if (hasHelmet == NULL) {
            // found power armor, helmet not requested, cancel loop
            return true;
        }
        // found power armor, continue search for helmet
        result = true;
        if( elem.covers( bp_head ) ) {
            *hasHelmet = true;
            return true;
        }
    }
    return result;
}

int player::adjust_for_focus(int amount) const
{
    int effective_focus = focus_pool;
    if (has_trait( trait_FASTLEARNER ))
    {
        effective_focus += 15;
    }
    if (has_trait( trait_SLOWLEARNER ))
    {
        effective_focus -= 15;
    }
    double tmp = amount * (effective_focus / 100.0);
    int ret = int(tmp);
    if (rng(0, 100) < 100 * (tmp - ret))
    {
        ret++;
    }
    return ret;
}

void player::practice( const skill_id &id, int amount, int cap )
{
    SkillLevel &level = get_skill_level( id );
    const Skill &skill = id.obj();
    // Double amount, but only if level.exercise isn't a small negative number?
    if (level.exercise() < 0) {
        if (amount >= -level.exercise()) {
            amount -= level.exercise();
        } else {
            amount += amount;
        }
    }

    if( !level.can_train() ) {
        // If leveling is disabled, don't train, don't drain focus, don't print anything
        // Leaving as a skill method rather than global for possible future skill cap setting
        return;
    }

    bool isSavant = has_trait( trait_SAVANT );

    skill_id savantSkill( skill_id::NULL_ID );
    SkillLevel savantSkillLevel = SkillLevel();

    if (isSavant) {
        for( auto const &skill : Skill::skills ) {
            if( get_skill_level( skill.ident() ) > savantSkillLevel ) {
                savantSkill = skill.ident();
                savantSkillLevel = get_skill_level( skill.ident() );
            }
        }
    }

    amount = adjust_for_focus(amount);

    if (has_trait( trait_PACIFIST ) && skill.is_combat_skill()) {
        if(!one_in(3)) {
          amount = 0;
        }
    }
    if (has_trait( trait_PRED2 ) && skill.is_combat_skill()) {
        if(one_in(3)) {
          amount *= 2;
        }
    }
    if (has_trait( trait_PRED3 ) && skill.is_combat_skill()) {
        amount *= 2;
    }

    if (has_trait( trait_PRED4 ) && skill.is_combat_skill()) {
        amount *= 3;
    }

    if (isSavant && id != savantSkill ) {
        amount /= 2;
    }



    if (get_skill_level( id ) > cap) { //blunt grinding cap implementation for crafting
        amount = 0;
        int curLevel = get_skill_level( id );
        if(is_player() && one_in(5)) {//remind the player intermittently that no skill gain takes place
            add_msg(m_info, _("This task is too simple to train your %s beyond %d."),
                    skill.name().c_str(), curLevel);
        }
    }

    if (amount > 0 && level.isTraining()) {
        int oldLevel = get_skill_level( id );
        get_skill_level( id ).train(amount);
        int newLevel = get_skill_level( id );
        if (is_player() && newLevel > oldLevel) {
            add_msg(m_good, _("Your skill in %s has increased to %d!"), skill.name().c_str(), newLevel);
            lua_callback("on_skill_increased");
        }
        if(is_player() && newLevel > cap) {
            //inform player immediately that the current recipe can't be used to train further
            add_msg(m_info, _("You feel that %s tasks of this level are becoming trivial."),
                    skill.name().c_str());
        }

        int chance_to_drop = focus_pool;
        focus_pool -= chance_to_drop / 100;
        // Apex Predators don't think about much other than killing.
        // They don't lose Focus when practicing combat skills.
        if ((rng(1, 100) <= (chance_to_drop % 100)) && (!(has_trait( trait_PRED4 ) &&
                                                          skill.is_combat_skill()))) {
            focus_pool--;
        }
    }

    get_skill_level( id ).practice();
}

int player::exceeds_recipe_requirements( const recipe &rec ) const
{
    int over = rec.skill_used ? get_skill_level( rec.skill_used ) - rec.difficulty : 0;
    for( const auto &elem : compare_skill_requirements( rec.required_skills ) ) {
        over = std::min( over, elem.second );
    }
    return over;
}

bool player::has_recipe_requirements( const recipe &rec ) const
{
    return exceeds_recipe_requirements( rec ) >= 0;
}

bool player::can_decomp_learn( const recipe &rec ) const
{
    return !rec.learn_by_disassembly.empty() &&
           meets_skill_requirements( rec.learn_by_disassembly );
}

bool player::knows_recipe(const recipe *rec) const
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

void player::learn_recipe( const recipe * const rec )
{
    learned_recipes.include( rec );
}

void player::assign_activity( activity_id type, int moves, int index, int pos, std::string name )
{
    assign_activity( player_activity( type, moves, index, pos, name ) );
}

void player::assign_activity( const player_activity &act, bool allow_resume )
{
    if( allow_resume && !backlog.empty() && backlog.front().can_resume_with( act, *this ) ) {
        add_msg_if_player( _("You resume your task.") );
        activity = backlog.front();
        backlog.pop_front();
    } else {
        if( activity ) {
            backlog.push_front( activity );
        }

        activity = act;
    }

    activity.warned_of_proximity = false;

    if( activity.rooted() ) {
        rooted_message();
    }
}

bool player::has_activity(const activity_id type) const
{
    return activity.id() == type;
}

void player::cancel_activity()
{
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
    activity = player_activity();
}

bool player::has_gun_for_ammo( const ammotype &at ) const
{
    return has_item_with( [at]( const item & it ) {
        // item::ammo_type considers the active gunmod.
        return it.is_gun() && it.ammo_type() == at;
    } );
}

bool player::has_magazine_for_ammo( const ammotype &at ) const
{
    return has_item_with( [&at]( const item & it ) {
        return !it.has_flag( "NO_RELOAD" ) &&
               ( ( it.is_magazine() && it.ammo_type() == at ) ||
                 ( it.is_gun() && it.magazine_integral() && it.ammo_type() == at ) ||
                 ( it.is_gun() && it.magazine_current() != nullptr &&
                   it.magazine_current()->ammo_type() == at ) );
    } );
}

std::string player::weapname() const
{
    if( weapon.is_gun() ) {
        std::stringstream str;
        str << weapon.type_name();

        // Is either the base item or at least one auxiliary gunmod loaded (includes empty magazines)
        bool base = weapon.ammo_capacity() > 0 && !weapon.has_flag( "RELOAD_AND_SHOOT" );

        const auto mods = weapon.gunmods();
        bool aux = std::any_of( mods.begin(), mods.end(), [&]( const item *e ) {
            return e->is_gun() && e->ammo_capacity() > 0 && !e->has_flag( "RELOAD_AND_SHOOT" );
        } );

        if( base || aux ) {
            str << " (";
            if( base ) {
                str << weapon.ammo_remaining();
                if( weapon.magazine_integral() ) {
                    str << "/" << weapon.ammo_capacity();
                }
            } else {
                str << "---";
            }
            str << ")";

            for( auto e : mods ) {
                if( e->is_gun() && e->ammo_capacity() > 0 && !e->has_flag( "RELOAD_AND_SHOOT" ) ) {
                    str << " (" << e->ammo_remaining();
                    if( e->magazine_integral() ) {
                        str << "/" << e->ammo_capacity();
                    }
                    str << ")";
                }
            }
        }
        return str.str();

    } else if( weapon.is_container() && weapon.contents.size() == 1 ) {
        return string_format( "%s (%d)", weapon.tname().c_str(), weapon.contents.front().charges );

    } else if( !is_armed() ) {
        return _( "fists" );

    } else {
        return weapon.tname();
    }
}

bool player::wield_contents( item &container, int pos, bool penalties, int base_cost )
{
    // if index not specified and container has multiple items then ask the player to choose one
    if( pos < 0 ) {
        std::vector<std::string> opts;
        std::transform( container.contents.begin(), container.contents.end(),
                        std::back_inserter( opts ), []( const item& elem ) {
                            return elem.display_name();
                        } );
        if( opts.size() > 1 ) {
            pos = ( uimenu( false, _("Wield what?"), opts ) ) - 1;
        } else {
            pos = 0;
        }
    }

    if( pos >= static_cast<int>(container.contents.size() ) ) {
        debugmsg( "Tried to wield non-existent item from container (player::wield_contents)" );
        return false;
    }

    auto target = std::next( container.contents.begin(), pos );
    if( !can_wield( *target ) ) {
        return false;
    }

    int mv = 0;

    if( is_armed() ) {
        if( !wield( ret_null ) ) {
            return false;
        }
        inv.unsort();
    }

    weapon = std::move( *target );
    container.contents.erase( target );
    container.on_contents_changed();

    inv.assign_empty_invlet( weapon, true );
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

void player::store( item& container, item& put, bool penalties, int base_cost )
{
    moves -= item_store_cost( put, container, penalties, base_cost );
    container.put_in( i_rem( &put ) );
}

nc_color encumb_color(int level)
{
 if (level < 0)
  return c_green;
 if (level < 10)
  return c_ltgray;
 if (level < 40)
  return c_yellow;
 if (level < 70)
  return c_ltred;
 return c_red;
}

float player::get_melee() const
{
    return get_skill_level( skill_id( "melee" ) );
}

void player::setID (int i)
{
    if( id >= 0 ) {
        debugmsg( "tried to set id of a npc/player, but has already a id: %d", id );
    } else if( i < -1 ) {
        debugmsg( "tried to set invalid id of a npc/player: %d", i );
    } else {
        id = i;
    }
}

int player::getID () const
{
    return this->id;
}

bool player::uncanny_dodge()
{
    bool is_u = this == &g->u;
    bool seen = g->u.sees( *this );
    if( this->power_level < 74 || !this->has_active_bionic("bio_uncanny_dodge") ) { return false; }
    tripoint adjacent = adjacent_tile();
    charge_power(-75);
    if( adjacent.x != posx() || adjacent.y != posy()) {
        position.x = adjacent.x;
        position.y = adjacent.y;
        if( is_u ) {
            add_msg( _("Time seems to slow down and you instinctively dodge!") );
        } else if( seen ) {
            add_msg( _("%s dodges... so fast!"), this->disp_name().c_str() );

        }
        return true;
    }
    if( is_u ) {
        add_msg( _("You try to dodge but there's no room!") );
    } else if( seen ) {
        add_msg( _("%s tries to dodge but there's no room!"), this->disp_name().c_str() );
    }
    return false;
}

// adjacent_tile() returns a safe, unoccupied adjacent tile. If there are no such tiles, returns player position instead.
tripoint player::adjacent_tile() const
{
    std::vector<tripoint> ret;
    int dangerous_fields;
    for( const tripoint &p : g->m.points_in_radius( pos(), 1 ) ) {
        if( p == pos() ) {
            // Don't consider player position
            continue;
        }
        const trap &curtrap = g->m.tr_at( p );
        if( g->mon_at( p ) == -1 && g->npc_at( p ) == -1 && g->m.passable( p ) &&
            (curtrap.is_null() || curtrap.is_benign()) ) {
            // Only consider tile if unoccupied, passable and has no traps
            dangerous_fields = 0;
            auto &tmpfld = g->m.field_at( p );
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
    }

    return random_entry( ret, pos() ); // player position if no valid adjacent tiles
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

    for (int part = 0; part < num_hp_parts; part++) {
        hp_cur[part] = hp_max[part];
    }
    set_hunger(0);
    set_thirst(0);
    set_fatigue(0);
    set_healthy(0);
    set_healthy_mod(0);
    stim = 0;
    set_pain(0);
    set_painkiller(0);
    radiation = 0;

    recalc_sight_limits();
    reset_encumbrance();
}

bool player::is_invisible() const
{
    static const std::string str_bio_cloak("bio_cloak"); // This function used in monster::plan_moves
    static const std::string str_bio_night("bio_night");
    return (
        has_active_bionic(str_bio_cloak) ||
        has_active_bionic(str_bio_night) ||
        has_active_optcloak() ||
        has_trait( trait_DEBUG_CLOAK ) ||
        has_artifact_with(AEP_INVISIBLE)
    );
}

int player::visibility( bool, int ) const
{
    // 0-100 %
    if ( is_invisible() ) {
        return 0;
    }
    // todo:
    // if ( dark_clothing() && light check ...
    return 100;
}

void player::set_destination(const std::vector<tripoint> &route)
{
    auto_move_route = route;
}

void player::clear_destination()
{
    auto_move_route.clear();
    next_expected_position = tripoint_min;
}

bool player::has_destination() const
{
    return !auto_move_route.empty();
}

std::vector<tripoint> &player::get_auto_move_route()
{
    return auto_move_route;
}

action_id player::get_next_auto_move_direction()
{
    if (!has_destination()) {
        return ACTION_NULL;
    }

    if (next_expected_position != tripoint_min ) {
        if( pos() != next_expected_position ) {
            // We're off course, possibly stumbling or stuck, cancel auto move
            return ACTION_NULL;
        }
    }

    next_expected_position = auto_move_route.front();
    auto_move_route.erase(auto_move_route.begin());

    tripoint dp = next_expected_position - pos();

    // Make sure the direction is just one step and that
    // all diagonal moves have 0 z component
    if( abs( dp.x ) > 1 || abs( dp.y ) > 1 || abs( dp.z ) > 1 ||
        ( abs( dp.z ) != 0 && ( abs( dp.x ) != 0 || abs( dp.y ) != 0 ) ) ) {
        // Should never happen, but check just in case
        return ACTION_NULL;
    }

    return get_movement_direction_from_delta( dp.x, dp.y, dp.z );
}

void player::shift_destination(int shiftx, int shifty)
{
    if( next_expected_position != tripoint_min ) {
        next_expected_position.x += shiftx;
        next_expected_position.y += shifty;
    }

    for( auto &elem : auto_move_route ) {
        elem.x += shiftx;
        elem.y += shifty;
    }
}

bool player::has_weapon() const
{
    return !unarmed_attack();
}

m_size player::get_size() const
{
    return MS_MEDIUM;
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
    if (has_trait( trait_BADCARDIO ))
        return 750;
    if (has_trait( trait_GOODCARDIO ))
        return 1250;
    return 1000;
}

void player::burn_move_stamina( int moves )
{
    int overburden_percentage = 0;
    int current_weight = weight_carried();
    int max_weight = weight_capacity();
    if (current_weight > max_weight) {
        overburden_percentage = (current_weight - max_weight) * 100 / max_weight;
    }
    // Regain 10 stamina / turn
    // 7/turn walking
    // 20/turn running
    int burn_ratio = 7;
    burn_ratio += overburden_percentage;
    if( move_mode == "run" ) {
        burn_ratio = burn_ratio * 3 - 1;
    }
    mod_stat( "stamina", -((moves * burn_ratio) / 100) );
    // Chance to suffer pain if overburden and stamina runs out or has trait BADBACK
    // Starts at 1 in 25, goes down by 5 for every 50% more carried
    if ((current_weight > max_weight) && (has_trait( trait_BADBACK ) || stamina == 0) && one_in(35 - 5 * current_weight / (max_weight / 2))) {
        add_msg_if_player(m_bad, _("Your body strains under the weight!"));
        // 1 more pain for every 800 grams more (5 per extra STR needed)
        if ( ((current_weight - max_weight) / 800 > get_pain() && get_pain() < 100)) {
            mod_pain(1);
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
        } else if( p->is_friend() ) {
            return A_FRIENDLY;
        } else {
            return A_NEUTRAL;
        }
    } else if( &other == this ) {
        return A_FRIENDLY;
    }

    return A_NEUTRAL;
}

bool player::sees( const tripoint &t, bool ) const
{
    static const std::string str_bio_night("bio_night");
    const int wanted_range = rl_dist( pos(), t );
    bool can_see = is_player() ? g->m.pl_sees( t, wanted_range ) :
        Creature::sees( t );
    // Clairvoyance is now pretty cheap, so we can check it early
    if( wanted_range < MAX_CLAIRVOYANCE && wanted_range < clairvoyance() ) {
        return true;
    }
    // Only check if we need to override if we already came to the opposite conclusion.
    if( can_see && wanted_range < 15 && wanted_range > sight_range(1) &&
        has_active_bionic(str_bio_night) ) {
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
    if (dist <= 3 && has_trait( trait_ANTENNAE )) {
        return true;
    }
    if( critter.digging() && has_active_bionic( "bio_ground_sonar" ) ) {
        // Bypass the check below, the bionic sonar also bypasses the sees(point) check because
        // walls don't block sonar which is transmitted in the ground, not the air.
        // TODO: this might need checks whether the player is in the air, or otherwise not connected
        // to the ground. It also might need a range check.
        return true;
    }
    return Creature::sees( critter );
}

nc_color player::bodytemp_color(int bp) const
{
  nc_color color =  c_ltgray; // default
    if (bp == bp_eyes) {
        color = c_ltgray;    // Eyes don't count towards warmth
    } else if (temp_conv[bp] >  BODYTEMP_SCORCHING) {
        color = c_red;
    } else if (temp_conv[bp] >  BODYTEMP_VERY_HOT) {
        color = c_ltred;
    } else if (temp_conv[bp] >  BODYTEMP_HOT) {
        color = c_yellow;
    } else if (temp_conv[bp] >  BODYTEMP_COLD) {
        color = c_green;
    } else if (temp_conv[bp] >  BODYTEMP_VERY_COLD) {
        color = c_ltblue;
    } else if (temp_conv[bp] >  BODYTEMP_FREEZING) {
        color = c_cyan;
    } else if (temp_conv[bp] <= BODYTEMP_FREEZING) {
        color = c_blue;
    }
    return color;
}

//message related stuff
void player::add_msg_if_player(const char* msg, ...) const
{
    va_list ap;
    va_start(ap, msg);
    Messages::vadd_msg(msg, ap);
    va_end(ap);
}
void player::add_msg_player_or_npc(const char* player_str, const char* npc_str, ...) const
{
    va_list ap;
    va_start(ap, npc_str);
    Messages::vadd_msg(player_str, ap);
    va_end(ap);
}
void player::add_msg_if_player(game_message_type type, const char* msg, ...) const
{
    va_list ap;
    va_start(ap, msg);
    Messages::vadd_msg(type, msg, ap);
    va_end(ap);
}
void player::add_msg_player_or_npc(game_message_type type, const char* player_str, const char* npc_str, ...) const
{
    va_list ap;
    va_start(ap, npc_str);
    Messages::vadd_msg(type, player_str, ap);
    va_end(ap);
}

void player::add_msg_player_or_say( const char *player_str, const char *npc_str, ... ) const
{
    va_list ap;
    va_start( ap, npc_str );
    Messages::vadd_msg( player_str, ap );
    va_end(ap);
}

void player::add_msg_player_or_say( game_message_type type, const char *player_str, const char *npc_str, ... ) const
{
    va_list ap;
    va_start( ap, npc_str );
    Messages::vadd_msg( type, player_str, ap );
    va_end(ap);
}

bool player::knows_trap( const tripoint &pos ) const
{
    const tripoint p = g->m.getabs( pos );
    return known_traps.count( p ) > 0;
}

void player::add_known_trap( const tripoint &pos, const trap &t)
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
    return get_effect_int( effect_deaf ) > 2 || worn_with_flag("DEAF") ||
           (has_active_bionic("bio_earplugs") && !has_active_bionic("bio_ears"));
}

bool player::can_hear( const tripoint &source, const int volume ) const
{
    if( is_deaf() ) {
        return false;
    }

    // source is in-ear and at our square, we can hear it
    if ( source == pos() && volume == 0 ) {
        return true;
    }

    // TODO: sound attenuation due to weather

    const int dist = rl_dist( source, pos() );
    const float volume_multiplier = hearing_ability();
    return volume * volume_multiplier >= dist;
}

float player::hearing_ability() const
{
    float volume_multiplier = 1.0;

    // Mutation/Bionic volume modifiers
    if( has_active_bionic("bio_ears") && !has_active_bionic("bio_earplugs") ) {
        volume_multiplier *= 3.5;
    }
    if( has_trait( trait_PER_SLIME ) ) {
        // Random hearing :-/
        // (when it's working at all, see player.cpp)
        // changed from 0.5 to fix Mac compiling error
        volume_multiplier *= (rng(1, 2));
    }
    if( has_trait( trait_BADHEARING ) ) {
        volume_multiplier *= .5;
    }
    if( has_trait( trait_GOODHEARING ) ) {
        volume_multiplier *= 1.25;
    }
    if( has_trait( trait_CANINE_EARS ) ) {
        volume_multiplier *= 1.5;
    }
    if( has_trait( trait_URSINE_EARS ) || has_trait( trait_FELINE_EARS ) ) {
        volume_multiplier *= 1.25;
    }
    if( has_trait( trait_LUPINE_EARS ) ) {
        volume_multiplier *= 1.75;
    }

    if( has_effect( effect_deaf ) ) {
        // Scale linearly up to 300
        volume_multiplier *= ( 300.0 - get_effect_dur( effect_deaf ) ) / 300.0;
    }

    if( has_effect( effect_earphones ) ) {
        volume_multiplier *= .25;
    }

    return volume_multiplier;
}

int player::print_info(WINDOW* w, int vStart, int, int column) const
{
    mvwprintw( w, vStart++, column, _( "You (%s)" ), name.c_str() );
    return vStart;
}

std::vector<Creature *> get_creatures_if( std::function<bool (const Creature &)>pred )
{
    std::vector<Creature *> result;
    for( size_t i = 0; i < g->num_zombies(); i++ ) {
        auto &critter = g->zombie( i );
        if( !critter.is_dead() && pred( critter ) ) {
            result.push_back( &critter );
        }
    }
    for( auto & n : g->active_npc ) {
        if( pred( *n ) ) {
            result.push_back( n );
        }
    }
    if( pred( g->u ) ) {
        result.push_back( &g->u );
    }
    return result;
}

bool player::is_visible_in_range( const Creature &critter, const int range ) const
{
    return sees( critter ) && rl_dist( pos(), critter.pos() ) <= range;
}

std::vector<Creature *> player::get_visible_creatures( const int range ) const
{
    return get_creatures_if( [this, range]( const Creature &critter ) -> bool {
        return this != &critter && pos() != critter.pos() && // @todo get rid of fake npcs (pos() check)
          rl_dist( pos(), critter.pos() ) <= range && sees( critter );
    } );
}

std::vector<Creature *> player::get_targetable_creatures( const int range ) const
{
    return get_creatures_if( [this, range]( const Creature &critter ) -> bool {
        return this != &critter && pos() != critter.pos() && // @todo get rid of fake npcs (pos() check)
          rl_dist( pos(), critter.pos() ) <= range &&
          ( sees( critter ) || sees_with_infrared( critter ) );
    } );
}

std::vector<Creature *> player::get_hostile_creatures( int range ) const
{
    return get_creatures_if( [this, range] ( const Creature &critter ) -> bool {
        return this != &critter && pos() != critter.pos() && // @todo get rid of fake npcs (pos() check)
            rl_dist( pos(), critter.pos() ) <= range &&
            critter.attitude_to( *this ) == A_HOSTILE && sees( critter );
    } );
}

void player::place_corpse()
{
    std::vector<item *> tmp = inv_dump();
    item body = item::make_corpse( mtype_id::NULL_ID, calendar::turn, name );
    for( auto itm : tmp ) {
        g->m.add_item_or_charges( pos(), *itm );
    }
    for( auto & bio : my_bionics ) {
        if( item::type_is_defined( bio.id ) ) {
            body.put_in( item( bio.id, calendar::turn ) );
        }
    }

    // Restore amount of installed pseudo-modules of Power Storage Units
    std::pair<int, int> storage_modules = amount_of_storage_bionics();
    for (int i = 0; i < storage_modules.first; ++i) {
        body.emplace_back( "bio_power_storage" );
    }
    for (int i = 0; i < storage_modules.second; ++i) {
        body.emplace_back( "bio_power_storage_mkII" );
    }
    g->m.add_item_or_charges( pos(), body );
}

bool player::sees_with_infrared( const Creature &critter ) const
{
    if( !vision_mode_cache[IR_VISION] || !critter.is_warm() ) {
        return false;
    }

    if( is_player() || critter.is_player() ) {
        // Players should not use map::sees
        // Likewise, players should not be "looked at" with map::sees, not to break symmetry
        return g->m.pl_line_of_sight( critter.pos(), sight_range( DAYLIGHT_LEVEL ) );
    }

    return g->m.sees( pos(), critter.pos(), sight_range( DAYLIGHT_LEVEL ) );
}

std::vector<std::string> player::get_overlay_ids() const
{
    std::vector<std::string> rval;
    std::multimap<int, std::string> mutation_sorting;


    // first get effects
    for( const auto &eff_pr : effects ) {
        rval.push_back( "effect_" + eff_pr.first.str() );
    }

    // then get mutations
    for( auto &mutation : get_mutations() ) {
        auto it = base_mutation_overlay_ordering.find( mutation );
        auto it2 = tileset_mutation_overlay_ordering.find( mutation );
        int value = 9999;
        if( it != base_mutation_overlay_ordering.end() ) {
            value = it->second;
        }
        if( it2 != tileset_mutation_overlay_ordering.end() ) {
            value = it2->second;
        }
        mutation_sorting.insert( std::pair<int, std::string>( value, mutation.str() ) );
    }

    for( auto &mutorder : mutation_sorting ) {
        rval.push_back( "mutation_" + mutorder.second );
    }

    // next clothing
    // TODO: worry about correct order of clothing overlays
    for( const item &worn_item : worn ) {
        rval.push_back( "worn_" + worn_item.typeId() );
    }

    // last weapon
    // TODO: might there be clothing that covers the weapon?
    if( is_armed() ) {
        rval.push_back( "wielded_" + weapon.typeId() );
    }
    return rval;
}

void player::spores()
{
    fungal_effects fe( *g, g->m );
    //~spore-release sound
    sounds::sound( pos(), 10, _("Pouf!"));
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) {
                continue;
            }

            tripoint sporep( posx() + i, posy() + j, posz() );
            fe.fungalize( sporep, this, 0.25 );
        }
    }
}

void player::blossoms()
{
    // Player blossoms are shorter-ranged, but you can fire much more frequently if you like.
    sounds::sound( pos(), 10, _("Pouf!"));
    tripoint tmp = pos();
    int &i = tmp.x;
    int &j = tmp.y;
    for ( i = posx() - 2; i <= posx() + 2; i++) {
        for ( j = posy() - 2; j <= posy() + 2; j++) {
            g->m.add_field( tmp, fd_fungal_haze, rng(1, 2), 0 );
        }
    }
}

float player::power_rating() const
{
    int dmg = std::max( { weapon.damage_melee( DT_BASH ),
                          weapon.damage_melee( DT_CUT ),
                          weapon.damage_melee( DT_STAB ) } );

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
    if( move_mode != "run" ) {
        ret *= 1.0f + ((float)stamina / (float)get_stamina_max());
    }

    return ret;
}

std::vector<const item *> player::all_items_with_flag( const std::string flag ) const
{
    return items_with( [&flag]( const item & it ) {
        return it.has_flag( flag );
    } );
}

bool player::has_item_with_flag( const std::string &flag ) const
{
    return has_item_with( [&flag]( const item & it ) {
        return it.has_flag( flag );
    } );
}

void player::on_mutation_gain( const trait_id &mid )
{
    morale->on_mutation_gain( mid );
}

void player::on_mutation_loss( const trait_id &mid )
{
    morale->on_mutation_loss( mid );
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

void player::on_mission_assignment( mission &new_mission )
{
    active_missions.push_back( &new_mission );
    set_active_mission( new_mission );
}

void player::on_mission_finished( mission &mission )
{
    if( mission.has_failed() ) {
        failed_missions.push_back( &mission );
        add_msg_if_player( m_bad, _( "Mission \"%s\" is failed." ), mission.name().c_str() );
    } else {
        completed_missions.push_back( &mission );
        add_msg_if_player( m_good, _( "Mission \"%s\" is successfully completed." ), mission.name().c_str() );
    }
    const auto iter = std::find( active_missions.begin(), active_missions.end(), &mission );
    if( iter == active_missions.end() ) {
        debugmsg( "completed mission %d was not in the active_missions list", mission.get_id() );
    } else {
        active_missions.erase( iter );
    }
    if( &mission == active_mission ) {
        if( active_missions.empty() ) {
            active_mission = nullptr;
        } else {
            active_mission = active_missions.front();
        }
    }
}

const targeting_data &player::get_targeting_data() {
    if( tdata.get() == nullptr ) {
        debugmsg( "Tried to get targeting data before setting it" );
        tdata.reset( new targeting_data() );
        tdata->relevant = nullptr;
    }

    return *tdata;
}

void player::set_targeting_data( const targeting_data &td ) {
    tdata.reset( new targeting_data( td ) );
}

void player::set_active_mission( mission &mission )
{
    const auto iter = std::find( active_missions.begin(), active_missions.end(), &mission );
    if( iter == active_missions.end() ) {
        debugmsg( "new active mission %d is not in the active_missions list", mission.get_id() );
    } else {
        active_mission = &mission;
    }
}

mission *player::get_active_mission() const
{
    return active_mission;
}

tripoint player::get_active_mission_target() const
{
    if( active_mission == nullptr ) {
        return overmap::invalid_tripoint;
    }
    return active_mission->get_target();
}

std::vector<mission*> player::get_active_missions() const
{
    return active_missions;
}

std::vector<mission*> player::get_completed_missions() const
{
    return completed_missions;
}

std::vector<mission*> player::get_failed_missions() const
{
    return failed_missions;
}

bool player::query_yn( const char *mes, ... ) const
{
    va_list ap;
    va_start( ap, mes );
    bool ret = internal_query_yn( mes, ap );
    va_end( ap );
    return ret;
}

const pathfinding_settings &player::get_pathfinding_settings() const
{
    return path_settings;
}

std::set<tripoint> player::get_path_avoid() const
{
    std::set<tripoint> ret;
    for( const npc *np : g->active_npc ) {
        if( sees( *np ) ) {
            ret.insert( np->pos() );
        }
    }

    // @todo Add known traps in a way that doesn't destroy performance

    return ret;
}
