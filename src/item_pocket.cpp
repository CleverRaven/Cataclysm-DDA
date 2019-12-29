#include "item_pocket.h"

#include "assign.h"
#include "crafting.h"
#include "enums.h"
#include "game.h"
#include "generic_factory.h"
#include "item.h"
#include "itype.h"
#include "json.h"
#include "map.h"
#include "player.h"
#include "point.h"
#include "units.h"

namespace io
{
// *INDENT-OFF*
template<>
std::string enum_to_string<item_pocket::pocket_type>( item_pocket::pocket_type data )
{
    switch ( data ) {
    case item_pocket::pocket_type::CONTAINER: return "CONTAINER";
    case item_pocket::pocket_type::MAGAZINE: return "MAGAZINE";
    case item_pocket::pocket_type::LEGACY_CONTAINER: return "LEGACY_CONTAINER";
    case item_pocket::pocket_type::LAST: break;
    }
    debugmsg( "Invalid valid_target" );
    abort();
}
// *INDENT-ON*
} // namespace io

void pocket_data::load( const JsonObject &jo )
{
    optional( jo, was_loaded, "pocket_type", type, item_pocket::pocket_type::CONTAINER );
    optional( jo, was_loaded, "min_item_volume", min_item_volume, volume_reader(), 0_ml );
    mandatory( jo, was_loaded, "max_contains_volume", max_contains_volume, volume_reader() );
    mandatory( jo, was_loaded, "max_contains_weight", max_contains_weight, mass_reader() );
    optional( jo, was_loaded, "spoil_multiplier", spoil_multiplier, 1.0f );
    optional( jo, was_loaded, "weight_multiplier", weight_multiplier, 1.0f );
    optional( jo, was_loaded, "moves", moves, 100 );
    optional( jo, was_loaded, "fire_protection", fire_protection, false );
    optional( jo, was_loaded, "watertight", watertight, false );
    optional( jo, was_loaded, "gastight", gastight, false );
    optional( jo, was_loaded, "open_container", open_container, false );
    optional( jo, was_loaded, "flag_restriction", flag_restriction );
    optional( jo, was_loaded, "rigid", rigid, false );
}

void item_pocket::serialize( JsonOut &json ) const
{
    json.start_object();

    json.member( "contents", contents );

    json.end_object();
}

bool item_pocket::operator==( const item_pocket &rhs ) const
{
    return *data == *rhs.data;
}

bool pocket_data::operator==( const pocket_data &rhs ) const
{
    return rigid == rhs.rigid &&
           watertight == rhs.watertight &&
           gastight == rhs.gastight &&
           fire_protection == rhs.fire_protection &&
           flag_restriction == rhs.flag_restriction &&
           type == rhs.type &&
           max_contains_volume == rhs.max_contains_volume &&
           min_item_volume == rhs.min_item_volume &&
           max_contains_weight == rhs.max_contains_weight &&
           spoil_multiplier == rhs.spoil_multiplier &&
           weight_multiplier == rhs.weight_multiplier &&
           moves == rhs.moves;
}

bool item_pocket::stacks_with( const item_pocket &rhs ) const
{
    if( contents.size() != rhs.contents.size() ) {
        return false;
    }
    return std::equal( contents.begin(), contents.end(), rhs.contents.begin(),
    []( const item & a, const item & b ) {
        return a.charges == b.charges && a.stacks_with( b );
    } );
}

void item_pocket::deserialize( JsonIn &jsin )
{
    JsonObject data = jsin.get_object();
    optional( data, was_loaded, "contents", contents );
}

item &item_pocket::back()
{
    return contents.back();
}

const item &item_pocket::back() const
{
    return contents.back();
}

item &item_pocket::front()
{
    return contents.front();
}

const item &item_pocket::front() const
{
    return contents.front();
}

void item_pocket::pop_back()
{
    contents.pop_back();
}

size_t item_pocket::size() const
{
    return contents.size();
}

units::volume item_pocket::volume_capacity() const
{
    return data->max_contains_volume;
}

units::volume item_pocket::remaining_volume() const
{
    return data->max_contains_volume - contains_volume();
}

units::volume item_pocket::item_size_modifier() const
{
    if( data->rigid ) {
        return 0_ml;
    }
    units::volume total_vol = 0_ml;
    for( const item &it : contents ) {
        total_vol += it.volume();
    }
    return total_vol;
}

units::mass item_pocket::item_weight_modifier() const
{
    units::mass total_mass = 0_gram;
    for( const item &it : contents ) {
        total_mass += it.weight() * data->weight_multiplier;
    }
    return total_mass;
}

item *item_pocket::magazine_current()
{
    auto iter = std::find_if( contents.begin(), contents.end(), []( const item & it ) {
        return it.is_magazine();
    } );
    return iter != contents.end() ? &*iter : nullptr;
}

void item_pocket::casings_handle( const std::function<bool( item & )> &func )
{
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

bool item_pocket::use_amount( const itype_id &it, int &quantity, std::list<item> &used )
{
    bool used_item = false;
    for( auto a = contents.begin(); a != contents.end() && quantity > 0; ) {
        if( a->use_amount( it, quantity, used ) ) {
            used_item = true;
            a = contents.erase( a );
        } else {
            ++a;
        }
    }
    return used_item;
}

bool item_pocket::will_explode_in_a_fire() const
{
    if( data->fire_protection ) {
        return false;
    }
    return std::any_of( contents.begin(), contents.end(), []( const item & it ) {
        return it.will_explode_in_fire();
    } );
}

bool item_pocket::detonate( const tripoint &pos, std::vector<item> &drops )
{
    const auto new_end = std::remove_if( contents.begin(), contents.end(), [&pos, &drops]( item & it ) {
        return it.detonate( pos, drops );
    } );
    if( new_end != contents.end() ) {
        contents.erase( new_end, contents.end() );
        // If any of the contents explodes, so does the container
        return true;
    }
    return false;
}

bool item_pocket::process( const itype &type, player *carrier, const tripoint &pos, bool activate,
                           float insulation, const temperature_flag flag )
{
    const bool preserves = type.container && type.container->preserves;
    bool processed = false;
    for( auto it = contents.begin(); it != contents.end(); ) {
        if( preserves ) {
            // Simulate that the item has already "rotten" up to last_rot_check, but as item::rot
            // is not changed, the item is still fresh.
            it->set_last_rot_check( calendar::turn );
        }
        if( it->process( carrier, pos, activate, type.insulation_factor * insulation, flag ) ) {
            it = contents.erase( it );
            processed = true;
        } else {
            ++it;
        }
    }
    return processed;
}

bool item_pocket::legacy_unload( player *guy, bool &changed )
{
    contents.erase( std::remove_if( contents.begin(), contents.end(),
    [guy, &changed]( item & e ) {
        int old_charges = e.charges;
        const bool consumed = guy->add_or_drop_with_msg( e, true );
        changed = changed || consumed || e.charges != old_charges;
        if( consumed ) {
            guy->mod_moves( -guy->item_handling_cost( e ) );
        }
        return consumed;
    } ), contents.end() );
    return changed;
}

void item_pocket::remove_all_ammo( Character &guy )
{
    for( auto iter = contents.begin(); iter != contents.end(); ) {
        if( iter->is_irremovable() ) {
            iter++;
            continue;
        }
        drop_or_handle( *iter, guy );
        iter = contents.erase( iter );
    }
}

void item_pocket::remove_all_mods( Character &guy )
{
    auto mod = std::find_if( contents.begin(), contents.end(), []( const item & e ) {
        return e.is_toolmod();
    } );
    guy.i_add_or_drop( *mod );
    contents.erase( mod );
}

static void insert_separation_line( std::vector<iteminfo> &info )
{
    if( info.empty() || info.back().sName != "--" ) {
        info.push_back( iteminfo( "DESCRIPTION", "--" ) );
    }
}

static std::string vol_to_string( const units::volume &vol )
{
    int converted_volume_scale = 0;
    const double converted_volume =
        convert_volume( vol.value(),
                        &converted_volume_scale );

    return string_format( "%.3f %s", converted_volume, volume_units_abbr() );
}

static std::string weight_to_string( const units::mass &weight )
{
    const double converted_weight = convert_weight( weight );
    return string_format( "%.2f %s", converted_weight, weight_units() );
}

void item_pocket::general_info( std::vector<iteminfo> &info, int pocket_number,
                                bool disp_pocket_number ) const
{
    const std::string space = "  ";

    if( type != LEGACY_CONTAINER ) {
        if( disp_pocket_number ) {
            info.emplace_back( "DESCRIPTION", _( string_format( "Pocket %d:", pocket_number ) ) );
        }
        if( data->rigid ) {
            info.emplace_back( "DESCRIPTION", _( "This pocket is <info>rigid</info>." ) );
        }
        if( data->min_item_volume > 0_ml ) {
            info.emplace_back( "DESCRIPTION",
                               _( string_format( "Minimum volume of item allowed: <neutral>%s</neutral>",
                                                 vol_to_string( data->min_item_volume ) ) ) );
        }
        info.emplace_back( "DESCRIPTION",
                           _( string_format( "Volume Capacity: <neutral>%s</neutral>",
                                             vol_to_string( data->max_contains_volume ) ) ) );
        info.emplace_back( "DESCRIPTION",
                           _( string_format( "Weight Capacity: <neutral>%s</neutral>",
                                             weight_to_string( data->max_contains_weight ) ) ) );

        info.emplace_back( "DESCRIPTION",
                           _( string_format( "This pocket takes <neutral>%d</neutral> base moves to take an item out.",
                                             data->moves ) ) );

        if( data->watertight ) {
            info.emplace_back( "DESCRIPTION",
                               _( "This pocket can <info>contain a liquid</info>." ) );
        }
        if( data->gastight ) {
            info.emplace_back( "DESCRIPTION",
                               _( "This pocket can <info>contain a gas</info>." ) );
        }
        if( data->open_container ) {
            info.emplace_back( "DESCRIPTION",
                               _( "This pocket will <bad>spill</bad> if placed into another item or worn." ) );
        }
        if( data->fire_protection ) {
            info.emplace_back( "DESCRIPTION",
                               _( "This pocket <info>protects its contents from fire</info>." ) );
        }
        if( data->spoil_multiplier != 1.0f ) {
            info.emplace_back( "DESCRIPTION",
                               string_format(
                                   _( "This pocket makes contained items spoil at <neutral>%.0f%%</neutral> their original rate." ),
                                   data->spoil_multiplier * 100 ) );
        }
        if( data->weight_multiplier != 1.0f ) {
            info.emplace_back( "DESCRIPTION",
                               string_format( _( "Items in this pocket weigh <neutral>%.0f%%</neutral> their original weight." ),
                                              data->weight_multiplier * 100 ) );
        }
    }
}

void item_pocket::contents_info( std::vector<iteminfo> &info, int pocket_number,
                                 bool disp_pocket_number ) const
{
    const std::string space = "  ";

    insert_separation_line( info );
    if( disp_pocket_number ) {
        info.emplace_back( "DESCRIPTION", _( string_format( "<bold>Pocket %d</bold>", pocket_number ) ) );
    }
    if( contents.empty() ) {
        info.emplace_back( "DESCRIPTION", _( "This pocket is empty." ) );
        return;
    }
    info.emplace_back( "DESCRIPTION",
                       _( string_format( "Volume: <neutral>%s / %s</neutral>",
                                         vol_to_string( contains_volume() ), vol_to_string( data->max_contains_volume ) ) ) );
    info.emplace_back( "DESCRIPTION",
                       _( string_format( "Weight: <neutral>%s / %s</neutral>",
                                         weight_to_string( contains_weight() ), weight_to_string( data->max_contains_weight ) ) ) );

    bool contents_header = false;
    for( const item &contents_item : contents ) {
        if( !contents_item.type->mod ) {
            if( !contents_header ) {
                info.emplace_back( "DESCRIPTION", _( "<bold>Contents of this pocket</bold>:" ) );
                contents_header = true;
            } else {
                // Separate items with a blank line
                info.emplace_back( "DESCRIPTION", space );
            }

            const translation &description = contents_item.type->description;

            if( contents_item.made_of_from_type( LIQUID ) ) {
                units::volume contents_volume = contents_item.volume();
                int converted_volume_scale = 0;
                const double converted_volume =
                    round_up( convert_volume( contents_volume.value(),
                                              &converted_volume_scale ), 2 );
                info.emplace_back( "DESCRIPTION", contents_item.display_name() );
                iteminfo::flags f = iteminfo::no_newline;
                if( converted_volume_scale != 0 ) {
                    f |= iteminfo::is_decimal;
                }
                info.emplace_back( "CONTAINER", description + space,
                                   string_format( "<num> %s", volume_units_abbr() ), f,
                                   converted_volume );
            } else {
                info.emplace_back( "DESCRIPTION", contents_item.display_name() );
            }
        }
    }
}

ret_val<item_pocket::contain_code> item_pocket::can_contain( const item &it ) const
{
    // legacy container must be added to explicitly
    if( type == pocket_type::LEGACY_CONTAINER ) {
        return ret_val<item_pocket::contain_code>::make_failure( contain_code::ERR_LEGACY_CONTAINER );
    }
    if( it.made_of( phase_id::LIQUID ) && !data->watertight ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_LIQUID, _( "can't contain liquid" ) );
    }
    if( it.made_of( phase_id::GAS ) && !data->gastight ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_GAS, _( "can't contain gas" ) );
    }
    if( it.volume() < data->min_item_volume ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_TOO_SMALL, _( "item is too small" ) );
    }
    if( it.weight() > data->max_contains_weight ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_TOO_HEAVY, _( "item is too heavy" ) );
    }
    if( it.weight() > remaining_weight() ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_CANNOT_SUPPORT, _( "pocket is holding too much weight" ) );
    }
    if( !data->flag_restriction.empty() && !it.has_any_flag( data->flag_restriction ) ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_FLAG, _( "item does not have correct flag" ) );
    }
    if( it.volume() > data->max_contains_volume ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_TOO_BIG, _( "item too big" ) );
    }
    if( it.volume() > remaining_volume() ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_NO_SPACE, _( "not enough space" ) );
    }
    return ret_val<item_pocket::contain_code>::make_success();
}

cata::optional<item> item_pocket::remove_item( const item &it )
{
    item ret( it );
    const size_t sz = contents.size();
    contents.remove_if( [&it]( const item & rhs ) {
        return &rhs == &it;
    } );
    if( sz == contents.size() ) {
        return cata::nullopt;
    } else {
        return ret;
    }
}

cata::optional<item> item_pocket::remove_item( const item_location &it )
{
    if( !it ) {
        return cata::nullopt;
    }
    return remove_item( *it );
}

bool item_pocket::spill_contents( const tripoint &pos )
{
    for( item &it : contents ) {
        g->m.add_item_or_charges( pos, it );
    }

    contents.clear();
    return true;
}

void item_pocket::clear_items()
{
    contents.clear();
}

bool item_pocket::has_item( const item &it ) const
{
    return contents.end() !=
    std::find_if( contents.begin(), contents.end(), [&it]( const item & e ) {
        return &it == &e;
    } );
}

item *item_pocket::get_item_with( const std::function<bool( const item & )> &filter )
{
    for( item &it : contents ) {
        if( filter( it ) ) {
            return &it;
        }
    }
    return nullptr;
}

void item_pocket::remove_items_if( const std::function<bool( item & )> &filter )
{
    contents.remove_if( filter );
}

void item_pocket::has_rotten_away( const tripoint &pnt )
{
    for( auto it = contents.begin(); it != contents.end(); ) {
        if( g->m.has_rotten_away( *it, pnt ) ) {
            it = contents.erase( it );
        } else {
            ++it;
        }
    }
}

bool item_pocket::empty() const
{
    return contents.empty();
}

void item_pocket::add( const item &it )
{
    contents.push_back( it );
}

std::list<item> &item_pocket::edit_contents()
{
    return contents;
}

ret_val<item_pocket::contain_code> item_pocket::insert_item( const item &it )
{
    const ret_val<item_pocket::contain_code> ret = can_contain( it );
    if( ret.success() ) {
        contents.push_back( it );
    }
    return ret;
}

int item_pocket::obtain_cost( const item &it ) const
{
    if( has_item( it ) ) {
        return data->moves;
    }
    return 0;
}

bool item_pocket::is_type( pocket_type ptype ) const
{
    return ptype == type;
}

units::volume item_pocket::contains_volume() const
{
    units::volume vol = 0_ml;
    for( const item &it : contents ) {
        vol += it.volume();
    }
    return vol;
}

units::mass item_pocket::contains_weight() const
{
    units::mass weight = 0_gram;
    for( const item &it : contents ) {
        weight += it.weight();
    }
    return weight;
}

units::mass item_pocket::remaining_weight() const
{
    return data->max_contains_weight - contains_weight();
}
