#pragma once
#ifndef CATA_SRC_MAPGEN_FUNCTIONS_H
#define CATA_SRC_MAPGEN_FUNCTIONS_H

#include <functional>
#include <iosfwd>
#include <map>
#include <utility>

#include "coordinates.h"
#include "mapdata.h"
#include "type_id.h"

class map;
class mapgendata;
class mission;
struct mapgen_arguments;
struct mapgen_parameters;
struct point;
struct tripoint;

using mapgen_update_func = std::function<void( const tripoint_abs_omt &map_pos3, mission *miss )>;
class JsonObject;

/**
 * Calculates the coordinates of a rotated point.
 * Should match the `mapgen_*` rotation.
 */
tripoint rotate_point( const tripoint &p, int rotations );

int terrain_type_to_nesw_array( oter_id terrain_type, std::array<bool, 4> &array );

using building_gen_pointer = void ( * )( mapgendata & );
building_gen_pointer get_mapgen_cfunction( const std::string &ident );
ter_id grass_or_dirt();
ter_id clay_or_sand();

// helper functions for mapgen.cpp, so that we can avoid having a massive switch statement (sorta)
void mapgen_null( mapgendata &dat );
void mapgen_forest( mapgendata &dat );
void mapgen_river_center( mapgendata &dat );
void mapgen_river_curved_not( mapgendata &dat );
void mapgen_river_straight( mapgendata &dat );
void mapgen_river_curved( mapgendata &dat );
void mapgen_cave( mapgendata &dat );
void mapgen_cave_rat( mapgendata &dat );
void mapgen_rock( mapgendata &dat );
void mapgen_rock_partial( mapgendata &dat );
void mapgen_open_air( mapgendata &dat );
void mapgen_rift( mapgendata &dat );
void mapgen_hellmouth( mapgendata &dat );
void mapgen_subway( mapgendata &dat );
void mapgen_lake_shore( mapgendata &dat );
void mapgen_ocean_shore( mapgendata &dat );
void mapgen_ravine_edge( mapgendata &dat );

// Temporary wrappers
void mremove_trap( map *m, const point &, trap_id type );
void mtrap_set( map *m, const point &, trap_id type, bool avoid_creatures = false );
void madd_field( map *m, const point &, field_type_id type, int intensity );
void mremove_fields( map *m, const point & );

mapgen_update_func add_mapgen_update_func( const JsonObject &jo, bool &defer );
bool run_mapgen_update_func(
    const update_mapgen_id &, const tripoint_abs_omt &omt_pos, const mapgen_arguments &,
    mission *miss = nullptr, bool cancel_on_collision = true, bool mirror_horizontal = false,
    bool mirror_vertical = false, int rotation = 0 );
bool run_mapgen_update_func( const update_mapgen_id &, mapgendata &dat,
                             bool cancel_on_collision = true );
void set_queued_points();
bool run_mapgen_func( const std::string &mapgen_id, mapgendata &dat );
bool apply_construction_marker( const update_mapgen_id &update_mapgen_id,
                                const tripoint_abs_omt &omt_pos,
                                const mapgen_arguments &args, bool mirror_horizontal,
                                bool mirror_vertical, int rotation, bool apply );
std::pair<std::map<ter_id, int>, std::map<furn_id, int>>
        get_changed_ids_from_update(
            const update_mapgen_id &, const mapgen_arguments &,
            ter_id const &base_ter = t_dirt );
mapgen_parameters get_map_special_params( const std::string &mapgen_id );

void resolve_regional_terrain_and_furniture( const mapgendata &dat );

#endif // CATA_SRC_MAPGEN_FUNCTIONS_H
