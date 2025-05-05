#include "regional_settings.h"

#include <algorithm>
#include <cstddef>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include "debug.h"
#include "enum_conversions.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "map_extras.h"
#include "options.h"
#include "output.h"
#include "rng.h"
#include "string_formatter.h"
#include "translation.h"

class mapgendata;

ter_furn_id::ter_furn_id() : ter( ter_str_id::NULL_ID().id() ),
    furn( furn_str_id::NULL_ID().id() ) { }

template<typename T>
void read_and_set_or_throw( const JsonObject &jo, const std::string &member, T &target,
                            bool required )
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
    const JsonObject &jo, forest_biome_component &forest_biome_component, const bool overlay )
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
        for( const JsonMember member : jo.get_object( "types" ) ) {
            if( member.is_comment() ) {
                continue;
            }
            forest_biome_component.unfinalized_types[member.name()] = member.get_int();
        }
    }
}

static void load_forest_biome_terrain_dependent_furniture( const JsonObject &jo,
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
        for( const JsonMember member : jo.get_object( "furniture" ) ) {
            if( member.is_comment() ) {
                continue;
            }
            forest_biome_terrain_dependent_furniture.unfinalized_furniture[member.name()] = member.get_int();
        }
    }
}

static void load_forest_biome( const JsonObject &jo, forest_biome &forest_biome,
                               const bool overlay )
{
    read_and_set_or_throw<int>( jo, "sparseness_adjacency_factor",
                                forest_biome.sparseness_adjacency_factor, !overlay );
    read_and_set_or_throw( jo, "item_group", forest_biome.item_group, !overlay );
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
        for( const JsonMember member : jo.get_object( "components" ) ) {
            if( member.is_comment() ) {
                continue;
            }
            JsonObject component_jo = member.get_object();
            load_forest_biome_component( component_jo, forest_biome.unfinalized_biome_components[member.name()],
                                         overlay );
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
        for( const JsonMember member : jo.get_object( "groundcover" ) ) {
            if( member.is_comment() ) {
                continue;
            }
            forest_biome.unfinalized_groundcover[member.name()] = member.get_int();
        }
    }

    if( !jo.has_object( "terrain_furniture" ) ) {
        if( !overlay ) {
            jo.throw_error( "terrain_furniture required" );
        }
    } else {
        for( const JsonMember member : jo.get_object( "terrain_furniture" ) ) {
            if( member.is_comment() ) {
                continue;
            }
            JsonObject terrain_furniture_jo = member.get_object();
            load_forest_biome_terrain_dependent_furniture( terrain_furniture_jo,
                    forest_biome.unfinalized_terrain_dependent_furniture[member.name()], overlay );
        }
    }
}

static void load_forest_mapgen_settings( const JsonObject &jo,
        forest_mapgen_settings &forest_mapgen_settings,
        const bool strict,
        const bool overlay )
{
    if( !jo.has_object( "forest_mapgen_settings" ) ) {
        if( strict ) {
            jo.throw_error( "\"forest_mapgen_settings\": { … } required for default" );
        }
    } else {
        for( const JsonMember member : jo.get_object( "forest_mapgen_settings" ) ) {
            if( member.is_comment() ) {
                continue;
            }
            JsonObject forest_biome_jo = member.get_object();
            if( forest_biome_jo.has_array( "terrains" ) ) {
                JsonArray forest_biome_ter_keys = forest_biome_jo.get_array( "terrains" );
                if( forest_biome_ter_keys.empty() ) {
                    forest_biome_jo.throw_error( "Biome is not associated with any terrains." );
                }
                std::string default_ter_name = member.name();
                auto default_ter_it = forest_mapgen_settings.unfinalized_biomes.find( default_ter_name );
                if( default_ter_it == forest_mapgen_settings.unfinalized_biomes.end() ) {
                    if( overlay ) {
                        forest_biome_jo.throw_error( "Terrain " + default_ter_name +
                                                     " is not defined, cannot be used for forest biome." );
                    }
                    default_ter_it = forest_mapgen_settings.unfinalized_biomes.emplace( default_ter_name, forest_biome{} ).first;
                }
                forest_biome &default_ter = default_ter_it->second;
                load_forest_biome( forest_biome_jo, default_ter, overlay );
                for( size_t biome_ter_idx = 0; biome_ter_idx < forest_biome_ter_keys.size(); biome_ter_idx++ ) {
                    std::string biome_ter_name = forest_biome_ter_keys.get_string( biome_ter_idx );
                    if( biome_ter_name == default_ter_name ) {
                        continue;
                    }
                    forest_mapgen_settings.unfinalized_biomes.insert( std::pair<std::string, forest_biome>
                            ( forest_biome_ter_keys.get_string( biome_ter_idx ), default_ter ) );
                }
            } else {
                load_forest_biome( forest_biome_jo, forest_mapgen_settings.unfinalized_biomes[member.name()],
                                   overlay );
            }
        }
    }
}

static void load_forest_trail_settings( const JsonObject &jo,
                                        forest_trail_settings &forest_trail_settings,
                                        const bool strict, const bool overlay )
{
    if( !jo.has_object( "forest_trail_settings" ) ) {
        if( strict ) {
            jo.throw_error( "\"forest_trail_settings\": { … } required for default" );
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

        if( !forest_trail_settings_jo.has_object( "trailheads" ) ) {
            if( !overlay ) {
                forest_trail_settings_jo.throw_error( "trailheads required" );
            }
        } else {
            for( const JsonMember member : forest_trail_settings_jo.get_object( "trailheads" ) ) {
                if( member.is_comment() ) {
                    continue;
                }
                forest_trail_settings.trailheads.add( overmap_special_id( member.name() ), member.get_int() );
            }
        }
    }
}

static void load_overmap_feature_flag_settings( const JsonObject &jo,
        overmap_feature_flag_settings &overmap_feature_flag_settings,
        const bool strict, const bool overlay )
{
    if( !jo.has_object( "overmap_feature_flag_settings" ) ) {
        if( strict ) {
            jo.throw_error( "\"overmap_feature_flag_settings\": { … } required for default" );
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
            for( const std::string line : overmap_feature_flag_settings_jo.get_array( "blacklist" ) ) {
                overmap_feature_flag_settings.blacklist.emplace( line );
            }
        }

        if( !overmap_feature_flag_settings_jo.has_array( "whitelist" ) ) {
            if( !overlay ) {
                overmap_feature_flag_settings_jo.throw_error( "whitelist required" );
            }
        } else {
            for( const std::string line : overmap_feature_flag_settings_jo.get_array( "whitelist" ) ) {
                overmap_feature_flag_settings.whitelist.emplace( line );
            }
        }
    }
}

static void load_overmap_forest_settings(
    const JsonObject &jo, overmap_forest_settings &overmap_forest_settings, const bool strict,
    const bool overlay )
{
    if( !jo.has_object( "overmap_forest_settings" ) ) {
        if( strict ) {
            jo.throw_error( "\"overmap_forest_settings\": { … } required for default" );
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

static void load_overmap_ravine_settings(
    const JsonObject &jo, overmap_ravine_settings &overmap_ravine_settings, const bool strict,
    const bool overlay )
{
    if( !jo.has_object( "overmap_ravine_settings" ) ) {
        if( strict ) {
            jo.throw_error( "\"overmap_ravine_settings\": { … } required for default" );
        }
    } else {
        JsonObject overmap_ravine_settings_jo = jo.get_object( "overmap_ravine_settings" );
        read_and_set_or_throw<int>( overmap_ravine_settings_jo, "num_ravines",
                                    overmap_ravine_settings.num_ravines, !overlay );
        read_and_set_or_throw<int>( overmap_ravine_settings_jo, "ravine_range",
                                    overmap_ravine_settings.ravine_range, !overlay );
        read_and_set_or_throw<int>( overmap_ravine_settings_jo, "ravine_width",
                                    overmap_ravine_settings.ravine_width, !overlay );
        read_and_set_or_throw<int>( overmap_ravine_settings_jo, "ravine_depth",
                                    overmap_ravine_settings.ravine_depth, !overlay );
    }
}

static void load_overmap_connection_settings(
    const JsonObject &jo, overmap_connection_settings &overmap_connection_settings, const bool strict,
    const bool overlay )
{
    if( !jo.has_object( "overmap_connection_settings" ) ) {
        if( strict ) {
            jo.throw_error( "\"overmap_connection_settings\": { … } required for default" );
        }
    } else {
        JsonObject overmap_connection_settings_jo = jo.get_object( "overmap_connection_settings" );
        read_and_set_or_throw<overmap_connection_id>( overmap_connection_settings_jo, "trail_connection",
                overmap_connection_settings.trail_connection, !overlay );
        read_and_set_or_throw<overmap_connection_id>( overmap_connection_settings_jo, "sewer_connection",
                overmap_connection_settings.sewer_connection, !overlay );
        read_and_set_or_throw<overmap_connection_id>( overmap_connection_settings_jo, "subway_connection",
                overmap_connection_settings.subway_connection, !overlay );
        read_and_set_or_throw<overmap_connection_id>( overmap_connection_settings_jo, "rail_connection",
                overmap_connection_settings.rail_connection, !overlay );
        read_and_set_or_throw<overmap_connection_id>( overmap_connection_settings_jo,
                "intra_city_road_connection",
                overmap_connection_settings.intra_city_road_connection, !overlay );
        read_and_set_or_throw<overmap_connection_id>( overmap_connection_settings_jo,
                "inter_city_road_connection",
                overmap_connection_settings.inter_city_road_connection, !overlay );
    }
}

static void load_overmap_lake_settings( const JsonObject &jo,
                                        overmap_lake_settings &overmap_lake_settings,
                                        const bool strict, const bool overlay )
{
    if( !jo.has_object( "overmap_lake_settings" ) ) {
        if( strict ) {
            jo.throw_error( "\"overmap_lake_settings\": { … } required for default" );
        }
    } else {
        JsonObject overmap_lake_settings_jo = jo.get_object( "overmap_lake_settings" );
        read_and_set_or_throw<double>( overmap_lake_settings_jo, "noise_threshold_lake",
                                       overmap_lake_settings.noise_threshold_lake, !overlay );
        read_and_set_or_throw<int>( overmap_lake_settings_jo, "lake_size_min",
                                    overmap_lake_settings.lake_size_min, !overlay );
        read_and_set_or_throw<int>( overmap_lake_settings_jo, "lake_depth",
                                    overmap_lake_settings.lake_depth, !overlay );

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
            for( JsonObject alias_entry :
                 overmap_lake_settings_jo.get_array( "shore_extendable_overmap_terrain_aliases" ) ) {
                shore_extendable_overmap_terrain_alias alias;
                alias_entry.read( "om_terrain", alias.overmap_terrain );
                alias_entry.read( "alias", alias.alias );
                alias.match_type = alias_entry.get_enum_value<ot_match_type>( "om_terrain_match_type",
                                   ot_match_type::contains );
                overmap_lake_settings.shore_extendable_overmap_terrain_aliases.emplace_back( alias );
            }
        }
    }
}

static void load_overmap_ocean_settings( const JsonObject &jo,
        overmap_ocean_settings &overmap_ocean_settings,
        const bool strict, const bool overlay )
{
    if( !jo.has_object( "overmap_ocean_settings" ) ) {
        if( strict ) {
            jo.throw_error( "OVERMAP_PLACE_OCEANS set to true, but \"overmap_ocean_settings\" not defined in region_settings" );
        }
    } else {
        JsonObject overmap_ocean_settings_jo = jo.get_object( "overmap_ocean_settings" );
        read_and_set_or_throw<double>( overmap_ocean_settings_jo, "noise_threshold_ocean",
                                       overmap_ocean_settings.noise_threshold_ocean, !overlay );
        read_and_set_or_throw<int>( overmap_ocean_settings_jo, "ocean_size_min",
                                    overmap_ocean_settings.ocean_size_min, !overlay );
        read_and_set_or_throw<int>( overmap_ocean_settings_jo, "ocean_depth",
                                    overmap_ocean_settings.ocean_depth, !overlay );
        read_and_set_or_throw<int>( overmap_ocean_settings_jo, "ocean_start_north",
                                    overmap_ocean_settings.ocean_start_north, !overlay );
        read_and_set_or_throw<int>( overmap_ocean_settings_jo, "ocean_start_east",
                                    overmap_ocean_settings.ocean_start_east, !overlay );
        read_and_set_or_throw<int>( overmap_ocean_settings_jo, "ocean_start_west",
                                    overmap_ocean_settings.ocean_start_west, !overlay );
        read_and_set_or_throw<int>( overmap_ocean_settings_jo, "ocean_start_south",
                                    overmap_ocean_settings.ocean_start_south, !overlay );
        read_and_set_or_throw<int>( overmap_ocean_settings_jo, "sandy_beach_width",
                                    overmap_ocean_settings.sandy_beach_width, !overlay );
    }
}

static void load_region_terrain_and_furniture_settings( const JsonObject &jo,
        region_terrain_and_furniture_settings &region_terrain_and_furniture_settings,
        const bool strict, const bool overlay )
{
    if( !jo.has_object( "region_terrain_and_furniture" ) ) {
        if( strict ) {
            jo.throw_error( "\"region_terrain_and_furniture\": { … } required for default" );
        }
    } else {
        JsonObject region_terrain_and_furniture_settings_jo =
            jo.get_object( "region_terrain_and_furniture" );

        if( !region_terrain_and_furniture_settings_jo.has_object( "terrain" ) ) {
            if( !overlay ) {
                region_terrain_and_furniture_settings_jo.throw_error( "terrain required" );
            }
        } else {
            for( const JsonMember region : region_terrain_and_furniture_settings_jo.get_object( "terrain" ) ) {
                if( region.is_comment() ) {
                    continue;
                }
                for( const JsonMember terrain : region.get_object() ) {
                    if( terrain.is_comment() ) {
                        continue;
                    }
                    region_terrain_and_furniture_settings.unfinalized_terrain[region.name()][terrain.name()] =
                        terrain.get_int();
                }
            }
        }

        if( !region_terrain_and_furniture_settings_jo.has_object( "furniture" ) ) {
            if( !overlay ) {
                region_terrain_and_furniture_settings_jo.throw_error( "furniture required" );
            }
        } else {
            for( const JsonMember template_furniture :
                 region_terrain_and_furniture_settings_jo.get_object( "furniture" ) ) {
                if( template_furniture.is_comment() ) {
                    continue;
                }
                for( const JsonMember furniture : template_furniture.get_object() ) {
                    if( furniture.is_comment() ) {
                        continue;
                    }
                    region_terrain_and_furniture_settings.unfinalized_furniture[template_furniture.name()][furniture.name()]
                        = furniture.get_int();
                }
            }
        }
    }
}

void load_region_settings( const JsonObject &jo )
{
    regional_settings new_region;
    if( !jo.read( "id", new_region.id ) ) {
        jo.throw_error( "No 'id' field." );
    }
    bool strict = new_region.id == "default";
    mandatory( jo, false, "default_oter", new_region.default_oter );
    // So the data definition goes from z = OVERMAP_HEIGHT to z = OVERMAP_DEPTH
    std::reverse( new_region.default_oter.begin(), new_region.default_oter.end() );
    if( !jo.read( "river_scale", new_region.river_scale ) && strict ) {
        jo.throw_error( "river_scale required for default" );
    }
    if( jo.has_array( "default_groundcover" ) ) {
        new_region.default_groundcover_str.reset( new weighted_int_list<ter_str_id> );
        for( JsonArray inner : jo.get_array( "default_groundcover" ) ) {
            if( new_region.default_groundcover_str->add( ter_str_id( inner.get_string( 0 ) ),
                    inner.get_int( 1 ) ) == nullptr ) {
                jo.throw_error( "'default_groundcover' must be a weighted list: an array of pairs [ \"id\", weight ]" );
            }
        }
    } else if( strict ) {
        jo.throw_error( "Weighted list 'default_groundcover' required for 'default'" );
    }

    load_forest_mapgen_settings( jo, new_region.forest_composition, strict, false );

    load_forest_trail_settings( jo, new_region.forest_trail, strict, false );

    if( !jo.has_object( "map_extras" ) ) {
        if( strict ) {
            jo.throw_error( "\"map_extras\": { … } required for default" );
        }
    } else {
        for( const JsonMember zone : jo.get_object( "map_extras" ) ) {
            if( zone.is_comment() ) {
                continue;
            }
            JsonObject zjo = zone.get_object();
            map_extras extras( 0 );

            if( !zjo.read( "chance", extras.chance ) && strict ) {
                zjo.throw_error( "chance required for default" );
            }

            if( !zjo.has_object( "extras" ) ) {
                if( strict ) {
                    zjo.throw_error( "\"extras\": { … } required for default" );
                }
            } else {
                for( const JsonMember member : zjo.get_object( "extras" ) ) {
                    if( member.is_comment() ) {
                        continue;
                    }
                    extras.values.add( map_extra_id( member.name() ), member.get_int() );
                }
            }

            new_region.region_extras[zone.name()] = extras;
        }
    }

    if( !jo.has_object( "city" ) ) {
        if( strict ) {
            jo.throw_error( "\"city\": { … } required for default" );
        }
    } else {
        JsonObject cjo = jo.get_object( "city" );
        if( !cjo.read( "shop_radius", new_region.city_spec.shop_radius ) && strict ) {
            jo.throw_error( "city: shop_radius required for default" );
        }
        if( !cjo.read( "shop_sigma", new_region.city_spec.shop_sigma ) && strict ) {
            jo.throw_error( "city: shop_sigma required for default" );
        }
        if( !cjo.read( "park_radius", new_region.city_spec.park_radius ) && strict ) {
            jo.throw_error( "city: park_radius required for default" );
        }
        if( !cjo.read( "park_sigma", new_region.city_spec.park_sigma ) && strict ) {
            jo.throw_error( "city: park_sigma required for default" );
        }
        const auto load_building_types = [&jo, &cjo, strict]( const std::string & type,
        building_bin & dest ) {
            if( !cjo.has_object( type ) && strict ) {
                jo.throw_error( "city: \"" + type + "\": { … } required for default" );
            } else {
                for( const JsonMember member : cjo.get_object( type ) ) {
                    if( member.is_comment() ) {
                        continue;
                    }
                    dest.add( overmap_special_id( member.name() ), member.get_int() );
                }
            }
        };
        load_building_types( "houses", new_region.city_spec.houses );
        load_building_types( "shops", new_region.city_spec.shops );
        load_building_types( "parks", new_region.city_spec.parks );
    }

    // TODO: Support overwriting only some values in non default regions
    if( strict && !jo.has_object( "weather" ) ) {
        jo.throw_error( "\"weather\": { … } required for default" );
    } else {
        JsonObject wjo = jo.get_object( "weather" );
        new_region.weather.load( wjo, false );
    }

    load_overmap_feature_flag_settings( jo, new_region.overmap_feature_flag, strict, false );

    load_overmap_forest_settings( jo, new_region.overmap_forest, strict, false );

    load_overmap_lake_settings( jo, new_region.overmap_lake, strict, false );

    load_overmap_ocean_settings( jo, new_region.overmap_ocean, strict, false );

    load_overmap_ravine_settings( jo, new_region.overmap_ravine, strict, false );

    load_overmap_connection_settings( jo, new_region.overmap_connection, strict, false );

    load_region_terrain_and_furniture_settings( jo, new_region.region_terrain_and_furniture, strict,
            false );

    region_settings_map[new_region.id] = new_region;
}

void check_region_settings()
{
    for( const std::pair<const std::string, regional_settings> &p : region_settings_map ) {
        const std::string &region_name = p.first;
        const regional_settings &region = p.second;
        for( const std::pair<const std::string, map_extras> &p2 : region.region_extras ) {
            const std::string extras_name = p2.first;
            const map_extras &extras = p2.second;
            if( extras.chance == 0 ) {
                continue;
            }
            const weighted_int_list<map_extra_id> &values = extras.values;
            if( !values.is_valid() ) {
                if( values.empty() ) {
                    debugmsg( "Invalid map extras for region \"%s\", extras \"%s\".  "
                              "Extras have nonzero chance but no extras are listed.",
                              region_name, extras_name );
                } else {
                    std::string list_of_values =
                        enumerate_as_string( values,
                    []( const weighted_object<int, map_extra_id> &w ) {
                        return '"' + w.obj.str() + '"';
                    } );
                    debugmsg( "Invalid map extras for region \"%s\", extras \"%s\".  "
                              "Extras %s are listed, but all have zero weight.",
                              region_name, extras_name,
                              list_of_values );
                }
            }
        }
    }
}

void reset_region_settings()
{
    region_settings_map.clear();
}

/*
 Entry point for parsing "region_overlay" json objects.
 Will loop through and apply the overlay to each of the overlay's regions.
 */
void load_region_overlay( const JsonObject &jo )
{
    if( jo.has_array( "regions" ) ) {
        JsonArray regions = jo.get_array( "regions" );
        for( const std::string regionid : regions ) {
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

void apply_region_overlay( const JsonObject &jo, regional_settings &region )
{
    jo.read( "default_oter", region.default_oter );
    jo.read( "river_scale", region.river_scale );
    if( jo.has_array( "default_groundcover" ) ) {
        region.default_groundcover_str.reset( new weighted_int_list<ter_str_id> );
        for( JsonArray inner : jo.get_array( "default_groundcover" ) ) {
            if( region.default_groundcover_str->add( ter_str_id( inner.get_string( 0 ) ),
                    inner.get_int( 1 ) ) == nullptr ) {
                jo.throw_error( "'default_groundcover' must be a weighted list: an array of pairs [ \"id\", weight ]" );
            }
        }
    }

    load_forest_mapgen_settings( jo, region.forest_composition, false, true );

    load_forest_trail_settings( jo, region.forest_trail, false, true );

    for( const JsonMember zone : jo.get_object( "map_extras" ) ) {
        if( zone.is_comment() ) {
            continue;
        }
        JsonObject zonejo = zone.get_object();
        map_extras &extras = region.region_extras[zone.name()];

        int tmpval = 0;
        if( zonejo.read( "chance", tmpval ) ) {
            extras.chance = tmpval;
        }

        for( const JsonMember member : zonejo.get_object( "extras" ) ) {
            if( member.is_comment() ) {
                continue;
            }
            extras.values.add_or_replace( map_extra_id( member.name() ), member.get_int() );
        }

        // It's possible that all the entries of the weighted list have their
        // weights set to zero by this overlay.  In that case we want to reset
        // the chance to zero.
        if( !extras.values.is_valid() ) {
            extras.chance = 0;
        }
    }

    JsonObject cityjo = jo.get_object( "city" );

    cityjo.read( "shop_radius", region.city_spec.shop_radius );
    cityjo.read( "shop_sigma", region.city_spec.shop_sigma );
    cityjo.read( "park_radius", region.city_spec.park_radius );
    cityjo.read( "park_sigma", region.city_spec.park_sigma );

    const auto load_building_types = [&cityjo]( std::string_view type, building_bin & dest ) {
        for( const JsonMember member : cityjo.get_object( type ) ) {
            if( member.is_comment() ) {
                continue;
            }
            dest.add( overmap_special_id( member.name() ), member.get_int() );
        }
    };
    load_building_types( "houses", region.city_spec.houses );
    load_building_types( "shops", region.city_spec.shops );
    load_building_types( "parks", region.city_spec.parks );

    if( jo.has_object( "weather" ) ) {
        JsonObject wjo = jo.get_object( "weather" );
        region.weather.load( wjo, true );
    }

    load_overmap_feature_flag_settings( jo, region.overmap_feature_flag, false, true );

    load_overmap_forest_settings( jo, region.overmap_forest, false, true );

    load_overmap_lake_settings( jo, region.overmap_lake, false, true );

    load_overmap_ocean_settings( jo, region.overmap_ocean, false, true );

    load_overmap_ravine_settings( jo, region.overmap_ravine, false, true );

    load_overmap_connection_settings( jo, region.overmap_connection, false, true );

    load_region_terrain_and_furniture_settings( jo, region.region_terrain_and_furniture, false, true );
}

void groundcover_extra::finalize()   // FIXME: return bool for failure
{
    default_ter = ter_id( default_ter_str );

    ter_furn_id tf_id;
    int wtotal = 0;
    int btotal = 0;

    for( std::map<std::string, double>::const_iterator it = percent_str.begin();
         it != percent_str.end(); ++it ) {
        tf_id.ter = ter_str_id::NULL_ID().id();
        tf_id.furn = furn_str_id::NULL_ID();
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
            debugmsg( "No clue what '%s' is!  No such terrain or furniture", it->first.c_str() );
            continue;
        }
        wtotal += static_cast<int>( it->second * 10000.0 );
        weightlist[ wtotal ] = tf_id;
    }

    for( std::map<std::string, double>::const_iterator it = boosted_percent_str.begin();
         it != boosted_percent_str.end(); ++it ) {
        tf_id.ter = ter_str_id::NULL_ID().id();
        tf_id.furn = furn_str_id::NULL_ID();
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
            debugmsg( "No clue what '%s' is!  No such terrain or furniture", it->first.c_str() );
            continue;
        }
        btotal += static_cast<int>( it->second * 10000.0 );
        boosted_weightlist[ btotal ] = tf_id;
    }

    if( wtotal > 1000000 ) {
        std::stringstream ss;
        for( auto it = percent_str.begin(); it != percent_str.end(); ++it ) {
            if( it != percent_str.begin() ) {
                ss << '+';
            }
            ss << it->second;
        }
        debugmsg( "plant coverage total (%s=%de-4) exceeds 100%%", ss.str(), wtotal );
    }
    if( btotal > 1000000 ) {
        std::stringstream ss;
        for( auto it = boosted_percent_str.begin(); it != boosted_percent_str.end(); ++it ) {
            if( it != boosted_percent_str.begin() ) {
                ss << '+';
            }
            ss << it->second;
        }
        debugmsg( "boosted plant coverage total (%s=%de-4) exceeds 100%%", ss.str(), btotal );
    }

    tf_id.furn = furn_str_id::NULL_ID();
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
    for( const std::pair<const std::string, int> &pr : unfinalized_types ) {
        ter_furn_id tf_id;
        tf_id.ter = ter_str_id::NULL_ID().id();
        tf_id.furn = furn_str_id::NULL_ID();
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
    for( const std::pair<const std::string, int> &pr : unfinalized_furniture ) {
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
    for( const forest_biome_component &pr : biome_components ) {
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

    for( const std::pair<const std::string, int> &pr : unfinalized_groundcover ) {
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
        const oter_type_id ot( pr.first );
        biomes[ot] = pr.second;
    }
}

void forest_trail_settings::finalize()
{
    trailheads.finalize();
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
            debugmsg( " %s was referenced as an alias in overmap_lake_settings shore_extendable_overmap_terrain_aliases, but the value is not present in the shore_extendable_overmap_terrain.",
                      alias.alias.c_str() );
            continue;
        }
    }
}

map_extras map_extras::filtered_by( const mapgendata &dat ) const
{
    map_extras result( chance );
    for( const weighted_object<int, map_extra_id> &obj : values ) {
        const map_extra_id &extra_id = obj.obj;
        if( extra_id->is_valid_for( dat ) ) {
            result.values.add( extra_id, obj.weight );
        }
    }
    if( !values.empty() && result.values.empty() ) {
        // OMT is too tall / too deep for all map extras. Skip map extra generation.
        result.chance = 0;
    }
    return result;
}

void region_terrain_and_furniture_settings::finalize()
{
    for( auto const &template_pr : unfinalized_terrain ) {
        const ter_str_id template_tid( template_pr.first );
        if( !template_tid.is_valid() ) {
            debugmsg( "Tried to add invalid regional template terrain %s to region_terrain_and_furniture terrain.",
                      template_tid.c_str() );
            continue;
        }
        for( auto const &actual_pr : template_pr.second ) {
            const ter_str_id tid( actual_pr.first );
            if( !tid.is_valid() ) {
                debugmsg( "Tried to add invalid regional terrain %s to region_terrain_and_furniture terrain template %s.",
                          tid.c_str(), template_tid.c_str() );
                continue;
            }
            terrain[template_tid.id()].add( tid.id(), actual_pr.second );
        }
    }

    for( auto const &template_pr : unfinalized_furniture ) {
        const furn_str_id template_fid( template_pr.first );
        if( !template_fid.is_valid() ) {
            debugmsg( "Tried to add invalid regional template furniture %s to region_terrain_and_furniture furniture.",
                      template_fid.c_str() );
            continue;
        }
        for( auto const &actual_pr : template_pr.second ) {
            const furn_str_id fid( actual_pr.first );
            if( !fid.is_valid() ) {
                debugmsg( "Tried to add invalid regional furniture %s to region_terrain_and_furniture furniture template %s.",
                          fid.c_str(), template_fid.c_str() );
                continue;
            }
            furniture[template_fid.id()].add( fid.id(), actual_pr.second );
        }
    }
}

ter_id region_terrain_and_furniture_settings::resolve( const ter_id &tid ) const
{
    ter_id result = tid;
    auto region_list = terrain.find( result );
    while( region_list != terrain.end() ) {
        result = *region_list->second.pick();
        region_list = terrain.find( result );
    }
    return result;
}

furn_id region_terrain_and_furniture_settings::resolve( const furn_id &fid ) const
{
    furn_id result = fid;
    auto region_list = furniture.find( result );
    while( region_list != furniture.end() ) {
        result = *region_list->second.pick();
        region_list = furniture.find( result );
    }
    return result;
}

void regional_settings::finalize()
{
    if( default_groundcover_str != nullptr ) {
        for( const auto &pr : *default_groundcover_str ) {
            default_groundcover.add( pr.obj.id(), pr.weight );
        }

        default_groundcover_str.reset();
        city_spec.finalize();
        forest_composition.finalize();
        forest_trail.finalize();
        overmap_lake.finalize();
        region_terrain_and_furniture.finalize();
        get_options().add_value( "DEFAULT_REGION", id, no_translation( id ) );
        for( std::pair<const std::string, regional_settings> &p : region_settings_map ) {
            p.second.weather.sort_weather();
        }
    }
}

void city_settings::finalize()
{
    houses.finalize();
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
        debugmsg( "There must be at least one entry in this building bin." );
        return;
    }

    for( const std::pair<const overmap_special_id, int> &pr : unfinalized_buildings ) {
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
