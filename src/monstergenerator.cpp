#include "mattack_common.h" // IWYU pragma: associated
#include "monstergenerator.h" // IWYU pragma: associated

#include <algorithm>
#include <optional>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>

#include "cached_options.h"
#include "calendar.h"
#include "cata_utility.h"
#include "color.h"
#include "condition.h"
#include "creature.h"
#include "damage.h"
#include "debug.h"
#include "enums.h"
#include "field_type.h"
#include "flexbuffer_json.h"
#include "game_constants.h"
#include "generic_factory.h"
#include "item.h"
#include "item_group.h"
#include "magic.h"
#include "mattack_actors.h"
#include "monattack.h"
#include "mondefense.h"
#include "mongroup.h"
#include "monster.h"
#include "mtype.h"
#include "options.h"
#include "pathfinding.h"
#include "rng.h"
#include "shearing.h"
#include "string_formatter.h"
#include "type_id.h"
#include "units.h"
#include "weakpoint.h"

struct itype;

static const harvest_id harvest_list_human( "human" );

static const material_id material_flesh( "flesh" );

static const speed_description_id speed_description_DEFAULT( "DEFAULT" );

static const spell_id spell_pseudo_dormant_trap_setup( "pseudo_dormant_trap_setup" );

namespace
{
generic_factory<mon_flag> mon_flags( "monster flags" );
} // namespace

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
        case mon_trigger::BRIGHT_LIGHT: return "BRIGHT_LIGHT";
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

/** @relates int_id */
template<>
bool int_id<mon_flag>::is_valid() const
{
    return mon_flags.is_valid( *this );
}

/** @relates int_id */
template<>
const mon_flag &int_id<mon_flag>::obj() const
{
    return mon_flags.obj( *this );
}

/** @relates int_id */
template<>
const string_id<mon_flag> &int_id<mon_flag>::id() const
{
    return mon_flags.convert( *this );
}

/** @relates int_id */
template<>
int_id<mon_flag> string_id<mon_flag>::id() const
{
    return mon_flags.convert( *this, int_id<mon_flag>( 0 ) );
}

/** @relates int_id */
template<>
int_id<mon_flag>::int_id( const string_id<mon_flag> &id ) : _id( id.id() )
{
}

/** @relates string_id */
template<>
const mon_flag &string_id<mon_flag>::obj() const
{
    return mon_flags.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<mon_flag>::is_valid() const
{
    return mon_flags.is_valid( *this );
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
    if( t.has_flag( mon_flag_BORES ) ) {
        ret *= 15;
    } else if( t.has_flag( mon_flag_DESTROYS ) ) {
        ret *= 2.5;
    } else if( !t.has_flag( mon_flag_BASHES ) ) {
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
    } else if( vol <= 108000_ml ) {
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
        mon.set_flag( mon_flag_id( flag ), flag_val );
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
    std::vector<mtype> extra_mtypes;
    for( const mtype &elem : mon_templates->get_all() ) {
        mtype &mon = const_cast<mtype &>( elem );

        if( !mon.default_faction.is_valid() ) {
            debugmsg( "Monster type '%s' has invalid default_faction: '%s'. "
                      "Add this faction to json as MONSTER_FACTION type.",
                      mon.id.str(), mon.default_faction.str() );
        }

        mon.flags.clear();
        for( const mon_flag_str_id &mf : mon.pre_flags_ ) {
            mon.flags.emplace( mf );
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

        finalize_damage_map( mon.armor.resist_vals, true );
        if( mon.armor_proportional.has_value() ) {
            finalize_damage_map( mon.armor_proportional->resist_vals, false, 1.f );
            for( std::pair<const damage_type_id, float> &dt : mon.armor.resist_vals ) {
                const auto iter = mon.armor_proportional->resist_vals.find( dt.first );
                if( iter != mon.armor_proportional->resist_vals.end() ) {
                    dt.second *= iter->second;
                }
            }
        }
        if( mon.armor_relative.has_value() ) {
            finalize_damage_map( mon.armor_relative->resist_vals, false, 0.f );
            for( std::pair<const damage_type_id, float> &dt : mon.armor.resist_vals ) {
                const auto iter = mon.armor_relative->resist_vals.find( dt.first );
                if( iter != mon.armor_relative->resist_vals.end() ) {
                    dt.second += iter->second;
                }
            }
        }

        float melee_dmg_total = mon.melee_damage.total_damage();
        float armor_diff = 3.0f;
        for( const auto &dt : mon.armor.resist_vals ) {
            if( dt.first->mon_difficulty ) {
                armor_diff += dt.second;
            }
        }
        std::unordered_set<std::string> blacklisted_specials{"PARROT", "PARROT_AT_DANGER", "GRAZE", "EAT_CROP", "EAT_FOOD", "EAT_CARRION"};
        int special_attacks_diff = 0;
        for( const auto &special : mon.special_attacks ) {
            if( !blacklisted_specials.count( special.first ) ) {
                special_attacks_diff++;
            }
        }
        mon.difficulty = ( mon.melee_skill + 1 ) * mon.melee_dice * ( melee_dmg_total + mon.melee_sides ) *
                         0.04 + ( mon.sk_dodge + 1 ) * armor_diff * 0.04 +
                         ( mon.difficulty_base + special_attacks_diff + 8 * mon.emit_fields.size() );
        mon.difficulty *= ( mon.hp + mon.speed - mon.attack_cost + ( mon.morale + mon.agro ) * 0.1 ) * 0.01
                          + ( mon.vision_day + 2 * mon.vision_night ) * 0.01;

        mon.difficulty = std::max( 1, mon.difficulty );

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

        mon.mdeath_effect.has_effect = mon.mdeath_effect.sp.is_valid();

        mon.weakpoints.clear();
        for( const weakpoints_id &wpset : mon.weakpoints_deferred ) {
            mon.weakpoints.add_from_set( wpset, true );
        }
        if( !mon.weakpoints_deferred_inline.weakpoint_list.empty() ) {
            mon.weakpoints_deferred_inline.finalize();
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
        mon.weakpoints.finalize();

        // check lastly to make extra fake monsters.
        if( mon.has_flag( mon_flag_GEN_DORMANT ) ) {
            extra_mtypes.push_back( generate_fake_pseudo_dormant_monster( mon ) );
        }
    }

    for( const mtype &mon : mon_templates->get_all() ) {
        if( !mon.has_flag( mon_flag_NOT_HALLUCINATION ) ) {
            hallucination_monsters.push_back( mon.id );
        }
    }

    // now add the fake monsters to the mon_templates
    for( mtype &mon : extra_mtypes ) {
        mon_templates->insert( mon );
    }
}

mtype MonsterGenerator::generate_fake_pseudo_dormant_monster( const mtype &mon )
{

    // this is what we are trying to build. Example for mon_zombie
    //{
    //    "id": "pseudo_dormant_mon_zombie",
    //        "type" : "MONSTER",
    //        "name" : { "str": "zombie" },
    //        "description" : "Fake zombie used for spawning dormant zombies.  If you see this, open an issue on github.",
    //        "copy-from" : "mon_zombie",
    //        "looks_like" : "corpse_mon_zombie",
    //        "hp" : 5,
    //        "speed" : 1,
    //        "flags" : ["FILTHY", "REVIVES", "DORMANT", "QUIETDEATH"] ,
    //        "zombify_into" : "mon_zombie",
    //        "special_attacks" : [
    //    {
    //        "id": "pseudo_dormant_trap_setup_attk",
    //            "type" : "spell",
    //            "spell_data" : { "id": "pseudo_dormant_trap_setup", "hit_self" : true },
    //            "cooldown" : 1,
    //            "allow_no_target" : true,
    //            "monster_message" : ""
    //    }
    //        ]
    //},
    mtype fake_mon = mtype( mon );
    fake_mon.id = mtype_id( "pseudo_dormant_" + mon.id.str() );
    // allowed (optional) flags: [ "FILTHY" ],
    // delete all others.
    // then add [ "DORMANT", "QUIETDEATH", "REVIVES", "REVIVES_HEALTHY" ]
    bool has_filthy = fake_mon.has_flag( mon_flag_FILTHY );
    fake_mon.flags.clear();
    fake_mon.flags.emplace( mon_flag_DORMANT );
    fake_mon.flags.emplace( mon_flag_QUIETDEATH );
    fake_mon.flags.emplace( mon_flag_REVIVES );
    fake_mon.flags.emplace( mon_flag_REVIVES_HEALTHY );
    if( has_filthy ) {
        fake_mon.flags.emplace( mon_flag_FILTHY );
    }

    // zombify into the original mon
    fake_mon.zombify_into = mon.id;
    // looks like "corpse" + original mon
    fake_mon.looks_like = "corpse_" + mon.id.str();
    // set the hp to 5
    fake_mon.hp = 5;
    // set the speed to 1
    fake_mon.speed = 1;
    // now ensure that monster will always die instantly. Nuke all resistances and dodges.
    // before this monsters like zombie predators could periodically resist the killing effect for a while.
    // clear dodge
    fake_mon.sk_dodge = 0;
    // clear regenerate
    fake_mon.regenerates = 0;
    fake_mon.regenerates_in_dark = false;
    // clear armor
    for( const auto &dam : fake_mon.armor.resist_vals ) {
        fake_mon.armor.resist_vals[dam.first] = 0;
    }
    // add the special attack.
    // first make a new mon_spellcasting_actor actor
    std::unique_ptr<mon_spellcasting_actor> new_actor( new mon_spellcasting_actor() );
    new_actor->allow_no_target = true;
    new_actor->cooldown = 1;
    new_actor->spell_data.id = spell_pseudo_dormant_trap_setup;
    new_actor->spell_data.self = true;

    // create the special attack now using the actor
    // use this constructor
    // explicit mtype_special_attack( std::unique_ptr<mattack_actor> f ) : actor( std::move( f ) ) { }

    std::unique_ptr<mattack_actor> base_actor = std::move( new_actor );
    mtype_special_attack new_attack( std::move( base_actor ) );

    std::pair<const std::string, mtype_special_attack> new_pair{
        std::string( "pseudo_dormant_trap_setup_attk" ),
        std::move( new_attack )
    };
    fake_mon.special_attacks.emplace( std::move( new_pair ) );
    fake_mon.special_attacks_names.emplace_back( "pseudo_dormant_trap_setup_attk" );

    return fake_mon;
}

void MonsterGenerator::apply_species_attributes( mtype &mon )
{
    for( const auto &spec : mon.species ) {
        if( !spec.is_valid() ) {
            continue;
        }
        const species_type &mspec = spec.obj();

        for( const mon_flag_str_id &f : mspec.flags ) {
            mon.flags.emplace( f );
            mon.pre_flags_.emplace( f );
        }
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

    if( mon.move_skills.climb.has_value() ) {
        mon.path_settings.climb_cost = move_skills_data::max_movemod_penalty -
                                       ( mon.move_skills.climb.value() * ( move_skills_data::max_movemod_penalty / 10 ) );
    } else if( mon.has_flag( mon_flag_CLIMBS ) ) {
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
    add_hardcoded_attack( "BROWSE", mattack::browse );
    add_hardcoded_attack( "EAT_CARRION", mattack::eat_carrion );
    add_hardcoded_attack( "EAT_CROP", mattack::eat_crop );
    add_hardcoded_attack( "EAT_FOOD", mattack::eat_food );
    add_hardcoded_attack( "GRAZE", mattack::graze );
    add_hardcoded_attack( "CHECK_UP", mattack::nurse_check_up );
    add_hardcoded_attack( "ASSIST", mattack::nurse_assist );
    add_hardcoded_attack( "OPERATE", mattack::nurse_operate );
    add_hardcoded_attack( "PAID_BOT", mattack::check_money_left );
    add_hardcoded_attack( "SHRIEK", mattack::shriek );
    add_hardcoded_attack( "SHRIEK_ALERT", mattack::shriek_alert );
    add_hardcoded_attack( "SHRIEK_STUN", mattack::shriek_stun );
    add_hardcoded_attack( "RATTLE", mattack::rattle );
    add_hardcoded_attack( "ACID", mattack::acid );
    add_hardcoded_attack( "ACID_BARF", mattack::acid_barf );
    add_hardcoded_attack( "SHOCKSTORM", mattack::shockstorm );
    add_hardcoded_attack( "SHOCKING_REVEAL", mattack::shocking_reveal );
    add_hardcoded_attack( "PULL_METAL_WEAPON", mattack::pull_metal_weapon );
    add_hardcoded_attack( "RESURRECT", mattack::resurrect );
    add_hardcoded_attack( "SMASH", mattack::smash );
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
    add_hardcoded_attack( "FUNGAL_TRAIL", mattack::fungal_trail );
    add_hardcoded_attack( "PLANT", mattack::plant );
    add_hardcoded_attack( "DISAPPEAR", mattack::disappear );
    add_hardcoded_attack( "DEPART", mattack::depart );
    add_hardcoded_attack( "FORMBLOB", mattack::formblob );
    add_hardcoded_attack( "CALLBLOBS", mattack::callblobs );
    add_hardcoded_attack( "JACKSON", mattack::jackson );
    add_hardcoded_attack( "DANCE", mattack::dance );
    add_hardcoded_attack( "DOGTHING", mattack::dogthing );
    add_hardcoded_attack( "PARA_STING", mattack::para_sting );
    add_hardcoded_attack( "TRIFFID_GROWTH", mattack::triffid_growth );
    add_hardcoded_attack( "PHOTOGRAPH", mattack::photograph );
    add_hardcoded_attack( "TAZER", mattack::tazer );
    add_hardcoded_attack( "SEARCHLIGHT", mattack::searchlight );
    add_hardcoded_attack( "SPEAKER", mattack::speaker );
    add_hardcoded_attack( "COPBOT", mattack::copbot );
    add_hardcoded_attack( "RATKING", mattack::ratking );
    add_hardcoded_attack( "GENERATOR", mattack::generator );
    add_hardcoded_attack( "UPGRADE", mattack::upgrade );
    add_hardcoded_attack( "BREATHE", mattack::breathe );
    add_hardcoded_attack( "BRANDISH", mattack::brandish );
    add_hardcoded_attack( "FLESH_GOLEM", mattack::flesh_golem );
    add_hardcoded_attack( "ABSORB_MEAT", mattack::absorb_meat );
    add_hardcoded_attack( "LUNGE", mattack::lunge );
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
    add_hardcoded_attack( "DOOT", mattack::doot );
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
    bp( bodypart_str_id::NULL_ID() ),
    duration( 1, 1 ),
    intensity( 0, 0 ) {}

void mon_effect_data::load( const JsonObject &jo )
{
    mandatory( jo, false, "id", id );
    optional( jo, false, "chance", chance, 100.f );
    optional( jo, false, "permanent", permanent, false );
    optional( jo, false, "affect_hit_bp", affect_hit_bp, false );
    optional( jo, false, "bp", bp, bodypart_str_id::NULL_ID() );
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

void mount_item_data::deserialize( const JsonObject &jo )
{
    optional( jo, false, "tied", tied, itype_id() );
    optional( jo, false, "tack", tack, itype_id() );
    optional( jo, false, "armor", armor, itype_id() );
    optional( jo, false, "storage", storage, itype_id() );
}

void revive_type::deserialize( const JsonObject &jo )
{
    // FIXME: reader for read_condition
    read_condition( jo, "condition", condition, true );
    if( jo.has_string( "monster" ) ) {
        mandatory( jo, false, "monster", revive_mon );
    } else {
        mandatory( jo, false, "monster_group", revive_monster_group );
    }
}

// this is a really gross reader - special_attacks and special_attacks_names really need to be one struct
// Then this gets a lot less gross and a lot more safe!
struct special_attacks_reader : generic_typed_reader<special_attacks_reader> {
    friend class MonsterGenerator;
    // For ordering purposes, we want to also grab the names when we load a special attack
    std::vector<std::string> &names;

    // special attacks load with src
    std::string_view src;
    mtype_id id;

    special_attacks_reader( std::vector<std::string> &name_vec, std::string_view _src,
                            mtype_id mtype ) : names( name_vec ), src( _src ), id( mtype ) {}

    void report_double_def( const std::string &name, const JsonValue &jv ) const {
        if( std::find( names.begin(), names.end(), name ) != names.end() ) {
            jv.throw_error(
                string_format( "%s specifies more than one attack of (sub)type %s, ignoring all but the last.  Add different `id`s to each attack of this type to prevent this.",
                               id.c_str(), name ) );
        }
    }

    std::pair<std::string, mtype_special_attack> get_next( const JsonValue &jv ) const {
        MonsterGenerator &gen = MonsterGenerator::generator();
        // only for delete
        if( jv.test_string() ) {
            std::string name = jv.get_string();
            auto it = std::find( names.begin(), names.end(), name );
            if( it == names.end() ) {
                jv.throw_error(
                    string_format( "Invalid special attack format or does not exist to delete for %s on %s",
                                   name, id.str() ) );
            }
            names.erase( it );
            return std::make_pair( name, mtype_special_attack( std::make_unique<invalid_mattack_actor>() ) );
        }
        if( jv.test_object() ) {
            mtype_special_attack new_attack = gen.create_actor( jv.get_object(), std::string( src ) );
            report_double_def( new_attack->id, jv );
            names.push_back( new_attack->id );
            return std::make_pair( new_attack->id, new_attack );
        }
        JsonArray inner = jv.get_array();
        std::string name = inner.get_string( 0 );
        const auto iter = gen.attack_map.find( name );
        if( iter == gen.attack_map.end() ) {
            inner.throw_error( "Invalid special_attacks" );
        }
        report_double_def( name, jv );

        mtype_special_attack new_attack = mtype_special_attack( iter->second );
        new_attack.actor->cooldown.deserialize( inner[1] );
        names.push_back( name );
        return std::make_pair( name, new_attack );
    }
};

void mtype::load( const JsonObject &jo, const std::string_view src )
{
    MonsterGenerator &gen = MonsterGenerator::generator();

    name.make_plural();
    mandatory( jo, was_loaded, "name", name );

    optional( jo, was_loaded, "description", description );

    optional( jo, was_loaded, "ascii_picture", picture_id );

    // Assign a default "flesh" material to prevent crash (#48988)
    optional( jo, was_loaded, "material", mat, weighted_string_id_reader<material_id, int> {1}, {{{material_flesh, 1}}} );
    mat_portion_total = 0;
    for( const std::pair<const material_id, int> &pr : mat ) {
        mat_portion_total += pr.second;
    }

    optional( jo, was_loaded, "species", species, string_id_reader<::species_type> {} );
    optional( jo, was_loaded, "categories", categories, auto_flags_reader<> {} );

    // See monfaction.cpp
    mandatory( jo, was_loaded, "default_faction", default_faction );

    mandatory( jo, was_loaded, "symbol", sym, unicode_symbol_reader );

    if( was_loaded && jo.has_member( "copy-from" ) && looks_like.empty() ) {
        looks_like = jo.get_string( "copy-from" );
    }
    optional( jo, was_loaded, "looks_like", looks_like, looks_like );

    optional( jo, was_loaded, "bodytype", bodytype );
    optional( jo, was_loaded, "color", color, nc_color_reader{}, c_white );
    optional( jo, was_loaded, "volume", volume, units_bound_reader<units::volume> { 0_ml }, 62499_ml );
    optional( jo, was_loaded, "weight", weight, units_bound_reader<units::mass> { 0_gram },
              81499_gram );

    optional( jo, was_loaded, "phase", phase, make_flag_reader( gen.phase_map, "phase id" ),
              phase_id::SOLID );

    optional( jo, was_loaded, "diff", difficulty_base, numeric_bound_reader<int> {0}, 0 );
    optional( jo, was_loaded, "hp", hp, numeric_bound_reader<int> {1} );
    optional( jo, was_loaded, "speed", speed, numeric_bound_reader<int> {0}, 0 );
    optional( jo, was_loaded, "aggression", agro, numeric_bound_reader<int> {-100, 100}, 0 );
    optional( jo, was_loaded, "morale", morale, 0 );
    optional( jo, was_loaded, "stomach_size", stomach_size, 0 );

    optional( jo, was_loaded, "tracking_distance", tracking_distance, numeric_bound_reader<int> {3},
              8 );

    optional( jo, was_loaded, "mountable_weight_ratio", mountable_weight_ratio, 0.2f );

    optional( jo, was_loaded, "attack_cost", attack_cost, numeric_bound_reader<int> {0}, 100 );
    optional( jo, was_loaded, "melee_skill", melee_skill, numeric_bound_reader<int> {0}, 0 );
    optional( jo, was_loaded, "melee_dice", melee_dice, numeric_bound_reader<int> {0}, 0 );
    optional( jo, was_loaded, "melee_dice_sides", melee_sides, numeric_bound_reader<int> {0}, 0 );
    optional( jo, was_loaded, "melee_dice_ap", melee_dice_ap, 0 );

    optional( jo, was_loaded, "grab_strength", grab_strength, numeric_bound_reader<int> {0}, 1 );

    optional( jo, was_loaded, "dodge", sk_dodge, numeric_bound_reader<int> {0} );

    // FIXME: load resistances by reader class
    if( jo.has_object( "armor" ) ) {
        armor = load_resistances_instance( jo.get_object( "armor" ) );
    }
    if( was_loaded && jo.has_object( "extend" ) ) {
        JsonObject ext = jo.get_object( "extend" );
        ext.allow_omitted_members();
        if( ext.has_object( "armor" ) ) {
            armor = extend_resistances_instance( armor, ext.get_object( "armor" ) );
        }
    }
    armor_proportional.reset();
    if( jo.has_object( "proportional" ) ) {
        JsonObject jprop = jo.get_object( "proportional" );
        jprop.allow_omitted_members();
        if( jprop.has_object( "armor" ) ) {
            armor_proportional = load_resistances_instance( jprop.get_object( "armor" ) );
        }
    }
    armor_relative.reset();
    if( jo.has_object( "relative" ) ) {
        JsonObject jrel = jo.get_object( "relative" );
        jrel.allow_omitted_members();
        if( jrel.has_object( "armor" ) ) {
            armor_relative = load_resistances_instance( jrel.get_object( "armor" ) );
        }
    }

    optional( jo, was_loaded, "trap_avoids", trap_avoids );

    if( !was_loaded ) {
        weakpoints_deferred.clear();
        weakpoints_deferred_inline.clear();
    }
    weakpoints_deferred_deleted.clear();

    // Load each set of weakpoints.
    // Each subsequent weakpoint set overwrites
    // matching weakpoints from the previous set.
    optional( jo, was_loaded, "weakpoint_sets", weakpoints_deferred, auto_flags_reader<weakpoints_id> {} );
    // Finally, inline weakpoints overwrite
    // any matching weakpoints from the previous sets.
    optional( jo, was_loaded, "weakpoints", weakpoints_deferred_inline, weakpoints_reader{weakpoints_deferred_deleted} );

    optional( jo, was_loaded, "status_chance_multiplier", status_chance_multiplier, numeric_bound_reader{0.0f, 5.0f},
              1.f );

    optional( jo, was_loaded, "families", families );

    optional( jo, was_loaded, "absorb_ml_per_hp", absorb_ml_per_hp, 250 );
    optional( jo, was_loaded, "split_move_cost", split_move_cost, 200 );
    optional( jo, was_loaded, "absorb_move_cost_per_ml", absorb_move_cost_per_ml, 0.025f );
    optional( jo, was_loaded, "absorb_move_cost_min", absorb_move_cost_min, 1 );
    optional( jo, was_loaded, "absorb_move_cost_max", absorb_move_cost_max, -1 );

    optional( jo, was_loaded, "absorb_material", absorb_material );
    optional( jo, was_loaded, "no_absorb_material", no_absorb_material );

    optional( jo, was_loaded, "bleed_rate", bleed_rate, 100 );

    optional( jo, was_loaded, "petfood", petfood );

    optional( jo, was_loaded, "move_skills", move_skills );

    optional( jo, was_loaded, "vision_day", vision_day, numeric_bound_reader{0}, 40 );
    optional( jo, was_loaded, "vision_night", vision_night, numeric_bound_reader{0}, 1 );

    optional( jo, was_loaded, "regenerates", regenerates, 0 );
    optional( jo, was_loaded, "regenerates_in_dark", regenerates_in_dark, false );
    optional( jo, was_loaded, "regen_morale", regen_morale, false );

    optional( jo, was_loaded, "regeneration_modifiers", regeneration_modifiers,
              weighted_string_id_reader<efftype_id, int> { 1 } );

    optional( jo, was_loaded, "starting_ammo", starting_ammo );
    optional( jo, was_loaded, "luminance", luminance, 0 );
    optional( jo, was_loaded, "revert_to_itype", revert_to_itype, itype_id() );
    optional( jo, was_loaded, "broken_itype", broken_itype, itype_id() );
    optional( jo, was_loaded, "mech_weapon", mech_weapon, itype_id() );
    optional( jo, was_loaded, "mech_str_bonus", mech_str_bonus, 0 );
    optional( jo, was_loaded, "mech_battery", mech_battery, itype_id() );

    optional( jo, was_loaded, "mount_items", mount_items );

    optional( jo, was_loaded, "revive_forms", revive_types );

    optional( jo, was_loaded, "zombify_into", zombify_into, string_id_reader<::mtype> {},
              mtype_id() );

    optional( jo, was_loaded, "fungalize_into", fungalize_into, string_id_reader<::mtype> {},
              mtype_id() );

    optional( jo, was_loaded, "aggro_character", aggro_character, true );

    optional( jo, was_loaded, "attack_effs", atk_effs );

    optional( jo, was_loaded, "melee_damage", melee_damage );

    optional( jo, was_loaded, "scents_tracked", scents_tracked );
    optional( jo, was_loaded, "scents_ignored", scents_ignored );

    if( jo.has_member( "death_drops" ) ) {
        death_drops =
            item_group::load_item_group( jo.get_member( "death_drops" ), "distribution",
                                         "death_drops for mtype " + id.str() );
    }

    // FIXME: there's code to check if harvest is null, which will never trigger as it defaults to human
    optional( jo, was_loaded, "harvest", harvest, harvest_list_human );

    optional( jo, was_loaded, "dissect", dissect );

    optional( jo, was_loaded, "decay", decay, harvest_id::NULL_ID() );

    optional( jo, was_loaded, "shearing", shearing );

    optional( jo, was_loaded, "speed_description", speed_desc, speed_description_DEFAULT );
    optional( jo, was_loaded, "death_function", mdeath_effect );

    optional( jo, was_loaded, "emit_fields", emit_fields, named_pair_reader<emit_id, time_duration> {"emit_id", "delay"} );

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

    // this is really gross, and special_attacks + special_attacks_names should be combined into one struct
    if( jo.has_member( "special_attacks" ) ) {
        special_attacks_names.clear();
    }
    optional( jo, was_loaded, "special_attacks", special_attacks, special_attacks_reader{special_attacks_names, src, id} );
    // to be extra safe
    for( const std::pair<const std::string, mtype_special_attack> &atk : special_attacks ) {
        if( dynamic_cast<const invalid_mattack_actor *>( atk.second.get() ) != nullptr ) {
            jo.throw_error( string_format( "Invalid special attack format for %s", atk.first ) );
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
        if( up.has_string( "into_group" ) ) {
            if( up.has_string( "into" ) ) {
                jo.throw_error_at( "upgrades", "Cannot specify both into_group and into." );
            }
            mandatory( up, was_loaded, "into_group", upgrade_group, string_id_reader<::MonsterGroup> {} );
            upgrade_into = mtype_id::NULL_ID();
        } else if( up.has_string( "into" ) ) {
            mandatory( up, was_loaded, "into", upgrade_into, string_id_reader<::mtype> {} );
            upgrade_group = mongroup_id::NULL_ID();
        }
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

        if( repro.has_object( "baby_type" ) ) {
            JsonObject jo_type = repro.get_object( "baby_type" );
            if( jo_type.has_member( "baby_monster" ) ) {
                mandatory( jo_type, was_loaded, "baby_monster", baby_type.baby_monster );
                baby_type.baby_monster_group = mongroup_id::NULL_ID();
                baby_type.baby_egg = itype_id::NULL_ID();
                baby_type.baby_egg_group = item_group_id::NULL_ID();
            } else if( jo_type.has_member( "baby_monster_group" ) ) {
                mandatory( jo_type, was_loaded, "baby_monster_group", baby_type.baby_monster_group );
                baby_type.baby_monster = mtype_id::NULL_ID();
                baby_type.baby_egg = itype_id::NULL_ID();
                baby_type.baby_egg_group = item_group_id::NULL_ID();
            } else if( jo_type.has_member( "baby_egg" ) ) {
                mandatory( jo_type, was_loaded, "baby_egg", baby_type.baby_egg );
                baby_type.baby_monster = mtype_id::NULL_ID();
                baby_type.baby_monster_group = mongroup_id::NULL_ID();
                baby_type.baby_egg_group = item_group_id::NULL_ID();
            } else if( jo_type.has_member( "baby_egg_group" ) ) {
                mandatory( jo_type, was_loaded, "baby_egg_group", baby_type.baby_egg_group );
                baby_type.baby_monster = mtype_id::NULL_ID();
                baby_type.baby_monster_group = mongroup_id::NULL_ID();
                baby_type.baby_egg = itype_id::NULL_ID();
            }
            if( jo_type.has_member( "baby_monster" ) + jo_type.has_member( "baby_monster_group" ) +
                jo_type.has_member( "baby_egg" ) + jo_type.has_member( "baby_egg_group" ) > 1 ) {
                debugmsg( "Reproduction has more than one baby_type defined, which is prohibited" );
            }
            reproduces = true;
        }
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

    optional( jo, was_loaded, "flags", pre_flags_, string_id_reader<mon_flag> {} );

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
        optional( jop, was_loaded, "avoid_dangerous_fields", path_settings.avoid_dangerous_fields, false );
    }
}

void MonsterGenerator::load_species( const JsonObject &jo, const std::string &src )
{
    mon_species->load( jo, src );
}

void species_type::finalize_all()
{
    MonsterGenerator::generator().mon_species->finalize();
}

void species_type::load( const JsonObject &jo, std::string_view )
{
    optional( jo, was_loaded, "description", description );
    optional( jo, was_loaded, "footsteps", footsteps, to_translation( "footsteps." ) );

    optional( jo, was_loaded, "flags", flags, string_id_reader<mon_flag> {} );

    const auto trigger_reader = enum_flags_reader<mon_trigger> { "monster trigger" };
    optional( jo, was_loaded, "anger_triggers", anger, trigger_reader );
    optional( jo, was_loaded, "placate_triggers", placate, trigger_reader );
    optional( jo, was_loaded, "fear_triggers", fear, trigger_reader );

    optional( jo, was_loaded, "bleeds", bleeds, string_id_reader<::field_type> {}, fd_null );
}

void mon_flag::load_mon_flags( const JsonObject &jo, const std::string &src )
{
    mon_flags.load( jo, src );
}

void mon_flag::finalize_all()
{
    mon_flags.finalize();
}

void mon_flag::load( const JsonObject &jo, std::string_view )
{
    mandatory( jo, was_loaded, "id", id );
}

const std::vector<mtype> &MonsterGenerator::get_all_mtypes() const
{
    return mon_templates->get_all();
}

const std::vector<mon_flag> &mon_flag::get_all()
{
    return mon_flags.get_all();
}

void mon_flag::reset()
{
    mon_flags.reset();
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
    } else if( attack_type == "invalid" ) {
        new_attack = std::make_unique<invalid_mattack_actor>();
    } else {
        obj.throw_error_at( "attack_type", "unknown monster attack" );
    }

    new_attack->load( obj, src );
    return mtype_special_attack( std::move( new_attack ) );
}

void mattack_actor::load( const JsonObject &jo, const std::string &src )
{
    if( jo.has_string( "id" ) ) {
        mandatory( jo, false, "id", id );
    } else {
        mandatory( jo, false, "type", id );
    }

    mandatory( jo, was_loaded, "cooldown", cooldown );

    load_internal( jo, src );
    // Set was_loaded manually because we don't have generic_factory to do it for us
    was_loaded = true;
}

void MonsterGenerator::load_monster_attack( const JsonObject &jo, const std::string &src )
{
    add_attack( create_actor( jo, src ) );
}

void MonsterGenerator::check_monster_definitions() const
{
    for( const mtype &mon : mon_templates->get_all() ) {
        if( !mon.src.empty() && mon.src.back().second.str() == "dda" ) {
            std::string mon_id = mon.id.str();
            std::string suffix_id = mon_id.substr( 0, mon_id.find( '_' ) );
            if( suffix_id != "mon" && suffix_id != "pseudo" ) {
                debugmsg( "monster %s is missing mon_ (or pseudo_) prefix from id", mon.id.c_str() );
            }
        }
        if( mon.harvest.is_null() && !mon.has_flag( mon_flag_ELECTRONIC ) && !mon.id.is_null() ) {
            debugmsg( "monster %s has no harvest entry", mon.id.c_str(), mon.harvest.c_str() );
        }
        if( mon.has_flag( mon_flag_MILKABLE ) && mon.starting_ammo.empty() ) {
            debugmsg( "monster %s is flagged milkable, but has no starting ammo", mon.id.c_str() );
        }
        if( mon.has_flag( mon_flag_MILKABLE ) && !mon.starting_ammo.empty() &&
            !item( mon.starting_ammo.begin()->first ).made_of( phase_id::LIQUID ) ) {
            debugmsg( "monster %s is flagged milkable, but starting ammo %s is not a liquid type",
                      mon.id.c_str(), mon.starting_ammo.begin()->first.str() );
        }
        if( mon.has_flag( mon_flag_MILKABLE ) && mon.starting_ammo.size() > 1 ) {
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
        if( !mon.broken_itype.is_empty() && !item::type_is_defined( mon.broken_itype ) ) {
            debugmsg( "monster %s has unknown broken_itype: %s", mon.id.c_str(),
                      mon.broken_itype.c_str() );
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
        if( !mon.mount_items.tied.is_empty() && !item::type_is_defined( mon.mount_items.tied ) ) {
            debugmsg( "monster %s has unknown mount_items.tied: %s", mon.id.c_str(),
                      mon.mount_items.tied.c_str() );
        }
        if( !mon.mount_items.tack.is_empty() && !item::type_is_defined( mon.mount_items.tack ) ) {
            debugmsg( "monster %s has unknown mount_items.tack: %s", mon.id.c_str(),
                      mon.mount_items.tack.c_str() );
        }
        if( !mon.mount_items.armor.is_empty() && !item::type_is_defined( mon.mount_items.armor ) ) {
            debugmsg( "monster %s has unknown mount_items.armor: %s", mon.id.c_str(),
                      mon.mount_items.armor.c_str() );
        }
        if( !mon.mount_items.storage.is_empty() && !item::type_is_defined( mon.mount_items.storage ) ) {
            debugmsg( "monster %s has unknown mount_items.storage: %s", mon.id.c_str(),
                      mon.mount_items.storage.c_str() );
        }
        if( !mon.harvest.is_valid() ) {
            debugmsg( "monster %s has invalid harvest_entry: %s", mon.id.c_str(), mon.harvest.c_str() );
        }
        if( !mon.dissect.is_empty() && !mon.dissect.is_valid() ) {
            debugmsg( "monster %s has invalid dissection harvest_entry: %s", mon.id.c_str(),
                      mon.dissect.c_str() );
        }
        if( mon.has_flag( mon_flag_WATER_CAMOUFLAGE ) && !monster( mon.id ).can_submerge() ) {
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

            bool baby_monster = mon.baby_type.baby_monster != mtype_id::NULL_ID(); // if null then false
            bool baby_monster_group = mon.baby_type.baby_monster_group != mongroup_id::NULL_ID();
            bool baby_egg = mon.baby_type.baby_egg != itype_id::NULL_ID();
            bool baby_egg_group = mon.baby_type.baby_egg_group != item_group_id::NULL_ID();
            if( !baby_monster && !baby_monster_group && !baby_egg && !baby_egg_group ) {
                debugmsg( "No baby_monster, baby_monster_group, baby_egg or baby_egg_group defined for monster %s",
                          mon.id.c_str() );
            }
            if( !mon.baby_type.baby_monster.is_valid() ) {
                debugmsg( "baby_monster %s of monster %s is not a valid monster id",
                          mon.baby_type.baby_monster.c_str(), mon.id.c_str() );
            }
            if( !mon.baby_type.baby_monster_group.is_valid() ) {
                debugmsg( "baby_monster_group %s of monster %s is not a valid monster group id",
                          mon.baby_type.baby_monster.c_str(), mon.id.c_str() );
            }
            if( !item::type_is_defined( mon.baby_type.baby_egg ) ) {
                debugmsg( "item_id %s of monster %s is not a valid item id",
                          mon.baby_type.baby_egg.c_str(), mon.id.c_str() );
            }
            if( !item_group::group_is_defined( mon.baby_type.baby_egg_group ) && baby_egg_group ) {
                debugmsg( "item_group_id %s of monster %s is not a valid itemgroup id",
                          mon.baby_type.baby_egg_group.c_str(), mon.id.c_str() );
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

        for( const std::pair<const damage_type_id, float> &dt : mon.armor.resist_vals ) {
            if( !dt.first.is_valid() ) {
                debugmsg( "Invalid armor type \"%s\" for monster %s", dt.first.c_str(), mon.id.c_str() );
            }
        }

        for( const std::pair<const std::string, mtype_special_attack> &spatk : mon.special_attacks ) {
            const melee_actor *atk = dynamic_cast<const melee_actor *>( &*spatk.second );
            if( !atk ) {
                continue;
            }
            for( const damage_unit &dt : atk->damage_max_instance.damage_units ) {
                if( !dt.type.is_valid() ) {
                    debugmsg( "Invalid monster attack damage type \"%s\" for monster %s", dt.type.c_str(),
                              mon.id.c_str() );
                }
            }
        }

        mon.weakpoints.check();
    }
}

void monster_death_effect::load( const JsonObject &jo )
{
    optional( jo, was_loaded, "message", death_message, to_translation( "The %s dies!" ) );
    optional( jo, was_loaded, "effect", sp );
    optional( jo, was_loaded, "corpse_type", corpse_type, mdeath_type::NORMAL );
    optional( jo, was_loaded, "eoc", eoc );
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

void move_skills_data::load( const JsonObject &jo )
{
    optional( jo, was_loaded, "climb", climb );
    optional( jo, was_loaded, "dig", dig );
    optional( jo, was_loaded, "swim", swim );

    if( climb && ( climb.value() < 0 || climb.value() > 10 ) ) {
        debugmsg( "climb value out of range.  It has to be between 0 and 10" );
        climb = std::max( std::min( climb.value(), 10 ), 0 );
    }
    if( dig && ( dig.value() < 0 || dig.value() > 10 ) ) {
        debugmsg( "dig value out of range.  It has to be between 0 and 10" );
        dig = std::max( std::min( dig.value(), 10 ), 0 );
    }
    if( dig && ( dig.value() < 0 || dig.value() > 10 ) ) {
        debugmsg( "dig value out of range.  It has to be between 0 and 10" );
        dig = std::max( std::min( dig.value(), 10 ), 0 );
    }
}

bool move_skills_data::handle_extend( const JsonValue &jv )
{
    JsonObject jo = jv.get_object();
    // because this is extend, was_loaded is always true
    optional( jo, true, "climb", climb );
    optional( jo, true, "dig", dig );
    optional( jo, true, "swim", swim );
    return true;
}

static void check_for_delete( const JsonObject &jo, const std::string_view name,
                              std::optional<int> &value )
{
    if( !jo.has_member( name ) ) {
        return;
    }
    std::optional<int> scratch;
    // because this is delete, was_loaded is always true
    optional( jo, true, name, scratch );
    if( value.has_value() && scratch.has_value() && *scratch == *value ) {
        value = std::nullopt;
        return;
    }
    debugmsg( "Delete value for '%s' (%d) does not match existing (%d)", name, scratch.value_or( -1 ),
              value.value_or( -1 ) );
}

bool move_skills_data::handle_delete( const JsonValue &jv )
{
    JsonObject jo = jv.get_object();
    check_for_delete( jo, "climb", climb );
    check_for_delete( jo, "dig", dig );
    check_for_delete( jo, "swim", swim );
    return true;
}

void move_skills_data::deserialize( const JsonObject &data )
{
    load( data );
}
