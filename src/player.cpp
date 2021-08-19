#include "player.h"

#include <functional>
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <new>
#include <string>
#include <unordered_map>

#include "action.h"
#include "activity_actor_definitions.h"
#include "activity_type.h"
#include "ammo.h"
#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character_martial_arts.h"
#include "clzones.h"
#include "color.h"
#include "coordinates.h"
#include "damage.h"
#include "debug.h"
#include "effect.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "faction.h"
#include "fault.h"
#include "field_type.h"
#include "flag.h"
#include "game.h"
#include "handle_liquid.h"
#include "input.h"
#include "inventory.h"
#include "item.h"
#include "item_location.h"
#include "item_pocket.h"
#include "itype.h"
#include "line.h"
#include "make_static.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "martialarts.h"
#include "messages.h"
#include "monster.h"
#include "morale.h"
#include "mtype.h"
#include "mutation.h"
#include "npc.h"
#include "output.h"
#include "overmap_types.h"
#include "overmapbuffer.h"
#include "pimpl.h"
#include "player_activity.h"
#include "pldata.h"
#include "profession.h"
#include "requirements.h"
#include "rng.h"
#include "skill.h"
#include "string_formatter.h"
#include "translations.h"
#include "trap.h"
#include "ui.h"
#include "uistate.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"
#include "visitable.h"
#include "vitamin.h"
#include "vpart_position.h"
#include "weather.h"

static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_downed( "downed" );
static const efftype_id effect_masked_scent( "masked_scent" );
static const efftype_id effect_narcosis( "narcosis" );
static const efftype_id effect_onfire( "onfire" );
static const efftype_id effect_sleep( "sleep" );

static const trait_id trait_DEBUG_BIONIC_POWER( "DEBUG_BIONIC_POWER" );
static const trait_id trait_NOMAD( "NOMAD" );
static const trait_id trait_NOMAD2( "NOMAD2" );
static const trait_id trait_NOMAD3( "NOMAD3" );

static const skill_id skill_firstaid( "firstaid" );
static const skill_id skill_swimming( "swimming" );

player::player()
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
    cash = 0;
    scent = 500;
    male = true;
    prof = profession::has_initialized() ? profession::generic() :
           nullptr; //workaround for a potential structural limitation, see player::create

    start_location = start_location_id( "sloc_shelter" );
    moves = 100;
    oxygen = 0;
    in_vehicle = false;
    controlling_vehicle = false;
    grab_point = tripoint_zero;
    hauling = false;
    set_focus( 100 );
    last_item = itype_id( "null" );
    sight_max = 9999;
    last_batch = 0;
    lastconsumed = itype_id( "null" );
    death_drops = true;

    nv_cached = false;
    volume = 0;

    set_value( "THIEF_MODE", "THIEF_ASK" );

    for( const auto &v : vitamin::all() ) {
        vitamin_levels[ v.first ] = 0;
    }

    // Only call these if game is initialized
    if( !!g && json_flag::is_ready() ) {
        recalc_sight_limits();
        calc_encumbrance();
    }
}

player::~player() = default;
player::player( player && ) noexcept( map_is_noexcept ) = default;
player &player::operator=( player && ) noexcept( list_is_noexcept ) = default;

void player::normalize()
{
    Character::normalize();

    recalc_hp();

    set_all_parts_temp_conv( BODYTEMP_NORM );
    set_stamina( get_stamina_max() );
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
    process_items();
    // Didn't just pick something up
    last_item = itype_id( "null" );

    if( !is_npc() && has_trait( trait_DEBUG_BIONIC_POWER ) ) {
        mod_power_level( get_max_power_level() );
    }

    visit_items( [this]( item * e, item * ) {
        e->process_relic( this, pos() );
        return VisitResponse::NEXT;
    } );

    suffer();
    // NPCs currently don't make any use of their scent, pointless to calculate it
    // TODO: make use of NPC scent.
    if( !is_npc() ) {
        if( !has_effect( effect_masked_scent ) ) {
            restore_scent();
        }
        const int mask_intensity = get_effect_int( effect_masked_scent );

        // Set our scent towards the norm
        int norm_scent = 500;
        int temp_norm_scent = INT_MIN;
        bool found_intensity = false;
        for( const trait_id &mut : get_mutations() ) {
            const cata::optional<int> &scent_intensity = mut->scent_intensity;
            if( scent_intensity ) {
                found_intensity = true;
                temp_norm_scent = std::max( temp_norm_scent, *scent_intensity );
            }
        }
        if( found_intensity ) {
            norm_scent = temp_norm_scent;
        }

        for( const trait_id &mut : get_mutations() ) {
            const cata::optional<int> &scent_mask = mut->scent_mask;
            if( scent_mask ) {
                norm_scent += *scent_mask;
            }
        }

        //mask from scent altering items;
        norm_scent += mask_intensity;

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

        if( !martial_arts_data->has_martialart( ma ) && can_autolearn( ma ) ) {
            martial_arts_data->add_martialart( ma );
            add_msg_if_player( m_info, _( "You have learned a new style: %s!" ), ma.obj().name );
        }
    }

    // Update time spent conscious in this overmap tile for the Nomad traits.
    if( !is_npc() && ( has_trait( trait_NOMAD ) || has_trait( trait_NOMAD2 ) ||
                       has_trait( trait_NOMAD3 ) ) &&
        !has_effect( effect_sleep ) && !has_effect( effect_narcosis ) ) {
        const tripoint_abs_omt ompos = global_omt_location();
        const point_abs_omt pos = ompos.xy();
        if( overmap_time.find( pos ) == overmap_time.end() ) {
            overmap_time[pos] = 1_turns;
        } else {
            overmap_time[pos] += 1_turns;
        }
    }
    // Decay time spent in other overmap tiles.
    if( !is_npc() && calendar::once_every( 1_hours ) ) {
        const tripoint_abs_omt ompos = global_omt_location();
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
            if( it->first == ompos.xy() ) {
                it++;
                continue;
            }
            // Find the amount of time passed since the player touched any of the overmap tile's submaps.
            const tripoint_abs_omt tpt( it->first, 0 );
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

void player::mod_stat( const std::string &stat, float modifier )
{
    if( stat == "thirst" ) {
        mod_thirst( modifier );
    } else if( stat == "fatigue" ) {
        mod_fatigue( modifier );
    } else if( stat == "oxygen" ) {
        oxygen += modifier;
    } else if( stat == "stamina" ) {
        mod_stamina( modifier );
    } else {
        // Fall through to the creature method.
        Character::mod_stat( stat, modifier );
    }
}

void player::pause()
{
    moves = 0;
    recoil = MAX_RECOIL;

    map &here = get_map();
    // Train swimming if underwater
    if( !in_vehicle ) {
        if( underwater ) {
            practice( skill_swimming, 1 );
            drench( 100, { {
                    body_part_leg_l, body_part_leg_r, body_part_torso, body_part_arm_l,
                    body_part_arm_r, body_part_head, body_part_eyes, body_part_mouth,
                    body_part_foot_l, body_part_foot_r, body_part_hand_l, body_part_hand_r
                }
            }, true );
        } else if( here.has_flag( TFLAG_DEEP_WATER, pos() ) ) {
            practice( skill_swimming, 1 );
            // Same as above, except no head/eyes/mouth
            drench( 100, { {
                    body_part_leg_l, body_part_leg_r, body_part_torso, body_part_arm_l,
                    body_part_arm_r, body_part_foot_l, body_part_foot_r, body_part_hand_l,
                    body_part_hand_r
                }
            }, true );
        } else if( here.has_flag( "SWIMMABLE", pos() ) ) {
            drench( 80, { { body_part_foot_l, body_part_foot_r, body_part_leg_l, body_part_leg_r } },
            false );
        }
    }

    // Try to put out clothing/hair fire
    if( has_effect( effect_onfire ) ) {
        time_duration total_removed = 0_turns;
        time_duration total_left = 0_turns;
        bool on_ground = is_prone();
        for( const bodypart_id &bp : get_all_body_parts() ) {
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
        if( total_left > 1_minutes && !is_dangerous_fields( here.field_at( pos() ) ) ) {
            add_effect( effect_downed, 2_turns, false, 0, true );
            add_msg_player_or_npc( m_warning,
                                   _( "You roll on the ground, trying to smother the fire!" ),
                                   _( "<npcname> rolls on the ground!" ) );
        } else if( total_removed > 0_turns ) {
            add_msg_player_or_npc( m_warning,
                                   _( "You attempt to put out the fire on you!" ),
                                   _( "<npcname> attempts to put out the fire on them!" ) );
        }
    }
    // put pressure on bleeding wound, prioritizing most severe bleeding
    if( !is_armed() && has_effect( effect_bleed ) ) {
        int most = 0;
        bodypart_id bp_id;
        for( const bodypart_id &bp : get_all_body_parts() ) {
            if( most <= get_effect_int( effect_bleed, bp ) ) {
                most = get_effect_int( effect_bleed, bp );
                bp_id =  bp ;
            }
        }
        effect &e = get_effect( effect_bleed, bp_id );
        int total_hand_encumb = 0;
        for( const bodypart_id &part : get_all_body_parts_of_type( body_part_type::type::hand ) ) {
            total_hand_encumb += encumb( part );
        }
        time_duration penalty = 1_turns * total_hand_encumb;
        time_duration benefit = 5_turns + 10_turns * get_skill_level( skill_firstaid );

        bool broken_arm = false;
        for( const bodypart_id &part : get_all_body_parts_of_type( body_part_type::type::arm ) ) {
            if( is_limb_broken( part ) ) {
                broken_arm = true;
                break;
            }
        }
        if( broken_arm ) {
            add_msg_player_or_npc( m_warning,
                                   _( "Your broken limb significantly hampers your efforts to put pressure on the bleeding wound!" ),
                                   _( "<npcname>'s broken limb significantly hampers their effort to put pressure on the bleeding wound!" ) );
            e.mod_duration( -1_turns );
        } else if( benefit <= penalty ) {
            add_msg_player_or_npc( m_warning,
                                   _( "Your hands are too encumbered to effectively put pressure on the bleeding wound!" ),
                                   _( "<npcname>'s hands are too encumbered to effectively put pressure on the bleeding wound!" ) );
            e.mod_duration( -1_turns );
        } else {
            e.mod_duration( - ( benefit - penalty ) );
            add_msg_player_or_npc( m_warning,
                                   _( "You attempt to put pressure on the bleeding wound!" ),
                                   _( "<npcname> attempts to put pressure on the bleeding wound!" ) );
            practice( skill_firstaid, 1 );
        }
    }
    // on-pause effects for martial arts
    martial_arts_data->ma_onpause_effects( *this );

    if( is_npc() ) {
        // The stuff below doesn't apply to NPCs
        // search_surroundings should eventually do, though
        return;
    }

    if( in_vehicle && one_in( 8 ) ) {
        VehicleList vehs = here.get_vehicles();
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
    wait_effects();
}

template <typename T>
bool player::can_lift( const T &obj ) const
{
    // avoid comparing by weight as different objects use differing scales (grams vs kilograms etc)
    int str = get_lift_str();
    if( mounted_creature ) {
        const auto mons = mounted_creature.get();
        str = mons->mech_str_addition() == 0 ? str : mons->mech_str_addition();
    }
    const int npc_str = get_lift_assist();
    return str + npc_str >= obj.lift_strength();
}
template bool player::can_lift<item>( const item &obj ) const;
template bool player::can_lift<vehicle>( const vehicle &obj ) const;

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

int player::climbing_cost( const tripoint &from, const tripoint &to ) const
{
    map &here = get_map();
    if( !here.valid_move( from, to, false, true ) ) {
        return 0;
    }

    const int diff = here.climb_difficulty( from );

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

    set_all_parts_hp_to_max();
    set_hunger( 0 );
    set_thirst( 0 );
    set_fatigue( 0 );
    set_healthy( 0 );
    set_healthy_mod( 0 );
    set_stim( 0 );
    set_pain( 0 );
    set_painkiller( 0 );
    set_rad( 0 );

    recalc_sight_limits();
    calc_encumbrance();
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

void player::add_msg_if_player( const game_message_params &params, const std::string &msg ) const
{
    Messages::add_msg( params, msg );
}

void player::add_msg_debug_if_player( debugmode::debug_filter type, const std::string &msg ) const
{
    Messages::add_msg_debug( type, msg );
}

void player::add_msg_player_or_npc( const game_message_params &params,
                                    const std::string &player_msg,
                                    const std::string &/*npc_msg*/ ) const
{
    Messages::add_msg( params, player_msg );
}

void player::add_msg_debug_player_or_npc( debugmode::debug_filter type,
        const std::string &player_msg,
        const std::string &/*npc_msg*/ ) const
{
    Messages::add_msg_debug( type, player_msg );
}

void player::add_msg_player_or_say( const std::string &player_msg,
                                    const std::string &/*npc_speech*/ ) const
{
    Messages::add_msg( player_msg );
}

void player::add_msg_player_or_say( const game_message_params &params,
                                    const std::string &player_msg,
                                    const std::string &/*npc_speech*/ ) const
{
    Messages::add_msg( params, player_msg );
}

bool player::query_yn( const std::string &mes ) const
{
    return ::query_yn( mes );
}
