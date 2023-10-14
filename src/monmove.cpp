// Monster movement code; essentially, the AI
#include "monster.h" // IWYU pragma: associated

#include <algorithm>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <list>
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>

#include "behavior.h"
#include "bionics.h"
#include "cached_options.h"
#include "cata_utility.h"
#include "character.h"
#include "colony.h"
#include "creature_tracker.h"
#include "debug.h"
#include "field.h"
#include "field_type.h"
#include "game.h"
#include "game_constants.h"
#include "line.h"
#include "make_static.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "mattack_common.h"
#include "memory_fast.h"
#include "messages.h"
#include "monfaction.h"
#include "monster_oracle.h"
#include "mtype.h"
#include "npc.h"
#include "options.h"
#include "pathfinding.h"
#include "pimpl.h"
#include "rng.h"
#include "scent_map.h"
#include "sounds.h"
#include "string_formatter.h"
#include "tileray.h"
#include "translations.h"
#include "trap.h"
#include "units.h"
#include "vehicle.h"
#include "viewer.h"
#include "vpart_position.h"

static const damage_type_id damage_cut( "cut" );

static const efftype_id effect_bouldering( "bouldering" );
static const efftype_id effect_countdown( "countdown" );
static const efftype_id effect_docile( "docile" );
static const efftype_id effect_downed( "downed" );
static const efftype_id effect_dragging( "dragging" );
static const efftype_id effect_grabbed( "grabbed" );
static const efftype_id effect_harnessed( "harnessed" );
static const efftype_id effect_led_by_leash( "led_by_leash" );
static const efftype_id effect_no_sight( "no_sight" );
static const efftype_id effect_operating( "operating" );
static const efftype_id effect_pacified( "pacified" );
static const efftype_id effect_pushed( "pushed" );
static const efftype_id effect_stumbled_into_invisible( "stumbled_into_invisible" );
static const efftype_id effect_stunned( "stunned" );

static const field_type_str_id field_fd_last_known( "fd_last_known" );

static const flag_id json_flag_GRAB( "GRAB" );
static const flag_id json_flag_GRAB_FILTER( "GRAB_FILTER" );

static const itype_id itype_pressurized_tank( "pressurized_tank" );

static const material_id material_iflesh( "iflesh" );

static const mon_flag_str_id mon_flag_ACIDPROOF( "ACIDPROOF" );
static const mon_flag_str_id mon_flag_ACIDTRAIL( "ACIDTRAIL" );
static const mon_flag_str_id mon_flag_AQUATIC( "AQUATIC" );
static const mon_flag_str_id mon_flag_ATTACKMON( "ATTACKMON" );
static const mon_flag_str_id mon_flag_CAN_OPEN_DOORS( "CAN_OPEN_DOORS" );
static const mon_flag_str_id mon_flag_DRIPS_GASOLINE( "DRIPS_GASOLINE" );
static const mon_flag_str_id mon_flag_DRIPS_NAPALM( "DRIPS_NAPALM" );
static const mon_flag_str_id mon_flag_ELECTRIC( "ELECTRIC" );
static const mon_flag_str_id mon_flag_FIREPROOF( "FIREPROOF" );
static const mon_flag_str_id mon_flag_GROUP_BASH( "GROUP_BASH" );
static const mon_flag_str_id mon_flag_GROUP_MORALE( "GROUP_MORALE" );
static const mon_flag_str_id mon_flag_IMMOBILE( "IMMOBILE" );
static const mon_flag_str_id mon_flag_INSECTICIDEPROOF( "INSECTICIDEPROOF" );
static const mon_flag_str_id mon_flag_KEENNOSE( "KEENNOSE" );
static const mon_flag_str_id mon_flag_KEEP_DISTANCE( "KEEP_DISTANCE" );
static const mon_flag_str_id mon_flag_LOUDMOVES( "LOUDMOVES" );
static const mon_flag_str_id mon_flag_NO_BREATHE( "NO_BREATHE" );
static const mon_flag_str_id mon_flag_PACIFIST( "PACIFIST" );
static const mon_flag_str_id mon_flag_PATH_AVOID_DANGER_1( "PATH_AVOID_DANGER_1" );
static const mon_flag_str_id mon_flag_PATH_AVOID_DANGER_2( "PATH_AVOID_DANGER_2" );
static const mon_flag_str_id mon_flag_PATH_AVOID_FALL( "PATH_AVOID_FALL" );
static const mon_flag_str_id mon_flag_PATH_AVOID_FIRE( "PATH_AVOID_FIRE" );
static const mon_flag_str_id mon_flag_PRIORITIZE_TARGETS( "PRIORITIZE_TARGETS" );
static const mon_flag_str_id mon_flag_PUSH_MON( "PUSH_MON" );
static const mon_flag_str_id mon_flag_PUSH_VEH( "PUSH_VEH" );
static const mon_flag_str_id mon_flag_QUEEN( "QUEEN" );
static const mon_flag_str_id mon_flag_RIDEABLE_MECH( "RIDEABLE_MECH" );
static const mon_flag_str_id mon_flag_SEES( "SEES" );
static const mon_flag_str_id mon_flag_SHORTACIDTRAIL( "SHORTACIDTRAIL" );
static const mon_flag_str_id mon_flag_SLUDGETRAIL( "SLUDGETRAIL" );
static const mon_flag_str_id mon_flag_SMALLSLUDGETRAIL( "SMALLSLUDGETRAIL" );
static const mon_flag_str_id mon_flag_SMELLS( "SMELLS" );
static const mon_flag_str_id mon_flag_STUMBLES( "STUMBLES" );
static const mon_flag_str_id mon_flag_SUNDEATH( "SUNDEATH" );
static const mon_flag_str_id mon_flag_SWARMS( "SWARMS" );
static const mon_flag_str_id mon_flag_WEBWALK( "WEBWALK" );

static const species_id species_FUNGUS( "FUNGUS" );
static const species_id species_ZOMBIE( "ZOMBIE" );

bool monster::is_immune_field( const field_type_id &fid ) const
{
    if( fid == fd_fungal_haze ) {
        return has_flag( mon_flag_NO_BREATHE ) || type->in_species( species_FUNGUS );
    }
    if( fid == fd_fungicidal_gas ) {
        return !type->in_species( species_FUNGUS );
    }
    if( fid == fd_insecticidal_gas ) {
        return !made_of( material_iflesh ) || has_flag( mon_flag_INSECTICIDEPROOF );
    }
    if( fid == fd_web ) {
        return has_flag( mon_flag_WEBWALK );
    }
    if( fid == fd_sludge || fid == fd_sap ) {
        return flies();
    }
    const field_type &ft = fid.obj();
    if( ft.has_fume ) {
        return has_flag( mon_flag_NO_BREATHE );
    }
    if( ft.has_acid ) {
        return has_flag( mon_flag_ACIDPROOF ) || flies();
    }
    if( ft.has_fire ) {
        return has_flag( mon_flag_FIREPROOF );
    }
    if( ft.has_elec ) {
        return has_flag( mon_flag_ELECTRIC );
    }
    if( ft.immune_mtypes.count( type->id ) > 0 ) {
        return true;
    }
    // No specific immunity was found, so fall upwards
    return Creature::is_immune_field( fid );
}

static bool z_is_valid( int z )
{
    return z >= -OVERMAP_DEPTH && z <= OVERMAP_HEIGHT;
}

bool monster::will_move_to( const tripoint &p ) const
{
    map &here = get_map();
    if( here.impassable( p ) ) {
        if( digging() ) {
            if( !here.has_flag( ter_furn_flag::TFLAG_BURROWABLE, p ) ) {
                return false;
            }
        } else if( !( can_climb() && here.has_flag( ter_furn_flag::TFLAG_CLIMBABLE, p ) ) ) {
            return false;
        }
    }

    if( !here.has_vehicle_floor( p ) ) {
        if( ( !can_submerge() && !flies() ) && here.has_flag( ter_furn_flag::TFLAG_DEEP_WATER, p ) ) {
            return false;
        }
    }

    if( digs() && !here.has_flag( ter_furn_flag::TFLAG_DIGGABLE, p ) &&
        !here.has_flag( ter_furn_flag::TFLAG_BURROWABLE, p ) ) {
        return false;
    }

    if( has_flag( mon_flag_AQUATIC ) && (
            !here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, p ) ||
            // AQUATIC (confined to water) monster avoid vehicles, unless they are already underneath one
            ( here.veh_at( p ) && !here.veh_at( pos() ) )
        ) ) {
        return false;
    }

    if( has_flag( mon_flag_SUNDEATH ) && g->is_in_sunlight( p ) ) {
        return false;
    }

    if( get_size() > creature_size::medium &&
        here.has_flag_ter( ter_furn_flag::TFLAG_SMALL_PASSAGE, p ) ) {
        return false; // if a large critter, can't move through tight passages
    }

    return true;
}

bool monster::know_danger_at( const tripoint &p ) const
{
    map &here = get_map();

    // Various avoiding behaviors.

    bool avoid_fire = has_flag( mon_flag_PATH_AVOID_FIRE );
    bool avoid_fall = has_flag( mon_flag_PATH_AVOID_FALL );
    bool avoid_simple = has_flag( mon_flag_PATH_AVOID_DANGER_1 );
    bool avoid_complex = has_flag( mon_flag_PATH_AVOID_DANGER_2 );
    bool avoid_sharp = get_pathfinding_settings().avoid_sharp;
    bool avoid_traps = get_pathfinding_settings().avoid_traps;
    /*
     * Because some avoidance behaviors are supersets of others,
     * we can cascade through the implications. Complex implies simple,
     * and simple implies fire and fall.
     * unfortunately, fall does not necessarily imply fire, nor the converse.
     */
    if( avoid_complex ) {
        avoid_simple = true;
        avoid_traps = true;
    }
    if( avoid_simple ) {
        avoid_fire = true;
        avoid_fall = true;
        avoid_sharp = true;
    }

    // technically this will shortcut in evaluation from fire or fall
    // before hitting simple or complex but this is more explicit
    if( avoid_fire || avoid_fall || avoid_simple ||
        avoid_complex || avoid_traps || avoid_sharp ) {
        const ter_id target = here.ter( p );
        if( !here.has_vehicle_floor( p ) ) {
            // Don't enter lava if we have any concept of heat being bad
            if( avoid_fire && target == t_lava ) {
                return false;
            }

            if( avoid_fall ) {
                // Don't throw ourselves off cliffs if we have a concept of falling
                if( !here.has_floor_or_water( p ) && !flies() ) {
                    return false;
                }

                // Don't enter open pits ever unless tiny, can fly or climb well
                if( !( type->size == creature_size::tiny || can_climb() ) &&
                    ( target == t_pit || target == t_pit_spiked || target == t_pit_glass ) ) {
                    return false;
                }
            }

            // Some things are only avoided if we're not attacking
            if( attitude( &get_player_character() ) != MATT_ATTACK ) {
                // Sharp terrain is ignored while attacking
                if( avoid_sharp && here.has_flag( ter_furn_flag::TFLAG_SHARP, p ) &&
                    !( type->size == creature_size::tiny || flies() ||
                       get_armor_type( damage_cut, bodypart_id( "torso" ) ) >= 10 ) ) {
                    return false;
                }
            }

            // Don't step on any traps (if we can see)
            const trap &target_trap = here.tr_at( p );
            if( avoid_traps && has_flag( mon_flag_SEES ) &&
                !target_trap.is_benign() && here.has_floor_or_water( p ) ) {
                return false;
            }
        }

        const field &target_field = here.field_at( p );
        // Higher awareness is needed for identifying these as threats.
        if( avoid_complex ) {
            // Don't enter any dangerous fields
            if( is_dangerous_fields( target_field ) ) {
                return false;
            }
        }

        // Without avoid_complex, only fire and electricity are checked for field avoidance.
        if( avoid_fire && target_field.find_field( fd_fire ) && !is_immune_field( fd_fire ) ) {
            return false;
        }
        if( avoid_simple && target_field.find_field( fd_electricity ) &&
            !is_immune_field( fd_electricity ) ) {
            return false;
        }
    }

    return true;
}

bool monster::can_reach_to( const tripoint &p ) const
{
    map &here = get_map();
    if( p.z > pos().z && z_is_valid( pos().z ) ) {
        if( here.has_flag( ter_furn_flag::TFLAG_RAMP_UP, tripoint( p.xy(), p.z - 1 ) ) ) {
            return true;
        }
        if( !here.has_flag( ter_furn_flag::TFLAG_GOES_UP, pos() ) && here.has_floor( p ) ) {
            // can't go through the roof
            return false;
        }
    } else if( p.z < pos().z && z_is_valid( pos().z ) ) {
        const tripoint above( p.xy(), p.z + 1 );
        if( here.has_flag( ter_furn_flag::TFLAG_RAMP_DOWN, above ) ) {
            return true;
        }
        if( !here.has_flag( ter_furn_flag::TFLAG_GOES_DOWN, pos() ) &&
            ( here.has_floor( above ) || ( !flies() && !here.has_floor_or_water( above ) ) ) ) {
            // can't go through the floor
            // Check floors for flying monsters movement
            return false;
        }
    }
    return true;
}

bool monster::can_move_to( const tripoint &p ) const
{
    return can_reach_to( p ) && will_move_to( p ) && know_danger_at( p );
}

float monster::rate_target( Creature &c, float best, bool smart ) const
{
    const FastDistanceApproximation d = rl_dist_fast( pos(), c.pos() );
    if( d <= 0 ) {
        return FLT_MAX;
    }

    // Check a very common and cheap case first
    if( !smart && d >= best ) {
        return FLT_MAX;
    }

    if( !sees( c ) ) {
        return FLT_MAX;
    }

    if( !smart ) {
        return static_cast<int>( d );
    }

    float power = c.power_rating();
    monster *mon = dynamic_cast< monster * >( &c );
    // Their attitude to us and not ours to them, so that bobcats won't get gunned down
    if( mon != nullptr && mon->attitude_to( *this ) == Attitude::HOSTILE ) {
        power += 2;
    }

    if( power > 0 ) {
        return static_cast<int>( d ) / power;
    }

    return FLT_MAX;
}

struct monster_plan {
    explicit monster_plan( const monster &mon );

    // Bots are more intelligent than most living stuff
    bool smart_planning;

    bool fleeing;
    bool docile;

    const bool angers_hostile_weak;
    const bool fears_hostile_weak;
    const bool placate_hostile_weak;
    const int angers_hostile_near;
    const int angers_hostile_seen;

    bool group_morale;
    bool swarms;

    // 8.6f is rating for tank drone 60 tiles away, moose 16 or boomer 33
    float dist;

    int max_sight_range;

    const int angers_mating_season;
    const int angers_cub_threatened;
    const int fears_hostile_near;
    const int fears_hostile_seen;

    Creature *target = nullptr;
};

monster_plan::monster_plan( const monster &mon ) :
    angers_hostile_weak( mon.type->has_anger_trigger( mon_trigger::HOSTILE_WEAK ) ),
    fears_hostile_weak( mon.type->has_fear_trigger( mon_trigger::HOSTILE_WEAK ) ),
    placate_hostile_weak( mon.type->has_placate_trigger( mon_trigger::HOSTILE_WEAK ) ),
    angers_hostile_near( mon.type->has_anger_trigger( mon_trigger::HOSTILE_CLOSE ) ? 5 : 0 ),
    angers_hostile_seen( mon.type->has_anger_trigger( mon_trigger::HOSTILE_SEEN ) ? rng( 0, 2 ) : 0 ),
    angers_mating_season( mon.type->has_anger_trigger( mon_trigger::MATING_SEASON ) ? 3 : 0 ),
    angers_cub_threatened( mon.type->has_anger_trigger( mon_trigger::PLAYER_NEAR_BABY ) ? 8 : 0 ),
    fears_hostile_near( mon.type->has_fear_trigger( mon_trigger::HOSTILE_CLOSE ) ? 5 : 0 ),
    fears_hostile_seen( mon.type->has_fear_trigger( mon_trigger::HOSTILE_SEEN ) ? rng( 0, 2 ) : 0 )
{
    smart_planning = mon.has_flag( mon_flag_PRIORITIZE_TARGETS );
    max_sight_range = std::max( mon.type->vision_day, mon.type->vision_night );
    dist = !smart_planning ? max_sight_range : 8.6f;
    fleeing = false;
    docile = mon.friendly != 0 && mon.has_effect( effect_docile );
    swarms = mon.has_flag( mon_flag_SWARMS );
    group_morale = mon.has_flag( mon_flag_GROUP_MORALE ) && mon.morale < mon.type->morale;
}

void monster::anger_hostile_seen( const monster_plan &mon_plan )
{
    if( mon_plan.fleeing ) {
        // runng away, too busy to be angry
        return;
    }

    // Decide that the player is too annoying, less likely than the other triggers
    if( mon_plan.angers_hostile_seen && x_in_y( anger, 200 ) ) {
        if( mon_plan.target->is_avatar() ) {
            add_msg_debug( debugmode::DF_MONSTER, "%s's character aggro triggered by seeing you", name() );
        } else {
            add_msg_debug( debugmode::DF_MONSTER, "%s's character aggro triggered by seeing %s", name(),
                           mon_plan.target->as_npc()->name );
        }
        aggro_character = true;
    }
}

void monster::anger_mating_season( const monster_plan &mon_plan )
{
    if( mon_plan.angers_mating_season > 0 && anger <= 30 ) {
        if( mating_angry() ) {
            anger += mon_plan.angers_mating_season;
            if( x_in_y( anger, 100 ) ) {
                add_msg_debug( debugmode::DF_MONSTER, "%s's character aggro triggered by season", name() );
                aggro_character = true;
            }
        }
    }
}

void monster::anger_cub_threatened( monster_plan &mon_plan )
{
    if( mon_plan.angers_cub_threatened < 0 ) {
        // return early, not angered by cubs being threatened
        return;
    }

    for( monster &tmp : g->all_monsters() ) {
        if( type->baby_monster == tmp.type->id ) {
            // baby nearby; is the player too close?
            mon_plan.dist = tmp.rate_target( *mon_plan.target, mon_plan.dist, mon_plan.smart_planning );
            if( mon_plan.dist <= 3 ) {
                //proximity to baby; monster gets furious and less likely to flee
                anger += mon_plan.angers_cub_threatened;
                morale += mon_plan.angers_cub_threatened / 2;
                add_msg_debug( debugmode::DF_MONSTER, "%s's character aggro triggered by threatening %s", name(),
                               tmp.name() );
                aggro_character = true;
            }
        }
    }
}

bool monster::mating_angry() const
{
    bool mating_angry = false;
    season_type season = season_of_year( calendar::turn );
    for( const std::string &elem : type->baby_flags ) {
        if( ( season == SUMMER && elem == "SUMMER" ) ||
            ( season == WINTER && elem == "WINTER" ) ||
            ( season == SPRING && elem == "SPRING" ) ||
            ( season == AUTUMN && elem == "AUTUMN" ) ) {
            mating_angry = true;
            break;
        }
    }
    return mating_angry;
}

void monster::plan()
{
    const auto &factions = g->critter_tracker->factions();

    monster_plan mon_plan( *this );

    map &here = get_map();
    std::bitset<OVERMAP_LAYERS> seen_levels = here.get_inter_level_visibility( pos().z );
    monster_attitude mood = attitude();
    Character &player_character = get_player_character();
    // If we can see the player, move toward them or flee.
    if( friendly == 0 && seen_levels.test( player_character.pos().z + OVERMAP_DEPTH ) &&
        sees( player_character ) ) {
        mon_plan.dist = rate_target( player_character, mon_plan.dist, mon_plan.smart_planning );
        mon_plan.fleeing = mon_plan.fleeing || is_fleeing( player_character );
        mon_plan.target = &player_character;
        if( !mon_plan.fleeing && anger <= 20 ) {
            anger += mon_plan.angers_hostile_seen;
        }

        anger_hostile_seen( mon_plan );

        if( mon_plan.dist <= 5 ) {
            if( anger <= 30 ) {
                anger += mon_plan.angers_hostile_near;
            }
            if( mon_plan.angers_hostile_near && x_in_y( anger, 100 ) ) {
                add_msg_debug( debugmode::DF_MONSTER, "%s's character aggro triggered by proximity", name() );
                aggro_character = true;
            }
            morale -= mon_plan.fears_hostile_near;
            anger_mating_season( mon_plan );
        }
        anger_cub_threatened( mon_plan );
    } else if( friendly != 0 && !mon_plan.docile ) {
        for( monster &tmp : g->all_monsters() ) {
            if( tmp.friendly == 0 && tmp.attitude_to( *this ) == Attitude::HOSTILE &&
                seen_levels.test( tmp.pos().z + OVERMAP_DEPTH ) ) {
                float rating = rate_target( tmp, mon_plan.dist, mon_plan.smart_planning );
                if( rating < mon_plan.dist ) {
                    mon_plan.target = &tmp;
                    mon_plan.dist = rating;
                }
            }
        }
    }

    if( mon_plan.docile ) {
        if( friendly != 0 && mon_plan.target != nullptr ) {
            set_dest( mon_plan.target->get_location() );
        }

        return;
    }

    int valid_targets = ( mon_plan.target == nullptr ) ? 0 : 1;
    for( npc &who : g->all_npcs() ) {
        mf_attitude faction_att = faction.obj().attitude( who.get_monster_faction() );
        if( faction_att == MFA_NEUTRAL || faction_att == MFA_FRIENDLY ) {
            continue;
        }
        if( !seen_levels.test( who.pos().z + OVERMAP_DEPTH ) ) {
            continue;
        }

        float rating = rate_target( who, mon_plan.dist, mon_plan.smart_planning );
        bool fleeing_from = is_fleeing( who );
        if( rating == mon_plan.dist && ( mon_plan.fleeing || attitude( &who ) == MATT_ATTACK ||
                                         attitude( &who ) == MATT_FOLLOW ) ) {
            ++valid_targets;
            if( one_in( valid_targets ) ) {
                mon_plan.target = &who;
            }
        }
        // Switch targets if closer and hostile or scarier than current target
        if( ( rating < mon_plan.dist && mon_plan.fleeing ) ||
            ( faction_att == MFA_HATE ) ||
            ( rating < mon_plan.dist && attitude( &who ) == MATT_ATTACK ) ||
            ( !mon_plan.fleeing && fleeing_from ) ) {
            mon_plan.target = &who;
            mon_plan.dist = rating;
            valid_targets = 1;
        }
        mon_plan.fleeing = mon_plan.fleeing || fleeing_from;
        if( rating <= 5 ) {
            if( anger <= 30 ) {
                anger += mon_plan.angers_hostile_near;
            }
            if( mon_plan.angers_hostile_near && x_in_y( anger, 100 ) ) {
                add_msg_debug( debugmode::DF_MONSTER, "%s's character aggro triggered by proximity to %s", name(),
                               who.name );
                aggro_character = true;
            }
            morale -= mon_plan.fears_hostile_near;
            anger_mating_season( mon_plan );
        }
        if( !mon_plan.fleeing && anger <= 20 && valid_targets != 0 ) {
            anger += mon_plan.angers_hostile_seen;
        }
        if( !mon_plan.fleeing && valid_targets != 0 ) {
            morale -= mon_plan.fears_hostile_seen;
            anger_hostile_seen( mon_plan );
        }
    }

    mon_plan.fleeing = mon_plan.fleeing || ( mood == MATT_FLEE );
    // Throttle monster thinking, if there are no apparent threats, stop paying attention.
    constexpr int max_turns_for_rate_limiting = 1800;
    constexpr double max_turns_to_skip = 600.0;
    // Outputs a range from 0.0 - 1.0.
    float rate_limiting_factor = 1.0 - logarithmic_range( 0, max_turns_for_rate_limiting,
                                 turns_since_target );
    int turns_to_skip = max_turns_to_skip * rate_limiting_factor;
    if( friendly == 0 && ( turns_to_skip == 0 || turns_since_target % turns_to_skip == 0 ) ) {
        for( const auto &fac_list : factions ) {
            mf_attitude faction_att = faction.obj().attitude( fac_list.first );
            if( faction_att == MFA_NEUTRAL || faction_att == MFA_FRIENDLY ) {
                continue;
            }

            for( const auto &fac : fac_list.second ) {
                if( !seen_levels.test( fac.first + OVERMAP_DEPTH ) ) {
                    continue;
                }
                for( const weak_ptr_fast<monster> &weak : fac.second ) {
                    const shared_ptr_fast<monster> shared = weak.lock();
                    if( !shared ) {
                        continue;
                    }
                    monster &mon = *shared;
                    float rating = rate_target( mon, mon_plan.dist, mon_plan.smart_planning );
                    if( rating == mon_plan.dist ) {
                        ++valid_targets;
                        if( one_in( valid_targets ) ) {
                            mon_plan.target = &mon;
                        }
                    }
                    if( rating < mon_plan.dist ) {
                        mon_plan.target = &mon;
                        mon_plan.dist = rating;
                        valid_targets = 1;
                    }
                    if( rating <= 5 ) {
                        if( anger <= 30 ) {
                            anger += mon_plan.angers_hostile_near;
                        }
                        morale -= mon_plan.fears_hostile_near;
                    }
                    if( !mon_plan.fleeing && anger <= 20 && valid_targets != 0 ) {
                        anger += mon_plan.angers_hostile_seen;
                    }
                    if( !mon_plan.fleeing && valid_targets != 0 ) {
                        morale -= mon_plan.fears_hostile_seen;
                    }
                }
            }
        }
    }
    if( mon_plan.target == nullptr ) {
        // Just avoiding overflow.
        turns_since_target = std::min( turns_since_target + 1, max_turns_for_rate_limiting );
    } else {
        turns_since_target = 0;
    }

    // Friendly monsters here
    // Avoid for hordes of same-faction stuff or it could get expensive
    const mfaction_id actual_faction = friendly == 0 ? faction : STATIC( mfaction_str_id( "player" ) );
    const auto &myfaction_iter = factions.find( actual_faction );
    if( myfaction_iter == factions.end() ) {
        DebugLog( D_ERROR, D_GAME ) << disp_name() << " tried to find faction "
                                    << actual_faction.id().str()
                                    << " which wasn't loaded in game::monmove";
        mon_plan.swarms = false;
        mon_plan.group_morale = false;
    }
    mon_plan.swarms = mon_plan.swarms && mon_plan.target == nullptr; // Only swarm if we have no target
    if( mon_plan.group_morale || mon_plan.swarms ) {
        for( const auto &fac : myfaction_iter->second ) {
            if( !seen_levels.test( fac.first + OVERMAP_DEPTH ) ) {
                continue;
            }
            for( const weak_ptr_fast<monster> &weak : fac.second ) {
                const shared_ptr_fast<monster> shared = weak.lock();
                if( !shared ) {
                    continue;
                }
                monster &mon = *shared;
                float rating = rate_target( mon, mon_plan.dist, mon_plan.smart_planning );
                if( mon_plan.group_morale && rating <= 10 ) {
                    morale += 10 - rating;
                }
                if( mon_plan.swarms ) {
                    if( rating < 5 ) { // Too crowded here
                        wander_pos = get_location() + point( rng( 1, 3 ), rng( 1, 3 ) );
                        wandf = 2;
                        mon_plan.target = nullptr;
                        // Swarm to the furthest ally you can see
                    } else if( rating < FLT_MAX && rating > mon_plan.dist && wandf <= 0 ) {
                        mon_plan.target = &mon;
                        mon_plan.dist = rating;
                    }
                }
            }
        }
    }

    // Operating monster keep you safe while they operate, how nice....
    if( type->has_special_attack( "OPERATE" ) ) {
        if( has_effect( effect_operating ) ) {
            friendly = 100;
            for( Creature *critter : here.get_creatures_in_radius( pos(), 6 ) ) {
                monster *mon = dynamic_cast<monster *>( critter );
                if( mon != nullptr && mon->type->in_species( species_ZOMBIE ) ) {
                    anger = 100;
                } else {
                    anger = 0;
                }
            }
        }
    }

    if( has_effect( effect_dragging ) ) {

        if( type->has_special_attack( "OPERATE" ) ) {

            bool found_path_to_couch = false;
            tripoint tmp( pos() + point( 12, 12 ) );
            tripoint couch_loc;
            for( const tripoint &couch_pos : here.find_furnitures_with_flag_in_radius( pos(), 10,
                    ter_furn_flag::TFLAG_AUTODOC_COUCH ) ) {
                if( here.clear_path( pos(), couch_pos, 10, 0, 100 ) ) {
                    if( rl_dist( pos(), couch_pos ) < rl_dist( pos(), tmp ) ) {
                        tmp = couch_pos;
                        found_path_to_couch = true;
                        couch_loc = couch_pos;
                    }
                }
            }

            if( !found_path_to_couch ) {
                anger = 0;
                remove_effect( effect_dragging );
            } else {
                set_dest( here.getglobal( couch_loc ) );
            }
        }

    } else if( mon_plan.target != nullptr ) {

        const tripoint_abs_ms dest = mon_plan.target->get_location();
        Creature::Attitude att_to_target = attitude_to( *mon_plan.target );
        if( att_to_target == Attitude::HOSTILE && !mon_plan.fleeing ) {
            set_dest( dest );
        } else if( mon_plan.fleeing ) {
            tripoint_abs_ms away = get_location() - dest + get_location();
            away.z() = posz();
            set_dest( away );
        }
        if( ( mon_plan.angers_hostile_weak || mon_plan.fears_hostile_weak ||
              mon_plan.placate_hostile_weak ) &&
            att_to_target != Attitude::FRIENDLY ) {
            int hp_per = mon_plan.target->hp_percentage();
            if( hp_per <= 70 ) {
                if( mon_plan.angers_hostile_weak && anger <= 40 ) {
                    anger += 10 - static_cast<int>( hp_per / 10 );
                    if( x_in_y( anger, 100 ) ) {
                        add_msg_debug( debugmode::DF_MONSTER, "%s's character aggro triggered by %s's weakness", name(),
                                       mon_plan.target->disp_name() );
                        aggro_character = true;
                    }
                } else if( mon_plan.fears_hostile_weak ) {
                    morale -= 10 - static_cast<int>( hp_per / 10 );
                } else if( mon_plan.placate_hostile_weak ) {
                    anger -= 10 - static_cast<int>( hp_per / 10 );
                }
            }
        }
    } else if( !patrol_route.empty() ) {
        // If we have a patrol route and no target, find the current step on the route
        tripoint_abs_ms next_stop = patrol_route.at( next_patrol_point );

        // if there is more than one patrol point, advance to the next one if we're almost there
        // this handles impassable obstacles but patrollers can still get stuck
        if( ( patrol_route.size() > 1 ) && rl_dist( next_stop, get_location() ) < 2 ) {
            next_patrol_point = ( next_patrol_point + 1 ) % patrol_route.size();
            next_stop = patrol_route.at( next_patrol_point );
        }
        set_dest( next_stop );
    } else if( friendly != 0 && has_effect( effect_led_by_leash ) &&
               get_location().z() == get_dest().z() ) {
        // visibility doesn't matter, we're getting pulled by a leash
        // To use stairs smoothly, if the destination is on a different Z-level, move there first.
        set_dest( player_character.get_location() );
        if( friendly > 0 && one_in( 3 ) ) {
            // Grow restless with no targets
            friendly--;
        }
    } else if( friendly > 0 && one_in( 3 ) ) {
        // Grow restless with no targets
        friendly--;
    } else if( is_pet_follow() && sees( player_character ) &&
               ( get_location().z() == player_character.get_location().z() ||
                 get_location().z() == get_dest().z() ) ) {
        // Simpleminded animals are too dumb to follow the player.
        // To use stairs smoothly, if the destination is on a different Z-level, move there first.
        set_dest( player_character.get_location() );
    }
}

/**
 * Method to make monster movement speed consistent in the face of staggering behavior and
 * differing distance metrics.
 * It works by scaling the cost to take a step by
 * how much that step reduces the distance to your goal.
 * Since it incorporates the current distance metric,
 * it also scales for diagonal vs orthogonal movement.
 **/
static float get_stagger_adjust( const tripoint &source, const tripoint &destination,
                                 const tripoint &next_step )
{
    // TODO: push this down into rl_dist
    const float initial_dist =
        trigdist ? trig_dist( source, destination ) : rl_dist( source, destination );
    const float new_dist =
        trigdist ? trig_dist( next_step, destination ) : rl_dist( next_step, destination );
    // If we return 0, it wil cancel the action.
    return std::max( 0.01f, initial_dist - new_dist );
}

/**
 * Returns true if the given square presents a possibility of drowning for the monster: it's deep water, it's liquid,
 * the monster can drown, and there is no boardable vehicle part present.
 */
bool monster::is_aquatic_danger( const tripoint &at_pos ) const
{
    map &here = get_map();
    return here.has_flag_ter( ter_furn_flag::TFLAG_DEEP_WATER, at_pos ) &&
           here.has_flag( ter_furn_flag::TFLAG_LIQUID, at_pos ) &&
           can_drown() && !here.veh_at( at_pos ).part_with_feature( "BOARDABLE", false );
}

bool monster::die_if_drowning( const tripoint &at_pos, const int chance )
{
    if( is_aquatic_danger( at_pos ) && one_in( chance ) ) {
        add_msg_if_player_sees( at_pos, _( "The %s drowns!" ), name() );
        die( nullptr );
        return true;
    }
    return false;
}

// General movement.
// Currently, priority goes:
// 1) Special Attack
// 2) Sight-based tracking
// 3) Scent-based tracking
// 4) Sound-based tracking
void monster::move()
{
    add_msg_debug( debugmode::DF_MONMOVE, "Monster %s starting monmove::move, remaining moves %d",
                   name(), moves );
    // We decrement wandf no matter what.  We'll save our wander_to plans until
    // after we finish out set_dest plans, UNLESS they time out first.
    if( wandf > 0 ) {
        wandf--;
    }

    //Hallucinations have a chance of disappearing each turn
    if( is_hallucination() && one_in( 25 ) ) {
        die( nullptr );
        return;
    }
    map &here = get_map();
    Character &player_character = get_player_character();

    behavior::monster_oracle_t oracle( this );
    behavior::tree goals;
    goals.add( type->get_goals() );
    std::string action = goals.tick( &oracle );
    //The monster can consume objects it stands on. Check if there are any.
    //If there are. Consume them.
    // TODO: Stick this in a map and dispatch to it via the action string.
    // TODO: Create a special attacks whitelist unordered map instead of an if chain.
    std::map<std::string, mtype_special_attack>::const_iterator attack =
        type->special_attacks.find( action );
    if( attack != type->special_attacks.end() && attack->second->call( *this ) ) {
        if( special_attacks.count( action ) != 0 ) {
            reset_special( action );
        }
    }
    // record position before moving to put the player there if we're dragging
    tripoint_abs_ms drag_to = get_location();

    const bool pacified = has_effect( effect_pacified );

    // First, use the special attack, if we can!
    // The attack may change `monster::special_attacks` (e.g. by transforming
    // this into another monster type). Therefore we can not iterate over it
    // directly and instead iterate over the map from the monster type
    // (properties of monster types should never change).
    for( const auto &sp_type : type->special_attacks ) {
        const std::string &special_name = sp_type.first;
        const auto local_iter = special_attacks.find( special_name );
        if( local_iter == special_attacks.end() ) {
            continue;
        }
        mon_special_attack &local_attack_data = local_iter->second;
        if( !local_attack_data.enabled ) {
            continue;
        }

        add_msg_debug( debugmode::DF_MATTACK, "%s attempting a special attack %s, cooldown %d", name(),
                       sp_type.first, local_attack_data.cooldown );

        // Cooldowns are decremented in monster::process_turn

        if( local_attack_data.cooldown == 0 && !pacified && !is_hallucination() ) {
            if( !sp_type.second->call( *this ) ) {
                add_msg_debug( debugmode::DF_MATTACK, "Attack failed" );
                continue;
            }

            // `special_attacks` might have changed at this point. Sadly `reset_special`
            // doesn't check the attack name, so we need to do it here.
            if( special_attacks.count( special_name ) == 0 ) {
                continue;
            }
            reset_special( special_name );
        }
    }

    // Check if they're dragging a foe and find their hapless victim
    Character *dragged_foe = find_dragged_foe();

    // Give nursebots a chance to do surgery.
    nursebot_operate( dragged_foe );

    // The monster can sometimes hang in air due to last fall being blocked
    if( !flies() && !here.has_floor_or_water( pos() ) && !here.has_vehicle_floor( pos_bub() ) ) {
        here.creature_on_trap( *this, false );
        if( is_dead() ) {
            return;
        }
    }

    // if the monster is in a deep water tile, it has a chance to drown
    if( die_if_drowning( pos(), 10 ) ) {
        return;
    }

    if( moves < 0 ) {
        return;
    }

    // TODO: Move this to attack_at/move_to/etc. functions
    bool attacking = false;
    if( !move_effects( attacking ) ) {
        moves = 0;
        return;
    }
    if( has_flag( mon_flag_IMMOBILE ) || has_flag( mon_flag_RIDEABLE_MECH ) ) {
        moves = 0;
        return;
    }
    if( has_effect( effect_stunned ) ) {
        stumble();
        moves = 0;
        return;
    }
    if( friendly > 0 ) {
        --friendly;
    }
    const optional_vpart_position ovp = here.veh_at( pos() );
    if( has_effect( effect_harnessed ) ) {
        if( !ovp.part_with_feature( "ANIMAL_CTRL", true ) ) {
            remove_effect( effect_harnessed ); // the harness part probably broke
        } else {
            moves = 0;
            return; // don't move if harnessed
        }
    }
    const std::optional<vpart_reference> vp_boardable = ovp.part_with_feature( "BOARDABLE", true );
    if( vp_boardable && friendly != 0 ) {
        const vehicle &veh = vp_boardable->vehicle();
        if( veh.is_moving() && veh.get_monster( vp_boardable->part_index() ) ) {
            moves = 0;
            return; // don't move if friendly and passenger in a moving vehicle
        }
    }
    // Set attitude to attitude to our current target
    monster_attitude current_attitude = attitude( nullptr );
    if( !is_wandering() ) {
        if( get_dest() == player_character.get_location() ) {
            current_attitude = attitude( &player_character );
        } else {
            for( const npc &guy : g->all_npcs() ) {
                if( get_dest() == guy.get_location() ) {
                    current_attitude = attitude( &guy );
                }
            }
        }
    }

    if( is_pet_follow() || ( friendly != 0 && has_effect( effect_led_by_leash ) ) ) {
        const int dist = rl_dist( get_location(), get_dest() );
        if( ( dist <= 1 || ( dist <= 2 && !has_effect( effect_led_by_leash ) &&
                             sees( player_character ) ) ) &&
            ( get_dest() == player_character.get_location() &&
              get_location().z() == player_character.get_location().z() ) ) {
            moves = 0;
            stumble();
            return;
        }
    } else if( ( current_attitude == MATT_IGNORE && patrol_route.empty() ) ||
               ( ( current_attitude == MATT_FOLLOW ||
                   ( has_flag( mon_flag_KEEP_DISTANCE ) && !( current_attitude == MATT_FLEE ) ) )
                 && rl_dist( get_location(), get_dest() ) <= type->tracking_distance ) ) {
        moves = 0;
        stumble();
        return;
    }

    bool moved = false;
    tripoint destination;

    bool try_to_move = false;
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint &dest : here.points_in_radius( pos(), 1 ) ) {
        if( dest != pos() ) {
            if( can_move_to( dest ) &&
                creatures.creature_at( dest, true ) == nullptr ) {
                try_to_move = true;
                break;
            }
        }
    }

    // If true, don't try to greedily avoid locally bad paths
    bool pathed = false;
    tripoint local_dest = here.getlocal( get_dest() );
    if( try_to_move ) {
        // Move using vision by follow smells and sounds
        bool move_without_target = false;
        if( is_wandering() && has_intelligence() && can_see() ) {
            if( has_flag( mon_flag_SMELLS ) ) {
                unset_dest();
                tripoint tmp = scent_move();
                if( tmp.x != -1 ) {
                    local_dest = tmp;
                    move_without_target = true;
                    add_msg_debug( debugmode::DF_MONMOVE, "%s follows smell using vision", name() );
                }
            }
            if( !move_without_target && wandf > 0 && friendly == 0 ) {
                unset_dest();
                if( wander_pos != get_location() ) {
                    local_dest = here.getlocal( wander_pos );
                    move_without_target = true;
                    add_msg_debug( debugmode::DF_MONMOVE, "%s follows sound using vision", name() );
                }
            }
        }

        if( !is_wandering() || move_without_target ) {
            while( !path.empty() && path.front() == pos() ) {
                path.erase( path.begin() );
            }

            const pathfinding_settings &pf_settings = get_pathfinding_settings();
            if( pf_settings.max_dist >= rl_dist( get_location(), get_dest() ) &&
                ( path.empty() || rl_dist( pos(), path.front() ) >= 2 || path.back() != local_dest ) ) {
                // We need a new path
                path = here.route( pos(), local_dest, pf_settings, get_path_avoid() );
            }

            // Try to respect old paths, even if we can't pathfind at the moment
            if( !path.empty() && path.back() == local_dest ) {
                destination = path.front();
                moved = true;
                pathed = true;
            } else {
                // Straight line forward, probably because we can't pathfind (well enough)
                destination = local_dest;
                moved = true;
            }
        }
    }
    if( !moved && has_flag( mon_flag_SMELLS ) ) {
        // No sight... or our plans are invalid (e.g. moving through a transparent, but
        //  solid, square of terrain).  Fall back to smell if we have it.
        unset_dest();
        tripoint tmp = scent_move();
        if( tmp.x != -1 ) {
            destination = tmp;
            moved = true;
            add_msg_debug( debugmode::DF_MONMOVE, "%s follows smell to not use vision", name() );
        }
    }
    if( wandf > 0 && !moved && friendly == 0 ) { // No LOS, no scent, so as a fall-back follow sound
        unset_dest();
        if( wander_pos != get_location() ) {
            destination = here.getlocal( wander_pos );
            moved = true;
            add_msg_debug( debugmode::DF_MONMOVE, "%s follows sound to not use vision", name() );
        }
    }

    point new_d( destination.xy() - pos().xy() );

    // toggle facing direction for sdl flip
    if( !g->is_tileset_isometric() ) {
        if( new_d.x < 0 ) {
            facing = FacingDirection::LEFT;
        } else if( new_d.x > 0 ) {
            facing = FacingDirection::RIGHT;
        }
    } else {
        if( new_d.y <= 0 && new_d.x <= 0 ) {
            facing = FacingDirection::LEFT;
        }
        if( new_d.x >= 0 && new_d.y >= 0 ) {
            facing = FacingDirection::RIGHT;
        }
    }

    tripoint_abs_ms next_step;
    const bool can_open_doors = has_flag( mon_flag_CAN_OPEN_DOORS ) && !is_hallucination();
    const bool staggers = has_flag( mon_flag_STUMBLES );
    if( moved ) {
        // Implement both avoiding obstacles and staggering.
        moved = false;
        float switch_chance = 0.0f;
        const bool can_bash = bash_skill() > 0;
        // This is a float and using trig_dist() because that Does the Right Thing(tm)
        // in both circular and roguelike distance modes.
        const float distance_to_target = trig_dist( pos(), destination );
        for( tripoint &candidate : squares_closer_to( pos(), destination ) ) {
            // rare scenario when monster is on the border of the map and it's goal is outside of the map
            if( !here.inbounds( candidate ) ) {
                continue;
            }

            bool via_ramp = false;
            int rampPos = 0;
            if( here.has_flag( ter_furn_flag::TFLAG_RAMP_UP, candidate ) ) {
                via_ramp = true;
                rampPos -= 1;
                candidate.z += 1;
            } else if( here.has_flag( ter_furn_flag::TFLAG_RAMP_DOWN, candidate ) ) {
                via_ramp = true;
                rampPos += 1;
                candidate.z -= 1;
            }
            const tripoint_abs_ms candidate_abs = get_map().getglobal( candidate );

            if( candidate.z != posz() ) {
                bool can_z_move = true;
                if( !here.valid_move( pos(), candidate, false, true, via_ramp ) ) {
                    // Can't phase through floor
                    can_z_move = false;
                }

                // If we're trying to go up but can't fly, check if we can climb. If we can't, then don't
                // This prevents non-climb/fly enemies running up walls
                if( candidate.z > posz() && !( via_ramp || flies() ) ) {
                    if( !can_climb() || !here.has_floor_or_support( candidate ) ) {
                        if( ( !here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, pos() ) ||
                              !here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, candidate ) ) ) {
                            // Can't "jump" up a whole z-level
                            can_z_move = false;
                        }
                    }
                }

                // Last chance - we can still do the z-level stair teleport bullshit that isn't removed yet
                // TODO: Remove z-level stair bullshit teleport after aligning all stairs
                if( !can_z_move &&
                    posx() / ( SEEX * 2 ) == candidate.x / ( SEEX * 2 ) &&
                    posy() / ( SEEY * 2 ) == candidate.y / ( SEEY * 2 ) ) {
                    const tripoint upper = candidate.z > posz() ? candidate : pos();
                    const tripoint lower = candidate.z > posz() ? pos() : candidate;
                    if( here.has_flag( ter_furn_flag::TFLAG_GOES_DOWN, upper ) &&
                        here.has_flag( ter_furn_flag::TFLAG_GOES_UP, lower ) ) {
                        can_z_move = true;
                    }
                }

                if( !can_z_move ) {
                    continue;
                }
            }

            // A flag to allow non-stumbling critters to stumble when the most direct choice is bad.
            bool bad_choice = false;

            const Creature *target = creatures.creature_at( candidate, is_hallucination() );
            if( target != nullptr ) {
                if( is_hallucination() != target->is_hallucination() && !target->is_avatar() ) {
                    // Hallucinations should only be capable of targeting the player or other hallucinations.
                    continue;
                }
                const Attitude att = attitude_to( *target );
                if( att == Attitude::HOSTILE ) {
                    // When attacking an adjacent enemy, we're direct.
                    moved = true;
                    next_step = candidate_abs;
                    break;
                }
                if( att == Attitude::FRIENDLY && ( target->is_avatar() || target->is_npc() ||
                                                   target->has_flag( mon_flag_QUEEN ) ) ) {
                    // Friendly firing the player or an NPC is illegal for gameplay reasons.
                    // Monsters should instinctively avoid attacking queens that regenerate their own population.
                    continue;
                }
                if( !has_flag( mon_flag_ATTACKMON ) && !has_flag( mon_flag_PUSH_MON ) ) {
                    // Bail out if there's a non-hostile monster in the way and we're not pushy.
                    continue;
                }
                // Friendly fire and pushing are always bad choices - they take a lot of time
                bad_choice = true;
            }

            // is there an openable door?
            if( can_open_doors &&
                here.open_door( *this, candidate, !here.is_outside( pos() ), true ) ) {
                moved = true;
                next_step = candidate_abs;
                continue;
            }

            // Try to shove vehicle out of the way
            shove_vehicle( destination, candidate );
            // Bail out if we can't move there and we can't bash.
            if( !pathed && !can_move_to( candidate ) ) {
                if( !can_bash ) {
                    continue;
                }
                // Don't bash if we're just tracking a noise.
                if( !provocative_sound && is_wandering() && destination == here.getlocal( wander_pos ) ) {
                    continue;
                }
                const int estimate = here.bash_rating( bash_estimate(), candidate );
                if( estimate <= 0 ) {
                    continue;
                }

                if( estimate < 5 ) {
                    bad_choice = true;
                }
            }

            const float progress = distance_to_target - trig_dist( tripoint( candidate.xy(),
                                   candidate.z + rampPos ), destination );
            // The x2 makes the first (and most direct) path twice as likely,
            // since the chance of switching is 1/1, 1/4, 1/6, 1/8
            switch_chance += progress * 2;
            // Randomly pick one of the viable squares to move to weighted by distance.
            if( progress > 0 && ( !moved || x_in_y( progress, switch_chance ) ) ) {
                moved = true;
                next_step = candidate_abs;
                // If we stumble, pick a random square, otherwise take the first one,
                // which is the most direct path.
                // Except if the direct path is bad, then check others
                // Or if the path is given by pathfinder
                if( !staggers && ( !bad_choice || pathed ) ) {
                    break;
                }
            }
        }
    }
    // Finished logic section.  By this point, we should have chosen a square to
    //  move to (moved = true).
    if( moved ) { // Actual effects of moving to the square we've chosen
        const tripoint local_next_step = here.getlocal( next_step );
        const bool did_something =
            ( !pacified && attack_at( local_next_step ) ) ||
            ( !pacified && can_open_doors &&
              here.open_door( *this, local_next_step, !here.is_outside( pos() ) ) ) ||
            ( !pacified && bash_at( local_next_step ) ) ||
            ( !pacified && push_to( local_next_step, 0, 0 ) ) ||
            move_to( local_next_step, false, false, get_stagger_adjust( pos(), destination, local_next_step ) );

        if( !did_something ) {
            moves -= 100; // If we don't do this, we'll get infinite loops.
        }
        if( has_effect( effect_dragging ) && dragged_foe != nullptr ) {

            if( !dragged_foe->has_effect( effect_grabbed ) ) {
                dragged_foe = nullptr;
                remove_effect( effect_dragging );
            } else if( drag_to != get_location() && creatures.creature_at( drag_to ) == nullptr ) {
                dragged_foe->move_to( drag_to );
            }
        }
    } else {
        moves = 0;
        stumble();
        path.clear();
    }
    if( has_effect( effect_led_by_leash ) ) {
        if( rl_dist( get_location(), player_character.get_location() ) > 2 ) {
            // Either failed to keep up with the player or moved away
            remove_effect( effect_led_by_leash );
            add_msg( m_info, _( "You lose hold of a leash." ) );
        }
    }
}

Character *monster::find_dragged_foe()
{
    // Make sure they're actually dragging someone.
    if( !dragged_foe_id.is_valid() || !has_effect( effect_dragging ) ) {
        dragged_foe_id = character_id();
        return nullptr;
    }

    // Dragged critters may die or otherwise become invalid, which is why we look
    // them up each time. Luckily, monsters dragging critters is relatively rare,
    // so this check should happen infrequently.
    Character *dragged_foe = g->critter_by_id<Character>( dragged_foe_id );

    if( dragged_foe == nullptr ) {
        // Target no longer valid.
        dragged_foe_id = character_id();
        remove_effect( effect_dragging );
    }

    return dragged_foe;
}

// Nursebot surgery code
void monster::nursebot_operate( Character *dragged_foe )
{
    // No dragged foe, nothing to do.
    if( dragged_foe == nullptr || !has_dest() ) {
        return;
    }

    // Nothing to do if they can't operate, or they don't think they're dragging.
    if( !( type->has_special_attack( "OPERATE" ) && has_effect( effect_dragging ) ) ) {
        return;
    }

    creature_tracker &creatures = get_creature_tracker();
    map &here = get_map();
    if( rl_dist( get_location(), get_dest() ) == 1 &&
        !here.has_flag_furn( ter_furn_flag::TFLAG_AUTODOC_COUCH, here.getlocal( get_dest() ) ) &&
        !has_effect( effect_operating ) ) {
        if( dragged_foe->has_effect( effect_grabbed ) && !has_effect( effect_countdown ) &&
            ( creatures.creature_at( get_dest() ) == nullptr ||
              creatures.creature_at( get_dest() ) == dragged_foe ) ) {
            add_msg( m_bad, _( "The %1$s slowly but firmly puts %2$s down onto the Autodoc couch." ), name(),
                     dragged_foe->disp_name() );

            dragged_foe->move_to( get_dest() );

            // There's still time to get away
            add_effect( effect_countdown, 2_turns );
            add_msg( m_bad, _( "The %s produces a syringe full of some translucent liquid." ), name() );
        } else if( creatures.creature_at( get_dest() ) != nullptr && has_effect( effect_dragging ) ) {
            sounds::sound( pos(), 8, sounds::sound_t::electronic_speech,
                           string_format(
                               _( "a soft robotic voice say, \"Please step away from the Autodoc, this patient needs immediate care.\"" ) ) );
            // TODO: Make it able to push NPC/player
            push_to( here.getlocal( get_dest() ), 4, 0 );
        }
    }
    if( get_effect_dur( effect_countdown ) == 1_turns && !has_effect( effect_operating ) ) {
        if( dragged_foe->has_effect( effect_grabbed ) ) {

            const bionic_collection &collec = *dragged_foe->my_bionics;
            cata_assert( !collec.empty() );
            const int index = rng( 0, collec.size() - 1 );
            const bionic *const target_cbm = &collec[index];
            const bionic &real_target =
                target_cbm->is_included()
                ? **dragged_foe->find_bionic_by_uid( target_cbm->get_parent_uid() )
                : *target_cbm;

            //8 intelligence*4 + 8 first aid*4 + 3 computer *3 + 4 electronic*1 = 77
            const float adjusted_skill = static_cast<float>( 77 ) - std::min( static_cast<float>( 40 ),
                                         static_cast<float>( 77 ) - static_cast<float>( 77 ) / static_cast<float>( 10.0 ) );

            dragged_foe->cancel_activity();
            get_player_character().uninstall_bionic( real_target, *this, *dragged_foe, adjusted_skill );

            // Remove target grab
            for( const effect &eff : dragged_foe->get_effects_with_flag( json_flag_GRAB ) ) {
                dragged_foe->remove_effect( eff.get_id() );
            }
            // And our own grab filters
            for( const effect &eff : get_effects_with_flag( json_flag_GRAB_FILTER ) ) {
                remove_effect( eff.get_id() );
            }
            dragged_foe_id = character_id();

        }
    }
}

// footsteps will determine how loud a monster's normal movement is
// and create a sound in the monsters location when they move
void monster::footsteps( const tripoint &p )
{
    if( is_hallucination() ) {
        return;
    }

    if( made_footstep ) {
        return;
    }
    made_footstep = true;
    int volume = 6; // same as player's footsteps
    if( flies() ) {
        volume = 0;    // Flying monsters don't have footsteps!
    }
    if( digging() ) {
        volume = 10;
    }
    switch( type->size ) {
        case creature_size::tiny:
            volume = 0; // No sound for the tinies
            break;
        case creature_size::small:
            volume /= 3;
            break;
        case creature_size::medium:
            break;
        case creature_size::large:
            volume *= 1.5;
            break;
        case creature_size::huge:
            volume *= 2;
            break;
        default:
            break;
    }
    if( has_flag( mon_flag_LOUDMOVES ) ) {
        volume += 6;
    }
    if( volume == 0 ) {
        return;
    }
    int dist = rl_dist( p, get_player_character().pos() );
    sounds::add_footstep( p, volume, dist, this, type->get_footsteps() );
}

tripoint monster::scent_move()
{
    // TODO: Remove when scentmap is 3D
    if( std::abs( posz() - get_map().get_abs_sub().z() ) > SCENT_MAP_Z_REACH ) {
        return { -1, -1, INT_MIN };
    }
    scent_map &scents = get_scent();
    bool in_range = scents.inbounds( pos() );
    if( !in_range ) {
        return { -1, -1, INT_MIN };
    }

    const std::set<scenttype_id> &tracked_scents = type->scents_tracked;
    const std::set<scenttype_id> &ignored_scents = type->scents_ignored;

    std::vector<tripoint> sdirection;
    std::vector<tripoint> smoves;

    int bestsmell = 10; // Squares with smell 0 are not eligible targets.
    int smell_threshold = 200; // Squares at or above this level are ineligible.
    if( has_flag( mon_flag_KEENNOSE ) ) {
        bestsmell = 1;
        smell_threshold = 400;
    }

    Character &player_character = get_player_character();
    const bool fleeing = is_fleeing( player_character );
    int scent_here = scents.get_unsafe( pos() );
    if( fleeing ) {
        bestsmell = scent_here;
    }

    tripoint next( -1, -1, posz() );
    // Scent is too weak, can't follow it.
    if( scent_here == 0 ) {
        return next;
    }
    // Check for the scent type being compatible.
    const scenttype_id &type_scent = scents.get_type();
    bool right_scent = false;
    // is the monster tracking this scent
    if( !tracked_scents.empty() ) {
        right_scent = tracked_scents.find( type_scent ) != tracked_scents.end();
    }
    //is this scent recognised by the monster species
    if( !type_scent.is_empty() ) {
        const std::set<species_id> &receptive_species = type_scent->receptive_species;
        const std::set<species_id> &monster_species = type->species;
        std::vector<species_id> v_intersection;
        std::set_intersection( receptive_species.begin(), receptive_species.end(), monster_species.begin(),
                               monster_species.end(), std::back_inserter( v_intersection ) );
        if( !v_intersection.empty() ) {
            right_scent = true;
        }
    }
    // is the monster actually ignoring this scent
    if( !ignored_scents.empty() && ( ignored_scents.find( type_scent ) != ignored_scents.end() ) ) {
        right_scent = false;
    }
    if( !right_scent ) {
        return { -1, -1, INT_MIN };
    }

    const bool can_bash = bash_skill() > 0;
    map &here = get_map();
    if( !fleeing && scent_here > smell_threshold ) {
        // Smell too strong to track, wander around
        sdirection.push_back( pos() );
    } else {
        // Get direction of scent
        for( const tripoint &dest : here.points_in_radius( pos(), 1, SCENT_MAP_Z_REACH ) ) {
            int smell = scents.get( dest );

            if( ( !fleeing && smell < bestsmell ) || ( fleeing && smell > bestsmell ) ) {
                continue;
            }
            if( ( !fleeing && smell > bestsmell ) || ( fleeing && smell < bestsmell ) ) {
                sdirection.clear();
                sdirection.push_back( dest );
                bestsmell = smell;
            } else if( ( !fleeing && smell == bestsmell ) || ( fleeing && smell == bestsmell ) ) {
                sdirection.push_back( dest );
            }
        }
    }

    for( const tripoint &direction : sdirection ) {
        // Add some randomness to make creatures zigzag towards the source
        for( const tripoint &dest : here.points_in_radius( direction, 1 ) ) {
            if( here.valid_move( pos(), dest, can_bash, true ) &&
                ( can_move_to( dest ) || ( dest == player_character.pos() ) ||
                  ( can_bash && here.bash_rating( bash_estimate(), dest ) > 0 ) ) ) {
                smoves.push_back( dest );
            }
        }
    }

    return random_entry( smoves, next );
}

int monster::calc_movecost( const tripoint &f, const tripoint &t ) const
{
    int movecost = 0;

    map &here = get_map();
    const int source_cost = here.move_cost( f );
    const int dest_cost = here.move_cost( t );
    // Digging and flying monsters ignore terrain cost
    if( flies() || ( digging() && here.has_flag( ter_furn_flag::TFLAG_DIGGABLE, t ) ) ) {
        movecost = 100;
        // Swimming monsters move super fast in water
    } else if( swims() ) {
        if( here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, f ) ) {
            movecost += 25;
        } else {
            movecost += 50 * here.move_cost( f );
        }
        if( here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, t ) ) {
            movecost += 25;
        } else {
            movecost += 50 * here.move_cost( t );
        }
    } else if( can_submerge() ) {
        // No-breathe monsters have to walk underwater slowly
        if( here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, f ) ) {
            movecost += 250;
        } else {
            movecost += 50 * here.move_cost( f );
        }
        if( here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, t ) ) {
            movecost += 250;
        } else {
            movecost += 50 * here.move_cost( t );
        }
        movecost /= 2;
    } else if( climbs() ) {
        if( here.has_flag( ter_furn_flag::TFLAG_CLIMBABLE, f ) ) {
            movecost += 150;
        } else {
            movecost += 50 * here.move_cost( f );
        }
        if( here.has_flag( ter_furn_flag::TFLAG_CLIMBABLE, t ) ) {
            movecost += 150;
        } else {
            movecost += 50 * here.move_cost( t );
        }
        movecost /= 2;
    } else {
        movecost = ( ( 50 * source_cost ) + ( 50 * dest_cost ) ) / 2.0;
    }

    return movecost;
}

int monster::calc_climb_cost( const tripoint &f, const tripoint &t ) const
{
    if( flies() ) {
        return 100;
    }

    map &here = get_map();
    if( climbs() && !here.has_flag( ter_furn_flag::TFLAG_NO_FLOOR, t ) ) {
        const int diff = here.climb_difficulty( f );
        if( diff <= 10 ) {
            return 150;
        }
    }

    return 0;
}

/*
 * Return points of an area extending 1 tile to either side and
 * (maxdepth) tiles behind basher.
 */
static std::vector<tripoint> get_bashing_zone( const tripoint &bashee, const tripoint &basher,
        int maxdepth )
{
    std::vector<tripoint> direction;
    direction.push_back( bashee );
    direction.push_back( basher );
    // Draw a line from the target through the attacker.
    std::vector<tripoint> path = continue_line( direction, maxdepth );
    // Remove the target.
    path.insert( path.begin(), basher );
    std::vector<tripoint> zone;
    // Go ahead and reserve enough room for all the points since
    // we know how many it will be.
    zone.reserve( 3 * maxdepth );
    tripoint previous = bashee;
    for( const tripoint &p : path ) {
        std::vector<point> swath = squares_in_direction( previous.xy(), p.xy() );
        for( const point &q : swath ) {
            zone.emplace_back( q, bashee.z );
        }

        previous = p;
    }
    return zone;
}

bool monster::bash_at( const tripoint &p )
{
    if( p.z != posz() ) {
        // TODO: Remove this
        return false;
    }

    //Hallucinations can't bash stuff.
    if( is_hallucination() ) {
        return false;
    }

    // Don't bash if a friendly monster is standing there
    monster *target = get_creature_tracker().creature_at<monster>( p );
    if( target != nullptr && attitude_to( *target ) == Attitude::FRIENDLY ) {
        return false;
    }

    bool try_bash = !can_move_to( p ) || one_in( 3 );
    if( !try_bash ) {
        return false;
    }

    if( bash_skill() <= 0 ) {
        return false;
    }

    map &here = get_map();
    if( !( here.is_bashable_furn( p ) || here.veh_at( p ).obstacle_at_part() ) ) {
        // if the only thing here is road or flat, rarely bash it
        bool flat_ground = here.has_flag( ter_furn_flag::TFLAG_ROAD, p ) ||
                           here.has_flag( ter_furn_flag::TFLAG_FLAT, p );
        if( !here.is_bashable_ter( p ) || ( flat_ground && !one_in( 50 ) ) ) {
            return false;
        }
    }

    int bashskill = group_bash_skill( p );
    here.bash( p, bashskill );
    moves -= 100;
    return true;
}

int monster::bash_estimate() const
{
    int estimate = bash_skill();
    if( has_flag( mon_flag_GROUP_BASH ) ) {
        // Right now just give them a boost so they try to bash a lot of stuff.
        // TODO: base it on number of nearby friendlies.
        estimate *= 2;
    }
    return estimate;
}

int monster::bash_skill() const
{
    return type->bash_skill;
}

int monster::group_bash_skill( const tripoint &target )
{
    if( !has_flag( mon_flag_GROUP_BASH ) ) {
        return bash_skill();
    }
    int bashskill = 0;

    // pileup = more bash skill, but only help bashing mob directly in front of target
    const int max_helper_depth = 5;
    const std::vector<tripoint> bzone = get_bashing_zone( target, pos(), max_helper_depth );

    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint &candidate : bzone ) {
        // Drawing this line backwards excludes the target and includes the candidate.
        std::vector<tripoint> path_to_target = line_to( target, candidate, 0, 0 );
        bool connected = true;
        monster *mon = nullptr;
        for( const tripoint &in_path : path_to_target ) {
            // If any point in the line from zombie to target is not a cooperating zombie,
            // it can't contribute.
            mon = creatures.creature_at<monster>( in_path );
            if( !mon ) {
                connected = false;
                break;
            }
            monster &helpermon = *mon;
            if( !helpermon.has_flag( mon_flag_GROUP_BASH ) || helpermon.is_hallucination() ) {
                connected = false;
                break;
            }
        }
        if( !connected || !mon ) {
            continue;
        }
        // If we made it here, the last monster checked was the candidate.
        monster &helpermon = *mon;
        // Contribution falls off rapidly with distance from target.
        bashskill += helpermon.bash_skill() / rl_dist( candidate, target );
    }

    return bashskill;
}

bool monster::attack_at( const tripoint &p )
{
    if( has_flag( mon_flag_PACIFIST ) ) {
        return false;
    }

    Character &player_character = get_player_character();
    const bool sees_player = sees( player_character );
    // Targeting player location
    if( p == player_character.pos() ) {
        if( sees_player ) {
            return melee_attack( player_character );
        } else {
            // Creature stumbles into a player it cannot see, briefly becoming aware of their location
            return stumble_invis( player_character );
        }
    }

    creature_tracker &creatures = get_creature_tracker();
    if( monster *mon_ = creatures.creature_at<monster>( p, is_hallucination() ) ) {
        monster &mon = *mon_;

        // Don't attack yourself.
        if( &mon == this ) {
            return false;
        }

        // With no melee dice, we can't attack, but we had to process until here
        // because hallucinations require no melee dice to destroy.
        if( type->melee_dice <= 0 ) {
            return false;
        }

        Creature::Attitude attitude = attitude_to( mon );
        // mon_flag_ATTACKMON == hulk behavior, whack everything in your way
        if( attitude == Attitude::HOSTILE || has_flag( mon_flag_ATTACKMON ) ) {
            return melee_attack( mon );
        }

        return false;
    }

    npc *const guy = creatures.creature_at<npc>( p, is_hallucination() );
    if( guy && type->melee_dice > 0 ) {
        // For now we're always attacking NPCs that are getting into our
        // way. This is consistent with how it worked previously, but
        // later on not hitting allied NPCs would be cool.
        guy->on_attacked( *this ); // allow NPC hallucination to be one shot by monsters
        return melee_attack( *guy );
    }

    // Attack last known position despite empty
    if( has_effect( effect_stumbled_into_invisible ) &&
        get_map().has_field_at( p, field_fd_last_known ) && !sees_player &&
        attitude_to( player_character ) == Attitude::HOSTILE ) {
        return attack_air( p );
    }

    // Nothing to attack.
    return false;
}

static tripoint find_closest_stair( const tripoint &near_this, const ter_furn_flag stair_type )
{
    map &here = get_map();
    for( const tripoint &candidate : closest_points_first( near_this, 10 ) ) {
        if( here.has_flag( stair_type, candidate ) ) {
            return candidate;
        }
    }
    // we didn't find it
    return near_this;
}

bool monster::move_to( const tripoint &p, bool force, bool step_on_critter,
                       const float stagger_adjustment )
{
    const bool on_ground = !digging() && !flies();

    const bool z_move = p.z != pos().z;
    const bool going_up = p.z > pos().z;

    tripoint destination = p;
    map &here = get_map();

    // This is stair teleportation hackery.
    // TODO: Remove this in favor of stair alignment
    if( going_up ) {
        if( here.has_flag( ter_furn_flag::TFLAG_GOES_UP, pos() ) ) {
            destination = find_closest_stair( p, ter_furn_flag::TFLAG_GOES_DOWN );
        }
    } else if( z_move ) {
        if( here.has_flag( ter_furn_flag::TFLAG_GOES_DOWN, pos() ) ) {
            destination = find_closest_stair( p, ter_furn_flag::TFLAG_GOES_UP );
        }
    }

    // Allows climbing monsters to move on terrain with movecost <= 0
    Creature *critter = get_creature_tracker().creature_at( destination, is_hallucination() );
    if( here.has_flag( ter_furn_flag::TFLAG_CLIMBABLE, destination ) ) {
        if( here.impassable( destination ) && critter == nullptr ) {
            if( flies() ) {
                moves -= 100;
                force = true;
                if( get_option<bool>( "LOG_MONSTER_MOVEMENT" ) ) {
                    add_msg_if_player_sees( *this, _( "The %1$s flies over the %2$s." ), name(),
                                            here.has_flag_furn( ter_furn_flag::TFLAG_CLIMBABLE, p ) ? here.furnname( p ) :
                                            here.tername( p ) );
                }
            } else if( climbs() ) {
                moves -= 150;
                force = true;
                if( get_option<bool>( "LOG_MONSTER_MOVEMENT" ) ) {
                    add_msg_if_player_sees( *this, _( "The %1$s climbs over the %2$s." ), name(),
                                            here.has_flag_furn( ter_furn_flag::TFLAG_CLIMBABLE, p ) ? here.furnname( p ) :
                                            here.tername( p ) );
                }
            }
        }
    }

    if( critter != nullptr && !step_on_critter ) {
        return false;
    }

    // Make sure that we can move there, unless force is true.
    if( !force && !can_move_to( destination ) ) {
        return false;
    }

    if( !force ) {
        // This adjustment is to make it so that monster movement speed relative to the player
        // is consistent even if the monster stumbles,
        // and the same regardless of the distance measurement mode.
        // Note: Keep this as float here or else it will cancel valid moves
        const float cost = stagger_adjustment *
                           static_cast<float>( climbs() &&
                                               here.has_flag( ter_furn_flag::TFLAG_NO_FLOOR, p ) ? calc_climb_cost( pos(),
                                                       destination ) : calc_movecost( pos(),
                                                               destination ) );
        if( cost > 0.0f ) {
            moves -= static_cast<int>( std::ceil( cost ) );
        } else {
            return false;
        }
    }

    //Check for moving into/out of water
    bool was_water = underwater;
    bool will_be_water =
        on_ground && (
            // AQUATIC monsters always "swim under" the vehicles, while other swimming monsters are forced to surface
            has_flag( mon_flag_AQUATIC ) || ( can_submerge() && !here.veh_at( destination ) )
        ) && here.is_divable( destination );

    if( get_option<bool>( "LOG_MONSTER_MOVEMENT" ) ) {
        //Birds and other flying creatures flying over the deep water terrain
        if( was_water && flies() ) {
            if( one_in( 4 ) ) {
                add_msg_if_player_sees( *this, m_warning, _( "A %1$s flies over the %2$s!" ),
                                        name(), here.tername( pos() ) );
            }
        } else if( was_water && !will_be_water ) {
            // Use more dramatic messages for swimming monsters
            add_msg_if_player_sees( *this, m_warning,
                                    //~ Message when a monster emerges from water
                                    //~ %1$s: monster name, %2$s: leaps/emerges, %3$s: terrain name
                                    pgettext( "monster movement", "A %1$s %2$s from the %3$s!" ),
                                    name(), swims() ||
                                    has_flag( mon_flag_AQUATIC ) ? _( "leaps" ) : _( "emerges" ), here.tername( pos() ) );
        } else if( !was_water && will_be_water ) {
            add_msg_if_player_sees( *this, m_warning, pgettext( "monster movement",
                                    //~ Message when a monster enters water
                                    //~ %1$s: monster name, %2$s: dives/sinks, %3$s: terrain name
                                    "A %1$s %2$s into the %3$s!" ),
                                    name(), swims() ||
                                    has_flag( mon_flag_AQUATIC ) ? _( "dives" ) : _( "sinks" ), here.tername( destination ) );
        }
    }

    optional_vpart_position vp_orig = here.veh_at( pos() );
    if( vp_orig ) {
        vp_orig->vehicle().invalidate_mass();
    }

    setpos( destination );
    footsteps( destination );
    underwater = will_be_water;
    optional_vpart_position vp_dest = here.veh_at( destination );
    if( vp_dest ) {
        vp_dest->vehicle().invalidate_mass();
    }
    if( is_hallucination() ) {
        //Hallucinations don't do any of the stuff after this point
        return true;
    }

    if( here.has_flag( ter_furn_flag::TFLAG_UNSTABLE, destination ) &&
        on_ground && !here.has_vehicle_floor( destination ) ) {
        add_effect( effect_bouldering, 1_turns, true );
    } else if( has_effect( effect_bouldering ) ) {
        remove_effect( effect_bouldering );
    }

    if( here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_NO_SIGHT, destination ) && on_ground ) {
        add_effect( effect_no_sight, 1_turns, true );
    } else if( has_effect( effect_no_sight ) ) {
        remove_effect( effect_no_sight );
    }

    if( !here.has_vehicle_floor( destination ) ) {
        if( type->size != creature_size::tiny && on_ground ) {
            const int sharp_damage = rng( 1, 10 );
            const int rough_damage = rng( 1, 2 );
            if( here.has_flag( ter_furn_flag::TFLAG_SHARP, pos() ) && !one_in( 4 ) &&
                get_armor_type( damage_cut, bodypart_id( "torso" ) ) < sharp_damage && get_hp() > sharp_damage ) {
                apply_damage( nullptr, bodypart_id( "torso" ), sharp_damage );
            }
            if( here.has_flag( ter_furn_flag::TFLAG_ROUGH, pos() ) && one_in( 6 ) &&
                get_armor_type( damage_cut, bodypart_id( "torso" ) ) < rough_damage && get_hp() > rough_damage ) {
                apply_damage( nullptr, bodypart_id( "torso" ), rough_damage );
            }
        }
        here.creature_on_trap( *this );
        if( is_dead() ) {
            return true;
        }
    }
    if( !will_be_water && ( digs() || can_dig() ) ) {
        underwater = here.has_flag( ter_furn_flag::TFLAG_DIGGABLE, pos() );
    }

    // Digging creatures leave a trail of churned earth
    // They always leave some on their tile, and larger creatures emit some around themselves as well
    if( digging() && here.has_flag( ter_furn_flag::TFLAG_DIGGABLE, pos() ) ) {
        int factor = 0;
        switch( type->size ) {
            case creature_size::medium:
                factor = 4;
                break;
            case creature_size::large:
                factor = 3;
                break;
            case creature_size::huge:
                factor = 2;
                break;
            case creature_size::num_sizes:
                debugmsg( "ERROR: Invalid Creature size class." );
                break;
            default:
                factor = 4;
                break;
        }
        here.add_field( pos(), fd_churned_earth, 2 );
        for( const tripoint &dest : here.points_in_radius( pos(), 1, 0 ) ) {
            if( here.has_flag( ter_furn_flag::TFLAG_DIGGABLE, dest ) && one_in( factor ) ) {
                here.add_field( dest, fd_churned_earth, 2 );
            }
        }
    }

    // Acid trail monsters leave... a trail of acid
    if( has_flag( mon_flag_ACIDTRAIL ) ) {
        here.add_field( pos(), fd_acid, 3 );
    }

    // Not all acid trail monsters leave as much acid. Every time this monster takes a step, there is a 1/5 chance it will drop a puddle.
    if( has_flag( mon_flag_SHORTACIDTRAIL ) ) {
        if( one_in( 5 ) ) {
            here.add_field( pos(), fd_acid, 3 );
        }
    }

    if( has_flag( mon_flag_SLUDGETRAIL ) ) {
        for( const tripoint &sludge_p : here.points_in_radius( pos(), 1 ) ) {
            const int fstr = 3 - ( std::abs( sludge_p.x - posx() ) + std::abs( sludge_p.y - posy() ) );
            if( fstr >= 2 ) {
                here.add_field( sludge_p, fd_sludge, fstr );
            }
        }
    }

    if( has_flag( mon_flag_SMALLSLUDGETRAIL ) ) {
        if( one_in( 2 ) ) {
            here.add_field( pos(), fd_sludge, 1 );
        }
    }

    // Don't leave any kind of liquids on water tiles
    if( !here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, destination ) ) {
        if( has_flag( mon_flag_DRIPS_NAPALM ) ) {
            if( one_in( 10 ) ) {
                // if it has more napalm, drop some and reduce ammo in tank
                if( ammo[itype_pressurized_tank] > 0 ) {
                    here.add_item_or_charges( pos(), item( "napalm", calendar::turn, 50 ) );
                    ammo[itype_pressurized_tank] -= 50;
                } else {
                    // TODO: remove mon_flag_DRIPS_NAPALM flag since no more napalm in tank
                    // Not possible for now since flag check is done on type, not individual monster
                }
            }
        }
        if( has_flag( mon_flag_DRIPS_GASOLINE ) ) {
            if( one_in( 5 ) ) {
                // TODO: use same idea that limits napalm dripping
                here.add_item_or_charges( pos(), item( "gasoline" ) );
            }
        }
    }
    return true;
}

bool monster::push_to( const tripoint &p, const int boost, const size_t depth )
{
    if( is_hallucination() ) {
        // Don't let hallucinations push, not even other hallucinations
        return false;
    }

    if( !has_flag( mon_flag_PUSH_MON ) || depth > 2 || has_effect( effect_pushed ) ) {
        return false;
    }

    creature_tracker &creatures = get_creature_tracker();
    // TODO: Generalize this to Creature
    monster *const critter = creatures.creature_at<monster>( p );
    if( critter == nullptr || critter == this ||
        p == pos() || critter->movement_impaired() ) {
        return false;
    }

    if( !can_move_to( p ) ) {
        return false;
    }

    if( critter->is_hallucination() ) {
        // Kill the hallu, but return false so that the regular move_to is uses instead
        critter->die( nullptr );
        return false;
    }

    // Stability roll of the pushed critter
    const int defend = critter->stability_roll();
    // Stability roll of the pushing zed
    const int attack = stability_roll() + boost;
    if( defend > attack ) {
        return false;
    }

    map &here = get_map();
    const int movecost_from = 50 * here.move_cost( p );
    const int movecost_attacker = std::max( movecost_from, 200 - 10 * ( attack - defend ) );
    const tripoint dir = p - pos();

    // Mark self as pushed to simplify recursive pushing
    add_effect( effect_pushed, 1_turns );

    for( size_t i = 0; i < 6; i++ ) {
        const point d( rng( -1, 1 ), rng( -1, 1 ) );
        if( d.x == 0 && d.y == 0 ) {
            continue;
        }

        // Pushing forward is easier than pushing aside
        const int direction_penalty = std::abs( d.x - dir.x ) + std::abs( d.y - dir.y );
        if( direction_penalty > 2 ) {
            continue;
        }

        tripoint dest( p + d );
        const int dest_movecost_from = 50 * here.move_cost( dest );

        // Pushing into cars/windows etc. is harder
        const int movecost_penalty = here.move_cost( dest ) - 2;
        if( movecost_penalty <= -2 ) {
            // Can't push into unpassable terrain
            continue;
        }

        int roll = attack - ( defend + direction_penalty + movecost_penalty );
        if( roll < 0 ) {
            continue;
        }

        Creature *critter_recur = creatures.creature_at( dest );
        if( !( critter_recur == nullptr || critter_recur->is_hallucination() ) ) {
            // Try to push recursively
            monster *mon_recur = dynamic_cast< monster * >( critter_recur );
            if( mon_recur == nullptr ) {
                continue;
            }

            if( critter->push_to( dest, roll, depth + 1 ) ) {
                // The tile isn't necessarily free, need to check
                if( !creatures.creature_at( p ) ) {
                    move_to( p );
                }

                moves -= movecost_attacker;

                // Don't knock down a creature that successfully
                // pushed another creature, just reduce moves
                critter->moves -= dest_movecost_from;
                return true;
            } else {
                return false;
            }
        }

        critter_recur = creatures.creature_at( dest );
        if( critter_recur != nullptr ) {
            if( critter_recur->is_hallucination() ) {
                critter_recur->die( nullptr );
            }
        } else if( !critter->has_flag( mon_flag_IMMOBILE ) ) {
            critter->setpos( dest );
            move_to( p );
            moves -= movecost_attacker;
            critter->add_effect( effect_downed, time_duration::from_turns( movecost_from / 100 + 1 ) );
        }
        return true;
    }

    // Try to trample over a much weaker zed (or one with worse rolls)
    // Don't allow trampling with boost
    if( boost > 0 || attack < 2 * defend ) {
        return false;
    }

    g->swap_critters( *critter, *this );
    critter->add_effect( effect_stunned, rng( 0_turns, 2_turns ) );
    Character &player_character = get_player_character();
    // Only print the message when near player or it can get spammy
    if( rl_dist( player_character.pos(), pos() ) < 4 ) {
        add_msg_if_player_sees( *critter, m_warning, _( "The %1$s tramples %2$s." ),
                                name(), critter->disp_name() );
    }

    moves -= movecost_attacker;
    if( movecost_from > 100 ) {
        critter->add_effect( effect_downed, time_duration::from_turns( movecost_from / 100 + 1 ) );
    } else {
        critter->moves -= movecost_from;
    }

    return true;
}

/**
 * Stumble in a random direction, but with some caveats.
 */
void monster::stumble()
{
    add_msg_debug( debugmode::DF_MONMOVE, "%s starting monmove::stumble", name() );
    // Only move every 10 turns.
    if( !one_in( 10 ) ) {
        return;
    }

    map &here = get_map();
    std::vector<tripoint> valid_stumbles;
    valid_stumbles.reserve( 11 );
    const bool avoid_water = has_flag( mon_flag_NO_BREATHE ) && !swims() &&
                             !has_flag( mon_flag_AQUATIC );
    for( const tripoint &dest : here.points_in_radius( pos(), 1 ) ) {
        if( dest != pos() ) {
            if( here.has_flag( ter_furn_flag::TFLAG_RAMP_DOWN, dest ) ) {
                valid_stumbles.emplace_back( dest.xy(), dest.z - 1 );
            } else  if( here.has_flag( ter_furn_flag::TFLAG_RAMP_UP, dest ) ) {
                valid_stumbles.emplace_back( dest.xy(), dest.z + 1 );
            } else {
                valid_stumbles.push_back( dest );
            }
        }
    }
    const tripoint below( posx(), posy(), posz() - 1 );
    if( here.valid_move( pos(), below, false, true ) ) {
        valid_stumbles.push_back( below );
    }

    creature_tracker &creatures = get_creature_tracker();
    while( !valid_stumbles.empty() && !is_dead() ) {
        const tripoint dest = random_entry_removed( valid_stumbles );
        if( can_move_to( dest ) &&
            //Stop zombies and other non-breathing monsters wandering INTO water
            //(Unless they can swim/are aquatic)
            //But let them wander OUT of water if they are there.
            !( avoid_water &&
               here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, dest ) &&
               !here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, pos() ) ) &&
            ( creatures.creature_at( dest, is_hallucination() ) == nullptr ) ) {
            if( move_to( dest, true, false ) ) {
                break;
            }
        }
    }
}

void monster::knock_back_to( const tripoint &to )
{
    if( to == pos() ) {
        return; // No effect
    }

    if( is_hallucination() ) {
        die( nullptr );
        return;
    }

    bool u_see = get_player_view().sees( to );

    creature_tracker &creatures = get_creature_tracker();
    // First, see if we hit another monster
    if( monster *const z = creatures.creature_at<monster>( to ) ) {
        apply_damage( z, bodypart_id( "torso" ), static_cast<float>( z->type->size ) );
        add_effect( effect_stunned, 1_turns );
        if( type->size > 1 + z->type->size ) {
            z->knock_back_from( pos() ); // Chain reaction!
            z->apply_damage( this, bodypart_id( "torso" ), static_cast<float>( type->size ) );
            z->add_effect( effect_stunned, 1_turns );
        } else if( type->size > z->type->size ) {
            z->apply_damage( this, bodypart_id( "torso" ), static_cast<float>( type->size ) );
            z->add_effect( effect_stunned, 1_turns );
        }
        z->check_dead_state();

        if( u_see ) {
            add_msg( _( "The %1$s bounces off a %2$s!" ), name(), z->name() );
        }

        return;
    }

    if( npc *const p = creatures.creature_at<npc>( to ) ) {
        apply_damage( p, bodypart_id( "torso" ), 3 );
        add_effect( effect_stunned, 1_turns );
        p->deal_damage( this, bodypart_id( "torso" ),
                        damage_instance( STATIC( damage_type_id( "bash" ) ), static_cast<float>( type->size ) ) );
        if( u_see ) {
            add_msg( _( "The %1$s bounces off %2$s!" ), name(), p->get_name() );
        }

        p->check_dead_state();
        return;
    }

    // If we're still in the function at this point, we're actually moving a tile!
    // die_if_drowning will kill the monster if necessary, but if the deep water
    // tile is on a vehicle, we should check for swimmers out of water
    if( !die_if_drowning( to ) && has_flag( mon_flag_AQUATIC ) ) {
        die( nullptr );
        if( u_see ) {
            add_msg( _( "The %s flops around and dies!" ), name() );
        }
    }

    map &here = get_map();
    // It's some kind of wall.
    if( here.impassable( to ) ) {
        const int dam = static_cast<int>( type->size );
        apply_damage( nullptr, bodypart_id( "torso" ), dam );
        add_effect( effect_stunned, 2_turns );
        if( u_see ) {
            add_msg( _( "The %1$s bounces off a %2$s and takes %3$d damage." ), name(),
                     here.obstacle_name( to ), dam );
        }

    } else { // It's no wall
        setpos( to );
    }
    check_dead_state();
}

/* will_reach() is used for determining whether we'll get to stairs (and
 * potentially other locations of interest).  It is generally permissive.
 * TODO: Pathfinding;
         Make sure that non-smashing monsters won't "teleport" through windows
         Injure monsters if they're gonna be walking through pits or whatever
 */
bool monster::will_reach( const point &p )
{
    monster_attitude att = attitude( &get_player_character() );
    if( att != MATT_FOLLOW && att != MATT_ATTACK && att != MATT_FRIEND ) {
        return false;
    }

    if( digs() || has_flag( mon_flag_AQUATIC ) ) {
        return false;
    }

    if( ( has_flag( mon_flag_IMMOBILE ) || has_flag( mon_flag_RIDEABLE_MECH ) ) &&
        ( pos().xy() != p ) ) {
        return false;
    }

    auto path = get_map().route( pos(), tripoint( p, posz() ), get_pathfinding_settings() );
    if( path.empty() ) {
        return false;
    }

    if( has_flag( mon_flag_SMELLS ) && get_scent().get( pos() ) > 0 &&
        get_scent().get( { p, posz() } ) > get_scent().get( pos() ) ) {
        return true;
    }

    if( can_hear() && wandf > 0 && rl_dist( get_map().getlocal( wander_pos ).xy(), p ) <= 2 &&
        rl_dist( get_location().xy(), wander_pos.xy() ) <= wandf ) {
        return true;
    }

    if( can_see() && sees( tripoint( p, posz() ) ) ) {
        return true;
    }

    return false;
}

int monster::turns_to_reach( const point &p )
{
    map &here = get_map();
    // HACK: This function is a(n old) temporary hack that should soon be removed
    auto path = here.route( pos(), tripoint( p, posz() ), get_pathfinding_settings() );
    if( path.empty() ) {
        return 999;
    }

    double turns = 0.;
    for( size_t i = 0; i < path.size(); i++ ) {
        const tripoint &next = path[i];
        if( here.impassable( next ) ) {
            // No bashing through, it looks stupid when you go back and find
            // the doors intact.
            return 999;
        } else if( i == 0 ) {
            turns += static_cast<double>( calc_movecost( pos(), next ) ) / get_speed();
        } else {
            turns += static_cast<double>( calc_movecost( path[i - 1], next ) ) / get_speed();
        }
    }

    return static_cast<int>( turns + .9 ); // Halve (to get turns) and round up
}

void monster::shove_vehicle( const tripoint &remote_destination,
                             const tripoint &nearby_destination )
{
    map &here = get_map();
    if( this->has_flag( mon_flag_PUSH_VEH ) && !is_hallucination() ) {
        optional_vpart_position vp = here.veh_at( nearby_destination );
        if( vp ) {
            vehicle &veh = vp->vehicle();
            const units::mass veh_mass = veh.total_mass();
            int shove_moves_minimal = 0;
            int shove_veh_mass_moves_factor = 0;
            int shove_velocity = 0;
            float shove_damage_min = 0.00F;
            float shove_damage_max = 0.00F;
            switch( this->get_size() ) {
                case creature_size::tiny:
                case creature_size::small:
                    break;
                case creature_size::medium:
                    if( veh_mass < 500_kilogram ) {
                        shove_moves_minimal = 150;
                        shove_veh_mass_moves_factor = 20;
                        shove_velocity = 500;
                        shove_damage_min = 0.00F;
                        shove_damage_max = 0.01F;
                    }
                    break;
                case creature_size::large:
                    if( veh_mass < 1000_kilogram ) {
                        shove_moves_minimal = 100;
                        shove_veh_mass_moves_factor = 8;
                        shove_velocity = 1000;
                        shove_damage_min = 0.00F;
                        shove_damage_max = 0.03F;
                    }
                    break;
                case creature_size::huge:
                    if( veh_mass < 2000_kilogram ) {
                        shove_moves_minimal = 50;
                        shove_veh_mass_moves_factor = 4;
                        shove_velocity = 1500;
                        shove_damage_min = 0.00F;
                        shove_damage_max = 0.05F;
                    }
                    break;
                default:
                    break;
            }
            if( shove_velocity > 0 ) {
                //~ %1$s - monster name, %2$s - vehicle name
                add_msg_if_player_sees( this->pos(), m_bad, _( "%1$s shoves %2$s out of their way!" ),
                                        this->disp_name(),
                                        veh.disp_name() );
                int shove_moves = shove_veh_mass_moves_factor * veh_mass / 10_kilogram;
                shove_moves = std::max( shove_moves, shove_moves_minimal );
                this->mod_moves( -shove_moves );
                const tripoint destination_delta( -nearby_destination + remote_destination );
                const tripoint shove_destination( clamp( destination_delta.x, -1, 1 ),
                                                  clamp( destination_delta.y, -1, 1 ),
                                                  clamp( destination_delta.z, -1, 1 ) );
                veh.skidding = true;
                veh.velocity = shove_velocity;
                if( shove_destination != tripoint_zero ) {
                    if( shove_destination.z != 0 ) {
                        veh.vertical_velocity = shove_destination.z < 0 ? -shove_velocity : +shove_velocity;
                    }
                    here.move_vehicle( veh, shove_destination, veh.face );
                }
                veh.move = tileray( destination_delta.xy() );
                veh.smash( here, shove_damage_min, shove_damage_max, 0.10F );
            }
        }
    }
}
