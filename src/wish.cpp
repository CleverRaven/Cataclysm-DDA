#include "game.h"
#include "output.h"
#include "keypress.h"
#include "item_factory.h"
#include <sstream>
#include "text_snippets.h"
#include "helper.h"
#include "uistate.h"
#include "monstergenerator.h"

#define LESS(a, b) ((a)<(b)?(a):(b))


class wish_mutate_callback: public uimenu_callback
{
    public:
        int lastlen;           // last menu entry
        std::string msg;       // feedback mesage
        bool started;
        std::vector<std::string> vTraits;
        std::map<std::string, bool> pTraits;
        player *p;
        std::string padding;

        nc_color mcolor(std::string m) {
            if ( pTraits[ m ] == true ) {
                return c_green;
            }
            return c_ltgray;
        }

        wish_mutate_callback() : msg("") {
            lastlen = 0;
            started = false;
            vTraits.clear();
            pTraits.clear();
        }
        virtual bool key(int key, int entnum, uimenu *menu) {
            if ( key == 't' && p->has_trait( vTraits[ entnum ] ) ) {
                if ( p->has_base_trait( vTraits[ entnum ] ) ) {
                    p->toggle_trait( vTraits[ entnum ] );
                    p->toggle_mutation( vTraits[ entnum ] );
                } else {
                    p->toggle_mutation( vTraits[ entnum ] );
                    p->toggle_trait( vTraits[ entnum ] );
                }
                menu->entries[ entnum ].text_color = ( p->has_trait( vTraits[ entnum ] ) ? c_green :
                                                       menu->text_color );
                menu->entries[ entnum ].extratxt.txt = ( p->has_base_trait( vTraits[ entnum ] ) ? "T" : "" );
                return true;
            }
            return false;
        }

        virtual void select(int entnum, uimenu *menu) {
            if ( ! started ) {
                started = true;
                padding = std::string(menu->pad_right - 1, ' ');
                for (std::map<std::string, trait>::iterator iter = traits.begin(); iter != traits.end(); ++iter) {
                    vTraits.push_back(iter->first);
                    pTraits[iter->first] = ( p->has_trait( iter->first ) );
                }
            }

            int startx = menu->w_width - menu->pad_right;
            for ( int i = 1; i < lastlen; i++ ) {
                mvwprintw(menu->window, i, startx, "%s", padding.c_str() );
            }

            mvwprintw(menu->window, 1, startx,
                      mutation_data[vTraits[ entnum ]].valid ? _("Valid") : _("Nonvalid"));
            int line2 = 2;

            if ( mutation_data[vTraits[ entnum ]].prereqs.size() > 0 ) {
                line2++;
                mvwprintz(menu->window, line2, startx, c_ltgray, _("Prereqs:"));
                for (int j = 0; j < mutation_data[vTraits[ entnum ]].prereqs.size(); j++) {
                    std::string mstr = mutation_data[vTraits[ entnum ]].prereqs[j];
                    mvwprintz(menu->window, line2, startx + 11, mcolor(mstr), traits[ mstr ].name.c_str());
                    line2++;
                }
            }

            if ( mutation_data[vTraits[ entnum ]].prereqs2.size() > 0 ) {
                line2++;
                mvwprintz(menu->window, line2, startx, c_ltgray, _("Prereqs, 2d:"));
                for (int j = 0; j < mutation_data[vTraits[ entnum ]].prereqs2.size(); j++) {
                    std::string mstr = mutation_data[vTraits[ entnum ]].prereqs2[j];
                    mvwprintz(menu->window, line2, startx + 15, mcolor(mstr), traits[ mstr ].name.c_str());
                    line2++;
                }
            }

            if ( mutation_data[vTraits[ entnum ]].threshreq.size() > 0 ) {
                line2++;
                mvwprintz(menu->window, line2, startx, c_ltgray, _("Thresholds required:"));
                for (int j = 0; j < mutation_data[vTraits[ entnum ]].threshreq.size(); j++) {
                    std::string mstr = mutation_data[vTraits[ entnum ]].threshreq[j];
                    mvwprintz(menu->window, line2, startx + 21, mcolor(mstr), traits[ mstr ].name.c_str());
                    line2++;
                }
            }

            if ( mutation_data[vTraits[ entnum ]].cancels.size() > 0 ) {
                line2++;
                mvwprintz(menu->window, line2, startx, c_ltgray, _("Cancels:"));
                for (int j = 0; j < mutation_data[vTraits[ entnum ]].cancels.size(); j++) {
                    std::string mstr = mutation_data[vTraits[ entnum ]].cancels[j];
                    mvwprintz(menu->window, line2, startx + 11, mcolor(mstr), traits[ mstr ].name.c_str());
                    line2++;
                }
            }

            if ( mutation_data[vTraits[ entnum ]].replacements.size() > 0 ) {
                line2++;
                mvwprintz(menu->window, line2, startx, c_ltgray, _("Becomes:"));
                for (int j = 0; j < mutation_data[vTraits[ entnum ]].replacements.size(); j++) {
                    std::string mstr = mutation_data[vTraits[ entnum ]].replacements[j];
                    mvwprintz(menu->window, line2, startx + 11, mcolor(mstr), traits[ mstr ].name.c_str());
                    line2++;
                }
            }

            if ( mutation_data[vTraits[ entnum ]].additions.size() > 0 ) {
                line2++;
                mvwprintz(menu->window, line2, startx, c_ltgray, _("Add-ons:"));
                for (int j = 0; j < mutation_data[vTraits[ entnum ]].additions.size(); j++) {
                    std::string mstr = mutation_data[vTraits[ entnum ]].additions[j];
                    mvwprintz(menu->window, line2, startx + 11, mcolor(mstr), traits[ mstr ].name.c_str());
                    line2++;
                }
            }

            if ( mutation_data[vTraits[ entnum ]].category.size() > 0 ) {
                line2++;
                mvwprintz(menu->window, line2, startx, c_ltgray,  _("Category:"));
                for (int j = 0; j < mutation_data[vTraits[ entnum ]].category.size(); j++) {
                    mvwprintw(menu->window, line2, startx + 11, mutation_data[vTraits[ entnum ]].category[j].c_str());
                    line2++;
                }
            }
            line2 += 2;

            mvwprintz(menu->window, line2, startx, c_ltgray, "pts: %d vis: %d ugly: %d",
                      traits[vTraits[ entnum ]].points,
                      traits[vTraits[ entnum ]].visibility,
                      traits[vTraits[ entnum ]].ugliness
                     );
            line2 += 2;

            std::vector<std::string> desc = foldstring( traits[vTraits[ entnum ]].description,
                                            menu->pad_right - 1 );
            for( int j = 0; j < desc.size(); j++ ) {
                mvwprintz(menu->window, line2, startx, c_ltgray, "%s", desc[j].c_str() );
                line2++;
            }
            lastlen = line2 + 1;

            mvwprintz(menu->window, menu->w_height - 3, startx, c_green, "%s", msg.c_str());
            msg = padding;
            mvwprintw(menu->window, menu->w_height - 2, startx, _("[/] find, [q]uit"));

        };

        ~wish_mutate_callback() {};
};


void game::wishmutate( player *p )
{
    uimenu wmenu;
    int c = 0;

    for (std::map<std::string, trait>::iterator iter = traits.begin(); iter != traits.end(); ++iter) {
        wmenu.addentry(-1, true, -2, "%s", iter->second.name.c_str() );
        wmenu.entries[ c ].extratxt.left = 1;
        wmenu.entries[ c ].extratxt.txt = "";
        wmenu.entries[ c ].extratxt.color = c_ltgreen;
        if( p->has_trait( iter->first ) ) {
            wmenu.entries[ c ].text_color = c_green;
            if ( p->has_base_trait( iter->first ) ) {
                wmenu.entries[ c ].extratxt.txt = "T";
            }
        }
        c++;
    }
    wmenu.w_x = 0;
    wmenu.w_width = TERMX;
    // disabled due to foldstring crash // ( TERMX - getmaxx(w_terrain) - 30 > 24 ? getmaxx(w_terrain) : TERMX );
    wmenu.pad_right = ( wmenu.w_width - 30 );
    wmenu.return_invalid = true;
    wmenu.selected = uistate.wishmutate_selected;
    wish_mutate_callback *cb = new wish_mutate_callback();
    cb->p = p;
    wmenu.callback = cb;
    do {
        wmenu.query();
        if ( wmenu.ret > 0 ) {
            int rc = 0;
            std::string mstr = cb->vTraits[ wmenu.ret ];
            bool threshold = mutation_data[mstr].threshold;
            //Manual override for the threshold-gaining
            if (threshold) {
                if ( p->has_trait( mstr ) ) {
                    do {
                        p->remove_mutation(mstr );
                        rc++;
                    } while (p->has_trait( mstr ) && rc < 10);
                } else {
                    do {
                        p->toggle_mutation(mstr );
                        rc++;
                    } while (!p->has_trait( mstr ) && rc < 10);
                }
            } else if ( p->has_trait( mstr ) ) {
                do {
                    p->remove_mutation(mstr );
                    rc++;
                } while (p->has_trait( mstr ) && rc < 10);
            } else {
                do {
                    p->mutate_towards(mstr );
                    rc++;
                } while (!p->has_trait( mstr ) && rc < 10);
            }
            cb->msg = string_format(_("%s Mutation changes: %d"), mstr.c_str(), rc);
            uistate.wishmutate_selected = wmenu.ret;
            if ( rc != 0 ) {
                for ( int i = 0; i < cb->vTraits.size(); i++ ) {
                    wmenu.entries[ i ].extratxt.txt = "";
                    if ( p->has_trait( cb->vTraits[ i ] ) ) {
                        wmenu.entries[ i ].text_color = c_green;
                        cb->pTraits[ cb->vTraits[ i ] ] = true;
                        if ( p->has_base_trait( cb->vTraits[ i ] ) ) {
                            wmenu.entries[ i ].extratxt.txt = "T";
                        }
                    } else {
                        wmenu.entries[ i ].text_color = wmenu.text_color;
                        cb->pTraits[ cb->vTraits[ i ] ] = false;
                    }
                }
            }
        }
    } while ( wmenu.keypress != 'q' && wmenu.keypress != KEY_ESCAPE && wmenu.keypress != ' ' );
    delete cb;
    cb = NULL;
    return;

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

        wish_monster_callback() : msg(""), padding("") {
            started = false;
            friendly = false;
            lastent = -2;
            w_info = NULL;
        }

        void setup(uimenu *menu) {
            w_info = newwin(menu->w_height - 2, menu->pad_right, 1,
                            menu->w_x + menu->w_width - 2 - menu->pad_right);
            padding = std::string( getmaxx(w_info), ' ' );
            werase(w_info);
            wrefresh(w_info);
        }

        virtual bool key(int key, int entnum, uimenu *menu) {
            (void)entnum; // unused
            (void)menu;   // unused
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
                tmp = monster(GetMType(entnum));
                if (friendly) {
                    tmp.friendly = -1;
                }
            }

            werase(w_info);
            tmp.print_info(w_info);

            std::string header = string_format("#%d: %s", entnum, GetMType(entnum)->name.c_str());
            mvwprintz(w_info, 1, ( getmaxx(w_info) - header.size() ) / 2, c_cyan, "%s",
                      header.c_str());

            mvwprintz(w_info, getmaxy(w_info) - 3, 0, c_green, "%s", msg.c_str());
            msg = padding;
            mvwprintw(w_info, getmaxy(w_info) - 2, 0, _("[/] find, [f] friendly, [q]uit"));
        }

        virtual void refresh(uimenu *menu) {
            (void)menu; // unused
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
    const std::map<std::string, mtype *> montypes = MonsterGenerator::generator().get_all_mtypes();

    uimenu wmenu;
    wmenu.w_x = 0;
    wmenu.w_width = TERMX;
    // disabled due to foldstring crash //( TERMX - getmaxx(w_terrain) - 30 > 24 ? getmaxx(w_terrain) : TERMX );
    wmenu.pad_right = ( wmenu.w_width - 30 );
    wmenu.return_invalid = true;
    wmenu.selected = uistate.wishmonster_selected;
    wish_monster_callback *cb = new wish_monster_callback();
    wmenu.callback = cb;

    int i = 0;
    for (std::map<std::string, mtype *>::const_iterator mon = montypes.begin();
         mon != montypes.end(); ++mon) {
        wmenu.addentry( i, true, 0, "%s", mon->second->name.c_str() );
        wmenu.entries[i].extratxt.txt = string_format("%c", mon->second->sym);
        wmenu.entries[i].extratxt.color = mon->second->color;
        wmenu.entries[i].extratxt.left = 1;
        ++i;
    }

    do {
        wmenu.query();
        if ( wmenu.ret >= 0 ) {
            monster mon = monster(GetMType(wmenu.ret));
            if (cb->friendly) {
                mon.friendly = -1;
            }
            point spawn = ( x == -1 && y == -1 ? look_around() : point ( x, y ) );
            if (spawn.x != -1) {
                mon.spawn(spawn.x, spawn.y);
                add_zombie(mon);
                cb->msg = _("Monster spawned, choose another or 'q' to quit.");
                uistate.wishmonster_selected = wmenu.ret;
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
        wish_item_callback() : msg("") {
            lastlen = 0;
            incontainer = false;
        }
        virtual bool key(int key, int entnum, uimenu *menu) {
            (void)entnum; // unused
            (void)menu;   // unused
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
                std::string header = string_format("#%d: %s%s", entnum,
                                                   standard_itype_ids[entnum].c_str(),
                                                   ( incontainer ? _(" (contained)") : "" ));

                mvwprintz(menu->window, 1, startx + ( menu->pad_right - 1 - header.size() ) / 2,
                          c_cyan, _("%s"), header.c_str());

                for(int i = 0; i < desc.size(); i++ ) {
                    mvwprintw(menu->window, starty + i, startx, "%s", desc[i].c_str() );
                }

                mvwprintz(menu->window, menu->w_height - 3, startx, c_green, "%s", msg.c_str());
                msg = padding;
                mvwprintw(menu->window, menu->w_height - 2, startx, _("[/] find, [f] container, [q]uit"));
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
    wmenu.w_x = 0;
    wmenu.w_width = TERMX;
    wmenu.pad_right = ( TERMX / 2 > 40 ? TERMX - 40 : TERMX / 2 );
    wmenu.return_invalid = true;
    wmenu.selected = uistate.wishitem_selected;
    wish_item_callback *cb = new wish_item_callback();
    wmenu.callback = cb;

    for (int i = 0; i < standard_itype_ids.size(); i++) {
        itype *ity = item_controller->find_template(standard_itype_ids[i]);
        wmenu.addentry( i, true, 0, string_format(_("%s"), ity->name.c_str()) ); //%s ity->category
        wmenu.entries[i].extratxt.txt = string_format("%c", ity->sym);
        wmenu.entries[i].extratxt.color = ity->color;
        wmenu.entries[i].extratxt.left = 1;
    }
    do {
        wmenu.query();
        if ( wmenu.ret >= 0 ) {
            amount = helper::to_int(
                         string_input_popup( _("How many?"), 20, helper::to_string( amount ),
                                             item_controller->find_template(standard_itype_ids[wmenu.ret])->name.c_str(),
                                             "", 5, true));
            item granted = item_controller->create(standard_itype_ids[wmenu.ret], turn);
            int incontainer = dynamic_cast<wish_item_callback *>(wmenu.callback)->incontainer;
            if ( p != NULL ) {
                dynamic_cast<wish_item_callback *>(wmenu.callback)->tmp.invlet = nextinv;
                for (int i = 0; i < amount; i++) {
                    p->i_add(!incontainer ? granted : granted.in_its_container(&itypes));
                }
                advance_nextinv();
            } else if ( x >= 0 && y >= 0 ) {
                m.add_item_or_charges(x, y, !incontainer ? granted : granted.in_its_container(&itypes));
                wmenu.keypress = 'q';
            }
            dynamic_cast<wish_item_callback *>(wmenu.callback)->msg =
                _("Item granted, choose another or 'q' to quit.");
            uistate.wishitem_selected = wmenu.ret;
        }
    } while ( wmenu.keypress != 'q' && wmenu.keypress != KEY_ESCAPE && wmenu.keypress != ' ' );
    delete wmenu.callback;
    wmenu.callback = NULL;
    return;
}

/*
 * Set skill on any player object; player character or NPC
 */
void game::wishskill(player *p)
{

    const int skoffset = 1;
    uimenu skmenu;
    skmenu.text = _("Select a skill to modify");
    skmenu.return_invalid = true;
    skmenu.addentry(0, true, '1', _("Set all skills to..."));
    int *origskills = new int[Skill::skills.size()] ;

    for (std::vector<Skill *>::iterator aSkill = Skill::skills.begin();
         aSkill != Skill::skills.end(); ++aSkill) {
        int skill_id = (*aSkill)->id();
        skmenu.addentry( skill_id + skoffset, true, -2, _("@ %d: %s  "),
                         (int)p->skillLevel(*aSkill), (*aSkill)->ident().c_str() );
        origskills[skill_id] = (int)p->skillLevel(*aSkill);
    }
    do {
        skmenu.query();
        int skill_id = -1;
        int skset = -1;
        int sksel = skmenu.selected - skoffset;
        if ( skmenu.ret == -1 && ( skmenu.keypress == KEY_LEFT || skmenu.keypress == KEY_RIGHT ) ) {
            if ( sksel >= 0 && sksel < Skill::skills.size() ) {
                skill_id = sksel;
                skset = (int)p->skillLevel( Skill::skills[skill_id]) +
                        ( skmenu.keypress == KEY_LEFT ? -1 : 1 );
            }
            skmenu.ret = -2;
        } else if ( skmenu.selected == skmenu.ret &&  sksel >= 0 && sksel < Skill::skills.size() ) {
            skill_id = sksel;
            uimenu sksetmenu;
            sksetmenu.w_x = skmenu.w_x + skmenu.w_width + 1;
            sksetmenu.w_y = skmenu.w_y + 2;
            sksetmenu.w_height = skmenu.w_height - 4;
            sksetmenu.return_invalid = true;
            sksetmenu.settext( _("Set '%s' to.."), Skill::skills[skill_id]->ident().c_str() );
            int skcur = (int)p->skillLevel(Skill::skills[skill_id]);
            sksetmenu.selected = skcur;
            for ( int i = 0; i < 21; i++ ) {
                sksetmenu.addentry( i, true, i + 48, "%d%s", i, (skcur == i ? _(" (current)") : "") );
            }
            sksetmenu.query();
            skset = sksetmenu.ret;
        }

        if ( skset != UIMENU_INVALID && skset != -1 && skill_id != -1 ) {
            p->skillLevel( Skill::skills[skill_id] ).level(skset);
            skmenu.textformatted[0] = string_format(_("%s set to %d             "),
                                                    Skill::skills[skill_id]->ident().c_str(),
                                                    (int)p->skillLevel(Skill::skills[skill_id])).substr(0, skmenu.w_width - 4);
            skmenu.entries[skill_id + skoffset].txt = string_format(_("@ %d: %s  "),
                    (int)p->skillLevel( Skill::skills[skill_id]), Skill::skills[skill_id]->ident().c_str() );
            skmenu.entries[skill_id + skoffset].text_color =
                ( (int)p->skillLevel(Skill::skills[skill_id]) == origskills[skill_id] ?
                  skmenu.text_color : c_yellow );
        } else if ( skmenu.ret == 0 && sksel == -1 ) {
            int ret = menu(true, _("Set all skills..."), "+3", "+1", "-1", "-3", _("To 0"),
                           _("To 5"), _("To 10"), _("(Reset changes)"), NULL);
            if ( ret > 0 ) {
                int skmod = 0;
                int skset = -1;
                if (ret < 5 ) {
                    skmod = ( ret < 3 ? ( ret == 1 ? 3 : 1 ) : ( ret == 3 ? -1 : -3 ));
                } else if ( ret < 8 ) {
                    skset = ( ( ret - 5 ) * 5 );
                }
                for (int skill_id = 0; skill_id < Skill::skills.size(); skill_id++ ) {
                    int changeto = ( skmod != 0 ? p->skillLevel( Skill::skills[skill_id] ) + skmod :
                                     ( skset != -1 ? skset : origskills[skill_id] ) );
                    p->skillLevel( Skill::skills[skill_id] ).level( changeto );
                    skmenu.entries[skill_id + skoffset].txt = string_format(_("@ %d: %s  "),
                            (int)p->skillLevel(Skill::skills[skill_id]),
                            Skill::skills[skill_id]->ident().c_str() );
                    skmenu.entries[skill_id + skoffset].text_color =
                        ( (int)p->skillLevel(Skill::skills[skill_id]) == origskills[skill_id] ?
                          skmenu.text_color : c_yellow );
                }
            }
        }
    } while ( skmenu.ret != -1 && ( skmenu.keypress != 'q' && skmenu.keypress != ' ' &&
                                    skmenu.keypress != KEY_ESCAPE ) );
    delete[] origskills;
}
