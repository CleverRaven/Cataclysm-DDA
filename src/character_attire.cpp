#include "character.h"
#include "event.h"
#include "event_bus.h"
#include "flag.h"
#include "game.h"
#include "mutation.h"

static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_incorporeal( "incorporeal" );

static const trait_id trait_ANTENNAE( "ANTENNAE" );
static const trait_id trait_ANTLERS( "ANTLERS" );
static const trait_id trait_HORNS_POINTED( "HORNS_POINTED" );
static const trait_id trait_SQUEAMISH( "SQUEAMISH" );
static const trait_id trait_WOOLALLERGY( "WOOLALLERGY" );

ret_val<bool> Character::can_wear( const item &it, bool with_equip_change ) const
{
    if( has_effect( effect_incorporeal ) ) {
        return ret_val<bool>::make_failure( _( "You can't wear anything while incorporeal." ) );
    }
    if( !it.is_armor() ) {
        return ret_val<bool>::make_failure( _( "Putting on a %s would be tricky." ), it.tname() );
    }

    if( has_trait( trait_WOOLALLERGY ) && ( it.made_of( material_id( "wool" ) ) ||
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
            !it.made_of( material_id( "wool" ) ) && !it.made_of( material_id( "cotton" ) ) &&
            !it.made_of( material_id( "nomex" ) ) && !it.made_of( material_id( "leather" ) ) &&
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

    if( it.is_power_armor() ) {
        for( const item &elem : worn ) {
            if( elem.get_covered_body_parts().make_intersection( it.get_covered_body_parts() ).any() &&
                !elem.has_flag( flag_POWERARMOR_COMPATIBLE ) ) {
                return ret_val<bool>::make_failure( _( "Can't wear power armor over other gear!" ) );
            }
        }
        if( !it.covers( body_part_torso ) ) {
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
            if( i.is_power_armor() && i.typeId() == it.typeId() ) {
                return ret_val<bool>::make_failure( _( "Can't wear more than one %s!" ), it.tname() );
            }
        }
    } else {
        // Only headgear can be worn with power armor, except other power armor components.
        // You can't wear headgear if power armor helmet is already sitting on your head.
        bool has_helmet = false;
        if( !it.has_flag( flag_POWERARMOR_COMPATIBLE ) && ( ( is_wearing_power_armor( &has_helmet ) &&
                ( has_helmet || !( it.covers( body_part_head ) || it.covers( body_part_mouth ) ||
                                   it.covers( body_part_eyes ) ) ) ) ) ) {
            return ret_val<bool>::make_failure( _( "Can't wear %s with power armor!" ), it.tname() );
        }
    }

    // Check if we don't have both hands available before wearing a briefcase, shield, etc. Also occurs if we're already wearing one.
    if( it.has_flag( flag_RESTRICT_HANDS ) && ( worn_with_flag( flag_RESTRICT_HANDS ) ||
            weapon.is_two_handed( *this ) ) ) {
        return ret_val<bool>::make_failure( ( is_avatar() ? _( "You don't have a hand free to wear that." )
                                              : string_format( _( "%s doesn't have a hand free to wear that." ), get_name() ) ) );
    }

    const bool this_restricts_only_one = it.has_flag( flag_id( "ONE_PER_LAYER" ) );
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
        if( i.has_flag( flag_ONLY_ONE ) && i.typeId() == it.typeId() ) {
            return ret_val<bool>::make_failure( _( "Can't wear more than one %s!" ), it.tname() );
        }

        if( this_restricts_only_one || i.has_flag( flag_id( "ONE_PER_LAYER" ) ) ) {
            cata::optional<side> overlaps = it.covers_overlaps( i );
            if( overlaps && sidedness_conflicts( *overlaps ) ) {
                return ret_val<bool>::make_failure( _( "%1$s conflicts with %2$s!" ), it.tname(), i.tname() );
            }
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

    const bool item_one_per_layer = to_wear_copy.has_flag( flag_id( "ONE_PER_LAYER" ) );
    for( const item &worn_item : worn ) {
        const cata::optional<side> sidedness_conflict = to_wear_copy.covers_overlaps( worn_item );
        if( sidedness_conflict && ( item_one_per_layer ||
                                    worn_item.has_flag( flag_id( "ONE_PER_LAYER" ) ) ) ) {
            // we can assume both isn't an option because it'll be caught in can_wear
            if( *sidedness_conflict == side::LEFT ) {
                to_wear_copy.set_side( side::RIGHT );
            } else {
                to_wear_copy.set_side( side::LEFT );
            }
        }
    }

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

ret_val<bool> Character::can_takeoff( const item &it, const std::list<item> *res )
{
    auto iter = std::find_if( worn.begin(), worn.end(), [ &it ]( const item & wit ) {
        return &it == &wit;
    } );

    if( iter == worn.end() ) {
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


bool Character::wearing_something_on( const bodypart_id &bp ) const
{
    for( const item &i : worn ) {
        if( i.covers( bp ) ) {
            return true;
        }
    }
    return false;
}

bool Character::is_wearing_shoes( const side &check_side ) const
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

    for( const bodypart_id &part : get_all_body_parts() ) {
        // Is any right|left foot...
        if( part->limb_type != body_part_type::type::foot ) {
            continue;
        }
        for( const item &worn_item : worn ) {
            // ... wearing...
            if( !worn_item.covers( part ) ) {
                continue;
            }
            // ... a shoe?
            if( exempt( worn_item ) ) {
                continue;
            }
            any_left_foot_is_covered = part->part_side == side::LEFT ||
                                       part->part_side == side::BOTH ||
                                       any_left_foot_is_covered;
            any_right_foot_is_covered = part->part_side == side::RIGHT ||
                                        part->part_side == side::BOTH ||
                                        any_right_foot_is_covered;
        }
    }
    if( !any_left_foot_is_covered && ( check_side == side::LEFT || check_side == side::BOTH ) ) {
        return false;
    }
    if( !any_right_foot_is_covered && ( check_side == side::RIGHT || check_side == side::BOTH ) ) {
        return false;
    }
    return true;
}

bool Character::is_worn_item_visible( std::list<item>::const_iterator worn_item ) const
{
    const body_part_set worn_item_body_parts = worn_item->get_covered_body_parts();
    return std::any_of( worn_item_body_parts.begin(), worn_item_body_parts.end(),
    [this, &worn_item]( const bodypart_str_id & bp ) {
        // no need to check items that are worn under worn_item in the armor sort order
        for( auto i = std::next( worn_item ), end = worn.end(); i != end; ++i ) {
            if( i->covers( bp ) && i->get_layer() != layer_level::BELTED &&
                i->get_layer() != layer_level::WAIST &&
                i->get_coverage( bp ) >= worn_item->get_coverage( bp ) ) {
                return false;
            }
        }
        return true;
    }
                      );
}

std::list<item> Character::get_visible_worn_items() const
{
    std::list<item> result;
    for( auto i = worn.cbegin(), end = worn.cend(); i != end; ++i ) {
        if( is_worn_item_visible( i ) ) {
            result.push_back( *i );
        }
    }
    return result;
}

bool Character::is_wearing_helmet() const
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

int Character::head_cloth_encumbrance() const
{
    int ret = 0;
    for( const item &i : worn ) {
        const item *worn_item = &i;
        if( i.covers( body_part_head ) && !i.has_flag( flag_SEMITANGIBLE ) &&
            ( worn_item->has_flag( flag_HELMET_COMPAT ) || worn_item->has_flag( flag_SKINTIGHT ) ) ) {
            ret += worn_item->get_encumber( *this, body_part_head );
        }
    }
    return ret;
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

    const bool item_one_per_layer = it.has_flag( flag_id( "ONE_PER_LAYER" ) );
    for( const item &worn_item : worn ) {
        if( item_one_per_layer && worn_item.has_flag( flag_id( "ONE_PER_LAYER" ) ) ) {
            const cata::optional<side> sidedness_conflict = it.covers_overlaps( worn_item );
            if( sidedness_conflict ) {
                const std::string player_msg = string_format(
                                                   _( "Your %s conflicts with %s, so you cannot swap its side." ),
                                                   it.tname(), worn_item.tname() );
                const std::string npc_msg = string_format(
                                                _( "<npcname>'s %s conflicts with %s so they cannot swap its side." ),
                                                it.tname(), worn_item.tname() );
                add_msg_player_or_npc( m_info, player_msg, npc_msg );
                return false;
            }
        }
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
    bool result = false;
    for( const item &elem : worn ) {
        if( !elem.is_power_armor() ) {
            continue;
        }
        if( hasHelmet == nullptr ) {
            // found power armor, helmet not requested, cancel loop
            return true;
        }
        // found power armor, continue search for helmet
        result = true;
        if( elem.covers( body_part_head ) ) {
            *hasHelmet = true;
            return true;
        }
    }
    return result;
}

bool Character::is_wearing_active_power_armor() const
{
    for( const item &w : worn ) {
        if( w.is_power_armor() && w.active ) {
            return true;
        }
    }
    return false;
}

bool Character::is_wearing_active_optcloak() const
{
    for( const item &w : worn ) {
        if( w.active && w.has_flag( flag_ACTIVE_CLOAKING ) ) {
            return true;
        }
    }
    return false;
}

static void layer_item( std::map<bodypart_id, encumbrance_data> &vals, const item &it,
                        std::map<bodypart_id, layer_level> &highest_layer_so_far, bool power_armor, const Character &c )
{
    body_part_set covered_parts = it.get_covered_body_parts();
    for( const bodypart_id &bp : c.get_all_body_parts() ) {
        if( !covered_parts.test( bp.id() ) ) {
            continue;
        }

        const layer_level item_layer = it.get_layer();
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

        const int armorenc = !power_armor || !it.is_power_armor() ?
                             encumber_val : std::max( 0, encumber_val - 40 );

        highest_layer_so_far[bp] = std::max( highest_layer_so_far[bp], item_layer );

        // Apply layering penalty to this layer, as well as any layer worn
        // within it that would normally be worn outside of it.
        for( layer_level penalty_layer = item_layer;
             penalty_layer <= highest_layer_so_far[bp]; ++penalty_layer ) {
            vals[bp].layer( penalty_layer, layering_encumbrance );
        }

        vals[bp].armor_encumbrance += armorenc;
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
void Character::item_encumb( std::map<bodypart_id, encumbrance_data> &vals,
                             const item &new_item ) const
{

    // reset all layer data
    vals = std::map<bodypart_id, encumbrance_data>();

    // Figure out where new_item would be worn
    std::list<item>::const_iterator new_item_position = worn.end();
    if( !new_item.is_null() ) {
        // const_cast required to work around g++-4.8 library bug
        // see the commit that added this comment to understand why
        new_item_position =
            const_cast<Character *>( this )->position_to_wear_new_item( new_item );
    }

    // Track highest layer observed so far so we can penalize out-of-order
    // items
    std::map<bodypart_id, layer_level> highest_layer_so_far;

    const bool power_armored = is_wearing_active_power_armor();
    for( auto w_it = worn.begin(); w_it != worn.end(); ++w_it ) {
        if( w_it == new_item_position ) {
            layer_item( vals, new_item, highest_layer_so_far, power_armored, *this );
        }
        layer_item( vals, *w_it, highest_layer_so_far, power_armored, *this );
    }

    if( worn.end() == new_item_position && !new_item.is_null() ) {
        layer_item( vals, new_item, highest_layer_so_far, power_armored, *this );
    }

    // make sure values are sane
    for( const bodypart_id &bp : get_all_body_parts() ) {
        encumbrance_data &elem = vals[bp];

        elem.armor_encumbrance = std::max( 0, elem.armor_encumbrance );

        // Add armor and layering penalties for the final values
        elem.encumbrance += elem.armor_encumbrance + elem.layer_penalty;
    }
}

