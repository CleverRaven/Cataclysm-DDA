#include "item.h"

#include "flag.h"
#include "advanced_inv.h"
#include "player.h"
#include "damage.h"
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
#include "projectile.h"
#include "item_group.h"
#include "options.h"
#include "messages.h"
#include "artifact.h"
#include "itype.h"
#include "iuse_actor.h"
#include "compatibility.h"
#include "translations.h"
#include "crafting.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "martialarts.h"
#include "npc.h"
#include "ui.h"
#include "vehicle.h"
#include "mtype.h"
#include "field.h"
#include "fire.h"
#include "weather.h"
#include "catacharset.h"
#include "cata_utility.h"
#include "input.h"
#include "fault.h"
#include "vehicle_selector.h"

#include <cmath> // floor
#include <sstream>
#include <algorithm>
#include <unordered_set>
#include <set>
#include <array>
#include <tuple>
#include <iterator>

using namespace units::literals;

static const std::string GUN_MODE_VAR_NAME( "item::mode" );

const skill_id skill_survival( "survival" );
const skill_id skill_melee( "melee" );
const skill_id skill_bashing( "bashing" );
const skill_id skill_cutting( "cutting" );
const skill_id skill_stabbing( "stabbing" );
const skill_id skill_unarmed( "unarmed" );

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

const long item::INFINITE_CHARGES = std::numeric_limits<long>::max();

item::item()
{
    type = nullitem();
}

item::item( const itype *type, int turn, long qty ) : type( type )
{
    bday = turn >= 0 ? turn : int( calendar::turn );
    corpse = typeId() == "corpse" ? &mtype_id::NULL_ID.obj() : nullptr;
    name = type_name();
    item_counter = type->countdown_interval;

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
            emplace_back( type->magazine->default_ammo, calendar::turn, type->magazine->count );
        }

    } else if( type->comestible ) {
        active = goes_bad() && !rotten();

    } else if( type->tool ) {
        if( ammo_remaining() && ammo_type() ) {
            ammo_set( default_ammo( ammo_type() ), ammo_remaining() );
        }
    }

    if( ( type->gun || type->tool ) && !magazine_integral() ) {
        set_var( "magazine_converted", true );
    }

    if( !type->snippet_category.empty() ) {
        note = SNIPPET.assign( type->snippet_category );
    }
}

item::item( const itype_id& id, int turn, long qty )
    : item( find_type( id ), turn, qty ) {}

item::item( const itype *type, int turn, default_charges_tag )
    : item( type, turn, type->charges_default() ) {}

item::item( const itype_id& id, int turn, default_charges_tag tag )
    : item( find_type( id ), turn, tag ) {}

item::item( const itype *type, int turn, solitary_tag )
    : item( type, turn, type->count_by_charges() ? 1 : -1 ) {}

item::item( const itype_id& id, int turn, solitary_tag tag )
    : item( find_type( id ), turn, tag ) {}

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

item& item::activate()
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


item& item::ammo_set( const itype_id& ammo, long qty )
{
    if( qty < 0 ) {
        // completely fill an integral or existing magazine
        if( magazine_integral() || magazine_current() ) {
            qty = ammo_capacity();
        } else {
            // if adding a magazine use default ammo count property if set
            item mag( magazine_default() );
            if( mag.type->magazine->count > 0 ) {
                qty = mag.type->magazine->count;
            } else {
                qty = item( magazine_default() ).ammo_capacity();
            }
        }
    }

    if( qty == 0 ) {
        ammo_unset();
        return *this;
    }

    // handle reloadable tools and guns with no specific ammo type as special case
    if( ammo == "null" && !ammo_type() ) {
        if( ( is_tool() || is_gun() ) && magazine_integral() ) {
            curammo = nullptr;
            charges = std::min( qty, ammo_capacity() );
        }
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
                std::sort( opts.begin(), opts.end(), [qty]( const itype_id &lhs, const itype_id &rhs ) {
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

item& item::set_damage( double qty )
{
    damage_ = std::max( std::min( qty, double( max_damage() ) ), double( min_damage() ) );
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
    return (type == nullptr || type == nullitem() || typeId() == s_null);
}

bool item::covers( const body_part bp ) const
{
    if( bp >= num_bp ) {
        debugmsg( "bad body part %d to check in item::covers", static_cast<int>( bp ) );
        return false;
    }
    return get_covered_body_parts().test(bp);
}

std::bitset<num_bp> item::get_covered_body_parts() const
{
    std::bitset<num_bp> res;

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

bool item::is_worn_only_with( const item &it ) const
{
    return is_power_armor() && it.is_power_armor() && it.covers( bp_torso );
}

item item::in_its_container() const
{
    return in_container( type->default_container );
}

item item::in_container( const itype_id &cont ) const
{
    if( cont != "null" ) {
        item ret( cont, bday );
        ret.contents.push_back( *this );
        if( made_of( LIQUID ) && ret.is_container() ) {
            // Note: we can't use any of the normal normal container functions as they check the
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
    // TODO: items should not have 0 volume at all!
    return type->volume != 0 ? vol / type->volume : INFINITE_CHARGES;
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
    if( contents.size() != rhs.contents.size() ) {
        return false;
    }
    return std::equal( contents.begin(), contents.end(), rhs.contents.begin(), []( const item& a, const item& b ) {
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
        item_counter = ( static_cast<double>( item_counter ) * charges + static_cast<double>( rhs.item_counter ) * rhs.charges ) / ( charges + rhs.charges );
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

        const std::string unit = string_format( "<num> %s", _( "ml" ) );
        info.push_back( iteminfo( "BASE", _( "<bold>Volume</bold>: " ), unit, to_milliliter( volume() ),
                                   true, "", false, true ) );

        info.push_back( iteminfo( "BASE", space + _( "Weight: " ),
                                  string_format( "<num> %s", weight_units() ),
                                  convert_weight( weight() ), false, "", true, true ) );

        if( !type->rigid ) {
            info.emplace_back( "BASE", _( "<bold>Rigid</bold>: " ), _( "No (contents increase volume)" ) );
        }

        int dmg_bash = damage_melee( DT_BASH );
        int dmg_cut  = damage_melee( DT_CUT );
        int dmg_stab = damage_melee( DT_STAB );

        if( dmg_bash ) {
            info.emplace_back( "BASE", _( "Bash: " ), "", dmg_bash, true, "", false );
        }
        if( dmg_cut ) {
            info.emplace_back( "BASE", ( dmg_bash ? space : std::string() ) + _( "Cut: " ),
                               "", dmg_cut, true, "", false );
        }
        if( dmg_stab ) {
            info.emplace_back( "BASE", ( ( dmg_bash || dmg_cut ) ? space : std::string() ) + _( "Pierce: " ),
                               "", dmg_stab, true, "", false );
        }

        if( dmg_bash || dmg_cut || dmg_stab ) {
            info.push_back( iteminfo( "BASE", space + _( "To-hit bonus: " ),
                                      ( ( type->m_to_hit > 0 ) ? "+" : "" ),
                                      type->m_to_hit, true, "" ) );
            info.push_back( iteminfo( "BASE", _( "Moves per attack: " ), "",
                                      attack_time(), true, "", true, true ) );

            if( !conductive () ) {
                info.push_back( iteminfo( "BASE", string_format( _( "* This weapon <good>does not conduct</good> electricity." ) ) ) );
            }
            else if( has_flag( "CONDUCTIVE" ) ) {
                info.push_back( iteminfo( "BASE", string_format( _( "* This weapon effectively <bad>conducts</bad> electricity, as it has no guard." ) ) ) );
            }
            else {
                info.push_back( iteminfo( "BASE", string_format( _( "* This weapon <bad>conducts</bad> electricity." ) ) ) );
            }
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
            req.push_back( string_format( "%s %d", skill_id( sk.first )->name().c_str(), sk.second ) );
        }
        if( !req.empty() ) {
            info.emplace_back( "BASE", _("<bold>Minimum requirements:</bold>") );
            info.emplace_back( "BASE", enumerate_as_string( req ) );
            insert_separation_line();
        }

        const std::vector<const material_type*> mat_types = made_of_types();
        if( !mat_types.empty() ) {
            const std::string material_list = enumerate_as_string( mat_types.begin(), mat_types.end(),
            []( const material_type *material ) {
                return string_format( "<stat>%s</stat>", material->name().c_str() );
            }, false );
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

                const item *food = is_food_container() ? &contents.front() : this;
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

        const auto vits = g->u.vitamins_from( *food_item );
        const std::string required_vits = enumerate_as_string( vits.begin(), vits.end(), []( const std::pair<vitamin_id, int> &v ) {
            return ( g->u.vitamin_rate( v.first ) > 0 && v.second != 0 ) // only display vitamins that we actually require
                ? string_format( "%s (%i%%)", v.first.obj().name().c_str(), int( v.second / ( DAYS( 1 ) / float( g->u.vitamin_rate( v.first ) ) ) * 100 ) )
                : std::string();
        } );
        if( !required_vits.empty() ) {
            info.emplace_back( "FOOD", _( "Vitamins (RDA): " ), required_vits.c_str() );
        }

        if( food_item->has_flag( "CANNIBALISM" ) ) {
            if( !g->u.has_trait_flag( "CANNIBAL" ) ) {
                info.emplace_back( "DESCRIPTION", _( "* This food contains <bad>human flesh</bad>." ) );
            } else {
                info.emplace_back( "DESCRIPTION", _( "* This food contains <good>human flesh</good>." ) );
            }
        }

        if( food_item->is_tainted() ) {
            info.emplace_back( "DESCRIPTION", _( "* This food is <bad>tainted</bad> and will poison you." ) );
        }

        ///\EFFECT_SURVIVAL >=3 allows detection of poisonous food
        if( food_item->has_flag( "HIDDEN_POISON" ) && g->u.get_skill_level( skill_survival ).level() >= 3 ) {
            info.emplace_back( "DESCRIPTION", _( "* On closer inspection, this appears to be <bad>poisonous</bad>." ) );
        }

        ///\EFFECT_SURVIVAL >=5 allows detection of hallucinogenic food
        if( food_item->has_flag( "HIDDEN_HALLU" ) && g->u.get_skill_level( skill_survival ).level() >= 5 ) {
            info.emplace_back( "DESCRIPTION", _( "* On closer inspection, this appears to be <neutral>hallucinogenic</neutral>." ) );
        }

        if( food_item->goes_bad() ) {
            const std::string rot_time = calendar( food_item->type->comestible->spoils ).textify_period();
            info.emplace_back( "DESCRIPTION",
                               string_format( _( "* This food is <neutral>perishable</neutral>, and takes <info>%s</info> to rot from full freshness, at room temperature." ),
                                              rot_time.c_str() ) );
            if( food_item->rotten() ) {
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
            }
        }
    }

    if( is_magazine() && !has_flag( "NO_RELOAD" ) ) {
        info.emplace_back( "MAGAZINE", _( "Capacity: " ),
                           string_format( ngettext( "<num> round of %s", "<num> rounds of %s", ammo_capacity() ),
                                          _( ammo_name( ammo_type() ).c_str() ) ), ammo_capacity(), true );

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
        const item *mod = this;
        const auto aux = gun_current_mode();
        // if we have an active auxiliary gunmod display stats for this instead
        if( aux && aux->is_gunmod() && aux->is_gun() ) {
            mod = &*aux;
            info.emplace_back( "DESCRIPTION", string_format( _( "Stats of the active <info>gunmod (%s)</info> are shown." ),
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

        islot_gun *gun = mod->type->gun.get();
        const auto curammo = mod->ammo_data();

        bool has_ammo = curammo && mod->ammo_remaining();

        int ammo_dam        = has_ammo ? curammo->ammo->damage     : 0;
        int ammo_range      = has_ammo ? curammo->ammo->range      : 0;
        int ammo_pierce     = has_ammo ? curammo->ammo->pierce     : 0;
        int ammo_dispersion = has_ammo ? curammo->ammo->dispersion : 0;

        const auto skill = &mod->gun_skill().obj();

        info.push_back( iteminfo( "GUN", _( "Skill used: " ), "<info>" + skill->name() + "</info>" ) );

        if( mod->magazine_integral() ) {
            if( mod->ammo_capacity() ) {
                info.emplace_back( "GUN", _( "<bold>Capacity:</bold> " ),
                                   string_format( ngettext( "<num> round of %s", "<num> rounds of %s", mod->ammo_capacity() ),
                                                  _( ammo_name( mod->ammo_type() ).c_str() ) ), mod->ammo_capacity(), true );
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

        int eff_range = round( g->u.gun_engagement_range( *mod, player::engagement::effective ) );
        int max_range = round( g->u.gun_engagement_range( *mod, player::engagement::maximum ) );

        if( eff_range > 0 ) {
            info.emplace_back( "GUN", _( "<bold>Effective range: </bold>" ), "<num>", eff_range, true, "", false );
            info.emplace_back( "GUN", space + _( "Maximum range: " ), "<num>", max_range );
        }

        int aim_mv = g->u.gun_engagement_moves( *mod );
        if( aim_mv > 0 ) {
            info.emplace_back( "GUN", _( "Maximum aiming time: " ), "<num> seconds", int( aim_mv / 16.67 ), true, "", true, true );
        }

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

        info.push_back( iteminfo( "GUN", _( "Dispersion: " ), "",
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

        // if effective sight dispersion differs from actual sight dispersion display both
        int act_disp = mod->sight_dispersion();
        int eff_disp = g->u.effective_dispersion( act_disp );
        int adj_disp = eff_disp - act_disp;

        if( adj_disp < 0 ) {
            info.emplace_back( "GUN", _( "Sight dispersion: " ),
                               string_format( "%i-%i = <num>", act_disp, -adj_disp), eff_disp, true, "", true, true );
        } else if( adj_disp > 0 ) {
            info.emplace_back( "GUN", _( "Sight dispersion: " ),
                               string_format( "%i+%i = <num>", act_disp, adj_disp), eff_disp, true, "", true, true );
        } else {
            info.emplace_back( "GUN", _( "Sight dispersion: " ), "", eff_disp, true, "", true, true );
        }

        bool bipod = mod->has_flag( "BIPOD" );
        if( aprox ) {
            if( aprox->gun_recoil( g->u ) ) {
                info.emplace_back( "GUN", _( "Approximate recoil: " ), "",
                                   aprox->gun_recoil( g->u ), true, "", !bipod, true );
                if( bipod ) {
                    info.emplace_back( "GUN", "bipod_recoil", _( " (with bipod <num>)" ),
                                       aprox->gun_recoil( g->u, true ), true, "", true, true, false );
                }
            }
        } else {
            if( mod->gun_recoil( g->u ) ) {
                info.emplace_back( "GUN", _( "Effective recoil: " ), "",
                                   mod->gun_recoil( g->u ), true, "", !bipod, true );
                if( bipod ) {
                    info.emplace_back( "GUN", "bipod_recoil", _( " (with bipod <num>)" ),
                                       mod->gun_recoil( g->u, true ), true, "", true, true, false );
                }
            }
        }

        auto fire_modes = mod->gun_all_modes();
        if( std::any_of( fire_modes.begin(), fire_modes.end(),
            []( const std::pair<std::string, gun_mode>& e ) {
                return e.second.qty > 1 && !e.second.melee();
        } ) ) {
            info.emplace_back( "GUN", _( "Reccomended strength (burst): "), "",
                               ceil( mod->type->weight / 333.0 ), true, "", true, true );
        }

        info.emplace_back( "GUN", _( "Reload time: " ),
                           has_flag( "RELOAD_ONE" ) ? _( "<num> seconds per round" ) : _( "<num> seconds" ),
                           int( gun->reload_time / 16.67 ), true, "", true, true );

        std::vector<std::string> fm;
        for( const auto &e : fire_modes ) {
            if( e.second.target == this && !e.second.melee() ) {
                fm.emplace_back( string_format( "%s (%i)", _( e.second.mode.c_str() ), e.second.qty ) );
            }
        }
        if( !fm.empty() ) {
            insert_separation_line();
            info.emplace_back( "GUN", _( "<bold>Fire modes:</bold> " ) + enumerate_as_string( fm ) );
        }

        if( !magazine_integral() ) {
            insert_separation_line();
            const auto compat = magazine_compatible();
            info.emplace_back( "DESCRIPTION", _( "<bold>Compatible magazines:</bold> " ) +
                enumerate_as_string( compat.begin(), compat.end(), []( const itype_id &id ) {
                    return item_controller->find_template( id )->nname( 1 );
            } ) );
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

        if( mod->casings_count() ) {
            insert_separation_line();
            std::string tmp = ngettext( "Contains <stat>%i</stat> casing",
                                        "Contains <stat>%i</stat> casings", mod->casings_count() );
            info.emplace_back( "DESCRIPTION", string_format( tmp, mod->casings_count() ) );
        }

    }
    if( is_gunmod() ) {
        const auto mod = type->gunmod.get();

        if( is_gun() ) {
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
        if( mod->aim_cost > 0 ) {
            info.push_back( iteminfo( "GUNMOD", _( "Aim cost: " ), "",
                                      mod->aim_cost, true, "", true, true ) );
        }
        if( mod->damage != 0 ) {
            info.push_back( iteminfo( "GUNMOD", _( "Damage: " ), "", mod->damage, true,
                                      ( ( mod->damage > 0 ) ? "+" : "" ) ) );
        }
        if( mod->pierce != 0 ) {
            info.push_back( iteminfo( "GUNMOD", _( "Armor-pierce: " ), "", mod->pierce, true,
                                      ( ( mod->pierce > 0 ) ? "+" : "" ) ) );
        }
        if( mod->handling != 0 ) {
            info.emplace_back( "GUNMOD", _( "Handling modifier: " ), mod->handling > 0 ? "+" : "", mod->handling, true );
        }
        if( mod->ammo_modifier ) {
            info.push_back( iteminfo( "GUNMOD",
                                      string_format( _( "Ammo: <stat>%s</stat>" ), ammo_name( mod->ammo_modifier ).c_str() ) ) );
        }

        temp1.str( "" );
        temp1 << _( "Used on: " ) << enumerate_as_string( mod->usable.begin(), mod->usable.end(), []( const std::string &used_on ) {
            //~ a weapon type which a gunmod is compatible (eg. "pistol", "crossbow", "rifle")
            return string_format( "<info>%s</info>", _( used_on.c_str() ) );
        } );

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

        const std::string unit = string_format( "<num> %s", _( "ml" ) );
        info.push_back( iteminfo( "ARMOR", space + _( "Storage: " ), unit, to_milliliter( get_storage() ) ) );

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
        if( !book->skill && !type->can_use( "MA_MANUAL" )) {
            info.push_back( iteminfo( "BOOK", _( "Just for fun." ) ) );
        }
        if( type->can_use( "MA_MANUAL" )) {
            info.push_back( iteminfo( "BOOK", _( "Some sort of <info>martial arts training manual</info>." ) ) );
        }
        if( book->req == 0 ) {
            info.push_back( iteminfo( "BOOK", _( "It can be <info>understood by beginners</info>." ) ) );
        }
        if( g->u.has_identified( typeId() ) ) {
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
                    const std::string name = nname( elem.recipe->result );
                    recipe_list.push_back( "<bold>" + name + "</bold>" );
                } else {
                    recipe_list.push_back( "<dark>" + elem.name + "</dark>" );
                }
            }
            if( !recipe_list.empty() ) {
                std::string recipe_line = string_format(
                                              ngettext( "This book contains %1$d crafting recipe: %2$s",
                                                        "This book contains %1$d crafting recipes: %2$s", recipe_list.size() ),
                                              recipe_list.size(), enumerate_as_string( recipe_list ).c_str() );

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

        temp1 << string_format( _( "can store <info>%.2f liters</info>." ), c.contains / 1000.0_ml );

        info.push_back( iteminfo( "CONTAINER", temp1.str() ) );
    }

    if( is_tool() ) {
        if( ammo_capacity() != 0 ) {
            info.emplace_back( "TOOL", string_format( _( "<bold>Charges</bold>: %d" ), ammo_remaining() ) );
        }

        if( !magazine_integral() ) {
            insert_separation_line();
            const auto compat = magazine_compatible();
            info.emplace_back( "TOOL", _( "<bold>Compatible magazines:</bold> " ),
                enumerate_as_string( compat.begin(), compat.end(), []( const itype_id &id ) {
                    return item_controller->find_template( id )->nname( 1 );
                } ) );
        } else if( ammo_capacity() != 0 ) {
            std::string tmp;
            if( ammo_type() ) {
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
        const auto &dis = recipe_dictionary::get_uncraft( typeId() );
        const auto &req = dis.disassembly_requirements();
        if( !req.is_empty() ) {
            const auto components = req.get_components();
            const std::string components_list = enumerate_as_string( components.begin(), components.end(),
            []( const std::vector<item_comp> &comps ) {
                return comps.front().to_string();
            } );

            insert_separation_line();
            info.push_back( iteminfo( "DESCRIPTION",
                string_format( _( "Disassembling this item takes %s and might yield: %s." ),
                               calendar::print_duration( dis.time / 100 ).c_str(), components_list.c_str() ) ) );
        }
    }

    auto name_quality = [&info]( const std::pair<quality_id,int>& q ) {
        std::string str;
        if( q.first == quality_jack || q.first == quality_lift ) {
            str = string_format( _( "Has level <info>%1$d %2$s</info> quality and is rated at <info>%3$dkg</info>" ),
                                 q.second, q.first.obj().name.c_str(), q.second * TOOL_LIFT_FACTOR / 1000 );
        } else {
            str = string_format( _( "Has level <info>%1$d %2$s</info> quality." ),
                                 q.second, q.first.obj().name.c_str() );
        }
        info.emplace_back( "QUALITIES", "", str );
    };

    for( const auto& q : type->qualities ) {
        name_quality( q );
    }

    if( std::any_of( contents.begin(), contents.end(), []( const item& e ) { return !e.type->qualities.empty(); } ) ) {
        info.emplace_back( "QUALITIES", "", _( "Contains items with qualities:" ) );
    }
    for( const auto& e : contents ) {
        for( const auto& q : e.type->qualities ) {
            name_quality( q );
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
        auto all_techniques = type->techniques;
        all_techniques.insert( techniques.begin(), techniques.end() );
        if( !all_techniques.empty() ) {
            insert_separation_line();
            info.push_back( iteminfo( "DESCRIPTION", _( "Techniques: " ) +
            enumerate_as_string( all_techniques.begin(), all_techniques.end(), []( const matec_id &tid ) {
                return string_format( "<stat>%s</stat>", tid.obj().name.c_str() );
            } ) ) );
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
        const auto &styles = g->u.ma_styles;
        const std::string valid_styles = enumerate_as_string( styles.begin(), styles.end(),
        [ this ]( const matype_id &mid ) {
            return mid.obj().has_weapon( typeId() ) ? mid.obj().name : std::string();
        } );
        if( !valid_styles.empty() ) {
            insert_separation_line();
            info.push_back( iteminfo( "DESCRIPTION",
                                      std::string( _( "You know how to use this with these martial arts styles: " ) ) +
                                      valid_styles ) );
        }

        for( const auto &method : type->use_methods ) {
            insert_separation_line();
            method.second.dump_info( *this, info );
        }

        insert_separation_line();

        // concatenate base and acquired flags...
        std::vector<std::string> flags;
        std::set_union( type->item_tags.begin(), type->item_tags.end(),
                        item_tags.begin(), item_tags.end(),
                        std::back_inserter( flags ) );

        // ...and display those which have an info description
        for( const auto &e : flags ) {
            auto &f = json_flag::get( e );
            if( !f.info().empty() ) {
                info.emplace_back( "DESCRIPTION", string_format( "* %s", f.info().c_str() ) );
            }
        }

        if( is_armor() ) {
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
            if( is_filthy() ) {
                info.push_back( iteminfo( "DESCRIPTION",
                                          _( "* This piece of clothing is <bad>filthy</bad>." ) ) );
            }
            if( is_power_armor() ) {
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
            if( typeId() == "rad_badge" ) {
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
        }

        if( has_flag( "RADIO_ACTIVATION" ) ) {
            if( has_flag( "RADIO_MOD" ) ) {
                info.emplace_back( "DESCRIPTION", _( "* This item has been modified to listen to <info>radio signals</info>.  It can still be activated manually." ) );
            } else {
                info.emplace_back( "DESCRIPTION", _( "* This item can only be activated by a <info>radio signal</info>." ) );
            }

            std::string signame;
            if( has_flag( "RADIOSIGNAL_1" ) ) {
                signame = "<color_c_red>red</color> radio signal.";
            } else if( has_flag( "RADIOSIGNAL_2" ) ) {
                signame = "<color_c_blue>blue</color> radio signal.";
            } else if( has_flag( "RADIOSIGNAL_3" ) ) {
                signame = "<color_c_green>green</color> radio signal.";
            }

            info.emplace_back( "DESCRIPTION", string_format( _( "* It will be activated by the %s radio signal." ), signame.c_str() ) );

            if( has_flag( "RADIO_INVOKE_PROC" ) ) {
                info.emplace_back( "DESCRIPTION",_( "* Activating this item with a <info>radio signal</info> will <neutral>detonate</neutral> it immediately." ) );
            }
        }

        if( is_bionic() ) {
            info.push_back( iteminfo( "DESCRIPTION", list_occupied_bps( typeId(),
                _( "This bionic is installed in the following body part(s):" ) ) ) );
        }

        if( is_gun() && has_flag( "FIRE_TWOHAND" ) ) {
            info.push_back( iteminfo( "DESCRIPTION",
                                      _( "* This weapon needs <info>two free hands</info> to fire." ) ) );
        }

        if( is_gunmod() && has_flag( "DISABLE_SIGHTS" ) ) {
            info.push_back( iteminfo( "DESCRIPTION",
                                      _( "* This mod <bad>obscures sights</bad> of the base weapon." ) ) );
        }

        if( has_flag( "BELT_CLIP" ) ) {
            info.push_back( iteminfo( "DESCRIPTION",
                                      _( "* This item can be <neutral>clipped or hooked</neutral> on to a <info>belt loop</info> of the appropriate size." ) ) );
        }

        if( has_flag( "LEAK_DAM" ) && has_flag( "RADIOACTIVE" ) && damage() > 0 ) {
            info.push_back( iteminfo( "DESCRIPTION",
                                      _( "* The casing of this item has <neutral>cracked</neutral>, revealing an <info>ominous green glow</info>." ) ) );
        }

        if( has_flag( "LEAK_ALWAYS" ) && has_flag( "RADIOACTIVE" ) ) {
            info.push_back( iteminfo( "DESCRIPTION",
                                      _( "* This object is <neutral>surrounded</neutral> by a <info>sickly green glow</info>." ) ) );
        }

        if( is_brewable() || ( !contents.empty() && contents.front().is_brewable() ) ) {
            const item &brewed = !is_brewable() ? contents.front() : *this;
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
                                                         nname( res, brewed.charges ).c_str() ) ) );
            }
        }

        for( const auto &e : faults ) {
            //~ %1$s is the name of a fault and %2$s is the description of the fault
            info.emplace_back( "DESCRIPTION", string_format( _( "* <bad>Faulty %1$s</bad>.  %2$s" ),
                               e.obj().name().c_str(), e.obj().description().c_str() ) );
        }

        ///\EFFECT_MELEE >2 allows seeing melee damage stats on weapons
        if( debug_mode || ( g->u.get_skill_level( skill_melee ) > 2 && ( damage_melee( DT_BASH ) > 0 ||
                            damage_melee( DT_CUT ) > 0 || damage_melee( DT_STAB ) > 0 || type->m_to_hit > 0 ) ) ) {
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
            const auto tt = dynamic_cast<const delayed_transform_iuse *>( u.second.get_actor_ptr() );
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
                        temp1 << _( "Integrated mod: " );
                    } else {
                        temp1 << _( "Mod: " );
                    }
                    temp1 << "<bold>" << mod->tname() << "</bold> (" << _( mod->type->gunmod->location.c_str() ) << ")";
                    insert_separation_line();
                    info.push_back( iteminfo( "DESCRIPTION", temp1.str() ) );
                    info.push_back( iteminfo( "DESCRIPTION", mod->type->description ) );
                }
            } else {
                info.emplace_back( "DESCRIPTION", contents.front().type->description );
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
        if( !known_recipes.empty() ) {
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
                [ &inv ]( const recipe *r ) {
                    if( r->requirements().can_make_with_inventory( inv ) ) {
                        return nname( r->result );
                    } else {
                        return string_format( "<dark>%s</dark>", nname( r->result ).c_str() );
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

const std::string &item::symbol() const
{
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
    } else if( is_filthy() ) {
        ret = c_brown;
    } else if ( has_flag("LEAK_DAM") && has_flag("RADIOACTIVE") && damage() > 0 ) {
        ret = c_ltgreen;
    } else if (active && !is_food() && !is_food_container()) { // Active items show up as yellow
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
        if(u->has_identified( typeId() )) {
            auto &tmp = *type->book;
            if( tmp.skill && // Book can improve skill: blue
                u->get_skill_level( tmp.skill ).can_train() &&
                u->get_skill_level( tmp.skill ) >= tmp.req &&
                u->get_skill_level( tmp.skill ) < tmp.level ) {
                ret = c_ltblue;
            } else if( tmp.skill && // Book can't improve skill right now, but maybe later: pink
                       u->get_skill_level( tmp.skill ).can_train() &&
                       u->get_skill_level( tmp.skill ) < tmp.level ) {
                ret = c_pink;
            } else if( !u->studied_all_recipes( *type ) ) { // Book can't improve skill anymore, but has more recipes: yellow
                ret = c_yellow;
            }
        } else {
            ret = c_red; // Book hasn't been identified yet: red
        }
    } else if (is_bionic()) {
        if (!u->has_bionic(typeId())) {
            ret = u->bionic_installation_issues( typeId() ).empty() ? c_green : c_red;
        }
    }
    return ret;
}

void item::on_wear( Character &p )
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

void item::on_takeoff( Character &p )
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
        } else if( is_melee() ) {
            d /= std::max( (float)p.get_skill_level( melee_skill() ), 1.0f );
        }

        int penalty = get_var( "volume", type->volume / units::legacy_volume_factor ) * d;
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

void item::on_damage( double, damage_type )
{

}

std::string item::tname( unsigned int quantity, bool with_prefix ) const
{
    std::stringstream ret;

// MATERIALS-TODO: put this in json
    std::string damtext = "";

    if( ( damage() != 0 || ( get_option<bool>( "ITEM_HEALTH_BAR" ) && is_armor() ) ) && !is_null() && with_prefix ) {
        if( damage() < 0 )  {
            if( get_option<bool>( "ITEM_HEALTH_BAR" ) ) {
                damtext = "<color_" + string_from_color( damage_color() ) + ">" + damage_symbol() + " </color>";

            } else if (is_gun())  {
                damtext = pgettext( "damage adjective", "accurized " );
            } else {
                damtext = pgettext( "damage adjective", "reinforced " );
            }
        } else {
            if (typeId() == "corpse") {
                if (damage() == 1) damtext = pgettext( "damage adjective", "bruised " );
                if (damage() == 2) damtext = pgettext( "damage adjective", "damaged " );
                if (damage() == 3) damtext = pgettext( "damage adjective", "mangled " );
                if (damage() >= 4) damtext = pgettext( "damage adjective", "pulped " );

            } else if ( get_option<bool>( "ITEM_HEALTH_BAR" ) ) {
                damtext = "<color_" + string_from_color( damage_color() ) + ">" + damage_symbol() + " </color>";

            } else {
                damtext = string_format( "%s ", get_base_material().dmg_adj( damage() ).c_str() );
            }
        }
    }

    if( !faults.empty() ) {
        damtext.insert( 0, _( "faulty " ) );
    }

    std::string vehtext = "";
    if( is_engine() && engine_displacement() > 0 ) {
        vehtext = string_format( pgettext( "vehicle adjective", "%2.1fL " ), engine_displacement() / 100.0f );

    } else if( is_wheel() && type->wheel->diameter > 0 ) {
        vehtext = string_format( pgettext( "vehicle adjective", "%d\" " ), type->wheel->diameter );
    }

    std::string burntext = "";
    if (with_prefix && !made_of(LIQUID)) {
        if( volume() >= 1000_ml && burnt * 125_ml >= volume() ) {
            burntext = pgettext( "burnt adjective", "badly burnt " );
        } else if (burnt > 0) {
            burntext = pgettext( "burnt adjective", "burnt " );
        }
    }

    const std::map<std::string, std::string>::const_iterator iname = item_vars.find("name");
    std::string maintext = "";
    if (corpse != NULL && typeId() == "corpse" ) {
        if (name != "") {
            maintext = string_format( npgettext( "item name", "%s corpse of %s",
                                           "%s corpses of %s",
                                           quantity), corpse->nname().c_str(), name.c_str());
        } else {
            maintext = string_format( npgettext( "item name", "%s corpse",
                                           "%s corpses",
                                           quantity), corpse->nname().c_str());
        }
    } else if (typeId() == "blood") {
        if (corpse == NULL || corpse->id == NULL_ID )
            maintext = string_format( npgettext( "item name", "human blood",
                                          "human blood",
                                          quantity));
        else
            maintext = string_format( npgettext( "item name", "%s blood",
                                           "%s blood",
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
        if( contents.front().made_of( LIQUID ) ) {
            maintext = string_format( pgettext( "item name", "%s of %s" ), label(quantity).c_str(), contents.front().tname( quantity, with_prefix ).c_str());
        } else if( contents.front().is_food() ) {
            maintext = contents.front().charges > 1 ? string_format( pgettext( "item name", "%s of %s" ), label(quantity).c_str(),
                                                            contents.front().tname(contents.front().charges, with_prefix).c_str()) :
                                                 string_format( pgettext( "item name", "%s of %s" ), label(quantity).c_str(),
                                                            contents.front().tname( quantity, with_prefix ).c_str());
        } else {
            maintext = string_format( pgettext( "item name", "%s with %s" ), label(quantity).c_str(), contents.front().tname( quantity, with_prefix ).c_str());
        }
    }
    else if (!contents.empty()) {
        maintext = string_format( pgettext( "item name", "%s, full" ), label(quantity).c_str());
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

    if( is_filthy() ) {
        ret << _(" (filthy)" );
    }

    if (is_tool() && has_flag("USE_UPS")){
        ret << _(" (UPS)");
    }
    if( has_flag( "RADIO_MOD" ) ) {
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

    if( has_flag( "DIAMOND" ) ) {
        modtext += std::string( _( "diamond" ) ) + " ";
    }

    if(has_flag("WET"))
       ret << _(" (wet)");

    if(has_flag("LITCIG"))
        ret << _(" (lit)");

    if( already_used_by_player( g->u ) ) {
        ret << _( " (used)" );
    }

    if( active && !is_food() && !is_corpse() && ( typeId().length() < 3 || typeId().compare( typeId().length() - 3, 3, "_on" ) != 0 ) ) {
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

    if( is_container() && contents.size() == 1 && contents.front().charges > 0 ) {
        // a container which is not empty
        qty = string_format(" (%i)", contents.front().charges);
    } else if( is_book() && get_chapters() > 0 ) {
        // a book which has remaining unread chapters
        qty = string_format(" (%i)", get_remaining_chapters(g->u));
    } else if( ammo_capacity() > 0 ) {
        // anything that can be reloaded including tools, magazines, guns and auxiliary gunmods
        qty = string_format(" (%i)", ammo_remaining());
    } else if( is_ammo_container() && !contents.empty() ) {
        qty = string_format( " (%i)", contents.front().charges );
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
        if( e->damage() > 0 ) {
            // maximal damage is 4, maximal reduction is 40% of the value.
            child -= child * static_cast<double>( e->damage() ) / 10;
        }

        if( e->count_by_charges() || e->made_of( LIQUID ) ) {
            // price from json data is for default-sized stack
            child *= e->charges / static_cast<double>( e->type->stack_size );

        } else if( e->magazine_integral() && e->ammo_remaining() && e->ammo_data() ) {
            // items with integral magazines may contain ammunition which can affect the price
            child += item( e->ammo_data(), calendar::turn, e->charges ).price( practical );

        } else if( e->is_tool() && !e->ammo_type() && e->ammo_capacity() ) {
            // if tool has no ammo (eg. spray can) reduce price proportional to remaining charges
            child *= e->ammo_remaining() / double( std::max( e->type->charges_default(), 1 ) );
        }

        res += child;
        return VisitResponse::NEXT;
    } );

    return res;
}

// MATERIALS-TODO: add a density field to materials.json
int item::weight( bool include_contents ) const
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
        if ( ammo_type() == ammotype( "plutonium" ) ) {
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
        const units::volume b = type->gun->barrel_length;
        const int max_barrel_weight = to_milliliter( b );
        const int barrel_weight = b * type->weight / type->volume;
        ret -= std::min( max_barrel_weight, barrel_weight );
    }

    // tool mods also add about a pound of weight
    if( has_flag("ATOMIC_AMMO") ) {
        ret += 250;
    }

    if( include_contents ) {
        for( auto &elem : contents ) {
            ret += elem.weight();
        }
    }

    return ret;
}

static units::volume corpse_volume( m_size corpse_size )
{
    switch( corpse_size ) {
        case MS_TINY:    return    750_ml;
        case MS_SMALL:   return  30000_ml;
        case MS_MEDIUM:  return  62500_ml;
        case MS_LARGE:   return  92500_ml;
        case MS_HUGE:    return 875000_ml;
    }
    debugmsg( "unknown monster size for corpse" );
    return 0;
}

units::volume item::base_volume() const
{
    if( is_null() ) {
        return 0;
    }

    if( is_corpse() ) {
        return corpse_volume( corpse->size );
    }

    return type->volume;
}

units::volume item::volume( bool integral ) const
{
    if( is_null() ) {
        return 0;
    }

    if( is_corpse() ) {
        return corpse_volume( corpse->size );
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
        ret *= charges;
    }

    // Non-rigid items add the volume of the content
    if( !type->rigid ) {
        for( auto &elem : contents ) {
            ret += elem.volume();
        }
    }

    // Some magazines sit (partly) flush with the item so add less extra volume
    if( magazine_current() != nullptr ) {
        ret += std::max( magazine_current()->volume() - type->magazine_well, units::volume( 0 ) );
    }

    if (is_gun()) {
        for( const auto elem : gunmods() ) {
            ret += elem->volume( true );
        }

        // @todo implement stock_length property for guns
        if (has_flag("COLLAPSIBLE_STOCK")) {
            // consider only the base size of the gun (without mods)
            int tmpvol = get_var( "volume", ( type->volume - type->gun->barrel_length ) / units::legacy_volume_factor );
            if     ( tmpvol <=  3 ) ; // intentional NOP
            else if( tmpvol <=  5 ) ret -=  500_ml;
            else if( tmpvol <=  6 ) ret -=  750_ml;
            else if( tmpvol <=  8 ) ret -= 1000_ml;
            else if( tmpvol <= 11 ) ret -= 1250_ml;
            else if( tmpvol <= 16 ) ret -= 1500_ml;
            else                    ret -= 1750_ml;
        }

        if( gunmod_find( "barrel_small" ) ) {
            ret -= type->gun->barrel_length;
        }
    }

    // Battery mods also add volume
    if( has_flag("ATOMIC_AMMO") ) {
        ret += 250_ml;
    }

    if( has_flag("DOUBLE_AMMO") ) {
        // Batteries have volume 1 per 100 charges
        // TODO: De-hardcode this
        ret += type->maximum_charges() * 2.50_ml;
    }

    return ret;
}

int item::lift_strength() const
{
    return weight() / STR_LIFT_FACTOR + ( weight() % STR_LIFT_FACTOR != 0 );
}

int item::attack_time() const
{
    int ret = 65 + volume() / 62.5_ml + weight() / 60;
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
    res -= res * damage() * 0.1;

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
        for( const auto e : gunmods() ) {
            // gunmods fired separately from the base gun do not contribute to base gun flags
            if( !e->is_gun() && e->has_flag( f ) ) {
                return true;
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
            add_msg( m_debug, "r: %s %d,%d %d->%d", typeId().c_str(), since, until, old, rot );
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

units::volume item::get_storage() const
{
    auto t = find_armor_data();
    if( t == nullptr )
        return 0;

    return t->storage;
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
        return is_gun() ? volume() / 750_ml : 0;
    }
    // it_armor::encumber is signed char
    int encumber = static_cast<int>( t->encumber );

    // Non-rigid items add additional encumbrance proportional to their volume
    if( !type->rigid ) {
        for( const auto &e : contents ) {
            encumber += e.volume() / 250_ml;
        }
    }

    // Fit checked before changes, fitting shouldn't reduce penalties from patching.
    if( item_tags.count("FIT") ) {
        encumber = std::max( encumber / 2, encumber - 10 );
    }

    const int thickness = get_thickness();
    const int coverage = get_coverage();
    if( item_tags.count("wooled") ) {
        encumber += 1 + 3 * coverage / 100;
    }
    if( item_tags.count("furred") ){
        encumber += 1 + 4 * coverage / 100;
    }

    if( item_tags.count("leather_padded") ) {
        encumber += thickness * coverage / 100 + 5;
    }
    if( item_tags.count("kevlar_padded") ) {
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

    if( item_tags.count("furred") > 0 ) {
        fur_lined = 35 * get_coverage() / 100;
    }

    if( item_tags.count("wooled") > 0 ) {
        wool_lined = 20 * get_coverage() / 100;
    }

    return result + fur_lined + wool_lined;
}


int item::brewing_time() const
{
    return ( is_brewable() ? type->brewable->time : 0 ) * ( calendar::season_length() / 14.0 );
}

const std::vector<itype_id> &item::brewing_results() const
{
    static const std::vector<itype_id> nulresult{};
    return is_brewable() ? type->brewable->results : nulresult;
}

bool item::can_revive() const
{
    if( is_corpse() && corpse->has_flag( MF_REVIVES ) && damage() < max_damage() ) {
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
    age_in_hours -= int((float)burnt / ( volume() / 250_ml ) );
    if( damage() > 0 ) {
        age_in_hours /= ( damage() + 1 );
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
        const int eff_damage = to_self ? std::min( damage(), 0 ) : std::max( damage(), 0 );
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
        const int eff_damage = to_self ? std::min( damage(), 0 ) : std::max( damage(), 0 );
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

#ifdef _MSC_VER
#pragma optimize( "", on )
#endif

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
    if( to_self ) {
        // Fire damages items in a different way
        return INT_MAX;
    }

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

int item::damage() const {
    return damage_ < 0 ? floor( damage_ ) : ceil( damage_ );
}

int item::min_damage() const
{
    return count_by_charges() ? 0 : -1;
}

int item::max_damage() const
{
    return count_by_charges() ? 0 : 4;
}

bool item::mod_damage( double qty, damage_type dt )
{
    bool destroy = false;

    if( count_by_charges() ) {
        charges -= std::min( type->stack_size * qty, double( charges ) );
        destroy |= charges == 0;
    }

    if( qty > 0 ) {
        on_damage( qty, dt );
    }

    if( !count_by_charges() ) {
        destroy |= damage_ + qty > max_damage();

        damage_ = std::max( std::min( damage_ + qty, double( max_damage() ) ), double( min_damage() ) );
    }

    return destroy;
}

nc_color item::damage_color() const
{
    // @todo unify with getDurabilityColor

    // reinforced, undamaged and nearly destroyed items are special case
    if( damage() < 0 ) {
        return c_green;
    }
    if( damage() == 0 ) {
        return c_ltgreen;
    }
    if( damage() == max_damage() ) {
        return c_red;
    }

    // assign other colors proportionally
    auto q = double( damage() ) / max_damage();
    if( q > 0.66 ) {
        return c_ltred;
    }
    if( q > 0.33 ) {
        return c_magenta;
    }
    return c_yellow;
}

std::string item::damage_symbol() const
{
    // reinforced, undamaged and nearly destroyed items are special case
    if( damage() < 0 ) {
        return _( R"(++)" );
    }
    if( damage() == 0 ) {
        return _( R"(||)" );
    }
    if( damage() == max_damage() ) {
        return _( R"(..)" );
    }

    // assign other colors proportionally
    auto q = double( damage() ) / max_damage();
    if( q > 0.66 ) {
        return _( R"(\.)" );
    }
    if( q > 0.33 ) {
        return _( R"(|.)" );
    }
    return _( R"(|\)" );
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

bool item::made_of_any( const std::set<material_id> &mat_idents ) const
{
    const auto mats = made_of();
    if( mats.empty() ) {
        return false;
    }

    return std::any_of( mats.begin(), mats.end(), [&mat_idents]( const material_id &e ) {
        return mat_idents.count( e );
    } );
}

bool item::only_made_of( const std::set<material_id> &mat_idents ) const
{
    const auto mats = made_of();
    if( mats.empty() ) {
        return false;
    }

    return std::all_of( mats.begin(), mats.end(), [&mat_idents]( const material_id &e ) {
        return mat_idents.count( e );
    } );
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
    return std::any_of( mats.begin(), mats.end(), []( const material_type *mt ) {
        return mt->elec_resist() <= 1;
    } );
}

bool item::destroyed_at_zero_charges() const
{
    return (is_ammo() || is_food());
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

bool item::is_bandolier() const
{
    return type->can_use( "bandolier" );
}

bool item::is_ammo() const
{
    return type->ammo.get() != nullptr;
}

bool item::is_food(player const*u) const
{
    if( !u ) {
        return is_food();
    }

    if( is_null() ) {
        return false;
    }

    if( type->comestible ) {
        return true;
    }

    if( u->has_active_bionic( "bio_batteries" ) && is_ammo() && ammo_type() == ammotype( "battery" ) ) {
        return true;
    }

    if( ( u->has_active_bionic( "bio_reactor" ) || u->has_active_bionic( "bio_advreactor" ) ) && is_ammo() && ( ammo_type() == ammotype( "reactor_slurry" ) || ammo_type() == ammotype( "plutonium" ) ) ) {
        return true;
    }
    if (u->has_active_bionic("bio_furnace") && flammable() && typeId() != "corpse")
        return true;
    return false;
}

bool item::is_food_container(player const*u) const
{
    return (contents.size() >= 1 && contents.front().is_food(u));
}

bool item::is_food() const
{
    return type->comestible != nullptr;
}

bool item::is_medication() const
{
    if( type->comestible == nullptr || type->comestible->comesttype != "MED" ) {
        return false;
    }

    return true;
}

bool item::is_medication_container() const
{
    return !contents.empty() && contents.front().is_medication();
}

bool item::is_brewable() const
{
    return type->brewable != nullptr;
}

bool item::is_food_container() const
{
    return (contents.size() >= 1 && contents.front().is_food());
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

bool item::is_non_resealable_container() const
{
    return type->container && !type->container->seals && type->container->unseals_into != "null";
}

bool item::is_bucket() const
{
    // That "preserves" part is a hack:
    // Currently all non-empty cans are effectively sealed at all times
    // Making them buckets would cause weirdness
    return type->container != nullptr &&
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
    return type->engine.get() != nullptr;
}

bool item::is_wheel() const
{
    return type->wheel.get() != nullptr;
}

bool item::is_faulty() const
{
    return is_engine() ? !faults.empty() : false;
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

bool item::is_reloadable_with( const itype_id& ammo ) const
{
    if( !is_reloadable() ) {
        return false;
    } else if( magazine_integral() ) {
        if( !ammo.empty() ) {
            if( ammo_data() ) {
                if( ammo_current() != ammo ) {
                    return false;
                }
            } else {
                auto at = find_type( ammo );
                if( !at->ammo || ammo_type() != at->ammo->type ) {
                    return false;
                }
            }
        }
        return ammo_remaining() < ammo_capacity();
    } else {
        return ammo.empty() ? true : magazine_compatible().count( ammo );
    }
}

bool item::is_salvageable() const
{
    if( is_null() ) {
        return false;
    }
    return !has_flag("NO_SALVAGE");
}

bool item::is_funnel_container(units::volume &bigger_than) const
{
    if ( ! is_watertight_container() ) {
        return false;
    }
    // todo; consider linking funnel to item or -making- it an active item
    if ( get_container_capacity() <= bigger_than ) {
        return false; // skip contents check, performance
    }
    if (
        contents.empty() ||
        contents.front().typeId() == "water" ||
        contents.front().typeId() == "water_acid" ||
        contents.front().typeId() == "water_acid_weak") {
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
    if( !is_container() || is_container_empty() ) {
        return true;
    }

    if( c.is_npc() ) {
        return spill_contents( c.pos() );
    }

    while( !contents.empty() ) {
        on_contents_changed();
        if( contents.front().made_of( LIQUID ) ) {
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
        const item *me = is_container() && !contents.empty() ? &contents.front() : this;
        const item *rhs = other.is_container() && !other.contents.empty() ? &other.contents.front() : &other;

        if (me->typeId() == rhs->typeId()) {
            return me->charges < rhs->charges;
        } else {
            std::string n1 = me->type->nname(1);
            std::string n2 = rhs->type->nname(1);
            return std::lexicographical_compare( n1.begin(), n1.end(),
                                                 n2.begin(), n2.end(), sort_case_insensitive_less() );
        }
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
        if( ammo_type() == ammotype( "bolt" ) || typeId() == "bullet_crossbow" ) {
            return "crossbow";
        } else{
            return "bow";
        }
    }
    return gun_skill().c_str();
}

skill_id item::melee_skill() const
{
    if( !is_melee() ) {
        return NULL_ID;
    }

    if( has_flag( "UNARMED_WEAPON" ) ) {
        return skill_unarmed;
    }

    int hi = 0;
    skill_id res = NULL_ID;

    for( auto idx = DT_NULL + 1; idx != NUM_DT; ++idx ) {
        auto val = damage_melee( static_cast<damage_type>( idx ) );
        auto sk  = skill_by_dt ( static_cast<damage_type>( idx ) );
        if( val > hi && sk ) {
            hi = val;
            res = sk;
        }
    }

    return res;
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
    dispersion_sum += damage() * 60;
    dispersion_sum = std::max(dispersion_sum, 0);
    if( with_ammo && ammo_data() ) {
        dispersion_sum += ammo_data()->ammo->dispersion;
    }
    dispersion_sum = std::max(dispersion_sum, 0);
    return dispersion_sum;
}

int item::sight_dispersion() const
{
    if( !is_gun() ) {
        return 0;
    }

    int res = has_flag( "DISABLE_SIGHTS" ) ? MIN_RECOIL : type->gun->sight_dispersion;

    for( const auto e : gunmods() ) {
        const auto mod = e->type->gunmod.get();
        if( mod->sight_dispersion < 0 || mod->aim_cost <= 0 ) {
            continue; // skip gunmods which don't provide a sight
        }
        res = std::min( res, mod->sight_dispersion );
    }

    return res;
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
    ret -= damage() * 2;
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

int item::gun_recoil( const player &p, bool bipod ) const
{
    if( !is_gun() || ( ammo_required() && !ammo_remaining() ) ) {
        return 0;
    }

    ///\EFFECT_STR improves the handling of heavier weapons
    // we consider only base weight to avoid exploits
    double wt = std::min( type->weight, p.str_cur * 333 ) / 333.0;

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
    return std::min( std::max( 0, ret ), MAX_RANGE );
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

    if( is_magazine() || is_bandolier() ) {
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

    if( is_bandolier() ) {
        return dynamic_cast<const bandolier_actor *>( type->get_use( "bandolier" )->get_actor_ptr() )->capacity;
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

bool item::ammo_sufficient( int qty ) const {
    return ammo_remaining() >= ammo_required() * qty;
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
        return !contents.empty() ? contents.front().ammo_data() : nullptr;
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
        if( has_flag( "ATOMIC_AMMO" ) ) {
            return ammotype( "plutonium" );
        }
        for( const auto mod : gunmods() ) {
            if( mod->type->gunmod->ammo_modifier ) {
                return mod->type->gunmod->ammo_modifier;
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
    }
    return NULL_ID;
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

    } else if( effects && ( mod.type->gunmod->ammo_modifier || !mod.type->gunmod->magazine_adaptor.empty() )
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

std::map<std::string, const item::gun_mode> item::gun_all_modes() const
{
    std::map<std::string, const item::gun_mode> res;

    if( !is_gun() || is_gunmod() ) {
        return res;
    }

    auto opts = gunmods();
    opts.push_back( this );

    for( const auto e : opts ) {
        if( e->is_gun() ) {
            for( auto m : e->type->gun->modes ) {
                // prefix attached gunmods, eg. M203_DEFAULT to avoid index key collisions
                std::string prefix = e->is_gunmod() ? ( std::string( e->typeId() ) += "_" ) : "";
                std::transform( prefix.begin(), prefix.end(), prefix.begin(), (int(*)(int))std::toupper );

                auto qty = std::get<1>( m.second );
                if( m.first == "AUTO" && e == this && has_flag( "RAPIDFIRE" ) ) {
                    qty *= 1.5;
                }

                res.emplace( prefix += m.first, item::gun_mode( std::get<0>( m.second ), const_cast<item *>( e ),
                                                                qty, std::get<2>( m.second ) ) );
            };
        }
        if( e->is_gunmod() ) {
            for( auto m : e->type->gunmod->mode_modifier ) {
                res.emplace( m.first, item::gun_mode { std::get<0>( m.second ), const_cast<item *>( e ),
                                                       std::get<1>( m.second ), std::get<2>( m.second ) } );
            }
        }
    }

    return res;
}

const item::gun_mode item::gun_get_mode( const std::string& mode ) const
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

item::gun_mode item::gun_current_mode()
{
    return gun_get_mode( const_cast<item *>( this )->gun_get_mode_id() );
}

std::string item::gun_get_mode_id() const
{
    if( !is_gun() || is_gunmod() ) {
        return "";
    }
    return get_var( GUN_MODE_VAR_NAME, "DEFAULT" );
}

bool item::gun_set_mode( const std::string& mode )
{
    if( !is_gun() || is_gunmod() || !gun_all_modes().count( mode ) ) {
        return false;
    }
    set_var( GUN_MODE_VAR_NAME, mode );
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

const item::gun_mode item::gun_current_mode() const
{
    return const_cast<item *>( this )->gun_current_mode();
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

int item::units_remaining( const Character& ch, int limit ) const
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
        qty_ = std::min( { val, ammo->contents.front().charges, target->ammo_capacity() - target->ammo_remaining() } );

    } else {
        qty_ = 1L; // when reloading target using a magazine
    }
    qty_ = std::max( std::min( qty_, max_qty ), 1L );
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

void item::casings_handle( const std::function<bool(item &)> &func )
{
    if( !is_gun() ) {
        return;
    }

    for( auto it = contents.begin(); it != contents.end(); ) {
        if( it->is_ammo() && it->ammo_type() != ammo_type() ) {
            if( func( *it ) ) {
                it = contents.erase( it );
                continue;
            }
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
    if ( ammo->is_ammo_container() ) {
        container = ammo;
        ammo = &ammo->contents.front();
    }

    if( !is_reloadable_with( ammo->typeId() ) ) {
        return false;
    }

    qty = std::min( qty, ammo_capacity() - ammo_remaining() );

    casings_handle( [&u]( item &e ) {
        return u.i_add_or_drop( e );
    } );

    if( is_magazine() ) {
        qty = std::min( qty, ammo->charges );

        if( is_ammo_belt() && type->magazine->linkage != "NULL" ) {
            if( !u.use_charges_if_avail( type->magazine->linkage, qty ) ) {
                debugmsg( "insufficient linkages available when reloading ammo belt" );
            }
        }

        contents.emplace_back( *ammo );
        contents.back().charges = qty;
        ammo->charges -= qty;

    } else if ( !magazine_integral() ) {
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
        curammo = find_type( ammo->typeId() );

        if( ammo_type() == ammotype( "plutonium" ) ) {
            // Warning: qty here refers to minimum of plutonium cells and capacity left
            // always consume at least one cell but never more than actually available
            auto cells = std::min( qty / PLUTONIUM_CHARGES + ( qty % PLUTONIUM_CHARGES != 0 ), ammo->charges );
            ammo->charges -= cells;
            // any excess is wasted rather than overfilling the item
            charges += std::min( cells, qty ) * PLUTONIUM_CHARGES;
            // Cap at max, because the above formula doesn't guarantee it
            charges = std::min( charges, ammo_capacity() );
        } else {
            qty = std::min( qty, ammo->charges );
            ammo->charges -= qty;
            charges += qty;
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

bool item::burn( fire_data &frd )
{
    const auto &mats = made_of();
    float smoke_added = 0.0f;
    float time_added = 0.0f;
    float burn_added = 0.0f;
    const int vol = base_volume() / units::legacy_volume_factor;
    for( const auto &m : mats ) {
        const auto &bd = m.obj().burn_data( frd.fire_intensity );
        if( bd.immune ) {
            // Made to protect from fire
            return false;
        }

        if( bd.chance_in_volume == 0 || bd.chance_in_volume >= vol ||
            x_in_y( bd.chance_in_volume, vol ) ) {
            time_added += bd.fuel;
            smoke_added += bd.smoke;
            burn_added += bd.burn;
        }
    }

    // Liquids that don't burn well smother fire well instead
    if( made_of( LIQUID ) && time_added < 200 ) {
        time_added -= rng( 100 * vol, 300 * vol );
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
            bday = calendar::turn;
            burnt = 0;
            return false;
        }

        if( burnt + burn_added > mt->hp ) {
            active = false;
        }
    }

    burnt += roll_remainder( burn_added );

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
    int chance = 0;
    for( const auto &m : mats ) {
        const auto &bd = m->burn_data( 1 );
        if( bd.immune ) {
            // Made to protect from fire
            return false;
        }

        flammability += bd.fuel;
        chance += bd.chance_in_volume;
    }

    if( threshold == 0 || flammability <= 0 ) {
        return flammability > 0;
    }

    chance /= mats.size();
    int vol = base_volume() / units::legacy_volume_factor;
    if( chance > 0 && chance < vol ) {
        flammability = flammability * chance / vol;
    } else {
        // If it burns well, it provides a bonus here
        flammability *= vol;
    }

    return flammability > threshold;
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
    return type ? type->get_id() : "null";
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

units::volume item::get_container_capacity() const
{
    if( !is_container() ) {
        return 0;
    }
    return type->container->contains;
}

long item::get_remaining_capacity_for_liquid( const item &liquid, bool allow_bucket, std::string *err ) const
{
    const auto error = [ &err ]( const std::string &message ) {
        if( err != nullptr ) {
            *err = message;
        }
        return 0;
    };

    long remaining_capacity = 0;

    if( is_reloadable_with( liquid.typeId() ) ) {
        if( ammo_remaining() != 0 && ammo_current() != liquid.typeId() ) {
            return error( string_format( _( "You can't mix loads in your %s." ), tname().c_str() ) );
        }
        remaining_capacity = ammo_capacity() - ammo_remaining();
    } else if( is_container() ) {
        if( !type->container->watertight ) {
            return error( string_format( _( "That %s isn't water-tight." ), tname().c_str() ) );
        } else if( !type->container->seals && ( !allow_bucket || !is_bucket() ) ) {
            return error( string_format( is_bucket() ? _( "That %s must be on the ground or held to hold contents!" )
                                                     : _( "You can't seal that %s!" ), tname().c_str() ) );
        } else if( !contents.empty() && contents.front().typeId() != liquid.typeId() ) {
            return error( string_format( _( "You can't mix loads in your %s." ), tname().c_str() ) );
        }
        remaining_capacity = liquid.charges_per_volume( get_container_capacity() );
        if( !contents.empty() ) {
            remaining_capacity -= contents.front().charges;
        }
    } else {
        return error( string_format( _( "That %1$s won't hold %2$s." ), tname().c_str(), liquid.tname().c_str() ) );
    }

    if( remaining_capacity <= 0 ) {
        return error( string_format( _( "Your %1$s can't hold any more %2$s." ), tname().c_str(),
                                     liquid.tname().c_str() ) );
    }

    return remaining_capacity;
}

long item::get_remaining_capacity_for_liquid( const item &liquid, const Character &p, std::string *err ) const
{
    const bool allow_bucket = this == &p.weapon || !p.has_item( *this );
    long res = get_remaining_capacity_for_liquid( liquid, allow_bucket, err );

    if( res > 0 && !type->rigid && p.inv.has_item( *this ) ) {
        const units::volume volume_to_expand = std::max( p.volume_capacity() - p.volume_carried(), units::volume( 0 ) );

        res = std::min( liquid.charges_per_volume( volume_to_expand ), res );

        if( res == 0 && err != nullptr ) {
            *err = string_format( _( "That %s doesn't have room to expand." ), tname().c_str() );
        }
    }

    return res;
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
    if( typeId() == it && quantity > 0 && allow_crafting_component() ) {
        used.push_back(*this);
        quantity--;
        return true;
    } else {
        return false;
    }
}

bool item::allow_crafting_component() const
{
    // vehicle batteries are implemented as magazines of charge
    if( is_magazine() && ammo_type() == ammotype( "battery" ) ) {
        return true;
    }

    return contents.empty();
}

void item::fill_with( item &liquid, long amount )
{
    amount = std::min( get_remaining_capacity_for_liquid( liquid, true ),
                       std::min( amount, liquid.charges ) );
    if( amount <= 0 ) {
        return;
    }

    if( is_reloadable_with( liquid.typeId() ) ) {
        ammo_set( liquid.typeId(), ammo_remaining() + amount );
    } else if( !is_container() ) {
        debugmsg( "Tried to fill %s which is not a container and can't be reloaded with %s.",
                  tname().c_str(), liquid.tname().c_str() );
    } else if( !is_container_empty() ) {
        contents.front().mod_charges( amount );
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
        debugmsg("can not set description for item %s without snippet category", typeId().c_str() );
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
        return contents.front().get_category();
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

bool item::will_explode_in_fire() const
{
    if( type->explode_in_fire ) {
        return true;
    }

    if( type->ammo != nullptr && ( type->ammo->special_cookoff || type->ammo->cookoff ) ) {
        return true;
    }

    // Most containers do nothing to protect the contents from fire
    if( !is_magazine() || !type->magazine->protects_contents ) {
        return std::any_of( contents.begin(), contents.end(), []( const item &it ) {
            return it.will_explode_in_fire();
        });
    }

    return false;
}

bool item::detonate( const tripoint &p, std::vector<item> &drops )
{
    if( type->explosion.power >= 0 ) {
        g->explosion( p, type->explosion );
        return true;
    } else if( type->ammo != nullptr && ( type->ammo->special_cookoff || type->ammo->cookoff ) ) {
        long charges_remaining = charges;
        const long rounds_exploded = rng( 1, charges_remaining );
        // Yank the exploding item off the map for the duration of the explosion
        // so it doesn't blow itself up.
        item temp_item = *this;
        const islot_ammo *ammo_type = type->ammo.get();

        if( ammo_type->special_cookoff ) {
            // If it has a special effect just trigger it.
            apply_ammo_effects( p, ammo_type->ammo_effects );
        } else if( ammo_type->cookoff ) {
            // Ammo that cooks off, but doesn't have a
            // large intrinsic effect blows up with shrapnel
            g->explosion( p, ammo_type->damage / 2, 0.5f, false, rounds_exploded / 5 );
        }
        charges_remaining -= rounds_exploded;
        if( charges_remaining > 0 ) {
            temp_item.charges = charges_remaining;
            drops.push_back( temp_item );
        }

        return true;
    } else if( !contents.empty() && ( type->magazine == nullptr || !type->magazine->protects_contents ) ) {
        const auto new_end = std::remove_if( contents.begin(), contents.end(), [ &p, &drops ]( item &it ) {
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

bool item_ptr_compare_by_charges( const item *left, const item *right)
{
    if(left->contents.empty()) {
        return false;
    } else if( right->contents.empty()) {
        return true;
    } else {
        return right->contents.front().charges < left->contents.front().charges;
    }
}

bool item_compare_by_charges( const item& left, const item& right)
{
    return item_ptr_compare_by_charges( &left, &right);
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

std::string item::components_to_string() const
{
    typedef std::map<std::string, int> t_count_map;
    t_count_map counts;
    for( const auto &elem : components ) {
        const std::string name = elem.display_name();
        counts[name]++;
    }
    return enumerate_as_string( counts.begin(), counts.end(),
    []( const std::pair<std::string, int> &entry ) -> std::string {
        if( entry.second != 1 ) {
            return string_format( _( "%d x %s" ), entry.second, entry.first.c_str() );
        } else {
            return entry.first;
        }
    }, false );
}

bool item::needs_processing() const
{
    return active || has_flag("RADIO_ACTIVATION") ||
           ( is_container() && !contents.empty() && contents.front().needs_processing() ) ||
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
        if( item_counter == 0 ) {
            item_tags.erase( "HOT" );
        }
    } else if( item_tags.count( "COLD" ) > 0 ) {
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
            carrier->moves -= 15;
        }

        if( ( carrier->has_effect( effect_shakes ) && one_in( 10 ) ) ||
            ( carrier->has_trait( "JITTERY" ) && one_in( 200 ) ) ) {
            carrier->add_msg_if_player( m_bad, _( "Your shaking hand causes you to drop your %s." ),
                                        tname().c_str() );
            g->m.add_item_or_charges( tripoint( pos.x + rng( -1, 1 ), pos.y + rng( -1, 1 ), pos.z ), *this, 2 );
            return true; // removes the item that has just been added to the map
        }

        if( carrier->has_effect( effect_sleep ) ) {
            carrier->add_msg_if_player( m_bad, _( "You fall asleep and drop your %s." ),
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

tripoint item::get_cable_target() const
{
    if( get_var( "state" ) != "pay_out_cable" ) {
        return tripoint_min;
    }

    int source_x = get_var( "source_x", 0 );
    int source_y = get_var( "source_y", 0 );
    int source_z = get_var( "source_z", 0 );
    tripoint source( source_x, source_y, source_z );

    tripoint relpos = g->m.getlocal( source );
    return relpos;
}

bool item::process_cable( player *p, const tripoint &pos )
{
    const tripoint &source = get_cable_target();
    if( source == tripoint_min ) {
        return false;
    }

    auto veh = g->m.veh_at( source );
    if( veh == nullptr || ( source.z != g->get_levz() && !g->m.has_zlevels() ) ) {
        if( p != nullptr && p->has_item( *this ) ) {
            p->add_msg_if_player(m_bad, _("You notice the cable has come loose!"));
        }
        reset_cable(p);
        return false;
    }

    int distance = rl_dist( pos, source );
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

    if( item_counter > 0 ) {
        item_counter--;
    }

    if( item_counter == 0 && type->countdown_action ) {
        type->countdown_action.call( carrier ? carrier : &g->u, this, false, pos );
        if( type->countdown_destroy ) {
            return true;
        }
    }

    for( const auto &e : type->emits ) {
        g->m.emit_field( pos, e );
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
    if( has_flag( "DANGEROUS" ) ) {
        return true;
    }

    // Note: Item should be dangerous regardless of what type of a container is it
    // Visitable interface would skip some options
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
    const auto mats = made_of();
    return std::any_of( mats.begin(), mats.end(), []( const material_id &mid ) {
        return mid.obj().soft();
    } );
}

bool item::is_reloadable() const
{
    if( has_flag( "NO_RELOAD") && !has_flag( "VEHICLE" ) ) {
        return false; // turrets ignore NO_RELOAD flag

    } else if( is_bandolier() ) {
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
    if( corpse != nullptr && typeId() == "corpse" ) {
        if( name.empty() ) {
            return string_format( npgettext( "item name", "%s corpse",
                                         "%s corpses", quantity ),
                               corpse->nname().c_str() );
        } else {
            return string_format( npgettext( "item name", "%s corpse of %s",
                                         "%s corpses of %s", quantity ),
                               corpse->nname().c_str(), name.c_str() );
        }
    } else if( typeId() == "blood" ) {
        if( corpse == nullptr || corpse->id == NULL_ID ) {
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

bool item::is_filthy() const
{
    return has_flag( "FILTHY" ) && ( get_world_option<bool>( "FILTHY_MORALE" ) || g->u.has_trait( "SQUEAMISH" ) );
}
