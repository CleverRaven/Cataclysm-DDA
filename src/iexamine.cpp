#include "iexamine.h"
#include "game.h"
#include "map.h"
#include "map_iterator.h"
#include "debug.h"
#include "mapdata.h"
#include "output.h"
#include "rng.h"
#include "line.h"
#include "player.h"
#include "translations.h"
#include "uistate.h"
#include "messages.h"
#include "compatibility.h"
#include "sounds.h"
#include "worldfactory.h"
#include "input.h"
#include "monster.h"
#include "event.h"
#include "catacharset.h"
#include "ui.h"
#include "trap.h"
#include "itype.h"
#include "basecamp.h"
#include "mtype.h"
#include "weather.h"
#include "sounds.h"
#include "cata_utility.h"

#include <sstream>
#include <algorithm>
#include <cstdlib>

const mtype_id mon_dark_wyrm( "mon_dark_wyrm" );
const mtype_id mon_fungal_blossom( "mon_fungal_blossom" );
const mtype_id mon_spider_web_s( "mon_spider_web_s" );
const mtype_id mon_spider_widow_giant_s( "mon_spider_widow_giant_s" );
const mtype_id mon_spider_cellar_giant_s( "mon_spider_cellar_giant_s" );
const mtype_id mon_turret( "mon_turret" );
const mtype_id mon_turret_rifle( "mon_turret_rifle" );

const skill_id skill_computer( "computer" );
const skill_id skill_mechanics( "mechanics" );
const skill_id skill_carpentry( "carpentry" );
const skill_id skill_cooking( "cooking" );
const skill_id skill_survival( "survival" );

const efftype_id effect_pkill2( "pkill2" );
const efftype_id effect_teleglow( "teleglow" );

static void pick_plant( player *p, map *m, const tripoint &examp, std::string itemType, ter_id new_ter,
                        bool seeds = false );

void iexamine::none(player *p, map *m, const tripoint &examp)
{
    (void)p; //unused
    add_msg(_("That is a %s."), m->name(examp).c_str());
}

void iexamine::gaspump(player *p, map *m, const tripoint &examp)
{
    if (!query_yn(_("Use the %s?"), m->tername(examp).c_str())) {
        none(p, m, examp);
        return;
    }

    auto items = m->i_at( examp );
    for( auto item_it = items.begin(); item_it != items.end(); ++item_it ) {
        if( item_it->made_of(LIQUID) ) {
            ///\EFFECT_DEX decreases chance of spilling gas from a pump
            if( one_in(10 + p->dex_cur) ) {
                add_msg(m_bad, _("You accidentally spill the %s."), item_it->type_name(1).c_str());
                item spill( item_it->type->id, calendar::turn );
                const auto min = item_it->liquid_charges( 1 );
                ///\EFFECT_DEX decreases amount of gas spilled from a pump
                const auto max = item_it->liquid_charges( 1 ) * 8.0 / p->dex_cur;
                spill.charges = rng( min, max );
                m->add_item_or_charges( p->pos(), spill, 1 );
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

namespace {
//--------------------------------------------------------------------------------------------------
//! Implements iexamine::atm(...)
//--------------------------------------------------------------------------------------------------
class atm_menu {
public:
    // menu choices
    enum options : int {
        cancel, purchase_card, deposit_money, withdraw_money, transfer_money, transfer_all_money
    };

    atm_menu()                           = delete;
    atm_menu(atm_menu const&)            = delete;
    atm_menu(atm_menu&&)                 = delete;
    atm_menu& operator=(atm_menu const&) = delete;
    atm_menu& operator=(atm_menu&&)      = delete;

    explicit atm_menu(player &p) : u(p) {
        reset(false);
    }

    void start() {
        for (bool result = false; !result; ) {
            switch( choose_option() ) {
            case purchase_card:      result = do_purchase_card();      break;
            case deposit_money:      result = do_deposit_money();      break;
            case withdraw_money:     result = do_withdraw_money();     break;
            case transfer_money:     result = do_transfer_money();     break;
            case transfer_all_money: result = do_transfer_all_money(); break;
            default:
                return;
            }
            if( u.activity.type != ACT_NULL ) {
                break;
            }
        }
    }
private:
    void add_choice(int const i, char const *const title) { amenu.addentry(i, true, -1, title); }
    void add_info(int const i, char const *const title) { amenu.addentry(i, false, -1, title); }

    options choose_option()
    {
        if( u.activity.type == ACT_ATM ) {
            return static_cast<options>( u.activity.index );
        }
        amenu.query();
        uistate.iexamine_atm_selected = amenu.ret;
        return static_cast<options>( amenu.ret );
    }

    //! Reset and repopulate the menu; with a fair bit of work this could be more efficient.
    void reset(bool const clear = true) {
        const int card_count   = u.amount_of("cash_card");
        const int charge_count = card_count ? u.charges_of("cash_card") : 0;

        if (clear) {
            amenu.reset();
        }

        amenu.selected = uistate.iexamine_atm_selected;
        amenu.return_invalid = true;
        amenu.text = string_format(_("Welcome to the C.C.B.o.t.T. ATM. What would you like to do?\n"
                                     "Your current balance is: $%ld.%02ld"),
                                     u.cash / 100, u.cash % 100);

        if (u.cash >= 100) {
            add_choice(purchase_card, _("Purchase cash card?"));
        } else {
            add_info(purchase_card, _("You need $1.00 in your account to purchase a card."));
        }

        if (card_count && u.cash > 0) {
            add_choice(withdraw_money, _("Withdraw Money") );
        } else if (u.cash > 0) {
            add_info(withdraw_money, _("You need a cash card before you can withdraw money!"));
        } else {
            add_info(withdraw_money,
                _("You need money in your account before you can withdraw money!"));
        }

        if (charge_count) {
            add_choice(deposit_money, _("Deposit Money"));
        } else {
            add_info(deposit_money,
                _("You need a charged cash card before you can deposit money!"));
        }

        if (card_count >= 2 && charge_count) {
            add_choice(transfer_money, _("Transfer Money"));
            add_choice(transfer_all_money, _("Transfer All Money"));
        } else if (charge_count) {
            add_info(transfer_money, _("You need two cash cards before you can move money!"));
        } else {
            add_info(transfer_money,
                _("One of your cash cards must be charged before you can move money!"));
        }

        amenu.addentry(cancel, true, 'q', _("Cancel"));
    }

    //! print a bank statement for @p print = true;
    void finish_interaction(bool const print = true) {
        if (print) {
            add_msg(m_info, ngettext("Your account now holds %d cent.",
                                     "Your account now holds %d cents.", u.cash), u.cash);
        }

        u.moves -= 100;
    }

    //! Prompt for a card to use (includes worn items).
    item* choose_card(char const *const msg) {
        const int index = g->inv_for_filter(msg, [](item const& itm) {
            return itm.type->id == "cash_card";
        });

        if (index == INT_MIN) {
            add_msg(m_info, _("Never mind."));
            return nullptr; // player canceled
        }

        auto &itm = u.i_at(index);
        if (itm.type->id != "cash_card") {
            popup(_("Please insert cash cards only!"));
            return nullptr; // must have selected an equipped item
        }

        return &itm;
    };

    //! Prompt for an integral value clamped to [0, max].
    static long prompt_for_amount(char const *const msg, long const max) {
        const std::string formatted = string_format(msg, max);
        const int amount = std::atol(string_input_popup(
            formatted, 20, to_string(max), "", "", -1, true).c_str());

        return (amount > max) ? max : (amount <= 0) ? 0 : amount;
    };

    bool do_purchase_card() {
        const char *prompt = _("This will automatically deduct $1.00 from your bank account. Continue?");

        if (!query_yn(prompt)) {
            return false;
        }

        item card("cash_card", calendar::turn);
        card.charges = 0;
        u.i_add(card);
        u.cash -= 100;
        u.moves -= 100;
        finish_interaction();

        return true;
    }

    bool do_deposit_money() {
        item *src = choose_card(_("Insert card for deposit."));
        if (!src) {
            return false;
        }

        if (!src->charges) {
            popup(_("You can only deposit money from charged cash cards!"));
            return false;
        }

        const int amount = prompt_for_amount(ngettext(
            "Deposit how much? Max: %d cent. (0 to cancel) ",
            "Deposit how much? Max: %d cents. (0 to cancel) ", src->charges), src->charges);

        if (!amount) {
            return false;
        }

        src->charges -= amount;
        u.cash += amount;
        u.moves -= 100;
        finish_interaction();

        return true;
    }

    bool do_withdraw_money() {
        item *dst = choose_card(_("Insert card for withdrawal."));
        if (!dst) {
            return false;
        }

        const int amount = prompt_for_amount(ngettext(
            "Withdraw how much? Max: %d cent. (0 to cancel) ",
            "Withdraw how much? Max: %d cents. (0 to cancel) ", u.cash), u.cash);

        if (!amount) {
            return false;
        }

        dst->charges += amount;
        u.cash -= amount;
        u.moves -= 100;
        finish_interaction();

        return true;
    }

    bool do_transfer_money() {
        item *dst = choose_card(_("Insert card for deposit."));
        if (!dst) {
            return false;
        }

        item *src = choose_card(_("Insert card for withdrawal."));
        if (!src) {
            return false;
        } else if (dst == src) {
            popup(_("You must select a different card to move from!"));
            return false;
        } else if (!src->charges) {
            popup(_("You can only move money from charged cash cards!"));
            return false;
        }

        const int amount = prompt_for_amount(ngettext(
            "Transfer how much? Max: %d cent. (0 to cancel) ",
            "Transfer how much? Max: %d cents. (0 to cancel) ", src->charges), src->charges);

        if (!amount) {
            return false;
        }

        src->charges -= amount;
        dst->charges += amount;
        u.moves -= 100;
        finish_interaction();

        return true;
    }

    bool do_transfer_all_money() {
        item *dst;
        if( u.activity.type == ACT_ATM ) {
            u.activity.type = ACT_NULL; // stop for now, if required, it will be created again.
            dst = &u.i_at( u.activity.position );
            if( dst->is_null() || dst->typeId() != "cash_card" ) {
                return false;
            }
        } else {
            dst = choose_card( _("Insert card for bulk deposit.") );
            if( !dst ) {
                return false;
            }
        }

        for (auto &i : u.inv_dump()) {
            if( i == dst || i->charges <= 0 || i->type->id != "cash_card" ) {
                continue;
            }
            if( u.moves < 0 ) {
                // Money from `*i` could be transferred, but we're out of moves, schedule it for
                // the next turn. Putting this here makes sure there will be something to be
                // done next turn.
                u.assign_activity( ACT_ATM, 0, transfer_all_money, u.get_item_position( dst ) );
                break;
            }

            dst->charges += i->charges;
            i->charges =  0;
            u.moves    -= 10;
        }

        return true;
    }

    player &u;
    uimenu amenu;
};
} //namespace

void iexamine::atm(player *const p, map *, const tripoint& )
{
    atm_menu {*p}.start();
}

void iexamine::vending(player * const p, map * const m, const tripoint &examp)
{
    constexpr int moves_cost = 250;

    auto vend_items = m->i_at(examp);
    if (vend_items.empty()) {
        add_msg(m_info, _("The vending machine is empty!"));
        return;
    } else if (!p->has_charges("cash_card", 1)) {
        popup(_("You need a charged cash card to purchase things!"));
        return;
    }

    item *card = &p->i_at(g->inv_for_filter(_("Insert card for purchases."),
        [](item const &i) { return i.type->id == "cash_card"; }));

    if (card->is_null()) {
        return; // player cancelled selection
    } else if (card->type->id != "cash_card") {
        popup(_("Please insert cash cards only!"));
        return;
    } else if (card->charges == 0) {
        popup(_("You must insert a charged cash card!"));
        return;
    }

    int const padding_x  = std::max(0, TERMX - FULL_SCREEN_WIDTH ) / 2;
    int const padding_y  = std::max(0, TERMY - FULL_SCREEN_HEIGHT) / 2;
    int const window_h   = FULL_SCREEN_HEIGHT;
    int const w_items_w  = FULL_SCREEN_WIDTH / 2 - 1; // minus 1 for a gap
    int const w_info_w   = FULL_SCREEN_WIDTH / 2;
    int const list_lines = window_h - 4; // minus for header and footer

    constexpr int first_item_offset = 3; // header size

    WINDOW_PTR const w_ptr {newwin(window_h, w_items_w, padding_y, padding_x)};
    WINDOW_PTR const w_item_info_ptr {
        newwin(window_h, w_info_w,  padding_y, padding_x + w_items_w + 1)};

    WINDOW *w           = w_ptr.get();
    WINDOW *w_item_info = w_item_info_ptr.get();

    bool used_machine = false;
    input_context ctxt("VENDING_MACHINE");
    ctxt.register_updown();
    ctxt.register_action("CONFIRM");
    ctxt.register_action("QUIT");
    ctxt.register_action("HELP_KEYBINDINGS");

    // Collate identical items.
    // First, build a map {item::tname} => {item_it, item_it, item_it...}
    using iterator_t = decltype(std::begin(vend_items)); // map_stack::iterator doesn't exist.

    std::map<std::string, std::vector<iterator_t>> item_map;
    for (auto it = std::begin(vend_items); it != std::end(vend_items); ++it) {
        // |# {name}|
        // 123      4
        item_map[utf8_truncate(it->tname(), static_cast<size_t>(w_items_w - 4))].push_back(it);
    }

    // Next, put pointers to the pairs in the map in a vector to allow indexing.
    std::vector<std::map<std::string, std::vector<iterator_t>>::value_type*> item_list;
    item_list.reserve(item_map.size());
    for (auto &pair : item_map) {
        item_list.emplace_back(&pair);
    }

    int const lines_above = list_lines / 2;                  // lines above the selector
    int const lines_below = list_lines / 2 + list_lines % 2; // lines below the selector

    int cur_pos = 0;
    for (;;) {
        // | {title}|
        // 12       3
        const std::string title = utf8_truncate(string_format(
            _("Money left: %d"), card->charges), static_cast<size_t>(w_items_w - 3));

        int const num_items = item_list.size();
        int const page_size = std::min(num_items, list_lines);

        werase(w);
        wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
        mvwhline(w, first_item_offset - 1, 1, LINE_OXOX, w_items_w - 2);
        mvwaddch(w, first_item_offset - 1, 0, LINE_XXXO); // |-
        mvwaddch(w, first_item_offset - 1, w_items_w - 1, LINE_XOXX); // -|

        mvwprintz(w, 1, 2, c_ltgray, title.c_str());

        // Keep the item selector centered in the page.
        int page_beg = 0;
        int page_end = page_size;
        if (cur_pos < num_items - cur_pos) {
            page_beg = std::max(0, cur_pos - lines_above);
            page_end = std::min(num_items, page_beg + list_lines);
        } else {
            page_end = std::min(num_items, cur_pos + lines_below);
            page_beg = std::max(0, page_end - list_lines);
        }

        for( int line = 0; line < page_size; ++line ) {
            const int i = page_beg + line;
            auto const color = (i == cur_pos) ? h_ltgray : c_ltgray;
            auto const &elem = item_list[i];
            const int count = elem->second.size();
            const char c = (count < 10) ? ('0' + count) : '*';
            trim_and_print(w, first_item_offset + line, 1, w_items_w-3, color, "%c %s", c, elem->first.c_str());
        }

        draw_scrollbar(w, cur_pos, list_lines, num_items, first_item_offset);
        wrefresh(w);

        // Item info
        auto &cur_items = item_list[static_cast<size_t>(cur_pos)]->second;
        auto &cur_item  = cur_items.back();

        werase(w_item_info);
        // | {line}|
        // 12      3
        fold_and_print(w_item_info, 1, 2, w_info_w - 3, c_ltgray, cur_item->info(true));
        wborder(w_item_info, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );

        //+<{name}>+
        //12      34
        const std::string name = utf8_truncate(cur_item->display_name(), static_cast<size_t>(w_info_w - 4));
        mvwprintw(w_item_info, 0, 1, "<%s>", name.c_str());
        wrefresh(w_item_info);

        const std::string &action = ctxt.handle_input();
        if (action == "DOWN") {
            cur_pos = (cur_pos + 1) % num_items;
        } else if (action == "UP") {
            cur_pos = (cur_pos + num_items - 1) % num_items;
        } else if (action == "CONFIRM") {
            if ( cur_item->price() > card->charges ) {
                popup(_("That item is too expensive!"));
                continue;
            }

            if (!used_machine) {
                used_machine = true;
                p->moves -= moves_cost;
            }

            card->charges -= cur_item->price();
            p->i_add_or_drop( *cur_item );

            vend_items.erase( cur_item );
            cur_items.pop_back();
            if (!cur_items.empty()) {
                continue;
            }

            item_list.erase(std::begin(item_list) + cur_pos);
            if (item_list.empty()) {
                add_msg(_("With a beep, the empty vending machine shuts down"));
                return;
            } else if (cur_pos == num_items - 1) {
                cur_pos--;
            }
        } else if (action == "QUIT") {
            break;
        }
    }
}

void iexamine::toilet(player *p, map *m, const tripoint &examp)
{
    auto items = m->i_at(examp);
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

void iexamine::elevator(player *p, map *m, const tripoint &examp)
{
    (void)p; //unused
    if (!query_yn(_("Use the %s?"), m->tername(examp).c_str())) {
        return;
    }
    int movez = (examp.z < 0 ? 2 : -2);
    g->vertical_move( movez, false );
}

void iexamine::controls_gate(player *p, map *m, const tripoint &examp)
{
    if (!query_yn(_("Use the %s?"), m->tername( examp ).c_str())) {
        none(p, m, examp);
        return;
    }
    g->open_gate( examp, m->ter( examp ) );
}

void iexamine::cardreader(player *p, map *m, const tripoint &examp)
{
    itype_id card_type = (m->ter(examp) == t_card_science ? "id_science" :
                          "id_military");
    if (p->has_amount(card_type, 1) && query_yn(_("Swipe your ID card?"))) {
        p->moves -= 100;
        for(const tripoint &tmp : m->points_in_radius( examp, 3 ) ) {
            if (m->ter(tmp) == t_door_metal_locked) {
                m->ter_set(tmp, t_floor);
            }
        }
        //TODO only despawn turrets "behind" the door
        for (int i = 0; i < (int)g->num_zombies(); i++) {
            if ( (g->zombie(i).type->id == mon_turret) ||
                 (g->zombie(i).type->id == mon_turret_rifle) ) {
                g->remove_zombie(i);
                i--;
            }
        }
        add_msg(_("You insert your ID card."));
        add_msg(m_good, _("The nearby doors slide into the floor."));
        p->use_amount(card_type, 1);
    } else {
        switch( hack_attempt( *p ) ) {
            case HACK_FAIL:
                m->ter_set(examp, t_card_reader_broken);
                break;
            case HACK_NOTHING:
                add_msg(_("Nothing happens."));
                break;
            case HACK_SUCCESS:
                {
                    add_msg(_("You activate the panel!"));
                    add_msg(m_good, _("The nearby doors slide into the floor."));
                    m->ter_set(examp, t_card_reader_broken);
                    for(const tripoint &tmp : m->points_in_radius( examp, 3 ) ) {
                        if (m->ter(tmp) == t_door_metal_locked) {
                            m->ter_set(tmp, t_floor);
                        }
                    }
                }
                break;
            case HACK_UNABLE:
                add_msg(
                    m_info,
                    p->skillLevel( skill_computer ) > 0 ?
                        _("Looks like you need a %s, or a tool to hack it with.") :
                        _("Looks like you need a %s."),
                    item::nname( card_type ).c_str()
                );
                break;
        }
    }
}

void iexamine::rubble(player *p, map *m, const tripoint &examp)
{
    bool has_digging_tool = p->has_items_with_quality( "DIG", 2, 1 );
    if( !has_digging_tool ) {
        add_msg(m_info, _("If only you had a shovel..."));
        return;
    }

    // Ask if there's something possibly more interesting than this rubble here
    std::string xname = m->furnname(examp);
    if( ( m->veh_at( examp ) != nullptr ||
          !m->tr_at( examp ).is_null() ||
          g->critter_at( examp ) != nullptr ) &&
          !query_yn(_("Clear up that %s?"), xname.c_str() ) ) {
        none(p, m, examp);
        return;
    }

    // "Remove"
    p->moves -= 200;
    m->furn_set(examp, f_null);

    // "Remind"
    add_msg(_("You clear up that %s."), xname.c_str());
}

void iexamine::crate(player *p, map *m, const tripoint &examp)
{
    // Check for a crowbar in the inventory
    bool has_prying_tool = p->crafting_inventory().has_items_with_quality( "PRY", 1, 1 );
    if( !has_prying_tool ) {
        add_msg( m_info, _("If only you had a crowbar...") );
        return;
    }

    // Ask if there's something possibly more interesting than this crate here
    // Shouldn't happen (what kind of creature lives in a crate?), but better safe than getting complaints
    std::string xname = m->furnname(examp);
    if( ( m->veh_at( examp ) != nullptr ||
          !m->tr_at( examp ).is_null() ||
          g->critter_at( examp ) != nullptr ) &&
          !query_yn(_("Pry that %s?"), xname.c_str() ) ) {
        none(p, m, examp);
        return;
    }

    // HACK ALERT: player::items_with returns const item* vector and so can't be used
    // so we'll be making a fake crowbar here
    // Not a problem for now, but if crowbar iuse-s ever get different, this will need a fix
    item fakecrow( "crowbar", 0 );

    iuse dummy;
    dummy.crowbar( p, &fakecrow, false, examp );
}


void iexamine::chainfence( player *p, map *m, const tripoint &examp )
{
    if( !query_yn( _( "Climb %s?" ), m->tername( examp ).c_str() ) ) {
        none( p, m, examp );
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
        ///\EFFECT_DEX decreases chances of slipping while climbing
        int climb = p->dex_cur;
        if (p->has_trait( "BADKNEES" )) {
            climb = climb / 2;
        }
        if( one_in( climb ) ) {
            add_msg( m_bad, _( "You slip while climbing and fall down again." ) );
            if( climb <= 1 ) {
                add_msg( m_bad, _( "Climbing this is impossible in your current state." ) );
            }
            return;
        }
        p->moves += climb * 10;
        sfx::play_variant_sound( "plmove", "clear_obstacle", sfx::get_heard_volume(g->u.pos()) );
    }
    if( p->in_vehicle ) {
        m->unboard_vehicle( p->pos() );
    }
    p->setpos( examp );
    if( examp.x < SEEX * int( MAPSIZE / 2 ) || examp.y < SEEY * int( MAPSIZE / 2 ) ||
        examp.x >= SEEX * ( 1 + int( MAPSIZE / 2 ) ) || examp.y >= SEEY * ( 1 + int( MAPSIZE / 2 ) ) ) {
        if( &g->u == p ) {
            g->update_map( p );
        }
    }
}

void iexamine::bars(player *p, map *m, const tripoint &examp)
{
    if(!(p->has_trait("AMORPHOUS"))) {
        none(p, m, examp);
        return;
    }
    if ( ((p->encumb(bp_torso)) >= 10) && ((p->encumb(bp_head)) >= 10) &&
         (p->encumb(bp_foot_l) >= 10 ||
          p->encumb(bp_foot_r) >= 10) ) { // Most likely places for rigid gear that would catch on the bars.
        add_msg(m_info, _("Your amorphous body could slip though the %s, but your cumbersome gear can't."),
                m->tername(examp).c_str());
        return;
    }
    if (!query_yn(_("Slip through the %s?"), m->tername(examp).c_str())) {
        none(p, m, examp);
        return;
    }
    p->moves -= 200;
    add_msg(_("You slide right between the bars."));
    p->setpos( examp );
}

void iexamine::portable_structure(player *p, map *m, const tripoint &examp)
{
    const auto &fr = m->furn_at( examp );
    std::string name;
    std::string dropped;
    if( fr.id == "f_groundsheet" ) {
        name = "tent";
        dropped = "tent_kit";
    } else if( fr.id == "f_center_groundsheet" ) {
        name = "tent";
        dropped = "large_tent_kit";
    } else if( fr.id == "f_skin_groundsheet" ) {
        name = "shelter";
        dropped = "shelter_kit";
    } else if( fr.id == "f_ladder" ) {
        name = "ladder";
        dropped = "stepladder";
    } else {
        name = "bug";
        dropped = "null";
    }

    if( !query_yn(_("Take down the %s?"), name.c_str() ) ) {
        none( p, m, examp );
        return;
    }

    p->moves -= 200;
    int radius = std::max( 1, fr.bash.collapse_radius );
    for( const tripoint &pt : m->points_in_radius( examp, radius ) ) {
        m->furn_set( pt, f_null );
    }

    m->add_item_or_charges( examp, item( dropped, calendar::turn ) );
}

void iexamine::pit(player *p, map *m, const tripoint &examp)
{
    inventory map_inv;
    map_inv.form_from_map( p->pos(), 1);

    bool player_has = p->has_amount("2x4", 1);
    bool map_has = map_inv.has_amount("2x4", 1);

    // return if there is no 2x4 around
    if (!player_has && !map_has) {
        none(p, m, examp);
        return;
    }

    if (query_yn(_("Place a plank over the pit?"))) {
        // if both have, then ask to use the one on the map
        if (player_has && map_has) {
            if (query_yn(_("Use the plank at your feet?"))) {
                long quantity = 1;
                m->use_amount( p->pos(), 1, "2x4", quantity);
            } else {
                p->use_amount("2x4", 1);
            }
        } else if (player_has && !map_has) { // only player has plank
            p->use_amount("2x4", 1);
        } else if (!player_has && map_has) { // only map has plank
            long quantity = 1;
            m->use_amount( p->pos(), 1, "2x4", quantity);
        }

        if( m->ter(examp) == t_pit ) {
            m->ter_set(examp, t_pit_covered);
        } else if( m->ter(examp) == t_pit_spiked ) {
            m->ter_set(examp, t_pit_spiked_covered);
        } else if( m->ter(examp) == t_pit_glass ) {
            m->ter_set(examp, t_pit_glass_covered);
        }
        add_msg(_("You place a plank of wood over the pit."));
    }
}

void iexamine::pit_covered(player *p, map *m, const tripoint &examp)
{
    if(!query_yn(_("Remove cover?"))) {
        none(p, m, examp);
        return;
    }

    item plank("2x4", calendar::turn);
    add_msg(_("You remove the plank."));
    m->add_item_or_charges(p->pos(), plank);

    if( m->ter(examp) == t_pit_covered ) {
        m->ter_set(examp, t_pit);
    } else if( m->ter(examp) == t_pit_spiked_covered ) {
        m->ter_set(examp, t_pit_spiked);
    } else if( m->ter(examp) == t_pit_glass_covered ) {
        m->ter_set(examp, t_pit_glass);
    }
}

void iexamine::fence_post(player *p, map *m, const tripoint &examp)
{

    int ch = menu(true, _("Fence Construction:"), _("Rope Fence"),
                  _("Wire Fence"), _("Barbed Wire Fence"), _("Cancel"), NULL);
    switch (ch) {
    case 1: {
        if (p->has_amount("rope_6", 2)) {
            p->use_amount("rope_6", 2);
            m->ter_set(examp, t_fence_rope);
            p->moves -= 200;
        } else {
            add_msg(m_info, _("You need 2 six-foot lengths of rope to do that"));
        }
    }
    break;

    case 2: {
        if (p->has_amount("wire", 2)) {
            p->use_amount("wire", 2);
            m->ter_set(examp, t_fence_wire);
            p->moves -= 200;
        } else {
            add_msg(m_info, _("You need 2 lengths of wire to do that!"));
        }
    }
    break;

    case 3: {
        if (p->has_amount("wire_barbed", 2)) {
            p->use_amount("wire_barbed", 2);
            m->ter_set(examp, t_fence_barbed);
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

void iexamine::remove_fence_rope(player *p, map *m, const tripoint &examp)
{
    if(!query_yn(_("Remove %s?"), m->tername(examp).c_str())) {
        none(p, m, examp);
        return;
    }
    item rope("rope_6", calendar::turn);
    m->add_item_or_charges(p->pos(), rope);
    m->add_item_or_charges(p->pos(), rope);
    m->ter_set(examp, t_fence_post);
    p->moves -= 200;

}

void iexamine::remove_fence_wire(player *p, map *m, const tripoint &examp)
{
    if(!query_yn(_("Remove %s?"), m->tername(examp).c_str())) {
        none(p, m, examp);
        return;
    }

    item rope("wire", calendar::turn);
    m->add_item_or_charges(p->pos(), rope);
    m->add_item_or_charges(p->pos(), rope);
    m->ter_set(examp, t_fence_post);
    p->moves -= 200;
}

void iexamine::remove_fence_barbed(player *p, map *m, const tripoint &examp)
{
    if(!query_yn(_("Remove %s?"), m->tername(examp).c_str())) {
        none(p, m, examp);
        return;
    }

    item rope("wire_barbed", calendar::turn);
    m->add_item_or_charges(p->pos(), rope);
    m->add_item_or_charges(p->pos(), rope);
    m->ter_set(examp, t_fence_post);
    p->moves -= 200;
}

void iexamine::slot_machine(player *p, map*, const tripoint&)
{
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

void iexamine::safe(player *p, map *m, const tripoint &examp)
{
    if ( !( p->has_amount("stethoscope", 1) || p->has_bionic("bio_ears") ) ) {
        p->moves -= 100;
        // one_in(30^3) chance of guessing
        if (one_in(27000)) {
            p->add_msg_if_player(m_good, _("You mess with the dial for a little bit... and it opens!"));
            m->furn_set(examp, f_safe_o);
            return;
        } else {
            p->add_msg_if_player(m_info, _("You mess with the dial for a little bit."));
            return;
        }
    }

    if (query_yn(_("Attempt to crack the safe?"))) {
        if (p->is_deaf()) {
            add_msg(m_info, _("You can't crack a safe while deaf!"));
            return;
        }
         // 150 minutes +/- 20 minutes per mechanics point away from 3 +/- 10 minutes per
        // perception point away from 8; capped at 30 minutes minimum. *100 to convert to moves
        ///\EFFECT_PER speeds up safe cracking

        ///\EFFECT_MECHANICS speeds up safe cracking
        int moves = std::max(MINUTES(150) + (p->skillLevel( skill_mechanics ) - 3) * MINUTES(-20) +
                             (p->get_per() - 8) * MINUTES(-10), MINUTES(30)) * 100;

         p->assign_activity( ACT_CRACKING, moves );
         p->activity.placement = examp;
    }
}

void iexamine::gunsafe_ml(player *p, map *m, const tripoint &examp)
{
    std::string furn_name = m->tername(examp).c_str();
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

    p->practice( skill_mechanics, 1);
    ///\EFFECT_DEX speeds up lock picking gun safe

    ///\EFFECT_MECHANICS speeds up lock picking gun safe
    p->moves -= (1000 - (pick_quality * 100)) - (p->dex_cur + p->skillLevel( skill_mechanics )) * 5;
    ///\EFFECT_DEX increases chance of lock picking gun safe

    ///\EFFECT_MECHANICS increases chance of lock picking gun safe
    int pick_roll = (dice(2, p->skillLevel( skill_mechanics )) + dice(2, p->dex_cur)) * pick_quality;
    int door_roll = dice(4, 30);
    if (pick_roll >= door_roll) {
        p->practice( skill_mechanics, 1);
        add_msg(_("You successfully unlock the gun safe."));
        g->m.furn_set(examp, "f_safe_o");
    } else if (door_roll > (3 * pick_roll)) {
        add_msg(_("Your clumsy attempt jams the lock!"));
        g->m.furn_set(examp, "f_gunsafe_mj");
    } else {
        add_msg(_("The gun safe stumps your efforts to pick it."));
    }
}

void iexamine::gunsafe_el(player *p, map *m, const tripoint &examp)
{
    std::string furn_name = m->tername(examp).c_str();
    switch( hack_attempt( *p ) ) {
        case HACK_FAIL:
            p->add_memorial_log(pgettext("memorial_male", "Set off an alarm."),
                                pgettext("memorial_female", "Set off an alarm."));
            sounds::sound(p->pos(), 60, _("An alarm sounds!"));
            if (examp.z > 0 && !g->event_queued(EVENT_WANTED)) {
                g->add_event(EVENT_WANTED, int(calendar::turn) + 300, 0, p->global_sm_location());
            }
            break;
        case HACK_NOTHING:
            add_msg(_("Nothing happens."));
            break;
        case HACK_SUCCESS:
            add_msg(_("You successfully hack the gun safe."));
            g->m.furn_set(examp, "f_safe_o");
            break;
        case HACK_UNABLE:
            add_msg(
                m_info,
                p->skillLevel( skill_computer ) > 0 ?
                    _("You can't hack this gun safe without a hacking tool.") :
                    _("This electronic safe looks too complicated to open.")
            );
            break;
    }
}

void iexamine::bulletin_board(player *, map *m, const tripoint &examp)
{
    basecamp *camp = m->camp_at( examp );
    if (camp && camp->board_x() == examp.x && camp->board_y() == examp.y) {
        std::vector<std::string> options;
        options.push_back(_("Cancel"));
        // Causes a warning due to being unused, but don't want to delete
        // since it's clearly what's intended for future functionality.
        //int choice = menu_vec(true, camp->board_name().c_str(), options) - 1;
    } else {
        bool create_camp = m->allow_camp( examp );
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
                m->add_camp( examp, _("Home") );
            }
        }
    }
}

void iexamine::fault(player*, map*, const tripoint&)
{
    popup(_("\
This wall is perfectly vertical.  Odd, twisted holes are set in it, leading\n\
as far back into the solid rock as you can see.  The holes are humanoid in\n\
shape, but with long, twisted, distended limbs."));
}

void iexamine::pedestal_wyrm(player *p, map *m, const tripoint &examp)
{
    if (!m->i_at(examp).empty()) {
        none(p, m, examp);
        return;
    }
    // Send in a few wyrms to start things off.
    g->u.add_memorial_log(pgettext("memorial_male", "Awoke a group of dark wyrms!"),
                         pgettext("memorial_female", "Awoke a group of dark wyrms!"));
    int num_wyrms = rng(1, 4);
    for (int i = 0; i < num_wyrms; i++) {
        int tries = 0;
        tripoint monp = examp;
        do {
            monp.x = rng(0, SEEX * MAPSIZE);
            monp.y = rng(0, SEEY * MAPSIZE);
            tries++;
        } while (tries < 10 && !g->is_empty( monp ) &&
                    rl_dist( p->pos(), monp ) <= 2);
        if (tries < 10) {
            g->m.ter_set( monp, t_rock_floor);
            g->summon_mon(mon_dark_wyrm, monp);
        }
    }
    add_msg(_("The pedestal sinks into the ground, with an ominous grinding noise..."));
    sounds::sound(examp, 80, (""));
    m->ter_set(examp, t_rock_floor);
    g->add_event(EVENT_SPAWN_WYRMS, int(calendar::turn) + rng(5, 10));
}

void iexamine::pedestal_temple(player *p, map *m, const tripoint &examp)
{

    if (m->i_at(examp).size() == 1 &&
        m->i_at(examp)[0].type->id == "petrified_eye") {
        add_msg(_("The pedestal sinks into the ground..."));
        m->ter_set(examp, t_dirt);
        m->i_clear(examp);
        g->add_event(EVENT_TEMPLE_OPEN, int(calendar::turn) + 4);
    } else if (p->has_amount("petrified_eye", 1) &&
               query_yn(_("Place your petrified eye on the pedestal?"))) {
        p->use_amount("petrified_eye", 1);
        add_msg(_("The pedestal sinks into the ground..."));
        m->ter_set(examp, t_dirt);
        g->add_event(EVENT_TEMPLE_OPEN, int(calendar::turn) + 4);
    } else
        add_msg(_("This pedestal is engraved in eye-shaped diagrams, and has a \
large semi-spherical indentation at the top."));
}

void iexamine::door_peephole(player *p, map *m, const tripoint &examp) {
    if (m->is_outside(p->pos())) {
        p->add_msg_if_player( _("You cannot look through the peephole from the outside."));
        return;
    }

    // Peek through the peephole, or open the door.
    int choice = menu( true, _("Do what with the door?"),
                       _("Peek through peephole."), _("Open door."),
                       _("Cancel"), NULL );
    if( choice == 1 ) {
        // Peek
        g->peek( examp );
        p->add_msg_if_player( _("You peek through the peephole.") );
    } else if( choice == 2 ) {
        m->open_door( examp, true, false);
        p->add_msg_if_player( _("You open the door.") );
    } else {
        p->add_msg_if_player( _("Never mind."));
    }
}

void iexamine::fswitch(player *p, map *m, const tripoint &examp)
{
    if(!query_yn(_("Flip the %s?"), m->tername(examp).c_str())) {
        none(p, m, examp);
        return;
    }
    ter_id terid = m->ter(examp);
    p->moves -= 100;
    tripoint tmp = examp;
    int &x = tmp.x;
    int &y = tmp.y;
    for (y = examp.y; y <= examp.y + 5; y++ ) {
        for (x = 0; x < SEEX * MAPSIZE; x++) {
            if ( terid == t_switch_rg ) {
                if (m->ter(tmp) == t_rock_red) {
                    m->ter_set(tmp, t_floor_red);
                } else if (m->ter(tmp) == t_floor_red) {
                    m->ter_set(tmp, t_rock_red);
                } else if (m->ter(tmp) == t_rock_green) {
                    m->ter_set(tmp, t_floor_green);
                } else if (m->ter(tmp) == t_floor_green) {
                    m->ter_set(tmp, t_rock_green);
                }
            } else if ( terid == t_switch_gb ) {
                if (m->ter(tmp) == t_rock_blue) {
                    m->ter_set(tmp, t_floor_blue);
                } else if (m->ter(tmp) == t_floor_blue) {
                    m->ter_set(tmp, t_rock_blue);
                } else if (m->ter(tmp) == t_rock_green) {
                    m->ter_set(tmp, t_floor_green);
                } else if (m->ter(tmp) == t_floor_green) {
                    m->ter_set(tmp, t_rock_green);
                }
            } else if ( terid == t_switch_rb ) {
                if (m->ter(tmp) == t_rock_blue) {
                    m->ter_set(tmp, t_floor_blue);
                } else if (m->ter(tmp) == t_floor_blue) {
                    m->ter_set(tmp, t_rock_blue);
                } else if (m->ter(tmp) == t_rock_red) {
                    m->ter_set(tmp, t_floor_red);
                } else if (m->ter(tmp) == t_floor_red) {
                    m->ter_set(tmp, t_rock_red);
                }
            } else if ( terid == t_switch_even ) {
                if ((y - examp.y) % 2 == 1) {
                    if (m->ter(tmp) == t_rock_red) {
                        m->ter_set(tmp, t_floor_red);
                    } else if (m->ter(tmp) == t_floor_red) {
                        m->ter_set(tmp, t_rock_red);
                    } else if (m->ter(tmp) == t_rock_green) {
                        m->ter_set(tmp, t_floor_green);
                    } else if (m->ter(tmp) == t_floor_green) {
                        m->ter_set(tmp, t_rock_green);
                    } else if (m->ter(tmp) == t_rock_blue) {
                        m->ter_set(tmp, t_floor_blue);
                    } else if (m->ter(tmp) == t_floor_blue) {
                        m->ter_set(tmp, t_rock_blue);
                    }
                }
            }
        }
    }
    add_msg(m_warning, _("You hear the rumble of rock shifting."));
    g->add_event(EVENT_TEMPLE_SPAWN, calendar::turn + 3);
}

bool dead_plant( bool flower, player &p, map &m, const tripoint &examp )
{
    if (calendar::turn.get_season() == WINTER) {
        if( flower ) {
            add_msg( m_info, _("This flower is dead. You can't get it.") );
        } else {
            add_msg( m_info, _("This plant is dead. You can't get it.") );
        }

        iexamine::none( &p, &m, examp );
        return true;
    }

    return false;
}

bool can_drink_nectar( const player &p )
{
    return ((p.has_active_mutation("PROBOSCIS")) || (p.has_active_mutation("BEAK_HUM"))) &&
        ((p.get_hunger()) > 0) && (!(p.wearing_something_on(bp_mouth)));
}

bool drink_nectar( player &p )
{
    if( can_drink_nectar( p ) ) {
        p.moves -= 50; // Takes 30 seconds
        add_msg(_("You drink some nectar."));
        p.mod_hunger(-15);
        return true;
    }

    return false;
}

void iexamine::flower_poppy(player *p, map *m, const tripoint &examp)
{
    if( dead_plant( true, *p, *m, examp ) ) {
        return;
    }
    // TODO: Get rid of this section and move it to eating
    // Two y/n prompts is just too much
    if( can_drink_nectar( *p ) ) {
        if (!query_yn(_("You feel woozy as you explore the %s. Drink?"), m->furnname(examp).c_str())) {
            return;
        }
        p->moves -= 150; // You take your time...
        add_msg(_("You slowly suck up the nectar."));
        p->mod_hunger(-25);
        p->add_effect( effect_pkill2, 70);
        p->fatigue += 20;
        // Please drink poppy nectar responsibly.
        if (one_in(20)) {
            p->add_addiction(ADD_PKILLER, 1);
        }
    }
    if(!query_yn(_("Pick %s?"), m->furnname(examp).c_str())) {
        none(p, m, examp);
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

    m->furn_set(examp, f_null);
    m->spawn_item(examp, "poppy_bud");
}

void iexamine::flower_bluebell(player *p, map *m, const tripoint &examp)
{
    if( dead_plant( true, *p, *m, examp ) ) {
        return;
    }

    drink_nectar( *p );

    // There was a bud and flower spawn here
    // But those were useless, don't re-add until they get useful
    none( p, m, examp );
    return;
}

void iexamine::flower_dahlia(player *p, map *m, const tripoint &examp)
{
    if( dead_plant( true, *p, *m, examp ) ) {
        return;
    }

    if( drink_nectar( *p ) ) {
        return;
    }

    if( !p->has_items_with_quality( "DIG", 1, 1 ) ) {
        none(p, m, examp);
        add_msg( m_info, _( "If only you had a shovel to dig up those roots..." ) );
        return;
    }

    if( !query_yn( _("Pick %s?"), m->furnname(examp).c_str() ) ) {
        none(p, m, examp);
        return;
    }

    m->furn_set(examp, f_null);
    m->spawn_item(examp, "dahlia_root");
    // There was a bud and flower spawn here
    // But those were useless, don't re-add until they get useful
}

void iexamine::flower_datura(player *p, map *m, const tripoint &examp)
{
    if( dead_plant( false, *p, *m, examp ) ) {
        return;
    }

    if( drink_nectar( *p ) ) {
        return;
    }

    if(!query_yn(_("Pick %s?"), m->furnname(examp).c_str())) {
        none(p, m, examp);
        return;
    }
    m->furn_set(examp, f_null);
    m->spawn_item(examp, "datura_seed", 2, 6 );
}

void iexamine::flower_dandelion(player *p, map *m, const tripoint &examp)
{
    if( dead_plant( false, *p, *m, examp ) ) {
        return;
    }

    if( drink_nectar( *p ) ) {
        return;
    }

    if(!query_yn(_("Pick %s?"), m->furnname(examp).c_str())) {
        none(p, m, examp);
        return;
    }

    m->furn_set(examp, f_null);
    m->spawn_item(examp, "raw_dandelion", rng( 1, 4 ) );
}

void iexamine::examine_cattails(player *p, map *m, const tripoint &examp)
{
    if(!query_yn(_("Pick %s?"), m->furnname(examp).c_str())) {
        none(p, m, examp);
        return;
    }
    if (calendar::turn.get_season() != WINTER) {
        m->spawn_item( p->pos(), "cattail_stalk", rng( 1, 4 ), 0, calendar::turn );
    }
    m->furn_set(examp, f_null);
    m->spawn_item( p->pos(), "cattail_rhizome", 1, 0, calendar::turn );
}

void iexamine::flower_marloss(player *p, map *m, const tripoint &examp)
{
    if (calendar::turn.get_season() == WINTER) {
        add_msg(m_info, _("This flower is still alive, despite the harsh conditions..."));
    }
    if( can_drink_nectar( *p ) ) {
        if (!query_yn(_("You feel out of place as you explore the %s. Drink?"), m->furnname(examp).c_str())) {
            return;
        }
        p->moves -= 50; // Takes 30 seconds
        add_msg(m_bad, _("This flower tastes very wrong..."));
        // If you can drink flowers, you're post-thresh and the Mycus does not want you.
        p->add_effect( effect_teleglow, 100 );
    }
    if(!query_yn(_("Pick %s?"), m->furnname(examp).c_str())) {
        none(p, m, examp);
        return;
    }
    m->furn_set(examp, f_null);
    m->spawn_item(examp, "marloss_seed", 1, 3);
}

void iexamine::egg_sack_generic( player *p, map *m, const tripoint &examp,
                                 const mtype_id& montype )
{
    const std::string old_furn_name = m->furnname( examp );
    if( !query_yn( _( "Harvest the %s?" ), old_furn_name.c_str() ) ) {
        none( p, m, examp );
        return;
    }
    m->spawn_item( examp, "spider_egg", rng( 1, 4 ) );
    m->furn_set( examp, f_egg_sacke );
    if( one_in( 2 ) ) {
        int monster_count = 0;
        const std::vector<tripoint> pts = closest_tripoints_first( 1, examp );
        for( const auto &pt : pts ) {
            if( g->is_empty( pt ) && one_in( 3 ) ) {
                g->summon_mon( montype, pt );
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

void iexamine::egg_sackbw( player *p, map *m, const tripoint &examp )
{
    egg_sack_generic( p, m, examp, mon_spider_widow_giant_s );
}

void iexamine::egg_sackcs( player *p, map *m, const tripoint &examp )
{
    egg_sack_generic( p, m, examp, mon_spider_cellar_giant_s );
}

void iexamine::egg_sackws( player *p, map *m, const tripoint &examp )
{
    egg_sack_generic( p, m, examp, mon_spider_web_s );
}

void iexamine::fungus(player *p, map *m, const tripoint &examp)
{
    add_msg(_("The %s crumbles into spores!"), m->furnname(examp).c_str());
    m->create_spores( examp, p);
    m->furn_set(examp, f_null);
    p->moves -= 50;
}

void iexamine::dirtmound(player *p, map *m, const tripoint &examp)
{

    if( !warm_enough_to_plant() ) {
        add_msg(m_info, _("It is too cold to plant anything now."));
        return;
    }
    /* ambient_light_at() not working?
    if (m->ambient_light_at(examp) < LIGHT_AMBIENT_LOW) {
        add_msg(m_info, _("It is too dark to plant anything now."));
        return;
    }*/
    std::vector<item *> seed_inv = p->items_with( []( const item &itm ) {
        return itm.is_seed();
    } );
    if( seed_inv.empty() ) {
        add_msg(m_info, _("You have no seeds to plant."));
        return;
    }
    if (m->i_at(examp).size() != 0) {
        add_msg(_("Something's lying there..."));
        return;
    }

    // Make lists of unique seed types and names for the menu(no multiple hemp seeds etc)
    std::map<itype_id, int> seed_map;
    for( auto &seed : seed_inv ) {
        seed_map[seed->typeId()] += (seed->charges > 0 ? seed->charges : 1);
    }

    using seed_tuple = std::tuple<itype_id, std::string, int>;
    std::vector<seed_tuple> seed_entries;
    for( const auto &pr : seed_map ) {
        seed_entries.emplace_back(
            pr.first, item::nname( pr.first, pr.second ), pr.second );
    }

    // Sort by name
    std::sort( seed_entries.begin(), seed_entries.end(),
        []( const seed_tuple &l, const seed_tuple &r ) {
            return std::get<1>( l ).compare( std::get<1>( r ) ) < 0;
    } );

    // Choose seed
    // Don't use y/n prompt, stick with one kind of menu
    uimenu smenu;
    smenu.return_invalid = true;
    smenu.text = _("Use which seed?");
    int count = 0;
    for( const auto &entry : seed_entries ) {
        smenu.addentry( count++, true, MENU_AUTOASSIGN, _("%s (%d)"),
            std::get<1>( entry ).c_str(), std::get<2>( entry ) );
    }
    smenu.query();

    int seed_index = smenu.ret;

    // Did we cancel?
    if( seed_index < 0 || seed_index >= (int)seed_entries.size() ) {
        add_msg(_("You saved your seeds for later."));
        return;
    }
    const auto &seed_id = std::get<0>( seed_entries[seed_index] );

    // Actual planting
    std::list<item> used_seed;
    if( item::count_by_charges( seed_id ) ) {
        used_seed = p->use_charges( seed_id, 1 );
    } else {
        used_seed = p->use_amount( seed_id, 1 );
    }
    used_seed.front().bday = calendar::turn;
    m->add_item_or_charges( examp, used_seed.front() );
    m->set( examp, t_dirt, f_plant_seed );
    p->moves -= 500;
    add_msg(_("Planted %s"), std::get<1>( seed_entries[seed_index] ).c_str() );
}

std::list<item> iexamine::get_harvest_items( const itype &type, const int plant_count,
                                             const int seed_count, const bool byproducts )
{
    std::list<item> result;
    if( !type.seed ) {
        return result;
    }
    const islot_seed &seed_data = *type.seed;
    const itype_id &seed_type = type.id;

    const auto add = [&]( const itype_id &id, const int count ) {
        item new_item( id, calendar::turn );
        if( new_item.count_by_charges() && count > 0 ) {
            new_item.charges *= count;
            new_item.charges /= seed_data.fruit_div;
            result.push_back( new_item );
        } else if( count > 0 ) {
            result.insert( result.begin(), count, new_item );
        }
    };

    if( seed_data.spawn_seeds ) {
        add( seed_type, seed_count );
    }

    add( seed_data.fruit_id, plant_count );

    if( byproducts ) {
        for( auto &b : seed_data.byproducts ) {
            add( b, 1 );
        }
    }

    return result;
}

void iexamine::aggie_plant(player *p, map *m, const tripoint &examp)
{
    if( m->i_at( examp ).empty() ) {
        m->i_clear( examp );
        m->furn_set( examp, f_null );
        debugmsg( "Missing seed in plant furniture!" );
        return;
    }
    const item &seed = m->i_at( examp ).front();
    if( !seed.is_seed() ) {
        debugmsg( "The seed item %s is not a seed!", seed.tname().c_str() );
        return;
    }

    const std::string pname = seed.get_plant_name();

    if (m->furn(examp) == f_plant_harvest && query_yn(_("Harvest the %s?"), pname.c_str() )) {
        const std::string &seedType = seed.typeId();
        if (seedType == "fungal_seeds") {
            fungus(p, m, examp);
            m->i_clear(examp);
        } else if (seedType == "marloss_seed") {
            fungus(p, m, examp);
            m->i_clear(examp);
            if (p->has_trait("M_DEPENDENT") && ((p->get_hunger() > 500) || p->thirst > 300 )) {
                m->ter_set(examp, t_marloss);
                add_msg(m_info, _("We have altered this unit's configuration to extract and provide local nutriment.  The Mycus provides."));
            } else if ( (p->has_trait("M_DEFENDER")) || ( (p->has_trait("M_SPORES") || p->has_trait("M_FERTILE")) &&
                one_in(2)) ) {
                g->summon_mon( mon_fungal_blossom, examp );
                add_msg(m_info, _("The seed blooms forth!  We have brought true beauty to this world."));
            } else if ( (p->has_trait("THRESH_MYCUS")) || one_in(4)) {
                m->furn_set(examp, f_flower_marloss);
                add_msg(m_info, _("The seed blossoms rather rapidly..."));
            } else {
                m->furn_set(examp, f_flower_fungal);
                add_msg(m_info, _("The seed blossoms into a flower-looking fungus."));
            }
        } else { // Generic seed, use the seed item data
            const itype &type = *seed.type;
            m->i_clear(examp);
            m->furn_set(examp, f_null);

            int skillLevel = p->skillLevel( skill_survival );
            ///\EFFECT_SURVIVAL increases number of plants harvested from a seed
            int plantCount = rng(skillLevel / 2, skillLevel);
            if (plantCount >= 12) {
                plantCount = 12;
            } else if( plantCount <= 0 ) {
                plantCount = 1;
            }
            const int seedCount = std::max( 1l, rng( plantCount / 4, plantCount / 2 ) );
            for( auto &i : get_harvest_items( type, plantCount, seedCount, true ) ) {
                m->add_item_or_charges( examp, i );
            }
            p->moves -= 500;
        }
    } else if (m->furn(examp) != f_plant_harvest) {
        if (m->i_at(examp).size() > 1) {
            add_msg(m_info, _("This %s has already been fertilized."), pname.c_str() );
            return;
        }
        std::vector<const item *> f_inv = p->all_items_with_flag( "FERTILIZER" );
        if( f_inv.empty() ) {
        add_msg(m_info, _("You have no fertilizer for the %s."), pname.c_str());
        return;
        }
        if (query_yn(_("Fertilize the %s"), pname.c_str() )) {
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
                f_names.push_back(_("Cancel"));
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
            if (!world->WORLD_OPTIONS.empty()) {
                fertilizerEpoch = 14400 * (world->WORLD_OPTIONS["SEASON_LENGTH"] * 0.2) ;
            }

            item &seed = m->i_at( examp ).front();

             if( seed.bday > fertilizerEpoch ) {
              seed.bday -= fertilizerEpoch;
            } else {
              seed.bday = 0;
            }
            // The plant furniture has the NOITEM token which prevents adding items on that square,
            // spawned items are moved to an adjacent field instead, but the fertilizer token
            // must be on the square of the plant, therefor this hack:
            const auto old_furn = m->furn( examp );
            m->furn_set( examp, f_null );
            m->spawn_item( examp, "fertilizer", 1, 1, (int)calendar::turn );
            m->furn_set( examp, old_furn );
        }
    }
}

// Highly modified fermenting vat functions
void iexamine::kiln_empty(player *p, map *m, const tripoint &examp)
{
    furn_id cur_kiln_type = m->furn( examp );
    furn_id next_kiln_type = f_null;
    if( cur_kiln_type == f_kiln_empty ) {
        next_kiln_type = f_kiln_full;
    } else if( cur_kiln_type == f_kiln_metal_empty ) {
        next_kiln_type = f_kiln_metal_full;
    } else {
        debugmsg( "Examined furniture has action kiln_empty, but is of type %s", m->get_furn( examp ).c_str() );
        return;
    }

    std::vector< std::string > kilnable{ "wood", "bone" };
    bool fuel_present = false;
    auto items = m->i_at( examp );
    for( auto i : items ) {
        if( i.typeId() == "charcoal" ) {
            add_msg( _("This kiln already contains charcoal.") );
            add_msg( _("Remove it before firing the kiln again.") );
            return;
        } else if( i.made_of_any( kilnable ) ) {
            fuel_present = true;
        } else {
            add_msg( m_bad, _("This kiln contains %s, which can't be made into charcoal!"), i.tname( 1, false ).c_str() );
            return;
        }
    }

    if( !fuel_present ) {
        add_msg( _("This kiln is empty. Fill it with wood or bone and try again.") );
        return;
    }

    ///\EFFECT_CARPENTRY decreases loss when firing a kiln
    SkillLevel &skill = p->skillLevel( skill_carpentry );
    int loss = 90 - 2 * skill; // We can afford to be inefficient - logs and skeletons are cheap, charcoal isn't

    // Burn stuff that should get charred, leave out the rest
    int total_volume = 0;
    for( auto i : items ) {
        total_volume += i.volume( false, false );
    }

    auto char_type = item::find_type( "unfinished_charcoal" );
    int char_charges = ( 100 - loss ) * total_volume * char_type->ammo->def_charges / 100 / char_type->volume;
    if( char_charges < 1 ) {
        add_msg( _("The batch in this kiln is too small to yield any charcoal.") );
        return;
    }

    if( !p->has_charges( "fire" , 1 ) ) {
        add_msg( _("This kiln is ready to be fired, but you have no fire source.") );
        return;
    } else if( !query_yn( _("Fire the kiln?") ) ) {
        return;
    }

    p->use_charges( "fire", 1 );
    g->m.i_clear( examp );
    m->furn_set( examp, next_kiln_type );
    item result( "unfinished_charcoal", calendar::turn.get_turn() );
    result.charges = char_charges;
    m->add_item( examp, result );
    add_msg( _("You fire the charcoal kiln.") );
    int practice_amount = ( 10 - skill ) * total_volume / 100; // 50 at 0 skill, 25 at 5, 10 at 8
    p->practice( skill_carpentry, practice_amount );
}

void iexamine::kiln_full(player *, map *m, const tripoint &examp)
{
    furn_id cur_kiln_type = m->furn( examp );
    furn_id next_kiln_type = f_null;
    if ( cur_kiln_type == f_kiln_full ) {
        next_kiln_type = f_kiln_empty;
    } else if( cur_kiln_type == f_kiln_metal_full ) {
        next_kiln_type = f_kiln_metal_empty;
    } else {
        debugmsg( "Examined furniture has action kiln_full, but is of type %s", m->get_furn( examp ).c_str() );
        return;
    }

    auto items = m->i_at( examp );
    if( items.empty() ) {
        add_msg( _("This kiln is empty...") );
        m->furn_set(examp, next_kiln_type);
        return;
    }
    int last_bday = items[0].bday;
    for( auto i : items ) {
        if( i.typeId() == "unfinished_charcoal" && i.bday > last_bday ) {
            last_bday = i.bday;
        }
    }
    auto char_type = item::find_type( "charcoal" );
    add_msg( _("There's a charcoal kiln there.") );
    const int firing_time = HOURS(6); // 5 days in real life
    int time_left = firing_time - calendar::turn.get_turn() + items[0].bday;
    if( time_left > 0 ) {
        add_msg( _("It should take %d minutes to finish burning."), time_left / MINUTES(1) + 1 );
        return;
    }

    int total_volume = 0;
    // Burn stuff that should get charred, leave out the rest
    for( auto item_it = items.begin(); item_it != items.end(); ) {
        if( item_it->typeId() == "unfinished_charcoal" || item_it->typeId() == "charcoal" ) {
            total_volume += item_it->volume( false, false );
            item_it = items.erase( item_it );
        } else {
            item_it++;
        }
    }

    item result( "charcoal", calendar::turn.get_turn() );
    result.charges = total_volume * char_type->ammo->def_charges / char_type->volume;
    m->add_item( examp, result );
    m->furn_set( examp, next_kiln_type);
}

void iexamine::fvat_empty(player *p, map *m, const tripoint &examp)
{
    itype_id brew_type;
    bool to_deposit = false;
    bool vat_full = false;
    bool brew_present = false;
    int charges_on_ground = 0;
    auto items = m->i_at(examp);
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
            b_names.push_back(_("Cancel"));
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
        item &brew = m->i_at(examp).front();
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
        m->i_clear(examp);
        //This is needed to bypass NOITEM
        m->add_item( examp, brew );
        p->moves -= 250;
    }
    if (vat_full || query_yn(_("Start fermenting cycle?"))) {
        m->i_at( examp).front().bday = calendar::turn;
        m->furn_set(examp, f_fvat_full);
        if (vat_full) {
            add_msg(_("The vat is full, so you close the lid and start the fermenting cycle."));
        } else {
            add_msg(_("You close the lid and start the fermenting cycle."));
        }
    }
}

void iexamine::fvat_full(player *p, map *m, const tripoint &examp)
{
    bool liquid_present = false;
    for (int i = 0; i < (int)m->i_at(examp).size(); i++) {
        if (!(m->i_at(examp)[i].made_of(LIQUID)) || liquid_present) {
            m->add_item_or_charges(examp, m->i_at(examp)[i]);
            m->i_rem( examp, i );
            i--;
        } else {
            liquid_present = true;
        }
    }
    if (!liquid_present) {
        debugmsg("fvat_full was empty or contained non-liquids only!");
        m->furn_set(examp, f_fvat_empty);
        return;
    }
    item brew_i = m->i_at(examp)[0];
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
                m->furn(examp) == f_fvat_full && query_yn(_("Finish brewing?")) ) {
                //declare fermenting result as the brew's ID minus "brew_"
                itype_id alcoholType = m->i_at(examp)[0].typeId().substr(5);
                ///\EFFECT_COOKING >4 prevents hb_beer from turning into just beer
                SkillLevel &cooking = p->skillLevel( skill_cooking );
                if (alcoholType == "hb_beer" && cooking < 5) {
                    alcoholType = alcoholType.substr(3);    //hb_beer -> beer
                }
                item booze(alcoholType, 0);
                booze.charges = brew_i.charges;
                booze.bday = brew_i.bday;

                m->i_clear(examp);
                m->add_item( examp, booze );
                p->moves -= 500;

                //low xp: you also get xp from crafting the brew
                p->practice( skill_cooking, std::min(brew_time / 600, 72) );
                add_msg(_("The %s is now ready for bottling."), booze.tname().c_str());
            }
        }
    } else { //Booze is done, so bottle it!
        item &booze = m->i_at(examp).front();
        if( g->handle_liquid( booze, true, false) ) {
            m->furn_set(examp, f_fvat_empty);
            add_msg(_("You squeeze the last drops of %s from the vat."), booze.tname().c_str());
            m->i_clear( examp );
        }
    }
}

//probably should move this functionality into the furniture JSON entries if we want to have more than a few "kegs"
static int get_keg_cap( const furn_t &furn ) {
    if( furn.id == "f_standing_tank" )  { return 1200; } //the furniture was a "standing tank", so can hold 1200
    else                                { return 600; } //default to old default value
    //add additional cases above
}

void iexamine::keg(player *p, map *m, const tripoint &examp)
{
    int keg_cap = get_keg_cap( m->furn_at(examp) );
    bool liquid_present = false;
    for (int i = 0; i < (int)m->i_at(examp).size(); i++) {
        if (!m->i_at(examp)[i].made_of( LIQUID ) || liquid_present) {
            m->add_item_or_charges(examp, m->i_at(examp)[i]);
            m->i_rem( examp, i );
            i--;
        } else {
            liquid_present = true;
        }
    }
    if (!liquid_present) {
        // Get list of all drinks
        auto drinks_inv = p->items_with( []( const item &it ) {
            return it.made_of( LIQUID );
        } );
        if ( drinks_inv.empty() ) {
            add_msg(m_info, _("You don't have any drinks to fill the %s with."), m->name(examp).c_str());
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
            drink_names.push_back(_("Cancel"));
            drink_index = menu_vec(false, _("Store which drink?"), drink_names) - 1;
            if (drink_index == (int)drink_names.size() - 1) {
                drink_index = -1;
            }
        } else { //Only one drink type was in inventory, so it's automatically used
            if (!query_yn(_("Fill the %1$s with %2$s?"), m->name(examp).c_str(), drink_names[0].c_str())) {
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
            add_msg(_("You completely fill the %1$s with %2$s."),
                    m->name(examp).c_str(), drink.tname().c_str());
        } else {
            add_msg(_("You fill the %1$s with %2$s."), m->name(examp).c_str(),
                    drink.tname().c_str());
        }
        p->moves -= 250;
        m->i_clear(examp);
        m->add_item( examp, drink );
        return;
    } else {
        auto drink = m->i_at(examp).begin();
        std::vector<std::string> menu_items;
        enum options {
            FILL_CONTAINER,
            HAVE_A_DRINK,
            REFILL,
            EXAMINE,
            CANCEL,
        };
        uimenu selectmenu;
        selectmenu.addentry( FILL_CONTAINER, true, MENU_AUTOASSIGN, _("Fill a container with %s"),
                            drink->tname().c_str() );
        selectmenu.addentry( HAVE_A_DRINK, drink->is_food(), MENU_AUTOASSIGN, _("Have a drink") );
        selectmenu.addentry( REFILL, true, MENU_AUTOASSIGN, _("Refill") );
        selectmenu.addentry( EXAMINE, true, MENU_AUTOASSIGN, _("Examine") );
        selectmenu.addentry( CANCEL, true, MENU_AUTOASSIGN, _("Cancel") );

        selectmenu.return_invalid = true;
        selectmenu.text = _("Select an action");
        selectmenu.selected = 0;
        selectmenu.query();

        switch( static_cast<options>( selectmenu.ret ) ) {
        case FILL_CONTAINER:
            if( g->handle_liquid(*drink, true, false) ) {
                add_msg(_("You squeeze the last drops of %1$s from the %2$s."), drink->tname().c_str(),
                        m->name(examp).c_str());
                m->i_clear( examp );
            }
            return;

        case HAVE_A_DRINK:
            if( !p->eat( &*drink, dynamic_cast<const it_comest *>(drink->type) ) ) {
                return; // They didn't actually drink
            }

            drink->charges--;
            if (drink->charges == 0) {
                add_msg(_("You squeeze the last drops of %1$s from the %2$s."), drink->tname().c_str(),
                        m->name(examp).c_str());
                m->i_clear( examp );
            }
            p->moves -= 250;
            return;

        case REFILL: {
            int charges_held = p->charges_of(drink->typeId());
            int d_vol = drink->volume(false, true) / 1000;
            if (d_vol >= keg_cap) {
                add_msg(_("The %s is completely full."), m->name(examp).c_str());
                return;
            }
            if (charges_held < 1) {
                add_msg(m_info, _("You don't have any %1$s to fill the %2$s with."),
                        drink->tname().c_str(), m->name(examp).c_str());
                return;
            }
            for (int i = 0; i < charges_held; i++) {
                p->use_charges(drink->typeId(), 1);
                drink->charges++;
                int d_vol = drink->volume(false, true) / 1000;
                if (d_vol >= keg_cap) {
                    add_msg(_("You completely fill the %1$s with %2$s."), m->name(examp).c_str(),
                            drink->tname().c_str());
                    p->moves -= 250;
                    return;
                }
            }
            add_msg(_("You fill the %1$s with %2$s."), m->name(examp).c_str(),
                    drink->tname().c_str());
            p->moves -= 250;
            return;
        }

        case EXAMINE: {
            add_msg(m_info, _("That is a %s."), m->name(examp).c_str());
            int full_pct = drink->volume(false, true) / (keg_cap * 10);
            add_msg(m_info, _("It contains %s (%d), %d%% full."),
                    drink->tname().c_str(), drink->charges, full_pct);
            return;
        }

        case CANCEL:
            return;
        }
    }
}

void pick_plant(player *p, map *m, const tripoint &examp,
                std::string itemType, ter_id new_ter, bool seeds)
{
    if (!query_yn(_("Harvest the %s?"), m->tername(examp).c_str())) {
        iexamine::none(p, m, examp);
        return;
    }

    SkillLevel &survival = p->skillLevel( skill_survival );
    if (survival < 1) {
        p->practice( skill_survival, rng(5, 12) );
    } else if (survival < 6) {
        p->practice( skill_survival, rng(1, 12 / survival) );
    }

    int plantBase = rng(2, 5);
    ///\EFFECT_SURVIVAL increases number of plants harvested
    int plantCount = rng(plantBase, plantBase + survival / 2);
    if (plantCount > 12) {
        plantCount = 12;
    }

    m->spawn_item( p->pos(), itemType, plantCount, 0, calendar::turn);

    if (seeds) {
        m->spawn_item( p->pos(), "seed_" + itemType, 1,
                      rng(plantCount / 4, plantCount / 2), calendar::turn);
    }

    m->ter_set(examp, new_ter);
}

void iexamine::harvest_tree_shrub(player *p, map *m, const tripoint &examp)
{
    if ( ((p->has_trait("PROBOSCIS")) || (p->has_trait("BEAK_HUM"))) &&
         ((p->get_hunger()) > 0) && (!(p->wearing_something_on(bp_mouth))) &&
         (calendar::turn.get_season() == SUMMER || calendar::turn.get_season() == SPRING) ) {
        p->moves -= 100; // Need to find a blossom (assume there's one somewhere)
        add_msg(_("You find a flower and drink some nectar."));
        p->mod_hunger(-15);
    }
    //if the fruit is not ripe yet
    if (calendar::turn.get_season() != m->get_ter_harvest_season(examp)) {
        std::string fruit = item::nname(m->get_ter_harvestable(examp), 10);
        fruit[0] = toupper(fruit[0]);
        add_msg(m_info, _("%1$s ripen in %2$s."), fruit.c_str(), season_name(m->get_ter_harvest_season(examp)).c_str());
        return;
    }
    //if the fruit has been recently harvested
    if (m->has_flag(TFLAG_HARVESTED, examp)){
        add_msg(m_info, _("This %s has already been harvested. Harvest it again next year."), m->tername(examp).c_str());
        return;
    }

    bool seeds = false;
    if (m->has_flag("SHRUB", examp)) { // if shrub, it gives seeds. todo -> trees give seeds(?) -> trees plantable
        seeds = true;
    }
    pick_plant(p, m, examp, m->get_ter_harvestable(examp), m->get_ter_transforms_into(examp), seeds);
}

void iexamine::tree_pine(player *p, map *m, const tripoint &examp)
{
    if(!query_yn(_("Pick %s?"), m->tername(examp).c_str())) {
        none(p, m, examp);
        return;
    }
    m->spawn_item(p->pos(), "pine_bough", 2, 12 );
    m->spawn_item( p->pos(), "pinecone", rng( 1, 4 ) );
    m->ter_set(examp, t_tree_deadpine);
}

void iexamine::tree_hickory(player *p, map *m, const tripoint &examp)
{
    harvest_tree_shrub(p,m,examp);
    ///\EFFECT_SURVIVAL >0 allows digging up hickory root
    if( !( p->skillLevel( skill_survival ) > 0 ) ) {
        return;
    }
    if( !p->has_items_with_quality( "DIG", 1, 1 ) ) {
        add_msg(m_info, _("You have no tool to dig with..."));
        return;
    }
    if(!query_yn(_("Dig up %s? This kills the tree!"), m->tername(examp).c_str())) {
        return;
    }
    m->spawn_item(p->pos(), "hickory_root", rng(1,4) );
    m->ter_set(examp, t_tree_hickory_dead);
    ///\EFFECT_SURVIVAL speeds up hickory root digging
    p->moves -= 2000 / ( p->skillLevel( skill_survival ) + 1 ) + 100;
    return;
    none(p, m, examp);
}

void iexamine::tree_bark(player *p, map *m, const tripoint &examp)
{
    if(!query_yn(_("Pick %s?"), m->tername(examp).c_str())) {
        none(p, m, examp);
        return;
    }
    m->spawn_item( p->pos(), m->get_ter_harvestable(examp), rng( 1, 2 ) );
    m->ter_set(examp, m->get_ter_transforms_into(examp));
}

void iexamine::shrub_marloss(player *p, map *m, const tripoint &examp)
{
    if (p->has_trait("THRESH_MYCUS")) {
        pick_plant(p, m, examp, "mycus_fruit", t_shrub_fungal);
    } else if (p->has_trait("THRESH_MARLOSS")) {
        m->spawn_item( examp, "mycus_fruit" );
        m->ter_set(examp, t_fungus);
        add_msg( m_info, _("The shrub offers up a fruit, then crumbles into a fungal bed."));
    } else {
        pick_plant(p, m, examp, "marloss_berry", t_shrub_fungal);
    }
}

void iexamine::tree_marloss(player *p, map *m, const tripoint &examp)
{
    if (p->has_trait("THRESH_MYCUS")) {
        pick_plant(p, m, examp, "mycus_fruit", t_tree_fungal);
        if (p->has_trait("M_DEPENDENT") && one_in(3)) {
            // Folks have a better shot at keeping fed.
            add_msg(m_info, _("We have located a particularly vital nutrient deposit underneath this location."));
            add_msg(m_good, _("Additional nourishment is available."));
            m->ter_set(examp, t_marloss_tree);
        }
    } else if (p->has_trait("THRESH_MARLOSS")) {
        m->spawn_item( p->pos(), "mycus_fruit" );
        m->ter_set(examp, t_tree_fungal);
        add_msg(m_info, _("The tree offers up a fruit, then shrivels into a fungal tree."));
    } else {
        pick_plant(p, m, examp, "marloss_berry", t_tree_fungal);
    }
}

void iexamine::shrub_wildveggies( player *p, map *m, const tripoint &examp )
{
    // Ask if there's something possibly more interesting than this shrub here
    if( ( !m->i_at( examp ).empty() ||
          m->veh_at( examp ) != nullptr ||
          !m->tr_at( examp ).is_null() ||
          g->critter_at( examp ) != nullptr ) &&
          !query_yn(_("Forage through %s?"), m->tername( examp ).c_str() ) ) {
        none( p, m, examp );
        return;
    }

    add_msg( _("You forage through the %s."), m->tername( examp ).c_str() );
    ///\EFFECT_SURVIVAL speeds up foraging
    int move_cost = 100000 / ( 2 * p->skillLevel( skill_survival ) + 5 );
    ///\EFFECT_PER randomly speeds up foraging
    move_cost /= rng( std::max( 4, p->per_cur ), 4 + p->per_cur * 2 );
    p->assign_activity( ACT_FORAGE, move_cost, 0 );
    p->activity.placement = examp;
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

void iexamine::recycler(player *p, map *m, const tripoint &examp)
{
    auto items_on_map = m->i_at(examp);

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
    const std::string weight_str = string_format("%.3f %s", convert_weight(steel_weight),
                                                            weight_units());
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

    sounds::sound(examp, 80, _("Ka-klunk!"));

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
        m->spawn_item(p->pos(), "steel_lump");
    }

    for (int i = 0; i < num_sheets; i++) {
        m->spawn_item(p->pos(), "sheet_metal");
    }

    for (int i = 0; i < num_chunks; i++) {
        m->spawn_item(p->pos(), "steel_chunk");
    }

    for (int i = 0; i < num_scraps; i++) {
        m->spawn_item(p->pos(), "scrap");
    }
}

void iexamine::trap(player *p, map *m, const tripoint &examp)
{
    const auto &tr = m->tr_at(examp);
    if( p == nullptr || !p->is_player() || tr.is_null() ) {
        return;
    }
    const int possible = tr.get_difficulty();
    bool seen = tr.can_see( examp, *p );
    if( seen && possible >= 99 ) {
        add_msg(m_info, _("That %s looks too dangerous to mess with. Best leave it alone."),
            tr.name.c_str());
        return;
    }
    // Some traps are not actual traps. Those should get a different query.
    if( seen && possible == 0 && tr.get_avoidance() == 0 ) { // Separated so saying no doesn't trigger the other query.
        if( query_yn(_("There is a %s there. Take down?"), tr.name.c_str()) ) {
            m->disarm_trap(examp);
        }
    } else if( seen && query_yn( _("There is a %s there.  Disarm?"), tr.name.c_str() ) ) {
        m->disarm_trap(examp);
    }
}

void iexamine::water_source(player *p, map *m, const tripoint &examp)
{
    item water = m->water_from( examp );
    p->assign_activity(ACT_FILL_LIQUID, -1, -1);
    p->activity.str_values.push_back(water.typeId());
    p->activity.values.push_back(water.poison);
    p->activity.values.push_back(water.bday);
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
    if (toolt != NULL && toolt->ammo_id != "NULL") {
        const std::string ammoid = default_ammo(toolt->ammo_id);
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

void iexamine::reload_furniture(player *p, map *m, const tripoint &examp)
{
    const furn_t &f = m->furn_at(examp);
    itype *type = f.crafting_pseudo_item_type();
    itype *ammo = f.crafting_ammo_item_type();
    if (type == NULL || ammo == NULL) {
        add_msg(m_info, _("This %s can not be reloaded!"), f.name.c_str());
        return;
    }
    const int amount_in_furn = count_charges_in_list( ammo, m->i_at( examp ) );
    if( amount_in_furn > 0 ) {
        //~ %1$s - furniture, %2$d - number, %3$s items.
        add_msg(_("The %1$s contains %2$d %3$s."), f.name.c_str(), amount_in_furn, ammo->nname(amount_in_furn).c_str());
    }
    const int max_amount_in_furn = f.max_volume * ammo->stack_size / ammo->volume;
    const int max_reload_amount = max_amount_in_furn - amount_in_furn;
    if( max_reload_amount <= 0 ) {
        return;
    }
    const int amount_in_inv = p->charges_of( ammo->id );
    if( amount_in_inv == 0 ) {
        //~ Reloading or restocking a piece of furniture, for example a forge.
        add_msg(m_info, _("You need some %1$s to reload this %2$s."), ammo->nname(2).c_str(), f.name.c_str());
        return;
    }
    const long max_amount = std::min( amount_in_inv, max_reload_amount );
    //~ Loading fuel or other items into a piece of furniture.
    const std::string popupmsg = string_format(_("Put how many of the %1$s into the %2$s?"),
                                 ammo->nname(max_amount).c_str(), f.name.c_str());
    long amount = std::atoi( string_input_popup( popupmsg, 20,
                                  to_string(max_amount),
                                  "", "", -1, true).c_str() );
    if (amount <= 0 || amount > max_amount) {
        return;
    }
    p->use_charges( ammo->id, amount );
    auto items = m->i_at(examp);
    for( auto & itm : items ) {
        if( itm.type == ammo ) {
            itm.charges += amount;
            amount = 0;
            break;
        }
    }
    if (amount != 0) {
        item it(ammo->id, 0);
        it.charges = amount;
        m->add_item( examp, it );
    }
    add_msg(_("You reload the %s."), m->furnname(examp).c_str());
    p->moves -= 100;
}

void iexamine::curtains(player *p, map *m, const tripoint &examp)
{
    if (m->is_outside(p->pos())) {
        p->add_msg_if_player( _("You cannot get to the curtains from the outside."));
        return;
    }

    // Peek through the curtains, or tear them down.
    int choice = menu( true, _("Do what with the curtains?"),
                       _("Peek through the curtains."), _("Tear down the curtains."),
                       _("Cancel"), NULL );
    if( choice == 1 ) {
        // Peek
        g->peek(examp );
        p->add_msg_if_player( _("You carefully peek through the curtains.") );
    } else if( choice == 2 ) {
        // Mr. Gorbachev, tear down those curtains!
        m->ter_set( examp, "t_window_no_curtains" );
        m->spawn_item( p->pos(), "nail", 1, 4 );
        m->spawn_item( p->pos(), "sheet", 2 );
        m->spawn_item( p->pos(), "stick" );
        m->spawn_item( p->pos(), "string_36" );
        p->moves -= 200;
        p->add_msg_if_player( _("You tear the curtains and curtain rod off the windowframe.") );
    } else {
        p->add_msg_if_player( _("Never mind."));
    }
}

void iexamine::sign(player *p, map *m, const tripoint &examp)
{
    std::string existing_signage = m->get_signage( examp );
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
                m->set_signage( examp, signage);
                p->add_msg_if_player(m_info, spray_painted_message.c_str());
                p->moves -= 2 * signage.length();
                p->use_charges("spray_can", required_writing_charges);
            }
        } else {
            p->add_msg_if_player(m_neutral, ignore_message.c_str());
        }
    }
}

static int getNearPumpCount(map *m, const tripoint &p)
{
    const int radius = 12;

    int result = 0;

    tripoint tmp = p;
    int &i = tmp.x;
    int &j = tmp.y;
    for (i = p.x - radius; i <= p.x + radius; i++) {
        for (j = p.y - radius; j <= p.y + radius; j++) {
            if (m->ter_at(tmp).id == "t_gas_pump" || m->ter_at(tmp).id == "t_gas_pump_a") {
                result++;
            }
        }
    }
    return result;
}

static tripoint getNearFilledGasTank(map *m, const tripoint &center, long &gas_units)
{
    const int radius = 24;

    tripoint tank_loc = tripoint_min;
    int distance = radius + 1;
    gas_units = 0;

    tripoint tmp = center;
    int &i = tmp.x;
    int &j = tmp.y;
    for (i = center.x - radius; i <= center.x + radius; i++) {
        for (j = center.y - radius; j <= center.y + radius; j++) {
            if (m->ter_at(tmp).id != "t_gas_tank") {
                continue;
            }

            int new_distance = rl_dist( center, tmp );

            if( new_distance >= distance ) {
                continue;
            }
            if( tank_loc == tripoint_min ) {
                // Return a potentially empty tank, but only if we don't find a closer full one.
                tank_loc = tmp;
            }
            for( auto &k : m->i_at(tmp)) {
                if(k.made_of(LIQUID)) {
                    const long units = k.liquid_units( k.charges );

                    distance = new_distance;
                    tank_loc = tmp;
                    gas_units = units;
                    break;
                }
            }
        }
    }
    return tank_loc;
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

static tripoint getGasPumpByNumber(map *m, const tripoint &p, int number)
{
    const int radius = 12;

    int k = 0;

    tripoint tmp = p;
    int &i = tmp.x;
    int &j = tmp.y;
    for (i = p.x - radius; i <= p.x + radius; i++) {
        for (j = p.y - radius; j <= p.y + radius; j++) {
            if( (m->ter_at(tmp).id == "t_gas_pump" ||
                 m->ter_at(tmp).id == "t_gas_pump_a") && number == k++) {
                return tmp;
            }
        }
    }

    return tripoint_min;
}

static bool toPumpFuel(map *m, const tripoint &src, const tripoint &dst, long units)
{
    if (src == tripoint_min) {
        return false;
    }

    auto items = m->i_at( src );
    for( auto item_it = items.begin(); item_it != items.end(); ++item_it ) {
        if( item_it->made_of(LIQUID)) {
            const long amount = item_it->liquid_charges( units );

            if( item_it->charges < amount ) {
                return false;
            }

            item_it->charges -= amount;

            item liq_d(item_it->type->id, calendar::turn);
            liq_d.charges = amount;

            ter_t backup_pump = m->ter_at(dst);
            m->ter_set( dst, "t_null");
            m->add_item_or_charges(dst, liq_d);
            m->ter_set(dst, backup_pump.id);

            if( item_it->charges < 1 ) {
                items.erase( item_it );
            }

            return true;
        }
    }

    return false;
}

static long fromPumpFuel(map *m, const tripoint &dst, const tripoint &src)
{
    if (src == tripoint_min) {
        return -1;
    }

    auto items = m->i_at( src );
    for( auto item_it = items.begin(); item_it != items.end(); ++item_it ) {
        if( item_it->made_of(LIQUID)) {
            // how much do we have in the pump?
            item liq_d(item_it->type->id, calendar::turn);
            liq_d.charges = item_it->charges;

            // add the charges to the destination
            ter_t backup_tank = m->ter_at(dst);
            m->ter_set(dst, "t_null");
            m->add_item_or_charges(dst, liq_d);
            m->ter_set(dst, backup_tank.id);

            // remove the liquid from the pump
            long amount = item_it->charges;
            items.erase( item_it );
            return item_it->liquid_units( amount );
        }
    }
    return -1;
}

static void turnOnSelectedPump(map *m, const tripoint &p, int number)
{
    const int radius = 12;

    int k = 0;
    tripoint tmp = p;
    int &i = tmp.x;
    int &j = tmp.y;
    for (i = p.x - radius; i <= p.x + radius; i++) {
        for (j = p.y - radius; j <= p.y + radius; j++) {
            if ((m->ter_at(tmp).id == "t_gas_pump" || m->ter_at(tmp).id == "t_gas_pump_a") ) {
                if (number == k++) {
                    m->ter_set(tmp, "t_gas_pump_a");
                } else {
                    m->ter_set(tmp, "t_gas_pump");
                }
            }
        }
    }
}

void iexamine::pay_gas(player *p, map *m, const tripoint &examp)
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

    int pumpCount = getNearPumpCount(m, examp);
    if (pumpCount == 0) {
        popup(str_to_illiterate_str(_("Failure! No gas pumps found!")).c_str());
        return;
    }

    long tankGasUnits;
    tripoint pTank = getNearFilledGasTank(m, examp, tankGasUnits);
    if (pTank == tripoint_min) {
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
                                  to_string( uistate.ags_pay_gas_selected_pump + 1 );
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
                            str_to_illiterate_str(_("Pump ")) + to_string(i + 1) );
        }
        amenu.query();
        choice = amenu.ret;

        if (choice == 0) {
            return;
        }

        uistate.ags_pay_gas_selected_pump = choice - 1;

        turnOnSelectedPump(m, examp, uistate.ags_pay_gas_selected_pump);

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
        long amount = std::atoi(string_input_popup(popupmsg, 20,
                                     to_string(max), "", "", -1, true).c_str()
                                    );
        if (amount <= 0) {
            return;
        }
        if (amount > max) {
            amount = max;
        }

        tripoint pGasPump = getGasPumpByNumber(m, examp,  uistate.ags_pay_gas_selected_pump);
        if (!toPumpFuel(m, pTank, pGasPump, amount)) {
            return;
        }

        sounds::sound(p->pos(), 6, _("Glug Glug Glug"));

        cashcard->charges -= amount * pricePerUnit;

        add_msg(m_info, ngettext("Your cash card now holds %d cent.",
                                 "Your cash card now holds %d cents.",
                                 cashcard->charges), cashcard->charges);
        p->moves -= 100;
        return;
    }

    if( hack == choice ) {
        switch( hack_attempt( *p ) ) {
            case HACK_UNABLE:
                break;
            case HACK_FAIL:
                p->add_memorial_log(pgettext("memorial_male", "Set off an alarm."),
                                    pgettext("memorial_female", "Set off an alarm."));
                sounds::sound(p->pos(), 60, _("An alarm sounds!"));
                if (examp.z > 0 && !g->event_queued(EVENT_WANTED)) {
                    g->add_event(EVENT_WANTED, int(calendar::turn) + 300, 0, p->global_sm_location());
                }
                break;
            case HACK_NOTHING:
                add_msg(_("Nothing happens."));
                break;
            case HACK_SUCCESS:
                tripoint pGasPump = getGasPumpByNumber(m, examp, uistate.ags_pay_gas_selected_pump);
                if (toPumpFuel(m, pTank, pGasPump, tankGasUnits)) {
                    add_msg(_("You hack the terminal and route all available fuel to your pump!"));
                    sounds::sound(p->pos(), 6, _("Glug Glug Glug Glug Glug Glug Glug Glug Glug"));
                } else {
                    add_msg(_("Nothing happens."));
                }
                break;
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
        tripoint pGasPump = getGasPumpByNumber(m, examp, uistate.ags_pay_gas_selected_pump);
        long amount = fromPumpFuel(m, pTank, pGasPump);
        if (amount >= 0) {
            sounds::sound(p->pos(), 6, _("Glug Glug Glug"));
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

void iexamine::climb_down( player *p, map *m, const tripoint &examp )
{
    if( !m->has_zlevels() ) {
        // No climbing down in 2D mode
        return;
    }

    if( !m->valid_move( p->pos(), examp, false, true ) ) {
        // Covered with something
        return;
    }

    tripoint where = examp;
    tripoint below = examp;
    below.z--;
    while( m->valid_move( where, below, false, true ) ) {
        where.z--;
        below.z--;
    }

    const int height = examp.z - where.z;
    if( height == 0 ) {
        p->add_msg_if_player( _("You can't climb down there") );
        return;
    }

    const int climb_cost = p->climbing_cost( where, examp );
    const auto fall_mod = p->fall_damage_mod();
    std::string query_str = ngettext("Looks like %d storey. Jump down?",
                                     "Looks like %d stories. Jump down?",
                                     height);
    if( height > 1 && !query_yn(query_str.c_str(), height) ) {
        return;
    } else if( height == 1 ) {
        std::string query;
        if( climb_cost <= 0 && fall_mod > 0.8 ) {
            query = _("You probably won't be able to get up and jumping down may hurt. Jump?");
        } else if( climb_cost <= 0 ) {
            query = _("You probably won't be able to get back up. Climb down?");
        } else if( climb_cost < 200 ) {
            query = _("You should be able to climb back up easily if you climb down there. Climb down?");
        } else {
            query = _("You may have problems climbing back up. Climb down?");
        }

        if( !query_yn( query.c_str() ) ) {
            return;
        }
    }

    p->moves -= 100 + 100 * fall_mod;
    p->setpos( examp );
    if( climb_cost > 0 || rng_float( 0.8, 1.0 ) > fall_mod ) {
        // One tile of falling less (possibly zero)
        g->vertical_move( -1, true );
    }

    m->creature_on_trap( *p );
}

/**
* Given then name of one of the above functions, returns the matching function
* pointer. If no match is found, defaults to iexamine::none but prints out a
* debug message as a warning.
* @param function_name The name of the function to get.
* @return A function pointer to the specified function.
*/
iexamine_function iexamine_function_from_string(std::string const &function_name)
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
    if( "crate" == function_name ) {
        return &iexamine::crate;
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
        return &iexamine::flower_bluebell;
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
    if ("examine_cattails" == function_name) {
        return &iexamine::examine_cattails;
    }
    if ("egg_sackbw" == function_name) {
        return &iexamine::egg_sackbw;
    }
    if ("egg_sackcs" == function_name) {
        return &iexamine::egg_sackcs;
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
    if ("tree_bark" == function_name) {
        return &iexamine::tree_bark;
    }
    if ("shrub_marloss" == function_name) {
        return &iexamine::shrub_marloss;
    }
    if ("tree_marloss" == function_name) {
        return &iexamine::tree_marloss;
    }
    if ("tree_hickory" == function_name) {
        return &iexamine::tree_hickory;
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
    if ("reload_furniture" == function_name) {
        return &iexamine::reload_furniture;
    }
    if ("curtains" == function_name) {
        return &iexamine::curtains;
    }
    if ("sign" == function_name) {
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
    if ("kiln_empty" == function_name) {
        return &iexamine::kiln_empty;
    }
    if ("kiln_full" == function_name) {
        return &iexamine::kiln_full;
    }
    if( "climb_down" == function_name ) {
        return &iexamine::climb_down;
    }
    //No match found
    debugmsg("Could not find an iexamine function matching '%s'!", function_name.c_str());
    return &iexamine::none;
}

hack_result iexamine::hack_attempt(player &p) {
    if( p.has_trait( "ILLITERATE" ) ) {
        return HACK_UNABLE;
    }
    bool using_electrohack = ( p.has_amount( "electrohack", 1 ) &&
                               query_yn( _( "Use electrohack?" ) ) );
    bool using_fingerhack = ( !using_electrohack && p.has_bionic( "bio_fingerhack" ) &&
                             p.power_level  > 0  &&
                             query_yn( _( "Use fingerhack?" ) ) );

    if( ! ( using_electrohack || using_fingerhack ) ) {
        return HACK_UNABLE;
    }

    p.moves -= 500;
    p.practice( skill_computer, 20 );
    ///\EFFECT_COMPUTER increases success chance of hacking card readers
    int success = rng( p.skillLevel( skill_computer ) / 4 - 2, p.skillLevel( skill_computer ) * 2 );
    success += rng( -3, 3 );
    if( using_fingerhack ) {
        success++;
    }

    // odds go up with int>8, down with int<8
    // 4 int stat is worth 1 computer skill here
    ///\EFFECT_INT increases success chance of hacking card readers
    success += rng( 0, int( ( p.int_cur - 8 ) / 2 ) );

    if( success < 0 ) {
        add_msg( _( "You cause a short circuit!" ) );
        if( success <= -5 ) {
            if( using_electrohack ) {
                add_msg( m_bad, _( "Your electrohack is ruined!" ) );
                p.use_amount( "electrohack", 1 );
            } else {
                add_msg( m_bad, _( "Your power is drained!" ) );
                p.charge_power( -rng( 0, p.power_level ) );
            }
        }
        return HACK_FAIL;
    } else if( success < 6 ) {
        return HACK_NOTHING;
    } else {
        return HACK_SUCCESS;
    }
}
