#include "mtype.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <unordered_map>

#include "behavior_strategy.h"
#include "creature.h"
#include "field_type.h"
#include "item.h"
#include "itype.h"
#include "mod_manager.h"
#include "mondeath.h"
#include "monstergenerator.h"
#include "output.h"
#include "translations.h"
#include "units.h"
#include "weakpoint.h"

static const harvest_id harvest_list_human( "human" );

static const itype_id itype_bone( "bone" );
static const itype_id itype_bone_tainted( "bone_tainted" );
static const itype_id itype_fish( "fish" );
static const itype_id itype_human_flesh( "human_flesh" );
static const itype_id itype_meat( "meat" );
static const itype_id itype_meat_tainted( "meat_tainted" );
static const itype_id itype_veggy( "veggy" );
static const itype_id itype_veggy_tainted( "veggy_tainted" );

static const material_id material_bone( "bone" );
static const material_id material_flesh( "flesh" );
static const material_id material_hflesh( "hflesh" );
static const material_id material_iflesh( "iflesh" );
static const material_id material_veggy( "veggy" );

static const species_id species_MOLLUSK( "MOLLUSK" );

// NOLINTNEXTLINE(cata-static-int_id-constants)
mon_flag_id mon_flag_ACIDPROOF,
            mon_flag_ACIDTRAIL,
            mon_flag_ACID_BLOOD,
            mon_flag_ALL_SEEING,
            mon_flag_ALWAYS_SEES_YOU,
            mon_flag_ALWAYS_VISIBLE,
            mon_flag_ANIMAL,
            mon_flag_AQUATIC,
            mon_flag_ARTHROPOD_BLOOD,
            mon_flag_ATTACKMON,
            mon_flag_ATTACK_LOWER,
            mon_flag_ATTACK_UPPER,
            mon_flag_BADVENOM,
            mon_flag_BASHES,
            mon_flag_BILE_BLOOD,
            mon_flag_BORES,
            mon_flag_CAMOUFLAGE,
            mon_flag_CANPLAY,
            mon_flag_CAN_BE_CULLED,
            mon_flag_CAN_DIG,
            mon_flag_CAN_OPEN_DOORS,
            mon_flag_CLIMBS,
            mon_flag_COMBAT_MOUNT,
            mon_flag_CONSOLE_DESPAWN,
            mon_flag_CONVERSATION,
            mon_flag_CORNERED_FIGHTER,
            mon_flag_DEADLY_VIRUS,
            mon_flag_DESTROYS,
            mon_flag_DIGS,
            mon_flag_DOGFOOD,
            mon_flag_DORMANT,
            mon_flag_DRACULIN_IMMUNE,
            mon_flag_GEN_DORMANT,
            mon_flag_DRIPS_GASOLINE,
            mon_flag_DRIPS_NAPALM,
            mon_flag_DROPS_AMMO,
            mon_flag_EATS,
            mon_flag_ELECTRIC,
            mon_flag_ELECTRIC_FIELD,
            mon_flag_ELECTRONIC,
            mon_flag_FAE_CREATURE,
            mon_flag_FILTHY,
            mon_flag_FIREPROOF,
            mon_flag_FIREY,
            mon_flag_FISHABLE,
            mon_flag_FLIES,
            mon_flag_GOODHEARING,
            mon_flag_GRABS,
            mon_flag_GROUP_BASH,
            mon_flag_GROUP_MORALE,
            mon_flag_GUILT_ANIMAL,
            mon_flag_GUILT_CHILD,
            mon_flag_GUILT_HUMAN,
            mon_flag_GUILT_OTHERS,
            mon_flag_HARDTOSHOOT,
            mon_flag_HAS_MIND,
            mon_flag_HEARS,
            mon_flag_HIT_AND_RUN,
            mon_flag_HUMAN,
            mon_flag_ID_CARD_DESPAWN,
            mon_flag_IMMOBILE,
            mon_flag_INSECTICIDEPROOF,
            mon_flag_INTERIOR_AMMO,
            mon_flag_KEENNOSE,
            mon_flag_KEEP_DISTANCE,
            mon_flag_LOUDMOVES,
            mon_flag_MECH_DEFENSIVE,
            mon_flag_MECH_RECON_VISION,
            mon_flag_MILKABLE,
            mon_flag_NEMESIS,
            mon_flag_NEVER_WANDER,
            mon_flag_NIGHT_INVISIBILITY,
            mon_flag_NOGIB,
            mon_flag_NOHEAD,
            mon_flag_NOT_HALLUCINATION,
            mon_flag_NO_BREATHE,
            mon_flag_NO_BREED,
            mon_flag_NO_FUNG_DMG,
            mon_flag_NO_NECRO,
            mon_flag_PACIFIST,
            mon_flag_PARALYZEVENOM,
            mon_flag_PATH_AVOID_DANGER_1,
            mon_flag_PATH_AVOID_DANGER_2,
            mon_flag_PATH_AVOID_FALL,
            mon_flag_PATH_AVOID_FIRE,
            mon_flag_PAY_BOT,
            mon_flag_PET_HARNESSABLE,
            mon_flag_PET_MOUNTABLE,
            mon_flag_PET_WONT_FOLLOW,
            mon_flag_PHOTOPHOBIC,
            mon_flag_PLASTIC,
            mon_flag_POISON,
            mon_flag_PRIORITIZE_TARGETS,
            mon_flag_PUSH_MON,
            mon_flag_PUSH_VEH,
            mon_flag_QUEEN,
            mon_flag_QUIETDEATH,
            mon_flag_QUIETMOVES,
            mon_flag_RANGED_ATTACKER,
            mon_flag_REVIVES,
            mon_flag_REVIVES_HEALTHY,
            mon_flag_RIDEABLE_MECH,
            mon_flag_SEES,
            mon_flag_SHORTACIDTRAIL,
            mon_flag_SILENT_DISAPPEAR,
            mon_flag_SILENTMOVES,
            mon_flag_SLUDGEPROOF,
            mon_flag_SLUDGETRAIL,
            mon_flag_SMALLSLUDGETRAIL,
            mon_flag_SMALL_HIDER,
            mon_flag_SMELLS,
            mon_flag_STUMBLES,
            mon_flag_STUN_IMMUNE,
            mon_flag_SUNDEATH,
            mon_flag_SWARMS,
            mon_flag_SWIMS,
            mon_flag_VAMP_VIRUS,
            mon_flag_VENOM,
            mon_flag_VERMIN,
            mon_flag_WARM,
            mon_flag_WATER_CAMOUFLAGE,
            mon_flag_WEBWALK,
            mon_flag_WIELDED_WEAPON;

void set_mon_flag_ids()
{
    mon_flag_ACIDPROOF = mon_flag_id( "ACIDPROOF" );
    mon_flag_ACIDTRAIL = mon_flag_id( "ACIDTRAIL" );
    mon_flag_ACID_BLOOD = mon_flag_id( "ACID_BLOOD" );
    mon_flag_ALL_SEEING = mon_flag_id( "ALL_SEEING" );
    mon_flag_ALWAYS_SEES_YOU = mon_flag_id( "ALWAYS_SEES_YOU" );
    mon_flag_ALWAYS_VISIBLE = mon_flag_id( "ALWAYS_VISIBLE" );
    mon_flag_ANIMAL = mon_flag_id( "ANIMAL" );
    mon_flag_AQUATIC = mon_flag_id( "AQUATIC" );
    mon_flag_ARTHROPOD_BLOOD = mon_flag_id( "ARTHROPOD_BLOOD" );
    mon_flag_ATTACKMON = mon_flag_id( "ATTACKMON" );
    mon_flag_ATTACK_LOWER = mon_flag_id( "ATTACK_LOWER" );
    mon_flag_ATTACK_UPPER = mon_flag_id( "ATTACK_UPPER" );
    mon_flag_BADVENOM = mon_flag_id( "BADVENOM" );
    mon_flag_BASHES = mon_flag_id( "BASHES" );
    mon_flag_BILE_BLOOD = mon_flag_id( "BILE_BLOOD" );
    mon_flag_BORES = mon_flag_id( "BORES" );
    mon_flag_CAMOUFLAGE = mon_flag_id( "CAMOUFLAGE" );
    mon_flag_CANPLAY = mon_flag_id( "CANPLAY" );
    mon_flag_CAN_BE_CULLED = mon_flag_id( "CAN_BE_CULLED" );
    mon_flag_CAN_DIG = mon_flag_id( "CAN_DIG" );
    mon_flag_CAN_OPEN_DOORS = mon_flag_id( "CAN_OPEN_DOORS" );
    mon_flag_CLIMBS = mon_flag_id( "CLIMBS" );
    mon_flag_COMBAT_MOUNT = mon_flag_id( "COMBAT_MOUNT" );
    mon_flag_CONSOLE_DESPAWN = mon_flag_id( "CONSOLE_DESPAWN" );
    mon_flag_CONVERSATION = mon_flag_id( "CONVERSATION" );
    mon_flag_CORNERED_FIGHTER = mon_flag_id( "CORNERED_FIGHTER" );
    mon_flag_DEADLY_VIRUS = mon_flag_id( "DEADLY_VIRUS" );
    mon_flag_DESTROYS = mon_flag_id( "DESTROYS" );
    mon_flag_DIGS = mon_flag_id( "DIGS" );
    mon_flag_DOGFOOD = mon_flag_id( "DOGFOOD" );
    mon_flag_DORMANT = mon_flag_id( "DORMANT" );
    mon_flag_DRACULIN_IMMUNE = mon_flag_id( "DRACULIN_IMMUNE" );
    mon_flag_GEN_DORMANT = mon_flag_id( "GEN_DORMANT" );
    mon_flag_DRIPS_GASOLINE = mon_flag_id( "DRIPS_GASOLINE" );
    mon_flag_DRIPS_NAPALM = mon_flag_id( "DRIPS_NAPALM" );
    mon_flag_DROPS_AMMO = mon_flag_id( "DROPS_AMMO" );
    mon_flag_EATS = mon_flag_id( "EATS" );
    mon_flag_ELECTRIC = mon_flag_id( "ELECTRIC" );
    mon_flag_ELECTRIC_FIELD = mon_flag_id( "ELECTRIC_FIELD" );
    mon_flag_ELECTRONIC = mon_flag_id( "ELECTRONIC" );
    mon_flag_FAE_CREATURE = mon_flag_id( "FAE_CREATURE" );
    mon_flag_FILTHY = mon_flag_id( "FILTHY" );
    mon_flag_FIREPROOF = mon_flag_id( "FIREPROOF" );
    mon_flag_FIREY = mon_flag_id( "FIREY" );
    mon_flag_FISHABLE = mon_flag_id( "FISHABLE" );
    mon_flag_FLIES = mon_flag_id( "FLIES" );
    mon_flag_GOODHEARING = mon_flag_id( "GOODHEARING" );
    mon_flag_GRABS = mon_flag_id( "GRABS" );
    mon_flag_GROUP_BASH = mon_flag_id( "GROUP_BASH" );
    mon_flag_GROUP_MORALE = mon_flag_id( "GROUP_MORALE" );
    mon_flag_GUILT_ANIMAL = mon_flag_id( "GUILT_ANIMAL" );
    mon_flag_GUILT_CHILD = mon_flag_id( "GUILT_CHILD" );
    mon_flag_GUILT_HUMAN = mon_flag_id( "GUILT_HUMAN" );
    mon_flag_GUILT_OTHERS = mon_flag_id( "GUILT_OTHERS" );
    mon_flag_HARDTOSHOOT = mon_flag_id( "HARDTOSHOOT" );
    mon_flag_HAS_MIND = mon_flag_id( "HAS_MIND" );
    mon_flag_HEARS = mon_flag_id( "HEARS" );
    mon_flag_HIT_AND_RUN = mon_flag_id( "HIT_AND_RUN" );
    mon_flag_HUMAN = mon_flag_id( "HUMAN" );
    mon_flag_ID_CARD_DESPAWN = mon_flag_id( "ID_CARD_DESPAWN" );
    mon_flag_IMMOBILE = mon_flag_id( "IMMOBILE" );
    mon_flag_INSECTICIDEPROOF = mon_flag_id( "INSECTICIDEPROOF" );
    mon_flag_INTERIOR_AMMO = mon_flag_id( "INTERIOR_AMMO" );
    mon_flag_KEENNOSE = mon_flag_id( "KEENNOSE" );
    mon_flag_KEEP_DISTANCE = mon_flag_id( "KEEP_DISTANCE" );
    mon_flag_LOUDMOVES = mon_flag_id( "LOUDMOVES" );
    mon_flag_MECH_DEFENSIVE = mon_flag_id( "MECH_DEFENSIVE" );
    mon_flag_MECH_RECON_VISION = mon_flag_id( "MECH_RECON_VISION" );
    mon_flag_MILKABLE = mon_flag_id( "MILKABLE" );
    mon_flag_NEMESIS = mon_flag_id( "NEMESIS" );
    mon_flag_NEVER_WANDER = mon_flag_id( "NEVER_WANDER" );
    mon_flag_NIGHT_INVISIBILITY = mon_flag_id( "NIGHT_INVISIBILITY" );
    mon_flag_NOGIB = mon_flag_id( "NOGIB" );
    mon_flag_NOHEAD = mon_flag_id( "NOHEAD" );
    mon_flag_NOT_HALLUCINATION = mon_flag_id( "NOT_HALLUCINATION" );
    mon_flag_NO_BREATHE = mon_flag_id( "NO_BREATHE" );
    mon_flag_NO_BREED = mon_flag_id( "NO_BREED" );
    mon_flag_NO_FUNG_DMG = mon_flag_id( "NO_FUNG_DMG" );
    mon_flag_NO_NECRO = mon_flag_id( "NO_NECRO" );
    mon_flag_PACIFIST = mon_flag_id( "PACIFIST" );
    mon_flag_PARALYZEVENOM = mon_flag_id( "PARALYZEVENOM" );
    mon_flag_PATH_AVOID_DANGER_1 = mon_flag_id( "PATH_AVOID_DANGER_1" );
    mon_flag_PATH_AVOID_DANGER_2 = mon_flag_id( "PATH_AVOID_DANGER_2" );
    mon_flag_PATH_AVOID_FALL = mon_flag_id( "PATH_AVOID_FALL" );
    mon_flag_PATH_AVOID_FIRE = mon_flag_id( "PATH_AVOID_FIRE" );
    mon_flag_PAY_BOT = mon_flag_id( "PAY_BOT" );
    mon_flag_PET_HARNESSABLE = mon_flag_id( "PET_HARNESSABLE" );
    mon_flag_PET_MOUNTABLE = mon_flag_id( "PET_MOUNTABLE" );
    mon_flag_PET_WONT_FOLLOW = mon_flag_id( "PET_WONT_FOLLOW" );
    mon_flag_PHOTOPHOBIC = mon_flag_id( "PHOTOPHOBIC" );
    mon_flag_PLASTIC = mon_flag_id( "PLASTIC" );
    mon_flag_POISON = mon_flag_id( "POISON" );
    mon_flag_PRIORITIZE_TARGETS = mon_flag_id( "PRIORITIZE_TARGETS" );
    mon_flag_PUSH_MON = mon_flag_id( "PUSH_MON" );
    mon_flag_PUSH_VEH = mon_flag_id( "PUSH_VEH" );
    mon_flag_QUEEN = mon_flag_id( "QUEEN" );
    mon_flag_QUIETDEATH = mon_flag_id( "QUIETDEATH" );
    mon_flag_RANGED_ATTACKER = mon_flag_id( "RANGED_ATTACKER" );
    mon_flag_REVIVES = mon_flag_id( "REVIVES" );
    mon_flag_REVIVES_HEALTHY = mon_flag_id( "REVIVES_HEALTHY" );
    mon_flag_RIDEABLE_MECH = mon_flag_id( "RIDEABLE_MECH" );
    mon_flag_SEES = mon_flag_id( "SEES" );
    mon_flag_SHORTACIDTRAIL = mon_flag_id( "SHORTACIDTRAIL" );
    mon_flag_SILENT_DISAPPEAR = mon_flag_id( "SILENT_DISAPPEAR" );
    mon_flag_SLUDGEPROOF = mon_flag_id( "SLUDGEPROOF" );
    mon_flag_SLUDGETRAIL = mon_flag_id( "SLUDGETRAIL" );
    mon_flag_SMALLSLUDGETRAIL = mon_flag_id( "SMALLSLUDGETRAIL" );
    mon_flag_SMALL_HIDER = mon_flag_id( "SMALL_HIDER" );
    mon_flag_SMELLS = mon_flag_id( "SMELLS" );
    mon_flag_STUMBLES = mon_flag_id( "STUMBLES" );
    mon_flag_STUN_IMMUNE = mon_flag_id( "STUN_IMMUNE" );
    mon_flag_SUNDEATH = mon_flag_id( "SUNDEATH" );
    mon_flag_SWARMS = mon_flag_id( "SWARMS" );
    mon_flag_SWIMS = mon_flag_id( "SWIMS" );
    mon_flag_VAMP_VIRUS = mon_flag_id( "VAMP_VIRUS" );
    mon_flag_VENOM = mon_flag_id( "VENOM" );
    mon_flag_VERMIN = mon_flag_id( "VERMIN" );
    mon_flag_WARM = mon_flag_id( "WARM" );
    mon_flag_WATER_CAMOUFLAGE = mon_flag_id( "WATER_CAMOUFLAGE" );
    mon_flag_WEBWALK = mon_flag_id( "WEBWALK" );
    mon_flag_WIELDED_WEAPON = mon_flag_id( "WIELDED_WEAPON" );
}

mtype::mtype()
{
    id = mtype_id::NULL_ID();
    name = pl_translation( "human", "humans" );
    sym = " ";
    color = c_white;
    size = creature_size::medium;
    volume = 62499_ml;
    weight = 81499_gram;
    mat = { { material_flesh, 1 } };
    phase = phase_id::SOLID;
    def_chance = 0;
    upgrades = false;
    upgrade_multi_range = std::optional<int>();
    upgrade_null_despawn = false;
    half_life = -1;
    age_grow = -1;
    upgrade_into = mtype_id::NULL_ID();
    upgrade_group = mongroup_id::NULL_ID();

    reproduces = false;
    baby_count = -1;
    baby_monster = mtype_id::NULL_ID();
    baby_egg = itype_id::NULL_ID();

    biosignatures = false;
    biosig_item = itype_id::NULL_ID();

    burn_into = mtype_id::NULL_ID();
    sp_defense = nullptr;
    melee_training_cap = MAX_SKILL;
    harvest = harvest_list_human;
    luminance = 0;
    bash_skill = 0;

    aggro_character = true;

    pre_flags_.emplace( "HUMAN" );
}

std::string mtype::nname( unsigned int quantity ) const
{
    return name.translated( quantity );
}

bool mtype::has_special_attack( const std::string &attack_name ) const
{
    return special_attacks.find( attack_name ) != special_attacks.end();
}

void mtype::set_flag( const mon_flag_id &flag, bool state )
{
    if( state ) {
        flags.emplace( flag );
    } else {
        flags.erase( flag );
    }
}

bool mtype::made_of( const material_id &material ) const
{
    return mat.find( material ) != mat.end();
}

bool mtype::made_of_any( const std::set<material_id> &materials ) const
{
    if( mat.empty() ) {
        return false;
    }

    return std::any_of( mat.begin(), mat.end(), [&materials]( const std::pair<material_id, int> &e ) {
        return materials.count( e.first );
    } );
}

bool mtype::has_anger_trigger( mon_trigger trigger ) const
{
    return anger[trigger];
}

bool mtype::has_fear_trigger( mon_trigger trigger ) const
{
    return fear[trigger];
}

bool mtype::has_placate_trigger( mon_trigger trigger ) const
{
    return placate[trigger];
}

bool mtype::in_category( const std::string &category ) const
{
    return categories.find( category ) != categories.end();
}

bool mtype::in_species( const species_id &spec ) const
{
    return species.count( spec ) > 0;
}

std::vector<std::string> mtype::species_descriptions() const
{
    std::vector<std::string> ret;
    for( const species_id &s : species ) {
        if( !s->description.empty() ) {
            ret.emplace_back( s->description.translated() );
        }
    }
    return ret;
}

field_type_id mtype::get_bleed_type() const
{
    if( bleed_rate > 0 ) {
        for( const species_id &s : species ) {
            if( !s->bleeds.is_empty() ) {
                return s->bleeds;
            }
        }
    }
    return fd_null;
}

bool mtype::same_species( const mtype &other ) const
{
    return std::any_of( species.begin(), species.end(), [&]( const species_id & s ) {
        return other.in_species( s );
    } );
}

field_type_id mtype::bloodType() const
{
    if( has_flag( mon_flag_ACID_BLOOD ) )
        //A monster that has the death effect "ACID" does not need to have acid blood.
    {
        return fd_acid;
    }
    if( has_flag( mon_flag_BILE_BLOOD ) ) {
        return fd_bile;
    }
    if( has_flag( mon_flag_ARTHROPOD_BLOOD ) ) {
        return fd_blood_invertebrate;
    }
    if( made_of( material_veggy ) ) {
        return fd_blood_veggy;
    }
    if( made_of( material_iflesh ) ) {
        return fd_blood_insect;
    }
    if( has_flag( mon_flag_WARM ) && made_of( material_flesh ) ) {
        return fd_blood;
    }
    return get_bleed_type();
}

field_type_id mtype::gibType() const
{
    if( in_species( species_MOLLUSK ) ) {
        return fd_gibs_invertebrate;
    }
    if( made_of( material_veggy ) ) {
        return fd_gibs_veggy;
    }
    if( made_of( material_iflesh ) ) {
        return fd_gibs_insect;
    }
    if( made_of( material_flesh ) ) {
        return fd_gibs_flesh;
    }
    // There are other materials not listed here like steel, protoplasmic, powder, null, stone, bone
    return fd_null;
}

itype_id mtype::get_meat_itype() const
{
    if( has_flag( mon_flag_POISON ) ) {
        if( made_of( material_flesh ) || made_of( material_hflesh ) ||
            //In the future, insects could drop insect flesh rather than plain ol' meat.
            made_of( material_iflesh ) ) {
            return itype_meat_tainted;
        } else if( made_of( material_veggy ) ) {
            return itype_veggy_tainted;
        } else if( made_of( material_bone ) ) {
            return itype_bone_tainted;
        }
    } else {
        if( made_of( material_flesh ) || made_of( material_hflesh ) ) {
            if( has_flag( mon_flag_HUMAN ) ) {
                return itype_human_flesh;
            } else if( has_flag( mon_flag_AQUATIC ) ) {
                return itype_fish;
            } else {
                return itype_meat;
            }
        } else if( made_of( material_iflesh ) ) {
            //In the future, insects could drop insect flesh rather than plain ol' meat.
            return itype_meat;
        } else if( made_of( material_veggy ) ) {
            return itype_veggy;
        } else if( made_of( material_bone ) ) {
            return itype_bone;
        }
    }
    return itype_id::NULL_ID();
}

int mtype::get_meat_chunks_count() const
{
    const float ch = to_gram( weight ) * ( 0.40f - 0.02f * std::log10( to_gram( weight ) ) );
    const itype *chunk = item::find_type( get_meat_itype() );
    return static_cast<int>( ch / to_gram( chunk->weight ) );
}

std::string mtype::get_description() const
{
    return description.translated();
}

ascii_art_id mtype::get_picture_id() const
{
    return picture_id;
}

std::string mtype::get_footsteps() const
{
    if( !species.empty() ) {
        return species.begin()->obj().get_footsteps();
    }
    return _( "footsteps." );
}

void mtype::set_strategy()
{
    goals.set_strategy( behavior::strategy_map[ "sequential_until_done" ] );
}

void mtype::add_goal( const std::string &goal_id )
{
    goals.add_child( &string_id<behavior::node_t>( goal_id ).obj() );
}

const behavior::node_t *mtype::get_goals() const
{
    return &goals;
}

void mtype::faction_display( catacurses::window &w, const point &top_left, const int width ) const
{
    int y = 0;
    // Name & symbol
    trim_and_print( w, top_left + point( 2, y ), width, c_white, string_format( "%s  %s", colorize( sym,
                    color ), nname() ) );
    y++;
    // Difficulty
    std::string diff_str;
    if( difficulty < 3 ) {
        diff_str = _( "<color_light_gray>Minimal threat.</color>" );
    } else if( difficulty < 10 ) {
        diff_str = _( "<color_light_gray>Mildly dangerous.</color>" );
    } else if( difficulty < 20 ) {
        diff_str = _( "<color_light_red>Dangerous.</color>" );
    } else if( difficulty < 30 ) {
        diff_str = _( "<color_red>Very dangerous.</color>" );
    } else if( difficulty < 50 ) {
        diff_str = _( "<color_red>Extremely dangerous.</color>" );
    } else {
        diff_str = _( "<color_red>Fatally dangerous!</color>" );
    }
    trim_and_print( w, top_left + point( 0, ++y ), width, c_light_gray,
                    string_format( "%s: %s", colorize( _( "Difficulty" ), c_white ), diff_str ) );
    // Origin
    std::vector<std::string> origin_list =
        foldstring( string_format( "%s: %s", colorize( _( "Origin" ), c_white ),
                                   enumerate_as_string( src.begin(), src.end(),
    []( const std::pair<mtype_id, mod_id> &source ) {
        return string_format( "'%s'", source.second->name() );
    }, enumeration_conjunction::arrow ) ), width );
    for( const std::string &org : origin_list ) {
        trim_and_print( w, top_left + point( 0, ++y ), width, c_light_gray, org );
    }
    // Size
    const std::map<creature_size, std::string> size_map = {
        {creature_size::tiny, translate_marker( "Tiny" )},
        {creature_size::small, translate_marker( "Small" )},
        {creature_size::medium, translate_marker( "Medium" )},
        {creature_size::large, translate_marker( "Large" )},
        {creature_size::huge, translate_marker( "Huge" )}
    };
    auto size_iter = size_map.find( size );
    trim_and_print( w, top_left + point( 0, ++y ), width, c_light_gray,
                    string_format( "%s: %s", colorize( _( "Size" ), c_white ),
                                   size_iter == size_map.end() ? _( "Unknown" ) : _( size_iter->second ) ) );
    // Species
    std::vector<std::string> species_list =
        foldstring( string_format( "%s: %s", colorize( _( "Species" ), c_white ),
    enumerate_as_string( species_descriptions(), []( const std::string & sp ) {
        return colorize( sp, c_yellow );
    } ) ), width );
    for( const std::string &sp : species_list ) {
        trim_and_print( w, top_left + point( 0, ++y ), width, c_light_gray, sp );
    }
    // Senses
    std::vector<std::string> senses_str;
    if( has_flag( mon_flag_HEARS ) ) {
        senses_str.emplace_back( colorize( _( "hearing" ), c_yellow ) );
    }
    if( has_flag( mon_flag_SEES ) ) {
        senses_str.emplace_back( colorize( _( "sight" ), c_yellow ) );
    }
    if( has_flag( mon_flag_SMELLS ) ) {
        senses_str.emplace_back( colorize( _( "smell" ), c_yellow ) );
    }
    trim_and_print( w, top_left + point( 0, ++y ), width, c_light_gray,
                    string_format( "%s: %s", colorize( _( "Senses" ), c_white ), enumerate_as_string( senses_str ) ) );
    // Abilities
    if( has_flag( mon_flag_SWIMS ) ) {
        trim_and_print( w, top_left + point( 0, ++y ), width, c_white, _( "It can swim." ) );
    }
    if( has_flag( mon_flag_FLIES ) ) {
        trim_and_print( w, top_left + point( 0, ++y ), width, c_white, _( "It can fly." ) );
    }
    if( has_flag( mon_flag_DIGS ) ) {
        trim_and_print( w, top_left + point( 0, ++y ), width, c_white, _( "It can burrow underground." ) );
    }
    if( has_flag( mon_flag_CLIMBS ) ) {
        trim_and_print( w, top_left + point( 0, ++y ), width, c_white, _( "It can climb." ) );
    }
    if( has_flag( mon_flag_GRABS ) ) {
        trim_and_print( w, top_left + point( 0, ++y ), width, c_white, _( "It can grab." ) );
    }
    if( has_flag( mon_flag_VENOM ) ) {
        trim_and_print( w, top_left + point( 0, ++y ), width, c_white, _( "It can inflict poison." ) );
    }
    if( has_flag( mon_flag_PARALYZEVENOM ) ) {
        trim_and_print( w, top_left + point( 0, ++y ), width, c_white, _( "It can inflict paralysis." ) );
    }
    // Description
    fold_and_print( w, top_left + point( 0, y + 2 ), width, c_light_gray, get_description() );
}
