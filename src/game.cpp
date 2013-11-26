#include "game.h"
#include "rng.h"
#include "input.h"
#include "keypress.h"
#include "output.h"
#include "skill.h"
#include "line.h"
#include "computer.h"
#include "veh_interact.h"
#include "advanced_inv.h"
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
#include "json.h"
#include "artifact.h"
#include "overmapbuffer.h"
#include "trap.h"
#include "mapdata.h"
#include "catacharset.h"
#include "translations.h"
#include "init.h"
#include "help.h"
#include "action.h"
#include "monstergenerator.h"
#include "worldfactory.h"
#include <map>
#include <set>
#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>
#include <math.h>
#include <vector>

#ifdef _MSC_VER
#include "wdirent.h"
#include <direct.h>
#else
#include <unistd.h>
#include <dirent.h>
#endif
#include <sys/stat.h>
#include "debug.h"
#include "catalua.h"

#if (defined _WIN32 || defined __WIN32__)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tchar.h>
#endif

#ifdef _MSC_VER
// MSVC doesn't have c99-compatible "snprintf", so do what picojson does and use _snprintf_s instead
#define snprintf _snprintf_s
#endif

#define dbg(x) dout((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "
void intro();
nc_color sev(int a); // Right now, ONLY used for scent debugging....

//The one and only game instance
game *g;
extern worldfactory *world_generator;

uistatedata uistate;

// This is the main game set-up process.
game::game() :
 uquit(QUIT_NO),
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
 dangerous_proximity(5),
 run_mode(1),
 mostseen(0),
 gamemode(NULL),
 lookHeight(13)
{
    world_generator = new worldfactory();
    // do nothing, everything that was in here is moved to init_data() which is called immediately after g = new game; in main.cpp
    // The reason for this move is so that g is not uninitialized when it gets to installing the parts into vehicles.
}
void game::init_data()
{
 dout() << "Game initialized.";
 try {
 // Gee, it sure is init-y around here!
    init_data_structures(); // initialize cata data structures
    load_json_dir("data/json"); // load it, load it all!
    init_npctalk();
    init_artifacts();
    init_weather();
    init_fields();
    init_faction_data();
    init_morale();
    init_itypes();               // Set up item types                (SEE itypedef.cpp)
    item_controller->init_old(); //Item manager
    init_monitems();             // Set up the items monsters carry  (SEE monitemsdef.cpp)
    init_missions();             // Set up mission templates         (SEE missiondef.cpp)
    init_autosave();             // Set up autosave
    init_diseases();             // Set up disease lookup table
    init_savedata_translation_tables();
    inp_mngr.init();            // Load input config JSON

    MonsterGenerator::generator().finalize_mtypes();
    finalize_vehicles();
    finalize_recipes();

 #ifdef LUA
    init_lua();                 // Set up lua                       (SEE catalua.cpp)
 #endif

 } catch(std::string &error_message)
 {
     uquit = QUIT_ERROR;
     if(!error_message.empty())
        debugmsg(error_message.c_str());
     return;
 }
 load_keyboard_settings();
 moveCount = 0;

 gamemode = new special_game; // Nothing, basically.
}

game::~game()
{
 delete gamemode;
 itypes.clear();
 delwin(w_terrain);
 delwin(w_minimap);
 delwin(w_HP);
 delwin(w_messages);
 delwin(w_location);
 delwin(w_status);
 delwin(w_status2);

 delete world_generator;

 release_traps();
 release_data_structures();
}

// Fixed window sizes
#define MINIMAP_HEIGHT 7
#define MINIMAP_WIDTH 7

void game::init_ui(){
    // clear the screen
    clear();
    // set minimum FULL_SCREEN sizes
    FULL_SCREEN_WIDTH = 80;
    FULL_SCREEN_HEIGHT = 24;
    // print an intro screen, making sure the terminal is the correct size
    intro();

    int sidebarWidth = (OPTIONS["SIDEBAR_STYLE"] == "narrow") ? 45 : 55;

    #if (defined TILES || defined _WIN32 || defined __WIN32__)
        TERMX = sidebarWidth + ((int)OPTIONS["VIEWPORT_X"] * 2 + 1);
        TERMY = (int)OPTIONS["VIEWPORT_Y"] * 2 + 1;
        POSX = (OPTIONS["VIEWPORT_X"] > 60) ? 60 : OPTIONS["VIEWPORT_X"];
        POSY = (OPTIONS["VIEWPORT_Y"] > 60) ? 60 : OPTIONS["VIEWPORT_Y"];
        // TERMY is always odd, so make FULL_SCREEN_HEIGHT odd too
        FULL_SCREEN_HEIGHT = 25;

        // If we've chosen the narrow sidebar, we might need to make the
        // viewport wider to fill an 80-column window.
        while (TERMX < FULL_SCREEN_WIDTH) {
            TERMX += 2;
            POSX += 1;
        }

        VIEW_OFFSET_X = (OPTIONS["VIEWPORT_X"] > 60) ? (int)OPTIONS["VIEWPORT_X"]-60 : 0;
        VIEW_OFFSET_Y = (OPTIONS["VIEWPORT_Y"] > 60) ? (int)OPTIONS["VIEWPORT_Y"]-60 : 0;
        TERRAIN_WINDOW_WIDTH  = (POSX * 2) + 1;
        TERRAIN_WINDOW_HEIGHT = (POSY * 2) + 1;
    #else
        getmaxyx(stdscr, TERMY, TERMX);

        // try to make FULL_SCREEN_HEIGHT symmetric according to TERMY
        if (TERMY % 2) {
            FULL_SCREEN_HEIGHT = 25;
        } else {
            FULL_SCREEN_HEIGHT = 24;
        }

        // now that TERMX and TERMY are set,
        // check if sidebar style needs to be overridden
        sidebarWidth = use_narrow_sidebar() ? 45 : 55;

        TERRAIN_WINDOW_WIDTH = (TERMX - sidebarWidth > 121) ? 121 : TERMX - sidebarWidth;
        TERRAIN_WINDOW_HEIGHT = (TERMY > 121) ? 121 : TERMY;

        VIEW_OFFSET_X = (TERMX - sidebarWidth > 121) ? (TERMX - sidebarWidth - 121)/2 : 0;
        VIEW_OFFSET_Y = (TERMY > 121) ? (TERMY - 121)/2 : 0;

        POSX = TERRAIN_WINDOW_WIDTH / 2;
        POSY = TERRAIN_WINDOW_HEIGHT / 2;
    #endif

    // Set up the main UI windows.
    w_terrain = newwin(TERRAIN_WINDOW_HEIGHT, TERRAIN_WINDOW_WIDTH, VIEW_OFFSET_Y, VIEW_OFFSET_X);
    werase(w_terrain);

    int minimapX, minimapY; // always MINIMAP_WIDTH x MINIMAP_HEIGHT in size
    int hpX, hpY, hpW, hpH;
    int messX, messY, messW, messH;
    int locX, locY, locW, locH;
    int statX, statY, statW, statH;
    int stat2X, stat2Y, stat2W, stat2H;
    int mouseview_y, mouseview_h;

    if (use_narrow_sidebar()) {
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

        mouseview_y = messY + 7;
        mouseview_h = TERMY - mouseview_y - 5;
    } else {
        // standard sidebar style
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
        stat2X = 0;
        stat2Y = statY + statH;
        stat2H = 1;
        stat2W = sidebarWidth;

        mouseview_y = stat2Y + stat2H;
        mouseview_h = TERMY - mouseview_y;
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

    int mouse_view_x = _x + minimapX;
    int mouse_view_width = sidebarWidth;
    if (mouseview_h < lookHeight) {
        // Not enough room below the status bar, just use the regular lookaround area
        get_lookaround_dimensions(mouse_view_width, mouseview_y, mouse_view_x);
        mouseview_h = lookHeight;
        liveview.compact_view = true;
        if (!use_narrow_sidebar()) {
            // Second status window must now take care of clearing the area to the
            // bottom of the screen.
            stat2H = TERMY - stat2Y;
        }
    }
    liveview.init(this, mouse_view_x, mouseview_y, sidebarWidth, mouseview_h);

    w_status2 = newwin(stat2H, stat2W, _y + stat2Y, _x + stat2X);
    werase(w_status2);
}

/*
 * Initialize more stuff after mapbuffer is loaded.
 */
void game::setup()
{
 m = map(&traps); // Init the root map with our vectors
 _active_monsters.reserve(1000); // Reserve some space

// Even though we may already have 'd', nextinv will be incremented as needed
 nextinv = 'd';
 next_npc_id = 1;
 next_faction_id = 1;
 next_mission_id = 1;
// Clear monstair values
 monstairx = -1;
 monstairy = -1;
 monstairz = -1;
 last_target = -1;  // We haven't targeted any monsters yet
 curmes = 0;        // We haven't read any messages yet
 uquit = QUIT_NO;   // We haven't quit the game
 debugmon = false;  // We're not printing debug messages

 weather = WEATHER_CLEAR; // Start with some nice weather...
 // Weather shift in 30
 nextweather = HOURS((int)OPTIONS["INITIAL_TIME"]) + MINUTES(30);

 turnssincelastmon = 0; //Auto safe mode init
 autosafemode = OPTIONS["AUTOSAFEMODE"];

 footsteps.clear();
 footsteps_source.clear();
 clear_zombies();
 coming_to_stairs.clear();
 active_npc.clear();
 factions.clear();
 active_missions.clear();
 items_dragged.clear();
 messages.clear();
 events.clear();

 turn.set_season(SUMMER);    // ... with winter conveniently a long ways off   (not sure if we need this...)

 // reset kill counts
 kills.clear();
// Set the scent map to 0
 for (int i = 0; i < SEEX * MAPSIZE; i++) {
  for (int j = 0; j < SEEX * MAPSIZE; j++)
   grscent[i][j] = 0;
 }

 load_auto_pickup(false); // Load global auto pickup rules
 // back to menu for save loading, new game etc
 }

// Set up all default values for a new game
void game::start_game(std::string worldname)
{
 turn = HOURS(ACTIVE_WORLD_OPTIONS["INITIAL_TIME"]);
 if (ACTIVE_WORLD_OPTIONS["INITIAL_SEASON"].getValue() == "spring");
 else if (ACTIVE_WORLD_OPTIONS["INITIAL_SEASON"].getValue() == "summer")
    turn += DAYS( (int) ACTIVE_WORLD_OPTIONS["SEASON_LENGTH"]);
 else if (ACTIVE_WORLD_OPTIONS["INITIAL_SEASON"].getValue() == "autumn")
    turn += DAYS( (int) ACTIVE_WORLD_OPTIONS["SEASON_LENGTH"] * 2);
 else
    turn += DAYS( (int) ACTIVE_WORLD_OPTIONS["SEASON_LENGTH"] * 3);
 nextweather = turn + MINUTES(30);
 run_mode = (OPTIONS["SAFEMODE"] ? 1 : 0);
 mostseen = 0; // ...and mostseen is 0, we haven't seen any monsters yet.

 clear();
 refresh();
 popup_nowait(_("Please wait as we build your world"));
// Init some factions.
 if (!load_master(worldname)) // Master data record contains factions.
  create_factions();
 cur_om = &overmap_buffer.get(this, 0, 0); // We start in the (0,0,0) overmap.

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
 m.spawn_monsters(this); // Static monsters
 //Put some NPCs in there!
 create_starting_npcs();

 //Create mutation_category_level
 u.set_highest_cat_level();
 //Calc mutation drench protection stats
 u.drench_mut_calc();

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
    if(!ACTIVE_WORLD_OPTIONS["STATIC_NPC"]) {
        return; //Do not generate a starting npc.
    }
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
    if (uquit == QUIT_DIED || uquit == QUIT_SUICIDE || uquit == QUIT_SAVED) {
        // Save the factions's, missions and set the NPC's overmap coords
        // Npcs are saved in the overmap.
        save_factions_missions_npcs(); //missions need to be saved as they are global for all saves.

        // save artifacts.
        save_artifacts();

        // and the overmap, and the local map.
        save_maps(); //Omap also contains the npcs who need to be saved.
    }

    // Clear the future weather for future projects
    weather_log.clear();

    if (uquit == QUIT_DIED) {
        popup_top(_("Game over! Press spacebar..."));
    }
    if (uquit == QUIT_DIED || uquit == QUIT_SUICIDE) {
        death_screen();
        u.add_memorial_log("%s %s", u.name.c_str(),
                uquit == QUIT_SUICIDE ? _("committed suicide.") : _("was killed."));
        write_memorial_file();
        u.memorial_log.clear();
        std::vector<std::string> characters = list_active_characters();
        // remove current player from the active characters list, as they are dead
        std::vector<std::string>::iterator curchar = std::find(characters.begin(), characters.end(), u.name);
        if (curchar != characters.end()){
            characters.erase(curchar);
        }
        if (characters.empty()) {
            if (ACTIVE_WORLD_OPTIONS["DELETE_WORLD"] == "yes" ||
            (ACTIVE_WORLD_OPTIONS["DELETE_WORLD"] == "query" && query_yn(_("Delete saved world?")))) {
                if (gamemode->id() == SGAME_NULL) {
                    delete_world(world_generator->active_world->world_name, false);
                } else {
                    delete_world(world_generator->active_world->world_name, true);
                }
            }
        } else if (ACTIVE_WORLD_OPTIONS["DELETE_WORLD"] != "no") {
            std::stringstream message;
            message << _("World retained. Characters remaining:");
            for (int i = 0; i < characters.size(); ++i) {
                message << "\n  " << characters[i];
            }
            popup(message.str().c_str());
        }
        if (gamemode) {
            delete gamemode;
            gamemode = new special_game; // null gamemode or something..
        }
    }
    MAPBUFFER.reset();
    MAPBUFFER.make_volatile();
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
    } else if (u.has_disease("jetinjector") &&
            u.disease_duration("jetinjector") > 400) {
        add_msg(_("Your heart spasms painfully and stops."));
        u.add_memorial_log(_("Died of a healing stimulant overdose."));
        u.hp_cur[hp_torso] = 0;
    }
    // Check if we're starving or have starved
    if (u.hunger >= 3000){
        if (u.hunger >= 6000){
            add_msg(_("You have starved to death."));
            u.add_memorial_log(_("Died of starvation."));
            u.hp_cur[hp_torso] = 0;
        } else if (u.hunger >= 5000 && turn % 20 == 0){
            add_msg(_("Food..."));
        } else if (u.hunger >= 4000 && turn % 20 == 0){
            add_msg(_("You are STARVING!"));
        } else if (turn % 20 == 0){
            add_msg(_("Your stomach feels so empty..."));
        }
    }

    // Check if we're dying of thirst
    if (u.thirst >= 600){
        if (u.thirst >= 1200){
            add_msg(_("You have died of dehydration."));
            u.add_memorial_log(_("Died of thirst."));
            u.hp_cur[hp_torso] = 0;
        } else if (u.thirst >= 1000 && turn % 20 == 0){
            add_msg(_("Even your eyes feel dry..."));
        } else if (u.thirst >= 800 && turn % 20 == 0){
            add_msg(_("You are THIRSTY!"));
        } else if (turn % 20 == 0){
            add_msg(_("Your mouth feels so dry..."));
        }
    }

    // Check if we're falling asleep, unless we're sleeping
    if (u.fatigue >= 600 && !u.has_disease("sleep")){
        if (u.fatigue >= 1000){
            add_msg(_("Survivor sleep now."));
            u.add_memorial_log(_("Succumbed to lack of sleep."));
            u.fatigue -= 10;
            u.try_to_sleep(this);
        } else if (u.fatigue >= 800 && turn % 10 == 0){
            add_msg(_("Anywhere would be a good place to sleep..."));
        } else if (turn % 50 == 0) {
            add_msg(_("You feel like you haven't slept in days."));
        }
    }

    if (turn % 50 == 0) { // Hunger, thirst, & fatigue up every 5 minutes
        if ((!u.has_trait("LIGHTEATER") || !one_in(3)) &&
            (!u.has_bionic("bio_recycler") || turn % 300 == 0)) {
            u.hunger++;
        }
        if ((!u.has_bionic("bio_recycler") || turn % 100 == 0) &&
            (!u.has_trait("PLANTSKIN") || !one_in(5))) {
            u.thirst++;
        }
        // Don't increase fatigue if sleeping or trying to sleep or if we're at the cap.
        if (u.fatigue < 1050 && !(u.has_disease("sleep") || u.has_disease("lying_down"))) {
            u.fatigue++;
        }
        if (u.fatigue == 192 && !u.has_disease("lying_down") && !u.has_disease("sleep")) {
            if (u.activity.type == ACT_NULL) {
                add_msg(_("You're feeling tired.  %s to lie down for sleep."),
                        press_x(ACTION_SLEEP).c_str());
            } else {
                cancel_activity_query(_("You're feeling tired."));
            }
        }
        if (u.stim < 0) {
            u.stim++;
        }
        if (u.stim > 0) {
            u.stim--;
        }
        if (u.pkill > 0) {
            u.pkill--;
        }
        if (u.pkill < 0) {
            u.pkill++;
        }
        if (u.has_bionic("bio_solar") && is_in_sunlight(u.posx, u.posy)) {
            u.charge_power(1);
        }
    }

    if (turn % 300 == 0) { // Pain up/down every 30 minutes
        if (u.pain > 0) {
            u.pain -= 1 + int(u.pain / 10);
        } else if (u.pain < 0) {
            u.pain++;
        }
        // Mutation healing effects
        if (u.has_trait("FASTHEALER2") && one_in(5)) {
            u.healall(1);
        }
        if (u.has_trait("REGEN") && one_in(2)) {
            u.healall(1);
        }
        if (u.has_trait("ROT2") && one_in(5)) {
            u.hurtall(1);
        }
        if (u.has_trait("ROT3") && one_in(2)) {
            u.hurtall(1);
        }

        if (u.radiation > 1 && one_in(3)) {
            u.radiation--;
        }
        u.get_sick( this );
    }

    // Auto-save if autosave is enabled
    if (OPTIONS["AUTOSAVE"] &&
        turn % ((int)OPTIONS["AUTOSAVE_TURNS"] * 10) == 0) {
        autosave();
    }

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
            if (!u.has_disease("sleep") && u.activity.type == ACT_NULL) {
                draw();
            }

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

    if (levz >= 0 && !u.is_underwater()) {
        weather_effect weffect;
        (weffect.*(weather_data[weather].effect))(this);
    }

    if (u.has_disease("sleep") && int(turn) % 300 == 0) {
        draw();
        refresh();
    }

    u.update_bodytemp(this);

    rustCheck();
    if (turn % 10 == 0) {
        u.update_morale();
    }
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
 item* book_item;
 item* reloadable;
 bool no_recipes;
 if (u.activity.type != ACT_NULL) {
  if (int(turn) % 150 == 0) {
   draw();
  }
  if (u.activity.type == ACT_WAIT || u.activity.type == ACT_WAIT_WEATHER) { // Based on time, not speed
   u.activity.moves_left -= 100;
   u.pause(this);
  } else if (u.activity.type == ACT_GAME) {

    //Gaming takes time, not speed
    u.activity.moves_left -= 100;

    if (u.activity.type == ACT_GAME) {
      item& game_item = u.weapon.invlet == u.activity.invlet ?
                            u.weapon : u.inv.item_by_letter(u.activity.invlet);

      //Deduct 1 battery charge for every minute spent playing
      if(int(turn) % 10 == 0) {
        game_item.charges--;
        u.add_morale(MORALE_GAME, 1, 100); //1 points/min, almost 2 hours to fill
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

  if (u.activity.moves_left <= 0) { // We finished our activity!

   switch (u.activity.type) {

   case ACT_RELOAD:
    if (u.activity.name[0] == u.weapon.invlet) {
        reloadable = &u.weapon;
    } else {
        reloadable = &u.inv.item_by_letter(u.activity.name[0]);
    }
    if (reloadable->reload(u, u.activity.invlet))
     if (reloadable->is_gun() && reloadable->has_flag("RELOAD_ONE")) {
      add_msg(_("You insert a cartridge into your %s."),
              reloadable->tname(this).c_str());
      if (u.recoil < 8)
       u.recoil = 8;
      if (u.recoil > 8)
       u.recoil = (8 + u.recoil) / 2;
     } else {
      add_msg(_("You reload your %s."), reloadable->tname(this).c_str());
      u.recoil = 6;
     }
    else
     add_msg(_("Can't reload your %s."), reloadable->tname(this).c_str());
    break;

   case ACT_READ:
    book_item = &(u.weapon.invlet == u.activity.invlet ?
                            u.weapon : u.inv.item_by_letter(u.activity.invlet));
    reading = dynamic_cast<it_book*>(book_item->type);

    if (reading->fun != 0) {
        int fun_bonus;
        if(book_item->charges == 0) {
            //Book is out of chapters -> re-reading old book, less fun
            add_msg(_("The %s isn't as much fun now that you've finished it."),
                    book_item->name.c_str());
            if(one_in(6)) { //Don't nag incessantly, just once in a while
                add_msg(_("Maybe you should find something new to read..."));
            }
            //50% penalty
            fun_bonus = (reading->fun * 5) / 2;
        } else {
            fun_bonus = reading->fun * 5;
        }
        u.add_morale(MORALE_BOOK, fun_bonus,
                     reading->fun * 15, 60, 30, true, reading);
    }

    if(book_item->charges > 0) {
        book_item->charges--;
    }

    no_recipes = true;
    if (!reading->recipes.empty())
    {
        bool recipe_learned = u.try_study_recipe(this, reading);
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

     if (u.skillLevel(reading->type) == originalSkillLevel && u.activity.continuous) {
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
   case ACT_WAIT_WEATHER:
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
    {
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

   case ACT_FIRSTAID:
    {
      item it = u.inv.item_by_letter(u.activity.invlet);
      iuse tmp;
      tmp.completefirstaid(&u, &it, false);
      u.inv.remove_item_by_charges(u.activity.invlet, 1);
      // Erase activity and values.
      u.activity.type = ACT_NULL;
      u.activity.values.clear();
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

    std::string stop_phrase[NUM_ACTIVITIES] = {
        _(" Stop?"), _(" Stop reloading?"),
        _(" Stop reading?"), _(" Stop playing?"),
        _(" Stop waiting?"), _(" Stop crafting?"),
        _(" Stop crafting?"), _(" Stop disassembly?"),
        _(" Stop butchering?"), _(" Stop foraging?"),
        _(" Stop construction?"), _(" Stop construction?"),
        _(" Stop pumping gas?"), _(" Stop training?"),
        _(" Stop waiting?"), _(" Stop using first aid?")
    };

    std::string stop_message = s + stop_phrase[u.activity.type] +
            _(" (Y)es, (N)o, (I)gnore further distractions and finish.");

    do {
        ch = popup_getkey(stop_message.c_str());
    } while (ch != '\n' && ch != ' ' && ch != KEY_ESCAPE &&
             ch != 'Y' && ch != 'N' && ch != 'I' &&
             (force_uc || (ch != 'y' && ch != 'n' && ch != 'i')));

  if (ch == 'Y' || ch == 'y') {
    u.cancel_activity();
  } else if (ch == 'I' || ch == 'i' ) {
    return true;
  }
  return false;
}

bool game::cancel_activity_query(const char* message, ...)
{
 char buff[1024];
 va_list ap;
 va_start(ap, message);
 vsprintf(buff, message, ap);
 va_end(ap);
 std::string s(buff);

 bool doit = false;;

    std::string stop_phrase[NUM_ACTIVITIES] = {
        _(" Stop?"), _(" Stop reloading?"),
        _(" Stop reading?"), _(" Stop playing?"),
        _(" Stop waiting?"), _(" Stop crafting?"),
        _(" Stop crafting?"), _(" Stop disassembly?"),
        _(" Stop butchering?"), _(" Stop foraging?"),
        _(" Stop construction?"), _(" Stop construction?"),
        _(" Stop pumping gas?"), _(" Stop training?"),
        _(" Stop waiting?"), _(" Stop using first aid?")
    };

    std::string stop_message = s + stop_phrase[u.activity.type];

    if (ACT_NULL == u.activity.type) {
        if (u.has_destination()) {
            add_msg(_("You were hurt. Auto-move cancelled"));
            u.clear_destination();
        }
        doit = false;
    } else if (query_yn(stop_message.c_str())) {
        doit = true;
    }

 if (doit)
  u.cancel_activity();

 return doit;
}

void game::update_weather()
{
    season_type season;
    // Default to current weather, and update to the furthest future weather if any.
    weather_segment prev_weather = {temperature, weather, nextweather};
    if ( !weather_log.empty() ) {
        prev_weather = weather_log.rbegin()->second;
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
        weather_log[ (int)new_weather.deadline ] = new_weather;
    }

    if( turn >= nextweather )
    {
        weather_type old_weather = weather;
        weather_segment  new_weather = weather_log.lower_bound((int)nextweather)->second;
        weather = new_weather.weather;
        temperature = new_weather.temperature;
        nextweather = weather_log.upper_bound(int(new_weather.deadline))->second.deadline;

        if (weather != old_weather && weather_data[weather].dangerous &&
            levz >= 0 && m.is_outside(u.posx, u.posy))
        {
            cancel_activity_query(_("The weather changed to %s!"), weather_data[weather].name.c_str());
        }

        if (weather != old_weather && u.has_activity(this, ACT_WAIT_WEATHER)) {
            u.assign_activity(this, ACT_WAIT_WEATHER, 0, 0);
        }
    }
}

int game::get_temperature()
{
    point location = om_location();
    int tmp_temperature = temperature;

    tmp_temperature += m.temperature(u.posx, u.posy);

    return tmp_temperature;
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

int game::kill_count(std::string mon){
    if (kills.find(mon) != kills.end()){
        return kills[mon];
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
   for (int i = 0; i < num_zombies(); i++) {
    if (zombie(i).mission_id == miss->uid)
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
      !active_missions[i].failed &&
      int(turn) > active_missions[i].deadline)
   fail_mission(active_missions[i].uid);
 }
}

void game::handle_key_blocking_activity() {
    // If player is performing a task and a monster is dangerously close, warn them
    // regardless of previous safemode warnings
    if (is_hostile_very_close() &&
        u.activity.type != ACT_NULL &&
        u.activity.moves_left > 0 &&
        !u.activity.warned_of_proximity)
    {
        u.activity.warned_of_proximity = true;
        if (cancel_activity_query(_("Monster dangerously close!"))) {
            return;
        }
    }

    if (u.activity.moves_left > 0 && u.activity.continuous == true &&
        (  // bool activity_is_abortable() ?
            u.activity.type == ACT_READ ||
            u.activity.type == ACT_BUILD ||
            u.activity.type == ACT_LONGCRAFT ||
            u.activity.type == ACT_REFILL_VEHICLE ||
            u.activity.type == ACT_WAIT ||
            u.activity.type == ACT_WAIT_WEATHER ||
            u.activity.type == ACT_FIRSTAID
        )
    ) {
        timeout(1);
        char ch = input();
        if(ch != ERR) {
            timeout(-1);
            switch(action_from_key(ch)) {  // should probably make the switch in handle_action() a function
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
                    display_help();
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

// Checks input to see if mouse was moved and handles the mouse view box accordingly.
// Returns true if input requires breaking out into a game action.
bool game::handle_mouseview(input_context &ctxt, std::string &action)
{
    do {
        action = ctxt.handle_input();
        if (action == "MOUSE_MOVE") {
            int mx, my;
            if (!ctxt.get_coordinates(w_terrain, mx, my)) {
                hide_mouseview();
            } else {
                liveview.show(mx, my);
            }
        }
    } while (action == "MOUSE_MOVE"); // Freeze animation when moving the mouse

    if (action != "TIMEOUT" && ctxt.get_raw_input().get_first_input() != ERR) {
        // Keyboard event, break out of animation loop
        hide_mouseview();
        return false;
    }

    // Mouse movement or un-handled key
    return true;
}

// Hides the mouse hover box and redraws what was under it
void game::hide_mouseview()
{
    if (liveview.hide()) {
        write_msg(); // Redraw anything hidden by mouseview
    }
}

input_context game::get_player_input(std::string &action)
{
    input_context ctxt("DEFAULTMODE");
    ctxt.register_directions();
    ctxt.register_action("ANY_INPUT");
    ctxt.register_action("COORDINATE");
    ctxt.register_action("MOUSE_MOVE");
    ctxt.register_action("SELECT");
    ctxt.register_action("SEC_SELECT");

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
        int dropCount = int(iEndX * iEndY * fFactor);
        //std::vector<std::pair<int, int> > vDrops;

        weather_printable wPrint;
        wPrint.colGlyph = colGlyph;
        wPrint.cGlyph = cGlyph;
        wPrint.wtype = weather;
        wPrint.vdrops.clear();
        wPrint.startx = iStartX;
        wPrint.starty = iStartY;
        wPrint.endx = iEndX;
        wPrint.endy = iEndY;

        /*
        Location to add rain drop animation bits! Since it refreshes w_terrain it can be added to the animation section easily
        Get tile information from above's weather information:
            WEATHER_ACID_DRIZZLE | WEATHER_ACID_RAIN = "weather_acid_drop"
            WEATHER_DRIZZLE | WEATHER_RAINY | WEATHER_THUNDER | WEATHER_LIGHTNING = "weather_rain_drop"
            WEATHER_SNOW | WEATHER_SNOWSTORM = "weather_snowflake"
        */
        int offset_x = (u.posx + u.view_offset_x) - getmaxx(w_terrain)/2;
        int offset_y = (u.posy + u.view_offset_y) - getmaxy(w_terrain)/2;

        do {
            for(int i=0; i < wPrint.vdrops.size(); i++) {
                m.drawsq(w_terrain, u,
                         //vDrops[i].first - getmaxx(w_terrain)/2 + u.posx + u.view_offset_x,
                         wPrint.vdrops[i].first + offset_x,
                         //vDrops[i].second - getmaxy(w_terrain)/2 + u.posy + u.view_offset_y,
                         wPrint.vdrops[i].second + offset_y,
                         false,
                         true,
                         u.posx + u.view_offset_x,
                         u.posy + u.view_offset_y);
            }

            //vDrops.clear();
            wPrint.vdrops.clear();

            for(int i=0; i < dropCount; i++) {
                int iRandX = rng(iStartX, iEndX-1);
                int iRandY = rng(iStartY, iEndY-1);

                if (mapRain[iRandY][iRandX]) {
                    //vDrops.push_back(std::make_pair(iRandX, iRandY));
                    wPrint.vdrops.push_back(std::make_pair(iRandX, iRandY));

                    //mvwputch(w_terrain, iRandY, iRandX, colGlyph, cGlyph);
                }
            }
            draw_weather(wPrint);

            wrefresh(w_terrain);
            inp_mngr.set_timeout(125);
        } while (handle_mouseview(ctxt, action));
        inp_mngr.set_timeout(-1);

    } else {
        while (handle_mouseview(ctxt, action));
    }

    return ctxt;
}

bool game::handle_action()
{
    std::string action;
    input_context ctxt;

    action_id act = ACTION_NULL;
    // Check if we have an auto-move destination
    if (u.has_destination()) {
        act = u.get_next_auto_move_direction();
        if (act == ACTION_NULL) {
            add_msg(_("Auto-move cancelled"));
            u.clear_destination();
            return false;
        }
    } else {
        // No auto-move, ask player for input
        ctxt = get_player_input(action);
    }

    int veh_part;
    vehicle *veh = m.veh_at(u.posx, u.posy, veh_part);
    bool veh_ctrl = veh && veh->player_in_control (&u);

    // If performing an action with right mouse button, co-ordinates
    // of location clicked.
    int mouse_action_x = -1, mouse_action_y = -1;

    if (act == ACTION_NULL) {
        if (action == "SELECT" || action == "SEC_SELECT") {
            // Mouse button click
            if (veh_ctrl) {
                // No mouse use in vehicle
                return false;
            }

            int mx, my;
            if (!ctxt.get_coordinates(w_terrain, mx, my) || !u_see(mx, my)) {
                // Not clicked in visible terrain
                return false;
            }

            if (action == "SELECT") {
                bool new_destination = true;
                if (destination_preview.size() > 0) {
                    point final_destination = destination_preview.back();
                    if (final_destination.x == mx && final_destination.y == my) {
                        // Second click
                        new_destination = false;
                        u.set_destination(destination_preview);
                        destination_preview.clear();
                        act = u.get_next_auto_move_direction();
                        if (act == ACTION_NULL) {
                            // Something went wrong
                            u.clear_destination();
                            return false;
                        }
                    }
                }

                if (new_destination) {
                    destination_preview = m.route(u.posx, u.posy, mx, my, false);
                    return false;
                }
            } else {
                // Right mouse button

                bool had_destination_to_clear = destination_preview.size() > 0;
                u.clear_destination();
                destination_preview.clear();

                if (had_destination_to_clear) {
                    return false;
                }

                mouse_action_x = mx;
                mouse_action_y = my;
                int mouse_selected_mondex = mon_at(mx, my);
                if (mouse_selected_mondex != -1) {
                    monster &z = _active_monsters[mouse_selected_mondex];
                    if (!u_see(&z)) {
                        add_msg(_("Nothing relevant here."));
                        return false;
                    }

                    if (!u.weapon.is_gun()) {
                        add_msg(_("You are not wielding a ranged weapon."));
                        return false;
                    }

                    //TODO: Add weapon range check. This requires weapon to be reloaded.

                    act = ACTION_FIRE;
                } else if (m.close_door(mx, my, !m.is_outside(mx, my), true)) {
                    act = ACTION_CLOSE;
                } else {
                    int dx = abs(u.posx - mx);
                    int dy = abs(u.posy - my);
                    if (dx < 2 && dy < 2) {
                        if (dy == 0 && dx == 0) {
                            // Clicked on self
                            act = ACTION_PICKUP;
                        } else {
                            // Clicked adjacent tile
                            act = ACTION_EXAMINE;
                        }
                    } else {
                        add_msg(_("Nothing relevant here."));
                        return false;
                    }
                }
            }
        }
    }

    if (act == ACTION_NULL) {
        // No auto-move action, no mouse clicks.
        u.clear_destination();
        destination_preview.clear();

        int ch = ctxt.get_raw_input().get_first_input();
        // Hack until new input system is fully implemented
        if (ch == KEY_UP) {
            act = ACTION_MOVE_N;
        } else if (ch == KEY_RIGHT) {
            act = ACTION_MOVE_E;
        } else if (ch == KEY_DOWN) {
            act = ACTION_MOVE_S;
        } else if (ch == KEY_LEFT) {
            act = ACTION_MOVE_W;
        } else if (ch == KEY_NPAGE) {
            act = ACTION_MOVE_DOWN;
        } else if (ch == KEY_PPAGE) {
            act = ACTION_MOVE_UP;
        } else {
            if (keymap.find(ch) == keymap.end()) {
                if (ch != ' ' && ch != '\n') {
                    add_msg(_("Unknown command: '%c'"), ch);
                }
                return false;
            }

            act = keymap[ch];
        }
    }

// This has no action unless we're in a special game mode.
 gamemode->pre_action(this, act);

 int soffset = (int)OPTIONS["MOVE_VIEW_OFFSET"];
 int soffsetr = 0 - soffset;

 int before_action_moves = u.moves;

 // Use to track if auto-move should be cancelled due to a failed
 // move or obstacle
 bool continue_auto_move = false;

 switch (act) {

  case ACTION_PAUSE:
   if (run_mode == 2 && (u.controlling_vehicle && safemodeveh) ) { // Monsters around and we don't wanna pause
     add_msg(_("Monster spotted--safe mode is on! (%s to turn it off.)"),
             press_x(ACTION_TOGGLE_SAFEMODE).c_str());}
   else
   if (u.has_trait("WEB_WEAVER") && !u.in_vehicle) {
      g->m.add_field(g, u.posx, u.posy, fd_web, 1); //this adds density to if its not already there.
      add_msg("You spin some webbing.");}
    u.pause(this);
   break;

  case ACTION_MOVE_N:
   moveCount++;

   if (veh_ctrl)
    pldrive(0, -1);
   else
    continue_auto_move = plmove(0, -1);
   break;

  case ACTION_MOVE_NE:
   moveCount++;

   if (veh_ctrl)
    pldrive(1, -1);
   else
    continue_auto_move = plmove(1, -1);
   break;

  case ACTION_MOVE_E:
   moveCount++;

   if (veh_ctrl)
    pldrive(1, 0);
   else
    continue_auto_move = plmove(1, 0);
   break;

  case ACTION_MOVE_SE:
   moveCount++;

   if (veh_ctrl)
    pldrive(1, 1);
   else
    continue_auto_move = plmove(1, 1);
   break;

  case ACTION_MOVE_S:
   moveCount++;

   if (veh_ctrl)
    pldrive(0, 1);
   else
   continue_auto_move = plmove(0, 1);
   break;

  case ACTION_MOVE_SW:
   moveCount++;

   if (veh_ctrl)
    pldrive(-1, 1);
   else
    continue_auto_move = plmove(-1, 1);
   break;

  case ACTION_MOVE_W:
   moveCount++;

   if (veh_ctrl)
    pldrive(-1, 0);
   else
    continue_auto_move = plmove(-1, 0);
   break;

  case ACTION_MOVE_NW:
   moveCount++;

   if (veh_ctrl)
    pldrive(-1, -1);
   else
    continue_auto_move = plmove(-1, -1);
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
   close(mouse_action_x, mouse_action_y);
   break;

  case ACTION_SMASH:
   if (veh_ctrl)
    handbrake();
   else
    smash();
   break;

  case ACTION_EXAMINE:
   examine(mouse_action_x, mouse_action_y);
   break;

  case ACTION_ADVANCEDINV:
   advanced_inv();
   break;

  case ACTION_PICKUP:
   pickup(u.posx, u.posy, 1);
   break;

  case ACTION_GRAB:
   grab();
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

  case ACTION_LIST_ITEMS: {
    int iRetItems = -1;
    int iRetMonsters = -1;
    int startas = uistate.list_item_mon;
    do {
        if ( startas != 2 ) { // last mode 2 = list_monster
            startas = 0;      // but only for the first bit of the loop
        iRetItems = list_items();
        } else {
            iRetItems = -2;   // so we'll try list_items if list_monsters found 0
        }
        if (iRetItems != -1 || startas == 2 ) {
            startas = 0;
            iRetMonsters = list_monsters();
            if ( iRetMonsters == 2 ) {
                iRetItems = -1; // will fire, exit loop
            } else if ( iRetMonsters == -1 && iRetItems == -2 ) {
                iRetItems = -1; // exit if requested on list_monsters firstrun
        }
        }
    } while (iRetItems != -1 && iRetMonsters != -1 && !(iRetItems == 0 && iRetMonsters == 0));

    if (iRetItems == 0 && iRetMonsters == 0) {
        add_msg(_("You dont see any items or monsters around you!"));
    } else if ( iRetMonsters == 2 ) {
        refresh_all();
        plfire(false);
    }
  } break;


  case ACTION_INVENTORY: {
   int cMenu = ' ';
   do {
     char chItem = inv(_("Inventory:"));
     cMenu=inventory_item_menu(chItem);
   } while (cMenu == ' ' || cMenu == '.' || cMenu == 'q' || cMenu == '\n' ||
            cMenu == KEY_ESCAPE || cMenu == KEY_LEFT || cMenu == '=' );
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
   plfire(false, mouse_action_x, mouse_action_y);
   break;

  case ACTION_FIRE_BURST:
   plfire(true, mouse_action_x, mouse_action_y);
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
    for(int i=0; i < new_seen_mon.size(); i++) {
        monster &z = _active_monsters[new_seen_mon[i]];
        z.ignoring = rl_dist( point(u.posx, u.posy), z.pos() );
    }
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
        if (query_yn(_("REALLY commit suicide?"))) {
            u.moves = 0;
            place_corpse();
            uquit = QUIT_SUICIDE;
        }
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
   display_help();
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

 if (!continue_auto_move) {
     u.clear_destination();
 }

 gamemode->post_action(this, act);

 u.movecounter = before_action_moves - u.moves;
 dbg(D_INFO) << string_format("%s: [%d] %d - %d = %d",action_ident(act).c_str(),int(turn),before_action_moves,u.movecounter,u.moves);
 return true;
}

#define SCENT_RADIUS 40

int& game::scent(int x, int y)
{
  if (x < (SEEX * MAPSIZE / 2) - SCENT_RADIUS || x >= (SEEX * MAPSIZE / 2) + SCENT_RADIUS ||
      y < (SEEY * MAPSIZE / 2) - SCENT_RADIUS || y >= (SEEY * MAPSIZE / 2) + SCENT_RADIUS) {
  nulscent = 0;
  return nulscent; // Out-of-bounds - null scent
 }
 return grscent[x][y];
}

void game::update_scent()
{
  static point player_last_position = point( u.posx, u.posy );
  static calendar player_last_moved = turn;
  // Stop updating scent after X turns of the player not moving.
  // Once wind is added, need to reset this on wind shifts as well.
  if( u.posx == player_last_position.x && u.posy == player_last_position.y ) {
    if( player_last_moved + 1000 < turn )
      return;
  } else {
    player_last_position = point( u.posx, u.posy );
    player_last_moved = turn;
  }

 // Requires at [2*SCENT_RADIUS+3][2*SCENT_RADIUS+1] in size to hold enough data.
 int sum_3_squares_y[SEEX * MAPSIZE][SEEY * MAPSIZE]; //intermediate variable
 int  squares_used_y[SEEX * MAPSIZE][SEEY * MAPSIZE]; //intermediate variable
 const float diffusivity = 1.0;   // Determines how readily scents will diffuse.
 const float adding = 1.1;     // How much scent to add for very low scent values in order to keep them longer.
 const float lossiness = 0.95;   // How much scent to erase when we gain more.

 // Determine how many neighbours to diffuse into and the scent values of our neighbours.
  for (int x = u.posx - SCENT_RADIUS -1; x <= u.posx + SCENT_RADIUS + 1; x++) {
    for (int y = u.posy - SCENT_RADIUS; y <= u.posy + SCENT_RADIUS; y++) {
      sum_3_squares_y[x][y] = grscent[x][y] + grscent[x][y-1] + grscent[x][y+1];
      squares_used_y[x][y] =(m.move_cost_ter_furn(x,y-1) > 0   || m.has_flag("BASHABLE",x,y-1)) +
                            (m.move_cost_ter_furn(x,y)   > 0   || m.has_flag("BASHABLE",x,y))   +
                            (m.move_cost_ter_furn(x,y+1) > 0   || m.has_flag("BASHABLE",x,y+1)) ;
    }
  }

  int squares_lost;   // How many squares we will diffuse into.
  int squares_gained; // The sum of the squares giving us stuff.
  int cur_scent;
  int lost;           // How much to lose based on diffusion.
  int gained;         // How much we gained.
  int fslime;
  int new_scent;

  int move_cost;
  bool is_bashable;
  for (int x = u.posx - SCENT_RADIUS; x <= u.posx + SCENT_RADIUS; x++) {
    for (int y = u.posy - SCENT_RADIUS; y <= u.posy + SCENT_RADIUS; y++) {
      move_cost = m.move_cost_ter_furn(x, y);
      is_bashable = m.has_flag("BASHABLE", x, y);

      if (move_cost != 0 || is_bashable) {
        squares_lost = squares_used_y[x-1][y] + squares_used_y[x][y] + squares_used_y[x+1][y];
        squares_gained = sum_3_squares_y[x-1][y] + sum_3_squares_y[x][y] + sum_3_squares_y[x+1][y];

        cur_scent = grscent[x][y];
        lost = (diffusivity * squares_lost * cur_scent) / 8;

        gained = (diffusivity * squares_gained) / 8;
        gained *= lossiness;

        new_scent = grscent[x][y] + gained - lost;
        if (new_scent < 10 && one_in(6)) {
          new_scent *= adding;
        }
        grscent[x][y] = new_scent;

        // Add scent back to the player.
        if (x == u.posx && y == u.posy) {
          u.scent += (grscent[x][y] * lossiness) / 9;
        }

        if (grscent[x][y] < 1) {
          grscent[x][y] = 0;
        }
        else if (grscent[x][y] == 1 && one_in(9)) {
          grscent[x][y] = 0;
        }

        fslime = m.get_field_strength(point(x,y), fd_slime) * 10;
        if (fslime > 0 && grscent[x][y] < fslime) {
          grscent[x][y] = fslime;
        }
        if (grscent[x][y] > 100000) {
          dbg(D_ERROR) << "game:update_scent: Wacky scent at " << x << ","
                       << y << " (" << grscent[x][y] << ")";
          debugmsg("Wacky scent at %d, %d (%d)", x, y, grscent[x][y]);
          grscent[x][y] = 0; // Scent should never be higher
        }
        if (u.scent > 100000) {
          dbg(D_ERROR) << "game:update_scent: Wacky player scent : " << u.scent;
          debugmsg("Wacky player scent : %d", u.scent);
          u.scent = 0; // Scent should never be higher
        }
        //Greatly reduce scent for bashable barriers, even more for ductaped barriers
        if( move_cost == 0 && is_bashable) {
          if( m.has_flag("REDUCE_SCENT", x, y))
            grscent[x][y] /= 9;
          else
            grscent[x][y] /= 3;
        }
      }
    }
  }

  // Finally, diffuse the player's scent.
  grscent[u.posx][u.posy] += u.scent * lossiness;
  u.scent *= lossiness;
}

bool game::is_game_over()
{
    if (uquit == QUIT_SUICIDE){
        if (u.in_vehicle)
            g->m.unboard_vehicle(u.posx, u.posy);
        std::stringstream playerfile;
        playerfile << world_generator->active_world->world_path << "/" << base64_encode(u.name) << ".sav";
        DebugLog() << "Unlinking player file: <"<< playerfile.str() << "> -- ";
        bool ok = (unlink(playerfile.str().c_str()) == 0);
        DebugLog() << (ok?"SUCCESS":"FAIL") << "\n";
        return true;
    }
    if (uquit != QUIT_NO){
        return true;
    }
    for (int i = 0; i <= hp_torso; i++){
        if (u.hp_cur[i] < 1) {
            if (u.in_vehicle)
                g->m.unboard_vehicle(u.posx, u.posy);
            place_corpse();
            std::stringstream playerfile;
            playerfile << world_generator->active_world->world_path << "/" << base64_encode(u.name) << ".sav";
            DebugLog() << "Unlinking player file: <"<< playerfile.str() << "> -- ";
            bool ok = (unlink(playerfile.str().c_str()) == 0);
            DebugLog() << (ok?"SUCCESS":"FAIL") << "\n";
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
  your_body.make_corpse(itypes["corpse"], GetMType("mon_null"), turn);
  your_body.name = u.name;
  for (int i = 0; i < tmp.size(); i++)
    m.add_item_or_charges(u.posx, u.posy, *(tmp[i]));
  for (int i = 0; i < u.num_bionics(); i++) {
    bionic &b = u.bionic_at_index(i);
    if (itypes.find(b.id) != itypes.end()) {
      your_body.contents.push_back(item(itypes[b.id], turn));
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
        int ret;
        ret = chdir("..");
        if (ret != 0) {
            debugmsg("game::death_screen: Can\'t chdir(\"..\") from \"save\" directory");
        }
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


bool game::load_master(std::string worldname)
{
 std::ifstream fin;
 std::string data;
 std::stringstream datafile;
 datafile << world_generator->all_worlds[worldname]->world_path << "/master.gsav";
 fin.open(datafile.str().c_str(), std::ifstream::in | std::ifstream::binary);
 if (!fin.is_open())
  return false;

unserialize_master(fin);
 fin.close();
 return true;
}

void game::load_uistate(std::string worldname) {
    std::stringstream savefile;
    savefile << world_generator->all_worlds[worldname]->world_path << "/uistate.json";

    std::ifstream fin;
    fin.open(savefile.str().c_str(), std::ifstream::in | std::ifstream::binary);
    if(!fin.good()) {
        fin.close();
        return;
    }
    try {
        JsonIn jsin(&fin);
        uistate.deserialize(jsin);
    } catch (std::string e) {
        dbg(D_ERROR) << "load_uistate: " << e;
    }
    fin.close();
    }

void game::load(std::string worldname, std::string name)
{
 std::ifstream fin;
 std::string worldpath = world_generator->all_worlds[worldname]->world_path;
 worldpath += "/";
 std::stringstream playerfile;
 playerfile << worldpath << name << ".sav";
 fin.open(playerfile.str().c_str(), std::ifstream::in | std::ifstream::binary);
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
 unserialize(fin);
 fin.close();

 // Stair handling.
 if (!coming_to_stairs.empty()) {
    monstairx = -1;
    monstairy = -1;
    monstairz = 999;
 }

 // weather
 std::string wfile = std::string( worldpath + base64_encode(u.name) + ".weather" );
 fin.open(wfile.c_str());
 if (fin.is_open()) {
     weather_log.clear();
     load_weather(fin);
 }
 fin.close();
 if ( weather_log.empty() ) { // todo: game::get_default_weather() { based on OPTION["STARTING_SEASON"]
    weather = WEATHER_CLEAR;
    temperature = 65;
    nextweather = int(turn)+300;
 }
 // log
 std::string mfile = std::string( worldpath + base64_encode(u.name) + ".log" );
 fin.open(mfile.c_str());
 if (fin.is_open()) {
      u.load_memorial_file( fin );
 }
 fin.close();
 // Now that the player's worn items are updated, their sight limits need to be
 // recalculated. (This would be cleaner if u.worn were private.)
 u.recalc_sight_limits();

 load_auto_pickup(true); // Load character auto pickup rules
 load_uistate(worldname);
// Now load up the master game data; factions (and more?)
 load_master(worldname);
 update_map(u.posx, u.posy);
 set_adjacent_overmaps(true);
 MAPBUFFER.set_dirty();
 draw();
}

//Saves all factions and missions and npcs.
void game::save_factions_missions_npcs ()
{
    std::stringstream masterfile;
    std::ofstream fout;
    masterfile << world_generator->active_world->world_path <<"/master.gsav";

    fout.open(masterfile.str().c_str());
    serialize_master(fout);
    fout.close();
}

void game::save_artifacts()
{
    std::ofstream fout;
    std::string artfilename = world_generator->active_world->world_path + "/artifacts.gsav";
    fout.open(artfilename.c_str(), std::ofstream::trunc);
    JsonOut json(&fout);
    json.start_array();
    for ( std::vector<std::string>::iterator it =
          artifact_itype_ids.begin();
          it != artifact_itype_ids.end(); ++it)
    {
        it_artifact_tool *art = dynamic_cast<it_artifact_tool*>(itypes[*it]);
        if (art) {
            json.write(*art);
        } else {
            json.write(*(dynamic_cast<it_artifact_armor*>(itypes[*it])));
    }
    }
    json.end_array();
    fout.close();
}

void game::save_maps()
{
    m.save(cur_om, turn, levx, levy, levz);
    overmap_buffer.save();
    MAPBUFFER.save();
}

void game::save_uistate() {
    std::stringstream savefile;
    savefile << world_generator->active_world->world_path << "/uistate.json";
    std::ofstream fout;
    fout.open(savefile.str().c_str());
    fout << uistate.serialize();
    fout.close();
}

void game::save()
{
 std::stringstream playerfile;
 std::ofstream fout;
 playerfile << world_generator->active_world->world_path << "/" << base64_encode(u.name);

 fout.open( std::string(playerfile.str() + ".sav").c_str() );
 serialize(fout);
 fout.close();
 // weather
 fout.open( std::string(playerfile.str() + ".weather").c_str() );
 save_weather(fout);
 fout.close();
 // log
 fout.open( std::string(playerfile.str() + ".log").c_str() );
 fout << u.dump_memorial();
 fout.close();
 //factions, missions, and npcs, maps and artifact data is saved in cleanup_at_end()
 save_auto_pickup(true); // Save character auto pickup rules
 save_uistate();
}

void game::delete_world(std::string worldname, bool delete_folder)
{
    std::string worldpath = world_generator->all_worlds[worldname]->world_path;
    std::string filetmp = "";
    std::string world_opfile = "worldoptions.txt";
#if (defined _WIN32 || defined __WIN32__)
      WIN32_FIND_DATA FindFileData;
      HANDLE hFind;
      TCHAR Buffer[MAX_PATH];

      GetCurrentDirectory(MAX_PATH, Buffer);
      SetCurrentDirectory(worldpath.c_str());
      hFind = FindFirstFile("*", &FindFileData);
      if(INVALID_HANDLE_VALUE != hFind) {
       do {
        filetmp = FindFileData.cFileName;
        if (delete_folder || filetmp != world_opfile){
        DeleteFile(FindFileData.cFileName);
        }
       } while(FindNextFile(hFind, &FindFileData) != 0);
       FindClose(hFind);
      }
      SetCurrentDirectory(Buffer);
      if (delete_folder){
        RemoveDirectory(worldpath.c_str());
      }
#else
     DIR *save_dir = opendir(worldpath.c_str());
     if(save_dir != NULL)
     {
      struct dirent *save_dirent = NULL;
      while ((save_dirent = readdir(save_dir)) != NULL){
        filetmp = save_dirent->d_name;
        if (delete_folder || filetmp != world_opfile){
          (void)unlink(std::string(worldpath + "/" + filetmp).c_str());
        }
      }
      (void)closedir(save_dir);
     }
     if (delete_folder){
        remove(worldpath.c_str());
     }
#endif
}

std::vector<std::string> game::list_active_characters()
{
    std::vector<std::string> saves;
    std::vector<std::string> worldsaves = world_generator->active_world->world_saves;
    for (int i = 0; i < worldsaves.size(); ++i){
        saves.push_back(base64_decode(worldsaves[i]));
    }
    return saves;
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

    /* Remove non-ASCII glyphs from character names - unicode symbols are not
     * valid in filenames. */
    std::stringstream player_name;
    for(int index = 0; index < u.name.size(); index++) {
        if((unsigned char)u.name[index] <= '~') {
            player_name << u.name[index];
        }
    }
    if(player_name.str().length() > 0) {
        //Separate name and timestamp
        player_name << '-';
    }

    //Omit the name if too many unusable characters stripped
    std::string memorial_file_path = string_format("memorial/%s%s.txt",
            player_name.str().length() <= (u.name.length() / 5) ? "" : player_name.str().c_str(),
            timestamp.c_str());

    std::ofstream memorial_file;
    memorial_file.open(memorial_file_path.c_str());

    u.memorial( memorial_file );

    if(!memorial_file.is_open()) {
      dbg(D_ERROR) << "game:write_memorial_file: Unable to open " << memorial_file_path;
      debugmsg("Could not open memorial file '%s'", memorial_file_path.c_str());
    }


    //Cleanup
    memorial_file.close();
    closedir(dir);
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

void game::add_msg_if_npc(player *p, const char* msg, ...)
{
    if (!p || !p->is_npc()) {
        return;
    }
    va_list ap;
    va_start(ap, msg);

    char buff[1024];
    vsprintf(buff, msg, ap);
    std::string processed_npc_string(buff);
    // These strings contain the substring <npcname>,
    // if present replace it with the actual npc name.
    size_t offset = processed_npc_string.find("<npcname>");
    if (offset != std::string::npos) {
        processed_npc_string.replace(offset, 9,  p->name);
    }
    add_msg_string(processed_npc_string);

    va_end(ap);
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
            processed_npc_string.replace(offset, 9,  p->name);
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

struct terrain {
   ter_id ter;
   terrain(ter_id tid) : ter(tid) {};
   terrain(std::string sid) {
       ter = t_null;
       if ( termap.find(sid) == termap.end() ) {
           debugmsg("terrain '%s' does not exist.",sid.c_str() );
       } else {
           ter = termap[ sid ].loadid;
       }
   };
};

bool game::event_queued(event_type type)
{
 for (int i = 0; i < events.size(); i++) {
  if (events[i].type == type)
   return true;
  }
  return false;
}
#include "savegame.h"
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
                   #ifdef LUA
                       _("Lua Command"), // 18
                   #endif
                   _("Cancel"),
                   NULL);
 int veh_num;
 std::vector<std::string> opts;
 switch (action) {
  case 1:
   wishitem(&u);
   break;

  case 2:
   teleport(&u, false);
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
            m.clear_vehicle_cache();
            m.vehicle_list.clear();
            // Save monsters.
            for (unsigned int i = 0; i < num_zombies(); i++) {
                force_save_monster(zombie(i));
            }
            clear_zombies();
            levx = tmp.x * 2 - int(MAPSIZE / 2);
            levy = tmp.y * 2 - int(MAPSIZE / 2);
            set_adjacent_overmaps(true);
            m.load(this, levx, levy, levz);
            load_npcs();
            m.spawn_monsters(this); // Static monsters
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
   wishmonster();
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
             otermap[cur_om->ter(levx / 2, levy / 2, levz)].name.c_str(),
             int(turn), int(nextspawn), (!ACTIVE_WORLD_OPTIONS["RANDOM_NPC"] ? _("NPCs are going to spawn.") :
                                         _("NPCs are NOT going to spawn.")),
             num_zombies(), active_npc.size(), events.size());
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
   wishmutate(&u);
   break;

  case 10:
   if (m.veh_at(u.posx, u.posy)) {
    dbg(D_ERROR) << "game:load: There's already vehicle here";
    debugmsg ("There's already vehicle here");
   }
   else {
    for(std::map<std::string, vehicle*>::iterator it = vtypes.begin();
             it != vtypes.end(); ++it) {
      if(it->first != "custom") {
        opts.push_back(it->second->type);
      }
    }
    opts.push_back (std::string(_("Cancel")));
    veh_num = menu_vec (false, _("Choose vehicle to spawn"), opts) + 1;
    veh_num -= 2;
    if(veh_num < opts.size() - 1) {
      //Didn't pick Cancel
      std::string selected_opt = opts[veh_num];
      vehicle* veh = m.add_vehicle (this, selected_opt, u.posx, u.posy, -90, 100, 0);
      if(veh != NULL) {
        m.board_vehicle (this, u.posx, u.posy, &u);
      }
    }
   }
   break;

  case 11: {
    wishskill(&u);
    }
    break;

  case 12:
      // TODO: Give the player martial arts.
      add_msg("Martial arts debug disabled.");
   break;

  case 13: {
   point pos = look_around();
   int npcdex = npc_at(pos.x, pos.y);
   if (npcdex == -1)
    popup(_("No NPC there."));
   else {
    std::stringstream data;
    npc *p = active_npc[npcdex];
    uimenu nmenu;
    nmenu.return_invalid = true;
    data << p->name << " " << (p->male ? _("Male") : _("Female")) << std::endl;

    data << npc_class_name(p->myclass) << "; " <<
            npc_attitude_name(p->attitude) << std::endl;
    if (p->has_destination()) {
     data << _("Destination: ") << p->goalx << ":" << p->goaly << "(" <<
             otermap[ cur_om->ter(p->goalx, p->goaly, p->goalz) ].name << ")" << std::endl;
    } else {
     data << _("No destination.") << std::endl;
    }
    data << _("Trust: ") << p->op_of_u.trust << _(" Fear: ") << p->op_of_u.fear <<
            _(" Value: ") << p->op_of_u.value << _(" Anger: ") << p->op_of_u.anger <<
            _(" Owed: ") << p->op_of_u.owed << std::endl;

    data << _("Aggression: ") << int(p->personality.aggression) << _(" Bravery: ") <<
            int(p->personality.bravery) << _(" Collector: ") <<
            int(p->personality.collector) << _(" Altruism: ") <<
            int(p->personality.altruism) << std::endl << " " << std::endl;
    nmenu.text=data.str();
    nmenu.addentry(0,true,'s',"%s",_("Edit [s]kills"));
    nmenu.addentry(1,true,'q',"%s",_("[q]uit"));
    nmenu.selected = 0;
    nmenu.query();
    if (nmenu.ret == 0 ) {
      wishskill(p);
    }
   }
  } break;

  case 14:
  {
      point center = look_around();
      artifact_natural_property prop =
          artifact_natural_property(rng(ARTPROP_NULL + 1, ARTPROP_MAX - 1));
      m.create_anomaly(center.x, center.y, prop);
      m.spawn_artifact(center.x, center.y, new_natural_artifact(itypes, prop), 0);
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
      weather_menu.addentry(-10,true,'v',"View weather log");
      weather_menu.query();

      if(weather_menu.ret > 0 && weather_menu.ret < NUM_WEATHER_TYPES) {
        add_msg("%d", weather_menu.selected);

        int selected_weather = weather_menu.selected + 1;
        weather = (weather_type) selected_weather;
      } else if(weather_menu.ret == -10) {
          uimenu weather_log_menu;
          int pweather = 0;
          int cweather = 0;
          std::map<int, weather_segment>::iterator pit = weather_log.lower_bound(int(turn));
          --pit;
          if ( pit != weather_log.end() ) {
              cweather = pit->first;
          }
          if ( cweather > 5 ) {
              --pit;
              if ( pit != weather_log.end() ) {
                  pweather = pit->first;
              }
          }
          weather_log_menu.text = string_format("turn: %d, next: %d, current: %d, prev: %d",
              int(turn), int(nextweather), cweather, pweather
          );
          for(std::map<int, weather_segment>::const_iterator it = weather_log.begin(); it != weather_log.end(); ++it) {
              weather_log_menu.addentry(-1,true,-1,"%dd%dh %d %s[%d] %d",
                  it->second.deadline.days(),it->second.deadline.hours(),
                  it->first,
                  weather_data[int(it->second.weather)].name.c_str(),
                  it->second.weather,
                  (int)it->second.temperature
              );
              if ( it->first == cweather ) {
                  weather_log_menu.entries.back().text_color = c_yellow;
          }
          }
          weather_log_menu.query();
      }
  }
  break;

  #ifdef LUA
      case 18: {
          std::string luacode = string_input_popup(_("Lua:"), 60, "");
          call_lua(luacode);
      }
      break;
  #endif
 }
 erase();
 refresh_all();
}

void game::mondebug()
{
 int tc;
 for (int i = 0; i < num_zombies(); i++) {
  monster &z = _active_monsters[i];
  z.debug(u);
  if (z.has_flag(MF_SEES) &&
      m.sees(z.posx(), z.posy(), u.posx, u.posy, -1, tc))
   debugmsg("The %s can see you.", z.name().c_str());
  else
   debugmsg("The %s can't see you...", z.name().c_str());
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
 for (std::map<std::string, int>::iterator kill = kills.begin(); kill != kills.end(); ++kill){
    types.push_back(MonsterGenerator::generator().get_mtype(kill->first));
    count.push_back(kill->second);
 }

 mvwprintz(w, 1, 32, c_white, _("KILL COUNT:"));

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
 std::vector<faction> valfac; // Factions that we know of.
 for (int i = 0; i < factions.size(); i++) {
  if (factions[i].known_by_u)
   valfac.push_back(factions[i]);
 }
 if (valfac.size() == 0) { // We don't know of any factions!
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
  case DirectionS: // Move selection down
   mvwprintz(w_list, sel + 2, 1, c_white, valfac[sel].name.c_str());
   if (sel == valfac.size() - 1)
    sel = 0; // Wrap around
   else
    sel++;
   break;
  case DirectionN: // Move selection up
   mvwprintz(w_list, sel + 2, 1, c_white, valfac[sel].name.c_str());
   if (sel == 0)
    sel = valfac.size() - 1; // Wrap around
   else
    sel--;
   break;
  case Cancel:
  case Close:
   sel = -1;
   break;
  }
  if (input == DirectionS || input == DirectionN) { // Changed our selection... update the windows
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
   case 0: umissions = u.active_missions;       break;
   case 1: umissions = u.completed_missions;    break;
   case 2: umissions = u.failed_missions;       break;
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
  draw_tab(w_missions, 56, _("FAILED MISSIONS"), (tab == 2) ? true : false);

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
    if (!liveview.compact_view) {
        liveview.hide(true, true);
    }
    u.disp_status(w_status, w_status2, this);

    bool sideStyle = use_narrow_sidebar();

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
    if (cur_ter == "")
    {
        if (cur_loc.x >= OMAPX && cur_loc.y >= OMAPY)
            cur_ter = om_diag->ter(cur_loc.x - OMAPX, cur_loc.y - OMAPY, levz);
        else if (cur_loc.x >= OMAPX)
            cur_ter = om_hori->ter(cur_loc.x - OMAPX, cur_loc.y, levz);
        else if (cur_loc.y >= OMAPY)
            cur_ter = om_vert->ter(cur_loc.x, cur_loc.y - OMAPY, levz);
    }

    std::string tername = otermap[cur_ter].name;
    werase(w_location);
    mvwprintz(w_location, 0,  0, otermap[cur_ter].color, utf8_substr(tername, 0, 14).c_str());

    if (levz < 0) {
        mvwprintz(w_location, 0, 18, c_ltgray, _("Underground"));
    } else {
        mvwprintz(w_location, 0, 18, weather_data[weather].color, weather_data[weather].name.c_str());
    }

    nc_color col_temp = c_blue;
    int display_temp = get_temperature();
    if (display_temp >= 90) {
        col_temp = c_red;
    } else if (display_temp >= 75) {
        col_temp = c_yellow;
    } else if (display_temp >= 60) {
        col_temp = c_ltgreen;
    } else if (display_temp >= 50) {
        col_temp = c_cyan;
    } else if (display_temp >  32) {
        col_temp = c_ltblue;
    }

    wprintz(w_location, col_temp, (std::string(" ") + print_temperature((float)display_temp)).c_str());
    wrefresh(w_location);

    //Safemode coloring
    WINDOW *day_window = sideStyle ? w_status2 : w_status;
    mvwprintz(day_window, 0, sideStyle ? 0 : 41, c_white, _("%s, day %d"), _(season_name[turn.get_season()].c_str()), turn.days() + 1);
    if (run_mode != 0 || autosafemode != 0) {
        int iPercent = int((turnssincelastmon*100)/OPTIONS["AUTOSAFEMODETURNS"]);
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
    if (test > down && test < up) {
        return true;
    } else {
        return false;
    }
}

void game::draw_ter(int posx, int posy)
{
 mapRain.clear();
// posx/posy default to -999
 if (posx == -999)
  posx = u.posx + u.view_offset_x;
 if (posy == -999)
  posy = u.posy + u.view_offset_y;

 ter_view_x = posx;
 ter_view_y = posy;

 m.build_map_cache(this);
 m.draw(this, w_terrain, point(posx, posy));

    // Draw monsters
    int mx, my;
    for (int i = 0; i < num_zombies(); i++) {
        monster &z = _active_monsters[i];
        my = POSY + (z.posy() - posy);
        mx = POSX + (z.posx() - posx);
        if (mx >= 0 && my >= 0 && mx < TERRAIN_WINDOW_WIDTH
                && my < TERRAIN_WINDOW_HEIGHT && u_see(&z)) {
            z.draw(w_terrain, posx, posy, false);
            mapRain[my][mx] = false;
        } else if (z.has_flag(MF_WARM)
                   && mx >= 0 && my >= 0
                   && mx < TERRAIN_WINDOW_WIDTH && my < TERRAIN_WINDOW_HEIGHT
                   && (u.has_active_bionic("bio_infrared")
                       || u.has_trait("INFRARED")
                       || u.has_trait("LIZ_IR"))
                   && m.pl_sees(u.posx,u.posy,z.posx(),z.posy(),
                                u.sight_range(DAYLIGHT_LEVEL))) {
            mvwputch(w_terrain, my, mx, c_red, '?');
        }
    }

    // Draw NPCs
    for (int i = 0; i < active_npc.size(); i++) {
        my = POSY + (active_npc[i]->posy - posy);
        mx = POSX + (active_npc[i]->posx - posx);
        if (mx >= 0 && my >= 0 && mx < TERRAIN_WINDOW_WIDTH
                && my < TERRAIN_WINDOW_HEIGHT
                && u_see(active_npc[i]->posx, active_npc[i]->posy)) {
            active_npc[i]->draw(w_terrain, posx, posy, false);
        }
    }

    if (u.has_active_bionic("bio_scent_vision")) {
        for (int realx = posx - POSX; realx <= posx + POSX; realx++) {
            for (int realy = posy - POSY; realy <= posy + POSY; realy++) {
                if (scent(realx, realy) != 0) {
                    int tempx = posx - realx, tempy = posy - realy;
                    if (!(isBetween(tempx, -2, 2) && isBetween(tempy, -2, 2))) {
                        if (mon_at(realx, realy) != -1) {
                            mvwputch(w_terrain, realy + POSY - posy,
                                     realx + POSX - posx, c_white, '?');
                        } else {
                            mvwputch(w_terrain, realy + POSY - posy,
                                     realx + POSX - posx, c_magenta, '#');
                        }
                    }
                }
            }
        }
    }

    if (destination_preview.size() > 0) {
        // Draw auto-move preview trail
        point final_destination = destination_preview.back();
        point center = point(u.posx + u.view_offset_x, u.posy + u.view_offset_y);
        draw_line(final_destination.x, final_destination.y, center, destination_preview);
        mvwputch(w_terrain, POSY + (final_destination.y - (u.posy + u.view_offset_y)),
            POSX + (final_destination.x - (u.posx + u.view_offset_x)), c_white, 'X');
    }

    wrefresh(w_terrain);

    if (u.has_disease("visuals") || (u.has_disease("hot_head") &&
            u.disease_intensity("hot_head") != 1)) {
        hallucinate(posx, posy);
    }
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
    static body_part part[] = { bp_head, bp_torso, bp_arms,
                           bp_arms, bp_legs, bp_legs, num_bp};
    static int side[] = { -1, -1, 0, 1, 0, 1, -1};
    int num_parts = sizeof(body_parts) / sizeof(body_parts[0]);
    for (int i = 0; i < num_parts; i++) {
        const char *str = body_parts[i];
        wmove(w_HP, i * dy, 0);
        if (wide)
            wprintz(w_HP, limb_color(&u, part[i], side[i]), " ");
        wprintz(w_HP, limb_color(&u, part[i], side[i]), str);
        if (!wide)
            wprintz(w_HP, limb_color(&u, part[i], side[i]), ":");
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

nc_color game::limb_color(player *p, body_part bp, int side, bool bleed, bool bite, bool infect)
{
    if (bp == num_bp) {
        return c_ltgray;
    }

    int color_bit = 0;
    nc_color i_color = c_ltgray;
    if (bleed && p->has_disease("bleed", bp, side)) {
        color_bit += 1;
    }
    if (bite && p->has_disease("bite", bp, side)) {
        color_bit += 10;
    }
    if (infect && p->has_disease("infected", bp, side)) {
        color_bit += 100;
    }
    switch (color_bit) {
        case 1:
            i_color = c_red;
            break;
        case 10:
            i_color = c_blue;
            break;
        case 100:
            i_color = c_green;
            break;
        case 11:
            i_color = c_magenta;
            break;
        case 101:
            i_color = c_yellow;
            break;
    }
    return i_color;
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
   oter_id cur_ter;// = "";
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
   nc_color ter_color = otermap[cur_ter].color;
   long ter_sym = otermap[cur_ter].sym;
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
    arrowx = int(3 + 3 * (targ.y > cursy ? slope : (0 - slope)));
    if (arrowx < 0)
     arrowx = 0;
    if (arrowx > 6)
     arrowx = 6;
   } else {
    arrowx = (targ.x > cursx ? 6 : 0);
    arrowy = int(3 + 3 * (targ.x > cursx ? slope : (0 - slope)));
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
    const int rx = x - POSX;
    const int ry = y - POSY;
    for (int i = 0; i <= TERRAIN_WINDOW_WIDTH; i++) {
        for (int j = 0; j <= TERRAIN_WINDOW_HEIGHT; j++) {
            if (one_in(10)) {
                char ter_sym = terlist[m.ter(i + rx + rng(-2, 2),
                                             j + ry + rng(-2, 2))].sym;
                nc_color ter_col = terlist[m.ter(i + rx + rng(-2, 2),
                                                 j + ry + rng(-2, 2))].color;
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
  ret = (float)turn.sunlight();
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
 if (levz < 0) // Underground!
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
    int range = 0;
 int mondex = mon_at(x,y);
 if (mondex != -1) {
  monster &z = _active_monsters[mondex];
        range = z.vision_range(u.posx, u.posy);
 }

 return (!(u.has_active_bionic("bio_cloak") || u.has_active_bionic("bio_night") ||
           u.has_active_optcloak() || u.has_artifact_with(AEP_INVISIBLE))
           && m.sees(x, y, u.posx, u.posy, range, t));
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
 int dist = rl_dist(u.posx, u.posy, mon->posx(), mon->posy());
 if (u.has_trait("ANTENNAE") && dist <= 3) {
  return true;
 }
 if (mon->digging() && !u.has_active_bionic("bio_ground_sonar") && dist > 1) {
  return false; // Can't see digging monsters until we're right next to them
 }
 if (m.is_divable(mon->posx(), mon->posy()) && mon->can_submerge()
         && !u.is_underwater()) {
   //Monster is in the water and submerged, and we're out of/above the water
   return false;
 }

 return u_see(mon->posx(), mon->posy());
}

bool game::pl_sees(player *p, monster *mon, int &t)
{
 // TODO: [lightmap] Allow npcs to use the lightmap
 if (mon->digging() && !p->has_active_bionic("bio_ground_sonar") &&
       rl_dist(p->posx, p->posy, mon->posx(), mon->posy()) > 1)
  return false; // Can't see digging monsters until we're right next to them
 int range = p->sight_range(light_level());
 return m.sees(p->posx, p->posy, mon->posx(), mon->posy(), range, t);
}

/**
 * Attempts to find which map co-ordinates the specified item is located at,
 * looking at the player, the ground, NPCs, and vehicles in that order.
 * @param it A pointer to the item to find.
 * @return The location of the item, or (-999, -999) if it wasn't found.
 */
point game::find_item(item *it)
{
    //Does the player have it?
    if (u.has_item(it)) {
        return point(u.posx, u.posy);
    }
    //Is it in a vehicle?
    for (std::set<vehicle*>::iterator veh_iterator = m.vehicle_list.begin();
            veh_iterator != m.vehicle_list.end(); veh_iterator++) {
        vehicle *next_vehicle = *veh_iterator;
        std::vector<int> cargo_parts = next_vehicle->all_parts_with_feature("CARGO", false);
        for(std::vector<int>::iterator part_index = cargo_parts.begin();
                part_index != cargo_parts.end(); part_index++) {
            std::vector<item> *items_in_part = &(next_vehicle->parts[*part_index].items);
            for (int n = items_in_part->size() - 1; n >= 0; n--) {
                if (&((*items_in_part)[n]) == it) {
                    int mapx = next_vehicle->global_x() + next_vehicle->parts[*part_index].precalc_dx[0];
                    int mapy = next_vehicle->global_y() + next_vehicle->parts[*part_index].precalc_dy[0];
                    return point(mapx, mapy);
                }
            }
        }
    }
    //Does an NPC have it?
    for (int i = 0; i < active_npc.size(); i++) {
        if (active_npc[i]->inv.has_item(it)) {
            return point(active_npc[i]->posx, active_npc[i]->posy);
        }
    }
    //Is it on the ground? (Check this last - takes the most time)
    point ret = m.find_item(it);
    if (ret.x != -1 && ret.y != -1) {
        return ret;
    }
    //Not found anywhere
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

bool vector_has(std::vector<std::string> vec, std::string test)
{
    for (int i = 0; i < vec.size(); ++i){
        if (vec[i] == test){
            return true;
        }
    }
    return false;
}

bool vector_has(std::vector<int> vec, int test)
{
 for (int i = 0; i < vec.size(); i++) {
  if (vec[i] == test)
   return true;
 }
 return false;
}

bool game::is_hostile_nearby()
{
    int distance = (OPTIONS["SAFEMODEPROXIMITY"] <= 0) ? 60 : OPTIONS["SAFEMODEPROXIMITY"];
    return is_hostile_within(distance);
}

bool game::is_hostile_very_close()
{
    return is_hostile_within(dangerous_proximity);
}

bool game::is_hostile_within(int distance){
    for (int i = 0; i < num_zombies(); i++) {
        monster &z = _active_monsters[i];
        if (!u_see(&z))
            continue;

        monster_attitude matt = z.attitude(&u);
        if (MATT_ATTACK != matt && MATT_FOLLOW != matt)
            continue;

        int mondist = rl_dist(u.posx, u.posy, z.posx(), z.posy());
        if (mondist <= distance)
            return true;
    }

    for (int i = 0; i < active_npc.size(); i++) {
        point npcp(active_npc[i]->posx, active_npc[i]->posy);

        if (!u_see(npcp.x, npcp.y))
            continue;

        if (active_npc[i]->attitude != NPCATT_KILL)
            continue;

        if (rl_dist(u.posx, u.posy, npcp.x, npcp.y) <= distance)
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
    const int startrow = use_narrow_sidebar() ? 1 : 0;

    int buff;
    std::string sbuff;
    int newseen = 0;
    const int iProxyDist = (OPTIONS["SAFEMODEPROXIMITY"] <= 0) ? 60 : OPTIONS["SAFEMODEPROXIMITY"];
    int newdist = 4096;
    int newtarget = -1;
    // 7 0 1    unique_types uses these indices;
    // 6 8 2    0-7 are provide by direction_from()
    // 5 4 3    8 is used for local monsters (for when we explain them below)
    std::vector<int> unique_types[9];
    std::vector<std::string> unique_mons[9];
    // dangerous_types tracks whether we should print in red to warn the player
    bool dangerous[8];
    for (int i = 0; i < 8; i++)
        dangerous[i] = false;

    direction dir_to_mon, dir_to_npc;
    int viewx = u.posx + u.view_offset_x;
    int viewy = u.posy + u.view_offset_y;
    new_seen_mon.clear();

    for (int i = 0; i < num_zombies(); i++) {
        monster &z = _active_monsters[i];
        if (u_see(&z) && !z.type->has_flag(MF_VERMIN)) {
            dir_to_mon = direction_from(viewx, viewy, z.posx(), z.posy());
            int index;
            int mx = POSX + (z.posx() - viewx);
            int my = POSY + (z.posy() - viewy);
            if (mx >= 0 && my >= 0 && mx < TERRAIN_WINDOW_WIDTH && my < TERRAIN_WINDOW_HEIGHT) {
                index = 8;
            } else {
                index = dir_to_mon;
            }

            monster_attitude matt = z.attitude(&u);
            if (MATT_ATTACK == matt || MATT_FOLLOW == matt) {
                int j;
                if (index < 8 && sees_u(z.posx(), z.posy(), j))
                    dangerous[index] = true;

                int mondist = rl_dist(u.posx, u.posy, z.posx(), z.posy());
                if (mondist <= iProxyDist) {
                    bool passmon = false;

                    if ( z.ignoring > 0 ) {
                        if ( run_mode != 1 ) {
                            z.ignoring = 0;
                        } else if ( mondist > z.ignoring / 2 || mondist < 6 ) {
                            passmon = true;
                        }
                    }
                    if (!passmon) {
                        newseen++;
                        new_seen_mon.push_back(i);
                        if ( mondist < newdist ) {
                            newdist = mondist; // todo: prioritize dist * attack+follow > attack > follow
                            newtarget = i; // todo: populate alt targeting map
                        }
                    }
                }
            }

            if (!vector_has(unique_mons[dir_to_mon], z.type->id))
                unique_mons[index].push_back(z.type->id);
        }
    }

    for (int i = 0; i < active_npc.size(); i++) {
        point npcp(active_npc[i]->posx, active_npc[i]->posy);
        if (u_see(npcp.x, npcp.y)) { // TODO: NPC invis
            if (active_npc[i]->attitude == NPCATT_KILL)
                if (rl_dist(u.posx, u.posy, npcp.x, npcp.y) <= iProxyDist)
                    newseen++;

            dir_to_npc = direction_from(viewx, viewy, npcp.x, npcp.y);
            int index;
            int mx = POSX + (npcp.x - viewx);
            int my = POSY + (npcp.y - viewy);
            if (mx >= 0 && my >= 0 && mx < TERRAIN_WINDOW_WIDTH && my < TERRAIN_WINDOW_HEIGHT) {
                index = 8;
            } else {
                index = dir_to_npc;
            }

            unique_types[index].push_back(-1 - i);
        }
    }

    if (newseen > mostseen) {
        cancel_activity_query(_("Monster spotted!"));
        turnssincelastmon = 0;
        if (run_mode == 1) {
            run_mode = 2; // Stop movement!
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
    // 7 0 1    unique_types uses these indices;
    // 6 8 2    0-7 are provide by direction_from()
    // 5 4 3    8 is used for local monsters (for when we explain them below)

    const char *dir_labels[] = {
        _("North:"), _("NE:"), _("East:"), _("SE:"),
        _("South:"), _("SW:"), _("West:"), _("NW:") };
    int widths[8];
    for (int i = 0; i < 8; i++) {
        widths[i] = utf8_width(dir_labels[i]);
    }
    int xcoords[8];
    const int ycoords[] = { 0, 0, 1, 2, 2, 2, 1, 0 };
    xcoords[0] = xcoords[4] = width / 3;
    xcoords[1] = xcoords[3] = xcoords[2] = (width / 3) * 2;
    xcoords[5] = xcoords[6] = xcoords[7] = 0;
    for (int i = 0; i < 8; i++) {
        nc_color c = unique_types[i].empty() && unique_mons[i].empty() ? c_dkgray
                   : (dangerous[i] ? c_ltred : c_ltgray);
        mvwprintz(w, ycoords[i] + startrow, xcoords[i], c, dir_labels[i]);
    }

    // Print the symbols of all monsters in all directions.
    for (int i = 0; i < 8; i++) {
        int symroom;
        point pr(xcoords[i] + widths[i] + 1, ycoords[i] + startrow);

        // The list of symbols needs a space on each end.
        symroom = (width / 3) - widths[i] - 2;
        const int typeshere_npc = unique_types[i].size();
        const int typeshere_mon = unique_mons[i].size();
        const int typeshere = typeshere_mon + typeshere_npc;
        for (int j = 0; j < typeshere && j < symroom; j++) {
            nc_color c;
            char sym;
            if (symroom < typeshere && j == symroom - 1) {
                // We've run out of room!
                c = c_white;
                sym = '+';
            } else if (j < typeshere_npc){
                buff = unique_types[i][j];
                switch (active_npc[(buff + 1) * -1]->attitude) {
                    case NPCATT_KILL:   c = c_red;     break;
                    case NPCATT_FOLLOW: c = c_ltgreen; break;
                    case NPCATT_DEFEND: c = c_green;   break;
                    default:            c = c_pink;    break;
                }
                sym = '@';
            }else{
                sbuff = unique_mons[i][j - typeshere_npc];
                c   = GetMType(sbuff)->color;
                sym = GetMType(sbuff)->sym;
            }
            mvwputch(w, pr.y, pr.x, c, sym);

            pr.x++;
        }
    } // for (int i = 0; i < 8; i++)

    // Now we print their full names!

    std::set<std::string> listed_mons;

    // Start printing monster names on row 4. Rows 0-2 are for labels, and row 3
    // is blank.
    point pr(0, 4 + startrow);

    int lastrowprinted = 2 + startrow;

    // Print monster names, starting with those at location 8 (nearby).
    for (int j = 8; j >= 0 && pr.y < maxheight; j--) {
        // Separate names by some number of spaces (more for local monsters).
        int namesep = (j == 8 ? 2 : 1);
        for (int i = 0; i < unique_mons[j].size() && pr.y < maxheight; i++) {
            sbuff = unique_mons[j][i];
            // buff < 0 means an NPC!  Don't list those.
            if (listed_mons.find(sbuff) == listed_mons.end()){
                listed_mons.insert(sbuff);

                std::string name = GetMType(sbuff)->name;

                // Move to the next row if necessary. (The +2 is for the "Z ").
                if (pr.x + 2 + name.length() >= width) {
                    pr.y++;
                    pr.x = 0;
                }

                if (pr.y < maxheight) { // Don't print if we've overflowed
                    lastrowprinted = pr.y;
                    mvwputch(w, pr.y, pr.x, GetMType(sbuff)->color, GetMType(sbuff)->sym);
                    pr.x += 2; // symbol and space
                    nc_color danger = c_dkgray;
                    if (GetMType(sbuff)->difficulty >= 30)
                        danger = c_red;
                    else if (GetMType(sbuff)->difficulty >= 16)
                        danger = c_ltred;
                    else if (GetMType(sbuff)->difficulty >= 8)
                        danger = c_white;
                    else if (GetMType(sbuff)->agro > 0)
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
    for( int i = 0; i < num_zombies(); i++ ) {
        monster &z = _active_monsters[i];
        if( z.dead || z.hp <= 0 ) {
            dbg (D_INFO) << string_format( "cleanup_dead: z[%d] %d,%d dead:%c hp:%d %s",
                                           i, z.posx(), z.posy(), (z.dead?'1':'0'),
                                           z.hp, z.type->name.c_str() );
            remove_zombie(i);
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

    // monster::plan() needs to know about all monsters with nonzero friendliness.
    // We'll build this list once (instead of once per monster) for speed.
    std::vector<int> friendlies;
    for (int i = 0, numz = num_zombies(); i < numz; i++) {
        if (zombie(i).friendly) {
            friendlies.push_back(i);
        }
    }

 for (int i = 0; i < num_zombies(); i++) {
  monster &z = _active_monsters[i];
  while (!z.dead && !z.can_move_to(this, z.posx(), z.posy())) {
// If we can't move to our current position, assign us to a new one
   if (debugmon)
   {
    dbg(D_ERROR) << "game:monmove: " << z.name().c_str()
                 << " can't move to its location! (" << z.posx()
                 << ":" << z.posy() << "), "
                 << m.tername(z.posx(), z.posy()).c_str();
    debugmsg("%s can't move to its location! (%d:%d), %s", z.name().c_str(),
             z.posx(), z.posy(), m.tername(z.posx(), z.posy()).c_str());
   }
   bool okay = false;
   int xdir = rng(1, 2) * 2 - 3, ydir = rng(1, 2) * 2 - 3; // -1 or 1
   int startx = z.posx() - 3 * xdir, endx = z.posx() + 3 * xdir;
   int starty = z.posy() - 3 * ydir, endy = z.posy() + 3 * ydir;
   for (int x = startx; x != endx && !okay; x += xdir) {
    for (int y = starty; y != endy && !okay; y += ydir){
     if (z.can_move_to(this, x, y) && is_empty(x, y)) {
      z.setpos(x, y);
      okay = true;
     }
    }
   }
   if (!okay)
    z.dead = true;
  }

  if (!z.dead) {
   z.process_effects(this);
   if (z.hurt(0))
    kill_mon(i, false);
  }

  m.mon_in_field(z.posx(), z.posy(), this, &z);

  while (z.moves > 0 && !z.dead) {
   z.made_footstep = false;
   z.plan(this, friendlies); // Formulate a path to follow
   z.move(this); // Move one square, possibly hit u
   z.process_triggers(this);
   m.mon_in_field(z.posx(), z.posy(), this, &z);
   if (z.hurt(0)) { // Maybe we died...
    kill_mon(i, false);
    z.dead = true;
   }
  }

  if (!z.dead) {
   if (u.has_active_bionic("bio_alarm") && u.power_level >= 1 &&
       rl_dist(u.posx, u.posy, z.posx(), z.posy()) <= 5) {
    u.power_level--;
    add_msg(_("Your motion alarm goes off!"));
    cancel_activity_query(_("Your motion alarm goes off!"));
    if (u.has_disease("sleep") || u.has_disease("lying_down")) {
     u.rem_disease("sleep");
     u.rem_disease("lying_down");
    }
   }
// We might have stumbled out of range of the player; if so, kill us
   if (z.posx() < 0 - (SEEX * MAPSIZE) / 6 ||
       z.posy() < 0 - (SEEY * MAPSIZE) / 6 ||
       z.posx() > (SEEX * MAPSIZE * 7) / 6 ||
       z.posy() > (SEEY * MAPSIZE * 7) / 6   ) {
// Re-absorb into local group, if applicable
    int group = valid_group((z.type->id), levx, levy, levz);
    if (group != -1) {
     cur_om->zg[group].population++;
     if (cur_om->zg[group].population / (cur_om->zg[group].radius * cur_om->zg[group].radius) > 5 &&
         !cur_om->zg[group].diffuse )
      cur_om->zg[group].radius++;
    } else if (MonsterGroupManager::Monster2Group((z.type->id)) != "GROUP_NULL") {
     cur_om->zg.push_back(mongroup(MonsterGroupManager::Monster2Group((z.type->id)),
                                  levx, levy, levz, 1, 1));
    }
    z.dead = true;
   } else
    z.receive_moves();
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
    // --- Monster sound handling here ---
    // Alert all monsters (that can hear) to the sound.
    for (int i = 0, numz = num_zombies(); i < numz; i++) {
        monster &z = _active_monsters[i];
        // rl_dist() is faster than z.has_flag() or z.can_hear(), so we'll check it first.
        int dist = rl_dist(x, y, z.posx(), z.posy());
        int vol_goodhearing = vol * 2 - dist;
        if (vol_goodhearing > 0 && z.can_hear()) {
            const bool goodhearing = z.has_flag(MF_GOODHEARING);
            int volume = goodhearing ? vol_goodhearing : (vol - dist);
            // Error is based on volume, louder sound = less error
            if (volume > 0) {
                int max_error = 0;
                if(volume < 2) {
                    max_error = 10;
                } else if(volume < 5) {
                    max_error = 5;
                } else if(volume < 10) {
                    max_error = 3;
                } else if(volume < 20) {
                    max_error = 1;
                }

                int target_x = x + rng(-max_error, max_error);
                int target_y = y + rng(-max_error, max_error);

                int wander_turns = volume * (goodhearing ? 6 : 1);
                z.wander_to(target_x, target_y, wander_turns);
                z.process_trigger(MTRIG_SOUND, volume);
            }
        }
    }

    // --- Player stuff below this point ---
    int dist = rl_dist(x, y, u.posx, u.posy);

    // Player volume meter includes all sounds from their tile and adjacent tiles
    if (dist <= 1) {
        u.volume += vol;
    }

    // Mutation/Bionic volume modifiers
    if (u.has_bionic("bio_ears")) {
  vol *= 3.5;
    }
    if (u.has_trait("BADHEARING")) {
  vol *= .5;
    }
    if (u.has_trait("CANINE_EARS")) {
  vol *= 1.5;
    }

    // Too far away, we didn't hear it!
    if (dist > vol) {
  return false;
 }

    if (u.has_disease("deaf")) {
        // Has to be here as well to work for stacking deafness (loud noises prolong deafness)
 if (!u.has_bionic("bio_ears") && rng( (vol - dist) / 2, (vol - dist) ) >= 150) {
            int duration = std::min(40, (vol - dist - 130) / 4);
            u.add_disease("deaf", duration);
        }
        // We're deaf, can't hear it
        return false;
    }

    // Check for deafness
    if (!u.has_bionic("bio_ears") && rng((vol - dist) / 2, (vol - dist)) >= 150) {
  int duration = (vol - dist - 130) / 4;
  u.add_disease("deaf", duration);
 }

    // See if we need to wake someone up
    if (u.has_disease("sleep")){
        if ((!u.has_trait("HEAVYSLEEPER") && dice(2, 15) < vol - dist) ||
              (u.has_trait("HEAVYSLEEPER") && dice(3, 15) < vol - dist)) {
            u.rem_disease("sleep");
            add_msg(_("You're woken up by a noise."));
        } else {
            return false;
        }
    }

 if (x != u.posx || y != u.posy) {
  if(u.activity.ignore_trivial != true) {
    std::string query;
            if (description != "") {
        query = string_format(_("Heard %s!"), description.c_str());
    } else {
        query = _("Heard a noise!");
    }

    if( cancel_activity_or_ignore_query(query.c_str()) ) {
        u.activity.ignore_trivial = true;
    }
  }
 }

    // Only print a description if it exists
    if (description != "") {
// If it came from us, don't print a direction
        if (x == u.posx && y == u.posy) {
  capitalize_letter(description, 0);
  add_msg("%s", description.c_str());
        } else {
            // Else print a direction as well
 std::string direction = direction_name(direction_from(u.posx, u.posy, x, y));
 add_msg(_("From the %s you hear %s"), direction.c_str(), description.c_str());
        }
    }
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

void game::explosion(int x, int y, int power, int shrapnel, bool has_fire)
{
 int radius = int(sqrt(double(power / 4)));
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
   if (m.has_flag("BASHABLE", i, j))
    m.bash(i, j, dam, junk);
   if (m.has_flag("BASHABLE", i, j)) // Double up for tough doors, etc.
    m.bash(i, j, dam, junk);
   if (m.is_destructable(i, j) && rng(25, 100) < dam)
    m.destroy(this, i, j, false);

   int mon_hit = mon_at(i, j), npc_hit = npc_at(i, j);
   if (mon_hit != -1) {
    monster &z = _active_monsters[mon_hit];
    if (!z.dead && z.hurt(rng(dam / 2, long(dam * 1.5)))) {
     if (z.hp < 0 - (z.type->size < 2? 1.5:3) * z.type->hp)
      explode_mon(mon_hit); // Explode them if it was big overkill
     else
      kill_mon(mon_hit); // TODO: player's fault?

     int vpart;
     vehicle *veh = m.veh_at(i, j, vpart);
     if (veh)
      veh->damage (vpart, dam, false);
    }
   }

   if (npc_hit != -1) {
    active_npc[npc_hit]->hit(this, bp_torso, -1, rng(dam / 2, long(dam * 1.5)), 0);
    active_npc[npc_hit]->hit(this, bp_head,  -1, rng(dam / 3, dam),       0);
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
    u.hit(this, bp_torso, -1, rng(dam / 2, dam * 1.5), 0);
    u.hit(this, bp_head,  -1, rng(dam / 3, dam),       0);
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
 draw_explosion(x, y, radius, c_red);

// The rest of the function is shrapnel
 if (shrapnel <= 0)
  return;
 int sx, sy, t, tx, ty;
 std::vector<point> traj;
 timespec ts;
 ts.tv_sec = 0;
 ts.tv_nsec = BULLET_SPEED; // Reset for animation of bullets
 for (int i = 0; i < shrapnel; i++) {
  sx = rng(x - 2 * radius, x + 2 * radius);
  sy = rng(y - 2 * radius, y + 2 * radius);
  if (m.sees(x, y, sx, sy, 50, t))
   traj = line_to(x, y, sx, sy, t);
  else
   traj = line_to(x, y, sx, sy, 0);
  dam = rng(20, 60);
  for (int j = 0; j < traj.size(); j++) {
   draw_bullet(u, traj[j].x, traj[j].y, j, traj, '`', ts);
   tx = traj[j].x;
   ty = traj[j].y;
   const int zid = mon_at(tx, ty);
   if (zid != -1) {
    monster &z = _active_monsters[zid];
    dam -= z.armor_cut();
    if (z.hurt(dam))
     kill_mon(zid);
   } else if (npc_at(tx, ty) != -1) {
    body_part hit = random_body_part();
    if (hit == bp_eyes || hit == bp_mouth || hit == bp_head)
     dam = rng(2 * dam, 5 * dam);
    else if (hit == bp_torso)
     dam = rng(long(1.5 * dam), 3 * dam);
    int npcdex = npc_at(tx, ty);
    active_npc[npcdex]->hit(this, hit, rng(0, 1), 0, dam);
    if (active_npc[npcdex]->hp_cur[hp_head] <= 0 ||
        active_npc[npcdex]->hp_cur[hp_torso] <= 0) {
     active_npc[npcdex]->die(this);
    }
   } else if (tx == u.posx && ty == u.posy) {
    body_part hit = random_body_part();
    int side = random_side(hit);
    add_msg(_("Shrapnel hits your %s!"), body_part_name(hit, side).c_str());
    u.hit(this, hit, random_side(hit), 0, dam);
   } else {
       std::set<std::string> shrapnel_effects;
       m.shoot(this, tx, ty, dam, j == traj.size() - 1, shrapnel_effects );
   }
  }
 }
}

void game::flashbang(int x, int y, bool player_immune)
{
    g->draw_explosion(x, y, 8, c_white);
    int dist = rl_dist(u.posx, u.posy, x, y), t;
    if (dist <= 8 && !player_immune) {
        if (!u.has_bionic("bio_ears")) {
            u.add_disease("deaf", 40 - dist * 4);
        }
        if (m.sees(u.posx, u.posy, x, y, 8, t)) {
            int flash_mod = 0;
            if (u.has_bionic("bio_sunglasses")) {
                flash_mod = 6;
            }
            u.infect("blind", bp_eyes, (12 - flash_mod - dist) / 2, 10 - dist);
        }
    }
    for (int i = 0; i < num_zombies(); i++) {
        monster &z = _active_monsters[i];
        dist = rl_dist(z.posx(), z.posy(), x, y);
        if (dist <= 4) {
            z.add_effect(ME_STUNNED, 10 - dist);
        }
        if (dist <= 8) {
            if (z.has_flag(MF_SEES) && m.sees(z.posx(), z.posy(), x, y, 8, t)) {
                z.add_effect(ME_BLIND, 18 - dist);
            }
            if (z.has_flag(MF_HEARS)) {
                z.add_effect(ME_DEAF, 60 - dist * 4);
            }
        }
    }
    sound(x, y, 12, _("a huge boom!"));
    // TODO: Blind/deafen NPC
}

void game::shockwave(int x, int y, int radius, int force, int stun, int dam_mult, bool ignore_player)
{
    draw_explosion(x, y, radius, c_blue);

    sound(x, y, force*force*dam_mult/2, _("Crack!"));
    for (int i = 0; i < num_zombies(); i++)
    {
        monster &z = _active_monsters[i];
        if (rl_dist(z.posx(), z.posy(), x, y) <= radius)
        {
            add_msg(_("%s is caught in the shockwave!"), z.name().c_str());
            knockback(x, y, z.posx(), z.posy(), force, stun, dam_mult);
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
    (void)force; //FIXME: unused but header says it should do something
    // TODO: make the force parameter actually do something.
    // the header file says higher force causes more damage.
    // perhaps that is what it should do?
    int tx = traj.front().x;
    int ty = traj.front().y;
    const int zid = mon_at(tx, ty);
    if (zid == -1 && npc_at(tx, ty) == -1 && (u.posx != tx && u.posy != ty))
    {
        debugmsg(_("Nothing at (%d,%d) to knockback!"), tx, ty);
        return;
    }
    //add_msg("line from %d,%d to %d,%d",traj.front().x,traj.front().y,traj.back().x,traj.back().y);
    std::string junk;
    int force_remaining = 0;
    if (zid != -1)
    {
        monster *targ = &_active_monsters[zid];
        if (stun > 0)
        {
            targ->add_effect(ME_STUNNED, stun);
            add_msg(ngettext("%s was stunned for %d turn!",
                             "%s was stunned for %d turns!", stun),
                    targ->name().c_str(), stun);
        }
        for(int i = 1; i < traj.size(); i++)
        {
            if (m.move_cost(traj[i].x, traj[i].y) == 0 && !m.has_flag("LIQUID", traj[i].x, traj[i].y)) // oops, we hit a wall!
            {
                targ->setpos(traj[i-1]);
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
                targ->setpos(traj[i-1]);
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
            targ->setpos(traj[i]);
            if(m.has_flag("LIQUID", targ->posx(), targ->posy()) && !targ->can_drown() && !targ->dead)
            {
                targ->hurt(9999);
                if (u_see(targ))
                    add_msg(_("The %s drowns!"), targ->name().c_str());
            }
            if(!m.has_flag("LIQUID", targ->posx(), targ->posy()) && targ->has_flag(MF_AQUATIC) && !targ->dead)
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
            if (m.move_cost(traj[i].x, traj[i].y) == 0 && !m.has_flag("LIQUID", traj[i].x, traj[i].y)) // oops, we hit a wall!
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
                    if (one_in(2)) targ->hit(this, bp_torso, -1, force_remaining*dam_mult, 0);
                    if (one_in(2)) targ->hit(this, bp_head, -1, force_remaining*dam_mult, 0);
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
            if (m.move_cost(traj[i].x, traj[i].y) == 0 && !m.has_flag("LIQUID", traj[i].x, traj[i].y)) // oops, we hit a wall!
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
                    if (one_in(2)) u.hit(this, bp_torso, -1, force_remaining*dam_mult, 0);
                    if (one_in(2)) u.hit(this, bp_head, -1, force_remaining*dam_mult, 0);
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
            if(m.has_flag("LIQUID", u.posx, u.posy) && force_remaining < 1)
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
     && !u.is_wearing("glasses_bifocal") && !u.has_disease("contacts")) {
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
 MonsterGroupResult spawn_details;
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
      field_id type = fd_null;
      switch (rng(1, 7)) {
       case 1: type = fd_blood; break;
       case 2: type = fd_bile; break;
       case 3:
       case 4: type = fd_slime; break;
       case 5: type = fd_fire; break;
       case 6:
       case 7: type = fd_nuke_gas; break;
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
    m.add_trap(i, j, tr_portal);
    break;
   case 11:
   case 12:
    m.add_trap(i, j, tr_goo);
    break;
   case 13:
   case 14:
   case 15:
    spawn_details = MonsterGroupManager::GetResultFromGroup("GROUP_NETHER");
    invader = monster(GetMType(spawn_details.name), i, j);
    add_zombie(invader);
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
        monster &z = _active_monsters[mondex];
        if (z.has_flag(MF_ELECTRONIC)) {
            z.make_friendly();
        }
        add_msg(_("The %s sparks and begins searching for a target!"), z.name().c_str());
    }
}

void game::emp_blast(int x, int y)
{
 int rn;
 if (m.has_flag("CONSOLE", x, y)) {
  add_msg(_("The %s is rendered non-functional!"), m.tername(x, y).c_str());
  m.ter_set(x, y, t_console_broken);
  return;
 }
// TODO: More terrain effects.
 if ( m.ter(x,y) == t_card_science || m.ter(x,y) == t_card_military ) {
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
 }
 int mondex = mon_at(x, y);
 if (mondex != -1) {
  monster &z = _active_monsters[mondex];
  if (z.has_flag(MF_ELECTRONIC)) {
   // TODO: Add flag to mob instead.
   if (z.type->id == "mon_turret" && one_in(3)) {
     add_msg(_("The %s beeps erratically and deactivates!"), z.name().c_str());
      remove_zombie(mondex);
      m.spawn_item(x, y, "bot_turret", 1, 0, turn);
      m.spawn_item(x, y, "9mm", 1, z.ammo, turn);
   }
   else if (z.type->id == "mon_laserturret" && one_in(3)) {
      add_msg(_("The %s beeps erratically and deactivates!"), z.name().c_str());
      remove_zombie(mondex);
      m.spawn_item(x, y, "bot_laserturret", 1, 0, turn);
   }
   else if (z.type->id == "mon_manhack" && one_in(6)) {
     add_msg(_("The %s flies erratically and drops from the air!"), z.name().c_str());
     remove_zombie(mondex);
     m.spawn_item(x, y, "bot_manhack", 1, 0, turn);
   }
   else {
      add_msg(_("The EMP blast fries the %s!"), z.name().c_str());
      int dam = dice(10, 10);
      if (z.hurt(dam))
        kill_mon(mondex); // TODO: Player's fault?
      else if (one_in(6))
        z.make_friendly();
    }
  } else
   add_msg(_("The %s is unaffected by the EMP blast."), z.name().c_str());
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

int game::npc_at(const int x, const int y) const
{
 for (int i = 0; i < active_npc.size(); i++) {
  if (active_npc[i]->posx == x && active_npc[i]->posy == y && !active_npc[i]->dead)
   return i;
 }
 return -1;
}

int game::npc_by_id(const int id) const
{
 for (int i = 0; i < active_npc.size(); i++) {
  if (active_npc[i]->getID() == id)
   return i;
 }
 return -1;
}

bool game::add_zombie(monster& m)
{
    if (m.type->id == "mon_null"){ // Don't wanna spawn null monsters o.O
        return false;
    }
    if (-1 != mon_at(m.pos())) {
        debugmsg("add_zombie: there's already a monster at %d,%d", m.posx(), m.posy());
        return false;
    }
    z_at[point(m.posx(), m.posy())] = _active_monsters.size();
    _active_monsters.push_back(m);
    return true;
}

size_t game::num_zombies() const
{
    return _active_monsters.size();
}

monster& game::zombie(const int idx)
{
    return _active_monsters[idx];
}

bool game::update_zombie_pos(const monster &m, const int newx, const int newy)
{
    bool success = false;
    const int zid = mon_at(m.posx(), m.posy());
    const int newzid = mon_at(newx, newy);
    if (newzid >= 0 && !_active_monsters[newzid].dead) {
        debugmsg("update_zombie_pos: new location %d,%d already has zombie %d",
                newx, newy, newzid);
    } else if (zid >= 0) {
        if (&m == &_active_monsters[zid]) {
            z_at.erase(point(m.posx(), m.posy()));
            z_at[point(newx, newy)] = zid;
            success = true;
        } else {
            debugmsg("update_zombie_pos: old location %d,%d had zombie %d instead",
                    m.posx(), m.posy(), zid);
        }
    } else {
        // We're changing the x/y coordinates of a zombie that hasn't been added
        // to the game yet. add_zombie() will update z_at for us.
        debugmsg("update_zombie_pos: no such zombie at %d,%d (moving to %d,%d)",
                m.posx(), m.posy(), newx, newy);
    }
    return success;
}

void game::remove_zombie(const int idx)
{
    monster& m = _active_monsters[idx];
    const point oldloc(m.posx(), m.posy());
    const std::map<point, int>::const_iterator i = z_at.find(oldloc);
    const int prev = (i == z_at.end() ? -1 : i->second);

    if (prev == idx) {
        z_at.erase(oldloc);
    }

    _active_monsters.erase(_active_monsters.begin() + idx);

    // Fix indices in z_at for any zombies that were just moved down 1 place.
    for (std::map<point, int>::iterator iter = z_at.begin(); iter != z_at.end(); ++iter) {
        if (iter->second > idx) {
            --iter->second;
        }
    }
}

void game::clear_zombies()
{
    _active_monsters.clear();
    z_at.clear();
}

/**
 * Attempts to spawn a hallucination somewhere close to the player. Returns
 * false if the hallucination couldn't be spawned for whatever reason, such as
 * a monster already in the target square.
 * @return Whether or not a hallucination was successfully spawned.
 */
bool game::spawn_hallucination()
{
  monster phantasm(MonsterGenerator::generator().get_valid_hallucination());
  phantasm.hallucination = true;
  phantasm.spawn(u.posx + rng(-10, 10), u.posy + rng(-10, 10));

  //Don't attempt to place phantasms inside of other monsters
  if (mon_at(phantasm.posx(), phantasm.posy()) == -1) {
    return add_zombie(phantasm);
  } else {
    return false;
  }
}

int game::mon_at(const int x, const int y) const
{
    std::map<point, int>::const_iterator i = z_at.find(point(x, y));
    if (i != z_at.end()) {
        const int zid = i->second;
        if (!_active_monsters[zid].dead) {
            return zid;
        }
    }
    return -1;
}

int game::mon_at(point p) const
{
    return mon_at(p.x, p.y);
}

void game::rebuild_mon_at_cache()
{
    z_at.clear();
    for (int i = 0, numz = num_zombies(); i < numz; i++) {
        monster &m = _active_monsters[i];
        z_at[point(m.posx(), m.posy())] = i;
    }
}

bool game::is_empty(const int x, const int y)
{
 return ((m.move_cost(x, y) > 0 || m.has_flag("LIQUID", x, y)) &&
         npc_at(x, y) == -1 && mon_at(x, y) == -1 &&
         (u.posx != x || u.posy != y));
}

bool game::is_in_sunlight(int x, int y)
{
 return (m.is_outside(x, y) && light_level() >= 40 &&
         (weather == WEATHER_CLEAR || weather == WEATHER_SUNNY));
}

bool game::is_in_ice_lab(point location)
{
    oter_id cur_ter = cur_om->ter(location.x, location.y, levz);
    bool is_in_ice_lab = false;

    if (cur_ter == "ice_lab"      || cur_ter == "ice_lab_stairs" ||
        cur_ter == "ice_lab_core" || cur_ter == "ice_lab_finale") {
        is_in_ice_lab = true;
    }

    return is_in_ice_lab;
}

void game::kill_mon(int index, bool u_did_it)
{
 if (index < 0 || index >= num_zombies()) {
  dbg(D_ERROR) << "game:kill_mon: Tried to kill monster " << index
               << "! (" << num_zombies() << " in play)";
  if (debugmon)  debugmsg("Tried to kill monster %d! (%d in play)", index, num_zombies());
  return;
 }
 monster &z = _active_monsters[index];
 if (!z.dead) {
  z.dead = true;
  if (u_did_it) {
   if (z.has_flag(MF_GUILT)) {
    mdeath tmpdeath;
    tmpdeath.guilt(&z);
   }
   if (!z.is_hallucination()) {
    kills[z.type->id]++; // Increment our kill counter
   }
  }
  for (int i = 0; i < z.inv.size(); i++)
   m.add_item_or_charges(z.posx(), z.posy(), z.inv[i]);
  z.die(this);
 }
}

void game::explode_mon(int index)
{
 if (index < 0 || index >= num_zombies()) {
  dbg(D_ERROR) << "game:explode_mon: Tried to explode monster " << index
               << "! (" << num_zombies() << " in play)";
  debugmsg("Tried to explode monster %d! (%d in play)", index, num_zombies());
  return;
 }
 monster &z = _active_monsters[index];
 if(z.is_hallucination()) {
   //Can't gib hallucinations
   return;
 }
 if (!z.dead) {
  z.dead = true;
  kills[z.type->id]++; // Increment our kill counter
// Send body parts and blood all over!
  mtype* corpse = z.type;
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

   int posx = z.posx(), posy = z.posy();
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
    m.spawn_item(tarx, tary, meat, 1, 0, turn);
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
    add_zombie(mon);
}

void game::open()
{
    int openx, openy;
    if (!choose_adjacent(_("Open where?"), openx, openy))
        return;

    u.moves -= 100;
    bool didit = false;

    int vpart;
    vehicle *veh = m.veh_at(openx, openy, vpart);
    if (veh) {
        int openable = veh->part_with_feature(vpart, "OPENABLE");
        if(openable >= 0) {
            const char *name = veh->part_info(openable).name.c_str();
            if (veh->part_info(openable).has_flag("OPENCLOSE_INSIDE")){
                const vehicle *in_veh = m.veh_at(u.posx, u.posy);
                if (!in_veh || in_veh != veh){
                    add_msg(_("That %s can only opened from the inside."), name);
                    return;
                }
            }
            if (veh->parts[openable].open) {
                add_msg(_("That %s is already open."), name);
                u.moves += 100;
            } else {
                veh->open(openable);
            }
        }
        return;
    }

    if (m.is_outside(u.posx, u.posy))
        didit = m.open_door(openx, openy, false);
    else
        didit = m.open_door(openx, openy, true);

    if (!didit) {
        const std::string terid = m.get_ter(openx, openy);
        if ( terid.find("t_door") != std::string::npos ) {
            if ( terid.find("_locked") != std::string::npos ) {
                add_msg(_("The door is locked!"));
                return;
            } else if ( termap[ terid ].close.size() > 0 && termap[ terid ].close != "t_null" ) {
                // if the following message appears unexpectedly, the prior check was for t_door_o
                add_msg(_("That door is already open."));
                u.moves += 100;
                return;
            }
        }
        add_msg(_("No door there."));
        u.moves += 100;
    }
}

void game::close(int closex, int closey)
{
    if (closex == -1) {
        if (!choose_adjacent(_("Close where?"), closex, closey)) {
            return;
        }
    }

    bool didit = false;

    int vpart;
    vehicle *veh = m.veh_at(closex, closey, vpart);
    int zid = mon_at(closex, closey);
    if (zid != -1) {
        monster &z = _active_monsters[zid];
        add_msg(_("There's a %s in the way!"), z.name().c_str());
    }
    else if (veh) {
        int openable = veh->part_with_feature(vpart, "OPENABLE");
        if(openable >= 0) {
            const char *name = veh->part_info(openable).name.c_str();
            if (veh->part_info(openable).has_flag("OPENCLOSE_INSIDE")){
                const vehicle *in_veh = m.veh_at(u.posx, u.posy);
                if (!in_veh || in_veh != veh){
                    add_msg(_("That %s can only closed from the inside."), name);
                    return;
                }
            }
            if (veh->parts[openable].open) {
                veh->close(openable);
                didit = true;
            } else {
                add_msg(_("That %s is already closed."), name);
            }
        }
    } else if (m.furn(closex, closey) != f_safe_o && m.i_at(closex, closey).size() > 0)
        add_msg(_("There's %s in the way!"), m.i_at(closex, closey).size() == 1 ?
                m.i_at(closex, closey)[0].tname(this).c_str() : _("some stuff"));
    else if (closex == u.posx && closey == u.posy)
        add_msg(_("There's some buffoon in the way!"));
    else if (m.ter(closex, closey) == t_window_domestic &&
             m.is_outside(u.posx, u.posy))  {
        add_msg(_("You cannot close the curtains from outside. You must be inside the building."));
    } else if (m.has_furn(closex, closey) && m.furn_at(closex, closey).close.size() == 0 ) {
       add_msg(_("There's a %s in the way!"), m.furnname(closex, closey).c_str());
    } else
        didit = m.close_door(closex, closey, true, false);

    if (didit)
        u.moves -= 90;
}

void game::smash()
{
    const int move_cost = int(u.weapon.is_null() ? 80 : u.weapon.attack_time() * 0.8);
    bool didit = false;
    std::string bashsound, extra;
    int smashskill = int(u.str_cur / 2.5 + u.weapon.type->melee_dam);
    int smashx, smashy;

    if (!choose_adjacent(_("Smash where?"), smashx, smashy))
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
    if (!corpses.empty())
    {
        const int num_corpses = corpses.size();

        // numbers logic: a str 8 character with a butcher knife (4 bash, 18 cut)
        // should have at least a 50% chance of damaging an intact zombie corpse (75 volume).
        // a str 8 character with a baseball bat (28 bash, 0 cut) should have around a 25% chance.

        int cut_power = u.weapon.type->melee_cut;
        // stabbing weapons are a lot less effective at pulping
        if (u.weapon.has_flag("STAB") || u.weapon.has_flag("SPEAR")) {
            cut_power /= 2;
        }
        double pulp_power = sqrt((double)(u.str_cur + u.weapon.type->melee_dam)) *
                            sqrt((double)(cut_power + 1));
        pulp_power = std::min(pulp_power, (double)u.str_cur);
        pulp_power *= 20; // constant multiplier to get the chance right
        int smashes = 0;
        while( !corpses.empty() ) {
            item *it = corpses.front();
            corpses.pop_front();
            int damage = pulp_power / it->volume();
            do {
                smashes++;

                // Increase damage as we keep smashing,
                // to insure that we eventually smash the target.
                if (x_in_y(pulp_power, it->volume())) {
                damage++;
            }

                it->damage += damage;
                // Splatter some blood around
                for (int x = smashx - 1; x <= smashx + 1; x++) {
                    for (int y = smashy - 1; y <= smashy + 1; y++) {
                        if (!one_in(damage+1)) {
                             m.add_field(this, x, y, fd_blood, 1);
                        }
                    }
                }
                if (it->damage >= full_pulp_threshold) {
                    it->damage = full_pulp_threshold;
                    // TODO mark corpses as inactive when appropriate
            }
            } while( it->damage < full_pulp_threshold );
        }

        // TODO: Factor in how long it took to do the smashing.
        add_msg(ngettext("The corpse is thoroughly pulped.",
                         "The corpses are thoroughly pulped.", num_corpses));

        u.moves -= smashes * move_cost;
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
        if (m.has_flag("ALARMED", smashx, smashy) &&
            !event_queued(EVENT_WANTED))
        {
            sound(smashx, smashy, 40, _("An alarm sounds!"));
            u.add_memorial_log(_("Set off an alarm."));
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
                u.hit(this, bp_hands, 0, 0, rng(0, long(u.weapon.volume() * .5)));
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
  ch = inv_activatable(_("Use item:"));
 else
  ch = chInput;

 if (ch == ' ' || ch == KEY_ESCAPE) {
  add_msg(_("Never mind."));
  return;
 }
 last_action += ch;
 refresh_all();
 u.use(this, ch);
}

void game::use_wielded_item()
{
  u.use_wielded(this);
}

bool game::choose_adjacent(std::string message, int &x, int &y)
{
    //~ appended to "Close where?" "Pry where?" etc.
    std::string query_text = message + _(" (Direction button)");
    mvwprintw(w_terrain, 0, 0, query_text.c_str());
    wrefresh(w_terrain);
    DebugLog() << "calling get_input() for " << message << "\n";
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

bool game::refill_vehicle_part (vehicle &veh, vehicle_part *part, bool test)
{
  vpart_info part_info = vehicle_part_types[part->id];
  if (!part_info.has_flag("FUEL_TANK")) {
    return false;
  }
  item* it = NULL;
  item *p_itm = NULL;
  int min_charges = -1;
  bool in_container = false;

  std::string ftype = part_info.fuel_type;
  itype_id itid = default_ammo(ftype);
  if (u.weapon.is_container() && u.weapon.contents.size() > 0 &&
          u.weapon.contents[0].type->id == itid) {
    it = &u.weapon;
    p_itm = &u.weapon.contents[0];
    min_charges = u.weapon.contents[0].charges;
    in_container = true;
  } else if (u.weapon.type->id == itid) {
    it = &u.weapon;
    p_itm = it;
    min_charges = u.weapon.charges;
  } else {
    it = &u.inv.item_or_container(itid);
    if (!it->is_null()) {
      if (it->type->id == itid) {
        p_itm = it;
      } else {
        //ah, must be a container of the thing
        p_itm = &(it->contents[0]);
        in_container = true;
      }
      min_charges = p_itm->charges;
    }
  }
  if (it->is_null()) {
    return false;
  } else if (test) {
    return true;
  }

  int fuel_per_charge = 1; //default for gasoline
  if (ftype == "plutonium") {
    fuel_per_charge = 1000;
  } else if (ftype == "plasma") {
    fuel_per_charge = 100;
  }
  int max_fuel = part_info.size;
  int charge_difference = (max_fuel - part->amount) / fuel_per_charge;
  if (charge_difference < 1) {
    charge_difference = 1;
  }
  bool rem_itm = min_charges <= charge_difference;
  int used_charges = rem_itm ? min_charges : charge_difference;
  part->amount += used_charges * fuel_per_charge;
  if (part->amount > max_fuel) {
    part->amount = max_fuel;
  }

  if (ftype == "battery") {
    add_msg(_("You recharge %s's battery."), veh.name.c_str());
    if (part->amount == max_fuel) {
      add_msg(_("The battery is fully charged."));
    }
  } else if (ftype == "gasoline") {
    add_msg(_("You refill %s's fuel tank."), veh.name.c_str());
    if (part->amount == max_fuel) {
      add_msg(_("The tank is full."));
    }
  } else if (ftype == "plutonium") {
    add_msg(_("You refill %s's reactor."), veh.name.c_str());
    if (part->amount == max_fuel) {
      add_msg(_("The reactor is full."));
    }
  }

  p_itm->charges -= used_charges;
  if (rem_itm) {
    if (in_container) {
      it->contents.erase(it->contents.begin());
    } else if (&u.weapon == it) {
      u.remove_weapon();
    } else {
      u.inv.remove_item_by_letter(it->invlet);
    }
  }
  return true;
}

bool game::pl_refill_vehicle (vehicle &veh, int part, bool test)
{
  return refill_vehicle_part(veh, &veh.parts[part], test);
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
    vehint.ddx = cx;
    vehint.ddy = cy;
    vehint.exec(this, &veh, examx, examy);
    if (vehint.sel_cmd != ' ')
    {                                                        // TODO: different activity times
        u.activity = player_activity(ACT_VEHICLE,
                                     vehint.sel_cmd == 'f' || vehint.sel_cmd == 's' ||
                                     vehint.sel_cmd == 'c' ? 200 : 20000,
                                     (int) vehint.sel_cmd, 0, "");
        u.activity.values.push_back (veh.global_x());    // values[0]
        u.activity.values.push_back (veh.global_y());    // values[1]
        u.activity.values.push_back (vehint.ddx);   // values[2]
        u.activity.values.push_back (vehint.ddy);   // values[3]
        u.activity.values.push_back (-vehint.ddx);   // values[4]
        u.activity.values.push_back (-vehint.ddy);   // values[5]
        u.activity.values.push_back (veh.index_of_part(vehint.sel_vehicle_part)); // values[6]
        u.activity.values.push_back (vehint.sel_type); // int. might make bitmask
        if(vehint.sel_vpart_info != NULL) {
          u.activity.str_values.push_back(vehint.sel_vpart_info->id);
        } else {
          u.activity.str_values.push_back("null");
        }
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
void game::open_gate( game *g, const int examx, const int examy, const ter_id handle_type ) {

 ter_id v_wall_type;
 ter_id h_wall_type;
 ter_id door_type;
 ter_id floor_type;
 const char *pull_message;
 const char *open_message;
 const char *close_message;

 if ( handle_type == t_gates_mech_control ) {
  v_wall_type = t_wall_v;
  h_wall_type = t_wall_h;
  door_type   = t_door_metal_locked;
  floor_type  = t_floor;
  pull_message = _("You turn the handle...");
  open_message = _("The gate is opened!");
  close_message = _("The gate is closed!");
 } else if ( handle_type == t_gates_control_concrete ) {
  v_wall_type = t_concrete_v;
  h_wall_type = t_concrete_h;
  door_type   = t_door_metal_locked;
  floor_type  = t_floor;
  pull_message = _("You turn the handle...");
  open_message = _("The gate is opened!");
  close_message = _("The gate is closed!");

 } else if ( handle_type == t_barndoor ) {
  v_wall_type = t_wall_wood;
  h_wall_type = t_wall_wood;
  door_type   = t_door_metal_locked;
  floor_type  = t_dirtfloor;
  pull_message = _("You pull the rope...");
  open_message = _("The barn doors opened!");
  close_message = _("The barn doors closed!");

 } else if ( handle_type == t_palisade_pulley ) {
  v_wall_type = t_palisade;
  h_wall_type = t_palisade;
  door_type   = t_palisade_gate;
  floor_type  = t_palisade_gate_o;
  pull_message = _("You pull the rope...");
  open_message = _("The palisade gate swings open!");
  close_message = _("The palisade gate swings closed with a crash!");
 } else {
   return;
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
    m.unboard_vehicle(u.posx, u.posy);
    u.moves -= 200;
    // Dive three tiles in the direction of tox and toy
    fling_player_or_monster(&u, 0, d, 30, true);
    // Hit the ground according to vehicle speed
    if (!m.has_flag("SWIMMABLE", u.posx, u.posy)) {
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
        veh->use_controls();
    } else if (veh && veh->part_with_feature(veh_part, "CONTROLS") >= 0
                   && u.in_vehicle) {
        u.controlling_vehicle = true;
        add_msg(_("You take control of the %s."), veh->name.c_str());
    } else {
        int examx, examy;
        if (!choose_adjacent(_("Control vehicle where?"), examx, examy))
            return;
        veh = m.veh_at(examx, examy, veh_part);
        if (!veh) {
            add_msg(_("No vehicle there."));
            return;
        }
        if (veh->part_with_feature(veh_part, "CONTROLS") < 0) {
            add_msg(_("No controls there."));
            return;
        }
        veh->use_controls();
    }
}

void game::examine(int examx, int examy)
{
    if (examx == -1) {
        if (!choose_adjacent(_("Examine where?"), examx, examy)) {
            return;
        }
    }

    int veh_part = 0;
    vehicle *veh = m.veh_at (examx, examy, veh_part);
    if (veh) {
        int vpcargo = veh->part_with_feature(veh_part, "CARGO", false);
        int vpkitchen = veh->part_with_feature(veh_part, "KITCHEN", true);
        int vpweldrig = veh->part_with_feature(veh_part, "WELDRIG", true);
        int vpcraftrig = veh->part_with_feature(veh_part, "CRAFTRIG", true);
        if ((vpcargo >= 0 && veh->parts[vpcargo].items.size() > 0)
                || vpkitchen >= 0 || vpweldrig >=0 || vpcraftrig >=0) {
            pickup(examx, examy, 0);
        } else if (u.controlling_vehicle) {
            add_msg (_("You can't do that while driving."));
        } else if (abs(veh->velocity) > 0) {
            add_msg (_("You can't do that on moving vehicle."));
        } else {
            exam_vehicle (*veh, examx, examy);
        }
    }

 if (m.has_flag("CONSOLE", examx, examy)) {
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

 if (m.has_flag("SEALED", examx, examy)) {
   if (none) add_msg(_("The %s is firmly sealed."), m.name(examx, examy).c_str());
 } else {
   //examx,examy has no traps, is a container and doesn't have a special examination function
  if (m.tr_at(examx, examy) == tr_null && m.i_at(examx, examy).size() == 0 && m.has_flag("CONTAINER", examx, examy) && none)
   add_msg(_("It is empty."));
  else
   if (!veh)pickup(examx, examy, 0);
 }
  //check for disarming traps last to avoid disarming query black box issue.
 if(m.tr_at(examx, examy) != tr_null) xmine.trap(this,&u,&m,examx,examy);

}

void game::advanced_inv()
{
    advanced_inventory advinv;
    advinv.display(this, &u);
}

//Shift player by one tile, look_around(), then restore previous position.
//represents carfully peeking around a corner, hence the large move cost.
void game::peek()
{
    int prevx, prevy, peekx, peeky;

    if (!choose_adjacent(_("Peek where?"), peekx, peeky))
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
point game::look_debug() {
  editmap * edit=new editmap(this);
  point ret=edit->edit();
  delete edit;
  edit=0;
  return ret;
}
////////////////////////////////////////////////////////////////////////////////////////////

void game::print_all_tile_info(int lx, int ly, WINDOW* w_look, int column, int &line, bool mouse_hover)
{
    print_terrain_info(lx, ly, w_look, column, line);
    print_fields_info(lx, ly, w_look, column, line);
    print_trap_info(lx, ly, w_look, column, line);
    print_object_info(lx, ly, w_look, column, line, mouse_hover);
}

void game::print_terrain_info(int lx, int ly, WINDOW* w_look, int column, int &line)
{
    int ending_line = line + 3;
    std::string tile = m.tername(lx, ly);
    if (m.has_furn(lx, ly)) {
        tile += "; " + m.furnname(lx, ly);
    }

    if (m.move_cost(lx, ly) == 0) {
        mvwprintw(w_look, line, column, _("%s; Impassable"), tile.c_str());
    } else {
        mvwprintw(w_look, line, column, _("%s; Movement cost %d"), tile.c_str(),
            m.move_cost(lx, ly) * 50);
    }
    mvwprintw(w_look, ++line, column, "%s", m.features(lx, ly).c_str());
    if (line < ending_line) {
        line = ending_line;
    }
}

void game::print_fields_info(int lx, int ly, WINDOW* w_look, int column, int &line)
{
    field &tmpfield = m.field_at(lx, ly);
    if (tmpfield.fieldCount() == 0) {
        return;
    }

    field_entry *cur = NULL;
    typedef std::map<field_id, field_entry*>::iterator field_iterator;
    for (field_iterator it = tmpfield.getFieldStart(); it != tmpfield.getFieldEnd(); ++it) {
        cur = it->second;
        if (cur == NULL) {
            continue;
        }
        mvwprintz(w_look, line++, column, fieldlist[cur->getFieldType()].color[cur->getFieldDensity()-1], "%s",
            fieldlist[cur->getFieldType()].name[cur->getFieldDensity()-1].c_str());
    }
}

void game::print_trap_info(int lx, int ly, WINDOW* w_look, const int column, int &line)
{
    trap_id trapid = m.tr_at(lx, ly);
    if (trapid == tr_null) {
        return;
    }

    int vis = traps[trapid]->visibility;
    if (vis == -1 || u.per_cur - u.encumb(bp_eyes) >= vis) {
        mvwprintz(w_look, line++, column, traps[trapid]->color, "%s", traps[trapid]->name.c_str());
    }
}

void game::print_object_info(int lx, int ly, WINDOW* w_look, const int column, int &line, bool mouse_hover)
{
    int veh_part = 0;
    vehicle *veh = m.veh_at(lx, ly, veh_part);
    int dex = mon_at(lx, ly);
    if (dex != -1 && u_see(&zombie(dex)))
    {
        if (!mouse_hover) {
            zombie(dex).draw(w_terrain, lx, ly, true);
        }
        line = zombie(dex).print_info(this, w_look, line, 6, column);
        handle_multi_item_info(lx, ly, w_look, column, line, mouse_hover);
    }
    else if (npc_at(lx, ly) != -1)
    {
        if (!mouse_hover) {
            active_npc[npc_at(lx, ly)]->draw(w_terrain, lx, ly, true);
        }
        line = active_npc[npc_at(lx, ly)]->print_info(w_look, column, line);
        handle_multi_item_info(lx, ly, w_look, column, line, mouse_hover);
    }
    else if (veh)
    {
        mvwprintw(w_look, line++, column, _("There is a %s there. Parts:"), veh->name.c_str());
        line = veh->print_part_desc(w_look, line, (mouse_hover) ? getmaxx(w_look) : 48, veh_part);
        if (!mouse_hover) {
            m.drawsq(w_terrain, u, lx, ly, true, true, lx, ly);
        }
    }
    else if (!m.has_flag("CONTAINER", lx, ly) && m.i_at(lx, ly).size() > 0)
    {
        if (!mouse_hover) {
            mvwprintw(w_look, line++, column, _("There is a %s there."),
                m.i_at(lx, ly)[0].tname(this).c_str());
            if (m.i_at(lx, ly).size() > 1)
            {
                mvwprintw(w_look, line++, column, _("There are other items there as well."));
            }
            m.drawsq(w_terrain, u, lx, ly, true, true, lx, ly);
        }
    } else if (m.has_flag("CONTAINER", lx, ly)) {
        mvwprintw(w_look, line++, column, _("You cannot see what is inside of it."));
        if (!mouse_hover) {
            m.drawsq(w_terrain, u, lx, ly, true, false, lx, ly);
        }
    }
    // The player is not at <u.posx + u.view_offset_x, u.posy + u.view_offset_y>
    // Should not be putting the "You (name)" at this location
    // Changing it to reflect actual position not view-center position
    else if (lx == u.posx && ly == u.posy )
    {
        int x,y;
        x = getmaxx(w_terrain)/2 - u.view_offset_x;
        y = getmaxy(w_terrain)/2 - u.view_offset_y;
        if (!mouse_hover) {
            mvwputch_inv(w_terrain, y, x, u.color(), '@');
        }

        mvwprintw(w_look, line++, column, _("You (%s)"), u.name.c_str());
        if (veh) {
            mvwprintw(w_look, line++, column, _("There is a %s there. Parts:"), veh->name.c_str());
            line = veh->print_part_desc(w_look, line, (mouse_hover) ? getmaxx(w_look) : 48, veh_part);
            if (!mouse_hover) {
                m.drawsq(w_terrain, u, lx, ly, true, true, lx, ly);
            }
        }

    }
    else if (!mouse_hover)
    {
        m.drawsq(w_terrain, u, lx, ly, true, true, lx, ly);
    }
}

void game::handle_multi_item_info(int lx, int ly, WINDOW* w_look, const int column, int &line, bool mouse_hover)
{
    if (!m.has_flag("CONTAINER", lx, ly))
    {
        if (!mouse_hover) {
            if (m.i_at(lx, ly).size() > 1) {
                mvwprintw(w_look, line++, column, _("There are several items there."));
            } else if (m.i_at(lx, ly).size() == 1) {
                mvwprintw(w_look, line++, column, _("There is an item there."));
            }
        }
    } else {
        mvwprintw(w_look, line++, column, _("You cannot see what is inside of it."));
    }
}


void game::get_lookaround_dimensions(int &lookWidth, int &begin_y, int &begin_x) const
{
    lookWidth = getmaxx(w_messages);
    begin_y = TERMY - lookHeight + 1;
    if (getbegy(w_messages) < begin_y) {
        begin_y = getbegy(w_messages);
    }
    begin_x = getbegx(w_messages);
}


point game::look_around()
{
 draw_ter();
 int lx = u.posx + u.view_offset_x, ly = u.posy + u.view_offset_y;
 std::string action;
 bool fast_scroll = false;
 int soffset = (int) OPTIONS["MOVE_VIEW_OFFSET"];

 int lookWidth, lookY, lookX;
 get_lookaround_dimensions(lookWidth, lookY, lookX);
 WINDOW* w_look = newwin(lookHeight, lookWidth, lookY, lookX);
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
  int off = 1;
  if (u_see(lx, ly)) {
      print_all_tile_info(lx, ly, w_look, 1, off, false);

  } else if (u.sight_impaired() &&
              m.light_at(lx, ly) == LL_BRIGHT &&
              rl_dist(u.posx, u.posy, lx, ly) < u.unimpaired_range() &&
              m.sees(u.posx, u.posy, lx, ly, u.unimpaired_range(), junk))
  {
   if (u.has_disease("boomered"))
    mvwputch_inv(w_terrain, POSY + (ly - u.posy), POSX + (lx - u.posx), c_pink, '#');
   else
    mvwputch_inv(w_terrain, POSY + (ly - u.posy), POSX + (lx - u.posx), c_ltgray, '#');
   mvwprintw(w_look, 1, 1, _("Bright light."));
  } else {
   mvwputch(w_terrain, POSY, POSX, c_white, 'x');
   mvwprintw(w_look, 1, 1, _("Unseen."));
  }

  if (fast_scroll) {
      // print a light green mark below the top right corner of the w_look window
      mvwprintz(w_look, 1, lookWidth-1, c_ltgreen, _("F"));
  } else {
      // redraw the border to clear out the marker.
      wborder(w_look, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
          LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
  }

  if (m.graffiti_at(lx, ly).contents)
   mvwprintw(w_look, ++off + 1, 1, _("Graffiti: %s"), m.graffiti_at(lx, ly).contents->c_str());
  //mvwprintw(w_look, 5, 1, _("Maploc: <%d,%d>"), lx, ly);
  wrefresh(w_look);
  wrefresh(w_terrain);

  DebugLog() << __FUNCTION__ << ": calling handle_input() \n";

    input_context ctxt("LOOK");
    ctxt.register_directions();
    ctxt.register_action("COORDINATE");
    ctxt.register_action("SELECT");
    ctxt.register_action("CONFIRM");
    ctxt.register_action("QUIT");
    ctxt.register_action("TOGGLE_FAST_SCROLL");
    action = ctxt.handle_input();

    if (!u_see(lx, ly))
        mvwputch(w_terrain, POSY + (ly - u.posy), POSX + (lx - u.posx), c_black, ' ');

    // Our coordinates will either be determined by coordinate input(mouse),
    // by a direction key, or by the previous value.
    if (action == "TOGGLE_FAST_SCROLL") {
        fast_scroll = !fast_scroll;
    } else if (!ctxt.get_coordinates(g->w_terrain, lx, ly)) {
        int dx, dy;
        ctxt.get_direction(dx, dy, action);
        if (dx == -2) {
            dx = 0;
            dy = 0;
        } else {
            if (fast_scroll) {
                dx *= soffset;
                dy *= soffset;
            }
        }
        lx += dx;
        ly += dy;
    }
 } while (action != "QUIT" && action != "CONFIRM");

 werase(w_look);
 delwin(w_look);
 if (action == "CONFIRM")
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

std::vector<map_item_stack> game::find_nearby_items(int iRadius)
{
    std::vector<item> here;
    std::map<std::string, map_item_stack> temp_items;
    std::vector<map_item_stack> ret;
    std::vector<std::string> vOrder;

    std::vector<point> points = closest_points_first(iRadius, u.posx, u.posy);

    int iLastX = 0;
    int iLastY = 0;

    for (std::vector<point>::iterator p_it = points.begin(); p_it != points.end(); ++p_it) {
        if (p_it->y >= u.posy - iRadius && p_it->y <= u.posy + iRadius &&
            u_see(p_it->x,p_it->y) &&
            (!m.has_flag("CONTAINER", p_it->x, p_it->y) ||
            (rl_dist(u.posx, u.posy, p_it->x, p_it->y) == 1 && !m.has_flag("SEALED", p_it->x, p_it->y)))) {

            here.clear();
            here = m.i_at(p_it->x, p_it->y);
            for (int i = 0; i < here.size(); i++) {
                const std::string name = here[i].tname(this);

                if (temp_items.find(name) == temp_items.end() || (iLastX != p_it->x || iLastY != p_it->y)) {
                    iLastX = p_it->x;
                    iLastY = p_it->y;

                    if (std::find(vOrder.begin(), vOrder.end(), name) == vOrder.end()) {
                        vOrder.push_back(name);
                        temp_items[name] = map_item_stack(here[i], p_it->x - u.posx, p_it->y - u.posy);
                    } else {
                        temp_items[name].addNewPos(p_it->x - u.posx, p_it->y - u.posy);
                    }

                } else {
                    temp_items[name].incCount();
                }
            }
        }
    }

    for (int i=0; i < vOrder.size(); i++) {
        ret.push_back(temp_items[vOrder[i]]);
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


void game::draw_trail_to_square(int x, int y, bool bDrawX)
{
    //Reset terrain
    draw_ter();

    //Draw trail
    point center = point(u.posx + u.view_offset_x, u.posy + u.view_offset_y);
    std::vector<point> vPoint = line_to(u.posx, u.posy, u.posx + x, u.posy + y, 0);

    draw_line(u.posx + x, u.posy + y, center, vPoint);
    if (bDrawX) {
        mvwputch(w_terrain, POSY + (vPoint[vPoint.size()-1].y - (u.posy + u.view_offset_y)),
                            POSX + (vPoint[vPoint.size()-1].x - (u.posx + u.view_offset_x)), c_white, 'X');
    }

    wrefresh(w_terrain);
}

//helper method so we can keep list_items shorter
void game::reset_item_list_state(WINDOW* window, int height)
{
    const int width = use_narrow_sidebar() ? 45 : 55;
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

    mvwprintz(window, 0, 2, c_ltgreen, "< ");
    wprintz(window, c_white, _("Items"));
    wprintz(window, c_ltgreen, " >");

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

void centerlistview(game *g, int iActiveX, int iActiveY)
{
    player & u = g->u;
    if (OPTIONS["SHIFT_LIST_ITEM_VIEW"] != "false") {
        int xpos = POSX + iActiveX;
        int ypos = POSY + iActiveY;
        if (OPTIONS["SHIFT_LIST_ITEM_VIEW"] == "centered") {
            int xOffset = TERRAIN_WINDOW_WIDTH / 2;
            int yOffset = TERRAIN_WINDOW_HEIGHT / 2;
            if ( xpos < 0 || xpos >= TERRAIN_WINDOW_WIDTH ||
                 ypos < 0 || ypos >= TERRAIN_WINDOW_HEIGHT ) {
                if (xpos < 0) {
                    u.view_offset_x = xpos - xOffset;
                } else {
                    u.view_offset_x = xpos - (TERRAIN_WINDOW_WIDTH - 1) + xOffset;
                }
                if (xpos < 0) {
                    u.view_offset_y = ypos - yOffset;
                } else {
                    u.view_offset_y = ypos - (TERRAIN_WINDOW_HEIGHT - 1) + yOffset;
                }
            } else {
                u.view_offset_x = 0;
                u.view_offset_y = 0;
            }
        } else {
            if (xpos < 0) {
                u.view_offset_x = xpos;
            } else if (xpos >= TERRAIN_WINDOW_WIDTH) {
                u.view_offset_x = xpos - (TERRAIN_WINDOW_WIDTH - 1);
            } else {
                u.view_offset_x = 0;
            }
            if (ypos < 0) {
                u.view_offset_y = ypos;
            } else if (ypos >= TERRAIN_WINDOW_HEIGHT) {
                u.view_offset_y = ypos - (TERRAIN_WINDOW_HEIGHT - 1);
            } else {
                u.view_offset_y = 0;
            }
        }
    }

}


int game::list_items()
{
    int iInfoHeight = 12;
    const int width = use_narrow_sidebar() ? 45 : 55;
    WINDOW* w_items = newwin(TERMY-iInfoHeight-VIEW_OFFSET_Y*2, width, VIEW_OFFSET_Y, TERRAIN_WINDOW_WIDTH + VIEW_OFFSET_X);
    WINDOW* w_item_info = newwin(iInfoHeight-1, width - 2, TERMY-iInfoHeight-VIEW_OFFSET_Y, TERRAIN_WINDOW_WIDTH+1+VIEW_OFFSET_X);
    WINDOW* w_item_info_border = newwin(iInfoHeight, width, TERMY-iInfoHeight-VIEW_OFFSET_Y, TERRAIN_WINDOW_WIDTH+VIEW_OFFSET_X);

    //Area to search +- of players position.
    const int iRadius = 12 + (u.per_cur * 2);

    //this stores the items found, along with the coordinates
    std::vector<map_item_stack> ground_items = find_nearby_items(iRadius);
    //this stores only those items that match our filter
    std::vector<map_item_stack> filtered_items = (sFilter != "" ?
                                                  filter_item_stacks(ground_items, sFilter) :
                                                  ground_items);
    int highPEnd = list_filter_high_priority(filtered_items,list_item_upvote);
    int lowPStart = list_filter_low_priority(filtered_items,highPEnd,list_item_downvote);
    const int iItemNum = ground_items.size();
    if ( iItemNum > 0 ) {
        uistate.list_item_mon = 1; // remember we've tabbed here
    }
    const int iStoreViewOffsetX = u.view_offset_x;
    const int iStoreViewOffsetY = u.view_offset_y;

    u.view_offset_x = 0;
    u.view_offset_y = 0;

    int iReturn = -1;
    int iActive = 0; // Item index that we're looking at
    const int iMaxRows = TERMY-iInfoHeight-2-VIEW_OFFSET_Y*2;
    int iStartPos = 0;
    int iActiveX = 0;
    int iActiveY = 0;
    int iLastActiveX = -1;
    int iLastActiveY = -1;
    InputEvent input = Undefined;
    long ch = 0; //this is a long because getch returns a long
    bool reset = true;
    bool refilter = true;
    int iFilter = 0;
    int iPage = 0;

    do
    {
        if (ground_items.size() > 0)
        {
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
                iPage = 0;
                iLastActiveX = -1;
                iLastActiveY = -1;
                refilter = false;
            }
            if (reset)
            {
                reset_item_list_state(w_items, iInfoHeight);
                reset = false;
            }

            // we're switching on input here, whereas above it was if/else clauses on a char
            switch(input)
            {
                case DirectionN:
                    iActive--;
                    iPage = 0;
                    if (iActive < 0) {
                        iActive = iItemNum - iFilter - 1;
                    }
                    break;
                case DirectionS:
                    iActive++;
                    iPage = 0;
                    if (iActive >= iItemNum - iFilter) {
                        iActive = 0;
                    }
                    break;
                case DirectionE:
                    iPage++;
                    if ( !filtered_items.empty() && iPage >= filtered_items[iActive].vIG.size()) {
                        iPage = filtered_items[iActive].vIG.size()-1;
                    }
                    break;
                case DirectionW:
                    iPage--;
                    if (iPage < 0) {
                        iPage = 0;
                    }
                    break;
                case Tab: //Switch to list_monsters();
                case DirectionDown:
                case DirectionUp:
                    u.view_offset_x = iStoreViewOffsetX;
                    u.view_offset_y = iStoreViewOffsetY;

                    werase(w_items);
                    werase(w_item_info);
                    werase(w_item_info_border);
                    delwin(w_items);
                    delwin(w_item_info);
                    delwin(w_item_info_border);
                    return 1;
                    break;
                default:
                    break;
            }

            //Draw Scrollbar
            draw_scrollbar(w_items, iActive, iMaxRows, iItemNum - iFilter, 1);

            calcStartPos(iStartPos, iActive, iMaxRows, iItemNum - iFilter);

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
                    int iThisPage = 0;

                    if (iNum == iActive) {
                        iThisPage = iPage;

                        iActiveX = iter->vIG[iThisPage].x;
                        iActiveY = iter->vIG[iThisPage].y;

                        activeItem = iter->example;
                    }

                    sText.str("");

                    if (iter->vIG.size() > 1) {
                        sText << "[" << iThisPage+1 << "/" << iter->vIG.size() << "] (" << iter->totalcount << ") ";
                    }

                    sText << iter->example.tname(this);

                    if (iter->vIG[iThisPage].count > 1) {
                        sText << " [" << iter->vIG[iThisPage].count << "]";
                    }

                    mvwprintz(w_items, 1 + iNum - iStartPos, 2,
                              ((iNum == iActive) ? c_ltgreen : (high ? c_yellow : (low ? c_red : c_white))),
                              "%s", (sText.str()).c_str());
                    int numw = iItemNum > 9 ? 2 : 1;
                    mvwprintz(w_items, 1 + iNum - iStartPos, width - (5 + numw),
                              ((iNum == iActive) ? c_ltgreen : c_ltgray), "%*d %s",
                              numw, trig_dist(0, 0, iter->vIG[iThisPage].x, iter->vIG[iThisPage].y),
                              direction_name_short(direction_from(0, 0, iter->vIG[iThisPage].x, iter->vIG[iThisPage].y)).c_str()
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
                centerlistview(this, iActiveX, iActiveY);
                draw_trail_to_square(iActiveX, iActiveY, true);
            }

            wrefresh(w_items);
            wrefresh(w_item_info_border);
            wrefresh(w_item_info);

            refresh();
            ch = getch();
            input = get_input(ch);
        }
        else
        {
            iReturn = 0;
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

    return iReturn;
}


std::vector<int> game::find_nearby_monsters(int iRadius)
{
    std::vector<int> ret;
    std::vector<point> points = closest_points_first(iRadius, u.posx, u.posy);

    for (std::vector<point>::iterator p_it = points.begin(); p_it != points.end(); ++p_it) {
        int dex = mon_at(p_it->x, p_it->y);
        if (dex != -1 && u_see(&zombie(dex))) {
            ret.push_back(dex);
        }
    }

    return ret;
}

int game::list_monsters()
{
    int iInfoHeight = 12;
    const int width = use_narrow_sidebar() ? 45 : 55;
    WINDOW* w_monsters = newwin(TERMY-iInfoHeight-VIEW_OFFSET_Y*2, width, VIEW_OFFSET_Y, TERRAIN_WINDOW_WIDTH + VIEW_OFFSET_X);
    WINDOW* w_monster_info = newwin(iInfoHeight-1, width - 2, TERMY-iInfoHeight-VIEW_OFFSET_Y, TERRAIN_WINDOW_WIDTH+1+VIEW_OFFSET_X);
    WINDOW* w_monster_info_border = newwin(iInfoHeight, width, TERMY-iInfoHeight-VIEW_OFFSET_Y, TERRAIN_WINDOW_WIDTH+VIEW_OFFSET_X);

    uistate.list_item_mon = 2; // remember we've tabbed here
    //this stores the monsters found
    std::vector<int> vMonsters = find_nearby_monsters(DAYLIGHT_LEVEL);

    const int iMonsterNum = vMonsters.size();

    if ( iMonsterNum > 0 ) {
        uistate.list_item_mon = 2; // remember we've tabbed here
    }
    const int iWeaponRange = u.weapon.range(&u);

    const int iStoreViewOffsetX = u.view_offset_x;
    const int iStoreViewOffsetY = u.view_offset_y;

    u.view_offset_x = 0;
    u.view_offset_y = 0;

    int iReturn = -1;
    int iActive = 0; // monster index that we're looking at
    const int iMaxRows = TERMY-iInfoHeight-2-VIEW_OFFSET_Y*2;
    int iStartPos = 0;
    int iActiveX = 0;
    int iActiveY = 0;
    int iLastActiveX = -1;
    int iLastActiveY = -1;
    InputEvent input = Undefined;
    long ch = 0; //this is a long because getch returns a long
    int iMonDex = -1;

    for (int i = 1; i < TERMX; i++) {
        if (i < width) {
            mvwputch(w_monsters, 0, i, c_ltgray, LINE_OXOX); // -
            mvwputch(w_monsters, TERMY-iInfoHeight-1-VIEW_OFFSET_Y*2, i, c_ltgray, LINE_OXOX); // -
        }

        if (i < TERMY-iInfoHeight-VIEW_OFFSET_Y*2) {
            mvwputch(w_monsters, i, 0, c_ltgray, LINE_XOXO); // |
            mvwputch(w_monsters, i, width - 1, c_ltgray, LINE_XOXO); // |
        }
    }

    mvwputch(w_monsters, 0, 0,         c_ltgray, LINE_OXXO); // |^
    mvwputch(w_monsters, 0, width - 1, c_ltgray, LINE_OOXX); // ^|

    mvwputch(w_monsters, TERMY-iInfoHeight-1-VIEW_OFFSET_Y*2, 0,         c_ltgray, LINE_XXXO); // |-
    mvwputch(w_monsters, TERMY-iInfoHeight-1-VIEW_OFFSET_Y*2, width - 1, c_ltgray, LINE_XOXX); // -|

    mvwprintz(w_monsters, 0, 2, c_ltgreen, "< ");
    wprintz(w_monsters, c_white, _("Monsters"));
    wprintz(w_monsters, c_ltgreen, " >");

    do {
        if (vMonsters.size() > 0) {
            // we're switching on input here, whereas above it was if/else clauses on a char
            switch(input) {
                case DirectionN:
                    iActive--;
                    if (iActive < 0) {
                        iActive = iMonsterNum - 1;
                    }
                    break;
                case DirectionS:
                    iActive++;
                    if (iActive >= iMonsterNum) {
                        iActive = 0;
                    }
                    break;
                case Tab: //Switch to list_items();
                case DirectionDown:
                case DirectionUp:
                    u.view_offset_x = iStoreViewOffsetX;
                    u.view_offset_y = iStoreViewOffsetY;

                    werase(w_monsters);
                    werase(w_monster_info);
                    werase(w_monster_info_border);
                    delwin(w_monsters);
                    delwin(w_monster_info);
                    delwin(w_monster_info_border);
                    return 1;
                    break;
                default: {
                    action_id act = action_from_key(ch);
                    switch (act) {
                        case ACTION_LOOK: {
                            point recentered=look_around();
                            iLastActiveX=recentered.x;
                            iLastActiveY=recentered.y;
                            } break;
                        case ACTION_FIRE: {
                            if ( rl_dist( point(u.posx, u.posy), zombie(iMonDex).pos() ) <= iWeaponRange ) {
                                last_target = iMonDex;
                                u.view_offset_x = iStoreViewOffsetX;
                                u.view_offset_y = iStoreViewOffsetY;
                                werase(w_monsters);
                                werase(w_monster_info);
                                werase(w_monster_info_border);
                                delwin(w_monsters);
                                delwin(w_monster_info);
                                delwin(w_monster_info_border);
                                return 2;
                            }
                        }
                        break;
                default:
                    break;
            }
                }
                break;
            }

            //Draw Scrollbar
            draw_scrollbar(w_monsters, iActive, iMaxRows, iMonsterNum, 1);

            calcStartPos(iStartPos, iActive, iMaxRows, iMonsterNum);

            for (int i = 0; i < iMaxRows; i++) {
                wmove(w_monsters, i + 1, 1);
                for (int i = 1; i < width - 1; i++)
                    wprintz(w_monsters, c_black, " ");
            }

            int iNum = 0;
            iActiveX = 0;
            iActiveY = 0;
            iMonDex = -1;

            for (int i = 0; i < vMonsters.size(); ++i) {
                if (iNum >= iStartPos && iNum < iStartPos + ((iMaxRows > iMonsterNum) ?
                                                             iMonsterNum : iMaxRows) ) {

                    if (iNum == iActive) {
                        iMonDex = vMonsters[i];

                        iActiveX = zombie(iMonDex).posx() - u.posx;
                        iActiveY = zombie(iMonDex).posy() - u.posy;
                    }

                    mvwprintz(w_monsters, 1 + iNum - iStartPos, 2,
                              ((iNum == iActive) ? c_ltgreen : c_white),
                              "%s", (zombie(vMonsters[i]).name()).c_str());

                    int numw = iMonsterNum > 9 ? 2 : 1;
                    mvwprintz(w_monsters, 1 + iNum - iStartPos, width - (5 + numw),
                              ((iNum == iActive) ? c_ltgreen : c_ltgray), "%*d %s",
                              numw, trig_dist(0, 0, zombie(vMonsters[i]).posx() - u.posx,
                                              zombie(vMonsters[i]).posy() - u.posy),
                              direction_name_short(
                                  direction_from( 0, 0, zombie(vMonsters[i]).posx() - u.posx,
                                                  zombie(vMonsters[i]).posy() - u.posy)).c_str() );
                 }
                 iNum++;
            }

            mvwprintz(w_monsters, 0, (width - 9) / 2 + ((iMonsterNum > 9) ? 0 : 1),
                      c_ltgreen, " %*d", ((iMonsterNum > 9) ? 2 : 1), iActive+1);
            wprintz(w_monsters, c_white, " / %*d ", ((iMonsterNum > 9) ? 2 : 1), iMonsterNum);

            werase(w_monster_info);

            //print monster info
            zombie(iMonDex).print_info(this, w_monster_info,1,11);

            for (int j=0; j < iInfoHeight-1; j++) {
                mvwputch(w_monster_info_border, j, 0, c_ltgray, LINE_XOXO);
                mvwputch(w_monster_info_border, j, width - 1, c_ltgray, LINE_XOXO);
            }


            for (int j=0; j < width - 1; j++) {
                mvwputch(w_monster_info_border, iInfoHeight-1, j, c_ltgray, LINE_OXOX);
            }

            mvwprintz(w_monsters, getmaxy(w_monsters)-1, 1, c_ltgreen, press_x(ACTION_LOOK).c_str());
            wprintz(w_monsters, c_ltgray, " %s",_("to look around"));
            if ( rl_dist(point(u.posx, u.posy), zombie(iMonDex).pos() ) <= iWeaponRange ) {
                wprintz(w_monsters, c_ltgray, "%s", " ");
                wprintz(w_monsters, c_ltgreen, press_x(ACTION_FIRE).c_str());
                wprintz(w_monsters, c_ltgray, " %s", _("to shoot"));
            }

            mvwputch(w_monster_info_border, iInfoHeight-1, 0, c_ltgray, LINE_XXOO);
            mvwputch(w_monster_info_border, iInfoHeight-1, width - 1, c_ltgray, LINE_XOOX);

            //Only redraw trail/terrain if x/y position changed
            if (iActiveX != iLastActiveX || iActiveY != iLastActiveY) {
                iLastActiveX = iActiveX;
                iLastActiveY = iActiveY;
                centerlistview(this, iActiveX, iActiveY);
                draw_trail_to_square(iActiveX, iActiveY, false);
            }

            wrefresh(w_monsters);
            wrefresh(w_monster_info_border);
            wrefresh(w_monster_info);

            refresh();
            ch = getch();
            input = get_input(ch);
        } else {
            iReturn = 0;
            ch = ' ';
            input = Close;
        }
    } while (input != Close && input != Cancel);

    u.view_offset_x = iStoreViewOffsetX;
    u.view_offset_y = iStoreViewOffsetY;

    werase(w_monsters);
    werase(w_monster_info);
    werase(w_monster_info_border);
    delwin(w_monsters);
    delwin(w_monster_info);
    delwin(w_monster_info_border);
    refresh_all(); // TODO - figure out what precisely needs refreshing, rather than the whole screen

    return iReturn;
}

// Pick up items at (posx, posy).
void game::pickup(int posx, int posy, int min)
{
    //min == -1 is Autopickup

    if (m.has_flag("SEALED", posx, posy)) {
        return;
    }

    item_exchanges_since_save += 1; // Keeping this simple.
    write_msg();
    if (u.weapon.type->id == "bio_claws_weapon") {
        if (min != -1) {
            add_msg(_("You cannot pick up items with your claws out!"));
        }
        return;
    }

    bool weight_is_okay = (u.weight_carried() <= u.weight_capacity());
    bool volume_is_okay = (u.volume_carried() <= u.volume_capacity() -  2);
    bool from_veh = false;
    int veh_part = 0;
    int k_part = 0;
    int w_part = 0;
    int craft_part = 0;
    vehicle *veh = m.veh_at (posx, posy, veh_part);
    if (min != -1 && veh) {
        k_part = veh->part_with_feature(veh_part, "KITCHEN");
        w_part = veh->part_with_feature(veh_part, "WELDRIG");
        craft_part = veh->part_with_feature(veh_part, "CRAFTRIG");
        veh_part = veh->part_with_feature(veh_part, "CARGO", false);
        from_veh = veh && veh_part >= 0 && veh->parts[veh_part].items.size() > 0;

        if (from_veh && !query_yn(_("Get items from %s?"),
                                  veh->part_info(veh_part).name.c_str())) {
            from_veh = false;
        }

        if (!from_veh) {
            //Either no cargo to grab, or we declined; what about RV kitchen?
            bool used_feature = false;
            if (k_part >= 0) {
                int choice = menu(true,
                _("RV kitchen:"), _("Use the hotplate"), _("Fill a container with water"), _("Have a drink"), _("Examine vehicle"), NULL);
                switch (choice) {
                case 1:
                    used_feature = true;
                    if (veh->fuel_left("battery") > 0) {
                        //Will be -1 if no battery at all
                        item tmp_hotplate( itypes["hotplate"], 0 );
                        // Drain a ton of power
                        tmp_hotplate.charges = veh->drain( "battery", 100 );
                        if( tmp_hotplate.is_tool() ) {
                            it_tool * tmptool = static_cast<it_tool*>((&tmp_hotplate)->type);
                            if ( tmp_hotplate.charges >= tmptool->charges_per_use ) {
                                tmptool->use.call(&u, &tmp_hotplate, false);
                                tmp_hotplate.charges -= tmptool->charges_per_use;
                                veh->refill( "battery", tmp_hotplate.charges );
                            }
                        }
                    } else {
                        add_msg(_("The battery is dead."));
                    }
                    break;
                case 2:
                    used_feature = true;
                    if (veh->fuel_left("water") > 0) { // -1 if no water at all
                        int amt = veh->drain("water", veh->fuel_left("water"));
                        item fill_water(itypes[default_ammo("water")], g->turn);
                        fill_water.charges = amt;
                        int back = g->move_liquid(fill_water);
                        if (back >= 0) {
                            veh->refill("water", back);
                        } else {
                            veh->refill("water", amt);
                        }
                    } else {
                        add_msg(_("The water tank is empty."));
                    }
                    break;
                case 3:
                    used_feature = true;
                    if (veh->fuel_left("water") > 0) { // -1 if no water at all
                        veh->drain("water", 1);
                        item water(itypes["water_clean"], 0);
                        u.consume(this, u.inv.add_item(water).invlet);
                        u.moves -= 250;
                    } else {
                        add_msg(_("The water tank is empty."));
                    }
                }
            }

            if (w_part >= 0) {
                if (query_yn(_("Use the welding rig?"))) {
                    used_feature = true;
                    if (veh->fuel_left("battery") > 0) {
                        //Will be -1 if no battery at all
                        item tmp_welder( itypes["welder"], 0 );
                        // Drain a ton of power
                        tmp_welder.charges = veh->drain( "battery", 1000 );
                        if( tmp_welder.is_tool() ) {
                            it_tool * tmptool = static_cast<it_tool*>((&tmp_welder)->type);
                            if ( tmp_welder.charges >= tmptool->charges_per_use ) {
                                tmptool->use.call( &u, &tmp_welder, false );
                                tmp_welder.charges -= tmptool->charges_per_use;
                                veh->refill( "battery", tmp_welder.charges );
                            }
                        }
                    } else {
                        add_msg(_("The battery is dead."));
                    }
                }
            }

            if (craft_part >= 0) {
                if (query_yn(_("Use the water purifier?"))) {
                    used_feature = true;
                    if (veh->fuel_left("battery") > 0) {
                        //Will be -1 if no battery at all
                        item tmp_purifier( itypes["water_purifier"], 0 );
                        // Drain a ton of power
                        tmp_purifier.charges = veh->drain( "battery", 100 );
                        if( tmp_purifier.is_tool() ) {
                            it_tool * tmptool = static_cast<it_tool*>((&tmp_purifier)->type);
                            if ( tmp_purifier.charges >= tmptool->charges_per_use ) {
                                tmptool->use.call( &u, &tmp_purifier, false );
                                tmp_purifier.charges -= tmptool->charges_per_use;
                                veh->refill( "battery", tmp_purifier.charges );
                            }
                        }
                    } else {
                        add_msg(_("The battery is dead."));
                    }
                }
            }

            //If we still haven't done anything, we probably want to examine the vehicle
            if(!used_feature) {
                exam_vehicle(*veh, posx, posy);
            }
        }
    }

    if (!from_veh) {
        bool isEmpty = (m.i_at(posx, posy).size() == 0);

        // Hide the pickup window if this is a toilet and there's nothing here
        // but water.
        if ((!isEmpty) && m.furn(posx, posy) == f_toilet) {
            isEmpty = true;
            for (int i = 0; isEmpty && i < m.i_at(posx, posy).size(); i++) {
                if (m.i_at(posx, posy)[i].typeId() != "water") {
                    isEmpty = false;
                }
            }
        }

        if (isEmpty) { return; }
    }

    // which items are we grabbing?
    std::vector<item> here = from_veh ? veh->parts[veh_part].items : m.i_at(posx, posy);

    // Not many items, just grab them
    if (here.size() <= min && min != -1) {
        int iter = 0;
        item newit = here[0];
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
                    // Armor can be instantly worn
                    if (newit.is_armor() &&
                            query_yn(_("Put on the %s?"),
                                     newit.tname(this).c_str())) {
                        if (u.wear_item(this, &newit)) {
                            if (from_veh) {
                                veh->remove_item (veh_part, 0);
                            } else {
                                m.i_clear(posx, posy);
                            }
                        }
                    } else if (query_yn(_("Drop your %s and pick up %s?"),
                                        u.weapon.tname(this).c_str(),
                                        newit.tname(this).c_str())) {
                        if (from_veh) {
                            veh->remove_item (veh_part, 0);
                        } else {
                            m.i_clear(posx, posy);
                        }
                        m.add_item_or_charges(posx, posy, u.remove_weapon(), 1);
                        u.wield(this, u.i_add(newit, this).invlet);
                        u.moves -= 100;
                        add_msg(_("Wielding %c - %s"), newit.invlet,
                                newit.tname(this).c_str());
                    } else {
                        decrease_nextinv();
                    }
                } else {
                    add_msg(_("There's no room in your inventory for the %s, \
and you can't unwield your %s."),
                            newit.tname(this).c_str(),
                            u.weapon.tname(this).c_str());
                    decrease_nextinv();
                }
            } else {
                u.wield(this, u.i_add(newit, this).invlet);
                if (from_veh) {
                    veh->remove_item (veh_part, 0);
                } else {
                    m.i_clear(posx, posy);
                }
                u.moves -= 100;
                add_msg(_("Wielding %c - %s"), newit.invlet,
                        newit.tname(this).c_str());
            }
        } else if (!u.is_armed() &&
                   (u.volume_carried() + newit.volume() > u.volume_capacity() - 2 ||
                    newit.is_weap() || newit.is_gun())) {
            u.weapon = newit;
            if (from_veh) {
                veh->remove_item (veh_part, 0);
            } else {
                m.i_clear(posx, posy);
            }
            u.moves -= 100;
            add_msg(_("Wielding %c - %s"), newit.invlet, newit.tname(this).c_str());
        } else {
            newit = u.i_add(newit, this);
            if (from_veh) {
                veh->remove_item (veh_part, 0);
            } else {
                m.i_clear(posx, posy);
            }
            u.moves -= 100;
            add_msg("%c - %s", newit.invlet, newit.tname(this).c_str());
        }

        if (weight_is_okay && u.weight_carried() >= u.weight_capacity()) {
            add_msg(_("You're overburdened!"));
        }
        if (volume_is_okay && u.volume_carried() > u.volume_capacity() - 2) {
            add_msg(_("You struggle to carry such a large volume!"));
        }
        return;
    }

    bool sideStyle = use_narrow_sidebar();

    // Otherwise, we have Autopickup, 2 or more items and should list them, etc.
    int maxmaxitems = sideStyle ? TERMY : getmaxy(w_messages) - 3;

    int itemsH = 12;
    int pickupBorderRows = 3;

    // The pickup list may consume the entire terminal, minus space needed for its
    // header/footer and the item info window.
    int minleftover = itemsH + pickupBorderRows;
    if(maxmaxitems > TERMY - minleftover) maxmaxitems = TERMY - minleftover;

    const int minmaxitems = sideStyle ? 6 : 9;

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

    int itemcount = 0;
    std::map<int, unsigned int> pickup_count; // Count of how many we'll pick up from each stack

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
                    if ( mapAutoPickupItems[here[i].tname(this)] == "true" ) {
                        bPickup = true;
                    } else if ( mapAutoPickupItems[here[i].tname(this)] != "false" ) {
                        //No prematched pickup rule found
                        //items with damage, (fits) or a container
                        createPickupRules(here[i].tname(this));

                        if ( mapAutoPickupItems[here[i].tname(this)] == "true" ) {
                            bPickup = true;
                        }
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
            static const std::string pickup_chars =
                "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ:;";
            size_t idx=-1;
            for (int i = 1; i < pickupH; i++) {
                mvwprintw(w_pickup, i, 0,
                          "                                                ");
            }
            if (ch >= '0' && ch <= '9') {
                ch = (char)ch - '0';
                itemcount *= 10;
                itemcount += ch;
            } else if ((ch == '<' || ch == KEY_PPAGE) && start > 0) {
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
                    if (start >= here.size()-1) {
                        start -= maxitems;
                    }
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

            if (idx != -1) {
                if (itemcount != 0 || pickup_count[idx] == 0) {
                    if (itemcount >= here[idx].charges) {
                        // Ignore the count if we pickup the whole stack anyway
                        itemcount = 0;
                    }
                    pickup_count[idx] = itemcount;
                    itemcount = 0;
                }
            }

            if ( idx < here.size()) {
                getitem[idx] = ( ch == KEY_RIGHT ? true : ( ch == KEY_LEFT ? false : !getitem[idx] ) );
                if ( ch != KEY_RIGHT && ch != KEY_LEFT) {
                    selected = idx;
                    start = (int)( idx / maxitems ) * maxitems;
                }

                if (getitem[idx]) {
                    if (pickup_count[idx] != 0 &&
                            pickup_count[idx] < here[idx].charges) {
                        item temp = here[idx].clone();
                        temp.charges = pickup_count[idx];
                        new_weight += temp.weight();
                        new_volume += temp.volume();
                    } else {
                        new_weight += here[idx].weight();
                        new_volume += here[idx].volume();
                    }
                } else if (pickup_count[idx] != 0 &&
                           pickup_count[idx] < here[idx].charges) {
                    item temp = here[idx].clone();
                    temp.charges = pickup_count[idx];
                    new_weight -= temp.weight();
                    new_volume -= temp.volume();
                    pickup_count[idx] = 0;
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
                    if (getitem[i]) {
                        count++;
                    } else {
                        new_weight += here[i].weight();
                        new_volume += here[i].volume();
                    }
                    getitem[i] = true;
                }
                if (count == here.size()) {
                    for (int i = 0; i < here.size(); i++) {
                        getitem[i] = false;
                    }
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
                    if (cur_it == selected) {
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
                    if (getitem[cur_it]) {
                        if (pickup_count[cur_it] == 0) {
                            wprintz(w_pickup, c_ltblue, " + ");
                        } else {
                            wprintz(w_pickup, c_ltblue, " # ");
                        }
                    } else {
                        wprintw(w_pickup, " - ");
                    }
                    wprintz(w_pickup, icolor, here[cur_it].tname(this).c_str());
                    if (here[cur_it].charges > 0) {
                        wprintz(w_pickup, icolor, " (%d)", here[cur_it].charges);
                    }
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
            if (start > 0) {
                mvwprintw(w_pickup, maxitems + 2, 0, prev);
            }
            mvwprintw(w_pickup, maxitems + 2, (pw - strlen(all)) / 2, all);
            if (cur_it < here.size()) {
                mvwprintw(w_pickup, maxitems + 2, pw - strlen(next), next);
            }

            if (update) { // Update weight & volume information
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
    bool got_water = false; // Did we try to pick up water?
    bool offered_swap = false;
    std::map<std::string, int> mapPickup;
    for (int i = 0; i < here.size(); i++) {
        iter = 0;
        // This while loop guarantees the inventory letter won't be a repeat. If it
        // tries all 52 letters, it fails and we don't pick it up.
        if (getitem[i] && here[i].made_of(LIQUID)) {
            got_water = true;
        } else if (getitem[i]) {
            bool picked_up = false;
            item temp = here[i].clone();
            iter = 0;
            while (iter < inv_chars.size() &&
                   (here[i].invlet == 0 || (u.has_item(here[i].invlet) &&
                         !u.i_at(here[i].invlet).stacks_with(here[i]))) ) {
                here[i].invlet = nextinv;
                iter++;
                advance_nextinv();
            }

            if(pickup_count[i] != 0) {
                // Reinserting leftovers happens after item removal to avoid stacking issues.
                int leftover_charges = here[i].charges - pickup_count[i];
                if (leftover_charges > 0) {
                    temp.charges = leftover_charges;
                    here[i].charges = pickup_count[i];
                }
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
                        // Armor can be instantly worn
                        if (here[i].is_armor() &&
                                query_yn(_("Put on the %s?"),
                                         here[i].tname(this).c_str())) {
                            if (u.wear_item(this, &(here[i]))) {
                                picked_up = true;
                            }
                        } else if (!offered_swap) {
                            if (query_yn(_("Drop your %s and pick up %s?"),
                                         u.weapon.tname(this).c_str(),
                                         here[i].tname(this).c_str())) {
                                picked_up = true;
                                m.add_item_or_charges(posx, posy, u.remove_weapon(), 1);
                                u.wield(this, u.i_add(here[i], this).invlet);
                                mapPickup[here[i].tname(this)]++;
                                add_msg(_("Wielding %c - %s"), u.weapon.invlet,
                                        u.weapon.tname(this).c_str());
                            }
                            offered_swap = true;
                        } else {
                            decrease_nextinv();
                        }
                    } else {
                        add_msg(_("There's no room in your inventory for the %s, and you can't unwield your %s."),
                                here[i].tname(this).c_str(),
                                u.weapon.tname(this).c_str());
                        decrease_nextinv();
                    }
                } else {
                    u.wield(this, u.i_add(here[i], this).invlet);
                    mapPickup[here[i].tname(this)]++;
                    picked_up = true;
                }
            } else if (!u.is_armed() &&
                       (u.volume_carried() + here[i].volume() > u.volume_capacity() - 2 ||
                        here[i].is_weap() || here[i].is_gun())) {
                u.weapon = here[i];
                picked_up = true;
            } else {
                u.i_add(here[i], this);
                mapPickup[here[i].tname(this)]++;
                picked_up = true;
            }

            if (picked_up) {
                if (from_veh) {
                    veh->remove_item (veh_part, curmit);
                } else {
                    m.i_rem(posx, posy, curmit);
                }
                curmit--;
                u.moves -= 100;
                if( pickup_count[i] != 0 ) {
                    bool to_map = !from_veh;

                    if (from_veh) {
                        to_map = !veh->add_item( veh_part, temp );
                    }
                    if (to_map) {
                        m.add_item_or_charges( posx, posy, temp );
                    }
                }
            }
        }
        curmit++;
    }

    // Auto pickup item message
    // FIXME: i18n
    if (min == -1 && !mapPickup.empty()) {
        std::stringstream sTemp;
        for (std::map<std::string, int>::iterator iter = mapPickup.begin();
                iter != mapPickup.end(); ++iter) {
            if (sTemp.str() != "") {
                sTemp << ", ";
            }
            sTemp << iter->second << " " << iter->first;
        }
        add_msg((_("You pick up: ") + sTemp.str()).c_str());
    }

    if (got_water) {
        add_msg(_("You can't pick up a liquid!"));
    }
    if (weight_is_okay && u.weight_carried() >= u.weight_capacity()) {
        add_msg(_("You're overburdened!"));
    }
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

// Establish or release a grab on a vehicle
void game::grab()
{
    int grabx = 0;
    int graby = 0;
    if( 0 != u.grab_point.x || 0 != u.grab_point.y ) {
        vehicle *veh = m.veh_at( u.posx + u.grab_point.x, u.posy + u.grab_point.y );
        if( veh ) {
            add_msg(_("You release the %s."), veh->name.c_str() );
        } else if ( m.has_furn( u.posx + u.grab_point.x, u.posy + u.grab_point.y ) ) {
            add_msg(_("You release the %s."), m.furnname( u.posx + u.grab_point.x, u.posy + u.grab_point.y ).c_str() );
        }
        u.grab_point.x = 0;
        u.grab_point.y = 0;
        u.grab_type = OBJECT_NONE;
        return;
    }
    if( choose_adjacent( _("Grab where?"), grabx, graby ) ) {
        vehicle *veh = m.veh_at(grabx, graby);
        if( veh != NULL ) { // If there's a vehicle, grab that.
            u.grab_point.x = grabx - u.posx;
            u.grab_point.y = graby - u.posy;
            u.grab_type = OBJECT_VEHICLE;
            add_msg(_("You grab the %s."), veh->name.c_str());
        } else if ( m.has_furn( grabx, graby ) ) { // If not, grab furniture if present
            u.grab_point.x = grabx - u.posx;
            u.grab_point.y = graby - u.posy;
            u.grab_type = OBJECT_FURNITURE;
            if (!m.can_move_furniture( grabx, graby, &u ))
              add_msg(_("You grab the %s. It feels really heavy."), m.furnname( grabx, graby).c_str() );
            else
              add_msg(_("You grab the %s."), m.furnname( grabx, graby).c_str() );
        } else { // todo: grab mob? Captured squirrel = pet (or meat that stays fresh longer).
            add_msg(_("There's nothing to grab there!"));
        }
    } else {
        add_msg(_("Never Mind."));
    }
}

// Handle_liquid returns false if we didn't handle all the liquid.
bool game::handle_liquid(item &liquid, bool from_ground, bool infinite, item *source, item *cont)
{
    if (!liquid.made_of(LIQUID)) {
        dbg(D_ERROR) << "game:handle_liquid: Tried to handle_liquid a non-liquid!";
        debugmsg("Tried to handle_liquid a non-liquid!");
        return false;
    }

    if (liquid.type->id == "gasoline" && vehicle_near() && query_yn(_("Refill vehicle?"))) {
        int vx = u.posx, vy = u.posy;
        refresh_all();
        if (choose_adjacent(_("Refill vehicle where?"), vx, vy)) {
            vehicle *veh = m.veh_at (vx, vy);
            if (veh) {
                ammotype ftype = "gasoline";
                int fuel_cap = veh->fuel_capacity(ftype);
                int fuel_amnt = veh->fuel_left(ftype);
                if (fuel_cap < 1) {
                    add_msg (_("This vehicle doesn't use %s."), ammo_name(ftype).c_str());
                } else if (fuel_amnt == fuel_cap) {
                    add_msg (_("Already full."));
                } else if (from_ground && query_yn(_("Pump until full?"))) {
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
            } else { // if (veh)
                add_msg (_("There isn't any vehicle there."));
            }
            return false;
        } // if (choose_adjacent(_("Refill vehicle where?"), vx, vy))
        return true;
    }

    // Ask to pour rotten liquid (milk!) from the get-go
    if (!from_ground && liquid.rotten(this) &&
            query_yn(_("Pour %s on the ground?"), liquid.tname(this).c_str())) {
        if (!m.has_flag("SWIMMABLE", u.posx, u.posy)) {
            m.add_item_or_charges(u.posx, u.posy, liquid, 1);
        }

        return true;
    }

    if (cont == NULL) {
        std::stringstream text;
        text << _("Container for ") << liquid.tname(this);

        char ch = inv_for_liquid(liquid, text.str().c_str(), false);
        if (!u.has_item(ch)) {
            // No container selected (escaped, ...), ask to pour
            // we asked to pour rotten already
            if (!from_ground && !liquid.rotten(this) &&
                query_yn(_("Pour %s on the ground?"), liquid.tname(this).c_str())) {
                    if (!m.has_flag("SWIMMABLE", u.posx, u.posy))
                        m.add_item_or_charges(u.posx, u.posy, liquid, 1);
                    return true;
            }
            return false;
        }

        cont = &(u.i_at(ch));
    }

    if (cont == NULL || cont->is_null()) {
        // Container is null, ask to pour.
        // we asked to pour rotten already
        if (!from_ground && !liquid.rotten(this) &&
                query_yn(_("Pour %s on the ground?"), liquid.tname(this).c_str())) {
            if (!m.has_flag("SWIMMABLE", u.posx, u.posy))
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
            it_tool *tool = dynamic_cast<it_tool *>(cont->type);
            ammo = tool->ammo;
            max = tool->max_charges;
        } else {
            it_gun *gun = dynamic_cast<it_gun *>(cont->type);
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

        if (cont->charges > 0 && cont->curammo != NULL && cont->curammo->id != liquid.type->id) {
            add_msg(_("You can't mix loads in your %s."), cont->tname(this).c_str());
            return false;
        }

        add_msg(_("You pour %s into your %s."), liquid.tname(this).c_str(),
                cont->tname(this).c_str());
        cont->curammo = dynamic_cast<it_ammo *>(liquid.type);
        if (infinite) {
            cont->charges = max;
        } else {
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

    } else {      // filling up normal containers
        LIQUID_FILL_ERROR error;
        int remaining_capacity = cont->get_remaining_capacity_for_liquid(liquid, error);
        if (remaining_capacity <= 0) {
            switch (error)
            {
            case L_ERR_NO_MIX:
                add_msg(_("You can't mix loads in your %s."), cont->tname(this).c_str());
                break;
            case L_ERR_NOT_CONTAINER:
                add_msg(_("That %s won't hold %s."), cont->tname(this).c_str(), liquid.tname(this).c_str());
                break;
            case L_ERR_NOT_WATERTIGHT:
                add_msg(_("That %s isn't water-tight."), cont->tname(this).c_str());
                break;
            case L_ERR_NOT_SEALED:
                add_msg(_("You can't seal that %s!"), cont->tname(this).c_str());
                break;
            case L_ERR_FULL:
                add_msg(_("Your %s can't hold any more %s."), cont->tname(this).c_str(),
                    liquid.tname(this).c_str());
                break;
            default:
                break;
            }
            return false;
        }

        if (!cont->contents.empty()) {
            // Container is partly full
            if (infinite) {
                cont->contents[0].charges += remaining_capacity;
                add_msg(_("You pour %s into your %s."), liquid.tname(this).c_str(),
                        cont->tname(this).c_str());
                return true;
            } else { // Container is finite, not empty and not full, add liquid to it
                add_msg(_("You pour %s into your %s."), liquid.tname(this).c_str(),
                        cont->tname(this).c_str());
                if (remaining_capacity > liquid.charges) {
                    remaining_capacity = liquid.charges;
                }
                cont->contents[0].charges += remaining_capacity;
                liquid.charges -= remaining_capacity;
                if (liquid.charges > 0) {
                    add_msg(_("There's some left over!"));
                    // Why not try to find another container here?
                    return false;
                }
                return true;
            }
        } else {
            // pouring into a valid empty container
            item liquid_copy = liquid;
            bool all_poured = true;
            if (infinite) { // if filling from infinite source, top it to max
                liquid_copy.charges = remaining_capacity;
                add_msg(_("You pour %s into your %s."), liquid.tname(this).c_str(),
                    cont->tname(this).c_str());
            } else if (liquid.charges > remaining_capacity) {
                add_msg(_("You fill your %s with some of the %s."), cont->tname(this).c_str(),
                        liquid.tname(this).c_str());
                u.inv.unsort();
                liquid.charges -= remaining_capacity;
                liquid_copy.charges = remaining_capacity;
                all_poured = false;
            } else {
                add_msg(_("You pour %s into your %s."), liquid.tname(this).c_str(),
                    cont->tname(this).c_str());
            }
            cont->put_in(liquid_copy);
            return all_poured;
        }
    }
    return false;
}

//Move_liquid returns the amount of liquid left if we didn't move all the liquid, otherwise returns sentinel -1, signifies transaction fail.
//One-use, strictly for liquid transactions. Not intended for use with while loops.
int game::move_liquid(item &liquid)
{
  if(!liquid.made_of(LIQUID)) {
   dbg(D_ERROR) << "game:move_liquid: Tried to move_liquid a non-liquid!";
   debugmsg("Tried to move_liquid a non-liquid!");
   return -1;
  }

  //liquid is in fact a liquid.
  std::stringstream text;
  text << _("Container for ") << liquid.tname(this);
  char ch = inv_for_liquid(liquid, text.str().c_str(), false);

  //is container selected?
  if(u.has_item(ch)) {
    item *cont = &(u.i_at(ch));
    if (cont == NULL || cont->is_null())
      return -1;
    else if (liquid.is_ammo() && (cont->is_tool() || cont->is_gun())) {
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
      return -1;
      }

      if (max <= 0 || cont->charges >= max) {
      add_msg(_("Your %s can't hold any more %s."), cont->tname(this).c_str(),
                                                    liquid.tname(this).c_str());
      return -1;
      }

      if (cont->charges > 0 && cont->curammo != NULL && cont->curammo->id != liquid.type->id) {
      add_msg(_("You can't mix loads in your %s."), cont->tname(this).c_str());
      return -1;
      }

      add_msg(_("You pour %s into your %s."), liquid.tname(this).c_str(),
                                          cont->tname(this).c_str());
      cont->curammo = dynamic_cast<it_ammo*>(liquid.type);
      cont->charges += liquid.charges;
      if (cont->charges > max) {
      int extra = cont->charges - max;
      cont->charges = max;
      add_msg(_("There's some left over!"));
      return extra;
      }
      else return 0;
    } else if (!cont->is_container()) {
      add_msg(_("That %s won't hold %s."), cont->tname(this).c_str(),
                                         liquid.tname(this).c_str());
      return -1;
    } else {
      if (!cont->contents.empty())
      {
        if  (cont->contents[0].type->id != liquid.type->id)
        {
          add_msg(_("You can't mix loads in your %s."), cont->tname(this).c_str());
          return -1;
        }
      }
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
      if (!cont->contents.empty()) {

        // case 1: container is completely full
        if (cont->contents[0].charges == holding_container_charges) {
          add_msg(_("Your %s can't hold any more %s."), cont->tname(this).c_str(),
                                                   liquid.tname(this).c_str());
          return -1;
        } else {
            add_msg(_("You pour %s into your %s."), liquid.tname(this).c_str(),
                      cont->tname(this).c_str());
          cont->contents[0].charges += liquid.charges;
            if (cont->contents[0].charges > holding_container_charges) {
              int extra = cont->contents[0].charges - holding_container_charges;
              cont->contents[0].charges = holding_container_charges;
              add_msg(_("There's some left over!"));
              return extra;
            }
            else return 0;
          }
      } else {
          if (!cont->has_flag("WATERTIGHT")) {
            add_msg(_("That %s isn't water-tight."), cont->tname(this).c_str());
            return -1;
          }
          else if (!(cont->has_flag("SEALS"))) {
            add_msg(_("You can't seal that %s!"), cont->tname(this).c_str());
            return -1;
          }
          // pouring into a valid empty container
          int default_charges = 1;

          if (liquid.is_food()) {
            it_comest* comest = dynamic_cast<it_comest*>(liquid.type);
            default_charges = comest->charges;
          }
          else if (liquid.is_ammo()) {
            it_ammo* ammo = dynamic_cast<it_ammo*>(liquid.type);
            default_charges = ammo->count;
          }
          if (liquid.charges > container->contains * default_charges) {
            add_msg(_("You fill your %s with some of the %s."), cont->tname(this).c_str(),
                                                      liquid.tname(this).c_str());
            u.inv.unsort();
            int extra = liquid.charges - container->contains * default_charges;
            liquid.charges = container->contains * default_charges;
            cont->put_in(liquid);
            return extra;
          } else {
          cont->put_in(liquid);
          return 0;
          }
      }
    }
    return -1;
  }
 return -1;
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
        veh_part = veh->part_with_feature (veh_part, "CARGO");
        to_veh = veh_part >= 0;
    }
    if (dropped.size() == 1 || same) {
        if (to_veh) {
            add_msg(ngettext("You put your %1$s in the %2$s's %3$s.",
                             "You put your %1$ss in the %2$s's %3$s.",
                             dropped.size()),
                    dropped[0].tname(this).c_str(),
                    veh->name.c_str(),
                    veh->part_info(veh_part).name.c_str());
        } else {
            add_msg(ngettext("You drop your %s.", "You drop your %ss.",
                             dropped.size()),
                    dropped[0].tname(this).c_str());
        }
    } else {
        if (to_veh) {
            add_msg(_("You put several items in the %s's %s."),
                    veh->name.c_str(), veh->part_info(veh_part).name.c_str());
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
    if (!choose_adjacent(_("Drop where?"), dirx, diry)) {
        return;
    }

    int veh_part = 0;
    bool to_veh = false;
    vehicle *veh = m.veh_at(dirx, diry, veh_part);
    if (veh) {
        veh_part = veh->part_with_feature (veh_part, "CARGO");
        to_veh = veh_part >= 0;
    }

    if (!m.can_put_items(dirx, diry)) {
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
                    veh->part_info(veh_part).name.c_str());
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
                    veh->name.c_str(), veh->part_info(veh_part).name.c_str());
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
 for (int i = 0; i < num_zombies(); i++) {
   monster &z = _active_monsters[i];
   if (u_see(&z)) {
     z.draw(w_terrain, u.posx, u.posy, true);
     if(rl_dist( u.posx, u.posy, z.posx(), z.posy() ) <= range) {
       mon_targets.push_back(z);
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

 // Base move cost on moves per turn of the weapon
 // and our skill.
 int move_cost = thrown.attack_time() / 2;
 int skill_cost = (int)(move_cost / (pow(u.skillLevel("throw"), 3)/400 +1));
 int dexbonus = (int)( pow(std::max(u.dex_cur - 8, 0), 0.8) * 3 );

 move_cost += skill_cost;
 move_cost += 20 * u.encumb(bp_torso);
 move_cost -= dexbonus;

 if (u.has_trait("LIGHT_BONES"))
  move_cost *= .9;
 if (u.has_trait("HOLLOW_BONES"))
  move_cost *= .8;

 move_cost -= u.disease_intensity("speed_boost");

 if (move_cost < 25)
  move_cost = 25;

 u.moves -= move_cost;
 u.practice(turn, "throw", 10);

 throw_item(u, x, y, thrown, trajectory);
}

void game::plfire(bool burst, int default_target_x, int default_target_y)
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
  if (u.has_charges("UPS_on", 1)  ||
      u.has_charges("adv_UPS_on", 1) ) {
   add_msg(_("Your %s starts charging."), u.weapon.tname().c_str());
   u.weapon.charges = 0;
   u.weapon.poison = 0;
   u.weapon.curammo = dynamic_cast<it_ammo*>(itypes["charge_shot"]);
   u.weapon.active = true;
   return;
  } else {
   add_msg(_("You need a powered UPS."));
   return;
  }
 }

 if (u.weapon.has_flag("NO_AMMO")) {
   u.weapon.charges = 1;
   u.weapon.curammo = dynamic_cast<it_ammo*>(itypes["generic_no_ammo"]);
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

 if (u.weapon.num_charges() == 0 && !u.weapon.has_flag("RELOAD_AND_SHOOT")
     && !u.weapon.has_flag("NO_AMMO")) {
  add_msg(_("You need to reload!"));
  return;
 }
 if (u.weapon.has_flag("FIRE_100") && u.weapon.num_charges() < 100) {
  add_msg(_("Your %s needs 100 charges to fire!"), u.weapon.tname().c_str());
  return;
 }
 if (u.weapon.has_flag("FIRE_50") && u.weapon.num_charges() < 50) {
  add_msg(_("Your %s needs 50 charges to fire!"), u.weapon.tname().c_str());
  return;
 }
 if (u.weapon.has_flag("USE_UPS") && !u.has_charges("UPS_off", 5) &&
     !u.has_charges("UPS_on", 5) && !u.has_charges("adv_UPS_off", 3) &&
     !u.has_charges("adv_UPS_on", 3)) {
  add_msg(_("You need a UPS with at least 5 charges or an advanced UPS with at least 3 charges to fire that!"));
  return;
 } else if (u.weapon.has_flag("USE_UPS_20") && !u.has_charges("UPS_off", 20) &&
  !u.has_charges("UPS_on", 20) && !u.has_charges("adv_UPS_off", 12) &&
  !u.has_charges("adv_UPS_on", 12)) {
  add_msg(_("You need a UPS with at least 20 charges or an advanced UPS with at least 12 charges to fire that!"));
  return;
 } else if (u.weapon.has_flag("USE_UPS_40") && !u.has_charges("UPS_off", 40) &&
  !u.has_charges("UPS_on", 40) && !u.has_charges("adv_UPS_off", 24) &&
  !u.has_charges("adv_UPS_on", 24)) {
  add_msg(_("You need a UPS with at least 40 charges or an advanced UPS with at least 24 charges to fire that!"));
  return;
 }

 int range = u.weapon.range(&u);

 m.draw(this, w_terrain, point(u.posx, u.posy));

// Populate a list of targets with the zombies in range and visible
 std::vector <monster> mon_targets;
 std::vector <int> targetindices;
 int passtarget = -1;
 for (int i = 0; i < num_zombies(); i++) {
   monster &z = _active_monsters[i];
   if (u_see(&z)) {
     z.draw(w_terrain, u.posx, u.posy, true);
     if(rl_dist( u.posx, u.posy, z.posx(), z.posy() ) <= range) {
       mon_targets.push_back(z);
       targetindices.push_back(i);
       bool is_default_target = default_target_x == z.posx() && default_target_y == z.posy();
       if (is_default_target || (passtarget == -1 && i == last_target)) {
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
  zombie(targetindices[passtarget]).add_effect(ME_HIT_BY_PLAYER, 100);
 }

 if (u.weapon.mode == "MODE_BURST")
  burst = true;

// Train up our skill
 it_gun* firing = dynamic_cast<it_gun*>(u.weapon.type);
 int num_shots = 1;
 if (burst)
  num_shots = u.weapon.burst_size();
 if (num_shots > u.weapon.num_charges() && !u.weapon.has_flag("NO_AMMO"))
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
    if (u.controlling_vehicle) {
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

 if (is_hostile_nearby() &&
     !query_yn(_("Hostiles are nearby! Start Butchering anyway?")))
 {
     return;
 }

 int butcher_corpse_index = 0;
 if (corpses.size() > 1) {
     uimenu kmenu;
     kmenu.text = _("Choose corpse to butcher");
     kmenu.selected = 0;
     for (int i = 0; i < corpses.size(); i++) {
         mtype *corpse = m.i_at(u.posx, u.posy)[corpses[i]].corpse;
         int hotkey = -1;
         if (i == 0) {
             for (std::map<char, action_id>::iterator it = keymap.begin(); it != keymap.end(); it++) {
                 if (it->second == ACTION_BUTCHER) {
                     hotkey = (it->first == 'q') ? -1 : it->first;
                     break;
                 }
             }
         }
         kmenu.addentry(i, true, hotkey, corpse->name.c_str());
     }
     kmenu.addentry(corpses.size(), true, 'q', _("Cancel"));
     kmenu.query();
     if (kmenu.ret == corpses.size()) {
         return;
     }
     butcher_corpse_index = kmenu.ret;
 }

 mtype *corpse = m.i_at(u.posx, u.posy)[corpses[butcher_corpse_index]].corpse;
 int time_to_cut = 0;
 switch (corpse->size) { // Time in turns to cut up te corpse
  case MS_TINY:   time_to_cut =  2; break;
  case MS_SMALL:  time_to_cut =  5; break;
  case MS_MEDIUM: time_to_cut = 10; break;
  case MS_LARGE:  time_to_cut = 18; break;
  case MS_HUGE:   time_to_cut = 40; break;
 }
 time_to_cut *= 100; // Convert to movement points
 time_to_cut += factor * 5; // Penalty for poor tool
 if (time_to_cut < 250)
  time_to_cut = 250;
 u.assign_activity(this, ACT_BUTCHER, time_to_cut, corpses[butcher_corpse_index]);
 u.moves = 0;
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
 int pieces = 0, skins = 0, bones = 0, sinews = 0, feathers = 0;
 double skill_shift = 0.;

 int sSkillLevel = u.skillLevel("survival");

 switch (corpse->size) {
  case MS_TINY:   pieces =  1; skins =  1; bones = 1; sinews = 1; feathers = 2;  break;
  case MS_SMALL:  pieces =  2; skins =  3; bones = 4; sinews = 4; feathers = 6;  break;
  case MS_MEDIUM: pieces =  4; skins =  6; bones = 9; sinews = 9; feathers = 11; break;
  case MS_LARGE:  pieces =  8; skins = 10; bones = 14;sinews = 14; feathers = 17;break;
  case MS_HUGE:   pieces = 16; skins = 18; bones = 21;sinews = 21; feathers = 24;break;
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
 if (skill_shift < 5)  { // Lose some skins and bones
  skins += ((int)skill_shift - 5);
  bones += ((int)skill_shift - 2);
  sinews += ((int)skill_shift - 8);
  feathers += ((int)skill_shift - 1);
 }

 if (bones > 0) {
  if (corpse->has_flag(MF_BONES)) {
    m.spawn_item(u.posx, u.posy, "bone", bones, 0, age);
   add_msg(_("You harvest some usable bones!"));
  } else if (corpse->mat == "veggy") {
    m.spawn_item(u.posx, u.posy, "plant_sac", bones, 0, age);
   add_msg(_("You harvest some fluid bladders!"));
  }
 }

 if (sinews > 0) {
  if (corpse->has_flag(MF_BONES)) {
    m.spawn_item(u.posx, u.posy, "sinew", sinews, 0, age);
   add_msg(_("You harvest some usable sinews!"));
  } else if (corpse->mat == "veggy") {
    m.spawn_item(u.posx, u.posy, "plant_fibre", sinews, 0, age);
   add_msg(_("You harvest some plant fibres!"));
  }
 }

    if ((corpse->has_flag(MF_FUR) || corpse->has_flag(MF_LEATHER) ||
         corpse->has_flag(MF_CHITIN)) && skins > 0) {
        add_msg(_("You manage to skin the %s!"), corpse->name.c_str());
        int fur = 0;
        int leather = 0;
        int chitin = 0;

        while (skins > 0) {
            if (corpse->has_flag(MF_CHITIN)) {
                chitin = rng(0, skins);
                skins -= chitin;
                skins = std::max(skins, 0);
            }
            if (corpse->has_flag(MF_FUR)) {
                fur = rng(0, skins);
                skins -= fur;
                skins = std::max(skins, 0);
            }
            if (corpse->has_flag(MF_LEATHER)) {
                leather = rng(0, skins);
                skins -= leather;
                skins = std::max(skins, 0);
            }
        }

        if(chitin) m.spawn_item(u.posx, u.posy, "chitin_piece", chitin, 0, age);
        if(fur) m.spawn_item(u.posx, u.posy, "fur", fur, 0, age);
        if(leather) m.spawn_item(u.posx, u.posy, "leather", leather, 0, age);
    }

 if (feathers > 0) {
  if (corpse->has_flag(MF_FEATHER)) {
    m.spawn_item(u.posx, u.posy, "feather", feathers, 0, age);
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
    m.spawn_item(u.posx, u.posy, "bio_power_storage", 1, 0, age);
   }else{//There is a burnt out CBM
    m.spawn_item(u.posx, u.posy, "burnt_out_bionic", 1, 0, age);
   }
  }
  if(skill_shift >= 0){
   //To see if it spawns a random additional CBM
   if(rng(0,1) == 1){ //The CBM works
    Item_tag bionic_item = item_controller->id_from("bionics");
    m.spawn_item(u.posx, u.posy, bionic_item, 1, 0, age);
   }else{//There is a burnt out CBM
    m.spawn_item(u.posx, u.posy, "burnt_out_bionic", 1, 0, age);
   }
  }
 }

 // Recover hidden items
 for (int i = 0; i < contents.size(); i++) {
   if ((skill_shift + 10) * 5 > rng(0,100)) {
     add_msg(_("You discover a %s in the %s!"), contents[i].tname().c_str(), corpse->name.c_str());
     m.add_item_or_charges(u.posx, u.posy, contents[i]);
   } else if (contents[i].is_bionic()){
     m.spawn_item(u.posx, u.posy, "burnt_out_bionic", 1, 0, age);
   }
 }

 if (pieces <= 0) {
  add_msg(_("Your clumsy butchering destroys the meat!"));
 } else {
  add_msg(_("You butcher the corpse."));
  itype_id meat;
  if (corpse->has_flag(MF_POISON)) {
    if (corpse->mat == "flesh") {
     meat = "meat_tainted";
    } else {
     meat = "veggy_tainted";
    }
  } else {
   if (corpse->mat == "flesh" || corpse->mat == "hflesh") {
    if(corpse->has_flag(MF_HUMAN)) {
     meat = "human_flesh";
    } else {
     meat = "meat";
    }
   } else if(corpse->mat == "bone") {
     meat = "bone";
   } else if(corpse->mat == "veggy") {
    meat = "veggy";
   } else {
     //Don't generate anything
     return;
  }
  }
  item tmpitem=item_controller->create(meat, age);
  tmpitem.corpse=dynamic_cast<mtype*>(corpse);
  while ( pieces > 0 ) {
    pieces--;
    m.add_item_or_charges(u.posx, u.posy, tmpitem);
  }
 }
}

void game::forage()
{
  int veggy_chance = rng(1, 20);

  if (veggy_chance < u.skillLevel("survival"))
  {
    add_msg(_("You found some wild veggies!"));
    u.practice(turn, "survival", 10);
    m.spawn_item(u.activity.placement.x, u.activity.placement.y, "veggy_wild", 1, 0, turn);
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
  ch = inv_type(_("Consume item:"), IC_COMESTIBLE );
 else
  ch = chInput;

 if (ch == ' ' || ch == KEY_ESCAPE) {
  add_msg(_("Never mind."));
  return;
 }

 if (!u.has_item(ch)) {
  add_msg(_("You don't have item '%c'!"), ch);
  return;
 }
 u.consume(this, u.lookup_item(ch));
}

void game::wear(char chInput)
{
 char ch;
 if (chInput == '.')
  ch = inv_type(_("Wear item:"), IC_ARMOR);
 else
  ch = chInput;

 if (inv_chars.find(ch) == std::string::npos) {
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

 if (ch == ' ' || ch == KEY_ESCAPE) {
  add_msg(_("Never mind."));
  return;
 }

 if (u.takeoff(this, ch))
  u.moves -= 250; // TODO: Make this variable
 else
  add_msg(_("Invalid selection."));
}

void game::reload(char chInput)
{
 item* it;
 if (u.weapon.invlet == chInput) {
      it = &u.weapon;
 } else {
      it = &u.inv.item_by_letter(chInput);
 }

 // Gun reloading is more complex.
 if (it->is_gun()) {

     // bows etc do not need to reload.
     if (it->has_flag("RELOAD_AND_SHOOT")) {
         add_msg(_("Your %s does not need to be reloaded, it reloads and fires "
                     "a single motion."), it->tname().c_str());
         return;
     }

     // Make sure the item is actually reloadable
     if (it->ammo_type() == "NULL") {
         add_msg(_("Your %s does not reload normally."), it->tname().c_str());
         return;
     }

     // See if the gun is fully loaded.
     if (it->charges == it->clip_size()) {

         // Also see if the spare magazine is loaded
         bool magazine_isfull = true;
         item contents;

         for (int i = 0; i < it->contents.size(); i++)
         {
             contents = it->contents[i];
             if ((contents.is_gunmod() &&
                  (contents.typeId() == "spare_mag" &&
                   contents.charges < (dynamic_cast<it_gun*>(it->type))->clip)) ||
                (contents.has_flag("AUX_MODE") &&
                 contents.charges < contents.clip_size()))
             {
                 magazine_isfull = false;
                 break;
             }
         }

         if (magazine_isfull) {
             add_msg(_("Your %s is fully loaded!"), it->tname().c_str());
             return;
         }
     }

     // pick ammo
     char am_invlet = it->pick_reload_ammo(u, true);
     if (am_invlet == 0) {
         add_msg(_("Out of ammo!"));
         return;
     }

     // and finally reload.
     const char chStr[2]={chInput, '\0'};
     u.assign_activity(this, ACT_RELOAD, it->reload_time(u), -1, am_invlet, chStr);
     u.moves = 0;

 } else if (it->is_tool()) { // tools are simpler
     it_tool* tool = dynamic_cast<it_tool*>(it->type);

     // see if its actually reloadable.
     if (tool->ammo == "NULL") {
         add_msg(_("You can't reload a %s!"), it->tname().c_str());
         return;
     }

    // pick ammo
    char am_invlet = it->pick_reload_ammo(u, true);

    if (am_invlet == 0) {
        // no ammo, fail reload
        add_msg(_("Out of %s!"), ammo_name(tool->ammo).c_str());
        return;
    }

    // do the actual reloading
     const char chStr[2]={chInput, '\0'};
    u.assign_activity(this, ACT_RELOAD, it->reload_time(u), -1, am_invlet, chStr);
    u.moves = 0;

 } else { // what else is there?
     add_msg(_("You can't reload a %s!"), it->tname().c_str());
 }

 // all done.
 refresh_all();
}

void game::reload()
{
    if (!u.is_armed()) {
      add_msg(_("You're not wielding anything."));
    } else {
      reload(u.weapon.invlet);
    }
}

// Unload a containter, gun, or tool
// If it's a gun, some gunmods can also be loaded
void game::unload(char chInput)
{ // this is necessary to prevent re-selection of the same item later
    item it = (u.inv.remove_item_by_letter(chInput));
    if (!it.is_null())
    {
        unload(it);
        u.i_add(it, this);
    }
    else
    {
        item ite;
        if (u.weapon.invlet == chInput) { // item is wielded as weapon.
            if (std::find(martial_arts_itype_ids.begin(), martial_arts_itype_ids.end(), u.weapon.type->id) != martial_arts_itype_ids.end()){
                return; //ABORT!
            } else {
                ite = u.weapon;
                u.weapon = item(itypes["null"], 0); //ret_null;
                unload(ite);
                u.weapon = ite;
                return;
            }
        } else { //this is that opportunity for reselection where the original container is worn, see issue #808
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
    int has_40mml = -1;
    int has_shotgun = -1;
    int has_shotgun2 = -1;
    int has_shotgun3 = -1;
    if (it.is_gun()) {
        spare_mag = it.has_gunmod ("spare_mag");
        has_m203 = it.has_gunmod ("m203");
        has_40mml = it.has_gunmod ("pipe_launcher40mm");
        has_shotgun = it.has_gunmod ("u_shotgun");
        has_shotgun2 = it.has_gunmod ("masterkey");
        has_shotgun3 = it.has_gunmod ("rm121aux");
    }
    if (it.is_container() ||
        (it.charges == 0 &&
         (spare_mag == -1 || it.contents[spare_mag].charges <= 0) &&
         (has_m203 == -1 || it.contents[has_m203].charges <= 0) &&
         (has_40mml == -1 || it.contents[has_40mml].charges <= 0) &&
         (has_shotgun == -1 || it.contents[has_shotgun].charges <= 0) &&
         (has_shotgun2 == -1 || it.contents[has_shotgun2].charges <= 0) &&
         (has_shotgun3 == -1 || it.contents[has_shotgun3].charges <= 0)))
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
        std::vector<item> new_contents; // In case we put stuff back
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
 if (weapon->is_gun()) { // Gun ammo is combined with existing items
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
  // Then try the pipe 40mm launcher
  else if (has_40mml != -1 && weapon->contents[has_40mml].charges > 0)
   weapon = &weapon->contents[has_40mml];
  // Then try an underslung shotgun
  else if (has_shotgun != -1 && weapon->contents[has_shotgun].charges > 0)
   weapon = &weapon->contents[has_shotgun];
  // Then try a masterkey shotgun
  else if (has_shotgun2 != -1 && weapon->contents[has_shotgun2].charges > 0)
   weapon = &weapon->contents[has_shotgun2];
  // Then try a Rivtech shotgun
  else if (has_shotgun3 != -1 && weapon->contents[has_shotgun3].charges > 0)
   weapon = &weapon->contents[has_shotgun3];
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
   weapon->charges += newam.charges; // Put it back in
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
 if (chInput == '.')
  ch = inv(_("Wield item:"));
 else
  ch = chInput;

 if (ch == ' ' || ch == KEY_ESCAPE) {
  add_msg(_("Never mind."));
  return;
 }

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

 if (ch == ' ' || ch == KEY_ESCAPE) {
  add_msg(_("Never mind."));
  return;
 }
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
    if (run_mode == 2 && safemodeveh) { // Monsters around and we don't wanna run
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
    int pctr = veh->part_with_feature (part, "CONTROLS");
    if (pctr < 0) {
        add_msg (_("You can't drive the vehicle from here. You need controls!"));
        return;
    }

    int thr_amount = 10 * 100;
    if (veh->cruise_on) {
        veh->cruise_thrust (-y * thr_amount);
    } else {
        veh->thrust (-y);
    }
    veh->turn (15 * x);
    if (veh->skidding && veh->valid_wheel_config()) {
        if (rng (0, veh->velocity) < u.dex_cur + u.skillLevel("driving") * 2) {
            add_msg (_("You regain control of the %s."), veh->name.c_str());
            u.practice(turn, "driving", veh->velocity / 5);
            veh->velocity = int(veh->forward_velocity());
            veh->skidding = false;
            veh->move.init (veh->turn_dir);
        }
    }
    // Don't spend turns to adjust cruise speed.
    if( x != 0 || !veh->cruise_on ) {
        u.moves = 0;
    }

    if (x != 0 && veh->velocity != 0 && one_in(10)) {
        u.practice(turn, "driving", 1);
    }
}

bool game::plmove(int dx, int dy)
{
    if (run_mode == 2) {
        // Monsters around and we don't wanna run
        add_msg(_("Monster spotted--safe mode is on! \
(%s to turn it off or %s to ignore monster.)"),
                press_x(ACTION_TOGGLE_SAFEMODE).c_str(),
                from_sentence_case(press_x(ACTION_IGNORE_ENEMY)).c_str());
        return false;
    }
 int x = 0;
 int y = 0;
 if (u.has_disease("stunned")) {
  x = rng(u.posx - 1, u.posx + 1);
  y = rng(u.posy - 1, u.posy + 1);
 } else {
  x = u.posx + dx;
  y = u.posy + dy;

  if (moveCount % 60 == 0) {
   if (u.has_bionic("bio_torsionratchet")) {
    u.charge_power(1);
   }
  }
 }

 dbg(D_PEDANTIC_INFO) << "game:plmove: From ("<<u.posx<<","<<u.posy<<") to ("<<x<<","<<y<<")";

// Check if our movement is actually an attack on a monster
 int mondex = mon_at(x, y);
 // Are we displacing a monster?  If it's vermin, always.
 bool displace = false;
 if (mondex != -1) {
     monster &z = zombie(mondex);
     if (z.friendly == 0 && !(z.type->has_flag(MF_VERMIN))) {
         if (u.has_destination()) {
             add_msg(_("Monster in the way. Auto-move canceled. Click directly on monster to attack."));
             u.clear_destination();
             return false;
         }
         int udam = u.hit_mon(this, &z);
         if (z.hurt(udam) || z.is_hallucination()) {
             kill_mon(mondex, true);
         }
         draw_hit_mon(x,y,z,z.dead);
         return false;
     } else {
         displace = true;
     }
 }
 // If not a monster, maybe there's an NPC there
 int npcdex = npc_at(x, y);
 if (npcdex != -1) {
     bool force_attack = false;
     if(!active_npc[npcdex]->is_enemy()){
         if (!query_yn(_("Really attack %s?"), active_npc[npcdex]->name.c_str())) {
             if (active_npc[npcdex]->is_friend()) {
                 add_msg(_("%s moves out of the way."), active_npc[npcdex]->name.c_str());
                 active_npc[npcdex]->move_away_from(this, u.posx, u.posy);
             }

             return false; // Cancel the attack
         } else {
             //The NPC knows we started the fight, used for morale penalty.
             active_npc[npcdex]->hit_by_player = true;
             force_attack = true;
         }
     }

     if (u.has_destination() && !force_attack) {
         add_msg(_("NPC in the way. Auto-move canceled. Click directly on NPC to attack."));
         u.clear_destination();
         return false;
     }

     u.hit_player(this, *active_npc[npcdex]);
     active_npc[npcdex]->make_angry();
     if (active_npc[npcdex]->hp_cur[hp_head]  <= 0 ||
         active_npc[npcdex]->hp_cur[hp_torso] <= 0   ) {
         active_npc[npcdex]->die(this, true);
     }
     return false;
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
   return false;
  }
 }

 if (u.has_disease("in_pit")) {
  if (rng(0, 40) > u.str_cur + int(u.dex_cur / 2)) {
   add_msg(_("You try to escape the pit, but slip back in."));
   u.moves -= 100;
   return false;
  } else {
   add_msg(_("You escape the pit!"));
   u.rem_disease("in_pit");
  }
 }
 if (u.has_disease("downed")) {
  if (rng(0, 40) > u.dex_cur + int(u.str_cur / 2)) {
   add_msg(_("You struggle to stand."));
   u.moves -= 100;
   return false;
  } else {
   add_msg(_("You stand up."));
   u.rem_disease("downed");
   u.moves -= 100;
   return false;
  }
 }

// GRAB: pre-action checking.
 int vpart0 = -1, vpart1 = -1, dpart = -1;
 vehicle *veh0 = m.veh_at(u.posx, u.posy, vpart0);
 vehicle *veh1 = m.veh_at(x, y, vpart1);
 bool pushing_furniture = false;  // moving -into- furniture tile; skip check for move_cost > 0
 bool pulling_furniture = false;  // moving -away- from furniture tile; check for move_cost > 0
 bool shifting_furniture = false; // moving furniture and staying still; skip check for move_cost > 0
 int movecost_modifier = 0;       // pulling moves furniture into our origin square, so this changes to subtract it.

 if( u.grab_point.x != 0 || u.grab_point.y ) {
     if ( u.grab_type == OBJECT_VEHICLE ) { // default; assume OBJECT_VEHICLE
         vehicle *grabbed_vehicle = m.veh_at( u.posx + u.grab_point.x, u.posy + u.grab_point.y );
         // If we're pushing a vehicle, the vehicle tile we'd be "stepping onto" is
         // actually the current tile.
         // If there's a vehicle there, it will actually result in failed movement.
         if( grabbed_vehicle == veh1 ) {
             veh1 = veh0;
             vpart1 = vpart0;
         }
     } else if ( u.grab_type == OBJECT_FURNITURE ) { // Determine if furniture grab is valid, and what we're wanting to do with it based on
         point fpos( u.posx + u.grab_point.x, u.posy + u.grab_point.y ); // where it is
         if ( m.has_furn(fpos.x, fpos.y) ) {
             pushing_furniture = ( dx == u.grab_point.x && dy == u.grab_point.y); // and where we're going
             if ( ! pushing_furniture ) {
                 point fdest( fpos.x + dx, fpos.y + dy );
                 pulling_furniture = ( fdest.x == u.posx && fdest.y == u.posy );
             }
             shifting_furniture = ( pushing_furniture == false && pulling_furniture == false );
         }
     }
 }
 bool veh_closed_door = false;
 if (veh1) {
  dpart = veh1->part_with_feature (vpart1, "OPENABLE");
  if (veh1->part_info(dpart).has_flag("OPENCLOSE_INSIDE") && (!veh0 || veh0 != veh1)){
      veh_closed_door = false;
  } else {
      veh_closed_door = dpart >= 0 && !veh1->parts[dpart].open;
  }
 }

 if (veh0 && abs(veh0->velocity) > 100) {
  if (!veh1) {
   if (query_yn(_("Dive from moving vehicle?"))) {
    moving_vehicle_dismount(x, y);
   }
   return false;
  } else if (veh1 != veh0) {
   add_msg(_("There is another vehicle in the way."));
   return false;
  } else if (veh1->part_with_feature(vpart1, "BOARDABLE") < 0) {
   add_msg(_("That part of the vehicle is currently unsafe."));
   return false;
  }
 }


 if (m.move_cost(x, y) > 0 || pushing_furniture || shifting_furniture ) {
    // move_cost() of 0 = impassible (e.g. a wall)
  u.set_underwater(false);

  //Ask for EACH bad field, maybe not? Maybe say "theres X bad shit in there don't do it."
  field_entry *cur = NULL;
  field &tmpfld = m.field_at(x, y);
    std::map<field_id, field_entry*>::iterator field_it;
    for (field_it = tmpfld.getFieldStart(); field_it != tmpfld.getFieldEnd(); ++field_it) {
        cur = field_it->second;
        if (cur == NULL) {
            continue;
        }
        field_id curType = cur->getFieldType();
        bool dangerous = false;

        switch (curType) {
            case fd_smoke:
                dangerous = !(u.resist(bp_mouth) >= 7);
                break;
            case fd_tear_gas:
            case fd_toxic_gas:
            case fd_gas_vent:
                dangerous = !(u.resist(bp_mouth) >= 15);
                break;
            default:
                dangerous = cur->is_dangerous();
                break;
        }
        if ((dangerous) && !query_yn(_("Really step into that %s?"), cur->name().c_str())) {
            return false;
    }
    }

  if (m.tr_at(x, y) != tr_null &&
    u.per_cur - u.encumb(bp_eyes) >= traps[m.tr_at(x, y)]->visibility){
        if (  !traps[m.tr_at(x, y)]->is_benign() &&
              !query_yn(_("Really step onto that %s?"),traps[m.tr_at(x, y)]->name.c_str())){
            return false;
        }
  }

  float drag_multiplier = 1.0;
  vehicle *grabbed_vehicle = NULL;
  if( u.grab_point.x != 0 || u.grab_point.y != 0 ) {
    // vehicle: pulling, pushing, or moving around the grabbed object.
    if ( u.grab_type == OBJECT_VEHICLE ) {
      grabbed_vehicle = m.veh_at( u.posx + u.grab_point.x, u.posy + u.grab_point.y );
      if( NULL != grabbed_vehicle ) {
          if( grabbed_vehicle == veh0 ) {
              add_msg(_("You can't move %s while standing on it!"), grabbed_vehicle->name.c_str());
              return false;
          }
          drag_multiplier += (float)(grabbed_vehicle->total_mass() * 1000) /
              (float)(u.weight_capacity() * 5);
          if( drag_multiplier > 2.0 ) {
              add_msg(_("The %s is too heavy for you to budge!"), grabbed_vehicle->name.c_str());
              return false;
          }
          tileray mdir;

          int dxVeh = u.grab_point.x * (-1);
          int dyVeh = u.grab_point.y * (-1);
          int prev_grab_x = u.grab_point.x;
          int prev_grab_y = u.grab_point.y;

          if (abs(dx+dxVeh) == 2 || abs(dy+dyVeh) == 2 || ((dxVeh + dx) == 0 && (dyVeh + dy) == 0))  {
              //We are not moving around the veh
              if ((dxVeh + dx) == 0 && (dyVeh + dy) == 0) {
                  //we are pushing in the direction of veh
                  dxVeh = dx;
                  dyVeh = dy;
              } else {
                  u.grab_point.x = dx * (-1);
                  u.grab_point.y = dy * (-1);
              }

              if ((abs(dx+dxVeh) == 0 || abs(dy+dyVeh) == 0) && u.grab_point.x != 0 && u.grab_point.y != 0) {
                  //We are moving diagonal while veh is diagonal too and one direction is 0
                  dxVeh = ((dx + dxVeh) == 0) ? 0 : dxVeh;
                  dyVeh = ((dy + dyVeh) == 0) ? 0 : dyVeh;

                  u.grab_point.x = dxVeh * (-1);
                  u.grab_point.y = dyVeh * (-1);
              }

              mdir.init( dxVeh, dyVeh );
              mdir.advance( 1 );
              grabbed_vehicle->precalc_mounts( 1, mdir.dir() );
              int imp = 0;
              std::vector<veh_collision> veh_veh_colls;
              bool can_move = true;
              // Set player location to illegal value so it can't collide with vehicle.
              int player_prev_x = u.posx;
              int player_prev_y = u.posy;
              u.posx = 0;
              u.posy = 0;
              if( grabbed_vehicle->collision( veh_veh_colls, dxVeh, dyVeh, can_move, imp, true ) ) {
                  // TODO: figure out what we collided with.
                  add_msg( _("The %s collides with something."), grabbed_vehicle->name.c_str() );
                  u.moves -= 10;
                  u.posx = player_prev_x;
                  u.posy = player_prev_y;
                  u.grab_point.x = prev_grab_x;
                  u.grab_point.y = prev_grab_y;
                  return false;
              }
              u.posx = player_prev_x;
              u.posy = player_prev_y;

              int gx = grabbed_vehicle->global_x();
              int gy = grabbed_vehicle->global_y();
              std::vector<int> wheel_indices = grabbed_vehicle->all_parts_with_feature("WHEEL", false);
              for( int i = 0; i < wheel_indices.size(); i++ ) {
                  int p = wheel_indices[i];
                  if( one_in(2) ) {
                      grabbed_vehicle->handle_trap( gx + grabbed_vehicle->parts[p].precalc_dx[0] + dxVeh,
                                                    gy + grabbed_vehicle->parts[p].precalc_dy[0] + dyVeh, p );
                  }
              }
              m.displace_vehicle( this, gx, gy, dxVeh, dyVeh );
          } else {
              //We are moving around the veh
              u.grab_point.x = (dx + dxVeh) * (-1);
              u.grab_point.y = (dy + dyVeh) * (-1);
          }
      } else {
          add_msg( _("No vehicle at grabbed point.") );
          u.grab_point.x = 0;
          u.grab_point.y = 0;
          u.grab_type = OBJECT_NONE;
      }
    // Furniture: pull, push, or standing still and nudging object around. Can push furniture out of reach.
    } else if ( u.grab_type == OBJECT_FURNITURE ) {
      point fpos( u.posx + u.grab_point.x, u.posy + u.grab_point.y ); // supposed position of grabbed furniture
      if ( ! m.has_furn( fpos.x, fpos.y ) ) { // where'd it go? We're grabbing thin air so reset.
          add_msg( _("No furniture at grabbed point.") );
          u.grab_point = point(0, 0);
          u.grab_type = OBJECT_NONE;
      } else {
          point fdest( fpos.x + dx, fpos.y + dy ); // intended destination of furniture.

          // unfortunately, game::is_empty fails for tiles we're standing on, which will forbid pulling, so:
          bool canmove = (
               ( m.move_cost(fdest.x, fdest.y) > 0 || m.has_flag("LIQUID", fdest.x, fdest.y) ) &&
               npc_at(fdest.x, fdest.y) == -1 &&
               mon_at(fdest.x, fdest.y) == -1 &&
               m.has_flag("FLAT", fdest.x, fdest.y) &&
               !m.has_furn(fdest.x, fdest.y) &&
               m.tr_at(fdest.x, fdest.y) == tr_null
          );

          const furn_t furntype = m.furn_at(fpos.x, fpos.y);
          int furncost = furntype.movecost;
          const int src_items = m.i_at(fpos.x, fpos.y).size();
          const int dst_items = m.i_at(fdest.x, fdest.y).size();
          bool dst_item_ok = ( ! m.has_flag("NOITEM", fdest.x, fdest.y) &&
                               ! m.has_flag("SWIMMABLE", fdest.x, fdest.y) &&
                               ! m.has_flag("DESTROY_ITEM", fdest.x, fdest.y) );
          bool src_item_ok = ( m.furn_at(fpos.x, fpos.y).has_flag("CONTAINER") ||
                               m.furn_at(fpos.x, fpos.y).has_flag("SEALED") );

          if ( ! canmove ) {
              add_msg( _("The %s collides with something."), furntype.name.c_str() );
              u.moves -= 50; // "oh was that your foot? Sorry :-O"
              return false;
          } else if ( !m.can_move_furniture( fpos.x, fpos.y, &u ) &&
                     one_in(std::max(20 - furntype.move_str_req - u.str_cur, 2)) ) {
              add_msg(_("You strain yourself trying to move the heavy %s!"), furntype.name.c_str() );
              u.moves -= 100;
              u.pain++; // Hurt ourself.
              return false; // furniture and or obstacle wins.
          } else if ( ! src_item_ok && dst_items > 0 ) {
              add_msg( _("There's stuff in the way.") );
              u.moves -= 50; // "oh was that your stuffed parrot? Sorry :-O"
              return false;
          }

          if ( pulling_furniture ) { // normalize movecost for pulling: furniture moves into our current square -then- we move away
              if ( furncost < 0 ) {  // this will make our exit-tile move cost 0
                   movecost_modifier += m.ter_at(fpos.x, fpos.y).movecost; // so add the base cost of our exit-tile's terrain.
              } else {               // or it will think we're walking over the furniture we're pulling
                   movecost_modifier += ( 0 - furncost ); // so subtract the base cost of our furniture.
              }
          }

          int str_req = furntype.move_str_req;
          u.moves -= str_req * 10;
          // Additional penalty if we can't comfortably move it.
          if (!m.can_move_furniture(fpos.x, fpos.y, &u)) {
              int move_penalty = std::min((int)pow(str_req, 2) + 100, 1000);
              u.moves -= move_penalty;
              if (move_penalty > 500) {
                if (one_in(3)) // Nag only occasionally.
                  add_msg( _("Moving the heavy %s is taking a lot of time!"), furntype.name.c_str() );
              }
              else if (move_penalty > 200) {
                if (one_in(3)) // Nag only occasionally.
                  add_msg( _("It takes some time to move the heavy %s."), furntype.name.c_str() );
              }
          }
          sound(x, y, furntype.move_str_req * 2, _("a scraping noise."));

          m.furn_set(fdest.x, fdest.y, m.furn(fpos.x, fpos.y));    // finally move it.
          m.furn_set(fpos.x, fpos.y, f_null);

          if ( src_items > 0 ) {  // and the stuff inside.
              if ( dst_item_ok && src_item_ok ) {
                  std::vector <item>& miat = m.i_at(fpos.x, fpos.y);
                  const int arbritrary_item_limit = MAX_ITEM_IN_SQUARE - dst_items; // within reason
                  for (int i=0; i < src_items; i++) { // ...carefully
                      if ( i < arbritrary_item_limit &&
                        miat.size() > 0 &&
                        m.add_item_or_charges(fdest.x, fdest.y, miat[0], 0) ) {
                          miat.erase(miat.begin());
                      } else {
                          add_msg("Stuff spills from the %s!", furntype.name.c_str() );
                          break;
                      }
                  }
              } else {
                  add_msg("Stuff spills from the %s!", furntype.name.c_str() );
              }
          }

          if ( shifting_furniture ) { // we didn't move
              if ( abs( u.grab_point.x + dx ) < 2 && abs( u.grab_point.y + dy ) < 2 ) {
                  u.grab_point = point ( u.grab_point.x + dx , u.grab_point.y + dy ); // furniture moved relative to us
              } else { // we pushed furniture out of reach
                  add_msg( _("You let go of the %s"), furntype.name.c_str() );
                  u.grab_point = point (0, 0);
                  u.grab_type = OBJECT_NONE;
              }
              return false; // We moved furniture but stayed still.
          } else if ( pushing_furniture && m.move_cost(x, y) <= 0 ) { // Not sure how that chair got into a wall, but don't let player follow.
              add_msg( _("You let go of the %s as it slides past %s"), furntype.name.c_str(), m.ter_at(x,y).name.c_str() );
              u.grab_point = point (0, 0);
              u.grab_type = OBJECT_NONE;
          }
      }
    // Unsupported!
    } else {
       add_msg( _("Nothing at grabbed point %d,%d."),u.grab_point.x,u.grab_point.y );
       u.grab_point.x = 0;
       u.grab_point.y = 0;
       u.grab_type = OBJECT_NONE;
    }
  }

// Calculate cost of moving
  bool diag = trigdist && u.posx != x && u.posy != y;
  u.moves -= int(u.run_cost(m.combined_movecost(u.posx, u.posy, x, y, grabbed_vehicle, movecost_modifier), diag) * drag_multiplier);

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
    add_msg(_("Moving past this %s is slow!"), veh1->part_info(vpart1).name.c_str());
   else
    add_msg(_("Moving past this %s is slow!"), m.name(x, y).c_str());
  }
  if (m.has_flag("ROUGH", x, y) && (!u.in_vehicle)) {
   if (one_in(5) && u.armor_bash(bp_feet) < rng(2, 5)) {
    add_msg(_("You hurt your feet on the %s!"), m.tername(x, y).c_str());
    u.hit(this, bp_feet, 0, 0, 1);
    u.hit(this, bp_feet, 1, 0, 1);
   }
  }
  if (m.has_flag("SHARP", x, y) && !one_in(3) && !one_in(40 - int(u.dex_cur/2))
      && (!u.in_vehicle)) {
   if (!u.has_trait("PARKOUR") || one_in(4)) {
    body_part bp = random_body_part();
    int side = random_side(bp);
    if(u.hit(this, bp, side, 0, rng(1, 4)) > 0)
     add_msg(_("You cut your %s on the %s!"), body_part_name(bp, side).c_str(), m.tername(x, y).c_str());
   }
  }
  if (!u.has_artifact_with(AEP_STEALTH) && !u.has_trait("LEG_TENTACLES")) {
   if (u.has_trait("LIGHTSTEP"))
    sound(x, y, 2, ""); // Sound of footsteps may awaken nearby monsters
   else
    sound(x, y, 6, ""); // Sound of footsteps may awaken nearby monsters
  }
  if (one_in(20) && u.has_artifact_with(AEP_MOVEMENT_NOISE))
   sound(x, y, 40, _("You emit a rattling sound."));
// If we moved out of the nonant, we need update our map data
  if (m.has_flag("SWIMMABLE", x, y) && u.has_disease("onfire")) {
   add_msg(_("The water puts out the flames!"));
   u.rem_disease("onfire");
  }
// displace is set at the top of this function.
  if (displace) { // We displaced a friendly monster!
// Immobile monsters can't be displaced.
   monster &z = zombie(mondex);
   if (z.has_flag(MF_IMMOBILE)) {
// ...except that turrets can be picked up.
// TODO: Make there a flag, instead of hard-coded to mon_turret
    if (z.type->id == "mon_turret") {
     if (query_yn(_("Deactivate the turret?"))) {
      remove_zombie(mondex);
      u.moves -= 100;
      m.spawn_item(x, y, "bot_turret", 1, 0, turn);
      m.spawn_item(x, y, "9mm", 1, z.ammo, turn);
     }
     return false;
    }
    else if (z.type->id == "mon_laserturret") {
     if (query_yn(_("Deactivate the laser turret?"))) {
      remove_zombie(mondex);
      u.moves -= 100;
      m.spawn_item(x, y, "bot_laserturret", 1, 0, turn);
     }
     return false;
    } else {
     add_msg(_("You can't displace your %s."), z.name().c_str());
     return false;
    }
   z.move_to(this, u.posx, u.posy, true); // Force the movement even though the player is there right now.
   add_msg(_("You displace the %s."), z.name().c_str());
   }
   else if (z.type->id == "mon_manhack") {
    if (query_yn(_("Reprogram the manhack?"))) {
      int choice = 0;
      if (z.has_effect(ME_DOCILE))
        choice = menu(true, _("Do what?"), _("Engage targets."), _("Deactivate."), NULL);
      else
        choice = menu(true, _("Do what?"), _("Follow me."), _("Deactivate."), NULL);
      switch (choice) {
      case 1:{
        if (z.has_effect(ME_DOCILE)) {
          z.rem_effect(ME_DOCILE);
          if (one_in(3))
            add_msg(_("The %s hovers momentarily as it surveys the area."), z.name().c_str());
        }
        else {
          z.add_effect(ME_DOCILE, -1);
          add_msg(_("The %s ."), z.name().c_str());
          if (one_in(3))
            add_msg(_("The %s lets out a whirring noise and starts to follow you."), z.name().c_str());
        }
        break;
      }
      case 2: {
        remove_zombie(mondex);
        m.spawn_item(x, y, "bot_manhack", 1, 0, turn);
        break;
      }
      default: {
        return false;
      }
      }
      u.moves -= 100;
    }
    return false;
  }
   z.move_to(this, u.posx, u.posy, true); // Force the movement even though the player is there right now.
   add_msg(_("You displace the %s."), z.name().c_str());
  }

  if (x < SEEX * int(MAPSIZE / 2) || y < SEEY * int(MAPSIZE / 2) ||
      x >= SEEX * (1 + int(MAPSIZE / 2)) || y >= SEEY * (1 + int(MAPSIZE / 2)))
   update_map(x, y);

// If the player is in a vehicle, unboard them from the current part
  if (u.in_vehicle)
   m.unboard_vehicle(u.posx, u.posy);

// Move the player
  u.posx = x;
  u.posy = y;
// Increase our scent.
  if (u.scent < u.norm_scent * 2)
    u.scent *= 1.2;
  if(dx != 0 || dy != 0) {
    u.lifetime_stats()->squares_walked++;
  }

  //Autopickup
  if (OPTIONS["AUTO_PICKUP"] && (!OPTIONS["AUTO_PICKUP_SAFEMODE"] || mostseen == 0) && (m.i_at(u.posx, u.posy)).size() > 0) {
   pickup(u.posx, u.posy, -1);
  }

// If the new tile is a boardable part, board it
  if (veh1 && veh1->part_with_feature(vpart1, "BOARDABLE") >= 0)
   m.board_vehicle(this, u.posx, u.posy, &u);

  if (m.tr_at(x, y) != tr_null) { // We stepped on a trap!
   trap* tr = traps[m.tr_at(x, y)];
   if (!u.avoid_trap(tr)) {
    trapfunc f;
    (f.*(tr->act))(x, y);
   }
  }

  // apply martial art move bonuses
  u.ma_onmove_effects();

  // leave the old martial arts stuff in for now
// Some martial art styles have special effects that trigger when we move
  if(u.weapon.type->id == "style_capoeira"){
    if (u.disease_duration("attack_boost") < 2)
     u.add_disease("attack_boost", 2, false, 2, 2);
    if (u.disease_duration("dodge_boost") < 2)
     u.add_disease("dodge_boost", 2, false, 2, 2);
  } else if(u.weapon.type->id == "style_ninjutsu"){
    u.add_disease("attack_boost", 2, false, 1, 3);
  } else if(u.weapon.type->id == "style_crane"){
    if (!u.has_disease("dodge_boost"))
     u.add_disease("dodge_boost", 1, false, 3, 3);
  } else if(u.weapon.type->id == "style_leopard"){
    u.add_disease("attack_boost", 2, false, 1, 4);
  } else if(u.weapon.type->id == "style_dragon"){
    if (!u.has_disease("damage_boost"))
     u.add_disease("damage_boost", 2, false, 3, 3);
  } else if(u.weapon.type->id == "style_lizard"){
    bool wall = false;
    for (int wallx = x - 1; wallx <= x + 1 && !wall; wallx++) {
     for (int wally = y - 1; wally <= y + 1 && !wall; wally++) {
      if (m.has_flag("SUPPORTS_ROOF", wallx, wally))
       wall = true;
     }
    }
    if (wall)
     u.add_disease("attack_boost", 2, false, 2, 8);
    else
     u.rem_disease("attack_boost");
  }

  // Drench the player if swimmable
  if (m.has_flag("SWIMMABLE", x, y))
    u.drench(this, 40, mfb(bp_feet) | mfb(bp_legs));

    // List items here
    if (!m.has_flag("SEALED", x, y)) {
        if (u.has_disease("blind") && !m.i_at(x, y).empty()) {
            add_msg(_("There's something here, but you can't see what it is."));
        } else if (!m.i_at(x, y).empty()) {
            std::vector<std::string> names;
            std::vector<size_t> counts;
            names.push_back(m.i_at(x, y)[0].tname(this));
            if (m.i_at(x, y)[0].count_by_charges()) {
                counts.push_back(m.i_at(x, y)[0].charges);
            } else {
                counts.push_back(1);
            }
            for (int i = 1; i < m.i_at(x, y).size(); i++) {
                item& tmpitem = m.i_at(x, y)[i];
                std::string next = tmpitem.tname(this);
                bool got_it = false;
                for (int i = 0; i < names.size(); ++i) {
                    if (next == names[i]) {
                        if (tmpitem.count_by_charges()) {
                            counts[i] += tmpitem.charges;
                        } else {
                            counts[i] += 1;
                        }
                        got_it = true;
                        break;
                    }
                }
                if (!got_it) {
                    names.push_back(next);
                    if (tmpitem.count_by_charges()) {
                        counts.push_back(tmpitem.charges);
                    } else {
                        counts.push_back(1);
                    }
                }
                if (names.size() > 6) {
                    break;
                }
            }
            for (int i = 0; i < names.size(); ++i) {
                std::string fmt;
                if (counts[i] == 1) {
                    //~ one item (e.g. "a dress")
                    fmt = _("a %s");
                    names[i] = string_format(fmt, names[i].c_str());
                } else {
                    //~ number of items: "<number> <item>"
                    fmt = ngettext("%1$d %2$s", "%1$d %2$ss", counts[i]);
                    names[i] = string_format(fmt, counts[i], names[i].c_str());
                }
            }
            if (names.size() == 1) {
                add_msg(_("You see here %s."), names[0].c_str());
            } else if (names.size() == 2) {
                add_msg(_("You see here %s and %s."),
                        names[0].c_str(), names[1].c_str());
            } else if (names.size() == 3) {
                add_msg(_("You see here %s, %s, and %s."), names[0].c_str(),
                        names[1].c_str(), names[2].c_str());
            } else if (names.size() < 7) {
                add_msg(_("There are %d items here."), names.size());
            } else {
                add_msg(_("There are many items here."));
            }
        }
    }

  if (veh1 && veh1->part_with_feature(vpart1, "CONTROLS") >= 0
           && u.in_vehicle)
      add_msg(_("There are vehicle controls here.  %s to drive."),
              press_x(ACTION_CONTROL_VEHICLE).c_str() );

  } else if (!m.has_flag("SWIMMABLE", x, y) && u.has_active_bionic("bio_probability_travel") && u.power_level >= 10) {
  //probability travel through walls but not water
  int tunneldist = 0;
  // tile is impassable
  while((m.move_cost(x + tunneldist*(x - u.posx), y + tunneldist*(y - u.posy)) == 0 &&
         // but allow water tiles
         !m.has_flag("SWIMMABLE", x + tunneldist*(x - u.posx), y + tunneldist*(y - u.posy))) ||
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
        m.unboard_vehicle(u.posx, u.posy);
    u.power_level -= (tunneldist * 10); //tunneling costs 10 bionic power per impassable tile
    u.moves -= 100; //tunneling costs 100 moves
    u.posx += (tunneldist + 1) * (x - u.posx); //move us the number of tiles we tunneled in the x direction, plus 1 for the last tile
    u.posy += (tunneldist + 1) * (y - u.posy); //ditto for y
    add_msg(_("You quantum tunnel through the %d-tile wide barrier!"), tunneldist);
    if (m.veh_at(u.posx, u.posy, vpart1) && m.veh_at(u.posx, u.posy, vpart1)->part_with_feature(vpart1, "BOARDABLE") >= 0)
        m.board_vehicle(this, u.posx, u.posy, &u);
  }
  else //or you couldn't tunnel due to lack of energy
  {
      u.power_level -= 10; //failure is expensive!
      return false;
  }

 } else if (veh_closed_door) { // move_cost <= 0
   veh1->open(dpart);
  u.moves -= 100;
  add_msg (_("You open the %s's %s."), veh1->name.c_str(),
                                    veh1->part_info(dpart).name.c_str());

 } else if (m.has_flag("SWIMMABLE", x, y)) { // Dive into water!
// Requires confirmation if we were on dry land previously
  if ((m.has_flag("SWIMMABLE", u.posx, u.posy) &&
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
  return false;
 }

 return true;
}

void game::plswim(int x, int y)
{
 if (x < SEEX * int(MAPSIZE / 2) || y < SEEY * int(MAPSIZE / 2) ||
     x >= SEEX * (1 + int(MAPSIZE / 2)) || y >= SEEY * (1 + int(MAPSIZE / 2)))
  update_map(x, y);
 u.posx = x;
 u.posy = y;
 if (!m.has_flag("SWIMMABLE", x, y)) {
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
 u.practice(turn, "swimming", u.is_underwater() ? 2 : 1);
 if (movecost >= 500) {
  if (!u.is_underwater()) {
    add_msg(_("You sink like a rock!"));
   u.set_underwater(true);
   u.oxygen = 30 + 2 * u.str_cur;
  }
 }
 if (u.oxygen <= 5 && u.is_underwater()) {
  if (movecost < 500)
    popup(_("You need to breathe! (%s to surface.)"),
          press_x(ACTION_MOVE_UP).c_str());
  else
   popup(_("You need to breathe but you can't swim!  Get to dry land, quick!"));
 }
 bool diagonal = (x != u.posx && y != u.posy);
 u.moves -= (movecost > 200 ? 200 : movecost)  * (trigdist && diagonal ? 1.41 : 1 );
 u.inv.rust_iron_items();

 int drenchFlags = mfb(bp_legs)|mfb(bp_torso)|mfb(bp_arms)|mfb(bp_feet);

 if (get_temperature() <= 50)
   drenchFlags |= mfb(bp_hands);

 if (u.is_underwater())
   drenchFlags |= mfb(bp_head)|mfb(bp_eyes)|mfb(bp_mouth)|mfb(bp_hands);

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
    if (is_player)
    {
        if (is_u)
            sname = std::string (_("You are"));
        else
            sname = p->name + _(" is");
    }
    else
        sname = zz->name() + _(" is");
    int range = flvel / 10;
    int x = (is_player? p->posx : zz->posx());
    int y = (is_player? p->posy : zz->posy());
    while (range > 0)
    {
        tdir.advance();
        x = (is_player? p->posx : zz->posx()) + tdir.dx();
        y = (is_player? p->posy : zz->posy()) + tdir.dy();
        std::string dname;
        bool thru = true;
        bool slam = false;
        int mondex = mon_at(x, y);
        dam1 = flvel / 3 + rng (0, flvel * 1 / 3);
        if (controlled)
            dam1 = std::max(dam1 / 2 - 5, 0);
        if (mondex >= 0)
        {
            monster &z = zombie(mondex);
            slam = true;
            dname = z.name();
            dam2 = flvel / 3 + rng (0, flvel * 1 / 3);
            if (z.hurt(dam2))
             kill_mon(mondex, false);
            else
             thru = false;
            if (is_player)
             p->hitall (this, dam1, 40);
            else
                zz->hurt(dam1);
        } else if (m.move_cost(x, y) == 0 && !m.has_flag("SWIMMABLE", x, y)) {
            slam = true;
            int vpart;
            vehicle *veh = m.veh_at(x, y, vpart);
            dname = veh ? veh->part_info(vpart).name : m.tername(x, y).c_str();
            if (m.has_flag("BASHABLE", x, y)) {
                thru = m.bash(x, y, flvel, snd);
            } else {
                thru = false;
            }
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
                zz->setpos(x, y);
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

    if (!m.has_flag("SWIMMABLE", x, y))
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

void game::vertical_move(int movez, bool force) {
// Check if there are monsters are using the stairs.
    bool slippedpast = false;
    if (!coming_to_stairs.empty()) {
		// TODO: Allow travel if zombie couldn't reach stairs, but spawn him when we go up.
            add_msg(_("You try to use the stairs. Suddenly you are blocked by a %s!"), coming_to_stairs[0].name().c_str());
            // Roll.
            int dexroll = dice(6, u.dex_cur + u.skillLevel("dodge") * 2);
            int strroll = dice(3, u.str_cur + u.skillLevel("melee") * 1.5);
            if (coming_to_stairs.size() > 4) {
                add_msg(_("The are a lot of them on the %s!"), m.tername(u.posx, u.posy).c_str());
                dexroll /= 4;
                strroll /= 2;
            }
            else if (coming_to_stairs.size() > 1) {
                 add_msg(_("There's something else behind it!"));
                 dexroll /= 2;
            }

            if (dexroll < 14 || strroll < 12) {
                update_stair_monsters();
                u.moves -= 100;
                return;
            }

            if (dexroll >= 14)
                add_msg(_("You manage to slip past!"));
            else if (strroll >= 12)
                add_msg(_("You manage to push past!"));
            slippedpast = true;
            u.moves -=100;
    }

// > and < are used for diving underwater.
 if (m.move_cost(u.posx, u.posy) == 0 && m.has_flag("SWIMMABLE", u.posx, u.posy)){
  if (movez == -1) {
   if (u.is_underwater()) {
    add_msg(_("You are already underwater!"));
    return;
   }
   if (u.worn_with_flag("FLOATATION")) {
    add_msg(_("You can't dive while wearing a flotation device."));
    return;
   }
   u.set_underwater(true);
   u.oxygen = 30 + 2 * u.str_cur;
   add_msg(_("You dive underwater!"));
  } else {
   if (u.swim_speed() < 500) {
    u.set_underwater(false);
    add_msg(_("You surface."));
   } else
    add_msg(_("You can't surface!"));
  }
  return;
 }
// Force means we're going down, even if there's no staircase, etc.
// This happens with sinkholes and the like.
 if (!force && ((movez == -1 && !m.has_flag("GOES_DOWN", u.posx, u.posy)) ||
                (movez ==  1 && !m.has_flag("GOES_UP",   u.posx, u.posy))) &&
                !(m.ter(u.posx, u.posy) == t_elevator)) {
  if (movez == -1) {
    add_msg(_("You can't go down here!"));
  } else {
    add_msg(_("You can't go up here!"));
  }
  return;
 }

 if( force ) {
     // Let go of a grabbed cart.
     u.grab_point.x = 0;
     u.grab_point.y = 0;
 } else if( u.grab_point.x != 0 || u.grab_point.y != 0 ) {
     // TODO: Warp the cart along with you if you're on an elevator
     add_msg(_("You can't drag things up and down stairs."));
     return;
 }

 map tmpmap(&traps);
 tmpmap.load(this, levx, levy, levz + movez, false);
// Find the corresponding staircase
 int stairx = -1, stairy = -1;
 bool rope_ladder = false;

    const int omtilesz=SEEX * 2;
    real_coords rc( m.getabs(u.posx, u.posy) );

    point omtile_align_start(
        m.getlocal( rc.begin_om_pos() )
    );

 if (force) {
  stairx = u.posx;
  stairy = u.posy;
 } else { // We need to find the stairs.
  int best = 999;
   for (int i = omtile_align_start.x; i <= omtile_align_start.x + omtilesz; i++) {
    for (int j = omtile_align_start.y; j <= omtile_align_start.y + omtilesz; j++) {
    if (rl_dist(u.posx, u.posy, i, j) <= best &&
        ((movez == -1 && tmpmap.has_flag("GOES_UP", i, j)) ||
         (movez == 1 && (tmpmap.has_flag("GOES_DOWN", i, j) ||
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

 if (!force) {
  monstairx = levx;
  monstairy = levy;
  monstairz = levz;
 }
 // Make sure monsters are saved!
 for (unsigned int i = 0; i < num_zombies(); i++) {
    monster &z = zombie(i);
    int turns = z.turns_to_reach(this, u.posx, u.posy);
        if (turns < 10 && coming_to_stairs.size() < 8 && z.will_reach(this, u.posx, u.posy)
            && !slippedpast) {
            z.onstairs = true;
            z.staircount = 10 + turns;
            coming_to_stairs.push_back(z);
            //remove_zombie(i);
        } else {
            force_save_monster(z);
        }
}
 despawn_monsters();
 clear_zombies();

// Figure out where we know there are up/down connectors
 std::vector<point> discover;
 for (int x = 0; x < OMAPX; x++) {
  for (int y = 0; y < OMAPY; y++) {
   if (cur_om->seen(x, y, levz) &&
       ((movez ==  1 && otermap[ cur_om->ter(x, y, levz) ].known_up) ||
        (movez == -1 && otermap[ cur_om->ter(x, y, levz) ].known_down) ))
    discover.push_back( point(x, y) );
  }
 }

 int z_coord = levz + movez;
 // Fill in all the tiles we know about (e.g. subway stations)
 for (int i = 0; i < discover.size(); i++) {
  int x = discover[i].x, y = discover[i].y;
  cur_om->seen(x, y, z_coord) = true;
  if (movez ==  1 && !otermap[ cur_om->ter(x, y, z_coord) ].known_down &&
      !cur_om->has_note(x, y, z_coord))
   cur_om->add_note(x, y, z_coord, _("AUTO: goes down"));
  if (movez == -1 && !otermap[ cur_om->ter(x, y, z_coord) ].known_up &&
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
  m.spawn_item(stairx + rng(-1, 1), stairy + rng(-1, 1), "manhole_cover");
  m.ter_set(stairx, stairy, t_manhole);
 }

 m.spawn_monsters(this);

 if (force) { // Basically, we fell.
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
   (f.*(tr->act))(u.posx, u.posy);
  }
 }

 set_adjacent_overmaps(true);
 refresh_all();
}


void game::update_map(int &x, int &y) {
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
 if (shiftx || shifty) {
    despawn_monsters(shiftx, shifty);
    u.shift_destination(-shiftx * SEEX, -shifty * SEEY);
 }

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
 m.spawn_monsters(this); // Static monsters
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
     cost = otermap[cur_om->ter(lx, ly, levz)].see_cost;
    else if ((lx < 0 || lx >= OMAPX) && (ly < 0 || ly >= OMAPY)) {
     if (lx < 0) lx += OMAPX;
     else        lx -= OMAPX;
     if (ly < 0) ly += OMAPY;
     else        ly -= OMAPY;
     cost = otermap[om_diag->ter(lx, ly, levz)].see_cost;
    } else if (lx < 0 || lx >= OMAPX) {
     if (lx < 0) lx += OMAPX;
     else        lx -= OMAPX;
     cost = otermap[om_hori->ter(lx, ly, levz)].see_cost;
    } else if (ly < 0 || ly >= OMAPY) {
     if (ly < 0) ly += OMAPY;
     else        ly -= OMAPY;
     cost = otermap[om_vert->ter(lx, ly, levz)].see_cost;
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
 for (int i = 0; i < coming_to_stairs.size(); i++) {
    coming_to_stairs[i].onstairs = false;
    coming_to_stairs[i].staircount = 0;
    add_zombie(coming_to_stairs[i]);
 }
 coming_to_stairs.clear();
}

//TODO: abstract out the location checking code
//TODO: refactor so zombies can follow up and down stairs instead of this mess
void game::update_stair_monsters() {

    // Search for the stairs closest to the player.
    std::vector<int> stairx, stairy;
    std::vector<int> stairdist;

    if (!coming_to_stairs.empty()) {
        for (int x = 0; x < SEEX * MAPSIZE; x++) {
            for (int y = 0; y < SEEY * MAPSIZE; y++) {
                if (m.has_flag("GOES_UP", x, y) || m.has_flag("GOES_DOWN", x, y)) {
                    stairx.push_back(x);
                    stairy.push_back(y);
                    stairdist.push_back(rl_dist(x, y, u.posx, u.posy));
                }
            }
        }
        if (stairdist.empty())
            return;         // Found no stairs?

        // Find closest stairs.
        int si = 0;
        for (int i = 0; i < stairdist.size(); i++) {
            if (stairdist[i] < stairdist[si])
                si = i;
        }

        // Attempt to spawn zombies.
        for (int i = 0; i < coming_to_stairs.size(); i++) {
            int mposx = stairx[si], mposy = stairy[si];
            monster &z = coming_to_stairs[i];

            // We might be not be visible.
            if (!( z.posx() < 0 - (SEEX * MAPSIZE) / 6 ||
                    z.posy() < 0 - (SEEY * MAPSIZE) / 6 ||
                    z.posx() > (SEEX * MAPSIZE * 7) / 6 ||
                    z.posy() > (SEEY * MAPSIZE * 7) / 6 ) ) {

                coming_to_stairs[i].staircount -= 4;
                // Let the player know zombies are trying to come.
                z.setpos(mposx, mposy, true);
                if (u_see(mposx, mposy)) {
                    std::stringstream dump;
                    if (coming_to_stairs[i].staircount > 4)
                        dump << _("You see a ") << z.name() << _(" on the stairs!");
                    else
                        dump << _("The ") << z.name() << _(" is almost at the ")
                        << (m.has_flag("GOES_UP", mposx, mposy) ? _("bottom") : _("top")) <<  _(" of the ")
                        << m.tername(mposx, mposy).c_str() << "!";
                    add_msg(dump.str().c_str());
                }
                else {
                    sound(mposx, mposy, 5, _("a sound nearby from the stairs!"));
                }

                if (is_empty(mposx, mposy) && coming_to_stairs[i].staircount <= 0) {
                    z.setpos(mposx, mposy, true);
                    z.onstairs = false;
                    z.staircount = 0;
                    add_zombie(z);
                    if (u_see(mposx, mposy)) {
                        if (m.has_flag("GOES_UP", mposx, mposy)) {
                            add_msg(_("The %s comes down the %s!"), z.name().c_str(),
                                    m.tername(mposx, mposy).c_str());
                        } else {
                            add_msg(_("The %s comes up the %s!"), z.name().c_str(),
                                    m.tername(mposx, mposy).c_str());
                        }
                    }
                    coming_to_stairs.erase(coming_to_stairs.begin() + i);
                } else if (u.posx == mposx && u.posy == mposy && z.staircount <= 0) {
                    // Search for a clear tile.
                    int pushx = -1, pushy = -1;
                    int tries = 0;
                    z.setpos(mposx, mposy, true);
                    while(tries < 9) {
                        pushx = rng(-1, 1), pushy = rng(-1, 1);
                        if (z.can_move_to(this, mposx + pushx, mposy + pushy) && pushx != 0 && pushy != 0) {
                            add_msg(_("The %s pushed you back!"), z.name().c_str());
                            u.posx += pushx;
                            u.posy += pushy;
                            u.moves -= 100;
                            // Stumble.
                            if (u.dodge(this) < 12)
                                u.add_disease("downed", 2);
                            return;
                        }
                        tries++;
                    }
                    add_msg(_("The %s tried to push you back but failed! It attacks you!"), z.name().c_str());
                    z.hit_player(this, u, false);
                    u.moves -= 100;
                    return;
                }
            }
        }
    }

    if (coming_to_stairs.empty()) {
        monstairx = -1;
        monstairy = -1;
        monstairz = 999;
    }
}

void game::force_save_monster(monster &z) {
    real_coords rc( m.getabs(z.posx(), z.posy() ) );
    z.spawnmapx = rc.om_sub.x;
    z.spawnmapy = rc.om_sub.y;
    z.spawnposx = rc.sub_pos.x;
    z.spawnposy = rc.sub_pos.y;

    tinymap tmp(&traps);
    tmp.load(this, z.spawnmapx, z.spawnmapy, levz, false);
    tmp.add_spawn(&z);
    tmp.save(cur_om, turn, z.spawnmapx, z.spawnmapy, levz);
}

void game::despawn_monsters(const int shiftx, const int shifty)
{
    for (unsigned int i = 0; i < num_zombies(); i++) {
        monster &z = zombie(i);
        // If either shift argument is non-zero, we're shifting.
        if(shiftx != 0 || shifty != 0) {
            z.shift(shiftx, shifty);
            if( z.posx() >= 0 && z.posx() <= SEEX * MAPSIZE &&
                    z.posy() >= 0 && z.posy() <= SEEY * MAPSIZE) {
                // We're inbounds, so don't despawn after all.
                continue;
            } else {
                if ( (z.spawnmapx != -1) || z.getkeep() ||
                          ((shiftx != 0 || shifty != 0) && z.friendly != 0 ) ) {
                    // translate shifty relative coordinates to submapx, submapy, subtilex, subtiley
                    real_coords rc( m.getabs(z.posx(), z.posy() ) ); // still madness, bud handles straddling omap and -/+
                    z.spawnmapx = rc.om_sub.x;
                    z.spawnmapy = rc.om_sub.y;
                    z.spawnposx = rc.sub_pos.x;
                    z.spawnposy = rc.sub_pos.y;

                    // We're saving him, so there's no need to keep anymore.
                    z.setkeep(false);

                    tinymap tmp(&traps);
                    tmp.load(this, z.spawnmapx, z.spawnmapy, levz, false);
                    tmp.add_spawn(&z);
                    tmp.save(cur_om, turn, z.spawnmapx, z.spawnmapy, levz);
                }
                else
                {
                    // No spawn site, so absorb them back into a group.
                    int group = valid_group((z.type->id), levx + shiftx, levy + shifty, levz);
                    if (group != -1)
                    {
                        cur_om->zg[group].population++;
                        if (cur_om->zg[group].population /
                                (cur_om->zg[group].radius * cur_om->zg[group].radius) > 5 &&
                                !cur_om->zg[group].diffuse)
                        {
                            cur_om->zg[group].radius++;
                        }
                    }
                }
                // Check if we should keep him.
                if (!z.getkeep()) {
                    remove_zombie(i);
                    i--;
                }
            }
        }
    }

    // The order in which zombies are shifted may cause zombies to briefly exist on
    // the same square. This messes up the mon_at cache, so we need to rebuild it.
    rebuild_mon_at_cache();
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
 if (ACTIVE_WORLD_OPTIONS["RANDOM_NPC"] && one_in(100 + 15 * cur_om->npcs.size())) {
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
    nextspawn += rng(group * 4 + num_zombies() * 4, group * 10 + num_zombies() * 10);

   for (int j = 0; j < group; j++) { // For each monster in the group get some spawn details
     MonsterGroupResult spawn_details = MonsterGroupManager::GetResultFromGroup( cur_om->zg[i].type,
                                                             &group, (int)turn );
     zom = monster(GetMType(spawn_details.name));
     for (int kk = 0; kk < spawn_details.pack_size; kk++){
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
        add_zombie(zom);
       }
     }
   } // Placing monsters of this group is done!
   if (cur_om->zg[i].population <= 0) { // Last monster in the group spawned...
    cur_om->zg.erase(cur_om->zg.begin() + i); // ...so remove that group
    i--; // And don't increment i.
   }
  }
 }
}

int game::valid_group(std::string type, int x, int y, int z_coord)
{
 std::vector <int> valid_groups;
 std::vector <int> semi_valid; // Groups that're ALMOST big enough
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

    if (bHasWatch) {
        as_m.entries.push_back(uimenu_entry(3, true, '3', _("1 hour") ));
        as_m.entries.push_back(uimenu_entry(4, true, '4', _("2 hours") ));
        as_m.entries.push_back(uimenu_entry(5, true, '5', _("3 hours") ));
        as_m.entries.push_back(uimenu_entry(6, true, '6', _("6 hours") ));
    }

    as_m.entries.push_back(uimenu_entry(7, true, 'd', _("Wait till dawn") ));
    as_m.entries.push_back(uimenu_entry(8, true, 'n', _("Wait till noon") ));
    as_m.entries.push_back(uimenu_entry(9, true, 'k', _("Wait till dusk") ));
    as_m.entries.push_back(uimenu_entry(10, true, 'm', _("Wait till midnight") ));
    as_m.entries.push_back(uimenu_entry(11, true, 'w', _("Wait till weather changes") ));

    as_m.entries.push_back(uimenu_entry(12, true, 'x', _("Exit") ));
    as_m.query(); /* calculate key and window variables, generate window, and loop until we get a valid answer */

    const int iHour = turn.getHour();

    int time = 0;
    activity_type actType = ACT_WAIT;

    switch (as_m.ret) {
        case 1:
            time =   5000;
            break;
        case 2:
            time =  30000;
            break;
        case 3:
            time =  60000;
            break;
        case 4:
            time = 120000;
            break;
        case 5:
            time = 180000;
            break;
        case 6:
            time = 360000;
            break;
        case 7:
            time = 60000 * ((iHour <= 6) ? 6-iHour : 24-iHour+6);
            break;
        case 8:
            time = 60000 * ((iHour <= 12) ? 12-iHour : 12-iHour+6);
            break;
        case 9:
            time = 60000 * ((iHour <= 18) ? 18-iHour : 18-iHour+6);
            break;
        case 10:
            time = 60000 * ((iHour <= 24) ? 24-iHour : 24-iHour+6);
            break;
        case 11:
            time = 999999999;
            actType = ACT_WAIT_WEATHER;
            break;
        default:
            return;
    }

    u.assign_activity(this, actType, time, 0);
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

  //Draw Scrollbar
  draw_scrollbar(w, offset, FULL_SCREEN_HEIGHT-2, messages.size(), 1);

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

void game::teleport(player *p, bool add_teleglow)
{
    if (p == NULL) {
        p = &u;
    }
    int newx, newy, tries = 0;
    bool is_u = (p == &u);

    if(add_teleglow) {
        p->add_disease("teleglow", 300);
    }
    do {
        newx = p->posx + rng(0, SEEX * 2) - SEEX;
        newy = p->posy + rng(0, SEEY * 2) - SEEY;
        tries++;
    } while (tries < 15 && !is_empty(newx, newy));
    bool can_see = (is_u || u_see(newx, newy));
    if (p->in_vehicle) {
        m.unboard_vehicle(p->posx, p->posy);
    }
    p->posx = newx;
    p->posy = newy;
    if (tries == 15) {
        if (m.move_cost(newx, newy) == 0) { // TODO: If we land in water, swim
            if (can_see) {
                if (is_u) {
                    add_msg(_("You teleport into the middle of a %s!"),
                            m.name(newx, newy).c_str());
                    p->add_memorial_log(_("Teleported into a %s."), m.name(newx, newy).c_str());
                } else {
                    add_msg(_("%s teleports into the middle of a %s!"),
                            p->name.c_str(), m.name(newx, newy).c_str());
                }
            }
            p->hurt(this, bp_torso, 0, 500);
        } else if (can_see) {
            const int i = mon_at(newx, newy);
            if (i != -1) {
                monster &z = zombie(i);
                if (is_u) {
                    add_msg(_("You teleport into the middle of a %s!"),
                            z.name().c_str());
                    u.add_memorial_log(_("Telefragged a %s."), z.name().c_str());
                } else {
                    add_msg(_("%s teleports into the middle of a %s!"),
                            p->name.c_str(), z.name().c_str());
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
    cur_om->ter(x, y, 0) = "crater";
    //Kill any npcs on that omap location.
    for(int i = 0; i < cur_om->npcs.size();i++)
        if(cur_om->npcs[i]->mapx/2== x && cur_om->npcs[i]->mapy/2 == y && cur_om->npcs[i]->omz == 0)
            cur_om->npcs[i]->marked_for_death = true;
}

bool game::spread_fungus(int x, int y)
{
    int growth = 1;
    for (int i = x - 1; i <= x + 1; i++) {
        for (int j = y - 1; j <= y + 1; j++) {
            if (i == x && j == y) {
                continue;
            }
            if (m.has_flag("FUNGUS", i, j)) {
                growth += 1;
            }
        }
    }

    bool converted = false;
    if (!m.has_flag_ter("FUNGUS", x, y)) {
        // Terrain conversion
        if (m.has_flag_ter("DIGGABLE", x, y)) {
            if (x_in_y(growth * 10, 100)) {
                m.ter_set(x, y, t_fungus);
                converted = true;
            }
        } else if (m.has_flag("FLAT", x, y)) {
            if (m.has_flag("INDOORS", x, y)) {
                if (x_in_y(growth * 10, 500)) {
                    m.ter_set(x, y, t_fungus_floor_in);
                    converted = true;
                }
            } else if (m.has_flag("SUPPORTS_ROOF", x, y)) {
                if (x_in_y(growth * 10, 1000)) {
                    m.ter_set(x, y, t_fungus_floor_sup);
                    converted = true;
                }
            } else {
                if (x_in_y(growth * 10, 2500)) {
                    m.ter_set(x, y, t_fungus_floor_out);
                    converted = true;
                }
            }
        } else if (m.has_flag("SHRUB", x, y)) {
            if (x_in_y(growth * 10, 200)) {
                m.ter_set(x, y, t_shrub_fungal);
                converted = true;
            } else if (x_in_y(growth, 1000)) {
                m.ter_set(x, y, t_marloss);
                converted = true;
            }
        } else if (m.has_flag("THIN_OBSTACLE", x, y)) {
            if (x_in_y(growth * 10, 150)) {
                m.ter_set(x, y, t_fungus_mound);
                converted = true;
            }
        } else if (m.has_flag("YOUNG", x, y)) {
            if (x_in_y(growth * 10, 500)) {
                m.ter_set(x, y, t_tree_fungal_young);
                converted = true;
            }
        } else if (m.has_flag("WALL", x, y)) {
            if (x_in_y(growth * 10, 5000)) {
                converted = true;
                if (m.ter_at(x, y).sym == LINE_OXOX) {
                    m.ter_set(x, y, t_fungus_wall_h);
                } else if (m.ter_at(x, y).sym == LINE_XOXO) {
                    m.ter_set(x, y, t_fungus_wall_v);
                } else {
                    m.ter_set(x, y, t_fungus_wall);
                }
            }
        }
        // Furniture conversion
        if (converted) {
            if (m.has_flag("FLOWER", x, y)){
                m.furn_set(x, y, f_flower_fungal);
            } else if (m.has_flag("ORGANIC", x, y)){
                if (m.furn_at(x, y).movecost == -10) {
                    m.furn_set(x, y, f_fungal_mass);
                } else {
                    m.furn_set(x, y, f_fungal_clump);
                }
            } else if (m.has_flag("PLANT", x, y)) {
                for (int k = 0; k < g->m.i_at(x, y).size(); k++) {
                    m.i_rem(x, y, k);
                }
                item seeds(itypes["fungal_seeds"], int(g->turn));
                m.add_item_or_charges(x, y, seeds);
            }
        }
        return true;
    } else {
        // Everything is already fungus
        if (growth == 9) {
            return false;
        }
        for (int i = x - 1; i <= x + 1; i++) {
            for (int j = y - 1; j <= y + 1; j++) {
                // One spread on average
                if (!m.has_flag("FUNGUS", i, j) && one_in(9 - growth)) {
                    //growth chance is 100 in X simplified
                    if (m.has_flag("DIGGABLE", i, j)) {
                        m.ter_set(i, j, t_fungus);
                        converted = true;
                    } else if (m.has_flag("FLAT", i, j)) {
                        if (m.has_flag("INDOORS", i, j)) {
                            if (one_in(5)) {
                                m.ter_set(i, j, t_fungus_floor_in);
                                converted = true;
                            }
                        } else if (m.has_flag("SUPPORTS_ROOF", i, j)) {
                            if (one_in(10)) {
                                m.ter_set(i, j, t_fungus_floor_sup);
                                converted = true;
                            }
                        } else {
                            if (one_in(25)) {
                                m.ter_set(i, j, t_fungus_floor_out);
                                converted = true;
                            }
                        }
                    } else if (m.has_flag("SHRUB", i, j)) {
                        if (one_in(2)) {
                            m.ter_set(i, j, t_shrub_fungal);
                            converted = true;
                        } else if (one_in(25)) {
                            m.ter_set(i, j, t_marloss);
                            converted = true;
                        }
                    } else if (m.has_flag("THIN_OBSTACLE", i, j)) {
                        if (x_in_y(10, 15)) {
                            m.ter_set(i, j, t_fungus_mound);
                            converted = true;
                        }
                    } else if (m.has_flag("YOUNG", i, j)) {
                        if (one_in(5)) {
                            m.ter_set(i, j, t_tree_fungal_young);
                            converted = true;
                        }
                    } else if (m.has_flag("TREE", i, j)) {
                        if (one_in(10)) {
                            m.ter_set(i, j, t_tree_fungal);
                            converted = true;
                        }
                    } else if (m.has_flag("WALL", i, j)) {
                        if (one_in(50)) {
                            converted = true;
                            if (m.ter_at(i, j).sym == LINE_OXOX) {
                                m.ter_set(i, j, t_fungus_wall_h);
                            } else if (m.ter_at(i, j).sym == LINE_XOXO) {
                                m.ter_set(i, j, t_fungus_wall_v);
                            } else {
                                m.ter_set(i, j, t_fungus_wall);
                            }
                        }
                    }

                    if (converted) {
                        if (m.has_flag("FLOWER", i, j)) {
                            m.furn_set(i, j, f_flower_fungal);
                        } else if (m.has_flag("ORGANIC", i, j)) {
                            if (m.furn_at(i, j).movecost == -10) {
                                m.furn_set(i, j, f_fungal_mass);
                            } else {
                                m.furn_set(i, j, f_fungal_clump);
                            }
                        } else if (m.has_flag("PLANT", i, j)) {
                            for (int k = 0; k < g->m.i_at(i, j).size(); k++) {
                                m.i_rem(i, j, k);
                            }
                            item seeds(itypes["fungal_seeds"], int(g->turn));
                            m.add_item_or_charges(x, y, seeds);
                        }
                    }
                }
            }
        }
        return false;
    }
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
        if (maxy < minHeight && maxx < minWidth) {
            fold_and_print(tmp, 0, 0, maxx, c_white, _("\
Whoa! Your terminal is tiny! This game requires a minimum terminal size of \
%dx%d to work properly. %dx%d just won't do. Maybe a smaller font would help?"),
                           minWidth, minHeight, maxx, maxy);
        } else if (maxx < minWidth) {
            fold_and_print(tmp, 0, 0, maxx, c_white, _("\
Oh! Hey, look at that. Your terminal is just a little too narrow. This game \
requires a minimum terminal size of %dx%d to function. It just won't work \
with only %dx%d. Can you stretch it out sideways a bit?"),
                           minWidth, minHeight, maxx, maxy);
        } else {
            fold_and_print(tmp, 0, 0, maxx, c_white, _("\
Woah, woah, we're just a little short on space here. The game requires a \
minimum terminal size of %dx%d to run. %dx%d isn't quite enough! Can you \
make the terminal just a smidgen taller?"),
                           minWidth, minHeight, maxx, maxy);
        }
        wgetch(tmp);
        getmaxyx(stdscr, maxy, maxx);
 }
 werase(tmp);
 mvwprintz(tmp, 0, 0, c_ltblue, ":)");
 wrefresh(tmp);
 delwin(tmp);
 erase();
}

void game::process_artifact(item *it, player *p, bool wielded)
{
    std::vector<art_effect_passive> effects;
    if (it->is_armor()) {
        it_artifact_armor* armor = dynamic_cast<it_artifact_armor*>(it->type);
        effects = armor->effects_worn;
    } else if (it->is_tool()) {
        it_artifact_tool* tool = dynamic_cast<it_artifact_tool*>(it->type);
        effects = tool->effects_carried;
        if (wielded) {
            for (int i = 0; i < tool->effects_wielded.size(); i++) {
                effects.push_back(tool->effects_wielded[i]);
            }
        }
        // Recharge it if necessary
        if (it->charges < tool->max_charges) {
            switch (tool->charge_type) {
            case ARTC_TIME:
                // Once per hour
                if (turn.seconds() == 0 && turn.minutes() == 0) {
                    it->charges++;
                }
                break;
            case ARTC_SOLAR:
                if (turn.seconds() == 0 && turn.minutes() % 10 == 0 &&
                    is_in_sunlight(p->posx, p->posy)) {
                    it->charges++;
                }
                break;
            case ARTC_PAIN:
                if (turn.seconds() == 0) {
                    add_msg(_("You suddenly feel sharp pain for no reason."));
                    p->pain += 3 * rng(1, 3);
                    it->charges++;
                }
                break;
            case ARTC_HP:
                if (turn.seconds() == 0) {
                    add_msg(_("You feel your body decaying."));
                    p->hurtall(1);
                    it->charges++;
                }
                break;
            }
        }
    }

    for (int i = 0; i < effects.size(); i++) {
        switch (effects[i]) {
        case AEP_STR_UP:
            p->str_cur += 4;
            break;
        case AEP_DEX_UP:
            p->dex_cur += 4;
            break;
        case AEP_PER_UP:
            p->per_cur += 4;
            break;
        case AEP_INT_UP:
            p->int_cur += 4;
            break;
        case AEP_ALL_UP:
            p->str_cur += 2;
            p->dex_cur += 2;
            p->per_cur += 2;
            p->int_cur += 2;
            break;
        case AEP_SPEED_UP: // Handled in player::current_speed()
            break;

        case AEP_IODINE:
            if (p->radiation > 0) {
                p->radiation--;
            }
            break;

        case AEP_SMOKE:
            if (one_in(10)) {
                int x = p->posx + rng(-1, 1), y = p->posy + rng(-1, 1);
                if (m.add_field(this, x, y, fd_smoke, rng(1, 3))) {
                    add_msg(_("The %s emits some smoke."),
                            it->tname().c_str());
                }
            }
            break;

        case AEP_SNAKES:
            break; // Handled in player::hit()

        case AEP_EXTINGUISH:
            for (int x = p->posx - 1; x <= p->posx + 1; x++) {
                for (int y = p->posy - 1; y <= p->posy + 1; y++) {
                    m.adjust_field_age(point(x,y), fd_fire, -1);
                }
            }

        case AEP_HUNGER:
            if (one_in(100)) {
                p->hunger++;
            }
            break;

        case AEP_THIRST:
            if (one_in(120)) {
                p->thirst++;
            }
            break;

        case AEP_EVIL:
            if (one_in(150)) { // Once every 15 minutes, on average
                p->add_disease("evil", 300);
                if (it->is_armor()) {
                    add_msg(_("You have an urge to wear the %s."),
                            it->tname().c_str());
                } else if (!wielded) {
                    add_msg(_("You have an urge to wield the %s."),
                            it->tname().c_str());
                }
            }
            break;

        case AEP_SCHIZO:
            break; // Handled in player::suffer()

        case AEP_RADIOACTIVE:
            if (one_in(4)) {
                p->radiation++;
            }
            break;

        case AEP_STR_DOWN:
            p->str_cur -= 3;
            break;

        case AEP_DEX_DOWN:
            p->dex_cur -= 3;
            break;

        case AEP_PER_DOWN:
            p->per_cur -= 3;
            break;

        case AEP_INT_DOWN:
            p->int_cur -= 3;
            break;

        case AEP_ALL_DOWN:
            p->str_cur -= 2;
            p->dex_cur -= 2;
            p->per_cur -= 2;
            p->int_cur -= 2;
            break;

        case AEP_SPEED_DOWN:
            break; // Handled in player::current_speed()
        }
    }
}

void game::add_artifact_messages(std::vector<art_effect_passive> effects)
{
    int net_str = 0, net_dex = 0, net_per = 0, net_int = 0, net_speed = 0;

    for (int i = 0; i < effects.size(); i++) {
        switch (effects[i]) {
        case AEP_STR_UP:   net_str += 4; break;
        case AEP_DEX_UP:   net_dex += 4; break;
        case AEP_PER_UP:   net_per += 4; break;
        case AEP_INT_UP:   net_int += 4; break;
        case AEP_ALL_UP:   net_str += 2;
                           net_dex += 2;
                           net_per += 2;
                           net_int += 2; break;
        case AEP_STR_DOWN: net_str -= 3; break;
        case AEP_DEX_DOWN: net_dex -= 3; break;
        case AEP_PER_DOWN: net_per -= 3; break;
        case AEP_INT_DOWN: net_int -= 3; break;
        case AEP_ALL_DOWN: net_str -= 2;
                           net_dex -= 2;
                           net_per -= 2;
                           net_int -= 2; break;

        case AEP_SPEED_UP:   net_speed += 20; break;
        case AEP_SPEED_DOWN: net_speed -= 20; break;

        case AEP_IODINE:
            break; // No message

        case AEP_SNAKES:
            add_msg(_("Your skin feels slithery."));
            break;

        case AEP_INVISIBLE:
            add_msg(_("You fade into invisibility!"));
            break;

        case AEP_CLAIRVOYANCE:
            add_msg(_("You can see through walls!"));
            break;

        case AEP_SUPER_CLAIRVOYANCE:
            add_msg(_("You can see through everything!"));
            break;

        case AEP_STEALTH:
            add_msg(_("Your steps stop making noise."));
            break;

        case AEP_GLOW:
            add_msg(_("A glow of light forms around you."));
            break;

        case AEP_PSYSHIELD:
            add_msg(_("Your mental state feels protected."));
            break;

        case AEP_RESIST_ELECTRICITY:
            add_msg(_("You feel insulated."));
            break;

        case AEP_CARRY_MORE:
            add_msg(_("Your back feels strengthened."));
            break;

        case AEP_HUNGER:
            add_msg(_("You feel hungry."));
            break;

        case AEP_THIRST:
            add_msg(_("You feel thirsty."));
            break;

        case AEP_EVIL:
            add_msg(_("You feel an evil presence..."));
            break;

        case AEP_SCHIZO:
            add_msg(_("You feel a tickle of insanity."));
            break;

        case AEP_RADIOACTIVE:
            add_msg(_("Your skin prickles with radiation."));
            break;

        case AEP_MUTAGENIC:
            add_msg(_("You feel your genetic makeup degrading."));
            break;

        case AEP_ATTENTION:
            add_msg(_("You feel an otherworldly attention upon you..."));
            break;

        case AEP_FORCE_TELEPORT:
            add_msg(_("You feel a force pulling you inwards."));
            break;

        case AEP_MOVEMENT_NOISE:
            add_msg(_("You hear a rattling noise coming from inside yourself."));
            break;

        case AEP_BAD_WEATHER:
            add_msg(_("You feel storms coming."));
            break;
        }
    }

    std::string stat_info = "";
    if (net_str != 0) {
        stat_info += string_format(_("Str %s%d! "),
                                   (net_str > 0 ? "+" : ""), net_str);
    }
    if (net_dex != 0) {
        stat_info += string_format(_("Dex %s%d! "),
                                   (net_dex > 0 ? "+" : ""), net_dex);
    }
    if (net_int != 0) {
        stat_info += string_format(_("Int %s%d! "),
                                   (net_int > 0 ? "+" : ""), net_int);
    }
    if (net_per != 0) {
        stat_info += string_format(_("Per %s%d! "),
                                   (net_per > 0 ? "+" : ""), net_per);
    }

    if (stat_info.length() > 0) {
        add_msg(stat_info.c_str());
    }

    if (net_speed != 0) {
        add_msg(_("Speed %s%d! "), (net_speed > 0 ? "+" : ""), net_speed);
    }
}
