#include "game.h"
#include "output.h"
#include "action.h"
#include "tutorial.h"

bool tutorial_game::init(game *g)
{
 g->turn = HOURS(12); // Start at noon
 for (int i = 0; i < NUM_LESSONS; i++)
  tutorials_seen[i] = false;
// Set the scent map to 0
 for (int i = 0; i < SEEX * MAPSIZE; i++) {
  for (int j = 0; j < SEEX * MAPSIZE; j++)
   g->scent(i, j) = 0;
 }
 g->temperature = 65;
// We use a Z-factor of 10 so that we don't plop down tutorial rooms in the
// middle of the "real" game world
 g->u.normalize(g);
 g->u.str_cur = g->u.str_max;
 g->u.per_cur = g->u.per_max;
 g->u.int_cur = g->u.int_max;
 g->u.dex_cur = g->u.dex_max;
 g->u.name = "John Smith";
 g->levx = 100;
 g->levy = 100;
 g->cur_om = overmap(g, 0, 0, TUTORIAL_Z - 1);
 g->cur_om.make_tutorial();
 g->cur_om.save(g->u.name, 0, 0, TUTORIAL_Z - 1);
 g->cur_om = overmap(g, 0, 0, TUTORIAL_Z);
 g->cur_om.make_tutorial();
 g->u.toggle_trait(PF_QUICK);
 g->u.inv.push_back(item(g->itypes[itm_lighter], 0, 'e'));
 g->u.sklevel[sk_gun] = 5;
 g->u.sklevel[sk_melee] = 5;
// Init the starting map at g location.
 for (int i = 0; i <= MAPSIZE; i += 2) {
  for (int j = 0; j <= MAPSIZE; j += 2) {
   tinymap tm(&g->itypes, &g->mapitems, &g->traps);
   tm.generate(g, &(g->cur_om), g->levx + i - 1, g->levy + j - 1, int(g->turn));
  }
 }
// Start with the overmap revealed
 for (int x = 0; x < OMAPX; x++) {
  for (int y = 0; y < OMAPY; y++)
   g->cur_om.seen(x, y) = true;
 }
 g->m.load(g, g->levx, g->levy);
 g->levz = 0;
 g->u.posx = SEEX + 2;
 g->u.posy = SEEY + 4;

 return true;
}

void tutorial_game::per_turn(game *g)
{
 if (g->turn == HOURS(12)) {
  add_message(g, LESSON_INTRO);
  add_message(g, LESSON_INTRO);
 } else if (g->turn == HOURS(12) + 3)
  add_message(g, LESSON_INTRO);

 if (g->light_level() == 1) {
  if (g->u.has_amount(itm_flashlight, 1))
   add_message(g, LESSON_DARK);
  else
   add_message(g, LESSON_DARK_NO_FLASH);
 }

 if (g->u.pain > 0)
  add_message(g, LESSON_PAIN);

 if (g->u.recoil >= 5)
  add_message(g, LESSON_RECOIL);

 if (!tutorials_seen[LESSON_BUTCHER]) {
  for (int i = 0; i < g->m.i_at(g->u.posx, g->u.posy).size(); i++) {
   if (g->m.i_at(g->u.posx, g->u.posy)[i].type->id == itm_corpse) {
    add_message(g, LESSON_BUTCHER);
    i = g->m.i_at(g->u.posx, g->u.posy).size();
   }
  }
 }

 bool showed_message = false;
 for (int x = g->u.posx - 1; x <= g->u.posx + 1 && !showed_message; x++) {
  for (int y = g->u.posy - 1; y <= g->u.posy + 1 && !showed_message; y++) {
   if (g->m.ter(x, y) == t_door_o) {
    add_message(g, LESSON_OPEN);
    showed_message = true;
   } else if (g->m.ter(x, y) == t_door_c) {
    add_message(g, LESSON_CLOSE);
    showed_message = true;
   } else if (g->m.ter(x, y) == t_window) {
    add_message(g, LESSON_SMASH);
    showed_message = true;
   } else if (g->m.ter(x, y) == t_rack && !g->m.i_at(x, y).empty()) {
    add_message(g, LESSON_EXAMINE);
    showed_message = true;
   } else if (g->m.ter(x, y) == t_stairs_down) {
    add_message(g, LESSON_STAIRS);
    showed_message = true;
   } else if (g->m.ter(x, y) == t_water_sh) {
    add_message(g, LESSON_PICKUP_WATER);
    showed_message = true;
   }
  }
 }

 if (!g->m.i_at(g->u.posx, g->u.posy).empty())
  add_message(g, LESSON_PICKUP);
}

void tutorial_game::pre_action(game *g, action_id &act)
{
}

void tutorial_game::post_action(game *g, action_id act)
{
 switch (act) {
 case ACTION_RELOAD:
  if (g->u.weapon.is_gun() && !tutorials_seen[LESSON_GUN_FIRE]) {
   monster tmp(g->mtypes[mon_zombie], g->u.posx, g->u.posy - 6);
   g->z.push_back(tmp);
   tmp.spawn(g->u.posx + 2, g->u.posy - 5);
   g->z.push_back(tmp);
   tmp.spawn(g->u.posx - 2, g->u.posy - 5);
   g->z.push_back(tmp);
   add_message(g, LESSON_GUN_FIRE);
  }
  break;

 case ACTION_OPEN:
  add_message(g, LESSON_CLOSE);
  break;

 case ACTION_CLOSE:
  add_message(g, LESSON_SMASH);
  break;

 case ACTION_USE:
  if (g->u.has_amount(itm_grenade_act, 1))
   add_message(g, LESSON_ACT_GRENADE);
  for (int x = g->u.posx - 1; x <= g->u.posx + 1; x++) {
   for (int y = g->u.posy - 1; y <= g->u.posy + 1; y++) {
    if (g->m.tr_at(x, y) == tr_bubblewrap)
     add_message(g, LESSON_ACT_BUBBLEWRAP);
   }
  }
  break;

 case ACTION_EAT:
  if (g->u.last_item == itm_codeine)
   add_message(g, LESSON_TOOK_PAINKILLER);
  else if (g->u.last_item == itm_cig)
   add_message(g, LESSON_TOOK_CIG);
  else if (g->u.last_item == itm_water)
   add_message(g, LESSON_DRANK_WATER);
  break;

 case ACTION_WEAR: {
  itype *it = g->itypes[ g->u.last_item];
  if (it->is_armor()) {
   it_armor *armor = dynamic_cast<it_armor*>(it);
   if (armor->dmg_resist >= 2 || armor->cut_resist >= 4)
    add_message(g, LESSON_WORE_ARMOR);
   if (armor->storage >= 20)
    add_message(g, LESSON_WORE_STORAGE);
   if (armor->env_resist >= 2)
    add_message(g, LESSON_WORE_MASK);
  }
 } break;

 case ACTION_WIELD:
  if (g->u.weapon.is_gun())
   add_message(g, LESSON_GUN_LOAD);
  break;

 case ACTION_EXAMINE:
  add_message(g, LESSON_INTERACT);
// Fall through to...
 case ACTION_PICKUP: {
  itype *it = g->itypes[ g->u.last_item ];
  if (it->is_armor())
   add_message(g, LESSON_GOT_ARMOR);
  else if (it->is_gun())
   add_message(g, LESSON_GOT_GUN);
  else if (it->is_ammo())
   add_message(g, LESSON_GOT_AMMO);
  else if (it->is_tool())
   add_message(g, LESSON_GOT_TOOL);
  else if (it->is_food())
   add_message(g, LESSON_GOT_FOOD);
  else if (it->melee_dam > 7 || it->melee_cut > 5)
   add_message(g, LESSON_GOT_WEAPON);

  if (g->u.volume_carried() > g->u.volume_capacity() - 2)
   add_message(g, LESSON_OVERLOADED);
 } break;

 }
}

void tutorial_game::add_message(game *g, tut_lesson lesson)
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
 g->refresh_all();
}
