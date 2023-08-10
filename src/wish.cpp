#include "debug_menu.h" // IWYU pragma: associated

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <map>
#include <memory>
#include <new>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "bionics.h"
#include "calendar.h"
#include "catacharset.h"
#include "character.h"
#include "color.h"
#include "cursesdef.h"
#include "debug.h"
#include "effect.h"
#include "enums.h"
#include "game.h"
#include "input.h"
#include "item.h"
#include "item_factory.h"
#include "itype.h"
#include "localized_comparator.h"
#include "map.h"
#include "memory_fast.h"
#include "monster.h"
#include "monstergenerator.h"
#include "mtype.h"
#include "mutation.h"
#include "output.h"
#include "point.h"
#include "proficiency.h"
#include "skill.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "text_snippets.h"
#include "translations.h"
#include "type_id.h"
#include "ui.h"
#include "ui_manager.h"
#include "uistate.h"
#include "units.h"

static const efftype_id effect_pet( "pet" );

class ui_adaptor;

class wish_mutate_callback: public uilist_callback
{
    public:
        // Last menu entry
        int lastlen = 0;
        // Feedback message
        std::string msg;
        bool started = false;
        bool only_active = false;
        std::vector<trait_id> vTraits;
        std::map<trait_id, bool> pTraits;
        Character *you;

        nc_color mcolor( const trait_id &m ) {
            if( pTraits[ m ] ) {
                return c_green;
            }
            return c_light_gray;
        }

        wish_mutate_callback() = default;
        bool key( const input_context &, const input_event &event, int entnum, uilist *menu ) override {
            if( event.get_first_input() == 't' && you->has_trait( vTraits[ entnum ] ) ) {
                if( !you->has_base_trait( vTraits[ entnum ] ) ) {
                    you->unset_mutation( vTraits[ entnum ] );
                }

                you->toggle_trait( vTraits[ entnum ] );
                you->set_mutation( vTraits[ entnum ] );

                menu->entries[ entnum ].text_color = you->has_trait( vTraits[ entnum ] ) ? c_green :
                                                     menu->text_color;
                menu->entries[ entnum ].extratxt.txt = you->has_base_trait( vTraits[ entnum ] ) ? "T" : "";
                return true;
            } else if( event.get_first_input() == 'a' ) {
                only_active = !only_active;

                for( size_t i = 0; i < vTraits.size(); i++ ) {
                    if( !you->has_trait( vTraits[ i ] ) ) {
                        menu->entries[ i ].enabled = !only_active;
                    }
                }

                return true;
            }
            return false;
        }

        void refresh( uilist *menu ) override {
            if( !started ) {
                started = true;
                for( const mutation_branch &traits_iter : mutation_branch::get_all() ) {
                    vTraits.push_back( traits_iter.id );
                    pTraits[traits_iter.id] = you->has_trait( traits_iter.id );
                }
            }

            const std::string padding = std::string( menu->pad_right - 1, ' ' );

            const int startx = menu->w_width - menu->pad_right;
            for( int i = 2; i < lastlen; i++ ) {
                mvwprintw( menu->window, point( startx, i ), padding );
            }

            int line2 = 4;

            if( menu->selected >= 0 && static_cast<size_t>( menu->selected ) < vTraits.size() ) {
                const mutation_branch &mdata = vTraits[menu->selected].obj();

                mvwprintw( menu->window, point( startx, 3 ),
                           mdata.valid ? _( "Valid" ) : _( "Nonvalid" ) );

                line2++;
                mvwprintz(
                    menu->window,
                    point( startx, line2 ),
                    c_light_gray,
                    _( "Id:" )
                );
                mvwprintw(
                    menu->window,
                    point( startx + 11, line2 ),
                    mdata.id.str()
                );

                if( !mdata.prereqs.empty() ) {
                    line2++;
                    mvwprintz( menu->window, point( startx, line2 ), c_light_gray, _( "Prereqs:" ) );
                    for( const trait_id &j : mdata.prereqs ) {
                        mvwprintz( menu->window, point( startx + 11, line2 ), mcolor( j ),
                                   j->name() );
                        line2++;
                    }
                }

                if( !mdata.prereqs2.empty() ) {
                    line2++;
                    mvwprintz( menu->window, point( startx, line2 ), c_light_gray, _( "Prereqs, 2d:" ) );
                    for( const trait_id &j : mdata.prereqs2 ) {
                        mvwprintz( menu->window, point( startx + 15, line2 ), mcolor( j ),
                                   j->name() );
                        line2++;
                    }
                }

                if( !mdata.threshreq.empty() ) {
                    line2++;
                    mvwprintz( menu->window, point( startx, line2 ), c_light_gray, _( "Thresholds required:" ) );
                    for( const trait_id &j : mdata.threshreq ) {
                        mvwprintz( menu->window, point( startx + 21, line2 ), mcolor( j ),
                                   j->name() );
                        line2++;
                    }
                }

                if( !mdata.cancels.empty() ) {
                    line2++;
                    mvwprintz( menu->window, point( startx, line2 ), c_light_gray, _( "Cancels:" ) );
                    for( const trait_id &j : mdata.cancels ) {
                        mvwprintz( menu->window, point( startx + 11, line2 ), mcolor( j ),
                                   j->name() );
                        line2++;
                    }
                }

                if( !mdata.replacements.empty() ) {
                    line2++;
                    mvwprintz( menu->window, point( startx, line2 ), c_light_gray, _( "Becomes:" ) );
                    for( const trait_id &j : mdata.replacements ) {
                        mvwprintz( menu->window, point( startx + 11, line2 ), mcolor( j ),
                                   j->name() );
                        line2++;
                    }
                }

                if( !mdata.additions.empty() ) {
                    line2++;
                    mvwprintz( menu->window, point( startx, line2 ), c_light_gray, _( "Add-ons:" ) );
                    for( const string_id<mutation_branch> &j : mdata.additions ) {
                        mvwprintz( menu->window, point( startx + 11, line2 ), mcolor( j ),
                                   j->name() );
                        line2++;
                    }
                }

                if( !mdata.types.empty() ) {
                    line2++;
                    mvwprintz( menu->window, point( startx, line2 ), c_light_gray,  _( "Type:" ) );
                    for( const std::string &j : mdata.types ) {
                        mvwprintw( menu->window, point( startx + 11, line2 ), j );
                        line2++;
                    }
                }

                if( !mdata.category.empty() ) {
                    line2++;
                    mvwprintz( menu->window, point( startx, line2 ), c_light_gray,  _( "Category:" ) );
                    for( const mutation_category_id &j : mdata.category ) {
                        mvwprintw( menu->window, point( startx + 11, line2 ), j.str() );
                        line2++;
                    }
                }
                line2 += 2;

                //~ pts: points, vis: visibility, ugly: ugliness
                mvwprintz( menu->window, point( startx, line2 ), c_light_gray, _( "pts: %d vis: %d ugly: %d" ),
                           mdata.points,
                           mdata.visibility,
                           mdata.ugliness
                         );
                line2 += 2;

                std::vector<std::string> desc = foldstring( mdata.desc(),
                                                menu->pad_right - 1 );
                for( auto &elem : desc ) {
                    mvwprintz( menu->window, point( startx, line2 ), c_light_gray, elem );
                    line2++;
                }
            }

            lastlen = line2 + 1;

            mvwprintz( menu->window, point( startx, menu->w_height - 4 ), c_green, msg );
            msg.clear();
            input_context ctxt( menu->input_category, keyboard_mode::keycode );
            mvwprintw( menu->window, point( startx, menu->w_height - 3 ),
                       _( "[%s] find, [%s] quit, [t] toggle base trait" ),
                       ctxt.get_desc( "FILTER" ), ctxt.get_desc( "QUIT" ) );

            if( only_active ) {
                mvwprintz( menu->window, point( startx, menu->w_height - 2 ), c_green,
                           _( "[a] show active traits (active)" ) );
            } else {
                mvwprintz( menu->window, point( startx, menu->w_height - 2 ), c_white,
                           _( "[a] show active traits" ) );
            }

            wnoutrefresh( menu->window );
        }

        ~wish_mutate_callback() override = default;
};

void debug_menu::wishmutate( Character *you )
{
    uilist wmenu;
    int c = 0;

    for( const mutation_branch &traits_iter : mutation_branch::get_all() ) {
        wmenu.addentry( -1, true, -2, traits_iter.name() );
        wmenu.entries[ c ].extratxt.left = 1;
        wmenu.entries[ c ].extratxt.txt.clear();
        wmenu.entries[ c ].extratxt.color = c_light_green;
        if( you->has_trait( traits_iter.id ) ) {
            wmenu.entries[ c ].txt = string_format( _( "%s (active)" ), traits_iter.name() );
            wmenu.entries[ c ].text_color = c_green;
            if( you->has_base_trait( traits_iter.id ) ) {
                wmenu.entries[ c ].extratxt.txt = "T";
            }
        } else {
            wmenu.entries[ c ].txt = traits_iter.name();
        }
        c++;
    }
    wmenu.w_x_setup = 0;
    wmenu.w_width_setup = []() -> int {
        return TERMX;
    };
    wmenu.pad_right_setup = []() -> int {
        return TERMX - 40;
    };
    wmenu.selected = uistate.wishmutate_selected;
    wish_mutate_callback cb;
    cb.you = you;
    wmenu.callback = &cb;
    do {
        wmenu.query();
        if( wmenu.ret >= 0 ) {
            int rc = 0;
            const trait_id mstr = cb.vTraits[ wmenu.ret ];
            const mutation_branch &mdata = mstr.obj();
            const bool threshold = mdata.threshold;
            const bool profession = mdata.profession;
            // Manual override for the threshold-gaining
            if( threshold || profession ) {
                if( you->has_trait( mstr ) ) {
                    do {
                        you->remove_mutation( mstr );
                        rc++;
                    } while( you->has_trait( mstr ) && rc < 10 );
                } else {
                    do {
                        you->set_mutation( mstr );
                        rc++;
                    } while( !you->has_trait( mstr ) && rc < 10 );
                }
            } else if( you->has_trait( mstr ) ) {
                do {
                    you->remove_mutation( mstr );
                    rc++;
                } while( you->has_trait( mstr ) && rc < 10 );
            } else {
                do {
                    you->mutate_towards( mstr );
                    rc++;
                } while( !you->has_trait( mstr ) && rc < 10 );
            }
            cb.msg = string_format( _( "%s Mutation changes: %d" ), mstr.c_str(), rc );
            uistate.wishmutate_selected = wmenu.selected;
            if( rc != 0 ) {
                for( size_t i = 0; i < cb.vTraits.size(); i++ ) {
                    uilist_entry &entry = wmenu.entries[ i ];
                    entry.extratxt.txt.clear();
                    if( you->has_trait( cb.vTraits[ i ] ) ) {
                        entry.txt = string_format( _( "%s (active)" ), cb.vTraits[ i ]->name() );
                        entry.enabled = true;
                        entry.text_color = c_green;
                        cb.pTraits[ cb.vTraits[ i ] ] = true;
                        if( you->has_base_trait( cb.vTraits[ i ] ) ) {
                            entry.extratxt.txt = "T";
                        }
                    } else {
                        entry.txt = cb.vTraits[ i ]->name();
                        entry.enabled = entry.enabled ? true : !cb.only_active;
                        entry.text_color = wmenu.text_color;
                        cb.pTraits[ cb.vTraits[ i ] ] = false;
                    }
                }
            }
            wmenu.filterlist();
        }
    } while( wmenu.ret >= 0 );
}

void debug_menu::wishbionics( Character *you )
{
    std::vector<const itype *> cbm_items = item_controller->find( []( const itype & itm ) -> bool {
        return itm.can_use( "install_bionic" );
    } );
    std::sort( cbm_items.begin(), cbm_items.end(), []( const itype * a, const itype * b ) {
        return localized_compare( a->nname( 1 ), b->nname( 1 ) );
    } );

    while( true ) {
        units::energy power_level = you->get_power_level();
        units::energy power_max = you->get_max_power_level();
        size_t num_installed = you->get_bionics().size();

        bool can_uninstall = num_installed > 0;
        bool can_uninstall_all = can_uninstall || power_max > 0_J;

        uilist smenu;
        smenu.text += string_format(
                          _( "Current power level: %s\nMax power: %s\nBionics installed: %d" ),
                          units::display( power_level ),
                          units::display( power_max ),
                          num_installed
                      );
        smenu.addentry( 0, true, 'i', _( "Install from CBM…" ) );
        smenu.addentry( 1, can_uninstall, 'u', _( "Uninstall…" ) );
        smenu.addentry( 2, can_uninstall_all, 'U', _( "Uninstall all" ) );
        smenu.addentry( 3, true, 'c', _( "Edit power capacity (kJ)" ) );
        smenu.addentry( 4, true, 'C', _( "Edit power capacity (J)" ) );
        smenu.addentry( 5, true, 'p', _( "Edit power level (kJ)" ) );
        smenu.addentry( 6, true, 'P', _( "Edit power level (J)" ) );
        smenu.query();
        switch( smenu.ret ) {
            case 0: {
                uilist scbms;
                for( size_t i = 0; i < cbm_items.size(); i++ ) {
                    bool enabled = !you->has_bionic( cbm_items[i]->bionic->id );
                    scbms.addentry( i, enabled, MENU_AUTOASSIGN, "%s", cbm_items[i]->nname( 1 ) );
                }
                scbms.query();
                if( scbms.ret >= 0 ) {
                    const itype &cbm = *cbm_items[scbms.ret];
                    const bionic_id &bio = cbm.bionic->id;
                    constexpr int difficulty = 0;
                    constexpr int success = 1;
                    constexpr int level = 99;

                    bionic_uid upbio_uid = 0;
                    if( std::optional<bionic *> upbio = you->find_bionic_by_type( bio->upgraded_bionic ) ) {
                        upbio_uid = ( *upbio )->get_uid();
                    }

                    you->perform_install( bio, upbio_uid, difficulty, success, level, "NOT_MED",
                                          bio->canceled_mutations,
                                          you->pos() );
                }
                break;
            }
            case 1: {
                const bionic_collection &installed_bionics = *you->my_bionics;
                std::vector<std::string> bionic_names;
                std::vector<const bionic *> bionics;
                for( const bionic &bio : installed_bionics ) {
                    if( item::type_is_defined( bio.info().itype() ) ) {
                        bionic_names.emplace_back( bio.info().name.translated() );
                        bionics.push_back( &bio );
                    }
                }
                int bionic_index = uilist( _( "Choose bionic to uninstall" ), bionic_names );
                if( bionic_index < 0 ) {
                    return;
                }

                you->remove_bionic( *bionics[bionic_index] );
                break;
            }
            case 2: {
                you->clear_bionics();
                you->set_power_level( units::from_kilojoule( 0 ) );
                you->set_max_power_level( units::from_kilojoule( 0 ) );
                break;
            }
            case 3: {
                int new_value = 0;
                if( query_int( new_value, _( "Set the value to (in kJ)?  Currently: %s" ),
                               units::display( power_max ) ) ) {
                    you->set_max_power_level( units::from_kilojoule( new_value ) );
                    you->set_power_level( you->get_power_level() );
                }
                break;
            }
            case 4: {
                int new_value = 0;
                if( query_int( new_value, _( "Set the value to (in J)?  Currently: %s" ),
                               units::display( power_max ) ) ) {
                    you->set_max_power_level( units::from_joule( new_value ) );
                    you->set_power_level( you->get_power_level() );
                }
                break;
            }
            case 5: {
                int new_value = 0;
                if( query_int( new_value, _( "Set the value to (in kJ)?  Currently: %s" ),
                               units::display( power_level ) ) ) {
                    you->set_power_level( units::from_kilojoule( new_value ) );
                }
                break;
            }
            case 6: {
                int new_value = 0;
                if( query_int( new_value, _( "Set the value to (in J)?  Currently: %s" ),
                               units::display( power_level ) ) ) {
                    you->set_power_level( units::from_joule( new_value ) );
                }
                break;
            }
            default: {
                return;
            }
        }
    }
}

void debug_menu::wisheffect( Character &p )
{
    static bodypart_str_id effectbp = bodypart_str_id::NULL_ID();
    std::vector<effect> effects;
    uilist efmenu;
    efmenu.desc_enabled = true;
    const int offset = 2;
    bool only_active = false;

    const size_t effect_size = get_effect_types().size();
    effects.reserve( effect_size );

    auto effect_description = []( const effect & eff ) -> std::string {
        const effect_type &efft = *eff.get_effect_type();
        std::ostringstream descstr;

        if( eff.get_effect_type()->use_name_ints() )
        {
            descstr << eff.disp_name() << '\n';
        }

        descstr << _( "Intensity threshold: " );
        descstr <<  colorize( std::to_string( to_seconds<int>( efft.intensity_duration() ) ),
                              c_yellow );
        descstr << "s | ";

        descstr << _( "Max: " );
        int max_duration = to_seconds<int>( eff.get_max_duration() );
        descstr << colorize( std::to_string( max_duration ), c_yellow );
        descstr << "s\n";

        if( eff.get_effect_type()->use_desc_ints( false ) )
        {
            descstr << eff.disp_desc( false ) << '\n';
        }

        return descstr.str();
    };

    auto rebuild_menu = [&]( const bodypart_str_id & bp ) -> void {
        effects.clear();
        efmenu.entries.clear();
        efmenu.title = string_format( _( "Debug Effects Menu: %s" ), bp->id.str() );
        efmenu.addentry( 0, true, 'a', _( "Show only active" ) );
        efmenu.addentry( 1, true, 'b', _( "Change body part" ) );
        only_active = false;

        for( const std::pair<const efftype_id, effect_type> &eff : get_effect_types() )
        {
            const effect &plyeff = p.get_effect( eff.first, bp );
            if( plyeff.is_null() ) {
                effects.emplace_back( &*eff.first );
            } else {
                effects.emplace_back( plyeff );
            }
        }

        std::sort( effects.begin(), effects.end(), []( const effect & effA, const effect & effB )
        {
            return localized_compare( effA.get_id().str(), effB.get_id().str() );
        } );

        for( size_t i = 0; i < effect_size; ++i )
        {
            const effect &eff = effects[i];
            uilist_entry entry{static_cast<int>( i + offset ), true, -2, eff.get_id().str()};

            int duration = to_seconds<int>( eff.get_duration() );
            if( duration ) {
                entry.ctxt = colorize( std::to_string( duration ), c_white );
                if( eff.is_permanent() ) {
                    entry.ctxt += colorize( " PERMANENT", c_white );
                }
            }

            entry.desc = effect_description( eff );
            efmenu.entries.emplace_back( entry );
        }
    };

    rebuild_menu( effectbp );

    do {
        efmenu.query();
        if( efmenu.ret == 0 ) {
            only_active = !only_active;
            for( uilist_entry &entry : efmenu.entries ) {
                if( only_active ) {
                    if( entry.retval >= offset ) {
                        const int duration = to_seconds<int>( effects[entry.retval - offset].get_duration() );
                        entry.enabled = duration > 0 || entry.retval < offset;
                    }
                } else {
                    entry.enabled = true;
                }
            }
            continue;
        } else if( efmenu.ret == 1 ) {
            uilist bpmenu;
            bpmenu.title = _( "Choose bodypart" );
            bpmenu.addentry( 0, true, 'a', bodypart_str_id::NULL_ID()->id.str() );

            const std::vector<bodypart_id> &bodyparts = p.get_all_body_parts();
            const size_t bodyparts_size = bodyparts.size();
            for( int i = 0; i < static_cast<int>( bodyparts_size ); ++i ) {
                bpmenu.addentry( i + 1, true, -1, bodyparts[i]->id.str() );
            }

            bpmenu.query();
            if( bpmenu.ret == 0 ) {
                effectbp = bodypart_str_id::NULL_ID();
            } else if( bpmenu.ret > 0 ) {
                effectbp = bodyparts[bpmenu.ret - 1]->id;
            }
            rebuild_menu( effectbp );
        } else if( efmenu.ret > offset - 1 ) {
            uilist_entry &entry = efmenu.entries[efmenu.ret];
            effect &eff = effects[efmenu.ret - offset];

            int duration = to_seconds<int>( eff.get_duration() );
            query_int( duration, _( "Set duration (current %1$d): " ), duration );
            if( duration < 0 ) {
                continue;
            }

            bool permanent = false;
            if( duration ) {
                permanent = query_yn( _( "Permanent?" ) );
            }

            p.remove_effect( eff.get_id(), effectbp );
            bool invalid_effect = false;
            if( duration > 0 ) {
                p.add_effect( eff.get_id(), time_duration::from_seconds( duration ), effectbp, permanent );
                // Some effects like bandages on a limb like foot_l
                // cause a segmentation fault, check if it was applied first
                const effect &new_effect = p.get_effect( eff.get_id(), effectbp );
                if( !new_effect.is_null() ) {
                    eff = new_effect;
                } else {
                    invalid_effect = true;
                }
            } else {
                eff.set_duration( 0_seconds );
                if( only_active ) {
                    entry.enabled = false;
                }
            }

            entry.ctxt.clear();
            entry.desc.clear();

            if( invalid_effect ) {
                entry.ctxt += colorize( _( "INVALID ON THIS LIMB" ), c_red );
                entry.desc += colorize( _( "This effect can not be applied on this limb" ), c_red );
                entry.desc += '\n';
            } else {
                int cur_duration = to_seconds<int>( p.get_effect_dur( eff.get_id(), effectbp ) );
                if( cur_duration ) {
                    entry.ctxt = colorize( std::to_string( cur_duration ), c_yellow );
                    if( eff.is_permanent() ) {
                        entry.ctxt += colorize( _( " PERMANENT" ), c_yellow );
                    }
                }
            }

            entry.desc += effect_description( eff );

        }
    } while( efmenu.ret != UILIST_CANCEL );
}

class wish_monster_callback: public uilist_callback
{
    public:
        // last menu entry
        int lastent;
        // feedback message
        std::string msg;
        // spawn friendly critter?
        bool friendly;
        bool hallucination;
        // Number of monsters to spawn.
        int group;
        // scrap critter for monster::print_info
        monster tmp;
        const std::vector<const mtype *> &mtypes;

        explicit wish_monster_callback( const std::vector<const mtype *> &mtypes )
            : mtypes( mtypes ) {
            friendly = false;
            hallucination = false;
            group = 0;
            lastent = -2;
        }

        bool key( const input_context &, const input_event &event, int /*entnum*/,
                  uilist * /*menu*/ ) override {
            if( event.get_first_input() == 'f' ) {
                friendly = !friendly;
                // Force tmp monster regen
                lastent = -2;
                // Tell menu we handled keypress
                return true;
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

        void refresh( uilist *menu ) override {
            catacurses::window w_info = catacurses::newwin( menu->w_height - 2, menu->pad_right,
                                        point( menu->w_x + menu->w_width - 1 - menu->pad_right, 1 ) );

            const int entnum = menu->selected;
            const bool valid_entnum = entnum >= 0 && static_cast<size_t>( entnum ) < mtypes.size();
            if( entnum != lastent ) {
                lastent = entnum;
                if( valid_entnum ) {
                    tmp = monster( mtypes[ entnum ]->id );
                    if( friendly ) {
                        tmp.friendly = -1;
                    }
                } else {
                    tmp = monster();
                }
            }

            werase( w_info );
            if( valid_entnum ) {
                tmp.print_info( w_info, 2, 5, 1 );

                std::string header = string_format( "#%d: %s (%d)%s", entnum, tmp.type->id.str(),
                                                    group, hallucination ? _( " (hallucination)" ) : "" );
                mvwprintz( w_info, point( ( getmaxx( w_info ) - utf8_width( header ) ) / 2, 0 ), c_cyan, header );
            }

            mvwprintz( w_info, point( 0, getmaxy( w_info ) - 3 ), c_green, msg );
            msg.clear();
            input_context ctxt( menu->input_category, keyboard_mode::keycode );
            mvwprintw( w_info, point( 0, getmaxy( w_info ) - 2 ),
                       _( "[%s] find, [f]riendly, [h]allucination, [i]ncrease group, [d]ecrease group, [%s] quit" ),
                       ctxt.get_desc( "FILTER" ), ctxt.get_desc( "QUIT" ) );

            wnoutrefresh( w_info );
        }

        ~wish_monster_callback() override = default;
};

void debug_menu::wishmonster( const std::optional<tripoint> &p )
{
    std::vector<const mtype *> mtypes;

    uilist wmenu;
    wmenu.w_x_setup = 0;
    wmenu.w_width_setup = []() -> int {
        return TERMX;
    };
    wmenu.pad_right_setup = []() -> int {
        return TERMX - 30;
    };
    wmenu.selected = uistate.wishmonster_selected;
    wish_monster_callback cb( mtypes );
    wmenu.callback = &cb;

    int i = 0;
    for( const mtype &montype : MonsterGenerator::generator().get_all_mtypes() ) {
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
            const mtype_id &mon_type = mtypes[ wmenu.ret ]->id;
            if( std::optional<tripoint> spawn = p ? p : g->look_around() ) {
                int num_spawned = 0;
                for( const tripoint &destination : closest_points_first( *spawn, cb.group ) ) {
                    monster *const mon = g->place_critter_at( mon_type, destination );
                    if( !mon ) {
                        continue;
                    }
                    if( cb.friendly ) {
                        mon->friendly = -1;
                        mon->add_effect( effect_pet, 1_turns, true );
                    }
                    if( cb.hallucination ) {
                        mon->hallucination = true;
                    }
                    ++num_spawned;
                }
                input_context ctxt( wmenu.input_category, keyboard_mode::keycode );
                cb.msg = string_format( _( "Spawned %d monsters, choose another or [%s] to quit." ),
                                        num_spawned, ctxt.get_desc( "QUIT" ) );
                if( num_spawned == 0 ) {
                    cb.msg += _( "\nTarget location is not suitable for placing this kind of monster.  Choose a different target or [i]ncrease the groups size." );
                }
                uistate.wishmonster_selected = wmenu.selected;
            }
        }
    } while( wmenu.ret >= 0 );
}

static item wishitem_produce( const itype &type, std::string &flags, bool incontainer )
{
    item granted( &type, calendar::turn );

    granted.unset_flags();
    for( const auto &tag : debug_menu::string_to_iterable<std::vector<std::string>>( flags, " " ) ) {
        const flag_id flag( tag );
        if( flag.is_valid() ) {
            granted.set_flag( flag_id( tag ) );
        }
    }

    if( incontainer ) {
        granted = granted.in_its_container();
    }
    // If the item has an ammunition, this loads it to capacity, including magazines.
    if( !granted.ammo_default().is_null() ) {
        granted.ammo_set( granted.ammo_default(), -1 );
    }

    return granted;
}

class wish_item_callback: public uilist_callback
{
    public:
        bool incontainer;
        bool spawn_everything;
        bool renew_snippet;
        std::string msg;
        std::string flags;
        std::string itype_flags;
        std::pair<int, std::string> chosen_snippet_id;
        const std::vector<const itype *> &standard_itype_ids;
        const std::vector<const itype_variant_data *> &itype_variants;
        std::string &last_snippet_id;

        explicit wish_item_callback( const std::vector<const itype *> &ids,
                                     const std::vector<const itype_variant_data *> &variants, std::string &snippet_ids ) :
            incontainer( false ), spawn_everything( false ),
            standard_itype_ids( ids ), itype_variants( variants ),
            last_snippet_id( snippet_ids ) {
        }

        void select( uilist *menu ) override {
            if( menu->selected < 0 ) {
                return;
            }

            chosen_snippet_id = { -1, "" };
            renew_snippet = true;
            const itype &selected_itype = *standard_itype_ids[menu->selected];
            // Make liquids "contained" by default (toggled with CONTAINER action)
            incontainer = selected_itype.phase == phase_id::LIQUID;
            // Clear instance flags when switching items
            flags.clear();
            // Grab default flags for the itype (added with the FLAG action)
            itype_flags = debug_menu::iterable_to_string( selected_itype.get_flags(), " ",
            []( const flag_id & f ) {
                return f.str();
            } );
        }

        bool key( const input_context &ctxt, const input_event &event, int /*entnum*/,
                  uilist *menu ) override {

            const std::string &action = ctxt.input_to_action( event );
            const int cur_key = event.get_first_input();
            if( action == "CONTAINER" ) {
                incontainer = !incontainer;
                return true;
            }
            if( action == "FLAG" ) {
                std::string edit_flags;
                if( flags.empty() ) {
                    // If this is the first time using the FLAG action on this item, start with itype flags
                    edit_flags = itype_flags;
                } else {
                    // Otherwise, edit the existing list of user-defined instance flags
                    edit_flags = flags;
                }
                string_input_popup popup;
                popup
                .title( _( "Flags:" ) )
                .description( _( "UPPERCASE, no quotes, separate with spaces" ) )
                .max_length( 100 )
                .text( edit_flags )
                .query();
                // Save instance flags on this item (will be reset when selecting another item)
                if( popup.confirmed() ) {
                    flags = popup.text();
                    return true;
                }
            }
            if( action == "EVERYTHING" ) {
                spawn_everything = !spawn_everything;
                return true;
            }
            if( action == "SNIPPET" ) {
                if( menu->selected <= -1 ) {
                    return true;
                }
                const int entnum = menu->selected;
                const itype &selected_itype = *standard_itype_ids[entnum];
                if( !selected_itype.snippet_category.empty() ) {
                    const std::string cat = selected_itype.snippet_category;
                    if( SNIPPET.has_category( cat ) ) {
                        std::vector<std::pair<snippet_id, std::string>> snippes = SNIPPET.get_snippets_by_category( cat,
                                true );
                        if( !snippes.empty() ) {
                            uilist snipp_query;
                            snipp_query.text = _( "Choose snippet type." );
                            snipp_query.desc_lines_hint = 2;
                            snipp_query.desc_enabled = true;
                            int cnt = 0;
                            for( const std::pair<snippet_id, std::string> &elem : snippes ) {
                                std::string desc = elem.second;
                                snipp_query.addentry_desc( cnt, true, -1, elem.first.str(), desc );
                                cnt ++;
                            }
                            snipp_query.query();
                            switch( snipp_query.ret ) {
                                case UILIST_CANCEL:
                                    break;
                                default:
                                    chosen_snippet_id = { entnum, snippes[snipp_query.ret].first.str() };
                                    break;
                            }
                        }
                    }
                }
                return true;
            }
            if( cur_key == KEY_LEFT || cur_key == KEY_RIGHT ) {
                // For Renew snippet_id.
                renew_snippet = true;
                return true;
            }
            return false;
        }

        void refresh( uilist *menu ) override {
            const int starty = 3;
            const int startx = menu->w_width - menu->pad_right;
            const std::string padding( menu->pad_right, ' ' );
            for( int y = 2; y < menu->w_height - 1; y++ ) {
                mvwprintw( menu->window, point( startx - 1, y ), padding );
            }
            mvwhline( menu->window, point( startx, 1 ), ' ', menu->pad_right - 1 );
            const int entnum = menu->selected;
            if( entnum >= 0 && static_cast<size_t>( entnum ) < standard_itype_ids.size() ) {
                item tmp = wishitem_produce( *standard_itype_ids[entnum], flags, false );

                const itype_variant_data *variant = itype_variants[entnum];
                if( variant != nullptr && tmp.has_itype_variant( false ) ) {
                    // Set the variant type as shown in the selected list item.
                    std::string variant_id = variant->id;
                    tmp.set_itype_variant( variant_id );
                }

                if( !tmp.type->snippet_category.empty() ) {
                    if( renew_snippet ) {
                        last_snippet_id = tmp.snip_id.str();
                        renew_snippet = false;
                    } else if( chosen_snippet_id.first == entnum && !chosen_snippet_id.second.empty() ) {
                        std::string snip = chosen_snippet_id.second;
                        if( snippet_id( snip ).is_valid() || snippet_id( snip ) == snippet_id::NULL_ID() ) {
                            tmp.snip_id = snippet_id( snip );
                            last_snippet_id = snip;
                        }
                    } else {
                        tmp.snip_id = snippet_id( last_snippet_id );
                    }
                }

                const std::string header = string_format( "#%d: %s%s%s", entnum,
                                           standard_itype_ids[entnum]->get_id().c_str(),
                                           incontainer ? _( " (contained)" ) : "",
                                           flags.empty() ? "" : _( " (flagged)" ) );
                mvwprintz( menu->window, point( startx + ( menu->pad_right - 1 - utf8_width( header ) ) / 2, 1 ),
                           c_cyan, header );

                fold_and_print( menu->window, point( startx, starty ), menu->pad_right - 1, c_light_gray,
                                tmp.info( true ) );
            }

            mvwprintz( menu->window, point( startx, menu->w_height - 3 ), c_green, msg );
            msg.erase();
            input_context ctxt( menu->input_category, keyboard_mode::keycode );
            mvwprintw( menu->window, point( startx, menu->w_height - 2 ),
                       _( "[%s] find, [%s] container, [%s] flag, [%s] everything, [%s] snippet, [%s] quit" ),
                       ctxt.get_desc( "FILTER" ), ctxt.get_desc( "CONTAINER" ),
                       ctxt.get_desc( "FLAG" ), ctxt.get_desc( "EVERYTHING" ),
                       ctxt.get_desc( "SNIPPET" ),
                       ctxt.get_desc( "QUIT" ) );
            wnoutrefresh( menu->window );
        }
};

void debug_menu::wishitem( Character *you )
{
    wishitem( you, tripoint( -1, -1, -1 ) );
}

void debug_menu::wishitem( Character *you, const tripoint &pos )
{
    if( you == nullptr && pos.x <= 0 ) {
        debugmsg( "game::wishitem(): invalid parameters" );
        return;
    }
    std::vector<std::tuple<std::string, const itype *, const itype_variant_data *>> opts;
    for( const itype *i : item_controller->all() ) {
        item option( i, calendar::turn_zero );

        if( i->variant_kind == itype_variant_kind::gun || i->variant_kind == itype_variant_kind::generic ) {
            for( const itype_variant_data &variant : i->variants ) {
                const std::string gun_variant_name = variant.alt_name.translated();
                const itype_variant_data *ivd = &variant;
                opts.emplace_back( gun_variant_name, i, ivd );
            }
        }
        option.clear_itype_variant();
        opts.emplace_back( option.tname( 1, false ), i, nullptr );
    }
    std::sort( opts.begin(), opts.end(), localized_compare );
    std::vector<const itype *> itypes;
    std::vector<const itype_variant_data *> ivariants;
    std::transform( opts.begin(), opts.end(), std::back_inserter( itypes ),
    []( const auto & pair ) {
        return std::get<1>( pair );
    } );

    std::transform( opts.begin(), opts.end(), std::back_inserter( ivariants ),
    []( const auto & pair ) {
        return std::get<2>( pair );
    } );

    int prev_amount = 1;
    int amount = 1;
    std::string snipped_id_str;
    uilist wmenu;
    wmenu.input_category = "WISH_ITEM";
    wmenu.additional_actions = {
        { "CONTAINER", translation() },
        { "FLAG", translation() },
        { "EVERYTHING", translation() },
        { "SNIPPET", translation() }
    };
    wmenu.w_x_setup = 0;
    wmenu.w_width_setup = []() -> int {
        return TERMX;
    };
    wmenu.pad_right_setup = []() -> int {
        return std::max( TERMX / 2, TERMX - 50 );
    };
    wmenu.selected = uistate.wishitem_selected;
    wish_item_callback cb( itypes, ivariants, snipped_id_str );
    wmenu.callback = &cb;

    for( size_t i = 0; i < opts.size(); i++ ) {
        item ity( std::get<1>( opts[i] ), calendar::turn_zero );
        std::string i_name = std::get<0>( opts[i] );
        if( std::get<2>( opts[i] ) != nullptr ) {
            i_name += "<color_dark_gray>(V)</color>";
        }
        if( !std::get<1>( opts[i] )->snippet_category.empty() ) {
            i_name += "<color_yellow>(S)</color>";
        }

        wmenu.addentry( i, true, 0, i_name );
        mvwzstr &entry_extra_text = wmenu.entries[i].extratxt;
        entry_extra_text.txt = ity.symbol();
        entry_extra_text.color = ity.color();
        entry_extra_text.left = 1;
    }
    do {
        wmenu.query();
        if( cb.spawn_everything ) {
            wmenu.ret = opts.size() - 1;
        }
        bool did_amount_prompt = false;
        while( wmenu.ret >= 0 ) {
            item granted = wishitem_produce( *std::get<1>( opts[wmenu.ret] ), cb.flags, cb.incontainer ) ;
            const itype_variant_data *variant = std::get<2>( opts[wmenu.ret] );
            if( variant != nullptr && granted.has_itype_variant( false ) ) {
                std::string variant_id = variant->id;
                granted.set_itype_variant( variant_id );
            }
            if( !granted.type->snippet_category.empty() && ( snippet_id( snipped_id_str ).is_valid() ||
                    snippet_id( snipped_id_str ) == snippet_id::NULL_ID() ) ) {
                granted.snip_id = snippet_id( snipped_id_str );
            }

            prev_amount = amount;
            bool canceled = false;
            if( you != nullptr && !did_amount_prompt ) {
                string_input_popup popup;
                popup
                .title( _( "How many?" ) )
                .width( 20 )
                .description( granted.tname() )
                .edit( amount );
                canceled = popup.canceled();
            }
            if( !canceled ) {
                did_amount_prompt = true;
                if( you != nullptr ) {
                    if( granted.count_by_charges() ) {
                        if( amount > 0 ) {
                            granted.charges = amount;
                            if( you->can_stash( granted ) ) {
                                you->i_add( granted );
                            } else {
                                get_map().add_item_or_charges( you->pos(), granted );
                            }
                        }
                    } else {
                        int stashable_copy_num = amount;
                        you->i_add( granted, stashable_copy_num, true, nullptr, nullptr, true, false );
                    }
                    you->invalidate_crafting_inventory();
                } else if( pos.x >= 0 && pos.y >= 0 ) {
                    get_map().add_item_or_charges( pos, granted );
                    wmenu.ret = -1;
                }
                if( amount > 0 ) {
                    input_context ctxt( wmenu.input_category, keyboard_mode::keycode );
                    cb.msg = string_format( _( "Wish granted.  Wish for more or hit [%s] to quit." ),
                                            ctxt.get_desc( "QUIT" ) );
                }
            }
            uistate.wishitem_selected = wmenu.selected;
            if( canceled || amount <= 0 ) {
                amount = prev_amount;
            }
            if( cb.spawn_everything ) {
                wmenu.ret--;
            } else {
                break;
            }
        }
    } while( wmenu.ret >= 0 );
}

/*
 * Set skill on any Character object; player character or NPC
 * Can change skill theory level
 */
void debug_menu::wishskill( Character *you, bool change_theory )
{
    const int skoffset = 1;
    uilist skmenu;
    skmenu.text = change_theory ?
                  _( "Select a skill to modify its theory level" ) : _( "Select a skill to modify" );
    skmenu.allow_anykey = true;
    skmenu.additional_actions = {
        { "LEFT", to_translation( "Decrease skill" ) },
        { "RIGHT", to_translation( "Increase skill" ) }
    };
    skmenu.addentry( 0, true, '1', _( "Modify all skills…" ) );

    auto sorted_skills = Skill::get_skills_sorted_by( []( const Skill & a, const Skill & b ) {
        return localized_compare( a.name(), b.name() );
    } );

    auto get_level = [&change_theory, &you]( const Skill & skill ) -> int {
        return change_theory ?
        you->get_knowledge_level( skill.ident() ) : you->get_skill_level( skill.ident() );
    };

    std::vector<int> origskills;
    origskills.reserve( sorted_skills.size() );

    for( const Skill *s : sorted_skills ) {
        const int level = get_level( *s );
        skmenu.addentry( origskills.size() + skoffset, true, -2, _( "@ %d: %s  " ), level,
                         s->name() );
        origskills.push_back( level );
    }

    shared_ptr_fast<ui_adaptor> skmenu_ui = skmenu.create_or_get_ui_adaptor();

    do {
        skmenu.query();
        int skill_id = -1;
        int skset = -1;
        const int sksel = skmenu.selected - skoffset;
        if( skmenu.ret == UILIST_UNBOUND && ( skmenu.ret_act == "LEFT" ||
                                              skmenu.ret_act == "RIGHT" ) ) {
            if( sksel >= 0 && sksel < static_cast<int>( sorted_skills.size() ) ) {
                skill_id = sksel;
                skset = get_level( *sorted_skills[skill_id] ) +
                        ( skmenu.ret_act == "LEFT" ? -1 : 1 );
            }
        } else if( skmenu.ret >= 0 && sksel >= 0 &&
                   sksel < static_cast<int>( sorted_skills.size() ) ) {
            skill_id = sksel;
            const Skill &skill = *sorted_skills[skill_id];
            uilist sksetmenu;
            sksetmenu.w_height_setup = MAX_SKILL + 5;
            sksetmenu.w_x_setup = [&]( int ) -> int {
                return skmenu.w_x + skmenu.w_width + 1;
            };
            sksetmenu.w_y_setup = [&]( const int height ) {
                return std::max( 0, skmenu.w_y + ( skmenu.w_height - height ) / 2 );
            };
            sksetmenu.settext( string_format( _( "Set '%s' to…" ), skill.name() ) );
            const int skcur = get_level( skill );
            sksetmenu.selected = skcur;
            for( int i = 0; i <= MAX_SKILL; i++ ) {
                sksetmenu.addentry( i, true, i + 48, "%d%s", i, skcur == i ? _( " (current)" ) : "" );
            }
            sksetmenu.query();
            skset = sksetmenu.ret;
        }

        if( skill_id >= 0 && skset >= 0 ) {
            const Skill &skill = *sorted_skills[skill_id];
            if( change_theory ) {
                you->set_knowledge_level( skill.ident(), skset );
            } else {
                you->set_skill_level( skill.ident(), skset );
            }
            skmenu.textformatted[0] = string_format( _( "%s set to %d             " ),
                                      skill.name(),
                                      get_level( skill ) ).substr( 0, skmenu.w_width - 4 );
            skmenu.entries[skill_id + skoffset].txt = string_format( _( "@ %d: %s  " ),
                    get_level( skill ),
                    skill.name() );
            skmenu.entries[skill_id + skoffset].text_color =
                get_level( skill ) == origskills[skill_id] ?
                skmenu.text_color : c_yellow;
        } else if( skmenu.ret == 0 && sksel == -1 ) {
            const int ret = uilist( _( "Alter all skill values" ), {
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
                for( size_t skill_id = 0; skill_id < sorted_skills.size(); skill_id++ ) {
                    const Skill &skill = *sorted_skills[skill_id];
                    int changeto = skmod != 0 ? get_level( skill ) + skmod :
                                   skset != -1 ? skset : origskills[skill_id];

                    if( change_theory ) {
                        you->set_knowledge_level( skill.ident(), std::max( 0, changeto ) );
                    } else {
                        you->set_skill_level( skill.ident(), std::max( 0, changeto ) );
                    }

                    skmenu.entries[skill_id + skoffset].txt = string_format( _( "@ %d: %s  " ),
                            get_level( skill ), skill.name() );

                    you->get_skill_level_object( skill.ident() ).practice();
                    skmenu.entries[skill_id + skoffset].text_color =
                        get_level( skill ) == origskills[skill_id] ? skmenu.text_color : c_yellow;
                }
            }
        }
    } while( skmenu.ret != UILIST_CANCEL );
}

/*
 * Set proficiency on any Character object; player character or NPC
 */
void debug_menu::wishproficiency( Character *you )
{
    bool know_all = true;
    const int proffset = 1;

    uilist prmenu;
    prmenu.text = _( "Select proficiency to toggle" );
    prmenu.allow_anykey = true;
    prmenu.addentry( 0, true, '1', _( "Toggle all proficiencies" ) );

    const std::vector<proficiency_id> &known_profs = you->known_proficiencies();
    std::vector<std::pair<proficiency_id, bool>> sorted_profs;

    for( const proficiency &cur : proficiency::get_all() ) {

        const auto iterator = std::find_if( known_profs.begin(), known_profs.end(),
        [&cur]( proficiency_id prof_id ) {
            return cur.prof_id() == prof_id;
        } );

        const bool player_know = iterator != known_profs.end();

        // Does the player know all proficiencies
        if( know_all ) {
            know_all = player_know;
        }

        sorted_profs.emplace_back( cur.prof_id(), player_know );
    }

    std::sort( sorted_profs.begin(), sorted_profs.end(), [](
    const std::pair<proficiency_id, bool> &profA,  const std::pair<proficiency_id, bool> &profB ) {
        return localized_compare( profA.first->name(), profB.first->name() );
    } );

    for( size_t i = 0; i < sorted_profs.size(); ++i ) {
        if( sorted_profs[i].second ) {
            prmenu.addentry( i + proffset, true, -2, _( "(known) %s" ),
                             sorted_profs[i].first->name() );
            prmenu.entries[i + proffset].text_color = c_yellow;
        } else {
            prmenu.addentry( i + proffset, true, -2, _( "%s" ),
                             sorted_profs[i].first->name() );
            prmenu.entries[i + proffset].text_color = prmenu.text_color;
        }
    }

    do {
        prmenu.query();
        const int prsel = prmenu.ret;
        if( prsel == 0 ) {
            // if the player knows everything, unlearn everything
            if( know_all ) {
                for( size_t i = 0; i < sorted_profs.size(); ++i ) {
                    std::pair<proficiency_id, bool> &cur = sorted_profs[i];
                    cur.second = false;
                    prmenu.entries[i + proffset].txt = string_format( "%s",  cur.first->name() );
                    prmenu.entries[i + proffset].text_color = prmenu.text_color;
                    you->lose_proficiency( cur.first, true );
                }
                know_all = false;
            } else {
                for( size_t i = 0; i < sorted_profs.size(); ++i ) {
                    std::pair<proficiency_id, bool> &cur = sorted_profs[i];

                    if( !cur.second ) {
                        cur.second = true;
                        prmenu.entries[i + proffset].txt = string_format( _( "(known) %s" ), cur.first->name() );
                        prmenu.entries[i + proffset].text_color = c_yellow;
                        you->add_proficiency( cur.first, true );
                    }
                }
                know_all = true;
            }
        } else if( prsel > 0 ) {
            std::pair<proficiency_id, bool> &cur = sorted_profs[prsel - proffset];
            // if the player didn't know it before now it does
            // if the player knew it before, unlearn proficiency
            bool know_prof = !cur.second;
            proficiency_id &prof = cur.first;

            cur.second = know_prof;

            if( know_prof ) {
                prmenu.entries[prmenu.selected].txt = string_format( _( "(known) %s" ), cur.first->name() );
                prmenu.entries[prmenu.selected].text_color = c_yellow;
                you->add_msg_if_player( m_good, _( "You are now proficient in %s!" ), prof->name() );
                you->add_proficiency( prof, true );
                continue;
            }

            know_all = false;
            prmenu.entries[prmenu.selected].txt = string_format( "%s", cur.first->name() );
            prmenu.entries[prmenu.selected].text_color = prmenu.text_color;
            you->add_msg_if_player( m_bad, _( "You are no longer proficient in %s." ), prof->name() );
            you->lose_proficiency( prof, true );
        }
    } while( prmenu.ret != UILIST_CANCEL );
}
