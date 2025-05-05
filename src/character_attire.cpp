#include "character_attire.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <iterator>
#include <memory>
#include <numeric>
#include <ostream>

#include "avatar_action.h"
#include "bodygraph.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "coordinates.h"
#include "creature.h"
#include "damage.h"
#include "debug.h"
#include "display.h"
#include "effect.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "fire.h"
#include "flag.h"
#include "flexbuffer_json.h"
#include "game_constants.h"
#include "inventory.h"
#include "item_contents.h"
#include "item_pocket.h"
#include "itype.h"
#include "json.h"
#include "line.h"
#include "make_static.h"
#include "map.h"
#include "melee.h"
#include "messages.h"
#include "mutation.h"
#include "output.h"
#include "pimpl.h"
#include "pocket_type.h"
#include "relic.h"
#include "rng.h"
#include "string_formatter.h"
#include "translation.h"
#include "translations.h"
#include "value_ptr.h"
#include "viewer.h"

static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_heating_bionic( "heating_bionic" );
static const efftype_id effect_incorporeal( "incorporeal" );
static const efftype_id effect_onfire( "onfire" );

static const flag_id json_flag_HIDDEN( "HIDDEN" );
static const flag_id json_flag_ONE_PER_LAYER( "ONE_PER_LAYER" );

static const material_id material_acidchitin( "acidchitin" );
static const material_id material_bone( "bone" );
static const material_id material_chitin( "chitin" );
static const material_id material_fur( "fur" );
static const material_id material_gutskin( "gutskin" );
static const material_id material_leather( "leather" );
static const material_id material_wool( "wool" );

static const sub_bodypart_str_id sub_body_part_foot_sole_l( "foot_sole_l" );
static const sub_bodypart_str_id sub_body_part_foot_sole_r( "foot_sole_r" );

static const trait_id trait_ANTENNAE( "ANTENNAE" );
static const trait_id trait_ANTLERS( "ANTLERS" );
static const trait_id trait_HORNS_POINTED( "HORNS_POINTED" );
static const trait_id trait_SQUEAMISH( "SQUEAMISH" );
static const trait_id trait_VEGAN( "VEGAN" );
static const trait_id trait_WOOLALLERGY( "WOOLALLERGY" );

nc_color item_penalties::color_for_stacking_badness() const
{
    switch( badness() ) {
        case 0:
            return c_light_gray;
        case 1:
            return c_yellow;
        case 2:
            return c_light_red;
    }
    debugmsg( "Unexpected badness %d", badness() );
    return c_light_gray;
}

units::mass get_selected_stack_weight( const item *i, const std::map<const item *, int> &without )
{
    auto stack = without.find( i );
    if( stack != without.end() ) {
        int selected = stack->second;
        item copy = *i;
        copy.charges = selected;
        return copy.weight();
    }

    return 0_gram;
}

ret_val<void> Character::can_wear( const item &it, bool with_equip_change ) const
{
    if( it.has_flag( flag_INTEGRATED ) ) {
        return ret_val<void>::make_success();
    }
    if( it.has_flag( flag_CANT_WEAR ) ) {
        return ret_val<void>::make_failure( _( "Can't be worn directly." ) );
    }
    if( has_effect( effect_incorporeal ) ) {
        return ret_val<void>::make_failure( _( "You can't wear anything while incorporeal." ) );
    }
    if( !it.is_armor() ) {
        return ret_val<void>::make_failure( _( "Putting on a %s would be tricky." ), it.tname() );
    }

    if( has_trait( trait_WOOLALLERGY ) && ( it.made_of( material_wool ) ||
                                            it.has_own_flag( flag_wooled ) ) ) {
        return ret_val<void>::make_failure( _( "Can't wear that, it's made of wool!" ) );
    }

    if( has_trait( trait_VEGAN ) && ( it.made_of( material_leather ) ||
                                      it.has_own_flag( flag_ANIMAL_PRODUCT ) ||
                                      it.made_of( material_fur ) ||
                                      it.made_of( material_wool ) ||
                                      it.made_of( material_chitin ) ||
                                      it.made_of( material_bone ) ||
                                      it.made_of( material_gutskin ) ||
                                      it.made_of( material_acidchitin ) ) ) {
        return ret_val<void>::make_failure( _( "Can't wear that, it's made from an animal!" ) );
    }

    if( it.is_filthy() && has_trait( trait_SQUEAMISH ) ) {
        return ret_val<void>::make_failure( _( "Can't wear that, it's filthy!" ) );
    }

    if( !it.has_flag( flag_OVERSIZE ) && !it.has_flag( flag_INTEGRATED ) &&
        !it.has_flag( flag_SEMITANGIBLE ) && !it.has_flag( flag_MORPHIC ) &&
        !it.has_flag( flag_UNRESTRICTED ) ) {
        for( const trait_id &mut : get_functioning_mutations() ) {
            const mutation_branch &branch = mut.obj();
            if( branch.conflicts_with_item( it ) ) {
                return ret_val<void>::make_failure( is_avatar() ?
                                                    _( "Your %s mutation prevents you from wearing your %s." ) :
                                                    _( "My %s mutation prevents me from wearing this %s." ), mutation_name( mut ),
                                                    it.type_name() );
            }
        }
        if( it.covers( body_part_head ) && !it.has_flag( flag_SEMITANGIBLE ) &&
            it.is_rigid() &&
            ( has_trait( trait_HORNS_POINTED ) || has_trait( trait_ANTENNAE ) ||
              has_trait( trait_ANTLERS ) ) ) {
            return ret_val<void>::make_failure( _( "Cannot wear a helmet over %s." ),
                                                ( has_trait( trait_HORNS_POINTED ) ? _( "horns" ) :
                                                  ( has_trait( trait_ANTENNAE ) ? _( "antennae" ) : _( "antlers" ) ) ) );
        }
    }

    if( it.has_flag( flag_SPLINT ) ) {
        bool need_splint = false;
        for( const bodypart_id &bp : get_all_body_parts() ) {
            if( !it.covers( bp ) ) {
                continue;
            }
            if( is_limb_broken( bp ) && !worn_with_flag( flag_SPLINT, bp ) ) {
                need_splint = true;
                break;
            }
        }
        if( !need_splint ) {
            return ret_val<void>::make_failure( is_avatar() ?
                                                _( "You don't have any broken limbs this could help." )
                                                : _( "%s doesn't have any broken limbs this could help." ), get_name() );
        }
    }

    if( it.has_flag( flag_TOURNIQUET ) ) {
        bool need_tourniquet = false;
        for( const bodypart_id &bp : get_all_body_parts() ) {
            if( !it.covers( bp ) ) {
                continue;
            }
            effect e = get_effect( effect_bleed, bp );
            if( !e.is_null() && e.get_intensity() > e.get_max_intensity() / 4 &&
                !worn_with_flag( flag_TOURNIQUET, bp ) ) {
                need_tourniquet = true;
                break;
            }
        }
        if( !need_tourniquet ) {
            std::string msg;
            if( is_avatar() ) {
                msg = _( "You don't need a tourniquet to stop the bleeding." );
            } else {
                msg = string_format( _( "%s doesn't need a tourniquet to stop the bleeding." ), get_name() );
            }
            return ret_val<void>::make_failure( msg );
        }
    }

    if( it.has_flag( flag_RESTRICT_HANDS ) && !has_min_manipulators() ) {
        return ret_val<void>::make_failure( ( is_avatar() ? _( "You don't have enough arms to wear that." )
                                              : string_format( _( "%s doesn't have enough arms to wear that." ), get_name() ) ) );
    }

    //Everything checked after here should be something that could be solved by changing equipment
    if( with_equip_change ) {
        return ret_val<void>::make_success();
    }

    {
        ret_val<void> power_armor_conflicts = worn.power_armor_conflicts( it );
        if( !power_armor_conflicts.success() ) {
            return power_armor_conflicts;
        }
    }

    // Check if we don't have both hands available before wearing a briefcase, shield, etc. Also occurs if we're already wearing one.
    if( it.has_flag( flag_RESTRICT_HANDS ) && ( worn_with_flag( flag_RESTRICT_HANDS ) ||
            weapon.is_two_handed( *this ) ) ) {
        return ret_val<void>::make_failure( ( is_avatar() ? _( "You don't have a hand free to wear that." )
                                              : string_format( _( "%s doesn't have a hand free to wear that." ), get_name() ) ) );
    }

    {
        ret_val<void> conflict = worn.only_one_conflicts( it );
        if( !conflict.success() ) {
            return conflict;
        }
    }

    // if the item is rigid make sure other rigid items aren't already equipped
    if( it.is_rigid() ) {
        ret_val<void> conflict = worn.check_rigid_conflicts( it );
        if( !conflict.success() ) {
            return conflict;
        }
    }

    if( amount_worn( it.typeId() ) >= it.max_worn() ) {
        return ret_val<void>::make_failure( _( "Can't wear %1$i or more %2$s at once." ),
                                            it.max_worn() + 1, it.tname( it.max_worn() + 1 ) );
    }

    return ret_val<void>::make_success();
}

std::optional<std::list<item>::iterator>
Character::wear( int pos, bool interactive )
{
    return wear( item_location( *this, &i_at( pos ) ), interactive );
}

std::optional<std::list<item>::iterator>
Character::wear( item_location item_wear, bool interactive )
{
    item &to_wear = *item_wear;

    // Need to account for case where we're trying to wear something that belongs to someone else
    if( !avatar_action::check_stealing( *this, to_wear ) ) {
        return std::nullopt;
    }

    if( is_worn( to_wear ) ) {
        if( interactive ) {
            add_msg_player_or_npc( m_info,
                                   _( "You are already wearing that." ),
                                   _( "<npcname> is already wearing that." )
                                 );
        }
        return std::nullopt;
    }
    if( to_wear.is_null() ) {
        if( interactive ) {
            add_msg_player_or_npc( m_info,
                                   _( "You don't have that item." ),
                                   _( "<npcname> doesn't have that item." ) );
        }
        return std::nullopt;
    }

    bool was_weapon;
    item to_wear_copy( to_wear );
    if( &to_wear == &weapon ) {
        weapon = item();
        was_weapon = true;
    } else if( has_item( to_wear ) ) {
        remove_item( to_wear );
        was_weapon = false;
    } else {
        // item is on the map if this point is reached.
        item_wear.remove_item();
        was_weapon = false;
    }

    worn.check_rigid_sidedness( to_wear_copy );
    worn.one_per_layer_sidedness( to_wear_copy );

    auto result = wear_item( to_wear_copy, interactive );
    if( !result ) {
        // set it to no longer be sided
        to_wear_copy.set_side( side::BOTH );
        if( was_weapon ) {
            weapon = to_wear_copy;
        } else {
            i_add( to_wear_copy );
        }
        return std::nullopt;
    }

    if( was_weapon ) {
        cata::event e = cata::event::make<event_type::character_wields_item>( getID(), weapon.typeId() );
        get_event_bus().send_with_talker( this, &item_wear, e );
    }

    return result;
}

std::optional<std::list<item>::iterator> outfit::wear_item( Character &guy, const item &to_wear,
        bool interactive, bool do_calc_encumbrance, bool do_sort_items, bool quiet )
{
    const map &here = get_map();

    const bool was_deaf = guy.is_deaf();
    const bool supertinymouse = guy.get_size() == creature_size::tiny;
    guy.last_item = to_wear.typeId();

    std::list<item>::iterator new_item_it = worn.end();

    if( do_sort_items ) {
        std::list<item>::iterator position = position_to_wear_new_item( to_wear );
        new_item_it = worn.insert( position, to_wear );
    } else {
        // this is used for debug and putting on clothing in the wrong order
        worn.push_back( to_wear );
    }

    item_location loc = new_item_it != worn.end() ?
                        item_location( guy, &*new_item_it ) : item_location();
    cata::event e = cata::event::make<event_type::character_wears_item>( guy.getID(), guy.last_item );
    get_event_bus().send_with_talker( &guy, &loc, e );

    if( interactive ) {
        if( !quiet ) {
            guy.add_msg_player_or_npc(
                _( "You put on your %s." ),
                _( "<npcname> puts on their %s." ),
                to_wear.tname() );
        }
        guy.mod_moves( -guy.item_wear_cost( to_wear ) );

        for( const bodypart_id &bp : guy.get_all_body_parts() ) {
            if( to_wear.covers( bp ) && guy.encumb( bp ) >= 40 && !quiet ) {
                guy.add_msg_if_player( m_warning,
                                       bp == body_part_eyes ?
                                       _( "Your %s are very encumbered!  %s" ) : _( "Your %s is very encumbered!  %s" ),
                                       body_part_name( bp ), encumb_text( bp ) );
            }
        }
        if( !was_deaf && guy.is_deaf() && !quiet ) {
            guy.add_msg_if_player( m_info, _( "You're deafened!" ) );
        }
        if( supertinymouse && !to_wear.has_flag( flag_UNDERSIZE ) && !quiet ) {
            guy.add_msg_if_player( m_warning,
                                   _( "This %s is too big to wear comfortably!  Maybe it could be refitted." ),
                                   to_wear.tname() );
        } else if( !supertinymouse && to_wear.has_flag( flag_UNDERSIZE ) && !quiet ) {
            guy.add_msg_if_player( m_warning,
                                   _( "This %s is too small to wear comfortably!  Maybe it could be refitted." ),
                                   to_wear.tname() );
        }
    } else if( guy.is_npc() && get_player_view().sees( here, guy ) && !quiet ) {
        guy.add_msg_if_npc( _( "<npcname> puts on their %s." ), to_wear.tname() );
    }

    // skip this for unsorted items in debug mode
    if( do_sort_items ) {
        new_item_it->on_wear( guy );

        guy.inv->update_invlet( *new_item_it );
        guy.inv->update_cache_with_item( *new_item_it );
    }

    if( do_calc_encumbrance ) {
        guy.recalc_sight_limits();
        guy.calc_encumbrance();
        guy.calc_discomfort();
    }

    if( to_wear.is_rigid() || to_wear.is_ablative() ) {
        recalc_ablative_blocking( &guy );
    }

    guy.recoil = MAX_RECOIL;

    return new_item_it;
}

void outfit::recalc_ablative_blocking( const Character *guy )
{
    std::set<sub_bodypart_id> rigid_locations;

    // get all rigid locations
    // ablative pocketed armor shouldn't block adding ablative pieces
    for( const item &w : worn ) {
        if( w.is_rigid() && !w.is_ablative() ) {
            for( const sub_bodypart_id &sbp : w.get_covered_sub_body_parts() ) {
                if( w.is_bp_rigid( sbp ) ) {
                    rigid_locations.emplace( sbp );
                }
            }
        }
    }

    bool should_warn = false;

    // pass that info to all the ablative pockets
    for( item &w : worn ) {
        if( w.is_ablative() ) {
            should_warn = true;
            for( item_pocket *p : w.get_all_ablative_pockets() ) {
                p->set_no_rigid( rigid_locations );
            }
        }
    }

    if( should_warn && !rigid_locations.empty() ) {
        guy->add_msg_if_player( m_warning,
                                _( "You are wearing rigid armor with armor that has pockets for armor.  Until hard armor is removed inserting plates on those locations will be disabled." ) );
    }
}

std::optional<std::list<item>::iterator> Character::wear_item( const item &to_wear,
        bool interactive, bool do_calc_encumbrance )
{
    invalidate_inventory_validity_cache();
    invalidate_leak_level_cache();
    const auto ret = can_wear( to_wear );
    if( !ret.success() ) {
        if( interactive ) {
            add_msg_if_player( m_info, "%s", ret.c_str() );
        }
        return std::nullopt;
    }

    return worn.wear_item( *this, to_wear, interactive, do_calc_encumbrance );
}

int Character::amount_worn( const itype_id &id ) const
{
    return worn.amount_worn( id );
}

int Character::item_wear_cost( const item &it ) const
{
    double mv = item_handling_cost( it );

    for( layer_level layer : it.get_layer() ) {
        switch( layer ) {
            case layer_level::SKINTIGHT:
                mv *= 1.5;
                break;

            case layer_level::NORMAL:
                break;

            case layer_level::WAIST:
            case layer_level::OUTER:
                mv /= 1.5;
                break;

            case layer_level::BELTED:
                mv /= 2.0;
                break;

            case layer_level::PERSONAL:
            case layer_level::AURA:
            default:
                break;
        }
    }

    mv *= std::max( it.get_avg_encumber( *this ) / 10.0, 1.0 );

    return mv;
}

ret_val<void> Character::can_takeoff( const item &it, const std::list<item> *res )
{
    if( !worn.is_worn( it ) ) {
        return ret_val<void>::make_failure( !is_npc() ? _( "You are not wearing that item." ) :
                                            _( "<npcname> is not wearing that item." ) );
    }

    if( it.has_flag( flag_INTEGRATED ) ) {
        return ret_val<void>::make_failure( !is_npc() ?
                                            _( "You can't remove a part of your body." ) :
                                            _( "<npcname> can't remove a part of their body." ) );
    }
    if( res == nullptr && !get_dependent_worn_items( it ).empty() ) {
        return ret_val<void>::make_failure( !is_npc() ?
                                            _( "You can't take off power armor while wearing other power armor components." ) :
                                            _( "<npcname> can't take off power armor while wearing other power armor components." ) );
    }
    if( it.has_flag( flag_NO_TAKEOFF ) ) {
        return ret_val<void>::make_failure( !is_npc() ?
                                            _( "You can't take that item off." ) :
                                            _( "<npcname> can't take that item off." ) );
    }
    return ret_val<void>::make_success();
}

bool Character::takeoff( item_location loc, std::list<item> *res )
{
    const std::string name = loc->tname();
    const bool success = worn.takeoff( loc, res, *this );

    if( success ) {
        add_msg_player_or_npc( _( "You take off your %s." ),
                               _( "<npcname> takes off their %s." ),
                               name );

        recalc_sight_limits();
        calc_encumbrance();
        worn.recalc_ablative_blocking( this );
        calc_discomfort();
        recoil = MAX_RECOIL;
        return true;
    } else {
        return false;
    }
}

bool Character::takeoff( int pos )
{
    item_location loc = item_location( *this, &i_at( pos ) );
    return takeoff( loc );
}

bool Character::is_wearing( const itype_id &it ) const
{
    return worn.is_worn( it );
}

bool Character::is_wearing_on_bp( const itype_id &it, const bodypart_id &bp ) const
{
    return worn.is_wearing_on_bp( it, bp );
}

bool Character::worn_with_flag( const flag_id &f, const bodypart_id &bp ) const
{
    return worn.worn_with_flag( f, bp );
}

bool Character::worn_with_flag( const flag_id &f ) const
{
    return worn.worn_with_flag( f );
}

item Character::item_worn_with_flag( const flag_id &f, const bodypart_id &bp ) const
{
    return worn.item_worn_with_flag( f, bp );
}

item Character::item_worn_with_flag( const flag_id &f ) const
{
    return worn.item_worn_with_flag( f );
}

item *Character::item_worn_with_id( const itype_id &i )
{
    return worn.item_worn_with_id( i );
}

bool Character::wearing_something_on( const bodypart_id &bp ) const
{
    return worn.wearing_something_on( bp );
}

bool Character::wearing_fitting_on( const bodypart_id &bp ) const
{
    return worn.wearing_fitting_on( bp );
}

bool Character::is_barefoot() const
{
    return worn.is_barefoot();
}

std::optional<const item *> outfit::item_worn_with_inv_let( const char invlet ) const
{
    for( const item &i : worn ) {
        if( i.invlet == invlet ) {
            return &i;
        }
    }
    return std::nullopt;
}

side outfit::is_wearing_shoes( const bodypart_id &bp ) const
{
    bool any_left_foot_is_covered = false;
    bool any_right_foot_is_covered = false;
    const auto exempt = []( const item & worn_item ) {
        return worn_item.has_flag( flag_BELTED ) ||
               worn_item.has_flag( flag_PERSONAL ) ||
               worn_item.has_flag( flag_AURA ) ||
               worn_item.has_flag( flag_SEMITANGIBLE ) ||
               worn_item.has_flag( flag_SKINTIGHT );
    };
    for( const item &worn_item : worn ) {
        // ... wearing...
        if( !worn_item.covers( bp ) ) {
            continue;
        }
        // ... a shoe?
        if( exempt( worn_item ) ) {
            continue;
        }
        any_left_foot_is_covered = bp->part_side == side::LEFT ||
                                   bp->part_side == side::BOTH ||
                                   any_left_foot_is_covered;
        any_right_foot_is_covered = bp->part_side == side::RIGHT ||
                                    bp->part_side == side::BOTH ||
                                    any_right_foot_is_covered;
    }

    if( any_left_foot_is_covered && any_right_foot_is_covered ) {
        return side::BOTH;
    } else if( any_left_foot_is_covered ) {
        return side::LEFT;
    } else if( any_right_foot_is_covered ) {
        return side::RIGHT;
    } else {
        return side::num_sides;
    }
}

bool Character::is_wearing_shoes( const side &check_side ) const
{
    side side_covered = side::num_sides;
    bool any_left_foot_is_covered = false;
    bool any_right_foot_is_covered = false;

    for( const bodypart_id &part : get_all_body_parts() ) {
        // Is any right|left foot...
        if( !part->has_type( body_part_type::type::foot ) ) {
            continue;
        }
        side_covered = worn.is_wearing_shoes( part );

        // early return if we've found a match
        switch( side_covered ) {
            case side::RIGHT:
                if( check_side == side::RIGHT ) {
                    return true;
                }
                any_right_foot_is_covered = true;
                break;
            case side::LEFT:
                if( check_side == side::LEFT ) {
                    return true;
                }
                any_left_foot_is_covered = true;
                break;
            case side::BOTH:
                // if we returned both
                return true;
                break;
            case side::num_sides:
                break;
        }

        // check if we've found both sides and are looking for both sides
        if( check_side == side::BOTH && any_right_foot_is_covered && any_left_foot_is_covered ) {
            return true;
        }
    }

    // fall through return if we haven't found a match
    return false;
}

bool Character::is_worn_item_visible( std::list<item>::const_iterator worn_item ) const
{
    const body_part_set worn_item_body_parts = worn_item->get_covered_body_parts();
    return worn.is_worn_item_visible( worn_item, worn_item_body_parts );
}

std::list<item_location> Character::get_visible_worn_items() const
{
    return const_cast<outfit &>( worn ).get_visible_worn_items( *this );
}

double Character::armwear_factor() const
{
    double ret = 0;
    if( wearing_something_on( body_part_arm_l ) ) {
        ret += .5;
    }
    if( wearing_something_on( body_part_arm_r ) ) {
        ret += .5;
    }
    return ret;
}

int Character::shoe_type_count( const itype_id &it ) const
{
    int ret = 0;
    if( is_wearing_on_bp( it, body_part_foot_l ) ) {
        ret++;
    }
    if( is_wearing_on_bp( it, body_part_foot_r ) ) {
        ret++;
    }
    return ret;
}

bool outfit::one_per_layer_change_side( item &it, const Character &guy ) const
{
    // test with a swapped side
    item it_copy( it );
    it_copy.swap_side();

    const bool item_one_per_layer = it_copy.has_flag( json_flag_ONE_PER_LAYER );
    for( const item &worn_item : worn ) {
        if( item_one_per_layer && worn_item.has_flag( json_flag_ONE_PER_LAYER ) ) {
            const std::optional<side> sidedness_conflict = it_copy.covers_overlaps( worn_item );
            if( sidedness_conflict ) {
                const std::string player_msg = string_format(
                                                   _( "Your %s conflicts with %s, so you cannot swap its side." ),
                                                   it_copy.tname(), worn_item.tname() );
                const std::string npc_msg = string_format(
                                                _( "<npcname>'s %s conflicts with %s so they cannot swap its side." ),
                                                it_copy.tname(), worn_item.tname() );
                guy.add_msg_player_or_npc( m_info, player_msg, npc_msg );
                return false;
            }
        }
    }
    return true;
}

bool outfit::check_rigid_change_side( item &it, const Character &guy ) const
{
    if( !check_rigid_conflicts( it,
                                it.get_side() == side::LEFT ? side::RIGHT : side::LEFT ).success() ) {
        const std::string player_msg = string_format(
                                           _( "Your %s conflicts with hard armor on your other side so you can't swap it." ),
                                           it.tname() );
        const std::string npc_msg = string_format(
                                        _( "<npcname>'s %s conflicts with hard armor on your other side so they can't swap it." ),
                                        it.tname() );
        guy.add_msg_player_or_npc( m_info, player_msg, npc_msg );
        return false;
    }
    return true;
}

bool Character::change_side( item &it, bool interactive )
{
    if( !it.is_sided() ) {
        if( interactive ) {
            add_msg_player_or_npc( m_info,
                                   _( "You cannot swap the side on which your %s is worn." ),
                                   _( "<npcname> cannot swap the side on which their %s is worn." ),
                                   it.tname() );
        }
        return false;
    }

    if( !worn.one_per_layer_change_side( it, *this ) ) {
        return false;
    }

    if( !worn.check_rigid_change_side( it, *this ) ) {
        return false;
    }

    //if made it here then swap the item
    it.swap_side();

    if( interactive ) {
        add_msg_player_or_npc( m_info, _( "You swap the side on which your %s is worn." ),
                               _( "<npcname> swaps the side on which their %s is worn." ),
                               it.tname() );
    }

    mod_moves( -250 );
    calc_encumbrance();
    calc_discomfort();

    return true;
}

bool Character::change_side( item_location &loc, bool interactive )
{
    if( !loc || !is_worn( *loc ) ) {
        if( interactive ) {
            add_msg_player_or_npc( m_info,
                                   _( "You are not wearing that item." ),
                                   _( "<npcname> isn't wearing that item." ) );
        }
        return false;
    }

    return change_side( *loc, interactive );
}

bool Character::is_wearing_power_armor( bool *hasHelmet ) const
{
    return worn.is_wearing_power_armor( hasHelmet );
}

bool Character::is_wearing_active_power_armor() const
{
    return worn.is_wearing_active_power_armor();
}

bool Character::is_wearing_active_optcloak() const
{
    return worn.is_wearing_active_optcloak();
}

static void layer_item( std::map<bodypart_id, encumbrance_data> &vals, const item &it,
                        std::map<sub_bodypart_id, layer_level> &highest_layer_so_far, const Character &c )
{
    body_part_set covered_parts = it.get_covered_body_parts();
    for( const bodypart_id &bp : c.get_all_body_parts() ) {
        if( !covered_parts.test( bp.id() ) ) {
            continue;
        }

        const std::vector<layer_level> item_layers = it.get_layer( bp );
        int encumber_val = it.get_encumber( c, bp.id() );
        int layering_encumbrance = clamp( encumber_val, 2, 10 );

        /*
         * Setting layering_encumbrance to 0 at this point makes the item cease to exist
         * for the purposes of the layer penalty system. (normally an item has a minimum
         * layering_encumbrance of 2 )
         * Personal layer items and semitangible items do not conflict.
         */
        if( it.has_flag( flag_SEMITANGIBLE ) ) {
            encumber_val = 0;
            layering_encumbrance = 0;
        }
        if( it.has_flag( flag_PERSONAL ) ) {
            layering_encumbrance = 0;
        }
        for( layer_level item_layer : item_layers ) {
            // do the sublayers of this armor conflict
            bool conflicts = false;

            // check if we've already added conflict for the layer and body part since each sbp is check individually
            std::map<layer_level, bool> bpcovered;

            // add the sublocations to the overall body part layer and update if we are conflicting
            for( sub_bodypart_id &sbp : it.get_covered_sub_body_parts() ) {
                if( std::count_if( bp->sub_parts.begin(),
                bp->sub_parts.end(), [sbp]( const sub_bodypart_str_id & bpid ) {
                return sbp.id() == bpid;
                } ) == 0 ) {
                    // the sub part isn't part of the bodypart we are checking
                    continue;
                }
                // bit hacky but needed since we are doing one layer at a time
                std::vector<layer_level> ll;
                ll.push_back( item_layer );
                if( !it.has_layer( ll, sbp ) ) {
                    // skip this layer and sbp if it doesn't cover it
                    continue;
                }

                if( item_layer >= highest_layer_so_far[sbp] ) {
                    conflicts = vals[bp].add_sub_location( item_layer, sbp );
                } else {
                    // if it is on a lower layer it conflicts for sure
                    conflicts = true;
                }

                highest_layer_so_far[sbp] = std::max( highest_layer_so_far[sbp], item_layer );

                // Apply layering penalty to this layer, as well as any layer worn
                // within it that would normally be worn outside of it.
                for( layer_level penalty_layer = item_layer;
                     penalty_layer <= highest_layer_so_far[sbp]; ++penalty_layer ) {

                    // make sure we haven't already found a subpart that covers and would cause penalty
                    if( !bpcovered[penalty_layer] ) {
                        vals[bp].layer( penalty_layer, layering_encumbrance, conflicts );
                        bpcovered[penalty_layer] = true;
                    }
                }
            }
        }
        vals[bp].armor_encumbrance += encumber_val;
    }
}

/*
 * Encumbrance logic:
 * Some clothing is intrinsically encumbering, such as heavy jackets, backpacks, body armor, etc.
 * These simply add their encumbrance value to each body part they cover.
 * In addition, each article of clothing after the first in a layer imposes an additional penalty.
 * e.g. one shirt will not encumber you, but two is tight and starts to restrict movement.
 * Clothes on separate layers don't interact, so if you wear e.g. a light jacket over a shirt,
 * they're intended to be worn that way, and don't impose a penalty.
 * The default is to assume that clothes do not fit, clothes that are "fitted" either
 * reduce the encumbrance penalty by ten, or if that is already 0, they reduce the layering effect.
 *
 * Use cases:
 * What would typically be considered normal "street clothes" should not be considered encumbering.
 * T-shirt, shirt, jacket on torso/arms, underwear and pants on legs, socks and shoes on feet.
 * This is currently handled by each of these articles of clothing
 * being on a different layer and/or body part, therefore accumulating no encumbrance.
 */
void outfit::item_encumb( std::map<bodypart_id, encumbrance_data> &vals,
                          const item &new_item, const Character &guy ) const
{
    // reset all layer data
    vals = std::map<bodypart_id, encumbrance_data>();

    // Figure out where new_item would be worn
    std::list<item>::const_iterator new_item_position = worn.end();
    if( !new_item.is_null() ) {
        // const_cast required to work around g++-4.8 library bug
        // see the commit that added this comment to understand why
        new_item_position =
            const_cast<outfit *>( this )->position_to_wear_new_item( new_item );
    }

    // Track highest layer observed so far so we can penalize out-of-order
    // items
    std::map<sub_bodypart_id, layer_level> highest_layer_so_far;

    for( auto w_it = worn.begin(); w_it != worn.end(); ++w_it ) {
        if( w_it == new_item_position ) {
            layer_item( vals, new_item, highest_layer_so_far, guy );
        }
        layer_item( vals, *w_it, highest_layer_so_far, guy );
    }

    if( worn.end() == new_item_position && !new_item.is_null() ) {
        layer_item( vals, new_item, highest_layer_so_far, guy );
    }

    // make sure values are sane
    for( const bodypart_id &bp : guy.get_all_body_parts() ) {
        encumbrance_data &elem = vals[bp];

        for( const layer_details &cur_layer : elem.layer_penalty_details ) {
            // only apply the layers penalty to the limb if it is conflicting
            if( cur_layer.is_conflicting ) {
                elem.layer_penalty += cur_layer.total;
            }
        }

        elem.armor_encumbrance = std::max( 0, elem.armor_encumbrance );

        // Add armor and layering penalties for the final values
        elem.encumbrance += elem.armor_encumbrance + elem.layer_penalty;
    }
}

size_t outfit::size() const
{
    return worn.size();
}

bool outfit::is_worn( const item &clothing ) const
{
    for( const item &elem : worn ) {
        if( &clothing == &elem ) {
            return true;
        }
    }
    return false;
}

bool outfit::is_worn( const itype_id &clothing ) const
{
    for( const item &i : worn ) {
        if( i.typeId() == clothing ) {
            return true;
        }
    }
    return false;
}
bool outfit::is_worn_module( const item &thing ) const

{
    return thing.has_flag( flag_CANT_WEAR ) &&
    std::any_of( worn.cbegin(), worn.cend(), [&thing]( item const & elem ) {
        return elem.has_flag( flag_MODULE_HOLDER ) &&
               elem.contained_where( thing ) != nullptr;
    } );
}

bool outfit::is_wearing_on_bp( const itype_id &clothing, const bodypart_id &bp ) const
{
    for( const item &i : worn ) {
        if( i.typeId() == clothing && i.covers( bp ) ) {
            return true;
        }
    }
    return false;
}

bool outfit::worn_with_flag( const flag_id &f, const bodypart_id &bp ) const
{
    return std::any_of( worn.begin(), worn.end(), [&f, bp]( const item & it ) {
        return it.has_flag( f ) && ( bp == bodypart_str_id::NULL_ID() || it.covers( bp ) );
    } );
}

bool outfit::worn_with_flag( const flag_id &f ) const
{
    return std::any_of( worn.begin(), worn.end(), [&f]( const item & it ) {
        return it.has_flag( f );
    } );
}

bool outfit::is_worn_item_visible( std::list<item>::const_iterator worn_item,
                                   const body_part_set &worn_item_body_parts ) const
{
    return std::any_of( worn_item_body_parts.begin(), worn_item_body_parts.end(),
    [this, &worn_item]( const bodypart_str_id & bp ) {
        // no need to check items that are worn under worn_item in the armor sort order
        for( auto i = std::next( worn_item ), end = worn.end(); i != end; ++i ) {
            if( i->covers( bp ) && i->has_layer( { layer_level::BELTED, layer_level::WAIST } ) &&
                i->get_coverage( bp ) >= worn_item->get_coverage( bp ) ) {
                return false;
            }
        }
        return true;
    } );
}

item outfit::item_worn_with_flag( const flag_id &f, const bodypart_id &bp ) const
{
    item it_with_flag;
    for( const item &it : worn ) {
        if( it.has_flag( f ) && ( bp == bodypart_str_id::NULL_ID() || it.covers( bp ) ) ) {
            it_with_flag = it;
            break;
        }
    }
    return it_with_flag;
}

item outfit::item_worn_with_flag( const flag_id &f ) const
{
    item it_with_flag;
    for( const item &it : worn ) {
        if( it.has_flag( f ) ) {
            it_with_flag = it;
            break;
        }
    }
    return it_with_flag;
}

item *outfit::item_worn_with_id( const itype_id &i )
{
    item *it_with_id = nullptr;
    for( item &it : worn ) {
        if( it.typeId() == i ) {
            it_with_id = &it;
            break;
        }
    }
    return it_with_id;
}

std::list<item_location> outfit::get_visible_worn_items( const Character &guy )
{
    std::list<item_location> result;
    for( auto i = worn.begin(), end = worn.end(); i != end; ++i ) {
        if( guy.is_worn_item_visible( i ) ) {
            item_location loc_here( const_cast<Character &>( guy ), &*i );
            result.emplace_back( loc_here );
        }
    }
    return result;
}

bool outfit::wearing_something_on( const bodypart_id &bp ) const
{
    for( const item &i : worn ) {
        if( i.covers( bp ) && !i.has_flag( flag_INTEGRATED ) ) {
            return true;
        }
    }
    return false;
}

bool outfit::wearing_fitting_on( const bodypart_id &bp ) const
{
    for( const item &i : worn ) {
        if( i.covers( bp ) && !i.has_flag( flag_INTEGRATED ) && !i.has_flag( flag_OVERSIZE ) &&
            !i.has_flag( flag_UNRESTRICTED ) ) {
            return true;
        }
    }
    return false;
}

bool outfit::is_barefoot() const
{
    for( const item &i : worn ) {
        if( ( i.covers( sub_body_part_foot_sole_l ) && !i.has_flag( flag_INTEGRATED ) ) ||
            ( i.covers( sub_body_part_foot_sole_r ) && !i.has_flag( flag_INTEGRATED ) ) ) {
            return false;
        }
    }
    return true;
}

int outfit::swim_modifier( const int swim_skill ) const
{
    int ret = 0;
    if( swim_skill < 10 ) {
        for( const item &i : worn ) {
            ret += i.volume() * ( 10 - swim_skill ) / 125_ml;
        }
    }
    return ret;
}

bool outfit::check_item_encumbrance_flag( bool update_required )
{
    for( item &i : worn ) {
        if( !update_required && i.encumbrance_update_ ) {
            update_required = true;
        }
        i.encumbrance_update_ = false;
    }
    return update_required;
}

static bool check_natural_attack_restricted_on_worn( const item &i )
{
    return !i.has_flag( flag_ALLOWS_NATURAL_ATTACKS ) &&
           !i.has_flag( flag_SEMITANGIBLE ) &&
           !i.has_flag( flag_PERSONAL ) && !i.has_flag( flag_AURA );
}

bool outfit::natural_attack_restricted_on( const bodypart_id &bp ) const
{
    for( const item &i : worn ) {
        if( i.covers( bp ) && check_natural_attack_restricted_on_worn( i ) ) {
            return true;
        }
    }
    return false;
}

bool outfit::natural_attack_restricted_on( const sub_bodypart_id &bp ) const
{
    for( const item &i : worn ) {
        if( i.covers( bp ) && check_natural_attack_restricted_on_worn( i ) ) {
            return true;
        }
    }
    return false;
}

std::list<item> outfit::remove_worn_items_with( const std::function<bool( item & )> &filter,
        Character &guy )
{
    std::list<item> result;
    for( auto iter = worn.begin(); iter != worn.end(); ) {
        if( filter( *iter ) ) {
            if( iter->can_unload() ) {
                iter->spill_contents( guy );
            }
            iter->on_takeoff( guy );
            result.splice( result.begin(), worn, iter++ );
        } else {
            ++iter;
        }
    }
    return result;
}


units::mass outfit::weight() const
{
    return std::accumulate( worn.begin(), worn.end(), 0_gram,
    []( units::mass sum, const item & itm ) {
        return sum + itm.weight();
    } );
}

units::mass outfit::weight_carried_with_tweaks( const std::map<const item *, int> &without ) const
{
    units::mass ret = 0_gram;
    for( const item &i : worn ) {
        if( without.empty() ) {
            ret += i.weight();
        } else if( !without.count( &i ) ) {
            for( const item *j : i.all_items_ptr( pocket_type::CONTAINER ) ) {
                if( j->count_by_charges() ) {
                    ret -= get_selected_stack_weight( j, without );
                } else if( without.count( j ) ) {
                    ret -= j->weight();
                }
            }
            ret += i.weight();
        }
    }
    return ret;
}

units::volume outfit::contents_volume_with_tweaks( const std::map<const item *, int> &without )
const
{
    units::volume ret = 0_ml;
    for( const item &i : worn ) {
        if( !without.count( &i ) ) {
            ret += i.get_contents_volume_with_tweaks( without );
        }
    }
    return ret;
}

ret_val<void> outfit::power_armor_conflicts( const item &clothing ) const
{
    if( clothing.is_power_armor() ) {
        for( const item &elem : worn ) {
            // Allow power armor with compatible parts and integrated (Subdermal CBM and mutant skin armor)
            if( elem.get_covered_body_parts().make_intersection( clothing.get_covered_body_parts() ).any() &&
                !elem.has_flag( flag_POWERARMOR_COMPATIBLE ) && !elem.has_flag( flag_INTEGRATED ) &&
                !elem.has_flag( flag_AURA ) ) {
                return ret_val<void>::make_failure( _( "Can't wear power armor over other gear!" ) );
            }
        }
        if( !clothing.covers( body_part_torso ) ) {
            bool power_armor = false;
            for( const item &elem : worn ) {
                if( elem.is_power_armor() ) {
                    power_armor = true;
                    break;
                }
            }
            if( !power_armor ) {
                return ret_val<void>::make_failure(
                           _( "You can only wear power armor components with power armor!" ) );
            }
        }

        for( const item &i : worn ) {
            if( i.is_power_armor() && i.typeId() == clothing.typeId() ) {
                return ret_val<void>::make_failure( _( "Can't wear more than one %s!" ), clothing.tname() );
            }
        }
    } else {
        // You can only wear headgear or non-covering items with power armor, except other power armor components.
        // You can't wear headgear if power armor helmet is already sitting on your head.
        bool has_helmet = false;
        if( !clothing.get_covered_body_parts().none() && !clothing.has_flag( flag_POWERARMOR_COMPATIBLE ) &&
            !clothing.has_flag( flag_AURA ) &&
            ( is_wearing_power_armor( &has_helmet ) &&
              ( has_helmet || !( clothing.covers( body_part_head ) || clothing.covers( body_part_mouth ) ||
                                 clothing.covers( body_part_eyes ) ) ) ) ) {
            return ret_val<void>::make_failure( _( "Can't wear %s with power armor!" ), clothing.tname() );
        }
    }
    return ret_val<void>::make_success();
}

bool outfit::is_wearing_power_armor( bool *has_helmet ) const
{
    bool result = false;
    for( const item &elem : worn ) {
        if( !elem.is_power_armor() ) {
            continue;
        }
        if( has_helmet == nullptr ) {
            // found power armor, helmet not requested, cancel loop
            return true;
        }
        // found power armor, continue search for helmet
        result = true;
        if( elem.covers( body_part_head ) ) {
            *has_helmet = true;
            return true;
        }
    }
    return result;
}

bool outfit::is_wearing_active_power_armor() const
{
    for( const item &w : worn ) {
        if( w.is_power_armor() && w.active ) {
            return true;
        }
    }
    return false;
}

bool outfit::is_wearing_active_optcloak() const
{
    for( const item &w : worn ) {
        if( w.active && w.has_flag( flag_ACTIVE_CLOAKING ) ) {
            return true;
        }
    }
    return false;
}

static ret_val<void> test_only_one_conflicts( const item &clothing, const item &i )
{
    const bool this_restricts_only_one = clothing.has_flag( json_flag_ONE_PER_LAYER );

    std::map<side, bool> sidedness;
    sidedness[side::BOTH] = false;
    sidedness[side::LEFT] = false;
    sidedness[side::RIGHT] = false;

    const auto sidedness_conflicts = [&sidedness]( side s ) -> bool {
        const bool ret = sidedness[s];
        sidedness[s] = true;
        if( sidedness[side::LEFT] && sidedness[side::RIGHT] )
        {
            sidedness[side::BOTH] = true;
            return true;
        }
        return ret;
    };

    if( i.max_worn() == 1 && i.typeId() == clothing.typeId() ) {
        return ret_val<void>::make_failure( _( "Can't wear more than one %s!" ), clothing.tname() );
    }

    if( this_restricts_only_one || i.has_flag( json_flag_ONE_PER_LAYER ) ) {
        std::optional<side> overlaps = clothing.covers_overlaps( i );
        if( overlaps && sidedness_conflicts( *overlaps ) ) {
            return ret_val<void>::make_failure( _( "%1$s conflicts with %2$s!" ), clothing.tname(), i.tname() );
        }
    }

    return ret_val<void>::make_success();
}

ret_val<void> outfit::only_one_conflicts( const item &clothing ) const
{
    for( const item &i : worn ) {
        ret_val<void> result = test_only_one_conflicts( clothing, i );
        if( !result.success() ) {
            return result;
        }

        if( i.is_ablative() ) {
            // if item has ablative armor we should check those too.
            for( const item *ablative_armor : i.all_ablative_armor() ) {
                result = test_only_one_conflicts( clothing, *ablative_armor );
                if( !result.success() ) {
                    return result;
                }
            }
        }
    }
    return ret_val<void>::make_success();
}

static ret_val<void> rigid_test( const item &clothing, const item &i,
                                 const std::unordered_set<sub_bodypart_id> &to_test )
{
    // check each sublimb individually
    for( const sub_bodypart_id &sbp : to_test ) {
        // skip if the item doesn't currently cover the bp
        if( !i.covers( sbp ) ) {
            continue;
        }

        // skip if either item cares only about its layer and they don't match up
        if( ( i.is_bp_rigid_selective( sbp ) || clothing.is_bp_rigid_selective( sbp ) ) &&
            !i.has_layer( clothing.get_layer( sbp ), sbp ) ) {
            continue;
        }

        // allow wearing splints on integrated armor such as protective bark
        if( i.has_flag( flag_INTEGRATED ) && clothing.has_flag( flag_SPLINT ) ) {
            continue;
        }

        if( i.is_bp_rigid( sbp ) ) {
            return ret_val<void>::make_failure( _( "Can't wear more than one rigid item on %s!" ), sbp->name );
        }
    }

    return ret_val<void>::make_success();
}

ret_val<void> outfit::check_rigid_conflicts( const item &clothing, side s ) const
{

    std::unordered_set<sub_bodypart_id> to_test;

    // if not overridden get the actual side of the item
    if( s == side::num_sides ) {
        s = clothing.get_side();
    }

    // if the clothing is sided and not on the side we want to test
    // create a copy that is swapped in side to test on
    if( clothing.is_sided() && clothing.get_side() != s ) {
        item swapped_item( clothing );
        swapped_item.set_side( s );
        // figure out which sublimbs need to be tested
        for( const sub_bodypart_id &sbp : swapped_item.get_covered_sub_body_parts() ) {
            if( swapped_item.is_bp_rigid( sbp ) ) {
                to_test.emplace( sbp );
            }
        }
    } else {
        // figure out which sublimbs need to be tested
        for( const sub_bodypart_id &sbp : clothing.get_covered_sub_body_parts() ) {
            if( clothing.is_bp_rigid( sbp ) ) {
                to_test.emplace( sbp );
            }
        }
    }

    // go through all worn and see if already wearing something rigid
    for( const item &i : worn ) {
        ret_val<void> result = rigid_test( clothing, i, to_test );
        if( !result.success() ) {
            return result;
        }
    }

    return ret_val<void>::make_success();
}

ret_val<void> outfit::check_rigid_conflicts( const item &clothing ) const
{
    if( !clothing.is_sided() ) {
        return check_rigid_conflicts( clothing, side::BOTH );
    }

    ret_val<void> ls = check_rigid_conflicts( clothing, side::LEFT );
    ret_val<void> rs = check_rigid_conflicts( clothing, side::RIGHT );

    if( !ls.success() && !rs.success() ) {
        return ls;
    }

    return ret_val<void>::make_success();
}

void outfit::one_per_layer_sidedness( item &clothing ) const
{
    const bool item_one_per_layer = clothing.has_flag( json_flag_ONE_PER_LAYER );
    for( const item &worn_item : worn ) {
        const std::optional<side> sidedness_conflict = clothing.covers_overlaps( worn_item );
        if( sidedness_conflict && ( item_one_per_layer ||
                                    worn_item.has_flag( json_flag_ONE_PER_LAYER ) ) ) {
            // we can assume both isn't an option because it'll be caught in can_wear
            if( *sidedness_conflict == side::LEFT ) {
                clothing.set_side( side::RIGHT );
            } else {
                clothing.set_side( side::LEFT );
            }
        }
    }
}

void outfit::check_rigid_sidedness( item &clothing ) const
{
    if( !clothing.is_sided() ) {
        //nothing to do
        return;
    }
    bool ls = check_rigid_conflicts( clothing, side::LEFT ).success();
    bool rs = check_rigid_conflicts( clothing, side::RIGHT ).success();
    if( ls && rs ) {
        clothing.set_side( side::BOTH );
    } else if( ls ) {
        clothing.set_side( side::LEFT );
    } else {
        // this is a fallback option if it must be something
        clothing.set_side( side::RIGHT );
    }
}

int outfit::amount_worn( const itype_id &clothing ) const
{
    int amount = 0;
    for( const item &elem : worn ) {
        if( elem.typeId() == clothing ) {
            ++amount;
        }
    }
    return amount;
}

bool outfit::takeoff( item_location loc, std::list<item> *res, Character &guy )
{
    item &it = *loc;
    const auto ret = guy.can_takeoff( it, res );
    if( !ret.success() ) {
        add_msg( m_info, "%s", ret.c_str() );
        return false;
    }

    auto iter = std::find_if( worn.begin(), worn.end(), [&it]( const item & wit ) {
        return &it == &wit;
    } );

    it.on_takeoff( guy );
    cata::event e = cata::event::make<event_type::character_takeoff_item>( guy.getID(),
                    it.typeId() );
    get_event_bus().send_with_talker( &guy, &loc, e );
    // Catching eoc of character_takeoff_item event may cause item to be invalid.
    // If so, skip worn.erase and guy.i_add or res->push_back.
    bool is_item_vaild = static_cast<bool>( loc );
    if( !is_item_vaild ) {
        return true;
    }
    item takeoff_copy( it );
    worn.erase( iter );
    if( res == nullptr ) {
        guy.i_add( takeoff_copy, true, &it, &it, true, !guy.has_weapon() );
    } else {
        res->push_back( takeoff_copy );
    }
    return true;
}

void outfit::damage_mitigate( const bodypart_id &bp, damage_unit &dam ) const
{
    for( const item &cloth : worn ) {
        if( cloth.get_coverage( bp ) == 100 && cloth.covers( bp ) ) {
            cloth.mitigate_damage( dam );
        }
    }
}

bool outfit::empty() const
{
    return worn.empty();
}

int outfit::get_coverage( bodypart_id bp, item::cover_type cover_type ) const
{
    int total_cover = 0;
    for( const item &it : worn ) {
        if( it.covers( bp ) ) {
            total_cover += it.get_coverage( bp, cover_type );
        }
    }
    return total_cover;
}

int outfit::coverage_with_flags_exclude( const bodypart_id &bp,
        const std::vector<flag_id> &flags ) const
{
    int coverage = 0;
    for( const item &clothing : worn ) {
        if( clothing.covers( bp ) ) {
            bool has_flag = false;
            for( const flag_id &flag : flags ) {
                if( clothing.has_flag( flag ) ) {
                    has_flag = true;
                    break;
                }
            }
            if( !has_flag ) {
                coverage += clothing.get_coverage( bp );
            }
        }
    }
    return coverage;
}

int outfit::sum_filthy_cover( bool ranged, bool melee, bodypart_id bp ) const
{
    int sum_cover = 0;
    for( const item &i : worn ) {
        if( i.covers( bp ) && i.is_filthy() ) {
            if( melee ) {
                sum_cover += i.get_coverage( bp, item::cover_type::COVER_MELEE );
            } else if( ranged ) {
                sum_cover += i.get_coverage( bp, item::cover_type::COVER_RANGED );
            } else {
                sum_cover += i.get_coverage( bp );
            }
        }
    }
    return sum_cover;
}

void outfit::inv_dump( std::vector<item *> &ret )
{
    for( item &i : worn ) {
        if( !i.has_flag( flag_INTEGRATED ) ) {
            ret.push_back( &i );
        }
    }
}

void outfit::inv_dump( std::vector<const item *> &ret ) const
{
    for( const item &i : worn ) {
        ret.push_back( &i );
    }
}

bool outfit::covered_with_flag( const flag_id &f, const body_part_set &parts ) const
{
    body_part_set to_cover( parts );

    for( const item &elem : worn ) {
        if( !elem.has_flag( f ) ) {
            continue;
        }

        to_cover.substract_set( elem.get_covered_body_parts() );

        if( to_cover.none() ) {
            return true;    // Allows early exit.
        }
    }

    return to_cover.none();
}

units::volume outfit::free_space() const
{
    units::volume volume_capacity = 0_ml;
    for( const item &w : worn ) {
        volume_capacity += w.get_total_capacity();
        for( const item_pocket *pocket : w.get_all_contained_pockets() ) {
            if( pocket->contains_phase( phase_id::SOLID ) ) {
                for( const item *it : pocket->all_items_top() ) {
                    volume_capacity -= it->volume();
                }
            } else if( !pocket->empty() ) {
                volume_capacity -= pocket->volume_capacity();
            }
        }
        volume_capacity += w.check_for_free_space();
    }
    return volume_capacity;
}

units::mass outfit::free_weight_capacity() const
{
    units::mass weight_capacity = 0_gram;
    for( const item &w : worn ) {
        weight_capacity += w.get_remaining_weight_capacity();
    }
    return weight_capacity;
}

units::volume outfit::holster_volume() const
{
    units::volume ret = 0_ml;
    for( const item &w : worn ) {
        if( w.is_holster() ) {
            ret += w.get_total_capacity();
        }
    }
    return ret;
}

int outfit::empty_holsters() const
{
    int e_holsters = 0;
    for( const item &w : worn ) {
        if( w.is_holster() && w.is_container_empty() ) {
            e_holsters += 1;
        }
    }
    return e_holsters;
}

int outfit::used_holsters() const
{
    int e_holsters = 0;
    for( const item &w : worn ) {
        e_holsters += w.get_used_holsters();
    }
    return e_holsters;
}

int outfit::total_holsters() const
{
    int e_holsters = 0;
    for( const item &w : worn ) {
        e_holsters += w.get_total_holsters();
    }
    return e_holsters;
}

units::volume outfit::free_holster_volume() const
{
    units::volume holster_volume = 0_ml;
    for( const item &w : worn ) {
        holster_volume += w.get_total_holster_volume() - w.get_used_holster_volume();
    }
    return holster_volume;
}

units::volume outfit::small_pocket_volume( const units::volume &threshold )  const
{
    units::volume small_spaces = 0_ml;
    for( const item &w : worn ) {
        if( !w.is_holster() ) {
            for( const item_pocket *pocket : w.get_all_contained_pockets() ) {
                if( pocket->volume_capacity() <= threshold ) {
                    small_spaces += pocket->volume_capacity();
                }
            }
        }
    }
    return small_spaces;
}

units::volume outfit::volume_capacity() const
{
    units::volume cap = 0_ml;
    for( const item &w : worn ) {
        cap += w.get_total_capacity();
    }
    return cap;
}

units::volume outfit::volume_capacity_with_tweaks(
    const std::map<const item *, int> &without ) const
{
    units::volume vol = 0_ml;
    for( const item &i : worn ) {
        if( !without.count( &i ) ) {
            vol += i.get_total_capacity();
        }
    }
    return vol;
}

int outfit::pocket_warmth() const
{
    int warmth = 0;
    for( const item &w : worn ) {
        if( w.has_flag( flag_POCKETS ) ) {
            warmth = std::max( warmth, w.get_warmth() );
        }
    }
    return warmth;
}

int outfit::hood_warmth() const
{
    int warmth = 0;
    for( const item &w : worn ) {
        if( w.has_flag( flag_HOOD ) ) {
            warmth = std::max( warmth, w.get_warmth() );
        }
    }
    return warmth;
}

int outfit::collar_warmth() const
{
    int warmth = 0;
    for( const item &w : worn ) {
        if( w.has_flag( flag_COLLAR ) ) {
            warmth = std::max( warmth, w.get_warmth() );
        }
    }
    return warmth;
}

std::list<item> outfit::use_amount( const itype_id &it, int quantity,
                                    std::list<item> &used, const std::function<bool( const item & )> &filter,
                                    Character &wearer )
{
    for( auto a = worn.begin(); a != worn.end() && quantity > 0; ) {
        if( a->use_amount( it, quantity, used, filter ) ) {
            a->on_takeoff( wearer );
            a = worn.erase( a );
        } else {
            ++a;
        }
    }
    return used;
}

void outfit::add_dependent_item( std::list<item *> &dependent, const item &it )
{
    // Adds dependent worn items recursively
    for( item &wit : worn ) {
        if( &wit == &it || !wit.is_worn_only_with( it ) ) {
            continue;
        }
        const auto iter = std::find_if( dependent.begin(), dependent.end(),
        [&wit]( const item * dit ) {
            return &wit == dit;
        } );
        if( iter == dependent.end() ) { // Not in the list yet
            add_dependent_item( dependent, wit );
            dependent.push_back( &wit );
        }
    }
}

bool outfit::can_pickVolume( const item &it, const bool ignore_pkt_settings,
                             const bool ignore_non_container_pocket ) const
{
    for( const item &w : worn ) {
        if( w.can_contain( it, false, false, ignore_pkt_settings, ignore_non_container_pocket
                         ).success() ) {
            return true;
        }
    }
    return false;
}

static void add_overlay_id_or_override( const item &item,
                                        std::vector<std::pair<std::string, std::string>> &overlay_ids )
{
    if( item.has_var( "sprite_override" ) ) {
        overlay_ids.emplace_back( "worn_" + item.get_var( "sprite_override" ),
                                  item.get_var( "sprite_override_variant", "" ) );
    } else {
        const std::string variant = item.has_itype_variant() ? item.itype_variant().id : "";
        overlay_ids.emplace_back( "worn_" + item.typeId().str(), variant );
    }
}

void outfit::get_overlay_ids( std::vector<std::pair<std::string, std::string>> &overlay_ids ) const
{
    // TODO: worry about correct order of clothing overlays
    for( const item &worn_item : worn ) {
        if( worn_item.has_flag( json_flag_HIDDEN ) ) {
            continue;
        }
        add_overlay_id_or_override( worn_item, overlay_ids );
        for( const item *ablative : worn_item.all_ablative_armor() ) {
            add_overlay_id_or_override( *ablative, overlay_ids );
        }
    }
}

std::list<item>::iterator outfit::position_to_wear_new_item( const item &new_item )
{
    // By default we put this item on after the last item on the same or any
    // lower layer. Integrated armor goes under normal armor.
    return std::find_if(
               worn.rbegin(), worn.rend(),
    [&]( const item & w ) {
        if( w.has_flag( flag_INTEGRATED ) == new_item.has_flag( flag_INTEGRATED ) ) {
            return w.get_layer() <= new_item.get_layer();
        } else {
            return w.has_flag( flag_INTEGRATED );
        }
    }
           ).base();
}

body_part_set outfit::exclusive_flag_coverage( body_part_set bps, const flag_id &flag ) const
{
    for( const item &elem : worn ) {
        if( !elem.has_flag( flag ) ) {
            // Unset the parts covered by this item
            bps.substract_set( elem.get_covered_body_parts() );
        }
    }
    return bps;
}

item &outfit::front()
{
    return worn.front();
}

void outfit::absorb_damage( Character &guy, damage_unit &elem, bodypart_id bp,
                            std::list<item> &worn_remains, bool &armor_destroyed )
{
    const map &here = get_map();

    sub_bodypart_id sbp;
    sub_bodypart_id secondary_sbp;
    // if this body part has sub part locations roll one
    if( !bp->sub_parts.empty() ) {
        sbp = bp->random_sub_part( false );
        // the torso and legs has a second layer of hanging body parts
        secondary_sbp = bp->random_sub_part( true );
    }

    // generate a single roll for determining if hit
    int roll = rng( 1, 100 );

    // Only the outermost armor can be set on fire
    bool outermost = true;
    // The worn vector has the innermost item first, so
    // iterate reverse to damage the outermost (last in worn vector) first.
    for( auto iter = worn.rbegin(); iter != worn.rend(); ) {
        item &armor = *iter;

        if( !armor.covers( bp ) ) {
            ++iter;
            continue;
        }

        const std::string pre_damage_name = armor.tname();
        bool destroy = false;

        // Heat damage can set armor on fire
        // Even though it doesn't cause direct physical damage to it
        // FIXME: Hardcoded damage type
        if( outermost && elem.type == STATIC( damage_type_id( "heat" ) ) && elem.amount >= 1.0f ) {
            // TODO: Different fire intensity values based on damage
            fire_data frd{ 2 };
            destroy = !armor.has_flag( flag_INTEGRATED ) && armor.burn( frd );
            int fuel = roll_remainder( frd.fuel_produced );
            if( fuel > 0 ) {
                guy.add_effect( effect_onfire, time_duration::from_turns( fuel + 1 ), bp, false, 0, false,
                                true );
            }
        }

        if( !destroy ) {
            // if the armor location has ablative armor apply that first
            if( armor.is_ablative() ) {
                guy.ablative_armor_absorb( elem, armor, sbp, roll );
            }

            // if not already destroyed to an armor absorb
            destroy = guy.armor_absorb( elem, armor, bp, sbp, roll );
            // for the torso we also need to consider if it hits anything hanging off the character or their neck
            if( secondary_sbp != sub_bodypart_id() && !destroy ) {
                destroy = guy.armor_absorb( elem, armor, bp, secondary_sbp, roll );
            }
        }

        if( destroy ) {
            if( get_player_view().sees( here, guy ) ) {
                SCT.add( guy.pos_bub( here ).xy().raw(), direction::NORTH, remove_color_tags( pre_damage_name ),
                         m_neutral, _( "destroyed" ), m_info );
            }
            destroyed_armor_msg( guy, pre_damage_name );
            armor_destroyed = true;
            armor.on_takeoff( guy );

            item_location loc = item_location( guy, &armor );
            cata::event e = cata::event::make<event_type::character_armor_destroyed>( guy.getID(),
                            armor.typeId() );
            get_event_bus().send_with_talker( &guy, &loc, e );
            for( const item *it : armor.all_items_top( pocket_type::CONTAINER ) ) {
                worn_remains.push_back( *it );
            }
            // decltype is the type name of the iterator, note that reverse_iterator::base returns the
            // iterator to the next element, not the one the revers_iterator points to.
            // http://stackoverflow.com/questions/1830158/how-to-call-erase-with-a-reverse-iterator
            iter = decltype( iter )( worn.erase( --iter.base() ) );
        } else {
            ++iter;
            outermost = false;
        }
    }
}

float outfit::damage_resist( const damage_type_id &dt, const bodypart_id &bp,
                             const bool to_self ) const
{
    float ret = 0.0f;
    for( const item &i : worn ) {
        if( i.covers( bp ) ) {
            ret += i.resist( dt, to_self, bp );
        }
    }
    return ret;
}

int outfit::get_env_resist( bodypart_id bp ) const
{
    int ret = 0;
    for( const item &i : worn ) {
        // Head protection works on eyes too (e.g. baseball cap)
        if( i.covers( bp ) || ( bp == body_part_eyes && i.covers( body_part_head ) ) ) {
            ret += i.get_env_resist();
        }
    }
    return ret;
}

std::map<bodypart_id, int> outfit::warmth( const Character &guy ) const
{
    std::map<bodypart_id, int> total_warmth;
    for( const bodypart_id &bp : guy.get_all_body_parts() ) {
        double warmth_val = 0.0;
        const float wetness_pct = guy.get_part_wetness_percentage( bp );
        for( const item &clothing : worn ) {
            if( !clothing.covers( bp ) ) {
                continue;
            }
            warmth_val = clothing.get_warmth( bp );
            // Wool items do not lose their warmth due to being wet.
            // Warmth is reduced by 0 - 66% based on wetness.
            if( !clothing.made_of( material_wool ) ) {
                warmth_val *= 1.0 - 0.66 * wetness_pct;
            }

            total_warmth[bp] += warmth_val;
        }
        total_warmth[bp] += guy.get_effect_int( effect_heating_bionic, bp );
    }
    return total_warmth;
}

std::unordered_set<bodypart_id> outfit::where_discomfort( const Character &guy ) const
{
    // get all rigid body parts to begin with
    std::unordered_set<sub_bodypart_id> covered_sbps;
    std::unordered_set<bodypart_id> uncomfortable_bps;

    for( const item &i : worn ) {
        // check each sublimb individually
        for( const sub_bodypart_id &sbp : i.get_covered_sub_body_parts() ) {
            if( i.is_bp_comfortable( sbp ) ) {
                covered_sbps.insert( sbp );
            }
            // if the bp is uncomfortable and has yet to display as covered with something comfortable then it should cause discomfort
            // note anything selectively rigid reasonably can be assumed to support itself so we don't need to worry about this
            // items must also be somewhat heavy in order to cause discomfort
            if( !i.is_bp_rigid_selective( sbp ) && !i.is_bp_comfortable( sbp ) &&
                i.weight() > units::from_gram( 250 ) ) {

                // need to go through each locations under location to check if its covered, since secondary locations can cover multiple underlying locations
                for( const sub_bodypart_str_id &under_sbp : sbp->locations_under ) {
                    if( covered_sbps.count( under_sbp ) != 1 ) {
                        guy.add_msg_if_player(
                            string_format( _( "<color_c_red>The %s rubs uncomfortably against your unpadded %s.</color>" ),
                                           i.display_name(), under_sbp->name ) );
                        uncomfortable_bps.insert( sbp->parent );
                    }
                }
            }
        }
    }

    return uncomfortable_bps;
}

void outfit::fire_options( Character &guy, std::vector<std::string> &options,
                           std::vector<std::function<void()>> &actions )
{
    for( item &clothing : worn ) {
        std::vector<item *> guns = clothing.items_with( []( const item & it ) {
            return it.is_gun();
        } );

        if( !guns.empty() && clothing.type->can_use( "holster" ) ) {
            //~ draw (first) gun contained in holster
            //~ %1$s: weapon name, %2$s: container name, %3$d: remaining ammo count
            options.push_back( string_format( pgettext( "holster", "%1$s from %2$s (%3$d)" ),
                                              guns.front()->tname(),
                                              clothing.type_name(),
                                              guns.front()->ammo_remaining( ) ) );

            actions.emplace_back( [&] { guy.invoke_item( &clothing, "holster" ); } );

        } else if( clothing.is_gun() && clothing.gunmod_find_by_flag( flag_BELTED ) ) {
            // wield item currently worn using shoulder strap
            options.push_back( clothing.display_name() );
            actions.emplace_back( [&] { guy.wield( clothing ); } );
        }
    }
}

void outfit::insert_item_at_index( const item &clothing, int index )
{
    if( static_cast<size_t>( index ) >= worn.size() || index == INT_MIN ) {
        index = worn.size() - 1;
    }
    auto it = worn.begin();
    std::advance( it, index );
    worn.insert( it, clothing );
}

units::length outfit::max_single_item_length() const
{
    units::length ret = 0_mm;
    for( const item &worn_it : worn ) {
        units::length candidate = worn_it.max_containable_length();
        if( candidate > ret ) {
            ret = candidate;
        }
    }
    return ret;
}

units::volume outfit::max_single_item_volume() const
{
    units::volume ret = 0_ml;
    for( const item &worn_it : worn ) {
        units::volume candidate = worn_it.max_containable_volume();
        if( candidate > ret ) {
            ret = candidate;
        }
    }
    return ret;
}

void outfit::best_pocket( Character &guy, const item &it, const item *avoid,
                          std::pair<item_location, item_pocket *> &current_best, bool ignore_settings )
{
    for( item &worn_it : worn ) {
        if( &worn_it == &it || &worn_it == avoid ) {
            continue;
        }
        item_location loc( guy, &worn_it );
        std::pair<item_location, item_pocket *> internal_pocket =
            worn_it.best_pocket( it, loc, avoid, false, ignore_settings );
        if( internal_pocket.second != nullptr &&
            ( current_best.second == nullptr ||
              current_best.second->better_pocket( *internal_pocket.second, it ) ) ) {
            current_best = internal_pocket;
        }
    }
}

void outfit::overflow( Character &guy )
{
    map &here = get_map();

    for( item_location &clothing : top_items_loc( guy ) ) {
        clothing.overflow( here );
    }
}

int outfit::worn_guns() const
{
    int ret = 0;
    for( const item &clothing : worn ) {
        // true bool casts to 1, false to 0
        ret += clothing.is_gun();
    }
    return ret;
}

void outfit::set_fitted()
{
    for( item &clothing : worn ) {
        if( clothing.has_flag( flag_VARSIZE ) ) {
            clothing.set_flag( flag_FIT );
        }
    }
}

void outfit::on_takeoff( Character &guy )
{
    for( item &it : worn ) {
        it.on_takeoff( guy );
    }
}

void outfit::on_item_wear( Character &guy )
{
    for( item &clothing : worn ) {
        if( clothing.relic_data && clothing.type->relic_data ) {
            clothing.relic_data = clothing.type->relic_data;
        }
        guy.on_item_wear( clothing );
    }
}

std::vector<item> outfit::available_pockets() const
{
    std::vector<item> avail_pockets;
    for( const item &clothing : worn ) {
        if( clothing.is_container() || clothing.is_holster() ) {
            avail_pockets.push_back( clothing );
        }
    }
    return avail_pockets;
}

item *outfit::best_shield()
{
    int best_val = 0;
    item *ret = nullptr;
    for( item &shield : worn ) {
        const int block = melee::blocking_ability( shield );
        if( shield.has_flag( flag_BLOCK_WHILE_WORN ) && block >= best_val ) {
            best_val = block;
            ret = &shield;
        }
    }
    return ret;
}

item *outfit::current_unarmed_weapon( const sub_bodypart_str_id &contact_area )
{
    item *cur_weapon = &null_item_reference();
    for( item &worn_item : worn ) {
        bool covers = worn_item.covers( contact_area );
        // Uses enum layer_level to make distinction for top layer.
        if( covers ) {
            if( cur_weapon->is_null() || ( worn_item.get_layer() >= cur_weapon->get_layer() ) ) {
                cur_weapon = &worn_item;
            }
        }
    }
    return cur_weapon;
}

void outfit::prepare_bodymap_info( bodygraph_info &info, const bodypart_id &bp,
                                   const std::set<sub_bodypart_id> &sub_parts, const Character &person ) const
{
    std::map<sub_bodypart_id, resistances> best_cases;
    std::map<sub_bodypart_id, resistances> median_cases;
    std::map<sub_bodypart_id, resistances> worst_cases;

    // general body part stats
    info.part_hp_cur = person.get_part_hp_cur( bp );
    info.part_hp_max = person.get_part_hp_max( bp );
    info.wetness = person.get_part_wetness_percentage( bp );
    info.temperature = { units::to_legacy_bodypart_temp( person.get_part_temp_conv( bp ) ), display::bodytemp_color( person, bp ) };
    std::pair<std::string, nc_color> tmp_approx = display::temp_text_color( person, bp.id() );
    info.temp_approx = colorize( tmp_approx.first, tmp_approx.second );
    info.parent_bp_name = uppercase_first_letter( bp->name.translated() );

    // body part effects
    for( const effect &eff : person.get_effects() ) {
        if( eff.get_bp() == bp ) {
            info.effects.emplace_back( eff );
        }
    }

    // go through every item and see how it handles these part(s) of the character
    for( const item &armor : worn ) {
        // check if it covers the part
        // FIXME: item::covers( const sub_bodypart_id & ) always
        // returns true if there is no sub_data (ex: hairpin),
        // so use item::get_covered_sub_body_parts() instead
        bool covered = false;
        if( sub_parts.empty() || sub_parts.size() > 1 ) {
            covered = armor.covers( bp );
        } else {
            info.specific_sublimb = true;
            const std::vector<sub_bodypart_id> splist = armor.get_covered_sub_body_parts();
            if( std::find( splist.begin(), splist.end(), *sub_parts.begin() ) != splist.end() ) {
                covered = true;
            }
        }
        if( !covered ) {
            // some clothing flags provide warmth without providing coverage or encumbrance
            // these are included in the worn list so that players aren't confused about why body parts are warm
            // but not included in the later coverage and encumbrance calculations
            if( ( bp == body_part_hand_l || bp == body_part_hand_r ) && armor.has_flag( flag_POCKETS ) &&
                person.can_use_pockets() ) {
                //~ name of a clothing/armor item, indicating it has pockets providing hand warmth
                info.worn_names.push_back( string_format( _( "%s (pockets)" ), armor.tname() ) );
            }
            if( bp == body_part_head && armor.has_flag( flag_HOOD ) && person.can_use_hood() ) {
                //~ name of a clothing/armor item, indicating it has a hood providing head warmth
                info.worn_names.push_back( string_format( _( "%s (hood)" ), armor.tname() ) );
            }
            if( bp == body_part_mouth && armor.has_flag( flag_COLLAR ) && person.can_use_collar() ) {
                // collars don't cover or encumber, but do conditionally provide warmth
                //~ name of a clothing/armor item, indicating it has a collar providing mouth warmth
                info.worn_names.push_back( string_format( _( "%s (collar)" ), armor.tname() ) );
            }

            continue;
        }

        info.worn_names.push_back( armor.tname() );

        info.total_encumbrance += armor.get_encumber( person, bp );

        // need to average the coverage on each sub part based on size
        int temp_coverage = 0;
        if( sub_parts.size() == 1 ) {
            temp_coverage = armor.get_coverage( *sub_parts.begin() );
        } else {
            // bp armor already has averaged coverage
            temp_coverage = armor.get_coverage( bp );
        }
        info.avg_coverage += temp_coverage;

        // need to do each sub part seperately and then average them if need be
        for( const sub_bodypart_id &sbp : sub_parts ) {
            int coverage = armor.get_coverage( sbp );

            // get worst case armor protection
            // if it doesn't 100% cover it may not protect you
            if( coverage == 100 ) {
                worst_cases[sbp] += resistances( armor, false, 99, sbp );
            }

            // get median case armor protection
            // if it doesn't at least 50% cover it may not protect you
            if( coverage >= 50 ) {
                median_cases[sbp] += resistances( armor, false, 50, sbp );
            }

            // get best case armor protection
            best_cases[sbp] += resistances( armor, false, 0, sbp );
        }

    }

    // need to average the protection values on each sublimb for full limbs
    if( sub_parts.size() == 1 ) {
        info.worst_case += worst_cases[*sub_parts.begin()];
        info.median_case += median_cases[*sub_parts.begin()];
        info.best_case += best_cases[*sub_parts.begin()];
    } else {
        for( const sub_bodypart_id &sbp : sub_parts ) {
            float scale_factor = static_cast<float>( sbp->max_coverage ) / 100.0f;
            info.worst_case += worst_cases[sbp] * scale_factor;
            info.median_case += median_cases[sbp] * scale_factor;
            info.best_case += best_cases[sbp] * scale_factor;
        }
    }

    // finally average the at this point cumulative average coverage by the number of articles
    if( !info.worn_names.empty() ) {
        info.avg_coverage = info.avg_coverage / info.worn_names.size();
    }
}

void outfit::bodypart_exposure( std::map<bodypart_id, float> &bp_exposure,
                                const std::vector<bodypart_id> &all_body_parts ) const
{
    for( const item &it : worn ) {
        // What body parts does this item cover?
        body_part_set covered = it.get_covered_body_parts();
        for( const bodypart_id &bp : all_body_parts ) {
            if( !covered.test( bp.id() ) ) {
                continue;
            }
            float part_exposure = ( 100 - it.get_coverage( bp ) ) / 100.0f;
            // Coverage multiplies, so two layers with 50% coverage will together give 75%
            bp_exposure[bp] *= part_exposure;
        }
    }
}

void outfit::pickup_stash( const item &newit, int &remaining_charges, bool ignore_pkt_settings )
{
    for( item &i : worn ) {
        if( remaining_charges == 0 ) {
            break;
        }
        if( i.can_contain_partial( newit ).success() ) {
            const int used_charges =
                i.fill_with( newit, remaining_charges, false, false, ignore_pkt_settings );
            remaining_charges -= used_charges;
        }
    }
}

static std::vector<pocket_data_with_parent> get_child_pocket_with_parent(
    const item_pocket *pocket, const item_location &parent, item_location it, const int nested_level,
    const std::function<bool( const item_pocket * )> &filter = return_true<const item_pocket *> )
{
    std::vector<pocket_data_with_parent> ret;
    if( pocket != nullptr ) {
        pocket_data_with_parent pocket_data = { pocket, item_location::nowhere, nested_level };
        const item_location new_parent = item_location( it );

        if( parent != item_location::nowhere ) {
            pocket_data.parent = item_location( parent, it.get_item() );
        }
        if( filter( pocket_data.pocket_ptr ) ) {
            ret.emplace_back( pocket_data );
        }

        for( const item *contained : pocket->all_items_top() ) {
            const item_location poc_loc = item_location( it, const_cast<item *>( contained ) );
            for( const item_pocket *pocket_nest : contained->get_all_contained_pockets() ) {
                std::vector<pocket_data_with_parent> child =
                    get_child_pocket_with_parent( pocket_nest, new_parent,
                                                  poc_loc, nested_level + 1, filter );
                ret.insert( ret.end(), child.begin(), child.end() );
            }
        }
    }
    return ret;
}

std::vector<pocket_data_with_parent> Character::get_all_pocket_with_parent(
    const std::function<bool( const item_pocket * )> &filter,
    const std::function<bool( const pocket_data_with_parent &a, const pocket_data_with_parent &b )>
    *sort_func )
{
    std::vector<pocket_data_with_parent> ret;
    const auto sort_pockets_func = [ sort_func ]
    ( const pocket_data_with_parent & l, const pocket_data_with_parent & r ) {
        const auto sort_pockets = *sort_func;
        return sort_pockets( l, r );
    };

    std::list<item_location> locs;
    item_location carried_item = get_wielded_item();
    if( carried_item != item_location::nowhere ) {
        locs.emplace_back( carried_item );
    }
    for( item_location &worn_loc : top_items_loc( ) ) {
        if( worn_loc != item_location::nowhere ) {
            locs.emplace_back( worn_loc );
        }
    }

    for( item_location &loc : locs ) {
        for( const item_pocket *pocket : loc->get_all_contained_pockets() ) {
            std::vector<pocket_data_with_parent> child =
                get_child_pocket_with_parent( pocket, item_location::nowhere, loc, 0, filter );
            ret.insert( ret.end(), child.begin(), child.end() );
        }
    }
    if( sort_func ) {
        std::sort( ret.begin(), ret.end(), sort_pockets_func );
    }
    return ret;
}

void outfit::add_stash( Character &guy, const item &newit, int &remaining_charges,
                        bool ignore_pkt_settings )
{
    guy.invalidate_weight_carried_cache();
    if( ignore_pkt_settings ) {
        // Crawl all pockets regardless of priority
        // Crawl First : wielded item
        item_location carried_item = guy.get_wielded_item();
        if( carried_item && !carried_item->has_pocket_type( pocket_type::MAGAZINE ) &&
            carried_item->can_contain_partial( newit ).success() ) {
            int used_charges = carried_item->fill_with( newit, remaining_charges, /*unseal_pockets=*/false,
                               /*allow_sealed=*/false, /*ignore_settings=*/false, /*into_bottom*/false, /*allow_nested*/true,
                               &guy );
            remaining_charges -= used_charges;
        }
        // Crawl Next : worn items
        pickup_stash( newit, remaining_charges, ignore_pkt_settings );
    } else {
        //item copy for test can contain
        item temp_it = item( newit );
        temp_it.charges = 1;

        // Collect all pockets
        std::vector<pocket_data_with_parent> pockets_with_parent;
        auto const pocket_filter = [&temp_it]( item_pocket const * pck ) {
            return pck->can_contain( temp_it ).success();
        };
        const std::function<bool( const pocket_data_with_parent &a, const pocket_data_with_parent &b )>
        &sort_f = [&temp_it]( const pocket_data_with_parent & a, const pocket_data_with_parent & b ) {
            return b.pocket_ptr->better_pocket( *a.pocket_ptr, temp_it, false );
        };
        pockets_with_parent = guy.get_all_pocket_with_parent( pocket_filter, &sort_f );

        const int amount = remaining_charges;
        int num_contained = 0;
        for( const pocket_data_with_parent &pocket_data_ptr : pockets_with_parent ) {
            if( amount <= num_contained || remaining_charges <= 0 ) {
                break;
            }
            int filled_count = 0;
            item_pocket *pocke = const_cast<item_pocket *>( pocket_data_ptr.pocket_ptr );
            if( pocke == nullptr ) {
                continue;
            }
            int max_contain_value = pocke->remaining_capacity_for_item( newit );
            const item_location parent_data = pocket_data_ptr.parent;

            if( parent_data.has_parent() ) {
                if( parent_data.parents_can_contain_recursive( &temp_it ).success() ) {
                    max_contain_value = parent_data.max_charges_by_parent_recursive( temp_it ).value();
                } else {
                    max_contain_value = 0;
                }
            }
            const int charges = std::min( max_contain_value, remaining_charges ) ;
            filled_count = pocke->fill_with( newit, guy, charges, false, false );
            num_contained += filled_count;
            remaining_charges -= filled_count;
        }
    }
}

void outfit::write_text_memorial( std::ostream &file, const std::string &indent,
                                  const char *eol ) const
{
    for( const item &elem : worn ) {
        item next_item = elem;
        file << indent << next_item.invlet << " - " << next_item.tname( 1, false );
        if( next_item.charges > 0 ) {
            file << " (" << next_item.charges << ")";
        }
        file << eol;
    }
}

bool outfit::hands_conductive() const
{
    for( const item &i : worn ) {
        if( !i.conductive()
            && ( ( i.get_coverage( bodypart_id( "hand_l" ) ) >= 95 ) ||
                 i.get_coverage( bodypart_id( "hand_r" ) ) >= 95 ) ) {
            return false;
        }
    }
    return true;
}

item_location outfit::first_item_covering_bp( Character &guy, bodypart_id bp )
{
    for( item &clothing : worn ) {
        if( clothing.covers( bp ) ) {
            return item_location( guy, &clothing );
        }
    }
    return item_location{};
}

std::optional<int> outfit::get_item_position( const item &it ) const
{
    int pos = 0;
    for( const item &clothing : worn ) {
        if( clothing.has_item( it ) ) {
            return pos;
        }
        pos++;
    }
    return std::nullopt;
}

const item &outfit::i_at( int position ) const
{
    if( static_cast<size_t>( position ) < worn.size() ) {
        auto iter = worn.begin();
        std::advance( iter, position );
        return *iter;
    } else {
        return null_item_reference();
    }
}

std::string outfit::get_armor_display( bodypart_id bp ) const
{
    for( auto it = worn.rbegin(); it != worn.rend(); ++it ) {
        if( it->covers( bp ) ) {
            return it->tname( 1 );
        }
    }
    return "-";
}

void outfit::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "worn", worn );
    json.end_object();
}

void outfit::deserialize( const JsonObject &jo )
{
    jo.read( "worn", worn );
}

int outfit::clatter_sound() const
{
    int max_volume = 0;
    for( const item &i : worn ) {
        // if the item has noise making pockets we should check if they have clatered
        if( i.has_noisy_pockets() ) {
            for( const item_pocket *pocket : i.get_all_contained_pockets() ) {
                int noise_chance = pocket->get_pocket_data()->activity_noise.chance;
                int volume = pocket->get_pocket_data()->activity_noise.volume;
                if( noise_chance > 0 && !pocket->empty() ) {
                    // if this pocket causes more volume and it triggers noise
                    if( volume > max_volume && rng( 1, 100 ) < noise_chance ) {
                        max_volume = volume;
                    }
                }
            }
        }
    }
    return std::round( max_volume );
}

float outfit::clothing_wetness_mult( const bodypart_id &bp ) const
{
    float clothing_mult = 1.0;
    for( const item &i : worn ) {
        if( i.covers( bp ) ) {
            const float item_coverage = static_cast<float>( i.get_coverage( bp ) ) / 100;
            const float item_breathability = static_cast<float>( i.breathability( bp ) ) / 100;

            // breathability of naked skin + breathability of item
            const float breathability = ( 1.0 - item_coverage ) + item_coverage * item_breathability;

            clothing_mult = std::min( clothing_mult, breathability );
        }
    }

    // always some evaporation even if completely covered
    // doesn't handle things that would be "air tight"
    clothing_mult = std::max( clothing_mult, .1f );
    return clothing_mult;
}

std::vector<item_pocket *> outfit::grab_drop_pockets()
{
    std::vector<item_pocket *> pd;
    for( item &i : worn ) {
        if( i.has_ripoff_pockets() ) {
            for( item_pocket *pocket : i.get_all_contained_pockets() ) {
                if( pocket->get_pocket_data()->ripoff > 0 && !pocket->empty() ) {
                    pd.push_back( pocket );
                }
            }
        }
    }
    return pd;
}

std::vector<item_pocket *> outfit::grab_drop_pockets( const bodypart_id &bp )
{
    std::vector<item_pocket *> pd;
    for( item &i : worn ) {
        // Check if we wear it on the grabbed limb in the first place
        if( !is_wearing_on_bp( i.typeId(), bp ) ) {
            continue;
        }
        if( i.has_ripoff_pockets() ) {
            for( item_pocket *pocket : i.get_all_contained_pockets() ) {
                if( pocket->get_pocket_data()->ripoff > 0 && !pocket->empty() ) {
                    pd.push_back( pocket );
                }
            }
        }
    }
    return pd;
}

void outfit::organize_items_menu()
{
    std::vector<item *> to_organize;
    for( item &i : worn ) {
        to_organize.push_back( &i );
    }
    pocket_management_menu( _( "Inventory Organization" ), to_organize );
}
