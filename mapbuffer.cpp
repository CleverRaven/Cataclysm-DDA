#include "mapbuffer.h"
#include "game.h"
#include "output.h"
#include "debug.h"
#include <fstream>

#define dbg(x) dout((DebugLevel)(x),D_MAP) << __FILE__ << ":" << __LINE__ << ": "

mapbuffer MAPBUFFER;

// g defaults to NULL
mapbuffer::mapbuffer(game *g)
{
 master_game = g;
}

mapbuffer::~mapbuffer()
{
 std::list<submap*>::iterator it;

 for (it = submap_list.begin(); it != submap_list.end(); it++)
  delete *it;
}

void mapbuffer::set_game(game *g)
{
 master_game = g;
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

void mapbuffer::save()
{
 std::map<tripoint, submap*>::iterator it;
 std::ofstream fout;
 fout.open("save/maps.txt");

 fout << submap_list.size() << std::endl;
 int num_saved_submaps = 0;
 int num_total_submaps = submap_list.size();

 for (it = submaps.begin(); it != submaps.end(); it++) {
  if (num_saved_submaps % 100 == 0)
   popup_nowait("Please wait as the map saves [%d/%d]",
                num_saved_submaps, num_total_submaps);

  fout << it->first.x << " " << it->first.y << " " << it->first.z << std::endl;
  submap *sm = it->second;
  fout << sm->turn_last_touched << std::endl;
// Dump the terrain.
  for (int j = 0; j < SEEY; j++) {
   for (int i = 0; i < SEEX; i++)
    fout << int(sm->ter[i][j]) << " ";
   fout << std::endl;
  }
 // Dump the radiation
  for (int j = 0; j < SEEY; j++) {
   for (int i = 0; i < SEEX; i++) 
    fout << sm->rad[i][j] << " ";
  }
  fout << std::endl;
 
 // Items section; designate it with an I.  Then check itm[][] for each square
 //   in the grid and print the coords and the item's details.
 // Designate it with a C if it's contained in the prior item.
 // Also, this wastes space since we print the coords for each item, when we
 //   could be printing a list of items for each coord (except the empty ones)
  item tmp;
  for (int j = 0; j < SEEY; j++) {
   for (int i = 0; i < SEEX; i++) {
    for (int k = 0; k < sm->itm[i][j].size(); k++) {
     tmp = sm->itm[i][j][k];
     fout << "I " << i << " " << j << std::endl;
     fout << tmp.save_info() << std::endl;
     for (int l = 0; l < tmp.contents.size(); l++)
      fout << "C " << std::endl << tmp.contents[l].save_info() << std::endl;
    }
   }
  }
 // Output the traps
  for (int j = 0; j < SEEY; j++) {
   for (int i = 0; i < SEEX; i++) {
    if (sm->trp[i][j] != tr_null)
     fout << "T " << i << " " << j << " " << sm->trp[i][j] <<
     std::endl;
   }
  }
 
 // Output the fields
  field tmpf;
  for (int j = 0; j < SEEY; j++) {
   for (int i = 0; i < SEEX; i++) {
    tmpf = sm->fld[i][j];
    if (tmpf.type != fd_null)
     fout << "F " << i << " " << j << " " << int(tmpf.type) << " " <<
             int(tmpf.density) << " " << tmpf.age << std::endl;
   }
  }
 // Output the spawn points
  spawn_point tmpsp;
  for (int i = 0; i < sm->spawns.size(); i++) {
   tmpsp = sm->spawns[i];
   fout << "S " << int(tmpsp.type) << " " << tmpsp.count << " " << tmpsp.posx <<
           " " << tmpsp.posy << " " << tmpsp.faction_id << " " <<
           tmpsp.mission_id << (tmpsp.friendly ? " 1 " : " 0 ") <<
           tmpsp.name << std::endl;
  }
 // Output the vehicles
  for (int i = 0; i < sm->vehicles.size(); i++) {
   fout << "V ";
   sm->vehicles[i]->save (fout);
  }
 // Output the computer
  if (sm->comp.name != "")
   fout << "c " << sm->comp.save_data() << std::endl;
  fout << "----" << std::endl;
  num_saved_submaps++;
 }
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

 int itx, ity, t, d, a, num_submaps, num_loaded=0;
 bool fields_here = false;
 item it_tmp;
 std::string databuff;
 fin >> num_submaps;

 while (!fin.eof()) {
  if (num_loaded % 100 == 0)
   popup_nowait("Please wait as the map loads [%d/%d]",
                num_loaded, num_submaps);
  int locx, locy, locz, turn;
  submap* sm = new submap;
  fin >> locx >> locy >> locz >> turn;
  sm->turn_last_touched = turn;
  int turndif = (master_game ? int(master_game->turn) - turn : 0);
  if (turndif < 0)
   turndif = 0;
// Load terrain
  for (int j = 0; j < SEEY; j++) {
   for (int i = 0; i < SEEX; i++) {
    int tmpter;
    fin >> tmpter;
    sm->ter[i][j] = ter_id(tmpter);
    sm->itm[i][j].clear();
    sm->trp[i][j] = tr_null;
    sm->fld[i][j] = field();
   }
  }
// Load irradiation
  for (int j = 0; j < SEEY; j++) {
   for (int i = 0; i < SEEX; i++) {
    int radtmp;
    fin >> radtmp;
    radtmp -= int(turndif / 100);	// Radiation slowly decays
    if (radtmp < 0)
     radtmp = 0;
    sm->rad[i][j] = radtmp;
   }
  }
// Load items and traps and fields and spawn points and vehicles
  std::string string_identifier;
  do {
   fin >> string_identifier; // "----" indicates end of this submap
   t = 0;
   if (string_identifier == "I") {
    fin >> itx >> ity;
    getline(fin, databuff); // Clear out the endline
    getline(fin, databuff);
    it_tmp.load_info(databuff, master_game);
    sm->itm[itx][ity].push_back(it_tmp);
    if (it_tmp.active)
     sm->active_item_count++;
   } else if (string_identifier == "C") {
    getline(fin, databuff); // Clear out the endline
    getline(fin, databuff);
    int index = sm->itm[itx][ity].size() - 1;
    it_tmp.load_info(databuff, master_game);
    sm->itm[itx][ity][index].put_in(it_tmp);
    if (it_tmp.active)
     sm->active_item_count++;
   } else if (string_identifier == "T") {
    fin >> itx >> ity >> t;
    sm->trp[itx][ity] = trap_id(t);
   } else if (string_identifier == "F") {
    fields_here = true;
    fin >> itx >> ity >> t >> d >> a;
    sm->fld[itx][ity] = field(field_id(t), d, a);
    sm->field_count++;
   } else if (string_identifier == "S") {
    char tmpfriend;
    int tmpfac = -1, tmpmis = -1;
    std::string spawnname;
    fin >> t >> a >> itx >> ity >> tmpfac >> tmpmis >> tmpfriend >> spawnname;
    spawn_point tmp(mon_id(t), a, itx, ity, tmpfac, tmpmis, (tmpfriend == '1'),
                    spawnname);
    sm->spawns.push_back(tmp);
   } else if (string_identifier == "V") {
    vehicle * veh = new vehicle(master_game);
    veh->load (fin);
    //veh.smx = gridx;
    //veh.smy = gridy;
    master_game->m.vehicle_list.insert(veh);
    sm->vehicles.push_back(veh);
   } else if (string_identifier == "c") {
    getline(fin, databuff);
    sm->comp.load_data(databuff);
   }
  } while (string_identifier != "----" && !fin.eof());

  submap_list.push_back(sm);
  submaps[ tripoint(locx, locy, locz) ] = sm;
  num_loaded++;
 }
 fin.close();
}

int mapbuffer::size()
{
 return submap_list.size();
}
