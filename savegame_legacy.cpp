
#include "game.h"
#include "output.h"
#include "skill.h"
#include "line.h"
#include "computer.h"
#include "options.h"
#include "auto_pickup.h"
#include "mapbuffer.h"
#include "debug.h"
#include "map.h"
#include "output.h"
#include "item_factory.h"
#include "artifact.h"
#include "overmapbuffer.h"
#include "trap.h"
#include "mapdata.h"
#include "translations.h"
#include <map>
#include <set>
#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>
#include <math.h>
#include <vector>
#include "debug.h"
#include "artifactdata.h"
#include "weather.h"
// for legacy classdata loaders
#include "inventory.h"
#include "item.h"
#include "monster.h"
#include "npc.h"
#include "overmap.h"
#include "player.h"
#include "pldata.h"
#include "profession.h"
#include "skill.h"
#include "vehicle.h"
//

/*
 * Properly reuse a stringstream object for line by line parsing
 */
inline std::stringstream & stream_line(std::ifstream & f, std::stringstream & s, std::string & buf) {
    s.clear();
    s.str("");
    getline(f, buf);
    s.str(buf);
    return s;
}

/*
 * Convenience macro for the above
 */
#define parseline() stream_line(fin,linein,linebuf)

/*
 * Parse an open .sav file in an old, now broken format to preserve a game across versions
 */
bool	 game::unserialize_legacy(std::ifstream & fin) {

   switch (savegame_loading_version) {

       case 7:
       case 5:
       case 4: {
   // Format version 4-?: Interim format. Still resembles a hairball, but it's at least a multi-line hairball;
   // Data is segmented for readabilty, stability, and gradual conversion into something closer to sanity.
            std::string linebuf;
            std::stringstream linein;

            int tmpturn, tmpspawn, tmprun, tmptar, comx, comy;

            parseline() >> tmpturn >> tmptar >> tmprun >> mostseen >> nextinv >> next_npc_id >>
                next_faction_id >> next_mission_id >> tmpspawn;

            getline(fin, linebuf);
            load_weather(linebuf);

            parseline() >> levx >> levy >> levz >> comx >> comy;

            turn = tmpturn;
            nextspawn = tmpspawn;

            cur_om = &overmap_buffer.get(this, comx, comy);
            m.load(this, levx, levy, levz);

            run_mode = tmprun;
            if (OPTIONS["SAFEMODE"] && run_mode == 0) {
                run_mode = 1;
            }
            autosafemode = OPTIONS["AUTOSAFEMODE"];
            last_target = tmptar;

            // Next, the scent map.
            parseline();

            for (int i = 0; i < SEEX *MAPSIZE; i++) {
                for (int j = 0; j < SEEY * MAPSIZE; j++) {
                    linein >> grscent[i][j];
                }
            }
            // Now the number of monsters...
            int nummon;
            parseline() >> nummon;

            // ... and the data on each one.
            std::string data;
            clear_zombies();
            monster montmp;
            int num_items;
            for (int i = 0; i < nummon; i++)
            {
                getline(fin, data);
                montmp.load_info(data, &mtypes);

                fin >> num_items;
                // Chomp the endline after number of items.
                getline( fin, data );
                for (int i = 0; i < num_items; i++) {
                    getline( fin, data );
                    montmp.inv.push_back( item( data, this ) );
                }

                add_zombie(montmp);
            }

            // And the kill counts;
            parseline();
            int kk;
            for (kk = 0; kk < num_monsters && !linein.eof(); kk++) {
                linein >> kills[kk];
            }
            if ( kk != num_monsters ) {
                debugmsg("Warning, number of monsters changed from %d to %d", kk+1, num_monsters );
            }

            // Finally, the data on the player.
            getline(fin, data);
            u.load_info(this, data);
            u.load_memorial_file( fin );

            // And the player's inventory...
            u.inv.load_invlet_cache( fin );

            char item_place;
            std::string itemdata;
            // We need a temporary vector of items.  Otherwise, when we encounter an item
            // which is contained in another item, the auto-sort/stacking behavior of the
            // player's inventory may cause the contained item to be misplaced.
            std::list<item> tmpinv;
            while (!fin.eof()) {
                fin >> item_place;
                if (!fin.eof()) {
                    getline(fin, itemdata);
                    if ( item_place == 'I' || item_place == 'C' || item_place == 'W' ||
                         item_place == 'S' || item_place == 'w' || item_place == 'c' ) {
                        item tmpitem(itemdata, this);
                        if (item_place == 'I') {
                            tmpinv.push_back(tmpitem);
                        } else if (item_place == 'C') {
                            tmpinv.back().contents.push_back(tmpitem);
                        } else if (item_place == 'W') {
                            u.worn.push_back(tmpitem);
                        } else if (item_place == 'S') {
                            u.worn.back().contents.push_back(tmpitem);
                        } else if (item_place == 'w') {
                            u.weapon = tmpitem;
                        } else if (item_place == 'c') {
                            u.weapon.contents.push_back(tmpitem);
                        }
                    }
                }
            }
            // Now dump tmpinv into the player's inventory
            u.inv.add_stack(tmpinv);
            return true;
        ////////
       } break;
       case 3: {

            std::string linebuf;
            std::stringstream linein;

            int tmpturn, tmpspawn, tmprun, tmptar, comx, comy;

            parseline() >> tmpturn >> tmptar >> tmprun >> mostseen >> nextinv >> next_npc_id >>
                next_faction_id >> next_mission_id >> tmpspawn;

            getline(fin, linebuf);
            load_weather(linebuf);

            parseline() >> levx >> levy >> levz >> comx >> comy;

            turn = tmpturn;
            nextspawn = tmpspawn;

            cur_om = &overmap_buffer.get(this, comx, comy);
            m.load(this, levx, levy, levz);

            run_mode = tmprun;
            if (OPTIONS["SAFEMODE"] && run_mode == 0) {
                run_mode = 1;
            }
            autosafemode = OPTIONS["AUTOSAFEMODE"];
            last_target = tmptar;

            // Next, the scent map.
            parseline();

            for (int i = 0; i < SEEX *MAPSIZE; i++) {
                for (int j = 0; j < SEEY * MAPSIZE; j++) {
                    linein >> grscent[i][j];
                }
            }
            // Now the number of monsters...
            int nummon;
            parseline() >> nummon;

            // ... and the data on each one.
            std::string data;
            clear_zombies();
            monster montmp;
            int num_items;
            for (int i = 0; i < nummon; i++)
            {
                getline(fin, data);
                montmp.load_info(data, &mtypes);

                fin >> num_items;
                // Chomp the endline after number of items.
                getline( fin, data );
                for (int i = 0; i < num_items; i++) {
                    getline( fin, data );
                    montmp.inv.push_back( item( data, this ) );
                }

                add_zombie(montmp);
            }

            // And the kill counts;
            parseline();
            int kk;
            for (kk = 0; kk < num_monsters && !linein.eof(); kk++) {
                linein >> kills[kk];
            }
            if ( kk != num_monsters ) {
                debugmsg("Warning, number of monsters changed from %d to %d", kk+1, num_monsters );
            }

            // Finally, the data on the player.
            getline(fin, data);
            u.load_info(this, data);
            u.load_memorial_file( fin );

            // And the player's inventory...
            u.inv.load_invlet_cache( fin );

            char item_place;
            std::string itemdata;
            // We need a temporary vector of items.  Otherwise, when we encounter an item
            // which is contained in another item, the auto-sort/stacking behavior of the
            // player's inventory may cause the contained item to be misplaced.
            std::list<item> tmpinv;
            while (!fin.eof()) {
                fin >> item_place;
                if (!fin.eof()) {
                    getline(fin, itemdata);
                    if ( item_place == 'I' || item_place == 'C' || item_place == 'W' ||
                         item_place == 'S' || item_place == 'w' || item_place == 'c' ) {
                        item tmpitem(itemdata, this);
                        if (item_place == 'I') {
                            tmpinv.push_back(tmpitem);
                        } else if (item_place == 'C') {
                            tmpinv.back().contents.push_back(tmpitem);
                        } else if (item_place == 'W') {
                            u.worn.push_back(tmpitem);
                        } else if (item_place == 'S') {
                            u.worn.back().contents.push_back(tmpitem);
                        } else if (item_place == 'w') {
                            u.weapon = tmpitem;
                        } else if (item_place == 'c') {
                            u.weapon.contents.push_back(tmpitem);
                        }
                    }
                }
            }
            // Now dump tmpinv into the player's inventory
            u.inv.add_stack(tmpinv);
            return true;
        ////////
        } break;

        case 0: {
         // todo: make savegame_legacy.cpp?
/*
original 'structure', which globs game/weather/location & killcount/player data onto the same lines.
*/
         int tmpturn, tmpspawn, tmprun, tmptar, comx, comy;
         fin >> tmpturn >> tmptar >> tmprun >> mostseen >> nextinv >> next_npc_id >>
             next_faction_id >> next_mission_id >> tmpspawn;

         load_weather(fin);

         fin >> levx >> levy >> levz >> comx >> comy;

         turn = tmpturn;
         nextspawn = tmpspawn;

         cur_om = &overmap_buffer.get(this, comx, comy);
         m.load(this, levx, levy, levz);

         run_mode = tmprun;
         if (OPTIONS["SAFEMODE"] && run_mode == 0)
          run_mode = 1;
         autosafemode = OPTIONS["AUTOSAFEMODE"];
         last_target = tmptar;

        // Next, the scent map.
         for (int i = 0; i < SEEX * MAPSIZE; i++) {
          for (int j = 0; j < SEEY * MAPSIZE; j++)
           fin >> grscent[i][j];
         }
        // Now the number of monsters...
         int nummon;
         fin >> nummon;
        // ... and the data on each one.
         std::string data;
         clear_zombies();
         monster montmp;
         char junk;
         int num_items;
         if (fin.peek() == '\n')
          fin.get(junk); // Chomp that pesky endline
         for (int i = 0; i < nummon; i++) {
          getline(fin, data);
          montmp.load_info(data, &mtypes);

          fin >> num_items;
          // Chomp the endline after number of items.
          getline( fin, data );
          for (int i = 0; i < num_items; i++) {
              getline( fin, data );
              montmp.inv.push_back( item( data, this ) );
          }

          add_zombie(montmp);
         }
        // And the kill counts;
         if (fin.peek() == '\n')
          fin.get(junk); // Chomp that pesky endline
         for (int i = 0; i < num_monsters; i++)
          fin >> kills[i];
        // Finally, the data on the player.
         if (fin.peek() == '\n')
          fin.get(junk); // Chomp that pesky endline
         getline(fin, data);
         u.load_info(this, data);
        // And the player's inventory...
         u.inv.load_invlet_cache( fin );

         char item_place;
         std::string itemdata;
        // We need a temporary vector of items.  Otherwise, when we encounter an item
        // which is contained in another item, the auto-sort/stacking behavior of the
        // player's inventory may cause the contained item to be misplaced.
         std::list<item> tmpinv;
         while (!fin.eof()) {
          fin >> item_place;
          if (!fin.eof()) {
           getline(fin, itemdata);
           if ( item_place == 'I' || item_place == 'C' || item_place == 'W' || item_place == 'S' || item_place == 'w' || item_place == 'c' ) {
               item tmpitem(itemdata, this);
               if (item_place == 'I') {
                   tmpinv.push_back(tmpitem);
               } else if (item_place == 'C') {
                   tmpinv.back().contents.push_back(tmpitem);
               } else if (item_place == 'W') {
                   u.worn.push_back(tmpitem);
               } else if (item_place == 'S') {
                   u.worn.back().contents.push_back(tmpitem);
               } else if (item_place == 'w') {
                   u.weapon = tmpitem;
               } else if (item_place == 'c') {
                   u.weapon.contents.push_back(tmpitem);
               }
           }
          }
         }
        // Now dump tmpinv into the player's inventory
         u.inv.add_stack(tmpinv);
         return true;
        ///////////////////////////////////// legacy save
        } break;
    }
    return false;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * Parse an open, obsolete overmap. These can linger unless the player moves around to all explored areas.
 */

bool overmap::unserialize_legacy(game *g, std::ifstream & fin, std::string const & plrfilename, std::string const & terfilename) {
   return false; // stub
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * Parse an open, obsolete maps.txt.
 */
bool mapbuffer::unserialize_legacy(std::ifstream & fin ) {
   return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// old stringstream based class loadgame functions. For saves from < 0.8 to git sep 20 '13

///// player.h
void player::load_legacy(game *g, std::stringstream & dump) {
 int inveh, vctrl;
 itype_id styletmp;
 std::string prof_ident;

 dump >> posx >> posy >> str_cur >> str_max >> dex_cur >> dex_max >>
         int_cur >> int_max >> per_cur >> per_max >> power_level >>
         max_power_level >> hunger >> thirst >> fatigue >> stim >>
         pain >> pkill >> radiation >> cash >> recoil >> driving_recoil >>
         inveh >> vctrl >> grab_point.x >> grab_point.y >> scent >> moves >>
         underwater >> dodges_left >> blocks_left >> oxygen >> active_mission >>
         focus_pool >> male >> prof_ident >> health >> styletmp;

 if (profession::exists(prof_ident)) {
  prof = profession::prof(prof_ident);
 } else {
  prof = profession::generic();
  debugmsg("Tried to use non-existent profession '%s'", prof_ident.c_str());
 }

 activity.load_legacy(dump);
 backlog.load_legacy(dump);

 in_vehicle = inveh != 0;
 controlling_vehicle = vctrl != 0;
 style_selected = styletmp;

 std::string sTemp = "";
 for (int i = 0; i < traits.size(); i++) {
    dump >> sTemp;
    if (sTemp == "TRAITS_END") {
        break;
    } else {
        my_traits.insert(sTemp);
    }
 }

 for (int i = 0; i < traits.size(); i++) {
    dump >> sTemp;
    if (sTemp == "MUTATIONS_END") {
        break;
    } else {
        my_mutations.insert(sTemp);
    }
 }

 set_highest_cat_level();
 drench_mut_calc();

 for (int i = 0; i < num_hp_parts; i++)
  dump >> hp_cur[i] >> hp_max[i];
 for (int i = 0; i < num_bp; i++)
  dump >> temp_cur[i] >> temp_conv[i] >> frostbite_timer[i];

 for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin(); aSkill != Skill::skills.end(); ++aSkill) {
   dump >> skillLevel(*aSkill);
 }

 int num_recipes;
 std::string rec_name;
 dump >> num_recipes;
 for (int i = 0; i < num_recipes; ++i)
 {
  dump >> rec_name;
  learned_recipes[rec_name] = recipe_by_name(rec_name);
 }

 int numstyles;
 itype_id styletype;
 dump >> numstyles;
 for (int i = 0; i < numstyles; i++) {
  dump >> styletype;
  ma_styles.push_back( styletype );
 }

 int numill;
 disease illtmp;
 int temp_bpart;
 dump >> numill;
 for (int i = 0; i < numill; i++) {
     dump >> illtmp.type >> illtmp.duration >> illtmp.intensity
          >> temp_bpart >> illtmp.side;
     illtmp.bp = (body_part)temp_bpart;
     illness.push_back(illtmp);
 }

 int numadd = 0;
 int typetmp;
 addiction addtmp;
 dump >> numadd;
 for (int i = 0; i < numadd; i++) {
  dump >> typetmp >> addtmp.intensity >> addtmp.sated;
  addtmp.type = add_type(typetmp);
  addictions.push_back(addtmp);
 }

 int numbio = 0;
 bionic_id biotype;
 bionic biotmp;
 dump >> numbio;
 for (int i = 0; i < numbio; i++) {
  dump >> biotype >> biotmp.invlet >> biotmp.powered >> biotmp.charge;
  biotmp.id = biotype;
  my_bionics.push_back(biotmp);
 }

 int nummor;
 morale_point mortmp;
 dump >> nummor;
 for (int i = 0; i < nummor; i++) {
  // Load morale properties in structure order.
  int mortype;
  std::string item_id;
  dump >> mortype >> item_id;
  mortmp.type = morale_type(mortype);
  if (g->itypes.find(item_id) == g->itypes.end())
   mortmp.item_type = NULL;
  else
   mortmp.item_type = g->itypes[item_id];

  dump >> mortmp.bonus >> mortmp.duration >> mortmp.decay_start
       >> mortmp.age;
  morale.push_back(mortmp);
 }

 int nummis = 0;
 int mistmp;
 dump >> nummis;
 for (int i = 0; i < nummis; i++) {
  dump >> mistmp;
  active_missions.push_back(mistmp);
 }
 dump >> nummis;
 for (int i = 0; i < nummis; i++) {
  dump >> mistmp;
  completed_missions.push_back(mistmp);
 }
 dump >> nummis;
 for (int i = 0; i < nummis; i++) {
  dump >> mistmp;
  failed_missions.push_back(mistmp);
 }

 stats & pstats = *lifetime_stats();

 dump >> pstats.squares_walked;

 recalc_sight_limits();
}

///// pldata.h
 void player_activity::load_legacy(std::stringstream &dump)
 {
  int tmp, tmptype, tmpinvlet;
  std::string tmpname;
  dump >> tmptype >> moves_left >> index >> tmpinvlet >> tmpname >> placement.x >> placement.y >> tmp;
  name = tmpname.substr(4);
  type = activity_type(tmptype);
  invlet = tmpinvlet;
  for (int i = 0; i < tmp; i++) {
   int tmp2;
   dump >> tmp2;
   values.push_back(tmp2);
  }
 }


///// npc.h
void npc::load_legacy(game *g, std::stringstream & dump) {
    std::string tmpname;
    int deathtmp, deadtmp, classtmp, npc_id;
 dump >> npc_id;
 setID(npc_id);
// Standard player stuff
 do {
  dump >> tmpname;
  if (tmpname != "||")
   name += tmpname + " ";
 } while (tmpname != "||");
 name = name.substr(0, name.size() - 1); // Strip off trailing " "
 dump >> posx >> posy >> str_cur >> str_max >> dex_cur >> dex_max >>
         int_cur >> int_max >> per_cur >> per_max >> hunger >> thirst >>
         fatigue >> stim >> pain >> pkill >> radiation >> cash >> recoil >>
         scent >> moves >> underwater >> dodges_left >> oxygen >> deathtmp >>
         deadtmp >> classtmp >> patience;

 if (deathtmp == 1)
  marked_for_death = true;
 else
  marked_for_death = false;

 if (deadtmp == 1)
  dead = true;
 else
  dead = false;

 myclass = npc_class(classtmp);

 std::string sTemp = "";
 for (int i = 0; i < traits.size(); i++) {
    dump >> sTemp;
    if (sTemp == "TRAITS_END") {
        break;
    } else {
        my_traits.insert(sTemp);
    }
 }

 for (int i = 0; i < num_hp_parts; i++)
  dump >> hp_cur[i] >> hp_max[i];
 for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin(); aSkill != Skill::skills.end(); ++aSkill) {
   dump >> skillLevel(*aSkill);
 }

 itype_id tmpstyle;
 int numstyle;
 dump >> numstyle;
 for (int i = 0; i < numstyle; i++) {
  dump >> tmpstyle;
  ma_styles.push_back(tmpstyle);
 }

 int typetmp;
 std::string disease_type_tmp;
 int numill;
 dump >> numill;
 disease illtmp;
 for (int i = 0; i < numill; i++) {
  dump >> disease_type_tmp >> illtmp.duration;
  illtmp.type = disease_type_tmp;
  illness.push_back(illtmp);
 }
 int numadd;
 addiction addtmp;
 dump >> numadd;
 for (int i = 0; i < numadd; i++) {
  dump >> typetmp >> addtmp.intensity >> addtmp.sated;
  addtmp.type = add_type(typetmp);
  addictions.push_back(addtmp);
 }
 bionic_id tmpbionic;
 int numbio;
 bionic biotmp;
 dump >> numbio;
 for (int i = 0; i < numbio; i++) {
  dump >> tmpbionic >> biotmp.invlet >> biotmp.powered >> biotmp.charge;
  biotmp.id = bionic_id(tmpbionic);
  my_bionics.push_back(biotmp);
 }
// Special NPC stuff
 int misstmp, flagstmp, tmpatt, agg, bra, col, alt;
 dump >> agg >> bra >> col >> alt >> wandx >> wandy >> wandf >> omx >> omy >>
         omz >> mapx >> mapy >> plx >> ply >> goalx >> goaly >> goalz >> misstmp >>
         flagstmp >> fac_id >> tmpatt;
 personality.aggression = agg;
 personality.bravery = bra;
 personality.collector = col;
 personality.altruism = alt;
 mission = npc_mission(misstmp);
 flags = flagstmp;
 attitude = npc_attitude(tmpatt);

 op_of_u.load_legacy(dump);
 chatbin.load_legacy(dump);
 combat_rules.load_legacy(dump);
}

 void npc_opinion::load_legacy(std::stringstream &info)
 {
  int tmpsize;
  info >> trust >> fear >> value >> anger >> owed >> tmpsize;
  for (int i = 0; i < tmpsize; i++) {
   int tmptype, tmpskill;
   std::string tmpitem;
   npc_favor tmpfavor;
   info >> tmptype >> tmpfavor.value >> tmpitem >> tmpskill;
   tmpfavor.type = npc_favor_type(tmptype);
   tmpfavor.item_id = tmpitem;
   tmpfavor.skill = Skill::skill(tmpskill);
   favors.push_back(tmpfavor);
  }
 }

 void npc_combat_rules::load_legacy(std::istream &data)
 {
  int tmpen;
  data >> tmpen >> use_guns >> use_grenades >> use_silent;
  engagement = combat_engagement(tmpen);
 }

 void npc_chatbin::load_legacy(std::stringstream &info)
 {
  int tmpsize_miss, tmpsize_assigned, tmptopic;
  std::string skill_ident;
  info >> tmptopic >> mission_selected >> tempvalue >> skill_ident >>
          tmpsize_miss >> tmpsize_assigned;
  first_topic = talk_topic(tmptopic);
  skill = skill_ident == "none" ? NULL : Skill::skill(skill_ident);
  for (int i = 0; i < tmpsize_miss; i++) {
   int tmpmiss;
   info >> tmpmiss;
   missions.push_back(tmpmiss);
  }
  for (int i = 0; i < tmpsize_assigned; i++) {
   int tmpmiss;
   info >> tmpmiss;
   missions_assigned.push_back(tmpmiss);
  }
 }

///// inventory.h
void inventory::load_invlet_cache( std::ifstream &fin ) {
    // Lines are of the format "P itemname abcde".
    while( fin.peek() == 'P' ) {
        std::string invlet_cache_line;
        getline( fin, invlet_cache_line );
        int first_sym = invlet_cache_line.find_first_of(' ', 2);
        std::string item_type( invlet_cache_line, 2, first_sym - 2 );
        std::vector<char> symbol_vec( invlet_cache_line.begin() + first_sym + 1,
                                      invlet_cache_line.end() );
        invlet_cache[ item_type ] = symbol_vec;
    }
}

///// skill.h
std::istream& operator>>(std::istream& is, SkillLevel& obj) {
  int level; int exercise; bool isTraining; int lastPracticed;

  is >> level >> exercise >> isTraining >> lastPracticed;

  obj = SkillLevel(level, exercise, isTraining, lastPracticed);

  return is;
}

///// monster.h
void monster::load_legacy(std::vector <mtype *> *mtypes, std::stringstream & dump) {
    int idtmp, plansize;
    dump >> idtmp >> _posx >> _posy >> wandx >> wandy >> wandf >> moves >> speed >>
         hp >> sp_timeout >> plansize >> friendly >> faction_id >> mission_id >>
         no_extra_death_drops >> dead >> anger >> morale;
    type = (*mtypes)[idtmp];
    point ptmp;
    plans.clear();
    for (int i = 0; i < plansize; i++) {
        dump >> ptmp.x >> ptmp.y;
        plans.push_back(ptmp);
    }
}
///// item.h

bool itag2ivar( std::string &item_tag, std::map<std::string, std::string> &item_vars );

void item::load_legacy(game * g, std::stringstream & dump) {
    clear();
    std::string idtmp, ammotmp, item_tag;
    int lettmp, damtmp, acttmp, corp, tag_count;
    dump >> lettmp >> idtmp >> charges >> damtmp >> tag_count;
    for( int i = 0; i < tag_count; ++i )
    {
        dump >> item_tag;
        if( itag2ivar(item_tag, item_vars ) == false ) {
            item_tags.insert( item_tag );
        }
    }

    dump >> burnt >> poison >> ammotmp >> owned >> bday >>
         mode >> acttmp >> corp >> mission_id >> player_id;
    if (corp != -1)
        corpse = g->mtypes[corp];
    else
        corpse = NULL;
    getline(dump, name);
    if (name == " ''")
        name = "";
    else {
        size_t pos = name.find_first_of("@@");
        while (pos != std::string::npos)  {
            name.replace(pos, 2, "\n");
            pos = name.find_first_of("@@");
        }
        name = name.substr(2, name.size() - 3); // s/^ '(.*)'$/\1/
    }
    make(g->itypes[idtmp]);
    invlet = char(lettmp);
    damage = damtmp;
    active = false;
    if (acttmp == 1)
        active = true;
    if (ammotmp != "null")
        curammo = dynamic_cast<it_ammo*>(g->itypes[ammotmp]);
    else
        curammo = NULL;
}

///// vehicle.h

void vehicle::load_legacy(std::ifstream &stin) {
    int fdir, mdir, skd, prts, cr_on, li_on, tag_count;
    std::string vehicle_tag;
    stin >>
        posx >>
        posy >>
        fdir >>
        mdir >>
        turn_dir >>
        velocity >>
        cruise_velocity >>
        cr_on >>
        li_on >>
        turret_mode >>
        skd >>
        of_turn_carry >>
        prts;
    face.init (fdir);
    move.init (mdir);
    skidding = skd != 0;
    cruise_on = cr_on != 0;
    lights_on = li_on != 0;
    std::string databuff;
    getline(stin, databuff); // Clear EoL
    getline(stin, name); // read name
    int itms = 0;
    for (int p = 0; p < prts; p++)
    {
        int pid, pdx, pdy, php, pam, pbld, pbig, pflag, pass, pnit;
        stin >> pid >> pdx >> pdy >> php >> pam >> pbld >> pbig >> pflag >> pass >> pnit;
        getline(stin, databuff); // Clear EoL
        vehicle_part new_part;
        new_part.id = legacy_vpart_id[ pid ];
        new_part.mount_dx = pdx;
        new_part.mount_dy = pdy;
        new_part.hp = php;
        new_part.blood = pbld;
        new_part.bigness = pbig;
        new_part.flags = pflag;
        new_part.passenger_id = pass;
        new_part.amount = pam;
        for (int j = 0; j < pnit; j++)
        {
            itms++;
            getline(stin, databuff);
            item itm;
            itm.load_info (databuff, g);
            new_part.items.push_back (itm);
            int ncont;
            stin >> ncont; // how many items inside container
            getline(stin, databuff); // Clear EoL
            for (int k = 0; k < ncont; k++)
            {
                getline(stin, databuff);
                item citm;
                citm.load_info (databuff, g);
                new_part.items[new_part.items.size()-1].put_in (citm);
            }
        }
        parts.push_back (new_part);
    }
    find_external_parts ();
    find_exhaust ();
    insides_dirty = true;
    precalc_mounts (0, face.dir());

    stin >> tag_count;
    for( int i = 0; i < tag_count; ++i )
    {
        stin >> vehicle_tag;
        tags.insert( vehicle_tag );
    }
    getline(stin, databuff); // Clear EoL
}
