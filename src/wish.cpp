#include "game.h"
#include "output.h"
#include "item_factory.h"
#include "uistate.h"
#include "monstergenerator.h"
#include "compatibility.h"

#include <sstream>

#define LESS(a, b) ((a)<(b)?(a):(b))


class wish_mutate_callback: public uimenu_callback
{
    public:
        int lastlen;           // last menu entry
        std::string msg;       // feedback message
        bool started;
        std::vector<std::string> vTraits;
        std::map<std::string, bool> pTraits;
        player *p;
        std::string padding;

        nc_color mcolor(std::string m)
        {
            if ( pTraits[ m ] == true ) {
                return c_green;
            }
            return c_ltgray;
        }

        wish_mutate_callback() : msg("")
        {
            lastlen = 0;
            started = false;
            vTraits.clear();
            pTraits.clear();
        }
        virtual bool key(int key, int entnum, uimenu *menu)
        {
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

        virtual void select(int entnum, uimenu *menu)
        {
            if ( ! started ) {
                started = true;
                padding = std::string(menu->pad_right - 1, ' ');
                for( auto &traits_iter : traits ) {
                    vTraits.push_back( traits_iter.first );
                    pTraits[traits_iter.first] = ( p->has_trait( traits_iter.first ) );
                }
            }

            int startx = menu->w_width - menu->pad_right;
            for ( int i = 1; i < lastlen; i++ ) {
                mvwprintw(menu->window, i, startx, "%s", padding.c_str() );
            }

            mvwprintw(menu->window, 1, startx,
                      mutation_data[vTraits[ entnum ]].valid ? _("Valid") : _("Nonvalid"));
            int line2 = 2;

            if ( !mutation_data[vTraits[entnum]].prereqs.empty() ) {
                line2++;
                mvwprintz(menu->window, line2, startx, c_ltgray, _("Prereqs:"));
                for (auto &j : mutation_data[vTraits[ entnum ]].prereqs) {
                    mvwprintz(menu->window, line2, startx + 11, mcolor(j), "%s", traits[j].name.c_str());
                    line2++;
                }
            }

            if ( !mutation_data[vTraits[entnum]].prereqs2.empty() ) {
                line2++;
                mvwprintz(menu->window, line2, startx, c_ltgray, _("Prereqs, 2d:"));
                for (auto &j : mutation_data[vTraits[ entnum ]].prereqs2) {
                    mvwprintz(menu->window, line2, startx + 15, mcolor(j), "%s", traits[j].name.c_str());
                    line2++;
                }
            }

            if ( !mutation_data[vTraits[entnum]].threshreq.empty() ) {
                line2++;
                mvwprintz(menu->window, line2, startx, c_ltgray, _("Thresholds required:"));
                for (auto &j : mutation_data[vTraits[ entnum ]].threshreq) {
                    mvwprintz(menu->window, line2, startx + 21, mcolor(j), "%s", traits[j].name.c_str());
                    line2++;
                }
            }

            if ( !mutation_data[vTraits[entnum]].cancels.empty() ) {
                line2++;
                mvwprintz(menu->window, line2, startx, c_ltgray, _("Cancels:"));
                for (auto &j : mutation_data[vTraits[ entnum ]].cancels) {
                    mvwprintz(menu->window, line2, startx + 11, mcolor(j), "%s", traits[j].name.c_str());
                    line2++;
                }
            }

            if ( !mutation_data[vTraits[entnum]].replacements.empty() ) {
                line2++;
                mvwprintz(menu->window, line2, startx, c_ltgray, _("Becomes:"));
                for (auto &j : mutation_data[vTraits[ entnum ]].replacements) {
                    mvwprintz(menu->window, line2, startx + 11, mcolor(j), "%s", traits[j].name.c_str());
                    line2++;
                }
            }

            if ( !mutation_data[vTraits[entnum]].additions.empty() ) {
                line2++;
                mvwprintz(menu->window, line2, startx, c_ltgray, _("Add-ons:"));
                for (auto &j : mutation_data[vTraits[ entnum ]].additions) {
                    mvwprintz(menu->window, line2, startx + 11, mcolor(j), "%s", traits[j].name.c_str());
                    line2++;
                }
            }

            if ( !mutation_data[vTraits[entnum]].category.empty() ) {
                line2++;
                mvwprintz(menu->window, line2, startx, c_ltgray,  _("Category:"));
                for (auto &j : mutation_data[vTraits[ entnum ]].category) {
                    mvwprintw(menu->window, line2, startx + 11, "%s", j.c_str());
                    line2++;
                }
            }
            line2 += 2;

            mvwprintz(menu->window, line2, startx, c_ltgray, "pts: %d vis: %d ugly: %d",
                      traits[vTraits[entnum]].points,
                      traits[vTraits[entnum]].visibility,
                      traits[vTraits[entnum]].ugliness
                     );
            line2 += 2;

            std::vector<std::string> desc = foldstring( traits[vTraits[ entnum ]].description,
                                            menu->pad_right - 1 );
            for( auto &elem : desc ) {
                mvwprintz( menu->window, line2, startx, c_ltgray, "%s", elem.c_str() );
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

    for( auto &traits_iter : traits ) {
        wmenu.addentry( -1, true, -2, "%s", traits_iter.second.name.c_str() );
        wmenu.entries[ c ].extratxt.left = 1;
        wmenu.entries[ c ].extratxt.txt = "";
        wmenu.entries[ c ].extratxt.color = c_ltgreen;
        if( p->has_trait( traits_iter.first ) ) {
            wmenu.entries[ c ].text_color = c_green;
            if( p->has_base_trait( traits_iter.first ) ) {
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
            bool profession = mutation_data[mstr].profession;
            //Manual override for the threshold-gaining
            if (threshold || profession) {
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
                for ( size_t i = 0; i < cb->vTraits.size(); i++ ) {
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
        std::string msg;       // feedback message
        bool friendly;         // spawn friendly critter?
        bool hallucination;
        int group;             // Number of monsters to spawn.
        WINDOW *w_info;        // ui_parent menu's padding area
        monster tmp;           // scrap critter for monster::print_info
        bool started;          // if unset, initialize window
        std::string padding;   // ' ' x window width

        wish_monster_callback() : msg(""), padding("")
        {
            started = false;
            friendly = false;
            hallucination = false;
            group = 0;
            lastent = -2;
            w_info = NULL;
        }

        void setup(uimenu *menu)
        {
            w_info = newwin(menu->w_height - 2, menu->pad_right, 1,
                            menu->w_x + menu->w_width - 1 - menu->pad_right);
            padding = std::string( getmaxx(w_info), ' ' );
            werase(w_info);
            wrefresh(w_info);
        }

        virtual bool key(int key, int entnum, uimenu *menu)
        {
            (void)entnum; // unused
            (void)menu;   // unused
            if ( key == 'f' ) {
                friendly = !friendly;
                lastent = -2; // force tmp monster regen
                return true;  // tell menu we handled keypress
            } else if( key == 'i' ) {
                group++;
                return true;
            } else if( key == 'h' ) {
                hallucination = !hallucination;
                return true;
            } else if( key == 'd' ) {
                group = std::max( 1, group - 1 );
                return true;
            }
            return false;
        }

        virtual void select(int entnum, uimenu *menu)
        {
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
            tmp.print_info( w_info, 6, 5, 1 );

            std::string header = string_format("#%d: %s", entnum, GetMType(entnum)->nname().c_str());
            mvwprintz(w_info, 1, ( getmaxx(w_info) - header.size() ) / 2, c_cyan, "%s",
                      header.c_str());
            if( hallucination ) {
                wprintw( w_info, _( " (hallucination)" ) );
            }

            mvwprintz(w_info, getmaxy(w_info) - 3, 0, c_green, "%s", msg.c_str());
            msg = padding;
            mvwprintw(w_info, getmaxy(w_info) - 2, 0,
                      _("[/] find, [f]riendly, [h]allucination [i]ncrease group, [d]ecrease group, [q]uit"));
        }

        virtual void refresh(uimenu *menu)
        {
            (void)menu; // unused
            wrefresh(w_info);
        }

        ~wish_monster_callback()
        {
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
    for( const auto &montype : montypes ) {
        wmenu.addentry( i, true, 0, "%s", montype.second->nname().c_str() );
        wmenu.entries[i].extratxt.txt = montype.second->sym;
        wmenu.entries[i].extratxt.color = montype.second->color;
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
            if (cb->hallucination) {
                mon.hallucination = true;
            }
            point spawn = ( x == -1 && y == -1 ? look_around() : point ( x, y ) );
            if (spawn.x != -1) {
                std::vector<point> spawn_points = closest_points_first( cb->group, spawn );
                for( auto spawn_point : spawn_points ) {
                    mon.spawn(spawn_point.x, spawn_point.y);
                    add_zombie(mon);
                }
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
        bool incontainer;
        std::string msg;
        const std::vector<std::string> &standard_itype_ids;
        wish_item_callback(const std::vector<std::string> &ids) : incontainer(false), msg("")
            , standard_itype_ids( ids )
        {
        }
        virtual bool key(int key, int /*entnum*/, uimenu * /*menu*/)
        {
            if ( key == 'f' ) {
                incontainer = !incontainer;
                return true;
            }
            return false;
        }

        virtual void select(int entnum, uimenu *menu)
        {
            const int starty = 3;
            const int startx = menu->w_width - menu->pad_right;
            const std::string padding(menu->pad_right, ' ');
            for(int y = 2; y < menu->w_height - 1; y++) {
                mvwprintw(menu->window, y, startx - 1, "%s", padding.c_str());
            }
            item tmp(standard_itype_ids[entnum], calendar::turn);
            const std::string header = string_format("#%d: %s%s", entnum, standard_itype_ids[entnum].c_str(),
                                       ( incontainer ? _(" (contained)") : "" ));
            mvwprintz(menu->window, 1, startx + ( menu->pad_right - 1 - header.size() ) / 2, c_cyan, "%s",
                      header.c_str());

            fold_and_print(menu->window, starty, startx, menu->pad_right - 1, c_white, tmp.info(true));

            mvwprintz(menu->window, menu->w_height - 3, startx, c_green, "%s", msg.c_str());
            msg.erase();

            mvwprintw(menu->window, menu->w_height - 2, startx, _("[/] find, [f] container, [q]uit"));
        }
};

void game::wishitem( player *p, int x, int y)
{
    if ( p == NULL && x <= 0 ) {
        debugmsg("game::wishitem(): invalid parameters");
        return;
    }
    const std::vector<std::string> standard_itype_ids = item_controller->get_all_itype_ids();
    int amount = 1;
    uimenu wmenu;
    wmenu.w_x = 0;
    wmenu.w_width = TERMX;
    wmenu.pad_right = ( TERMX / 2 > 40 ? TERMX - 40 : TERMX / 2 );
    wmenu.return_invalid = true;
    wmenu.selected = uistate.wishitem_selected;
    wish_item_callback *cb = new wish_item_callback( standard_itype_ids );
    wmenu.callback = cb;

    for (size_t i = 0; i < standard_itype_ids.size(); i++) {
        item ity( standard_itype_ids[i], 0 );
        wmenu.addentry( i, true, 0, string_format(_("%s"), ity.tname(1).c_str()) );
        wmenu.entries[i].extratxt.txt = string_format("%c", ity.symbol());
        wmenu.entries[i].extratxt.color = ity.color();
        wmenu.entries[i].extratxt.left = 1;
    }
    do {
        wmenu.query();
        if ( wmenu.ret >= 0 ) {
            item granted(standard_itype_ids[wmenu.ret], calendar::turn);
            if (p != NULL) {
                amount = std::stoi(
                             string_input_popup(_("How many?"), 20, to_string( amount ),
                                                granted.tname()));
            }
            if (dynamic_cast<wish_item_callback *>(wmenu.callback)->incontainer) {
                granted = granted.in_its_container();
            }
            if ( p != NULL ) {
                for (int i = 0; i < amount; i++) {
                    p->i_add(granted);
                }
                p->invalidate_crafting_inventory();
            } else if ( x >= 0 && y >= 0 ) {
                m.add_item_or_charges(x, y, granted);
                wmenu.keypress = 'q';
            }
            dynamic_cast<wish_item_callback *>(wmenu.callback)->msg =
                _("Wish granted. Wish for more or hit 'q' to quit.");
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

    for( auto &skill : Skill::skills ) {
        int skill_id = ( skill )->id();
        skmenu.addentry( skill_id + skoffset, true, -2, _( "@ %d: %s  " ),
                         (int)p->skillLevel( skill ), ( skill )->name().c_str() );
        origskills[skill_id] = (int)p->skillLevel( skill );
    }
    do {
        skmenu.query();
        int skill_id = -1;
        int skset = -1;
        int sksel = skmenu.selected - skoffset;
        if ( skmenu.ret == -1 && ( skmenu.keypress == KEY_LEFT || skmenu.keypress == KEY_RIGHT ) ) {
            if ( sksel >= 0 && sksel < (int)Skill::skills.size() ) {
                skill_id = sksel;
                skset = (int)p->skillLevel( Skill::skills[skill_id]) +
                        ( skmenu.keypress == KEY_LEFT ? -1 : 1 );
            }
            skmenu.ret = -2;
        } else if ( skmenu.selected == skmenu.ret &&  sksel >= 0 && sksel < (int)Skill::skills.size() ) {
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
            int ret = menu(true, _("Alter all skill values"), _("Add 3"), _("Add 1"), _("Subtract 1"),
                           _("Subtract 3"), _("set to 0"),
                           _("Set to 5"), _("Set to 10"), _("(Reset changes)"), NULL);
            if ( ret > 0 ) {
                int skmod = 0;
                int skset = -1;
                if (ret < 5 ) {
                    skmod = ( ret < 3 ? ( ret == 1 ? 3 : 1 ) : ( ret == 3 ? -1 : -3 ));
                } else if ( ret < 8 ) {
                    skset = ( ( ret - 5 ) * 5 );
                }
                for (size_t skill_id = 0; skill_id < Skill::skills.size(); skill_id++ ) {
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
