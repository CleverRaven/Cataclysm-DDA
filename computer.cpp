#include "computer.h"
#include "game.h"
#include "monster.h"
#include "overmap.h"
#include "output.h"
#include <fstream>
#include <string>
#include <sstream>

computer::computer()
{
 security = 0;
 name = DEFAULT_COMPUTER_NAME;
 w_terminal = NULL;
 mission_id = -1;
}

computer::computer(std::string Name, int Security)
{
 security = Security;
 name = Name;
 w_terminal = NULL;
 mission_id = -1;
}

computer::~computer()
{
 if (w_terminal != NULL)
  delwin(w_terminal);
}

computer& computer::operator=(const computer &rhs)
{
 security = rhs.security;
 name = rhs.name;
 mission_id = rhs.mission_id;
 options.clear();
 for (int i = 0; i < rhs.options.size(); i++)
  options.push_back(rhs.options[i]);
 failures.clear();
 for (int i = 0; i < rhs.failures.size(); i++)
   failures.push_back(rhs.failures[i]);
 w_terminal = NULL;
 return *this;
}

void computer::set_security(int Security)
{
 security = Security;
}

void computer::add_option(std::string opt_name, computer_action action,
                          int Security)
{
 options.push_back(computer_option(opt_name, action, Security));
}

void computer::add_failure(computer_failure failure)
{
 failures.push_back(failure);
}

void computer::shutdown_terminal()
{
 werase(w_terminal);
 delwin(w_terminal);
 w_terminal = NULL;
}

void computer::use(game *g)
{
 if (w_terminal == NULL)
  w_terminal = newwin(25, 80, 0, 0);
 wborder(w_terminal, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                     LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );

 print_line("Logging into %s...", name.c_str());

 if (security > 0) {
  print_error("ERROR!  Access denied!");
  switch (query_ynq("Bypass security?")) {
  case 'q':
  case 'Q':
   shutdown_terminal();
   return;

  case 'n':
  case 'N':
   print_line("Shutting down... press any key.");
   getch();
   shutdown_terminal();
   return;

  case 'y':
  case 'Y':
   if (!hack_attempt(&(g->u))) {
    if (failures.size() == 0) {
     print_line("Maximum login attempts exceeded. Press any key...");
     getch();
     shutdown_terminal();
     return;
    }
    activate_random_failure(g);
    shutdown_terminal();
    return;
   } else { // Successful hack attempt
    security = 0;
    print_line("Login successful.  Press any key...");
    getch();
    reset_terminal();
   }
  }
 } else { // No security
  print_line("Login successful.  Press any key...");
  getch();
  reset_terminal();
 }

// Main computer loop

 bool done = false; // Are we done using the computer?
 do {
  //reset_terminal();
  print_line("");
  print_line("%s - Root Menu", name.c_str());
  for (int i = 0; i < options.size(); i++)
   print_line("%d - %s", i + 1, options[i].name.c_str());
  print_line("Q - Quit and shut down");
  print_line("");

  char ch;
  do
   ch = getch();
  while (ch != 'q' && ch != 'Q' && (ch < '1' || ch - '1' >= options.size()));
  if (ch == 'q' || ch == 'Q')
   done = true;
  else { // We selected an option other than quit.
   ch -= '1'; // So '1' -> 0; index in options.size()
   computer_option current = options[ch];
   if (current.security > 0) {
    print_error("Password required.");
    if (query_bool("Hack into system?")) {
     if (!hack_attempt(&(g->u), current.security)) {
      activate_random_failure(g);
      shutdown_terminal();
      return;
     } else {
      activate_function(g, current.action);
      reset_terminal();
     }
    }
   } else // No need to hack, just activate
    activate_function(g, current.action);
  } // Done processing a selected option.
 } while (!done); // Done with main terminal loop

 shutdown_terminal(); // This should have been done by now, but just in case.
}

bool computer::hack_attempt(player *p, int Security)
{
 if (Security == -1)
  Security = security; // Set to main system security if no value passed

 p->practice("computer", 5 + Security * 2);
 int player_roll = p->skillLevel("computer").level();
 if (p->int_cur < 8 && one_in(2))
  player_roll -= rng(0, 8 - p->int_cur);
 else if (p->int_cur > 8 && one_in(3))
  player_roll += rng(0, p->int_cur - 8);

 return (dice(player_roll, 6) >= dice(Security, 6));
}

std::string computer::save_data()
{
 std::stringstream data;
 std::string savename = name; // Replace " " with "_"
 size_t found = savename.find(" ");
 while (found != std::string::npos) {
  savename.replace(found, 1, "_");
  found = savename.find(" ");
 }
 data << savename << " " << security << " " << mission_id << " " <<
         options.size() << " ";
 for (int i = 0; i < options.size(); i++) {
  savename = options[i].name;
  found = savename.find(" ");
  while (found != std::string::npos) {
   savename.replace(found, 1, "_");
   found = savename.find(" ");
  }
  data << savename << " " << int(options[i].action) << " " <<
          options[i].security << " ";
 }
 data << failures.size() << " ";
 for (int i = 0; i < failures.size(); i++)
  data << int(failures[i]) << " ";

 return data.str();
}

void computer::load_data(std::string data)
{
 options.clear();
 failures.clear();
 std::stringstream dump;
 std::string buffer;
 dump << data;
// Pull in name and security
 dump >> name >> security >> mission_id;
 size_t found = name.find("_");
 while (found != std::string::npos) {
  name.replace(found, 1, " ");
  found = name.find("_");
 }
// Pull in options
 int optsize;
 dump >> optsize;
 for (int n = 0; n < optsize; n++) {
  std::string tmpname;
  int tmpaction, tmpsec;
  dump >> tmpname >> tmpaction >> tmpsec;
  size_t found = tmpname.find("_");
  while (found != std::string::npos) {
   tmpname.replace(found, 1, " ");
   found = tmpname.find("_");
  }
  add_option(tmpname, computer_action(tmpaction), tmpsec);
 }
// Pull in failures
 int failsize, tmpfail;
 dump >> failsize;
 for (int n = 0; n < failsize; n++) {
  dump >> tmpfail;
  failures.push_back(computer_failure(tmpfail));
 }
}

void computer::activate_function(game *g, computer_action action)
{
 switch (action) {

  case COMPACT_NULL:
   break; // Why would this be called?

  case COMPACT_OPEN:
   g->m.translate(t_door_metal_locked, t_floor);
   print_line("Doors opened.");
   break;

  case COMPACT_SAMPLE:
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++) {
     if (g->m.ter(x, y) == t_sewage_pump) {
      for (int x1 = x - 1; x1 <= x + 1; x1++) {
       for (int y1 = y - 1; y1 <= y + 1; y1++ ) {
        if (g->m.ter(x1, y1) == t_counter) {
         bool found_item = false;
         for (int i = 0; i < g->m.i_at(x1, y1).size(); i++) {
          item *it = &(g->m.i_at(x1, y1)[i]);
          if (it->is_container() && it->contents.empty()) {
           it->put_in( item(g->itypes[itm_sewage], g->turn) );
           found_item = true;
          }
         }
         if (!found_item) {
          item sewage(g->itypes[itm_sewage], g->turn);
          g->m.add_item(x1, y1, sewage);
         }
        }
       }
      }
     }
    }
   }
   break;

  case COMPACT_RELEASE:
   g->sound(g->u.posx, g->u.posy, 40, "An alarm sounds!");
   g->m.translate(t_reinforced_glass_h, t_floor);
   g->m.translate(t_reinforced_glass_v, t_floor);
   print_line("Containment shields opened.");
   break;

  case COMPACT_TERMINATE:
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++) {
     int mondex = g->mon_at(x, y);
     if (mondex != -1 &&
         ((g->m.ter(x, y - 1) == t_reinforced_glass_h &&
           g->m.ter(x, y + 1) == t_wall_h) ||
          (g->m.ter(x, y + 1) == t_reinforced_glass_h &&
           g->m.ter(x, y - 1) == t_wall_h)))
      g->kill_mon(mondex, true);
    }
   }
   print_line("Subjects terminated.");
   break;

  case COMPACT_PORTAL:
   for (int i = 0; i < SEEX * MAPSIZE; i++) {
    for (int j = 0; j < SEEY * MAPSIZE; j++) {
     int numtowers = 0;
     for (int xt = i - 2; xt <= i + 2; xt++) {
      for (int yt = j - 2; yt <= j + 2; yt++) {
       if (g->m.ter(xt, yt) == t_radio_tower)
        numtowers++;
      }
     }
     if (numtowers == 4) {
      if (g->m.tr_at(i, j) == tr_portal)
       g->m.tr_at(i, j) = tr_null;
      else
       g->m.add_trap(i, j, tr_portal);
     }
    }
   }
   break;

  case COMPACT_CASCADE: {
   if (!query_bool("WARNING: Resonance cascade carries severe risk!  Continue?"))
    return;
   std::vector<point> cascade_points;
   for (int i = g->u.posx - 10; i <= g->u.posx + 10; i++) {
    for (int j = g->u.posy - 10; j <= g->u.posy + 10; j++) {
     if (g->m.ter(i, j) == t_radio_tower)
      cascade_points.push_back(point(i, j));
    }
   }
   if (cascade_points.size() == 0)
    g->resonance_cascade(g->u.posx, g->u.posy);
   else {
    point p = cascade_points[rng(0, cascade_points.size() - 1)];
    g->resonance_cascade(p.x, p.y);
   }
  } break;

  case COMPACT_RESEARCH: {
   int lines = 0, notes = 0;
   std::string log, tmp;
   int ch;
   std::ifstream fin;
   fin.open("data/LAB_NOTES");
   if (!fin.is_open()) {
    debugmsg("Couldn't open ./data/LAB_NOTES for reading");
    return;
   }
   while (fin.good()) {
    ch = fin.get();
    if (ch == '%')
     notes++;
   }

   while (lines < 10) {
    fin.clear();
    fin.seekg(0, std::ios::beg);
    fin.clear();
    int choice = rng(1, notes);
    while (choice > 0) {
     getline(fin, tmp);
     if (tmp.find_first_of('%') == 0)
      choice--;
    }
    getline(fin, tmp);
    do {
     lines++;
     if (lines < 15 && tmp.find_first_of('%') != 0) {
      log.append(tmp);
      log.append("\n");
     }
    } while(tmp.find_first_of('%') != 0 && getline(fin, tmp));
   }
   print_line(" %s", log.c_str());
   print_line("Press any key...");
   getch();
  } break;

  case COMPACT_MAPS: {
   int minx = int((g->levx + int(MAPSIZE / 2)) / 2) - 40;
   int maxx = int((g->levx + int(MAPSIZE / 2)) / 2) + 40;
   int miny = int((g->levy + int(MAPSIZE / 2)) / 2) - 40;
   int maxy = int((g->levy + int(MAPSIZE / 2)) / 2) + 40;
   if (minx < 0)             minx = 0;
   if (maxx >= OMAPX) maxx = OMAPX - 1;
   if (miny < 0)             miny = 0;
   if (maxy >= OMAPY) maxy = OMAPY - 1;
   overmap tmp(g, g->cur_om.posx, g->cur_om.posy, 0);
   for (int i = minx; i <= maxx; i++) {
    for (int j = miny; j <= maxy; j++)
     tmp.seen(i, j) = true;
   }
   tmp.save(g->u.name, g->cur_om.posx, g->cur_om.posy, 0);
   print_line("Surface map data downloaded.");
  } break;

  case COMPACT_MAP_SEWER: {
   int minx = int((g->levx + int(MAPSIZE / 2)) / 2) - 60;
   int maxx = int((g->levx + int(MAPSIZE / 2)) / 2) + 60;
   int miny = int((g->levy + int(MAPSIZE / 2)) / 2) - 60;
   int maxy = int((g->levy + int(MAPSIZE / 2)) / 2) + 60;
   if (minx < 0)      minx = 0;
   if (maxx >= OMAPX) maxx = OMAPX - 1;
   if (miny < 0)      miny = 0;
   if (maxy >= OMAPY) maxy = OMAPY - 1;
   for (int i = minx; i <= maxx; i++) {
    for (int j = miny; j <= maxy; j++)
     if ((g->cur_om.ter(i, j) >= ot_sewer_ns &&
          g->cur_om.ter(i, j) <= ot_sewer_nesw) ||
         (g->cur_om.ter(i, j) >= ot_sewage_treatment &&
          g->cur_om.ter(i, j) <= ot_sewage_treatment_under))
     g->cur_om.seen(i, j) = true;
   }
   print_line("Sewage map data downloaded.");
  } break;


  case COMPACT_MISS_LAUNCH: {
   overmap tmp_om(g, g->cur_om.posx, g->cur_om.posy, 0);
// Target Acquisition.
   point target = tmp_om.choose_point(g);
   if (target.x == -1) {
    print_line("Launch canceled.");
    return;
   }
// Figure out where the glass wall is...
   int wall_spot = 0;
   for (int i = g->u.posx; i < g->u.posx + SEEX * 2 && wall_spot == 0; i++) {
    if (g->m.ter(i, 10) == t_wall_glass_v)
     wall_spot = i;
   }
// ...and put radioactive to the right of it
   for (int i = wall_spot + 1; i < SEEX * 2 - 1; i++) {
    for (int j = 1; j < SEEY * 2 - 1; j++) {
     if (one_in(3))
      g->m.add_field(NULL, i, j, fd_nuke_gas, 3);
    }
   }
// For each level between here and the surface, remove the missile
   for (int level = g->cur_om.posz; level < 0; level++) {
    tmp_om = g->cur_om;
    g->cur_om = overmap(g, tmp_om.posx, tmp_om.posy, level);
    tinymap tmpmap(&g->itypes, &g->mapitems, &g->traps);
    tmpmap.load(g, g->levx, g->levy, false);
    tmpmap.translate(t_missile, t_hole);
    tmpmap.save(&tmp_om, g->turn, g->levx, g->levy);
   }
   g->cur_om = tmp_om;
   for (int x = target.x - 2; x <= target.x + 2; x++) {
    for (int y = target.y -  2; y <= target.y + 2; y++)
     g->nuke(x, y);
   }
  } break;

  case COMPACT_MISS_DISARM: // TODO: This!
   break;

  case COMPACT_LIST_BIONICS: {
   std::vector<std::string> names;
   int more = 0;
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++) {
     for (int i = 0; i < g->m.i_at(x, y).size(); i++) {
      if (g->m.i_at(x, y)[i].is_bionic()) {
       if (names.size() < 9)
        names.push_back(g->m.i_at(x, y)[i].tname());
       else
        more++;
      }
     }
    }
   }
   for (int i = 0; i < names.size(); i++)
    print_line(names[i].c_str());
   if (more > 0)
    print_line("%d OTHERS FOUND...");
  } break;

  case COMPACT_ELEVATOR_ON:
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++) {
     if (g->m.ter(x, y) == t_elevator_control_off)
      g->m.ter(x, y) = t_elevator_control;
    }
   }
   print_line("Elevator activated.");
   break;

  case COMPACT_AMIGARA_LOG: // TODO: This is static, move to data file?
   print_line("NEPower Mine(%d:%d) Log", g->levx, g->levy);
   print_line("\
ENTRY 47:\n\
Our normal mining routine has unearthed a hollow chamber.  This would not be\n\
out of the ordinary, save for the odd, perfectly vertical faultline found.\n\
This faultline has several odd concavities in it which have the more\n\
superstitious crew members alarmed; they seem to be of human origin.\n\
\n\
ENTRY 48:\n\
The concavities are between 10 and 20 feet tall, and run the length of the\n\
faultline.  Each one is vaguely human in shape, but with the proportions of\n\
the limbs, neck and head greatly distended, all twisted and curled in on\n\
themselves.\n");
   if (!query_bool("Continue reading?"))
    return;
   reset_terminal();
   print_line("NEPower Mine(%d:%d) Log", g->levx, g->levy);
   print_line("\
ENTRY 49:\n\
We've stopped mining operations in this area, obviously, until archaeologists\n\
have the chance to inspect the area.  This is going to set our schedule back\n\
by at least a week.  This stupid artifact-preservation law has been in place\n\
for 50 years, and hasn't even been up for termination despite the fact that\n\
these mining operations are the backbone of our economy.\n\
\n\
ENTRY 52:\n\
Still waiting on the archaeologists.  We've done a little light insepction of\n\
the faultline; our sounding equipment is insufficient to measure the depth of\n\
the concavities.  The equipment is rated at 15 miles depth, but it isn't made\n\
for such narrow tunnels, so it's hard to say exactly how far back they go.\n");
   if (!query_bool("Continue reading?"))
    return;
   reset_terminal();
   print_line("NEPower Mine(%d:%d) Log", g->levx, g->levy);
   print_line("\
ENTRY 54:\n\
I noticed a couple of the guys down in the chamber with a chisel, breaking\n\
off a piece of the sheer wall.  I'm looking the other way.  It's not like\n\
the eggheads are going to notice a little piece missing.  Fuck em.\n\
\n\
ENTRY 55:\n\
Well, the archaeologists are down there now with a couple of the boys as\n\
guides.  They're hardly Indiana Jones types; I doubt they been below 20\n\
feet.  I hate taking guys off assignment just to babysit the scientists, but\n\
if they get hurt we'll be shut down for god knows how long.\n\
\n\
ENTRY 58:\n\
They're bringing in ANOTHER CREW?  Christ, it's just some cave carvings!  I\n\
know that's sort of a big deal, but come on, these guys can't handle it?\n");
   if (!query_bool("Continue reading?"))
    return;
   reset_terminal();
   for (int i = 0; i < 10; i++)
    print_gibberish_line();
   print_line("");
   print_line("");
   print_line("");
   print_line("AMIGARA PROJECT");
   print_line("");
   print_line("");
   if (!query_bool("Continue reading?"))
    return;
   reset_terminal();
   print_line("\
SITE %d%d%d%d%d\n\
PERTINANT FOREMAN LOGS WILL BE PREPENDED TO NOTES",
g->cur_om.posx, g->cur_om.posy, g->levx, g->levy, abs(g->levz));
   print_line("\n\
MINE OPERATIONS SUSPENDED; CONTROL TRANSFERRED TO AMIGARA PROJECT UNDER\n\
   IMPERATIVE 2:07B\n\
FAULTLINE SOUNDING HAS PLACED DEPTH AT 30.09 KM\n\
DAMAGE TO FAULTLINE DISCOVERED; NEPOWER MINE CREW PLACED UNDER ARREST FOR\n\
   VIOLATION OF REGULATION 87.08 AND TRANSFERRED TO LAB 89-C FOR USE AS\n\
   SUBJECTS\n\
QUALITIY OF FAULTLINE NOT COMPROMISED\n\
INITIATING STANDARD TREMOR TEST...");
   print_gibberish_line();
   print_gibberish_line();
   print_line("");
   print_error("FILE CORRUPTED, PRESS ANY KEY...");
   getch();
   reset_terminal();
   break;

  case COMPACT_AMIGARA_START:
   g->add_event(EVENT_AMIGARA, int(g->turn) + 10, 0, 0, 0);
   if (!g->u.has_artifact_with(AEP_PSYSHIELD))
    g->u.add_disease(DI_AMIGARA, 20, g);
   break;

  case COMPACT_DOWNLOAD_SOFTWARE:
   if (!g->u.has_amount(itm_usb_drive, 1))
    print_error("USB drive required!");
   else {
    mission *miss = g->find_mission(mission_id);
    if (miss == NULL) {
     debugmsg("Computer couldn't find its mission!");
     return;
    }
    item software(g->itypes[miss->item_id], 0);
    software.mission_id = mission_id;
    int index = g->u.pick_usb();
    g->u.inv[index].contents.clear();
    g->u.inv[index].put_in(software);
    print_line("Software downloaded.");
   }
   break;

  case COMPACT_BLOOD_ANAL:
   for (int x = g->u.posx - 2; x <= g->u.posx + 2; x++) {
    for (int y = g->u.posy - 2; y <= g->u.posy + 2; y++) {
     if (g->m.ter(x, y) == t_centrifuge) {
      if (g->m.i_at(x, y).empty())
       print_error("ERROR: Please place sample in centrifuge.");
      else if (g->m.i_at(x, y).size() > 1)
       print_error("ERROR: Please remove all but one sample from centrifuge.");
      else if (g->m.i_at(x, y)[0].type->id != itm_vacutainer)
       print_error("ERROR: Please use vacutainer-contained samples.");
      else if (g->m.i_at(x, y)[0].contents.empty())
       print_error("ERROR: Vacutainer empty.");
      else if (g->m.i_at(x, y)[0].contents[0].type->id != itm_blood)
       print_error("ERROR: Please only use blood samples.");
      else { // Success!
       item *blood = &(g->m.i_at(x, y)[0].contents[0]);
       if (blood->corpse == NULL || blood->corpse->id == mon_null)
        print_line("Result:  Human blood, no pathogens found.");
       else if (blood->corpse->sym == 'Z') {
        print_line("Result:  Human blood.  Unknown pathogen found.");
        print_line("Pathogen bonded to erythrocytes and leukocytes.");
        if (query_bool("Download data?")) {
         if (!g->u.has_amount(itm_usb_drive, 1))
          print_error("USB drive required!");
         else {
          item software(g->itypes[itm_software_blood_data], 0);
          int index = g->u.pick_usb();
          g->u.inv[index].contents.clear();
          g->u.inv[index].put_in(software);
          print_line("Software downloaded.");
         }
        }
       } else
        print_line("Result: Unknown blood type.  Test nonconclusive.");
       print_line("Press any key...");
       getch();
      }
     }
    }
   }
   break;

 } // switch (action)
}

void computer::activate_random_failure(game *g)
{
 computer_failure fail = (failures.empty() ? COMPFAIL_SHUTDOWN :
                          failures[rng(0, failures.size() - 1)]);
 activate_failure(g, fail);
}

void computer::activate_failure(game *g, computer_failure fail)
{
 switch (fail) {

  case COMPFAIL_NULL:
   break;	// Do nothing.  Why was this even called >:|

  case COMPFAIL_SHUTDOWN:
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++) {
     if (g->m.has_flag(console, x, y))
      g->m.ter(x, y) = t_console_broken;
    }
   }
   break;

  case COMPFAIL_ALARM:
   g->sound(g->u.posx, g->u.posy, 60, "An alarm sounds!");
   if (g->levz > 0 && !g->event_queued(EVENT_WANTED))
    g->add_event(EVENT_WANTED, int(g->turn) + 300, 0, g->levx, g->levy);
   break;

  case COMPFAIL_MANHACKS: {
   int num_robots = rng(4, 8);
   for (int i = 0; i < num_robots; i++) {
    int mx, my, tries = 0;
    do {
     mx = rng(g->u.posx - 3, g->u.posx + 3);
     my = rng(g->u.posy - 3, g->u.posy + 3);
     tries++;
    } while (!g->is_empty(mx, my) && tries < 10);
    if (tries != 10) {
     g->add_msg("Manhacks drop from compartments in the ceiling.");
     monster robot(g->mtypes[mon_manhack]);
     robot.spawn(mx, my);
     g->z.push_back(robot);
    }
   }
  } break;

  case COMPFAIL_SECUBOTS: {
   int num_robots = 1;
   for (int i = 0; i < num_robots; i++) {
    int mx, my, tries = 0;
    do {
     mx = rng(g->u.posx - 3, g->u.posx + 3);
     my = rng(g->u.posy - 3, g->u.posy + 3);
     tries++;
    } while (!g->is_empty(mx, my) && tries < 10);
    if (tries != 10) {
     g->add_msg("Secubots emerge from compartments in the floor.");
     monster robot(g->mtypes[mon_secubot]);
     robot.spawn(mx, my);
     g->z.push_back(robot);
    }
   }
  } break;

  case COMPFAIL_DAMAGE:
   g->add_msg("The console electrocutes you!");
   g->u.hurtall(rng(1, 10));
   break;

  case COMPFAIL_PUMP_EXPLODE:
   g->add_msg("The pump explodes!");
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++) {
     if (g->m.ter(x, y) == t_sewage_pump) {
      g->m.ter(x, y) = t_rubble;
      g->explosion(x, y, 10, 0, false);
     }
    }
   }
   break;

  case COMPFAIL_PUMP_LEAK:
   g->add_msg("Sewage leaks!");
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++) {
     if (g->m.ter(x, y) == t_sewage_pump) {
      point p(x, y);
      int leak_size = rng(4, 10);
      for (int i = 0; i < leak_size; i++) {
       std::vector<point> next_move;
       if (g->m.move_cost(p.x, p.y - 1) > 0)
        next_move.push_back( point(p.x, p.y - 1) );
       if (g->m.move_cost(p.x + 1, p.y) > 0)
        next_move.push_back( point(p.x + 1, p.y) );
       if (g->m.move_cost(p.x, p.y + 1) > 0)
        next_move.push_back( point(p.x, p.y + 1) );
       if (g->m.move_cost(p.x - 1, p.y) > 0)
        next_move.push_back( point(p.x - 1, p.y) );

       if (next_move.empty())
        i = leak_size;
       else {
        p = next_move[rng(0, next_move.size() - 1)];
        g->m.ter(p.x, p.y) = t_sewage;
       }
      }
     }
    }
   }
   break;

  case COMPFAIL_AMIGARA:
   g->add_event(EVENT_AMIGARA, int(g->turn) + 5, 0, 0, 0);
   g->u.add_disease(DI_AMIGARA, 20, g);
   g->explosion(rng(0, SEEX * MAPSIZE), rng(0, SEEY * MAPSIZE), 10, 10, false);
   g->explosion(rng(0, SEEX * MAPSIZE), rng(0, SEEY * MAPSIZE), 10, 10, false);
   break;

  case COMPFAIL_DESTROY_BLOOD:
   print_error("ERROR: Disruptive Spin");
   for (int x = g->u.posx - 2; x <= g->u.posx + 2; x++) {
    for (int y = g->u.posy - 2; y <= g->u.posy + 2; y++) {
     if (g->m.ter(x, y) == t_centrifuge) {
      for (int i = 0; i < g->m.i_at(x, y).size(); i++) {
       if (g->m.i_at(x, y).empty())
        print_error("ERROR: Please place sample in centrifuge.");
       else if (g->m.i_at(x, y).size() > 1)
        print_error("ERROR: Please remove all but one sample from centrifuge.");
       else if (g->m.i_at(x, y)[0].type->id != itm_vacutainer)
        print_error("ERROR: Please use vacutainer-contained samples.");
       else if (g->m.i_at(x, y)[0].contents.empty())
        print_error("ERROR: Vacutainer empty.");
       else if (g->m.i_at(x, y)[0].contents[0].type->id != itm_blood)
        print_error("ERROR: Please only use blood samples.");
       else {
        print_error("ERROR: Blood sample destroyed.");
        g->m.i_at(x, y)[i].contents.clear();
       }
      }
     }
    }
   }
   getch();
   break;
 }// switch (fail)
}

bool computer::query_bool(const char *mes, ...)
{
// Translate the printf flags
 va_list ap;
 va_start(ap, mes);
 char buff[6000];
 vsprintf(buff, mes, ap);
 va_end(ap);
// Append with (Y/N/Q)
 std::string full_line = buff;
 full_line += " (Y/N/Q)";
// Print the resulting text
 print_line(full_line.c_str());
 char ret;
 do
  ret = getch();
 while (ret != 'y' && ret != 'Y' && ret != 'n' && ret != 'N' && ret != 'q' &&
        ret != 'Q');
 return (ret == 'y' || ret == 'Y');
}

char computer::query_ynq(const char *mes, ...)
{
// Translate the printf flags
 va_list ap;
 va_start(ap, mes);
 char buff[6000];
 vsprintf(buff, mes, ap);
 va_end(ap);
// Append with (Y/N/Q)
 std::string full_line = buff;
 full_line += " (Y/N/Q)";
// Print the resulting text
 print_line(full_line.c_str());
 char ret;
 do
  ret = getch();
 while (ret != 'y' && ret != 'Y' && ret != 'n' && ret != 'N' && ret != 'q' &&
        ret != 'Q');
 return ret;
}

void computer::print_line(const char *mes, ...)
{
// Translate the printf flags
 va_list ap;
 va_start(ap, mes);
 char buff[6000];
 vsprintf(buff, mes, ap);
 va_end(ap);
// Replace any '\n' with "\n " to allow for the border
 std::string message = buff;
 size_t pos = 0;
 while (pos != std::string::npos) {
  pos = message.find("\n", pos);
  if (pos != std::string::npos) {
   message.replace(pos, 1, "\n ");
   pos += 2;
  }
 }
// Print the line.
 wprintz(w_terminal, c_green, " %s\n", message.c_str());
// Reprint the border, in case we pushed a line over it
 wborder(w_terminal, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                     LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 wrefresh(w_terminal);
}

void computer::print_error(const char *mes, ...)
{
// Translate the printf flags
 va_list ap;
 va_start(ap, mes);
 char buff[6000];
 vsprintf(buff, mes, ap);
 va_end(ap);
// Print the line.
 wprintz(w_terminal, c_red, " %s%s", buff, "\n");
// Reprint the border, in case we pushed a line over it
 wborder(w_terminal, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                     LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 wrefresh(w_terminal);
}

void computer::print_gibberish_line()
{
 std::string gibberish;
 int length = rng(50, 70);
 for (int i = 0; i < length; i++) {
  switch (rng(0, 4)) {
  case 0: gibberish += '0' + rng(0, 9); break;
  case 1:
  case 2: gibberish += 'a' + rng(0, 25); break;
  case 3:
  case 4: gibberish += 'A' + rng(0, 25); break;
  }
 }
 wprintz(w_terminal, c_yellow, gibberish.c_str());
 wborder(w_terminal, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                     LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 wrefresh(w_terminal);
}

void computer::reset_terminal()
{
 werase(w_terminal);
 wborder(w_terminal, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                     LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 wmove(w_terminal, 1, 1);
 wrefresh(w_terminal);
}

