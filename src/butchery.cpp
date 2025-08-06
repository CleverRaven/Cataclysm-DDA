#include "butchery.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <functional>
#include <list>
#include <memory>
#include <utility>

#include "activity_handlers.h"
#include "avatar.h"
#include "butchery_requirements.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "color.h"
#include "coordinates.h"
#include "creature.h"
#include "debug.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "flag.h"
#include "game.h"
#include "game_constants.h"
#include "handle_liquid.h"
#include "harvest.h"
#include "inventory.h"
#include "item.h"
#include "itype.h"
#include "map.h"
#include "messages.h"
#include "mtype.h"
#include "npc.h"
#include "npc_opinion.h"
#include "output.h"
#include "player_activity.h"
#include "pocket_type.h"
#include "proficiency.h"
#include "requirements.h"
#include "rng.h"
#include "string_formatter.h"
#include "text_snippets.h"
#include "translation.h"
#include "translations.h"
#include "uilist.h"
#include "units.h"
#include "weakpoint.h"

static const activity_id ACT_BLEED( "ACT_BLEED" );
static const activity_id ACT_BUTCHER( "ACT_BUTCHER" );
static const activity_id ACT_BUTCHER_FULL( "ACT_BUTCHER_FULL" );
static const activity_id ACT_DISMEMBER( "ACT_DISMEMBER" );
static const activity_id ACT_DISSECT( "ACT_DISSECT" );
static const activity_id ACT_FIELD_DRESS( "ACT_FIELD_DRESS" );
static const activity_id ACT_MULTIPLE_BUTCHER( "ACT_MULTIPLE_BUTCHER" );
static const activity_id ACT_QUARTER( "ACT_QUARTER" );
static const activity_id ACT_SKIN( "ACT_SKIN" );

static const harvest_drop_type_id harvest_drop_blood( "blood" );
static const harvest_drop_type_id harvest_drop_bone( "bone" );
static const harvest_drop_type_id harvest_drop_flesh( "flesh" );
static const harvest_drop_type_id harvest_drop_offal( "offal" );
static const harvest_drop_type_id harvest_drop_skin( "skin" );

static const itype_id itype_burnt_out_bionic( "burnt_out_bionic" );

static const json_character_flag json_flag_INSTANT_BLEED( "INSTANT_BLEED" );

static const morale_type morale_butcher( "morale_butcher" );

static const proficiency_id proficiency_prof_butchering_adv( "prof_butchering_adv" );
static const proficiency_id proficiency_prof_butchering_basic( "prof_butchering_basic" );
static const proficiency_id proficiency_prof_dissect_humans( "prof_dissect_humans" );
static const proficiency_id proficiency_prof_skinning_adv( "prof_skinning_adv" );
static const proficiency_id proficiency_prof_skinning_basic( "prof_skinning_basic" );

static const quality_id qual_BUTCHER( "BUTCHER" );
static const quality_id qual_CUT_FINE( "CUT_FINE" );

static const skill_id skill_firstaid( "firstaid" );
static const skill_id skill_survival( "survival" );

namespace io
{
    // *INDENT-OFF*
    template<>
    std::string enum_to_string<butcher_type>( butcher_type data )
    {
        switch( data ) {
        case butcher_type::BLEED: return "BLEED";
        case butcher_type::DISMEMBER: return "DISMEMBER";
        case butcher_type::DISSECT: return "DISSECT";
        case butcher_type::FIELD_DRESS: return "FIELD_DRESS";
        case butcher_type::FULL: return "FULL";
        case butcher_type::QUARTER: return "QUARTER";
        case butcher_type::QUICK: return "QUICK";
        case butcher_type::SKIN: return "SKIN";
        case butcher_type::NUM_TYPES: break;
        }
        cata_fatal( "Invalid valid_target" );
    }
    // *INDENT-ON*

} // namespace io

static bool check_butcher_dissect( const int roll )
{
    // Failure rates for dissection rolls
    // 100% at roll 0, 66% at roll 1, 50% at roll 2, 40% @ 3, 33% @ 4, 28% @ 5, ... , 16% @ 10
    // Roll is roughly a rng(0, -3 + 1st_aid + fine_cut_quality + 1/2 electronics + small_dex_bonus)
    // Roll is reduced by corpse damage level, but to no less then 0
    add_msg_debug( debugmode::DF_ACT_BUTCHER, "Roll = %i", roll );
    add_msg_debug( debugmode::DF_ACT_BUTCHER, "Failure chance = %f%%",
                   ( 10.0f / ( 10.0f + roll * 5.0f ) ) * 100.0f );
    const bool failed = x_in_y( 10, ( 10 + roll * 5 ) );
    return !failed;
}

static bool butcher_dissect_item( item &what, const tripoint_bub_ms &pos,
                                  const time_point &age, const int roll )
{
    if( roll < 0 ) {
        return false;
    }
    bool success = check_butcher_dissect( roll );
    map &here = get_map();
    if( what.is_bionic() && !success ) {
        item burnt_out_cbm( itype_burnt_out_bionic, age );
        add_msg( m_good, _( "You discover a %s!" ), burnt_out_cbm.tname() );
        here.add_item( pos, burnt_out_cbm );
    } else if( success ) {
        add_msg( m_good, _( "You discover a %s!" ), what.tname() );
        here.add_item( pos, what );
    }
    return success;
}

std::string butcher_progress_var( const butcher_type action )
{
    return io::enum_to_string( action ) + "_progress";
}

// How much of `butcher_type` has already been completed, in range [0..1], 0=not started yet, 1=completed.
// used for resuming previously started butchery
double butcher_get_progress( const item &corpse_item, const butcher_type action )
{
    return corpse_item.get_var( butcher_progress_var( action ), 0.0 );
}

butcher_type get_butcher_type( player_activity *act )
{
    butcher_type action = butcher_type::QUICK;
    if( act->id() == ACT_BUTCHER ) {
        action = butcher_type::QUICK;
    } else if( act->id() == ACT_BUTCHER_FULL ) {
        action = butcher_type::FULL;
    } else if( act->id() == ACT_FIELD_DRESS ) {
        action = butcher_type::FIELD_DRESS;
    } else if( act->id() == ACT_QUARTER ) {
        action = butcher_type::QUARTER;
    } else if( act->id() == ACT_DISSECT ) {
        action = butcher_type::DISSECT;
    } else if( act->id() == ACT_BLEED ) {
        action = butcher_type::BLEED;
    } else if( act->id() == ACT_SKIN ) {
        action = butcher_type::SKIN;
    } else if( act->id() == ACT_DISMEMBER ) {
        action = butcher_type::DISMEMBER;
    }
    return action;
}

static bool check_anger_empathetic_npcs_with_cannibalism( const Character &you,
        const mtype_id &monster )
{
    const map &here = get_map();

    if( !you.is_avatar() ) {
        return true; // NPCs dont accidentally cause player hate
    }

    bool you_empathize = you.empathizes_with_monster( monster );
    bool nearby_empathetic_npc = false;

    for( npc &guy : g->all_npcs() ) {
        if( guy.is_active() && guy.sees( here, you ) && guy.empathizes_with_monster( monster ) ) {
            nearby_empathetic_npc = true;
            break;
        }
    }

    if( you_empathize && nearby_empathetic_npc ) {
        if( !query_yn(
                _( "Really desecrate the mortal remains of this being by butchering them for meat?  You feel others may take issue with your action." ) ) ) {
            return false; // player cancels
        }
    } else if( you_empathize && !nearby_empathetic_npc ) {
        if( !query_yn(
                _( "Really desecrate the mortal remains of this being by butchering them for meat?" ) ) ) {
            return false; // player cancels
        }
    } else if( !you_empathize && nearby_empathetic_npc ) {
        if( !query_yn(
                _( "Really take this thing apart?  You feel others may take issue with your action." ) ) ) {
            return false; // player cancels
        }
    }
    // !you_empathize && !nearby_empathetic_npc means no check.  Will likely happen for most combinations.

    for( npc &guy : g->all_npcs() ) {
        if( guy.is_active() && guy.sees( here, you ) && guy.empathizes_with_monster( monster ) ) {
            guy.say( _( "<swear!>?  Are you butchering them?  That's not okay, <name_b>." ) );
            // massive opinion penalty
            guy.op_of_u.trust -= 5;
            guy.op_of_u.value -= 5;
            guy.op_of_u.anger += 5;
            if( guy.turned_hostile() ) {
                guy.make_angry();
            }
        }
    }

    return true;
}

bool set_up_butchery( player_activity &act, Character &you, butchery_data bd )
{
    const int factor = you.max_quality( bd.b_type == butcher_type::DISSECT ? qual_CUT_FINE :
                                        qual_BUTCHER, PICKUP_RANGE );

    const butcher_type action = bd.b_type;
    const item &corpse_item = *bd.corpse;
    const mtype &corpse = *corpse_item.get_mtype();

    if( bd.b_type != butcher_type::DISSECT ) {
        if( factor == 0 ) {
            you.add_msg_if_player( m_info,
                                   _( "None of your cutting tools are suitable for butchering." ) );
            act.set_to_null();
            return false;
        } else if( factor < 10 && one_in( 3 ) ) {
            you.add_msg_if_player( m_bad,
                                   _( "You don't trust the quality of your tools, but carry on anyway." ) );
        }
    }

    if( bd.b_type == butcher_type::DISSECT ) {
        switch( factor ) {
            case 0:
                you.add_msg_if_player( m_info, _( "None of your tools are sharp and precise enough to do that." ) );
                act.set_to_null();
                return false;
            case 1:
                you.add_msg_if_player( m_info, _( "You could use a better tool, but this will do." ) );
                break;
            case 2:
                you.add_msg_if_player( m_info, _( "This tool is great, but you still would like a scalpel." ) );
                break;
            case 3:
                you.add_msg_if_player( m_info, _( "You dissect the corpse with a trusty scalpel." ) );
                break;
            case 5:
                you.add_msg_if_player( m_info,
                                       _( "You dissect the corpse with a sophisticated system of surgical grade scalpels." ) );
                break;
        }
    }

    // Requirements for the various types
    const requirement_id butchery_requirement = bd.req;

    if( !butchery_requirement->can_make_with_inventory(
            you.crafting_inventory( you.pos_bub(), PICKUP_RANGE ), is_crafting_component ) ) {
        std::string popup_output = _( "You can't butcher this; you are missing some tools.\n" );

        for( const std::string &str : butchery_requirement->get_folded_components_list(
                 45, c_light_gray, you.crafting_inventory( you.pos_bub(), PICKUP_RANGE ), is_crafting_component ) ) {
            popup_output += str + '\n';
        }
        for( const std::string &str : butchery_requirement->get_folded_tools_list(
                 45, c_light_gray, you.crafting_inventory( you.pos_bub(), PICKUP_RANGE ) ) ) {
            popup_output += str + '\n';
        }

        popup( popup_output );
        return false;
    }

    if( bd.b_type == butcher_type::BLEED && ( corpse_item.has_flag( flag_BLED ) ||
            corpse_item.has_flag( flag_QUARTERED ) || corpse_item.has_flag( flag_FIELD_DRESS_FAILED ) ||
            corpse_item.has_flag( flag_FIELD_DRESS ) ) ) {
        you.add_msg_if_player( m_info, _( "This corpse has already been bled." ) );
        return false;
    }

    if( action == butcher_type::DISSECT && ( corpse_item.has_flag( flag_QUARTERED ) ||
            corpse_item.has_flag( flag_FIELD_DRESS_FAILED ) ) ) {
        you.add_msg_if_player( m_info,
                               _( "It would be futile to dissect this badly damaged corpse." ) );
        return false;
    }

    if( action == butcher_type::FIELD_DRESS && ( corpse_item.has_flag( flag_FIELD_DRESS ) ||
            corpse_item.has_flag( flag_FIELD_DRESS_FAILED ) ) ) {
        you.add_msg_if_player( m_info, _( "This corpse is already field dressed." ) );
        return false;
    }

    if( action == butcher_type::SKIN && corpse_item.has_flag( flag_SKINNED ) ) {
        you.add_msg_if_player( m_info, _( "This corpse is already skinned." ) );
        return false;
    }

    if( action == butcher_type::QUARTER ) {
        if( corpse.size == creature_size::tiny ) {
            you.add_msg_if_player( m_bad, _( "This corpse is too small to quarter without damaging." ),
                                   corpse.nname() );
            return false;
        }
        if( corpse_item.has_flag( flag_QUARTERED ) ) {
            you.add_msg_if_player( m_bad, _( "This is already quartered." ), corpse.nname() );
            return false;
        }
        if( !( corpse_item.has_flag( flag_FIELD_DRESS ) ||
               corpse_item.has_flag( flag_FIELD_DRESS_FAILED ) ) &&
            corpse_item.get_mtype()->harvest->has_entry_type( harvest_drop_offal ) ) {
            you.add_msg_if_player( m_bad, _( "You need to perform field dressing before quartering." ),
                                   corpse.nname() );
            return false;
        }
    }

    // Dissections are slightly less angering than other butcher types
    if( action == butcher_type::DISSECT ) {
        if( you.has_proficiency( proficiency_prof_dissect_humans ) ) {
            if( you.empathizes_with_monster( corpse.id ) ) {
                // this is a dissection, and we are trained for dissection, so no morale penalty, anger, and lighter flavor text.
                you.add_msg_if_player( m_good, SNIPPET.random_from_category(
                                           "msg_human_dissection_with_prof" ).value_or( translation() ).translated() );
            }
        } else {
            if( check_anger_empathetic_npcs_with_cannibalism( you, corpse.id ) ) {
                if( you.empathizes_with_monster( corpse.id ) ) {
                    // give us a message indicating we are dissecting without the stomach for it, but not actually butchering. lower morale penalty.
                    you.add_msg_if_player( m_good, SNIPPET.random_from_category(
                                               "msg_human_dissection_no_prof" ).value_or( translation() ).translated() );
                    you.add_morale( morale_butcher, -40, 0, 1_days, 2_hours );
                }
            } else {
                // standard refusal to butcher
                you.add_msg_if_player( m_good, _( "It needs a coffin, not a knife." ) );
                return false;
            }
        }
    } else if( action != butcher_type::DISMEMBER ) {
        if( check_anger_empathetic_npcs_with_cannibalism( you, corpse.id ) ) {
            if( you.empathizes_with_monster( corpse.id ) ) {
                // give the player a random message showing their disgust and cause morale penalty.
                you.add_msg_if_player( m_good, SNIPPET.random_from_category(
                                           "msg_human_butchery" ).value_or( translation() ).translated() );
                you.add_morale( morale_butcher, -50, 0, 2_days, 3_hours );
            }
        } else {
            if( you.has_proficiency( proficiency_prof_dissect_humans ) ) {
                // player has the dissect_humans prof, so they're familiar with dissection. reference this in their refusal to butcher
                you.add_msg_if_player( m_good, _( "You were trained for autopsies, not butchery." ) );
                return false;
            } else {
                // player doesn't have dissect_humans, so just give a regular refusal to mess with the corpse
                you.add_msg_if_player( m_good, _( "It needs a coffin, not a knife." ) );
                return false;
            }
        }
    }

    return true;
}

int butcher_time_to_cut( Character &you, const item &corpse_item, const butcher_type action )
{
    const mtype &corpse = *corpse_item.get_mtype();
    const int factor = you.max_quality( action == butcher_type::DISSECT ? qual_CUT_FINE : qual_BUTCHER,
                                        PICKUP_RANGE );
    // in moves
    int time_to_cut;
    switch( corpse.size ) {
        // Time (roughly) in turns to cut up the corpse
        case creature_size::tiny:
            time_to_cut = 150;
            break;
        case creature_size::small:
            time_to_cut = 300;
            break;
        case creature_size::medium:
            time_to_cut = 450;
            break;
        case creature_size::large:
            time_to_cut = 600;
            break;
        case creature_size::huge:
            time_to_cut = 1800;
            break;
        default:
            debugmsg( "ERROR: Invalid creature_size on %s", corpse.nname() );
            time_to_cut = 450; // default to medium
            break;
    }

    // At factor 0, base 100 time_to_cut remains 100. At factor 50, it's 50 , at factor 75 it's 25
    time_to_cut *= std::max( 25, 100 - factor );

    switch( action ) {
        case butcher_type::QUICK:
            break;
        case butcher_type::BLEED:
            if( you.has_flag( json_flag_INSTANT_BLEED ) ) {
                time_to_cut = 1;
            }
            break;
        case butcher_type::FULL:
            if( !corpse_item.has_flag( flag_FIELD_DRESS ) || corpse_item.has_flag( flag_FIELD_DRESS_FAILED ) ) {
                time_to_cut *= 6;
            } else {
                time_to_cut *= 4;
            }
            break;
        case butcher_type::FIELD_DRESS:
        case butcher_type::SKIN:
            time_to_cut *= 2;
            break;
        case butcher_type::QUARTER:
            time_to_cut /= 4;
            if( time_to_cut < 1200 ) {
                time_to_cut = 1200;
            }
            break;
        case butcher_type::DISMEMBER:
            time_to_cut /= 10;
            if( time_to_cut < 600 ) {
                time_to_cut = 600;
            }
            break;
        case butcher_type::DISSECT:
            time_to_cut *= 6;
            break;
        case butcher_type::NUM_TYPES:
            debugmsg( "ERROR: Invalid butcher_type" );
            break;
    }

    if( corpse_item.has_flag( flag_QUARTERED ) ) {
        time_to_cut /= 4;
    }

    double butch_basic = you.get_proficiency_practice( proficiency_prof_butchering_basic );
    double butch_adv = you.get_proficiency_practice( proficiency_prof_butchering_adv );
    double skin_basic = you.get_proficiency_practice( proficiency_prof_skinning_basic );
    double penalty_small = 0.5;
    double penalty_big = 1.5;

    int prof_butch_penalty = penalty_big * ( 1 - butch_basic ) + penalty_small * ( 1 - butch_adv );
    int prof_skin_penalty = penalty_small * ( 1 - skin_basic );

    // there supposed to be a code for book mitigation, but we don't have any book fitting for this

    if( action == butcher_type::FULL ) {
        // 40% of butchering and gutting, 40% of skinning, 20% another activities
        time_to_cut *= 0.4 * ( 1 + prof_butch_penalty ) + 0.4 * ( 1 + prof_skin_penalty ) + 0.2;
    }

    if( action == butcher_type::QUICK ) {
        // 70% of butchery, 15% skinning, 15% another activities
        time_to_cut *= 0.7 * ( 1 + prof_butch_penalty ) + 0.15 * ( 1 + prof_skin_penalty ) + 0.15;
    }

    if( action == butcher_type::FIELD_DRESS ) {
        time_to_cut *= 1 + prof_butch_penalty;
    }

    if( action == butcher_type::SKIN ) {
        time_to_cut *= 1 + prof_skin_penalty;
    }

    time_to_cut *= ( 1.0f - ( get_player_character().get_num_crafting_helpers( 3 ) / 10.0f ) );
    return time_to_cut;
}

// this function modifies the input weight by its damage level, depending on the bodypart
static int corpse_damage_effect( int weight, const harvest_drop_type_id &entry_type,
                                 int damage_level )
{
    const float slight_damage = 0.9f;
    const float damage = 0.75f;
    const float high_damage = 0.5f;
    const int destroyed = 0;

    switch( damage_level ) {
        case 2:
            // "damaged"
            if( entry_type == harvest_drop_offal ) {
                return std::round( weight * damage );
            }
            if( entry_type == harvest_drop_skin ) {
                return std::round( weight * damage );
            }
            if( entry_type == harvest_drop_flesh ) {
                return std::round( weight * slight_damage );
            }
            break;
        case 3:
            // "mangled"
            if( entry_type == harvest_drop_offal ) {
                return destroyed;
            }
            if( entry_type == harvest_drop_skin ) {
                return std::round( weight * high_damage );
            }
            if( entry_type == harvest_drop_bone ) {
                return std::round( weight * slight_damage );
            }
            if( entry_type == harvest_drop_flesh ) {
                return std::round( weight * damage );
            }
            break;
        case 4:
            // "pulped"
            if( entry_type == harvest_drop_offal ) {
                return destroyed;
            }
            if( entry_type == harvest_drop_skin ) {
                return destroyed;
            }
            if( entry_type == harvest_drop_bone ) {
                return std::round( weight * damage );
            }
            if( entry_type == harvest_drop_flesh ) {
                return std::round( weight * high_damage );
            }
            break;
        default:
            // "bruised" modifier is almost impossible to avoid; also includes no modifier (zero damage)
            break;
    }
    return weight;
}

static int butchery_dissect_skill_level( Character &you, int tool_quality,
        const harvest_drop_type_id &htype )
{
    // DISSECT has special case factor calculation and results.
    if( htype.is_valid() ) {
        float sk_total = 0;
        int sk_count = 0;
        for( const skill_id &sk : htype->get_harvest_skills() ) {
            sk_total += you.get_average_skill_level( sk );
            sk_count++;
        }
        return round( ( sk_total + tool_quality ) / ( sk_count > 0 ? sk_count : 1 ) );
    }
    return round( you.get_average_skill_level( skill_survival ) );
}

int roll_butchery_dissect( int skill_level, int dex, int tool_quality )
{
    double skill_shift = 0.0;
    ///\EFFECT_SURVIVAL randomly increases butcher rolls
    skill_shift += rng_float( 0, skill_level - 3 );
    ///\EFFECT_DEX >8 randomly increases butcher rolls, slightly, <8 decreases
    skill_shift += rng_float( 0, dex - 8 ) / 4.0;

    if( tool_quality < 0 ) {
        skill_shift -= rng_float( 0, -tool_quality / 5.0 );
    } else {
        skill_shift += std::min( tool_quality, 4 );
    }

    return static_cast<int>( std::round( skill_shift ) );
}

// A number between 0 and 1 that represents how much usable material can be harvested
// as a fraction of the maximum possible amount.
static double butchery_dissect_yield_mult( int skill_level, int dex, int tool_quality )
{
    const double skill_score = skill_level / 10.0;
    const double tool_score = ( tool_quality + 50.0 ) / 100.0;
    const double dex_score = dex / 20.0;
    return 0.5 * clamp( skill_score, 0.0, 1.0 ) +
           0.3 * clamp( tool_score, 0.0, 1.0 ) +
           0.2 * clamp( dex_score, 0.0, 1.0 );
}

static std::vector<item> create_charge_items( const itype *drop, int count,
        const harvest_entry &entry, const item *corpse_item, const Character &you )
{
    std::vector<item> objs;
    while( count > 0 ) {
        item obj( drop, calendar::turn, 1 );
        obj.charges = std::min( count, DEFAULT_TILE_VOLUME / obj.volume() );
        count -= obj.charges;

        if( obj.has_temperature() ) {
            obj.set_item_temperature( corpse_item->temperature );
            if( obj.goes_bad() ) {
                obj.set_rot( corpse_item->get_rot() );
            }
        }
        for( const flag_id &flg : entry.flags ) {
            obj.set_flag( flg );
        }
        for( const fault_id &flt : entry.faults ) {
            obj.set_fault( flt );
        }
        if( !you.backlog.empty() && you.backlog.front().id() == ACT_MULTIPLE_BUTCHER ) {
            obj.set_var( "activity_var", you.name );

        }
        objs.push_back( obj );
    }
    return objs;
}

bool butchery_drops_harvest( butchery_data bt, Character &you )
{
    const butcher_type action = bt.b_type;
    item corpse_item = *bt.corpse.get_item();
    const mtype &mt = *corpse_item.get_mtype();
    const time_duration moves_total = bt.time_to_butcher;

    const int tool_quality = you.max_quality( action == butcher_type::DISSECT ? qual_CUT_FINE :
                             qual_BUTCHER, PICKUP_RANGE );

    //all BUTCHERY types - FATAL FAILURE
    if( action != butcher_type::DISSECT &&
        roll_butchery_dissect( round( you.get_average_skill_level( skill_survival ) ), you.dex_cur,
                               tool_quality ) <= ( -15 ) && one_in( 2 ) ) {
        return false;
    }

    you.add_msg_if_player( m_neutral, mt.harvest->message() );
    int monster_weight = to_gram( mt.weight );
    monster_weight += std::round( monster_weight * rng_float( -0.1, 0.1 ) );
    if( corpse_item.has_flag( flag_QUARTERED ) ) {
        monster_weight *= 0.95;
    }
    if( corpse_item.has_flag( flag_GIBBED ) ) {
        monster_weight = std::round( 0.85 * monster_weight );
        if( action != butcher_type::FIELD_DRESS ) {
            you.add_msg_if_player( m_bad,
                                   _( "You salvage what you can from the corpse, but it is badly damaged." ) );
        }
    }
    if( corpse_item.has_flag( flag_UNDERFED ) ) {
        monster_weight = std::round( 0.9 * monster_weight );
        if( action != butcher_type::FIELD_DRESS && action != butcher_type::SKIN &&
            action != butcher_type::DISSECT ) {
            you.add_msg_if_player( m_bad,
                                   _( "The corpse looks a little underweight�" ) );
        }
    }
    if( corpse_item.has_flag( flag_SKINNED ) ) {
        monster_weight = std::round( 0.85 * monster_weight );
    }
    const int entry_count = ( action == butcher_type::DISSECT &&
                              !mt.dissect.is_empty() ) ? mt.dissect->get_all().size() : mt.harvest->get_all().size();
    int monster_weight_remaining = monster_weight;
    int practice = 0;

    if( mt.harvest.is_null() ) {
        debugmsg( "ERROR: %s has no harvest entry.", mt.id.c_str() );
        return false;
    }

    if( action == butcher_type::DISSECT )    {
        int dissectable_practice = 0;
        int dissectable_num = 0;
        for( item *item : corpse_item.all_items_top( pocket_type::CORPSE ) ) {
            dissectable_num++;
            const int skill_level = butchery_dissect_skill_level( you, tool_quality,
                                    item->dropped_from );
            const int butchery = roll_butchery_dissect( skill_level, you.dex_cur, tool_quality );
            dissectable_practice += ( 4 + butchery );
            int roll = butchery - corpse_item.damage_level();
            roll = roll < 0 ? 0 : roll;
            add_msg_debug( debugmode::DF_ACT_BUTCHER, "Roll penalty for corpse damage = %s",
                           0 - corpse_item.damage_level() );
            butcher_dissect_item( *item, you.pos_bub(), calendar::turn, roll );
        }
        if( dissectable_num > 0 ) {
            practice += dissectable_practice / dissectable_num;
        }
    }

    map &here = get_map();
    const tripoint_bub_ms corpse_loc = bt.corpse.pos_bub( here );

    for( const harvest_entry &entry : ( action == butcher_type::DISSECT &&
                                        !mt.dissect.is_empty() ) ? *mt.dissect : *mt.harvest ) {
        const int skill_level = butchery_dissect_skill_level( you, tool_quality, entry.type );
        const int butchery = roll_butchery_dissect( skill_level, you.dex_cur, tool_quality );
        practice += ( 4 + butchery ) / entry_count;
        const float min_num = entry.base_num.first + butchery * entry.scale_num.first;
        const float max_num = entry.base_num.second + butchery * entry.scale_num.second;

        int roll = 0;
        // mass_ratio will override the use of base_num, scale_num, and max
        if( entry.mass_ratio != 0.00f ) {
            roll = static_cast<int>( std::round( entry.mass_ratio * monster_weight ) );
            roll = corpse_damage_effect( roll, entry.type, corpse_item.damage_level() );
        } else {
            roll = std::min<int>( entry.max, std::round( rng_float( min_num, max_num ) ) );
            // will not give less than min_num defined in the JSON
            roll = std::max<int>( corpse_damage_effect( roll, entry.type, corpse_item.damage_level() ),
                                  entry.base_num.first );
        }

        itype_id drop_id = itype_id::NULL_ID();
        const itype *drop = nullptr;
        if( entry.type->is_itype() ) {
            drop_id = itype_id( entry.drop );
            drop = item::find_type( drop_id );
        }

        // Check if monster was gibbed, and handle accordingly
        if( corpse_item.has_flag( flag_GIBBED ) && ( entry.type == harvest_drop_flesh ||
                entry.type == harvest_drop_bone ) ) {
            roll /= 2;
        }
        if( corpse_item.has_flag( flag_UNDERFED ) && ( entry.type == harvest_drop_flesh ) ) {
            roll /= 1.6;
        }

        if( corpse_item.has_flag( flag_SKINNED ) && entry.type == harvest_drop_skin ) {
            roll = 0;
        }

        const double butch_basic = you.get_proficiency_practice( proficiency_prof_butchering_basic );
        const double skin_basic = you.get_proficiency_practice( proficiency_prof_skinning_basic );
        const double skin_adv = you.get_proficiency_practice( proficiency_prof_skinning_adv );
        const double penalty_small = 0.15;
        const double penalty_big = 2;

        if( entry.type == harvest_drop_flesh || entry.type == harvest_drop_offal ) {
            roll /= 1 + ( penalty_small * ( 1 - butch_basic ) );
        }

        if( entry.type == harvest_drop_skin ) {
            roll /= 1 + ( penalty_big * ( 1 - skin_basic ) );
            roll /= 1 + ( penalty_small * ( 1 - skin_adv ) );
        }

        // QUICK BUTCHERY
        if( action == butcher_type::QUICK ) {
            if( entry.type == harvest_drop_flesh ) {
                roll /= 4;
            } else if( entry.type == harvest_drop_bone ) { // NOLINT(bugprone-branch-clone)
                roll /= 2;
            } else if( mt.size >= creature_size::medium &&
                       entry.type == harvest_drop_skin ) {
                roll /= 2;
            } else if( entry.type == harvest_drop_offal ) {
                roll /= 5;
            } else {
                continue;
            }
        }
        // RIP AND TEAR
        if( action == butcher_type::DISMEMBER ) {
            roll = 0;
        }
        // field dressing ignores skin, flesh, and blood
        if( action == butcher_type::FIELD_DRESS ) {
            if( entry.type == harvest_drop_bone ) {
                roll = rng( 0, roll / 2 );
            }
            if( entry.type == harvest_drop_flesh ) {
                continue;
            }
            if( entry.type == harvest_drop_skin ) {
                continue;
            }
        }

        // you only get the blood from bleeding
        if( action == butcher_type::BLEED ) {
            if( entry.type != harvest_drop_blood ) {
                continue;
            }
        }

        // you only get the skin from skinning
        if( action == butcher_type::SKIN ) {
            if( entry.type != harvest_drop_skin ) {
                continue;
            }
            if( corpse_item.has_flag( flag_FIELD_DRESS_FAILED ) ) {
                roll = rng( 0, roll );
            }
        }

        // field dressing removed innards and bones from meatless limbs
        if( ( action == butcher_type::FULL || action == butcher_type::QUICK ) &&
            corpse_item.has_flag( flag_FIELD_DRESS ) ) {
            if( entry.type == harvest_drop_offal ) {
                continue;
            }
            if( entry.type == harvest_drop_bone ) {
                roll = ( roll / 2 ) + rng( roll / 2, roll );
            }
        }
        // unskillfull field dressing may damage the skin, meat, and other parts
        if( ( action == butcher_type::FULL || action == butcher_type::QUICK ) &&
            corpse_item.has_flag( flag_FIELD_DRESS_FAILED ) ) {
            if( entry.type == harvest_drop_offal ) {
                continue;
            }
            if( entry.type == harvest_drop_bone ) {
                roll = ( roll / 2 ) + rng( roll / 2, roll );
            }
            if( entry.type == harvest_drop_flesh || entry.type == harvest_drop_skin ) {
                roll = rng( 0, roll );
            }
        }
        // quartering ruins skin
        if( corpse_item.has_flag( flag_QUARTERED ) ) {
            if( entry.type == harvest_drop_skin ) {
                //not continue to show fail effect
                roll = 0;
            } else {
                roll /= 4;
            }
        }

        if( drop != nullptr ) {
            // divide total dropped weight by drop's weight to get amount
            if( entry.mass_ratio != 0.00f ) {
                // apply skill before converting to items, but only if mass_ratio is defined
                roll *= butchery_dissect_yield_mult( skill_level, you.dex_cur, tool_quality );
                monster_weight_remaining -= roll;
                roll = std::ceil( static_cast<double>( roll ) /
                                  to_gram( drop->weight ) );
            } else {
                monster_weight_remaining -= roll * to_gram( drop->weight );
            }

            const translation msg = action == butcher_type::FIELD_DRESS ?
                                    entry.type->field_dress_msg( roll > 0 ) :
                                    action == butcher_type::QUICK || action == butcher_type::FULL ||
                                    action == butcher_type::DISMEMBER ? entry.type->butcher_msg( roll > 0 ) : translation();
            if( !msg.empty() ) {
                you.add_msg_if_player( m_bad, msg.translated() );
            }

            if( roll <= 0 ) {
                you.add_msg_if_player( m_bad, _( "You fail to harvest: %s" ), drop->nname( 1 ) );
                continue;
            }
            if( drop->phase == phase_id::LIQUID ) {
                item obj( drop, calendar::turn, roll );
                if( obj.has_temperature() ) {
                    obj.set_item_temperature( corpse_item.temperature );
                    if( obj.goes_bad() ) {
                        obj.set_rot( corpse_item.get_rot() );
                    }
                }
                for( const flag_id &flg : entry.flags ) {
                    obj.set_flag( flg );
                }
                for( const fault_id &flt : entry.faults ) {
                    obj.remove_fault( flt );
                }

                // If we're not bleeding the animal we don't care about the blood being wasted
                if( action != butcher_type::BLEED ) {
                    drop_on_map( you, item_drop_reason::deliberate, { obj }, &here, corpse_loc );
                } else {
                    liquid_handler::handle_all_or_npc_liquid( you, obj, 1 );
                }
            } else if( drop->count_by_charges() ) {
                std::vector<item> objs = create_charge_items( drop, roll, entry, &corpse_item, you );
                for( item &obj : objs ) {
                    item_location loc = here.add_item_or_charges_ret_loc( corpse_loc, obj );
                    if( loc ) {
                        you.may_activity_occupancy_after_end_items_loc.push_back( loc );
                    }
                }
            } else {
                item obj( drop, calendar::turn );
                obj.set_mtype( &mt );
                if( obj.has_temperature() ) {
                    obj.set_item_temperature( corpse_item.temperature );
                    if( obj.goes_bad() ) {
                        obj.set_rot( corpse_item.get_rot() );
                    }
                }
                for( const flag_id &flg : entry.flags ) {
                    obj.set_flag( flg );
                }
                for( const fault_id &flt : entry.faults ) {
                    obj.remove_fault( flt );
                }
                if( !you.backlog.empty() && you.backlog.front().id() == ACT_MULTIPLE_BUTCHER ) {
                    obj.set_var( "activity_var", you.name );
                }
                for( int i = 0; i != roll; ++i ) {
                    item_location loc = here.add_item_or_charges_ret_loc( corpse_loc, obj );
                    if( loc ) {
                        you.may_activity_occupancy_after_end_items_loc.push_back( loc );
                    }
                }
            }
            you.add_msg_if_player( m_good, _( "You harvest: %s" ), drop->nname( roll ) );
        }
        practice++;
    }
    // 20% of the original corpse weight is not an item, but liquid gore
    monster_weight_remaining -= monster_weight / 5;
    // add the remaining unusable weight as rotting garbage
    if( monster_weight_remaining > 0 && action != butcher_type::BLEED ) {
        if( action == butcher_type::FIELD_DRESS ) {
            // 25% of the corpse weight is what's removed during field dressing
            monster_weight_remaining -= monster_weight * 3 / 4;
        } else if( action == butcher_type::SKIN ) {
            monster_weight_remaining -= monster_weight * 0.85;
        } else {
            // a carcass is 75% of the weight of the unmodified creature's weight
            if( ( corpse_item.has_flag( flag_FIELD_DRESS ) ||
                  corpse_item.has_flag( flag_FIELD_DRESS_FAILED ) ) &&
                !corpse_item.has_flag( flag_QUARTERED ) ) {
                monster_weight_remaining -= monster_weight / 4;
            } else if( corpse_item.has_flag( flag_QUARTERED ) ) {
                monster_weight_remaining -= ( monster_weight - ( monster_weight * 3 / 4 / 4 ) );
            }
            if( corpse_item.has_flag( flag_SKINNED ) ) {
                monster_weight_remaining -= monster_weight * 0.15;
            }
        }
        const itype_id &leftover_id = mt.harvest->leftovers;
        const int item_charges = monster_weight_remaining / to_gram( item::find_type(
                                     leftover_id )->weight );
        if( item_charges > 0 ) {
            item ruined_parts( leftover_id, calendar::turn );
            ruined_parts.set_mtype( &mt );
            ruined_parts.set_item_temperature( corpse_item.temperature );
            ruined_parts.set_rot( corpse_item.get_rot() );
            if( !you.backlog.empty() && you.backlog.front().id() == ACT_MULTIPLE_BUTCHER ) {
                ruined_parts.set_var( "activity_var", you.name );
            }
            for( int i = 0; i < item_charges; ++i ) {
                item_location loc = here.add_item_or_charges_ret_loc( corpse_loc, ruined_parts );
                if( loc ) {
                    you.may_activity_occupancy_after_end_items_loc.push_back( loc );
                }
            }
        }
    }

    if( action == butcher_type::DISSECT ) {
        you.practice( skill_firstaid, std::max( 0, practice ), mt.size + std::min( tool_quality, 3 ) + 2 );
        mt.families.practice_dissect( you );
    } else {
        you.practice( skill_survival, std::max( 0, practice ), std::max( mt.size - creature_size::medium,
                      0 ) + 4 );
    }

    // handle our prof training
    if( action == butcher_type::FULL && ( mt.harvest->has_entry_type( harvest_drop_flesh ) ||
                                          mt.harvest->has_entry_type( harvest_drop_offal ) ) ) {
        // 40% of butchering and gutting, 40% of skinning, 20% another activities
        if( you.has_proficiency( proficiency_prof_butchering_basic ) ) {
            you.practice_proficiency( proficiency_prof_butchering_adv, moves_total * 0.4 );
        } else {
            you.practice_proficiency( proficiency_prof_butchering_basic, moves_total * 0.4 );
        }
    }

    if( action == butcher_type::FULL && mt.harvest->has_entry_type( harvest_drop_skin ) ) {
        // 40% of butchering and gutting, 40% of skinning, 20% another activities
        if( you.has_proficiency( proficiency_prof_skinning_basic ) ) {
            you.practice_proficiency( proficiency_prof_skinning_adv, moves_total * 0.4 );
        } else {
            you.practice_proficiency( proficiency_prof_skinning_basic, moves_total * 0.4 );
        }
    }

    if( action == butcher_type::QUICK && ( mt.harvest->has_entry_type( harvest_drop_flesh ) ||
                                           mt.harvest->has_entry_type( harvest_drop_offal ) ) ) {
        // 70% of butchery, 15% skinning, 15% another activities
        if( you.has_proficiency( proficiency_prof_butchering_basic ) ) {
            you.practice_proficiency( proficiency_prof_butchering_adv, moves_total * 0.7 );
        } else {
            you.practice_proficiency( proficiency_prof_butchering_basic, moves_total * 0.7 );
        }
    }

    if( action == butcher_type::QUICK && mt.harvest->has_entry_type( harvest_drop_skin ) ) {
        // 70% of butchery, 15% skinning, 15% another activities
        if( you.has_proficiency( proficiency_prof_skinning_basic ) ) {
            you.practice_proficiency( proficiency_prof_skinning_adv, moves_total * 0.15 );
        } else {
            you.practice_proficiency( proficiency_prof_skinning_basic, moves_total * 0.15 );
        }
    }

    if( action == butcher_type::FIELD_DRESS && ( mt.harvest->has_entry_type( harvest_drop_flesh ) ||
            mt.harvest->has_entry_type( harvest_drop_offal ) ) ) {
        if( you.has_proficiency( proficiency_prof_butchering_basic ) ) {
            you.practice_proficiency( proficiency_prof_butchering_adv, moves_total );
        } else {
            you.practice_proficiency( proficiency_prof_butchering_basic, moves_total );
        }
    }

    if( action == butcher_type::SKIN && mt.harvest->has_entry_type( harvest_drop_skin ) ) {
        // 70% of butchery, 15% skinning, 15% another activities
        if( you.has_proficiency( proficiency_prof_skinning_basic ) ) {
            you.practice_proficiency( proficiency_prof_skinning_adv, moves_total );
        } else {
            you.practice_proficiency( proficiency_prof_skinning_basic, moves_total );
        }
    }

    // after this point, if there was a liquid handling from the harvest,
    // and the liquid handling was interrupted, then the activity was canceled,
    // therefore operations on this activity's targets and values may be invalidated.
    // reveal hidden items / hidden content
    if( action != butcher_type::FIELD_DRESS && action != butcher_type::SKIN &&
        action != butcher_type::BLEED && action != butcher_type:: DISMEMBER ) {
        for( item *content : corpse_item.all_items_top( pocket_type::CONTAINER ) ) {
            if( ( roll_butchery_dissect( round( you.get_average_skill_level( skill_survival ) ), you.dex_cur,
                                         tool_quality ) + 10 ) * 5 > rng( 0, 100 ) ) {
                //~ %1$s - item name, %2$s - monster name
                you.add_msg_if_player( m_good, _( "You discover a %1$s in the %2$s!" ), content->tname(),
                                       corpse_item.get_mtype()->nname() );
                here.add_item_or_charges( you.pos_bub(), *content );
            } else if( content->is_bionic() ) {
                here.spawn_item( you.pos_bub(), itype_burnt_out_bionic, 1, 0, calendar::turn );
            }
        }
    }

    you.invalidate_crafting_inventory();
    return true;
}

void butchery_quarter( item *corpse_item, const Character &you )
{
    corpse_item->set_flag( flag_QUARTERED );
    you.add_msg_if_player( m_good,
                           _( "You roughly slice the corpse of %s into four parts and set them aside." ),
                           corpse_item->get_mtype()->nname() );
    map &here = get_map();
    tripoint_bub_ms pos = you.pos_bub();

    // 4 quarters (one exists, add 3, flag does the rest)
    for( int i = 1; i <= 3; i++ ) {
        here.add_item_or_charges( pos, *corpse_item, true );
    }
}

void destroy_the_carcass( const butchery_data &bd, Character &you )
{
    map &here = get_map();

    item_location target = bd.corpse;
    const tripoint_bub_ms corpse_pos = target.pos_bub( here );
    const butcher_type action = bd.b_type;
    item &corpse_item = *target;
    const mtype *corpse = corpse_item.get_mtype();
    const field_type_id type_blood = corpse->bloodType();
    const field_type_id type_gib = corpse->gibType();

    corpse_item.erase_var( butcher_progress_var( action ) );

    if( action == butcher_type::QUARTER ) {
        butchery_quarter( &corpse_item, you );
        return;
    }

    if( action == butcher_type::DISMEMBER ) {
        here.add_splatter( type_gib, corpse_pos, rng( corpse->size + 2,
                           ( corpse->size + 1 ) * 2 ) );
    }

    // all action types - yields
    if( !butchery_drops_harvest( bd, you ) ) {
        // FATAL FAILURE
        add_msg( m_warning, SNIPPET.random_from_category( "harvest_drop_default_dissect_failed" ).value_or(
                     translation() ).translated() );

        // Remove the target from the map
        target.remove_item();

        here.add_splatter( type_gib, corpse_pos, rng( corpse->size + 2, ( corpse->size + 1 ) * 2 ) );
        here.add_splatter( type_blood, corpse_pos, rng( corpse->size + 2, ( corpse->size + 1 ) * 2 ) );
        for( int i = 1; i <= corpse->size; i++ ) {
            here.add_splatter_trail( type_gib, corpse_pos,
                                     random_entry( here.points_in_radius( corpse_pos,
                                                   corpse->size + 1 ) ) );
            here.add_splatter_trail( type_blood, corpse_pos,
                                     random_entry( here.points_in_radius( corpse_pos,
                                                   corpse->size + 1 ) ) );
        }

        return;
    }

    //end messages and effects
    switch( action ) {
        case butcher_type::QUARTER:
            break;
        case butcher_type::QUICK:
            add_msg( m_good, SNIPPET.random_from_category( "harvest_drop_default_quick_butcher" ).value_or(
                         translation() ).translated() );
            // Remove the target from the map
            target.remove_item();
            break;
        case butcher_type::FULL:
            add_msg( m_good, SNIPPET.random_from_category( "harvest_drop_default_full_butcher" ).value_or(
                         translation() ).translated() );

            // Remove the target from the map
            target.remove_item();
            break;
        case butcher_type::FIELD_DRESS: {
            bool success = roll_butchery_dissect( round( you.get_average_skill_level( skill_survival ) ),
                                                  you.dex_cur,
                                                  you.max_quality( qual_BUTCHER, PICKUP_RANGE ) ) > 0;
            add_msg( success ? m_good : m_warning,
                     SNIPPET.random_from_category( success ? "harvest_drop_default_field_dress_success" :
                                                   "harvest_drop_default_field_dress_failed" ).value_or( translation() ).translated() );
            corpse_item.set_flag( success ? flag_FIELD_DRESS : flag_FIELD_DRESS_FAILED );
            here.add_splatter( type_gib, corpse_pos, rng( corpse->size + 2, ( corpse->size + 1 ) * 2 ) );
            here.add_splatter( type_blood, corpse_pos, rng( corpse->size + 2, ( corpse->size + 1 ) * 2 ) );
            for( int i = 1; i <= corpse->size; i++ ) {
                here.add_splatter_trail( type_gib, corpse_pos,
                                         random_entry( here.points_in_radius( corpse_pos,
                                                       corpse->size + 1 ) ) );
                here.add_splatter_trail( type_blood, corpse_pos,
                                         random_entry( here.points_in_radius( corpse_pos,
                                                       corpse->size + 1 ) ) );
            }
            break;
        }
        case butcher_type::SKIN:
            add_msg( m_good, SNIPPET.random_from_category( "harvest_drop_default_skinning" ).value_or(
                         translation() ).translated() );
            corpse_item.set_flag( flag_SKINNED );
            break;
        case butcher_type::BLEED:
            add_msg( m_good, SNIPPET.random_from_category( "harvest_drop_default_bleed" ).value_or(
                         translation() ).translated() );
            corpse_item.set_flag( flag_BLED );
            break;
        case butcher_type::DISMEMBER:
            add_msg( m_good, SNIPPET.random_from_category( "harvest_drop_default_dismember" ).value_or(
                         translation() ).translated() );
            // Remove the target from the map
            target.remove_item();
            break;
        case butcher_type::DISSECT:
            add_msg( m_good, SNIPPET.random_from_category( "harvest_drop_default_dissect_success" ).value_or(
                         translation() ).translated() );

            // Remove the target from the map
            target.remove_item();
            break;
        case butcher_type::NUM_TYPES:
            debugmsg( "ERROR: Invalid butcher_type" );
            break;
    }

    you.recoil = MAX_RECOIL;

    get_event_bus().send<event_type::character_butchered_corpse>( you.getID(),
            corpse_item.get_mtype()->id, "ACT_" + io::enum_to_string<butcher_type>( bd.b_type ) );

}

// Butchery sub-menu and time calculation
std::optional<butcher_type> butcher_submenu( const std::vector<map_stack::iterator> &corpses,
        int index )
{
    avatar &player_character = get_avatar();
    constexpr int num_butcher_types = static_cast<int>( butcher_type::NUM_TYPES );

    std::array<time_duration, num_butcher_types> cut_times;
    std::array<bool, num_butcher_types> has_started;
    for( int bt_i = 0; bt_i < num_butcher_types; bt_i++ ) {
        const butcher_type bt = static_cast<butcher_type>( bt_i );
        int time_to_cut = 0;
        if( index != -1 ) {
            const mtype &corpse = *corpses[index]->get_mtype();
            const float factor = corpse.harvest->get_butchery_requirements().get_fastest_requirements(
                                     player_character.crafting_inventory(),
                                     corpse.size, bt ).first;
            time_to_cut = butcher_time_to_cut( player_character, *corpses[index], bt ) * factor;
            has_started[bt_i] = butcher_get_progress( *corpses[index], bt ) > 0;
        } else {
            has_started[bt_i] = false;
            for( const map_stack::iterator &it : corpses ) {
                const mtype &corpse = *it->get_mtype();
                const float factor = corpse.harvest->get_butchery_requirements().get_fastest_requirements(
                                         player_character.crafting_inventory(),
                                         corpse.size, bt ).first;
                time_to_cut += butcher_time_to_cut( player_character, *it, bt ) * factor;
                has_started[bt_i] |= butcher_get_progress( *it, bt ) > 0;
            }
        }
        cut_times[bt_i] = time_duration::from_moves( time_to_cut );
    }
    auto cut_time = [&]( butcher_type bt ) {
        return to_string_clipped( cut_times[static_cast<int>( bt )] );
    };
    auto progress_str = [&]( butcher_type bt ) {
        std::string result;
        if( index != -1 ) {
            const double progress = butcher_get_progress( *corpses[index], bt );
            if( progress > 0 ) {
                result = string_format( _( "%d%% complete" ), static_cast<int>( 100 * progress ) );
            }
        } else {
            for( const map_stack::iterator &it : corpses ) {
                const double progress = butcher_get_progress( *it, bt );
                if( progress > 0 ) {
                    result = _( "partially complete" );
                }
            }
        }
        return result.empty() ? "" : ( " " + colorize( result, c_dark_gray ) );
    };
    const bool enough_light = player_character.fine_detail_vision_mod() <= 4;

    const int factor = player_character.max_quality( qual_BUTCHER, PICKUP_RANGE );
    const std::string msgFactor = factor > INT_MIN
                                  ? string_format( _( "Your best tool has <color_cyan>%d butchering</color>." ), factor )
                                  :  _( "You have no butchering tool." );

    const int factorD = player_character.max_quality( qual_CUT_FINE, PICKUP_RANGE );
    const std::string msgFactorD = factorD > INT_MIN
                                   ? string_format( _( "Your best tool has <color_cyan>%d fine cutting</color>." ), factorD )
                                   :  _( "You have no fine cutting tool." );

    bool has_blood = false;
    bool has_skin = false;
    bool has_organs = false;
    std::string dissect_wp_hint; // dissection weakpoint proficiencies training hint

    if( index != -1 ) {
        const mtype *dead_mon = corpses[index]->get_mtype();
        if( dead_mon ) {
            for( const harvest_entry &entry : dead_mon->harvest.obj() ) {
                if( entry.type == harvest_drop_skin && !corpses[index]->has_flag( flag_SKINNED ) ) {
                    has_skin = true;
                }
                if( entry.type == harvest_drop_offal && !( corpses[index]->has_flag( flag_QUARTERED ) ||
                        corpses[index]->has_flag( flag_FIELD_DRESS ) ||
                        corpses[index]->has_flag( flag_FIELD_DRESS_FAILED ) ) ) {
                    has_organs = true;
                }
                if( entry.type == harvest_drop_blood && dead_mon->bleed_rate > 0 &&
                    !( corpses[index]->has_flag( flag_QUARTERED ) ||
                       corpses[index]->has_flag( flag_FIELD_DRESS ) ||
                       corpses[index]->has_flag( flag_FIELD_DRESS_FAILED ) || corpses[index]->has_flag( flag_BLED ) ) ) {
                    has_blood = true;
                }
            }
            if( !dead_mon->families.families.empty() ) {
                dissect_wp_hint += std::string( "\n\n" ) + _( "Dissecting may yield knowledge of:" );
                for( const weakpoint_family &wf : dead_mon->families.families ) {
                    std::string prof_status;
                    if( !player_character.has_prof_prereqs( wf.proficiency ) ) {
                        prof_status += colorize( string_format( " (%s)", _( "missing proficiencies" ) ), c_red );
                    } else if( player_character.has_proficiency( wf.proficiency ) ) {
                        prof_status += colorize( string_format( " (%s)", _( "already known" ) ), c_dark_gray );
                    }
                    dissect_wp_hint += string_format( "\n  %s%s", wf.proficiency->name(), prof_status );
                }
            }
        }
    }

    // Returns true if a cruder method is already in progress, to disallow finer butchering methods
    auto has_started_cruder_type = [&]( butcher_type bt ) {
        for( int other_bt = 0; other_bt < num_butcher_types; other_bt++ ) {
            if( has_started[other_bt] && cut_times[other_bt] < cut_times[static_cast<int>( bt )] ) {
                return true;
            }
        }
        return false;
    };
    auto is_enabled = [&]( butcher_type bt ) {
        if( bt == butcher_type::DISMEMBER ) {
            return true;
        } else if( !enough_light
                   || ( bt == butcher_type::FIELD_DRESS && !has_organs )
                   || ( bt == butcher_type::SKIN && !has_skin )
                   || ( bt == butcher_type::BLEED && !has_blood )
                   || has_started_cruder_type( bt ) ) {
            return false;
        }
        return true;
    };

    const std::string cannot_see = colorize( _( "can't see!" ), c_red );
    auto time_or_disabledreason = [&]( butcher_type bt ) {
        if( bt == butcher_type::DISMEMBER ) {
            return cut_time( bt );
        } else if( !enough_light ) {
            return cannot_see;
        } else if( bt == butcher_type::FIELD_DRESS && !has_organs ) {
            return colorize( _( "has no organs" ), c_red );
        } else if( bt == butcher_type::SKIN && !has_skin ) {
            return colorize( _( "has no skin" ), c_red );
        } else if( bt == butcher_type::BLEED && !has_blood ) {
            return colorize( _( "has no blood" ), c_red );
        } else if( has_started_cruder_type( bt ) ) {
            return colorize( _( "other type started" ), c_red );
        }
        return cut_time( bt );
    };

    uilist smenu;
    smenu.desc_enabled = true;
    smenu.title = _( "Choose type of butchery:" );

    smenu.addentry_col( static_cast<int>( butcher_type::QUICK ), is_enabled( butcher_type::QUICK ),
                        'B', _( "Quick butchery" )
                        + progress_str( butcher_type::QUICK ),
                        time_or_disabledreason( butcher_type::QUICK ),
                        wrap60( string_format( "%s  %s",
                                _( "This technique is used when you are in a hurry, "
                                   "but still want to harvest something from the corpse. "
                                   " Yields are lower as you don't try to be precise, "
                                   "but it's useful if you don't want to set up a workshop.  "
                                   "Prevents zombies from raising." ),
                                msgFactor ) ) );
    smenu.addentry_col( static_cast<int>( butcher_type::FULL ),
                        is_enabled( butcher_type::FULL ),
                        'b', _( "Full butchery" )
                        + progress_str( butcher_type::FULL ),
                        time_or_disabledreason( butcher_type::FULL ),
                        wrap60( string_format( "%s  %s",
                                _( "This technique is used to properly butcher a corpse, "
                                   "and requires a rope & a tree or a butchering rack, "
                                   "a flat surface (for ex. a table, a leather tarp, etc.) "
                                   "and good tools.  Yields are plentiful and varied, "
                                   "but it is time consuming." ),
                                msgFactor ) ) );
    smenu.addentry_col( static_cast<int>( butcher_type::FIELD_DRESS ),
                        is_enabled( butcher_type::FIELD_DRESS ),
                        'f', _( "Field dress corpse" )
                        + progress_str( butcher_type::FIELD_DRESS ),
                        time_or_disabledreason( butcher_type::FIELD_DRESS ),
                        wrap60( string_format( "%s  %s",
                                _( "Technique that involves removing internal organs and "
                                   "viscera to protect the corpse from rotting from inside.  "
                                   "Yields internal organs.  Carcass will be lighter and will "
                                   "stay fresh longer.  Can be combined with other methods for "
                                   "better effects." ),
                                msgFactor ) ) );
    smenu.addentry_col( static_cast<int>( butcher_type::SKIN ),
                        is_enabled( butcher_type::SKIN ),
                        's', _( "Skin corpse" )
                        + progress_str( butcher_type::SKIN ),
                        time_or_disabledreason( butcher_type::SKIN ),
                        wrap60( string_format( "%s  %s",
                                _( "Skinning a corpse is an involved and careful process that "
                                   "usually takes some time.  You need skill and an appropriately "
                                   "sharp and precise knife to do a good job.  Some corpses are "
                                   "too small to yield a full-sized hide and will instead produce "
                                   "scraps that can be used in other ways." ),
                                msgFactor ) ) );
    smenu.addentry_col( static_cast<int>( butcher_type::BLEED ),
                        is_enabled( butcher_type::BLEED ),
                        'l', _( "Bleed corpse" )
                        + progress_str( butcher_type::BLEED ),
                        time_or_disabledreason( butcher_type::BLEED ),
                        wrap60( string_format( "%s  %s",
                                _( "Bleeding involves severing the carotid arteries and jugular "
                                   "veins, or the blood vessels from which they arise.  "
                                   "You need skill and an appropriately sharp and precise knife "
                                   "to do a good job." ),
                                msgFactor ) ) );
    smenu.addentry_col( static_cast<int>( butcher_type::QUARTER ),
                        is_enabled( butcher_type::QUARTER ),
                        'k', _( "Quarter corpse" )
                        + progress_str( butcher_type::QUARTER ),
                        time_or_disabledreason( butcher_type::QUARTER ),
                        wrap60( string_format( "%s  %s",
                                _( "By quartering a previously field dressed corpse you will "
                                   "acquire four parts with reduced weight and volume.  It "
                                   "may help in transporting large game.  This action destroys "
                                   "skin, hide, pelt, etc., so don't use it if you want to "
                                   "harvest them later." ),
                                msgFactor ) ) );
    smenu.addentry_col( static_cast<int>( butcher_type::DISMEMBER ),
                        is_enabled( butcher_type::DISMEMBER ),
                        'm', _( "Dismember corpse" )
                        + progress_str( butcher_type::DISMEMBER ),
                        time_or_disabledreason( butcher_type::DISMEMBER ),
                        wrap60( string_format( "%s  %s",
                                _( "If you're aiming to just destroy a body outright and don't "
                                   "care about harvesting it, dismembering it will hack it apart "
                                   "in a very short amount of time but yields little to no usable flesh." ),
                                msgFactor ) ) );
    smenu.addentry_col( static_cast<int>( butcher_type::DISSECT ),
                        is_enabled( butcher_type::DISSECT ),
                        'd', _( "Dissect corpse" )
                        + progress_str( butcher_type::DISSECT ),
                        time_or_disabledreason( butcher_type::DISSECT ),
                        wrap60( string_format( "%s  %s%s",
                                _( "By careful dissection of the corpse, you will examine it for "
                                   "possible bionic implants, or discrete organs and harvest them "
                                   "if possible.  Requires scalpel-grade cutting tools, ruins "
                                   "corpse, and consumes a lot of time.  Your medical knowledge "
                                   "is most useful here." ),
                                msgFactorD, dissect_wp_hint ) ) );
    smenu.query();
    switch( smenu.ret ) {
        case static_cast<int>( butcher_type::QUICK ):
            return butcher_type::QUICK;
            break;
        case static_cast<int>( butcher_type::FULL ):
            return butcher_type::FULL;
            break;
        case static_cast<int>( butcher_type::FIELD_DRESS ):
            return butcher_type::FIELD_DRESS;
            break;
        case static_cast<int>( butcher_type::SKIN ):
            return butcher_type::SKIN;
            break;
        case static_cast<int>( butcher_type::BLEED ) :
            return butcher_type::BLEED;
            break;
        case static_cast<int>( butcher_type::QUARTER ):
            return butcher_type::QUARTER;
            break;
        case static_cast<int>( butcher_type::DISMEMBER ):
            return butcher_type::DISMEMBER;
            break;
        case static_cast<int>( butcher_type::DISSECT ):
            return butcher_type::DISSECT;
            break;
        default:
            return std::nullopt;
    }
}
