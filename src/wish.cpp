#include "debug_menu.h" // IWYU pragma: associated

#include "debug.h"
#include "game.h"
#include "input.h"
#include "item_factory.h"
#include "map.h"
#include "monster.h"
#include "monstergenerator.h"
#include "mtype.h"
#include "mutation.h"
#include "output.h"
#include "player.h"
#include "skill.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "ui.h"
#include "uistate.h"

class wish_mutate_callback: public uilist_callback
{
    public:
        int lastlen = 0;       // last menu entry
        std::string msg;       // feedback message
        bool started = false;
        std::vector<trait_id> vTraits;
        std::map<trait_id, bool> pTraits;
        player *p;
        std::string padding;

        nc_color mcolor( const trait_id &m ) {
            if( pTraits[ m ] ) {
                return c_green;
            }
            return c_light_gray;
        }

        wish_mutate_callback() = default;
        bool key( const input_context &, const input_event &event, int entnum, uilist *menu ) override {
            if( event.get_first_input() == 't' && p->has_trait( vTraits[ entnum ] ) ) {
                if( p->has_base_trait( vTraits[ entnum ] ) ) {
                    p->toggle_trait( vTraits[ entnum ] );
                    p->unset_mutation( vTraits[ entnum ] );

                } else {
                    p->set_mutation( vTraits[ entnum ] );
                    p->toggle_trait( vTraits[ entnum ] );
                }
                menu->entries[ entnum ].text_color = ( p->has_trait( vTraits[ entnum ] ) ? c_green :
                                                       menu->text_color );
                menu->entries[ entnum ].extratxt.txt = ( p->has_base_trait( vTraits[ entnum ] ) ? "T" : "" );
                return true;
            }
            return false;
        }

        void select( int entnum, uilist *menu ) override {
            if( ! started ) {
                started = true;
                padding = std::string( menu->pad_right - 1, ' ' );
                for( auto &traits_iter : mutation_branch::get_all() ) {
                    vTraits.push_back( traits_iter.id );
                    pTraits[traits_iter.id] = ( p->has_trait( traits_iter.id ) );
                }
            }
            const mutation_branch &mdata = vTraits[entnum].obj();

            int startx = menu->w_width - menu->pad_right;
            for( int i = 2; i < lastlen; i++ ) {
                mvwprintw( menu->window, i, startx, padding );
            }

            mvwprintw( menu->window, 3, startx,
                       mdata.valid ? _( "Valid" ) : _( "Nonvalid" ) );
            int line2 = 4;

            if( !mdata.prereqs.empty() ) {
                line2++;
                mvwprintz( menu->window, line2, startx, c_light_gray, _( "Prereqs:" ) );
                for( auto &j : mdata.prereqs ) {
                    mvwprintz( menu->window, line2, startx + 11, mcolor( j ),
                               mutation_branch::get_name( j ) );
                    line2++;
                }
            }

            if( !mdata.prereqs2.empty() ) {
                line2++;
                mvwprintz( menu->window, line2, startx, c_light_gray, _( "Prereqs, 2d:" ) );
                for( auto &j : mdata.prereqs2 ) {
                    mvwprintz( menu->window, line2, startx + 15, mcolor( j ),
                               mutation_branch::get_name( j ) );
                    line2++;
                }
            }

            if( !mdata.threshreq.empty() ) {
                line2++;
                mvwprintz( menu->window, line2, startx, c_light_gray, _( "Thresholds required:" ) );
                for( auto &j : mdata.threshreq ) {
                    mvwprintz( menu->window, line2, startx + 21, mcolor( j ),
                               mutation_branch::get_name( j ) );
                    line2++;
                }
            }

            if( !mdata.cancels.empty() ) {
                line2++;
                mvwprintz( menu->window, line2, startx, c_light_gray, _( "Cancels:" ) );
                for( auto &j : mdata.cancels ) {
                    mvwprintz( menu->window, line2, startx + 11, mcolor( j ),
                               mutation_branch::get_name( j ) );
                    line2++;
                }
            }

            if( !mdata.replacements.empty() ) {
                line2++;
                mvwprintz( menu->window, line2, startx, c_light_gray, _( "Becomes:" ) );
                for( auto &j : mdata.replacements ) {
                    mvwprintz( menu->window, line2, startx + 11, mcolor( j ),
                               mutation_branch::get_name( j ) );
                    line2++;
                }
            }

            if( !mdata.additions.empty() ) {
                line2++;
                mvwprintz( menu->window, line2, startx, c_light_gray, _( "Add-ons:" ) );
                for( auto &j : mdata.additions ) {
                    mvwprintz( menu->window, line2, startx + 11, mcolor( j ),
                               mutation_branch::get_name( j ) );
                    line2++;
                }
            }

            if( !mdata.types.empty() ) {
                line2++;
                mvwprintz( menu->window, line2, startx, c_light_gray,  _( "Type:" ) );
                for( auto &j : mdata.types ) {
                    mvwprintw( menu->window, line2, startx + 11, j );
                    line2++;
                }
            }

            if( !mdata.category.empty() ) {
                line2++;
                mvwprintz( menu->window, line2, startx, c_light_gray,  _( "Category:" ) );
                for( auto &j : mdata.category ) {
                    mvwprintw( menu->window, line2, startx + 11, j );
                    line2++;
                }
            }
            line2 += 2;

            //~ pts: points, vis: visibility, ugly: ugliness
            mvwprintz( menu->window, line2, startx, c_light_gray, _( "pts: %d vis: %d ugly: %d" ),
                       mdata.points,
                       mdata.visibility,
                       mdata.ugliness
                     );
            line2 += 2;

            std::vector<std::string> desc = foldstring( mdata.desc(),
                                            menu->pad_right - 1 );
            for( auto &elem : desc ) {
                mvwprintz( menu->window, line2, startx, c_light_gray, elem );
                line2++;
            }
            lastlen = line2 + 1;

            mvwprintz( menu->window, menu->w_height - 3, startx, c_green, msg );
            msg = padding;
            input_context ctxt( menu->input_category );
            mvwprintw( menu->window, menu->w_height - 2, startx,
                       _( "[%s] find, [%s] quit, [t] toggle base trait" ),
                       ctxt.get_desc( "FILTER" ).c_str(), ctxt.get_desc( "QUIT" ).c_str() );

        }

        ~wish_mutate_callback() override = default;
};

void debug_menu::wishmutate( player *p )
{
    uilist wmenu;
    int c = 0;

    for( auto &traits_iter : mutation_branch::get_all() ) {
        wmenu.addentry( -1, true, -2, traits_iter.name() );
        wmenu.entries[ c ].extratxt.left = 1;
        wmenu.entries[ c ].extratxt.txt.clear();
        wmenu.entries[ c ].extratxt.color = c_light_green;
        if( p->has_trait( traits_iter.id ) ) {
            wmenu.entries[ c ].text_color = c_green;
            if( p->has_base_trait( traits_iter.id ) ) {
                wmenu.entries[ c ].extratxt.txt = "T";
            }
        }
        c++;
    }
    wmenu.w_x = 0;
    wmenu.w_width = TERMX;
    // disabled due to foldstring crash // ( TERMX - getmaxx(w_terrain) - 30 > 24 ? getmaxx(w_terrain) : TERMX );
    wmenu.pad_right = ( wmenu.w_width - 40 );
    wmenu.selected = uistate.wishmutate_selected;
    wish_mutate_callback cb;
    cb.p = p;
    wmenu.callback = &cb;
    do {
        wmenu.query();
        if( wmenu.ret >= 0 ) {
            int rc = 0;
            const trait_id mstr = cb.vTraits[ wmenu.ret ];
            const auto &mdata = mstr.obj();
            bool threshold = mdata.threshold;
            bool profession = mdata.profession;
            //Manual override for the threshold-gaining
            if( threshold || profession ) {
                if( p->has_trait( mstr ) ) {
                    do {
                        p->remove_mutation( mstr );
                        rc++;
                    } while( p->has_trait( mstr ) && rc < 10 );
                } else {
                    do {
                        p->set_mutation( mstr );
                        rc++;
                    } while( !p->has_trait( mstr ) && rc < 10 );
                }
            } else if( p->has_trait( mstr ) ) {
                do {
                    p->remove_mutation( mstr );
                    rc++;
                } while( p->has_trait( mstr ) && rc < 10 );
            } else {
                do {
                    p->mutate_towards( mstr );
                    rc++;
                } while( !p->has_trait( mstr ) && rc < 10 );
            }
            cb.msg = string_format( _( "%s Mutation changes: %d" ), mstr.c_str(), rc );
            uistate.wishmutate_selected = wmenu.selected;
            if( rc != 0 ) {
                for( size_t i = 0; i < cb.vTraits.size(); i++ ) {
                    wmenu.entries[ i ].extratxt.txt.clear();
                    if( p->has_trait( cb.vTraits[ i ] ) ) {
                        wmenu.entries[ i ].text_color = c_green;
                        cb.pTraits[ cb.vTraits[ i ] ] = true;
                        if( p->has_base_trait( cb.vTraits[ i ] ) ) {
                            wmenu.entries[ i ].extratxt.txt = "T";
                        }
                    } else {
                        wmenu.entries[ i ].text_color = wmenu.text_color;
                        cb.pTraits[ cb.vTraits[ i ] ] = false;
                    }
                }
            }
        }
    } while( wmenu.ret >= 0 );
}

class wish_monster_callback: public uilist_callback
{
    public:
        int lastent;           // last menu entry
        std::string msg;       // feedback message
        bool friendly;         // spawn friendly critter?
        bool hallucination;
        int group;             // Number of monsters to spawn.
        catacurses::window w_info;        // ui_parent menu's padding area
        monster tmp;           // scrap critter for monster::print_info
        bool started;          // if unset, initialize window
        std::string padding;   // ' ' x window width
        const std::vector<const mtype *> &mtypes;

        wish_monster_callback( const std::vector<const mtype *> &mtypes )
            : mtypes( mtypes ) {
            started = false;
            friendly = false;
            hallucination = false;
            group = 0;
            lastent = -2;
        }

        void setup( uilist *menu ) {
            w_info = catacurses::newwin( menu->w_height - 2, menu->pad_right, 1,
                                         menu->w_x + menu->w_width - 1 - menu->pad_right );
            padding = std::string( getmaxx( w_info ), ' ' );
            werase( w_info );
            wrefresh( w_info );
        }

        bool key( const input_context &, const input_event &event, int entnum, uilist *menu ) override {
            ( void )entnum; // unused
            ( void )menu; // unused
            if( event.get_first_input() == 'f' ) {
                friendly = !friendly;
                lastent = -2; // force tmp monster regen
                return true;  // tell menu we handled keypress
            } else if( event.get_first_input() == 'i' ) {
                group++;
                return true;
            } else if( event.get_first_input() == 'h' ) {
                hallucination = !hallucination;
                return true;
            } else if( event.get_first_input() == 'd' && group != 0 ) {
                group--;
                return true;
            }
            return false;
        }

        void select( int entnum, uilist *menu ) override {
            if( ! started ) {
                started = true;
                setup( menu );
            }
            if( entnum != lastent ) {
                lastent = entnum;
                tmp = monster( mtypes[ entnum ]->id );
                if( friendly ) {
                    tmp.friendly = -1;
                }
            }

            werase( w_info );
            wrefresh( w_info );
            tmp.print_info( w_info, 2, 5, 1 );

            std::string header = string_format( "#%d: %s (%d)%s", entnum, tmp.type->nname().c_str(),
                                                group, ( hallucination ? _( " (hallucination)" ) : "" ) );
            mvwprintz( w_info, 0, ( getmaxx( w_info ) - header.size() ) / 2, c_cyan, header );

            mvwprintz( w_info, getmaxy( w_info ) - 3, 0, c_green, msg );
            msg = padding;
            input_context ctxt( menu->input_category );
            mvwprintw( w_info, getmaxy( w_info ) - 2, 0,
                       _( "[%s] find, [f]riendly, [h]allucination, [i]ncrease group, [d]ecrease group, [%s] quit" ),
                       ctxt.get_desc( "FILTER" ).c_str(), ctxt.get_desc( "QUIT" ).c_str() );
        }

        void refresh( uilist *menu ) override {
            ( void )menu; // unused
            wrefresh( w_info );
        }

        ~wish_monster_callback() override {
            werase( w_info );
            wrefresh( w_info );
        }
};

void debug_menu::wishmonster( const cata::optional<tripoint> &p )
{
    std::vector<const mtype *> mtypes;

    uilist wmenu;
    wmenu.w_x = 0;
    wmenu.w_width = TERMX;
    // disabled due to foldstring crash //( TERMX - getmaxx(w_terrain) - 30 > 24 ? getmaxx(w_terrain) : TERMX );
    wmenu.pad_right = ( wmenu.w_width - 30 );
    wmenu.selected = uistate.wishmonster_selected;
    wish_monster_callback cb( mtypes );
    wmenu.callback = &cb;

    int i = 0;
    for( const auto &montype : MonsterGenerator::generator().get_all_mtypes() ) {
        wmenu.addentry( i, true, 0, montype.nname() );
        wmenu.entries[i].extratxt.txt = montype.sym;
        wmenu.entries[i].extratxt.color = montype.color;
        wmenu.entries[i].extratxt.left = 1;
        ++i;
        mtypes.push_back( &montype );
    }

    do {
        wmenu.query();
        if( wmenu.ret >= 0 ) {
            monster mon = monster( mtypes[ wmenu.ret ]->id );
            if( cb.friendly ) {
                mon.friendly = -1;
            }
            if( cb.hallucination ) {
                mon.hallucination = true;
            }
            if( cata::optional<tripoint> spawn = p ? p : g->look_around() ) {
                const std::vector<tripoint> spawn_points = closest_tripoints_first( cb.group, *spawn );
                int num_spawned = 0;
                for( const tripoint &spawn_point : spawn_points ) {
                    if( g->critter_at( spawn_point ) == nullptr ) {
                        ++num_spawned;
                        mon.spawn( spawn_point );
                        g->add_zombie( mon, true );
                    }
                }
                input_context ctxt( wmenu.input_category );
                cb.msg = string_format( _( "Spawned %d/%d monsters, choose another or [%s] to quit." ),
                                        num_spawned, int( spawn_points.size() ), ctxt.get_desc( "QUIT" ).c_str() );
                uistate.wishmonster_selected = wmenu.selected;
                wmenu.redraw();
            }
        }
    } while( wmenu.ret >= 0 );
}

class wish_item_callback: public uilist_callback
{
    public:
        bool incontainer;
        bool has_flag;
        std::string msg;
        std::string flag;
        const std::vector<const itype *> &standard_itype_ids;
        wish_item_callback( const std::vector<const itype *> &ids ) :
            incontainer( false ), has_flag( false ), standard_itype_ids( ids ) {
        }
        bool key( const input_context &, const input_event &event, int /*entnum*/,
                  uilist * /*menu*/ ) override {
            if( event.get_first_input() == 'f' ) {
                incontainer = !incontainer;
                return true;
            }
            if( event.get_first_input() == 'F' ) {
                flag = string_input_popup()
                       .title( _( "Add which flag?  Use UPPERCASE letters without quotes" ) )
                       .query_string();
                if( flag.length() > 0 ) {
                    has_flag = true;
                }
                return true;
            }
            return false;
        }

        void select( int entnum, uilist *menu ) override {
            const int starty = 3;
            const int startx = menu->w_width - menu->pad_right;
            const std::string padding( menu->pad_right, ' ' );
            for( int y = 2; y < menu->w_height - 1; y++ ) {
                mvwprintw( menu->window, y, startx - 1, padding );
            }
            item tmp( standard_itype_ids[entnum], calendar::turn );
            mvwhline( menu->window, 1, startx, ' ', menu->pad_right - 1 );
            const std::string header = string_format( "#%d: %s%s%s", entnum,
                                       standard_itype_ids[entnum]->get_id().c_str(),
                                       ( incontainer ? _( " (contained)" ) : "" ),
                                       ( has_flag ? _( " (flagged)" ) : "" ) );
            mvwprintz( menu->window, 1, startx + ( menu->pad_right - 1 - header.size() ) / 2, c_cyan,
                       header );

            fold_and_print( menu->window, starty, startx, menu->pad_right - 1, c_light_gray, tmp.info( true ) );

            mvwprintz( menu->window, menu->w_height - 3, startx, c_green, msg );
            msg.erase();

            input_context ctxt( menu->input_category );
            mvwprintw( menu->window, menu->w_height - 2, startx,
                       _( "[%s] find, [f] container, [F] flag, [%s] quit" ),
                       ctxt.get_desc( "FILTER" ).c_str(), ctxt.get_desc( "QUIT" ).c_str() );
        }
};

void debug_menu::wishitem( player *p, int x, int y, int z )
{
    if( p == nullptr && x <= 0 ) {
        debugmsg( "game::wishitem(): invalid parameters" );
        return;
    }
    const auto opts = item_controller->all();

    int prev_amount = 1;
    int amount = 1;
    uilist wmenu;
    wmenu.w_x = 0;
    wmenu.w_width = TERMX;
    wmenu.pad_right = ( TERMX / 2 > 40 ? TERMX - 40 : TERMX / 2 );
    wmenu.selected = uistate.wishitem_selected;
    wish_item_callback cb( opts );
    wmenu.callback = &cb;

    for( size_t i = 0; i < opts.size(); i++ ) {
        item ity( opts[i], 0 );
        wmenu.addentry( i, true, 0, string_format( _( "%.*s" ), wmenu.pad_right - 5,
                        ity.tname( 1, false ).c_str() ) );
        wmenu.entries[i].extratxt.txt = ity.symbol();
        wmenu.entries[i].extratxt.color = ity.color();
        wmenu.entries[i].extratxt.left = 1;
    }

    do {
        wmenu.query();
        if( wmenu.ret >= 0 ) {
            item granted( opts[wmenu.ret] );
            if( cb.incontainer ) {
                granted = granted.in_its_container();
            }
            if( cb.has_flag ) {
                granted.item_tags.insert( cb.flag );
            }
            prev_amount = amount;
            bool canceled = false;
            if( p != nullptr ) {
                string_input_popup popup;
                popup
                .title( _( "How many?" ) )
                .width( 20 )
                .description( granted.tname() )
                .edit( amount );
                canceled = popup.canceled();
            }
            if( !canceled ) {
                if( p != nullptr ) {
                    if( granted.count_by_charges() ) {
                        if( amount > 0 ) {
                            granted.charges = amount;
                            p->i_add( granted );
                        }
                    } else {
                        for( int i = 0; i < amount; i++ ) {
                            p->i_add( granted );
                        }
                    }
                    p->invalidate_crafting_inventory();
                } else if( x >= 0 && y >= 0 ) {
                    g->m.add_item_or_charges( tripoint( x, y, z ), granted );
                    wmenu.ret = -1;
                }
                if( amount > 0 ) {
                    input_context ctxt( wmenu.input_category );
                    cb.msg = string_format( _( "Wish granted. Wish for more or hit [%s] to quit." ),
                                            ctxt.get_desc( "QUIT" ).c_str() );
                }
            }
            uistate.wishitem_selected = wmenu.selected;
            if( canceled || amount <= 0 ) {
                amount = prev_amount;
            }
        }
    } while( wmenu.ret >= 0 );
}

/*
 * Set skill on any player object; player character or NPC
 */
void debug_menu::wishskill( player *p )
{
    const int skoffset = 1;
    uilist skmenu;
    skmenu.text = _( "Select a skill to modify" );
    skmenu.allow_anykey = true;
    skmenu.addentry( 0, true, '1', _( "Modify all skills..." ) );

    std::vector<int> origskills;
    origskills.reserve( Skill::skills.size() );

    for( const auto &s : Skill::skills ) {
        const int level = p->get_skill_level( s.ident() );
        skmenu.addentry( origskills.size() + skoffset, true, -2, _( "@ %d: %s  " ), level,
                         s.name().c_str() );
        origskills.push_back( level );
    }

    do {
        skmenu.query();
        int skill_id = -1;
        int skset = -1;
        int sksel = skmenu.selected - skoffset;
        if( skmenu.ret == UILIST_UNBOUND && ( skmenu.keypress == KEY_LEFT ||
                                              skmenu.keypress == KEY_RIGHT ) ) {
            if( sksel >= 0 && sksel < static_cast<int>( Skill::skills.size() ) ) {
                skill_id = sksel;
                skset = p->get_skill_level( Skill::skills[skill_id].ident() ) +
                        ( skmenu.keypress == KEY_LEFT ? -1 : 1 );
            }
        } else if( skmenu.ret >= 0 && sksel >= 0 &&
                   sksel < static_cast<int>( Skill::skills.size() ) ) {
            skill_id = sksel;
            const Skill &skill = Skill::skills[skill_id];
            const int NUM_SKILL_LVL = 21;
            uilist sksetmenu;
            sksetmenu.w_height = NUM_SKILL_LVL + 4;
            sksetmenu.w_x = skmenu.w_x + skmenu.w_width + 1;
            sksetmenu.w_y = std::max( 0, skmenu.w_y + ( skmenu.w_height - sksetmenu.w_height ) / 2 );
            sksetmenu.settext( string_format( _( "Set '%s' to.." ), skill.name() ) );
            int skcur = p->get_skill_level( skill.ident() );
            sksetmenu.selected = skcur;
            for( int i = 0; i < NUM_SKILL_LVL; i++ ) {
                sksetmenu.addentry( i, true, i + 48, "%d%s", i, ( skcur == i ? _( " (current)" ) : "" ) );
            }
            sksetmenu.query();
            g->draw_ter();
            wrefresh( g->w_terrain );
            skset = sksetmenu.ret;
        }

        if( skill_id >= 0 && skset >= 0 ) {
            const Skill &skill = Skill::skills[skill_id];
            p->set_skill_level( skill.ident(), skset );
            skmenu.textformatted[0] = string_format( _( "%s set to %d             " ),
                                      skill.name(),
                                      p->get_skill_level( skill.ident() ) ).substr( 0, skmenu.w_width - 4 );
            skmenu.entries[skill_id + skoffset].txt = string_format( _( "@ %d: %s  " ),
                    p->get_skill_level( skill.ident() ),
                    skill.name() );
            skmenu.entries[skill_id + skoffset].text_color =
                ( p->get_skill_level( skill.ident() ) == origskills[skill_id] ?
                  skmenu.text_color : c_yellow );
        } else if( skmenu.ret == 0 && sksel == -1 ) {
            int ret = uilist( _( "Alter all skill values" ), {
                _( "Add 3" ), _( "Add 1" ),
                _( "Subtract 1" ), _( "Subtract 3" ), _( "Set to 0" ),
                _( "Set to 5" ), _( "Set to 10" ), _( "(Reset changes)" )
            } );
            if( ret >= 0 ) {
                int skmod = 0;
                int skset = -1;
                if( ret < 4 ) {
                    skmod = 3 - ret * 2;
                } else if( ret < 7 ) {
                    skset = ( ret - 4 ) * 5;
                }
                for( size_t skill_id = 0; skill_id < Skill::skills.size(); skill_id++ ) {
                    const Skill &skill = Skill::skills[skill_id];
                    int changeto = ( skmod != 0 ? p->get_skill_level( skill.ident() ) + skmod :
                                     ( skset != -1 ? skset : origskills[skill_id] ) );
                    p->set_skill_level( skill.ident(), std::max( 0, changeto ) );
                    skmenu.entries[skill_id + skoffset].txt = string_format( _( "@ %d: %s  " ),
                            p->get_skill_level( skill.ident() ),
                            skill.name() );
                    skmenu.entries[skill_id + skoffset].text_color =
                        ( p->get_skill_level( skill.ident() ) == origskills[skill_id] ?
                          skmenu.text_color : c_yellow );
                }
            }
        }
    } while( skmenu.ret != UILIST_CANCEL );
}
