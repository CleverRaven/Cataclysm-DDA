#include "mattack_common.h" // IWYU pragma: associated
#include "monstergenerator.h" // IWYU pragma: associated

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <new>
#include <optional>
#include <set>
#include <string>
#include <utility>

#include "assign.h"
#include "bodypart.h"
#include "cached_options.h"
#include "calendar.h"
#include "catacharset.h"
#include "creature.h"
#include "damage.h"
#include "debug.h"
#include "enum_conversions.h"
#include "field_type.h"
#include "generic_factory.h"
#include "item.h"
#include "item_group.h"
#include "json.h"
#include "mattack_actors.h"
#include "monattack.h"
#include "mondeath.h"
#include "mondefense.h"
#include "mongroup.h"
#include "options.h"
#include "pathfinding.h"
#include "rng.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "weakpoint.h"

static const material_id material_flesh( "flesh" );

static const speed_description_id speed_description_DEFAULT( "DEFAULT" );

namespace behavior
{
class node_t;
}  // namespace behavior

namespace io
{

template<>
std::string enum_to_string<mon_trigger>( mon_trigger data )
{
    switch( data ) {
        // *INDENT-OFF*
        case mon_trigger::STALK: return "STALK";
        case mon_trigger::HOSTILE_WEAK: return "PLAYER_WEAK";
        case mon_trigger::HOSTILE_CLOSE: return "PLAYER_CLOSE";
        case mon_trigger::HOSTILE_SEEN: return "HOSTILE_SEEN";
        case mon_trigger::HURT: return "HURT";
        case mon_trigger::FIRE: return "FIRE";
        case mon_trigger::FRIEND_DIED: return "FRIEND_DIED";
        case mon_trigger::FRIEND_ATTACKED: return "FRIEND_ATTACKED";
        case mon_trigger::SOUND: return "SOUND";
        case mon_trigger::PLAYER_NEAR_BABY: return "PLAYER_NEAR_BABY";
        case mon_trigger::MATING_SEASON: return "MATING_SEASON";
        // *INDENT-ON*
        case mon_trigger::LAST:
            break;
    }
    cata_fatal( "Invalid mon_trigger" );
}

template<>
std::string enum_to_string<mdeath_type>( mdeath_type data )
{
    switch( data ) {
        case mdeath_type::NORMAL:
            return "NORMAL";
        case mdeath_type::SPLATTER:
            return "SPLATTER";
        case mdeath_type::BROKEN:
            return "BROKEN";
        case mdeath_type::NO_CORPSE:
            return "NO_CORPSE";
        case mdeath_type::LAST:
            break;
    }
    cata_fatal( "Invalid mdeath_type" );
}

template<>
std::string enum_to_string<m_flag>( m_flag data )
{
    // see mtype.h for commentary
    switch( data ) {
        // *INDENT-OFF*
        case MF_SEES: return "SEES";
        case MF_HEARS: return "HEARS";
        case MF_GOODHEARING: return "GOODHEARING";
        case MF_SMELLS: return "SMELLS";
        case MF_KEENNOSE: return "KEENNOSE";
        case MF_STUMBLES: return "STUMBLES";
        case MF_WARM: return "WARM";
        case MF_NEMESIS: return "NEMESIS";
        case MF_NOHEAD: return "NOHEAD";
        case MF_HARDTOSHOOT: return "HARDTOSHOOT";
        case MF_GRABS: return "GRABS";
        case MF_BASHES: return "BASHES";
        case MF_GROUP_BASH: return "GROUP_BASH";
        case MF_DESTROYS: return "DESTROYS";
        case MF_BORES: return "BORES";
        case MF_POISON: return "POISON";
        case MF_VENOM: return "VENOM";
        case MF_BADVENOM: return "BADVENOM";
        case MF_PARALYZE: return "PARALYZEVENOM";
        case MF_WEBWALK: return "WEBWALK";
        case MF_DIGS: return "DIGS";
        case MF_CAN_DIG: return "CAN_DIG";
        case MF_CAN_OPEN_DOORS: return "CAN_OPEN_DOORS";
        case MF_FLIES: return "FLIES";
        case MF_AQUATIC: return "AQUATIC";
        case MF_SWIMS: return "SWIMS";
        case MF_FISHABLE: return "FISHABLE";
        case MF_ATTACKMON: return "ATTACKMON";
        case MF_ANIMAL: return "ANIMAL";
        case MF_PLASTIC: return "PLASTIC";
        case MF_SUNDEATH: return "SUNDEATH";
        case MF_ELECTRIC: return "ELECTRIC";
        case MF_ACIDPROOF: return "ACIDPROOF";
        case MF_ACIDTRAIL: return "ACIDTRAIL";
        case MF_SHORTACIDTRAIL: return "SHORTACIDTRAIL";
        case MF_FIREPROOF: return "FIREPROOF";
        case MF_SLUDGEPROOF: return "SLUDGEPROOF";
        case MF_SLUDGETRAIL: return "SLUDGETRAIL";
        case MF_SMALLSLUDGETRAIL: return "SMALLSLUDGETRAIL";
        case MF_COLDPROOF: return "COLDPROOF";
        case MF_COMBAT_MOUNT: return "COMBAT_MOUNT";
        case MF_FIREY: return "FIREY";
        case MF_QUEEN: return "QUEEN";
        case MF_ELECTRONIC: return "ELECTRONIC";
        case MF_CONSOLE_DESPAWN: return "CONSOLE_DESPAWN";
        case MF_IMMOBILE: return "IMMOBILE";
        case MF_ID_CARD_DESPAWN: return "ID_CARD_DESPAWN";
        case MF_RIDEABLE_MECH: return "RIDEABLE_MECH";
        case MF_MILITARY_MECH: return "MILITARY_MECH";
        case MF_MECH_RECON_VISION: return "MECH_RECON_VISION";
        case MF_MECH_DEFENSIVE: return "MECH_DEFENSIVE";
        case MF_HIT_AND_RUN: return "HIT_AND_RUN";
        case MF_PAY_BOT: return "PAY_BOT";
        case MF_HUMAN: return "HUMAN";
        case MF_NO_BREATHE: return "NO_BREATHE";
        case MF_FLAMMABLE: return "FLAMMABLE";
        case MF_REVIVES: return "REVIVES";
        case MF_VERMIN: return "VERMIN";
        case MF_NOGIB: return "NOGIB";
        case MF_ARTHROPOD_BLOOD: return "ARTHROPOD_BLOOD";
        case MF_ACID_BLOOD: return "ACID_BLOOD";
        case MF_BILE_BLOOD: return "BILE_BLOOD";
        case MF_FILTHY: return "FILTHY";
        case MF_SWARMS: return "SWARMS";
        case MF_CLIMBS: return "CLIMBS";
        case MF_GROUP_MORALE: return "GROUP_MORALE";
        case MF_INTERIOR_AMMO: return "INTERIOR_AMMO";
        case MF_NIGHT_INVISIBILITY: return "NIGHT_INVISIBILITY";
        case MF_REVIVES_HEALTHY: return "REVIVES_HEALTHY";
        case MF_NO_NECRO: return "NO_NECRO";
        case MF_PACIFIST: return "PACIFIST";
        case MF_KEEP_DISTANCE: return "KEEP_DISTANCE";
        case MF_PUSH_MON: return "PUSH_MON";
        case MF_PUSH_VEH: return "PUSH_VEH";
        case MF_AVOID_DANGER_1: return "PATH_AVOID_DANGER_1";
        case MF_AVOID_DANGER_2: return "PATH_AVOID_DANGER_2";
        case MF_AVOID_FALL: return "PATH_AVOID_FALL";
        case MF_AVOID_FIRE: return "PATH_AVOID_FIRE";
        case MF_PRIORITIZE_TARGETS: return "PRIORITIZE_TARGETS";
        case MF_NOT_HALLU: return "NOT_HALLUCINATION";
        case MF_CANPLAY: return "CANPLAY";
        case MF_CAN_BE_CULLED: return "CAN_BE_CULLED";
        case MF_PET_MOUNTABLE: return "PET_MOUNTABLE";
        case MF_PET_HARNESSABLE: return "PET_HARNESSABLE";
        case MF_DOGFOOD: return "DOGFOOD";
        case MF_MILKABLE: return "MILKABLE";
        case MF_SHEARABLE: return "SHEARABLE";
        case MF_NO_BREED: return "NO_BREED";
        case MF_NO_FUNG_DMG: return "NO_FUNG_DMG";
        case MF_PET_WONT_FOLLOW: return "PET_WONT_FOLLOW";
        case MF_DRIPS_NAPALM: return "DRIPS_NAPALM";
        case MF_DRIPS_GASOLINE: return "DRIPS_GASOLINE";
        case MF_ELECTRIC_FIELD: return "ELECTRIC_FIELD";
        case MF_STUN_IMMUNE: return "STUN_IMMUNE";
        case MF_LOUDMOVES: return "LOUDMOVES";
        case MF_DROPS_AMMO: return "DROPS_AMMO";
        case MF_INSECTICIDEPROOF: return "INSECTICIDEPROOF";
        case MF_RANGED_ATTACKER: return "RANGED_ATTACKER";
        case MF_CAMOUFLAGE: return "CAMOUFLAGE";
        case MF_WATER_CAMOUFLAGE: return "WATER_CAMOUFLAGE";
        case MF_ATTACK_UPPER: return "ATTACK_UPPER";
        case MF_ATTACK_LOWER: return "ATTACK_LOWER";
        case MF_DEADLY_VIRUS: return "DEADLY_VIRUS";
        case MF_VAMP_VIRUS: return "VAMP_VIRUS";
        case MF_ALWAYS_VISIBLE: return "ALWAYS_VISIBLE";
        case MF_ALWAYS_SEES_YOU: return "ALWAYS_SEES_YOU";
        case MF_ALL_SEEING: return "ALL_SEEING";
        case MF_NEVER_WANDER: return "NEVER_WANDER";
        case MF_CONVERSATION: return "CONVERSATION";
        // *INDENT-ON*
        case m_flag::MF_MAX:
            break;
    }
    cata_fatal( "Invalid m_flag" );
}

} // namespace io

/** @relates string_id */
template<>
const mtype &string_id<mtype>::obj() const
{
    return MonsterGenerator::generator().mon_templates->obj( *this );
}

/** @relates string_id */
template<>
bool string_id<mtype>::is_valid() const
{
    return MonsterGenerator::generator().mon_templates->is_valid( *this );
}

/** @relates string_id */
template<>
const species_type &string_id<species_type>::obj() const
{
    return MonsterGenerator::generator().mon_species->obj( *this );
}

/** @relates string_id */
template<>
bool string_id<species_type>::is_valid() const
{
    return MonsterGenerator::generator().mon_species->is_valid( *this );
}

std::optional<mon_action_death> MonsterGenerator::get_death_function( const std::string &f ) const
{
    const auto it = death_map.find( f );

    return it != death_map.cend()
           ? std::optional<mon_action_death>( it->second )
           : std::optional<mon_action_death>();
}

MonsterGenerator::MonsterGenerator()
    : mon_templates( "monster type" )
    , mon_species( "species" )
{
    mon_templates->insert( mtype() );
    mon_species->insert( species_type() );
    init_phases();
    init_attack();
    init_defense();
}

MonsterGenerator::~MonsterGenerator() = default;

void MonsterGenerator::reset()
{
    mon_templates->reset();
    mon_templates->insert( mtype() );

    mon_species->reset();
    mon_species->insert( species_type() );

    hallucination_monsters.clear();

    attack_map.clear();
    // Hardcode attacks need to be re-added here
    // TODO: Move initialization from constructor to init()
    init_attack();
}

static int calc_bash_skill( const mtype &t )
{
    // IOW, the critter's max bashing damage
    int ret = t.melee_dice * t.melee_sides;
    // This is for stuff that goes through solid rock: minerbots, dark wyrms, etc
    if( t.has_flag( MF_BORES ) ) {
        ret *= 15;
    } else if( t.has_flag( MF_DESTROYS ) ) {
        ret *= 2.5;
    } else if( !t.has_flag( MF_BASHES ) ) {
        ret = 0;
    }

    return ret;
}

static creature_size volume_to_size( const units::volume &vol )
{
    if( vol <= 7500_ml ) {
        return creature_size::tiny;
    } else if( vol <= 46250_ml ) {
        return creature_size::small;
    } else if( vol <= 77500_ml ) {
        return creature_size::medium;
    } else if( vol <= 483750_ml ) {
        return creature_size::large;
    }
    return creature_size::huge;
}

struct monster_adjustment {
    species_id species;
    std::string stat;
    float stat_adjust = 0.0f;
    std::string flag;
    bool flag_val = false;
    std::string special;
    void apply( mtype &mon ) const;
};

void monster_adjustment::apply( mtype &mon ) const
{
    if( !mon.in_species( species ) ) {
        return;
    }
    if( !stat.empty() ) {
        if( stat == "speed" ) {
            mon.speed *= stat_adjust;
        } else if( stat == "hp" ) {
            mon.hp *= stat_adjust;
        } else if( stat == "bleed_rate" ) {
            mon.bleed_rate *= stat_adjust;
        }
    }
    if( !flag.empty() ) {
        mon.set_flag( io::string_to_enum<m_flag>( flag ), flag_val );
    }
    if( !special.empty() ) {
        if( special == "nightvision" ) {
            mon.vision_night = mon.vision_day;
        }
    }
}

static std::vector<monster_adjustment> adjustments;

void reset_monster_adjustment()
{
    adjustments.clear();
}

void load_monster_adjustment( const JsonObject &jsobj )
{
    monster_adjustment adj;
    adj.species = species_id( jsobj.get_string( "species" ) );
    if( jsobj.has_member( "stat" ) ) {
        JsonObject stat = jsobj.get_object( "stat" );
        stat.read( "name", adj.stat );
        stat.read( "modifier", adj.stat_adjust );
    }
    if( jsobj.has_member( "flag" ) ) {
        JsonObject flag = jsobj.get_object( "flag" );
        flag.read( "name", adj.flag );
        flag.read( "value", adj.flag_val );
    }
    if( jsobj.has_member( "special" ) ) {
        jsobj.read( "special", adj.special );
    }
    adjustments.push_back( adj );
}

static void build_behavior_tree( mtype &type )
{
    type.set_strategy();
    for( const std::pair<const std::string, mtype_special_attack> &attack : type.special_attacks ) {
        if( string_id<behavior::node_t>( attack.first ).is_valid() ) {
            type.add_goal( attack.first );
        } /* TODO: Make this an error once all the special attacks are migrated. */
    }
}

void MonsterGenerator::finalize_mtypes()
{
    mon_templates->finalize();
    for( const mtype &elem : mon_templates->get_all() ) {
        mtype &mon = const_cast<mtype &>( elem );

        if( !mon.default_faction.is_valid() ) {
            debugmsg( "Monster type '%s' has invalid default_faction: '%s'. "
                      "Add this faction to json as MONSTER_FACTION type.",
                      mon.id.str(), mon.default_faction.str() );
        }

        apply_species_attributes( mon );
        validate_species_ids( mon );
        mon.size = volume_to_size( mon.volume );

        // adjust for worldgen difficulty parameters
        mon.speed *= get_option<int>( "MONSTER_SPEED" )      / 100.0;
        mon.hp    *= get_option<int>( "MONSTER_RESILIENCE" ) / 100.0;

        for( const monster_adjustment &adj : adjustments ) {
            adj.apply( mon );
        }

        if( mon.bash_skill < 0 ) {
            mon.bash_skill = calc_bash_skill( mon );
        }

        if( mon.armor_bash < 0 ) {
            mon.armor_bash = 0;
        }
        if( mon.armor_cut < 0 ) {
            mon.armor_cut = 0;
        }
        if( mon.armor_stab < 0 ) {
            mon.armor_stab = mon.armor_cut * 0.8;
        }
        if( mon.armor_bullet < 0 ) {
            mon.armor_bullet = 0;
        }
        if( mon.armor_acid < 0 ) {
            mon.armor_acid = mon.armor_cut * 0.5;
        }
        if( mon.armor_fire < 0 ) {
            mon.armor_fire = 0;
        }
        if( mon.armor_elec < 0 ) {
            mon.armor_elec = 0;
        }
        if( mon.armor_cold < 0 ) {
            mon.armor_cold = 0;
        }
        if( mon.armor_pure < 0 ) {
            mon.armor_pure = 0;
        }
        if( mon.armor_biological < 0 ) {
            mon.armor_biological = 0;
        }
        if( mon.status_chance_multiplier < 0 ) {
            mon.status_chance_multiplier = 0;
        }

        // Check if trap_ids are valid
        for( trap_str_id trap_avoid_id : mon.trap_avoids ) {
            if( !trap_avoid_id.is_valid() ) {
                debugmsg( "Invalid trap '%s'", trap_avoid_id.str() );
            }
        }

        // Lower bound for hp scaling
        mon.hp = std::max( mon.hp, 1 );

        //If the result monster is blacklisted no need to have the original monster look like it can revive
        if( !mon.zombify_into.is_empty() &&
            MonsterGroupManager::monster_is_blacklisted( mon.zombify_into ) ) {
            mon.zombify_into = mtype_id();
        }

        build_behavior_tree( mon );
        finalize_pathfinding_settings( mon );

        mon.weakpoints.clear();
        for( const weakpoints_id &wpset : mon.weakpoints_deferred ) {
            mon.weakpoints.add_from_set( wpset, true );
        }
        if( !mon.weakpoints_deferred_inline.weakpoint_list.empty() ) {
            mon.weakpoints.add_from_set( mon.weakpoints_deferred_inline, true );
        }
        for( const std::string &wp_del : mon.weakpoints_deferred_deleted ) {
            for( auto iter = mon.weakpoints.weakpoint_list.begin();
                 iter != mon.weakpoints.weakpoint_list.end(); ) {
                if( iter->id == wp_del ) {
                    iter = mon.weakpoints.weakpoint_list.erase( iter );
                } else {
                    iter++;
                }
            }
        }
    }

    for( const mtype &mon : mon_templates->get_all() ) {
        if( !mon.has_flag( MF_NOT_HALLU ) ) {
            hallucination_monsters.push_back( mon.id );
        }
    }
}

void MonsterGenerator::apply_species_attributes( mtype &mon )
{
    for( const auto &spec : mon.species ) {
        if( !spec.is_valid() ) {
            continue;
        }
        const species_type &mspec = spec.obj();

        mon.flags |= mspec.flags;
        mon.anger |= mspec.anger;
        mon.fear |= mspec.fear;
        mon.placate |= mspec.placate;
    }
}

void MonsterGenerator::finalize_pathfinding_settings( mtype &mon )
{
    if( mon.path_settings.max_length < 0 ) {
        mon.path_settings.max_length = mon.path_settings.max_dist * 5;
    }

    if( mon.path_settings.bash_strength < 0 ) {
        mon.path_settings.bash_strength = mon.bash_skill;
    }

    if( mon.has_flag( MF_CLIMBS ) ) {
        mon.path_settings.climb_cost = 3;
    }
}

void MonsterGenerator::init_phases()
{
    phase_map["NULL"] = phase_id::PNULL;
    phase_map["SOLID"] = phase_id::SOLID;
    phase_map["LIQUID"] = phase_id::LIQUID;
    phase_map["GAS"] = phase_id::GAS;
    phase_map["PLASMA"] = phase_id::PLASMA;
}

void MonsterGenerator::init_attack()
{
    add_hardcoded_attack( "NONE", mattack::none );
    add_hardcoded_attack( "ABSORB_ITEMS", mattack::absorb_items );
    add_hardcoded_attack( "SPLIT", mattack::split );
    add_hardcoded_attack( "EAT_CROP", mattack::eat_crop );
    add_hardcoded_attack( "EAT_FOOD", mattack::eat_food );
    add_hardcoded_attack( "CHECK_UP", mattack::nurse_check_up );
    add_hardcoded_attack( "ASSIST", mattack::nurse_assist );
    add_hardcoded_attack( "OPERATE", mattack::nurse_operate );
    add_hardcoded_attack( "PAID_BOT", mattack::check_money_left );
    add_hardcoded_attack( "SHRIEK", mattack::shriek );
    add_hardcoded_attack( "SHRIEK_ALERT", mattack::shriek_alert );
    add_hardcoded_attack( "SHRIEK_STUN", mattack::shriek_stun );
    add_hardcoded_attack( "RATTLE", mattack::rattle );
    add_hardcoded_attack( "HOWL", mattack::howl );
    add_hardcoded_attack( "ACID", mattack::acid );
    add_hardcoded_attack( "ACID_BARF", mattack::acid_barf );
    add_hardcoded_attack( "ACID_ACCURATE", mattack::acid_accurate );
    add_hardcoded_attack( "SHOCKSTORM", mattack::shockstorm );
    add_hardcoded_attack( "SHOCKING_REVEAL", mattack::shocking_reveal );
    add_hardcoded_attack( "PULL_METAL_WEAPON", mattack::pull_metal_weapon );
    add_hardcoded_attack( "BOOMER", mattack::boomer );
    add_hardcoded_attack( "BOOMER_GLOW", mattack::boomer_glow );
    add_hardcoded_attack( "RESURRECT", mattack::resurrect );
    add_hardcoded_attack( "SMASH", mattack::smash );
    add_hardcoded_attack( "SCIENCE", mattack::science );
    add_hardcoded_attack( "GROWPLANTS", mattack::growplants );
    add_hardcoded_attack( "GROW_VINE", mattack::grow_vine );
    add_hardcoded_attack( "VINE", mattack::vine );
    add_hardcoded_attack( "SPIT_SAP", mattack::spit_sap );
    add_hardcoded_attack( "TRIFFID_HEARTBEAT", mattack::triffid_heartbeat );
    add_hardcoded_attack( "FUNGUS", mattack::fungus );
    add_hardcoded_attack( "FUNGUS_CORPORATE", mattack::fungus_corporate );
    add_hardcoded_attack( "FUNGUS_HAZE", mattack::fungus_haze );
    add_hardcoded_attack( "FUNGUS_BIG_BLOSSOM", mattack::fungus_big_blossom );
    add_hardcoded_attack( "FUNGUS_INJECT", mattack::fungus_inject );
    add_hardcoded_attack( "FUNGUS_BRISTLE", mattack::fungus_bristle );
    add_hardcoded_attack( "FUNGUS_GROWTH", mattack::fungus_growth );
    add_hardcoded_attack( "FUNGUS_SPROUT", mattack::fungus_sprout );
    add_hardcoded_attack( "FUNGUS_FORTIFY", mattack::fungus_fortify );
    add_hardcoded_attack( "DERMATIK", mattack::dermatik );
    add_hardcoded_attack( "DERMATIK_GROWTH", mattack::dermatik_growth );
    add_hardcoded_attack( "FUNGAL_TRAIL", mattack::fungal_trail );
    add_hardcoded_attack( "PLANT", mattack::plant );
    add_hardcoded_attack( "DISAPPEAR", mattack::disappear );
    add_hardcoded_attack( "DEPART", mattack::depart );
    add_hardcoded_attack( "FORMBLOB", mattack::formblob );
    add_hardcoded_attack( "CALLBLOBS", mattack::callblobs );
    add_hardcoded_attack( "JACKSON", mattack::jackson );
    add_hardcoded_attack( "DANCE", mattack::dance );
    add_hardcoded_attack( "DOGTHING", mattack::dogthing );
    add_hardcoded_attack( "TENTACLE", mattack::tentacle );
    add_hardcoded_attack( "GENE_STING", mattack::gene_sting );
    add_hardcoded_attack( "PARA_STING", mattack::para_sting );
    add_hardcoded_attack( "TRIFFID_GROWTH", mattack::triffid_growth );
    add_hardcoded_attack( "STARE", mattack::stare );
    add_hardcoded_attack( "FEAR_PARALYZE", mattack::fear_paralyze );
    add_hardcoded_attack( "PHOTOGRAPH", mattack::photograph );
    add_hardcoded_attack( "TAZER", mattack::tazer );
    add_hardcoded_attack( "SEARCHLIGHT", mattack::searchlight );
    add_hardcoded_attack( "SPEAKER", mattack::speaker );
    add_hardcoded_attack( "FLAMETHROWER", mattack::flamethrower );
    add_hardcoded_attack( "COPBOT", mattack::copbot );
    add_hardcoded_attack( "CHICKENBOT", mattack::chickenbot );
    add_hardcoded_attack( "MULTI_ROBOT", mattack::multi_robot );
    add_hardcoded_attack( "RATKING", mattack::ratking );
    add_hardcoded_attack( "GENERATOR", mattack::generator );
    add_hardcoded_attack( "UPGRADE", mattack::upgrade );
    add_hardcoded_attack( "BREATHE", mattack::breathe );
    add_hardcoded_attack( "IMPALE", mattack::impale );
    add_hardcoded_attack( "BRANDISH", mattack::brandish );
    add_hardcoded_attack( "FLESH_GOLEM", mattack::flesh_golem );
    add_hardcoded_attack( "ABSORB_MEAT", mattack::absorb_meat );
    add_hardcoded_attack( "LUNGE", mattack::lunge );
    add_hardcoded_attack( "LONGSWIPE", mattack::longswipe );
    add_hardcoded_attack( "PARROT", mattack::parrot );
    add_hardcoded_attack( "PARROT_AT_DANGER", mattack::parrot_at_danger );
    add_hardcoded_attack( "BLOW_WHISTLE", mattack::blow_whistle );
    add_hardcoded_attack( "DARKMAN", mattack::darkman );
    add_hardcoded_attack( "SLIMESPRING", mattack::slimespring );
    add_hardcoded_attack( "EVOLVE_KILL_STRIKE", mattack::evolve_kill_strike );
    add_hardcoded_attack( "LEECH_SPAWNER", mattack::leech_spawner );
    add_hardcoded_attack( "MON_LEECH_EVOLUTION", mattack::mon_leech_evolution );
    add_hardcoded_attack( "TINDALOS_TELEPORT", mattack::tindalos_teleport );
    add_hardcoded_attack( "FLESH_TENDRIL", mattack::flesh_tendril );
    add_hardcoded_attack( "BIO_OP_BIOJUTSU", mattack::bio_op_random_biojutsu );
    add_hardcoded_attack( "BIO_OP_TAKEDOWN", mattack::bio_op_takedown );
    add_hardcoded_attack( "BIO_OP_IMPALE", mattack::bio_op_impale );
    add_hardcoded_attack( "BIO_OP_DISARM", mattack::bio_op_disarm );
    add_hardcoded_attack( "SUICIDE", mattack::suicide );
    add_hardcoded_attack( "KAMIKAZE", mattack::kamikaze );
    add_hardcoded_attack( "GRENADIER", mattack::grenadier );
    add_hardcoded_attack( "GRENADIER_ELITE", mattack::grenadier_elite );
    add_hardcoded_attack( "RIOTBOT", mattack::riotbot );
    add_hardcoded_attack( "STRETCH_ATTACK", mattack::stretch_attack );
    add_hardcoded_attack( "STRETCH_BITE", mattack::stretch_bite );
    add_hardcoded_attack( "RANGED_PULL", mattack::ranged_pull );
    add_hardcoded_attack( "GRAB", mattack::grab );
    add_hardcoded_attack( "GRAB_DRAG", mattack::grab_drag );
    add_hardcoded_attack( "DOOT", mattack::doot );
    add_hardcoded_attack( "DSA_DRONE_SCAN", mattack::dsa_drone_scan );
    add_hardcoded_attack( "ZOMBIE_FUSE", mattack::zombie_fuse );
}

void MonsterGenerator::init_defense()
{
    // No special attack-back
    defense_map["NONE"] = &mdefense::none;
    // Shock attacker on hit
    defense_map["ZAPBACK"] = &mdefense::zapback;
    // Splash acid on the attacker
    defense_map["ACIDSPLASH"] = &mdefense::acidsplash;
    // Blind fire on unseen attacker
    defense_map["RETURN_FIRE"] = &mdefense::return_fire;
}

void MonsterGenerator::validate_species_ids( mtype &mon )
{
    for( const auto &s : mon.species ) {
        if( !s.is_valid() ) {
            debugmsg( "Tried to assign species %s to monster %s, but no entry for the species exists",
                      s.c_str(), mon.id.c_str() );
        }
    }
}

void MonsterGenerator::load_monster( const JsonObject &jo, const std::string &src )
{
    mon_templates->load( jo, src );
}

mon_effect_data::mon_effect_data() :
    chance( 100.0f ),
    permanent( false ),
    affect_hit_bp( false ),
    bp( body_part_bp_null ),
    duration( 1, 1 ),
    intensity( 0, 0 ) {}

void mon_effect_data::load( const JsonObject &jo )
{
    mandatory( jo, false, "id", id );
    optional( jo, false, "chance", chance, 100.f );
    optional( jo, false, "permanent", permanent, false );
    optional( jo, false, "affect_hit_bp", affect_hit_bp, false );
    optional( jo, false, "bp", bp, body_part_bp_null );
    optional( jo, false, "message", message );
    // Support shorthand for a single value.
    if( jo.has_int( "duration" ) ) {
        int i = 1;
        mandatory( jo, false, "duration", i );
        duration = { i, i };
    } else {
        optional( jo, false, "duration", duration, std::pair<int, int> { 1, 1 } );
    }
    if( jo.has_int( "intensity" ) ) {
        int i = 1;
        mandatory( jo, false, "intensity", i );
        intensity = { i, i };
    } else {
        optional( jo, false, "intensity", intensity, std::pair<int, int> { 1, 1 } );
    }

    if( chance > 100.f || chance < 0.f ) {
        float chance_wrong = chance;
        chance = clamp<float>( chance, 0.f, 100.f );
        jo.throw_error_at( "chance",
                           string_format( "\"chance\" is defined as %f, "
                                          "but must be a decimal number between 0.0 and 100.0", chance_wrong ) );
    }
}

void mtype::load( const JsonObject &jo, const std::string &src )
{
    bool strict = src == "dda";

    MonsterGenerator &gen = MonsterGenerator::generator();

    name.make_plural();
    mandatory( jo, was_loaded, "name", name );

    optional( jo, was_loaded, "description", description );

    assign( jo, "ascii_picture", picture_id );

    if( jo.has_member( "material" ) ) {
        mat.clear();
        for( const std::string &m : jo.get_tags( "material" ) ) {
            mat.emplace( m, 1 );
            mat_portion_total += 1;
        }
    }
    if( mat.empty() ) { // Assign a default "flesh" material to prevent crash (#48988)
        mat.emplace( material_flesh, 1 );
        mat_portion_total += 1;
    }
    optional( jo, was_loaded, "species", species, string_id_reader<::species_type> {} );
    optional( jo, was_loaded, "categories", categories, auto_flags_reader<> {} );

    // See monfaction.cpp
    if( !was_loaded || jo.has_member( "default_faction" ) ) {
        default_faction = mfaction_str_id( jo.get_string( "default_faction" ) );
    }

    if( !was_loaded || jo.has_member( "symbol" ) ) {
        sym = jo.get_string( "symbol" );
        if( utf8_width( sym ) != 1 ) {
            jo.throw_error_at( "symbol", "monster symbol should be exactly one console cell width" );
        }
    }
    if( was_loaded && jo.has_member( "copy-from" ) && looks_like.empty() ) {
        looks_like = jo.get_string( "copy-from" );
    }
    jo.read( "looks_like", looks_like );

    assign( jo, "bodytype", bodytype );
    assign( jo, "color", color );
    assign( jo, "volume", volume, strict, 0_ml );
    assign( jo, "weight", weight, strict, 0_gram );

    optional( jo, was_loaded, "phase", phase, make_flag_reader( gen.phase_map, "phase id" ),
              phase_id::SOLID );

    assign( jo, "diff", difficulty_base, strict, 0 );
    assign( jo, "hp", hp, strict, 1 );
    assign( jo, "speed", speed, strict, 0 );
    assign( jo, "aggression", agro, strict, -100, 100 );
    assign( jo, "morale", morale, strict );

    assign( jo, "tracking_distance", tracking_distance, strict, 3 );

    assign( jo, "mountable_weight_ratio", mountable_weight_ratio, strict );

    assign( jo, "attack_cost", attack_cost, strict, 0 );
    assign( jo, "melee_skill", melee_skill, strict, 0 );
    assign( jo, "melee_dice", melee_dice, strict, 0 );
    assign( jo, "melee_dice_sides", melee_sides, strict, 0 );

    assign( jo, "grab_strength", grab_strength, strict, 0 );

    assign( jo, "dodge", sk_dodge, strict, 0 );
    assign( jo, "armor_bash", armor_bash, strict, 0 );
    assign( jo, "armor_cut", armor_cut, strict, 0 );
    assign( jo, "armor_bullet", armor_bullet, strict, 0 );
    assign( jo, "armor_stab", armor_stab, strict, 0 );
    assign( jo, "armor_acid", armor_acid, strict, 0 );
    assign( jo, "armor_fire", armor_fire, strict, 0 );
    assign( jo, "armor_elec", armor_elec, strict, 0 );
    assign( jo, "armor_cold", armor_cold, strict, 0 );
    assign( jo, "armor_pure", armor_pure, strict, 0 );
    assign( jo, "armor_biological", armor_biological, strict, 0 );

    optional( jo, was_loaded, "trap_avoids", trap_avoids );

    if( !was_loaded ) {
        weakpoints_deferred.clear();
        weakpoints_deferred_inline.clear();
    }
    weakpoints_deferred_deleted.clear();

    // Load each set of weakpoints.
    // Each subsequent weakpoint set overwrites
    // matching weakpoints from the previous set.
    if( jo.has_array( "weakpoint_sets" ) ) {
        weakpoints_deferred.clear();
        for( JsonValue jval : jo.get_array( "weakpoint_sets" ) ) {
            weakpoints_deferred.emplace_back( weakpoints_id( jval.get_string() ) );
        }
    }

    // Finally, inline weakpoints overwrite
    // any matching weakpoints from the previous sets.
    if( jo.has_array( "weakpoints" ) ) {
        weakpoints_deferred_inline.clear();
        ::weakpoints tmp_wp;
        tmp_wp.load( jo.get_array( "weakpoints" ) );
        weakpoints_deferred_inline.add_from_set( tmp_wp, true );
    }

    if( jo.has_object( "extend" ) ) {
        JsonObject tmp = jo.get_object( "extend" );
        tmp.allow_omitted_members();
        if( tmp.has_array( "weakpoint_sets" ) ) {
            for( JsonValue jval : tmp.get_array( "weakpoint_sets" ) ) {
                weakpoints_deferred.emplace_back( jval.get_string() );
            }
        }
        if( tmp.has_array( "weakpoints" ) ) {
            ::weakpoints tmp_wp;
            tmp_wp.load( tmp.get_array( "weakpoints" ) );
            weakpoints_deferred_inline.add_from_set( tmp_wp, true );
        }
    }

    if( jo.has_object( "delete" ) ) {
        JsonObject tmp = jo.get_object( "delete" );
        tmp.allow_omitted_members();
        if( tmp.has_array( "weakpoint_sets" ) ) {
            for( JsonValue jval : tmp.get_array( "weakpoint_sets" ) ) {
                weakpoints_id set_id( jval.get_string() );
                auto iter = std::find( weakpoints_deferred.begin(), weakpoints_deferred.end(), set_id );
                if( iter != weakpoints_deferred.end() ) {
                    weakpoints_deferred.erase( iter );
                }
            }
        }
        if( tmp.has_array( "weakpoints" ) ) {
            ::weakpoints tmp_wp;
            tmp_wp.load( tmp.get_array( "weakpoints" ) );
            for( const weakpoint &wp_del : tmp_wp.weakpoint_list ) {
                weakpoints_deferred_deleted.emplace( wp_del.id );
            }
            weakpoints_deferred_inline.del_from_set( tmp_wp );
        }
    }

    assign( jo, "status_chance_multiplier", status_chance_multiplier, strict, 0.0f, 5.0f );

    if( !was_loaded || jo.has_array( "families" ) ) {
        families.clear();
        families.load( jo.get_array( "families" ) );
    } else {
        if( jo.has_object( "extend" ) ) {
            JsonObject tmp = jo.get_object( "extend" );
            tmp.allow_omitted_members();
            if( tmp.has_array( "families" ) ) {
                families.load( tmp.get_array( "families" ) );
            }
        }
        if( jo.has_object( "delete" ) ) {
            JsonObject tmp = jo.get_object( "delete" );
            tmp.allow_omitted_members();
            if( tmp.has_array( "families" ) ) {
                families.remove( tmp.get_array( "families" ) );
            }
        }
    }

    optional( jo, was_loaded, "absorb_ml_per_hp", absorb_ml_per_hp, 250 );
    optional( jo, was_loaded, "split_move_cost", split_move_cost, 200 );
    optional( jo, was_loaded, "absorb_move_cost_per_ml", absorb_move_cost_per_ml, 0.025f );
    optional( jo, was_loaded, "absorb_move_cost_min", absorb_move_cost_min, 1 );
    optional( jo, was_loaded, "absorb_move_cost_max", absorb_move_cost_max, -1 );

    if( jo.has_member( "absorb_material" ) ) {
        absorb_material.clear();
        if( jo.has_array( "absorb_material" ) ) {
            for( std::string mat : jo.get_string_array( "absorb_material" ) ) {
                absorb_material.emplace_back( material_id( mat ) );
            }
        } else {
            absorb_material.emplace_back( material_id( jo.get_string( "absorb_material" ) ) );
        }
    }

    optional( jo, was_loaded, "bleed_rate", bleed_rate, 100 );

    optional( jo, was_loaded, "petfood", petfood );

    assign( jo, "vision_day", vision_day, strict, 0 );
    assign( jo, "vision_night", vision_night, strict, 0 );

    optional( jo, was_loaded, "regenerates", regenerates, 0 );
    optional( jo, was_loaded, "regenerates_in_dark", regenerates_in_dark, false );
    optional( jo, was_loaded, "regen_morale", regen_morale, false );

    if( !was_loaded || jo.has_member( "regeneration_modifiers" ) ) {
        regeneration_modifiers.clear();
        add_regeneration_modifiers( jo, "regeneration_modifiers", src );
    } else {
        // Note: regeneration_modifers left as is, new modifiers are added to it!
        // Note: member name prefixes are compatible with those used by generic_typed_reader
        if( jo.has_object( "extend" ) ) {
            JsonObject tmp = jo.get_object( "extend" );
            tmp.allow_omitted_members();
            add_regeneration_modifiers( tmp, "regeneration_modifiers", src );
        }
        if( jo.has_object( "delete" ) ) {
            JsonObject tmp = jo.get_object( "delete" );
            tmp.allow_omitted_members();
            remove_regeneration_modifiers( tmp, "regeneration_modifiers", src );
        }
    }

    optional( jo, was_loaded, "starting_ammo", starting_ammo );
    optional( jo, was_loaded, "luminance", luminance, 0 );
    optional( jo, was_loaded, "revert_to_itype", revert_to_itype, itype_id() );
    optional( jo, was_loaded, "mech_weapon", mech_weapon, itype_id() );
    optional( jo, was_loaded, "mech_str_bonus", mech_str_bonus, 0 );
    optional( jo, was_loaded, "mech_battery", mech_battery, itype_id() );

    optional( jo, was_loaded, "zombify_into", zombify_into, string_id_reader<::mtype> {},
              mtype_id() );
    optional( jo, was_loaded, "fungalize_into", fungalize_into, string_id_reader<::mtype> {},
              mtype_id() );

    optional( jo, was_loaded, "aggro_character", aggro_character, true );

    if( jo.has_array( "attack_effs" ) ) {
        atk_effs.clear();
        for( const JsonObject effect_jo : jo.get_array( "attack_effs" ) ) {
            mon_effect_data effect;
            effect.load( effect_jo );
            atk_effs.push_back( std::move( effect ) );
        }
    }

    // TODO: make this work with `was_loaded`
    if( jo.has_array( "melee_damage" ) ) {
        melee_damage = load_damage_instance( jo.get_array( "melee_damage" ) );
    } else if( jo.has_object( "melee_damage" ) ) {
        melee_damage = load_damage_instance( jo.get_object( "melee_damage" ) );
    } else if( jo.has_object( "relative" ) ) {
        std::optional<damage_instance> tmp_dmg;
        JsonObject rel = jo.get_object( "relative" );
        rel.allow_omitted_members();
        if( rel.has_array( "melee_damage" ) ) {
            tmp_dmg = load_damage_instance( rel.get_array( "melee_damage" ) );
        } else if( rel.has_object( "melee_damage" ) ) {
            tmp_dmg = load_damage_instance( rel.get_object( "melee_damage" ) );
        } else if( rel.has_int( "melee_damage" ) ) {
            const int rel_amt = rel.get_int( "melee_damage" );
            for( damage_unit &du : melee_damage ) {
                du.amount += rel_amt;
            }
        }
        if( !!tmp_dmg ) {
            melee_damage.add( tmp_dmg.value() );
        }
    } else if( jo.has_object( "proportional" ) ) {
        std::optional<damage_instance> tmp_dmg;
        JsonObject prop = jo.get_object( "proportional" );
        prop.allow_omitted_members();
        if( prop.has_array( "melee_damage" ) ) {
            tmp_dmg = load_damage_instance( prop.get_array( "melee_damage" ) );
        } else if( prop.has_object( "melee_damage" ) ) {
            tmp_dmg = load_damage_instance( prop.get_object( "melee_damage" ) );
        } else if( prop.has_float( "melee_damage" ) ) {
            melee_damage.mult_damage( prop.get_float( "melee_damage" ), true );
        }
        if( !!tmp_dmg ) {
            for( const damage_unit &du : tmp_dmg.value() ) {
                auto iter = std::find_if( melee_damage.begin(),
                melee_damage.end(), [&du]( const damage_unit & mdu ) {
                    return mdu.type == du.type;
                } );
                if( iter != melee_damage.end() ) {
                    iter->amount *= du.amount;
                }
            }
        }
    }

    if( jo.has_array( "scents_tracked" ) ) {
        for( const std::string line : jo.get_array( "scents_tracked" ) ) {
            scents_tracked.emplace( line );
        }
    }

    if( jo.has_array( "scents_ignored" ) ) {
        for( const std::string line : jo.get_array( "scents_ignored" ) ) {
            scents_ignored.emplace( line );
        }
    }

    if( jo.has_member( "death_drops" ) ) {
        death_drops =
            item_group::load_item_group( jo.get_member( "death_drops" ), "distribution",
                                         "death_drops for mtype " + id.str() );
    }

    assign( jo, "harvest", harvest );

    optional( jo, was_loaded, "dissect", dissect );

    if( jo.has_array( "shearing" ) ) {
        std::vector<shearing_entry> entries;
        for( JsonObject shearing_entry : jo.get_array( "shearing" ) ) {
            struct shearing_entry entry {};
            entry.load( shearing_entry );
            entries.emplace_back( entry );
        }
        shearing = shearing_data( entries );
    }

    optional( jo, was_loaded, "speed_description", speed_desc, speed_description_DEFAULT );
    optional( jo, was_loaded, "death_function", mdeath_effect );

    if( jo.has_array( "emit_fields" ) ) {
        JsonArray jar = jo.get_array( "emit_fields" );
        if( jar.has_string( 0 ) ) { // TEMPORARY until 0.F
            for( const std::string id : jar ) {
                emit_fields.emplace( emit_id( id ), 1_seconds );
            }
        } else {
            while( jar.has_more() ) {
                JsonObject obj = jar.next_object();
                emit_fields.emplace( emit_id( obj.get_string( "emit_id" ) ),
                                     read_from_json_string<time_duration>( obj.get_member( "delay" ), time_duration::units ) );
            }
        }
    }

    if( jo.has_member( "special_when_hit" ) ) {
        JsonArray jsarr = jo.get_array( "special_when_hit" );
        const auto iter = gen.defense_map.find( jsarr.get_string( 0 ) );
        if( iter == gen.defense_map.end() ) {
            jsarr.throw_error( "Invalid monster defense function" );
        }
        sp_defense = iter->second;
        def_chance = jsarr.get_int( 1 );
    } else if( !was_loaded ) {
        sp_defense = &mdefense::none;
        def_chance = 0;
    }

    if( !was_loaded || jo.has_member( "special_attacks" ) ) {
        special_attacks.clear();
        special_attacks_names.clear();
        add_special_attacks( jo, "special_attacks", src );
    } else {
        // Note: special_attacks left as is, new attacks are added to it!
        // Note: member name prefixes are compatible with those used by generic_typed_reader
        if( jo.has_object( "extend" ) ) {
            JsonObject tmp = jo.get_object( "extend" );
            tmp.allow_omitted_members();
            add_special_attacks( tmp, "special_attacks", src );
        }
        if( jo.has_object( "delete" ) ) {
            JsonObject tmp = jo.get_object( "delete" );
            tmp.allow_omitted_members();
            remove_special_attacks( tmp, "special_attacks", src );
        }
    }
    optional( jo, was_loaded, "melee_training_cap", melee_training_cap, std::min( melee_skill + 2,
              MAX_SKILL ) );
    optional( jo, was_loaded, "chat_topics", chat_topics );
    // Disable upgrading when JSON contains `"upgrades": false`, but fallback to the
    // normal behavior (including error checking) if "upgrades" is not boolean or not `false`.
    if( jo.has_bool( "upgrades" ) && !jo.get_bool( "upgrades" ) ) {
        upgrade_group = mongroup_id::NULL_ID();
        upgrade_into = mtype_id::NULL_ID();
        upgrades = false;
        upgrade_null_despawn = false;
    } else if( jo.has_member( "upgrades" ) ) {
        JsonObject up = jo.get_object( "upgrades" );
        optional( up, was_loaded, "half_life", half_life, -1 );
        optional( up, was_loaded, "age_grow", age_grow, -1 );
        optional( up, was_loaded, "into_group", upgrade_group, string_id_reader<::MonsterGroup> {},
                  mongroup_id::NULL_ID() );
        optional( up, was_loaded, "into", upgrade_into, string_id_reader<::mtype> {}, mtype_id::NULL_ID() );
        bool multi = !!upgrade_multi_range;
        optional( up, was_loaded, "multiple_spawns", multi, false );
        if( multi && jo.has_bool( "multiple_spawns" ) ) {
            mandatory( up, was_loaded, "spawn_range", upgrade_multi_range );
        } else if( multi ) {
            optional( up, was_loaded, "spawn_range", upgrade_multi_range );
        } else {
            jo.get_int( "spawn_range", 0 ); // ignore if defined
        }
        optional( up, was_loaded, "despawn_when_null", upgrade_null_despawn, false );
        upgrades = true;
    }

    //Reproduction
    if( jo.has_member( "reproduction" ) ) {
        JsonObject repro = jo.get_object( "reproduction" );
        optional( repro, was_loaded, "baby_count", baby_count, -1 );
        if( repro.has_int( "baby_timer" ) ) {
            baby_timer = time_duration::from_days( repro.get_int( "baby_timer" ) );
        } else if( repro.has_string( "baby_timer" ) ) {
            baby_timer = read_from_json_string<time_duration>( repro.get_member( "baby_timer" ),
                         time_duration::units );
        }
        optional( repro, was_loaded, "baby_monster", baby_monster, string_id_reader<::mtype> {},
                  mtype_id::NULL_ID() );
        optional( repro, was_loaded, "baby_egg", baby_egg, string_id_reader<::itype> {},
                  itype_id::NULL_ID() );
        reproduces = true;
    }

    if( jo.has_member( "baby_flags" ) ) {
        // Because this determines mating season and some monsters have a mating season but not in-game offspring, declare this separately
        baby_flags.clear();
        for( const std::string line : jo.get_array( "baby_flags" ) ) {
            baby_flags.push_back( line );
        }
    }

    if( jo.has_member( "biosignature" ) ) {
        JsonObject biosig = jo.get_object( "biosignature" );
        if( biosig.has_int( "biosig_timer" ) ) {
            biosig_timer = time_duration::from_days( biosig.get_int( "biosig_timer" ) );
        } else if( biosig.has_string( "biosig_timer" ) ) {
            biosig_timer = read_from_json_string<time_duration>( biosig.get_member( "biosig_timer" ),
                           time_duration::units );
        }

        optional( biosig, was_loaded, "biosig_item", biosig_item, string_id_reader<::itype> {},
                  itype_id::NULL_ID() );
        biosignatures = true;
    }

    optional( jo, was_loaded, "burn_into", burn_into, string_id_reader<::mtype> {},
              mtype_id::NULL_ID() );

    const auto flag_reader = enum_flags_reader<m_flag> { "monster flag" };
    optional( jo, was_loaded, "flags", flags, flag_reader );
    // Can't calculate yet - we want all flags first
    optional( jo, was_loaded, "bash_skill", bash_skill, -1 );

    const auto trigger_reader = enum_flags_reader<mon_trigger> { "monster trigger" };
    optional( jo, was_loaded, "anger_triggers", anger, trigger_reader );
    optional( jo, was_loaded, "placate_triggers", placate, trigger_reader );
    optional( jo, was_loaded, "fear_triggers", fear, trigger_reader );

    if( jo.has_member( "path_settings" ) ) {
        JsonObject jop = jo.get_object( "path_settings" );
        // Here rather than in pathfinding.cpp because we want monster-specific defaults and was_loaded
        optional( jop, was_loaded, "max_dist", path_settings.max_dist, 0 );
        optional( jop, was_loaded, "max_length", path_settings.max_length, -1 );
        optional( jop, was_loaded, "bash_strength", path_settings.bash_strength, -1 );
        optional( jop, was_loaded, "allow_open_doors", path_settings.allow_open_doors, false );
        optional( jop, was_loaded, "avoid_traps", path_settings.avoid_traps, false );
        optional( jop, was_loaded, "allow_climb_stairs", path_settings.allow_climb_stairs, true );
        optional( jop, was_loaded, "avoid_sharp", path_settings.avoid_sharp, false );
    }
    float melee_dmg_total = melee_damage.total_damage();
    difficulty = ( melee_skill + 1 ) * melee_dice * ( melee_dmg_total + melee_sides ) * 0.04 +
                 ( sk_dodge + 1 ) * ( 3 + armor_bash + armor_cut ) * 0.04 +
                 ( difficulty_base + special_attacks.size() + 8 * emit_fields.size() );
    difficulty *= ( hp + speed - attack_cost + ( morale + agro ) * 0.1 ) * 0.01 +
                  ( vision_day + 2 * vision_night ) * 0.01;

}

void MonsterGenerator::load_species( const JsonObject &jo, const std::string &src )
{
    mon_species->load( jo, src );
}

void species_type::load( const JsonObject &jo, const std::string & )
{
    optional( jo, was_loaded, "description", description );
    optional( jo, was_loaded, "footsteps", footsteps, to_translation( "footsteps." ) );
    const auto flag_reader = enum_flags_reader<m_flag> { "monster flag" };
    optional( jo, was_loaded, "flags", flags, flag_reader );

    const auto trigger_reader = enum_flags_reader<mon_trigger> { "monster trigger" };
    optional( jo, was_loaded, "anger_triggers", anger, trigger_reader );
    optional( jo, was_loaded, "placate_triggers", placate, trigger_reader );
    optional( jo, was_loaded, "fear_triggers", fear, trigger_reader );

    optional( jo, was_loaded, "bleeds", bleeds, string_id_reader<::field_type> {}, fd_null );
}

const std::vector<mtype> &MonsterGenerator::get_all_mtypes() const
{
    return mon_templates->get_all();
}

mtype_id MonsterGenerator::get_valid_hallucination() const
{
    return random_entry( hallucination_monsters );
}

class mattack_hardcoded_wrapper : public mattack_actor
{
    private:
        mon_action_attack cpp_function;
    public:
        mattack_hardcoded_wrapper( const mattack_id &id, const mon_action_attack f )
            : mattack_actor( id )
            , cpp_function( f ) { }

        ~mattack_hardcoded_wrapper() override = default;
        bool call( monster &m ) const override {
            return cpp_function( &m );
        }
        std::unique_ptr<mattack_actor> clone() const override {
            return std::make_unique<mattack_hardcoded_wrapper>( *this );
        }

        void load_internal( const JsonObject &, const std::string & ) override {}
};

mtype_special_attack::mtype_special_attack( const mattack_id &id, const mon_action_attack f )
    : mtype_special_attack( std::make_unique<mattack_hardcoded_wrapper>( id, f ) ) {}

void MonsterGenerator::add_hardcoded_attack( const std::string &type, const mon_action_attack f )
{
    add_attack( mtype_special_attack( type, f ) );
}

void MonsterGenerator::add_attack( std::unique_ptr<mattack_actor> ptr )
{
    add_attack( mtype_special_attack( std::move( ptr ) ) );
}

void MonsterGenerator::add_attack( const mtype_special_attack &wrapper )
{
    if( attack_map.count( wrapper->id ) > 0 ) {
        if( test_mode ) {
            debugmsg( "Overwriting monster attack with id %s", wrapper->id.c_str() );
        }

        attack_map.erase( wrapper->id );
    }

    attack_map.emplace( wrapper->id, wrapper );
}

mtype_special_attack MonsterGenerator::create_actor( const JsonObject &obj,
        const std::string &src ) const
{
    // Legacy support: tolerate attack types being specified as the type
    const std::string type = obj.get_string( "type", "monster_attack" );
    const std::string attack_type = obj.get_string( "attack_type", type );

    if( type != "monster_attack" && attack_type != type ) {
        obj.throw_error_at(
            "type",
            R"(Specifying "attack_type" is only allowed when "type" is "monster_attack" or not specified)" );
    }

    std::unique_ptr<mattack_actor> new_attack;
    if( attack_type == "monster_attack" ) {
        const std::string id = obj.get_string( "id" );
        const auto &iter = attack_map.find( id );
        if( iter == attack_map.end() ) {
            obj.throw_error_at( "type", "Monster attacks must specify type and/or id" );
        }

        new_attack = iter->second->clone();
    } else if( attack_type == "leap" ) {
        new_attack = std::make_unique<leap_actor>();
    } else if( attack_type == "melee" ) {
        new_attack = std::make_unique<melee_actor>();
    } else if( attack_type == "bite" ) {
        new_attack = std::make_unique<bite_actor>();
    } else if( attack_type == "gun" ) {
        new_attack = std::make_unique<gun_actor>();
    } else if( attack_type == "spell" ) {
        new_attack = std::make_unique<mon_spellcasting_actor>();
    } else {
        obj.throw_error_at( "attack_type", "unknown monster attack" );
    }

    new_attack->load( obj, src );
    return mtype_special_attack( std::move( new_attack ) );
}

void mattack_actor::load( const JsonObject &jo, const std::string &src )
{
    bool strict = src == "dda";

    // Legacy support
    if( !jo.has_string( "id" ) ) {
        id = jo.get_string( "type" );
    } else {
        // Loading ids can't be strict at the moment, since it has to match the stored version
        assign( jo, "id", id, false );
    }

    assign( jo, "cooldown", cooldown, strict );

    load_internal( jo, src );
    // Set was_loaded manually because we don't have generic_factory to do it for us
    was_loaded = true;
}

void MonsterGenerator::load_monster_attack( const JsonObject &jo, const std::string &src )
{
    add_attack( create_actor( jo, src ) );
}

void mtype::add_special_attack( const JsonObject &obj, const std::string &src )
{
    mtype_special_attack new_attack = MonsterGenerator::generator().create_actor( obj, src );

    if( special_attacks.count( new_attack->id ) > 0 ) {
        special_attacks.erase( new_attack->id );
        const auto iter = std::find( special_attacks_names.begin(), special_attacks_names.end(),
                                     new_attack->id );
        if( iter != special_attacks_names.end() ) {
            special_attacks_names.erase( iter );
        }
        if( test_mode ) {
            debugmsg( "%s specifies more than one attack of (sub)type %s, ignoring all but the last",
                      id.c_str(), new_attack->id.c_str() );
        }
    }

    special_attacks.emplace( new_attack->id, new_attack );
    special_attacks_names.push_back( new_attack->id );
}

void mtype::add_special_attack( const JsonArray &inner, const std::string & )
{
    MonsterGenerator &gen = MonsterGenerator::generator();
    const std::string name = inner.get_string( 0 );
    const auto iter = gen.attack_map.find( name );
    if( iter == gen.attack_map.end() ) {
        inner.throw_error( "Invalid special_attacks" );
    }

    if( special_attacks.count( name ) > 0 ) {
        special_attacks.erase( name );
        const auto iter = std::find( special_attacks_names.begin(), special_attacks_names.end(), name );
        if( iter != special_attacks_names.end() ) {
            special_attacks_names.erase( iter );
        }
        if( test_mode ) {
            debugmsg( "%s specifies more than one attack of (sub)type %s, ignoring all but the last",
                      id.c_str(), name );
        }
    }
    mtype_special_attack new_attack = mtype_special_attack( iter->second );
    new_attack.actor->cooldown = inner.get_int( 1 );
    special_attacks.emplace( name, new_attack );
    special_attacks_names.push_back( name );
}

void mtype::add_special_attacks( const JsonObject &jo, const std::string &member,
                                 const std::string &src )
{

    if( !jo.has_array( member ) ) {
        return;
    }

    for( const JsonValue entry : jo.get_array( member ) ) {
        if( entry.test_array() ) {
            add_special_attack( entry.get_array(), src );
        } else if( entry.test_object() ) {
            add_special_attack( entry.get_object(), src );
        } else {
            entry.throw_error( "array element is neither array nor object." );
        }
    }
}

void mtype::remove_special_attacks( const JsonObject &jo, const std::string &member_name,
                                    const std::string & )
{
    for( const std::string &name : jo.get_tags( member_name ) ) {
        special_attacks.erase( name );
        const auto iter = std::find( special_attacks_names.begin(), special_attacks_names.end(), name );
        if( iter != special_attacks_names.end() ) {
            special_attacks_names.erase( iter );
        }
    }
}

void mtype::add_regeneration_modifier( const JsonArray &inner, const std::string & )
{
    const std::string effect_name = inner.get_string( 0 );
    const efftype_id effect( effect_name );
    //TODO: if invalid effect, throw error
    //  inner.throw_error( "Invalid regeneration_modifiers" );

    if( regeneration_modifiers.count( effect ) > 0 ) {
        regeneration_modifiers.erase( effect );
        if( test_mode ) {
            debugmsg( "%s specifies more than one regeneration modifier for effect %s, ignoring all but the last",
                      id.c_str(), effect_name );
        }
    }
    int amount = inner.get_int( 1 );
    regeneration_modifiers.emplace( effect, amount );
}

void mtype::add_regeneration_modifiers( const JsonObject &jo, const std::string &member,
                                        const std::string &src )
{
    if( !jo.has_array( member ) ) {
        return;
    }

    for( const JsonValue entry : jo.get_array( member ) ) {
        if( entry.test_array() ) {
            add_regeneration_modifier( entry.get_array(), src );
            // TODO: add support for regeneration_modifer objects
            //} else if ( entry.test_object() ) {
            //    add_regeneration_modifier( entry.get_object(), src );
        } else {
            entry.throw_error( "array element is not an array " );
        }
    }
}

void mtype::remove_regeneration_modifiers( const JsonObject &jo, const std::string &member_name,
        const std::string & )
{
    for( const std::string &name : jo.get_tags( member_name ) ) {
        const efftype_id effect( name );
        regeneration_modifiers.erase( effect );
    }
}

void MonsterGenerator::check_monster_definitions() const
{
    for( const mtype &mon : mon_templates->get_all() ) {
        if( mon.harvest.is_null() && !mon.has_flag( MF_ELECTRONIC ) && !mon.id.is_null() ) {
            debugmsg( "monster %s has no harvest entry", mon.id.c_str(), mon.harvest.c_str() );
        }
        if( mon.has_flag( MF_MILKABLE ) && mon.starting_ammo.empty() ) {
            debugmsg( "monster %s is flagged milkable, but has no starting ammo", mon.id.c_str() );
        }
        if( mon.has_flag( MF_MILKABLE ) && !mon.starting_ammo.empty() &&
            !item( mon.starting_ammo.begin()->first ).made_of( phase_id::LIQUID ) ) {
            debugmsg( "monster %s is flagged milkable, but starting ammo %s is not a liquid type",
                      mon.id.c_str(), mon.starting_ammo.begin()->first.str() );
        }
        if( mon.has_flag( MF_MILKABLE ) && mon.starting_ammo.size() > 1 ) {
            debugmsg( "monster %s is flagged milkable, but has multiple starting_ammo defined",
                      mon.id.c_str() );
        }
        for( const species_id &spec : mon.species ) {
            if( !spec.is_valid() ) {
                debugmsg( "monster %s has invalid species %s", mon.id.c_str(), spec.c_str() );
            }
        }
        if( !mon.death_drops.is_empty() && !item_group::group_is_defined( mon.death_drops ) ) {
            debugmsg( "monster %s has unknown death drop item group: %s", mon.id.c_str(),
                      mon.death_drops.c_str() );
        }
        for( const auto &m : mon.mat ) {
            if( m.first.str() == "null" || !m.first.is_valid() ) {
                debugmsg( "monster %s has unknown material: %s", mon.id.c_str(), m.first.c_str() );
            }
        }
        if( !mon.revert_to_itype.is_empty() && !item::type_is_defined( mon.revert_to_itype ) ) {
            debugmsg( "monster %s has unknown revert_to_itype: %s", mon.id.c_str(),
                      mon.revert_to_itype.c_str() );
        }
        if( !mon.zombify_into.is_empty() && !mon.zombify_into.is_valid() ) {
            debugmsg( "monster %s has unknown zombify_into: %s", mon.id.c_str(),
                      mon.zombify_into.c_str() );
        }
        if( !mon.fungalize_into.is_empty() && !mon.fungalize_into.is_valid() ) {
            debugmsg( "monster %s has unknown fungalize_into: %s", mon.id.c_str(),
                      mon.fungalize_into.c_str() );
        }
        if( !mon.picture_id.is_empty() && !mon.picture_id.is_valid() ) {
            debugmsg( "monster %s has unknown ascii_picture: %s", mon.id.c_str(),
                      mon.picture_id.c_str() );
        }
        if( !mon.mech_weapon.is_empty() && !item::type_is_defined( mon.mech_weapon ) ) {
            debugmsg( "monster %s has unknown mech_weapon: %s", mon.id.c_str(),
                      mon.mech_weapon.c_str() );
        }
        if( !mon.mech_battery.is_empty() && !item::type_is_defined( mon.mech_battery ) ) {
            debugmsg( "monster %s has unknown mech_battery: %s", mon.id.c_str(),
                      mon.mech_battery.c_str() );
        }
        if( !mon.harvest.is_valid() ) {
            debugmsg( "monster %s has invalid harvest_entry: %s", mon.id.c_str(), mon.harvest.c_str() );
        }
        if( !mon.dissect.is_empty() && !mon.dissect.is_valid() ) {
            debugmsg( "monster %s has invalid dissection harvest_entry: %s", mon.id.c_str(),
                      mon.dissect.c_str() );
        }
        if( mon.has_flag( MF_WATER_CAMOUFLAGE ) && !monster( mon.id ).can_submerge() ) {
            debugmsg( "monster %s has WATER_CAMOUFLAGE but cannot submerge", mon.id.c_str() );
        }
        for( const scenttype_id &s_id : mon.scents_tracked ) {
            if( !s_id.is_empty() && !s_id.is_valid() ) {
                debugmsg( "monster %s has unknown scents_tracked %s", mon.id.c_str(), s_id.c_str() );
            }
        }
        for( const scenttype_id &s_id : mon.scents_ignored ) {
            if( !s_id.is_empty() && !s_id.is_valid() ) {
                debugmsg( "monster %s has unknown scents_ignored %s", mon.id.c_str(), s_id.c_str() );
            }
        }
        for( const std::pair<const itype_id, int> &s : mon.starting_ammo ) {
            if( !item::type_is_defined( s.first ) ) {
                debugmsg( "starting ammo %s of monster %s is unknown", s.first.c_str(), mon.id.c_str() );
            }
        }
        for( const mon_effect_data &e : mon.atk_effs ) {
            if( !e.id.is_valid() ) {
                debugmsg( "attack effect %s of monster %s is unknown", e.id.c_str(), mon.id.c_str() );
            }
        }

        for( const std::pair<const emit_id, time_duration> &e : mon.emit_fields ) {
            const emit_id emid = e.first;
            if( !emid.is_valid() ) {
                debugmsg( "monster %s has invalid emit source %s", mon.id.c_str(), emid.c_str() );
            }
        }

        if( mon.upgrades ) {
            if( mon.half_life < 0 && mon.age_grow < 0 ) {
                debugmsg( "half_life %d and age_grow %d (<0) of monster %s is invalid",
                          mon.half_life, mon.age_grow, mon.id.c_str() );
            }
            if( !mon.upgrade_into && !mon.upgrade_group ) {
                debugmsg( "no into nor into_group defined for monster %s", mon.id.c_str() );
            }
            if( mon.upgrade_into && mon.upgrade_group ) {
                debugmsg( "both into and into_group defined for monster %s", mon.id.c_str() );
            }
            if( !mon.upgrade_into.is_valid() ) {
                debugmsg( "upgrade_into %s of monster %s is not a valid monster id",
                          mon.upgrade_into.c_str(), mon.id.c_str() );
            }
            if( !mon.upgrade_group.is_valid() ) {
                debugmsg( "upgrade_group %s of monster %s is not a valid monster group id",
                          mon.upgrade_group.c_str(), mon.id.c_str() );
            }
        }

        if( mon.reproduces ) {
            if( !mon.baby_timer || *mon.baby_timer <= 0_seconds ) {
                debugmsg( "Time between reproductions (%d) is invalid for %s",
                          mon.baby_timer ? to_turns<int>( *mon.baby_timer ) : -1, mon.id.c_str() );
            }
            if( mon.baby_count < 1 ) {
                debugmsg( "Number of children (%d) is invalid for %s",
                          mon.baby_count, mon.id.c_str() );
            }
            if( !mon.baby_monster && mon.baby_egg.is_null() ) {
                debugmsg( "No baby or egg defined for monster %s", mon.id.c_str() );
            }
            if( mon.baby_monster && !mon.baby_egg.is_null() ) {
                debugmsg( "Both an egg and a live birth baby are defined for %s", mon.id.c_str() );
            }
            if( !mon.baby_monster.is_valid() ) {
                debugmsg( "baby_monster %s of monster %s is not a valid monster id",
                          mon.baby_monster.c_str(), mon.id.c_str() );
            }
            if( !item::type_is_defined( mon.baby_egg ) ) {
                debugmsg( "item_id %s of monster %s is not a valid item id",
                          mon.baby_egg.c_str(), mon.id.c_str() );
            }
        }

        if( mon.biosignatures ) {
            if( !mon.biosig_timer || *mon.biosig_timer <= 0_seconds ) {
                debugmsg( "Time between biosignature drops (%d) is invalid for %s",
                          mon.biosig_timer ? to_turns<int>( *mon.biosig_timer ) : -1, mon.id.c_str() );
            }
            if( mon.biosig_item.is_null() ) {
                debugmsg( "No biosignature drop defined for monster %s", mon.id.c_str() );
            }
            if( !item::type_is_defined( mon.biosig_item ) ) {
                debugmsg( "item_id %s of monster %s is not a valid item id",
                          mon.biosig_item.c_str(), mon.id.c_str() );
            }
        }
    }
}

void monster_death_effect::load( const JsonObject &jo )
{
    optional( jo, was_loaded, "message", death_message, to_translation( "The %s dies!" ) );
    optional( jo, was_loaded, "effect", sp );
    has_effect = sp.is_valid();
    optional( jo, was_loaded, "corpse_type", corpse_type, mdeath_type::NORMAL );
}

void monster_death_effect::deserialize( const JsonObject &data )
{
    load( data );
}

void pet_food_data::load( const JsonObject &jo )
{
    mandatory( jo, was_loaded, "food", food );
    optional( jo, was_loaded, "feed", feed );
    optional( jo, was_loaded, "pet", pet );
}

void pet_food_data::deserialize( const JsonObject &data )
{
    load( data );
}
