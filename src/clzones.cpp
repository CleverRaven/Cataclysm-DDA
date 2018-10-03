#include "clzones.h"
#include "game.h"
#include "json.h"
#include "debug.h"
#include "output.h"
#include "cata_utility.h"
#include "translations.h"
#include "ui.h"
#include "string_input_popup.h"
#include "line.h"
#include "item.h"
#include "itype.h"
#include "item_category.h"
#include "iexamine.h"

#include <iostream>

zone_manager::zone_manager()
{
    types.emplace( zone_type_id( "NO_AUTO_PICKUP" ),
                   zone_type( translate_marker( "No Auto Pickup" ),
                              translate_marker( "You won't auto-pickup items inside the zone." ) ) );
    types.emplace( zone_type_id( "NO_NPC_PICKUP" ),
                   zone_type( translate_marker( "No NPC Pickup" ),
                              translate_marker( "Friendly NPCs don't pickup items inside the zone." ) ) );
    types.emplace( zone_type_id( "LOOT_UNSORTED" ),
                   zone_type( translate_marker( "Loot: Unsorted" ),
                              translate_marker( "Place to drop unsorted loot. You can use \"sort out loot\" zone-action to sort items inside. It can overlap with Loot zones of different types." ) ) );
    types.emplace( zone_type_id( "LOOT_FOOD" ),
                   zone_type( translate_marker( "Loot: Food" ),
                              translate_marker( "Destination for comestibles. If more specific food zone is not defined, all food is moved here." ) ) );
    types.emplace( zone_type_id( "LOOT_PFOOD" ),
                   zone_type( translate_marker_context( "perishable food", "Loot: P.Food" ),
                              translate_marker( "Destination for perishable comestibles. Does include perishable drinks if such zone is not specified." ) ) );
    types.emplace( zone_type_id( "LOOT_DRINK" ),
                   zone_type( translate_marker( "Loot: Drink" ),
                              translate_marker( "Destination for drinks. Does include perishable drinks if such zone is not specified." ) ) );
    types.emplace( zone_type_id( "LOOT_PDRINK" ),
                   zone_type( translate_marker_context( "perishable drink", "Loot: P.Drink" ),
                              translate_marker( "Destination for perishable drinks." ) ) );
    types.emplace( zone_type_id( "LOOT_GUNS" ),
                   zone_type( translate_marker( "Loot: Guns" ),
                              translate_marker( "Destination for guns, bows and similar weapons." ) ) );
    types.emplace( zone_type_id( "LOOT_MAGAZINES" ),
                   zone_type( translate_marker_context( "gun magazines", "Loot: Magazines" ),
                              translate_marker( "Destination for gun magazines." ) ) );
    types.emplace( zone_type_id( "LOOT_AMMO" ),
                   zone_type( translate_marker( "Loot: Ammo" ),
                              translate_marker( "Destination for ammo." ) ) );
    types.emplace( zone_type_id( "LOOT_WEAPONS" ),
                   zone_type( translate_marker( "Loot: Weapons" ),
                              translate_marker( "Destination for melee weapons." ) ) );
    types.emplace( zone_type_id( "LOOT_TOOLS" ),
                   zone_type( translate_marker( "Loot: Tools" ),
                              translate_marker( "Destination for tools." ) ) );
    types.emplace( zone_type_id( "LOOT_CLOTHING" ),
                   zone_type( translate_marker( "Loot: Clothing" ),
                              translate_marker( "Destination for clothing. Does include filthy clothing if such zone is not specified." ) ) );
    types.emplace( zone_type_id( "LOOT_FCLOTHING" ),
                   zone_type( translate_marker_context( "filthy clothing", "Loot: F.Clothing" ),
                              translate_marker( "Destination for filthy clothing." ) ) );
    types.emplace( zone_type_id( "LOOT_DRUGS" ),
                   zone_type( translate_marker( "Loot: Drugs" ),
                              translate_marker( "Destination for drugs and other medical items." ) ) );
    types.emplace( zone_type_id( "LOOT_BOOKS" ),
                   zone_type( translate_marker( "Loot: Books" ),
                              translate_marker( "Destination for books and magazines." ) ) );
    types.emplace( zone_type_id( "LOOT_MODS" ),
                   zone_type( translate_marker( "Loot: Mods" ),
                              translate_marker( "Destination for firearm modifications and similar items." ) ) );
    types.emplace( zone_type_id( "LOOT_MUTAGENS" ),
                   zone_type( translate_marker( "Loot: Mutagens" ),
                              translate_marker( "Destination for mutagens, serums, and purifiers." ) ) );
    types.emplace( zone_type_id( "LOOT_BIONICS" ),
                   zone_type( translate_marker( "Loot: Bionics" ),
                              translate_marker( "Destination for Compact Bionics Modules aka CBMs." ) ) );
    types.emplace( zone_type_id( "LOOT_VEHICLE_PARTS" ),
                   zone_type( translate_marker_context( "vehicle parts", "Loot: V.Parts" ),
                              translate_marker( "Destination for vehicle parts." ) ) );
    types.emplace( zone_type_id( "LOOT_OTHER" ),
                   zone_type( translate_marker( "Loot: Other" ),
                              translate_marker( "Destination for other miscellaneous items." ) ) );
    types.emplace( zone_type_id( "LOOT_FUEL" ),
                   zone_type( translate_marker( "Loot: Fuel" ),
                              translate_marker( "Destination for gasoline, diesel, lamp oil and other substances used as a fuel." ) ) );
    types.emplace( zone_type_id( "LOOT_SEEDS" ),
                   zone_type( translate_marker( "Loot: Seeds" ),
                              translate_marker( "Destination for seeds, stems and similar items." ) ) );
    types.emplace( zone_type_id( "LOOT_CHEMICAL" ),
                   zone_type( translate_marker( "Loot: Chemical" ),
                              translate_marker( "Destination for chemicals." ) ) );
    types.emplace( zone_type_id( "LOOT_SPARE_PARTS" ),
                   zone_type( translate_marker_context( "spare parts", "Loot: S.Parts" ),
                              translate_marker( "Destination for spare parts." ) ) );
    types.emplace( zone_type_id( "LOOT_ARTIFACTS" ),
                   zone_type( translate_marker( "Loot: Artifacts" ),
                              translate_marker( "Destination for artifacts" ) ) );
    types.emplace( zone_type_id( "LOOT_ARMOR" ),
                   zone_type( translate_marker( "Loot: Armor" ),
                              translate_marker( "Destination for armor. Does include filthy armor if such zone is not specified." ) ) );
    types.emplace( zone_type_id( "LOOT_FARMOR" ),
                   zone_type( translate_marker_context( "filthy armor", "Loot: F.Armor" ),
                              translate_marker( "Destination for filthy armor." ) ) );
    types.emplace( zone_type_id( "LOOT_WOOD" ),
                   zone_type( translate_marker( "Loot: Wood" ),
                              translate_marker( "Destination for firewood and items that can be used as such." ) ) );
    types.emplace( zone_type_id( "LOOT_IGNORE" ),
                   zone_type( translate_marker( "Loot: Ignore" ),
                              translate_marker( "Items inside of this zone are ignored by \"sort out loot\" zone-action." ) ) );
    types.emplace( zone_type_id( "FARM_PLOT" ),
                   zone_type( translate_marker_context( "plot of land", "Farm: Plot" ),
                              translate_marker( "Designate a farm plot for tilling and planting." ) ) );
}

std::string zone_type::name() const
{
    return _( name_.c_str() );
}

std::string zone_type::desc() const
{
    return _( desc_.c_str() );
}

std::shared_ptr<zone_options> zone_options::create( const zone_type_id &type )
{
    if( type == zone_type_id( "FARM_PLOT" ) ) {
        return std::make_shared<plot_options>();
    }

    return std::make_shared<zone_options>();
};

bool zone_options::is_valid( const zone_type_id &type, const zone_options &options )
{
    if( type == zone_type_id( "FARM_PLOT" ) ) {
        return dynamic_cast<const plot_options *>( &options ) != nullptr ;
    }

    // ensure options is not derived class for the rest of zone types
    return !options.has_options();
}

plot_options::query_seed_result plot_options::query_seed()
{
    player &p = g->u;

    std::vector<item *> seed_inv = p.items_with( []( const item & itm ) {
        return itm.is_seed();
    } );

    auto seed_entries = iexamine::get_seed_entries( seed_inv );
    seed_entries.emplace( seed_entries.begin(), seed_tuple( itype_id( "null" ), "No seed", 0 ) );

    int seed_index = iexamine::query_seed( seed_entries );

    if( seed_index > 0 && seed_index < static_cast<int>( seed_entries.size() ) ) {
        const auto &seed_entry = seed_entries[seed_index];
        const auto new_seed = std::get<0>( seed_entry );
        std::string new_mark;

        item it = item( itype_id( new_seed ) );
        if( it.is_seed() ) {
            new_mark = it.type->seed->fruit_id;
        } else {
            new_mark = seed;
        }

        if( new_seed != seed || new_mark != mark ) {
            seed = new_seed;
            mark = new_mark;
            return changed;
        } else {
            return successful;
        }
    } else if( seed_index == 0 ) { // No seeds
        if( seed != "" || mark != "" ) {
            seed = "";
            mark = "";
            return changed;
        } else {
            return successful;
        }
    } else {
        return canceled;
    }
}

bool plot_options::query_at_creation()
{
    return query_seed() != canceled;
};

bool plot_options::query()
{
    return query_seed() == changed;
};

std::string plot_options::get_zone_name_suggestion() const
{
    if( seed != "" ) {
        auto type = itype_id( seed );
        item it = item( type );
        if( it.is_seed() ) {
            return it.type->seed->plant_name;
        } else {
            return item::nname( type );
        }
    }

    return _( "No seed" );
};

std::vector<std::pair<std::string, std::string>> plot_options::get_descriptions() const
{
    auto options = std::vector<std::pair<std::string, std::string>>();
    options.emplace_back( std::make_pair( _( "Plant seed: " ),
                                          seed != "" ? item::nname( itype_id( seed ) ) : _( "No seed" ) ) );

    return options;
}

void plot_options::serialize( JsonOut &json ) const
{
    json.member( "mark", mark );
    json.member( "seed", seed );
};

void plot_options::deserialize( JsonObject &jo_zone )
{
    mark = jo_zone.get_string( "mark", "" );
    seed = jo_zone.get_string( "seed", "" );
};

cata::optional<std::string> zone_manager::query_name( std::string default_name ) const
{
    string_input_popup popup;
    popup
    .title( _( "Zone name:" ) )
    .width( 55 )
    .text( default_name )
    .max_length( 15 )
    .query();
    if( popup.canceled() ) {
        return {};
    } else {
        return popup.text();
    }
}

cata::optional<zone_type_id> zone_manager::query_type() const
{
    const auto &types = get_manager().get_types();
    uilist as_m;
    as_m.desc_enabled = true;
    as_m.text = _( "Select zone type:" );

    size_t i = 0;
    for( const auto &pair : types ) {
        const auto &type = pair.second;

        as_m.addentry_desc( i++, true, MENU_AUTOASSIGN, type.name(), type.desc() );
    }

    as_m.query();
    if( as_m.ret < 0 ) {
        return {};
    }
    size_t index = as_m.ret;

    auto iter = types.begin();
    std::advance( iter, index );

    return iter->first;
}

bool zone_manager::zone_data::set_name()
{
    const auto maybe_name = get_manager().query_name( name );
    if( maybe_name.has_value() ) {
        auto new_name = maybe_name.value();
        if( new_name.empty() ) {
            new_name = _( "<no name>" );
        }
        if( name != new_name ) {
            name = new_name;
            return true;
        }
    }
    return false;
}

bool zone_manager::zone_data::set_type()
{
    const auto maybe_type = get_manager().query_type();
    if( maybe_type.has_value() && maybe_type.value() != type ) {
        auto new_options = zone_options::create( maybe_type.value() );
        if( new_options->query_at_creation() ) {
            type = maybe_type.value();
            options = new_options;
            get_manager().cache_data();
            return true;
        }
    }
    return false;
}

void zone_manager::zone_data::set_position( const std::pair<tripoint, tripoint> position )
{
    start = position.first;
    end = position.second;

    get_manager().cache_data();
}

void zone_manager::zone_data::set_enabled( const bool _enabled )
{
    enabled = _enabled;
}

tripoint zone_manager::zone_data::get_center_point() const
{
    return tripoint( ( start.x + end.x ) / 2, ( start.y + end.y ) / 2, ( start.z + end.z ) / 2 );
}

std::string zone_manager::get_name_from_type( const zone_type_id &type ) const
{
    const auto &iter = types.find( type );
    if( iter != types.end() ) {
        return iter->second.name();
    }

    return "Unknown Type";
}

bool zone_manager::has_type( const zone_type_id &type ) const
{
    return types.count( type ) > 0;
}

void zone_manager::cache_data()
{
    area_cache.clear();

    for( auto &elem : zones ) {
        if( !elem.get_enabled() ) {
            continue;
        }

        const zone_type_id &type = elem.get_type();
        auto &cache = area_cache[type];

        tripoint start = elem.get_start_point();
        tripoint end = elem.get_end_point();

        // Draw marked area
        for( int x = start.x; x <= end.x; ++x ) {
            for( int y = start.y; y <= end.y; ++y ) {
                for( int z = start.z; z <= end.z; ++z ) {
                    cache.insert( tripoint( x, y, z ) );
                }
            }
        }
    }
}

std::unordered_set<tripoint> zone_manager::get_point_set( const zone_type_id &type ) const
{
    const auto &type_iter = area_cache.find( type );
    if( type_iter == area_cache.end() ) {
        return std::unordered_set<tripoint>();
    }

    return type_iter->second;;
}

bool zone_manager::has( const zone_type_id &type, const tripoint &where ) const
{
    const auto &point_set = get_point_set( type );
    return point_set.find( where ) != point_set.end();
}

bool zone_manager::has_near( const zone_type_id &type, const tripoint &where ) const
{
    const auto &point_set = get_point_set( type );

    for( auto &point : point_set ) {
        if( square_dist( point, where ) <= MAX_DISTANCE ) {
            return true;
        }
    }

    return false;
}

std::unordered_set<tripoint> zone_manager::get_near( const zone_type_id &type,
        const tripoint &where ) const
{
    const auto &point_set = get_point_set( type );
    auto near_point_set = std::unordered_set<tripoint>();

    for( auto &point : point_set ) {
        if( square_dist( point, where ) <= MAX_DISTANCE ) {
            near_point_set.insert( point );
        }
    }

    return near_point_set;
}

zone_type_id zone_manager::get_near_zone_type_for_item( const item &it,
        const tripoint &where ) const
{
    auto cat = it.get_category();

    if( it.has_flag( "FIREWOOD" ) ) {
        if( has_near( zone_type_id( "LOOT_WOOD" ), where ) ) {
            return zone_type_id( "LOOT_WOOD" );
        }
    }

    if( cat.id() == "food" ) {
        const bool preserves = it.is_food_container() && it.type->container->preserves ? 1 : 0;
        const auto &it_food = it.is_food_container() ? it.contents.front() : it;

        if( it_food.is_food() ) { // skip food without comestible, like MREs
            if( it_food.type->comestible->comesttype == "DRINK" ) {
                if( !preserves && it_food.goes_bad() && has_near( zone_type_id( "LOOT_PDRINK" ), where ) ) {
                    return zone_type_id( "LOOT_PDRINK" );
                } else if( has_near( zone_type_id( "LOOT_DRINK" ), where ) ) {
                    return zone_type_id( "LOOT_DRINK" );
                }
            }

            if( !preserves && it_food.goes_bad() && has_near( zone_type_id( "LOOT_PFOOD" ), where ) ) {
                return zone_type_id( "LOOT_PFOOD" );
            }
        }

        return zone_type_id( "LOOT_FOOD" );
    }
    if( cat.id() == "guns" ) {
        return zone_type_id( "LOOT_GUNS" );
    }
    if( cat.id() == "magazines" ) {
        return zone_type_id( "LOOT_MAGAZINES" );
    }
    if( cat.id() == "ammo" ) {
        return zone_type_id( "LOOT_AMMO" );
    }
    if( cat.id() == "weapons" ) {
        return zone_type_id( "LOOT_WEAPONS" );
    }
    if( cat.id() == "tools" ) {
        return zone_type_id( "LOOT_TOOLS" );
    }
    if( cat.id() == "clothing" ) {
        if( it.is_filthy() && has_near( zone_type_id( "LOOT_FCLOTHING" ), where ) ) {
            return zone_type_id( "LOOT_FCLOTHING" );
        }
        return zone_type_id( "LOOT_CLOTHING" );
    }
    if( cat.id() == "drugs" ) {
        return zone_type_id( "LOOT_DRUGS" );
    }
    if( cat.id() == "books" ) {
        return zone_type_id( "LOOT_BOOKS" );
    }
    if( cat.id() == "mods" ) {
        return zone_type_id( "LOOT_MODS" );
    }
    if( cat.id() == "mutagen" ) {
        return zone_type_id( "LOOT_MUTAGENS" );
    }
    if( cat.id() == "bionics" ) {
        return zone_type_id( "LOOT_BIONICS" );
    }
    if( cat.id() == "veh_parts" ) {
        return zone_type_id( "LOOT_VEHICLE_PARTS" );
    }
    if( cat.id() == "other" ) {
        return zone_type_id( "LOOT_OTHER" );
    }
    if( cat.id() == "fuel" ) {
        return zone_type_id( "LOOT_FUEL" );
    }
    if( cat.id() == "seeds" ) {
        return zone_type_id( "LOOT_SEEDS" );
    }
    if( cat.id() == "chems" ) {
        return zone_type_id( "LOOT_CHEMICAL" );
    }
    if( cat.id() == "spare_parts" ) {
        return zone_type_id( "LOOT_SPARE_PARTS" );
    }
    if( cat.id() == "artifacts" ) {
        return zone_type_id( "LOOT_ARTIFACTS" );
    }
    if( cat.id() == "armor" ) {
        if( it.is_filthy() && has_near( zone_type_id( "LOOT_FARMOR" ), where ) ) {
            return zone_type_id( "LOOT_FARMOR" );
        }
        return zone_type_id( "LOOT_ARMOR" );
    }

    return zone_type_id();
}

std::vector<zone_manager::zone_data> zone_manager::get_zones( const zone_type_id &type,
        const tripoint &where ) const
{
    auto zones = std::vector<zone_manager::zone_data>();

    for( const auto &zone : this->zones ) {
        if( zone.get_type() == type ) {
            if( zone.has_inside( where ) ) {
                zones.emplace_back( zone );
            }
        }
    }

    return zones;
}

const zone_manager::zone_data *zone_manager::get_top_zone( const tripoint &where ) const
{
    for( const auto &zone : zones ) {
        if( zone.has_inside( where ) ) {
            return &zone;
        }
    }

    return nullptr;
}

const zone_manager::zone_data *zone_manager::get_bottom_zone( const tripoint &where ) const
{
    for( auto it = zones.rbegin(); it != zones.rend(); ++it ) {
        const auto &zone = *it;

        if( zone.has_inside( where ) ) {
            return &zone;
        }
    }

    return nullptr;
}

zone_manager::zone_data &zone_manager::add( const std::string &name, const zone_type_id &type,
        const bool invert, const bool enabled, const tripoint &start, const tripoint &end,
        std::shared_ptr<zone_options> options )
{
    zones.push_back( zone_data( name, type, invert, enabled, start, end, options ) );
    cache_data();

    return zones.back();
}

bool zone_manager::remove( const size_t index )
{
    if( index < zones.size() ) {
        zones.erase( zones.begin() + index );
        return true;
    }

    return false;
}

bool zone_manager::remove( zone_data &zone )
{
    for( auto it = zones.begin(); it != zones.end(); ++it ) {
        if( &zone == &*it ) {
            zones.erase( it );
            return true;
        }
    }

    return false;
}

void zone_manager::swap( zone_data &a, zone_data &b )
{
    std::swap( a, b );
}

std::vector<zone_manager::ref_zone_data> zone_manager::get_zones()
{
    auto zones = std::vector<ref_zone_data>();

    for( auto &zone : this->zones ) {
        zones.emplace_back( zone );
    }

    return zones;
}

std::vector<zone_manager::ref_const_zone_data> zone_manager::get_zones() const
{
    auto zones = std::vector<ref_const_zone_data>();

    for( auto &zone : this->zones ) {
        zones.emplace_back( zone );
    }

    return zones;
}

void zone_manager::serialize( JsonOut &json ) const
{
    json.start_array();
    for( auto &elem : zones ) {
        json.start_object();

        json.member( "name", elem.get_name() );
        json.member( "type", elem.get_type() );

        json.member( "invert", elem.get_invert() );
        json.member( "enabled", elem.get_enabled() );

        tripoint start = elem.get_start_point();
        tripoint end = elem.get_end_point();

        json.member( "start_x", start.x );
        json.member( "start_y", start.y );
        json.member( "start_z", start.z );
        json.member( "end_x", end.x );
        json.member( "end_y", end.y );
        json.member( "end_z", end.z );

        elem.get_options().serialize( json );

        json.end_object();
    }

    json.end_array();
}

void zone_manager::deserialize( JsonIn &jsin )
{
    zones.clear();

    jsin.start_array();
    while( !jsin.end_array() ) {
        JsonObject jo_zone = jsin.get_object();

        const std::string name = jo_zone.get_string( "name" );
        const zone_type_id type( jo_zone.get_string( "type" ) );

        const bool invert = jo_zone.get_bool( "invert" );
        const bool enabled = jo_zone.get_bool( "enabled" );

        // Z-coordinates need to have a default value - old saves won't have those
        const int start_x = jo_zone.get_int( "start_x" );
        const int start_y = jo_zone.get_int( "start_y" );
        const int start_z = jo_zone.get_int( "start_z", 0 );
        const int end_x = jo_zone.get_int( "end_x" );
        const int end_y = jo_zone.get_int( "end_y" );
        const int end_z = jo_zone.get_int( "end_z", 0 );

        if( has_type( type ) ) {
            auto &zone = add( name, type, invert, enabled,
                              tripoint( start_x, start_y, start_z ),
                              tripoint( end_x, end_y, end_z ) );
            zone.get_options().deserialize( jo_zone );
        } else {
            debugmsg( "Invalid zone type: %s", type.c_str() );
        }
    }
}

bool zone_manager::save_zones()
{
    std::string savefile = g->get_player_base_save_path() + ".zones.json";

    return write_to_file_exclusive( savefile, [&]( std::ostream & fout ) {
        JsonOut jsout( fout );
        serialize( jsout );
    }, _( "zones date" ) );
}

void zone_manager::load_zones()
{
    std::string savefile = g->get_player_base_save_path() + ".zones.json";

    read_from_file_optional( savefile, [&]( std::istream & fin ) {
        JsonIn jsin( fin );
        deserialize( jsin );
    } );

    cache_data();
}
