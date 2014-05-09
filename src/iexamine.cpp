#include "item_factory.h"
#include "iuse.h"
#include "helper.h"
#include "game.h"
#include "mapdata.h"
#include "output.h"
#include "rng.h"
#include "line.h"
#include "player.h"
#include "translations.h"
#include "monstergenerator.h"
#include "helper.h"
#include "uistate.h"
#include "messages.h"
#include <sstream>
#include <algorithm>

void iexamine::none(player *p, map *m, int examx, int examy)
{
    (void)p; //unused
    add_msg(_("That is a %s."), m->name(examx, examy).c_str());
};

void iexamine::gaspump(player *p, map *m, int examx, int examy) {
 if (!query_yn(_("Use the %s?"),m->tername(examx, examy).c_str())) {
  none(p, m, examx, examy);
  return;
 }

 for (int i = 0; i < m->i_at(examx, examy).size(); i++) {
  if (m->i_at(examx, examy)[i].made_of(LIQUID)) {
   item* liq = &(m->i_at(examx, examy)[i]);

   if (one_in(10 + p->dex_cur)) {
    add_msg(m_bad, _("You accidentally spill the %s."), liq->type->name.c_str());
    item spill(liq->type, calendar::turn);
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
     add_msg(_("With a clang and a shudder, the %s pump goes silent."), liq->type->name.c_str());
     m->i_at(examx, examy).erase(m->i_at(examx, examy).begin() + i);
    }
   }
   return;
  }
 }
 add_msg(m_info, _("Out of order."));
}

void iexamine::atm(player *p, map *m, int examx, int examy) {
    (void)m; //unused
    (void)examx; //unused
    (void)examy; //unused
    int choice = -1;
    const int purchase_cash_card = 0;
    const int deposit_money = 1;
    const int withdraw_money = 2;
    const int transfer_money = 3;
    const int cancel = 4;
    long amount = 0;
    long max = 0;
    std::string popupmsg;
    int pos;
    int pos2;
    item *dep;
    item *with;

    uimenu amenu;
    amenu.selected = uistate.iexamine_atm_selected;
    amenu.text = _("Welcome to the C.C.B.o.t.T. ATM. What would you like to do?");
    if (p->cash >= 100) {
        amenu.addentry( purchase_cash_card, true, -1, _("Purchase cash card?") );
    } else {
        amenu.addentry( purchase_cash_card, false, -1,
                        _("You need $1.00 in your account to purchase a card.") );
    }

    if (p->has_amount("cash_card", 1) && p->cash > 0) {
        amenu.addentry( withdraw_money, true, -1, _("Withdraw Money") );
    } else if (p->cash > 0) {
        amenu.addentry( withdraw_money, false, -1,
                        _("You need a cash card before you can withdraw money!") );
    } else {
        amenu.addentry( withdraw_money, false, -1,
                        _("You need money in your account before you can withdraw money!") );
    }

    if (p->has_charges("cash_card", 1)) {
        amenu.addentry( deposit_money, true, -1, _("Deposit Money") );
    } else {
        amenu.addentry( deposit_money, false, -1,
                        _("You need a charged cash card before you can deposit money!") );
    }

    if (p->has_amount("cash_card", 2) && p->has_charges("cash_card", 1)) {
        amenu.addentry( transfer_money, true, -1, _("Transfer Money") );
    } else if (p->has_charges("cash_card", 1)) {
        amenu.addentry( transfer_money, false, -1,
                        _("You need two cash cards before you can move money!") );
    } else {
        amenu.addentry( transfer_money, false, -1,
                        _("One of your cash cards must be charged before you can move money!") );
    }

    amenu.addentry( cancel, true, 'q', _("Cancel") );
    amenu.query();
    choice = amenu.ret;
    uistate.iexamine_atm_selected = choice;

    if (choice == deposit_money) {
        pos = g->inv(_("Insert card for deposit."));
        dep = &(p->i_at(pos));

        if (dep->is_null()) {
            popup(_("You do not have that item!"));
            return;
        }
        if (dep->type->id != "cash_card") {
            popup(_("Please insert cash cards only!"));
            return;
        }
        if (dep->charges == 0) {
            popup(_("You can only deposit money from charged cash cards!"));
            return;
        }

        max = dep->charges;
        std::string popupmsg=string_format(ngettext("Deposit how much? Max:%d cent. (0 to cancel) ",
                                                    "Deposit how much? Max:%d cents. (0 to cancel) ",
                                                    max),
                                           max);
        amount = helper::to_int( string_input_popup( popupmsg, 20,
                   helper::to_string_int(max), "", "", -1, true)
                );
        if (amount <= 0) {
            return;
        }
        if (amount > max) {
            amount = max;
        }
        p->cash += amount;
        dep->charges -= amount;
        add_msg(m_info, ngettext("Your account now holds %d cent.","Your account now holds %d cents.",p->cash),
                   p->cash);
        p->moves -= 100;
        return;

    } else if (choice == withdraw_money) {
        pos = g->inv(_("Insert card for withdrawal."));
        with = &(p->i_at(pos));

        if (with->is_null()) {
            popup(_("You do not have that item!"));
            return;
        }
        if (with->type->id != "cash_card") {
            popup(_("Please insert cash cards only!"));
            return;
        }

        max = p->cash;
        std::string popupmsg=string_format(ngettext("Withdraw how much? Max:%d cent. (0 to cancel) ",
                                                    "Withdraw how much? Max:%d cents. (0 to cancel) ",
                                                    max),
                                           max);
        amount = helper::to_int( string_input_popup( popupmsg, 20,
                   helper::to_string_int(max), "", "", -1, true)
                );
        if (amount <= 0) {
            return;
        }
        if (amount > max) {
            amount = max;
        }
        p->cash -= amount;
        with->charges += amount;
        add_msg(m_info, ngettext("Your account now holds %d cent.",
                            "Your account now holds %d cents.",
                            p->cash),
                   p->cash);
        p->moves -= 100;
        return;

    } else if (choice == transfer_money) {
        pos = g->inv(_("Insert card for deposit."));
        dep = &(p->i_at(pos));
        if (dep->is_null()) {
            popup(_("You do not have that item!"));
            return;
        }
        if (dep->type->id != "cash_card") {
            popup(_("Please insert cash cards only!"));
            return;
        }

        pos2 = g->inv(_("Insert card for withdrawal."));
        with = &(p->i_at(pos2));
        if (with->is_null()) {
            popup(_("You do not have that item!"));
            return;
        }
        if (with->type->id != "cash_card") {
            popup(_("Please insert cash cards only!"));
            return;
        }
        if (with == dep) {
            popup(_("You must select a different card to move from!"));
            return;
        }
        if (with->charges == 0) {
            popup(_("You can only move money from charged cash cards!"));
            return;
        }

        max = with->charges;
        std::string popupmsg=string_format(ngettext("Transfer how much? Max:%d cent. (0 to cancel) ",
                                                    "Transfer how much? Max:%d cents. (0 to cancel) ", 
                                                    max),
                                           max);
        amount = helper::to_int( string_input_popup( popupmsg, 20,
                   helper::to_string_int(max), "", "", -1, true)
                );
        if (amount <= 0) {
            return;
        }
        if (amount > max) {
            amount = max;
        }
        with->charges -= amount;
        dep->charges += amount;
        p->moves -= 100;
        return;

    } else if (choice == purchase_cash_card) {
        if(query_yn(_("This will automatically deduct $1.00 from your bank account. Continue?"))) {
            item card(itypes["cash_card"], calendar::turn);
            it_tool* tool = dynamic_cast<it_tool*>(card.type);
            card.charges = tool->def_charges;
            p->i_add(card);
            p->cash -= 100;
            p->moves -= 100;
        }
    } else {
        return;
    }
}

void iexamine::vending(player *p, map *m, int examx, int examy) {
    std::vector<item>& vend_items = m->i_at(examx, examy);
    int num_items = vend_items.size();

    if (num_items == 0) {
        add_msg(m_info, _("The vending machine is empty!"));
        return;
    }

    item *card;
    if (!p->has_charges("cash_card", 1)) {
        popup(_("You need a charged cash card to purchase things!"));
        return;
    }
    int pos = g->inv(_("Insert card for purchases."));
    card = &(p->i_at(pos));

    if (card->is_null()) {
        popup(_("You do not have that item!"));
        return;
    }
    if (card->type->id != "cash_card") {
        popup(_("Please insert cash cards only!"));
        return;
    }
    if (card->charges == 0) {
        popup(_("You must insert a charged cash card!"));
        return;
    }

    int cur_pos = 0;

    const int iContentHeight = FULL_SCREEN_HEIGHT - 4;
    const int iHalf = iContentHeight / 2;
    const bool odd = iContentHeight % 2;

    const int w_width = FULL_SCREEN_WIDTH / 2 - 1;
    WINDOW *w = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH / 2 - 1,
                       (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0,
                       (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0);
    WINDOW* w_item_info = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH / 2,
                       (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0,
                       (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 + FULL_SCREEN_WIDTH / 2 : FULL_SCREEN_WIDTH / 2);

    bool used_machine = false;
    input_context ctxt("VENDING_MACHINE");
    ctxt.register_updown();
    ctxt.register_action("CONFIRM");
    ctxt.register_action("QUIT");
    ctxt.register_action("HELP_KEYBINDINGS");
    while(true) {
        werase(w);
        wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                         LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
        for (int i = 1; i < FULL_SCREEN_WIDTH / 2 - 2; i++) {
            mvwaddch(w, 2, i, LINE_OXOX);
        }

        mvwaddch(w, 2, 0, LINE_XXXO); // |-
        mvwaddch(w, 2, w_width - 1, LINE_XOXX); // -|

        vend_items = m->i_at(examx, examy);
        num_items = vend_items.size();

        mvwprintz(w, 1, 2, c_ltgray, _("Money left:%d Press 'q' or ESC to stop."), card->charges);

        int first_i, end_i;
        if (cur_pos < iHalf || num_items <= iContentHeight) {
            first_i = 0;
            end_i = std::min(iContentHeight, num_items);
        } else if (cur_pos >= num_items - iHalf) {
            first_i = num_items - iContentHeight;
            end_i = num_items;
        } else {
            first_i = cur_pos - iHalf;
            if (odd) {
                end_i = cur_pos + iHalf + 1;
            } else {
                end_i = cur_pos + iHalf;
            }
        }
        int base_y = 3 - first_i;
        for (int i = first_i; i < end_i; ++i) {
            mvwprintz(w, base_y + i, 2,
                      (i == cur_pos ? h_ltgray : c_ltgray), vend_items[i].tname().c_str());
        }

        //Draw Scrollbar
        draw_scrollbar(w, cur_pos, iContentHeight, num_items, 3);
        wrefresh(w);

        // Item info
        werase(w_item_info);
        fold_and_print(w_item_info,1,2,48-3, c_ltgray, vend_items[cur_pos].info(true));
        wborder(w_item_info, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                             LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
        mvwprintw(w_item_info, 0, 2, "< %s >", vend_items[cur_pos].display_name().c_str() );
        wrefresh(w_item_info);
        const std::string action = ctxt.handle_input();
        if (action == "DOWN") {
            cur_pos++;
            if (cur_pos >= num_items) {
                cur_pos = 0;
            }
        } else if (action == "UP") {
            cur_pos--;
            if (cur_pos < 0) {
                cur_pos = num_items - 1;
            }
        } else if (action == "CONFIRM") {
            if (vend_items[cur_pos].price() > card->charges) {
                popup(_("That item is too expensive!"));
                continue;
            }
            card->charges -= vend_items[cur_pos].price();
            p->i_add_or_drop(vend_items[cur_pos]);
            m->i_rem(examx, examy, cur_pos);
            if (cur_pos == vend_items.size()) {
                cur_pos--;
            }
            used_machine = true;

            if (num_items == 1) {
                add_msg(_("With a beep, the empty vending machine shuts down"));
                break;
            }
        } else if (action == "QUIT") {
            break;
        }
    }
    if (used_machine) {
        p->moves -= 250;
    }
    delwin(w_item_info);
    delwin(w);
}

void iexamine::toilet(player *p, map *m, int examx, int examy) {
    std::vector<item>& items = m->i_at(examx, examy);
    int waterIndex = -1;
    for (int i = 0; i < items.size(); i++) {
        if (items[i].typeId() == "water") {
            waterIndex = i;
            break;
        }
    }

    if (waterIndex < 0) {
        add_msg(m_info, _("This toilet is empty."));
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
            // If player is slaked water might not get consumed.
            if (p->consume(p->inv.position_by_type(water_temp.typeId())))
            {
                p->moves -= 350;

                water.charges -= water_temp.charges;
                if (water.charges <= 0) {
                    drained = true;
                }
            } else {
                p->inv.remove_item(p->inv.position_by_type(water_temp.typeId()));
            }
        }

        if (drained) {
            items.erase(items.begin() + waterIndex);
        }
    }
}

void iexamine::elevator(player *p, map *m, int examx, int examy)
{
    (void)p; //unused
    if (!query_yn(_("Use the %s?"), m->tername(examx, examy).c_str())) {
        return;
    }
    int movez = (g->levz < 0 ? 2 : -2);
    g->vertical_move( movez, false );
}

void iexamine::controls_gate(player *p, map *m, int examx, int examy) {
 if (!query_yn(_("Use the %s?"),m->tername(examx, examy).c_str())) {
  none(p, m, examx, examy);
  return;
 }
 g->open_gate(examx,examy, (ter_id)m->ter(examx,examy));
}

void iexamine::cardreader(player *p, map *m, int examx, int examy) {
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
   if (g->zombie(i).type->id == "mon_turret") {
    g->remove_zombie(i);
    i--;
   }
  }
  add_msg(_("You insert your ID card."));
  add_msg(m_good, _("The nearby doors slide into the floor."));
  p->use_amount(card_type, 1);
 } else {
  bool using_electrohack = (p->has_amount("electrohack", 1) &&
                            query_yn(_("Use electrohack on the reader?")));
  bool using_fingerhack = (!using_electrohack && p->has_bionic("bio_fingerhack") &&
                           p->power_level > 0 &&
                           query_yn(_("Use fingerhack on the reader?")));
  if (using_electrohack || using_fingerhack) {
   p->moves -= 500;
   p->practice(calendar::turn, "computer", 20);
   int success = rng(p->skillLevel("computer") / 4 - 2, p->skillLevel("computer") * 2);
   success += rng(-3, 3);
   if (using_fingerhack)
    success++;
   if (p->int_cur < 8)
    success -= rng(0, int((8 - p->int_cur) / 2));
    else if (p->int_cur > 8)
     success += rng(0, int((p->int_cur - 8) / 2));
     if (success < 0) {
      add_msg(_("You cause a short circuit!"));
      if (success <= -5) {
       if (using_electrohack) {
        add_msg(m_bad, _("Your electrohack is ruined!"));
        p->use_amount("electrohack", 1);
       } else {
        add_msg(m_bad, _("Your power is drained!"));
        p->charge_power(0 - rng(0, p->power_level));
       }
      }
      m->ter_set(examx, examy, t_card_reader_broken);
     } else if (success < 6)
      add_msg(_("Nothing happens."));
      else {
       add_msg(_("You activate the panel!"));
       add_msg(m_good, _("The nearby doors slide into the floor."));
       m->ter_set(examx, examy, t_card_reader_broken);
       for (int i = -3; i <= 3; i++) {
        for (int j = -3; j <= 3; j++) {
         if (m->ter(examx + i, examy + j) == t_door_metal_locked)
          m->ter_set(examx + i, examy + j, t_floor);
          }
       }
      }
  } else {
   add_msg(m_info, _("Looks like you need a %s."),itypes[card_type]->name.c_str());
  }
 }
}

void iexamine::rubble(player *p, map *m, int examx, int examy) {
    if (!(p->has_amount("shovel", 1) || p->has_amount("primitive_shovel", 1)|| p->has_amount("e_tool", 1))) {
        add_msg(m_info, _("If only you had a shovel..."));
        return;
    }
    std::string xname = m->tername(examx, examy);
    if (query_yn(_("Clear up that %s?"), xname.c_str())) {
        // "Remove"
        p->moves -= 200;

        // "Replace"
        if(m->ter(examx,examy) == t_rubble) {
            item rock(itypes["rock"], calendar::turn);
            m->add_item_or_charges(p->posx, p->posy, rock);
            m->add_item_or_charges(p->posx, p->posy, rock);
        }

        // "Refloor"
        if (g->levz < 0) {
            m->ter_set(examx, examy, t_rock_floor);
        } else {
            m->ter_set(examx, examy, t_dirt);
        }

        // "Remind"
        add_msg(_("You clear up that %s."), xname.c_str());
    }
}

void iexamine::chainfence(player *p, map *m, int examx, int examy) {
 if (!query_yn(_("Climb %s?"),m->tername(examx, examy).c_str())) {
  none(p, m, examx, examy);
  return;
 }
 if ( (p->has_trait("ARACHNID_ARMS_OK")) && (!(p->wearing_something_on(bp_torso))) ) {
    add_msg(_("Climbing the fence is trivial for one such as you."));
    p->moves -= 75; // Yes, faster than walking.  6-8 limbs are impressive.
    p->posx = examx;
    p->posy = examy;
    return;
 }
 if ( (p->has_trait("INSECT_ARMS_OK")) && (!(p->wearing_something_on(bp_torso))) ) {
    add_msg(_("You quickly scale the fence."));
    p->moves -= 90;
    p->posx = examx;
    p->posy = examy;
    return;
 }

 p->moves -= 400;
 if (one_in(p->dex_cur)) {
  add_msg(m_bad, _("You slip whilst climbing and fall down again."));
 } else {
  p->moves += p->dex_cur * 10;
  p->posx = examx;
  p->posy = examy;
 }
}

void iexamine::bars(player *p, map *m, int examx, int examy) {
 if(!(p->has_trait("AMORPHOUS"))) {
    none(p, m, examx, examy);
    return;
 }
 if ( ((p->encumb(bp_torso)) >= 1) && ((p->encumb(bp_head)) >= 1) &&
    ((p->encumb(bp_feet)) >= 1) ) { // Most likely places for rigid gear that would catch on the bars.
    add_msg(m_info, _("Your amorphous body could slip though the %s, but your cumbersome gear can't."),m->tername(examx, examy).c_str());
    return;
 }
 if (!query_yn(_("Slip through the %s?"),m->tername(examx, examy).c_str())) {
  none(p, m, examx, examy);
  return;
 }
  p->moves -= 200;
  add_msg(_("You slide right between the bars."));
  p->posx = examx;
  p->posy = examy;
}

void iexamine::tent(player *p, map *m, int examx, int examy) {
 if (!query_yn(_("Take down your tent?"))) {
  none(p, m, examx, examy);
  return;
 }
 p->moves -= 200;
 for (int i = -1; i <= 1; i++)
  for (int j = -1; j <= 1; j++)
   m->furn_set(examx + i, examy + j, f_null);
 add_msg(_("You take down the tent"));
 item dropped(itypes["tent_kit"], calendar::turn);
 m->add_item_or_charges(examx, examy, dropped);
}

void iexamine::shelter(player *p, map *m, int examx, int examy) {
 if (!query_yn(_("Take down %s?"),m->furnname(examx, examy).c_str())) {
  none(p, m, examx, examy);
  return;
 }
 p->moves -= 200;
 for (int i = -1; i <= 1; i++)
  for (int j = -1; j <= 1; j++)
   m->furn_set(examx + i, examy + j, f_null);
 add_msg(_("You take down the shelter"));
 item dropped(itypes["shelter_kit"], calendar::turn);
 m->add_item_or_charges(examx, examy, dropped);
}

void iexamine::wreckage(player *p, map *m, int examx, int examy) {
 if (!(p->has_amount("shovel", 1) || p->has_amount("primitive_shovel", 1)|| p->has_amount("e_tool", 1))) {
  add_msg(m_info, _("If only you had a shovel..."));
  return;
 }

 if (query_yn(_("Clear up that wreckage?"))) {
  p->moves -= 200;
  m->ter_set(examx, examy, t_dirt);
  item chunk(itypes["steel_chunk"], calendar::turn);
  item scrap(itypes["scrap"], calendar::turn);
  item pipe(itypes["pipe"], calendar::turn);
  item wire(itypes["wire"], calendar::turn);
  m->add_item_or_charges(examx, examy, chunk);
  m->add_item_or_charges(examx, examy, scrap);
  if (one_in(5)) {
   m->add_item_or_charges(examx, examy, pipe);
   m->add_item_or_charges(examx, examy, wire); }
  add_msg(_("You clear the wreckage up"));
 }
}

void iexamine::pit(player *p, map *m, int examx, int examy)
{
    inventory map_inv;
    map_inv.form_from_map(point(p->posx, p->posy), 1);

    bool player_has = p->has_amount("2x4", 1);
    bool map_has = map_inv.has_amount("2x4", 1);

    // return if there is no 2x4 around
    if (!player_has && !map_has)
    {
        none(p, m, examx, examy);
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
        add_msg(_("You place a plank of wood over the pit."));
    }
}

void iexamine::pit_covered(player *p, map *m, int examx, int examy)
{
    if(!query_yn(_("Remove cover?")))
    {
        none(p, m, examx, examy);
        return;
    }

    item plank(itypes["2x4"], calendar::turn);
    add_msg(_("You remove the plank."));
    m->add_item_or_charges(p->posx, p->posy, plank);

    if( m->ter(examx, examy) == t_pit_covered )
    {
        m->ter_set(examx, examy, t_pit);
    }
    else if( m->ter(examx, examy) == t_pit_spiked_covered )
    {
        m->ter_set(examx, examy, t_pit_spiked);
    }
}

void iexamine::fence_post(player *p, map *m, int examx, int examy) {

 int ch = menu(true, _("Fence Construction:"), _("Rope Fence"),
               _("Wire Fence"), _("Barbed Wire Fence"), _("Cancel"), NULL);
 switch (ch){
  case 1:{
   if (p->has_amount("rope_6", 2)) {
    p->use_amount("rope_6", 2);
    m->ter_set(examx, examy, t_fence_rope);
    p->moves -= 200;
   } else
    add_msg(m_info, _("You need 2 six-foot lengths of rope to do that"));
  } break;

  case 2:{
   if (p->has_amount("wire", 2)) {
    p->use_amount("wire", 2);
    m->ter_set(examx, examy, t_fence_wire);
    p->moves -= 200;
   } else
    add_msg(m_info, _("You need 2 lengths of wire to do that!"));
  } break;

  case 3:{
   if (p->has_amount("wire_barbed", 2)) {
    p->use_amount("wire_barbed", 2);
    m->ter_set(examx, examy, t_fence_barbed);
    p->moves -= 200;
   } else
    add_msg(m_info, _("You need 2 lengths of barbed wire to do that!"));
  } break;

  case 4:
  default:
   break;
 }
}

void iexamine::remove_fence_rope(player *p, map *m, int examx, int examy) {
 if(!query_yn(_("Remove %s?"),m->tername(examx, examy).c_str())) {
  none(p, m, examx, examy);
  return;
 }
 item rope(itypes["rope_6"], calendar::turn);
 m->add_item_or_charges(p->posx, p->posy, rope);
 m->add_item_or_charges(p->posx, p->posy, rope);
 m->ter_set(examx, examy, t_fence_post);
 p->moves -= 200;

}

void iexamine::remove_fence_wire(player *p, map *m, int examx, int examy) {
 if(!query_yn(_("Remove %s?"),m->tername(examx, examy).c_str())) {
  none(p, m, examx, examy);
  return;
 }

 item rope(itypes["wire"], calendar::turn);
 m->add_item_or_charges(p->posx, p->posy, rope);
 m->add_item_or_charges(p->posx, p->posy, rope);
 m->ter_set(examx, examy, t_fence_post);
 p->moves -= 200;
}

void iexamine::remove_fence_barbed(player *p, map *m, int examx, int examy) {
 if(!query_yn(_("Remove %s?"),m->tername(examx, examy).c_str())) {
  none(p, m, examx, examy);
  return;
 }

 item rope(itypes["wire_barbed"], calendar::turn);
 m->add_item_or_charges(p->posx, p->posy, rope);
 m->add_item_or_charges(p->posx, p->posy, rope);
 m->ter_set(examx, examy, t_fence_post);
 p->moves -= 200;
}

void iexamine::slot_machine(player *p, map *m, int examx, int examy)
{
    (void)m; (void)examx; (void)examy; //unused
    if (p->cash < 10) {
        add_msg(m_info, _("You need $10 to play."));
    } else if (query_yn(_("Insert $10?"))) {
        do {
            if (one_in(5)) {
                popup(_("Three cherries... you get your money back!"));
            } else if (one_in(20)) {
                popup(_("Three bells... you win $50!"));
                p->cash += 40; // Minus the $10 we wagered
            } else if (one_in(50)) {
                popup(_("Three stars... you win $200!"));
                p->cash += 190;
            } else if (one_in(1000)) {
                popup(_("JACKPOT!  You win $3000!"));
                p->cash += 2990;
            } else {
                popup(_("No win."));
                p->cash -= 10;
            }
        } while (p->cash >= 10 && query_yn(_("Play again?")));
    }
}

void iexamine::safe(player *p, map *m, int examx, int examy) {
  if (!p->has_amount("stethoscope", 1)) {
    add_msg(m_info, _("You need a stethoscope for safecracking."));
    return;
  }

  if (query_yn(_("Attempt to crack the safe?"))) {
    bool success = true;

    if (success) {
      m->furn_set(examx, examy, f_safe_o);
      add_msg(m_good, _("You successfully crack the safe!"));
    } else {
      add_msg(_("The safe resists your attempt at cracking it."));
    }
  }
}

void iexamine::bulletin_board(player *p, map *m, int examx, int examy) {
    (void)g; (void)p; //unused
    basecamp *camp = m->camp_at(examx, examy);
    if (camp && camp->board_x() == examx && camp->board_y() == examy) {
        std::vector<std::string> options;
        options.push_back(_("Cancel"));
        // Causes a warning due to being unused, but don't want to delete
        // since it's clearly what's intened for future functionality.
        //int choice = menu_vec(true, camp->board_name().c_str(), options) - 1;
    } else {
        bool create_camp = m->allow_camp(examx, examy);
        std::vector<std::string> options;
        if (create_camp) {
            options.push_back(_("Create camp"));
        }
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

void iexamine::fault(player *p, map *m, int examx, int examy)
{
    (void)g; (void)p; (void)m; (void)examx; (void)examy; //unused
    popup(_("\
This wall is perfectly vertical.  Odd, twisted holes are set in it, leading\n\
as far back into the solid rock as you can see.  The holes are humanoid in\n\
shape, but with long, twisted, distended limbs."));
}

void iexamine::pedestal_wyrm(player *p, map *m, int examx, int examy) {
 if (!m->i_at(examx, examy).empty()) {
  none(p, m, examx, examy);
  return;
 }
 add_msg(_("The pedestal sinks into the ground..."));
 m->ter_set(examx, examy, t_rock_floor);
 g->add_event(EVENT_SPAWN_WYRMS, int(calendar::turn) + rng(5, 10));
}

void iexamine::pedestal_temple(player *p, map *m, int examx, int examy) {

 if (m->i_at(examx, examy).size() == 1 &&
     m->i_at(examx, examy)[0].type->id == "petrified_eye") {
  add_msg(_("The pedestal sinks into the ground..."));
  m->ter_set(examx, examy, t_dirt);
  m->i_at(examx, examy).clear();
  g->add_event(EVENT_TEMPLE_OPEN, int(calendar::turn) + 4);
 } else if (p->has_amount("petrified_eye", 1) &&
            query_yn(_("Place your petrified eye on the pedestal?"))) {
  p->use_amount("petrified_eye", 1);
  add_msg(_("The pedestal sinks into the ground..."));
  m->ter_set(examx, examy, t_dirt);
  g->add_event(EVENT_TEMPLE_OPEN, int(calendar::turn) + 4);
 } else
  add_msg(_("This pedestal is engraved in eye-shaped diagrams, and has a \
large semi-spherical indentation at the top."));
}

void iexamine::fswitch(player *p, map *m, int examx, int examy)
{
    if(!query_yn(_("Flip the %s?"), m->tername(examx, examy).c_str())) {
        none(p, m, examx, examy);
        return;
    }
    ter_id terid = m->ter(examx, examy);
    p->moves -= 100;
    for (int y = examy; y <= examy + 5; y++) {
        for (int x = 0; x < SEEX * MAPSIZE; x++) {
            if ( terid == t_switch_rg ) {
                if (m->ter(x, y) == t_rock_red) {
                    m->ter_set(x, y, t_floor_red);
                } else if (m->ter(x, y) == t_floor_red) {
                    m->ter_set(x, y, t_rock_red);
                } else if (m->ter(x, y) == t_rock_green) {
                    m->ter_set(x, y, t_floor_green);
                } else if (m->ter(x, y) == t_floor_green) {
                    m->ter_set(x, y, t_rock_green);
                }
            } else if ( terid == t_switch_gb ) {
                if (m->ter(x, y) == t_rock_blue) {
                    m->ter_set(x, y, t_floor_blue);
                } else if (m->ter(x, y) == t_floor_blue) {
                    m->ter_set(x, y, t_rock_blue);
                } else if (m->ter(x, y) == t_rock_green) {
                    m->ter_set(x, y, t_floor_green);
                } else if (m->ter(x, y) == t_floor_green) {
                    m->ter_set(x, y, t_rock_green);
                }
            } else if ( terid == t_switch_rb ) {
                if (m->ter(x, y) == t_rock_blue) {
                    m->ter_set(x, y, t_floor_blue);
                } else if (m->ter(x, y) == t_floor_blue) {
                    m->ter_set(x, y, t_rock_blue);
                } else if (m->ter(x, y) == t_rock_red) {
                    m->ter_set(x, y, t_floor_red);
                } else if (m->ter(x, y) == t_floor_red) {
                    m->ter_set(x, y, t_rock_red);
                }
            } else if ( terid == t_switch_even ) {
                if ((y - examy) % 2 == 1) {
                    if (m->ter(x, y) == t_rock_red) {
                        m->ter_set(x, y, t_floor_red);
                    } else if (m->ter(x, y) == t_floor_red) {
                        m->ter_set(x, y, t_rock_red);
                    } else if (m->ter(x, y) == t_rock_green) {
                        m->ter_set(x, y, t_floor_green);
                    } else if (m->ter(x, y) == t_floor_green) {
                        m->ter_set(x, y, t_rock_green);
                    } else if (m->ter(x, y) == t_rock_blue) {
                        m->ter_set(x, y, t_floor_blue);
                    } else if (m->ter(x, y) == t_floor_blue) {
                        m->ter_set(x, y, t_rock_blue);
                    }
                }
            }
        }
    }
    add_msg(m_warning, _("You hear the rumble of rock shifting."));
    g->add_event(EVENT_TEMPLE_SPAWN, calendar::turn + 3);
}

void iexamine::flower_poppy(player *p, map *m, int examx, int examy) {
  if ( ((p->has_trait("PROBOSCIS")) || (p->has_trait("BEAK_HUM"))) && ((p->hunger) > 0) &&
      (!(p->wearing_something_on(bp_mouth))) ) {
      if (!query_yn(_("You feel woozy as you explore the %s. Drink?"),m->furnname(examx, examy).c_str())) {
          return;
      }
      p->moves -= 150; // You take your time...
      add_msg(_("You slowly suck up the nectar."));
      p->hunger -= 25;
      p->add_disease("pkill2", 70);
      p->fatigue += 20;
      // Please drink poppy nectar responsibly.
      if (one_in(20)) {
          p->add_addiction(ADD_PKILLER, 1);
      }
  }
  if(!query_yn(_("Pick %s?"),m->furnname(examx, examy).c_str())) {
    none(p, m, examx, examy);
    return;
  }

  int resist = p->get_env_resist(bp_mouth);

  if (resist < 10) {
    // Can't smell the flowers with a gas mask on!
    add_msg(m_warning, _("This flower has a heady aroma."));
  }

  if (one_in(3) && resist < 5)  {
    // Should user player::infect, but can't!
    // player::infect needs to be restructured to return a bool indicating success.
    add_msg(m_bad, _("You fall asleep..."));
    p->fall_asleep(1200);
    add_msg(m_bad, _("Your legs are covered in the poppy's roots!"));
    p->hurt(bp_legs, 0, 4);
    p->moves -=50;
  }

  m->furn_set(examx, examy, f_null);
  m->spawn_item(examx, examy, "poppy_flower");
  m->spawn_item(examx, examy, "poppy_bud");
}

void iexamine::flower_blubell(player *p, map *m, int examx, int examy) {
  if ( ((p->has_trait("PROBOSCIS")) || (p->has_trait("BEAK_HUM"))) &&
      ((p->hunger) > 0) && (!(p->wearing_something_on(bp_mouth))) ) {
      p->moves -= 50; // Takes 30 seconds
      add_msg(_("You drink some nectar."));
      p->hunger -= 15;
  }
  if(!query_yn(_("Pick %s?"),m->furnname(examx, examy).c_str())) {
    none(p, m, examx, examy);
    return;
  }
  m->furn_set(examx, examy, f_null);
  m->spawn_item(examx, examy, "bluebell_flower");
  m->spawn_item(examx, examy, "bluebell_bud");
}

void iexamine::flower_dahlia(player *p, map *m, int examx, int examy) {
  if ( ((p->has_trait("PROBOSCIS")) || (p->has_trait("BEAK_HUM"))) &&
      ((p->hunger) > 0) && (!(p->wearing_something_on(bp_mouth))) ) {
      p->moves -= 50; // Takes 30 seconds
      add_msg(_("You drink some nectar."));
      p->hunger -= 15;
  }
  if(!query_yn(_("Pick %s?"),m->furnname(examx, examy).c_str())) {
    none(p, m, examx, examy);
    return;
  }
  m->furn_set(examx, examy, f_null);
  m->spawn_item(examx, examy, "dahlia_flower");
  m->spawn_item(examx, examy, "dahlia_bud");
}

void iexamine::egg_sackbw(player *p, map *m, int examx, int examy) {
  if(!query_yn(_("Harvest the %s?"),m->furnname(examx, examy).c_str())) {
    none(p, m, examx, examy);
    return;
  }
  if (one_in(2)){
    monster spider_widow_giant_s(GetMType("mon_spider_widow_giant_s"));
    int f = 0;
    for (int i = examx -1; i <= examx + 1; i++) {
        for (int j = examy -1; j <= examy + 1; j++) {
                if (!(g->u.posx == i && g->u.posy == j) && one_in(3)){
                    spider_widow_giant_s.spawn(i, j);
                    g->add_zombie(spider_widow_giant_s);
                    f++;
                }
        }
    }
    if (f == 1){
        add_msg(m_warning, _("A spiderling brusts from the %s!"),m->furnname(examx, examy).c_str());
    } else if (f >= 1) {
        add_msg(m_warning, _("Spiderlings brust from the %s!"),m->furnname(examx, examy).c_str());
    }
  }
  m->spawn_item(examx, examy, "spider_egg", rng(1,4));
  m->furn_set(examx, examy, f_egg_sacke);
}

void iexamine::egg_sackws(player *p, map *m, int examx, int examy) {
  if(!query_yn(_("Harvest the %s?"),m->furnname(examx, examy).c_str())) {
    none(p, m, examx, examy);
    return;
  }
  if (one_in(2)){
    monster mon_spider_web_s(GetMType("mon_spider_web_s"));
    int f = 0;
    for (int i = examx -1; i <= examx + 1; i++) {
        for (int j = examy -1; j <= examy + 1; j++) {
                if (!(g->u.posx == i && g->u.posy == j) && one_in(3)){
                    mon_spider_web_s.spawn(i, j);
                    g->add_zombie(mon_spider_web_s);
                    f++;
                }
        }
    }
    if (f == 1){
        add_msg(m_warning, _("A spiderling brusts from the %s!"),m->furnname(examx, examy).c_str());
    } else if (f >= 1) {
        add_msg(m_warning, _("Spiderlings brust from the %s!"),m->furnname(examx, examy).c_str());
    }
  }
  m->spawn_item(examx, examy, "spider_egg", rng(1,4));
  m->furn_set(examx, examy, f_egg_sacke);
}
void iexamine::fungus(player *p, map *m, int examx, int examy) {
    // TODO: Infect NPCs?
    monster spore(GetMType("mon_spore"));
    int mondex;
    add_msg(_("The %s crumbles into spores!"), m->furnname(examx, examy).c_str());
    for (int i = examx - 1; i <= examx + 1; i++) {
        for (int j = examy - 1; j <= examy + 1; j++) {
            mondex = g->mon_at(i, j);
            if (g->m.move_cost(i, j) > 0 || (i == examx && j == examy)) {
                if (mondex != -1) { // Spores hit a monster
                    if (g->u_see(i, j) &&
                            !g->zombie(mondex).type->in_species("FUNGUS")) {
                        add_msg(_("The %s is covered in tiny spores!"),
                                        g->zombie(mondex).name().c_str());
                    }
                    if (!g->zombie(mondex).make_fungus()) {
                        g->kill_mon(mondex, false);
                    }
                } else if (g->u.posx == i && g->u.posy == j) {
                    // Spores hit the player
                    bool hit = false;
                    if (one_in(4) && g->u.infect("spores", bp_head, 3, 90, false, 1, 3, 120, 1, true)) {
                        hit = true;
                    }
                    if (one_in(2) && g->u.infect("spores", bp_torso, 3, 90, false, 1, 3, 120, 1, true)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.infect("spores", bp_arms, 3, 90, false, 1, 3, 120, 1, true, 1)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.infect("spores", bp_arms, 3, 90, false, 1, 3, 120, 1, true, 0)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.infect("spores", bp_legs, 3, 90, false, 1, 3, 120, 1, true, 1)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.infect("spores", bp_legs, 3, 90, false, 1, 3, 120, 1, true, 0)) {
                        hit = true;
                    }
                    if (hit) {
                        add_msg(m_warning, _("You're covered in tiny spores!"));
                    }
                } else if (((i == examx && j == examy) || one_in(4)) &&
                              g->num_zombies() <= 1000) { // Spawn a spore
                    spore.spawn(i, j);
                    g->add_zombie(spore);
                }
            }
        }
    }
    m->furn_set(examx, examy, f_null);
    p->moves -=50;
}

void iexamine::dirtmound(player *p, map *m, int examx, int examy) {

    if (g->get_temperature() < 50) { // semi-appropriate temperature for most plants
        add_msg(m_info, _("It is too cold to plant anything now."));
        return;
    }
    /* ambient_light_at() not working?
    if (m->ambient_light_at(examx, examy) < LIGHT_AMBIENT_LOW) {
        add_msg(m_info, _("It is too dark to plant anything now."));
        return;
    }*/
    if (!p->has_item_with_flag("SEED")){
        add_msg(m_info, _("You have no seeds to plant."));
        return;
    }
    if (m->i_at(examx, examy).size() != 0){
        add_msg(_("Something's lying there..."));
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
        add_msg(_("You saved your seeds for later.")); // huehuehue
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
    m->spawn_item(examx, examy, seed_types[seed_index], 1, 1, calendar::turn);
    m->set(examx, examy, t_dirt, f_plant_seed);
    p->moves -= 500;
    add_msg(_("Planted %s"), seed_names[seed_index].c_str());
}

void iexamine::aggie_plant(player *p, map *m, int examx, int examy) {
    if (m->furn(examx, examy) == f_plant_harvest && query_yn(_("Harvest plant?"))) {
        itype_id seedType = m->i_at(examx, examy)[0].typeId();
        if (seedType == "fungal_seeds") {
            fungus(p, m, examx, examy);
            for (int k = 0; k < g->m.i_at(examx, examy).size(); k++) {
                g->m.i_rem(examx, examy, k);
            }
        } else {
            m->i_clear(examx, examy);
            m->furn_set(examx, examy, f_null);

            int skillLevel = p->skillLevel("survival");
            int plantCount = rng(skillLevel / 2, skillLevel);
            if (plantCount >= 12) {
                plantCount = 12;
            }

            m->spawn_item(examx, examy, seedType.substr(5), plantCount, 0, calendar::turn);
            if(item_controller->find_template(seedType)->count_by_charges()) {
                m->spawn_item(examx, examy, seedType, 1, rng(plantCount / 4, plantCount / 2));
            } else {
                m->spawn_item(examx, examy, seedType, rng(plantCount / 4, plantCount / 2));
            }

            p->moves -= 500;
        }
    } else if (m->furn(examx,examy) != f_plant_harvest && m->i_at(examx, examy).size() == 1 &&
                 p->charges_of("fertilizer_liquid") && query_yn(_("Fertilize plant"))) {
        //Reduce the amount of time it takes until the next stage of the plant by 20% of a seasons length. (default 2.8 days).
        WORLDPTR world = world_generator->active_world;
        unsigned int fertilizerEpoch = 14400 * 2; //default if options is empty for some reason.
        if (!world->world_options.empty()) {
            fertilizerEpoch = 14400 * (world->world_options["SEASON_LENGTH"] * 0.2) ;
        }

        if (m->i_at(examx, examy)[0].bday > fertilizerEpoch) {
            m->i_at(examx, examy)[0].bday -= fertilizerEpoch;
        } else {
            m->i_at(examx, examy)[0].bday = 0;
        }
        p->use_charges("fertilizer_liquid", 1);
        m->i_at(examx, examy).push_back(item_controller->create("fertilizer", (int) calendar::turn));
    }
}

void iexamine::fvat_empty(player *p, map *m, int examx, int examy) {
    itype_id brew_type;
    bool to_deposit = false;
    bool vat_full = false;
    bool brew_present = false;
    int charges_on_ground = 0;
    for (int i = 0; i < m->i_at(examx, examy).size(); i++) {
        if (!(m->i_at(examx, examy)[i].has_flag("BREW")) || brew_present) {
            //This isn't a brew or there was already another kind of brew inside, so this has to be moved.
            m->add_item_or_charges(examx, examy, m->i_at(examx, examy)[i]);
            //Add_item_or_charges will add items to a space near the vat, because it's flagged as NOITEM.
            m->i_at(examx, examy).erase(m->i_at(examx, examy).begin() + i);
            //Now that a copy of the item was spawned in a nearby square, the original is deleted.
            i--;
        }
        else brew_present = true;
    }
    if (!brew_present)
    {
        if ( !p->has_item_with_flag("BREW") ) {
            add_msg(m_info, _("You have no brew to ferment."));
            return;
        }
        // Get list of all inv+wielded ferment-able items.
        std::vector<item*> b_inv = p->inv.all_items_with_flag("BREW");
        if (g->u.weapon.contains_with_flag("BREW"))
            b_inv.push_back(&g->u.weapon.contents[0]);
        // Make lists of unique typeids and names for the menu
        // Code shamelessly stolen from the crop planting function!
        std::vector<itype_id> b_types;
        std::vector<std::string> b_names;
        for (std::vector<item*>::iterator it = b_inv.begin() ; it != b_inv.end(); it++) {
            if (std::find(b_types.begin(), b_types.end(), (*it)->typeId()) == b_types.end()) {
                b_types.push_back((*it)->typeId());
                b_names.push_back((*it)->name);
            }
        }
        // Choose brew from list
        int b_index = 0;
        if (b_types.size() > 1) {
            b_names.push_back("Cancel");
            b_index = menu_vec(false, _("Use which brew?"), b_names) - 1;
            if (b_index == b_names.size() - 1)
                b_index = -1;
        } else { //Only one brew type was in inventory, so it's automatically used
            if (!query_yn(_("Set %s in the vat?"), b_names[0].c_str()))
                b_index = -1;
        }
        if (b_index < 0) {
            return;
        }
        to_deposit = true;
        brew_type = b_types[b_index];
    }
    else {
        item brew = m->i_at(examx, examy)[0];
        brew_type = brew.typeId();
        charges_on_ground = brew.charges;
        if (p->charges_of(brew_type) > 0)
            if (query_yn(_("Add %s to the vat?"), brew.name.c_str()))
                to_deposit = true;
    }
    if (to_deposit) {
        item brew(itypes[brew_type], 0);
        int charges_held = p->charges_of(brew_type);
        brew.charges = charges_on_ground;
        for (int i=0; i<charges_held && !vat_full; i++) {
            p->use_charges(brew_type, 1);
            brew.charges++;
            if ( ((brew.count_by_charges()) ? brew.volume(false, true)/1000 :
                brew.volume(false, true)/1000*brew.charges ) >= 100)
                vat_full = true; //vats hold 50 units of brew, or 350 charges for a count_by_charges brew
        }
        add_msg(_("Set %s in the vat."), brew.name.c_str());
        m->i_clear(examx, examy);
        m->i_at(examx, examy).push_back(brew); //This is needed to bypass NOITEM
        p->moves -= 250;
    }
    if (vat_full || query_yn(_("Start fermenting cycle?"))) {
        m->i_at(examx, examy)[0].bday = calendar::turn;
        m->furn_set(examx, examy, f_fvat_full);
        if (vat_full)
            add_msg(_("The vat is full, so you close the lid and start the fermenting cycle."));
        else
            add_msg(_("You close the lid and start the fermenting cycle."));
    }
}

void iexamine::fvat_full(player *p, map *m, int examx, int examy) {
    bool liquid_present = false;
    for (int i = 0; i < m->i_at(examx, examy).size(); i++) {
        if (!(m->i_at(examx, examy)[i].made_of(LIQUID)) || liquid_present) {
            m->add_item_or_charges(examx, examy, m->i_at(examx, examy)[i]);
            m->i_at(examx, examy).erase(m->i_at(examx, examy).begin() + i);
            i--;
        }
        else liquid_present = true;
    }
    if (!liquid_present) {
        debugmsg("fvat_full was empty or contained non-liquids only!");
        m->furn_set(examx, examy, f_fvat_empty);
        return;
    }
    item brew_i = m->i_at(examx, examy)[0];
    if (brew_i.has_flag("BREW")) //Does the vat contain unfermented brew, or already fermented booze?
    {
        int brew_time = brew_i.brewing_time();
        int brewing_stage = 3 * ((float)(calendar::turn.get_turn() - brew_i.bday) / (brew_time));
        add_msg(_("There's a vat full of %s set to ferment there."), brew_i.name.c_str());
        switch (brewing_stage) {
        case 0:
            add_msg(_("It's been set recently, and will take some time to ferment.")); break;
        case 1:
            add_msg(_("It is about halfway done fermenting.")); break;
        case 2:
            add_msg(_("It will be ready for bottling soon.")); break; //More messages can be added to show progress if desired
        default:
            if ( (calendar::turn.get_turn() > (brew_i.bday + brew_time) ) //Double-checking that the brew is actually ready
            && m->furn(examx, examy) == f_fvat_full && query_yn(_("Finish brewing?")) )
            {
                itype_id alcoholType = m->i_at(examx, examy)[0].typeId().substr(5); //declare fermenting result as the brew's ID minus "brew_"
                SkillLevel& cooking = p->skillLevel("cooking");
                if (alcoholType=="hb_beer" && cooking<5)
                    alcoholType=alcoholType.substr(3); //hb_beer -> beer
                item booze(itypes[alcoholType], 0);
                booze.charges = brew_i.charges; booze.bday = brew_i.bday;

                m->i_clear(examx, examy);
                m->i_at(examx, examy).push_back(booze);
                p->moves -= 500;

                p->practice( calendar::turn, "cooking", std::min(brew_time/600, 72) ); //low xp: you also get xp from crafting the brew
                /*if ((cooking<4 && !one_in(cooking)) || (cooking>=4 && !one_in(4))) { //Couldn't figure out how to spawn yeast
                    add_msg(_("You manage to retrieve some yeast from the vat!"));  //directly into the player's inventory,
                    // add_item(???)                                                   //then decided that yeast culturing was
                }                                                                      //a better idea. */
                add_msg(_("The %s is now ready for bottling."), booze.name.c_str());
            }
        }
    }
    else { //Booze is done, so bottle it!
        item* booze = &(m->i_at(examx, examy)[0]);
        if (g->handle_liquid(*booze, true, false)) {
            m->i_at(examx, examy).erase(m->i_at(examx, examy).begin());
            m->furn_set(examx, examy, f_fvat_empty);
            add_msg(_("You squeeze the last drops of %s from the vat."), booze->name.c_str());
        }
    }
}

void iexamine::keg(player *p, map *m, int examx, int examy) {
    int keg_cap = 600;
    bool liquid_present = false;
    for (int i = 0; i < m->i_at(examx, examy).size(); i++) {
        if (!(m->i_at(examx, examy)[i].is_drink()) || liquid_present) {
            m->add_item_or_charges(examx, examy, m->i_at(examx, examy)[i]);
            m->i_at(examx, examy).erase(m->i_at(examx, examy).begin() + i);
            i--;
        }
        else liquid_present = true;
    }
    if (!liquid_present) {
        if ( !p->has_drink() ) {
            add_msg(m_info, _("You don't have any drinks to fill the %s with."), m->name(examx, examy).c_str());
            return;
        }
        // Get list of all drinks
        std::vector<item*> drinks_inv = p->inv.all_drinks();
        if (!g->u.weapon.contents.empty() && g->u.weapon.contents[0].is_drink())
            drinks_inv.push_back(&g->u.weapon.contents[0]);
        // Make lists of unique drinks... about third time we do this, maybe we oughta make a function next time
        std::vector<itype_id> drink_types;
        std::vector<std::string> drink_names;
        for (std::vector<item*>::iterator it = drinks_inv.begin() ; it != drinks_inv.end(); it++) {
            if (std::find(drink_types.begin(), drink_types.end(), (*it)->typeId()) == drink_types.end()) {
                drink_types.push_back((*it)->typeId());
                drink_names.push_back((*it)->name);
            }
        }
        // Choose drink to store in keg from list
        int drink_index = 0;
        if (drink_types.size() > 1) {
            drink_names.push_back("Cancel");
            drink_index = menu_vec(false, _("Store which drink?"), drink_names) - 1;
            if (drink_index == drink_names.size() - 1)
                drink_index = -1;
        } else { //Only one drink type was in inventory, so it's automatically used
            if (!query_yn(_("Fill the %s with %s?"), m->name(examx, examy).c_str(), drink_names[0].c_str()))
                drink_index = -1;
        }
        if (drink_index < 0)
            return;
        //Store liquid chosen in the keg
        itype_id drink_type = drink_types[drink_index];
        int charges_held = p->charges_of(drink_type);
        item drink (itypes[drink_type], 0);
        drink.charges = 0;
        bool keg_full = false;
        for (int i=0; i<charges_held && !keg_full; i++) {
            g->u.use_charges(drink.typeId(), 1);
            drink.charges++;
            int d_vol = (drink.count_by_charges()) ? drink.volume(false, true)/1000
                : drink.volume(false, true)/1000*drink.charges;
            if (d_vol >= keg_cap)
                keg_full = true;
        }
        if (keg_full) add_msg(_("You completely fill the %s with %s."),
                m->name(examx, examy).c_str(), drink.name.c_str());
        else add_msg(_("You fill the %s with %s."), m->name(examx, examy).c_str(),
                drink.name.c_str());
        p->moves -= 250;
        m->i_clear(examx, examy);
        m->i_at(examx, examy).push_back(drink);
        return;
    }
    else {
        item* drink = &(m->i_at(examx, examy)[0]);
        std::vector<std::string> menu_items;
        std::vector<uimenu_entry> options_message;
        menu_items.push_back(_("Fill a container with %drink"));
        options_message.push_back(uimenu_entry(string_format(_("Fill a container with %s"), drink->name.c_str()), '1'));
        menu_items.push_back(_("Have a drink"));
        options_message.push_back(uimenu_entry(_("Have a drink"), '2'));
        menu_items.push_back(_("Refill"));
        options_message.push_back(uimenu_entry(_("Refill"), '3'));
        menu_items.push_back(_("Examine"));
        options_message.push_back(uimenu_entry(_("Examine"), '4'));
        menu_items.push_back(_("Cancel"));
        options_message.push_back(uimenu_entry(_("Cancel"), '5'));

        int choice;
        if( menu_items.size() == 1 )
          choice = 0;
        else {
          uimenu selectmenu;
          selectmenu.return_invalid = true;
          selectmenu.text = _("Select an action");
          selectmenu.entries = options_message;
          selectmenu.selected = 0;
          selectmenu.query();
          choice = selectmenu.ret; }
        if(choice<0)
          return;

        if(menu_items[choice]==_("Fill a container with %drink")){
            if (g->handle_liquid(*drink, true, false)) {
                m->i_at(examx, examy).erase(m->i_at(examx, examy).begin());
                add_msg(_("You squeeze the last drops of %s from the %s."), drink->name.c_str(),
                           m->name(examx, examy).c_str());
            }
            return;
        }

        if(menu_items[choice]==_("Have a drink")){
            drink->charges--;
            if (drink->charges == 0) {
                m->i_at(examx, examy).erase(m->i_at(examx, examy).begin());
                add_msg(_("You squeeze the last drops of %s from the %s."), drink->name.c_str(),
                           m->name(examx, examy).c_str());
            }
            p->eat(drink, dynamic_cast<it_comest*>(drink->type));
            p->moves -= 250;
            return;
        }

        if(menu_items[choice]==_("Refill")){
            int charges_held = p->charges_of(drink->typeId());
            int d_vol = (drink->count_by_charges()) ? drink->volume(false, true)/1000
                : drink->volume(false, true)/1000*drink->charges;
            if (d_vol >= keg_cap){
                add_msg(_("The %s is completely full."), m->name(examx, examy).c_str());
                return;
            }
            if (charges_held < 1) {
                add_msg(m_info, _("You don't have any %s to fill the %s with."), drink->name.c_str(),
                           m->name(examx, examy).c_str());
                return;
            }
            for (int i=0; i<charges_held; i++) {
                g->u.use_charges(drink->typeId(), 1);
                drink->charges++;
                int d_vol = (drink->count_by_charges()) ? drink->volume(false, true)/1000
                    : drink->volume(false, true)/1000*drink->charges;
                if (d_vol >= keg_cap) {
                    add_msg(_("You completely fill the %s with %s."), m->name(examx, examy).c_str(),
                               drink->name.c_str());
                    p->moves -= 250;
                    return;
                }
            }
            add_msg(_("You fill the %s with %s."), m->name(examx, examy).c_str(),
                   drink->name.c_str());
            p->moves -= 250;
            return;
        }

        if(menu_items[choice]==_("Examine")){
            add_msg(m_info, _("That is a %s."), m->name(examx, examy).c_str());
            int d_vol = (drink->count_by_charges()) ? drink->volume(false, true)/1000
                : drink->volume(false, true)/1000*drink->charges;
            if (d_vol < 1)
                add_msg(m_info, ngettext("It has %d portion of %s left.",
                                    "It has %d portions of %s left.",
                                    drink->charges),
                           drink->charges, drink->name.c_str());
            else
                add_msg(m_info, _("%s contained: %d/%d"), drink->name.c_str(), d_vol, keg_cap);
            return;
        }
    }
}

void iexamine::pick_plant(player *p, map *m, int examx, int examy, std::string itemType, int new_ter, bool seeds) {
  if (!query_yn(_("Pick %s?"), m->tername(examx, examy).c_str())) {
    none(p, m, examx, examy);
    return;
  }

  SkillLevel& survival = p->skillLevel("survival");
  if (survival < 1)
    p->practice(calendar::turn, "survival", rng(5, 12));
  else if (survival < 6)
    p->practice(calendar::turn, "survival", rng(1, 12 / survival));

  int plantCount = rng(survival / 2, survival);
  if (plantCount > 12)
    plantCount = 12;

  m->spawn_item(examx, examy, itemType, plantCount, 0, calendar::turn);

  if (seeds) {
    m->spawn_item(examx, examy, "seed_" + itemType, 1, rng(plantCount / 4, plantCount / 2), calendar::turn);
  }

  m->ter_set(examx, examy, (ter_id)new_ter);
}

void iexamine::tree_apple(player *p, map *m, int examx, int examy) {
  if ( ((p->has_trait("PROBOSCIS")) || (p->has_trait("BEAK_HUM"))) &&
      ((p->hunger) > 0) && (!(p->wearing_something_on(bp_mouth))) ) {
      p->moves -= 100; // Need to find a blossom (assume there's one somewhere)
      add_msg(_("You find a flower and drink some nectar."));
      p->hunger -= 15;
  }
  if(!query_yn(_("Harvest from the %s?"),m->tername(examx, examy).c_str())) {
    none(p, m, examx, examy);
    return;
  }
  pick_plant(p, m, examx, examy, "apple", t_tree);
}

void iexamine::shrub_blueberry(player *p, map *m, int examx, int examy) {
  pick_plant(p, m, examx, examy, "blueberries", t_shrub, true);
}

void iexamine::shrub_strawberry(player *p, map *m, int examx, int examy) {
  pick_plant(p, m, examx, examy, "strawberries", t_shrub, true);
}

void iexamine::shrub_marloss(player *p, map *m, int examx, int examy) {
  pick_plant(p, m, examx, examy, "marloss_berry", t_shrub_fungal);
}

void iexamine::shrub_wildveggies(player *p, map *m, int examx, int examy) {
 if(!query_yn(_("Pick %s?"),m->tername(examx, examy).c_str())) return;

 p->assign_activity(ACT_FORAGE, 500 / (p->skillLevel("survival") + 1), 0);
 p->activity.placement = point(examx, examy);
}

int sum_up_item_weight_by_material(std::vector<item> &items, const std::string &material, bool remove_items) {
    int sum_weight = 0;
    for (int i = items.size() - 1; i >= 0; i--) {
        const item &it = items[i];
        if (it.made_of(material) && it.weight() > 0) {
            sum_weight += it.weight();
            if (remove_items) {
                items.erase(items.begin() + i);
            }
        }
    }
    return sum_weight;
}

void add_recyle_menu_entry(uimenu &menu, int w, char hk, const std::string &type) {
    const itype *itt = item_controller->find_template(type);
    menu.entries.push_back(
        uimenu_entry(
            menu.entries.size() + 1, // value return by uimenu for this entry
            true, // enabled
            hk, // hotkey
            string_format(_("about %d %s"), (int) (w / itt->weight), itt->name.c_str())
        )
    );
}

void iexamine::recycler(player *p, map *m, int examx, int examy) {
    std::vector<item> &items_on_map = m->i_at(examx, examy);

    // check for how much steel, by weight, is in the recycler
    // only items made of STEEL are checked
    // IRON and other metals cannot be turned into STEEL for now
    int steel_weight = sum_up_item_weight_by_material(items_on_map, "steel", false);
    if (steel_weight == 0)
    {
        add_msg(m_info, _("The recycler is currently empty.  Drop some metal items onto it and examine it again."));
        return;
    }
    // See below for recover_factor (rng(6,9)/10), this
    // is the normal value of that recover factor.
    static const double norm_recover_factor = 8.0 / 10.0;
    const int norm_recover_weight = steel_weight * norm_recover_factor;
    uimenu as_m;
    // Get format for printing weights, convert weight to that format,
    const std::string format = OPTIONS["USE_METRIC_WEIGHTS"].getValue() == "lbs" ? _("%.3f lbs") : _("%.3f kg");
    const std::string weight_str = string_format(format, g->u.convert_weight(steel_weight));
    as_m.text = string_format(_("Recycle %s metal into:"), weight_str.c_str());
    add_recyle_menu_entry(as_m, norm_recover_weight, 'l', "steel_lump");
    add_recyle_menu_entry(as_m, norm_recover_weight, 'S', "sheet_metal");
    add_recyle_menu_entry(as_m, norm_recover_weight, 'c', "steel_chunk");
    add_recyle_menu_entry(as_m, norm_recover_weight, 's', "scrap");
    as_m.entries.push_back(uimenu_entry(0, true, 'c', _("Cancel")));
    as_m.selected = 4;
    as_m.query(); /* calculate key and window variables, generate window, and loop until we get a valid answer */
    int ch = as_m.ret;
    int num_lumps = 0;
    int num_sheets = 0;
    int num_chunks = 0;
    int num_scraps = 0;

    if (ch >= 5 || ch <= 0)
    {
        add_msg(_("Never mind."));
        return;
    }

    // Sum up again, this time remove the items,
    // ignore result, should be the same as before.
    sum_up_item_weight_by_material(items_on_map, "steel", true);

    double recover_factor = rng(6, 9) / 10.0;
    steel_weight = (int)(steel_weight * recover_factor);

    g->sound(examx, examy, 80, _("Ka-klunk!"));

    int lump_weight = item_controller->find_template("steel_lump")->weight;
    int sheet_weight = item_controller->find_template("sheet_metal")->weight;
    int chunk_weight = item_controller->find_template("steel_chunk")->weight;
    int scrap_weight = item_controller->find_template("scrap")->weight;

    if (steel_weight < scrap_weight)
    {
        add_msg(_("The recycler chews up all the items in its hopper."));
        add_msg(_("The recycler beeps: \"No steel to process!\""));
        return;
    }

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
                add_msg(_("The recycler beeps: \"Insufficient steel!\""));
                add_msg(_("It spits out an assortment of smaller pieces instead."));
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
                add_msg(_("The recycler beeps: \"Insufficient steel!\""));
                add_msg(_("It spits out an assortment of smaller pieces instead."));
            }
            break;

        case 3: // 1 steel chunk = weight 340
            num_chunks = steel_weight / (chunk_weight);
            steel_weight -= num_chunks * (chunk_weight);
            num_scraps = steel_weight / (scrap_weight);
            if (num_chunks == 0)
            {
                add_msg(_("The recycler beeps: \"Insufficient steel!\""));
                add_msg(_("It spits out an assortment of smaller pieces instead."));
            }
            break;

        case 4: // 1 metal scrap = weight 113
            num_scraps = steel_weight / (scrap_weight);
            break;
    }

    for (int i = 0; i < num_lumps; i++)
    {
        m->spawn_item(p->posx, p->posy, "steel_lump");
    }

    for (int i = 0; i < num_sheets; i++)
    {
        m->spawn_item(p->posx, p->posy, "sheet_metal");
    }

    for (int i = 0; i < num_chunks; i++)
    {
        m->spawn_item(p->posx, p->posy, "steel_chunk");
    }

    for (int i = 0; i < num_scraps; i++)
    {
        m->spawn_item(p->posx, p->posy, "scrap");
    }
}

void iexamine::trap(player *p, map *m, int examx, int examy) {
    const trap_id tid = m->tr_at(examx, examy);
    if (p == NULL || !p->is_player() || tid == tr_null) {
        return;
    }
    const struct trap& t = *traplist[tid];
    if (t.can_see(*p, examx, examy) && query_yn(_("There is a %s there.  Disarm?"), t.name.c_str())) {
        m->disarm_trap(examx, examy);
    }
}

void iexamine::water_source(player *p, map *m, const int examx, const int examy)
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
        p->consume(p->inv.position_by_type(water.typeId()));
        p->moves -= 350;
    }
}
void iexamine::swater_source(player *p, map *m, const int examx, const int examy)
{
    item swater = m->swater_from(examx, examy);
    // Try to handle first (bottling) drink after.
    // changed boolean, large sources should be infinite
    if (g->handle_liquid(swater, true, true))
    {
        p->moves -= 100;
    }
    else if (query_yn(_("Drink from your hands?")))
    {
        p->inv.push_back(swater);
        p->consume(p->inv.position_by_type(swater.typeId()));
        p->moves -= 350;
    }
}
void iexamine::acid_source(player *p, map *m, const int examx, const int examy)
{
    item acid = m->acid_from(examx, examy);
    if (g->handle_liquid(acid, true, true))
    {
        p->moves -= 100;
    }
}

itype *furn_t::crafting_pseudo_item_type() const
{
    if (crafting_pseudo_item.empty()) {
        return NULL;
    }
    return item_controller->find_template(crafting_pseudo_item);
}

itype *furn_t::crafting_ammo_item_type() const
{
    const it_tool *toolt = dynamic_cast<const it_tool*>(crafting_pseudo_item_type());
    if (toolt != NULL && toolt->ammo != "NULL") {
        const std::string ammoid = default_ammo(toolt->ammo);
        return item_controller->find_template(ammoid);
    }
    return NULL;
}

size_t find_in_list(const itype *type, const std::vector<item> &items)
{
    for (size_t i = 0; i < items.size(); i++) {
        if (items[i].type == type) {
            return i;
        }
    }
    return static_cast<size_t>(-1);
}

long count_charges_in_list(const itype *type, const std::vector<item> &items)
{
    const size_t i = find_in_list(type, items);
    return (i == static_cast<size_t>(-1)) ? 0 : items[i].charges;
}

long remove_charges_in_list(const itype *type, std::vector<item> &items, long quantity)
{
    const size_t i = find_in_list(type, items);
    if (i != static_cast<size_t>(-1)) {
        if (items[i].charges > quantity) {
            items[i].charges -= quantity;
            return quantity;
        } else {
            const long charges = items[i].charges;
            items[i].charges = 0;
            if (items[i].destroyed_at_zero_charges()) {
                items.erase(items.begin() + i);
            }
            return charges;
        }
    }
    return 0;
}

void iexamine::reload_furniture(player *p, map *m, const int examx, const int examy)
{
    const furn_t &f = m->furn_at(examx, examy);
    itype *type = f.crafting_pseudo_item_type();
    itype *ammo = f.crafting_ammo_item_type();
    if (type == NULL || ammo == NULL) {
        add_msg(m_info, "This %s can not be reloaded!", f.name.c_str());
        return;
    }
    const int pos = p->inv.position_by_type(ammo->id);
    if (pos == INT_MIN) {
        const int amount = count_charges_in_list(ammo, m->i_at(examx, examy));
        if (amount > 0) {
            add_msg("The %s contains %d %s.", f.name.c_str(), amount, ammo->name.c_str());
        }
        add_msg(m_info, "You need some %s to reload this %s.", ammo->name.c_str(), f.name.c_str());
        return;
    }
    const long max_amount = p->inv.find_item(pos).charges;
    const std::string popupmsg = string_format(_("Put how many of the %s into the %s?"), ammo->name.c_str(), f.name.c_str());
    long amount = helper::to_int(
        string_input_popup(
            popupmsg, 20, helper::to_string_int(max_amount), "", "", -1, true));
    if (amount <= 0 || amount > max_amount) {
        return;
    }
    p->inv.reduce_charges(pos, amount);
    std::vector<item> &items = m->i_at(examx, examy);
    for (size_t i = 0; i < items.size(); i++) {
        if (items[i].type == ammo) {
            items[i].charges += amount;
            amount = 0;
            break;
        }
    }
    if (amount != 0) {
        item it(ammo, 0);
        it.charges = amount;
        items.push_back(it);
    }
    add_msg("You reload the %s.", m->furnname(examx, examy).c_str());
    p->moves -= 100;
}

/**
 * Given then name of one of the above functions, returns the matching function
 * pointer. If no match is found, defaults to iexamine::none but prints out a
 * debug message as a warning.
 * @param function_name The name of the function to get.
 * @return A function pointer to the specified function.
 */
void (iexamine::*iexamine_function_from_string(std::string function_name))(player*, map*, int, int) {
  if ("none" == function_name) {
    return &iexamine::none;
  }
  if ("gaspump" == function_name) {
    return &iexamine::gaspump;
  }
  if ("atm" == function_name) {
    return &iexamine::atm;
  }
  if ("vending" == function_name) {
    return &iexamine::vending;
  }
  if ("toilet" == function_name) {
    return &iexamine::toilet;
  }
  if ("elevator" == function_name) {
    return &iexamine::elevator;
  }
  if ("controls_gate" == function_name) {
    return &iexamine::controls_gate;
  }
  if ("cardreader" == function_name) {
    return &iexamine::cardreader;
  }
  if ("rubble" == function_name) {
    return &iexamine::rubble;
  }
  if ("chainfence" == function_name) {
    return &iexamine::chainfence;
  }
  if ("bars" == function_name) {
    return &iexamine::bars;
  }
  if ("tent" == function_name) {
    return &iexamine::tent;
  }
  if ("shelter" == function_name) {
    return &iexamine::shelter;
  }
  if ("wreckage" == function_name) {
    return &iexamine::wreckage;
  }
  if ("pit" == function_name) {
    return &iexamine::pit;
  }
  if ("pit_covered" == function_name) {
    return &iexamine::pit_covered;
  }
  if ("fence_post" == function_name) {
    return &iexamine::fence_post;
  }
  if ("remove_fence_rope" == function_name) {
    return &iexamine::remove_fence_rope;
  }
  if ("remove_fence_wire" == function_name) {
    return &iexamine::remove_fence_wire;
  }
  if ("remove_fence_barbed" == function_name) {
    return &iexamine::remove_fence_barbed;
  }
  if ("slot_machine" == function_name) {
    return &iexamine::slot_machine;
  }
  if ("safe" == function_name) {
    return &iexamine::safe;
  }
  if ("bulletin_board" == function_name) {
    return &iexamine::bulletin_board;
  }
  if ("fault" == function_name) {
    return &iexamine::fault;
  }
  if ("pedestal_wyrm" == function_name) {
    return &iexamine::pedestal_wyrm;
  }
  if ("pedestal_temple" == function_name) {
    return &iexamine::pedestal_temple;
  }
  if ("fswitch" == function_name) {
    return &iexamine::fswitch;
  }
  if ("flower_poppy" == function_name) {
    return &iexamine::flower_poppy;
  }
  if ("fungus" == function_name) {
    return &iexamine::fungus;
  }
  if ("flower_bluebell" == function_name) {
    return &iexamine::flower_blubell;
  }
  if ("flower_dahlia" == function_name) {
    return &iexamine::flower_dahlia;
  }
  if ("egg_sackbw" == function_name) {
    return &iexamine::egg_sackbw;
  }
  if ("egg_sackws" == function_name) {
    return &iexamine::egg_sackws;
  }
  if ("dirtmound" == function_name) {
    return &iexamine::dirtmound;
  }
  if ("aggie_plant" == function_name) {
    return &iexamine::aggie_plant;
  }
  if ("fvat_empty" == function_name) {
    return &iexamine::fvat_empty;
  }
  if ("fvat_full" == function_name) {
    return &iexamine::fvat_full;
  }
  if ("keg" == function_name) {
    return &iexamine::keg;
  }
  //pick_plant deliberately missing due to different function signature
  if ("tree_apple" == function_name) {
    return &iexamine::tree_apple;
  }
  if ("shrub_blueberry" == function_name) {
    return &iexamine::shrub_blueberry;
  }
  if ("shrub_strawberry" == function_name) {
    return &iexamine::shrub_strawberry;
  }
  if ("shrub_marloss" == function_name) {
    return &iexamine::shrub_marloss;
  }
  if ("shrub_wildveggies" == function_name) {
    return &iexamine::shrub_wildveggies;
  }
  if ("recycler" == function_name) {
    return &iexamine::recycler;
  }
  if ("trap" == function_name) {
    return &iexamine::trap;
  }
  if ("water_source" == function_name) {
    return &iexamine::water_source;
  }
  if ("swater_source" == function_name) {
    return &iexamine::swater_source;
  }
  if ("acid_source" == function_name) {
    return &iexamine::acid_source;
  }
  if ("reload_furniture" == function_name) {
    return &iexamine::reload_furniture;
  }

  //No match found
  debugmsg("Could not find an iexamine function matching '%s'!", function_name.c_str());
  return &iexamine::none;

}
