#include "item.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <iomanip>
#include <iterator>
#include <set>
#include <sstream>

#include "advanced_inv.h"
#include "ammo.h"
#include "bionics.h"
#include "bodypart.h"
#include "cata_utility.h"
#include "coordinate_conversions.h"
#include "damage.h"
#include "debug.h"
#include "dispersion.h"
#include "effect.h" // for weed_msg
#include "enums.h"
#include "fault.h"
#include "field.h"
#include "fire.h"
#include "flag.h"
#include "game.h"
#include "game_constants.h"
#include "gun_mode.h"
#include "iexamine.h"
#include "item_category.h"
#include "item_factory.h"
#include "iteminfo_query.h"
#include "itype.h"
#include "iuse_actor.h"
#include "map.h"
#include "martialarts.h"
#include "material.h"
#include "messages.h"
#include "mtype.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "overmapbuffer.h"
#include "overmap.h"
#include "player.h"
#include "projectile.h"
#include "ranged.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "ret_val.h"
#include "skill.h"
#include "string_formatter.h"
#include "text_snippets.h"
#include "translations.h"
#include "units.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "vitamin.h"
#include "vpart_position.h"
#include "vpart_reference.h"
#include "weather.h"

static const std::string GUN_MODE_VAR_NAME( "item::mode" );

const skill_id skill_survival( "survival" );
const skill_id skill_melee( "melee" );
const skill_id skill_bashing( "bashing" );
const skill_id skill_cutting( "cutting" );
const skill_id skill_stabbing( "stabbing" );
const skill_id skill_unarmed( "unarmed" );
const skill_id skill_cooking( "cooking" );

const quality_id quality_jack( "JACK" );
const quality_id quality_lift( "LIFT" );

const species_id FISH( "FISH" );
const species_id BIRD( "BIRD" );
const species_id INSECT( "INSECT" );
const species_id ROBOT( "ROBOT" );

const efftype_id effect_cig( "cig" );
const efftype_id effect_shakes( "shakes" );
const efftype_id effect_sleep( "sleep" );
const efftype_id effect_weed_high( "weed_high" );

const material_id mat_leather( "leather" );
const material_id mat_kevlar( "kevlar" );

const trait_id trait_small2( "SMALL2" );
const trait_id trait_small_ok( "SMALL_OK" );

const std::string &rad_badge_color( const int rad )
{
    using pair_t = std::pair<const int, const std::string>;

    static const std::array<pair_t, 6> values = {{
            pair_t {  0, _( "green" ) },
            pair_t { 30, _( "blue" )  },
            pair_t { 60, _( "yellow" )},
            pair_t {120, pgettext( "color", "orange" )},
            pair_t {240, _( "red" )   },
            pair_t {500, _( "black" ) },
        }
    };

    for( const auto &i : values ) {
        if( rad <= i.first ) {
            return i.second;
        }
    }

    return values.back().second;
}

light_emission nolight = {0, 0, 0};

// Returns the default item type, used for the null item (default constructed),
// the returned pointer is always valid, it's never cleared by the @ref Item_factory.
static const itype *nullitem()
{
    static itype nullitem_m;
    return &nullitem_m;
}

item &null_item_reference()
{
    static item result{};
    // reset it, in case a previous caller has changed it
    result = item();
    return result;
}

const long item::INFINITE_CHARGES = std::numeric_limits<long>::max();

item::item() : bday( calendar::time_of_cataclysm )
{
    type = nullitem();
}

item::item( const itype *type, time_point turn, long qty ) : type( type ), bday( turn )
{
    corpse = typeId() == "corpse" ? &mtype_id::NULL_ID().obj() : nullptr;
    item_counter = type->countdown_interval;

    if( qty >= 0 ) {
        charges = qty;
    } else {
        if( type->tool && type->tool->rand_charges.size() > 1 ) {
            const auto charge_roll = rng( 1, type->tool->rand_charges.size() - 1 );
            charges = rng( type->tool->rand_charges[charge_roll - 1], type->tool->rand_charges[charge_roll] );
        } else {
            charges = type->charges_default();
        }
    }

    if( has_flag( "NANOFAB_TEMPLATE" ) ) {
        itype_id nanofab_recipe = item_group::item_from( "nanofab_recipes" ).typeId();
        set_var( "NANOFAB_ITEM_ID", nanofab_recipe );
    }

    if( type->gun ) {
        for( const auto &mod : type->gun->built_in_mods ) {
            emplace_back( mod, turn, qty ).item_tags.insert( "IRREMOVABLE" );
        }
        for( const auto &mod : type->gun->default_mods ) {
            emplace_back( mod, turn, qty );
        }

    } else if( type->magazine ) {
        if( type->magazine->count > 0 ) {
            emplace_back( type->magazine->default_ammo, calendar::turn, type->magazine->count );
        }

    } else if( type->comestible ) {
        active = is_food();
        last_temp_check = bday;

    } else if( type->tool ) {
        if( ammo_remaining() && ammo_type() ) {
            ammo_set( ammo_type()->default_ammotype(), ammo_remaining() );
        }
    }

    if( ( type->gun || type->tool ) && !magazine_integral() ) {
        set_var( "magazine_converted", 1 );
    }

    if( !type->snippet_category.empty() ) {
        note = SNIPPET.assign( type->snippet_category );
    }

    if( current_phase == PNULL ) {
        current_phase = type->phase;
    }
}

item::item( const itype_id &id, time_point turn, long qty )
    : item( find_type( id ), turn, qty ) {}

item::item( const itype *type, time_point turn, default_charges_tag )
    : item( type, turn, type->charges_default() ) {}

item::item( const itype_id &id, time_point turn, default_charges_tag tag )
    : item( find_type( id ), turn, tag ) {}

item::item( const itype *type, time_point turn, solitary_tag )
    : item( type, turn, type->count_by_charges() ? 1 : -1 ) {}

item::item( const itype_id &id, time_point turn, solitary_tag tag )
    : item( find_type( id ), turn, tag ) {}

item item::make_corpse( const mtype_id &mt, time_point turn, const std::string &name )
{
    if( !mt.is_valid() ) {
        debugmsg( "tried to make a corpse with an invalid mtype id" );
    }

    item result( "corpse", turn );
    result.corpse = &mt.obj();

    result.active = result.corpse->has_flag( MF_REVIVES );
    if( result.active && one_in( 20 ) ) {
        result.item_tags.insert( "REVIVE_SPECIAL" );
    }

    // This is unconditional because the const itemructor above sets result.name to
    // "human corpse".
    result.corpse_name = name;

    return result;
}

item &item::convert( const itype_id &new_type )
{
    type = find_type( new_type );
    return *this;
}

item &item::deactivate( const Character *ch, bool alert )
{
    if( !active ) {
        return *this; // no-op
    }

    if( is_tool() && type->tool->revert_to ) {
        if( ch && alert && !type->tool->revert_msg.empty() ) {
            ch->add_msg_if_player( m_info, _( type->tool->revert_msg.c_str() ), tname().c_str() );
        }
        convert( *type->tool->revert_to );
        active = false;

    }
    return *this;
}

item &item::activate()
{
    if( active ) {
        return *this; // no-op
    }

    if( type->countdown_interval > 0 ) {
        item_counter = type->countdown_interval;
    }

    active = true;

    return *this;
}

item &item::ammo_set( const itype_id &ammo, long qty )
{
    if( qty < 0 ) {
        // completely fill an integral or existing magazine
        if( magazine_integral() || magazine_current() ) {
            qty = ammo_capacity();

            // else try to add a magazine using default ammo count property if set
        } else if( magazine_default() != "null" ) {
            item mag( magazine_default() );
            if( mag.type->magazine->count > 0 ) {
                qty = mag.type->magazine->count;
            } else {
                qty = item( magazine_default() ).ammo_capacity();
            }
        }
    }

    if( qty <= 0 ) {
        ammo_unset();
        return *this;
    }

    // handle reloadable tools and guns with no specific ammo type as special case
    if( ( ammo == "null" && !ammo_type() ) || ammo_type().str() == "money" ) {
        if( ( is_tool() || is_gun() ) && magazine_integral() ) {
            curammo = nullptr;
            charges = std::min( qty, ammo_capacity() );
        }
        return *this;
    }

    // check ammo is valid for the item
    const itype *atype = item_controller->find_template( ammo );
    if( !atype->ammo || !atype->ammo->type.count( ammo_type() ) ) {
        debugmsg( "Tried to set invalid ammo of %s for %s", atype->nname( qty ).c_str(), tname().c_str() );
        return *this;
    }

    if( is_magazine() ) {
        ammo_unset();
        emplace_back( ammo, calendar::turn, std::min( qty, ammo_capacity() ) );
        if( has_flag( "NO_UNLOAD" ) ) {
            contents.back().item_tags.insert( "NO_DROP" );
            contents.back().item_tags.insert( "IRREMOVABLE" );
        }

    } else if( magazine_integral() ) {
        curammo = atype;
        charges = std::min( qty, ammo_capacity() );

    } else {
        if( !magazine_current() ) {
            const itype *mag = find_type( magazine_default() );
            if( !mag->magazine ) {
                debugmsg( "Tried to set ammo of %s without suitable magazine for %s",
                          atype->nname( qty ).c_str(), tname().c_str() );
                return *this;
            }

            // if default magazine too small fetch instead closest available match
            if( mag->magazine->capacity < qty ) {
                // as above call to magazine_default successful can infer minimum one option exists
                auto iter = type->magazines.find( ammo_type() );
                std::vector<itype_id> opts( iter->second.begin(), iter->second.end() );
                std::sort( opts.begin(), opts.end(), []( const itype_id & lhs, const itype_id & rhs ) {
                    return find_type( lhs )->magazine->capacity < find_type( rhs )->magazine->capacity;
                } );
                mag = find_type( opts.back() );
                for( const auto &e : opts ) {
                    if( find_type( e )->magazine->capacity >= qty ) {
                        mag = find_type( e );
                        break;
                    }
                }
            }
            emplace_back( mag );
        }
        magazine_current()->ammo_set( ammo, qty );
    }

    return *this;
}

item &item::ammo_unset()
{
    if( !is_tool() && !is_gun() && !is_magazine() ) {
        // do nothing
    } else if( is_magazine() ) {
        contents.clear();
    } else if( magazine_integral() ) {
        curammo = nullptr;
        charges = 0;
    } else if( magazine_current() ) {
        magazine_current()->ammo_unset();
    }

    return *this;
}

int item::damage() const
{
    return damage_;
}

int item::damage_level( int max ) const
{
    if( damage_ == 0 || max <= 0 ) {
        return 0;
    } else if( max_damage() <= 1 ) {
        return damage_ > 0 ? max : damage_;
    } else if( damage_ < 0 ) {
        return -( ( max - 1 ) * ( -damage_ - 1 ) / ( max_damage() - 1 ) + 1 );
    } else {
        return ( max - 1 ) * ( damage_ - 1 ) / ( max_damage() - 1 ) + 1;
    }
}

item &item::set_damage( int qty )
{
    damage_ = std::max( std::min( qty, max_damage() ), min_damage() );
    return *this;
}

item item::split( long qty )
{
    if( !count_by_charges() || qty <= 0 || qty >= charges ) {
        return item();
    }
    item res = *this;
    res.charges = qty;
    charges -= qty;
    return res;
}

bool item::is_null() const
{
    static const std::string s_null( "null" ); // used a lot, no need to repeat
    // Actually, type should never by null at all.
    return ( type == nullptr || type == nullitem() || typeId() == s_null );
}

bool item::is_unarmed_weapon() const
{
    return has_flag( "UNARMED_WEAPON" ) || is_null();
}

bool item::covers( const body_part bp ) const
{
    return get_covered_body_parts().test( bp );
}

body_part_set item::get_covered_body_parts() const
{
    return get_covered_body_parts( get_side() );
}

body_part_set item::get_covered_body_parts( const side s ) const
{
    body_part_set res;

    if( is_gun() ) {
        // Currently only used for guns with the should strap mod, other guns might
        // go on another bodypart.
        res.set( bp_torso );
    }

    const auto armor = find_armor_data();
    if( armor == nullptr ) {
        return res;
    }

    res |= armor->covers;

    if( !armor->sided ) {
        return res; // Just ignore the side.
    }

    switch( s ) {
        case side::BOTH:
            break;

        case side::LEFT:
            res.reset( bp_arm_r );
            res.reset( bp_hand_r );
            res.reset( bp_leg_r );
            res.reset( bp_foot_r );
            break;

        case side::RIGHT:
            res.reset( bp_arm_l );
            res.reset( bp_hand_l );
            res.reset( bp_leg_l );
            res.reset( bp_foot_l );
            break;
    }

    return res;
}

bool item::is_sided() const
{
    auto t = find_armor_data();
    return t ? t->sided : false;
}

side item::get_side() const
{
    // MSVC complains if directly cast double to enum
    return static_cast<side>( static_cast<int>( get_var( "lateral",
                              static_cast<int>( side::BOTH ) ) ) );
}

bool item::set_side( side s )
{
    if( !is_sided() ) {
        return false;
    }

    if( s == side::BOTH ) {
        erase_var( "lateral" );
    } else {
        set_var( "lateral", static_cast<int>( s ) );
    }

    return true;
}

bool item::swap_side()
{
    return set_side( opposite_side( get_side() ) );
}

bool item::is_worn_only_with( const item &it ) const
{
    return is_power_armor() && it.is_power_armor() && it.covers( bp_torso );
}

item item::in_its_container() const
{
    return in_container( type->default_container.value_or( "null" ) );
}

item item::in_container( const itype_id &cont ) const
{
    if( cont != "null" ) {
        item ret( cont, birthday() );
        ret.contents.push_back( *this );
        if( made_of( LIQUID ) && ret.is_container() ) {
            // Note: we can't use any of the normal container functions as they check the
            // container being suitable (seals, watertight etc.)
            ret.contents.back().charges = charges_per_volume( ret.get_container_capacity() );
        }

        ret.invlet = invlet;
        return ret;
    } else {
        return *this;
    }
}

long item::charges_per_volume( const units::volume &vol ) const
{
    if( type->volume == 0_ml ) {
        return INFINITE_CHARGES; // TODO: items should not have 0 volume at all!
    }
    // Type cast to prevent integer overflow with large volume containers like the cargo dimension
    return count_by_charges() ? vol * static_cast<int64_t>( type->stack_size ) / type->volume
           : vol / volume();
}

bool item::stacks_with( const item &rhs, bool check_components ) const
{
    if( type != rhs.type ) {
        return false;
    }
    if( ammo_type() == "money" && charges != 0 && rhs.charges != 0 ) {
        // Dealing with nonempty cash cards
        return true;
    }
    // This function is also used to test whether items counted by charges should be merged, for that
    // check the, the charges must be ignored. In all other cases (tools/guns), the charges are important.
    if( !count_by_charges() && charges != rhs.charges ) {
        return false;
    }
    if( damage_ != rhs.damage_ ) {
        return false;
    }
    if( burnt != rhs.burnt ) {
        return false;
    }
    if( active != rhs.active ) {
        return false;
    }
    if( item_tags != rhs.item_tags ) {
        return false;
    }
    if( faults != rhs.faults ) {
        return false;
    }
    if( techniques != rhs.techniques ) {
        return false;
    }
    if( item_vars != rhs.item_vars ) {
        return false;
    }
    if( goes_bad() ) {
        // If this goes bad, the other item should go bad, too. It only depends on the item type.
        // Stack items that fall into the same "bucket" of freshness.
        // Distant buckets are larger than near ones.
        std::pair<int, clipped_unit> my_clipped_time_to_rot =
            clipped_time( type->comestible->spoils - rot );
        std::pair<int, clipped_unit> other_clipped_time_to_rot =
            clipped_time( rhs.type->comestible->spoils - rhs.rot );
        if( my_clipped_time_to_rot != other_clipped_time_to_rot ) {
            return false;
        }
        if( rotten() != rhs.rotten() ) {
            // just to be safe that rotten and unrotten food is *never* stacked.
            return false;
        }
    }
    if( ( corpse == nullptr && rhs.corpse != nullptr ) ||
        ( corpse != nullptr && rhs.corpse == nullptr ) ) {
        return false;
    }
    if( corpse != nullptr && rhs.corpse != nullptr && corpse->id != rhs.corpse->id ) {
        return false;
    }
    if( check_components || rhs.is_comestible() ) {
        //Only check if at least one item isn't using the default recipe or is comestible
        if( !components.empty() || !rhs.components.empty() ) {
            if( get_uncraft_components() != rhs.get_uncraft_components() ) {
                return false;
            }
        }
    }
    if( contents.size() != rhs.contents.size() ) {
        return false;
    }
    return std::equal( contents.begin(), contents.end(), rhs.contents.begin(), []( const item & a,
    const item & b ) {
        return a.charges == b.charges && a.stacks_with( b );
    } );
}

bool item::merge_charges( const item &rhs )
{
    if( !count_by_charges() || !stacks_with( rhs ) ) {
        return false;
    }
    // Prevent overflow when either item has "near infinite" charges.
    if( charges >= INFINITE_CHARGES / 2 || rhs.charges >= INFINITE_CHARGES / 2 ) {
        charges = INFINITE_CHARGES;
        return true;
    }
    // We'll just hope that the item counter represents the same thing for both items
    if( item_counter > 0 || rhs.item_counter > 0 ) {
        item_counter = ( static_cast<double>( item_counter ) * charges + static_cast<double>
                         ( rhs.item_counter ) * rhs.charges ) / ( charges + rhs.charges );
    }
    charges += rhs.charges;
    return true;
}

void item::put_in( const item &payload )
{
    contents.push_back( payload );
}

void item::set_var( const std::string &name, const int value )
{
    std::ostringstream tmpstream;
    tmpstream.imbue( std::locale::classic() );
    tmpstream << value;
    item_vars[name] = tmpstream.str();
}

void item::set_var( const std::string &name, const long value )
{
    std::ostringstream tmpstream;
    tmpstream.imbue( std::locale::classic() );
    tmpstream << value;
    item_vars[name] = tmpstream.str();
}

void item::set_var( const std::string &name, const double value )
{
    item_vars[name] = string_format( "%f", value );
}

double item::get_var( const std::string &name, const double default_value ) const
{
    const auto it = item_vars.find( name );
    if( it == item_vars.end() ) {
        return default_value;
    }
    return atof( it->second.c_str() );
}

void item::set_var( const std::string &name, const tripoint &value )
{
    item_vars[name] = string_format( "%d,%d,%d", value.x, value.y, value.z );
}

tripoint item::get_var( const std::string &name, const tripoint &default_value ) const
{
    const auto it = item_vars.find( name );
    if( it == item_vars.end() ) {
        return default_value;
    }
    std::vector<std::string> values = string_split( it->second, ',' );
    return tripoint( atoi( values[0].c_str() ),
                     atoi( values[1].c_str() ),
                     atoi( values[2].c_str() ) );
}

void item::set_var( const std::string &name, const std::string &value )
{
    item_vars[name] = value;
}

std::string item::get_var( const std::string &name, const std::string &default_value ) const
{
    const auto it = item_vars.find( name );
    if( it == item_vars.end() ) {
        return default_value;
    }
    return it->second;
}

std::string item::get_var( const std::string &name ) const
{
    return get_var( name, "" );
}

bool item::has_var( const std::string &name ) const
{
    return item_vars.count( name ) > 0;
}

void item::erase_var( const std::string &name )
{
    item_vars.erase( name );
}

void item::clear_vars()
{
    item_vars.clear();
}

const char ivaresc = 001;

bool itag2ivar( std::string &item_tag, std::map<std::string, std::string> &item_vars )
{
    size_t pos = item_tag.find( '=' );
    if( item_tag.at( 0 ) == ivaresc && pos != std::string::npos && pos >= 2 ) {
        std::string val_decoded;
        int svarlen = 0;
        int svarsep = 0;
        svarsep = item_tag.find( '=' );
        svarlen = item_tag.size();
        val_decoded.clear();
        std::string var_name = item_tag.substr( 1, svarsep - 1 ); // will assume sanity here for now
        for( int s = svarsep + 1; s < svarlen;
             s++ ) { // cheap and temporary, AFAIK stringstream IFS = [\r\n\t ];
            if( item_tag[s] == ivaresc && s < svarlen - 2 ) {
                if( item_tag[s + 1] == '0' && item_tag[s + 2] == 'A' ) {
                    s += 2;
                    val_decoded.append( 1, '\n' );
                } else if( item_tag[s + 1] == '0' && item_tag[s + 2] == 'D' ) {
                    s += 2;
                    val_decoded.append( 1, '\r' );
                } else if( item_tag[s + 1] == '0' && item_tag[s + 2] == '6' ) {
                    s += 2;
                    val_decoded.append( 1, '\t' );
                } else if( item_tag[s + 1] == '2' && item_tag[s + 2] == '0' ) {
                    s += 2;
                    val_decoded.append( 1, ' ' );
                } else {
                    val_decoded.append( 1, item_tag[s] ); // hhrrrmmmmm should be passing \a?
                }
            } else {
                val_decoded.append( 1, item_tag[s] );
            }
        }
        item_vars[var_name] = val_decoded;
        return true;
    } else {
        return false;
    }
}

// @todo Get rid of, handle multiple types gracefully
static int get_ranged_pierce( const common_ranged_data &ranged )
{
    if( ranged.damage.empty() ) {
        return 0;
    }
    return ranged.damage.damage_units.front().res_pen;
}

std::string item::info( bool showtext ) const
{
    std::vector<iteminfo> dummy;
    return info( showtext, dummy );
}

std::string item::info( bool showtext, std::vector<iteminfo> &iteminfo ) const
{
    return info( showtext, iteminfo, 1 );
}

std::string item::info( bool showtext, std::vector<iteminfo> &iteminfo, int batch ) const
{
    return info( iteminfo, showtext ? &iteminfo_query::all : &iteminfo_query::notext, batch );
}

// Generates a long-form description of the freshness of the given rottable food item.
// NB: Doesn't check for non-rottable!
std::string get_freshness_description( const item &food_item )
{
    // So, skilled characters looking at food that is neither super-fresh nor about to rot
    // can guess its age as one of {quite fresh,midlife,past midlife,old soon}, and also
    // guess about how long until it spoils.
    const double rot_progress = food_item.get_relative_rot();
    const time_duration shelf_life = food_item.type->comestible->spoils;
    time_duration time_left = shelf_life - ( shelf_life * rot_progress );

    // Correct for an estimate that exceeds shelf life -- this happens especially with
    // fresh items.
    if( time_left > shelf_life ) {
        time_left = shelf_life;
    }

    if( food_item.is_fresh() ) {
        // Fresh food is assumed to be obviously so regardless of skill.
        if( g->u.can_estimate_rot() ) {
            return string_format( _( "* This food looks as <good>fresh</good> as it can be.  "
                                     "It still has <info>%s</info> until it spoils." ),
                                  to_string_approx( time_left ) );
        } else {
            return _( "* This food looks as <good>fresh</good> as it can be." );
        }
    } else if( food_item.is_going_bad() ) {
        // Old food likewise is assumed to be fairly obvious.
        if( g->u.can_estimate_rot() ) {
            return string_format( _( "* This food looks <bad>old</bad>.  "
                                     "It's just <info>%s</info> from becoming inedible." ),
                                  to_string_approx( time_left ) );
        } else {
            return _( "* This food looks <bad>old</bad>.  "
                      "It's on the brink of becoming inedible." );
        }
    }

    if( !g->u.can_estimate_rot() ) {
        // Unskilled characters only get a hint that more information exists...
        return _( "* This food looks <info>fine</info>.  If you were more skilled in "
                  "cooking or survival, you might be able to make a better estimation." );
    }

    // Otherwise, a skilled character can determine the below options:
    if( rot_progress < 0.3 ) {
        //~ here, %s is an approximate time span, e.g., "over 2 weeks" or "about 1 season"
        return string_format( _( "* This food looks <good>quite fresh</good>.  "
                                 "It has <info>%s</info> until it spoils." ),
                              to_string_approx( time_left ) );
    } else if( rot_progress < 0.5 ) {
        //~ here, %s is an approximate time span, e.g., "over 2 weeks" or "about 1 season"
        return string_format( _( "* This food looks like it is reaching its <neutral>midlife</neutral>.  "
                                 "There's <info>%s</info> before it spoils." ),
                              to_string_approx( time_left ) );
    } else if( rot_progress < 0.7 ) {
        //~ here, %s is an approximate time span, e.g., "over 2 weeks" or "about 1 season"
        return string_format( _( "* This food looks like it has <neutral>passed its midlife</neutral>.  "
                                 "Edible, but will go bad in <info>%s</info>." ),
                              to_string_approx( time_left ) );
    } else {
        //~ here, %s is an approximate time span, e.g., "over 2 weeks" or "about 1 season"
        return string_format( _( "* This food looks like it <bad>will be old soon</bad>.  "
                                 "It has <info>%s</info>, so if you plan to use it, it's now or never." ),
                              to_string_approx( time_left ) );
    }
}

int get_base_env_resist( const item &it )
{
    const auto t = it.find_armor_data();
    if( t == nullptr ) {
        return 0;
    }

    return t->env_resist * it.get_relative_health();
}

std::string item::info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch ) const
{
    std::stringstream temp1;
    std::stringstream temp2;
    std::string space = "  ";
    const bool debug = g != nullptr && debug_mode;

    if( parts == nullptr ) {
        parts = &iteminfo_query::all;
    }

    info.clear();

    auto insert_separation_line = [&]() {
        if( info.empty() || info.back().sName != "--" ) {
            info.push_back( iteminfo( "DESCRIPTION", "--" ) );
        }
    };

    if( !is_null() ) {
        if( parts->test( iteminfo_parts::BASE_CATEGORY ) )
            info.push_back( iteminfo( "BASE", _( "Category: " ),
                                      "<header>" + get_category().name() + "</header>",
                                      iteminfo::no_newline ) );
        const int price_preapoc = price( false ) * batch;
        const int price_postapoc = price( true ) * batch;
        if( parts->test( iteminfo_parts::BASE_PRICE ) )
            info.push_back( iteminfo( "BASE", space + _( "Price: " ), _( "$<num>" ),
                                      iteminfo::is_decimal | iteminfo::lower_is_better,
                                      static_cast<double>( price_preapoc ) / 100 ) );
        if( price_preapoc != price_postapoc && parts->test( iteminfo_parts::BASE_BARTER ) ) {
            info.push_back( iteminfo( "BASE", space + _( "Barter value: " ), _( "$<num>" ),
                                      iteminfo::is_decimal | iteminfo::lower_is_better,
                                      static_cast<double>( price_postapoc ) / 100 ) );
        }

        int converted_volume_scale = 0;
        const double converted_volume = round_up( convert_volume( volume().value(),
                                        &converted_volume_scale ) * batch, 2 );
        if( parts->test( iteminfo_parts::BASE_VOLUME ) ) {
            iteminfo::flags f = iteminfo::lower_is_better | iteminfo::no_newline;
            if( converted_volume_scale != 0 ) {
                f |= iteminfo::is_decimal;
            }
            info.push_back( iteminfo( "BASE", _( "<bold>Volume</bold>: " ),
                                      string_format( "<num> %s", volume_units_abbr() ),
                                      f, converted_volume ) );
        }
        if( parts->test( iteminfo_parts::BASE_WEIGHT ) )
            info.push_back( iteminfo( "BASE", space + _( "Weight: " ),
                                      string_format( "<num> %s", weight_units() ),
                                      iteminfo::lower_is_better | iteminfo::is_decimal,
                                      convert_weight( weight() ) * batch ) );

        if( !type->rigid && parts->test( iteminfo_parts::BASE_RIGIDITY ) ) {
            info.emplace_back( "BASE", _( "<bold>Rigid</bold>: " ), _( "No (contents increase volume)" ) );
        }

        int dmg_bash = damage_melee( DT_BASH );
        int dmg_cut  = damage_melee( DT_CUT );
        int dmg_stab = damage_melee( DT_STAB );
        if( parts->test( iteminfo_parts::BASE_DAMAGE ) ) {
            std::string sep;
            if( dmg_bash ) {
                info.emplace_back( "BASE", _( "Bash: " ), "", iteminfo::no_newline, dmg_bash );
                sep = space;
            }
            if( dmg_cut ) {
                info.emplace_back( "BASE", sep + _( "Cut: " ),
                                   "", iteminfo::no_newline, dmg_cut );
                sep = space;
            }
            if( dmg_stab ) {
                info.emplace_back( "BASE", sep + _( "Pierce: " ),
                                   "", iteminfo::no_newline, dmg_stab );
            }
        }

        if( dmg_bash || dmg_cut || dmg_stab ) {
            if( parts->test( iteminfo_parts::BASE_TOHIT ) )
                info.push_back( iteminfo( "BASE", space + _( "To-hit bonus: " ), "",
                                          iteminfo::show_plus, type->m_to_hit ) );

            if( parts->test( iteminfo_parts::BASE_MOVES ) )
                info.push_back( iteminfo( "BASE", _( "Moves per attack: " ),
                                          "", iteminfo::lower_is_better,
                                          attack_time() ) );
        }

        insert_separation_line();

        if( parts->test( iteminfo_parts::BASE_REQUIREMENTS ) ) {
            // Display any minimal stat or skill requirements for the item
            std::vector<std::string> req;
            if( get_min_str() > 0 ) {
                req.push_back( string_format( "%s %d", _( "strength" ), get_min_str() ) );
            }
            if( type->min_dex > 0 ) {
                req.push_back( string_format( "%s %d", _( "dexterity" ), type->min_dex ) );
            }
            if( type->min_int > 0 ) {
                req.push_back( string_format( "%s %d", _( "intelligence" ), type->min_int ) );
            }
            if( type->min_per > 0 ) {
                req.push_back( string_format( "%s %d", _( "perception" ), type->min_per ) );
            }
            for( const auto &sk : type->min_skills ) {
                req.push_back( string_format( "%s %d", skill_id( sk.first )->name().c_str(), sk.second ) );
            }
            if( !req.empty() ) {
                info.emplace_back( "BASE", _( "<bold>Minimum requirements:</bold>" ) );
                info.emplace_back( "BASE", enumerate_as_string( req ) );
                insert_separation_line();
            }
        }

        const std::vector<const material_type *> mat_types = made_of_types();
        if( !mat_types.empty() && parts->test( iteminfo_parts::BASE_MATERIAL ) ) {
            const std::string material_list = enumerate_as_string( mat_types.begin(), mat_types.end(),
            []( const material_type * material ) {
                return string_format( "<stat>%s</stat>", _( material->name().c_str() ) );
            }, enumeration_conjunction::none );
            info.push_back( iteminfo( "BASE", string_format( _( "Material: %s" ), material_list.c_str() ) ) );
        }
        if( has_var( "contained_name" ) && parts->test( iteminfo_parts::BASE_CONTENTS ) ) {
            info.push_back( iteminfo( "BASE", string_format( _( "Contains: %s" ),
                                      get_var( "contained_name" ).c_str() ) ) );
        }
        if( count_by_charges() && !is_food() && !is_medication() &&
            parts->test( iteminfo_parts::BASE_AMOUNT ) ) {
            info.push_back( iteminfo( "BASE", _( "Amount: " ), "<num>", iteminfo::no_flags,
                                      charges * batch ) );
        }
        if( debug && parts->test( iteminfo_parts::BASE_DEBUG ) ) {
            if( g != nullptr ) {
                info.push_back( iteminfo( "BASE", _( "age: " ), "", iteminfo::lower_is_better,
                                          to_hours<int>( age() ) ) );

                const item *food = is_food_container() ? &contents.front() : this;
                if( food && food->goes_bad() ) {
                    info.push_back( iteminfo( "BASE", _( "bday rot: " ),
                                              "", iteminfo::lower_is_better,
                                              to_turns<int>( food->age() ) ) );
                    info.push_back( iteminfo( "BASE", _( "temp rot: " ),
                                              "", iteminfo::lower_is_better,
                                              to_turns<int>( food->rot ) ) );
                    info.push_back( iteminfo( "BASE", space + _( "max rot: " ),
                                              "", iteminfo::lower_is_better,
                                              to_turns<int>( food->type->comestible->spoils ) ) );
                    info.push_back( iteminfo( "BASE", _( "last rot: " ),
                                              "", iteminfo::lower_is_better,
                                              to_turn<int>( food->last_rot_check ) ) );
                    info.push_back( iteminfo( "BASE", _( "last temp: " ),
                                              "", iteminfo::lower_is_better,
                                              to_turn<int>( food->last_temp_check ) ) );
                }
                if( food->item_tags.count( "HOT" ) ) {
                    info.push_back( iteminfo( "BASE", _( "HOT: " ), "", iteminfo::lower_is_better,
                                              food->item_counter ) );
                }
                if( food->item_tags.count( "COLD" ) ) {
                    info.push_back( iteminfo( "BASE", _( "COLD: " ), "",
                                              iteminfo::lower_is_better, food->item_counter ) );
                }
                if( food->item_tags.count( "FROZEN" ) ) {
                    info.push_back( iteminfo( "BASE", _( "FROZEN: " ), "",
                                              iteminfo::lower_is_better, food->item_counter ) );
                }

            }
            info.push_back( iteminfo( "BASE", _( "burn: " ), "", iteminfo::lower_is_better,
                                      burnt ) );
        }
    }

    const item *med_item = nullptr;
    if( is_medication() ) {
        med_item = this;
    } else if( is_med_container() ) {
        med_item = &contents.front();
    }
    if( med_item != nullptr ) {
        const auto &med_com = med_item->type->comestible;
        if( med_com->quench != 0 && parts->test( iteminfo_parts::MED_QUENCH ) ) {
            info.push_back( iteminfo( "MED", _( "Quench: " ), med_com->quench ) );
        }

        if( med_com->fun != 0 && parts->test( iteminfo_parts::MED_JOY ) ) {
            info.push_back( iteminfo( "MED", _( "Enjoyability: " ),
                                      g->u.fun_for( *med_item ).first ) );
        }

        if( med_com->stim != 0 && parts->test( iteminfo_parts::MED_STIMULATION ) ) {
            auto name = string_format( "%s <stat>%s</stat>", _( "Stimulation:" ),
                                       med_com->stim > 0 ? _( "Upper" ) : _( "Downer" ) );
            info.push_back( iteminfo( "MED", name ) );
        }

        if( parts->test( iteminfo_parts::MED_PORTIONS ) ) {
            info.push_back( iteminfo( "MED", _( "Portions: " ),
                                      abs( int( med_item->charges ) * batch ) ) );
        }

        if( med_com->addict && parts->test( iteminfo_parts::DESCRIPTION_MED_ADDICTING ) ) {
            info.emplace_back( "DESCRIPTION", _( "* Consuming this item is <bad>addicting</bad>." ) );
        }
    }

    const item *food_item = nullptr;
    if( is_food() ) {
        food_item = this;
    } else if( is_food_container() ) {
        food_item = &contents.front();
    }
    if( food_item != nullptr ) {
        if( g->u.kcal_for( *food_item ) != 0 || food_item->type->comestible->quench != 0 ) {
            if( parts->test( iteminfo_parts::FOOD_NUTRITION ) ) {
                auto value = g->u.kcal_for( *food_item );
                info.push_back( iteminfo( "FOOD", _( "<bold>Calories (kcal)</bold>: " ),
                                          "", iteminfo::no_newline, value ) );
            }
            if( parts->test( iteminfo_parts::FOOD_QUENCH ) ) {
                info.push_back( iteminfo( "FOOD", space + _( "Quench: " ),
                                          food_item->type->comestible->quench ) );
            }
        }

        if( food_item->type->comestible->fun != 0 && parts->test( iteminfo_parts::FOOD_JOY ) ) {
            info.push_back( iteminfo( "FOOD", _( "Enjoyability: " ),
                                      g->u.fun_for( *food_item ).first ) );
        }

        if( parts->test( iteminfo_parts::FOOD_PORTIONS ) ) {
            info.push_back( iteminfo( "FOOD", _( "Portions: " ),
                                      abs( int( food_item->charges ) * batch ) ) );
        }
        if( food_item->corpse != nullptr && ( debug || ( g != nullptr &&
                                              ( g->u.has_bionic( bionic_id( "bio_scent_vision" ) ) || g->u.has_trait( trait_id( "CARNIVORE" ) ) ||
                                                g->u.has_artifact_with( AEP_SUPER_CLAIRVOYANCE ) ) ) )
            && parts->test( iteminfo_parts::FOOD_SMELL ) ) {
            info.push_back( iteminfo( "FOOD", _( "Smells like: " ) + food_item->corpse->nname() ) );
        }

        const auto vits = g->u.vitamins_from( *food_item );
        const std::string required_vits = enumerate_as_string( vits.begin(),
        vits.end(), []( const std::pair<vitamin_id, int> &v ) {
            return ( g->u.vitamin_rate( v.first ) > 0_turns &&
                     v.second != 0 ) // only display vitamins that we actually require
                   ? string_format( "%s (%i%%)", v.first.obj().name().c_str(),
                                    int( v.second * g->u.vitamin_rate( v.first ) / 1_days * 100 ) )
                   : std::string();
        } );
        if( !required_vits.empty() && parts->test( iteminfo_parts::FOOD_VITAMINS ) ) {
            info.emplace_back( "FOOD", _( "Vitamins (RDA): " ), required_vits.c_str() );
        }

        if( food_item->has_flag( "CANNIBALISM" ) && parts->test( iteminfo_parts::FOOD_CANNIBALISM ) ) {
            if( !g->u.has_trait_flag( "CANNIBAL" ) ) {
                info.emplace_back( "DESCRIPTION", _( "* This food contains <bad>human flesh</bad>." ) );
            } else {
                info.emplace_back( "DESCRIPTION", _( "* This food contains <good>human flesh</good>." ) );
            }
        }

        if( food_item->is_tainted() && parts->test( iteminfo_parts::FOOD_CANNIBALISM ) ) {
            info.emplace_back( "DESCRIPTION", _( "* This food is <bad>tainted</bad> and will poison you." ) );
        }

        ///\EFFECT_SURVIVAL >=3 allows detection of poisonous food
        if( food_item->has_flag( "HIDDEN_POISON" ) && g->u.get_skill_level( skill_survival ) >= 3 &&
            parts->test( iteminfo_parts::FOOD_POISON ) ) {
            info.emplace_back( "DESCRIPTION",
                               _( "* On closer inspection, this appears to be <bad>poisonous</bad>." ) );
        }

        ///\EFFECT_SURVIVAL >=5 allows detection of hallucinogenic food
        if( food_item->has_flag( "HIDDEN_HALLU" ) && g->u.get_skill_level( skill_survival ) >= 5 &&
            parts->test( iteminfo_parts::FOOD_HALLUCINOGENIC ) ) {
            info.emplace_back( "DESCRIPTION",
                               _( "* On closer inspection, this appears to be <neutral>hallucinogenic</neutral>." ) );
        }

        if( food_item->goes_bad() && parts->test( iteminfo_parts::FOOD_ROT ) ) {
            const std::string rot_time = to_string_clipped( food_item->type->comestible->spoils );
            info.emplace_back( "DESCRIPTION",
                               string_format(
                                   _( "* This food is <neutral>perishable</neutral>, and at room temperature has an estimated nominal shelf life of <info>%s</info>." ),
                                   rot_time.c_str() ) );

            if( !food_item->rotten() ) {
                info.emplace_back( "DESCRIPTION", get_freshness_description( *food_item ) );
            }

            if( food_item->has_flag( "FREEZERBURN" ) && !food_item->rotten() &&
                !food_item->has_flag( "MUSHY" ) ) {
                info.emplace_back( "DESCRIPTION",
                                   _( "* Quality of this food suffers when it's frozen, and it <neutral>will become mushy after thawing out</neutral>." ) );
            }
            if( food_item->has_flag( "MUSHY" ) && !food_item->rotten() ) {
                info.emplace_back( "DESCRIPTION",
                                   _( "* It was frozen once and after thawing became <bad>mushy and tasteless</bad>.  It will rot if thawed again." ) );
            }
            if( food_item->has_flag( "NO_PARASITES" ) && g->u.get_skill_level( skill_cooking ) >= 3 ) {
                info.emplace_back( "DESCRIPTION",
                                   _( "* It seems that deep freezing <good>killed all parasites</good>." ) );
            }
            if( food_item->rotten() ) {
                if( g->u.has_bionic( bionic_id( "bio_digestion" ) ) ) {
                    info.push_back( iteminfo( "DESCRIPTION",
                                              _( "This food has started to <neutral>rot</neutral>, but <info>your bionic digestion can tolerate it</info>." ) ) );
                } else if( g->u.has_trait( trait_id( "SAPROVORE" ) ) ) {
                    info.push_back( iteminfo( "DESCRIPTION",
                                              _( "This food has started to <neutral>rot</neutral>, but <info>you can tolerate it</info>." ) ) );
                } else {
                    info.push_back( iteminfo( "DESCRIPTION",
                                              _( "This food has started to <bad>rot</bad>.  <info>Eating</info> it would be a <bad>very bad idea</bad>." ) ) );
                }
            }
        }
    }

    if( is_magazine() && !has_flag( "NO_RELOAD" ) ) {

        if( parts->test( iteminfo_parts::MAGAZINE_CAPACITY ) ) {
            auto fmt = string_format(
                           ngettext( "<num> round of %s", "<num> rounds of %s", ammo_capacity() ),
                           ammo_type()->name() );
            info.emplace_back( "MAGAZINE", _( "Capacity: " ), fmt, iteminfo::no_flags,
                               ammo_capacity() );
        }
        if( parts->test( iteminfo_parts::MAGAZINE_RELOAD ) )
            info.emplace_back( "MAGAZINE", _( "Reload time: " ), _( "<num> per round" ),
                               iteminfo::lower_is_better, type->magazine->reload_time );

        insert_separation_line();
    }

    if( !is_gun() ) {
        if( ammo_data() && parts->test( iteminfo_parts::AMMO_REMAINING_OR_TYPES ) ) {
            if( ammo_remaining() > 0 ) {
                info.emplace_back( "AMMO", _( "Ammunition: " ), ammo_data()->nname( ammo_remaining() ) );
            } else if( is_ammo() ) {
                info.emplace_back( "AMMO", _( "Types: " ),
                                   enumerate_as_string( type->ammo->type.begin(), type->ammo->type.end(),
                []( const ammotype & e ) {
                    return e->name();
                }, enumeration_conjunction::none ) );
            }

            const auto &ammo = *ammo_data()->ammo;
            if( !ammo.damage.empty() || ammo.prop_damage ) {
                if( !ammo.damage.empty() ) {
                    if( parts->test( iteminfo_parts::AMMO_DAMAGE_VALUE ) ) {
                        info.emplace_back( "AMMO", _( "<bold>Damage</bold>: " ), "",
                                           iteminfo::no_newline, ammo.damage.total_damage() );
                    }
                } else if( ammo.prop_damage ) {
                    if( parts->test( iteminfo_parts::AMMO_DAMAGE_PROPORTIONAL ) ) {
                        info.emplace_back( "AMMO", _( "<bold>Damage multiplier</bold>: " ), "",
                                           iteminfo::no_newline, *ammo.prop_damage );
                    }
                }
                if( parts->test( iteminfo_parts::AMMO_DAMAGE_AP ) ) {
                    info.emplace_back( "AMMO", space + _( "Armor-pierce: " ),
                                       get_ranged_pierce( ammo ) );
                }
                if( parts->test( iteminfo_parts::AMMO_DAMAGE_RANGE ) ) {
                    info.emplace_back( "AMMO", _( "Range: " ), "",
                                       iteminfo::no_newline, ammo.range );
                }
                if( parts->test( iteminfo_parts::AMMO_DAMAGE_DISPERSION ) ) {
                    info.emplace_back( "AMMO", space + _( "Dispersion: " ), "",
                                       iteminfo::lower_is_better, ammo.dispersion );
                }
                if( parts->test( iteminfo_parts::AMMO_DAMAGE_RECOIL ) ) {
                    info.emplace_back( "AMMO", _( "Recoil: " ), "",
                                       iteminfo::lower_is_better, ammo.recoil );
                }
            }

            std::vector<std::string> fx;
            if( ammo.ammo_effects.count( "RECYCLED" ) && parts->test( iteminfo_parts::AMMO_FX_RECYCLED ) ) {
                fx.emplace_back( _( "This ammo has been <bad>hand-loaded</bad>" ) );
            }
            if( ammo.ammo_effects.count( "NEVER_MISFIRES" ) &&
                parts->test( iteminfo_parts::AMMO_FX_CANTMISSFIRE ) ) {
                fx.emplace_back( _( "This ammo <good>never misfires</good>" ) );
            }
            if( ammo.ammo_effects.count( "INCENDIARY" ) && parts->test( iteminfo_parts::AMMO_FX_INDENDIARY ) ) {
                fx.emplace_back( _( "This ammo <neutral>starts fires</neutral>" ) );
            }
            if( !fx.empty() ) {
                insert_separation_line();
                for( const auto &e : fx ) {
                    info.emplace_back( "AMMO", e );
                }
            }
        }

    } else {
        const item *mod = this;
        const auto aux = gun_current_mode();
        // if we have an active auxiliary gunmod display stats for this instead
        if( aux && aux->is_gunmod() && aux->is_gun() &&
            parts->test( iteminfo_parts::DESCRIPTION_AUX_GUNMOD_HEADER ) ) {
            mod = &*aux;
            info.emplace_back( "DESCRIPTION",
                               string_format( _( "Stats of the active <info>gunmod (%s)</info> are shown." ),
                                              mod->tname().c_str() ) );
        }

        // many statistics are dependent upon loaded ammo
        // if item is unloaded (or is RELOAD_AND_SHOOT) shows approximate stats using default ammo
        item *aprox = nullptr;
        item tmp;
        if( mod->ammo_required() && !mod->ammo_remaining() ) {
            tmp = *mod;
            tmp.ammo_set( tmp.ammo_default() );
            aprox = &tmp;
        }

        const islot_gun &gun = *mod->type->gun;
        const auto curammo = mod->ammo_data();

        bool has_ammo = curammo && mod->ammo_remaining();

        damage_instance ammo_dam = has_ammo ? curammo->ammo->damage : damage_instance();
        // @todo This doesn't cover multiple damage types
        int ammo_pierce     = has_ammo ? get_ranged_pierce( *curammo->ammo ) : 0;
        int ammo_dispersion = has_ammo ? curammo->ammo->dispersion : 0;

        const Skill &skill = *mod->gun_skill();

        if( parts->test( iteminfo_parts::GUN_USEDSKILL ) ) {
            info.push_back( iteminfo( "GUN", _( "Skill used: " ),
                                      "<info>" + skill.name() + "</info>" ) );
        }

        if( mod->magazine_integral() || mod->magazine_current() ) {
            if( mod->magazine_current() && parts->test( iteminfo_parts::GUN_MAGAZINE ) ) {
                info.emplace_back( "GUN", _( "Magazine: " ),
                                   string_format( "<stat>%s</stat>",
                                                  mod->magazine_current()->tname() ) );
            }
            if( mod->ammo_capacity() && parts->test( iteminfo_parts::GUN_CAPACITY ) ) {
                auto fmt = string_format(
                               ngettext( "<num> round of %s", "<num> rounds of %s", mod->ammo_capacity() ),
                               mod->ammo_type()->name() );
                info.emplace_back( "GUN", _( "<bold>Capacity:</bold> " ), fmt, iteminfo::no_flags,
                                   mod->ammo_capacity() );
            }
        } else if( parts->test( iteminfo_parts::GUN_TYPE ) ) {
            info.emplace_back( "GUN", _( "Type: " ), mod->ammo_type()->name() );
        }

        if( mod->ammo_data() && parts->test( iteminfo_parts::AMMO_REMAINING ) ) {
            info.emplace_back( "AMMO", _( "Ammunition: " ), string_format( "<stat>%s</stat>",
                               mod->ammo_data()->nname( mod->ammo_remaining() ).c_str() ) );
        }

        if( mod->get_gun_ups_drain() && parts->test( iteminfo_parts::AMMO_UPSCOST ) ) {
            info.emplace_back( "AMMO", string_format( ngettext( "Uses <stat>%i</stat> charge of UPS per shot",
                               "Uses <stat>%i</stat> charges of UPS per shot", mod->get_gun_ups_drain() ),
                               mod->get_gun_ups_drain() ) );
        }

        insert_separation_line();

        int max_gun_range = mod->gun_range( &g->u );
        if( max_gun_range > 0 && parts->test( iteminfo_parts::GUN_MAX_RANGE ) ) {
            info.emplace_back( "GUN", _( "Maximum range: " ), "<num>", iteminfo::no_flags,
                               max_gun_range );
        }

        if( parts->test( iteminfo_parts::GUN_AIMING_STATS ) ) {
            info.emplace_back( "GUN", _( "Base aim speed: " ), "<num>", iteminfo::no_flags,
                               g->u.aim_per_move( *mod, MAX_RECOIL ) );
            for( const aim_type &type : g->u.get_aim_types( *mod ) ) {
                // Nameless aim levels don't get an entry.
                if( type.name.empty() ) {
                    continue;
                }
                info.emplace_back( "GUN", _( type.name.c_str() ) );
                int max_dispersion = g->u.get_weapon_dispersion( *mod ).max();
                int range = range_with_even_chance_of_good_hit( max_dispersion + type.threshold );
                info.emplace_back( "GUN", _( "Even chance of good hit at range: " ),
                                   _( "<num>" ), iteminfo::no_flags, range );
                int aim_mv = g->u.gun_engagement_moves( *mod, type.threshold );
                info.emplace_back( "GUN", _( "Time to reach aim level: " ), _( "<num> seconds" ),
                                   iteminfo::is_decimal | iteminfo::lower_is_better,
                                   TICKS_TO_SECONDS( aim_mv ) );
            }
        }

        if( parts->test( iteminfo_parts::GUN_DAMAGE ) ) {
            info.push_back( iteminfo( "GUN", _( "Damage: " ), "", iteminfo::no_newline,
                                      mod->gun_damage( false ).total_damage() ) );
        }

        if( has_ammo ) {
            // ammo_damage, sum_of_damage, and ammo_mult not shown so don't need to translate.
            if( mod->ammo_data()->ammo->prop_damage ) {
                if( parts->test( iteminfo_parts::GUN_DAMAGE_AMMOPROP ) )
                    info.push_back( iteminfo( "GUN", "ammo_mult", "*",
                                              iteminfo::no_newline | iteminfo::no_name,
                                              *mod->ammo_data()->ammo->prop_damage ) );
            } else {
                if( parts->test( iteminfo_parts::GUN_DAMAGE_LOADEDAMMO ) )
                    info.push_back( iteminfo( "GUN", "ammo_damage", "",
                                              iteminfo::no_newline | iteminfo::no_name |
                                              iteminfo::show_plus,
                                              ammo_dam.total_damage() ) );
            }
            if( parts->test( iteminfo_parts::GUN_DAMAGE_TOTAL ) )
                info.push_back( iteminfo( "GUN", "sum_of_damage", _( " = <num>" ),
                                          iteminfo::no_newline | iteminfo::no_name,
                                          mod->gun_damage( true ).total_damage() ) );
        }

        if( parts->test( iteminfo_parts::GUN_ARMORPIERCE ) )
            info.push_back( iteminfo( "GUN", space + _( "Armor-pierce: " ), "",
                                      iteminfo::no_newline, get_ranged_pierce( gun ) ) );
        if( has_ammo ) {
            // ammo_armor_pierce and sum_of_armor_pierce don't need to translate.
            if( parts->test( iteminfo_parts::GUN_ARMORPIERCE_LOADEDAMMO ) )
                info.push_back( iteminfo( "GUN", "ammo_armor_pierce", "",
                                          iteminfo::no_newline | iteminfo::no_name |
                                          iteminfo::show_plus, ammo_pierce ) );
            if( parts->test( iteminfo_parts::GUN_ARMORPIERCE_TOTAL ) )
                info.push_back( iteminfo( "GUN", "sum_of_armor_pierce", _( " = <num>" ),
                                          iteminfo::no_name,
                                          get_ranged_pierce( gun ) + ammo_pierce ) );
        }
        info.back().bNewLine = true;

        if( parts->test( iteminfo_parts::GUN_DISPERSION ) )
            info.push_back( iteminfo( "GUN", _( "Dispersion: " ), "",
                                      iteminfo::no_newline | iteminfo::lower_is_better,
                                      mod->gun_dispersion( false, false ) ) );
        if( has_ammo ) {
            // ammo_dispersion and sum_of_dispersion don't need to translate.
            if( parts->test( iteminfo_parts::GUN_DISPERSION_LOADEDAMMO ) )
                info.push_back( iteminfo( "GUN", "ammo_dispersion", "",
                                          iteminfo::no_newline | iteminfo::lower_is_better |
                                          iteminfo::no_name | iteminfo::show_plus,
                                          ammo_dispersion ) );
            if( parts->test( iteminfo_parts::GUN_DISPERSION_TOTAL ) )
                info.push_back( iteminfo( "GUN", "sum_of_dispersion", _( " = <num>" ),
                                          iteminfo::lower_is_better | iteminfo::no_name,
                                          mod->gun_dispersion( true, false ) ) );
        }
        info.back().bNewLine = true;

        // if effective sight dispersion differs from actual sight dispersion display both
        int act_disp = mod->sight_dispersion();
        int eff_disp = g->u.effective_dispersion( act_disp );
        int adj_disp = eff_disp - act_disp;

        if( parts->test( iteminfo_parts::GUN_DISPERSION_SIGHT ) ) {
            info.push_back( iteminfo( "GUN", _( "Sight dispersion: " ), "",
                                      iteminfo::no_newline | iteminfo::lower_is_better,
                                      act_disp ) );

            if( adj_disp ) {
                info.push_back( iteminfo( "GUN", "sight_adj_disp", "",
                                          iteminfo::no_newline | iteminfo::lower_is_better |
                                          iteminfo::no_name | iteminfo::show_plus,
                                          adj_disp ) );
                info.push_back( iteminfo( "GUN", "sight_eff_disp", _( " = <num>" ),
                                          iteminfo::lower_is_better | iteminfo::no_name,
                                          eff_disp ) );
            }
        }

        bool bipod = mod->has_flag( "BIPOD" );
        if( aprox ) {
            if( aprox->gun_recoil( g->u ) ) {
                if( parts->test( iteminfo_parts::GUN_RECOIL ) )
                    info.emplace_back( "GUN", _( "Approximate recoil: " ), "",
                                       iteminfo::no_newline | iteminfo::lower_is_better,
                                       aprox->gun_recoil( g->u ) );
                if( bipod && parts->test( iteminfo_parts::GUN_RECOIL_BIPOD ) ) {
                    info.emplace_back( "GUN", "bipod_recoil", _( " (with bipod <num>)" ),
                                       iteminfo::lower_is_better | iteminfo::no_name,
                                       aprox->gun_recoil( g->u, true ) );
                }
            }
        } else {
            if( mod->gun_recoil( g->u ) ) {
                if( parts->test( iteminfo_parts::GUN_RECOIL ) )
                    info.emplace_back( "GUN", _( "Effective recoil: " ), "",
                                       iteminfo::no_newline | iteminfo::lower_is_better,
                                       mod->gun_recoil( g->u ) );
                if( bipod && parts->test( iteminfo_parts::GUN_RECOIL_BIPOD ) ) {
                    info.emplace_back( "GUN", "bipod_recoil", _( " (with bipod <num>)" ),
                                       iteminfo::lower_is_better | iteminfo::no_name,
                                       mod->gun_recoil( g->u, true ) );
                }
            }
        }
        info.back().bNewLine = true;

        auto fire_modes = mod->gun_all_modes();
        if( std::any_of( fire_modes.begin(), fire_modes.end(),
        []( const std::pair<gun_mode_id, gun_mode> &e ) {
        return e.second.qty > 1 && !e.second.melee();
        } ) ) {
            info.emplace_back( "GUN", _( "Recommended strength (burst): " ), "",
                               iteminfo::lower_is_better,
                               ceil( mod->type->weight / 333.0_gram ) );
        }

        if( parts->test( iteminfo_parts::GUN_RELOAD_TIME ) )
            info.emplace_back( "GUN", _( "Reload time: " ),
                               has_flag( "RELOAD_ONE" ) ? _( "<num> seconds per round" ) : _( "<num> seconds" ),
                               iteminfo::lower_is_better,
                               int( mod->get_reload_time() / 16.67 ) );

        if( parts->test( iteminfo_parts::GUN_FIRE_MODES ) ) {
            std::vector<std::string> fm;
            for( const auto &e : fire_modes ) {
                if( e.second.target == this && !e.second.melee() ) {
                    fm.emplace_back( string_format( "%s (%i)", e.second.name(), e.second.qty ) );
                }
            }
            if( !fm.empty() ) {
                insert_separation_line();
                info.emplace_back( "GUN", _( "<bold>Fire modes:</bold> " ) + enumerate_as_string( fm ) );
            }
        }

        if( !magazine_integral() && parts->test( iteminfo_parts::GUN_ALLOWED_MAGAZINES ) ) {
            insert_separation_line();
            const auto compat = magazine_compatible();
            info.emplace_back( "DESCRIPTION", _( "<bold>Compatible magazines:</bold> " ) +
            enumerate_as_string( compat.begin(), compat.end(), []( const itype_id & id ) {
                return item_controller->find_template( id )->nname( 1 );
            } ) );
        }

        if( !gun.valid_mod_locations.empty() && parts->test( iteminfo_parts::DESCRIPTION_GUN_MODS ) ) {
            insert_separation_line();

            temp1.str( "" );
            temp1 << _( "<bold>Mods:</bold> " );

            std::map<gunmod_location, int> mod_locations = get_mod_locations();

            int iternum = 0;
            for( auto &elem : mod_locations ) {
                if( iternum != 0 ) {
                    temp1 << "; ";
                }
                const int free_slots = ( elem ).second - get_free_mod_locations( ( elem ).first );
                temp1 << "<bold>" << free_slots << "/" << ( elem ).second << "</bold> " << elem.first.name();
                bool first_mods = true;
                for( const auto mod : gunmods() ) {
                    if( mod->type->gunmod->location == ( elem ).first ) { // if mod for this location
                        if( first_mods ) {
                            temp1 << ": ";
                            first_mods = false;
                        } else {
                            temp1 << ", ";
                        }
                        temp1 << "<stat>" << mod->tname() << "</stat>";
                    }
                }
                iternum++;
            }
            temp1 << ".";
            info.push_back( iteminfo( "DESCRIPTION", temp1.str() ) );
        }

        if( mod->casings_count() && parts->test( iteminfo_parts::DESCRIPTION_GUN_CASINGS ) ) {
            insert_separation_line();
            std::string tmp = ngettext( "Contains <stat>%i</stat> casing",
                                        "Contains <stat>%i</stat> casings", mod->casings_count() );
            info.emplace_back( "DESCRIPTION", string_format( tmp, mod->casings_count() ) );
        }

    }
    if( is_gunmod() ) {
        const auto &mod = *type->gunmod;

        if( is_gun() && parts->test( iteminfo_parts::DESCRIPTION_GUNMOD ) ) {
            info.push_back( iteminfo( "DESCRIPTION",
                                      _( "This mod <info>must be attached to a gun</info>, it can not be fired separately." ) ) );
        }
        if( has_flag( "REACH_ATTACK" ) && parts->test( iteminfo_parts::DESCRIPTION_GUNMOD_REACH ) ) {
            info.push_back( iteminfo( "DESCRIPTION",
                                      _( "When attached to a gun, <good>allows</good> making <info>reach melee attacks</info> with it." ) ) );
        }
        if( mod.dispersion != 0 && parts->test( iteminfo_parts::GUNMOD_DISPERSION ) ) {
            info.push_back( iteminfo( "GUNMOD", _( "Dispersion modifier: " ), "",
                                      iteminfo::lower_is_better | iteminfo::show_plus,
                                      mod.dispersion ) );
        }
        if( mod.sight_dispersion != -1 && parts->test( iteminfo_parts::GUNMOD_DISPERSION_SIGHT ) ) {
            info.push_back( iteminfo( "GUNMOD", _( "Sight dispersion: " ), "",
                                      iteminfo::lower_is_better, mod.sight_dispersion ) );
        }
        if( mod.aim_speed >= 0 && parts->test( iteminfo_parts::GUNMOD_AIMSPEED ) ) {
            info.push_back( iteminfo( "GUNMOD", _( "Aim speed: " ), "",
                                      iteminfo::lower_is_better, mod.aim_speed ) );
        }
        int total_damage = static_cast<int>( mod.damage.total_damage() );
        if( total_damage != 0 && parts->test( iteminfo_parts::GUNMOD_DAMAGE ) ) {
            info.push_back( iteminfo( "GUNMOD", _( "Damage: " ), "", iteminfo::show_plus,
                                      total_damage ) );
        }
        int pierce = get_ranged_pierce( mod );
        if( get_ranged_pierce( mod ) != 0 && parts->test( iteminfo_parts::GUNMOD_ARMORPIERCE ) ) {
            info.push_back( iteminfo( "GUNMOD", _( "Armor-pierce: " ), "", iteminfo::show_plus,
                                      pierce ) );
        }
        if( mod.handling != 0 && parts->test( iteminfo_parts::GUNMOD_HANDLING ) ) {
            info.emplace_back( "GUNMOD", _( "Handling modifier: " ), "",
                               iteminfo::show_plus, mod.handling );
        }
        if( type->mod->ammo_modifier && parts->test( iteminfo_parts::GUNMOD_AMMO ) ) {
            info.push_back( iteminfo( "GUNMOD", string_format( _( "Ammo: <stat>%s</stat>" ),
                                      type->mod->ammo_modifier->name() ) ) );
        }
        if( mod.reload_modifier != 0 && parts->test( iteminfo_parts::GUNMOD_RELOAD ) ) {
            info.emplace_back( "GUNMOD", _( "Reload modifier: " ), _( "<num>%" ),
                               iteminfo::lower_is_better, mod.reload_modifier );
        }
        if( mod.min_str_required_mod > 0 && parts->test( iteminfo_parts::GUNMOD_STRENGTH ) ) {
            info.push_back( iteminfo( "GUNMOD", _( "Minimum strength required modifier: " ),
                                      mod.min_str_required_mod ) );
        }
        if( !mod.add_mod.empty() && parts->test( iteminfo_parts::GUNMOD_ADD_MOD ) ) {
            insert_separation_line();

            temp1.str( "" );
            temp1 << _( "<bold>Adds mod locations: </bold> " );

            std::map<gunmod_location, int> mod_locations = mod.add_mod;

            int iternum = 0;
            for( auto &elem : mod_locations ) {
                if( iternum != 0 ) {
                    temp1 << "; ";
                }
                temp1 << "<bold>" << elem.second << "</bold> " << elem.first.name();
                iternum++;
            }
            temp1 << ".";
            info.push_back( iteminfo( "GUNMOD", temp1.str() ) );
        }

        insert_separation_line();
        temp1.str( "" );
        temp1 << _( "Used on: " ) << enumerate_as_string( mod.usable.begin(),
        mod.usable.end(), []( const gun_type_type & used_on ) {
            return string_format( "<info>%s</info>", used_on.name().c_str() );
        } );

        temp2.str( "" );
        temp2 << _( "Location: " );
        temp2 << mod.location.name();

        if( parts->test( iteminfo_parts::GUNMOD_USEDON ) ) {
            info.push_back( iteminfo( "GUNMOD", temp1.str() ) );
        }
        if( parts->test( iteminfo_parts::GUNMOD_LOCATION ) ) {
            info.push_back( iteminfo( "GUNMOD", temp2.str() ) );
        }
        if( !mod.blacklist_mod.empty() && parts->test( iteminfo_parts::GUNMOD_BLACKLIST_MOD ) ) {
            temp1.str( "" );
            temp1 << _( "<bold>Incompatible with mod location: </bold> " );
            int iternum = 0;
            for( const auto &black : mod.blacklist_mod ) {
                if( iternum != 0 ) {
                    temp1 << ", ";
                }
                temp1 << black.name();
                iternum++;
            }
            temp1 << ".";
            info.push_back( iteminfo( "GUNMOD", temp1.str() ) );
        }

    }
    if( is_armor() ) {
        body_part_set covered_parts = get_covered_body_parts();
        bool covers_anything = covered_parts.any();

        if( parts->test( iteminfo_parts::ARMOR_BODYPARTS ) ) {
            temp1.str( "" );
            temp1 << _( "Covers: " );
            if( covers( bp_head ) ) {
                temp1 << _( "The <info>head</info>. " );
            }
            if( covers( bp_eyes ) ) {
                temp1 << _( "The <info>eyes</info>. " );
            }
            if( covers( bp_mouth ) ) {
                temp1 << _( "The <info>mouth</info>. " );
            }
            if( covers( bp_torso ) ) {
                temp1 << _( "The <info>torso</info>. " );
            }

            if( is_sided() && ( covers( bp_arm_l ) || covers( bp_arm_r ) ) ) {
                temp1 << _( "Either <info>arm</info>. " );
            } else if( covers( bp_arm_l ) && covers( bp_arm_r ) ) {
                temp1 << _( "The <info>arms</info>. " );
            } else if( covers( bp_arm_l ) ) {
                temp1 << _( "The <info>left arm</info>. " );
            } else if( covers( bp_arm_r ) ) {
                temp1 << _( "The <info>right arm</info>. " );
            }

            if( is_sided() && ( covers( bp_hand_l ) || covers( bp_hand_r ) ) ) {
                temp1 << _( "Either <info>hand</info>. " );
            } else if( covers( bp_hand_l ) && covers( bp_hand_r ) ) {
                temp1 << _( "The <info>hands</info>. " );
            } else if( covers( bp_hand_l ) ) {
                temp1 << _( "The <info>left hand</info>. " );
            } else if( covers( bp_hand_r ) ) {
                temp1 << _( "The <info>right hand</info>. " );
            }

            if( is_sided() && ( covers( bp_leg_l ) || covers( bp_leg_r ) ) ) {
                temp1 << _( "Either <info>leg</info>. " );
            } else if( covers( bp_leg_l ) && covers( bp_leg_r ) ) {
                temp1 << _( "The <info>legs</info>. " );
            } else if( covers( bp_leg_l ) ) {
                temp1 << _( "The <info>left leg</info>. " );
            } else if( covers( bp_leg_r ) ) {
                temp1 << _( "The <info>right leg</info>. " );
            }

            if( is_sided() && ( covers( bp_foot_l ) || covers( bp_foot_r ) ) ) {
                temp1 << _( "Either <info>foot</info>. " );
            } else if( covers( bp_foot_l ) && covers( bp_foot_r ) ) {
                temp1 << _( "The <info>feet</info>. " );
            } else if( covers( bp_foot_l ) ) {
                temp1 << _( "The <info>left foot</info>. " );
            } else if( covers( bp_foot_r ) ) {
                temp1 << _( "The <info>right foot</info>. " );
            }

            if( !covers_anything ) {
                temp1 << _( "<info>Nothing</info>." );
            }

            info.push_back( iteminfo( "ARMOR", temp1.str() ) );
        }

        if( parts->test( iteminfo_parts::ARMOR_LAYER ) && covers_anything ) {
            temp1.str( "" );
            temp1 << _( "Layer: " );
            if( has_flag( "SKINTIGHT" ) ) {
                temp1 << _( "<stat>Close to skin</stat>. " );
            } else if( has_flag( "BELTED" ) ) {
                temp1 << _( "<stat>Strapped</stat>. " );
            } else if( has_flag( "OUTER" ) ) {
                temp1 << _( "<stat>Outer</stat>. " );
            } else if( has_flag( "WAIST" ) ) {
                temp1 << _( "<stat>Waist</stat>. " );
            } else {
                temp1 << _( "<stat>Normal</stat>. " );
            }

            info.push_back( iteminfo( "ARMOR", temp1.str() ) );
        }

        if( parts->test( iteminfo_parts::ARMOR_COVERAGE ) && covers_anything ) {
            info.push_back( iteminfo( "ARMOR", _( "Coverage: " ), "<num>%",
                                      iteminfo::no_newline, get_coverage() ) );
        }
        if( parts->test( iteminfo_parts::ARMOR_WARMTH ) && covers_anything ) {
            info.push_back( iteminfo( "ARMOR", space + _( "Warmth: " ), get_warmth() ) );
        }

        insert_separation_line();

        if( parts->test( iteminfo_parts::ARMOR_ENCUMBRANCE ) && covers_anything ) {
            int encumbrance = get_encumber( g->u );
            std::string format;
            if( has_flag( "FIT" ) ) {
                format = _( "<num> <info>(fits)</info>" );
            } else if( has_flag( "VARSIZE" ) && encumbrance ) {
                format = _( "<num> <bad>(poor fit)</bad>" );
            }
            info.push_back( iteminfo( "ARMOR", _( "<bold>Encumbrance</bold>: " ), format,
                                      iteminfo::no_newline | iteminfo::lower_is_better,
                                      encumbrance ) );
            if( !type->rigid ) {
                const auto encumbrance_when_full =
                    get_encumber_when_containing( g->u, get_total_capacity() );
                info.push_back( iteminfo( "ARMOR", space + _( "Encumbrance when full: " ), "",
                                          iteminfo::no_newline | iteminfo::lower_is_better,
                                          encumbrance_when_full ) );
            }
        }

        int converted_storage_scale = 0;
        const double converted_storage = round_up( convert_volume( get_storage().value(),
                                         &converted_storage_scale ), 2 );
        if( parts->test( iteminfo_parts::ARMOR_STORAGE ) && converted_storage > 0 ) {
            auto f = converted_storage_scale == 0 ? iteminfo::no_flags : iteminfo::is_decimal;
            info.push_back( iteminfo( "ARMOR", space + _( "Storage: " ),
                                      string_format( "<num> %s", volume_units_abbr() ),
                                      f, converted_storage ) );
        }

        // Whatever the last entry was, we want a newline at this point
        info.back().bNewLine = true;

        if( parts->test( iteminfo_parts::ARMOR_PROTECTION ) && covers_anything ) {
            info.push_back( iteminfo( "ARMOR", _( "<bold>Protection</bold>: Bash: " ), "",
                                      iteminfo::no_newline, bash_resist() ) );
            info.push_back( iteminfo( "ARMOR", space + _( "Cut: " ),
                                      cut_resist() ) );
            info.push_back( iteminfo( "ARMOR", space + _( "Acid: " ), "",
                                      iteminfo::no_newline, acid_resist() ) );
            info.push_back( iteminfo( "ARMOR", space + _( "Fire: " ), "",
                                      iteminfo::no_newline, fire_resist() ) );
            info.push_back( iteminfo( "ARMOR", space + _( "Environmental: " ),
                                      get_base_env_resist( *this ) ) );
            if( type->can_use( "GASMASK" ) ) {
                info.push_back( iteminfo( "ARMOR",
                                          _( "<bold>Protection when active</bold>: " ) ) );
                info.push_back( iteminfo( "ARMOR", space + _( "Acid: " ), "",
                                          iteminfo::no_newline,
                                          acid_resist( false, get_base_env_resist_w_filter() ) ) );
                info.push_back( iteminfo( "ARMOR", space + _( "Fire: " ), "",
                                          iteminfo::no_newline,
                                          fire_resist( false, get_base_env_resist_w_filter() ) ) );
                info.push_back( iteminfo( "ARMOR", space + _( "Environmental: " ),
                                          get_env_resist( get_base_env_resist_w_filter() ) ) );
            }

            if( damage() > 0 ) {
                info.push_back( iteminfo( "ARMOR",
                                          _( "Protection values are <bad>reduced by "
                                             "damage</bad> and you may be able to "
                                             "<info>improve them by repairing this "
                                             "item</info>." ) ) );
            }
        }

    }
    if( is_book() ) {
        insert_separation_line();
        const auto &book = *type->book;
        // Some things about a book you CAN tell by it's cover.
        if( !book.skill && !type->can_use( "MA_MANUAL" ) && parts->test( iteminfo_parts::BOOK_SUMMARY ) ) {
            info.push_back( iteminfo( "BOOK", _( "Just for fun." ) ) );
        }
        if( type->can_use( "MA_MANUAL" ) && parts->test( iteminfo_parts::BOOK_SUMMARY ) ) {
            info.push_back( iteminfo( "BOOK",
                                      _( "Some sort of <info>martial arts training manual</info>." ) ) );
        }
        if( book.req == 0 && parts->test( iteminfo_parts::BOOK_REQUIREMENTS_BEGINNER ) ) {
            info.push_back( iteminfo( "BOOK", _( "It can be <info>understood by beginners</info>." ) ) );
        }
        if( g->u.has_identified( typeId() ) ) {
            if( book.skill ) {
                if( g->u.get_skill_level_object( book.skill ).can_train() &&
                    parts->test( iteminfo_parts::BOOK_SKILLRANGE_MAX ) ) {
                    auto fmt = string_format(
                                   _( "Can bring your <info>%s skill to</info> <num>" ),
                                   book.skill.obj().name() );
                    info.push_back( iteminfo( "BOOK", "", fmt, iteminfo::no_flags, book.level ) );
                }

                if( book.req != 0 && parts->test( iteminfo_parts::BOOK_SKILLRANGE_MIN ) ) {
                    auto fmt = string_format(
                                   _( "<info>Requires %s level</info> <num> to understand." ),
                                   book.skill.obj().name() );
                    info.push_back( iteminfo( "BOOK", "", fmt,
                                              iteminfo::lower_is_better, book.req ) );
                }
            }

            if( book.intel != 0 && parts->test( iteminfo_parts::BOOK_REQUIREMENTS_INT ) ) {
                info.push_back( iteminfo( "BOOK", "",
                                          _( "Requires <info>intelligence of</info> <num> to easily read." ),
                                          iteminfo::lower_is_better, book.intel ) );
            }
            if( g->u.book_fun_for( *this, g->u ) != 0 && parts->test( iteminfo_parts::BOOK_MORALECHANGE ) ) {
                info.push_back( iteminfo( "BOOK", "",
                                          _( "Reading this book affects your morale by <num>" ),
                                          iteminfo::show_plus, g->u.book_fun_for( *this, g->u ) ) );
            }
            if( parts->test( iteminfo_parts::BOOK_TIMEPERCHAPTER ) ) {
                auto fmt = ngettext(
                               "A chapter of this book takes <num> <info>minute to read</info>.",
                               "A chapter of this book takes <num> <info>minutes to read</info>.",
                               book.time );
                info.push_back( iteminfo( "BOOK", "", fmt,
                                          iteminfo::lower_is_better, book.time ) );
            }

            if( book.chapters > 0 && parts->test( iteminfo_parts::BOOK_NUMUNREADCHAPTERS ) ) {
                const int unread = get_remaining_chapters( g->u );
                auto fmt = ngettext( "This book has <num> <info>unread chapter</info>.",
                                     "This book has <num> <info>unread chapters</info>.",
                                     unread );
                info.push_back( iteminfo( "BOOK", "", fmt, iteminfo::no_flags, unread ) );
            }

            std::vector<std::string> recipe_list;
            for( const auto &elem : book.recipes ) {
                const bool knows_it = g->u.knows_recipe( elem.recipe );
                const bool can_learn = g->u.get_skill_level( elem.recipe->skill_used )  >= elem.skill_level;
                // If the player knows it, they recognize it even if it's not clearly stated.
                if( elem.is_hidden() && !knows_it ) {
                    continue;
                }
                if( knows_it ) {
                    // In case the recipe is known, but has a different name in the book, use the
                    // real name to avoid confusing the player.
                    const std::string name = elem.recipe->result_name();
                    recipe_list.push_back( "<bold>" + name + "</bold>" );
                } else if( !can_learn ) {
                    recipe_list.push_back( "<color_brown>" + elem.name + "</color>" );
                } else {
                    recipe_list.push_back( "<dark>" + elem.name + "</dark>" );
                }
            }

            if( !recipe_list.empty() && parts->test( iteminfo_parts::DESCRIPTION_BOOK_RECIPES ) ) {
                std::string recipe_line =
                    string_format( ngettext( "This book contains %1$d crafting recipe: %2$s",
                                             "This book contains %1$d crafting recipes: %2$s",
                                             recipe_list.size() ),
                                   recipe_list.size(), enumerate_as_string( recipe_list ) );

                insert_separation_line();
                info.push_back( iteminfo( "DESCRIPTION", recipe_line ) );
            }

            if( recipe_list.size() != book.recipes.size() &&
                parts->test( iteminfo_parts::DESCRIPTION_BOOK_ADDITIONAL_RECIPES ) ) {
                info.push_back( iteminfo(
                                    "DESCRIPTION",
                                    _( "It might help you figuring out some <good>more recipes</good>." ) ) );
            }

        } else {
            if( parts->test( iteminfo_parts::BOOK_UNREAD ) )
                info.push_back( iteminfo(
                                    "BOOK",
                                    _( "You need to <info>read this book to see its contents</info>." ) ) );
        }

    }
    if( is_container() && parts->test( iteminfo_parts::CONTAINER_DETAILS ) ) {
        insert_separation_line();
        const auto &c = *type->container;

        temp1.str( "" );
        temp1 << _( "This container " );

        if( c.seals ) {
            temp1 << _( "can be <info>resealed</info>, " );
        }
        if( c.watertight ) {
            temp1 << _( "is <info>watertight</info>, " );
        }
        if( c.preserves ) {
            temp1 << _( "<good>preserves spoiling</good>, " );
        }

        temp1 << string_format( _( "can store <info>%s %s</info>." ),
                                format_volume( c.contains ).c_str(),
                                volume_units_long() );

        info.push_back( iteminfo( "CONTAINER", temp1.str() ) );
    }

    if( is_tool() ) {
        insert_separation_line();
        if( ammo_capacity() != 0 && parts->test( iteminfo_parts::TOOL_CHARGES ) ) {
            info.emplace_back( "TOOL", string_format( _( "<bold>Charges</bold>: %d" ), ammo_remaining() ) );
        }

        if( !magazine_integral() ) {
            if( magazine_current() && parts->test( iteminfo_parts::TOOL_MAGAZINE_CURRENT ) ) {
                info.emplace_back( "TOOL", _( "Magazine: " ), string_format( "<stat>%s</stat>",
                                   magazine_current()->tname().c_str() ) );
            }

            if( parts->test( iteminfo_parts::TOOL_MAGAZINE_COMPATIBLE ) ) {
                insert_separation_line();
                const auto compat = magazine_compatible();
                info.emplace_back( "TOOL", _( "<bold>Compatible magazines:</bold> " ),
                enumerate_as_string( compat.begin(), compat.end(), []( const itype_id & id ) {
                    return item_controller->find_template( id )->nname( 1 );
                } ) );
            }
        } else if( ammo_capacity() != 0 && parts->test( iteminfo_parts::TOOL_CAPACITY ) ) {
            std::string tmp;
            if( ammo_type() ) {
                //~ "%s" is ammunition type. This types can't be plural.
                tmp = ngettext( "Maximum <num> charge of %s.", "Maximum <num> charges of %s.", ammo_capacity() );
                tmp = string_format( tmp, ammo_type()->name().c_str() );
            } else {
                tmp = ngettext( "Maximum <num> charge.", "Maximum <num> charges.", ammo_capacity() );
            }
            info.emplace_back( "TOOL", "", tmp, iteminfo::no_flags, ammo_capacity() );
        }
    }

    if( !components.empty() && parts->test( iteminfo_parts::DESCRIPTION_COMPONENTS_MADEFROM ) ) {
        info.push_back( iteminfo( "DESCRIPTION", string_format( _( "Made from: %s" ),
                                  _( components_to_string().c_str() ) ) ) );
    } else {
        const auto &dis = recipe_dictionary::get_uncraft( typeId() );
        const auto &req = dis.disassembly_requirements();
        if( !req.is_empty() && parts->test( iteminfo_parts::DESCRIPTION_COMPONENTS_DISASSEMBLE ) ) {
            const auto &components = req.get_components();
            const std::string components_list = enumerate_as_string( components.begin(), components.end(),
            []( const std::vector<item_comp> &comps ) {
                return comps.front().to_string();
            } );

            insert_separation_line();
            info.push_back( iteminfo( "DESCRIPTION",
                                      string_format( _( "Disassembling this item takes %s and might yield: %s." ),
                                              to_string_approx( time_duration::from_turns( dis.time / 100 ) ), components_list.c_str() ) ) );
        }
    }

    auto name_quality = [&info]( const std::pair<quality_id, int> &q ) {
        std::string str;
        if( q.first == quality_jack || q.first == quality_lift ) {
            str = string_format(
                      _( "Has level <info>%1$d %2$s</info> quality and is rated at <info>%3$d</info> %4$s" ),
                      q.second, q.first.obj().name.c_str(),
                      static_cast<int>( convert_weight( q.second * TOOL_LIFT_FACTOR ) ),
                      weight_units() );
        } else {
            str = string_format( _( "Has level <info>%1$d %2$s</info> quality." ),
                                 q.second, q.first.obj().name.c_str() );
        }
        info.emplace_back( "QUALITIES", "", str );
    };

    if( parts->test( iteminfo_parts::QUALITIES ) ) {
        for( const auto &q : type->qualities ) {
            name_quality( q );
        }
    }

    if( parts->test( iteminfo_parts::QUALITIES_CONTAINED ) &&
    std::any_of( contents.begin(), contents.end(), []( const item & e ) {
    return !e.type->qualities.empty();
    } ) ) {

        info.emplace_back( "QUALITIES", "", _( "Contains items with qualities:" ) );
        std::map<quality_id, int> most_quality;
        for( const auto &e : contents ) {
            for( const auto &q : e.type->qualities ) {
                auto emplace_result = most_quality.emplace( q );
                if( !emplace_result.second && most_quality.at( emplace_result.first->first ) < q.second ) {
                    most_quality[ q.first ] = q.second;
                }
            }
        }
        for( const auto &q : most_quality ) {
            name_quality( q );
        }
    }

    if( !is_null() ) {
        if( parts->test( iteminfo_parts::DESCRIPTION ) ) {
            insert_separation_line();
            const std::map<std::string, std::string>::const_iterator idescription =
                item_vars.find( "description" );
            if( !type->snippet_category.empty() ) {
                // Just use the dynamic description
                info.push_back( iteminfo( "DESCRIPTION", SNIPPET.get( note ) ) );
            } else if( idescription != item_vars.end() ) {
                info.push_back( iteminfo( "DESCRIPTION", idescription->second ) );
            } else {
                info.push_back( iteminfo( "DESCRIPTION", _( type->description.c_str() ) ) );
            }
        }
        auto all_techniques = type->techniques;
        all_techniques.insert( techniques.begin(), techniques.end() );
        if( !all_techniques.empty() && parts->test( iteminfo_parts::DESCRIPTION_TECHNIQUES ) ) {
            insert_separation_line();
            info.push_back( iteminfo( "DESCRIPTION", _( "<bold>Techniques when wielded</bold>: " ) +
            enumerate_as_string( all_techniques.begin(), all_techniques.end(), []( const matec_id & tid ) {
                return string_format( "<stat>%s:</stat> <info>%s</info>", _( tid.obj().name.c_str() ),
                                      _( tid.obj().description.c_str() ) );
            } ) ) );
        }

        if( !is_gunmod() && has_flag( "REACH_ATTACK" ) &&
            parts->test( iteminfo_parts::DESCRIPTION_GUNMOD_ADDREACHATTACK ) ) {
            insert_separation_line();
            if( has_flag( "REACH3" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This item can be used to make <stat>long reach attacks</stat>." ) ) );
            } else {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This item can be used to make <stat>reach attacks</stat>." ) ) );
            }
        }

        ///\EFFECT_MELEE >2 allows seeing melee damage stats on weapons
        if( debug_mode || ( g->u.get_skill_level( skill_melee ) > 2 && ( damage_melee( DT_BASH ) > 0 ||
                            damage_melee( DT_CUT ) > 0 || damage_melee( DT_STAB ) > 0 || type->m_to_hit > 0 ) ) ) {
            damage_instance non_crit;
            g->u.roll_all_damage( false, non_crit, true, *this );
            damage_instance crit;
            g->u.roll_all_damage( true, crit, true, *this );
            int attack_cost = g->u.attack_speed( *this );
            insert_separation_line();
            if( parts->test( iteminfo_parts::DESCRIPTION_MELEEDMG ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          string_format( _( "<bold>Average melee damage:</bold>" ) ) ) );
            }
            if( parts->test( iteminfo_parts::DESCRIPTION_MELEEDMG_CRIT ) )
                info.push_back( iteminfo( "DESCRIPTION",
                                          string_format( _( "Critical hit chance %d%% - %d%%" ),
                                                  int( g->u.crit_chance( 0, 100, *this ) * 100 ),
                                                  int( g->u.crit_chance( 100, 0, *this ) * 100 ) ) ) );
            if( parts->test( iteminfo_parts::DESCRIPTION_MELEEDMG_BASH ) )
                info.push_back( iteminfo( "DESCRIPTION",
                                          string_format( _( "%d bashing (%d on a critical hit)" ),
                                                  int( non_crit.type_damage( DT_BASH ) ),
                                                  int( crit.type_damage( DT_BASH ) ) ) ) );
            if( ( non_crit.type_damage( DT_CUT ) > 0.0f || crit.type_damage( DT_CUT ) > 0.0f )
                && parts->test( iteminfo_parts::DESCRIPTION_MELEEDMG_CUT ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          string_format( _( "%d cutting (%d on a critical hit)" ),
                                                  int( non_crit.type_damage( DT_CUT ) ),
                                                  int( crit.type_damage( DT_CUT ) ) ) ) );
            }
            if( ( non_crit.type_damage( DT_STAB ) > 0.0f || crit.type_damage( DT_STAB ) > 0.0f )
                && parts->test( iteminfo_parts::DESCRIPTION_MELEEDMG_PIERCE ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          string_format( _( "%d piercing (%d on a critical hit)" ),
                                                  int( non_crit.type_damage( DT_STAB ) ),
                                                  int( crit.type_damage( DT_STAB ) ) ) ) );
            }
            if( parts->test( iteminfo_parts::DESCRIPTION_MELEEDMG_MOVES ) )
                info.push_back( iteminfo( "DESCRIPTION",
                                          string_format( _( "%d moves per attack" ), attack_cost ) ) );
        }

        //lets display which martial arts styles character can use with this weapon
        if( parts->test( iteminfo_parts::DESCRIPTION_APPLICABLEMARTIALARTS ) ) {
            const auto &styles = g->u.ma_styles;
            const std::string valid_styles = enumerate_as_string( styles.begin(), styles.end(),
            [this]( const matype_id & mid ) {
                return mid.obj().has_weapon( typeId() ) ? _( mid.obj().name.c_str() ) : std::string();
            } );
            if( !valid_styles.empty() ) {
                insert_separation_line();
                info.push_back( iteminfo( "DESCRIPTION",
                                          std::string( _( "You know how to use this with these martial arts styles: " ) ) +
                                          valid_styles ) );
            }
        }

        if( parts->test( iteminfo_parts::DESCRIPTION_USE_METHODS ) ) {
            for( const auto &method : type->use_methods ) {
                insert_separation_line();
                method.second.dump_info( *this, info );
            }
        }

        if( parts->test( iteminfo_parts::DESCRIPTION_REPAIREDWITH ) ) {
            insert_separation_line();

            const auto &rep = repaired_with();

            if( !rep.empty() ) {
                info.emplace_back( "DESCRIPTION", _( "<bold>Repaired with</bold>: " ) +
                enumerate_as_string( rep.begin(), rep.end(), []( const itype_id & e ) {
                    return item::find_type( e )->nname( 1 );
                }, enumeration_conjunction::or_ ) );
                insert_separation_line();

            } else {
                info.emplace_back( "DESCRIPTION", _( "* This item is <bad>not repairable</bad>." ) );
            }
        }

        if( parts->test( iteminfo_parts::DESCRIPTION_CONDUCTIVITY ) ) {
            if( !conductive() ) {
                info.push_back( iteminfo( "BASE",
                                          string_format( _( "* This item <good>does not conduct</good> electricity." ) ) ) );
            } else if( has_flag( "CONDUCTIVE" ) ) {
                info.push_back( iteminfo( "BASE",
                                          string_format(
                                              _( "* This item effectively <bad>conducts</bad> electricity, as it has no guard." ) ) ) );
            } else {
                info.push_back( iteminfo( "BASE",
                                          string_format( _( "* This item <bad>conducts</bad> electricity." ) ) ) );
            }
        }

        bool anyFlags = ( *parts & iteminfo_query::anyflags ).any();
        if( anyFlags ) {
            insert_separation_line();
        }

        if( parts->test( iteminfo_parts::DESCRIPTION_FLAGS ) ) {
            // concatenate base and acquired flags...
            std::vector<std::string> flags;
            std::set_union( type->item_tags.begin(), type->item_tags.end(),
                            item_tags.begin(), item_tags.end(),
                            std::back_inserter( flags ) );

            // ...and display those which have an info description
            for( const auto &e : flags ) {
                auto &f = json_flag::get( e );
                if( !f.info().empty() ) {
                    info.emplace_back( "DESCRIPTION", string_format( "* %s", _( f.info().c_str() ) ) );
                }
            }
        }

        if( is_armor() ) {
            if( has_flag( "HELMET_COMPAT" ) && parts->test( iteminfo_parts::DESCRIPTION_FLAGS_HELMETCOMPAT ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This item can be <info>worn with a helmet</info>." ) ) );
            }
            const bool little = g->u.has_trait( trait_id( "SMALL2" ) ) ||
                                g->u.has_trait( trait_id( "SMALL_OK" ) );
            if( has_flag( "FIT" ) && parts->test( iteminfo_parts::DESCRIPTION_FLAGS_FITS ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This piece of clothing <info>fits</info> you perfectly." ) ) );
            } else if( has_flag( "VARSIZE" ) && parts->test( iteminfo_parts::DESCRIPTION_FLAGS_VARSIZE ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This piece of clothing <info>can be refitted</info>." ) ) );
            }
            if( little && get_encumber( g->u ) ) {
                if( !has_flag( "UNDERSIZE" ) ) {
                    info.push_back( iteminfo( "DESCRIPTION",
                                              _( "* These clothes are <bad>too large</bad> but <info>can be undersized</info>." ) ) );
                } else {
                    info.push_back( iteminfo( "DESCRIPTION",
                                              _( "* These clothes are <good>undersized</good> enough to accommodate <info>abnormally small mutated anatomy</info>." ) ) );
                }
            } else if( has_flag( "UNDERSIZE" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* These clothes are <bad>undersized</bad> but <info>can be refitted</info>." ) ) );
            }
            if( is_sided() && parts->test( iteminfo_parts::DESCRIPTION_FLAGS_SIDED ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This item can be worn on <info>either side</info> of the body." ) ) );
            }
            if( is_power_armor() && parts->test( iteminfo_parts::DESCRIPTION_FLAGS_POWERARMOR ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This gear is a part of power armor." ) ) );
                if( parts->test( iteminfo_parts::DESCRIPTION_FLAGS_POWERARMOR_RADIATIONHINT ) ) {
                    if( covers( bp_head ) ) {
                        info.push_back( iteminfo( "DESCRIPTION",
                                                  _( "* When worn with a power armor suit, it will <good>fully protect</good> you from <info>radiation</info>." ) ) );
                    } else {
                        info.push_back( iteminfo( "DESCRIPTION",
                                                  _( "* When worn with a power armor helmet, it will <good>fully protect</good> you from <info>radiation</info>." ) ) );
                    }
                }
            }
            if( typeId() == "rad_badge" && parts->test( iteminfo_parts::DESCRIPTION_IRRIDATION ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          string_format( _( "* The film strip on the badge is %s." ),
                                                  rad_badge_color( irridation ).c_str() ) ) );
            }
        }

        if( is_tool() ) {
            if( has_flag( "USE_UPS" ) && parts->test( iteminfo_parts::DESCRIPTION_RECHARGE_UPSMODDED ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This tool has been modified to use a <info>universal power supply</info> and is <neutral>not compatible</neutral> with <info>standard batteries</info>." ) ) );
            } else if( has_flag( "RECHARGE" ) && has_flag( "NO_RELOAD" ) &&
                       parts->test( iteminfo_parts::DESCRIPTION_RECHARGE_NORELOAD ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This tool has a <info>rechargeable power cell</info> and is <neutral>not compatible</neutral> with <info>standard batteries</info>." ) ) );
            } else if( has_flag( "RECHARGE" ) &&
                       parts->test( iteminfo_parts::DESCRIPTION_RECHARGE_UPSCAPABLE ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This tool has a <info>rechargeable power cell</info> and can be recharged in any <neutral>UPS-compatible recharging station</neutral>. You could charge it with <info>standard batteries</info>, but unloading it is impossible." ) ) );
            }
        }

        if( has_flag( "RADIO_ACTIVATION" ) &&
            parts->test( iteminfo_parts::DESCRIPTION_RADIO_ACTIVATION ) ) {
            if( has_flag( "RADIO_MOD" ) ) {
                info.emplace_back( "DESCRIPTION",
                                   _( "* This item has been modified to listen to <info>radio signals</info>.  It can still be activated manually." ) );
            } else {
                info.emplace_back( "DESCRIPTION",
                                   _( "* This item can only be activated by a <info>radio signal</info>." ) );
            }

            std::string signame;
            if( has_flag( "RADIOSIGNAL_1" ) ) {
                signame = "<color_c_red>red</color> radio signal.";
            } else if( has_flag( "RADIOSIGNAL_2" ) ) {
                signame = "<color_c_blue>blue</color> radio signal.";
            } else if( has_flag( "RADIOSIGNAL_3" ) ) {
                signame = "<color_c_green>green</color> radio signal.";
            }
            if( parts->test( iteminfo_parts::DESCRIPTION_RADIO_ACTIVATION_CHANNEL ) ) {
                info.emplace_back( "DESCRIPTION", string_format( _( "* It will be activated by the %s." ),
                                   signame.c_str() ) );
            }

            if( has_flag( "RADIO_INVOKE_PROC" ) &&
                parts->test( iteminfo_parts::DESCRIPTION_RADIO_ACTIVATION_PROC ) ) {
                info.emplace_back( "DESCRIPTION",
                                   _( "* Activating this item with a <info>radio signal</info> will <neutral>detonate</neutral> it immediately." ) );
            }
        }

        // @todo: Unhide when enforcing limits
        if( is_bionic() && get_option < bool >( "CBM_SLOTS_ENABLED" )
            && parts->test( iteminfo_parts::DESCRIPTION_CBM_SLOTS ) ) {
            info.push_back( iteminfo( "DESCRIPTION", list_occupied_bps( type->bionic->id,
                                      _( "This bionic is installed in the following body part(s):" ) ) ) );
        }

        if( is_gun() && has_flag( "FIRE_TWOHAND" ) &&
            parts->test( iteminfo_parts::DESCRIPTION_TWOHANDED ) ) {
            info.push_back( iteminfo( "DESCRIPTION",
                                      _( "* This weapon needs <info>two free hands</info> to fire." ) ) );
        }

        if( is_gunmod() && has_flag( "DISABLE_SIGHTS" ) &&
            parts->test( iteminfo_parts::DESCRIPTION_GUNMOD_DISABLESSIGHTS ) ) {
            info.push_back( iteminfo( "DESCRIPTION",
                                      _( "* This mod <bad>obscures sights</bad> of the base weapon." ) ) );
        }

        if( is_gunmod() && has_flag( "CONSUMABLE" ) &&
            parts->test( iteminfo_parts::DESCRIPTION_GUNMOD_CONSUMABLE ) ) {
            info.push_back( iteminfo( "DESCRIPTION",
                                      _( "* This mod might <bad>suffer wear</bad> when firing the base weapon." ) ) );
        }

        if( has_flag( "LEAK_DAM" ) && has_flag( "RADIOACTIVE" ) && damage() > 0
            && parts->test( iteminfo_parts::DESCRIPTION_RADIOACTIVITY_DAMAGED ) ) {
            info.push_back( iteminfo( "DESCRIPTION",
                                      _( "* The casing of this item has <neutral>cracked</neutral>, revealing an <info>ominous green glow</info>." ) ) );
        }

        if( has_flag( "LEAK_ALWAYS" ) && has_flag( "RADIOACTIVE" ) &&
            parts->test( iteminfo_parts::DESCRIPTION_RADIOACTIVITY_ALWAYS ) ) {
            info.push_back( iteminfo( "DESCRIPTION",
                                      _( "* This object is <neutral>surrounded</neutral> by a <info>sickly green glow</info>." ) ) );
        }

        if( is_brewable() || ( !contents.empty() && contents.front().is_brewable() ) ) {
            const item &brewed = !is_brewable() ? contents.front() : *this;
            if( parts->test( iteminfo_parts::DESCRIPTION_BREWABLE_DURATION ) ) {
                const time_duration btime = brewed.brewing_time();
                if( btime <= 2_days ) {
                    info.push_back( iteminfo( "DESCRIPTION",
                                              string_format( ngettext( "* Once set in a vat, this will ferment in around %d hour.",
                                                      "* Once set in a vat, this will ferment in around %d hours.", to_hours<int>( btime ) ),
                                                      to_hours<int>( btime ) ) ) );
                } else {
                    info.push_back( iteminfo( "DESCRIPTION",
                                              string_format( ngettext( "* Once set in a vat, this will ferment in around %d day.",
                                                      "* Once set in a vat, this will ferment in around %d days.", to_days<int>( btime ) ),
                                                      to_days<int>( btime ) ) ) );
                }
            }

            if( parts->test( iteminfo_parts::DESCRIPTION_BREWABLE_PRODUCTS ) ) {
                for( const auto &res : brewed.brewing_results() ) {
                    info.push_back( iteminfo( "DESCRIPTION",
                                              string_format( _( "* Fermenting this will produce <neutral>%s</neutral>." ),
                                                      nname( res, brewed.charges ).c_str() ) ) );
                }
            }
        }

        if( parts->test( iteminfo_parts::DESCRIPTION_FAULTS ) ) {
            for( const auto &e : faults ) {
                //~ %1$s is the name of a fault and %2$s is the description of the fault
                info.emplace_back( "DESCRIPTION", string_format( _( "* <bad>Faulty %1$s</bad>.  %2$s" ),
                                   e.obj().name().c_str(), e.obj().description().c_str() ) );
            }
        }

        // does the item fit in any holsters?
        auto holsters = Item_factory::find( [this]( const itype & e ) {
            if( !e.can_use( "holster" ) ) {
                return false;
            }
            auto ptr = dynamic_cast<const holster_actor *>( e.get_use( "holster" )->get_actor_ptr() );
            return ptr->can_holster( *this );
        } );

        if( !holsters.empty() && parts->test( iteminfo_parts::DESCRIPTION_HOLSTERS ) ) {
            insert_separation_line();
            info.emplace_back( "DESCRIPTION", _( "<bold>Can be stored in:</bold> " ) +
                               enumerate_as_string( holsters.begin(), holsters.end(),
            []( const itype * e ) {
                return e->nname( 1 );
            } ) );
        }

        if( parts->test( iteminfo_parts::DESCRIPTION_ACTIVATABLE_TRANSFORMATION ) ) {
            for( auto &u : type->use_methods ) {
                const auto tt = dynamic_cast<const delayed_transform_iuse *>( u.second.get_actor_ptr() );
                if( tt == nullptr ) {
                    continue;
                }
                const int time_to_do = tt->time_to_do( *this );
                if( time_to_do <= 0 ) {
                    info.push_back( iteminfo( "DESCRIPTION", _( "It's done and <info>can be activated</info>." ) ) );
                } else {
                    const auto time = to_string_clipped( time_duration::from_turns( time_to_do ) );
                    info.push_back( iteminfo( "DESCRIPTION", string_format( _( "It will be done in %s." ),
                                              time.c_str() ) ) );
                }
            }
        }

        std::map<std::string, std::string>::const_iterator item_note = item_vars.find( "item_note" );
        std::map<std::string, std::string>::const_iterator item_note_type =
            item_vars.find( "item_note_type" );

        if( item_note != item_vars.end() && parts->test( iteminfo_parts::DESCRIPTION_NOTES ) ) {
            insert_separation_line();
            std::string ntext;
            if( item_note_type != item_vars.end() ) {
                ntext += string_format( _( "%1$s on the %2$s is: " ),
                                        item_note_type->second.c_str(), tname().c_str() );
            } else {
                ntext += _( "Note: " );
            }
            info.push_back( iteminfo( "DESCRIPTION", ntext + item_note->second ) );
        }

        // describe contents
        if( !contents.empty() && parts->test( iteminfo_parts::DESCRIPTION_CONTENTS ) ) {
            for( const auto mod : is_gun() ? gunmods() : toolmods() ) {
                if( mod->type->gunmod ) {
                    temp1.str( "" );
                    if( mod->is_irremovable() ) {
                        temp1 << _( "Integrated mod: " );
                    } else {
                        temp1 << _( "Mod: " );
                    }
                    temp1 << "<bold>" << mod->tname() << "</bold> (" << mod->type->gunmod->location.name() << ")";
                }
                insert_separation_line();
                info.emplace_back( "DESCRIPTION", temp1.str() );
                info.emplace_back( "DESCRIPTION", _( mod->type->description.c_str() ) );
            }
            bool contents_header = false;
            for( const auto &contents_item : contents ) {
                if( !contents_item.type->mod ) {
                    if( !contents_header ) {
                        insert_separation_line();
                        info.emplace_back( "DESCRIPTION", _( "<bold>Contents of this item</bold>:" ) );
                        contents_header = true;
                    } else {
                        // Separate items with a blank line
                        info.emplace_back( "DESCRIPTION", space );
                    }

                    const auto description = _( contents_item.type->description.c_str() );

                    if( contents_item.made_of_from_type( LIQUID ) ) {
                        auto contents_volume = contents_item.volume() * batch;
                        int converted_volume_scale = 0;
                        const double converted_volume =
                            round_up( convert_volume( contents_volume.value(),
                                                      &converted_volume_scale ), 2 );
                        info.emplace_back( "DESCRIPTION", contents_item.display_name() );
                        auto f = iteminfo::no_newline;
                        if( converted_volume_scale != 0 ) {
                            f |= iteminfo::is_decimal;
                        }
                        info.emplace_back( "CONTAINER", description + space,
                                           string_format( "<num> %s", volume_units_abbr() ), f,
                                           converted_volume );
                    } else {
                        info.emplace_back( "DESCRIPTION", contents_item.display_name() );
                        info.emplace_back( "DESCRIPTION", description );
                    }
                }
            }
        }

        // list recipes you could use it in
        itype_id tid;
        if( contents.empty() ) { // use this item
            tid = typeId();
        } else { // use the contained item
            tid = contents.front().typeId();
        }
        const auto &known_recipes = g->u.get_learned_recipes().of_component( tid );
        if( !known_recipes.empty() && parts->test( iteminfo_parts::DESCRIPTION_APPLICABLE_RECIPES ) ) {
            temp1.str( "" );
            const inventory &inv = g->u.crafting_inventory();

            if( known_recipes.size() > 24 ) {
                insert_separation_line();
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "You know dozens of things you could craft with it." ) ) );
            } else if( known_recipes.size() > 12 ) {
                insert_separation_line();
                info.push_back( iteminfo( "DESCRIPTION", _( "You could use it to craft various other things." ) ) );
            } else {
                const std::string recipes = enumerate_as_string( known_recipes.begin(), known_recipes.end(),
                [ &inv ]( const recipe * r ) {
                    if( r->requirements().can_make_with_inventory( inv ) ) {
                        return r->result_name();
                    } else {
                        return string_format( "<dark>%s</dark>", r->result_name() );
                    }
                } );
                if( !recipes.empty() ) {
                    insert_separation_line();
                    info.push_back( iteminfo( "DESCRIPTION", string_format( _( "You could use it to craft: %s" ),
                                              recipes.c_str() ) ) );
                }
            }
        }
    }

    if( !info.empty() && info.back().sName == "--" ) {
        info.pop_back();
    }

    return format_item_info( info, {} );
}

std::map<gunmod_location, int> item::get_mod_locations() const
{
    std::map<gunmod_location, int> mod_locations = type->gun->valid_mod_locations;

    for( const auto mod : gunmods() ) {
        if( !mod->type->gunmod->add_mod.empty() ) {
            std::map<gunmod_location, int> add_locations = mod->type->gunmod->add_mod;

            for( auto &add_location : add_locations ) {
                mod_locations[add_location.first] += add_location.second;
            }
        }
    }

    return mod_locations;
}

int item::get_free_mod_locations( const gunmod_location &location ) const
{
    if( !is_gun() ) {
        return 0;
    }

    std::map<gunmod_location, int> mod_locations = get_mod_locations();

    const auto loc = mod_locations.find( location );
    if( loc == mod_locations.end() ) {
        return 0;
    }
    int result = loc->second;
    for( const auto &elem : contents ) {
        const auto &mod = elem.type->gunmod;
        if( mod && mod->location == location ) {
            result--;
        }
    }
    return result;
}

int item::engine_displacement() const
{
    return type->engine ? type->engine->displacement : 0;
}

const std::string &item::symbol() const
{
    return type->sym;
}

nc_color item::color_in_inventory() const
{
    player &u = g->u; // TODO: make a const reference
    nc_color ret = c_light_gray;

    if( has_flag( "WET" ) ) {
        ret = c_cyan;
    } else if( has_flag( "LITCIG" ) ) {
        ret = c_red;
    } else if( is_filthy() || item_tags.count( "DIRTY" ) ) {
        ret = c_brown;
    } else if( has_flag( "LEAK_DAM" ) && has_flag( "RADIOACTIVE" ) && damage() > 0 ) {
        ret = c_light_green;
    } else if( active && !is_food() && !is_food_container() ) { // Active items show up as yellow
        ret = c_yellow;
    } else if( is_food() || is_food_container() ) {
        const bool preserves = type->container && type->container->preserves;
        const item &to_color = is_food() ? *this : contents.front();
        // Default: permafood, drugs
        // Brown: rotten (for non-saprophages) or non-rotten (for saprophages)
        // Dark gray: inedible
        // Red: morale penalty
        // Yellow: will rot soon
        // Cyan: will rot eventually
        const auto rating = u.will_eat( to_color );
        // TODO: More colors
        switch( rating.value() ) {
            case EDIBLE:
            case TOO_FULL:
                if( preserves ) {
                    // Nothing, canned food won't rot
                } else if( to_color.is_going_bad() ) {
                    ret = c_yellow;
                } else if( to_color.goes_bad() ) {
                    ret = c_cyan;
                }
                break;
            case INEDIBLE:
            case INEDIBLE_MUTATION:
                ret = c_dark_gray;
                break;
            case ALLERGY:
            case ALLERGY_WEAK:
            case CANNIBALISM:
                ret = c_red;
                break;
            case ROTTEN:
                ret = c_brown;
                break;
            case NAUSEA:
                ret = c_pink;
                break;
            case NO_TOOL:
                break;
        }
    } else if( is_gun() ) {
        // Guns are green if you are carrying ammo for them
        // ltred if you have ammo but no mags
        // Gun with integrated mag counts as both
        ammotype amtype = ammo_type();
        // get_ammo finds uncontained ammo, find_ammo finds ammo in magazines
        bool has_ammo = !u.get_ammo( amtype ).empty() || !u.find_ammo( *this, false, -1 ).empty();
        bool has_mag = magazine_integral() || !u.find_ammo( *this, true, -1 ).empty();
        if( has_ammo && has_mag ) {
            ret = c_green;
        } else if( has_ammo || has_mag ) {
            ret = c_light_red;
        }
    } else if( is_ammo() ) {
        // Likewise, ammo is green if you have guns that use it
        // ltred if you have the gun but no mags
        // Gun with integrated mag counts as both
        bool has_gun = u.has_item_with( [this]( const item & i ) {
            return i.is_gun() && type->ammo->type.count( i.ammo_type() );
        } );
        bool has_mag = u.has_item_with( [this]( const item & i ) {
            return ( i.is_gun() && i.magazine_integral() && type->ammo->type.count( i.ammo_type() ) ) ||
                   ( i.is_magazine() && type->ammo->type.count( i.ammo_type() ) );
        } );
        if( has_gun && has_mag ) {
            ret = c_green;
        } else if( has_gun || has_mag ) {
            ret = c_light_red;
        }
    } else if( is_magazine() ) {
        // Magazines are green if you have guns and ammo for them
        // ltred if you have one but not the other
        bool has_gun = u.has_item_with( [this]( const item & it ) {
            return it.is_gun() && it.magazine_compatible().count( typeId() ) > 0;
        } );
        bool has_ammo = !u.find_ammo( *this, false, -1 ).empty();
        if( has_gun && has_ammo ) {
            ret = c_green;
        } else if( has_gun || has_ammo ) {
            ret = c_light_red;
        }
    } else if( is_book() ) {
        if( u.has_identified( typeId() ) ) {
            auto &tmp = *type->book;
            if( tmp.skill && // Book can improve skill: blue
                u.get_skill_level_object( tmp.skill ).can_train() &&
                u.get_skill_level( tmp.skill ) >= tmp.req &&
                u.get_skill_level( tmp.skill ) < tmp.level ) {
                ret = c_light_blue;
            } else if( type->can_use( "MA_MANUAL" ) &&
                       !u.has_martialart( martial_art_learned_from( *type ) ) ) {
                ret = c_light_blue;
            } else if( tmp.skill && // Book can't improve skill right now, but maybe later: pink
                       u.get_skill_level_object( tmp.skill ).can_train() &&
                       u.get_skill_level( tmp.skill ) < tmp.level ) {
                ret = c_pink;
            } else if( !u.studied_all_recipes(
                           *type ) ) { // Book can't improve skill anymore, but has more recipes: yellow
                ret = c_yellow;
            }
        } else {
            ret = c_red; // Book hasn't been identified yet: red
        }
    } else if( is_bionic() ) {
        if( !u.has_bionic( type->bionic->id ) ) {
            ret = u.bionic_installation_issues( type->bionic->id ).empty() ? c_green : c_red;
        }
    }
    return ret;
}

void item::on_wear( Character &p )
{
    if( is_sided() && get_side() == side::BOTH ) {
        // for sided items wear the item on the side which results in least encumbrance
        int lhs = 0;
        int rhs = 0;

        set_side( side::LEFT );
        const auto left_enc = p.get_encumbrance( *this );
        for( const body_part bp : all_body_parts ) {
            lhs += left_enc[bp].encumbrance;
        }

        set_side( side::RIGHT );
        const auto right_enc = p.get_encumbrance( *this );
        for( const body_part bp : all_body_parts ) {
            rhs += right_enc[bp].encumbrance;
        }

        set_side( lhs <= rhs ? side::LEFT : side::RIGHT );
    }

    // TODO: artifacts currently only work with the player character
    if( &p == &g->u && type->artifact ) {
        g->add_artifact_messages( type->artifact->effects_worn );
    }

    p.on_item_wear( *this );
}

void item::on_takeoff( Character &p )
{
    p.on_item_takeoff( *this );

    if( is_sided() ) {
        set_side( side::BOTH );
    }
}

void item::on_wield( player &p, int mv )
{
    // TODO: artifacts currently only work with the player character
    if( &p == &g->u && type->artifact ) {
        g->add_artifact_messages( type->artifact->effects_wielded );
    }

    // weapons with bayonet/bipod or other generic "unhandiness"
    if( has_flag( "SLOW_WIELD" ) && !is_gunmod() ) {
        float d = 32.0; // arbitrary linear scaling factor
        if( is_gun() ) {
            d /= std::max( p.get_skill_level( gun_skill() ), 1 );
        } else if( is_melee() ) {
            d /= std::max( p.get_skill_level( melee_skill() ), 1 );
        }

        int penalty = get_var( "volume", volume() / units::legacy_volume_factor ) * d;
        p.moves -= penalty;
        mv += penalty;
    }

    // firearms with a folding stock or tool/melee without collapse/retract iuse
    if( has_flag( "NEEDS_UNFOLD" ) && !is_gunmod() ) {
        int penalty = 50; // 200-300 for guns, 50-150 for melee, 50 as fallback
        if( is_gun() ) {
            penalty = std::max( 0, 300 - p.get_skill_level( gun_skill() ) * 10 );
        } else if( is_melee() ) {
            penalty = std::max( 0, 150 - p.get_skill_level( melee_skill() ) * 10 );
        }

        p.moves -= penalty;
        mv += penalty;
    }

    std::string msg;

    if( mv > 500 ) {
        msg = _( "It takes you an extremely long time to wield your %s." );
    } else if( mv > 250 ) {
        msg = _( "It takes you a very long time to wield your %s." );
    } else if( mv > 100 ) {
        msg = _( "It takes you a long time to wield your %s." );
    } else if( mv > 50 ) {
        msg = _( "It takes you several seconds to wield your %s." );
    } else {
        msg = _( "You wield your %s." );
    }

    p.add_msg_if_player( msg.c_str(), tname().c_str() );
}

void item::on_pickup( Character &p )
{
    // Fake characters are used to determine pickup weight and volume
    if( p.is_fake() ) {
        return;
    }

    // TODO: artifacts currently only work with the player character
    if( &p == &g->u && type->artifact ) {
        g->add_artifact_messages( type->artifact->effects_carried );
    }

    if( is_bucket_nonempty() ) {
        for( const auto &it : contents ) {
            g->m.add_item_or_charges( p.pos(), it );
        }

        contents.clear();
    }
}

void item::on_contents_changed()
{
    if( is_non_resealable_container() ) {
        convert( type->container->unseals_into );
    }
}

void item::on_damage( int, damage_type )
{

}

std::string item::tname( unsigned int quantity, bool with_prefix ) const
{
    std::stringstream ret;

    // MATERIALS-TODO: put this in json
    std::string damtext;

    if( ( damage() != 0 || ( get_option<bool>( "ITEM_HEALTH_BAR" ) && is_armor() ) ) && !is_null() &&
        with_prefix ) {
        if( damage() < 0 )  {
            if( get_option<bool>( "ITEM_HEALTH_BAR" ) ) {
                damtext = "<color_" + string_from_color( damage_color() ) + ">" + damage_symbol() + " </color>";
            } else if( is_gun() ) {
                damtext = pgettext( "damage adjective", "accurized " );
            } else {
                damtext = pgettext( "damage adjective", "reinforced " );
            }
        } else if( typeId() == "corpse" ) {
            if( damage() > 0 ) {
                switch( damage_level( 4 ) ) {
                    case 1:
                        damtext = pgettext( "damage adjective", "bruised " );
                        break;
                    case 2:
                        damtext = pgettext( "damage adjective", "damaged " );
                        break;
                    case 3:
                        damtext = pgettext( "damage adjective", "mangled " );
                        break;
                    default:
                        damtext = pgettext( "damage adjective", "pulped " );
                        break;
                }
            }
        } else if( get_option<bool>( "ITEM_HEALTH_BAR" ) ) {
            damtext = "<color_" + string_from_color( damage_color() ) + ">" + damage_symbol() + " </color>";
        } else {
            damtext = string_format( "%s ", get_base_material().dmg_adj( damage_level( 4 ) ).c_str() );
        }
    }
    if( !faults.empty() ) {
        damtext.insert( 0, _( "faulty " ) );
    }

    std::string vehtext;
    if( is_engine() && engine_displacement() > 0 ) {
        vehtext = string_format( pgettext( "vehicle adjective", "%2.1fL " ),
                                 engine_displacement() / 100.0f );

    } else if( is_wheel() && type->wheel->diameter > 0 ) {
        vehtext = string_format( pgettext( "vehicle adjective", "%d\" " ), type->wheel->diameter );
    }

    std::string burntext;
    if( with_prefix && !made_of_from_type( LIQUID ) ) {
        if( volume() >= 1000_ml && burnt * 125_ml >= volume() ) {
            burntext = pgettext( "burnt adjective", "badly burnt " );
        } else if( burnt > 0 ) {
            burntext = pgettext( "burnt adjective", "burnt " );
        }
    }

    std::string maintext;
    if( is_corpse() || typeId() == "blood" || item_vars.find( "name" ) != item_vars.end() ) {
        maintext = type_name( quantity );
    } else if( is_gun() || is_tool() || is_magazine() ) {
        int amt = 0;
        ret.str( "" );
        ret << label( quantity );
        for( const auto mod : is_gun() ? gunmods() : toolmods() ) {
            if( !type->gun || !type->gun->built_in_mods.count( mod->typeId() ) ) {
                amt++;
            }
        }
        if( amt ) {
            ret << string_format( "+%d", amt );
        }
        maintext = ret.str();
    } else if( is_armor() && item_tags.count( "wooled" ) + item_tags.count( "furred" ) +
               item_tags.count( "leather_padded" ) + item_tags.count( "kevlar_padded" ) > 0 ) {
        ret.str( "" );
        ret << label( quantity );
        ret << "+1";
        maintext = ret.str();
    } else if( contents.size() == 1 ) {
        const item &contents_item = contents.front();
        if( contents_item.made_of( LIQUID ) || contents_item.is_food() ) {
            const unsigned contents_count = contents_item.charges > 1 ? contents_item.charges : quantity;
            maintext = string_format( pgettext( "item name", "%s of %s" ), label( quantity ).c_str(),
                                      contents_item.tname( contents_count, with_prefix ).c_str() );
        } else {
            maintext = string_format( pgettext( "item name", "%s with %s" ), label( quantity ).c_str(),
                                      contents_item.tname( quantity, with_prefix ).c_str() );
        }
    } else if( !contents.empty() ) {
        maintext = string_format( npgettext( "item name",
                                             "%s with %zd item",
                                             "%s with %zd items", contents.size() ),
                                  label( quantity ), contents.size() );
    } else {
        maintext = label( quantity );
    }

    ret.str( "" );
    if( is_food() ) {
        if( has_flag( "HIDDEN_POISON" ) && g->u.get_skill_level( skill_survival ) >= 3 ) {
            ret << _( " (poisonous)" );
        } else if( has_flag( "HIDDEN_HALLU" ) && g->u.get_skill_level( skill_survival ) >= 5 ) {
            ret << _( " (hallucinogenic)" );
        }

        if( item_tags.count( "DIRTY" ) ) {
            ret << _( " (dirty)" );
        } else if( rotten() ) {
            ret << _( " (rotten)" );
        } else if( has_flag( "MUSHY" ) ) {
            ret << _( " (mushy)" );
        } else if( is_going_bad() ) {
            ret << _( " (old)" );
        } else if( is_fresh() ) {
            ret << _( " (fresh)" );
        }

        if( has_flag( "HOT" ) ) {
            ret << _( " (hot)" );
        }
        if( has_flag( "COLD" ) ) {
            ret << _( " (cold)" );
        }
        if( has_flag( "FROZEN" ) ) {
            ret << _( " (frozen)" );
        } else if( has_flag( "MELTS" ) ) {
            ret << _( " (melted)" ); // he melted
        }
    }

    const bool small = g->u.has_trait( trait_id( "SMALL2" ) ) ||
                       g->u.has_trait( trait_id( "SMALL_OK" ) );
    const bool fits = has_flag( "FIT" );
    const bool undersize = has_flag( "UNDERSIZE" );
    if( get_encumber( g->u ) ) {
        if( small && !undersize ) {
            ret << _( " (oversize)" );
        } else if( !small && undersize ) {
            ret << _( " (undersize)" );
        } else if( !fits && has_flag( "VARSIZE" ) ) {
            ret << _( " (poor fit)" );
        }
    }

    if( is_filthy() ) {
        ret << _( " (filthy)" );
    }

    if( is_tool() && has_flag( "USE_UPS" ) ) {
        ret << _( " (UPS)" );
    }

    if( has_var( "NANOFAB_ITEM_ID" ) ) {
        ret << string_format( " (%s)", nname( get_var( "NANOFAB_ITEM_ID" ) ) );
    }

    if( has_flag( "RADIO_MOD" ) ) {
        ret << _( " (radio:" );
        if( has_flag( "RADIOSIGNAL_1" ) ) {
            ret << pgettext( "The radio mod is associated with the [R]ed button.", "R)" );
        } else if( has_flag( "RADIOSIGNAL_2" ) ) {
            ret << pgettext( "The radio mod is associated with the [B]lue button.", "B)" );
        } else if( has_flag( "RADIOSIGNAL_3" ) ) {
            ret << pgettext( "The radio mod is associated with the [G]reen button.", "G)" );
        } else {
            debugmsg( "Why is the radio neither red, blue, nor green?" );
            ret << "?)";
        }
    }

    if( has_flag( "WET" ) ) {
        ret << _( " (wet)" );
    }
    if( already_used_by_player( g->u ) ) {
        ret << _( " (used)" );
    }
    if( active && ( has_flag( "WATER_EXTINGUISH" ) || has_flag( "LITCIG" ) ) ) {
        ret << _( " (lit)" );
    } else if( active && !is_food() && !is_corpse() && ( typeId().length() < 3 ||
               typeId().compare( typeId().length() - 3, 3, "_on" ) != 0 ) ) {
        // Usually the items whose ids end in "_on" have the "active" or "on" string already contained
        // in their name, also food is active while it rots.
        ret << _( " (active)" );
    }

    const std::string tagtext = ret.str();

    std::string modtext;
    if( gunmod_find( "barrel_small" ) ) {
        modtext += _( "sawn-off " );
    }
    if( has_flag( "DIAMOND" ) ) {
        modtext += std::string( pgettext( "Adjective, as in diamond katana", "diamond" ) ) + " ";
    }

    ret.str( "" );
    //~ This is a string to construct the item name as it is displayed. This format string has been added for maximum flexibility. The strings are: %1$s: Damage text (e.g. "bruised"). %2$s: burn adjectives (e.g. "burnt"). %3$s: tool modifier text (e.g. "atomic"). %4$s: vehicle part text (e.g. "3.8-Liter"). $5$s: main item text (e.g. "apple"). %6s: tags (e.g. "(wet) (poor fit)").
    ret << string_format( _( "%1$s%2$s%3$s%4$s%5$s%6$s" ), damtext.c_str(), burntext.c_str(),
                          modtext.c_str(), vehtext.c_str(), maintext.c_str(), tagtext.c_str() );

    if( item_vars.find( "item_note" ) != item_vars.end() ) {
        //~ %s is an item name. This style is used to denote items with notes.
        return string_format( _( "*%s*" ), ret.str().c_str() );
    } else {
        return ret.str();
    }
}

std::string item::display_money( unsigned int quantity, unsigned long amount ) const
{
    //~ This is a string to display the total amount of money in a stack of cash cards. The strings are: %s is the display name of cash cards. The following bracketed $%.2f is the amount of money on the stack of cards in dollars, to two decimal points. (e.g. "cash cards ($15.35)")
    return string_format( "%s %s", tname( quantity ).c_str(), format_money( amount ) );
}

std::string item::display_name( unsigned int quantity ) const
{
    std::string name = tname( quantity );
    std::string sidetxt;
    std::string amt;

    switch( get_side() ) {
        case side::BOTH:
            break;
        case side::LEFT:
            sidetxt = string_format( " (%s)", _( "left" ) );
            break;
        case side::RIGHT:
            sidetxt = string_format( " (%s)", _( "right" ) );
            break;
    }
    int amount = 0;
    int max_amount = 0;
    bool has_item = is_container() && contents.size() == 1;
    bool has_ammo = is_ammo_container() && contents.size() == 1;
    bool contains = has_item || has_ammo;
    bool show_amt = false;
    // We should handle infinite charges properly in all cases.
    if( contains ) {
        amount = contents.front().charges;
        max_amount = contents.front().charges_per_volume( get_container_capacity() );
    } else if( is_book() && get_chapters() > 0 ) {
        // a book which has remaining unread chapters
        amount = get_remaining_chapters( g->u );
    } else if( ammo_capacity() > 0 ) {
        // anything that can be reloaded including tools, magazines, guns and auxiliary gunmods
        // but excluding bows etc., which have ammo, but can't be reloaded
        amount = ammo_remaining();
        max_amount = ammo_capacity();
        show_amt = !has_flag( "RELOAD_AND_SHOOT" );
    } else if( count_by_charges() && !has_infinite_charges() ) {
        // A chargeable item
        amount = charges;
        max_amount = ammo_capacity();
    }

    if( amount || show_amt ) {
        if( ammo_type().str() == "money" ) {
            amt = string_format( " $%.2f", amount / 100.0 );
        } else {
            if( max_amount != 0 ) {
                amt = string_format( " (%i/%i)", amount, max_amount );
            } else {
                amt = string_format( " (%i)", amount );
            }
        }
    }

    // This is a hack to prevent possible crashing when displaying maps as items during character creation
    if( is_map() && calendar::turn != calendar::time_of_cataclysm ) {
        const city *c = overmap_buffer.closest_city( omt_to_sm_copy( get_var( "reveal_map_center_omt",
                        g->u.global_omt_location() ) ) ).city;
        if( c != nullptr ) {
            name = string_format( "%s %s", c->name.c_str(), name.c_str() );
        }
    }

    return string_format( "%s%s%s", name.c_str(), sidetxt.c_str(), amt.c_str() );
}

nc_color item::color() const
{
    if( is_null() ) {
        return c_black;
    }
    if( is_corpse() ) {
        return corpse->color;
    }
    return type->color;
}

int item::price( bool practical ) const
{
    int res = 0;

    visit_items( [&res, practical]( const item * e ) {
        if( e->rotten() ) {
            // @todo: Special case things that stay useful when rotten
            return VisitResponse::NEXT;
        }

        int child = practical ? e->type->price_post : e->type->price;
        if( e->damage() > 0 ) {
            // maximal damage level is 4, maximal reduction is 40% of the value.
            child -= child * static_cast<double>( e->damage_level( 4 ) ) / 10;
        }

        if( e->count_by_charges() || e->made_of( LIQUID ) ) {
            // price from json data is for default-sized stack
            child *= e->charges / static_cast<double>( e->type->stack_size );

        } else if( e->magazine_integral() && e->ammo_remaining() && e->ammo_data() ) {
            // items with integral magazines may contain ammunition which can affect the price
            child += item( e->ammo_data(), calendar::turn, e->charges ).price( practical );

        } else if( e->is_tool() && !e->ammo_type() && e->ammo_capacity() ) {
            // if tool has no ammo (e.g. spray can) reduce price proportional to remaining charges
            child *= e->ammo_remaining() / double( std::max( e->type->charges_default(), 1 ) );
        }

        res += child;
        return VisitResponse::NEXT;
    } );

    return res;
}

// MATERIALS-TODO: add a density field to materials.json
units::mass item::weight( bool include_contents ) const
{
    if( is_null() ) {
        return 0_gram;
    }

    // Items that don't drop aren't really there, they're items just for ease of implementation
    if( has_flag( "NO_DROP" ) ) {
        return 0_gram;
    }

    units::mass ret = units::from_gram( get_var( "weight", to_gram( type->weight ) ) );
    if( has_flag( "REDUCED_WEIGHT" ) ) {
        ret *= 0.75;
    }

    if( count_by_charges() ) {
        ret *= charges;

    } else if( is_corpse() ) {
        ret = corpse->weight;
        if( has_flag( "FIELD_DRESS" ) || has_flag( "FIELD_DRESS_FAILED" ) ) {
            ret *= 0.75;
        }
        if( has_flag( "QUARTERED" ) ) {
            ret /= 4;
        }
        if( has_flag( "GIBBED" ) ) {
            ret *= 0.85;
        }
        if( has_flag( "SKINNED" ) ) {
            ret *= 0.85;
        }

    } else if( magazine_integral() && !is_magazine() ) {
        if( ammo_type() == ammotype( "plutonium" ) ) {
            ret += ammo_remaining() * find_type( ammo_type()->default_ammotype() )->weight / PLUTONIUM_CHARGES;
        } else if( ammo_data() ) {
            ret += ammo_remaining() * ammo_data()->weight;
        }
    }

    // if this is an ammo belt add the weight of any implicitly contained linkages
    if( is_magazine() && type->magazine->linkage ) {
        item links( *type->magazine->linkage );
        links.charges = ammo_remaining();
        ret += links.weight();
    }

    // reduce weight for sawn-off weapons capped to the apportioned weight of the barrel
    if( gunmod_find( "barrel_small" ) ) {
        const units::volume b = type->gun->barrel_length;
        const units::mass max_barrel_weight = units::from_gram( to_milliliter( b ) );
        const units::mass barrel_weight = units::from_gram( b.value() * type->weight.value() /
                                          type->volume.value() );
        ret -= std::min( max_barrel_weight, barrel_weight );
    }

    if( include_contents ) {
        for( auto &elem : contents ) {
            ret += elem.weight();
        }
    }

    return ret;
}

units::volume item::corpse_volume( const mtype *corpse ) const
{
    units::volume corpse_volume = corpse->volume;
    if( has_flag( "QUARTERED" ) ) {
        corpse_volume /= 4;
    }
    if( has_flag( "FIELD_DRESS" ) || has_flag( "FIELD_DRESS_FAILED" ) ) {
        corpse_volume *= 0.75;
    }
    if( has_flag( "GIBBED" ) ) {
        corpse_volume *= 0.85;
    }
    if( has_flag( "SKINNED" ) ) {
        corpse_volume *= 0.85;
    }
    if( corpse_volume > 0_ml ) {
        return corpse_volume;
    }
    debugmsg( "invalid monster volume for corpse" );
    return 0_ml;
}

units::volume item::base_volume() const
{
    if( is_null() ) {
        return 0_ml;
    }
    if( is_corpse() ) {
        return corpse_volume( corpse );
    }

    if( count_by_charges() ) {
        if( type->volume % type->stack_size == 0_ml ) {
            return type->volume / type->stack_size;
        } else {
            return type->volume / type->stack_size + 1_ml;
        }
    }

    return type->volume;
}

units::volume item::volume( bool integral ) const
{
    if( is_null() ) {
        return 0_ml;
    }

    if( is_corpse() ) {
        return corpse_volume( corpse );
    }

    const int local_volume = get_var( "volume", -1 );
    units::volume ret;
    if( local_volume >= 0 ) {
        ret = local_volume * units::legacy_volume_factor;
    } else if( integral ) {
        ret = type->integral_volume;
    } else {
        ret = type->volume;
    }

    if( count_by_charges() || made_of( LIQUID ) ) {
        auto num = ret * static_cast<int64_t>( charges );
        ret = num / type->stack_size;
        if( num % type->stack_size != 0_ml ) {
            ret += 1_ml;
        }
    }

    // Non-rigid items add the volume of the content
    if( !type->rigid ) {
        for( auto &elem : contents ) {
            ret += elem.volume();
        }
    }

    // Some magazines sit (partly) flush with the item so add less extra volume
    if( magazine_current() != nullptr ) {
        ret += std::max( magazine_current()->volume() - type->magazine_well, 0_ml );
    }

    if( is_gun() ) {
        for( const auto elem : gunmods() ) {
            ret += elem->volume( true );
        }

        // @todo: implement stock_length property for guns
        if( has_flag( "COLLAPSIBLE_STOCK" ) ) {
            // consider only the base size of the gun (without mods)
            int tmpvol = get_var( "volume",
                                  ( type->volume - type->gun->barrel_length ) / units::legacy_volume_factor );
            if( tmpvol <=  3 ) {
                // intentional NOP
            } else if( tmpvol <=  5 ) {
                ret -=  250_ml;
            } else if( tmpvol <=  6 ) {
                ret -=  500_ml;
            } else if( tmpvol <=  9 ) {
                ret -=  750_ml;
            } else if( tmpvol <= 12 ) {
                ret -= 1000_ml;
            } else if( tmpvol <= 15 ) {
                ret -= 1250_ml;
            } else {
                ret -= 1500_ml;
            }
        }

        if( gunmod_find( "barrel_small" ) ) {
            ret -= type->gun->barrel_length;
        }
    }

    return ret;
}

int item::lift_strength() const
{
    return std::max( weight() / 10000_gram, 1 );
}

int item::attack_time() const
{
    int ret = 65 + volume() / 62.5_ml + weight() / 60_gram;
    return ret;
}

int item::damage_melee( damage_type dt ) const
{
    assert( dt >= DT_NULL && dt < NUM_DT );
    if( is_null() ) {
        return 0;
    }

    // effectiveness is reduced by 10% per damage level
    int res = type->melee[ dt ];
    res -= res * damage_level( 4 ) * 0.1;

    // apply type specific flags
    switch( dt ) {
        case DT_BASH:
            if( has_flag( "REDUCED_BASHING" ) ) {
                res *= 0.5;
            }
            break;

        case DT_CUT:
        case DT_STAB:
            if( has_flag( "DIAMOND" ) ) {
                res *= 1.3;
            }
            break;

        default:
            break;
    }

    // consider any melee gunmods
    if( is_gun() ) {
        std::vector<int> opts = { res };
        for( const auto &e : gun_all_modes() ) {
            if( e.second.target != this && e.second.melee() ) {
                opts.push_back( e.second.target->damage_melee( dt ) );
            }
        }
        return *std::max_element( opts.begin(), opts.end() );

    }

    return std::max( res, 0 );
}

damage_instance item::base_damage_melee() const
{
    // @todo: Caching
    damage_instance ret;
    for( size_t i = DT_NULL + 1; i < NUM_DT; i++ ) {
        damage_type dt = static_cast<damage_type>( i );
        int dam = damage_melee( dt );
        if( dam > 0 ) {
            ret.add_damage( dt, dam );
        }
    }

    return ret;
}

damage_instance item::base_damage_thrown() const
{
    // @todo: Create a separate cache for individual items (for modifiers like diamond etc.)
    return type->thrown_damage;
}

int item::reach_range( const player &p ) const
{
    int res = 1;

    if( has_flag( "REACH_ATTACK" ) ) {
        res = has_flag( "REACH3" ) ? 3 : 2;
    }

    // for guns consider any attached gunmods
    if( is_gun() && !is_gunmod() ) {
        for( const auto &m : gun_all_modes() ) {
            if( p.is_npc() && m.second.flags.count( "NPC_AVOID" ) ) {
                continue;
            }
            if( m.second.melee() ) {
                res = std::max( res, m.second.qty );
            }
        }
    }

    return res;
}

void item::unset_flags()
{
    item_tags.clear();
}

bool item::has_flag( const std::string &f ) const
{
    bool ret = false;

    if( json_flag::get( f ).inherit() ) {
        for( const auto e : is_gun() ? gunmods() : toolmods() ) {
            // gunmods fired separately do not contribute to base gun flags
            if( !e->is_gun() && e->has_flag( f ) ) {
                return true;
            }
        }
    }

    // other item type flags
    ret = type->item_tags.count( f );
    if( ret ) {
        return ret;
    }

    // now check for item specific flags
    ret = item_tags.count( f );
    return ret;
}

bool item::has_any_flag( const std::vector<std::string> &flags ) const
{
    for( auto &flag : flags ) {
        if( has_flag( flag ) ) {
            return true;
        }
    }

    return false;
}

item &item::set_flag( const std::string &flag )
{
    item_tags.insert( flag );
    return *this;
}

item &item::unset_flag( const std::string &flag )
{
    item_tags.erase( flag );
    return *this;
}

bool item::has_property( const std::string &prop ) const
{
    return type->properties.find( prop ) != type->properties.end();
}

std::string item::get_property_string( const std::string &prop, const std::string &def ) const
{
    const auto it = type->properties.find( prop );
    return it != type->properties.end() ? it->second : def;
}

long item::get_property_long( const std::string &prop, long def ) const
{
    const auto it = type->properties.find( prop );
    if( it != type->properties.end() ) {
        char *e = nullptr;
        long  r = std::strtol( it->second.c_str(), &e, 10 );
        if( !it->second.empty() && *e == '\0' ) {
            return r;
        }
        debugmsg( "invalid property '%s' for item '%s'", prop.c_str(), tname().c_str() );
    }
    return def;
}

int item::get_quality( const quality_id &id ) const
{
    int return_quality = INT_MIN;

    /**
     * EXCEPTION: Items with quality BOIL only count as such if they are empty,
     * excluding items of their ammo type if they are tools.
     */
    if( id == quality_id( "BOIL" ) && !( contents.empty() ||
                                         ( is_tool() && std::all_of( contents.begin(), contents.end(),
    [this]( const item & itm ) {
    if( !itm.is_ammo() ) {
            return false;
        }
        auto &ammo_types = itm.type->ammo->type;
        return ammo_types.find( ammo_type() ) != ammo_types.end();
    } ) ) ) ) {
        return INT_MIN;
    }

    for( const auto &quality : type->qualities ) {
        if( quality.first == id ) {
            return_quality = quality.second;
        }
    }
    for( auto &itm : contents ) {
        return_quality = std::max( return_quality, itm.get_quality( id ) );
    }

    return return_quality;
}

bool item::has_technique( const matec_id &tech ) const
{
    return type->techniques.count( tech ) > 0 || techniques.count( tech ) > 0;
}

void item::add_technique( const matec_id &tech )
{
    techniques.insert( tech );
}

std::vector<item *> item::toolmods()
{
    std::vector<item *> res;
    if( is_tool() ) {
        res.reserve( contents.size() );
        for( auto &e : contents ) {
            if( e.is_toolmod() ) {
                res.push_back( &e );
            }
        }
    }
    return res;
}

std::vector<const item *> item::toolmods() const
{
    std::vector<const item *> res;
    if( is_tool() ) {
        res.reserve( contents.size() );
        for( auto &e : contents ) {
            if( e.is_toolmod() ) {
                res.push_back( &e );
            }
        }
    }
    return res;
}

std::set<matec_id> item::get_techniques() const
{
    std::set<matec_id> result = type->techniques;
    result.insert( techniques.begin(), techniques.end() );
    return result;
}

bool item::goes_bad() const
{
    return is_food() && type->comestible->spoils != 0_turns;
}

double item::get_relative_rot() const
{
    return goes_bad() ? rot / type->comestible->spoils : 0;
}

void item::set_relative_rot( double val )
{
    if( goes_bad() ) {
        rot = type->comestible->spoils * val;
        // calc_rot uses last_rot_check (when it's not time_of_cataclysm) instead of bday.
        // this makes sure the rotting starts from now, not from bday.
        // if this item is the result of smoking don't do this, we want to start from bday.
        if( !has_flag( "SMOKING_RESULT" ) ) {
            last_rot_check = calendar::turn;
        }
        active = true;
    }
}

int item::spoilage_sort_order()
{
    item *subject;
    int bottom = std::numeric_limits<int>::max();

    if( type->container && !contents.empty() ) {
        if( type->container->preserves ) {
            return bottom - 3;
        }
        subject = &contents.front();
    } else {
        subject = this;
    }

    if( subject->goes_bad() ) {
        return to_turns<int>( subject->type->comestible->spoils - subject->rot );
    }

    if( subject->type->comestible ) {
        if( subject->get_category().id() == "food" ) {
            return bottom - 3;
        } else if( subject->get_category().id() == "drugs" ) {
            return bottom - 2;
        } else {
            return bottom - 1;
        }
    }
    return bottom;
}

void item::calc_rot( const tripoint &location )
{
    // Avoid needlessly calculating already rotten things.  Corpses should
    // always rot away and food rots away at twice the shelf life.  If the food
    // is in a sealed container they won't rot away, this avoids needlessly
    // calculating their rot in that case.
    if( !is_corpse() && get_relative_rot() > 2.0 ) {
        return;
    }

    const time_point now = calendar::turn;
    if( now - last_rot_check > 10_turns ) {
        const time_point since = last_rot_check == calendar::time_of_cataclysm ? bday : last_rot_check;

        last_rot_check = now;

        // Frozen food do not rot, so no change to rot variable
        // Smoking food will be checked for rot in smoker_finalize
        if( item_tags.count( "FROZEN" ) || item_tags.count( "SMOKING" ) ) {
            return;
        }

        // rot modifier
        float factor = 1.0;
        if( is_corpse() && has_flag( "FIELD_DRESS" ) ) {
            factor = 0.75;
        }

        // simulation of different age of food at calendar::time_of_cataclysm and good/bad storage
        // conditions by applying starting variation bonus/penalty of +/- 20% of base shelf-life
        // positive = food was produced some time before calendar::time_of_cataclysm and/or bad storage
        // negative = food was stored in good condiitons before calendar::time_of_cataclysm
        if( since == calendar::time_of_cataclysm && goes_bad() ) {
            time_duration spoil_variation = type->comestible->spoils * 0.2f;
            rot += factor * rng( -spoil_variation, spoil_variation );
        }

        if( item_tags.count( "COLD" ) ) {
            rot += factor * ( ( now - since ) / 1_hours *
                              get_hourly_rotpoints_at_temp( temperatures::fridge ) * 1_turns ) ;
        } else {
            rot += factor * get_rot_since( since, now, location );
        }

    }
}

void item::calc_rot_while_smoking( const tripoint &location, time_duration smoking_duration )
{
    if( !item_tags.count( "SMOKING" ) ) {
        debugmsg( "calc_rot_while_smoking called on non smoking item: %s", tname().c_str() );
        return;
    }

    // Apply rot at 1/2 normal rate while smoking
    rot += 0.5 * get_rot_since( last_rot_check, last_rot_check + smoking_duration, location );
    last_rot_check += smoking_duration;
}

units::volume item::get_storage() const
{
    auto t = find_armor_data();
    if( t == nullptr ) {
        return 0_ml;
    }

    return t->storage;
}

int item::get_env_resist( int override_base_resist ) const
{
    const auto t = find_armor_data();
    if( t == nullptr ) {
        return 0;
    }
    // modify if item is a gas mask and has filter
    int resist_base = t->env_resist;
    int resist_filter = get_var( "overwrite_env_resist", 0 );
    int resist = std::max( { resist_base, resist_filter, override_base_resist } );

    return lround( resist * get_relative_health() );
}

int item::get_base_env_resist_w_filter() const
{
    const auto t = find_armor_data();
    if( t == nullptr ) {
        return 0;
    }
    return t->env_resist_w_filter;
}

bool item::is_power_armor() const
{
    const auto t = find_armor_data();
    if( t == nullptr ) {
        return false;
    }
    return t->power_armor;
}

int item::get_encumber( const Character &p ) const
{
    units::volume contents_volume( 0_ml );
    for( const auto &e : contents ) {
        contents_volume += e.volume();
    }
    return get_encumber_when_containing( p, contents_volume );
}

int item::get_encumber_when_containing(
    const Character &p, const units::volume &contents_volume ) const
{
    const auto t = find_armor_data();
    if( t == nullptr ) {
        // handle wearable guns (e.g. shoulder strap) as special case
        return is_gun() ? volume() / 750_ml : 0;
    }
    int encumber = t->encumber;

    // Non-rigid items add additional encumbrance proportional to their volume
    if( !type->rigid ) {
        encumber += contents_volume / 250_ml;
    }

    // Fit checked before changes, fitting shouldn't reduce penalties from patching.
    if( item_tags.count( "FIT" ) && has_flag( "VARSIZE" ) ) {
        encumber = std::max( encumber / 2, encumber - 10 );
    }

    const bool tiniest = p.has_trait( trait_small2 ) ||
                         p.has_trait( trait_small_ok );
    const bool is_undersize = has_flag( "UNDERSIZE" );
    if( !is_undersize && tiniest ) {
        encumber *= 2; // clothes bag up around smol mousefolk and encumber them more
    } else if( is_undersize && !tiniest ) {
        encumber *= 3; // normal humans have a HARD time wearing undersized clothing
    }

    const int thickness = get_thickness();
    const int coverage = get_coverage();
    if( item_tags.count( "wooled" ) ) {
        encumber += 1 + 3 * coverage / 100;
    }
    if( item_tags.count( "furred" ) ) {
        encumber += 1 + 4 * coverage / 100;
    }

    if( item_tags.count( "leather_padded" ) ) {
        encumber += ceil( 2 * thickness * coverage / 100.0f );
    }
    if( item_tags.count( "kevlar_padded" ) ) {
        encumber += ceil( 2 * thickness * coverage / 100.0f );
    }

    return encumber;
}

layer_level item::get_layer() const
{
    if( type->armor ) {
        // We assume that an item will never have per-item flags defining its
        // layer, so we can defer to the itype.
        return type->layer;
    }

    if( has_flag( "SKINTIGHT" ) ) {
        return UNDERWEAR;
    } else if( has_flag( "WAIST" ) ) {
        return WAIST_LAYER;
    } else if( has_flag( "OUTER" ) ) {
        return OUTER_LAYER;
    } else if( has_flag( "BELTED" ) ) {
        return BELTED_LAYER;
    } else {
        return REGULAR_LAYER;
    }
}

int item::get_coverage() const
{
    const auto t = find_armor_data();
    if( t == nullptr ) {
        return 0;
    }
    return t->coverage;
}

int item::get_thickness() const
{
    const auto t = find_armor_data();
    if( t == nullptr ) {
        return 0;
    }
    return t->thickness;
}

int item::get_warmth() const
{
    int fur_lined = 0;
    int wool_lined = 0;
    const auto t = find_armor_data();
    if( t == nullptr ) {
        return 0;
    }
    int result = t->warmth;

    if( item_tags.count( "furred" ) > 0 ) {
        fur_lined = 35 * get_coverage() / 100;
    }

    if( item_tags.count( "wooled" ) > 0 ) {
        wool_lined = 20 * get_coverage() / 100;
    }

    return result + fur_lined + wool_lined;
}

time_duration item::brewing_time() const
{
    return is_brewable() ? type->brewable->time * calendar::season_from_default_ratio() : 0_turns;
}

const std::vector<itype_id> &item::brewing_results() const
{
    static const std::vector<itype_id> nulresult{};
    return is_brewable() ? type->brewable->results : nulresult;
}

bool item::can_revive() const
{
    if( is_corpse() && corpse->has_flag( MF_REVIVES ) && damage() < max_damage() &&
        !( has_flag( "FIELD_DRESS" ) || has_flag( "FIELD_DRESS_FAILED" ) || has_flag( "QUARTERED" ) ||
           has_flag( "SKINNED" ) ) ) {

        return true;
    }
    return false;
}

bool item::ready_to_revive( const tripoint &pos ) const
{
    if( !can_revive() ) {
        return false;
    }
    int age_in_hours = to_hours<int>( age() );
    age_in_hours -= int( static_cast<float>( burnt ) / ( volume() / 250_ml ) );
    if( damage_level( 4 ) > 0 ) {
        age_in_hours /= ( damage_level( 4 ) + 1 );
    }
    int rez_factor = 48 - age_in_hours;
    if( age_in_hours > 6 && ( rez_factor <= 0 || one_in( rez_factor ) ) ) {
        // If we're a special revival zombie, wait to get up until the player is nearby.
        const bool isReviveSpecial = has_flag( "REVIVE_SPECIAL" );
        if( isReviveSpecial ) {
            const int distance = rl_dist( pos, g->u.pos() );
            if( distance > 3 ) {
                return false;
            }
            if( !one_in( distance + 1 ) ) {
                return false;
            }
        }

        return true;
    }
    return false;
}

bool item::count_by_charges() const
{
    return type->count_by_charges();
}

long item::count() const
{
    return count_by_charges() ? charges : 1;
}

bool item::craft_has_charges()
{
    if( count_by_charges() ) {
        return true;
    } else if( !ammo_type() ) {
        return true;
    }

    return false;
}

#ifdef _MSC_VER
// Deal with MSVC compiler bug (#17791, #17958)
#pragma optimize( "", off )
#endif

int item::bash_resist( bool to_self ) const
{
    if( is_null() ) {
        return 0;
    }

    const int base_thickness = get_thickness();
    float resist = 0;
    float padding = 0;
    int eff_thickness = 1;

    // Armor gets an additional multiplier.
    if( is_armor() ) {
        // base resistance
        // Don't give reinforced items +armor, just more resistance to ripping
        const int dmg = damage_level( 4 );
        const int eff_damage = to_self ? std::min( dmg, 0 ) : std::max( dmg, 0 );
        eff_thickness = std::max( 1, get_thickness() - eff_damage );
    }
    if( item_tags.count( "leather_padded" ) > 0 ) {
        padding += base_thickness;
    }
    if( item_tags.count( "kevlar_padded" ) > 0 ) {
        padding += base_thickness;
    }

    const std::vector<const material_type *> mat_types = made_of_types();
    if( !mat_types.empty() ) {
        for( auto mat : mat_types ) {
            resist += mat->bash_resist();
        }
        // Average based on number of materials.
        resist /= mat_types.size();
    }

    return lround( ( resist * eff_thickness ) + padding );
}

int item::cut_resist( bool to_self ) const
{
    if( is_null() ) {
        return 0;
    }

    const int base_thickness = get_thickness();
    float resist = 0;
    float padding = 0;
    int eff_thickness = 1;

    // Armor gets an additional multiplier.
    if( is_armor() ) {
        // base resistance
        // Don't give reinforced items +armor, just more resistance to ripping
        const int dmg = damage_level( 4 );
        const int eff_damage = to_self ? std::min( dmg, 0 ) : std::max( dmg, 0 );
        eff_thickness = std::max( 1, base_thickness - eff_damage );
    }
    if( item_tags.count( "leather_padded" ) > 0 ) {
        padding += base_thickness;
    }
    if( item_tags.count( "kevlar_padded" ) > 0 ) {
        padding += base_thickness * 2;
    }

    const std::vector<const material_type *> mat_types = made_of_types();
    if( !mat_types.empty() ) {
        for( auto mat : mat_types ) {
            resist += mat->cut_resist();
        }
        // Average based on number of materials.
        resist /= mat_types.size();
    }

    return lround( ( resist * eff_thickness ) + padding );
}

#ifdef _MSC_VER
#pragma optimize( "", on )
#endif

int item::stab_resist( bool to_self ) const
{
    // Better than hardcoding it in multiple places
    return static_cast<int>( 0.8f * cut_resist( to_self ) );
}

int item::acid_resist( bool to_self, int base_env_resist ) const
{
    if( to_self ) {
        // Currently no items are damaged by acid
        return INT_MAX;
    }

    float resist = 0.0;
    if( is_null() ) {
        return 0.0;
    }

    const std::vector<const material_type *> mat_types = made_of_types();
    if( !mat_types.empty() ) {
        // Not sure why cut and bash get an armor thickness bonus but acid doesn't,
        // but such is the way of the code.

        for( auto mat : mat_types ) {
            resist += mat->acid_resist();
        }
        // Average based on number of materials.
        resist /= mat_types.size();
    }

    const int env = get_env_resist( base_env_resist );
    if( env < 10 ) {
        // Low env protection means it doesn't prevent acid seeping in.
        resist *= env / 10.0f;
    }

    return lround( resist );
}

int item::fire_resist( bool to_self, int base_env_resist ) const
{
    if( to_self ) {
        // Fire damages items in a different way
        return INT_MAX;
    }

    float resist = 0.0;
    if( is_null() ) {
        return 0.0;
    }

    const std::vector<const material_type *> mat_types = made_of_types();
    if( !mat_types.empty() ) {
        for( auto mat : mat_types ) {
            resist += mat->fire_resist();
        }
        // Average based on number of materials.
        resist /= mat_types.size();
    }

    const int env = get_env_resist( base_env_resist );
    if( env < 10 ) {
        // Iron resists immersion in magma, iron-clad knight won't.
        resist *= env / 10.0f;
    }

    return lround( resist );
}

int item::chip_resistance( bool worst ) const
{
    int res = worst ? INT_MAX : INT_MIN;
    for( const auto &mat : made_of_types() ) {
        const int val = mat->chip_resist();
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

int item::min_damage() const
{
    return type->damage_min;
}

int item::max_damage() const
{
    return type->damage_max;
}

float item::get_relative_health() const
{
    return ( max_damage() + 1.0f - damage() ) / ( max_damage() + 1.0f );
}

bool item::mod_damage( int qty, damage_type dt )
{
    bool destroy = false;

    if( count_by_charges() ) {
        charges -= std::min( long( type->stack_size * qty / itype::damage_scale ), charges );
        destroy |= charges == 0;
    }

    if( qty > 0 ) {
        on_damage( qty, dt );
    }

    if( !count_by_charges() ) {
        destroy |= damage_ + qty > max_damage();

        damage_ = std::max( std::min( damage_ + qty, max_damage() ), min_damage() );
    }

    return destroy;
}

bool item::mod_damage( const int qty )
{
    return mod_damage( qty, DT_NULL );
}

bool item::inc_damage( const damage_type dt )
{
    return mod_damage( itype::damage_scale, dt );
}

bool item::inc_damage()
{
    return inc_damage( DT_NULL );
}

nc_color item::damage_color() const
{
    // @todo: unify with veh_interact::countDurability
    switch( damage_level( 4 ) ) {
        default: // reinforced
            if( damage() <= min_damage() ) { // fully reinforced
                return c_green;
            } else {
                return c_light_green;
            }
        case 0:
            return c_light_green;
        case 1:
            return c_yellow;
        case 2:
            return c_magenta;
        case 3:
            return c_light_red;
        case 4:
            return c_red;
    }
}

std::string item::damage_symbol() const
{
    switch( damage_level( 4 ) ) {
        default: // reinforced
            return _( R"(++)" );
        case 0:
            return _( R"(||)" );
        case 1:
            return _( R"(|\)" );
        case 2:
            return _( R"(|.)" );
        case 3:
            return _( R"(\.)" );
        case 4:
            return _( R"(..)" );
    }
}

const std::set<itype_id> &item::repaired_with() const
{
    static std::set<itype_id> no_repair;
    return has_flag( "NO_REPAIR" )  ? no_repair : type->repair;
}

void item::mitigate_damage( damage_unit &du ) const
{
    const resistances res = resistances( *this );
    const float mitigation = res.get_effective_resist( du );
    du.amount -= mitigation;
    du.amount = std::max( 0.0f, du.amount );
}

int item::damage_resist( damage_type dt, bool to_self ) const
{
    switch( dt ) {
        case DT_NULL:
        case NUM_DT:
            return 0;
        case DT_TRUE:
        case DT_BIOLOGICAL:
        case DT_ELECTRIC:
        case DT_COLD:
            // Currently hardcoded:
            // Items can never be damaged by those types
            // But they provide 0 protection from them
            return to_self ? INT_MAX : 0;
        case DT_BASH:
            return bash_resist( to_self );
        case DT_CUT:
            return cut_resist( to_self );
        case DT_ACID:
            return acid_resist( to_self );
        case DT_STAB:
            return stab_resist( to_self );
        case DT_HEAT:
            return fire_resist( to_self );
        default:
            debugmsg( "Invalid damage type: %d", dt );
    }

    return 0;
}

bool item::is_two_handed( const player &u ) const
{
    if( has_flag( "ALWAYS_TWOHAND" ) ) {
        return true;
    }
    ///\EFFECT_STR determines which weapons can be wielded with one hand
    return ( ( weight() / 113_gram ) > u.str_cur * 4 );
}

const std::vector<material_id> &item::made_of() const
{
    if( is_corpse() ) {
        return corpse->mat;
    }
    return type->materials;
}

const std::map<quality_id, int> &item::quality_of() const
{
    return type->qualities;
}

std::vector<const material_type *> item::made_of_types() const
{
    std::vector<const material_type *> material_types_composed_of;
    for( const material_id &mat_id : made_of() ) {
        material_types_composed_of.push_back( &mat_id.obj() );
    }
    return material_types_composed_of;
}

bool item::made_of_any( const std::set<material_id> &mat_idents ) const
{
    const auto mats = made_of();
    if( mats.empty() ) {
        return false;
    }

    return std::any_of( mats.begin(), mats.end(), [&mat_idents]( const material_id & e ) {
        return mat_idents.count( e );
    } );
}

bool item::only_made_of( const std::set<material_id> &mat_idents ) const
{
    const auto mats = made_of();
    if( mats.empty() ) {
        return false;
    }

    return std::all_of( mats.begin(), mats.end(), [&mat_idents]( const material_id & e ) {
        return mat_idents.count( e );
    } );
}

bool item::made_of( const material_id &mat_ident ) const
{
    const auto &materials = made_of();
    return std::find( materials.begin(), materials.end(), mat_ident ) != materials.end();
}

bool item::contents_made_of( const phase_id phase ) const
{
    return !contents.empty() && contents.front().made_of( phase );
}

bool item::made_of( phase_id phase ) const
{
    if( is_null() ) {
        return false;
    }
    return current_phase == phase;
}

bool item::made_of_from_type( phase_id phase ) const
{
    if( is_null() ) {
        return false;
    }
    return type->phase == phase;
}

bool item::conductive() const
{
    if( is_null() ) {
        return false;
    }

    if( has_flag( "CONDUCTIVE" ) ) {
        return true;
    }

    if( has_flag( "NONCONDUCTIVE" ) ) {
        return false;
    }

    // If any material has electricity resistance equal to or lower than flesh (1) we are conductive.
    const auto mats = made_of_types();
    return std::any_of( mats.begin(), mats.end(), []( const material_type * mt ) {
        return mt->elec_resist() <= 1;
    } );
}

bool item::destroyed_at_zero_charges() const
{
    return ( is_ammo() || is_food() );
}

bool item::is_gun() const
{
    return type->gun.has_value();
}

bool item::is_firearm() const
{
    static const std::string primitive_flag( "PRIMITIVE_RANGED_WEAPON" );
    return is_gun() && !has_flag( primitive_flag );
}

int item::get_reload_time() const
{
    if( !is_gun() ) {
        return 0;
    }

    int reload_time = type->gun->reload_time;
    for( const auto mod : gunmods() ) {
        reload_time = int( reload_time * ( 100 + mod->type->gunmod->reload_modifier ) / 100 );
    }

    return reload_time;
}

bool item::is_silent() const
{
    return gun_noise().volume < 5;
}

bool item::is_gunmod() const
{
    return type->gunmod.has_value();
}

bool item::is_bionic() const
{
    return type->bionic.has_value();
}

bool item::is_magazine() const
{
    return type->magazine.has_value();
}

bool item::is_ammo_belt() const
{
    return is_magazine() && has_flag( "MAG_BELT" );
}

bool item::is_bandolier() const
{
    return type->can_use( "bandolier" );
}

bool item::is_holster() const
{
    return type->can_use( "holster" );
}

bool item::is_ammo() const
{
    return type->ammo.has_value();
}

bool item::is_comestible() const
{
    return type->comestible.has_value();
}

bool item::is_food() const
{
    return is_comestible() && ( type->comestible->comesttype == "FOOD" ||
                                type->comestible->comesttype == "DRINK" );
}

bool item::is_medication() const
{
    return is_comestible() && type->comestible->comesttype == "MED";
}

bool item::is_brewable() const
{
    return type->brewable.has_value();
}

bool item::is_food_container() const
{
    return !contents.empty() && contents.front().is_food();
}

bool item::is_med_container() const
{
    return !contents.empty() && contents.front().is_medication();
}

bool item::is_corpse() const
{
    return typeId() == "corpse" && corpse != nullptr;
}

const mtype *item::get_mtype() const
{
    return corpse;
}

void item::set_mtype( const mtype *const m )
{
    // This is potentially dangerous, e.g. for corpse items, which *must* have a valid mtype pointer.
    if( m == nullptr ) {
        debugmsg( "setting item::corpse of %s to NULL", tname().c_str() );
        return;
    }
    corpse = m;
}

bool item::is_ammo_container() const
{
    return !is_magazine() && !contents.empty() && contents.front().is_ammo();
}

bool item::is_melee() const
{
    for( auto idx = DT_NULL + 1; idx != NUM_DT; ++idx ) {
        if( is_melee( static_cast<damage_type>( idx ) ) ) {
            return true;
        }
    }
    return false;
}

bool item::is_melee( damage_type dt ) const
{
    return damage_melee( dt ) > MELEE_STAT;
}

const islot_armor *item::find_armor_data() const
{
    if( type->armor ) {
        return &*type->armor;
    }
    // Currently the only way to make a non-armor item into armor is to install a gun mod.
    // The gunmods are stored in the items contents, as are the contents of a container, and the
    // tools in a tool belt (a container actually), or the ammo in a quiver (container again).
    for( const auto mod : gunmods() ) {
        if( mod->type->armor ) {
            return &*mod->type->armor;
        }
    }
    return nullptr;
}

bool item::is_armor() const
{
    return find_armor_data() != nullptr || has_flag( "IS_ARMOR" );
}

bool item::is_book() const
{
    return type->book.has_value();
}

bool item::is_map() const
{
    return get_category().id() == "maps";
}

bool item::is_container() const
{
    return type->container.has_value();
}

bool item::is_watertight_container() const
{
    return type->container && type->container->watertight && type->container->seals;
}

bool item::is_non_resealable_container() const
{
    return type->container && !type->container->seals && type->container->unseals_into != "null";
}

bool item::is_bucket() const
{
    // That "preserves" part is a hack:
    // Currently all non-empty cans are effectively sealed at all times
    // Making them buckets would cause weirdness
    return type->container &&
           type->container->watertight &&
           !type->container->seals &&
           type->container->unseals_into == "null";
}

bool item::is_bucket_nonempty() const
{
    return is_bucket() && !is_container_empty();
}

bool item::is_engine() const
{
    return type->engine.has_value();
}

bool item::is_wheel() const
{
    return type->wheel.has_value();
}

bool item::is_fuel() const
{
    return type->fuel.has_value();
}

bool item::is_toolmod() const
{
    return !is_gunmod() && type->mod;
}

bool item::is_faulty() const
{
    return is_engine() ? !faults.empty() : false;
}

bool item::is_irremovable() const
{
    return has_flag( "IRREMOVABLE" );
}

std::set<fault_id> item::faults_potential() const
{
    std::set<fault_id> res;
    if( type->engine ) {
        res.insert( type->engine->faults.begin(), type->engine->faults.end() );
    }
    return res;
}

int item::wheel_area() const
{
    return is_wheel() ? type->wheel->diameter * type->wheel->width : 0;
}

float item::fuel_energy() const
{
    return is_fuel() ? type->fuel->energy : 0.0f;
}

std::string item::fuel_pump_terrain() const
{
    return is_fuel() ? type->fuel->pump_terrain : "t_null";
}

bool item::has_explosion_data() const
{
    return is_fuel() ? type->fuel->has_explode_data : false;
}

struct fuel_explosion item::get_explosion_data()
{
    static struct fuel_explosion null_data;
    return has_explosion_data() ? type->fuel->explosion_data : null_data;
}

bool item::is_container_empty() const
{
    return contents.empty();
}

bool item::is_container_full( bool allow_bucket ) const
{
    if( is_container_empty() ) {
        return false;
    }
    return get_remaining_capacity_for_liquid( contents.front(), allow_bucket ) == 0;
}

bool item::can_unload_liquid() const
{
    if( is_container_empty() ) {
        return true;
    }

    const item &cts = contents.front();
    if( !is_bucket() && cts.made_of_from_type( LIQUID ) && cts.made_of( SOLID ) ) {
        return false;
    }

    return true;
}

bool item::can_reload_with( const itype_id &ammo ) const
{
    return is_reloadable_helper( ammo, false );
}

bool item::is_reloadable_with( const itype_id &ammo ) const
{
    return is_reloadable_helper( ammo, true );
}

bool item::is_reloadable_helper( const itype_id &ammo, bool now ) const
{
    if( !is_reloadable() ) {
        return false;
    } else if( is_watertight_container() ) {
        return ( now ? !is_container_full() : true ) &&
               ( ammo.empty() || is_container_empty() || contents.front().typeId() == ammo );
    } else if( magazine_integral() ) {
        if( !ammo.empty() ) {
            if( ammo_data() ) {
                if( ammo_current() != ammo ) {
                    return false;
                }
            } else {
                auto at = find_type( ammo );
                if( ( !at->ammo || !at->ammo->type.count( ammo_type() ) ) &&
                    !magazine_compatible().count( ammo ) ) {
                    return false;
                }
            }
        }
        return now ? ( ammo_remaining() < ammo_capacity() ) : true;
    } else {
        return ammo.empty() ? true : magazine_compatible().count( ammo );
    }
}

bool item::is_salvageable() const
{
    if( is_null() ) {
        return false;
    }
    const auto mats = made_of();
    if( std::none_of( mats.begin(), mats.end(), []( const material_id & m ) {
    return m->salvaged_into().has_value();
    } ) ) {
        return false;
    }
    return !has_flag( "NO_SALVAGE" );
}

bool item::is_funnel_container( units::volume &bigger_than ) const
{
    if( !is_bucket() && !is_watertight_container() ) {
        return false;
    }
    // @todo; consider linking funnel to item or -making- it an active item
    if( get_container_capacity() <= bigger_than ) {
        return false; // skip contents check, performance
    }
    if(
        contents.empty() ||
        contents.front().typeId() == "water" ||
        contents.front().typeId() == "water_acid" ||
        contents.front().typeId() == "water_acid_weak" ) {
        bigger_than = get_container_capacity();
        return true;
    }
    return false;
}

bool item::is_emissive() const
{
    return light.luminance > 0 || type->light_emission > 0;
}

bool item::is_tool() const
{
    return type->tool.has_value();
}

bool item::is_tool_reversible() const
{
    if( is_tool() && type->tool->revert_to ) {
        item revert( *type->tool->revert_to );
        npc n;
        revert.type->invoke( n, revert, tripoint( -999, -999, -999 ) );
        return revert.is_tool() && typeId() == revert.typeId();
    }
    return false;
}

bool item::is_artifact() const
{
    return type->artifact.has_value();
}

bool item::can_contain( const item &it ) const
{
    // @todo: Volume check
    return can_contain( *it.type );
}

bool item::can_contain( const itype &tp ) const
{
    if( !type->container ) {
        // @todo: Tools etc.
        return false;
    }

    if( tp.phase == LIQUID && !type->container->watertight ) {
        return false;
    }

    // @todo: Acid in waterskins
    return true;
}

const item &item::get_contained() const
{
    if( contents.empty() ) {
        return null_item_reference();
    }
    return contents.front();
}

bool item::spill_contents( Character &c )
{
    if( !is_container() || is_container_empty() ) {
        return true;
    }

    if( c.is_npc() ) {
        return spill_contents( c.pos() );
    }

    while( !contents.empty() ) {
        on_contents_changed();
        if( contents_made_of( LIQUID ) ) {
            if( !g->handle_liquid_from_container( *this, 1 ) ) {
                return false;
            }
        } else {
            c.i_add_or_drop( contents.front() );
            contents.erase( contents.begin() );
        }
    }

    return true;
}

bool item::spill_contents( const tripoint &pos )
{
    if( !is_container() || is_container_empty() ) {
        return true;
    }

    for( item &it : contents ) {
        g->m.add_item_or_charges( pos, it );
    }

    contents.clear();
    return true;
}

int item::get_chapters() const
{
    if( !type->book ) {
        return 0;
    }
    return type->book->chapters;
}

int item::get_remaining_chapters( const player &u ) const
{
    const auto var = string_format( "remaining-chapters-%d", u.getID() );
    return get_var( var, get_chapters() );
}

void item::mark_chapter_as_read( const player &u )
{
    const auto var = string_format( "remaining-chapters-%d", u.getID() );
    if( type->book && type->book->chapters == 0 ) {
        // books without chapters will always have remaining chapters == 0, so we don't need to store them
        erase_var( var );
        return;
    }
    const int remain = std::max( 0, get_remaining_chapters( u ) - 1 );
    set_var( var, remain );
}

std::vector<std::pair<const recipe *, int>> item::get_available_recipes( const player &u ) const
{
    std::vector<std::pair<const recipe *, int>> recipe_entries;
    if( is_book() ) {
        // NPCs don't need to identify books
        if( u.is_player() && !u.has_identified( typeId() ) ) {
            return recipe_entries;
        }

        for( const auto &elem : type->book->recipes ) {
            if( u.get_skill_level( elem.recipe->skill_used ) >= elem.skill_level ) {
                recipe_entries.push_back( std::make_pair( elem.recipe, elem.skill_level ) );
            }
        }
    } else if( has_var( "EIPC_RECIPES" ) ) {
        // See einkpc_download_memory_card() in iuse.cpp where this is set.
        const std::string recipes = get_var( "EIPC_RECIPES" );
        // Capture the index one past the delimiter, i.e. start of target string.
        size_t first_string_index = recipes.find_first_of( ',' ) + 1;
        while( first_string_index != std::string::npos ) {
            size_t next_string_index = recipes.find_first_of( ',', first_string_index );
            if( next_string_index == std::string::npos ) {
                break;
            }
            std::string new_recipe = recipes.substr( first_string_index,
                                     next_string_index - first_string_index );
            const recipe *r = &recipe_id( new_recipe ).obj();
            if( u.get_skill_level( r->skill_used ) >= r->difficulty ) {
                recipe_entries.push_back( std::make_pair( r, r->difficulty ) );
            }
            first_string_index = next_string_index + 1;
        }
    }
    return recipe_entries;
}

const material_type &item::get_random_material() const
{
    return random_entry( made_of(), material_id::NULL_ID() ).obj();
}

const material_type &item::get_base_material() const
{
    const auto mats = made_of();
    return mats.empty() ? material_id::NULL_ID().obj() : mats.front().obj();
}

bool item::operator<( const item &other ) const
{
    const item_category &cat_a = get_category();
    const item_category &cat_b = other.get_category();
    if( cat_a != cat_b ) {
        return cat_a < cat_b;
    } else {
        const item *me = is_container() && !contents.empty() ? &contents.front() : this;
        const item *rhs = other.is_container() &&
                          !other.contents.empty() ? &other.contents.front() : &other;

        if( me->typeId() == rhs->typeId() ) {
            return me->charges < rhs->charges;
        } else {
            std::string n1 = me->type->nname( 1 );
            std::string n2 = rhs->type->nname( 1 );
            return std::lexicographical_compare( n1.begin(), n1.end(),
                                                 n2.begin(), n2.end(), sort_case_insensitive_less() );
        }
    }
}

skill_id item::gun_skill() const
{
    if( !is_gun() ) {
        return skill_id::NULL_ID();
    }
    return type->gun->skill_used;
}

gun_type_type item::gun_type() const
{
    static skill_id skill_archery( "archery" );

    if( !is_gun() ) {
        return gun_type_type( std::string() );
    }
    //@todo: move to JSON and remove extraction of this from "GUN" (via skill id)
    //and from "GUNMOD" (via "mod_targets") in lang/extract_json_strings.py
    if( gun_skill() == skill_archery ) {
        if( ammo_type() == ammotype( "bolt" ) || typeId() == "bullet_crossbow" ) {
            return gun_type_type( translate_marker_context( "gun_type_type", "crossbow" ) );
        } else {
            return gun_type_type( translate_marker_context( "gun_type_type", "bow" ) );
        }
    }
    return gun_type_type( gun_skill().str() );
}

skill_id item::melee_skill() const
{
    if( !is_melee() ) {
        return skill_id::NULL_ID();
    }

    if( has_flag( "UNARMED_WEAPON" ) ) {
        return skill_unarmed;
    }

    int hi = 0;
    skill_id res = skill_id::NULL_ID();

    for( auto idx = DT_NULL + 1; idx != NUM_DT; ++idx ) {
        auto val = damage_melee( static_cast<damage_type>( idx ) );
        const skill_id &sk  = skill_by_dt( static_cast<damage_type>( idx ) );
        if( val > hi && sk ) {
            hi = val;
            res = sk;
        }
    }

    return res;
}

int item::gun_dispersion( bool with_ammo, bool with_scaling ) const
{
    if( !is_gun() ) {
        return 0;
    }
    int dispersion_sum = type->gun->dispersion;
    for( const auto mod : gunmods() ) {
        dispersion_sum += mod->type->gunmod->dispersion;
    }
    int dispPerDamage = get_option< int >( "DISPERSION_PER_GUN_DAMAGE" );
    dispersion_sum += damage_level( 4 ) * dispPerDamage;
    dispersion_sum = std::max( dispersion_sum, 0 );
    if( with_ammo && ammo_data() ) {
        dispersion_sum += ammo_data()->ammo->dispersion;
    }
    if( !with_scaling ) {
        return dispersion_sum;
    }

    // Dividing dispersion by 15 temporarily as a gross adjustment,
    // will bake that adjustment into individual gun definitions in the future.
    // Absolute minimum gun dispersion is 1.
    double divider = get_option< float >( "GUN_DISPERSION_DIVIDER" );
    dispersion_sum = std::max( static_cast<int>( std::round( dispersion_sum / divider ) ), 1 );

    return dispersion_sum;
}

int item::sight_dispersion() const
{
    if( !is_gun() ) {
        return 0;
    }

    int res = has_flag( "DISABLE_SIGHTS" ) ? 90 : type->gun->sight_dispersion;

    for( const auto e : gunmods() ) {
        const auto &mod = *e->type->gunmod;
        if( mod.sight_dispersion < 0 || mod.aim_speed < 0 ) {
            continue; // skip gunmods which don't provide a sight
        }
        res = std::min( res, mod.sight_dispersion );
    }

    return res;
}

damage_instance item::gun_damage( bool with_ammo ) const
{
    if( !is_gun() ) {
        return damage_instance();
    }
    damage_instance ret = type->gun->damage;

    for( const auto mod : gunmods() ) {
        ret.add( mod->type->gunmod->damage );
    }

    if( with_ammo && ammo_data() ) {
        if( ammo_data()->ammo->prop_damage ) {
            for( auto &elem : ret.damage_units ) {
                if( elem.type == DT_STAB ) {
                    elem.amount *= *ammo_data()->ammo->prop_damage;
                    elem.res_pen = ammo_data()->ammo->legacy_pierce;
                }
            }
        } else {
            ret.add( ammo_data()->ammo->damage );
        }
    }

    int item_damage = damage_level( 4 );
    if( item_damage != 0 ) {
        // @todo This isn't a good solution for multi-damage guns/ammos
        for( damage_unit &du : ret ) {
            du.amount -= item_damage * 2;
        }
    }

    return ret;
}

int item::gun_recoil( const player &p, bool bipod ) const
{
    if( !is_gun() || ( ammo_required() && !ammo_remaining() ) ) {
        return 0;
    }

    ///\EFFECT_STR improves the handling of heavier weapons
    // we consider only base weight to avoid exploits
    double wt = std::min( type->weight, p.str_cur * 333_gram ) / 333.0_gram;

    double handling = type->gun->handling;
    for( const auto mod : gunmods() ) {
        if( bipod || !mod->has_flag( "BIPOD" ) ) {
            handling += mod->type->gunmod->handling;
        }
    }

    // rescale from JSON units which are intentionally specified as integral values
    handling /= 10;

    // algorithm is biased so heavier weapons benefit more from improved handling
    handling = pow( wt, 0.8 ) * pow( handling, 1.2 );

    int qty = type->gun->recoil;
    if( ammo_data() ) {
        qty += ammo_data()->ammo->recoil;
    }

    // handling could be either a bonus or penalty dependent upon installed mods
    if( handling > 1.0 ) {
        return qty / handling;
    } else {
        return qty * ( 1.0 + std::abs( handling ) );
    }
}

int item::gun_range( bool with_ammo ) const
{
    if( !is_gun() ) {
        return 0;
    }
    int ret = type->gun->range;
    for( const auto mod : gunmods() ) {
        ret += mod->type->gunmod->range;
    }
    if( with_ammo && ammo_data() ) {
        ret += ammo_data()->ammo->range;
    }
    return std::min( std::max( 0, ret ), RANGE_HARD_CAP );
}

int item::gun_range( const player *p ) const
{
    int ret = gun_range( true );
    if( p == nullptr ) {
        return ret;
    }
    if( !p->meets_requirements( *this ) ) {
        return 0;
    }

    // Reduce bow range until player has twice minimm required strength
    if( has_flag( "STR_DRAW" ) ) {
        ret += std::max( 0.0, ( p->get_str() - get_min_str() ) * 0.5 );
    }

    return std::max( 0, ret );
}

long item::ammo_remaining() const
{
    const item *mag = magazine_current();
    if( mag ) {
        return mag->ammo_remaining();
    }

    if( is_tool() || is_gun() ) {
        // includes auxiliary gunmods
        return charges;
    }

    if( is_magazine() || is_bandolier() ) {
        long res = 0;
        for( const auto &e : contents ) {
            res += e.charges;
        }
        return res;
    }

    return 0;
}

long item::ammo_capacity() const
{
    long res = 0;

    const item *mag = magazine_current();
    if( mag ) {
        return mag->ammo_capacity();
    }

    if( is_tool() ) {
        res = type->tool->max_charges;
        for( const auto e : toolmods() ) {
            res *= e->type->mod->capacity_multiplier;
        }
    }

    if( is_gun() ) {
        res = type->gun->clip;
        for( const auto e : gunmods() ) {
            res *= e->type->mod->capacity_multiplier;
        }
    }

    if( is_magazine() ) {
        res = type->magazine->capacity;
    }

    if( is_bandolier() ) {
        return dynamic_cast<const bandolier_actor *>
               ( type->get_use( "bandolier" )->get_actor_ptr() )->capacity;
    }

    return res;
}

long item::ammo_required() const
{
    if( is_tool() ) {
        return std::max( type->charges_to_use(), 0 );
    }

    if( is_gun() ) {
        if( !ammo_type() ) {
            return 0;
        } else if( has_flag( "FIRE_100" ) ) {
            return 100;
        } else if( has_flag( "FIRE_50" ) ) {
            return 50;
        } else if( has_flag( "FIRE_20" ) ) {
            return 20;
        } else {
            return 1;
        }
    }

    return 0;
}

bool item::ammo_sufficient( int qty ) const
{
    return ammo_remaining() >= ammo_required() * qty;
}

long item::ammo_consume( long qty, const tripoint &pos )
{
    if( qty < 0 ) {
        debugmsg( "Cannot consume negative quantity of ammo for %s", tname().c_str() );
        return 0;
    }

    item *mag = magazine_current();
    if( mag ) {
        auto res = mag->ammo_consume( qty, pos );
        if( res && ammo_remaining() == 0 ) {
            if( mag->has_flag( "MAG_DESTROY" ) ) {
                contents.erase( std::remove_if( contents.begin(), contents.end(), [&mag]( const item & e ) {
                    return mag == &e;
                } ) );
            } else if( mag->has_flag( "MAG_EJECT" ) ) {
                g->m.add_item( pos, *mag );
                contents.erase( std::remove_if( contents.begin(), contents.end(), [&mag]( const item & e ) {
                    return mag == &e;
                } ) );
            }
        }
        return res;
    }

    if( is_magazine() ) {
        auto need = qty;
        while( !contents.empty() ) {
            auto &e = *contents.rbegin();
            if( need >= e.charges ) {
                need -= e.charges;
                contents.pop_back();
            } else {
                e.charges -= need;
                need = 0;
                break;
            }
        }
        return qty - need;

    } else if( is_tool() || is_gun() ) {
        qty = std::min( qty, charges );
        charges -= qty;
        if( charges == 0 ) {
            curammo = nullptr;
        }
        return qty;
    }

    return 0;
}

const itype *item::ammo_data() const
{
    const item *mag = magazine_current();
    if( mag ) {
        return mag->ammo_data();
    }

    if( is_ammo() ) {
        return type;
    }

    if( is_magazine() ) {
        return !contents.empty() ? contents.front().ammo_data() : nullptr;
    }

    auto mods = is_gun() ? gunmods() : toolmods();
    for( const auto e : mods ) {
        if( e->type->mod->ammo_modifier &&
            item_controller->has_template( itype_id( e->type->mod->ammo_modifier.str() ) ) ) {
            return item_controller->find_template( itype_id( e->type->mod->ammo_modifier.str() ) );
        }
    }

    return curammo;
}

itype_id item::ammo_current() const
{
    const auto ammo = ammo_data();
    return ammo ? ammo->get_id() : "null";
}

ammotype item::ammo_type( bool conversion ) const
{
    if( conversion ) {
        auto mods = is_gun() ? gunmods() : toolmods();
        for( const auto e : mods ) {
            if( e->type->mod->ammo_modifier ) {
                return e->type->mod->ammo_modifier;
            }
        }
    }

    if( is_gun() ) {
        return type->gun->ammo;
    } else if( is_tool() ) {
        return type->tool->ammo_id;
    } else if( is_magazine() ) {
        return type->magazine->type;
    }
    return ammotype::NULL_ID();
}

itype_id item::ammo_default( bool conversion ) const
{
    auto res = ammo_type( conversion )->default_ammotype();
    return !res.empty() ? res : "NULL";
}

std::set<std::string> item::ammo_effects( bool with_ammo ) const
{
    if( !is_gun() ) {
        return std::set<std::string>();
    }

    std::set<std::string> res = type->gun->ammo_effects;
    if( with_ammo && ammo_data() ) {
        res.insert( ammo_data()->ammo->ammo_effects.begin(), ammo_data()->ammo->ammo_effects.end() );
    }

    for( const auto mod : gunmods() ) {
        res.insert( mod->type->gunmod->ammo_effects.begin(), mod->type->gunmod->ammo_effects.end() );
    }

    return res;
}

bool item::magazine_integral() const
{
    // If a mod sets a magazine type, we're not integral.
    for( const auto m : is_gun() ? gunmods() : toolmods() ) {
        if( !m->type->mod->magazine_adaptor.empty() ) {
            return false;
        }
    }

    // We have an integral magazine if we're a gun with an ammo capacity (clip) or we have no magazines.
    return ( is_gun() && type->gun->clip > 0 ) || type->magazines.empty();
}

itype_id item::magazine_default( bool conversion ) const
{
    auto mag = type->magazine_default.find( ammo_type( conversion ) );
    return mag != type->magazine_default.end() ? mag->second : "null";
}

std::set<itype_id> item::magazine_compatible( bool conversion ) const
{
    // mods that define magazine_adaptor may override the items usual magazines
    auto mods = is_gun() ? gunmods() : toolmods();
    for( const auto m : mods ) {
        if( !m->type->mod->magazine_adaptor.empty() ) {
            auto mags = m->type->mod->magazine_adaptor.find( ammo_type( conversion ) );
            return mags != m->type->mod->magazine_adaptor.end() ? mags->second : std::set<itype_id>();
        }
    }

    auto mags = type->magazines.find( ammo_type( conversion ) );
    return mags != type->magazines.end() ? mags->second : std::set<itype_id>();
}

item *item::magazine_current()
{
    auto iter = std::find_if( contents.begin(), contents.end(), []( const item & it ) {
        return it.is_magazine();
    } );
    return iter != contents.end() ? &*iter : nullptr;
}

const item *item::magazine_current() const
{
    return const_cast<item *>( this )->magazine_current();
}

std::vector<item *> item::gunmods()
{
    std::vector<item *> res;
    if( is_gun() ) {
        res.reserve( contents.size() );
        for( auto &e : contents ) {
            if( e.is_gunmod() ) {
                res.push_back( &e );
            }
        }
    }
    return res;
}

std::vector<const item *> item::gunmods() const
{
    std::vector<const item *> res;
    if( is_gun() ) {
        res.reserve( contents.size() );
        for( auto &e : contents ) {
            if( e.is_gunmod() ) {
                res.push_back( &e );
            }
        }
    }
    return res;
}

item *item::gunmod_find( const itype_id &mod )
{
    auto mods = gunmods();
    auto it = std::find_if( mods.begin(), mods.end(), [&mod]( item * e ) {
        return e->typeId() == mod;
    } );
    return it != mods.end() ? *it : nullptr;
}

const item *item::gunmod_find( const itype_id &mod ) const
{
    return const_cast<item *>( this )->gunmod_find( mod );
}

ret_val<bool> item::is_gunmod_compatible( const item &mod ) const
{
    if( !mod.is_gunmod() ) {
        debugmsg( "Tried checking compatibility of non-gunmod" );
        return ret_val<bool>::make_failure();
    }
    static const gun_type_type pistol_gun_type( translate_marker_context( "gun_type_type", "pistol" ) );

    if( !is_gun() ) {
        return ret_val<bool>::make_failure( _( "isn't a weapon" ) );

    } else if( is_gunmod() ) {
        return ret_val<bool>::make_failure( _( "is a gunmod and cannot be modded" ) );

    } else if( gunmod_find( mod.typeId() ) ) {
        return ret_val<bool>::make_failure( _( "already has a %s" ), mod.tname( 1 ).c_str() );

    } else if( !get_mod_locations().count( mod.type->gunmod->location ) ) {
        return ret_val<bool>::make_failure( _( "doesn't have a slot for this mod" ) );

    } else if( get_free_mod_locations( mod.type->gunmod->location ) <= 0 ) {
        return ret_val<bool>::make_failure( _( "doesn't have enough room for another %s mod" ),
                                            mod.type->gunmod->location.name().c_str() );

    } else if( !mod.type->gunmod->usable.count( gun_type() ) &&
               !mod.type->gunmod->usable.count( typeId() ) ) {
        return ret_val<bool>::make_failure( _( "cannot have a %s" ), mod.tname().c_str() );

    } else if( typeId() == "hand_crossbow" && !mod.type->gunmod->usable.count( pistol_gun_type ) ) {
        return ret_val<bool>::make_failure( _( "isn't big enough to use that mod" ) );

    } else if( mod.type->gunmod->location.str() == "underbarrel" &&
               !mod.has_flag( "PUMP_RAIL_COMPATIBLE" ) && has_flag( "PUMP_ACTION" ) ) {
        return ret_val<bool>::make_failure( _( "can only accept small mods on that slot" ) );

    } else if( !mod.type->mod->acceptable_ammo.empty() &&
               !mod.type->mod->acceptable_ammo.count( ammo_type( false ) ) ) {
        //~ %1$s - name of the gunmod, %2$s - name of the ammo
        return ret_val<bool>::make_failure( _( "%1$s cannot be used on %2$s" ), mod.tname( 1 ).c_str(),
                                            ammo_type( false )->name().c_str() );

    } else if( mod.typeId() == "waterproof_gunmod" && has_flag( "WATERPROOF_GUN" ) ) {
        return ret_val<bool>::make_failure( _( "is already waterproof" ) );

    } else if( mod.typeId() == "tuned_mechanism" && has_flag( "NEVER_JAMS" ) ) {
        return ret_val<bool>::make_failure( _( "is already eminently reliable" ) );

    } else if( mod.typeId() == "brass_catcher" && has_flag( "RELOAD_EJECT" ) ) {
        return ret_val<bool>::make_failure( _( "cannot have a brass catcher" ) );

    } else if( ( mod.type->mod->ammo_modifier || !mod.type->mod->magazine_adaptor.empty() )
               && ( ammo_remaining() > 0 || magazine_current() ) ) {
        return ret_val<bool>::make_failure( _( "must be unloaded before installing this mod" ) );
    }

    for( const auto &slot : mod.type->gunmod->blacklist_mod ) {
        if( get_mod_locations().count( slot ) ) {
            return ret_val<bool>::make_failure( _( "cannot be installed on a weapon with \"%s\"" ),
                                                slot.name().c_str() );
        }
    }

    return ret_val<bool>::make_success();
}

std::map<gun_mode_id, gun_mode> item::gun_all_modes() const
{
    std::map<gun_mode_id, gun_mode> res;

    if( !is_gun() || is_gunmod() ) {
        return res;
    }

    auto opts = gunmods();
    opts.push_back( this );

    for( const auto e : opts ) {

        // handle base item plus any auxiliary gunmods
        if( e->is_gun() ) {
            for( auto m : e->type->gun->modes ) {
                // prefix attached gunmods, e.g. M203_DEFAULT to avoid index key collisions
                std::string prefix = e->is_gunmod() ? ( std::string( e->typeId() ) += "_" ) : "";
                std::transform( prefix.begin(), prefix.end(), prefix.begin(), ( int( * )( int ) )toupper );

                auto qty = m.second.qty();

                res.emplace( gun_mode_id( prefix + m.first.str() ), gun_mode( m.second.name(),
                             const_cast<item *>( e ),
                             qty, m.second.flags() ) );
            }

            // non-auxiliary gunmods may provide additional modes for the base item
        } else if( e->is_gunmod() ) {
            for( auto m : e->type->gunmod->mode_modifier ) {
                //checks for melee gunmod, points to gunmod
                if( m.first == "REACH" ) {
                    res.emplace( m.first, gun_mode { m.second.name(), const_cast<item *>( e ),
                                                     m.second.qty(), m.second.flags() } );
                    //otherwise points to the parent gun, not the gunmod
                } else {
                    res.emplace( m.first, gun_mode { m.second.name(), const_cast<item *>( this ),
                                                     m.second.qty(), m.second.flags() } );
                }
            }
        }
    }

    return res;
}

gun_mode item::gun_get_mode( const gun_mode_id &mode ) const
{
    if( is_gun() ) {
        for( auto e : gun_all_modes() ) {
            if( e.first == mode ) {
                return e.second;
            }
        }
    }
    return gun_mode();
}

gun_mode item::gun_current_mode() const
{
    return gun_get_mode( gun_get_mode_id() );
}

gun_mode_id item::gun_get_mode_id() const
{
    if( !is_gun() || is_gunmod() ) {
        return gun_mode_id();
    }
    return gun_mode_id( get_var( GUN_MODE_VAR_NAME, "DEFAULT" ) );
}

bool item::gun_set_mode( const gun_mode_id &mode )
{
    if( !is_gun() || is_gunmod() || !gun_all_modes().count( mode ) ) {
        return false;
    }
    set_var( GUN_MODE_VAR_NAME, mode.str() );
    return true;
}

void item::gun_cycle_mode()
{
    if( !is_gun() || is_gunmod() ) {
        return;
    }

    auto cur = gun_get_mode_id();
    auto modes = gun_all_modes();

    for( auto iter = modes.begin(); iter != modes.end(); ++iter ) {
        if( iter->first == cur ) {
            if( std::next( iter ) == modes.end() ) {
                break;
            }
            gun_set_mode( std::next( iter )->first );
            return;
        }
    }
    gun_set_mode( modes.begin()->first );

    return;
}

const use_function *item::get_use( const std::string &use_name ) const
{
    if( type != nullptr && type->get_use( use_name ) != nullptr ) {
        return type->get_use( use_name );
    }

    for( const auto &elem : contents ) {
        const auto fun = elem.get_use( use_name );
        if( fun != nullptr ) {
            return fun;
        }
    }

    return nullptr;
}

item *item::get_usable_item( const std::string &use_name )
{
    if( type != nullptr && type->get_use( use_name ) != nullptr ) {
        return this;
    }

    for( auto &elem : contents ) {
        const auto fun = elem.get_use( use_name );
        if( fun != nullptr ) {
            return &elem;
        }
    }

    return nullptr;
}

int item::units_remaining( const Character &ch, int limit ) const
{
    if( count_by_charges() ) {
        return std::min( int( charges ), limit );
    }

    auto res = ammo_remaining();
    if( res < limit && has_flag( "USE_UPS" ) ) {
        res += ch.charges_of( "UPS", limit - res );
    }

    return std::min( int( res ), limit );
}

bool item::units_sufficient( const Character &ch, int qty ) const
{
    if( qty < 0 ) {
        qty = count_by_charges() ? 1 : ammo_required();
    }

    return units_remaining( ch, qty ) == qty;
}

item::reload_option::reload_option( const reload_option &rhs ) :
    who( rhs.who ), target( rhs.target ), ammo( rhs.ammo.clone() ),
    qty_( rhs.qty_ ), max_qty( rhs.max_qty ), parent( rhs.parent ) {}

item::reload_option &item::reload_option::operator=( const reload_option &rhs )
{
    who = rhs.who;
    target = rhs.target;
    ammo = rhs.ammo.clone();
    qty_ = rhs.qty_;
    max_qty = rhs.max_qty;
    parent = rhs.parent;

    return *this;
}

item::reload_option::reload_option( const player *who, const item *target, const item *parent,
                                    item_location &&ammo ) :
    who( who ), target( target ), ammo( std::move( ammo ) ), parent( parent )
{
    if( this->target->is_ammo_belt() && this->target->type->magazine->linkage ) {
        max_qty = this->who->charges_of( * this->target->type->magazine->linkage );
    }
    qty( max_qty );
}

int item::reload_option::moves() const
{
    int mv = ammo.obtain_cost( *who, qty() ) + who->item_reload_cost( *target, *ammo, qty() );
    if( parent != target ) {
        if( parent->is_gun() ) {
            mv += parent->get_reload_time();
        } else if( parent->is_tool() ) {
            mv += 100;
        }
    }
    return mv;
}

void item::reload_option::qty( long val )
{
    bool ammo_in_container = ammo->is_ammo_container();
    bool ammo_in_liquid_container = ammo->is_watertight_container();
    item &ammo_obj = ( ammo_in_container || ammo_in_liquid_container ) ?
                     ammo->contents.front() : *ammo;

    if( ( ammo_in_container && !ammo_obj.is_ammo() ) ||
        ( ammo_in_liquid_container && !ammo_obj.made_of( LIQUID ) ) ) {
        debugmsg( "Invalid reload option: %s", ammo_obj.tname().c_str() );
        return;
    }

    // Checking ammo capacity implicitly limits guns with removable magazines to capacity 0.
    // This gets rounded up to 1 later.
    long remaining_capacity = target->is_watertight_container() ?
                              target->get_remaining_capacity_for_liquid( ammo_obj, true ) :
                              target->ammo_capacity() - target->ammo_remaining();
    if( target->has_flag( "RELOAD_ONE" ) && !ammo->has_flag( "SPEEDLOADER" ) ) {
        remaining_capacity = 1;
    }
    if( target->ammo_type() == ammotype( "plutonium" ) ) {
        remaining_capacity = remaining_capacity / PLUTONIUM_CHARGES +
                             ( remaining_capacity % PLUTONIUM_CHARGES != 0 );
    }

    bool ammo_by_charges = ammo_obj.is_ammo() || ammo_in_liquid_container;
    long available_ammo = ammo_by_charges ? ammo_obj.charges : ammo_obj.ammo_remaining();
    // constrain by available ammo, target capacity and other external factors (max_qty)
    // @ref max_qty is currently set when reloading ammo belts and limits to available linkages
    qty_ = std::min( { val, available_ammo, remaining_capacity, max_qty } );

    // always expect to reload at least one charge
    qty_ = std::max( qty_, 1L );

}

int item::casings_count() const
{
    int res = 0;

    const_cast<item *>( this )->casings_handle( [&res]( item & ) {
        ++res;
        return false;
    } );

    return res;
}

void item::casings_handle( const std::function<bool( item & )> &func )
{
    if( !is_gun() ) {
        return;
    }

    for( auto it = contents.begin(); it != contents.end(); ) {
        if( it->has_flag( "CASING" ) ) {
            it->unset_flag( "CASING" );
            if( func( *it ) ) {
                it = contents.erase( it );
                continue;
            }
            // didn't handle the casing so reset the flag ready for next call
            it->set_flag( "CASING" );
        }
        ++it;
    }
}

bool item::reload( player &u, item_location loc, long qty )
{
    if( qty <= 0 ) {
        debugmsg( "Tried to reload zero or less charges" );
        return false;
    }
    item *ammo = loc.get_item();
    if( ammo == nullptr || ammo->is_null() ) {
        debugmsg( "Tried to reload using non-existent ammo" );
        return false;
    }

    item *container = nullptr;
    if( ammo->is_ammo_container() || ammo->is_container() ) {
        container = ammo;
        ammo = &ammo->contents.front();
    }

    if( !is_reloadable_with( ammo->typeId() ) ) {
        return false;
    }

    // limit quantity of ammo loaded to remaining capacity
    long limit = is_watertight_container()
                 ? get_remaining_capacity_for_liquid( *ammo )
                 : ammo_capacity() - ammo_remaining();

    if( ammo_type() == ammotype( "plutonium" ) ) {
        limit = limit / PLUTONIUM_CHARGES + ( limit % PLUTONIUM_CHARGES != 0 );
    }

    qty = std::min( qty, limit );

    casings_handle( [&u]( item & e ) {
        return u.i_add_or_drop( e );
    } );

    if( is_magazine() ) {
        qty = std::min( qty, ammo->charges );

        if( is_ammo_belt() && type->magazine->linkage ) {
            if( !u.use_charges_if_avail( *type->magazine->linkage, qty ) ) {
                debugmsg( "insufficient linkages available when reloading ammo belt" );
            }
        }

        contents.emplace_back( *ammo );
        contents.back().charges = qty;
        ammo->charges -= qty;

    } else if( is_watertight_container() ) {
        if( !ammo->made_of_from_type( LIQUID ) ) {
            debugmsg( "Tried to reload liquid container with non-liquid." );
            return false;
        }
        if( !ammo->made_of( LIQUID ) ) {
            u.add_msg_if_player( m_bad, _( "The %s froze solid before you could finish." ),
                                 ammo->tname().c_str() );
            return false;
        }
        if( container ) {
            container->on_contents_changed();
        }
        fill_with( *ammo, qty );
    } else if( !magazine_integral() ) {
        // if we already have a magazine loaded prompt to eject it
        if( magazine_current() ) {
            std::string prompt = string_format( _( "Eject %s from %s?" ),
                                                magazine_current()->tname().c_str(), tname().c_str() );

            // eject magazine to player inventory and try to dispose of it from there
            item &mag = u.i_add( *magazine_current() );
            if( !u.dispose_item( item_location( u, &mag ), prompt ) ) {
                u.remove_item( mag ); // user canceled so delete the clone
                return false;
            }
            remove_item( *magazine_current() );
        }

        contents.emplace_back( *ammo );
        loc.remove_item();
        return true;

    } else {
        if( ammo->has_flag( "SPEEDLOADER" ) ) {
            curammo = find_type( ammo->contents.front().typeId() );
            qty = std::min( qty, ammo->ammo_remaining() );
            ammo->ammo_consume( qty, tripoint_zero );
            charges += qty;
        } else if( ammo_type() == ammotype( "plutonium" ) ) {
            curammo = find_type( ammo->typeId() );
            ammo->charges -= qty;

            // any excess is wasted rather than overfilling the item
            charges += qty * PLUTONIUM_CHARGES;
            charges = std::min( charges, ammo_capacity() );
        } else {
            curammo = find_type( ammo->typeId() );
            qty = std::min( qty, ammo->charges );
            ammo->charges -= qty;
            charges += qty;
        }
    }

    if( ammo->charges == 0 && !ammo->has_flag( "SPEEDLOADER" ) ) {
        if( container != nullptr ) {
            container->contents.erase( container->contents.begin() );
            u.inv.restack( u ); // emptied containers do not stack with non-empty ones
        } else {
            loc.remove_item();
        }
    }
    return true;
}

float item::simulate_burn( fire_data &frd ) const
{
    const auto &mats = made_of();
    float smoke_added = 0.0f;
    float time_added = 0.0f;
    float burn_added = 0.0f;
    const units::volume vol = base_volume();
    const int effective_intensity = frd.contained ? 3 : frd.fire_intensity;
    for( const auto &m : mats ) {
        const auto &bd = m.obj().burn_data( effective_intensity );
        if( bd.immune ) {
            // Made to protect from fire
            return 0.0f;
        }

        // If fire is contained, burn rate is independent of volume
        if( frd.contained || bd.volume_per_turn == 0_ml ) {
            time_added += bd.fuel;
            smoke_added += bd.smoke;
            burn_added += bd.burn;
        } else {
            double volume_burn_rate = to_liter( bd.volume_per_turn ) / to_liter( vol );
            time_added += bd.fuel * volume_burn_rate;
            smoke_added += bd.smoke * volume_burn_rate;
            burn_added += bd.burn * volume_burn_rate;
        }
    }

    // Liquids that don't burn well smother fire well instead
    if( made_of( LIQUID ) && time_added < 200 ) {
        time_added -= rng( 400.0 * to_liter( vol ), 1200.0 * to_liter( vol ) );
    } else if( mats.size() > 1 ) {
        // Average the materials
        time_added /= mats.size();
        smoke_added /= mats.size();
        burn_added /= mats.size();
    } else if( mats.empty() ) {
        // Non-liquid items with no specified materials will burn at moderate speed
        burn_added = 1;
    }

    frd.fuel_produced += time_added;
    frd.smoke_produced += smoke_added;
    return burn_added;
}

bool item::burn( fire_data &frd )
{
    float burn_added = simulate_burn( frd );

    if( burn_added <= 0 ) {
        return false;
    }

    if( count_by_charges() ) {
        burn_added *= rng( type->stack_size / 2, type->stack_size );
        charges -= roll_remainder( burn_added );
        if( charges <= 0 ) {
            return true;
        }
    }

    if( is_corpse() ) {
        const mtype *mt = get_mtype();
        if( active && mt != nullptr && burnt + burn_added > mt->hp &&
            !mt->burn_into.is_null() && mt->burn_into.is_valid() ) {
            corpse = &get_mtype()->burn_into.obj();
            // Delay rezing
            set_age( 0_turns );
            burnt = 0;
            return false;
        }

        if( burnt + burn_added > mt->hp ) {
            active = false;
        }
    } else if( is_food() ) {
        heat_up();
    } else if( is_food_container() ) {
        contents.front().heat_up();
    }

    burnt += roll_remainder( burn_added );

    const int vol = base_volume() / units::legacy_volume_factor;
    return burnt >= vol * 3;
}

bool item::flammable( int threshold ) const
{
    const auto &mats = made_of_types();
    if( mats.empty() ) {
        // Don't know how to burn down something made of nothing.
        return false;
    }

    int flammability = 0;
    units::volume volume_per_turn = 0_ml;
    for( const auto &m : mats ) {
        const auto &bd = m->burn_data( 1 );
        if( bd.immune ) {
            // Made to protect from fire
            return false;
        }

        flammability += bd.fuel;
        volume_per_turn += bd.volume_per_turn;
    }

    if( threshold == 0 || flammability <= 0 ) {
        return flammability > 0;
    }

    volume_per_turn /= mats.size();
    units::volume vol = base_volume();
    if( volume_per_turn > 0_ml && volume_per_turn < vol ) {
        flammability = flammability * volume_per_turn / vol;
    } else {
        // If it burns well, it provides a bonus here
        flammability *= vol / units::legacy_volume_factor;
    }

    return flammability > threshold;
}

itype_id item::typeId() const
{
    return type ? type->get_id() : "null";
}

bool item::getlight( float &luminance, int &width, int &direction ) const
{
    luminance = 0;
    width = 0;
    direction = 0;
    if( light.luminance > 0 ) {
        luminance = static_cast<float>( light.luminance );
        if( light.width > 0 ) {  // width > 0 is a light arc
            width = light.width;
            direction = light.direction;
        }
        return true;
    } else {
        const int lumint = getlight_emit();
        if( lumint > 0 ) {
            luminance = static_cast<float>( lumint );
            return true;
        }
    }
    return false;
}

int item::getlight_emit() const
{
    float lumint = type->light_emission;

    if( lumint == 0 ) {
        return 0;
    }
    if( has_flag( "CHARGEDIM" ) && is_tool() && !has_flag( "USE_UPS" ) ) {
        // Falloff starts at 1/5 total charge and scales linearly from there to 0.
        if( ammo_capacity() && ammo_remaining() < ( ammo_capacity() / 5 ) ) {
            lumint *= ammo_remaining() * 5.0 / ammo_capacity();
        }
    }
    return lumint;
}

units::volume item::get_container_capacity() const
{
    if( !is_container() ) {
        return 0_ml;
    }
    return type->container->contains;
}

units::volume item::get_total_capacity() const
{
    auto result = get_container_capacity();

    // Consider various iuse_actors which add containing capability
    // Treating these two as special cases for now; if more appear in the
    // future then this probably warrants a new method on use_function to
    // access this information generically.
    if( is_bandolier() ) {
        result += dynamic_cast<const bandolier_actor *>
                  ( type->get_use( "bandolier" )->get_actor_ptr() )->max_stored_volume();
    }

    if( is_holster() ) {
        result += dynamic_cast<const holster_actor *>
                  ( type->get_use( "holster" )->get_actor_ptr() )->max_stored_volume();
    }

    return result;
}

long item::get_remaining_capacity_for_liquid( const item &liquid, bool allow_bucket,
        std::string *err ) const
{
    const auto error = [ &err ]( const std::string & message ) {
        if( err != nullptr ) {
            *err = message;
        }
        return 0;
    };

    long remaining_capacity = 0;

    // TODO(sm): is_reloadable_with and this function call each other and can recurse for
    // watertight containers.
    if( !is_container() && is_reloadable_with( liquid.typeId() ) ) {
        if( ammo_remaining() != 0 && ammo_current() != liquid.typeId() ) {
            return error( string_format( _( "You can't mix loads in your %s." ), tname().c_str() ) );
        }
        remaining_capacity = ammo_capacity() - ammo_remaining();
    } else if( is_container() ) {
        if( !type->container->watertight ) {
            return error( string_format( _( "That %s isn't water-tight." ), tname().c_str() ) );
        } else if( !type->container->seals && ( !allow_bucket || !is_bucket() ) ) {
            return error( string_format( is_bucket() ?
                                         _( "That %s must be on the ground or held to hold contents!" )
                                         : _( "You can't seal that %s!" ), tname().c_str() ) );
        } else if( !contents.empty() && contents.front().typeId() != liquid.typeId() ) {
            return error( string_format( _( "You can't mix loads in your %s." ), tname().c_str() ) );
        }
        remaining_capacity = liquid.charges_per_volume( get_container_capacity() );
        if( !contents.empty() ) {
            remaining_capacity -= contents.front().charges;
        }
    } else {
        return error( string_format( _( "That %1$s won't hold %2$s." ), tname().c_str(),
                                     liquid.tname().c_str() ) );
    }

    if( remaining_capacity <= 0 ) {
        return error( string_format( _( "Your %1$s can't hold any more %2$s." ), tname().c_str(),
                                     liquid.tname().c_str() ) );
    }

    return remaining_capacity;
}

long item::get_remaining_capacity_for_liquid( const item &liquid, const Character &p,
        std::string *err ) const
{
    const bool allow_bucket = this == &p.weapon || !p.has_item( *this );
    long res = get_remaining_capacity_for_liquid( liquid, allow_bucket, err );

    if( res > 0 && !type->rigid && p.inv.has_item( *this ) ) {
        const units::volume volume_to_expand = std::max( p.volume_capacity() - p.volume_carried(),
                                               0_ml );

        res = std::min( liquid.charges_per_volume( volume_to_expand ), res );

        if( res == 0 && err != nullptr ) {
            *err = string_format( _( "That %s doesn't have room to expand." ), tname().c_str() );
        }
    }

    return res;
}

bool item::use_amount( const itype_id &it, long &quantity, std::list<item> &used,
                       const std::function<bool( const item & )> &filter )
{
    // Remember quantity so that we can unseal self
    long old_quantity = quantity;
    // First, check contents
    for( auto a = contents.begin(); a != contents.end() && quantity > 0; ) {
        if( a->use_amount( it, quantity, used ) ) {
            a = contents.erase( a );
        } else {
            ++a;
        }
    }

    if( quantity != old_quantity ) {
        on_contents_changed();
    }

    // Now check the item itself
    if( typeId() == it && quantity > 0 && filter( *this ) ) {
        used.push_back( *this );
        quantity--;
        return true;
    } else {
        return false;
    }
}

bool item::allow_crafting_component() const
{
    if( is_toolmod() && is_irremovable() ) {
        return false;
    }

    // vehicle batteries are implemented as magazines of charge
    if( is_magazine() && ammo_type() == ammotype( "battery" ) ) {
        return true;
    }

    // fixes #18886 - turret installation may require items with irremovable mods
    if( is_gun() ) {
        return std::all_of( contents.begin(), contents.end(), [&]( const item & e ) {
            return e.is_magazine() || ( e.is_gunmod() && e.is_irremovable() );
        } );
    }

    return contents.empty();
}

int item::get_static_temp_counter() const
{
    int counters = item_counter;
    if( item_tags.count( "FROZEN" ) ) {
        counters = -1200 - counters;
    } else if( item_tags.count( "COLD" ) ) {
        counters = -600 - counters;
    } else if( item_tags.count( "HOT" ) ) {
        counters += 600;
    } else if( !item_tags.count( "WARM" ) ) { // if not warm, then ticking for cold
        counters = -counters;
    }
    return counters;
}

void item::set_temp_from_static( const int counters )
{
    item_tags.erase( "COLD" );
    item_tags.erase( "HOT" );

    if( counters < -1200 ) {
        item_tags.insert( "FROZEN" );
        item_counter = -counters - 1200;
        if( current_phase == LIQUID ) {
            current_phase = SOLID;
        }
    } else if( counters < -600 ) {
        item_tags.insert( "COLD" );
        item_counter = -counters - 600;
        apply_freezerburn();
    } else if( counters < 0 ) {
        item_counter = -counters;
        apply_freezerburn();
    } else if( counters > 600 ) {
        item_counter = counters - 600;
        item_tags.insert( "HOT" );
        // item is limited to how much heat it can retain by its mass
        const int limit = clamp( to_gram( weight() ), 100, 600 );
        item_counter = std::min( limit, item_counter );
        apply_freezerburn();
        return;
    } else {
        item_tags.insert( "WARM" );
        item_counter = counters;
        apply_freezerburn();
    }
    // it shouldn't be possible to be outside these bounds, but just as a
    // safety precaution in case the math goes weird
    item_counter = clamp( item_counter, 0, 600 );
}

void item::fill_with( item &liquid, long amount )
{
    amount = std::min( get_remaining_capacity_for_liquid( liquid, true ),
                       std::min( amount, liquid.charges ) );
    if( amount <= 0 ) {
        return;
    }

    if( !is_container() ) {
        if( !is_reloadable_with( liquid.typeId() ) ) {
            debugmsg( "Tried to fill %s which is not a container and can't be reloaded with %s.",
                      tname().c_str(), liquid.tname().c_str() );
            return;
        }
        ammo_set( liquid.typeId(), ammo_remaining() + amount );
    } else if( !is_container_empty() ) {
        // if container already has liquid we need to average temperature
        item &cts = contents.front();
        const int lhs_counters = cts.get_static_temp_counter() * cts.charges;
        const int rhs_counters = liquid.get_static_temp_counter() * amount;
        const int avg_counters = ( lhs_counters + rhs_counters ) /
                                 ( cts.charges + amount );
        cts.set_temp_from_static( avg_counters );
        // use maximum rot between the two
        cts.set_relative_rot( std::max( cts.get_relative_rot(),
                                        liquid.get_relative_rot() ) );
        cts.mod_charges( amount );
    } else {
        item liquid_copy( liquid );
        liquid_copy.charges = amount;
        put_in( liquid_copy );
    }

    liquid.mod_charges( -amount );
    on_contents_changed();
}

void item::set_countdown( int num_turns )
{
    if( num_turns < 0 ) {
        debugmsg( "Tried to set a negative countdown value %d.", num_turns );
        return;
    }
    if( ammo_type() ) {
        debugmsg( "Tried to set countdown on an item with ammo=%s.", ammo_type().c_str() );
        return;
    }
    charges = num_turns;
}

bool item::use_charges( const itype_id &what, long &qty, std::list<item> &used,
                        const tripoint &pos )
{
    std::vector<item *> del;

    // Remember qty to unseal self
    long old_qty = qty;
    visit_items( [&what, &qty, &used, &pos, &del]( item * e ) {
        if( qty == 0 ) {
            // found sufficient charges
            return VisitResponse::ABORT;
        }

        if( e->is_tool() ) {
            if( e->typeId() == what ) {
                int n = std::min( e->ammo_remaining(), qty );
                qty -= n;

                used.push_back( item( *e ).ammo_set( e->ammo_current(), n ) );
                e->ammo_consume( n, pos );
            }
            return VisitResponse::SKIP;

        } else if( e->count_by_charges() ) {
            if( e->typeId() == what ) {

                // if can supply excess charges split required off leaving remainder in-situ
                item obj = e->split( qty );
                if( !obj.is_null() ) {
                    used.push_back( obj );
                    qty = 0;
                    return VisitResponse::ABORT;
                }

                qty -= e->charges;
                used.push_back( *e );
                del.push_back( e );
            }
            // items counted by charges are not themselves expected to be containers
            return VisitResponse::SKIP;
        }

        // recurse through any nested containers
        return VisitResponse::NEXT;
    } );

    bool destroy = false;
    for( auto e : del ) {
        if( e == this ) {
            destroy = true; // cannot remove ourselves...
        } else {
            remove_item( *e );
        }
    }

    if( qty != old_qty || !del.empty() ) {
        on_contents_changed();
    }

    return destroy;
}

void item::set_snippet( const std::string &snippet_id )
{
    if( is_null() ) {
        return;
    }
    if( type->snippet_category.empty() ) {
        debugmsg( "can not set description for item %s without snippet category", typeId().c_str() );
        return;
    }
    const int hash = SNIPPET.get_snippet_by_id( snippet_id );
    if( SNIPPET.get( hash ).empty() ) {
        debugmsg( "snippet id %s is not contained in snippet category %s", snippet_id.c_str(),
                  type->snippet_category.c_str() );
        return;
    }
    note = hash;
}

const item_category &item::get_category() const
{
    if( is_container() && !contents.empty() ) {
        return contents.front().get_category();
    }

    static item_category null_category;
    return type->category ? *type->category : null_category;
}

iteminfo::iteminfo( const std::string &Type, const std::string &Name, const std::string &Fmt,
                    flags Flags, double Value )
{
    sType = Type;
    sName = replace_colors( Name );
    sFmt = replace_colors( Fmt );
    is_int = !( Flags & is_decimal );
    dValue = Value;
    bShowPlus = static_cast<bool>( Flags & show_plus );
    std::stringstream convert;
    if( bShowPlus ) {
        convert << std::showpos;
    }
    if( is_int ) {
        convert << std::setprecision( 0 );
    } else {
        convert << std::setprecision( 2 );
    }
    convert << std::fixed << Value;
    sValue = convert.str();
    bNewLine = !( Flags & no_newline );
    bLowerIsBetter = static_cast<bool>( Flags & lower_is_better );
    bDrawName = !( Flags & no_name );
}

iteminfo::iteminfo( const std::string &Type, const std::string &Name, double Value )
    : iteminfo( Type, Name, "", no_flags, Value )
{
}

bool item::will_explode_in_fire() const
{
    if( type->explode_in_fire ) {
        return true;
    }

    if( type->ammo && ( type->ammo->special_cookoff || type->ammo->cookoff ) ) {
        return true;
    }

    // Most containers do nothing to protect the contents from fire
    if( !is_magazine() || !type->magazine->protects_contents ) {
        return std::any_of( contents.begin(), contents.end(), []( const item & it ) {
            return it.will_explode_in_fire();
        } );
    }

    return false;
}

bool item::detonate( const tripoint &p, std::vector<item> &drops )
{
    if( type->explosion.power >= 0 ) {
        g->explosion( p, type->explosion );
        return true;
    } else if( type->ammo && ( type->ammo->special_cookoff || type->ammo->cookoff ) ) {
        long charges_remaining = charges;
        const long rounds_exploded = rng( 1, charges_remaining );
        // Yank the exploding item off the map for the duration of the explosion
        // so it doesn't blow itself up.
        item temp_item = *this;
        const islot_ammo &ammo_type = *type->ammo;

        if( ammo_type.special_cookoff ) {
            // If it has a special effect just trigger it.
            apply_ammo_effects( p, ammo_type.ammo_effects );
        }
        charges_remaining -= rounds_exploded;
        if( charges_remaining > 0 ) {
            temp_item.charges = charges_remaining;
            drops.push_back( temp_item );
        }

        return true;
    } else if( !contents.empty() && ( !type->magazine || !type->magazine->protects_contents ) ) {
        const auto new_end = std::remove_if( contents.begin(), contents.end(), [ &p, &drops ]( item & it ) {
            return it.detonate( p, drops );
        } );
        if( new_end != contents.end() ) {
            contents.erase( new_end, contents.end() );
            // If any of the contents explodes, so does the container
            return true;
        }
    }

    return false;
}

bool item_ptr_compare_by_charges( const item *left, const item *right )
{
    if( left->contents.empty() ) {
        return false;
    } else if( right->contents.empty() ) {
        return true;
    } else {
        return right->contents.front().charges < left->contents.front().charges;
    }
}

bool item_compare_by_charges( const item &left, const item &right )
{
    return item_ptr_compare_by_charges( &left, &right );
}

static const std::string USED_BY_IDS( "USED_BY_IDS" );
bool item::already_used_by_player( const player &p ) const
{
    const auto it = item_vars.find( USED_BY_IDS );
    if( it == item_vars.end() ) {
        return false;
    }
    // USED_BY_IDS always starts *and* ends with a ';', the search string
    // ';<id>;' matches at most one part of USED_BY_IDS, and only when exactly that
    // id has been added.
    const std::string needle = string_format( ";%d;", p.getID() );
    return it->second.find( needle ) != std::string::npos;
}

void item::mark_as_used_by_player( const player &p )
{
    std::string &used_by_ids = item_vars[ USED_BY_IDS ];
    if( used_by_ids.empty() ) {
        // *always* start with a ';'
        used_by_ids = ";";
    }
    // and always end with a ';'
    used_by_ids += string_format( "%d;", p.getID() );
}

bool item::can_holster( const item &obj, bool ignore ) const
{
    if( !type->can_use( "holster" ) ) {
        return false; // item is not a holster
    }

    auto ptr = dynamic_cast<const holster_actor *>( type->get_use( "holster" )->get_actor_ptr() );
    if( !ptr->can_holster( obj ) ) {
        return false; // item is not a suitable holster for obj
    }

    if( !ignore && static_cast<int>( contents.size() ) >= ptr->multi ) {
        return false; // item is already full
    }

    return true;
}

std::string item::components_to_string() const
{
    typedef std::map<std::string, int> t_count_map;
    t_count_map counts;
    for( const auto &elem : components ) {
        const std::string name = elem.display_name();
        if( !elem.has_flag( "BYPRODUCT" ) ) {
            counts[name]++;
        }
    }
    return enumerate_as_string( counts.begin(), counts.end(),
    []( const std::pair<std::string, int> &entry ) -> std::string {
        if( entry.second != 1 )
        {
            return string_format( _( "%d x %s" ), entry.second, entry.first.c_str() );
        } else
        {
            return entry.first;
        }
    }, enumeration_conjunction::none );
}

bool item::needs_processing() const
{
    return active || has_flag( "RADIO_ACTIVATION" ) ||
           ( is_container() && !contents.empty() && contents.front().needs_processing() ) ||
           is_artifact() || ( is_food() );
}

int item::processing_speed() const
{
    if( is_corpse() || is_food() || is_food_container() ) {
        return 100;
    }
    // Unless otherwise indicated, update every turn.
    return 1;
}

void item::apply_freezerburn()
{
    if( !item_tags.count( "FROZEN" ) ) {
        return;
    }
    item_tags.erase( "FROZEN" );
    current_phase = type->phase;

    if( !has_flag( "FREEZERBURN" ) ) {
        return;
    }
    if( !item_tags.count( "MUSHY" ) ) {
        item_tags.insert( "MUSHY" );
    } else {
        set_relative_rot( 1.01 );
    }
}

static int temp_difference_ratio( const int temp_one, const int temp_two )
{
    // increments of 10F (~5.5C), clamped to 1-4F (~0.55-2.22C)
    return clamp( abs( temp_one - temp_two ) / 10,  1, 4 );
}

void item::update_temp( const int temp, const float insulation )
{
    const time_point now = calendar::turn;
    const time_duration dur = now - last_temp_check;

    // if player debug menu'd the time backward it breaks stuff, just reset the
    // last_temp_check in this case
    if( dur < 0_turns ) {
        last_temp_check = now;
        return;
    }

    // only process temperature at most every 10_turns, note we're also gated
    // by item::processing_speed
    if( dur > 10_turns ) {
        calc_temp( temp, insulation, dur );
        last_temp_check = now;
    }
}

void item::calc_temp( const int temp, const float insulation, const time_duration &time )
{
    // we simulate the maximum temperature an item can retain by its mass
    // TODO: Limit max frozen by this as well (don't forget to fix deep freeze
    // parasites if you do).
    const int max_heat = clamp( to_gram( weight() ), 100, 600 );
    const int freeze_point = type->comestible->freeze_point;
    bool is_hot = item_tags.count( "HOT" );
    bool is_warm = item_tags.count( "WARM" );
    bool is_cold = item_tags.count( "COLD" );
    bool is_frozen = item_tags.count( "FROZEN" );
    int loop_diff = 0;
    int diff = is_frozen ? temp_difference_ratio( temp, freeze_point ) :
               is_cold ? temp_difference_ratio( temp, temperatures::cold ) :
               is_hot || is_warm ? temp_difference_ratio( temp, temperatures::hot ) :
               temp_difference_ratio( temp, temperatures::normal );
    diff *= to_turns<int>( time );
    diff /= std::max( insulation, static_cast<float>( 0.1 ) );
    // no matter how much insulation temperature will shift at least a 1 degree
    // every ten turns if less than cold
    diff = std::max( diff, 1 );

    // process diff in chunks of 600 because that's the barrier at which a
    // something can become frozen or cold, we do this once here and at the end
    // of the loop to avoid looping twice if possible
    loop_diff = std::min( 600, diff );
    if( diff >= 600 ) {
        diff -= 600;
    } else  {
        diff = 0;
    }

    do {
        if( is_hot ) {
            if( temp >= temperatures::hot ) {
                item_counter += loop_diff;
            } else {
                item_counter -= loop_diff;
            }
            if( item_counter <= 0 ) {
                item_tags.erase( "HOT" );
                item_tags.insert( "WARM" );
                item_counter = 600;
                is_warm = true;
                is_hot = false;
            } else if( item_counter > max_heat ) {
                // item cannot heat any further
                item_counter = max_heat;
                return;
            }
        } else if( is_warm ) {
            if( temp >= temperatures::hot ) {
                item_counter += loop_diff;
            } else {
                item_counter -= loop_diff;
            }

            if( item_counter <= 0 ) {
                item_tags.erase( "WARM" );
                item_counter = 0;
                if( temp > temperatures::cold ) {
                    // item cannot cool any further, we're done
                    return;
                }
                is_warm = false;
            } else if( item_counter > 600 ) {
                item_tags.erase( "WARM" );
                item_tags.insert( "HOT" );
                is_warm = false;
                is_hot = true;
            }
        } else if( is_cold ) {
            if( temp <= temperatures::cold ) {
                item_counter += loop_diff;
                if( item_counter > 600 ) {
                    if( temp <= freeze_point ) {
                        // if temp is colder than freeze point start ticking frozen
                        item_tags.erase( "COLD" );
                        item_tags.insert( "FROZEN" );
                        current_phase = SOLID;
                        item_counter = 1;
                        is_cold = false;
                        is_frozen = true;
                    } else {
                        // if temp is warmer than freezing, cold counter cannot
                        // exceed 600 and we're done here because item cannot cool
                        // off any further
                        item_counter = 600;
                        return;
                    }
                }
            } else {
                item_counter -= loop_diff;
                // if item is cold and counter is decreasing, we can only be in a
                // warmer temp, so we're done if it's less than zero, if it's
                // greater than 600 that means we're still cooling off and need to
                // transition to frozen.
                if( item_counter <= 0 ) {
                    item_tags.erase( "COLD" );
                    item_counter = 0;
                    return;
                }
            }
        } else if( is_frozen ) {
            if( temp <= freeze_point ) {
                item_counter += loop_diff;
                if( item_counter > 500 && type->comestible->parasites > 0 ) {
                    item_tags.insert( "NO_PARASITES" );
                }

                // counter cannot exceed 600 as frozen, if it does we know
                // we're done because item is cooling down and cannot cool any
                // further
                if( item_counter > 600 ) {
                    item_counter = 600;
                    return;
                }
            } else {
                item_counter -= loop_diff;
                // item is defrosting
                if( item_counter < 0 ) {
                    apply_freezerburn(); // removes frozen tag and applies mushy/rot if needed
                    item_tags.insert( "COLD" );
                    item_counter = 600;
                    is_frozen = false;
                    is_cold = true;
                }
            }
        } else if( temp <= temperatures::cold ) {
            // see if we need to add a cold tag
            item_counter += loop_diff;
            if( item_counter > 600 ) {
                item_tags.insert( "COLD" );
                item_counter = 1;
                is_cold = true;
            }
        } else if( temp >= temperatures::hot ) {
            // if we have item_counters without a tag we were starting to
            // increment toward cold, remove counters until we don't have any,
            // then add warm tag and start to increment with WARM tag
            item_counter -= loop_diff;
            if( item_counter < 0 ) {
                item_tags.insert( "WARM" );
                item_counter = std::max( 600, -item_counter );
                is_warm = true;
            }
        } else {
            // no tags yet and not heating or cooling
            item_counter -= loop_diff;
            if( item_counter < 0 ) {
                item_counter = 0;
                return;
            }
        }

        loop_diff = std::min( 600, diff );
        if( diff >= 600 ) {
            diff -= 600;
        } else  {
            diff = 0;
        }
    } while( loop_diff > 0 || item_counter < 0 || item_counter > 600 );
}

void item::heat_up()
{
    item_tags.erase( "COLD" );
    item_tags.erase( "FROZEN" );
    item_tags.erase( "WARM" );
    item_tags.insert( "HOT" );
    // links the amount of heat an item can retain to its mass
    item_counter = clamp( to_gram( weight() ), 100, 600 );
    reset_temp_check();
}

void item::reset_temp_check()
{
    last_temp_check = calendar::turn;
}

bool item::process_food( const player *carrier, const tripoint &p, int temp, float insulation )
{
    if( carrier != nullptr && carrier->has_item( *this ) ) {
        temp += 5; // body heat increases inventory temperature
        insulation *= 1.5; // clothing provides inventory some level of insulation
    }

    // temperature can affect rot, so do it first
    update_temp( temp, insulation );
    calc_rot( p );
    return false;
}

void item::process_artifact( player *carrier, const tripoint & /*pos*/ )
{
    if( !is_artifact() ) {
        return;
    }
    // Artifacts are currently only useful for the player character, the messages
    // don't consider npcs. Also they are not processed when laying on the ground.
    // TODO: change game::process_artifact to work with npcs,
    // TODO: consider moving game::process_artifact here.
    if( carrier == &g->u ) {
        g->process_artifact( *this, *carrier );
    }
}

bool item::process_corpse( player *carrier, const tripoint &pos )
{
    // some corpses rez over time
    if( corpse == nullptr ) {
        return false;
    }
    if( !ready_to_revive( pos ) ) {
        return false;
    }

    active = false;
    if( rng( 0, volume() / units::legacy_volume_factor ) > burnt && g->revive_corpse( pos, *this ) ) {
        if( carrier == nullptr ) {
            if( g->u.sees( pos ) ) {
                if( corpse->in_species( ROBOT ) ) {
                    add_msg( m_warning, _( "A nearby robot has repaired itself and stands up!" ) );
                } else {
                    add_msg( m_warning, _( "A nearby corpse rises and moves towards you!" ) );
                }
            }
        } else {
            //~ %s is corpse name
            carrier->add_memorial_log( pgettext( "memorial_male", "Had a %s revive while carrying it." ),
                                       pgettext( "memorial_female", "Had a %s revive while carrying it." ),
                                       tname().c_str() );
            if( corpse->in_species( ROBOT ) ) {
                carrier->add_msg_if_player( m_warning,
                                            _( "Oh dear god, a robot you're carrying has started moving!" ) );
            } else {
                carrier->add_msg_if_player( m_warning,
                                            _( "Oh dear god, a corpse you're carrying has started moving!" ) );
            }
        }
        // Destroy this corpse item
        return true;
    }

    return false;
}

bool item::process_fake_smoke( player * /*carrier*/, const tripoint &pos )
{
    if( g->m.furn( pos ) != furn_str_id( "f_smoking_rack_active" ) ) {
        item_counter = 0;
        return true; //destroy fake smoke
    }

    if( age() >= 6_hours || item_counter == 0 ) {
        iexamine::on_smoke_out( pos, birthday() ); //activate effects when timers goes to zero
        return true; //destroy fake smoke when it 'burns out'
    }

    return false;
}

bool item::process_litcig( player *carrier, const tripoint &pos )
{
    process_extinguish( carrier, pos );
    // process_extinguish might have extinguished the item already
    if( !active ) {
        return false;
    }
    field_id smoke_type;
    if( has_flag( "TOBACCO" ) ) {
        smoke_type = fd_cigsmoke;
    } else {
        smoke_type = fd_weedsmoke;
    }
    // if carried by someone:
    if( carrier != nullptr ) {
        // only puff every other turn
        if( item_counter % 2 == 0 ) {
            time_duration duration = 1_minutes;
            if( carrier->has_trait( trait_id( "TOLERANCE" ) ) ) {
                duration = 5_turns;
            } else if( carrier->has_trait( trait_id( "LIGHTWEIGHT" ) ) ) {
                duration = 2_minutes;
            }
            carrier->add_msg_if_player( m_neutral, _( "You take a puff of your %s." ), tname().c_str() );
            if( has_flag( "TOBACCO" ) ) {
                carrier->add_effect( effect_cig, duration );
            } else {
                carrier->add_effect( effect_weed_high, duration / 2 );
            }
            carrier->moves -= 15;
        }

        if( ( carrier->has_effect( effect_shakes ) && one_in( 10 ) ) ||
            ( carrier->has_trait( trait_id( "JITTERY" ) ) && one_in( 200 ) ) ) {
            carrier->add_msg_if_player( m_bad, _( "Your shaking hand causes you to drop your %s." ),
                                        tname().c_str() );
            g->m.add_item_or_charges( tripoint( pos.x + rng( -1, 1 ), pos.y + rng( -1, 1 ), pos.z ), *this );
            return true; // removes the item that has just been added to the map
        }

        if( carrier->has_effect( effect_sleep ) ) {
            carrier->add_msg_if_player( m_bad, _( "You fall asleep and drop your %s." ),
                                        tname().c_str() );
            g->m.add_item_or_charges( tripoint( pos.x + rng( -1, 1 ), pos.y + rng( -1, 1 ), pos.z ), *this );
            return true; // removes the item that has just been added to the map
        }
    } else {
        // If not carried by someone, but laying on the ground:
        // release some smoke every five ticks
        if( item_counter % 5 == 0 ) {
            g->m.add_field( tripoint( pos.x + rng( -2, 2 ), pos.y + rng( -2, 2 ), pos.z ), smoke_type, 1 );
            // lit cigarette can start fires
            if( g->m.flammable_items_at( pos ) ||
                g->m.has_flag( "FLAMMABLE", pos ) ||
                g->m.has_flag( "FLAMMABLE_ASH", pos ) ) {
                g->m.add_field( pos, fd_fire, 1 );
            }
        }
    }

    // cig dies out
    if( item_counter == 0 ) {
        if( carrier != nullptr ) {
            carrier->add_msg_if_player( m_neutral, _( "You finish your %s." ), tname().c_str() );
        }
        if( typeId() == "cig_lit" ) {
            convert( "cig_butt" );
        } else if( typeId() == "cigar_lit" ) {
            convert( "cigar_butt" );
        } else { // joint
            convert( "joint_roach" );
            if( carrier != nullptr ) {
                carrier->add_effect( effect_weed_high, 1_minutes ); // one last puff
                g->m.add_field( tripoint( pos.x + rng( -1, 1 ), pos.y + rng( -1, 1 ), pos.z ), fd_weedsmoke, 2 );
                weed_msg( *carrier );
            }
        }
        active = false;
    }
    // Item remains
    return false;
}

bool item::process_extinguish( player *carrier, const tripoint &pos )
{
    // checks for water
    bool extinguish = false;
    bool in_inv = carrier != nullptr && carrier->has_item( *this );
    bool submerged = false;
    bool precipitation = false;
    bool windtoostrong = false;
    w_point weatherPoint = *g->weather_precise;
    int windpower = g->windspeed;
    switch( g->weather ) {
        case WEATHER_DRIZZLE:
        case WEATHER_FLURRIES:
            precipitation = one_in( 50 );
            break;
        case WEATHER_RAINY:
        case WEATHER_SNOW:
            precipitation = one_in( 25 );
            break;
        case WEATHER_THUNDER:
        case WEATHER_LIGHTNING:
        case WEATHER_SNOWSTORM:
            precipitation = one_in( 10 );
            break;
        default:
            break;
    }
    if( in_inv && g->m.has_flag( "DEEP_WATER", pos ) ) {
        extinguish = true;
        submerged = true;
    }
    if( ( !in_inv && g->m.has_flag( "LIQUID", pos ) ) ||
        ( precipitation && !g->is_sheltered( pos ) ) ) {
        extinguish = true;
    }
    if( in_inv && windpower > 5 && !g->is_sheltered( pos ) &&
        this->has_flag( "WIND_EXTINGUISH" ) ) {
        windtoostrong = true;
        extinguish = true;
    }
    if( !extinguish ||
        ( in_inv && precipitation && carrier->weapon.has_flag( "RAIN_PROTECT" ) ) ) {
        return false; //nothing happens
    }
    if( carrier != nullptr ) {
        if( submerged ) {
            carrier->add_msg_if_player( m_neutral, _( "Your %s is quenched by water." ), tname().c_str() );
        } else if( precipitation ) {
            carrier->add_msg_if_player( m_neutral, _( "Your %s is quenched by precipitation." ),
                                        tname().c_str() );
        } else if( windtoostrong ) {
            carrier->add_msg_if_player( m_neutral, _( "Your %s is blown out by the wind." ),
                                        tname().c_str() );
        }
    }

    // cig dies out
    if( has_flag( "LITCIG" ) ) {
        if( typeId() == "cig_lit" ) {
            convert( "cig_butt" );
        } else if( typeId() == "cigar_lit" ) {
            convert( "cigar_butt" );
        } else { // joint
            convert( "joint_roach" );
        }
    } else { // transform (lit) items
        if( type->tool->revert_to ) {
            convert( *type->tool->revert_to );
        } else {
            type->invoke( carrier != nullptr ? *carrier : g->u, *this, pos, "transform" );
        }

    }
    active = false;
    // Item remains
    return false;
}

cata::optional<tripoint> item::get_cable_target( player *p, const tripoint &pos ) const
{
    const std::string &state = get_var( "state" );
    if( state != "pay_out_cable" && state != "cable_charger_link" ) {
        return cata::nullopt;
    }
    const optional_vpart_position vp_pos = g->m.veh_at( pos );
    if( vp_pos ) {
        const cata::optional<vpart_reference> seat = vp_pos.part_with_feature( "BOARDABLE", true );
        if( seat && p == seat->vehicle().get_passenger( seat->part_index() ) ) {
            return pos;
        }
    }

    int source_x = get_var( "source_x", 0 );
    int source_y = get_var( "source_y", 0 );
    int source_z = get_var( "source_z", 0 );
    tripoint source( source_x, source_y, source_z );

    return g->m.getlocal( source );
}

bool item::process_cable( player *p, const tripoint &pos )
{
    const cata::optional<tripoint> source = get_cable_target( p, pos );
    if( !source ) {
        return false;
    }

    if( !g->m.veh_at( *source ) || ( source->z != g->get_levz() && !g->m.has_zlevels() ) ) {
        if( p != nullptr && p->has_item( *this ) ) {
            p->add_msg_if_player( m_bad, _( "You notice the cable has come loose!" ) );
        }
        reset_cable( p );
        return false;
    }

    int distance = rl_dist( pos, *source );
    int max_charges = type->maximum_charges();
    charges = max_charges - distance;

    if( charges < 1 ) {
        if( p != nullptr && p->has_item( *this ) ) {
            p->add_msg_if_player( m_bad, _( "The over-extended cable breaks loose!" ) );
        }
        reset_cable( p );
    }

    return false;
}

void item::reset_cable( player *p )
{
    int max_charges = type->maximum_charges();

    set_var( "state", "attach_first" );
    erase_var( "source_x" );
    erase_var( "source_y" );
    erase_var( "source_z" );
    active = false;
    charges = max_charges;

    if( p != nullptr ) {
        p->add_msg_if_player( m_info, _( "You reel in the cable." ) );
        p->moves -= charges * 10;
    }
}

bool item::process_wet( player * /*carrier*/, const tripoint & /*pos*/ )
{
    if( item_counter == 0 ) {
        if( is_tool() && type->tool->revert_to ) {
            convert( *type->tool->revert_to );
        }
        item_tags.erase( "WET" );
        active = false;
    }
    // Always return true so our caller will bail out instead of processing us as a tool.
    return true;
}

bool item::process_tool( player *carrier, const tripoint &pos )
{
    if( type->tool->turns_per_charge > 0 &&
        int( calendar::turn ) % type->tool->turns_per_charge == 0 ) {
        auto qty = std::max( ammo_required(), 1L );
        qty -= ammo_consume( qty, pos );

        // for items in player possession if insufficient charges within tool try UPS
        if( carrier && has_flag( "USE_UPS" ) ) {
            if( carrier->use_charges_if_avail( "UPS", qty ) ) {
                qty = 0;
            }
        }

        // if insufficient available charges shutdown the tool
        if( qty > 0 ) {
            if( carrier && has_flag( "USE_UPS" ) ) {
                carrier->add_msg_if_player( m_info, _( "You need an UPS to run the %s!" ), tname().c_str() );
            }

            // invoking the object can convert the item to another type
            const bool had_revert_to = type->tool->revert_to.has_value();
            type->invoke( carrier != nullptr ? *carrier : g->u, *this, pos );
            if( had_revert_to ) {
                deactivate( carrier );
                return false;
            } else {
                return true;
            }
        }
    }

    type->tick( carrier != nullptr ? *carrier : g->u, *this, pos );
    return false;
}

bool item::process( player *carrier, const tripoint &pos, bool activate )
{
    if( is_food() || is_food_container() ) {
        return process( carrier, pos, activate, g->get_temperature( pos ), 1 );
    } else {
        return process( carrier, pos, activate, 0, 1 );
    }
}

bool item::process( player *carrier, const tripoint &pos, bool activate, int temp,
                    float insulation )
{
    const bool preserves = type->container && type->container->preserves;
    for( auto it = contents.begin(); it != contents.end(); ) {
        if( preserves ) {
            // Simulate that the item has already "rotten" up to last_rot_check, but as item::rot
            // is not changed, the item is still fresh.
            it->last_rot_check = calendar::turn;
        }
        if( it->process( carrier, pos, activate, temp, type->insulation_factor * insulation ) ) {
            it = contents.erase( it );
        } else {
            ++it;
        }
    }

    if( activate ) {
        return type->invoke( carrier != nullptr ? *carrier : g->u, *this, pos );
    }
    // How this works: it checks what kind of processing has to be done
    // (e.g. for food, for drying towels, lit cigars), and if that matches,
    // call the processing function. If that function returns true, the item
    // has been destroyed by the processing, so no further processing has to be
    // done.
    // Otherwise processing continues. This allows items that are processed as
    // food and as litcig and as ...

    // Remaining stuff is only done for active items.
    if( !active ) {
        return false;
    }

    if( !is_food() && item_counter > 0 ) {
        item_counter--;
    }

    if( item_counter == 0 && type->countdown_action ) {
        type->countdown_action.call( carrier ? *carrier : g->u, *this, false, pos );
        if( type->countdown_destroy ) {
            return true;
        }
    }

    for( const auto &e : type->emits ) {
        g->m.emit_field( pos, e );
    }

    if( has_flag( "FAKE_SMOKE" ) && process_fake_smoke( carrier, pos ) ) {
        return true;
    }
    if( is_food() &&  process_food( carrier, pos, temp, insulation ) ) {
        return true;
    }
    if( is_corpse() && process_corpse( carrier, pos ) ) {
        return true;
    }
    if( has_flag( "WET" ) && process_wet( carrier, pos ) ) {
        // Drying items are never destroyed, but we want to exit so they don't get processed as tools.
        return false;
    }
    if( has_flag( "LITCIG" ) && process_litcig( carrier, pos ) ) {
        return true;
    }
    if( ( has_flag( "WATER_EXTINGUISH" ) || has_flag( "WIND_EXTINGUISH" ) ) &&
        process_extinguish( carrier, pos ) ) {
        return false;
    }
    if( has_flag( "CABLE_SPOOL" ) ) {
        // DO NOT process this as a tool! It really isn't!
        return process_cable( carrier, pos );
    }
    if( is_tool() ) {
        return process_tool( carrier, pos );
    }
    return false;
}

void item::mod_charges( long mod )
{
    if( has_infinite_charges() ) {
        return;
    }

    if( !count_by_charges() ) {
        debugmsg( "Tried to remove %s by charges, but item is not counted by charges.", tname().c_str() );
    } else if( mod < 0 && charges + mod < 0 ) {
        debugmsg( "Tried to remove charges that do not exist, removing maximum available charges instead." );
        charges = 0;
    } else if( mod > 0 && charges >= INFINITE_CHARGES - mod ) {
        charges = INFINITE_CHARGES - 1; // Highly unlikely, but finite charges should not become infinite.
    } else {
        charges += mod;
    }
}

bool item::has_effect_when_wielded( art_effect_passive effect ) const
{
    if( !type->artifact ) {
        return false;
    }
    auto &ew = type->artifact->effects_wielded;
    if( std::find( ew.begin(), ew.end(), effect ) != ew.end() ) {
        return true;
    }
    return false;
}

bool item::has_effect_when_worn( art_effect_passive effect ) const
{
    if( !type->artifact ) {
        return false;
    }
    auto &ew = type->artifact->effects_worn;
    if( std::find( ew.begin(), ew.end(), effect ) != ew.end() ) {
        return true;
    }
    return false;
}

bool item::has_effect_when_carried( art_effect_passive effect ) const
{
    if( !type->artifact ) {
        return false;
    }
    auto &ec = type->artifact->effects_carried;
    if( std::find( ec.begin(), ec.end(), effect ) != ec.end() ) {
        return true;
    }
    for( auto &i : contents ) {
        if( i.has_effect_when_carried( effect ) ) {
            return true;
        }
    }
    return false;
}

bool item::is_seed() const
{
    return type->seed.has_value();
}

time_duration item::get_plant_epoch() const
{
    if( !type->seed ) {
        return 0_turns;
    }
    // Growing times have been based around real world season length rather than
    // the default in-game season length to give
    // more accuracy for longer season lengths
    // Also note that seed->grow is the time it takes from seeding to harvest, this is
    // divided by 3 to get the time it takes from one plant state to the next.
    //@todo: move this into the islot_seed
    return type->seed->grow * calendar::season_ratio() / 3;
}

std::string item::get_plant_name() const
{
    if( !type->seed ) {
        return std::string{};
    }
    return type->seed->plant_name;
}

bool item::is_dangerous() const
{
    if( has_flag( "DANGEROUS" ) ) {
        return true;
    }

    // Note: Item should be dangerous regardless of what type of a container is it
    // Visitable interface would skip some options
    return std::any_of( contents.begin(), contents.end(), []( const item & it ) {
        return it.is_dangerous();
    } );
}

bool item::is_tainted() const
{
    return corpse && corpse->has_flag( MF_POISON );
}

bool item::is_soft() const
{
    const auto mats = made_of();
    return std::any_of( mats.begin(), mats.end(), []( const material_id & mid ) {
        return mid.obj().soft();
    } );
}

bool item::is_reloadable() const
{
    if( has_flag( "NO_RELOAD" ) && !has_flag( "VEHICLE" ) ) {
        return false; // turrets ignore NO_RELOAD flag

    } else if( is_bandolier() ) {
        return true;

    } else if( is_container() ) {
        return true;

    } else if( !is_gun() && !is_tool() && !is_magazine() ) {
        return false;

    } else if( !ammo_type() ) {
        return false;
    }

    return true;
}

std::string item::type_name( unsigned int quantity ) const
{
    const auto iter = item_vars.find( "name" );
    bool f_dressed = has_flag( "FIELD_DRESS" ) || has_flag( "FIELD_DRESS_FAILED" );
    bool quartered = has_flag( "QUARTERED" );
    bool skinned = has_flag( "SKINNED" );
    if( corpse != nullptr && typeId() == "corpse" ) {
        if( corpse_name.empty() ) {
            if( skinned && !f_dressed && !quartered ) {
                return string_format( npgettext( "item name", "skinned %s corpse", "skinned %s corpses", quantity ),
                                      corpse->nname().c_str() );
            } else if( skinned && f_dressed && !quartered ) {
                return string_format( npgettext( "item name", "skinned %s carcass", "skinned %s carcasses",
                                                 quantity ), corpse->nname().c_str() );
            } else if( f_dressed && !quartered ) {
                return string_format( npgettext( "item name", "%s carcass",
                                                 "%s carcasses", quantity ),
                                      corpse->nname().c_str() );
            } else if( f_dressed && quartered ) {
                return string_format( npgettext( "item name", "quartered %s carcass",
                                                 "quartered %s carcasses", quantity ),
                                      corpse->nname().c_str() );
            }
            return string_format( npgettext( "item name", "%s corpse",
                                             "%s corpses", quantity ),
                                  corpse->nname().c_str() );
        } else {
            if( skinned && !f_dressed && !quartered ) {
                return string_format( npgettext( "item name", "skinned %s corpse of %s", "skinned %s corpses of %s",
                                                 quantity ) );
            } else if( skinned && f_dressed && !quartered ) {
                return string_format( npgettext( "item name", "skinned %s carcass of %s",
                                                 "skinned %s carcasses of %s", quantity ) );
            } else            if( f_dressed && !quartered && !skinned ) {
                return string_format( npgettext( "item name", "%s carcass of %s",
                                                 "%s carcasses of %s", quantity ),
                                      corpse->nname().c_str(), corpse_name.c_str() );
            } else if( f_dressed && quartered && !skinned ) {
                return string_format( npgettext( "item name", "quartered %s carcass",
                                                 "quartered %s carcasses", quantity ),
                                      corpse->nname().c_str() );
            }
            return string_format( npgettext( "item name", "%s corpse of %s",
                                             "%s corpses of %s", quantity ),
                                  corpse->nname().c_str(), corpse_name.c_str() );
        }
    } else if( typeId() == "blood" ) {
        if( corpse == nullptr || corpse->id.is_null() ) {
            return string_format( npgettext( "item name", "human blood",
                                             "human blood", quantity ) );
        } else {
            return string_format( npgettext( "item name", "%s blood",
                                             "%s blood",  quantity ),
                                  corpse->nname().c_str() );
        }
    } else if( iter != item_vars.end() ) {
        return iter->second;
    } else {
        return type->nname( quantity );
    }
}

std::string item::nname( const itype_id &id, unsigned int quantity )
{
    const auto t = find_type( id );
    return t->nname( quantity );
}

bool item::count_by_charges( const itype_id &id )
{
    const auto t = find_type( id );
    return t->count_by_charges();
}

bool item::type_is_defined( const itype_id &id )
{
    return item_controller->has_template( id );
}

const itype *item::find_type( const itype_id &type )
{
    return item_controller->find_template( type );
}

int item::get_gun_ups_drain() const
{
    int draincount = 0;
    if( type->gun ) {
        draincount += type->gun->ups_charges;
        for( const auto mod : gunmods() ) {
            draincount += mod->type->gunmod->ups_charges;
        }
    }
    return draincount;
}

bool item::has_label() const
{
    return has_var( "item_label" );
}

std::string item::label( unsigned int quantity ) const
{
    if( has_label() ) {
        return get_var( "item_label" );
    }

    return type_name( quantity );
}

bool item::has_infinite_charges() const
{
    return charges == INFINITE_CHARGES;
}

skill_id item::contextualize_skill( const skill_id &id ) const
{
    if( id->is_contextual_skill() ) {
        static const skill_id weapon_skill( "weapon" );

        if( id == weapon_skill ) {
            if( is_gun() ) {
                return gun_skill();
            } else if( is_melee() ) {
                return melee_skill();
            }
        }
    }

    return id;
}

bool item::is_filthy() const
{
    return has_flag( "FILTHY" ) && ( get_option<bool>( "FILTHY_MORALE" ) ||
                                     g->u.has_trait( trait_id( "SQUEAMISH" ) ) );
}

bool item::on_drop( const tripoint &pos )
{
    return on_drop( pos, g->m );
}

bool item::on_drop( const tripoint &pos, map &m )
{
    // dropping liquids, even currently frozen ones, on the ground makes them
    // dirty
    if( made_of_from_type( LIQUID ) && !m.has_flag( "LIQUIDCONT", pos ) &&
        !item_tags.count( "DIRTY" ) ) {
        item_tags.insert( "DIRTY" );
    }
    return type->drop_action && type->drop_action.call( g->u, *this, false, pos );
}

time_duration item::age() const
{
    return calendar::turn - birthday();
}

void item::set_age( const time_duration &age )
{
    set_birthday( time_point( calendar::turn ) - age );
}

time_point item::birthday() const
{
    return bday;
}

void item::set_birthday( const time_point &bday )
{
    this->bday = bday;
}

bool item::is_upgrade() const
{
    if( !type->bionic ) {
        return false;
    }
    return type->bionic->is_upgrade;
}

int item::get_min_str() const
{
    if( type->gun ) {
        int min_str = type->min_str;
        for( const auto mod : gunmods() ) {
            min_str += mod->type->gunmod->min_str_required_mod;
        }
        return min_str > 0 ? min_str : 0;
    } else {
        return type->min_str;
    }
}

std::vector<item_comp> item::get_uncraft_components() const
{
    std::vector<item_comp> ret;
    if( components.empty() ) {
        //If item wasn't crafted with specific components use default recipe
        auto recipe = recipe_dictionary::get_uncraft(
                          typeId() ).disassembly_requirements().get_components();
        for( auto &component : recipe ) {
            ret.push_back( component.front() );
        }
    } else {
        //Make a new vector of components from the registered components
        for( auto &component : components ) {
            ret.push_back( item_comp( component.typeId(), component.count() ) );
        }
    }
    return ret;
}
