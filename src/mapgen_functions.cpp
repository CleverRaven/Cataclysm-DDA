#include "mapgen.h"
#include "mapgen_functions.h"
#include "map_iterator.h"
#include "output.h"
#include "item_factory.h"
#include "line.h"
#include "mapgenformat.h"
#include "overmap.h"
#include "options.h"
#include "game.h"
#include "debug.h"
#include "scenario.h"
#include "translations.h"
#include "trap.h"
#include <array>
#include "vehicle_group.h"
#include "computer.h"
#include "mapdata.h"
#include "field.h"
#include <algorithm>
#include <iterator>

const mtype_id mon_ant_larva( "mon_ant_larva" );
const mtype_id mon_ant_queen( "mon_ant_queen" );
const mtype_id mon_bat( "mon_bat" );
const mtype_id mon_bee( "mon_bee" );
const mtype_id mon_beekeeper( "mon_beekeeper" );
const mtype_id mon_fungaloid_queen( "mon_fungaloid_queen" );
const mtype_id mon_fungaloid_seeder( "mon_fungaloid_seeder" );
const mtype_id mon_fungaloid_tower( "mon_fungaloid_tower" );
const mtype_id mon_jabberwock( "mon_jabberwock" );
const mtype_id mon_rat_king( "mon_rat_king" );
const mtype_id mon_sewer_rat( "mon_sewer_rat" );
const mtype_id mon_shia( "mon_shia" );
const mtype_id mon_spider_web( "mon_spider_web" );
const mtype_id mon_spider_widow_giant( "mon_spider_widow_giant" );
const mtype_id mon_spider_cellar_giant( "mon_spider_cellar_giant" );
const mtype_id mon_wasp( "mon_wasp" );
const mtype_id mon_zombie_jackson( "mon_zombie_jackson" );
const mtype_id mon_zombie( "mon_zombie" );

mapgendata::mapgendata( oter_id north, oter_id east, oter_id south, oter_id west,
                        oter_id northeast, oter_id southeast, oter_id southwest, oter_id northwest,
                        oter_id up, int z, const regional_settings *rsettings, map *mp ) :
    default_groundcover( t_null, 1, t_null )
{
    t_nesw[0] = north;
    t_nesw[1] = east;
    t_nesw[2] = south;
    t_nesw[3] = west;
    t_nesw[4] = northeast;
    t_nesw[5] = southeast;
    t_nesw[6] = southwest;
    t_nesw[7] = northwest;
    t_above = up;
    zlevel = z;
    n_fac = 0;
    e_fac = 0;
    s_fac = 0;
    w_fac = 0;
    ne_fac = 0;
    se_fac = 0;
    sw_fac = 0;
    nw_fac = 0;
    region = rsettings;
    m = mp;
    // making a copy so we can fudge values if desired
    default_groundcover.chance = region->default_groundcover.chance;
    default_groundcover.primary = region->default_groundcover.primary;
    default_groundcover.secondary = region->default_groundcover.secondary;

}

tripoint rotate_point( const tripoint &p, int rotations )
{
    if( p.x < 0 || p.x >= SEEX * 2 ||
        p.y < 0 || p.y >= SEEY * 2 ) {
        debugmsg( "Point out of range: %d,%d,%d", p.x, p.y, p.z );
        // Mapgen is vulnerable, don't supply invalid points, debugmsg is enough
        return tripoint( 0, 0, p.z );
    }

    rotations = rotations % 4;

    tripoint ret = p;
    switch( rotations ) {
        case 0:
            break;
        case 1:
            ret.x = p.y;
            ret.y = SEEX * 2 - 1 - p.x;
            break;
        case 2:
            ret.x = SEEX * 2 - 1 - p.x;
            ret.y = SEEY * 2 - 1 - p.y;
            break;
        case 3:
            ret.x = SEEY * 2 - 1 - p.y;
            ret.y = p.x;
            break;
    }

    return ret;
}

std::map<std::string, building_gen_pointer> mapgen_cfunction_map;

void init_mapgen_builtin_functions() {
    mapgen_cfunction_map.clear();
    mapgen_cfunction_map["null"]             = &mapgen_null;
    mapgen_cfunction_map["crater"]           = &mapgen_crater;
    mapgen_cfunction_map["field"]            = &mapgen_field;
    mapgen_cfunction_map["dirtlot"]          = &mapgen_dirtlot;
    mapgen_cfunction_map["forest"]           = &mapgen_forest_general;
    mapgen_cfunction_map["hive"]             = &mapgen_hive;
    mapgen_cfunction_map["spider_pit"]       = &mapgen_spider_pit;
    mapgen_cfunction_map["fungal_bloom"]     = &mapgen_fungal_bloom;
    mapgen_cfunction_map["fungal_tower"]     = &mapgen_fungal_tower;
    mapgen_cfunction_map["fungal_flowers"]   = &mapgen_fungal_flowers;
    mapgen_cfunction_map["road_straight"]    = &mapgen_road;
    mapgen_cfunction_map["road_curved"]      = &mapgen_road;
    mapgen_cfunction_map["road_end"]         = &mapgen_road;
    mapgen_cfunction_map["road_tee"]         = &mapgen_road;
    mapgen_cfunction_map["road_four_way"]    = &mapgen_road;
    mapgen_cfunction_map["field"]            = &mapgen_field;
    mapgen_cfunction_map["bridge"]           = &mapgen_bridge;
    mapgen_cfunction_map["highway"]          = &mapgen_highway;
    mapgen_cfunction_map["river_center"] = &mapgen_river_center;
    mapgen_cfunction_map["river_curved_not"] = &mapgen_river_curved_not;
    mapgen_cfunction_map["river_straight"]   = &mapgen_river_straight;
    mapgen_cfunction_map["river_curved"]     = &mapgen_river_curved;
    mapgen_cfunction_map["parking_lot"]      = &mapgen_parking_lot;
    mapgen_cfunction_map["s_gas"]      = &mapgen_gas_station;
    mapgen_cfunction_map["house_generic_boxy"]      = &mapgen_generic_house_boxy;
    mapgen_cfunction_map["house_generic_big_livingroom"]      = &mapgen_generic_house_big_livingroom;
    mapgen_cfunction_map["house_generic_center_hallway"]      = &mapgen_generic_house_center_hallway;
    mapgen_cfunction_map["church_new_england"]             = &mapgen_church_new_england;
    mapgen_cfunction_map["church_gothic"]             = &mapgen_church_gothic;
    mapgen_cfunction_map["s_pharm"]             = &mapgen_pharm;
    mapgen_cfunction_map["spider_pit"] = mapgen_spider_pit;
    mapgen_cfunction_map["s_sports"] = mapgen_s_sports;
    mapgen_cfunction_map["shelter"] = &mapgen_shelter;
    mapgen_cfunction_map["shelter_under"] = &mapgen_shelter_under;
    mapgen_cfunction_map["lmoe"] = &mapgen_lmoe;
    mapgen_cfunction_map["basement_generic_layout"] = &mapgen_basement_generic_layout; // empty, not bound
    mapgen_cfunction_map["basement_junk"] = &mapgen_basement_junk;
    /*
    mapgen_cfunction_map["basement_chemlab"] = &mapgen_basement_chemlab;
    mapgen_cfunction_map["basement_weed"] = &mapgen_basement_weed;
    mapgen_cfunction_map["basement_game"] = &mapgen_basement_game;
    */
    mapgen_cfunction_map["basement_spiders"] = &mapgen_basement_spiders;
    mapgen_cfunction_map["office_doctor"] = &mapgen_office_doctor;
    mapgen_cfunction_map["sub_station"] = &mapgen_sub_station;
    mapgen_cfunction_map["s_garage"] = &mapgen_s_garage;
//    mapgen_cfunction_map["farm"] = &mapgen_farm;
//    mapgen_cfunction_map["farm_field"] = &mapgen_farm_field;
    mapgen_cfunction_map["police"] = &mapgen_police;
    mapgen_cfunction_map["bank"] = &mapgen_bank;
    mapgen_cfunction_map["pawn"] = &mapgen_pawn;
    mapgen_cfunction_map["mil_surplus"] = &mapgen_mil_surplus;
    mapgen_cfunction_map["cave"] = &mapgen_cave;
    mapgen_cfunction_map["cave_rat"] = &mapgen_cave_rat;
    mapgen_cfunction_map["cavern"] = &mapgen_cavern;
    mapgen_cfunction_map["open_air"] = &mapgen_open_air;
    mapgen_cfunction_map["rift"] = &mapgen_rift;
    mapgen_cfunction_map["hellmouth"] = &mapgen_hellmouth;

    // New rock function - should be default, but isn't yet for compatibility reasons (old overmaps)
    mapgen_cfunction_map["empty_rock"] = &mapgen_rock;
    // Old rock behavior, for compatibility and near caverns and slime pits
    mapgen_cfunction_map["rock"] = &mapgen_rock_partial;

    mapgen_cfunction_map["subway_station"] = &mapgen_subway_station;

    mapgen_cfunction_map["subway_straight"]    = &mapgen_subway_straight;
    mapgen_cfunction_map["subway_curved"]      = &mapgen_subway_curved;
    // @todo Add a dedicated dead-end function. For now it copies the straight section above.
    mapgen_cfunction_map["subway_end"]         = &mapgen_subway_straight;
    mapgen_cfunction_map["subway_tee"]         = &mapgen_subway_tee;
    mapgen_cfunction_map["subway_four_way"]    = &mapgen_subway_four_way;

    mapgen_cfunction_map["sewer_straight"]    = &mapgen_sewer_straight;
    mapgen_cfunction_map["sewer_curved"]      = &mapgen_sewer_curved;
    // @todo Add a dedicated dead-end function. For now it copies the straight section above.
    mapgen_cfunction_map["sewer_end"]         = &mapgen_sewer_straight;
    mapgen_cfunction_map["sewer_tee"]         = &mapgen_sewer_tee;
    mapgen_cfunction_map["sewer_four_way"]    = &mapgen_sewer_four_way;

    mapgen_cfunction_map["ants_straight"]    = &mapgen_ants_straight;
    mapgen_cfunction_map["ants_curved"]      = &mapgen_ants_curved;
    // @todo Add a dedicated dead-end function. For now it copies the straight section above.
    mapgen_cfunction_map["ants_end"]         = &mapgen_ants_straight;
    mapgen_cfunction_map["ants_tee"]         = &mapgen_ants_tee;
    mapgen_cfunction_map["ants_four_way"]    = &mapgen_ants_four_way;
    mapgen_cfunction_map["ants_food"] = &mapgen_ants_food;
    mapgen_cfunction_map["ants_larvae"] = &mapgen_ants_larvae;
    mapgen_cfunction_map["ants_queen"] = &mapgen_ants_queen;
/* todo
    mapgen_cfunction_map["office_tower_1_entrance"] = &mapgen_office_tower_1_entrance;
    mapgen_cfunction_map["office_tower_1"] = &mapgen_office_tower_1;
    mapgen_cfunction_map["office_tower_b_entrance"] = &mapgen_office_tower_b_entrance;
    mapgen_cfunction_map["office_tower_b"] = &mapgen_office_tower_b;
    mapgen_cfunction_map["cathedral_1_entrance"] = &mapgen_cathedral_1_entrance;
    mapgen_cfunction_map["cathedral_1"] = &mapgen_cathedral_1;
    mapgen_cfunction_map["cathedral_b_entrance"] = &mapgen_cathedral_b_entrance;
    mapgen_cfunction_map["cathedral_b"] = &mapgen_cathedral_b;

    mapgen_cfunction_map["megastore_entrance"] = &mapgen_megastore_entrance;
    mapgen_cfunction_map["megastore"] = &mapgen_megastore;
    mapgen_cfunction_map["hospital_entrance"] = &mapgen_hospital_entrance;
    mapgen_cfunction_map["hospital"] = &mapgen_hospital;
    mapgen_cfunction_map["public_works_entrance"] = &mapgen_public_works_entrance;
    mapgen_cfunction_map["public_works"] = &mapgen_public_works;
    mapgen_cfunction_map["school_1"] = &mapgen_school_1;
    mapgen_cfunction_map["school_2"] = &mapgen_school_2;
    mapgen_cfunction_map["school_3"] = &mapgen_school_3;
    mapgen_cfunction_map["school_4"] = &mapgen_school_4;
    mapgen_cfunction_map["school_5"] = &mapgen_school_5;
    mapgen_cfunction_map["school_6"] = &mapgen_school_6;
    mapgen_cfunction_map["school_7"] = &mapgen_school_7;
    mapgen_cfunction_map["school_8"] = &mapgen_school_8;
    mapgen_cfunction_map["school_9"] = &mapgen_school_9;
    mapgen_cfunction_map["prison_1"] = &mapgen_prison_1;
    mapgen_cfunction_map["prison_2"] = &mapgen_prison_2;
    mapgen_cfunction_map["prison_3"] = &mapgen_prison_3;
    mapgen_cfunction_map["prison_4"] = &mapgen_prison_4;
    mapgen_cfunction_map["prison_5"] = &mapgen_prison_5;
    mapgen_cfunction_map["prison_6"] = &mapgen_prison_6;
    mapgen_cfunction_map["prison_7"] = &mapgen_prison_7;
    mapgen_cfunction_map["prison_8"] = &mapgen_prison_8;
    mapgen_cfunction_map["prison_9"] = &mapgen_prison_9;
    mapgen_cfunction_map["prison_b"] = &mapgen_prison_b;
    mapgen_cfunction_map["prison_b_entrance"] = &mapgen_prison_b_entrance;
    mapgen_cfunction_map["hotel_tower_1_1"] = &mapgen_hotel_tower_1_1;
    mapgen_cfunction_map["hotel_tower_1_2"] = &mapgen_hotel_tower_1_2;
    mapgen_cfunction_map["hotel_tower_1_3"] = &mapgen_hotel_tower_1_3;
    mapgen_cfunction_map["hotel_tower_1_4"] = &mapgen_hotel_tower_1_4;
    mapgen_cfunction_map["hotel_tower_1_5"] = &mapgen_hotel_tower_1_5;
    mapgen_cfunction_map["hotel_tower_1_6"] = &mapgen_hotel_tower_1_6;
    mapgen_cfunction_map["hotel_tower_1_7"] = &mapgen_hotel_tower_1_7;
    mapgen_cfunction_map["hotel_tower_1_8"] = &mapgen_hotel_tower_1_8;
    mapgen_cfunction_map["hotel_tower_1_9"] = &mapgen_hotel_tower_1_9;
    mapgen_cfunction_map["hotel_tower_b_1"] = &mapgen_hotel_tower_b_1;
    mapgen_cfunction_map["hotel_tower_b_2"] = &mapgen_hotel_tower_b_2;
    mapgen_cfunction_map["hotel_tower_b_3"] = &mapgen_hotel_tower_b_3;
    mapgen_cfunction_map["mansion_entrance"] = &mapgen_mansion_entrance;
    mapgen_cfunction_map["mansion"] = &mapgen_mansion;
    mapgen_cfunction_map["fema_entrance"] = &mapgen_fema_entrance;
    mapgen_cfunction_map["fema"] = &mapgen_fema;
    mapgen_cfunction_map["station_radio"] = &mapgen_station_radio;
    mapgen_cfunction_map["lab"] = &mapgen_lab;
    mapgen_cfunction_map["lab_stairs"] = &mapgen_lab_stairs;
    mapgen_cfunction_map["lab_core"] = &mapgen_lab_core;
    mapgen_cfunction_map["lab_finale"] = &mapgen_lab_finale;
    mapgen_cfunction_map["ice_lab"] = &mapgen_ice_lab;
    mapgen_cfunction_map["ice_lab_stairs"] = &mapgen_ice_lab_stairs;
    mapgen_cfunction_map["ice_lab_core"] = &mapgen_ice_lab_core;
    mapgen_cfunction_map["ice_lab_finale"] = &mapgen_ice_lab_finale;
    mapgen_cfunction_map["nuke_plant"] = &mapgen_nuke_plant;
    mapgen_cfunction_map["bunker"] = &mapgen_bunker;
    mapgen_cfunction_map["outpost"] = &mapgen_outpost;
    mapgen_cfunction_map["silo"] = &mapgen_silo;
    mapgen_cfunction_map["silo_finale"] = &mapgen_silo_finale;
    mapgen_cfunction_map["temple"] = &mapgen_temple;
    mapgen_cfunction_map["temple_stairs"] = &mapgen_temple_stairs;
    mapgen_cfunction_map["temple_finale"] = &mapgen_temple_finale;
    mapgen_cfunction_map["sewage_treatment"] = &mapgen_sewage_treatment;
    mapgen_cfunction_map["sewage_treatment_hub"] = &mapgen_sewage_treatment_hub;
    mapgen_cfunction_map["sewage_treatment_under"] = &mapgen_sewage_treatment_under;
    mapgen_cfunction_map["mine_entrance"] = &mapgen_mine_entrance;
    mapgen_cfunction_map["mine_shaft"] = &mapgen_mine_shaft;
    mapgen_cfunction_map["mine"] = &mapgen_mine;
    mapgen_cfunction_map["mine_down"] = &mapgen_mine_down;
    mapgen_cfunction_map["mine_finale"] = &mapgen_mine_finale;
    mapgen_cfunction_map["spiral_hub"] = &mapgen_spiral_hub;
    mapgen_cfunction_map["spiral"] = &mapgen_spiral;
    mapgen_cfunction_map["radio_tower"] = &mapgen_radio_tower;
    mapgen_cfunction_map["toxic_dump"] = &mapgen_toxic_dump;
    mapgen_cfunction_map["haz_sar_entrance"] = &mapgen_haz_sar_entrance;
    mapgen_cfunction_map["haz_sar"] = &mapgen_haz_sar;
    mapgen_cfunction_map["haz_sar_entrance_b1"] = &mapgen_haz_sar_entrance_b1;
    mapgen_cfunction_map["haz_sar_b1"] = &mapgen_haz_sar_b1;

    mapgen_cfunction_map["spider_pit_under"] = &mapgen_spider_pit_under;
    mapgen_cfunction_map["anthill"] = &mapgen_anthill;
    mapgen_cfunction_map["slimepit"] = &mapgen_slimepit;
    mapgen_cfunction_map["slimepit_down"] = &mapgen_slimepit_down;
    mapgen_cfunction_map["triffid_roots"] = &mapgen_triffid_roots;
    mapgen_cfunction_map["triffid_finale"] = &mapgen_triffid_finale;

*/
    mapgen_cfunction_map["tutorial"] = &mapgen_tutorial;
//
}

void mapgendata::set_dir(int dir_in, int val)
{
    switch (dir_in) {
    case 0:
        n_fac = val;
        break;
    case 1:
        e_fac = val;
        break;
    case 2:
        s_fac = val;
        break;
    case 3:
        w_fac = val;
        break;
    case 4:
        ne_fac = val;
        break;
    case 5:
        se_fac = val;
        break;
    case 6:
        sw_fac = val;
        break;
    case 7:
        nw_fac = val;
        break;
    default:
        debugmsg("Invalid direction for mapgendata::set_dir. dir_in = %d", dir_in);
        break;
    }
}

void mapgendata::fill(int val)
{
    n_fac = val;
    e_fac = val;
    s_fac = val;
    w_fac = val;
    ne_fac = val;
    se_fac = val;
    sw_fac = val;
    nw_fac = val;
}

int& mapgendata::dir(int dir_in)
{
    switch (dir_in) {
    case 0:
        return n_fac;
        break;
    case 1:
        return e_fac;
        break;
    case 2:
        return s_fac;
        break;
    case 3:
        return w_fac;
        break;
    case 4:
        return ne_fac;
        break;
    case 5:
        return se_fac;
        break;
    case 6:
        return sw_fac;
        break;
    case 7:
        return nw_fac;
        break;
    default:
        debugmsg("Invalid direction for mapgendata::set_dir. dir_in = %d", dir_in);
        //return something just so the compiler doesn't freak out. Not really correct, though.
        return n_fac;
        break;
    }
}

ter_id grass_or_dirt()
{
 if (one_in(4))
  return t_grass;
 return t_dirt;
}

ter_id dirt_or_pile()
{
 if (one_in(4))
  return t_dirtmound;
 return t_dirt;
}

void mapgendata::square_groundcover(const int x1, const int y1, const int x2, const int y2) {
    m->draw_square_ter( this->default_groundcover, x1, y1, x2, y2);
}
void mapgendata::fill_groundcover() {
    m->draw_fill_background( this->default_groundcover );
}
bool mapgendata::is_groundcover(const ter_id iid ) const {
    return this->default_groundcover.match( iid );
}

ter_id mapgendata::groundcover() {
    return this->default_groundcover.get();
}

void mapgen_rotate( map * m, oter_id terrain_type, bool north_is_down ) {
    const auto dir = north_is_down ? om_direction::opposite( terrain_type->dir ) : terrain_type->dir;
    m->rotate( static_cast<int>( dir ) );
}

#define autorotate(x) mapgen_rotate(m, terrain_type, x)
#define autorotate_down() mapgen_rotate(m, terrain_type, true)

/////////////////////////////////////////////////////////////////////////////////////////////////
///// builtin terrain-specific mapgen functions. big multi-overmap-tile terrains are located in
///// mapgen_functions_big.cpp

void mapgen_null(map *m, oter_id, mapgendata, int, float)
{
    debugmsg("Generating null terrain, please report this as a bug");
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            m->ter_set(i, j, t_null);
            m->set_radiation(i, j, 0);
        }
    }
}

void mapgen_crater(map *m, oter_id, mapgendata dat, int, float)
{
    for(int i = 0; i < 4; i++) {
        if(dat.t_nesw[i] != "crater") {
            dat.set_dir(i, 6);
        }
    }

    for (int i = 0; i < SEEX * 2; i++) {
       for (int j = 0; j < SEEY * 2; j++) {
           if (rng(0, dat.w_fac) <= i && rng(0, dat.e_fac) <= SEEX * 2 - 1 - i &&
               rng(0, dat.n_fac) <= j && rng(0, dat.s_fac) <= SEEX * 2 - 1 - j ) {
               m->ter_set(i, j, t_dirt);
               m->make_rubble( tripoint( i,  j, m->get_abs_sub().z ), f_rubble_rock, true);
               m->set_radiation(i, j, rng(0, 4) * rng(0, 2));
           } else {
               m->ter_set(i, j, dat.groundcover());
               m->set_radiation(i, j, rng(0, 2) * rng(0, 2) * rng(0, 2));
            }
        }
    }
    m->place_items("wreckage", 83, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
}

// todo: make void map::ter_or_furn_set(const int x, const int y, const ter_furn_id & tfid);
void ter_or_furn_set( map * m, const int x, const int y, const ter_furn_id & tfid ) {
    if ( tfid.ter != t_null ) {
        m->ter_set(x, y, tfid.ter );
    } else if ( tfid.furn != f_null ) {
        m->furn_set(x, y, tfid.furn );
    }
}

/*
 * Default above ground non forested 'blank' area; typically a grassy field with a scattering of shrubs,
 *  but changes according to dat->region
 */
void mapgen_field(map *m, oter_id, mapgendata dat, int turn, float)
{
    // random area of increased vegetation. Or lava / toxic sludge / etc
    const bool boosted_vegetation = ( dat.region->field_coverage.boost_chance > rng( 0, 1000000 ) );
    const int & mpercent_bush = ( boosted_vegetation ?
       dat.region->field_coverage.boosted_mpercent_coverage :
       dat.region->field_coverage.mpercent_coverage
    );

    ter_furn_id altbush = dat.region->field_coverage.pick( true ); // one dominant plant type ( for boosted_vegetation == true )

    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            m->ter_set(i, j, dat.groundcover() ); // default is
            if ( mpercent_bush > rng(0, 1000000) ) { // yay, a shrub ( or tombstone )
                if ( boosted_vegetation && dat.region->field_coverage.boosted_other_mpercent > rng(0, 1000000) ) {
                    // already chose the lucky terrain/furniture/plant/rock/etc
                    ter_or_furn_set(m, i, j, altbush );
                } else {
                    // pick from weighted list
                    ter_or_furn_set(m, i, j, dat.region->field_coverage.pick( false ) );
                }
            }
        }
    }

    m->place_items("field", 60, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, turn); // fixme: take 'rock' out and add as regional biome setting
}

void mapgen_dirtlot(map *m, oter_id, mapgendata, int, float)
{
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            m->ter_set(i, j, t_dirt);
            if (one_in(120)) {
                m->ter_set(i, j, t_pit_shallow);
            } else if (one_in(50)) {
                m->ter_set(i,j, t_grass);
            }
        }
    }
    int num_v = rng(0,1) * rng(0,2); // (0, 0, 0, 0, 1, 2) vehicles
    for(int v = 0; v < num_v; v++) {
        const tripoint vp( rng(0, 16) + 4, rng(0, 16) + 4, m->get_abs_sub().z );
        int theta = rng(0,3)*180 + one_in(3)*rng(0,89);
        if( !m->veh_at( vp ) ) {
            m->add_vehicle( vgroup_id("dirtlot"), vp, theta, -1, -1 );
        }
    }
}
// todo: more region_settings for forest biome
void mapgen_forest_general(map *m, oter_id terrain_type, mapgendata dat, int turn, float)
{
    if (terrain_type == "forest_thick") {
        dat.fill(8);
    } else if (terrain_type == "forest_water") {
        dat.fill(4);
    } else if (terrain_type == "forest") {
        dat.fill(0);
    }
    for (int i = 0; i < 4; i++) {
        if (dat.t_nesw[i] == "forest" || dat.t_nesw[i] == "forest_water") {
            dat.dir(i) += 14;
        } else if (dat.t_nesw[i] == "forest_thick") {
            dat.dir(i) += 18;
        }
    }
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            int forest_chance = 0, num = 0;
            if (j < dat.n_fac) {
                forest_chance += dat.n_fac - j;
                num++;
            }
            if (SEEX * 2 - 1 - i < dat.e_fac) {
                forest_chance += dat.e_fac - (SEEX * 2 - 1 - i);
                num++;
            }
            if (SEEY * 2 - 1 - j < dat.s_fac) {
                forest_chance += dat.s_fac - (SEEY * 2 - 1 - j);
                num++;
            }
            if (i < dat.w_fac) {
                forest_chance += dat.w_fac - i;
                num++;
            }
            if (num > 0) {
                forest_chance /= num;
            }
            int rn = rng(0, forest_chance);
            if ((forest_chance > 0 && rn > 13) || one_in(100 - forest_chance)) {
                std::array<std::pair<int, ter_id>, 15> tree_chances = {{
                        // todo: JSONize this array!
                        // Ensure that these one_in chances
                        // (besides the last) don't add up to more than 1 in 1
                        // Reserve the last one (1 in 1) for simple trees that fill up the rest.
                        { 250, t_tree_apple },
                        { 300, t_tree_pear },
                        { 300, t_tree_cherry },
                        { 350, t_tree_apricot },
                        { 350, t_tree_peach },
                        { 350, t_tree_plum },
                        { 256, t_tree_birch },
                        { 256, t_tree_willow },
                        { 256, t_tree_maple },
                        { 128, t_tree_deadpine },
                        { 128, t_tree_hickory_dead },
                        { 32, t_tree_hickory },
                        { 16, t_tree_pine },
                        { 16, t_tree_blackjack },
                        { 1, t_tree },
                    }};
                double earlier_chances = 0;
                // Remember the earlier chances to calculate the sliding errors
                for (size_t c = 0; c < tree_chances.size(); c++){
                    if (tree_chances[c].first == 1) {
                        // If something has chances of 1, just put it in and go on.
                        m->ter_set(i, j, tree_chances[c].second );
                        break;
                    }
                    else if( earlier_chances != 1 &&
                             (one_in_improved((1/(1 - earlier_chances))*tree_chances[c].first)) ){
                        // (1/(1 - earlier_chances)) is the sliding error. fixed here
                        m->ter_set(i, j, tree_chances[c].second );
                        break;
                    }
                    else {
                        earlier_chances += 1 / double(tree_chances[c].first);
                    }
                }
            } else if ((forest_chance > 0 && rn > 10) || one_in(100 - forest_chance)) {
                m->ter_set(i, j, t_tree_young);
            } else if ((forest_chance > 0 && rn >  9) || one_in(100 - forest_chance)) {
                if (one_in(250)) {
                    m->ter_set(i, j, t_shrub_blueberry);
                } else {
                    m->ter_set(i, j, t_underbrush);
                }
            } else {
                m->ter_set(i, j, dat.groundcover());
            }
        }
    }
    m->place_items("forest", 60, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, turn); // fixme: region settings

    if (terrain_type == "forest_water") {
        // Reset *_fac to handle where to place water
        for (int i = 0; i < 8; i++) {
            if (dat.t_nesw[i] == "forest_water") {
                dat.set_dir(i, 2);
            } else if (is_ot_type("river", dat.t_nesw[i])) {
                dat.set_dir(i, 3);
            } else if (dat.t_nesw[i] == "forest" || dat.t_nesw[i] == "forest_thick") {
                dat.set_dir(i, 1);
            } else {
                dat.set_dir(i, 0);
            }
        }
        int x = SEEX / 2 + rng(0, SEEX), y = SEEY / 2 + rng(0, SEEY);
        for (int i = 0; i < 20; i++) {
            if (x >= 0 && x < SEEX * 2 && y >= 0 && y < SEEY * 2) {
                if (m->ter(x, y) == t_swater_sh) {
                    m->ter_set(x, y, t_swater_dp);
                } else if ( dat.is_groundcover( m->ter(x, y) ) ||
                         m->ter(x, y) == t_underbrush) {
                    m->ter_set(x, y, t_swater_sh);
                }
                if (m->ter(x, y) == t_water_sh) {
                    m->ter_set(x, y, t_water_dp);
                } else if ( dat.is_groundcover( m->ter(x, y) ) ||
                         m->ter(x, y) == t_underbrush) {
                    m->ter_set(x, y, t_water_sh);
                }
            } else {
                i = 20;
            }
            x += rng(-2, 2);
            y += rng(-2, 2);
            if (x < 0 || x >= SEEX * 2) {
                x = SEEX / 2 + rng(0, SEEX);
            }
            if (y < 0 || y >= SEEY * 2) {
                y = SEEY / 2 + rng(0, SEEY);
            }
            int factor = dat.n_fac + (dat.ne_fac / 2) + (dat.nw_fac / 2);
            for (int j = 0; j < factor; j++) {
                int wx = rng(0, SEEX * 2 -1), wy = rng(0, SEEY - 1);
                if (dat.is_groundcover( m->ter(wx, wy) ) ||
                    m->ter(wx, wy) == t_underbrush) {
                    m->ter_set(wx, wy, t_swater_sh);
                }
            }
            factor = dat.e_fac + (dat.ne_fac / 2) + (dat.se_fac / 2);
            for (int j = 0; j < factor; j++) {
                int wx = rng(SEEX, SEEX * 2 - 1), wy = rng(0, SEEY * 2 - 1);
                if (dat.is_groundcover( m->ter(wx, wy) ) ||
                      m->ter(wx, wy) == t_underbrush) {
                    m->ter_set(wx, wy, t_water_sh);
                }
            }
            factor = dat.s_fac + (dat.se_fac / 2) + (dat.sw_fac / 2);
            for (int j = 0; j < factor; j++) {
                int wx = rng(0, SEEX * 2 - 1), wy = rng(SEEY, SEEY * 2 - 1);
                if (dat.is_groundcover( m->ter(wx, wy) ) ||
                      m->ter(wx, wy) == t_underbrush) {
                    m->ter_set(wx, wy, t_swater_sh);
                }
            }
            factor = dat.s_fac + (dat.se_fac / 2) + (dat.ne_fac / 2);
            for (int j = 0; j < factor; j++) {
                int wx = rng(0, SEEX * 2 - 1), wy = rng(SEEY, SEEY * 2 - 1);
                if (m->ter(wx, wy) == t_water_sh) {
                    m->furn_set(wx, wy, f_cattails);
                }
            }
            factor = dat.s_fac + (dat.se_fac / 2) + (dat.sw_fac / 2);
            for (int j = 0; j < factor; j++) {
                int wx = rng(0, SEEX * 2 - 1), wy = rng(SEEY, SEEY * 2 - 1);
                if (m->ter(wx, wy) == t_water_sh) {
                    m->furn_set(wx, wy, f_cattails);
                }
            }
            factor = dat.w_fac + (dat.nw_fac / 2) + (dat.sw_fac / 2);
            for (int j = 0; j < factor; j++) {
                int wx = rng(0, SEEX - 1), wy = rng(0, SEEY * 2 - 1);
                if (dat.is_groundcover( m->ter(wx, wy) ) ||
                      m->ter(wx, wy) == t_underbrush) {
                    m->ter_set(wx, wy, t_water_sh);
                }
            }
        }
        int rn = rng(0, 2) * rng(0, 1) + rng(0, 1);// Good chance of 0
        for (int i = 0; i < rn; i++) {
            x = rng(0, SEEX * 2 - 1);
            y = rng(0, SEEY * 2 - 1);
            madd_trap( m, x, y, tr_sinkhole);
            if (m->ter(x, y) != t_swater_sh && m->ter(x, y) != t_water_sh) {
                m->ter_set(x, y, dat.groundcover());
            }
        }
    }

    //1-2 per overmap, very bad day for low level characters
    if (one_in(10000)) {
        m->add_spawn(mon_jabberwock, 1, SEEX, SEEY); // fixme add to monster_group?
    }

    //Very rare easter egg, ~1 per 10 overmaps
    if (one_in(1000000)) {
        m->add_spawn(mon_shia, 1, SEEX, SEEY);
    }


    // One in 100 forests has a spider living in it :o
    if (one_in(100)) {
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEX * 2; j++) {
                if ((dat.is_groundcover( m->ter(i, j) ) ||
                     m->ter(i, j) == t_underbrush) && !one_in(3)) {
                    madd_field( m, i, j, fd_web, rng(1, 3));
                }
            }
        }
        m->ter_set( 12, 12, t_dirt );
        m->furn_set(12, 12, f_egg_sackws);
        m->remove_field({12, 12, m->get_abs_sub().z}, fd_web);
        m->add_spawn(mon_spider_web, rng(1, 2), SEEX, SEEY);
    }
}

void mapgen_hive(map *m, oter_id, mapgendata dat, int turn, float)
{
    // Start with a basic forest pattern
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            int rn = rng(0, 14);
            if (rn > 13) {
                m->ter_set(i, j, t_tree);
            } else if (rn > 11) {
                m->ter_set(i, j, t_tree_young);
            } else if (rn > 10) {
                m->ter_set(i, j, t_underbrush);
            } else {
                m->ter_set(i, j, dat.groundcover());
            }
        }
    }

    // j and i loop through appropriate hive-cell center squares
    const bool is_center = dat.t_nesw[0] == "hive" && dat.t_nesw[1] == "hive" &&
                           dat.t_nesw[2] == "hive" && dat.t_nesw[3] == "hive";
    for (int j = 5; j < SEEY * 2 - 5; j += 6) {
        for (int i = (j == 5 || j == 17 ? 3 : 6); i < SEEX * 2 - 5; i += 6) {
            if (!one_in(8)) {
                // Caps are always there
                m->ter_set(i    , j - 5, t_wax);
                m->ter_set(i    , j + 5, t_wax);
                for (int k = -2; k <= 2; k++) {
                    for (int l = -1; l <= 1; l++) {
                        m->ter_set(i + k, j + l, t_floor_wax);
                    }
                }
                m->add_spawn(mon_bee, 2, i, j);
                m->add_spawn(mon_beekeeper, 1, i, j);
                m->ter_set(i    , j - 3, t_floor_wax);
                m->ter_set(i    , j + 3, t_floor_wax);
                m->ter_set(i - 1, j - 2, t_floor_wax);
                m->ter_set(i    , j - 2, t_floor_wax);
                m->ter_set(i + 1, j - 2, t_floor_wax);
                m->ter_set(i - 1, j + 2, t_floor_wax);
                m->ter_set(i    , j + 2, t_floor_wax);
                m->ter_set(i + 1, j + 2, t_floor_wax);

                // Up to two of these get skipped; an entrance to the cell
                int skip1 = rng(0, 23);
                int skip2 = rng(0, 23);

                m->ter_set(i - 1, j - 4, t_wax);
                m->ter_set(i    , j - 4, t_wax);
                m->ter_set(i + 1, j - 4, t_wax);
                m->ter_set(i - 2, j - 3, t_wax);
                m->ter_set(i - 1, j - 3, t_wax);
                m->ter_set(i + 1, j - 3, t_wax);
                m->ter_set(i + 2, j - 3, t_wax);
                m->ter_set(i - 3, j - 2, t_wax);
                m->ter_set(i - 2, j - 2, t_wax);
                m->ter_set(i + 2, j - 2, t_wax);
                m->ter_set(i + 3, j - 2, t_wax);
                m->ter_set(i - 3, j - 1, t_wax);
                m->ter_set(i - 3, j    , t_wax);
                m->ter_set(i - 3, j - 1, t_wax);
                m->ter_set(i - 3, j + 1, t_wax);
                m->ter_set(i - 3, j    , t_wax);
                m->ter_set(i - 3, j + 1, t_wax);
                m->ter_set(i - 2, j + 3, t_wax);
                m->ter_set(i - 1, j + 3, t_wax);
                m->ter_set(i + 1, j + 3, t_wax);
                m->ter_set(i + 2, j + 3, t_wax);
                m->ter_set(i - 1, j + 4, t_wax);
                m->ter_set(i    , j + 4, t_wax);
                m->ter_set(i + 1, j + 4, t_wax);

                if (skip1 ==  0 || skip2 ==  0)
                    m->ter_set(i - 1, j - 4, t_floor_wax);
                if (skip1 ==  1 || skip2 ==  1)
                    m->ter_set(i    , j - 4, t_floor_wax);
                if (skip1 ==  2 || skip2 ==  2)
                    m->ter_set(i + 1, j - 4, t_floor_wax);
                if (skip1 ==  3 || skip2 ==  3)
                    m->ter_set(i - 2, j - 3, t_floor_wax);
                if (skip1 ==  4 || skip2 ==  4)
                    m->ter_set(i - 1, j - 3, t_floor_wax);
                if (skip1 ==  5 || skip2 ==  5)
                    m->ter_set(i + 1, j - 3, t_floor_wax);
                if (skip1 ==  6 || skip2 ==  6)
                    m->ter_set(i + 2, j - 3, t_floor_wax);
                if (skip1 ==  7 || skip2 ==  7)
                    m->ter_set(i - 3, j - 2, t_floor_wax);
                if (skip1 ==  8 || skip2 ==  8)
                    m->ter_set(i - 2, j - 2, t_floor_wax);
                if (skip1 ==  9 || skip2 ==  9)
                    m->ter_set(i + 2, j - 2, t_floor_wax);
                if (skip1 == 10 || skip2 == 10)
                    m->ter_set(i + 3, j - 2, t_floor_wax);
                if (skip1 == 11 || skip2 == 11)
                    m->ter_set(i - 3, j - 1, t_floor_wax);
                if (skip1 == 12 || skip2 == 12)
                    m->ter_set(i - 3, j    , t_floor_wax);
                if (skip1 == 13 || skip2 == 13)
                    m->ter_set(i - 3, j - 1, t_floor_wax);
                if (skip1 == 14 || skip2 == 14)
                    m->ter_set(i - 3, j + 1, t_floor_wax);
                if (skip1 == 15 || skip2 == 15)
                    m->ter_set(i - 3, j    , t_floor_wax);
                if (skip1 == 16 || skip2 == 16)
                    m->ter_set(i - 3, j + 1, t_floor_wax);
                if (skip1 == 17 || skip2 == 17)
                    m->ter_set(i - 2, j + 3, t_floor_wax);
                if (skip1 == 18 || skip2 == 18)
                    m->ter_set(i - 1, j + 3, t_floor_wax);
                if (skip1 == 19 || skip2 == 19)
                    m->ter_set(i + 1, j + 3, t_floor_wax);
                if (skip1 == 20 || skip2 == 20)
                    m->ter_set(i + 2, j + 3, t_floor_wax);
                if (skip1 == 21 || skip2 == 21)
                    m->ter_set(i - 1, j + 4, t_floor_wax);
                if (skip1 == 22 || skip2 == 22)
                    m->ter_set(i    , j + 4, t_floor_wax);
                if (skip1 == 23 || skip2 == 23)
                    m->ter_set(i + 1, j + 4, t_floor_wax);

                if( is_center ) {
                    m->place_items("hive_center", 90, i - 2, j - 2, i + 2, j + 2, false, turn);
                } else {
                    m->place_items("hive", 80, i - 2, j - 2, i + 2, j + 2, false, turn);
                }
            }
        }
    }

    if( is_center ) {
        m->place_npc( SEEX, SEEY, "apis" );
    }
}

void mapgen_spider_pit(map *m, oter_id, mapgendata dat, int turn, float)
{
    // First generate a forest
    dat.fill(4);
    for (int i = 0; i < 4; i++) {
        if (dat.t_nesw[i] == "forest" || dat.t_nesw[i] == "forest_water") {
            dat.dir(i) += 14;
        } else if (dat.t_nesw[i] == "forest_thick") {
            dat.dir(i) += 18;
        }
    }
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            int forest_chance = 0, num = 0;
            if (j < dat.n_fac) {
                forest_chance += dat.n_fac - j;
                num++;
            }
            if (SEEX * 2 - 1 - i < dat.e_fac) {
                forest_chance += dat.e_fac - (SEEX * 2 - 1 - i);
                num++;
            }
            if (SEEY * 2 - 1 - j < dat.s_fac) {
                forest_chance += dat.s_fac - (SEEX * 2 - 1 - j);
                num++;
            }
            if (i < dat.w_fac) {
                forest_chance += dat.w_fac - i;
                num++;
            }
            if (num > 0) {
                forest_chance /= num;
            }
            int rn = rng(0, forest_chance);
            if ((forest_chance > 0 && rn > 13) || one_in(100 - forest_chance)) {
                m->ter_set(i, j, t_tree);
            } else if ((forest_chance > 0 && rn > 10) || one_in(100 - forest_chance)) {
                m->ter_set(i, j, t_tree_young);
            } else if ((forest_chance > 0 && rn >  9) || one_in(100 - forest_chance)) {
                m->ter_set(i, j, t_underbrush);
            } else {
                m->ter_set(i, j, dat.groundcover());
            }
        }
    }
    m->place_items("forest", 60, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, turn);
    // Next, place webs and sinkholes
    for (int i = 0; i < 4; i++) {
        int x = rng(3, SEEX * 2 - 4), y = rng(3, SEEY * 2 - 4);
        if (i == 0)
            m->ter_set(x, y, t_slope_down);
        else {
            m->ter_set(x, y, dat.groundcover());
            madd_trap( m, x, y, tr_sinkhole);
        }
        for (int x1 = x - 3; x1 <= x + 3; x1++) {
            for (int y1 = y - 3; y1 <= y + 3; y1++) {
                madd_field( m, x1, y1, fd_web, rng(2, 3));
                if (m->ter(x1, y1) != t_slope_down)
                    m->ter_set(x1, y1, t_dirt);
            }
        }
    }
}

void mapgen_fungal_bloom(map *m, oter_id, mapgendata dat, int, float)
{
    (void)dat;
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            if (one_in(rl_dist(i, j, 12, 12) * 4)) {
                m->ter_set(i, j, t_marloss);
            } else if (one_in(10)) {
                if (one_in(3)) {
                    m->ter_set(i, j, t_tree_fungal);
                } else {
                    m->ter_set(i, j, t_tree_fungal_young);
                }

            } else if (one_in(5)) {
                m->ter_set(i, j, t_shrub_fungal);
            } else if (one_in(10)) {
                m->ter_set(i, j, t_fungus_mound);
            } else {
                m->ter_set(i, j, t_fungus);
            }
        }
    }
    square(m, t_fungus, SEEX - 2, SEEY - 2, SEEX + 2, SEEY + 2);
    m->add_spawn(mon_fungaloid_queen, 1, 12, 12);
}

void mapgen_fungal_tower(map *m, oter_id, mapgendata dat, int, float)
{
    (void)dat;
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            if (one_in(8)) {
                if (one_in(3)) {
                    m->ter_set(i, j, t_tree_fungal);
                } else {
                    m->ter_set(i, j, t_tree_fungal_young);
                }

            } else if (one_in(10)) {
                m->ter_set(i, j, t_fungus_mound);
            } else {
                m->ter_set(i, j, t_fungus);
            }
        }
    }
    square(m, t_fungus, SEEX - 2, SEEY - 2, SEEX + 2, SEEY + 2);
    m->add_spawn(mon_fungaloid_tower, 1, 12, 12);
}

void mapgen_fungal_flowers(map *m, oter_id, mapgendata dat, int, float)
{
    (void)dat;
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            if (one_in(rl_dist(i, j, 12, 12) * 6)) {
                m->ter_set(i, j, t_fungus);
                m->furn_set(i, j, f_flower_marloss);
            } else if (one_in(10)) {
                if (one_in(3)) {
                    m->ter_set(i, j, t_fungus_mound);
                } else {
                    m->ter_set(i, j, t_tree_fungal_young);
                }

            } else if (one_in(5)) {
                m->ter_set(i, j, t_fungus);
                m->furn_set(i, j, f_flower_fungal);
            } else if (one_in(10)) {
                m->ter_set(i, j, t_shrub_fungal);
            } else {
                m->ter_set(i, j, t_fungus);
            }
        }
    }
    square(m, t_fungus, SEEX - 2, SEEY - 2, SEEX + 2, SEEY + 2);
    m->add_spawn(mon_fungaloid_seeder, 1, 12, 12);
}

int terrain_type_with_suffix_to_nesw_array( oter_id terrain_type, bool array[4] ) {
    // extract the suffix from the terrain type name
    const std::string &sid = terrain_type.id().str();
    std::string suffix = sid.substr( sid.find_last_of( "_" ) + 1 );
    // non-"_end" end tiles have _north _east _south _west, all of which contain "t"
    if( suffix.find( "t" ) != std::string::npos ) {
        suffix = suffix.substr( 0, 1 );
        suffix = ( suffix == "n" ) ? "s" : // and they are all backwards :(
                 ( suffix == "e" ) ? "w" :
                 ( suffix == "s" ) ? "n" :
                 ( suffix == "w" ) ? "e" : "" ;
    }
    // manhole exception
    if( suffix == "manhole" ) {
        suffix = "nesw";
    }
    // count and mark which directions the road goes
    int num_dirs = 0;
    num_dirs += ( array[0] = ( suffix.find( "n" ) != std::string::npos ) );
    num_dirs += ( array[1] = ( suffix.find( "e" ) != std::string::npos ) );
    num_dirs += ( array[2] = ( suffix.find( "s" ) != std::string::npos ) );
    num_dirs += ( array[3] = ( suffix.find( "w" ) != std::string::npos ) );
    return num_dirs;
}

// perform dist counterclockwise rotations on a nesw or neswx array
template<typename T>
void nesw_array_rotate( T *array, size_t len, size_t dist ) {
    if( len == 4 ) {
        while( dist-- ) {
            T temp = array[0];
            array[0] = array[1];
            array[1] = array[2];
            array[2] = array[3];
            array[3] = temp;
        }
    } else {
        while( dist-- ) {
            // N E S W NE SE SW NW
            T temp = array[0];
            array[0] = array[4];
            array[4] = array[1];
            array[1] = array[5];
            array[5] = array[2];
            array[2] = array[6];
            array[6] = array[3];
            array[3] = array[7];
            array[7] = temp;
        }
    }
}

// take x/y coords in a map and rotate them counterclockwise around the center
void coord_rotate_cw( int &x, int &y, int rot ) {
    for( ; rot--; ) {
        int temp = y;
        y = x;
        x = ( SEEY * 2 - 1 ) - temp;
    }
}

bool compare_neswx( bool *a1, std::initializer_list<int> a2 ) {
    return std::equal( std::begin( a2 ), std::end( a2 ), a1 );
}

// mapgen_road replaces previous mapgen_road_straight _end _curved _tee _four_way
void mapgen_road( map *m, oter_id terrain_type, mapgendata dat, int turn, float density )
{
    // start by filling the whole map with grass/dirt/etc
    dat.fill_groundcover();

    // which and how many neighbors have sidewalks?
    bool sidewalks_neswx[8] = {};
    int neighbor_sidewalks = 0;
    for( int dir = 0; dir < 8; dir++ ) { // N E S W NE SE SW NW
        sidewalks_neswx[dir] = dat.t_nesw[dir]->has_flag( has_sidewalk );
        neighbor_sidewalks += sidewalks_neswx[dir];
    }

    // which of the cardinal directions get roads?
    bool roads_nesw[4] = {};
    int num_dirs = terrain_type_with_suffix_to_nesw_array( terrain_type, roads_nesw );
    // if this is a dead end, extend past the middle of the tile
    int dead_end_extension = ( num_dirs == 1 ? 8 : 0 );

    // which way should our roads curve, based on neighbor roads?
    int curvedir_nesw[4] = {};
    for( int dir = 0; dir < 4; dir++ ) { // N E S W
        if( roads_nesw[dir] == false || dat.t_nesw[dir]->get_type_id().str() != "road" ) {
            continue;
        }

        // n_* contain details about the neighbor being considered
        bool n_roads_nesw[4] = {};
        //TODO figure out how to call this function without creating a new oter_id object
        int n_num_dirs = terrain_type_with_suffix_to_nesw_array( dat.t_nesw[dir], n_roads_nesw );
        // if 2-way neighbor has a road facing us
        if( n_num_dirs == 2 && n_roads_nesw[( dir + 2 ) % 4] ) {
            // curve towards the direction the neighbor turns
            if( n_roads_nesw[( dir - 1 + 4 ) % 4] ) {
                curvedir_nesw[dir]--;    // our road curves counterclockwise
            }
            if( n_roads_nesw[( dir + 1 ) % 4] ) {
                curvedir_nesw[dir]++;    // our road curves clockwise
            }
        }
    }

    // calculate how far to rotate the map so we can work with just one orientation
    // also keep track of diagonal roads and plazas
    int rot = 0;
    bool diag = false;
    int plaza_dir = -1;
    bool fourways_neswx[8] = {};
    //TODO reduce amount of logical/conditional constructs here
    //TODO make plazas include adjacent tees
    switch ( num_dirs ) {
        case 4: // 4-way intersection
            for( int dir = 0; dir < 8; dir++ ) {
                fourways_neswx[dir] = ( dat.t_nesw[dir].id() == "road_nesw" ||
                                        dat.t_nesw[dir].id() == "road_nesw_manhole" );
            }
            // is this the middle, or which side or corner, of a plaza?
            plaza_dir = compare_neswx( fourways_neswx, {1, 1, 1, 1, 1, 1, 1, 1} ) ? 8 :
                        compare_neswx( fourways_neswx, {0, 1, 1, 0, 0, 1, 0, 0} ) ? 7 :
                        compare_neswx( fourways_neswx, {1, 1, 0, 0, 1, 0, 0, 0} ) ? 6 :
                        compare_neswx( fourways_neswx, {1, 0, 0, 1, 0, 0, 0, 1} ) ? 5 :
                        compare_neswx( fourways_neswx, {0, 0, 1, 1, 0, 0, 1, 0} ) ? 4 :
                        compare_neswx( fourways_neswx, {1, 1, 1, 0, 1, 1, 0, 0} ) ? 3 :
                        compare_neswx( fourways_neswx, {1, 1, 0, 1, 1, 0, 0, 1} ) ? 2 :
                        compare_neswx( fourways_neswx, {1, 0, 1, 1, 0, 0, 1, 1} ) ? 1 :
                        compare_neswx( fourways_neswx, {0, 1, 1, 1, 0, 1, 1, 0} ) ? 0 :
                        -1;
            if( plaza_dir > -1 ) { rot = plaza_dir % 4; }
        case 3: // tee
            if( !roads_nesw[0] ) { rot = 2; break; } // E/S/W, rotate 180 degrees
            if( !roads_nesw[1] ) { rot = 3; break; } // N/S/W, rotate 270 degrees
            if( !roads_nesw[3] ) { rot = 1; break; } // N/E/S, rotate  90 degrees
            break;                                  // N/E/W, don't rotate
        case 2: // straight or diagonal
            if( roads_nesw[1] && roads_nesw[3] ) { rot = 1; break; }            // E/W, rotate  90 degrees
            if( roads_nesw[1] && roads_nesw[2] ) { rot = 1; diag = true; break; } // E/S, rotate  90 degrees
            if( roads_nesw[2] && roads_nesw[3] ) { rot = 2; diag = true; break; } // S/W, rotate 180 degrees
            if( roads_nesw[3] && roads_nesw[0] ) { rot = 3; diag = true; break; } // W/N, rotate 270 degrees
            if( roads_nesw[0] && roads_nesw[1] ) {          diag = true; break; } // N/E, don't rotate
            break;                                                               // N/S, don't rotate
        case 1: // dead end
            if( roads_nesw[1] ) { rot = 1; break; } // E, rotate  90 degrees
            if( roads_nesw[2] ) { rot = 2; break; } // S, rotate 180 degrees
            if( roads_nesw[3] ) { rot = 3; break; } // W, rotate 270 degrees
            break;                               // N, don't rotate
    }

    // rotate the arrays left by rot steps
    nesw_array_rotate<bool>( sidewalks_neswx, 8, rot * 2 );
    nesw_array_rotate<bool>( roads_nesw,      4, rot );
    nesw_array_rotate<int> ( curvedir_nesw,   4, rot );

    // now we have only these shapes: '   |   '-   -'-   -|-

    if( diag ) { // diagonal roads get drawn differently from all other types
        // draw sidewalks if a S/SW/W neighbor has_sidewalk
        if( sidewalks_neswx[4] || sidewalks_neswx[5] || sidewalks_neswx[6] ) {
            for( int y = 0; y < SEEY * 2; y++ ) {
                for( int x = 0; x < SEEX * 2; x++ ) {
                    if( x > y - 4 && ( x < 4 || y > SEEY * 2 - 5 || y >= x ) ) {
                        m->ter_set( x, y, t_sidewalk );
                    }
                }
            }
        }
        // draw diagonal road
        for( int y = 0; y < SEEY * 2; y++ ) {
            for( int x = 0; x < SEEX * 2; x++ ) {
                if( x > y && // definitely only draw in the upper right half of the map
                     ( ( x > 3 && y < ( SEEY * 2 - 4 ) ) || // middle, for both corners and diagonals
                       ( x < 4 && curvedir_nesw[0] < 0 ) || // diagonal heading northwest
                       ( y > ( SEEY * 2 - 5 ) && curvedir_nesw[1] > 0 ) ) ) { // diagonal heading southeast
                    if( ( x + rot / 2 ) % 4 && ( x - y == SEEX - 1 + ( 1 - ( rot / 2 ) ) || x - y == SEEX + ( 1 - ( rot / 2 ) ) ) ) {
                        m->ter_set( x, y, t_pavement_y );
                    } else {
                        m->ter_set( x, y, t_pavement );
                    }
                }
            }
        }
    } else { // normal road drawing
        bool cul_de_sac = false;
        // dead ends become cul de sacs, 1/3 of the time, if a neighbor has_sidewalk
        if( num_dirs == 1 && one_in( 3 ) && neighbor_sidewalks ) {
            cul_de_sac = true;
            fill_background( m, t_sidewalk );
        }

        // draw normal sidewalks
        for( int dir = 0; dir < 4; dir++ ) {
            if( roads_nesw[dir] ) {
                // sidewalk west of north road, etc
                if( sidewalks_neswx[ ( dir + 3 ) % 4     ] ||  // has_sidewalk west?
                    sidewalks_neswx[ ( dir + 3 ) % 4 + 4 ] ||  // has_sidewalk northwest?
                    sidewalks_neswx[   dir               ] ) { // has_sidewalk north?
                    int x1 = 0, y1 = 0, x2 = 3, y2 = SEEY - 1 + dead_end_extension;
                    coord_rotate_cw( x1, y1, dir );
                    coord_rotate_cw( x2, y2, dir );
                    square( m, t_sidewalk, x1, y1, x2, y2 );
                }
                // sidewalk east of north road, etc
                if( sidewalks_neswx[ ( dir + 1 ) % 4 ] ||  // has_sidewalk east?
                    sidewalks_neswx[   dir + 4       ] ||  // has_sidewalk northeast?
                    sidewalks_neswx[   dir           ] ) { // has_sidewalk north?
                    int x1 = SEEX * 2 - 5, y1 = 0, x2 = SEEX * 2 - 1, y2 = SEEY - 1 + dead_end_extension;
                    coord_rotate_cw( x1, y1, dir );
                    coord_rotate_cw( x2, y2, dir );
                    square( m, t_sidewalk, x1, y1, x2, y2 );
                }
            }
        }

        //draw dead end sidewalk
        if( dead_end_extension > 0 && sidewalks_neswx[ 2 ] ) {
            square( m, t_sidewalk, 0, SEEY + dead_end_extension, SEEX * 2 - 1, SEEY + dead_end_extension + 4 );
        }

        // draw 16-wide pavement from the middle to the edge in each road direction
        // also corner pieces to curve towards diagonal neighbors
        for( int dir = 0; dir < 4; dir++ ) {
            if( roads_nesw[dir] ) {
                int x1 = 4, y1 = 0, x2 = SEEX * 2 - 1 - 4, y2 = SEEY - 1 + dead_end_extension;
                coord_rotate_cw( x1, y1, dir );
                coord_rotate_cw( x2, y2, dir );
                square( m, t_pavement, x1, y1, x2, y2 );
                if( curvedir_nesw[dir] != 0 ) {
                    for( int x = 1; x < 4; x++ ) {
                        for( int y = 0; y < x; y++ ) {
                            int ty = y, tx = ( curvedir_nesw[dir] == -1 ? x : SEEX * 2 - 1 - x );
                            coord_rotate_cw( tx, ty, dir );
                            m->ter_set( tx, ty, t_pavement );
                        }
                    }
                }
            }
        }

        // draw yellow dots on the pavement
        for( int dir = 0; dir < 4; dir++ ) {
            if( roads_nesw[dir] ) {
                int max_y = SEEY;
                if ( num_dirs == 4 || ( num_dirs == 3 && dir == 0 ) ) {
                    max_y = 4; // dots don't extend into some intersections
                }
                for( int x = SEEX - 1; x <= SEEX; x++ ) {
                    for( int y = 0; y < max_y; y++ ) {
                        if( ( y + ( ( dir + rot ) / 2 % 2 ) ) % 4 ) {
                            int xn = x, yn = y;
                            coord_rotate_cw( xn, yn, dir );
                            m->ter_set( xn, yn, t_pavement_y );
                        }
                    }
                }
            }
        }

        // draw round pavement for cul de sac late, to overdraw the yellow dots
        if( cul_de_sac ) {
            circle( m, t_pavement, double( SEEX ) - 0.5, double( SEEY ) - 0.5, 11.0 );
        }

        // overwrite part of intersection with rotary/plaza
        if( plaza_dir > -1 ) {
            if( plaza_dir == 8 ) { // plaza center
                fill_background( m, t_sidewalk );
                //TODO something interesting here
            } else if( plaza_dir < 4 ) { // plaza side
                square( m, t_pavement, 0, SEEY - 10, SEEX * 2 - 1, SEEY - 1 );
                square( m, t_sidewalk, 0, SEEY - 2 , SEEX * 2 - 1, SEEY * 2 - 1 );
                if( one_in( 3 ) ) {
                    line( m, t_tree_young, 1, SEEY, SEEX * 2 - 2, SEEY );
                }
                if( one_in( 3 ) ) {
                    line_furn( m, f_bench, 2, SEEY + 2, 5, SEEY + 2 );
                    line_furn( m, f_bench, 10, SEEY + 2, 13, SEEY + 2 );
                    line_furn( m, f_bench, 18, SEEY + 2, 21, SEEY + 2 );
                }
            } else { // plaza corner
                circle( m, t_pavement, 0, SEEY * 2 - 1, 21 );
                circle( m, t_sidewalk, 0, SEEY * 2 - 1, 13 );
                if( one_in( 3 ) ) {
                    circle( m, t_tree_young, 0, SEEY * 2 - 1, 11 );
                    circle( m, t_sidewalk,   0, SEEY * 2 - 1, 10 );
                }
                if( one_in( 3 ) ) {
                    circle( m, t_water_sh, 4, SEEY * 2 - 5, 3 );
                }
            }
        }
    }

    // spawn some vehicles
    if( plaza_dir != 8 ) {
        vspawn_id( neighbor_sidewalks ? "default_city" : "default_country" ).obj().apply(
            *m,
            num_dirs == 4 ? "road_four_way" :
            num_dirs == 3 ? "road_tee"      :
            num_dirs == 1 ? "road_end"      :
            diag          ? "road_curved"   :
            "road_straight"
        );
    }

    // spawn some monsters
    if( neighbor_sidewalks ) {
        m->place_spawns( mongroup_id( "GROUP_ZOMBIE" ), 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density );
        // 1 per 10 overmaps
        if( one_in( 10000 ) ) {
            m->add_spawn( mon_zombie_jackson, 1, SEEX, SEEY );
        }
    }

    // add some items
    bool plaza = ( plaza_dir > -1 );
    m->place_items( plaza ? "trash" : "road", 5, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, plaza,
                    plaza ? 0 : turn );

    // add a manhole if appropriate
    if( terrain_type == "road_nesw_manhole" ) {
        m->ter_set( rng( 6, SEEX * 2 - 6 ), rng( 6, SEEX * 2 - 6 ), t_manhole_cover );
    }

    // finally, un-rotate the map
    m->rotate( rot );

}
///////////////////

//    } else if (terrain_type == "subway_station") {
void mapgen_subway_station(map *m, oter_id, mapgendata dat, int, float)
{
        if (is_ot_type("subway", dat.north()) && connects_to(dat.north(), 2)) {
            dat.set_dir(0, 1); //n_fac = 1;
        }
        if (is_ot_type("subway", dat.east()) && connects_to(dat.east(), 3)) {
            dat.set_dir(0, 1);
            //e_fac = 1;
        }
        if (is_ot_type("subway", dat.south()) && connects_to(dat.south(), 0)) {
            dat.set_dir(0, 1);
            //s_fac = 1;
        }
        if (is_ot_type("subway", dat.west()) && connects_to(dat.west(), 1)) {
            dat.set_dir(0, 1);
            //w_fac = 1;
        }

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if ((i < 4 && (dat.w_fac == 0 || j < 4 || j > SEEY * 2 - 5)) ||
                    (j < 4 && (dat.n_fac == 0 || i < 4 || i > SEEX * 2 - 5)) ||
                    (i > SEEX * 2 - 5 && (dat.e_fac == 0 || j < 4 || j > SEEY * 2 - 5)) ||
                    (j > SEEY * 2 - 5 && (dat.s_fac == 0 || i < 4 || i > SEEX * 2 - 5))) {
                    m->ter_set(i, j, t_rock_floor);
                } else {
                    m->ter_set(i, j, t_rock_floor);
                }
            }
        }
        m->ter_set(2,            2           , t_stairs_up);
        m->ter_set(SEEX * 2 - 3, 2           , t_stairs_up);
        m->ter_set(2,            SEEY * 2 - 3, t_stairs_up);
        m->ter_set(SEEX * 2 - 3, SEEY * 2 - 3, t_stairs_up);
        if (m->ter(2, SEEY) == t_floor) {
            m->ter_set(2, SEEY, t_stairs_up);
        }
        if (m->ter(SEEX * 2 - 3, SEEY) == t_floor) {
            m->ter_set(SEEX * 2 - 3, SEEY, t_stairs_up);
        }
        if (m->ter(SEEX, 2) == t_floor) {
            m->ter_set(SEEX, 2, t_stairs_up);
        }
        if (m->ter(SEEX, SEEY * 2 - 3) == t_floor) {
            m->ter_set(SEEX, SEEY * 2 - 3, t_stairs_up);
        }
}

void mapgen_subway_straight(map *m, oter_id terrain_type, mapgendata dat, int, float)
{
        if (terrain_type == "subway_ns") {
            dat.w_fac = (dat.west()  == "cavern" ? 0 : 4);
            dat.e_fac = (dat.east()  == "cavern" ? SEEX * 2 : SEEX * 2 - 5);
        } else {
            dat.w_fac = (dat.north() == "cavern" ? 0 : 4);
            dat.e_fac = (dat.south() == "cavern" ? SEEX * 2 : SEEX * 2 - 5);
        }
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (i < dat.w_fac || i > dat.e_fac) {
                    m->ter_set(i, j, t_rock);
                } else if (one_in(90)) {
                    m->ter_set(i, j, t_rock_floor);
                    m->make_rubble( tripoint( i,  j, m->get_abs_sub().z ), f_rubble_rock, true);
                } else {
                    m->ter_set(i, j, t_rock_floor);
                }
            }
        }
        if (is_ot_type("sub_station", dat.t_above)) {
            m->ter_set(SEEX * 2 - 5, rng(SEEY - 5, SEEY + 4), t_stairs_up);
        }
        m->place_items("subway", 30, 4, 0, SEEX * 2 - 5, SEEY * 2 - 1, true, 0);
        if (terrain_type == "subway_ew") {
            m->rotate(1);
        }
}

void mapgen_subway_curved(map *m, oter_id terrain_type, mapgendata dat, int, float)
{
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if ((i >= SEEX * 2 - 4 && j < 4) || i < 4 || j >= SEEY * 2 - 4) {
                    m->ter_set(i, j, t_rock);
                } else if (one_in(30)) {
                    m->ter_set(i, j, t_rock_floor);
                    m->make_rubble( tripoint( i,  j, m->get_abs_sub().z ), f_rubble_rock, true);
                } else {
                    m->ter_set(i, j, t_rock_floor);
                }
            }
        }
        if( is_ot_type( "sub_station", dat.t_above ) ) {
            m->ter_set(SEEX * 2 - 5, rng(SEEY - 5, SEEY + 4), t_stairs_up);
        }
        m->place_items("subway", 30, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
        if (terrain_type == "subway_es") {
            m->rotate(1);
        }
        if (terrain_type == "subway_sw") {
            m->rotate(2);
        }
        if (terrain_type == "subway_wn") {
            m->rotate(3);
        }
}

void mapgen_subway_tee(map *m, oter_id terrain_type, mapgendata dat, int, float)
{
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (i < 4 || (i >= SEEX * 2 - 4 && (j < 4 || j >= SEEY * 2 - 4))) {
                    m->ter_set(i, j, t_rock);
                } else if (one_in(30)) {
                    m->ter_set(i, j, t_rock_floor);
                    m->make_rubble( tripoint( i,  j, m->get_abs_sub().z ), f_rubble_rock, true);
                } else {
                    m->ter_set(i, j, t_rock_floor);
                }
            }
        }
        if( is_ot_type( "sub_station", dat.t_above ) ) {
            m->ter_set(4, rng(SEEY - 5, SEEY + 4), t_stairs_up);
        }
        m->place_items("subway", 35, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
        if (terrain_type == "subway_esw") {
            m->rotate(1);
        }
        if (terrain_type == "subway_nsw") {
            m->rotate(2);
        }
        if (terrain_type == "subway_new") {
            m->rotate(3);
        }
}

void mapgen_subway_four_way(map *m, oter_id, mapgendata dat, int, float)
{

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if ((i < 4 || i >= SEEX * 2 - 4) &&
                    (j < 4 || j >= SEEY * 2 - 4)) {
                    m->ter_set(i, j, t_rock);
                } else if (one_in(30)) {
                    m->ter_set(i, j, t_rock_floor);
                    m->make_rubble( tripoint( i,  j, m->get_abs_sub().z ), f_rubble_rock, true);
                } else {
                    m->ter_set(i, j, t_rock_floor);
                }
            }
        }

        if (is_ot_type("sub_station", dat.t_above)) {
            m->ter_set(4 + rng(0, 1) * (SEEX * 2 - 9), 4 + rng(0, 1) * (SEEY * 2 - 9), t_stairs_up);
        }
        m->place_items("subway", 40, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
}

void mapgen_sewer_straight(map *m, oter_id terrain_type, mapgendata dat, int, float)
{
    (void)dat;
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (i < SEEX - 2 || i > SEEX + 1) {
                    m->ter_set(i, j, t_rock);
                } else {
                    m->ter_set(i, j, t_sewage);
                }
            }
        }
        m->place_items("sewer", 10, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
        if (terrain_type == "sewer_ew") {
            m->rotate(1);
        }
}

void mapgen_sewer_curved(map *m, oter_id terrain_type, mapgendata dat, int, float)
{
    (void)dat;
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if ((i > SEEX + 1 && j < SEEY - 2) || i < SEEX - 2 || j > SEEY + 1) {
                    m->ter_set(i, j, t_rock);
                } else {
                    m->ter_set(i, j, t_sewage);
                }
            }
        }
        m->place_items("sewer", 18, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
        if (terrain_type == "sewer_es") {
            m->rotate(1);
        }
        if (terrain_type == "sewer_sw") {
            m->rotate(2);
        }
        if (terrain_type == "sewer_wn") {
            m->rotate(3);
        }
}

void mapgen_sewer_tee(map *m, oter_id terrain_type, mapgendata dat, int, float)
{
    (void)dat;
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (i < SEEX - 2 || (i > SEEX + 1 && (j < SEEY - 2 || j > SEEY + 1))) {
                    m->ter_set(i, j, t_rock);
                } else {
                    m->ter_set(i, j, t_sewage);
                }
            }
        }
        m->place_items("sewer", 23, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
        if (terrain_type == "sewer_esw") {
            m->rotate(1);
        }
        if (terrain_type == "sewer_nsw") {
            m->rotate(2);
        }
        if (terrain_type == "sewer_new") {
            m->rotate(3);
        }
}

void mapgen_sewer_four_way(map *m, oter_id, mapgendata dat, int, float)
{
    (void)dat;
        int rn = rng(0, 3);
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if ((i < SEEX - 2 || i > SEEX + 1) && (j < SEEY - 2 || j > SEEY + 1)) {
                    m->ter_set(i, j, t_rock);
                } else {
                    m->ter_set(i, j, t_sewage);
                }
                if (rn == 0 && (trig_dist(i, j, SEEX - 1, SEEY - 1) <= 6 ||
                                trig_dist(i, j, SEEX - 1, SEEY    ) <= 6 ||
                                trig_dist(i, j, SEEX,     SEEY - 1) <= 6 ||
                                trig_dist(i, j, SEEX,     SEEY    ) <= 6   )) {
                    m->ter_set(i, j, t_sewage);
                }
                if (rn == 0 && (i == SEEX - 1 || i == SEEX) && (j == SEEY - 1 || j == SEEY)) {
                    m->ter_set(i, j, t_grate);
                }
            }
        }
        m->place_items("sewer", 28, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
}

///////////////////
void mapgen_bridge(map *m, oter_id terrain_type, mapgendata dat, int turn, float)
{
    (void)dat;
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            if (i < 2 || i >= SEEX * 2 - 2) {
                m->ter_set(i, j, t_water_dp);
            } else if (i == 2 || i == SEEX * 2 - 3) {
                m->ter_set(i, j, t_guardrail_bg_dp);
            } else if (i == 3 || i == SEEX * 2 - 4) {
                m->ter_set(i, j, t_sidewalk_bg_dp);
            } else {
                if ((i == SEEX - 1 || i == SEEX) && j % 4 != 0) {
                    m->ter_set(i, j, t_pavement_y_bg_dp);
                } else {
                    m->ter_set(i, j, t_pavement_bg_dp);
                }
            }
        }
    }

    // spawn regular road out of fuel vehicles
    VehicleSpawn::apply(vspawn_id("default_bridge"), *m, "bridge");

    if (terrain_type == "bridge_ew") {
        m->rotate(1);
    }
    m->place_items("road", 5, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, false, turn);
}

void mapgen_highway(map *m, oter_id terrain_type, mapgendata dat, int turn, float)
{
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            if (i < 3 || i >= SEEX * 2 - 3) {
                m->ter_set(i, j, dat.groundcover());
            } else if (i == 3 || i == SEEX * 2 - 4) {
                m->ter_set(i, j, t_railing_v);
            } else {
                if ((i == SEEX - 1 || i == SEEX) && j % 4 != 0) {
                    m->ter_set(i, j, t_pavement_y);
                } else {
                    m->ter_set(i, j, t_pavement);
                }
            }
        }
    }

    // spawn regular road out of fuel vehicles
    VehicleSpawn::apply(vspawn_id("default_highway"), *m, "highway");

    if (terrain_type == "hiway_ew") {
        m->rotate(1);
    }
    m->place_items("road", 8, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, false, turn);
}

void mapgen_river_center(map *m, oter_id, mapgendata dat, int, float)
{
    (void)dat;
    fill_background(m, t_water_dp);
}

void mapgen_river_curved_not(map *m, oter_id terrain_type, mapgendata dat, int, float)
{
    (void)dat;
    fill_background(m, t_water_dp);
    // this is not_ne, so deep on all sides except ne corner, which is shallow
    // shallow is 20,0, 23,4
    int north_edge = rng(16, 18);
    int east_edge = rng(4, 8);

    for(int x = north_edge; x < 24; x++){
        for(int y = 0; y < east_edge; y++){
            int circle_edge = ((24 - x) * (24 - x)) + (y * y);
            if(circle_edge <= 8){
                m->ter_set(x, y, grass_or_dirt());
            }
            else if(circle_edge <= 36){
                m->ter_set(x, y, t_water_sh);
            }
        }
    }

    if (terrain_type == "river_c_not_se") {
        m->rotate(1);
    }
    if (terrain_type == "river_c_not_sw") {
        m->rotate(2);
    }
    if (terrain_type == "river_c_not_nw") {
        m->rotate(3);
    }
}

void mapgen_river_straight(map *m, oter_id terrain_type, mapgendata dat, int, float)
{
    (void)dat;
    fill_background(m, t_water_dp);

    for(int x = 0; x <= 24; x++){
        int ground_edge = rng(1,3);
        int shallow_edge = rng(4,6);
        line(m, grass_or_dirt(), x, 0, x, ground_edge);
        line(m, t_water_sh, x, ground_edge, x, shallow_edge);
    }

    if (terrain_type == "river_east") {
        m->rotate(1);
    }
    if (terrain_type == "river_south") {
        m->rotate(2);
    }
    if (terrain_type == "river_west") {
        m->rotate(3);
    }
}

void mapgen_river_curved(map *m, oter_id terrain_type, mapgendata dat, int, float)
{
    (void)dat;
    fill_background(m, t_water_dp);
    // NE corner deep, other corners are shallow.  do 2 passes: one x, one y
    for(int x = 0; x < 24; x++){
        int ground_edge = rng(1,3);
        int shallow_edge = rng(4,6);
        line(m, grass_or_dirt(), x, 0, x, ground_edge);
        line(m, t_water_sh, x, ground_edge, x, shallow_edge);
    }
    for(int y = 0; y < 24; y++){
        int ground_edge = rng(19,21);
        int shallow_edge = rng(16,18);
        line(m, grass_or_dirt(), ground_edge, y, 23, y);
        line(m, t_water_sh, shallow_edge, y, ground_edge, y);
    }

    if (terrain_type == "river_se") {
        m->rotate(1);
    }
    if (terrain_type == "river_sw") {
        m->rotate(2);
    }
    if (terrain_type == "river_nw") {
        m->rotate(3);
    }
}

void mapgen_parking_lot(map *m, oter_id, mapgendata dat, int turn, float)
{
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            if ((j == 5 || j == 9 || j == 13 || j == 17 || j == 21) &&
                 ((i > 1 && i < 8) || (i > 14 && i < SEEX * 2 - 2)))
                m->ter_set(i, j, t_pavement_y);
            else if ((j < 2 && i > 7 && i < 17) || (j >= 2 && j < SEEY * 2 - 2 && i > 1 &&
                      i < SEEX * 2 - 2))
                m->ter_set(i, j, t_pavement);
            else
                m->ter_set(i, j, dat.groundcover());
        }
    }

    VehicleSpawn::apply(vspawn_id("default_parkinglot"), *m, "parkinglot");

    m->place_items("road", 8, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, false, turn);
    for (int i = 1; i < 4; i++) {
        const std::string &id = dat.t_nesw[i].id().str();
        if( id.size() > 5 && id.find( "road_" ) == 0 ) {
            m->rotate(i);
        }
    }
}

void mapgen_gas_station(map *m, oter_id terrain_type, mapgendata dat, int, float density)
{
    int top_w = rng(5, 14);
    int bottom_w = SEEY * 2 - rng(1, 2);
    int middle_w = rng(top_w + 5, bottom_w - 3);
    if (middle_w < bottom_w - 5) {
        middle_w = bottom_w - 5;
    }
    int left_w = rng(0, 3);
    int right_w = SEEX * 2 - rng(1, 4);
    int center_w = rng(left_w + 4, right_w - 5);
    int pump_count = rng(3, 6);
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEX * 2; j++) {
            if (j < top_w && (top_w - j) % 4 == 0 && i > left_w && i < right_w &&
                 (i - (1 + left_w)) % pump_count == 0) {
                m->place_gas_pump(i, j, rng(1000, 10000));
            } else if ((j < 2 && i > 7 && i < 16) || (j < top_w && i > left_w && i < right_w)) {
                m->ter_set(i, j, t_pavement);
            } else if (j == top_w && (i == left_w + 6 || i == left_w + 7 || i == right_w - 7 ||
                      i == right_w - 6)) {
                m->ter_set(i, j, t_window);
            } else if (((j == top_w || j == bottom_w) && i >= left_w && i <= right_w) ||
                      (j == middle_w && (i >= center_w && i < right_w))) {
                m->ter_set(i, j, t_wall);
            } else if (((i == left_w || i == right_w) && j > top_w && j < bottom_w) ||
                      (j > middle_w && j < bottom_w && (i == center_w || i == right_w - 2))) {
                m->ter_set(i, j, t_wall);
            } else if (i == left_w + 1 && j > top_w && j < bottom_w) {
                m->set(i, j, t_floor, f_glass_fridge);
            } else if (i > left_w + 2 && i < left_w + 12 && i < center_w && i % 2 == 1 &&
                      j > top_w + 1 && j < middle_w - 1) {
                m->set(i, j, t_floor, f_rack);
            } else if ((i == right_w - 5 && j > top_w + 1 && j < top_w + 4) ||
                      (j == top_w + 3 && i > right_w - 5 && i < right_w)) {
                m->set(i, j, t_floor, f_counter);
            } else if (i > left_w && i < right_w && j > top_w && j < bottom_w) {
                m->ter_set(i, j, t_floor);
            } else {
                m->ter_set(i, j, dat.groundcover());
            }
        }
    }
    //vending
    bool drinks = rng(0,1);
    std::string type;
    std::string type2;
    if (drinks) {
        type = "vending_drink";
        type2 = "vending_food";
    } else {
        type2 = "vending_drink";
        type = "vending_food";
    }
    int vset = rng(1,5), vset2 = rng(1,5);
    if(rng(0,1)) {
        vset += left_w;
    } else {
        vset = right_w - vset;
    }
    m->place_vending(vset, top_w-1, type);
    if(rng(0,1))
    {
        if(rng(0,1)) {
            vset2 += left_w;
        } else {
            vset2 = right_w - vset2;
        }
        if (vset2 != vset) {
            m->place_vending(vset2, top_w-1, type);
        }
    }
    if (vset2 != vset-1) {
        if(rng(0,1)) {
            //ATM
            m->ter_set(vset - 1, top_w-1, t_atm);
        } else {
            //charging rack
            m->furn_set(vset - 1, top_w-1, f_rack);
            m->place_items("gas_charging_rack", 100, vset - 1, top_w-1, vset - 1, top_w-1, false, 0);
        }
    }
    //
    m->ter_set(center_w, rng(middle_w + 1, bottom_w - 1), t_door_c);
    m->ter_set(right_w - 1, middle_w, t_door_c);
    m->ter_set(right_w - 1, bottom_w - 1, t_floor);
    m->place_toilet(right_w - 1, bottom_w - 1);
    m->ter_set(rng(10, 13), top_w, t_door_c);
    if (one_in(5)) {
        m->ter_set(rng(left_w + 1, center_w - 1), bottom_w, (one_in(4) ? t_door_c : t_door_locked));
    }
    for (int i = left_w + (left_w % 2 == 0 ? 3 : 4); i < center_w && i < left_w + 12; i += 2) {
        if (!one_in(3)) {
            m->place_items("snacks", 74, i, top_w + 2, i, middle_w - 2, false, 0);
        } else {
            m->place_items("magazines", 74, i, top_w + 2, i, middle_w - 2, false, 0);
        }
    }
    m->place_items("fridgesnacks", 82, left_w + 1, top_w + 1, left_w + 1, bottom_w - 1, false, 0);
    m->place_items("road",  12, 0,      0,  SEEX*2 - 1, top_w - 1, false, 0);
    m->place_items("behindcounter", 70, right_w - 4, top_w + 1, right_w - 1, top_w + 2, false, 0);
    m->place_items("softdrugs", 12, right_w - 1, bottom_w - 2, right_w - 1, bottom_w - 2, false, 0);
    if (terrain_type == "s_gas_east") {
        m->rotate(1);
    }
    if (terrain_type == "s_gas_south") {
        m->rotate(2);
    }
    if (terrain_type == "s_gas_west") {
        m->rotate(3);
    }
    m->place_spawns( mongroup_id( "GROUP_ZOMBIE" ), 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);
}
////////////////////


void house_room(map *m, room_type type, int x1, int y1, int x2, int y2, mapgendata & dat)
{
    int pos_x1 = 0;
    int pos_y1 = 0;

    if (type == room_backyard) { //processing it separately
        m->furn_set(x1 + 2, y1, f_chair);
        m->furn_set(x1 + 2, y1 + 1, f_table);
        for (int i = x1; i <= x2; i++) {
            for (int j = y1; j <= y2; j++) {
                if ((i == x1) || (i == x2)) {
                    m->ter_set(i, j, t_fence_v);
                } else if (j == y2) {
                    m->ter_set(i, j, t_fence_h);
                } else {
                    m->ter_set( i, j, t_grass);
                    if (one_in(35) && !m->has_furn(i ,j)) {
                        m->ter_set(i, j, t_tree_young);
                    } else if (one_in(35) && !m->has_furn(i ,j)) {
                        m->ter_set(i, j, t_tree);
                    } else if (one_in(25)) {
                        m->ter_set(i, j, t_dirt);
                    }
                }
            }
        }
        m->ter_set((x1 + x2) / 2, y2, t_fencegate_c);
        return;
    }

    for (int i = x1; i <= x2; i++) {
        for (int j = y1; j <= y2; j++) {
            if (dat.is_groundcover( m->ter(i, j) ) ||
//m->ter(i, j) == t_grass || m->ter(i, j) == t_dirt ||
                m->ter(i, j) == t_floor) {
                if (j == y1 || j == y2) {
                    m->ter_set(i, j, t_wall);
                } else if (i == x1 || i == x2) {
                    m->ter_set(i, j, t_wall);
                } else {
                    m->ter_set(i, j, t_floor);
                }
            }
        }
    }
    for (int i = y1 + 1; i <= y2 - 1; i++) {
        m->ter_set(x1, i, t_wall);
        m->ter_set(x2, i, t_wall);
    }

    items_location placed = "none";
    int chance = 0, rn;
    switch (type) {
    case room_study:
        placed = "livingroom";
        chance = 40;
        break;
    case room_living:
        placed = "livingroom";
        chance = 83;
        //choose random wall
        switch (rng(1, 4)) { //some bookshelves
        case 1:
            pos_x1 = x1 + 2;
            pos_y1 = y1 + 1;
            m->furn_set(x1 + 2, y2 - 1, f_desk);
            while (pos_x1 < x2) {
                pos_x1 += 1;
                if (m->ter(pos_x1, pos_y1) == t_wall) {
                    break;
                }
                m->furn_set(pos_x1, pos_y1, f_bookcase);
                pos_x1 += 1;
                if (m->ter(pos_x1, pos_y1) == t_wall) {
                    break;
                }
                m->furn_set(pos_x1, pos_y1, f_bookcase);
                pos_x1 += 2;
            }
            break;
        case 2:
            pos_x1 = x2 - 2;
            pos_y1 = y1 + 1;
            m->furn_set(x1 + 2, y2 - 1, f_desk);
            while (pos_x1 > x1) {
                pos_x1 -= 1;
                if (m->ter(pos_x1, pos_y1) == t_wall) {
                    break;
                }
                m->furn_set(pos_x1, pos_y1, f_bookcase);
                pos_x1 -= 1;
                if (m->ter(pos_x1, pos_y1) == t_wall) {
                    break;
                }
                m->furn_set(pos_x1, pos_y1, f_bookcase);
                pos_x1 -= 2;
            }
            break;
        case 3:
            pos_x1 = x1 + 2;
            pos_y1 = y2 - 1;
            m->furn_set(x1 + 2, y2 - 1, f_desk);
            while (pos_x1 < x2) {
                pos_x1 += 1;
                if (m->ter(pos_x1, pos_y1) == t_wall) {
                    break;
                }
                m->furn_set(pos_x1, pos_y1, f_bookcase);
                pos_x1 += 1;
                if (m->ter(pos_x1, pos_y1) == t_wall) {
                    break;
                }
                m->furn_set(pos_x1, pos_y1, f_bookcase);
                pos_x1 += 2;
            }
            break;
        case 4:
            pos_x1 = x2 - 2;
            pos_y1 = y2 - 1;
            m->furn_set(x1 + 2, y2 - 1, f_desk);
            while (pos_x1 > x1) {
                pos_x1 -= 1;
                if (m->ter(pos_x1, pos_y1) == t_wall) {
                    break;
                }
                m->furn_set(pos_x1, pos_y1, f_bookcase);
                pos_x1 -= 1;
                if (m->ter(pos_x1, pos_y1) == t_wall) {
                    break;
                }
                m->furn_set(pos_x1, pos_y1, f_bookcase);
                pos_x1 -= 2;
            }
            break;
            m->furn_set(rng(x1 + 2, x2 - 2), rng(y1 + 1, y2 - 1), f_armchair);
        }


        break;
    case room_kitchen: {
        placed = "kitchen";
        chance = 75;
        m->place_items("cleaning",  58, x1 + 1, y1 + 1, x2 - 1, y2 - 2, false, 0);
        m->place_items("home_hw",   40, x1 + 1, y1 + 1, x2 - 1, y2 - 2, false, 0);
        int oven_x = -1, oven_y = -1, cupboard_x = -1, cupboard_y = -1;

        switch (rng(1, 4)) { //fridge, sink, oven and some cupboards near them
        case 1:
            m->furn_set(x1 + 2, y1 + 1, f_fridge);
            m->place_items("fridge", 82, x1 + 2, y1 + 1, x1 + 2, y1 + 1, false, 0);
            m->furn_set(x1 + 1, y1 + 1, f_sink);
            if (x1 + 4 < x2) {
                oven_x     = x1 + 3;
                cupboard_x = x1 + 4;
                oven_y = cupboard_y = y1 + 1;
            }

            break;
        case 2:
            m->furn_set(x2 - 2, y1 + 1, f_fridge);
            m->place_items("fridge", 82, x2 - 2, y1 + 1, x2 - 2, y1 + 1, false, 0);
            m->furn_set(x2 - 1, y1 + 1, f_sink);
            if (x2 - 4 > x1) {
                oven_x     = x2 - 3;
                cupboard_x = x2 - 4;
                oven_y = cupboard_y = y1 + 1;
            }
            break;
        case 3:
            m->furn_set(x1 + 2, y2 - 1, f_fridge);
            m->place_items("fridge", 82, x1 + 2, y2 - 1, x1 + 2, y2 - 1, false, 0);
            m->furn_set(x1 + 1, y2 - 1, f_sink);
            if (x1 + 4 < x2) {
                oven_x     = x1 + 3;
                cupboard_x = x1 + 4;
                oven_y = cupboard_y = y2 - 1;
            }
            break;
        case 4:
            m->furn_set(x2 - 2, y2 - 1, f_fridge);
            m->place_items("fridge", 82, x2 - 2, y2 - 1, x2 - 2, y2 - 1, false, 0);
            m->furn_set(x2 - 1, y2 - 1, f_sink);
            if (x2 - 4 > x1) {
                oven_x     = x2 - 3;
                cupboard_x = x2 - 4;
                oven_y = cupboard_y = y2 - 1;
            }
            break;
        }

        // oven and it's contents
        if ( oven_x != -1 && oven_y != -1 ) {
            m->furn_set(oven_x, oven_y, f_oven);
            m->place_items("oven",       70, oven_x, oven_y, oven_x, oven_y, false, 0);
        }

        // cupboard and it's contents
        if ( cupboard_x != -1 && cupboard_y != -1 ) {
            m->furn_set(cupboard_x, cupboard_y, f_cupboard);
            m->place_items("cleaning",   30, cupboard_x, cupboard_y, cupboard_x, cupboard_y, false, 0);
            m->place_items("home_hw",    30, cupboard_x, cupboard_y, cupboard_x, cupboard_y, false, 0);
            m->place_items("cannedfood", 30, cupboard_x, cupboard_y, cupboard_x, cupboard_y, false, 0);
            m->place_items("pasta",      30, cupboard_x, cupboard_y, cupboard_x, cupboard_y, false, 0);
        }

        if (one_in(2)) { //dining table in the kitchen
            square_furn(m, f_table, int((x1 + x2) / 2) - 1, int((y1 + y2) / 2) - 1, int((x1 + x2) / 2),
                        int((y1 + y2) / 2) );
            m->place_items("dining", 20, int((x1 + x2) / 2) - 1, int((y1 + y2) / 2) - 1,
                           int((x1 + x2) / 2), int((y1 + y2) / 2), false, 0);
        }
        if (one_in(2)) {
            for (int i = 0; i <= 2; i++) {
                pos_x1 = rng(x1 + 2, x2 - 2);
                pos_y1 = rng(y1 + 1, y2 - 1);
                if (m->ter(pos_x1, pos_y1) == t_floor && !(m->furn(pos_x1, pos_y1) == f_cupboard ||
                    m->furn(pos_x1, pos_y1) == f_oven || m->furn(pos_x1, pos_y1) == f_sink ||
                    m->furn(pos_x1, pos_y1) == f_fridge)) {
                    m->furn_set(pos_x1, pos_y1, f_chair);
                }
            }
        }

    }
    break;
    case room_bedroom:
        placed = "bedroom";
        chance = 78;
        if (one_in(14)) {
            m->place_items("homeguns", 58, x1 + 1, y1 + 1, x2 - 1, y2 - 1, false, 0);
        }
        if (one_in(10)) {
            m->place_items("home_hw",  40, x1 + 1, y1 + 1, x2 - 1, y2 - 1, false, 0);
        }
        switch (rng(1, 5)) {
        case 1:
            m->furn_set(x1 + 1, y1 + 2, f_bed);
            m->furn_set(x1 + 1, y1 + 3, f_bed);
            m->place_items("bed", 60, x1 + 1, y1 + 2, x1 + 1, y1 + 2, false, 0);
            m->place_items("bed", 60, x1 + 1, y1 + 3, x1 + 1, y1 + 3, false, 0);
            break;
        case 2:
            m->furn_set(x1 + 2, y2 - 1, f_bed);
            m->furn_set(x1 + 3, y2 - 1, f_bed);
            m->place_items("bed", 60, x1 + 2, y2 - 1, x1 + 2, y2 - 1, false, 0);
            m->place_items("bed", 60, x1 + 2, y2 - 1, x1 + 2, y2 - 1, false, 0);
            break;
        case 3:
            m->furn_set(x2 - 1, y2 - 3, f_bed);
            m->furn_set(x2 - 1, y2 - 2, f_bed);
            m->place_items("bed", 60, x2 - 1, y2 - 3, x2 - 1, y2 - 3, false, 0);
            m->place_items("bed", 60, x2 - 1, y2 - 2, x2 - 1, y2 - 2, false, 0);
            break;
        case 4:
            m->furn_set(x2 - 3, y1 + 1, f_bed);
            m->furn_set(x2 - 2, y1 + 1, f_bed);
            m->place_items("bed", 60, x2 - 3, y1 + 1, x2 - 3, y1 + 1, false, 0);
            m->place_items("bed", 60, x2 - 2, y1 + 1, x2 - 2, y1 + 1, false, 0);
            break;
        case 5:
            m->furn_set(int((x1 + x2) / 2)    , y2 - 1, f_bed);
            m->furn_set(int((x1 + x2) / 2) + 1, y2 - 1, f_bed);
            m->furn_set(int((x1 + x2) / 2)    , y2 - 2, f_bed);
            m->furn_set(int((x1 + x2) / 2) + 1, y2 - 2, f_bed);
            m->place_items("bed", 60, int((x1 + x2) / 2), y2 - 1, int((x1 + x2) / 2), y2 - 1, false, 0);
            m->place_items("bed", 60, int((x1 + x2) / 2) + 1, y2 - 1, int((x1 + x2) / 2) + 1, y2 - 1, false, 0);
            m->place_items("bed", 60, int((x1 + x2) / 2), y2 - 2, int((x1 + x2) / 2), y2 - 2, false, 0);
            m->place_items("bed", 60, int((x1 + x2) / 2) + 1, y2 - 2, int((x1 + x2) / 2) + 1, y2 - 2, false, 0);
            break;
        }
        switch (rng(1, 4)) {
        case 1:
            m->furn_set(x1 + 2, y1 + 1, f_dresser);
            m->place_items("dresser", 80, x1 + 2, y1 + 1, x1 + 2, y1 + 1, false, 0);
            break;
        case 2:
            m->furn_set(x2 - 2, y2 - 1, f_dresser);
            m->place_items("dresser", 80, x2 - 2, y2 - 1, x2 - 2, y2 - 1, false, 0);
            break;
        case 3:
            rn = int((x1 + x2) / 2);
            m->furn_set(rn, y1 + 1, f_dresser);
            m->place_items("dresser", 80, rn, y1 + 1, rn, y1 + 1, false, 0);
            break;
        case 4:
            rn = int((y1 + y2) / 2);
            m->furn_set(x1 + 1, rn, f_dresser);
            m->place_items("dresser", 80, x1 + 1, rn, x1 + 1, rn, false, 0);
            break;
        }
        break;
    case room_bathroom:
        m->place_toilet(x2 - 1, y2 - 1);
        m->place_items("harddrugs", 18, x1 + 1, y1 + 1, x2 - 1, y2 - 2, false, 0);
        m->place_items("cleaning",  48, x1 + 1, y1 + 1, x2 - 1, y2 - 2, false, 0);
        placed = "softdrugs";
        chance = 72;
        m->furn_set(x2 - 1, y2 - 2, f_bathtub);
        if (one_in(3) && !(m->ter(x2 - 1, y2 - 3) == t_wall)) {
            m->furn_set(x2 - 1, y2 - 3, f_bathtub);
        }
        if (!((m->furn(x1 + 1, y2 - 2) == f_toilet) || (m->furn(x1 + 1, y2 - 2) == f_bathtub))) {
            m->furn_set(x1 + 1, y2 - 2, f_sink);
        }
        if (one_in(4)) {
            for (int x = x1 + 1; x <= x2 - 1; x++) {
                for (int y = y1 + 1; y <= y2 - 1; y++) {
                    m->ter_set(x, y, t_linoleum_white);
                }
            }
        } else if (one_in(4)) {
            for (int x = x1 + 1; x <= x2 - 1; x++) {
                for (int y = y1 + 1; y <= y2 - 1; y++) {
                    m->ter_set(x, y, t_linoleum_gray);
                }
            }
        }
        break;
    default:
        break;
    }
    m->place_items(placed, chance, x1 + 1, y1 + 1, x2 - 1, y2 - 1, false, 0);
}


void mapgen_generic_house_boxy(map *m, oter_id terrain_type, mapgendata dat, int turn, float density) {
    mapgen_generic_house(m, terrain_type, dat, turn, density, 1);
}

void mapgen_generic_house_big_livingroom(map *m, oter_id terrain_type, mapgendata dat, int turn, float density) {
    mapgen_generic_house(m, terrain_type, dat, turn, density, 2);
}

void mapgen_generic_house_center_hallway(map *m, oter_id terrain_type, mapgendata dat, int turn, float density) {
    mapgen_generic_house(m, terrain_type, dat, turn, density, 3);
}

void mapgen_generic_house(map *m, oter_id terrain_type, mapgendata dat, int turn, float density, int variant)
{
    int rn = 0;
    int lw = 0;
    int rw = 0;
    int mw = 0;
    int tw = 0;
    int bw = 0;
    int cw = 0;
    int actual_house_height = 0;
    int bw_old = 0;

    int x = 0;
    int y = 0;
    lw = rng(0, 4);  // West external wall
    mw = lw + rng(7, 10);  // Middle wall between bedroom & kitchen/bath
    rw = SEEX * 2 - rng(1, 5); // East external wall
    tw = rng(1, 6);  // North external wall
    bw = SEEX * 2 - rng(2, 5); // South external wall
    cw = tw + rng(4, 7);  // Middle wall between living room & kitchen/bed
    actual_house_height = bw - rng(4,
                                   6); //reserving some space for backyard. Actual south external wall.
    bw_old = bw;

    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            if (i > lw && i < rw && j > tw && j < bw) {
                m->ter_set(i, j, t_floor);
            } else {
                m->ter_set(i, j, dat.groundcover());
            }
            if (i >= lw && i <= rw && (j == tw || j == bw)) { //placing north and south walls
                m->ter_set(i, j, t_wall);
            }
            if ((i == lw || i == rw) && j > tw &&
                j < bw /*actual_house_height*/) { //placing west (lw) and east walls
                m->ter_set(i, j, t_wall);
            }
        }
    }
    switch(variant) {
    case 1: // Quadrants, essentially
        mw = rng(lw + 5, rw - 5);
        cw = tw + rng(4, 7);
        house_room(m, room_living, mw, tw, rw, cw, dat );
        house_room(m, room_kitchen, lw, tw, mw, cw, dat);
        m->ter_set(mw, rng(tw + 2, cw - 2), (one_in(3) ? t_door_c : t_floor));
        rn = rng(lw + 1, mw - 2);
        m->ter_set(rn    , tw, t_window_domestic);
        m->ter_set(rn + 1, tw, t_window_domestic);
        rn = rng(mw + 1, rw - 2);
        m->ter_set(rn    , tw, t_window_domestic);
        m->ter_set(rn + 1, tw, t_window_domestic);
        rn = rng(lw + 3, rw - 3); // Bottom part mw
        if (rn <= lw + 5) {
            // Bedroom on right, bathroom on left
            house_room(m, room_bedroom, rn, cw, rw, bw, dat);

            // Put door between bedroom and living
            m->ter_set(rng(rw - 1, rn > mw ? rn + 1 : mw + 1), cw, t_door_c);

            if (bw - cw >= 10 && rn - lw >= 6) {
                // All fits, placing bathroom and 2nd bedroom
                house_room(m, room_bathroom, lw, bw - 5, rn, bw, dat);
                house_room(m, room_bedroom, lw, cw, rn, bw - 5, dat);

                // Put door between bathroom and bedroom
                m->ter_set(rn, rng(bw - 4, bw - 1), t_door_c);

                if (one_in(3)) {
                    // Put door between 2nd bedroom and 1st bedroom
                    m->ter_set(rn, rng(cw + 1, bw - 6), t_door_c);
                } else {
                    // ...Otherwise, between 2nd bedroom and kitchen
                    m->ter_set(rng(lw + 1, rn > mw ? mw - 1 : rn - 1), cw, t_door_c);
                }
            } else if (bw - cw > 4) {
                // Too big for a bathroom, not big enough for 2nd bedroom
                // Make it a bathroom anyway, but give the excess space back to
                // the kitchen.
                house_room(m, room_bathroom, lw, bw - 4, rn, bw, dat);
                for (int i = lw + 1; i < mw && i < rn; i++) {
                    m->ter_set(i, cw, t_floor);
                }

                // Put door between excess space and bathroom
                m->ter_set(rng(lw + 1, rn - 1), bw - 4, t_door_c);

                // Put door between excess space and bedroom
                m->ter_set(rn, rng(cw + 1, bw - 5), t_door_c);
            } else {
                // Small enough to be a bathroom; make it one.
                house_room(m, room_bathroom, lw, cw, rn, bw, dat);

                if (one_in(5)) {
                    // Put door between bathroom and kitchen with low chance
                    m->ter_set(rng(lw + 1, rn > mw ? mw - 1 : rn - 1), cw, t_door_c);
                } else {
                    // ...Otherwise, between bathroom and bedroom
                    m->ter_set(rn, rng(cw + 1, bw - 1), t_door_c);
                }
            }
            // Point on bedroom wall, for window
            rn = rng(rn + 2, rw - 2);
        } else {
            // Bedroom on left, bathroom on right
            house_room(m, room_bedroom, lw, cw, rn, bw, dat);

            // Put door between bedroom and kitchen
            m->ter_set(rng(lw + 1, rn > mw ? mw - 1 : rn - 1), cw, t_door_c);

            if (bw - cw >= 10 && rw - rn >= 6) {
                // All fits, placing bathroom and 2nd bedroom
                house_room(m, room_bathroom, rn, bw - 5, rw, bw, dat);
                house_room(m, room_bedroom, rn, cw, rw, bw - 5, dat);

                // Put door between bathroom and bedroom
                m->ter_set(rn, rng(bw - 4, bw - 1), t_door_c);

                if (one_in(3)) {
                    // Put door between 2nd bedroom and 1st bedroom
                    m->ter_set(rn, rng(cw + 1, bw - 6), t_door_c);
                } else {
                    // ...Otherwise, between 2nd bedroom and living
                    m->ter_set(rng(rw - 1, rn > mw ? rn + 1 : mw + 1), cw, t_door_c);
                }
            } else if (bw - cw > 4) {
                // Too big for a bathroom, not big enough for 2nd bedroom
                // Make it a bathroom anyway, but give the excess space back to
                // the living.
                house_room(m, room_bathroom, rn, bw - 4, rw, bw, dat);
                for (int i = rw - 1; i > rn && i > mw; i--) {
                    m->ter_set(i, cw, t_floor);
                }

                // Put door between excess space and bathroom
                m->ter_set(rng(rw - 1, rn + 1), bw - 4, t_door_c);

                // Put door between excess space and bedroom
                m->ter_set(rn, rng(cw + 1, bw - 5), t_door_c);
            } else {
                // Small enough to be a bathroom; make it one.
                house_room(m, room_bathroom, rn, cw, rw, bw, dat);

                if (one_in(5)) {
                    // Put door between bathroom and living with low chance
                    m->ter_set(rng(rw - 1, rn > mw ? rn + 1 : mw + 1), cw, t_door_c);
                } else {
                    // ...Otherwise, between bathroom and bedroom
                    m->ter_set(rn, rng(cw + 1, bw - 1), t_door_c);
                }
            }
            // Point on bedroom wall, for window
            rn = rng(lw + 2, rn - 2);
        }
        m->ter_set(rn    , bw, t_window_domestic);
        m->ter_set(rn + 1, bw, t_window_domestic);
        if (!one_in(3) && rw < SEEX * 2 - 1) { // Potential side windows
            rn = rng(tw + 2, bw - 6);
            m->ter_set(rw, rn    , t_window_domestic);
            m->ter_set(rw, rn + 4, t_window_domestic);
        }
        if (!one_in(3) && lw > 0) { // Potential side windows
            rn = rng(tw + 2, bw - 6);
            m->ter_set(lw, rn    , t_window_domestic);
            m->ter_set(lw, rn + 4, t_window_domestic);
        }
        if (one_in(2)) { // Placement of the main door
            m->ter_set(rng(lw + 2, mw - 1), tw, (one_in(6) ? (one_in(6) ? t_door_c : t_door_c_peep) : (one_in(6) ? t_door_locked : t_door_locked_peep)));
            if (one_in(5)) { // Placement of side door
                m->ter_set(rw, rng(tw + 2, cw - 2), (one_in(6) ? t_door_c : t_door_locked));
            }
        } else {
            m->ter_set(rng(mw + 1, rw - 2), tw, (one_in(6) ? (one_in(6) ? t_door_c : t_door_c_peep) : (one_in(6) ? t_door_locked : t_door_locked_peep)));
            if (one_in(5)) {
                m->ter_set(lw, rng(tw + 2, cw - 2), (one_in(6) ? t_door_c : t_door_locked));
            }
        }
        break;

    case 2: // Old-style; simple;
        //Modified by Jovan in 28 Aug 2013
        //Long narrow living room in front, big kitchen and HUGE bedroom
        bw = SEEX * 2 - 2;
        cw = tw + rng(3, 6);
        mw = rng(lw + 7, rw - 4);
        //int actual_house_height=bw-rng(4,6);
        //in some rare cases some rooms (especially kitchen and living room) may get rather small
        if ((tw <= 3) && ( abs((actual_house_height - 3) - cw) >= 3 ) ) {
            //everything is fine
            house_room(m, room_backyard, lw, actual_house_height + 1, rw, bw, dat);
            //door from bedroom to backyard
            m->ter_set((lw + mw) / 2, actual_house_height, t_door_c);
        } else { //using old layout
            actual_house_height = bw_old;
        }
        // Plop down the rooms
        house_room(m, room_living, lw, tw, rw, cw, dat);
        house_room(m, room_kitchen, mw, cw, rw, actual_house_height - 3, dat);
        house_room(m, room_bedroom, lw, cw, mw, actual_house_height, dat); //making bedroom smaller
        house_room(m, room_bathroom, mw, actual_house_height - 3, rw, actual_house_height, dat);

        // Space between kitchen & living room:
        rn = rng(mw + 1, rw - 3);
        m->ter_set(rn    , cw, t_floor);
        m->ter_set(rn + 1, cw, t_floor);
        // Front windows
        rn = rng(2, 5);
        m->ter_set(lw + rn    , tw, t_window_domestic);
        m->ter_set(lw + rn + 1, tw, t_window_domestic);
        m->ter_set(rw - rn    , tw, t_window_domestic);
        m->ter_set(rw - rn + 1, tw, t_window_domestic);
        // Front door
        m->ter_set(rng(lw + 4, rw - 4), tw, (one_in(6) ? t_door_c : t_door_locked));
        if (one_in(3)) { // Kitchen windows
            rn = rng(cw + 1, actual_house_height - 5);
            m->ter_set(rw, rn    , t_window_domestic);
            m->ter_set(rw, rn + 1, t_window_domestic);
        }
        if (one_in(3)) { // Bedroom windows
            rn = rng(cw + 1, actual_house_height - 2);
            m->ter_set(lw, rn    , t_window_domestic);
            m->ter_set(lw, rn + 1, t_window_domestic);
        }
        // Door to bedroom
        if (one_in(4)) {
            m->ter_set(rng(lw + 1, mw - 1), cw, t_door_c);
        } else {
            m->ter_set(mw, rng(cw + 3, actual_house_height - 4), t_door_c);
        }
        // Door to bathrom
        if (one_in(4)) {
            m->ter_set(mw, actual_house_height - 1, t_door_c);
        } else {
            m->ter_set(rng(mw + 2, rw - 2), actual_house_height - 3, t_door_c);
        }
        // Back windows
        rn = rng(lw + 1, mw - 2);
        m->ter_set(rn    , actual_house_height, t_window_domestic);
        m->ter_set(rn + 1, actual_house_height, t_window_domestic);
        rn = rng(mw + 1, rw - 1);
        m->ter_set(rn, actual_house_height, t_window_domestic);
        break;

    case 3: // Long center hallway, kitchen, living room and office
        mw = int((lw + rw) / 2);
        cw = bw - rng(5, 7);
        // Hallway doors and windows
        m->ter_set(mw    , tw, (one_in(6) ? t_door_c : t_door_locked));
        if (one_in(4)) {
            m->ter_set(mw - 1, tw, t_window_domestic);
            m->ter_set(mw + 1, tw, t_window_domestic);
        }
        for (int i = tw + 1; i < cw; i++) { // Hallway walls
            m->ter_set(mw - 2, i, t_wall);
            m->ter_set(mw + 2, i, t_wall);
        }
        if (one_in(2)) { // Front rooms are kitchen or living room
            house_room(m, room_living, lw, tw, mw - 2, cw, dat);
            house_room(m, room_kitchen, mw + 2, tw, rw, cw, dat);
        } else {
            house_room(m, room_kitchen, lw, tw, mw - 2, cw, dat);
            house_room(m, room_living, mw + 2, tw, rw, cw, dat);
        }
        // Front windows
        rn = rng(lw + 1, mw - 4);
        m->ter_set(rn    , tw, t_window_domestic);
        m->ter_set(rn + 1, tw, t_window_domestic);
        rn = rng(mw + 3, rw - 2);
        m->ter_set(rn    , tw, t_window_domestic);
        m->ter_set(rn + 1, tw, t_window_domestic);
        if (one_in(3) && lw > 0) { // Side windows?
            rn = rng(tw + 1, cw - 2);
            m->ter_set(lw, rn    , t_window_domestic);
            m->ter_set(lw, rn + 1, t_window_domestic);
        }
        if (one_in(3) && rw < SEEX * 2 - 1) { // Side windows?
            rn = rng(tw + 1, cw - 2);
            m->ter_set(rw, rn    , t_window_domestic);
            m->ter_set(rw, rn + 1, t_window_domestic);
        }
        if (one_in(2)) { // Bottom rooms are bedroom or bathroom
            //bathroom to the left (eastern wall), study to the right
            //house_room(m, room_bedroom, lw, cw, rw - 3, bw, dat);
            house_room(m, room_bedroom, mw - 2, cw, rw - 3, bw, dat);
            house_room(m, room_bathroom, rw - 3, cw, rw, bw, dat);
            house_room(m, room_study, lw, cw, mw - 2, bw, dat);
            //===Study Room Furniture==
            m->ter_set(mw - 2, (bw + cw) / 2, t_door_o);
            m->furn_set(lw + 1, cw + 1, f_chair);
            m->furn_set(lw + 1, cw + 2, f_table);
            m->ter_set(lw + 1, cw + 3, t_console_broken);
            m->furn_set(lw + 3, bw - 1, f_bookcase);
            m->place_items("magazines", 30,  lw + 3,  bw - 1, lw + 3,  bw - 1, false, 0);
            m->place_items("novels", 40,  lw + 3,  bw - 1, lw + 3,  bw - 1, false, 0);
            m->place_items("alcohol", 20,  lw + 3,  bw - 1, lw + 3,  bw - 1, false, 0);
            m->place_items("manuals", 30,  lw + 3,  bw - 1, lw + 3,  bw - 1, false, 0);
            //=========================
            m->ter_set(rng(lw + 2, mw - 3), cw, t_door_c);
            if (one_in(4)) {
                m->ter_set(rng(rw - 2, rw - 1), cw, t_door_c);
            } else {
                m->ter_set(rw - 3, rng(cw + 2, bw - 2), t_door_c);
            }
            rn = rng(mw, rw - 5); //bedroom windows
            m->ter_set(rn    , bw, t_window_domestic);
            m->ter_set(rn + 1, bw, t_window_domestic);
            m->ter_set(rng(lw + 2, mw - 3), bw, t_window_domestic); //study window

            if (one_in(4)) {
                m->ter_set(rng(rw - 2, rw - 1), bw, t_window_domestic);
            } else {
                m->ter(rw, rng(cw + 1, bw - 1));
            }
        } else { //bathroom to the right
            house_room(m, room_bathroom, lw, cw, lw + 3, bw, dat);
            //house_room(m, room_bedroom, lw + 3, cw, rw, bw, dat);
            house_room(m, room_bedroom, lw + 3, cw, mw + 2, bw, dat);
            house_room(m, room_study, mw + 2, cw, rw, bw, dat);
            //===Study Room Furniture==
            m->ter_set(mw + 2, (bw + cw) / 2, t_door_c);
            m->furn_set(rw - 1, cw + 1, f_chair);
            m->furn_set(rw - 1, cw + 2, f_table);
            m->ter_set(rw - 1, cw + 3, t_console_broken);
            m->furn_set(rw - 3, bw - 1, f_bookcase);
            m->place_items("magazines", 40,  rw - 3,  bw - 1, rw - 3,  bw - 1, false, 0);
            m->place_items("novels", 40,  rw - 3,  bw - 1, rw - 3,  bw - 1, false, 0);
            m->place_items("alcohol", 20,  rw - 3,  bw - 1, rw - 3,  bw - 1, false, 0);
            m->place_items("manuals", 20,  rw - 3,  bw - 1, rw - 3,  bw - 1, false, 0);
            //=========================

            if (one_in(4)) {
                m->ter_set(rng(lw + 1, lw + 2), cw, t_door_c);
            } else {
                m->ter_set(lw + 3, rng(cw + 2, bw - 2), t_door_c);
            }
            rn = rng(lw + 4, mw); //bedroom windows
            m->ter_set(rn    , bw, t_window_domestic);
            m->ter_set(rn + 1, bw, t_window_domestic);
            m->ter_set(rng(mw + 3, rw - 1), bw, t_window_domestic); //study window
            if (one_in(4)) {
                m->ter_set(rng(lw + 1, lw + 2), bw, t_window_domestic);
            } else {
                m->ter(lw, rng(cw + 1, bw - 1));
            }
        }
        // Doors off the sides of the hallway
        m->ter_set(mw - 2, rng(tw + 3, cw - 3), t_door_c);
        m->ter_set(mw + 2, rng(tw + 3, cw - 3), t_door_c);
        m->ter_set(mw, cw, t_door_c);
        break;
    } // Done with the various house structures
    //////
    if (rng(2, 7) < tw) { // Big front yard has a chance for a fence
        for (int i = lw; i <= rw; i++) {
            m->ter_set(i, 0, t_fence_h);
        }
        for (int i = 1; i < tw; i++) {
            m->ter_set(lw, i, t_fence_v);
            m->ter_set(rw, i, t_fence_v);
        }
        int hole = rng(SEEX - 3, SEEX + 2);
        m->ter_set(hole, 0, t_dirt);
        m->ter_set(hole + 1, 0, t_dirt);
        if (one_in(tw)) {
            m->ter_set(hole - 1, 1, t_tree_young);
            m->ter_set(hole + 2, 1, t_tree_young);
        }
    }

    // For rotation
    const bool has_basement = terrain_type->get_type_id().str() == "house_base";
    if( has_basement ) {
        const bool force = get_world_option<bool>( "ALIGN_STAIRS" );
        // Find the basement's stairs first
        const tripoint abs_sub_here = m->get_abs_sub();
        tinymap basement;
        basement.load( abs_sub_here.x, abs_sub_here.y, abs_sub_here.z - 1, false );
        std::vector<tripoint> upstairs;
        const tripoint from( 0, 0, abs_sub_here.z - 1 );
        const tripoint to( SEEX * 2, SEEY * 2, abs_sub_here.z - 1 );
        for( const tripoint &p : m->points_in_rectangle( from, to ) ) {
            if( basement.has_flag( TFLAG_GOES_UP, p ) ) {
                upstairs.emplace_back( p );
            }
        }

        bool placed_any = false;
        for( const tripoint &p : upstairs ) {
            static const tripoint up = tripoint( 0, 0, 1 );
            const tripoint here = om_direction::rotate( p + up, terrain_type->dir );
            // @todo Less ugly check
            // If aligning isn't forced, allow only floors. Otherwise allow all non-walls
            const ter_t &ter_here = m->ter( here ).obj();
            if( ( force && ter_here.movecost > 0 ) ||
                ( ter_here.has_flag( "INDOORS" ) && ter_here.has_flag( "FLAT" ) ) ) {
                m->ter_set( here, t_stairs_down );
                placed_any = true;
            }

            // Try to push away furniture
            const furn_id furn_here = m->furn( here );
            if( furn_here != f_null ) {
                for( const tripoint &push_point : m->points_in_radius( here, 1 ) ) {
                    if( m->furn( push_point ) == f_null ) {
                        m->furn_set( push_point, furn_here );
                        break;
                    }
                }

                m->furn_set( here, f_null );
            }
        }

        // If not forcing alignment and didn't place any stairs, allow legacy stair placement
        // Note: any, not all - legacy stairs wouldn't deal well with multiple random stairs
        if( !placed_any && !force ) {
            // Legacy stair spawning code - allows teleports
            int attempts = 100;
            int stairs_height = actual_house_height - 1;
            do {
                rn = rng( lw + 1, rw - 1 );
                // After 50 failed attempts, relax the placement limitations a bit
                // Otherwise it will most likely fail the next 50 too
                if( attempts < 50 ) {
                    stairs_height = rng( 1, SEEY );
                }
                attempts--;
                if( m->ter( rn, stairs_height ) == t_floor && !m->has_furn( rn, stairs_height ) ) {
                    m->ter_set( rn, stairs_height, t_stairs_down );
                    break;
                }
            } while( attempts > 0 );
        }
    }

    if (one_in(100)) { // todo: region data // Houses have a 1 in 100 chance of wasps!
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (m->ter(i, j) == t_door_c || m->ter(i, j) == t_door_locked) {
                    m->ter_set(i, j, t_door_frame);
                }
                if (m->ter(i, j) == t_window_domestic && !one_in(3)) {
                    m->ter_set(i, j, t_window_frame);
                }
                if (m->ter(i, j) == t_wall && one_in(8)) {
                    m->ter_set(i, j, t_paper);
                }
            }
        }
        int num_pods = rng(8, 12);
        for (int i = 0; i < num_pods; i++) {
            int podx = rng(1, SEEX * 2 - 2), pody = rng(1, SEEY * 2 - 2);
            int nonx = 0, nony = 0;
            while (nonx == 0 && nony == 0) {
                nonx = rng(-1, 1);
                nony = rng(-1, 1);
            }
            for (int x = -1; x <= 1; x++) {
                for (int y = -1; y <= 1; y++) {
                    if ((x != nonx || y != nony) && (x != 0 || y != 0)) {
                        m->ter_set(podx + x, pody + y, t_paper);
                    }
                }
            }
            m->add_spawn(mon_wasp, 1, podx, pody);
        }
        m->place_items("rare", 70, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, false, turn);

    } else if (one_in(150)) { // todo; region_data // No wasps; black widows?
        auto spider_type = mon_spider_widow_giant;
        auto egg_type = f_egg_sackbw;
    if( one_in(2) ) {
        spider_type = mon_spider_cellar_giant;
        egg_type = f_egg_sackcs;
        }
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (m->ter(i, j) == t_floor) {
                    if (one_in(15)) {
                        m->add_spawn(spider_type, rng(1, 2), i, j);
                        for (int x = i - 1; x <= i + 1; x++) {
                            for (int y = j - 1; y <= j + 1; y++) {
                                if (m->ter(x, y) == t_floor) {
                                    madd_field( m, x, y, fd_web, rng(2, 3));
                                    if (one_in(4)){
                                     m->furn_set(i, j, egg_type);
                                     m->remove_field({i, j, m->get_abs_sub().z}, fd_web);
                                    }
                                }
                            }
                        }
                    } else if (m->passable(i, j) && one_in(5)) {
                        madd_field( m, x, y, fd_web, 1);
                    }
                }
            }
        }
        m->place_items("rare", 60, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, false, turn);

    } else { // Just boring old zombies
        m->place_spawns( mongroup_id( "GROUP_ZOMBIE" ), 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);
    }

    m->rotate( static_cast<int>( terrain_type->dir ) );
}

/////////////////////////////
void mapgen_church_new_england(map *m, oter_id terrain_type, mapgendata dat, int, float) {
    computer *tmpcomp = NULL;
    dat.fill_groundcover();
    //New England or Country style, single centered steeple low clear windows
    mapf::formatted_set_simple(m, 0, 0,"\
         ^^^^^^         \n\
     |---|--------|     \n\
    ||dh.|.6ooo.ll||    \n\
    |l...+.........Dsss \n\
  ^^|--+-|------+--|^^s \n\
  ^||..............||^s \n\
   w.......tt.......w s \n\
   |................| s \n\
  ^w................w^s \n\
  ^|.######..######.|^s \n\
  ^w................w^s \n\
  ^|.######..######.|^s \n\
   w................w s \n\
   |.######..######.| s \n\
  ^w................w^s \n\
  ^|.######..######.|^s \n\
  ^|................|^s \n\
   |-w|----..----|w-| s \n\
    ^^|ll|....|ST|^^  s \n\
     ^|.......+..|^   s \n\
      |----++-|--|    s \n\
         O ss O       s \n\
     ^^    ss    ^^   s \n\
     ^^    ss    ^^   s \n",
       mapf::ter_bind("O 6 ^ . - | # t + = D w T S e o h c d l s", t_column, t_console, t_shrub, t_floor,
               t_wall, t_wall, t_floor, t_floor, t_door_c, t_door_locked_alarm, t_door_locked, t_window,
               t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor, t_floor,  t_sidewalk),
       mapf::furn_bind("O 6 ^ . - | # t + = D w T S e o h c d l s", f_null,   f_null,    f_null,  f_null,
               f_null,   f_null,   f_bench, f_table, f_null,   f_null,              f_null,        f_null,
               f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,  f_locker, f_null)
    );
    m->spawn_item(9, 6, "brazier");
    m->spawn_item(14, 6, "brazier");
    m->place_items("church", 40,  5,  5, 8,  16, false, 0);
    m->place_items("church", 40,  5,  5, 8,  16, false, 0);
    m->place_items("church", 85,  12,  2, 14,  2, false, 0);
    m->place_items("office", 60,  6,  2, 8,  3, false, 0);
    m->place_items("jackets", 85,  7,  18, 8,  18, false, 0);
    tmpcomp = m->add_computer( tripoint( 11, 2, m->get_abs_sub().z ), _("Church Bells 1.2"), 0);
    tmpcomp->add_option(_("Gathering Toll"), COMPACT_TOLL, 0);
    tmpcomp->add_option(_("Wedding Toll"), COMPACT_TOLL, 0);
    tmpcomp->add_option(_("Funeral Toll"), COMPACT_TOLL, 0);
    autorotate(true);
}

void mapgen_church_gothic(map *m, oter_id terrain_type, mapgendata dat, int, float) {
    computer *tmpcomp = NULL;
    dat.fill_groundcover();
    //Gothic Style, unreachable high stained glass windows, stone construction
    mapf::formatted_set_simple(m, 0, 0, "\
 $$    W        W    $$ \n\
 $$  WWWWGVBBVGWWWW  $$ \n\
     W..h.cccc.h..W     \n\
 WWVWWR..........RWWBWW \n\
WW#.#.R....cc....R.#.#WW\n\
 G#.#.R..........R.#.#V \n\
 V#.#.Rrrrr..rrrrR.#.#B \n\
WW#..................#WW\n\
 WW+WW#####..#####WW+WW \n\
ssss V............B ssss\n\
s   WW#####..#####WW   s\n\
s $ WW............WW $ s\n\
s $  G#####..#####V  $ s\n\
s $  B............G  $ s\n\
s $ WW#####..#####WW $ s\n\
s $ WW............WW $ s\n\
s    V####....####B    s\n\
s WWWW--|--gg-----WWWW s\n\
s WllWTS|.....lll.W..W s\n\
s W..+..+.........+..W s\n\
s W..WWWWWW++WWWWWW6.W s\n\
s W.CWW$$WWssWW$$WW..W s\n\
s WWWWW    ss    WWWWW s\n\
ssssssssssssssssssssssss\n",
       mapf::ter_bind("C V G B W R r 6 $ . - | # t + g T S h c l s", t_floor,   t_window_stained_red,
               t_window_stained_green, t_window_stained_blue, t_rock, t_railing_v, t_railing_h, t_console, t_shrub,
               t_rock_floor, t_wall, t_wall, t_rock_floor, t_rock_floor, t_door_c, t_door_glass_c,
               t_rock_floor, t_rock_floor, t_rock_floor, t_rock_floor, t_rock_floor, t_sidewalk),
       mapf::furn_bind("C V G B W R r 6 $ . - | # t + g T S h c l s", f_crate_c, f_null,
               f_null,                 f_null,                f_null, f_null,      f_null,      f_null,    f_null,
               f_null,       f_null,   f_null,   f_bench,      f_table,      f_null,   f_null,         f_toilet,
               f_sink,       f_chair,      f_counter,    f_locker,     f_null)
    );
    m->spawn_item(8, 4, "brazier");
    m->spawn_item(15, 4, "brazier");
    m->place_items("church", 70,  6,  7, 17,  16, false, 0);
    m->place_items("church", 70,  6,  7, 17,  16, false, 0);
    m->place_items("church", 60,  6,  7, 17,  16, false, 0);
    m->place_items("cleaning", 60,  3,  18, 4,  21, false, 0);
    m->place_items("jackets", 85,  14,  18, 16,  18, false, 0);
    tmpcomp = m->add_computer( tripoint( 19, 20, m->get_abs_sub().z ), _("Church Bells 1.2"), 0);
    tmpcomp->add_option(_("Gathering Toll"), COMPACT_TOLL, 0);
    tmpcomp->add_option(_("Wedding Toll"), COMPACT_TOLL, 0);
    tmpcomp->add_option(_("Funeral Toll"), COMPACT_TOLL, 0);
    autorotate(true);
}



//////////////////////////////
void mapgen_pharm(map *m, oter_id terrain_type, mapgendata dat, int, float density) {

    int lw = 0;
    int rw = 0;
    int mw = 0;
    int tw = 0;
    int bw = 0;
    int cw = 0;


        tw = rng(0, 4);
        bw = SEEY * 2 - rng(1, 5);
        mw = bw - rng(3, 4); // Top of the storage room
        lw = rng(0, 4);
        rw = SEEX * 2 - rng(1, 5);
        cw = rng(13, rw - 5); // Left side of the storage room
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (j == tw && ((i > lw + 2 && i < lw + 6) || (i > rw - 6 && i < rw - 2))) {
                    m->ter_set(i, j, t_window);
                } else if ((j == tw && (i == lw + 8 || i == lw + 9)) ||
                           (i == cw && j == mw + 1)) {
                    m->ter_set(i, j, t_door_c);
                } else if (((j == tw || j == bw) && i >= lw && i <= rw) ||
                           (j == mw && i >= cw && i < rw)) {
                    m->ter_set(i, j, t_wall);
                } else if (((i == lw || i == rw) && j > tw && j < bw) ||
                           (i == cw && j > mw && j < bw)) {
                    m->ter_set(i, j, t_wall);
                } else if (((i == lw + 8 || i == lw + 9 || i == rw - 4 || i == rw - 3) &&
                            j > tw + 3 && j < mw - 2) ||
                           (j == bw - 1 && i > lw + 1 && i < cw - 1)) {
                    m->set(i, j, t_floor, f_rack);
                } else if ((i == lw + 1 && j > tw + 8 && j < mw - 1) ||
                           (j == mw - 1 && i > cw + 1 && i < rw)) {
                    m->set(i, j, t_floor, f_glass_fridge);
                } else if ((j == mw     && i > lw + 1 && i < cw) ||
                           (j == tw + 6 && i > lw + 1 && i < lw + 6) ||
                           (i == lw + 5 && j > tw     && j < tw + 7)) {
                    m->set(i, j, t_floor, f_counter);
                } else if (i > lw && i < rw && j > tw && j < bw) {
                    m->ter_set(i, j, t_floor);
                } else {
                    m->ter_set(i, j, dat.groundcover());
                }
            }
        }

        {
            int num_carts = rng(0, 5);
            for( int i = 0; i < num_carts; i++ ) {
                m->add_vehicle( vproto_id( "shopping_cart" ), rng(lw, cw), rng(tw, mw), 90);
            }
        }

        if (one_in(3)) {
            m->place_items("snacks", 74, lw + 8, tw + 4, lw + 8, mw - 3, false, 0);
        } else if (one_in(4)) {
            m->place_items("cleaning", 74, lw + 8, tw + 4, lw + 8, mw - 3, false, 0);
        } else {
            m->place_items("magazines", 74, lw + 8, tw + 4, lw + 8, mw - 3, false, 0);
        }
        if (one_in(5)) {
            m->place_items("softdrugs", 84, lw + 9, tw + 4, lw + 9, mw - 3, false, 0);
        } else if (one_in(4)) {
            m->place_items("cleaning", 74, lw + 9, tw + 4, lw + 9, mw - 3, false, 0);
        } else {
            m->place_items("snacks", 74, lw + 9, tw + 4, lw + 9, mw - 3, false, 0);
        }
        if (one_in(5)) {
            m->place_items("softdrugs", 84, rw - 4, tw + 4, rw - 4, mw - 3, false, 0);
        } else {
            m->place_items("snacks", 74, rw - 4, tw + 4, rw - 4, mw - 3, false, 0);
        }
        if (one_in(3)) {
            m->place_items("snacks", 70, rw - 3, tw + 4, rw - 3, mw - 3, false, 0);
        } else {
            m->place_items("softdrugs", 80, rw - 3, tw + 4, rw - 3, mw - 3, false, 0);
        }
        m->place_items("fridgesnacks", 74, lw + 1, tw + 9, lw + 1, mw - 2, false, 0);
        m->place_items("fridgesnacks", 74, cw + 2, mw - 1, rw - 1, mw - 1, false, 0);
        m->place_items("harddrugs", 88, lw + 2, bw - 1, cw - 2, bw - 1, false, 0);
        m->place_items("behindcounter", 78, lw + 1, tw + 1, lw + 4, tw + 5, false, 0);
        autorotate(false);
        m->place_spawns( mongroup_id( "GROUP_PHARM" ), 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);

}

void mapgen_s_sports(map *m, oter_id terrain_type, mapgendata dat, int, float density) {
//    } else if (is_ot_type("s_sports", terrain_type)) {
  int rn = 0;
    int lw = 0;
    int rw = 0;
    int tw = 0;
    int bw = 0;
    int cw = 0;


        lw = rng(0, 3);
        rw = SEEX * 2 - 1 - rng(0, 3);
        tw = rng(3, 10);
        bw = SEEY * 2 - 1 - rng(0, 3);
        cw = bw - rng(3, 5);
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (((j == tw || j == bw) && i >= lw && i <= rw) ||
                    (j == cw && i > lw && i < rw)) {
                    m->ter_set(i, j, t_wall);
                } else if ((i == lw || i == rw) && j > tw && j < bw) {
                    m->ter_set(i, j, t_wall);
                } else if ((j == cw - 1 && i > lw && i < rw - 4) ||
                           (j < cw - 3 && j > tw && (i == lw + 1 || i == rw - 1))) {
                    m->set(i, j, t_floor, f_rack);
                } else if (j == cw - 3 && i > lw && i < rw - 4) {
                    m->set(i, j, t_floor, f_counter);
                } else if (j > tw && j < bw && i > lw && i < rw) {
                    m->ter_set(i, j, t_floor);
                } else if (tw >= 6 && j >= tw - 6 && j < tw && i >= lw && i <= rw) {
                    if ((i - lw) % 4 == 0) {
                        m->ter_set(i, j, t_pavement_y);
                    } else {
                        m->ter_set(i, j, t_pavement);
                    }
                } else {
                    m->ter_set(i, j, dat.groundcover());
                }
            }
        }
        rn = rng(tw + 2, cw - 6);
        for (int i = lw + 3; i <= rw - 5; i += 4) {
            if (cw - 6 > tw + 1) {
                m->furn_set(i    , rn + 1, f_rack);
                m->furn_set(i    , rn    , f_rack);
                m->furn_set(i + 1, rn + 1, f_rack);
                m->furn_set(i + 1, rn    , f_rack);
                m->place_items("camping", 86, i, rn, i + 1, rn + 1, false, 0);
            } else if (cw - 5 > tw + 1) {
                m->furn_set(i    , cw - 5, f_rack);
                m->furn_set(i + 1, cw - 5, f_rack);
                m->place_items("camping", 80, i, cw - 5, i + 1, cw - 5, false, 0);
            }
        }
        m->ter_set(rw - rng(2, 3), cw, t_door_c);
        rn = rng(2, 4);
        for (int i = lw + 2; i <= lw + 2 + rn; i++) {
            m->ter_set(i, tw, t_window);
        }
        for (int i = rw - 2; i >= rw - 2 - rn; i--) {
            m->ter_set(i, tw, t_window);
        }
        m->ter_set(rng(lw + 3 + rn, rw - 3 - rn), tw, t_door_c);
        if (one_in(4)) {
            m->ter_set(rng(lw + 2, rw - 2), bw, t_door_locked);
        }
        m->place_items("allsporting", 90, lw + 1, cw - 1, rw - 5, cw - 1, false, 0);
        m->place_items("sports", 82, lw + 1, tw + 1, lw + 1, cw - 4, false, 0);
        m->place_items("sports", 82, rw - 1, tw + 1, rw - 1, cw - 4, false, 0);
        if (!one_in(4)) {
            m->place_items("allsporting", 92, lw + 1, cw + 1, rw - 1, bw - 1, false, 0);
        }
        autorotate(false);
        m->place_spawns( mongroup_id( "GROUP_ZOMBIE" ), 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);
}

////////////////////
//    } else if (terrain_type == "shelter") {
void mapgen_shelter(map *m, oter_id, mapgendata dat, int, float) {
    static const mongroup_id GROUP_ZOMBIE( "GROUP_ZOMBIE" );

        // Init to grass & dirt;
        dat.fill_groundcover();
        square(m, t_floor, 5, 5, SEEX * 2 - 6, SEEY * 2 - 6);
        mapf::formatted_set_simple(m, 4, 4,
                                   "\
|----:-++-:----|\n\
|llll      c  6|\n\
| b b b    c   |\n\
| b b b    c   |\n\
| b b b    c   |\n\
: b b b        :\n\
|              |\n\
+      >>      +\n\
+      >>      +\n\
|              |\n\
: b b b        :\n\
| b b b    c   |\n\
| b b b    c   |\n\
| b b b    c   |\n\
|          c  x|\n\
|----:-++-:----|\n",
                                   mapf::ter_bind("- | + : 6 x >", t_wall, t_wall, t_door_c, t_window_domestic,  t_console,
                                           t_console_broken, t_stairs_down),
                                   mapf::furn_bind("b c l", f_bench, f_counter, f_locker));
        computer * tmpcomp = m->add_computer( tripoint( SEEX + 6, 5, m->get_abs_sub().z ), _("Evac shelter computer"), 0);
        tmpcomp->add_option(_("Emergency Message"), COMPACT_EMERG_MESS, 0);
        tmpcomp->add_option(_("Disable External Power"), COMPACT_COMPLETE_MISSION, 0);
        tmpcomp->add_option(_("Contact Us"), COMPACT_EMERG_REF_CENTER, 0);
        int lx = rng(5 , 8);
        // The shelter does have some useful stuff in case of winter problems!
        m->spawn_item(lx, 5, "jacket_evac");
        m->spawn_item(lx, 5, "emer_blanket");
        if (one_in(3)) {
            int lxa = rng(5 , 8);
            m->spawn_item(lxa, 5, "jacket_evac");
            m->spawn_item(lxa, 5, "emer_blanket");
            if (one_in(2)) {
                m->spawn_item(lxa, 5, "mask_gas"); // See! The gas mask is real!
            }
        }
        if(get_world_option<bool>( "BLACK_ROAD" ) || g->scen->has_flag("SUR_START")) {
            //place zombies outside
            m->place_spawns( GROUP_ZOMBIE, get_world_option<float>( "SPAWN_DENSITY" ), 0, 0, SEEX * 2 - 1, 3, 0.4f);
            m->place_spawns( GROUP_ZOMBIE, get_world_option<float>( "SPAWN_DENSITY" ), 0, 4, 3, SEEX * 2 - 4, 0.4f);
            m->place_spawns( GROUP_ZOMBIE, get_world_option<float>( "SPAWN_DENSITY" ), SEEX * 2 - 3, 4,
                         SEEX * 2 - 1, SEEX * 2 - 4, 0.4f);
            m->place_spawns( GROUP_ZOMBIE, get_world_option<float>( "SPAWN_DENSITY" ), 0, SEEX * 2 - 3,
                         SEEX * 2 - 1, SEEX * 2 - 1, 0.4f);
        }
}


void mapgen_shelter_under(map *m, oter_id, mapgendata dat, int, float) {
//    } else if (terrain_type == "shelter_under") {

(void)dat;
        // Make the whole area rock, then plop an open area in the center.
        square(m, t_rock, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1);
        square(m, t_rock_floor, 6, 6, SEEX * 2 - 8, SEEY * 2 - 8);
        // Create an anteroom with the stairs and some locked doors.
        m->ter_set(SEEX - 1, SEEY * 2 - 7, t_door_locked);
        m->ter_set(SEEX    , SEEY * 2 - 7, t_door_locked);
        m->ter_set(SEEX - 1, SEEY * 2 - 6, t_rock_floor);
        m->ter_set(SEEX    , SEEY * 2 - 6, t_rock_floor);
        m->ter_set(SEEX - 1, SEEY * 2 - 5, t_stairs_up);
        m->ter_set(SEEX    , SEEY * 2 - 5, t_stairs_up);
        if( one_in(10) ) {
            // Scatter around lots of items and some zombies.
            for( int x = 0; x < 10; ++x ) {
                m->place_items("shelter", 90, 6, 6, SEEX * 2 - 8, SEEY * 2 - 8, false, 0);
            }
            m->place_spawns( mongroup_id( "GROUP_ZOMBIE" ), 1, 6, 6, SEEX * 2 - 8, SEEX * 2 - 8, 0.2);
        } else {
            // Scatter around some items.
            m->place_items("shelter", 80, 6, 6, SEEX * 2 - 8, SEEY * 2 - 8, false, 0);
        }
}


void mapgen_lmoe(map *m, oter_id, mapgendata dat, int, float) {
//    } else if (terrain_type == "lmoe") {

        // Init to grass & dirt;
        dat.fill_groundcover();
        square(m, t_shrub, 7, 6, 16, 12);
        square(m, t_rock, 10, 9, 13, 12);
        square(m, t_rock_floor, 11, 10, 12, 11);
        line(m, t_stairs_down, 11, 10, 12, 10);
        m->ter_set(11, 12, t_door_metal_c);
        line(m, t_tree, 9, 8, 14, 8);
        line(m, t_tree, 9, 8, 9, 12);
        line(m, t_tree, 14, 8, 14, 12);
        square(m, t_shrub, 13, 13, 15, 14);
        square(m, t_shrub, 8, 13, 10, 14);
        m->ter_set(10, 6, t_tree_young);
        m->ter_set(14, 6, t_tree_young);
        line(m, t_tree_young, 9, 7, 10, 7);
        m->ter_set(12, 7, t_tree_young);
        m->ter_set(14, 7, t_tree_young);
        m->ter_set(8, 9, t_tree_young);
        line(m, t_tree_young, 7, 11, 8, 11);
        line(m, t_tree_young, 15, 10, 15, 11);
        m->ter_set(16, 12, t_tree_young);
        m->ter_set(9, 13, t_tree_young);
        m->ter_set(12, 13, t_tree_young);
        m->ter_set(16, 12, t_tree_young);
        line(m, t_tree_young, 14, 13, 15, 13);
        m->ter_set(10, 14, t_tree_young);
        m->ter_set(13, 14, t_tree_young);
}



///////////////////////////////////////////////////////////
void mapgen_basement_generic_layout(map *m, oter_id, mapgendata, int, float)
{
    const ter_id t_rock_smooth( "t_rock_smooth" );
    const int up = 0;
    const int left = 0;
    const int down = SEEY * 2 - 5;
    const int right = SEEX * 2 - 1;
    square( m, t_rock, left, down, right, SEEY * 2 - 1 );
    square( m, t_rock_floor, 1, 1, right - 1, down - 1 );
    line( m, t_rock_smooth, left, up, right, up );
    line( m, t_rock_smooth, left, down, right, down );
    line( m, t_rock_smooth, left, up, left, down );
    line( m, t_rock_smooth, right, up, right, down );
    m->ter_set( SEEX - 1, down - 1, t_stairs_up );
    m->ter_set( SEEX    , down - 1, t_stairs_up );
    line( m, t_rock_smooth, SEEX - 2, down - 1, SEEX - 2, down - 3 );
    line( m, t_rock_smooth, SEEX + 1, down - 1, SEEX + 1, down - 3 );
    line( m, t_door_locked, SEEX - 1, down - 3, SEEX, down - 3 );

    // Rotate randomly, now that other basements are more generic
    m->rotate( rng( 0, 3 ) );
}

namespace furn_space {
bool clear( const map &m, const tripoint &from, const tripoint &to )
{
    for( const auto &p : m.points_in_rectangle( from, to ) ) {
        if( m.ter( p ).obj().movecost == 0 ) {
            return false;
        }
    }

    return true;
}

point best_expand( const map &m, const tripoint &from, int maxx, int maxy )
{
    if( clear( m, from, from + point( maxx, maxy ) ) ) {
        // Common case
        return point( maxx, maxy );
    }

    // Brute force all the combinations for the one with biggest area
    int best_area = 0;
    point best;
    for( int x = 0; x <= maxx; x++ ) {
        for( int y = 0; y <= maxy; y++ ) {
            int area = x * y;
            if( area <= best_area ) {
                continue;
            }

            if( clear( m, from, from + point( x, y ) ) ) {
                best_area = area;
                best = point( x, y );
            }
        }
    }

    return best;
}
}

void mapgen_basement_junk(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    // Junk!
    mapgen_basement_generic_layout(m, terrain_type, dat, turn, density);
    //makes a square of randomly thrown around furniture and places stuff.
    const int z = m->get_abs_sub().z;
    for( const auto &p : m->points_in_rectangle( tripoint( 1, 1, z ), tripoint( 22, 22, z ) ) ) {
        if( m->ter( p ).obj().movecost == 0 ) {
            // Wall, skip
            continue;
        }

        if( one_in( 1600 ) ) {
            m->furn_set( p, furn_str_id( "f_gun_safe_el" ) );
            if( one_in( 2 ) ) {
                m->spawn_item( p, "9mm", 2 );
                m->spawn_item( p, "usp_9mm" );
                m->spawn_item( p, "suppressor" );
                m->spawn_item( p, "cash_card", 2 );
            } else {
                m->place_items( "ammo", 96,  p.x,  p.y, p.x,  p.y, false, 0 );
                m->place_items( "guns_survival", 90,  p.x,  p.y, p.x,  p.y, false, 0 );
            }
        }
        if( one_in( 20 ) ){
            int rn = rng( 1, 8 );
            if( rn == 1 ){
                m->furn_set( p, f_dresser );
                m->place_items( "dresser", 30,  p.x,  p.y, p.x,  p.y, false, 0 );
                m->place_items( "trash_forest", 60,  p.x,  p.y, p.x,  p.y, false, 0 );
            } else if( rn == 2 ){
                m->furn_set( p, f_chair );
            } else if( rn == 3 ){
                m->furn_set( p, f_cupboard );
                m->place_items( "trash", 60,  p.x,  p.y, p.x,  p.y, false, 0 );
                m->place_items( "dining", 40,  p.x,  p.y, p.x,  p.y, false, 0 );
            } else if( rn == 4 ){
                tripoint rs = p + furn_space::best_expand( *m, p, rng( 0, 4 ), 0 );
                square_furn( m, f_bookcase, p.x, p.y, rs.x, rs.y );
                m->place_items( "novels", 60,  p.x,  p.y, rs.x, rs.y, false, 0 );
                m->place_items( "magazines", 20,  p.x,  p.y, rs.x, rs.y, false, 0 );
            } else if( rn == 5 ){
                tripoint rs = p + furn_space::best_expand( *m, p, 0, rng( 0, 4 ) );
                square_furn( m, f_bookcase, p.x, p.y, rs.x, rs.y );
                m->place_items( "novels", 60,  p.x,  p.y, rs.x, rs.y, false, 0 );
                m->place_items( "magazines", 20,  p.x,  p.y, rs.x, rs.y, false, 0 );
            } else if( rn == 6 ){
                tripoint rs = p + furn_space::best_expand( *m, p, rng( 0, 2 ), 0 );
                square_furn( m, f_locker, p.x, p.y, rs.x, rs.y );
                m->place_items( "trash", 60, p.x, p.y, rs.x, rs.y, false, 0 );
                m->place_items( "home_hw", 20, p.x, p.y, rs.x, rs.y, false, 0 );
            } else if( rn == 7 ){
                tripoint rs = p + furn_space::best_expand( *m, p, 0, rng( 0, 2 ) );
                square_furn( m, f_locker, p.x, p.y, rs.x, rs.y );
                m->place_items( "trash", 60, p.x,  p.y, rs.x, rs.y, false, 0 );
                m->place_items( "home_hw", 20, p.x, p.y, rs.x, rs.y, false, 0 );
            } else {
                tripoint rs = p + furn_space::best_expand( *m, p, rng( 0, 2 ), rng( 0, 2 ) );
                square_furn( m, f_table, p.x, p.y, rs.x, rs.y );
            }
        }
    }

    m->place_items( "bedroom", 60, 1, 1, SEEX * 2 - 2, SEEY * 2 - 2, false, 0 );
    m->place_items( "home_hw", 80, 1, 1, SEEX * 2 - 2, SEEY * 2 - 2, false, 0 );
    m->place_items( "homeguns", 10, 1, 1, SEEX * 2 - 2, SEEY * 2 - 2, false, 0 );
    // Chance of zombies in the basement
    m->place_spawns( mongroup_id( "GROUP_ZOMBIE" ), 2, 1, 1, SEEX * 2 - 2, SEEY * 2 - 2, density );
}

void mapgen_basement_spiders(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    // Oh no! A spider nest!
    mapgen_basement_junk(m, terrain_type, dat, turn, density);
    auto spider_type = mon_spider_widow_giant;
    auto egg_type = f_egg_sackbw;
    if( one_in(2) ) {
        spider_type = mon_spider_cellar_giant;
        egg_type = f_egg_sackcs;
    }
    for (int i = 1; i < 22; i++) {
        for (int j = 1; j < 22; j++) {
            if( m->ter( i, j ).obj().movecost == 0 ) {
                // Wall, skip
                continue;
            }

            if( !one_in( 3 ) ) {
                madd_field( m, i, j, fd_web, rng(1, 3));
            }
            if( one_in( 30 ) && m->passable( i, j ) ) {
                m->furn_set( i, j, egg_type );
                m->add_spawn( spider_type, rng( 1, 2 ), i, j ); //hope you like'em spiders
                m->remove_field( { i, j, m->get_abs_sub().z }, fd_web );
            }
        }
    }
    m->place_items("rare", 70, 1, 1, SEEX * 2 - 1, SEEY * 2 - 5, false, turn);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////
void mapgen_office_doctor(map *m, oter_id terrain_type, mapgendata dat, int, float)
{

//    } else if (is_ot_type("office_doctor", terrain_type)) {

        // Init to grass & dirt;
        dat.fill_groundcover();
        mapf::formatted_set_simple(m, 0, 0,
                                   "\
                        \n\
   |---|----|--------|  \n\
   |..l|.T.S|..eccScc|  \n\
   |...+....+........D  \n\
   |--------|.......6|r \n\
   |o.......|..|--X--|r \n\
   |d.hd.h..+..|l...6|  \n\
   |o.......|..|l...l|  \n\
   |--------|..|l...l|  \n\
   |l....ccS|..|lllll|  \n\
   |l..t....+..|-----|  \n\
   |l.......|..|....l|  \n\
   |--|-----|..|.t..l|  \n\
   |T.|d.......+....l|  \n\
   |S.|d.h..|..|Scc..|  \n\
   |-+|-ccc-|++|-----|  \n\
   |.................|  \n\
   w....####....####.w  \n\
   w.................w  \n\
   |....####....####.|  \n\
   |.................|  \n\
   |-++--wwww-wwww---|  \n\
     ss                 \n\
     ss                 \n",
                                   mapf::ter_bind(". - | 6 X # r t + = D w T S e o h c d l s", t_floor, t_wall, t_wall,
                                           t_console, t_door_metal_locked, t_floor, t_floor,    t_floor, t_door_c, t_door_locked_alarm,
                                           t_door_locked, t_window, t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor,
                                           t_floor,  t_sidewalk),
                                   mapf::furn_bind(". - | 6 X # r t + = D w T S e o h c d l s", f_null,  f_null,   f_null,   f_null,
                                           f_null,              f_bench, f_trashcan, f_table, f_null,   f_null,              f_null,
                                           f_null,   f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,  f_locker, f_null));
        computer * tmpcomp = m->add_computer( tripoint( 20, 4, m->get_abs_sub().z ), _("Medical Supply Access"), 2);
        tmpcomp->add_option(_("Lock Door"), COMPACT_LOCK, 2);
        tmpcomp->add_option(_("Unlock Door"), COMPACT_UNLOCK, 2);
        tmpcomp->add_failure(COMPFAIL_SHUTDOWN);
        tmpcomp->add_failure(COMPFAIL_ALARM);

        tmpcomp = m->add_computer( tripoint( 20, 6, m->get_abs_sub().z ), _("Medical Supply Access"), 2);
        tmpcomp->add_option(_("Unlock Door"), COMPACT_UNLOCK, 2);
        tmpcomp->add_failure(COMPFAIL_SHUTDOWN);
        tmpcomp->add_failure(COMPFAIL_ALARM);

        if (one_in(2)) {
            m->spawn_item(7, 6, "record_patient");
        }
        m->place_items("dissection", 60,  4,  9, 4,  11, false, 0);
        m->place_items("dissection", 60,  9,  9, 10,  9, false, 0);
        m->place_items("dissection", 60,  20,  11, 20,  13, false, 0);
        m->place_items("dissection", 60,  17,  14, 18,  14, false, 0);
        m->place_items("fridge", 50,  15,  2, 15,  2, false, 0);
        m->place_items("gear_medical", 30,  4,  9, 11,  11, false, 0);
        m->place_items("gear_medical", 30,  16,  11, 20, 4, false, 0);
        m->place_items("harddrugs", 60,  16,  6, 16, 9, false, 0);
        m->place_items("harddrugs", 60,  17,  9, 19, 9, false, 0);
        m->place_items("softdrugs", 60,  20,  9, 20, 7, false, 0);
        m->place_items("cleaning", 50,  4,  2, 6,  3, false, 0);

        autorotate(true);

}

void mapgen_office_tower_1_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_office_tower_1(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_office_tower_b_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_office_tower_b(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_cathedral_1_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_cathedral_1(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_cathedral_b_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_cathedral_b(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_sub_station(map *m, oter_id terrain_type, mapgendata dat, int, float)
{
    (void)dat;
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (j < 9 || j > 12 || i < 4 || i > 19) {
                    m->ter_set(i, j, t_pavement);
                } else if (j < 12 && j > 8 && (i == 4 || i == 19)) {
                    m->ter_set(i, j, t_wall);
                } else if (i > 3 && i < 20 && j == 12) {
                    m->ter_set(i, j, t_wall);
                } else {
                    m->ter_set(i, j, t_floor);
                }
            }
        }
        //vending
        bool drinks = rng(0,1);
        std::string type;
        std::string type2;
        if (drinks) {
            type = "vending_drink";
            type2 = "vending_food";
        } else {
            type2 = "vending_drink";
            type = "vending_food";
        }
        int vset = rng(0,17);
        if (vset < 3) m->place_vending(5, vset+9, type);
        else if (vset < 15) m->place_vending(5 + (vset - 3), 11, type);
        else m->place_vending(18, 11 - (vset - 15), type);
        if(one_in(3))
        {
            int vset2 = rng(0,16);
            if(vset2 >= vset) vset2++;
            if (vset2 < 3) m->place_vending(5, vset2+9, type2);
            else if (vset2 < 15) m->place_vending(5 + (vset2 - 3), 11, type2);
            else m->place_vending(18, 11 - (vset2 - 15), type2);
        }
        //
        m->ter_set(16, 10, t_stairs_down);
        autorotate(false);

}


void mapgen_s_garage(map *m, oter_id terrain_type, mapgendata dat, int, float)
{

        dat.fill_groundcover();
        int yard_wdth = rng(4,6);
        square(m, t_floor, 0, yard_wdth, SEEX * 2 - 4, SEEY * 2 - 4);
        line(m, t_wall, 0, yard_wdth, 0, SEEY * 2 - 4);
        line(m, t_wall, SEEX * 2 - 3, yard_wdth, SEEX * 2 - 3, SEEY * 2 - 4);
        line(m, t_wall, 0, SEEY * 2 - 4, SEEX * 2 - 3, SEEY * 2 - 4);
        line(m, t_window, 0, SEEY * 2 - 4, SEEX * 2 - 14, SEEY * 2 - 4);
        line(m, t_wall, 0, SEEY * 2 - 4, SEEX * 2 - 20, SEEY * 2 - 4);
        line(m, t_wall, 0, yard_wdth, 3, yard_wdth);
        line(m, t_wall, 12, yard_wdth, 13, yard_wdth);
        line(m, t_wall, 20, yard_wdth, 21, yard_wdth);
        line_furn(m, f_counter, 1, yard_wdth + 1, 1, yard_wdth + 7);
        line(m, t_wall, 1, SEEY * 2 - 9, 3, SEEY * 2 - 9);
        line(m, t_wall, 3, SEEY * 2 - 8, 3, SEEY * 2 - 5);
        m->ter_set(3, SEEY * 2 - 7, t_door_frame);
        m->ter_set(21, SEEY * 2 - 7, t_door_c);
        line_furn(m, f_counter, 4, SEEY * 2 - 5, 15, SEEY * 2 - 5);
        //office
        line(m, t_wall_glass, 16, SEEY * 2 - 9 , 20, SEEY * 2 - 9);
        line(m, t_wall_glass, 16, SEEY * 2 - 8, 16, SEEY * 2 - 5);
        m->ter_set(16, SEEY * 2 - 7, t_door_glass_c);
        line_furn(m, f_bench, SEEX * 2 - 6, SEEY * 2 - 8, SEEX * 2 - 4, SEEY * 2 - 8);
        m->ter_set(SEEX * 2 - 6, SEEY * 2 - 6, t_console_broken);
        m->furn_set(SEEX * 2 - 5, SEEY * 2 - 6, f_bench);
        line_furn(m, f_locker, SEEX * 2 - 6, SEEY * 2 - 5, SEEX * 2 - 4, SEEY * 2 - 5);
        //gates
        line(m, t_door_metal_locked, 4, yard_wdth, 11, yard_wdth);
        m->ter_set(3, yard_wdth + 1, t_gates_mech_control);
        m->ter_set(3, yard_wdth - 1, t_gates_mech_control);
        line(m, t_door_metal_locked, 14, yard_wdth, 19, yard_wdth );
        m->ter_set(13, yard_wdth + 1, t_gates_mech_control);
        m->ter_set(13, yard_wdth - 1, t_gates_mech_control);

        //place items
        m->place_items("mechanics", 90, 1, yard_wdth + 1, 1, yard_wdth + 7, true, 0);
        m->place_items("mechanics", 90, 4, SEEY * 2 - 5, 15, SEEY * 2 - 5, true, 0);
        m->place_items("clothing_work_set", 40, SEEX * 2 - 6, SEEY * 2 - 5, SEEX * 2 - 4, SEEY * 2 - 5, true, 0);

        // rotate garage

        int vy = 0, vx = 0, theta = 0, tdx = 0, tdy = 0, td = 9;

        if (terrain_type == "s_garage_north") {
            vx = 8, vy = yard_wdth + 6;
            theta = 90;
            tdx = td;
        } else if (terrain_type == "s_garage_east") {
            m->rotate(1);
            vx = yard_wdth + 8, vy = 7;
            theta = 0;
            tdy = td;
        } else if (terrain_type == "s_garage_south") {
            m->rotate(2);
            vx = SEEX * 2 - 9, vy = SEEY * 2 - (yard_wdth + 6);
            theta = 270;
            tdx = -td;
        } else if (terrain_type == "s_garage_west") {
            m->rotate(3);
            vx = SEEX * 2 - yard_wdth - 9, vy = SEEY * 2 - 8;
            theta = 180;
            tdy = -td;
        }

        // place vehicles, if any
        for (int v=0; v<=1; v++) {
            if (one_in(4)) {
                m->add_vehicle(vgroup_id("garage"), {vx + v * tdx, vy + v * tdy}, theta + one_in(3)*rng(-1,1)*30, -1, -1);
            }
        }
}

void mapgen_farm(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_farm_field(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_police(map *m, oter_id terrain_type, mapgendata dat, int, float density)
{

(void)dat;
//    } else if (is_ot_type("police", terrain_type)) {

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if ((j ==  7 && i != 17 && i != 18) ||
                    (j == 12 && i !=  0 && i != 17 && i != 18 && i != SEEX * 2 - 1) ||
                    (j == 14 && ((i > 0 && i < 6) || i == 9 || i == 13 || i == 17)) ||
                    (j == 15 && i > 17  && i < SEEX * 2 - 1) ||
                    (j == 17 && i >  0  && i < 17) ||
                    (j == 20)) {
                    m->ter_set(i, j, t_wall);
                } else if (((i == 0 || i == SEEX * 2 - 1) && j > 7 && j < 20) ||
                           ((i == 5 || i == 10 || i == 16 || i == 19) && j > 7 && j < 12) ||
                           ((i == 5 || i ==  9 || i == 13) && j > 14 && j < 17) ||
                           (i == 17 && j > 14 && j < 20)) {
                    m->ter_set(i, j, t_wall);
                } else if (j == 14 && i > 5 && i < 17 && i % 2 == 0) {
                    m->ter_set(i, j, t_bars);
                } else if ((i > 1 && i < 4 && j > 8 && j < 11) ||
                           (j == 17 && i > 17 && i < 21)) {
                    m->set(i, j, t_floor, f_counter);
                } else if ((i == 20 && j > 7 && j < 12) || (j == 8 && i > 19 && i < 23) ||
                           (j == 15 && i > 0 && i < 5)) {
                    m->set(i, j, t_floor, f_locker);
                } else if (j < 7) {
                    m->ter_set(i, j, t_pavement);
                } else if (j > 20) {
                    m->ter_set(i, j, t_sidewalk);
                } else {
                    m->ter_set(i, j, t_floor);
                }
            }
        }
        m->ter_set(17, 7, t_door_locked);
        m->ter_set(18, 7, t_door_locked);
        m->ter_set(rng( 1,  4), 12, t_door_c);
        m->ter_set(rng( 6,  9), 12, t_door_c);
        m->ter_set(rng(11, 15), 12, t_door_c);
        m->ter_set(21, 12, t_door_metal_locked);
        computer * tmpcomp = m->add_computer( tripoint( 22, 13, m->get_abs_sub().z ), _("PolCom OS v1.47"), 3);
        tmpcomp->add_option(_("Open Supply Room"), COMPACT_OPEN, 3);
        tmpcomp->add_failure(COMPFAIL_SHUTDOWN);
        tmpcomp->add_failure(COMPFAIL_ALARM);
        tmpcomp->add_failure(COMPFAIL_MANHACKS);
        m->ter_set( 7, 14, t_door_c);
        m->ter_set(11, 14, t_door_c);
        m->ter_set(15, 14, t_door_c);
        m->ter_set(rng(20, 22), 15, t_door_c);
        m->ter_set(2, 17, t_door_metal_locked);
        tmpcomp = m->add_computer( tripoint( 22, 13, m->get_abs_sub().z ), _("PolCom OS v1.47"), 3);
        tmpcomp->add_option(_("Open Evidence Locker"), COMPACT_OPEN, 3);
        tmpcomp->add_failure(COMPFAIL_SHUTDOWN);
        tmpcomp->add_failure(COMPFAIL_ALARM);
        tmpcomp->add_failure(COMPFAIL_MANHACKS);
        m->ter_set(17, 18, t_door_c);
        for (int i = 18; i < SEEX * 2 - 1; i++) {
            m->ter_set(i, 20, t_window);
        }
        if (one_in(3)) {
            for (int j = 16; j < 20; j++) {
                m->ter_set(SEEX * 2 - 1, j, t_window);
            }
        }
        int rn = rng(18, 21);
        if (one_in(4)) {
            m->ter_set(rn    , 20, t_door_c);
            m->ter_set(rn + 1, 20, t_door_c);
        } else {
            m->ter_set(rn    , 20, t_door_locked);
            m->ter_set(rn + 1, 20, t_door_locked);
        }
        rn = rng(1, 5);
        m->ter_set(rn, 20, t_window);
        m->ter_set(rn + 1, 20, t_window);
        rn = rng(10, 14);
        m->ter_set(rn, 20, t_window);
        m->ter_set(rn + 1, 20, t_window);
        if (one_in(2)) {
            for (int i = 6; i < 10; i++) {
                m->furn_set(i, 8, f_counter);
            }
        }
        if (one_in(3)) {
            for (int j = 8; j < 12; j++) {
                m->furn_set(6, j, f_counter);
            }
        }
        if (one_in(3)) {
            for (int j = 8; j < 12; j++) {
                m->furn_set(9, j, f_counter);
            }
        }

        m->place_items("kitchen",      40,  6,  8,  9, 11,    false, 0);
        m->place_items("cop_armory",  70, 20,  8, 22,  8,    false, 0);
        m->place_items("cop_gear",  70, 20,  8, 20, 11,    false, 0);
        m->place_items("cop_evidence", 60,  1, 15,  4, 15,    false, 0);

        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (m->ter(i, j) == t_floor && one_in(80)) {
                    m->spawn_item(i, j, "badge_deputy");
                }
            }
        }
        autorotate_down();

        m->place_spawns( mongroup_id( "GROUP_POLICE" ), 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);


}


void mapgen_bank(map *m, oter_id terrain_type, mapgendata dat, int, float)
{
    dat.fill_groundcover();
    // Basic floorplan
    square(m, t_floor, 1,  1, 22, 22);
    line(m, t_wall,  1,  1, 22,  1);
    line(m, t_wall,  2,  5,  5,  5);
    line(m, t_wall, 16,  5, 19,  5);
    line(m, t_wall,  2,  9, 19,  9);
    line(m, t_wall, 12, 12, 21, 12);
    line(m, t_wall,  2, 14,  6, 14);
    line(m, t_wall, 13, 16, 21, 16);
    line(m, t_wall,  1, 22, 22, 22);
    line(m, t_wall,  1,  2,  1, 21);
    line(m, t_wall, 22,  2, 22, 21);
    line(m, t_wall,  7, 10,  7, 21);
    line(m, t_wall, 12, 12, 12, 21);
    line(m, t_wall, 19, 13, 19, 15);
    line(m, t_wall, 16,  6, 16,  8);
    line(m, t_wall, 19,  6, 19,  8);
    line(m, t_wall_metal,  2, 15,  6, 15);
    line(m, t_wall_metal,  2, 21,  6, 21);
    line(m, t_wall_metal,  2, 16,  2, 20);
    line(m, t_wall_metal,  6, 16,  6, 20);
    //Fixed doors
    line(m, t_door_glass_c, 9, 1, 10, 1);
    m->ter_set( 19,  6, t_door_c);
    m->ter_set( 12, 17, t_door_c);
    m->ter_set( 17, 12, t_door_c);
    square(m, t_door_metal_locked, 6, 19, 7, 20);
    if (one_in(3)) {
        line(m, t_door_locked_interior, 20, 9, 21, 9);
    } else {
        line(m, t_door_c, 20, 9, 21, 9);
    }
    if (one_in(4)) {
        m->ter_set( 7, 10, t_door_locked_interior);
    } else {
        m->ter_set( 7, 10, t_door_c);
    }
    //Lobby
    line(m, t_atm,  2,  2,  2,  3);
    if (one_in(2)) {
        line_furn(m, f_chair, 13, 2, 15, 2);
        m->furn_set(14, 2, f_table);
    }
    if (one_in(2)) {
        line_furn(m, f_chair, 17, 2, 19, 2);
        m->furn_set(18, 2, f_table);
    }
    if (one_in(2)) {
        m->furn_set(21, 2, f_indoor_plant);
    }
    //Windows or glass wall front?
    int tmp = 0;
    if (!one_in(3)) {
        line(m, t_wall_glass_alarm, 1, 1, 8, 1);
        line(m, t_wall_glass_alarm, 11, 1, 22, 1);
    } else {
        m->ter_set( rng(4,7),  1, t_window_alarm);
        m->ter_set( rng(12,16),  1, t_window_alarm);
        m->ter_set( rng(17,20),  1, t_window_alarm);
        tmp = 1;
    }
    //Windows or glass wall side?
    if (tmp != 1 && one_in(3)) {
        line(m, t_wall_glass_alarm, 22, 1, 22, 8);
        if (one_in(2)) {
            line(m, t_wall_glass_alarm, 22, 1, 22, 11);
        }
    } else {
        m->ter_set( 22, rng(3,5), t_window_alarm);
        m->ter_set( 22, rng(6,8), t_window_alarm);
    }
    //Teller counters, 1 in 2 chance to have windows
    if (one_in(2)) {
        m->furn_set(  7,  5, f_counter);
        m->ter_set(   8,  5, t_window);
        m->furn_set(  9,  5, f_counter);
        m->ter_set(  10,  5, t_window);
        m->furn_set( 11,  5, f_counter);
        m->ter_set(  12,  5, t_window);
        m->furn_set( 13,  5, f_counter);
        m->ter_set(  14,  5, t_window);
        m->furn_set( 15,  5, f_counter);
    } else {
        line_furn(m, f_counter,  7,  5, 15, 5);
    }
    //Back wall teller space counter
    tmp = rng(9, 11);
    line_furn(m, f_counter,  rng(2,4),  8,  tmp,  8);
    m->furn_set( tmp + 1, 8, f_indoor_plant);
    //Teller doors
    m->ter_set(rng(2,4), 5, t_door_locked_interior);
    m->ter_set(rng(13,15), 9, t_door_c);
    //Bathroom
    m->furn_set( 17, 6, f_sink);
    m->place_toilet( 17, 8);
    //Storage
    if (one_in(2)){
        if (!one_in(4)) {
            m->ter_set( 20, 12, t_door_c);
        } else {
            m->ter_set( 20, 12, t_door_locked_interior);
        }
        line_furn(m, f_counter, 21, 13, 21, 15);
        m->furn_set( 20, 15, f_counter);
    } else {
        if (!one_in(4)) {
            m->ter_set( 21, 12, t_door_c);
        } else {
            m->ter_set( 21, 12, t_door_locked_interior);
        }
        line_furn(m, f_counter, 20, 13, 20, 15);
        m->furn_set( 21, 15, f_counter);
    }
    //Interview office
    if (one_in(2)) {
        m->ter_set( 13, 12, t_door_c);
    } else {
        m->ter_set( 14, 12, t_door_c);
    }
    if (one_in(2)) {
        m->furn_set( 18, 13, f_indoor_plant);
    }
    line_furn(m, f_desk, 16, 14, 16, 15);
    line_furn(m, f_chair, 15, 14, 15, 15);
    m->furn_set( 17, 15, f_chair);
    //Executive office
    if (!one_in(3)) {
        m->furn_set( 2, 13, f_indoor_plant);
    }
    line_furn(m, f_desk, 3, 12, 5, 12);
    m->furn_set(4, 13, f_chair);
    if (one_in(2)) {
        m->ter_set( 1, 11, t_window_alarm);
    } else {
        m->ter_set( 1, 10, t_window_alarm);
        m->ter_set( 1, 12, t_window_alarm);
    }
    //Conference room
    line_furn(m, f_chair, 15, 18, 15, 20);
    line_furn(m, f_chair, 17, 18, 17, 20);
    line_furn(m, f_chair, 19, 18, 19, 20);
    line_furn(m, f_table, 15, 19, 19, 19);
    m->furn_set( 20, 19, f_chair);
    //Conference windows or glass walls?
    if (one_in(4)) {
        line(m, t_wall_glass_alarm, 13, 22, 22, 22);
        line(m, t_wall_glass_alarm, 22, 17, 22, 21);
    } else {
        m->ter_set( rng(13,17), 22, t_window_alarm);
        m->ter_set( rng(17,21), 22, t_window_alarm);
        m->ter_set( 22, rng(18,20), t_window_alarm);
    }
    //Vault
    line_furn(m, f_safe_l, 3, 16, 5, 16);
    line_furn(m, f_table, 3, 19, 3, 20);
    if (one_in(3)){
        line(m, t_bars, 8, 18, 11, 18);
        line(m, t_door_metal_locked, 9, 18, 10, 18);
    }
    computer * tmpcomp = m->add_computer( tripoint( 8, 21, m->get_abs_sub().z ), _("Consolidated Computerized Bank of the Treasury"), 3);
    tmpcomp->add_option(_("Open Vault"), COMPACT_OPEN, 3);
    tmpcomp->add_failure(COMPFAIL_SHUTDOWN);
    tmpcomp->add_failure(COMPFAIL_ALARM);

    m->place_items("office",    30,  4,  8,  9,  8, false, 0);
    m->place_items("office",    30,  3, 12,  5, 12, false, 0);
    m->place_items("office",    70, 16, 14, 16, 15, false, 0);
    m->place_items("vault",     50,  3, 19,  3, 20, false, 0);
    m->place_items("vault",     90,  3, 16,  5, 16, false, 0);

    autorotate(false);
}

void mapgen_pawn(map *m, oter_id terrain_type, mapgendata dat, int, float)
{

//    } else if (is_ot_type("pawn", terrain_type)) {

        // Init to plain grass/dirt
        dat.fill_groundcover();

        int tw = rng(0, 10);
        int bw = SEEY * 2 - rng(1, 2) - rng(0, 1) * rng(0, 1);
        int lw = rng(0, 4);
        int rw = SEEX * 2 - rng(1, 5);
        if (tw >= 6) { // Big enough for its own parking lot
            square(m, t_pavement, 0, 0, SEEX * 2 - 1, tw - 1);
            for (int i = rng(0, 1); i < SEEX * 2; i += 4) {
                line(m, t_pavement_y, i, 1, i, tw - 1);
            }
        }
        // Floor and walls
        square(m, t_floor, lw, tw, rw, bw);
        line(m, t_wall, lw, tw, rw, tw);
        line(m, t_wall, lw, bw, rw, bw);
        line(m, t_wall, lw, tw + 1, lw, bw - 1);
        line(m, t_wall, rw, tw + 1, rw, bw - 1);
        // Doors and windows--almost certainly alarmed
        if (one_in(15)) {
            line(m, t_window, lw + 2, tw, lw + 5, tw);
            line(m, t_window, rw - 5, tw, rw - 2, tw);
            line(m, t_door_locked, SEEX, tw, SEEX + 1, tw);
        } else {
            line(m, t_window_alarm, lw + 2, tw, lw + 5, tw);
            line(m, t_window_alarm, rw - 5, tw, rw - 2, tw);
            line(m, t_door_locked_alarm, SEEX, tw, SEEX + 1, tw);
        }
        // Some display racks by the left and right walls
        line_furn(m, f_rack, lw + 1, tw + 1, lw + 1, bw - 1);
        m->place_items("pawn", 86, lw + 1, tw + 1, lw + 1, bw - 1, false, 0);
        line_furn(m, f_rack, rw - 1, tw + 1, rw - 1, bw - 1);
        m->place_items("pawn", 86, rw - 1, tw + 1, rw - 1, bw - 1, false, 0);
        // Some display counters
        line_furn(m, f_counter, lw + 4, tw + 2, lw + 4, bw - 3);
        m->place_items("pawn", 80, lw + 4, tw + 2, lw + 4, bw - 3, false, 0);
        line_furn(m, f_counter, rw - 4, tw + 2, rw - 4, bw - 3);
        m->place_items("pawn", 80, rw - 4, tw + 2, rw - 4, bw - 3, false, 0);
        // More display counters, if there's room for them
        if (rw - lw >= 18 && one_in(rw - lw - 17)) {
            for (int j = tw + rng(3, 5); j <= bw - 3; j += 3) {
                line_furn(m, f_counter, lw + 6, j, rw - 6, j);
                m->place_items("pawn", 75, lw + 6, j, rw - 6, j, false, 0);
            }
        }
        // Finally, place an office sometimes
        if (!one_in(5)) {
            if (one_in(2)) { // Office on the left side
                int office_top = bw - rng(3, 5), office_right = lw + rng(4, 7);
                // Clear out any items in that area!  And reset to floor.
                for (int i = lw + 1; i <= office_right; i++) {
                    for (int j = office_top; j <= bw - 1; j++) {
                        m->i_clear(i, j);
                        m->ter_set(i, j, t_floor);
                        m->furn_set( i, j, f_null );
                    }
                }
                line(m, t_wall, lw + 1, office_top, office_right, office_top);
                line(m, t_wall, office_right, office_top + 1, office_right, bw - 1);
                m->ter_set(office_right, rng(office_top + 1, bw - 1), t_door_locked);
                if (one_in(4)) { // Back door
                    m->ter_set(rng(lw + 1, office_right - 1), bw, t_door_locked_alarm);
                }
                // Finally, add some stuff in there
                m->place_items("office", 70, lw + 1, office_top + 1, office_right - 1, bw - 1,
                            false, 0);
                m->place_items("homeguns", 50, lw + 1, office_top + 1, office_right - 1,
                            bw - 1, false, 0);
                m->place_items("harddrugs", 20, lw + 1, office_top + 1, office_right - 1,
                            bw - 1, false, 0);
            } else { // Office on the right side
                int office_top = bw - rng(3, 5), office_left = rw - rng(4, 7);
                for (int i = office_left; i <= rw - 1; i++) {
                    for (int j = office_top; j <= bw - 1; j++) {
                        m->i_clear(i, j);
                        m->ter_set(i, j, t_floor);
                        m->furn_set( i, j, f_null );
                    }
                }
                line(m, t_wall, office_left, office_top, rw - 1, office_top);
                line(m, t_wall, office_left, office_top + 1, office_left, bw - 1);
                m->ter_set(office_left, rng(office_top + 1, bw - 1), t_door_locked);
                if (one_in(4)) { // Back door
                    m->ter_set(rng(office_left + 1, rw - 1), bw, t_door_locked_alarm);
                }
                m->place_items("office", 70, office_left + 1, office_top + 1, rw - 1, bw - 1,
                            false, 0);
                m->place_items("homeguns", 50, office_left + 1, office_top + 1, rw - 1,
                            bw - 1, false, 0);
                m->place_items("harddrugs", 20, office_left + 1, office_top + 1, rw - 1,
                            bw - 1, false, 0);
            }
        }
        autorotate(false);

}


void mapgen_mil_surplus(map *m, oter_id terrain_type, mapgendata dat, int, float)
{

//    } else if (is_ot_type("mil_surplus", terrain_type)) {

        // Init to plain grass/dirt
        dat.fill_groundcover();
        int lw = rng(0, 2);
        int rw = SEEX * 2 - rng(1, 3);
        int tw = rng(0, 4);
        int bw = SEEY * 2 - rng(3, 8);
        square(m, t_floor, lw, tw, rw, bw);
        line(m, t_wall, lw, tw, rw, tw);
        line(m, t_wall, lw, bw, rw, bw);
        line(m, t_wall, lw, tw + 1, lw, bw - 1);
        line(m, t_wall, rw, tw + 1, rw, bw - 1);
        int rn = rng(4, 7);
        line(m, t_window, lw + 2, tw, lw + rn, tw);
        line(m, t_window, rw - rn, tw, rw - 2, tw);
        line(m, t_door_c, SEEX, tw, SEEX + 1, tw);
        if (one_in(2)) { // counter on left
            line_furn(m, f_counter, lw + 2, tw + 1, lw + 2, tw + rng(3, 4));
        } else { // counter on right
            line_furn(m, f_counter, rw - 2, tw + 1, rw - 2, tw + rng(3, 4));
        }
        for (int i = lw + 1; i <= SEEX; i += 2) {
            line_furn(m, f_rack, i, tw + 5, i, bw - 2);
            items_location loc;
            if (one_in(3)) {
                loc = "mil_armor";
            } else if (one_in(3)) {
                loc = "mil_surplus";
            } else {
                loc = "mil_food_nodrugs";
            }
            m->place_items(loc, 70, i, tw + 5, i, bw - 2, false, 0);
        }
        for (int i = rw - 1; i >= SEEX + 1; i -= 2) {
            line_furn(m, f_rack, i, tw + 5, i, bw - 2);
            items_location loc;
            if (one_in(3)) {
                loc = "mil_armor";
            } else if (one_in(3)) {
                loc = "mil_surplus";
            } else {
                loc = "mil_food_nodrugs";
            }
            m->place_items(loc, 70, i, tw + 5, i, bw - 2, false, 0);
        }
        autorotate(false);
}


void mapgen_megastore_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_megastore(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_hospital_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_hospital(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_public_works_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_public_works(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_school_1(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_school_2(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_school_3(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_school_4(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_school_5(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_school_6(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_school_7(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_school_8(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_school_9(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_prison_1(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_prison_2(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_prison_3(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_prison_4(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_prison_5(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_prison_6(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_prison_7(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_prison_8(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_prison_9(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_prison_b(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_prison_b_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_hotel_tower_1_1(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_hotel_tower_1_2(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_hotel_tower_1_3(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_hotel_tower_1_4(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_hotel_tower_1_5(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_hotel_tower_1_6(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_hotel_tower_1_7(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_hotel_tower_1_8(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_hotel_tower_1_9(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_hotel_tower_b_1(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_hotel_tower_b_2(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_hotel_tower_b_3(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_mansion_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_mansion(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_fema_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_fema(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_station_radio(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_lab(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_lab_stairs(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_lab_core(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_lab_finale(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_ice_lab(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_ice_lab_stairs(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_ice_lab_core(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_ice_lab_finale(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_nuke_plant_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_nuke_plant(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_bunker(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_outpost(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_silo(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_silo_finale(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_temple(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_temple_stairs(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_temple_core(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_temple_finale(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_sewage_treatment(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_sewage_treatment_hub(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_sewage_treatment_under(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_mine_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_mine_shaft(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_mine(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_mine_down(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_mine_finale(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_spiral_hub(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_spiral(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_radio_tower(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_toxic_dump(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_haz_sar_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_haz_sar(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_haz_sar_entrance_b1(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_haz_sar_b1(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_cave(map *m, oter_id, mapgendata dat, int turn, float density)
{
        if (dat.above() == "cave") {
            // We're underground! // FIXME; y u no use zlevel
            for (int i = 0; i < SEEX * 2; i++) {
                for (int j = 0; j < SEEY * 2; j++) {
                    bool floorHere = (rng(0, 6) < i || SEEX * 2 - rng(1, 7) > i ||
                                      rng(0, 6) < j || SEEY * 2 - rng(1, 7) > j );
                    if (floorHere) {
                        m->ter_set(i, j, t_rock_floor);
                    } else {
                        m->ter_set(i, j, t_rock);
                    }
                }
            }
            square(m, t_slope_up, SEEX - 1, SEEY - 1, SEEX, SEEY);
            switch(rng(1, 10)) {
            case 1:
                // natural refuse, chance of minerals
                m->place_items("cave_minerals", 50, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
                m->place_items("monparts", 80, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
                break;
            case 2:
                // trash, minerals less likely
                m->place_items("cave_minerals", 25, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
                m->place_items("trash", 70, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
                break;
            case 3:
                // bat corpses
                m->place_items("cave_minerals", 50, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
                for (int i = rng(1, 12); i > 0; i--) {
                    m->add_item_or_charges(rng(1, SEEX * 2 - 1), rng(1, SEEY * 2 - 1), item::make_corpse( mon_bat ) );
                }
                break;
            case 4:
                // ant food, chance of 80
                m->place_items("cave_minerals", 25, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
                m->place_items("ant_food", 85, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
                break;
            case 5: {
                // hermitage
                int origx = rng(SEEX - 1, SEEX),
                    origy = rng(SEEY - 1, SEEY),
                    hermx = rng(SEEX - 6, SEEX + 5),
                    hermy = rng(SEEX - 6, SEEY + 5);
                std::vector<point> bloodline = line_to(origx, origy, hermx, hermy, 0);
                for (auto &ii : bloodline) {
                    madd_field( m, ii.x, ii.y, fd_blood, 2);
                }
                m->add_item_or_charges(hermx, hermy, item::make_corpse() );
                // This seems verbose.  Maybe a function to spawn from a list of item groups?
                m->place_items("stash_food", 50, hermx - 1, hermy - 1, hermx + 1, hermy + 1, true, 0);
                m->place_items("gear_survival", 50, hermx - 1, hermy - 1, hermx + 1, hermy + 1, true, 0);
                m->place_items("survival_armor", 50, hermx - 1, hermy - 1, hermx + 1, hermy + 1, true, 0);
                m->place_items("weapons", 40, hermx - 1, hermy - 1, hermx + 1, hermy + 1, true, 0);
                m->place_items("magazines", 40, hermx - 1, hermy - 1, hermx + 1, hermy + 1, true, 0);
                m->place_items("rare", 30, hermx - 1, hermy - 1, hermx + 1, hermy + 1, true, 0);
                break;
            }
            default:
                // nothing except maybe minerals, default occurs half the time
                m->place_items("cave_minerals", 50, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
                break;
            }
            m->place_spawns( mongroup_id( "GROUP_CAVE" ), 2, 6, 6, 18, 18, 1.0);
        } else { // We're above ground!
            // First, draw a forest
/*
            draw_map(oter_id("forest"), dat.north(), dat.east(), dat.south(), dat.west(), dat.neast(), dat.seast(), dat.nwest(), dat.swest(),
                     dat.above(), turn, g, density, dat.zlevel);
*/
            mapgen_forest_general(m, oter_str_id("forest").id(), dat, turn, density);
            // Clear the center with some rocks
            square(m, t_rock, SEEX - 6, SEEY - 6, SEEX + 5, SEEY + 5);
            int pathx, pathy;
            if (one_in(2)) {
                pathx = rng(SEEX - 6, SEEX + 5);
                pathy = (one_in(2) ? SEEY - 8 : SEEY + 7);
            } else {
                pathx = (one_in(2) ? SEEX - 8 : SEEX + 7);
                pathy = rng(SEEY - 6, SEEY + 5);
            }
            std::vector<point> pathline = line_to(pathx, pathy, SEEX - 1, SEEY - 1, 0);
            for (auto &ii : pathline) {
                square(m, t_dirt, ii.x, ii.y,
                       ii.x + 1, ii.y + 1);
            }
            while (!one_in(8)) {
                m->ter_set(rng(SEEX - 6, SEEX + 5), rng(SEEY - 6, SEEY + 5), t_dirt);
            }
            square(m, t_slope_down, SEEX - 1, SEEY - 1, SEEX, SEEY);
        }





}


void mapgen_cave_rat(map *m, oter_id, mapgendata dat, int, float)
{

        fill_background(m, t_rock);

        if (dat.above() == "cave_rat") { // Finale
            rough_circle(m, t_rock_floor, SEEX, SEEY, 8);
            square(m, t_rock_floor, SEEX - 1, SEEY, SEEX, SEEY * 2 - 2);
            line(m, t_slope_up, SEEX - 1, SEEY * 2 - 3, SEEX, SEEY * 2 - 2);
            for (int i = SEEX - 4; i <= SEEX + 4; i++) {
                for (int j = SEEY - 4; j <= SEEY + 4; j++) {
                    if ((i <= SEEX - 2 || i >= SEEX + 2) && (j <= SEEY - 2 || j >= SEEY + 2)) {
                        m->add_spawn(mon_sewer_rat, 1, i, j);
                    }
                }
            }
            m->add_spawn(mon_rat_king, 1, SEEX, SEEY);
            m->place_items("rare", 75, SEEX - 4, SEEY - 4, SEEX + 4, SEEY + 4, true, 0);
        } else { // Level 1
            int cavex = SEEX, cavey = SEEY * 2 - 3;
            int stairsx = SEEX - 1, stairsy = 1; // Default stairs location--may change
            int centerx = 0;
            do {
                cavex += rng(-1, 1);
                cavey -= rng(0, 1);
                for (int cx = cavex - 1; cx <= cavex + 1; cx++) {
                    for (int cy = cavey - 1; cy <= cavey + 1; cy++) {
                        m->ter_set(cx, cy, t_rock_floor);
                        if (one_in(10)) {
                            madd_field( m, cx, cy, fd_blood, rng(1, 3));
                        }
                        if (one_in(20)) {
                            m->add_spawn(mon_sewer_rat, 1, cx, cy);
                        }
                    }
                }
                if (cavey == SEEY - 1) {
                    centerx = cavex;
                }
            } while (cavey > 2);
            // Now draw some extra passages!
            do {
                int tox = (one_in(2) ? 2 : SEEX * 2 - 3), toy = rng(2, SEEY * 2 - 3);
                std::vector<point> path = line_to(centerx, SEEY - 1, tox, toy, 0);
                for (auto &i : path) {
                    for (int cx = i.x - 1; cx <= i.x + 1; cx++) {
                        for (int cy = i.y - 1; cy <= i.y + 1; cy++) {
                            m->ter_set(cx, cy, t_rock_floor);
                            if (one_in(10)) {
                                madd_field( m, cx, cy, fd_blood, rng(1, 3));
                            }
                            if (one_in(20)) {
                                m->add_spawn(mon_sewer_rat, 1, cx, cy);
                            }
                        }
                    }
                }
                if (one_in(2)) {
                    stairsx = tox;
                    stairsy = toy;
                }
            } while (one_in(2));
            // Finally, draw the stairs up and down.
            m->ter_set(SEEX - 1, SEEX * 2 - 2, t_slope_up);
            m->ter_set(SEEX    , SEEX * 2 - 2, t_slope_up);
            m->ter_set(stairsx, stairsy, t_slope_down);
        }
}


void mapgen_spider_pit_under(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_anthill(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_slimepit(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_slimepit_down(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_triffid_grove(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_triffid_roots(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_triffid_finale(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void)m; (void)terrain_type; (void)dat; (void)turn; (void)density; // STUB
/*

*/
}


void mapgen_cavern(map *m, oter_id, mapgendata dat, int, float)
{

    for (int i = 0; i < 4; i++) { // don't look at me like that, this was messed up before I touched it :P - AD ( FIXME )
       dat.set_dir(i,
             (dat.t_nesw[i] == "cavern" || dat.t_nesw[i] == "subway_ns" ||
                       dat.t_nesw[i] == "subway_ew" ? 0 : 3)
        );
    }
    dat.e_fac = SEEX * 2 - 1 - dat.e_fac;
    dat.s_fac = SEEY * 2 - 1 - dat.s_fac;

    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            if ((j < dat.n_fac || j > dat.s_fac || i < dat.w_fac || i > dat.e_fac) &&
                (!one_in(3) || j == 0 || j == SEEY * 2 - 1 || i == 0 || i == SEEX * 2 - 1)) {
                m->ter_set(i, j, t_rock);
            } else {
                m->ter_set(i, j, t_rock_floor);
            }
        }
    }

    int rn = rng(0, 2) * rng(0, 3) + rng(0, 1); // Number of pillars
    for (int n = 0; n < rn; n++) {
        int px = rng(5, SEEX * 2 - 6);
        int py = rng(5, SEEY * 2 - 6);
        for (int i = px - 1; i <= px + 1; i++) {
            for (int j = py - 1; j <= py + 1; j++) {
                m->ter_set(i, j, t_rock);
            }
        }
    }

    if (connects_to(dat.north(), 2)) {
        for (int i = SEEX - 2; i <= SEEX + 3; i++) {
            for (int j = 0; j <= SEEY; j++) {
                m->ter_set(i, j, t_rock_floor);
            }
        }
    }
    if (connects_to(dat.east(), 3)) {
        for (int i = SEEX; i <= SEEX * 2 - 1; i++) {
            for (int j = SEEY - 2; j <= SEEY + 3; j++) {
                m->ter_set(i, j, t_rock_floor);
            }
        }
    }
    if (connects_to(dat.south(), 0)) {
        for (int i = SEEX - 2; i <= SEEX + 3; i++) {
            for (int j = SEEY; j <= SEEY * 2 - 1; j++) {
                m->ter_set(i, j, t_rock_floor);
            }
        }
    }
    if (connects_to(dat.west(), 1)) {
        for (int i = 0; i <= SEEX; i++) {
            for (int j = SEEY - 2; j <= SEEY + 3; j++) {
                m->ter_set(i, j, t_rock_floor);
            }
        }
    }
    m->place_items("cavern", 60, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, false, 0);
    if (one_in(6)) { // Miner remains
        int x, y;
        do {
            x = rng(0, SEEX * 2 - 1);
            y = rng(0, SEEY * 2 - 1);
        } while (m->impassable(x, y));
        if (!one_in(3)) {
            m->spawn_item(x, y, "jackhammer");
        }
        if (one_in(3)) {
            m->spawn_item(x, y, "mask_dust");
        }
        if (one_in(2)) {
            m->spawn_item(x, y, "hat_hard");
        }
        while (!one_in(3)) {
            for( int i = 0; i < 3; ++i ) {
                m->put_items_from_loc( "cannedfood", tripoint( x, y, m->get_abs_sub().z ), 0 );
            }
        }
    }



}

void mapgen_rock_partial(map *m, oter_id, mapgendata dat, int, float)
{
    fill_background( m, t_rock );
    for( int i = 0; i < 4; i++ ) {
        if( dat.t_nesw[i] == "cavern" || dat.t_nesw[i] == "slimepit" ||
            dat.t_nesw[i] == "slimepit_down" ) {
            dat.dir(i) = 6;
        } else {
            dat.dir(i) = 0;
        }
    }

    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( rng(0, dat.n_fac) > j || rng(0, dat.s_fac) > SEEY * 2 - 1 - j ||
                rng(0, dat.w_fac) > i || rng(0, dat.e_fac) > SEEX * 2 - 1 - i ) {
                m->ter_set(i, j, t_rock_floor);
            }
        }
    }
}

void mapgen_rock(map *m, oter_id, mapgendata, int, float)
{
    fill_background( m, t_rock );
}


void mapgen_open_air(map *m, oter_id, mapgendata, int, float){
    fill_background( m, t_open_air );
}


void mapgen_rift(map *m, oter_id, mapgendata dat, int, float)
{

    if (dat.north() != "rift" && dat.north() != "hellmouth") {
        if (connects_to(dat.north(), 2)) {
            dat.n_fac = rng(-6, -2);
        } else {
            dat.n_fac = rng(2, 6);
        }
    }
    if (dat.east() != "rift" && dat.east() != "hellmouth") {
        if (connects_to(dat.east(), 3)) {
            dat.e_fac = rng(-6, -2);
        } else {
            dat.e_fac = rng(2, 6);
        }
    }
    if (dat.south() != "rift" && dat.south() != "hellmouth") {
        if (connects_to(dat.south(), 0)) {
            dat.s_fac = rng(-6, -2);
        } else {
            dat.s_fac = rng(2, 6);
        }
    }
    if (dat.west() != "rift" && dat.west() != "hellmouth") {
        if (connects_to(dat.west(), 1)) {
            dat.w_fac = rng(-6, -2);
        } else {
            dat.w_fac = rng(2, 6);
        }
    }
    // Negative *_fac values indicate rock floor connection, otherwise solid rock
    // Of course, if we connect to a rift, *_fac = 0, and thus lava extends all the
    //  way.
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            if ((dat.n_fac < 0 && j < dat.n_fac * -1) || (dat.s_fac < 0 && j >= SEEY * 2 - dat.s_fac) ||
                (dat.w_fac < 0 && i < dat.w_fac * -1) || (dat.e_fac < 0 && i >= SEEX * 2 - dat.e_fac)  ) {
                m->ter_set(i, j, t_rock_floor);
            } else if (j < dat.n_fac || j >= SEEY * 2 - dat.s_fac ||
                       i < dat.w_fac || i >= SEEX * 2 - dat.e_fac   ) {
                m->ter_set(i, j, t_rock);
            } else {
                m->ter_set(i, j, t_lava);
            }
        }
    }



}


void mapgen_hellmouth(map *m, oter_id, mapgendata dat, int, float)
{
    // what is this, doom?
    // .. seriously, though...
    for (int i = 0; i < 4; i++) {
        if (dat.t_nesw[i] != "rift" && dat.t_nesw[i] != "hellmouth") {
            dat.dir(i) = 6;
        }
    }

    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            if (j < dat.n_fac || j >= SEEY * 2 - dat.s_fac || i < dat.w_fac || i >= SEEX * 2 - dat.e_fac ||
                (i >= 6 && i < SEEX * 2 - 6 && j >= 6 && j < SEEY * 2 - 6)) {
                m->ter_set(i, j, t_rock_floor);
            } else {
                m->ter_set(i, j, t_lava);
            }
            if (i >= SEEX - 1 && i <= SEEX && j >= SEEY - 1 && j <= SEEY) {
                m->ter_set(i, j, t_slope_down);
            }
        }
    }
    switch (rng(0, 4)) { // Randomly chosen "altar" design
        case 0:
            for (int i = 7; i <= 16; i += 3) {
                m->ter_set(i, 6, t_rock);
                m->ter_set(i, 17, t_rock);
                m->ter_set(6, i, t_rock);
                m->ter_set(17, i, t_rock);
                if (i > 7 && i < 16) {
                    m->ter_set(i, 10, t_rock);
                    m->ter_set(i, 13, t_rock);
                } else {
                    m->ter_set(i - 1, 6 , t_rock);
                    m->ter_set(i - 1, 10, t_rock);
                    m->ter_set(i - 1, 13, t_rock);
                    m->ter_set(i - 1, 17, t_rock);
                }
            }
            break;
        case 1:
            for (int i = 6; i < 11; i++) {
                m->ter_set(i, i, t_lava);
                m->ter_set(SEEX * 2 - 1 - i, i, t_lava);
                m->ter_set(i, SEEY * 2 - 1 - i, t_lava);
                m->ter_set(SEEX * 2 - 1 - i, SEEY * 2 - 1 - i, t_lava);
                if (i < 10) {
                    m->ter_set(i + 1, i, t_lava);
                    m->ter_set(SEEX * 2 - i, i, t_lava);
                    m->ter_set(i + 1, SEEY * 2 - 1 - i, t_lava);
                    m->ter_set(SEEX * 2 - i, SEEY * 2 - 1 - i, t_lava);

                    m->ter_set(i, i + 1, t_lava);
                    m->ter_set(SEEX * 2 - 1 - i, i + 1, t_lava);
                    m->ter_set(i, SEEY * 2 - i, t_lava);
                    m->ter_set(SEEX * 2 - 1 - i, SEEY * 2 - i, t_lava);
                }
                if (i < 9) {
                    m->ter_set(i + 2, i, t_rock);
                    m->ter_set(SEEX * 2 - i + 1, i, t_rock);
                    m->ter_set(i + 2, SEEY * 2 - 1 - i, t_rock);
                    m->ter_set(SEEX * 2 - i + 1, SEEY * 2 - 1 - i, t_rock);

                    m->ter_set(i, i + 2, t_rock);
                    m->ter_set(SEEX * 2 - 1 - i, i + 2, t_rock);
                    m->ter_set(i, SEEY * 2 - i + 1, t_rock);
                    m->ter_set(SEEX * 2 - 1 - i, SEEY * 2 - i + 1, t_rock);
                }
            }
            break;
        case 2:
            for (int i = 7; i < 17; i++) {
                m->ter_set(i, 6, t_rock);
                m->ter_set(6, i, t_rock);
                m->ter_set(i, 17, t_rock);
                m->ter_set(17, i, t_rock);
                if (i != 7 && i != 16 && i != 11 && i != 12) {
                    m->ter_set(i, 8, t_rock);
                    m->ter_set(8, i, t_rock);
                    m->ter_set(i, 15, t_rock);
                    m->ter_set(15, i, t_rock);
                }
                if (i == 11 || i == 12) {
                    m->ter_set(i, 10, t_rock);
                    m->ter_set(10, i, t_rock);
                    m->ter_set(i, 13, t_rock);
                    m->ter_set(13, i, t_rock);
                }
            }
            break;
        case 3:
            for (int i = 6; i < 11; i++) {
                for (int j = 6; j < 11; j++) {
                    m->ter_set(i, j, t_lava);
                    m->ter_set(SEEX * 2 - 1 - i, j, t_lava);
                    m->ter_set(i, SEEY * 2 - 1 - j, t_lava);
                    m->ter_set(SEEX * 2 - 1 - i, SEEY * 2 - 1 - j, t_lava);
                }
            }
            break;
    }


}


void mapgen_ants_curved(map *m, oter_id terrain_type, mapgendata dat, int, float)
{
    (void)dat;
    int x = SEEX;
    int y = 1;
    int rn = 0;
    // First, set it all to rock
    fill_background(m, t_rock);

    for (int i = SEEX - 2; i <= SEEX + 3; i++) {
        m->ter_set(i, 0, t_rock_floor);
        m->ter_set(i, 1, t_rock_floor);
        m->ter_set(i, 2, t_rock_floor);
        m->ter_set(SEEX * 2 - 1, i, t_rock_floor);
        m->ter_set(SEEX * 2 - 2, i, t_rock_floor);
        m->ter_set(SEEX * 2 - 3, i, t_rock_floor);
    }
    do {
        for (int i = x - 2; i <= x + 3; i++) {
            for (int j = y - 2; j <= y + 3; j++) {
                if (i > 0 && i < SEEX * 2 - 1 && j > 0 && j < SEEY * 2 - 1) {
                    m->ter_set(i, j, t_rock_floor);
                }
            }
        }
        if (rn < SEEX) {
            x += rng(-1, 1);
            y++;
        } else {
            x++;
            if (!one_in(x - SEEX)) {
                y += rng(-1, 1);
            } else if (y < SEEY) {
                y++;
            } else if (y > SEEY) {
                y--;
            }
        }
        rn++;
    } while (x < SEEX * 2 - 1 || y != SEEY);
    for (int i = x - 2; i <= x + 3; i++) {
        for (int j = y - 2; j <= y + 3; j++) {
            if (i > 0 && i < SEEX * 2 - 1 && j > 0 && j < SEEY * 2 - 1) {
                m->ter_set(i, j, t_rock_floor);
            }
        }
    }
    if (terrain_type == "ants_es") {
        m->rotate(1);
    }
    if (terrain_type == "ants_sw") {
        m->rotate(2);
    }
    if (terrain_type == "ants_wn") {
        m->rotate(3);
    }


}

void mapgen_ants_four_way(map *m, oter_id, mapgendata dat, int, float)
{
    (void)dat;
    fill_background(m, t_rock);
    int x = SEEX;
    for (int j = 0; j < SEEY * 2; j++) {
        for (int i = x - 2; i <= x + 3; i++) {
            if (i >= 1 && i < SEEX * 2 - 1) {
                m->ter_set(i, j, t_rock_floor);
            }
        }
        x += rng(-1, 1);
        while (abs(SEEX - x) > SEEY * 2 - j - 1) {
            if (x < SEEX) {
                x++;
            }
            if (x > SEEX) {
                x--;
            }
        }
    }

    int y = SEEY;
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = y - 2; j <= y + 3; j++) {
            if (j >= 1 && j < SEEY * 2 - 1) {
                m->ter_set(i, j, t_rock_floor);
            }
        }
        y += rng(-1, 1);
        while (abs(SEEY - y) > SEEX * 2 - i - 1) {
            if (y < SEEY) {
                y++;
            }
            if (y > SEEY) {
                y--;
            }
        }
    }

}

void mapgen_ants_straight(map *m, oter_id terrain_type, mapgendata dat, int, float)
{
    (void)dat;
    int x = SEEX;
    fill_background(m, t_rock);
    for (int j = 0; j < SEEY * 2; j++) {
        for (int i = x - 2; i <= x + 3; i++) {
            if (i >= 1 && i < SEEX * 2 - 1) {
                m->ter_set(i, j, t_rock_floor);
            }
        }
        x += rng(-1, 1);
        while (abs(SEEX - x) > SEEX * 2 - j - 1) {
            if (x < SEEX) {
                x++;
            }
            if (x > SEEX) {
                x--;
            }
        }
    }
    if (terrain_type == "ants_ew") {
        m->rotate(1);
    }

}

void mapgen_ants_tee(map *m, oter_id terrain_type, mapgendata dat, int, float)
{
    (void)dat;
    fill_background(m, t_rock);
    int x = SEEX;
    for (int j = 0; j < SEEY * 2; j++) {
        for (int i = x - 2; i <= x + 3; i++) {
            if (i >= 1 && i < SEEX * 2 - 1) {
                m->ter_set(i, j, t_rock_floor);
            }
        }
        x += rng(-1, 1);
        while (abs(SEEX - x) > SEEY * 2 - j - 1) {
            if (x < SEEX) {
                x++;
            }
            if (x > SEEX) {
                x--;
            }
        }
    }
    int y = SEEY;
    for (int i = SEEX; i < SEEX * 2; i++) {
        for (int j = y - 2; j <= y + 3; j++) {
            if (j >= 1 && j < SEEY * 2 - 1) {
                m->ter_set(i, j, t_rock_floor);
            }
        }
        y += rng(-1, 1);
        while (abs(SEEY - y) > SEEX * 2 - 1 - i) {
            if (y < SEEY) {
                y++;
            }
            if (y > SEEY) {
                y--;
            }
        }
    }
    if (terrain_type == "ants_new") {
        m->rotate(3);
    }
    if (terrain_type == "ants_nsw") {
        m->rotate(2);
    }
    if (terrain_type == "ants_esw") {
        m->rotate(1);
    }

}


void mapgen_ants_generic(map *m, oter_id terrain_type, mapgendata dat, int, float)
{

    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            if (i < SEEX - 4 || i > SEEX + 5 || j < SEEY - 4 || j > SEEY + 5) {
                m->ter_set(i, j, t_rock);
            } else {
                m->ter_set(i, j, t_rock_floor);
            }
        }
    }
    int rn = rng(10, 20);
    int x = 0;
    int y = 0;
    for (int n = 0; n < rn; n++) {
        int cw = rng(1, 8);
        do {
            x = rng(1 + cw, SEEX * 2 - 2 - cw);
            y = rng(1 + cw, SEEY * 2 - 2 - cw);
        } while (m->ter(x, y) == t_rock);
        for (int i = x - cw; i <= x + cw; i++) {
            for (int j = y - cw; j <= y + cw; j++) {
                if (trig_dist(x, y, i, j) <= cw) {
                    m->ter_set(i, j, t_rock_floor);
                }
            }
        }
    }
    if (connects_to(dat.north(), 2)) {
        for (int i = SEEX - 2; i <= SEEX + 3; i++) {
            for (int j = 0; j <= SEEY; j++) {
                m->ter_set(i, j, t_rock_floor);
            }
        }
    }
    if (connects_to(dat.east(), 3)) {
        for (int i = SEEX; i <= SEEX * 2 - 1; i++) {
            for (int j = SEEY - 2; j <= SEEY + 3; j++) {
                m->ter_set(i, j, t_rock_floor);
            }
        }
    }
    if (connects_to(dat.south(), 0)) {
        for (int i = SEEX - 2; i <= SEEX + 3; i++) {
            for (int j = SEEY; j <= SEEY * 2 - 1; j++) {
                m->ter_set(i, j, t_rock_floor);
            }
        }
    }
    if (connects_to(dat.west(), 1)) {
        for (int i = 0; i <= SEEX; i++) {
            for (int j = SEEY - 2; j <= SEEY + 3; j++) {
                m->ter_set(i, j, t_rock_floor);
            }
        }
    }
    if (terrain_type == "ants_food") {
        m->place_items("ant_food", 92, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
    } else {
        m->place_items("ant_egg",  98, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
    }
    if (terrain_type == "ants_queen") {
        m->add_spawn(mon_ant_queen, 1, SEEX, SEEY);
    } else if (terrain_type == "ants_larvae") {
        m->add_spawn(mon_ant_larva, 10, SEEX, SEEY);
    }


}


void mapgen_ants_food(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    mapgen_ants_generic(m, terrain_type, dat, turn, density);
    m->place_items("ant_food", 92, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
}


void mapgen_ants_larvae(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    mapgen_ants_generic(m, terrain_type, dat, turn, density);
    m->place_items("ant_egg",  98, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
    m->add_spawn(mon_ant_larva, 10, SEEX, SEEY);
}


void mapgen_ants_queen(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    mapgen_ants_generic(m, terrain_type, dat, turn, density);
    m->place_items("ant_egg",  98, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
    m->add_spawn(mon_ant_queen, 1, SEEX, SEEY);

}


void mapgen_tutorial(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    (void) density; // Not used, no normally generated zombies here
    (void) terrain_type; // Not used, should always be "tutorial"
    (void) turn; // Not used for tutorial
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            if (j == 0 || j == SEEY * 2 - 1) {
                m->ter_set(i, j, t_wall);
            } else if (i == 0 || i == SEEX * 2 - 1) {
                m->ter_set(i, j, t_wall);
            } else if (j == SEEY) {
                if (i % 4 == 2) {
                    m->ter_set(i, j, t_door_c);
                } else if (i % 5 == 3) {
                    m->ter_set(i, j, t_window_domestic);
                } else {
                    m->ter_set(i, j, t_wall);
                }
            } else {
                m->ter_set(i, j, t_floor);
            }
        }
    }
    m->furn_set(7, SEEY * 2 - 4, f_rack);
    m->place_gas_pump(SEEX * 2 - 2, SEEY * 2 - 4, rng(500, 1000));
    if( dat.zlevel < 0 ) {
        m->ter_set(SEEX - 2, SEEY + 2, t_stairs_up);
        m->ter_set(2, 2, t_water_sh);
        m->ter_set(2, 3, t_water_sh);
        m->ter_set(3, 2, t_water_sh);
        m->ter_set(3, 3, t_water_sh);
    } else {
        m->spawn_item(           5, SEEY + 1, "helmet_bike");
        m->spawn_item(           4, SEEY + 1, "backpack");
        m->spawn_item(           3, SEEY + 1, "pants_cargo");
        m->spawn_item(           7, SEEY * 2 - 4, "machete");
        m->spawn_item(           7, SEEY * 2 - 4, "9mm");
        m->spawn_item(           7, SEEY * 2 - 4, "9mmP");
        m->spawn_item(           7, SEEY * 2 - 4, "uzi");
        m->spawn_item(           7, SEEY * 2 - 4, "uzimag");
        m->spawn_item(SEEX * 2 - 2, SEEY + 5, "bubblewrap");
        m->spawn_item(SEEX * 2 - 2, SEEY + 6, "grenade");
        m->spawn_item(SEEX * 2 - 3, SEEY + 6, "flashlight");
        m->spawn_item(SEEX * 2 - 2, SEEY + 7, "cig");
        m->spawn_item(SEEX * 2 - 2, SEEY + 7, "codeine");
        m->spawn_item(SEEX * 2 - 3, SEEY + 7, "water");
        m->ter_set(SEEX - 2, SEEY + 2, t_stairs_down);
    }
}

void madd_trap( map *m, int x, int y, trap_id t )
{
    tripoint actual_location( x, y, m->get_abs_sub().z );
    m->add_trap( actual_location, t );
}

void mremove_trap( map *m, int x, int y )
{
    tripoint actual_location( x, y, m->get_abs_sub().z );
    m->remove_trap( actual_location );
}

void mtrap_set( map *m, int x, int y, trap_id t )
{
    tripoint actual_location( x, y, m->get_abs_sub().z );
    m->trap_set( actual_location, t );
}

void madd_field( map *m, int x, int y, field_id t, int density )
{
    tripoint actual_location( x, y, m->get_abs_sub().z );
    m->add_field( actual_location, t, density, 0 );
}
