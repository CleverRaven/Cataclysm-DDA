#include "regional_settings.h"

#include <algorithm>
#include <map>
#include <string>
#include <utility>

#include "debug.h"
#include "json.h"
#include "options.h"
#include "rng.h"
#include "string_formatter.h"
#include "translations.h"

ter_furn_id::ter_furn_id() : ter( t_null ), furn( f_null ) { }

template<typename T>
void read_and_set_or_throw( JsonObject &jo, const std::string &member, T &target, bool required )
{
    T tmp;
    if( !jo.read( member, tmp ) ) {
        if( required ) {
            jo.throw_error( string_format( "%s required", member ) );
        }
    } else {
        target = tmp;
    }
}

static void load_forest_biome_component(
    JsonObject &jo, forest_biome_component &forest_biome_component, const bool overlay )
{
    read_and_set_or_throw<int>( jo, "chance", forest_biome_component.chance, !overlay );
    read_and_set_or_throw<int>( jo, "sequence", forest_biome_component.sequence, !overlay );
    read_and_set_or_throw<bool>( jo, "clear_types", forest_biome_component.clear_types, !overlay );

    if( forest_biome_component.clear_types ) {
        forest_biome_component.unfinalized_types.clear();
    }

    if( !jo.has_object( "types" ) ) {
        if( !overlay ) {
            jo.throw_error( "types required" );
        }
    } else {
        JsonObject feature_types_jo = jo.get_object( "types" );
        std::set<std::string> keys = feature_types_jo.get_member_names();
        for( const auto &key : keys ) {
            int weight = 0;
            if( key != "//" ) {
                if( feature_types_jo.read( key, weight ) ) {
                    forest_biome_component.unfinalized_types[key] = weight;
                }
            }
        }
    }
}

static void load_forest_biome_terrain_dependent_furniture( JsonObject &jo,
        forest_biome_terrain_dependent_furniture &forest_biome_terrain_dependent_furniture,
        const bool overlay )
{
    read_and_set_or_throw<int>( jo, "chance", forest_biome_terrain_dependent_furniture.chance,
                                !overlay );
    read_and_set_or_throw<bool>( jo, "clear_furniture",
                                 forest_biome_terrain_dependent_furniture.clear_furniture, !overlay );

    if( forest_biome_terrain_dependent_furniture.clear_furniture ) {
        forest_biome_terrain_dependent_furniture.unfinalized_furniture.clear();
    }

    if( !jo.has_object( "furniture" ) ) {
        if( !overlay ) {
            jo.throw_error( "furniture required" );
        }
    } else {
        JsonObject feature_types_jo = jo.get_object( "furniture" );
        std::set<std::string> keys = feature_types_jo.get_member_names();
        for( const auto &key : keys ) {
            int weight = 0;
            if( key != "//" ) {
                if( feature_types_jo.read( key, weight ) ) {
                    forest_biome_terrain_dependent_furniture.unfinalized_furniture[key] = weight;
                }
            }
        }
    }
}

static void load_forest_biome( JsonObject &jo, forest_biome &forest_biome, const bool overlay )
{
    read_and_set_or_throw<int>( jo, "sparseness_adjacency_factor",
                                forest_biome.sparseness_adjacency_factor, !overlay );
    read_and_set_or_throw<std::string>( jo, "item_group", forest_biome.item_group, !overlay );
    read_and_set_or_throw<int>( jo, "item_group_chance", forest_biome.item_group_chance, !overlay );
    read_and_set_or_throw<int>( jo, "item_spawn_iterations", forest_biome.item_spawn_iterations,
                                !overlay );
    read_and_set_or_throw<bool>( jo, "clear_components", forest_biome.clear_components, !overlay );
    read_and_set_or_throw<bool>( jo, "clear_groundcover", forest_biome.clear_groundcover, !overlay );
    read_and_set_or_throw<bool>( jo, "clear_terrain_furniture", forest_biome.clear_terrain_furniture,
                                 !overlay );

    if( forest_biome.clear_components ) {
        forest_biome.unfinalized_biome_components.clear();
    }

    if( !jo.has_object( "components" ) ) {
        if( !overlay ) {
            jo.throw_error( "components required" );
        }
    } else {
        JsonObject components_jo = jo.get_object( "components" );
        std::set<std::string> component_names = components_jo.get_member_names();
        for( const auto &name : component_names ) {
            if( name != "//" ) {
                JsonObject component_jo = components_jo.get_object( name );
                load_forest_biome_component( component_jo, forest_biome.unfinalized_biome_components[name],
                                             overlay );
            }
        }
    }

    if( forest_biome.clear_groundcover ) {
        forest_biome.unfinalized_groundcover.clear();
    }

    if( !jo.has_object( "groundcover" ) ) {
        if( !overlay ) {
            jo.throw_error( "groundcover required" );
        }
    } else {
        JsonObject groundcover_jo = jo.get_object( "groundcover" );
        std::set<std::string> keys = groundcover_jo.get_member_names();
        for( const auto &key : keys ) {
            int weight = 0;
            if( key != "//" ) {
                if( groundcover_jo.read( key, weight ) ) {
                    forest_biome.unfinalized_groundcover[key] = weight;
                }
            }
        }
    }

    if( !jo.has_object( "terrain_furniture" ) ) {
        if( !overlay ) {
            jo.throw_error( "terrain_furniture required" );
        }
    } else {
        JsonObject terrain_furnitures_jo = jo.get_object( "terrain_furniture" );
        std::set<std::string> terrain_furniture_names = terrain_furnitures_jo.get_member_names();
        for( const auto &name : terrain_furniture_names ) {
            if( name != "//" ) {
                JsonObject terrain_furniture_jo = terrain_furnitures_jo.get_object( name );
                load_forest_biome_terrain_dependent_furniture( terrain_furniture_jo,
                        forest_biome.unfinalized_terrain_dependent_furniture[name],
                        overlay );
            }
        }
    }
}

static void load_forest_mapgen_settings( JsonObject &jo,
        forest_mapgen_settings &forest_mapgen_settings,
        const bool strict,
        const bool overlay )
{
    if( !jo.has_object( "forest_mapgen_settings" ) ) {
        if( strict ) {
            jo.throw_error( "\"forest_mapgen_settings\": { ... } required for default" );
        }
    } else {
        JsonObject forest_biomes_list_jo = jo.get_object( "forest_mapgen_settings" );
        std::set<std::string> forest_biome_names = forest_biomes_list_jo.get_member_names();
        for( const auto &forest_biome_name : forest_biome_names ) {
            if( forest_biome_name != "//" ) {
                JsonObject forest_biome_jo = forest_biomes_list_jo.get_object( forest_biome_name );
                load_forest_biome( forest_biome_jo, forest_mapgen_settings.unfinalized_biomes[forest_biome_name],
                                   overlay );
            }
        }
    }
}

static void load_forest_trail_settings( JsonObject &jo,
                                        forest_trail_settings &forest_trail_settings,
                                        const bool strict, const bool overlay )
{
    if( !jo.has_object( "forest_trail_settings" ) ) {
        if( strict ) {
            jo.throw_error( "\"forest_trail_settings\": { ... } required for default" );
        }
    } else {
        JsonObject forest_trail_settings_jo = jo.get_object( "forest_trail_settings" );
        read_and_set_or_throw<int>( forest_trail_settings_jo, "chance", forest_trail_settings.chance,
                                    !overlay );
        read_and_set_or_throw<int>( forest_trail_settings_jo, "border_point_chance",
                                    forest_trail_settings.border_point_chance, !overlay );
        read_and_set_or_throw<int>( forest_trail_settings_jo, "minimum_forest_size",
                                    forest_trail_settings.minimum_forest_size, !overlay );
        read_and_set_or_throw<int>( forest_trail_settings_jo, "random_point_min",
                                    forest_trail_settings.random_point_min, !overlay );
        read_and_set_or_throw<int>( forest_trail_settings_jo, "random_point_max",
                                    forest_trail_settings.random_point_max, !overlay );
        read_and_set_or_throw<int>( forest_trail_settings_jo, "random_point_size_scalar",
                                    forest_trail_settings.random_point_size_scalar, !overlay );
        read_and_set_or_throw<int>( forest_trail_settings_jo, "trailhead_chance",
                                    forest_trail_settings.trailhead_chance, !overlay );
        read_and_set_or_throw<int>( forest_trail_settings_jo, "trailhead_road_distance",
                                    forest_trail_settings.trailhead_road_distance, !overlay );
        read_and_set_or_throw<int>( forest_trail_settings_jo, "trail_center_variance",
                                    forest_trail_settings.trail_center_variance, !overlay );
        read_and_set_or_throw<int>( forest_trail_settings_jo, "trail_width_offset_min",
                                    forest_trail_settings.trail_width_offset_min, !overlay );
        read_and_set_or_throw<int>( forest_trail_settings_jo, "trail_width_offset_max",
                                    forest_trail_settings.trail_width_offset_max, !overlay );
        read_and_set_or_throw<bool>( forest_trail_settings_jo, "clear_trail_terrain",
                                     forest_trail_settings.clear_trail_terrain, !overlay );

        if( forest_trail_settings.clear_trail_terrain ) {
            forest_trail_settings.unfinalized_trail_terrain.clear();
        }

        if( !forest_trail_settings_jo.has_object( "trail_terrain" ) ) {
            if( !overlay ) {
                forest_trail_settings_jo.throw_error( "trail_terrain required" );
            }
        } else {
            JsonObject trail_terrain_jo = forest_trail_settings_jo.get_object( "trail_terrain" );
            std::set<std::string> keys = trail_terrain_jo.get_member_names();
            for( const auto &key : keys ) {
                int weight = 0;
                if( key != "//" ) {
                    if( trail_terrain_jo.read( key, weight ) ) {
                        forest_trail_settings.unfinalized_trail_terrain[key] = weight;
                    }
                }
            }
        }
    }
}

static void load_overmap_feature_flag_settings( JsonObject &jo,
        overmap_feature_flag_settings &overmap_feature_flag_settings,
        const bool strict, const bool overlay )
{
    if( !jo.has_object( "overmap_feature_flag_settings" ) ) {
        if( strict ) {
            jo.throw_error( "\"overmap_feature_flag_settings\": { ... } required for default" );
        }
    } else {
        JsonObject overmap_feature_flag_settings_jo = jo.get_object( "overmap_feature_flag_settings" );
        read_and_set_or_throw<bool>( overmap_feature_flag_settings_jo, "clear_blacklist",
                                     overmap_feature_flag_settings.clear_blacklist, !overlay );
        read_and_set_or_throw<bool>( overmap_feature_flag_settings_jo, "clear_whitelist",
                                     overmap_feature_flag_settings.clear_whitelist, !overlay );

        if( overmap_feature_flag_settings.clear_blacklist ) {
            overmap_feature_flag_settings.blacklist.clear();
        }

        if( overmap_feature_flag_settings.clear_whitelist ) {
            overmap_feature_flag_settings.whitelist.clear();
        }

        if( !overmap_feature_flag_settings_jo.has_array( "blacklist" ) ) {
            if( !overlay ) {
                overmap_feature_flag_settings_jo.throw_error( "blacklist required" );
            }
        } else {
            JsonArray blacklist_ja = overmap_feature_flag_settings_jo.get_array( "blacklist" );
            while( blacklist_ja.has_more() ) {
                overmap_feature_flag_settings.blacklist.emplace( blacklist_ja.next_string() );
            }
        }

        if( !overmap_feature_flag_settings_jo.has_array( "whitelist" ) ) {
            if( !overlay ) {
                overmap_feature_flag_settings_jo.throw_error( "whitelist required" );
            }
        } else {
            JsonArray whitelist_ja = overmap_feature_flag_settings_jo.get_array( "whitelist" );
            while( whitelist_ja.has_more() ) {
                overmap_feature_flag_settings.whitelist.emplace( whitelist_ja.next_string() );
            }
        }
    }
}

static void load_overmap_forest_settings(
    JsonObject &jo, overmap_forest_settings &overmap_forest_settings, const bool strict,
    const bool overlay )
{
    if( !jo.has_object( "overmap_forest_settings" ) ) {
        if( strict ) {
            jo.throw_error( "\"overmap_forest_settings\": { ... } required for default" );
        }
    } else {
        JsonObject overmap_forest_settings_jo = jo.get_object( "overmap_forest_settings" );
        read_and_set_or_throw<double>( overmap_forest_settings_jo, "noise_threshold_forest",
                                       overmap_forest_settings.noise_threshold_forest, !overlay );
        read_and_set_or_throw<double>( overmap_forest_settings_jo, "noise_threshold_forest_thick",
                                       overmap_forest_settings.noise_threshold_forest_thick, !overlay );
        read_and_set_or_throw<double>( overmap_forest_settings_jo, "noise_threshold_swamp_adjacent_water",
                                       overmap_forest_settings.noise_threshold_swamp_adjacent_water, !overlay );
        read_and_set_or_throw<double>( overmap_forest_settings_jo, "noise_threshold_swamp_isolated",
                                       overmap_forest_settings.noise_threshold_swamp_isolated, !overlay );
        read_and_set_or_throw<int>( overmap_forest_settings_jo, "river_floodplain_buffer_distance_min",
                                    overmap_forest_settings.river_floodplain_buffer_distance_min, !overlay );
        read_and_set_or_throw<int>( overmap_forest_settings_jo, "river_floodplain_buffer_distance_max",
                                    overmap_forest_settings.river_floodplain_buffer_distance_max, !overlay );
    }
}

static void load_overmap_lake_settings( JsonObject &jo,
                                        overmap_lake_settings &overmap_lake_settings,
                                        const bool strict, const bool overlay )
{
    if( !jo.has_object( "overmap_lake_settings" ) ) {
        if( strict ) {
            jo.throw_error( "\"overmap_lake_settings\": { ... } required for default" );
        }
    } else {
        JsonObject overmap_lake_settings_jo = jo.get_object( "overmap_lake_settings" );
        read_and_set_or_throw<double>( overmap_lake_settings_jo, "noise_threshold_lake",
                                       overmap_lake_settings.noise_threshold_lake, !overlay );
        read_and_set_or_throw<int>( overmap_lake_settings_jo, "lake_size_min",
                                    overmap_lake_settings.lake_size_min, !overlay );

        if( !overmap_lake_settings_jo.has_array( "shore_extendable_overmap_terrain" ) ) {
            if( !overlay ) {
                overmap_lake_settings_jo.throw_error( "shore_extendable_overmap_terrain required" );
            }
        } else {
            const std::vector<std::string> from_json =
                overmap_lake_settings_jo.get_string_array( "shore_extendable_overmap_terrain" );
            overmap_lake_settings.unfinalized_shore_extendable_overmap_terrain.insert(
                overmap_lake_settings.unfinalized_shore_extendable_overmap_terrain.end(), from_json.begin(),
                from_json.end() );
        }

        if( !overmap_lake_settings_jo.has_array( "shore_extendable_overmap_terrain_aliases" ) ) {
            if( !overlay ) {
                overmap_lake_settings_jo.throw_error( "shore_extendable_overmap_terrain_aliases required" );
            }
        } else {
            JsonArray aliases_jarr =
                overmap_lake_settings_jo.get_array( "shore_extendable_overmap_terrain_aliases" );
            oter_str_id alias;
            while( aliases_jarr.has_more() ) {
                shore_extendable_overmap_terrain_alias alias;
                JsonObject jo = aliases_jarr.next_object();
                jo.read( "om_terrain", alias.overmap_terrain );
                jo.read( "alias", alias.alias );
                alias.match_type = jo.get_enum_value<ot_match_type>( jo.get_string( "om_terrain_match_type",
                                   "CONTAINS" ), ot_match_type::contains );
                overmap_lake_settings.shore_extendable_overmap_terrain_aliases.emplace_back( alias );
            }
        }
    }
}

void load_region_settings( JsonObject &jo )
{
    regional_settings new_region;
    if( ! jo.read( "id", new_region.id ) ) {
        jo.throw_error( "No 'id' field." );
    }
    bool strict = new_region.id == "default";
    if( ! jo.read( "default_oter", new_region.default_oter ) && strict ) {
        jo.throw_error( "default_oter required for default ( though it should probably remain 'field' )" );
    }
    if( ! jo.read( "river_scale", new_region.river_scale ) && strict ) {
        jo.throw_error( "river_scale required for default" );
    }
    if( jo.has_array( "default_groundcover" ) ) {
        JsonArray jia = jo.get_array( "default_groundcover" );
        new_region.default_groundcover_str.reset( new weighted_int_list<ter_str_id> );
        while( jia.has_more() ) {
            JsonArray inner = jia.next_array();
            if( new_region.default_groundcover_str->add( ter_str_id( inner.get_string( 0 ) ),
                    inner.get_int( 1 ) ) == nullptr ) {
                jo.throw_error( "'default_groundcover' must be a weighted list: an array of pairs [ \"id\", weight ]" );
            }
        }
    } else if( strict ) {
        jo.throw_error( "Weighted list 'default_groundcover' required for 'default'" );
    }

    if( ! jo.has_object( "field_coverage" ) ) {
        if( strict ) {
            jo.throw_error( "\"field_coverage\": { ... } required for default" );
        }
    } else {
        JsonObject pjo = jo.get_object( "field_coverage" );
        double tmpval = 0.0f;
        if( ! pjo.read( "percent_coverage", tmpval ) ) {
            pjo.throw_error( "field_coverage: percent_coverage required" );
        }
        new_region.field_coverage.mpercent_coverage = static_cast<int>( tmpval * 10000.0 );
        if( ! pjo.read( "default_ter", new_region.field_coverage.default_ter_str ) ) {
            pjo.throw_error( "field_coverage: default_ter required" );
        }
        tmpval = 0.0f;
        if( pjo.has_object( "other" ) ) {
            JsonObject opjo = pjo.get_object( "other" );
            std::set<std::string> keys = opjo.get_member_names();
            for( const auto &key : keys ) {
                tmpval = 0.0f;
                if( key != "//" ) {
                    if( opjo.read( key, tmpval ) ) {
                        new_region.field_coverage.percent_str[key] = tmpval;
                    }
                }
            }
        }
        if( pjo.read( "boost_chance", tmpval ) && tmpval != 0.0f ) {
            new_region.field_coverage.boost_chance = static_cast<int>( tmpval * 10000.0 );
            if( ! pjo.read( "boosted_percent_coverage", tmpval ) ) {
                pjo.throw_error( "boost_chance > 0 requires boosted_percent_coverage" );
            }
            new_region.field_coverage.boosted_mpercent_coverage = static_cast<int>( tmpval * 10000.0 );
            if( ! pjo.read( "boosted_other_percent", tmpval ) ) {
                pjo.throw_error( "boost_chance > 0 requires boosted_other_percent" );
            }
            new_region.field_coverage.boosted_other_mpercent = static_cast<int>( tmpval * 10000.0 );
            if( pjo.has_object( "boosted_other" ) ) {
                JsonObject opjo = pjo.get_object( "boosted_other" );
                std::set<std::string> keys = opjo.get_member_names();
                for( const auto &key : keys ) {
                    tmpval = 0.0f;
                    if( key != "//" ) {
                        if( opjo.read( key, tmpval ) ) {
                            new_region.field_coverage.boosted_percent_str[key] = tmpval;
                        }
                    }
                }
            } else {
                pjo.throw_error( "boost_chance > 0 requires boosted_other { ... }" );
            }
        }
    }

    load_forest_mapgen_settings( jo, new_region.forest_composition, strict, false );

    load_forest_trail_settings( jo, new_region.forest_trail, strict, false );

    if( ! jo.has_object( "map_extras" ) ) {
        if( strict ) {
            jo.throw_error( "\"map_extras\": { ... } required for default" );
        }
    } else {
        JsonObject pjo = jo.get_object( "map_extras" );

        std::set<std::string> zones = pjo.get_member_names();
        for( const auto &zone : zones ) {
            if( zone != "//" ) {
                JsonObject zjo = pjo.get_object( zone );
                map_extras extras( 0 );

                if( ! zjo.read( "chance", extras.chance ) && strict ) {
                    zjo.throw_error( "chance required for default" );
                }

                if( ! zjo.has_object( "extras" ) ) {
                    if( strict ) {
                        zjo.throw_error( "\"extras\": { ... } required for default" );
                    }
                } else {
                    JsonObject exjo = zjo.get_object( "extras" );

                    std::set<std::string> keys = exjo.get_member_names();
                    for( const auto &key : keys ) {
                        if( key != "//" ) {
                            extras.values.add( key, exjo.get_int( key, 0 ) );
                        }
                    }
                }

                new_region.region_extras[zone] = extras;
            }
        }
    }

    if( ! jo.has_object( "city" ) ) {
        if( strict ) {
            jo.throw_error( "\"city\": { ... } required for default" );
        }
    } else {
        JsonObject cjo = jo.get_object( "city" );
        if( ! cjo.read( "shop_radius", new_region.city_spec.shop_radius ) && strict ) {
            jo.throw_error( "city: shop_radius required for default" );
        }
        if( !cjo.read( "shop_sigma", new_region.city_spec.shop_sigma ) && strict ) {
            jo.throw_error( "city: shop_sigma required for default" );
        }
        if( ! cjo.read( "park_radius", new_region.city_spec.park_radius ) && strict ) {
            jo.throw_error( "city: park_radius required for default" );
        }
        if( !cjo.read( "park_sigma", new_region.city_spec.park_sigma ) && strict ) {
            jo.throw_error( "city: park_sigma required for default" );
        }
        if( ! cjo.read( "house_basement_chance", new_region.city_spec.house_basement_chance ) && strict ) {
            jo.throw_error( "city: house_basement_chance required for default" );
        }
        const auto load_building_types = [&jo, &cjo, strict]( const std::string & type,
        building_bin & dest ) {
            if( !cjo.has_object( type ) && strict ) {
                jo.throw_error( "city: \"" + type + "\": { ... } required for default" );
            } else {
                JsonObject wjo = cjo.get_object( type );
                std::set<std::string> keys = wjo.get_member_names();
                for( const auto &key : keys ) {
                    if( key != "//" ) {
                        if( wjo.has_int( key ) ) {
                            dest.add( overmap_special_id( key ), wjo.get_int( key ) );
                        }
                    }
                }
            }
        };
        load_building_types( "houses", new_region.city_spec.houses );
        load_building_types( "basements", new_region.city_spec.basements );
        load_building_types( "shops", new_region.city_spec.shops );
        load_building_types( "parks", new_region.city_spec.parks );
    }

    if( ! jo.has_object( "weather" ) ) {
        if( strict ) {
            jo.throw_error( "\"weather\": { ... } required for default" );
        }
    } else {
        JsonObject wjo = jo.get_object( "weather" );
        new_region.weather = weather_generator::load( wjo );
    }

    load_overmap_feature_flag_settings( jo, new_region.overmap_feature_flag, strict, false );

    load_overmap_forest_settings( jo, new_region.overmap_forest, strict, false );

    load_overmap_lake_settings( jo, new_region.overmap_lake, strict, false );

    region_settings_map[new_region.id] = new_region;
}

void reset_region_settings()
{
    region_settings_map.clear();
}

/*
 Entry point for parsing "region_overlay" json objects.
 Will loop through and apply the overlay to each of the overlay's regions.
 */
void load_region_overlay( JsonObject &jo )
{
    if( jo.has_array( "regions" ) ) {
        JsonArray regions = jo.get_array( "regions" );

        while( regions.has_more() ) {
            std::string regionid = regions.next_string();

            if( regionid == "all" ) {
                if( regions.size() != 1 ) {
                    jo.throw_error( "regions: More than one region is not allowed when \"all\" is used" );
                }

                for( auto &itr : region_settings_map ) {
                    apply_region_overlay( jo, itr.second );
                }
            } else {
                auto itr = region_settings_map.find( regionid );
                if( itr == region_settings_map.end() ) {
                    jo.throw_error( "region: " + regionid + " not found in region_settings_map" );
                } else {
                    apply_region_overlay( jo, itr->second );
                }
            }
        }
    } else {
        jo.throw_error( "\"regions\" is required and must be an array" );
    }
}

void apply_region_overlay( JsonObject &jo, regional_settings &region )
{
    jo.read( "default_oter", region.default_oter );
    jo.read( "river_scale", region.river_scale );
    if( jo.has_array( "default_groundcover" ) ) {
        JsonArray jia = jo.get_array( "default_groundcover" );
        region.default_groundcover_str.reset( new weighted_int_list<ter_str_id> );
        while( jia.has_more() ) {
            JsonArray inner = jia.next_array();
            if( region.default_groundcover_str->add( ter_str_id( inner.get_string( 0 ) ),
                    inner.get_int( 1 ) ) == nullptr ) {
                jo.throw_error( "'default_groundcover' must be a weighted list: an array of pairs [ \"id\", weight ]" );
            }
        }
    }

    JsonObject fieldjo = jo.get_object( "field_coverage" );
    double tmpval = 0.0f;
    if( fieldjo.read( "percent_coverage", tmpval ) ) {
        region.field_coverage.mpercent_coverage = static_cast<int>( tmpval * 10000.0 );
    }

    fieldjo.read( "default_ter", region.field_coverage.default_ter_str );

    JsonObject otherjo = fieldjo.get_object( "other" );
    std::set<std::string> keys = otherjo.get_member_names();
    for( const auto &key : keys ) {
        if( key != "//" ) {
            if( otherjo.read( key, tmpval ) ) {
                region.field_coverage.percent_str[key] = tmpval;
            }
        }
    }

    if( fieldjo.read( "boost_chance", tmpval ) ) {
        region.field_coverage.boost_chance = static_cast<int>( tmpval * 10000.0 );
    }
    if( fieldjo.read( "boosted_percent_coverage", tmpval ) ) {
        if( region.field_coverage.boost_chance > 0.0f && tmpval == 0.0f ) {
            fieldjo.throw_error( "boost_chance > 0 requires boosted_percent_coverage" );
        }

        region.field_coverage.boosted_mpercent_coverage = static_cast<int>( tmpval * 10000.0 );
    }

    if( fieldjo.read( "boosted_other_percent", tmpval ) ) {
        if( region.field_coverage.boost_chance > 0.0f && tmpval == 0.0f ) {
            fieldjo.throw_error( "boost_chance > 0 requires boosted_other_percent" );
        }

        region.field_coverage.boosted_other_mpercent = static_cast<int>( tmpval * 10000.0 );
    }

    JsonObject boostedjo = fieldjo.get_object( "boosted_other" );
    std::set<std::string> boostedkeys = boostedjo.get_member_names();
    for( const auto &key : boostedkeys ) {
        if( key != "//" ) {
            if( boostedjo.read( key, tmpval ) ) {
                region.field_coverage.boosted_percent_str[key] = tmpval;
            }
        }
    }

    if( region.field_coverage.boost_chance > 0.0f &&
        region.field_coverage.boosted_percent_str.empty() ) {
        fieldjo.throw_error( "boost_chance > 0 requires boosted_other { ... }" );
    }

    load_forest_mapgen_settings( jo, region.forest_composition, false, true );

    load_forest_trail_settings( jo, region.forest_trail, false, true );

    JsonObject mapextrajo = jo.get_object( "map_extras" );
    std::set<std::string> extrazones = mapextrajo.get_member_names();
    for( const auto &zone : extrazones ) {
        if( zone != "//" ) {
            JsonObject zonejo = mapextrajo.get_object( zone );

            int tmpval = 0;
            if( zonejo.read( "chance", tmpval ) ) {
                region.region_extras[zone].chance = tmpval;
            }

            JsonObject extrasjo = zonejo.get_object( "extras" );
            std::set<std::string> extrakeys = extrasjo.get_member_names();
            for( const auto &key : extrakeys ) {
                if( key != "//" ) {
                    region.region_extras[zone].values.add_or_replace( key, extrasjo.get_int( key ) );
                }
            }
        }
    }

    JsonObject cityjo = jo.get_object( "city" );

    cityjo.read( "shop_radius", region.city_spec.shop_radius );
    cityjo.read( "shop_sigma", region.city_spec.shop_sigma );
    cityjo.read( "park_radius", region.city_spec.park_radius );
    cityjo.read( "park_sigma", region.city_spec.park_sigma );
    cityjo.read( "house_basement_chance", region.city_spec.house_basement_chance );

    const auto load_building_types = [&cityjo]( const std::string & type, building_bin & dest ) {
        JsonObject typejo = cityjo.get_object( type );
        std::set<std::string> type_keys = typejo.get_member_names();
        for( const auto &key : type_keys ) {
            if( key != "//" && typejo.has_int( key ) ) {
                dest.add( overmap_special_id( key ), typejo.get_int( key ) );
            }
        }
    };
    load_building_types( "houses", region.city_spec.houses );
    load_building_types( "basements", region.city_spec.basements );
    load_building_types( "shops", region.city_spec.shops );
    load_building_types( "parks", region.city_spec.parks );

    load_overmap_feature_flag_settings( jo, region.overmap_feature_flag, false, true );

    load_overmap_forest_settings( jo, region.overmap_forest, false, true );

    load_overmap_lake_settings( jo, region.overmap_lake, false, true );
}

void groundcover_extra::finalize()   // FIXME: return bool for failure
{
    default_ter = ter_id( default_ter_str );

    ter_furn_id tf_id;
    int wtotal = 0;
    int btotal = 0;

    for( std::map<std::string, double>::const_iterator it = percent_str.begin();
         it != percent_str.end(); ++it ) {
        tf_id.ter = t_null;
        tf_id.furn = f_null;
        if( it->second < 0.0001 ) {
            continue;
        }
        const ter_str_id tid( it->first );
        const furn_str_id fid( it->first );
        if( tid.is_valid() ) {
            tf_id.ter = tid.id();
        } else if( fid.is_valid() ) {
            tf_id.furn = fid.id();
        } else {
            debugmsg( "No clue what '%s' is! No such terrain or furniture", it->first.c_str() );
            continue;
        }
        wtotal += static_cast<int>( it->second * 10000.0 );
        weightlist[ wtotal ] = tf_id;
    }

    for( std::map<std::string, double>::const_iterator it = boosted_percent_str.begin();
         it != boosted_percent_str.end(); ++it ) {
        tf_id.ter = t_null;
        tf_id.furn = f_null;
        if( it->second < 0.0001 ) {
            continue;
        }
        const ter_str_id tid( it->first );
        const furn_str_id fid( it->first );

        if( tid.is_valid() ) {
            tf_id.ter = tid.id();
        } else if( fid.is_valid() ) {
            tf_id.furn = fid.id();
        } else {
            debugmsg( "No clue what '%s' is! No such terrain or furniture", it->first.c_str() );
            continue;
        }
        btotal += static_cast<int>( it->second * 10000.0 );
        boosted_weightlist[ btotal ] = tf_id;
    }

    if( wtotal > 1000000 ) {
        debugmsg( "plant coverage total exceeds 100%%" );
    }
    if( btotal > 1000000 ) {
        debugmsg( "boosted plant coverage total exceeds 100%%" );
    }

    tf_id.furn = f_null;
    tf_id.ter = default_ter;
    weightlist[ 1000000 ] = tf_id;
    boosted_weightlist[ 1000000 ] = tf_id;

    percent_str.clear();
    boosted_percent_str.clear();
}

ter_furn_id groundcover_extra::pick( bool boosted ) const
{
    if( boosted ) {
        return boosted_weightlist.lower_bound( rng( 0, 1000000 ) )->second;
    }
    return weightlist.lower_bound( rng( 0, 1000000 ) )->second;
}

void forest_biome_component::finalize()
{
    for( const std::pair<std::string, int> &pr : unfinalized_types ) {
        ter_furn_id tf_id;
        tf_id.ter = t_null;
        tf_id.furn = f_null;
        const ter_str_id tid( pr.first );
        const furn_str_id fid( pr.first );
        if( tid.is_valid() ) {
            tf_id.ter = tid.id();
        } else if( fid.is_valid() ) {
            tf_id.furn = fid.id();
        } else {
            continue;
        }
        types.add( tf_id, pr.second );
    }
}

void forest_biome_terrain_dependent_furniture::finalize()
{
    for( const std::pair<std::string, int> &pr : unfinalized_furniture ) {
        const furn_str_id fid( pr.first );
        if( !fid.is_valid() ) {
            continue;
        }
        furniture.add( fid.id(), pr.second );
    }
}

ter_furn_id forest_biome::pick() const
{
    // Iterate through the biome components (which have already been put into sequence), roll for the
    // one_in chance that component contributes a feature, and if so pick that feature and return it.
    // If a given component does not roll as success, proceed to the next feature in sequence until
    // a feature is picked or none are picked, in which case an empty feature is returned.
    const ter_furn_id *result = nullptr;
    for( auto &pr : biome_components ) {
        if( one_in( pr.chance ) ) {
            result = pr.types.pick();
            break;
        }
    }

    if( result == nullptr ) {
        return ter_furn_id();
    }

    return *result;
}

void forest_biome::finalize()
{
    for( auto &pr : unfinalized_biome_components ) {
        pr.second.finalize();
        biome_components.push_back( pr.second );
    }

    std::sort( biome_components.begin(), biome_components.end(), []( const forest_biome_component & a,
    const forest_biome_component & b ) {
        return a.sequence < b.sequence;
    } );

    for( const std::pair<std::string, int> &pr : unfinalized_groundcover ) {
        const ter_str_id tid( pr.first );
        if( !tid.is_valid() ) {
            continue;
        }
        groundcover.add( tid.id(), pr.second );
    }

    for( auto &pr : unfinalized_terrain_dependent_furniture ) {
        pr.second.finalize();
        const ter_id t( pr.first );
        terrain_dependent_furniture[t] = pr.second;
    }
}

void forest_mapgen_settings::finalize()
{
    for( auto &pr : unfinalized_biomes ) {
        pr.second.finalize();
        const oter_id ot( pr.first );
        biomes[ot] = pr.second;
    }
}

void forest_trail_settings::finalize()
{
    for( const std::pair<const std::string, int> &pr : unfinalized_trail_terrain ) {
        const ter_str_id tid( pr.first );
        if( !tid.is_valid() ) {
            debugmsg( "Tried to add invalid terrain %s to forest_trail_settings trail_terrain.", tid.c_str() );
            continue;
        }
        trail_terrain.add( tid.id(), pr.second );
    }
}

void overmap_lake_settings::finalize()
{
    for( const std::string &oid : unfinalized_shore_extendable_overmap_terrain ) {
        const oter_str_id ot( oid );
        if( !ot.is_valid() ) {
            debugmsg( "Tried to add invalid overmap terrain %s to overmap_lake_settings shore_extendable_overmap_terrain.",
                      ot.c_str() );
            continue;
        }
        shore_extendable_overmap_terrain.emplace_back( ot.id() );
    }

    for( shore_extendable_overmap_terrain_alias &alias : shore_extendable_overmap_terrain_aliases ) {
        if( std::find( shore_extendable_overmap_terrain.begin(), shore_extendable_overmap_terrain.end(),
                       alias.alias ) == shore_extendable_overmap_terrain.end() ) {
            debugmsg( " %s was referenced as an alias in overmap_lake_settings shore_extendable_overmap_terrain_alises, but the value is not present in the shore_extendable_overmap_terrain.",
                      alias.alias.c_str() );
            continue;
        }
    }
}

void regional_settings::finalize()
{
    if( default_groundcover_str != nullptr ) {
        for( const auto &pr : *default_groundcover_str ) {
            default_groundcover.add( pr.obj.id(), pr.weight );
        }

        field_coverage.finalize();
        default_groundcover_str.reset();
        city_spec.finalize();
        forest_composition.finalize();
        forest_trail.finalize();
        overmap_lake.finalize();
        get_options().add_value( "DEFAULT_REGION", id, no_translation( id ) );
    }
}

void city_settings::finalize()
{
    houses.finalize();
    basements.finalize();
    shops.finalize();
    parks.finalize();
}

void building_bin::add( const overmap_special_id &building, int weight )
{
    if( finalized ) {
        debugmsg( "Tried to add special %s to a finalized building bin", building.c_str() );
        return;
    }

    unfinalized_buildings[ building ] = weight;
}

overmap_special_id building_bin::pick() const
{
    if( !finalized ) {
        debugmsg( "Tried to pick a special out of a non-finalized bin" );
        overmap_special_id null_special( "null" );
        return null_special;
    }

    return *buildings.pick();
}

void building_bin::clear()
{
    finalized = false;
    buildings.clear();
    unfinalized_buildings.clear();
    all.clear();
}

void building_bin::finalize()
{
    if( finalized ) {
        debugmsg( "Tried to finalize a finalized bin (that's a code-side error which can't be fixed with jsons)" );
        return;
    }
    if( unfinalized_buildings.empty() ) {
        debugmsg( "There must be at least one house, shop, and park for each regional map setting used." );
        return;
    }

    for( const std::pair<overmap_special_id, int> &pr : unfinalized_buildings ) {
        overmap_special_id current_id = pr.first;
        if( !current_id.is_valid() ) {
            // First, try to convert oter to special
            string_id<oter_type_t> converted_id( pr.first.str() );
            if( !converted_id.is_valid() ) {
                debugmsg( "Tried to add city building %s, but it is neither a special nor a terrain type",
                          pr.first.c_str() );
                continue;
            } else {
                all.emplace_back( pr.first.str() );
            }
            current_id = overmap_specials::create_building_from( converted_id );
        }
        buildings.add( current_id, pr.second );
    }

    finalized = true;
}
