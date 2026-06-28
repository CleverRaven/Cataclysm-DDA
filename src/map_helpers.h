#pragma once
#ifndef CATA_CLEAR_MAP_HELPER
#define CATA_CLEAR_MAP_HELPER

class map;

void wipe_map_terrain( map *target = nullptr );
void wipe_map_terrain_with_vision( map *target = nullptr, bool with_vision = true );
void clear_creatures();
void clear_npcs();
void clear_fields( int zlevel );
void clear_items( int zlevel );
void clear_zones();
void clear_basecamps();
void clear_map( int zmin = -2, int zmax = 0 );
void clear_map_without_vision( int zmin = -2, int zmax = 0 );
void clear_map_with_vision( int zmin = -2, int zmax = 0, bool with_vision = true );
void clear_radiation();
void clear_map_and_put_player_underground();
void clear_vehicles( map *target = nullptr );
void clear_overmaps();

#endif // CATA_CLEAR_MAP_HELPER
