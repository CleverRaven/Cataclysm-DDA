#include "mapbuffer.h"
#include "game.h"
#include "output.h"
#include "debug.h"
#include "translations.h"
#include "compress.h"
#include <fstream>

#define dbg(x) dout((DebugLevel)(x),D_MAP) << __FILE__ << ":" << __LINE__ << ": "

mapbuffer MAPBUFFER;

// g defaults to NULL
mapbuffer::mapbuffer()
{
 dirty = false;
}

mapbuffer::~mapbuffer()
{
 reset();
}

void mapbuffer::reset(){
 std::list<submap*>::iterator it;
 for (it = submap_list.begin(); it != submap_list.end(); it++)
  delete *it;

 submaps.clear();
 submap_list.clear();
}

// game g's existance does not imply that it has been identified, started, or loaded.
void mapbuffer::set_game(game *g)
{
 master_game = g;
}

// set to dirty right before the game starts & the player starts changing stuff.
void mapbuffer::set_dirty()
{
 dirty = true;
}
// initial state; no need to synchronize.
// make volatile after game has ended.
void mapbuffer::make_volatile()
{
 dirty = false;
}

bool mapbuffer::add_submap(int x, int y, int z, submap *sm)
{
 dbg(D_INFO) << "mapbuffer::add_submap( x["<< x <<"], y["<< y <<"], z["<< z <<"], submap["<< sm <<"])";

 tripoint p(x, y, z);
 if (submaps.count(p) != 0)
  return false;

 if (master_game)
  sm->turn_last_touched = int(master_game->turn);
 submap_list.push_back(sm);
 submaps[p] = sm;

 return true;
}

submap* mapbuffer::lookup_submap(int x, int y, int z)
{
 dbg(D_INFO) << "mapbuffer::lookup_submap( x["<< x <<"], y["<< y <<"], z["<< z <<"])";

 tripoint p(x, y, z);

 if (submaps.count(p) == 0)
  return NULL;

 dbg(D_INFO) << "mapbuffer::lookup_submap success: "<< submaps[p];

 return submaps[p];
}

void mapbuffer::save_if_dirty()
{
 if(dirty)
  save();
}

void mapbuffer::save()
{
    std::map<tripoint, submap*, pointcomp>::iterator it;
    std::ofstream fout;
    std::stringstream data;
    fout.open("save/maps.txt");

    // Some pairs of numbers could form a valid header for a zlib file,
    // e.g. "41", so add a pair that won't
    data << "00"; 
    data << submap_list.size() << std::endl;
    int num_saved_submaps = 0;
    int num_total_submaps = submap_list.size();

    for (it = submaps.begin(); it != submaps.end(); it++) {
        if (num_saved_submaps % 100 == 0)
            popup_nowait(_("Please wait as the map saves [%d/%d]"),
                         num_saved_submaps, num_total_submaps);

        data << it->first.x << " " << it->first.y << " " << it->first.z << std::endl;
        submap *sm = it->second;
        data << sm->turn_last_touched << std::endl;
// Dump the terrain.
        for (int j = 0; j < SEEY; j++) {
            for (int i = 0; i < SEEX; i++)
                data << int(sm->ter[i][j]) << " ";
            data << std::endl;
        }
        // Dump the radiation
        for (int j = 0; j < SEEY; j++) {
            for (int i = 0; i < SEEX; i++)
                data << sm->rad[i][j] << " ";
        }
        data << std::endl;

        // Furniture
        for (int j = 0; j < SEEY; j++) {
            for (int i = 0; i < SEEX; i++) {
                if (sm->frn[i][j] != f_null)
                    data << "f " << i << " " << j << " " << sm->frn[i][j] <<
                        std::endl;
            }
        }
        // Items section; designate it with an I.  Then check itm[][] for each
        //   square in the grid and print the coords and the item's details.
        // Designate it with a C if it's contained in the prior item.
        // Also, this wastes space since we print the coords for each item, when
        //   we could be printing a list of items for each coord (except the
        //   empty ones)
        item tmp;
        for (int j = 0; j < SEEY; j++) {
            for (int i = 0; i < SEEX; i++) {
                for (int k = 0; k < sm->itm[i][j].size(); k++) {
                    tmp = sm->itm[i][j][k];
                    data << "I " << i << " " << j << std::endl;
                    data << tmp.save_info() << std::endl;
                    for (int l = 0; l < tmp.contents.size(); l++)
                        data << "C " << std::endl << tmp.contents[l].save_info() << std::endl;
                }
            }
        }
        // Output the traps
        for (int j = 0; j < SEEY; j++) {
            for (int i = 0; i < SEEX; i++) {
                if (sm->trp[i][j] != tr_null)
                    data << "T " << i << " " << j << " " << sm->trp[i][j] <<
                        std::endl;
            }
        }

        // Output the fields
        for (int j = 0; j < SEEY; j++) {
            for (int i = 0; i < SEEX; i++) {
                if (sm->fld[i][j].fieldCount() > 0){
                    for(std::map<field_id, field_entry*>::iterator it = sm->fld[i][j].getFieldStart();
                        it != sm->fld[i][j].getFieldEnd(); ++it){
                        if(it->second != NULL){
                            data << "F " << i << " " << j << " " << int(it->second->getFieldType()) << " " <<
                                int(it->second->getFieldDensity()) << " " << (it->second->getFieldAge()) << std::endl;
                        }
                    }
                }
            }
        }
        // Output the spawn points
        spawn_point tmpsp;
        for (int i = 0; i < sm->spawns.size(); i++) {
            tmpsp = sm->spawns[i];
            data << "S " << int(tmpsp.type) << " " << tmpsp.count << " " << tmpsp.posx <<
                " " << tmpsp.posy << " " << tmpsp.faction_id << " " <<
                tmpsp.mission_id << (tmpsp.friendly ? " 1 " : " 0 ") <<
                tmpsp.name << std::endl;
        }
        // Output the vehicles
        for (int i = 0; i < sm->vehicles.size(); i++) {
            data << "V ";
            sm->vehicles[i]->save (data);
        }
        // Output the computer
        if (sm->comp.name != "")
            data << "c " << sm->comp.save_data() << std::endl;

        // Output base camp if any
        if (sm->camp.is_valid())
            data << "B " << sm->camp.save_data() << std::endl;

        // Output the graffiti
        for (int j = 0; j < SEEY; j++) {
            for (int i = 0; i < SEEX; i++) {
                if (sm->graf[i][j].contents)
                    data << "G " << i << " " << j << *sm->graf[i][j].contents << std::endl;
            }
        }

        data << "----" << std::endl;
        num_saved_submaps++;
    }
    fout << compress_string(data.str());
    // Close the file; that's all we need.
    fout.close();
}

void mapbuffer::load()
{
    if (!master_game) {
        debugmsg("Can't load mapbuffer without a master_game");
        return;
    }
    std::map<tripoint, submap*>::iterator it;
    std::ifstream fin;
    fin.open("save/maps.txt");
    if (!fin.is_open())
        return;

    std::stringstream data;

    char buffer[1024];
    while (!fin.eof())
    {
        fin.read(buffer,1024);
        data.write(buffer,fin.gcount());
    }
    fin.close();

    data.str(decompress_string(data.str()));

    int itx, ity, t, d, a, num_submaps, num_loaded = 0;
    item it_tmp;
    std::string databuff;
    data >> num_submaps;

    while (!data.eof()) {
        if (num_loaded % 100 == 0)
            popup_nowait(_("Please wait as the map loads [%d/%d]"),
                         num_loaded, num_submaps);
        int locx, locy, locz, turn;
        submap* sm = new submap();
        data >> locx >> locy >> locz >> turn;
        if(data.eof()) {
            break;
        }
        sm->turn_last_touched = turn;
        int turndif = (master_game ? int(master_game->turn) - turn : 0);
        if (turndif < 0)
            turndif = 0;
// Load terrain
        for (int j = 0; j < SEEY; j++) {
            for (int i = 0; i < SEEX; i++) {
                int tmpter;
                data >> tmpter;
                sm->ter[i][j] = ter_id(tmpter);
                sm->frn[i][j] = f_null;
                sm->itm[i][j].clear();
                sm->trp[i][j] = tr_null;
                //sm->fld[i][j] = field(); //not needed now
                sm->graf[i][j] = graffiti();
            }
        }
// Load irradiation
        for (int j = 0; j < SEEY; j++) {
            for (int i = 0; i < SEEX; i++) {
                int radtmp;
                data >> radtmp;
                radtmp -= int(turndif / 100);	// Radiation slowly decays
                if (radtmp < 0)
                    radtmp = 0;
                sm->rad[i][j] = radtmp;
            }
        }
// Load items and traps and fields and spawn points and vehicles
        std::string string_identifier;
        do {
            data >> string_identifier; // "----" indicates end of this submap
            t = 0;
            if (string_identifier == "I") {
                data >> itx >> ity;
                getline(data, databuff); // Clear out the endline
                getline(data, databuff);
                it_tmp.load_info(databuff, master_game);
                sm->itm[itx][ity].push_back(it_tmp);
                if (it_tmp.active)
                    sm->active_item_count++;
            } else if (string_identifier == "C") {
                getline(data, databuff); // Clear out the endline
                getline(data, databuff);
                int index = sm->itm[itx][ity].size() - 1;
                it_tmp.load_info(databuff, master_game);
                sm->itm[itx][ity][index].put_in(it_tmp);
                if (it_tmp.active)
                    sm->active_item_count++;
            } else if (string_identifier == "T") {
                data >> itx >> ity >> t;
                sm->trp[itx][ity] = trap_id(t);
            } else if (string_identifier == "f") {
                data >> itx >> ity >> t;
                sm->frn[itx][ity] = furn_id(t);
            } else if (string_identifier == "F") {
                data >> itx >> ity >> t >> d >> a;
                if(!sm->fld[itx][ity].findField(field_id(t)))
                    sm->field_count++;
                sm->fld[itx][ity].addField(field_id(t), d, a);
            } else if (string_identifier == "S") {
                char tmpfriend;
                int tmpfac = -1, tmpmis = -1;
                std::string spawnname;
                data >> t >> a >> itx >> ity >> tmpfac >> tmpmis >> tmpfriend >> spawnname;
                spawn_point tmp(mon_id(t), a, itx, ity, tmpfac, tmpmis, (tmpfriend == '1'),
                                spawnname);
                sm->spawns.push_back(tmp);
            } else if (string_identifier == "V") {
                vehicle * veh = new vehicle(master_game);
                veh->load (data);
                //veh.smx = gridx;
                //veh.smy = gridy;
                master_game->m.vehicle_list.insert(veh);
                sm->vehicles.push_back(veh);
            } else if (string_identifier == "c") {
                getline(data, databuff);
                sm->comp.load_data(databuff);
            } else if (string_identifier == "B") {
                getline(data, databuff);
                sm->camp.load_data(databuff);
            } else if (string_identifier == "G") {
                std::string s;
                int j;
                int i;
                data >> j >> i;
                getline(data,s);
                sm->graf[j][i] = graffiti(s);
            }
        } while (string_identifier != "----" && !data.eof());

        submap_list.push_back(sm);
        submaps[ tripoint(locx, locy, locz) ] = sm;
        num_loaded++;
    }
}

int mapbuffer::size()
{
 return submap_list.size();
}
