//
//  iexamine.cpp
//  Cataclysm
//
//  Livingstone
//

#include "item_factory.h"
#include "iuse.h"
#include "game.h"
#include "mapdata.h"
#include "keypress.h"
#include "output.h"
#include "rng.h"
#include "line.h"
#include "player.h"
#include "translations.h"
#include <sstream>
#include <algorithm>

void iexamine::none	(game *g, player *p, map *m, int examx, int examy) {
 g->add_msg(_("That is a %s."), m->name(examx, examy).c_str());
};

void iexamine::gaspump(game *g, player *p, map *m, int examx, int examy) {
 if (!query_yn(_("Use the %s?"),m->tername(examx, examy).c_str())) {
  none(g, p, m, examx, examy);
  return;
 }

 for (int i = 0; i < m->i_at(examx, examy).size(); i++) {
  if (m->i_at(examx, examy)[i].made_of(LIQUID)) {
   item* liq = &(m->i_at(examx, examy)[i]);

   if (one_in(10 + p->dex_cur)) {
    g->add_msg(_("You accidentally spill the %s."), liq->type->name.c_str());
    item spill(liq->type, g->turn);
    spill.charges = rng(dynamic_cast<it_ammo*>(liq->type)->count,
                        dynamic_cast<it_ammo*>(liq->type)->count * (float)(8 / p->dex_cur));
    m->add_item_or_charges(p->posx, p->posy, spill, 1);
    liq->charges -= spill.charges;
    if (liq->charges < 1) {
     m->i_at(examx, examy).erase(m->i_at(examx, examy).begin() + i);
    }
   } else {
    p->moves -= 300;
    if (g->handle_liquid(*liq, true, false)) {
     g->add_msg(_("With a clang and a shudder, the %s pump goes silent."), liq->type->name.c_str());
     m->i_at(examx, examy).erase(m->i_at(examx, examy).begin() + i);
    }
   }
   return;
  }
 }
 g->add_msg(_("Out of order."));
}

void iexamine::toilet(game *g, player *p, map *m, int examx, int examy) {
    std::vector<item>& items = m->i_at(examx, examy);
    int waterIndex = -1;
    for (int i = 0; i < items.size(); i++) {
        if (items[i].typeId() == "water") {
            waterIndex = i;
            break;
        }
    }

    if (waterIndex < 0) {
        g->add_msg(_("This toilet is empty."));
    } else {
        bool drained = false;

        item& water = items[waterIndex];
        // Use a different poison value each time water is drawn from the toilet.
        water.poison = one_in(3) ? 0 : rng(1, 3);

        // First try handling/bottling, then try drinking.
        if (g->handle_liquid(water, true, false))
        {
            p->moves -= 100;
            drained = true;
        }
        else if (query_yn(_("Drink from your hands?")))
        {
            // Create a dose of water no greater than the amount of water remaining.
            item water_temp(item_controller->find_template("water"), 0);
            water_temp.poison = water.poison;
            water_temp.charges = std::min(water_temp.charges, water.charges);

            p->inv.push_back(water_temp);
            water_temp = p->inv.item_by_type(water_temp.typeId());
            p->eat(g, water_temp.invlet);
            p->moves -= 350;

            water.charges -= water_temp.charges;
            if (water.charges <= 0) {
                drained = true;
            }
        }

        if (drained) {
            items.erase(items.begin() + waterIndex);
        }
    }
}

void iexamine::elevator(game *g, player *p, map *m, int examx, int examy){
 if (!query_yn(_("Use the %s?"),m->tername(examx, examy).c_str())) return;
 int movez = (g->levz < 0 ? 2 : -2);
 g->vertical_move( movez, false );
}

void iexamine::controls_gate(game *g, player *p, map *m, int examx, int examy) {
 if (!query_yn(_("Use the %s?"),m->tername(examx, examy).c_str())) {
  none(g, p, m, examx, examy);
  return;
 }
 g->open_gate(g,examx,examy, m->ter(examx,examy));
}

void iexamine::cardreader(game *g, player *p, map *m, int examx, int examy) {
 itype_id card_type = (m->ter(examx, examy) == t_card_science ? "id_science" :
                       "id_military");
 if (p->has_amount(card_type, 1) && query_yn(_("Swipe your ID card?"))) {
  p->moves -= 100;
  for (int i = -3; i <= 3; i++) {
   for (int j = -3; j <= 3; j++) {
    if (m->ter(examx + i, examy + j) == t_door_metal_locked)
     m->ter_set(examx + i, examy + j, t_floor);
     }
  }
  for (int i = 0; i < g->num_zombies(); i++) {
   if (g->zombie(i).type->id == mon_turret) {
    g->remove_zombie(i);
    i--;
   }
  }
  g->add_msg(_("You insert your ID card."));
  g->add_msg(_("The nearby doors slide into the floor."));
  p->use_amount(card_type, 1);
 } else {
  bool using_electrohack = (p->has_amount("electrohack", 1) &&
                            query_yn(_("Use electrohack on the reader?")));
  bool using_fingerhack = (!using_electrohack && p->has_bionic("bio_fingerhack") &&
                           p->power_level > 0 &&
                           query_yn(_("Use fingerhack on the reader?")));
  if (using_electrohack || using_fingerhack) {
   p->moves -= 500;
   p->practice(g->turn, "computer", 20);
   int success = rng(p->skillLevel("computer") / 4 - 2, p->skillLevel("computer") * 2);
   success += rng(-3, 3);
   if (using_fingerhack)
    success++;
   if (p->int_cur < 8)
    success -= rng(0, int((8 - p->int_cur) / 2));
    else if (p->int_cur > 8)
     success += rng(0, int((p->int_cur - 8) / 2));
     if (success < 0) {
      g->add_msg(_("You cause a short circuit!"));
      if (success <= -5) {
       if (using_electrohack) {
        g->add_msg(_("Your electrohack is ruined!"));
        p->use_amount("electrohack", 1);
       } else {
        g->add_msg(_("Your power is drained!"));
        p->charge_power(0 - rng(0, p->power_level));
       }
      }
      m->ter_set(examx, examy, t_card_reader_broken);
     } else if (success < 6)
      g->add_msg(_("Nothing happens."));
      else {
       g->add_msg(_("You activate the panel!"));
       g->add_msg(_("The nearby doors slide into the floor."));
       m->ter_set(examx, examy, t_card_reader_broken);
       for (int i = -3; i <= 3; i++) {
        for (int j = -3; j <= 3; j++) {
         if (m->ter(examx + i, examy + j) == t_door_metal_locked)
          m->ter_set(examx + i, examy + j, t_floor);
          }
       }
      }
  } else {
   g->add_msg(_("Looks like you need a %s."),g->itypes[card_type]->name.c_str());
  }
 }
}

void iexamine::rubble(game *g, player *p, map *m, int examx, int examy) {
 if (!(p->has_amount("shovel", 1) || p->has_amount("primitive_shovel", 1))) {
  g->add_msg(_("If only you had a shovel..."));
  return;
 }
 const char *xname = m->tername(examx, examy).c_str();
 if (query_yn(_("Clear up that %s?"),xname)) {
   // "Remove"
  p->moves -= 200;

   // "Replace"
  if(m->ter(examx,examy) == t_rubble) {
   item rock(g->itypes["rock"], g->turn);
   m->add_item(p->posx, p->posy, rock);
   m->add_item(p->posx, p->posy, rock);
  }

   // "Refloor"
  if (g->levz < 0) {
   m->ter_set(examx, examy, t_rock_floor);
  } else {
   m->ter_set(examx, examy, t_dirt);
  }

   // "Remind"
  g->add_msg(_("You clear up that %s."), xname);
 }
}

void iexamine::chainfence(game *g, player *p, map *m, int examx, int examy) {
 if (!query_yn(_("Climb %s?"),m->tername(examx, examy).c_str())) {
  none(g, p, m, examx, examy);
  return;
 }
 p->moves -= 400;
 if (one_in(p->dex_cur)) {
  g->add_msg(_("You slip whilst climbing and fall down again"));
 } else {
  p->moves += p->dex_cur * 10;
  p->posx = examx;
  p->posy = examy;
 }
}

void iexamine::tent(game *g, player *p, map *m, int examx, int examy) {
 if (!query_yn(_("Take down your tent?"))) {
  none(g, p, m, examx, examy);
  return;
 }
 p->moves -= 200;
 for (int i = -1; i <= 1; i++)
  for (int j = -1; j <= 1; j++)
   m->furn_set(examx + i, examy + j, f_null);
 g->add_msg(_("You take down the tent"));
 item dropped(g->itypes["tent_kit"], g->turn);
 m->add_item(examx, examy, dropped);
}

void iexamine::shelter(game *g, player *p, map *m, int examx, int examy) {
 if (!query_yn(_("Take down %s?"),m->furnname(examx, examy).c_str())) {
  none(g, p, m, examx, examy);
  return;
 }
 p->moves -= 200;
 for (int i = -1; i <= 1; i++)
  for (int j = -1; j <= 1; j++)
   m->furn_set(examx + i, examy + j, f_null);
 g->add_msg(_("You take down the shelter"));
 item dropped(g->itypes["shelter_kit"], g->turn);
 m->add_item(examx, examy, dropped);
}

void iexamine::wreckage(game *g, player *p, map *m, int examx, int examy) {
 if (!(p->has_amount("shovel", 1) || p->has_amount("primitive_shovel", 1))) {
  g->add_msg(_("If only you had a shovel..."));
  return;
 }

 if (query_yn(_("Clear up that wreckage?"))) {
  p->moves -= 200;
  m->ter_set(examx, examy, t_dirt);
  item chunk(g->itypes["steel_chunk"], g->turn);
  item scrap(g->itypes["scrap"], g->turn);
  item pipe(g->itypes["pipe"], g->turn);
  item wire(g->itypes["wire"], g->turn);
  m->add_item(examx, examy, chunk);
  m->add_item(examx, examy, scrap);
  if (one_in(5)) {
   m->add_item(examx, examy, pipe);
   m->add_item(examx, examy, wire); }
  g->add_msg(_("You clear the wreckage up"));
 }
}

void iexamine::pit(game *g, player *p, map *m, int examx, int examy)
{
    inventory map_inv;
    map_inv.form_from_map(g, point(p->posx, p->posy), 1);

    bool player_has = p->has_amount("2x4", 1);
    bool map_has = map_inv.has_amount("2x4", 1);

    // return if there is no 2x4 around
    if (!player_has && !map_has)
    {
        none(g, p, m, examx, examy);
        return;
    }

    if (query_yn(_("Place a plank over the pit?")))
    {
        // if both have, then ask to use the one on the map
        if (player_has && map_has)
        {
            if (query_yn(_("Use the plank at your feet?")))
            {
                m->use_amount(point(p->posx, p->posy), 1, "2x4", 1, false);
            }
            else
            {
                p->use_amount("2x4", 1);
            }
        }
        else if (player_has && !map_has)    // only player has plank
        {
            p->use_amount("2x4", 1);
        }
        else if (!player_has && map_has)    // only map has plank
        {
            m->use_amount(point(p->posx, p->posy), 1, "2x4", 1, false);
        }

        if( m->ter(examx, examy) == t_pit )
        {
            m->ter_set(examx, examy, t_pit_covered);
        }
        else if( m->ter(examx, examy) == t_pit_spiked )
        {
            m->ter_set(examx, examy, t_pit_spiked_covered);
        }
        g->add_msg(_("You place a plank of wood over the pit."));
    }
}

void iexamine::pit_covered(game *g, player *p, map *m, int examx, int examy)
{
    if(!query_yn(_("Remove cover?")))
    {
        none(g, p, m, examx, examy);
        return;
    }

    item plank(g->itypes["2x4"], g->turn);
    g->add_msg(_("You remove the plank."));
    m->add_item(p->posx, p->posy, plank);

    if( m->ter(examx, examy) == t_pit_covered )
    {
        m->ter_set(examx, examy, t_pit);
    }
    else if( m->ter(examx, examy) == t_pit_spiked_covered )
    {
        m->ter_set(examx, examy, t_pit_spiked);
    }
}

void iexamine::fence_post(game *g, player *p, map *m, int examx, int examy) {

 int ch = menu(true, _("Fence Construction:"), _("Rope Fence"),
               _("Wire Fence"), _("Barbed Wire Fence"), _("Cancel"), NULL);
 switch (ch){
  case 1:{
   if (p->has_amount("rope_6", 2)) {
    p->use_amount("rope_6", 2);
    m->ter_set(examx, examy, t_fence_rope);
    p->moves -= 200;
   } else
    g->add_msg(_("You need 2 six-foot lengths of rope to do that"));
  } break;

  case 2:{
   if (p->has_amount("wire", 2)) {
    p->use_amount("wire", 2);
    m->ter_set(examx, examy, t_fence_wire);
    p->moves -= 200;
   } else
    g->add_msg(_("You need 2 lengths of wire to do that!"));
  } break;

  case 3:{
   if (p->has_amount("wire_barbed", 2)) {
    p->use_amount("wire_barbed", 2);
    m->ter_set(examx, examy, t_fence_barbed);
    p->moves -= 200;
   } else
    g->add_msg(_("You need 2 lengths of barbed wire to do that!"));
  } break;

  case 4:
  default:
   break;
 }
}

void iexamine::remove_fence_rope(game *g, player *p, map *m, int examx, int examy) {
 if(!query_yn(_("Remove %s?"),m->tername(examx, examy).c_str())) {
  none(g, p, m, examx, examy);
  return;
 }
 item rope(g->itypes["rope_6"], g->turn);
 m->add_item(p->posx, p->posy, rope);
 m->add_item(p->posx, p->posy, rope);
 m->ter_set(examx, examy, t_fence_post);
 p->moves -= 200;

}

void iexamine::remove_fence_wire(game *g, player *p, map *m, int examx, int examy) {
 if(!query_yn(_("Remove %s?"),m->tername(examx, examy).c_str())) {
  none(g, p, m, examx, examy);
  return;
 }

 item rope(g->itypes["wire"], g->turn);
 m->add_item(p->posx, p->posy, rope);
 m->add_item(p->posx, p->posy, rope);
 m->ter_set(examx, examy, t_fence_post);
 p->moves -= 200;
}

void iexamine::remove_fence_barbed(game *g, player *p, map *m, int examx, int examy) {
 if(!query_yn(_("Remove %s?"),m->tername(examx, examy).c_str())) {
  none(g, p, m, examx, examy);
  return;
 }

 item rope(g->itypes["wire_barbed"], g->turn);
 m->add_item(p->posx, p->posy, rope);
 m->add_item(p->posx, p->posy, rope);
 m->ter_set(examx, examy, t_fence_post);
 p->moves -= 200;
}

void iexamine::slot_machine(game *g, player *p, map *m, int examx, int examy) {
 if (p->cash < 10)
  g->add_msg(_("You need $10 to play."));
 else if (query_yn(_("Insert $10?"))) {
  do {
   if (one_in(5))
    popup(_("Three cherries... you get your money back!"));
   else if (one_in(20)) {
    popup(_("Three bells... you win $50!"));
    p->cash += 40;	// Minus the $10 we wagered
   } else if (one_in(50)) {
    popup(_("Three stars... you win $200!"));
    p->cash += 190;
   } else if (one_in(1000)) {
    popup(_("JACKPOT!  You win $5000!"));
    p->cash += 4990;
   } else {
    popup(_("No win."));
    p->cash -= 10;
   }
  } while (p->cash >= 10 && query_yn(_("Play again?")));
 }
}

void iexamine::safe(game *g, player *p, map *m, int examx, int examy) {
  if (!p->has_amount("stethoscope", 1)) {
    g->add_msg(_("You need a stethoscope for safecracking."));
    return;
  }

  if (query_yn(_("Attempt to crack the safe?"))) {
    bool success = true;

    if (success) {
      m->furn_set(examx, examy, f_safe_o);
      g->add_msg(_("You successfully crack the safe!"));
    } else {
      g->add_msg(_("The safe resists your attempt at cracking it."));
    }
  }
}

void iexamine::bulletin_board(game *g, player *p, map *m, int examx, int examy) {
 basecamp *camp = m->camp_at(examx, examy);
 if (camp && camp->board_x() == examx && camp->board_y() == examy) {
  std::vector<std::string> options;
  options.push_back(_("Cancel"));
  // Causes a warning due to being unused, but don't want to delete since
  // it's clearly what's intened for future functionality.
  //int choice = menu_vec(true, camp->board_name().c_str(), options) - 1;
 }
 else {
  bool create_camp = m->allow_camp(examx, examy);
  std::vector<std::string> options;
  if (create_camp)
   options.push_back(_("Create camp"));
  options.push_back(_("Cancel"));
 		// TODO: Other Bulletin Boards
  int choice = menu_vec(true, _("Bulletin Board"), options) - 1;
  if (choice >= 0 && choice < options.size()) {
   if (options[choice] == _("Create camp")) {
  			// TODO: Allow text entry for name
    m->add_camp(_("Home"), examx, examy);
   }
  }
 }
}

void iexamine::fault(game *g, player *p, map *m, int examx, int examy) {
 popup(_("\
This wall is perfectly vertical.  Odd, twisted holes are set in it, leading\n\
as far back into the solid rock as you can see.  The holes are humanoid in\n\
shape, but with long, twisted, distended limbs."));
}

void iexamine::pedestal_wyrm(game *g, player *p, map *m, int examx, int examy) {
 if (!m->i_at(examx, examy).empty()) {
  none(g, p, m, examx, examy);
  return;
 }
 g->add_msg(_("The pedestal sinks into the ground..."));
 m->ter_set(examx, examy, t_rock_floor);
 g->add_event(EVENT_SPAWN_WYRMS, int(g->turn) + rng(5, 10));
}

void iexamine::pedestal_temple(game *g, player *p, map *m, int examx, int examy) {

 if (m->i_at(examx, examy).size() == 1 &&
     m->i_at(examx, examy)[0].type->id == "petrified_eye") {
  g->add_msg(_("The pedestal sinks into the ground..."));
  m->ter_set(examx, examy, t_dirt);
  m->i_at(examx, examy).clear();
  g->add_event(EVENT_TEMPLE_OPEN, int(g->turn) + 4);
 } else if (p->has_amount("petrified_eye", 1) &&
            query_yn(_("Place your petrified eye on the pedestal?"))) {
  p->use_amount("petrified_eye", 1);
  g->add_msg(_("The pedestal sinks into the ground..."));
  m->ter_set(examx, examy, t_dirt);
  g->add_event(EVENT_TEMPLE_OPEN, int(g->turn) + 4);
 } else
  g->add_msg(_("This pedestal is engraved in eye-shaped diagrams, and has a \
large semi-spherical indentation at the top."));
}

void iexamine::fswitch(game *g, player *p, map *m, int examx, int examy) {
 if(!query_yn(_("Flip the %s?"),m->tername(examx, examy).c_str())) {
  none(g, p, m, examx, examy);
  return;
 }

  p->moves -= 100;
  for (int y = examy; y <= examy + 5; y++) {
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    switch (m->ter(examx, examy)) {
     case t_switch_rg:
      if (m->ter(x, y) == t_rock_red)
       m->ter_set(x, y, t_floor_red);
       else if (m->ter(x, y) == t_floor_red)
        m->ter_set(x, y, t_rock_red);
        else if (m->ter(x, y) == t_rock_green)
         m->ter_set(x, y, t_floor_green);
         else if (m->ter(x, y) == t_floor_green)
          m->ter_set(x, y, t_rock_green);
          break;
     case t_switch_gb:
      if (m->ter(x, y) == t_rock_blue)
       m->ter_set(x, y, t_floor_blue);
       else if (m->ter(x, y) == t_floor_blue)
        m->ter_set(x, y, t_rock_blue);
        else if (m->ter(x, y) == t_rock_green)
         m->ter_set(x, y, t_floor_green);
         else if (m->ter(x, y) == t_floor_green)
          m->ter_set(x, y, t_rock_green);
          break;
     case t_switch_rb:
      if (m->ter(x, y) == t_rock_blue)
       m->ter_set(x, y, t_floor_blue);
       else if (m->ter(x, y) == t_floor_blue)
        m->ter_set(x, y, t_rock_blue);
        else if (m->ter(x, y) == t_rock_red)
         m->ter_set(x, y, t_floor_red);
         else if (m->ter(x, y) == t_floor_red)
          m->ter_set(x, y, t_rock_red);
          break;
     case t_switch_even:
      if ((y - examy) % 2 == 1) {
       if (m->ter(x, y) == t_rock_red)
        m->ter_set(x, y, t_floor_red);
        else if (m->ter(x, y) == t_floor_red)
         m->ter_set(x, y, t_rock_red);
         else if (m->ter(x, y) == t_rock_green)
          m->ter_set(x, y, t_floor_green);
          else if (m->ter(x, y) == t_floor_green)
           m->ter_set(x, y, t_rock_green);
           else if (m->ter(x, y) == t_rock_blue)
            m->ter_set(x, y, t_floor_blue);
            else if (m->ter(x, y) == t_floor_blue)
             m->ter_set(x, y, t_rock_blue);
             }
      break;
    }
   }
  }
  g->add_msg(_("You hear the rumble of rock shifting."));
  g->add_event(EVENT_TEMPLE_SPAWN, g->turn + 3);
}

void iexamine::flower_poppy(game *g, player *p, map *m, int examx, int examy) {
  if(!query_yn(_("Pick %s?"),m->furnname(examx, examy).c_str())) {
    none(g, p, m, examx, examy);
    return;
  }

  int resist = p->resist(bp_mouth);

  if (resist < 10) {
    // Can't smell the flowers with a gas mask on!
    g->add_msg(_("This flower has a heady aroma"));
  }

  if (one_in(3) && resist < 5)  {
    // Should user player::infect, but can't!
    // player::infect needs to be restructured to return a bool indicating success.
    g->add_msg(_("You fall asleep..."));
    p->add_disease("sleep", 1200);
    g->add_msg(_("Your legs are covered by flower's roots!"));
    p->hurt(g,bp_legs, 0, 4);
    p->moves-=50;
  }

  m->furn_set(examx, examy, f_null);
  m->spawn_item(examx, examy, "poppy_flower", 0);
  m->spawn_item(examx, examy, "poppy_bud", 0);
}

void iexamine::dirtmound(game *g, player *p, map *m, int examx, int examy) {

    if (g->get_temperature() < 50) { // semi-appropriate temperature for most plants
        g->add_msg(_("It is too cold to plant anything now."));
        return;
    }
    /* ambient_light_at() not working?
    if (m->ambient_light_at(examx, examy) < LIGHT_AMBIENT_LOW) {
        g->add_msg(_("It is too dark to plant anything now."));
        return;
    }*/
    if (!p->has_item_with_flag("SEED")){
        g->add_msg(_("You have no seeds to plant."));
        return;
    }
    if (m->i_at(examx, examy).size() != 0){
        g->add_msg(_("Something's lying there..."));
        return;
    }

    // Get list of all inv+wielded seeds
    std::vector<item*> seed_inv = p->inv.all_items_with_flag("SEED");
    if (g->u.weapon.has_flag("SEED"))
        seed_inv.push_back(&g->u.weapon);

    // Make lists of unique seed types and names for the menu(no multiple hemp seeds etc)
    std::vector<itype_id> seed_types;
    std::vector<std::string> seed_names;
    for (std::vector<item*>::iterator it = seed_inv.begin() ; it != seed_inv.end(); it++){
        if (std::find(seed_types.begin(), seed_types.end(), (*it)->typeId()) == seed_types.end()){
            seed_types.push_back((*it)->typeId());
            seed_names.push_back((*it)->name);
        }
    }

    // Choose seed if applicable
    int seed_index = 0;
    if (seed_types.size() > 1) {
        seed_names.push_back("Cancel");
        seed_index = menu_vec(false, _("Use which seed?"), seed_names) - 1; // TODO: make cancelable using ESC
        if (seed_index == seed_names.size() - 1)
            seed_index = -1;
    } else {
        if (!query_yn(_("Plant %s here?"), seed_names[0].c_str()))
            seed_index = -1;
    }

    // Did we cancel?
    if (seed_index < 0) {
        g->add_msg(_("You saved your seeds for later.")); // huehuehue
        return;
    }

    // Actual planting
    std::list<item> planted = p->inv.use_charges(seed_types[seed_index], 1);
    if (planted.empty()) { // nothing was removed from inv => weapon is the SEED
        if (g->u.weapon.charges > 1) {
            g->u.weapon.charges--;
        } else {
            g->u.remove_weapon();
        }
    }
    m->spawn_item(examx, examy, seed_types[seed_index], g->turn, 1, 1);
    m->set(examx, examy, t_dirt, f_plant_seed);
    p->moves -= 500;
    g->add_msg(_("Planted %s"), seed_names[seed_index].c_str());
}

void iexamine::aggie_plant(game *g, player *p, map *m, int examx, int examy) {
  if (m->furn(examx, examy) == f_plant_harvest && query_yn(_("Harvest plant?"))) {
    itype_id seedType = m->i_at(examx, examy)[0].typeId();

    m->i_clear(examx, examy);
    m->furn_set(examx, examy, f_null);

    int skillLevel = p->skillLevel("survival");
    int plantCount = rng(skillLevel / 2, skillLevel);
    if (plantCount >= 12)
      plantCount = 12;

    m->spawn_item(examx, examy, seedType.substr(5), g->turn, plantCount);
    m->spawn_item(examx, examy, seedType, 0, 1, rng(plantCount / 4, plantCount / 2));

    p->moves -= 500;
  } else if (m->furn(examx,examy) != f_plant_harvest && m->i_at(examx, examy).size() == 1 && p->charges_of("fertilizer_liquid") && query_yn(_("Fertilize plant"))) {
    unsigned int fertilizerEpoch = 14400 * 2;

    if (m->i_at(examx, examy)[0].bday > fertilizerEpoch) {
      m->i_at(examx, examy)[0].bday -= fertilizerEpoch;
    } else {
      m->i_at(examx, examy)[0].bday = 0;
    }

    p->use_charges("fertilizer_liquid", 1);
    m->spawn_item(examx, examy, "fertilizer", 0, 1, 1);
  }
}

void iexamine::pick_plant(game *g, player *p, map *m, int examx, int examy, std::string itemType, int new_ter, bool seeds) {
  if (!query_yn(_("Pick %s?"), m->tername(examx, examy).c_str())) {
    none(g, p, m, examx, examy);
    return;
  }

  SkillLevel& survival = p->skillLevel("survival");
  if (survival < 1)
    p->practice(g->turn, "survival", rng(5, 12));
  else if (survival < 6)
    p->practice(g->turn, "survival", rng(1, 12 / survival));

  int plantCount = rng(survival / 2, survival);
  if (plantCount > 12)
    plantCount = 12;

  m->spawn_item(examx, examy, itemType, g->turn, plantCount);

  if (seeds) {
    m->spawn_item(examx, examy, "seed_" + itemType, g->turn, 1, rng(plantCount / 4, plantCount / 2));
  }

  m->ter_set(examx, examy, (ter_id)new_ter);
}

void iexamine::tree_apple(game *g, player *p, map *m, int examx, int examy) {
  pick_plant(g, p, m, examx, examy, "apple", t_tree);
}

void iexamine::shrub_blueberry(game *g, player *p, map *m, int examx, int examy) {
  pick_plant(g, p, m, examx, examy, "blueberries", t_shrub, true);
}

void iexamine::shrub_strawberry(game *g, player *p, map *m, int examx, int examy) {
  pick_plant(g, p, m, examx, examy, "strawberries", t_shrub, true);
}

void iexamine::shrub_wildveggies(game *g, player *p, map *m, int examx, int examy) {
 if(!query_yn(_("Pick %s?"),m->tername(examx, examy).c_str())) return;

 p->assign_activity(g, ACT_FORAGE, 500 / (p->skillLevel("survival") + 1), 0);
 p->activity.placement = point(examx, examy);
 p->moves = 0;
}

void iexamine::recycler(game *g, player *p, map *m, int examx, int examy) {
    int ch = menu(true, _("Recycle metal into?:"), _("Lumps"), _("Sheets"),
                  _("Chunks"), _("Scraps"), _("Cancel"), NULL);

    // check for how much steel, by weight, is in the recycler
    // only items made of STEEL are checked
    // IRON and other metals cannot be turned into STEEL for now

    int steel_weight = 0;
    int num_lumps = 0;
    int num_sheets = 0;
    int num_chunks = 0;
    int num_scraps = 0;

    if (m->i_at(examx, examy).size() == 0)
    {
        g->add_msg(_("The recycler is currently empty.  Drop some metal items onto it and examine it again."));
        return;
    }

    if (ch == 5)
    {
        g->add_msg(_("Never mind."));
        return;
    }

    for (int i = 0; i < m->i_at(examx, examy).size(); i++)
    {
        item *it = &(m->i_at(examx, examy)[i]);
        if (it->made_of("steel"))
            steel_weight += it->weight();
        m->i_at(examx, examy).erase(m->i_at(examx, examy).begin() + i);
        i--;
    }

    double recover_factor = rng(6, 9) / 10.0;
    steel_weight = (int)(steel_weight * recover_factor);

    if (steel_weight < 113)
    {
        g->add_msg(_("The recycler chews up all the items in its hopper."));
        g->add_msg(_("The recycler beeps: \"No steel to process!\""));
        return;
    }

    g->sound(examx, examy, 80, _("Ka-klunk!"));

    int lump_weight = item_controller->find_template("steel_lump")->weight;
    int sheet_weight = item_controller->find_template("sheet_metal")->weight;
    int chunk_weight = item_controller->find_template("steel_chunk")->weight;
    int scrap_weight = item_controller->find_template("scrap")->weight;

    switch(ch)
    {
        case 1: // 1 steel lump = weight 1360
            num_lumps = steel_weight / (lump_weight);
            steel_weight -= num_lumps * (lump_weight);
            num_sheets = steel_weight / (sheet_weight);
            steel_weight -= num_sheets * (sheet_weight);
            num_chunks = steel_weight / (chunk_weight);
            steel_weight -= num_chunks * (chunk_weight);
            num_scraps = steel_weight / (scrap_weight);
            if (num_lumps == 0)
            {
                g->add_msg(_("The recycler beeps: \"Insufficient steel!\""));
                g->add_msg(_("It spits out an assortment of smaller pieces instead."));
            }
            break;

        case 2: // 1 metal sheet = weight 1000
            num_sheets = steel_weight / (sheet_weight);
            steel_weight -= num_sheets * (sheet_weight);
            num_chunks = steel_weight / (chunk_weight);
            steel_weight -= num_chunks * (chunk_weight);
            num_scraps = steel_weight / (scrap_weight);
            if (num_sheets == 0)
            {
                g->add_msg(_("The recycler beeps: \"Insufficient steel!\""));
                g->add_msg(_("It spits out an assortment of smaller pieces instead."));
            }
            break;

        case 3: // 1 steel chunk = weight 340
            num_chunks = steel_weight / (chunk_weight);
            steel_weight -= num_chunks * (chunk_weight);
            num_scraps = steel_weight / (scrap_weight);
            if (num_chunks == 0)
            {
                g->add_msg(_("The recycler beeps: \"Insufficient steel!\""));
                g->add_msg(_("It spits out an assortment of smaller pieces instead."));
            }
            break;

        case 4: // 1 metal scrap = weight 113
            num_scraps = steel_weight / (scrap_weight);
            break;
    }

    for (int i = 0; i < num_lumps; i++)
    {
        m->spawn_item(p->posx, p->posy, "steel_lump", 0);
    }

    for (int i = 0; i < num_sheets; i++)
    {
        m->spawn_item(p->posx, p->posy, "sheet_metal", 0);
    }

    for (int i = 0; i < num_chunks; i++)
    {
        m->spawn_item(p->posx, p->posy, "steel_chunk", 0);
    }

    for (int i = 0; i < num_scraps; i++)
    {
        m->spawn_item(p->posx, p->posy, "scrap", 0);
    }
}

void iexamine::trap(game *g, player *p, map *m, int examx, int examy) {
 if (g->traps[m->tr_at(examx, examy)]->difficulty < 99 &&
     p->per_cur-p->encumb(bp_eyes) >= g->traps[m->tr_at(examx, examy)]->visibility &&
     query_yn(_("There is a %s there.  Disarm?"),
              g->traps[m->tr_at(examx, examy)]->name.c_str())) {
     m->disarm_trap(g, examx, examy);
 }
}

void iexamine::water_source(game *g, player *p, map *m, const int examx, const int examy)
{
    item water = m->water_from(examx, examy);
    // Try to handle first (bottling) drink after.
    // changed boolean, large sources should be infinite
    if (g->handle_liquid(water, true, true))
    {
        p->moves -= 100;
    }
    else if (query_yn(_("Drink from your hands?")))
    {
        p->inv.push_back(water);
        water = p->inv.item_by_type(water.typeId());
        p->eat(g, water.invlet);
        p->moves -= 350;
    }
}

void iexamine::acid_source(game *g, player *p, map *m, const int examx, const int examy)
{
    item acid = m->acid_from(examx, examy);
    if (g->handle_liquid(acid, true, true))
    {
        p->moves -= 100;
    }
}
