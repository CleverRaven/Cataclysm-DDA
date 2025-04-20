#include "monattack.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "active_item_cache.h"
#include "ballistics.h"
#include "bionics.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_assert.h"
#include "cata_utility.h"
#include "character.h"
#include "character_id.h"
#include "character_martial_arts.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "damage.h"
#include "debug.h"
#include "dispersion.h"
#include "effect.h"
#include "effect_source.h"
#include "enums.h"
#include "explosion.h"
#include "field_type.h"
#include "flag.h"
#include "fungal_effects.h"
#include "game.h"
#include "game_constants.h"
#include "harvest.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "mapdata.h"
#include "martialarts.h"
#include "material.h"
#include "mattack_common.h"
#include "memorial_logger.h"
#include "memory_fast.h"
#include "messages.h"
#include "mission.h"
#include "monfaction.h"
#include "monster.h"
#include "mtype.h"
#include "output.h"
#include "pathfinding.h"
#include "pimpl.h"
#include "point.h"
#include "projectile.h"
#include "rng.h"
#include "sounds.h"
#include "speech.h"
#include "stomach.h"
#include "string_formatter.h"
#include "text_snippets.h"
#include "timed_event.h"
#include "translation.h"
#include "translations.h"
#include "trap.h"
#include "type_id.h"
#include "uilist.h"
#include "units.h"
#include "value_ptr.h"
#include "viewer.h"
#include "weighted_list.h"

static const activity_id ACT_RELOAD( "ACT_RELOAD" );

static const ammo_effect_str_id ammo_effect_APPLY_SAP( "APPLY_SAP" );
static const ammo_effect_str_id ammo_effect_DRAW_AS_LINE( "DRAW_AS_LINE" );
static const ammo_effect_str_id ammo_effect_JET( "JET" );
static const ammo_effect_str_id ammo_effect_NO_DAMAGE_SCALING( "NO_DAMAGE_SCALING" );
static const ammo_effect_str_id ammo_effect_NO_ITEM_DAMAGE( "NO_ITEM_DAMAGE" );
static const ammo_effect_str_id ammo_effect_NO_OVERSHOOT( "NO_OVERSHOOT" );

static const bionic_id bio_uncanny_dodge( "bio_uncanny_dodge" );

static const damage_type_id damage_acid( "acid" );
static const damage_type_id damage_bash( "bash" );
static const damage_type_id damage_cut( "cut" );
static const damage_type_id damage_electric( "electric" );
static const damage_type_id damage_stab( "stab" );

static const efftype_id effect_assisted( "assisted" );
static const efftype_id effect_bite( "bite" );
static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_blind( "blind" );
static const efftype_id effect_boomered( "boomered" );
static const efftype_id effect_controlled( "controlled" );
static const efftype_id effect_corroding( "corroding" );
static const efftype_id effect_countdown( "countdown" );
static const efftype_id effect_darkness( "darkness" );
static const efftype_id effect_dazed( "dazed" );
static const efftype_id effect_deaf( "deaf" );
static const efftype_id effect_downed( "downed" );
static const efftype_id effect_dragging( "dragging" );
static const efftype_id effect_eyebot_assisted( "eyebot_assisted" );
static const efftype_id effect_eyebot_depleted( "eyebot_depleted" );
static const efftype_id effect_fungus( "fungus" );
static const efftype_id effect_glowing( "glowing" );
static const efftype_id effect_got_checked( "got_checked" );
static const efftype_id effect_grabbed( "grabbed" );
static const efftype_id effect_grabbing( "grabbing" );
static const efftype_id effect_grown_of_fuse( "grown_of_fuse" );
static const efftype_id effect_has_bag( "has_bag" );
static const efftype_id effect_operating( "operating" );
static const efftype_id effect_paid( "paid" );
static const efftype_id effect_paralyzepoison( "paralyzepoison" );
static const efftype_id effect_pet( "pet" );
static const efftype_id effect_raising( "raising" );
static const efftype_id effect_rat( "rat" );
static const efftype_id effect_shrieking( "shrieking" );
static const efftype_id effect_slimed( "slimed" );
static const efftype_id effect_social_dissatisfied( "social_dissatisfied" );
static const efftype_id effect_stunned( "stunned" );

static const flag_id json_flag_CANNOT_MOVE( "CANNOT_MOVE" );

static const itype_id itype_anesthetic( "anesthetic" );
static const itype_id itype_badge_deputy( "badge_deputy" );
static const itype_id itype_badge_detective( "badge_detective" );
static const itype_id itype_badge_doctor( "badge_doctor" );
static const itype_id itype_badge_marshal( "badge_marshal" );
static const itype_id itype_badge_swat( "badge_swat" );
static const itype_id itype_bot_c4_hack( "bot_c4_hack" );
static const itype_id itype_bot_flashbang_hack( "bot_flashbang_hack" );
static const itype_id itype_bot_gasbomb_hack( "bot_gasbomb_hack" );
static const itype_id itype_bot_grenade_hack( "bot_grenade_hack" );
static const itype_id itype_bot_mininuke_hack( "bot_mininuke_hack" );
static const itype_id itype_bot_pacification_hack( "bot_pacification_hack" );
static const itype_id itype_e_handcuffs( "e_handcuffs" );
static const itype_id itype_processor( "processor" );
static const itype_id itype_sporeos( "sporeos" );

static const json_character_flag json_flag_BIONIC_LIMB( "BIONIC_LIMB" );

static const limb_score_id limb_score_grip( "grip" );
static const limb_score_id limb_score_reaction( "reaction" );

static const material_id material_budget_steel( "budget_steel" );
static const material_id material_ch_steel( "ch_steel" );
static const material_id material_flesh( "flesh" );
static const material_id material_hc_steel( "hc_steel" );
static const material_id material_hflesh( "hflesh" );
static const material_id material_iflesh( "iflesh" );
static const material_id material_iron( "iron" );
static const material_id material_lc_steel( "lc_steel" );
static const material_id material_mc_steel( "mc_steel" );
static const material_id material_qt_steel( "qt_steel" );
static const material_id material_steel( "steel" );
static const material_id material_veggy( "veggy" );
static const material_id material_water( "water" );

static const morale_type morale_support( "morale_support" );

static const mtype_id mon_amalgamation_breather( "mon_amalgamation_breather" );
static const mtype_id mon_amalgamation_breather_hub( "mon_amalgamation_breather_hub" );
static const mtype_id mon_biollante( "mon_biollante" );
static const mtype_id mon_blob( "mon_blob" );
static const mtype_id mon_blob_brain( "mon_blob_brain" );
static const mtype_id mon_blob_large( "mon_blob_large" );
static const mtype_id mon_blob_small( "mon_blob_small" );
static const mtype_id mon_breather( "mon_breather" );
static const mtype_id mon_breather_hub( "mon_breather_hub" );
static const mtype_id mon_creeper_hub( "mon_creeper_hub" );
static const mtype_id mon_creeper_vine( "mon_creeper_vine" );
static const mtype_id mon_fungal_hedgerow( "mon_fungal_hedgerow" );
static const mtype_id mon_fungal_tendril( "mon_fungal_tendril" );
static const mtype_id mon_fungal_wall( "mon_fungal_wall" );
static const mtype_id mon_fungaloid( "mon_fungaloid" );
static const mtype_id mon_fungaloid_young( "mon_fungaloid_young" );
static const mtype_id mon_headless_dog_thing( "mon_headless_dog_thing" );
static const mtype_id mon_hound_tindalos_afterimage( "mon_hound_tindalos_afterimage" );
static const mtype_id mon_leech_blossom( "mon_leech_blossom" );
static const mtype_id mon_leech_root_drone( "mon_leech_root_drone" );
static const mtype_id mon_leech_root_runner( "mon_leech_root_runner" );
static const mtype_id mon_leech_stalk( "mon_leech_stalk" );
static const mtype_id mon_nursebot_defective( "mon_nursebot_defective" );
static const mtype_id mon_shadow( "mon_shadow" );
static const mtype_id mon_triffid( "mon_triffid" );
static const mtype_id mon_turret_searchlight( "mon_turret_searchlight" );
static const mtype_id mon_zombie_dancer( "mon_zombie_dancer" );
static const mtype_id mon_zombie_gasbag_crawler( "mon_zombie_gasbag_crawler" );
static const mtype_id mon_zombie_gasbag_impaler( "mon_zombie_gasbag_impaler" );
static const mtype_id mon_zombie_jackson( "mon_zombie_jackson" );
static const mtype_id mon_zombie_skeltal_minion( "mon_zombie_skeltal_minion" );

static const skill_id skill_melee( "melee" );

static const species_id species_LEECH_PLANT( "LEECH_PLANT" );
static const species_id species_SLIME( "SLIME" );
static const species_id species_ZOMBIE( "ZOMBIE" );

static const ter_str_id ter_t_dirt( "t_dirt" );
static const ter_str_id ter_t_dirtmound( "t_dirtmound" );
static const ter_str_id ter_t_marloss( "t_marloss" );
static const ter_str_id ter_t_root_wall( "t_root_wall" );
static const ter_str_id ter_t_tree( "t_tree" );
static const ter_str_id ter_t_tree_young( "t_tree_young" );
static const ter_str_id ter_t_underbrush( "t_underbrush" );

static const trait_id trait_MARLOSS( "MARLOSS" );
static const trait_id trait_MARLOSS_BLUE( "MARLOSS_BLUE" );
static const trait_id trait_PROF_FED( "PROF_FED" );
static const trait_id trait_PROF_PD_DET( "PROF_PD_DET" );
static const trait_id trait_PROF_POLICE( "PROF_POLICE" );
static const trait_id trait_PROF_SWAT( "PROF_SWAT" );
static const trait_id trait_THRESH_MARLOSS( "THRESH_MARLOSS" );
static const trait_id trait_THRESH_MYCUS( "THRESH_MYCUS" );

// shared utility functions
static bool within_visual_range( monster *z, int max_range )
{
    const map &here = get_map();

    Character &player_character = get_player_character();
    return !( rl_dist( z->pos_abs(), player_character.pos_abs() ) > max_range ||
              !z->sees( here, player_character ) );
}

static bool within_target_range( const monster *const z, const Creature *const target, int range )
{
    const map &here = get_map();

    return target != nullptr &&
           rl_dist( z->pos_abs(), target->pos_abs() ) <= range &&
           z->sees( here, *target );
}

static Creature *sting_get_target( monster *z, float range = 5.0f )
{
    const map &here = get_map();

    Creature *target = z->attack_target();

    if( target == nullptr ) {
        return nullptr;
    }

    // Can't see/reach target, no attack
    if( !z->sees( here,  *target ) ||
        !here.clear_path( z->pos_bub(), target->pos_bub(), range, 1, 100 ) ) {
        return nullptr;
    }

    return rl_dist( z->pos_abs(), target->pos_abs() ) <= range ? target : nullptr;
}

static bool sting_shoot( monster *z, Creature *target, damage_instance &dam, float range )
{
    if( target->uncanny_dodge() ) {
        target->add_msg_if_player( m_bad, _( "The %s shoots a dart but you dodge it." ),
                                   z->name() );
        return false;
    }

    projectile proj;
    proj.speed = 10;
    proj.range = range;
    proj.impact.add( dam );
    proj.proj_effects.insert( ammo_effect_NO_OVERSHOOT );

    dealt_projectile_attack atk;
    projectile_attack( atk, proj, z->pos_bub(), target->pos_bub(),
                       dispersion_sources{ 500 }, z );
    if( atk.dealt_dam.total_damage() > 0 ) {
        target->add_msg_if_player( m_bad, _( "The %s shoots a dart into you!" ), z->name() );
        // whether this function returns true determines whether it applies a status effect like paralysis or mutation
        if( atk.dealt_dam.bp_hit->has_flag( json_flag_BIONIC_LIMB ) ) {
            return false;
        } else {
            return true;
        }
    } else {
        if( atk.missed_by == 1 ) {
            target->add_msg_if_player( m_good,
                                       _( "The %s shoots a dart at you, but misses!" ),
                                       z->name() );
        } else {
            target->add_msg_if_player( m_good,
                                       _( "The %s shoots a dart but it bounces off your armor." ),
                                       z->name() );
        }
        return false;
    }
}

bool mattack::none( monster * )
{
    return true;
}

bool mattack::eat_crop( monster *z )
{
    std::optional<tripoint_bub_ms> target;
    int num_targets = 1;
    map &here = get_map();
    for( const tripoint_bub_ms &p : here.points_in_radius( z->pos_bub(), 1 ) ) {
        if( here.has_flag( ter_furn_flag::TFLAG_PLANT, p ) &&
            ( here.has_flag( ter_furn_flag::TFLAG_GROWTH_HARVEST, p ) ||
              here.has_flag( ter_furn_flag::TFLAG_GROWTH_MATURE, p ) ) && one_in( num_targets ) ) {
            num_targets++;
            target = p;
        }
        if( target ) {
            if( !z->has_fully_eaten() ) {
                add_msg_if_player_sees( *z, _( "The %1s eats the %2s." ), z->name(), here.furnname( p ) );
                here.furn_set( *target, furn_str_id( here.furn( *target )->plant->base ) );
                here.i_clear( *target );
                z->mod_amount_eaten( 350 );
                return true;
            }
        }
        map_stack items = here.i_at( p );
        for( item &item : items ) {
            //This prevents crop eaters from eating planted seeds
            if( here.has_flag( ter_furn_flag::TFLAG_PLANT, p ) ) {
                continue;
            }
            if( !item.is_food() || item.get_comestible_fun() < -20 || item.made_of( material_water ) ||
                !item.has_flag( flag_CATTLE ) ) {
                continue;
            }
            if( !z->has_fully_eaten() ) {
                //Check for stomach size 0 so as to not break creatures which haven't
                //been given a stomach size yet.
                int consumed = 1;
                if( item.count_by_charges() ) {
                    int kcal = default_character_compute_effective_nutrients( item ).kcal();
                    z->mod_amount_eaten( kcal );
                    add_msg_if_player_sees( *z, _( "The %1s eats the %2s." ), z->name(), item.display_name() );
                    here.use_charges( p, 1, item.type->get_id(), consumed );
                } else {
                    int kcal = default_character_compute_effective_nutrients( item ).kcal();
                    z->mod_amount_eaten( kcal );
                    add_msg_if_player_sees( *z, _( "The %1s gobbles up the %2s." ), z->name(), item.display_name() );
                    here.use_amount( p, 1, item.type->get_id(), consumed );
                }
                return true;
            }
        }
    }
    return true;
}

bool mattack::split( monster *z )
{
    bool split_performed = false;
    while( z->get_hp() / 2 > z->type->hp ) {
        monster *const spawn = g->place_critter_around( z->type->id, z->pos_bub(), 1 );
        if( !spawn ) {
            break;
        }
        split_performed = true;
        z->set_hp( z->get_hp() - z->type->hp );
        //this is a new copy of the monster. Ideally we should copy the stats/effects that affect the parent
        spawn->make_ally( *z );
        add_msg_if_player_sees( *z, _( "The %s splits in two!" ), z->name() );
        z->mod_moves( -z->type->split_move_cost );
        spawn->mod_moves( -z->type->split_move_cost );

        if( z->get_moves() <= 0 ) {
            break;
        }
    }

    return split_performed;
}

bool mattack::absorb_items( monster *z )
{
    map &here = get_map();

    static const units::quantity<int, units::volume_in_milliliter_tag> ml_per_hp =
        units::from_milliliter( z->type->absorb_ml_per_hp );

    std::vector<item *> consumed_items;
    std::vector<material_id> absorb_material = z->get_absorb_material();
    std::vector<material_id> no_absorb_material = z->get_no_absorb_material();

    for( item &elem : here.i_at( z->pos_bub() ) ) {
        bool any_materials_match = false;

        // there is no whitelist or blacklist so allow anything.
        if( absorb_material.empty() && no_absorb_material.empty() ) {
            any_materials_match = true;
            // there is a whitelist but no blacklist
        } else if( !absorb_material.empty() && no_absorb_material.empty() ) {
            for( const material_type *mat_type : elem.made_of_types() ) {
                if( std::find( absorb_material.begin(), absorb_material.end(),
                               mat_type->id ) != absorb_material.end() ) {
                    any_materials_match = true;
                }
            }
            // there is no whitelist but there is a blacklist
        } else if( absorb_material.empty() && !no_absorb_material.empty() ) {
            bool found = false;
            for( const material_type *mat_type : elem.made_of_types() ) {
                if( std::find( no_absorb_material.begin(), no_absorb_material.end(),
                               mat_type->id ) != no_absorb_material.end() ) {
                    found = true;
                }
            }
            // if the item wasn't on the blacklist, it's allowed
            if( !found ) {
                any_materials_match = true;
            }
            // there is a whitelist and a blacklist
        } else if( !absorb_material.empty() && !no_absorb_material.empty() ) {
            for( const material_type *mat_type : elem.made_of_types() ) {
                if( !( std::find( no_absorb_material.begin(), no_absorb_material.end(),
                                  mat_type->id ) != no_absorb_material.end() ) ) {
                    // the item wasn't found on the blacklist so check the whitelist
                    if( std::find( absorb_material.begin(), absorb_material.end(),
                                   mat_type->id ) != absorb_material.end() ) {
                        any_materials_match = true;
                    }
                }
            }
        }

        // make sure we don't absorb the wrong types of items
        if( !any_materials_match ) {
            continue;
        }

        add_msg_if_player_sees( *z,
                                _( "The %s flows around the %s and it is quickly dissolved!" ),
                                z->name(), elem.display_name() );

        int volume_in_ml = units::to_milliliter( elem.volume() );
        // Allows the monster to exceed normal max HP. Split occurs as normal max HP * 2.
        z->set_hp( z->get_hp() + std::max( volume_in_ml / ml_per_hp.value(), 1 ) );
        int absorb_move_cost = static_cast<int>( z->type->absorb_move_cost_per_ml * volume_in_ml );
        absorb_move_cost = std::max( absorb_move_cost, z->type->absorb_move_cost_min );
        if( z->type->absorb_move_cost_max != -1 ) {
            absorb_move_cost = clamp( absorb_move_cost, z->type->absorb_move_cost_min,
                                      z->type->absorb_move_cost_max );
        }
        z->mod_moves( -absorb_move_cost );
        consumed_items.push_back( &elem );
        // stop consuming once we're out of moves
        if( z->get_moves() <= 0 ) {
            break;
        }
    }
    for( item *it : consumed_items ) {
        // check if the item being removed is a corpse so that the items are dropped.
        // only do this if the monster is selectively eating flesh
        if( it->is_container() && !absorb_material.empty() ) {
            it->spill_contents( z->pos_bub() );
        }
        here.i_rem( z->pos_bub(), it );
    }

    return !consumed_items.empty();
}

bool mattack::eat_food( monster *z )
{
    map &here = get_map();
    for( const tripoint_bub_ms &p : here.points_in_radius( z->pos_bub(), 1 ) ) {
        //Protect crop seeds from carnivores, give omnivores eat_crop special also
        if( here.has_flag( ter_furn_flag::TFLAG_PLANT, p ) ) {
            continue;
        }
        // Don't snap up food RIGHT under the player's nose.
        if( z->friendly && rl_dist( get_player_character().pos_bub(), p ) <= 2 ) {
            continue;
        }
        map_stack items = here.i_at( p );
        for( item &item : items ) {
            //Fun limit prevents scavengers from eating feces
            if( !item.is_food() || item.get_comestible_fun() < -20 || item.has_flag( flag_INEDIBLE ) ||
                item.made_of( material_water ) ) {
                continue;
            }
            //Don't eat own eggs
            if( z->type->baby_type.baby_egg != item.type->get_id() && ( !z->has_fully_eaten() ) ) {
                int consumed = 1;
                if( item.count_by_charges() ) {
                    int kcal = default_character_compute_effective_nutrients( item ).kcal();
                    z->mod_amount_eaten( kcal );
                    add_msg_if_player_sees( *z, _( "The %1s eats the %2s." ), z->name(), item.display_name() );
                    here.use_charges( p, 1, item.type->get_id(), consumed );
                } else {
                    int kcal = default_character_compute_effective_nutrients( item ).kcal();
                    z->mod_amount_eaten( kcal );
                    add_msg_if_player_sees( *z, _( "The %1s gobbles up the %2s." ), z->name(), item.display_name() );
                    here.use_amount( p, 1, item.type->get_id(), consumed );
                }
                return true;
            }
        }
    }
    return true;
}

bool mattack::eat_carrion( monster *z )
{
    map &here = get_map();
    for( const tripoint_bub_ms &p : here.points_in_radius( z->pos_bub(), 1 ) ) {
        // Don't snap up food RIGHT under the player's nose.
        if( rl_dist( get_player_character().pos_bub(), p ) <= 2 ) {
            continue;
        }
        map_stack items = here.i_at( p );
        for( item &item : items ) {
            //TODO: Completely eaten corpses should leave bones and other inedibles.
            if( item.has_flag( flag_CORPSE ) && item.damage() < item.max_damage() &&
                !z->has_fully_eaten() &&
                ( item.made_of( material_flesh ) || item.made_of( material_iflesh ) ||
                  item.made_of( material_hflesh ) || item.made_of( material_veggy ) ) ) {
                item.mod_damage( 700 );
                if( item.damage() >= item.max_damage() && item.can_revive() ) {
                    item.set_flag( flag_PULPED );
                }
                z->mod_amount_eaten( 100 );
                add_msg_if_player_sees( *z, _( "The %1s gnaws on the %2s." ), z->name(), item.display_name() );
                return true;
            }
        }
    }
    return true;
}

bool mattack::graze( monster *z )
{
    map &here = get_map();
    //Grazers eat grass and entire plants or bushes. Toxic/inedible plants can be blacklisted with GRAZER_INEDIBLE.
    for( const tripoint_bub_ms &p : here.points_in_radius( z->pos_bub(), 1 ) ) {
        //Don't eat grass right next to the player.
        if( z->friendly && rl_dist( get_player_character().pos_bub(), p ) <= 2 ) {
            continue;
        }
        if( here.has_flag( ter_furn_flag::TFLAG_FLOWER, p ) &&
            !here.has_flag( ter_furn_flag::TFLAG_GRAZER_INEDIBLE, p ) &&
            ( !z->has_fully_eaten() ) ) {
            here.furn_set( p, furn_str_id::NULL_ID() );
            z->mod_amount_eaten( 50 );
            //Calorie amount is based on the "small_plant" dummy item, as with the grazer mutation.
            return true;
        }
        if( here.has_flag( ter_furn_flag::TFLAG_SHRUB, p ) &&
            !here.has_flag( ter_furn_flag::TFLAG_GRAZER_INEDIBLE, p ) &&
            ( !z->has_fully_eaten() ) ) {
            add_msg_if_player_sees( *z, _( "The %1s eats the %2s." ), z->name(), here.tername( p ) );
            here.ter_set( p, ter_t_dirt );
            z->mod_amount_eaten( 174 );
            //Calorie amount is based on the "underbrush" dummy item, as with the grazer mutation.
            return true;
        }
        if( here.has_flag( ter_furn_flag::TFLAG_GRAZABLE, p ) && !z->has_fully_eaten() ) {
            here.ter_set( p, here.get_ter_transforms_into( p ) );
            z->mod_amount_eaten( 70 );
            //Calorie amount is based on the "grass" dummy item, as with the grazer mutation.
            return true;
        }
    }
    return true;
}

bool mattack::browse( monster *z )
{
    map &here = get_map();
    //Browsers eat fruit/nuts/etc off of seasonally harvestable plants and trees.
    for( const tripoint_bub_ms &p : here.points_in_radius( z->pos_bub(), 1 ) ) {
        // Don't forage for food if the player is right there.
        if( z->friendly && rl_dist( get_player_character().pos_bub(), p ) <= 2 ) {
            continue;
        }
        if( here.has_flag( ter_furn_flag::TFLAG_BROWSABLE, p ) && ( !z->has_fully_eaten() ) ) {
            const harvest_id harvest = here.get_harvest( p );
            if( !harvest.is_null() || !harvest->empty() ) {
                add_msg_if_player_sees( *z, _( "The %1s eats from the %2s." ), z->name(), here.tername( p ) );
                here.ter_set( p, here.get_ter_transforms_into( p ) );
                z->mod_amount_eaten( 174 );
                //Calorie amount is based on the "underbrush" dummy item, as with the grazer mutation.
                return true;
            }
        }
    }
    return true;
}

bool mattack::shriek( monster *z )
{
    const map &here = get_map();

    Creature *target = z->attack_target();
    if( target == nullptr ||
        rl_dist( z->pos_abs(), target->pos_abs() ) > 4 ||
        !z->sees( here, *target ) ) {
        return false;
    }

    // It takes a while
    z->mod_moves( -to_moves<int>( 1_seconds ) * 2.4 );
    sounds::sound( z->pos_bub(), 50, sounds::sound_t::alert, _( "a terrible shriek!" ), false, "shout",
                   "shriek" );
    return true;
}

bool mattack::shriek_alert( monster *z )
{
    const map &here = get_map();

    if( !z->can_act() || z->has_effect( effect_shrieking ) ) {
        return false;
    }

    Creature *target = z->attack_target();

    if( target == nullptr || rl_dist( z->pos_bub(), target->pos_bub() ) > 15 ||
        !z->sees( here, *target ) ) {
        return false;
    }
    add_msg_if_player_sees( *z, _( "The %s begins shrieking!" ), z->name() );
    z->mod_moves( -to_moves<int>( 1_seconds ) * 1.5 );
    sounds::sound( z->pos_bub(), 120, sounds::sound_t::alert, _( "a piercing wail!" ), false, "shout",
                   "wail" );
    z->add_effect( effect_shrieking, 1_minutes );

    return true;
}

bool mattack::shriek_stun( monster *z )
{
    map &here = get_map();

    if( !z->can_act() || !z->has_effect( effect_shrieking ) ) {
        return false;
    }

    Creature *target = z->attack_target();
    if( target == nullptr ) {
        return false;
    }

    int dist = rl_dist( z->pos_bub(), target->pos_bub() );
    // Currently the cone is 2D, so don't use it for 3D attacks
    if( dist > 7 ||
        z->posz() != target->posz() ||
        !z->sees( here, *target ) ) {
        return false;
    }

    units::angle target_angle = coord_to_angle( z->pos_bub(), target->pos_bub() );
    units::angle cone_angle = 20_degrees;
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint_bub_ms &cone : here.points_in_radius( z->pos_bub(), 4 ) ) {
        units::angle tile_angle = coord_to_angle( z->pos_bub().raw(), cone.raw() );
        units::angle diff = units::fabs( target_angle - tile_angle );
        // Skip the target, because it's outside cone or it's the source
        if( diff + cone_angle > 360_degrees || diff > cone_angle || cone == z->pos_bub() ) {
            continue;
        }
        // Affect the target
        // Small bash to every square, silent to not flood message box
        here.bash( cone, 4, true );

        // If a monster is there, chance for stun
        Creature *target = creatures.creature_at( cone );
        if( target == nullptr ) {
            continue;
        }
        if( one_in( dist / 2 ) && !target->is_immune_effect( effect_deaf ) ) {
            target->add_effect( effect_dazed, rng( 1_minutes, 2_minutes ), false, rng( 1, ( 15 - dist ) / 3 ) );
        }

    }

    return true;
}

bool mattack::rattle( monster *z )
{
    const map &here = get_map();

    // TODO: Let it rattle at non-player friendlies
    const int min_dist = z->friendly != 0 ? 1 : 4;
    Creature *target = &get_player_character();
    // Can't use attack_target - the snake has no target
    if( rl_dist( z->pos_abs(), target->pos_abs() ) > min_dist ||
        !z->sees( here, *target ) ) {
        return false;
    }

    // It takes a very short while
    z->mod_moves( -to_moves<int>( 1_seconds ) * 0.2 );
    sounds::sound( z->pos_bub(), 10, sounds::sound_t::alarm, _( "a sibilant rattling sound!" ), false,
                   "misc", "rattling" );

    return true;
}

bool mattack::acid( monster *z )
{
    if( !z->can_act() ) {
        return false;
    }

    Creature *target = z->attack_target();
    if( target == nullptr ) {
        return false;
    }

    map &here = get_map();
    // Can't see/reach target, no attack
    if( !z->sees( here,  *target ) ||
        !here.clear_path( z->pos_bub(), target->pos_bub(), 10, 1, 100 ) ) {
        return false;
    }
    // It takes a while
    z->mod_moves( -to_moves<int>( 3_seconds ) );
    sounds::sound( z->pos_bub(), 4, sounds::sound_t::combat, _( "a spitting noise." ), false, "misc",
                   "spitting" );

    projectile proj;
    proj.speed = 10;
    // Mostly just for momentum
    proj.impact.add_damage( damage_acid, 5 );
    proj.range = 10;
    proj.proj_effects.insert( ammo_effect_NO_OVERSHOOT );
    dealt_projectile_attack dealt;
    projectile_attack( dealt, proj, z->pos_bub(), target->pos_bub(),
                       dispersion_sources{ 5400 }, z );
    const tripoint_bub_ms &hitp = dealt.end_point ;
    const Creature *hit_critter = dealt.last_hit_critter;
    if( hit_critter == nullptr && here.hit_with_acid( hitp ) ) {
        add_msg_if_player_sees( hitp,  _( "A glob of acid hits the %s!" ), here.tername( hitp ) );
        if( here.impassable( hitp ) ) {
            // TODO: Allow it to spill on the side it hit from
            return true;
        }
    }

    for( int i = -3; i <= 3; i++ ) {
        for( int j = -3; j <= 3; j++ ) {
            tripoint_bub_ms dest = hitp + tripoint_rel_ms( i, j, 0 );
            if( here.passable( dest ) &&
                here.clear_path( dest, hitp, 6, 1, 100 ) &&
                ( ( one_in( std::abs( j ) ) && one_in( std::abs( i ) ) ) || ( i == 0 && j == 0 ) ) ) {
                here.add_field( dest, fd_acid, 2 );
            }
        }
    }

    return true;
}

bool mattack::acid_barf( monster *z )
{
    map &here = get_map();

    if( !z->can_act() ) {
        return false;
    }

    // Let it be used on non-player creatures
    Creature *target = z->attack_target();
    if( target == nullptr || !z->is_adjacent( target, false ) ) {
        return false;
    }

    z->mod_moves( -to_moves<int>( 1_seconds ) * 0.8 );
    // Make sure it happens before uncanny dodge
    here.add_field( target->pos_bub(), fd_acid, 1 );

    bodypart_id hit = target->get_random_body_part();
    damage_instance dam_inst = damage_instance( damage_acid, rng( 5, 12 ) );

    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if( target->dodge_check( z, hit, dam_inst ) ) {
        game_message_type msg_type = target->is_avatar() ? m_warning : m_info;
        target->add_msg_player_or_npc( msg_type,
                                       _( "The %s barfs acid at you, but you dodge!" ),
                                       _( "The %s barfs acid at <npcname>, but they dodge!" ),
                                       z->name() );

        target->on_dodge( z, z->type->melee_skill * 2 );
        return true;
    }

    target->block_hit( z, hit, dam_inst );

    int dam = target->deal_damage( z,  hit, dam_inst ).total_damage();

    target->add_env_effect( effect_corroding, hit, 5, time_duration::from_turns( dam / 2 + 5 ), hit );

    if( dam > 0 ) {
        game_message_type msg_type = target->is_avatar() ? m_bad : m_info;
        target->add_msg_player_or_npc( msg_type,
                                       //~ 1$s is monster name, 2$s bodypart in accusative
                                       _( "The %1$s barfs acid on your %2$s for %3$d damage!" ),
                                       //~ 1$s is monster name, 2$s bodypart in accusative
                                       _( "The %1$s barfs acid on <npcname>'s %2$s for %3$d damage!" ),
                                       z->name(),
                                       body_part_name_accusative( hit ),
                                       dam );

        if( hit == bodypart_id( "eyes" ) ) {
            target->add_env_effect( effect_blind, bodypart_id( "eyes" ), 3, 1_minutes );
        }
    } else {
        target->add_msg_player_or_npc(
            _( "The %1$s barfs acid on your %2$s, but it washes off the armor!" ),
            _( "The %1$s barfs acid on <npcname>'s %2$s, but it washes off the armor!" ),
            z->name(),
            body_part_name_accusative( hit ) );
    }

    target->on_hit( &here, z, hit,  z->type->melee_skill );

    return true;
}

bool mattack::shockstorm( monster *z )
{
    map &here = get_map();
    const tripoint_bub_ms z_pos = z->pos_bub( here );

    if( !z->can_act() ) {
        return false;
    }

    Creature *target = z->attack_target();

    if( target == nullptr ) {
        return false;
    }

    const tripoint_bub_ms target_pos = target->pos_bub( here );

    Character &player_character = get_player_character();
    bool seen = player_character.sees( here, *z );

    bool can_attack = z->sees( here, *target ) && rl_dist( z->pos_abs(), target->pos_abs() ) <= 12;
    std::vector<tripoint_bub_ms> path = here.find_clear_path( z_pos, target_pos );
    for( const tripoint_bub_ms &point : path ) {
        if( here.impassable( point ) &&
            !( here.has_flag( ter_furn_flag::TFLAG_THIN_OBSTACLE, point ) ||
               here.has_flag( ter_furn_flag::TFLAG_PERMEABLE, point ) ) ) {
            can_attack = false;
            break;
        }
    }

    // Can't see/reach target, no attack
    if( !can_attack ) {
        return false;
    }

    // It takes a while
    z->mod_moves( -to_moves<int>( 1_seconds ) * 0.5 );

    if( seen ) {
        game_message_type msg_type = target->is_avatar() ? m_bad : m_neutral;
        add_msg( msg_type, _( "A bolt of electricity arcs towards %s!" ), target->disp_name() );
    }
    if( !player_character.is_deaf() ) {
        sfx::play_variant_sound( "fire_gun", "bio_lightning", sfx::get_heard_volume( z_pos ) );
    }
    tripoint_bub_ms tarp( target_pos.x() + rng( -1, 1 ) + rng( -1, 1 ),
                          target_pos.y() + rng( -1, 1 ) + rng( -1, 1 ),
                          target_pos.z() );
    std::vector<tripoint_bub_ms> bolt = line_to( z_pos, tarp, 0, 0 );
    // Fill the LOS with electricity
    for( tripoint_bub_ms &i : bolt ) {
        if( !one_in( 4 ) ) {
            here.add_field( i, fd_electricity, rng( 1, 3 ) );
        }
    }

    // 3x3 cloud of electricity at the square hit
    for( const tripoint_bub_ms &dest : here.points_in_radius( tarp, 1 ) ) {
        if( one_in( 3 ) ) {
            here.add_field( dest, fd_electricity, rng( 4, 10 ) );
        }
    }

    return true;
}

bool mattack::shocking_reveal( monster *z )
{
    shockstorm( z );
    const translation WHAT_A_SCOOP = SNIPPET.random_from_category( "clickbait" ).value_or(
                                         translation() );
    sounds::sound( z->pos_bub(), 10, sounds::sound_t::alert,
                   string_format( _( "the %s obnoxiously yelling \"%s!!!\"" ),
                                  z->name(), WHAT_A_SCOOP ) );
    return true;
}

bool mattack::pull_metal_weapon( monster *z )
{
    map &here = get_map();

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constants and Configuration

    // max distance that "pull_metal_weapon" can be applied to the target.
    constexpr int max_distance = 12;

    // attack movement costs
    constexpr int att_cost_pull = 150;

    // minimum str to resist "pull_metal_weapon"
    constexpr int min_str = 4;

    Creature *target = z->attack_target();
    if( target == nullptr ) {
        return false;
    }

    // Can't see/reach target, no attack
    if( !z->sees( here, *target ) || !here.clear_path( z->pos_bub( here ), target->pos_bub( here ),
            max_distance, 1, 100 ) ) {
        return false;
    }
    Character *foe = dynamic_cast< Character * >( target );
    if( foe != nullptr ) {
        // Wielded steel or iron items except for built-in things like bionic claws or monomolecular blade
        const item_location weapon = foe->get_wielded_item();
        if( weapon ) {
            const int metal_portion = weapon->made_of( material_budget_steel ) +
                                      weapon->made_of( material_ch_steel ) +
                                      weapon->made_of( material_hc_steel ) +
                                      weapon->made_of( material_iron ) +
                                      weapon->made_of( material_lc_steel ) +
                                      weapon->made_of( material_mc_steel ) +
                                      weapon->made_of( material_qt_steel ) +
                                      weapon->made_of( material_steel );

            // Take the total portion of metal in the item into account
            const float metal_fraction = metal_portion / static_cast<float>( weapon->type->mat_portion_total );
            if( !weapon->has_flag( flag_NO_UNWIELD ) && metal_portion ) {
                const float wp_skill = foe->get_skill_level( skill_melee );
                // It takes a while
                z->mod_moves( -att_cost_pull );
                int success = 100;
                ///\Grip strength increases resistance to pull_metal_weapon special attack
                if( foe->str_cur > min_str ) {
                    ///\EFFECT_MELEE increases resistance to pull_metal_weapon special attack
                    success = std::max( ( 100 * metal_fraction ) - ( 6 * ( foe->str_cur - 6 ) * foe->get_limb_score(
                                            limb_score_grip ) ) - ( 6 * wp_skill ),
                                        0.0f );
                }
                game_message_type m_type = foe->is_avatar() ? m_bad : m_neutral;
                if( rng( 1, 100 ) <= success ) {
                    item pulled_weapon = foe->remove_weapon();
                    projectile proj;
                    proj.speed = 50;
                    proj.impact = damage_instance( damage_bash, pulled_weapon.weight() / 250_gram );
                    // make the projectile stop one tile short to prevent hitting the monster
                    proj.range = rl_dist( foe->pos_bub(), z->pos_bub() ) - 1;
                    proj.proj_effects = { { ammo_effect_NO_ITEM_DAMAGE, ammo_effect_DRAW_AS_LINE, ammo_effect_NO_DAMAGE_SCALING, ammo_effect_JET } };

                    dealt_projectile_attack dealt;
                    projectile_attack( dealt, proj, foe->pos_bub(), z->pos_bub(),
                                       dispersion_sources{ 0 }, z );
                    here.add_item( dealt.end_point, pulled_weapon );
                    target->add_msg_player_or_npc( m_type, _( "The %s is pulled away from your hands!" ),
                                                   _( "The %s is pulled away from <npcname>'s hands!" ), pulled_weapon.tname() );
                    if( foe->has_activity( ACT_RELOAD ) ) {
                        foe->cancel_activity();
                    }
                    foe->recoil = MAX_RECOIL;
                } else {
                    target->add_msg_player_or_npc( m_type,
                                                   _( "The %s unsuccessfully attempts to pull your weapon away." ),
                                                   _( "The %s unsuccessfully attempts to pull <npcname>'s weapon away." ), z->name() );
                }
            }
        }
    }

    return true;
}

bool mattack::boomer( monster *z )
{
    map &here = get_map();

    if( !z->can_act() ) {
        return false;
    }

    Creature *target = z->attack_target();
    if( target == nullptr || rl_dist( z->pos_abs(), target->pos_abs() ) > 3 ||
        !z->sees( here, *target ) ) {
        return false;
    }

    std::vector<tripoint_bub_ms> line = here.find_clear_path( z->pos_bub(), target->pos_bub() );
    // It takes a while
    z->mod_moves( -to_moves<int>( 1_seconds ) * 2.5 );
    Character &player_character = get_player_character();
    bool u_see = player_character.sees( here, *z );
    if( u_see ) {
        add_msg( m_warning, _( "The %s spews bile!" ), z->name() );
    }
    for( tripoint_bub_ms &i : line ) {
        here.add_field( i, fd_bile, 1.0f );
        // If bile hit a solid tile, return.
        if( here.impassable( i ) ) {
            here.add_field( i, fd_bile, 3 );
            add_msg_if_player_sees( i,  _( "Bile splatters on the %s!" ), here.tername( i ) );
            return true;
        }
    }

    if( !target->dodge_check( z, 1.0f ) ) {
        target->add_env_effect( effect_boomered, bodypart_id( "eyes" ), 3, 12_turns );
    } else if( u_see ) {
        target->add_msg_player_or_npc( _( "You dodge it!" ),
                                       _( "<npcname> dodges it!" ) );
    }
    target->on_dodge( z, 5, 1 );

    return true;
}

bool mattack::boomer_glow( monster *z )
{
    map &here = get_map();

    if( !z->can_act() ) {
        return false;
    }

    Creature *target = z->attack_target();
    if( target == nullptr || rl_dist( z->pos_abs(), target->pos_abs() ) > 3 ||
        !z->sees( here, *target ) ) {
        return false;
    }

    std::vector<tripoint_bub_ms> line = here.find_clear_path( z->pos_bub(), target->pos_bub() );
    // It takes a while
    z->mod_moves( -to_moves<int>( 1_seconds ) * 2.5 );
    Character &player_character = get_player_character();
    bool u_see = player_character.sees( here,  *z );
    if( u_see ) {
        add_msg( m_warning, _( "The %s spews bile!" ), z->name() );
    }
    for( tripoint_bub_ms &i : line ) {
        here.add_field( i, fd_bile, 1 );
        if( here.impassable( i ) ) {
            here.add_field( i, fd_bile, 3 );
            add_msg_if_player_sees( i, _( "Bile splatters on the %s!" ), here.tername( i ) );
            return true;
        }
    }

    if( !target->dodge_check( z, 1.0f ) ) {
        target->add_env_effect( effect_boomered, bodypart_id( "eyes" ), 5, 25_turns );
        target->on_dodge( z, 5, 1.0f );
        for( int i = 0; i < rng( 2, 4 ); i++ ) {
            const bodypart_id &bp = target->random_body_part();
            target->add_env_effect( effect_glowing, bp, 4, 4_minutes );
            if( target->has_effect( effect_glowing ) ) {
                break;
            }
        }
    } else {
        target->add_msg_player_or_npc( _( "You dodge it!" ),
                                       _( "<npcname> dodges it!" ) );
    }

    return true;
}

bool mattack::resurrect( monster *z )
{
    map &here = get_map();

    // Chance to recover some of our missing speed (yes this will regain
    // loses from being revived ourselves as well).
    // Multiplying by (current base speed / max speed) means that the
    // rate of speed regaining is unaffected by what our current speed is, i.e.
    // we will regain the same amount per minute at speed 50 as speed 200.
    if( one_in( static_cast<int>( 15 * static_cast<double>( z->get_speed_base() ) / static_cast<double>
                                  ( z->type->speed ) ) ) ) {
        // Restore 10% of our current speed, capping at our type maximum
        z->set_speed_base( std::min( z->type->speed,
                                     static_cast<int>( z->get_speed_base() + .1 * z->type->speed ) ) );
    }

    int raising_level = 0;
    if( z->has_effect( effect_raising ) ) {
        raising_level = z->get_effect_int( effect_raising ) * 40;
    }

    Character &player_character = get_player_character();
    bool sees_necromancer = player_character.sees( here, *z );
    std::vector<std::pair<tripoint_bub_ms, item *>> corpses;
    // Find all corpses that we can see within 10 tiles.
    int range = 10;
    bool found_eligible_corpse = false;
    int lowest_raise_score = INT_MAX;
    // Cache map stats.
    std::unordered_map<tripoint_bub_ms, bool> sees_and_is_empty_cache;
    auto sees_and_is_empty = [&sees_and_is_empty_cache, &z, &here]( const tripoint_bub_ms & T ) {
        auto iter = sees_and_is_empty_cache.find( T );
        if( iter != sees_and_is_empty_cache.end() ) {
            return iter->second;
        }
        sees_and_is_empty_cache[T] = here.sees( z->pos_bub(), T, -1 ) && g->is_empty( T );
        return sees_and_is_empty_cache[T];
    };
    for( item_location &location : here.get_active_items_in_radius( z->pos_bub(), range,
            special_item_type::corpse ) ) {
        const tripoint_bub_ms &p = location.pos_bub( here );
        item &i = *location;
        const mtype *mt = i.get_mtype();
        if( !( i.can_revive() && i.active && mt->has_flag( mon_flag_REVIVES ) &&
               mt->in_species( species_ZOMBIE ) && !mt->has_flag( mon_flag_NO_NECRO ) ) ) {
            continue;
        }
        if( here.get_field_intensity( p, fd_fire ) > 1 || !sees_and_is_empty( p ) ) {
            continue;
        }

        found_eligible_corpse = true;
        if( raising_level == 0 ) {
            // Since we have a target, start charging to raise it.
            if( sees_necromancer ) {
                add_msg( m_info, _( "The %s throws its arms wide." ), z->name() );
            }
            while( z->get_moves() >= 0 ) {
                z->add_effect( effect_raising, 1_minutes );
                z->mod_moves( -to_moves<int>( 1_seconds ) );
            }
            return false;
        }
        int raise_score = ( i.damage_level() + 1 ) * mt->hp + i.burnt;
        lowest_raise_score = std::min( lowest_raise_score, raise_score );
        if( raise_score <= raising_level ) {
            corpses.emplace_back( p.raw(), &i );
        }
    }

    if( corpses.empty() ) { // No nearby corpses
        if( found_eligible_corpse ) {
            // There was a corpse, but we haven't charged enough.
            if( sees_necromancer && x_in_y( 1, std::sqrt( lowest_raise_score / 30.0 ) ) ) {
                add_msg( m_info, _( "The %s gesticulates wildly." ), z->name() );
            }
            while( z->get_moves() >= 0 ) {
                z->add_effect( effect_raising, 1_minutes );
                z->mod_moves( -to_moves<int>( 1_seconds ) );
                return false;
            }
        } else if( raising_level != 0 ) {
            z->remove_effect( effect_raising );
        }
        // Check to see if there are any nearby living zombies to see if we should get angry
        const bool allies = g->get_creature_if( [&]( const Creature & critter ) {
            const monster *const zed = dynamic_cast<const monster *>( &critter );
            return zed && zed != z && zed->type->has_flag( mon_flag_REVIVES ) &&
                   zed->type->in_species( species_ZOMBIE ) &&
                   z->attitude_to( *zed ) == Creature::Attitude::FRIENDLY  &&
                   within_target_range( z, zed, 10 );
        } );
        if( !allies ) {
            // Nobody around who we could revive, get angry
            z->anger = 100;
        } else {
            // Someone is around who might die and we could revive,
            // calm down.
            z->anger = 5;
        }
        return false;
    } else {
        // We're reviving someone/could revive someone, calm down.
        z->anger = 5;
    }

    if( z->get_speed_base() <= z->type->speed / 2 ) {
        // We can only resurrect so many times in a time period
        // and we're currently out
        return false;
    }

    std::pair<tripoint_bub_ms, item *> raised = random_entry( corpses );
    // To appease static analysis
    cata_assert( raised.second );
    // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
    float corpse_damage = raised.second->damage_level();
    creature_tracker &creatures = get_creature_tracker();
    // Did we successfully raise something?
    if( g->revive_corpse( raised.first, *raised.second ) ) {
        here.i_rem( raised.first, raised.second );
        // check to ensure that we destroy any dormant zombie traps in the same tile.
        if( here.tr_at( raised.first ) == trap_id( "tr_dormant_corpse" ) ) {
            here.remove_trap( raised.first );
        }
        if( sees_necromancer ) {
            add_msg( m_info, _( "You feel a strange pulse of energy from the %s." ), z->name() );
        }
        z->remove_effect( effect_raising );
        // Takes one turn
        // Note: Moves will already be 0 when raised, this purposefully incurs a move deficit.
        z->mod_moves( -z->type->speed );
        // Penalize speed by between 10% and 50% based on how damaged the corpse is.
        float speed_penalty = 0.1 + ( corpse_damage * 0.1 );
        z->set_speed_base( z->get_speed_base() - speed_penalty * z->type->speed );
        monster *const zed = creatures.creature_at<monster>( raised.first );
        if( !zed ) {
            debugmsg( "Misplaced or failed to revive a zombie corpse" );
            return true;
        }

        zed->make_ally( *z );
        if( player_character.sees( here, *zed ) ) {
            add_msg( m_warning, _( "A nearby %s rises from the dead!" ), zed->name() );
        } else if( sees_necromancer ) {
            // We saw the necromancer but not the revival
            add_msg( m_info, _( "But nothing seems to happen." ) );
        }
    }

    return true;
}

void mattack::smash_specific( monster *z, Creature *target )
{
    if( target == nullptr || !z->is_adjacent( target, false ) ) {
        return;
    }
    if( z->has_flag( mon_flag_RIDEABLE_MECH ) ) {
        z->use_mech_power( 5_kJ );
    }
    z->set_dest( target->pos_abs() );
    smash( z );
}

// this has been jsonified and isn't used by brutes any longer
bool mattack::smash( monster *z )
{
    if( !z->can_act() ) {
        return false;
    }

    Creature *target = z->attack_target();
    if( target == nullptr || !z->is_adjacent( target, false ) ) {
        return false;
    }

    //Don't try to smash immobile targets
    if( target->has_flag( mon_flag_IMMOBILE ) ) {
        return false;
    }

    // Costs lots of moves to give you a little bit of a chance to get away.
    z->mod_moves( -to_moves<int>( 4_seconds ) );

    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if( target->dodge_check( z ) ) {
        target->add_msg_player_or_npc( _( "The %s takes a powerful swing at you, but you dodge it!" ),
                                       _( "The %s takes a powerful swing at <npcname>, who dodges it!" ),
                                       z->name() );
        target->on_dodge( z, z->type->melee_skill );
        return true;
    }

    target->add_msg_player_or_npc( _( "A blow from the %1$s sends %2$s flying!" ),
                                   _( "A blow from the %s sends <npcname> flying!" ),
                                   z->name(), target->disp_name() );

    // Release the grabbed creature right before the fling
    z->remove_effect( effect_grabbing );

    // TODO: Make this parabolic
    g->fling_creature( target, coord_to_angle( z->pos_bub(), target->pos_bub() ),
                       z->type->melee_sides * z->type->melee_dice * 3 );

    return true;
}

//--------------------------------------------------------------------------------------------------
// TODO: move elsewhere
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Find empty spaces around origin within a radius of N.
 *
 * @returns a pair with first  = array<tripoint, area>; area = (2*N + 1)^2.
 *                      second = the number of empty spaces found.
 */
template <size_t N = 1>
std::pair < std::array < tripoint_bub_ms, ( 2 * N + 1 ) * ( 2 * N + 1 ) >, size_t >
find_empty_neighbors( const tripoint_bub_ms &origin )
{
    constexpr int r = static_cast<int>( N );

    std::pair < std::array < tripoint_bub_ms, ( 2 * N + 1 )*( 2 * N + 1 ) >, size_t > result;

    for( const tripoint_bub_ms &tmp : get_map().points_in_radius( origin, r ) ) {
        if( g->is_empty( tmp ) ) {
            result.first[result.second++] = tmp;
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Find empty spaces around a creature within a radius of N.
 *
 * @see find_empty_neighbors
 */
template <size_t N = 1>
std::pair < std::array < tripoint_bub_ms, ( 2 * N + 1 ) * ( 2 * N + 1 ) >, size_t >
find_empty_neighbors( const Creature &c )
{
    return find_empty_neighbors<N>( c.pos_bub() );
}

//--------------------------------------------------------------------------------------------------
/**
 * Get a size_t value in the closed interval [0, size]; a convenience to avoid messy casting.
  */
static size_t get_random_index( const size_t size )
{
    return static_cast<size_t>( rng( 0, static_cast<int>( size - 1 ) ) );
}

//--------------------------------------------------------------------------------------------------
/**
 * Get a size_t value in the closed interval [0, c.size() - 1]; a convenience to avoid messy casting.
 */
template <typename Container>
size_t get_random_index( const Container &c )
{
    return get_random_index( c.size() );
}

static bodypart_id body_part_hit_by_plant()
{
    bodypart_id hit;
    if( one_in( 2 ) ) {
        hit = bodypart_id( "leg_l" );
    } else {
        hit = bodypart_id( "leg_r" );
    }
    if( one_in( 4 ) ) {
        hit = bodypart_id( "torso" );
    } else if( one_in( 2 ) ) {
        if( one_in( 2 ) ) {
            hit = bodypart_id( "foot_l" );
        } else {
            hit = bodypart_id( "foot_r" );
        }
    }
    return hit;
}

bool mattack::growplants( monster *z )
{
    map &here = get_map();
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint_bub_ms &p : here.points_in_radius( z->pos_bub(), 3 ) ) {

        // Only affect natural, dirtlike terrain or trees.
        if( !( here.has_flag_ter( ter_furn_flag::TFLAG_DIGGABLE, p ) ||
               here.has_flag_ter( ter_furn_flag::TFLAG_TREE, p ) ||
               here.ter( p ) == ter_t_tree_young ) ) {
            continue;
        }

        if( here.is_bashable( p ) && one_in( 3 ) ) {
            // Destroy everything
            here.destroy( p );
            // And then make the ground fertile
            here.ter_set( p, ter_t_dirtmound );
            continue;
        }

        // 1 in 4 chance to grow a tree
        if( !one_in( 4 ) ) {
            if( one_in( 3 ) ) {
                // If no tree, perhaps underbrush
                here.ter_set( p, ter_t_underbrush );
            }

            continue;
        }

        // Grow a tree and pierce stuff with it
        Creature *critter = creatures.creature_at( p );
        // Don't grow under friends (and self)
        if( critter != nullptr &&
            z->attitude_to( *critter ) == Creature::Attitude::FRIENDLY ) {
            continue;
        }

        here.ter_set( p, ter_t_tree_young );
        if( critter == nullptr || critter->uncanny_dodge() ) {
            continue;
        }

        const bodypart_id &hit = body_part_hit_by_plant();
        critter->add_msg_player_or_npc( m_bad,
                                        //~ %s is bodypart name in accusative.
                                        _( "A tree bursts forth from the earth and pierces your %s!" ),
                                        //~ %s is bodypart name in accusative.
                                        _( "A tree bursts forth from the earth and pierces <npcname>'s %s!" ),
                                        body_part_name_accusative( hit ) );
        critter->deal_damage( z, hit, damage_instance( damage_stab, rng( 10, 30 ) ) );
    }

    // 1 in 5 chance of making existing vegetation grow larger
    if( !one_in( 5 ) ) {
        return true;
    }
    for( const tripoint_bub_ms &p : here.points_in_radius( z->pos_bub(), 5 ) ) {
        const ter_id &ter = here.ter( p );
        if( ter != ter_t_tree_young && ter != ter_t_underbrush ) {
            // Skip as soon as possible to avoid all the checks
            continue;
        }

        Creature *critter = creatures.creature_at( p );
        if( critter != nullptr && z->attitude_to( *critter ) == Creature::Attitude::FRIENDLY ) {
            // Don't buff terrain below friends (and self)
            continue;
        }

        if( ter == ter_t_tree_young ) {
            // Young tree => tree
            // TODO: Make this deal damage too - young tree can be walked on, tree can't
            here.ter_set( p, ter_t_tree );
        } else if( ter == ter_t_underbrush ) {
            // Underbrush => young tree
            here.ter_set( p, ter_t_tree_young );
            if( critter != nullptr && !critter->uncanny_dodge() ) {
                const bodypart_id &hit = body_part_hit_by_plant();
                critter->add_msg_player_or_npc( m_bad,
                                                //~ %s is bodypart name in accusative.
                                                _( "The underbrush beneath your feet grows and pierces your %s!" ),
                                                //~ %s is bodypart name in accusative.
                                                _( "Underbrush grows into a tree, and it pierces <npcname>'s %s!" ),
                                                body_part_name_accusative( hit ) );
                critter->deal_damage( z, hit, damage_instance( damage_stab, rng( 10, 30 ) ) );
            }
        }
    }

    // added during refactor, previously had no cooldown reset
    return true;
}

bool mattack::grow_vine( monster *z )
{
    if( z->friendly ) {
        if( rl_dist( get_player_character().pos_bub(), z->pos_bub() ) <= 3 ) {
            // Friendly vines keep the area around you free, so you can move.
            return false;
        }
    }
    z->mod_moves( -to_moves<int>( 1_seconds ) );
    // Attempt to fill up to 8 surrounding tiles.
    for( int i = 0; i < rng( 1, 8 ); ++i ) {
        if( monster *const vine = g->place_critter_around( mon_creeper_vine, z->pos_bub(), 1 ) ) {
            vine->make_ally( *z );
            // Store position of parent hub in vine goal point.
            vine->set_dest( z->pos_abs() );
        }
    }

    return true;
}

// Return true if the creeper hub that spawned the vine is still there
static bool has_vine_parent( monster *z )
{
    if( !z->has_dest() || !get_map().inbounds( z->get_dest() ) ) {
        return false;
    }
    const monster *parent = get_creature_tracker().creature_at<monster>( z->get_dest() );
    return parent != nullptr && parent->type->id == mon_creeper_hub;
}

bool mattack::vine( monster *z )
{
    if( !has_vine_parent( z ) ) {
        // TODO: Should probably die instead.
        return true;
    }
    int vine_neighbors = 0;
    map &here = get_map();
    creature_tracker &creatures = get_creature_tracker();
    z->mod_moves( -to_moves<int>( 1_seconds ) );
    for( const tripoint_bub_ms &dest : here.points_in_radius( z->pos_bub(), 1 ) ) {
        Creature *critter = creatures.creature_at( dest );
        if( critter != nullptr && z->attitude_to( *critter ) == Creature::Attitude::HOSTILE ) {
            if( critter->uncanny_dodge() ) {
                return true;
            }

            const bodypart_id &bphit = critter->get_random_body_part();
            critter->add_msg_player_or_npc( m_bad,
                                            //~ 1$s monster name(vine), 2$s bodypart in accusative
                                            _( "The %1$s lashes your %2$s!" ),
                                            _( "The %1$s lashes <npcname>'s %2$s!" ),
                                            z->name(),
                                            body_part_name_accusative( bphit ) );
            damage_instance d;
            d.add_damage( damage_cut, 8 );
            d.add_damage( damage_bash, 8 );
            critter->deal_damage( z, bphit, d );
            critter->check_dead_state( &here );
            z->mod_moves( -to_moves<int>( 1_seconds ) );
            return true;
        }

        if( monster *const neighbor = creatures.creature_at<monster>( dest ) ) {
            if( neighbor->type->id == mon_creeper_vine ) {
                vine_neighbors++;
            }
        }
    }
    // Calculate distance from nearest hub
    int dist_from_hub = rl_dist( z->pos_abs(), z->get_dest() );
    if( dist_from_hub > 20 || vine_neighbors > 5 || one_in( 7 - vine_neighbors ) ||
        !one_in( dist_from_hub ) ) {
        return true;
    }
    if( monster *const vine = g->place_critter_around( mon_creeper_vine, z->pos_bub(), 1 ) ) {
        vine->make_ally( *z );
        vine->reset_special( "VINE" );
        // Store position of parent hub in vine goal point.
        vine->set_dest( z->get_dest() );
    }

    return true;
}

bool mattack::spit_sap( monster *z )
{
    const map &here = get_map();

    if( !z->can_act() ) {
        return false;
    }

    Creature *target = z->attack_target();
    if( target == nullptr ||
        rl_dist( z->pos_abs(), target->pos_abs() ) > 12 ||
        !z->sees( here, *target ) ) {
        return false;
    }

    z->mod_moves( -to_moves<int>( 1_seconds ) * 1.5 );

    projectile proj;
    proj.speed = 10;
    proj.range = 12;
    proj.proj_effects.insert( ammo_effect_APPLY_SAP );
    proj.impact.add_damage( damage_acid, rng( 5, 10 ) );
    dealt_projectile_attack atk;
    projectile_attack( atk, proj, z->pos_bub(), target->pos_bub(), dispersion_sources{ 150 }, z );

    return true;
}

bool mattack::triffid_heartbeat( monster *z )
{
    sounds::sound( z->pos_bub(), 14, sounds::sound_t::movement, _( "thu-THUMP." ), true, "misc",
                   "heartbeat" );
    z->mod_moves( -to_moves<int>( 3_seconds ) );
    if( z->friendly != 0 ) {
        return true;
        // TODO: when friendly: open a way to the stairs, don't spawn monsters
    }
    Character &player_character = get_player_character();
    if( player_character.posz() != z->posz() ) {
        // Maybe remove this and allow spawning monsters above?
        return true;
    }

    map &here = get_map();
    const tripoint_bub_ms pos = player_character.pos_bub( here );
    const tripoint_bub_ms z_pos = z->pos_bub( here );

    creature_tracker &creatures = get_creature_tracker();
    static pathfinding_settings root_pathfind( 10, 20, 50, 0, false, false, false, false, false,
            false );
    const pathfinding_target pf_t = pathfinding_target::point( z_pos );
    if( rl_dist( z_pos, pos ) > 5 &&
        !here.route( pos, pf_t, root_pathfind ).empty() ) {
        add_msg( m_warning, _( "The root walls creak around you." ) );
        for( const tripoint_bub_ms &dest : here.points_in_radius( z_pos, 3 ) ) {
            if( g->is_empty( dest ) && one_in( 4 ) ) {
                here.ter_set( dest, ter_t_root_wall );
            } else if( here.ter( dest ) == ter_t_root_wall && one_in( 10 ) ) {
                here.ter_set( dest, ter_t_dirt );
            }
        }
        // Open blank tiles as long as there's no possible route
        int tries = 0;
        while( here.route( pos, pf_t, root_pathfind ).empty() &&
               tries < 20 ) {
            point_bub_ms p( rng( pos.x(), z_pos.x() - 3 ),
                            rng( pos.y(), z_pos.y() - 3 ) );
            tripoint_bub_ms dest( p, z->posz() );
            tries++;
            here.ter_set( dest, ter_t_dirt );
            if( rl_dist( dest, pos ) > 3 && g->num_creatures() < 30 &&
                !creatures.creature_at( dest ) && one_in( 20 ) ) { // Spawn an extra monster
                mtype_id montype = mon_triffid;
                if( one_in( 4 ) ) {
                    montype = mon_creeper_hub;
                } else if( one_in( 3 ) ) {
                    montype = mon_biollante;
                }
                if( monster *const plant = g->place_critter_at( montype, dest ) ) {
                    plant->make_ally( *z );
                }
            }
        }

    } else { // The player is close enough for a fight!

        // Spawn a monster in (about) every second surrounding tile.
        for( int i = 0; i < 4; ++i ) {
            if( monster *const triffid = g->place_critter_around( mon_triffid, z_pos, 1 ) ) {
                triffid->make_ally( *z );
            }
        }
    }

    return true;
}

bool mattack::fungus( monster *z )
{
    // TODO: Infect NPCs?
    // It takes a while
    z->mod_moves( -to_moves<int>( 2_seconds ) );

    //~ the sound of a fungus releasing spores
    sounds::sound( z->pos_bub(), 10, sounds::sound_t::combat, _( "Pouf!" ), false, "misc", "puff" );
    add_msg_if_player_sees( *z, m_warning, _( "Spores are released from the %s!" ), z->name() );

    // Use less laggy methods of reproduction when there is a lot of mons around
    double spore_chance = 0.25;
    int radius = 1;
    if( g->num_creatures() > 25 ) {
        // Number of creatures in the bubble and the resulting average number of spores per "Pouf!":
        // 0-25: 2
        // 50  : 0.5
        // 75  : 0.22
        // 100 : 0.125
        // Assuming all creatures in the bubble were fungaloids (unlikely), the average number of spores per generation:
        // 25  : 50
        // 50  : 25
        // 75  : 17
        // 100 : 13
        spore_chance *= ( 25.0 / g->num_creatures() ) * ( 25.0 / g->num_creatures() );
        if( x_in_y( g->num_creatures(), 100 ) ) {
            // Don't make the increased radius spawn more spores
            const double old_area = ( ( 2 * radius + 1 ) * ( 2 * radius + 1 ) ) - 1;
            radius++;
            const double new_area = ( ( 2 * radius + 1 ) * ( 2 * radius + 1 ) ) - 1;
            spore_chance *= old_area / new_area;
        }
    }

    map &here = get_map();
    fungal_effects fe;
    for( const tripoint_bub_ms &sporep : here.points_in_radius( z->pos_bub(), radius ) ) {
        if( sporep == z->pos_bub() ) {
            continue;
        }
        const int dist = rl_dist( z->pos_bub(), sporep );
        if( !one_in( dist ) ||
            here.impassable( sporep ) ||
            ( dist > 1 && !here.clear_path( z->pos_bub(), sporep, 2, 1, 10 ) ) ) {
            continue;
        }

        fe.fungalize( sporep, z, spore_chance );
    }

    return true;
}

bool mattack::fungus_corporate( monster *z )
{
    map &here = get_map();

    if( x_in_y( 1, 20 ) ) {
        sounds::sound( z->pos_bub(), 10, sounds::sound_t::speech, _( "\"Buy SpOreos™ now!\"" ) );
        if( get_player_view().sees( here, *z ) ) {
            add_msg( m_warning, _( "Delicious snacks are released from the %s!" ), z->name() );
            here.add_item( z->pos_bub(), item( itype_sporeos ) );
        } // only spawns SpOreos if the player is near; can't have the COMMONERS stealing our product from good customers
        return true;
    } else {
        return fungus( z );
    }
}

bool mattack::fungus_haze( monster *z )
{
    //~ That spore sound again
    sounds::sound( z->pos_bub(), 10, sounds::sound_t::combat, _( "Pouf!" ), true, "misc", "puff" );
    add_msg_if_player_sees( *z, m_info, _( "The %s pulses, and fresh fungal material bursts forth." ),
                            z->name() );
    z->mod_moves( -to_moves<int>( 1_seconds ) * 1.5 );
    map &here = get_map();
    for( const tripoint_bub_ms &dest : here.points_in_radius( z->pos_bub(), 3 ) ) {
        here.add_field( dest, fd_fungal_haze, rng( 1, 2 ) );
    }

    return true;
}

bool mattack::fungus_big_blossom( monster *z )
{
    map &here = get_map();

    bool firealarm = false;
    const bool u_see = get_player_view().sees( here, *z );
    // Fungal fire-suppressor! >:D
    for( const tripoint_bub_ms &dest : here.points_in_radius( z->pos_bub(), 6 ) ) {
        if( here.get_field_intensity( dest, fd_fire ) != 0 ) {
            firealarm = true;
        }
        if( firealarm ) {
            here.remove_field( dest, fd_fire );
            here.remove_field( dest, fd_smoke );
            here.add_field( dest, fd_fungal_haze, 3 );
        }
    }
    // Special effects handled outside the loop
    if( firealarm ) {
        if( u_see ) {
            // Sucks up all the smoke
            add_msg( m_warning, _( "The %s suddenly inhales!" ), z->name() );
        }
        //~Sound of a giant fungal blossom inhaling
        sounds::sound( z->pos_bub(), 20, sounds::sound_t::combat, _( "WOOOSH!" ), true, "misc", "inhale" );
        if( u_see ) {
            add_msg( m_bad, _( "The %s discharges an immense flow of spores, smothering the flames!" ),
                     z->name() );
        }
        //~Sound of a giant fungal blossom blowing out the dangerous fire!
        sounds::sound( z->pos_bub(), 20, sounds::sound_t::combat, _( "POUFF!" ), true, "misc", "exhale" );
        return true;
    } else {
        // No fire detected, routine haze-emission
        //~ That spore sound, much louder
        sounds::sound( z->pos_bub(), 15, sounds::sound_t::combat, _( "POUF." ), true, "misc", "puff" );
        if( u_see ) {
            add_msg( m_info, _( "The %s pulses, and fresh fungal material bursts forth!" ), z->name() );
        }
        z->mod_moves( -to_moves<int>( 1_seconds ) * 1.5 );
        for( const tripoint_bub_ms &dest : here.points_in_radius( z->pos_bub(), 12 ) ) {
            here.add_field( dest, fd_fungal_haze, rng( 1, 2 ) );
        }
    }

    return true;
}

bool mattack::fungus_inject( monster *z )
{
    map &here = get_map();
    // For faster copy+paste
    Creature *target = &get_player_character();
    Character &player_character = get_player_character();
    if( rl_dist( z->pos_bub(), player_character.pos_bub() ) > 1 ) {
        return false;
    }

    if( player_character.has_trait( trait_THRESH_MARLOSS ) ||
        player_character.has_trait( trait_THRESH_MYCUS ) ) {
        z->friendly = 1;
        return true;
    }
    if( player_character.has_trait( trait_MARLOSS ) &&
        player_character.has_trait( trait_MARLOSS_BLUE ) &&
        !player_character.crossed_threshold() ) {
        add_msg( m_info, _( "The %s seems to wave you toward the tower…" ), z->name() );
        z->anger = 0;
        return true;
    }
    if( z->friendly ) {
        // TODO: attack other creatures, not just player_character, for now just skip the code below as it
        // only attacks player_character but the monster is friendly.
        return true;
    }
    add_msg( m_warning, _( "The %s jabs at you with a needlelike point!" ), z->name() );
    z->mod_moves( -to_moves<int>( 1_seconds ) * 1.5 );

    bodypart_id hit = target->get_random_body_part();
    damage_instance dam_inst = damage_instance( damage_cut, rng( 5, 11 ) );

    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if( target->dodge_check( z, hit, dam_inst ) ) {
        target->add_msg_player_or_npc( _( "You dodge it!" ),
                                       _( "<npcname> dodges it!" ) );
        target->on_dodge( z, z->type->melee_skill );
        return true;
    }

    target->block_hit( z, hit, dam_inst );

    int dam = player_character.deal_damage( z, hit, dam_inst ).total_damage();
    if( dam > 0 ) {
        //~ 1$s is monster name, 2$s bodypart in accusative
        add_msg( m_bad, _( "The %1$s sinks its point into your %2$s!" ), z->name(),
                 body_part_name_accusative( hit ) );
        // do not fungal infect a bionic limb
        if( !hit->has_flag( json_flag_BIONIC_LIMB ) && one_in( 10 - dam ) ) {
            player_character.add_effect( effect_fungus, 10_minutes, true );
            add_msg( m_warning, _( "You feel thousands of live spores pumping into you…" ) );
        }
    } else {
        //~ 1$s is monster name, 2$s bodypart in accusative
        add_msg( _( "The %1$s strikes your %2$s, but your armor protects you." ), z->name(),
                 body_part_name_accusative( hit ) );
    }

    target->on_hit( &here, z, hit,  z->type->melee_skill );
    player_character.check_dead_state( &here );

    return true;
}

bool mattack::fungus_bristle( monster *z )
{
    map &here = get_map();

    Character &player_character = get_player_character();
    if( player_character.has_trait( trait_THRESH_MARLOSS ) ||
        player_character.has_trait( trait_THRESH_MYCUS ) ) {
        z->friendly = 1;
    }
    Creature *target = z->attack_target();
    if( target == nullptr ||
        !z->is_adjacent( target, true ) ||
        !z->sees( here, *target ) ) {
        return false;
    }

    game_message_type msg_type = target->is_avatar() ? m_warning : m_neutral;

    add_msg( msg_type, _( "The %1$s swipes at %2$s with a barbed tendril!" ), z->name(),
             target->disp_name() );
    z->mod_moves( -to_moves<int>( 1_seconds ) * 1.5 );

    bodypart_id hit = target->get_random_body_part();
    damage_instance dam_inst = damage_instance( damage_cut, rng( 7, 16 ) );

    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if( target->dodge_check( z, hit, dam_inst ) ) {
        target->add_msg_player_or_npc( _( "You dodge it!" ),
                                       _( "<npcname> dodges it!" ) );
        target->on_dodge( z, z->type->melee_skill );
        return true;
    }

    target->block_hit( z, hit, dam_inst );

    int dam = target->deal_damage( z, hit, dam_inst ).total_damage();
    if( dam > 0 ) {
        //~ 1$s is monster name, 2$s bodypart in accusative
        target->add_msg_if_player( m_bad, _( "The %1$s sinks several needlelike barbs into your %2$s!" ),
                                   z->name(), body_part_name_accusative( hit ) );
        // no fungal infection if it is a bionic limb
        if( !hit->has_flag( json_flag_BIONIC_LIMB ) && one_in( 15 - dam ) ) {
            target->add_effect( effect_fungus, 20_minutes, true );
            target->add_msg_if_player( m_warning,
                                       _( "You feel thousands of live spores pumping into you…" ) );
        }
    } else {
        //~ 1$s is monster name, 2$s bodypart in accusative
        target->add_msg_if_player( _( "The %1$s slashes your %2$s, but your armor protects you." ),
                                   z->name(), body_part_name_accusative( hit ) );
    }

    target->on_hit( &here, z, hit,  z->type->melee_skill );

    return true;
}

bool mattack::fungus_growth( monster *z )
{
    add_msg_if_player_sees( *z, m_warning, _( "The %s grows into an adult!" ), z->name() );
    z->poly( mon_fungaloid );

    return false;
}

bool mattack::fungus_sprout( monster *z )
{
    // To avoid map shift weirdness
    bool push_player = false;
    Character &player_character = get_player_character();
    for( const tripoint_bub_ms &dest : get_map().points_in_radius( z->pos_bub(), 1 ) ) {
        if( player_character.pos_bub() == dest ) {
            push_player = true;
        }
        if( monster *const wall = g->place_critter_at( mon_fungal_wall, dest ) ) {
            wall->make_ally( *z );
        }
    }

    if( push_player ) {
        const units::angle angle = coord_to_angle( z->pos_bub(), player_character.pos_bub() );
        add_msg( m_bad, _( "You're shoved away as a fungal wall grows!" ) );
        g->fling_creature( &player_character, angle, rng( 10, 50 ) );
    }

    return true;
}

bool mattack::fungus_fortify( monster *z )
{
    if( z->friendly ) {
        // TODO: handle friendly monsters
        return false;
    }
    Creature *target = &get_player_character();
    Character &player_character = get_player_character();
    bool mycus = false;
    bool peaceful = true;
    //No nifty support effects.  Yet.  This lets it rebuild hedges.
    if( player_character.has_trait( trait_THRESH_MARLOSS ) ||
        player_character.has_trait( trait_THRESH_MYCUS ) ) {
        mycus = true;
    }
    map &here = get_map();
    if( player_character.has_trait( trait_MARLOSS ) &&
        player_character.has_trait( trait_MARLOSS_BLUE ) &&
        !player_character.crossed_threshold() && !mycus ) {
        // You have the other two.  Is it really necessary for us to fight?
        add_msg( m_info, _( "The %s spreads its tendrils.  It seems as though it's expecting you…" ),
                 z->name() );
        if( rl_dist( z->pos_bub(), player_character.pos_bub() ) < 3 ) {
            if( query_yn( _( "The tower extends and aims several tendrils from its depths.  Hold still?" ) ) ) {
                add_msg( m_warning,
                         _( "The %s works several tendrils into your arms, legs, torso, and even neck…" ),
                         z->name() );
                player_character.hurtall( 1, z );
                add_msg( m_warning,
                         _( "You see a clear golden liquid pump through the tendrils--and then lose consciousness." ) );
                player_character.unset_mutation( trait_MARLOSS );
                player_character.unset_mutation( trait_MARLOSS_BLUE );
                player_character.set_mutation( trait_THRESH_MARLOSS );
                here.ter_set( player_character.pos_bub(),
                              ter_t_marloss ); // We only show you the door.  You walk through it on your own.
                get_memorial().add(
                    pgettext( "memorial_male", "Was shown to the Marloss Gateway." ),
                    pgettext( "memorial_female", "Was shown to the Marloss Gateway." ) );
                add_msg( m_good,
                         _( "You wake up in a Marloss bush.  Almost *cradled* in it, actually, as though it grew there for you." ) );
                add_msg( m_good,
                         //~ Beginning to hear the Mycus while conscious: this is it speaking
                         _( "assistance, on an arduous quest.  unity.  together we have reached the door.  now to pass through…" ) );
                return true;
            } else {
                peaceful = false; // You declined the offer.  Fight!
            }
        }
    } else {
        peaceful = false; // You weren't eligible.  Fight!
    }

    bool fortified = false;
    bool push_player = false; // To avoid map shift weirdness
    for( const tripoint_bub_ms &dest : here.points_in_radius( z->pos_bub(), 1 ) ) {
        if( player_character.pos_bub() == dest ) {
            push_player = true;
        }
        if( monster *const wall = g->place_critter_at( mon_fungal_hedgerow, dest ) ) {
            wall->make_ally( *z );
            fortified = true;
        }
    }
    if( push_player ) {
        add_msg( m_bad, _( "You're shoved away as a fungal hedgerow grows!" ) );
        g->fling_creature( &player_character, coord_to_angle( z->pos_bub(), player_character.pos_bub() ),
                           rng( 10,
                                50 ) );
    }
    if( fortified || mycus || peaceful ) {
        return true;
    }

    // TODO: De-playerize the whole block
    const int dist = rl_dist( z->pos_bub(), player_character.pos_bub() );
    if( dist >= 12 ) {
        return false;
    }

    if( dist > 3 ) {
        // Oops, can't reach. ):
        // How's about we spawn more tendrils? :)
        // Aimed at the player, too?  Sure!
        const tripoint_bub_ms hit_pos = target->pos_bub() + point( rng( -1, 1 ), rng( -1, 1 ) );
        if( hit_pos == target->pos_bub() && !target->uncanny_dodge() ) {
            const bodypart_id &hit = body_part_hit_by_plant();
            //~ %s is bodypart name in accusative.
            add_msg( m_bad, _( "A fungal tendril bursts forth from the earth and pierces your %s!" ),
                     body_part_name_accusative( hit ) );
            player_character.deal_damage( z,  hit, damage_instance( damage_cut, rng( 5, 11 ) ) );
            player_character.check_dead_state( &here );
            // Probably doesn't have spores available *just* yet.  Let's be nice.
        } else if( monster *const tendril = g->place_critter_at( mon_fungal_tendril, hit_pos ) ) {
            add_msg( m_bad, _( "A fungal tendril bursts forth from the earth!" ) );
            tendril->make_ally( *z );
        }
        return true;
    }

    add_msg( m_warning, _( "The %s takes aim, and spears at you with a massive tendril!" ),
             z->name() );
    z->mod_moves( -to_moves<int>( 1_seconds ) * 1.5 );

    bodypart_id hit = target->get_random_body_part();
    damage_instance dam_inst = damage_instance( damage_stab, rng( 15, 21 ) );

    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if( target->dodge_check( z, hit, dam_inst ) ) {
        target->add_msg_player_or_npc( _( "You dodge it!" ),
                                       _( "<npcname> dodges it!" ) );
        target->on_dodge( z, z->type->melee_skill );
        return true;
    }

    target->block_hit( z, hit, dam_inst );

    int dam = player_character.deal_damage( z, hit, dam_inst ).total_damage();
    // no way to pump spores into a bionic limb
    if( !hit->has_flag( json_flag_BIONIC_LIMB ) && dam > 0 ) {
        //~ 1$s is monster name, 2$s bodypart in accusative
        add_msg( m_bad, _( "The %1$s sinks its point into your %2$s!" ), z->name(),
                 body_part_name_accusative( hit ) );
        player_character.add_effect( effect_fungus, 40_minutes, true );
        add_msg( m_warning, _( "You feel millions of live spores pumping into you…" ) );
    } else {
        //~ 1$s is monster name, 2$s bodypart in accusative
        add_msg( _( "The %1$s strikes your %2$s, but your armor protects you." ), z->name(),
                 body_part_name_accusative( hit ) );
    }

    target->on_hit( &here, z, hit,  z->type->melee_skill );
    player_character.check_dead_state( &here );
    return true;
}

bool mattack::fungal_trail( monster *z )
{
    fungal_effects fe;
    fe.spread_fungus( z->pos_bub() );
    return false;
}

bool mattack::plant( monster *z )
{
    map &here = get_map();
    fungal_effects fe;
    const tripoint_bub_ms monster_position = z->pos_bub();
    const bool is_fungi = here.has_flag_ter( ter_furn_flag::TFLAG_FUNGUS, monster_position );
    // Spores taking seed and growing into a fungaloid
    fe.spread_fungus( monster_position );
    if( is_fungi && one_in( 10 + g->num_creatures() / 5 ) ) {
        add_msg_if_player_sees( *z, m_warning, _( "The %s takes seed and becomes a young fungaloid!" ),
                                z->name() );

        z->poly( mon_fungaloid_young );
        z->mod_moves( -to_moves<int>( 10_seconds ) ); // It takes a while
        return false;
    } else {
        add_msg_if_player_sees( *z, _( "The %s falls to the ground and bursts!" ), z->name() );
        z->set_hp( 0 );
        // Try fungifying once again
        fe.spread_fungus( monster_position );
        return true;
    }
}

bool mattack::disappear( monster *z )
{
    z->set_hp( 0 );
    return true;
}

bool mattack::depart( monster *z )
{
    map &here = get_map();
    if( z->has_flag( mon_flag_FLIES ) && here.is_outside( z->pos_bub() ) ) {
        add_msg_if_player_sees( *z, m_info, _( "The %s turns to a steady climb before departing." ),
                                z->name() );
    } else {
        add_msg_if_player_sees( *z, m_info, _( "The %s departs." ), z->name() );
    }
    z->no_corpse_quiet = true;
    z->no_extra_death_drops = true;
    z->die( &here, nullptr );
    return true;
}

static void poly_keep_speed( monster &mon, const mtype_id &id )
{
    // Retain old speed after polymorph
    // This prevents blobs regenerating speed through polymorphs
    // and thus replicating indefinitely, covering entire map
    const int old_speed = mon.get_speed_base();
    mon.poly( id );
    mon.set_speed_base( old_speed );
}

static bool blobify( monster &blob, monster &target )
{
    add_msg_if_player_sees( target, m_warning, _( "%s is engulfed by %s!" ),
                            target.disp_name(), blob.disp_name() );
    switch( target.get_size() ) {
        case creature_size::tiny:
            // Just consume it
            target.set_hp( 0 );
            blob.set_speed_base( blob.get_speed_base() + 5 );
            return false;
        case creature_size::small:
            target.poly( mon_blob_small );
            break;
        case creature_size::medium:
            target.poly( mon_blob );
            break;
        case creature_size::large:
            target.poly( mon_blob_large );
            break;
        case creature_size::huge:
            // No polymorphing huge stuff
            target.add_effect( effect_slimed, rng( 2_turns, 10_turns ) );
            break;
        default:
            debugmsg( "Tried to blobify %s with invalid size: %d",
                      target.disp_name(), static_cast<int>( target.get_size() ) );
            return false;
    }

    target.make_ally( blob );
    return true;
}

bool mattack::formblob( monster *z )
{
    if( z->friendly ) {
        // TODO: handle friendly monsters
        return false;
    }

    bool didit = false;
    std::vector<tripoint_bub_ms> pts = closest_points_first( z->pos_bub(), 1, 1 );
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint_bub_ms &dest : pts ) {
        Creature *critter = creatures.creature_at( dest );
        if( critter == nullptr ) {
            if( z->get_speed_base() > mon_blob_small->speed + 35 && rng( 0, 250 ) < z->get_speed_base() ) {
                // If we're big enough, spawn a baby blob.
                shared_ptr_fast<monster> mon = make_shared_fast<monster>( mon_blob_small );
                mon->ammo = mon->type->starting_ammo;
                if( mon->will_move_to( dest ) && mon->know_danger_at( dest ) ) {
                    didit = true;
                    z->set_speed_base( z->get_speed_base() - mon_blob_small->speed );
                    if( monster *const blob = g->place_critter_around( mon, dest, 0 ) ) {
                        blob->make_ally( *z );
                    }
                    break;
                }
            }

            continue;
        }

        if( critter->is_avatar() || critter->is_npc() ) {
            // If we hit the player or some NPC, cover them with slime
            didit = true;
            // TODO: Add some sort of a resistance/dodge roll
            critter->add_effect( effect_slimed, rng( 0_turns, 1_turns * z->get_hp() ) );
            break;
        }

        monster &othermon = *( dynamic_cast<monster *>( critter ) );
        // Hit a monster.  If it's a blob, give it our speed.  Otherwise, blobify it?
        if( othermon.type->in_species( species_SLIME ) ) {
            if( othermon.type->id == mon_blob_brain ) {
                // Brain blobs don't get sped up, they heal at the cost of the other blob.
                // But only if they are hurt badly.
                const int othermon_half_hp = othermon.get_hp_max() / 2;
                if( othermon.get_hp() < othermon_half_hp ) {
                    const int heal_value = std::min( z->get_speed_base(), othermon_half_hp - othermon.get_hp() );
                    othermon.heal( heal_value, true );
                    if( heal_value >= z->get_speed_base() ) {
                        z->set_speed_base( 0 );
                        z->set_hp( 0 );
                        return true;
                    } else {
                        didit = true;
                        z->set_speed_base( z->get_speed_base() - heal_value );
                    }
                }
                continue;
            }

            if( z->get_speed_base() > 40 ) {
                didit = true;
                othermon.set_speed_base( othermon.get_speed_base() + 5 );
                z->set_speed_base( z->get_speed_base() - 5 );
                if( othermon.type->id == mon_blob_small &&
                    othermon.get_speed_base() >= mon_blob_small->speed + 10 ) {
                    poly_keep_speed( othermon, mon_blob );
                } else if( othermon.type->id == mon_blob && othermon.get_speed_base() >= mon_blob->speed + 10 ) {
                    poly_keep_speed( othermon, mon_blob_large );
                }
            } else if( one_in( z->get_speed_base() ) ) {
                othermon.set_speed_base( othermon.get_speed_base() + z->get_speed_base() );
                z->set_speed_base( 0 );
                z->set_hp( 0 );
                if( othermon.type->id == mon_blob_small &&
                    othermon.get_speed_base() >= mon_blob_small->speed + 10 ) {
                    poly_keep_speed( othermon, mon_blob );
                } else if( othermon.type->id == mon_blob && othermon.get_speed_base() >= mon_blob->speed + 10 ) {
                    poly_keep_speed( othermon, mon_blob_large );
                }
                return true;
            }
        } else if( ( othermon.made_of( material_flesh ) ||
                     othermon.made_of( material_veggy ) ||
                     othermon.made_of( material_iflesh ) ) &&
                   rng( 0, z->get_hp() ) > rng( othermon.get_hp() / 2, othermon.get_hp() ) ) {
            didit = blobify( *z, othermon );
        }
    }

    if( didit ) { // We did SOMEthing.
        if( z->type->id == mon_blob && z->get_speed_base() <= mon_blob_small->speed ) {
            // We shrank!
            poly_keep_speed( *z, mon_blob_small );
        } else if( z->type->id == mon_blob_large && z->get_speed_base() <= mon_blob->speed ) {
            // We shrank!
            poly_keep_speed( *z, mon_blob );
        }

        z->set_moves( 0 );
        return true;
    }

    return true; // consider returning false to try again immediately if nothing happened?
}

bool mattack::callblobs( monster *z )
{
    if( z->friendly ) {
        // TODO: handle friendly monsters
        return false;
    }
    // The huge brain blob interposes other blobs between it and any threat.
    // For the moment just target the player, this gets a bit more complicated
    // if we want to deal with NPCS and friendly monsters as well.
    // The strategy is to send about 1/3 of the available blobs after the player,
    // and keep the rest near the brain blob for protection.
    const tripoint_abs_ms enemy = get_player_character().pos_abs();
    const std::vector<tripoint_abs_ms> nearby_points = closest_points_first( z->pos_abs(), 3 );
    std::list<monster *> allies;
    for( monster &candidate : g->all_monsters() ) {
        if( candidate.type->in_species( species_SLIME ) && candidate.type->id != mon_blob_brain ) {
            // Just give the allies consistent assignments.
            // Don't worry about trying to make the orders optimal.
            allies.push_back( &candidate );
        }
    }
    // 1/3 of the available blobs, unless they would fill the entire area near the brain.
    const int num_guards = std::min( allies.size() / 3, nearby_points.size() );
    int guards = 0;
    for( std::list<monster *>::iterator ally = allies.begin();
         ally != allies.end(); ++ally, ++guards ) {
        tripoint_abs_ms post = enemy;
        if( guards < num_guards ) {
            // Each guard is assigned a spot in the nearby_points vector based on their order.
            int assigned_spot = ( nearby_points.size() * guards ) / num_guards;
            post = nearby_points[ assigned_spot ];
        }
        ( *ally )->set_dest( post );
        if( !( *ally )->has_effect( effect_controlled ) ) {
            ( *ally )->add_effect( effect_controlled, 1_turns, true );
        }
    }
    // This is telepathy, doesn't take any moves.

    return true;
}

bool mattack::jackson( monster *z )
{
    // Jackson draws nearby zombies into the dance.
    const std::vector<tripoint_abs_ms> nearby_points = closest_points_first( z->pos_abs(), 3 );
    std::list<monster *> allies;
    for( monster &candidate : g->all_monsters() ) {
        if( candidate.type->in_species( species_ZOMBIE ) && candidate.type->id != mon_zombie_jackson ) {
            // Just give the allies consistent assignments.
            // Don't worry about trying to make the orders optimal.
            allies.push_back( &candidate );
        }
    }
    const int num_dancers = std::min( allies.size(), nearby_points.size() );
    int dancers = 0;
    bool converted = false;
    for( auto ally = allies.begin(); ally != allies.end(); ++ally, ++dancers ) {
        tripoint_abs_ms post = z->pos_abs();
        if( dancers < num_dancers ) {
            // Each dancer is assigned a spot in the nearby_points vector based on their order.
            int assigned_spot = ( nearby_points.size() * dancers ) / num_dancers;
            post = nearby_points[ assigned_spot ];
        }
        if( ( *ally )->type->id != mon_zombie_dancer ) {
            ( *ally )->poly( mon_zombie_dancer );
            converted = true;
        }
        ( *ally )->set_dest( post );
        if( !( *ally )->has_effect( effect_controlled ) ) {
            ( *ally )->add_effect( effect_controlled, 1_turns, true );
        }
    }
    // Did we convert anybody?
    if( converted ) {
        add_msg_if_player_sees( *z, m_warning, _( "The %s lets out a high-pitched cry!" ), z->name() );
    }
    // This is telepathy, doesn't take any moves.
    return true;
}

bool mattack::dance( monster *z )
{
    const map &here = get_map();

    if( get_player_view().sees( here, *z ) ) {
        switch( rng( 1, 10 ) ) {
            case 1:
                add_msg( m_neutral, _( "The %s swings its arms from side to side!" ), z->name() );
                break;
            case 2:
                add_msg( m_neutral, _( "The %s does some fancy footwork!" ), z->name() );
                break;
            case 3:
                add_msg( m_neutral, _( "The %s shrugs its shoulders!" ), z->name() );
                break;
            case 4:
                add_msg( m_neutral, _( "The %s spins in place!" ), z->name() );
                break;
            case 5:
                add_msg( m_neutral, _( "The %s crouches on the ground!" ), z->name() );
                break;
            case 6:
                add_msg( m_neutral, _( "The %s looks left and right!" ), z->name() );
                break;
            case 7:
                add_msg( m_neutral, _( "The %s jumps back and forth!" ), z->name() );
                break;
            case 8:
                add_msg( m_neutral, _( "The %s raises its arms in the air!" ), z->name() );
                break;
            case 9:
                add_msg( m_neutral, _( "The %s swings its hips!" ), z->name() );
                break;
            case 10:
                add_msg( m_neutral, _( "The %s claps!" ), z->name() );
                break;
        }
    }

    return true;
}

bool mattack::dogthing( monster *z )
{
    map &here = get_map();

    if( z == nullptr ) {
        // TODO: replace pointers with references
        return false;
    }

    if( !one_in( 3 ) || !get_player_view().sees( here, *z ) ) {
        return false;
    }

    add_msg( _( "The %s's head explodes in a mass of roiling tentacles!" ),
             z->name() );

    here.add_splash( z->bloodType(), z->pos_bub(), 2, 3 );

    z->friendly = 0;
    z->poly( mon_headless_dog_thing );

    return false;
}

bool mattack::para_sting( monster *z )
{
    const float range = 4.0f;
    Creature *target = sting_get_target( z, range );
    if( target == nullptr ) {
        return false;
    }

    z->mod_moves( -to_moves<int>( 1_seconds ) * 1.5 );

    damage_instance dam = damage_instance();
    dam.add_damage( damage_stab, 6, 8, 0.8, 1 );
    bool hit = sting_shoot( z, target, dam, range );
    if( hit ) {
        target->add_msg_if_player( m_bad, _( "You feel poison enter your body!" ) );
        target->add_effect( effect_paralyzepoison, 5_minutes );
    }

    return true;
}

bool mattack::triffid_growth( monster *z )
{
    add_msg_if_player_sees( *z, m_warning, _( "The %s young triffid grows into an adult!" ),
                            z->name() );
    z->poly( mon_triffid );

    return false;
}

bool mattack::nurse_check_up( monster *z )
{
    bool found_target = false;
    Character *target = nullptr;
    tripoint_bub_ms tmp_pos( z->pos_bub() + point( 12, 12 ) );
    map &here = get_map();
    for( Creature *critter : here.get_creatures_in_radius( z->pos_bub(), 6 ) ) {
        Character *tmp_player = dynamic_cast<Character *>( critter );
        if( tmp_player != nullptr && z->sees( here, *tmp_player ) &&
            here.clear_path( z->pos_bub(), tmp_player->pos_bub(), 10, 0,
                             100 ) ) { // no need to scan players we can't reach
            if( rl_dist( z->pos_bub(), tmp_player->pos_bub() ) < rl_dist( z->pos_bub(), tmp_pos ) ) {
                tmp_pos = tmp_player->pos_bub();
                target = tmp_player;
                found_target = true;
            }
        }
    }
    if( found_target ) {

        // First we offer the check up then we wait to the player to come close
        if( !z->has_effect( effect_countdown ) ) {
            sounds::sound( z->pos_bub(), 8, sounds::sound_t::electronic_speech,
                           string_format(
                               _( "a soft robotic voice say, \"Come here and stand still for a few minutes, I'll give you a check-up.\"" ) ) );
            z->add_effect( effect_countdown, 30_minutes );
        } else if( rl_dist( target->pos_bub(), z->pos_bub() ) > 1 ) {
            // Giving them some encouragement
            sounds::sound( z->pos_bub(), 8, sounds::sound_t::electronic_speech,
                           string_format(
                               _( "a soft robotic voice say, \"Come on.  I don't bite, I promise it won't hurt one bit.\"" ) ) );
        } else {
            sounds::sound( z->pos_bub(), 8, sounds::sound_t::electronic_speech,
                           string_format(
                               _( "a soft robotic voice say, \"Here we go.  Just hold still.\"" ) ) );
            if( target->is_avatar() ) {
                add_msg( m_good, _( "You get a medical check-up." ) );
            }
            target->add_effect( effect_got_checked, 10_turns );
            z->remove_effect( effect_countdown );
        }
        return true;
    }
    return false;
}
bool mattack::nurse_assist( monster *z )
{
    const map &here = get_map();

    const bool u_see = get_player_view().sees( here, *z );

    if( u_see && one_in( 100 ) ) {
        add_msg( m_info, _( "The %s is scanning its surroundings." ), z->name() );
    }

    bool found_target = false;
    Character *target = nullptr;
    tripoint_bub_ms tmp_pos( z->pos_bub() + point( 12, 12 ) );
    for( Creature *critter : here.get_creatures_in_radius( z->pos_bub(), 6 ) ) {
        Character *tmp_player = dynamic_cast<Character *>( critter );
        // No need to scan players we can't reach
        if( tmp_player != nullptr && z->sees( here, *tmp_player ) &&
            here.clear_path( z->pos_bub(), tmp_player->pos_bub(), 10, 0, 100 ) ) {
            if( rl_dist( z->pos_abs(), tmp_player->pos_abs() ) < rl_dist( z->pos_bub(), tmp_pos ) ) {
                tmp_pos = tmp_player->pos_bub();
                target = tmp_player;
                found_target = true;
            }
        }
    }

    if( found_target ) {
        if( target->is_wearing( itype_badge_doctor ) ||
            z->attitude_to( *target ) == Creature::Attitude::FRIENDLY ) {
            sounds::sound( z->pos_bub(), 8, sounds::sound_t::electronic_speech,
                           SNIPPET.expand( target->male
                                           ? _( "a soft robotic voice say, \"Welcome doctor <male_full_name>.  I'll be your assistant today.\"" )
                                           : _( "a soft robotic voice say, \"Welcome doctor <female_full_name>.  I'll be your assistant today.\"" ) ) );
            target->add_effect( effect_assisted, 20_turns, false, 12 );
            return true;
        }
    }
    return false;
}
bool mattack::nurse_operate( monster *z )
{
    const map &here = get_map();

    if( z->has_effect( effect_dragging ) || z->has_effect( effect_operating ) ) {
        return false;
    }
    Character &player_character = get_player_character();
    const bool u_see = player_character.sees( here, *z );

    if( u_see && one_in( 100 ) ) {
        add_msg( m_info, _( "The %s is scanning its surroundings." ), z->name() );
    }

    if( ( ( player_character.is_wearing( itype_badge_doctor ) ||
            z->attitude_to( player_character ) == Creature::Attitude::FRIENDLY ) && u_see ) && one_in( 100 ) ) {

        add_msg( m_info, _( "The %s doesn't seem to register you as a doctor." ), z->name() );
    }

    if( z->ammo[itype_anesthetic] == 0 && u_see ) {
        if( one_in( 100 ) ) {
            add_msg( m_info, _( "The %s looks at its empty anesthesia kit with a dejected look." ), z->name() );
        }
        return false;
    }

    bool found_target = false;
    Character *target = nullptr;
    tripoint_bub_ms tmp_pos( z->pos_bub() + point( 12, 12 ) );
    for( Creature *critter : here.get_creatures_in_radius( z->pos_bub(), 6 ) ) {
        Character *tmp_player = dynamic_cast< Character *>( critter );
        // No need to scan players we can't reach
        if( tmp_player != nullptr && z->sees( here, *tmp_player ) &&
            here.clear_path( z->pos_bub(), tmp_player->pos_bub(), 10, 0, 100 ) ) {
            if( tmp_player->has_any_bionic() ) {
                if( rl_dist( z->pos_abs(), tmp_player->pos_abs() ) < rl_dist( z->pos_bub(), tmp_pos ) ) {
                    tmp_pos = tmp_player->pos_bub();
                    target = tmp_player;
                    found_target = true;
                }
            }
        }
    }
    if( found_target && z->attitude_to( player_character ) == Creature::Attitude::FRIENDLY ) {
        // 50% chance to not turn hostile again
        if( one_in( 2 ) ) {
            return false;
        }
    }
    if( found_target && u_see ) {
        add_msg( m_info, _( "The %1$s scans %2$s and seems to detect something." ), z->name(),
                 target->disp_name() );
    }

    if( found_target ) {

        z->friendly = 0;
        z->anger = 100;
        std::list<tripoint_bub_ms> couch_pos = here.find_furnitures_with_flag_in_radius( z->pos_bub(), 10,
                                               ter_furn_flag::TFLAG_AUTODOC_COUCH );

        if( couch_pos.empty() ) {
            add_msg( m_info, _( "The %s looks for something but doesn't seem to find it." ), z->name() );
            z->anger = 0;
            return false;
        }
        // Should designate target as the attack_target
        z->set_dest( target->pos_abs() );

        // Check if target is already grabbed by something else
        if( target->has_effect( effect_grabbed ) ) {
            for( Creature *critter : here.get_creatures_in_radius( target->pos_bub(), 1 ) ) {
                monster *mon = dynamic_cast<monster *>( critter );
                if( mon != nullptr && mon != z ) {
                    if( mon->type->id != mon_nursebot_defective ) {
                        sounds::sound( z->pos_bub(), 8, sounds::sound_t::electronic_speech,
                                       string_format(
                                           _( "a soft robotic voice say, \"Unhand this patient immediately!  If you keep interfering with the procedure I'll be forced to call law enforcement.\"" ) ) );
                        // Try to push the perpetrator away
                        z->push_to( mon->pos_bub(), 6, 0 );
                    } else {
                        sounds::sound( z->pos_bub(), 8, sounds::sound_t::electronic_speech,
                                       string_format(
                                           _( "a soft robotic voice say, \"Greetings kinbot.  Please take good care of this patient.\"" ) ) );
                        z->anger = 0;
                        // Situation is under control no need to intervene;
                        return false;
                    }
                }
            }
        } else {
            z->type->special_attacks.at( "grab" )->call( *z );
            // Check if we successfully grabbed the target
            if( target->has_effect( effect_grabbed ) ) {
                z->dragged_foe_id = target->getID();
                z->add_effect( effect_dragging, 1_turns, true );
                return true;
            }
        }
        return false;
    }
    z->anger = 0;
    return false;
}
bool mattack::check_money_left( monster *z )
{
    if( !z->has_effect( effect_pet ) ) {
        if( z->friendly == -1 &&
            z->has_effect( effect_paid ) ) { // if the pet effect runs out we're no longer friends
            z->friendly = 0;

            if( !z->inv.empty() ) {
                for( const item &it : z->inv ) {
                    if( it.has_var( "DESTROY_ITEM_ON_MON_DEATH" ) ) {
                        continue;
                    }
                    get_map().add_item_or_charges( z->pos_bub(), it );
                }
                z->inv.clear();
                z->remove_effect( effect_has_bag );
                add_msg( m_info,
                         _( "The %s dumps the contents of its bag on the ground and drops the bag on top of it." ),
                         z->get_name() );
            }

            const SpeechBubble &speech_no_time = get_speech( "mon_grocerybot_friendship_done" );
            sounds::sound( z->pos_bub(), speech_no_time.volume,
                           sounds::sound_t::electronic_speech, speech_no_time.text );
            z->remove_effect( effect_paid );
            return true;
        }
    } else {
        const time_duration time_left = z->get_effect_dur( effect_pet );
        if( time_left < 1_minutes ) {
            if( calendar::once_every( 20_seconds ) ) {
                const SpeechBubble &speech_time_low = get_speech( "mon_grocerybot_running_out_of_friendship" );
                sounds::sound( z->pos_bub(), speech_time_low.volume,
                               sounds::sound_t::electronic_speech, speech_time_low.text );
            }
        }
    }
    if( z->friendly == -1 && !z->has_effect( effect_paid ) ) {
        if( calendar::once_every( 3_hours ) ) {
            const SpeechBubble &speech_override_start = get_speech( "mon_grocerybot_hacked" );
            sounds::sound( z->pos_bub(), speech_override_start.volume,
                           sounds::sound_t::electronic_speech, speech_override_start.text );
        }
    }
    return false;
}
bool mattack::photograph( monster *z )
{
    map &here = get_map();

    if( !within_visual_range( z, 6 ) ) {
        return false;
    }

    Character &player_character = get_player_character();
    const item_location weapon = player_character.get_wielded_item();
    // Badges should NOT be swappable between roles.
    // Hence separate checking.
    // If you are in fact listed as a police officer
    if( player_character.has_trait( trait_PROF_POLICE ) ) {
        // And you're wearing your badge
        if( player_character.is_wearing( itype_badge_deputy ) ) {
            if( one_in( 3 ) ) {
                add_msg( m_info, _( "The %s flashes a LED and departs.  Human officer on scene." ),
                         z->name() );
                z->no_corpse_quiet = true;
                z->no_extra_death_drops = true;
                z->die( &here, nullptr );
                return false;
            } else {
                add_msg( m_info,
                         _( "The %s acknowledges you as an officer responding, but hangs around to watch." ),
                         z->name() );
                add_msg( m_info, _( "Probably some now-obsolete Internal Affairs subroutine…" ) );
                return true;
            }
        }
    }

    if( player_character.has_trait( trait_PROF_PD_DET ) ) {
        // And you have your shield on
        if( player_character.is_wearing( itype_badge_detective ) ) {
            if( one_in( 4 ) ) {
                add_msg( m_info, _( "The %s flashes a LED and departs.  Human officer on scene." ),
                         z->name() );
                z->no_corpse_quiet = true;
                z->no_extra_death_drops = true;
                z->die( &here, nullptr );
                return false;
            } else {
                add_msg( m_info,
                         _( "The %s acknowledges you as an officer responding, but hangs around to watch." ),
                         z->name() );
                add_msg( m_info, _( "Ops used to do that in case you needed backup…" ) );
                return true;
            }
        }
    } else if( player_character.has_trait( trait_PROF_SWAT ) ) {
        // And you're wearing your badge
        if( player_character.is_wearing( itype_badge_swat ) ) {
            if( one_in( 3 ) ) {
                add_msg( m_info, _( "The %s flashes a LED and departs.  SWAT's working the area." ),
                         z->name() );
                z->no_corpse_quiet = true;
                z->no_extra_death_drops = true;
                z->die( &here, nullptr );
                return false;
            } else {
                add_msg( m_info, _( "The %s acknowledges you as SWAT onsite, but hangs around to watch." ),
                         z->name() );
                add_msg( m_info, _( "Probably some now-obsolete Internal Affairs subroutine…" ) );
                return true;
            }
        }
    }

    if( player_character.has_trait( trait_PROF_FED ) ) {
        // And you're wearing your badge
        if( player_character.is_wearing( itype_badge_marshal ) ) {
            add_msg( m_info, _( "The %s flashes a LED and departs.  The Feds got this." ), z->name() );
            z->no_corpse_quiet = true;
            z->no_extra_death_drops = true;
            z->die( &here, nullptr );
            return false;
        }
    }

    if( z->friendly || ( weapon && weapon->typeId() == itype_e_handcuffs ) ) {
        // Friendly (hacked?) bot ignore the player. Arrested suspect ignored too.
        // TODO: might need to be revisited when it can target npcs.
        return false;
    }
    z->mod_moves( -to_moves<int>( 1_seconds ) * 1.5 );
    add_msg( m_warning, _( "The %s takes your picture!" ), z->name() );
    // TODO: Make the player known to the faction
    std::string msg;
    if( one_in( 6 ) ) {
        if( player_character.male ) {
            msg = SNIPPET.expand( _( "a robotic voice boom, \"Citizen <male_full_name>!\"" ) );
        } else {
            msg = SNIPPET.expand( _( "a robotic voice boom, \"Citizen <female_full_name>!\"" ) );
        }
    } else if( one_in( 3 ) ) {
        msg = string_format( _( "a robotic voice boom, \"Citizen %s!\"" ), player_character.name );
    } else {
        msg = _( "a robotic voice boom, \"Citizen… database connection lost!\"" );
    }
    sounds::sound( z->pos_bub(), 15, sounds::sound_t::alert, msg, false, "speech", z->type->id.str() );

    if( player_character.is_armed() ) {
        std::string msg = string_format( _( "\"Drop your %s!  Now!\"" ),
                                         weapon->is_gun() ? _( "gun" ) : _( "weapon" ) );
        sounds::sound( z->pos_bub(), 15, sounds::sound_t::alert, msg );
    }
    const SpeechBubble &speech = get_speech( z->type->id.str() );
    sounds::sound( z->pos_bub(), speech.volume, sounds::sound_t::alert, speech.text.translated() );

    const effect &depleted = z->get_effect( effect_eyebot_depleted );
    const effect &assisted = z->get_effect( effect_eyebot_assisted );
    const bool is_depleted = !depleted.is_null() &&
                             depleted.get_intensity() == depleted.get_max_intensity();
    const bool fully_assisted = !assisted.is_null() &&
                                assisted.get_intensity() == assisted.get_max_intensity();
    if( fully_assisted || is_depleted ) {
        // Only spawn 3 every 6 hours
        // Or stop once this eyebot has spawned 10 bots
        return true;
    }

    get_timed_events().add( timed_event_type::ROBOT_ATTACK, calendar::turn + rng( 15_turns, 30_turns ),
                            0, player_character.pos_abs() );
    z->add_effect( effect_source::empty(), effect_eyebot_assisted, 6_hours );
    z->add_effect( effect_source::empty(), effect_eyebot_depleted, 1_minutes, true, 0 );

    return true;
}

bool mattack::tazer( monster *z )
{
    Creature *target = z->attack_target();
    if( target == nullptr || !z->is_adjacent( target, false ) ) {
        return false;
    }

    taze( z, target );
    return true;
}

void mattack::taze( monster *z, Creature *target )
{
    map &here = get_map();

    // It takes a while
    z->mod_moves( -to_moves<int>( 2_seconds ) );
    if( target == nullptr ) {
        return;
    };

    /** @EFFECT_DODGE increases chance of dodging a tazer attack */
    const bool tazer_was_dodged = target->dodge_check( z );
    const int tazer_resistance = target->get_armor_type( damage_bash,
                                 bodypart_id( "torso" ) );
    const bool tazer_was_armored = dice( 15, 10 ) < dice( tazer_resistance, 10 );

    if( tazer_was_dodged || target->uncanny_dodge() ) {
        target->add_msg_if_player( m_bad, _( "The %s attempts to shock you but you dodge." ),
                                   z->name() );
        return;
    }

    if( tazer_was_armored ) {
        target->add_msg_if_player( m_bad, _( "The %s unsuccessfully attempts to shock you." ),
                                   z->name() );
        return;
    }

    int dam = target->deal_damage( z, bodypart_id( "torso" ), damage_instance( damage_electric,
                                   rng( 1, 5 ) ) ).total_damage();
    if( dam == 0 ) {
        target->add_msg_player_or_npc( _( "The %s unsuccessfully attempts to shock you." ),
                                       _( "The %s unsuccessfully attempts to shock <npcname>." ),
                                       z->name() );
        return;
    }

    game_message_type m_type = target->attitude_to( get_player_character() ) ==
                               Creature::Attitude::FRIENDLY ?
                               m_bad : m_neutral;
    target->add_msg_player_or_npc( m_type,
                                   _( "The %s shocks you!" ),
                                   _( "The %s shocks <npcname>!" ),
                                   z->name() );
    target->check_dead_state( &here );
}

bool mattack::searchlight( monster *z )
{
    map &here = get_map();

    int max_lamp_count = 3;
    if( z->get_hp() < z->get_hp_max() ) {
        max_lamp_count--;
    }
    if( z->get_hp() < z->get_hp_max() / 3 ) {
        max_lamp_count--;
    }

    const tripoint_bub_ms pos = z->pos_bub( here );
    const point_bub_ms zpos( pos.xy() );

    //this searchlight is not initialized
    if( z->inv.empty() ) {

        creature_tracker &creatures = get_creature_tracker();
        for( int i = 0; i < max_lamp_count; i++ ) {

            item settings( itype_processor, calendar::turn_zero );

            settings.set_var( "SL_PREFER_UP", "TRUE" );
            settings.set_var( "SL_PREFER_DOWN", "TRUE" );
            settings.set_var( "SL_PREFER_RIGHT", "TRUE" );
            settings.set_var( "SL_PREFER_LEFT", "TRUE" );

            for( const tripoint_bub_ms &dest : here.points_in_radius( pos, 24 ) ) {
                const monster *const mon = creatures.creature_at<monster>( dest );
                if( mon && mon->type->id == mon_turret_searchlight ) {
                    if( dest.x() < zpos.x() ) {
                        settings.set_var( "SL_PREFER_LEFT", "FALSE" );
                    }
                    if( dest.x() > zpos.x() ) {
                        settings.set_var( "SL_PREFER_RIGHT", "FALSE" );
                    }
                    if( dest.y() < zpos.y() ) {
                        settings.set_var( "SL_PREFER_UP", "FALSE" );
                    }
                    if( dest.y() > zpos.y() ) {
                        settings.set_var( "SL_PREFER_DOWN", "FALSE" );
                    }
                }
            }

            settings.set_var( "SL_SPOT_X", 0 );
            settings.set_var( "SL_SPOT_Y", 0 );
            settings.set_var( "DESTROY_ITEM_ON_MON_DEATH", "TRUE" );

            z->add_item( settings );
        }
    }

    //battery charge from the generator is enough for some time of work
    if( calendar::once_every( 10_minutes ) ) {

        bool generator_ok = false;

        for( int x = zpos.x() - 24; x < zpos.x() + 24; x++ ) {
            for( int y = zpos.y() - 24; y < zpos.y() + 24; y++ ) {
                tripoint_bub_ms dest( x, y, pos.z() );
                if( here.has_flag( ter_furn_flag::TFLAG_ACTIVE_GENERATOR, dest ) ) {
                    generator_ok = true;
                }
            }
        }

        if( !generator_ok ) {
            for( item &settings : z->inv ) {
                settings.set_var( "SL_POWER", "OFF" );
            }

            return true;
        }
    }

    Character &player_character = get_player_character();
    for( int i = 0; i < max_lamp_count; i++ ) {

        item &settings = z->inv[i];

        if( settings.get_var( "SL_POWER" )  == "OFF" ) {
            return true;
        }

        const int rng_dir = rng( 0, 7 );

        if( one_in( 5 ) ) {

            if( !one_in( 5 ) ) {
                settings.set_var( "SL_DIR", rng_dir );
            } else {
                const int rng_pref = rng( 0, 3 );
                static const std::array<std::string, 4> settings_names = {{
                        "SL_PREFER_UP", "SL_PREFER_RIGHT", "SL_PREFER_DOWN", "SL_PREFER_LEFT"
                    }
                };
                if( settings.get_var( settings_names[rng_pref] ) == "TRUE" ) {
                    settings.set_var( "SL_DIR", rng_pref * 2 );
                }
            }
        }

        point_bub_ms p( zpos + point( settings.get_var( "SL_SPOT_X", 0 ), settings.get_var( "SL_SPOT_Y",
                                      0 ) ) );
        point_bub_ms preshift_p = p;
        int shift = 0;

        for( int i = 0; i < rng( 1, 2 ); i++ ) {

            if( !z->sees( here,  player_character ) ) {
                shift = settings.get_var( "SL_DIR", shift );

                switch( shift ) {
                    case 0:
                        p.y()--;
                        break;
                    case 1:
                        p.y()--;
                        p.x()++;
                        break;
                    case 2:
                        p.x()++;
                        break;
                    case 3:
                        p.x()++;
                        p.y()++;
                        break;
                    case 4:
                        p.y()++;
                        break;
                    case 5:
                        p.y()++;
                        p.x()--;
                        break;
                    case 6:
                        p.x()--;
                        break;
                    case 7:
                        p.x()--;
                        p.y()--;
                        break;

                    default:
                        break;
                }

            } else {
                if( p.x() < pos.x() ) {
                    p.x()++;
                }
                if( p.x() > pos.x() ) {
                    p.x()--;
                }
                if( p.y() < pos.y() ) {
                    p.y()++;
                }
                if( p.y() > pos.y() ) {
                    p.y()--;
                }
            }

            if( rl_dist( p, zpos ) > 50 ) {
                if( p.x() > zpos.x() ) {
                    p.x()--;
                }
                if( p.x() < zpos.x() ) {
                    p.x()++;
                }
                if( p.y() > zpos.y() ) {
                    p.y()--;
                }
                if( p.y() < zpos.y() ) {
                    p.y()++;
                }
            }
        }

        tripoint_bub_ms t = tripoint_bub_ms( p, pos.z() );
        if( z->sees( here, t, false, 0 ) ) {
            settings.set_var( "SL_SPOT_X", p.x() - zpos.x() );
            settings.set_var( "SL_SPOT_Y", p.y() - zpos.y() );
        } else {
            // If the searchlight cannot see the location it was going to look at check if
            // the searchlight can see the current location (a door could have closed or
            // a window covered). If the searchlight cannot see the current location then
            // reset the searchlight to search from itself
            t = tripoint_bub_ms( preshift_p, pos.z() );
            if( !z->sees( here, t, false, 0 ) ) {
                settings.set_var( "SL_SPOT_X", 0 );
                settings.set_var( "SL_SPOT_Y", 0 );
                t = pos;
            }
        }

        here.add_field( t, field_type_id( "fd_spotlight" ), 1 );
    }

    return true;
}

bool mattack::copbot( monster *z )
{
    const map &here = get_map();

    Creature *target = z->attack_target();
    if( target == nullptr ) {
        return false;
    }

    // TODO: Make it recognize zeds as human, but ignore animals
    Character *foe = dynamic_cast<Character *>( target );
    bool sees_u = foe != nullptr && z->sees( here, *foe );
    bool cuffed = foe != nullptr && foe->get_wielded_item() &&
                  foe->get_wielded_item()->typeId() == itype_e_handcuffs;
    // Taze first, then ask questions (simplifies later checks for non-humans)
    if( !cuffed && z->is_adjacent( target, true ) ) {
        taze( z, target );
        return true;
    }

    if( rl_dist( z->pos_abs(), target->pos_abs() ) > 2 || foe == nullptr ||
        !z->sees( here, *target ) ) {
        if( one_in( 3 ) ) {
            if( sees_u ) {
                if( foe->unarmed_attack() ) {
                    sounds::sound( z->pos_bub(), 18, sounds::sound_t::alert,
                                   _( "a robotic voice boom, \"Citizen, Halt!\"" ), false, "speech", z->type->id.str() );
                } else if( !cuffed ) {
                    sounds::sound( z->pos_bub(), 18, sounds::sound_t::alert,
                                   _( "a robotic voice boom, \"Please put down your weapon.\"" ), false, "speech", z->type->id.str() );
                }
            } else {
                sounds::sound( z->pos_bub(), 18, sounds::sound_t::alert,
                               _( "a robotic voice boom, \"Come out with your hands up!\"" ), false, "speech", z->type->id.str() );
            }
        } else {
            sounds::sound( z->pos_bub(), 18, sounds::sound_t::alarm,
                           _( "a police siren, whoop WHOOP" ), false, "environment", "police_siren" );
        }
        return true;
    }

    // If cuffed don't attack the player, unless the bot is damaged
    // presumably because of the player's actions
    if( z->get_hp() == z->get_hp_max() ) {
        z->anger = 1;
    } else {
        z->anger = z->type->agro;
    }

    return true;
}

bool mattack::ratking( monster *z )
{
    if( z->friendly ) {
        // TODO: handle friendly monsters
        return false;
    }
    Character &player_character = get_player_character();
    // Disable z-level ratting or it can get silly
    if( rl_dist( z->pos_bub(), player_character.pos_bub() ) > 50 ||
        z->posz() != player_character.posz() ) {
        return false;
    }

    switch( rng( 1, 5 ) ) { // What do we say?
        case 1:
            add_msg( m_warning, _( "\"YOU… ARE FILTH…\"" ) );
            break;
        case 2:
            add_msg( m_warning, _( "\"VERMIN… YOU ARE VERMIN…\"" ) );
            break;
        case 3:
            add_msg( m_warning, _( "\"LEAVE NOW…\"" ) );
            break;
        case 4:
            add_msg( m_warning, _( "\"WE… WILL FEAST… UPON YOU…\"" ) );
            break;
        case 5:
            add_msg( m_warning, _( "\"FOUL INTERLOPER…\"" ) );
            break;
    }
    if( rl_dist( z->pos_bub(), player_character.pos_bub() ) <= 10 ) {
        player_character.add_effect( effect_rat, 3_minutes );
    }

    return true;
}

bool mattack::generator( monster *z )
{
    sounds::sound( z->pos_bub(), 100, sounds::sound_t::activity, "hmmmm" );
    if( calendar::once_every( 1_minutes ) && z->get_hp() < z->get_hp_max() ) {
        z->heal( 1 );
    }

    return true;
}

bool mattack::upgrade( monster *z )
{
    const map &here = get_map();

    std::vector<monster *> targets;
    for( monster &zed : g->all_monsters() ) {
        // Check this first because it is a relatively cheap check
        if( zed.can_upgrade() ) {
            // Then do the more expensive ones
            if( z->attitude_to( zed ) != Creature::Attitude::HOSTILE &&
                within_target_range( z, &zed, 10 ) ) {
                targets.push_back( &zed );
            }
        }
    }
    if( targets.empty() ) {
        // Nobody to upgrade, get MAD!
        z->anger = 100;
        return false;
    } else {
        // We've got zombies to upgrade now, calm down again
        z->anger = 5;
    }

    // Takes one turn
    z->mod_moves( -z->type->speed );

    monster *target = random_entry( targets );

    std::string old_name = target->name();
    viewer &player_view = get_player_view();
    const bool could_see = player_view.sees( here, *target );
    target->hasten_upgrade();
    target->try_upgrade( false );
    const bool can_see = player_view.sees( here, *target );
    if( player_view.sees( here, *z ) ) {
        if( could_see ) {
            add_msg( m_warning,
                     //~ %1$s is the name of the zombie upgrading the other, %2$s is the zombie being upgraded.
                     _( "You feel a sudden, intense burst of energy in the air between the %1$s and the %2$s." ),
                     z->name(), old_name );
        } else {
            add_msg( m_warning, _( "You feel a sudden, intense burst of energy from the %s." ), z->name() );
        }
    }
    if( target->name() != old_name ) {
        if( could_see && can_see ) {
            //~ %1$s is the pre-upgrade monster, %2$s is the post-upgrade monster.
            add_msg( m_warning, _( "The %1$s becomes a %2$s!" ), old_name,
                     target->name() );
        } else if( could_see ) {
            add_msg( m_warning, _( "The %s vanishes!" ), old_name );
        } else if( can_see ) {
            add_msg( m_warning, _( "A %s appears!" ), target->name() );
        }
    }

    return true;
}

bool mattack::breathe( monster *z )
{
    // It takes a while
    z->mod_moves( -to_moves<int>( 1_seconds ) );
    bool old_breather_type = z->type->id == mon_breather || z->type->id == mon_breather_hub;
    mtype_id breather_type = old_breather_type ? mon_breather : mon_amalgamation_breather;
    mtype_id hub_type = old_breather_type ? mon_breather_hub : mon_amalgamation_breather_hub;
    int spawn_radius = old_breather_type ? 3 : 2;

    bool able = z->type->id == hub_type;
    creature_tracker &creatures = get_creature_tracker();
    if( !able ) {
        for( const tripoint_bub_ms &dest : get_map().points_in_radius( z->pos_bub(), spawn_radius ) ) {
            monster *const mon = creatures.creature_at<monster>( dest );
            if( mon && mon->type->id == hub_type ) {
                able = true;
                break;
            }
        }
    }
    if( !able ) {
        return true;
    }

    if( monster *const spawned = g->place_critter_around( breather_type, z->pos_bub(), 1 ) ) {
        spawned->reset_special( "BREATHE" );
        spawned->make_ally( *z );
    }

    return true;
}

bool mattack::brandish( monster *z )
{
    const map &here = get_map();

    if( z->friendly ) {
        // TODO: handle friendly monsters
        return false;
    }
    // Only brandish if we can see you!
    if( !z->sees( here, get_player_character() ) ) {
        return false;
    }
    add_msg( m_warning, _( "He's brandishing a knife!" ) );
    add_msg( _( "Quiet, quiet" ) );

    return true;
}

bool mattack::flesh_golem( monster *z )
{
    map &here = get_map();

    if( !z->can_act() ) {
        return false;
    }

    Creature *target = z->attack_target();
    if( target == nullptr ) {
        return false;
    }

    int dist = rl_dist( z->pos_abs(), target->pos_abs() );
    if( dist > 20 ||
        !z->sees( here, *target ) ) {
        return false;
    }

    if( dist > 1 ) {
        if( one_in( 12 ) ) {
            z->mod_moves( -to_moves<int>( 2_seconds ) );
            // It doesn't "nearly deafen you" when it roars from the other side of bubble
            sounds::sound( z->pos_bub( here ), 80, sounds::sound_t::alert, _( "a terrifying roar!" ), false,
                           "shout",
                           "roar" );
            return true;
        }
        return false;
    }
    if( !z->is_adjacent( target, true ) ) {
        // No attacking through floor, even if we can see the target somehow
        return false;
    }
    add_msg_if_player_sees( *z, _( "The %1$s swings a massive claw at %2$s!" ),
                            z->name(), target->disp_name() );
    z->mod_moves( -to_moves<int>( 1_seconds ) );

    bodypart_id hit = target->get_random_body_part();
    damage_instance dam_inst = damage_instance( damage_bash, rng( 5, 10 ) );

    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if( target->dodge_check( z, hit, dam_inst ) ) {
        target->add_msg_player_or_npc( _( "You dodge it!" ),
                                       _( "<npcname> dodges it!" ) );
        target->on_dodge( z, z->type->melee_skill );
        return true;
    }

    target->block_hit( z, hit, dam_inst );

    int dam = target->deal_damage( z, hit, dam_inst ).total_damage();
    if( one_in( 6 ) ) {
        target->add_effect( effect_downed, 3_minutes );
    }

    //~ 1$s is bodypart name, 2$d is damage value.
    target->add_msg_if_player( m_bad, _( "Your %1$s is battered for %2$d damage!" ),
                               body_part_name( hit ), dam );
    target->on_hit( &here, z, hit,  z->type->melee_skill );

    return true;
}

bool mattack::absorb_meat( monster *z )
{
    //Absorb no more than 1/10th monster's volume, times the volume of a meat chunk
    const int monster_volume = units::to_liter( z->get_volume() );
    const float average_meat_chunk_volume = 0.5f;
    // TODO: dynamically get volume of meat
    const int max_meat_absorbed = monster_volume / 10.0f * average_meat_chunk_volume;
    //For every milliliter of meat absorbed, heal this many HP
    const float meat_absorption_factor = 0.01f;
    Character &player_character = get_player_character();
    map &here = get_map();
    //Search surrounding tiles for meat
    for( const tripoint_bub_ms &p : here.points_in_radius( z->pos_bub(), 1 ) ) {
        map_stack items = here.i_at( p );
        for( item &current_item : items ) {
            const material_id current_item_material = current_item.get_base_material().ident();
            if( current_item_material == material_flesh ||
                current_item_material == material_hflesh ) {
                //We have something meaty! Calculate how much it will heal the monster
                const int ml_of_meat = units::to_milliliter<int>( current_item.volume() );
                const int total_charges = current_item.count();
                const int ml_per_charge = ml_of_meat / total_charges;
                //We have a max size of meat here to avoid absorbing whole corpses.
                if( ml_per_charge > max_meat_absorbed * 1000 ) {
                    add_msg( m_info, _( "The %1$s quivers hungrily in the direction of the %2$s." ), z->name(),
                             current_item.tname() );
                    return false;
                }
                if( current_item.count_by_charges() ) {
                    //Choose a random amount of meat charges to absorb
                    int meat_absorbed = std::min( max_meat_absorbed, rng( 1, total_charges ) );
                    const int hp_to_heal = meat_absorbed * ml_per_charge * meat_absorption_factor;
                    z->heal( hp_to_heal, true );
                    here.use_charges( p, 0, current_item.type->get_id(), meat_absorbed );
                } else {
                    //Only absorb one meaty item
                    int meat_absorbed = 1;
                    const int hp_to_heal = meat_absorbed * ml_per_charge * meat_absorption_factor;
                    z->heal( hp_to_heal, true );
                    here.use_amount( p, 0, current_item.type->get_id(), meat_absorbed );
                }
                if( player_character.sees( here, *z ) ) {
                    add_msg( m_warning, _( "The %1$s absorbs the %2$s, growing larger." ), z->name(),
                             current_item.tname() );
                    add_msg_debug( debugmode::DF_MATTACK, "The %1$s now has %2$s out of %3$s hp", z->name(),
                                   z->get_hp(),
                                   z->get_hp_max() );
                }
                return true;
            }
        }
    }
    return false;
}

bool mattack::lunge( monster *z )
{
    map &here = get_map();

    if( !z->can_act() ) {
        return false;
    }

    Creature *target = z->attack_target();
    if( target == nullptr ) {
        return false;
    }

    int dist = rl_dist( z->pos_abs(), target->pos_abs() );
    if( dist > 20 ||
        !z->sees( here, *target ) ) {
        return false;
    }

    bool seen = get_player_view().sees( here, *z );
    if( dist > 1 ) {
        if( one_in( 5 ) ) {
            // Out of range
            if( dist > 4 || !z->sees( here, *target ) ) {
                return false;
            }
            z->mod_moves( z->get_speed() * 2.0 );
            if( seen ) {
                add_msg( _( "The %1$s lunges for %2$s!" ), z->name(), target->disp_name() );
            }
            return true;
        }
        return false;
    }

    if( !z->is_adjacent( target, false ) ) {
        // No attacking up or down - lunging requires contact
        // There could be a lunge down attack, though
        return false;
    }

    z->mod_moves( -to_moves<int>( 1_seconds ) );

    bodypart_id hit = target->get_random_body_part();
    damage_instance dam_inst = damage_instance( damage_bash, rng( 3, 7 ) );

    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if( target->dodge_check( z, hit, dam_inst ) ) {
        target->add_msg_player_or_npc( _( "The %1$s lunges at you, but you sidestep it!" ),
                                       _( "The %1$s lunges at <npcname>, but they sidestep it!" ), z->name() );
        target->on_dodge( z, z->type->melee_skill );
        return true;
    }

    target->block_hit( z, hit, dam_inst );

    int dam = target->deal_damage( z, hit, dam_inst ).total_damage();
    if( dam > 0 ) {
        game_message_type msg_type = target->is_avatar() ? m_bad : m_warning;
        target->add_msg_player_or_npc( msg_type,
                                       _( "The %1$s lunges at your %2$s, battering it for %3$d damage!" ),
                                       _( "The %1$s lunges at <npcname>'s %2$s, battering it for %3$d damage!" ),
                                       z->name(), body_part_name( hit ), dam );
    } else {
        target->add_msg_player_or_npc( _( "The %1$s lunges at your %2$s, but your armor prevents injury!" ),
                                       _( "The %1$s lunges at <npcname>'s %2$s, but their armor prevents injury!" ),
                                       z->name(),
                                       body_part_name_accusative( hit ) );
    }
    if( one_in( 6 ) ) {
        target->add_effect( effect_downed, 3_turns );
    }
    target->on_hit( &here, z, hit,  z->type->melee_skill );
    target->check_dead_state( &here );
    return true;
}

bool mattack::blow_whistle( monster *z )
{
    const map &here = get_map();

    if( z->friendly ) {
        // TODO: handle friendly monsters
        return false;
    }
    // Only blow whistle if we can see you!
    if( !z->sees( here, get_player_character() ) ) {
        return false;
    }
    add_msg_if_player_sees( *z, m_warning, _( "The %1$s loudly blows their whistle!" ), z->name() );
    sounds::sound( z->pos_bub( here ), 40, sounds::sound_t::alarm, _( "FWEEEET!" ), false, "misc",
                   "whistle" );

    return true;
}

static void parrot_common( monster *parrot )
{
    // It takes a while
    parrot->mod_moves( -to_moves<int>( 1_seconds ) );
    const SpeechBubble &speech = get_speech( parrot->type->id.str() );
    sounds::sound( parrot->pos_bub(), speech.volume, sounds::sound_t::speech, speech.text.translated(),
                   false, "speech", parrot->type->id.str() );
}

bool mattack::parrot( monster *z )
{
    if( z->has_effect( effect_shrieking ) ) {
        sounds::sound( z->pos_bub(), 120, sounds::sound_t::alert, _( "a piercing wail!" ), false, "shout",
                       "wail" );
        z->mod_moves( -to_moves<int>( 1_seconds ) * 0.4 );
        return false;
    } else if( one_in( 20 ) ) {
        parrot_common( z );
        return true;
    }

    return false;
}

bool mattack::parrot_at_danger( monster *parrot )
{
    const map &here = get_map();

    Creature *other = get_creature_tracker().find_reachable( *parrot, [parrot,
    &here]( Creature * creature ) {
        if( !creature->is_hallucination() && one_in( 20 ) ) {
            if( creature->is_avatar() || creature->is_npc() ) {
                Character *character = creature->as_character();
                return character->attitude_to( *parrot ) == Creature::Attitude::HOSTILE &&
                       parrot->sees( here, *character );
            } else {
                monster *monster = creature->as_monster();
                return ( monster->faction->attitude( parrot->faction ) == mf_attitude::MFA_HATE ||
                         ( monster->anger > 0 &&
                           monster->faction->attitude( parrot->faction ) == mf_attitude::MFA_BY_MOOD ) ) &&
                       parrot->sees( here, *monster );
            }
        }
        return false;
    } );

    if( other ) {
        parrot_common( parrot );
        return true;
    }

    return false;
}

bool mattack::darkman( monster *z )
{
    const map &here = get_map();

    if( z->friendly ) {
        // TODO: handle friendly monsters
        return false;
    }
    Character &player_character = get_player_character();
    if( rl_dist( z->pos_abs(), player_character.pos_abs() ) > 40 ) {
        return false;
    }
    if( monster *const shadow = g->place_critter_around( mon_shadow, z->pos_bub( here ), 1 ) ) {
        z->mod_moves( -to_moves<int>( 1_seconds ) * 0.1 );
        shadow->make_ally( *z );
        add_msg_if_player_sees( *z, m_warning, _( "A shadow splits from the %s!" ), z->name() );
    }
    // Won't do the combat stuff unless it can see you
    if( !z->sees( here, player_character ) ) {
        return true;
    }
    // What do we say?
    switch( rng( 1, 7 ) ) {
        case 1:
            add_msg( _( "\"Stop it please.\"" ) );
            break;
        case 2:
            add_msg( _( "\"Let us help you.\"" ) );
            break;
        case 3:
            add_msg( _( "\"We wish you no harm.\"" ) );
            break;
        case 4:
            add_msg( _( "\"Do not fear.\"" ) );
            break;
        case 5:
            add_msg( _( "\"We can help you.\"" ) );
            break;
        case 6:
            add_msg( _( "\"We are friendly.\"" ) );
            break;
        case 7:
            add_msg( _( "\"Please don't.\"" ) );
            break;
    }
    player_character.add_effect( effect_darkness, 1_turns, true );

    return true;
}

bool mattack::slimespring( monster *z )
{
    const map &here = get_map();

    Character &player_character = get_player_character();
    if( rl_dist( z->pos_abs(), player_character.pos_abs() ) > 30 ) {
        return false;
    }

    // This morale buff effect could get spammy
    if( player_character.get_morale_level() <= 1 ) {
        add_msg( m_good, "%s", SNIPPET.random_from_category( "slime_cheers" ).value_or( translation() ) );
        player_character.add_morale( morale_support, 10, 50 );
    }
    // They will stave off loneliness, but aren't a substitute for real friends.
    if( player_character.has_effect( effect_social_dissatisfied ) ) {
        add_msg( m_good, "%s", SNIPPET.random_from_category( "slime_cheers" ).value_or( translation() ) );
        player_character.remove_effect( effect_social_dissatisfied );
    }
    if( rl_dist( z->pos_abs(), player_character.pos_abs() ) <= 3 &&
        z->sees( here, player_character ) ) {
        if( player_character.has_effect( effect_bleed ) ||
            player_character.has_effect( effect_bite ) ) {
            //~ Lowercase is intended: they're small voices.
            add_msg( _( "\"let me help!\"" ) );
            // Yes, your slime microbian(s) handle/don't all Bad Damage at the same time.
            for( const bodypart_id &bp_healed :
                 player_character.get_all_body_parts( get_body_part_flags::only_main ) ) {
                if( player_character.has_effect( effect_bite, bp_healed.id() ) ) {
                    if( one_in( 3 ) ) {
                        player_character.remove_effect( effect_bite, bp_healed );
                        add_msg( m_good, _( "The slime cleans you out!" ) );
                    } else {
                        add_msg( _( "The slime flows over you, but your gouges still ache." ) );
                    }
                }
                if( player_character.has_effect( effect_bleed, bp_healed.id() ) ) {
                    if( one_in( 2 ) ) {
                        effect &e = player_character.get_effect( effect_bleed, bp_healed );
                        e.mod_duration( -e.get_int_dur_factor() * rng( 1, 5 ) );
                        add_msg( m_good, _( "The slime seals up your leaks!" ) );
                    } else {
                        add_msg( _( "The slime flows over you, but your fluids are still leaking." ) );
                    }
                }
            }
        }
    }
    return true;
}

bool mattack::riotbot( monster *z )
{
    map &here = get_map();
    const tripoint_bub_ms z_pos = z->pos_bub( here );

    Creature *target = z->attack_target();
    if( target == nullptr ) {
        return false;
    }

    Character *foe = dynamic_cast<Character *>( target );
    const tripoint_bub_ms target_pos = target->pos_bub( here );

    if( calendar::once_every( 1_minutes ) ) {
        for( const tripoint_bub_ms &dest : here.points_in_radius( z_pos, 4 ) ) {
            if( here.passable( dest ) &&
                here.clear_path( z_pos, dest, 3, 1, 100 ) ) {
                here.add_field( dest, fd_relax_gas, rng( 1, 3 ) );
            }
        }
    }

    // Already arrested?
    // And yes, if the player has no hands, we are not going to arrest them.
    if( foe != nullptr ) {
        item_location foe_weapon = foe->get_wielded_item();

        if( foe_weapon && ( foe_weapon->typeId() == itype_e_handcuffs || !foe->has_two_arms_lifting() ) ) {
            z->anger = 0;

            if( calendar::once_every( 25_turns ) ) {
                sounds::sound( z_pos, 10, sounds::sound_t::electronic_speech,
                               _( "Halt and submit to arrest, citizen!  The police will be here any moment." ), false, "speech",
                               z->type->id.str() );
            }

            return true;
        }
    }

    if( z->anger < z->type->agro ) {
        z->anger += z->type->agro / 20;
        return true;
    }

    const int dist = rl_dist( z_pos, target_pos );

    // We need empty hands to arrest
    if( foe && foe->is_avatar() && !foe->is_armed() ) {

        sounds::sound( z_pos, 15, sounds::sound_t::electronic_speech,
                       _( "Please stay in place, citizen, do not make any movements!" ), false, "speech",
                       z->type->id.str() );

        // We need to come closer and arrest
        if( !z->is_adjacent( foe, false ) ) {
            return true;
        }

        // Strain the atmosphere, forcing the player to wait. Let them feel the power of the law!
        if( !one_in( 10 ) ) {
            foe->add_msg_player_or_npc( _( "The robot carefully scans you." ),
                                        _( "The robot carefully scans <npcname>." ) );
            return true;
        }

        enum {ur_arrest, ur_resist, ur_trick};

        // Arrest!
        uilist amenu;
        amenu.allow_cancel = false;
        amenu.text = _( "The robot orders you to present your hands and be cuffed." );

        amenu.addentry( ur_arrest, true, 'a', _( "Allow yourself to be arrested." ) );
        amenu.addentry( ur_resist, true, 'r', _( "Resist arrest!" ) );
        ///\EFFECT_INT >10 allows and increases chance whether you can feign death to avoid riot bot arrest
        if( foe->int_cur > 12 || ( foe->int_cur > 10 && !one_in( foe->int_cur - 8 ) ) ) {
            amenu.addentry( ur_trick, true, 't', _( "Feign death." ) );
        }

        amenu.query();
        const int choice = amenu.ret;

        if( choice == ur_arrest ) {
            z->anger = 0;

            item handcuffs( itype_e_handcuffs, calendar::turn_zero );
            handcuffs.charges = handcuffs.type->maximum_charges();
            handcuffs.active = true;
            const tripoint_bub_ms foe_pos = foe->pos_bub( here );

            handcuffs.set_var( "HANDCUFFS_X", foe_pos.x() );
            handcuffs.set_var( "HANDCUFFS_Y", foe_pos.y() );

            const bool is_uncanny = foe->has_active_bionic( bio_uncanny_dodge ) &&
                                    foe->get_power_level() > bio_uncanny_dodge.obj().power_trigger &&
                                    !one_in( 3 );
            ///\EFFECT_DEX >13 allows and increases chance to slip out of handcuffs
            const bool is_dex = foe->dex_cur > 13 && !one_in( foe->dex_cur - 11 );

            if( is_uncanny || is_dex ) {

                if( is_uncanny ) {
                    foe->mod_power_level( -bio_uncanny_dodge->power_trigger );
                }

                add_msg( m_good,
                         _( "You deftly slip out of the handcuffs just as the robot closes them.  The robot didn't seem to notice!" ) );
                foe->i_add( handcuffs );
            } else {
                foe->wield( foe->i_add( handcuffs ) );
                item_location wielded = foe->get_wielded_item();
                if( wielded && wielded->type == handcuffs.type ) {
                    wielded->set_flag( flag_NO_UNWIELD );
                }
                foe->mod_moves( -to_moves<int>( 3_seconds ) );
                add_msg( m_bad, _( "The robot puts handcuffs on you." ) );
            }

            sounds::sound( z_pos, 5, sounds::sound_t::electronic_speech,
                           _( "You are under arrest, citizen.  You have the right to remain silent.  If you do not remain silent, anything you say may be used against you in a court of law." ),
                           false, "speech", z->type->id.str() );
            sounds::sound( z_pos, 5, sounds::sound_t::electronic_speech,
                           _( "You have the right to an attorney.  If you cannot afford an attorney, one will be provided at no cost to you.  You may have your attorney present during any questioning." ) );
            sounds::sound( z_pos, 5, sounds::sound_t::electronic_speech,
                           _( "If you do not understand these rights, an officer will explain them in greater detail when taking you into custody." ) );
            sounds::sound( z_pos, 5, sounds::sound_t::electronic_speech,
                           _( "Do not attempt to flee or to remove the handcuffs, citizen.  That can be dangerous to your health." ) );

            z->mod_moves( -to_moves<int>( 3_seconds ) );

            return true;
        }

        bool bad_trick = false;

        if( choice == ur_trick ) {

            ///\EFFECT_INT >10 allows and increases chance of successful feign death
            if( !one_in( foe->int_cur - 10 ) ) {

                add_msg( m_good,
                         _( "You fall to the ground and feign a sudden convulsive attack.  Though you're obviously still alive, the robot cannot tell the difference between your 'attack' and a potentially fatal medical condition.  It backs off, signaling for medical help." ) );

                z->mod_moves( -to_moves<int>( 3_seconds ) );
                z->anger = -rng( 0, 50 );
                return true;
            } else {
                add_msg( m_bad, _( "Your awkward movements do not fool the robot." ) );
                foe->mod_moves( -to_moves<int>( 1_seconds ) );
                bad_trick = true;
            }
        }

        if( ( choice == ur_resist ) || bad_trick ) {

            add_msg( m_bad, _( "The robot sprays tear gas!" ) );
            z->mod_moves( -to_moves<int>( 2_seconds ) );

            for( const tripoint_bub_ms &dest : here.points_in_radius( z_pos, 2 ) ) {
                if( here.passable( dest ) &&
                    here.clear_path( z_pos, dest, 3, 1, 100 ) ) {
                    here.add_field( dest, fd_tear_gas, rng( 1, 3 ) );
                }
            }

            return true;
        }

        return true;
    }

    if( calendar::once_every( 5_turns ) ) {
        sounds::sound( z_pos, 25, sounds::sound_t::electronic_speech,
                       _( "Empty your hands and hold your position, citizen!" ), false, "speech", z->type->id.str() );
    }

    if( dist > 5 && dist < 18 && one_in( 10 ) ) {

        z->mod_moves( -to_moves<int>( 1_seconds ) * 0.5 );

        // Precautionary shot
        int delta = dist / 4 + 1;
        // Precision shot
        if( z->get_hp() < z->get_hp_max() ) {
            delta = 1;
        }

        tripoint_bub_ms dest( target_pos.x() + rng( 0, delta ) - rng( 0, delta ),
                              target_pos.y() + rng( 0, delta ) - rng( 0, delta ),
                              target_pos.z() );

        //~ Sound of a riot control bot using its blinding flash
        sounds::sound( z_pos, 3, sounds::sound_t::combat, _( "fzzzzzt" ), false, "misc", "flash" );

        std::vector<tripoint_bub_ms> traj = line_to( z_pos, dest, 0, 0 );
        for( tripoint_bub_ms &elem : traj ) {
            if( !here.is_transparent( elem ) ) {
                break;
            }
            here.add_field( elem, fd_dazzling, 1 );
        }
        return true;

    }

    return true;
}

bool mattack::evolve_kill_strike( monster *z )
{
    map &here = get_map();

    Creature *target = z->attack_target();
    if( target == nullptr ||
        !z->is_adjacent( target, false ) ||
        !z->sees( here, *target ) ) {
        return false;
    }
    if( !z->can_act() ) {
        return false;
    }

    z->mod_moves( -to_moves<int>( 1_seconds ) );

    damage_instance damage( z->type->melee_damage );
    damage.mult_damage( 1.33f );
    damage.add( damage_instance( damage_stab, dice( z->type->melee_dice, z->type->melee_sides ),
                                 z->type->melee_dice_ap + rng( 5, 15 ), 1.0, 0.5 ) );

    if( target->dodge_check( z, bodypart_id( "torso" ), damage ) ) {
        game_message_type msg_type = target->is_avatar() ? m_warning : m_info;
        target->add_msg_player_or_npc( msg_type, _( "The %s lunges at you, but you dodge!" ),
                                       _( "The %s lunges at <npcname>, but they dodge!" ),
                                       z->name() );

        target->on_dodge( z, z->type->melee_skill * 2 );
        target->add_msg_player_or_npc( msg_type, _( "The %s lunges at you, but you dodge!" ),
                                       _( "The %s lunges at <npcname>, but they dodge!" ),
                                       z->name() );
        return true;
    }
    tripoint_abs_ms const target_pos = target->pos_abs();
    const std::string target_name = target->disp_name();

    int damage_dealt = target->deal_damage( z, bodypart_id( "torso" ), damage ).total_damage();
    if( damage_dealt > 0 ) {
        game_message_type msg_type = target->is_avatar() ? m_bad : m_warning;
        target->add_msg_player_or_npc( msg_type,
                                       _( "The %1$s impales your chest for %2$d damage!" ),
                                       _( "The %1$s impales <npcname>'s chest for %2$d damage!" ),
                                       z->name(), damage_dealt );
    } else {
        target->add_msg_player_or_npc(
            _( "The %1$s attempts to burrow itself into you, but is stopped by your armor!" ),
            _( "The %1$s slashes at <npcname>'s torso, but is stopped by their armor!" ),
            z->name() );
        return true;
    }
    Character &player_character = get_player_character();
    if( target->is_dead_state() && g->is_empty( &here, target_pos ) &&
        target->made_of_any( Creature::cmat_flesh ) ) {
        const std::string old_name = z->name();
        const bool could_see_z = player_character.sees( here, *z );
        z->allow_upgrade();
        z->try_upgrade( false );
        z->setpos( target_pos );
        const std::string upgrade_name = z->name();
        const bool can_see_z_upgrade = player_character.sees( here, *z );
        if( could_see_z && can_see_z_upgrade ) {
            add_msg( m_warning, _( "The %1$s burrows within %2$s corpse and a %3$s emerges from the remains!" ),
                     old_name,
                     target_name, upgrade_name );
        } else if( could_see_z ) {
            add_msg( m_warning, _( "The %1$s burrows within %2$s corpse!" ), old_name, target_name );
        } else if( can_see_z_upgrade ) {
            add_msg( m_warning, _( "A %1$s emerges from %2$s corpse!" ), upgrade_name, target_name );
        }
    }
    return true;
}

bool mattack::leech_spawner( monster *z )
{
    const map &here = get_map();

    const bool u_see = get_player_view().sees( here, *z );
    std::list<monster *> allies;
    for( monster &candidate : g->all_monsters() ) {
        if( candidate.in_species( species_LEECH_PLANT ) && !( candidate.has_flag( mon_flag_IMMOBILE ) ||
                candidate.has_flag( json_flag_CANNOT_MOVE ) ) ) {
            allies.push_back( &candidate );
        }
    }
    if( allies.size() > 45 ) {
        return true;
    }
    const int monsters_spawned = rng( 1, 4 );
    const mtype_id monster_type = one_in( 3 ) ? mon_leech_root_runner : mon_leech_root_drone;
    for( int i = 0; i < monsters_spawned; i++ ) {
        if( monster *const new_mon = g->place_critter_around( monster_type, z->pos_bub( here ), 1 ) ) {
            if( u_see ) {
                add_msg( m_warning,
                         _( "An egg pod ruptures and a %s crawls out from the remains!" ), new_mon->name() );
            }
            if( one_in( 25 ) ) {
                z->poly( mon_leech_stalk );
                if( u_see ) {
                    add_msg( m_warning,
                             _( "Resplendent fronds emerge from the still intact pods!" ) );
                }
            }
        }
    }
    return true;
}

bool mattack::mon_leech_evolution( monster *z )
{
    const bool is_queen = z->has_flag( mon_flag_QUEEN );
    std::list<monster *> queens;
    for( monster &candidate : g->all_monsters() ) {
        if( candidate.in_species( species_LEECH_PLANT ) && candidate.has_flag( mon_flag_QUEEN ) &&
            rl_dist( z->pos_bub(), candidate.pos_bub() ) < 35 ) {
            queens.push_back( &candidate );
        }
    }
    if( !is_queen ) {
        if( queens.empty() ) {
            z->poly( mon_leech_blossom );
            z->set_hp( z->get_hp_max() );
            add_msg_if_player_sees( *z, m_warning, _( "The %s blooms into flowers!" ), z->name() );
        }
    }
    return true;
}

bool mattack::tindalos_teleport( monster *z )
{
    map &here = get_map();

    Creature *target = z->attack_target();
    if( target == nullptr ) {
        return false;
    }
    if( one_in( 7 ) ) {
        if( monster *const afterimage = g->place_critter_around( mon_hound_tindalos_afterimage,
                                        z->pos_bub( here ),
                                        1 ) ) {
            z->mod_moves( -to_moves<int>( 1_seconds ) * 1.4 );
            afterimage->make_ally( *z );
            afterimage->nickname = z->nickname;
            add_msg_if_player_sees( *z, m_warning,
                                    _( "The hound's movements chaotically rewind as a living afterimage splits from it!" ) );
        }
    }
    const int distance_to_target = rl_dist( z->pos_abs(), target->pos_abs() );
    if( distance_to_target > 5 ) {
        const tripoint_bub_ms oldpos = z->pos_bub( here );
        for( const tripoint_bub_ms &dest : here.points_in_radius( target->pos_bub( here ), 4 ) ) {
            if( here.is_cornerfloor( dest ) ) {
                if( g->is_empty( dest ) ) {
                    z->setpos( here, dest );
                    // Not teleporting if it means losing sight of our current target
                    if( z->sees( here, *target ) ) {
                        here.add_field( oldpos, fd_tindalos_rift, 2 );
                        here.add_field( dest, fd_tindalos_rift, 2 );
                        add_msg_if_player_sees( *z, m_bad,
                                                _( "The %s dissipates and reforms itself from the angles in the corner." ), z->name() );
                        return true;
                    }
                }
            }
        }
        // couldn't teleport without losing sight of target
        z->setpos( here, oldpos );
        return true;
    }
    return true;
}

bool mattack::flesh_tendril( monster *z )
{
    map &here = get_map();

    Creature *target = z->attack_target();

    if( target == nullptr || !z->sees( here, *target ) ) {
        if( one_in( 70 ) ) {
            add_msg( _( "The floor trembles underneath your feet." ) );
            z->mod_moves( -to_moves<int>( 2_seconds ) );
            sounds::sound( z->pos_bub(), MAX_VIEW_DISTANCE, sounds::sound_t::alert, _( "a deafening roar!" ),
                           false, "shout",
                           "roar" );
        }
        return false;
    }

    const int distance_to_target = rl_dist( z->pos_abs(), target->pos_abs() );

    // the monster summons stuff to fight you
    if( distance_to_target > 3 && one_in( 12 ) ) {
        mtype_id spawned = mon_zombie_gasbag_crawler;
        if( one_in( 2 ) ) {
            spawned = mon_zombie_gasbag_impaler;
        }
        if( monster *const summoned = g->place_critter_around( spawned, z->pos_bub(), 1 ) ) {
            z->mod_moves( -to_moves<int>( 1_seconds ) );
            summoned->make_ally( *z );
            here.propagate_field( z->pos_bub( here ), fd_gibs_flesh, 75, 1 );
            add_msg_if_player_sees( *z, m_warning, _( "A %s struggles to pull itself free from the %s!" ),
                                    summoned->name(), z->name() );
        }
        return true;
    }

    if( ( distance_to_target == 2 || distance_to_target == 3 ) && one_in( 4 ) ) {
        //it pulls you towards itself and then knocks you away
        bool pulled = z->type->special_attacks.at( "ranged_pull" )->call( *z );
        if( pulled && one_in( 4 ) ) {
            sounds::sound( z->pos_bub(), MAX_VIEW_DISTANCE, sounds::sound_t::alarm, _( "a deafening roar!" ),
                           false, "shout",
                           "roar" );
        }
        return pulled;
    }

    if( distance_to_target <= 1 ) {
        if( one_in( 8 ) ) {
            g->fling_creature( target, coord_to_angle( z->pos_bub(), target->pos_bub() ),
                               z->type->melee_sides * z->type->melee_dice * 3 );
        } else {
            z->type->special_attacks.at( "grab" )->call( *z );
        }
    }

    return false;
}

bool mattack::bio_op_random_biojutsu( monster *z )
{
    const map &here = get_map();

    int choice = 0;
    bool redo = false;

    if( !z->can_act() ) {
        return false;
    }

    Creature *target = z->attack_target();
    if( target == nullptr ||
        !z->is_adjacent( target, false ) ||
        !z->sees( here, *target ) ) {
        return false;
    }

    Character *foe = dynamic_cast< Character * >( target );

    do {
        choice = rng( 1, 3 );
        redo = false;

        // ignore disarm if the target isn't a "player" or isn't armed
        if( choice == 3 && foe != nullptr && !foe->is_armed() ) {
            redo = true;
        }

    } while( redo );

    switch( choice ) {
        case 1:
            bio_op_takedown( z );
            break;
        case 2:
            bio_op_impale( z );
            break;
        case 3:
            bio_op_disarm( z );
            break;
    }

    return true;
}

bool mattack::bio_op_takedown( monster *z )
{
    map &here = get_map();

    if( !z->can_act() ) {
        return false;
    }

    Creature *target = z->attack_target();
    // TODO: Allow drop-takedown form above
    if( target == nullptr ||
        !z->is_adjacent( target, false ) ||
        !z->sees( here, *target ) ) {
        return false;
    }

    bool seen = get_player_view().sees( here, *z );
    Character *foe = dynamic_cast< Character * >( target );
    if( seen ) {
        add_msg( _( "The %1$s mechanically grabs at %2$s!" ), z->name(),
                 target->disp_name() );
    }
    z->mod_moves( -to_moves<int>( 1_seconds ) );

    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if( target->dodge_check( z ) ) {
        target->add_msg_player_or_npc( _( "You dodge it!" ),
                                       _( "<npcname> dodges it!" ) );
        target->on_dodge( z, z->type->melee_skill );
        return true;
    }
    int dam = rng( 3, 9 );
    if( foe == nullptr ) {
        // Handle mons earlier - less to check for
        dam = rng( 6, 18 );
        // Always aim for the torso
        target->deal_damage( z, bodypart_id( "torso" ), damage_instance( damage_bash, dam ) );
        // Two hits - "leg" and torso
        target->deal_damage( z, bodypart_id( "torso" ), damage_instance( damage_bash, dam ) );
        target->add_effect( effect_downed, 3_turns );
        if( seen ) {
            add_msg( _( "%1$s slams %2$s to the ground!" ), z->name(), target->disp_name() );
        }
        target->check_dead_state( &here );
        return true;
    }
    // Yes, it has the CQC bionic.
    bodypart_id hit = bodypart_str_id::NULL_ID();
    if( one_in( 2 ) ) {
        hit = bodypart_id( "leg_l" );
    } else {
        hit = bodypart_id( "leg_r" );
    }
    // Weak kick to start with, knocks you off your footing

    //~ 1$s is monster name, 2$s is bodypart name in accusative, 3$d is damage value.
    target->add_msg_if_player( m_bad, _( "The %1$s kicks your %2$s for %3$d damage…" ),
                               z->name(),
                               body_part_name_accusative( hit ), dam );
    foe->deal_damage( z,  hit, damage_instance( damage_bash, dam ) );
    // At this point, Martial Arts or Tentacle Bracing can make this much less painful
    if( target->has_grab_break_tec() ) {
        Character *pl = dynamic_cast<Character *>( target );
        ///\EFFECT_STR increases chance to avoid being grabbed
        ///\EFFECT_DEX increases chance to avoid being grabbed
        int defender_check = rng( 0, std::max( pl->get_dex(), pl->get_str() ) );
        int attacker_check = rng( 0, z->type->melee_sides + z->type->melee_dice );
        const ma_technique grab_break = pl->martial_arts_data->get_grab_break( *pl );

        if( pl->get_effect_int( effect_stunned ) ) {
            defender_check = defender_check - 2;
        }

        if( pl->get_effect_int( effect_downed ) ) {
            defender_check = defender_check - 2;
        }

        if( defender_check > attacker_check ) {
            target->add_msg_if_player( m_info, grab_break.avatar_message.translated() );
            return true;
        }
    }
    if( !target->is_immune_effect( effect_downed ) ) {
        if( one_in( 4 ) ) {
            hit = bodypart_id( "head" );
            // 50% damage buff for the headshot.
            dam = rng( 9, 21 );
            target->add_msg_if_player( m_bad, _( "and slams you, face first, to the ground for %d damage!" ),
                                       dam );
            foe->deal_damage( z, bodypart_id( "head" ), damage_instance( damage_bash, dam ) );
        } else {
            hit = bodypart_id( "torso" );
            dam = rng( 6, 18 );
            target->add_msg_if_player( m_bad, _( "and slams you to the ground for %d damage!" ), dam );
            foe->deal_damage( z, bodypart_id( "torso" ), damage_instance( damage_bash, dam ) );
        }
        foe->add_effect( effect_downed, 3_turns );
    } else if( !foe->is_armed() ||
               foe->martial_arts_data->selected_has_weapon( foe->get_wielded_item()->typeId() ) ) {
        // Saved by the tentacle-bracing! :)
        hit = bodypart_id( "torso" );
        dam = rng( 3, 9 );
        target->add_msg_if_player( m_bad, _( "and slams you for %d damage!" ), dam );
        foe->deal_damage( z, bodypart_id( "torso" ), damage_instance( damage_bash, dam ) );
    }
    target->on_hit( &here, z, hit,  z->type->melee_skill );
    foe->check_dead_state( &here );

    return true;
}

bool mattack::bio_op_impale( monster *z )
{
    map &here = get_map();

    if( !z->can_act() ) {
        return false;
    }

    Creature *target = z->attack_target();
    if( target == nullptr ||
        !z->is_adjacent( target, false ) ||
        !z->sees( here, *target ) ) {
        return false;
    }

    const bool seen = get_player_view().sees( here, *z );
    Character *foe = dynamic_cast< Character * >( target );
    if( seen ) {
        add_msg( _( "The %1$s mechanically lunges at %2$s!" ), z->name(),
                 target->disp_name() );
    }
    z->mod_moves( -to_moves<int>( 1_seconds ) );

    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if( target->dodge_check( z ) ) {
        target->add_msg_player_or_npc( _( "You dodge it!" ),
                                       _( "<npcname> dodges it!" ) );
        target->on_dodge( z, z->type->melee_skill );
        return true;
    }

    // Yes, it has the CQC bionic.
    int dam = rng( 8, 24 );
    bool do_bleed = false;
    int t_dam;

    if( one_in( 4 ) ) {
        dam = rng( 12, 36 ); // 50% damage buff for the crit.
        do_bleed = true;
    }

    if( foe == nullptr ) {
        // Handle mons earlier - less to check for
        target->deal_damage( z, bodypart_id( "torso" ), damage_instance( damage_stab, dam ) );
        if( do_bleed ) {
            target->add_effect( effect_source( z ), effect_bleed, rng( 3_minutes, 10_minutes ),
                                target->get_random_body_part_of_type( body_part_type::type::torso ) );
        }
        if( seen ) {
            add_msg( _( "The %1$s impales %2$s!" ), z->name(), target->disp_name() );
        }
        target->check_dead_state( &here );
        return true;
    }

    const bodypart_id hit = target->get_random_body_part( true );

    t_dam = foe->deal_damage( z, hit, damage_instance( damage_stab, dam ) ).total_damage();

    target->add_msg_player_or_npc( _( "The %1$s tries to impale your %2$s…" ),
                                   _( "The %1$s tries to impale <npcname>'s %2$s…" ),
                                   z->name(), body_part_name_accusative( hit ) );

    if( t_dam > 0 ) {
        target->add_msg_if_player( m_bad, _( "and deals %d damage!" ), t_dam );

        if( do_bleed ) {
            target->add_effect( effect_source( z ), effect_bleed, rng( 75_turns, 125_turns ), hit );
        }
    } else {
        target->add_msg_player_or_npc( _( "but fails to penetrate your armor!" ),
                                       _( "but fails to penetrate <npcname>'s armor!" ) );
    }

    target->on_hit( &here, z, hit, z->type->melee_skill );
    foe->check_dead_state( &here );

    return true;
}

bool mattack::bio_op_disarm( monster *z )
{
    map &here = get_map();

    if( !z->can_act() ) {
        return false;
    }

    Creature *target = z->attack_target();
    if( target == nullptr ||
        !z->is_adjacent( target, false ) ||
        !z->sees( here, *target ) ) {
        return false;
    }

    const bool seen = get_player_view().sees( here, *z );
    Character *foe = dynamic_cast< Character * >( target );

    // disarm doesn't work on creatures or unarmed targets
    if( foe == nullptr || !foe->is_armed() ) {
        return false;
    }

    if( seen ) {
        add_msg( _( "The %1$s mechanically reaches for %2$s!" ), z->name(),
                 target->disp_name() );
    }
    z->mod_moves( -to_moves<int>( 1_seconds ) );

    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if( target->dodge_check( z ) ) {
        target->add_msg_player_or_npc( _( "You dodge it!" ),
                                       _( "<npcname> dodges it!" ) );
        target->on_dodge( z, z->type->melee_skill );
        return true;
    }

    int mon_stat = z->type->melee_dice * z->type->melee_sides;
    int my_roll = dice( 3, 2 * mon_stat );
    my_roll += dice( 3, z->type->melee_skill );

    /** @ARM_STR increases chance to avoid disarm, primary stat */
    /** @EFFECT_DEX increases chance to avoid disarm, secondary stat */
    /** Grip and reaction scores increase the  chance to avoid/ resist disarm */
    /** @EFFECT_MELEE increases chance to avoid disarm */
    int their_roll = dice( 3, foe->get_limb_score( limb_score_grip ) * ( foe->get_arm_str() +
                           foe->get_dex() ) );
    their_roll += dice( 3, round( foe->get_skill_level( skill_melee ) ) );
    their_roll *= foe->get_limb_score( limb_score_reaction );

    item_location it = foe->get_wielded_item();

    target->add_msg_if_player( m_bad, _( "The zombie grabs your %s…" ), it->tname() );

    if( my_roll >= their_roll && !it->has_flag( flag_NO_UNWIELD ) ) {
        target->add_msg_if_player( m_bad, _( "and throws it to the ground!" ) );
        tripoint_bub_ms tp = foe->pos_bub( here ) + tripoint( rng( -1, 1 ), rng( -1, 1 ), 0 );
        if( !here.can_put_items( tp ) ) {
            tp = foe->pos_bub( here );
        }
        here.add_item_or_charges( tp, foe->i_rem( &*it ) );
    } else {
        target->add_msg_if_player( m_good, _( "but you break its grip!" ) );
    }

    return true;
}

bool mattack::suicide( monster *z )
{
    map &here = get_map();

    Creature *target = z->attack_target();
    if( !within_target_range( z, target, 2 ) ) {
        return false;
    }
    z->die( &here, z );

    return false;
}

bool mattack::kamikaze( monster *z )
{
    map &here = get_map();

    if( z->ammo.empty() ) {
        // We somehow lost our ammo! Toggle this special off so we stop processing
        debugmsg( "Missing ammo in kamikaze special for %s.", z->name() );
        z->disable_special( "KAMIKAZE" );
        return true;
    }

    // To work the kamikaze monster has to be configured as such:
    // It carries one bomb item (bomb_type)
    // The bomb item has "use_action": "transform" that turns the bomb on
    // The transform action "target_timer" is used as delay
    // The bomb item turns into active bomb (act_bomb_type)
    // The active bomb has "countdown_action": "explosion"
    // This explosion is triggered at the end of the kamikaze attack

    // The bomb item
    const itype *bomb_type = item::find_type( z->ammo.begin()->first );
    const itype *act_bomb_type;

    const use_function *usage = bomb_type->get_use( "transform" );
    if( usage == nullptr ) {
        // Invalid item usage, Toggle this special off so we stop processing
        debugmsg( "Invalid bomb transform use in kamikaze special for %s.",
                  z->name() );
        z->disable_special( "KAMIKAZE" );
        return true;
    }
    const iuse_transform *actor = dynamic_cast<const iuse_transform *>( usage->get_actor_ptr() );
    if( actor == nullptr ) {
        // Invalid bomb item, Toggle this special off so we stop processing
        debugmsg( "Invalid bomb type in kamikaze special for %s.", z->name() );
        z->disable_special( "KAMIKAZE" );
        return true;
    }
    act_bomb_type = item::find_type( actor->target );
    int delay = to_seconds<int>( actor->target_timer );

    // HACK: HORRIBLE HACK ALERT! Remove the following code completely once we have working monster inventory processing
    if( z->has_effect( effect_countdown ) ) {
        if( z->get_effect( effect_countdown ).get_duration() == 1_turns ) {
            z->die( &here, nullptr );
            // Timer is out, detonate
            item i_explodes( act_bomb_type, calendar::turn );
            i_explodes.active = true;
            i_explodes.countdown_point = calendar::turn_zero;
            i_explodes.process( here, nullptr, z->pos_bub() );
            return false;
        }
        return false;
    }
    // END HORRIBLE HACK

    // lets assume this is explosion actor without checking
    const use_function *use = &act_bomb_type->countdown_action;

    if( use == nullptr ) {
        // Invalid active bomb item usage, Toggle this special off so we stop processing
        debugmsg( "Invalid active bomb explosion use in kamikaze special for %s.",
                  z->name() );
        z->disable_special( "KAMIKAZE" );
        return true;
    }
    const explosion_iuse *exp_actor = dynamic_cast<const explosion_iuse *>( use->get_actor_ptr() );
    if( exp_actor == nullptr ) {
        // Invalid active bomb item, Toggle this special off so we stop processing
        debugmsg( "Invalid active bomb type in kamikaze special for %s.",
                  z->name() );
        z->disable_special( "KAMIKAZE" );
        return true;
    }

    // Get our blast radius
    int radius = -1;
    if( exp_actor->fields_radius > radius ) {
        radius = exp_actor->fields_radius;
    }
    if( exp_actor->emp_blast_radius > radius ) {
        radius = exp_actor->emp_blast_radius;
    }
    // Extra check here to avoid sqrt if not needed
    if( exp_actor->explosion.power > -1 ) {
        int tmp = static_cast<int>( std::sqrt( static_cast<double>( exp_actor->explosion.power / 4 ) ) );
        if( tmp > radius ) {
            radius = tmp;
        }
    }
    if( exp_actor->explosion.shrapnel.casing_mass > 0 ) {
        // Actual factor is 2 * radius, but figure most pieces of shrapnel will miss
        int tmp = static_cast<int>( std::sqrt( exp_actor->explosion.power ) );
        if( tmp > radius ) {
            radius = tmp;
        }
    }
    // Flashbangs have a max range of 8
    if( exp_actor->do_flashbang && radius < 8 ) {
        radius = 8;
    }
    if( radius <= -1 ) {
        // Not a valid explosion size, toggle this special off to stop processing
        z->disable_special( "KAMIKAZE" );
        return true;
    }

    Creature *target = z->attack_target();
    if( target == nullptr ) {
        return false;
    }
    // Range is (radius + distance they expect to gain on you during the countdown)
    // We double target speed because if the player is walking and then start to run their effective speed doubles
    // .65 factor was determined experimentally to be about the factor required for players to be able to *just barely*
    // outrun the explosion if they drop everything and run.
    float factor = static_cast<float>( z->get_speed() ) / static_cast<float>( target->get_speed() * 2 );
    int range = std::max( 1, static_cast<int>( .65 * ( radius + 1 + factor * delay ) ) );

    // Check if we are in range to begin the countdown
    if( !within_target_range( z, target, range ) ) {
        return false;
    }

    // HACK: HORRIBLE HACK ALERT! Currently uses the target_timer as a pseudo-timer.
    // Once we have proper monster inventory item processing replace the following
    // line with the code below.
    z->add_effect( effect_countdown, 1_turns * delay + 1_turns );
    /* Replacement code here for once we have working monster inventories

    item i_explodes(act_bomb_type->id, 0);
    i_explodes.countdown_point = delay;
    z->add_item(i_explodes);
    z->disable_special("KAMIKAZE");
    */
    // END HORRIBLE HACK

    add_msg_if_player_sees( z->pos_bub(), m_bad, _( "The %s lights up menacingly." ), z->name() );

    return true;
}

struct grenade_helper_struct {
    std::string message;
    int chance = 1;
};

// Returns 0 if this should be retired, 1 if it was successful, and -1 if something went horribly wrong
static int grenade_helper( monster *const z, Creature *const target, const int dist,
                           const int moves, std::map<itype_id, grenade_helper_struct> data )
{
    const map &here = get_map();

    // Can't do anything if we can't act
    if( !z->can_act() ) {
        return 0;
    }
    // Too far or we can't target them
    if( !within_target_range( z, target, dist ) ) {
        return 0;
    }
    // We need an open space for these attacks
    const auto empty_neighbors = find_empty_neighbors( *z );
    const size_t empty_neighbor_count = empty_neighbors.second;
    if( !empty_neighbor_count ) {
        return 0;
    }

    // Find how much ammo we currently have
    int curr_ammo = 0;
    for( const std::pair<const itype_id, int> &amm : z->ammo ) {
        curr_ammo += amm.second;
    }
    if( curr_ammo == 0 ) {
        // We've run out of ammo, get angry and toggle the special off.
        z->anger = 100;
        return -1;
    }

    // Grab all attacks that pass their chance check
    weighted_float_list<itype_id> possible_attacks;
    for( const std::pair<const itype_id, int> &amm : z->ammo ) {
        if( amm.second > 0 ) {
            possible_attacks.add( amm.first, 1.0 / data[amm.first].chance );
        }
    }
    itype_id att = *possible_attacks.pick();

    z->mod_moves( -moves );
    z->ammo[att]--;

    // if the player can see it
    if( get_player_view().sees( here, *z ) ) {
        if( data[att].message.empty() ) {
            add_msg_debug( debugmode::DF_MATTACK, "Invalid ammo message in grenadier special." );
        } else {
            add_msg( m_bad, data[att].message, z->name() );
        }
    }

    // Get our monster type
    const itype *bomb_type = item::find_type( att );
    const use_function *usage = bomb_type->get_use( "place_monster" );
    if( usage == nullptr ) {
        // Invalid bomb item usage, Toggle this special off so we stop processing
        add_msg_debug( debugmode::DF_MATTACK, "Invalid bomb item usage in grenadier special for %s.",
                       z->name() );
        return -1;
    }
    const place_monster_iuse *actor = dynamic_cast<const place_monster_iuse *>
                                      ( usage->get_actor_ptr() );
    if( actor == nullptr ) {
        // Invalid bomb item, Toggle this special off so we stop processing
        add_msg_debug( debugmode::DF_MATTACK, "Invalid bomb type in grenadier special for %s.", z->name() );
        return -1;
    }

    const tripoint_bub_ms where{ empty_neighbors.first[get_random_index( empty_neighbor_count )] };

    if( monster *const hack = g->place_critter_at( actor->mtypeid, where ) ) {
        hack->make_ally( *z );
    }
    return 1;
}

bool mattack::grenadier( monster *const z )
{
    // Build our grenade map
    std::map<itype_id, grenade_helper_struct> grenades;
    // Grenades
    grenades[itype_bot_pacification_hack].message =
        _( "The %s deploys a pacification hack!" );
    // Flashbangs
    grenades[itype_bot_flashbang_hack].message =
        _( "The %s deploys a flashbang hack!" );
    // Gasbombs
    grenades[itype_bot_gasbomb_hack].message =
        _( "The %s deploys a tear gas hack!" );
    // C-4
    grenades[itype_bot_c4_hack].message = _( "The %s buzzes and deploys a C-4 hack!" );
    grenades[itype_bot_c4_hack].chance = 8;

    // Only can actively target the player right now. Once we have the ability to grab targets that we aren't
    // actively attacking change this to use that instead.
    Creature *const target = static_cast<Creature *>( &get_player_character() );
    if( z->attitude_to( *target ) == Creature::Attitude::FRIENDLY ) {
        return false;
    }
    int ret = grenade_helper( z, target, 30, 60, grenades );
    if( ret == -1 ) {
        // Something broke badly, disable our special
        z->disable_special( "GRENADIER" );
    }
    return true;
}

bool mattack::grenadier_elite( monster *const z )
{
    // Build our grenade map
    std::map<itype_id, grenade_helper_struct> grenades;
    // Grenades
    grenades[itype_bot_grenade_hack].message = _( "The %s deploys a grenade hack!" );
    // Flashbangs
    grenades[itype_bot_flashbang_hack].message =
        _( "The %s deploys a flashbang hack!" );
    // Gasbombs
    grenades[itype_bot_gasbomb_hack].message = _( "The %s deploys a tear gas hack!" );
    // C-4
    grenades[itype_bot_c4_hack].message = _( "The %s buzzes and deploys a C-4 hack!" );
    grenades[itype_bot_c4_hack].chance = 8;
    // Mininuke
    grenades[itype_bot_mininuke_hack].message =
        _( "A klaxon blares from %s as it deploys a mininuke hack!" );
    grenades[itype_bot_mininuke_hack].chance = 50;

    // Only can actively target the player right now. Once we have the ability to grab targets that we aren't
    // actively attacking change this to use that instead.
    Creature *const target = static_cast<Creature *>( &get_player_character() );
    if( z->attitude_to( *target ) == Creature::Attitude::FRIENDLY ) {
        return false;
    }
    int ret = grenade_helper( z, target, 30, 60, grenades );
    if( ret == -1 ) {
        // Something broke badly, disable our special
        z->disable_special( "GRENADIER_ELITE" );
    }

    return true;
}

bool mattack::zombie_fuse( monster *z )
{
    map &here = get_map();

    monster *critter = nullptr;
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint_bub_ms &p : here.points_in_radius( z->pos_bub(), 1 ) ) {
        critter = creatures.creature_at<monster>( p );
        if( critter != nullptr && critter->faction == z->faction
            && critter != z && critter->get_size() <= z->get_size() ) {
            break;
        }
        critter = nullptr;
    }

    if( critter == nullptr || ( z->has_effect( effect_grown_of_fuse ) &&
                                ( z->get_hp() + critter->get_hp() > z->get_hp_max() + z->get_effect(
                                      effect_grown_of_fuse ).get_max_intensity() ) ) ) {
        return false;
    }
    add_msg_if_player_sees( *z, _( "The %1$s fuses with the %2$s." ),
                            critter->name(), z->name() );
    z->mod_moves( -to_moves<int>( 2_seconds ) );
    if( z->get_size() < creature_size::huge ) {
        z->add_effect( effect_grown_of_fuse, 10_days, true,
                       std::min( critter->get_hp_max(),
                                 ( 80 * ( critter->get_volume() / 62500_ml ) ) )
                       + z->get_effect( effect_grown_of_fuse ).get_intensity() );
    }
    z->heal( critter->get_hp(), true );
    if( mission::on_creature_fusion( *z, *critter ) ) {
        z->mission_fused.emplace_back( critter->name() );
        z->mission_fused.insert( z->mission_fused.end(),
                                 critter->mission_fused.begin(), critter->mission_fused.end() );
        // let the player know that they still need to kill the fusing monster
        add_msg_if_player_sees( *z, _( "%1$s still seems to be moving inside %2$s…" ),
                                critter->name(), z->name() );
    }
    critter->death_drops = false;
    critter->quiet_death = true;
    critter->die( &here, z );
    return true;
}

bool mattack::doot( monster *z )
{
    z->mod_moves( -to_moves<int>( 3_seconds ) );
    add_msg_if_player_sees( *z, _( "The %s doots its trumpet!" ), z->name() );
    int spooks = 0;
    for( const tripoint_bub_ms &spookyscary : get_map().points_in_radius( z->pos_bub(), 2 ) ) {
        if( !g->is_empty( spookyscary ) ) {
            continue;
        }
        const int dist = rl_dist( z->pos_bub(), spookyscary );
        if( ( one_in( dist + 3 ) || spooks == 0 ) && spooks < 5 ) {
            add_msg_if_player_sees( spookyscary, _( "A spooky skeleton rises from the ground!" ) );
            g->place_critter_at( mon_zombie_skeltal_minion, spookyscary );
            spooks++;
            continue;
        }
    }
    sounds::sound( z->pos_bub(), 200, sounds::sound_t::music, _( "DOOT." ), false, "music_instrument",
                   "trumpet" );
    return true;
}

bool mattack::speaker( monster *z )
{
    sounds::sound( z->pos_bub(), MAX_VIEW_DISTANCE, sounds::sound_t::order,
                   SNIPPET.random_from_category( "speaker_warning" ).value_or( translation() ) );
    return true;
}
