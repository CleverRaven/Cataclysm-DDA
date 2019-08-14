#include "mattack_common.h" // IWYU pragma: associated
#include "monstergenerator.h" // IWYU pragma: associated

#include <algorithm>
#include <set>
#include <utility>

#include "catacharset.h"
#include "creature.h"
#include "debug.h"
#include "generic_factory.h"
#include "item.h"
#include "item_group.h"
#include "json.h"
#include "mattack_actors.h"
#include "monattack.h"
#include "mondeath.h"
#include "mondefense.h"
#include "monfaction.h"
#include "options.h"
#include "rng.h"
#include "assign.h"
#include "bodypart.h"
#include "damage.h"
#include "game.h"
#include "pathfinding.h"
#include "units.h"
#include "translations.h"

namespace
{

const std::map<std::string, mon_trigger> trigger_map = {
    { "STALK",              mon_trigger::STALK },
    { "MEAT",               mon_trigger::MEAT },
    { "PLAYER_WEAK",        mon_trigger::HOSTILE_WEAK },
    { "PLAYER_CLOSE",       mon_trigger::HOSTILE_CLOSE },
    { "HURT",               mon_trigger::HURT },
    { "FIRE",               mon_trigger::FIRE },
    { "FRIEND_DIED",        mon_trigger::FRIEND_DIED },
    { "FRIEND_ATTACKED",    mon_trigger::FRIEND_ATTACKED },
    { "SOUND",              mon_trigger::SOUND },
    { "PLAYER_NEAR_BABY",   mon_trigger::PLAYER_NEAR_BABY },
    { "MATING_SEASON",      mon_trigger::MATING_SEASON }
};

const std::map<std::string, m_flag> flag_map = {
    // see mtype.h for commentary
    { "SEES", MF_SEES },
    { "HEARS", MF_HEARS },
    { "GOODHEARING", MF_GOODHEARING },
    { "SMELLS", MF_SMELLS },
    { "KEENNOSE", MF_KEENNOSE },
    { "STUMBLES", MF_STUMBLES },
    { "WARM", MF_WARM },
    { "NOHEAD", MF_NOHEAD },
    { "HARDTOSHOOT", MF_HARDTOSHOOT },
    { "GRABS", MF_GRABS },
    { "BASHES", MF_BASHES },
    { "GROUP_BASH", MF_GROUP_BASH },
    { "DESTROYS", MF_DESTROYS },
    { "BORES", MF_BORES },
    { "POISON", MF_POISON },
    { "VENOM", MF_VENOM },
    { "BADVENOM", MF_BADVENOM },
    { "PARALYZEVENOM", MF_PARALYZE },
    { "BLEED", MF_BLEED },
    { "WEBWALK", MF_WEBWALK },
    { "DIGS", MF_DIGS },
    { "CAN_DIG", MF_CAN_DIG },
    { "CAN_OPEN_DOORS", MF_CAN_OPEN_DOORS },
    { "FLIES", MF_FLIES },
    { "AQUATIC", MF_AQUATIC },
    { "SWIMS", MF_SWIMS },
    { "FISHABLE", MF_FISHABLE },
    { "ATTACKMON", MF_ATTACKMON },
    { "ANIMAL", MF_ANIMAL },
    { "PLASTIC", MF_PLASTIC },
    { "SUNDEATH", MF_SUNDEATH },
    { "ELECTRIC", MF_ELECTRIC },
    { "ACIDPROOF", MF_ACIDPROOF },
    { "ACIDTRAIL", MF_ACIDTRAIL },
    { "SHORTACIDTRAIL", MF_SHORTACIDTRAIL },
    { "FIREPROOF", MF_FIREPROOF },
    { "SLUDGEPROOF", MF_SLUDGEPROOF },
    { "SLUDGETRAIL", MF_SLUDGETRAIL },
    { "FIREY", MF_FIREY },
    { "QUEEN", MF_QUEEN },
    { "ELECTRONIC", MF_ELECTRONIC },
    { "FUR", MF_FUR },
    { "LEATHER", MF_LEATHER },
    { "WOOL", MF_WOOL },
    { "FEATHER", MF_FEATHER },
    { "CBM_CIV", MF_CBM_CIV },
    { "BONES", MF_BONES },
    { "FAT", MF_FAT },
    { "IMMOBILE", MF_IMMOBILE },
    { "RIDEABLE_MECH", MF_RIDEABLE_MECH },
    { "MILITARY_MECH", MF_MILITARY_MECH },
    { "MECH_RECON_VISION", MF_MECH_RECON_VISION },
    { "HIT_AND_RUN", MF_HIT_AND_RUN },
    { "GUILT", MF_GUILT },
    { "HUMAN", MF_HUMAN },
    { "NO_BREATHE", MF_NO_BREATHE },
    { "REGENERATES_50", MF_REGENERATES_50 },
    { "REGENERATES_10", MF_REGENERATES_10 },
    { "REGENERATES_1", MF_REGENERATES_1 },
    { "REGENERATES_IN_DARK", MF_REGENERATES_IN_DARK },
    { "FLAMMABLE", MF_FLAMMABLE },
    { "REVIVES", MF_REVIVES },
    { "CHITIN", MF_CHITIN },
    { "VERMIN", MF_VERMIN },
    { "NOGIB", MF_NOGIB },
    { "ABSORBS", MF_ABSORBS },
    { "ABSORBS_SPLITS", MF_ABSORBS_SPLITS },
    { "LARVA", MF_LARVA },
    { "ARTHROPOD_BLOOD", MF_ARTHROPOD_BLOOD },
    { "ACID_BLOOD", MF_ACID_BLOOD },
    { "BILE_BLOOD", MF_BILE_BLOOD },
    { "REGEN_MORALE", MF_REGENMORALE },
    { "CBM_POWER", MF_CBM_POWER },
    { "CBM_SCI", MF_CBM_SCI },
    { "CBM_OP", MF_CBM_OP },
    { "CBM_TECH", MF_CBM_TECH },
    { "CBM_SUBS", MF_CBM_SUBS },
    { "FILTHY", MF_FILTHY },
    { "SWARMS", MF_SWARMS },
    { "CLIMBS", MF_CLIMBS },
    { "GROUP_MORALE", MF_GROUP_MORALE },
    { "INTERIOR_AMMO", MF_INTERIOR_AMMO },
    { "NIGHT_INVISIBILITY", MF_NIGHT_INVISIBILITY },
    { "REVIVES_HEALTHY", MF_REVIVES_HEALTHY },
    { "NO_NECRO", MF_NO_NECRO },
    { "PACIFIST", MF_PACIFIST },
    { "PUSH_MON", MF_PUSH_MON },
    { "PUSH_VEH", MF_PUSH_VEH },
    { "PATH_AVOID_DANGER_1", MF_AVOID_DANGER_1 },
    { "PATH_AVOID_DANGER_2", MF_AVOID_DANGER_2 },
    { "PATH_AVOID_FALL", MF_AVOID_FALL },
    { "PATH_AVOID_FIRE", MF_AVOID_FIRE },
    { "PRIORITIZE_TARGETS", MF_PRIORITIZE_TARGETS },
    { "NOT_HALLUCINATION", MF_NOT_HALLU },
    { "CATFOOD", MF_CATFOOD },
    { "CANPLAY", MF_CANPLAY },
    { "CATTLEFODDER", MF_CATTLEFODDER },
    { "BIRDFOOD", MF_BIRDFOOD },
    { "PET_MOUNTABLE", MF_PET_MOUNTABLE },
    { "DOGFOOD", MF_DOGFOOD },
    { "MILKABLE", MF_MILKABLE },
    { "NO_BREED", MF_NO_BREED },
    { "PET_WONT_FOLLOW", MF_PET_WONT_FOLLOW },
    { "DRIPS_NAPALM", MF_DRIPS_NAPALM },
    { "DRIPS_GASOLINE", MF_DRIPS_GASOLINE },
    { "ELECTRIC_FIELD", MF_ELECTRIC_FIELD },
    { "STUN_IMMUNE", MF_STUN_IMMUNE },
    { "LOUDMOVES", MF_LOUDMOVES },
    { "DROPS_AMMO", MF_DROPS_AMMO }
};

} // namespace

namespace io
{

template<>
mon_trigger string_to_enum<mon_trigger>( const std::string &trigger )
{
    return string_to_enum_look_up( trigger_map, trigger );
}

template<>
m_flag string_to_enum<m_flag>( const std::string &flag )
{
    return string_to_enum_look_up( flag_map, flag );
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

MonsterGenerator::MonsterGenerator()
    : mon_templates( "monster type", "id", "alias" )
    , mon_species( "species" )
{
    mon_templates->insert( mtype() );
    mon_species->insert( species_type() );
    init_phases();
    init_attack();
    init_defense();
    init_death();
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
    int ret = t.melee_dice * t.melee_sides; // IOW, the critter's max bashing damage
    if( t.has_flag( MF_BORES ) ) {
        ret *= 15; // This is for stuff that goes through solid rock: minerbots, dark wyrms, etc
    } else if( t.has_flag( MF_DESTROYS ) ) {
        ret *= 2.5;
    } else if( !t.has_flag( MF_BASHES ) ) {
        ret = 0;
    }

    return ret;
}

static m_size volume_to_size( const units::volume vol )
{
    if( vol <= 7500_ml ) {
        return MS_TINY;
    } else if( vol <= 46250_ml ) {
        return MS_SMALL;
    } else if( vol <= 77500_ml ) {
        return MS_MEDIUM;
    } else if( vol <= 483750_ml ) {
        return MS_LARGE;
    }
    return MS_HUGE;
}

struct monster_adjustment {
    species_id species;
    std::string stat;
    float stat_adjust;
    std::string flag;
    bool flag_val;
    std::string special;
    void apply( mtype &mon );
};

void monster_adjustment::apply( mtype &mon )
{
    if( !mon.in_species( species ) ) {
        return;
    }
    if( !stat.empty() ) {
        if( stat == "speed" ) {
            mon.speed *= stat_adjust;
        } else if( stat == "hp" ) {
            mon.hp *= stat_adjust;
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

void load_monster_adjustment( JsonObject &jsobj )
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

void MonsterGenerator::finalize_mtypes()
{
    mon_templates->finalize();
    for( const auto &elem : mon_templates->get_all() ) {
        mtype &mon = const_cast<mtype &>( elem );
        apply_species_attributes( mon );
        set_species_ids( mon );
        mon.size = volume_to_size( mon.volume );

        // adjust for worldgen difficulty parameters
        mon.speed *= get_option<int>( "MONSTER_SPEED" )      / 100.0;
        mon.hp    *= get_option<int>( "MONSTER_RESILIENCE" ) / 100.0;

        for( monster_adjustment adj : adjustments ) {
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
        if( mon.armor_acid < 0 ) {
            mon.armor_acid = mon.armor_cut * 0.5;
        }
        if( mon.armor_fire < 0 ) {
            mon.armor_fire = 0;
        }

        mon.hp = std::max( mon.hp, 1 ); // lower bound for hp scaling

        finalize_pathfinding_settings( mon );
    }

    for( const auto &mon : mon_templates->get_all() ) {
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
    phase_map["NULL"] = PNULL;
    phase_map["SOLID"] = SOLID;
    phase_map["LIQUID"] = LIQUID;
    phase_map["GAS"] = GAS;
    phase_map["PLASMA"] = PLASMA;
}

void MonsterGenerator::init_death()
{
    death_map["NORMAL"] = &mdeath::normal;// Drop a body
    death_map["SPLATTER"] = &mdeath::splatter;//Explodes in gibs and chunks
    death_map["ACID"] = &mdeath::acid;// Acid instead of a body
    death_map["BOOMER"] = &mdeath::boomer;// Explodes in vomit :3
    death_map["BOOMER_GLOW"] = &mdeath::boomer_glow;// Explodes in glowing vomit :3
    death_map["KILL_VINES"] = &mdeath::kill_vines;// Kill all nearby vines
    death_map["VINE_CUT"] = &mdeath::vine_cut;// Kill adjacent vine if it's cut
    death_map["TRIFFID_HEART"] = &mdeath::triffid_heart;// Destroy all roots
    death_map["FUNGUS"] = &mdeath::fungus;// Explodes in spores D:
    death_map["DISINTEGRATE"] = &mdeath::disintegrate;// Falls apart
    death_map["WORM"] = &mdeath::worm;// Spawns 2 half-worms
    death_map["DISAPPEAR"] = &mdeath::disappear;// Hallucination disappears
    death_map["GUILT"] = &mdeath::guilt;// Morale penalty
    death_map["BRAINBLOB"] = &mdeath::brainblob;// Frees blobs, redirects to brainblob()
    death_map["BLOBSPLIT"] = &mdeath::blobsplit;// Creates more blobs
    death_map["JACKSON"] = &mdeath::jackson;// Reverts dancers
    death_map["MELT"] = &mdeath::melt;// Normal death, but melts
    death_map["AMIGARA"] = &mdeath::amigara;// Removes hypnosis if last one
    death_map["THING"] = &mdeath::thing;// Turn into a full thing
    death_map["EXPLODE"] = &mdeath::explode;// Damaging explosion
    death_map["FOCUSEDBEAM"] = &mdeath::focused_beam;// Blinding ray
    death_map["BROKEN"] = &mdeath::broken;// Spawns a broken robot.
    death_map["RATKING"] = &mdeath::ratking;// Cure verminitis
    death_map["DARKMAN"] = &mdeath::darkman;// sight returns to normal
    death_map["GAS"] = &mdeath::gas;// Explodes in toxic gas
    death_map["KILL_BREATHERS"] = &mdeath::kill_breathers;// All breathers die
    // Gives a message about destroying ammo and then calls "BROKEN"
    death_map["BROKEN_AMMO"] = &mdeath::broken_ammo;
    death_map["SMOKEBURST"] = &mdeath::smokeburst;// Explode like a huge smoke bomb.
    death_map["FUNGALBURST"] = &mdeath::fungalburst;// Explode with a cloud of fungal haze.
    death_map["JABBERWOCKY"] = &mdeath::jabberwock; // Snicker-snack!
    death_map["DETONATE"] = &mdeath::detonate; // Take them with you
    death_map["GAMEOVER"] = &mdeath::gameover;// Game over!  Defense mode
    death_map["PREG_ROACH"] = &mdeath::preg_roach;// Spawn some cockroach nymphs
    death_map["FIREBALL"] = &mdeath::fireball;// Explode in a fireball
    death_map["CONFLAGRATION"] = &mdeath::conflagration; // Explode in a huge fireball

    /* Currently Unimplemented */
    //death_map["SHRIEK"] = &mdeath::shriek;// Screams loudly
    //death_map["HOWL"] = &mdeath::howl;// Wolf's howling
    //death_map["RATTLE"] = &mdeath::rattle;// Rattles like a rattlesnake
}

void MonsterGenerator::init_attack()
{
    add_hardcoded_attack( "NONE", mattack::none );
    add_hardcoded_attack( "EAT_CROP", mattack::eat_crop );
    add_hardcoded_attack( "EAT_FOOD", mattack::eat_food );
    add_hardcoded_attack( "ANTQUEEN", mattack::antqueen );
    add_hardcoded_attack( "CHECK_UP", mattack::nurse_check_up );
    add_hardcoded_attack( "ASSIST", mattack::nurse_assist );
    add_hardcoded_attack( "OPERATE", mattack::nurse_operate );
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
    add_hardcoded_attack( "DARKMAN", mattack::darkman );
    add_hardcoded_attack( "SLIMESPRING", mattack::slimespring );
    add_hardcoded_attack( "BIO_OP_TAKEDOWN", mattack::bio_op_takedown );
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
}

void MonsterGenerator::init_defense()
{
    defense_map["NONE"] = &mdefense::none; //No special attack-back
    defense_map["ZAPBACK"] = &mdefense::zapback; //Shock attacker on hit
    defense_map["ACIDSPLASH"] = &mdefense::acidsplash; //Splash acid on the attacker
}

void MonsterGenerator::set_species_ids( mtype &mon )
{
    for( const auto &s : mon.species ) {
        if( s.is_valid() ) {
            mon.species_ptrs.insert( &s.obj() );
        } else {
            debugmsg( "Tried to assign species %s to monster %s, but no entry for the species exists",
                      s.c_str(), mon.id.c_str() );
        }
    }
}

void MonsterGenerator::load_monster( JsonObject &jo, const std::string &src )
{
    mon_templates->load( jo, src );
}

mon_effect_data load_mon_effect_data( JsonObject &e )
{
    return mon_effect_data( efftype_id( e.get_string( "id" ) ), e.get_int( "duration", 0 ),
                            e.get_bool( "affect_hit_bp", false ),
                            get_body_part_token( e.get_string( "bp", "NUM_BP" ) ),
                            e.get_bool( "permanent", false ),
                            e.get_int( "chance", 100 ) );
}

class mon_attack_effect_reader : public generic_typed_reader<mon_attack_effect_reader>
{
    public:
        mon_effect_data get_next( JsonIn &jin ) const {
            JsonObject e = jin.get_object();
            return load_mon_effect_data( e );
        }
        template<typename C>
        void erase_next( JsonIn &jin, C &container ) const {
            const efftype_id id = efftype_id( jin.get_string() );
            reader_detail::handler<C>().erase_if( container, [&id]( const mon_effect_data & e ) {
                return e.id == id;
            } );
        }
};

void mtype::load( JsonObject &jo, const std::string &src )
{
    bool strict = src == "dda";

    MonsterGenerator &gen = MonsterGenerator::generator();

    // Name and name plural are not translated here, but when needed in
    // combination with the actual count in `mtype::nname`.
    mandatory( jo, was_loaded, "name", name );
    // default behavior: Assume the regular plural form (appending an “s”)
    optional( jo, was_loaded, "name_plural", name_plural, name + "s" );
    optional( jo, was_loaded, "description", description );

    optional( jo, was_loaded, "material", mat, auto_flags_reader<material_id> {} );
    optional( jo, was_loaded, "species", species, auto_flags_reader<species_id> {} );
    optional( jo, was_loaded, "categories", categories, auto_flags_reader<> {} );

    // See monfaction.cpp
    if( !was_loaded || jo.has_member( "default_faction" ) ) {
        const auto faction = mfaction_str_id( jo.get_string( "default_faction" ) );
        default_faction = monfactions::get_or_add_faction( faction );
    }

    if( !was_loaded || jo.has_member( "symbol" ) ) {
        sym = jo.get_string( "symbol" );
        if( utf8_wrapper( sym ).display_width() != 1 ) {
            jo.throw_error( "monster symbol should be exactly one console cell width", "symbol" );
        }
    }
    if( was_loaded && jo.has_member( "copy-from" ) && looks_like.empty() ) {
        looks_like = jo.get_string( "copy-from" );
    }
    if( jo.has_member( "looks_like" ) ) {
        looks_like = jo.get_string( "looks_like" );
    }

    assign( jo, "bodytype", bodytype );
    assign( jo, "color", color );
    assign( jo, "volume", volume, strict, 0_ml );
    assign( jo, "weight", weight, strict, 0_gram );

    optional( jo, was_loaded, "phase", phase, make_flag_reader( gen.phase_map, "phase id" ), SOLID );

    assign( jo, "diff", difficulty_base, strict, 0 );
    assign( jo, "hp", hp, strict, 1 );
    assign( jo, "speed", speed, strict, 0 );
    assign( jo, "aggression", agro, strict, -100, 100 );
    assign( jo, "morale", morale, strict );

    assign( jo, "attack_cost", attack_cost, strict, 0 );
    assign( jo, "melee_skill", melee_skill, strict, 0 );
    assign( jo, "melee_dice", melee_dice, strict, 0 );
    assign( jo, "melee_dice_sides", melee_sides, strict, 0 );

    assign( jo, "grab_strength", grab_strength, strict, 0 );

    assign( jo, "dodge", sk_dodge, strict, 0 );
    assign( jo, "armor_bash", armor_bash, strict, 0 );
    assign( jo, "armor_cut", armor_cut, strict, 0 );
    assign( jo, "armor_stab", armor_stab, strict, 0 );
    assign( jo, "armor_acid", armor_acid, strict, 0 );
    assign( jo, "armor_fire", armor_fire, strict, 0 );

    assign( jo, "vision_day", vision_day, strict, 0 );
    assign( jo, "vision_night", vision_night, strict, 0 );

    optional( jo, was_loaded, "starting_ammo", starting_ammo );
    optional( jo, was_loaded, "luminance", luminance, 0 );
    optional( jo, was_loaded, "revert_to_itype", revert_to_itype, "" );
    optional( jo, was_loaded, "attack_effs", atk_effs, mon_attack_effect_reader{} );
    optional( jo, was_loaded, "mech_weapon", mech_weapon, "" );
    optional( jo, was_loaded, "mech_str_bonus", mech_str_bonus, 0 );
    optional( jo, was_loaded, "mech_battery", mech_battery, "" );

    // TODO: make this work with `was_loaded`
    if( jo.has_array( "melee_damage" ) ) {
        JsonArray arr = jo.get_array( "melee_damage" );
        melee_damage = load_damage_instance( arr );
    } else if( jo.has_object( "melee_damage" ) ) {
        melee_damage = load_damage_instance( jo );
    }

    int bonus_cut = 0;
    if( jo.has_int( "melee_cut" ) ) {
        bonus_cut = jo.get_int( "melee_cut" );
        melee_damage.add_damage( DT_CUT, bonus_cut );
    }

    if( jo.has_member( "death_drops" ) ) {
        JsonIn &stream = *jo.get_raw( "death_drops" );
        death_drops = item_group::load_item_group( stream, "distribution" );
    }

    assign( jo, "harvest", harvest );

    const auto death_reader = make_flag_reader( gen.death_map, "monster death function" );
    optional( jo, was_loaded, "death_function", dies, death_reader );
    if( dies.empty() ) {
        // TODO: really needed? Is an empty `dies` container not allowed?
        dies.push_back( mdeath::normal );
    }

    assign( jo, "emit_fields", emit_fields );

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
            auto tmp = jo.get_object( "extend" );
            add_special_attacks( tmp, "special_attacks", src );
        }
        if( jo.has_object( "delete" ) ) {
            auto tmp = jo.get_object( "delete" );
            remove_special_attacks( tmp, "special_attacks", src );
        }
    }

    // Disable upgrading when JSON contains `"upgrades": false`, but fallback to the
    // normal behavior (including error checking) if "upgrades" is not boolean or not `false`.
    if( jo.has_bool( "upgrades" ) && !jo.get_bool( "upgrades" ) ) {
        upgrade_group = mongroup_id::NULL_ID();
        upgrade_into = mtype_id::NULL_ID();
        upgrades = false;
    } else if( jo.has_member( "upgrades" ) ) {
        JsonObject up = jo.get_object( "upgrades" );
        optional( up, was_loaded, "half_life", half_life, -1 );
        optional( up, was_loaded, "age_grow", age_grow, -1 );
        optional( up, was_loaded, "into_group", upgrade_group, auto_flags_reader<mongroup_id> {},
                  mongroup_id::NULL_ID() );
        optional( up, was_loaded, "into", upgrade_into, auto_flags_reader<mtype_id> {},
                  mtype_id::NULL_ID() );
        upgrades = true;
    }

    //Reproduction
    if( jo.has_member( "reproduction" ) ) {
        JsonObject repro = jo.get_object( "reproduction" );
        optional( repro, was_loaded, "baby_count", baby_count, -1 );
        optional( repro, was_loaded, "baby_timer", baby_timer, -1 );
        optional( repro, was_loaded, "baby_monster", baby_monster, auto_flags_reader<mtype_id> {},
                  mtype_id::NULL_ID() );
        optional( repro, was_loaded, "baby_egg", baby_egg, auto_flags_reader<itype_id> {},
                  "null" );
        reproduces = true;
    }

    if( jo.has_member( "baby_flags" ) ) {
        // Because this determines mating season and some monsters have a mating season but not in-game offspring, declare this separately
        baby_flags.clear();
        JsonArray baby_tags = jo.get_array( "baby_flags" );
        while( baby_tags.has_more() ) {
            baby_flags.push_back( baby_tags.next_string() );
        }
    }

    if( jo.has_member( "biosignature" ) ) {
        JsonObject biosig = jo.get_object( "biosignature" );
        optional( biosig, was_loaded, "biosig_timer", biosig_timer, -1 );
        optional( biosig, was_loaded, "biosig_item", biosig_item, auto_flags_reader<itype_id> {},
                  "null" );
        biosignatures = true;
    }

    optional( jo, was_loaded, "burn_into", burn_into, auto_flags_reader<mtype_id> {},
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
        auto jop = jo.get_object( "path_settings" );
        // Here rather than in pathfinding.cpp because we want monster-specific defaults and was_loaded
        optional( jop, was_loaded, "max_dist", path_settings.max_dist, 0 );
        optional( jop, was_loaded, "max_length", path_settings.max_length, -1 );
        optional( jop, was_loaded, "bash_strength", path_settings.bash_strength, -1 );
        optional( jop, was_loaded, "allow_open_doors", path_settings.allow_open_doors, false );
        optional( jop, was_loaded, "avoid_traps", path_settings.avoid_traps, false );
        optional( jop, was_loaded, "allow_climb_stairs", path_settings.allow_climb_stairs, true );
    }
    difficulty = ( melee_skill + 1 ) * melee_dice * ( bonus_cut + melee_sides ) * 0.04 +
                 ( sk_dodge + 1 ) * ( 3 + armor_bash + armor_cut ) * 0.04 +
                 ( difficulty_base + special_attacks.size() + 8 * emit_fields.size() );
    difficulty *= ( hp + speed - attack_cost + ( morale + agro ) * 0.1 ) * 0.01 +
                  ( vision_day + 2 * vision_night ) * 0.01;
}

void MonsterGenerator::load_species( JsonObject &jo, const std::string &src )
{
    mon_species->load( jo, src );
}

void species_type::load( JsonObject &jo, const std::string & )
{
    optional( jo, was_loaded, "footsteps", footsteps, "footsteps." );
    footsteps = _( footsteps );
    const auto flag_reader = enum_flags_reader<m_flag> { "monster flag" };
    optional( jo, was_loaded, "flags", flags, flag_reader );

    const auto trigger_reader = enum_flags_reader<mon_trigger> { "monster trigger" };
    optional( jo, was_loaded, "anger_triggers", anger, trigger_reader );
    optional( jo, was_loaded, "placate_triggers", placate, trigger_reader );
    optional( jo, was_loaded, "fear_triggers", fear, trigger_reader );
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
        mattack_actor *clone() const override {
            return new mattack_hardcoded_wrapper( *this );
        }

        void load_internal( JsonObject &, const std::string & ) override {}
};

mtype_special_attack::mtype_special_attack( const mattack_id &id, const mon_action_attack f )
    : mtype_special_attack( new mattack_hardcoded_wrapper( id, f ) ) {}

void MonsterGenerator::add_hardcoded_attack( const std::string &type, const mon_action_attack f )
{
    add_attack( mtype_special_attack( type, f ) );
}

void MonsterGenerator::add_attack( mattack_actor *ptr )
{
    add_attack( mtype_special_attack( ptr ) );
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

mtype_special_attack MonsterGenerator::create_actor( JsonObject obj, const std::string &src ) const
{
    // Legacy support: tolerate attack types being specified as the type
    const std::string type = obj.get_string( "type", "monster_attack" );
    const std::string attack_type = obj.get_string( "attack_type", type );

    if( type != "monster_attack" && attack_type != type ) {
        obj.throw_error( "Specifying \"attack_type\" is only allowed when \"type\" is \"monster_attack\" or not specified",
                         "type" );
    }

    mattack_actor *new_attack = nullptr;
    if( attack_type == "monster_attack" ) {
        const std::string id = obj.get_string( "id" );
        const auto &iter = attack_map.find( id );
        if( iter == attack_map.end() ) {
            obj.throw_error( "Monster attacks must specify type and/or id", "type" );
        }

        new_attack = iter->second->clone();
    } else if( attack_type == "leap" ) {
        new_attack = new leap_actor();
    } else if( attack_type == "melee" ) {
        new_attack = new melee_actor();
    } else if( attack_type == "bite" ) {
        new_attack = new bite_actor();
    } else if( attack_type == "gun" ) {
        new_attack = new gun_actor();
    } else if( attack_type == "spell" ) {
        new_attack = new mon_spellcasting_actor();
    } else {
        obj.throw_error( "unknown monster attack", "attack_type" );
    }

    new_attack->load( obj, src );
    return mtype_special_attack( new_attack );
}

void mattack_actor::load( JsonObject &jo, const std::string &src )
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

void MonsterGenerator::load_monster_attack( JsonObject &jo, const std::string &src )
{
    add_attack( create_actor( jo, src ) );
}

void mtype::add_special_attack( JsonObject obj, const std::string &src )
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

void mtype::add_special_attack( JsonArray inner, const std::string & )
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
    auto new_attack = mtype_special_attack( iter->second );
    new_attack.actor->cooldown = inner.get_int( 1 );
    special_attacks.emplace( name, new_attack );
    special_attacks_names.push_back( name );
}

void mtype::add_special_attacks( JsonObject &jo, const std::string &member,
                                 const std::string &src )
{

    if( !jo.has_array( member ) ) {
        return;
    }

    JsonArray outer = jo.get_array( member );
    while( outer.has_more() ) {
        if( outer.test_array() ) {
            add_special_attack( outer.next_array(), src );
        } else if( outer.test_object() ) {
            add_special_attack( outer.next_object(), src );
        } else {
            outer.throw_error( "array element is neither array nor object." );
        }
    }
}

void mtype::remove_special_attacks( JsonObject &jo, const std::string &member_name,
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

void MonsterGenerator::check_monster_definitions() const
{
    for( const auto &mon : mon_templates->get_all() ) {
        if( mon.harvest == "null" && !mon.has_flag( MF_ELECTRONIC ) && mon.id != mtype_id( "mon_null" ) ) {
            debugmsg( "monster %s has no harvest entry", mon.id.c_str(), mon.harvest.c_str() );
        }
        for( auto &spec : mon.species ) {
            if( !spec.is_valid() ) {
                debugmsg( "monster %s has invalid species %s", mon.id.c_str(), spec.c_str() );
            }
        }
        if( !mon.death_drops.empty() && !item_group::group_is_defined( mon.death_drops ) ) {
            debugmsg( "monster %s has unknown death drop item group: %s", mon.id.c_str(),
                      mon.death_drops.c_str() );
        }
        for( auto &m : mon.mat ) {
            if( m.str() == "null" || !m.is_valid() ) {
                debugmsg( "monster %s has unknown material: %s", mon.id.c_str(), m.c_str() );
            }
        }
        if( !mon.revert_to_itype.empty() && !item::type_is_defined( mon.revert_to_itype ) ) {
            debugmsg( "monster %s has unknown revert_to_itype: %s", mon.id.c_str(),
                      mon.revert_to_itype.c_str() );
        }
        if( !mon.mech_weapon.empty() && !item::type_is_defined( mon.mech_weapon ) ) {
            debugmsg( "monster %s has unknown mech_weapon: %s", mon.id.c_str(),
                      mon.mech_weapon.c_str() );
        }
        if( !mon.mech_battery.empty() && !item::type_is_defined( mon.mech_battery ) ) {
            debugmsg( "monster %s has unknown mech_battery: %s", mon.id.c_str(),
                      mon.mech_battery.c_str() );
        }
        for( auto &s : mon.starting_ammo ) {
            if( !item::type_is_defined( s.first ) ) {
                debugmsg( "starting ammo %s of monster %s is unknown", s.first.c_str(), mon.id.c_str() );
            }
        }
        for( auto &e : mon.atk_effs ) {
            if( !e.id.is_valid() ) {
                debugmsg( "attack effect %s of monster %s is unknown", e.id.c_str(), mon.id.c_str() );
            }
        }

        for( const auto &e : mon.emit_fields ) {
            if( !e.is_valid() ) {
                debugmsg( "monster %s has invalid emit source %s", mon.id.c_str(), e.c_str() );
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
            if( mon.baby_timer < 1 ) {
                debugmsg( "Time between reproductions (%d) is invalid for %s",
                          mon.baby_timer, mon.id.c_str() );
            }
            if( mon.baby_count < 1 ) {
                debugmsg( "Number of children (%d) is invalid for %s",
                          mon.baby_count, mon.id.c_str() );
            }
            if( !mon.baby_monster && mon.baby_egg == "null" ) {
                debugmsg( "No baby or egg defined for monster %s", mon.id.c_str() );
            }
            if( mon.baby_monster && mon.baby_egg != "null" ) {
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
            if( mon.biosig_timer < 1 ) {
                debugmsg( "Time between biosignature drops (%d) is invalid for %s",
                          mon.biosig_timer, mon.id.c_str() );
            }
            if( mon.biosig_item == "null" ) {
                debugmsg( "No biosignature drop defined for monster %s", mon.id.c_str() );
            }
            if( !item::type_is_defined( mon.biosig_item ) ) {
                debugmsg( "item_id %s of monster %s is not a valid item id",
                          mon.biosig_item.c_str(), mon.id.c_str() );
            }
        }
    }
}
