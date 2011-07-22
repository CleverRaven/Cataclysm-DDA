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
}

computer::computer(std::string Name, int Security)
{
 security = Security;
 name = Name;
 w_terminal = NULL;
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
  reset_terminal();
  print_line("%s - Root Menu", name.c_str());
  for (int i = 0; i < options.size(); i++)
   print_line("%d - %s", i + 1, options[i].name.c_str());
  print_line("Q - Quit and shut down");
 
  char ch;
  do
   ch = getch();
  while (ch != 'q' && ch != 'Q' && (ch < '1' || ch - '1' > options.size()));
  if (ch == 'q' || ch == 'Q')
   done = true;
  else { // We selected an option other than quit.
   ch -= '1'; // So '1' -> 0; index in options.size()
   computer_option current = options[ch];
   if (current.security > 0) {
    print_error("Password required.");
    if (query_ynq("Hack into system?")) {
     if (!hack_attempt(&(g->u), current.security)) {
      activate_random_failure(g);
      shutdown_terminal();
      return;
     } else {
      activate_function(g, current.action);
      done = true;
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

 p->practice(sk_computer, Security * 2);
 int player_roll = p->sklevel[sk_computer];
 if (p->int_cur < 8)
  player_roll -= rng(0, 8 - p->int_cur);
 else if (p->int_cur > 8)
  player_roll += rng(0, p->int_cur - 8);

 return (rng(player_roll, 6) >= rng(Security, 6));
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
 data << savename << " " << security << " " << options.size() << " ";
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
 dump >> name >> security;
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

  case COMPACT_RELEASE:
   g->sound(g->u.posx, g->u.posy, 40, "An alarm sounds!");
   g->m.translate(t_reinforced_glass_h, t_floor);
   g->m.translate(t_reinforced_glass_v, t_floor);
   print_line("Containment shields opened.");
   break;

  case COMPACT_TERMINATE:
   for (int x = 0; x < SEEX * 3; x++) {
    for (int y = 0; y < SEEY * 3; y++) {
     int mondex = g->mon_at(x, y);
     if (mondex != -1 &&
         ((g->m.ter(x, y - 1) == t_reinforced_glass_h &&
           g->m.ter(x, y + 1) == t_wall_h) ||
          (g->m.ter(x, y + 1) == t_reinforced_glass_h &&
           g->m.ter(x, y - 1) == t_wall_h)))
      g->kill_mon(mondex);
    }
   }
   print_line("Subjects terminated.");
   break;

  case COMPACT_PORTAL:
   for (int i = 0; i < SEEX * 3; i++) {
    for (int j = 0; j < SEEY * 3; j++) {
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
   if (!query_yn("WARNING: Resonance cascade carries severe risk!  Continue?"))
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
    bool get_okay;
    getline(fin, tmp);
    do {
     lines++;
     if (lines < 15 && tmp.find_first_of('%') != 0) {
      log.append(tmp);
      log.append("\n");
     }
    } while(tmp.find_first_of('%') != 0 && getline(fin, tmp));
   }
   print_line(log.c_str());
   print_line("Press any key...");
   getch();
  } break;

  case COMPACT_MAPS: {
   int minx = int(g->levx / 2) - 40;
   int maxx = int(g->levx / 2) + 40;
   int miny = int(g->levy / 2) - 40;
   int maxy = int(g->levy / 2) + 40;
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
   for (int i = 0; i < SEEX * 3 && wall_spot == 0; i++) {
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
    map tmpmap(&g->itypes, &g->mapitems, &g->traps);
    tmpmap.load(g, g->levx, g->levy);
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
   for (int x = 0; x < SEEX * 3; x++) {
    for (int y = 0; y < SEEY * 3; y++) {
     if (g->m.has_flag(console, x, y))
      g->m.ter(x, y) = t_console_broken;
    }
   }
   break;

  case COMPFAIL_ALARM:
   g->sound(g->u.posx, g->u.posy, 60, "An alarm sounds!");
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
   int num_robots = rng(4, 8);
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
 }// switch (fail)
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
// Print the line.
 wprintz(w_terminal, c_green, " %s%s", buff, "\n");
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

void computer::reset_terminal()
{
 werase(w_terminal);
 wborder(w_terminal, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                     LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 wmove(w_terminal, 1, 1);
 wrefresh(w_terminal);
}

