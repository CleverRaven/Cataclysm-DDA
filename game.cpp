#include "game.h"
#include "rng.h"
#include "keypress.h"
#include "output.h"
#include "skill.h"
#include "line.h"
#include "computer.h"
#include "weather_data.h"
#include <fstream>
#include <sstream>
#include <math.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

void intro();
nc_color sev(int a);	// Right now, ONLY used for scent debugging....
moncat_id mt_to_mc(mon_id type);	// Pick the moncat that contains type

#if (defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__
/* Windows lacks the nanosleep() function. The following code was stuffed
   together from GNUlib (http://www.gnu.org/software/gnulib/), which is
   licensed under the GPLv3. */
int
nanosleep (const struct timespec *requested_delay,
           struct timespec *remaining_delay)
{
  static bool initialized;
  /* Number of performance counter increments per nanosecond,
     or zero if it could not be determined.  */
  static double ticks_per_nanosecond;

  if (requested_delay->tv_nsec < 0 || BILLION <= requested_delay->tv_nsec)
    {
      errno = EINVAL;
      return -1;
    }

  /* For requested delays of one second or more, 15ms resolution is
     sufficient.  */
  if (requested_delay->tv_sec == 0)
    {
      if (!initialized)
        {
          /* Initialize ticks_per_nanosecond.  */
          LARGE_INTEGER ticks_per_second;

          if (QueryPerformanceFrequency (&ticks_per_second))
            ticks_per_nanosecond =
              (double) ticks_per_second.QuadPart / 1000000000.0;

          initialized = true;
        }
      if (ticks_per_nanosecond)
        {
          /* QueryPerformanceFrequency worked.  We can use
             QueryPerformanceCounter.  Use a combination of Sleep and
             busy-looping.  */
          /* Number of milliseconds to pass to the Sleep function.
             Since Sleep can take up to 8 ms less or 8 ms more than requested
             (or maybe more if the system is loaded), we subtract 10 ms.  */
          int sleep_millis = (int) requested_delay->tv_nsec / 1000000 - 10;
          /* Determine how many ticks to delay.  */
          LONGLONG wait_ticks = requested_delay->tv_nsec * ticks_per_nanosecond;
          /* Start.  */
          LARGE_INTEGER counter_before;
          if (QueryPerformanceCounter (&counter_before))
            {
              /* Wait until the performance counter has reached this value.
                 We don't need to worry about overflow, because the performance
                 counter is reset at reboot, and with a frequency of 3.6E6
                 ticks per second 63 bits suffice for over 80000 years.  */
              LONGLONG wait_until = counter_before.QuadPart + wait_ticks;
              /* Use Sleep for the longest part.  */
              if (sleep_millis > 0)
                Sleep (sleep_millis);
              /* Busy-loop for the rest.  */
              for (;;)
                {
                  LARGE_INTEGER counter_after;
                  if (!QueryPerformanceCounter (&counter_after))
                    /* QueryPerformanceCounter failed, but succeeded earlier.
                       Should not happen.  */
                    break;
                  if (counter_after.QuadPart >= wait_until)
                    /* The requested time has elapsed.  */
                    break;
                }
              goto done;
            }
        }
    }
  /* Implementation for long delays and as fallback.  */
  Sleep (requested_delay->tv_sec * 1000 + requested_delay->tv_nsec / 1000000);

 done:
  /* Sleep is not interruptible.  So there is no remaining delay.  */
  if (remaining_delay != NULL)
    {
      remaining_delay->tv_sec = 0;
      remaining_delay->tv_nsec = 0;
    }
  return 0;
}

#endif

// This is the main game set-up process.
game::game()
{
 clear();	// Clear the screen
 intro();	// Print an intro screen, make sure we're at least 80x25
// Gee, it sure is init-y around here!
 init_itypes();	  // Set up item types                   (SEE itypedef.cpp)
 init_mtypes();	  // Set up monster types                (SEE mtypedef.cpp)
 init_monitems(); // Set up which items monsters carry   (SEE monitemsdef.cpp)
 init_traps();	  // Set up the trap types               (SEE trapdef.cpp)
 init_mapitems(); // Set up which items appear where     (SEE mapitemsdef.cpp)
 init_recipes();  // Set up crafting reciptes            (SEE crafting.cpp)
 init_moncats();  // Set up monster categories           (SEE mongroupdef.cpp)
 init_missions(); // Set up mission templates            (SEE missiondef.cpp)

 m = map(&itypes, &mapitems, &traps); // Init the root map with our vectors

// Set up the main UI windows.
// Aw hell, we getting ncursey up in here!
 w_terrain = newwin(SEEY * 2 + 1, SEEX * 2 + 1, 0, 0);
 werase(w_terrain);
 w_minimap = newwin(7, 7, 0, SEEX * 2 + 1);
 werase(w_minimap);
 w_HP = newwin(14, 7, 7, SEEX * 2 + 1);
 werase(w_HP);
 w_moninfo = newwin(12, 48, 0, SEEX * 2 + 8);
 werase(w_moninfo);
 w_messages = newwin(9, 48, 12, SEEX * 2 + 8);
 werase(w_messages);
 w_status = newwin(4, 55, 21, SEEX * 2 + 1);
 werase(w_status);
// Even though we may already have 'd', nextinv will be incremented as needed
 nextinv = 'd';
 next_npc_id = 1;
 next_mission_id = 1;
 last_target = -1;	// We haven't targeted any monsters yet
 curmes = 0;		// We haven't read any messages yet
 uquit = QUIT_NO;	// We haven't quit the game
 debugmon = false;	// We're not printing debug messages
 in_tutorial = false;	// We're not in a tutorial game
 weather = WEATHER_CLEAR;
 season = SPRING;
 for (int i = 0; i < num_monsters; i++)	// Reset kill counts to 0
  kills[i] = 0;
// Set the scent map to 0
 for (int i = 0; i < SEEX * 3; i++) {
  for (int j = 0; j < SEEX * 3; j++)
   grscent[i][j] = 0;
 }
 if (opening_screen()) {// Opening menu
// Finally, draw the screen!
  refresh_all();
  draw();
 }
}

game::~game()
{
 for (int i = 0; i < itypes.size(); i++)
  delete itypes[i];
 for (int i = 0; i < mtypes.size(); i++)
  delete mtypes[i];
 delwin(w_terrain);
 delwin(w_minimap);
 delwin(w_HP);
 delwin(w_moninfo);
 delwin(w_messages);
 delwin(w_status);
}

bool game::opening_screen()
{
 WINDOW* w_open = newwin(25, 80, 0, 0);
 erase();
 for (int i = 0; i < 80; i++)
  mvwputch(w_open, 21, i, c_white, LINE_OXOX);
 mvwprintz(w_open, 0, 1, c_blue, "Welcome to Cataclysm!");
 mvwprintz(w_open, 1, 0, c_red, "\
This alpha release is highly unstable. Please report any crashes or bugs to\n\
fivedozenwhales@gmail.com.");
 refresh();
 wrefresh(w_open);
 refresh();
 std::vector<std::string> savegames;
 std::string tmp;
 dirent *dp;
 DIR *dir = opendir("save");
 if (!dir) {
#if (defined _WIN32 || defined __WIN32__)
  mkdir("save");
#else
  mkdir("save", 0777);
#endif
  dir = opendir("save");
 }
 if (!dir) {
  debugmsg("Could not make './save' directory");
  endwin();
  exit(1);
 }
 while (dp = readdir(dir)) {
  tmp = dp->d_name;
  if (tmp.find(".sav") != std::string::npos && savegames.size() < 18)
   savegames.push_back(tmp.substr(0, tmp.find(".sav")));
 }
 int sel1 = 0, sel2 = 1, layer = 1;
 char ch;
 bool start = false;

// Load MOTD and store it in a string
 std::vector<std::string> motd;
 std::ifstream motd_file;
 motd_file.open("data/motd");
 if (!motd_file.is_open())
  motd.push_back("No message today.");
 else {
  while (!motd_file.eof()) {
   std::string tmp;
   getline(motd_file, tmp);
   if (tmp[0] != '#')
    motd.push_back(tmp);
  }
 }

 do {
  if (layer == 1) {
   mvwprintz(w_open, 4, 1, (sel1 == 0 ? h_white : c_white), "MOTD");
   mvwprintz(w_open, 5, 1, (sel1 == 1 ? h_white : c_white), "New Game");
   mvwprintz(w_open, 6, 1, (sel1 == 2 ? h_white : c_white), "Load Game");
   mvwprintz(w_open, 7, 1, (sel1 == 3 ? h_white : c_white), "Tutorial");
   mvwprintz(w_open, 8, 1, (sel1 == 4 ? h_white : c_white), "Help");
   mvwprintz(w_open, 9, 1, (sel1 == 5 ? h_white : c_white), "Quit");

   if (sel1 == 0) {	// Print the MOTD.
    for (int i = 0; i < motd.size() && i < 16; i++)
     mvwprintz(w_open, i + 4, 12, c_ltred, motd[i].c_str());
   } else {	// Clear the lines if not viewing MOTD.
    for (int i = 4; i < 20; i++) {
     for (int j = 12; j < 79; j++)
      mvwputch(w_open, i, j, c_black, 'x');
    }
   }

   wrefresh(w_open);
   refresh();
   ch = input();
   if (ch == 'k' && sel1 > 0)
    sel1--;
   if (ch == 'j' && sel1 < 5)
    sel1++;
   if ((ch == 'l' || ch == '\n' || ch == '>') && sel1 > 0) {
    if (sel1 == 5) {
     uquit = QUIT_MENU;
     return false;
    } else if (sel1 == 4) {
     help();
     clear();
     mvwprintz(w_open, 0, 1, c_blue, "Welcome to Cataclysm!");
     mvwprintz(w_open, 1, 0, c_red, "\
This alpha release is highly unstable. Please report any crashes or bugs to\n\
fivedozenwhales@gmail.com.");
     refresh();
     wrefresh(w_open);
     refresh();
    } else if (sel1 == 3) {
     u.normalize(this);
     start_tutorial(TUT_BASIC);
     return true;
    } else {
     sel2 = 1;
     layer = 2;
    }
    mvwprintz(w_open, 4, 1, (sel1 == 0 ? c_white : c_dkgray), "MOTD");
    mvwprintz(w_open, 5, 1, (sel1 == 1 ? c_white : c_dkgray), "New Game");
    mvwprintz(w_open, 6, 1, (sel1 == 2 ? c_white : c_dkgray), "Load Game");
    mvwprintz(w_open, 7, 1, (sel1 == 3 ? c_white : c_dkgray), "Tutorial");
    mvwprintz(w_open, 8, 1, (sel1 == 3 ? c_white : c_dkgray), "Help");
    mvwprintz(w_open, 9, 1, (sel1 == 4 ? c_white : c_dkgray), "Quit");
   }
  } else if (layer == 2) {
   if (sel1 == 1) {	// New Character
    mvwprintz(w_open, 5, 12, (sel2 == 1 ? h_white : c_white),
              "Custom Character");
    mvwprintz(w_open, 6, 12, (sel2 == 2 ? h_white : c_white),
              "Preset Character");
    mvwprintz(w_open, 7, 12, (sel2 == 3 ? h_white : c_white),
              "Random Character");
    wrefresh(w_open);
    refresh();
    ch = input();
    if (ch == 'k' && sel2 > 1)
     sel2--;
    if (ch == 'j' && sel2 < 3)
     sel2++;
    if (ch == 'h' || ch == '<' || ch == KEY_ESCAPE) {
     mvwprintz(w_open, 5, 12, c_black, "                ");
     mvwprintz(w_open, 6, 12, c_black, "                ");
     mvwprintz(w_open, 7, 12, c_black, "                ");
     layer = 1;
     sel1 = 1;
    }
    if (ch == 'l' || ch == '\n' || ch == '>') {
     if (sel2 == 1) {
      if (!u.create(this, PLTYPE_CUSTOM)) {
       u = player();
       delwin(w_open);
       return (opening_screen());
      }
      start_game();
      start = true;
      ch = 0;
     }
     if (sel2 == 2) {
      layer = 3;
      sel1 = 2;
      mvwprintz(w_open, 5, 12, c_dkgray, "Custom Character");
      mvwprintz(w_open, 6, 12, c_white,  "Preset Character");
      mvwprintz(w_open, 7, 12, c_dkgray, "Random Character");
     }
     if (sel2 == 3) {
      if (!u.create(this, PLTYPE_RANDOM)) {
       u = player();
       delwin(w_open);
       return (opening_screen());
      }
      start_game();
      start = true;
      ch = 0;
     }
    }
   } else if (sel1 == 2) {	// Load Character
    if (savegames.size() == 0)
     mvwprintz(w_open, 6, 12, c_red, "No save games found!");
    else {
     int savestart = (sel2 < 7 ?  0 : sel2 - 7),
         saveend   = (sel2 < 7 ? 14 : sel2 + 7);
/*
     if (saveend > savegames.size())
      saveend = savegames.size();
*/
     for (int i = savestart; i < saveend; i++) {
      int line = 6 + i - savestart;
      mvwprintz(w_open, line, 12, c_black, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
      if (i < savegames.size())
       mvwprintz(w_open, line, 12, (sel2 - 1 == i ? h_white : c_white),
                 savegames[i].c_str());
     }
    }
    wrefresh(w_open);
    refresh();
    ch = input();
    if (ch == 'k' && sel2 > 1)
     sel2--;
    if (ch == 'j' && sel2 < savegames.size())
     sel2++;
    if (ch == 'h' || ch == '<' || ch == KEY_ESCAPE) {
     layer = 1;
     for (int i = 0; i < 14; i++)
      mvwprintz(w_open, 6 + i, 12, c_black, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
    }
    if (ch == 'l' || ch == '\n' || ch == '>') {
     if (sel2 > 0 && savegames.size() > 0) {
      load(savegames[sel2 - 1]);
      start = true;
      ch = 0;
     }
    }
   }
  } else if (layer == 3) {	// Character presets
   for (int i = 2; i < PLTYPE_MAX; i++)
    mvwprintz(w_open, 4 + i, 29, (sel1 == i ? h_white : c_white),
              pltype_name[i].c_str());
   for (int i = 22; i < 25; i++)
    mvwprintw(w_open, i, 0, "                                                  \
                              ");
   mvwprintz(w_open, 22, 0, c_magenta, pltype_desc[sel1].c_str());
   wrefresh(w_open);
   refresh();
   ch = input();
   if (ch == 'k' && sel1 > 2)
    sel1--;
   if (ch == 'j' && sel1 < PLTYPE_MAX - 1)
    sel1++;
   if (ch == 'h' || ch == '<' || ch == KEY_ESCAPE) {
    sel1 = 1;
    layer = 2;
    for (int i = 2; i < PLTYPE_MAX; i++)
     mvwprintz(w_open, 3 + i, 12, c_black, "                                 ");
    for (int i = 22; i < 25; i++)
     mvwprintw(w_open, i, 0, "                                                 \
                                ");
   }
   if (ch == 'l' || ch == '\n' || ch == '>') {
    if (!u.create(this, character_type(sel1))) {
     u = player();
     delwin(w_open);
     return (opening_screen());
    }
    start_game();
    start = true;
    ch = 0;
   }
  }
 } while (ch != 0);
 delwin(w_open);
 if (start == false)
  uquit = QUIT_MENU;
 return start;
}

// Set up all default values for a new game
void game::start_game()
{
 turn = 0;	// It's turn 0...
 run_mode = 1;	// run_mode is on by default...
 mostseen = 0;	// ...and mostseen is 0, we haven't seen any monsters yet.

// Init some factions.
 if (!load_master())	// Master data record contains factions.
  create_factions();
 cur_om = overmap(this, 0, 0, 0);	// We start in the (0,0,0) overmap.
// Find a random house on the map, and set us there.
 cur_om.first_house(levx, levy);
 levz = 0;
// Start the overmap out with none of it seen by the player...
 for (int i = 0; i < OMAPX; i++) {
  for (int j = 0; j < OMAPX; j++)
   cur_om.seen(i, j) = false;
 }
// ...except for our immediate neighborhood.
 for (int i = -20; i <= 20; i++) {
  for (int j = -20; j <= 20; j++)
   cur_om.seen(levx + i, levy + j) = true;
 }
// Convert the overmap coordinates to submap coordinates
 levx = levx * 2 - 1;
 levy = levy * 2 - 1;
// Init the starting map at this location.
 m.load(this, levx, levy);
// Start us off somewhere in the house.
 u.posx = SEEX + 4;
 u.posy = SEEY + 5;
 u.str_cur = u.str_max;
 u.per_cur = u.per_max;
 u.int_cur = u.int_max;
 u.dex_cur = u.dex_max;
 nextspawn = 300;	// No monsters until 8:30 AM!
 temperature = 65;	// Kind of cool for June, but okay.

// Testing pet dog!
 monster doggy(mtypes[mon_dog], u.posx - 1, u.posy - 1);
 doggy.friendly = -1;
 z.push_back(doggy);

// Testing missions!
 //give_mission(MISSION_REACH_SAFETY);
}

void game::start_tutorial(tut_type type)
{
 for (int i = 0; i < NUM_LESSONS; i++)
  tutorials_seen[i] = false;
// Set the scent map to 0
 for (int i = 0; i < SEEX * 3; i++) {
  for (int j = 0; j < SEEX * 3; j++)
   grscent[i][j] = 0;
 }
 temperature = 65;		// Kind of cool for June, but okay.
 in_tutorial = true;
 switch (type) {
 case TUT_NULL:
  debugmsg("Null tutorial requested.");
  return;
 case TUT_BASIC:
// We use a Z-factor of 10 so that we don't plop down tutorial rooms in the
// middle of the "real" game world
  u.name = "John Smith";
  levx = 100;
  levy =  99;
  cur_om = overmap(this, 0, 0, TUTORIAL_Z - 1);
  cur_om.make_tutorial();
  cur_om.save(u.name, 0, 0, 9);
  cur_om = overmap(this, 0, 0, TUTORIAL_Z);
  cur_om.make_tutorial();
  u.toggle_trait(PF_QUICK);
  u.inv.push_back(item(itypes[itm_lighter], 0, 'e'));
  u.sklevel[sk_gun] = 5;
  u.sklevel[sk_melee] = 5;
// Start the overmap out with all of it seen by the player
  for (int i = 0; i < OMAPX; i++) {
   for (int j = 0; j < OMAPX; j++)
    cur_om.seen(i, j) = true;
  }
// Init the starting map at this location.
  m.load(this, levx, levy);
// Make sure the map is totally reset
  for (int i = 0; i < SEEX * 3; i++) {
   for (int j = 0; j < SEEY * 3; j++)
    m.i_at(i, j).clear();
  }
// Special items for the tutorial.
  m.add_item(           5, SEEY * 2 + 1, itypes[itm_helmet_bike], 0);
  m.add_item(           4, SEEY * 2 + 1, itypes[itm_backpack], 0);
  m.add_item(           3, SEEY * 2 + 1, itypes[itm_pants_cargo], 0);
  m.add_item(           7, SEEY * 3 - 4, itypes[itm_machete], 0);
  m.add_item(           7, SEEY * 3 - 4, itypes[itm_9mm], 0);
  m.add_item(           7, SEEY * 3 - 4, itypes[itm_9mmP], 0);
  m.add_item(           7, SEEY * 3 - 4, itypes[itm_uzi], 0);
  m.add_item(SEEX * 2 - 2, SEEY * 2 + 5, itypes[itm_bubblewrap], 0);
  m.add_item(SEEX * 2 - 2, SEEY * 2 + 6, itypes[itm_grenade], 0);
  m.add_item(SEEX * 2 - 3, SEEY * 2 + 6, itypes[itm_flashlight], 0);
  m.add_item(SEEX * 2 - 2, SEEY * 2 + 7, itypes[itm_cig], 0);
  m.add_item(SEEX * 2 - 2, SEEY * 2 + 7, itypes[itm_codeine], 0);
  m.add_item(SEEX * 2 - 3, SEEY * 2 + 7, itypes[itm_water], 0);

  levz = 0;
  u.posx = SEEX + 2;
  u.posy = SEEY + 4;
  break;
 default:
  debugmsg("Haven't made that tutorial yet.");
  return;
 }
}

void game::create_factions()
{
 int num = dice(4, 3);
 faction tmp;
 tmp.make_army();
 factions.push_back(tmp);
 for (int i = 0; i < num; i++) {
  tmp.randomize();
  tmp.id = i + 1;
  tmp.likes_u = 100;
  tmp.respects_u = 100;
  tmp.known_by_u = true;
  factions.push_back(tmp);
 }
}

// MAIN GAME LOOP
// Returns true if game is over (death, saved, quit, etc)
bool game::do_turn()
{
 if (is_game_over()) {
  if (uquit == QUIT_DIED)
   popup_top("Game over!");
  if (uquit == QUIT_DIED || uquit == QUIT_SUICIDE)
   death_screen();
  return true;
 }
 turn++;
 process_events();
 if (in_tutorial) {
  if (turn == 1) {
   tutorial_message(LESSON_INTRO);	// Goes through a list of intro topics
   tutorial_message(LESSON_INTRO);
  } else if (turn == 3)
   tutorial_message(LESSON_INTRO);
  if (turn == 50) {
   monster tmp(mtypes[mon_zombie], 3, 3);
   z.push_back(tmp);
  }
 }
// Check if we've overdosed... in any deadly way.
 if (u.stim > 250) {
  add_msg("You have a sudden heart attack!");
  u.hp_cur[hp_torso] = 0;
 } else if (u.stim < -200 || u.pkill > 240) {
  add_msg("Your breathing stops completely.");
  u.hp_cur[hp_torso] = 0;
 }

 if (turn % 50 == 0) {	// Hunger, thirst, & fatigue up every 5 minutes
  if ((!u.has_trait(PF_LIGHTEATER) || !one_in(3)) &&
      (!u.has_bionic(bio_recycler) || turn % 300 == 0))
   u.hunger++;
  if (!u.has_bionic(bio_recycler) || turn % 100 == 0)
   u.thirst++;
  u.fatigue++;
  if (u.fatigue == 192 && !u.has_disease(DI_LYING_DOWN) &&
      !u.has_disease(DI_SLEEP)) {
   add_msg("You're feeling tired.  Press '$' to lie down for sleep.");
   u.activity.type = ACT_NULL;
  }
  if (u.stim < 0)
   u.stim++;
  if (u.stim > 0)
   u.stim--;
  if (u.pkill > 0)
   u.pkill--;
  if (u.pkill < 0)
   u.pkill++;
  if (u.has_bionic(bio_solar) && is_in_sunlight(u.posx, u.posy))
   u.charge_power(1);
 }
 if (turn % 300 == 0) {	// Pain up/down every 30 minutes
  if (u.pain > 0)
   u.pain--;
  else if (u.pain < 0)
   u.pain++;
  if (u.has_trait(PF_REGEN) && !one_in(3))
   u.healall(1);
  if (u.has_trait(PF_ROT) && !one_in(3))
   u.hurtall(1);
  if (u.radiation > 1 && one_in(3))
   u.radiation--;
  u.get_sick(this);
// On the half-hour, we also save and update the weather.
  update_weather();
  save();
 }

// The following happens when we stay still; 10/40 minutes overdue for spawn
 if ((!u.has_trait(PF_INCONSPICUOUS) && turn > nextspawn +  100) ||
     ( u.has_trait(PF_INCONSPICUOUS) && turn > nextspawn +  400)   ) {
  spawn_mon(-1 + 2 * rng(0, 1), -1 + 2 * rng(0, 1));
  nextspawn = turn;
 }
 process_activity();
 while (u.moves > 0) {
  draw();
  get_input();
  if (is_game_over()) {
   if (uquit == QUIT_DIED)
    popup_top("Game over!");
   if (uquit == QUIT_DIED || uquit == QUIT_SUICIDE)
    death_screen();
   return true;
  }
 }
 update_scent();
 m.process_fields(this);
 m.process_active_items(this);
 m.step_in_field(u.posx, u.posy, this);

 monmove();
 om_npcs_move();
 u.reset();
 u.process_active_items(this);
 u.suffer(this);

 if (levz >= 0) {
  weather_effect weffect;
  (weffect.*(weather_data[weather].effect))(this);
  check_warmth();
 }

 update_skills();
 if (turn % 10 == 0)
  u.update_morale();
 if (u.has_disease(DI_SLEEP)) {
  draw();
  refresh();
 }
 return false;
}

void game::update_skills()
{
//    SKILL   TURNS/--
//	1	2048
//	2	1024
//	3	 512
//	4	 256
//	5	 128
//	6	  64
//	7+	  32
 int tmp;
 for (int i = 0; i < num_skill_types; i++) {
  tmp = u.sklevel[i] > 7 ? 7 : u.sklevel[i];
  if (u.sklevel[i] > 0 && turn % (4096 / int(pow(2, tmp - 1))) == 0 &&
      (( u.has_trait(PF_FORGETFUL) && one_in(3)) ||
       (!u.has_trait(PF_FORGETFUL) && one_in(4))  )) {
   if (u.has_bionic(bio_memory) && u.power_level > 0) {
    if (one_in(5))
     u.power_level--;
   } else
    u.skexercise[i]--;
  }
  if (u.skexercise[i] < -100) {
   u.sklevel[i]--;
   add_msg("Your skill in %s has reduced to %d!",
           skill_name(skill(i)).c_str(), u.sklevel[i]);
   u.skexercise[i] = 0;
  } else if (u.skexercise[i] >= 100) {
   u.sklevel[i]++;
   add_msg("Your skill in %s has increased to %d!",
           skill_name(skill(i)).c_str() ,u.sklevel[i]);
   u.skexercise[i] = 0;
  }
 }
}

void game::process_events()
{
 for (int i = 0; i < events.size(); i++) {
  if (events[i].turn <= turn) {
   events[i].actualize(this);
   events.erase(events.begin() + i);
   i--;
  }
 }
}

void game::process_activity()
{
 it_gun* reloading;
 it_book* reading;
 if (u.activity.type != ACT_NULL) {
  draw();
  if (u.activity.type == ACT_WAIT) {	// Based on time, not speed
   u.activity.moves_left -= 100;
   u.pause();
  } else {
   u.activity.moves_left -= u.moves;
   u.moves = 0;
  }

  if (u.activity.moves_left <= 0) {	// We finished our activity!

   switch (u.activity.type) {

   case ACT_RELOAD:
    u.weapon.reload(u, u.activity.index);
    if (u.weapon.is_gun())
     reloading = dynamic_cast<it_gun*>(u.weapon.type);
    if (u.weapon.is_gun() && reloading->ammo != AT_BB &&
        reloading->ammo != AT_NAIL && one_in(u.sklevel[reloading->skill_used]))
     u.practice(reloading->skill_used, rng(2, 6));
    if (u.weapon.is_gun() && reloading->skill_used == sk_shotgun) {
     add_msg("You insert a cartridge into your %s.",
             u.weapon.tname(this).c_str());
     if (u.recoil < 8)
      u.recoil = 8;
     if (u.recoil > 8)
      u.recoil = (8 + u.recoil) / 2;
    } else {
     add_msg("You reload your %s.", u.weapon.tname(this).c_str());
     u.recoil = 6;
     if (in_tutorial) {
      tutorial_message(LESSON_GUN_FIRE);
      monster tmp(mtypes[mon_zombie], u.posx, u.posy - 6);
      z.push_back(tmp);
      tmp.spawn(u.posx + 2, u.posy - 5);
      z.push_back(tmp);
      tmp.spawn(u.posx - 2, u.posy - 5);
      z.push_back(tmp);
     }
    }
    break;

   case ACT_READ:
    if (u.activity.index == -2)
     reading = dynamic_cast<it_book*>(u.weapon.type);
    else
     reading = dynamic_cast<it_book*>(u.inv[u.activity.index].type);

    if (reading->fun != 0) {
     std::stringstream morale_text;
     u.add_morale(MORALE_BOOK, reading->fun * 5, reading->fun * 15, reading);
    }

    if (u.sklevel[reading->type] < reading->level) {
     add_msg("You learn a little about %s!", skill_name(reading->type).c_str());
     int min_ex = reading->time / 3 + u.int_cur,
         max_ex = reading->time     + u.int_cur;
     if (min_ex < 5)
      min_ex = 5;
     if (max_ex > 35)
      max_ex = 35;
     u.skexercise[reading->type] += rng(min_ex, max_ex);
     if (u.sklevel[reading->type] +
        (u.skexercise[reading->type] >= 100 ? 1 : 0) >= reading->level)
      add_msg("You can no longer learn from this %s.", reading->name.c_str());
    }
    break;

   case ACT_WAIT:
    add_msg("You finish waiting.");
    break;

   case ACT_CRAFT:
    complete_craft();
    break;

   case ACT_BUTCHER:
    complete_butcher(u.activity.index);
    break;
   }
   u.activity.type = ACT_NULL;
  }
 }
}

void game::cancel_activity()
{
 u.activity.type = ACT_NULL;
}

void game::cancel_activity_query(std::string message)
{
 switch (u.activity.type) {
  case ACT_READ:
   if (query_yn("%s Stop reading?", message.c_str()))
    u.activity.type = ACT_NULL;
   break;
  case ACT_RELOAD:
   if (query_yn("%s Stop reloading?", message.c_str()))
    u.activity.type = ACT_NULL;
   break;
  case ACT_CRAFT:
   if (query_yn("%s Stop crafting?", message.c_str()))
    u.activity.type = ACT_NULL;
   break;
  case ACT_BUTCHER:
   if  (query_yn("%s Stop butchering?", message.c_str()))
    u.activity.type = ACT_NULL;
   break;
  default:
   u.activity.type = ACT_NULL;
 }
}

void game::update_weather()
{
// Pick a new weather type (most likely the same one)
 int chances[NUM_WEATHER_TYPES];
 int total = 0;
 for (int i = 0; i < NUM_WEATHER_TYPES; i++) {
// Reduce the chance for freezing-temp-only weather to 0 if it's above freezing
// and vice versa.
  if ((weather_data[i].avg_temperature[season] < 32 && temperature > 32) ||
      (weather_data[i].avg_temperature[season] > 32 && temperature < 32)   )
   chances[i] = 0;
  else {
   chances[i] = weather_shift[season][weather][i];
   total += chances[i];
  }
 }
 int choice = rng(0, total - 1);
 weather_type new_weather = WEATHER_CLEAR;
 while (choice >= chances[new_weather]) {
  choice -= chances[new_weather];
  new_weather = weather_type(int(new_weather) + 1);
 }
 weather = new_weather;

// Now update temperature
 if (!one_in(4)) { // 3 in 4 chance of respecting avg temp for the weather
  int average = weather_data[weather].avg_temperature[season];
  if (temperature < average)
   temperature++;
  else if (temperature > average)
   temperature--;
 } else // 1 in 4 chance of random walk
  temperature += rng(-1, 1);

// TODO: Temperature shifts based on day/nighttime
}

void game::give_mission(mission_id type)
{
 mission tmp = mission_types[type].create(this);
 tmp.uid = next_mission_id;
 next_mission_id++;
 u.active_missions.push_back(tmp);
 u.active_mission = u.active_missions.size() - 1;
}

int game::reserve_mission(mission_id type)
{
 mission tmp = mission_types[type].create(this);
 tmp.uid = next_mission_id;
 active_missions.push_back(tmp);
 int ret = next_mission_id;
 next_mission_id++;
 return ret;
}

mission* game::find_mission(int id)
{
 for (int i = 0; i < active_missions.size(); i++) {
  if (active_missions[i].uid == id)
   return &(active_missions[i]);
 }
 return NULL;
}

void game::get_input()
{
 char ch = input(); // See keypress.h

 last_action = ch;

// These are the default characters for all actions.  It's the job of input(),
// found in keypress.h, to translate the user's input into these characters.
 if (ch == 'y' || ch == 'u' || ch == 'h' || ch == 'j' || ch == 'k' ||
     ch == 'l' || ch == 'b' || ch == 'n') {
  int movex, movey;
  get_direction(movex, movey, ch);
  plmove(movex, movey);
 } else if (ch == '>')
  vertical_move(-1, false);
 else if (ch == '<')
  vertical_move( 1, false);
 else if (ch == '.') {
  if (run_mode == 2) // Monsters around and we don't wanna pause
   add_msg("Monster spotted--run mode is on! (Press '!' to turn it off.)");
  else
   u.pause();
 } else if (ch == 'o')
  open();
 else if (ch == 'c')
  close();
 else if (ch == 'p') {
  u.power_bionics(this);
  refresh_all();
 } else if (ch == 'e')
  examine();
 else if (ch == ';' || ch == 'x')
  look_around();
 else if (ch == ',' || ch == 'g')
  pickup(u.posx, u.posy, 1);
 else if (ch == 'd')
  drop();
 else if (ch == 'i') {
  bool has = false;
  do {
   char ch = inv();
   has = u.has_item(ch);
   if (has)
    full_screen_popup(u.i_at(ch).info(true).c_str());
  } while (has);
  refresh_all();
 } else if (ch == 'B')
  butcher();
 else if (ch == 'E')
  eat();
 else if (ch == 'a')
  use_item();
 else if (ch == 'W')
  wear();
 else if (ch == 'w')
  wield();
 else if (ch == '^')
  wait();
 else if (ch == 'T')
  takeoff();
 else if (ch == 'r')
  reload();
 else if (ch == 'U')
  unload();
 else if (ch == 'R')
  read();
 else if (ch == 't')
  plthrow();
 else if (ch == 'f')
  plfire(false);
 else if (ch == 'F')
  plfire(true);
 else if (ch == 'C')
  chat();
// <DEBUG>
 else if (ch == 'z') {
  debugmsg("%d radio towers", cur_om.radios.size());
  for (int i = 0; i < OMAPX; i++) {
   for (int j = 0; j < OMAPY; j++)
    cur_om.seen(i, j) = true;
  }
 } else if (ch == 'N') {
  erase();
  mvprintw(0, 0, "%d addictions", u.addictions.size());
  for (int i = 0; i < u.addictions.size(); i++) {
  mvprintw(1+i, 0, "%d: int %d sate %d", int(u.addictions[i].type),
           u.addictions[i].intensity, u.addictions[i].sated);
  }
  mvprintw(20, 0, "Turn %d; nextspawn %d", turn, nextspawn);
  getch();
 } else if (ch == 'Z')
  wish();
 else if (ch == 'G') {
  npc temp;
  temp.randomize(this);
  temp.attitude = NPCATT_TALK;
  temp.spawn_at(&cur_om, levx + (1 * rng(-2, 2)), levy + (1 * rng(-2, 2)));
  temp.posx = u.posx - 4;
  temp.posy = u.posy - 4;
  active_npc.push_back(temp);
 } else if (ch == 'g')
  groupdebug();
 else if (ch == '~')
  debugmon = !debugmon;
 else if (ch == '\'') {
  display_scent();
  getch();
 } else if (ch == '*')
  teleport();
// </DEBUG>
 else if (ch == ':' || ch == 'm')
  draw_overmap();
 else if (ch == 'M')
  list_missions();
 else if (ch == '@') {
  u.disp_info(this);
  refresh_all();
 } else if (ch == '#')
  list_factions();
 else if (ch == '%') {
  u.disp_morale();
  refresh_all();
 } else if (ch == '&')
  craft();
 else if (ch == '$' && query_yn("Are you sure you want to sleep?")) {
   u.try_to_sleep(this);
   u.moves = 0;
 } else if (ch == '!') {
  if (run_mode == 0 ) {
   run_mode = 1;
   add_msg("Run mode ON!");
  } else {
   run_mode = 0;
   add_msg("Run mode OFF!");
  }
 } else if (ch == 's')
  smash();
 else if (ch == 'S' && query_yn("Save and quit?")) {
  save();
  u.moves = 0;
  uquit = QUIT_SAVED;
 } else if (ch == 'Q' && query_yn("Commit suicide?")) {
  u.moves = 0;
  std::vector<item> tmp = u.inv_dump();
  item your_body;
  your_body.make_corpse(itypes[itm_corpse], mtypes[mon_null], turn);
  your_body.name = u.name;
  m.add_item(u.posx, u.posy, your_body);
  for (int i = 0; i < tmp.size(); i++)
   m.add_item(u.posx, u.posy, tmp[i]);
  m.save(&cur_om, turn, levx, levy);
  uquit = QUIT_SUICIDE;
 } else if (ch == '?') {
  help();
  refresh_all();
 }
}

int& game::scent(int x, int y)
{
 if (x < 0 || x >= SEEX * 3 || y < 0 || y >= SEEY * 3) {
  nulscent = 0;
  return nulscent;	// Out-of-bounds - null scent
 }
 return grscent[x][y];
}

void game::update_scent()
{
 signed int newscent[SEEX * 3][SEEY * 3];
 int x, y, i, j, squares_used;
 if (!u.has_active_bionic(bio_scent_mask))
  scent(u.posx, u.posy) = u.scent;
 else
  scent(u.posx, u.posy) = 0;
 for (x = 0; x < SEEX * 3; x++) {
  for (y = 0; y < SEEY * 3; y++) {
   newscent[x][y] = 0;
   squares_used = 0;
   for (i = -1; i <= 1; i++) {
    for (j = -1; j <= 1; j++) {
     if ((m.move_cost(x, y) != 0 || m.has_flag(bashable, x, y)) &&
         scent(x, y) <= scent(x+i, y+j)) {
      newscent[x][y] += scent(x+i, y+j);
      squares_used++;
     }
    }
   }
   newscent[x][y] /= (squares_used + 1);
   if (m.field_at(x, y).type == fd_slime &&
       newscent[x][y] < 10 * m.field_at(x, y).density)
    newscent[x][y] = 10 * m.field_at(x, y).density;
   if (newscent[x][y] > 10000) {
    debugmsg("Wacky scent at %d, %d (%d)", x, y, newscent[x][y]);
    newscent[x][y] = 0; // Scent should never be higher
   }
  }
 }
 for (x = 0; x < SEEX * 3; x++) {
  for (y = 0; y < SEEY * 3; y++)
   scent(x, y) = newscent[x][y];
 }
 if (!u.has_active_bionic(bio_scent_mask))
  scent(u.posx, u.posy) = u.scent;
 else
  scent(u.posx, u.posy) = 0;
}

bool game::is_game_over()
{
 if (uquit != QUIT_NO)
  return true;
 for (int i = 0; i <= hp_torso; i++) {
  if (u.hp_cur[i] < 1) {
   std::vector<item> tmp = u.inv_dump();
   item your_body;
   your_body.make_corpse(itypes[itm_corpse], mtypes[mon_null], turn);
   your_body.name = u.name;
   m.add_item(u.posx, u.posy, your_body);
   for (int i = 0; i < tmp.size(); i++)
    m.add_item(u.posx, u.posy, tmp[i]);
   m.save(&cur_om, turn, levx, levy);
   uquit = QUIT_DIED;
   return true;
  }
 }
 return false;
}

void game::death_screen()
{
 std::stringstream playerfile;
 playerfile << "save/" << u.name << ".sav";
 int err = unlink(playerfile.str().c_str());
 int selection = 0;
 int num_kills = 0;
 for (int i = 0; i < num_monsters; i++)
  num_kills += kills[i];

 WINDOW* w_death = newwin(25, 80, 0, 0);
 mvwprintz(w_death, 0, 35, c_red, "GAME OVER - Press Spacebar to Quit");
 mvwprintz(w_death, 2, 0, c_white, "Number of kills: %d", num_kills);
 int line = 0, mon = 0;
 while (line < 40 && mon < num_monsters) {
  if (kills[mon] > 0) {
   int y = line % 20 + 3, x = int(line / 20) * 40 + 1;
   mvwprintz(w_death, y, x, c_white, "%s: %d", mtypes[mon]->name.c_str(),
             kills[mon]);
   line++;
  }
  mon++;
 }

 wrefresh(w_death);
 refresh();
 char ch;
 do
  ch = getch();
 while(ch != ' ' && ch != '\n' && ch != KEY_ESCAPE);
 delwin(w_death);
}

bool game::load_master()
{
 std::ifstream fin;
 std::string data;
 fin.open("save/master.gsav");
 if (!fin.is_open())
  return false;

 char datatype;
 while (!fin.eof()) {
  fin >> datatype;
  if (datatype == 'F') {
   getline(fin, data);
   faction tmp;
   tmp.load_info(data);
   factions.push_back(tmp);
  }
 }
 fin.close();
 return true;
}

void game::load(std::string name)
{
 std::ifstream fin;
 std::stringstream playerfile;
 playerfile << "save/" << name << ".sav";
 fin.open(playerfile.str().c_str());
// First, read in basic game state information.
 if (!fin.is_open()) {
  debugmsg("No save game exists!");
  return;
 }
 u = player();
 u.name = name;
 u.ret_null = item(itypes[0], 0);
 u.weapon = item(itypes[0], 0);
 int tmprun, tmptar, tmptemp, comx, comy;
 fin >> turn >> tmptar >> tmprun >> mostseen >> nextinv >> next_npc_id >>
        next_mission_id >> nextspawn >> tmptemp >> levx >> levy >> levz >>
        comx >> comy;
 cur_om = overmap(this, comx, comy, levz);
// m = map(&itypes, &mapitems, &traps); // Init the root map with our vectors
 m.load(this, levx, levy);
 run_mode = tmprun;
 last_target = tmptar;
 temperature = tmptemp;
// Next, the scent map.
 for (int i = 0; i < SEEX * 3; i++) {
  for (int j = 0; j < SEEY * 3; j++)
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
 if (fin.peek() == '\n')
  fin.get(junk); // Chomp that pesky endline
 for (int i = 0; i < nummon; i++) {
  getline(fin, data);
  montmp.load_info(data, &mtypes);
  z.push_back(montmp);
 }
// Finally, the data on the player.
 if (fin.peek() == '\n')
  fin.get(junk); // Chomp that pesky endline
 getline(fin, data);
 u.load_info(this, data);
// And the player's inventory...
 char item_place;
 std::string itemdata;
 while (!fin.eof()) {
  fin >> item_place;
  if (!fin.eof()) {
   getline(fin, itemdata);
   if (item_place == 'I')
    u.inv.push_back(item(itemdata, this));
   else if (item_place == 'C')
    u.inv[u.inv.size() - 1].contents.push_back(item(itemdata, this));
   else if (item_place == 'W')
    u.worn.push_back(item(itemdata, this));
   else if (item_place == 'w')
    u.weapon = item(itemdata, this);
   else if (item_place == 'c')
    u.weapon.contents.push_back(item(itemdata, this));
  }
 }
// And the kill counts;
 for (int i = 0; i < num_monsters; i++)
  fin >> kills[i];
 fin.close();
// Now load up the master game data; factions (and more?)
 load_master();
 draw();
}

void game::save()
{
 std::stringstream playerfile, masterfile;
 std::ofstream fout;
 playerfile << "save/" << u.name << ".sav";
 masterfile << "save/master.gsav";
 fout.open(playerfile.str().c_str());
// First, write out basic game state information.
 fout << turn << " " << int(last_target) << " " << int(run_mode) << " " <<
         mostseen << " " << nextinv << " " << next_npc_id << " " <<
         next_mission_id << " " << nextspawn << " " << int(temperature) <<
         " " << levx << " " << levy << " " << levz << " " << cur_om.posx <<
         " " << cur_om.posy << " " << std::endl;
// Next, the scent map.
 for (int i = 0; i < SEEX * 3; i++) {
  for (int j = 0; j < SEEY * 3; j++)
   fout << grscent[i][j] << " ";
 }
// Now save all monsters.
 fout << std::endl << z.size() << std::endl;
 for (int i = 0; i < z.size(); i++)
  fout << z[i].save_info() << std::endl;
// And finally the player.
 fout << u.save_info() << std::endl;
 for (int i = 0; i < num_monsters; i++)	// Save the kill counts, too.
  fout << kills[i] << " ";
 fout << std::endl;
 fout.close();
// Now write things that aren't player-specific: factions and NPCs
 fout.open(masterfile.str().c_str());
 for (int i = 0; i < factions.size(); i++)
  fout << "F " << factions[i].save_info() << std::endl;
 fout.close();
// aaaand the overmap, and the local map.
 cur_om.save(u.name);
 m.save(&cur_om, turn, levx, levy);
}

void game::advance_nextinv()
{
 if (nextinv == 'z')
  nextinv = 'A';
 else if (nextinv == 'Z')
  nextinv = 'a';
 else
  nextinv++;
}

void game::add_msg(const char* msg, ...)
{
 char buff[512];
 va_list ap;
 va_start(ap, msg);
 vsprintf(buff, msg, ap);
 va_end(ap);

 int maxlength = 80 - (SEEX * 2 + 10);	// Matches write_msg() below
 if (messages.size() == 256)
  messages.erase(messages.begin());
 curmes++;
 std::string s(buff);
 size_t split;
 while (s.length() > maxlength) {
  split = s.find_last_of(' ', maxlength);
  if (split > maxlength)
   split = maxlength;
  messages.push_back(s.substr(0, split));
  curmes++;
  s = s.substr(split);
 }
 messages.push_back(s);
}

void game::add_event(event_type type, int on_turn, faction* rel)
{
 debugmsg("type %d, on turn %d (%d)", type, on_turn, turn);
 event tmp(type, on_turn, rel);
 events.push_back(tmp);
}

void game::debug()
{
// WINDOW *w = newwin(80, 25, 0, 0);
// mvwprintw(w,
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
 mvprintw(0, 0, "OM %d : %d    M %d : %d", cur_om.posx, cur_om.posy, levx,
                                           levy);
 int dist, linenum = 1;
 for (int i = 0; i < cur_om.zg.size(); i++) {
  dist = trig_dist(levx, levy, cur_om.zg[i].posx, cur_om.zg[i].posy);
  if (dist <= cur_om.zg[i].radius) {
   mvprintw(linenum, 0, "Zgroup %d: Centered at %d:%d, radius %d, pop %d",
            i, cur_om.zg[i].posx, cur_om.zg[i].posy, cur_om.zg[i].radius,
            cur_om.zg[i].population);
   linenum++;
  }
 }
 getch();
}

void game::draw_overmap()
{
 point tmp = cur_om.choose_point(this);
 if (tmp.x != -1) {
  z.clear();
  m.save(&cur_om, turn, levx, levy);
  levx = tmp.x * 2;
  levy = tmp.y * 2;
  m.load(this, levx, levy);
 }
}

void game::disp_kills()
{
 WINDOW* w = newwin(25, 80, 0, 0);
 std::vector<mtype *> types;
 std::vector<int> count;
 for (int i = 0; i < num_monsters; i++) {
  if (kills[i] > 0) {
   types.push_back(mtypes[i]);
   count.push_back(kills[i]);
  }
 }

 mvwprintz(w, 0, 35, c_red, "KILL COUNTS:");

 if (types.size() == 0) {
  mvwprintz(w, 2, 2, c_white, "You haven't killed any monsters yet!");
  wrefresh(w);
  getch();
  werase(w);
  wrefresh(w);
  delwin(w);
  refresh_all();
  return;
 }

 for (int i = 0; i < types.size(); i++) {
  if (i < 24) {
   mvwprintz(w, i + 1,  0, types[i]->color, "%c %s", types[i]->sym,
             types[i]->name.c_str());
   int hori = 25;
   if (count[i] >= 10)
    hori = 24;
   if (count[i] >= 100)
    hori = 23;
   if (count[i] >= 1000)
    hori = 22;
   mvwprintz(w, i + 1, hori, c_white, "%d", count[i]);
  } else {
   mvwprintz(w, i + 1, 40, types[i]->color, "%c %s", types[i]->sym,
             types[i]->name.c_str());
   int hori = 65;
   if (count[i] >= 10)
    hori = 64;
   if (count[i] >= 100)
    hori = 63;
   if (count[i] >= 1000)
    hori = 62;
   mvwprintz(w, i + 1, hori, c_white, "%d", count[i]);
  }
 }

 wrefresh(w);
 getch();
 werase(w);
 wrefresh(w);
 delwin(w);
 refresh_all();
}

void game::disp_NPCs()
{
 WINDOW* w = newwin(25, 80, 0, 0);
 mvwprintz(w, 0, 0, c_white, "Your position: %d:%d", levx, levy);
 std::vector<npc*> closest;
 closest.push_back(&cur_om.npcs[0]);
 for (int i = 1; i < cur_om.npcs.size(); i++) {
  if (closest.size() < 20)
   closest.push_back(&cur_om.npcs[i]);
  else if (rl_dist(levx, levy, cur_om.npcs[i].mapx, cur_om.npcs[i].mapy) <
           rl_dist(levx, levy, closest[19]->mapx, closest[19]->mapy)) {
   for (int j = 0; j < 20; j++) {
    if (rl_dist(levx, levy, closest[j]->mapx, closest[j]->mapy) >
        rl_dist(levx, levy, cur_om.npcs[i].mapx, cur_om.npcs[i].mapy)) {
     closest.insert(closest.begin() + j, &cur_om.npcs[i]);
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
  popup("You don't know of any factions.  Press Spacebar...");
  return NULL;
 }
 WINDOW* w_list = newwin(25,      MAX_FAC_NAME_SIZE, 0, 0);
 WINDOW* w_info = newwin(25, 80 - MAX_FAC_NAME_SIZE, 0, MAX_FAC_NAME_SIZE);
 int maxlength = 79 - MAX_FAC_NAME_SIZE;
 int sel = 0;

// Init w_list content
 mvwprintz(w_list, 0, 0, c_white, title.c_str());
 for (int i = 0; i < valfac.size(); i++) {
  nc_color col = (i == 0 ? h_white : c_white);
  mvwprintz(w_list, i + 1, 0, col, valfac[i].name.c_str());
 }
 wrefresh(w_list);
// Init w_info content
// fac_*_text() is in faction.cpp
 mvwprintz(w_info, 0, 0, c_white,
          "Ranking: %s", fac_ranking_text(valfac[0].likes_u).c_str());
 mvwprintz(w_info, 1, 0, c_white,
          "Respect: %s", fac_respect_text(valfac[0].respects_u).c_str());
 std::string desc = valfac[0].describe();
 int linenum = 3;
 while (desc.length() > maxlength) {
  size_t split = desc.find_last_of(' ', maxlength);
  std::string line = desc.substr(0, split);
  mvwprintz(w_info, linenum, 0, c_white, line.c_str());
  desc = desc.substr(split + 1);
  linenum++;
 }
 mvwprintz(w_info, linenum, 0, c_white, desc.c_str());
 wrefresh(w_info);
 char ch;
 do {
  ch = input();
  switch ( ch ) {
  case 'j':	// Move selection down
   mvwprintz(w_list, sel + 1, 0, c_white, valfac[sel].name.c_str());
   if (sel == valfac.size() - 1)
    sel = 0;	// Wrap around
   else
    sel++;
   break;
  case 'k':	// Move selection up
   mvwprintz(w_list, sel + 1, 0, c_white, valfac[sel].name.c_str());
   if (sel == 0)
    sel = valfac.size() - 1;	// Wrap around
   else
    sel--;
   break;
  case KEY_ESCAPE:
  case 'q':
   sel = -1;
   break;
  }
  if (ch == 'j' || ch == 'k') {	// Changed our selection... update the windows
   mvwprintz(w_list, sel + 1, 0, h_white, valfac[sel].name.c_str());
   wrefresh(w_list);
   werase(w_info);
// fac_*_text() is in faction.cpp
   mvwprintz(w_info, 0, 0, c_white,
            "Ranking: %s", fac_ranking_text(valfac[sel].likes_u).c_str());
   mvwprintz(w_info, 1, 0, c_white,
            "Respect: %s", fac_respect_text(valfac[sel].respects_u).c_str());
   std::string desc = valfac[sel].describe();
   int linenum = 3;
   while (desc.length() > maxlength) {
    size_t split = desc.find_last_of(' ', maxlength);
    std::string line = desc.substr(0, split);
    mvwprintz(w_info, linenum, 0, c_white, line.c_str());
    desc = desc.substr(split + 1);
    linenum++;
   }
   mvwprintz(w_info, linenum, 0, c_white, desc.c_str());
   wrefresh(w_info);
  }
 } while (ch != KEY_ESCAPE && ch != '\n' && ch != 'q');
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
 WINDOW *w_missions = newwin(25, 80, 0, 0);
 int tab = 0, selection = 0;
 char ch;
 bool redraw = true;
 do {
  werase(w_missions);
  draw_tabs(w_missions, tab, "ACTIVE MISSIONS", "COMPLETED MISSIONS",
            "FAILED MISSIONS", NULL);
  std::vector<mission> umissions;
  switch (tab) {
   case 0: umissions = u.active_missions;	break;
   case 1: umissions = u.completed_missions;	break;
   case 2: umissions = u.failed_missions;	break;
  }
  for (int y = 3; y < 25; y++)
   mvwputch(w_missions, y, 30, c_white, LINE_XOXO);
  for (int i = 0; i < umissions.size(); i++) {
   if (selection == i)
    mvwprintz(w_missions, 3 + i, 0, h_white, umissions[i].name().c_str());
   else
    mvwprintz(w_missions, 3 + i, 0, c_white, umissions[i].name().c_str());
  }

  if (selection >= 0 && selection < umissions.size()) {
   mvwprintz(w_missions, 4, 31, c_white,
             umissions[selection].description.c_str());
   if (umissions[selection].deadline != 0)
    mvwprintz(w_missions, 5, 31, c_white, "Deadline: %d (%d)",
              umissions[selection].deadline, turn);
   mvwprintz(w_missions, 6, 31, c_white, "Target: (%d, %d)   You: (%d, %d)",
             umissions[selection].target.x, umissions[selection].target.y,
             (levx + 1) / 2, (levy + 1) / 2);
  } else {
   std::string nope;
   switch (tab) {
    case 0: nope = "You have no active missions!"; break;
    case 1: nope = "You haven't completed any missions!"; break;
    case 2: nope = "You haven't failed and missions!"; break;
   }
   mvwprintz(w_missions, 4, 31, c_ltred, nope.c_str());
  }

  wrefresh(w_missions);
  ch = input();
  switch (ch) {
  case '>':
   tab++;
   if (tab == 3)
    tab = 0;
   break;
  case '<':
   tab--;
   if (tab < 0)
    tab = 2;
   break;
  case 'j':
   selection++;
   if (selection >= umissions.size())
    selection = 0;
   break;
  case 'k':
   selection--;
   if (selection < 0)
    selection = umissions.size() - 1;
   break;
  }

 } while (ch != 'q' && ch != 'Q' && ch != KEY_ESCAPE);


 werase(w_missions);
 delwin(w_missions);
 refresh_all();
}

void game::display_scent()
{
 int sc;
 char ch[1];
 for (int x = u.posx - SEEX; x <= u.posx + SEEX; x++) {
  for (int y = u.posy - SEEY; y <= u.posy + SEEY; y++) {
   sc = scent(x, y);
   sprintf(ch, "%d", 10 * (sc - ( (sc - 1) / (u.scent / 6) ) ) );
   mvputch(y + SEEY - u.posy, x + SEEX - u.posx, sev((sc - 1) / (u.scent / 6)),
           ch[0]);
  }
 }
}

void game::tutorial_message(tut_lesson lesson)
{
// Cycle through intro lessons
 if (lesson == LESSON_INTRO) {
  while (lesson != NUM_LESSONS && tutorials_seen[lesson]) {
   switch (lesson) {
    case LESSON_INTRO:	lesson = LESSON_MOVE; break;
    case LESSON_MOVE:	lesson = LESSON_LOOK; break;
    case LESSON_LOOK:	lesson = NUM_LESSONS; break;
   }
  }
  if (lesson == NUM_LESSONS)
   return;
 }
 if (tutorials_seen[lesson])
  return;
 tutorials_seen[lesson] = true;
 popup_top(tut_text[lesson].c_str());
 refresh_all();
}

void game::draw()
{
 // Draw map
 werase(w_terrain);
 draw_ter();
 mon_info();
 // Draw Status
 draw_HP();
 werase(w_status);
 u.disp_status(w_status);
 int minutes = int(turn / 10);	// One turn = 6 seconds
 int hours = 8 + int(minutes / 60);
 minutes = minutes % 60;
 int day = 1 + int(hours / 24);
 hours = hours % 24;
 day = day % 14;
 if (hours == 12)
  mvwprintz(w_status, 1, 41, c_white, "12:%02d PM", minutes);
 else if (hours == 0)
  mvwprintz(w_status, 1, 41, c_white, "12:%02d AM", minutes);
 else if (hours > 12)
  mvwprintz(w_status, 1, 41, c_white, "%d:%02d PM", hours - 12, minutes);
 else
  mvwprintz(w_status, 1, 41, c_white, "%d:%02d AM", hours, minutes);

 oter_id cur_ter = cur_om.ter((levx + 1) / 2, (levy + 1) / 2);
 std::string tername = oterlist[cur_ter].name;
 if (tername.length() > 14)
  tername = tername.substr(0, 14);
 mvwprintz(w_status, 0,  0, oterlist[cur_ter].color, tername.c_str());
 if (levz < 0)
  mvwprintz(w_status, 0, 18, c_ltgray, "Underground");
 else
  mvwprintz(w_status, 0, 18, weather_data[weather].color,
                             weather_data[weather].name.c_str());
 nc_color col_temp = c_blue;
 if (temperature >= 90)
  col_temp = c_red;
 else if (temperature >= 75)
  col_temp = c_yellow;
 else if (temperature >= 60)
  col_temp = c_ltgreen;
 else if (temperature >= 50)
  col_temp = c_cyan;
 else if (temperature >  32)
  col_temp = c_ltblue;
 wprintz(w_status, col_temp, " %dF", temperature);
 switch (season) {
  case SPRING: mvwprintz(w_status, 0, 41, c_white, "Spring, day %d", day);break;
  case SUMMER: mvwprintz(w_status, 0, 41, c_white, "Summer, day %d", day);break;
  case AUTUMN: mvwprintz(w_status, 0, 41, c_white, "Autumn, day %d", day);break;
  case WINTER: mvwprintz(w_status, 0, 41, c_white, "Winter, day %d", day);break;
 }
 if (run_mode != 0)
  mvwprintz(w_status, 2, 52, c_red, "RUN");
 wrefresh(w_status);
 // Draw messages
 write_msg();
}

void game::draw_ter()
{
 int t = 0;
 m.draw(this, w_terrain);

 // Draw monsters
 int distx, disty;
 for (int i = 0; i < z.size(); i++) {
  disty = abs(z[i].posy - u.posy);
  distx = abs(z[i].posx - u.posx);
  if (distx <= SEEX && disty <= SEEY && u_see(&(z[i]), t))
   z[i].draw(w_terrain, u.posx, u.posy, false);
  else if (z[i].has_flag(MF_WARM) && distx <= SEEX && disty <= SEEY &&
           (u.has_active_bionic(bio_infrared) || u.has_trait(PF_INFRARED)))
   mvwputch(w_terrain, SEEY + z[i].posy - u.posy, SEEX + z[i].posx - u.posx,
            c_red, '?');
 }
 // Draw NPCs
 for (int i = 0; i < active_npc.size(); i++) {
  disty = abs(active_npc[i].posy - u.posy);
  distx = abs(active_npc[i].posx - u.posx);
  if (distx <= SEEX && disty <= SEEY &&
      u_see(active_npc[i].posx, active_npc[i].posy, t))
   active_npc[i].draw(w_terrain, u.posx, u.posy, false);
 }
 wrefresh(w_terrain);
 if (u.has_disease(DI_VISUALS))
  hallucinate();
 if (in_tutorial && light_level() == 1) {
  if (u.has_amount(itm_flashlight, 1))
   tutorial_message(LESSON_DARK);
  else
   tutorial_message(LESSON_DARK_NO_FLASH);
 }
}

void game::refresh_all()
{
 draw();
 draw_minimap();
 werase(w_HP); // fix by headswe
 draw_HP();
 wrefresh(w_moninfo);
 wrefresh(w_messages);
 refresh();
}

void game::draw_HP()
{
 int curhp;
 nc_color col;
 for (int i = 0; i < num_hp_parts; i++) {
  curhp = u.hp_cur[i];
       if (curhp == u.hp_max[i])
   col = c_green;
  else if (curhp > u.hp_max[i] * .8)
   col = c_ltgreen;
  else if (curhp > u.hp_max[i] * .5)
   col = c_yellow;
  else if (curhp > u.hp_max[i] * .3)
   col = c_ltred;
  else
   col = c_red;
  if (u.has_trait(PF_HPIGNORANT)) {
   mvwprintz(w_HP, i * 2 + 1, 0, col, " ***  ");
  } else {
   if (curhp >= 100)
    mvwprintz(w_HP, i * 2 + 1, 0, col, "%d     ", curhp);
   else if (curhp >= 10)
    mvwprintz(w_HP, i * 2 + 1, 0, col, " %d    ", curhp);
   else
    mvwprintz(w_HP, i * 2 + 1, 0, col, "  %d    ", curhp);
  }
 }
 mvwprintz(w_HP,  0, 0, c_ltgray, "HEAD:");
 mvwprintz(w_HP,  2, 0, c_ltgray, "TORSO:");
 mvwprintz(w_HP,  4, 0, c_ltgray, "L. ARM:");
 mvwprintz(w_HP,  6, 0, c_ltgray, "R. ARM:");
 mvwprintz(w_HP,  8, 0, c_ltgray, "L. LEG:");
 mvwprintz(w_HP, 10, 0, c_ltgray, "R. LEG:");
 mvwprintz(w_HP, 12, 0, c_ltgray, "POW:");
 if (u.max_power_level == 0)
  mvwprintz(w_HP, 13, 0, c_ltgray, " --   ");
 else {
  if (u.power_level == u.max_power_level)
   col = c_blue;
  else if (u.power_level >= u.max_power_level * .5)
   col = c_ltblue;
  else if (u.power_level > 0)
   col = c_yellow;
  else
   col = c_red;
  if (u.power_level >= 100)
   mvwprintz(w_HP, 13, 0, col, "%d     ", u.power_level);
  else if (u.power_level >= 10)
   mvwprintz(w_HP, 13, 0, col, " %d    ", u.power_level);
  else
   mvwprintz(w_HP, 13, 0, col, "  %d    ", u.power_level);
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

 int cursx = (levx + 1) / 2;
 int cursy = (levy + 1) / 2;
 int omx, omy;
 oter_id cur_ter;
 nc_color ter_color;
 long ter_sym;
 bool seen = true;
 overmap *hori = NULL; //when it wasn't a pointer it used too much space on stack and died on windows
 overmap *vert = NULL; //not to forget to free it in the end of function
 if (cursx < 2)
  hori = new overmap(this, cur_om.posx - 1, cur_om.posy, 0);
 if (cursx > OMAPX - 3)
  hori = new overmap(this, cur_om.posx + 1, cur_om.posy, 0);
 if (cursy < 2)
  vert = new overmap(this, cur_om.posx, cur_om.posy - 1, 0);
 if (cursy > OMAPY - 3)
  vert = new overmap(this, cur_om.posx, cur_om.posy + 1, 0);
 for (int i = -1; i <= 1; i++) {
  for (int j = -1; j <= 1; j++) {
   omx = cursx + i;
   omy = cursy + j;
   if (omx >= 0 && omx < OMAPX && omy >= 0 && omy < OMAPY) {
    cur_ter = cur_om.ter(omx, omy);
    cur_om.seen(omx, omy) = true;
   } else if ((omx < 0 || omx >= OMAPX) && (omy < 0 || omy >= OMAPY)) {
    cur_ter = ot_null;
   } else if (omx < 0) {
    omx += OMAPX;
    cur_ter = hori->ter(omx, omy);
    hori->seen(omx, omy) = true;
   } else if (omx >= OMAPX) {
    omx -= OMAPX;
    cur_ter = hori->ter(omx, omy);
    hori->seen(omx, omy) = true;
   } else if (omy < 0) {
    omy += OMAPY;
    cur_ter = vert->ter(omx, omy);
    vert->seen(omx, omy) = true;
   } else if (omy >= OMAPY) {
    omy -= OMAPY;
    cur_ter = vert->ter(omx, omy);
    vert->seen(omx, omy) = true;
   } else {
    debugmsg("No data loaded! omx: %d omy: %d", omx, omy);
   }
   ter_color = oterlist[cur_ter].color;
   ter_sym = oterlist[cur_ter].sym;
   if (i == 0 && j == 0)
    mvwputch_hi(w_minimap, 3,     3,     ter_color, ter_sym);
   else
    mvwputch   (w_minimap, 3 + j, 3 + i, ter_color, ter_sym);
  }
 }
// Two spaces away!
 int barx, bary;
 for (int i = -2; i <= 2; i++) {
  for (int j = -2; j <= 2; j++) {
   omx = cursx + i;
   omy = cursy + j;
   barx = cursx + sgn(i);
   bary = cursy + sgn(j);
   if (abs(i) == 2 || abs(j) == 2) {
    if (i == 0) barx = cursx;
    if (j == 0) bary = cursy;
    seen = false;
    if (omx >= 0 && omx < OMAPX && omy >= 0 && omy < OMAPY)
     seen = cur_om.seen(omx, omy);
           if (barx >= 0 && barx < OMAPX && bary >= 0 && bary < OMAPY) {
     cur_ter = cur_om.ter(barx, bary);
    } else if ((barx < 0 || barx >= OMAPX) && (bary < 0 || bary >= OMAPY)) {
     cur_ter = ot_null;
    } else if (barx < 0) {
     barx += OMAPX;
     cur_ter = hori->ter(barx, bary);
    } else if (barx >= OMAPX) {
     barx -= OMAPX;
     cur_ter = hori->ter(barx, bary);
    } else if (bary < 0) {
     bary += OMAPY;
     cur_ter = vert->ter(barx, bary);
    } else if (bary >= OMAPY) {
     bary -= OMAPY;
     cur_ter = vert->ter(barx, bary);
    }
    if (oterlist[cur_ter].see_cost <= 2 || seen) {
            if (omx >= 0 && omx < OMAPX && omy >= 0 && omy < OMAPY) {
      cur_ter = cur_om.ter(omx, omy);
      cur_om.seen(omx, omy) = true;
     } else if ((omx < 0 || omx >= OMAPX) && (omy < 0 || omy >= OMAPY)) {
      cur_ter = ot_null;
     } else if (omx < 0) {
      omx += OMAPX;
      cur_ter = hori->ter(omx, omy);
      hori->seen(omx, omy) = true;
     } else if (omx >= OMAPX) {
      omx -= OMAPX;
      cur_ter = hori->ter(omx, omy);
      hori->seen(omx, omy) = true;
     } else if (omy < 0) {
      omy += OMAPY;
      cur_ter = vert->ter(omx, omy);
      vert->seen(omx, omy) = true;
     } else if (omy >= OMAPY) {
      omy -= OMAPY;
      cur_ter = vert->ter(omx, omy);
      vert->seen(omx, omy) = true;
     }
     ter_color = oterlist[cur_ter].color;
     ter_sym = oterlist[cur_ter].sym;
     mvwputch(w_minimap, 3 + j, 3 + i, ter_color, ter_sym);
    }
   }
  }
 }
 wrefresh(w_minimap);
 if(hori != NULL)delete hori;
 if(vert != NULL)delete vert;
}

void game::hallucinate()
{
 for (int i = 0; i <= SEEX * 2 + 1; i++) {
  for (int j = 0; j <= SEEY * 2 + 1; j++) {
   if (one_in(10)) {
    char ter_sym = terlist[m.ter(i + rng(-2, 2), j + rng(-2, 2))].sym;
    nc_color ter_col = terlist[m.ter(i + rng(-2, 2), j + rng(-2, 2))].color;
    mvwputch(w_terrain, j, i, ter_col, ter_sym);
   }
  }
 }
 wrefresh(w_terrain);
}

unsigned char game::light_level()
{
 int ret;
 if (levz < 0)	// Underground!
  ret = 1;
 else {
  int minutes = int(turn / 10);	// One turn = 6 seconds
  int hours = 8 + int(minutes / 60);
  minutes = minutes % 60;
  hours = hours % 24;
  if (hours < 6 || hours >= 21)
   ret = 1;
  else if (hours >= 6 && hours < 7)
   ret = int(minutes);
  else if (hours >= 20 && hours < 21)
   ret = 60 - int(minutes);
  else
   ret = 60;
 }
 int flashlight = u.active_item_charges(itm_flashlight_on);
 if (ret < 10 && flashlight > 0) {
  ret = flashlight;
  if (ret > 10)
   ret = 10;
 }
 if (ret < 8 && u.has_active_bionic(bio_flashlight))
  ret = 8;
 return ret;
}

int game::assign_npc_id()
{
 int ret = next_npc_id;
 next_npc_id++;
 return ret;
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
 faction newfac;
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
 faction newfac;
 do
  newfac.randomize();
 while (newfac.good > -5);
 newfac.id = factions.size();
 factions.push_back(newfac);
 return &(factions[factions.size() - 1]);
}

bool game::sees_u(int x, int y, int &t)
{
 return (!u.has_active_bionic(bio_cloak) &&
         m.sees(x, y, u.posx, u.posy, light_level(), t));
}

bool game::u_see(int x, int y, int &t)
{
 return m.sees(u.posx, u.posy, x, y, u.sight_range(light_level()), t);
}

bool game::u_see(monster *mon, int &t)
{
 if (mon->has_flag(MF_DIGS) && !u.has_active_bionic(bio_ground_sonar) &&
     rl_dist(u.posx, u.posy, mon->posx, mon->posy) > 1)
  return false;	// Can't see digging monsters until we're right next to them
 int range = u.sight_range(light_level());
 return m.sees(u.posx, u.posy, mon->posx, mon->posy, range, t);
}

bool game::pl_sees(player *p, monster *mon, int &t)
{
 if (mon->has_flag(MF_DIGS) && !p->has_active_bionic(bio_ground_sonar) &&
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
  for (int j = 0; j < active_npc[i].inv.size(); j++) {
   if (it == &(active_npc[i].inv[j]))
    return point(active_npc[i].posx, active_npc[i].posy);
  }
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
 for (int i = 0; i < u.inv.size(); i++) {
  if (it == &u.inv[i]) {
   u.i_remn(i);
   return;
  }
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
  if (it == &active_npc[i].weapon) {
   active_npc[i].remove_weapon();
   return;
  }
  for (int j = 0; j < active_npc[i].inv.size(); j++) {
   if (it == &active_npc[i].inv[j]) {
    active_npc[i].i_remn(j);
    return;
   }
  }
  for (int j = 0; j < u.worn.size(); j++) {
   if (it == &active_npc[i].worn[j]) {
    active_npc[i].worn.erase(u.worn.begin() + j);
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

void game::mon_info()
{
 werase(w_moninfo);
 int buff;
 int newseen = 0;
// 0 1 2
// 3 4 5
// 6 7 8
 std::vector<int> unique_types[10];
 int direction;
 for (int i = 0; i < z.size(); i++) {
  if (u_see(&(z[i]), buff)) {
   if (!z[i].is_fleeing(u) && z[i].friendly == 0)
    newseen++;
   if (z[i].posx < u.posx - SEEX) {
    if (z[i].posy < u.posy - SEEY)
     direction = 0;
    else if (z[i].posy > u.posy + SEEY)
     direction = 6;
    else
     direction = 3;
   } else if (z[i].posx > u.posx + SEEX) {
    if (z[i].posy < u.posy - SEEY)
     direction = 2;
    else if (z[i].posy > u.posy + SEEY)
     direction = 8;
    else
     direction = 5;
   } else {
    if (z[i].posy < u.posy - SEEY)
     direction = 1;
    else if (z[i].posy > u.posy + SEEY)
     direction = 7;
    else
     direction = 4;
   }

   if (!vector_has(unique_types[direction], z[i].type->id))
    unique_types[direction].push_back(z[i].type->id);
  }
 }
 for (int i = 0; i < active_npc.size(); i++) {
  if (u_see(active_npc[i].posx, active_npc[i].posy, buff)) { // TODO: NPC invis
   if (active_npc[i].attitude == NPCATT_KILL)
    newseen++;
   if (active_npc[i].posx < u.posx - SEEX) {
    if (active_npc[i].posy < u.posy - SEEY)
     direction = 0;
    else if (active_npc[i].posy > u.posy + SEEY)
     direction = 6;
    else
     direction = 3;
   } else if (active_npc[i].posx > u.posx + SEEX) {
    if (active_npc[i].posy < u.posy - SEEY)
     direction = 2;
    else if (active_npc[i].posy > u.posy + SEEY)
     direction = 8;
    else
     direction = 5;
   } else {
    if (active_npc[i].posy < u.posy - SEEY)
     direction = 1;
    else if (active_npc[i].posy > u.posy + SEEY)
     direction = 7;
    else
     direction = 4;
   }
   unique_types[direction].push_back(-1 - i);
  }
 }

 if (newseen > mostseen) {
  cancel_activity_query("Monster spotted!");
  if (run_mode == 1)
   run_mode = 2;	// Stop movement!
 }
 mostseen = newseen;
 int line = 0;
 nc_color tmpcol;
 for (int i = 0; i < 9; i++) {
  if (unique_types[i].size() > 0) {
   switch(i) {
    case 0: mvwprintz(w_moninfo, line, 0, c_magenta, "NORTHWEST");	break;
    case 1: mvwprintz(w_moninfo, line, 0, c_magenta, "NORTH");		break;
    case 2: mvwprintz(w_moninfo, line, 0, c_magenta, "NORTHEAST");	break;
    case 3: mvwprintz(w_moninfo, line, 0, c_magenta, "WEST");		break;
    case 4: mvwprintz(w_moninfo, line, 0, c_magenta, "NEARBY");		break;
    case 5: mvwprintz(w_moninfo, line, 0, c_magenta, "EAST");		break;
    case 6: mvwprintz(w_moninfo, line, 0, c_magenta, "SOUTHWEST");	break;
    case 7: mvwprintz(w_moninfo, line, 0, c_magenta, "SOUTH");		break;
    case 8: mvwprintz(w_moninfo, line, 0, c_magenta, "SOUTHEAST");	break;
   }
   line++;
  }
  for (int j = 0; j < unique_types[i].size() && line < 12; j++) {
   buff = unique_types[i][j];
   if (buff < 0) {
    switch (active_npc[(buff + 1) * -1].attitude) {
     case NPCATT_KILL:   tmpcol = c_red;     break;
     case NPCATT_FOLLOW: tmpcol = c_ltgreen; break;
     case NPCATT_DEFEND: tmpcol = c_green;   break;
     default:            tmpcol = c_pink;    break;
    }
    mvwputch (w_moninfo, line, 0, tmpcol, '@');
    mvwprintw(w_moninfo, line, 2, active_npc[(buff + 1) * -1].name.c_str());
   } else {
    mvwputch (w_moninfo, line, 0, mtypes[buff]->color, mtypes[buff]->sym);
    mvwprintw(w_moninfo, line, 2, mtypes[buff]->name.c_str());
   }
   line++;
  }
 }
 wrefresh(w_moninfo);
 refresh();
}

void game::monmove()
{
 for (int i = 0; i < z.size(); i++) {
  if (i < 0 || i > z.size())
   debugmsg("Moving out of bounds monster! i %d, z.size() %d", i, z.size());
  while (!z[i].can_move_to(m, z[i].posx, z[i].posy) && i < z.size()) {
// If we can't move to our current position, assign us to a new one
   bool okay = false;
   int xdir = rng(1, 2) * 2 - 3, ydir = rng(1, 2) * 2 - 3; // -1 or 1
   int startx = z[i].posx - 3 * xdir, endx = z[i].posx + 3 * xdir;
   int starty = z[i].posy - 3 * ydir, endy = z[i].posy + 3 * ydir;
   for (int x = startx; x != endx && !okay; x += xdir) {
    for (int y = starty; y != endy && !okay; y += ydir){
     if (z[i].can_move_to(m, x, y)) {
      z[i].posx = x;
      z[i].posy = y;
      okay = true;
     }
    }
   }
   if (!okay)
    z.erase(z.begin() + i);// Delete us if no replacement found
  }

  bool dead = false;
  z[i].process_effects(this);
  if (z[i].hurt(0)) {
   kill_mon(i);
   dead = true;
  }
  while (z[i].moves > 0 && !dead) {
   z[i].plan(this);	// Formulate a path to follow
   z[i].move(this);	// Move one square, possibly hit u
   m.mon_in_field(z[i].posx, z[i].posy, this, &(z[i]));
   if (z[i].hurt(0)) {	// Maybe we died...
    kill_mon(i);
    dead = true;
   }
  }
  if (dead)
   i--;
  else {
   if (in_tutorial && u.pain > 0)
    tutorial_message(LESSON_PAIN);
   if (u.has_active_bionic(bio_alarm) && u.power_level >= 1 &&
       abs(z[i].posx - u.posx) <= 5 && abs(z[i].posy - u.posy) <= 5) {
    u.power_level--;
    add_msg("Your motion alarm goes off!");
    u.activity.type = ACT_NULL;
    if (u.has_disease(DI_SLEEP) || u.has_disease(DI_LYING_DOWN)) {
     u.rem_disease(DI_SLEEP);
     u.rem_disease(DI_LYING_DOWN);
    }
   }
// We might have stumbled out of range of the player; if so, delete us
   if (z[i].posx < 0 - SEEX * 4 || z[i].posy < 0 - SEEX * 4 ||
       z[i].posx >     SEEX * 5 || z[i].posy >     SEEY * 5) {
    int group = valid_group((mon_id)(z[i].type->id), levx, levy);
    if (group != -1) {
     cur_om.zg[group].population++;
     if (cur_om.zg[group].population / pow(cur_om.zg[group].radius, 2) > 5)
      cur_om.zg[group].radius++;
    } else if (mt_to_mc((mon_id)(z[i].type->id)) != mcat_null) {
     cur_om.zg.push_back(mongroup(mt_to_mc((mon_id)(z[i].type->id)),
                                  levx, levy, 1, 1));
    }
    z.erase(z.begin()+i);
    i--;
   } else
    z[i].receive_moves();
  }
 }

// Now, do active NPCs.
 for (int i = 0; i < active_npc.size(); i++) {
  int turns = 0;
  if(active_npc[i].hp_cur[hp_head] <= 0 || active_npc[i].hp_cur[hp_torso] <= 0){
   active_npc[i].die(this);
   active_npc.erase(active_npc.begin() + i);
   i--;
  } else {
   active_npc[i].reset();
   active_npc[i].suffer(this);
   while (active_npc[i].moves > 0 && turns < 3) {
    turns++;
    active_npc[i].move(this);
   }
   if (turns == 3) {
    add_msg("%s's brain explodes!", active_npc[i].name.c_str());
    active_npc[i].die(this);
    active_npc.erase(active_npc.begin() + i);
    i--;
   }
  }
 }
}

void game::om_npcs_move()
{
/*
 for (int i = 0; i < cur_om.npcs.size(); i++) {
  cur_om.npcs[i].perform_mission(this);
  if (abs(cur_om.npcs[i].mapx - levx) <= 1 && abs(cur_om.npcs[i].mapy - levy) <= 1) {
   cur_om.npcs[i].posx = u.posx + SEEX * 2 * (cur_om.npcs[i].mapx - levx);
   cur_om.npcs[i].posy = u.posy + SEEY * 2 * (cur_om.npcs[i].mapy - levy);
   active_npc.push_back(cur_om.npcs[i]);
   cur_om.npcs.erase(cur_om.npcs.begin() + i);
   i--;
  }
 }
*/
}

void game::check_warmth()
{
 // HEAD
 int warmth = u.warmth(bp_head) + int((temperature - 65) / 10);
 if (warmth <= -6) {
  add_msg("Your head is freezing!");
  u.add_disease(DI_COLD, abs(warmth * 2), this);// Heat loss via head is bad
  u.hurt(this, bp_head, 0, rng(0, abs(warmth / 3)));
 } else if (warmth <= -3) {
  add_msg("Your head is cold.");
  u.add_disease(DI_COLD, abs(warmth * 2), this);
 } else if (warmth >= 8) {
/*
  add_msg("Your head is overheating!");
  u.add_disease(DI_HOT, warmth * 1.5, this);
*/
 }
 // FACE -- Mouth and eyes
 warmth = u.warmth(bp_eyes) + u.warmth(bp_mouth) + int((temperature - 65) / 10);
 if (warmth <= -6) {
  add_msg("Your face is freezing!");
  u.add_disease(DI_COLD_FACE, abs(warmth), this);
  u.hurt(this, bp_head, 0, rng(0, abs(warmth / 3)));
 } else if (warmth <= -4) {
  add_msg("Your face is cold.");
  u.add_disease(DI_COLD_FACE, abs(warmth), this);
 } else if (warmth >= 12) {
  add_msg("Your face is overheating!");
  u.add_disease(DI_HOT, warmth, this);
 }
 // TORSO
 warmth = u.warmth(bp_torso) + int((temperature - 65) / 10);
 if (warmth <= -8) {
  add_msg("Your body is freezing!");
  u.add_disease(DI_COLD, abs(warmth), this);
  u.hurt(this, bp_torso, 0, rng(0, abs(warmth / 4)));
 } else if (warmth <= -2) {
  add_msg("Your body is cold.");
  u.add_disease(DI_COLD, abs(warmth), this);
 } else if (warmth >= 12) {
/*
  add_msg("Your body is too hot."); 
  u.add_disease(DI_HOT, warmth * 2, this);
*/
 }
 // HANDS
 warmth = u.warmth(bp_hands) + int((temperature - 65) / 10);
 if (warmth <= -4) {
  add_msg("Your hands are freezing!");
  u.add_disease(DI_COLD_HANDS, abs(warmth), this);
 } else if (warmth >= 8) {
  add_msg("Your hands are overheating!");
  u.add_disease(DI_HOT, rng(0, warmth / 2), this);
 }
 // LEGS
 warmth = u.warmth(bp_legs) + int((temperature - 65) / 10);
 if (warmth <= -6) {
  add_msg("Your legs are freezing!");
  u.add_disease(DI_COLD_LEGS, abs(warmth), this);
 } else if (warmth <= -3) {
  add_msg("Your legs are very cold.");
  u.add_disease(DI_COLD_LEGS, abs(warmth), this);
 } else if (warmth >= 8) {
/*
  add_msg("Your legs are overheating!");
  u.add_disease(DI_HOT, rng(0, warmth), this);
*/
 }
 // FEET
 warmth = u.warmth(bp_feet) + int((temperature - 65) / 10);
 if (warmth <= -3) {
  add_msg("Your feet are freezing!");
  u.add_disease(DI_COLD_FEET, warmth, this);
 } else if (warmth >= 12) {
  add_msg("Your feet are overheating!");
  u.add_disease(DI_HOT, rng(0, warmth), this);
 }
}

void game::sound(int x, int y, int vol, std::string description)
{
// First, alert all monsters (that can hear) to the sound
 double dist;
 for (int i = 0; i < z.size(); i++) {
  if (z[i].has_flag(MF_HEARS)) {
   dist = trig_dist(x, y, z[i].posx, z[i].posy);
   if (z[i].has_flag(MF_GOODHEARING) && int(dist) >> 1 <= vol)
    z[i].wander_to(x, y, int(vol - (int(dist) >> 1)));
   else if (dist <= vol && dist >= 2) // Adjacent sounds are likely caused by us
    z[i].wander_to(x, y, int(vol - dist));
  }
 }
// Loud sounds make the next spawn sooner!
 if (vol >= 20 && nextspawn > vol + 20) {
  int max = (vol - 20);
  int min = int(max / 6);
  if (max > 50)
   max = 50;
  nextspawn -= rng(min, max);
 }
// Next, display the sound as the player hears it
 if (description == "")
  return;	// No description (e.g., footsteps)
 if (u.has_bionic(bio_ears))
  vol *= 3.5;
 if (u.has_trait(PF_BADHEARING))
  vol *= .5;
 dist = trig_dist(x, y, u.posx, u.posy);
 if (dist > vol)
  return;	// Too far away, we didn't hear it!
 if (u.has_disease(DI_SLEEP) &&
     ((!u.has_trait(PF_HEAVYSLEEPER) && dice(2, 20) < vol - dist) ||
      ( u.has_trait(PF_HEAVYSLEEPER) && dice(3, 20) < vol - dist)   )) {
  u.rem_disease(DI_SLEEP);
  add_msg("You're woken up by a noise.");
  return;
 }
 cancel_activity_query("Heard a noise!");
// We need to figure out where it was coming from, relative to the player
 int dx = x - u.posx;
 int dy = y - u.posy;
// If it came from us, don't print a direction
 if (dx == 0 && dy == 0) {
  if (description[0] >= 'a' && description[0] <= 'z')
   description[0] += 'A' - 'a';	// Capitalize the sound
  add_msg("%s", description.c_str());
  return;
 }
 std::string direction = direction_name(direction_from(u.posx, u.posy, x, y));
 add_msg("From the %s you hear %s", direction.c_str(), description.c_str());
}

void game::explosion(int x, int y, int power, int shrapnel, bool fire)
{
 timespec ts;	// Timespec for the animation of the explosion
 ts.tv_sec = 0;
 ts.tv_nsec = EXPLOSION_SPEED;
 int radius = sqrt(power / 4);
 int dam;
 std::string junk;
 if (power >= 30)
  sound(x, y, power * 10, "a huge explosion!");
 else
  sound(x, y, power * 10, "an explosion!");
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
   if (mon_hit != -1 && z[mon_hit].hurt(rng(dam / 2, dam * 1.5))) {
    if (z[mon_hit].hp < 0 - 1.5 * z[mon_hit].type->hp)
     explode_mon(mon_hit); // Explode them if it was big overkill
    else
     kill_mon(mon_hit);
   }
   if (npc_hit != -1) {
    active_npc[npc_hit].hit(this, bp_torso, 0, rng(dam / 2, dam * 1.5), 0);
    active_npc[npc_hit].hit(this, bp_head,  0, rng(dam / 3, dam),       0);
    active_npc[npc_hit].hit(this, bp_legs,  0, rng(dam / 3, dam),       0);
    active_npc[npc_hit].hit(this, bp_legs,  1, rng(dam / 3, dam),       0);
    active_npc[npc_hit].hit(this, bp_arms,  0, rng(dam / 3, dam),       0);
    active_npc[npc_hit].hit(this, bp_arms,  1, rng(dam / 3, dam),       0);
    if (active_npc[npc_hit].hp_cur[hp_head]  <= 0 ||
        active_npc[npc_hit].hp_cur[hp_torso] <= 0   ) {
     active_npc[npc_hit].die(this, true);
     active_npc.erase(active_npc.begin() + npc_hit);
    }
   }
   if (u.posx == i && u.posy == j) {
    add_msg("You're caught in the explosion!");
    u.hit(this, bp_torso, 0, rng(dam / 2, dam * 1.5), 0);
    u.hit(this, bp_head,  0, rng(dam / 3, dam),       0);
    u.hit(this, bp_legs,  0, rng(dam / 3, dam),       0);
    u.hit(this, bp_legs,  1, rng(dam / 3, dam),       0);
    u.hit(this, bp_arms,  0, rng(dam / 3, dam),       0);
    u.hit(this, bp_arms,  1, rng(dam / 3, dam),       0);
   }
   if (fire) {
    if (m.field_at(i, j).type == fd_smoke)
     m.field_at(i, j) = field(fd_fire, 1, 0);
    m.add_field(this, i, j, fd_fire, dam / 10);
   }
  }
 }
// Draw the explosion
 for (int i = 1; i <= radius; i++) {
  mvwputch(w_terrain, y - i + SEEY - u.posy, x - i + SEEX - u.posx, c_red, '/');
  mvwputch(w_terrain, y - i + SEEY - u.posy, x + i + SEEX - u.posx, c_red,'\\');
  mvwputch(w_terrain, y + i + SEEY - u.posy, x - i + SEEX - u.posx, c_red,'\\');
  mvwputch(w_terrain, y + i + SEEY - u.posy, x + i + SEEX - u.posx, c_red, '/');
  for (int j = 1 - i; j < 0 + i; j++) {
   mvwputch(w_terrain, y - i + SEEY - u.posy, x + j + SEEX - u.posx, c_red,'-');
   mvwputch(w_terrain, y + i + SEEY - u.posy, x + j + SEEX - u.posx, c_red,'-');
   mvwputch(w_terrain, y + j + SEEY - u.posy, x - i + SEEX - u.posx, c_red,'|');
   mvwputch(w_terrain, y + j + SEEY - u.posy, x + i + SEEX - u.posx, c_red,'|');
  }
  wrefresh(w_terrain);
  nanosleep(&ts, NULL);
 }

// The rest of the function is shrapnel
 if (shrapnel <= 0)
  return;
 int sx, sy, t, ijunk, tx, ty;
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
  dam = rng(10, 40);
  for (int j = 0; j < traj.size(); j++) {
   if (j > 0 && u_see(traj[j - 1].x, traj[j - 1].y, ijunk))
    m.drawsq(w_terrain, u, traj[j - 1].x, traj[j - 1].y, false, true);
   if (u_see(traj[j].x, traj[j].y, ijunk)) {
    mvwputch(w_terrain, traj[j].y + SEEY - u.posy,
                        traj[j].x + SEEX - u.posx, c_red, '`');
    wrefresh(w_terrain);
    nanosleep(&ts, NULL);
   }
   tx = traj[j].x;
   ty = traj[j].y;
   if (mon_at(tx, ty) != -1) {
    dam -= z[mon_at(tx, ty)].armor();
    if (z[mon_at(tx, ty)].hurt(dam))
     kill_mon(mon_at(tx, ty));
   } else if (npc_at(tx, ty) != -1) {
    body_part hit = random_body_part();
    if (hit == bp_eyes || hit == bp_mouth || hit == bp_head)
     dam = rng(2 * dam, 5 * dam);
    else if (hit == bp_torso)
     dam = rng(1.5 * dam, 3 * dam);
    int npcdex = npc_at(tx, ty);
    active_npc[npcdex].hit(this, hit, rng(0, 1), 0, dam);
    if (active_npc[npcdex].hp_cur[hp_head] <= 0 ||
        active_npc[npcdex].hp_cur[hp_torso] <= 0) {
     active_npc[npcdex].die(this);
     active_npc.erase(active_npc.begin() + npcdex);
    }
   } else if (tx == u.posx && ty == u.posy) {
    body_part hit = random_body_part();
    int side = rng(0, 1);
    add_msg("Shrapnel hits your %s!", body_part_name(hit, side).c_str());
    u.hit(this, hit, rng(0, 1), 0, dam);
   } else
    m.shoot(this, tx, ty, dam, j == traj.size() - 1);
  }
 }
}

void game::use_computer(int x, int y)
{
 computerk used;
 u.practice(sk_computer, 5);
 switch (m.ter(x, y)) {
 case t_computer_nether:
  used.difficulty = 4;
  used.failure = &computerk::spawn_manhacks;
  used.add_opt("Release Specimens",		&computerk::release);
  used.add_opt("Terminate Specimens",		&computerk::terminate);
  used.add_opt("Flash Portal",			&computerk::portal);
  used.add_opt("Activate Resonance Cascade",	&computerk::cascade);
  break;
 case t_computer_lab:
  used.difficulty = 2;
  used.failure = &computerk::spawn_manhacks;
  used.add_opt("Read Research Logs",		&computerk::research);
  used.add_opt("Download Map Data",		&computerk::maps);
  break;
 case t_computer_silo:
  used.difficulty = 5;
  used.failure = &computerk::spawn_secubots;
  used.add_opt("Download Map Data",		&computerk::maps);
  used.add_opt("Launch Missile",		&computerk::launch);
  used.add_opt("Disarm Missile",		&computerk::disarm);
  break;
 default:
  debugmsg("%s is not a computer!", m.tername(x, y).c_str());
  return;
 }
 int success = u.sklevel[sk_computer] + int((u.int_cur - 8) / 3) +
               rng(int(used.difficulty * .5), int(used.difficulty * 1.5));
 int choice = menu_vec(m.tername(x, y).c_str(), used.list_opts());
 choice--;
 if (success < -2) {
  (used.*used.failure)(this);
   return;
 }
 if (!(used.*used.options[choice].action)(this, success))
  m.ter(x, y) = t_computer_broken;
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
  u.add_disease(DI_TELEGLOW, rng(minglow, maxglow) * 100, this);
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
      if (m.field_at(k, l).type == fd_null || !one_in(3))
       m.field_at(k, l) = field(type, 3, 0);
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
    spawn = moncats[mcat_nether][rng(0, moncats[mcat_nether].size() - 1)];
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

void game::emp_blast(int x, int y)
{
 int rn;
 if (m.has_flag(computer, x, y)) {
  add_msg("The %s is rendered non-functional!", m.tername(x, y).c_str());
  m.ter(x, y) = t_computer_broken;
  return;
 }
// TODO: More terrain effects.
 switch (m.ter(x, y)) {
 case t_card_reader:
  rn = rng(1, 100);
  if (rn > 92 || rn < 40) {
   add_msg("The card reader is rendered non-functional.");
   m.ter(x, y) = t_card_reader_broken;
  }
  if (rn > 80) {
   add_msg("The nearby doors slide open!");
   for (int i = -5; i <= 5; i++) {
    for (int j = -5; j <= 5; j++) {
     if (m.ter(x + i, y + j) == t_door_metal_locked)
      m.ter(x + i, y + j) = t_floor;
    }
   }
  }
  if (rn >= 40 && rn <= 80)
   add_msg("Nothing happens.");
  break;
 }
 int mondex = mon_at(x, y);
 if (mondex != -1) {
  if (z[mondex].has_flag(MF_ELECTRONIC)) {
   add_msg("The EMP blast fries the %s!", z[mondex].name().c_str());
   int dam = dice(10, 10);
   if (z[mondex].hurt(dam))
    kill_mon(mondex);
  } else
   add_msg("The %s is unaffected by the EMP blast.", z[mondex].name().c_str());
 }
 if (u.posx == x && u.posy == y) {
  if (u.power_level > 0) {
   add_msg("The EMP blast drains your power.");
   u.charge_power(rng(-20, -5));
  }
// TODO: More effects?
 }
// Drain any items of their battery charge
 for (int i = 0; i < m.i_at(x, y).size(); i++) {
  if (m.i_at(x, y)[i].is_tool() &&
      (dynamic_cast<it_tool*>(m.i_at(x, y)[i].type))->ammo == AT_BATT)
   m.i_at(x, y)[i].charges = 0;
 }
// TODO: Drain NPC energy reserves
}

int game::npc_at(int x, int y)
{
 for (int i = 0; i < active_npc.size(); i++) {
  if (active_npc[i].posx == x && active_npc[i].posy == y)
   return i;
 }
 return -1;
}

int game::mon_at(int x, int y)
{
 for (int i = 0; i < z.size(); i++) {
  if (z[i].posx == x && z[i].posy == y)
   return i;
 }
 return -1;
}

bool game::is_empty(int x, int y)
{
 return (m.move_cost(x, y) > 0 && npc_at(x, y) == -1 && mon_at(x, y) == -1 &&
         (u.posx != x || u.posy != y));
}

bool game::is_in_sunlight(int x, int y)
{
 return (m.is_outside(x, y) && light_level() >= 40 &&
         (weather == WEATHER_CLEAR || weather == WEATHER_SUNNY));
}

void game::kill_mon(int index)
{
 if (index < 0 || index >= z.size()) {
  debugmsg("Tried to kill monster %d! (%d in play)", index, z.size());
  return;
 }
 if (z[index].dead)
  return;
 z[index].dead = true;
 kills[z[index].type->id]++;	// Increment our kill counter
 for (int i = 0; i < z[index].inv.size(); i++)
  m.add_item(z[index].posx, z[index].posy, z[index].inv[i]);
 z[index].die(this);
// If they left a corpse, give a tutorial message on butchering
 if (in_tutorial && !(tutorials_seen[LESSON_BUTCHER])) {
  for (int i = 0; i < m.i_at(z[index].posx, z[index].posy).size(); i++) {
   if (m.i_at(z[index].posx, z[index].posy)[i].type->id == itm_corpse) {
    tutorial_message(LESSON_BUTCHER);
    i = m.i_at(z[index].posx, z[index].posy).size();
   }
  }
 }
 z.erase(z.begin()+index);
 if (last_target == index)
  last_target = -1;
 else if (last_target > index)
   last_target--;
}

void game::explode_mon(int index)
{
 if (index < 0 || index >= z.size()) {
  debugmsg("Tried to explode monster %d! (%d in play)", index, z.size());
  return;
 }
 if (z[index].dead)
  return;
 z[index].dead = true;
 kills[z[index].type->id]++;	// Increment our kill counter
// Send body parts and blood all over!
 mtype* corpse = z[index].type;
 if (corpse->mat == FLESH || corpse->mat == VEGGY) { // No chunks otherwise
  int num_chunks;
  switch (corpse->size) {
   case MS_TINY:   num_chunks =  1; break;
   case MS_SMALL:  num_chunks =  2; break;
   case MS_MEDIUM: num_chunks =  4; break;
   case MS_LARGE:  num_chunks =  8; break;
   case MS_HUGE:   num_chunks = 16; break;
  }
  itype* meat;
  if (corpse->flags & mfb(MF_POISON)) {
   if (corpse->mat == FLESH)
    meat = itypes[itm_meat_tainted];
   else
    meat = itypes[itm_veggy_tainted];
  } else {
   if (corpse->mat == FLESH)
    meat = itypes[itm_meat];
   else
    meat = itypes[itm_veggy];
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
    if (m.field_at(tarx, tary).type == blood_type &&
        m.field_at(tarx, tary).density < 3)
     m.field_at(tarx, tary).density++;
    else
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
   m.add_item(tarx, tary, meat, turn);
  }
 }

 z.erase(z.begin()+index);
 if (last_target == index)
  last_target = -1;
 else if (last_target > index)
   last_target--;
}

void game::open()
{
 u.moves -= 100;
 bool didit = false;
 mvwprintw(w_terrain, 0, 0, "Open where? (hjklyubn) ");
 wrefresh(w_terrain);
 int openx, openy;
 char ch = input();
 last_action += ch;
 get_direction(openx, openy, ch);
 if (openx != -2 && openy != -2)
  if (m.ter(u.posx, u.posy) == t_floor)
   didit = m.open_door(u.posx + openx, u.posy + openy, true);
  else
   didit = m.open_door(u.posx + openx, u.posy + openy, false);
 else
  add_msg("Invalid direction.");
 if (!didit) {
  switch(m.ter(u.posx + openx, u.posy + openy)) {
  case t_door_locked:
   add_msg("The door is locked!");
   break;	// Trying to open a locked door uses the full turn's movement
  case t_door_o:
   add_msg("That door is already open.");
   u.moves += 100;
   break;
  default:
   add_msg("No door there.");
   u.moves += 100;
  }
 }
 if (in_tutorial)
  tutorial_message(LESSON_CLOSE);
}

void game::close()
{
 bool didit = false;
 mvwprintw(w_terrain, 0, 0, "Close where? (hjklyubn) ");
 wrefresh(w_terrain);
 int closex, closey;
 char ch = input();
 last_action += ch;
 get_direction(closex, closey, ch);
 if (closex != -2 && closey != -2) {
  closex += u.posx;
  closey += u.posy;
  if (mon_at(closex, closey) != -1)
   add_msg("There's a %s in the way!",z[mon_at(closex, closey)].name().c_str());
  else if (m.i_at(closex, closey).size() > 0)
   add_msg("There's %s in the way!", m.i_at(closex, closey).size() == 1 ?
           m.i_at(closex, closey)[0].tname(this).c_str() : "some stuff");
  else
   didit = m.close_door(closex, closey);
 } else
  add_msg("Invalid direction.");
 if (didit)
  u.moves -= 90;
 if (in_tutorial)
  tutorial_message(LESSON_SMASH);
}

void game::smash()
{
 bool didit = false;
 std::string bashsound, extra;
 int smashskill = int(u.str_cur / 2.5 + u.weapon.type->melee_dam);
 mvwprintw(w_terrain, 0, 0, "Smash what? (hjklyubn) ");
 wrefresh(w_terrain);
 char ch = input();
 last_action += ch;
 if (ch == KEY_ESCAPE) {
  add_msg("Never mind.");
  return;
 }
 int smashx, smashy;
 get_direction(smashx, smashy, ch);
 if (smashx != -2 && smashy != -2)
  didit = m.bash(u.posx + smashx, u.posy + smashy, smashskill, bashsound);
 else
  add_msg("Invalid direction.");
 if (didit) {
  if (extra != "")
   add_msg(extra.c_str());
  sound(u.posx, u.posy, 18, bashsound);
  u.moves -= 80;
  if (u.sklevel[sk_melee] == 0)
   u.practice(sk_melee, rng(0, 1) * rng(0, 1));
  if (u.weapon.made_of(GLASS) &&
      rng(0, u.weapon.volume() + 3) < u.weapon.volume()) {
   add_msg("Your %s shatters!", u.weapon.tname(this).c_str());
   for (int i = 0; i < u.weapon.contents.size(); i++)
    m.add_item(u.posx, u.posy, u.weapon.contents[i]);
   sound(u.posx, u.posy, 16, "");
   u.hit(this, bp_hands, 1, 0, rng(0, u.weapon.volume()));
   if (u.weapon.volume() > 20)// Hurt left arm too, if it was big
    u.hit(this, bp_hands, 0, 0, rng(0, u.weapon.volume() * .5));
   u.remove_weapon();
  }
 } else
  add_msg("There's nothing there!");
}

void game::use_item()
{
 char ch = inv("Use item:");
 if (ch == KEY_ESCAPE) {
  add_msg("Never mind.");
  return;
 }
 last_action += ch;
 itype_id tut;
 if (in_tutorial)
  tut = itype_id(u.i_at(ch).type->id);
 u.use(this, ch);

 if (in_tutorial) {
  if (tut == itm_grenade)
   tutorial_message(LESSON_ACT_GRENADE);
  else if (tut == itm_bubblewrap)
   tutorial_message(LESSON_ACT_BUBBLEWRAP);
 }
}

void game::examine()
{
 mvwprintw(w_terrain, 0, 0, "Examine where? (Direction button) ");
 wrefresh(w_terrain);
 int examx, examy;
 char ch = input();
 last_action += ch;
 if (ch == KEY_ESCAPE || ch == 'e' || ch == 'q')
  return;
 get_direction(examx, examy, ch);
 if (examx == -2 || examy == -2) {
  add_msg("Invalid direction.");
  return;
 }
 examx += u.posx;
 examy += u.posy;
 add_msg("That is a %s.", m.tername(examx, examy).c_str());
 if (m.has_flag(sealed, examx, examy)) {
  if (m.trans(examx, examy)) {
   std::string buff;
   if (m.i_at(examx, examy).size() <= 3 && m.i_at(examx, examy).size() != 0) {
    buff = "It contains ";
    for (int i = 0; i < m.i_at(examx, examy).size(); i++) {
     buff += m.i_at(examx, examy)[i].tname(this);
     if (i + 2 < m.i_at(examx, examy).size())
      buff += ", ";
     else if (i + 1 < m.i_at(examx, examy).size())
      buff += ", and ";
    }
    buff += ",";
   } else if (m.i_at(examx, examy).size() != 0)
    buff = "It contains many items,";
   buff += " but is firmly sealed.";
   add_msg(buff.c_str());
  } else {
   add_msg("There's something in there, but you can't see what it is, and the\
 %s is firmly sealed.", m.tername(examx, examy).c_str());
  }
 } else {
  if (in_tutorial && m.i_at(examx, examy).size() == 0)
   tutorial_message(LESSON_INTERACT);
  if (m.i_at(examx, examy).size() == 0 && m.has_flag(container, examx, examy) &&
      !(m.has_flag(swimmable, examx, examy) || m.ter(examx, examy) == t_toilet))
   add_msg("It is empty.");
  else
   pickup(examx, examy, 0);
 }
 if (m.has_flag(computer, examx, examy)) {
  use_computer(examx, examy);
  return;
 }
 if (m.ter(examx, examy) == t_card_reader) {
  if (u.has_amount(itm_card_id, 1) && query_yn("Swipe your ID card?")) {
   u.moves -= 100;
   for (int i = -5; i <= 5; i++) {
    for (int j = -5; j <= 5; j++) {
     if (m.ter(examx + i, examy + j) == t_door_metal_locked)
      m.ter(examx + i, examy + j) = t_floor;
    }
   }
   for (int i = 0; i < z.size(); i++) {
    if (z[i].type->id == mon_turret)
     kill_mon(i);
   }
   add_msg("You insert your ID card.");
   add_msg("The nearby doors slide into the floor.");
   u.use_up(itm_card_id, 1);
  }
  bool using_electrohack = (u.has_amount(itm_electrohack, 1) &&
                            query_yn("Use electrohack on the reader?"));
  bool using_fingerhack = (!using_electrohack && u.has_bionic(bio_fingerhack) &&
                           u.power_level > 0 &&
                           query_yn("Use fingerhack on the reader?"));
  if (using_electrohack || using_fingerhack) {
   u.moves -= 500;
   u.practice(sk_computer, 20);
   int success = rng(u.sklevel[sk_computer]/4 - 2, u.sklevel[sk_computer] * 2);
   success += rng(-3, 3);
   if (using_fingerhack)
    success++;
   if (u.int_cur < 8)
    success -= rng(0, int((8 - u.int_cur) / 2));
   else if (u.int_cur > 8)
    success += rng(0, int((u.int_cur - 8) / 2));
   if (success < 0) {
    add_msg("You cause a short circuit!");
    if (success <= -5) {
     if (using_electrohack) {
      add_msg("Your electrohack is ruined!");
      u.use_up(itm_electrohack, 1);
     } else {
      add_msg("Your power is drained!");
      u.charge_power(0 - rng(0, u.power_level));
     }
    }
    m.ter(examx, examy) = t_card_reader_broken;
   } else if (success < 6)
    add_msg("Nothing happens.");
   else {
    add_msg("You activate the panel!");
    add_msg("The nearby doors slide into the floor.");
    for (int i = -5; i <= 5; i++) {
     for (int j = -5; j <= 5; j++) {
      if (m.ter(examx + i, examy + j) == t_door_metal_locked)
       m.ter(examx + i, examy + j) = t_floor;
     }
    }
   }
  }
 } else if (m.ter(examx, examy) == t_gas_pump && query_yn("Pump gas?")) {
  item gas(itypes[itm_gasoline], turn);
  if (one_in(u.dex_cur)) {
   add_msg("You accidentally spill the gasoline.");
   m.add_item(u.posx, u.posy, gas);
  } else {
   u.moves -= 300;
   handle_liquid(gas, false, true);
  }
 } else if (m.ter(examx, examy) == t_slot_machine) {
  if (u.cash > 0 && query_yn("Insert $1?")) {
   do {
    if (one_in(5))
     popup("Three cherries... you get your coin back!");
    else if (one_in(20)) {
     popup("Three bells... you win $5!");
     u.cash += 4;	// Minus the $1 we wagered
    } else if (one_in(50)) {
     popup("Three stars... you win $20!");
     u.cash += 19;
    } else if (one_in(1000)) {
     popup("JACKPOT!  You win $500!");
     u.cash += 499;
    } else {
     popup("No win.");
     u.cash -= 1;
    }
   } while (u.cash > 0 && query_yn("Play again?"));
  }
 } else if (m.ter(examx, examy) == t_bulletin) {
// TODO: Bulletin Boards
  switch (menu("Bulletin Board", "Check jobs", "Check events",
               "Check other notices", "Post notice", "Cancel", NULL)) {
   case 1:
    break;
   case 2:
    break;
   case 3:
    break;
   case 4:
    break;
  }
 }
 if (m.tr_at(examx, examy) != tr_null &&
     u.per_cur-u.encumb(bp_eyes) >= traps[m.tr_at(examx, examy)]->visibility &&
     query_yn("There is a %s there.  Disarm?",
              traps[m.tr_at(examx, examy)]->name.c_str()))
  m.disarm_trap(this, examx, examy);
}

void game::look_around()
{
 int lx = u.posx, ly = u.posy;
 int mx, my, junk;
 char ch;
 WINDOW* w_look = newwin(13, 48, 12, SEEX * 2 + 8);
 wborder(w_look, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                 LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 mvwprintz(w_look, 1, 1, c_white, "Looking Around");
 mvwprintz(w_look, 2, 1, c_white, "Use directional keys to move the cursor");
 mvwprintz(w_look, 3, 1, c_white, "to a nearby square.");
 wrefresh(w_look);
 do {
  ch = input();
  if (!u_see(lx, ly, junk))
   mvwputch(w_terrain, ly - u.posy + SEEY, lx - u.posx + SEEX, c_black, ' ');
  draw_ter();
  get_direction(mx, my, ch);
  if (mx != -2 && my != -2) {	// Directional key pressed
   lx += mx;
   ly += my;
   if (lx < u.posx - SEEX)
    lx = u.posx - SEEX;
   if (lx > u.posx + SEEX)
    lx = u.posx + SEEX;
   if (ly < u.posy - SEEY)
    ly = u.posy - SEEY;
   if (ly > u.posy + SEEY)
    ly = u.posy + SEEY;
  }
  for (int i = 1; i < 12; i++) {
   for (int j = 1; j < 46; j++)
    mvwputch(w_look, i, j, c_white, ' ');
  }
  if (u_see(lx, ly, junk)) {
   if (m.move_cost(lx, ly) == 0)
    mvwprintw(w_look, 1, 1, "%s; Impassable", m.tername(lx, ly).c_str());
   else
    mvwprintw(w_look, 1, 1, "%s; Movement cost %d", m.tername(lx, ly).c_str(),
                                                    m.move_cost(lx, ly) * 50);
   mvwprintw(w_look, 2, 1, "%s", m.features(lx, ly).c_str());
   field tmpfield = m.field_at(lx, ly);
   if (tmpfield.type != fd_null)
    mvwprintz(w_look, 4, 1, fieldlist[tmpfield.type].color[tmpfield.density-1],
              "%s", fieldlist[tmpfield.type].name[tmpfield.density-1].c_str());
   if (m.tr_at(lx, ly) != tr_null &&
       u.per_cur - u.encumb(bp_eyes) >= traps[m.tr_at(lx, ly)]->visibility)
    mvwprintz(w_look, 5, 1, traps[m.tr_at(lx, ly)]->color, "%s",
              traps[m.tr_at(lx, ly)]->name.c_str());
   int dex = mon_at(lx, ly);
   if (dex != -1 && u_see(&(z[dex]), junk)) {
    z[mon_at(lx, ly)].draw(w_terrain, u.posx, u.posy, true);
    z[mon_at(lx, ly)].print_info(this, w_look);
    if (m.i_at(lx, ly).size() > 1)
     mvwprintw(w_look, 3, 1, "There are several items there.");
    else if (m.i_at(lx, ly).size() == 1)
     mvwprintw(w_look, 3, 1, "There is an item there.");
   } else if (npc_at(lx, ly) != -1) {
    active_npc[npc_at(lx, ly)].draw(w_terrain, u.posx, u.posy, true);
    active_npc[npc_at(lx, ly)].print_info(w_look);
    if (m.i_at(lx, ly).size() > 1)
     mvwprintw(w_look, 3, 1, "There are several items there.");
    else if (m.i_at(lx, ly).size() == 1)
     mvwprintw(w_look, 3, 1, "There is an item there.");
   } else if (m.i_at(lx, ly).size() > 0) {
    mvwprintw(w_look, 3, 1, "There is a %s there.",
              m.i_at(lx, ly)[0].tname(this).c_str());
    if (m.i_at(lx, ly).size() > 1)
     mvwprintw(w_look, 4, 1, "There are other items there as well.");
    m.drawsq(w_terrain, u, lx, ly, true, true);
   } else
    m.drawsq(w_terrain, u, lx, ly, true, true);

  } else if (lx == u.posx && ly == u.posy) {
   mvwputch_inv(w_terrain, SEEX, SEEY, u.color(), '@');
   mvwprintw(w_look, 1, 1, "You (%s)", u.name.c_str());
  } else {
   mvwputch(w_terrain, ly - u.posy + SEEY, lx - u.posx + SEEX, c_white, 'x');
   mvwprintw(w_look, 1, 1, "Unseen.");
  }
  wrefresh(w_look);
  wrefresh(w_terrain);
 } while (ch != ' ' && ch != KEY_ESCAPE && ch != ';' && ch != '\n');
}

// Pick up items at (posx, posy).
void game::pickup(int posx, int posy, int min)
{
 write_msg();
 if (u.weapon.type->id == itm_bio_claws) {
  add_msg("You cannot pick up items with your claws out!");
  return;
 }
 bool weight_is_okay = (u.weight_carried() <= u.weight_capacity() * .25);
 bool volume_is_okay = (u.volume_carried() <= u.volume_capacity() -  2);
// Picking up water?
 if (m.i_at(posx, posy).size() == 0) {
  if (m.has_flag(swimmable, posx, posy) || m.ter(posx, posy) == t_toilet) {
   item water = m.water_from(posx, posy);
   if (query_yn("Drink from your hands?")) {
    u.inv.push_back(water);
    u.eat(this, u.inv.size() - 1);
    u.moves -= 350;
   } else {
    handle_liquid(water, true, false);
    u.moves -= 100;
   }
  }
  return;
// Few item here, just get it
 } else if (m.i_at(posx, posy).size() <= min) {
  int iter = 0;
  item newit = m.i_at(posx, posy)[0];
  if (newit.made_of(LIQUID)) {
   add_msg("You can't pick up a liquid!");
   return;
  }
  while ((newit.invlet == 0 || u.has_item(newit.invlet)) && iter < 52) {
   newit.invlet = nextinv;
   iter++;
   advance_nextinv();
  }
  if (iter == 52) {
   add_msg("You're carrying too many items!");
   return;
  } else if (u.weight_carried() + newit.weight() > u.weight_capacity()) {
   add_msg("The %s is too heavy!", newit.tname(this).c_str());
   nextinv--;
  } else if (u.volume_carried() + newit.volume() > u.volume_capacity()) {
   if (u.is_armed()) {
    if (u.weapon.type->id < num_items && // Not a bionic
        query_yn("Drop your %s and pick up %s?",
                 u.weapon.tname(this).c_str(), newit.tname(this).c_str())) {
     m.i_clear(posx, posy);
     m.add_item(posx, posy, u.remove_weapon());
     u.i_add(newit);
     u.wield(this, u.inv.size() - 1);
     u.moves -= 100;
     add_msg("Wielding %c - %s", newit.invlet, newit.tname(this).c_str());
     if (in_tutorial) {
      tutorial_message(LESSON_FULL_INV);
      if (newit.is_armor())
       tutorial_message(LESSON_GOT_ARMOR);
      else if (newit.is_gun())
       tutorial_message(LESSON_GOT_GUN);
      else if (newit.is_weap())
       tutorial_message(LESSON_GOT_WEAPON);
      else if (newit.is_ammo())
       tutorial_message(LESSON_GOT_AMMO);
      else if (newit.is_tool())
       tutorial_message(LESSON_GOT_TOOL);
      else if (newit.is_food() || newit.is_food_container())
       tutorial_message(LESSON_GOT_FOOD);
     }
    } else
     nextinv--;
   } else {
    u.i_add(newit);
    u.wield(this, u.inv.size() - 1);
    m.i_clear(posx, posy);
    u.moves -= 100;
    add_msg("Wielding %c - %s", newit.invlet, newit.tname(this).c_str());
    if (in_tutorial) {
     tutorial_message(LESSON_WIELD_NO_SPACE);
     if (newit.is_armor())
      tutorial_message(LESSON_GOT_ARMOR);
     else if (newit.is_gun())
      tutorial_message(LESSON_GOT_GUN);
     else if (newit.is_ammo())
      tutorial_message(LESSON_GOT_AMMO);
     else if (newit.is_tool())
      tutorial_message(LESSON_GOT_TOOL);
     else if (newit.is_food() || newit.is_food_container())
      tutorial_message(LESSON_GOT_FOOD);
    }
   }
  } else if (!u.is_armed() &&
             (u.volume_carried() + newit.volume() > u.volume_capacity() - 2 ||
              newit.is_weap())) {
   u.weapon = newit;
   m.i_clear(posx, posy);
   u.moves -= 100;
   add_msg("Wielding %c - %s", newit.invlet, newit.tname(this).c_str());
   if (in_tutorial) {
    if (newit.is_weap())
     tutorial_message(LESSON_AUTOWIELD);
    else
     tutorial_message(LESSON_WIELD_NO_SPACE);
    if (newit.is_armor())
     tutorial_message(LESSON_GOT_ARMOR);
    else if (newit.is_gun())
      tutorial_message(LESSON_GOT_GUN);
    else if (newit.is_ammo())
     tutorial_message(LESSON_GOT_AMMO);
    else if (newit.is_tool())
     tutorial_message(LESSON_GOT_TOOL);
    else if (newit.is_food() || newit.is_food_container())
     tutorial_message(LESSON_GOT_FOOD);
   }
  } else {
   u.i_add(newit);
   m.i_clear(posx, posy);
   u.moves -= 100;
   add_msg("%c - %s", newit.invlet, newit.tname(this).c_str());
   if (in_tutorial) {
    tutorial_message(LESSON_ITEM_INTO_INV);
    if (newit.is_armor())
     tutorial_message(LESSON_GOT_ARMOR);
    else if (newit.is_gun())
      tutorial_message(LESSON_GOT_GUN);
    else if (newit.is_weap())
     tutorial_message(LESSON_GOT_WEAPON);
    else if (newit.is_ammo())
     tutorial_message(LESSON_GOT_AMMO);
    else if (newit.is_tool())
     tutorial_message(LESSON_GOT_TOOL);
    else if (newit.is_food() || newit.is_food_container())
     tutorial_message(LESSON_GOT_FOOD);
   }
  }
  if (weight_is_okay && u.weight_carried() >= u.weight_capacity() * .25)
   add_msg("You're overburdened!");
  if (volume_is_okay && u.volume_carried() > u.volume_capacity() - 2) {
   add_msg("You struggle to carry such a large volume!");
   if (in_tutorial)
    tutorial_message(LESSON_OVERLOADED);
  }
  return;
 }
// Otherwise, we have 2 or more items and should list them, etc.
 WINDOW* w_pickup = newwin(12, 48, 0, SEEX * 2 + 8);
 WINDOW* w_item_info = newwin(12, 48, 12, SEEX * 2 + 8);
 int maxitems = 9;	 // Number of items to show at one time.
 std::vector <item> here = m.i_at(posx, posy);
 bool getitem[here.size()];
 for (int i = 0; i < here.size(); i++)
  getitem[i] = false;
 char ch = ' ';
 int start = 0, cur_it, iter;
 int new_weight = u.weight_carried(), new_volume = u.volume_carried();
 bool update = true;
 mvwprintw(w_pickup, 0,  0, "PICK UP");
// Now print the two lists; those on the ground and about to be added to inv
// Continue until we hit return or space
 do {
  for (int i = 1; i < 12; i++) {
   for (int j = 0; j < 48; j++)
    mvwaddch(w_pickup, i, j, ' ');
  }
  if (ch == '<' && start > 0) {
   start -= maxitems;
   mvwprintw(w_pickup, maxitems + 2, 0, "         ");
  }
  if (ch == '>' && start + maxitems < here.size()) {
   start += maxitems;
   mvwprintw(w_pickup, maxitems + 2, 12, "            ");
  }
  if (ch >= 'a' && ch <= 'a' + here.size() - 1) {
   ch -= 'a';
   getitem[ch] = !getitem[ch];
   wclear(w_item_info);
   if (getitem[ch]) {
    mvwprintw(w_item_info, 1, 0, here[ch].info().c_str());
    wborder(w_item_info, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                         LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
    wrefresh(w_item_info);
    new_weight += here[ch].weight();
    new_volume += here[ch].volume();
    update = true;
   } else {
    wborder(w_item_info, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                         LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
    wrefresh(w_item_info);
    new_weight -= here[ch].weight();
    new_volume -= here[ch].volume();
    update = true;
   }
  }
  if (ch == ',' || ch == 'g') {
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
    mvwputch(w_pickup, 1 + (cur_it % maxitems), 0, here[cur_it].color(&u),
             char(cur_it + 'a'));
    if (getitem[cur_it])
     wprintw(w_pickup, " + ");
    else
     wprintw(w_pickup, " - ");
    wprintz(w_pickup, here[cur_it].color(&u), here[cur_it].tname(this).c_str());
    if (here[cur_it].charges > 0)
     wprintz(w_pickup, here[cur_it].color(&u), " (%d)", here[cur_it].charges);
   }
  }
  if (start > 0)
   mvwprintw(w_pickup, maxitems + 2, 0, "< Go Back");
  if (cur_it < here.size())
   mvwprintw(w_pickup, maxitems + 2, 12, "> More items");
  if (update) {		// Update weight & volume information
   update = false;
   mvwprintw(w_pickup, 0,  7, "                           ");
   mvwprintz(w_pickup, 0,  9,
             (new_weight >= u.weight_capacity() * .25 ? c_red : c_white),
             "Wgt %d", new_weight);
   wprintz(w_pickup, c_white, "/%d", int(u.weight_capacity() * .25));
   mvwprintz(w_pickup, 0, 22,
             (new_volume > u.volume_capacity() - 2 ? c_red : c_white),
             "Vol %d", new_volume);
   wprintz(w_pickup, c_white, "/%d", u.volume_capacity() - 2);
  }
  wrefresh(w_pickup);
  ch = getch();
 } while (ch != ' ' && ch != '\n' && ch != KEY_ESCAPE);
 if (ch == KEY_ESCAPE) {
  werase(w_pickup);
  wrefresh(w_pickup);
  delwin(w_pickup);
  return;
 }
// At this point we've selected our items, now we add them to our inventory
 int curmit = 0;
 bool got_water = false;	// Did we try to pick up water?
 for (int i = 0; i < here.size(); i++) {
  iter = 0;
// This while loop guarantees the inventory letter won't be a repeat. If it
// tries all 52 letters, it fails and we don't pick it up.
  if (getitem[i] && here[i].made_of(LIQUID))
   got_water = true;
  else if (getitem[i]) {
   while ((here[i].invlet == 0 || u.has_item(here[i].invlet)) && iter < 52) {
    here[i].invlet = nextinv;
    advance_nextinv();
    iter++;
   }
   if (iter == 52) {
    add_msg("You're carrying too many items!");
    werase(w_pickup);
    wrefresh(w_pickup);
    delwin(w_pickup);
    return;
   } else if (u.weight_carried() + here[i].weight() > u.weight_capacity()) {
    add_msg("The %s is too heavy!", here[i].tname(this).c_str());
    nextinv--;
   } else if (u.volume_carried() + here[i].volume() > u.volume_capacity()) {
    if (u.is_armed()) {
     if (u.weapon.type->id < num_items && // Not a bionic
         query_yn("Drop your %s and pick up %s?",
                  u.weapon.tname(this).c_str(), here[i].tname(this).c_str())) {
      m.add_item(posx, posy, u.remove_weapon());
      u.i_add(here[i]);
      u.wield(this, u.inv.size() - 1);
      u.moves -= 100;
      m.i_rem(posx, posy, curmit);
      curmit--;
      if (in_tutorial) {
       tutorial_message(LESSON_WIELD_NO_SPACE);
       if (here[i].is_armor())
        tutorial_message(LESSON_GOT_ARMOR);
       else if (here[i].is_gun())
        tutorial_message(LESSON_GOT_GUN);
       else if (here[i].is_weap())
        tutorial_message(LESSON_GOT_WEAPON);
       else if (here[i].is_ammo())
        tutorial_message(LESSON_GOT_AMMO);
       else if (here[i].is_tool())
        tutorial_message(LESSON_GOT_TOOL);
       else if (here[i].is_food() || here[i].is_food_container())
        tutorial_message(LESSON_GOT_FOOD);
      }
     } else
      nextinv--;
    } else {
     u.i_add(here[i]);
     u.wield(this, u.inv.size() - 1);
     m.i_rem(posx, posy, curmit);
     curmit--;
     u.moves -= 100;
     if (in_tutorial) {
      tutorial_message(LESSON_WIELD_NO_SPACE);
      if (here[i].is_armor())
       tutorial_message(LESSON_GOT_ARMOR);
      else if (here[i].is_gun())
       tutorial_message(LESSON_GOT_GUN);
      else if (here[i].is_weap())
       tutorial_message(LESSON_GOT_WEAPON);
      else if (here[i].is_ammo())
       tutorial_message(LESSON_GOT_AMMO);
      else if (here[i].is_tool())
       tutorial_message(LESSON_GOT_TOOL);
      else if (here[i].is_food() || here[i].is_food_container())
       tutorial_message(LESSON_GOT_FOOD);
     }
    }
   } else if (!u.is_armed() &&
            (u.volume_carried() + here[i].volume() > u.volume_capacity() - 2 ||
              here[i].is_weap())) {
    u.weapon = here[i];
    m.i_rem(posx, posy, curmit);
    u.moves -= 100;
    curmit--;
    if (in_tutorial) {
     if (here[i].is_weap())
      tutorial_message(LESSON_AUTOWIELD);
     else
      tutorial_message(LESSON_WIELD_NO_SPACE);
     if (here[i].is_armor())
      tutorial_message(LESSON_GOT_ARMOR);
     else if (here[i].is_gun())
      tutorial_message(LESSON_GOT_GUN);
     else if (here[i].is_weap())
      tutorial_message(LESSON_GOT_WEAPON);
     else if (here[i].is_ammo())
      tutorial_message(LESSON_GOT_AMMO);
     else if (here[i].is_tool())
      tutorial_message(LESSON_GOT_TOOL);
     else if (here[i].is_food() || here[i].is_food_container())
      tutorial_message(LESSON_GOT_FOOD);
    }
   } else {
    u.i_add(here[i]);
    m.i_rem(posx, posy, curmit);
    u.moves -= 100;
    curmit--;
    if (in_tutorial) {
     tutorial_message(LESSON_ITEM_INTO_INV);
     if (here[i].is_armor())
      tutorial_message(LESSON_GOT_ARMOR);
     else if (here[i].is_gun())
      tutorial_message(LESSON_GOT_GUN);
     else if (here[i].is_weap())
      tutorial_message(LESSON_GOT_WEAPON);
     else if (here[i].is_ammo())
      tutorial_message(LESSON_GOT_AMMO);
     else if (here[i].is_tool())
      tutorial_message(LESSON_GOT_TOOL);
     else if (here[i].is_food() || here[i].is_food_container())
      tutorial_message(LESSON_GOT_FOOD);
    }
   }
  }
  curmit++;
 }
 if (got_water)
  add_msg("You can't pick up a liquid!");
 if (weight_is_okay && u.weight_carried() >= u.weight_capacity() * .25)
  add_msg("You're overburdened!");
 if (volume_is_okay && u.volume_carried() > u.volume_capacity() - 2) {
  add_msg("You struggle to carry such a large volume!");
  if (in_tutorial)
   tutorial_message(LESSON_OVERLOADED);
 }
 werase(w_pickup);
 wrefresh(w_pickup);
 delwin(w_pickup);
}

// Handle_liquid returns false if we didn't handle all the liquid.
bool game::handle_liquid(item &liquid, bool from_ground, bool infinite)
{
 if (!liquid.made_of(LIQUID)) {
  debugmsg("Tried to handle_liquid a non-liquid!");
  return false;
 }
 if (!from_ground &&
     query_yn("Pour %s on the ground?", liquid.tname(this).c_str())) {
  m.add_item(u.posx, u.posy, liquid);
  return true;
 } else {
  std::stringstream text;
  text << "Container for " << liquid.tname(this);
  char ch = inv(text.str().c_str());
  if (!u.has_item(ch))
   return false;
  item *cont = &(u.i_at(ch));
  ammotype type = cont->ammo();
  if (cont->type->id == itm_null) {
   add_msg("Never mind.");
   return false;
  } else if (cont->is_tool() &&
             (dynamic_cast<it_tool*>(cont->type))->max_charges > 0 &&
             (dynamic_cast<it_tool*>(cont->type))->ammo == type &&
             (cont->charges == 0 || cont->curammo->id == liquid.type->id)) {
   add_msg("You pour %s into your %s.", ammo_name(type).c_str(),
                                        cont->tname(this).c_str());
   cont->curammo = dynamic_cast<it_ammo*>(liquid.type);
   int max_charges = (dynamic_cast<it_tool*>(cont->type))->max_charges;
   if (infinite)
    cont->charges = max_charges;
   else {
    cont->charges += liquid.charges;
    if (cont->charges > max_charges) {
     int extra = 0 - cont->charges;
     cont->charges = max_charges;
     liquid.charges = extra;
     add_msg("There's some left over!");
     return false;
    }
   }
  } else if (!cont->is_container()) {
   add_msg("That %s won't hold %s.", cont->tname(this).c_str(),
                                     liquid.tname(this).c_str());
   return false;
  } else if (!cont->contents.empty()) {
   add_msg("Your %s is not empty.", cont->tname(this).c_str());
   return false;
  } else {
   it_container* container = dynamic_cast<it_container*>(cont->type);
   if (!container->flags & mfb(con_wtight)) {
    add_msg("That %s isn't water-tight.", cont->tname(this).c_str());
    return false;
   } else if (!container->flags & mfb(con_seals)) {
    add_msg("You can't seal that %s!", cont->tname(this).c_str());
    return false;
   }
   int default_charges = 1;
   if (liquid.is_food()) {
    it_comest* comest = dynamic_cast<it_comest*>(liquid.type);
    default_charges = comest->charges;
   } else if (liquid.is_ammo()) {
    it_ammo* ammo = dynamic_cast<it_ammo*>(liquid.type);
    default_charges = ammo->count;
   }
   if (liquid.charges * default_charges > container->contains) {
    add_msg("You fill your %s with some of the %s.", cont->tname(this).c_str(),
                                                    liquid.tname(this).c_str());
    u.inv_sorted = false;
    int oldcharges = liquid.charges - container->contains * default_charges;
    liquid.charges = container->contains;
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


void game::drop()
{
 char ch = inv("Drop item:");
 if (ch == KEY_ESCAPE) {
  add_msg("Never mind.");
  return;
 }
 if (ch == u.weapon.invlet && u.weapon.type->id > num_items) {
  add_msg("You cannot drop your %s.", u.weapon.tname(this).c_str());
  return;
 }
 item tmp = u.i_rem(ch);
 if (tmp.type->name == "none") {
  add_msg("You do not have that item.");
  return;
 }
 m.add_item(u.posx, u.posy, tmp);
 add_msg("You drop your %s.", tmp.tname(this).c_str());
}

// Display current inventory.
char game::inv(std::string title)
{
 if (title == "")
  title = "Inventory:";
 WINDOW* w_inv = newwin(25, 80, 0, 0);
 const int maxitems = 20;	// Number of items to show at one time.
 char ch = '.';
 int start = 0, cur_it;
 //if (!u.inv_sorted)
 u.sort_inv();
 int first_gun   = -1,
     first_ammo  = -1,
     first_weap  = -1,
     first_armor = -1,
     first_food  = -1,
     first_tool  = -1,
     first_book  = -1,
     first_other = -1;
 for (int i = 0; i < u.inv.size(); i++) {
       if (first_gun == -1 && u.inv[i].is_gun())
   first_gun = i;
  else if (first_ammo == -1 && u.inv[i].is_ammo())
   first_ammo = i;
  else if (first_armor == -1 && u.inv[i].is_armor())
   first_armor = i;
  else if (first_food == -1 &&
           (u.inv[i].is_food() || u.inv[i].is_food_container()))
   first_food = i;
  else if (first_tool == -1 && (u.inv[i].is_tool() || u.inv[i].is_gunmod()))
   first_tool = i;
  else if (first_book == -1 && u.inv[i].is_book())
   first_book = i;
  else if (first_weap == -1 && u.inv[i].is_weap())
   first_weap = i;
  else if (first_other == -1   && !u.inv[i].is_food()  && !u.inv[i].is_ammo() &&
           !u.inv[i].is_gun()  && !u.inv[i].is_armor() && !u.inv[i].is_book() &&
           !u.inv[i].is_tool() && !u.inv[i].is_weap()  &&
           !u.inv[i].is_gunmod() && !u.inv[i].is_food_container())
   first_other = i;
 }
// Print our header
 mvwprintw(w_inv, 0, 0, title.c_str());
 mvwprintw(w_inv, 0, 40, "Weight: ");
 if (u.weight_carried() >= u.weight_capacity() * .25)
  wprintz(w_inv, c_red, "%d", u.weight_carried());
 else
  wprintz(w_inv, c_ltgray, "%d", u.weight_carried());
 wprintz(w_inv, c_ltgray, "/%d/%d", int(u.weight_capacity() * .25),
                                    u.weight_capacity());
 mvwprintw(w_inv, 0, 60, "Volume: ");
 if (u.volume_carried() > u.volume_capacity() - 2)
  wprintz(w_inv, c_red, "%d", u.volume_carried());
 else
  wprintz(w_inv, c_ltgray, "%d", u.volume_carried());
 wprintw(w_inv, "/%d", u.volume_capacity() - 2);
// Print our weapon
 mvwprintz(w_inv, 2, 40, c_magenta, "WEAPON:");
 mvwprintw(w_inv, 3, 42, u.weapname().c_str());
 if (u.is_armed())
  mvwputch(w_inv, 3, 40, c_white, u.weapon.invlet);
// Print worn items
 if (u.worn.size() > 0)
  mvwprintz(w_inv, 5, 40, c_magenta, "ITEMS WORN:");
 for (int i = 0; i < u.worn.size(); i++) {
  mvwputch(w_inv, 6 + i, 40, c_white, u.worn[i].invlet);
  mvwprintw(w_inv, 6 + i, 41, " %s", u.worn[i].tname(this).c_str());
 }


 do {
  if (ch == '<' && start > 0) {
   for (int i = 1; i < 25; i++)
    mvwprintz(w_inv, i, 0, c_black, "                                        ");
   start -= maxitems;
   if (start < 0)
    start = 0;
   mvwprintw(w_inv, maxitems + 2, 0, "         ");
  }
  if (ch == '>' && cur_it < u.inv.size()) {
   start = cur_it;
   mvwprintw(w_inv, maxitems + 2, 12, "            ");
   for (int i = 1; i < 25; i++)
    mvwprintz(w_inv, i, 0, c_black, "                                        ");
  }
  int cur_line = 2;
  for (cur_it = start; cur_it < start + maxitems && cur_line < 23; cur_it++) {
// Clear the current line;
   mvwprintw(w_inv, cur_line, 0, "                                    ");
// Print category header
   if (cur_it == first_gun) {
    mvwprintz(w_inv, cur_line, 0, c_magenta, "FIREARMS:");
    cur_line++;
   } else if (cur_it == first_ammo) {
    mvwprintz(w_inv, cur_line, 0, c_magenta, "AMMUNITION:");
    cur_line++;
   } else if (cur_it == first_weap) {
    mvwprintz(w_inv, cur_line, 0, c_magenta, "WEAPONS:");
    cur_line++;
   } else if (cur_it == first_armor) {
    mvwprintz(w_inv, cur_line, 0, c_magenta, "CLOTHING:");
    cur_line++;
   } else if (cur_it == first_food) {
    mvwprintz(w_inv, cur_line, 0, c_magenta, "COMESTIBLES:");
    cur_line++;
   } else if (cur_it == first_tool) {
    mvwprintz(w_inv, cur_line, 0, c_magenta, "TOOLS:");
    cur_line++;
   } else if (cur_it == first_book) {
    mvwprintz(w_inv, cur_line, 0, c_magenta, "BOOKS:");
    cur_line++;
   } else if (cur_it == first_other) {
    mvwprintz(w_inv, cur_line, 0, c_magenta, "OTHER:");
    cur_line++;
   }
   if (cur_it < u.inv.size()) {
    mvwputch (w_inv, cur_line, 0, c_white, u.inv[cur_it].invlet);
    mvwprintw(w_inv, cur_line, 1, " %s", u.inv[cur_it].tname(this).c_str());
    if (u.inv[cur_it].charges > 0)
     wprintw(w_inv, " (%d)", u.inv[cur_it].charges);
   }
   cur_line++;
  }
  if (start > 0)
   mvwprintw(w_inv, maxitems + 4, 0, "< Go Back");
  if (cur_it < u.inv.size())
   mvwprintw(w_inv, maxitems + 4, 12, "> More items");
  wrefresh(w_inv);
  ch = getch();
 } while (ch == '<' || ch == '>');
 werase(w_inv);
 delwin(w_inv);
 erase();
 refresh_all();
 return ch;
}

void game::plthrow()
{
 char ch = inv("Throw item:");
 int range = u.throw_range(u.lookup_item(ch));
 if (range < 0) {
  add_msg("You don't have that item.");
  return;
 } else if (range == 0) {
  add_msg("That is too heavy to throw.");
  return;
 }
 item thrown = u.i_at(ch);

 int x = u.posx, y = u.posy;
 int x0 = x - range;
 int y0 = y - range;
 int x1 = x + range;
 int y1 = y + range;
 int junk;

 for (int j = u.posx - SEEX; j <= u.posx + SEEX; j++) {
  for (int k = u.posy - SEEY; k <= u.posy + SEEY; k++) {
   if (u_see(j, k, junk)) {
    if (k >= y0 && k <= y1 && j >= x0 && j <= x1)
     m.drawsq(w_terrain, u, j, k, false, true);
    else
     mvwputch(w_terrain, k + SEEY - u.posy, j + SEEX - u.posx, c_dkgray, '#');
   }
  }
 }

 std::vector <monster> mon_targets;
 std::vector <int> targetindices;
 int passtarget = -1;
 for (int i = 0; i < z.size(); i++) {
  if (u_see(&(z[i]), junk) && z[i].posx >= x0 && z[i].posx <= x1 &&
                              z[i].posy >= y0 && z[i].posy <= y1) {
   mon_targets.push_back(z[i]);
   targetindices.push_back(i);
   if (i == last_target)
    passtarget = mon_targets.size() - 1;
   z[i].draw(w_terrain, u.posx, u.posy, true);
  }
 }

 // target() sets x and y, or returns false if we canceled (by pressing Esc)
 std::vector <point> trajectory = target(x, y, x0, y0, x1, y1, mon_targets,
                                         passtarget, &thrown);
 if (trajectory.size() == 0)
  return;
 if (passtarget != -1)
  last_target = targetindices[passtarget];

 u.i_rem(ch);
 u.moves -= 125;
 u.practice(sk_throw, 10);

 throw_item(u, x, y, thrown, trajectory);
}

void game::plfire(bool burst)
{
 if (!u.weapon.is_gun())
  return;
 if (u.weapon.charges == 0) {
  add_msg("You need to reload!");
  return;
 }

 int junk;
 int range = u.weapon.curammo->range;
 int x = u.posx, y = u.posy;
 int x0 = x - range;
 int y0 = y - range;
 int x1 = x + range;
 int y1 = y + range;
 for (int j = x - SEEX; j <= x + SEEX; j++) {
  for (int k = y - SEEY; k <= y + SEEY; k++) {
   if (u_see(j, k, junk)) {
    if (k >= y0 && k <= y1 && j >= x0 && j <= x1)
     m.drawsq(w_terrain, u, j, k, false, true);
    else
     mvwputch(w_terrain, k + SEEY - y, j + SEEX - x, c_dkgray, '#');
   }
  }
 }
// Populate a list of targets with the zombies in range and visible
 std::vector <monster> mon_targets;
 std::vector <int> targetindices;
 int passtarget = -1;
 for (int i = 0; i < z.size(); i++) {
  if (z[i].posx >= x0 && z[i].posx <= x1 &&
      z[i].posy >= y0 && z[i].posy <= y1 &&
      z[i].friendly == 0 && u_see(&(z[i]), junk)) {
   mon_targets.push_back(z[i]);
   targetindices.push_back(i);
   if (i == last_target)
    passtarget = mon_targets.size() - 1;
   z[i].draw(w_terrain, u.posx, u.posy, true);
  }
 }

 // target() sets x and y, and returns an empty vector if we canceled (Esc)
 std::vector <point> trajectory = target(x, y, x0, y0, x1, y1, mon_targets,
                                         passtarget, &u.weapon);
 if (trajectory.size() == 0)
  return;
 if (passtarget != -1)	// We picked a real live target
  last_target = targetindices[passtarget]; // Make it our default for next time

// Train up our skill
 it_gun* firing = dynamic_cast<it_gun*>(u.weapon.type);
 int num_shots = 1;
 if (burst)
  num_shots = u.weapon.burst_size();
 if (num_shots > u.weapon.charges)
  num_shots = u.weapon.charges;
 if (u.sklevel[firing->skill_used] == 0 ||
     (firing->ammo != AT_BB && firing->ammo != AT_NAIL))
  u.practice(firing->skill_used, 4 + (num_shots / 2));
 if (u.sklevel[sk_gun] == 0 ||
     (firing->ammo != AT_BB && firing->ammo != AT_NAIL))
  u.practice(sk_gun, 5);

 fire(u, x, y, trajectory, burst);
 if (in_tutorial && u.recoil >= 5)
  tutorial_message(LESSON_RECOIL);
}

void game::butcher()
{
 std::vector<int> corpses;
 for (int i = 0; i < m.i_at(u.posx, u.posy).size(); i++) {
  if (m.i_at(u.posx, u.posy)[i].type->id == itm_corpse)
   corpses.push_back(i);
 }
 if (corpses.size() == 0) {
  add_msg("There are no corpses here to butcher.");
  return;
 }
 int factor = u.butcher_factor();
 if (factor == 999) {
  add_msg("You don't have a sharp item to butcher with.");
  return;
 }
// We do it backwards to prevent the deletion of a corpse from corrupting our
// vector of indices.
 for (int i = corpses.size() - 1; i >= 0; i--) {
  mtype *corpse = m.i_at(u.posx, u.posy)[corpses[i]].corpse;
  if (query_yn("Butcher the %s corpse?", corpse->name.c_str())) {
   int time_to_cut;
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
   u.activity = player_activity(ACT_BUTCHER, time_to_cut, corpses[i]);
   u.moves = 0;
   return;
  }
 }
}

void game::complete_butcher(int index)
{
 mtype* corpse = m.i_at(u.posx, u.posy)[index].corpse;
 int age = m.i_at(u.posx, u.posy)[index].bday;
 m.i_rem(u.posx, u.posy, index);
 int factor = u.butcher_factor();
 int pieces, pelts;
 double skill_shift = 0.;
 switch (corpse->size) {
  case MS_TINY:   pieces =  1; pelts =  1; break;
  case MS_SMALL:  pieces =  2; pelts =  3; break;
  case MS_MEDIUM: pieces =  4; pelts =  6; break;
  case MS_LARGE:  pieces =  8; pelts = 10; break;
  case MS_HUGE:   pieces = 16; pelts = 18; break;
 }
 if (u.sklevel[sk_butcher] < 3)
  skill_shift -= rng(0, 8 - u.sklevel[sk_butcher]);
 else
  skill_shift += rng(0, u.sklevel[sk_butcher]);
 if (u.dex_cur < 8)
  skill_shift -= rng(0, 8 - u.dex_cur) / 4;
 else
  skill_shift += rng(0, u.dex_cur - 8) / 4;
 if (u.str_cur < 4)
  skill_shift -= rng(0, 5 * (4 - u.str_cur)) / 4;
 if (factor > 0)
  skill_shift -= rng(0, factor / 5);

 int practice = 4 + pieces;
 if (practice > 20)
  practice = 20;
 u.practice(sk_butcher, practice);

 pieces += int(skill_shift);
 if (skill_shift < 5)	// Lose some pelts
  pelts += (skill_shift - 5);

 if ((corpse->flags & mfb(MF_FUR) || corpse->flags & mfb(MF_LEATHER)) &&
     pelts > 0) {
  add_msg("You manage to skin the %s!", corpse->name.c_str());
  for (int i = 0; i < pelts; i++) {
   itype* pelt;
   if (corpse->flags & mfb(MF_FUR) && corpse->flags & mfb(MF_LEATHER)) {
    if (one_in(2))
     pelt = itypes[itm_fur];
    else
     pelt = itypes[itm_leather];
   } else if (corpse->flags & mfb(MF_FUR))
    pelt = itypes[itm_fur];
   else
    pelt = itypes[itm_leather];
   m.add_item(u.posx, u.posy, pelt, age);
  }
 }
 if (pieces <= 0)
  add_msg("Your clumsy butchering destroys the meat!");
 else {
  itype* meat;
  if (corpse->flags & mfb(MF_POISON)) {
    if (corpse->mat == FLESH)
     meat = itypes[itm_meat_tainted];
    else
     meat = itypes[itm_veggy_tainted];
  } else {
   if (corpse->mat == FLESH)
    meat = itypes[itm_meat];
   else
    meat = itypes[itm_veggy];
  }
  for (int i = 0; i < pieces; i++)
   m.add_item(u.posx, u.posy, meat, age);
  add_msg("You butcher the corpse.");
 }
}

void game::eat()
{
 u.moves -= 250;	// TODO: Set this to a variable?
 char ch = inv("Consume item:");
 if (ch == KEY_ESCAPE) {
  add_msg("Never mind.");
  return;
 }
 if (in_tutorial) {
  if (u.i_at(ch).type->id == itm_codeine)
   tutorial_message(LESSON_TOOK_PAINKILLER);
  else if (u.i_at(ch).type->id == itm_cig)
   tutorial_message(LESSON_TOOK_CIG);
  else if (u.i_at(ch).type->id == itm_bottle_plastic)
   tutorial_message(LESSON_DRANK_WATER);
 }
 if (!u.has_item(ch)) {
  add_msg("You don't have item '%c'!", ch);
  u.moves += 250;
  return;
 }
 if (!u.eat(this, u.lookup_item(ch))) {
  add_msg("You can't eat that!");
  u.moves += 250;
 } else if (u.has_trait(PF_GOURMAND))
  u.moves += 150;
}

void game::wear()
{
 char ch = inv("Wear item:");
 if (ch == KEY_ESCAPE) {
  add_msg("Never mind.");
  return;
 }
 if (u.wear(this, ch)) {
  u.moves -= 350;	// TODO: Make this variable
  if (in_tutorial) {
   it_armor* armor = dynamic_cast<it_armor*>(u.worn[u.worn.size() - 1].type);
   if (armor->dmg_resist >= 2 || armor->cut_resist >= 4)
    tutorial_message(LESSON_WORE_ARMOR);
   else if (armor->storage >= 20)
    tutorial_message(LESSON_WORE_STORAGE);
   else if (armor->env_resist >= 2)
    tutorial_message(LESSON_WORE_MASK);
  }
 }
}

void game::takeoff()
{
 if (u.takeoff(this, inv("Take off item:")))
  u.moves -= 250; // TODO: Make this variable
 else
  add_msg("Invalid selection.");
}

void game::reload()
{
 if (u.weapon.is_gun()) {
  if (u.weapon.charges == u.weapon.clip_size()) {
   add_msg("Your %s is fully loaded!", u.weapon.tname(this).c_str());
   return;
  }
  int index = u.weapon.pick_reload_ammo(u, true);
  if (index == -1) {
   add_msg("Out of ammo!");
   return;
  }
  u.activity = player_activity(ACT_RELOAD, u.weapon.reload_time(u), index);
  u.moves = 0;
 } else if (u.weapon.is_tool()) {
  it_tool* tool = dynamic_cast<it_tool*>(u.weapon.type);
  if (tool->ammo == AT_NULL) {
   add_msg("You can't reload a %s!", u.weapon.tname(this).c_str());
   return;
  }
  int index = u.weapon.pick_reload_ammo(u, true);
  if (index == -1) {
// Reload failed
   add_msg("Out of %s!", ammo_name(tool->ammo).c_str());
   return;
  }
  u.activity = player_activity(ACT_RELOAD, u.weapon.reload_time(u), index);
  u.moves = 0;
 } else if (!u.is_armed())
  add_msg("You're not wielding anything.");
 else
  add_msg("You can't reload a %s!", u.weapon.tname(this).c_str());
 refresh_all();
}

void game::unload()
{
 if (!u.weapon.is_gun() && u.weapon.contents.size() == 0 &&
     (!u.weapon.is_tool() || u.weapon.ammo() == AT_NULL)){
  add_msg("You can't unload a %s!", u.weapon.tname(this).c_str());
  return;
 } else if (u.weapon.is_container() || u.weapon.charges == 0) {
  if (u.weapon.contents.size() == 0) {
   if (u.weapon.is_gun())
    add_msg("Your %s isn't loaded, and is not modified.",
            u.weapon.tname(this).c_str());
   else
    add_msg("Your %s isn't charged." , u.weapon.tname(this).c_str());
   return;
  }
  u.moves -= 40 * u.weapon.contents.size();
  std::vector<item> new_contents;	// In case we put stuff back
  while (u.weapon.contents.size() > 0) {
   item content = u.weapon.contents[0];
   int iter = 0;
   while ((content.invlet == 0 || u.has_item(content.invlet)) && iter < 52) {
    content.invlet = nextinv;
    advance_nextinv();
    iter++;
   }
   if (content.made_of(LIQUID)) {
    if (!handle_liquid(content, false, false))
     new_contents.push_back(content);// Put it back in (we canceled)
   } else {
    if (u.volume_carried() + content.volume() <= u.volume_capacity() &&
        u.weight_carried() + content.weight() <= u.weight_capacity() &&
        iter < 52) {
     add_msg("You put the %s in your inventory.", content.tname(this).c_str());
     u.i_add(content);
    } else {
     add_msg("You drop the %s on the ground.", content.tname(this).c_str());
     m.add_item(u.posx, u.posy, content);
    }
   }
   u.weapon.contents.erase(u.weapon.contents.begin());
  }
  u.weapon.contents = new_contents;
  return;
 }
 u.moves -= int(u.weapon.reload_time(u) / 2);
 it_ammo* tmpammo;
 if (u.weapon.is_gun()) {	// Gun ammo is combined with existing items
  for (int i = 0; i < u.inv.size() && u.weapon.charges > 0; i++) {
   if (u.inv[i].is_ammo()) {
    tmpammo = dynamic_cast<it_ammo*>(u.inv[i].type);
    if (tmpammo->id == u.weapon.curammo->id &&
        u.inv[i].charges < tmpammo->count) {
     u.weapon.charges -= (tmpammo->count - u.inv[i].charges);
     u.inv[i].charges = tmpammo->count;
     if (u.weapon.charges < 0) {
      u.inv[i].charges += u.weapon.charges;
      u.weapon.charges = 0;
     }
    }
   }
  }
 }
 item newam;
 int iter;
 while (u.weapon.charges > 0) {
  if (u.weapon.curammo != NULL)
   newam = item(u.weapon.curammo, turn);
  else
   newam = item(itypes[default_ammo(u.weapon.ammo())], turn);
  iter = 0;
  while ((newam.invlet == 0 || u.has_item(newam.invlet)) && iter < 52) {
   newam.invlet = nextinv;
   advance_nextinv();
   iter++;
  }
  if (newam.made_of(LIQUID))
   newam.charges = u.weapon.charges;
  u.weapon.charges -= newam.charges;
  if (u.weapon.charges < 0) {
   newam.charges += u.weapon.charges;
   u.weapon.charges = 0;
  }
  if (u.weight_carried() + newam.weight() < u.weight_capacity() &&
      u.volume_carried() + newam.volume() < u.volume_capacity() && iter < 52) {
   if (newam.made_of(LIQUID)) {
    if (!handle_liquid(newam, false, false))
     u.weapon.charges += newam.charges;	// Put it back in
   } else
    u.i_add(newam);
  } else
   m.add_item(u.posx, u.posy, newam);
 }
 u.weapon.curammo = NULL;
}

void game::wield()
{
 if (u.weapon.type->id > num_items) {
  add_msg("You cannot unwield your %s.", u.weapon.tname(this).c_str());
  return;
 }
 char ch = inv("Wield item:");
 if (ch == '-')
  u.wield(this, -3);
 else
  u.wield(this, u.lookup_item(ch));
 if (in_tutorial && u.weapon.is_gun())
  tutorial_message(LESSON_GUN_LOAD);
}

void game::read()
{
 char ch = inv("Read:");
 u.read(this, ch);
}

void game::chat()
{
 if (active_npc.size() == 0) {
  add_msg("You talk to yourself for a moment.");
  return;
 }
 std::vector<npc*> available;
 int junk;
 for (int i = 0; i < active_npc.size(); i++) {
  if (u_see(active_npc[i].posx, active_npc[i].posy, junk) &&
      trig_dist(u.posx, u.posy, active_npc[i].posx, active_npc[i].posy) <= 12)
   available.push_back(&active_npc[i]);
 }
 if (available.size() == 1)
  available[0]->talk_to_u(this);
 else {
  WINDOW *w = newwin(available.size() + 3, 40, 10, 20);
  wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
             LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
  for (int i = 0; i < available.size(); i++)
   mvwprintz(w, i + 1, 1, c_white, "%d: %s", i + 1, available[i]->name.c_str());
  mvwprintz(w, available.size() + 1, 1, c_white, "%d: Cancel",
            available.size() + 1);
  wrefresh(w);
  char ch;
  do {
   ch = getch();
  } while (ch < '1' || ch > '1' + available.size());
  ch -= '1';
  if (ch == available.size())
   return;
  delwin(w);
  available[ch]->talk_to_u(this);
 }
 u.moves -= 100;
}

void game::plmove(int x, int y)
{
 if (run_mode == 2) { // Monsters around and we don't wanna run
  add_msg("Monster spotted--run mode is on! (Press '!' to turn it off.)");
  return;
 }
 x += u.posx;
 y += u.posy;
// Check if our movement is actually an attack on a monster
 int mondex = mon_at(x, y);
 bool displace = false;	// Are we displacing a monster?
 if (mondex != -1) {
  if (z[mondex].friendly == 0) {
   int udam = u.hit_mon(this, &z[mondex]);
   if (z[mondex].hurt(udam))
    kill_mon(mondex);
/*
   else if (udam > 0)	// Stun them
    z[mondex].moves -= udam + int((udam / z[mondex].hp) * z[mondex].speed);
*/
   return;
  } else
   displace = true;
 }
// If not a monster, maybe there's an NPC there
 int npcdex = npc_at(x, y);
 if (npcdex != -1) {
  if (!active_npc[npcdex].is_enemy() &&
      !query_yn("Really attack %s?", active_npc[npcdex].name.c_str()))
   return;	// Cancel the attack
  body_part bphit;
  int hitdam = 0, hitcut = 0;
  bool success = u.hit_player(active_npc[npcdex], bphit, hitdam, hitcut);
  if (u.recoil <= 30)
   u.recoil += 6;
  u.moves -= (80 + u.weapon.volume() * 2 + u.weapon.weight() +
              u.encumb(bp_torso));
  if (!success) {
   int stumble_pen = u.weapon.volume() + int(u.weapon.weight() / 2);
   if (u.has_trait(PF_DEFT))
    stumble_pen = int(stumble_pen * .4) - 10;
   if (stumble_pen < 0)
    stumble_pen = 0;
   if (stumble_pen > 0 && (one_in(16 - u.str_cur) || one_in(22 - u.dex_cur)))
    stumble_pen = rng(0, stumble_pen);
   if (stumble_pen >= 30)
    add_msg("You miss and stumble with the momentum.");
   else if (stumble_pen >= 10)
    add_msg("You swing wildly and miss.");
   else
    add_msg("You miss.");
   u.moves -= stumble_pen;
  } else {	// We hit!
   int side = rng(0, 1);
   add_msg("You hit %s's %s.", active_npc[npcdex].name.c_str(),
           body_part_name(bphit, side).c_str());
   if (u.has_bionic(bio_shock) && u.power_level >= 2 && one_in(3) &&
       (!u.is_armed() || u.weapon.type->id > num_items)) {
    add_msg("You shock %s!", active_npc[npcdex].name.c_str());
    int shock = rng(2, 5);
    hitdam += shock;
    active_npc[npcdex].moves -= shock * 300;
    u.power_level -= 2;
   }
   if (u.has_bionic(bio_heat_absorb) && u.power_level >= 1 && !u.is_armed()) {
    u.power_level--;
    if (one_in(2)) {
     add_msg("You drain %s's body heat!", active_npc[npcdex].name.c_str());
     u.charge_power(rng(3, 5));
     hitdam += rng(2, 6);
    } else {
     add_msg("You attempt to drain body heat, but fail.");
    }
   }
   if (u.has_trait(PF_FANGS) && one_in(20-u.dex_cur)) {
    add_msg("You sink your fangs into %s!", active_npc[npcdex].name.c_str());
    hitcut += 18;
   }
   sound(u.posx, u.posy, 6, "");
   if (u.weapon.made_of(GLASS) &&
       rng(0, u.weapon.volume() + 8) < u.weapon.volume() + u.str_cur) {
// Glass weapon shattered
    add_msg("Your %s shatters!", u.weapon.tname(this).c_str());
    for (int i = 0; i < u.weapon.contents.size(); i++)
     m.add_item(x, y, u.weapon.contents[i]);
    sound(u.posx, u.posy, 16, "");
    u.hit(this, bp_hands, 1, 0, rng(0, u.weapon.volume() * 2));
    if (u.weapon.is_two_handed(&u))// Hurt left arm too, if it was big
     u.hit(this, bp_hands, 0, 0, rng(0, u.weapon.volume()));
    hitcut += rng(0, int(u.weapon.volume() * 1.5));	// Hurt the monster
    u.remove_weapon();
   }
   active_npc[npcdex].hit(this, bphit, side, hitdam, hitcut);
   if (active_npc[npcdex].hp_cur[hp_head]  <= 0 ||
       active_npc[npcdex].hp_cur[hp_torso] <= 0   ) {
    active_npc[npcdex].die(this, true);
    active_npc.erase(active_npc.begin() + npcdex);
   }
  }
  return;
 }

// Otherwise, actual movement, zomg
 if (u.has_disease(DI_IN_PIT)) {
  if (rng(0, 40) > u.str_cur + int(u.dex_cur / 2)) {
   add_msg("You try to escape the pit, but slip back in.");
   return;
  } else {
   add_msg("You escape the pit!");
   u.rem_disease(DI_IN_PIT);
  }
 }
 if (m.move_cost(x, y) > 0) { // move_cost() of 0 = impassible (e.g. a wall)
  if (u.underwater)
   u.underwater = false;
  int movecost;
  if (m.field_at(x, y).is_dangerous() &&
      !query_yn("Really step into that %s?", m.field_at(x, y).name().c_str()))
   return;
  if (m.tr_at(x, y) != tr_null &&
      u.per_cur - u.encumb(bp_eyes) >= traps[m.tr_at(x, y)]->visibility &&
      !query_yn("Really step onto that %s?",traps[m.tr_at(x, y)]->name.c_str()))
   return;
  if (u.has_trait(PF_PARKOUR) && m.move_cost(x, y) <= 4)
   movecost = 100 + u.encumb(bp_feet) * 5 + u.encumb(bp_legs) * 3;
  else
   movecost = m.move_cost(x, y) * 50 + u.encumb(bp_feet) * 5 +
                                       u.encumb(bp_legs) * 3;
  if (u.has_trait(PF_FLEET) && m.move_cost(x, y) == 2)
   movecost = int(movecost * .85);
  movecost += u.encumb(bp_mouth) * 5;
  if (!u.wearing_something_on(bp_feet))
   movecost += 15;
  if (u.recoil > 0) {
   if (int(u.str_cur / 2) + u.sklevel[sk_gun] >= u.recoil)
    u.recoil = 0;
   else {
    u.recoil -= int(u.str_cur / 2) + u.sklevel[sk_gun];
    u.recoil = int(u.recoil / 2);
   }
  }
  u.moves -= movecost;
  if (m.move_cost(x, y) > 2 &&
      (!u.has_trait(PF_PARKOUR) || m.move_cost(x, y) > 4))
   add_msg("Moving past this %s is slow!", m.tername(x, y).c_str());
  if (m.has_flag(rough, x, y)) {
   if (one_in(5) && u.armor_bash(bp_feet) < rng(1, 5)) {
    add_msg("You hurt your feet on the %s!", m.tername(x, y).c_str());
    u.hit(this, bp_feet, 0, 0, 1);
    u.hit(this, bp_feet, 1, 0, 1);
   }
  }
  if (m.has_flag(sharp, x, y) && !one_in(3) && !one_in(40 - int(u.dex_cur/2))) {
   add_msg("You cut yourself on the %s!", m.tername(x, y).c_str());
   u.hit(this, bp_torso, 0, 0, rng(1, 4));
  }
  if (u.has_trait(PF_LIGHTSTEP))
   sound(x, y, 2, "");	// Sound of footsteps may awaken nearby monsters
  else
   sound(x, y, 6, "");	// Sound of footsteps may awaken nearby monsters
// If we moved out of the nonant, we need update our map data
  if (m.has_flag(swimmable, x, y) && u.has_disease(DI_ONFIRE)) {
   add_msg("The water puts out the flames!");
   u.rem_disease(DI_ONFIRE);
  }
  if (displace) {	// We displaced a friendly monster!
   z[mondex].move_to(this, u.posx, u.posy);
   add_msg("You displace the %s.", z[mondex].name().c_str());
  }
  if (x < SEEX || y < SEEY || x >= SEEX * 2 || y >= SEEY * 2)
   update_map(x, y);
  u.posx = x;
  u.posy = y;
  if (m.tr_at(x, y) != tr_null) { // We stepped on a trap!
   trap* tr = traps[m.tr_at(x, y)];
   if (!u.avoid_trap(tr)) {
    trapfunc f;
    (f.*(tr->act))(this, x, y);
   }
  }

// Special tutorial messages
  if (in_tutorial) {
   bool showed_message = false;
   for (int i = u.posx - 1; i <= u.posx + 1 && !showed_message; i++) {
    for (int j = u.posy - 1; j <= u.posy + 1 && !showed_message; j++) {
     if (m.ter(i, j) == t_door_c) {
      showed_message = true;
      tutorial_message(LESSON_OPEN);
     } else if (m.ter(i, j) == t_door_o) {
      showed_message = true;
      tutorial_message(LESSON_CLOSE);
     } else if (m.ter(i, j) == t_window) {
      showed_message = true;
      tutorial_message(LESSON_WINDOW);
     } else if (m.ter(i, j) == t_rack && m.i_at(i, j).size() > 0) {
      showed_message = true;
      tutorial_message(LESSON_EXAMINE);
     } else if (m.ter(i, j) == t_stairs_down) {
      showed_message = true;
      tutorial_message(LESSON_STAIRS);
     } else if (m.ter(i, j) == t_water_sh) {
      showed_message = true;
      tutorial_message(LESSON_PICKUP_WATER);
     }
    }
   }
  }
// List items here
  if (!u.has_disease(DI_BLIND) && m.i_at(x, y).size() <= 3 &&
                                  m.i_at(x, y).size() != 0) {
   if (in_tutorial)
    tutorial_message(LESSON_PICKUP);
   std::string buff = "You see here ";
   for (int i = 0; i < m.i_at(x, y).size(); i++) {
    buff += m.i_at(x, y)[i].tname(this);
    if (i + 2 < m.i_at(x, y).size())
     buff += ", ";
    else if (i + 1 < m.i_at(x, y).size())
     buff += ", and ";
   }
   buff += ".";
   add_msg(buff.c_str());
  } else if (m.i_at(x, y).size() != 0)
   add_msg("There are many items here.");
 } else if (m.has_flag(swimmable, x, y)) {	// Dive into water!
// Requires confirmation if we were on dry land previously
  if ((m.has_flag(swimmable, u.posx, u.posy) &&
      m.move_cost(u.posx, u.posy) == 0) || query_yn("Dive into the water?")) {
   if (m.move_cost(u.posx, u.posy) > 0 && u.swim_speed() < 500)
    add_msg("You start swimming.  Press '>' to dive underwater.");
   plswim(x, y);
   if (u.has_disease(DI_ONFIRE)) {
    add_msg("The water puts out the flames!");
    u.rem_disease(DI_ONFIRE);
   }
  }
 } else { // Invalid move
  if (u.has_disease(DI_BLIND)) {
   add_msg("You bump into something!");
   u.moves -= 100;
  } else if (m.open_door(x, y, m.ter(u.posx, u.posy) == t_floor))
   u.moves -= 100;
  else if (m.ter(x, y) == t_door_locked) {
   u.moves -= 100;
   add_msg("That door is locked!");
   if (in_tutorial)
    tutorial_message(LESSON_SMASH);
  }
 }
}

void game::plswim(int x, int y)
{
 if (x < SEEX || y < SEEY || x >= SEEX * 2 || y >= SEEY * 2)
  update_map(x, y);
 u.posx = x;
 u.posy = y;
 if (!m.has_flag(swimmable, x, y)) {
  debugmsg("Tried to swim in %s!", m.tername(x, y).c_str());
  return;
 }
 int movecost = u.swim_speed();
 u.practice(sk_swimming, 1);
 if (movecost >= 500) {
  if (!u.underwater) {
   add_msg("You sink%s!", (movecost >= 400 ? " like a rock" : ""));
   u.underwater = true;
   u.oxygen = 20 + u.str_cur;
  }
 }
 if (u.oxygen <= 5 && u.underwater) {
  if (movecost < 500)
   popup("You need to breathe! (Press '<' to surface.)");
  else
   popup("You need to breathe but you can't swim!  Get to dry land, quick!");
 }
 u.moves -= movecost;
 for (int i = 0; i < u.inv.size(); i++) {
  if (u.inv[i].type->m1 == IRON && u.inv[i].damage < 5 && one_in(3))
   u.inv[i].damage++;
 }
}

void game::vertical_move(int movez, bool force)
{
// > and < are used for diving underwater.
 if (m.move_cost(u.posx, u.posy) == 0 && m.has_flag(swimmable, u.posx, u.posy)){
  if (movez == -1) {
   u.underwater = true;
   u.oxygen = 20 + u.str_cur;
   add_msg("You dive underwater!");
  } else {
   if (u.swim_speed() < 500) {
    u.underwater = false;
    add_msg("You surface.");
   } else
    add_msg("You can't surface!");
  }
  return;
 }
// Force means we're going down, even if there's no staircase, etc.
// This happens with sinkholes and the like.
 if (!force && ((movez == -1 && !m.has_flag(goes_down, u.posx, u.posy)) ||
                (movez ==  1 && !m.has_flag(goes_up,   u.posx, u.posy))   )) {
  add_msg("You can't go %s here!", (movez == -1 ? "down" : "up"));
  return;
 }
 levz += movez;
 if (levz == monbuffz &&
     monbuff_turn + 120 <= turn - trig_dist(levx, levy, monbuffx, monbuffy)) {
  for (int i = 0; i < monbuff.size(); i++)
   z.push_back(monbuff[i]);
  monbuff.clear();
 }
 u.moves -= 100;
 if (force) {	// Basically, we fell.
  int dam = int((u.str_max / 4) + rng(5, 10)) * rng(1, 3);// The bigger they are
  dam -= rng(u.dodge(), u.dodge() * 3);
  if (dam <= 0)
   add_msg("You fall expertly and take no damage.");
  else {
   add_msg("You fall heavily, taking %d damage.", dam);
   u.hurtall(dam);
  }
 }
 int group, junk;
 monbuffx = levx;
 monbuffy = levy;
 monbuffz = levz - movez;
 for (int i = 0; i < z.size(); i++) {
  if (trig_dist(u.posx, u.posy, z[i].posx, z[i].posy) > 3) {
   monbuff.push_back(z[i]);
   group = valid_group((mon_id)(z[i].type->id), levx, levy);
   if (group != -1) {
    cur_om.zg[group].population++;
    if (cur_om.zg[group].population / pow(cur_om.zg[group].radius, 2) > 5)
     cur_om.zg[group].radius++;
   } else if (mt_to_mc((mon_id)(z[i].type->id)) != mcat_null)
    cur_om.zg.push_back(mongroup(mt_to_mc(mon_id(z[i].type->id)),
                                 levx, levy, 1, 1));
   z.erase(z.begin()+i);
   i--;
  } else if (u_see(&(z[i]), junk))
   add_msg("The %s follows you %s.", z[i].name().c_str(),
           (movez == -1 ? "down" : "up"));
 }
 cur_om.save(u.name);
 m.save(&cur_om, turn, levx, levy);
 cur_om = overmap(this, cur_om.posx, cur_om.posy, cur_om.posz + movez);
 m.load(this, levx, levy);
// Move the player to the corresponding up-route. (If one exists.)
 for (int i = 0; i < SEEX * 3; i++) {
  for (int j = 0; j < SEEY * 3; j++) {
   if ((movez == -1 && m.has_flag(goes_up,   i, j)) ||
       (movez ==  1 && m.has_flag(goes_down, i, j))   )  {
    u.posx = i;
    u.posy = j;
    j = SEEY * 3;
    i = SEEX * 3;
   } else if (movez == 1 && m.ter(i, j) == t_manhole_cover) {
    m.add_item(i + rng(-1, 1), j + rng(-1, 1), itypes[itm_manhole_cover], 0);
    m.ter(i, j) = t_manhole;
    u.posx = i;
    u.posy = j;
    j = SEEY * 3;
    i = SEEX * 3;
   }
  }
 }
 update_map(u.posx, u.posy);
 for (int i = 0; i < SEEX * 3; i++) {
  for (int j = 0; j < SEEY * 3; j++) {
   if ((movez == -1 && m.has_flag(goes_up,   i, j)) ||
       (movez ==  1 && m.has_flag(goes_down, i, j))   )  {
    u.posx = i;
    u.posy = j;
    j = SEEY * 3;
    i = SEEX * 3;
   }
  }
 }
 refresh_all();
}


void game::update_map(int &x, int &y)
{
 int shiftx = 0, shifty = 0;
 int group;
 int olevx = 0, olevy = 0;
      if (x <  SEEX    ) { x = SEEX * 2 - 1; shiftx = -1; }
 else if (x >= SEEX * 2) { x = SEEX;         shiftx =  1; }
      if (y <  SEEY    ) { y = SEEY * 2 - 1; shifty = -1; }
 else if (y >= SEEY * 2) { y = SEEY;         shifty =  1; }
// Before we shift/save the map, check if we need to move monsters back to
// their spawn locations.
 m.shift(this, levx, levy, shiftx, shifty);
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
  cur_om.save(u.name);
  cur_om = overmap(this, cur_om.posx + olevx, cur_om.posy + olevy, cur_om.posz);
 }

// Shift monsters
 for (int i = 0; i < z.size(); i++) {
  z[i].shift(shiftx, shifty);
  if (z[i].posx < 0 - SEEX * 3 || z[i].posy < 0 - SEEX * 3 ||
      z[i].posx >     SEEX * 6 || z[i].posy >     SEEY * 6   ) {
   if (z[i].spawnmapx != -1) {	// Static spawn, move them back there
    map tmp;
    tmp.load(this, z[i].spawnmapx, z[i].spawnmapy);
    tmp.add_spawn(mon_id(z[i].type->id), 1, z[i].spawnposx, z[i].spawnposy);
    tmp.save(&cur_om, turn, z[i].spawnmapx, z[i].spawnmapy);
   } else {	// Absorb them back into a group
    group = valid_group((mon_id)(z[i].type->id), levx + shiftx, levy + shifty);
    if (group != -1) {
     cur_om.zg[group].population++;
     if (cur_om.zg[group].population / pow(cur_om.zg[group].radius, 2) > 5)
      cur_om.zg[group].radius++;
    } else if (mt_to_mc((mon_id)(z[i].type->id)) != mcat_null)
     cur_om.zg.push_back(mongroup(mt_to_mc((mon_id)(z[i].type->id)),
                                  levx + shiftx, levy + shifty, 1, 1));
   }
   z.erase(z.begin()+i);
   i--;
  }
 }
// Shift NPCs
 for (int i = 0; i < active_npc.size(); i++) {
  active_npc[i].shift(shiftx, shifty);
  if (active_npc[i].posx < 0 - SEEX * 3 || active_npc[i].posy < 0 - SEEX * 3 ||
      active_npc[i].posx >     SEEX * 6 || active_npc[i].posy >     SEEY * 6) {
   cur_om.npcs.push_back(active_npc[i]);
   active_npc.erase(active_npc.begin() + i);
   i--;
  }
 }
// Spawn static NPCs?
 if (!in_tutorial) {
  npc temp;
  for (int i = 0; i < cur_om.npcs.size(); i++) {
   if (rl_dist(levx, levy, cur_om.npcs[i].mapx, cur_om.npcs[i].mapy) <= 2) {
    int dx = cur_om.npcs[i].mapx - levx, dy = cur_om.npcs[i].mapy - levy;
    if (debugmon)
     debugmsg("Spawning static NPC, %d:%d (%d:%d)", levx, levy,
              cur_om.npcs[i].mapx, cur_om.npcs[i].mapy);
    temp = cur_om.npcs[i];
    if (temp.posx == -1 || temp.posy == -1) {
     debugmsg("Static NPC with no fine location data.");
     temp.posx = SEEX * 2 * (temp.mapx - levx) + rng(0 - SEEX, SEEX);
     temp.posy = SEEY * 2 * (temp.mapy - levy) + rng(0 - SEEY, SEEY);
    } else {
     if (debugmon)
      debugmsg("Static NPC fine location %d:%d (%d:%d)", temp.posx, temp.posy,
               temp.posx + dx * SEEX, temp.posy + dy * SEEY);
     temp.posx += dx * SEEX;
     temp.posy += dy * SEEY;
    }
    active_npc.push_back(temp);
    cur_om.npcs.erase(cur_om.npcs.begin() + i);
    i--;
   }
  }
 }
// Spawn monsters if appropriate
 m.spawn_monsters(this);	// Static monsters
 if (turn >= nextspawn)
  spawn_mon(shiftx, shifty);
// Shift scent
 unsigned int newscent[SEEX * 3][SEEY * 3];
 for (int i = 0; i < SEEX * 3; i++) {
  for (int j = 0; j < SEEY * 3; j++)
   newscent[i][j] = scent(i + (shiftx * SEEX), j + (shifty * SEEY));
 }
 for (int i = 0; i < SEEX * 3; i++) {
  for (int j = 0; j < SEEY * 3; j++)
   scent(i, j) = newscent[i][j];
 }
 draw_minimap();
}

void game::spawn_mon(int shiftx, int shifty)
{
 int nlevx = levx + shiftx;
 int nlevy = levy + shifty;
 int group;
 int monx, mony, rntype;
 int dist;
 int pop, rad;
 int iter;
 int t;
 // Create a new NPC?
  if (one_in(50 + 5 * cur_om.npcs.size())) {
   npc temp;
   temp.randomize(this);
   temp.spawn_at(&cur_om, levx + (1 * rng(-2, 2)), levy + (1 * rng(-2, 2)));
   temp.posx = SEEX * 2 * (temp.mapx - levx) + rng(0 - SEEX, SEEX);
   temp.posy = SEEY * 2 * (temp.mapy - levy) + rng(0 - SEEY, SEEY);
   temp.attitude = NPCATT_TALK;
   active_npc.push_back(temp);
  }

// Now, spawn monsters (perhaps)
 monster zom;
 for (int i = 0; i < cur_om.zg.size(); i++) { // For each valid group...
  group = 0;
  rntype = 0;
  dist = trig_dist(nlevx, nlevy, cur_om.zg[i].posx, cur_om.zg[i].posy);
  pop = cur_om.zg[i].population;
  rad = cur_om.zg[i].radius;
  if (dist <= rad) {
// (The area of the group's territory) in (population/square at this range)
// chance of adding one monster; cap at the population OR 16
   while (long((1.1 - double(dist / rad)) * pop) > rng(0, pow(rad, 2)) &&
          rng(1, 20) > group && group < pop && group < 16)
    group++;
   cur_om.zg[i].population -= group;
   if (group > 0) // If we spawned some zombies, advance the timer
    nextspawn += rng(group + z.size() * 5, group * 3 + z.size() * 10);
   for (int j = 0; j < group; j++) {	// For each monster in the group...
    mon_id type = valid_monster_from(moncats[cur_om.zg[i].type]);
    if (type == mon_null)
     j = group;	// No monsters may be spawned; not soon enough?
    else {
     zom = monster(mtypes[type]);
     iter = 0;
     do {
      monx = rng(0, SEEX * 3 - 1);
      mony = rng(0, SEEY * 3 - 1);
      if (shiftx == -1) monx = 0 - SEEX * 2; if (shiftx == 1) monx = SEEX * 4;
      if (shifty == -1) mony = 0 - SEEX * 2; if (shifty == 1) mony = SEEY * 4;
      monx += rng(-5, 10);
      mony += rng(-5, 10);
      iter++;
     } while ((!zom.can_move_to(m, monx, mony) || !is_empty(monx, mony) ||
                m.sees(u.posx, u.posy, monx, mony, SEEX, t)) && iter < 50);
     if (iter < 50) {
      zom.spawn(monx, mony);
      z.push_back(zom);
     }
    }
   }	// Placing monsters of this group is done!
   if (cur_om.zg[i].population <= 0) { // Last monster in the group spawned...
    cur_om.zg.erase(cur_om.zg.begin() + i); // ...so remove that group
    i--;	// And don't increment i.
   }
  }
 }
}

mon_id game::valid_monster_from(std::vector<mon_id> group)
{
 std::vector<mon_id> valid;
 int rntype = 0;
 for (int i = 0; i < group.size(); i++) {
  if (mtypes[group[i]]->frequency > 0 &&
      turn + 900 >= mtypes[group[i]]->difficulty * 300) {
   valid.push_back(group[i]);
   rntype += mtypes[group[i]]->frequency;
  }
 }
 if (valid.size() == 0)
  return mon_null;
 int curmon = -1;
 if (rntype > 0)
  rntype = rng(0, rntype - 1);	// rntype set to [0, rntype)
 do {
  curmon++;
  rntype -= mtypes[valid[curmon]]->frequency;
 } while (rntype > 0);
 return valid[curmon];
}


int game::valid_group(mon_id type, int x, int y)
{
 std::vector <int> valid_groups;
 std::vector <int> semi_valid;	// Groups that're ALMOST big enough
 int dist;
 for (int i = 0; i < cur_om.zg.size(); i++) {
  dist = trig_dist(x, y, cur_om.zg[i].posx, cur_om.zg[i].posy);
  if (dist < cur_om.zg[i].radius) {
   for (int j = 0; j < (moncats[cur_om.zg[i].type]).size(); j++) {
    if (type == (moncats[cur_om.zg[i].type])[j]) {
     valid_groups.push_back(i);
     j = (moncats[cur_om.zg[i].type]).size();
    }
   }
  } else if (dist < cur_om.zg[i].radius + 1) {
   for (int j = 0; j < (moncats[cur_om.zg[i].type]).size(); j++) {
    if (type == (moncats[cur_om.zg[i].type])[j]) {
     semi_valid.push_back(i);
     j = (moncats[cur_om.zg[i].type]).size();
    }
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
   cur_om.zg[semi_valid[semi]].radius++;
   return semi_valid[semi];
  }
 }
 return valid_groups[rng(0, valid_groups.size() - 1)];
}

void game::wait()
{
 char ch = menu("Wait for how long?", "5 Minutes", "30 Minutes", "1 hour",
                "2 hours", "3 hours", "6 hours", "Exit", NULL);
 int time;
 if (ch == 7)
  return;
 switch (ch) {
  case 1: time =   5000; break;
  case 2: time =  30000; break;
  case 3: time =  60000; break;
  case 4: time = 120000; break;
  case 5: time = 180000; break;
  case 6: time = 360000; break;
 }
 u.activity = player_activity(ACT_WAIT, time, 0);
 u.moves = 0;
}

void game::gameover()
{
 erase();
 mvprintw(0, 35, "GAME OVER");
 inv();
}

void game::write_msg()
{
 werase(w_messages);
 int size = 7;
 for (int i = size; i > 0; i--) {
  if (messages.size() >= i) {
   if (curmes >= i)
    mvwprintz(w_messages, size - (i - 1), 0, c_ltred,
              messages[messages.size() - i].c_str());
   else
    mvwprintz(w_messages, size - (i - 1), 0, c_dkgray,
              messages[messages.size() - i].c_str());
  }
 }
 curmes = 0;
 wrefresh(w_messages);
}

void game::teleport()
{
 int newx, newy, tries = 0;
 u.add_disease(DI_TELEGLOW, 300, this);
 do {
  newx = u.posx + rng(0, SEEX * 2) - SEEX;
  newy = u.posy + rng(0, SEEY * 2) - SEEY;
  tries++;
 } while (tries < 15 &&
          (m.move_cost(newx, newy) == 0 || mon_at(newx, newy) != -1));
 u.posx = newx;
 u.posy = newy;
 if (tries == 15) {
  if (m.move_cost(newx, newy) == 0) {	// TODO: If we land in water, swim
   add_msg("You teleport into the middle of a %s!",
           m.tername(newx, newy).c_str());
   u.hurt(this, bp_torso, 0, 500);
  } else if (mon_at(newx, newy) != -1) {
   int i = mon_at(newx, newy);
   add_msg("You teleport into the middle of a %s!", z[i].name().c_str());
   explode_mon(i);
  }
 }
 update_map(u.posx, u.posy);
}

void game::nuke(int x, int y)
{
 overmap tmp_om = cur_om;
 cur_om = overmap(this, tmp_om.posx, tmp_om.posy, 0);
 if (x < 0 || y < 0 || x >= OMAPX || y >= OMAPY)
  return;
 int mapx = x * 2, mapy = y * 2;
 map tmpmap(&itypes, &mapitems, &traps);
 tmpmap.load(this, mapx, mapy);
 for (int i = 0; i < SEEX * 2; i++) {
  for (int j = 0; j < SEEY * 2; j++) {
   if (!one_in(10))
    tmpmap.ter(i, j) = t_rubble;
   if (one_in(3))
    tmpmap.add_field(NULL, i, j, fd_nuke_gas, 3);
   tmpmap.radiation(i, j) += rng(20, 80);
  }
 }
 tmpmap.save(&cur_om, turn, mapx, mapy);
 cur_om.ter(x, y) = ot_crater;
 cur_om = tmp_om;
}

std::vector<faction *> game::factions_at(int x, int y)
{
 std::vector<faction *> ret;
 for (int i = 0; i < factions.size(); i++) {
  if (factions[i].omx == cur_om.posx && factions[i].omy == cur_om.posy &&
      trig_dist(x, y, factions[i].mapx, factions[i].mapy) <= factions[i].size)
   ret.push_back(&(factions[i]));
 }
 return ret;
}

oter_id game::ter_at(int omx, int omy, bool& mark_as_seen)
{
 oter_id ret;
 int sx = 0, sy = 0;
 if (omx >= OMAPX)
  sx = 1;
 if (omx < 0)
  sx = -1;
 if (omy >= OMAPY)
  sy = 1;
 if (omy < 0)
  sy = -1;
 if (sx != 0 || sy != 0) {
  omx -= sx * OMAPX;
  omy -= sy * OMAPY;
  overmap tmp(this, cur_om.posx + sx, cur_om.posy + sy, 0);
  if (mark_as_seen) {
   tmp.seen(omx, omy) = true;
   tmp.save(u.name, tmp.posx, tmp.posy, cur_om.posz);
  } else {
   mark_as_seen = tmp.seen(omx, omy);
  }
  ret = tmp.ter(omx, omy);
 } else {
  ret = cur_om.ter(omx, omy);
  if (mark_as_seen)
   cur_om.seen(omx, omy) = true;
  else
   mark_as_seen = cur_om.seen(omx, omy);
 }
 return ret;
}

moncat_id game::mt_to_mc(mon_id type)
{
 for (int i = 0; i < num_moncats; i++) {
  for (int j = 0; j < (moncats[i]).size(); j++) {
   if ((moncats[i])[j] == type)
    return (moncat_id)(i);
  }
 }
 return mcat_null;
}

nc_color sev(int a)
{
 switch (a) {
  case 0: return c_cyan;
  case 1: return c_blue;
  case 2: return c_green;
  case 3: return c_yellow;
  case 4: return c_ltred;
  case 5: return c_red;
  case 6: return c_magenta;
 }
 return c_dkgray;
}

void intro()
{
 int maxx, maxy;
 getmaxyx(stdscr, maxy, maxx);
 WINDOW* tmp = newwin(25, 80, 0, 0);
 while (maxy < 25 || maxx < 80) {
  werase(tmp);
  wprintw(tmp, "\
Whoa. Whoa. Hey. This game requires a minimum terminal size of 80x25. I'm\n\
sorry if your graphical terminal emulator went with the woefully-diminuitive\n\
80x24 as its default size, but that just won't work here.  Now stretch the\n\
bottom of your window downward so you get an extra line.\n");
  wrefresh(tmp);
  refresh();
  wrefresh(tmp);
  getch();
  getmaxyx(stdscr, maxy, maxx);
 }
 werase(tmp);
 wrefresh(tmp);
 delwin(tmp);
 erase();
}
