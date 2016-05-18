#include "item.h"

#include "advanced_inv.h"
#include "player.h"
#include "output.h"
#include "skill.h"
#include "bionics.h"
#include "game.h"
#include "map.h"
#include "debug.h"
#include "cursesdef.h"
#include "text_snippets.h"
#include "material.h"
#include "item_factory.h"
#include "item_group.h"
#include "options.h"
#include "uistate.h"
#include "messages.h"
#include "artifact.h"
#include "itype.h"
#include "iuse_actor.h"
#include "compatibility.h"
#include "translations.h"
#include "crafting.h"
#include "recipe_dictionary.h"
#include "martialarts.h"
#include "npc.h"
#include "ui.h"
#include "vehicle.h"
#include "mtype.h"
#include "field.h"
#include "weather.h"
#include "catacharset.h"
#include "cata_utility.h"
#include "input.h"
#include "fault.h"

#include <cmath> // floor
#include <sstream>
#include <algorithm>
#include <unordered_set>
#include <set>
#include <array>
#include <tuple>
#include <iterator>

static const std::string GUN_MODE_VAR_NAME( "item::mode" );

const skill_id skill_survival( "survival" );
const skill_id skill_melee( "melee" );
const skill_id skill_bashing( "bashing" );
const skill_id skill_cutting( "cutting" );
const skill_id skill_stabbing( "stabbing" );

const species_id FISH( "FISH" );
const species_id BIRD( "BIRD" );
const species_id INSECT( "INSECT" );
const species_id ROBOT( "ROBOT" );

const efftype_id effect_cig( "cig" );
const efftype_id effect_shakes( "shakes" );
const efftype_id effect_weed_high( "weed_high" );

enum item::LIQUID_FILL_ERROR : int {
    L_ERR_NONE, L_ERR_NO_MIX, L_ERR_NOT_CONTAINER, L_ERR_NOT_WATERTIGHT,
    L_ERR_NOT_SEALED, L_ERR_FULL
};

std::string const& rad_badge_color(int const rad)
{
    using pair_t = std::pair<int const, std::string const>;

    static std::array<pair_t, 6> const values = {{
        pair_t {  0, _("green") },
        pair_t { 30, _("blue")  },
        pair_t { 60, _("yellow")},
        pair_t {120, pgettext("color", "orange")},
        pair_t {240, _("red")   },
        pair_t {500, _("black") },
    }};

    for (auto const &i : values) {
        if (rad <= i.first) {
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

item::item()
{
    type = nullitem();
}

item::item( const itype *type, int turn, int qty ) : type( type )
{
    bday = turn >= 0 ? turn : int( calendar::turn );
    corpse = type->id == "corpse" ? &mtype_id::NULL_ID.obj() : nullptr;
    name = type_name();

    if( qty >= 0 ) {
        charges = qty;
    } else {
        if( type->spawn && type->spawn->rand_charges.size() > 1 ) {
            const auto charge_roll = rng( 1, type->spawn->rand_charges.size() - 1 );
            charges = rng( type->spawn->rand_charges[charge_roll - 1], type->spawn->rand_charges[charge_roll] );
        } else {
            charges = type->charges_default();
        }
    }

    if( type->gun ) {
        for( const auto &mod : type->gun->built_in_mods ){
            emplace_back( mod, turn, qty ).item_tags.insert( "IRREMOVABLE" );
        }
        for( const auto &mod : type->gun->default_mods ) {
            emplace_back( mod, turn, qty );
        }

    } else if( type->magazine ) {
        if( type->magazine->count > 0 ) {
            emplace_back( default_ammo( type->magazine->type ), calendar::turn, type->magazine->count );
        }

    } else if( type->comestible ) {
        active = goes_bad() && !rotten();

    } else if( type->tool ) {
        if( ammo_remaining() && ammo_type() != "NULL" ) {
            ammo_set( default_ammo( ammo_type() ), ammo_remaining() );
        }
    }

    if( ( type->gun || type->tool ) && !magazine_integral() ) {
        set_var( "magazine_converted", true );
    }

    if( type->variable_bigness ) {
        bigness = rng( type->variable_bigness->min_bigness, type->variable_bigness->max_bigness );
    }
    if( !type->snippet_category.empty() ) {
        note = SNIPPET.assign( type->snippet_category );
    }
}

item::item( const itype_id& id, int turn, int qty )
    : item( find_type( id ), turn, qty ) {}

item::item( const itype *type, int turn, default_charges_tag )
    : item( type, turn, type->charges_default() ) {}

item::item( const itype_id& id, int turn, default_charges_tag tag )
    : item( item::find_type( id ), turn, tag ) {}

item::item( const itype *type, int turn, solitary_tag )
    : item( type, turn, type->count_by_charges() ? 1 : -1 ) {}

item::item( const itype_id& id, int turn, solitary_tag tag )
    : item( item::find_type( id ), turn, tag ) {}

item item::make_corpse( const mtype_id& mt, int turn, const std::string &name )
{
    if( !mt.is_valid() ) {
        debugmsg( "tried to make a corpse with an invalid mtype id" );
    }

    item result( "corpse", turn >= 0 ? turn : int( calendar::turn ) );
    result.corpse = &mt.obj();

    result.active = result.corpse->has_flag( MF_REVIVES );
    if( result.active && one_in( 20 ) ) {
        result.item_tags.insert( "REVIVE_SPECIAL" );
    }

    // This is unconditional because the item constructor above sets result.name to
    // "human corpse".
    result.name = name;

    return result;
}

item::item(JsonObject &jo)
{
    deserialize(jo);
}

item& item::convert( const itype_id& new_type )
{
    type = find_type( new_type );
    return *this;
}

item& item::deactivate( const Character *ch, bool alert )
{
    if( !active ) {
        return *this; // no-op
    }

    if( is_tool() && type->tool->revert_to != "null" ) {
        if( ch && alert && !type->tool->revert_msg.empty() ) {
            ch->add_msg_if_player( m_info, _( type->tool->revert_msg.c_str() ), tname().c_str() );
        }
        convert( type->tool->revert_to );
        active = false;

    }
    return *this;
}

item& item::ammo_set( const itype_id& ammo, long qty )
{
    // if negative qty completely fill the item
    if( qty < 0 ) {
        if( magazine_integral() || magazine_current() ) {
            qty = ammo_capacity();
        } else {
            qty = item( magazine_default() ).ammo_capacity();
        }
    }

    if( qty == 0 ) {
        ammo_unset();
        return *this;
    }

    // handle reloadable tools and guns with no specific ammo type as special case
    if( ( is_tool() || is_gun() ) && magazine_integral() && ammo == "null" && ammo_type() == "NULL" ) {
        curammo = nullptr;
        charges = std::min( qty, ammo_capacity() );
        return *this;
    }

    // check ammo is valid for the item
    const itype *atype = item_controller->find_template( ammo );
    if( !atype->ammo || atype->ammo->type != ammo_type() ) {
        debugmsg( "Tried to set invalid ammo of %s for %s", atype->nname( qty ).c_str(), tname().c_str() );
        return *this;
    }

    if( is_magazine() ) {
        ammo_unset();
        emplace_back( ammo, calendar::turn, std::min( qty, ammo_capacity() ) );

    } else if( magazine_integral() ) {
        curammo = atype;
        charges = std::min( qty, ammo_capacity() );

    } else {
        if( !magazine_current() ) {
            emplace_back( magazine_default() );
        }
        magazine_current()->ammo_set( ammo, qty );
    }

    return *this;
}

item& item::ammo_unset()
{
    if( !is_tool() && !is_gun() && !is_magazine() ) {
        ; // do nothing

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
    static const std::string s_null("null"); // used alot, no need to repeat
    // Actually, type should never by null at all.
    return (type == nullptr || type == nullitem() || type->id == s_null);
}

bool item::covers( const body_part bp ) const
{
    if( bp >= num_bp ) {
        debugmsg( "bad body part %d to check in item::covers", static_cast<int>( bp ) );
        return false;
    }
    if( is_gun() ) {
        // Currently only used for guns with the should strap mod, other guns might
        // go on another bodypart.
        return bp == bp_torso;
    }
    return get_covered_body_parts().test(bp);
}

std::bitset<num_bp> item::get_covered_body_parts() const
{
    const auto armor = find_armor_data();
    if( armor == nullptr ) {
        return std::bitset<num_bp>();
    }
    auto res = armor->covers;

    switch (get_side()) {
        case LEFT:
            res.reset(bp_arm_r);
            res.reset(bp_hand_r);
            res.reset(bp_leg_r);
            res.reset(bp_foot_r);
            break;

        case RIGHT:
            res.reset(bp_arm_l);
            res.reset(bp_hand_l);
            res.reset(bp_leg_l);
            res.reset(bp_foot_l);
            break;
    }

    return res;
}

bool item::is_sided() const {
    auto t = find_armor_data();
    return t ? t->sided : false;
}

int item::get_side() const {
    return get_var("lateral", BOTH);
}

bool item::set_side (side s) {
    if (!is_sided()) return false;

    if (s == BOTH) {
        erase_var("lateral");
    } else {
        set_var("lateral", s);
    }

    return true;
}

item item::in_its_container()
{
    if( type->default_container != "null" ) {
        item ret( type->default_container, bday );
        if( made_of( LIQUID ) && ret.is_container() ) {
            // Note: we can't use any of the normal normal container functions as they check the
            // container being suitable (seals, watertight etc.)
            charges = liquid_charges( ret.type->container->contains );
        }
        ret.contents.push_back(*this);
        ret.invlet = invlet;
        return ret;
    } else {
        return *this;
    }
}

long item::liquid_charges( long units ) const
{
    if( is_ammo() ) {
        return type->ammo->def_charges * units;
    } else if( is_food() ) {
        return type->comestible->def_charges * units;
    } else {
        return units;
    }
}

long item::liquid_units( long charges ) const
{
    if( is_ammo() ) {
        return charges / type->ammo->def_charges;
    } else if( is_food() ) {
        return charges / type->comestible->def_charges;
    } else {
        return charges;
    }
}

bool item::stacks_with( const item &rhs ) const
{
    if( type != rhs.type ) {
        return false;
    }
    // This function is also used to test whether items counted by charges should be merged, for that
    // check the, the charges must be ignored. In all other cases (tools/guns), the charges are important.
    if( !count_by_charges() && charges != rhs.charges ) {
        return false;
    }
    if( damage != rhs.damage ) {
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
        if( bday != rhs.bday ) {
            return false;
        }
        // Because spoiling items are only processed every processing_speed()-th turn
        // the rotting value becomes slightly different for items that have
        // been created at the same time and place and with the same initial rot.
        if( std::abs( rot - rhs.rot ) > processing_speed() ) {
            return false;
        } else if( rotten() != rhs.rotten() ) {
            // just to be save that rotten and unrotten food is *never* stacked.
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
    if( is_var_veh_part() && bigness != rhs.bigness ) {
        return false;
    }
    if( contents.size() != rhs.contents.size() ) {
        return false;
    }
    for( size_t i = 0; i < contents.size(); i++ ) {
        if( contents[i].charges != rhs.contents[i].charges ) {
            // Don't stack *containers* with different sized contents.
            return false;
        }
        if( !contents[i].stacks_with( rhs.contents[i] ) ) {
            return false;
        }
    }
    return true;
}

bool item::merge_charges( const item &rhs )
{
    if( !count_by_charges() || !stacks_with( rhs ) ) {
        return false;
    }
    // We'll just hope that the item counter represents the same thing for both items
    if( item_counter > 0 || rhs.item_counter > 0 ) {
        item_counter = ( item_counter * charges + rhs.item_counter * rhs.charges ) / ( charges + rhs.charges );
    }
    charges += rhs.charges;
    return true;
}

void item::put_in(item payload)
{
    contents.push_back(payload);
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
    size_t pos = item_tag.find('=');
    if(item_tag.at(0) == ivaresc && pos != std::string::npos && pos >= 2 ) {
        std::string var_name, val_decoded;
        int svarlen, svarsep;
        svarsep = item_tag.find('=');
        svarlen = item_tag.size();
        val_decoded = "";
        var_name = item_tag.substr(1, svarsep - 1); // will assume sanity here for now
        for(int s = svarsep + 1; s < svarlen; s++ ) { // cheap and temporary, afaik stringstream IFS = [\r\n\t ];
            if(item_tag[s] == ivaresc && s < svarlen - 2 ) {
                if ( item_tag[s + 1] == '0' && item_tag[s + 2] == 'A' ) {
                    s += 2;
                    val_decoded.append(1, '\n');
                } else if ( item_tag[s + 1] == '0' && item_tag[s + 2] == 'D' ) {
                    s += 2;
                    val_decoded.append(1, '\r');
                } else if ( item_tag[s + 1] == '0' && item_tag[s + 2] == '6' ) {
                    s += 2;
                    val_decoded.append(1, '\t');
                } else if ( item_tag[s + 1] == '2' && item_tag[s + 2] == '0' ) {
                    s += 2;
                    val_decoded.append(1, ' ');
                } else {
                    val_decoded.append(1, item_tag[s]); // hhrrrmmmmm should be passing \a?
                }
            } else {
                val_decoded.append(1, item_tag[s]);
            }
        }
        item_vars[var_name]=val_decoded;
        return true;
    } else {
        return false;
    }
}

std::string item::info( bool showtext ) const
{
    std::vector<iteminfo> dummy;
    return info( showtext, dummy );
}

std::string item::info( bool showtext, std::vector<iteminfo> &info ) const
{
    std::stringstream temp1, temp2;
    std::string space = "  ";
    const bool debug = g != nullptr && ( debug_mode ||
                                         g->u.has_artifact_with( AEP_SUPER_CLAIRVOYANCE ) );

    info.clear();

    auto insert_separation_line = [&]() {
        if( info.back().sName != "--" ) {
            info.push_back( iteminfo( "DESCRIPTION", "--" ) );
        }
    };

    if( !is_null() ) {
        info.push_back( iteminfo( "BASE", _( "Category: " ), "<header>" + get_category().name + "</header>",
                                  -999, true, "", false ) );
        const int price_preapoc = price( false );
        const int price_postapoc = price( true );
        info.push_back( iteminfo( "BASE", space + _( "Price: " ), "<num>",
                                  ( double )price_preapoc / 100, false, "$", true, true ) );
        if( price_preapoc != price_postapoc ) {
            info.push_back( iteminfo( "BASE", space + _( "Barter value: " ), "<num>",
                            ( double )price_postapoc / 100, false, "$", true, true ) );
        }

        info.push_back( iteminfo( "BASE", _( "<bold>Volume</bold>: " ), "", volume(), true, "", false,
                                  true ) );

        info.push_back( iteminfo( "BASE", space + _( "Weight: " ),
                                  string_format( "<num> %s", weight_units() ),
                                  convert_weight( weight() ), false, "", true, true ) );

        if( count_by_charges() && type->volume > 0 && type->stack_size > 1 ) {
            if( type->volume == 1 ) {
                //~ %1$d is stack size and guaranteed to be > 1
                info.emplace_back( "BASE", string_format( _( "Stacks in groups of <stat>%1$d</stat>" ), type->stack_size ) );
            } else {
                //~ %1$d is stack size and %2$d is base volume with both guaranteed to be > 1
                info.emplace_back( "BASE", string_format( _( "Stack of <stat>%1$d</stat> consumes <stat>%2$d</stat> volume" ),
                                   type->stack_size, type->volume ) );
            }
        }

        if( !type->rigid ) {
            info.emplace_back( "BASE", _( "<bold>Rigid</bold>: " ), _( "No (contents increase volume)" ) );
        }

        if( damage_bash() > 0 || damage_cut() > 0 ) {
            info.push_back( iteminfo( "BASE", _( "Bash: " ), "", damage_bash(), true, "", false ) );
            if( has_flag( "SPEAR" ) ) {
                info.push_back( iteminfo( "BASE", space + _( "Pierce: " ), "", damage_cut(), true, "", false ) );
            } else if( has_flag( "STAB" ) ) {
                info.push_back( iteminfo( "BASE", space + _( "Stab: " ), "", damage_cut(), true, "", false ) );
            } else {
                info.push_back( iteminfo( "BASE", space + _( "Cut: " ), "", damage_cut(), true, "", false ) );
            }
            info.push_back( iteminfo( "BASE", space + _( "To-hit bonus: " ),
                                      ( ( type->m_to_hit > 0 ) ? "+" : "" ),
                                      type->m_to_hit, true, "" ) );
            info.push_back( iteminfo( "BASE", _( "Moves per attack: " ), "",
                                      attack_time(), true, "", true, true ) );
        }

        insert_separation_line();

        // Display any minimal stat or skill requirements for the item
        std::vector<std::string> req;
        if( type->min_str > 0 ) {
            req.push_back( string_format( "%s %d", _( "strength" ), type->min_str ) );
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
            req.push_back( string_format( "%s %d", sk.first.obj().name().c_str(), sk.second ) );
        }
        if( !req.empty() ) {
            std::ostringstream tmp;
            std::copy( req.begin(), req.end() - 1, std::ostream_iterator<std::string>( tmp, ", " ) );
            tmp << req.back();
            info.emplace_back( "BASE", _("<bold>Minimum requirements:</bold>") );
            info.emplace_back( "BASE", tmp.str() );
            insert_separation_line();
        }

        const std::vector<const material_type*> mat_types = made_of_types();
        if( !mat_types.empty() ) {
            std::string material_list;
            for( auto next_material : mat_types ) {
                if( !material_list.empty() ) {
                    material_list.append( ", " );
                }
                material_list.append( "<stat>" + next_material->name() + "</stat>" );
            }
            info.push_back( iteminfo( "BASE", string_format( _( "Material: %s" ), material_list.c_str() ) ) );
        }
        if( has_var( "contained_name" ) ) {
            info.push_back( iteminfo( "BASE", string_format( _( "Contains: %s" ),
                                      get_var( "contained_name" ).c_str() ) ) );
        }
        if( debug == true ) {
            if( g != NULL ) {
                info.push_back( iteminfo( "BASE", _( "age: " ), "",
                                          ( int( calendar::turn ) - bday ) / ( 10 * 60 ), true, "", true, true ) );

                const item *food = is_food_container() ? &contents[ 0 ] : this;
                if( food && food->goes_bad() ) {
                    info.push_back( iteminfo( "BASE", _( "bday rot: " ), "",
                                              ( int( calendar::turn ) - food->bday ), true, "", true, true ) );
                    info.push_back( iteminfo( "BASE", _( "temp rot: " ), "",
                                              ( int )food->rot, true, "", true, true ) );
                    info.push_back( iteminfo( "BASE", space + _( "max rot: " ), "",
                                              food->type->comestible->spoils, true, "", true, true ) );
                    info.push_back( iteminfo( "BASE", space + _( "fridge: " ), "",
                                              ( int )food->fridge, true, "", true, true ) );
                    info.push_back( iteminfo( "BASE", _( "last rot: " ), "",
                                              ( int )food->last_rot_check, true, "", true, true ) );
                }
            }
            info.push_back( iteminfo( "BASE", _( "burn: " ), "",  burnt, true, "", true, true ) );
        }
    }

    const item *food_item = nullptr;
    if( is_food() ) {
        food_item = this;
    } else if( is_food_container() ) {
        food_item = &contents.front();
    }
    if( food_item != nullptr ) {
        if( g->u.nutrition_for( food_item->type ) != 0 || food_item->type->comestible->quench != 0 ) {
            info.push_back( iteminfo( "FOOD", _( "<bold>Nutrition</bold>: " ), "", g->u.nutrition_for( food_item->type ),
                                      true, "", false, true ) );
            info.push_back( iteminfo( "FOOD", space + _( "Quench: " ), "", food_item->type->comestible->quench ) );
        }

        if( food_item->type->comestible->fun ) {
            info.push_back( iteminfo( "FOOD", _( "Enjoyability: " ), "", food_item->type->comestible->fun ) );
        }

        info.push_back( iteminfo( "FOOD", _( "Portions: " ), "", abs( int( food_item->charges ) ) ) );
        if( food_item->corpse != NULL && ( debug == true || ( g != NULL &&
                                           ( g->u.has_bionic( "bio_scent_vision" ) || g->u.has_trait( "CARNIVORE" ) ||
                                             g->u.has_artifact_with( AEP_SUPER_CLAIRVOYANCE ) ) ) ) ) {
            info.push_back( iteminfo( "FOOD", _( "Smells like: " ) + food_item->corpse->nname() ) );
        }

        std::string vits;
        for( const auto &v : g->u.vitamins_from( *food_item ) ) {
            // only display vitamins that we actually require
            if( g->u.vitamin_rate( v.first ) > 0 && v.second != 0 ) {
                if( !vits.empty() ) {
                    vits += ", ";
                }
                vits += string_format( "%s (%i%%)", v.first.obj().name().c_str(), int( v.second / ( DAYS( 1 ) / float( g->u.vitamin_rate( v.first ) ) ) * 100 ) );
            }
        }
        if( !vits.empty() ) {
            info.emplace_back( "FOOD", _( "Vitamins (RDA): " ), vits.c_str() );
        }
    }

    if( is_magazine() && !has_flag( "NO_RELOAD" ) ) {
        info.emplace_back( "MAGAZINE", _( "Capacity: " ),
                           string_format( ngettext( "<num> round of %s", "<num> rounds of %s", ammo_capacity() ),
                                          ammo_name( ammo_type() ).c_str() ), ammo_capacity(), true );

        info.emplace_back( "MAGAZINE", _( "Reload time: " ), _( "<num> per round" ),
                           type->magazine->reload_time, true, "", true, true );

        insert_separation_line();
    }

    if( !is_gun() ) {
        if( ammo_data() ) {
            if( ammo_remaining() > 0 ) {
                info.emplace_back( "AMMO", _( "Ammunition: " ), ammo_data()->nname( ammo_remaining() ) );
            } else if( is_ammo() ) {
                info.emplace_back( "AMMO", _( "Type: " ), ammo_name( ammo_type() ) );
            }

            const auto& ammo = *ammo_data()->ammo;
            if( ammo.damage > 0 ) {
                info.emplace_back( "AMMO", _( "<bold>Damage</bold>: " ), "", ammo.damage, true, "", false, false );
                info.emplace_back( "AMMO", space + _( "Armor-pierce: " ), "", ammo.pierce, true, "", true, false );
                info.emplace_back( "AMMO", _( "Range: " ), "", ammo.range, true, "", false, false );
                info.emplace_back( "AMMO", space + _( "Dispersion: " ), "", ammo.dispersion, true, "", true, true );
                info.emplace_back( "AMMO", _( "Recoil: " ), "", ammo.recoil, true, "", true, true );
             }

            std::vector<std::string> fx;
            if( ammo.ammo_effects.count( "RECYCLED" ) ) {
                fx.emplace_back( _( "This ammo has been <bad>hand-loaded</bad>" ) );
            }
            if( ammo.ammo_effects.count( "NEVER_MISFIRES" ) ) {
                fx.emplace_back( _( "This ammo <good>never misfires</good>" ) );
            }
            if( ammo.ammo_effects.count( "INCENDIARY" ) ) {
                fx.emplace_back( _( "This ammo <neutral>starts fires</neutral>" ) );
            }
            if( !fx.empty() ) {
                insert_separation_line();
                for( const auto& e : fx ) {
                    info.emplace_back( "AMMO", e );
                }
            }
        }

    } else {
        auto mod = gunmod_current();
        if( mod == nullptr ) {
            mod = this;
        } else {
            info.push_back( iteminfo( "DESCRIPTION",
                                      string_format( _( "Stats of the active <info>gunmod (%s)</info> are shown." ),
                                              mod->tname().c_str() ) ) );
        }
        islot_gun *gun = mod->type->gun.get();
        const auto curammo = mod->ammo_data();

        bool has_ammo = curammo && mod->ammo_remaining();

        int ammo_dam        = has_ammo ? curammo->ammo->damage     : 0;
        int ammo_range      = has_ammo ? curammo->ammo->range      : 0;
        int ammo_recoil     = has_ammo ? curammo->ammo->recoil     : 0;
        int ammo_pierce     = has_ammo ? curammo->ammo->pierce     : 0;
        int ammo_dispersion = has_ammo ? curammo->ammo->dispersion : 0;

        const auto skill = &mod->gun_skill().obj();

        info.push_back( iteminfo( "GUN", _( "Skill used: " ), "<info>" + skill->name() + "</info>" ) );

        if( mod->magazine_integral() ) {
            if( mod->ammo_capacity() ) {
                info.emplace_back( "GUN", _( "<bold>Capacity:</bold> " ),
                                   string_format( ngettext( "<num> round of %s", "<num> rounds of %s", mod->ammo_capacity() ),
                                                  ammo_name( mod->ammo_type() ).c_str() ), mod->ammo_capacity(), true );
            }
        } else {
            info.emplace_back( "GUN", _( "Type: " ), ammo_name( mod->ammo_type() ) );
            if( mod->magazine_current() ) {
                info.emplace_back( "GUN", _( "Magazine: " ), string_format( "<stat>%s</stat>", mod->magazine_current()->tname().c_str() ) );
            }
        }

        if( mod->ammo_data() ) {
            info.emplace_back( "AMMO", _( "Ammunition: " ), string_format( "<stat>%s</stat>", mod->ammo_data()->nname( mod->ammo_remaining() ).c_str() ) );
        }

        if( mod->get_gun_ups_drain() ) {
            info.emplace_back( "AMMO", string_format( ngettext( "Uses <stat>%i</stat> charge of UPS per shot",
                                                                "Uses <stat>%i</stat> charges of UPS per shot", mod->get_gun_ups_drain() ),
                                                      mod->get_gun_ups_drain() ) );
        }

        insert_separation_line();

        info.push_back( iteminfo( "GUN", _( "Damage: " ), "", mod->gun_damage( false ), true, "", false, false ) );

        if( has_ammo ) {
            temp1.str( "" );
            temp1 << ( ammo_dam >= 0 ? "+" : "" );
            // ammo_damage and sum_of_damage don't need to translate.
            info.push_back( iteminfo( "GUN", "ammo_damage", "",
                                      ammo_dam, true, temp1.str(), false, false, false ) );
            info.push_back( iteminfo( "GUN", "sum_of_damage", _( " = <num>" ),
                                      mod->gun_damage( true ), true, "", false, false, false ) );
        }

        info.push_back( iteminfo( "GUN", space + _( "Armor-pierce: " ), "",
                                  mod->gun_pierce( false ), true, "", !has_ammo, false ) );
        if( has_ammo ) {
            temp1.str( "" );
            temp1 << ( ammo_pierce >= 0 ? "+" : "" );
            // ammo_armor_pierce and sum_of_armor_pierce don't need to translate.
            info.push_back( iteminfo( "GUN", "ammo_armor_pierce", "",
                                      ammo_pierce, true, temp1.str(), false, false, false ) );
            info.push_back( iteminfo( "GUN", "sum_of_armor_pierce", _( " = <num>" ),
                                      mod->gun_pierce( true ), true, "", true, false, false ) );
        }

        info.push_back( iteminfo( "GUN", _( "Range: " ), "", mod->gun_range( false ), true, "", false,
                                  false ) );
        if( has_ammo ) {
            temp1.str( "" );
            temp1 << ( ammo_range >= 0 ? "+" : "" );
            // ammo_range and sum_of_range don't need to translate.
            info.push_back( iteminfo( "GUN", "ammo_range", "",
                                      ammo_range, true, temp1.str(), false, false, false ) );
            info.push_back( iteminfo( "GUN", "sum_of_range", _( " = <num>" ),
                                      mod->gun_range( true ), true, "", false, false, false ) );
        }

        info.push_back( iteminfo( "GUN", space + _( "Dispersion: " ), "",
                                  mod->gun_dispersion( false ), true, "", !has_ammo, true ) );
        if( has_ammo ) {
            temp1.str( "" );
            temp1 << ( ammo_range >= 0 ? "+" : "" );
            // ammo_dispersion and sum_of_dispersion don't need to translate.
            info.push_back( iteminfo( "GUN", "ammo_dispersion", "",
                                      ammo_dispersion, true, temp1.str(), false, true, false ) );
            info.push_back( iteminfo( "GUN", "sum_of_dispersion", _( " = <num>" ),
                                      mod->gun_dispersion( true ), true, "", true, true, false ) );
        }

        info.push_back( iteminfo( "GUN", _( "Sight dispersion: " ), "",
                                  mod->sight_dispersion( -1 ), true, "", false, true ) );

        info.push_back( iteminfo( "GUN", space + _( "Aim speed: " ), "",
                                  mod->aim_speed( -1 ), true, "", true, true ) );

        info.push_back( iteminfo( "GUN", _( "Recoil: " ), "", mod->gun_recoil( false ), true, "", false,
                                  true ) );
        if( has_ammo ) {
            temp1.str( "" );
            temp1 << ( ammo_recoil >= 0 ? "+" : "" );
            // ammo_recoil and sum_of_recoil don't need to translate.
            info.push_back( iteminfo( "GUN", "ammo_recoil", "",
                                      ammo_recoil, true, temp1.str(), false, true, false ) );
            info.push_back( iteminfo( "GUN", "sum_of_recoil", _( " = <num>" ),
                                      mod->gun_recoil( true ), true, "", false, true, false ) );
        }

        info.push_back( iteminfo( "GUN", space + _( "Reload time: " ),
                                  ( ( has_flag( "RELOAD_ONE" ) ) ? _( "<num> per round" ) : "" ),
                                  gun->reload_time, true, "", true, true ) );

        if( mod->burst_size() == 1 ) {
            if( skill->ident() == skill_id( "pistol" ) && has_flag( "RELOAD_ONE" ) ) {
                info.push_back( iteminfo( "GUN", _( "Fire mode: <info>Revolver</info>." ) ) );
            } else {
                info.push_back( iteminfo( "GUN", _( "Fire mode: <info>Semi-automatic</info>." ) ) );
            }
        } else {
            if( has_flag( "BURST_ONLY" ) ) {
                info.push_back( iteminfo( "GUN", _( "Fire mode: <info>Fully-automatic</info> (burst only)." ) ) );
            }
            info.push_back( iteminfo( "GUN", _( "Burst size: " ), "", mod->burst_size() ) );
        }

        if( !magazine_integral() ) {
            insert_separation_line();
            std::string mags = _( "<bold>Compatible magazines:</bold> " );
            const auto compat = magazine_compatible();
            for( auto iter = compat.cbegin(); iter != compat.cend(); ++iter ) {
                if( iter != compat.cbegin() ) {
                    mags += ", ";
                }
                mags += item_controller->find_template( *iter )->nname( 1 );
            }
            info.emplace_back( "DESCRIPTION", mags );
        }

        if( !gun->valid_mod_locations.empty() ) {
            insert_separation_line();

            temp1.str( "" );
            temp1 << _( "<bold>Mods:<bold> " );
            int iternum = 0;
            for( auto &elem : gun->valid_mod_locations ) {
                if( iternum != 0 ) {
                    temp1 << "; ";
                }
                const int free_slots = ( elem ).second - get_free_mod_locations( ( elem ).first );
                temp1 << "<bold>" << free_slots << "/" << ( elem ).second << "</bold> " << _( (
                            elem ).first.c_str() );
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
    }
    if( is_gunmod() ) {
        const auto mod = type->gunmod.get();

        if( is_auxiliary_gunmod() ) {
            info.push_back( iteminfo( "DESCRIPTION",
                                      _( "This mod <info>must be attached to a gun</info>, it can not be fired separately." ) ) );
        }
        if( has_flag( "REACH_ATTACK" ) ) {
            info.push_back( iteminfo( "DESCRIPTION",
                                      _( "When attached to a gun, <good>allows</good> making <info>reach melee attacks</info> with it." ) ) );
        }
        if( mod->dispersion != 0 ) {
            info.push_back( iteminfo( "GUNMOD", _( "Dispersion modifier: " ), "",
                                      mod->dispersion, true, ( ( mod->dispersion > 0 ) ? "+" : "" ), true, true ) );
        }
        if( mod->sight_dispersion != -1 ) {
            info.push_back( iteminfo( "GUNMOD", _( "Sight dispersion: " ), "",
                                      mod->sight_dispersion, true, "", true, true ) );
        }
        if( mod->aim_speed != -1 ) {
            info.push_back( iteminfo( "GUNMOD", _( "Aim speed: " ), "",
                                      mod->aim_speed, true, "", true, true ) );
        }
        if( mod->damage != 0 ) {
            info.push_back( iteminfo( "GUNMOD", _( "Damage: " ), "", mod->damage, true,
                                      ( ( mod->damage > 0 ) ? "+" : "" ) ) );
        }
        if( mod->pierce != 0 ) {
            info.push_back( iteminfo( "GUNMOD", _( "Armor-pierce: " ), "", mod->pierce, true,
                                      ( ( mod->pierce > 0 ) ? "+" : "" ) ) );
        }
        if( mod->recoil != 0 )
            info.push_back( iteminfo( "GUNMOD", _( "Recoil: " ), "", mod->recoil, true,
                                      ( ( mod->recoil > 0 ) ? "+" : "" ), true, true ) );
        if( mod->burst != 0 )
            info.push_back( iteminfo( "GUNMOD", _( "Burst: " ), "", mod->burst, true,
                                      ( mod->burst > 0 ? "+" : "" ) ) );

        if( mod->ammo_modifier != "NULL" ) {
            info.push_back( iteminfo( "GUNMOD",
                                      string_format( _( "Ammo: <stat>%s</stat>" ), ammo_name( mod->ammo_modifier ).c_str() ) ) );
        }

        temp1.str( "" );
        temp1 << _( "Used on: " );
        for( auto it = mod->usable.begin(); it != mod->usable.end(); ) {
            //~ a weapon type which a gunmod is compatible (eg. "pistol", "crossbow", "rifle")
            temp1 << string_format( "<info>%s</info>", _( it->c_str() ) );
            if( ++it != mod->usable.end() ) {
                temp1 << ", ";
            }
        }

        temp2.str( "" );
        temp2 << _( "Location: " );
        temp2 << _( mod->location.c_str() );

        info.push_back( iteminfo( "GUNMOD", temp1.str() ) );
        info.push_back( iteminfo( "GUNMOD", temp2.str() ) );

    }
    if( is_armor() ) {
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

        info.push_back( iteminfo( "ARMOR", temp1.str() ) );

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

        info.push_back( iteminfo( "ARMOR", _( "Coverage: " ), "<num>%", get_coverage(), true, "", false ) );
        info.push_back( iteminfo( "ARMOR", space + _( "Warmth: " ), "", get_warmth() ) );

        insert_separation_line();

        if( has_flag( "FIT" ) ) {
            info.push_back( iteminfo( "ARMOR", _( "<bold>Encumbrance</bold>: " ),
                                      _( "<num> <info>(fits)</info>" ),
                                      get_encumber(), true, "", false, true ) );
        } else {
            info.push_back( iteminfo( "ARMOR", _( "<bold>Encumbrance</bold>: " ), "",
                                      get_encumber(), true, "", false, true ) );
        }

        info.push_back( iteminfo( "ARMOR", space + _( "Storage: " ), "", get_storage() ) );

        info.push_back( iteminfo( "ARMOR", _( "Protection: Bash: " ), "", bash_resist(), true, "",
                                  false ) );
        info.push_back( iteminfo( "ARMOR", space + _( "Cut: " ), "", cut_resist(), true, "", false ) );
        info.push_back( iteminfo( "ARMOR", space + _( "Acid: " ), "", acid_resist(), true, "", true ) );
        info.push_back( iteminfo( "ARMOR", space + _( "Fire: " ), "", fire_resist(), true, "", true ) );
        info.push_back( iteminfo( "ARMOR", _( "Environmental protection: " ), "", get_env_resist() ) );

    }
    if( is_book() ) {

        insert_separation_line();
        auto book = type->book.get();
        // Some things about a book you CAN tell by it's cover.
        if( !book->skill ) {
            info.push_back( iteminfo( "BOOK", _( "Just for fun." ) ) );
        }
        if( book->req == 0 ) {
            info.push_back( iteminfo( "BOOK", _( "It can be <info>understood by beginners</info>." ) ) );
        }
        if( g->u.has_identified( type->id ) ) {
            if( book->skill ) {
                if( g->u.get_skill_level( book->skill ).can_train() ) {
                    info.push_back( iteminfo( "BOOK", "",
                                              string_format( _( "Can bring your <info>%s skill to</info> <num>" ),
                                                      book->skill.obj().name().c_str() ), book->level ) );
                }

                if( book->req != 0 ) {
                    info.push_back( iteminfo( "BOOK", "",
                                              string_format( _( "<info>Requires %s level</info> <num> to understand." ),
                                                      book->skill.obj().name().c_str() ),
                                              book->req, true, "", true, true ) );
                }
            }

            info.push_back( iteminfo( "BOOK", "",
                                      _( "Requires <info>intelligence of</info> <num> to easily read." ),
                                      book->intel, true, "", true, true ) );
            if( book->fun != 0 ) {
                info.push_back( iteminfo( "BOOK", "",
                                          _( "Reading this book affects your morale by <num>" ),
                                          book->fun, true, ( book->fun > 0 ? "+" : "" ) ) );
            }
            info.push_back( iteminfo( "BOOK", "",
                                      ngettext( "A chapter of this book takes <num> <info>minute to read</info>.",
                                                "A chapter of this book takes <num> <info>minutes to read</info>.",
                                                book->time ),
                                      book->time, true, "", true, true ) );
            if( book->chapters > 0 ) {
                const int unread = get_remaining_chapters( g->u );
                info.push_back( iteminfo( "BOOK", "", ngettext( "This book has <num> <info>unread chapter</info>.",
                                          "This book has <num> <info>unread chapters</info>.",
                                          unread ),
                                          unread ) );
            }

            std::vector<std::string> recipe_list;
            for( auto const &elem : book->recipes ) {
                const bool knows_it = g->u.knows_recipe( elem.recipe );
                // If the player knows it, they recognize it even if it's not clearly stated.
                if( elem.is_hidden() && !knows_it ) {
                    continue;
                }
                if( knows_it ) {
                    // In case the recipe is known, but has a different name in the book, use the
                    // real name to avoid confusing the player.
                    const std::string name = item::nname( elem.recipe->result );
                    recipe_list.push_back( "<bold>" + name + "</bold>" );
                } else {
                    recipe_list.push_back( "<dark>" + elem.name + "</dark>" );
                }
            }
            if( !recipe_list.empty() ) {
                std::string recipes = "";
                size_t index = 1;
                for( auto iter = recipe_list.begin();
                     iter != recipe_list.end(); ++iter, ++index ) {
                    recipes += *iter;
                    if( index == recipe_list.size() - 1 ) {
                        recipes += _( " and " ); // Who gives a fuck about an oxford comma?
                    } else if( index != recipe_list.size() ) {
                        recipes += _( ", " );
                    }
                }
                std::string recipe_line = string_format(
                                              ngettext( "This book contains %1$d crafting recipe: %2$s",
                                                        "This book contains %1$d crafting recipes: %2$s", recipe_list.size() ),
                                              recipe_list.size(), recipes.c_str() );

                insert_separation_line();
                info.push_back( iteminfo( "DESCRIPTION", recipe_line ) );
            }
            if( recipe_list.size() != book->recipes.size() ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "It might help you figuring out some <good>more recipes</good>." ) ) );
            }
        } else {
            info.push_back( iteminfo( "BOOK",
                                      _( "You need to <info>read this book to see its contents</info>." ) ) );
        }

    }
    if( is_container() ) {
        const auto &c = *type->container;

        info.push_back( iteminfo( "ARMOR", temp1.str() ) );

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

        temp1 << string_format( _( "can store <info>%.2f liters</info>." ), c.contains / 4.0 );

        info.push_back( iteminfo( "CONTAINER", temp1.str() ) );
    }

    if( is_tool() ) {
        if( ammo_capacity() != 0 ) {
            info.emplace_back( "TOOL", string_format( _( "<bold>Charges</bold>: %d" ), ammo_remaining() ) );
        }

        if( !magazine_integral() ) {
            insert_separation_line();
            std::string tmp = _( "<bold>Compatible magazines:</bold> " );
            const auto compat = magazine_compatible();
            for( auto iter = compat.cbegin(); iter != compat.cend(); ++iter ) {
                if( iter != compat.cbegin() ) {
                    tmp += ", ";
                }
                tmp += item_controller->find_template( *iter )->nname( 1 );
            }
            info.emplace_back( "TOOL", tmp );

        } else if( ammo_capacity() != 0 ) {
            std::string tmp;
            if( ammo_type() != "NULL" ) {
                //~ "%s" is ammunition type. This types can't be plural.
                tmp = ngettext( "Maximum <num> charge of %s.", "Maximum <num> charges of %s.", ammo_capacity() );
                tmp = string_format( tmp, ammo_name( ammo_type() ).c_str() );
            } else {
                tmp = ngettext( "Maximum <num> charge.", "Maximum <num> charges.", ammo_capacity() );
            }
            info.emplace_back( "TOOL", "", tmp, ammo_capacity() );
        }
    }

    if( !components.empty() ) {
        info.push_back( iteminfo( "DESCRIPTION", string_format( _( "Made from: %s" ),
                                  components_to_string().c_str() ) ) );
    } else {
        const recipe *dis_recipe = get_disassemble_recipe( type->id );
        if( dis_recipe != nullptr ) {
            std::ostringstream buffer;
            bool first_component = true;
            for( const auto &it : dis_recipe->requirements.get_components() ) {
                if( first_component ) {
                    first_component = false;
                } else {
                    buffer << _( ", " );
                }
                buffer << it.front().to_string();
            }
            insert_separation_line();
            info.push_back( iteminfo( "DESCRIPTION", _( "Disassembling this item might yield:" ) ) );
            info.push_back( iteminfo( "DESCRIPTION", buffer.str().c_str() ) );
        }
    }

    for( const auto &quality : type->qualities ) {
        const auto desc = string_format( _( "Has level <info>%1$d %2$s</info> quality." ),
                                         quality.second,
                                         quality.first.obj().name.c_str() );
        info.push_back( iteminfo( "QUALITIES", "", desc ) );
    }
    bool intro = false; // Did we print the "Contains items with qualities" line
    for( const auto &content : contents ) {
        for( const auto quality : content.type->qualities ) {
            if( !intro ) {
                intro = true;
                info.push_back( iteminfo( "QUALITIES", "", _( "Contains items with qualities:" ) ) );
            }

            const auto desc = string_format( space + _( "Level %1$d %2$s quality." ),
                                             quality.second,
                                             quality.first.obj().name.c_str() );
            info.push_back( iteminfo( "QUALITIES", "", desc ) );
        }
    }

    if( showtext && !is_null() ) {
        const std::map<std::string, std::string>::const_iterator idescription =
            item_vars.find( "description" );
        insert_separation_line();
        if( !type->snippet_category.empty() ) {
            // Just use the dynamic description
            info.push_back( iteminfo( "DESCRIPTION", SNIPPET.get( note ) ) );
        } else if( idescription != item_vars.end() ) {
            info.push_back( iteminfo( "DESCRIPTION", idescription->second ) );
        } else {
            info.push_back( iteminfo( "DESCRIPTION", type->description ) );
        }
        std::ostringstream tec_buffer;
        for( const auto &elem : type->techniques ) {
            const ma_technique &tec = elem.obj();
            if( tec.name.empty() ) {
                continue;
            }
            if( !tec_buffer.str().empty() ) {
                tec_buffer << _( ", " );
            }
            tec_buffer << "<stat>" << tec.name << "</stat>";
        }
        for( const auto &elem : techniques ) {
            const ma_technique &tec = elem.obj();
            if( tec.name.empty() ) {
                continue;
            }
            if( !tec_buffer.str().empty() ) {
                tec_buffer << _( ", " );
            }
            tec_buffer << "<stat>" << tec.name << "</stat>";
        }
        if( !tec_buffer.str().empty() ) {
            insert_separation_line();
            info.push_back( iteminfo( "DESCRIPTION", std::string( _( "Techniques: " ) ) + tec_buffer.str() ) );
        }

        if( !is_gunmod() && has_flag( "REACH_ATTACK" ) ) {
            insert_separation_line();
            if( has_flag( "REACH3" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "This item can be used to make <info>long reach attacks</info>." ) ) );
            } else {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "This item can be used to make <info>reach attacks</info>." ) ) );
            }
        }

        //lets display which martial arts styles character can use with this weapon
        if( g->u.ma_styles.size() > 0 ) {
            std::vector<matype_id> valid_styles;
            std::ostringstream style_buffer;
            for( auto style : g->u.ma_styles ) {
                if( style.obj().has_weapon( type->id ) ) {
                    if( !style_buffer.str().empty() ) {
                        style_buffer << _( ", " );
                    }
                    style_buffer << style.obj().name;
                }
            }
            if( !style_buffer.str().empty() ) {
                insert_separation_line();
                info.push_back( iteminfo( "DESCRIPTION",
                                          std::string( _( "You know how to use this with these martial arts styles: " ) ) +
                                          style_buffer.str() ) );
            }
        }

        for( const auto &method : type->use_methods ) {
            insert_separation_line();
            method.dump_info( *this, info );
        }

        insert_separation_line();

        if( is_armor() ) {
            //See shorten version of this in armor_layers.cpp::clothing_flags_description
            if( has_flag( "FIT" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This piece of clothing <info>fits</info> you perfectly." ) ) );
            } else if( has_flag( "VARSIZE" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This piece of clothing <info>can be refitted</info>." ) ) );
            }
            if( is_sided() ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This item can be worn on <info>either side</info> of the body." ) ) );
            }
            if( has_flag( "SKINTIGHT" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This piece of clothing <info>lies close</info> to the skin." ) ) );
            } else if( has_flag( "BELTED" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This gear is <info>strapped</info> onto you." ) ) );
            } else if( has_flag( "WAIST" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This gear is worn on or around your <info>waist</info>." ) ) );
            } else if( has_flag( "OUTER" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This gear is generally <info>worn over</info> clothing." ) ) );
            } else {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This gear is generally worn as clothing." ) ) );
            }
            if( has_flag( "OVERSIZE" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This piece of clothing is large enough to accommodate <info>mutated anatomy</info>." ) ) );
            }
            if( has_flag( "BLOCK_WHILE_WORN" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This piece of clothing can be used to block attacks when worn." ) ) );
            }
            if( has_flag( "ALLOWS_NATURAL_ATTACKS" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This piece of clothing won't hinder special attacks that involve <info>mutated anatomy</info>." ) ) );
            }
            if( has_flag( "POCKETS" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This piece of clothing has <info>pockets</info> to warm your hands.  Put away your weapon to warm your hands in the pockets." ) ) );
            }
            if( has_flag( "HOOD" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This piece of clothing has a <info>hood</info> to keep your head warm.  Leave your head unencumbered to put on the hood." ) ) );
            }
            if( has_flag( "COLLAR" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This piece of clothing has a <info>wide collar</info> that can keep your mouth warm.  Leave your mouth unencumbered to raise the collar." ) ) );
            }
            if( has_flag( "RAINPROOF" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This piece of clothing is designed to keep you <info>dry</info> in the rain." ) ) );
            }
            if( has_flag( "SUN_GLASSES" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This piece of clothing keeps the <info>glare</info> out of your eyes." ) ) );
            }
            if( has_flag( "WATER_FRIENDLY" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This piece of clothing <good>performs well</good> even when <info>soaking wet</info>. This can feel good." ) ) );
            }
            if( has_flag( "WATERPROOF" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This piece of clothing <info>won't let water through</info>.  Unless you jump in the river or something like that." ) ) );
            }
            if( has_flag( "STURDY" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This piece of clothing is designed to <good>protect</good> you from harm and withstand <info>a lot of abuse</info>." ) ) );
            }
            if( has_flag( "FRAGILE" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This gear is <bad>fragile</bad> and <info>won't protect you for long</info>." ) ) );
            }
            if( has_flag( "DEAF" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This gear <bad>prevents</bad> you from <info>hearing any sounds</info>." ) ) );
            }
            if( has_flag( "PARTIAL_DEAF" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This gear <good>reduces</good> the volume of <info>sounds</info> to a safe level." ) ) );
            }
            if( has_flag( "BLIND" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This gear <bad>prevents</bad> you from <info>seeing</info> anything." ) ) );
            }
            if( has_flag( "SWIM_GOGGLES" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This piece of clothing allows you to <good>see much further</good> <info>under water</info>." ) ) );
            }
            if( item_tags.count( "wooled" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This piece of clothing has a wool lining sewn into it to <good>increase</good> its overall <info>warmth</info>." ) ) );
            }
            if( item_tags.count( "furred" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This piece of clothing has a fur lining sewn into it to <good>increase</good> its overall <info>warmth</info>." ) ) );
            }
            if( item_tags.count( "leather_padded" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This gear has certain parts padded with leather to <good>increase protection</good> with moderate <bad>increase to encumbrance</bad>." ) ) );
            }
            if( item_tags.count( "kevlar_padded" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This gear has Kevlar inserted into strategic locations to <good>increase protection</good> with some <bad>increase to encumbrance</bad>." ) ) );
            }
            if( has_flag( "FLOTATION" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This piece of clothing <neutral>prevents</neutral> you from <info>going underwater</info> (including voluntary diving)." ) ) );
            }
            if( is_disgusting_for( g->u ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This piece of clothing is <bad>filthy</bad>." ) ) );
            }
            if( has_flag( "RAD_PROOF" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This piece of clothing <good>completely protects</good> you from <info>radiation</info>." ) ) );
            } else if( has_flag( "RAD_RESIST" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This piece of clothing <neutral>partially protects</neutral> you from <info>radiation</info>." ) ) );
            } else if( is_power_armor() ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This gear is a part of power armor." ) ) );
                if( covers( bp_head ) ) {
                    info.push_back( iteminfo( "DESCRIPTION",
                                              _( "* When worn with a power armor suit, it will <good>fully protect</good> you from <info>radiation</info>." ) ) );
                } else {
                    info.push_back( iteminfo( "DESCRIPTION",
                                              _( "* When worn with a power armor helmet, it will <good>fully protect</good> you from <info>radiation</info>." ) ) );
                }
            }
            if( has_flag( "ELECTRIC_IMMUNE" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This gear <good>completely protects</good> you from <info>electric discharges</info>." ) ) );
            }
            if( has_flag( "THERMOMETER" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This gear is equipped with an <info>accurate thermometer</info>." ) ) );
            }
            if( has_flag( "ALARMCLOCK" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This gear has an <info>alarm clock</info> feature." ) ) );
            }
            if( has_flag( "BOOTS" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* You can <info>store knives</info> in this gear." ) ) );
            }
            if( has_flag( "FANCY" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This piece of clothing is <info>fancy</info>." ) ) );
            } else if( has_flag( "SUPER_FANCY" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This piece of clothing is <info>very fancy</info>." ) ) );
            }
            if( type->id == "rad_badge" ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          string_format( _( "* The film strip on the badge is %s." ),
                                                  rad_badge_color( irridation ).c_str() ) ) );
            }
        }

        if( is_tool() ) {
            if( has_flag( "DOUBLE_AMMO" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This tool has <good>double</good> the normal <info>maximum charges</info>." ) ) );
            }
            if( has_flag( "ATOMIC_AMMO" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This tool has been modified to run off <info>plutonium cells</info> instead of batteries." ) ) );
            }
            if( has_flag( "USE_UPS" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This tool has been modified to use a <info>universal power supply</info> and is <neutral>not compatible</neutral> with <info>standard batteries</info>." ) ) );
            } else if( has_flag( "RECHARGE" ) && has_flag( "NO_RELOAD" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This tool has a <info>rechargeable power cell</info> and is <neutral>not compatible</neutral> with <info>standard batteries</info>." ) ) );
            } else if( has_flag( "RECHARGE" ) ) {
                info.push_back( iteminfo( "DESCRIPTION",
                    _( "* This tool has a <info>rechargeable power cell</info> and can be recharged in any <neutral>UPS-compatible recharging station</neutral>. You could charge it with <info>standard batteries</info>, but unloading it is impossible." ) ) );
            }
            if( has_flag( "RADIO_ACTIVATION" ) ) {
                if( has_flag( "RADIO_MOD" ) ) {
                    info.push_back( iteminfo( "DESCRIPTION",
                                              _( "* This item has been modified to listen to <info>radio signals</info>.  It can still be activated manually." ) ) );
                } else {
                    info.push_back( iteminfo( "DESCRIPTION",
                                              _( "* This item can only be activated by a <info>radio signal</info>." ) ) );
                }

                if( has_flag( "RADIOSIGNAL_1" ) ) {
                    info.push_back( iteminfo( "DESCRIPTION",
                                              _( "* It will be activated by <color_c_red>\"Red\"</color> radio signal." ) ) );
                } else if( has_flag( "RADIOSIGNAL_2" ) ) {
                    info.push_back( iteminfo( "DESCRIPTION",
                                              _( "* It will be activated by <color_c_blue>\"Blue\"</color> radio signal." ) ) );
                } else if( has_flag( "RADIOSIGNAL_3" ) ) {
                    info.push_back( iteminfo( "DESCRIPTION",
                                              _( "* It will be activated by <color_c_green>\"Green\"</color> radio signal." ) ) );
                } else {
                    info.push_back( iteminfo( "DESCRIPTION",
                                              _( "* It is <bad>bugged</bad> and does not actually listen to <info>radio signals</info>." ) ) );
                }

                if( has_flag( "RADIO_INVOKE_PROC" ) ) {
                    info.push_back( iteminfo( "DESCRIPTION",
                                              _( "* Activating this item with a <info>radio signal</info> will <neutral>detonate</neutral> it immediately." ) ) );
                }
            }
        }

        if( is_bionic() ) {
            info.push_back( iteminfo( "DESCRIPTION", list_occupied_bps( type->id,
                _( "This bionic is installed in the following body part(s):" ) ) ) );
        }

        if( is_gun() && has_flag( "FIRE_TWOHAND" ) ) {
            info.push_back( iteminfo( "DESCRIPTION",
                                      _( "* This weapon needs <info>two free hands</info> to fire." ) ) );
        }

        if( has_flag( "BELT_CLIP" ) ) {
            info.push_back( iteminfo( "DESCRIPTION",
                                      _( "* This item can be <neutral>clipped or hooked</neutral> on to a <info>belt loop</info> of the appropriate size." ) ) );
        }

        if( has_flag( "LEAK_DAM" ) && has_flag( "RADIOACTIVE" ) && damage > 0 ) {
            info.push_back( iteminfo( "DESCRIPTION",
                                      _( "* The casing of this item has <neutral>cracked</neutral>, revealing an <info>ominous green glow</info>." ) ) );
        }

        if( has_flag( "LEAK_ALWAYS" ) && has_flag( "RADIOACTIVE" ) ) {
            info.push_back( iteminfo( "DESCRIPTION",
                                      _( "* This object is <neutral>surrounded</neutral> by a <info>sickly green glow</info>." ) ) );
        }

        if( is_food() ) {
            if( has_flag( "CANNIBALISM" ) ) {
                if( !g->u.has_trait_flag( "CANNIBAL" ) ) {
                    info.emplace_back( "DESCRIPTION", _( "* This food contains <bad>human flesh</bad>." ) );
                } else {
                    info.emplace_back( "DESCRIPTION", _( "* This food contains <good>human flesh</good>." ) );
                }
            }

            if( is_tainted() ) {
                info.emplace_back( "DESCRIPTION", _( "* This food is <bad>tainted</bad> and will poison you." ) );
            }

            ///\EFFECT_SURVIVAL >=3 allows detection of poisonous food
            if( has_flag( "HIDDEN_POISON" ) && g->u.get_skill_level( skill_survival ).level() >= 3 ) {
                info.emplace_back( "DESCRIPTION", _( "* On closer inspection, this appears to be <bad>poisonous</bad>." ) );
            }

            ///\EFFECT_SURVIVAL >=5 allows detection of hallucinogenic food
            if( has_flag( "HIDDEN_HALLU" ) && g->u.get_skill_level( skill_survival ).level() >= 5 ) {
                info.emplace_back( "DESCRIPTION", _( "* On closer inspection, this appears to be <neutral>hallucinogenic</neutral>." ) );
            }
        }

        if( is_brewable() || ( !contents.empty() && contents[0].is_brewable() ) ) {
            const item &brewed = !is_brewable() ? contents[0] : *this;
            int btime = brewed.brewing_time();
            if( btime <= HOURS(48) )
                info.push_back( iteminfo( "DESCRIPTION",
                                          string_format( ngettext( "* Once set in a vat, this will ferment in around %d hour.",
                                                  "* Once set in a vat, this will ferment in around %d hours.", btime / HOURS(1) ),
                                                  btime / HOURS(1) ) ) );
            else {
                btime = 0.5 + btime / HOURS(48); //Round down to 12-hour intervals
                if( btime % 2 == 1 ) {
                    info.push_back( iteminfo( "DESCRIPTION",
                                              string_format( _( "* Once set in a vat, this will ferment in around %d and a half days." ),
                                                      btime / 2 ) ) );
                } else {
                    info.push_back( iteminfo( "DESCRIPTION",
                                              string_format( ngettext( "* Once set in a vat, this will ferment in around %d day.",
                                                      "* Once set in a vat, this will ferment in around %d days.", btime / 2 ),
                                                      btime / 2 ) ) );
                }
            }

            for( const auto &res : brewed.brewing_results() ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          string_format( _( "* Fermenting this will produce <neutral>%s</neutral>." ),
                                                         item::nname( res, brewed.charges ).c_str() ) ) );
            }
        }

        for( const auto &e : faults ) {
            //~ %1$s is the name of a fault and %2$s is the description of the fault
            info.emplace_back( "DESCRIPTION", string_format( _( "* <bad>Faulty %1$s</bad>.  %2$s" ),
                               e.obj().name().c_str(), e.obj().description().c_str() ) );
        }

        ///\EFFECT_MELEE >2 allows seeing melee damage stats on weapons
        if( debug_mode || ( g->u.get_skill_level( skill_melee ) > 2 && ( damage_bash() > 0 ||
                            damage_cut() > 0 || type->m_to_hit > 0 ) ) ) {
            damage_instance non_crit;
            g->u.roll_all_damage( false, non_crit, true, *this );
            damage_instance crit;
            g->u.roll_all_damage( true, crit, true, *this );
            int attack_cost = g->u.attack_speed( *this, true );
            insert_separation_line();
            info.push_back( iteminfo( "DESCRIPTION", string_format( _( "Average melee damage:" ) ) ) );
            info.push_back( iteminfo( "DESCRIPTION",
                                      string_format( _( "Critical hit chance %d%% - %d%%" ),
                                              int( g->u.crit_chance( 0, 100, *this ) * 100 ),
                                              int( g->u.crit_chance( 100, 0, *this ) * 100 ) ) ) );
            info.push_back( iteminfo( "DESCRIPTION",
                                      string_format( _( "%d bashing (%d on a critical hit)" ),
                                              int( non_crit.type_damage( DT_BASH ) ),
                                              int( crit.type_damage( DT_BASH ) ) ) ) );
            if( non_crit.type_damage( DT_CUT ) > 0.0f || crit.type_damage( DT_CUT ) > 0.0f ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          string_format( _( "%d cutting (%d on a critical hit)" ),
                                                  int( non_crit.type_damage( DT_CUT ) ),
                                                  int( crit.type_damage( DT_CUT ) ) ) ) );
            }
            if( non_crit.type_damage( DT_STAB ) > 0.0f || crit.type_damage( DT_STAB ) > 0.0f ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          string_format( _( "%d piercing (%d on a critical hit)" ),
                                                  int( non_crit.type_damage( DT_STAB ) ),
                                                  int( crit.type_damage( DT_STAB ) ) ) ) );
            }
            info.push_back( iteminfo( "DESCRIPTION",
                                      string_format( _( "%d moves per attack" ), attack_cost ) ) );
        }

        for( auto &u : type->use_methods ) {
            const auto tt = dynamic_cast<const delayed_transform_iuse *>( u.get_actor_ptr() );
            if( tt == nullptr ) {
                continue;
            }
            const int time_to_do = tt->time_to_do( *this );
            if( time_to_do <= 0 ) {
                info.push_back( iteminfo( "DESCRIPTION", _( "It's done and <info>can be activated</info>." ) ) );
            } else {
                const auto time = calendar( time_to_do ).textify_period();
                info.push_back( iteminfo( "DESCRIPTION", string_format( _( "It will be done in %s." ),
                                          time.c_str() ) ) );
            }
        }

        if( ( is_food() && goes_bad() ) || ( is_food_container() && contents[0].goes_bad() ) ) {
            if( rotten() || ( is_food_container() && contents[0].rotten() ) ) {
                if( g->u.has_bionic( "bio_digestion" ) ) {
                    info.push_back( iteminfo( "DESCRIPTION",
                                              _( "This food has started to <neutral>rot</neutral>, but <info>your bionic digestion can tolerate it</info>." ) ) );
                } else if( g->u.has_trait( "SAPROVORE" ) ) {
                    info.push_back( iteminfo( "DESCRIPTION",
                                              _( "This food has started to <neutral>rot</neutral>, but <info>you can tolerate it</info>." ) ) );
                } else {
                    info.push_back( iteminfo( "DESCRIPTION",
                                              _( "This food has started to <bad>rot</bad>.  <info>Eating</info> it would be a <bad>very bad idea</bad>." ) ) );
                }
            } else {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "This food is <neutral>perishable</neutral>, and will eventually rot." ) ) );
            }

        }
        std::map<std::string, std::string>::const_iterator item_note = item_vars.find( "item_note" );
        std::map<std::string, std::string>::const_iterator item_note_type =
            item_vars.find( "item_note_type" );

        if( item_note != item_vars.end() ) {
            insert_separation_line();
            std::string ntext = "";
            if( item_note_type != item_vars.end() ) {
                ntext += string_format( _( "%1$s on the %2$s is: " ),
                                        item_note_type->second.c_str(), tname().c_str() );
            } else {
                ntext += _( "Note: " );
            }
            info.push_back( iteminfo( "DESCRIPTION", ntext + item_note->second ) );
        }

        // describe contents
        if( !contents.empty() ) {
            if( is_gun() ) { //Mods description
                for( const auto mod : gunmods() ) {
                    temp1.str( "" );
                    if( mod->has_flag( "IRREMOVABLE" ) ) {
                        temp1 << _( "[Integrated]" );
                    }
                    temp1 << _( "Mod: " ) << "<bold>" << mod->tname() << "</bold> (" << _( mod->type->gunmod->location.c_str() ) <<
                          ")";
                    insert_separation_line();
                    info.push_back( iteminfo( "DESCRIPTION", temp1.str() ) );
                    info.push_back( iteminfo( "DESCRIPTION", mod->type->description ) );
                }
            } else {
                info.push_back( iteminfo( "DESCRIPTION", contents[0].type->description ) );
            }
        }

        // list recipes you could use it in
        itype_id tid;
        if( contents.empty() ) { // use this item
            tid = type->id;
        } else { // use the contained item
            tid = contents[0].type->id;
        }
        const std::vector<recipe *> &rec = recipe_dict.of_component( tid );
        if( !rec.empty() ) {
            temp1.str( "" );
            const inventory &inv = g->u.crafting_inventory();
            // only want known recipes
            std::vector<recipe *> known_recipes;
            for( recipe *r : rec ) {
                if( g->u.knows_recipe( r ) ) {
                    known_recipes.push_back( r );
                }
            }
            if( known_recipes.size() > 24 ) {
                insert_separation_line();
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "You know dozens of things you could craft with it." ) ) );
            } else if( known_recipes.size() > 12 ) {
                insert_separation_line();
                info.push_back( iteminfo( "DESCRIPTION", _( "You could use it to craft various other things." ) ) );
            } else {
                bool found_recipe = false;
                for( recipe *r : known_recipes ) {
                    if( found_recipe ) {
                        temp1 << _( ", " );
                    }
                    found_recipe = true;
                    // darken recipes you can't currently craft
                    bool can_make = r->can_make_with_inventory( inv );
                    if( !can_make ) {
                        temp1 << "<dark>";
                    }
                    temp1 << item::nname( r->result );
                    if( !can_make ) {
                        temp1 << "</dark>";
                    }
                }
                if( found_recipe ) {
                    insert_separation_line();
                    info.push_back( iteminfo( "DESCRIPTION", string_format( _( "You could use it to craft: %s" ),
                                              temp1.str().c_str() ) ) );
                }
            }
        }
    }

    if( !info.empty() && info.back().sName == "--" ) {
        info.pop_back();
    }

    temp1.str( "" );
    for( auto &elem : info ) {
        if( elem.sType == "DESCRIPTION" ) {
            temp1 << "\n";
        }

        if( elem.bDrawName ) {
            temp1 << elem.sName;
        }
        size_t pos = elem.sFmt.find( "<num>" );
        std::string sPost = "";
        if( pos != std::string::npos ) {
            temp1 << elem.sFmt.substr( 0, pos );
            sPost = elem.sFmt.substr( pos + 5 );
        } else {
            temp1 << elem.sFmt.c_str();
        }
        if( elem.sValue != "-999" ) {
            temp1 << elem.sPlus << "<neutral>" << elem.sValue << "</neutral>";
        }
        temp1 << sPost;
        temp1 << ( ( elem.bNewLine ) ? "\n" : "" );
    }

    return replace_colors( temp1.str() );
}

int item::get_free_mod_locations( const std::string &location ) const
{
    if( !is_gun() ) {
        return 0;
    }
    const islot_gun *gt = type->gun.get();
    std::map<std::string, int>::const_iterator loc =
        gt->valid_mod_locations.find( location );
    if( loc == gt->valid_mod_locations.end() ) {
        return 0;
    }
    int result = loc->second;
    for( const auto &elem : contents ) {
        const auto mod = elem.type->gunmod.get();
        if( mod != NULL && mod->location == location ) {
            result--;
        }
    }
    return result;
}

int item::engine_displacement() const
{
    return type->engine ? type->engine->displacement : 0;
}

char item::symbol() const
{
    if( is_null() )
        return ' ';
    return type->sym;
}

nc_color item::color_in_inventory() const
{
    player* const u = &g->u; // TODO: make a reference, make a const reference
    nc_color ret = c_ltgray;

    if(has_flag("WET")) {
        ret = c_cyan;
    } else if(has_flag("LITCIG")) {
        ret = c_red;
    } else if ( has_flag("LEAK_DAM") && has_flag("RADIOACTIVE") && damage > 0 ) {
        ret = c_ltgreen;
    } else if (active && !is_food() && !is_food_container()) { // Active items show up as yellow
        ret = c_yellow;
    } else if( is_food() || is_food_container() ) {
        const bool preserves = type->container && type->container->preserves;
        const item &to_color = is_food() ? *this : contents[0];
        // Default: permafood, drugs
        // Brown: rotten (for non-saprophages) or non-rotten (for saprophages)
        // Dark gray: inedible
        // Red: morale penalty
        // Yellow: will rot soon
        // Cyan: will rot eventually
        const auto rating = u->can_eat( to_color );
        // TODO: More colors
        switch( rating ) {
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
                ret = c_dkgray;
                break;
            case ALLERGY:
            case ALLERGY_WEAK:
            case CANNIBALISM:
                ret = c_red;
                break;
            case ROTTEN:
                ret = c_brown;
                break;
            case NO_TOOL:
                break;
        }
    } else if( is_gun() ) {
        // Guns are green if you are carrying ammo for them
        // ltred if you have ammo but no mags
        // Gun with integrated mag counts as both
        ammotype amtype = ammo_type();
        bool has_ammo = !u->find_ammo( *this, false, -1 ).empty();
        bool has_mag = magazine_integral() || !u->find_ammo( *this, true, -1 ).empty();
        if( has_ammo && has_mag ) {
            ret = c_green;
        } else if( has_ammo || has_mag ) {
            ret = c_ltred;
        }
    } else if( is_ammo() ) {
        // Likewise, ammo is green if you have guns that use it
        // ltred if you have the gun but no mags
        // Gun with integrated mag counts as both
        ammotype amtype = ammo_type();
        bool has_gun = u->has_gun_for_ammo( amtype );
        bool has_mag = u->has_magazine_for_ammo( amtype );
        if( has_gun && has_mag ) {
            ret = c_green;
        } else if( has_gun || has_mag ) {
            ret = c_ltred;
        }
    } else if( is_magazine() ) {
        // Magazines are green if you have guns and ammo for them
        // ltred if you have one but not the other
        ammotype amtype = ammo_type();
        bool has_gun = u->has_item_with( [this]( const item & it ) {
            return it.is_gun() && it.magazine_compatible().count( typeId() ) > 0;
        } );
        bool has_ammo = !u->find_ammo( *this, false, -1 ).empty();
        if( has_gun && has_ammo ) {
            ret = c_green;
        } else if( has_gun || has_ammo ) {
            ret = c_ltred;
        }
    } else if (is_book()) {
        if(u->has_identified( type->id )) {
            auto &tmp = *type->book;
            if( tmp.skill && // Book can improve skill: blue
                u->get_skill_level( tmp.skill ).can_train() &&
                u->get_skill_level( tmp.skill ) >= tmp.req &&
                u->get_skill_level( tmp.skill ) < tmp.level ) {
                ret = c_ltblue;
            } else if( !u->studied_all_recipes( *type ) ) { // Book can't improve skill right now, but has more recipes: yellow
                ret = c_yellow;
            } else if( tmp.skill && // Book can't improve skill right now, but maybe later: pink
                       u->get_skill_level( tmp.skill ).can_train() &&
                       u->get_skill_level( tmp.skill ) < tmp.level ) {
                ret = c_pink;
            } else if( !tmp.use_methods.empty() && // Book has function or can teach new martial art: blue
                // TODO: replace this terrible hack to rely on the item name matching the style name, it's terrible.
                       (!item_group::group_contains_item("ma_manuals", type->id) || !u->has_martialart(matype_id( "style_" + type->id.substr(7)))) ) {
                ret = c_ltblue;
            }
        } else {
            ret = c_red; // Book hasn't been identified yet: red
        }
    } else if (is_bionic()) {
        if (!u->has_bionic(type->id)) {
            ret = u->bionic_installation_issues( type->id ).empty() ? c_green : c_red;
        }
    }
    return ret;
}

void item::on_wear( player &p )
{
    if (is_sided() && get_side() == BOTH) {
        // for sided items wear the item on the side which results in least encumbrance
        int lhs = 0, rhs = 0;

        set_side(LEFT);
        const auto left_enc = p.get_encumbrance( *this );
        for( size_t i = 0; i < num_bp; i++ ) {
            lhs += left_enc[i].encumbrance;
        }

        set_side(RIGHT);
        const auto right_enc = p.get_encumbrance( *this );
        for( size_t i = 0; i < num_bp; i++ ) {
            rhs += right_enc[i].encumbrance;
        }

        set_side(lhs <= rhs ? LEFT : RIGHT);
    }

    // TODO: artifacts currently only work with the player character
    if( &p == &g->u && type->artifact ) {
        g->add_artifact_messages( type->artifact->effects_worn );
    }

    p.on_item_wear( *this );
}

void item::on_takeoff (player &p)
{
    p.on_item_takeoff( *this );

    if (is_sided()) {
        set_side(BOTH);
    }
}

void item::on_wield( player &p, int mv )
{
    // TODO: artifacts currently only work with the player character
    if( &p == &g->u && type->artifact ) {
        g->add_artifact_messages( type->artifact->effects_wielded );
    }

    if( has_flag("SLOW_WIELD") && !is_gunmod() ) {
        float d = 32.0; // arbitrary linear scaling factor
        if( is_gun() ) {
            d /= std::max( (float)p.get_skill_level( gun_skill() ),  1.0f );
        } else if( is_weap() ) {
            d /= std::max( (float)p.get_skill_level( weap_skill() ), 1.0f );
        }

        int penalty = get_var( "volume", type->volume ) * d;
        p.moves -= penalty;
        mv += penalty;
    }

    std::string msg;

    if( mv > 250 ) {
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
    // TODO: artifacts currently only work with the player character
    if( &p == &g->u && type->artifact ) {
        g->add_artifact_messages( type->artifact->effects_carried );
    }

    if( is_bucket_nonempty() ) {
        for( const auto &it : contents ) {
            g->m.add_item( p.pos(), it );
        }

        contents.clear();
    }
}

std::string item::tname( unsigned int quantity, bool with_prefix ) const
{
    std::stringstream ret;

// MATERIALS-TODO: put this in json
    std::string damtext = "";
    if ((damage != 0 || ( OPTIONS["ITEM_HEALTH_BAR"] && is_armor() )) && !is_null() && with_prefix) {
        if( damage < 0 )  {
            if( damage < MIN_ITEM_DAMAGE ) {
                damtext = rm_prefix(_("<dam_adj>bugged "));
            } else if ( OPTIONS["ITEM_HEALTH_BAR"] ) {
                auto const &nc_text = get_item_hp_bar(damage);
                damtext = "<color_" + string_from_color(nc_text.second) + ">" + nc_text.first + " </color>";
            } else if (is_gun())  {
                damtext = rm_prefix(_("<dam_adj>accurized "));
            } else {
                damtext = rm_prefix(_("<dam_adj>reinforced "));
            }
        } else {
            if (type->id == "corpse") {
                if (damage == 1) damtext = rm_prefix(_("<dam_adj>bruised "));
                if (damage == 2) damtext = rm_prefix(_("<dam_adj>damaged "));
                if (damage == 3) damtext = rm_prefix(_("<dam_adj>mangled "));
                if (damage == 4) damtext = rm_prefix(_("<dam_adj>pulped "));

            } else if ( OPTIONS["ITEM_HEALTH_BAR"] ) {
                auto const &nc_text = get_item_hp_bar(damage);
                damtext = "<color_" + string_from_color(nc_text.second) + ">" + nc_text.first + " </color>";

            } else {
                damtext = rmp_format("%s ", get_base_material().dmg_adj(damage).c_str());
            }
        }
    }

    if( !faults.empty() ) {
        damtext.insert( 0, _( "faulty " ) );
    }

    std::string vehtext = "";
    if( is_engine() && engine_displacement() > 0 ) {
        vehtext = rmp_format( _( "<veh_adj>%2.1fL " ), engine_displacement() / 100.0f );

    } else if( is_var_veh_part() ) {
        switch( type->variable_bigness->bigness_aspect ) {
            case BIGNESS_WHEEL_DIAMETER:
                //~ inches, e.g. 20" wheel
                vehtext = rmp_format( _( "<veh_adj>%d\" " ), bigness );
                break;
        }
    }

    std::string burntext = "";
    if (with_prefix && !made_of(LIQUID)) {
        if (volume() >= 4 && burnt >= volume() * 2) {
            burntext = rm_prefix(_("<burnt_adj>badly burnt "));
        } else if (burnt > 0) {
            burntext = rm_prefix(_("<burnt_adj>burnt "));
        }
    }

    const std::map<std::string, std::string>::const_iterator iname = item_vars.find("name");
    std::string maintext = "";
    if (corpse != NULL && typeId() == "corpse" ) {
        if (name != "") {
            maintext = rmp_format(ngettext("<item_name>%s corpse of %s",
                                           "<item_name>%s corpses of %s",
                                           quantity), corpse->nname().c_str(), name.c_str());
        } else {
            maintext = rmp_format(ngettext("<item_name>%s corpse",
                                           "<item_name>%s corpses",
                                           quantity), corpse->nname().c_str());
        }
    } else if (typeId() == "blood") {
        if (corpse == NULL || corpse->id == NULL_ID )
            maintext = rm_prefix(ngettext("<item_name>human blood",
                                          "<item_name>human blood",
                                          quantity));
        else
            maintext = rmp_format(ngettext("<item_name>%s blood",
                                           "<item_name>%s blood",
                                           quantity), corpse->nname().c_str());
    }
    else if (iname != item_vars.end()) {
        maintext = iname->second;
    }
    else if( is_gun() || is_tool() || is_magazine() ) {
        ret.str("");
        ret << label(quantity);
        for( const auto mod : gunmods() ) {
            if( !type->gun->built_in_mods.count( mod->typeId() ) ) {
                ret << "+";
            }
        }
        maintext = ret.str();
    } else if( is_armor() && item_tags.count("wooled") + item_tags.count("furred") +
        item_tags.count("leather_padded") + item_tags.count("kevlar_padded") > 0 ) {
        ret.str("");
        ret << label(quantity);
        ret << "+";
        maintext = ret.str();
    } else if (contents.size() == 1) {
        if(contents[0].made_of(LIQUID)) {
            maintext = rmp_format(_("<item_name>%s of %s"), label(quantity).c_str(), contents[0].tname( quantity, with_prefix ).c_str());
        } else if (contents[0].is_food()) {
            maintext = contents[0].charges > 1 ? rmp_format(_("<item_name>%s of %s"), label(quantity).c_str(),
                                                            contents[0].tname(contents[0].charges, with_prefix).c_str()) :
                                                 rmp_format(_("<item_name>%s of %s"), label(quantity).c_str(),
                                                            contents[0].tname( quantity, with_prefix ).c_str());
        } else {
            maintext = rmp_format(_("<item_name>%s with %s"), label(quantity).c_str(), contents[0].tname( quantity, with_prefix ).c_str());
        }
    }
    else if (!contents.empty()) {
        maintext = rmp_format(_("<item_name>%s, full"), label(quantity).c_str());
    } else {
        maintext = label(quantity);
    }

    std::string tagtext = "";
    std::string modtext = "";
    ret.str("");
    if (is_food()) {
        if( rotten() ) {
            ret << _(" (rotten)");
        } else if ( is_going_bad()) {
            ret << _(" (old)");
        } else if( is_fresh() ) {
            ret << _(" (fresh)");
        }

        if (has_flag("HOT")) {
            ret << _(" (hot)");
            }
        if (has_flag("COLD")) {
            ret << _(" (cold)");
            }
    }

    if (has_flag("FIT")) {
        ret << _(" (fits)");
    }

    if( is_disgusting_for( g->u ) ) {
        ret << _(" (filthy)" );
    }

    if (is_tool() && has_flag("USE_UPS")){
        ret << _(" (UPS)");
    }
    if (is_tool() && has_flag("RADIO_MOD")){
        ret << _(" (radio:");
        if( has_flag( "RADIOSIGNAL_1" ) ) {
            ret << _("R)");
        } else if( has_flag( "RADIOSIGNAL_2" ) ) {
            ret << _("B)");
        } else if( has_flag( "RADIOSIGNAL_3" ) ) {
            ret << _("G)");
        } else {
            ret << _("Bug");
        }
    }

    if (has_flag("ATOMIC_AMMO")) {
        modtext += _( "atomic " );
    }

    if( gunmod_find( "barrel_small" ) ) {
        modtext += _( "sawn-off ");
    }

    if(has_flag("WET"))
       ret << _(" (wet)");

    if(has_flag("LITCIG"))
        ret << _(" (lit)");

    if( already_used_by_player( g->u ) ) {
        ret << _( " (used)" );
    }

    if( active && !is_food() && !is_corpse() && ( type->id.length() < 3 || type->id.compare( type->id.length() - 3, 3, "_on" ) != 0 ) ) {
        // Usually the items whose ids end in "_on" have the "active" or "on" string already contained
        // in their name, also food is active while it rots.
        ret << _( " (active)" );
    }

    tagtext = ret.str();

    ret.str("");

    //~ This is a string to construct the item name as it is displayed. This format string has been added for maximum flexibility. The strings are: %1$s: Damage text (eg. "bruised"). %2$s: burn adjectives (eg. "burnt"). %3$s: tool modifier text (eg. "atomic"). %4$s: vehicle part text (eg. "3.8-Liter"). $5$s: main item text (eg. "apple"). %6s: tags (eg. "(wet) (fits)").
    ret << string_format(_("%1$s%2$s%3$s%4$s%5$s%6$s"), damtext.c_str(), burntext.c_str(),
                        modtext.c_str(), vehtext.c_str(), maintext.c_str(), tagtext.c_str());

    static const std::string const_str_item_note("item_note");
    if( item_vars.find(const_str_item_note) != item_vars.end() ) {
        //~ %s is an item name. This style is used to denote items with notes.
        return string_format(_("*%s*"), ret.str().c_str());
    } else {
        return ret.str();
    }
}

std::string item::display_name(unsigned int quantity) const
{
    std::string name = tname(quantity);
    std::string side = "";
    std::string qty  = "";

    switch (get_side()) {
        case LEFT:
            side = string_format(" (%s)", _("left"));
            break;
        case RIGHT:
            side = string_format(" (%s)", _("right"));
            break;
    }

    if( is_container() && contents.size() == 1 && contents[0].charges > 0 ) {
        // a container which is not empty
        qty = string_format(" (%i)", contents[0].charges);
    } else if( is_book() && get_chapters() > 0 ) {
        // a book which has remaining unread chapters
        qty = string_format(" (%i)", get_remaining_chapters(g->u));
    } else if( ammo_capacity() > 0 ) {
        // anything that can be reloaded including tools, magazines, guns and auxiliary gunmods
        qty = string_format(" (%i)", ammo_remaining());
    } else if( is_ammo_container() && !contents.empty() ) {
        qty = string_format( " (%i)", contents[0].charges );
    } else if( count_by_charges() ) {
        qty = string_format(" (%i)", charges);
    }

    return string_format("%s%s%s", name.c_str(), side.c_str(), qty.c_str());
}

nc_color item::color() const
{
    if( is_null() )
        return c_black;
    if( is_corpse() ) {
        return corpse->color;
    }
    return type->color;
}

int item::price( bool practical ) const
{
    int res = 0;

    visit_items( [&res, practical]( const item *e ) {
        if( e->rotten() ) {
            // @todo Special case things that stay useful when rotten
            return VisitResponse::NEXT;
        }

        int child = practical ? e->type->price_post : e->type->price;
        if( e->damage > 0 ) {
            // maximal damage is 4, maximal reduction is 40% of the value.
            child -= child * static_cast<double>( e->damage ) / 10;
        }

        if( e->count_by_charges() || e->made_of( LIQUID ) ) {
            // price from json data is for default-sized stack similar to volume calculation
            child *= e->charges / static_cast<double>( e->type->stack_size );

        } else if( e->magazine_integral() && e->ammo_remaining() && e->ammo_data() ) {
            // items with integral magazines may contain ammunition which can affect the price
            child += item( e->ammo_data(), calendar::turn, e->charges ).price( practical );

        } else if( e->is_tool() && e->ammo_type() == "NULL" && e->ammo_capacity() ) {
            // if tool has no ammo (eg. spray can) reduce price proportional to remaining charges
            child *= e->ammo_remaining() / double( std::max( e->type->charges_default(), 1 ) );
        }

        res += child;
        return VisitResponse::NEXT;
    } );

    return res;
}

// MATERIALS-TODO: add a density field to materials.json
int item::weight() const
{
    if( is_null() ) {
        return 0;
    }

    int ret = get_var( "weight", type->weight );
    if( has_flag( "REDUCED_WEIGHT" ) ) {
        ret *= 0.75;
    }

    if( count_by_charges() ) {
        ret *= charges;

    } else if( is_corpse() ) {
        switch( corpse->size ) {
            case MS_TINY:   ret =   1000;  break;
            case MS_SMALL:  ret =  40750;  break;
            case MS_MEDIUM: ret =  81500;  break;
            case MS_LARGE:  ret = 120000;  break;
            case MS_HUGE:   ret = 200000;  break;
        }
        if( made_of( material_id( "veggy" ) ) ) {
            ret /= 3;
        }
        if( corpse->in_species( FISH ) || corpse->in_species( BIRD ) || corpse->in_species( INSECT ) || made_of( material_id( "bone" ) ) ) {
            ret /= 8;
        } else if ( made_of( material_id( "iron" ) ) || made_of( material_id( "steel" ) ) || made_of( material_id( "stone" ) ) ) {
            ret *= 7;
        }

    } else if( magazine_integral() && !is_magazine() ) {
        if ( ammo_type() == "plutonium" ) {
            ret += ammo_remaining() * find_type( default_ammo( ammo_type() ) )->weight / PLUTONIUM_CHARGES;
        } else if( ammo_data() ) {
            ret += ammo_remaining() * ammo_data()->weight;
        }
    }

    // if this is an ammo belt add the weight of any implicitly contained linkages
    if( is_magazine() && type->magazine->linkage != "NULL" ) {
        item links( type->magazine->linkage, calendar::turn );
        links.charges = ammo_remaining();
        ret += links.weight();
    }

    // reduce weight for sawn-off weepons capped to the apportioned weight of the barrel
    if( gunmod_find( "barrel_small" ) ) {
        float b = type->gun->barrel_length;
        ret -= std::min( b * 250, b / type->volume * type->weight );
    }

    // tool mods also add about a pound of weight
    if( has_flag("ATOMIC_AMMO") ) {
        ret += 250;
    }

    for( auto &elem : contents ) {
        ret += elem.weight();
    }

    return ret;
}

int item::precise_unit_volume() const
{
    if( count_by_charges() || made_of( LIQUID ) ) {
        return get_var( "volume", type->volume ) * 1000 / type->stack_size;
    }
    return volume() * 1000;
}

int item::volume( bool integral ) const
{
    if( is_null() ) {
        return 0;
    }

    if( is_corpse() ) {
        switch( corpse->size ) {
            case MS_TINY:    return    3;
            case MS_SMALL:   return  120;
            case MS_MEDIUM:  return  250;
            case MS_LARGE:   return  370;
            case MS_HUGE:    return 3500;
        }
        debugmsg( "unknown monster size for corpse" );
        return 0;
    }

    int ret = get_var( "volume", integral ? type->integral_volume : type->volume );

    // For items counted per charge the above volume is per stack so adjust dependent upon charges
    if( count_by_charges() || made_of( LIQUID ) ) {
        ret = ceil( ret * double( charges ) / type->stack_size );
    }

    // Non-rigid items add the volume of the content
    if( !type->rigid ) {
        for( auto &elem : contents ) {
            ret += elem.volume();
        }
    }

    // Some magazines sit (partly) flush with the item so add less extra volume
    if( magazine_current() != nullptr ) {
        ret += std::max( magazine_current()->volume() - type->magazine_well, 0 );
    }

    if (is_gun()) {
        for( const auto elem : gunmods() ) {
            ret += elem->volume( true );
        }

        // @todo implement stock_length property for guns
        if (has_flag("COLLAPSIBLE_STOCK")) {
            // consider only the base size of the gun (without mods)
            int tmpvol = get_var( "volume", type->volume - type->gun->barrel_length );
            if     ( tmpvol <=  3 ) ; // intentional NOP
            else if( tmpvol <=  5 ) ret -= 2;
            else if( tmpvol <=  6 ) ret -= 3;
            else if( tmpvol <=  8 ) ret -= 4;
            else if( tmpvol <= 11 ) ret -= 5;
            else if( tmpvol <= 16 ) ret -= 6;
            else                    ret -= 7;
        }

        if( gunmod_find( "barrel_small" ) ) {
            ret -= type->gun->barrel_length;
        }
    }

    // Battery mods also add volume
    if( has_flag("ATOMIC_AMMO") ) {
        ret += 1;
    }

    if( has_flag("DOUBLE_AMMO") ) {
        // Batteries have volume 1 per 100 charges
        // TODO: De-hardcode this
        ret += type->maximum_charges() / 100;
    }

    return ret;
}

int item::lift_strength() const
{
    return weight() / STR_LIFT_FACTOR + ( weight() % STR_LIFT_FACTOR != 0 );
}

int item::attack_time() const
{
    int ret = 65 + 4 * volume() + weight() / 60;
    return ret;
}

int item::damage_bash() const
{
    int total = type->melee_dam;
    if( is_null() ) {
        return 0;
    }
    total -= total * (damage * 0.1);
    if(has_flag("REDUCED_BASHING")) {
        total *= 0.5;
    }
    if (total > 0) {
        return total;
    } else {
        return 0;
    }
}

int item::damage_cut() const
{
    int total = type->melee_cut;
    if (is_gun()) {
        static const std::string FLAG_BAYONET( "BAYONET" );
        for( auto &elem : contents ) {
            if( elem.has_flag( FLAG_BAYONET ) ) {
                return elem.type->melee_cut;
            }
        }
    }

    if( is_null() ) {
        return 0;
    }

    total -= total * (damage * 0.1);
    if (total > 0) {
        return total;
    } else {
        return 0;
    }
}

int item::damage_by_type( damage_type dt ) const
{
    switch( dt ) {
        case DT_BASH:
            return damage_bash();
        case DT_CUT:
            return ( has_flag( "SPEAR" ) || has_flag( "STAB" ) ) ? 0 : damage_cut();
        case DT_STAB:
            return ( has_flag( "SPEAR" ) || has_flag( "STAB" ) ) ? damage_cut() : 0;
        default:
            break;
    }

    return 0;
}

int item::reach_range() const
{
    if( is_gunmod() || !has_flag( "REACH_ATTACK" ) ) {
        return 1;
    }

    return has_flag( "REACH3" ) ? 3 : 2;
}



void item::unset_flags()
{
    item_tags.clear();
}

bool item::has_flag( const std::string &f ) const
{
    bool ret = false;
    // TODO: this might need checking against the firing code, that code should use the
    // auxiliary gun mod item directly (and call has_flag on it, *not* on the gun),
    // e.g. for the NEVER_JAMS flag, that should not be inherited to the gun mod
    if (is_gun()) {
        if (is_in_auxiliary_mode()) {
            item const* gunmod = gunmod_current();
            if( gunmod != NULL )
                ret = gunmod->has_flag(f);
            if (ret) return ret;
        } else {
            for( auto &elem : contents ) {
                // Don't report flags from active gunmods for the gun.
                if( elem.has_flag( f ) && !( elem.is_auxiliary_gunmod() || elem.is_magazine() ) ) {
                    ret = true;
                    return ret;
                }
            }
        }
    }
    // other item type flags
    ret = type->item_tags.count(f);
    if (ret) {
        return ret;
    }

    // now check for item specific flags
    ret = item_tags.count(f);
    return ret;
}

bool item::has_any_flag( const std::vector<std::string>& flags ) const
{
    for( auto &flag : flags ) {
        if( has_flag( flag ) ) {
            return true;
        }
    }

    return false;
}

bool item::has_property( const std::string& prop ) const {
   return type->properties.find(prop) != type->properties.end();
}

std::string item::get_property_string( const std::string &prop, const std::string& def ) const
{
    const auto it = type->properties.find(prop);
    return it != type->properties.end() ? it->second : def;
}

long item::get_property_long( const std::string& prop, long def ) const
{
    const auto it = type->properties.find( prop );
    if  (it != type->properties.end() ) {
        char *e = nullptr;
        long  r = std::strtol( it->second.c_str(), &e, 10 );
        if( it->second.size() && *e == '\0' ) {
            return r;
        }
        debugmsg("invalid property '%s' for item '%s'", prop.c_str(), tname().c_str());
    }
    return def;
}

int item::get_quality( const quality_id &id ) const
{
    int return_quality = INT_MIN;
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

bool item::has_technique( const matec_id & tech ) const
{
    return type->techniques.count( tech ) > 0 || techniques.count( tech ) > 0;
}

void item::add_technique( const matec_id & tech )
{
    techniques.insert( tech );
}

std::set<matec_id> item::get_techniques() const
{
    std::set<matec_id> result = type->techniques;
    result.insert( techniques.begin(), techniques.end() );
    return result;
}

bool item::goes_bad() const
{
    return is_food() && type->comestible->spoils;
}

double item::get_relative_rot() const
{
    return goes_bad() ? rot / double( type->comestible->spoils ) : 0;
}

void item::set_relative_rot( double val )
{
    if( goes_bad() ) {
        rot = type->comestible->spoils * val;
        // calc_rot uses last_rot_check (when it's not 0) instead of bday.
        // this makes sure the rotting starts from now, not from bday.
        last_rot_check = calendar::turn;
        fridge = 0;
        active = !rotten();
    }
}

void item::calc_rot(const tripoint &location)
{
    const int now = calendar::turn;
    if ( last_rot_check + 10 < now ) {
        const int since = ( last_rot_check == 0 ? bday : last_rot_check );
        const int until = ( fridge > 0 ? fridge : now );
        if ( since < until ) {
            // rot (outside of fridge) from bday/last_rot_check until fridge/now
            int old = rot;
            rot += get_rot_since( since, until, location );
            add_msg( m_debug, "r: %s %d,%d %d->%d", type->id.c_str(), since, until, old, rot );
        }
        last_rot_check = now;

        if (fridge > 0) {
            // Flat 20%, rot from time of putting it into fridge up to now
            rot += (now - fridge) * 0.2;
            fridge = 0;
        }
        // item stays active to let the item counter work
        if( item_counter == 0 && rotten() ) {
            active = false;
        }
    }
}

bool item::is_auxiliary_gunmod() const
{
    return type->gunmod && type->gun;
}

int item::get_storage() const
{
    auto t = find_armor_data();
    if( t == nullptr )
        return 0;

    // it_armor::storage is unsigned char
    return static_cast<int> (static_cast<unsigned int>( t->storage ) );
}

int item::get_env_resist() const
{
    const auto t = find_armor_data();
    if( t == nullptr ) {
        return 0;
    }
    // it_armor::env_resist is unsigned char
    return static_cast<int>( static_cast<unsigned int>( t->env_resist ) );
}

bool item::is_power_armor() const
{
    const auto t = find_armor_data();
    if( t == nullptr ) {
        return false;
    }
    return t->power_armor;
}

int item::get_encumber() const
{
    const auto t = find_armor_data();
    if( t == nullptr ) {
        // handle wearable guns (eg. shoulder strap) as special case
        return is_gun() ? volume() / 3 : 0;
    }
    // it_armor::encumber is signed char
    int encumber = static_cast<int>( t->encumber );

    // Non-rigid items add additional encumbrance proportional to their volume
    if( !type->rigid ) {
        for( const auto &e : contents ) {
            encumber += e.volume();
        }
    }

    // Fit checked before changes, fitting shouldn't reduce penalties from patching.
    if( item::item_tags.count("FIT") ) {
        encumber = std::max( encumber / 2, encumber - 10 );
    }

    const int thickness = get_thickness();
    const int coverage = get_coverage();
    if( item::item_tags.count("wooled") ) {
        encumber += 1 + 3 * coverage / 100;
    }
    if( item::item_tags.count("furred") ){
        encumber += 1 + 4 * coverage / 100;
    }

    if( item::item_tags.count("leather_padded") ) {
        encumber += thickness * coverage / 100 + 5;
    }
    if( item::item_tags.count("kevlar_padded") ) {
        encumber += thickness * coverage / 100 + 5;
    }

    return encumber;
}

int item::get_layer() const
{
    if( has_flag("SKINTIGHT") ) {
        return UNDERWEAR;
    } else if( has_flag("WAIST") ) {
        return WAIST_LAYER;
    } else if( has_flag("OUTER") ) {
        return OUTER_LAYER;
    } else if( has_flag("BELTED") ) {
        return BELTED_LAYER;
    }
    return REGULAR_LAYER;
}

int item::get_coverage() const
{
    const auto t = find_armor_data();
    if( t == nullptr ) {
        return 0;
    }
    // it_armor::coverage is unsigned char
    return static_cast<int>( static_cast<unsigned int>( t->coverage ) );
}

int item::get_thickness() const
{
    const auto t = find_armor_data();
    if( t == nullptr ) {
        return 0;
    }
    // it_armor::thickness is unsigned char
    return static_cast<int>( static_cast<unsigned int>( t->thickness) );
}

int item::get_warmth() const
{
    int fur_lined = 0;
    int wool_lined = 0;
    const auto t = find_armor_data();
    if( t == nullptr ){
        return 0;
    }
    // it_armor::warmth is signed char
    int result = static_cast<int>( t->warmth );

    if( item::item_tags.count("furred") > 0 ) {
        fur_lined = 35 * get_coverage() / 100;
    }

    if( item::item_tags.count("wooled") > 0 ) {
        wool_lined = 20 * get_coverage() / 100;
    }

    return result + fur_lined + wool_lined;
}


int item::brewing_time() const
{
    return ( is_brewable() ? type->brewable->time : 0 ) * ( ACTIVE_WORLD_OPTIONS["SEASON_LENGTH"] / 14.0 );
}

const std::vector<itype_id> &item::brewing_results() const
{
    static const std::vector<itype_id> nulresult;
    return is_brewable() ? type->brewable->results : nulresult;
}

bool item::can_revive() const
{
    if( is_corpse() && corpse->has_flag( MF_REVIVES ) && damage < CORPSE_PULP_THRESHOLD ) {
        return true;
    }
    return false;
}

bool item::ready_to_revive( const tripoint &pos ) const
{
    if(can_revive() == false) {
        return false;
    }
    int age_in_hours = (int(calendar::turn) - bday) / HOURS( 1 );
    age_in_hours -= int((float)burnt / volume() * 24);
    if( damage > 0 ) {
        age_in_hours /= (damage + 1);
    }
    int rez_factor = 48 - age_in_hours;
    if( age_in_hours > 6 && (rez_factor <= 0 || one_in(rez_factor)) ) {
        // If we're a special revival zombie, wait to get up until the player is nearby.
        const bool isReviveSpecial = has_flag("REVIVE_SPECIAL");
        if( isReviveSpecial ) {
            const int distance = rl_dist( pos, g->u.pos() );
            if (distance > 3) {
                return false;
            }
            if (!one_in(distance + 1)) {
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

bool item::craft_has_charges()
{
    if (count_by_charges()) {
        return true;
    } else if (ammo_type() == "NULL") {
        return true;
    }

    return false;
}

int item::bash_resist( bool to_self ) const
{
    float resist = 0;
    float l_padding = 0;
    float k_padding = 0;
    int eff_thickness = 1;
    // With the multiplying and dividing in previous code, the following
    // is a coefficient equivalent to the bonuses and maluses hardcoded in
    // previous versions. Adjust to make you happier/sadder.
    float adjustment = 1.5;

    static constexpr float max_value = 10.0f;
    static constexpr float stepness = -0.8f;
    static constexpr float center_of_S = 2.0f;

    if( is_null() ) {
        return resist;
    }
    if( item_tags.count("leather_padded") > 0 ){
        l_padding = max_value / ( 1 + exp( stepness * ( get_thickness() - center_of_S )));
    }
    if( item_tags.count("kevlar_padded") > 0 ){
        k_padding = max_value / ( 1 + exp( stepness * ( get_thickness() - center_of_S )));
    }
    // Armor gets an additional multiplier.
    if( is_armor() ) {
        // base resistance
        // Don't give reinforced items +armor, just more resistance to ripping
        const int eff_damage = to_self ? std::min( damage, 0 ) : std::max( damage, 0 );
        eff_thickness = std::max( 1, get_thickness() - eff_damage );
    }

    const std::vector<const material_type*> mat_types = made_of_types();
    if( !mat_types.empty() ) {
        for (auto mat : mat_types) {
            resist += mat->bash_resist();
        }
        // Average based on number of materials.
        resist /= mat_types.size();
    }

    return lround((resist * eff_thickness * adjustment) + l_padding + k_padding);
}

int item::cut_resist( bool to_self ) const
{
    float resist = 0;
    float l_padding = 0;
    float k_padding = 0;
    int eff_thickness = 1;
    // With the multiplying and dividing in previous code, the following
    // is a coefficient equivalent to the bonuses and maluses hardcoded in
    // previous versions. Adjust to make you happier/sadder.
    float adjustment = 1.5;

    if( is_null() ) {
        return resist;
    }
    if( item_tags.count("leather_padded") > 0 ){
        static constexpr float max_value = 10.0f;
        static constexpr float stepness = -0.8f;
        static constexpr float center_of_S = 2.0f;
        l_padding = max_value / ( 1 + exp( stepness * ( get_thickness() - center_of_S )));
    }
    if( item_tags.count("kevlar_padded") > 0 ){
        static constexpr float max_value = 15.0f;
        static constexpr float stepness = -0.5f;
        static constexpr float center_of_S = 2.0f;
        k_padding = max_value / ( 1 + exp( stepness * ( get_thickness() - center_of_S )));
    }
    // Armor gets an additional multiplier.
    if( is_armor() ) {
        // base resistance
        // Don't give reinforced items +armor, just more resistance to ripping
        const int eff_damage = to_self ? std::min( damage, 0 ) : std::max( damage, 0 );
        eff_thickness = std::max( 1, get_thickness() - eff_damage );
    }

    const std::vector<const material_type*> mat_types = made_of_types();
    if( !mat_types.empty() ) {
        for( auto mat : mat_types ) {
            resist += mat->cut_resist();
        }
        // Average based on number of materials.
        resist /= mat_types.size();
    }

    return lround((resist * eff_thickness * adjustment) + l_padding + k_padding);
}

int item::stab_resist(bool to_self) const
{
    // Better than hardcoding it in multiple places
    return (int)(0.8f * cut_resist( to_self ));
}

int item::acid_resist( bool to_self ) const
{
    if( to_self ) {
        // Currently no items are damaged by acid
        return INT_MAX;
    }

    float resist = 0.0;
    if( is_null() ) {
        return 0.0;
    }

    const std::vector<const material_type*> mat_types = made_of_types();
    if( !mat_types.empty() ) {
        // Not sure why cut and bash get an armor thickness bonus but acid doesn't,
        // but such is the way of the code.

        for( auto mat : mat_types ) {
            resist += mat->acid_resist();
        }
        // Average based on number of materials.
        resist /= mat_types.size();
    }

    const int env = get_env_resist();
    if( !to_self && env < 10 ) {
        // Low env protection means it doesn't prevent acid seeping in.
        resist *= env / 10.0f;
    }

    return lround(resist);
}

int item::fire_resist( bool to_self ) const
{
    float resist = 0.0;
    if( is_null() ) {
        return 0.0;
    }

    const std::vector<const material_type*> mat_types = made_of_types();
    if( !mat_types.empty() ) {
        for( auto mat : mat_types ) {
            resist += mat->fire_resist();
        }
        // Average based on number of materials.
        resist /= mat_types.size();
    }

    const int env = get_env_resist();
    if( !to_self && env < 10 ) {
        // Iron resists immersion in magma, iron-clad knight won't.
        resist *= env / 10.0f;
    }

    return lround(resist);
}

int item::chip_resistance( bool worst ) const
{
    if( damage > MAX_ITEM_DAMAGE ) {
        return 0;
    }

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
            return cut_resist ( to_self );
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
    if( has_flag("ALWAYS_TWOHAND") ) {
        return true;
    }
    ///\EFFECT_STR determines which weapons can be wielded with one hand
    return ((weight() / 113) > u.str_cur * 4);
}

const std::vector<material_id> &item::made_of() const
{
    if( is_corpse() ) {
        return corpse->mat;
    }
    return type->materials;
}

std::vector<const material_type*> item::made_of_types() const
{
    std::vector<const material_type*> material_types_composed_of;
    for (auto mat_id : made_of()) {
        material_types_composed_of.push_back( &mat_id.obj() );
    }
    return material_types_composed_of;
}

bool item::made_of_any( const std::vector<material_id> &mat_idents ) const
{
    const auto mats = made_of();
    if( mats.empty() ) {
        return false;
    }
    for( auto candidate_material : mat_idents ) {
        if( std::find( mats.begin(), mats.end(), candidate_material ) != mats.end() ) {
            return true;
        }
    }
    return false;
}

bool item::only_made_of( const std::vector<material_id> &mat_idents ) const
{
    const auto mats = made_of();
    if( mats.empty() ) {
        return false;
    }
    for( auto target_material : mats ) {
        if( std::find( mat_idents.begin(), mat_idents.end(), target_material ) == mat_idents.end() ) {
            return false;
        }
    }
    return true;
}

bool item::made_of( const material_id &mat_ident ) const
{
    const auto &materials = made_of();
    return std::find( materials.begin(), materials.end(), mat_ident ) != materials.end();
}

bool item::made_of(phase_id phase) const
{
    if (is_null()) {
        return false;
    }
    return (type->phase == phase);
}


bool item::conductive() const
{
    if (is_null()) {
        return false;
    }

    // If any material does not resist electricity we are conductive.
    for (auto mat : made_of_types()) {
        if (mat->elec_resist() <= 0) {
            return true;
        }
    }
    return false;
}

bool item::destroyed_at_zero_charges() const
{
    return (is_ammo() || is_food());
}

bool item::is_var_veh_part() const
{
    return type->variable_bigness.get() != nullptr;
}

bool item::is_gun() const
{
    return type->gun.get() != nullptr;
}

bool item::is_firearm() const
{
    static const std::string primitive_flag( "PRIMITIVE_RANGED_WEAPON" );
    return is_gun() && !has_flag( primitive_flag );
}

bool item::is_silent() const
{
    return gun_noise().volume < 5;
}

bool item::is_gunmod() const
{
    return type->gunmod.get() != nullptr;
}

bool item::is_bionic() const
{
    return type->bionic.get() != nullptr;
}

bool item::is_magazine() const
{
    return type->magazine.get() != nullptr;
}

bool item::is_ammo_belt() const
{
    return is_magazine() && has_flag( "MAG_BELT" );
}

bool item::is_ammo() const
{
    return type->ammo.get() != nullptr;
}

bool item::is_food(player const*u) const
{
    if (!u)
        return is_food();

    if( is_null() )
        return false;

    if( type->comestible ) {
        return true;
    }

    if( u->has_active_bionic( "bio_batteries" ) && is_ammo() && ammo_type() == "battery" ) {
        return true;
    }

    if( ( u->has_active_bionic( "bio_reactor" ) || u->has_active_bionic( "bio_advreactor" ) ) && is_ammo() && ( ammo_type() == "reactor_slurry" || ammo_type() == "plutonium" ) ) {
        return true;
    }
    if (u->has_active_bionic("bio_furnace") && flammable() && typeId() != "corpse")
        return true;
    return false;
}

bool item::is_food_container(player const*u) const
{
    return (contents.size() >= 1 && contents[0].is_food(u));
}

bool item::is_food() const
{
    return type->comestible != nullptr;
}

bool item::is_brewable() const
{
    return type->brewable != nullptr;
}

bool item::is_food_container() const
{
    return (contents.size() >= 1 && contents[0].is_food());
}

bool item::is_corpse() const
{
    return typeId() == "corpse" && corpse != nullptr;
}

const mtype *item::get_mtype() const
{
    return corpse;
}

void item::set_mtype( const mtype * const m )
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
    return !is_magazine() && !contents.empty() && contents[0].is_ammo();
}

bool item::is_weap() const
{
    if( is_null() )
        return false;

    if (is_gun() || is_food() || is_ammo() || is_food_container() || is_armor() ||
            is_book() || is_tool() || is_bionic() || is_gunmod())
        return false;
    return (type->melee_dam > 7 || type->melee_cut > 5);
}

bool item::is_bashing_weapon() const
{
    if( is_null() )
        return false;

    return (type->melee_dam >= 8);
}

bool item::is_cutting_weapon() const
{
    if( is_null() )
        return false;

    return (type->melee_cut >= 8 && !has_flag("SPEAR"));
}

const islot_armor *item::find_armor_data() const
{
    if( type->armor ) {
        return type->armor.get();
    }
    // Currently the only way to make a non-armor item into armor is to install a gun mod.
    // The gunmods are stored in the items contents, as are the contents of a container, and the
    // tools in a tool belt (a container actually), or the ammo in a quiver (container again).
    for( const auto mod : gunmods() ) {
        if( mod->type->armor ) {
            return mod->type->armor.get();
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
    return type->book.get() != nullptr;
}

bool item::is_container() const
{
    return type->container.get() != nullptr;
}

bool item::is_watertight_container() const
{
    return type->container && type->container->watertight && type->container->seals;
}

bool item::is_sealable_container() const
{
    return type->container && type->container->seals;
}

bool item::is_bucket() const
{
    // That "preserves" part is a hack:
    // Currently all non-empty cans are effectively sealed at all times
    // Making them buckets would cause weirdness
    return type->container != nullptr &&
           type->container->watertight &&
           !type->container->seals &&
           !type->container->preserves;
}

bool item::is_bucket_nonempty() const
{
    return is_bucket() && !is_container_empty();
}

bool item::is_engine() const
{
    return type->engine.get() != nullptr;
}

bool item::is_faulty() const
{
    return is_engine() ? !faults.empty() : false;
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
    return get_remaining_capacity_for_liquid( contents[0], allow_bucket ) == 0;
}

bool item::is_salvageable() const
{
    if( is_null() ) {
        return false;
    }
    return !has_flag("NO_SALVAGE");
}

bool item::is_disassemblable() const
{
    if( is_null() ) {
        return false;
    }
    return get_disassemble_recipe(typeId()) != NULL;
}

bool item::is_funnel_container(int &bigger_than) const
{
    if ( ! is_watertight_container() ) {
        return false;
    }
    // todo; consider linking funnel to item or -making- it an active item
    if ( type->container->contains <= bigger_than ) {
        return false; // skip contents check, performance
    }
    if (
        contents.empty() ||
        contents[0].typeId() == "water" ||
        contents[0].typeId() == "water_acid" ||
        contents[0].typeId() == "water_acid_weak") {
        bigger_than = type->container->contains;
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
    return type->tool != nullptr;
}

bool item::is_tool_reversible() const
{
    if( is_tool() && type->tool->revert_to != "null" ) {
        item revert( type->tool->revert_to );
        npc n;
        revert.type->invoke( &n, &revert, tripoint(-999, -999, -999) );
        return revert.is_tool() && typeId() == revert.typeId();
    }
    return false;
}

bool item::is_artifact() const
{
    return type->artifact.get() != nullptr;
}

bool item::can_contain( const item &it ) const
{
    // @todo Volume check
    return can_contain( *it.type );
}

bool item::can_contain( const itype &tp ) const
{
    if( type->container == nullptr ) {
        // @todo: Tools etc.
        return false;
    }

    if( tp.phase == LIQUID && !type->container->watertight ) {
        return false;
    }

    // @todo Acid in waterskins
    return true;
}

bool item::spill_contents( Character &c )
{
    if( c.is_npc() ) {
        return spill_contents( c.pos() );
    }

    while( !contents.empty() ) {
        if( contents[0].made_of( LIQUID ) ) {
            if( !g->handle_liquid_from_container( *this, 1 ) ) {
                return false;
            }
        } else {
            c.i_add_or_drop( contents[0] );
            contents.erase( contents.begin() );
        }
    }

    return true;
}

bool item::spill_contents( const tripoint &pos )
{
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
    const int remain = std::max( 0, get_remaining_chapters( u ) - 1 );
    const auto var = string_format( "remaining-chapters-%d", u.getID() );
    set_var( var, remain );
}

const material_type &item::get_random_material() const
{
    if( type->materials.empty() ) {
        return material_id( "null" ).obj();
    }
    return random_entry( type->materials ).obj();
}

const material_type &item::get_base_material() const
{
    if( type->materials.empty() ) {
        return material_id( "null" ).obj();
    }
    return type->materials.front().obj();
}

bool item::operator<(const item& other) const
{
    const item_category &cat_a = get_category();
    const item_category &cat_b = other.get_category();
    if(cat_a != cat_b) {
        return cat_a < cat_b;
    } else {
        const item *me = is_container() && !contents.empty() ? &contents[0] : this;
        const item *rhs = other.is_container() && !other.contents.empty() ? &other.contents[0] : &other;

        if (me->type->id == rhs->type->id) {
            return me->charges < rhs->charges;
        } else {
            std::string n1 = me->type->nname(1);
            std::string n2 = rhs->type->nname(1);
            return std::lexicographical_compare( n1.begin(), n1.end(),
                                                 n2.begin(), n2.end(), sort_case_insensitive_less() );
        }
    }
}

item* item::gunmod_current()
{
    return const_cast<item*>( const_cast<const item*>( this )->gunmod_current() );
}

item const* item::gunmod_current() const
{
    if( is_in_auxiliary_mode() ) {
        const auto mods = gunmods();
        auto it = std::find_if( mods.begin(), mods.end(), []( const item *e ) {
            return e->is_in_auxiliary_mode();
        } );
        return it != mods.end() ? *it : nullptr;
    }
    return nullptr;
}

bool item::is_in_auxiliary_mode() const
{
    return get_gun_mode() == "MODE_AUX";
}

void item::set_auxiliary_mode()
{
    set_gun_mode( "MODE_AUX" );
}

std::string item::get_gun_mode() const
{
    // has_flag() calls get_gun_mode(), so this:
    const std::string default_mode = type->item_tags.count( "BURST_ONLY" ) ? "MODE_BURST" : "NULL";
    return get_var( GUN_MODE_VAR_NAME, default_mode );
}

void item::set_gun_mode( const std::string &mode )
{
    // a gun mode only makes sense on things that can fire, all other items are ignored!
    if( !is_gun() ) {
        return;
    }
    if( mode.empty() || mode == "NULL" ) {
        erase_var( GUN_MODE_VAR_NAME );
    } else {
        set_var( GUN_MODE_VAR_NAME, mode );
    }
}

void item::next_mode()
{
    auto mode = get_gun_mode();
    if( mode == "NULL" && has_flag("MODE_BURST") ) {
        set_gun_mode("MODE_BURST");
    } else if( mode == "NULL" || mode == "MODE_BURST" ) {
        // mode is MODE_BURST, or item has no MODE_BURST flag and mode is NULL
        // Enable the first mod with an AUX firing mode.
        for( auto &elem : contents ) {
            if( elem.is_auxiliary_gunmod() ) {
                set_auxiliary_mode();
                elem.set_auxiliary_mode();
                return;
            }
        }
        if( has_flag( "REACH_ATTACK" ) ) {
            set_gun_mode( "MODE_REACH" );
        } else {
            set_gun_mode( "NULL" );
        }
    } else if( is_in_auxiliary_mode() ) {
        size_t i = 0;
        // Advance to next aux mode, or if there isn't one, normal mode
        for( ; i < contents.size(); i++ ) {
            if( contents[i].is_gunmod() && contents[i].is_in_auxiliary_mode() ) {
                contents[i].set_gun_mode( "NULL" );
                break;
            }
        }
        for( i++; i < contents.size(); i++ ) {
            if( contents[i].is_auxiliary_gunmod() ) {
                contents[i].set_auxiliary_mode();
                break;
            }
        }
        if( i == contents.size() ) {
            if( has_flag( "REACH_ATTACK" ) ) {
                set_gun_mode( "MODE_REACH" );
            } else {
                set_gun_mode( "NULL" );
            }
        }
    } else if( mode == "MODE_REACH" ) {
        set_gun_mode( "NULL" );
    }
    // ensure MODE_BURST for BURST_ONLY weapons
    mode = get_gun_mode();
    if( mode == "NULL" && has_flag( "BURST_ONLY" ) ) {
        set_gun_mode( "MODE_BURST" );
    }
}

skill_id item::gun_skill() const
{
    if( !is_gun() ) {
        return NULL_ID;
    }
    return type->gun->skill_used;
}

std::string item::gun_type() const
{
    static skill_id skill_archery( "archery" );

    if( !is_gun() ) {
        return std::string();
    }
    if( gun_skill() == skill_archery ) {
        if( ammo_type() == "bolt" || typeId() == "bullet_crossbow" ) {
            return "crossbow";
        } else{
            return "bow";
        }
    }
    return gun_skill().c_str();
}

skill_id item::weap_skill() const
{
    if( !is_weap() && !is_tool() ) {
        return NULL_ID;
    }

    if (type->melee_dam >= type->melee_cut) return skill_bashing;
    if( has_flag("STAB") || has_flag( "SPEAR" ) ) return skill_stabbing;
    return skill_cutting;
}

int item::gun_dispersion( bool with_ammo ) const
{
    if( !is_gun() ) {
        return 0;
    }
    int dispersion_sum = type->gun->dispersion;
    for( const auto mod : gunmods() ) {
        dispersion_sum += mod->type->gunmod->dispersion;
    }
    dispersion_sum += damage * 60;
    dispersion_sum = std::max(dispersion_sum, 0);
    if( with_ammo && ammo_data() ) {
        dispersion_sum += ammo_data()->ammo->dispersion;
    }
    dispersion_sum = std::max(dispersion_sum, 0);
    return dispersion_sum;
}

// Sight dispersion and aim speed pick the best sight bonus to use.
// The best one is the fastest one whose dispersion is under the threshold.
// If you provide a threshold of -1, it just gives lowest dispersion.
int item::sight_dispersion( int aim_threshold ) const
{
    if (!is_gun()) {
        return 0;
    }
    const auto gun = type->gun.get();
    int best_dispersion = gun->sight_dispersion;
    int best_aim_speed = INT_MAX;
    if( gun->sight_dispersion < aim_threshold || aim_threshold == -1 ) {
        best_aim_speed = gun->aim_speed;
    }
    for( const auto e : gunmods() ) {
        const auto mod = e->type->gunmod.get();
        if( mod->sight_dispersion != -1 && mod->aim_speed != -1 &&
            ( ( aim_threshold == -1 && mod->sight_dispersion < best_dispersion ) ||
              ( mod->sight_dispersion < aim_threshold && mod->aim_speed < best_aim_speed ) ) ) {
            best_aim_speed = mod->aim_speed;
            best_dispersion = mod->sight_dispersion;
        }
    }
    return best_dispersion;
}

// This method should never be called if the threshold exceeds the accuracy of the available sights.
int item::aim_speed( int aim_threshold ) const
{
    if (!is_gun()) {
        return 0;
    }
    const auto gun = type->gun.get();
    int best_dispersion = gun->sight_dispersion;
    int best_aim_speed = INT_MAX;
    if( gun->sight_dispersion <= aim_threshold || aim_threshold == -1 ) {
        best_aim_speed = gun->aim_speed;
    }
    for( const auto e : gunmods() ) {
        const auto mod = e->type->gunmod.get();
        if( mod->sight_dispersion != -1 && mod->aim_speed != -1 &&
            ((aim_threshold == -1 && mod->sight_dispersion < best_dispersion ) ||
             (mod->sight_dispersion <= aim_threshold &&
              mod->aim_speed < best_aim_speed)) ) {
            best_aim_speed = mod->aim_speed;
            best_dispersion = mod->sight_dispersion;
        }
    }
    return best_aim_speed;
}

int item::gun_damage( bool with_ammo ) const
{
    if( !is_gun() ) {
        return 0;
    }
    int ret = type->gun->damage;
    if( with_ammo && ammo_data() ) {
        ret += ammo_data()->ammo->damage;
    }
    for( const auto mod : gunmods() ) {
        ret += mod->type->gunmod->damage;
    }
    ret -= damage * 2;
    return ret;
}

int item::gun_pierce( bool with_ammo ) const
{
    if( !is_gun() ) {
        return 0;
    }
    int ret = type->gun->pierce;
    if( with_ammo && ammo_data() ) {
        ret += ammo_data()->ammo->pierce;
    }
    for( const auto mod : gunmods() ) {
        ret += mod->type->gunmod->pierce;
    }
    // TODO: item::damage is not used here, but it is in item::gun_damage?
    return ret;
}

int item::burst_size() const
{
    if( !is_gun() ) {
        return 0;
    }
    // No burst fire for gunmods right now.
    if( is_in_auxiliary_mode() ) {
        return 1;
    }
    int ret = type->gun->burst;
    for( const auto mod : gunmods() ) {
        ret += mod->type->gunmod->burst;
    }
    return std::max( 1, ret );
}

int item::gun_recoil( bool with_ammo ) const
{
    if( !is_gun() ) {
        return 0;
    }
    int ret = type->gun->recoil;
    if( with_ammo && ammo_data() ) {
        ret += ammo_data()->ammo->recoil;
    }
    for( const auto mod : gunmods() ) {
        ret += mod->type->gunmod->recoil;
    }
    ret += 15 * damage;
    return ret;
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
    return std::max( 0, ret );
}

int item::gun_range( const player *p ) const
{
    int ret = gun_range( true );
    if( p == nullptr ) {
        return ret;
    }
    if( !p->can_use( *this, false ) ) {
        return 0;
    }

    // Reduce bow range until player has twice minimm required strength
    if( has_flag( "STR_DRAW" ) ) {
        ret -= std::max( 0, type->min_str * 2 - p->get_str() );
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

    if( is_magazine() ) {
        long res = 0;
        for( const auto& e : contents ) {
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

        if( has_flag("DOUBLE_AMMO") ) {
            res *= 2;
        }
        if( has_flag("ATOMIC_AMMO") ) {
            res *= 100;
        }
    }

    if( is_gun() ) {
        res = type->gun->clip;
    }

    if( is_magazine() ) {
        res = type->magazine->capacity;
    }

    return res;
}

long item::ammo_required() const
{
    if( is_tool() ) {
        return std::max( type->charges_to_use(), 0 );
    }

    if( is_gun() ) {
        if( ammo_type() == "NULL" ) {
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

long item::ammo_consume( long qty, const tripoint& pos ) {
    if( qty < 0 ) {
        debugmsg( "Cannot consume negative quantity of ammo for %s", tname().c_str() );
        return 0;
    }

    item *mag = magazine_current();
    if( mag ) {
        auto res = mag->ammo_consume( qty, pos );
        if( res && ammo_remaining() == 0 ) {
            if( mag->has_flag( "MAG_DESTROY" ) ) {
                contents.erase( std::remove_if( contents.begin(), contents.end(), [&mag]( const item& e ) {
                    return mag == &e;
                } ) );
            } else if ( mag->has_flag( "MAG_EJECT" ) ) {
                g->m.add_item( pos, *mag );
                contents.erase( std::remove_if( contents.begin(), contents.end(), [&mag]( const item& e ) {
                    return mag == &e;
                } ) );
            }
        }
        return res;
    }

    if( is_magazine() ) {
        auto need = qty;
        while( contents.size() ) {
            auto& e = *contents.rbegin();
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

const itype * item::ammo_data() const
{
    const item *mag = magazine_current();
    if( mag ) {
        return mag->ammo_data();
    }

    if( is_ammo() ) {
        return type;
    }

    if( is_magazine() ) {
        return !contents.empty() ? contents[0].ammo_data() : nullptr;
    }

    return curammo;
}

itype_id item::ammo_current() const
{
    const auto ammo = ammo_data();
    return ammo ? ammo->id : "null";
}

ammotype item::ammo_type( bool conversion ) const
{
    if( conversion ) {
        if( has_flag( "ATOMIC_AMMO" ) ) {
            return "plutonium";
        }
        for( const auto mod : gunmods() ) {
            if( !mod->is_auxiliary_gunmod() && mod->ammo_type() != "NULL" ) {
                return mod->ammo_type();
            }
        }
    }

    if( is_gun() ) {
        return type->gun->ammo;
    } else if( is_tool() ) {
        return type->tool->ammo_id;
    } else if( is_magazine() ) {
        return type->magazine->type;
    } else if( is_ammo() ) {
        return type->ammo->type;
    } else if( is_gunmod() ) {
        return type->gunmod->ammo_modifier;
    }
    return "NULL";
}

itype_id item::ammo_default( bool conversion ) const
{
    auto res = default_ammo( ammo_type( conversion ) );
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
    return res;
}

itype_id item::ammo_casing() const
{
    if( !is_gun() || !ammo_data() ) {
        return "null";
    }
    return ammo_data()->ammo->casing;
}

bool item::magazine_integral() const
{
    // finds first ammo type which specifies at least one magazine
    const auto& mags = type->magazines;
    return std::none_of( mags.begin(), mags.end(), []( const std::pair<ammotype, const std::set<itype_id>>& e ) {
        return e.second.size();
    } );
}

itype_id item::magazine_default( bool conversion ) const
{
    auto mag = type->magazine_default.find( ammo_type( conversion ) );
    return mag != type->magazine_default.end() ? mag->second : "null";
}

std::set<itype_id> item::magazine_compatible( bool conversion ) const
{
    // gunmods that define magazine_adaptor may override the items usual magazines
    for( const auto m : gunmods() ) {
        if( !m->type->gunmod->magazine_adaptor.empty() ) {
            auto mags = m->type->gunmod->magazine_adaptor.find( ammo_type( conversion ) );
            return mags != m->type->gunmod->magazine_adaptor.end() ? mags->second : std::set<itype_id>();
        }
    }

    auto mags = type->magazines.find( ammo_type( conversion ) );
    return mags != type->magazines.end() ? mags->second : std::set<itype_id>();
}

item * item::magazine_current()
{
    auto iter = std::find_if( contents.begin(), contents.end(), []( const item& it ) {
        return it.is_magazine();
    });
    return iter != contents.end() ? &*iter : nullptr;
}

const item * item::magazine_current() const
{
    return const_cast<item *>(this)->magazine_current();
}

std::vector<item *> item::gunmods()
{
    std::vector<item *> res;
    if( is_gun() ) {
        res.reserve( contents.size() );
        for( auto& e : contents ) {
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
        for( auto& e : contents ) {
            if( e.is_gunmod() ) {
                res.push_back( &e );
            }
        }
    }
    return res;
}

item * item::gunmod_find( const itype_id& mod )
{
    auto mods = gunmods();
    auto it = std::find_if( mods.begin(), mods.end(), [&mod]( item *e ) {
        return e->typeId() == mod;
    } );
    return it != mods.end() ? *it : nullptr;
}

const item * item::gunmod_find( const itype_id& mod ) const
{
    return const_cast<item *>( this )->gunmod_find( mod );
}

bool item::gunmod_compatible( const item& mod, bool alert, bool effects ) const
{
    if( !mod.is_gunmod() ) {
        debugmsg( "Tried checking compatibility of non-gunmod" );
        return false;
    }

    std::string msg;

    if( !is_gun() ) {
        msg = string_format( _( "That %s is not a weapon." ), tname().c_str() );

    } else if( is_gunmod() ) {
        msg = string_format( _( "That %s is a gunmod, it can not be modded." ), tname().c_str() );

    } else if( gunmod_find( mod.typeId() ) ) {
        msg = string_format( _( "Your %1$s already has a %2$s." ), tname().c_str(), mod.tname( 1 ).c_str() );

    } else if( !type->gun->valid_mod_locations.count( mod.type->gunmod->location ) ) {
        msg = string_format( _( "Your %s doesn't have a slot for this mod." ), tname().c_str() );

    } else if( get_free_mod_locations( mod.type->gunmod->location ) <= 0 ) {
        msg = string_format( _( "Your %1$s doesn't have enough room for another %2$s mod." ), tname().c_str(), _( mod.type->gunmod->location.c_str() ) );

    } else if( effects && ( mod.type->gunmod->ammo_modifier != "NULL" || !mod.type->gunmod->magazine_adaptor.empty() )
                       && ( ammo_remaining() > 0 || magazine_current() ) ) {
        msg = string_format( _( "You must unload your %s before installing this mod." ), tname().c_str() );

    } else if( !mod.type->gunmod->usable.count( gun_type() ) ) {
        msg = string_format( _( "That %s cannot be attached to a %s" ), mod.tname().c_str(), _( gun_type().c_str() ) );

    } else if( typeId() == "hand_crossbow" && !!mod.type->gunmod->usable.count( "pistol" ) ) {
        msg = string_format( _("Your %s isn't big enough to use that mod.'"), tname().c_str() );

    } else if ( !mod.type->gunmod->acceptable_ammo.empty() && !mod.type->gunmod->acceptable_ammo.count( ammo_type( false ) ) ) {
        msg = string_format( _( "That %1$s cannot be used on a %2$s." ), mod.tname( 1 ).c_str(), ammo_name( ammo_type( false ) ).c_str() );

    } else if( mod.typeId() == "waterproof_gunmod" && has_flag( "WATERPROOF_GUN" ) ) {
        msg = string_format( _( "Your %s is already waterproof." ), tname().c_str() );

    } else if( mod.typeId() == "tuned_mechanism" && has_flag( "NEVER_JAMS" ) ) {
        msg = string_format( _( "This %s is eminently reliable. You can't improve upon it this way." ), tname().c_str() );

    } else if( mod.typeId() == "brass_catcher" && has_flag( "RELOAD_EJECT" ) ) {
        msg = string_format( _( "You cannot attach a brass catcher to your %s." ), tname().c_str() );

    } else {
        return true;
    }

    if( alert ) {
        add_msg( m_info, msg.c_str() );
    }
    return false;
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

item::reload_option::reload_option( const player *who, const item *target, const item *parent, item_location&& ammo ) :
    who( who ), target( target ), ammo( std::move( ammo ) ), parent( parent )
{
    if( this->target->is_ammo_belt() && this->target->type->magazine->linkage != "NULL" ) {
        max_qty = this->who->charges_of( this->target->type->magazine->linkage );
    }

    // magazine, ammo or ammo container
    item& tmp = this->ammo->is_ammo_container() ? this->ammo->contents.front() : *this->ammo;

    long amt = tmp.is_ammo() ? tmp.charges : 1;

    if( this->target->is_gun() && this->target->magazine_integral() && tmp.made_of( SOLID ) ) {
        amt = 1; // guns with integral magazines reload one round at a time
    }

    qty( amt );
}


int item::reload_option::moves() const
{
    int mv = ammo.obtain_cost( *who, qty() ) + who->item_reload_cost( *target, *ammo, qty() );
    if( parent != target ) {
        if( parent->is_gun() ) {
            mv += parent->type->gun->reload_time;
        } else if( parent->is_tool() ) {
            mv += 100;
        }
    }
    return mv;
}

void item::reload_option::qty( long val )
{
    if( ammo->is_ammo() ) {
        qty_ = std::min( { val, ammo->charges, target->ammo_capacity() - target->ammo_remaining() } );

    } else if( ammo->is_ammo_container() ) {
        qty_ = std::min( { val, ammo->contents[ 0 ].charges, target->ammo_capacity() - target->ammo_remaining() } );

    } else {
        qty_ = 1L; // when reloading target using a magazine
    }
    qty_ = std::max( std::min( qty_, max_qty ), 1L );
}

// TODO: Constify the player &u
item::reload_option item::pick_reload_ammo( player &u, bool prompt ) const
{
    std::vector<reload_option> ammo_list;

    auto opts = gunmods();
    opts.push_back( this );

    if( magazine_current() ) {
        opts.push_back( magazine_current() );
    }

    for( const auto e : opts ) {
        for( item_location& ammo : u.find_ammo( *e ) ) {
            auto id = ammo->is_ammo_container() ? ammo->contents[0].typeId() : ammo->typeId();
            if( u.can_reload( *e, id ) || e->has_flag( "RELOAD_AND_SHOOT" ) ) {
                ammo_list.emplace_back( &u, e, this, std::move( ammo ) );
            }
        }
    }

    if( ammo_list.empty() ) {
        if( !is_magazine() && !magazine_integral() && !magazine_current() ) {
            u.add_msg_if_player( m_info, _( "You need a compatible magazine to reload the %s!" ), tname().c_str() );

        } else {
            auto name = ammo_data() ? ammo_data()->nname( 1 ) : ammo_name( ammo_type() );
            u.add_msg_if_player( m_info, _( "Out of %s!" ), name.c_str() );
        }
        return reload_option();
    }

    // sort in order of move cost (ascending), then remaining ammo (descending) with empty magazines always last
    std::stable_sort( ammo_list.begin(), ammo_list.end(), []( const reload_option& lhs, const reload_option& rhs ) {
        return lhs.ammo->ammo_remaining() > rhs.ammo->ammo_remaining();
    } );
    std::stable_sort( ammo_list.begin(), ammo_list.end(), []( const reload_option& lhs, const reload_option& rhs ) {
        return lhs.moves() < rhs.moves();
    } );
    std::stable_sort( ammo_list.begin(), ammo_list.end(), []( const reload_option& lhs, const reload_option& rhs ) {
        return ( lhs.ammo->ammo_remaining() != 0 ) > ( rhs.ammo->ammo_remaining() != 0 );
    } );

    if( u.is_npc() ) {
        return std::move( ammo_list[ 0 ] );
    }

    if( !prompt && ammo_list.size() == 1 ) {
        // Suppress display of reload prompt when...
        if( !is_gun() ) {
            return std::move( ammo_list[ 0 ]); // reloading tools

        } else if( magazine_integral() && ammo_remaining() > 0 ) {
            return std::move( ammo_list[ 0 ] ); // adding to partially filled integral magazines

        } else if( has_flag( "RELOAD_AND_SHOOT" ) && u.has_item( *ammo_list[ 0 ].ammo ) ) {
            return std::move( ammo_list[ 0 ] ); // using bows etc and ammo is already in player possession
        }
    }

    uimenu menu;
    menu.text = string_format( _("Reload %s" ), tname().c_str() );
    menu.return_invalid = true;
    menu.w_width = -1;
    menu.w_height = -1;

    // Construct item names
    std::vector<std::string> names;
    std::transform( ammo_list.begin(), ammo_list.end(), std::back_inserter( names ), [&u]( const reload_option& e ) {
        if( e.ammo->is_magazine() && e.ammo->ammo_data() ) {
            //~ magazine with ammo (count)
            return string_format( _( "%s with %s (%d)" ), e.ammo->type_name().c_str(),
                                  e.ammo->ammo_data()->nname( e.ammo->ammo_remaining() ).c_str(), e.ammo->ammo_remaining() );

        } else if( e.ammo->is_ammo_container() && u.is_worn( *e.ammo ) ) {
            // worn ammo containers should be named by their contents with their location also updated below
            return e.ammo->contents[ 0 ].display_name();

        } else {
            return e.ammo->display_name();
        }
    } );

    // Get location descriptions
    std::vector<std::string> where;
    std::transform( ammo_list.begin(), ammo_list.end(), std::back_inserter( where ), [&u]( const reload_option& e ) {
        if( e.ammo->is_ammo_container() && u.is_worn( *e.ammo ) ) {
            return e.ammo->type_name();
        }
        return e.ammo.describe( &g->u );
    } );

    // Pads elements to match longest member and return length
    auto pad = []( std::vector<std::string>& vec, int n, int t ) -> int {
        for( const auto& e : vec ) {
            n = std::max( n, utf8_width( e, true ) + t );
        };
        for( auto& e : vec ) {
            e += std::string( n - utf8_width( e, true ), ' ' );
        }
        return n;
    };

    // Pad the first column including 4 trailing spaces
    int w = pad( names, utf8_width( menu.text, true ), 6 );
    menu.text.insert( 0, 2, ' ' ); // add space for UI hotkeys
    menu.text += std::string( w + 2 - utf8_width( menu.text, true ), ' ' );

    // Pad the location similarly (excludes leading "| " and trailing " ")
    w = pad( where, utf8_width( _( "| Location " ) ) - 3, 6 );
    menu.text += _("| Location " );
    menu.text += std::string( w + 3 - utf8_width( _( "| Location " ) ), ' ' );

    menu.text += _( "| Amount  " );
    menu.text += _( "| Moves   " );

    // We only show ammo statistics for guns and magazines
    if( is_gun() || is_magazine() ) {
        menu.text += _( "| Damage  | Pierce  " );
    }

    auto draw_row = [&]( int idx ) {
        const auto& sel = ammo_list[ idx ];
        std::string row = string_format( "%s| %s |", names[ idx ].c_str(), where[ idx ].c_str() );
        row += string_format( ( sel.ammo->is_ammo() || sel.ammo->is_ammo_container() ) ? " %-7d |" : "         |", sel.qty() );
        row += string_format( " %-7d ", sel.moves() );

        if( is_gun() || is_magazine() ) {
            const itype *ammo = sel.ammo->is_ammo_container() ? sel.ammo->contents[ 0 ].ammo_data() : sel.ammo->ammo_data();
            if( ammo ) {
                row += string_format( "| %-7d | %-7d", ammo->ammo->damage, ammo->ammo->pierce );
            } else {
                row += "|         |         ";
            }
        }
        return row;
    };

    struct : public uimenu_callback {
        std::function<std::string( int )> draw_row;

        bool key( int ch, int idx, uimenu * menu ) {
            auto& sel = static_cast<std::vector<reload_option> *>( myptr )->operator[]( idx );
            switch( ch ) {
                case KEY_LEFT:
                    sel.qty( sel.qty() - 1 );
                    menu->entries[ idx ].txt = draw_row( idx );
                    return true;

                case KEY_RIGHT:
                    sel.qty( sel.qty() + 1 );
                    menu->entries[ idx ].txt = draw_row( idx );
                    return true;
            }
            return false;
        }
    } cb;
    cb.setptr( &ammo_list );
    cb.draw_row = draw_row;
    menu.callback = &cb;

    itype_id last = uistate.lastreload[ ammo_type() ];

    for( auto i = 0; i != (int) ammo_list.size(); ++i ) {
        const item& ammo = ammo_list[ i ].ammo->is_ammo_container() ? ammo_list[ i ].ammo->contents[ 0 ] : *ammo_list[ i ].ammo;

        char hotkey = -1;
        if( u.has_item( ammo ) ) {
            // if ammo in player possession and either it or any container has a valid invlet use this
            if( ammo.invlet ) {
                hotkey = ammo.invlet;
            } else {
                for( const auto obj : u.parents( ammo ) ) {
                    if( obj->invlet ) {
                        hotkey = obj->invlet;
                        break;
                    }
                }
            }
        }
        if( hotkey == -1 && last == ammo.typeId() ) {
            // if this is the first occurrence of the most recently used type of ammo and the hotkey
            // was not already set above then set it to the keypress that opened this prompt
            hotkey = inp_mngr.get_previously_pressed_key();
            last = std::string();
        }

        menu.addentry( i, true, hotkey, draw_row( i ) );
    }

    menu.query();
    if( menu.ret < 0 || menu.ret >= ( int ) ammo_list.size() ) {
        u.add_msg_if_player( m_info, _( "Never mind." ) );
        return reload_option();
    }

    const item_location& sel = ammo_list[ menu.ret ].ammo;
    uistate.lastreload[ ammo_type() ] = sel->is_ammo_container() ? sel->contents[ 0 ].typeId() : sel->typeId();
    return std::move( ammo_list[ menu.ret ] );
}

// Helper to handle ejecting casings from guns that require them to be manually extracted.
static void eject_casings( player &p, item& target )
{
    int qty = target.get_var( "CASINGS", 0 );
    if( !target.has_flag( "RELOAD_EJECT" ) || target.ammo_casing() == "null" || qty <= 0 ) {
        return;
    }

    g->m.add_item_or_charges( p.pos(), item( target.ammo_casing(), calendar::turn, qty ) );
    target.erase_var( "CASINGS" );
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
    if ( ammo->is_ammo_container() ) {
        container = ammo;
        ammo = &ammo->contents[0];
    }

    // Chance to fail pulling an arrow at lower levels
    if( container && container->type->can_use( "QUIVER" ) ) {
        int archery = u.get_skill_level( skill_id( "archery" ) );
        ///\EFFECT_ARCHERY increases reliability of pulling arrows from a quiver
        if( archery <= 2 && one_in( 10 ) ) {
            u.moves -= 30;
            u.add_msg_if_player( _( "You try to pull a %1$s from your %2$s, but fail!" ),
                                ammo->tname().c_str(), container->type_name().c_str() );
            return false;
        }
        u.add_msg_if_player( _( "You pull a %1$s from your %2$s and nock it." ),
                             ammo->tname().c_str(), container->type_name().c_str() );
    }

    item *obj = this; // what are we trying to reload?

    // for holsters and ammo pouches try to reload any contained item
    if( type->can_use( "holster" ) && !contents.empty() ) {
        // @todo add moves penalty
        obj = &contents[ 0 ];
    }

    if( !obj->is_reloadable() ) {
        return false;
    }

    // Firstly try reloading active gunmod, then item itself, any other auxiliary gunmods and finally any currently loaded magazine
    std::vector<item *> opts = { obj->gunmod_current(), obj };
    auto mods = obj->gunmods();
    std::copy_if( mods.begin(), mods.end(), std::back_inserter( opts ), []( item *e ) {
        return e->is_auxiliary_gunmod();
    });
    opts.push_back( obj->magazine_current() );

    auto target = std::find_if( opts.begin(), opts.end(), [&u,&ammo]( item *e ) {
        return e && u.can_reload( *e, ammo->typeId() );
    } );
    if( target == opts.end() ) {
        return false;
    }

    obj = *target;
    qty = std::min( qty, obj->ammo_capacity() - obj->ammo_remaining() );

    eject_casings( u, *obj );

    if( obj->is_magazine() ) {
        qty = std::min( qty, ammo->charges );

        if( obj->is_ammo_belt() && obj->type->magazine->linkage != "NULL" ) {
            if( !u.use_charges_if_avail( obj->type->magazine->linkage, qty ) ) {
                debugmsg( "insufficient linkages available when reloading ammo belt" );
            }
        }

        obj->contents.emplace_back( *ammo );
        obj->contents.back().charges = qty;
        ammo->charges -= qty;

    } else if ( !obj->magazine_integral() ) {
        // if we already have a magazine loaded prompt to eject it
        if( obj->magazine_current() ) {
            std::string prompt = string_format( _( "Eject %s from %s?" ), ammo->tname().c_str(), obj->tname().c_str() );
            if( !u.dispose_item( *obj->magazine_current(), prompt ) ) {
                return false;
            }
        }

        obj->contents.emplace_back( *ammo );
        loc.remove_item();
        return true;

    } else {
        obj->set_curammo( *ammo );

        if( ammo_type() == "plutonium" ) {
            // always consume at least one cell but never more than actually available
            auto cells = std::min( qty / PLUTONIUM_CHARGES + ( qty % PLUTONIUM_CHARGES != 0 ), ammo->charges );
            ammo->charges -= cells;
            // any excess is wasted rather than overfilling the obj
            obj->charges += std::min( cells * PLUTONIUM_CHARGES, qty );
        } else {
            qty = std::min( qty, ammo->charges );
            ammo->charges   -= qty;
            obj->charges += qty;
        }
    }

    if( ammo->charges == 0 ) {
        if( container != nullptr ) {
            container->contents.erase(container->contents.begin());
            u.inv.restack(&u); // emptied containers do not stack with non-empty ones
        } else {
            loc.remove_item();
        }
    }
    return true;
}

bool item::burn(int amount)
{
    if( amount < 0 ) {
        return false;
    }

    if( is_corpse() ) {
        const mtype *mt = get_mtype();
        if( active && mt != nullptr && burnt + amount > mt->hp &&
            !mt->burn_into.is_null() && mt->burn_into.is_valid() ) {
            corpse = &get_mtype()->burn_into.obj();
            // Delay rezing
            bday = calendar::turn;
            burnt = 0;
            return false;
        }

        if( burnt + amount > mt->hp ) {
            active = false;
        }
    }

    if( !count_by_charges() ) {
        burnt += amount;
        return burnt >= volume() * 3;
    }

    amount *= rng( type->stack_size / 2, type->stack_size );
    if( charges <= amount ) {
        return true;
    }

    charges -= amount;
    return false;
}

bool item::flammable() const
{
    const auto mats = made_of_types();
    if( mats.empty() ) {
        // Don't know how to burn down something made of nothing.
        return false;
    }
    int flammability = 0;
    for( auto mat : mats ) {
        flammability += mat->fire_resist();
    }

    if( flammability == 0 ) {
        return true;
    }

    if( made_of( material_id( "nomex" ) ) ) {
        return false;
    }

    if( made_of( material_id( "paper" ) ) || made_of( material_id( "powder" ) ) || made_of( material_id( "plastic" ) ) ) {
        return true;
    }

    int vol = volume();
    if( ( made_of( material_id( "wood" ) ) || made_of( material_id( "veggy" ) ) ) && ( burnt < 1 || vol <= 10 ) ) {
        return true;
    }

    if( ( made_of( material_id( "cotton" ) ) || made_of( material_id( "wool" ) ) ) && ( burnt / ( vol + 1 ) <= 1 ) ) {
        return true;
    }

    return false;
}

std::ostream & operator<<(std::ostream & out, const item * it)
{
    out << "item(";
    if(!it)
    {
        out << "NULL)";
        return out;
    }
    out << it->tname() << ")";
    return out;
}

std::ostream & operator<<(std::ostream & out, const item & it)
{
    out << (&it);
    return out;
}


itype_id item::typeId() const
{
    if (!type) {
        return "null";
    }
    return type->id;
}

bool item::getlight(float & luminance, int & width, int & direction ) const
{
    luminance = 0;
    width = 0;
    direction = 0;
    if ( light.luminance > 0 ) {
        luminance = (float)light.luminance;
        if ( light.width > 0 ) { // width > 0 is a light arc
            width = light.width;
            direction = light.direction;
        }
        return true;
    } else {
        const int lumint = getlight_emit();
        if ( lumint > 0 ) {
            luminance = (float)lumint;
            return true;
        }
    }
    return false;
}

int item::getlight_emit() const
{
    float lumint = type->light_emission;

    if ( lumint == 0 ) {
        return 0;
    }
    if ( has_flag("CHARGEDIM") && is_tool() && !has_flag("USE_UPS")) {
        // Falloff starts at 1/5 total charge and scales linearly from there to 0.
        if( ammo_capacity() && ammo_remaining() < ( ammo_capacity() / 5 ) ) {
            lumint *= ammo_remaining() * 5.0 / ammo_capacity();
        }
    }
    return lumint;
}

long item::get_remaining_capacity_for_liquid( const item &liquid, bool allow_bucket ) const
{
    if ( has_valid_capacity_for_liquid( liquid, allow_bucket ) != L_ERR_NONE) {
        return 0;
    }

    if (liquid.is_ammo() && (is_tool() || is_gun())) {
        // for filling up chainsaws, jackhammers and flamethrowers
        return ammo_capacity() - ammo_remaining();
    }

    const auto total_capacity = liquid.liquid_charges( type->container->contains );

    long remaining_capacity = total_capacity;
    if (!contents.empty()) {
        remaining_capacity -= contents[0].charges;
    }
    return remaining_capacity;
}

item::LIQUID_FILL_ERROR item::has_valid_capacity_for_liquid( const item &liquid, bool allow_bucket ) const
{
    if (liquid.is_ammo() && (is_tool() || is_gun())) {
        // for filling up chainsaws, jackhammers and flamethrowers
        if( ammo_type() != liquid.ammo_type() ) {
            return L_ERR_NOT_CONTAINER;
        }

        if( ammo_remaining() >= ammo_capacity() ) {
            return L_ERR_FULL;
        }

        if( ammo_remaining() && ammo_current() != liquid.typeId() ) {
            return L_ERR_NO_MIX;
        }
    }

    if( !is_container() ) {
        return L_ERR_NOT_CONTAINER;
    }

    if( !contents.empty() && contents[0].type->id != liquid.type->id ) {
        return L_ERR_NO_MIX;
    }

    if( !type->container->watertight ) {
        return L_ERR_NOT_WATERTIGHT;
    }

    if( !type->container->seals && ( !allow_bucket || !is_bucket() ) ) {
        return L_ERR_NOT_SEALED;
    }

    if (!contents.empty()) {
        const auto total_capacity = liquid.liquid_charges( type->container->contains);
        if( (total_capacity - contents[0].charges) <= 0) {
            return L_ERR_FULL;
        }
    }
    return L_ERR_NONE;
}

bool item::use_amount(const itype_id &it, long &quantity, std::list<item> &used)
{
    // First, check contents
    for( auto a = contents.begin(); a != contents.end() && quantity > 0; ) {
        if (a->use_amount(it, quantity, used)) {
            a = contents.erase(a);
        } else {
            ++a;
        }
    }
    // Now check the item itself
    if (type->id == it && quantity > 0 && contents.empty()) {
        used.push_back(*this);
        quantity--;
        return true;
    } else {
        return false;
    }
}

bool item::fill_with( item &liquid, std::string &err, bool allow_bucket )
{
    LIQUID_FILL_ERROR lferr = has_valid_capacity_for_liquid( liquid, allow_bucket );
    switch ( lferr ) {
        case L_ERR_NONE :
            break;
        case L_ERR_NO_MIX:
            err = string_format( _( "You can't mix loads in your %s." ), tname().c_str() );
            return false;
        case L_ERR_NOT_CONTAINER:
            err = string_format( _( "That %1$s won't hold %2$s." ), tname().c_str(), liquid.tname().c_str());
            return false;
        case L_ERR_NOT_WATERTIGHT:
            err = string_format( _( "That %s isn't water-tight." ), tname().c_str());
            return false;
        case L_ERR_NOT_SEALED:
            err = is_bucket() ?
                  string_format( _( "That %s must be on the ground or held to hold contents!" ), tname().c_str()) :
                  string_format( _( "You can't seal that %s!" ), tname().c_str());
            return false;
        case L_ERR_FULL:
            err = string_format( _( "Your %1$s can't hold any more %2$s." ), tname().c_str(), liquid.tname().c_str());
            return false;
        default:
            err = string_format( _( "Unimplemented liquid fill error '%s'." ),lferr);
            return false;
    }

    const long remaining_capacity = get_remaining_capacity_for_liquid( liquid, allow_bucket );
    const long amount = std::min( remaining_capacity, liquid.charges );

    if( !is_container_empty() ) {
        contents[0].charges += amount;
    } else {
        item liquid_copy = liquid;
        liquid_copy.charges = amount;
        put_in( liquid_copy );
    }
    liquid.charges -= amount;

    return true;
}

bool item::use_charges( const itype_id& what, long& qty, std::list<item>& used, const tripoint& pos )
{
    std::vector<item *> del;

    visit_items( [&what, &qty, &used, &pos, &del] ( item *e ) {
        if( qty == 0 ) {
             // found sufficient charges
            return VisitResponse::ABORT;
        }

        if( e->is_tool() ) {
            // for tools we also need to check if this item is a subtype of the required id
            if( e->typeId() == what || e->type->tool->subtype == what ) {
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
            destroy = true; // cannot remove ourself...
        } else {
            remove_item( *e );
        }
    }
    return destroy;
}

void item::set_snippet( const std::string &snippet_id )
{
    if( is_null() ) {
        return;
    }
    if( type->snippet_category.empty() ) {
        debugmsg("can not set description for item %s without snippet category", type->id.c_str() );
        return;
    }
    const int hash = SNIPPET.get_snippet_by_id( snippet_id );
    if( SNIPPET.get( hash ).empty() ) {
        debugmsg("snippet id %s is not contained in snippet category %s", snippet_id.c_str(), type->snippet_category.c_str() );
        return;
    }
    note = hash;
}

const item_category &item::get_category() const
{
    if(is_container() && !contents.empty()) {
        return contents[0].get_category();
    }
    if(type != 0) {
        if(type->category == 0) {
            // Category not set? Set it now.
            itype *t = const_cast<itype *>(type);
            t->category = item_controller->get_category(item_controller->calc_category(t));
        }
        return *type->category;
    }
    // null-item -> null-category
    static item_category null_category;
    return null_category;
}

bool item_matches_locator(const item &it, const itype_id &id, int)
{
    return it.typeId() == id;
}

bool item_matches_locator(const item &, int locator_pos, int item_pos)
{
    return item_pos == locator_pos;
}

bool item_matches_locator(const item &it, const item *other, int)
{
    return &it == other;
}

iteminfo::iteminfo(std::string Type, std::string Name, std::string Fmt,
                   double Value, bool _is_int, std::string Plus,
                   bool NewLine, bool LowerIsBetter, bool DrawName)
{
    sType = Type;
    sName = replace_colors(Name);
    sFmt = replace_colors(Fmt);
    is_int = _is_int;
    dValue = Value;
    std::stringstream convert;
    if (_is_int) {
        int dIn0i = int(Value);
        convert << dIn0i;
    } else {
        convert.precision(2);
        convert << std::fixed << Value;
    }
    sValue = convert.str();
    sPlus = Plus;
    bNewLine = NewLine;
    bLowerIsBetter = LowerIsBetter;
    bDrawName = DrawName;
}

void item::detonate( const tripoint &p ) const
{
    if( type == nullptr || type->explosion.power < 0 ) {
        return;
    }

    g->explosion( p, type->explosion );
}

bool item_ptr_compare_by_charges( const item *left, const item *right)
{
    if(left->contents.empty()) {
        return false;
    } else if( right->contents.empty()) {
        return true;
    } else {
        return right->contents[0].charges < left->contents[0].charges;
    }
}

bool item_compare_by_charges( const item& left, const item& right)
{
    return item_ptr_compare_by_charges( &left, &right);
}

//return value is number of arrows/bolts quivered
int item::quiver_store_arrow( item &arrow)
{
    if( arrow.charges <= 0 ) {
        return 0;
    }

    //item is valid quiver to store items in if it satisfies these conditions:
    // a) is a quiver
    // b) has some arrow already, but same type is ok
    // c) quiver isn't full

    if( !type->can_use( "QUIVER")) {
        return 0;
    }

    if( !contents.empty() && contents[0].type->id != arrow.type->id) {
        return 0;
    }

    long max_arrows = (long)max_charges_from_flag( "QUIVER");
    if( !contents.empty() && contents[0].charges >= max_arrows) {
        return 0;
    }

    // check ends, now store.
    if( contents.empty()) {
        item quivered_arrow( arrow);
        quivered_arrow.charges = std::min( max_arrows, arrow.charges);
        put_in( quivered_arrow);
        arrow.charges -= quivered_arrow.charges;
        return quivered_arrow.charges;
    } else {
        int quivered = std::min( max_arrows - contents[0].charges, arrow.charges);
        contents[0].charges += quivered;
        arrow.charges -= quivered;
        return quivered;
    }
}

//used to implement charges for items that aren't tools (e.g. quivers)
//flagName arg is the flag's name before the underscore and integer on the end
//e.g. for "QUIVER_20" flag, flagName = "QUIVER"
int item::max_charges_from_flag(std::string flagName)
{
    item* it = this;
    int maxCharges = 0;

    //loop through item's flags, looking for flag that matches flagName
    for( auto flag : it->type->item_tags ) {

        if(flag.substr(0, flagName.size()) == flagName ) {
            //get the substring of the flag starting w/ digit after underscore
            std::stringstream ss(flag.substr(flagName.size() + 1, flag.size()));

            //attempt to store that stringstream into maxCharges and error if there's a problem
            if(!(ss >> maxCharges)) {
                debugmsg("Error parsing %s_n tag (item::max_charges_from_flag)"), flagName.c_str();
                maxCharges = -1;
            }
            break;
        }
    }

    return maxCharges;
}

static const std::string USED_BY_IDS( "USED_BY_IDS" );
bool item::already_used_by_player(const player &p) const
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

void item::mark_as_used_by_player(const player &p)
{
    std::string &used_by_ids = item_vars[ USED_BY_IDS ];
    if( used_by_ids.empty() ) {
        // *always* start with a ';'
        used_by_ids = ";";
    }
    // and always end with a ';'
    used_by_ids += string_format( "%d;", p.getID() );
}

bool item::can_holster ( const item& obj, bool ignore ) const {
    if( !type->can_use("holster") ) {
        return false; // item is not a holster
    }

    auto ptr = dynamic_cast<const holster_actor *>(type->get_use("holster")->get_actor_ptr());
    if( !ptr->can_holster(obj) ) {
        return false; // item is not a suitable holster for obj
    }

    if( !ignore && (int) contents.size() >= ptr->multi ) {
        return false; // item is already full
    }

    return true;
}

void item::unset_curammo()
{
    curammo = nullptr;
}

void item::set_curammo( const itype_id &type )
{
    if( type == "null" ) {
        unset_curammo();
        return;
    }
    const auto at = item_controller->find_template( type );
    if( !at->ammo ) {
        // Much code expects curammo to be a valid ammo, or null, make sure this assumption
        // is correct
        debugmsg( "Tried to set non-ammo type %s as curammo of %s", type.c_str(), tname().c_str() );
        return;
    }
    curammo = at;
}

void item::set_curammo( const item &ammo )
{
    if( ammo.is_null() ) {
        unset_curammo();
        return;
    }
    const auto at = ammo.type;
    if( !at->ammo ) {
        debugmsg( "Tried to set non-ammo type %s as curammo of %s", ammo.type->id.c_str(),
                  tname().c_str() );
        return;
    }
    curammo = at;
}

std::string item::components_to_string() const
{
    typedef std::map<std::string, int> t_count_map;
    t_count_map counts;
    for( const auto &elem : components ) {
        const std::string name = elem.display_name();
        counts[name]++;
    }
    std::ostringstream buffer;
    for(t_count_map::const_iterator a = counts.begin(); a != counts.end(); ++a) {
        if (a != counts.begin()) {
            buffer << _(", ");
        }
        if (a->second != 1) {
            buffer << string_format(_("%d x %s"), a->second, a->first.c_str());
        } else {
            buffer << a->first;
        }
    }
    return buffer.str();
}

bool item::needs_processing() const
{
    return active || has_flag("RADIO_ACTIVATION") ||
           ( is_container() && !contents.empty() && contents[0].needs_processing() ) ||
           is_artifact();
}

int item::processing_speed() const
{
    if( is_food() && !( item_tags.count("HOT") || item_tags.count("COLD") ) ) {
        // Hot and cold food need turn-by-turn updates.
        // If they ever become a performance problem, update process_food to handle them occasionally.
        return 600;
    }
    if( is_corpse() ) {
        return 100;
    }
    // Unless otherwise indicated, update every turn.
    return 1;
}

bool item::process_food( player * /*carrier*/, const tripoint &pos )
{
    calc_rot( g->m.getabs( pos ) );
    if( item_tags.count( "HOT" ) > 0 ) {
        item_counter--;
        if( item_counter == 0 ) {
            item_tags.erase( "HOT" );
        }
    } else if( item_tags.count( "COLD" ) > 0 ) {
        item_counter--;
        if( item_counter == 0 ) {
            item_tags.erase( "COLD" );
        }
    }
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
        g->process_artifact( this, carrier );
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
    if( rng( 0, volume() ) > burnt && g->revive_corpse( pos, *this ) ) {
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
                carrier->add_msg_if_player( m_warning, _( "Oh dear god, a robot you're carrying has started moving!" ) );
            } else {
                carrier->add_msg_if_player( m_warning, _( "Oh dear god, a corpse you're carrying has started moving!" ) );
            }
        }
        // Destroy this corpse item
        return true;
    }

    return false;
}

bool item::process_litcig( player *carrier, const tripoint &pos )
{
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
            int duration = 10;
            if( carrier->has_trait( "TOLERANCE" ) ) {
                duration = 5;
            } else if( carrier->has_trait( "LIGHTWEIGHT" ) ) {
                duration = 20;
            }
            carrier->add_msg_if_player( m_neutral, _( "You take a puff of your %s." ), tname().c_str() );
            if( has_flag( "TOBACCO" ) ) {
                carrier->add_effect( effect_cig, duration );
            } else {
                carrier->add_effect( effect_weed_high, duration / 2 );
            }
            g->m.add_field( tripoint( pos.x + rng( -1, 1 ), pos.y + rng( -1, 1 ), pos.z ), smoke_type, 2, 0 );
            carrier->moves -= 15;
        }

        if( ( carrier->has_effect( effect_shakes ) && one_in( 10 ) ) ||
            ( carrier->has_trait( "JITTERY" ) && one_in( 200 ) ) ) {
            carrier->add_msg_if_player( m_bad, _( "Your shaking hand causes you to drop your %s." ),
                                        tname().c_str() );
            g->m.add_item_or_charges( tripoint( pos.x + rng( -1, 1 ), pos.y + rng( -1, 1 ), pos.z ), *this, 2 );
            return true; // removes the item that has just been added to the map
        }
    } else {
        // If not carried by someone, but laying on the ground:
        // release some smoke every five ticks
        if( item_counter % 5 == 0 ) {
            g->m.add_field( tripoint( pos.x + rng( -2, 2 ), pos.y + rng( -2, 2 ), pos.z ), smoke_type, 1, 0 );
            // lit cigarette can start fires
            if( g->m.flammable_items_at( pos ) ||
                g->m.has_flag( "FLAMMABLE", pos ) ||
                g->m.has_flag( "FLAMMABLE_ASH", pos ) ) {
                g->m.add_field( pos, fd_fire, 1, 0 );
            }
        }
    }

    item_counter--;
    // cig dies out
    if( item_counter == 0 ) {
        if( carrier != nullptr ) {
            carrier->add_msg_if_player( m_neutral, _( "You finish your %s." ), tname().c_str() );
        }
        if( type->id == "cig_lit" ) {
            convert( "cig_butt" );
        } else if( type->id == "cigar_lit" ) {
            convert( "cigar_butt" );
        } else { // joint
            convert( "joint_roach" );
            if( carrier != nullptr ) {
                carrier->add_effect( effect_weed_high, 10 ); // one last puff
                g->m.add_field( tripoint( pos.x + rng( -1, 1 ), pos.y + rng( -1, 1 ), pos.z ), fd_weedsmoke, 2, 0 );
                weed_msg( carrier );
            }
        }
        active = false;
    }
    // Item remains
    return false;
}

bool item::process_cable( player *p, const tripoint &pos )
{
    if( get_var( "state" ) != "pay_out_cable" ) {
        return false;
    }

    int source_x = get_var( "source_x", 0 );
    int source_y = get_var( "source_y", 0 );
    int source_z = get_var( "source_z", 0 );
    tripoint source( source_x, source_y, source_z );

    tripoint relpos = g->m.getlocal( source );
    auto veh = g->m.veh_at( relpos );
    if( veh == nullptr || source_z != g->get_levz() ) {
        if( p != nullptr && p->has_item( *this ) ) {
            p->add_msg_if_player(m_bad, _("You notice the cable has come loose!"));
        }
        reset_cable(p);
        return false;
    }

    tripoint abspos = g->m.getabs( pos );

    int distance = rl_dist( abspos, source );
    int max_charges = type->maximum_charges();
    charges = max_charges - distance;

    if( charges < 1 ) {
        if( p != nullptr && p->has_item( *this ) ) {
            p->add_msg_if_player(m_bad, _("The over-extended cable breaks loose!"));
        }
        reset_cable(p);
    }

    return false;
}

void item::reset_cable( player* p )
{
    int max_charges = type->maximum_charges();

    set_var( "state", "attach_first" );
    active = false;
    charges = max_charges;

    if ( p != nullptr ) {
        p->add_msg_if_player(m_info, _("You reel in the cable."));
        p->moves -= charges * 10;
    }
}

bool item::process_wet( player * /*carrier*/, const tripoint & /*pos*/ )
{
    item_counter--;
    if( item_counter == 0 ) {
        if( is_tool() && type->tool->revert_to != "null" ) {
            convert( type->tool->revert_to );
        }
        item_tags.erase( "WET" );
        active = false;
    }
    // Always return true so our caller will bail out instead of processing us as a tool.
    return true;
}

bool item::process_tool( player *carrier, const tripoint &pos )
{
    if( type->tool->turns_per_charge > 0 && int( calendar::turn ) % type->tool->turns_per_charge == 0 ) {
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

            auto revert = type->tool->revert_to; // invoking the object can convert the item to another type
            type->invoke( carrier != nullptr ? carrier : &g->u, this, pos );
            if( revert == "null" ) {
                return true;
            } else {
                deactivate( carrier );
                return false;
            }
        }
    }

    type->tick( carrier != nullptr ? carrier : &g->u, this, pos );
    return false;
}

bool item::process( player *carrier, const tripoint &pos, bool activate )
{
    const bool preserves = type->container && type->container->preserves;
    for( auto it = contents.begin(); it != contents.end(); ) {
        if( preserves ) {
            // Simulate that the item has already "rotten" up to last_rot_check, but as item::rot
            // is not changed, the item is still fresh.
            it->last_rot_check = calendar::turn;
        }
        if( it->process( carrier, pos, activate ) ) {
            it = contents.erase( it );
        } else {
            ++it;
        }
    }
    if( activate ) {
        return type->invoke( carrier != nullptr ? carrier : &g->u, this, pos );
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
    if( is_food() &&  process_food( carrier, pos ) ) {
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
    if( has_flag( "CABLE_SPOOL" ) ) {
        // DO NOT process this as a tool! It really isn't!
        return process_cable(carrier, pos);
    }
    if( is_tool() ) {
        return process_tool( carrier, pos );
    }
    return false;
}

bool item::reduce_charges( long quantity )
{
    if( !count_by_charges() ) {
        debugmsg( "Tried to remove %s by charges, but item is not counted by charges", tname().c_str() );
        return false;
    }
    if( quantity > charges ) {
        debugmsg( "Charges: Tried to remove charges that do not exist, removing maximum available charges instead" );
    }
    if( charges <= quantity ) {
        return true;
    }
    charges -= quantity;
    return false;
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
    return type->seed.get() != nullptr;
}

int item::get_plant_epoch() const
{
    if( !type->seed ) {
        return 0;
    }
    // 91 days is the approximate length of a real world season
    // Growing times have been based around 91 rather than the default of 14 to give
    // more accuracy for longer season lengths
    // Note that it is converted based on the season_length option!
    // Also note that seed->grow is the time it takes from seeding to harvest, this is
    // divied by 3 to get the time it takes from one plant state to the next.
    return DAYS( type->seed->grow * calendar::season_length() / ( 91 * 3 ) );
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
    // Note: doesn't handle the pipebomb or radio bombs
    // Consider flagging dangerous items with an explicit flag instead
    static const std::string explosion_string( "explosion" );
    if( type->can_use( explosion_string ) ) {
        return true;
    }

    return std::any_of( contents.begin(), contents.end(), []( const item &it ) {
        return it.is_dangerous();
    } );
}

bool item::is_tainted() const
{
    return corpse && corpse->has_flag( MF_POISON );
}

bool item::is_soft() const
{
    // @todo Make this a material property
    // @todo Add a SOFT flag (for chainmail and the like)
    static const std::vector<material_id> soft_mats = {{
        material_id( "cotton" ), material_id( "leather" ), material_id( "wool" ), material_id( "nomex" )
    }};

    return made_of_any( soft_mats );
}

bool item::is_reloadable() const
{
    if( !is_gun() && !is_tool() && !is_magazine() ) {
        return false;

    } else if( has_flag( "NO_RELOAD") ) {
        return false;

    } else if( ammo_type() == "NULL" ) {
        return false;
    }

    return true;
}

std::string item::type_name( unsigned int quantity ) const
{
    const auto iter = item_vars.find( "name" );
    if( corpse != nullptr && typeId() == "corpse" ) {
        if( name.empty() ) {
            return rmp_format( ngettext( "<item_name>%s corpse",
                                         "<item_name>%s corpses", quantity ),
                               corpse->nname().c_str() );
        } else {
            return rmp_format( ngettext( "<item_name>%s corpse of %s",
                                         "<item_name>%s corpses of %s", quantity ),
                               corpse->nname().c_str(), name.c_str() );
        }
    } else if( typeId() == "blood" ) {
        if( corpse == nullptr || corpse->id == NULL_ID ) {
            return rm_prefix( ngettext( "<item_name>human blood",
                                        "<item_name>human blood", quantity ) );
        } else {
            return rmp_format( ngettext( "<item_name>%s blood",
                                         "<item_name>%s blood",  quantity ),
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

const itype * item::find_type( const itype_id& type )
{
    return item_controller->find_template( type );
}

int item::get_gun_ups_drain() const
{
    int draincount = 0;
    if( type->gun ){
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
    if ( has_label() ) {
        return get_var( "item_label" );
    }

    return type_name( quantity );
}

item_category::item_category() : id(), name(), sort_rank( 0 )
{
}

item_category::item_category( const std::string &id_, const std::string &name_,
                              int sort_rank_ )
    : id( id_ ), name( name_ ), sort_rank( sort_rank_ )
{
}

bool item_category::operator<( const item_category &rhs ) const
{
    if( sort_rank != rhs.sort_rank ) {
        return sort_rank < rhs.sort_rank;
    }
    if( name != rhs.name ) {
        return name < rhs.name;
    }
    return id < rhs.id;
}

bool item_category::operator==( const item_category &rhs ) const
{
    return sort_rank == rhs.sort_rank && name == rhs.name && id == rhs.id;
}

bool item_category::operator!=( const item_category &rhs ) const
{
    return !( *this == rhs );
}

bool item::is_disgusting_for( const player &p ) const {
    return has_flag( "FILTHY" ) && p.has_trait( "SQUEAMISH" );
}
