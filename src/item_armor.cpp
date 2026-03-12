#include "item.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "body_part_set.h"
#include "bodypart.h"
#include "character.h"
#include "character_id.h"
#include "clothing_mod.h"
#include "creature.h"
#include "damage.h"
#include "debug.h"
#include "enum_traits.h"
#include "enums.h"
#include "fault.h"
#include "flag.h"
#include "item_contents.h"
#include "item_pocket.h"
#include "itype.h"
#include "material.h"
#include "subbodypart.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"

static const std::string CLOTHING_MOD_VAR_PREFIX( "clothing_mod_" );
static const std::string var_lateral( "lateral" );

static const damage_type_id damage_acid( "acid" );
static const damage_type_id damage_bash( "bash" );
static const damage_type_id damage_bullet( "bullet" );
static const damage_type_id damage_cut( "cut" );
static const damage_type_id damage_heat( "heat" );

static const efftype_id effect_bleed( "bleed" );

static const sub_bodypart_str_id sub_body_part_torso_lower( "torso_lower" );
static const sub_bodypart_str_id sub_body_part_torso_upper( "torso_upper" );

bool item::covers( const sub_bodypart_id &bp, bool check_ablative_armor ) const
{
    // if the item has no armor data it doesn't cover that part
    const islot_armor *armor = find_armor_data();
    if( armor == nullptr ) {
        return false;
    }

    bool has_sub_data = false;

    // If the item has no sub location info and covers the part's parent,
    // then assume that the item covers that sub body part.
    for( const armor_portion_data &data : armor->sub_data ) {
        if( !data.sub_coverage.empty() ) {
            has_sub_data = true;
        }
    }

    if( !has_sub_data ) {
        return covers( bp->parent );
    }

    bool does_cover = false;
    bool subpart_cover = false;

    iterate_covered_sub_body_parts_internal( get_side(), [&]( const sub_bodypart_str_id & covered ) {
        does_cover = does_cover || bp == covered;
    }, check_ablative_armor );

    return does_cover || subpart_cover;
}

bool item::covers( const bodypart_id &bp, bool check_ablative_armor ) const
{
    bool does_cover = false;
    bool subpart_cover = false;
    iterate_covered_body_parts_internal( get_side(), [&]( const bodypart_str_id & covered ) {
        does_cover = does_cover || bp == covered;
    }, check_ablative_armor );

    return does_cover || subpart_cover;
}

std::optional<side> item::covers_overlaps( const item &rhs ) const
{
    if( get_layer() != rhs.get_layer() ) {
        return std::nullopt;
    }
    const islot_armor *armor = find_armor_data();
    if( armor == nullptr ) {
        return std::nullopt;
    }
    const islot_armor *rhs_armor = rhs.find_armor_data();
    if( rhs_armor == nullptr ) {
        return std::nullopt;
    }
    body_part_set this_covers;
    for( const armor_portion_data &data : armor->data ) {
        if( data.covers.has_value() ) {
            this_covers.unify_set( *data.covers );
        }
    }
    body_part_set rhs_covers;
    for( const armor_portion_data &data : rhs_armor->data ) {
        if( data.covers.has_value() ) {
            rhs_covers.unify_set( *data.covers );
        }
    }
    if( this_covers.intersect_set( rhs_covers ).any() ) {
        return rhs.get_side();
    } else {
        return std::nullopt;
    }
}

std::vector<sub_bodypart_id> item::get_covered_sub_body_parts() const
{
    return get_covered_sub_body_parts( get_side() );
}

std::vector<sub_bodypart_id> item::get_covered_sub_body_parts( const side s ) const
{
    std::vector<sub_bodypart_id> res;
    iterate_covered_sub_body_parts_internal( s, [&]( const sub_bodypart_id & bp ) {
        res.push_back( bp );
    } );
    return res;
}

body_part_set item::get_covered_body_parts() const
{
    return get_covered_body_parts( get_side() );
}

body_part_set item::get_covered_body_parts( const side s ) const
{
    body_part_set res;
    iterate_covered_body_parts_internal( s, [&]( const bodypart_str_id & bp ) {
        res.set( bp );
    } );
    return res;
}

namespace
{

const std::array<bodypart_str_id, 4> &left_side_parts()
{
    static const std::array<bodypart_str_id, 4> result{ {
            body_part_arm_l,
            body_part_hand_l,
            body_part_leg_l,
            body_part_foot_l
        } };
    return result;
}

const std::array<bodypart_str_id, 4> &right_side_parts()
{
    static const std::array<bodypart_str_id, 4> result{ {
            body_part_arm_r,
            body_part_hand_r,
            body_part_leg_r,
            body_part_foot_r
        } };
    return result;
}

} // namespace

static void iterate_helper_sbp( const item *i, const side s,
                                const std::function<void( const sub_bodypart_str_id & )> &cb )
{
    const islot_armor *armor = i->find_armor_data();
    if( armor == nullptr ) {
        return;
    }

    for( const armor_portion_data &data : armor->sub_data ) {
        if( !data.sub_coverage.empty() ) {
            if( !armor->sided || s == side::BOTH || s == side::num_sides ) {
                for( const sub_bodypart_str_id &bpid : data.sub_coverage ) {
                    cb( bpid );
                }
                continue;
            }
            for( const sub_bodypart_str_id &bpid : data.sub_coverage ) {
                if( bpid->part_side == s || bpid->part_side == side::BOTH ) {
                    cb( bpid );
                }
            }
        }
    }
}

void item::iterate_covered_sub_body_parts_internal( const side s,
        const std::function<void( const sub_bodypart_str_id & )> &cb,
        bool check_ablative_armor ) const

{
    iterate_helper_sbp( this, s, cb );

    //check for ablative armor too
    if( check_ablative_armor && is_ablative() ) {
        for( const item_pocket *pocket : get_ablative_pockets() ) {
            if( !pocket->empty() ) {
                // get the contained plate
                const item &ablative_armor = pocket->front();

                iterate_helper_sbp( &ablative_armor, s, cb );
            }
        }
    }
}

static void iterate_helper( const item *i, const side s,
                            const std::function<void( const bodypart_str_id & )> &cb )
{
    const islot_armor *armor = i->find_armor_data();
    if( armor == nullptr ) {
        return;
    }

    // If we are querying for only one side of the body, we ignore coverage
    // for parts on the opposite side of the body.
    const auto &opposite_side_parts = s == side::LEFT ? right_side_parts() : left_side_parts();

    for( const armor_portion_data &data : armor->data ) {
        if( data.covers.has_value() ) {
            if( !armor->sided || s == side::BOTH || s == side::num_sides ) {
                for( const bodypart_str_id &bpid : data.covers.value() ) {
                    cb( bpid );
                }
                continue;
            }
            for( const bodypart_str_id &bpid : data.covers.value() ) {
                if( std::find( opposite_side_parts.begin(), opposite_side_parts.end(),
                               bpid ) == opposite_side_parts.end() ) {
                    cb( bpid );
                }
            }
        }
    }
}

void item::iterate_covered_body_parts_internal( const side s,
        const std::function<void( const bodypart_str_id & )> &cb,
        bool check_ablative_armor ) const
{
    iterate_helper( this, s, cb );

    //check for ablative armor too
    if( check_ablative_armor && is_ablative() ) {
        for( const item_pocket *pocket : get_ablative_pockets() ) {
            if( !pocket->empty() ) {
                // get the contained plate
                const item &ablative_armor = pocket->front();

                iterate_helper( &ablative_armor, s, cb );
            }
        }
    }
}

bool item::is_sided() const
{
    const islot_armor *t = find_armor_data();
    return t ? t->sided : false;
}

side item::get_side() const
{
    // MSVC complains if directly cast double to enum
    return static_cast<side>(
               static_cast<int>(
                   get_var( var_lateral, static_cast<int>( side::BOTH ) )
               )
           );
}

bool item::set_side( side s )
{
    if( !is_sided() ) {
        return false;
    }

    if( s == side::BOTH ) {
        erase_var( var_lateral );
    } else {
        set_var( var_lateral, static_cast<int>( s ) );
    }

    return true;
}

bool item::swap_side()
{
    return set_side( opposite_side( get_side() ) );
}

bool item::is_ablative() const
{
    const islot_armor *t = find_armor_data();
    return t ? t->ablative : false;
}

bool item::has_additional_encumbrance() const
{
    const islot_armor *t = find_armor_data();
    return t ? t->additional_pocket_enc : false;
}

bool item::has_ripoff_pockets() const
{
    const islot_armor *t = find_armor_data();
    return t ? t->ripoff_chance : false;
}

bool item::has_noisy_pockets() const
{
    const islot_armor *t = find_armor_data();
    return t ? t->noisy : false;
}

bool item::is_worn_only_with( const item &it ) const
{
    return is_power_armor() && it.is_power_armor() && it.covers( bodypart_id( "torso" ) );
}
bool item::is_worn_by_player() const
{
    return get_player_character().is_worn( *this );
}

item::sizing item::get_sizing( const Character &p ) const
{
    const islot_armor *armor_data = find_armor_data();
    if( !armor_data ) {
        return sizing::ignore;
    }
    bool to_ignore = true;
    for( const armor_portion_data &piece : armor_data->data ) {
        if( piece.encumber != 0 ) {
            to_ignore = false;
        }
    }
    if( to_ignore ) {
        return sizing::ignore;
    } else {
        const bool small = p.get_size() == creature_size::tiny;

        const bool big = p.get_size() == creature_size::huge;

        if( has_flag( flag_INTEGRATED ) ) {
            if( big ) {
                return sizing::big_sized_big_char;
            } else if( small ) {
                return sizing::small_sized_small_char;
            } else {
                return sizing::human_sized_human_char;
            }
        }

        if( has_flag( flag_MORPHIC ) ) {
            if( big ) {
                return sizing::big_sized_big_char;
            } else if( small ) {
                return sizing::small_sized_small_char;
            } else {
                return sizing::human_sized_human_char;
            }
        }

        // due to the iterative nature of these features, something can fit and be undersized/oversized
        // but that is fine because we have separate logic to adjust encumbrance per each. One day we
        // may want to have fit be a flag that only applies if a piece of clothing is sized for you as there
        // is a bit of cognitive dissonance when something 'fits' and is 'oversized' and the same time
        const bool undersize = has_flag( flag_UNDERSIZE );
        const bool oversize = has_flag( flag_OVERSIZE );

        if( undersize ) {
            if( small ) {
                return sizing::small_sized_small_char;
            } else if( big ) {
                return sizing::small_sized_big_char;
            } else {
                return sizing::small_sized_human_char;
            }
        } else if( oversize ) {
            if( big ) {
                return sizing::big_sized_big_char;
            } else if( small ) {
                return sizing::big_sized_small_char;
            } else {
                return sizing::big_sized_human_char;
            }
        } else {
            if( big ) {
                return sizing::human_sized_big_char;
            } else if( small ) {
                return sizing::human_sized_small_char;
            } else {
                return sizing::human_sized_human_char;
            }
        }
    }
}

void item::on_wear( Character &p )
{
    if( is_sided() && get_side() == side::BOTH ) {
        if( has_flag( flag_SPLINT ) ) {
            set_side( side::LEFT );
            if( ( covers( bodypart_id( "leg_l" ) ) && p.is_limb_broken( bodypart_id( "leg_r" ) ) &&
                  !p.worn_with_flag( flag_SPLINT, bodypart_id( "leg_r" ) ) ) ||
                ( covers( bodypart_id( "arm_l" ) ) && p.is_limb_broken( bodypart_id( "arm_r" ) ) &&
                  !p.worn_with_flag( flag_SPLINT, bodypart_id( "arm_r" ) ) ) ) {
                set_side( side::RIGHT );
            }
        } else if( has_flag( flag_TOURNIQUET ) ) {
            set_side( side::LEFT );
            if( ( covers( bodypart_id( "leg_l" ) ) &&
                  p.has_effect( effect_bleed, body_part_leg_r ) &&
                  !p.worn_with_flag( flag_TOURNIQUET, bodypart_id( "leg_r" ) ) ) ||
                ( covers( bodypart_id( "arm_l" ) ) && p.has_effect( effect_bleed, body_part_arm_r ) &&
                  !p.worn_with_flag( flag_TOURNIQUET, bodypart_id( "arm_r" ) ) ) ) {
                set_side( side::RIGHT );
            }
        } else {
            // for sided items wear the item on the side which results in least encumbrance
            int lhs = 0;
            int rhs = 0;
            set_side( side::LEFT );
            for( const bodypart_id &bp : p.get_all_body_parts() ) {
                lhs += p.get_part_encumbrance( bp );
            }

            set_side( side::RIGHT );
            for( const bodypart_id &bp : p.get_all_body_parts() ) {
                rhs += p.get_part_encumbrance( bp );
            }

            set_side( lhs <= rhs ? side::LEFT : side::RIGHT );
        }
    }

    // if game is loaded - don't want ownership assigned during char creation
    if( get_player_character().getID().is_valid() ) {
        handle_pickup_ownership( p );
    }
    p.on_item_acquire( *this );
    p.on_item_wear( *this );
}

void item::on_takeoff( Character &p )
{
    p.on_item_takeoff( *this );

    if( is_sided() ) {
        set_side( side::BOTH );
    }
    // reset any pockets to not have restrictions
    if( is_ablative() ) {
        for( item_pocket *pocket : get_standard_pockets() ) {
            pocket->set_no_rigid( {} );
        }
    }
}

bool item::is_power_armor() const
{
    const islot_armor *t = find_armor_data();
    if( t == nullptr ) {
        return is_pet_armor() ? type->pet_armor->power_armor : false;
    }
    return t->power_armor;
}

int item::max_worn() const
{
    return type->max_worn;
}

int item::get_avg_encumber( const Character &p, encumber_flags flags ) const
{
    const islot_armor *t = find_armor_data();
    if( !t ) {
        return 0;
    }

    int avg_encumber = 0;
    int avg_ctr = 0;

    for( const armor_portion_data &entry : t->data ) {
        if( entry.covers.has_value() ) {
            for( const bodypart_str_id &limb : entry.covers.value() ) {
                int encumber = get_encumber( p, bodypart_id( limb ), flags );
                if( encumber ) {
                    avg_encumber += encumber;
                    ++avg_ctr;
                }
            }
        }
    }
    if( avg_encumber == 0 ) {
        return 0;
    } else {
        avg_encumber /= avg_ctr;
        return avg_encumber;
    }
}

int item::get_encumber( const Character &p, const bodypart_id &bodypart,
                        encumber_flags flags ) const
{
    const islot_armor *t = find_armor_data();
    int encumber = 0;
    if( !t ) {
        return 0;
    }

    float relative_encumbrance;

    if( flags & encumber_flags::assume_full ) {
        relative_encumbrance = 1.0f;
    } else if( flags & encumber_flags::assume_empty ) {
        relative_encumbrance = 0.0f;
    } else {
        // Additional encumbrance from non-rigid pockets

        // p.get_check_encumbrance() may be set when it's not possible
        // to reset `cached_relative_encumbrance` for individual items
        // (e.g. when dropping via AIM, see #42983)
        if( !cached_relative_encumbrance || p.get_check_encumbrance() ) {
            cached_relative_encumbrance = contents.relative_encumbrance();
        }
        relative_encumbrance = *cached_relative_encumbrance;
    }

    if( const armor_portion_data *portion_data = portion_for_bodypart( bodypart ) ) {
        encumber = portion_data->encumber;
        if( is_gun() ) {
            encumber += volume() * portion_data->volume_encumber_modifier /
                        armor_portion_data::volume_per_encumbrance;
        }

        // encumbrance from added or modified pockets
        int additional_encumbrance = get_contents().get_additional_pocket_encumbrance(
                                         portion_data->volume_encumber_modifier );
        encumber += std::ceil( relative_encumbrance * ( portion_data->max_encumber +
                               additional_encumbrance - portion_data->encumber ) );

        // add the encumbrance values of any ablative plates and additional encumbrance pockets
        if( has_additional_encumbrance() ) {
            for( const item_pocket *pocket : contents.get_container_pockets() ) {
                if( pocket->get_pocket_data()->extra_encumbrance > 0 && !pocket->empty() ) {
                    encumber += pocket->get_pocket_data()->extra_encumbrance;
                }
            }
        }
    }

    // even if we don't have data we might have ablative armor draped over it
    if( is_ablative() ) {
        for( const item_pocket *pocket : contents.get_ablative_pockets() ) {
            if( !pocket->empty() ) {
                // get the contained plate
                const item &ablative_armor = pocket->front();
                encumber += ablative_armor.get_encumber( p, bodypart, flags );
            }
        }
    }

    // Fit checked before changes, fitting shouldn't reduce penalties from patching.
    if( has_flag( flag_FIT ) && has_flag( flag_VARSIZE ) ) {
        encumber = std::max( encumber / 2, encumber - 10 );
    }

    // TODO: Should probably have sizing affect coverage
    const sizing sizing_level = get_sizing( p );
    switch( sizing_level ) {
        case sizing::small_sized_human_char:
        case sizing::small_sized_big_char:
            // non small characters have a HARD time wearing undersized clothing
            encumber *= 3;
            break;
        case sizing::human_sized_small_char:
        case sizing::big_sized_small_char:
            // clothes bag up around smol characters and encumber them more
            encumber *= 2;
            break;
        default:
            break;
    }

    encumber += static_cast<int>( std::ceil( get_clothing_mod_val( clothing_mod_type_encumbrance ) ) );

    if( !faults.empty() ) {
        int add = 0;
        float mult = 1.f;
        for( const fault_id fault : faults ) {
            add += fault->encumb_mod_flat();
            mult *= fault->encumb_mod_mult();
        }
        encumber = std::max( 0.f, ( encumber + add ) * mult );
    }

    return encumber;
}

std::vector<layer_level> item::get_layer() const
{
    const islot_armor *armor = find_armor_data();
    if( armor == nullptr ) {
        return std::vector<layer_level>();
    }
    return armor->all_layers;
}

static void get_layer_helper( const item *i, bodypart_id bp, std::set<layer_level> &layers )
{
    const islot_armor *t = i->find_armor_data();
    if( t == nullptr ) {
        return;
    }

    for( const armor_portion_data &data : t->data ) {
        if( !data.covers.has_value() ) {
            continue;
        }
        for( const bodypart_str_id &bpid : data.covers.value() ) {
            if( bp == bpid ) {
                layers.insert( data.layers.begin(), data.layers.end() );
            }
        }
    }
}

std::vector<layer_level> item::get_layer( bodypart_id bp ) const
{

    std::set<layer_level> layers;

    get_layer_helper( this, bp, layers );

    //get layers for ablative armor too
    if( is_ablative() ) {
        for( const item_pocket *pocket : get_ablative_pockets() ) {
            if( !pocket->empty() ) {
                // get the contained plate
                const item &ablative_armor = pocket->front();
                get_layer_helper( &ablative_armor, bp, layers );
            }
        }
    }

    // if the item has additional pockets and is on the torso it should also be strapped
    if( bp == body_part_torso && contents.has_additional_pockets() ) {
        layers.insert( layer_level::BELTED );
    }

    return std::vector<layer_level>( layers.begin(), layers.end() );
}

std::vector<layer_level> item::get_layer( sub_bodypart_id sbp ) const
{
    const islot_armor *t = find_armor_data();
    if( t == nullptr ) {
        return std::vector<layer_level>();
    }

    for( const armor_portion_data &data : t->sub_data ) {
        for( const sub_bodypart_str_id &bpid : data.sub_coverage ) {
            if( sbp == bpid ) {
                // if the item has additional pockets and is on the torso it should also be strapped
                if( ( sbp == sub_body_part_torso_upper || sbp == sub_body_part_torso_lower ) &&
                    contents.has_additional_pockets() ) {
                    std::set<layer_level> with_belted = data.layers;
                    with_belted.insert( layer_level::BELTED );
                    return std::vector<layer_level>( with_belted.begin(), with_belted.end() );
                } else {
                    return std::vector<layer_level>( data.layers.begin(), data.layers.end() );
                }
            }
        }
    }
    // body part not covered by this armour
    return std::vector<layer_level>();
}

bool item::has_layer( const std::vector<layer_level> &ll ) const
{
    const islot_armor *t = find_armor_data();
    if( t == nullptr ) {
        return false;
    }
    for( const layer_level &test_level : ll ) {
        if( std::find( t->all_layers.begin(), t->all_layers.end(), test_level ) != t->all_layers.end() ) {
            return true;
        }
    }
    return false;
}

bool item::has_layer( const std::vector<layer_level> &ll, const bodypart_id &bp ) const
{
    const std::vector<layer_level> layers = get_layer( bp );
    for( const layer_level &test_level : ll ) {
        if( std::find( layers.begin(), layers.end(), test_level ) != layers.end() ) {
            return true;
        }
    }
    return false;
}

bool item::has_layer( const std::vector<layer_level> &ll, const sub_bodypart_id &sbp ) const
{
    const std::vector<layer_level> layers = get_layer( sbp );
    for( const layer_level &test_level : ll ) {
        if( std::find( layers.begin(), layers.end(), test_level ) != layers.end() ) {
            return true;
        }
    }
    return false;
}

layer_level item::get_highest_layer( const sub_bodypart_id &sbp ) const
{
    layer_level highest_layer = layer_level::PERSONAL;

    // find highest layer from this item
    for( layer_level our_layer : get_layer( sbp ) ) {
        if( our_layer > highest_layer ) {
            highest_layer = our_layer;
        }
    }

    return highest_layer;
}

item::cover_type item::get_cover_type( const damage_type_id &type )
{
    item::cover_type ctype = item::cover_type::COVER_DEFAULT;
    if( type->physical ) {
        if( type->melee_only ) {
            ctype = item::cover_type::COVER_MELEE;
        } else {
            ctype = item::cover_type::COVER_RANGED;
        }
    }
    return ctype;
}

int item::get_avg_coverage( const cover_type &type ) const
{
    const islot_armor *t = find_armor_data();
    if( !t ) {
        return 0;
    }
    int avg_coverage = 0;
    int avg_ctr = 0;
    for( const armor_portion_data &entry : t->data ) {
        if( entry.covers.has_value() ) {
            for( const bodypart_str_id &limb : entry.covers.value() ) {
                int coverage = get_coverage( limb, type );
                if( coverage ) {
                    avg_coverage += coverage;
                    ++avg_ctr;
                }
            }
        }
    }
    if( avg_coverage == 0 ) {
        return 0;
    } else {
        avg_coverage /= avg_ctr;
        return avg_coverage;
    }
}

int item::get_coverage( const bodypart_id &bodypart, const cover_type &type ) const
{
    int coverage = 0;
    if( const armor_portion_data *portion_data = portion_for_bodypart( bodypart ) ) {
        switch( type ) {
            case cover_type::COVER_DEFAULT: {
                coverage = portion_data->coverage;
                break;
            }
            case cover_type::COVER_MELEE: {
                coverage = portion_data->cover_melee;
                break;
            }
            case cover_type::COVER_RANGED: {
                coverage = portion_data->cover_ranged;
                break;
            }
            case cover_type::COVER_VITALS: {
                coverage = portion_data->cover_vitals;
                break;
            }
        }
    }
    if( is_ablative() ) {
        for( const item_pocket *pocket : contents.get_ablative_pockets() ) {
            if( !pocket->empty() ) {
                // get the contained plate
                const item &ablative_armor = pocket->front();
                //Rationale for this kind of aggregation is that ablative armor
                //can't be bigger than an armor that contains it. But it may cover
                //new body parts, e.g. face shield attached to hard hat.
                coverage = std::max( coverage, ablative_armor.get_coverage( bodypart ) );
            }
        }
    }
    return coverage;
}

int item::get_coverage( const sub_bodypart_id &bodypart, const cover_type &type ) const
{
    int coverage = 0;
    if( const armor_portion_data *portion_data = portion_for_bodypart( bodypart ) ) {
        switch( type ) {
            case cover_type::COVER_DEFAULT: {
                coverage = portion_data->coverage;
                break;
            }
            case cover_type::COVER_MELEE: {
                coverage = portion_data->cover_melee;
                break;
            }
            case cover_type::COVER_RANGED: {
                coverage = portion_data->cover_ranged;
                break;
            }
            case cover_type::COVER_VITALS: {
                coverage = portion_data->cover_vitals;
                break;
            }
        }
    }
    if( is_ablative() ) {
        for( const item_pocket *pocket : contents.get_ablative_pockets() ) {
            if( !pocket->empty() ) {
                // get the contained plate
                const item &ablative_armor = pocket->front();
                coverage = std::max( coverage, ablative_armor.get_coverage( bodypart ) );
            }
        }
    }
    return coverage;
}

bool item::has_sublocations() const
{
    const islot_armor *t = find_armor_data();
    if( !t ) {
        return false;
    }
    return t->has_sub_coverage;
}

const armor_portion_data *item::portion_for_bodypart( const bodypart_id &bodypart ) const
{
    const islot_armor *t = find_armor_data();
    if( !t ) {
        return nullptr;
    }
    for( const armor_portion_data &entry : t->data ) {
        if( entry.covers.has_value() && entry.covers->test( bodypart.id() ) ) {
            return &entry;
        }
    }
    return nullptr;
}

const armor_portion_data *item::portion_for_bodypart( const sub_bodypart_id &bodypart ) const
{
    const islot_armor *t = find_armor_data();
    if( !t ) {
        return nullptr;
    }
    for( const armor_portion_data &entry : t->sub_data ) {
        for( const sub_bodypart_str_id &tmp : entry.sub_coverage ) {
            const sub_bodypart_id &subpart = tmp;
            if( subpart == bodypart ) {
                return &entry;
            }
        }
    }
    return nullptr;
}

float item::get_thickness() const
{
    const islot_armor *t = find_armor_data();
    // if an item isn't armor we assume it is 1mm thick
    // TODO: Estimate normal items thickness or protection based on weight and density
    if( t == nullptr ) {
        return is_pet_armor() ? type->pet_armor->thickness : 1.0f;
    }
    return t->avg_thickness();
}

float item::get_thickness( const bodypart_id &bp ) const
{
    const islot_armor *t = find_armor_data();
    // don't return a fixed value for this one since an item is never going to be covering a body part
    if( t == nullptr ) {
        return is_pet_armor() ? type->pet_armor->thickness : 0.0f;
    }
    float avg_thickness = 0.0f;

    for( const armor_portion_data &data : t->data ) {
        if( !data.covers.has_value() ) {
            continue;
        }
        for( const bodypart_str_id &bpid : data.covers.value() ) {
            if( bp == bpid ) {
                avg_thickness = data.avg_thickness;
                break;
            }
        }
        if( avg_thickness > 0.0f ) {
            break;
        }
    }
    if( is_ablative() ) {
        int ablatives = 0;
        float ablative_thickness = 0.0;
        for( const item_pocket *pocket : contents.get_ablative_pockets() ) {
            if( !pocket->empty() ) {
                // get the contained plate
                const item &ablative_armor = pocket->front();
                float tmp_thickness = ablative_armor.get_thickness( bp );
                if( tmp_thickness > 0.0f ) {
                    ablative_thickness += tmp_thickness;
                    ablatives += 1;
                }
            }
        }
        if( ablatives ) {
            avg_thickness += ablative_thickness / ablatives;
        }
    }
    return avg_thickness;
}

float item::get_thickness( const sub_bodypart_id &bp ) const
{
    return get_thickness( bp->parent );
}

int item::get_warmth() const
{
    const islot_armor *t = find_armor_data();
    if( t == nullptr ) {
        return 0;
    }
    int result = t->warmth;

    result += get_clothing_mod_val( clothing_mod_type_warmth );

    return result;
}

int item::get_warmth( const bodypart_id &bp ) const
{
    double warmth_val = 0.0;
    float limb_coverage = 0.0f;
    bool check_ablative_armor = false;
    if( !covers( bp, check_ablative_armor ) ) {
        return 0;
    }
    warmth_val = get_warmth();
    // calculate how much of the limb the armor ideally covers
    // idea being that an item that covers the shoulders and torso shouldn't
    // heat the whole arm like it covers it
    // TODO: fully configure this per armor entry
    if( !has_sublocations() ) {
        // if it doesn't have sublocations it has 100% covered
        limb_coverage = 100;
    } else {
        for( const sub_bodypart_str_id &sbp : bp->sub_parts ) {
            if( !covers( sbp, check_ablative_armor ) ) {
                continue;
            }

            // TODO: handle non 100% sub body part coverages
            limb_coverage += sbp->max_coverage;
        }
    }
    int warmth = std::round( warmth_val * limb_coverage / 100.0f );

    if( is_ablative() ) {
        for( const item_pocket *pocket : contents.get_ablative_pockets() ) {
            if( !pocket->empty() ) {
                // get the contained plate
                const item &ablative_armor = pocket->front();
                warmth += ablative_armor.get_warmth( bp );
            }
        }
    }

    return warmth;
}

units::volume item::get_pet_armor_max_vol() const
{
    return is_pet_armor() ? type->pet_armor->max_vol : 0_ml;
}

units::volume item::get_pet_armor_min_vol() const
{
    return is_pet_armor() ? type->pet_armor->min_vol : 0_ml;
}

std::string item::get_pet_armor_bodytype() const
{
    return is_pet_armor() ? type->pet_armor->bodytype : "";
}

float item::get_clothing_mod_val_for_damage_type( const damage_type_id &dmg_type ) const
{
    // FIXME: Hardcoded damage types
    if( dmg_type == damage_bash ) {
        return get_clothing_mod_val( clothing_mod_type_bash );
    } else if( dmg_type == damage_cut ) {
        return get_clothing_mod_val( clothing_mod_type_cut );
    } else if( dmg_type == damage_bullet ) {
        return get_clothing_mod_val( clothing_mod_type_bullet );
    } else if( dmg_type == damage_heat ) {
        return get_clothing_mod_val( clothing_mod_type_fire );
    } else if( dmg_type == damage_acid ) {
        return get_clothing_mod_val( clothing_mod_type_acid );
    }
    return 0.0f; // No mods for other damage types
}

int item::breathability( const bodypart_id &bp ) const
{
    const bool bp_null = bp == bodypart_id();
    // if the armor doesn't have armor data
    // or its not on a specific limb
    if( bp_null || armor_made_of( bp ).empty() ) {
        return breathability();
    }
    const armor_portion_data *a = portion_for_bodypart( bp );
    if( a == nullptr ) {
        //This body part might still be covered by attachments of this armor (for example face shield).
        //Check their breathability.
        if( is_ablative() ) {
            for( const item_pocket *pocket : contents.get_ablative_pockets() ) {
                if( !pocket->empty() ) {
                    // get the contained plate
                    const item &ablative_armor = pocket->front();
                    if( ablative_armor.covers( bp ) ) {
                        return ablative_armor.breathability( bp );
                    }
                }
            }
        }
        // if it doesn't cover it breathes great
        return 100;
    }

    return a->breathability;
}

int item::breathability() const
{
    // very rudimentary average for simple armor and non limb based situations
    int breathability = 0;

    const int total = type->mat_portion_total == 0 ? 1 : type->mat_portion_total;
    const std::map<material_id, int> mats = made_of();
    if( !mats.empty() ) {
        for( const auto &m : mats ) {
            breathability += m.first->breathability() * m.second;
        }
        // Average based portion of materials
        breathability /= total;
    }

    return breathability;
}

int item::chip_resistance( bool worst, const bodypart_id &bp ) const
{
    int res = worst ? INT_MAX : INT_MIN;
    if( bp != bodypart_id() ) {
        const std::vector<const part_material *> &armor_mats = armor_made_of( bp );
        // If we have armour portion materials for this body part, use that instead
        if( !armor_mats.empty() ) {
            for( const part_material *m : armor_mats ) {
                const int val = m->id->chip_resist() * m->cover;
                res = worst ? std::min( res, val ) : std::max( res, val );
            }
            if( res == INT_MAX || res == INT_MIN ) {
                return 2;
            }
            res /= 100;
            if( res <= 0 ) {
                return 0;
            }
            return res;
        }
    }

    const int total = type->mat_portion_total == 0 ? 1 : type->mat_portion_total;
    for( const std::pair<const material_id, int> &mat : made_of() ) {
        const int val = ( mat.first->chip_resist() * mat.second ) / total;
        res = worst ? std::min( res, val ) : std::max( res, val );
    }

    if( res == INT_MAX || res == INT_MIN ) {
        return 2;
    }

    if( res <= 0 ) {
        return 0;
    }

    return res;
}

void item::mitigate_damage( damage_unit &du, const bodypart_id &bp, int roll ) const
{
    const resistances res = resistances( *this, false, roll, bp );
    const float mitigation = res.get_effective_resist( du );
    // get_effective_resist subtracts the flat penetration value before multiplying the remaining armor.
    // therefore, res_pen is reduced by the full value of the item's armor value even though mitigation might be smaller (such as an attack with a 0.5 armor multiplier)
    du.res_pen -= res.type_resist( du.type );
    du.res_pen = std::max( 0.0f, du.res_pen );
    du.amount -= mitigation;
    du.amount = std::max( 0.0f, du.amount );
}

void item::mitigate_damage( damage_unit &du, const sub_bodypart_id &bp, int roll ) const
{
    const resistances res = resistances( *this, false, roll, bp );
    const float mitigation = res.get_effective_resist( du );
    // get_effective_resist subtracts the flat penetration value before multiplying the remaining armor.
    // therefore, res_pen is reduced by the full value of the item's armor value even though mitigation might be smaller (such as an attack with a 0.5 armor multiplier)
    du.res_pen -= res.type_resist( du.type );
    du.res_pen = std::max( 0.0f, du.res_pen );
    du.amount -= mitigation;
    du.amount = std::max( 0.0f, du.amount );
}

std::vector<const part_material *> item::armor_made_of( const bodypart_id &bp ) const
{
    std::vector<const part_material *> matlist;
    const islot_armor *a = find_armor_data();
    if( a == nullptr || a->data.empty() || !covers( bp ) ) {
        return matlist;
    }
    for( const armor_portion_data &d : a->data ) {
        if( !d.covers.has_value() ) {
            continue;
        }
        for( const bodypart_str_id &bpid : d.covers.value() ) {
            if( bp != bpid ) {
                continue;
            }
            matlist.reserve( matlist.size() + d.materials.size() );
            for( const part_material &m : d.materials ) {
                matlist.emplace_back( &m );
            }
            break;
        }
        if( !matlist.empty() ) {
            break;
        }
    }
    if( is_ablative() ) {
        for( const item_pocket *pocket : contents.get_ablative_pockets() ) {
            if( !pocket->empty() ) {
                // get the contained plate
                const item &ablative_armor = pocket->front();
                auto abl_mat = ablative_armor.armor_made_of( bp );
                //It is not clear how to aggregate armor material from different pockets.
                //A vest might contain two different plates from different materials.
                //But only one plate can be hit at one attack (according to Character::ablative_armor_absorb).
                //So for now return here only material for one plate + base clothing materials.
                if( !abl_mat.empty() ) {
                    matlist.insert( std::end( matlist ), std::begin( abl_mat ), std::end( abl_mat ) );
                    return matlist;
                }

            }
        }
    }
    return matlist;
}

std::vector<const part_material *> item::armor_made_of( const sub_bodypart_id &bp ) const
{
    std::vector<const part_material *> matlist;
    const islot_armor *a = find_armor_data();
    if( a == nullptr || a->data.empty() || !covers( bp ) ) {
        return matlist;
    }
    for( const armor_portion_data &d : a->sub_data ) {
        if( d.sub_coverage.empty() ) {
            continue;
        }
        for( const sub_bodypart_str_id &bpid : d.sub_coverage ) {
            if( bp != bpid ) {
                continue;
            }
            for( const part_material &m : d.materials ) {
                matlist.emplace_back( &m );
            }
            break;
        }
        if( !matlist.empty() ) {
            break;
        }
    }
    if( is_ablative() ) {
        for( const item_pocket *pocket : contents.get_ablative_pockets() ) {
            if( !pocket->empty() ) {
                // get the contained plate
                const item &ablative_armor = pocket->front();
                auto abl_mat = ablative_armor.armor_made_of( bp );
                if( !abl_mat.empty() ) {
                    matlist.insert( std::end( matlist ), std::begin( abl_mat ), std::end( abl_mat ) );
                    return matlist;
                }

            }
        }
    }
    return matlist;
}

bool item::is_holster() const
{
    return type->can_use( "holster" );
}

const islot_armor *item::find_armor_data() const
{
    if( type->armor ) {
        return &*type->armor;
    }
    // Currently the only way to make a non-armor item into armor is to install a gun mod.
    // The gunmods are stored in the items contents, as are the contents of a container, and the
    // tools in a tool belt (a container actually), or the ammo in a quiver (container again).
    for( const item *mod : gunmods() ) {
        if( mod->type->armor ) {
            return &*mod->type->armor;
        }
    }
    return nullptr;
}

bool item::is_pet_armor( bool on_pet ) const
{
    bool is_worn = on_pet && !get_var( "pet_armor", "" ).empty();
    return has_flag( flag_IS_PET_ARMOR ) && ( is_worn || !on_pet );
}

bool item::is_armor() const
{
    return find_armor_data() != nullptr || has_flag( flag_IS_ARMOR );
}

int item::wind_resist() const
{
    std::vector<const material_type *> materials = made_of_types();
    if( materials.empty() ) {
        debugmsg( "Called item::wind_resist on an item (%s) made of nothing!", tname() );
        return 99;
    }

    int best = -1;
    for( const material_type *mat : materials ) {
        std::optional<int> resistance = mat->wind_resist();
        if( resistance && *resistance > best ) {
            best = *resistance;
        }
    }

    // Default to 99% effective
    if( best == -1 ) {
        return 99;
    }

    return best;
}

bool item::is_soft() const
{
    if( has_flag( flag_SOFT ) ) {
        return true;
    } else if( has_flag( flag_HARD ) ) {
        return false;
    }

    const std::map<material_id, int> &mats = made_of();
    return std::all_of( mats.begin(), mats.end(), []( const std::pair<material_id, int> &mid ) {
        return mid.first->soft();
    } );
}

bool item::is_rigid() const
{
    // overrides for the item overall
    if( has_flag( flag_SOFT ) ) {
        return false;
    } else if( has_flag( flag_HARD ) ) {
        return true;
    }

    // if the item has no armor data it isn't rigid
    const islot_armor *armor = find_armor_data();
    if( armor == nullptr ) {
        return false;
    }

    bool is_rigid = false;

    for( const armor_portion_data &portion : armor->sub_data ) {
        is_rigid |= portion.rigid;
    }

    // check if ablative pieces are rigid too
    if( is_ablative() ) {
        for( const item_pocket *pocket : contents.get_ablative_pockets() ) {
            if( !pocket->empty() ) {
                // get the contained plate
                const item &ablative_armor = pocket->front();
                is_rigid |= ablative_armor.is_rigid();
            }
        }
    }

    return is_rigid;
}

bool item::is_comfortable() const
{
    // overrides for the item overall
    // NO_WEAR_EFFECT is there for jewelry and the like which is too small to be considered
    if( has_flag( flag_SOFT ) || has_flag( flag_PADDED ) || has_flag( flag_NO_WEAR_EFFECT ) ) {
        return true;
    } else if( has_flag( flag_HARD ) ) {
        return false;
    }

    // if the item has no armor data it isn't rigid
    const islot_armor *armor = find_armor_data();
    if( armor == nullptr ) {
        return true;
    }

    for( const armor_portion_data &portion : armor->sub_data ) {
        if( portion.comfortable ) {
            return true;
        }
    }

    return false;
}

template <typename T>
bool item::is_bp_rigid( const T &bp ) const
{
    // overrides for the item overall
    if( has_flag( flag_SOFT ) ) {
        return false;
    } else if( has_flag( flag_HARD ) ) {
        return true;
    }

    const armor_portion_data *portion = portion_for_bodypart( bp );

    bool is_rigid = false;

    if( portion ) {
        is_rigid |= portion->rigid;
    }

    // check if ablative pieces are rigid too
    if( is_ablative() ) {
        for( const item_pocket *pocket : contents.get_ablative_pockets() ) {
            if( !pocket->empty() ) {
                // get the contained plate
                const item &ablative_armor = pocket->front();
                is_rigid |= ablative_armor.is_bp_rigid( bp );
            }
        }
    }

    return is_rigid;
}

// initialize for sub_bodyparts and body parts
template bool item::is_bp_rigid<sub_bodypart_id>( const sub_bodypart_id &bp ) const;

template bool item::is_bp_rigid<bodypart_id>( const bodypart_id &bp ) const;

template <typename T>
bool item::is_bp_rigid_selective( const T &bp ) const
{
    bool is_rigid;

    const armor_portion_data *portion = portion_for_bodypart( bp );

    if( !portion ) {
        return false;
    }

    // overrides for the item overall
    if( has_flag( flag_SOFT ) ) {
        is_rigid = false;
    } else if( has_flag( flag_HARD ) ) {
        is_rigid = true;
    } else {
        is_rigid = portion->rigid;
    }

    return is_rigid && portion->rigid_layer_only;
}

// initialize for sub_bodyparts and body parts
template bool item::is_bp_rigid_selective<sub_bodypart_id>( const sub_bodypart_id &bp ) const;

template bool item::is_bp_rigid_selective<bodypart_id>( const bodypart_id &bp ) const;

template <typename T>
bool item::is_bp_comfortable( const T &bp ) const
{
    // overrides for the item overall
    // NO_WEAR_EFFECT is there for jewelry and the like which is too small to be considered
    if( has_flag( flag_SOFT ) || has_flag( flag_PADDED ) || has_flag( flag_NO_WEAR_EFFECT ) ) {
        return true;
    } else if( has_flag( flag_HARD ) ) {
        return false;
    }

    const armor_portion_data *portion = portion_for_bodypart( bp );

    if( !portion ) {
        return false;
    }

    return portion->comfortable;
}

// initialize for sub_bodyparts and body parts
template bool item::is_bp_comfortable<sub_bodypart_id>( const sub_bodypart_id &bp ) const;

template bool item::is_bp_comfortable<bodypart_id>( const bodypart_id &bp ) const;

bool item::has_clothing_mod() const
{
    for( const clothing_mod &cm : clothing_mods::get_all() ) {
        if( has_own_flag( cm.flag ) ) {
            return true;
        }
    }
    return false;
}

static const std::string &get_clothing_mod_val_key( clothing_mod_type type )
{
    const static auto cache = ( []() {
        std::array<std::string, clothing_mods::all_clothing_mod_types.size()> res;
        for( const clothing_mod_type &type : clothing_mods::all_clothing_mod_types ) {
            res[type] = CLOTHING_MOD_VAR_PREFIX + clothing_mods::string_from_clothing_mod_type(
                            clothing_mods::all_clothing_mod_types[type]
                        );
        }
        return res;
    } )();

    return cache[ type ];
}

float item::get_clothing_mod_val( clothing_mod_type type ) const
{
    return get_var( get_clothing_mod_val_key( type ), 0.0f );
}

void item::update_clothing_mod_val()
{
    for( const clothing_mod_type &type : clothing_mods::all_clothing_mod_types ) {
        float tmp = 0.0f;
        for( const clothing_mod &cm : clothing_mods::get_all_with( type ) ) {
            if( has_own_flag( cm.flag ) ) {
                tmp += cm.get_mod_val( type, *this );
            }
        }
        set_var( get_clothing_mod_val_key( type ), tmp );
    }
}

std::list<item *> item::all_ablative_armor()
{
    return contents.all_ablative_armor();
}

std::list<const item *> item::all_ablative_armor() const
{
    return contents.all_ablative_armor();
}
