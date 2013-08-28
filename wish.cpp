#include "game.h"
#include "output.h"
#include "keypress.h"
#include "item_factory.h"
#include <sstream>
#include "text_snippets.h"
#include "helper.h"
#include "uistate.h"

#define LESS(a, b) ((a)<(b)?(a):(b))

void game::mutation_wish()
{
    WINDOW* w_list = newwin(25, 30, 0,  0);
    WINDOW* w_info = newwin(25, 50, 0, 30);

    std::vector<std::string> vTraits;
    for (std::map<std::string, trait>::iterator iter = traits.begin(); iter != traits.end(); ++iter) {
        vTraits.push_back(iter->first);
    }

    int result_selected = 0;
    int iCurrentLine = 0;
    int iStartPos = 0;
    int iContentHeight = 23;
    int ch = '.';

    bool search = false;
    std::string pattern;
    std::string info;
    std::vector<int> search_results;

    do {
        werase(w_info);
        werase(w_list);
        mvwprintw(w_list, 0, 0, _("Mutate: "));

        if (ch == '>' || ch == KEY_NPAGE) {
            search = false;
            if (!search_results.empty()) {
                result_selected++;
                if (result_selected >= search_results.size()) {
                    result_selected = 0;
                }

                iCurrentLine = search_results[result_selected];
            }
        } else if (ch == '<' || ch == KEY_PPAGE) {
            search = false;
            if (!search_results.empty()) {
                result_selected--;
                if (result_selected < 0) {
                    result_selected = search_results.size() - 1;
                }

                iCurrentLine = search_results[result_selected];
            }
        } else if (search) {
            if (ch == KEY_BACKSPACE || ch == 127) {
                if (pattern.length() > 0) {
                    pattern.erase(pattern.end() - 1);
                }

            } else {
                pattern += ch;
                search_results.clear();
            }

            if (search) {
                for (int i = 0; i < vTraits.size(); i++) {
                    if (traits[vTraits[i]].name.find(pattern) != std::string::npos) {
                        result_selected = 0;
                        search_results.push_back(i);
                    }
                }

                if (search_results.size() > 0) {
                    iCurrentLine = search_results[result_selected];
                }
            }

        } else {	// Not searching; scroll by keys
            if (ch == 'j') {
                iCurrentLine++;
                if (iCurrentLine >= vTraits.size()) {
                    iCurrentLine = 0;
                }
            } else if (ch == 'k') {
                iCurrentLine--;
                if (iCurrentLine < 0) {
                    iCurrentLine = vTraits.size()-1;
                }
            } else if (ch == '/') {
                search = true;
                pattern =  "";
                search_results.clear();
            }
        }

        if (!search_results.empty()) {
            mvwprintz(w_list, 0, 11, c_green, "%s               ", pattern.c_str());
        } else if (pattern.length() > 0) {
            mvwprintz(w_list, 0, 11, c_red, _("%s not found!            "),pattern.c_str());
        }

        if (vTraits.size() > iContentHeight) {
            iStartPos = iCurrentLine - (iContentHeight - 1) / 2;

            if (iStartPos < 0) {
                iStartPos = 0;
            } else if (iStartPos + iContentHeight > vTraits.size()) {
                iStartPos = vTraits.size() - iContentHeight;
            }
        }

        //Draw Mutations
        for (int i = iStartPos; i < vTraits.size(); i++) {
            if (i >= iStartPos && i < iStartPos + ((iContentHeight > vTraits.size()) ? vTraits.size() : iContentHeight)) {
                nc_color cLineColor = c_white;

                mvwprintz(w_list, 1 + i - iStartPos, 0, (iCurrentLine == i) ? hilite(cLineColor) : cLineColor, traits[vTraits[i]].name.c_str());
            }
        }

        mvwprintw(w_info, 1, 0, mutation_data[vTraits[iCurrentLine]].valid ? _("Valid") : _("Nonvalid"));
        int line2 = 2;

        bool found = false;
        std::string sCat = "";
        for (std::map<std::string, std::vector<std::string> >::iterator iter = mutations_category.begin(); iter != mutations_category.end(); ++iter) {
            std::vector<std::string> group = mutations_category[iter->first];

            for (int j = 0; !found && j < group.size(); j++) {
                if (group[j] == vTraits[iCurrentLine]) {
                    found = true;
                    sCat = iter->first;
                    break;
                }
            }
        }

        mvwprintw(w_info, line2, 0, _("Prereqs:"));
        for (int j = 0; j < mutation_data[vTraits[iCurrentLine]].prereqs.size(); j++) {
            mvwprintw(w_info, line2, 9, traits[ mutation_data[vTraits[iCurrentLine]].prereqs[j] ].name.c_str());
            line2++;
        }

        mvwprintw(w_info, line2, 0, _("Cancels:"));
        for (int j = 0; j < mutation_data[vTraits[iCurrentLine]].cancels.size(); j++) {
            mvwprintw(w_info, line2, 9, traits[ mutation_data[vTraits[iCurrentLine]].cancels[j] ].name.c_str());
            line2++;
        }

        mvwprintw(w_info, line2, 0, _("Becomes:"));
        for (int j = 0; j < mutation_data[vTraits[iCurrentLine]].replacements.size(); j++) {
            mvwprintw(w_info, line2, 9, traits[ mutation_data[vTraits[iCurrentLine]].replacements[j] ].name.c_str());
            line2++;
        }

        mvwprintw(w_info, line2, 0, _("Add-ons:"));
        for (int j = 0; j < mutation_data[vTraits[iCurrentLine]].additions.size(); j++) {
            mvwprintw(w_info, line2, 9, traits[ mutation_data[vTraits[iCurrentLine]].additions[j] ].name.c_str());
            line2++;
        }

        mvwprintw(w_info, line2, 0, _("Category: "));
        for (int j = 0; j < mutation_data[vTraits[iCurrentLine]].category.size(); j++) {
            mvwprintw(w_info, line2, 10, mutation_data[vTraits[iCurrentLine]].category[j].c_str());
            line2++;
        }

        wrefresh(w_info);
        wrefresh(w_list);

        if (search) {
            ch = getch();
        } else {
            ch = input();
        }
    } while (ch != '\n' && ch != 27);
    clear();

    if (ch == '\n') {
        do {
            u.mutate_towards(this, vTraits[iCurrentLine]);
        } while (!u.has_trait(vTraits[iCurrentLine]));
    }

    delwin(w_info);
    delwin(w_list);
}


class wish_monster_callback: public uimenu_callback
{
    public:
        int lastent;           // last menu entry
        std::string msg;       // feedback mesage
        bool friendly;         // spawn friendly critter?
        WINDOW *w_info;        // ui_parent menu's padding area 
        monster tmp;           // scrap critter for monster::print_info
        bool started;          // if unset, intialize window
        std::string padding;   // ' ' x window width

        wish_monster_callback() {
            started = false;
            friendly = false;
            msg = "";
            lastent = -2;
            w_info = NULL;
            padding = "";
        }

        void setup(uimenu *menu) {
            w_info = newwin(menu->w_height - 2, menu->pad_right , 1, menu->w_x + menu->w_width - 2 - menu->pad_right);
            padding = std::string( getmaxx(w_info), ' ' );
            werase(w_info);
            wrefresh(w_info);
        }

        virtual bool key(int key, int entnum, uimenu *menu) {
            if ( key == 'f' ) {
                friendly = !friendly;
                lastent = -2; // force tmp monster regen
                return true;  // tell menu we handled keypress
            }
            return false;
        }

        virtual void select(int entnum, uimenu *menu) {
            if ( ! started ) {
                started = true;
                setup(menu);
            }
            if (entnum != lastent) {
                lastent = entnum;
                tmp = monster(g->mtypes[entnum]);
                if (friendly) {
                    tmp.friendly = -1;
                }
            }

            werase(w_info);
            tmp.print_info(g, w_info);

            std::string header = string_format("#%d: %s", entnum, g->mtypes[entnum]->name.c_str()
                                              );
            mvwprintz(w_info, 1, ( getmaxx(w_info) - header.size() ) / 2, c_cyan, "%s",
                      header.c_str()
                     );

            mvwprintz(w_info, getmaxy(w_info) - 3, 0, c_green, "%s", msg.c_str());
            msg = padding;
            mvwprintw(w_info, getmaxy(w_info) - 2, 0, "[/] find, [f] friendly, [q]uit");
            //wrefresh(w_info); // for some reason this makes everything disappear on first run? Not needed, at any rate.
        }

        virtual void refresh(uimenu *menu) {
            wrefresh(w_info);
        }

        ~wish_monster_callback() {
            werase(w_info);
            wrefresh(w_info);
            delwin(w_info);
        }
};

void game::wishmonster(int x, int y)
{
    uimenu wmenu;
    wmenu.w_x = 0;
    wmenu.w_width = ( TERMX - getmaxx(w_terrain) - 30 > 24 ? getmaxx(w_terrain) : TERMX );
    wmenu.pad_right = ( wmenu.w_width - 30 );
    wmenu.return_invalid = true;
    wmenu.selected = uistate.wishmonster_selected;
    wish_monster_callback *cb = new wish_monster_callback();
    wmenu.callback = cb;

    for (int i = 0; i < mtypes.size(); i++) {
        wmenu.addentry( i, true, 0, "%s", mtypes[i]->name.c_str() );
        wmenu.entries[i].extratxt.txt = string_format("%c", mtypes[i]->sym);
        wmenu.entries[i].extratxt.color = mtypes[i]->color;
        wmenu.entries[i].extratxt.left = 1;
    }

    do {
        wmenu.query();
        if ( wmenu.ret >= 0 ) {
            monster mon = monster(g->mtypes[wmenu.ret]);
            if (cb->friendly) {
                mon.friendly = -1;
            }
            point spawn = ( x == -1 && y == -1 ? look_around() : point ( x, y ) );
            if (spawn.x != -1) {
                mon.spawn(spawn.x, spawn.y);
#define old_game_z 1
// If this errors and you've merged pr 2690, undefine old_game_z (or remove these ifdefs alltogether)
#ifdef old_game_z
                z.push_back(mon);
#else
                add_zombie(mon);
#endif
                cb->msg = _("Monster spawned, choose another or 'q' to quit.");
                wmenu.redraw();
            }
        }
    } while ( wmenu.keypress != 'q' && wmenu.keypress != KEY_ESCAPE && wmenu.keypress != ' ' );
    delete cb;
    cb = NULL;
    return;
}

class wish_item_callback: public uimenu_callback
{
    public:
        int lastlen;
        bool incontainer;
        std::string msg;
        item tmp;
        wish_item_callback() {
            lastlen = 0;
            incontainer = false;
            msg = "";
        }
        virtual bool key(int key, int entnum, uimenu *menu) {
            if ( key == 'f' ) {
                incontainer = !incontainer;
                return true;
            }
            return false;
        }

        virtual void select(int entnum, uimenu *menu) {
            const int starty = 3;
            const int startx = menu->w_width - menu->pad_right;
            itype *ity = item_controller->find_template(standard_itype_ids[entnum]);

            std::string padding = std::string(menu->pad_right - 1, ' ');

            for(int i = 0; i < lastlen + starty + 1; i++ ) {
                mvwprintw(menu->window, 1 + i, startx, "%s", padding.c_str() );
            }

            if ( ity != NULL ) {
                tmp.make(item_controller->find_template(standard_itype_ids[entnum]));
                tmp.bday = g->turn;
                if (tmp.is_tool()) {
                    tmp.charges = dynamic_cast<it_tool *>(tmp.type)->max_charges;
                } else if (tmp.is_ammo()) {
                    tmp.charges = 100;
                } else if (tmp.is_gun()) {
                    tmp.charges = 0;
                } else if (tmp.is_gunmod() && (tmp.has_flag("MODE_AUX") ||
                                               tmp.typeId() == "spare_mag")) {
                    tmp.charges = 0;
                } else {
                    tmp.charges = -1;
                }
                if( tmp.is_stationary() ) {
                    tmp.note = SNIPPET.assign( (dynamic_cast<it_stationary *>(tmp.type))->category );
                }

                std::vector<std::string> desc = foldstring(tmp.info(true), menu->pad_right - 1);
                int dsize = desc.size();
                if ( dsize > menu->w_height - 5 ) {
                    dsize = menu->w_height - 5;
                }
                lastlen = dsize;
                std::string header = string_format("#%d: %s%s",
                                                   entnum,
                                                   standard_itype_ids[entnum].c_str(),
                                                   ( incontainer ? " (contained)" : "" )
                                                  );

                mvwprintz(menu->window, 1, startx + ( menu->pad_right - 1 - header.size() ) / 2, c_cyan, "%s",
                          header.c_str()
                         );

                for(int i = 0; i < desc.size(); i++ ) {
                    mvwprintw(menu->window, starty + i, startx, "%s", desc[i].c_str() );
                }

                mvwprintz(menu->window, menu->w_height - 3, startx, c_green, "%s", msg.c_str());
                msg = padding;
                mvwprintw(menu->window, menu->w_height - 2, startx, "[/] find, [f] container, [q]uit");
            }
        }
};

void game::wishitem( player *p, int x, int y)
{
    if ( p == NULL && x <= 0 ) {
        debugmsg("game::wishitem(): invalid parameters");
        return;
    }
    int amount = 1;
    uimenu wmenu;
    wmenu.w_width = TERMX;
    wmenu.pad_right = ( TERMX / 2 > 40 ? TERMX - 40 : TERMX / 2 );
    wmenu.return_invalid = true;
    wmenu.selected = uistate.wishitem_selected;
    wish_item_callback *cb = new wish_item_callback();
    wmenu.callback = cb;

    for (int i = 0; i <  standard_itype_ids.size(); i++) {
        itype *ity = item_controller->find_template(standard_itype_ids[i]);
        wmenu.addentry( i, true, 0, string_format("%s", ity->name.c_str()) );
        wmenu.entries[i].extratxt.txt = string_format("%c", ity->sym);
        wmenu.entries[i].extratxt.color = ity->color;
        wmenu.entries[i].extratxt.left = 1;
    }
    do {
        wmenu.query();
        if ( wmenu.ret >= 0 ) {
            amount = helper::to_int(
                         string_input_popup( _("How many?"), 20,
                                             helper::to_string( amount ),
                                             item_controller->find_template(standard_itype_ids[wmenu.ret])->name.c_str()
                                           )
                     );
            item granted = item_controller->create(standard_itype_ids[wmenu.ret], turn);
            int incontainer = dynamic_cast<wish_item_callback *>(wmenu.callback)->incontainer;
            if ( p != NULL ) {
                dynamic_cast<wish_item_callback *>(wmenu.callback)->tmp.invlet = nextinv;
                for (int i = 0; i < amount; i++) {
                    p->i_add(!incontainer ? granted : granted.in_its_container(&itypes));
                }
                advance_nextinv();
            } else if ( x >= 0 && y >= 0 ) {
                m.add_item_or_charges(x, y, granted);
            }
            dynamic_cast<wish_item_callback *>(wmenu.callback)->msg = _("Item granted, choose another or 'q' to quit.");
            uistate.wishitem_selected = wmenu.ret;
        }
    } while ( wmenu.keypress != 'q' && wmenu.keypress != KEY_ESCAPE && wmenu.keypress != ' ' );
    delete wmenu.callback;
    wmenu.callback = NULL;
    return;
}
