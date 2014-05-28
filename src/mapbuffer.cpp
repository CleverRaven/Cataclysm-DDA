#include "mapbuffer.h"
#include "output.h"
#include "debug.h"
#include "translations.h"
#include "savegame.h"
#include "file_wrapper.h"
#include "overmapbuffer.h"
#include "mapsharing.h"
#include "mapdata.h"
#include "worldfactory.h"
#include "game.h"
#include <fstream>
#include <sstream>

#define dbg(x) dout((DebugLevel)(x),D_MAP) << __FILE__ << ":" << __LINE__ << ": "

mapbuffer MAPBUFFER;

// g defaults to NULL
mapbuffer::mapbuffer()
{
}

mapbuffer::~mapbuffer()
{
    reset();
}

void mapbuffer::reset()
{
    std::list<submap *>::iterator it;
    for (it = submap_list.begin(); it != submap_list.end(); it++) {
        delete *it;
    }

    submaps.clear();
    submap_list.clear();
}

bool mapbuffer::add_submap(int x, int y, int z, submap *sm)
{
    dbg(D_INFO) << "mapbuffer::add_submap( x[" <<
                x << "], y[" << y << "], z[" << z << "], submap[" << sm << "])";

    const tripoint p(x, y, z);
    if (submaps.count(p) != 0) {
        return false;
    }

    sm->turn_last_touched = int(calendar::turn);
    submap_list.push_back(sm);
    submaps[p] = sm;

    return true;
}

void mapbuffer::remove_submap( tripoint addr )
{
    if (submaps.count( addr ) == 0) {
        return;
    }
    std::map<tripoint, submap *, pointcomp>::iterator m_target = submaps.find( addr );
    std::list<submap *>::iterator l_target = find( submap_list.begin(), submap_list.end(),
                                                   m_target->second );
    // We're probably leaking vehicle objects here.
    delete m_target->second;
    submap_list.erase( l_target );
    submaps.erase( m_target );
}

submap *mapbuffer::lookup_submap(int x, int y, int z)
{
    dbg(D_INFO) << "mapbuffer::lookup_submap( x[" << x << "], y[" << y << "], z[" << z << "])";

    const tripoint p(x, y, z);
    if (submaps.count(p) == 0) {
        try {
            return unserialize_submaps( p );
        } catch (std::string &err) {
            debugmsg("Failed to load submap (%d,%d,%d): %s", x, y, z, err.c_str());
        } catch (const std::exception &err) {
            debugmsg("Failed to load submap (%d,%d,%d): %s", x, y, z, err.what());
        }
        return NULL;
    }

    dbg(D_INFO) << "mapbuffer::lookup_submap success: " << submaps[p];

    return submaps[p];
}

void mapbuffer::save( bool delete_after_save )
{
    std::stringstream map_directory;
    map_directory << world_generator->active_world->world_path << "/maps";
    assure_dir_exist( map_directory.str().c_str() );

    int num_saved_submaps = 0;
    int num_total_submaps = submap_list.size();

    point map_origin = overmapbuffer::sm_to_omt_copy( g->levx, g->levy );
    map_origin.x += g->cur_om->pos().x * OMAPX;
    map_origin.y += g->cur_om->pos().y * OMAPY;

    // A set of already-saved submaps, in global overmap coordinates.
    std::set<tripoint, pointcomp> saved_submaps;
    std::list<tripoint> submaps_to_delete;
    for( std::map<tripoint, submap *, pointcomp>::iterator it = submaps.begin();
         it != submaps.end(); ++it ) {
        if (num_total_submaps > 100 && num_saved_submaps % 100 == 0) {
            popup_nowait(_("Please wait as the map saves [%d/%d]"),
                         num_saved_submaps, num_total_submaps);
        }
        // Whatever the coordinates of the current submap are,
        // we're saving a 2x2 quad of submaps at a time.
        // Submaps are generated in quads, so we know if we have one member of a quad,
        // we have the rest of it, if that assumtion is broken we have REAL problems.
        const tripoint om_addr = overmapbuffer::sm_to_omt_copy( it->first );
        if( saved_submaps.count( om_addr ) != 0 ) {
            // Already handled this one.
            continue;
        }
        saved_submaps.insert( om_addr );

        // A segment is a chunk of 32x32 submap quads.
        // We're breaking them into subdirectories so there aren't too many files per directory.
        // Might want to make a set for this one too so it's only checked once per save().
        std::stringstream segment_path;
        tripoint segment_addr = overmapbuffer::omt_to_seg_copy( om_addr );
        segment_path << map_directory.str() << "/" << segment_addr.x << "." <<
            segment_addr.y << "." << segment_addr.z;
        assure_dir_exist( segment_path.str().c_str() );

        std::stringstream quad_path;
        quad_path << segment_path.str() << "/" << om_addr.x << "." <<
            om_addr.y << "." << om_addr.z << ".map";

                   // delete_on_save deletes everything, otherwise delete submaps
                   // outside the current map.
        save_quad( quad_path.str(), om_addr, submaps_to_delete,
                   delete_after_save || om_addr.z != g->levz ||
                   om_addr.x < map_origin.x || om_addr.y < map_origin.y ||
                   om_addr.x > map_origin.x + (MAPSIZE / 2) ||
                   om_addr.y > map_origin.y + (MAPSIZE / 2) );
        num_saved_submaps += 4;
    }
    for( std::list<tripoint>::iterator it = submaps_to_delete.begin();
         it != submaps_to_delete.end(); ++it ) {
        remove_submap( *it );
    }
}

void mapbuffer::save_quad( const std::string &filename, const tripoint &om_addr,
                           std::list<tripoint> &submaps_to_delete, bool delete_after_save )
{
    std::ofstream fout;
    fopen_exclusive(fout, filename.c_str());
    if(!fout.is_open()) {
        return;
    }

    std::vector<point> offsets;
    offsets.push_back( point(0, 0) );
    offsets.push_back( point(0, 1) );
    offsets.push_back( point(1, 0) );
    offsets.push_back( point(1, 1) );
    JsonOut jsout( fout );
    jsout.start_array();
    for( std::vector<point>::iterator offset = offsets.begin();
         offset != offsets.end(); ++offset ) {
        tripoint submap_addr = overmapbuffer::omt_to_sm_copy( om_addr );
        submap_addr.x += offset->x;
        submap_addr.y += offset->y;

        if (submaps.count(submap_addr) == 0) {
            continue;
        }
        submap *sm = submaps[submap_addr];
        if( sm == NULL ) {
            continue;
        }

        jsout.start_object();

        jsout.member( "version", savegame_version);

        jsout.member( "coordinates" );
        jsout.start_array();
        jsout.write( submap_addr.x );
        jsout.write( submap_addr.y );
        jsout.write( submap_addr.z );
        jsout.end_array();

        jsout.member( "turn_last_touched", sm->turn_last_touched );
        jsout.member( "temperature", sm->temperature );

        jsout.member( "terrain" );
        jsout.start_array();
        for(int j = 0; j < SEEY; j++) {
            for(int i = 0; i < SEEX; i++) {
                // Save terrains
                jsout.write( terlist[sm->ter[i][j]].id );
            }
        }
        jsout.end_array();

        // Write out the radiation array in a simple RLE scheme.
        // written in intensity, count pairs
        jsout.member( "radiation" );
        jsout.start_array();
        int lastrad = -1;
        int count = 0;
        for(int j = 0; j < SEEY; j++) {
            for(int i = 0; i < SEEX; i++) {
                // Save radiation, re-examine this because it doesnt look like it works right
                int r = sm->get_radiation(i, j);
                if (r == lastrad) {
                    count++;
                } else {
                    if (count) {
                        jsout.write( count );
                    }
                    jsout.write( r );
                    lastrad = r;
                    count = 1;
                }
            }
        }
        jsout.write( count );
        jsout.end_array();

        jsout.member("furniture");
        jsout.start_array();
        for(int j = 0; j < SEEY; j++) {
            for(int i = 0; i < SEEX; i++) {
                // Save furniture
                if( sm->get_furn( i, j ) != f_null ) {
                    jsout.start_array();
                    jsout.write( i );
                    jsout.write( j );
                    jsout.write( furnlist[ sm->get_furn( i, j ) ].id );
                    jsout.end_array();
                }
            }
        }
        jsout.end_array();

        jsout.member( "items" );
        jsout.start_array();
        for(int j = 0; j < SEEY; j++) {
            for(int i = 0; i < SEEX; i++) {
                if( sm->itm[i][j].empty() ) {
                    continue;
                }
                jsout.write( i );
                jsout.write( j );
                jsout.write( sm->itm[i][j] );
            }
        }
        jsout.end_array();

        jsout.member( "traps" );
        jsout.start_array();
        for(int j = 0; j < SEEY; j++) {
            for(int i = 0; i < SEEX; i++) {
                // Save traps
                if (sm->get_trap( i, j ) != tr_null) {
                    jsout.start_array();
                    jsout.write( i );
                    jsout.write( j );
                    jsout.write( traplist[ sm->get_trap( i, j ) ]->id );
                    jsout.end_array();
                }
            }
        }
        jsout.end_array();

        jsout.member( "fields" );
        jsout.start_array();
        for(int j = 0; j < SEEY; j++) {
            for(int i = 0; i < SEEX; i++) {
                // Save fields
                if (sm->fld[i][j].fieldCount() > 0) {
                    jsout.write( i );
                    jsout.write( j );
                    jsout.start_array();
                    for(std::map<field_id, field_entry *>::iterator it = sm->fld[i][j].getFieldStart();
                        it != sm->fld[i][j].getFieldEnd(); ++it) {
                        if(it->second != NULL) {
                            // We don't seem to have a string identifier for fields anywhere.
                            jsout.write( it->second->getFieldType() );
                            jsout.write( it->second->getFieldDensity() );
                            jsout.write( it->second->getFieldAge() );
                        }
                    }
                    jsout.end_array();
                }
            }
        }
        jsout.end_array();

        jsout.member( "graffiti" );
        jsout.start_array();
        for(int j = 0; j < SEEY; j++) {
            for(int i = 0; i < SEEX; i++) {
                // Save graffiti
                if (sm->get_graffiti(i, j).contents) {
                    jsout.start_array();
                    jsout.write( i );
                    jsout.write( j );
                    jsout.write( *sm->get_graffiti(i, j).contents );
                    jsout.end_array();
                }
            }
        }
        jsout.end_array();

        // Output the spawn points
        jsout.member( "spawns" );
        jsout.start_array();
        for( std::vector<spawn_point>::iterator spawn_it = sm->spawns.begin();
             spawn_it != sm->spawns.end(); ++spawn_it ) {
            jsout.start_array();
            jsout.write( spawn_it->type );
            jsout.write( spawn_it->count );
            jsout.write( spawn_it->posx );
            jsout.write( spawn_it->posy );
            jsout.write( spawn_it->faction_id );
            jsout.write( spawn_it->mission_id );
            jsout.write( spawn_it->friendly );
            jsout.write( spawn_it->name );
            jsout.end_array();
        }
        jsout.end_array();

        jsout.member( "vehicles" );
        jsout.start_array();
        for( std::vector<vehicle *>::iterator vehicle_it = sm->vehicles.begin();
             vehicle_it != sm->vehicles.end(); ++vehicle_it ) {
            // json lib doesn't know how to turn a vehicle * into a vehicle,
            // so we have to iterate manually.
            jsout.write( **vehicle_it );
        }
        jsout.end_array();

        // Output the computer
        if (sm->comp.name != "") {
            jsout.member( "computers", sm->comp.save_data() );
        }

        // Output base camp if any
        if (sm->camp.is_valid()) {
            jsout.member( "camp" );
            jsout.write( sm->camp.save_data() );
        }
        if( delete_after_save ) {
            submaps_to_delete.push_back( submap_addr );
        }
        jsout.end_object();
    }
    jsout.end_array();
    fclose_exclusive(fout, filename.c_str());
}

// We're reading in way too many entities here to mess around with creating sub-objects and
// seeking around in them, so we're using the json streaming API.
submap *mapbuffer::unserialize_submaps( const tripoint &p )
{
    // Map the tripoint to the submap quad that stores it.
    const tripoint om_addr = overmapbuffer::sm_to_omt_copy( p );
    const tripoint segment_addr = overmapbuffer::omt_to_seg_copy( om_addr );
    std::stringstream quad_path;
    quad_path << world_generator->active_world->world_path << "/maps/" <<
        segment_addr.x << "." << segment_addr.y << "." << segment_addr.z << "/" <<
        om_addr.x << "." << om_addr.y << "." << om_addr.z << ".map";

    std::ifstream fin( quad_path.str().c_str() );
    if( !fin.is_open() ) {
        // If it doesn't exist, trigger generating it.
        return NULL;
    }

    JsonIn jsin( fin );
    jsin.start_array();
    while( !jsin.end_array() ) {
        submap *sm = new submap();
        tripoint submap_coordinates;
        jsin.start_object();
        while( !jsin.end_object() ) {
            std::string submap_member_name = jsin.get_member_name();
            if( submap_member_name == "version" ) {
                // We aren't using the version number for anything at the moment.
                jsin.skip_value();
            } else if( submap_member_name == "coordinates" ) {
                jsin.start_array();
                int locx = jsin.get_int();
                int locy = jsin.get_int();
                int locz = jsin.get_int();
                jsin.end_array();
                submap_coordinates = tripoint( locx, locy, locz );
            } else if( submap_member_name == "turn_last_touched" ) {
                sm->turn_last_touched = jsin.get_int();
            } else if( submap_member_name == "temperature" ) {
                sm->temperature = jsin.get_int();
            } else if( submap_member_name == "terrain" ) {
                // TODO: try block around this to error out if we come up short?
                jsin.start_array();
                for( int j = 0; j < SEEY; j++ ) {
                    for( int i = 0; i < SEEX; i++ ) {
                        sm->ter[i][j] = termap[ jsin.get_string() ].loadid;
                    }
                }
                jsin.end_array();
            } else if( submap_member_name == "radiation" ) {
                int rad_cell = 0;
                jsin.start_array();
                while( !jsin.end_array() ) {
                    int rad_strength = jsin.get_int();
                    int rad_num = jsin.get_int();
                    for( int i = 0; i < rad_num; ++i ) {
                        // A little array trick here, assign to it as a 1D array.
                        // If it's not in bounds we're kinda hosed anyway.
                        sm->set_radiation(0, rad_cell, rad_strength);
                        rad_cell++;
                    }
                }
            } else if( submap_member_name == "furniture" ) {
                jsin.start_array();
                while( !jsin.end_array() ) {
                    jsin.start_array();
                    int i = jsin.get_int();
                    int j = jsin.get_int();
                    sm->frn[i][j] = furnmap[ jsin.get_string() ].loadid;
                    jsin.end_array();
                }
            } else if( submap_member_name == "items" ) {
                jsin.start_array();
                while( !jsin.end_array() ) {
                    int i = jsin.get_int();
                    int j = jsin.get_int();
                    jsin.start_array();
                    while( !jsin.end_array() ) {
                        item tmp;
                        jsin.read( tmp );
                        sm->itm[i][j].push_back( tmp );
                    }
                }
            } else if( submap_member_name == "traps" ) {
                jsin.start_array();
                while( !jsin.end_array() ) {
                    jsin.start_array();
                    int i = jsin.get_int();
                    int j = jsin.get_int();
                    sm->trp[i][j] = trapmap[ jsin.get_string() ];
                    jsin.end_array();
                }
            } else if( submap_member_name == "fields" ) {
                jsin.start_array();
                while( !jsin.end_array() ) {
                    // Coordinates loop
                    int i = jsin.get_int();
                    int j = jsin.get_int();
                    jsin.start_array();
                    while( !jsin.end_array() ) {
                        int type = jsin.get_int();
                        int density = jsin.get_int();
                        int age = jsin.get_int();
                        if (sm->fld[i][j].findField(field_id(type)) == NULL) {
                            sm->field_count++;
                        }
                        sm->fld[i][j].addField(field_id(type), density, age);
                    }
                }
            } else if( submap_member_name == "griffiti" ) {
                jsin.start_array();
                while( !jsin.end_array() ) {
                    jsin.start_array();
                    int i = jsin.get_int();
                    int j = jsin.get_int();
                    sm->set_graffiti(i, j, graffiti( jsin.get_string() ));
                    jsin.end_array();
                }
            } else if( submap_member_name == "spawns" ) {
                jsin.start_array();
                while( !jsin.end_array() ) {
                    jsin.start_array();
                    std::string type = jsin.get_string();
                    int count = jsin.get_int();
                    int i = jsin.get_int();
                    int j = jsin.get_int();
                    int faction_id = jsin.get_int();
                    int mission_id = jsin.get_int();
                    bool friendly = jsin.get_bool();
                    std::string name = jsin.get_string();
                    jsin.end_array();
                    spawn_point tmp( type, count, i, j, faction_id, mission_id, friendly, name );
                    sm->spawns.push_back( tmp );
                }
            } else if( submap_member_name == "vehicles" ) {
                jsin.start_array();
                while( !jsin.end_array() ) {
                    vehicle *tmp = new vehicle();
                    jsin.read( *tmp );
                    sm->vehicles.push_back( tmp );
                }
            } else if( submap_member_name == "computers" ) {
                std::string computer_data = jsin.get_string();
                sm->comp.load_data( computer_data );
            } else if( submap_member_name == "camp" ) {
                std::string camp_data = jsin.get_string();
                sm->camp.load_data( camp_data );
            } else {
                jsin.skip_value();
            }
        }
        submap_list.push_back(sm);
        submaps[ submap_coordinates ] = sm;
    }
    return submaps[ p ];
}
