#pragma once
#ifndef CATA_SRC_MAPGEN_FUNCTIONS_H
#define CATA_SRC_MAPGEN_FUNCTIONS_H

#include <array>
#include <functional>
#include <map>
#include <string>
#include <utility>

#include "coords_fwd.h"
#include "ret_val.h"
#include "type_id.h"

class map;
class mapgendata;
class mission;
class tinymap;
struct mapgen_arguments;
struct mapgen_parameters;

using mapgen_update_func = std::function<void( const tripoint_abs_omt &map_pos3, mission *miss )>;
class JsonObject;

/**
 * Calculates the coordinates of a rotated point.
 * Should match the `mapgen_*` rotation.
 */
tripoint_bub_ms rotate_point( const tripoint_bub_ms &p, int rotations );

int terrain_type_to_nesw_array( oter_id terrain_type, std::array<bool, 4> &array );

using building_gen_pointer = void ( * )( mapgendata & );
building_gen_pointer get_mapgen_cfunction( const std::string &ident );
ter_str_id grass_or_dirt();
ter_str_id clay_or_sand();

// helper functions for mapgen.cpp, so that we can avoid having a massive switch statement (sorta)
void mapgen_forest( mapgendata &dat );
void mapgen_river_curved_not( mapgendata &dat );
void mapgen_river_straight( mapgendata &dat );
void mapgen_river_curved( mapgendata &dat );
void mapgen_subway( mapgendata &dat );
void mapgen_lake_shore( mapgendata &dat );
void mapgen_ocean_shore( mapgendata &dat );
void mapgen_ravine_edge( mapgendata &dat );

// Temporary wrappers
void mremove_trap( map *m, const tripoint_bub_ms &, trap_id type );
void mtrap_set( map *m, const tripoint_bub_ms &, trap_id type, bool avoid_creatures = false );
void mtrap_set( tinymap *m, const point_omt_ms &, trap_id type, bool avoid_creatures = false );
void madd_field( map *m, const point_bub_ms &, field_type_id type, int intensity );
void mremove_fields( map *m, const tripoint_bub_ms & );

mapgen_update_func add_mapgen_update_func( const JsonObject &jo, bool &defer );
// Return contains the name of a colliding "vehicle" on failure.
ret_val<void> run_mapgen_update_func(
    const update_mapgen_id &, const tripoint_abs_omt &omt_pos, const mapgen_arguments &,
    mission *miss = nullptr, bool cancel_on_collision = true, bool mirror_horizontal = false,
    bool mirror_vertical = false, int rotation = 0 );
// Return contains the name of a colliding "vehicle" on failure.
ret_val<void> run_mapgen_update_func( const update_mapgen_id &, mapgendata &dat,
                                      bool cancel_on_collision = true );
void set_queued_points();
bool run_mapgen_func( const std::string &mapgen_id, mapgendata &dat );
bool apply_construction_marker( const update_mapgen_id &update_mapgen_id,
                                const tripoint_abs_omt &omt_pos,
                                const mapgen_arguments &args, bool mirror_horizontal,
                                bool mirror_vertical, int rotation, bool apply );
std::pair<std::map<ter_id, int>, std::map<furn_id, int>> get_changed_ids_from_update(
            const update_mapgen_id &, const mapgen_arguments &,
            ter_id const &base_ter = ter_str_id( "t_dirt" ).id() );
mapgen_parameters get_map_special_params( const std::string &mapgen_id );

void resolve_regional_terrain_and_furniture( const mapgendata &dat );

#endif // CATA_SRC_MAPGEN_FUNCTIONS_H
