#include "monstergenerator.h"

#include "catacharset.h"
#include "color.h"
#include "creature.h"
#include "debug.h"
#include "generic_factory.h"
#include "item.h"
#include "item_group.h"
#include "mattack_actors.h"
#include "monattack.h"
#include "mondeath.h"
#include "mondefense.h"
#include "monfaction.h"
#include "mtype.h"
#include "output.h"
#include "rng.h"
#include "translations.h"
#include "material.h"

#include <algorithm>

const mtype_id mon_generator( "mon_generator" );

template<>
const mtype_id string_id<mtype>::NULL_ID( "mon_null" );

template<>
const mtype& string_id<mtype>::obj() const
{
    return MonsterGenerator::generator().mon_templates->obj( *this );
}

template<>
bool string_id<mtype>::is_valid() const
{
    return MonsterGenerator::generator().mon_templates->is_valid( *this );
}

template<>
const species_id string_id<species_type>::NULL_ID( "spec_null" );

template<>
const species_type& string_id<species_type>::obj() const
{
    return MonsterGenerator::generator().mon_species->obj( *this );
}

template<>
bool string_id<species_type>::is_valid() const
{
    return MonsterGenerator::generator().mon_species->is_valid( *this );
}

MonsterGenerator::MonsterGenerator()
: mon_templates( new generic_factory<mtype>( "monster type", "id", "alias" ) )
, mon_species( new generic_factory<species_type>( "species" ) )
{
    mon_templates->insert( mtype() );
    mon_species->insert( species_type() );
    //ctor
    init_phases();
    init_attack();
    init_defense();
    init_death();
    init_flags();
    init_trigger();
}

MonsterGenerator::~MonsterGenerator()
{
    reset();
}

void MonsterGenerator::reset()
{
    mon_templates->reset();
    mon_templates->insert( mtype() );

    mon_species->reset();
    mon_species->insert( species_type() );
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

void MonsterGenerator::finalize_mtypes()
{
    for( const auto &elem : mon_templates->get_all() ) {
        mtype &mon = const_cast<mtype&>( elem );
        apply_species_attributes( mon );
        set_mtype_flags( mon );
        set_species_ids( mon );

        if( mon.bash_skill < 0 ) {
            mon.bash_skill = calc_bash_skill( mon );
        }

        finalize_pathfinding_settings( mon );
    }
}

void MonsterGenerator::apply_species_attributes( mtype &mon )
{
    for( const auto &spec : mon.species ) {
        if( !spec.is_valid() ) {
            continue;
        }
        const species_type &mspec = spec.obj();

        apply_set_to_set( mspec.flags, mon.flags );
        apply_set_to_set( mspec.anger_trig, mon.anger );
        apply_set_to_set( mspec.fear_trig, mon.fear );
        apply_set_to_set( mspec.placate_trig, mon.placate );
    }
}

void MonsterGenerator::set_mtype_flags( mtype &mon )
{
    // The flag vectors are slow, given how often has_flags() is called,
    // so instead we'll use bitsets and initialize them here.
    m_flag nflag;
    for (std::set<m_flag>::iterator flag = mon.flags.begin(); flag != mon.flags.end(); ++flag) {
        nflag = m_flag(*flag);
        mon.bitflags[nflag] = true;
    }
    monster_trigger ntrig;
    for (std::set<monster_trigger>::iterator trig = mon.anger.begin(); trig != mon.anger.end();
         ++trig) {
        ntrig = monster_trigger(*trig);
        mon.bitanger[ntrig] = true;
    }
    for (std::set<monster_trigger>::iterator trig = mon.fear.begin(); trig != mon.fear.end();
         ++trig) {
        ntrig = monster_trigger(*trig);
        mon.bitfear[ntrig] = true;
    }
    for (std::set<monster_trigger>::iterator trig = mon.placate.begin(); trig != mon.placate.end();
         ++trig) {
        ntrig = monster_trigger(*trig);
        mon.bitplacate[ntrig] = true;
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
}

template <typename T>
void MonsterGenerator::apply_set_to_set(std::set<T> from, std::set<T> &to)
{
    for( const auto &elem : from ) {
        to.insert( elem );
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
    death_map["BROKEN_AMMO"] = &mdeath::broken_ammo;// Gives a message about destroying ammo and then calls "BROKEN"
    death_map["SMOKEBURST"] = &mdeath::smokeburst;// Explode like a huge smoke bomb.
    death_map["JABBERWOCKY"] = &mdeath::jabberwock; // Snicker-snack!
    death_map["DETONATE"] = &mdeath::detonate; // Take them with you
    death_map["GAMEOVER"] = &mdeath::gameover;// Game over!  Defense mode
    death_map["PREG_ROACH"] = &mdeath::preg_roach;// Spawn some cockroach nymphs

    /* Currently Unimplemented */
    //death_map["SHRIEK"] = &mdeath::shriek;// Screams loudly
    //death_map["HOWL"] = &mdeath::howl;// Wolf's howling
    //death_map["RATTLE"] = &mdeath::rattle;// Rattles like a rattlesnake
}

void MonsterGenerator::init_attack()
{
    attack_map["NONE"] = &mattack::none;
    attack_map["ANTQUEEN"] = &mattack::antqueen;
    attack_map["SHRIEK"] = &mattack::shriek;
    attack_map["SHRIEK_ALERT"] = &mattack::shriek_alert;
    attack_map["SHRIEK_STUN"] = &mattack::shriek_stun;
    attack_map["RATTLE"] = &mattack::rattle;
    attack_map["HOWL"] = &mattack::howl;
    attack_map["ACID"] = &mattack::acid;
    attack_map["ACID_BARF"] = &mattack::acid_barf;
    attack_map["ACID_ACCURATE"] = &mattack::acid_accurate;
    attack_map["SHOCKSTORM"] = &mattack::shockstorm;
    attack_map["PULL_METAL_WEAPON"] = &mattack::pull_metal_weapon;
    attack_map["BOOMER"] = &mattack::boomer;
    attack_map["BOOMER_GLOW"] = &mattack::boomer_glow;
    attack_map["RESURRECT"] = &mattack::resurrect;
    attack_map["SMASH"] = &mattack::smash;
    attack_map["SCIENCE"] = &mattack::science;
    attack_map["GROWPLANTS"] = &mattack::growplants;
    attack_map["GROW_VINE"] = &mattack::grow_vine;
    attack_map["VINE"] = &mattack::vine;
    attack_map["SPIT_SAP"] = &mattack::spit_sap;
    attack_map["TRIFFID_HEARTBEAT"] = &mattack::triffid_heartbeat;
    attack_map["FUNGUS"] = &mattack::fungus;
    attack_map["FUNGUS_HAZE"] = &mattack::fungus_haze;
    attack_map["FUNGUS_BIG_BLOSSOM"] = &mattack::fungus_big_blossom;
    attack_map["FUNGUS_INJECT"] = &mattack::fungus_inject;
    attack_map["FUNGUS_BRISTLE"] = &mattack::fungus_bristle;
    attack_map["FUNGUS_GROWTH"] = &mattack::fungus_growth;
    attack_map["FUNGUS_SPROUT"] = &mattack::fungus_sprout;
    attack_map["FUNGUS_FORTIFY"] = &mattack::fungus_fortify;
    attack_map["DERMATIK"] = &mattack::dermatik;
    attack_map["DERMATIK_GROWTH"] = &mattack::dermatik_growth;
    attack_map["PLANT"] = &mattack::plant;
    attack_map["DISAPPEAR"] = &mattack::disappear;
    attack_map["FORMBLOB"] = &mattack::formblob;
    attack_map["CALLBLOBS"] = &mattack::callblobs;
    attack_map["JACKSON"] = &mattack::jackson;
    attack_map["DANCE"] = &mattack::dance;
    attack_map["DOGTHING"] = &mattack::dogthing;
    attack_map["TENTACLE"] = &mattack::tentacle;
    attack_map["GENE_STING"] = &mattack::gene_sting;
    attack_map["PARA_STING"] = &mattack::para_sting;
    attack_map["TRIFFID_GROWTH"] = &mattack::triffid_growth;
    attack_map["STARE"] = &mattack::stare;
    attack_map["FEAR_PARALYZE"] = &mattack::fear_paralyze;
    attack_map["PHOTOGRAPH"] = &mattack::photograph;
    attack_map["TAZER"] = &mattack::tazer;
    attack_map["SEARCHLIGHT"] = &mattack::searchlight;
    attack_map["FLAMETHROWER"] = &mattack::flamethrower;
    attack_map["COPBOT"] = &mattack::copbot;
    attack_map["CHICKENBOT"] = &mattack::chickenbot;
    attack_map["MULTI_ROBOT"] = &mattack::multi_robot;
    attack_map["RATKING"] = &mattack::ratking;
    attack_map["GENERATOR"] = &mattack::generator;
    attack_map["UPGRADE"] = &mattack::upgrade;
    attack_map["BREATHE"] = &mattack::breathe;
    attack_map["IMPALE"] = &mattack::impale;
    attack_map["BRANDISH"] = &mattack::brandish;
    attack_map["FLESH_GOLEM"] = &mattack::flesh_golem;
    attack_map["LUNGE"] = &mattack::lunge;
    attack_map["LONGSWIPE"] = &mattack::longswipe;
    attack_map["PARROT"] = &mattack::parrot;
    attack_map["DARKMAN"] = &mattack::darkman;
    attack_map["SLIMESPRING"] = &mattack::slimespring;
    attack_map["BIO_OP_TAKEDOWN"] = &mattack::bio_op_takedown;
    attack_map["SUICIDE"] = &mattack::suicide;
    attack_map["KAMIKAZE"] = &mattack::kamikaze;
    attack_map["GRENADIER"] = &mattack::grenadier;
    attack_map["GRENADIER_ELITE"] = &mattack::grenadier_elite;
    attack_map["RIOTBOT"] = &mattack::riotbot;
    attack_map["STRETCH_ATTACK"] = &mattack::stretch_attack;
    attack_map["STRETCH_BITE"] = &mattack::stretch_bite;
    attack_map["RANGED_PULL"] = &mattack::ranged_pull;
    attack_map["GRAB"] = &mattack::grab;
    attack_map["GRAB_DRAG"] = &mattack::grab_drag;
}

void MonsterGenerator::init_defense()
{
    defense_map["NONE"] = &mdefense::none; //No special attack-back
    defense_map["ZAPBACK"] = &mdefense::zapback; //Shock attacker on hit
    defense_map["ACIDSPLASH"] = &mdefense::acidsplash; //Splash acid on the attacker
}

void MonsterGenerator::init_trigger()
{
    trigger_map["NULL"] = MTRIG_NULL;// = 0,
    trigger_map["STALK"] = MTRIG_STALK;//  // Increases when following the player
    trigger_map["MEAT"] = MTRIG_MEAT;//  // Meat or a corpse nearby
    trigger_map["PLAYER_WEAK"] = MTRIG_HOSTILE_WEAK;// // Hurt hostile player/npc/monster seen
    trigger_map["PLAYER_CLOSE"] = MTRIG_HOSTILE_CLOSE;// // Hostile creature within a few tiles
    trigger_map["HURT"] = MTRIG_HURT;//  // We are hurt
    trigger_map["FIRE"] = MTRIG_FIRE;//  // Fire nearby
    trigger_map["FRIEND_DIED"] = MTRIG_FRIEND_DIED;// // A monster of the same type died
    trigger_map["FRIEND_ATTACKED"] = MTRIG_FRIEND_ATTACKED;// // A monster of the same type attacked
    trigger_map["SOUND"] = MTRIG_SOUND;//  // Heard a sound
}

void MonsterGenerator::init_flags()
{
    // see mtype.h for commentary
    flag_map["NULL"] = MF_NULL;
    flag_map["SEES"] = MF_SEES;
    flag_map["HEARS"] = MF_HEARS;
    flag_map["GOODHEARING"] = MF_GOODHEARING;
    flag_map["SMELLS"] = MF_SMELLS;
    flag_map["KEENNOSE"] = MF_KEENNOSE;
    flag_map["STUMBLES"] = MF_STUMBLES;
    flag_map["WARM"] = MF_WARM;
    flag_map["NOHEAD"] = MF_NOHEAD;
    flag_map["HARDTOSHOOT"] = MF_HARDTOSHOOT;
    flag_map["GRABS"] = MF_GRABS;
    flag_map["BASHES"] = MF_BASHES;
    flag_map["GROUP_BASH"] = MF_GROUP_BASH;
    flag_map["DESTROYS"] = MF_DESTROYS;
    flag_map["BORES"] = MF_BORES;
    flag_map["POISON"] = MF_POISON;
    flag_map["VENOM"] = MF_VENOM;
    flag_map["BADVENOM"] = MF_BADVENOM;
    flag_map["PARALYZEVENOM"] = MF_PARALYZE;
    flag_map["BLEED"] = MF_BLEED;
    flag_map["WEBWALK"] = MF_WEBWALK;
    flag_map["DIGS"] = MF_DIGS;
    flag_map["CAN_DIG"] = MF_CAN_DIG;
    flag_map["FLIES"] = MF_FLIES;
    flag_map["AQUATIC"] = MF_AQUATIC;
    flag_map["SWIMS"] = MF_SWIMS;
    flag_map["FISHABLE"] = MF_FISHABLE;
    flag_map["ATTACKMON"] = MF_ATTACKMON;
    flag_map["ANIMAL"] = MF_ANIMAL;
    flag_map["PLASTIC"] = MF_PLASTIC;
    flag_map["SUNDEATH"] = MF_SUNDEATH;
    flag_map["ELECTRIC"] = MF_ELECTRIC;
    flag_map["ACIDPROOF"] = MF_ACIDPROOF;
    flag_map["ACIDTRAIL"] = MF_ACIDTRAIL;
    flag_map["FIREPROOF"] = MF_FIREPROOF;
    flag_map["SLUDGEPROOF"] = MF_SLUDGEPROOF;
    flag_map["SLUDGETRAIL"] = MF_SLUDGETRAIL;
    flag_map["FIREY"] = MF_FIREY;
    flag_map["QUEEN"] = MF_QUEEN;
    flag_map["ELECTRONIC"] = MF_ELECTRONIC;
    flag_map["FUR"] = MF_FUR;
    flag_map["LEATHER"] = MF_LEATHER;
    flag_map["WOOL"] = MF_WOOL;
    flag_map["FEATHER"] = MF_FEATHER;
    flag_map["CBM_CIV"] = MF_CBM_CIV;
    flag_map["BONES"] = MF_BONES;
    flag_map["FAT"] = MF_FAT;
    flag_map["IMMOBILE"] = MF_IMMOBILE;
    flag_map["HIT_AND_RUN"] = MF_HIT_AND_RUN;
    flag_map["GUILT"] = MF_GUILT;
    flag_map["HUMAN"] = MF_HUMAN;
    flag_map["NO_BREATHE"] = MF_NO_BREATHE;
    flag_map["REGENERATES_50"] = MF_REGENERATES_50;
    flag_map["REGENERATES_10"] = MF_REGENERATES_10;
    flag_map["FLAMMABLE"] = MF_FLAMMABLE;
    flag_map["REVIVES"] = MF_REVIVES;
    flag_map["CHITIN"] = MF_CHITIN;
    flag_map["VERMIN"] = MF_VERMIN;
    flag_map["NOGIB"] = MF_NOGIB;
    flag_map["ABSORBS"] = MF_ABSORBS;
    flag_map["LARVA"] = MF_LARVA;
    flag_map["ARTHROPOD_BLOOD"] = MF_ARTHROPOD_BLOOD;
    flag_map["ACID_BLOOD"] = MF_ACID_BLOOD;
    flag_map["BILE_BLOOD"] = MF_BILE_BLOOD;
    flag_map["REGEN_MORALE"] = MF_REGENMORALE;
    flag_map["CBM_POWER"] = MF_CBM_POWER;
    flag_map["CBM_SCI"] = MF_CBM_SCI;
    flag_map["CBM_OP"] = MF_CBM_OP;
    flag_map["CBM_TECH"] = MF_CBM_TECH;
    flag_map["CBM_SUBS"] = MF_CBM_SUBS;
    flag_map["SWARMS"] = MF_SWARMS;
    flag_map["CLIMBS"] = MF_CLIMBS;
    flag_map["GROUP_MORALE"] = MF_GROUP_MORALE;
    flag_map["INTERIOR_AMMO"] = MF_INTERIOR_AMMO;
    flag_map["NIGHT_INVISIBILITY"] = MF_NIGHT_INVISIBILITY;
    flag_map["REVIVES_HEALTHY"] = MF_REVIVES_HEALTHY;
    flag_map["NO_NECRO"] = MF_NO_NECRO;
    flag_map["PUSH_MON"] = MF_PUSH_MON;
    flag_map["PATH_AVOID_DANGER_1"] = MF_AVOID_DANGER_1;
    flag_map["PATH_AVOID_DANGER_2"] = MF_AVOID_DANGER_2;
    flag_map["PRIORITIZE_TARGETS"] = MF_PRIORITIZE_TARGETS;
}

void MonsterGenerator::set_species_ids( mtype &mon )
{
    for( const auto &s : mon.species ) {
        if( s.is_valid() ) {
            mon.species_ptrs.insert( &s.obj() );
        } else {
            debugmsg( "Tried to assign species %s to monster %s, but no entry for the species exists", s.c_str(), mon.id.c_str() );
        }
    }
}

void MonsterGenerator::load_monster(JsonObject &jo)
{
    mon_templates->load( jo );
}

class mon_attack_effect_reader : public generic_typed_reader<mon_attack_effect_reader> {
    public:
        mon_effect_data get_next( JsonIn &jin ) const {
            JsonObject e = jin.get_object();
            return mon_effect_data( efftype_id( e.get_string( "id" ) ), e.get_int( "duration", 0 ),
                                    get_body_part_token( e.get_string( "bp", "NUM_BP" ) ), e.get_bool( "permanent", false ),
                                    e.get_int( "chance", 100 ) );
        }
        template<typename C>
        void erase_next( JsonIn &jin, C &container ) const {
            const efftype_id id = efftype_id( jin.get_string() );
            reader_detail::handler<C>().erase_if( container, [&id]( const mon_effect_data &e ) { return e.id == id; } );
        }
};

void mtype::load( JsonObject &jo )
{
    MonsterGenerator &gen = MonsterGenerator::generator();

    // Name and name plural are not translated here, but when needed in
    // combination with the actual count in `mtype::nname`.
    mandatory( jo, was_loaded, "name", name );
    // default behaviour: Assume the regular plural form (appending an “s”)
    optional( jo, was_loaded, "name_plural", name_plural, name + "s" );
    mandatory( jo, was_loaded, "description", description, translated_string_reader );

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

    mandatory( jo, was_loaded, "color", color, color_reader{} );
    const typed_flag_reader<decltype( Creature::size_map )> size_reader{ Creature::size_map, "invalid creature size" };
    optional( jo, was_loaded, "size", size, size_reader, MS_MEDIUM );
    const typed_flag_reader<decltype( gen.phase_map )> phase_reader{ gen.phase_map, "invalid phase id" };
    optional( jo, was_loaded, "phase", phase, phase_reader, SOLID );

    optional( jo, was_loaded, "diff", difficulty, 0 );
    optional( jo, was_loaded, "aggression", agro, 0 );
    optional( jo, was_loaded, "morale", morale, 0 );
    optional( jo, was_loaded, "speed", speed, 0 );
    optional( jo, was_loaded, "attack_cost", attack_cost, 100 );
    optional( jo, was_loaded, "melee_skill", melee_skill, 0 );
    optional( jo, was_loaded, "melee_dice", melee_dice, 0 );
    optional( jo, was_loaded, "melee_dice_sides", melee_sides, 0 );
    optional( jo, was_loaded, "dodge", sk_dodge, 0 );
    optional( jo, was_loaded, "armor_bash", armor_bash, 0 );
    optional( jo, was_loaded, "armor_cut", armor_cut, 0 );
    optional( jo, was_loaded, "armor_acid", armor_acid, armor_cut / 2 );
    optional( jo, was_loaded, "armor_fire", armor_fire, 0 );
    optional( jo, was_loaded, "hp", hp, 0 );
    optional( jo, was_loaded, "starting_ammo", starting_ammo );
    optional( jo, was_loaded, "luminance", luminance, 0 );
    optional( jo, was_loaded, "revert_to_itype", revert_to_itype, "" );
    optional( jo, was_loaded, "vision_day", vision_day, 40 );
    optional( jo, was_loaded, "vision_night", vision_night, 1 );
    optional( jo, was_loaded, "armor_stab", armor_stab, 0.8f * armor_cut );
    optional( jo, was_loaded, "attack_effs", atk_effs, mon_attack_effect_reader{} );

    // TODO: make this work with `was_loaded`
    if( jo.has_array( "melee_damage" ) ) {
        JsonArray arr = jo.get_array( "melee_damage" );
        melee_damage = load_damage_instance( arr );
    } else if( jo.has_object( "melee_damage" ) ) {
        melee_damage = load_damage_instance( jo );
    }

    if( jo.has_int( "melee_cut" ) ) {
        int bonus_cut = jo.get_int( "melee_cut" );
        melee_damage.add_damage( DT_CUT, bonus_cut );
    }

    if( jo.has_member( "death_drops" ) ) {
        JsonIn &stream = *jo.get_raw( "death_drops" );
        death_drops = item_group::load_item_group( stream, "distribution" );
    }

    const typed_flag_reader<decltype( gen.death_map )> death_reader{ gen.death_map, "invalid monster death function" };
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
        add_special_attacks( jo, "special_attacks" );
    } else {
        // Note: special_attacks left as is, new attacks are added to it!
        // Note: member name prefixes are compatible with those used by generic_typed_reader
        remove_special_attacks( jo, "remove:special_attacks" );
        add_special_attacks( jo, "add:special_attacks" );
    }

    // Disable upgrading when JSON contains `"upgrades": false`, but fallback to the
    // normal behavior (including error checking) if "upgrades" is not boolean or not `false`.
    if( jo.has_bool( "upgrades" ) && !jo.get_bool( "upgrades" ) ) {
        upgrade_group = mongroup_id::NULL_ID;
        upgrade_into = mtype_id::NULL_ID;
        upgrades = false;
    } else if( jo.has_member( "upgrades" ) ) {
        JsonObject up = jo.get_object( "upgrades" );
        optional( up, was_loaded, "half_life", half_life, -1 );
        optional( up, was_loaded, "into_group", upgrade_group, auto_flags_reader<mongroup_id> {}, mongroup_id::NULL_ID );
        optional( up, was_loaded, "into", upgrade_into, auto_flags_reader<mtype_id> {}, mtype_id::NULL_ID );
        upgrades = true;
    }

    optional( jo, was_loaded, "burn_into", burn_into, auto_flags_reader<mtype_id> {}, mtype_id::NULL_ID );

    const typed_flag_reader<decltype( gen.flag_map )> flag_reader{ gen.flag_map, "invalid monster flag" };
    optional( jo, was_loaded, "flags", flags, flag_reader );
    // Can't calculate yet - we want all flags first
    optional( jo, was_loaded, "bash_skill", bash_skill, -1 );

    const typed_flag_reader<decltype( gen.trigger_map )> trigger_reader{ gen.trigger_map, "invalid monster trigger" };
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
}

void MonsterGenerator::load_species(JsonObject &jo)
{
    mon_species->load( jo );
}

void species_type::load( JsonObject &jo )
{
    MonsterGenerator &gen = MonsterGenerator::generator();

    const typed_flag_reader<decltype( gen.flag_map )> flag_reader{ gen.flag_map, "invalid monster flag" };
    optional( jo, was_loaded, "flags", flags, flag_reader );

    const typed_flag_reader<decltype( gen.trigger_map )> trigger_reader{ gen.trigger_map, "invalid monster trigger" };
    optional( jo, was_loaded, "anger_triggers", anger_trig, trigger_reader );
    optional( jo, was_loaded, "placate_triggers", placate_trig, trigger_reader );
    optional( jo, was_loaded, "fear_triggers", fear_trig, trigger_reader );
}

const std::vector<mtype> &MonsterGenerator::get_all_mtypes() const
{
    return mon_templates->get_all();
}

mtype_id MonsterGenerator::get_valid_hallucination() const
{
    std::vector<mtype_id> potentials;
    for( const auto &mon : mon_templates->get_all() ) {
        if( mon.id != NULL_ID && mon.id != mon_generator ) {
            potentials.push_back( mon.id );
        }
    }

    return random_entry( potentials );
}

m_flag MonsterGenerator::m_flag_from_string( std::string flag ) const
{
    return flag_map.find( flag )->second;
}

template<typename mattack_actor_type>
mtype_special_attack load_actor( JsonObject obj, int cooldown )
{
    std::unique_ptr<mattack_actor_type> actor( new mattack_actor_type() );
    actor->load( obj );
    return mtype_special_attack( actor.release(), cooldown );
}

void mtype::add_special_attack( JsonObject obj )
{
    const std::string type = obj.get_string( "type" );
    const int cooldown = obj.get_int( "cooldown" );

    if( type == "leap" ) {
        special_attacks[type] = load_actor<leap_actor>( obj, cooldown );
    } else if( type == "bite" ) {
        special_attacks[type] = load_actor<bite_actor>( obj, cooldown );
    } else if( type == "gun" ) {
        special_attacks[type] = load_actor<gun_actor>( obj, cooldown );
    } else {
        obj.throw_error( "unknown monster attack", "type" );
        return;
    }

    special_attacks_names.push_back( type );
}

void mtype::add_special_attack( JsonArray inner )
{
    MonsterGenerator &gen = MonsterGenerator::generator();
    const std::string name = inner.get_string( 0 );
    const auto iter = gen.attack_map.find( name );
    if( iter == gen.attack_map.end() ) {
        inner.throw_error( "Invalid special_attacks" );
    }
    special_attacks[name] = mtype_special_attack( iter->second, inner.get_int( 1 ) );
    special_attacks_names.push_back( name );
}

void mtype::add_special_attacks( JsonObject &jo, const std::string &member ) {

    if( !jo.has_array( member ) ) {
        return;
    }

    JsonArray outer = jo.get_array(member);
    while( outer.has_more() ) {
        if( outer.test_array() ) {
            add_special_attack( outer.next_array() );
        } else if( outer.test_object() ) {
            add_special_attack( outer.next_object() );
        } else {
            outer.throw_error( "array element is neither array nor object." );
        }
    }
}

void mtype::remove_special_attacks( JsonObject &jo, const std::string &member_name )
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
        for( auto &spec : mon.species ) {
            if( !spec.is_valid() ) {
                debugmsg("monster %s has invalid species %s", mon.id.c_str(), spec.c_str());
            }
        }
        if (!mon.death_drops.empty() && !item_group::group_is_defined(mon.death_drops)) {
            debugmsg("monster %s has unknown death drop item group: %s", mon.id.c_str(),
                     mon.death_drops.c_str());
        }
        for( auto &m : mon.mat ) {
            if( m.str() == "null" || !m.is_valid() ) {
                debugmsg( "monster %s has unknown material: %s", mon.id.c_str(), m.c_str() );
            }
        }
        if( !mon.revert_to_itype.empty() && !item::type_is_defined( mon.revert_to_itype ) ) {
            debugmsg("monster %s has unknown revert_to_itype: %s", mon.id.c_str(),
                     mon.revert_to_itype.c_str());
        }
        for( auto & s : mon.starting_ammo ) {
            if( !item::type_is_defined( s.first ) ) {
                debugmsg( "starting ammo %s of monster %s is unknown", s.first.c_str(), mon.id.c_str() );
            }
        }
        for( auto & e : mon.atk_effs ) {
            if( !e.id.is_valid() ) {
                debugmsg( "attack effect %s of monster %s is unknown", e.id.c_str(), mon.id.c_str() );
            }
        }

        for( const auto& e : mon.emit_fields ) {
            if( !e.is_valid() ) {
                debugmsg( "monster %s has invalid emit source %s", mon.id.c_str(), e.c_str() );
            }
        }

        if( mon.upgrades ) {
            if( mon.half_life <= 0 ) {
                debugmsg( "half_life %d (<= 0) of monster %s is invalid", mon.half_life, mon.id.c_str() );
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
    }
}
