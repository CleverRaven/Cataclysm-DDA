#include "game.h"
#include "rng.h"
#include "input.h"
#include "keypress.h"
#include "output.h"
#include "skill.h"
#include "line.h"
#include "computer.h"
#include "veh_interact.h"
#include "options.h"
#include "auto_pickup.h"
#include "mapbuffer.h"
#include "debug.h"
#include "editmap.h"
#include "bodypart.h"
#include "map.h"
#include "output.h"
#include "uistate.h"
#include "item_factory.h"
#include "helper.h"
#include "text_snippets.h"
#include "catajson.h"
#include "artifact.h"
#include "overmapbuffer.h"
#include "trap.h"
#include "mapdata.h"
#include "catacharset.h"
#include "translations.h"
#include <map>
#include <set>
#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>
#include <math.h>
#include <vector>
#ifndef _MSC_VER
#include <unistd.h>
#include <dirent.h>
#endif
#include <sys/stat.h>
#include "debug.h"
#include "artifactdata.h"

#if (defined _WIN32 || defined __WIN32__)
#include <windows.h>
#include <tchar.h>
#endif

#ifdef _MSC_VER
// MSVC doesn't have c99-compatible "snprintf", so do what picojson does and use _snprintf_s instead
#define snprintf _snprintf_s
#endif

#define dbg(x) dout((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "
void intro();
nc_color sev(int a);	// Right now, ONLY used for scent debugging....

//The one and only game instance
game *g;

uistatedata uistate;

// This is the main game set-up process.
game::game() :
 w_terrain(NULL),
 w_minimap(NULL),
 w_HP(NULL),
 w_messages(NULL),
 w_location(NULL),
 w_status(NULL),
 w_status2(NULL),
 om_hori(NULL),
 om_vert(NULL),
 om_diag(NULL),
 gamemode(NULL)
{
 dout() << "Game initialized.";

 try {
 if(!json_good())
  throw (std::string)"Failed to initialize a static variable";
 // Gee, it sure is init-y around here!
 init_npctalk();
 init_materials();
 init_artifacts();
 init_weather();
 init_overmap();
 init_fields();
 init_faction_data();
 init_morale();
 init_skills();
 init_professions();
 init_bionics();              // Set up bionics                   (SEE bionics.cpp)
 init_mtypes();	              // Set up monster types             (SEE mtypedef.cpp)
 init_itypes();	              // Set up item types                (SEE itypedef.cpp)
 SNIPPET.load();
 item_controller->init(this); //Item manager
 init_monitems();             // Set up the items monsters carry  (SEE monitemsdef.cpp)
 init_traps();                // Set up the trap types            (SEE trapdef.cpp)
 init_recipes();              // Set up crafting reciptes         (SEE crafting.cpp)
 init_mongroups();            // Set up monster groupings         (SEE mongroupdef.cpp)
 init_missions();             // Set up mission templates         (SEE missiondef.cpp)
 init_construction();         // Set up constructables            (SEE construction.cpp)
 init_traits_mutations();
 init_vehicles();             // Set up vehicles                  (SEE veh_typedef.cpp)
 init_autosave();             // Set up autosave
 init_diseases();             // Set up disease lookup table
 init_dreams();				  // Set up dreams					  (SEE mutation_data.cpp)
 } catch(std::string &error_message)
 {
     uquit = QUIT_ERROR;
     if(!error_message.empty())
        debugmsg(error_message.c_str());
     return;
 }
 load_keyboard_settings();
 moveCount = 0;

 gamemode = new special_game;	// Nothing, basically.
}

game::~game()
{
 delete gamemode;
 itypes.clear();
 for (int i = 0; i < mtypes.size(); i++)
  delete mtypes[i];
 delwin(w_terrain);
 delwin(w_minimap);
 delwin(w_HP);
 delwin(w_messages);
 delwin(w_location);
 delwin(w_status);
 delwin(w_status2);
}

void game::init_skills() throw (std::string)
{
    try
    {
    Skill::skills = Skill::loadSkills();
    }
    catch (std::string &error_message)
    {
        throw;
    }
}

// Fixed window sizes
#define MINIMAP_HEIGHT 7
#define MINIMAP_WIDTH 7

void game::init_ui(){
    clear();	// Clear the screen
    intro();	// Print an intro screen, make sure we're at least 80x25

    const int sidebarWidth = (OPTIONS["SIDEBAR_STYLE"] == "Narrow") ? 45 : 55;

    #if (defined TILES || defined _WIN32 || defined __WIN32__)
        TERMX = sidebarWidth + (OPTIONS["VIEWPORT_X"] * 2 + 1);
        TERMY = OPTIONS["VIEWPORT_Y"] * 2 + 1;
        VIEWX = (OPTIONS["VIEWPORT_X"] > 60) ? 60 : OPTIONS["VIEWPORT_X"];
        VIEWY = (OPTIONS["VIEWPORT_Y"] > 60) ? 60 : OPTIONS["VIEWPORT_Y"];

        // If we've chosen the narrow sidebar, we might need to make the
        // viewport wider to fill an 80-column window.
        while (TERMX < FULL_SCREEN_WIDTH) {
            TERMX += 2;
            VIEWX += 1;
        }

        VIEW_OFFSET_X = (OPTIONS["VIEWPORT_X"] > 60) ? OPTIONS["VIEWPORT_X"]-60 : 0;
        VIEW_OFFSET_Y = (OPTIONS["VIEWPORT_Y"] > 60) ? OPTIONS["VIEWPORT_Y"]-60 : 0;
        TERRAIN_WINDOW_WIDTH  = (VIEWX * 2) + 1;
        TERRAIN_WINDOW_HEIGHT = (VIEWY * 2) + 1;
    #else
        getmaxyx(stdscr, TERMY, TERMX);

        //make sure TERRAIN_WINDOW_WIDTH and TERRAIN_WINDOW_HEIGHT are uneven
        if (TERMX%2 == 1) {
            TERMX--;
        }

        if (TERMY%2 == 0) {
            TERMY--;
        }

        TERRAIN_WINDOW_WIDTH = (TERMX - sidebarWidth > 121) ? 121 : TERMX - sidebarWidth;
        TERRAIN_WINDOW_HEIGHT = (TERMY > 121) ? 121 : TERMY;

        VIEW_OFFSET_X = (TERMX - sidebarWidth > 121) ? (TERMX - sidebarWidth - 121)/2 : 0;
        VIEW_OFFSET_Y = (TERMY > 121) ? (TERMY - 121)/2 : 0;

        VIEWX = (TERRAIN_WINDOW_WIDTH - 1) / 2;
        VIEWY = (TERRAIN_WINDOW_HEIGHT - 1) / 2;
    #endif

    if (VIEWX < 12) {
        VIEWX = 12;
    }

    if (VIEWY < 12) {
        VIEWY = 12;
    }

    // Set up the main UI windows.
    w_terrain = newwin(TERRAIN_WINDOW_HEIGHT, TERRAIN_WINDOW_WIDTH, VIEW_OFFSET_Y, VIEW_OFFSET_X);
    werase(w_terrain);

    int minimapX, minimapY; // always MINIMAP_WIDTH x MINIMAP_HEIGHT in size
    int hpX, hpY, hpW, hpH;
    int messX, messY, messW, messH;
    int locX, locY, locW, locH;
    int statX, statY, statW, statH;
    int stat2X, stat2Y, stat2W, stat2H;

    switch ((int)(OPTIONS["SIDEBAR_STYLE"] == "Narrow")) {
        case 0: // standard
            minimapX = 0;
            minimapY = 0;
            messX = MINIMAP_WIDTH;
            messY = 0;
            messW = sidebarWidth - messX;
            messH = 20;
            hpX = 0;
            hpY = MINIMAP_HEIGHT;
            hpH = 14;
            hpW = 7;
            locX = MINIMAP_WIDTH;
            locY = messY + messH;
            locH = 1;
            locW = sidebarWidth - locX;
            statX = 0;
            statY = locY + locH;
            statH = 4;
            statW = sidebarWidth;

            // The default style only uses one status window.
            // The second status window needs to consume the void at the bottom
            // of the sidebar.
            stat2X = 0;
            stat2Y = statY + statH;
            stat2H = TERMY - stat2Y;
            stat2W = sidebarWidth;

            break;


        case 1: // narrow

            // First, figure out how large each element will be.

            hpH         = 7;
            hpW         = 14;

            statH       = 7;
            statW       = sidebarWidth - MINIMAP_WIDTH - hpW;

            locH        = 1;
            locW        = sidebarWidth;

            stat2H      = 2;
            stat2W      = sidebarWidth;

            messH       = TERMY - (statH + locH + stat2H);
            messW       = sidebarWidth;


            // Now position the elements relative to each other.

            minimapX = 0;
            minimapY = 0;

            hpX = minimapX + MINIMAP_WIDTH;
            hpY = 0;

            locX = 0;
            locY = minimapY + MINIMAP_HEIGHT;

            statX = hpX + hpW;
            statY = 0;

            stat2X = 0;
            stat2Y = locY + locH;

            messX = 0;
            messY = stat2Y + stat2H;

            break;
    }

    int _y = VIEW_OFFSET_Y;
    int _x = TERMX - VIEW_OFFSET_X - sidebarWidth;

    w_minimap = newwin(MINIMAP_HEIGHT, MINIMAP_WIDTH, _y + minimapY, _x + minimapX);
    werase(w_minimap);

    w_HP = newwin(hpH, hpW, _y + hpY, _x + hpX);
    werase(w_HP);

    w_messages = newwin(messH, messW, _y + messY, _x + messX);
    werase(w_messages);

    w_location = newwin(locH, locW, _y + locY, _x + locX);
    werase(w_location);

    w_status = newwin(statH, statW, _y + statY, _x + statX);
    werase(w_status);

    w_status2 = newwin(stat2H, stat2W, _y + stat2Y, _x + stat2X);
    werase(w_status2);
}

void game::setup()
{
 u = player();
 m = map(&traps); // Init the root map with our vectors
 z.reserve(1000); // Reserve some space

// Even though we may already have 'd', nextinv will be incremented as needed
 nextinv = 'd';
 next_npc_id = 1;
 next_faction_id = 1;
 next_mission_id = 1;
// Clear monstair values
 monstairx = -1;
 monstairy = -1;
 monstairz = -1;
 last_target = -1;	// We haven't targeted any monsters yet
 curmes = 0;		// We haven't read any messages yet
 uquit = QUIT_NO;	// We haven't quit the game
 debugmon = false;	// We're not printing debug messages

 weather = WEATHER_CLEAR; // Start with some nice weather...
 // Weather shift in 30
 nextweather = HOURS(OPTIONS["INITIAL_TIME"]) + MINUTES(30);

 turnssincelastmon = 0; //Auto safe mode init
 autosafemode = OPTIONS["AUTOSAFEMODE"];

 footsteps.clear();
 footsteps_source.clear();
 z.clear();
 coming_to_stairs.clear();
 active_npc.clear();
 factions.clear();
 active_missions.clear();
 items_dragged.clear();
 messages.clear();
 events.clear();

 turn.set_season(SUMMER);    // ... with winter conveniently a long ways off

 for (int i = 0; i < num_monsters; i++)	// Reset kill counts to 0
  kills[i] = 0;
// Set the scent map to 0
 for (int i = 0; i < SEEX * MAPSIZE; i++) {
  for (int j = 0; j < SEEX * MAPSIZE; j++)
   grscent[i][j] = 0;
 }

 load_auto_pickup(false); // Load global auto pickup rules

 if (opening_screen()) {// Opening menu
// Finally, draw the screen!
  refresh_all();
  draw();
 }
}

// Set up all default values for a new game
void game::start_game()
{
 turn = HOURS(OPTIONS["INITIAL_TIME"]);
 run_mode = (OPTIONS["SAFEMODE"] ? 1 : 0);
 mostseen = 0;	// ...and mostseen is 0, we haven't seen any monsters yet.

 popup_nowait(_("Please wait as we build your world"));
// Init some factions.
 if (!load_master())	// Master data record contains factions.
  create_factions();
 cur_om = &overmap_buffer.get(this, 0, 0);	// We start in the (0,0,0) overmap.

// Find a random house on the map, and set us there.
 cur_om->first_house(levx, levy);
 levx -= int(int(MAPSIZE / 2) / 2);
 levy -= int(int(MAPSIZE / 2) / 2);
 levz = 0;
// Start the overmap with out immediate neighborhood visible
 for (int i = -15; i <= 15; i++) {
  for (int j = -15; j <= 15; j++)
   cur_om->seen(levx + i, levy + j, 0) = true;
 }
// Convert the overmap coordinates to submap coordinates
 levx = levx * 2 - 1;
 levy = levy * 2 - 1;
 set_adjacent_overmaps(true);
// Init the starting map at this location.
 m.load(this, levx, levy, levz);
// Start us off somewhere in the shelter.
 u.posx = SEEX * int(MAPSIZE / 2) + 5;
 u.posy = SEEY * int(MAPSIZE / 2) + 6;
 u.str_cur = u.str_max;
 u.per_cur = u.per_max;
 u.int_cur = u.int_max;
 u.dex_cur = u.dex_max;
 nextspawn = int(turn);
 temperature = 65; // Springtime-appropriate?
 u.next_climate_control_check=0;  // Force recheck at startup
 u.last_climate_control_ret=false;

 //Reset character pickup rules
 vAutoPickupRules[2].clear();
 //Load NPCs. Set nearby npcs to active.
 load_npcs();
 //spawn the monsters
 m.spawn_monsters(this);	// Static monsters
 //Put some NPCs in there!
 create_starting_npcs();

 //Create mutation_category_level
 u.set_highest_cat_level();

 MAPBUFFER.set_dirty();

 u.add_memorial_log(_("%s began their journey into the Cataclysm."), u.name.c_str());
}

void game::create_factions()
{
 int num = dice(4, 3);
 faction tmp(0);
 tmp.make_army();
 factions.push_back(tmp);
 for (int i = 0; i < num; i++) {
  tmp = faction(assign_faction_id());
  tmp.randomize();
  tmp.likes_u = 100;
  tmp.respects_u = 100;
  tmp.known_by_u = true;
  factions.push_back(tmp);
 }
}

//Make any nearby overmap npcs active, and put them in the right location.
void game::load_npcs()
{
    for (int i = 0; i < cur_om->npcs.size(); i++)
    {

        if (square_dist(levx + int(MAPSIZE / 2), levy + int(MAPSIZE / 2),
              cur_om->npcs[i]->mapx, cur_om->npcs[i]->mapy) <=
              int(MAPSIZE / 2) + 1 && !cur_om->npcs[i]->is_active(this))
        {
            int dx = cur_om->npcs[i]->mapx - levx, dy = cur_om->npcs[i]->mapy - levy;
            if (debugmon)debugmsg("game::load_npcs: Spawning static NPC, %d:%d (%d:%d)", levx, levy, cur_om->npcs[i]->mapx, cur_om->npcs[i]->mapy);

            npc * temp = cur_om->npcs[i];

            if (temp->posx == -1 || temp->posy == -1)
            {
                dbg(D_ERROR) << "game::load_npcs: Static NPC with no fine location "
                    "data (" << temp->posx << ":" << temp->posy << ").";
                debugmsg("game::load_npcs Static NPC with no fine location data (%d:%d) New loc data (%d:%d).",
                         temp->posx, temp->posy, SEEX * 2 * (temp->mapx - levx) + rng(0 - SEEX, SEEX),
                         SEEY * 2 * (temp->mapy - levy) + rng(0 - SEEY, SEEY));
                temp->posx = SEEX * 2 * (temp->mapx - levx) + rng(0 - SEEX, SEEX);
                temp->posy = SEEY * 2 * (temp->mapy - levy) + rng(0 - SEEY, SEEY);
            } else {
                if (debugmon) debugmsg("game::load_npcs Static NPC fine location %d:%d (%d:%d)", temp->posx, temp->posy, temp->posx + dx * SEEX, temp->posy + dy * SEEY);
                temp->posx += dx * SEEX;
                temp->posy += dy * SEEY;
            }

        //check if the loaded position doesn't already contain an object, monster or npc.
        //If it isn't free, spiralsearch for a free spot.
        temp->place_near(this, temp->posx, temp->posy);

        //In the rare case the npc was marked for death while it was on the overmap. Kill it.
        if (temp->marked_for_death)
            temp->die(this, false);
        else
            active_npc.push_back(temp);
        }
    }
}

void game::create_starting_npcs()
{
 if(!OPTIONS["STATIC_NPC"])
 	return; //Do not generate a starting npc.
 npc * tmp = new npc();
 tmp->normalize(this);
 tmp->randomize(this, (one_in(2) ? NC_DOCTOR : NC_NONE));
 tmp->spawn_at(cur_om, levx, levy, levz); //spawn the npc in the overmap.
 tmp->place_near(this, SEEX * int(MAPSIZE / 2) + SEEX, SEEY * int(MAPSIZE / 2) + 6);
 tmp->form_opinion(&u);
 tmp->attitude = NPCATT_NULL;
 tmp->mission = NPC_MISSION_SHELTER; //This sets the npc mission. This NPC remains in the shelter.
 tmp->chatbin.first_topic = TALK_SHELTER;
 tmp->chatbin.missions.push_back(
     reserve_random_mission(ORIGIN_OPENER_NPC, om_location(), tmp->getID()) ); //one random shelter mission/

 active_npc.push_back(tmp);
}

void game::cleanup_at_end(){
    write_msg();
    if (uquit == QUIT_DIED || uquit == QUIT_SUICIDE || uquit == QUIT_SAVED)
	{
		// Save the factions's, missions and set the NPC's overmap coords
		// Npcs are saved in the overmap.
		save_factions_missions_npcs(); //missions need to be saved as they are global for all saves.

		// save artifacts.
		save_artifacts();

		// and the overmap, and the local map.
		save_maps(); //Omap also contains the npcs who need to be saved.
	}

    // Clear the future weather for future projects
    future_weather.clear();

    if (uquit == QUIT_DIED)
    {
        popup_top(_("Game over! Press spacebar..."));
    }
    if (uquit == QUIT_DIED || uquit == QUIT_SUICIDE)
    {
        death_screen();
        u.add_memorial_log("%s %s", u.name.c_str(),
                uquit == QUIT_SUICIDE ? _("committed suicide.") : _("was killed."));
        write_memorial_file();
        if (OPTIONS["DELETE_WORLD"] == "Yes" ||
            (OPTIONS["DELETE_WORLD"] == "Query" && query_yn(_("Delete saved world?"))))
        {
            delete_save();
            MAPBUFFER.reset();
            MAPBUFFER.make_volatile();
        }
        if(gamemode)
        {
            delete gamemode;
            gamemode = new special_game;	// null gamemode or something..
        }
    }
    overmap_buffer.clear();
}

// MAIN GAME LOOP
// Returns true if game is over (death, saved, quit, etc)
bool game::do_turn()
{
 if (is_game_over()) {
  cleanup_at_end();
  return true;
 }
// Actual stuff
 gamemode->per_turn(this);
 turn.increment();
 process_events();
 process_missions();
 if (turn.hours() == 0 && turn.minutes() == 0 && turn.seconds() == 0) // Midnight!
  cur_om->process_mongroups();

// Check if we've overdosed... in any deadly way.
 if (u.stim > 250) {
  add_msg(_("You have a sudden heart attack!"));
  u.add_memorial_log(_("Died of a drug overdose."));
  u.hp_cur[hp_torso] = 0;
 } else if (u.stim < -200 || u.pkill > 240) {
  add_msg(_("Your breathing stops completely."));
  u.add_memorial_log(_("Died of a drug overdose."));
  u.hp_cur[hp_torso] = 0;
 }
// Check if we're starving or have starved
    if (u.hunger > 2999) {
     switch (u.hunger) {
         case 3000: if (turn % 10 == 0)
          add_msg(_("You haven't eaten in over a week!")); break;
         case 4000: if (turn % 10 == 0)
          add_msg(_("You are STARVING!")); break;
         case 5000: if (turn % 10 == 0)
          add_msg(_("Food...")); break;
         case 6000:
          add_msg(_("You have starved to death."));
          u.add_memorial_log(_("Died of starvation."));
          u.hp_cur[hp_torso] = 0;
          break;
     }
    }
// Check if we're dying of thirst
    if (u.thirst > 599) {
     switch (u.thirst) {
         case  600: if (turn % 10 == 0)
          add_msg(_("You haven't had anything to drink in 2 days!")); break;
         case  800: if (turn % 10 == 0)
          add_msg(_("You are THIRSTY!")); break;
         case 1000: if (turn % 10 == 0)
          add_msg(_("4 days... no water..")); break;
         case 1200:
          add_msg(_("You have died of dehydration."));
          u.add_memorial_log(_("Died of thirst."));
          u.hp_cur[hp_torso] = 0;
          break;
     }
    }
// Check if we're falling asleep
    if (u.fatigue > 599) {
     switch (u.fatigue) {
         case  600: if (turn % 10 == 0)
          add_msg(_("You haven't slept in 2 days!")); break;
         case  800: if (turn % 10 == 0)
          add_msg(_("Anywhere would be a good place to sleep...")); break;
         case 1000:
          add_msg(_("Surivor sleep now."));
          u.fatigue -= 10;
          u.try_to_sleep(this);
          break;
     }
    }

 if (turn % 50 == 0) {	// Hunger, thirst, & fatigue up every 5 minutes
  if ((!u.has_trait("LIGHTEATER") || !one_in(3)) &&
      (!u.has_bionic("bio_recycler") || turn % 300 == 0))
   u.hunger++;
  if ((!u.has_bionic("bio_recycler") || turn % 100 == 0) &&
      (!u.has_trait("PLANTSKIN") || !one_in(5)))
   u.thirst++;
  u.fatigue++;
  if (u.fatigue == 192 && !u.has_disease("lying_down") &&
      !u.has_disease("sleep")) {
   if (u.activity.type == ACT_NULL)
     add_msg(_("You're feeling tired.  %s to lie down for sleep."),
             press_x(ACTION_SLEEP).c_str());
   else
    cancel_activity_query(_("You're feeling tired."));
  }
  if (u.stim < 0)
   u.stim++;
  if (u.stim > 0)
   u.stim--;
  if (u.pkill > 0)
   u.pkill--;
  if (u.pkill < 0)
   u.pkill++;
  if (u.has_bionic("bio_solar") && is_in_sunlight(u.posx, u.posy))
   u.charge_power(1);
 }
 if (turn % 300 == 0) {	// Pain up/down every 30 minutes
  if (u.pain > 0)
   u.pain -= 1 + int(u.pain / 10);
  else if (u.pain < 0)
   u.pain++;
// Mutation healing effects
  if (u.has_trait("FASTHEALER2") && one_in(5))
   u.healall(1);
  if (u.has_trait("REGEN") && one_in(2))
   u.healall(1);
  if (u.has_trait("ROT2") && one_in(5))
   u.hurtall(1);
  if (u.has_trait("ROT3") && one_in(2))
   u.hurtall(1);

  if (u.radiation > 1 && one_in(3))
   u.radiation--;
  u.get_sick(this);
 }

// Auto-save if autosave is enabled
 if (OPTIONS["AUTOSAVE"] &&
     turn % ((int)OPTIONS["AUTOSAVE_TURNS"] * 10) == 0)
     autosave();

 update_weather();

// The following happens when we stay still; 10/40 minutes overdue for spawn
 if ((!u.has_trait("INCONSPICUOUS") && turn > nextspawn +  100) ||
     ( u.has_trait("INCONSPICUOUS") && turn > nextspawn +  400)   ) {
  spawn_mon(-1 + 2 * rng(0, 1), -1 + 2 * rng(0, 1));
  nextspawn = turn;
 }

 process_activity();
 if(u.moves > 0) {
     while (u.moves > 0) {
          cleanup_dead();
          if (!u.has_disease("sleep") && u.activity.type == ACT_NULL)
              draw();

          if(handle_action()) {
              ++moves_since_last_save;
              u.action_taken();
          }

          if (is_game_over()) {
              cleanup_at_end();
              return true;
          }
     }
 } else {
     handle_key_blocking_activity();
 }
 update_scent();
 m.vehmove(this);
 m.process_fields(this);
 m.process_active_items(this);
 m.step_in_field(u.posx, u.posy, this);

 monmove();
 update_stair_monsters();
 u.reset(this);
 u.process_active_items(this);
 u.suffer(this);

 if (levz >= 0) {
  weather_effect weffect;
  (weffect.*(weather_data[weather].effect))(this);
 }

 if (u.has_disease("sleep") && int(turn) % 300 == 0) {
  draw();
  refresh();
 }

 u.update_bodytemp(this);

 rustCheck();
 if (turn % 10 == 0)
  u.update_morale();
 return false;
}

void game::rustCheck()
{
    for (std::vector<Skill*>::iterator aSkill = ++Skill::skills.begin();
         aSkill != Skill::skills.end(); ++aSkill) {
        if (u.rust_rate() <= rng(0, 1000)) continue;
        bool charged_bio_mem = u.has_bionic("bio_memory") && u.power_level > 0;
        int oldSkillLevel = u.skillLevel(*aSkill);

        if (u.skillLevel(*aSkill).rust(turn, charged_bio_mem))
        {
            u.power_level--;
        }
        int newSkill =u.skillLevel(*aSkill);
        if (newSkill < oldSkillLevel)
        {
            add_msg(_("Your skill in %s has reduced to %d!"),
                    (*aSkill)->name().c_str(), newSkill);
        }
    }
}

void game::process_events()
{
 for (int i = 0; i < events.size(); i++) {
  events[i].per_turn(this);
  if (events[i].turn <= int(turn)) {
   events[i].actualize(this);
   events.erase(events.begin() + i);
   i--;
  }
 }
}

void game::process_activity()
{
 it_book* reading;
 bool no_recipes;
 if (u.activity.type != ACT_NULL) {
  if (int(turn) % 150 == 0) {
   draw();
  }
  if (u.activity.type == ACT_WAIT) {	// Based on time, not speed
   u.activity.moves_left -= 100;
   u.pause(this);
  } else if (u.activity.type == ACT_GAME) {

    //Gaming takes time, not speed
    u.activity.moves_left -= 100;

    if (u.activity.type == ACT_GAME) {
      item& game_item = u.inv.item_by_letter(u.activity.invlet);

      //Deduct 1 battery charge for every minute spent playing
      if(int(turn) % 10 == 0) {
        game_item.charges--;
        u.add_morale(MORALE_GAME, 2, 100); //2 points/min, almost an hour to fill
      }
      if(game_item.charges == 0) {
        u.activity.moves_left = 0;
        g->add_msg(_("The %s runs out of batteries."), game_item.name.c_str());
      }

    }
    u.pause(this);

  } else if (u.activity.type == ACT_REFILL_VEHICLE) {
   vehicle *veh = m.veh_at( u.activity.placement.x, u.activity.placement.y );
   if (!veh) {  // Vehicle must've moved or something!
    u.activity.moves_left = 0;
    return;
   }
   for(int i = -1; i <= 1; i++) {
    for(int j = -1; j <= 1; j++) {
     if(m.ter(u.posx + i, u.posy + j) == t_gas_pump) {
      for (int n = 0; n < m.i_at(u.posx + i, u.posy + j).size(); n++) {
       if (m.i_at(u.posx + i, u.posy + j)[n].type->id == "gasoline") {
        item* gas = &(m.i_at(u.posx + i, u.posy + j)[n]);
        int lack = (veh->fuel_capacity("gasoline") - veh->fuel_left("gasoline")) < 200 ?
                   (veh->fuel_capacity("gasoline") - veh->fuel_left("gasoline")) : 200;
        if (gas->charges > lack) {
         veh->refill ("gasoline", lack);
         gas->charges -= lack;
         u.activity.moves_left -= 100;
        } else {
         add_msg(_("With a clang and a shudder, the gasoline pump goes silent."));
         veh->refill ("gasoline", gas->charges);
         m.i_at(u.posx + i, u.posy + j).erase(m.i_at(u.posx + i, u.posy + j).begin() + n);
         u.activity.moves_left = 0;
        }
        i = 2; j = 2;
        break;
       }
      }
     }
    }
   }
   u.pause(this);
  } else {
   u.activity.moves_left -= u.moves;
   u.moves = 0;
  }

  if (u.activity.moves_left <= 0) {	// We finished our activity!

   switch (u.activity.type) {

   case ACT_RELOAD:
    if (u.weapon.reload(u, u.activity.invlet))
     if (u.weapon.is_gun() && u.weapon.has_flag("RELOAD_ONE")) {
      add_msg(_("You insert a cartridge into your %s."),
              u.weapon.tname(this).c_str());
      if (u.recoil < 8)
       u.recoil = 8;
      if (u.recoil > 8)
       u.recoil = (8 + u.recoil) / 2;
     } else {
      add_msg(_("You reload your %s."), u.weapon.tname(this).c_str());
      u.recoil = 6;
     }
    else
     add_msg(_("Can't reload your %s."), u.weapon.tname(this).c_str());
    break;

   case ACT_READ:
    if (u.activity.index == -2)
     reading = dynamic_cast<it_book*>(u.weapon.type);
    else
     reading = dynamic_cast<it_book*>(u.inv.item_by_letter(u.activity.invlet).type);

    if (reading->fun != 0) {
     std::stringstream morale_text;
     u.add_morale(MORALE_BOOK, reading->fun * 5, reading->fun * 15, 60, 30,
                  true, reading);
    }

    no_recipes = true;
    if (reading->recipes.size() > 0)
    {
        bool recipe_learned = false;

        recipe_learned = u.try_study_recipe(this, reading);
        if (!u.studied_all_recipes(reading))
        {
            no_recipes = false;
        }

        // for books that the player cannot yet read due to skill level or have no skill component,
        // but contain lower level recipes, break out once recipe has been studied
        if (reading->type == NULL || (u.skillLevel(reading->type) < (int)reading->req))
        {
            if (recipe_learned)
                add_msg(_("The rest of the book is currently still beyond your understanding."));
            break;
        }
    }

    if (u.skillLevel(reading->type) < (int)reading->level) {
     int originalSkillLevel = u.skillLevel(reading->type);
     int min_ex = reading->time / 10 + u.int_cur / 4,
         max_ex = reading->time /  5 + u.int_cur / 2 - originalSkillLevel;
     if (min_ex < 1)
     {
         min_ex = 1;
     }
     if (max_ex < 2)
     {
         max_ex = 2;
     }
     if (max_ex > 10)
     {
         max_ex = 10;
     }
     if (max_ex < min_ex)
     {
         max_ex = min_ex;
     }

     min_ex *= originalSkillLevel + 1;
     max_ex *= originalSkillLevel + 1;

     u.skillLevel(reading->type).readBook(min_ex, max_ex, turn, reading->level);

     add_msg(_("You learn a little about %s! (%d%%%%)"), reading->type->name().c_str(),
             u.skillLevel(reading->type).exercise());

     if (u.skillLevel(reading->type) == originalSkillLevel && (u.activity.continuous || query_yn(_("Study %s?"), reading->type->name().c_str()))) {
      u.cancel_activity();
      if (u.activity.index == -2) {
       u.read(this,u.weapon.invlet);
      } else {
       u.read(this,u.activity.invlet);
      }
      if (u.activity.type != ACT_NULL) {
       u.activity.continuous = true;
       return;
      }
     }

     u.activity.continuous = false;

     int new_skill_level = (int)u.skillLevel(reading->type);
     if (new_skill_level > originalSkillLevel) {
      add_msg(_("You increase %s to level %d."),
              reading->type->name().c_str(),
              new_skill_level);

      if(new_skill_level % 4 == 0) {
       u.add_memorial_log(_("Reached skill level %d in %s."),
                      new_skill_level, reading->type->name().c_str());
      }
     }

     if (u.skillLevel(reading->type) == (int)reading->level) {
      if (no_recipes) {
       add_msg(_("You can no longer learn from %s."), reading->name.c_str());
      } else {
       add_msg(_("Your skill level won't improve, but %s has more recipes for you."), reading->name.c_str());
      }
     }
    }
    break;

   case ACT_WAIT:
    u.activity.continuous = false;
    add_msg(_("You finish waiting."));
    break;

   case ACT_CRAFT:
   case ACT_LONGCRAFT:
    complete_craft();
    break;

   case ACT_DISASSEMBLE:
    complete_disassemble();
    break;

   case ACT_BUTCHER:
    complete_butcher(u.activity.index);
    break;

   case ACT_FORAGE:
    forage();
    break;

   case ACT_BUILD:
    complete_construction();
    break;

   case ACT_TRAIN:
    if (u.activity.index < 0) {
     add_msg(_("You learn %s."), martial_arts_itype_ids[0 - u.activity.index].c_str());
     u.styles.push_back( martial_arts_itype_ids[0 - u.activity.index] );
    } else {
     Skill* skill = Skill::skill(u.activity.name);
     int new_skill_level = u.skillLevel(skill) + 1;
     u.skillLevel(skill).level(new_skill_level);
     add_msg(_("You finish training %s to level %d."),
             skill->name().c_str(),
             new_skill_level);
     if(new_skill_level % 4 == 0) {
       u.add_memorial_log(_("Reached skill level %d in %s."),
                      new_skill_level, skill->name().c_str());
     }

    }
    break;

   case ACT_VEHICLE:
    complete_vehicle (this);
    break;
   }

   bool act_veh = (u.activity.type == ACT_VEHICLE);
   bool act_longcraft = (u.activity.type == ACT_LONGCRAFT);
   u.activity.type = ACT_NULL;
   if (act_veh) {
    if (u.activity.values.size() < 7)
    {
     dbg(D_ERROR) << "game:process_activity: invalid ACT_VEHICLE values: "
                  << u.activity.values.size();
     debugmsg ("process_activity invalid ACT_VEHICLE values:%d",
                u.activity.values.size());
    }
    else {
     vehicle *veh = m.veh_at(u.activity.values[0], u.activity.values[1]);
     if (veh) {
      exam_vehicle(*veh, u.activity.values[0], u.activity.values[1],
                         u.activity.values[2], u.activity.values[3]);
      return;
     } else
     {
      dbg(D_ERROR) << "game:process_activity: ACT_VEHICLE: vehicle not found";
      debugmsg ("process_activity ACT_VEHICLE: vehicle not found");
     }
    }
   } else if (act_longcraft) {
    if (making_would_work(u.lastrecipe))
     make_all_craft(u.lastrecipe);
   }
  }
 }
}

void game::cancel_activity()
{
 u.cancel_activity();
}

bool game::cancel_activity_or_ignore_query(const char* reason, ...) {
  if(u.activity.type == ACT_NULL) return false;
  char buff[1024];
  va_list ap;
  va_start(ap, reason);
  vsprintf(buff, reason, ap);
  va_end(ap);
  std::string s(buff);

  bool force_uc = OPTIONS["FORCE_CAPITAL_YN"];
  int ch=(int)' ';

  std::string verbs[NUM_ACTIVITIES] = {
    _("whatever"),
    _("reloading"), _("reading"), _("playing"), _("waiting"), _("crafting"), _("crafting"),
    _("disassembly"), _("butchering"), _("foraging"), _("construction"), _("construction"), _("pumping gas"),
    _("training")
  };
  do {
    ch=popup_getkey(_("%s Stop %s? (Y)es, (N)o, (I)gnore further distractions and finish."),
      s.c_str(), verbs[u.activity.type].c_str() );
  } while (ch != '\n' && ch != ' ' && ch != KEY_ESCAPE &&
    ch != 'Y' && ch != 'N' && ch != 'I' &&
    (force_uc || (ch != 'y' && ch != 'n' && ch != 'i'))
  );
  if (ch == 'Y' || ch == 'y') {
    u.cancel_activity();
  } else if (ch == 'I' || ch == 'i' ) {
    return true;
  }
  return false;
}

void game::cancel_activity_query(const char* message, ...)
{
 char buff[1024];
 va_list ap;
 va_start(ap, message);
 vsprintf(buff, message, ap);
 va_end(ap);
 std::string s(buff);

 bool doit = false;;
 std::string verbs[NUM_ACTIVITIES] = {
    _("whatever"),
    _("reloading"), _("reading"), _("playing"), _("waiting"), _("crafting"), _("crafting"),
    _("disassembly"), _("butchering"), _("foraging"), _("construction"), _("construction"), _("pumping gas"),
    _("training")
 };

 if(ACT_NULL==u.activity.type)
 {
  doit = false;
 }
 else
 {
   if (query_yn(_("%s Stop %s?"), s.c_str(), verbs[u.activity.type].c_str()))
    doit = true;
 }

 if (doit)
  u.cancel_activity();
}

void game::update_weather()
{
    season_type season;
    // Default to current weather, and update to the furthest future weather if any.
    weather_segment prev_weather = {temperature, weather, nextweather};
    if( !future_weather.empty() )
    {
        prev_weather = future_weather.back();
    }

    while( prev_weather.deadline < turn + HOURS(MAX_FUTURE_WEATHER) )
    {
        weather_segment new_weather;
        // Pick a new weather type (most likely the same one)
        int chances[NUM_WEATHER_TYPES];
        int total = 0;
        season = prev_weather.deadline.get_season();
        for (int i = 0; i < NUM_WEATHER_TYPES; i++) {
            // Reduce the chance for freezing-temp-only weather to 0 if it's above freezing
            // and vice versa.
            if ((weather_data[i].avg_temperature[season] < 32 && temperature > 32) ||
                (weather_data[i].avg_temperature[season] > 32 && temperature < 32)   )
            {
                chances[i] = 0;
            } else {
                chances[i] = weather_shift[season][prev_weather.weather][i];
                if (weather_data[i].dangerous && u.has_artifact_with(AEP_BAD_WEATHER))
                {
                    chances[i] = chances[i] * 4 + 10;
                }
                total += chances[i];
            }
        }
        int choice = rng(0, total - 1);
        new_weather.weather = WEATHER_CLEAR;

        if (total > 0)
        {
            while (choice >= chances[new_weather.weather])
            {
                choice -= chances[new_weather.weather];
                new_weather.weather = weather_type(int(new_weather.weather) + 1);
            }
        } else {
            new_weather.weather = weather_type(int(new_weather.weather) + 1);
        }
        // Advance the weather timer
        int minutes = rng(weather_data[new_weather.weather].mintime,
                          weather_data[new_weather.weather].maxtime);
        new_weather.deadline = prev_weather.deadline + MINUTES(minutes);
        if (new_weather.weather == WEATHER_SUNNY && new_weather.deadline.is_night())
        {
            new_weather.weather = WEATHER_CLEAR;
        }

        // Now update temperature
        if (!one_in(4))
        { // 3 in 4 chance of respecting avg temp for the weather
            int average = weather_data[weather].avg_temperature[season];
            if (prev_weather.temperature < average)
            {
                new_weather.temperature = prev_weather.temperature + 1;
            } else if (prev_weather.temperature > average) {
                new_weather.temperature = prev_weather.temperature - 1;
            } else {
                new_weather.temperature = prev_weather.temperature;
            }
        } else {// 1 in 4 chance of random walk
            new_weather.temperature = prev_weather.temperature + rng(-1, 1);
        }

        if (turn.is_night())
        {
            new_weather.temperature += rng(-2, 1);
        } else {
            new_weather.temperature += rng(-1, 2);
        }
        prev_weather = new_weather;
        future_weather.push_back(new_weather);
    }

    if( turn >= nextweather )
    {
        weather_type old_weather = weather;
        weather = future_weather.front().weather;
        temperature = future_weather.front().temperature;
        nextweather = future_weather.front().deadline;
        future_weather.pop_front();
        if (weather != old_weather && weather_data[weather].dangerous &&
            levz >= 0 && m.is_outside(u.posx, u.posy))
        {
            cancel_activity_query(_("The weather changed to %s!"), weather_data[weather].name.c_str());
        }
    }
}

int game::assign_mission_id()
{
 int ret = next_mission_id;
 next_mission_id++;
 return ret;
}

void game::give_mission(mission_id type)
{
 mission tmp = mission_types[type].create(this);
 active_missions.push_back(tmp);
 u.active_missions.push_back(tmp.uid);
 u.active_mission = u.active_missions.size() - 1;
 mission_start m_s;
 mission *miss = find_mission(tmp.uid);
 (m_s.*miss->type->start)(this, miss);
}

void game::assign_mission(int id)
{
 u.active_missions.push_back(id);
 u.active_mission = u.active_missions.size() - 1;
 mission_start m_s;
 mission *miss = find_mission(id);
 (m_s.*miss->type->start)(this, miss);
}

int game::reserve_mission(mission_id type, int npc_id)
{
 mission tmp = mission_types[type].create(this, npc_id);
 active_missions.push_back(tmp);
 return tmp.uid;
}

int game::reserve_random_mission(mission_origin origin, point p, int npc_id)
{
 std::vector<int> valid;
 mission_place place;
 for (int i = 0; i < mission_types.size(); i++) {
  for (int j = 0; j < mission_types[i].origins.size(); j++) {
   if (mission_types[i].origins[j] == origin &&
       (place.*mission_types[i].place)(this, p.x, p.y)) {
    valid.push_back(i);
    j = mission_types[i].origins.size();
   }
  }
 }

 if (valid.empty())
  return -1;

 int index = valid[rng(0, valid.size() - 1)];

 return reserve_mission(mission_id(index), npc_id);
}

npc* game::find_npc(int id)
{
    //All the active NPCS are listed in the overmap.
    for (int i = 0; i < cur_om->npcs.size(); i++)
    {
        if (cur_om->npcs[i]->getID() == id)
            return (cur_om->npcs[i]);
    }
    return NULL;
}

int game::kill_count(mon_id mon){
 std::vector<mtype *> types;
 for (int i = 0; i < num_monsters; i++) {
  if (mtypes[i]-> id == mon)
   return kills[i];
 }
 return 0;
}

mission* game::find_mission(int id)
{
 for (int i = 0; i < active_missions.size(); i++) {
  if (active_missions[i].uid == id)
   return &(active_missions[i]);
 }
 dbg(D_ERROR) << "game:find_mission: " << id << " - it's NULL!";
 debugmsg("game::find_mission(%d) - it's NULL!", id);
 return NULL;
}

mission_type* game::find_mission_type(int id)
{
 for (int i = 0; i < active_missions.size(); i++) {
  if (active_missions[i].uid == id)
   return active_missions[i].type;
 }
 return NULL;
}

bool game::mission_complete(int id, int npc_id)
{
 mission *miss = find_mission(id);
 if (miss == NULL) { return false; }
 mission_type* type = miss->type;
 switch (type->goal) {
  case MGOAL_GO_TO: {
   point cur_pos(levx + int(MAPSIZE / 2), levy + int(MAPSIZE / 2));
   if (rl_dist(cur_pos.x, cur_pos.y, miss->target.x, miss->target.y) <= 1)
    return true;
   return false;
  } break;

  case MGOAL_GO_TO_TYPE: {
   oter_id cur_ter = cur_om->ter((levx + int (MAPSIZE / 2)) / 2, (levy + int (MAPSIZE / 2)) / 2, levz);
   if (cur_ter == miss->type->target_id){
    return true;}
   return false;
  } break;

  case MGOAL_FIND_ITEM:
   if (!u.has_amount(type->item_id, 1))
    return false;
   if (miss->npc_id != -1 && miss->npc_id != npc_id)
    return false;
   return true;

  case MGOAL_FIND_ANY_ITEM:
   return (u.has_mission_item(miss->uid) &&
           (miss->npc_id == -1 || miss->npc_id == npc_id));

  case MGOAL_FIND_MONSTER:
   if (miss->npc_id != -1 && miss->npc_id != npc_id)
    return false;
   for (int i = 0; i < z.size(); i++) {
    if (z[i].mission_id == miss->uid)
     return true;
   }
   return false;

  case MGOAL_RECRUIT_NPC:
   for (int i = 0; i < cur_om->npcs.size(); i++) {
    if (cur_om->npcs[i]->getID() == miss->recruit_npc_id) {
        if (cur_om->npcs[i]->attitude == NPCATT_FOLLOW)
            return true;
    }
   }
   return false;

  case MGOAL_RECRUIT_NPC_CLASS:
   for (int i = 0; i < cur_om->npcs.size(); i++) {
    if (cur_om->npcs[i]->myclass == miss->recruit_class) {
            if (cur_om->npcs[i]->attitude == NPCATT_FOLLOW)
                return true;
    }
   }
   return false;

  case MGOAL_FIND_NPC:
   return (miss->npc_id == npc_id);

  case MGOAL_KILL_MONSTER:
   return (miss->step >= 1);

  case MGOAL_KILL_MONSTER_TYPE:
   debugmsg("%d kill count", kill_count(miss->monster_type));
   debugmsg("%d goal", miss->monster_kill_goal);
   if (kill_count(miss->monster_type) >= miss->monster_kill_goal){
    return true;}
   return false;

  default:
   return false;
 }
 return false;
}

bool game::mission_failed(int id)
{
    mission *miss = find_mission(id);
    if (miss == NULL) { return true;} //If the mission is null it is failed.
    return (miss->failed);
}

void game::wrap_up_mission(int id)
{
 mission *miss = find_mission(id);
 if (miss == NULL) { return; }
 u.completed_missions.push_back( id );
 for (int i = 0; i < u.active_missions.size(); i++) {
  if (u.active_missions[i] == id) {
   u.active_missions.erase( u.active_missions.begin() + i );
   i--;
  }
 }
 switch (miss->type->goal) {
  case MGOAL_FIND_ITEM:
   u.use_amount(miss->type->item_id, 1);
   break;
  case MGOAL_FIND_ANY_ITEM:
   u.remove_mission_items(miss->uid);
   break;
 }
 mission_end endfunc;
 (endfunc.*miss->type->end)(this, miss);
}

void game::fail_mission(int id)
{
 mission *miss = find_mission(id);
 if (miss == NULL) { return; }
 miss->failed = true;
 u.failed_missions.push_back( id );
 for (int i = 0; i < u.active_missions.size(); i++) {
  if (u.active_missions[i] == id) {
   u.active_missions.erase( u.active_missions.begin() + i );
   i--;
  }
 }
 mission_fail failfunc;
 (failfunc.*miss->type->fail)(this, miss);
}

void game::mission_step_complete(int id, int step)
{
 mission *miss = find_mission(id);
 if (miss == NULL) { return; }
 miss->step = step;
 switch (miss->type->goal) {
  case MGOAL_FIND_ITEM:
  case MGOAL_FIND_MONSTER:
  case MGOAL_KILL_MONSTER: {
   bool npc_found = false;
   for (int i = 0; i < cur_om->npcs.size(); i++) {
    if (cur_om->npcs[i]->getID() == miss->npc_id) {
     miss->target = point(cur_om->npcs[i]->mapx, cur_om->npcs[i]->mapy);
     npc_found = true;
    }
   }
   if (!npc_found)
    miss->target = point(-1, -1);
  } break;
 }
}

void game::process_missions()
{
 for (int i = 0; i < active_missions.size(); i++) {
  if (active_missions[i].deadline > 0 &&
      int(turn) > active_missions[i].deadline)
   fail_mission(active_missions[i].uid);
 }
}

void game::handle_key_blocking_activity() {
    if (u.activity.moves_left > 0 && u.activity.continuous == true &&
        (  // bool activity_is_abortable() ?
            u.activity.type == ACT_READ ||
            u.activity.type == ACT_BUILD ||
            u.activity.type == ACT_LONGCRAFT ||
            u.activity.type == ACT_REFILL_VEHICLE ||
            u.activity.type == ACT_WAIT
        )
    ) {
        timeout(1);
        char ch = input();
        if(ch != ERR) {
            timeout(-1);
            switch(keymap[ch]){  // should probably make the switch in handle_action() a function
                case ACTION_PAUSE:
                    cancel_activity_query(_("Confirm:"));
                    break;
                case ACTION_PL_INFO:
                    u.disp_info(this);
                    refresh_all();
                    break;
                case ACTION_MESSAGES:
                    msg_buffer();
                    break;
                case ACTION_HELP:
                    help();
                    refresh_all();
                    break;
            }
        }
        timeout(-1);
    }
}
//// item submenu for 'i' and '/'
int game::inventory_item_menu(char chItem, int iStartX, int iWidth) {
    int cMenu = (int)'+';

    if (u.has_item(chItem)) {
        item oThisItem = u.i_at(chItem);
        std::vector<iteminfo> vThisItem, vDummy, vMenu;

        const int iOffsetX = 2;
        const bool bHPR = hasPickupRule(oThisItem.tname(this));

        vMenu.push_back(iteminfo("MENU", "", "iOffsetX", iOffsetX));
        vMenu.push_back(iteminfo("MENU", "", "iOffsetY", 0));
        vMenu.push_back(iteminfo("MENU", "a", _("<a>ctivate"), u.rate_action_use(&oThisItem)));
        vMenu.push_back(iteminfo("MENU", "R", _("<R>ead"), u.rate_action_read(&oThisItem, this)));
        vMenu.push_back(iteminfo("MENU", "E", _("<E>at"), u.rate_action_eat(&oThisItem)));
        vMenu.push_back(iteminfo("MENU", "W", _("<W>ear"), u.rate_action_wear(&oThisItem)));
        vMenu.push_back(iteminfo("MENU", "w", _("<w>ield")));
        vMenu.push_back(iteminfo("MENU", "t", _("<t>hrow")));
        vMenu.push_back(iteminfo("MENU", "T", _("<T>ake off"), u.rate_action_takeoff(&oThisItem)));
        vMenu.push_back(iteminfo("MENU", "d", _("<d>rop")));
        vMenu.push_back(iteminfo("MENU", "U", _("<U>nload"), u.rate_action_unload(&oThisItem)));
        vMenu.push_back(iteminfo("MENU", "r", _("<r>eload"), u.rate_action_reload(&oThisItem)));
        vMenu.push_back(iteminfo("MENU", "D", _("<D>isassemble"), u.rate_action_disassemble(&oThisItem, this)));
        vMenu.push_back(iteminfo("MENU", "=", _("<=> reassign")));
        vMenu.push_back(iteminfo("MENU", (bHPR) ? "-":"+", (bHPR) ? _("<-> Autopickup") : _("<+> Autopickup"), (bHPR) ? HINT_IFFY : HINT_GOOD));

        oThisItem.info(true, &vThisItem, this);
        compare_split_screen_popup(iStartX,iWidth, TERMY-VIEW_OFFSET_Y*2, oThisItem.tname(this), vThisItem, vDummy);

        const int iMenuStart = iOffsetX;
        const int iMenuItems = vMenu.size() - 1;
        int iSelected = iOffsetX - 1;

        do {
            cMenu = compare_split_screen_popup(iStartX + iWidth, iMenuItems + iOffsetX, vMenu.size()+iOffsetX*2, "", vMenu, vDummy,
                iSelected >= iOffsetX && iSelected <= iMenuItems ? iSelected : -1
            );

            switch(cMenu) {
                case 'a':
                 use_item(chItem);
                 break;
                case 'E':
                 eat(chItem);
                 break;
                case 'W':
                 wear(chItem);
                 break;
                case 'w':
                 wield(chItem);
                 break;
                case 't':
                 plthrow(chItem);
                 break;
                case 'T':
                 takeoff(chItem);
                 break;
                case 'd':
                 drop(chItem);
                 break;
                case 'U':
                 unload(chItem);
                 break;
                case 'r':
                 reload(chItem);
                 break;
                case 'R':
                 u.read(this, chItem);
                 break;
                case 'D':
                 disassemble(chItem);
                 break;
                case '=':
                 reassign_item(chItem);
                 break;
                case KEY_UP:
                 iSelected--;
                 break;
                case KEY_DOWN:
                 iSelected++;
                 break;
                case '+':
                 if (!bHPR) {
                  addPickupRule(oThisItem.tname(this));
                  add_msg(_("'%s' added to character pickup rules."), oThisItem.tname(this).c_str());
                 }
                 break;
                case '-':
                 if (bHPR) {
                  removePickupRule(oThisItem.tname(this));
                  add_msg(_("'%s' removed from character pickup rules."), oThisItem.tname(this).c_str());
                 }
                 break;
                default:
                 break;
            }
            if( iSelected < iMenuStart-1 ) { // wraparound, but can be hidden
                iSelected = iMenuItems;
            } else if ( iSelected > iMenuItems + 1 ) {
                iSelected = iMenuStart;
            }
        } while (cMenu == KEY_DOWN || cMenu == KEY_UP );
    }
    return cMenu;
}
//

bool game::handle_action()
{
    char ch = '.';

    char cGlyph = ',';
    nc_color colGlyph = c_ltblue;
    float fFactor = 0.01f;

    bool bWeatherEffect = true;
    switch(weather) {
        case WEATHER_ACID_DRIZZLE:
            cGlyph = '.';
            colGlyph = c_ltgreen;
            fFactor = 0.01f;
            break;
        case WEATHER_ACID_RAIN:
            cGlyph = ',';
            colGlyph = c_ltgreen;
            fFactor = 0.02f;
            break;
        case WEATHER_DRIZZLE:
            cGlyph = '.';
            colGlyph = c_ltblue;
            fFactor = 0.01f;
            break;
        case WEATHER_RAINY:
            cGlyph = ',';
            colGlyph = c_ltblue;
            fFactor = 0.02f;
            break;
        case WEATHER_THUNDER:
            cGlyph = '.';
            colGlyph = c_ltblue;
            fFactor = 0.02f;
            break;
        case WEATHER_LIGHTNING:
            cGlyph = ',';
            colGlyph = c_ltblue;
            fFactor = 0.04f;
            break;
        case WEATHER_SNOW:
            cGlyph = '*';
            colGlyph = c_white;
            fFactor = 0.02f;
            break;
        case WEATHER_SNOWSTORM:
            cGlyph = '*';
            colGlyph = c_white;
            fFactor = 0.04f;
            break;
        default:
            bWeatherEffect = false;
            break;
    }

    if (bWeatherEffect && OPTIONS["RAIN_ANIMATION"]) {
        int iStartX = (TERRAIN_WINDOW_WIDTH > 121) ? (TERRAIN_WINDOW_WIDTH-121)/2 : 0;
        int iStartY = (TERRAIN_WINDOW_HEIGHT > 121) ? (TERRAIN_WINDOW_HEIGHT-121)/2: 0;
        int iEndX = (TERRAIN_WINDOW_WIDTH > 121) ? TERRAIN_WINDOW_WIDTH-(TERRAIN_WINDOW_WIDTH-121)/2: TERRAIN_WINDOW_WIDTH;
        int iEndY = (TERRAIN_WINDOW_HEIGHT > 121) ? TERRAIN_WINDOW_HEIGHT-(TERRAIN_WINDOW_HEIGHT-121)/2: TERRAIN_WINDOW_HEIGHT;

        //x% of the Viewport, only shown on visible areas
        int dropCount = iEndX * iEndY * fFactor;
        std::vector<std::pair<int, int> > vDrops;

        int iCh;

        timeout(125);
        do {
            for(int i=0; i < vDrops.size(); i++) {
                m.drawsq(w_terrain, u,
                         vDrops[i].first - getmaxx(w_terrain)/2 + u.posx + u.view_offset_x,
                         vDrops[i].second - getmaxy(w_terrain)/2 + u.posy + u.view_offset_y,
                         false,
                         true,
                         u.posx + u.view_offset_x,
                         u.posy + u.view_offset_y);
            }

            vDrops.clear();

            for(int i=0; i < dropCount; i++) {
                int iRandX = rng(iStartX, iEndX-1);
                int iRandY = rng(iStartY, iEndY-1);

                if (mapRain[iRandY][iRandX]) {
                    vDrops.push_back(std::make_pair(iRandX, iRandY));
                    mvwputch(w_terrain, iRandY, iRandX, colGlyph, cGlyph);
                }
            }

            wrefresh(w_terrain);
        } while ((iCh = getch()) == ERR);
        timeout(-1);

        ch = input(iCh);
    } else {
        ch = input();
    }

  if (keymap.find(ch) == keymap.end()) {
	  if (ch != ' ' && ch != '\n')
		  add_msg(_("Unknown command: '%c'"), ch);
	  return false;
  }

 action_id act = keymap[ch];

// This has no action unless we're in a special game mode.
 gamemode->pre_action(this, act);

 int veh_part;
 vehicle *veh = m.veh_at(u.posx, u.posy, veh_part);
 bool veh_ctrl = veh && veh->player_in_control (&u);

 int soffset = OPTIONS["MOVE_VIEW_OFFSET"];
 int soffsetr = 0 - soffset;

 int before_action_moves = u.moves;

 switch (act) {

  case ACTION_PAUSE:
   if (run_mode == 2) // Monsters around and we don't wanna pause
     add_msg(_("Monster spotted--safe mode is on! (%s to turn it off.)"),
             press_x(ACTION_TOGGLE_SAFEMODE).c_str());
   else
    u.pause(this);
   break;

  case ACTION_MOVE_N:
   moveCount++;

   if (veh_ctrl)
    pldrive(0, -1);
   else
    plmove(0, -1);
   break;

  case ACTION_MOVE_NE:
   moveCount++;

   if (veh_ctrl)
    pldrive(1, -1);
   else
    plmove(1, -1);
   break;

  case ACTION_MOVE_E:
   moveCount++;

   if (veh_ctrl)
    pldrive(1, 0);
   else
    plmove(1, 0);
   break;

  case ACTION_MOVE_SE:
   moveCount++;

   if (veh_ctrl)
    pldrive(1, 1);
   else
    plmove(1, 1);
   break;

  case ACTION_MOVE_S:
   moveCount++;

   if (veh_ctrl)
    pldrive(0, 1);
   else
   plmove(0, 1);
   break;

  case ACTION_MOVE_SW:
   moveCount++;

   if (veh_ctrl)
    pldrive(-1, 1);
   else
    plmove(-1, 1);
   break;

  case ACTION_MOVE_W:
   moveCount++;

   if (veh_ctrl)
    pldrive(-1, 0);
   else
    plmove(-1, 0);
   break;

  case ACTION_MOVE_NW:
   moveCount++;

   if (veh_ctrl)
    pldrive(-1, -1);
   else
    plmove(-1, -1);
   break;

  case ACTION_MOVE_DOWN:
   if (!u.in_vehicle)
    vertical_move(-1, false);
   break;

  case ACTION_MOVE_UP:
   if (!u.in_vehicle)
    vertical_move( 1, false);
   break;

  case ACTION_CENTER:
   u.view_offset_x = 0;
   u.view_offset_y = 0;
   break;

  case ACTION_SHIFT_N:
   u.view_offset_y += soffsetr;
   break;

  case ACTION_SHIFT_NE:
   u.view_offset_x += soffset;
   u.view_offset_y += soffsetr;
   break;

  case ACTION_SHIFT_E:
   u.view_offset_x += soffset;
   break;

  case ACTION_SHIFT_SE:
   u.view_offset_x += soffset;
   u.view_offset_y += soffset;
   break;

  case ACTION_SHIFT_S:
   u.view_offset_y += soffset;
   break;

  case ACTION_SHIFT_SW:
   u.view_offset_x += soffsetr;
   u.view_offset_y += soffset;
   break;

  case ACTION_SHIFT_W:
   u.view_offset_x += soffsetr;
   break;

  case ACTION_SHIFT_NW:
   u.view_offset_x += soffsetr;
   u.view_offset_y += soffsetr;
   break;

  case ACTION_OPEN:
   open();
   break;

  case ACTION_CLOSE:
   close();
   break;

  case ACTION_SMASH:
   if (veh_ctrl)
    handbrake();
   else
    smash();
   break;

  case ACTION_EXAMINE:
   examine();
   break;

  case ACTION_ADVANCEDINV:
   advanced_inv();
   break;

  case ACTION_PICKUP:
   pickup(u.posx, u.posy, 1);
   break;

  case ACTION_BUTCHER:
   butcher();
   break;

  case ACTION_CHAT:
   chat();
   break;

  case ACTION_LOOK:
   look_around();
   break;

  case ACTION_PEEK:
   peek();
   break;

  case ACTION_LIST_ITEMS:
   list_items();
   break;

  case ACTION_INVENTORY: {
   int cMenu = ' ';
   do {
     const std::string sSpaces = "                              ";
     char chItem = inv(_("Inventory:"));
     cMenu=inventory_item_menu(chItem);
   } while (cMenu == ' ' || cMenu == '.' || cMenu == 'q' || cMenu == '\n' || cMenu == KEY_ESCAPE || cMenu == KEY_LEFT || cMenu == '=' );
   refresh_all();
  } break;

  case ACTION_COMPARE:
   compare();
   break;

  case ACTION_ORGANIZE:
   reassign_item();
   break;

  case ACTION_USE:
   use_item();
   break;

  case ACTION_USE_WIELDED:
   use_wielded_item();
   break;

  case ACTION_WEAR:
   wear();
   break;

  case ACTION_TAKE_OFF:
   takeoff();
   break;

  case ACTION_EAT:
   eat();
   break;

  case ACTION_READ:
   read();
   break;

  case ACTION_WIELD:
   wield();
   break;

  case ACTION_PICK_STYLE:
   u.pick_style(this);
   if (u.weapon.type->id == "null" || u.weapon.is_style()) {
    u.weapon = item(itypes[u.style_selected], 0);
    u.weapon.invlet = ':';
   }
   refresh_all();
   break;

  case ACTION_RELOAD:
   reload();
   break;

  case ACTION_UNLOAD:
   unload(u.weapon);
   break;

  case ACTION_THROW:
   plthrow();
   break;

  case ACTION_FIRE:
   plfire(false);
   break;

  case ACTION_FIRE_BURST:
   plfire(true);
   break;

  case ACTION_SELECT_FIRE_MODE:
   u.weapon.next_mode();
   break;

  case ACTION_DROP:
   drop();
   break;

  case ACTION_DIR_DROP:
   drop_in_direction();
   break;

  case ACTION_BIONICS:
   u.power_bionics(this);
   refresh_all();
   break;

  case ACTION_SORT_ARMOR:
    u.sort_armor(this);
    refresh_all();
    break;

  case ACTION_WAIT:
   wait();
   if (veh_ctrl) {
    veh->turret_mode++;
    if (veh->turret_mode > 1)
     veh->turret_mode = 0;
   }
   break;

  case ACTION_CRAFT:
   craft();
   break;

  case ACTION_RECRAFT:
   recraft();
   break;

  case ACTION_LONGCRAFT:
   long_craft();
   break;

  case ACTION_DISASSEMBLE:
   if (u.in_vehicle)
    add_msg(_("You can't disassemble items while in vehicle."));
   else
    disassemble();
   break;

  case ACTION_CONSTRUCT:
   if (u.in_vehicle)
    add_msg(_("You can't construct while in vehicle."));
   else
    construction_menu();
   break;

  case ACTION_SLEEP:
    if (veh_ctrl)
    {
        add_msg(_("Vehicle control has moved, %s"),
        press_x(ACTION_CONTROL_VEHICLE, _("new binding is "), _("new default binding is '^'.")).c_str());

    }
    else
    {
        uimenu as_m;
        as_m.text = _("Are you sure you want to sleep?");
        as_m.entries.push_back(uimenu_entry(0, true, (OPTIONS["FORCE_CAPITAL_YN"]?'Y':'y'), _("Yes.")) );

        if (OPTIONS["SAVE_SLEEP"])
        {
            as_m.entries.push_back(uimenu_entry(1,
            (moves_since_last_save || item_exchanges_since_save),
            (OPTIONS["FORCE_CAPITAL_YN"]?'S':'s'),
            _("Yes, and save game before sleeping.") ));
        }

        as_m.entries.push_back(uimenu_entry(2, true, (OPTIONS["FORCE_CAPITAL_YN"]?'N':'n'), _("No.")) );

        if (u.has_item_with_flag("ALARMCLOCK"))
        {
            as_m.entries.push_back(uimenu_entry(3, true, '3', _("Set alarm to wake up in 3 hours.") ));
            as_m.entries.push_back(uimenu_entry(4, true, '4', _("Set alarm to wake up in 4 hours.") ));
            as_m.entries.push_back(uimenu_entry(5, true, '5', _("Set alarm to wake up in 5 hours.") ));
            as_m.entries.push_back(uimenu_entry(6, true, '6', _("Set alarm to wake up in 6 hours.") ));
            as_m.entries.push_back(uimenu_entry(7, true, '7', _("Set alarm to wake up in 7 hours.") ));
            as_m.entries.push_back(uimenu_entry(8, true, '8', _("Set alarm to wake up in 8 hours.") ));
            as_m.entries.push_back(uimenu_entry(9, true, '9', _("Set alarm to wake up in 9 hours.") ));
        }

        as_m.query(); /* calculate key and window variables, generate window, and loop until we get a valid answer */

        bool bSleep = false;
        if (as_m.ret == 0)
        {
            bSleep = true;
        }
        else if (as_m.ret == 1)
        {
            quicksave();
            bSleep = true;
        }
        else if (as_m.ret >= 3 && as_m.ret <= 9)
        {
            u.add_disease("alarm_clock", 600*as_m.ret);
            bSleep = true;
        }

        if (bSleep)
        {
            u.moves = 0;
            u.try_to_sleep(this);
        }
    }
    break;

  case ACTION_CONTROL_VEHICLE:
   control_vehicle();
   break;

  case ACTION_TOGGLE_SAFEMODE:
   if (run_mode == 0 ) {
    run_mode = 1;
    mostseen = 0;
    add_msg(_("Safe mode ON!"));
   } else {
    turnssincelastmon = 0;
    run_mode = 0;
    if (autosafemode)
    add_msg(_("Safe mode OFF! (Auto safe mode still enabled!)"));
    else
    add_msg(_("Safe mode OFF!"));
   }
   break;

  case ACTION_TOGGLE_AUTOSAFE:
   if (autosafemode) {
    add_msg(_("Auto safe mode OFF!"));
    autosafemode = false;
   } else {
    add_msg(_("Auto safe mode ON"));
    autosafemode = true;
   }
   break;

  case ACTION_IGNORE_ENEMY:
   if (run_mode == 2) {
    add_msg(_("Ignoring enemy!"));
    run_mode = 1;
   }
   break;

  case ACTION_SAVE:
   if (query_yn(_("Save and quit?"))) {
    save();
    u.moves = 0;
    uquit = QUIT_SAVED;
    MAPBUFFER.make_volatile();
   }
   break;

  case ACTION_QUICKSAVE:
    quicksave();
    return false;

  case ACTION_QUIT:
   if (query_yn(_("Commit suicide?"))) {
    u.moves = 0;
    place_corpse();
    uquit = QUIT_SUICIDE;
   }
   break;

  case ACTION_PL_INFO:
   u.disp_info(this);
   refresh_all();
   break;

  case ACTION_MAP:
   draw_overmap();
   break;

  case ACTION_MISSIONS:
   list_missions();
   break;

  case ACTION_KILLS:
   disp_kills();
   break;

  case ACTION_FACTIONS:
   list_factions(_("FACTIONS:"));
   break;

  case ACTION_MORALE:
   u.disp_morale(this);
   refresh_all();
   break;

  case ACTION_MESSAGES:
   msg_buffer();
   break;

  case ACTION_HELP:
   help();
   refresh_all();
   break;

  case ACTION_DEBUG:
   debug();
   break;

  case ACTION_DISPLAY_SCENT:
   display_scent();
   break;

  case ACTION_TOGGLE_DEBUGMON:
   debugmon = !debugmon;
   if (debugmon) {
    add_msg(_("Debug messages ON!"));
   } else {
    add_msg(_("Debug messages OFF!"));
   }
   break;
 }

 gamemode->post_action(this, act);

 u.movecounter = before_action_moves - u.moves;

 return true;
}

#define SCENT_RADIUS 40

int& game::scent(int x, int y)
{
  if (x < (SEEX * MAPSIZE / 2) - SCENT_RADIUS || x >= (SEEX * MAPSIZE / 2) + SCENT_RADIUS ||
      y < (SEEY * MAPSIZE / 2) - SCENT_RADIUS || y >= (SEEY * MAPSIZE / 2) + SCENT_RADIUS) {
  nulscent = 0;
  return nulscent;	// Out-of-bounds - null scent
 }
 return grscent[x][y];
}

void game::update_scent()
{
 int newscent[SEEX * MAPSIZE][SEEY * MAPSIZE];
 int scale[SEEX * MAPSIZE][SEEY * MAPSIZE];
 if (!u.has_active_bionic("bio_scent_mask"))
  grscent[u.posx][u.posy] = u.scent;

 for (int x = u.posx - SCENT_RADIUS; x <= u.posx + SCENT_RADIUS; x++) {
  for (int y = u.posy - SCENT_RADIUS; y <= u.posy + SCENT_RADIUS; y++) {
   const int move_cost = m.move_cost_ter_furn(x, y);
   field &field_at = m.field_at(x, y);
   const bool is_bashable = m.has_flag(bashable, x, y);
   newscent[x][y] = 0;
   scale[x][y] = 1;
   if (move_cost != 0 || is_bashable) {
    int squares_used = 0;
    const int this_field = grscent[x][y];
    /*
    for (int i = x - 1; i <= x + 1; i++) {
        for (int j = y - 1; j <= y + 1; j++) {
           const int scent = grscent[i][j];
           newscent[x][y] += (scent >= this_field) * scent;
           squares_used += (scent >= this_field);
        }
    }
    */
    // Unrolled for performance.  The above block is the rolled up equivalent.
    newscent[x][y] += grscent[x - 1] [y - 1] * (grscent  [x - 1] [y - 1] >= this_field);
    squares_used +=   grscent[x - 1] [y - 1] >= this_field;
    newscent[x][y] += grscent[x - 1] [y]     * (grscent  [x - 1] [y]     >= this_field);
    squares_used +=   grscent[x - 1] [y]     >= this_field;
    newscent[x][y] += grscent[x - 1] [y + 1] * (grscent  [x - 1] [y + 1] >= this_field);
    squares_used +=   grscent[x - 1] [y + 1] >= this_field;
    newscent[x][y] += grscent[x]     [y - 1] * (grscent  [x]     [y - 1] >= this_field);
    squares_used +=   grscent[x]     [y - 1] >= this_field;
    newscent[x][y] += grscent[x]     [y]     * (grscent  [x]     [y]     >= this_field);
    squares_used +=   grscent[x]     [y]     >= this_field;
    newscent[x][y] += grscent[x]     [y + 1] * (grscent  [x]     [y + 1] >= this_field);
    squares_used +=   grscent[x]     [y + 1] >= this_field;
    newscent[x][y] += grscent[x + 1] [y - 1] * (grscent  [x + 1] [y - 1] >= this_field);
    squares_used +=   grscent[x + 1] [y - 1] >= this_field;
    newscent[x][y] += grscent[x + 1] [y]     * (grscent  [x + 1] [y]     >= this_field);
    squares_used +=   grscent[x + 1] [y]     >= this_field;
    newscent[x][y] += grscent[x + 1] [y + 1] * (grscent  [x + 1] [y + 1] >= this_field);
    squares_used +=   grscent[x + 1] [y + 1] >= this_field;

    scale[x][y] += squares_used;
    if (field_at.findField(fd_slime) && newscent[x][y] < 10 * field_at.findField(fd_slime)->getFieldDensity())
    {
        newscent[x][y] = 10 * field_at.findField(fd_slime)->getFieldDensity();
    }
    if (newscent[x][y] > 10000)
    {
     dbg(D_ERROR) << "game:update_scent: Wacky scent at " << x << ","
                  << y << " (" << newscent[x][y] << ")";
     debugmsg("Wacky scent at %d, %d (%d)", x, y, newscent[x][y]);
     newscent[x][y] = 0; // Scent should never be higher
    }
    //Greatly reduce scent for bashable barriers, even more for ductaped barriers
    if( move_cost == 0 && is_bashable)
    {
        if( m.has_flag(reduce_scent, x, y))
        {
            scale[x][y] *= 12;
        } else {
            scale[x][y] *= 4;
        }
    }
   }
  }
 }
 // Simultaneously copy the scent values back and scale them down based on factors determined in
 // the first loop.
 for (int x = u.posx - SCENT_RADIUS; x <= u.posx + SCENT_RADIUS; x++) {
     for (int y = u.posy - SCENT_RADIUS; y <= u.posy + SCENT_RADIUS; y++) {
         grscent[x][y] = newscent[x][y] / scale[x][y];
     }
 }
}

bool game::is_game_over()
{
 if (uquit != QUIT_NO)
  return true;
 for (int i = 0; i <= hp_torso; i++) {
  if (u.hp_cur[i] < 1) {
   place_corpse();
   std::stringstream playerfile;
   playerfile << "save/" << base64_encode(u.name) << ".sav";
   unlink(playerfile.str().c_str());
   uquit = QUIT_DIED;
   return true;
  }
 }
 return false;
}

void game::place_corpse()
{
  std::vector<item *> tmp = u.inv_dump();
  item your_body;
  your_body.make_corpse(itypes["corpse"], mtypes[mon_null], turn);
  your_body.name = u.name;
  for (int i = 0; i < tmp.size(); i++)
    m.add_item_or_charges(u.posx, u.posy, *(tmp[i]));
  for (int i = 0; i < u.my_bionics.size(); i++) {
    if (itypes.find(u.my_bionics[i].id) != itypes.end()) {
      your_body.contents.push_back(item(itypes[u.my_bionics[i].id], turn));
    }
  }
  int pow = u.max_power_level;
  while (pow >= 4) {
    if (pow % 4 != 0 && pow >= 10){
      pow -= 10;
      your_body.contents.push_back(item(itypes["bio_power_storage_mkII"], turn));
    } else {
      pow -= 4;
      your_body.contents.push_back(item(itypes["bio_power_storage"], turn));
    }
  }
  m.add_item_or_charges(u.posx, u.posy, your_body);
}

void game::death_screen()
{
    gamemode->game_over(this);

#if (defined _WIN32 || defined __WIN32__)
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind;
    TCHAR Buffer[MAX_PATH];

    GetCurrentDirectory(MAX_PATH, Buffer);
    SetCurrentDirectory("save");
    std::stringstream playerfile;
    playerfile << base64_encode(u.name) << "*";
    hFind = FindFirstFile(playerfile.str().c_str(), &FindFileData);
    if(INVALID_HANDLE_VALUE != hFind) {
        do {
            DeleteFile(FindFileData.cFileName);
        } while(FindNextFile(hFind, &FindFileData) != 0);
        FindClose(hFind);
    }
    SetCurrentDirectory(Buffer);
#else
    DIR *save_dir = opendir("save");
    struct dirent *save_dirent = NULL;
    if(save_dir != NULL && 0 == chdir("save"))
    {
        while ((save_dirent = readdir(save_dir)) != NULL)
        {
            std::string name_prefix = save_dirent->d_name;
            std::string tmpname = base64_encode(u.name);
            name_prefix = name_prefix.substr(0,tmpname.length());

            if (tmpname == name_prefix)
            {
                std::string graveyard_path( "../graveyard/" );
                mkdir( graveyard_path.c_str(), 0777 );
                graveyard_path.append( save_dirent->d_name );
                (void)rename( save_dirent->d_name, graveyard_path.c_str() );
            }
        }
        (void)chdir("..");
        (void)closedir(save_dir);
    }
#endif

    const std::string sText = _("GAME OVER - Press Spacebar to Quit");

    WINDOW *w_death = newwin(5, 6+sText.size(), (TERMY-5)/2, (TERMX+6-sText.size())/2);

    wborder(w_death, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                     LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );

    mvwprintz(w_death, 2, 3, c_ltred, sText.c_str());
    wrefresh(w_death);
    refresh();
    InputEvent input;
    do
        input = get_input();
    while(input != Cancel && input != Close && input != Confirm);
    delwin(w_death);

    msg_buffer();
    disp_kills();
}


bool game::load_master()
{
 std::ifstream fin;
 std::string data;
 char junk;
 fin.open("save/master.gsav");
 if (!fin.is_open())
  return false;

// First, get the next ID numbers for each of these
 fin >> next_mission_id >> next_faction_id >> next_npc_id;
 int num_missions, num_factions;

 fin >> num_missions;
 if (fin.peek() == '\n')
  fin.get(junk); // Chomp that pesky endline
 for (int i = 0; i < num_missions; i++) {
  mission tmpmiss;
  tmpmiss.load_info(this, fin);
  active_missions.push_back(tmpmiss);
 }

 fin >> num_factions;
 if (fin.peek() == '\n')
  fin.get(junk); // Chomp that pesky endline
 for (int i = 0; i < num_factions; i++) {
  getline(fin, data);
  faction tmp;
  tmp.load_info(data);
  factions.push_back(tmp);
 }
 fin.close();
 return true;
}

void game::load_uistate() {
    const std::string savedir="save";
    std::stringstream savefile;
    savefile << savedir << "/uistate.json";

    std::ifstream fin;
    fin.open(savefile.str().c_str());
    if(!fin.good()) {
        fin.close();
        return;
    }
    picojson::value wrapped_data;
    fin >> wrapped_data;
    fin.close();
    std::string jsonerr=picojson::get_last_error();
    if ( ! jsonerr.empty() ) {
       dbg(D_ERROR) << "load_uistate: " << jsonerr.c_str();
       return;
    }
    bool success=uistate.load(wrapped_data);
    if ( ! success ) {
       dbg(D_ERROR) << "load_uistate: " << uistate.errdump;
    }
    uistate.errdump="";
}

void game::load_artifacts()
{
    std::ifstream file_test("save/artifacts.gsav");
    if(!file_test.good())
    {
    	file_test.close();
    	return;
    }
    file_test.close();

    catajson artifact_list(std::string("save/artifacts.gsav"));

    if(!json_good())
    {
        uquit = QUIT_ERROR;
        return;
    }

    artifact_list.set_begin();
    while (artifact_list.has_curr())
    {
	catajson artifact = artifact_list.curr();
	std::string id = artifact.get(std::string("id")).as_string();
	unsigned int price = artifact.get(std::string("price")).as_int();
	std::string name = artifact.get(std::string("name")).as_string();
	std::string description =
	    artifact.get(std::string("description")).as_string();
	char sym = artifact.get(std::string("sym")).as_int();
	nc_color color =
	    int_to_color(artifact.get(std::string("color")).as_int());
	std::string m1 = artifact.get(std::string("m1")).as_string();
	std::string m2 = artifact.get(std::string("m2")).as_string();
	unsigned int volume = artifact.get(std::string("volume")).as_int();
	unsigned int weight = artifact.get(std::string("weight")).as_int();
 signed char melee_dam = artifact.get(std::string("melee_dam")).as_int();
 signed char melee_cut = artifact.get(std::string("melee_cut")).as_int();
	signed char m_to_hit = artifact.get(std::string("m_to_hit")).as_int();
 std::set<std::string> item_tags = artifact.get(std::string("item_flags")).as_tags();

	std::string type = artifact.get(std::string("type")).as_string();
	if (type == "artifact_tool")
	{
	    unsigned int max_charges =
		artifact.get(std::string("max_charges")).as_int();
	    unsigned int def_charges =
		artifact.get(std::string("def_charges")).as_int();
	    unsigned char charges_per_use =
		artifact.get(std::string("charges_per_use")).as_int();
	    unsigned char turns_per_charge =
		artifact.get(std::string("turns_per_charge")).as_int();
	    ammotype ammo = artifact.get(std::string("ammo")).as_string();
	    std::string revert_to =
		artifact.get(std::string("revert_to")).as_string();

	    it_artifact_tool* art_type = new it_artifact_tool(
		id, price, name, description, sym, color, m1, m2, volume,
		weight, melee_dam, melee_cut, m_to_hit, item_tags,

		max_charges, def_charges, charges_per_use, turns_per_charge,
		ammo, revert_to);

	    art_charge charge_type =
		(art_charge)artifact.get(std::string("charge_type")).as_int();

	    catajson effects_wielded_json =
		artifact.get(std::string("effects_wielded"));
	    effects_wielded_json.set_begin();
	    std::vector<art_effect_passive> effects_wielded;
	    while (effects_wielded_json.has_curr())
	    {
		art_effect_passive effect =
		    (art_effect_passive)effects_wielded_json.curr().as_int();
		effects_wielded.push_back(effect);
		effects_wielded_json.next();
	    }

	    catajson effects_activated_json =
		artifact.get(std::string("effects_activated"));
	    effects_activated_json.set_begin();
	    std::vector<art_effect_active> effects_activated;
	    while (effects_activated_json.has_curr())
	    {
		art_effect_active effect =
		    (art_effect_active)effects_activated_json.curr().as_int();
		effects_activated.push_back(effect);
		effects_activated_json.next();
	    }

	    catajson effects_carried_json =
		artifact.get(std::string("effects_carried"));
	    effects_carried_json.set_begin();
	    std::vector<art_effect_passive> effects_carried;
	    while (effects_carried_json.has_curr())
	    {
		art_effect_passive effect =
		    (art_effect_passive)effects_carried_json.curr().as_int();
		effects_carried.push_back(effect);
		effects_carried_json.next();
	    }

	    art_type->charge_type = charge_type;
	    art_type->effects_wielded = effects_wielded;
	    art_type->effects_activated = effects_activated;
	    art_type->effects_carried = effects_carried;

	    itypes[id] = art_type;
	}
	else if (type == "artifact_armor")
	{
	    unsigned char covers =
		artifact.get(std::string("covers")).as_int();
	    signed char encumber =
		artifact.get(std::string("encumber")).as_int();
	    unsigned char coverage =
		artifact.get(std::string("coverage")).as_int();
	    unsigned char thickness =
		artifact.get(std::string("material_thickness")).as_int();
	    unsigned char env_resist =
		artifact.get(std::string("env_resist")).as_int();
	    signed char warmth = artifact.get(std::string("warmth")).as_int();
	    unsigned char storage =
		artifact.get(std::string("storage")).as_int();
	    bool power_armor =
		artifact.get(std::string("power_armor")).as_bool();

	    it_artifact_armor* art_type = new it_artifact_armor(
		id, price, name, description, sym, color, m1, m2, volume,
		weight, melee_dam, melee_cut, m_to_hit, item_tags,

		covers, encumber, coverage, thickness, env_resist, warmth,
		storage);
	    art_type->power_armor = power_armor;

	    catajson effects_worn_json =
		artifact.get(std::string("effects_worn"));
	    effects_worn_json.set_begin();
	    std::vector<art_effect_passive> effects_worn;
	    while (effects_worn_json.has_curr())
	    {
		art_effect_passive effect =
		    (art_effect_passive)effects_worn_json.curr().as_int();
		effects_worn.push_back(effect);
		effects_worn_json.next();
	    }
	    art_type->effects_worn = effects_worn;

	    itypes[id] = art_type;
	}

	artifact_list.next();
    }

    if(!json_good())
    	uquit = QUIT_ERROR;
}

void game::load_weather(std::ifstream &fin)
{
    int tmpnextweather, tmpweather, tmptemp, num_segments;
    weather_segment new_segment;

    fin >> num_segments >> tmpnextweather >> tmpweather >> tmptemp;

    weather = weather_type(tmpweather);
    temperature = tmptemp;
    nextweather = tmpnextweather;

    for( int i = 0; i < num_segments - 1; ++i)
    {
        fin >> tmpnextweather >> tmpweather >> tmptemp;
        new_segment.weather = weather_type(tmpweather);
        new_segment.temperature = tmptemp;
        new_segment.deadline = tmpnextweather;
        future_weather.push_back(new_segment);
    }
}

void game::load(std::string name)
{
 std::ifstream fin;
 std::stringstream playerfile;
 playerfile << "save/" << name << ".sav";
 fin.open(playerfile.str().c_str());
// First, read in basic game state information.
 if (!fin.is_open()) {
  dbg(D_ERROR) << "game:load: No save game exists!";
  debugmsg("No save game exists!");
  return;
 }
 u = player();
 u.name = base64_decode(name);
 u.ret_null = item(itypes["null"], 0);
 u.weapon = item(itypes["null"], 0);
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
 z.clear();
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

  z.push_back(montmp);
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
   if (item_place == 'I') {
       tmpinv.push_back(item(itemdata, this));
   } else if (item_place == 'C') {
       tmpinv.back().contents.push_back(item(itemdata, this));
   } else if (item_place == 'W') {
       u.worn.push_back(item(itemdata, this));
   } else if (item_place == 'S') {
       u.worn.back().contents.push_back(item(itemdata, this));
   } else if (item_place == 'w') {
       u.weapon = item(itemdata, this);
   } else if (item_place == 'c') {
       u.weapon.contents.push_back(item(itemdata, this));
   }
  }
 }
// Now dump tmpinv into the player's inventory
 u.inv.add_stack(tmpinv);
 fin.close();
 load_auto_pickup(true); // Load character auto pickup rules
 load_uistate();
// Now load up the master game data; factions (and more?)
 load_master();
 update_map(u.posx, u.posy);
 set_adjacent_overmaps(true);
 MAPBUFFER.set_dirty();
 draw();
}

//Saves all factions and missions and npcs.
//Requires a valid std:stringstream masterfile to save the
void game::save_factions_missions_npcs ()
{
	std::stringstream masterfile;
	std::ofstream fout;
    masterfile << "save/master.gsav";

    fout.open(masterfile.str().c_str());

    fout << next_mission_id << " " << next_faction_id << " " << next_npc_id <<
        " " << active_missions.size() << " ";
    for (int i = 0; i < active_missions.size(); i++)
        fout << active_missions[i].save_info() << " ";

    fout << factions.size() << std::endl;
    for (int i = 0; i < factions.size(); i++)
        fout << factions[i].save_info() << std::endl;

    fout.close();
}

void game::save_artifacts()
{
    std::ofstream fout;
    std::vector<picojson::value> artifacts;
    fout.open("save/artifacts.gsav");
    for ( std::vector<std::string>::iterator it =
	      artifact_itype_ids.begin();
	  it != artifact_itype_ids.end(); ++it)
    {
	artifacts.push_back(itypes[*it]->save_data());
    }
    picojson::value out = picojson::value(artifacts);
    fout << out.serialize();
    fout.close();
}

void game::save_maps()
{
    m.save(cur_om, turn, levx, levy, levz);
    overmap_buffer.save();
    MAPBUFFER.save();
}

void game::save_uistate() {
    const std::string savedir="save";
    std::stringstream savefile;
    savefile << savedir << "/uistate.json";
    std::ofstream fout;
    fout.open(savefile.str().c_str());
    fout << uistate.save();
    fout.close();
    uistate.errdump="";
}

std::string game::save_weather() const
{
    std::stringstream weather_string;
    weather_string << future_weather.size() + 1 << " ";
    weather_string << int(nextweather) << " " << weather << " " << int(temperature) << " ";
    for( std::list<weather_segment>::const_iterator current_weather = future_weather.begin();
         current_weather != future_weather.end(); ++current_weather )
    {
        weather_string << int(current_weather->deadline) << " ";
        weather_string << current_weather->weather << " ";
        weather_string << int(current_weather->temperature) << " ";
    }
    return weather_string.str();
}

void game::save()
{
 std::stringstream playerfile;
 std::ofstream fout;
 playerfile << "save/" << base64_encode(u.name) << ".sav";

 fout.open(playerfile.str().c_str());
 // First, write out basic game state information.
 fout << int(turn) << " " << int(last_target) << " " << int(run_mode) << " " <<
         mostseen << " " << nextinv << " " << next_npc_id << " " <<
     next_faction_id << " " << next_mission_id << " " << int(nextspawn) << " ";

 fout << save_weather();

 fout << levx << " " << levy << " " << levz << " " << cur_om->pos().x <<
         " " << cur_om->pos().y << " " << std::endl;
 // Next, the scent map.
 for (int i = 0; i < SEEX * MAPSIZE; i++) {
  for (int j = 0; j < SEEY * MAPSIZE; j++)
   fout << grscent[i][j] << " ";
 }
 // Now save all monsters.
 fout << std::endl << z.size() << std::endl;
 for (int i = 0; i < z.size(); i++) {
     fout << z[i].save_info() << std::endl;
     fout << z[i].inv.size() << std::endl;
     for( std::vector<item>::iterator it = z[i].inv.begin(); it != z[i].inv.end(); ++it )
     {
         fout << it->save_info() << std::endl;
     }
 }
 for (int i = 0; i < num_monsters; i++)	// Save the kill counts, too.
  fout << kills[i] << " ";
 // And finally the player.
 fout << u.save_info() << std::endl;
 fout << std::endl;
 fout.close();
 //factions, missions, and npcs, maps and artifact data is saved in cleanup_at_end()
 save_auto_pickup(true); // Save character auto pickup rules
 save_uistate();
}

void game::delete_save()
{
#if (defined _WIN32 || defined __WIN32__)
      WIN32_FIND_DATA FindFileData;
      HANDLE hFind;
      TCHAR Buffer[MAX_PATH];

      GetCurrentDirectory(MAX_PATH, Buffer);
      SetCurrentDirectory("save");
      hFind = FindFirstFile("*", &FindFileData);
      if(INVALID_HANDLE_VALUE != hFind) {
       do {
        DeleteFile(FindFileData.cFileName);
       } while(FindNextFile(hFind, &FindFileData) != 0);
       FindClose(hFind);
      }
      SetCurrentDirectory(Buffer);
#else
     DIR *save_dir = opendir("save");
     struct dirent *save_dirent = NULL;
     if(save_dir != NULL && 0 == chdir("save"))
     {
      while ((save_dirent = readdir(save_dir)) != NULL)
       (void)unlink(save_dirent->d_name);
      (void)chdir("..");
      (void)closedir(save_dir);
     }
#endif
}

/**
 * Writes information about the character out to a text file timestamped with
 * the time of the file was made. This serves as a record of the character's
 * state at the time the memorial was made (usually upon death) and
 * accomplishments in a human-readable format.
 */
void game::write_memorial_file() {

    //Open the file first
    DIR *dir = opendir("memorial");
    if (!dir) {
        #if (defined _WIN32 || defined __WIN32__)
            mkdir("memorial");
        #else
            mkdir("memorial", 0777);
        #endif
        dir = opendir("memorial");
        if (!dir) {
            dbg(D_ERROR) << "game:write_memorial_file: Unable to make memorial directory.";
            debugmsg("Could not make './memorial' directory");
            return;
        }
    }

    //To ensure unique filenames and to sort files, append a timestamp
    time_t rawtime;
    time (&rawtime);
    std::string timestamp = ctime(&rawtime);

    //Fun fact: ctime puts a \n at the end of the timestamp. Get rid of it.
    size_t end = timestamp.find_last_of('\n');
    timestamp = timestamp.substr(0, end);

    //Colons are not usable in paths, so get rid of them
    for(int index = 0; index < timestamp.size(); index++) {
        if(timestamp[index] == ':') {
            timestamp[index] = '-';
        }
    }

    std::string memorial_file_path = string_format("memorial/%s-%s.txt",
            u.name.c_str(), timestamp.c_str());

    std::ofstream memorial_file;
    memorial_file.open(memorial_file_path.c_str());

    u.memorial( memorial_file );

    if(!memorial_file.is_open()) {
      dbg(D_ERROR) << "game:write_memorial_file: Unable to open " << memorial_file_path;
      debugmsg("Could not open memorial file '%s'", memorial_file_path.c_str());
    }


    //Cleanup
    memorial_file.close();

}

void game::advance_nextinv()
{
  if (nextinv == inv_chars.end()[-1])
    nextinv = inv_chars.begin()[0];
  else
    nextinv = inv_chars[inv_chars.find(nextinv) + 1];
}

void game::decrease_nextinv()
{
  if (nextinv == inv_chars.begin()[0])
    nextinv = inv_chars.end()[-1];
  else
    nextinv = inv_chars[inv_chars.find(nextinv) - 1];
}

void game::vadd_msg(const char* msg, va_list ap)
{
 char buff[1024];
 vsprintf(buff, msg, ap);
 std::string s(buff);
 add_msg_string(s);
}

void game::add_msg_string(const std::string &s)
{
 if (s.length() == 0)
  return;
 if (!messages.empty() && int(messages.back().turn) + 3 >= int(turn) &&
     s == messages.back().message) {
  messages.back().count++;
  messages.back().turn = turn;
  return;
 }

 if (messages.size() == 256)
  messages.erase(messages.begin());
 messages.push_back( game_message(turn, s) );
}

void game::add_msg(const char* msg, ...)
{
 va_list ap;
 va_start(ap, msg);
 vadd_msg(msg, ap);
 va_end(ap);
}

void game::add_msg_if_player(player *p, const char* msg, ...)
{
 if (p && !p->is_npc())
 {
  va_list ap;
  va_start(ap, msg);
  vadd_msg(msg, ap);
  va_end(ap);
 }
}

void game::add_msg_player_or_npc(player *p, const char* player_str, const char* npc_str, ...)
{
    va_list ap;
    if( !p ) {return; }

    va_start( ap, npc_str );

    if( !p->is_npc() ) {
        vadd_msg( player_str, ap );
    } else if( u_see( p ) ) {
        char buff[1024];
        vsprintf(buff, npc_str, ap);
        std::string processed_npc_string(buff);
        // These strings contain the substring <npcname>,
        // if present replace it with the actual npc name.
        size_t offset = processed_npc_string.find("<npcname>");
        if( offset != std::string::npos ) {
            processed_npc_string.replace(offset, sizeof("<npcname>"),  p->name);
        }
        add_msg_string( processed_npc_string );
    }

    va_end(ap);
}

std::vector<game_message> game::recent_messages(int message_count)
{
  std::vector<game_message> backlog;
  for(int i = messages.size() - 1; i > 0 && message_count > 0; i--) {
    backlog.push_back(messages[i]);
    message_count--;
  }
  return backlog;
}

void game::add_event(event_type type, int on_turn, int faction_id, int x, int y)
{
 event tmp(type, on_turn, faction_id, x, y);
 events.push_back(tmp);
}

bool game::event_queued(event_type type)
{
 for (int i = 0; i < events.size(); i++) {
  if (events[i].type == type)
   return true;
  }
  return false;
}

void game::debug()
{
 int action = menu(true, // cancelable
                   _("Debug Functions - Using these is CHEATING!"),
                   _("Wish for an item"),       // 1
                   _("Teleport - Short Range"), // 2
                   _("Teleport - Long Range"),  // 3
                   _("Reveal map"),             // 4
                   _("Spawn NPC"),              // 5
                   _("Spawn Monster"),          // 6
                   _("Check game state..."),    // 7
                   _("Kill NPCs"),              // 8
                   _("Mutate"),                 // 9
                   _("Spawn a vehicle"),        // 10
                   _("Increase all skills"),    // 11
                   _("Learn all melee styles"), // 12
                   _("Check NPC"),              // 13
                   _("Spawn Artifact"),         // 14
                   _("Spawn Clarivoyance Artifact"), //15
                   _("Map editor"), // 16
                   _("Change weather"),         // 17
                   _("Cancel"),                 // 18
                   NULL);
 int veh_num;
 std::vector<std::string> opts;
 switch (action) {
  case 1:
   wish();
   break;

  case 2:
   teleport();
   break;

  case 3: {
        point tmp = cur_om->draw_overmap(this, levz);
        if (tmp.x != -1)
        {
            //First offload the active npcs.
            for (int i = 0; i < active_npc.size(); i++)
            {
                active_npc[i]->omx = cur_om->pos().x;
                active_npc[i]->omy = cur_om->pos().y;
                active_npc[i]->mapx = levx + (active_npc[i]->posx / SEEX);
                active_npc[i]->mapy = levy + (active_npc[i]->posy / SEEY);
                active_npc[i]->posx %= SEEX;
                active_npc[i]->posy %= SEEY;
            }
            active_npc.clear();
            z.clear();
            levx = tmp.x * 2 - int(MAPSIZE / 2);
            levy = tmp.y * 2 - int(MAPSIZE / 2);
            set_adjacent_overmaps(true);
            m.load(this, levx, levy, levz);
            load_npcs();
            m.spawn_monsters(this);	// Static monsters
        }
    } break;
  case 4:
   debugmsg("%d radio towers", cur_om->radios.size());
   for (int i = 0; i < OMAPX; i++) {
       for (int j = 0; j < OMAPY; j++) {
           for (int k = -OVERMAP_DEPTH; k <= OVERMAP_HEIGHT; k++)
           {
               cur_om->seen(i, j, k) = true;
           }
       }
   }
   add_msg(_("Current overmap revealed."));
   break;

  case 5: {
   npc * temp = new npc();
   temp->normalize(this);
   temp->randomize(this);
   //temp.attitude = NPCATT_TALK; //not needed
   temp->spawn_at(cur_om, levx, levy, levz);
   temp->place_near(this, u.posx - 4, u.posy - 4);
   temp->form_opinion(&u);
   //temp.attitude = NPCATT_TALK;//The newly spawned npc always wants to talk. Disabled as form opinion sets the attitude.
   temp->mission = NPC_MISSION_NULL;
   int mission_index = reserve_random_mission(ORIGIN_ANY_NPC,
                                              om_location(), temp->getID());
   if (mission_index != -1)
   temp->chatbin.missions.push_back(mission_index);
   active_npc.push_back(temp);
  } break;

  case 6:
   monster_wish();
   break;

  case 7:
   popup_top(_("\
Location %d:%d in %d:%d, %s\n\
Current turn: %d; Next spawn %d.\n\
%s\n\
%d monsters exist.\n\
%d currently active NPC's.\n\
%d events planned."),
             u.posx, u.posy, levx, levy,
             oterlist[cur_om->ter(levx / 2, levy / 2, levz)].name.c_str(),
             int(turn), int(nextspawn), (!OPTIONS["RANDOM_NPC"] ? _("NPCs are going to spawn.") :
                                         _("NPCs are NOT going to spawn.")),
             z.size(), active_npc.size(), events.size());
		 if( !active_npc.empty() ) {
       for (int i = 0; i < active_npc.size(); i++) {
           add_msg(_("%s: map (%d:%d) pos (%d:%d)"),
                   active_npc[i]->name.c_str(), active_npc[i]->mapx, active_npc[i]->mapy,
                   active_npc[i]->posx, active_npc[i]->posy);
       }
       add_msg(_("(you: %d:%d)"), u.posx, u.posy);
			}
   break;

  case 8:
   for (int i = 0; i < active_npc.size(); i++) {
    add_msg(_("%s's head implodes!"), active_npc[i]->name.c_str());
    active_npc[i]->hp_cur[bp_head] = 0;
   }
   break;

  case 9:
   mutation_wish();
   break;

  case 10:
   if (m.veh_at(u.posx, u.posy)) {
    dbg(D_ERROR) << "game:load: There's already vehicle here";
    debugmsg ("There's already vehicle here");
   }
   else {
    for (int i = 2; i < vtypes.size(); i++)
     opts.push_back (vtypes[i]->name);
    opts.push_back (std::string(_("Cancel")));
    veh_num = menu_vec (false, _("Choose vehicle to spawn"), opts) + 1;
    if (veh_num > 1 && veh_num < num_vehicles)
     m.add_vehicle (this, (vhtype_id)veh_num, u.posx, u.posy, -90, 100, 0);
     m.board_vehicle (this, u.posx, u.posy, &u);
   }
   break;

  case 11: {
      const int skoffset = 1;
      uimenu skmenu;
      skmenu.text = "Select a skill to modify";
      skmenu.return_invalid = true;
      skmenu.addentry(0, true, '1', "Set all skills to...");
      int origskills[ Skill::skills.size()] ;

      for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin();
           aSkill != Skill::skills.end(); ++aSkill) {
        int skill_id = (*aSkill)->id();
        skmenu.addentry( skill_id + skoffset, true, -1, "@ %d: %s  ",
                         (int)u.skillLevel(*aSkill), (*aSkill)->ident().c_str() );
        origskills[skill_id] = (int)u.skillLevel(*aSkill);
      }
      do {
        skmenu.query();
        int skill_id = -1;
        int skset = -1;
        int sksel = skmenu.selected - skoffset;
        if ( skmenu.ret == -1 && ( skmenu.keypress == KEY_LEFT || skmenu.keypress == KEY_RIGHT ) ) {
          if ( sksel >= 0 && sksel < Skill::skills.size() ) {
            skill_id = sksel;
            skset = (int)u.skillLevel( Skill::skills[skill_id]) +
                ( skmenu.keypress == KEY_LEFT ? -1 : 1 );
          }
        } else if ( skmenu.selected == skmenu.ret &&  sksel >= 0 && sksel < Skill::skills.size() ) {
          skill_id = sksel;
          uimenu sksetmenu;
          sksetmenu.w_x = skmenu.w_x + skmenu.w_width + 1;
          sksetmenu.w_y = skmenu.w_y + 2;
          sksetmenu.w_height = skmenu.w_height - 4;
          sksetmenu.return_invalid = true;
          sksetmenu.settext( "Set '%s' to..", Skill::skills[skill_id]->ident().c_str() );
          int skcur = (int)u.skillLevel(Skill::skills[skill_id]);
          sksetmenu.selected = skcur;
          for ( int i = 0; i < 21; i++ ) {
              sksetmenu.addentry( i, true, i + 48, "%d%s", i, (skcur == i ? " (current)" : "") );
          }
          sksetmenu.query();
          skset = sksetmenu.ret;
        }
        if ( skset != -1 && skill_id != -1 ) {
          u.skillLevel( Skill::skills[skill_id] ).level(skset);
          skmenu.textformatted[0] = string_format("%s set to %d             ",
              Skill::skills[skill_id]->ident().c_str(),
              (int)u.skillLevel(Skill::skills[skill_id])).substr(0,skmenu.w_width - 4);
          skmenu.entries[skill_id + skoffset].txt = string_format("@ %d: %s  ",
              (int)u.skillLevel( Skill::skills[skill_id]), Skill::skills[skill_id]->ident().c_str() );
          skmenu.entries[skill_id + skoffset].text_color =
              ( (int)u.skillLevel(Skill::skills[skill_id]) == origskills[skill_id] ?
                skmenu.text_color : c_yellow );
        } else if ( skmenu.ret == 0 && sksel == -1 ) {
          int ret = menu(true, "Set all skills...",
                         "+3","+1","-1","-3","To 0","To 5","To 10","(Reset changes)",NULL);
          if ( ret > 0 ) {
              int skmod = 0;
              int skset = -1;
              if (ret < 5 ) {
                skmod=( ret < 3 ? ( ret == 1 ? 3 : 1 ) :
                    ( ret == 3 ? -1 : -3 )
                );
              } else if ( ret < 8 ) {
                skset=( ( ret - 5 ) * 5 );
              }
              for (int skill_id = 0; skill_id < Skill::skills.size(); skill_id++ ) {
                int changeto = ( skmod != 0 ? u.skillLevel( Skill::skills[skill_id] ) + skmod :
                                 ( skset != -1 ? skset : origskills[skill_id] ) );
                u.skillLevel( Skill::skills[skill_id] ).level( changeto );
                skmenu.entries[skill_id + skoffset].txt =
                    string_format("@ %d: %s  ", (int)u.skillLevel(Skill::skills[skill_id]),
                                  Skill::skills[skill_id]->ident().c_str() );
                skmenu.entries[skill_id + skoffset].text_color =
                    ( (int)u.skillLevel(Skill::skills[skill_id]) == origskills[skill_id] ?
                      skmenu.text_color : c_yellow );
              }
          }
        }
      } while ( ! ( skmenu.ret == -1 && ( skmenu.keypress == 'q' || skmenu.keypress == ' ' ||
                                          skmenu.keypress == KEY_ESCAPE ) ) );
    }
    break;

  case 12:
    for(std::vector<std::string>::iterator it = martial_arts_itype_ids.begin();
          it != martial_arts_itype_ids.end(); ++it){
        u.styles.push_back(*it);
    }
    add_msg(_("Martial arts gained."));
   break;

  case 13: {
   point pos = look_around();
   int npcdex = npc_at(pos.x, pos.y);
   if (npcdex == -1)
    popup(_("No NPC there."));
   else {
    std::stringstream data;
    npc *p = active_npc[npcdex];
    data << p->name << " " << (p->male ? _("Male") : _("Female")) << std::endl;
    data << npc_class_name(p->myclass) << "; " <<
            npc_attitude_name(p->attitude) << std::endl;
    if (p->has_destination())
     data << _("Destination: ") << p->goalx << ":" << p->goaly << "(" <<
             oterlist[ cur_om->ter(p->goalx, p->goaly, p->goalz) ].name << ")" <<
             std::endl;
    else
     data << _("No destination.") << std::endl;
    data << _("Trust: ") << p->op_of_u.trust << _(" Fear: ") << p->op_of_u.fear <<
            _(" Value: ") << p->op_of_u.value << _(" Anger: ") << p->op_of_u.anger <<
            _(" Owed: ") << p->op_of_u.owed << std::endl;
    data << _("Aggression: ") << int(p->personality.aggression) << _(" Bravery: ") <<
            int(p->personality.bravery) << _(" Collector: ") <<
            int(p->personality.collector) << _(" Altruism: ") <<
            int(p->personality.altruism) << std::endl;
    for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin(); aSkill != Skill::skills.end(); ++aSkill) {
      data << (*aSkill)->name() << ": " << p->skillLevel(*aSkill) << std::endl;
    }

    full_screen_popup(data.str().c_str());
   }
  } break;

  case 14:
  {
   point center = look_around();
   artifact_natural_property prop =
    artifact_natural_property(rng(ARTPROP_NULL + 1, ARTPROP_MAX - 1));
   m.create_anomaly(center.x, center.y, prop);
   m.spawn_artifact(center.x, center.y, new_natural_artifact(prop), 0);
  }
  break;

  case 15:
  {
      std::string artifact_name(std::string type);

      it_artifact_tool *art = new it_artifact_tool();
      artifact_tool_form_datum *info = &(artifact_tool_form_data[ARTTOOLFORM_CUBE]);
      art->name = artifact_name(info->name);
      art->color = info->color;
      art->sym = info->sym;
      art->m1 = info->m1;
      art->m2 = info->m2;
      art->volume = rng(info->volume_min, info->volume_max);
      art->weight = rng(info->weight_min, info->weight_max);
      // Set up the basic weapon type
      artifact_weapon_datum *weapon = &(artifact_weapon_data[info->base_weapon]);
      art->melee_dam = rng(weapon->bash_min, weapon->bash_max);
      art->melee_cut = rng(weapon->cut_min, weapon->cut_max);
      art->m_to_hit = rng(weapon->to_hit_min, weapon->to_hit_max);
      if( weapon->tag != "" ) {
          art->item_tags.insert(weapon->tag);
      }
      // Add an extra weapon perhaps?
      art->description = _("The architect's cube.");
      art->effects_carried.push_back(AEP_SUPER_CLAIRVOYANCE);
      art->id = itypes.size();
      itypes[art->name] = art;

      item artifact( art, 0);
      u.i_add(artifact);
  }
  break;

  case 16: {
      point coord = look_debug();
  }
  break;

  case 17: {
      const int weather_offset = 1;
      uimenu weather_menu;
      weather_menu.text = "Select new weather pattern:";
      weather_menu.return_invalid = true;
      for(int weather_id = 1; weather_id < NUM_WEATHER_TYPES; weather_id++) {

        weather_menu.addentry(weather_id + weather_offset, true, -1, weather_data[weather_id].name);

      }

      weather_menu.query();

      if(weather_menu.ret > 0) {
        add_msg("%d", weather_menu.selected);

        int selected_weather = weather_menu.selected + 1;
        weather = (weather_type) selected_weather;

      }
  }
  break;
 }
 erase();
 refresh_all();
}

void game::mondebug()
{
 int tc;
 for (int i = 0; i < z.size(); i++) {
  z[i].debug(u);
  if (z[i].has_flag(MF_SEES) &&
      m.sees(z[i].posx, z[i].posy, u.posx, u.posy, -1, tc))
   debugmsg("The %s can see you.", z[i].name().c_str());
  else
   debugmsg("The %s can't see you...", z[i].name().c_str());
 }
}

void game::groupdebug()
{
 erase();
 mvprintw(0, 0, "OM %d : %d    M %d : %d", cur_om->pos().x, cur_om->pos().y, levx,
                                           levy);
 int dist, linenum = 1;
 for (int i = 0; i < cur_om->zg.size(); i++) {
 	if (cur_om->zg[i].posz != levz) { continue; }
  dist = trig_dist(levx, levy, cur_om->zg[i].posx, cur_om->zg[i].posy);
  if (dist <= cur_om->zg[i].radius) {
   mvprintw(linenum, 0, "Zgroup %d: Centered at %d:%d, radius %d, pop %d",
            i, cur_om->zg[i].posx, cur_om->zg[i].posy, cur_om->zg[i].radius,
            cur_om->zg[i].population);
   linenum++;
  }
 }
 getch();
}

void game::draw_overmap()
{
 cur_om->draw_overmap(this, levz);
}

void game::disp_kills()
{
 WINDOW *w = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                    (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0,
                    (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0);

 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );

 std::vector<mtype *> types;
 std::vector<int> count;
 for (int i = 0; i < num_monsters; i++) {
  if (kills[i] > 0) {
   types.push_back(mtypes[i]);
   count.push_back(kills[i]);
  }
 }

 mvwprintz(w, 1, 32, c_white, "KILL COUNT:");

 if (types.size() == 0) {
  mvwprintz(w, 2, 2, c_white, _("You haven't killed any monsters yet!"));
  wrefresh(w);
  getch();
  werase(w);
  wrefresh(w);
  delwin(w);
  refresh_all();
  return;
 }
 int totalkills = 0;
 int hori = 1;
 int horimove = 0;
 int vert = -2;
 // display individual kill counts
 for (int i = 0; i < types.size(); i++) {
  hori = 1;
  if (i > 21) {
   hori = 28;
   vert = 20;
  }
  if( i > 43) {
   hori = 56;
   vert = 42;
  }
  mvwprintz(w, i - vert, hori, types[i]->color, "%c %s", types[i]->sym, types[i]->name.c_str());
  if (count[i] >= 10)
   horimove = -1;
  if (count[i] >= 100)
   horimove = -2;
  if (count[i] >= 1000)
   horimove = -3;
  mvwprintz(w, i - vert, hori + 22 + horimove, c_white, "%d", count[i]);
  totalkills += count[i];
  horimove = 0;
 }
 // Display total killcount at top of window
 mvwprintz(w, 1, 44, c_white, "%d", totalkills);

 wrefresh(w);
 getch();
 werase(w);
 wrefresh(w);
 delwin(w);
 refresh_all();
}

void game::disp_NPCs()
{
 WINDOW *w = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                    (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0,
                    (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0);

 mvwprintz(w, 0, 0, c_white, _("Your position: %d:%d"), levx, levy);
 std::vector<npc*> closest;
 closest.push_back(cur_om->npcs[0]);
 for (int i = 1; i < cur_om->npcs.size(); i++) {
  if (closest.size() < 20)
   closest.push_back(cur_om->npcs[i]);
  else if (rl_dist(levx, levy, cur_om->npcs[i]->mapx, cur_om->npcs[i]->mapy) <
           rl_dist(levx, levy, closest[19]->mapx, closest[19]->mapy)) {
   for (int j = 0; j < 20; j++) {
    if (rl_dist(levx, levy, closest[j]->mapx, closest[j]->mapy) >
        rl_dist(levx, levy, cur_om->npcs[i]->mapx, cur_om->npcs[i]->mapy)) {
     closest.insert(closest.begin() + j, cur_om->npcs[i]);
     closest.erase(closest.end() - 1);
     j = 20;
    }
   }
  }
 }
 for (int i = 0; i < 20; i++)
  mvwprintz(w, i + 2, 0, c_white, "%s: %d:%d", closest[i]->name.c_str(),
            closest[i]->mapx, closest[i]->mapy);

 wrefresh(w);
 getch();
 werase(w);
 wrefresh(w);
 delwin(w);
}

faction* game::list_factions(std::string title)
{
 std::vector<faction> valfac;	// Factions that we know of.
 for (int i = 0; i < factions.size(); i++) {
  if (factions[i].known_by_u)
   valfac.push_back(factions[i]);
 }
 if (valfac.size() == 0) {	// We don't know of any factions!
  popup(_("You don't know of any factions.  Press Spacebar..."));
  return NULL;
 }

 WINDOW *w_list = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                         ((TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0),
                         (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0);
 WINDOW *w_info = newwin(FULL_SCREEN_HEIGHT-2, FULL_SCREEN_WIDTH-1 - MAX_FAC_NAME_SIZE,
                         1 + ((TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0),
                         MAX_FAC_NAME_SIZE + ((TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0));

 wborder(w_list, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                 LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );

 int maxlength = FULL_SCREEN_WIDTH - 1 - MAX_FAC_NAME_SIZE;
 int sel = 0;

// Init w_list content
 mvwprintz(w_list, 1, 1, c_white, title.c_str());
 for (int i = 0; i < valfac.size(); i++) {
  nc_color col = (i == 0 ? h_white : c_white);
  mvwprintz(w_list, i + 2, 1, col, valfac[i].name.c_str());
 }
 wrefresh(w_list);
// Init w_info content
// fac_*_text() is in faction.cpp
 mvwprintz(w_info, 0, 0, c_white,
          _("Ranking: %s"), fac_ranking_text(valfac[0].likes_u).c_str());
 mvwprintz(w_info, 1, 0, c_white,
          _("Respect: %s"), fac_respect_text(valfac[0].respects_u).c_str());
 fold_and_print(w_info, 3, 0, maxlength, c_white, valfac[0].describe().c_str());
 wrefresh(w_info);
 InputEvent input;
 do {
  input = get_input();
  switch ( input ) {
  case DirectionS:	// Move selection down
   mvwprintz(w_list, sel + 2, 1, c_white, valfac[sel].name.c_str());
   if (sel == valfac.size() - 1)
    sel = 0;	// Wrap around
   else
    sel++;
   break;
  case DirectionN:	// Move selection up
   mvwprintz(w_list, sel + 2, 1, c_white, valfac[sel].name.c_str());
   if (sel == 0)
    sel = valfac.size() - 1;	// Wrap around
   else
    sel--;
   break;
  case Cancel:
  case Close:
   sel = -1;
   break;
  }
  if (input == DirectionS || input == DirectionN) {	// Changed our selection... update the windows
   mvwprintz(w_list, sel + 2, 1, h_white, valfac[sel].name.c_str());
   wrefresh(w_list);
   werase(w_info);
// fac_*_text() is in faction.cpp
   mvwprintz(w_info, 0, 0, c_white,
            _("Ranking: %s"), fac_ranking_text(valfac[sel].likes_u).c_str());
   mvwprintz(w_info, 1, 0, c_white,
            _("Respect: %s"), fac_respect_text(valfac[sel].respects_u).c_str());
   fold_and_print(w_info, 3, 0, maxlength, c_white, valfac[sel].describe().c_str());
   wrefresh(w_info);
  }
 } while (input != Cancel && input != Confirm && input != Close);
 werase(w_list);
 werase(w_info);
 delwin(w_list);
 delwin(w_info);
 refresh_all();
 if (sel == -1)
  return NULL;
 return &(factions[valfac[sel].id]);
}

void game::list_missions()
{
 WINDOW *w_missions = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                              (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0,
                              (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0);

 int tab = 0, selection = 0;
 InputEvent input;
 do {
  werase(w_missions);
  //draw_tabs(w_missions, tab, "ACTIVE MISSIONS", "COMPLETED MISSIONS", "FAILED MISSIONS", NULL);
  std::vector<int> umissions;
  switch (tab) {
   case 0: umissions = u.active_missions;	break;
   case 1: umissions = u.completed_missions;	break;
   case 2: umissions = u.failed_missions;	break;
  }

  for (int i = 1; i < FULL_SCREEN_WIDTH-1; i++) {
   mvwputch(w_missions, 2, i, c_ltgray, LINE_OXOX);
   mvwputch(w_missions, FULL_SCREEN_HEIGHT-1, i, c_ltgray, LINE_OXOX);

   if (i > 2 && i < FULL_SCREEN_HEIGHT-1) {
    mvwputch(w_missions, i, 0, c_ltgray, LINE_XOXO);
    mvwputch(w_missions, i, 30, c_ltgray, LINE_XOXO);
    mvwputch(w_missions, i, FULL_SCREEN_WIDTH-1, c_ltgray, LINE_XOXO);
   }
  }

  draw_tab(w_missions, 7, _("ACTIVE MISSIONS"), (tab == 0) ? true : false);
  draw_tab(w_missions, 30, _("COMPLETED MISSIONS"), (tab == 1) ? true : false);
  draw_tab(w_missions, 56, "FAILED MISSIONS", (tab == 2) ? true : false);

  mvwputch(w_missions, 2,  0, c_white, LINE_OXXO); // |^
  mvwputch(w_missions, 2, FULL_SCREEN_WIDTH-1, c_white, LINE_OOXX); // ^|

  mvwputch(w_missions, FULL_SCREEN_HEIGHT-1, 0, c_ltgray, LINE_XXOO); // |
  mvwputch(w_missions, FULL_SCREEN_HEIGHT-1, FULL_SCREEN_WIDTH-1, c_ltgray, LINE_XOOX); // _|

  mvwputch(w_missions, 2, 30, c_white, (tab == 1) ? LINE_XOXX : LINE_XXXX); // + || -|
  mvwputch(w_missions, FULL_SCREEN_HEIGHT-1, 30, c_white, LINE_XXOX); // _|_

  for (int i = 0; i < umissions.size(); i++) {
   mission *miss = find_mission(umissions[i]);
   nc_color col = c_white;
   if (i == u.active_mission && tab == 0)
    col = c_ltred;
   if (selection == i)
    mvwprintz(w_missions, 3 + i, 1, hilite(col), miss->name().c_str());
   else
    mvwprintz(w_missions, 3 + i, 1, col, miss->name().c_str());
  }

  if (selection >= 0 && selection < umissions.size()) {
   mission *miss = find_mission(umissions[selection]);
   mvwprintz(w_missions, 4, 31, c_white,
             miss->description.c_str());
   if (miss->deadline != 0)
    mvwprintz(w_missions, 5, 31, c_white, _("Deadline: %d (%d)"),
              miss->deadline, int(turn));
   mvwprintz(w_missions, 6, 31, c_white, _("Target: (%d, %d)   You: (%d, %d)"),
             miss->target.x, miss->target.y,
             (levx + int (MAPSIZE / 2)) / 2, (levy + int (MAPSIZE / 2)) / 2);
  } else {
   std::string nope;
   switch (tab) {
    case 0: nope = _("You have no active missions!"); break;
    case 1: nope = _("You haven't completed any missions!"); break;
    case 2: nope = _("You haven't failed any missions!"); break;
   }
   mvwprintz(w_missions, 4, 31, c_ltred, nope.c_str());
  }

  wrefresh(w_missions);
  input = get_input();
  switch (input) {
  case DirectionE:
   tab++;
   if (tab == 3)
    tab = 0;
   break;
  case DirectionW:
   tab--;
   if (tab < 0)
    tab = 2;
   break;
  case DirectionS:
   selection++;
   if (selection >= umissions.size())
    selection = 0;
   break;
  case DirectionN:
   selection--;
   if (selection < 0)
    selection = umissions.size() - 1;
   break;
  case Confirm:
   u.active_mission = selection;
   break;
  }

 } while (input != Cancel && input != Close);


 werase(w_missions);
 delwin(w_missions);
 refresh_all();
}

void game::draw()
{
    // Draw map
    werase(w_terrain);
    draw_ter();
    draw_footsteps();

    // Draw Status
    draw_HP();
    werase(w_status);
    werase(w_status2);
    u.disp_status(w_status, w_status2, this);

    const int sideStyle = (int)(OPTIONS["SIDEBAR_STYLE"] == "Narrow");

    WINDOW *time_window = sideStyle ? w_status2 : w_status;
    wmove(time_window, sideStyle ? 0 : 1, sideStyle ? 15 : 41);
    if ( u.has_item_with_flag("WATCH") ) {
        wprintz(time_window, c_white, turn.print_time().c_str());
    } else {
        std::vector<std::pair<char, nc_color> > vGlyphs;
        vGlyphs.push_back(std::make_pair('_', c_red));
        vGlyphs.push_back(std::make_pair('_', c_cyan));
        vGlyphs.push_back(std::make_pair('.', c_brown));
        vGlyphs.push_back(std::make_pair(',', c_blue));
        vGlyphs.push_back(std::make_pair('+', c_yellow));
        vGlyphs.push_back(std::make_pair('c', c_ltblue));
        vGlyphs.push_back(std::make_pair('*', c_yellow));
        vGlyphs.push_back(std::make_pair('C', c_white));
        vGlyphs.push_back(std::make_pair('+', c_yellow));
        vGlyphs.push_back(std::make_pair('c', c_ltblue));
        vGlyphs.push_back(std::make_pair('.', c_brown));
        vGlyphs.push_back(std::make_pair(',', c_blue));
        vGlyphs.push_back(std::make_pair('_', c_red));
        vGlyphs.push_back(std::make_pair('_', c_cyan));

        const int iHour = turn.getHour();
        wprintz(time_window, c_white, "[");
        bool bAddTrail = false;

        for (int i=0; i < 14; i+=2) {
            if (iHour >= 8+i && iHour <= 13+(i/2)) {
                wputch(time_window, hilite(c_white), ' ');

            } else if (iHour >= 6+i && iHour <= 7+i) {
                wputch(time_window, hilite(vGlyphs[i].second), vGlyphs[i].first);
                bAddTrail = true;

            } else if (iHour >= (18+i)%24 && iHour <= (19+i)%24) {
                wputch(time_window, vGlyphs[i+1].second, vGlyphs[i+1].first);

            } else if (bAddTrail && iHour >= 6+(i/2)) {
                wputch(time_window, hilite(c_white), ' ');

            } else {
                wputch(time_window, c_white, ' ');
            }
        }

        wprintz(time_window, c_white, "]");
    }

    point cur_loc = om_location();
    oter_id cur_ter = cur_om->ter(cur_loc.x, cur_loc.y, levz);
    if (cur_ter == ot_null)
    {
        if (cur_loc.x >= OMAPX && cur_loc.y >= OMAPY)
            cur_ter = om_diag->ter(cur_loc.x - OMAPX, cur_loc.y - OMAPY, levz);
        else if (cur_loc.x >= OMAPX)
            cur_ter = om_hori->ter(cur_loc.x - OMAPX, cur_loc.y, levz);
        else if (cur_loc.y >= OMAPY)
            cur_ter = om_vert->ter(cur_loc.x, cur_loc.y - OMAPY, levz);
    }

    std::string tername = oterlist[cur_ter].name;
    werase(w_location);
    mvwprintz(w_location, 0,  0, oterlist[cur_ter].color, utf8_substr(tername, 0, 14).c_str());

    if (levz < 0) {
        mvwprintz(w_location, 0, 18, c_ltgray, _("Underground"));
    } else {
        mvwprintz(w_location, 0, 18, weather_data[weather].color, weather_data[weather].name.c_str());
    }

    nc_color col_temp = c_blue;
    if (temperature >= 90) {
        col_temp = c_red;
    } else if (temperature >= 75) {
        col_temp = c_yellow;
    } else if (temperature >= 60) {
        col_temp = c_ltgreen;
    } else if (temperature >= 50) {
        col_temp = c_cyan;
    } else if (temperature >  32) {
        col_temp = c_ltblue;
    }

    wprintz(w_location, col_temp, (std::string(" ") + print_temperature(temperature)).c_str());
    wrefresh(w_location);

    //Safemode coloring
    WINDOW *day_window = sideStyle ? w_status2 : w_status;
    mvwprintz(day_window, 0, sideStyle ? 0 : 41, c_white, _("%s, day %d"), _(season_name[turn.get_season()].c_str()), turn.days() + 1);
    if (run_mode != 0 || autosafemode != 0) {
        int iPercent = ((turnssincelastmon*100)/OPTIONS["AUTOSAFEMODETURNS"]);
        wmove(w_status, sideStyle ? 4 : 1, getmaxx(w_status) - 4);
        const char *letters[] = {"S", "A", "F", "E"};
        for (int i = 0; i < 4; i++) {
            nc_color c = (run_mode == 0 && iPercent < (i + 1) * 25) ? c_red : c_green;
            wprintz(w_status, c, letters[i]);
        }
    }
    wrefresh(w_status);
    wrefresh(w_status2);

    std::string *graffiti = m.graffiti_at(u.posx, u.posy).contents;
    if (graffiti) {
        add_msg(_("Written here: %s"), utf8_substr(*graffiti, 0, 40).c_str());
    }

    // Draw messages
    write_msg();
}

bool game::isBetween(int test, int down, int up)
{
	if(test>down && test<up) return true;
	else return false;
}

void game::draw_ter(int posx, int posy)
{
 mapRain.clear();
// posx/posy default to -999
 if (posx == -999)
  posx = u.posx + u.view_offset_x;
 if (posy == -999)
  posy = u.posy + u.view_offset_y;
 m.build_map_cache(this);
 m.draw(this, w_terrain, point(posx, posy));

 // Draw monsters
 int distx, disty;
 for (int i = 0; i < z.size(); i++) {
  disty = abs(z[i].posy - posy);
  distx = abs(z[i].posx - posx);
  if (distx <= VIEWX && disty <= VIEWY && u_see(&(z[i]))) {
   z[i].draw(w_terrain, posx, posy, false);
   mapRain[VIEWY + z[i].posy - posy][VIEWX + z[i].posx - posx] = false;
  } else if (z[i].has_flag(MF_WARM) && distx <= VIEWX && disty <= VIEWY &&
           (u.has_active_bionic("bio_infrared") || u.has_trait("INFRARED")) &&
             m.pl_sees(u.posx,u.posy,z[i].posx,z[i].posy,
                       u.sight_range(DAYLIGHT_LEVEL)))
   mvwputch(w_terrain, VIEWY + z[i].posy - posy, VIEWX + z[i].posx - posx,
            c_red, '?');
 }
 // Draw NPCs
 for (int i = 0; i < active_npc.size(); i++) {
  disty = abs(active_npc[i]->posy - posy);
  distx = abs(active_npc[i]->posx - posx);
  if (distx <= VIEWX && disty <= VIEWY &&
      u_see(active_npc[i]->posx, active_npc[i]->posy))
   active_npc[i]->draw(w_terrain, posx, posy, false);
 }
 if (u.has_active_bionic("bio_scent_vision")) {
  for (int realx = posx - VIEWX; realx <= posx + VIEWX; realx++) {
   for (int realy = posy - VIEWY; realy <= posy + VIEWY; realy++) {
    if (scent(realx, realy) != 0) {
     int tempx = posx - realx, tempy = posy - realy;
     if (!(isBetween(tempx, -2, 2) && isBetween(tempy, -2, 2))) {
      if (mon_at(realx, realy) != -1)
       mvwputch(w_terrain, realy + VIEWY - posy, realx + VIEWX - posx,
                c_white, '?');
      else
       mvwputch(w_terrain, realy + VIEWY - posy, realx + VIEWX - posx,
                c_magenta, '#');
     }
    }
   }
  }
 }
 wrefresh(w_terrain);
 if (u.has_disease("visuals") || (u.has_disease("hot_head") && u.disease_intensity("hot_head") != 1))
   hallucinate(posx, posy);
}

void game::refresh_all()
{
 m.reset_vehicle_cache();
 draw();
 draw_HP();
 wrefresh(w_messages);
 refresh();
 draw_minimap();
}

void game::draw_HP()
{
    werase(w_HP);
    int current_hp;
    nc_color color;
    std::string health_bar = "";

    // The HP window can be in "tall" mode (7x14) or "wide" mode (14x7).
    bool wide = (getmaxy(w_HP) == 7);
    int hpx = wide ? 7 : 0;
    int hpy = wide ? 0 : 1;
    int dy  = wide ? 1 : 2;
    for (int i = 0; i < num_hp_parts; i++) {
        current_hp = u.hp_cur[i];
        if (current_hp == u.hp_max[i]){
          color = c_green;
          health_bar = "|||||";
        } else if (current_hp > u.hp_max[i] * .9) {
          color = c_green;
          health_bar = "||||\\";
        } else if (current_hp > u.hp_max[i] * .8) {
          color = c_ltgreen;
          health_bar = "||||";
        } else if (current_hp > u.hp_max[i] * .7) {
          color = c_ltgreen;
          health_bar = "|||\\";
        } else if (current_hp > u.hp_max[i] * .6) {
          color = c_yellow;
          health_bar = "|||";
        } else if (current_hp > u.hp_max[i] * .5) {
          color = c_yellow;
          health_bar = "||\\";
        } else if (current_hp > u.hp_max[i] * .4) {
          color = c_ltred;
          health_bar = "||";
        } else if (current_hp > u.hp_max[i] * .3) {
          color = c_ltred;
          health_bar = "|\\";
        } else if (current_hp > u.hp_max[i] * .2) {
          color = c_red;
          health_bar = "|";
        } else if (current_hp > u.hp_max[i] * .1) {
          color = c_red;
          health_bar = "\\";
        } else if (current_hp > 0) {
          color = c_red;
          health_bar = ":";
        } else {
          color = c_ltgray;
          health_bar = "-----";
        }
        wmove(w_HP, i * dy + hpy, hpx);
        if (u.has_trait("SELFAWARE")) {
            wprintz(w_HP, color, "%3d  ", current_hp);
        } else {
            wprintz(w_HP, color, health_bar.c_str());

            //Add the trailing symbols for a not-quite-full health bar
            int bar_remainder = 5;
            while(bar_remainder > health_bar.size()){
                --bar_remainder;
                wprintz(w_HP, c_white, ".");
            }
        }
    }

    static const char *body_parts[] = { _("HEAD"), _("TORSO"), _("L ARM"),
                           _("R ARM"), _("L LEG"), _("R LEG"), _("POWER") };
    int num_parts = sizeof(body_parts) / sizeof(body_parts[0]);
    for (int i = 0; i < num_parts; i++) {
        nc_color c = c_ltgray;
        const char *str = body_parts[i];
        wmove(w_HP, i * dy, 0);
        if (wide)
            wprintz(w_HP, c, " ");
        wprintz(w_HP, c, str);
        if (!wide)
            wprintz(w_HP, c, ":");
    }

    int powx = hpx;
    int powy = wide ? 6 : 13;
    if (u.max_power_level == 0){
        wmove(w_HP, powy, powx);
        if (wide)
            for (int i = 0; i < 2; i++)
                wputch(w_HP, c_ltgray, LINE_OXOX);
        else
            wprintz(w_HP, c_ltgray, " --   ");
    } else {
        if (u.power_level == u.max_power_level){
            color = c_blue;
        } else if (u.power_level >= u.max_power_level * .5){
            color = c_ltblue;
        } else if (u.power_level > 0){
            color = c_yellow;
        } else {
            color = c_red;
        }
        mvwprintz(w_HP, powy, powx, color, "%-3d", u.power_level);
    }
    wrefresh(w_HP);
}

void game::draw_minimap()
{
 // Draw the box
 werase(w_minimap);
 mvwputch(w_minimap, 0, 0, c_white, LINE_OXXO);
 mvwputch(w_minimap, 0, 6, c_white, LINE_OOXX);
 mvwputch(w_minimap, 6, 0, c_white, LINE_XXOO);
 mvwputch(w_minimap, 6, 6, c_white, LINE_XOOX);
 for (int i = 1; i < 6; i++) {
  mvwputch(w_minimap, i, 0, c_white, LINE_XOXO);
  mvwputch(w_minimap, i, 6, c_white, LINE_XOXO);
  mvwputch(w_minimap, 0, i, c_white, LINE_OXOX);
  mvwputch(w_minimap, 6, i, c_white, LINE_OXOX);
 }

 int cursx = (levx + int(MAPSIZE / 2)) / 2;
 int cursy = (levy + int(MAPSIZE / 2)) / 2;

 bool drew_mission = false;
 point targ(-1, -1);
 if (u.active_mission >= 0 && u.active_mission < u.active_missions.size())
  targ = find_mission(u.active_missions[u.active_mission])->target;
 else
  drew_mission = true;

 if (targ.x == -1)
  drew_mission = true;

 for (int i = -2; i <= 2; i++) {
  for (int j = -2; j <= 2; j++) {
   int omx = cursx + i;
   int omy = cursy + j;
   bool seen = false;
   oter_id cur_ter = ot_null;
   long note_sym = 0;
   bool note = false;
   if (omx >= 0 && omx < OMAPX && omy >= 0 && omy < OMAPY) {
    cur_ter = cur_om->ter(omx, omy, levz);
    seen    = cur_om->seen(omx, omy, levz);
    if (cur_om->has_note(omx,omy,levz))
    {
        if (cur_om->note(omx,omy,levz)[1] == ':')
            note_sym = cur_om->note(omx,omy,levz)[0];
        note = true;
    }
   } else if ((omx < 0 || omx >= OMAPX) && (omy < 0 || omy >= OMAPY)) {
    if (omx < 0) omx += OMAPX;
    else         omx -= OMAPX;
    if (omy < 0) omy += OMAPY;
    else         omy -= OMAPY;
    cur_ter = om_diag->ter(omx, omy, levz);
    seen    = om_diag->seen(omx, omy, levz);
    if (om_diag->has_note(omx,omy,levz))
    {
        if (om_diag->note(omx,omy,levz)[1] == ':')
            note_sym = om_diag->note(omx,omy,levz)[0];
        note = true;
    }
   } else if (omx < 0 || omx >= OMAPX) {
    if (omx < 0) omx += OMAPX;
    else         omx -= OMAPX;
    cur_ter = om_hori->ter(omx, omy, levz);
    seen    = om_hori->seen(omx, omy, levz);
    if (om_hori->has_note(omx,omy,levz))
    {
        if (om_hori->note(omx,omy,levz)[1] == ':')
            note_sym = om_hori->note(omx,omy,levz)[0];
        note = true;
    }
   } else if (omy < 0 || omy >= OMAPY) {
    if (omy < 0) omy += OMAPY;
    else         omy -= OMAPY;
    cur_ter = om_vert->ter(omx, omy, levz);
    seen    = om_vert->seen(omx, omy, levz);
    if (om_vert->has_note(omx,omy,levz))
    {
        if (om_vert->note(omx,omy,levz)[1] == ':')
            note_sym = om_vert->note(omx,omy,levz)[0];
        note = true;
    }
   } else {
    dbg(D_ERROR) << "game:draw_minimap: No data loaded! omx: "
                 << omx << " omy: " << omy;
    debugmsg("No data loaded! omx: %d omy: %d", omx, omy);
   }
   nc_color ter_color = oterlist[cur_ter].color;
   long ter_sym = oterlist[cur_ter].sym;
   if (note)
   {
       ter_sym = note_sym ? note_sym : 'N';
       ter_color = c_yellow;
   }
   if (seen) {
    if (!drew_mission && targ.x == omx && targ.y == omy) {
     drew_mission = true;
     if (i != 0 || j != 0)
      mvwputch   (w_minimap, 3 + j, 3 + i, red_background(ter_color), ter_sym);
     else
      mvwputch_hi(w_minimap, 3,     3,     ter_color, ter_sym);
    } else if (i == 0 && j == 0)
     mvwputch_hi(w_minimap, 3,     3,     ter_color, ter_sym);
    else
     mvwputch   (w_minimap, 3 + j, 3 + i, ter_color, ter_sym);
   }
  }
 }

// Print arrow to mission if we have one!
 if (!drew_mission) {
  double slope;
  if (cursx != targ.x)
   slope = double(targ.y - cursy) / double(targ.x - cursx);
  if (cursx == targ.x || abs(slope) > 3.5 ) { // Vertical slope
   if (targ.y > cursy)
    mvwputch(w_minimap, 6, 3, c_red, '*');
   else
    mvwputch(w_minimap, 0, 3, c_red, '*');
  } else {
   int arrowx = 3, arrowy = 3;
   if (abs(slope) >= 1.) { // y diff is bigger!
    arrowy = (targ.y > cursy ? 6 : 0);
    arrowx = 3 + 3 * (targ.y > cursy ? slope : (0 - slope));
    if (arrowx < 0)
     arrowx = 0;
    if (arrowx > 6)
     arrowx = 6;
   } else {
    arrowx = (targ.x > cursx ? 6 : 0);
    arrowy = 3 + 3 * (targ.x > cursx ? slope : (0 - slope));
    if (arrowy < 0)
     arrowy = 0;
    if (arrowy > 6)
     arrowy = 6;
   }
   mvwputch(w_minimap, arrowy, arrowx, c_red, '*');
  }
 }

 wrefresh(w_minimap);
}

void game::hallucinate(const int x, const int y)
{
 for (int i = 0; i <= TERRAIN_WINDOW_WIDTH; i++) {
  for (int j = 0; j <= TERRAIN_WINDOW_HEIGHT; j++) {
   if (one_in(10)) {
    char ter_sym = terlist[m.ter(i + x - VIEWX + rng(-2, 2), j + y - VIEWY + rng(-2, 2))].sym;
    nc_color ter_col = terlist[m.ter(i + x - VIEWX + rng(-2, 2), j + y - VIEWY+ rng(-2, 2))].color;
    mvwputch(w_terrain, j, i, ter_col, ter_sym);
   }
  }
 }
 wrefresh(w_terrain);
}

float game::natural_light_level() const
{
 float ret = 0;

 if (levz >= 0) {
  ret = turn.sunlight();
  ret += weather_data[weather].light_modifier;
 }

 return std::max(0.0f, ret);
}

unsigned char game::light_level()
{
 //already found the light level for now?
 if(turn == latest_lightlevel_turn)
  return latest_lightlevel;

 int ret;
 if (levz < 0)	// Underground!
  ret = 1;
 else {
  ret = turn.sunlight();
  ret -= weather_data[weather].sight_penalty;
 }
 for (int i = 0; i < events.size(); i++) {
  // The EVENT_DIM event slowly dims the sky, then relights it
  // EVENT_DIM has an occurance date of turn + 50, so the first 25 dim it
  if (events[i].type == EVENT_DIM) {
   int turns_left = events[i].turn - int(turn);
   i = events.size();
   if (turns_left > 25)
    ret = (ret * (turns_left - 25)) / 25;
   else
    ret = (ret * (25 - turns_left)) / 25;
  }
 }
 if (ret < 8 && event_queued(EVENT_ARTIFACT_LIGHT))
  ret = 8;
 if(ret < 1)
  ret = 1;

 latest_lightlevel = ret;
 latest_lightlevel_turn = turn;
 return ret;
}

void game::reset_light_level()
{
 latest_lightlevel = 0;
 latest_lightlevel_turn = 0;
}

//Gets the next free ID, also used for player ID's.
int game::assign_npc_id()
{
 int ret = next_npc_id;
 next_npc_id++;
 return ret;
}

int game::assign_faction_id()
{
 int ret = next_faction_id;
 next_faction_id++;
 return ret;
}

faction* game::faction_by_id(int id)
{
 for (int i = 0; i < factions.size(); i++) {
  if (factions[i].id == id)
   return &(factions[i]);
 }
 return NULL;
}

faction* game::random_good_faction()
{
 std::vector<int> valid;
 for (int i = 0; i < factions.size(); i++) {
  if (factions[i].good >= 5)
   valid.push_back(i);
 }
 if (valid.size() > 0) {
  int index = valid[rng(0, valid.size() - 1)];
  return &(factions[index]);
 }
// No good factions exist!  So create one!
 faction newfac(assign_faction_id());
 do
  newfac.randomize();
 while (newfac.good < 5);
 newfac.id = factions.size();
 factions.push_back(newfac);
 return &(factions[factions.size() - 1]);
}

faction* game::random_evil_faction()
{
 std::vector<int> valid;
 for (int i = 0; i < factions.size(); i++) {
  if (factions[i].good <= -5)
   valid.push_back(i);
 }
 if (valid.size() > 0) {
  int index = valid[rng(0, valid.size() - 1)];
  return &(factions[index]);
 }
// No good factions exist!  So create one!
 faction newfac(assign_faction_id());
 do
  newfac.randomize();
 while (newfac.good > -5);
 newfac.id = factions.size();
 factions.push_back(newfac);
 return &(factions[factions.size() - 1]);
}

bool game::sees_u(int x, int y, int &t)
{
 // TODO: [lightmap] Apply default monster vison levels here
 //                  the light map should deal lighting from player or fires
 int range = light_level();

 // Set to max possible value if the player is lit brightly
 if (m.light_at(u.posx, u.posy) >= LL_LOW)
  range = DAYLIGHT_LEVEL;

 int mondex = mon_at(x,y);
 if (mondex != -1) {
  if(z[mondex].has_flag(MF_VIS10))
   range -= 50;
  else if(z[mondex].has_flag(MF_VIS20))
   range -= 40;
  else if(z[mondex].has_flag(MF_VIS30))
   range -= 30;
  else if(z[mondex].has_flag(MF_VIS40))
   range -= 20;
  else if(z[mondex].has_flag(MF_VIS50))
   range -= 10;
 }
 if( range <= 0)
  range = 1;

 return (!(u.has_active_bionic("bio_cloak") || u.has_active_bionic("bio_night") ||
           u.has_artifact_with(AEP_INVISIBLE)) && m.sees(x, y, u.posx, u.posy, range, t));
}

bool game::u_see(int x, int y)
{
 int wanted_range = rl_dist(u.posx, u.posy, x, y);

 bool can_see = false;
 if (wanted_range < u.clairvoyance())
  can_see = true;
 else if (wanted_range <= u.sight_range(light_level()) ||
          (wanted_range <= u.sight_range(DAYLIGHT_LEVEL) &&
            m.light_at(x, y) >= LL_LOW))
     can_see = m.pl_sees(u.posx, u.posy, x, y, wanted_range);
     if (u.has_active_bionic("bio_night") && wanted_range < 15 && wanted_range > u.sight_range(1))
        return false;

 return can_see;
}

bool game::u_see(player *p)
{
 return u_see(p->posx, p->posy);
}

bool game::u_see(monster *mon)
{
 int dist = rl_dist(u.posx, u.posy, mon->posx, mon->posy);
 if (u.has_trait("ANTENNAE") && dist <= 3)
  return true;
 if (mon->has_flag(MF_DIGS) && !u.has_active_bionic("bio_ground_sonar") &&
     dist > 1)
  return false;	// Can't see digging monsters until we're right next to them

 return u_see(mon->posx, mon->posy);
}

bool game::pl_sees(player *p, monster *mon, int &t)
{
 // TODO: [lightmap] Allow npcs to use the lightmap
 if (mon->has_flag(MF_DIGS) && !p->has_active_bionic("bio_ground_sonar") &&
     rl_dist(p->posx, p->posy, mon->posx, mon->posy) > 1)
  return false;	// Can't see digging monsters until we're right next to them
 int range = p->sight_range(light_level());
 return m.sees(p->posx, p->posy, mon->posx, mon->posy, range, t);
}

point game::find_item(item *it)
{
 if (u.has_item(it))
  return point(u.posx, u.posy);
 point ret = m.find_item(it);
 if (ret.x != -1 && ret.y != -1)
  return ret;
 for (int i = 0; i < active_npc.size(); i++) {
  if (active_npc[i]->inv.has_item(it))
   return point(active_npc[i]->posx, active_npc[i]->posy);
 }
 return point(-999, -999);
}

void game::remove_item(item *it)
{
 point ret;
 if (it == &u.weapon) {
  u.remove_weapon();
  return;
 }
 if (!u.inv.remove_item(it).is_null()) {
  return;
 }
 for (int i = 0; i < u.worn.size(); i++) {
  if (it == &u.worn[i]) {
   u.worn.erase(u.worn.begin() + i);
   return;
  }
 }
 ret = m.find_item(it);
 if (ret.x != -1 && ret.y != -1) {
  for (int i = 0; i < m.i_at(ret.x, ret.y).size(); i++) {
   if (it == &m.i_at(ret.x, ret.y)[i]) {
    m.i_rem(ret.x, ret.y, i);
    return;
   }
  }
 }
 for (int i = 0; i < active_npc.size(); i++) {
  if (it == &active_npc[i]->weapon) {
   active_npc[i]->remove_weapon();
   return;
  }
  if (!active_npc[i]->inv.remove_item(it).is_null()) {
   return;
  }
  for (int j = 0; j < active_npc[i]->worn.size(); j++) {
   if (it == &active_npc[i]->worn[j]) {
    active_npc[i]->worn.erase(active_npc[i]->worn.begin() + j);
    return;
   }
  }
 }
}

bool vector_has(std::vector<int> vec, int test)
{
 for (int i = 0; i < vec.size(); i++) {
  if (vec[i] == test)
   return true;
 }
 return false;
}

// Print monster info to the given window, and return the lowest row (0-indexed)
// to which we printed. This is used to share a window with the message log and
// make optimal use of space.
int game::mon_info(WINDOW *w)
{
    const int width = getmaxx(w);
    const int maxheight = 12;
    const int startrow = (OPTIONS["SIDEBAR_STYLE"] == "Narrow") ? 1 : 0;

    int buff;
    int newseen = 0;
    const int iProxyDist = (OPTIONS["SAFEMODEPROXIMITY"] <= 0) ? 60 : OPTIONS["SAFEMODEPROXIMITY"];
    int newdist = 4096;
    int newtarget = -1;
    // 7 0 1	unique_types uses these indices;
    // 6 8 2	0-7 are provide by direction_from()
    // 5 4 3	8 is used for local monsters (for when we explain them below)
    std::vector<int> unique_types[9];
    // dangerous_types tracks whether we should print in red to warn the player
    bool dangerous[8];
    for (int i = 0; i < 8; i++)
        dangerous[i] = false;

    direction dir_to_mon, dir_to_npc;
    int viewx = u.posx + u.view_offset_x;
    int viewy = u.posy + u.view_offset_y;
    for (int i = 0; i < z.size(); i++) {
        if (u_see(&(z[i]))) {
            dir_to_mon = direction_from(viewx, viewy, z[i].posx, z[i].posy);
            int index = (abs(viewx - z[i].posx) <= VIEWX &&
                         abs(viewy - z[i].posy) <= VIEWY) ?
                         8 : dir_to_mon;

            monster_attitude matt = z[i].attitude(&u);
            if (MATT_ATTACK == matt || MATT_FOLLOW == matt) {
                int j;
                if (index < 8 && sees_u(z[i].posx, z[i].posy, j))
                    dangerous[index] = true;

                int mondist = rl_dist(u.posx, u.posy, z[i].posx, z[i].posy);
                if (mondist <= iProxyDist) {
                    newseen++;
                    if ( mondist < newdist ) {
                        newdist = mondist; // todo: prioritize dist * attack+follow > attack > follow
                        newtarget = i; // todo: populate alt targeting map
                    }
                }
            }

            if (!vector_has(unique_types[dir_to_mon], z[i].type->id))
                unique_types[index].push_back(z[i].type->id);
        }
    }

    for (int i = 0; i < active_npc.size(); i++) {
        point npcp(active_npc[i]->posx, active_npc[i]->posy);
        if (u_see(npcp.x, npcp.y)) { // TODO: NPC invis
            if (active_npc[i]->attitude == NPCATT_KILL)
                if (rl_dist(u.posx, u.posy, npcp.x, npcp.y) <= iProxyDist)
                    newseen++;

            dir_to_npc = direction_from(viewx, viewy, npcp.x, npcp.y);
            int index = (abs(viewx - npcp.x) <= VIEWX &&
                         abs(viewy - npcp.y) <= VIEWY) ?
                         8 : dir_to_npc;
            unique_types[index].push_back(-1 - i);
        }
    }

    if (newseen > mostseen) {
        if (u.activity.type == ACT_REFILL_VEHICLE)
            cancel_activity_query(_("Monster Spotted!"));
        cancel_activity_query(_("Monster spotted!"));
        turnssincelastmon = 0;
        if (run_mode == 1) {
            run_mode = 2;	// Stop movement!
            if ( last_target == -1 && newtarget != -1 ) {
                last_target = newtarget;
            }
        }
    } else if (autosafemode && newseen == 0) { // Auto-safemode
        turnssincelastmon++;
        if (turnssincelastmon >= OPTIONS["AUTOSAFEMODETURNS"] && run_mode == 0)
            run_mode = 1;
    }

    if (newseen == 0 && run_mode == 2)
        run_mode = 1;

    mostseen = newseen;

    // Print the direction headings
    // Reminder:
    // 7 0 1	unique_types uses these indices;
    // 6 8 2	0-7 are provide by direction_from()
    // 5 4 3	8 is used for local monsters (for when we explain them below)

    const char *dir_labels[] = {
        _("North:"), _("NE:"), _("East:"), _("SE:"),
        _("South:"), _("SW:"), _("West:"), _("NW:") };
    int widths[8];
    for (int i = 0; i < 8; i++) {
        widths[i] = strlen(dir_labels[i]);
    }
    const int row1spaces = width - (widths[7] + widths[0] + widths[1]);
    const int row3spaces = width - (widths[5] + widths[4] + widths[3]);
    int xcoords[8];
    const int ycoords[] = { 0, 0, 1, 2, 2, 2, 1, 0 };
    xcoords[0] = widths[7] +  row1spaces / 3;
    xcoords[1] = widths[7] - (row1spaces / 3) + row1spaces + widths[0];
    xcoords[4] = widths[5] +  row3spaces / 3;
    xcoords[3] = widths[5] - (row3spaces / 3) + row3spaces + widths[4];
    xcoords[2] = (xcoords[1] + xcoords[3]) / 2;
    xcoords[5] = xcoords[6] = xcoords[7] = 0;
    for (int i = 0; i < 8; i++) {
        nc_color c = unique_types[i].empty() ? c_dkgray
                   : (dangerous[i] ? c_ltred : c_ltgray);
        mvwprintz(w, ycoords[i] + startrow, xcoords[i], c, dir_labels[i]);
    }

    // The list of symbols needs a space on each end.
    const int symroom = row1spaces / 3 - 2;

    // Print the symbols of all monsters in all directions.
    for (int i = 0; i < 8; i++) {
        point pr(xcoords[i] + strlen(dir_labels[i]) + 1, ycoords[i] + startrow);

        const int typeshere = unique_types[i].size();
        for (int j = 0; j < typeshere && j < symroom; j++) {
            buff = unique_types[i][j];
            nc_color c;
            char sym;

            if (symroom < typeshere && j == symroom - 1) {
                // We've run out of room!
                c = c_white;
                sym = '+';
            } else if (buff < 0) { // It's an NPC!
                switch (active_npc[(buff + 1) * -1]->attitude) {
                    case NPCATT_KILL:   c = c_red;     break;
                    case NPCATT_FOLLOW: c = c_ltgreen; break;
                    case NPCATT_DEFEND: c = c_green;   break;
                    default:            c = c_pink;    break;
                }
                sym = '@';
            } else { // It's a monster!
                c   = mtypes[buff]->color;
                sym = mtypes[buff]->sym;
            }
            mvwputch(w, pr.y, pr.x, c, sym);

            pr.x++;
        }
    } // for (int i = 0; i < 8; i++)

    // Now we print their full names!

    bool listed_it[num_monsters]; // Don't list any twice!
    for (int i = 0; i < num_monsters; i++)
        listed_it[i] = false;

    // Start printing monster names on row 4. Rows 0-2 are for labels, and row 3
    // is blank.
    point pr(0, 4 + startrow);

    int lastrowprinted = 2 + startrow;

    // Print monster names, starting with those at location 8 (nearby).
    for (int j = 8; j >= 0 && pr.y < maxheight; j--) {
        // Separate names by some number of spaces (more for local monsters).
        int namesep = (j == 8 ? 2 : 1);
        for (int i = 0; i < unique_types[j].size() && pr.y < maxheight; i++) {
            buff = unique_types[j][i];
            // buff < 0 means an NPC!  Don't list those.
            if (buff >= 0 && !listed_it[buff]) {
                listed_it[buff] = true;
                std::string name = mtypes[buff]->name;

                // Move to the next row if necessary. (The +2 is for the "Z ").
                if (pr.x + 2 + name.length() >= width) {
                    pr.y++;
                    pr.x = 0;
                }

                if (pr.y < maxheight) { // Don't print if we've overflowed
                    lastrowprinted = pr.y;
                    mvwputch(w, pr.y, pr.x, mtypes[buff]->color, mtypes[buff]->sym);
                    pr.x += 2; // symbol and space
                    nc_color danger = c_dkgray;
                    if (mtypes[buff]->difficulty >= 30)
                        danger = c_red;
                    else if (mtypes[buff]->difficulty >= 16)
                        danger = c_ltred;
                    else if (mtypes[buff]->difficulty >= 8)
                        danger = c_white;
                    else if (mtypes[buff]->agro > 0)
                        danger = c_ltgray;
                    mvwprintz(w, pr.y, pr.x, danger, name.c_str());
                    pr.x += name.length() + namesep;
                }
            }
        }
    }

    return lastrowprinted;
}

void game::cleanup_dead()
{
    for( int i = 0; i < z.size(); i++ ) {
        if( z[i].dead || z[i].hp <= 0 ) {
            dbg (D_INFO) << string_format( "cleanup_dead: z[%d] %d,%d dead:%c hp:%d %s",
                                           i, z[i].posx, z[i].posy, (z[i].dead?'1':'0'),
                                           z[i].hp, z[i].type->name.c_str() );
            z.erase( z.begin() + i );
            if( last_target == i ) {
                last_target = -1;
            } else if( last_target > i ) {
                last_target--;
            }
            i--;
        }
    }

    //Cleanup any dead npcs.
    //This will remove the npc object, it is assumed that they have been transformed into
    //dead bodies before this.
    for (int i = 0; i < active_npc.size(); i++)
    {
        if (active_npc[i]->dead)
        {
            int npc_id = active_npc[i]->getID();
            active_npc.erase( active_npc.begin() + i );
            cur_om->remove_npc(npc_id);
            i--;
        }
    }
}

void game::monmove()
{
 cleanup_dead();
 for (int i = 0; i < z.size(); i++) {
  while (!z[i].dead && !z[i].can_move_to(this, z[i].posx, z[i].posy)) {
// If we can't move to our current position, assign us to a new one
   if (debugmon)
   {
    dbg(D_ERROR) << "game:monmove: " << z[i].name().c_str()
                 << " can't move to its location! (" << z[i].posx
                 << ":" << z[i].posy << "), "
                 << m.tername(z[i].posx, z[i].posy).c_str();
    debugmsg("%s can't move to its location! (%d:%d), %s", z[i].name().c_str(),
             z[i].posx, z[i].posy, m.tername(z[i].posx, z[i].posy).c_str());
   }
   bool okay = false;
   int xdir = rng(1, 2) * 2 - 3, ydir = rng(1, 2) * 2 - 3; // -1 or 1
   int startx = z[i].posx - 3 * xdir, endx = z[i].posx + 3 * xdir;
   int starty = z[i].posy - 3 * ydir, endy = z[i].posy + 3 * ydir;
   for (int x = startx; x != endx && !okay; x += xdir) {
    for (int y = starty; y != endy && !okay; y += ydir){
     if (z[i].can_move_to(this, x, y)) {
      z[i].posx = x;
      z[i].posy = y;
      okay = true;
     }
    }
   }
   if (!okay)
    z[i].dead = true;
  }

  if (!z[i].dead) {
   z[i].process_effects(this);
   if (z[i].hurt(0))
    kill_mon(i, false);
  }

  m.mon_in_field(z[i].posx, z[i].posy, this, &(z[i]));

  while (z[i].moves > 0 && !z[i].dead) {
   z[i].made_footstep = false;
   z[i].plan(this);	// Formulate a path to follow
   z[i].move(this);	// Move one square, possibly hit u
   z[i].process_triggers(this);
   m.mon_in_field(z[i].posx, z[i].posy, this, &(z[i]));
   if (z[i].hurt(0)) {	// Maybe we died...
    kill_mon(i, false);
    z[i].dead = true;
   }
  }

  if (!z[i].dead) {
   if (u.has_active_bionic("bio_alarm") && u.power_level >= 1 &&
       rl_dist(u.posx, u.posy, z[i].posx, z[i].posy) <= 5) {
    u.power_level--;
    add_msg(_("Your motion alarm goes off!"));
    cancel_activity_query(_("Your motion alarm goes off!"));
    if (u.has_disease("sleep") || u.has_disease("lying_down")) {
     u.rem_disease("sleep");
     u.rem_disease("lying_down");
    }
   }
// We might have stumbled out of range of the player; if so, kill us
   if (z[i].posx < 0 - (SEEX * MAPSIZE) / 6 ||
       z[i].posy < 0 - (SEEY * MAPSIZE) / 6 ||
       z[i].posx > (SEEX * MAPSIZE * 7) / 6 ||
       z[i].posy > (SEEY * MAPSIZE * 7) / 6   ) {
// Re-absorb into local group, if applicable
    int group = valid_group((mon_id)(z[i].type->id), levx, levy, levz);
    if (group != -1) {
     cur_om->zg[group].population++;
     if (cur_om->zg[group].population / (cur_om->zg[group].radius * cur_om->zg[group].radius) > 5 &&
         !cur_om->zg[group].diffuse )
      cur_om->zg[group].radius++;
    } else if (MonsterGroupManager::Monster2Group((mon_id)(z[i].type->id)) != "GROUP_NULL") {
     cur_om->zg.push_back(mongroup(MonsterGroupManager::Monster2Group((mon_id)(z[i].type->id)),
                                  levx, levy, levz, 1, 1));
    }
    z[i].dead = true;
   } else
    z[i].receive_moves();
  }
 }

 cleanup_dead();

// Now, do active NPCs.
 for (int i = 0; i < active_npc.size(); i++) {
  int turns = 0;
  if(active_npc[i]->hp_cur[hp_head] <= 0 || active_npc[i]->hp_cur[hp_torso] <= 0)
   active_npc[i]->die(this);
  else {
   active_npc[i]->reset(this);
   active_npc[i]->suffer(this);
   while (!active_npc[i]->dead && active_npc[i]->moves > 0 && turns < 10) {
    turns++;
    active_npc[i]->move(this);
    //build_monmap();
   }
   if (turns == 10) {
    add_msg(_("%s's brain explodes!"), active_npc[i]->name.c_str());
    active_npc[i]->die(this);
   }
  }
 }
 cleanup_dead();
}

bool game::sound(int x, int y, int vol, std::string description)
{
 vol *= 1.5; // Scale it a little
// First, alert all monsters (that can hear) to the sound
 for (int i = 0; i < z.size(); i++) {
  if (z[i].can_hear()) {
   int dist = rl_dist(x, y, z[i].posx, z[i].posy);
   int volume = vol - (z[i].has_flag(MF_GOODHEARING) ? int(dist / 2) : dist);
   z[i].wander_to(x, y, volume);
   z[i].process_trigger(MTRIG_SOUND, volume);
  }
 }
// Loud sounds make the next spawn sooner!
 int spawn_range = int(MAPSIZE / 2) * SEEX;
 if (vol >= spawn_range) {
  int max = (vol - spawn_range);
  int min = int(max / 6);
  if (max > spawn_range * 4)
   max = spawn_range * 4;
  if (min > spawn_range * 4)
   min = spawn_range * 4;
  int change = rng(min, max);
  if (nextspawn < change)
   nextspawn = 0;
  else
   nextspawn -= change;
 }
// Next, display the sound as the player hears it
 if (description == "")
  return false;	// No description (e.g., footsteps)
 if (u.has_disease("deaf"))
  return false;	// We're deaf, can't hear it

 if (u.has_bionic("bio_ears"))
  vol *= 3.5;
 if (u.has_trait("BADHEARING"))
  vol *= .5;
 if (u.has_trait("CANINE_EARS"))
  vol *= 1.5;
 int dist = rl_dist(x, y, u.posx, u.posy);
 if (dist > vol)
  return false;	// Too far away, we didn't hear it!
 if (u.has_disease("sleep") &&
     ((!u.has_trait("HEAVYSLEEPER") && dice(2, 20) < vol - dist) ||
      ( u.has_trait("HEAVYSLEEPER") && dice(3, 20) < vol - dist)   )) {
  u.rem_disease("sleep");
  if (description != "alarm_clock")
   add_msg(_("You're woken up by a noise."));
  return true;
 } else if (description == "alarm_clock") {
  return false;
 }
 if (!u.has_bionic("bio_ears") && rng( (vol - dist) / 2, (vol - dist) ) >= 150) {
  int duration = (vol - dist - 130) / 4;
  if (duration > 40)
   duration = 40;
  u.add_disease("deaf", duration);
 }
 if (x != u.posx || y != u.posy) {
  if(u.activity.ignore_trivial != true) {
    std::string query;
    if (description == "") {
        query = string_format(_("Heard %s!"), description.c_str());
    } else {
        query = _("Heard a noise!");
    }
    if( cancel_activity_or_ignore_query(query.c_str()) ) {
        u.activity.ignore_trivial = true;
    }
  }
 } else {
     u.volume += vol;
 }

// We need to figure out where it was coming from, relative to the player
 int dx = x - u.posx;
 int dy = y - u.posy;
// If it came from us, don't print a direction
 if (dx == 0 && dy == 0) {
  capitalize_letter(description, 0);
  add_msg("%s", description.c_str());
  return true;
 }
 std::string direction = direction_name(direction_from(u.posx, u.posy, x, y));
 add_msg(_("From the %s you hear %s"), direction.c_str(), description.c_str());
 return true;
}

// add_footstep will create a list of locations to draw monster
// footsteps. these will be more or less accurate depending on the
// characters hearing and how close they are
void game::add_footstep(int x, int y, int volume, int distance, monster* source)
{
 if (x == u.posx && y == u.posy)
  return;
 else if (u_see(x, y))
  return;
 int err_offset;
 if (volume / distance < 2)
  err_offset = 3;
 else if (volume / distance < 3)
  err_offset = 2;
 else
  err_offset = 1;
 if (u.has_bionic("bio_ears"))
  err_offset--;
 if (u.has_trait("BADHEARING"))
  err_offset++;

 int origx = x, origy = y;
 std::vector<point> point_vector;
 for (x = origx-err_offset; x <= origx+err_offset; x++)
 {
     for (y = origy-err_offset; y <= origy+err_offset; y++)
     {
         point_vector.push_back(point(x,y));
     }
 }
 footsteps.push_back(point_vector);
 footsteps_source.push_back(source);
 return;
}

// draws footsteps that have been created by monsters moving about
void game::draw_footsteps()
{
 for (int i = 0; i < footsteps.size(); i++) {
     if (!u_see(footsteps_source[i]->posx,footsteps_source[i]->posy))
     {
         std::vector<point> unseen_points;
         for (int j = 0; j < footsteps[i].size(); j++)
         {
             if (!u_see(footsteps[i][j].x,footsteps[i][j].y))
             {
                 unseen_points.push_back(point(footsteps[i][j].x,
                                               footsteps[i][j].y));
             }
         }

         if (unseen_points.size() > 0)
         {
             point selected = unseen_points[rng(0,unseen_points.size() - 1)];

             mvwputch(w_terrain,
                      VIEWY + selected.y - u.posy - u.view_offset_y,
                      VIEWX + selected.x - u.posx - u.view_offset_x,
                      c_yellow, '?');
         }
     }
 }
 footsteps.clear();
 footsteps_source.clear();
 wrefresh(w_terrain);
 return;
}

void game::explosion(int x, int y, int power, int shrapnel, bool has_fire)
{
 timespec ts;	// Timespec for the animation of the explosion
 ts.tv_sec = 0;
 ts.tv_nsec = EXPLOSION_SPEED;
 int radius = sqrt(double(power / 4));
 int dam;
 std::string junk;
 int noise = power * (has_fire ? 2 : 10);

 if (power >= 30)
  sound(x, y, noise, _("a huge explosion!"));
 else
  sound(x, y, noise, _("an explosion!"));
 for (int i = x - radius; i <= x + radius; i++) {
  for (int j = y - radius; j <= y + radius; j++) {
   if (i == x && j == y)
    dam = 3 * power;
   else
    dam = 3 * power / (rl_dist(x, y, i, j));
   if (m.has_flag(bashable, i, j))
    m.bash(i, j, dam, junk);
   if (m.has_flag(bashable, i, j))	// Double up for tough doors, etc.
    m.bash(i, j, dam, junk);
   if (m.is_destructable(i, j) && rng(25, 100) < dam)
    m.destroy(this, i, j, false);

   int mon_hit = mon_at(i, j), npc_hit = npc_at(i, j);
   if (mon_hit != -1 && !z[mon_hit].dead &&
       z[mon_hit].hurt(rng(dam / 2, dam * 1.5))) {
    if (z[mon_hit].hp < 0 - (z[mon_hit].type->size < 2? 1.5:3) * z[mon_hit].type->hp)
     explode_mon(mon_hit); // Explode them if it was big overkill
    else
     kill_mon(mon_hit); // TODO: player's fault?

    int vpart;
    vehicle *veh = m.veh_at(i, j, vpart);
    if (veh)
     veh->damage (vpart, dam, false);
   }

   if (npc_hit != -1) {
    active_npc[npc_hit]->hit(this, bp_torso, 0, rng(dam / 2, dam * 1.5), 0);
    active_npc[npc_hit]->hit(this, bp_head,  0, rng(dam / 3, dam),       0);
    active_npc[npc_hit]->hit(this, bp_legs,  0, rng(dam / 3, dam),       0);
    active_npc[npc_hit]->hit(this, bp_legs,  1, rng(dam / 3, dam),       0);
    active_npc[npc_hit]->hit(this, bp_arms,  0, rng(dam / 3, dam),       0);
    active_npc[npc_hit]->hit(this, bp_arms,  1, rng(dam / 3, dam),       0);
    if (active_npc[npc_hit]->hp_cur[hp_head]  <= 0 ||
        active_npc[npc_hit]->hp_cur[hp_torso] <= 0   ) {
     active_npc[npc_hit]->die(this, true);
    }
   }
   if (u.posx == i && u.posy == j) {
    add_msg(_("You're caught in the explosion!"));
    u.hit(this, bp_torso, 0, rng(dam / 2, dam * 1.5), 0);
    u.hit(this, bp_head,  0, rng(dam / 3, dam),       0);
    u.hit(this, bp_legs,  0, rng(dam / 3, dam),       0);
    u.hit(this, bp_legs,  1, rng(dam / 3, dam),       0);
    u.hit(this, bp_arms,  0, rng(dam / 3, dam),       0);
    u.hit(this, bp_arms,  1, rng(dam / 3, dam),       0);
   }
   if (has_fire) {
    m.add_field(this, i, j, fd_fire, dam / 10);
   }
  }
 }
// Draw the explosion
 for (int i = 1; i <= radius; i++) {
  mvwputch(w_terrain, y - i + VIEWY - u.posy - u.view_offset_y,
                      x - i + VIEWX - u.posx - u.view_offset_x, c_red, '/');
  mvwputch(w_terrain, y - i + VIEWY - u.posy - u.view_offset_y,
                      x + i + VIEWX - u.posx - u.view_offset_x, c_red,'\\');
  mvwputch(w_terrain, y + i + VIEWY - u.posy - u.view_offset_y,
                      x - i + VIEWX - u.posx - u.view_offset_x, c_red,'\\');
  mvwputch(w_terrain, y + i + VIEWY - u.posy - u.view_offset_y,
                      x + i + VIEWX - u.posx - u.view_offset_x, c_red, '/');
  for (int j = 1 - i; j < 0 + i; j++) {
   mvwputch(w_terrain, y - i + VIEWY - u.posy - u.view_offset_y,
                       x + j + VIEWX - u.posx - u.view_offset_x, c_red,'-');
   mvwputch(w_terrain, y + i + VIEWY - u.posy - u.view_offset_y,
                       x + j + VIEWX - u.posx - u.view_offset_x, c_red,'-');
   mvwputch(w_terrain, y + j + VIEWY - u.posy - u.view_offset_y,
                       x - i + VIEWX - u.posx - u.view_offset_x, c_red,'|');
   mvwputch(w_terrain, y + j + VIEWY - u.posy - u.view_offset_y,
                       x + i + VIEWX - u.posx - u.view_offset_x, c_red,'|');
  }
  wrefresh(w_terrain);
  nanosleep(&ts, NULL);
 }

// The rest of the function is shrapnel
 if (shrapnel <= 0)
  return;
 int sx, sy, t, tx, ty;
 std::vector<point> traj;
 ts.tv_sec = 0;
 ts.tv_nsec = BULLET_SPEED;	// Reset for animation of bullets
 for (int i = 0; i < shrapnel; i++) {
  sx = rng(x - 2 * radius, x + 2 * radius);
  sy = rng(y - 2 * radius, y + 2 * radius);
  if (m.sees(x, y, sx, sy, 50, t))
   traj = line_to(x, y, sx, sy, t);
  else
   traj = line_to(x, y, sx, sy, 0);
  dam = rng(20, 60);
  for (int j = 0; j < traj.size(); j++) {
   if (j > 0 && u_see(traj[j - 1].x, traj[j - 1].y))
    m.drawsq(w_terrain, u, traj[j - 1].x, traj[j - 1].y, false, true);
   if (u_see(traj[j].x, traj[j].y)) {
    mvwputch(w_terrain, traj[j].y + VIEWY - u.posy - u.view_offset_y,
                        traj[j].x + VIEWX - u.posx - u.view_offset_x, c_red, '`');
    wrefresh(w_terrain);
    nanosleep(&ts, NULL);
   }
   tx = traj[j].x;
   ty = traj[j].y;
   if (mon_at(tx, ty) != -1) {
    dam -= z[mon_at(tx, ty)].armor_cut();
    if (z[mon_at(tx, ty)].hurt(dam))
     kill_mon(mon_at(tx, ty));
   } else if (npc_at(tx, ty) != -1) {
    body_part hit = random_body_part();
    if (hit == bp_eyes || hit == bp_mouth || hit == bp_head)
     dam = rng(2 * dam, 5 * dam);
    else if (hit == bp_torso)
     dam = rng(1.5 * dam, 3 * dam);
    int npcdex = npc_at(tx, ty);
    active_npc[npcdex]->hit(this, hit, rng(0, 1), 0, dam);
    if (active_npc[npcdex]->hp_cur[hp_head] <= 0 ||
        active_npc[npcdex]->hp_cur[hp_torso] <= 0) {
     active_npc[npcdex]->die(this);
    }
   } else if (tx == u.posx && ty == u.posy) {
    body_part hit = random_body_part();
    int side = rng(0, 1);
    add_msg(_("Shrapnel hits your %s!"), body_part_name(hit, side).c_str());
    u.hit(this, hit, rng(0, 1), 0, dam);
   } else {
       std::set<std::string> shrapnel_effects;
       m.shoot(this, tx, ty, dam, j == traj.size() - 1, shrapnel_effects );
   }
  }
 }
}

void game::flashbang(int x, int y, bool player_immune)
{
 int dist = rl_dist(u.posx, u.posy, x, y), t;
 if (dist <= 8 && !player_immune) {
  if (!u.has_bionic("bio_ears"))
   u.add_disease("deaf", 40 - dist * 4);
  if (m.sees(u.posx, u.posy, x, y, 8, t))
   u.infect("blind", bp_eyes, (12 - dist) / 2, 10 - dist, this);
 }
 for (int i = 0; i < z.size(); i++) {
  dist = rl_dist(z[i].posx, z[i].posy, x, y);
  if (dist <= 4)
   z[i].add_effect(ME_STUNNED, 10 - dist);
  if (dist <= 8) {
   if (z[i].has_flag(MF_SEES) && m.sees(z[i].posx, z[i].posy, x, y, 8, t))
    z[i].add_effect(ME_BLIND, 18 - dist);
   if (z[i].has_flag(MF_HEARS))
    z[i].add_effect(ME_DEAF, 60 - dist * 4);
  }
 }
 sound(x, y, 12, _("a huge boom!"));
// TODO: Blind/deafen NPC
}

void game::shockwave(int x, int y, int radius, int force, int stun, int dam_mult, bool ignore_player)
{
  //borrowed code from game::explosion()
  timespec ts;	// Timespec for the animation of the explosion
  ts.tv_sec = 0;
  ts.tv_nsec = EXPLOSION_SPEED;
    for (int i = 1; i <= radius; i++) {
  mvwputch(w_terrain, y - i + VIEWY - u.posy - u.view_offset_y,
                      x - i + VIEWX - u.posx - u.view_offset_x, c_blue, '/');
  mvwputch(w_terrain, y - i + VIEWY - u.posy - u.view_offset_y,
                      x + i + VIEWX - u.posx - u.view_offset_x, c_blue,'\\');
  mvwputch(w_terrain, y + i + VIEWY - u.posy - u.view_offset_y,
                      x - i + VIEWX - u.posx - u.view_offset_x, c_blue,'\\');
  mvwputch(w_terrain, y + i + VIEWY - u.posy - u.view_offset_y,
                      x + i + VIEWX - u.posx - u.view_offset_x, c_blue, '/');
  for (int j = 1 - i; j < 0 + i; j++) {
   mvwputch(w_terrain, y - i + VIEWY - u.posy - u.view_offset_y,
                       x + j + VIEWX - u.posx - u.view_offset_x, c_blue,'-');
   mvwputch(w_terrain, y + i + VIEWY - u.posy - u.view_offset_y,
                       x + j + VIEWX - u.posx - u.view_offset_x, c_blue,'-');
   mvwputch(w_terrain, y + j + VIEWY - u.posy - u.view_offset_y,
                       x - i + VIEWX - u.posx - u.view_offset_x, c_blue,'|');
   mvwputch(w_terrain, y + j + VIEWY - u.posy - u.view_offset_y,
                       x + i + VIEWX - u.posx - u.view_offset_x, c_blue,'|');
  }
  wrefresh(w_terrain);
  nanosleep(&ts, NULL);
 }
 // end borrowed code from game::explosion()

    sound(x, y, force*force*dam_mult/2, _("Crack!"));
    for (int i = 0; i < z.size(); i++)
    {
        if (rl_dist(z[i].posx, z[i].posy, x, y) <= radius)
        {
            add_msg(_("%s is caught in the shockwave!"), z[i].name().c_str());
            knockback(x, y, z[i].posx, z[i].posy, force, stun, dam_mult);
        }
    }
    for (int i = 0; i < active_npc.size(); i++)
    {
        if (rl_dist(active_npc[i]->posx, active_npc[i]->posy, x, y) <= radius)
        {
            add_msg(_("%s is caught in the shockwave!"), active_npc[i]->name.c_str());
            knockback(x, y, active_npc[i]->posx, active_npc[i]->posy, force, stun, dam_mult);
        }
    }
    if (rl_dist(u.posx, u.posy, x, y) <= radius && !ignore_player)
    {
        add_msg(_("You're caught in the shockwave!"));
        knockback(x, y, u.posx, u.posy, force, stun, dam_mult);
    }
    return;
}

/* Knockback target at (tx,ty) by force number of tiles in direction from (sx,sy) to (tx,ty)
   stun > 0 indicates base stun duration, and causes impact stun; stun == -1 indicates only impact stun
   dam_mult multiplies impact damage, bash effect on impact, and sound level on impact */

void game::knockback(int sx, int sy, int tx, int ty, int force, int stun, int dam_mult)
{
    std::vector<point> traj;
    traj.clear();
    traj = line_to(sx, sy, tx, ty, 0);
    traj.insert(traj.begin(), point(sx, sy)); // how annoying, line_to() doesn't include the originating point!
    traj = continue_line(traj, force);
    traj.insert(traj.begin(), point(tx, ty)); // how annoying, continue_line() doesn't either!

    knockback(traj, force, stun, dam_mult);
    return;
}

/* Knockback target at traj.front() along line traj; traj should already have considered knockback distance.
   stun > 0 indicates base stun duration, and causes impact stun; stun == -1 indicates only impact stun
   dam_mult multiplies impact damage, bash effect on impact, and sound level on impact */

void game::knockback(std::vector<point>& traj, int force, int stun, int dam_mult)
{
    int tx = traj.front().x;
    int ty = traj.front().y;
    if (mon_at(tx, ty) == -1 && npc_at(tx, ty) == -1 && (u.posx != tx && u.posy != ty))
    {
        debugmsg(_("Nothing at (%d,%d) to knockback!"), tx, ty);
        return;
    }
    //add_msg("line from %d,%d to %d,%d",traj.front().x,traj.front().y,traj.back().x,traj.back().y);
    std::string junk;
    int force_remaining = 0;
    if (mon_at(tx, ty) != -1)
    {
        monster *targ = &z[mon_at(tx, ty)];
        if (stun > 0)
        {
            targ->add_effect(ME_STUNNED, stun);
            add_msg(ngettext("%s was stunned for %d turn!",
                             "%s was stunned for %d turns!", stun),
                    targ->name().c_str(), stun);
        }
        for(int i = 1; i < traj.size(); i++)
        {
            if (m.move_cost(traj[i].x, traj[i].y) == 0 && !m.has_flag(liquid, traj[i].x, traj[i].y)) // oops, we hit a wall!
            {
                targ->posx = traj[i-1].x;
                targ->posy = traj[i-1].y;
                force_remaining = traj.size() - i;
                if (stun != 0)
                {
                    if (targ->has_effect(ME_STUNNED))
                    {
                        targ->add_effect(ME_STUNNED, force_remaining);
                        add_msg(ngettext("%s was stunned AGAIN for %d turn!",
                                         "%s was stunned AGAIN for %d turns!",
                                         force_remaining),
                                targ->name().c_str(), force_remaining);
                    }
                    else
                    {
                        targ->add_effect(ME_STUNNED, force_remaining);
                        add_msg(ngettext("%s was stunned for %d turn!",
                                         "%s was stunned for %d turns!",
                                         force_remaining),
                                targ->name().c_str(), force_remaining);
                    }
                    add_msg(_("%s took %d damage!"), targ->name().c_str(), dam_mult*force_remaining);
                    targ->hp -= dam_mult*force_remaining;
                    if (targ->hp <= 0)
                        targ->die(this);
                }
                m.bash(traj[i].x, traj[i].y, 2*dam_mult*force_remaining, junk);
                sound(traj[i].x, traj[i].y, dam_mult*force_remaining*force_remaining/2, junk);
                break;
            }
            else if (mon_at(traj[i].x, traj[i].y) != -1 || npc_at(traj[i].x, traj[i].y) != -1 ||
                      (u.posx == traj[i].x && u.posy == traj[i].y))
            {
                targ->posx = traj[i-1].x;
                targ->posy = traj[i-1].y;
                force_remaining = traj.size() - i;
                if (stun != 0)
                {
                    if (targ->has_effect(ME_STUNNED))
                    {
                        targ->add_effect(ME_STUNNED, force_remaining);
                        add_msg(ngettext("%s was stunned AGAIN for %d turn!",
                                         "%s was stunned AGAIN for %d turns!",
                                         force_remaining),
                                targ->name().c_str(), force_remaining);
                    }
                    else
                    {
                        targ->add_effect(ME_STUNNED, force_remaining);
                        add_msg(ngettext("%s was stunned for %d turn!",
                                         "%s was stunned for %d turns!",
                                         force_remaining),
                                targ->name().c_str(), force_remaining);
                    }
                }
                traj.erase(traj.begin(), traj.begin()+i);
                if (mon_at(traj.front().x, traj.front().y) != -1) {
                    add_msg(_("%s collided with something else and sent it flying!"),
                            targ->name().c_str());
                } else if (npc_at(traj.front().x, traj.front().y) != -1) {
                    if (active_npc[npc_at(traj.front().x, traj.front().y)]->male) {
                        add_msg(_("%s collided with someone else and sent him flying!"),
                                targ->name().c_str());
                    } else {
                        add_msg(_("%s collided with someone else and sent her flying!"),
                                targ->name().c_str());
                    }
                } else if (u.posx == traj.front().x && u.posy == traj.front().y) {
                    add_msg(_("%s collided with you and sent you flying!"), targ->name().c_str());
                }
                knockback(traj, force_remaining, stun, dam_mult);
                break;
            }
            targ->posx = traj[i].x;
            targ->posy = traj[i].y;
            if(m.has_flag(liquid, targ->posx, targ->posy) && !targ->has_flag(MF_SWIMS) &&
                !targ->has_flag(MF_AQUATIC) && !targ->has_flag(MF_FLIES) && !targ->dead)
            {
                targ->hurt(9999);
                if (u_see(targ))
                    add_msg(_("The %s drowns!"), targ->name().c_str());
            }
            if(!m.has_flag(liquid, targ->posx, targ->posy) && targ->has_flag(MF_AQUATIC) && !targ->dead)
            {
                targ->hurt(9999);
                if (u_see(targ))
                    add_msg(_("The %s flops around and dies!"), targ->name().c_str());
            }
        }
    }
    else if (npc_at(tx, ty) != -1)
    {
        npc *targ = active_npc[npc_at(tx, ty)];
        if (stun > 0)
        {
            targ->add_disease("stunned", stun);
            add_msg(ngettext("%s was stunned for %d turn!",
                             "%s was stunned for %d turns!", stun),
                    targ->name.c_str(), stun);
        }
        for(int i = 1; i < traj.size(); i++)
        {
            if (m.move_cost(traj[i].x, traj[i].y) == 0 && !m.has_flag(liquid, traj[i].x, traj[i].y)) // oops, we hit a wall!
            {
                targ->posx = traj[i-1].x;
                targ->posy = traj[i-1].y;
                force_remaining = traj.size() - i;
                if (stun != 0)
                {
                    if (targ->has_disease("stunned"))
                    {
                        targ->add_disease("stunned", force_remaining);
                        if (targ->has_disease("stunned"))
                            add_msg(ngettext("%s was stunned AGAIN for %d turn!",
                                             "%s was stunned AGAIN for %d turns!",
                                             force_remaining),
                                    targ->name.c_str(), force_remaining);
                    }
                    else
                    {
                        targ->add_disease("stunned", force_remaining);
                        if (targ->has_disease("stunned"))
                            add_msg(ngettext("%s was stunned for %d turn!",
                                             "%s was stunned for %d turns!",
                                             force_remaining),
                                     targ->name.c_str(), force_remaining);
                    }
                    add_msg(_("%s took %d damage! (before armor)"), targ->name.c_str(), dam_mult*force_remaining);
                    if (one_in(2)) targ->hit(this, bp_arms, 0, force_remaining*dam_mult, 0);
                    if (one_in(2)) targ->hit(this, bp_arms, 1, force_remaining*dam_mult, 0);
                    if (one_in(2)) targ->hit(this, bp_legs, 0, force_remaining*dam_mult, 0);
                    if (one_in(2)) targ->hit(this, bp_legs, 1, force_remaining*dam_mult, 0);
                    if (one_in(2)) targ->hit(this, bp_torso, 0, force_remaining*dam_mult, 0);
                    if (one_in(2)) targ->hit(this, bp_head, 0, force_remaining*dam_mult, 0);
                    if (one_in(2)) targ->hit(this, bp_hands, 0, force_remaining*dam_mult, 0);
                }
                m.bash(traj[i].x, traj[i].y, 2*dam_mult*force_remaining, junk);
                sound(traj[i].x, traj[i].y, dam_mult*force_remaining*force_remaining/2, junk);
                break;
            }
            else if (mon_at(traj[i].x, traj[i].y) != -1 || npc_at(traj[i].x, traj[i].y) != -1 ||
                      (u.posx == traj[i].x && u.posy == traj[i].y))
            {
                targ->posx = traj[i-1].x;
                targ->posy = traj[i-1].y;
                force_remaining = traj.size() - i;
                if (stun != 0)
                {
                    if (targ->has_disease("stunned"))
                    {
                        add_msg(ngettext("%s was stunned AGAIN for %d turn!",
                                         "%s was stunned AGAIN for %d turns!",
                                         force_remaining),
                                 targ->name.c_str(), force_remaining);
                    }
                    else
                    {
                        add_msg(ngettext("%s was stunned for %d turn!",
                                         "%s was stunned for %d turns!",
                                         force_remaining),
                                 targ->name.c_str(), force_remaining);
                    }
                    targ->add_disease("stunned", force_remaining);
                }
                traj.erase(traj.begin(), traj.begin()+i);
                if (mon_at(traj.front().x, traj.front().y) != -1) {
                    add_msg(_("%s collided with something else and sent it flying!"),
                            targ->name.c_str());
                } else if (npc_at(traj.front().x, traj.front().y) != -1) {
                    if (active_npc[npc_at(traj.front().x, traj.front().y)]->male) {
                        add_msg(_("%s collided with someone else and sent him flying!"),
                                targ->name.c_str());
                    } else {
                        add_msg(_("%s collided with someone else and sent her flying!"),
                                targ->name.c_str());
                    }
                } else if (u.posx == traj.front().x && u.posy == traj.front().y) {
                    add_msg(_("%s collided with you and sent you flying!"), targ->name.c_str());
                }
                knockback(traj, force_remaining, stun, dam_mult);
                break;
            }
            targ->posx = traj[i].x;
            targ->posy = traj[i].y;
        }
    }
    else if (u.posx == tx && u.posy == ty)
    {
        if (stun > 0)
        {
            u.add_disease("stunned", stun);
            add_msg(_("You were stunned for %d turns!"), stun);
        }
        for(int i = 1; i < traj.size(); i++)
        {
            if (m.move_cost(traj[i].x, traj[i].y) == 0 && !m.has_flag(liquid, traj[i].x, traj[i].y)) // oops, we hit a wall!
            {
                u.posx = traj[i-1].x;
                u.posy = traj[i-1].y;
                force_remaining = traj.size() - i;
                if (stun != 0)
                {
                    if (u.has_disease("stunned"))
                    {
                        add_msg(_("You were stunned AGAIN for %d turns!"), force_remaining);
                    }
                    else
                    {
                        add_msg(_("You were stunned for %d turns!"), force_remaining);
                    }
                    u.add_disease("stunned", force_remaining);
                    if (one_in(2)) u.hit(this, bp_arms, 0, force_remaining*dam_mult, 0);
                    if (one_in(2)) u.hit(this, bp_arms, 1, force_remaining*dam_mult, 0);
                    if (one_in(2)) u.hit(this, bp_legs, 0, force_remaining*dam_mult, 0);
                    if (one_in(2)) u.hit(this, bp_legs, 1, force_remaining*dam_mult, 0);
                    if (one_in(2)) u.hit(this, bp_torso, 0, force_remaining*dam_mult, 0);
                    if (one_in(2)) u.hit(this, bp_head, 0, force_remaining*dam_mult, 0);
                    if (one_in(2)) u.hit(this, bp_hands, 0, force_remaining*dam_mult, 0);
                }
                m.bash(traj[i].x, traj[i].y, 2*dam_mult*force_remaining, junk);
                sound(traj[i].x, traj[i].y, dam_mult*force_remaining*force_remaining/2, junk);
                break;
            }
            else if (mon_at(traj[i].x, traj[i].y) != -1 || npc_at(traj[i].x, traj[i].y) != -1)
            {
                u.posx = traj[i-1].x;
                u.posy = traj[i-1].y;
                force_remaining = traj.size() - i;
                if (stun != 0)
                {
                    if (u.has_disease("stunned"))
                    {
                        add_msg(_("You were stunned AGAIN for %d turns!"), force_remaining);
                    }
                    else
                    {
                        add_msg(_("You were stunned for %d turns!"), force_remaining);
                    }
                    u.add_disease("stunned", force_remaining);
                }
                traj.erase(traj.begin(), traj.begin()+i);
                if (mon_at(traj.front().x, traj.front().y) != -1) {
                    add_msg(_("You collided with something and sent it flying!"));
                } else if (npc_at(traj.front().x, traj.front().y) != -1) {
                    if (active_npc[npc_at(traj.front().x, traj.front().y)]->male) {
                        add_msg(_("You collided with someone and sent him flying!"));
                    } else {
                        add_msg(_("You collided with someone and sent her flying!"));
                    }
                }
                knockback(traj, force_remaining, stun, dam_mult);
                break;
            }
            if(m.has_flag(liquid, u.posx, u.posy) && force_remaining < 1)
            {
                plswim(u.posx, u.posy);
            }
            else
            {
                u.posx = traj[i].x;
                u.posy = traj[i].y;
            }
        }
    }
    return;
}

void game::use_computer(int x, int y)
{
 if (u.has_trait("ILLITERATE")) {
  add_msg(_("You can not read a computer screen!"));
  return;
 }

 if (u.has_trait("HYPEROPIC") && !u.is_wearing("glasses_reading")
     && !u.is_wearing("glasses_bifocal")) {
  add_msg(_("You'll need to put on reading glasses before you can see the screen."));
  return;
 }

 computer* used = m.computer_at(x, y);

 if (used == NULL) {
  dbg(D_ERROR) << "game:use_computer: Tried to use computer at (" << x
               << ", " << y << ") - none there";
  debugmsg("Tried to use computer at (%d, %d) - none there", x, y);
  return;
 }

 used->use(this);

 refresh_all();
}

void game::resonance_cascade(int x, int y)
{
 int maxglow = 100 - 5 * trig_dist(x, y, u.posx, u.posy);
 int minglow =  60 - 5 * trig_dist(x, y, u.posx, u.posy);
 mon_id spawn;
 monster invader;
 if (minglow < 0)
  minglow = 0;
 if (maxglow > 0)
  u.add_disease("teleglow", rng(minglow, maxglow) * 100);
 int startx = (x < 8 ? 0 : x - 8), endx = (x+8 >= SEEX*3 ? SEEX*3 - 1 : x + 8);
 int starty = (y < 8 ? 0 : y - 8), endy = (y+8 >= SEEY*3 ? SEEY*3 - 1 : y + 8);
 for (int i = startx; i <= endx; i++) {
  for (int j = starty; j <= endy; j++) {
   switch (rng(1, 80)) {
   case 1:
   case 2:
    emp_blast(i, j);
    break;
   case 3:
   case 4:
   case 5:
    for (int k = i - 1; k <= i + 1; k++) {
     for (int l = j - 1; l <= j + 1; l++) {
      field_id type;
      switch (rng(1, 7)) {
       case 1: type = fd_blood;
       case 2: type = fd_bile;
       case 3:
       case 4: type = fd_slime;
       case 5: type = fd_fire;
       case 6:
       case 7: type = fd_nuke_gas;
      }
      if (!one_in(3))
	   m.add_field(this, k, l, type, 3);
     }
    }
    break;
   case  6:
   case  7:
   case  8:
   case  9:
   case 10:
    m.tr_at(i, j) = tr_portal;
    break;
   case 11:
   case 12:
    m.tr_at(i, j) = tr_goo;
    break;
   case 13:
   case 14:
   case 15:
    spawn = MonsterGroupManager::GetMonsterFromGroup("GROUP_NETHER", &mtypes);
    invader = monster(mtypes[spawn], i, j);
    z.push_back(invader);
    break;
   case 16:
   case 17:
   case 18:
    m.destroy(this, i, j, true);
    break;
   case 19:
    explosion(i, j, rng(1, 10), rng(0, 1) * rng(0, 6), one_in(4));
    break;
   }
  }
 }
}

void game::scrambler_blast(int x, int y)
{
 int mondex = mon_at(x, y);
 if (mondex != -1) {
  if (z[mondex].has_flag(MF_ELECTRONIC))
    z[mondex].make_friendly();
   add_msg(_("The %s sparks and begins searching for a target!"), z[mondex].name().c_str());
 }
}
void game::emp_blast(int x, int y)
{
 int rn;
 if (m.has_flag(console, x, y)) {
  add_msg(_("The %s is rendered non-functional!"), m.tername(x, y).c_str());
  m.ter_set(x, y, t_console_broken);
  return;
 }
// TODO: More terrain effects.
 switch (m.ter(x, y)) {
 case t_card_science:
 case t_card_military:
  rn = rng(1, 100);
  if (rn > 92 || rn < 40) {
   add_msg(_("The card reader is rendered non-functional."));
   m.ter_set(x, y, t_card_reader_broken);
  }
  if (rn > 80) {
   add_msg(_("The nearby doors slide open!"));
   for (int i = -3; i <= 3; i++) {
    for (int j = -3; j <= 3; j++) {
     if (m.ter(x + i, y + j) == t_door_metal_locked)
      m.ter_set(x + i, y + j, t_floor);
    }
   }
  }
  if (rn >= 40 && rn <= 80)
   add_msg(_("Nothing happens."));
  break;
 }
 int mondex = mon_at(x, y);
 if (mondex != -1) {
  if (z[mondex].has_flag(MF_ELECTRONIC)) {
   add_msg(_("The EMP blast fries the %s!"), z[mondex].name().c_str());
   int dam = dice(10, 10);
   if (z[mondex].hurt(dam))
    kill_mon(mondex); // TODO: Player's fault?
   else if (one_in(6))
    z[mondex].make_friendly();
  } else
   add_msg(_("The %s is unaffected by the EMP blast."), z[mondex].name().c_str());
 }
 if (u.posx == x && u.posy == y) {
  if (u.power_level > 0) {
   add_msg(_("The EMP blast drains your power."));
   int max_drain = (u.power_level > 40 ? 40 : u.power_level);
   u.charge_power(0 - rng(1 + max_drain / 3, max_drain));
  }
// TODO: More effects?
 }
// Drain any items of their battery charge
 for (int i = 0; i < m.i_at(x, y).size(); i++) {
  if (m.i_at(x, y)[i].is_tool() &&
      (dynamic_cast<it_tool*>(m.i_at(x, y)[i].type))->ammo == "battery")
   m.i_at(x, y)[i].charges = 0;
 }
// TODO: Drain NPC energy reserves
}

int game::npc_at(int x, int y)
{
 for (int i = 0; i < active_npc.size(); i++) {
  if (active_npc[i]->posx == x && active_npc[i]->posy == y && !active_npc[i]->dead)
   return i;
 }
 return -1;
}

int game::npc_by_id(int id)
{
 for (int i = 0; i < active_npc.size(); i++) {
  if (active_npc[i]->getID() == id)
   return i;
 }
 return -1;
}

int game::mon_at(int x, int y)
{
 for (int i = 0; i < z.size(); i++) {
  if (z[i].posx == x && z[i].posy == y) {
   if (z[i].dead)
    return -1;
   else
    return i;
  }
 }
 return -1;
}

bool game::is_empty(int x, int y)
{
 return ((m.move_cost(x, y) > 0 || m.has_flag(liquid, x, y)) &&
         npc_at(x, y) == -1 && mon_at(x, y) == -1 &&
         (u.posx != x || u.posy != y));
}

bool game::is_in_sunlight(int x, int y)
{
 return (m.is_outside(x, y) && light_level() >= 40 &&
         (weather == WEATHER_CLEAR || weather == WEATHER_SUNNY));
}

void game::kill_mon(int index, bool u_did_it)
{
 if (index < 0 || index >= z.size()) {
  dbg(D_ERROR) << "game:kill_mon: Tried to kill monster " << index
               << "! (" << z.size() << " in play)";
  if (debugmon)  debugmsg("Tried to kill monster %d! (%d in play)", index, z.size());
  return;
 }
 if (!z[index].dead) {
  z[index].dead = true;
  if (u_did_it) {
   if (z[index].has_flag(MF_GUILT)) {
    mdeath tmpdeath;
    tmpdeath.guilt(this, &(z[index]));
   }
   if (z[index].type->species != species_hallu)
    kills[z[index].type->id]++;	// Increment our kill counter
  }
  for (int i = 0; i < z[index].inv.size(); i++)
   m.add_item_or_charges(z[index].posx, z[index].posy, z[index].inv[i]);
  z[index].die(this);
 }
}

void game::explode_mon(int index)
{
 if (index < 0 || index >= z.size()) {
  dbg(D_ERROR) << "game:explode_mon: Tried to explode monster " << index
               << "! (" << z.size() << " in play)";
  debugmsg("Tried to explode monster %d! (%d in play)", index, z.size());
  return;
 }
 if (!z[index].dead) {
  z[index].dead = true;
  kills[z[index].type->id]++;	// Increment our kill counter
// Send body parts and blood all over!
  mtype* corpse = z[index].type;
  if (corpse->mat == "flesh" || corpse->mat == "veggy") { // No chunks otherwise
   int num_chunks = 0;
   switch (corpse->size) {
    case MS_TINY:   num_chunks =  1; break;
    case MS_SMALL:  num_chunks =  2; break;
    case MS_MEDIUM: num_chunks =  4; break;
    case MS_LARGE:  num_chunks =  8; break;
    case MS_HUGE:   num_chunks = 16; break;
   }
   itype_id meat;
   if (corpse->has_flag(MF_POISON)) {
    if (corpse->mat == "flesh")
     meat = "meat_tainted";
    else
     meat = "veggy_tainted";
   } else {
    if (corpse->mat == "flesh")
     meat = "meat";
    else
     meat = "veggy";
   }

   int posx = z[index].posx, posy = z[index].posy;
   for (int i = 0; i < num_chunks; i++) {
    int tarx = posx + rng(-3, 3), tary = posy + rng(-3, 3);
    std::vector<point> traj = line_to(posx, posy, tarx, tary, 0);

    bool done = false;
    for (int j = 0; j < traj.size() && !done; j++) {
     tarx = traj[j].x;
     tary = traj[j].y;
// Choose a blood type and place it
     field_id blood_type = fd_blood;
     if (corpse->dies == &mdeath::boomer)
      blood_type = fd_bile;
     else if (corpse->dies == &mdeath::acid)
      blood_type = fd_acid;

      m.add_field(this, tarx, tary, blood_type, 1);

     if (m.move_cost(tarx, tary) == 0) {
      std::string tmp = "";
      if (m.bash(tarx, tary, 3, tmp))
       sound(tarx, tary, 18, tmp);
      else {
       if (j > 0) {
        tarx = traj[j - 1].x;
        tary = traj[j - 1].y;
       }
       done = true;
      }
     }
    }
    m.spawn_item(tarx, tary, meat, turn);
   }
  }
 }

 // there WAS an erasure of the monster here, but it caused issues with loops
 // we should structure things so that z.erase is only called in specified cleanup
 // functions

 if (last_target == index)
  last_target = -1;
 else if (last_target > index)
   last_target--;
}

void game::revive_corpse(int x, int y, int n)
{
    if (m.i_at(x, y).size() <= n)
    {
        debugmsg("Tried to revive a non-existent corpse! (%d, %d), #%d of %d", x, y, n, m.i_at(x, y).size());
        return;
    }
    item* it = &m.i_at(x, y)[n];
    revive_corpse(x, y, it);
    m.i_rem(x, y, n);
}

void game::revive_corpse(int x, int y, item *it)
{
    if (it->type->id != "corpse" || it->corpse == NULL)
    {
        debugmsg("Tried to revive a non-corpse.");
        return;
    }
    int burnt_penalty = it->burnt;
    monster mon(it->corpse, x, y);
    mon.speed = int(mon.speed * .8) - burnt_penalty / 2;
    mon.hp    = int(mon.hp    * .7) - burnt_penalty;
    if (it->damage > 0)
    {
        mon.speed /= it->damage + 1;
        mon.hp /= it->damage + 1;
    }
    mon.no_extra_death_drops = true;
    z.push_back(mon);
}

void game::open()
{
    int openx, openy;
    if (!choose_adjacent(_("Open"), openx, openy))
        return;

    u.moves -= 100;
    bool didit = false;

    int vpart;
    vehicle *veh = m.veh_at(openx, openy, vpart);
    if (veh && veh->part_flag(vpart, vpf_openable)) {
        if (veh->parts[vpart].open) {
            add_msg(_("That door is already open."));
            u.moves += 100;
        } else {
            veh->parts[vpart].open = 1;
            veh->insides_dirty = true;
        }
        return;
    }

    if (m.is_outside(u.posx, u.posy))
        didit = m.open_door(openx, openy, false);
    else
        didit = m.open_door(openx, openy, true);

    if (!didit) {
        switch(m.ter(openx, openy)) {
        case t_door_locked:
        case t_door_locked_interior:
        case t_door_locked_alarm:
        case t_door_bar_locked:
            add_msg(_("The door is locked!"));
            break;	// Trying to open a locked door uses the full turn's movement
        case t_door_o:
            add_msg(_("That door is already open."));
            u.moves += 100;
            break;
        default:
            add_msg(_("No door there."));
            u.moves += 100;
        }
    }
}

void game::close()
{
    int closex, closey;
    if (!choose_adjacent(_("Close"), closex, closey))
        return;

    bool didit = false;

    int vpart;
    vehicle *veh = m.veh_at(closex, closey, vpart);
    if (mon_at(closex, closey) != -1)
        add_msg(_("There's a %s in the way!"),
                z[mon_at(closex, closey)].name().c_str());
    else if (veh && veh->part_flag(vpart, vpf_openable) &&
             veh->parts[vpart].open) {
        veh->parts[vpart].open = 0;
        veh->insides_dirty = true;
        didit = true;
    } else if (m.furn(closex, closey) != f_safe_o && m.i_at(closex, closey).size() > 0)
        add_msg(_("There's %s in the way!"), m.i_at(closex, closey).size() == 1 ?
                m.i_at(closex, closey)[0].tname(this).c_str() : _("some stuff"));
    else if (closex == u.posx && closey == u.posy)
        add_msg(_("There's some buffoon in the way!"));
    else if (m.ter(closex, closey) == t_window_domestic &&
             m.is_outside(u.posx, u.posy))  {
        add_msg(_("You cannot close the curtains from outside. You must be inside the building."));
    } else if (m.has_furn(closex, closey) &&
               m.furn(closex, closey) != f_canvas_door_o &&
               m.furn(closex, closey) != f_skin_door_o &&
               m.furn(closex, closey) != f_safe_o) {
       add_msg(_("There's a %s in the way!"), m.furnname(closex, closey).c_str());
    } else
        didit = m.close_door(closex, closey, true);

    if (didit)
        u.moves -= 90;
}

void game::smash()
{
    const int move_cost = (u.weapon.is_null() ? 80 : u.weapon.attack_time() * 0.8);
    bool didit = false;
    std::string bashsound, extra;
    int smashskill = int(u.str_cur / 2.5 + u.weapon.type->melee_dam);
    int smashx, smashy;

    if (!choose_adjacent(_("Smash"), smashx, smashy))
        return;

    const int full_pulp_threshold = 4;
    std::list<item*> corpses;
    for (int i = 0; i < m.i_at(smashx, smashy).size(); ++i)
    {
        item *it = &m.i_at(smashx, smashy)[i];
        if (it->type->id == "corpse" && it->damage < full_pulp_threshold)
        {
            corpses.push_back(it);
        }
    }
    if (corpses.size() > 0)
    {
        add_msg(ngettext("You swing at the corpse.",
                         "You swing at the corpses.", corpses.size()));

        // numbers logic: a str 8 character with a butcher knife (4 bash, 18 cut)
        // should have at least a 50% chance of damaging an intact zombie corpse (75 volume).
        // a str 8 character with a baseball bat (28 bash, 0 cut) should have around a 25% chance.

        int cut_power = u.weapon.type->melee_cut;
        // stabbing weapons are a lot less effective at pulping
        if (u.weapon.has_flag("STAB") || u.weapon.has_flag("SPEAR"))
        {
            cut_power /= 2;
        }
        double pulp_power = sqrt((double)(u.str_cur + u.weapon.type->melee_dam)) * sqrt((double)(cut_power + 1));
        pulp_power *= 20; // constant multiplier to get the chance right
        int rn = rng(0, pulp_power);
        while (rn > 0 && !corpses.empty())
        {
            item *it = corpses.front();
            corpses.pop_front();
            int damage = rn / it->volume();
            if (damage + it->damage > full_pulp_threshold)
            {
                damage = full_pulp_threshold - it->damage;
            }
            rn -= (damage + 1) * it->volume(); // slight efficiency loss to swing

            // chance of a critical success, higher chance for small critters
            // comes AFTER the loss of power from the above calculation
            if (one_in(it->volume()))
            {
                damage++;
            }

            if (damage > 0)
            {
                if (damage > 1) {
                    add_msg(_("You greatly damage the %s!"), it->tname().c_str());
                } else {
                    add_msg(_("You damage the %s!"), it->tname().c_str());
                }
                it->damage += damage;
                if (it->damage >= 4)
                {
                    add_msg(_("The corpse is now thoroughly pulped."));
                    it->damage = 4;
                    // TODO mark corpses as inactive when appropriate
                }
                // Splatter some blood around
                for (int x = smashx - 1; x <= smashx + 1; x++) {
                    for (int y = smashy - 1; y <= smashy + 1; y++) {
                        if (!one_in(damage+1)) {
                             m.add_field(this, x, y, fd_blood, 1);
                        }
                    }
                }
            }
        }
        u.moves -= move_cost;
        return; // don't smash terrain if we've smashed a corpse
    }
    else
    {
        didit = m.bash(smashx, smashy, smashskill, bashsound);
    }

    if (didit)
    {
        if (extra != "")
        {
            add_msg(extra.c_str());
        }
        sound(smashx, smashy, 18, bashsound);
        // TODO: Move this elsewhere, like maybe into the map on-break code
        if (m.has_flag(alarmed, smashx, smashy) &&
            !event_queued(EVENT_WANTED))
        {
            sound(smashx, smashy, 40, _("An alarm sounds!"));
            add_event(EVENT_WANTED, int(turn) + 300, 0, levx, levy);
        }
        u.moves -= move_cost;
        if (u.skillLevel("melee") == 0)
        {
            u.practice(turn, "melee", rng(0, 1) * rng(0, 1));
        }
        if (u.weapon.made_of("glass") &&
            rng(0, u.weapon.volume() + 3) < u.weapon.volume())
        {
            add_msg(_("Your %s shatters!"), u.weapon.tname(this).c_str());
            for (int i = 0; i < u.weapon.contents.size(); i++)
            {
                m.add_item_or_charges(u.posx, u.posy, u.weapon.contents[i]);
            }
            sound(u.posx, u.posy, 24, "");
            u.hit(this, bp_hands, 1, 0, rng(0, u.weapon.volume()));
            if (u.weapon.volume() > 20)
            {
                // Hurt left arm too, if it was big
                u.hit(this, bp_hands, 0, 0, rng(0, u.weapon.volume() * .5));
            }
            u.remove_weapon();
        }
    }
    else
    {
        add_msg(_("There's nothing there!"));
    }
}

void game::use_item(char chInput)
{
 char ch;
 if (chInput == '.')
  ch = inv(_("Use item:"));
 else
  ch = chInput;

 if (ch == ' ') {
  add_msg(_("Never mind."));
  return;
 }
 last_action += ch;
 u.use(this, ch);
}

void game::use_wielded_item()
{
  u.use_wielded(this);
}

bool game::choose_adjacent(std::string verb, int &x, int &y)
{
    refresh_all();
    std::string query_text = verb + _(" where? (Direction button)");
    mvwprintw(w_terrain, 0, 0, query_text.c_str());
    wrefresh(w_terrain);
    DebugLog() << "calling get_input() for " << verb << "\n";
    InputEvent input = get_input();
    last_action += input;
    if (input == Cancel || input == Close)
        return false;
    else
        get_direction(x, y, input);
    if (x == -2 || y == -2) {
        add_msg(_("Invalid direction."));
        return false;
    }
    x += u.posx;
    y += u.posy;
    return true;
}

bool game::vehicle_near ()
{
 for (int dx = -1; dx <= 1; dx++) {
  for (int dy = -1; dy <= 1; dy++) {
   if (m.veh_at(u.posx + dx, u.posy + dy))
    return true;
  }
 }
 return false;
}

bool game::pl_refill_vehicle (vehicle &veh, int part, bool test)
{
    if (!veh.part_flag(part, vpf_fuel_tank))
        return false;
    item* it = NULL;
    item *p_itm = NULL;
    int min_charges = -1;
    bool i_cont = false;

    std::string ftype = veh.part_info(part).fuel_type;
    itype_id itid = default_ammo(ftype);
    if (u.weapon.is_container() && u.weapon.contents.size() > 0 &&
        u.weapon.contents[0].type->id == itid)
    {
        it = &u.weapon;
        p_itm = &u.weapon.contents[0];
        min_charges = u.weapon.contents[0].charges;
        i_cont = true;
    }
    else if (u.weapon.type->id == itid)
    {
        it = &u.weapon;
        p_itm = it;
        min_charges = u.weapon.charges;
    }
    else
    {
        it = &u.inv.item_or_container(itid);
        if (!it->is_null()) {
            if (it->type->id == itid) {
                p_itm = it;
            } else {
                //ah, must be a container of the thing
                p_itm = &(it->contents[0]);
                i_cont = true;
            }
            min_charges = p_itm->charges;
        }
    }
    if (it->is_null()) {
        return false;
    } else if (test) {
        return true;
    }

    int fuel_per_charge = 1;
    if( ftype == "plutonium" ) { fuel_per_charge = 1000; }
    else if( ftype == "plasma" ) { fuel_per_charge = 100; }
    int max_fuel = veh.part_info(part).size;
    int dch = (max_fuel - veh.parts[part].amount) / fuel_per_charge;
    if (dch < 1)
        dch = 1;
    bool rem_itm = min_charges <= dch;
    int used_charges = rem_itm? min_charges : dch;
    veh.parts[part].amount += used_charges * fuel_per_charge;
    if (veh.parts[part].amount > max_fuel) {
        veh.parts[part].amount = max_fuel;
    }

    if (ftype == "battery") {
        add_msg(_("You recharge %s's battery."), veh.name.c_str());
        if (veh.parts[part].amount == max_fuel) {
            add_msg(_("The battery is fully charged."));
        }
    } else if (ftype == "gasoline") {
        add_msg(_("You refill %s's fuel tank."), veh.name.c_str());
        if (veh.parts[part].amount == max_fuel) {
            add_msg(_("The tank is full."));
        }
    } else if (ftype == "plutonium") {
        add_msg(_("You refill %s's reactor."), veh.name.c_str());
        if (veh.parts[part].amount == max_fuel) {
            add_msg(_("The reactor is full."));
        }
    }

    p_itm->charges -= used_charges;
    if (rem_itm) {
        if (i_cont) {
            it->contents.erase(it->contents.begin());
        } else if (&u.weapon == it) {
            u.remove_weapon();
        } else {
            u.inv.remove_item_by_letter(it->invlet);
        }
    }
    return true;
}

void game::handbrake ()
{
 vehicle *veh = m.veh_at (u.posx, u.posy);
 if (!veh)
  return;
 add_msg (_("You pull a handbrake."));
 veh->cruise_velocity = 0;
 if (veh->last_turn != 0 && rng (15, 60) * 100 < abs(veh->velocity)) {
  veh->skidding = true;
  add_msg (_("You lose control of %s."), veh->name.c_str());
  veh->turn (veh->last_turn > 0? 60 : -60);
 } else if (veh->velocity < 0)
  veh->stop();
 else {
  veh->velocity = veh->velocity / 2 - 10*100;
  if (veh->velocity < 0)
      veh->stop();
 }
 u.moves = 0;
}

void game::exam_vehicle(vehicle &veh, int examx, int examy, int cx, int cy)
{
    veh_interact vehint;
    vehint.cx = cx;
    vehint.cy = cy;
    vehint.exec(this, &veh, examx, examy);
//    debugmsg ("exam_vehicle cmd=%c %d", vehint.sel_cmd, (int) vehint.sel_cmd);
    if (vehint.sel_cmd != ' ')
    {                                                        // TODO: different activity times
        u.activity = player_activity(ACT_VEHICLE,
                                     vehint.sel_cmd == 'f' || vehint.sel_cmd == 's' ||
                                     vehint.sel_cmd == 'c' ? 200 : 20000,
                                     (int) vehint.sel_cmd, 0, "");
        u.activity.values.push_back (veh.global_x());    // values[0]
        u.activity.values.push_back (veh.global_y());    // values[1]
        u.activity.values.push_back (vehint.cx);   // values[2]
        u.activity.values.push_back (vehint.cy);   // values[3]
        u.activity.values.push_back (-vehint.ddx - vehint.cy);   // values[4]
        u.activity.values.push_back (vehint.cx - vehint.ddy);   // values[5]
        u.activity.values.push_back (vehint.sel_part); // values[6]
        u.activity.values.push_back (vehint.sel_type); // int. might make bitmask
        u.moves = 0;
    }
    refresh_all();
}

// A gate handle is adjacent to a wall section, and next to that wall section on one side or
// another is the gate.  There may be a handle on the other side, but this is optional.
// The gate continues until it reaches a non-floor tile, so they can be arbitrary length.
//
//   |  !|!  -++-++-  !|++++-
//   +   +      !      +
//   +   +   -++-++-   +
//   +   +             +
//   +   +   !|++++-   +
//  !|   |!        !   |
//
// The terrain type of the handle is passed in, and that is used to determine the type of
// the wall and gate.
void game::open_gate( game *g, const int examx, const int examy, const enum ter_id handle_type ) {

 enum ter_id v_wall_type;
 enum ter_id h_wall_type;
 enum ter_id door_type;
 enum ter_id floor_type;
 const char *pull_message;
 const char *open_message;
 const char *close_message;

 switch(handle_type) {
 case t_gates_mech_control:
  v_wall_type = t_wall_v;
  h_wall_type = t_wall_h;
  door_type   = t_door_metal_locked;
  floor_type  = t_floor;
  pull_message = _("You turn the handle...");
  open_message = _("The gate is opened!");
  close_message = _("The gate is closed!");
  break;

 case t_gates_control_concrete:
  v_wall_type = t_concrete_v;
  h_wall_type = t_concrete_h;
  door_type   = t_door_metal_locked;
  floor_type  = t_floor;
  pull_message = _("You turn the handle...");
  open_message = _("The gate is opened!");
  close_message = _("The gate is closed!");
  break;

 case t_barndoor:
  v_wall_type = t_wall_wood;
  h_wall_type = t_wall_wood;
  door_type   = t_door_metal_locked;
  floor_type  = t_dirtfloor;
  pull_message = _("You pull the rope...");
  open_message = _("The barn doors opened!");
  close_message = _("The barn doors closed!");
  break;

 case t_palisade_pulley:
  v_wall_type = t_palisade;
  h_wall_type = t_palisade;
  door_type   = t_palisade_gate;
  floor_type  = t_palisade_gate_o;
  pull_message = _("You pull the rope...");
  open_message = _("The palisade gate swings open!");
  close_message = _("The palisade gate swings closed with a crash!");
  break;

  default: return; // No matching gate type
 }

 g->add_msg(pull_message);
 g->u.moves -= 900;

 bool open = false;
 bool close = false;

 for (int wall_x = -1; wall_x <= 1; wall_x++) {
   for (int wall_y = -1; wall_y <= 1; wall_y++) {
     for (int gate_x = -1; gate_x <= 1; gate_x++) {
       for (int gate_y = -1; gate_y <= 1; gate_y++) {
         if ((wall_x + wall_y == 1 || wall_x + wall_y == -1) &&  // make sure wall not diagonally opposite to handle
             (gate_x + gate_y == 1 || gate_x + gate_y == -1) &&  // same for gate direction
            ((wall_y != 0 && (g->m.ter(examx+wall_x, examy+wall_y) == h_wall_type)) ||  //horizontal orientation of the gate
             (wall_x != 0 && (g->m.ter(examx+wall_x, examy+wall_y) == v_wall_type)))) { //vertical orientation of the gate

           int cur_x = examx+wall_x+gate_x;
           int cur_y = examy+wall_y+gate_y;

           if (!close && (g->m.ter(examx+wall_x+gate_x, examy+wall_y+gate_y) == door_type)) {  //opening the gate...
             open = true;
             while (g->m.ter(cur_x, cur_y) == door_type) {
               g->m.ter_set(cur_x, cur_y, floor_type);
               cur_x = cur_x+gate_x;
               cur_y = cur_y+gate_y;
             }
           }

           if (!open && (g->m.ter(examx+wall_x+gate_x, examy+wall_y+gate_y) == floor_type)) {  //closing the gate...
             close = true;
             while (g->m.ter(cur_x, cur_y) == floor_type) {
               g->m.ter_set(cur_x, cur_y, door_type);
               cur_x = cur_x+gate_x;
               cur_y = cur_y+gate_y;
             }
           }
         }
       }
     }
   }
 }

 if(open){
   g->add_msg(open_message);
 } else if(close){
   g->add_msg(close_message);
 } else {
   add_msg(_("Nothing happens."));
 }
}

void game::moving_vehicle_dismount(int tox, int toy)
{
    int vpart;
    vehicle *veh = m.veh_at(u.posx, u.posy, vpart);
    if (!veh) {
        debugmsg("Tried to exit non-existent vehicle.");
        return;
    }
    if (u.posx == tox && u.posy == toy) {
        debugmsg("Need somewhere to dismount towards.");
        return;
    }
    int d = (45 * (direction_from(u.posx, u.posy, tox, toy)) - 90) % 360;
    add_msg(_("You dive from the %s."), veh->name.c_str());
    m.unboard_vehicle(this, u.posx, u.posy);
    u.moves -= 200;
    // Dive three tiles in the direction of tox and toy
    fling_player_or_monster(&u, 0, d, 30, true);
    // Hit the ground according to vehicle speed
    if (!m.has_flag(swimmable, u.posx, u.posy)) {
        if (veh->velocity > 0)
            fling_player_or_monster(&u, 0, veh->face.dir(), veh->velocity / (float)100);
        else
            fling_player_or_monster(&u, 0, veh->face.dir() + 180, -(veh->velocity) / (float)100);
    }
    return;
}

void game::control_vehicle()
{
    int veh_part;
    vehicle *veh = m.veh_at(u.posx, u.posy, veh_part);

    if (veh && veh->player_in_control(&u)) {
        std::string message = veh->use_controls();
        if (!message.empty())
            add_msg(message.c_str());
    } else if (veh && veh->part_with_feature(veh_part, vpf_controls) >= 0
                   && u.in_vehicle) {
        u.controlling_vehicle = true;
        add_msg(_("You take control of the %s."), veh->name.c_str());
    } else {
        int examx, examy;
        if (!choose_adjacent(_("Control vehicle"), examx, examy))
            return;
        veh = m.veh_at(examx, examy, veh_part);
        if (!veh) {
            add_msg(_("No vehicle there."));
            return;
        }
        if (veh->part_with_feature(veh_part, vpf_controls) < 0) {
            add_msg(_("No controls there."));
            return;
        }
        std::string message = veh->use_controls();
        if (!message.empty())
            add_msg(message.c_str());
    }
}

void game::examine()
{
 int examx, examy;
 if (!choose_adjacent(_("Examine"), examx, examy))
    return;

 int veh_part = 0;
 vehicle *veh = m.veh_at (examx, examy, veh_part);
 if (veh) {
  int vpcargo = veh->part_with_feature(veh_part, vpf_cargo, false);
  int vpkitchen = veh->part_with_feature(veh_part, vpf_kitchen, true);
  if ((vpcargo >= 0 && veh->parts[vpcargo].items.size() > 0) || vpkitchen >= 0)
   pickup(examx, examy, 0);
  else if (u.in_vehicle)
   add_msg (_("You can't do that while onboard."));
  else if (abs(veh->velocity) > 0)
   add_msg (_("You can't do that on moving vehicle."));
  else
   exam_vehicle (*veh, examx, examy);
 }

 if (m.has_flag(console, examx, examy)) {
  use_computer(examx, examy);
  return;
 }
 const furn_t *xfurn_t = &furnlist[m.furn(examx,examy)];
 const ter_t *xter_t = &terlist[m.ter(examx,examy)];
 iexamine xmine;

 if (m.has_furn(examx, examy))
   (xmine.*xfurn_t->examine)(this,&u,&m,examx,examy);
 else
   (xmine.*xter_t->examine)(this,&u,&m,examx,examy);

 bool none = true;
 if (xter_t->examine != &iexamine::none || xfurn_t->examine != &iexamine::none)
   none = false;

 if (m.has_flag(sealed, examx, examy)) {
   if (none) add_msg(_("The %s is firmly sealed."), m.name(examx, examy).c_str());
 } else {
   //examx,examy has no traps, is a container and doesn't have a special examination function
  if (m.tr_at(examx, examy) == tr_null && m.i_at(examx, examy).size() == 0 && m.has_flag(container, examx, examy) && none)
   add_msg(_("It is empty."));
  else
   if (!veh)pickup(examx, examy, 0);
 }
  //check for disarming traps last to avoid disarming query black box issue.
 if(m.tr_at(examx, examy) != tr_null) xmine.trap(this,&u,&m,examx,examy);

}

#define ADVINVOFS 7
// abstract of selected origin which can be inventory, or  map tile / vehicle storage / aggregate
struct advanced_inv_area {
    const int id;
    const int hscreenx;
    const int hscreeny;
    const int offx;
    const int offy;
    int x;
    int y;
    const std::string name;
    const std::string shortname;
    bool canputitems;
    vehicle *veh;
    int vstor;
    int size;
    std::string desc;
    int volume, weight;
    int max_size, max_volume;
};

// for printing items in environment
struct advanced_inv_listitem {
    int idx;
    int area;
    item *it;
    std::string name;
    bool autopickup;
    int stacks;
    int volume;
    int weight;
    int cat;
};

// left/right listwindows
struct advanced_inv_pane {
    int pos;
    int area, offx, offy, size, vstor;  // quick lookup later
    int index, max_page, max_index, page;
    std::string area_string;
    int sortby;
    int issrc;
    vehicle *veh;
    WINDOW *window;
    std::vector<advanced_inv_listitem> items;
    int numcats;
};

int getsquare(int c , int &off_x, int &off_y, std::string &areastring, advanced_inv_area *squares) {
    int ret=-1;
    if (!( c >= 0 && c <= 10 )) return ret;
    ret=c;
    off_x = squares[ret].offx;
    off_y = squares[ret].offy;
    areastring = squares[ret].name;
    return ret;
}

int getsquare(char c , int &off_x, int &off_y, std::string &areastring, advanced_inv_area *squares) {
    int ret=-1;
    switch(c)
    {
        case '0':
        case 'I':
            ret=0;
            break;
        case '1':
        case 'B':
            ret=1;
            break;
        case '2':
        case 'J':
            ret=2;
            break;
        case '3':
        case 'N':
            ret=3;
            break;
        case '4':
        case 'H':
            ret=4;
            break;
        case '5':
        case 'G':
            ret=5;
            break;
        case '6':
        case 'L':
            ret=6;
            break;
        case '7':
        case 'Y':
            ret=7;
            break;
        case '8':
        case 'K':
            ret=8;
            break;
        case '9':
        case 'U':
            ret=9;
            break;
        case 'a':
            ret=10;
            break;
        default :
            return -1;
    }
    return getsquare(ret,off_x,off_y,areastring, squares);
}

void advprintItems(advanced_inv_pane &pane, advanced_inv_area* squares, bool active, game* g)
{
    std::vector<advanced_inv_listitem> &items = pane.items;
    WINDOW* window = pane.window;
    int page = pane.page;
    int selected_index = pane.index;
    bool isinventory = ( pane.area == 0 );
    bool isall = ( pane.area == 10 );
    int itemsPerPage;
    itemsPerPage = getmaxy( window ) - ADVINVOFS; // fixme
    int columns = getmaxx( window );
    int rightcol = columns - 8;
    int amount_column = columns - 15;
    nc_color norm = active ? c_white : c_dkgray;
    std::string spaces(getmaxx(window)-4, ' ');
    bool compact=(TERMX<=100);

    if(isinventory) {
        int hrightcol=rightcol; // intentionally -not- shifting rightcol since heavy items are rare, and we're stingy on screenspace
        if (g->u.convert_weight(g->u.weight_carried()) > 9.9 ) {
          hrightcol--;
          if (g->u.convert_weight(g->u.weight_carried()) > 99.9 ) { // not uncommon
            hrightcol--;
            if (g->u.convert_weight(g->u.weight_carried()) > 999.9 ) {
              hrightcol--;
              if (g->u.convert_weight(g->u.weight_carried()) > 9999.9 ) { // hohum. time to consider tile destruction and sinkholes elsewhere?
                hrightcol--;
              }
            }
          }
        }
        mvwprintz( window, 4, hrightcol, c_ltgreen, "%3.1f %3d", g->u.convert_weight(g->u.weight_carried()), g->u.volume_carried() );
    } else {
        int hrightcol=rightcol; // intentionally -not- shifting rightcol since heavy items are rare, and we're stingy on screenspace
        if (g->u.convert_weight(squares[pane.area].weight) > 9.9 ) {
          hrightcol--;
          if (g->u.convert_weight(squares[pane.area].weight) > 99.9 ) { // not uncommon
            hrightcol--;
            if (g->u.convert_weight(squares[pane.area].weight) > 999.9 ) {
              hrightcol--;
              if (g->u.convert_weight(squares[pane.area].weight) > 9999.9 ) { // hohum. time to consider tile destruction and sinkholes elsewhere?
                hrightcol--;
              }
            }
          }
        }
        if ( squares[pane.area].volume > 999 ) { // pile 'o dead bears
          hrightcol--;
          if ( squares[pane.area].volume > 9999 ) { // theoretical limit; 1024*9
            hrightcol--;
          }
        }

        mvwprintz( window, 4, hrightcol, norm, "%3.1f %3d", g->u.convert_weight(squares[pane.area].weight), squares[pane.area].volume);
    }

    mvwprintz( window, 5, ( compact ? 1 : 4 ), c_ltgray, _("Name (charges)") );
    if (isinventory) {
        //~ advanced inventory; "amount", "weight", "volume"; 14 letters
        mvwprintz( window, 5, rightcol - 7, c_ltgray, _("amt weight vol") );
    } else if (isall) {
        //~ advanced inventory; "source", "weight", "volume"; 14 letters
        mvwprintz( window, 5, rightcol - 7, c_ltgray, _("src weight vol") );
    } else {
        //~ advanced inventory; "weight", "volume"; 14 letters, right-aligned
        mvwprintz( window, 5, rightcol - 7, c_ltgray, _("    weight vol") );
    }

    for(int i = page * itemsPerPage , x = 0 ; i < items.size() && x < itemsPerPage ; i++ ,x++) {
      if ( items[i].volume == -8 ) { // I'm a header!
        mvwprintz(window,6+x,( columns - items[i].name.size()-6 )/2,c_cyan, "[%s]", items[i].name.c_str() );
      } else {
        nc_color thiscolor = active ? items[i].it->color(&g->u) : norm;
        nc_color thiscolordark = c_dkgray;

        if(active && selected_index == x)
        {
            thiscolor = hilite(thiscolor);
            thiscolordark = hilite(thiscolordark);
            if ( compact ) {
                mvwprintz(window,6+x,1,thiscolor, "  %s", spaces.c_str());
            } else {
                mvwprintz(window,6+x,1,thiscolor, ">>%s", spaces.c_str());
            }

        }
        mvwprintz(window, 6 + x, ( compact ? 1 : 4 ), thiscolor, "%s", items[i].it->tname(g).c_str() );

        if(items[i].it->charges > 0) {
            wprintz(window, thiscolor, " (%d)",items[i].it->charges);
        } else if(items[i].it->contents.size() == 1 && items[i].it->contents[0].charges > 0) {
            wprintz(window, thiscolor, " (%d)",items[i].it->contents[0].charges);
        }

        if( isinventory && items[i].stacks > 1 ) {
            mvwprintz(window, 6 + x, amount_column, thiscolor, "[%d]", items[i].stacks);
        } else if ( isall ) {
            mvwprintz(window, 6 + x, amount_column, thiscolor, "%s", squares[items[i].area].shortname.c_str());
        }
//mvwprintz(window, 6 + x, amount_column-3, thiscolor, "%d", items[i].cat);
        int xrightcol=rightcol;
        if (g->u.convert_weight(items[i].weight) > 9.9 ) {
          xrightcol--;
          if (g->u.convert_weight(items[i].weight) > 99.9 ) {
            xrightcol--;
            if (g->u.convert_weight(items[i].weight) > 999.9 ) { // anything beyond this is excessive. Enjoy your clear plastic bottle of neutronium
              xrightcol--;
            }
          }
        }
        if ( items[i].volume > 999 ) { // does not exist, but can fit in 1024 tile limit
          xrightcol--;
          if ( items[i].volume > 9999 ) { // oh hey what about z levels. best give up now
            xrightcol--;
          }
        }
        mvwprintz(window, 6 + x, xrightcol, (g->u.convert_weight(items[i].weight) > 0 ? thiscolor : thiscolordark),
            "%3.1f", g->u.convert_weight(items[i].weight) );

        wprintz(window, (items[i].volume > 0 ? thiscolor : thiscolordark), " %3d", items[i].volume );
        if(active && items[i].autopickup==true) {
          mvwprintz(window,6+x,1, magenta_background(items[i].it->color(&g->u)),"%s",(compact?items[i].it->tname(g).substr(0,1):">").c_str());
        }
      }
    }


}

// should probably move to an adv_inv_pane class

enum advanced_inv_sortby {
    SORTBY_NONE = 1 , SORTBY_NAME, SORTBY_WEIGHT, SORTBY_VOLUME, SORTBY_CHARGES, SORTBY_CATEGORY, NUM_SORTBY
};

struct advanced_inv_sort_case_insensitive_less : public std::binary_function< char,char,bool > {
    bool operator () (char x, char y) const {
        return toupper( static_cast< unsigned char >(x)) < toupper( static_cast< unsigned char >(y));
    }
};

struct advanced_inv_sorter {
    int sortby;
    advanced_inv_sorter(int sort) { sortby=sort; };
    bool operator()(const advanced_inv_listitem& d1, const advanced_inv_listitem& d2) {
        if ( sortby != SORTBY_NAME ) {
            switch(sortby) {
                case SORTBY_WEIGHT: {
                    if ( d1.weight != d2.weight ) return d1.weight > d2.weight;
                    break;
                }
                case SORTBY_VOLUME: {
                    if ( d1.volume != d2.volume ) return d1.volume > d2.volume;
                    break;
                }
                case SORTBY_CHARGES: {
                    if ( d1.it->charges != d2.it->charges ) return d1.it->charges > d2.it->charges;
                    break;
                }
                case SORTBY_CATEGORY: {
                    if ( d1.cat != d2.cat ) {
                      return d1.cat < d2.cat;
                    } else if ( d1.volume == -8 ) {
                      return true;
                    } else if ( d2.volume == -8 ) {
                      return false;
                    }
                    break;
                }
                default: return d1.idx > d2.idx; break;
            };
        }
        // secondary sort by name
        std::string n1=d1.name;
        std::string n2=d2.name;
        return std::lexicographical_compare( n1.begin(), n1.end(),
            n2.begin(), n2.end(), advanced_inv_sort_case_insensitive_less() );
    };
};

void advanced_inv_menu_square(advanced_inv_area* squares, uimenu *menu ) {
    int ofs=-25-4;
    int sel=menu->selected+1;
    for ( int i=1; i < 10; i++ ) {
        char key=(char)(i+48);
        char bracket[3]="[]";
        if ( squares[i].vstor >= 0 ) strcpy(bracket,"<>");
        bool canputitems=( squares[i].canputitems && menu->entries[i-1].enabled ? true : false);
        nc_color bcolor = ( canputitems ? ( sel == i ? h_cyan : c_cyan ) : c_dkgray );
        nc_color kcolor = ( canputitems ? ( sel == i ? h_ltgreen : c_ltgreen ) : c_dkgray );
        mvwprintz(menu->window,squares[i].hscreenx+5,squares[i].hscreeny+ofs, bcolor, "%c", bracket[0]);
        wprintz(menu->window, kcolor, "%c", key);
        wprintz(menu->window, bcolor, "%c", bracket[1]);
    }
}

void advanced_inv_print_header(advanced_inv_area* squares, advanced_inv_pane &pane, int sel=-1 )
{
    WINDOW* window=pane.window;
    int area=pane.area;
    int wwidth=getmaxx(window);
    int ofs=wwidth-25-2-14;
    for ( int i=0; i < 11; i++ ) {
        char key=( i == 0 ? 'I' : ( i == 10 ? 'A' : (char)(i+48) ) );
        char bracket[3]="[]";
        if ( squares[i].vstor >= 0 ) strcpy(bracket,"<>");
        nc_color bcolor = ( squares[i].canputitems ? ( area == i || ( area == 10 && i != 0 ) ? c_cyan : c_ltgray ) : c_red );
        nc_color kcolor = ( squares[i].canputitems ? ( area == i ? c_ltgreen : ( i == sel ? c_cyan : c_ltgray ) ) : c_red );
        mvwprintz(window,squares[i].hscreenx,squares[i].hscreeny+ofs, bcolor, "%c", bracket[0]);
        wprintz(window, kcolor, "%c", key);
        wprintz(window, bcolor, "%c", bracket[1]);
    }
}

void advanced_inv_update_area( advanced_inv_area &area, game *g ) {
    int i = area.id;
    player u = g->u;
    area.x = g->u.posx+area.offx;
    area.y = g->u.posy+area.offy;
    area.size = 0;
    area.veh = NULL;
    area.vstor = -1;
    area.desc = "";
    if( i > 0 && i < 10 ) {
        int vp = 0;
        area.veh = g->m.veh_at( u.posx+area.offx,u.posy+area.offy, vp );
        if ( area.veh ) {
            area.vstor = area.veh->part_with_feature(vp, vpf_cargo, false);
        }
        if ( area.vstor >= 0 ) {
            area.desc = area.veh->name;
            area.canputitems=true;
            area.size = area.veh->parts[area.vstor].items.size();
            area.max_size = MAX_ITEM_IN_VEHICLE_STORAGE;
            area.max_volume = area.veh->max_volume(area.vstor);
        } else {
            area.canputitems=(!(g->m.has_flag(noitem,u.posx+area.offx,u.posy+area.offy)) && !(g->m.has_flag(sealed,u.posx+area.offx,u.posy+area.offy) ));
            area.size = g->m.i_at(u.posx+area.offx,u.posy+area.offy).size();
            area.max_size = MAX_ITEM_IN_SQUARE;
            area.max_volume = g->m.max_volume(u.posx+area.offx,u.posy+area.offy);
            if (g->m.graffiti_at(u.posx+area.offx,u.posy+area.offy).contents) {
                area.desc = g->m.graffiti_at(u.posx+area.offx,u.posy+area.offy).contents->c_str();
            }
        }
    } else if ( i == 0 ) {
        area.size=u.inv.size();
        area.canputitems=true;
    } else {
        area.desc = _("All 9 squares");
        area.canputitems=true;
    }
    area.volume=0; // must update in main function
    area.weight=0; // must update in main function
}

int advanced_inv_getinvcat(item *it) {
    if ( it->is_gun() ) return 0;
    if ( it->is_ammo() ) return 1;
    if ( it->is_weap() ) return 2;
    if ( it->is_tool() ) return 3;
    if ( it->is_armor() ) return 4;
    if ( it->is_food_container() ) return 5;
    if ( it->is_food() ) {
        it_comest* comest = dynamic_cast<it_comest*>(it->type);
        return ( comest->comesttype != "MED" ? 5 : 6 );
    }
    if ( it->is_book() ) return 7;
    if (it->is_gunmod() || it->is_bionic()) return 8;
    return 9;
}

std::string center_text(const char *str, int width)
{
    std::string spaces;
    int numSpaces = width - strlen(str);
    for (int i = 0; i < numSpaces / 2; i++) {
        spaces += " ";
    }
    return spaces + std::string(str);
}

void game::advanced_inv()
{
    u.inv.sort();
    u.inv.restack(&u);

    const int head_height = 5;
    const int min_w_height = 10;
    const int min_w_width = FULL_SCREEN_WIDTH;
    const int max_w_width = 120;

    const int left = 0;  // readability, should be #define..
    const int right = 1;
    const int isinventory = 0;
    const int isall = 10;
    std::string sortnames[8] = { "-none-", _("none"), _("name"), _("weight"), _("volume"), _("charges"), _("category"), "-" };
    std::string invcats[10] = { _("guns"), _("ammo"), _("weapons"), _("tools"), _("clothing"), _("food"), _("drugs"), _("books"), _("mods"), _("other") };
    bool checkshowmsg=false;
    bool showmsg=false;

    int itemsPerPage = 10;
    int w_height = (TERMY<min_w_height+head_height) ? min_w_height : TERMY-head_height;
    int w_width = (TERMX<min_w_width) ? min_w_width : (TERMX>max_w_width) ? max_w_width : (int)TERMX;

    int headstart = 0; //(TERMY>w_height)?(TERMY-w_height)/2:0;
    int colstart = (TERMX > w_width) ? (TERMX - w_width)/2 : 0;
    WINDOW *head = newwin(head_height,w_width, headstart, colstart);
    WINDOW *left_window = newwin(w_height,w_width/2, headstart+head_height,colstart);
    WINDOW *right_window = newwin(w_height,w_width/2, headstart+head_height,colstart+w_width/2);

    itemsPerPage=getmaxy(left_window)-ADVINVOFS;
    // todo: awaiting ui::menu // last_tmpdest=-1;
    bool exit = false;
    bool redraw = true;
    bool recalc = true;
    int lastCh = 0;

    advanced_inv_area squares[11] = {
        {0, 2, 25, 0, 0, 0, 0, _("Inventory"), "IN", false, NULL, -1, 0, "", 0, 0, 0, 0 },
        {1, 3, 30, -1, 1, 0, 0, _("South West"), "SW", false, NULL, -1, 0, "", 0, 0, 0, 0 },
        {2, 3, 33, 0, 1, 0, 0, _("South"), "S", false, NULL, -1, 0, "", 0, 0, 0, 0 },
        {3, 3, 36, 1, 1, 0, 0, _("South East"), "SE", false, NULL, -1, 0, "", 0, 0, 0, 0 },
        {4, 2, 30, -1, 0, 0, 0, _("West"), "W", false, NULL, -1, 0, "", 0, 0, 0, 0 },
        {5, 2, 33, 0, 0, 0, 0, _("Directly below you"), "DN", false, NULL, -1, 0, "", 0, 0, 0, 0 },
        {6, 2, 36, 1, 0, 0, 0, _("East"), "E", false, NULL, -1, 0, "", 0, 0, 0, 0 },
        {7, 1, 30, -1, -1, 0, 0, _("North West"), "NW", false, NULL, -1, 0, "", 0, 0, 0, 0 },
        {8, 1, 33, 0, -1, 0, 0, _("North"), "N", false, NULL, -1, 0, "", 0, 0, 0, 0 },
        {9, 1, 36, 1, -1, 0, 0, _("North East"), "NE", false, NULL, -1, 0, "", 0, 0, 0, 0 },
        {10, 3, 25, 0, 0, 0, 0, _("Surrounding area"), "AL", false, NULL, -1, 0, "", 0, 0, 0, 0 }
    };

    for ( int i = 0; i < 11; i++ ) {
        advanced_inv_update_area(squares[i], this);
    }


    std::vector<advanced_inv_listitem> listitem_stub;
    advanced_inv_pane panes[2] = {
        {0,  5, 0, 0, 0, -1,  0, 0, 0, 0,  _("Initializing..."), 1, 0, NULL, NULL, listitem_stub, 0},
        {1,  isinventory, 0, 0, 0, -1,  0, 0, 0, 0,  _("Initializing..."), 1, 0, NULL, NULL, listitem_stub, 0},
    };

    panes[left].sortby = uistate.adv_inv_leftsort;
    panes[right].sortby = uistate.adv_inv_rightsort;
    panes[left].area = uistate.adv_inv_leftarea;
    panes[right].area = uistate.adv_inv_rightarea;
    bool moved=( uistate.adv_inv_last_coords.x != u.posx || uistate.adv_inv_last_coords.y != u.posy );
    if ( !moved || panes[left].area == isinventory ) {
        panes[left].index = uistate.adv_inv_leftindex;
        panes[left].page = uistate.adv_inv_leftpage;
    }
    if ( !moved || panes[right].area == isinventory ) {
        panes[right].index = uistate.adv_inv_rightindex;
        panes[right].page = uistate.adv_inv_rightpage;
    }

    panes[left].window = left_window;
    panes[right].window = right_window;

    int src = left; // the active screen , 0 for left , 1 for right.
    int dest = right;
    int max_inv = inv_chars.size() - u.worn.size() - ( u.is_armed() || u.weapon.is_style() ? 1 : 0 );
    bool examineScroll = false;

    while(!exit)
    {
        dest = (src==left ? right : left);
        if ( recalc ) redraw=true;
        if(redraw)
        {
            max_inv = inv_chars.size() - u.worn.size() - ( u.is_armed() || u.weapon.is_style() ? 1 : 0 );
            for (int i = 0; i < 2; i++) {
                int idest = (i==left ? right : left);

                // calculate the offset.
                getsquare(panes[i].area, panes[i].offx, panes[i].offy, panes[i].area_string, squares);

                if(recalc) {

                    panes[i].items.clear();
                    bool hascat[10]={false,false,false,false,false,false,false,false,false,false};
                    panes[i].numcats=0;
                    int avolume=0;
                    int aweight=0;
                    if(panes[i].area == isinventory) {

                        invslice stacks = u.inv.slice(0, u.inv.size());
                        for( int x = 0; x < stacks.size(); ++x ) {
                            item& item = stacks[x]->front();
                            advanced_inv_listitem it;
                            it.idx=x;
                            // todo: for the love of gods create a u.inv.stack_by_int()
                            int size = u.inv.stack_by_letter(item.invlet).size();
                            if ( size < 1 ) size = 1;
                            it.name=item.tname(this);
                            it.autopickup=hasPickupRule(it.name);
                            it.stacks=size;
                            it.weight=item.weight() * size;
                            it.volume=item.volume() * size;
                            it.cat=advanced_inv_getinvcat(&item);
                            it.it=&item;
                            it.area=panes[i].area;
                            if( !hascat[it.cat] ) {
                                hascat[it.cat]=true;
                                panes[i].numcats++;
                                if(panes[i].sortby == SORTBY_CATEGORY) {
                                    advanced_inv_listitem itc;
                                    itc.idx=-8; itc.stacks=-8; itc.weight=-8; itc.volume=-8;
                                    itc.cat=it.cat; itc.name=invcats[it.cat];
                                    itc.area=panes[i].area;
                                    panes[i].items.push_back(itc);
                                }
                            }
                            avolume+=it.volume;
                            aweight+=it.weight;
                            panes[i].items.push_back(it);
                        }
                    } else {

                        int s1 = panes[i].area;
                        int s2 = panes[i].area;
                        if ( panes[i].area == isall ) {
                            s1 = 1;
                            s2 = 9;
                        }
                        for(int s = s1; s <= s2; s++) {
                            int savolume=0;
                            int saweight=0;
                            advanced_inv_update_area(squares[s], this);
                            //mvprintw(s+(i*10), 0, "%d %d                                   ",i,s);
                            if( panes[idest].area != s && squares[s].canputitems ) {
                                std::vector<item>& items = squares[s].vstor >= 0 ?
                                    squares[s].veh->parts[squares[s].vstor].items :
                                    m.i_at(squares[s].x , squares[s].y );
                                for(int x = 0; x < items.size() ; x++) {
                                    advanced_inv_listitem it;
                                    it.idx=x;
                                    it.name=items[x].tname(this);
                                    it.autopickup=hasPickupRule(it.name);
                                    it.stacks=1;
                                    it.weight=items[x].weight();
                                    it.volume=items[x].volume();
                                    it.cat=advanced_inv_getinvcat(&items[x]);
                                    it.it=&items[x];
                                    it.area=s;
                                    if( ! hascat[it.cat] ) {
                                        hascat[it.cat]=true;
                                        panes[i].numcats++;
                                        if(panes[i].sortby == SORTBY_CATEGORY) {
                                            advanced_inv_listitem itc;
                                            itc.idx=-8; itc.stacks=-8; itc.weight=-8; itc.volume=-8;
                                            itc.cat=it.cat; itc.name=invcats[it.cat]; itc.area=s;
                                            panes[i].items.push_back(itc);
                                        }
                                    }

                                    savolume+=it.volume;
                                    saweight+=it.weight;
                                    panes[i].items.push_back(it);

                                } // for(int x = 0; x < items.size() ; x++)

                            } // if( panes[idest].area != s && squares[s].canputitems )
                            avolume+=savolume;
                            aweight+=saweight;
                        } // for(int s = s1; s <= s2; s++)

                    } // if(panes[i].area ?? isinventory)

                    advanced_inv_update_area(squares[panes[i].area], this);

                    squares[panes[i].area].volume = avolume;
                    squares[panes[i].area].weight = aweight;

                    panes[i].veh = squares[panes[i].area].veh; // <--v-- todo deprecate
                    panes[i].vstor = squares[panes[i].area].vstor;
                    panes[i].size = panes[i].items.size();

                    // sort the stuff
                    switch(panes[i].sortby) {
                        case SORTBY_NONE:
                            if ( i != isinventory ) {
                                std::sort( panes[i].items.begin(), panes[i].items.end(), advanced_inv_sorter(SORTBY_NONE) );
                            }
                            break;
                        default:
                            std::sort( panes[i].items.begin(), panes[i].items.end(), advanced_inv_sorter( panes[i].sortby ) );
                            break;
                    }
                }

                // paginate (not sure why)
                panes[i].max_page = (int)ceil(panes[i].size/(itemsPerPage+0.0)); //(int)ceil(panes[i].size/20.0);
                panes[i].max_index = panes[i].page == (-1 + panes[i].max_page) ? ((panes[i].size % itemsPerPage)==0?itemsPerPage:panes[i].size % itemsPerPage) : itemsPerPage;
                // check if things are out of bound
                panes[i].index = (panes[i].index >= panes[i].max_index) ? panes[i].max_index - 1 : panes[i].index;


                panes[i].page = panes[i].max_page == 0 ? 0 : ( panes[i].page >= panes[i].max_page ? panes[i].max_page - 1 : panes[i].page);

                if( panes[i].sortby == SORTBY_CATEGORY && panes[i].items.size() > 0 ) {
                  int lpos = panes[i].index + (panes[i].page * itemsPerPage);
                  if ( lpos < panes[i].items.size() && panes[i].items[lpos].volume == -8 ) {
                     panes[i].index += ( panes[i].index+1 >= itemsPerPage ? -1 : 1 );
                  }
                }

                // draw the stuff
                werase(panes[i].window);



                advprintItems( panes[i], squares, (src == i), this );

                int sel=-1;
                if ( panes[i].size > 0 ) sel = panes[i].items[panes[i].index].area;

                advanced_inv_print_header(squares,panes[i], sel );
                // todo move --v to --^
                mvwprintz(panes[i].window,1,2,src == i ? c_cyan : c_ltgray, "%s", panes[i].area_string.c_str());
                mvwprintz(panes[i].window, 2, 2, src == i ? c_green : c_dkgray , "%s", squares[panes[i].area].desc.c_str() );

            }

            recalc=false;

            werase(head);
            {
                wborder(head,LINE_XOXO,LINE_XOXO,LINE_OXOX,LINE_OXOX,LINE_OXXO,LINE_OOXX,LINE_XXOO,LINE_XOOX);
                int line=1;
                if( checkshowmsg || showmsg ) {
                  for (int i = messages.size() - 1; i >= 0 && line < 4; i--) {
                    std::string mes = messages[i].message;
                    if (messages[i].count > 1) {
                      std::stringstream mesSS;
                      mesSS << mes << " x " << messages[i].count;
                      mes = mesSS.str();
                    }
                    nc_color col = c_dkgray;
                    if (int(messages[i].turn) >= curmes) {
                       col = c_ltred;
                       showmsg=true;
                    } else {
                       col = c_ltgray;
                    }
                    if ( showmsg ) mvwprintz(head, line, 2, col, mes.c_str());
                    line++;
                  }
                }
                if ( ! showmsg ) {
                  mvwprintz(head,0,w_width-18,c_white,_("< [?] show log >"));
                  mvwprintz(head,1,2, c_white, _("hjkl or arrow keys to move cursor, [m]ove item between panes,"));
                  mvwprintz(head,2,2, c_white, _("1-9 (or GHJKLYUBNI) to select square for active tab, 0 for inventory,"));
                  mvwprintz(head,3,2, c_white, _("[e]xamine item,  [s]ort display, toggle auto[p]ickup, [q]uit."));
                } else {
                  mvwprintz(head,0,w_width-19,c_white,"< [?] show help >");
                }
            }

            if(panes[src].max_page > 1 ) {
                mvwprintz(panes[src].window, 4, 2, c_ltblue, _("[<] page %d of %d [>]"), panes[src].page+1, panes[src].max_page);
            }
            redraw = false;
        }

        int list_pos = panes[src].index + (panes[src].page * itemsPerPage);
        int item_pos = panes[src].size > 0 ? panes[src].items[list_pos].idx : 0;
        // todo move
        for (int i = 0; i < 2; i++) {
            if ( src == i ) {
                wattron(panes[i].window, c_cyan);
            }
            wborder(panes[i].window,LINE_XOXO,LINE_XOXO,LINE_OXOX,LINE_OXOX,LINE_OXXO,LINE_OOXX,LINE_XXOO,LINE_XOOX);
            mvwprintw(panes[i].window, 0, 3, _("< [s]ort: %s >"), sortnames[ ( panes[i].sortby <= 6 ? panes[i].sortby : 0 ) ].c_str() );
            int max=( panes[i].area == isinventory ? max_inv : MAX_ITEM_IN_SQUARE );
            if ( panes[i].area == isall ) max *= 9;
            int fmtw=7 + ( panes[i].size > 99 ? 3 : panes[i].size > 9 ? 2 : 1 ) + ( max > 99 ? 3 : max > 9 ? 2 : 1 );
            mvwprintw(panes[i].window,0 ,(w_width/2)-fmtw,"< %d/%d >", panes[i].size, max );
            if ( src == i ) {
                wattroff(panes[i].window, c_white);
            }
        }

        if (!examineScroll) {
            wrefresh(head);
            wrefresh(panes[right].window);
        }
        wrefresh(panes[left].window);
        examineScroll = false;

        int changex = -1;
        int changey = 0;
        bool donothing = false;


        int c = lastCh ? lastCh : getch();
        lastCh = 0;
        int changeSquare;

        if(c == 'i')
            c = (char)'0';

        if(c == 'a' ) c = (char)'a';

        changeSquare = getsquare((char)c, panes[src].offx, panes[src].offy, panes[src].area_string, squares);

        if(changeSquare != -1)
        {
            if(panes[left].area == changeSquare || panes[right].area == changeSquare) // do nthing
            {
                lastCh = (int)popup_getkey(_("same square!"));
                if(lastCh == 'q' || lastCh == KEY_ESCAPE || lastCh == ' ' ) lastCh=0;
            }
            else if(squares[changeSquare].canputitems)
            {
                panes[src].area = changeSquare;
                panes[src].page = 0;
                panes[src].index = 0;
            }
            else
            {
                popup(_("You can't put items there"));
            }
            recalc = true;
        }
        else if('m' == c || 'M' == c)
        {
            // If the active screen has no item.
            if( panes[src].size == 0 ) {
                continue;
            } else if ( item_pos == -8 ) {
                continue; // category header
            }
            int destarea = panes[dest].area;
            if ( panes[dest].area == isall || 'M' == c ) {
                // popup("Choose a specific square in the destination window.");  continue;
                bool valid=false;
                uimenu m; /* using new uimenu class */
                m.text=_("Select destination");
                m.pad_left=9; /* free space for advanced_inv_menu_square */

                for(int i=1; i < 10; i++) {
                    std::string prefix = string_format("%2d/%d",
                            squares[i].size, MAX_ITEM_IN_SQUARE);
                    if (squares[i].size >= MAX_ITEM_IN_SQUARE) {
                        prefix += _(" (FULL)");
                    }
                    m.entries.push_back( uimenu_entry( /* std::vector<uimenu_entry> */
                        i, /* return value */
                        (squares[i].canputitems && i != panes[src].area), /* enabled */
                        i+48, /* hotkey */
                        prefix + " " +
                          squares[i].name + " " +
                          ( squares[i].vstor >= 0 ? squares[i].veh->name : "" ) /* entry text */
                    ) );
                }

                m.selected=uistate.adv_inv_last_popup_dest-1; // selected keyed to uimenu.entries, which starts at 0;
                m.show(); // generate and show window.
                while ( m.ret == UIMENU_INVALID && m.keypress != 'q' && m.keypress != KEY_ESCAPE ) {
                    advanced_inv_menu_square(squares, &m ); // render a fancy ascii grid at the left of the menu
                    m.query(false); // query, but don't loop
                }
                if ( m.ret >= 0 && m.ret <= 9 ) { // is it a square?
                    if ( m.ret == panes[src].area ) { // should never happen, but sanity checks keep developers sane.
                        popup(_("Can't move stuff to the same place."));
                    } else if ( ! squares[m.ret].canputitems ) { // this was also disabled in it's uimenu_entry
                        popup(_("Invalid. Like the menu said."));
                    } else {
                        destarea = m.ret;
                        valid=true;
                        uistate.adv_inv_last_popup_dest=m.ret;
                    }
                }
                if ( ! valid ) continue;
            }
// from inventory
            if(panes[src].area == isinventory) {

                int max = (squares[destarea].max_size - squares[destarea].size);
                int volmax = max;
                int free_volume = ( squares[ destarea ].vstor >= 0 ?
                    squares[ destarea ].veh->free_volume( squares[ destarea ].vstor ) :
                    m.free_volume ( squares[ destarea ].x, squares[ destarea ].y )
                ) * 100;
                // TODO figure out a better way to get the item. Without invlets.
                item* it = &u.inv.slice(item_pos, 1).front()->front();
                std::list<item>& stack = u.inv.stack_by_letter(it->invlet);

                int amount = 1;
                int volume = it->volume() * 100; // sigh

                bool askamount = false;
                if ( stack.size() > 1) {
                    amount = stack.size();
                    askamount = true;
                } else if ( it->count_by_charges() ) {
                    amount = it->charges;
                    volume = it->type->volume;
                    askamount = true;
                }

                if ( volume > 0 && volume * amount > free_volume ) {
                    volmax = int( free_volume / volume );
                    if ( volmax == 0 ) {
                        popup(_("Destination area is full. Remove some items first."));
                        continue;
                    }
                    if ( stack.size() > 1) {
                        max = ( volmax < max ? volmax : max );
                    } else if ( it->count_by_charges() ) {
                        max = volmax;
                    }
                } else if ( it->count_by_charges() ) {
                    max = amount;
                }
                if ( max == 0 ) {
                    popup(_("Destination area has too many items. Remove some first."));
                    continue;
                }
                if ( askamount ) {
                    std::string popupmsg=_("How many do you want to move? (0 to cancel)");
                    if(amount > max) {
                        popupmsg=_("Destination can only hold ") + helper::to_string(max) + _("! Move how many? (0 to cancel) ");
                    }
                    // fixme / todo make popup take numbers only (m = accept, q = cancel)
                    amount = helper::to_int(
                        string_input_popup( popupmsg, 20,
                             helper::to_string(
                                 ( amount > max ? max : amount )
                             )
                        )
                    );
                }
                recalc=true;
                if(stack.size() > 1) { // if the item is stacked
                    if ( amount != 0 && amount <= stack.size() ) {
                        amount = amount > max ? max : amount;
                        std::list<item> moving_items = u.inv.remove_partial_stack(it->invlet,amount);
                        bool chargeback=false;
                        int moved=0;
                        for(std::list<item>::iterator iter = moving_items.begin();
                            iter != moving_items.end();
                            ++iter)
                        {

                          if ( chargeback == true ) {
                                u.i_add(*iter,this);
                          } else {
                            if(squares[destarea].vstor >= 0) {
                                if(squares[destarea].veh->add_item(squares[destarea].vstor,*iter) == false) {
                                    // testme
                                    u.i_add(*iter,this);
                                    popup(_("Destination full. %d / %d moved. Please report a bug if items have vanished."),moved,amount);
                                    chargeback=true;
                                }
                            } else {
                                if(m.add_item_or_charges(squares[destarea].x, squares[destarea].y, *iter, 0) == false) {
                                    // testme
                                    u.i_add(*iter,this);
                                    popup(_("Destination full. %d / %d moved. Please report a bug if items have vanished."),moved,amount);
                                    chargeback=true;
                                }
                            }
                            moved++;
                          }
                        }
                        if ( moved != 0 ) u.moves -= 100;
                    }
                } else if(it->count_by_charges()) {
                    if(amount != 0 && amount <= it->charges ) {

                        item moving_item = u.inv.remove_item_by_charges(it->invlet,amount);

                        if (squares[destarea].vstor>=0) {
                            if(squares[destarea].veh->add_item(squares[destarea].vstor,moving_item) == false) {
                                // fixme add item back
                                u.i_add(moving_item,this);
                                popup(_("Destination full. Please report a bug if items have vanished."));
                                continue;
                            }
                        } else {
                            if ( m.add_item_or_charges(squares[destarea].x, squares[destarea].y, moving_item, 0) == false ) {
                                // fixme add item back
                                u.i_add(moving_item,this);
                                popup(_("Destination full. Please report a bug if items have vanished."));
                                continue;
                            }
                        }
                        u.moves -= 100;
                    }
                } else {
                    item moving_item = u.inv.remove_item_by_letter(it->invlet);
                    if(squares[destarea].vstor>=0) {
                        if(squares[destarea].veh->add_item(squares[destarea].vstor, moving_item) == false) {
                           // fixme add item back (test)
                           u.i_add(moving_item,this);
                           popup(_("Destination full. Please report a bug if items have vanished."));
                           continue;
                        }
                    } else {
                        if(m.add_item_or_charges(squares[destarea].x, squares[destarea].y, moving_item) == false) {
                           // fixme add item back (test)
                           u.i_add(moving_item,this);
                           popup(_("Destination full. Please report a bug if items have vanished."));
                           continue;
                        }
                    }
                    u.moves -= 100;
                }
// from map / vstor
            } else {
                int s;
                if(panes[src].area == isall) {
                    s = panes[src].items[list_pos].area;
                    // todo: phase out these vars? ---v // temp_fudge pending tests/cleanup
                    panes[src].offx = squares[s].offx;
                    panes[src].offy = squares[s].offy;
                    panes[src].vstor = squares[s].vstor;
                    panes[src].veh = squares[s].veh;
                    recalc = true;
                } else {
                    s = panes[src].area;
                }
                if ( s == destarea ) {
                    popup(_("Source area is the same as destination (%s)."),squares[destarea].name.c_str());
                    continue;
                }

                std::vector<item> src_items = squares[s].vstor >= 0 ?
                    squares[s].veh->parts[squares[s].vstor].items :
                    m.i_at(squares[s].x,squares[s].y);
                if(src_items[item_pos].made_of(LIQUID))
                {
                    popup(_("You can't pick up a liquid."));
                    continue;
                }
                else // from veh/map
                {
                    if ( destarea == isinventory ) // if destination is inventory
                    {
                        if(!u.can_pickVolume(src_items[item_pos].volume()))
                        {
                            popup(_("There's no room in your inventory."));
                            continue;
                        }
                        else if(!u.can_pickWeight(src_items[item_pos].weight(), false))
                        {
                            popup(_("This is too heavy!"));
                            continue;
                        }
                        else if(squares[destarea].size >= max_inv)
                        {
                            popup(_("Too many itens"));
                            continue;
                        }
                    }

                    recalc=true;

                    item new_item = src_items[item_pos];

                    if(destarea == isinventory) {
                        new_item.invlet = nextinv;
                        advance_nextinv();
                        u.i_add(new_item,this);
                        u.moves -= 100;
                    } else if (squares[destarea].vstor >= 0) {
                        if( squares[destarea].veh->add_item( squares[destarea].vstor, new_item ) == false) {
                            popup(_("Destination area is full. Remove some items first"));
                            continue;
                        }
                    } else {
                        if ( m.add_item_or_charges(squares[destarea].x, squares[destarea].y, new_item, 0 ) == false ) {
                            popup(_("Destination area is full. Remove some items first"));
                            continue;
                        }
                    }
                    if(panes[src].vstor>=0) {
                        panes[src].veh->remove_item (panes[src].vstor, item_pos);
                    } else {
                        m.i_rem(u.posx+panes[src].offx,u.posy+panes[src].offy, item_pos);
                    }
                }
            }
        } else if('?' == c) {
            showmsg=(!showmsg);
            checkshowmsg=false;
            redraw=true;
        } else if('s' == c) {
            // int ch = uimenu(true, "Sort by... ", "Unsorted (recently added first)", "name", "weight", "volume", "charges", NULL );
            redraw=true;
            uimenu sm; /* using new uimenu class */
            sm.text=_("Sort by... ");
            sm.entries.push_back(uimenu_entry(SORTBY_NONE, true, 'u', _("Unsorted (recently added first)") ));
            sm.entries.push_back(uimenu_entry(SORTBY_NAME, true, 'n', sortnames[SORTBY_NAME]));
            sm.entries.push_back(uimenu_entry(SORTBY_WEIGHT, true, 'w', sortnames[SORTBY_WEIGHT]));
            sm.entries.push_back(uimenu_entry(SORTBY_VOLUME, true, 'v', sortnames[SORTBY_VOLUME]));
            sm.entries.push_back(uimenu_entry(SORTBY_CHARGES, true, 'x', sortnames[SORTBY_CHARGES]));
            sm.entries.push_back(uimenu_entry(SORTBY_CATEGORY, true, 'c', sortnames[SORTBY_CATEGORY]));
            sm.selected=panes[src].sortby-1; /* pre-select current sort. uimenu.selected is entries[index] (starting at 0), not return value */
            sm.query(); /* calculate key and window variables, generate window, and loop until we get a valid answer */
            if(sm.ret < 1) continue; /* didn't get a valid answer =[ */
            panes[src].sortby = sm.ret;

            if ( src == left ) {
                uistate.adv_inv_leftsort=sm.ret;
            } else {
                uistate.adv_inv_rightsort=sm.ret;
            }
            recalc = true;
        } else if('p' == c) {
            if(panes[src].size == 0) {
                continue;
            } else if ( item_pos == -8 ) {
                continue; // category header
            }
            if ( panes[src].items[list_pos].autopickup == true ) {
                removePickupRule(panes[src].items[list_pos].name);
                panes[src].items[list_pos].autopickup=false;
            } else {
                addPickupRule(panes[src].items[list_pos].name);
                panes[src].items[list_pos].autopickup=true;
            }
            redraw = true;
        } else if('e' == c) {
            if(panes[src].size == 0) {
                continue;
            } else if ( item_pos == -8 ) {
                continue; // category header
            }
            item *it = panes[src].items[list_pos].it;
            int ret=0;
            if(panes[src].area == isinventory ) {
                char pleaseDeprecateMe=it->invlet;
                ret=inventory_item_menu(pleaseDeprecateMe, 0, w_width/2
                   // fixme: replace compare_split_screen_popup which requires y=0 for item menu to function right
                   // colstart + ( src == left ? w_width/2 : 0 ), 50
                );
                recalc=true;
                checkshowmsg=true;
            } else {
                std::vector<iteminfo> vThisItem, vDummy, vMenu;
                it->info(true, &vThisItem, this);
                int rightWidth = w_width / 2 - 2;
                vThisItem.push_back(iteminfo(_("DESCRIPTION"), "\n"));
                vThisItem.push_back(iteminfo(_("DESCRIPTION"), center_text(_("[up / page up] previous"), rightWidth - 4)));
                vThisItem.push_back(iteminfo(_("DESCRIPTION"), center_text(_("[down / page down] next"), rightWidth - 4)));
                ret=compare_split_screen_popup( 1 + colstart + ( src == isinventory ? w_width/2 : 0 ),
                    rightWidth, 0, it->tname(this), vThisItem, vDummy );
            }
            if ( ret == KEY_NPAGE || ret == KEY_DOWN ) {
                changey += 1;
                lastCh='e';
            } else if ( ret == KEY_PPAGE || ret == KEY_UP ) {
                changey += -1;
                lastCh='e';
            }
            if (changey) { examineScroll = true; }
            redraw = true;
        }
        else if( 'q' == c || KEY_ESCAPE == c || ' ' == c )
        {
            exit = true;
        }
        else if('>' == c || KEY_NPAGE == c)
        {
            panes[src].page++;
            if( panes[src].page >= panes[src].max_page ) panes[src].page = 0;
            redraw = true;
        }
        else if('<' == c || KEY_PPAGE == c)
        {
            panes[src].page--;
            if( panes[src].page < 0 ) panes[src].page = panes[src].max_page;
            redraw = true;
        }
        else
        {
            switch(c)
            {
                case 'j':
                case KEY_DOWN:
                    changey = 1;
                    break;
                case 'k':
                case KEY_UP:
                    changey = -1;
                    break;
                case 'h':
                case KEY_LEFT:
                    changex = 0;
                    break;
                case 'l':
                case KEY_RIGHT:
                    changex = 1;
                    break;
                case '\t':
                    changex = dest;
                    break;
                default :
                    donothing = true;
                    break;
            }
        }
        if(!donothing)
        {
          if ( changey != 0 ) {
            for ( int l=2; l > 0; l-- ) {
              panes[src].index += changey;
              if ( panes[src].index < 0 ) {
                  panes[src].page--;
                  if( panes[src].page < 0 ) {
                    panes[src].page = panes[src].max_page-1;
                    panes[src].index = panes[src].items.size() - 1 - ( panes[src].page * itemsPerPage );
                  } else {
                    panes[src].index = itemsPerPage; // corrected at the start of next iteration
                  }
              } else if ( panes[src].index >= panes[src].max_index ) {
                  panes[src].page++;
                  if( panes[src].page >= panes[src].max_page ) panes[src].page = 0;
                  panes[src].index = 0;
              }
              int lpos=panes[src].index + (panes[src].page * itemsPerPage);
              if ( lpos < panes[src].items.size() && panes[src].items[lpos].volume != -8 ) {
                  l=0;
              }

            }
            redraw = true;
          }
          if ( changex >= 0 ) {
            src = changex;
            redraw = true;
          }
        }
    }

    uistate.adv_inv_last_coords.x = u.posx;
    uistate.adv_inv_last_coords.y = u.posy;
    uistate.adv_inv_leftarea = panes[left].area;
    uistate.adv_inv_rightarea = panes[right].area;
    uistate.adv_inv_leftindex = panes[left].index;
    uistate.adv_inv_leftpage = panes[left].page;
    uistate.adv_inv_rightindex = panes[right].index;
    uistate.adv_inv_rightpage = panes[right].page;

    werase(head);
    werase(panes[left].window);
    werase(panes[right].window);
    delwin(head);
    delwin(panes[left].window);
    delwin(panes[right].window);
    refresh_all();
}

//Shift player by one tile, look_around(), then restore previous position.
//represents carfully peeking around a corner, hence the large move cost.
void game::peek()
{
    int prevx, prevy, peekx, peeky;

    if (!choose_adjacent(_("Peek"), peekx, peeky))
        return;

    if (m.move_cost(peekx, peeky) == 0)
        return;

    u.moves -= 200;
    prevx = u.posx;
    prevy = u.posy;
    u.posx = peekx;
    u.posy = peeky;
    look_around();
    u.posx = prevx;
    u.posy = prevy;
}
////////////////////////////////////////////////////////////////////////////////////////////
point game::look_debug(point coords) {
  editmap * edit=new editmap(this);
  point ret=edit->edit(coords);
  delete edit;
  edit=0;
  return ret;
}
////////////////////////////////////////////////////////////////////////////////////////////
point game::look_around()
{
 draw_ter();
 int lx = u.posx + u.view_offset_x, ly = u.posy + u.view_offset_y;
 int mx, my;
 InputEvent input;

 const int lookHeight = 13;
 const int lookWidth = getmaxx(w_messages);
 int lookY = TERMY - lookHeight + 1;
 if (getbegy(w_messages) < lookY) lookY = getbegy(w_messages);
 WINDOW* w_look = newwin(lookHeight, lookWidth, lookY, getbegx(w_messages));
 wborder(w_look, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                 LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 mvwprintz(w_look, 1, 1, c_white, _("Looking Around"));
 mvwprintz(w_look, 2, 1, c_white, _("Use directional keys to move the cursor"));
 mvwprintz(w_look, 3, 1, c_white, _("to a nearby square."));
 wrefresh(w_look);
 do {
  werase(w_terrain);
  draw_ter(lx, ly);
  for (int i = 1; i < lookHeight - 1; i++) {
   for (int j = 1; j < lookWidth - 1; j++)
    mvwputch(w_look, i, j, c_white, ' ');
  }

  // Debug helper
  //mvwprintw(w_look, 6, 1, "Items: %d", m.i_at(lx, ly).size() );
  int junk;
  int veh_part = 0;
  int off = 4;
  vehicle *veh = m.veh_at(lx, ly, veh_part);
  if (u_see(lx, ly)) {
   std::string tile = m.tername(lx, ly);
   if (m.has_furn(lx, ly))
    tile += "; " + m.furnname(lx, ly);

   if (m.move_cost(lx, ly) == 0)
    mvwprintw(w_look, 1, 1, _("%s; Impassable"), tile.c_str());
   else
    mvwprintw(w_look, 1, 1, _("%s; Movement cost %d"), tile.c_str(),
                                                    m.move_cost(lx, ly) * 50);
   mvwprintw(w_look, 2, 1, "%s", m.features(lx, ly).c_str());

   field &tmpfield = m.field_at(lx, ly);

   if (tmpfield.fieldCount() > 0) {
		field_entry *cur = NULL;
		for(std::map<field_id, field_entry*>::iterator field_list_it = tmpfield.getFieldStart(); field_list_it != tmpfield.getFieldEnd(); ++field_list_it){
			cur = field_list_it->second;
			if(cur == NULL) continue;
			mvwprintz(w_look, off, 1, fieldlist[cur->getFieldType()].color[cur->getFieldDensity()-1], "%s",
				fieldlist[cur->getFieldType()].name[cur->getFieldDensity()-1].c_str());
			off++; // 4ish
		}
    }
   //if (tmpfield.type != fd_null)
   // mvwprintz(w_look, 4, 1, fieldlist[tmpfield.type].color[tmpfield.density-1],
   //           "%s", fieldlist[tmpfield.type].name[tmpfield.density-1].c_str());

   if (m.tr_at(lx, ly) != tr_null &&
       u.per_cur - u.encumb(bp_eyes) >= traps[m.tr_at(lx, ly)]->visibility)
    mvwprintz(w_look, ++off, 1, traps[m.tr_at(lx, ly)]->color, "%s",
              traps[m.tr_at(lx, ly)]->name.c_str());

   int dex = mon_at(lx, ly);
   if (dex != -1 && u_see(&(z[dex])))
   {
       z[mon_at(lx, ly)].draw(w_terrain, lx, ly, true);
       z[mon_at(lx, ly)].print_info(this, w_look);
       if (!m.has_flag(container, lx, ly))
       {
           if (m.i_at(lx, ly).size() > 1)
           {
               mvwprintw(w_look, 3, 1, _("There are several items there."));
           }
           else if (m.i_at(lx, ly).size() == 1)
           {
               mvwprintw(w_look, 3, 1, _("There is an item there."));
           }
       } else {
           mvwprintw(w_look, 3, 1, _("You cannot see what is inside of it."));
       }
   }
   else if (npc_at(lx, ly) != -1)
   {
       active_npc[npc_at(lx, ly)]->draw(w_terrain, lx, ly, true);
       active_npc[npc_at(lx, ly)]->print_info(w_look);
       if (!m.has_flag(container, lx, ly))
       {
           if (m.i_at(lx, ly).size() > 1)
           {
               mvwprintw(w_look, 3, 1, _("There are several items there."));
           }
           else if (m.i_at(lx, ly).size() == 1)
           {
               mvwprintw(w_look, 3, 1, _("There is an item there."));
           }
       } else {
           mvwprintw(w_look, 3, 1, _("You cannot see what is inside of it."));
       }
   }
   else if (veh)
   {
       mvwprintw(w_look, 3, 1, _("There is a %s there. Parts:"), veh->name.c_str());
       veh->print_part_desc(w_look, ++off, 48, veh_part);
       m.drawsq(w_terrain, u, lx, ly, true, true, lx, ly);
   }
   else if (!m.has_flag(container, lx, ly) && m.i_at(lx, ly).size() > 0)
   {
       mvwprintw(w_look, 3, 1, _("There is a %s there."),
                 m.i_at(lx, ly)[0].tname(this).c_str());
       if (m.i_at(lx, ly).size() > 1)
       {
           mvwprintw(w_look, ++off, 1, _("There are other items there as well."));
       }
       m.drawsq(w_terrain, u, lx, ly, true, true, lx, ly);
   } else if (m.has_flag(container, lx, ly)) {
       mvwprintw(w_look, 3, 1, _("You cannot see what is inside of it."));
       m.drawsq(w_terrain, u, lx, ly, true, false, lx, ly);
   }
   else if (lx == u.posx + u.view_offset_x && ly == u.posy + u.view_offset_y)
   {
       int x,y;
       x = getmaxx(w_terrain)/2 - u.view_offset_x;
       y = getmaxy(w_terrain)/2 - u.view_offset_y;
       mvwputch_inv(w_terrain, y, x, u.color(), '@');

       mvwprintw(w_look, 1, 1, _("You (%s)"), u.name.c_str());
       if (veh) {
           mvwprintw(w_look, 3, 1, _("There is a %s there. Parts:"), veh->name.c_str());
           veh->print_part_desc(w_look, 4, 48, veh_part);
           m.drawsq(w_terrain, u, lx, ly, true, true, lx, ly);
   }

   }
   else
   {
       m.drawsq(w_terrain, u, lx, ly, true, true, lx, ly);
   }
  } else if (u.sight_impaired() &&
              m.light_at(lx, ly) == LL_BRIGHT &&
              rl_dist(u.posx, u.posy, lx, ly) < u.unimpaired_range() &&
              m.sees(u.posx, u.posy, lx, ly, u.unimpaired_range(), junk)) {
   if (u.has_disease("boomered"))
    mvwputch_inv(w_terrain, ly - u.posy + VIEWY, lx - u.posx + VIEWX, c_pink, '#');
   else
    mvwputch_inv(w_terrain, ly - u.posy + VIEWY, lx - u.posx + VIEWX, c_ltgray, '#');
   mvwprintw(w_look, 1, 1, _("Bright light."));
  } else {
   mvwputch(w_terrain, VIEWY, VIEWX, c_white, 'x');
   mvwprintw(w_look, 1, 1, _("Unseen."));
  }
  if (m.graffiti_at(lx, ly).contents)
   mvwprintw(w_look, ++off + 1, 1, _("Graffiti: %s"), m.graffiti_at(lx, ly).contents->c_str());
  wrefresh(w_look);
  wrefresh(w_terrain);

  DebugLog() << __FUNCTION__ << "calling get_input() \n";
  input = get_input();
  if (!u_see(lx, ly))
   mvwputch(w_terrain, ly - u.posy + VIEWY, lx - u.posx + VIEWX, c_black, ' ');
  get_direction(mx, my, input);
  if (mx != -2 && my != -2) {	// Directional key pressed
   lx += mx;
   ly += my;
  }
 } while (input != Close && input != Cancel && input != Confirm);

 werase(w_look);
 delwin(w_look);
 if (input == Confirm)
  return point(lx, ly);
 return point(-1, -1);
}

bool game::list_items_match(std::string sText, std::string sPattern)
{
 size_t iPos;

 do {
  iPos = sPattern.find(",");

  if (sText.find((iPos == std::string::npos) ? sPattern : sPattern.substr(0, iPos)) != std::string::npos)
   return true;

  if (iPos != std::string::npos)
   sPattern = sPattern.substr(iPos+1, sPattern.size());

 } while(iPos != std::string::npos);

 return false;
}

std::vector<map_item_stack> game::find_nearby_items(int iSearchX, int iSearchY)
{
    std::vector<item> here;
    std::map<std::string, map_item_stack> temp_items;
    std::vector<map_item_stack> ret;

    std::vector<point> points = closest_points_first(iSearchX, u.posx, u.posy);

    for (std::vector<point>::iterator p_it = points.begin();
        p_it != points.end(); p_it++)
    {
        if (p_it->y >= u.posy - iSearchY && p_it->y <= u.posy + iSearchY &&
           u_see(p_it->x,p_it->y) &&
           (!m.has_flag(container, p_it->x, p_it->y) ||
           (rl_dist(u.posx, u.posy, p_it->x, p_it->y) == 1 && !m.has_flag(sealed, p_it->x, p_it->y))))
            {
                temp_items.clear();
                here.clear();
                here = m.i_at(p_it->x, p_it->y);
                for (int i = 0; i < here.size(); i++)
                {
                    const std::string name = here[i].tname(this);
                    if (temp_items.find(name) == temp_items.end())
                    {
                        temp_items[name] = map_item_stack(here[i], p_it->x - u.posx, p_it->y - u.posy);
                    }
                    else
                    {
                        temp_items[name].count++;
                    }
                }
                for (std::map<std::string, map_item_stack>::iterator iter = temp_items.begin();
                     iter != temp_items.end();
                     ++iter)
                {
                    ret.push_back(iter->second);
                }

            }
    }
    return ret;
}

std::vector<map_item_stack> game::filter_item_stacks(std::vector<map_item_stack> stack, std::string filter)
{
    std::vector<map_item_stack> ret;

    std::string sFilterPre = "";
    std::string sFilterTemp = filter;
    if (sFilterTemp != "" && filter.substr(0, 1) == "-")
    {
        sFilterPre = "-";
        sFilterTemp = sFilterTemp.substr(1, sFilterTemp.size()-1);
    }

    for (std::vector<map_item_stack>::iterator iter = stack.begin(); iter != stack.end(); ++iter)
    {
        std::string name = iter->example.tname(this);
        if (sFilterTemp == "" || ((sFilterPre != "-" && list_items_match(name, sFilterTemp)) ||
                                  (sFilterPre == "-" && !list_items_match(name, sFilterTemp))))
        {
            ret.push_back(*iter);
        }
    }
    return ret;
}

std::string game::ask_item_filter(WINDOW* window, int rows)
{
    for (int i = 0; i < rows-1; i++)
    {
        mvwprintz(window, i, 1, c_black, "%s", "\
                                                     ");
    }

    mvwprintz(window, 2, 2, c_white, "%s", _("Type part of an item's name to see"));
    mvwprintz(window, 3, 2, c_white, "%s", _("nearby matching items."));
    mvwprintz(window, 5, 2, c_white, "%s", _("Seperate multiple items with ,"));
    mvwprintz(window, 6, 2, c_white, "%s", _("Example: back,flash,aid, ,band"));
    //TODO: fix up the filter code so that "-" applies to each comma-separated bit
    //or, failing that, make the description sound more like what the current behavior does
    mvwprintz(window, 8, 2, c_white, "%s", _("To exclude items, place - in front"));
    mvwprintz(window, 9, 2, c_white, "%s", _("Example: -pipe,chunk,steel"));
    wrefresh(window);
    return string_input_popup(_("Filter:"), 55, sFilter, _("UP: history, CTRL-U clear line, ESC: abort, ENTER: save"), "item_filter", 256);
}


void game::draw_trail_to_square(std::vector<point>& vPoint, int x, int y)
{
    //Remove previous trail, if any
    for (int i = 0; i < vPoint.size(); i++)
    {
        m.drawsq(w_terrain, u, vPoint[i].x, vPoint[i].y, false, true);
    }

    //Draw new trail
    vPoint = line_to(u.posx, u.posy, u.posx + x, u.posy + y, 0);
    for (int i = 1; i < vPoint.size(); i++)
    {
        m.drawsq(w_terrain, u, vPoint[i-1].x, vPoint[i-1].y, true, true);
    }

    mvwputch(w_terrain, vPoint[vPoint.size()-1].y + VIEWY - u.posy - u.view_offset_y,
                        vPoint[vPoint.size()-1].x + VIEWX - u.posx - u.view_offset_x, c_white, 'X');

    wrefresh(w_terrain);
}

//helper method so we can keep list_items shorter
void game::reset_item_list_state(WINDOW* window, int height)
{
    const int width = (OPTIONS["SIDEBAR_STYLE"] == "Narrow") ? 45 : 55;
    for (int i = 1; i < TERMX; i++)
    {
        if (i < width)
        {
            mvwputch(window, 0, i, c_ltgray, LINE_OXOX); // -
            mvwputch(window, TERMY-height-1-VIEW_OFFSET_Y*2, i, c_ltgray, LINE_OXOX); // -
        }

        if (i < TERMY-height-VIEW_OFFSET_Y*2)
        {
            mvwputch(window, i, 0, c_ltgray, LINE_XOXO); // |
            mvwputch(window, i, width - 1, c_ltgray, LINE_XOXO); // |
        }
    }

    mvwputch(window, 0, 0,         c_ltgray, LINE_OXXO); // |^
    mvwputch(window, 0, width - 1, c_ltgray, LINE_OOXX); // ^|

    mvwputch(window, TERMY-height-1-VIEW_OFFSET_Y*2, 0,         c_ltgray, LINE_XXXO); // |-
    mvwputch(window, TERMY-height-1-VIEW_OFFSET_Y*2, width - 1, c_ltgray, LINE_XOXX); // -|


    std::vector<std::string> tokens;
    if (sFilter != "")
    {
        tokens.push_back(_("<R>eset"));
    }

    tokens.push_back(_("<E>xamine"));
    tokens.push_back(_("<C>ompare"));
    tokens.push_back(_("<F>ilter"));
    tokens.push_back(_("<+/->Priority"));

    int gaps = tokens.size()+1;
    int letters = 0;
    int n = tokens.size();
    for (int i = 0; i < n; i++) {
        letters += utf8_width(tokens[i].c_str())-2; //length ignores < >
    }

    int usedwidth = letters;
    const int gap_spaces = (width - usedwidth) / gaps;
    usedwidth += gap_spaces * gaps;
    int xpos = gap_spaces + (width - usedwidth) / 2;
    const int ypos = TERMY - height - 1 - VIEW_OFFSET_Y * 2;

    for (int i = 0; i < n; i++) {
        xpos += shortcut_print(window, ypos, xpos, c_white, c_ltgreen, tokens[i].c_str()) + gap_spaces;
    }

    refresh_all();
}

//returns the first non prority items.
int game::list_filter_high_priority(std::vector<map_item_stack> &stack, std::string prorities)
{
    //TODO:optimize if necessary
    std::vector<map_item_stack> tempstack; // temp
    for(int i = 0 ; i < stack.size() ; i++)
    {
        std::string name = stack[i].example.tname(this);
        if(prorities == "" || !list_items_match(name,prorities))
        {
            tempstack.push_back(stack[i]);
            stack.erase(stack.begin()+i);
            i--;
        }
    }
    int id = stack.size();
    for(int i = 0 ; i < tempstack.size() ; i++)
    {
        stack.push_back(tempstack[i]);
    }
    return id;
}
int game::list_filter_low_priority(std::vector<map_item_stack> &stack, int start,std::string prorities)
{
    //TODO:optimize if necessary
    std::vector<map_item_stack> tempstack; // temp
    for(int i = start ; i < stack.size() ; i++)
    {
        std::string name = stack[i].example.tname(this);
        if(prorities != "" && list_items_match(name,prorities))
        {
            tempstack.push_back(stack[i]);
            stack.erase(stack.begin()+i);
            i--;
        }
    }
    int id = stack.size();
    for(int i = 0 ; i < tempstack.size() ; i++)
    {
        stack.push_back(tempstack[i]);
    }
    return id;
}

void game::list_items()
{
    int iInfoHeight = 12;
    const int width = (OPTIONS["SIDEBAR_STYLE"] == "Narrow") ? 45 : 55;
    WINDOW* w_items = newwin(TERMY-iInfoHeight-VIEW_OFFSET_Y*2, width, VIEW_OFFSET_Y, TERRAIN_WINDOW_WIDTH + VIEW_OFFSET_X);
    WINDOW* w_item_info = newwin(iInfoHeight-1, width - 2, TERMY-iInfoHeight-VIEW_OFFSET_Y, TERRAIN_WINDOW_WIDTH+1+VIEW_OFFSET_X);
    WINDOW* w_item_info_border = newwin(iInfoHeight, width, TERMY-iInfoHeight-VIEW_OFFSET_Y, TERRAIN_WINDOW_WIDTH+VIEW_OFFSET_X);

    //Area to search +- of players position.
    const int iSearchX = 12 + (u.per_cur * 2);
    const int iSearchY = 12 + (u.per_cur * 2);

    //this stores the items found, along with the coordinates
    std::vector<map_item_stack> ground_items = find_nearby_items(iSearchX, iSearchY);
    //this stores only those items that match our filter
    std::vector<map_item_stack> filtered_items = (sFilter != "" ?
                                                  filter_item_stacks(ground_items, sFilter) :
                                                  ground_items);
    int highPEnd = list_filter_high_priority(filtered_items,list_item_upvote);
    int lowPStart = list_filter_low_priority(filtered_items,highPEnd,list_item_downvote);
    const int iItemNum = ground_items.size();

    const int iStoreViewOffsetX = u.view_offset_x;
    const int iStoreViewOffsetY = u.view_offset_y;

    int iActive = 0; // Item index that we're looking at
    const int iMaxRows = TERMY-iInfoHeight-2-VIEW_OFFSET_Y*2;
    int iStartPos = 0;
    int iActiveX = 0;
    int iActiveY = 0;
    int iLastActiveX = -1;
    int iLastActiveY = -1;
    std::vector<point> vPoint;
    InputEvent input = Undefined;
    long ch = 0; //this is a long because getch returns a long
    bool reset = true;
    bool refilter = true;
    int iFilter = 0;
    bool bStopDrawing = false;
    do
    {
        if (ground_items.size() > 0)
        {
            u.view_offset_x = 0;
            u.view_offset_y = 0;

            if (ch == 'I' || ch == 'c' || ch == 'C')
            {
                compare(iActiveX, iActiveY);
                reset = true;
                refresh_all();
            }
            else if (ch == 'f' || ch == 'F')
            {
                sFilter = ask_item_filter(w_item_info, iInfoHeight);
                reset = true;
                refilter = true;
            }
            else if (ch == 'r' || ch == 'R')
            {
                sFilter = "";
                filtered_items = ground_items;
                iLastActiveX = -1;
                iLastActiveY = -1;
                reset = true;
                refilter = true;
            }
            else if ((ch == 'e' || ch == 'E') && filtered_items.size())
            {
                item oThisItem = filtered_items[iActive].example;
                std::vector<iteminfo> vThisItem, vDummy;

                oThisItem.info(true, &vThisItem);
                compare_split_screen_popup(0, width - 5, TERMY-VIEW_OFFSET_Y*2, oThisItem.tname(this), vThisItem, vDummy);

                getch(); // wait until the user presses a key to wipe the screen
                iLastActiveX = -1;
                iLastActiveY = -1;
                reset = true;
            }
            else if(ch == '+')
            {
                std::string temp = string_input_popup(_("High Priority:"), width, list_item_upvote, _("UP: history, CTRL-U clear line, ESC: abort, ENTER: save"), "list_item_priority", 256);
                list_item_upvote = temp;
                refilter = true;
                reset = true;
            }
            else if(ch == '-')
            {
                std::string temp = string_input_popup(_("Low Priority:"), width, list_item_downvote, _("UP: history, CTRL-U clear line, ESC: abort, ENTER: save"), "list_item_downvote", 256);
                list_item_downvote = temp;
                refilter = true;
                reset = true;
            }
            if (refilter)
            {
                filtered_items = filter_item_stacks(ground_items, sFilter);
                highPEnd = list_filter_high_priority(filtered_items,list_item_upvote);
                lowPStart = list_filter_low_priority(filtered_items,highPEnd,list_item_downvote);
                iActive = 0;
                iLastActiveX = -1;
                iLastActiveY = -1;
                refilter = false;
            }
            if (reset)
            {
                reset_item_list_state(w_items, iInfoHeight);
                reset = false;
            }

            bStopDrawing = false;

            // we're switching on input here, whereas above it was if/else clauses on a char
            switch(input)
            {
                case DirectionN:
                    iActive--;
                    if (iActive < 0)
                    {
                        iActive = 0;
                        bStopDrawing = true;
                    }
                    break;
                case DirectionS:
                    iActive++;
                    if (iActive >= iItemNum - iFilter)
                    {
                        iActive = iItemNum - iFilter-1;
                        bStopDrawing = true;
                    }
                    break;
            }

            if (!bStopDrawing)
            {
                if (iItemNum - iFilter > iMaxRows)
                {
                    iStartPos = iActive - (iMaxRows - 1) / 2;

                    if (iStartPos < 0)
                    {
                        iStartPos = 0;
                    }
                    else if (iStartPos + iMaxRows > iItemNum - iFilter)
                    {
                        iStartPos = iItemNum - iFilter - iMaxRows;
                    }
                }

                for (int i = 0; i < iMaxRows; i++)
                {
                    wmove(w_items, i + 1, 1);
                    for (int i = 1; i < width - 1; i++)
                        wprintz(w_items, c_black, " ");
                }

                int iNum = 0;
                iFilter = ground_items.size() - filtered_items.size();
                iActiveX = 0;
                iActiveY = 0;
                std::string sActiveItemName;
                item activeItem;
                std::stringstream sText;
                bool high = true;
                bool low = false;
                int index = 0;
                for (std::vector<map_item_stack>::iterator iter = filtered_items.begin() ;
                     iter != filtered_items.end();
                     ++iter,++index)
                {
                    if(index == highPEnd)
                    {
                        high = false;
                    }
                    if(index == lowPStart)
                    {
                        low = true;
                    }
                    if (iNum >= iStartPos && iNum < iStartPos + ((iMaxRows > iItemNum) ? iItemNum : iMaxRows) )
                    {
                        if (iNum == iActive)
                        {
                            iActiveX = iter->x;
                            iActiveY = iter->y;
                            sActiveItemName = iter->example.tname(this);
                            activeItem = iter->example;
                        }
                        sText.str("");
                        sText << iter->example.tname(this);
                        if (iter->count > 1)
                        {
                            sText << " " << "[" << iter->count << "]";
                        }
                        mvwprintz(w_items, 1 + iNum - iStartPos, 2,
                                  ((iNum == iActive) ? c_ltgreen : (high ? c_yellow : (low ? c_red : c_white))),
                                  "%s", (sText.str()).c_str());
                        int numw = iItemNum > 9 ? 2 : 1;
                        mvwprintz(w_items, 1 + iNum - iStartPos, width - (5 + numw),
                                  ((iNum == iActive) ? c_ltgreen : c_ltgray), "%*d %s",
                                  numw, trig_dist(0, 0, iter->x, iter->y),
                                  direction_name_short(direction_from(0, 0, iter->x, iter->y)).c_str()
                                 );
                     }
                     iNum++;
                }

                mvwprintz(w_items, 0, (width - 9) / 2 + ((iItemNum - iFilter > 9) ? 0 : 1),
                          c_ltgreen, " %*d", ((iItemNum - iFilter > 9) ? 2 : 1), iActive+1);
                wprintz(w_items, c_white, " / %*d ", ((iItemNum - iFilter > 9) ? 2 : 1), iItemNum - iFilter);

                werase(w_item_info);
                fold_and_print(w_item_info,1,1,width - 5, c_white, "%s", activeItem.info().c_str());

                for (int j=0; j < iInfoHeight-1; j++)
                {
                    mvwputch(w_item_info_border, j, 0, c_ltgray, LINE_XOXO);
                }

                for (int j=0; j < iInfoHeight-1; j++)
                {
                    mvwputch(w_item_info_border, j, width - 1, c_ltgray, LINE_XOXO);
                }

                for (int j=0; j < width - 1; j++)
                {
                    mvwputch(w_item_info_border, iInfoHeight-1, j, c_ltgray, LINE_OXOX);
                }

                mvwputch(w_item_info_border, iInfoHeight-1, 0, c_ltgray, LINE_XXOO);
                mvwputch(w_item_info_border, iInfoHeight-1, width - 1, c_ltgray, LINE_XOOX);

                //Only redraw trail/terrain if x/y position changed
                if (iActiveX != iLastActiveX || iActiveY != iLastActiveY)
                {
                    iLastActiveX = iActiveX;
                    iLastActiveY = iActiveY;

                    draw_trail_to_square(vPoint, iActiveX, iActiveY);
                }

                wrefresh(w_items);
                wrefresh(w_item_info_border);
                wrefresh(w_item_info);
            }

            refresh();
            ch = getch();
            input = get_input(ch);
        }
        else
        {
            add_msg(_("You dont see any items around you!"));
            ch = ' ';
            input = Close;
        }
    }
    while (input != Close && input != Cancel);

    u.view_offset_x = iStoreViewOffsetX;
    u.view_offset_y = iStoreViewOffsetY;

    werase(w_items);
    werase(w_item_info);
    werase(w_item_info_border);
    delwin(w_items);
    delwin(w_item_info);
    delwin(w_item_info_border);
    refresh_all(); // TODO - figure out what precisely needs refreshing, rather than the whole screen
}

// Pick up items at (posx, posy).
void game::pickup(int posx, int posy, int min)
{
 //min == -1 is Autopickup

 if (m.has_flag(sealed, posx, posy)) return;

 item_exchanges_since_save += 1; // Keeping this simple.
 write_msg();
 if (u.weapon.type->id == "bio_claws_weapon") {
  if (min != -1)
   add_msg(_("You cannot pick up items with your claws out!"));
  return;
 }
 bool weight_is_okay = (u.weight_carried() <= u.weight_capacity());
 bool volume_is_okay = (u.volume_carried() <= u.volume_capacity() -  2);
 bool from_veh = false;
 int veh_part = 0;
 int k_part = 0;
 vehicle *veh = m.veh_at (posx, posy, veh_part);
 if (min != -1 && veh) {
  k_part = veh->part_with_feature(veh_part, vpf_kitchen);
  veh_part = veh->part_with_feature(veh_part, vpf_cargo, false);
  from_veh = veh && veh_part >= 0 &&
             veh->parts[veh_part].items.size() > 0 &&
             query_yn(_("Get items from %s?"), veh->part_info(veh_part).name);

  if (!from_veh && k_part >= 0) {
    if (veh->fuel_left("water")) {
      if (query_yn(_("Have a drink?"))) {
        veh->drain("water", 1);

        item water(itypes["water_clean"], 0);
        u.eat(this, u.inv.add_item(water).invlet);
        u.moves -= 250;
      }
    } else {
      add_msg(_("The water tank is empty."));
    }
  }
 }
 if ((!from_veh) && m.i_at(posx, posy).size() == 0)
 {
     return;
 }
 // Not many items, just grab them
 if ((from_veh ? veh->parts[veh_part].items.size() : m.i_at(posx, posy).size() ) <= min && min != -1)
 {
  int iter = 0;
  item newit = from_veh ? veh->parts[veh_part].items[0] : m.i_at(posx, posy)[0];
  if (newit.made_of(LIQUID)) {
   add_msg(_("You can't pick up a liquid!"));
   return;
  }
  newit.invlet = u.inv.get_invlet_for_item( newit.typeId() );
  if (newit.invlet == 0) {
   newit.invlet = nextinv;
   advance_nextinv();
  }
  while (iter <= inv_chars.size() && u.has_item(newit.invlet) &&
         !u.i_at(newit.invlet).stacks_with(newit)) {
   newit.invlet = nextinv;
   iter++;
   advance_nextinv();
  }
  if (iter > inv_chars.size()) {
   add_msg(_("You're carrying too many items!"));
   return;
  } else if (!u.can_pickWeight(newit.weight(), false)) {
   add_msg(_("The %s is too heavy!"), newit.tname(this).c_str());
   decrease_nextinv();
  } else if (!u.can_pickVolume(newit.volume())) {
   if (u.is_armed()) {
    if (!u.weapon.has_flag("NO_UNWIELD")) {
     if (newit.is_armor() && // Armor can be instantly worn
         query_yn(_("Put on the %s?"), newit.tname(this).c_str())) {
      if(u.wear_item(this, &newit)){
       if (from_veh)
        veh->remove_item (veh_part, 0);
       else
        m.i_clear(posx, posy);
      }
     } else if (query_yn(_("Drop your %s and pick up %s?"),
                u.weapon.tname(this).c_str(), newit.tname(this).c_str())) {
      if (from_veh)
       veh->remove_item (veh_part, 0);
      else
       m.i_clear(posx, posy);
      m.add_item_or_charges(posx, posy, u.remove_weapon(), 1);
      u.wield(this, u.i_add(newit, this).invlet);
      u.moves -= 100;
      add_msg(_("Wielding %c - %s"), newit.invlet, newit.tname(this).c_str());
     } else
      decrease_nextinv();
    } else {
     add_msg(_("There's no room in your inventory for the %s, and you can't\
 unwield your %s."), newit.tname(this).c_str(), u.weapon.tname(this).c_str());
     decrease_nextinv();
    }
   } else {
    u.wield(this, u.i_add(newit, this).invlet);
    if (from_veh)
     veh->remove_item (veh_part, 0);
    else
     m.i_clear(posx, posy);
    u.moves -= 100;
    add_msg(_("Wielding %c - %s"), newit.invlet, newit.tname(this).c_str());
   }
  } else if (!u.is_armed() &&
             (u.volume_carried() + newit.volume() > u.volume_capacity() - 2 ||
              newit.is_weap() || newit.is_gun())) {
   u.weapon = newit;
   if (from_veh)
    veh->remove_item (veh_part, 0);
   else
    m.i_clear(posx, posy);
   u.moves -= 100;
   add_msg(_("Wielding %c - %s"), newit.invlet, newit.tname(this).c_str());
  } else {
   newit = u.i_add(newit, this);
   if (from_veh)
    veh->remove_item (veh_part, 0);
   else
    m.i_clear(posx, posy);
   u.moves -= 100;
   add_msg("%c - %s", newit.invlet, newit.tname(this).c_str());
  }
  if (weight_is_okay && u.weight_carried() >= u.weight_capacity())
   add_msg(_("You're overburdened!"));
  if (volume_is_okay && u.volume_carried() > u.volume_capacity() - 2) {
   add_msg(_("You struggle to carry such a large volume!"));
  }
  return;
 }

 const int sideStyle = (OPTIONS["SIDEBAR_STYLE"] == "Narrow");

 // Otherwise, we have Autopickup, 2 or more items and should list them, etc.
 int maxmaxitems = TERMY;
 #ifndef MAXLISTHEIGHT
  maxmaxitems = sideStyle ? TERMY : getmaxy(w_messages) - 3;
 #endif

 int itemsH = 12;
 int pickupBorderRows = 3;

 // The pickup list may consume the entire terminal, minus space needed for its
 // header/footer and the item info window.
 int minleftover = itemsH + pickupBorderRows;
 if(maxmaxitems > TERMY - minleftover) maxmaxitems = TERMY - minleftover;

 const int minmaxitems = sideStyle ? 6 : 9;

 std::vector <item> here = from_veh? veh->parts[veh_part].items : m.i_at(posx, posy);
 std::vector<bool> getitem;
 getitem.resize(here.size(), false);

 int maxitems=here.size();
 maxitems=(maxitems < minmaxitems ? minmaxitems : (maxitems > maxmaxitems ? maxmaxitems : maxitems ));

 int pickupH = maxitems + pickupBorderRows;
 int pickupW = getmaxx(w_messages);
 int pickupY = VIEW_OFFSET_Y;
 int pickupX = getbegx(w_messages);

 int itemsW = pickupW;
 int itemsY = sideStyle ? pickupY + pickupH : TERMY - itemsH;
 int itemsX = pickupX;

 WINDOW* w_pickup    = newwin(pickupH, pickupW, pickupY, pickupX);
 WINDOW* w_item_info = newwin(itemsH,  itemsW,  itemsY,  itemsX);

 int ch = ' ';
 int start = 0, cur_it, iter;
 int new_weight = u.weight_carried(), new_volume = u.volume_carried();
 bool update = true;
 mvwprintw(w_pickup, 0,  0, _("PICK UP (, = all)"));
 int selected=0;
 int last_selected=-1;

 if (min == -1) { //Auto Pickup, select matching items
    bool bFoundSomething = false;

    //Loop through Items lowest Volume first
    bool bPickup = false;

    for(int iVol=0, iNumChecked = 0; iNumChecked < here.size(); iVol++) {
        for (int i = 0; i < here.size(); i++) {
            bPickup = false;
            if (here[i].volume() == iVol) {
                iNumChecked++;

                //Auto Pickup all items with 0 Volume and Weight <= AUTO_PICKUP_ZERO * 50
                if (OPTIONS["AUTO_PICKUP_ZERO"]) {
                    if (here[i].volume() == 0 && here[i].weight() <= OPTIONS["AUTO_PICKUP_ZERO"] * 50) {
                        bPickup = true;
                    }
                }

                //Check the Pickup Rules
                if ( mapAutoPickupItems[here[i].tname(this)] ) {
                    bPickup = true;
                }
            }

            if (bPickup) {
                getitem[i] = bPickup;
                bFoundSomething = true;
            }
        }
    }

    if (!bFoundSomething) {
        return;
    }
 } else {
 // Now print the two lists; those on the ground and about to be added to inv
 // Continue until we hit return or space
  do {
   static const std::string pickup_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:;";
   size_t idx=-1;
   for (int i = 1; i < pickupH; i++) {
     mvwprintw(w_pickup, i, 0, "                                                ");
   }
   if ((ch == '<' || ch == KEY_PPAGE) && start > 0) {
    start -= maxitems;
    selected = start;
    mvwprintw(w_pickup, maxitems + 2, 0, "         ");
   } else if ((ch == '>' || ch == KEY_NPAGE) && start + maxitems < here.size()) {
    start += maxitems;
    selected = start;
    mvwprintw(w_pickup, maxitems + 2, pickupH, "            ");
   } else if ( ch == KEY_UP ) {
       selected--;
       if ( selected < 0 ) {
           selected = here.size()-1;
           start = (int)( here.size() / maxitems ) * maxitems;
           if (start >= here.size()-1) start -= maxitems;
       } else if ( selected < start ) {
           start -= maxitems;
       }
   } else if ( ch == KEY_DOWN ) {
       selected++;
       if ( selected >= here.size() ) {
           selected=0;
           start=0;
       } else if ( selected >= start + maxitems ) {
           start+=maxitems;
       }
   } else if ( selected >= 0 && (
                  ( ch == KEY_RIGHT && !getitem[selected]) ||
                  ( ch == KEY_LEFT && getitem[selected] )
             ) ) {
       idx = selected;
   } else if ( ch == '`' ) {
       std::string ext = string_input_popup(_("Enter 2 letters (case sensitive):"), 2);
       if(ext.size() == 2) {
            int p1=pickup_chars.find(ext.at(0));
            int p2=pickup_chars.find(ext.at(1));
            if ( p1 != -1 && p2 != -1 ) {
                 idx=pickup_chars.size() + ( p1 * pickup_chars.size() ) + p2;
            }
       }
   } else {
       idx = pickup_chars.find(ch);
   }

   if ( idx < here.size()) {
    getitem[idx] = ( ch == KEY_RIGHT ? true : ( ch == KEY_LEFT ? false : !getitem[idx] ) );
    if ( ch != KEY_RIGHT && ch != KEY_LEFT) {
       selected = idx;
       start = (int)( idx / maxitems ) * maxitems;
    }

    if (getitem[idx]) {
        new_weight += here[idx].weight();
        new_volume += here[idx].volume();
    } else {
        new_weight -= here[idx].weight();
        new_volume -= here[idx].volume();
    }
    update = true;
   }

   if ( selected != last_selected ) {
       last_selected = selected;
       werase(w_item_info);
       if ( selected >= 0 && selected <= here.size()-1 ) {
           fold_and_print(w_item_info,1,2,48-3, c_ltgray, "%s",  here[selected].info().c_str());
       }
       wborder(w_item_info, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
       mvwprintw(w_item_info, 0, 2, "< %s >", here[selected].tname(this).c_str() );
       wrefresh(w_item_info);
   }

   if (ch == ',') {
    int count = 0;
    for (int i = 0; i < here.size(); i++) {
     if (getitem[i])
      count++;
     else {
      new_weight += here[i].weight();
      new_volume += here[i].volume();
     }
     getitem[i] = true;
    }
    if (count == here.size()) {
     for (int i = 0; i < here.size(); i++)
      getitem[i] = false;
     new_weight = u.weight_carried();
     new_volume = u.volume_carried();
    }
    update = true;
   }
   for (cur_it = start; cur_it < start + maxitems; cur_it++) {
    mvwprintw(w_pickup, 1 + (cur_it % maxitems), 0,
              "                                        ");
    if (cur_it < here.size()) {
     nc_color icolor=here[cur_it].color(&u);
     if(cur_it == selected) {
         icolor=hilite(icolor);
     }

     if (cur_it < pickup_chars.size() ) {
        mvwputch(w_pickup, 1 + (cur_it % maxitems), 0, icolor, char(pickup_chars[cur_it]));
     } else {
        int p=cur_it - pickup_chars.size();
        int p1=p / pickup_chars.size();
        int p2=p % pickup_chars.size();
        mvwprintz(w_pickup, 1 + (cur_it % maxitems), 0, icolor, "`%c%c",char(pickup_chars[p1]),char(pickup_chars[p2]));
     }
     if (getitem[cur_it])
      wprintz(w_pickup, c_ltblue, " + ");
     else
      wprintw(w_pickup, " - ");
     wprintz(w_pickup, icolor, here[cur_it].tname(this).c_str());
     if (here[cur_it].charges > 0)
      wprintz(w_pickup, icolor, " (%d)", here[cur_it].charges);
    }
   }

   int pw = pickupW;
   const char *unmark = _("[left] Unmark");
   const char *scroll = _("[up/dn] Scroll");
   const char *mark   = _("[right] Mark");
   mvwprintw(w_pickup, maxitems + 1, 0,                         unmark);
   mvwprintw(w_pickup, maxitems + 1, (pw - strlen(scroll)) / 2, scroll);
   mvwprintw(w_pickup, maxitems + 1,  pw - strlen(mark),        mark);
   const char *prev = _("[pgup] Prev");
   const char *all = _("[,] All");
   const char *next   = _("[pgdn] Next");
   if (start > 0)
    mvwprintw(w_pickup, maxitems + 2, 0, prev);
   mvwprintw(w_pickup, maxitems + 2, (pw - strlen(all)) / 2, all);
   if (cur_it < here.size())
    mvwprintw(w_pickup, maxitems + 2, pw - strlen(next), next);

   if (update) {		// Update weight & volume information
    update = false;
    mvwprintw(w_pickup, 0,  7, "                           ");
    mvwprintz(w_pickup, 0,  9,
              (new_weight >= u.weight_capacity() ? c_red : c_white),
              _("Wgt %.1f"), u.convert_weight(new_weight));
    wprintz(w_pickup, c_white, "/%.1f", u.convert_weight(u.weight_capacity()));
    mvwprintz(w_pickup, 0, 24,
              (new_volume > u.volume_capacity() - 2 ? c_red : c_white),
              _("Vol %d"), new_volume);
    wprintz(w_pickup, c_white, "/%d", u.volume_capacity() - 2);
   }
   wrefresh(w_pickup);
 /* No longer using
   ch = input();
    because it pulls a: case KEY_DOWN:  return 'j';
 */
   ch = (int)getch();

  } while (ch != ' ' && ch != '\n' && ch != KEY_ESCAPE);
  if (ch != '\n') {
   werase(w_pickup);
   wrefresh(w_pickup);
   werase(w_item_info);
   wrefresh(w_item_info);
   delwin(w_pickup);
   delwin(w_item_info);
   add_msg(_("Never mind."));
   refresh_all();
   return;
  }
 }

 // At this point we've selected our items, now we add them to our inventory
 int curmit = 0;
 bool got_water = false;	// Did we try to pick up water?
 bool offered_swap = false;
 std::map<std::string, int> mapPickup;
 for (int i = 0; i < here.size(); i++) {
  iter = 0;
  // This while loop guarantees the inventory letter won't be a repeat. If it
  // tries all 52 letters, it fails and we don't pick it up.
  if (getitem[i] && here[i].made_of(LIQUID))
   got_water = true;
  else if (getitem[i]) {
   iter = 0;
   while (iter < inv_chars.size() && (here[i].invlet == 0 ||
                        (u.has_item(here[i].invlet) &&
                         !u.i_at(here[i].invlet).stacks_with(here[i]))) ) {
    here[i].invlet = nextinv;
    iter++;
    advance_nextinv();
   }
   if (iter == inv_chars.size()) {
    add_msg(_("You're carrying too many items!"));
    werase(w_pickup);
    wrefresh(w_pickup);
    delwin(w_pickup);
    return;
   } else if (!u.can_pickWeight(here[i].weight(), false)) {
    add_msg(_("The %s is too heavy!"), here[i].tname(this).c_str());
    decrease_nextinv();
   } else if (!u.can_pickVolume(here[i].volume())) {
    if (u.is_armed()) {
     if (!u.weapon.has_flag("NO_UNWIELD")) {
      if (here[i].is_armor() && // Armor can be instantly worn
          query_yn(_("Put on the %s?"), here[i].tname(this).c_str())) {
       if(u.wear_item(this, &(here[i])))
       {
        if (from_veh)
         veh->remove_item (veh_part, curmit);
        else
         m.i_rem(posx, posy, curmit);
        curmit--;
       }
      } else if (!offered_swap) {
       if (query_yn(_("Drop your %s and pick up %s?"),
                u.weapon.tname(this).c_str(), here[i].tname(this).c_str())) {
        if (from_veh)
         veh->remove_item (veh_part, curmit);
        else
         m.i_rem(posx, posy, curmit);
        m.add_item_or_charges(posx, posy, u.remove_weapon(), 1);
        u.wield(this, u.i_add(here[i], this).invlet);
        mapPickup[here[i].tname(this)]++;
        curmit--;
        u.moves -= 100;
        add_msg(_("Wielding %c - %s"), u.weapon.invlet, u.weapon.tname(this).c_str());
       }
       offered_swap = true;
      } else
       decrease_nextinv();
     } else {
      add_msg(_("There's no room in your inventory for the %s, and you can't\
  unwield your %s."), here[i].tname(this).c_str(), u.weapon.tname(this).c_str());
      decrease_nextinv();
     }
    } else {
     u.wield(this, u.i_add(here[i], this).invlet);
     mapPickup[here[i].tname(this)]++;
     if (from_veh)
      veh->remove_item (veh_part, curmit);
     else
      m.i_rem(posx, posy, curmit);
     curmit--;
     u.moves -= 100;
    }
   } else if (!u.is_armed() &&
            (u.volume_carried() + here[i].volume() > u.volume_capacity() - 2 ||
              here[i].is_weap() || here[i].is_gun())) {
    u.weapon = here[i];
    if (from_veh)
     veh->remove_item (veh_part, curmit);
    else
     m.i_rem(posx, posy, curmit);
    u.moves -= 100;
    curmit--;
   } else {
    u.i_add(here[i], this);
    mapPickup[here[i].tname(this)]++;
    if (from_veh)
     veh->remove_item (veh_part, curmit);
    else
     m.i_rem(posx, posy, curmit);
    u.moves -= 100;
    curmit--;
   }
  }
  curmit++;
 }

 if (min == -1) { //Auto pickup item message
     if (mapPickup.size() > 0) {
        std::stringstream sTemp;

        for (std::map<std::string, int>::iterator iter = mapPickup.begin(); iter != mapPickup.end(); ++iter) {
            if (sTemp.str() != "") {
                sTemp << ", ";
            }

            sTemp << iter->second << " " << iter->first;
        }

        add_msg((_("You pick up: ") + sTemp.str()).c_str());
     }
 }

 if (got_water)
  add_msg(_("You can't pick up a liquid!"));
 if (weight_is_okay && u.weight_carried() >= u.weight_capacity())
  add_msg(_("You're overburdened!"));
 if (volume_is_okay && u.volume_carried() > u.volume_capacity() - 2) {
  add_msg(_("You struggle to carry such a large volume!"));
 }
 werase(w_pickup);
 wrefresh(w_pickup);
 werase(w_item_info);
 wrefresh(w_item_info);
 delwin(w_pickup);
 delwin(w_item_info);
}

// Handle_liquid returns false if we didn't handle all the liquid.
bool game::handle_liquid(item &liquid, bool from_ground, bool infinite, item *source)
{
 if (!liquid.made_of(LIQUID)) {
  dbg(D_ERROR) << "game:handle_liquid: Tried to handle_liquid a non-liquid!";
  debugmsg("Tried to handle_liquid a non-liquid!");
  return false;
 }
 if (liquid.type->id == "gasoline" && vehicle_near() && query_yn(_("Refill vehicle?"))) {
  int vx = u.posx, vy = u.posy;
  refresh_all();
  if (choose_adjacent(_("Refill vehicle"), vx, vy)) {
   vehicle *veh = m.veh_at (vx, vy);
   if (veh) {
    ammotype ftype = "gasoline";
    int fuel_cap = veh->fuel_capacity(ftype);
    int fuel_amnt = veh->fuel_left(ftype);
    if (fuel_cap < 1)
     add_msg (_("This vehicle doesn't use %s."), ammo_name(ftype).c_str());
    else if (fuel_amnt == fuel_cap)
     add_msg (_("Already full."));
    else if (from_ground && query_yn(_("Pump until full?"))) {
     u.assign_activity(this, ACT_REFILL_VEHICLE, 2 * (fuel_cap - fuel_amnt));
     u.activity.placement = point(vx, vy);
    } else { // Not pump
     veh->refill ("gasoline", liquid.charges);
     if (veh->fuel_left(ftype) < fuel_cap) {
        add_msg(_("You refill %s with %s."),
                veh->name.c_str(), ammo_name(ftype).c_str());
     } else {
        add_msg(_("You refill %s with %s to its maximum."),
                veh->name.c_str(), ammo_name(ftype).c_str());
     }

     u.moves -= 100;
     return true;
    }
   } else // if (veh)
    add_msg (_("There isn't any vehicle there."));
   return false;
  } // if (choose_adjacent("Refill vehicle", vx, vy))

 } else { // Not filling vehicle

   // Ask to pour rotten liquid (milk!) from the get-go
  if (!from_ground && liquid.rotten(this) &&
      query_yn(_("Pour %s on the ground?"), liquid.tname(this).c_str())) {
   m.add_item_or_charges(u.posx, u.posy, liquid, 1);
   return true;
  }

  std::stringstream text;
  text << _("Container for ") << liquid.tname(this);
  char ch = inv_type(text.str().c_str(), IC_CONTAINER);
  if (!u.has_item(ch)) {
    // No container selected (escaped, ...), ask to pour
    // we asked to pour rotten already
   if (!from_ground && !liquid.rotten(this) &&
       query_yn(_("Pour %s on the ground?"), liquid.tname(this).c_str())) {
    m.add_item_or_charges(u.posx, u.posy, liquid, 1);
    return true;
   }
   return false;
  }

  item *cont = &(u.i_at(ch));
  if (cont == NULL || cont->is_null()) {
    // Container is null, ask to pour.
    // we asked to pour rotten already
   if (!from_ground && !liquid.rotten(this) &&
       query_yn(_("Pour %s on the ground?"), liquid.tname(this).c_str())) {
    m.add_item_or_charges(u.posx, u.posy, liquid, 1);
    return true;
   }
   add_msg(_("Never mind."));
   return false;

  } else if(cont == source) {

    //Source and destination are the same; abort
    add_msg(_("That's the same container!"));
    return false;

  } else if (liquid.is_ammo() && (cont->is_tool() || cont->is_gun())) {
// for filling up chainsaws, jackhammers and flamethrowers
   ammotype ammo = "NULL";
   int max = 0;

   if (cont->is_tool()) {
    it_tool* tool = dynamic_cast<it_tool*>(cont->type);
    ammo = tool->ammo;
    max = tool->max_charges;
   } else {
    it_gun* gun = dynamic_cast<it_gun*>(cont->type);
    ammo = gun->ammo;
    max = gun->clip;
   }

   ammotype liquid_type = liquid.ammo_type();

   if (ammo != liquid_type) {
    add_msg(_("Your %s won't hold %s."), cont->tname(this).c_str(),
                                      liquid.tname(this).c_str());
    return false;
   }

   if (max <= 0 || cont->charges >= max) {
    add_msg(_("Your %s can't hold any more %s."), cont->tname(this).c_str(),
                                               liquid.tname(this).c_str());
    return false;
   }

   if (cont->charges > 0 && cont->curammo->id != liquid.type->id) {
    add_msg(_("You can't mix loads in your %s."), cont->tname(this).c_str());
    return false;
   }

   add_msg(_("You pour %s into your %s."), liquid.tname(this).c_str(),
                                        cont->tname(this).c_str());
   cont->curammo = dynamic_cast<it_ammo*>(liquid.type);
   if (infinite)
    cont->charges = max;
   else {
    cont->charges += liquid.charges;
    if (cont->charges > max) {
     int extra = cont->charges - max;
     cont->charges = max;
     liquid.charges = extra;
     add_msg(_("There's some left over!"));
     return false;
    }
   }
   return true;

  } else if (!cont->is_container()) {
   add_msg(_("That %s won't hold %s."), cont->tname(this).c_str(),
                                     liquid.tname(this).c_str());
   return false;
  } else        // filling up normal containers
    {
      // first, check if liquid types are compatible
      if (!cont->contents.empty())
      {
        if  (cont->contents[0].type->id != liquid.type->id)
        {
          add_msg(_("You can't mix loads in your %s."), cont->tname(this).c_str());
          return false;
        }
      }

      // ok, liquids are compatible.  Now check what the type of liquid is
      // this will determine how much the holding container can hold

      it_container* container = dynamic_cast<it_container*>(cont->type);
      int holding_container_charges;

      if (liquid.type->is_food())
      {
        it_comest* tmp_comest = dynamic_cast<it_comest*>(liquid.type);
        holding_container_charges = container->contains * tmp_comest->charges;
      }
      else if (liquid.type->is_ammo())
      {
        it_ammo* tmp_ammo = dynamic_cast<it_ammo*>(liquid.type);
        holding_container_charges = container->contains * tmp_ammo->count;
      }
      else
        holding_container_charges = container->contains;

      // if the holding container is NOT empty
      if (!cont->contents.empty())
      {
        // case 1: container is completely full
        if (cont->contents[0].charges == holding_container_charges)
        {
          add_msg(_("Your %s can't hold any more %s."), cont->tname(this).c_str(),
                                                   liquid.tname(this).c_str());
          return false;
        }

        // case 2: container is half full

        if (infinite)
        {
          cont->contents[0].charges = holding_container_charges;
          add_msg(_("You pour %s into your %s."), liquid.tname(this).c_str(),
                                        cont->tname(this).c_str());
          return true;
        }
        else // Container is finite, not empty and not full, add liquid to it
        {
          add_msg(_("You pour %s into your %s."), liquid.tname(this).c_str(),
                    cont->tname(this).c_str());
          cont->contents[0].charges += liquid.charges;
          if (cont->contents[0].charges > holding_container_charges)
          {
            int extra = cont->contents[0].charges - holding_container_charges;
            cont->contents[0].charges = holding_container_charges;
            liquid.charges = extra;
            add_msg(_("There's some left over!"));
            // Why not try to find another container here?
            return false;
          }
          return true;
        }
      }
      else  // pouring into an empty container
      {
        if (!cont->has_flag("WATERTIGHT"))  // invalid container types
        {
          add_msg(_("That %s isn't water-tight."), cont->tname(this).c_str());
          return false;
        }
        else if (!(cont->has_flag("SEALS")))
        {
          add_msg(_("You can't seal that %s!"), cont->tname(this).c_str());
          return false;
        }
        // pouring into a valid empty container
        int default_charges = 1;

        if (liquid.is_food())
        {
          it_comest* comest = dynamic_cast<it_comest*>(liquid.type);
          default_charges = comest->charges;
        }
        else if (liquid.is_ammo())
        {
          it_ammo* ammo = dynamic_cast<it_ammo*>(liquid.type);
          default_charges = ammo->count;
        }

        if (infinite) // if filling from infinite source, top it to max
          liquid.charges = container->contains * default_charges;
        else if (liquid.charges > container->contains * default_charges)
        {
          add_msg(_("You fill your %s with some of the %s."), cont->tname(this).c_str(),
                                                    liquid.tname(this).c_str());
          u.inv.unsort();
          int oldcharges = liquid.charges - container->contains * default_charges;
          liquid.charges = container->contains * default_charges;
          cont->put_in(liquid);
          liquid.charges = oldcharges;
          return false;
        }
        cont->put_in(liquid);
        return true;
      }
    }
    return false;
  }
 return true;
}

void game::drop(char chInput)
{
    std::vector<item> dropped;

    if (chInput == '.') {
        dropped = multidrop();
    } else {
        if (u.inv.item_by_letter(chInput).is_null()) {
            dropped.push_back(u.i_rem(chInput));
        } else {
            dropped.push_back(u.inv.remove_item_by_letter(chInput));
        }
    }

    if (dropped.size() == 0) {
        add_msg(_("Never mind."));
        return;
    }

    item_exchanges_since_save += dropped.size();

    itype_id first = itype_id(dropped[0].type->id);
    bool same = true;
    for (int i = 1; i < dropped.size() && same; i++) {
        if (dropped[i].type->id != first) {
            same = false;
        }
    }

    int veh_part = 0;
    bool to_veh = false;
    vehicle *veh = m.veh_at(u.posx, u.posy, veh_part);
    if (veh) {
        veh_part = veh->part_with_feature (veh_part, vpf_cargo);
        to_veh = veh_part >= 0;
    }
    if (dropped.size() == 1 || same) {
        if (to_veh) {
            add_msg(ngettext("You put your %1$s in the %2$s's %3$s.",
                             "You put your %1$ss in the %2$s's %3$s.",
                             dropped.size()),
                    dropped[0].tname(this).c_str(),
                    veh->name.c_str(),
                    veh->part_info(veh_part).name);
        } else {
            add_msg(ngettext("You drop your %s.", "You drop your %ss.",
                             dropped.size()),
                    dropped[0].tname(this).c_str());
        }
    } else {
        if (to_veh) {
            add_msg(_("You put several items in the %s's %s."),
                    veh->name.c_str(), veh->part_info(veh_part).name);
        } else {
            add_msg(_("You drop several items."));
        }
    }

    if (to_veh) {
        bool vh_overflow = false;
        for (int i = 0; i < dropped.size(); i++) {
            vh_overflow = vh_overflow || !veh->add_item (veh_part, dropped[i]);
            if (vh_overflow) {
                m.add_item_or_charges(u.posx, u.posy, dropped[i], 1);
            }
        }
        if (vh_overflow) {
            add_msg (_("The trunk is full, so some items fall on the ground."));
        }
    } else {
        for (int i = 0; i < dropped.size(); i++)
            m.add_item_or_charges(u.posx, u.posy, dropped[i], 2);
    }
}

void game::drop_in_direction()
{
    int dirx, diry;
    if (!choose_adjacent(_("Drop"), dirx, diry)) {
        return;
    }

    int veh_part = 0;
    bool to_veh = false;
    vehicle *veh = m.veh_at(dirx, diry, veh_part);
    if (veh) {
        veh_part = veh->part_with_feature (veh_part, vpf_cargo);
        to_veh = veh->type != veh_null && veh_part >= 0;
    }

    if (m.has_flag(noitem, dirx, diry) || m.has_flag(sealed, dirx, diry)) {
        add_msg(_("You can't place items there!"));
        return;
    }

    bool can_move_there = m.move_cost(dirx, diry) != 0;

    std::vector<item> dropped = multidrop();

    if (dropped.size() == 0) {
        add_msg(_("Never mind."));
        return;
    }

    item_exchanges_since_save += dropped.size();

    itype_id first = itype_id(dropped[0].type->id);
    bool same = true;
    for (int i = 1; i < dropped.size() && same; i++) {
        if (dropped[i].type->id != first) {
            same = false;
        }
    }

    if (dropped.size() == 1 || same) {
        if (to_veh) {
            add_msg(ngettext("You put your %1$s in the %2$s's %3$s.",
                             "You put your %1$ss in the %2$s's %3$s.",
                             dropped.size()),
                    dropped[0].tname(this).c_str(),
                    veh->name.c_str(),
                    veh->part_info(veh_part).name);
        } else if (can_move_there) {
            add_msg(ngettext("You drop your %s on the %s.",
                             "You drop your %ss on the %s.", dropped.size()),
                    dropped[0].tname(this).c_str(),
                    m.name(dirx, diry).c_str());
        } else {
            add_msg(ngettext("You put your %s in the %s.",
                             "You put your %ss in the %s.", dropped.size()),
                    dropped[0].tname(this).c_str(),
                    m.name(dirx, diry).c_str());
        }
    } else {
        if (to_veh) {
            add_msg(_("You put several items in the %s's %s."),
                    veh->name.c_str(), veh->part_info(veh_part).name);
        } else if (can_move_there) {
            add_msg(_("You drop several items on the %s."),
                    m.name(dirx, diry).c_str());
        } else {
            add_msg(_("You put several items in the %s."),
                    m.name(dirx, diry).c_str());
        }
    }

    if (to_veh) {
        bool vh_overflow = false;
        for (int i = 0; i < dropped.size(); i++) {
            vh_overflow = vh_overflow || !veh->add_item (veh_part, dropped[i]);
            if (vh_overflow) {
                m.add_item_or_charges(dirx, diry, dropped[i], 1);
            }
        }
        if (vh_overflow) {
            add_msg (_("The trunk is full, so some items fall on the ground."));
        }
    } else {
        for (int i = 0; i < dropped.size(); i++) {
            m.add_item_or_charges(dirx, diry, dropped[i], 1);
        }
    }
}

void game::reassign_item(char ch)
{
 if (ch == '.') {
     ch = inv(_("Reassign item:"));
 }
 if (ch == ' ') {
  add_msg(_("Never mind."));
  return;
 }
 if (!u.has_item(ch)) {
  add_msg(_("You do not have that item."));
  return;
 }
 char newch = popup_getkey(_("%c - %s; enter new letter."), ch,
                           u.i_at(ch).tname().c_str());
 if (inv_chars.find(newch) == std::string::npos) {
  add_msg(_("%c is not a valid inventory letter."), newch);
  return;
 }
 item* change_from = &(u.i_at(ch));
 if (u.has_item(newch)) {
  item* change_to = &(u.i_at(newch));
  change_to->invlet = ch;
  add_msg("%c - %s", ch, change_to->tname().c_str());
 }
 change_from->invlet = newch;
 add_msg("%c - %s", newch, change_from->tname().c_str());
}

void game::plthrow(char chInput)
{
 char ch;

 if (chInput != '.') {
  ch = chInput;
 } else {
  ch = inv(_("Throw item:"));
  refresh_all();
 }

 int range = u.throw_range(u.lookup_item(ch));
 if (range < 0) {
  add_msg(_("You don't have that item."));
  return;
 } else if (range == 0) {
  add_msg(_("That is too heavy to throw."));
  return;
 }
 item thrown = u.i_at(ch);
  if( std::find(unreal_itype_ids.begin(), unreal_itype_ids.end(),
    thrown.type->id) != unreal_itype_ids.end()) {
  add_msg(_("That's part of your body, you can't throw that!"));
  return;
 }

 m.draw(this, w_terrain, point(u.posx, u.posy));

 std::vector <monster> mon_targets;
 std::vector <int> targetindices;
 int passtarget = -1;
 for (int i = 0; i < z.size(); i++) {
   if (u_see(&(z[i]))) {
     z[i].draw(w_terrain, u.posx, u.posy, true);
     if(rl_dist( u.posx, u.posy, z[i].posx, z[i].posy ) <= range) {
       mon_targets.push_back(z[i]);
       targetindices.push_back(i);
       if (i == last_target) {
	 passtarget = mon_targets.size() - 1;
       }
     }
   }
 }

 int x = u.posx;
 int y = u.posy;

 // target() sets x and y, or returns false if we canceled (by pressing Esc)
 std::vector <point> trajectory = target(x, y, u.posx - range, u.posy - range,
					 u.posx + range, u.posy + range,
					 mon_targets, passtarget, &thrown);
 if (trajectory.size() == 0)
  return;
 if (passtarget != -1)
  last_target = targetindices[passtarget];

 // Throw a single charge of a stacking object.
 if( thrown.count_by_charges() && thrown.charges > 1 ) {
     u.i_at(ch).charges--;
     thrown.charges = 1;
 } else {
     u.i_rem(ch);
 }

 u.moves -= 125;
 u.practice(turn, "throw", 10);

 throw_item(u, x, y, thrown, trajectory);
}

void game::plfire(bool burst)
{
 char reload_invlet = 0;
 if (!u.weapon.is_gun())
  return;
 vehicle *veh = m.veh_at(u.posx, u.posy);
 if (veh && veh->player_in_control(&u) && u.weapon.is_two_handed(&u)) {
  add_msg (_("You need a free arm to drive!"));
  return;
 }
 if (u.weapon.has_flag("CHARGE") && !u.weapon.active) {
  if (u.has_charges("UPS_on", 1) || u.has_charges("UPS_off", 1) ||
      u.has_charges("adv_UPS_on", 1) || u.has_charges("adv_UPS_off", 1)) {
   add_msg(_("Your %s starts charging."), u.weapon.tname().c_str());
   u.weapon.charges = 0;
   u.weapon.curammo = dynamic_cast<it_ammo*>(itypes["charge_shot"]);
   u.weapon.active = true;
   return;
  } else {
   add_msg(_("You need a charged UPS."));
   return;
  }
 }

 if ((u.weapon.has_flag("STR8_DRAW")  && u.str_cur <  4) ||
     (u.weapon.has_flag("STR10_DRAW") && u.str_cur <  5) ||
     (u.weapon.has_flag("STR12_DRAW") && u.str_cur <  6)   ) {
  add_msg(_("You're not strong enough to draw the bow!"));
  return;
 }

 if (u.weapon.has_flag("RELOAD_AND_SHOOT") && u.weapon.charges == 0) {
  reload_invlet = u.weapon.pick_reload_ammo(u, true);
  if (reload_invlet == 0) {
   add_msg(_("Out of ammo!"));
   return;
  }

  u.weapon.reload(u, reload_invlet);
  u.moves -= u.weapon.reload_time(u);
  refresh_all();
 }

 if (u.weapon.num_charges() == 0 && !u.weapon.has_flag("RELOAD_AND_SHOOT")) {
  add_msg(_("You need to reload!"));
  return;
 }
 if (u.weapon.has_flag("FIRE_100") && u.weapon.num_charges() < 100) {
  add_msg(_("Your %s needs 100 charges to fire!"), u.weapon.tname().c_str());
  return;
 }
 if (u.weapon.has_flag("USE_UPS") && !u.has_charges("UPS_off", 5) &&
     !u.has_charges("UPS_on", 5) && !u.has_charges("adv_UPS_off", 53) &&
     !u.has_charges("adv_UPS_on", 3)) {
  add_msg(_("You need a UPS with at least 5 charges or an advanced UPS with at least 3 charged to fire that!"));
  return;
 }

 int range = u.weapon.range(&u);

 m.draw(this, w_terrain, point(u.posx, u.posy));

// Populate a list of targets with the zombies in range and visible
 std::vector <monster> mon_targets;
 std::vector <int> targetindices;
 int passtarget = -1;
 for (int i = 0; i < z.size(); i++) {
   if (u_see(&(z[i]))) {
     z[i].draw(w_terrain, u.posx, u.posy, true);
     if(rl_dist( u.posx, u.posy, z[i].posx, z[i].posy ) <= range) {
       mon_targets.push_back(z[i]);
       targetindices.push_back(i);
       if (i == last_target) {
	 passtarget = mon_targets.size() - 1;
       }
     }
   }
 }

 int x = u.posx;
 int y = u.posy;

 // target() sets x and y, and returns an empty vector if we canceled (Esc)
 std::vector <point> trajectory = target(x, y, u.posx - range, u.posy - range,
					 u.posx + range, u.posy + range,
					 mon_targets, passtarget, &u.weapon);

 draw_ter(); // Recenter our view
 if (trajectory.size() == 0) {
  if(u.weapon.has_flag("RELOAD_AND_SHOOT"))
  {
      u.moves += u.weapon.reload_time(u);
      unload(u.weapon);
      u.moves += u.weapon.reload_time(u) / 2; // unloading time
  }
  return;
 }
 if (passtarget != -1) { // We picked a real live target
  last_target = targetindices[passtarget]; // Make it our default for next time
  z[targetindices[passtarget]].add_effect(ME_HIT_BY_PLAYER, 100);
 }

 if (u.weapon.has_flag("USE_UPS")) {
  if (u.has_charges("adv_UPS_off", 3))
   u.use_charges("adv_UPS_off", 3);
  else if (u.has_charges("adv_UPS_on", 3))
   u.use_charges("adv_UPS_on", 3);
  else if (u.has_charges("UPS_off", 5))
   u.use_charges("UPS_off", 5);
  else if (u.has_charges("UPS_on", 5))
   u.use_charges("UPS_on", 5);
 }
 if (u.weapon.mode == "MODE_BURST")
  burst = true;

// Train up our skill
 it_gun* firing = dynamic_cast<it_gun*>(u.weapon.type);
 int num_shots = 1;
 if (burst)
  num_shots = u.weapon.burst_size();
 if (num_shots > u.weapon.num_charges())
   num_shots = u.weapon.num_charges();
 if (u.skillLevel(firing->skill_used) == 0 ||
     (firing->ammo != "BB" && firing->ammo != "nail"))
     u.practice(turn, firing->skill_used, 4 + (num_shots / 2));
 if (u.skillLevel("gun") == 0 ||
     (firing->ammo != "BB" && firing->ammo != "nail"))
     u.practice(turn, "gun", 5);

 fire(u, x, y, trajectory, burst);
}

void game::butcher()
{
 if (u.in_vehicle)
 {
     add_msg(_("You can't butcher while driving!"));
     return;
 }
 std::vector<int> corpses;
 for (int i = 0; i < m.i_at(u.posx, u.posy).size(); i++) {
  if (m.i_at(u.posx, u.posy)[i].type->id == "corpse")
   corpses.push_back(i);
 }
 if (corpses.size() == 0) {
  add_msg(_("There are no corpses here to butcher."));
  return;
 }
 int factor = u.butcher_factor();
 if (factor == 999) {
  add_msg(_("You don't have a sharp item to butcher with."));
  return;
 }
// We do it backwards to prevent the deletion of a corpse from corrupting our
// vector of indices.
 for (int i = corpses.size() - 1; i >= 0; i--) {
  mtype *corpse = m.i_at(u.posx, u.posy)[corpses[i]].corpse;
  if (query_yn(_("Butcher the %s corpse?"), corpse->name.c_str())) {
   int time_to_cut = 0;
   switch (corpse->size) {	// Time in turns to cut up te corpse
    case MS_TINY:   time_to_cut =  2; break;
    case MS_SMALL:  time_to_cut =  5; break;
    case MS_MEDIUM: time_to_cut = 10; break;
    case MS_LARGE:  time_to_cut = 18; break;
    case MS_HUGE:   time_to_cut = 40; break;
   }
   time_to_cut *= 100;	// Convert to movement points
   time_to_cut += factor * 5;	// Penalty for poor tool
   if (time_to_cut < 250)
    time_to_cut = 250;
   u.assign_activity(this, ACT_BUTCHER, time_to_cut, corpses[i]);
   u.moves = 0;
   return;
  }
 }
}

void game::complete_butcher(int index)
{
 // corpses can disappear (rezzing!), so check for that
 if (m.i_at(u.posx, u.posy).size() <= index || m.i_at(u.posx, u.posy)[index].corpse == NULL || m.i_at(u.posx, u.posy)[index].typeId() != "corpse" ) {
  add_msg(_("There's no corpse to butcher!"));
  return;
 }
 mtype* corpse = m.i_at(u.posx, u.posy)[index].corpse;
 std::vector<item> contents = m.i_at(u.posx, u.posy)[index].contents;
 int age = m.i_at(u.posx, u.posy)[index].bday;
 m.i_rem(u.posx, u.posy, index);
 int factor = u.butcher_factor();
 int pieces = 0, pelts = 0, bones = 0, sinews = 0, feathers = 0;
 double skill_shift = 0.;

 int sSkillLevel = u.skillLevel("survival");

 switch (corpse->size) {
  case MS_TINY:   pieces =  1; pelts =  1; bones = 1; sinews = 1; feathers = 2;  break;
  case MS_SMALL:  pieces =  2; pelts =  3; bones = 4; sinews = 4; feathers = 6;  break;
  case MS_MEDIUM: pieces =  4; pelts =  6; bones = 9; sinews = 9; feathers = 11; break;
  case MS_LARGE:  pieces =  8; pelts = 10; bones = 14;sinews = 14; feathers = 17;break;
  case MS_HUGE:   pieces = 16; pelts = 18; bones = 21;sinews = 21; feathers = 24;break;
 }

 skill_shift += rng(0, sSkillLevel - 3);
 skill_shift += rng(0, u.dex_cur - 8) / 4;
 if (u.str_cur < 4)
  skill_shift -= rng(0, 5 * (4 - u.str_cur)) / 4;
 if (factor > 0)
  skill_shift -= rng(0, factor / 5);

 int practice = 4 + pieces;
 if (practice > 20)
  practice = 20;
 u.practice(turn, "survival", practice);

 pieces += int(skill_shift);
 if (skill_shift < 5)  {	// Lose some pelts and bones
  pelts += (skill_shift - 5);
  bones += (skill_shift - 2);
  sinews += (skill_shift - 8);
  feathers += (skill_shift - 1);
 }

 if (bones > 0) {
  if (corpse->has_flag(MF_BONES)) {
    m.spawn_item(u.posx, u.posy, "bone", age, bones);
   add_msg(_("You harvest some usable bones!"));
  } else if (corpse->mat == "veggy") {
    m.spawn_item(u.posx, u.posy, "plant_sac", age, bones);
   add_msg(_("You harvest some fluid bladders!"));
  }
 }

 if (sinews > 0) {
  if (corpse->has_flag(MF_BONES)) {
    m.spawn_item(u.posx, u.posy, "sinew", age, sinews);
   add_msg(_("You harvest some usable sinews!"));
  } else if (corpse->mat == "veggy") {
    m.spawn_item(u.posx, u.posy, "plant_fibre", age, sinews);
   add_msg(_("You harvest some plant fibres!"));
  }
 }

 if ((corpse->has_flag(MF_FUR) || corpse->has_flag(MF_LEATHER)) &&
     pelts > 0) {
  add_msg(_("You manage to skin the %s!"), corpse->name.c_str());
  int fur = 0;
  int leather = 0;

  if (corpse->has_flag(MF_FUR) && corpse->has_flag(MF_LEATHER)) {
   fur = rng(0, pelts);
   leather = pelts - fur;
  } else if (corpse->has_flag(MF_FUR)) {
   fur = pelts;
  } else {
   leather = pelts;
  }

  if(fur) m.spawn_item(u.posx, u.posy, "fur", age, fur);
  if(leather) m.spawn_item(u.posx, u.posy, "leather", age, leather);
 }

 if (feathers > 0) {
  if (corpse->has_flag(MF_FEATHER)) {
    m.spawn_item(u.posx, u.posy, "feather", age, feathers);
   add_msg(_("You harvest some feathers!"));
  }
 }

 //Add a chance of CBM recovery. For shocker and cyborg corpses.
 if (corpse->has_flag(MF_CBM)) {
  //As long as the factor is above -4 (the sinew cutoff), you will be able to extract cbms
  if(skill_shift >= 0){
   add_msg(_("You discover a CBM in the %s!"), corpse->name.c_str());
   //To see if it spawns a battery
   if(rng(0,1) == 1){ //The battery works
    m.spawn_item(u.posx, u.posy, "bio_power_storage", age);
   }else{//There is a burnt out CBM
    m.spawn_item(u.posx, u.posy, "burnt_out_bionic", age);
   }
  }
  if(skill_shift >= 0){
   //To see if it spawns a random additional CBM
   if(rng(0,1) == 1){ //The CBM works
    Item_tag bionic_item = item_controller->id_from("bionics");
    m.spawn_item(u.posx, u.posy, bionic_item, age);
   }else{//There is a burnt out CBM
    m.spawn_item(u.posx, u.posy, "burnt_out_bionic", age);
   }
  }
 }

 // Recover hidden items
 for (int i = 0; i < contents.size(); i++) {
   if ((skill_shift + 10) * 5 > rng(0,100)) {
     add_msg(_("You discover a %s in the %s!"), contents[i].tname().c_str(), corpse->name.c_str());
     m.add_item_or_charges(u.posx, u.posy, contents[i]);
   } else if (contents[i].is_bionic()){
     m.spawn_item(u.posx, u.posy, "burnt_out_bionic", age);
   }
 }

 if (pieces <= 0)
  add_msg(_("Your clumsy butchering destroys the meat!"));
 else {
  itype_id meat;
  if (corpse->has_flag(MF_POISON)) {
    if (corpse->mat == "flesh")
     meat = "meat_tainted";
    else
     meat = "veggy_tainted";
  } else {
   if (corpse->mat == "flesh" || corpse->mat == "hflesh")
    if(corpse->has_flag(MF_HUMAN))
     meat = "human_flesh";
    else
     meat = "meat";
   else
    meat = "veggy";
  }
  item tmpitem=item_controller->create(meat, age);
  tmpitem.corpse=dynamic_cast<mtype*>(corpse);
  while ( pieces > 0 ) {
    pieces--;
    m.add_item_or_charges(u.posx, u.posy, tmpitem);
  }
  add_msg(_("You butcher the corpse."));
 }
}

void game::forage()
{
  int veggy_chance = rng(1, 20);

  if (veggy_chance < u.skillLevel("survival"))
  {
    add_msg(_("You found some wild veggies!"));
    u.practice(turn, "survival", 10);
    m.spawn_item(u.activity.placement.x, u.activity.placement.y, "veggy_wild", turn, 0);
    m.ter_set(u.activity.placement.x, u.activity.placement.y, t_dirt);
  }
  else
  {
    add_msg(_("You didn't find anything."));
    if (u.skillLevel("survival") < 7)
        u.practice(turn, "survival", rng(3, 6));
    else
        u.practice(turn, "survival", 1);
    if (one_in(2))
        m.ter_set(u.activity.placement.x, u.activity.placement.y, t_dirt);
  }
}

void game::eat(char chInput)
{
 char ch;
 if (u.has_trait("RUMINANT") && m.ter(u.posx, u.posy) == t_underbrush &&
     query_yn(_("Eat underbrush?"))) {
  u.moves -= 400;
  u.hunger -= 10;
  m.ter_set(u.posx, u.posy, t_grass);
  add_msg(_("You eat the underbrush."));
  return;
 }
 if (chInput == '.')
  ch = inv_type(_("Consume item:"), IC_COMESTIBLE);
 else
  ch = chInput;

 if (ch == ' ') {
  add_msg(_("Never mind."));
  return;
 }

 if (!u.has_item(ch)) {
  add_msg(_("You don't have item '%c'!"), ch);
  return;
 }
 u.eat(this, u.lookup_item(ch));
}

void game::wear(char chInput)
{
 char ch;
 if (chInput == '.')
  ch = inv_type(_("Wear item:"), IC_ARMOR);
 else
  ch = chInput;

 if (ch == ' ') {
  add_msg(_("Never mind."));
  return;
 }
 u.wear(this, ch);
}

void game::takeoff(char chInput)
{
 char ch;
 if (chInput == '.')
  ch = inv_type(_("Take off item:"), IC_NULL);
 else
  ch = chInput;

 if (u.takeoff(this, ch))
  u.moves -= 250; // TODO: Make this variable
 else
  add_msg(_("Invalid selection."));
}

void game::reload(char chInput)
{
 //Quick and dirty hack
 //Save old weapon in temp variable
 //Wield item that should be unloaded
 //Reload weapon
 //Put unloaded item back into inventory
 //Wield old weapon
 bool bSwitch = false;
 item oTempWeapon;
 item inv_it = u.inv.item_by_letter(chInput);

 if (u.weapon.invlet != chInput && !inv_it.is_null()) {
  oTempWeapon = u.weapon;
  u.weapon = inv_it;
  u.inv.remove_item_by_letter(chInput);
  bSwitch = true;
 }

 if (bSwitch || u.weapon.invlet == chInput) {
  reload();
  u.activity.moves_left = 0;
  monmove();
  process_activity();
 }

 if (bSwitch) {
  u.inv.push_back(u.weapon);
  u.weapon = oTempWeapon;
 }
}

void game::reload()
{
 if (u.weapon.is_gun()) {
  if (u.weapon.has_flag("RELOAD_AND_SHOOT")) {
   add_msg(_("Your %s does not need to be reloaded; it reloads and fires in a \
single action."), u.weapon.tname().c_str());
   return;
  }
  if (u.weapon.ammo_type() == "NULL") {
   add_msg(_("Your %s does not reload normally."), u.weapon.tname().c_str());
   return;
  }
  if (u.weapon.charges == u.weapon.clip_size()) {
      int alternate_magazine = -1;
      for (int i = 0; i < u.weapon.contents.size(); i++)
      {
          if ((u.weapon.contents[i].is_gunmod() &&
               (u.weapon.contents[i].typeId() == "spare_mag" &&
                u.weapon.contents[i].charges < (dynamic_cast<it_gun*>(u.weapon.type))->clip)) ||
              ((u.weapon.contents[i].has_flag("MODE_AUX") &&
                u.weapon.contents[i].charges < u.weapon.contents[i].clip_size())))
          {
              alternate_magazine = i;
          }
      }
      if(alternate_magazine == -1) {
          add_msg(_("Your %s is fully loaded!"), u.weapon.tname(this).c_str());
          return;
      }
  }
  char invlet = u.weapon.pick_reload_ammo(u, true);
  if (invlet == 0) {
   add_msg(_("Out of ammo!"));
   return;
  }
  u.assign_activity(this, ACT_RELOAD, u.weapon.reload_time(u), -1, invlet);
  u.moves = 0;
 } else if (u.weapon.is_tool()) {
  it_tool* tool = dynamic_cast<it_tool*>(u.weapon.type);
  if (tool->ammo == "NULL") {
   add_msg(_("You can't reload a %s!"), u.weapon.tname(this).c_str());
   return;
  }
  char invlet = u.weapon.pick_reload_ammo(u, true);
  if (invlet == 0) {
// Reload failed
   add_msg(_("Out of %s!"), ammo_name(tool->ammo).c_str());
   return;
  }
  u.assign_activity(this, ACT_RELOAD, u.weapon.reload_time(u), -1, invlet);
  u.moves = 0;
 } else if (!u.is_armed())
  add_msg(_("You're not wielding anything."));
 else
  add_msg(_("You can't reload a %s!"), u.weapon.tname(this).c_str());
 refresh_all();
}

// Unload a containter, gun, or tool
// If it's a gun, some gunmods can also be loaded
void game::unload(char chInput)
{	// this is necessary to prevent re-selection of the same item later
    item it = (u.inv.remove_item_by_letter(chInput));
    if (!it.is_null())
    {
        unload(it);
        u.i_add(it, this);
    }
    else
    {
        item ite;
        if (u.weapon.invlet == chInput) {	// item is wielded as weapon.
            if (std::find(martial_arts_itype_ids.begin(), martial_arts_itype_ids.end(), u.weapon.type->id) != martial_arts_itype_ids.end()){
                return; //ABORT!
            } else {
                ite = u.weapon;
                u.weapon = item(itypes["null"], 0); //ret_null;
                unload(ite);
                u.weapon = ite;
                return;
            }
        } else {	//this is that opportunity for reselection where the original container is worn, see issue #808
            item& itm = u.i_at(chInput);
            if (!itm.is_null())
            {
                unload(itm);
            }
        }
    }
}

void game::unload(item& it)
{
    if ( !it.is_gun() && it.contents.size() == 0 && (!it.is_tool() || it.ammo_type() == "NULL") )
    {
        add_msg(_("You can't unload a %s!"), it.tname(this).c_str());
        return;
    }
    int spare_mag = -1;
    int has_m203 = -1;
    int has_shotgun = -1;
    if (it.is_gun()) {
        spare_mag = it.has_gunmod ("spare_mag");
        has_m203 = it.has_gunmod ("m203");
        has_shotgun = it.has_gunmod ("u_shotgun");
    }
    if (it.is_container() ||
        (it.charges == 0 &&
         (spare_mag == -1 || it.contents[spare_mag].charges <= 0) &&
         (has_m203 == -1 || it.contents[has_m203].charges <= 0) &&
         (has_shotgun == -1 || it.contents[has_shotgun].charges <= 0)))
    {
        if (it.contents.size() == 0)
        {
            if (it.is_gun())
            {
                add_msg(_("Your %s isn't loaded, and is not modified."),
                        it.tname(this).c_str());
            }
            else
            {
                add_msg(_("Your %s isn't charged.") , it.tname(this).c_str());
            }
            return;
        }
        // Unloading a container!
        u.moves -= 40 * it.contents.size();
        std::vector<item> new_contents;	// In case we put stuff back
        while (it.contents.size() > 0)
        {
            item content = it.contents[0];
            int iter = 0;
// Pick an inventory item for the contents
            while ((content.invlet == 0 || u.has_item(content.invlet)) && iter < inv_chars.size())
            {
                content.invlet = nextinv;
                advance_nextinv();
                iter++;
            }
            if (content.made_of(LIQUID))
            {
                if (!handle_liquid(content, false, false, &it))
                {
                    new_contents.push_back(content);// Put it back in (we canceled)
                }
            } else {
                if (u.can_pickVolume(content.volume()) && u.can_pickWeight(content.weight(), !OPTIONS["DANGEROUS_PICKUPS"]) &&
                    iter < inv_chars.size())
                {
                    add_msg(_("You put the %s in your inventory."), content.tname(this).c_str());
                    u.i_add(content, this);
                } else {
                    add_msg(_("You drop the %s on the ground."), content.tname(this).c_str());
                    m.add_item_or_charges(u.posx, u.posy, content, 1);
                }
            }
            it.contents.erase(it.contents.begin());
        }
        it.contents = new_contents;
        return;
    }

    if(it.has_flag("NO_UNLOAD")) {
      add_msg(_("You can't unload a %s!"), it.tname(this).c_str());
      return;
    }

// Unloading a gun or tool!
 u.moves -= int(it.reload_time(u) / 2);

 // Default to unloading the gun, but then try other alternatives.
 item* weapon = &it;
 if (weapon->is_gun()) {	// Gun ammo is combined with existing items
  // If there's an active gunmod, unload it first.
  item* active_gunmod = weapon->active_gunmod();
  if (active_gunmod != NULL && active_gunmod->charges > 0)
   weapon = active_gunmod;
  // Then try and unload a spare magazine if there is one.
  else if (spare_mag != -1 && weapon->contents[spare_mag].charges > 0)
   weapon = &weapon->contents[spare_mag];
  // Then try the grenade launcher
  else if (has_m203 != -1 && weapon->contents[has_m203].charges > 0)
   weapon = &weapon->contents[has_m203];
  // Then try an underslung shotgun
  else if (has_shotgun != -1 && weapon->contents[has_shotgun].charges > 0)
   weapon = &weapon->contents[has_shotgun];
 }

 item newam;

 if (weapon->curammo != NULL) {
  newam = item(weapon->curammo, turn);
 } else {
  newam = item(itypes[default_ammo(weapon->ammo_type())], turn);
 }
 if(weapon->typeId() == "adv_UPS_off" || weapon->typeId() == "adv_UPS_on") {
    int chargesPerPlutonium = 500;
    int chargesRemoved = weapon->charges - (weapon-> charges % chargesPerPlutonium);;
    int plutoniumRemoved = chargesRemoved / chargesPerPlutonium;
    if(chargesRemoved < weapon->charges) {
        add_msg(_("You can't remove partially depleted plutonium!"));
    }
    if(plutoniumRemoved > 0) {
        add_msg(_("You remove %i plutonium from the advanced UPS"), plutoniumRemoved);
        newam.charges = plutoniumRemoved;
        weapon->charges -= chargesRemoved;
    } else { return; }
 } else {
    newam.charges = weapon->charges;
    weapon->charges = 0;
 }

 if (newam.made_of(LIQUID)) {
  if (!handle_liquid(newam, false, false))
   weapon->charges += newam.charges;	// Put it back in
 } else if(newam.charges > 0) {
  int iter = 0;
  while ((newam.invlet == 0 || u.has_item(newam.invlet)) && iter < inv_chars.size()) {
   newam.invlet = nextinv;
   advance_nextinv();
   iter++;
  }
  if (u.can_pickWeight(newam.weight(), !OPTIONS["DANGEROUS_PICKUPS"]) &&
      u.can_pickVolume(newam.volume()) && iter < inv_chars.size()) {
   u.i_add(newam, this);
  } else {
   m.add_item_or_charges(u.posx, u.posy, newam, 1);
  }
 }
 // null the curammo, but only if we did empty the item
 if (weapon->charges == 0) {
  weapon->curammo = NULL;
 }
}

void game::wield(char chInput)
{
 if (u.weapon.has_flag("NO_UNWIELD")) {
// Bionics can't be unwielded
  add_msg(_("You cannot unwield your %s."), u.weapon.tname(this).c_str());
  return;
 }
 char ch;
 if (chInput == '.') {
  if (u.styles.empty())
   ch = inv(_("Wield item:"));
  else
   ch = inv(_("Wield item: Press - to choose a style"));
 } else
  ch = chInput;

 bool success = false;
 if (ch == '-')
  success = u.wield(this, -3);
 else
  success = u.wield(this, u.lookup_item(ch));

 if (success)
  u.recoil = 5;
}

void game::read()
{
 char ch = inv_type(_("Read:"), IC_BOOK);
 u.read(this, ch);
}

void game::chat()
{
    if (active_npc.size() == 0)
    {
        add_msg(_("You talk to yourself for a moment."));
        return;
    }

    std::vector<npc*> available;

    for (int i = 0; i < active_npc.size(); i++)
    {
        if (u_see(active_npc[i]->posx, active_npc[i]->posy) && rl_dist(u.posx, u.posy, active_npc[i]->posx, active_npc[i]->posy) <= 24)
        {
            available.push_back(active_npc[i]);
        }
    }

    if (available.size() == 0)
    {
        add_msg(_("There's no-one close enough to talk to."));
        return;
    }
    else if (available.size() == 1)
    {
        available[0]->talk_to_u(this);
    }
    else
    {
        std::vector<std::string> npcs;

        for (int i = 0; i < available.size(); i++)
        {
            npcs.push_back(available[i]->name);
        }
        npcs.push_back(_("Cancel"));

        int npc_choice = menu_vec(true, _("Who do you want to talk to?"), npcs) - 1;

        if(npc_choice >= 0 && npc_choice < available.size())
        {
            available[npc_choice]->talk_to_u(this);
        }
    }
    u.moves -= 100;
}

void game::pldrive(int x, int y) {
 if (run_mode == 2) { // Monsters around and we don't wanna run
   add_msg(_("Monster spotted--run mode is on! "
           "(%s to turn it off or %s to ignore monster.)"),
           press_x(ACTION_TOGGLE_SAFEMODE).c_str(),
           from_sentence_case(press_x(ACTION_IGNORE_ENEMY)).c_str());
  return;
 }
 int part = -1;
 vehicle *veh = m.veh_at (u.posx, u.posy, part);
 if (!veh) {
  dbg(D_ERROR) << "game:pldrive: can't find vehicle! Drive mode is now off.";
  debugmsg ("game::pldrive error: can't find vehicle! Drive mode is now off.");
  u.in_vehicle = false;
  return;
 }
 int pctr = veh->part_with_feature (part, vpf_controls);
 if (pctr < 0) {
  add_msg (_("You can't drive the vehicle from here. You need controls!"));
  return;
 }

 int thr_amount = 10 * 100;
 if (veh->cruise_on)
  veh->cruise_thrust (-y * thr_amount);
 else {
  veh->thrust (-y);
 }
 veh->turn (15 * x);
 if (veh->skidding && veh->valid_wheel_config()) {
  if (rng (0, 100) < u.dex_cur + u.skillLevel("driving") * 2) {
   add_msg (_("You regain control of the %s."), veh->name.c_str());
   veh->velocity = veh->forward_velocity();
   veh->skidding = false;
   veh->move.init (veh->turn_dir);
  }
 }
 // Don't spend turns to adjust cruise speed.
 if( x != 0 || !veh->cruise_on ){ u.moves = 0; }

 if (x != 0 && veh->velocity != 0 && one_in(4))
     u.practice(turn, "driving", 1);
}

void game::plmove(int x, int y)
{
 if (run_mode == 2) { // Monsters around and we don't wanna run
   add_msg(_("Monster spotted--safe mode is on! \
(%s to turn it off or %s to ignore monster.)"),
           press_x(ACTION_TOGGLE_SAFEMODE).c_str(),
           from_sentence_case(press_x(ACTION_IGNORE_ENEMY)).c_str());
  return;
 }
 if (u.has_disease("stunned")) {
  x = rng(u.posx - 1, u.posx + 1);
  y = rng(u.posy - 1, u.posy + 1);
 } else {
  x += u.posx;
  y += u.posy;

  if (moveCount % 60 == 0) {
   if (u.has_bionic("bio_torsionratchet")) {
    u.charge_power(1);
   }
  }
 }

 dbg(D_PEDANTIC_INFO) << "game:plmove: From ("<<u.posx<<","<<u.posy<<") to ("<<x<<","<<y<<")";

// Check if our movement is actually an attack on a monster
 int mondex = mon_at(x, y);
 bool displace = false;	// Are we displacing a monster?
 if (mondex != -1) {
  if (z[mondex].friendly == 0) {
   int udam = u.hit_mon(this, &z[mondex]);
   char sMonSym = '%';
   nc_color cMonColor = z[mondex].type->color;
   if (z[mondex].hurt(udam))
    kill_mon(mondex, true);
   else
    sMonSym = z[mondex].symbol();
   hit_animation(x - u.posx + VIEWX - u.view_offset_x,
                 y - u.posy + VIEWY - u.view_offset_y,
                 red_background(cMonColor), sMonSym);
   return;
  } else
   displace = true;
 }
// If not a monster, maybe there's an NPC there
 int npcdex = npc_at(x, y);
 if (npcdex != -1) {
	 if(!active_npc[npcdex]->is_enemy()){
		if (!query_yn(_("Really attack %s?"), active_npc[npcdex]->name.c_str())) {
				if (active_npc[npcdex]->is_friend()) {
					add_msg(_("%s moves out of the way."), active_npc[npcdex]->name.c_str());
					active_npc[npcdex]->move_away_from(this, u.posx, u.posy);
				}

				return;	// Cancel the attack
		} else {
			active_npc[npcdex]->hit_by_player = true; //The NPC knows we started the fight, used for morale penalty.
		}
	 }

	 u.hit_player(this, *active_npc[npcdex]);
	 active_npc[npcdex]->make_angry();
	 if (active_npc[npcdex]->hp_cur[hp_head]  <= 0 ||
		 active_npc[npcdex]->hp_cur[hp_torso] <= 0   ) {
			 active_npc[npcdex]->die(this, true);
	 }
	 return;
 }

// Otherwise, actual movement, zomg
 if (u.has_disease("amigara")) {
  int curdist = 999, newdist = 999;
  for (int cx = 0; cx < SEEX * MAPSIZE; cx++) {
   for (int cy = 0; cy < SEEY * MAPSIZE; cy++) {
    if (m.ter(cx, cy) == t_fault) {
     int dist = rl_dist(cx, cy, u.posx, u.posy);
     if (dist < curdist)
      curdist = dist;
     dist = rl_dist(cx, cy, x, y);
     if (dist < newdist)
      newdist = dist;
    }
   }
  }
  if (newdist > curdist) {
   add_msg(_("You cannot pull yourself away from the faultline..."));
   return;
  }
 }

 if (u.has_disease("in_pit")) {
  if (rng(0, 40) > u.str_cur + int(u.dex_cur / 2)) {
   add_msg(_("You try to escape the pit, but slip back in."));
   u.moves -= 100;
   return;
  } else {
   add_msg(_("You escape the pit!"));
   u.rem_disease("in_pit");
  }
 }
 if (u.has_disease("downed")) {
  if (rng(0, 40) > u.dex_cur + int(u.str_cur / 2)) {
   add_msg(_("You struggle to stand."));
   u.moves -= 100;
   return;
  } else {
   add_msg(_("You stand up."));
   u.rem_disease("downed");
   u.moves -= 100;
   return;
  }
 }

 int vpart0 = -1, vpart1 = -1, dpart = -1;
 vehicle *veh0 = m.veh_at(u.posx, u.posy, vpart0);
 vehicle *veh1 = m.veh_at(x, y, vpart1);
 bool veh_closed_door = false;
 if (veh1) {
  dpart = veh1->part_with_feature (vpart1, vpf_openable);
  veh_closed_door = dpart >= 0 && !veh1->parts[dpart].open;
 }

 if (veh0 && abs(veh0->velocity) > 100) {
  if (!veh1) {
   if (query_yn(_("Dive from moving vehicle?"))) {
    moving_vehicle_dismount(x, y);
   }
   return;
  } else if (veh1 != veh0) {
   add_msg(_("There is another vehicle in the way."));
   return;
  } else if (veh1->part_with_feature(vpart1, vpf_boardable) < 0) {
   add_msg(_("That part of the vehicle is currently unsafe."));
   return;
  }
 }


 if (m.move_cost(x, y) > 0) { // move_cost() of 0 = impassible (e.g. a wall)
  if (u.underwater)
   u.underwater = false;

  //Ask for EACH bad field, maybe not? Maybe say "theres X bad shit in there don't do it."
  field_entry *cur = NULL;
  field &tmpfld = m.field_at(x, y);
  for(std::map<field_id, field_entry*>::iterator field_list_it = tmpfld.getFieldStart();
      field_list_it != tmpfld.getFieldEnd(); ++field_list_it) {
		cur = field_list_it->second;
		if(cur == NULL) continue;
		if (cur->is_dangerous() &&
			!query_yn(_("Really step into that %s?"), cur->name().c_str()))
			return;
	}


// no need to query if stepping into 'benign' traps
/*
  if (m.tr_at(x, y) != tr_null &&
      u.per_cur - u.encumb(bp_eyes) >= traps[m.tr_at(x, y)]->visibility &&
      !query_yn(_("Really step onto that %s?"),traps[m.tr_at(x, y)]->name.c_str()))
   return;
*/

  if (m.tr_at(x, y) != tr_null &&
      u.per_cur - u.encumb(bp_eyes) >= traps[m.tr_at(x, y)]->visibility)
      {
        if (!traps[m.tr_at(x, y)]->is_benign())
                  if (!query_yn(_("Really step onto that %s?"),traps[m.tr_at(x, y)]->name.c_str()))
             return;
      }

// Calculate cost of moving
  bool diag = trigdist && u.posx != x && u.posy != y;
  u.moves -= u.run_cost(m.combined_movecost(u.posx, u.posy, x, y), diag);

// Adjust recoil down
  if (u.recoil > 0) {
    if (int(u.str_cur / 2) + u.skillLevel("gun") >= u.recoil)
    u.recoil = 0;
   else {
     u.recoil -= int(u.str_cur / 2) + u.skillLevel("gun");
    u.recoil = int(u.recoil / 2);
   }
  }
  if ((!u.has_trait("PARKOUR") && m.move_cost(x, y) > 2) ||
      ( u.has_trait("PARKOUR") && m.move_cost(x, y) > 4    ))
  {
   if (veh1 && m.move_cost(x,y) != 2)
    add_msg(_("Moving past this %s is slow!"), veh1->part_info(vpart1).name);
   else
    add_msg(_("Moving past this %s is slow!"), m.name(x, y).c_str());
  }
  if (m.has_flag(rough, x, y) && (!u.in_vehicle)) {
   if (one_in(5) && u.armor_bash(bp_feet) < rng(2, 5)) {
    add_msg(_("You hurt your feet on the %s!"), m.tername(x, y).c_str());
    u.hit(this, bp_feet, 0, 0, 1);
    u.hit(this, bp_feet, 1, 0, 1);
   }
  }
  if (m.has_flag(sharp, x, y) && !one_in(3) && !one_in(40 - int(u.dex_cur/2))
      && (!u.in_vehicle)) {
   if (!u.has_trait("PARKOUR") || one_in(4)) {
    body_part bp = random_body_part();
    int side = rng(0, 1);
    if(u.hit(this, bp, side, 0, rng(1, 4)) > 0)
     add_msg(_("You cut your %s on the %s!"), body_part_name(bp, side).c_str(), m.tername(x, y).c_str());
   }
  }
  if (!u.has_artifact_with(AEP_STEALTH) && !u.has_trait("LEG_TENTACLES")) {
   if (u.has_trait("LIGHTSTEP"))
    sound(x, y, 2, "");	// Sound of footsteps may awaken nearby monsters
   else
    sound(x, y, 6, "");	// Sound of footsteps may awaken nearby monsters
  }
  if (one_in(20) && u.has_artifact_with(AEP_MOVEMENT_NOISE))
   sound(x, y, 40, _("You emit a rattling sound."));
// If we moved out of the nonant, we need update our map data
  if (m.has_flag(swimmable, x, y) && u.has_disease("onfire")) {
   add_msg(_("The water puts out the flames!"));
   u.rem_disease("onfire");
  }
// displace is set at the top of this function.
  if (displace) { // We displaced a friendly monster!
// Immobile monsters can't be displaced.
   if (z[mondex].has_flag(MF_IMMOBILE)) {
// ...except that turrets can be picked up.
// TODO: Make there a flag, instead of hard-coded to mon_turret
    if (z[mondex].type->id == mon_turret) {
     if (query_yn(_("Deactivate the turret?"))) {
      z.erase(z.begin() + mondex);
      u.moves -= 100;
      m.spawn_item(x, y, "bot_turret", turn);
     }
     return;
    } else {
     add_msg(_("You can't displace your %s."), z[mondex].name().c_str());
     return;
    }
   }
   z[mondex].move_to(this, u.posx, u.posy, true); // Force the movement even though the player is there right now.
   add_msg(_("You displace the %s."), z[mondex].name().c_str());
  }
  if (x < SEEX * int(MAPSIZE / 2) || y < SEEY * int(MAPSIZE / 2) ||
      x >= SEEX * (1 + int(MAPSIZE / 2)) || y >= SEEY * (1 + int(MAPSIZE / 2)))
   update_map(x, y);

// If the player is in a vehicle, unboard them from the current part
  if (u.in_vehicle)
   m.unboard_vehicle(this, u.posx, u.posy);

// Move the player
  u.posx = x;
  u.posy = y;

  //Autopickup
  if (OPTIONS["AUTO_PICKUP"] && (!OPTIONS["AUTO_PICKUP_SAFEMODE"] || mostseen == 0) && (m.i_at(u.posx, u.posy)).size() > 0) {
   pickup(u.posx, u.posy, -1);
  }

// If the new tile is a boardable part, board it
  if (veh1 && veh1->part_with_feature(vpart1, vpf_boardable) >= 0)
   m.board_vehicle(this, u.posx, u.posy, &u);

  if (m.tr_at(x, y) != tr_null) { // We stepped on a trap!
   trap* tr = traps[m.tr_at(x, y)];
   if (!u.avoid_trap(tr)) {
    trapfunc f;
    (f.*(tr->act))(this, x, y);
   }
  }

// Some martial art styles have special effects that trigger when we move
  if(u.weapon.type->id == "style_capoeira"){
    if (u.disease_level("attack_boost") < 2)
     u.add_disease("attack_boost", 2, 2, 2);
    if (u.disease_level("dodge_boost") < 2)
     u.add_disease("dodge_boost", 2, 2, 2);
  } else if(u.weapon.type->id == "style_ninjutsu"){
    u.add_disease("attack_boost", 2, 1, 3);
  } else if(u.weapon.type->id == "style_crane"){
    if (!u.has_disease("dodge_boost"))
     u.add_disease("dodge_boost", 1, 3, 3);
  } else if(u.weapon.type->id == "style_leopard"){
    u.add_disease("attack_boost", 2, 1, 4);
  } else if(u.weapon.type->id == "style_dragon"){
    if (!u.has_disease("damage_boost"))
     u.add_disease("damage_boost", 2, 3, 3);
  } else if(u.weapon.type->id == "style_lizard"){
    bool wall = false;
    for (int wallx = x - 1; wallx <= x + 1 && !wall; wallx++) {
     for (int wally = y - 1; wally <= y + 1 && !wall; wally++) {
      if (m.has_flag(supports_roof, wallx, wally))
       wall = true;
     }
    }
    if (wall)
     u.add_disease("attack_boost", 2, 2, 8);
    else
     u.rem_disease("attack_boost");
  }

  // Drench the player if swimmable
  if (m.has_flag(swimmable, x, y))
    u.drench(this, 40, mfb(bp_feet) | mfb(bp_legs));

  // List items here
  if (!m.has_flag(sealed, x, y)) {
    if (!u.has_disease("blind") && m.i_at(x, y).size() <= 3 && m.i_at(x, y).size() != 0) {
      // TODO: Rewrite to be localizable
      std::string buff = _("You see here ");

      for (int i = 0; i < m.i_at(x, y).size(); i++) {
        buff += m.i_at(x, y)[i].tname(this);

        if (i + 2 < m.i_at(x, y).size())
          buff += _(", ");
        else if (i + 1 < m.i_at(x, y).size())
          buff += _(", and ");

      }

      buff += _(".");

      add_msg(buff.c_str());
    } else if (m.i_at(x, y).size() != 0) {
      add_msg(_("There are many items here."));
    }
  }

  if (veh1 && veh1->part_with_feature(vpart1, vpf_controls) >= 0
           && u.in_vehicle)
      add_msg(_("There are vehicle controls here.  %s to drive."),
              press_x(ACTION_CONTROL_VEHICLE).c_str() );

 } else if (!m.has_flag(swimmable, x, y) && u.has_active_bionic("bio_probability_travel")) { //probability travel through walls but not water
  int tunneldist = 0;
  // tile is impassable
  while((m.move_cost(x + tunneldist*(x - u.posx), y + tunneldist*(y - u.posy)) == 0 &&
         // but allow water tiles
         !m.has_flag(swimmable, x + tunneldist*(x - u.posx), y + tunneldist*(y - u.posy))) ||
         // a monster is there
         ((mon_at(x + tunneldist*(x - u.posx), y + tunneldist*(y - u.posy)) != -1 ||
           // so keep tunneling
           npc_at(x + tunneldist*(x - u.posx), y + tunneldist*(y - u.posy)) != -1) &&
          // assuming we've already started
          tunneldist > 0))
  {
      tunneldist += 1; //add 1 to tunnel distance for each impassable tile in the line
      if(tunneldist * 10 > u.power_level) //oops, not enough energy! Tunneling costs 10 bionic power per impassable tile
      {
          add_msg(_("You try to quantum tunnel through the barrier but are reflected! Try again with more energy!"));
          tunneldist = 0; //we didn't tunnel anywhere
          break;
      }
      if(tunneldist > 24)
      {
          add_msg(_("It's too dangerous to tunnel that far!"));
          tunneldist = 0;
          break;    //limit maximum tunneling distance
      }
  }
  if(tunneldist) //you tunneled
  {
    if (u.in_vehicle)
        m.unboard_vehicle(this, u.posx, u.posy);
    u.power_level -= (tunneldist * 10); //tunneling costs 10 bionic power per impassable tile
    u.moves -= 100; //tunneling costs 100 moves
    u.posx += (tunneldist + 1) * (x - u.posx); //move us the number of tiles we tunneled in the x direction, plus 1 for the last tile
    u.posy += (tunneldist + 1) * (y - u.posy); //ditto for y
    add_msg(_("You quantum tunnel through the %d-tile wide barrier!"), tunneldist);
    if (m.veh_at(u.posx, u.posy, vpart1) && m.veh_at(u.posx, u.posy, vpart1)->part_with_feature(vpart1, vpf_boardable) >= 0)
        m.board_vehicle(this, u.posx, u.posy, &u);
  }
  else //or you couldn't tunnel due to lack of energy
  {
      u.power_level -= 10; //failure is expensive!
  }

 } else if (veh_closed_door) { // move_cost <= 0
  veh1->parts[dpart].open = 1;
  veh1->insides_dirty = true;
  u.moves -= 100;
  add_msg (_("You open the %s's %s."), veh1->name.c_str(),
                                    veh1->part_info(dpart).name);

 } else if (m.has_flag(swimmable, x, y)) { // Dive into water!
// Requires confirmation if we were on dry land previously
  if ((m.has_flag(swimmable, u.posx, u.posy) &&
      m.move_cost(u.posx, u.posy) == 0) || query_yn(_("Dive into the water?"))) {
   if (m.move_cost(u.posx, u.posy) > 0 && u.swim_speed() < 500)
     add_msg(_("You start swimming.  %s to dive underwater."),
             press_x(ACTION_MOVE_DOWN).c_str());
   plswim(x, y);
  }
 } else { // Invalid move
  if (u.has_disease("blind") || u.has_disease("stunned")) {
// Only lose movement if we're blind
   add_msg(_("You bump into a %s!"), m.name(x, y).c_str());
   u.moves -= 100;
  } else if (m.furn(x, y) != f_safe_c && m.open_door(x, y, !m.is_outside(u.posx, u.posy)))
   u.moves -= 100;
  else if (m.ter(x, y) == t_door_locked || m.ter(x, y) == t_door_locked_alarm || m.ter(x, y) == t_door_locked_interior) {
   u.moves -= 100;
   add_msg(_("That door is locked!"));
  }
  else if (m.ter(x, y) == t_door_bar_locked) {
   u.moves -= 80;
   add_msg(_("You rattle the bars but the door is locked!"));
  }
 }
}

void game::plswim(int x, int y)
{
 if (x < SEEX * int(MAPSIZE / 2) || y < SEEY * int(MAPSIZE / 2) ||
     x >= SEEX * (1 + int(MAPSIZE / 2)) || y >= SEEY * (1 + int(MAPSIZE / 2)))
  update_map(x, y);
 u.posx = x;
 u.posy = y;
 if (!m.has_flag(swimmable, x, y)) {
  dbg(D_ERROR) << "game:plswim: Tried to swim in "
               << m.tername(x, y).c_str() << "!";
  debugmsg("Tried to swim in %s!", m.tername(x, y).c_str());
  return;
 }
 if (u.has_disease("onfire")) {
  add_msg(_("The water puts out the flames!"));
  u.rem_disease("onfire");
 }
 int movecost = u.swim_speed();
 u.practice(turn, "swimming", u.underwater ? 2 : 1);
 if (movecost >= 500) {
  if (!u.underwater) {
    add_msg(_("You sink like a rock!"));
   u.underwater = true;
   u.oxygen = 30 + 2 * u.str_cur;
  }
 }
 if (u.oxygen <= 5 && u.underwater) {
  if (movecost < 500)
    popup(_("You need to breathe! (%s to surface.)"),
          press_x(ACTION_MOVE_UP).c_str());
  else
   popup(_("You need to breathe but you can't swim!  Get to dry land, quick!"));
 }
 bool diagonal = (x != u.posx && y != u.posy);
 u.moves -= (movecost > 200 ? 200 : movecost)  * (trigdist && diagonal ? 1.41 : 1 );
 u.inv.rust_iron_items();

 int drenchFlags = mfb(bp_legs)|mfb(bp_torso)|mfb(bp_arms);

 if (temperature < 50)
   drenchFlags |= mfb(bp_feet)|mfb(bp_hands);

 if (u.underwater)
   drenchFlags |= mfb(bp_head)|mfb(bp_eyes)|mfb(bp_mouth);

 u.drench(this, 100, drenchFlags);
}

void game::fling_player_or_monster(player *p, monster *zz, const int& dir, float flvel, bool controlled)
{
    int steps = 0;
    bool is_u = p && (p == &u);
    int dam1, dam2;

    bool is_player;
    if (p)
        is_player = true;
    else
    if (zz)
        is_player = false;
    else
    {
     dbg(D_ERROR) << "game:fling_player_or_monster: "
                     "neither player nor monster";
     debugmsg ("game::fling neither player nor monster");
     return;
    }

    tileray tdir(dir);
    std::string sname, snd;
    if (p)
    {
        if (is_u)
            sname = std::string (_("You are"));
        else
            sname = p->name + _(" is");
    }
    else
        sname = zz->name() + _(" is");
    int range = flvel / 10;
    int x = (is_player? p->posx : zz->posx);
    int y = (is_player? p->posy : zz->posy);
    while (range > 0)
    {
        tdir.advance();
        x = (is_player? p->posx : zz->posx) + tdir.dx();
        y = (is_player? p->posy : zz->posy) + tdir.dy();
        std::string dname;
        bool thru = true;
        bool slam = false;
        int mondex = mon_at(x, y);
        dam1 = flvel / 3 + rng (0, flvel * 1 / 3);
        if (controlled)
            dam1 = std::max(dam1 / 2 - 5, 0);
        if (mondex >= 0)
        {
            slam = true;
            dname = z[mondex].name();
            dam2 = flvel / 3 + rng (0, flvel * 1 / 3);
            if (z[mondex].hurt(dam2))
             kill_mon(mondex, false);
            else
             thru = false;
            if (is_player)
             p->hitall (this, dam1, 40);
            else
                zz->hurt(dam1);
        } else if (m.move_cost(x, y) == 0 && !m.has_flag(swimmable, x, y)) {
            slam = true;
            int vpart;
            vehicle *veh = m.veh_at(x, y, vpart);
            dname = veh ? veh->part_info(vpart).name : m.tername(x, y).c_str();
            if (m.has_flag(bashable, x, y))
                thru = m.bash(x, y, flvel, snd);
            else
                thru = false;
            if (snd.length() > 0)
                add_msg (_("You hear a %s"), snd.c_str());
            if (is_player)
                p->hitall (this, dam1, 40);
            else
                zz->hurt (dam1);
            flvel = flvel / 2;
        }
        if (slam && dam1)
            add_msg (_("%s slammed against the %s for %d damage!"), sname.c_str(), dname.c_str(), dam1);
        if (thru)
        {
            if (is_player)
            {
                p->posx = x;
                p->posy = y;
            }
            else
            {
                zz->posx = x;
                zz->posy = y;
            }
        }
        else
            break;
        range--;
        steps++;
        timespec ts;   // Timespec for the animation
        ts.tv_sec = 0;
        ts.tv_nsec = BILLION / 20;
        nanosleep (&ts, 0);
    }

    if (!m.has_flag(swimmable, x, y))
    {
        // fall on ground
        dam1 = rng (flvel / 3, flvel * 2 / 3) / 2;
        if (controlled)
            dam1 = std::max(dam1 / 2 - 5, 0);
        if (is_player)
        {
            int dex_reduce = p->dex_cur < 4? 4 : p->dex_cur;
            dam1 = dam1 * 8 / dex_reduce;
            if (p->has_trait("PARKOUR"))
            {
                dam1 /= 2;
            }
            if (dam1 > 0)
            {
                p->hitall (this, dam1, 40);
            }
        } else {
            zz->hurt (dam1);
        }
        if (is_u)
        {
            if (dam1 > 0)
            {
                add_msg (_("You fall on the ground for %d damage."), dam1);
            } else if (!controlled) {
                add_msg (_("You land on the ground."));
            }
        }
    }
    else if (is_u)
    {
        if (controlled)
            add_msg (_("You dive into water."));
        else
            add_msg (_("You fall into water."));
    }
}

void game::vertical_move(int movez, bool force)
{
// > and < are used for diving underwater.
 if (m.move_cost(u.posx, u.posy) == 0 && m.has_flag(swimmable, u.posx, u.posy)){
  if (movez == -1) {
   if (u.underwater) {
    add_msg(_("You are already underwater!"));
    return;
   }
   if (u.worn_with_flag("FLOATATION")) {
    add_msg(_("You can't dive while wearing a flotation device."));
    return;
   }
   u.underwater = true;
   u.oxygen = 30 + 2 * u.str_cur;
   add_msg(_("You dive underwater!"));
  } else {
   if (u.swim_speed() < 500) {
    u.underwater = false;
    add_msg(_("You surface."));
   } else
    add_msg(_("You can't surface!"));
  }
  return;
 }
// Force means we're going down, even if there's no staircase, etc.
// This happens with sinkholes and the like.
 if (!force && ((movez == -1 && !m.has_flag(goes_down, u.posx, u.posy)) ||
                (movez ==  1 && !m.has_flag(goes_up,   u.posx, u.posy))) &&
                !(m.ter(u.posx, u.posy) == t_elevator)) {
  if (movez == -1) {
    add_msg(_("You can't go down here!"));
  } else {
    add_msg(_("You can't go up here!"));
  }
  return;
 }

 map tmpmap(&traps);
 tmpmap.load(this, levx, levy, levz + movez, false);
// Find the corresponding staircase
 int stairx = -1, stairy = -1;
 bool rope_ladder = false;
 if (force) {
  stairx = u.posx;
  stairy = u.posy;
 } else { // We need to find the stairs.
  int best = 999;
   for (int i = u.posx - SEEX * 2; i <= u.posx + SEEX * 2; i++) {
    for (int j = u.posy - SEEY * 2; j <= u.posy + SEEY * 2; j++) {
    if (rl_dist(u.posx, u.posy, i, j) <= best &&
        ((movez == -1 && tmpmap.has_flag(goes_up, i, j)) ||
         (movez == 1 && (tmpmap.has_flag(goes_down, i, j) ||
                         tmpmap.ter(i, j) == t_manhole_cover)) ||
         ((movez == 2 || movez == -2) && tmpmap.ter(i, j) == t_elevator))) {
     stairx = i;
     stairy = j;
     best = rl_dist(u.posx, u.posy, i, j);
    }
   }
  }

  if (stairx == -1 || stairy == -1) { // No stairs found!
   if (movez < 0) {
    if (tmpmap.move_cost(u.posx, u.posy) == 0) {
     popup(_("Halfway down, the way down becomes blocked off."));
     return;
    } else if (u.has_amount("rope_30", 1)) {
     if (query_yn(_("There is a sheer drop halfway down. Climb your rope down?"))){
      rope_ladder = true;
      u.use_amount("rope_30", 1);
     } else
      return;
    } else if (!query_yn(_("There is a sheer drop halfway down.  Jump?")))
     return;
   }
   stairx = u.posx;
   stairy = u.posy;
  }
 }

 bool replace_monsters = false;
// Replace the stair monsters if we just came back
 if (abs(monstairx - levx) <= 1 && abs(monstairy - levy) <= 1 &&
     monstairz == levz + movez)
  replace_monsters = true;

 if (!force) {
  monstairx = levx;
  monstairy = levy;
  monstairz = levz;
 }
 // Despawn monsters, only push them onto the stair monster list if we're taking stairs.
 despawn_monsters( abs(movez) == 1 && !force );
 z.clear();

// Figure out where we know there are up/down connectors
 std::vector<point> discover;
 for (int x = 0; x < OMAPX; x++) {
  for (int y = 0; y < OMAPY; y++) {
   if (cur_om->seen(x, y, levz) &&
       ((movez ==  1 && oterlist[ cur_om->ter(x, y, levz) ].known_up) ||
        (movez == -1 && oterlist[ cur_om->ter(x, y, levz) ].known_down) ))
    discover.push_back( point(x, y) );
  }
 }

 int z_coord = levz + movez;
 // Fill in all the tiles we know about (e.g. subway stations)
 for (int i = 0; i < discover.size(); i++) {
  int x = discover[i].x, y = discover[i].y;
  cur_om->seen(x, y, z_coord) = true;
  if (movez ==  1 && !oterlist[ cur_om->ter(x, y, z_coord) ].known_down &&
      !cur_om->has_note(x, y, z_coord))
   cur_om->add_note(x, y, z_coord, _("AUTO: goes down"));
  if (movez == -1 && !oterlist[ cur_om->ter(x, y, z_coord) ].known_up &&
      !cur_om->has_note(x, y, z_coord))
   cur_om->add_note(x, y, z_coord, _("AUTO: goes up"));
 }

 levz += movez;
 u.moves -= 100;
 m.clear_vehicle_cache();
 m.vehicle_list.clear();
 m.load(this, levx, levy, levz);
 u.posx = stairx;
 u.posy = stairy;
 if (rope_ladder)
  m.ter_set(u.posx, u.posy, t_rope_up);
 if (m.ter(stairx, stairy) == t_manhole_cover) {
  m.spawn_item(stairx + rng(-1, 1), stairy + rng(-1, 1), "manhole_cover", 0);
  m.ter_set(stairx, stairy, t_manhole);
 }

 if (replace_monsters)
  replace_stair_monsters();

 m.spawn_monsters(this);

 if (force) {	// Basically, we fell.
  if (u.has_trait("WINGS_BIRD"))
   add_msg(_("You flap your wings and flutter down gracefully."));
  else {
   int dam = int((u.str_max / 4) + rng(5, 10)) * rng(1, 3);//The bigger they are
   dam -= rng(u.dodge(this), u.dodge(this) * 3);
   if (dam <= 0)
    add_msg(_("You fall expertly and take no damage."));
   else {
    add_msg(_("You fall heavily, taking %d damage."), dam);
    u.hurtall(dam);
   }
  }
 }

 if (m.tr_at(u.posx, u.posy) != tr_null) { // We stepped on a trap!
  trap* tr = traps[m.tr_at(u.posx, u.posy)];
  if (force || !u.avoid_trap(tr)) {
   trapfunc f;
   (f.*(tr->act))(this, u.posx, u.posy);
  }
 }

 set_adjacent_overmaps(true);
 refresh_all();
}


void game::update_map(int &x, int &y)
{
 int shiftx = 0, shifty = 0;
 int olevx = 0, olevy = 0;
 while (x < SEEX * int(MAPSIZE / 2)) {
  x += SEEX;
  shiftx--;
 }
 while (x >= SEEX * (1 + int(MAPSIZE / 2))) {
  x -= SEEX;
  shiftx++;
 }
 while (y < SEEY * int(MAPSIZE / 2)) {
  y += SEEY;
  shifty--;
 }
 while (y >= SEEY * (1 + int(MAPSIZE / 2))) {
  y -= SEEY;
  shifty++;
 }
 m.shift(this, levx, levy, levz, shiftx, shifty);
 levx += shiftx;
 levy += shifty;
 if (levx < 0) {
  levx += OMAPX * 2;
  olevx = -1;
 } else if (levx > OMAPX * 2 - 1) {
  levx -= OMAPX * 2;
  olevx = 1;
 }
 if (levy < 0) {
  levy += OMAPY * 2;
  olevy = -1;
 } else if (levy > OMAPY * 2 - 1) {
  levy -= OMAPY * 2;
  olevy = 1;
 }
 if (olevx != 0 || olevy != 0) {
  cur_om->save();
  cur_om = &overmap_buffer.get(this, cur_om->pos().x + olevx, cur_om->pos().y + olevy);
 }
 set_adjacent_overmaps();

 // Shift monsters if we're actually shifting
 if(shiftx || shifty)
  despawn_monsters(false, shiftx, shifty);

 // Shift NPCs
 for (int i = 0; i < active_npc.size(); i++) {
  active_npc[i]->shift(shiftx, shifty);
  if (active_npc[i]->posx < 0 - SEEX * 2 ||
      active_npc[i]->posy < 0 - SEEX * 2 ||
      active_npc[i]->posx >     SEEX * (MAPSIZE + 2) ||
      active_npc[i]->posy >     SEEY * (MAPSIZE + 2)   ) {
   active_npc[i]->mapx = levx + (active_npc[i]->posx / SEEX);
   active_npc[i]->mapy = levy + (active_npc[i]->posy / SEEY);
   active_npc[i]->posx %= SEEX;
   active_npc[i]->posy %= SEEY;
    //don't remove them from the overmap list.
   active_npc.erase(active_npc.begin() + i); //Remove the npc from the active list. It remains in the overmap list.
   i--;
  }
 }
    // Check for overmap saved npcs that should now come into view.
    // Put those in the active list.
    load_npcs();
 // Spawn monsters if appropriate
 m.spawn_monsters(this);	// Static monsters
 if (turn >= nextspawn)
  spawn_mon(shiftx, shifty);
// Shift scent
 unsigned int newscent[SEEX * MAPSIZE][SEEY * MAPSIZE];
 for (int i = 0; i < SEEX * MAPSIZE; i++) {
  for (int j = 0; j < SEEY * MAPSIZE; j++)
   newscent[i][j] = scent(i + (shiftx * SEEX), j + (shifty * SEEY));
 }
 for (int i = 0; i < SEEX * MAPSIZE; i++) {
  for (int j = 0; j < SEEY * MAPSIZE; j++)
   scent(i, j) = newscent[i][j];

 }
 // Make sure map cache is consistent since it may have shifted.
 m.build_map_cache(this);
// Update what parts of the world map we can see
 update_overmap_seen();
 draw_minimap();
}

void game::set_adjacent_overmaps(bool from_scratch)
{
 bool do_h = false, do_v = false, do_d = false;
 int hori_disp = (levx > OMAPX) ? 1 : -1;
 int vert_disp = (levy > OMAPY) ? 1 : -1;
 int diag_posx = cur_om->pos().x + hori_disp;
 int diag_posy = cur_om->pos().y + vert_disp;

 if(!om_hori || om_hori->pos().x != diag_posx || om_hori->pos().y != cur_om->pos().y || from_scratch)
  do_h = true;
 if(!om_vert || om_vert->pos().x != cur_om->pos().x || om_vert->pos().y != diag_posy || from_scratch)
  do_v = true;
 if(!om_diag || om_diag->pos().x != diag_posx || om_diag->pos().y != diag_posy || from_scratch)
  do_d = true;

 if(do_h){
  om_hori = &overmap_buffer.get(this, diag_posx, cur_om->pos().y);
 }
 if(do_v){
  om_vert = &overmap_buffer.get(this, cur_om->pos().x, diag_posy);
 }
 if(do_d){
  om_diag = &overmap_buffer.get(this, diag_posx, diag_posy);
 }
}

void game::update_overmap_seen()
{
 int omx = (levx + int(MAPSIZE / 2)) / 2, omy = (levy + int(MAPSIZE / 2)) / 2;
 int dist = u.overmap_sight_range(light_level());
 cur_om->seen(omx, omy, levz) = true; // We can always see where we're standing
 if (dist == 0)
  return; // No need to run the rest!
 for (int x = omx - dist; x <= omx + dist; x++) {
  for (int y = omy - dist; y <= omy + dist; y++) {
   std::vector<point> line = line_to(omx, omy, x, y, 0);
   int sight_points = dist;
   int cost = 0;
   for (int i = 0; i < line.size() && sight_points >= 0; i++) {
    int lx = line[i].x, ly = line[i].y;
    if (lx >= 0 && lx < OMAPX && ly >= 0 && ly < OMAPY)
     cost = oterlist[cur_om->ter(lx, ly, levz)].see_cost;
    else if ((lx < 0 || lx >= OMAPX) && (ly < 0 || ly >= OMAPY)) {
     if (lx < 0) lx += OMAPX;
     else        lx -= OMAPX;
     if (ly < 0) ly += OMAPY;
     else        ly -= OMAPY;
     cost = oterlist[om_diag->ter(lx, ly, levz)].see_cost;
    } else if (lx < 0 || lx >= OMAPX) {
     if (lx < 0) lx += OMAPX;
     else        lx -= OMAPX;
     cost = oterlist[om_hori->ter(lx, ly, levz)].see_cost;
    } else if (ly < 0 || ly >= OMAPY) {
     if (ly < 0) ly += OMAPY;
     else        ly -= OMAPY;
     cost = oterlist[om_vert->ter(lx, ly, levz)].see_cost;
    }
    sight_points -= cost;
   }
   if (sight_points >= 0) {
    int tmpx = x, tmpy = y;
    if (tmpx >= 0 && tmpx < OMAPX && tmpy >= 0 && tmpy < OMAPY)
     cur_om->seen(tmpx, tmpy, levz) = true;
    else if ((tmpx < 0 || tmpx >= OMAPX) && (tmpy < 0 || tmpy >= OMAPY)) {
     if (tmpx < 0) tmpx += OMAPX;
     else          tmpx -= OMAPX;
     if (tmpy < 0) tmpy += OMAPY;
     else          tmpy -= OMAPY;
     om_diag->seen(tmpx, tmpy, levz) = true;
    } else if (tmpx < 0 || tmpx >= OMAPX) {
     if (tmpx < 0) tmpx += OMAPX;
     else          tmpx -= OMAPX;
     om_hori->seen(tmpx, tmpy, levz) = true;
    } else if (tmpy < 0 || tmpy >= OMAPY) {
     if (tmpy < 0) tmpy += OMAPY;
     else          tmpy -= OMAPY;
     om_vert->seen(tmpx, tmpy, levz) = true;
    }
   }
  }
 }
}

point game::om_location()
{
 point ret;
 ret.x = int( (levx + int(MAPSIZE / 2)) / 2);
 ret.y = int( (levy + int(MAPSIZE / 2)) / 2);
 return ret;
}

void game::replace_stair_monsters()
{
 for (int i = 0; i < coming_to_stairs.size(); i++)
  z.push_back(coming_to_stairs[i].mon);
 coming_to_stairs.clear();
}

void game::update_stair_monsters()
{
 if (abs(levx - monstairx) > 1 || abs(levy - monstairy) > 1)
  return;

 for (int i = 0; i < coming_to_stairs.size(); i++) {
  coming_to_stairs[i].count--;
  if (coming_to_stairs[i].count <= 0) {
   int startx = rng(0, SEEX * MAPSIZE - 1), starty = rng(0, SEEY * MAPSIZE - 1);
   bool found_stairs = false;
   for (int x = 0; x < SEEX * MAPSIZE && !found_stairs; x++) {
    for (int y = 0; y < SEEY * MAPSIZE && !found_stairs; y++) {
     int sx = (startx + x) % (SEEX * MAPSIZE),
         sy = (starty + y) % (SEEY * MAPSIZE);
     if (m.has_flag(goes_up, sx, sy) || m.has_flag(goes_down, sx, sy)) {
      found_stairs = true;
      int mposx = sx, mposy = sy;
      int tries = 0;
      while (!is_empty(mposx, mposy) && tries < 10) {
       mposx = sx + rng(-2, 2);
       mposy = sy + rng(-2, 2);
       tries++;
      }
      if (tries < 10) {
       coming_to_stairs[i].mon.posx = sx;
       coming_to_stairs[i].mon.posy = sy;
       z.push_back( coming_to_stairs[i].mon );
       if (u_see(sx, sy)) {
        if (m.has_flag(goes_up, sx, sy)) {
            add_msg(_("A %s comes down the %s!"), coming_to_stairs[i].mon.name().c_str(),
                m.tername(sx, sy).c_str());
        } else {
            add_msg(_("A %s comes up the %s!"), coming_to_stairs[i].mon.name().c_str(),
                m.tername(sx, sy).c_str());
        }
       }
      }
     }
    }
   }
   coming_to_stairs.erase(coming_to_stairs.begin() + i);
   i--;
  }
 }
 if (coming_to_stairs.empty()) {
  monstairx = -1;
  monstairy = -1;
  monstairz = 999;
 }
}

void game::despawn_monsters(const bool stairs, const int shiftx, const int shifty)
{
 for (unsigned int i = 0; i < z.size(); i++) {
  // If either shift argument is non-zero, we're shifting.
  if(shiftx != 0 || shifty != 0) {
   z[i].shift(shiftx, shifty);
   if (z[i].posx >= 0 - SEEX             && z[i].posy >= 0 - SEEX &&
       z[i].posx <= SEEX * (MAPSIZE + 1) && z[i].posy <= SEEY * (MAPSIZE + 1))
     // We're inbounds, so don't despawn after all.
     continue;
  }

  if (stairs && z[i].will_reach(this, u.posx, u.posy)) {
   int turns = z[i].turns_to_reach(this, u.posx, u.posy);
   if (turns < 999)
    coming_to_stairs.push_back( monster_and_count(z[i], 1 + turns) );
  } else if ( (z[i].spawnmapx != -1) ||
      ((stairs || shiftx != 0 || shifty != 0) && z[i].friendly != 0 ) ) {
    // translate shifty relative coordinates to submapx, submapy, subtilex, subtiley
    real_coords rc(levx, levy, z[i].posx, z[i].posy); // this is madness
    z[i].spawnmapx = rc.sub.x;
    z[i].spawnmapy = rc.sub.y;
    z[i].spawnposx = rc.sub_pos.x;
    z[i].spawnposy = rc.sub_pos.y;

    tinymap tmp(&traps);
    tmp.load(this, z[i].spawnmapx, z[i].spawnmapy, levz, false);
    tmp.add_spawn(&(z[i]));
    tmp.save(cur_om, turn, z[i].spawnmapx, z[i].spawnmapy, levz);
  } else {
   	// No spawn site, so absorb them back into a group.
   int group = valid_group((mon_id)(z[i].type->id), levx + shiftx, levy + shifty, levz);
   if (group != -1) {
    cur_om->zg[group].population++;
    if (cur_om->zg[group].population / (cur_om->zg[group].radius * cur_om->zg[group].radius) > 5 &&
        !cur_om->zg[group].diffuse)
     cur_om->zg[group].radius++;
   }
  }
  // Shifting needs some cleanup for despawned monsters since they won't be cleared afterwards.
  if(shiftx != 0 || shifty != 0) {
    z.erase(z.begin()+i);
    i--;
  }
 }
}

void game::spawn_mon(int shiftx, int shifty)
{
 int nlevx = levx + shiftx;
 int nlevy = levy + shifty;
 int group;
 int monx, mony;
 int dist;
 int pop, rad;
 int iter;
 int t;
 // Create a new NPC?
 if (OPTIONS["RANDOM_NPC"] && one_in(100 + 15 * cur_om->npcs.size())) {
  npc * tmp = new npc();
  tmp->normalize(this);
  tmp->randomize(this);
  //tmp->stock_missions(this);
  tmp->spawn_at(cur_om, levx, levy, levz);
  tmp->place_near(this, SEEX * 2 * (tmp->mapx - levx) + rng(0 - SEEX, SEEX), SEEY * 2 * (tmp->mapy - levy) + rng(0 - SEEY, SEEY));
  tmp->form_opinion(&u);
  //tmp->attitude = NPCATT_TALK; //Form opinion seems to set the attitude.
  tmp->mission = NPC_MISSION_NULL;
  int mission_index = reserve_random_mission(ORIGIN_ANY_NPC,
                                             om_location(), tmp->getID());
  if (mission_index != -1)
  tmp->chatbin.missions.push_back(mission_index);
  active_npc.push_back(tmp);
 }

// Now, spawn monsters (perhaps)
 monster zom;
 for (int i = 0; i < cur_om->zg.size(); i++) { // For each valid group...
 	if (cur_om->zg[i].posz != levz) { continue; } // skip other levels - hack
  group = 0;
  if(cur_om->zg[i].diffuse)
   dist = square_dist(nlevx, nlevy, cur_om->zg[i].posx, cur_om->zg[i].posy);
  else
   dist = trig_dist(nlevx, nlevy, cur_om->zg[i].posx, cur_om->zg[i].posy);
  pop = cur_om->zg[i].population;
  rad = cur_om->zg[i].radius;
  if (dist <= rad) {
// (The area of the group's territory) in (population/square at this range)
// chance of adding one monster; cap at the population OR 16
   while ( (cur_om->zg[i].diffuse ?
            long( pop) :
            long((1.0 - double(dist / rad)) * pop) )
	  > rng(0, (rad * rad)) &&
          rng(0, MAPSIZE * 4) > group && group < pop && group < MAPSIZE * 3)
    group++;

   cur_om->zg[i].population -= group;
   // Reduce group radius proportionally to remaining
   // population to maintain a minimal population density.
   if (cur_om->zg[i].population / (cur_om->zg[i].radius * cur_om->zg[i].radius) < 1.0 &&
       !cur_om->zg[i].diffuse)
     cur_om->zg[i].radius--;

   if (group > 0) // If we spawned some zombies, advance the timer
    nextspawn += rng(group * 4 + z.size() * 4, group * 10 + z.size() * 10);

   for (int j = 0; j < group; j++) {	// For each monster in the group...
     mon_id type = MonsterGroupManager::GetMonsterFromGroup( cur_om->zg[i].type, &mtypes,
                                                             &group, (int)turn );
     zom = monster(mtypes[type]);
     iter = 0;
     do {
      monx = rng(0, SEEX * MAPSIZE - 1);
      mony = rng(0, SEEY * MAPSIZE - 1);
      if (shiftx == 0 && shifty == 0) {
       if (one_in(2))
        shiftx = 1 - 2 * rng(0, 1);
       else
        shifty = 1 - 2 * rng(0, 1);
      }
      if (shiftx == -1)
       monx = (SEEX * MAPSIZE) / 6;
      else if (shiftx == 1)
       monx = (SEEX * MAPSIZE * 5) / 6;
      if (shifty == -1)
       mony = (SEEY * MAPSIZE) / 6;
      if (shifty == 1)
       mony = (SEEY * MAPSIZE * 5) / 6;
      monx += rng(-5, 5);
      mony += rng(-5, 5);
      iter++;

     } while ((!zom.can_move_to(this, monx, mony) || !is_empty(monx, mony) ||
               m.sees(u.posx, u.posy, monx, mony, SEEX, t) || !m.is_outside(monx, mony) ||
               rl_dist(u.posx, u.posy, monx, mony) < 8) && iter < 50);
     if (iter < 50) {
      zom.spawn(monx, mony);
      z.push_back(zom);
     }
   }	// Placing monsters of this group is done!
   if (cur_om->zg[i].population <= 0) { // Last monster in the group spawned...
    cur_om->zg.erase(cur_om->zg.begin() + i); // ...so remove that group
    i--;	// And don't increment i.
   }
  }
 }
}

int game::valid_group(mon_id type, int x, int y, int z_coord)
{
 std::vector <int> valid_groups;
 std::vector <int> semi_valid;	// Groups that're ALMOST big enough
 int dist;
 for (int i = 0; i < cur_om->zg.size(); i++) {
 	if (cur_om->zg[i].posz != z_coord) { continue; }
  dist = trig_dist(x, y, cur_om->zg[i].posx, cur_om->zg[i].posy);
  if (dist < cur_om->zg[i].radius) {
   if(MonsterGroupManager::IsMonsterInGroup(cur_om->zg[i].type, type)) {
     valid_groups.push_back(i);
   }
  } else if (dist < cur_om->zg[i].radius + 3) {
   if(MonsterGroupManager::IsMonsterInGroup(cur_om->zg[i].type, type)) {
     semi_valid.push_back(i);
   }
  }
 }
 if (valid_groups.size() == 0) {
  if (semi_valid.size() == 0)
   return -1;
  else {
// If there's a group that's ALMOST big enough, expand that group's radius
// by one and absorb into that group.
   int semi = rng(0, semi_valid.size() - 1);
   if (!cur_om->zg[semi_valid[semi]].diffuse)
    cur_om->zg[semi_valid[semi]].radius++;
   return semi_valid[semi];
  }
 }
 return valid_groups[rng(0, valid_groups.size() - 1)];
}

void game::wait()
{
    const bool bHasWatch = u.has_item_with_flag("WATCH");

    uimenu as_m;
    as_m.text = _("Wait for how long?");
    as_m.entries.push_back(uimenu_entry(1, true, '1', (bHasWatch) ? _("5 Minutes") : _("Wait 300 heartbeats") ));
    as_m.entries.push_back(uimenu_entry(2, true, '2', (bHasWatch) ? _("30 Minutes") : _("Wait 1800 heartbeats") ));
    as_m.entries.push_back(uimenu_entry(3, true, '3', (bHasWatch) ? _("1 hour") : _("Wait till dawn") ));
    as_m.entries.push_back(uimenu_entry(4, true, '4', (bHasWatch) ? _("2 hours") : _("Wait till noon") ));
    as_m.entries.push_back(uimenu_entry(5, true, '5', (bHasWatch) ? _("3 hours") : _("Wait till dusk") ));
    as_m.entries.push_back(uimenu_entry(6, true, '6', (bHasWatch) ? _("6 hours") : _("Wait till midnight") ));
    as_m.entries.push_back(uimenu_entry(7, true, '7', _("Exit") ));
    as_m.query(); /* calculate key and window variables, generate window, and loop until we get a valid answer */

    const int iHour = turn.getHour();

    int time = 0;
    switch (as_m.ret) {
        case 1:
            time =   5000;
            break;
        case 2:
            time =  30000;
            break;
        case 3:
            time =  (bHasWatch) ? 60000 : (60000 * ((iHour <= 6) ? 6-iHour : 24-iHour+6));
            break;
        case 4:
            time = (bHasWatch) ? 120000 : (60000 * ((iHour <= 12) ? 12-iHour : 12-iHour+6));
            break;
        case 5:
            time = (bHasWatch) ? 180000 : (60000 * ((iHour <= 18) ? 18-iHour : 18-iHour+6));
            break;
        case 6:
            time = (bHasWatch) ? 360000 : (60000 * ((iHour <= 24) ? 24-iHour : 24-iHour+6));
            break;
        default:
            return;
    }

    u.assign_activity(this, ACT_WAIT, time, 0);
    u.activity.continuous = true;
    u.moves = 0;
}

void game::gameover()
{
 erase();
 gamemode->game_over(this);
 mvprintw(0, 35, _("GAME OVER"));
 inv(_("Inventory:"));
}

bool game::game_quit() { return (uquit == QUIT_MENU); }

bool game::game_error() { return (uquit == QUIT_ERROR); }

void game::write_msg()
{
    werase(w_messages);
    int maxlength = getmaxx(w_messages);

    // Print monster info and start our output below it.
    const int topline = mon_info(w_messages) + 2;

    int line = getmaxy(w_messages) - 1;
    for (int i = messages.size() - 1; i >= 0 && line >= topline; i--) {
        game_message &m = messages[i];
        std::string mstr = m.message;
        if (m.count > 1) {
            std::stringstream mesSS;
            mesSS << mstr << " x " << m.count;
            mstr = mesSS.str();
        }
        // Split the message into many if we must!
        nc_color col = c_dkgray;
        if (int(m.turn) >= curmes)
            col = c_ltred;
        else if (int(m.turn) + 5 >= curmes)
            col = c_ltgray;
        std::vector<std::string> folded = foldstring(mstr, maxlength);
        for (int j = folded.size() - 1; j >= 0 && line >= topline; j--, line--) {
            mvwprintz(w_messages, line, 0, col, folded[j].c_str());
        }
    }
    curmes = int(turn);
    wrefresh(w_messages);
}

void game::msg_buffer()
{
 WINDOW *w = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                     (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0,
                     (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0);

 int offset = 0;
 InputEvent input;
 do {
  werase(w);
  wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
             LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
  mvwprintz(w, FULL_SCREEN_HEIGHT-1, 32, c_red, _("Press q to return"));

  int line = 1;
  int lasttime = -1;
  int i;
  for (i = 1; i <= 20 && line <= FULL_SCREEN_HEIGHT-2 && offset + i <= messages.size(); i++) {
   game_message *mtmp = &(messages[ messages.size() - (offset + i) ]);
   calendar timepassed = turn - mtmp->turn;

   if (int(timepassed) > lasttime) {
    mvwprintz(w, line, 3, c_ltblue, _("%s ago:"),
              timepassed.textify_period().c_str());
    line++;
    lasttime = int(timepassed);
   }

   if (line <= FULL_SCREEN_HEIGHT-2) { // Print the actual message... we may have to split it
    std::string mes = mtmp->message;
    if (mtmp->count > 1) {
     std::stringstream mesSS;
     mesSS << mes << " x " << mtmp->count;
     mes = mesSS.str();
    }
// Split the message into many if we must!
    std::vector<std::string> folded = foldstring(mes, FULL_SCREEN_WIDTH-2);
    for(int j=0; j<folded.size() && line <= FULL_SCREEN_HEIGHT-2; j++, line++) {
     mvwprintz(w, line, 1, c_ltgray, folded[j].c_str());
    }
   } // if (line <= 23)
  } //for (i = 1; i <= 10 && line <= 23 && offset + i <= messages.size(); i++)
  if (offset > 0)
   mvwprintz(w, FULL_SCREEN_HEIGHT-1, 27, c_magenta, "^^^");
  if (offset + i < messages.size())
   mvwprintz(w, FULL_SCREEN_HEIGHT-1, 51, c_magenta, "vvv");
  wrefresh(w);

  DebugLog() << __FUNCTION__ << "calling get_input() \n";
  input = get_input();
  int dirx = 0, diry = 0;

  get_direction(dirx, diry, input);
  if (diry == -1 && offset > 0)
   offset--;
  if (diry == 1 && offset < messages.size())
   offset++;

 } while (input != Close && input != Cancel && input != Confirm);

 werase(w);
 delwin(w);
 refresh_all();
}

void game::teleport(player *p)
{
    if (p == NULL) {
        p = &u;
    }
    int newx, newy, tries = 0;
    bool is_u = (p == &u);

    p->add_disease("teleglow", 300);
    do {
        newx = p->posx + rng(0, SEEX * 2) - SEEX;
        newy = p->posy + rng(0, SEEY * 2) - SEEY;
        tries++;
    } while (tries < 15 && !is_empty(newx, newy));
    bool can_see = (is_u || u_see(newx, newy));
    if (p->in_vehicle) {
        m.unboard_vehicle (this, p->posx, p->posy);
    }
    p->posx = newx;
    p->posy = newy;
    if (tries == 15) {
        if (m.move_cost(newx, newy) == 0) { // TODO: If we land in water, swim
            if (can_see) {
                if (is_u) {
                    add_msg(_("You teleport into the middle of a %s!"),
                            m.name(newx, newy).c_str());
                } else {
                    add_msg(_("%s teleports into the middle of a %s!"),
                            p->name.c_str(), m.name(newx, newy).c_str());
                }
            }
            p->hurt(this, bp_torso, 0, 500);
        } else if (mon_at(newx, newy) != -1) {
            int i = mon_at(newx, newy);
            if (can_see) {
                if (is_u) {
                    add_msg(_("You teleport into the middle of a %s!"),
                            z[i].name().c_str());
                } else {
                    add_msg(_("%s teleports into the middle of a %s!"),
                            p->name.c_str(), z[i].name().c_str());
                }
                explode_mon(i);
            }
        }
    }
    if (is_u) {
        update_map(u.posx, u.posy);
    }
}

void game::nuke(int x, int y)
{
	// TODO: nukes hit above surface, not z = 0
    if (x < 0 || y < 0 || x >= OMAPX || y >= OMAPY)
        return;
    int mapx = x * 2, mapy = y * 2;
    map tmpmap(&traps);
    tmpmap.load(this, mapx, mapy, 0, false);
    for (int i = 0; i < SEEX * 2; i++)
    {
        for (int j = 0; j < SEEY * 2; j++)
        {
            if (!one_in(10))
                tmpmap.ter_set(i, j, t_rubble);
            if (one_in(3))
                tmpmap.add_field(NULL, i, j, fd_nuke_gas, 3);
            tmpmap.radiation(i, j) += rng(20, 80);
        }
    }
    tmpmap.save(cur_om, turn, mapx, mapy, 0);
    cur_om->ter(x, y, 0) = ot_crater;
    //Kill any npcs on that omap location.
    for(int i = 0; i < cur_om->npcs.size();i++)
        if(cur_om->npcs[i]->mapx/2== x && cur_om->npcs[i]->mapy/2 == y && cur_om->npcs[i]->omz == 0)
            cur_om->npcs[i]->marked_for_death = true;
}

std::vector<faction *> game::factions_at(int x, int y)
{
 std::vector<faction *> ret;
 for (int i = 0; i < factions.size(); i++) {
  if (factions[i].omx == cur_om->pos().x && factions[i].omy == cur_om->pos().y &&
      trig_dist(x, y, factions[i].mapx, factions[i].mapy) <= factions[i].size)
   ret.push_back(&(factions[i]));
 }
 return ret;
}

nc_color sev(int a)
{
 switch (a) {
  case 0: return c_cyan;
  case 1: return c_ltcyan;
  case 2: return c_ltblue;
  case 3: return c_blue;
  case 4: return c_ltgreen;
  case 5: return c_green;
  case 6: return c_yellow;
  case 7: return c_pink;
  case 8: return c_ltred;
  case 9: return c_red;
  case 10: return c_magenta;
  case 11: return c_brown;
  case 12: return c_cyan_red;
  case 13: return c_ltcyan_red;
  case 14: return c_ltblue_red;
  case 15: return c_blue_red;
  case 16: return c_ltgreen_red;
  case 17: return c_green_red;
  case 18: return c_yellow_red;
  case 19: return c_pink_red;
  case 20: return c_magenta_red;
  case 21: return c_brown_red;
 }
 return c_dkgray;
}

void game::display_scent()
{
 int div = 1 + query_int(_("Sensitivity"));
 draw_ter();
 for (int x = u.posx - getmaxx(w_terrain)/2; x <= u.posx + getmaxx(w_terrain)/2; x++) {
  for (int y = u.posy - getmaxy(w_terrain)/2; y <= u.posy + getmaxy(w_terrain)/2; y++) {
   int sn = scent(x, y) / (div * 2);
   mvwprintz(w_terrain, getmaxy(w_terrain)/2 + y - u.posy, getmaxx(w_terrain)/2 + x - u.posx, sev(sn/10), "%d",
             sn % 10);
  }
 }
 wrefresh(w_terrain);
 getch();
}

void game::init_autosave()
{
 moves_since_last_save = 0;
 item_exchanges_since_save = 0;
 last_save_timestamp = time(NULL);
}

/* Currently unused.
int game::autosave_timeout()
{
 if (!OPTIONS["AUTOSAVE"])
  return -1; // -1 means block instead of timeout

 const double upper_limit = 60 * 1000;
 const double lower_limit = 5 * 1000;
 const double range = upper_limit - lower_limit;

 // Items exchanged
 const double max_changes = 20.0;
 const double max_moves = 500.0;

 double move_multiplier = 0.0;
 double changes_multiplier = 0.0;

 if( moves_since_last_save < max_moves )
  move_multiplier = 1 - (moves_since_last_save / max_moves);

 if( item_exchanges_since_save < max_changes )
  changes_multiplier = 1 - (item_exchanges_since_save / max_changes);

 double ret = lower_limit + (range * move_multiplier * changes_multiplier);
 return ret;
}
*/

void game::quicksave(){
    if(!moves_since_last_save && !item_exchanges_since_save){return;}//Don't autosave if the player hasn't done anything since the last autosave/quicksave,
    add_msg(_("Saving game, this may take a while"));

    time_t now = time(NULL);    //timestamp for start of saving procedure

    //perform save
    save();
    save_factions_missions_npcs();
    save_artifacts();
    save_maps();
    save_uistate();
    //Now reset counters for autosaving, so we don't immediately autosave after a quicksave or autosave.
    moves_since_last_save = 0;
    item_exchanges_since_save = 0;
    last_save_timestamp = now;
}

void game::autosave(){
    //Don't autosave if the min-autosave interval has not passed since the last autosave/quicksave.
    if(time(NULL) < last_save_timestamp + (60 * OPTIONS["AUTOSAVE_MINUTES"])){return;}
    quicksave();    //Driving checks are handled by quicksave()
}

void intro()
{
 int maxx, maxy;
 getmaxyx(stdscr, maxy, maxx);
 const int minHeight = FULL_SCREEN_HEIGHT;
 const int minWidth = FULL_SCREEN_WIDTH;
 WINDOW* tmp = newwin(minHeight, minWidth, 0, 0);
 while (maxy < minHeight || maxx < minWidth) {
  werase(tmp);
  wprintw(tmp, _("\
Whoa. Whoa. Hey. This game requires a minimum terminal size of %dx%d. I'm\n\
sorry if your graphical terminal emulator went with the woefully-diminutive\n\
%dx%d as its default size, but that just won't work here.  Now stretch the\n\
window until you've got it at the right size (or bigger).\n"),
          minWidth, minHeight, maxx, maxy);
  wgetch(tmp);
  getmaxyx(stdscr, maxy, maxx);
 }
 werase(tmp);
 wrefresh(tmp);
 delwin(tmp);
 erase();
}

// (Press X (or Y)|Try) to Z
std::string game::press_x(action_id act)
{
    return press_x(act,_("Press "),"",_("Try"));
}
std::string game::press_x(action_id act, std::string key_bound, std::string key_unbound)
{
    return press_x(act,key_bound,"",key_unbound);
}
std::string game::press_x(action_id act, std::string key_bound_pre, std::string key_bound_suf, std::string key_unbound)
{
    std::vector<char> keys = keys_bound_to( action_id(act) );
    if (keys.empty()) {
        return key_unbound;
    } else {
        std::string keyed = key_bound_pre.append("");
        for (int j = 0; j < keys.size(); j++) {
            if (keys[j] == '\'' || keys[j] == '"'){
                if (j < keys.size() - 1) {
                    keyed += keys[j]; keyed += _(" or ");
                } else {
                    keyed += keys[j];
                }
            } else {
                if (j < keys.size() - 1) {
                    keyed += "'"; keyed += keys[j]; keyed += _("' or ");
                } else {
                    if (keys[j] == '_') {
                        keyed += _("'_' (underscore)");
                    } else {
                        keyed += "'"; keyed += keys[j]; keyed += "'";
                    }
                }
            }
        }
        return keyed.append(key_bound_suf.c_str());
    }
}
// ('Z'ing|zing) (\(X( or Y))\))
std::string game::press_x(action_id act, std::string act_desc)
{
    bool key_after=false;
    bool z_ing=false;
    char zing = tolower(act_desc.at(0));
    std::vector<char> keys = keys_bound_to( action_id(act) );
    if (keys.empty()) {
        return act_desc;
    } else {
        std::string keyed = ("");
        for (int j = 0; j < keys.size(); j++) {
            if (tolower(keys[j])==zing) {
                if (z_ing) {
                    keyed.replace(1,1,1,act_desc.at(0));
                    if (key_after) {
                        keyed += _(" or '");
                        keyed += (islower(act_desc.at(0)) ? toupper(act_desc.at(0))
                                                          : tolower(act_desc.at(0)));
                        keyed += "'";
                    } else {
                        keyed +=" ('";
                        keyed += (islower(act_desc.at(0)) ? toupper(act_desc.at(0))
                                                          : tolower(act_desc.at(0)));
                        keyed += "'";
                        key_after=true;
                    }
                } else {
                    std::string uhh="";
                    if (keys[j] == '\'' || keys[j] == '"'){
                        uhh+="("; uhh+=keys[j]; uhh+=")";
                    } else {
                        uhh+="'"; uhh+=keys[j]; uhh+="'";
                    }
                    if(act_desc.length()>1) {
                        uhh+=act_desc.substr(1);
                    }
                    if (keys[j] == '_') {
                        uhh += _(" (underscore)");
                    }
                    keyed.insert(0,uhh);
                    z_ing=true;
                }
            } else {
                if (key_after) {
                    if (keys[j] == '\'' || keys[j] == '"'){
                        keyed += _(" or "); keyed += keys[j];
                    } else if (keys[j] == '_') {
                        keyed += _("or '_' (underscore)");
                    } else {
                        keyed+=_(" or '"); keyed+=keys[j]; keyed+="'";
                    }
                } else {
                    if (keys[j] == '\'' || keys[j] == '"'){
                        keyed += " ("; keyed += keys[j];
                    } else if (keys[j] == '_') {
                        keyed += _(" ('_' (underscore)");
                    } else {
                        keyed += " ('"; keyed+=keys[j]; keyed+="'";
                    }
                   key_after=true;
                }
            }
        }
        if (!z_ing) {
            keyed.insert(0,act_desc);
        }
        if (key_after) {
            keyed+=")";
        }
        return keyed;
    }
}
