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

void iexamine::gaspump(player *p, map *m, int examx, int examy)
{
    if (!query_yn(_("Use the %s?"), m->tername(examx, examy).c_str())) {
        none(p, m, examx, examy);
        return;
    }

    auto items = m->i_at( examx, examy );
    for( auto item_it = items.begin(); item_it != items.end(); ++item_it ) {
        if( item_it->made_of(LIQUID) ) {
            if( one_in(10 + p->dex_cur) ) {
                add_msg(m_bad, _("You accidentally spill the %s."), item_it->type_name(1).c_str());
                item spill( item_it->type->id, calendar::turn );
                spill.charges = rng( dynamic_cast<it_ammo *>(item_it->type)->count,
                                     dynamic_cast<it_ammo *>(item_it->type)->count *
                                     (float)(8 / p->dex_cur) );
                m->add_item_or_charges( p->posx, p->posy, spill, 1 );
                item_it->charges -= spill.charges;
                if( item_it->charges < 1 ) {
                    items.erase( item_it );
                }
            } else {
                p->moves -= 300;
                if( g->handle_liquid( *item_it, true, false ) ) {
                    add_msg(_("With a clang and a shudder, the %s pump goes silent."),
                            item_it->type_name(1).c_str());
                    items.erase( item_it );
                }
            }
            return;
        }
    }
    add_msg(m_info, _("Out of order."));
}

void iexamine::atm(player *p, map *m, int examx, int examy)
{
    (void)m; //unused
    (void)examx; //unused
    (void)examy; //unused
    int choice = -1;
    const int purchase_cash_card = 1;
    const int deposit_money = 2;
    const int withdraw_money = 3;
    const int transfer_money = 4;
    const int transfer_all_money = 5;
    const int cancel = 0;
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
        amenu.addentry( transfer_all_money, true, -1, _("Transfer All Money") );
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
        popupmsg = string_format(ngettext("Deposit how much? Max:%d cent. (0 to cancel) ",
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
        add_msg(m_info, ngettext("Your account now holds %d cent.", "Your account now holds %d cents.",
                                 p->cash),
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
        std::string popupmsg = string_format(ngettext("Withdraw how much? Max:%d cent. (0 to cancel) ",
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
        std::string popupmsg = string_format(ngettext("Transfer how much? Max:%d cent. (0 to cancel) ",
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
            item card("cash_card", calendar::turn);
            it_tool *tool = dynamic_cast<it_tool *>(card.type);
            card.charges = tool->def_charges;
            p->i_add(card);
            p->cash -= 100;
            p->moves -= 100;
        }
    } else if (choice == transfer_all_money) {
        pos = g->inv(_("Insert card for bulk deposit."));
        dep = &(p->i_at(pos));
        if (dep->is_null()) {
            popup(_("You do not have that item!"));
            return;
        }
        if (dep->type->id != "cash_card") {
            popup(_("Please insert cash cards only!"));
            return;
        }

        //for all cash cards in inventory
        for (auto &elem : g->u.inv.all_items_by_type("cash_card")) {
            if (elem.first == dep) continue;
            dep->charges += elem.first->charges;
            elem.first->charges = 0;
            // Assuming a bulk interface for cards. Don't want to get people killed doing this.
            p->moves -= 10;
        }
    } else {
        return;
    }
}

void iexamine::vending(player *p, map *m, int examx, int examy)
{
    auto vend_items = m->i_at(examx, examy);
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

    const int iContentHeight = FULL_SCREEN_HEIGHT - 4;
    const int iHalf = iContentHeight / 2;
    const bool odd = iContentHeight % 2;

    const int w_width = FULL_SCREEN_WIDTH / 2 - 1;
    WINDOW *w = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH / 2 - 1,
                       (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0,
                       (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0);
    WINDOW *w_item_info = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH / 2,
                                 (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0,
                                 (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 + FULL_SCREEN_WIDTH / 2 :
                                 FULL_SCREEN_WIDTH / 2);

    bool used_machine = false;
    input_context ctxt("VENDING_MACHINE");
    ctxt.register_updown();
    ctxt.register_action("CONFIRM");
    ctxt.register_action("QUIT");
    ctxt.register_action("HELP_KEYBINDINGS");

    int cur_pos = 0;
    auto cur_item = vend_items.begin();
    while(true) {
        werase(w);
        wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
        for (int i = 1; i < FULL_SCREEN_WIDTH / 2 - 2; i++) {
            mvwaddch(w, 2, i, LINE_OXOX);
        }

        mvwaddch(w, 2, 0, LINE_XXXO); // |-
        mvwaddch(w, 2, w_width - 1, LINE_XOXX); // -|

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
        fold_and_print(w_item_info, 1, 2, 48 - 3, c_ltgray, cur_item->info(true));
        wborder(w_item_info, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
        mvwprintw(w_item_info, 0, 2, "< %s >", cur_item->display_name().c_str() );
        wrefresh(w_item_info);
        const std::string action = ctxt.handle_input();
        if (action == "DOWN") {
            cur_pos++;
            if (cur_pos >= num_items) {
                cur_pos = 0;
                cur_item = vend_items.begin();
            } else {
                cur_item++;
            }
        } else if (action == "UP") {
            cur_pos--;
            if (cur_pos < 0) {
                cur_pos = num_items - 1;
                cur_item = vend_items.end();
            }
            cur_item--;
        } else if (action == "CONFIRM") {
            if( cur_item->price() > card->charges ) {
                popup(_("That item is too expensive!"));
                continue;
            }
            card->charges -= cur_item->price();
            p->i_add_or_drop( *cur_item );
            cur_item = vend_items.erase( cur_item );
            if (cur_pos == (int)vend_items.size()) {
                cur_item = vend_items.end();
                cur_item--;
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

void iexamine::toilet(player *p, map *m, int examx, int examy)
{
    auto items = m->i_at(examx, examy);
    auto water = items.begin();
    for( ; water != items.end(); ++water ) {
        if( water->typeId() == "water") {
            break;
        }
    }

    if( water == items.end() ) {
        add_msg(m_info, _("This toilet is empty."));
    } else {
        int initial_charges = water->charges;
        // Use a different poison value each time water is drawn from the toilet.
        water->poison = one_in(3) ? 0 : rng(1, 3);

        // First try handling/bottling, then try drinking, but only try
        // drinking if we don't handle or bottle.
        bool drained = g->handle_liquid( *water, true, false );
        if( drained || initial_charges != water->charges ) {
            // The bottling happens in handle_liquid, but delay of action
            // does not.
            p->moves -= 100;
        } else if( !drained && initial_charges == water->charges ){
            int charges_consumed = p->drink_from_hands( *water );
            // Drink_from_hands handles moves, but doesn't decrease water
            // charges.
            water->charges -= charges_consumed;
        }

        if( drained || water->charges <= 0 ) {
            items.erase( water );
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

void iexamine::controls_gate(player *p, map *m, int examx, int examy)
{
    if (!query_yn(_("Use the %s?"), m->tername(examx, examy).c_str())) {
        none(p, m, examx, examy);
        return;
    }
    g->open_gate(examx, examy, (ter_id)m->ter(examx, examy));
}

void iexamine::cardreader(player *p, map *m, int examx, int examy)
{
    itype_id card_type = (m->ter(examx, examy) == t_card_science ? "id_science" :
                          "id_military");
    if (p->has_amount(card_type, 1) && query_yn(_("Swipe your ID card?"))) {
        p->moves -= 100;
        for (int i = -3; i <= 3; i++) {
            for (int j = -3; j <= 3; j++) {
                if (m->ter(examx + i, examy + j) == t_door_metal_locked) {
                    m->ter_set(examx + i, examy + j, t_floor);
                }
            }
        }
        for (int i = 0; i < (int)g->num_zombies(); i++) {
            if ( (g->zombie(i).type->id == "mon_turret") ||
                 (g->zombie(i).type->id == "mon_turret_rifle") ) {
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
            p->practice( "computer", 20 );
            int success = rng(p->skillLevel("computer") / 4 - 2, p->skillLevel("computer") * 2);
            success += rng(-3, 3);
            if (using_fingerhack) {
                success++;
            }
            if (p->int_cur < 8) {
                success -= rng(0, int((8 - p->int_cur) / 2));
            } else if (p->int_cur > 8) {
                success += rng(0, int((p->int_cur - 8) / 2));
            }
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
            } else if (success < 6) {
                add_msg(_("Nothing happens."));
            } else {
                add_msg(_("You activate the panel!"));
                add_msg(m_good, _("The nearby doors slide into the floor."));
                m->ter_set(examx, examy, t_card_reader_broken);
                for (int i = -3; i <= 3; i++) {
                    for (int j = -3; j <= 3; j++) {
                        if (m->ter(examx + i, examy + j) == t_door_metal_locked) {
                            m->ter_set(examx + i, examy + j, t_floor);
                        }
                    }
                }
            }
        } else {
            add_msg(m_info, _("Looks like you need a %s."), item::nname( card_type ).c_str());
        }
    }
}

void iexamine::rubble(player *p, map *m, int examx, int examy)
{
    bool has_digging_tool = p->has_items_with_quality( "DIG", 2, 1 );
  // Perhaps check for vehicle covering the rubble and bail out if so (string freeze ATM)?
    if (!has_digging_tool) {
        add_msg(m_info, _("If only you had a shovel..."));
        return;
    }
    std::string xname = m->furnname(examx, examy);
    if (query_yn(_("Clear up that %s?"), xname.c_str())) {
        // "Remove"
        p->moves -= 200;
        m->furn_set(examx, examy, f_null);

        // "Remind"
        add_msg(_("You clear up that %s."), xname.c_str());
    }
}

void iexamine::chainfence( player *p, map *m, int examx, int examy )
{
    if( !query_yn( _( "Climb %s?" ), m->tername( examx, examy ).c_str() ) ) {
        none( p, m, examx, examy );
        return;
    }
    if( p->has_trait( "ARACHNID_ARMS_OK" ) && !p->wearing_something_on( bp_torso ) ) {
        add_msg( _( "Climbing the fence is trivial for one such as you." ) );
        p->moves -= 75; // Yes, faster than walking.  6-8 limbs are impressive.
    } else if( p->has_trait( "INSECT_ARMS_OK" ) && !p->wearing_something_on( bp_torso ) ) {
        add_msg( _( "You quickly scale the fence." ) );
        p->moves -= 90;
    } else if( p->has_trait( "PARKOUR" ) ) {
        add_msg( _( "The fence is no match for your freerunning abilities." ) );
        p->moves -= 100;
    } else {
        p->moves -= 400;
        int climb = p->dex_cur;
        if (p->has_trait( "BADKNEES" )) {
            climb = climb / 2;
        }
        if( one_in( climb ) ) {
            add_msg( m_bad, _( "You slip while climbing and fall down again." ) );
            return;
        }
        p->moves += climb * 10;
    }
    if( p->in_vehicle ) {
        m->unboard_vehicle( p->posx, p->posy );
    }
    if( examx < SEEX * int( MAPSIZE / 2 ) || examy < SEEY * int( MAPSIZE / 2 ) ||
        examx >= SEEX * ( 1 + int( MAPSIZE / 2 ) ) || examy >= SEEY * ( 1 + int( MAPSIZE / 2 ) ) ) {
        if( &g->u == p ) {
            g->update_map( examx, examy );
        }
    }
    p->posx = examx;
    p->posy = examy;
}

void iexamine::bars(player *p, map *m, int examx, int examy)
{
    if(!(p->has_trait("AMORPHOUS"))) {
        none(p, m, examx, examy);
        return;
    }
    if ( ((p->encumb(bp_torso)) >= 1) && ((p->encumb(bp_head)) >= 1) &&
         (p->encumb(bp_foot_l) >= 1 ||
          p->encumb(bp_foot_r) >= 1) ) { // Most likely places for rigid gear that would catch on the bars.
        add_msg(m_info, _("Your amorphous body could slip though the %s, but your cumbersome gear can't."),
                m->tername(examx, examy).c_str());
        return;
    }
    if (!query_yn(_("Slip through the %s?"), m->tername(examx, examy).c_str())) {
        none(p, m, examx, examy);
        return;
    }
    p->moves -= 200;
    add_msg(_("You slide right between the bars."));
    p->posx = examx;
    p->posy = examy;
}

void iexamine::portable_structure(player *p, map *m, int examx, int examy)
{
    int radius = m->furn(examx, examy) == f_center_groundsheet ? 2 : 1;
    const char *name = m->furn(examx, examy) == f_skin_groundsheet ? _("shelter") : _("tent");
      // We don't take the name from the item in case "kit" is in
      // it, instead of just the name of the structure.
    std::string dropped =
        m->furn(examx, examy) == f_groundsheet        ? "tent_kit"
      : m->furn(examx, examy) == f_center_groundsheet ? "large_tent_kit"
      :                                                 "shelter_kit";
    if (!query_yn(_("Take down the %s?"), name)) {
        none(p, m, examx, examy);
        return;
    }
    p->moves -= 200;
    for (int i = -radius; i <= radius; i++) {
        for (int j = -radius; j <= radius; j++) {
            m->furn_set(examx + i, examy + j, f_null);
        }
    }
    add_msg(_("You take down the %s."), name);
    m->add_item_or_charges(examx, examy, item(dropped, calendar::turn));
}

void iexamine::pit(player *p, map *m, int examx, int examy)
{
    inventory map_inv;
    map_inv.form_from_map(point(p->posx, p->posy), 1);

    bool player_has = p->has_amount("2x4", 1);
    bool map_has = map_inv.has_amount("2x4", 1);

    // return if there is no 2x4 around
    if (!player_has && !map_has) {
        none(p, m, examx, examy);
        return;
    }

    if (query_yn(_("Place a plank over the pit?"))) {
        // if both have, then ask to use the one on the map
        if (player_has && map_has) {
            if (query_yn(_("Use the plank at your feet?"))) {
                m->use_amount(point(p->posx, p->posy), 1, "2x4", 1, false);
            } else {
                p->use_amount("2x4", 1);
            }
        } else if (player_has && !map_has) { // only player has plank
            p->use_amount("2x4", 1);
        } else if (!player_has && map_has) { // only map has plank
            m->use_amount(point(p->posx, p->posy), 1, "2x4", 1, false);
        }

        if( m->ter(examx, examy) == t_pit ) {
            m->ter_set(examx, examy, t_pit_covered);
        } else if( m->ter(examx, examy) == t_pit_spiked ) {
            m->ter_set(examx, examy, t_pit_spiked_covered);
        } else if( m->ter(examx, examy) == t_pit_glass ) {
            m->ter_set(examx, examy, t_pit_glass_covered);
        }
        add_msg(_("You place a plank of wood over the pit."));
    }
}

void iexamine::pit_covered(player *p, map *m, int examx, int examy)
{
    if(!query_yn(_("Remove cover?"))) {
        none(p, m, examx, examy);
        return;
    }

    item plank("2x4", calendar::turn);
    add_msg(_("You remove the plank."));
    m->add_item_or_charges(p->posx, p->posy, plank);

    if( m->ter(examx, examy) == t_pit_covered ) {
        m->ter_set(examx, examy, t_pit);
    } else if( m->ter(examx, examy) == t_pit_spiked_covered ) {
        m->ter_set(examx, examy, t_pit_spiked);
    } else if( m->ter(examx, examy) == t_pit_glass_covered ) {
        m->ter_set(examx, examy, t_pit_glass);
    }
}

void iexamine::fence_post(player *p, map *m, int examx, int examy)
{

    int ch = menu(true, _("Fence Construction:"), _("Rope Fence"),
                  _("Wire Fence"), _("Barbed Wire Fence"), _("Cancel"), NULL);
    switch (ch) {
    case 1: {
        if (p->has_amount("rope_6", 2)) {
            p->use_amount("rope_6", 2);
            m->ter_set(examx, examy, t_fence_rope);
            p->moves -= 200;
        } else {
            add_msg(m_info, _("You need 2 six-foot lengths of rope to do that"));
        }
    }
    break;

    case 2: {
        if (p->has_amount("wire", 2)) {
            p->use_amount("wire", 2);
            m->ter_set(examx, examy, t_fence_wire);
            p->moves -= 200;
        } else {
            add_msg(m_info, _("You need 2 lengths of wire to do that!"));
        }
    }
    break;

    case 3: {
        if (p->has_amount("wire_barbed", 2)) {
            p->use_amount("wire_barbed", 2);
            m->ter_set(examx, examy, t_fence_barbed);
            p->moves -= 200;
        } else {
            add_msg(m_info, _("You need 2 lengths of barbed wire to do that!"));
        }
    }
    break;

    case 4:
    default:
        break;
    }
}

void iexamine::remove_fence_rope(player *p, map *m, int examx, int examy)
{
    if(!query_yn(_("Remove %s?"), m->tername(examx, examy).c_str())) {
        none(p, m, examx, examy);
        return;
    }
    item rope("rope_6", calendar::turn);
    m->add_item_or_charges(p->posx, p->posy, rope);
    m->add_item_or_charges(p->posx, p->posy, rope);
    m->ter_set(examx, examy, t_fence_post);
    p->moves -= 200;

}

void iexamine::remove_fence_wire(player *p, map *m, int examx, int examy)
{
    if(!query_yn(_("Remove %s?"), m->tername(examx, examy).c_str())) {
        none(p, m, examx, examy);
        return;
    }

    item rope("wire", calendar::turn);
    m->add_item_or_charges(p->posx, p->posy, rope);
    m->add_item_or_charges(p->posx, p->posy, rope);
    m->ter_set(examx, examy, t_fence_post);
    p->moves -= 200;
}

void iexamine::remove_fence_barbed(player *p, map *m, int examx, int examy)
{
    if(!query_yn(_("Remove %s?"), m->tername(examx, examy).c_str())) {
        none(p, m, examx, examy);
        return;
    }

    item rope("wire_barbed", calendar::turn);
    m->add_item_or_charges(p->posx, p->posy, rope);
    m->add_item_or_charges(p->posx, p->posy, rope);
    m->ter_set(examx, examy, t_fence_post);
    p->moves -= 200;
}

void iexamine::slot_machine(player *p, map *m, int examx, int examy)
{
    (void)m;
    (void)examx;
    (void)examy; //unused
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

void iexamine::safe(player *p, map *m, int examx, int examy)
{
    if (!p->has_amount("stethoscope", 1)) {
        add_msg(m_info, _("You need a stethoscope for safecracking."));
        return;
    }

    if (query_yn(_("Attempt to crack the safe?"))) {
        bool success = true;
        if (p->is_deaf()) {
            add_msg(m_info, _("You can't crack a safe while deaf!"));
            return;
        }

        if (success) {
            m->furn_set(examx, examy, f_safe_o);
            add_msg(m_good, _("You successfully crack the safe!"));
        } else {
            add_msg(_("The safe resists your attempt at cracking it."));
        }
    }
}

void iexamine::gunsafe_ml(player *p, map *m, int examx, int examy)
{
    std::string furn_name = m->tername(examx, examy).c_str();
    if( !( p->has_amount("crude_picklock", 1) || p->has_amount("hairpin", 1) ||
           p->has_amount("picklocks", 1) || p->has_bionic("bio_lockpick") ) ) {
        add_msg(m_info, _("You need a lockpick to open this gun safe."));
        return;
    } else if( !query_yn(_("Pick the gun safe?")) ) {
        return;
    }

    int pick_quality = 1;
    if( p->has_amount("picklocks", 1) || p->has_bionic("bio_lockpick") ) {
        pick_quality = 5;
    } else {
        pick_quality = 3;
    }

    p->practice("mechanics", 1);
    p->moves -= (1000 - (pick_quality * 100)) - (p->dex_cur + p->skillLevel("mechanics")) * 5;
    int pick_roll = (dice(2, p->skillLevel("mechanics")) + dice(2, p->dex_cur)) * pick_quality;
    int door_roll = dice(4, 30);
    if (pick_roll >= door_roll) {
        p->practice("mechanics", 1);
        add_msg(_("You successfully unlock the gun safe."));
        g->m.furn_set(examx, examy, "f_safe_o");
    } else if (door_roll > (3 * pick_roll)) {
        add_msg(_("Your clumsy attempt jams the lock!"));
        g->m.furn_set(examx, examy, "f_gunsafe_mj");
    } else {
        add_msg(_("The gun safe stumps your efforts to pick it."));
    }
}

void iexamine::gunsafe_el(player *p, map *m, int examx, int examy)
{
    std::string furn_name = m->tername(examx, examy).c_str();
    bool can_hack = ( !p->has_trait("ILLITERATE") &&
                      ( (p->has_amount("electrohack", 1)) ||
                        (p->has_bionic("bio_fingerhack") && p->power_level > 0) ) );
    if (!can_hack) {
        add_msg(_("You can't hack this gun safe without an electrohack."));
        return;
    }

    bool using_electrohack = (p->has_amount("electrohack", 1) &&
                              query_yn(_("Use electrohack on the gun safe?")));
    bool using_fingerhack = (!using_electrohack && p->has_bionic("bio_fingerhack") &&
                             p->power_level > 0 && query_yn(_("Use fingerhack on the gun safe?")));
    if (using_electrohack || using_fingerhack) {
        p->moves -= 500;
        p->practice("computer", 20);
        int success = rng(p->skillLevel("computer") / 4 - 2, p->skillLevel("computer") * 2);
        success += rng(-3, 3);
        if (using_fingerhack) {
            success++;
        }
        if (p->int_cur < 8) {
            success -= rng(0, int((8 - p->int_cur) / 2));
        } else if (p->int_cur > 8) {
            success += rng(0, int((p->int_cur - 8) / 2));
        }
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
            p->add_memorial_log(pgettext("memorial_male", "Set off an alarm."),
                                pgettext("memorial_female", "Set off an alarm."));
            g->sound(p->posx, p->posy, 60, _("An alarm sounds!"));
            if (g->levz > 0 && !g->event_queued(EVENT_WANTED)) {
                g->add_event(EVENT_WANTED, int(calendar::turn) + 300, 0, g->levx, g->levy);
            }
        } else if (success < 6) {
            add_msg(_("Nothing happens."));
        } else {
            add_msg(_("You successfully hack the gun safe."));
            g->m.furn_set(examx, examy, "f_safe_o");
        }
    }
}

void iexamine::bulletin_board(player *p, map *m, int examx, int examy)
{
    (void)p;
    basecamp *camp = m->camp_at(examx, examy);
    if (camp && camp->board_x() == examx && camp->board_y() == examy) {
        std::vector<std::string> options;
        options.push_back(_("Cancel"));
        // Causes a warning due to being unused, but don't want to delete
        // since it's clearly what's intended for future functionality.
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
        if (choice >= 0 && size_t(choice) < options.size()) {
            if (options[choice] == _("Create camp")) {
                // TODO: Allow text entry for name
                m->add_camp(_("Home"), examx, examy);
            }
        }
    }
}

void iexamine::fault(player *p, map *m, int examx, int examy)
{
    (void)p;
    (void)m;
    (void)examx;
    (void)examy; //unused
    popup(_("\
This wall is perfectly vertical.  Odd, twisted holes are set in it, leading\n\
as far back into the solid rock as you can see.  The holes are humanoid in\n\
shape, but with long, twisted, distended limbs."));
}

void iexamine::pedestal_wyrm(player *p, map *m, int examx, int examy)
{
    if (!m->i_at(examx, examy).empty()) {
        none(p, m, examx, examy);
        return;
    }
    // Send in a few wyrms to start things off.
    g->u.add_memorial_log(pgettext("memorial_male", "Awoke a group of dark wyrms!"),
                         pgettext("memorial_female", "Awoke a group of dark wyrms!"));
   monster wyrm(GetMType("mon_dark_wyrm"));
   int num_wyrms = rng(1, 4);
   for (int i = 0; i < num_wyrms; i++) {
    int tries = 0;
    int monx = -1, mony = -1;
    do {
     monx = rng(0, SEEX * MAPSIZE);
     mony = rng(0, SEEY * MAPSIZE);
     tries++;
    } while (tries < 10 && !g->is_empty(monx, mony) &&
             rl_dist(g->u.posx, g->u.posy, monx, mony) <= 2);
      if (tries < 10) {
          g->m.ter_set(monx, mony, t_rock_floor);
          wyrm.spawn(monx, mony);
          g->add_zombie(wyrm);
      }
   }
    add_msg(_("The pedestal sinks into the ground, with an ominous grinding noise..."));
    g->sound(examx, examy, 80, (""));
    m->ter_set(examx, examy, t_rock_floor);
    g->add_event(EVENT_SPAWN_WYRMS, int(calendar::turn) + rng(5, 10));
}

void iexamine::pedestal_temple(player *p, map *m, int examx, int examy)
{

    if (m->i_at(examx, examy).size() == 1 &&
        m->i_at(examx, examy)[0].type->id == "petrified_eye") {
        add_msg(_("The pedestal sinks into the ground..."));
        m->ter_set(examx, examy, t_dirt);
        m->i_clear(examx, examy);
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

void iexamine::door_peephole(player *p, map *m, int examx, int examy) {
    if (m->is_outside(p->posx, p->posy)) {
        p->add_msg_if_player( _("You cannot look through the peephole from the outside."));
        return;
    }

    // Peek through the peephole, or open the door.
    int choice = menu( true, _("Do what with the door?"),
                       _("Peek through peephole."), _("Open door."),
                       _("Cancel"), NULL );
    if( choice == 1 ) {
        // Peek
        g->peek( examx, examy );
        p->add_msg_if_player( _("You peek through the peephole.") );
    } else if( choice == 2 ) {
        m->open_door(examx, examy, true, false);
        p->add_msg_if_player( _("You open the door.") );
    } else {
        p->add_msg_if_player( _("Never mind."));
    }
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

void iexamine::flower_poppy(player *p, map *m, int examx, int examy)
{
    if (calendar::turn.get_season() == WINTER) {
        add_msg(m_info, _("This flower is dead. You can't get it."));
        none(p, m, examx, examy);
        return;
    }
    if ( ((p->has_trait("PROBOSCIS")) || (p->has_trait("BEAK_HUM"))) && ((p->hunger) > 0) &&
         (!(p->wearing_something_on(bp_mouth))) ) {
        if (!query_yn(_("You feel woozy as you explore the %s. Drink?"), m->furnname(examx,
                      examy).c_str())) {
            return;
        }
        p->moves -= 150; // You take your time...
        add_msg(_("You slowly suck up the nectar."));
        p->hunger -= 25;
        p->add_effect("pkill2", 70);
        p->fatigue += 20;
        // Please drink poppy nectar responsibly.
        if (one_in(20)) {
            p->add_addiction(ADD_PKILLER, 1);
        }
    }
    if(!query_yn(_("Pick %s?"), m->furnname(examx, examy).c_str())) {
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
        p->apply_damage(nullptr, bp_leg_l, 4);
        p->apply_damage(nullptr, bp_leg_r, 4);
        p->moves -= 50;
    }

    m->furn_set(examx, examy, f_null);
    m->spawn_item(examx, examy, "poppy_flower");
    m->spawn_item(examx, examy, "poppy_bud");
}

void iexamine::flower_blubell(player *p, map *m, int examx, int examy)
{
    if (calendar::turn.get_season() == WINTER) {
        add_msg(m_info, _("This flower is dead. You can't get it."));
        none(p, m, examx, examy);
        return;
    }
    if ( ((p->has_trait("PROBOSCIS")) || (p->has_trait("BEAK_HUM"))) &&
         ((p->hunger) > 0) && (!(p->wearing_something_on(bp_mouth))) ) {
        p->moves -= 50; // Takes 30 seconds
        add_msg(_("You drink some nectar."));
        p->hunger -= 15;
    }
    if(!query_yn(_("Pick %s?"), m->furnname(examx, examy).c_str())) {
        none(p, m, examx, examy);
        return;
    }
    m->furn_set(examx, examy, f_null);
    m->spawn_item(examx, examy, "bluebell_flower");
    m->spawn_item(examx, examy, "bluebell_bud");
}

void iexamine::flower_dahlia(player *p, map *m, int examx, int examy)
{
    if (calendar::turn.get_season() == WINTER) {
        add_msg(m_info, _("This flower is dead. You can't get it."));
        none(p, m, examx, examy);
        return;
    }
    if ( ((p->has_trait("PROBOSCIS")) || (p->has_trait("BEAK_HUM"))) &&
         ((p->hunger) > 0) && (!(p->wearing_something_on(bp_mouth))) ) {
        p->moves -= 50; // Takes 30 seconds
        add_msg(_("You drink some nectar."));
        p->hunger -= 15;
    }
    if(!query_yn(_("Pick %s?"), m->furnname(examx, examy).c_str())) {
        none(p, m, examx, examy);
        return;
    }
    m->furn_set(examx, examy, f_null);
    m->spawn_item(examx, examy, "dahlia_flower");
    m->spawn_item(examx, examy, "dahlia_bud");
    if( p->has_items_with_quality( "DIG", 1, 1 ) ) {
        m->spawn_item(examx, examy, "dahlia_root");
    } else {
        add_msg( m_info, _( "If only you had a shovel to dig up those roots..." ) );
    }
}

void iexamine::flower_datura(player *p, map *m, int examx, int examy)
{
    if (calendar::turn.get_season() == WINTER) {
        add_msg(m_info, _("This plant is dead. You can't get it."));
        none(p, m, examx, examy);
        return;
    }
    if ( ((p->has_trait("PROBOSCIS")) || (p->has_trait("BEAK_HUM"))) &&
         ((p->hunger) > 0) && (!(p->wearing_something_on(bp_mouth))) ) {
        p->moves -= 50; // Takes 30 seconds
        add_msg(_("You drink some nectar."));
        p->hunger -= 15;
    }
    if(!query_yn(_("Pick %s?"), m->furnname(examx, examy).c_str())) {
        none(p, m, examx, examy);
        return;
    }
    m->furn_set(examx, examy, f_null);
    m->spawn_item(examx, examy, "datura_seed", 2, 6 );
}

void iexamine::flower_dandelion(player *p, map *m, int examx, int examy)
{
    if (calendar::turn.get_season() == WINTER) {
        add_msg(m_info, _("This plant is dead. You can't get it."));
        none(p, m, examx, examy);
        return;
    }
    if ( ((p->has_trait("PROBOSCIS")) || (p->has_trait("BEAK_HUM"))) &&
         ((p->hunger) > 0) && (!(p->wearing_something_on(bp_mouth))) ) {
        p->moves -= 50; // Takes 30 seconds
        add_msg(_("You drink some nectar."));
        p->hunger -= 15;
    }
    if(!query_yn(_("Pick %s?"), m->furnname(examx, examy).c_str())) {
        none(p, m, examx, examy);
        return;
    }
    m->furn_set(examx, examy, f_null);
    m->spawn_item(examx, examy, "raw_dandelion", rng( 1, 4 ) );
}

void iexamine::flower_marloss(player *p, map *m, int examx, int examy)
{
    if (calendar::turn.get_season() == WINTER) {
        add_msg(m_info, _("This flower is still alive, despite the harsh conditions..."));
    }
    if ( ((p->has_trait("PROBOSCIS")) || (p->has_trait("BEAK_HUM"))) &&
         ((p->hunger) > 0) ) {
            if (!(p->wearing_something_on(bp_mouth))) {
                if (!query_yn(_("You feel out of place as you explore the %s. Drink?"), m->furnname(examx,
                      examy).c_str())) {
            return;
        }
            p->moves -= 50; // Takes 30 seconds
            add_msg(m_bad, _("This flower tastes very wrong..."));
            // If you can drink flowers, you're post-thresh and the Mycus does not want you.
            p->add_effect("teleglow", 100);
        }
    }
    if(!query_yn(_("Pick %s?"), m->furnname(examx, examy).c_str())) {
        none(p, m, examx, examy);
        return;
    }
    m->furn_set(examx, examy, f_null);
    m->spawn_item(examx, examy, "marloss_seed", 1, 3);
}

void iexamine::egg_sack_generic( player *p, map *m, int examx, int examy,
                                 const std::string &montype )
{
    const std::string old_furn_name = m->furnname( examx, examy );
    if( !query_yn( _( "Harvest the %s?" ), old_furn_name.c_str() ) ) {
        none( p, m, examx, examy );
        return;
    }
    m->spawn_item( examx, examy, "spider_egg", rng( 1, 4 ) );
    m->furn_set( examx, examy, f_egg_sacke );
    if( one_in( 2 ) ) {
        monster spiderling( GetMType( montype ) );
        int monster_count = 0;
        const std::vector<point> points = closest_points_first( 1, point( examx, examy ) );
        for( const auto &point : points ) {
            if( g->is_empty( point.x, point.y ) && one_in( 3 ) ) {
                spiderling.spawn( point.x, point.y );
                g->add_zombie( spiderling );
                monster_count++;
            }
        }
        if( monster_count == 1 ) {
            add_msg( m_warning, _( "A spiderling bursts from the %s!" ), old_furn_name.c_str() );
        } else if( monster_count >= 1 ) {
            add_msg( m_warning, _( "Spiderlings burst from the %s!" ), old_furn_name.c_str() );
        }
    }
}

void iexamine::egg_sackbw( player *p, map *m, int examx, int examy )
{
    egg_sack_generic( p, m, examx, examy, "mon_spider_widow_giant_s" );
}

void iexamine::egg_sackws( player *p, map *m, int examx, int examy )
{
    egg_sack_generic( p, m, examx, examy, "mon_spider_web_s" );
}

void iexamine::fungus(player *p, map *m, int examx, int examy)
{
    add_msg(_("The %s crumbles into spores!"), m->furnname(examx, examy).c_str());
    m->create_spores(examx, examy, p);
    m->furn_set(examx, examy, f_null);
    p->moves -= 50;
}

void iexamine::dirtmound(player *p, map *m, int examx, int examy)
{

    if (g->get_temperature() < 50) { // semi-appropriate temperature for most plants
        add_msg(m_info, _("It is too cold to plant anything now."));
        return;
    }
    /* ambient_light_at() not working?
    if (m->ambient_light_at(examx, examy) < LIGHT_AMBIENT_LOW) {
        add_msg(m_info, _("It is too dark to plant anything now."));
        return;
    }*/
    std::vector<const item *> seed_inv = p->all_items_with_flag( "SEED" );
    if( seed_inv.empty() ) {
        add_msg(m_info, _("You have no seeds to plant."));
        return;
    }
    if (m->i_at(examx, examy).size() != 0) {
        add_msg(_("Something's lying there..."));
        return;
    }

    // Make lists of unique seed types and names for the menu(no multiple hemp seeds etc)
    std::vector<itype_id> seed_types;
    std::vector<std::string> seed_names;
    for( auto &seed : seed_inv ) {
        if( std::find( seed_types.begin(), seed_types.end(), seed->typeId() ) == seed_types.end() ) {
            seed_types.push_back( seed->typeId() );
            seed_names.push_back( seed->tname() );
        }
    }

    // Choose seed if applicable
    int seed_index = 0;
    if (seed_types.size() > 1) {
        seed_names.push_back("Cancel");
        seed_index = menu_vec(false, _("Use which seed?"),
                              seed_names) - 1; // TODO: make cancelable using ESC
        if (seed_index == (int)seed_names.size() - 1) {
            seed_index = -1;
        }
    } else {
        if (!query_yn(_("Plant %s here?"), seed_names[0].c_str())) {
            seed_index = -1;
        }
    }

    // Did we cancel?
    if (seed_index < 0) {
        add_msg(_("You saved your seeds for later.")); // huehuehue
        return;
    }

    // Actual planting
    std::list<item> planted = p->use_charges( seed_types[seed_index], 1 );
    m->spawn_item(examx, examy, seed_types[seed_index], 1, 1, calendar::turn);
    m->set(examx, examy, t_dirt, f_plant_seed);
    p->moves -= 500;
    add_msg(_("Planted %s"), seed_names[seed_index].c_str());
}

void iexamine::aggie_plant(player *p, map *m, int examx, int examy)
{
    if (m->furn(examx, examy) == f_plant_harvest && query_yn(_("Harvest plant?"))) {
        if (m->i_at(examx, examy).empty()) {
            m->i_clear(examx, examy);
            m->furn_set(examx, examy, f_null);
            debugmsg("Missing seeds in harvested plant!");
            return;
        }
        itype_id seedType = m->i_at(examx, examy)[0].typeId();
        if (seedType == "fungal_seeds") {
            fungus(p, m, examx, examy);
            m->i_clear(examx, examy);
        } else if (seedType == "marloss_seed") {
            fungus(p, m, examx, examy);
            m->i_clear(examx, examy);
            if (p->has_trait("M_DEPENDENT") && ((p->hunger > 500) || p->thirst > 300 )) {
                m->ter_set(examx, examy, t_marloss);
                add_msg(m_info, _("We have altered this unit's configuration to extract and provide local nutriment.  The Mycus provides."));
            } else if ( (p->has_trait("M_DEFENDER")) || ( (p->has_trait("M_SPORES") || p->has_trait("M_FERTILE")) &&
              one_in(2)) ) {
                m->add_spawn("mon_fungal_blossom", 1, examx, examy);
                add_msg(m_info, _("The seed blooms forth!  We have brought true beauty to this world."));
            } else if ( (p->has_trait("THRESH_MYCUS")) || one_in(4)) {
                m->furn_set(examx, examy, f_flower_marloss);
                add_msg(m_info, _("The seed blossoms rather rapidly..."));
            } else {
                m->furn_set(examx, examy, f_flower_fungal);
                add_msg(m_info, _("The seed blossoms into a flower-looking fungus."));
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
            if( item::count_by_charges( seedType ) ) {
                m->spawn_item(examx, examy, seedType, 1, rng(plantCount / 4, plantCount / 2));
            } else {
                m->spawn_item(examx, examy, seedType, rng(plantCount / 4, plantCount / 2));
            }

            if ((seedType == "seed_wheat") || (seedType == "seed_barley") ||
                (seedType == "seed_hops")) {
                m->spawn_item(examx, examy, "straw_pile");
            } else if (seedType != "seed_sugar_beet") {
                m->spawn_item(examx, examy, "withered");
            }
            p->moves -= 500;
        }
    } else if (m->furn(examx, examy) != f_plant_harvest) {
        if (m->i_at(examx, examy).size() > 1) {
            add_msg(m_info, _("This plant has already been fertilized."));
            return;
        }
        std::vector<const item *> f_inv = p->all_items_with_flag( "FERTILIZER" );
        if( f_inv.empty() ) {
        add_msg(m_info, _("You have no fertilizer."));
        return;
        }
        if (query_yn(_("Fertilize plant"))) {
        std::vector<itype_id> f_types;
        std::vector<std::string> f_names;
            for( auto &f : f_inv ) {
                if( std::find( f_types.begin(), f_types.end(), f->typeId() ) == f_types.end() ) {
                    f_types.push_back( f->typeId() );
                    f_names.push_back( f->tname() );
                }
            }
            // Choose fertilizer from list
            int f_index = 0;
            if (f_types.size() > 1) {
                f_names.push_back("Cancel");
                f_index = menu_vec(false, _("Use which fertilizer?"), f_names) - 1;
                if (f_index == (int)f_names.size() - 1) {
                    f_index = -1;
                }
            } else {
                    f_index = 0;
            }
            if (f_index < 0) {
                return;
            }
            std::list<item> planted = p->use_charges( f_types[f_index], 1 );
            if (planted.empty()) { // nothing was removed from inv => weapon is the SEED
                if (p->weapon.charges > 1) {
                    p->weapon.charges--;
                } else {
                    p->remove_weapon();
                }
            }
            // Reduce the amount of time it takes until the next stage of the plant by
            // 20% of a seasons length. (default 2.8 days).
            WORLDPTR world = world_generator->active_world;
            int fertilizerEpoch = 14400 * 2; //default if options is empty for some reason.
            if (!world->world_options.empty()) {
                fertilizerEpoch = 14400 * (world->world_options["SEASON_LENGTH"] * 0.2) ;
            }

            item &seed = m->i_at( examx, examy ).front();
            if( seed.bday > fertilizerEpoch ) {
                seed.bday -= fertilizerEpoch;
            } else {
                seed.bday = 0;
            }
            m->spawn_item( examx, examy, "fertilizer", 1, 1, (int)calendar::turn );
        }
    }
}

void iexamine::fvat_empty(player *p, map *m, int examx, int examy)
{
    itype_id brew_type;
    bool to_deposit = false;
    bool vat_full = false;
    bool brew_present = false;
    int charges_on_ground = 0;
    auto items = m->i_at(examx, examy);
    for( auto item_it = items.begin(); item_it != items.end(); ) {
        if( !item_it->has_flag("BREW") || brew_present ) {
            // This isn't a brew or there was already another kind of brew inside,
            // so this has to be moved.
            items.push_back( *item_it );
            // This will add items to a space near the vat, because it's flagged as NOITEM.
            item_it = items.erase( item_it );
        } else {
            item_it++;
            brew_present = true;
        }
    }
    if (!brew_present) {
        std::vector<const item *> b_inv = p->all_items_with_flag( "BREW" );
        if( b_inv.empty() ) {
            add_msg(m_info, _("You have no brew to ferment."));
            return;
        }
        // Make lists of unique typeids and names for the menu
        // Code shamelessly stolen from the crop planting function!
        std::vector<itype_id> b_types;
        std::vector<std::string> b_names;
        for( auto &b : b_inv ) {
            if( std::find( b_types.begin(), b_types.end(), b->typeId() ) == b_types.end() ) {
                b_types.push_back( b->typeId() );
                b_names.push_back( b->tname() );
            }
        }
        // Choose brew from list
        int b_index = 0;
        if (b_types.size() > 1) {
            b_names.push_back("Cancel");
            b_index = menu_vec(false, _("Use which brew?"), b_names) - 1;
            if (b_index == (int)b_names.size() - 1) {
                b_index = -1;
            }
        } else { //Only one brew type was in inventory, so it's automatically used
            if (!query_yn(_("Set %s in the vat?"), b_names[0].c_str())) {
                b_index = -1;
            }
        }
        if (b_index < 0) {
            return;
        }
        to_deposit = true;
        brew_type = b_types[b_index];
    } else {
        item &brew = m->i_at(examx, examy).front();
        brew_type = brew.typeId();
        charges_on_ground = brew.charges;
        if (p->charges_of(brew_type) > 0)
            if (query_yn(_("Add %s to the vat?"), brew.tname().c_str())) {
                to_deposit = true;
            }
    }
    if (to_deposit) {
        item brew(brew_type, 0);
        int charges_held = p->charges_of(brew_type);
        brew.charges = charges_on_ground;
        for (int i = 0; i < charges_held && !vat_full; i++) {
            p->use_charges(brew_type, 1);
            brew.charges++;
            if ( ((brew.count_by_charges()) ? brew.volume(false, true) / 1000 :
                  brew.volume(false, true) / 1000 * brew.charges ) >= 100) {
                vat_full = true;    //vats hold 50 units of brew, or 350 charges for a count_by_charges brew
            }
        }
        add_msg(_("Set %s in the vat."), brew.tname().c_str());
        m->i_clear(examx, examy);
        //This is needed to bypass NOITEM
        m->add_item( examx, examy, brew );
        p->moves -= 250;
    }
    if (vat_full || query_yn(_("Start fermenting cycle?"))) {
        m->i_at( examx, examy).front().bday = calendar::turn;
        m->furn_set(examx, examy, f_fvat_full);
        if (vat_full) {
            add_msg(_("The vat is full, so you close the lid and start the fermenting cycle."));
        } else {
            add_msg(_("You close the lid and start the fermenting cycle."));
        }
    }
}

void iexamine::fvat_full(player *p, map *m, int examx, int examy)
{
    bool liquid_present = false;
    for (int i = 0; i < (int)m->i_at(examx, examy).size(); i++) {
        if (!(m->i_at(examx, examy)[i].made_of(LIQUID)) || liquid_present) {
            m->add_item_or_charges(examx, examy, m->i_at(examx, examy)[i]);
            m->i_rem( examx, examy, i );
            i--;
        } else {
            liquid_present = true;
        }
    }
    if (!liquid_present) {
        debugmsg("fvat_full was empty or contained non-liquids only!");
        m->furn_set(examx, examy, f_fvat_empty);
        return;
    }
    item brew_i = m->i_at(examx, examy)[0];
    if (brew_i.has_flag("BREW")) { //Does the vat contain unfermented brew, or already fermented booze?
        int brew_time = brew_i.brewing_time();
        int brewing_stage = 3 * ((float)(calendar::turn.get_turn() - brew_i.bday) / (brew_time));
        add_msg(_("There's a vat full of %s set to ferment there."), brew_i.tname().c_str());
        switch (brewing_stage) {
        case 0:
            add_msg(_("It's been set recently, and will take some time to ferment."));
            break;
        case 1:
            add_msg(_("It is about halfway done fermenting."));
            break;
        case 2:
            add_msg(_("It will be ready for bottling soon."));
            break;
        // More messages can be added to show progress if desired
        default:
            // Double-checking that the brew is actually ready
            if( (calendar::turn.get_turn() > (brew_i.bday + brew_time) ) &&
                m->furn(examx, examy) == f_fvat_full && query_yn(_("Finish brewing?")) ) {
                //declare fermenting result as the brew's ID minus "brew_"
                itype_id alcoholType = m->i_at(examx, examy)[0].typeId().substr(5);
                SkillLevel &cooking = p->skillLevel("cooking");
                if (alcoholType == "hb_beer" && cooking < 5) {
                    alcoholType = alcoholType.substr(3);    //hb_beer -> beer
                }
                item booze(alcoholType, 0);
                booze.charges = brew_i.charges;
                booze.bday = brew_i.bday;

                m->i_clear(examx, examy);
                m->add_item( examx, examy, booze );
                p->moves -= 500;

                //low xp: you also get xp from crafting the brew
                p->practice( "cooking", std::min(brew_time / 600, 72) );
                add_msg(_("The %s is now ready for bottling."), booze.tname().c_str());
            }
        }
    } else { //Booze is done, so bottle it!
        item &booze = m->i_at(examx, examy).front();
        if( g->handle_liquid( booze, true, false) ) {
            m->furn_set(examx, examy, f_fvat_empty);
            add_msg(_("You squeeze the last drops of %s from the vat."), booze.tname().c_str());
            m->i_clear( examx, examy );
        }
    }
}

struct filter_is_drink {
    bool operator()(const item &it) {
        return it.is_drink();
    }
};

//probably should move this functionality into the furniture JSON entries if we want to have more than a few "kegs"
static int get_keg_cap(std::string furn_name) {
    if("standing tank" == furn_name)    { return 1200; } //the furniture was a "standing tank", so can hold 1200
    else                                { return 600; } //default to old default value
    //add additional cases above
}

void iexamine::keg(player *p, map *m, int examx, int examy)
{
    int keg_cap = get_keg_cap( m->name(examx, examy) );
    bool liquid_present = false;
    for (int i = 0; i < (int)m->i_at(examx, examy).size(); i++) {
        if (!(m->i_at(examx, examy)[i].is_drink()) || liquid_present) {
            m->add_item_or_charges(examx, examy, m->i_at(examx, examy)[i]);
            m->i_rem( examx, examy, i );
            i--;
        } else {
            liquid_present = true;
        }
    }
    if (!liquid_present) {
        // Get list of all drinks
        auto drinks_inv = p->items_with( filter_is_drink() );
        if ( drinks_inv.empty() ) {
            add_msg(m_info, _("You don't have any drinks to fill the %s with."), m->name(examx, examy).c_str());
            return;
        }
        // Make lists of unique drinks... about third time we do this, maybe we oughta make a function next time
        std::vector<itype_id> drink_types;
        std::vector<std::string> drink_names;
        for( auto &drink : drinks_inv ) {
            if (std::find(drink_types.begin(), drink_types.end(), drink->typeId()) == drink_types.end()) {
                drink_types.push_back(drink->typeId());
                drink_names.push_back(drink->tname());
            }
        }
        // Choose drink to store in keg from list
        int drink_index = 0;
        if (drink_types.size() > 1) {
            drink_names.push_back("Cancel");
            drink_index = menu_vec(false, _("Store which drink?"), drink_names) - 1;
            if (drink_index == (int)drink_names.size() - 1) {
                drink_index = -1;
            }
        } else { //Only one drink type was in inventory, so it's automatically used
            if (!query_yn(_("Fill the %s with %s?"), m->name(examx, examy).c_str(), drink_names[0].c_str())) {
                drink_index = -1;
            }
        }
        if (drink_index < 0) {
            return;
        }
        //Store liquid chosen in the keg
        itype_id drink_type = drink_types[drink_index];
        int charges_held = p->charges_of(drink_type);
        item drink (drink_type, 0);
        drink.charges = 0;
        bool keg_full = false;
        for (int i = 0; i < charges_held && !keg_full; i++) {
            g->u.use_charges(drink.typeId(), 1);
            drink.charges++;
            int d_vol = drink.volume(false, true) / 1000;
            if (d_vol >= keg_cap) {
                keg_full = true;
            }
        }
        if( keg_full ) {
            add_msg(_("You completely fill the %s with %s."),
                    m->name(examx, examy).c_str(), drink.tname().c_str());
        } else {
            add_msg(_("You fill the %s with %s."), m->name(examx, examy).c_str(),
                    drink.tname().c_str());
        }
        p->moves -= 250;
        m->i_clear(examx, examy);
        m->add_item( examx, examy, drink );
        return;
    } else {
        auto drink = m->i_at(examx, examy).begin();
        std::vector<std::string> menu_items;
        std::vector<uimenu_entry> options_message;
        menu_items.push_back(_("Fill a container with %drink"));
        options_message.push_back(uimenu_entry(string_format(_("Fill a container with %s"),
                                               drink->tname().c_str()), '1'));
        menu_items.push_back(_("Have a drink"));
        options_message.push_back(uimenu_entry(_("Have a drink"), '2'));
        menu_items.push_back(_("Refill"));
        options_message.push_back(uimenu_entry(_("Refill"), '3'));
        menu_items.push_back(_("Examine"));
        options_message.push_back(uimenu_entry(_("Examine"), '4'));
        menu_items.push_back(_("Cancel"));
        options_message.push_back(uimenu_entry(_("Cancel"), '5'));

        int choice;
        if( menu_items.size() == 1 ) {
            choice = 0;
        } else {
            uimenu selectmenu;
            selectmenu.return_invalid = true;
            selectmenu.text = _("Select an action");
            selectmenu.entries = options_message;
            selectmenu.selected = 0;
            selectmenu.query();
            choice = selectmenu.ret;
        }
        if(choice < 0) {
            return;
        }

        if(menu_items[choice] == _("Fill a container with %drink")) {
            if( g->handle_liquid(*drink, true, false) ) {
                add_msg(_("You squeeze the last drops of %s from the %s."), drink->tname().c_str(),
                        m->name(examx, examy).c_str());
                m->i_clear( examx, examy );
            }
            return;
        }

        if(menu_items[choice] == _("Have a drink")) {
            if( !p->eat( &*drink, dynamic_cast<it_comest *>(drink->type) ) ) {
                return; // They didn't actually drink
            }

            drink->charges--;
            if (drink->charges == 0) {
                add_msg(_("You squeeze the last drops of %s from the %s."), drink->tname().c_str(),
                        m->name(examx, examy).c_str());
                m->i_clear( examx, examy );
            }
            p->moves -= 250;
            return;
        }

        if(menu_items[choice] == _("Refill")) {
            int charges_held = p->charges_of(drink->typeId());
            int d_vol = drink->volume(false, true) / 1000;
            if (d_vol >= keg_cap) {
                add_msg(_("The %s is completely full."), m->name(examx, examy).c_str());
                return;
            }
            if (charges_held < 1) {
                add_msg(m_info, _("You don't have any %s to fill the %s with."),
                        drink->tname().c_str(), m->name(examx, examy).c_str());
                return;
            }
            for (int i = 0; i < charges_held; i++) {
                p->use_charges(drink->typeId(), 1);
                drink->charges++;
                int d_vol = drink->volume(false, true) / 1000;
                if (d_vol >= keg_cap) {
                    add_msg(_("You completely fill the %s with %s."), m->name(examx, examy).c_str(),
                            drink->tname().c_str());
                    p->moves -= 250;
                    return;
                }
            }
            add_msg(_("You fill the %s with %s."), m->name(examx, examy).c_str(),
                    drink->tname().c_str());
            p->moves -= 250;
            return;
        }

        if(menu_items[choice] == _("Examine")) {
            add_msg(m_info, _("That is a %s."), m->name(examx, examy).c_str());
            int full_pct = drink->volume(false, true) / (keg_cap * 10);
            add_msg(m_info, _("It contains %s (%d), %d%% full."),
                    drink->tname().c_str(), drink->charges, full_pct);
            return;
        }
    }
}

void iexamine::pick_plant(player *p, map *m, int examx, int examy,
                          std::string itemType, int new_ter, bool seeds)
{
    if (!query_yn(_("Harvest the %s?"), m->tername(examx, examy).c_str())) {
        none(p, m, examx, examy);
        return;
    }

    SkillLevel &survival = p->skillLevel("survival");
    if (survival < 1) {
        p->practice( "survival", rng(5, 12) );
    } else if (survival < 6) {
        p->practice("survival", rng(1, 12 / survival) );
    }

    int plantBase = rng(2, 5);
    int plantCount = rng(plantBase, plantBase + survival / 2);
    if (plantCount > 12) {
        plantCount = 12;
    }

    m->spawn_item(examx, examy, itemType, plantCount, 0, calendar::turn);

    if (seeds) {
        m->spawn_item(examx, examy, "seed_" + itemType, 1,
                      rng(plantCount / 4, plantCount / 2), calendar::turn);
    }

    m->ter_set(examx, examy, (ter_id)new_ter);
}

void iexamine::harvest_tree_shrub(player *p, map *m, int examx, int examy)
{
    if ( ((p->has_trait("PROBOSCIS")) || (p->has_trait("BEAK_HUM"))) &&
         ((p->hunger) > 0) && (!(p->wearing_something_on(bp_mouth))) &&
         (calendar::turn.get_season() == SUMMER || calendar::turn.get_season() == SPRING) ) {
        p->moves -= 100; // Need to find a blossom (assume there's one somewhere)
        add_msg(_("You find a flower and drink some nectar."));
        p->hunger -= 15;
    }
    //if the fruit is not ripe yet
    if (calendar::turn.get_season() != m->get_ter_harvest_season(examx, examy)) {
        std::string fruit = item::nname(m->get_ter_harvestable(examx, examy), 10);
        fruit[0] = toupper(fruit[0]);
        add_msg(m_info, _("%s ripen in %s."), fruit.c_str(), season_name[m->get_ter_harvest_season(examx, examy)].c_str());
        return;
    }
    //if the fruit has been recently harvested
    if (m->has_flag(TFLAG_HARVESTED, examx, examy)){
        add_msg(m_info, _("This %s has already been harvested. Harvest it again next year."), m->tername(examx, examy).c_str());
        return;
    }

    bool seeds = false;
    if (m->has_flag("SHRUB", examx, examy)) { // if shrub, it gives seeds. todo -> trees give seeds(?) -> trees plantable
        seeds = true;
    }
    pick_plant(p, m, examx, examy, m->get_ter_harvestable(examx, examy), m->get_ter_transforms_into(examx, examy), seeds);
}

void iexamine::tree_pine(player *p, map *m, int examx, int examy)
{
    if(!query_yn(_("Pick %s?"), m->tername(examx, examy).c_str())) {
        none(p, m, examx, examy);
        return;
    }
    m->spawn_item(p->xpos(), p->ypos(), "pine_bough", 2, 12 );
    m->spawn_item( p->xpos(), p->ypos(), "pinecone", rng( 1, 4 ) );
    m->ter_set(examx, examy, t_tree_deadpine);
}

void iexamine::tree_blackjack(player *p, map *m, int examx, int examy)
{
    if(!query_yn(_("Pick %s?"), m->tername(examx, examy).c_str())) {
        none(p, m, examx, examy);
        return;
    }
    m->spawn_item(p->xpos(), p->ypos(), "acorns", 2, 6 );
    m->spawn_item( p->xpos(), p->ypos(), "tanbark", rng( 1, 2 ) );
    m->ter_set(examx, examy, t_tree);
}

void iexamine::shrub_marloss(player *p, map *m, int examx, int examy)
{
    if (p->has_trait("THRESH_MYCUS")) {
        pick_plant(p, m, examx, examy, "mycus_fruit", t_shrub_fungal);
    } else if (p->has_trait("THRESH_MARLOSS")) {
        m->spawn_item( examx, examy, "mycus_fruit" );
        m->ter_set(examx, examy, t_fungus);
        add_msg( m_info, _("The shrub offers up a fruit, then crumbles into a fungal bed."));
    } else {
        pick_plant(p, m, examx, examy, "marloss_berry", t_shrub_fungal);
    }
}

void iexamine::tree_marloss(player *p, map *m, int examx, int examy)
{
    if (p->has_trait("THRESH_MYCUS")) {
        pick_plant(p, m, examx, examy, "mycus_fruit", t_tree_fungal);
        if (p->has_trait("M_DEPENDENT") && one_in(3)) {
            // Folks have a better shot at keeping fed.
            add_msg(m_info, _("We have located a particularly vital nutrient deposit underneath this location."));
            add_msg(m_good, _("Additional nourishment is available."));
            m->ter_set(examx, examy, t_marloss_tree);
        }
    } else if (p->has_trait("THRESH_MARLOSS")) {
        m->spawn_item( examx, examy, "mycus_fruit" );
        m->ter_set(examx, examy, t_tree_fungal);
        add_msg(m_info, _("The tree offers up a fruit, then shrivels into a fungal tree."));
    } else {
        pick_plant(p, m, examx, examy, "marloss_berry", t_tree_fungal);
    }
}

void iexamine::shrub_wildveggies(player *p, map *m, int examx, int examy)
{
    add_msg("You forage through the %s.", m->tername(examx, examy).c_str());
    p->assign_activity(ACT_FORAGE, 500 / (p->skillLevel("survival") + 1), 0);
    p->activity.placement = point(examx, examy);
    return;
}

int sum_up_item_weight_by_material( map_stack &stack, const std::string &material, bool remove_items )
{
    int sum_weight = 0;
    for( auto item_it = stack.begin(); item_it != stack.end(); ) {
        if( item_it->made_of(material) && item_it->weight() > 0) {
            sum_weight += item_it->weight();
            if( remove_items ) {
                item_it = stack.erase( item_it );
            } else {
                ++item_it;
            }
        } else {
            ++item_it;
        }
    }
    return sum_weight;
}

void add_recyle_menu_entry(uimenu &menu, int w, char hk, const std::string &type)
{
    const auto itt = item( type, 0 );
    const int amount = w / itt.weight();
    menu.addentry(
        menu.entries.size() + 1, // value return by uimenu for this entry
        true, // enabled
        hk, // hotkey
        string_format(_("about %d %s"), amount, itt.tname( amount ).c_str())
    );
}

void iexamine::recycler(player *p, map *m, int examx, int examy)
{
    auto items_on_map = m->i_at(examx, examy);

    // check for how much steel, by weight, is in the recycler
    // only items made of STEEL are checked
    // IRON and other metals cannot be turned into STEEL for now
    int steel_weight = sum_up_item_weight_by_material( items_on_map, "steel", false );
    if (steel_weight == 0) {
        add_msg(m_info,
                _("The recycler is currently empty.  Drop some metal items onto it and examine it again."));
        return;
    }
    // See below for recover_factor (rng(6,9)/10), this
    // is the normal value of that recover factor.
    static const double norm_recover_factor = 8.0 / 10.0;
    const int norm_recover_weight = steel_weight * norm_recover_factor;
    uimenu as_m;
    // Get format for printing weights, convert weight to that format,
    const std::string format = OPTIONS["USE_METRIC_WEIGHTS"].getValue() == "lbs" ? _("%.3f lbs") :
                               _("%.3f kg");
    const std::string weight_str = string_format(format, p->convert_weight(steel_weight));
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

    if (ch >= 5 || ch <= 0) {
        add_msg(_("Never mind."));
        return;
    }

    // Sum up again, this time remove the items,
    // ignore result, should be the same as before.
    sum_up_item_weight_by_material( items_on_map, "steel", true );

    double recover_factor = rng(6, 9) / 10.0;
    steel_weight = (int)(steel_weight * recover_factor);

    g->sound(examx, examy, 80, _("Ka-klunk!"));

    int lump_weight = item( "steel_lump", 0 ).weight();
    int sheet_weight = item( "sheet_metal", 0 ).weight();
    int chunk_weight = item( "steel_chunk", 0 ).weight();
    int scrap_weight = item( "scrap", 0 ).weight();

    if (steel_weight < scrap_weight) {
        add_msg(_("The recycler chews up all the items in its hopper."));
        add_msg(_("The recycler beeps: \"No steel to process!\""));
        return;
    }

    switch(ch) {
    case 1: // 1 steel lump = weight 1360
        num_lumps = steel_weight / (lump_weight);
        steel_weight -= num_lumps * (lump_weight);
        num_sheets = steel_weight / (sheet_weight);
        steel_weight -= num_sheets * (sheet_weight);
        num_chunks = steel_weight / (chunk_weight);
        steel_weight -= num_chunks * (chunk_weight);
        num_scraps = steel_weight / (scrap_weight);
        if (num_lumps == 0) {
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
        if (num_sheets == 0) {
            add_msg(_("The recycler beeps: \"Insufficient steel!\""));
            add_msg(_("It spits out an assortment of smaller pieces instead."));
        }
        break;

    case 3: // 1 steel chunk = weight 340
        num_chunks = steel_weight / (chunk_weight);
        steel_weight -= num_chunks * (chunk_weight);
        num_scraps = steel_weight / (scrap_weight);
        if (num_chunks == 0) {
            add_msg(_("The recycler beeps: \"Insufficient steel!\""));
            add_msg(_("It spits out an assortment of smaller pieces instead."));
        }
        break;

    case 4: // 1 metal scrap = weight 113
        num_scraps = steel_weight / (scrap_weight);
        break;
    }

    for (int i = 0; i < num_lumps; i++) {
        m->spawn_item(p->posx, p->posy, "steel_lump");
    }

    for (int i = 0; i < num_sheets; i++) {
        m->spawn_item(p->posx, p->posy, "sheet_metal");
    }

    for (int i = 0; i < num_chunks; i++) {
        m->spawn_item(p->posx, p->posy, "steel_chunk");
    }

    for (int i = 0; i < num_scraps; i++) {
        m->spawn_item(p->posx, p->posy, "scrap");
    }
}

void iexamine::trap(player *p, map *m, int examx, int examy)
{
    const trap_id tid = m->tr_at(examx, examy);
    if (p == NULL || !p->is_player() || tid == tr_null) {
        return;
    }
    const struct trap &t = *traplist[tid];
    const int possible = t.get_difficulty();
    if ( (t.can_see(*p, examx, examy)) && (possible == 99) ) {
        add_msg(m_info, _("That %s looks too dangerous to mess with. Best leave it alone."),
            t.name.c_str());
        return;
    }
    // Some traps are not actual traps. Those should get a different query.
    if (t.can_see(*p, examx, examy) && possible == 0 &&
        t.get_avoidance() == 0) { // Separated so saying no doesn't trigger the other query.
        if (query_yn(_("There is a %s there. Take down?"), t.name.c_str())) {
            m->disarm_trap(examx, examy);
        }
    } else if (t.can_see(*p, examx, examy) &&
               query_yn(_("There is a %s there.  Disarm?"), t.name.c_str())) {
        m->disarm_trap(examx, examy);
    }
}

void iexamine::water_source(player *p, map *m, const int examx, const int examy)
{
    item water = m->water_from(examx, examy);
    const std::string text = string_format(_("Container for %s"), water.tname().c_str());
    item *cont = g->inv_map_for_liquid(water, text);
    if (cont == NULL || cont->is_null()) {
        // No container selected, try drinking from out hands
        p->drink_from_hands(water);
    } else {
        // Turns needed is the number of liquid units / 10 * 100 (because 100 moves in a turn).
        LIQUID_FILL_ERROR junk;
        int turns = cont->get_remaining_capacity_for_liquid( water, junk ) * 10;
        if (turns > 0) {
            if( turns/1000 > 1 ) {
                // If it takes less than a minute, no need to inform the player about time.
                p->add_msg_if_player(m_info, _("It will take around %d minutes to fill that container."), turns / 1000);
            }
            p->assign_activity(ACT_FILL_LIQUID, turns, -1, p->get_item_position(cont), cont->tname());
            p->activity.str_values.push_back(water.typeId());
            p->activity.values.push_back(water.poison);
            p->activity.values.push_back(water.bday);
        }
    }
}
void iexamine::swater_source(player *p, map *m, const int examx, const int examy)
{
    item swater = m->swater_from(examx, examy);
    const std::string text = string_format(_("Container for %s"), swater.tname().c_str());
    item *cont = g->inv_map_for_liquid(swater, text);
    if (cont == NULL || cont->is_null()) {
        // No container selected, try drinking from out hands
        p->drink_from_hands(swater);
    } else {
        // Turns needed is the number of liquid units / 10 * 100 (because 100 moves in a turn).
        LIQUID_FILL_ERROR junk;
        int turns = cont->get_remaining_capacity_for_liquid( swater, junk ) * 10;
        if (turns > 0) {
            if( turns/1000 > 1 ) {
                // If it takes less than a minute, no need to inform the player about time.
                p->add_msg_if_player(m_info, _("It will take around %d minutes to fill that container."), turns / 1000);
            }
            p->assign_activity(ACT_FILL_LIQUID, turns, -1, p->get_item_position(cont), cont->tname());
            p->activity.str_values.push_back(swater.typeId());
            p->activity.values.push_back(swater.poison);
            p->activity.values.push_back(swater.bday);
        }
    }
}
void iexamine::acid_source(player *p, map *m, const int examx, const int examy)
{
    item acid = m->acid_from(examx, examy);
    if (g->handle_liquid(acid, true, true)) {
        p->moves -= 100;
    }
}

itype *furn_t::crafting_pseudo_item_type() const
{
    if (crafting_pseudo_item.empty()) {
        return NULL;
    }
    return item::find_type( crafting_pseudo_item );
}

itype *furn_t::crafting_ammo_item_type() const
{
    const it_tool *toolt = dynamic_cast<const it_tool *>(crafting_pseudo_item_type());
    if (toolt != NULL && toolt->ammo != "NULL") {
        const std::string ammoid = default_ammo(toolt->ammo);
        return item::find_type( ammoid );
    }
    return NULL;
}

static long count_charges_in_list(const itype *type, const map_stack &items)
{
    for( const auto &candidate : items ) {
        if( candidate.type == type ) {
            return candidate.charges;
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
        add_msg(m_info, _("This %s can not be reloaded!"), f.name.c_str());
        return;
    }
    const int pos = p->inv.position_by_type(ammo->id);
    if (pos == INT_MIN) {
        const int amount = count_charges_in_list(ammo, m->i_at(examx, examy));
        if (amount > 0) {
            //~ The <piece of furniture> contains <number> <items>.
            add_msg(_("The %s contains %d %s."), f.name.c_str(), amount, ammo->nname(amount).c_str());
        }
        //~ Reloading or restocking a piece of furniture, for example a forge.
        add_msg(m_info, _("You need some %s to reload this %s."), ammo->nname(2).c_str(), f.name.c_str());
        return;
    }
    const long max_amount = p->inv.find_item(pos).charges;
    //~ Loading fuel or other items into a piece of furniture.
    const std::string popupmsg = string_format(_("Put how many of the %s into the %s?"),
                                 ammo->nname(max_amount).c_str(), f.name.c_str());
    long amount = helper::to_int( string_input_popup( popupmsg, 20,
                                  helper::to_string_int(max_amount),
                                  "", "", -1, true) );
    if (amount <= 0 || amount > max_amount) {
        return;
    }
    p->reduce_charges(pos, amount);
    auto items = m->i_at(examx, examy);
    for( auto an_item = items.begin(); an_item != items.end(); ++an_item ) {
        if( an_item->type == ammo ) {
            an_item->charges = amount;
            amount = 0;
            break;
        }
    }
    if (amount != 0) {
        item it(ammo->id, 0);
        it.charges = amount;
        m->add_item( examx, examy, it );
    }
    add_msg(_("You reload the %s."), m->furnname(examx, examy).c_str());
    p->moves -= 100;
}

void iexamine::curtains(player *p, map *m, const int examx, const int examy)
{
    if (m->is_outside(p->posx, p->posy)) {
        p->add_msg_if_player( _("You cannot get to the curtains from the outside."));
        return;
    }

    // Peek through the curtains, or tear them down.
    int choice = menu( true, _("Do what with the curtains?"),
                       _("Peek through the curtains."), _("Tear down the curtains."),
                       _("Cancel"), NULL );
    if( choice == 1 ) {
        // Peek
        g->peek( examx, examy );
        p->add_msg_if_player( _("You carefully peek through the curtains.") );
    } else if( choice == 2 ) {
        // Mr. Gorbachev, tear down those curtains!
        m->ter_set( examx, examy, "t_window" );
        m->spawn_item( p->xpos(), p->ypos(), "nail", 1, 4 );
        m->spawn_item( p->xpos(), p->ypos(), "sheet", 2 );
        m->spawn_item( p->xpos(), p->ypos(), "stick" );
        m->spawn_item( p->xpos(), p->ypos(), "string_36" );
        p->moves -= 200;
        p->add_msg_if_player( _("You tear the curtains and curtain rod off the windowframe.") );
    } else {
        p->add_msg_if_player( _("Never mind."));
    }
}

void iexamine::sign(player *p, map *m, int examx, int examy)
{
    std::string existing_signage = m->get_signage(examx, examy);
    bool previous_signage_exists = !existing_signage.empty();

    // Display existing message, or lack thereof.
    if (previous_signage_exists) {
        popup(existing_signage.c_str());
    } else {
        p->add_msg_if_player(m_neutral, _("Nothing legible on the sign."));
    }

    // Allow chance to modify message.
    // Chose spray can because it seems appropriate.
    int required_writing_charges = 1;
    if (p->has_charges("spray_can", required_writing_charges)) {
        // Different messages if the sign already has writing associated with it.
        std::string query_message = previous_signage_exists ?
                                    _("Overwrite the existing message on the sign with spray paint?") :
                                    _("Add a message to the sign with spray paint?");
        std::string spray_painted_message = previous_signage_exists ?
                                            _("You overwrite the previous message on the sign with your graffiti") :
                                            _("You graffiti a message onto the sign.");
        std::string ignore_message = _("You leave the sign alone.");
        if (query_yn(query_message.c_str())) {
            std::string signage = string_input_popup(_("Spray what?"), 0, "", "", "signage");
            if (signage.empty()) {
                p->add_msg_if_player(m_neutral, ignore_message.c_str());
            } else {
                m->set_signage(examx, examy, signage);
                p->add_msg_if_player(m_info, spray_painted_message.c_str());
                p->moves -= 2 * signage.length();
                p->use_charges("spray_can", required_writing_charges);
            }
        } else {
            p->add_msg_if_player(m_neutral, ignore_message.c_str());
        }
    }
}

static int getNearPumpCount(map *m, int x, int y)
{
    const int radius = 12;

    int result = 0;

    for (int i = x - radius; i <= x + radius; i++) {
        for (int j = y - radius; j <= y + radius; j++) {
            if (m->ter_at(i, j).id == "t_gas_pump" || m->ter_at(i, j).id == "t_gas_pump_a") {
                result++;
            }
        }
    }
    return result;
}

static point getNearFilledGasTank(map *m, int x, int y, long &gas_units)
{
    const int radius = 24;

    point p = point(-999, -999);
    int distance = radius + 1;
    gas_units = 0;

    for (int i = x - radius; i <= x + radius; i++) {
        for (int j = y - radius; j <= y + radius; j++) {
            if (m->ter_at(i, j).id != "t_gas_tank") {
                continue;
            }

            int new_distance = rl_dist( x, y, i, j );

            if( new_distance >= distance ) {
                continue;
            }
            if( p.x == -999 ) {
                // Return a potentially empty tank, but only if we don't find a closer full one.
                p = point(i, j);
            }
            for( auto &k : m->i_at(i, j)) {
                if(k.made_of(LIQUID)) {
                    long count = dynamic_cast<it_ammo *>(k.type)->count;
                    long units = k.charges / count;

                    distance = new_distance;
                    p = point(i, j);
                    gas_units = units;
                    break;
                }
            }
        }
    }
    return p;
}

static int getGasDiscountCardQuality(item it)
{
    std::set<std::string> tags = it.type->item_tags;

    for( auto tag : tags ) {

        if( tag.size() > 15 && tag.substr(0, 15) == "DISCOUNT_VALUE_" ) {
            return atoi(tag.substr(15).c_str());
        }
    }

    return 0;
}

static int findBestGasDiscount(player *p)
{
    int discount = 0;

    for (size_t i = 0; i < p->inv.size(); i++) {
        item &it = p->inv.find_item(i);

        if (it.has_flag("GAS_DISCOUNT")) {

            int q = getGasDiscountCardQuality(it);
            if (q > discount) {
                discount = q;
            }
        }
    }

    return discount;
}

static std::string str_to_illiterate_str(std::string s)
{
    if (!g->u.has_trait("ILLITERATE")) {
        return s;
    } else {
        for (auto &i : s) {
            i = i + rng(0, 5) - rng(0, 5);
            if( i < ' ' ) {
                // some control character, most likely not handled correctly be the print functions
                i = ' ';
            } else if( i == '%' ) {
                // avoid characters that trigger formatting in the various print functions
                i++;
            }
        }
        return s;
    }
}

static std::string getGasDiscountName(int discount)
{
    if (discount == 3) {
        return str_to_illiterate_str(_("Platinum member"));
    } else if (discount == 2) {
        return str_to_illiterate_str(_("Gold member"));
    } else if (discount == 1) {
        return str_to_illiterate_str(_("Silver member"));
    } else {
        return str_to_illiterate_str(_("Beloved customer"));
    }
}

static int getPricePerGasUnit(int discount)
{
    if (discount == 3) {
        return 250;
    } else if (discount == 2) {
        return 300;
    } else if (discount == 1) {
        return 330;
    } else {
        return 350;
    }
}

static point getGasPumpByNumber(map *m, int x, int y, int number)
{
    const int radius = 12;

    int k = 0;

    for( int i = x - radius; i <= x + radius; i++ ) {
        for( int j = y - radius; j <= y + radius; j++ ) {
            if( (m->ter_at(i, j).id == "t_gas_pump" ||
                 m->ter_at(i, j).id == "t_gas_pump_a") && number == k++) {
                return point(i, j);
            }
        }
    }

    return point(-999, -999);
}

static bool toPumpFuel(map *m, point src, point dst, long units)
{
    if (src.x == -999) {
        return false;
    }
    if (dst.x == -999) {
        return false;
    }

    auto items = m->i_at( src.x, src.y );
    for( auto item_it = items.begin(); item_it != items.end(); ++item_it ) {
        if( item_it->made_of(LIQUID)) {
            long count = dynamic_cast<it_ammo *>(item_it->type)->count;

            if( item_it->charges < count * units ) {
                return false;
            }

            item_it->charges -= count * units;

            item liq_d(item_it->type->id, calendar::turn);
            liq_d.charges = count * units;

            ter_t backup_pump = m->ter_at(dst.x, dst.y);
            m->ter_set(dst.x, dst.y, "t_null");
            m->add_item_or_charges(dst.x, dst.y, liq_d);
            m->ter_set(dst.x, dst.y, backup_pump.id);

            if( item_it->charges < 1 ) {
                items.erase( item_it );
            }

            return true;
        }
    }

    return false;
}

static long fromPumpFuel(map *m, point dst, point src)
{
    if (src.x == -999) {
        return -1;
    }
    if (dst.x == -999) {
        return -1;
    }

    auto items = m->i_at( src.x, src.y );
    for( auto item_it = items.begin(); item_it != items.end(); ++item_it ) {
        if( item_it->made_of(LIQUID)) {
            long count = dynamic_cast<it_ammo *>(item_it->type)->count;

            // how much do we have in the pump?
            item liq_d(item_it->type->id, calendar::turn);
            liq_d.charges = item_it->charges;

            // add the charges to the destination
            ter_t backup_tank = m->ter_at(dst.x, dst.y);
            m->ter_set(dst.x, dst.y, "t_null");
            m->add_item_or_charges(dst.x, dst.y, liq_d);
            m->ter_set(dst.x, dst.y, backup_tank.id);

            // remove the liquid from the pump
            long amount = item_it->charges;
            items.erase( item_it );
            return amount / count;
        }
    }
    return -1;
}

static void turnOnSelectedPump(map *m, int x, int y, int number)
{
    const int radius = 12;

    int k = 0;
    for (int i = x - radius; i <= x + radius; i++) {
        for (int j = y - radius; j <= y + radius; j++) {
            if ((m->ter_at(i, j).id == "t_gas_pump" || m->ter_at(i, j).id == "t_gas_pump_a") ) {
                if (number == k++) {
                    m->ter_set(i, j, "t_gas_pump_a");
                } else {
                    m->ter_set(i, j, "t_gas_pump");
                }
            }
        }
    }
}

void iexamine::pay_gas(player *p, map *m, const int examx, const int examy)
{

    int choice = -1;
    const int buy_gas = 1;
    const int choose_pump = 2;
    const int hack = 3;
    const int refund = 4;
    const int cancel = 5;

    if (p->has_trait("ILLITERATE")) {
        popup(_("You're illiterate, and can't read the screen."));
    }

    int pumpCount = getNearPumpCount(m, examx, examy);
    if (pumpCount == 0) {
        popup(str_to_illiterate_str(_("Failure! No gas pumps found!")).c_str());
        return;
    }

    long tankGasUnits;
    point pTank = getNearFilledGasTank(m, examx, examy, tankGasUnits);
    if (pTank.x == -999) {
        popup(str_to_illiterate_str(_("Failure! No gas tank found!")).c_str());
        return;
    }

    if (tankGasUnits == 0) {
        popup(str_to_illiterate_str(
                  _("This station is out of fuel.  We apologize for the inconvenience.")).c_str());
        return;
    }

    if (uistate.ags_pay_gas_selected_pump + 1 > pumpCount) {
        uistate.ags_pay_gas_selected_pump = 0;
    }

    int discount = findBestGasDiscount(p);
    std::string discountName = getGasDiscountName(discount);

    int pricePerUnit = getPricePerGasUnit(discount);
    std::string unitPriceStr = string_format(_("$%0.2f"), pricePerUnit / 100.0);

    bool can_hack = (!p->has_trait("ILLITERATE") && ((p->has_amount("electrohack", 1)) ||
                     (p->has_bionic("bio_fingerhack") && p->power_level > 0)));

    uimenu amenu;
    amenu.selected = 1;
    amenu.text = str_to_illiterate_str(_("Welcome to AutoGas!"));
    amenu.addentry(0, false, -1, str_to_illiterate_str(_("What would you like to do?")));

    amenu.addentry(buy_gas, true, 'b', str_to_illiterate_str(_("Buy gas.")));
    amenu.addentry(refund, true, 'r', str_to_illiterate_str(_("Refund cash.")));

    std::string gaspumpselected = str_to_illiterate_str(_("Current gas pump: ")) +
                                  helper::to_string_int( uistate.ags_pay_gas_selected_pump + 1 );
    amenu.addentry(0, false, -1, gaspumpselected);
    amenu.addentry(choose_pump, true, 'p', str_to_illiterate_str(_("Choose a gas pump.")));

    amenu.addentry(0, false, -1, str_to_illiterate_str(_("Your discount: ")) + discountName);
    amenu.addentry(0, false, -1, str_to_illiterate_str(_("Your price per gasoline unit: ")) +
                   unitPriceStr);

    if (can_hack) {
        amenu.addentry(hack, true, 'h', _("Hack console."));
    }

    amenu.addentry(cancel, true, 'q', str_to_illiterate_str(_("Cancel")));

    amenu.query();
    choice = amenu.ret;

    if (choose_pump == choice) {
        uimenu amenu;
        amenu.selected = uistate.ags_pay_gas_selected_pump + 1;
        amenu.text = str_to_illiterate_str(_("Please choose gas pump:"));

        amenu.addentry(0, true, 'q', str_to_illiterate_str(_("Cancel")));

        for (int i = 0; i < pumpCount; i++) {
            amenu.addentry( i + 1, true, -1,
                            str_to_illiterate_str(_("Pump ")) + helper::to_string_int(i + 1) );
        }
        amenu.query();
        choice = amenu.ret;

        if (choice == 0) {
            return;
        }

        uistate.ags_pay_gas_selected_pump = choice - 1;

        turnOnSelectedPump(m, examx, examy, uistate.ags_pay_gas_selected_pump);

        return;

    }

    if (buy_gas == choice) {

        int pos;
        item *cashcard;

        pos = g->inv(_("Insert card."));
        cashcard = &(p->i_at(pos));

        if (cashcard->is_null()) {
            popup(_("You do not have that item!"));
            return;
        }
        if (cashcard->type->id != "cash_card") {
            popup(_("Please insert cash cards only!"));
            return;
        }
        if (cashcard->charges < pricePerUnit) {
            popup(str_to_illiterate_str(
                      _("Not enough money, please refill your cash card.")).c_str()); //or ride on a solar car, ha ha ha
            return;
        }

        long c_max = cashcard->charges / pricePerUnit;
        long max = (c_max < tankGasUnits) ? c_max : tankGasUnits;

        std::string popupmsg = string_format(
                                   ngettext("How many gas units to buy? Max:%d unit. (0 to cancel) ",
                                            "How many gas units to buy? Max:%d units. (0 to cancel) ",
                                            max), max);
        long amount = helper::to_int(string_input_popup(popupmsg, 20,
                                     helper::to_string_int(max), "", "", -1, true)
                                    );
        if (amount <= 0) {
            return;
        }
        if (amount > max) {
            amount = max;
        }

        point pGasPump = getGasPumpByNumber(m, examx, examy,  uistate.ags_pay_gas_selected_pump);
        if (!toPumpFuel(m, pTank, pGasPump, amount)) {
            return;
        }

        g->sound(p->posx, p->posy, 6, _("Glug Glug Glug"));

        cashcard->charges -= amount * pricePerUnit;

        add_msg(m_info, ngettext("Your cash card now holds %d cent.",
                                 "Your cash card now holds %d cents.",
                                 cashcard->charges), cashcard->charges);
        p->moves -= 100;
        return;
    }

    if (hack == choice) {
        bool using_electrohack = (p->has_amount("electrohack", 1) &&
                                  query_yn(_("Use electrohack on the reader?")));
        bool using_fingerhack = (!using_electrohack && p->has_bionic("bio_fingerhack") &&
                                 p->power_level > 0 &&
                                 query_yn(_("Use fingerhack on the reader?")));
        if (using_electrohack || using_fingerhack) {
            p->moves -= 500;
            p->practice("computer", 20);
            int success = rng(p->skillLevel("computer") / 4 - 2, p->skillLevel("computer") * 2);
            success += rng(-3, 3);
            if (using_fingerhack) {
                success++;
            }
            if (p->int_cur < 8) {
                success -= rng(0, int((8 - p->int_cur) / 2));
            } else if (p->int_cur > 8) {
                success += rng(0, int((p->int_cur - 8) / 2));
            }
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
                p->add_memorial_log(pgettext("memorial_male", "Set off an alarm."),
                                      pgettext("memorial_female", "Set off an alarm."));
                g->sound(p->posx, p->posy, 60, _("An alarm sounds!"));
                if (g->levz > 0 && !g->event_queued(EVENT_WANTED)) {
                    g->add_event(EVENT_WANTED, int(calendar::turn) + 300, 0, g->levx, g->levy);
                }
            } else if (success < 6) {
                add_msg(_("Nothing happens."));
            } else {
                point pGasPump = getGasPumpByNumber(m, examx, examy, uistate.ags_pay_gas_selected_pump);
                if (toPumpFuel(m, pTank, pGasPump, tankGasUnits)) {
                    add_msg(_("You hack the terminal and route all available fuel to your pump!"));
                    g->sound(p->posx, p->posy, 6, _("Glug Glug Glug Glug Glug Glug Glug Glug Glug"));
                } else {
                    add_msg(_("Nothing happens."));
                }
            }
        } else {
            return;
        }
    }

    if (refund == choice) {
        int pos;
        item *cashcard;

        pos = g->inv(_("Insert card."));
        cashcard = &(p->i_at(pos));

        if (cashcard->is_null()) {
            popup(_("You do not have that item!"));
            return;
        }
        if (cashcard->type->id != "cash_card") {
            popup(_("Please insert cash cards only!"));
            return;
        }
        // Ok, we have a cash card. Now we need to know what's left in the pump.
        point pGasPump = getGasPumpByNumber(m, examx, examy, uistate.ags_pay_gas_selected_pump);
        long amount = fromPumpFuel(m, pTank, pGasPump);
        if (amount >= 0) {
            g->sound(p->posx, p->posy, 6, _("Glug Glug Glug"));
            cashcard->charges += amount * pricePerUnit;
            add_msg(m_info, ngettext("Your cash card now holds %d cent.",
                                     "Your cash card now holds %d cents.",
                                     cashcard->charges), cashcard->charges);
            p->moves -= 100;
            return;
        } else {
            popup(_("Unable to refund, no fuel in pump."));
            return;
        }
    }
}

/**
 * Given then name of one of the above functions, returns the matching function
 * pointer. If no match is found, defaults to iexamine::none but prints out a
 * debug message as a warning.
 * @param function_name The name of the function to get.
 * @return A function pointer to the specified function.
 */
void (iexamine::*iexamine_function_from_string(std::string function_name))(player *, map *, int, int)
{
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
    if ("portable_structure" == function_name) {
        return &iexamine::portable_structure;
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
    if ("door_peephole" == function_name) {
        return &iexamine::door_peephole;
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
    if ("flower_datura" == function_name) {
        return &iexamine::flower_datura;
    }
    if ("flower_marloss" == function_name) {
        return &iexamine::flower_marloss;
    }
    if ("flower_dandelion" == function_name) {
        return &iexamine::flower_dandelion;
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
    if ("harvest_tree_shrub" == function_name) {
        return &iexamine::harvest_tree_shrub;
    }
    if ("tree_pine" == function_name) {
        return &iexamine::tree_pine;
    }
    if ("tree_blackjack" == function_name) {
        return &iexamine::tree_blackjack;
    }
    if ("shrub_marloss" == function_name) {
        return &iexamine::shrub_marloss;
    }
    if ("tree_marloss" == function_name) {
        return &iexamine::tree_marloss;
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
    if( "curtains" == function_name ) {
        return &iexamine::curtains;
    }
    if( "sign" == function_name ) {
        return &iexamine::sign;
    }
    if ("pay_gas" == function_name) {
        return &iexamine::pay_gas;
    }
    if ("gunsafe_ml" == function_name) {
        return &iexamine::gunsafe_ml;
    }
    if ("gunsafe_el" == function_name) {
        return &iexamine::gunsafe_el;
    }

    //No match found
    debugmsg("Could not find an iexamine function matching '%s'!", function_name.c_str());
    return &iexamine::none;

}
