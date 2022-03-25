#include "character_attire.h"

#include "character.h"
#include "event.h"
#include "event_bus.h"
#include "flag.h"
#include "game.h"
#include "inventory.h"
#include "itype.h"
#include "melee.h"
#include "memorial_logger.h"
#include "messages.h"
#include "mutation.h"
#include "output.h"

static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_heating_bionic( "heating_bionic" );
static const efftype_id effect_incorporeal( "incorporeal" );
static const efftype_id effect_onfire( "onfire" );

static const flag_id json_flag_HIDDEN( "HIDDEN" );
static const flag_id json_flag_ONE_PER_LAYER( "ONE_PER_LAYER" );

static const itype_id itype_shoulder_strap( "shoulder_strap" );

static const material_id material_cotton( "cotton" );
static const material_id material_leather( "leather" );
static const material_id material_nomex( "nomex" );
static const material_id material_wool( "wool" );

static const trait_id trait_ANTENNAE( "ANTENNAE" );
static const trait_id trait_ANTLERS( "ANTLERS" );
static const trait_id trait_HORNS_POINTED( "HORNS_POINTED" );
static const trait_id trait_SQUEAMISH( "SQUEAMISH" );
static const trait_id trait_WOOLALLERGY( "WOOLALLERGY" );

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

ret_val<bool> Character::can_wear( const item &it, bool with_equip_change ) const
{
    if( it.has_flag( flag_CANT_WEAR ) ) {
        return ret_val<bool>::make_failure( _( "Can't be worn directly." ) );
    }
    if( has_effect( effect_incorporeal ) ) {
        return ret_val<bool>::make_failure( _( "You can't wear anything while incorporeal." ) );
    }
    if( !it.is_armor() ) {
        return ret_val<bool>::make_failure( _( "Putting on a %s would be tricky." ), it.tname() );
    }

    if( has_trait( trait_WOOLALLERGY ) && ( it.made_of( material_wool ) ||
                                            it.has_own_flag( flag_wooled ) ) ) {
        return ret_val<bool>::make_failure( _( "Can't wear that, it's made of wool!" ) );
    }

    if( it.is_filthy() && has_trait( trait_SQUEAMISH ) ) {
        return ret_val<bool>::make_failure( _( "Can't wear that, it's filthy!" ) );
    }

    if( !it.has_flag( flag_OVERSIZE ) && !it.has_flag( flag_SEMITANGIBLE ) ) {
        for( const trait_id &mut : get_mutations() ) {
            const auto &branch = mut.obj();
            if( branch.conflicts_with_item( it ) ) {
                return ret_val<bool>::make_failure( is_avatar() ?
                                                    _( "Your %s mutation prevents you from wearing your %s." ) :
                                                    _( "My %s mutation prevents me from wearing this %s." ), branch.name(),
                                                    it.type_name() );
            }
        }
        if( it.covers( body_part_head ) && !it.has_flag( flag_SEMITANGIBLE ) &&
            !it.made_of( material_wool ) && !it.made_of( material_cotton ) &&
            !it.made_of( material_nomex ) && !it.made_of( material_leather ) &&
            ( has_trait( trait_HORNS_POINTED ) || has_trait( trait_ANTENNAE ) ||
              has_trait( trait_ANTLERS ) ) ) {
            return ret_val<bool>::make_failure( _( "Cannot wear a helmet over %s." ),
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
            return ret_val<bool>::make_failure( is_avatar() ?
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
            return ret_val<bool>::make_failure( msg );
        }
    }

    if( it.has_flag( flag_RESTRICT_HANDS ) && !has_min_manipulators() ) {
        return ret_val<bool>::make_failure( ( is_avatar() ? _( "You don't have enough arms to wear that." )
                                              : string_format( _( "%s doesn't have enough arms to wear that." ), get_name() ) ) );
    }

    //Everything checked after here should be something that could be solved by changing equipment
    if( with_equip_change ) {
        return ret_val<bool>::make_success();
    }

    {
        const ret_val<bool> power_armor_conflicts = worn.power_armor_conflicts( it );
        if( !power_armor_conflicts.success() ) {
            return power_armor_conflicts;
        }
    }

    // Check if we don't have both hands available before wearing a briefcase, shield, etc. Also occurs if we're already wearing one.
    if( it.has_flag( flag_RESTRICT_HANDS ) && ( worn_with_flag( flag_RESTRICT_HANDS ) ||
            weapon.is_two_handed( *this ) ) ) {
        return ret_val<bool>::make_failure( ( is_avatar() ? _( "You don't have a hand free to wear that." )
                                              : string_format( _( "%s doesn't have a hand free to wear that." ), get_name() ) ) );
    }

    {
        ret_val<bool> conflict = worn.only_one_conflicts( it );
        if( !conflict.success() ) {
            return conflict;
        }
    }

    if( amount_worn( it.typeId() ) >= MAX_WORN_PER_TYPE ) {
        return ret_val<bool>::make_failure( _( "Can't wear %1$i or more %2$s at once." ),
                                            MAX_WORN_PER_TYPE + 1, it.tname( MAX_WORN_PER_TYPE + 1 ) );
    }

    if( ( ( it.covers( body_part_foot_l ) && is_wearing_shoes( side::LEFT ) ) ||
          ( it.covers( body_part_foot_r ) && is_wearing_shoes( side::RIGHT ) ) ) &&
        ( !it.has_flag( flag_OVERSIZE ) || !it.has_flag( flag_OUTER ) ) && !it.has_flag( flag_SKINTIGHT ) &&
        !it.has_flag( flag_BELTED ) && !it.has_flag( flag_PERSONAL ) && !it.has_flag( flag_AURA ) &&
        !it.has_flag( flag_SEMITANGIBLE ) ) {
        // Checks to see if the player is wearing shoes
        return ret_val<bool>::make_failure( ( is_avatar() ? _( "You're already wearing footwear!" )
                                              : string_format( _( "%s is already wearing footwear!" ), get_name() ) ) );
    }

    if( it.covers( body_part_head ) &&
        !it.has_flag( flag_HELMET_COMPAT ) && !it.has_flag( flag_SKINTIGHT ) &&
        !it.has_flag( flag_PERSONAL ) &&
        !it.has_flag( flag_AURA ) && !it.has_flag( flag_SEMITANGIBLE ) && !it.has_flag( flag_OVERSIZE ) &&
        is_wearing_helmet() ) {
        return ret_val<bool>::make_failure( wearing_something_on( body_part_head ),
                                            ( is_avatar() ? _( "You can't wear that with other headgear!" )
                                              : string_format( _( "%s can't wear that with other headgear!" ), get_name() ) ) );
    }

    if( it.covers( body_part_head ) && !it.has_flag( flag_SEMITANGIBLE ) &&
        ( it.has_flag( flag_SKINTIGHT ) || it.has_flag( flag_HELMET_COMPAT ) ) &&
        ( head_cloth_encumbrance() + it.get_encumber( *this, body_part_head ) > 40 ) ) {
        return ret_val<bool>::make_failure( ( is_avatar() ? _( "You can't wear that much on your head!" )
                                              : string_format( _( "%s can't wear that much on their head!" ), get_name() ) ) );
    }

    return ret_val<bool>::make_success();
}

cata::optional<std::list<item>::iterator>
Character::wear( int pos, bool interactive )
{
    return wear( item_location( *this, &i_at( pos ) ), interactive );
}

cata::optional<std::list<item>::iterator>
Character::wear( item_location item_wear, bool interactive )
{
    item to_wear = *item_wear;
    if( is_worn( to_wear ) ) {
        if( interactive ) {
            add_msg_player_or_npc( m_info,
                                   _( "You are already wearing that." ),
                                   _( "<npcname> is already wearing that." )
                                 );
        }
        return cata::nullopt;
    }
    if( to_wear.is_null() ) {
        if( interactive ) {
            add_msg_player_or_npc( m_info,
                                   _( "You don't have that item." ),
                                   _( "<npcname> doesn't have that item." ) );
        }
        return cata::nullopt;
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

    worn.one_per_layer_sidedness( to_wear_copy );

    auto result = wear_item( to_wear_copy, interactive );
    if( !result ) {
        if( was_weapon ) {
            weapon = to_wear_copy;
        } else {
            i_add( to_wear_copy );
        }
        return cata::nullopt;
    }

    if( was_weapon ) {
        get_event_bus().send<event_type::character_wields_item>( getID(), weapon.typeId() );
    }

    return result;
}

cata::optional<std::list<item>::iterator> outfit::wear_item( Character &guy, const item &to_wear,
        bool interactive, bool do_calc_encumbrance, bool do_sort_items, bool quiet )
{
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


    get_event_bus().send<event_type::character_wears_item>( guy.getID(), guy.last_item );

    if( interactive ) {
        if( !quiet ) {
            guy.add_msg_player_or_npc(
                _( "You put on your %s." ),
                _( "<npcname> puts on their %s." ),
                to_wear.tname() );
        }
        guy.moves -= guy.item_wear_cost( to_wear );

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
    } else if( guy.is_npc() && get_player_view().sees( guy ) && !quiet ) {
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
    }

    return new_item_it;
}

cata::optional<std::list<item>::iterator> Character::wear_item( const item &to_wear,
        bool interactive, bool do_calc_encumbrance )
{
    invalidate_inventory_validity_cache();
    const auto ret = can_wear( to_wear );
    if( !ret.success() ) {
        if( interactive ) {
            add_msg_if_player( m_info, "%s", ret.c_str() );
        }
        return cata::nullopt;
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

    for( layer_level layer : it.get_layer() )
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

    mv *= std::max( it.get_avg_encumber( *this ) / 10.0, 1.0 );

    return mv;
}

ret_val<bool> Character::can_takeoff( const item &it, const std::list<item> *res )
{
    if( !worn.is_worn( it ) ) {
        return ret_val<bool>::make_failure( !is_npc() ? _( "You are not wearing that item." ) :
                                            _( "<npcname> is not wearing that item." ) );
    }

    if( res == nullptr && !get_dependent_worn_items( it ).empty() ) {
        return ret_val<bool>::make_failure( !is_npc() ?
                                            _( "You can't take off power armor while wearing other power armor components." ) :
                                            _( "<npcname> can't take off power armor while wearing other power armor components." ) );
    }
    if( it.has_flag( flag_NO_TAKEOFF ) ) {
        return ret_val<bool>::make_failure( !is_npc() ?
                                            _( "You can't take that item off." ) :
                                            _( "<npcname> can't take that item off." ) );
    }
    return ret_val<bool>::make_success();
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

bool Character::wearing_something_on( const bodypart_id &bp ) const
{
    return worn.wearing_something_on( bp );
}

cata::optional<const item *> outfit::item_worn_with_inv_let( const char invlet ) const
{
    for( const item &i : worn ) {
        if( i.invlet == invlet ) {
            return &i;
        }
    }
    return cata::nullopt;
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

std::list<item> Character::get_visible_worn_items() const
{
    return worn.get_visible_worn_items( *this );
}

bool outfit::is_wearing_helmet() const
{
    for( const item &i : worn ) {
        if( i.covers( body_part_head ) && !i.has_flag( flag_HELMET_COMPAT ) &&
            !i.has_flag( flag_SKINTIGHT ) &&
            !i.has_flag( flag_PERSONAL ) && !i.has_flag( flag_AURA ) && !i.has_flag( flag_SEMITANGIBLE ) &&
            !i.has_flag( flag_OVERSIZE ) ) {
            return true;
        }
    }
    return false;
}

bool Character::is_wearing_helmet() const
{
    return worn.is_wearing_helmet();
}

int Character::head_cloth_encumbrance() const
{
    return worn.head_cloth_encumbrance( *this );
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

double Character::footwear_factor() const
{
    return worn.footwear_factor();
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
    const bool item_one_per_layer = it.has_flag( json_flag_ONE_PER_LAYER );
    for( const item &worn_item : worn ) {
        if( item_one_per_layer && worn_item.has_flag( json_flag_ONE_PER_LAYER ) ) {
            const cata::optional<side> sidedness_conflict = it.covers_overlaps( worn_item );
            if( sidedness_conflict ) {
                const std::string player_msg = string_format(
                                                   _( "Your %s conflicts with %s, so you cannot swap its side." ),
                                                   it.tname(), worn_item.tname() );
                const std::string npc_msg = string_format(
                                                _( "<npcname>'s %s conflicts with %s so they cannot swap its side." ),
                                                it.tname(), worn_item.tname() );
                guy.add_msg_player_or_npc( m_info, player_msg, npc_msg );
                return false;
            }
        }
    }
    return true;
}

bool Character::change_side( item &it, bool interactive )
{
    if( !it.swap_side() ) {
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

    if( interactive ) {
        add_msg_player_or_npc( m_info, _( "You swap the side on which your %s is worn." ),
                               _( "<npcname> swaps the side on which their %s is worn." ),
                               it.tname() );
    }

    mod_moves( -250 );
    calc_encumbrance();

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
                        std::map<bodypart_id, layer_level> &highest_layer_so_far, const Character &c )
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
         */
        if( it.has_flag( flag_SEMITANGIBLE ) ) {
            encumber_val = 0;
            layering_encumbrance = 0;
        }

        for( layer_level item_layer : item_layers ) {
            // do the sublayers of this armor conflict
            bool conflicts = false;

            // add the sublocations to the overall body part layer and update if we are conflicting
            if( !bp->sub_parts.empty() && item_layer >= highest_layer_so_far[bp] ) {
                conflicts = vals[bp].add_sub_locations( item_layer, it.get_covered_sub_body_parts() );
            } else {
                // the body part doesn't have sublocations it for sure conflicts
                // if its on the wrong layer it for sure conflicts
                conflicts = true;
            }

            highest_layer_so_far[bp] = std::max( highest_layer_so_far[bp], item_layer );

            // Apply layering penalty to this layer, as well as any layer worn
            // within it that would normally be worn outside of it.
            for( layer_level penalty_layer = item_layer;
                 penalty_layer <= highest_layer_so_far[bp]; ++penalty_layer ) {
                vals[bp].layer( penalty_layer, layering_encumbrance, conflicts );
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
    std::map<bodypart_id, layer_level> highest_layer_so_far;

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

std::list<item> outfit::get_visible_worn_items( const Character &guy ) const
{
    std::list<item> result;
    for( auto i = worn.cbegin(), end = worn.cend(); i != end; ++i ) {
        if( guy.is_worn_item_visible( i ) ) {
            result.push_back( *i );
        }
    }
    return result;
}

bool outfit::wearing_something_on( const bodypart_id &bp ) const
{
    for( const item &i : worn ) {
        if( i.covers( bp ) ) {
            return true;
        }
    }
    return false;
}

int outfit::head_cloth_encumbrance( const Character &guy ) const
{
    int ret = 0;
    for( const item &i : worn ) {
        const item *worn_item = &i;
        if( i.covers( body_part_head ) && !i.has_flag( flag_SEMITANGIBLE ) &&
            ( worn_item->has_flag( flag_HELMET_COMPAT ) || worn_item->has_flag( flag_SKINTIGHT ) ) ) {
            ret += worn_item->get_encumber( guy, body_part_head );
        }
    }
    return ret;
}

int outfit::swim_modifier( const int swim_skill ) const
{
    int ret = 0;
    if( swim_skill < 10 ) {
        for( const item &i : worn ) {
            ret += i.volume() / 125_ml * ( 10 - swim_skill );
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

bool outfit::natural_attack_restricted_on( const bodypart_id &bp ) const
{
    for( const item &i : worn ) {
        if( i.covers( bp ) && !i.has_flag( flag_ALLOWS_NATURAL_ATTACKS ) &&
            !i.has_flag( flag_SEMITANGIBLE ) &&
            !i.has_flag( flag_PERSONAL ) && !i.has_flag( flag_AURA ) ) {
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
            iter->on_takeoff( guy );
            result.splice( result.begin(), worn, iter++ );
        } else {
            ++iter;
        }
    }
    return result;
}

units::mass outfit::weight_capacity_bonus() const
{
    units::mass ret = 0_gram;
    for( const item &clothing : worn ) {
        ret += clothing.get_weight_capacity_bonus();
    }
    return ret;
}

float outfit::weight_capacity_modifier() const
{
    float ret = 1.0f;
    for( const item &clothing : worn ) {
        ret *= clothing.get_weight_capacity_modifier();
    }
    return ret;
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
        if( !without.count( &i ) ) {
            for( const item *j : i.all_items_ptr( item_pocket::pocket_type::CONTAINER ) ) {
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

ret_val<bool> outfit::power_armor_conflicts( const item &clothing ) const
{
    if( clothing.is_power_armor() ) {
        for( const item &elem : worn ) {
            if( elem.get_covered_body_parts().make_intersection( clothing.get_covered_body_parts() ).any() &&
                !elem.has_flag( flag_POWERARMOR_COMPATIBLE ) ) {
                return ret_val<bool>::make_failure( _( "Can't wear power armor over other gear!" ) );
            }
        }
        if( !clothing.covers( body_part_torso ) ) {
            bool power_armor = false;
            if( !worn.empty() ) {
                for( const item &elem : worn ) {
                    if( elem.is_power_armor() ) {
                        power_armor = true;
                        break;
                    }
                }
            }
            if( !power_armor ) {
                return ret_val<bool>::make_failure(
                           _( "You can only wear power armor components with power armor!" ) );
            }
        }

        for( const item &i : worn ) {
            if( i.is_power_armor() && i.typeId() == clothing.typeId() ) {
                return ret_val<bool>::make_failure( _( "Can't wear more than one %s!" ), clothing.tname() );
            }
        }
    } else {
        // Only headgear can be worn with power armor, except other power armor components.
        // You can't wear headgear if power armor helmet is already sitting on your head.
        bool has_helmet = false;
        if( !clothing.has_flag( flag_POWERARMOR_COMPATIBLE ) && ( is_wearing_power_armor( &has_helmet ) &&
                ( has_helmet || !( clothing.covers( body_part_head ) || clothing.covers( body_part_mouth ) ||
                                   clothing.covers( body_part_eyes ) ) ) ) ) {
            return ret_val<bool>::make_failure( _( "Can't wear %s with power armor!" ), clothing.tname() );
        }
    }
    return ret_val<bool>::make_success();
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

ret_val<bool> outfit::only_one_conflicts( const item &clothing ) const
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

    for( const item &i : worn ) {
        if( i.has_flag( flag_ONLY_ONE ) && i.typeId() == clothing.typeId() ) {
            return ret_val<bool>::make_failure( _( "Can't wear more than one %s!" ), clothing.tname() );
        }

        if( this_restricts_only_one || i.has_flag( json_flag_ONE_PER_LAYER ) ) {
            cata::optional<side> overlaps = clothing.covers_overlaps( i );
            if( overlaps && sidedness_conflicts( *overlaps ) ) {
                return ret_val<bool>::make_failure( _( "%1$s conflicts with %2$s!" ), clothing.tname(), i.tname() );
            }
        }
    }
    return ret_val<bool>::make_success();
}

void outfit::one_per_layer_sidedness( item &clothing ) const
{
    const bool item_one_per_layer = clothing.has_flag( json_flag_ONE_PER_LAYER );
    for( const item &worn_item : worn ) {
        const cata::optional<side> sidedness_conflict = clothing.covers_overlaps( worn_item );
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

    item takeoff_copy( it );
    worn.erase( iter );
    takeoff_copy.on_takeoff( guy );
    if( res == nullptr ) {
        guy.i_add( takeoff_copy, true, &it, &it, true, !guy.has_weapon() );
    } else {
        res->push_back( takeoff_copy );
    }
    return true;
}

double outfit::footwear_factor() const
{
    double ret = 0;
    for( const item &i : worn ) {
        if( i.covers( body_part_foot_l ) && !i.has_flag( flag_NOT_FOOTWEAR ) ) {
            ret += 0.5f;
            break;
        }
    }
    for( const item &i : worn ) {
        if( i.covers( body_part_foot_r ) && !i.has_flag( flag_NOT_FOOTWEAR ) ) {
            ret += 0.5f;
            break;
        }
    }
    return ret;
}

void outfit::damage_mitigate( const bodypart_id &bp, damage_unit &dam ) const
{
    for( const item &cloth : worn ) {
        if( cloth.get_coverage( bp ) == 100 && cloth.covers( bp ) ) {
            cloth.mitigate_damage( dam );
        }
    }
}

void outfit::clear()
{
    worn.clear();
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
        ret.push_back( &i );
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

    for( const auto &elem : worn ) {
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
        for( const item_pocket *pocket : w.get_all_contained_pockets().value() ) {
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
            for( const item_pocket *pocket : w.get_all_contained_pockets().value() ) {
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
        if( w.has_flag( flag_POCKETS ) && w.get_warmth() > warmth ) {
            warmth = w.get_warmth();
        }
    }
    return warmth;
}

int outfit::hood_warmth() const
{
    int warmth = 0;
    for( const item &w : worn ) {
        if( w.has_flag( flag_HOOD ) && w.get_warmth() > warmth ) {
            warmth = w.get_warmth();
        }
    }
    return warmth;
}

int outfit::collar_warmth() const
{
    int warmth = 0;
    for( const item &w : worn ) {
        if( w.has_flag( flag_COLLAR ) && w.get_warmth() > warmth ) {
            warmth = w.get_warmth();
        }
    }
    return warmth;
}

std::list<item> outfit::use_amount( const itype_id &it, int quantity,
                                    std::list<item> &used, const std::function<bool( const item & )> filter,
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

void outfit::append_radio_items( std::list<item *> &rc_items )
{
    for( item &elem : worn ) {
        if( elem.has_flag( flag_RADIO_ACTIVATION ) ) {
            rc_items.push_back( &elem );
        }
    }
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

bool outfit::can_pickVolume( const item &it, const bool ignore_pkt_settings ) const
{
    for( const item &w : worn ) {
        if( w.can_contain( it, false, false, ignore_pkt_settings ).success() ) {
            return true;
        }
    }
    return false;
}

void outfit::get_overlay_ids( std::vector<std::pair<std::string, std::string>> &overlay_ids ) const
{
    // TODO: worry about correct order of clothing overlays
    for( const item &worn_item : worn ) {
        if( worn_item.has_flag( json_flag_HIDDEN ) ) {
            continue;
        }
        const std::string variant = worn_item.has_itype_variant() ? worn_item.itype_variant().id : "";
        overlay_ids.emplace_back( "worn_" + worn_item.typeId().str(), variant );
    }
}

bool outfit::in_climate_control() const
{
    for( const item &w : worn ) {
        if( w.active && w.is_power_armor() ) {
            return true;
        }
        if( w.has_flag( flag_CLIMATE_CONTROL ) ) {
            return true;
        }
    }
    return false;
}

std::list<item>::iterator outfit::position_to_wear_new_item( const item &new_item )
{
    // By default we put this item on after the last item on the same or any
    // lower layer.
    return std::find_if(
               worn.rbegin(), worn.rend(),
    [&]( const item & w ) {
        return w.get_layer() <= new_item.get_layer();
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

static void item_armor_enchantment_adjust( Character &guy, damage_unit &du, item &armor )
{
    switch( du.type ) {
        case damage_type::ACID:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_ACID );
            break;
        case damage_type::BASH:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_BASH );
            break;
        case damage_type::BIOLOGICAL:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_BIO );
            break;
        case damage_type::COLD:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_COLD );
            break;
        case damage_type::CUT:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_CUT );
            break;
        case damage_type::ELECTRIC:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_ELEC );
            break;
        case damage_type::HEAT:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_HEAT );
            break;
        case damage_type::STAB:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_STAB );
            break;
        case damage_type::BULLET:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_BULLET );
            break;
        default:
            return;
    }
    du.amount = std::max( 0.0f, du.amount );
}

void outfit::absorb_damage( Character &guy, damage_unit &elem, bodypart_id bp,
                            std::list<item> &worn_remains, bool &armor_destroyed )
{
    sub_bodypart_id sbp;
    sub_bodypart_id secondary_sbp;
    // if this body part has sub part locations roll one
    if( !bp->sub_parts.empty() ) {
        sbp = bp->random_sub_part( false );
        // the torso has a second layer of hanging body parts
        if( bp == body_part_torso ) {
            secondary_sbp = bp->random_sub_part( true );
        }
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

        item_armor_enchantment_adjust( guy, elem, armor );
        // Heat damage can set armor on fire
        // Even though it doesn't cause direct physical damage to it
        if( outermost && elem.type == damage_type::HEAT && elem.amount >= 1.0f ) {
            // TODO: Different fire intensity values based on damage
            fire_data frd{ 2 };
            destroy = armor.burn( frd );
            int fuel = roll_remainder( frd.fuel_produced );
            if( fuel > 0 ) {
                guy.add_effect( effect_onfire, time_duration::from_turns( fuel + 1 ), bp, false, 0, false,
                                true );
            }
        }

        if( !destroy ) {
            // if we don't have sub parts data
            // this is the feet head and hands
            if( bp->sub_parts.empty() ) {
                destroy = guy.armor_absorb( elem, armor, bp, roll );
            } else {
                // if this armor has sublocation data test against it instead of just a generic roll
                destroy = guy.armor_absorb( elem, armor, bp, sbp, roll );
                // for the torso we also need to consider if it hits anything hanging off the character or their neck
                if( bp == body_part_torso ) {
                    destroy = guy.armor_absorb( elem, armor, bp, secondary_sbp, roll );
                }

            }
        }

        if( destroy ) {
            if( get_player_view().sees( guy ) ) {
                SCT.add( point( guy.posx(), guy.posy() ), direction::NORTH, remove_color_tags( pre_damage_name ),
                         m_neutral, _( "destroyed" ), m_info );
            }
            destroyed_armor_msg( guy, pre_damage_name );
            armor_destroyed = true;
            armor.on_takeoff( guy );
            for( const item *it : armor.all_items_top( item_pocket::pocket_type::CONTAINER ) ) {
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

float outfit::damage_resist( damage_type dt, bodypart_id bp, bool to_self ) const
{
    float ret = 0.0f;
    for( const item &i : worn ) {
        if( i.covers( bp ) ) {
            ret += i.damage_resist( dt, to_self );
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
            warmth_val = clothing.get_warmth();
            // Wool items do not lose their warmth due to being wet.
            // Warmth is reduced by 0 - 66% based on wetness.
            if( !clothing.made_of( material_wool ) ) {
                warmth_val *= 1.0 - 0.66 * wetness_pct;
            }
            total_warmth[bp] += std::round( warmth_val );
        }
        total_warmth[bp] += guy.get_effect_int( effect_heating_bionic, bp );
    }
    return total_warmth;
}

void outfit::fire_options( Character &guy, std::vector<std::string> &options,
                           std::vector<std::function<void()>> &actions )
{
    for( item &clothing : worn ) {

        std::vector<item *> guns = clothing.items_with( []( const item & it ) {
            return it.is_gun();
        } );

        if( !guns.empty() && clothing.type->can_use( "holster" ) &&
            !clothing.has_flag( flag_NO_QUICKDRAW ) ) {
            //~ draw (first) gun contained in holster
            //~ %1$s: weapon name, %2$s: container name, %3$d: remaining ammo count
            options.push_back( string_format( pgettext( "holster", "%1$s from %2$s (%3$d)" ),
                                              guns.front()->tname(),
                                              clothing.type_name(),
                                              guns.front()->ammo_remaining() ) );

            actions.emplace_back( [&] { guy.invoke_item( &clothing, "holster" ); } );

        } else if( clothing.is_gun() && clothing.gunmod_find( itype_shoulder_strap ) ) {
            // wield item currently worn using shoulder strap
            options.push_back( clothing.display_name() );
            actions.emplace_back( [&] { guy.wield( clothing ); } );
        }
    }
}

void outfit::insert_item_at_index( item clothing, int index )
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

void outfit::overflow( const tripoint &pos )
{
    for( item &clothing : worn ) {
        clothing.overflow( pos );
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

item *outfit::current_unarmed_weapon( const std::string &attack_vector, item *cur_weapon )
{
    for( item &worn_item : worn ) {
        bool covers = false;

        if( attack_vector == "HAND" || attack_vector == "GRAPPLE" || attack_vector == "THROW" ) {
            covers = worn_item.covers( bodypart_id( "hand_l" ) ) &&
                     worn_item.covers( bodypart_id( "hand_r" ) );
        } else if( attack_vector == "ARM" ) {
            covers = worn_item.covers( bodypart_id( "arm_l" ) ) &&
                     worn_item.covers( bodypart_id( "arm_r" ) );
        } else if( attack_vector == "ELBOW" ) {
            covers = worn_item.covers( sub_bodypart_id( "arm_elbow_l" ) ) &&
                     worn_item.covers( sub_bodypart_id( "arm_elbow_r" ) );
        } else if( attack_vector == "FINGERS" ) {
            covers = worn_item.covers( sub_bodypart_id( "hand_fingers_l" ) ) &&
                     worn_item.covers( sub_bodypart_id( "hand_fingers_r" ) );
        } else if( attack_vector == "WRIST" ) {
            covers = worn_item.covers( sub_bodypart_id( "hand_wrist_l" ) ) &&
                     worn_item.covers( sub_bodypart_id( "hand_wrist_r" ) );
        } else if( attack_vector == "PALM" ) {
            covers = worn_item.covers( sub_bodypart_id( "hand_palm_l" ) ) &&
                     worn_item.covers( sub_bodypart_id( "hand_palm_r" ) );
        } else if( attack_vector == "HAND_BACK" ) {
            covers = worn_item.covers( sub_bodypart_id( "hand_back_l" ) ) &&
                     worn_item.covers( sub_bodypart_id( "hand_back_r" ) );
        } else if( attack_vector == "SHOULDER" ) {
            covers = worn_item.covers( sub_bodypart_id( "arm_shoulder_l" ) ) &&
                     worn_item.covers( sub_bodypart_id( "arm_shoulder_r" ) );
        } else if( attack_vector == "FOOT" ) {
            covers = worn_item.covers( bodypart_id( "foot_l" ) ) &&
                     worn_item.covers( bodypart_id( "foot_r" ) );
        } else if( attack_vector == "LOWER_LEG" ) {
            covers = worn_item.covers( sub_bodypart_id( "leg_lower_l" ) ) &&
                     worn_item.covers( sub_bodypart_id( "leg_lower_r" ) );
        } else if( attack_vector == "KNEE" ) {
            covers = worn_item.covers( sub_bodypart_id( "leg_knee_l" ) ) &&
                     worn_item.covers( sub_bodypart_id( "leg_knee_r" ) );
        } else if( attack_vector == "HIP" ) {
            covers = worn_item.covers( sub_bodypart_id( "leg_hip_l" ) ) &&
                     worn_item.covers( sub_bodypart_id( "leg_hip_r" ) );
        } else if( attack_vector == "HEAD" ) {
            covers = worn_item.covers( bodypart_id( "head" ) );
        } else if( attack_vector == "TORSO" ) {
            covers = worn_item.covers( bodypart_id( "torso" ) );
        }

        // Uses enum layer_level to make distinction for top layer.
        if( covers ) {
            if( cur_weapon->is_null() || ( worn_item.get_layer() >= cur_weapon->get_layer() ) ) {
                cur_weapon = &worn_item;
            }
        }
    }
    return cur_weapon;
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
            // How much exposure does this item leave on this part? (1.0 == naked)
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
        if( i.can_contain_partial( newit ) ) {
            const int used_charges =
                i.fill_with( newit, remaining_charges, false, false, ignore_pkt_settings );
            remaining_charges -= used_charges;
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

cata::optional<int> outfit::get_item_position( const item &it ) const
{
    int pos = 0;
    for( const item &clothing : worn ) {
        if( clothing.has_item( it ) ) {
            return pos;
        }
        pos++;
    }
    return cata::nullopt;
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

std::string outfit::get_armor_display( bodypart_id bp, unsigned int truncate ) const
{
    for( auto it = worn.rbegin(); it != worn.rend(); ++it ) {
        if( it->covers( bp ) ) {
            return it->tname( 1, true, truncate );
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
            for( const item_pocket *pocket : i.get_all_contained_pockets().value() ) {
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
        // if the item has ripoff pockets we should itterate on them also grabs only effect the torso
        if( i.has_ripoff_pockets() ) {
            for( item_pocket *pocket : i.get_all_contained_pockets().value() ) {
                if( pocket->get_pocket_data()->ripoff > 0 && !pocket->empty() ) {
                    pd.push_back( pocket );
                }
            }
        }
    }
    return pd;
}
