#include "npctrade.h"

#include <algorithm>
#include <string>
#include <vector>

#include "cata_utility.h"
#include "debug.h"
#include "game.h"
#include "help.h"
#include "input.h"
#include "item_group.h"
#include "map.h"
#include "map_selector.h"
#include "npc.h"
#include "output.h"
#include "skill.h"
#include "string_formatter.h"
#include "translations.h"
#include "vehicle.h"
#include "vehicle_selector.h"

const skill_id skill_barter( "barter" );

inventory inventory_exchange( inventory &inv,
                              const std::set<item *> &without, const std::vector<item *> &added )
{
    std::vector<item *> item_dump;
    inv.dump( item_dump );
    item_dump.insert( item_dump.end(), added.begin(), added.end() );
    inventory new_inv;
    new_inv.copy_invlet_of( inv );

    for( item *it : item_dump ) {
        if( without.count( it ) == 0 ) {
            new_inv.add_item( *it, true, false );
        }
    }

    return new_inv;
}

std::vector<item_pricing> init_selling( npc &p )
{
    std::vector<item_pricing> result;
    invslice slice = p.inv.slice();
    for( auto &i : slice ) {
        auto &it = i->front();

        const int price = it.price( true );
        int val = p.value( it );
        if( p.wants_to_sell( it, val, price ) ) {
            result.emplace_back( p, &i->front(), val, false );
        }
    }

    if( p.is_friend() & !p.weapon.is_null() && !p.weapon.has_flag( "NO_UNWIELD" ) ) {
        result.emplace_back( p, &p.weapon, p.value( p.weapon ), false );
    }

    return result;
}

template <typename T, typename Callback>
void buy_helper( T &src, Callback cb )
{
    src.visit_items( [&src, &cb]( item * node ) {
        cb( std::move( item_location( src, node ) ) );

        return VisitResponse::SKIP;
    } );
}

std::vector<item_pricing> init_buying( npc &p, player &u )
{
    std::vector<item_pricing> result;

    const auto check_item = [&p, &result]( item_location && loc ) {
        item *it_ptr = loc.get_item();
        if( it_ptr == nullptr || it_ptr->is_null() ) {
            return;
        }

        auto &it = *it_ptr;
        int market_price = it.price( true );
        int val = p.value( it, market_price );
        if( p.wants_to_buy( it, val, market_price ) ) {
            result.emplace_back( std::move( loc ), val, false );
        }
    };

    invslice slice = u.inv.slice();
    for( auto &i : slice ) {
        // @todo: Sane way of handling multi-item stacks
        check_item( item_location( u, &i->front() ) );
    }

    if( !u.weapon.has_flag( "NO_UNWIELD" ) ) {
        check_item( item_location( u, &u.weapon ) );
    }

    for( auto &cursor : map_selector( u.pos(), 1 ) ) {
        buy_helper( cursor, check_item );
    }
    for( auto &cursor : vehicle_selector( u.pos(), 1 ) ) {
        buy_helper( cursor, check_item );
    }

    return result;
}

bool trade( npc &p, int cost, const std::string &deal )
{
    catacurses::window w_head = catacurses::newwin( 4, TERMX, 0, 0 );
    const int win_they_w = TERMX / 2;
    catacurses::window w_them = catacurses::newwin( TERMY - 4, win_they_w, 4, 0 );
    catacurses::window w_you = catacurses::newwin( TERMY - 4, TERMX - win_they_w, 4, win_they_w );
    catacurses::window w_tmp;
    std::string header_message = _( "\
TAB key to switch lists, letters to pick items, Enter to finalize, Esc to quit,\n\
? to get information on an item." );
    mvwprintz( w_head, 0, 0, c_white, header_message.c_str(), p.name.c_str() );

    // If entries were to get over a-z and A-Z, we wouldn't have good keys for them
    const size_t entries_per_page = std::min( TERMY - 7, 2 + ( 'z' - 'a' ) + ( 'Z' - 'A' ) );

    // Set up line drawings
    for( int i = 0; i < TERMX; i++ ) {
        mvwputch( w_head, 3, i, c_white, LINE_OXOX );
    }
    wrefresh( w_head );
    // End of line drawings

    // Populate the list of what the NPC is willing to buy, and the prices they pay
    // Note that the NPC's barter skill is factored into these prices.
    // TODO: Recalc item values every time a new item is selected
    // Trading is not linear - starving NPC may pay $100 for 3 jerky, but not $100000 for 300 jerky
    std::vector<item_pricing> theirs = init_selling( p );
    std::vector<item_pricing> yours = init_buying( p, g->u );

    // Adjust the prices based on your barter skill.
    // cap adjustment so nothing is ever sold below value
    ///\EFFECT_INT_NPC slightly increases bartering price changes, relative to your INT

    ///\EFFECT_BARTER_NPC increases bartering price changes, relative to your BARTER
    double their_adjust = ( price_adjustment( p.get_skill_level( skill_barter ) - g->u.get_skill_level(
                                skill_barter ) ) +
                            ( p.int_cur - g->u.int_cur ) / 20.0 );
    if( their_adjust < 1.0 ) {
        their_adjust = 1.0;
    }
    for( item_pricing &p : theirs ) {
        p.price *= their_adjust;
    }
    ///\EFFECT_INT slightly increases bartering price changes, relative to NPC INT

    ///\EFFECT_BARTER increases bartering price changes, relative to NPC BARTER
    double your_adjust = ( price_adjustment( g->u.get_skill_level( skill_barter ) - p.get_skill_level(
                               skill_barter ) ) +
                           ( g->u.int_cur - p.int_cur ) / 20.0 );
    if( your_adjust < 1.0 ) {
        your_adjust = 1.0;
    }
    for( item_pricing &p : yours ) {
        p.price *= your_adjust;
    }

    // Just exchanging items, no barter involved
    const bool ex = p.is_friend();

    // How much cash you get in the deal (negative = losing money)
    long cash = cost + p.op_of_u.owed;
    bool focus_them = true; // Is the focus on them?
    bool update = true;     // Re-draw the screen?
    size_t them_off = 0, you_off = 0; // Offset from the start of the list
    size_t ch, help;

    if( ex ) {
        // Sometimes owed money fails to reset for friends
        // NPC AI is way too weak to manage money, so let's just make them give stuff away for free
        cash = 0;
    }

    // Make a temporary copy of the NPC inventory to make sure volume calculations are correct
    inventory temp = p.inv;
    units::volume volume_left = p.volume_capacity() - p.volume_carried();
    units::mass weight_left = p.weight_capacity() - p.weight_carried();

    do {
#ifdef __ANDROID__
        input_context ctxt( "NPC_TRADE" );
        ctxt.register_manual_key( '\t', "Switch lists" );
        ctxt.register_manual_key( '<', "Back" );
        ctxt.register_manual_key( '>', "More" );
        ctxt.register_manual_key( '?', "Examine item" );
#endif

        auto &target_list = focus_them ? theirs : yours;
        auto &offset = focus_them ? them_off : you_off;
        if( update ) { // Time to re-draw
            update = false;
            // Draw borders, one of which is highlighted
            werase( w_them );
            werase( w_you );
            for( int i = 1; i < TERMX; i++ ) {
                mvwputch( w_head, 3, i, c_white, LINE_OXOX );
            }

            std::set<item *> without;
            std::vector<item *> added;

            for( auto &pricing : yours ) {
                if( pricing.selected ) {
                    added.push_back( pricing.loc.get_item() );
                }
            }

            for( auto &pricing : theirs ) {
                if( pricing.selected ) {
                    without.insert( pricing.loc.get_item() );
                }
            }

            temp = inventory_exchange( p.inv, without, added );

            volume_left = p.volume_capacity() - p.volume_carried_with_tweaks( { temp } );
            weight_left = p.weight_capacity() - p.weight_carried_with_tweaks( { temp } );
            mvwprintz( w_head, 3, 2, ( volume_left < 0_ml || weight_left < 0_gram ) ? c_red : c_green,
                       _( "Volume: %s %s, Weight: %.1f %s" ),
                       format_volume( volume_left ).c_str(), volume_units_abbr(),
                       convert_weight( weight_left ), weight_units() );

            std::string cost_string = ex ? _( "Exchange" ) : ( cash >= 0 ? _( "Profit %s" ) :
                                      _( "Cost %s" ) );
            mvwprintz( w_head, 3, TERMX / 2 + ( TERMX / 2 - cost_string.length() ) / 2,
                       ( cash < 0 && static_cast<int>( g->u.cash ) >= cash * -1 ) || ( cash >= 0 &&
                               static_cast<int>( p.cash )  >= cash ) ? c_green : c_red,
                       cost_string.c_str(), format_money( std::abs( cash ) ) );

            if( !deal.empty() ) {
                mvwprintz( w_head, 3, ( TERMX - deal.length() ) / 2, cost < 0 ? c_light_red : c_light_green,
                           deal.c_str() );
            }
            draw_border( w_them, ( focus_them ? c_yellow : BORDER_COLOR ) );
            draw_border( w_you, ( !focus_them ? c_yellow : BORDER_COLOR ) );

            mvwprintz( w_them, 0, 2, ( cash < 0 || static_cast<int>( p.cash ) >= cash ? c_green : c_red ),
                       _( "%s: %s" ), p.name.c_str(), format_money( p.cash ) );
            mvwprintz( w_you,  0, 2, ( cash > 0 ||
                                       static_cast<int>( g->u.cash ) >= cash * -1 ? c_green : c_red ),
                       _( "You: %s" ), format_money( g->u.cash ) );
            // Draw lists of items, starting from offset
            for( size_t whose = 0; whose <= 1; whose++ ) {
                const bool they = whose == 0;
                const auto &list = they ? theirs : yours;
                const auto &offset = they ? them_off : you_off;
                const auto &person = they ? p : g->u;
                auto &w_whose = they ? w_them : w_you;
                int win_h = getmaxy( w_whose );
                int win_w = getmaxx( w_whose );
                // Borders
                win_h -= 2;
                win_w -= 2;
                for( size_t i = offset; i < list.size() && i < entries_per_page + offset; i++ ) {
                    const item_pricing &ip = list[i];
                    const item *it = ip.loc.get_item();
                    auto color = it == &person.weapon ? c_yellow : c_light_gray;
                    std::string itname = it->display_name();
                    if( ip.loc.where() != item_location::type::character ) {
                        itname = itname + " " + ip.loc.describe( &g->u );
                        color = c_light_blue;
                    }

                    if( ip.selected ) {
                        color = c_white;
                    }

                    int keychar = i - offset + 'a';
                    if( keychar > 'z' ) {
                        keychar = keychar - 'z' - 1 + 'A';
                    }
                    trim_and_print( w_whose, i - offset + 1, 1, win_w, color, "%c %c %s",
                                    static_cast<char>( keychar ), ip.selected ? '+' : '-', itname.c_str() );
#ifdef __ANDROID__
                    ctxt.register_manual_key( keychar, itname.c_str() );
#endif

                    std::string price_str = string_format( "%.2f", ip.price / 100.0 );
                    nc_color price_color = ex ? c_dark_gray : ( ip.selected ? c_white : c_light_gray );
                    mvwprintz( w_whose, i - offset + 1, win_w - price_str.length(),
                               price_color, price_str.c_str() );
                }
                if( offset > 0 ) {
                    mvwprintw( w_whose, entries_per_page + 2, 1, _( "< Back" ) );
                }
                if( offset + entries_per_page < list.size() ) {
                    mvwprintw( w_whose, entries_per_page + 2, 9, _( "More >" ) );
                }
            }
            wrefresh( w_head );
            wrefresh( w_them );
            wrefresh( w_you );
        } // Done updating the screen
        // TODO: use input context
        ch = inp_mngr.get_input_event().get_first_input();
        switch( ch ) {
            case '\t':
                focus_them = !focus_them;
                update = true;
                break;
            case '<':
                if( offset > 0 ) {
                    offset -= entries_per_page;
                    update = true;
                }
                break;
            case '>':
                if( offset + entries_per_page < target_list.size() ) {
                    offset += entries_per_page;
                    update = true;
                }
                break;
            case '?':
                update = true;
                w_tmp = catacurses::newwin( 3, 21, 1 + ( TERMY - FULL_SCREEN_HEIGHT ) / 2,
                                            30 + ( TERMX - FULL_SCREEN_WIDTH ) / 2 );
                mvwprintz( w_tmp, 1, 1, c_red, _( "Examine which item?" ) );
                draw_border( w_tmp );
                wrefresh( w_tmp );
                // TODO: use input context
                help = inp_mngr.get_input_event().get_first_input() - 'a';
                mvwprintz( w_head, 0, 0, c_white, header_message.c_str(), p.name.c_str() );
                wrefresh( w_head );
                help += offset;
                if( help < target_list.size() ) {
                    popup( target_list[help].loc.get_item()->info(), PF_NONE );
                }
                break;
            case '\n': // Check if we have enough cash...
                // The player must pay cash, and it should not put the player negative.
                if( cash < 0 && static_cast<int>( g->u.cash ) < cash * -1 ) {
                    popup( _( "Not enough cash!  You have %s, price is %s." ), format_money( g->u.cash ),
                           format_money( -cash ) );
                    update = true;
                    ch = ' ';
                } else if( volume_left < 0_ml || weight_left < 0_gram ) {
                    // Make sure NPC doesn't go over allowed volume
                    popup( _( "%s can't carry all that." ), p.name.c_str() );
                    update = true;
                    ch = ' ';
                }
                break;
            default: // Letters & such
                if( ch >= 'a' && ch <= 'z' ) {
                    ch -= 'a';
                } else if( ch >= 'A' && ch <= 'Z' ) {
                    ch = ch - 'A' + ( 'z' - 'a' ) + 1;
                } else {
                    continue;
                }

                ch += offset;
                if( ch < target_list.size() ) {
                    update = true;
                    item_pricing &ip = target_list[ch];
                    ip.selected = !ip.selected;
                    if( !ex && ip.selected == focus_them ) {
                        cash -= ip.price;
                    } else if( !ex ) {
                        cash += ip.price;
                    }
                }
                ch = 0;
        }
    } while( ch != KEY_ESCAPE && ch != '\n' );

    const bool traded = ch == '\n';
    if( traded ) {
        int practice = 0;

        std::list<item_location *> from_map;
        const auto mark_for_exchange =
            [&practice, &from_map]( item_pricing & pricing, std::set<item *> &removing,
        std::vector<item *> &giving ) {
            if( !pricing.selected ) {
                return;
            }

            giving.push_back( pricing.loc.get_item() );
            practice++;

            if( pricing.loc.where() == item_location::type::character ) {
                removing.insert( pricing.loc.get_item() );
            } else {
                from_map.push_back( &pricing.loc );
            }
        };
        // This weird exchange is needed to prevent pointer bugs
        // Removing items from an inventory invalidates the pointers
        std::set<item *> removing_yours;
        std::vector<item *> giving_them;

        for( auto &pricing : yours ) {
            mark_for_exchange( pricing, removing_yours, giving_them );
        }

        std::set<item *> removing_theirs;
        std::vector<item *> giving_you;
        for( auto &pricing : theirs ) {
            mark_for_exchange( pricing, removing_theirs, giving_you );
        }

        const inventory &your_new_inv = inventory_exchange( g->u.inv,
                                        removing_yours, giving_you );
        const inventory &their_new_inv = inventory_exchange( p.inv,
                                         removing_theirs, giving_them );

        g->u.inv = your_new_inv;
        p.inv = their_new_inv;

        if( removing_yours.count( &g->u.weapon ) ) {
            g->u.remove_weapon();
        }

        if( removing_theirs.count( &p.weapon ) ) {
            p.remove_weapon();
        }

        for( item_location *loc_ptr : from_map ) {
            loc_ptr->remove_item();
        }

        if( !ex && cash > static_cast<int>( p.cash ) ) {
            // Trade was forced, give the NPC's cash to the player.
            p.op_of_u.owed = ( cash - p.cash );
            g->u.cash += p.cash;
            p.cash = 0;
        } else if( !ex ) {
            g->u.cash += cash;
            p.cash -= cash;
        }

        // TODO: Make this depend on prices
        // TODO: Make this depend on npc price adjustment vs. your price adjustment
        if( !ex ) {
            g->u.practice( skill_barter, practice / 2 );
        }
    }
    g->refresh_all();
    return traded;
}
