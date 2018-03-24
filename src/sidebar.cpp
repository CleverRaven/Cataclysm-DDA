#include "player.h"

#include "action.h"
#include "coordinate_conversions.h"
#include "profession.h"
#include "itype.h"
#include "string_formatter.h"
#include "bionics.h"
#include "mapdata.h"
#include "mission.h"
#include "game.h"
#include "map.h"
#include "filesystem.h"
#include "fungal_effects.h"
#include "debug.h"
#include "addiction.h"
#include "inventory.h"
#include "skill.h"
#include "options.h"
#include "weather.h"
#include "item.h"
#include "material.h"
#include "translations.h"
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
#include "effect.h"
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
#include "overlay_ordering.h"
#include "vitamin.h"
#include "fault.h"
#include "recipe_dictionary.h"
#include "ranged.h"
#include "ammo.h"

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
static const trait_id trait_MOREPAIN( "MORE_PAIN" );
static const trait_id trait_MOREPAIN2( "MORE_PAIN2" );
static const trait_id trait_MOREPAIN3( "MORE_PAIN3" );
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
static const trait_id trait_THRESH_BIRD( "THRESH_BIRD" );
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

void player::print_stamina_bar( const catacurses::window &w ) const
{
    std::string sta_bar;
    nc_color sta_color;
    std::tie( sta_bar, sta_color ) = get_hp_bar( stamina, get_stamina_max() );
    wprintz( w, sta_color, sta_bar.c_str() );
}

void player::disp_status( const catacurses::window &w, const catacurses::window &w2 )
{
    bool sideStyle = use_narrow_sidebar();
    const catacurses::window &weapwin = sideStyle ? w2 : w;

    {
        const int y = sideStyle ? 1 : 0;
        const int wn = getmaxx( weapwin );
        trim_and_print( weapwin, y, 0, wn, c_light_gray, print_gun_mode( *this ) );
    }

    // Print currently used style or weapon mode.
    std::string style;
    const auto style_color = is_armed() ? c_red : c_blue;
    const auto &cur_style = style_selected.obj();
    if( cur_style.force_unarmed || cur_style.weapon_valid( weapon ) ) {
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
        wprintz( w, c_light_red,  _( "Near starving" ) );
    } else if( get_hunger() > 300 ) {
        wprintz( w, c_light_red,  _( "Famished" ) );
    } else if( get_hunger() > 100 ) {
        wprintz( w, c_yellow, _( "Very hungry" ) );
    } else if( get_hunger() > 40 ) {
        wprintz( w, c_yellow, _( "Hungry" ) );
    } else if( get_hunger() < -60 ) {
        wprintz( w, c_green,  _( "Engorged" ) );
    } else if( get_hunger() < -20 ) {
        wprintz( w, c_green,  _( "Sated" ) );
    } else if( get_hunger() < 0 ) {
        wprintz( w, c_green,  _( "Full" ) );
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
    } else {
        temp_message = _( " (Falling!!)" );
    }

    // printCur the hottest/coldest bodypart, and if it is rising or falling in temperature
    wmove( w, sideStyle ? 6 : 1, sideStyle ? 0 : 9 );
    if( temp_cur[current_bp_extreme] >  BODYTEMP_SCORCHING ) {
        wprintz( w, c_red,   _( "Scorching!%s" ), temp_message );
    } else if( temp_cur[current_bp_extreme] >  BODYTEMP_VERY_HOT ) {
        wprintz( w, c_light_red, _( "Very hot!%s" ), temp_message );
    } else if( temp_cur[current_bp_extreme] >  BODYTEMP_HOT ) {
        wprintz( w, c_yellow, _( "Warm%s" ), temp_message );
    } else if( temp_cur[current_bp_extreme] >
               BODYTEMP_COLD ) { // If you're warmer than cold, you are comfortable
        wprintz( w, c_green, _( "Comfortable%s" ), temp_message );
    } else if( temp_cur[current_bp_extreme] >  BODYTEMP_VERY_COLD ) {
        wprintz( w, c_light_blue, _( "Chilly%s" ), temp_message );
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
        wprintz( w, c_light_red,  _( "Parched" ) );
    } else if( get_thirst() > 240 ) {
        wprintz( w, c_light_red,  _( "Dehydrated" ) );
    } else if( get_thirst() > 80 ) {
        wprintz( w, c_yellow, _( "Very thirsty" ) );
    } else if( get_thirst() > 40 ) {
        wprintz( w, c_yellow, _( "Thirsty" ) );
    } else if( get_thirst() < -60 ) {
        wprintz( w, c_green,  _( "Turgid" ) );
    } else if( get_thirst() < -20 ) {
        wprintz( w, c_green,  _( "Hydrated" ) );
    } else if( get_thirst() < 0 ) {
        wprintz( w, c_green,  _( "Slaked" ) );
    }

    wmove( w, sideStyle ? 3 : 2, sideStyle ? 0 : 30 );
    if( get_fatigue() > EXHAUSTED ) {
        wprintz( w, c_red,    _( "Exhausted" ) );
    } else if( get_fatigue() > DEAD_TIRED ) {
        wprintz( w, c_light_red,  _( "Dead tired" ) );
    } else if( get_fatigue() > TIRED ) {
        wprintz( w, c_yellow, _( "Tired" ) );
    }

    wmove( w, sideStyle ? 4 : 2, sideStyle ? 0 : 41 );
    wprintz( w, c_white, _( "Focus" ) );
    nc_color col_xp = c_dark_gray;
    if( focus_pool >= 100 ) {
        col_xp = c_white;
    } else if( focus_pool >  0 ) {
        col_xp = c_light_gray;
    }
    wprintz( w, col_xp, " %d", focus_pool );

    nc_color col_pain = c_yellow;
    if( get_perceived_pain() >= 60 ) {
        col_pain = c_red;
    } else if( get_perceived_pain() >= 40 ) {
        col_pain = c_light_red;
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
    if( get_option<std::string>( "MORALE_STYLE" ) == "horizontal" ) {
        if( has_trait( trait_THRESH_FELINE ) || has_trait( trait_THRESH_URSINE ) ) {
            if( morale_cur >= 200 ) {
                morale_str = "@W@";
            } else if( morale_cur >= 100 ) {
                morale_str = "OWO";
            } else if( morale_cur >= 50 ) {
                morale_str = "owo";
            } else if( morale_cur >= 10 ) {
                morale_str = "^w^";
            } else if( morale_cur >= -10 ) {
                morale_str = "-w-";
            } else if( morale_cur >= -50 ) {
                morale_str = "-m-";
            } else if( morale_cur >= -100 ) {
                morale_str = "TmT";
            } else if( morale_cur >= -200 ) {
                morale_str = "XmX";
            } else {
                morale_str = "@m@";
            }
        } else if( has_trait( trait_THRESH_BIRD ) ) {
            if( morale_cur >= 200 ) {
                morale_str = "@v@";
            } else if( morale_cur >= 100 ) {
                morale_str = "OvO";
            } else if( morale_cur >= 50 ) {
                morale_str = "ovo";
            } else if( morale_cur >= 10 ) {
                morale_str = "^v^";
            } else if( morale_cur >= -10 ) {
                morale_str = "-v-";
            } else if( morale_cur >= -50 ) {
                morale_str = ".v.";
            } else if( morale_cur >= -100 ) {
                morale_str = "TvT";
            } else if( morale_cur >= -200 ) {
                morale_str = "XvX";
            } else {
                morale_str = "@v@";
            }
        } else if( morale_cur >= 200 ) {
            morale_str = "@U@";
        } else if( morale_cur >= 100 ) {
            morale_str = "OuO";
        } else if( morale_cur >= 50 ) {
            morale_str = "^u^";
        } else if( morale_cur >= 10 ) {
            morale_str = "n_n";
        } else if( morale_cur >= -10 ) {
            morale_str = "-_-";
        } else if( morale_cur >= -50 ) {
            morale_str = "-n-";
        } else if( morale_cur >= -100 ) {
            morale_str = "TnT";
        } else if( morale_cur >= -200 ) {
            morale_str = "XnX";
        } else {
            morale_str = "@n@";
        }
    } else if( morale_cur >= 100 ) {
        morale_str = "8D";
    } else if( morale_cur >= 50 ) {
        morale_str = ":D";
    } else if( has_trait( trait_THRESH_FELINE ) && morale_cur >= 10 ) {
        morale_str = ":3";
    } else if( !has_trait( trait_THRESH_FELINE ) && morale_cur >= 10 ) {
        morale_str = ":)";
    } else if( morale_cur >= -10 ) {
        morale_str = ":|";
    } else if( morale_cur >= -50 ) {
        morale_str = "):";
    } else if( morale_cur >= -100 ) {
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
        nc_color col_indf1 = c_light_gray;

        float strain = veh->strain();
        nc_color col_vel = strain <= 0 ? c_light_blue :
                           ( strain <= 0.2 ? c_yellow :
                             ( strain <= 0.4 ? c_light_red : c_red ) );

        // Draw the speedometer.
        int speedox = sideStyle ? 0 : 28;
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
            mvwprintz( w, speedoy, speedox + cruisex, c_light_green, "%4d",
                       int( convert_velocity( veh->cruise_velocity, VU_VEHICLE ) ) );
        }

        const int vel_offset = 11 + ( veh->velocity != 0 ? 2 : 0 );
        wmove( w, sideStyle ? 4 : 3, getmaxx( w ) - vel_offset );
        if( veh->velocity != 0 ) {
            nc_color col_indc = veh->skidding ? c_red : c_green;
            int dfm = veh->face.dir() - veh->move.dir();

            if( dfm == 0 ) {
                wprintz( w, col_indc, "^" );
            } else if( dfm < 0 ) {
                wprintz( w, col_indc, "<" );
            } else {
                wprintz( w, col_indc, ">" );
            }

            wprintz( w, c_white, " " );
        }

        //Vehicle direction indicator in 0-359° where 0 is north (veh->face.dir() 0° is west)
        wprintz( w, c_white, string_format( "%3d°", ( veh->face.dir() + 90 ) % 360 ).c_str() );

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
                col_time = c_dark_gray_magenta;
            } else {
                col_time = c_dark_gray_red;
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


