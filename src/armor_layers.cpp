#include "player.h" // IWYU pragma: associated

#include <algorithm>
#include <string>
#include <vector>
#include <iterator>
#include <cstddef>

#include "avatar.h"
#include "cata_utility.h"
#include "catacharset.h" // used for utf8_width()
#include "game.h"
#include "game_inventory.h"
#include "input.h"
#include "item.h"
#include "line.h"
#include "output.h"
#include "string_formatter.h"
#include "translations.h"
#include "debug.h"
#include "enums.h"

namespace
{
std::string clothing_layer( const item &worn_item );
std::vector<std::string> clothing_properties(
    const item &worn_item, int width, const Character & );
std::vector<std::string> clothing_protection( const item &worn_item, int width );
std::vector<std::string> clothing_flags_description( const item &worn_item );

struct item_penalties {
    std::vector<body_part> body_parts_with_stacking_penalty;
    std::vector<body_part> body_parts_with_out_of_order_penalty;
    std::set<std::string> bad_items_within;

    int badness() const {
        return !body_parts_with_stacking_penalty.empty() +
               !body_parts_with_out_of_order_penalty.empty();
    }

    nc_color color_for_stacking_badness() const {
        switch( badness() ) {
            case 0:
                return c_light_gray;
            case 1:
                return c_yellow;
            case 2:
                return c_light_red;
        }
        debugmsg( "Unexpected badness %d", badness() );
        return c_light_gray;
    }
};

// Figure out encumbrance penalties this clothing is involved in
item_penalties get_item_penalties( std::list<item>::const_iterator worn_item_it,
                                   const Character &c, int tabindex )
{
    layer_level layer = worn_item_it->get_layer();

    std::vector<body_part> body_parts_with_stacking_penalty;
    std::vector<body_part> body_parts_with_out_of_order_penalty;
    std::vector<std::set<std::string>> lists_of_bad_items_within;

    for( body_part bp : all_body_parts ) {
        if( bp != tabindex && num_bp != tabindex ) {
            continue;
        }
        if( !worn_item_it->covers( bp ) ) {
            continue;
        }
        const int num_items = std::count_if( c.worn.begin(), c.worn.end(),
        [layer, bp]( const item & i ) {
            return i.get_layer() == layer && i.covers( bp ) && !i.has_flag( "SEMITANGIBLE" );
        } );
        if( num_items > 1 ) {
            body_parts_with_stacking_penalty.push_back( bp );
        }

        std::set<std::string> bad_items_within;
        for( auto it = c.worn.begin(); it != worn_item_it; ++it ) {
            if( it->get_layer() > layer && it->covers( bp ) ) {
                bad_items_within.insert( it->type_name() );
            }
        }
        if( !bad_items_within.empty() ) {
            body_parts_with_out_of_order_penalty.push_back( bp );
            lists_of_bad_items_within.push_back( bad_items_within );
        }
    }

    // We intersect all the lists_of_bad_items_within so that if there is one
    // common bad item we're wearing this one over it can be mentioned in the
    // message explaining the penalty.
    while( lists_of_bad_items_within.size() > 1 ) {
        std::set<std::string> intersection_of_first_two;
        std::set_intersection(
            lists_of_bad_items_within[0].begin(), lists_of_bad_items_within[0].end(),
            lists_of_bad_items_within[1].begin(), lists_of_bad_items_within[1].end(),
            std::inserter( intersection_of_first_two, intersection_of_first_two.begin() )
        );
        lists_of_bad_items_within.erase( lists_of_bad_items_within.begin() );
        lists_of_bad_items_within[0] = std::move( intersection_of_first_two );
    }

    if( lists_of_bad_items_within.empty() ) {
        lists_of_bad_items_within.emplace_back();
    }

    return { std::move( body_parts_with_stacking_penalty ),
             std::move( body_parts_with_out_of_order_penalty ),
             std::move( lists_of_bad_items_within[0] ) };
}

std::string body_part_names( const std::vector<body_part> &parts )
{
    if( parts.empty() ) {
        debugmsg( "Asked for names of empty list" );
        return {};
    }

    std::vector<std::string> names;
    names.reserve( parts.size() );
    for( size_t i = 0; i < parts.size(); ++i ) {
        const body_part part = parts[i];
        if( i + 1 < parts.size() && parts[i + 1] == static_cast<body_part>( bp_aiOther[part] ) ) {
            // Can combine two body parts (e.g. arms)
            names.push_back( body_part_name_accusative( part, 2 ) );
            ++i;
        } else {
            names.push_back( body_part_name_accusative( part ) );
        }
    }

    return enumerate_as_string( names );
}

void draw_mid_pane( const catacurses::window &w_sort_middle,
                    std::list<item>::const_iterator const worn_item_it,
                    const Character &c, int tabindex )
{
    const int win_width = getmaxx( w_sort_middle );
    const size_t win_height = static_cast<size_t>( getmaxy( w_sort_middle ) );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    size_t i = fold_and_print( w_sort_middle, point( 1, 0 ), win_width - 1, c_white,
                               worn_item_it->type_name( 1 ) ) - 1;
    std::vector<std::string> props = clothing_properties( *worn_item_it, win_width - 3, c );
    nc_color color = c_light_gray;
    for( std::string &iter : props ) {
        print_colored_text( w_sort_middle, point( 2, ++i ), color, c_light_gray, iter );
    }

    std::vector<std::string> prot = clothing_protection( *worn_item_it, win_width - 3 );
    if( i + prot.size() < win_height ) {
        for( std::string &iter : prot ) {
            print_colored_text( w_sort_middle, point( 2, ++i ), color, c_light_gray, iter );
        }
    } else {
        return;
    }

    i++;
    std::vector<std::string> layer_desc = foldstring( clothing_layer( *worn_item_it ), win_width );
    if( i + layer_desc.size() < win_height && !clothing_layer( *worn_item_it ).empty() ) {
        for( std::string &iter : layer_desc ) {
            mvwprintz( w_sort_middle, point( 0, ++i ), c_light_blue, iter );
        }
    }

    i++;
    std::vector<std::string> desc = clothing_flags_description( *worn_item_it );
    if( !desc.empty() ) {
        for( size_t j = 0; j < desc.size() && i + j < win_height; ++j ) {
            i += fold_and_print( w_sort_middle, point( 0, i ), win_width, c_light_blue, desc[j] );
        }
    }

    const item_penalties penalties = get_item_penalties( worn_item_it, c, tabindex );

    if( !penalties.body_parts_with_stacking_penalty.empty() ) {
        std::string layer_description = [&]() {
            switch( worn_item_it->get_layer() ) {
                case PERSONAL_LAYER:
                    return _( "in your <color_light_blue>personal aura</color>" );
                case UNDERWEAR_LAYER:
                    return _( "<color_light_blue>close to your skin</color>" );
                case REGULAR_LAYER:
                    return _( "of <color_light_blue>normal</color> clothing" );
                case WAIST_LAYER:
                    return _( "on your <color_light_blue>waist</color>" );
                case OUTER_LAYER:
                    return _( "of <color_light_blue>outer</color> clothing" );
                case BELTED_LAYER:
                    return _( "<color_light_blue>strapped</color> to you" );
                case AURA_LAYER:
                    return _( "an <color_light_blue>aura</color> around you" );
                default:
                    debugmsg( "Unexpected layer" );
                    return "";
            }
        }
        ();
        std::string body_parts =
            body_part_names( penalties.body_parts_with_stacking_penalty );
        std::string message =
            string_format(
                ngettext( "Wearing multiple items %s on your "
                          "<color_light_red>%s</color> is adding encumbrance there.",
                          "Wearing multiple items %s on your "
                          "<color_light_red>%s</color> is adding encumbrance there.",
                          penalties.body_parts_with_stacking_penalty.size() ),
                layer_description, body_parts
            );
        i += fold_and_print( w_sort_middle, point( 0, i ), win_width, c_light_gray, message );
    }

    if( !penalties.body_parts_with_out_of_order_penalty.empty() ) {
        std::string body_parts =
            body_part_names( penalties.body_parts_with_out_of_order_penalty );
        std::string message;

        if( penalties.bad_items_within.empty() ) {
            message = string_format(
                          ngettext( "Wearing this outside items it would normally be beneath "
                                    "is adding encumbrance to your <color_light_red>%s</color>.",
                                    "Wearing this outside items it would normally be beneath "
                                    "is adding encumbrance to your <color_light_red>%s</color>.",
                                    penalties.body_parts_with_out_of_order_penalty.size() ),
                          body_parts
                      );
        } else {
            std::string bad_item_name = *penalties.bad_items_within.begin();
            message = string_format(
                          ngettext( "Wearing this outside your <color_light_blue>%s</color> "
                                    "is adding encumbrance to your <color_light_red>%s</color>.",
                                    "Wearing this outside your <color_light_blue>%s</color> "
                                    "is adding encumbrance to your <color_light_red>%s</color>.",
                                    penalties.body_parts_with_out_of_order_penalty.size() ),
                          bad_item_name, body_parts
                      );
        }
        // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
        i += fold_and_print( w_sort_middle, point( 0, i ), win_width, c_light_gray, message );
    }
}

std::string clothing_layer( const item &worn_item )
{
    std::string layer;

    if( worn_item.has_flag( "PERSONAL" ) ) {
        layer = _( "This is in your personal aura." );
    } else if( worn_item.has_flag( "SKINTIGHT" ) ) {
        layer = _( "This is worn next to the skin." );
    } else if( worn_item.has_flag( "WAIST" ) ) {
        layer = _( "This is worn on or around your waist." );
    } else if( worn_item.has_flag( "OUTER" ) ) {
        layer = _( "This is worn over your other clothes." );
    } else if( worn_item.has_flag( "BELTED" ) ) {
        layer = _( "This is strapped onto you." );
    } else if( worn_item.has_flag( "AURA" ) ) {
        layer = _( "This is an aura around you." );
    }

    return layer;
}

std::vector<std::string> clothing_properties(
    const item &worn_item, const int width, const Character &c )
{
    std::vector<std::string> props;
    props.reserve( 5 );

    const std::string space = "  ";
    props.push_back( string_format( "<color_c_green>[%s]</color>", _( "Properties" ) ) );
    props.push_back( name_and_value( space + _( "Coverage:" ),
                                     string_format( "%3d", worn_item.get_coverage() ), width ) );
    props.push_back( name_and_value( space + _( "Encumbrance:" ),
                                     string_format( "%3d", worn_item.get_encumber( c ) ),
                                     width ) );
    props.push_back( name_and_value( space + _( "Warmth:" ),
                                     string_format( "%3d", worn_item.get_warmth() ), width ) );
    props.push_back( name_and_value( space + string_format( _( "Storage (%s):" ), volume_units_abbr() ),
                                     format_volume( worn_item.get_storage() ), width ) );
    return props;
}

std::vector<std::string> clothing_protection( const item &worn_item, const int width )
{
    std::vector<std::string> prot;
    prot.reserve( 4 );

    const std::string space = "  ";
    prot.push_back( string_format( "<color_c_green>[%s]</color>", _( "Protection" ) ) );
    prot.push_back( name_and_value( space + _( "Bash:" ),
                                    string_format( "%3d", static_cast<int>( worn_item.bash_resist() ) ), width ) );
    prot.push_back( name_and_value( space + _( "Cut:" ),
                                    string_format( "%3d", static_cast<int>( worn_item.cut_resist() ) ), width ) );
    prot.push_back( name_and_value( space + _( "Environmental:" ),
                                    string_format( "%3d", static_cast<int>( worn_item.get_env_resist() ) ), width ) );
    return prot;
}

std::vector<std::string> clothing_flags_description( const item &worn_item )
{
    std::vector<std::string> description_stack;

    if( worn_item.has_flag( "FIT" ) ) {
        description_stack.push_back( _( "It fits you well." ) );
    } else if( worn_item.has_flag( "VARSIZE" ) ) {
        description_stack.push_back( _( "It could be refitted." ) );
    }

    if( worn_item.has_flag( "HOOD" ) ) {
        description_stack.push_back( _( "It has a hood." ) );
    }
    if( worn_item.has_flag( "POCKETS" ) ) {
        description_stack.push_back( _( "It has pockets." ) );
    }
    if( worn_item.has_flag( "WATERPROOF" ) ) {
        description_stack.push_back( _( "It is waterproof." ) );
    }
    if( worn_item.has_flag( "WATER_FRIENDLY" ) ) {
        description_stack.push_back( _( "It is water friendly." ) );
    }
    if( worn_item.has_flag( "FANCY" ) ) {
        description_stack.push_back( _( "It looks fancy." ) );
    }
    if( worn_item.has_flag( "SUPER_FANCY" ) ) {
        description_stack.push_back( _( "It looks really fancy." ) );
    }
    if( worn_item.has_flag( "FLOTATION" ) ) {
        description_stack.push_back( _( "You will not drown today." ) );
    }
    if( worn_item.has_flag( "OVERSIZE" ) ) {
        description_stack.push_back( _( "It is very bulky." ) );
    }
    if( worn_item.has_flag( "SWIM_GOGGLES" ) ) {
        description_stack.push_back( _( "It helps you to see clearly underwater." ) );
    }
    if( worn_item.has_flag( "SEMITANGIBLE" ) ) {
        description_stack.push_back( _( "It can occupy the same space as other things." ) );
    }

    return description_stack;
}

} //namespace

struct layering_item_info {
    item_penalties penalties;
    int encumber;
    std::string name;

    // Operator overload required to leverage vector equality operator.
    bool operator ==( const layering_item_info &o ) const {
        // This is used to merge e.g. both arms into one entry when their items
        // are equivalent.  For that purpose we don't care about the exact
        // penalities because they will list different body parts; we just
        // check that the badness is the same (which is all that matters for
        // rendering the right-hand list).
        return this->penalties.badness() == o.penalties.badness() &&
               this->encumber == o.encumber &&
               this->name == o.name;
    }
};

static std::vector<layering_item_info> items_cover_bp( const Character &c, int bp )
{
    std::vector<layering_item_info> s;
    for( auto elem_it = c.worn.begin(); elem_it != c.worn.end(); ++elem_it ) {
        if( elem_it->covers( static_cast<body_part>( bp ) ) ) {
            s.push_back( { get_item_penalties( elem_it, c, bp ),
                           elem_it->get_encumber( c ),
                           elem_it->tname()
                         } );
        }
    }
    return s;
}

static void draw_grid( const catacurses::window &w, int left_pane_w, int mid_pane_w )
{
    const int win_w = getmaxx( w );
    const int win_h = getmaxy( w );

    draw_border( w );
    mvwhline( w, point( 1, 2 ), 0, win_w - 2 );
    mvwvline( w, point( left_pane_w + 1, 3 ), 0, win_h - 4 );
    mvwvline( w, point( left_pane_w + mid_pane_w + 2, 3 ), 0, win_h - 4 );

    // intersections
    mvwputch( w, point( 0, 2 ), BORDER_COLOR, LINE_XXXO );
    mvwputch( w, point( win_w - 1, 2 ), BORDER_COLOR, LINE_XOXX );
    mvwputch( w, point( left_pane_w + 1, 2 ), BORDER_COLOR, LINE_OXXX );
    mvwputch( w, point( left_pane_w + 1, win_h - 1 ), BORDER_COLOR, LINE_XXOX );
    mvwputch( w, point( left_pane_w + mid_pane_w + 2, 2 ), BORDER_COLOR, LINE_OXXX );
    mvwputch( w, point( left_pane_w + mid_pane_w + 2, win_h - 1 ), BORDER_COLOR, LINE_XXOX );

    wrefresh( w );
}

void player::sort_armor()
{
    /* Define required height of the right pane:
    * + 3 - horizontal lines;
    * + 1 - caption line;
    * + 2 - innermost/outermost string lines;
    * + num_bp - sub-categories (torso, head, eyes, etc.);
    * + 1 - gap;
    * number of lines required for displaying all items is calculated dynamically,
    * because some items can have multiple entries (i.e. cover a few parts of body).
    */

    int req_right_h = 3 + 1 + 2 + num_bp + 1;
    for( const body_part cover : all_body_parts ) {
        for( const item &elem : worn ) {
            if( elem.covers( cover ) ) {
                req_right_h++;
            }
        }
    }

    /* Define required height of the mid pane:
    * + 3 - horizontal lines;
    * + 1 - caption line;
    * + 8 - general properties
    * + 13 - ASSUMPTION: max possible number of flags @ item
    * + num_bp+1 - warmth & enc block
    */
    const int req_mid_h = 3 + 1 + 8 + 13 + num_bp + 1;

    const int win_h = std::min( TERMY, std::max( FULL_SCREEN_HEIGHT,
                                std::max( req_right_h, req_mid_h ) ) );
    const int win_w = FULL_SCREEN_WIDTH + ( TERMX - FULL_SCREEN_WIDTH ) * 3 / 4;
    const int win_x = TERMX / 2 - win_w / 2;
    const int win_y = TERMY / 2 - win_h / 2;

    int cont_h   = win_h - 4;
    int left_w   = ( win_w - 4 ) / 3;
    int right_w  = left_w;
    int middle_w = ( win_w - 4 ) - left_w - right_w;

    int tabindex = num_bp;
    const int tabcount = num_bp + 1;

    int leftListIndex  = 0;
    int leftListOffset = 0;
    int selected       = -1;

    int rightListOffset = 0;

    std::vector<std::list<item>::iterator> tmp_worn;
    std::array<std::string, 13> armor_cat = {{
            _( "Torso" ), _( "Head" ), _( "Eyes" ), _( "Mouth" ), _( "L. Arm" ), _( "R. Arm" ),
            _( "L. Hand" ), _( "R. Hand" ), _( "L. Leg" ), _( "R. Leg" ), _( "L. Foot" ),
            _( "R. Foot" ), _( "All" )
        }
    };

    // Layout window
    catacurses::window w_sort_armor = catacurses::newwin( win_h, win_w, point( win_x, win_y ) );
    draw_grid( w_sort_armor, left_w, middle_w );
    // Subwindows (between lines)
    catacurses::window w_sort_cat = catacurses::newwin( 1, win_w - 4, point( win_x + 2, win_y + 1 ) );
    catacurses::window w_sort_left = catacurses::newwin( cont_h, left_w,   point( win_x + 1,
                                     win_y + 3 ) );
    catacurses::window w_sort_middle = catacurses::newwin( cont_h - num_bp - 1, middle_w,
                                       point( win_x + left_w + 2, win_y + 3 ) );
    catacurses::window w_sort_right = catacurses::newwin( cont_h, right_w,
                                      point( win_x + left_w + middle_w + 3, win_y + 3 ) );
    catacurses::window w_encumb = catacurses::newwin( num_bp + 1, middle_w,
                                  point( win_x + left_w + 2, win_y + 3 + cont_h - num_bp - 1 ) );

    input_context ctxt( "SORT_ARMOR" );
    ctxt.register_cardinal();
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "MOVE_ARMOR" );
    ctxt.register_action( "CHANGE_SIDE" );
    ctxt.register_action( "ASSIGN_INVLETS" );
    ctxt.register_action( "SORT_ARMOR" );
    ctxt.register_action( "EQUIP_ARMOR" );
    ctxt.register_action( "EQUIP_ARMOR_HERE" );
    ctxt.register_action( "REMOVE_ARMOR" );
    ctxt.register_action( "USAGE_HELP" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    auto do_return_entry = []() {
        g->u.assign_activity( activity_id( "ACT_ARMOR_LAYERS" ), 0 );
        g->u.activity.auto_resume = true;
        g->u.activity.moves_left = INT_MAX;
    };

    bool exit = false;
    while( !exit ) {
        if( is_player() ) {
            // Totally hoisted this from advanced_inv
            if( g->u.moves < 0 ) {
                do_return_entry();
                return;
            }
        } else {
            // Player is sorting NPC's armor here
            if( rl_dist( g->u.pos(), pos() ) > 1 ) {
                add_msg_if_npc( m_bad, _( "%s is too far to sort armor." ), name );
                return;
            }
            if( attitude_to( g->u ) != Creature::A_FRIENDLY ) {
                add_msg_if_npc( m_bad, _( "%s is not friendly!" ), name );
                return;
            }
        }
        werase( w_sort_cat );
        werase( w_sort_left );
        werase( w_sort_middle );
        werase( w_sort_right );
        werase( w_encumb );

        // top bar
        wprintz( w_sort_cat, c_white, _( "Sort Armor" ) );
        wprintz( w_sort_cat, c_yellow, "  << %s >>", armor_cat[tabindex] );
        right_print( w_sort_cat, 0, 0, c_white, string_format(
                         _( "Press %s for help.  Press %s to change keybindings." ),
                         ctxt.get_desc( "USAGE_HELP" ),
                         ctxt.get_desc( "HELP_KEYBINDINGS" ) ) );

        // Create ptr list of items to display
        tmp_worn.clear();
        if( tabindex == num_bp ) { // All
            for( auto it = worn.begin(); it != worn.end(); ++it ) {
                tmp_worn.push_back( it );
            }
        } else { // bp_*
            body_part bp = static_cast<body_part>( tabindex );
            for( auto it = worn.begin(); it != worn.end(); ++it ) {
                if( it->covers( bp ) ) {
                    tmp_worn.push_back( it );
                }
            }
        }
        int leftListSize = ( static_cast<int>( tmp_worn.size() ) < cont_h - 2 ) ? static_cast<int>
                           ( tmp_worn.size() ) : cont_h - 2;

        // Ensure leftListIndex is in bounds
        int new_index_upper_bound = std::max( 0, static_cast<int>( tmp_worn.size() ) - 1 );
        leftListIndex = std::min( leftListIndex, new_index_upper_bound );

        // Left header
        mvwprintz( w_sort_left, point_zero, c_light_gray, _( "(Innermost)" ) );
        right_print( w_sort_left, 0, 0, c_light_gray, string_format( _( "Storage (%s)" ),
                     volume_units_abbr() ) );
        // Left list
        for( int drawindex = 0; drawindex < leftListSize; drawindex++ ) {
            int itemindex = leftListOffset + drawindex;

            if( itemindex == leftListIndex ) {
                mvwprintz( w_sort_left, point( 0, drawindex + 1 ), c_yellow, ">>" );
            }

            std::string worn_armor_name = tmp_worn[itemindex]->tname();
            item_penalties const penalties =
                get_item_penalties( tmp_worn[itemindex], *this, tabindex );

            const int offset_x = ( itemindex == selected ) ? 3 : 2;
            trim_and_print( w_sort_left, point( offset_x, drawindex + 1 ), left_w - offset_x - 3,
                            penalties.color_for_stacking_badness(), worn_armor_name );
            right_print( w_sort_left, drawindex + 1, 0, c_light_gray,
                         format_volume( tmp_worn[itemindex]->get_storage() ) );
        }

        // Left footer
        mvwprintz( w_sort_left, point( 0, cont_h - 1 ), c_light_gray, _( "(Outermost)" ) );
        if( leftListSize > static_cast<int>( tmp_worn.size() ) ) {
            // TODO: replace it by right_print()
            mvwprintz( w_sort_left, point( left_w - utf8_width( _( "<more>" ) ), cont_h - 1 ),
                       c_light_blue, _( "<more>" ) );
        }
        if( leftListSize == 0 ) {
            // TODO: replace it by right_print()
            mvwprintz( w_sort_left, point( left_w - utf8_width( _( "<empty>" ) ), cont_h - 1 ),
                       c_light_blue, _( "<empty>" ) );
        }

        // Items stats
        if( leftListSize > 0 ) {
            draw_mid_pane( w_sort_middle, tmp_worn[leftListIndex], *this, tabindex );
        } else {
            // NOLINTNEXTLINE(cata-use-named-point-constants)
            fold_and_print( w_sort_middle, point( 1, 0 ), middle_w - 1, c_white,
                            _( "Nothing to see here!" ) );
        }

        mvwprintz( w_encumb, point_east, c_white, _( "Encumbrance and Warmth" ) );
        print_encumbrance( w_encumb, -1, ( leftListSize > 0 ) ? &*tmp_worn[leftListIndex] : nullptr );

        // Right header
        mvwprintz( w_sort_right, point_zero, c_light_gray, _( "(Innermost)" ) );
        right_print( w_sort_right, 0, 0, c_light_gray, _( "Encumbrance" ) );

        // Right list
        int rightListSize = 0;
        for( int cover = 0, pos = 1; cover < num_bp; cover++ ) {
            bool combined = false;
            if( cover > 3 && cover % 2 == 0 &&
                items_cover_bp( *this, cover ) == items_cover_bp( *this, cover + 1 ) ) {
                combined = true;
            }
            if( rightListSize >= rightListOffset && pos <= cont_h - 2 ) {
                mvwprintz( w_sort_right, point( 1, pos ), ( cover == tabindex ? c_yellow : c_white ),
                           "%s:", body_part_name_as_heading( all_body_parts[cover], combined ? 2 : 1 ) );
                pos++;
            }
            rightListSize++;
            for( layering_item_info &elem : items_cover_bp( *this, cover ) ) {
                if( rightListSize >= rightListOffset && pos <= cont_h - 2 ) {
                    nc_color color = elem.penalties.color_for_stacking_badness();
                    trim_and_print( w_sort_right, point( 2, pos ), right_w - 5, color,
                                    elem.name );
                    char plus = elem.penalties.badness() > 0 ? '+' : ' ';
                    mvwprintz( w_sort_right, point( right_w - 4, pos ), c_light_gray, "%3d%c",
                               elem.encumber, plus );
                    pos++;
                }
                rightListSize++;
            }
            if( combined ) {
                cover++;
            }
        }

        // Right footer
        mvwprintz( w_sort_right, point( 0, cont_h - 1 ), c_light_gray, _( "(Outermost)" ) );
        if( rightListSize > cont_h - 2 ) {
            // TODO: replace it by right_print()
            mvwprintz( w_sort_right, point( right_w - utf8_width( _( "<more>" ) ), cont_h - 1 ), c_light_blue,
                       _( "<more>" ) );
        }
        // F5
        wrefresh( w_sort_cat );
        wrefresh( w_sort_left );
        wrefresh( w_sort_middle );
        wrefresh( w_sort_right );
        wrefresh( w_encumb );

        const std::string action = ctxt.handle_input();
        if( is_npc() && action == "ASSIGN_INVLETS" ) {
            // It doesn't make sense to assign invlets to NPC items
            continue;
        }

        // Helper function for moving items in the list
        auto shift_selected_item = [&]() {
            if( selected >= 0 ) {
                std::list<item>::iterator to = tmp_worn[leftListIndex];
                if( leftListIndex > selected ) {
                    ++to;
                }
                worn.splice( to, worn, tmp_worn[selected] );
                selected = leftListIndex;
                reset_encumbrance();
            }
        };

        if( action == "UP" && leftListSize > 0 ) {
            leftListIndex--;
            if( leftListIndex < 0 ) {
                leftListIndex = tmp_worn.size() - 1;
            }

            // Scrolling logic
            leftListOffset = ( leftListIndex < leftListOffset ) ? leftListIndex : leftListOffset;
            if( !( ( leftListIndex >= leftListOffset ) &&
                   ( leftListIndex < leftListOffset + leftListSize ) ) ) {
                leftListOffset = leftListIndex - leftListSize + 1;
                leftListOffset = ( leftListOffset > 0 ) ? leftListOffset : 0;
            }

            shift_selected_item();
        } else if( action == "DOWN" && leftListSize > 0 ) {
            leftListIndex = ( leftListIndex + 1 ) % tmp_worn.size();

            // Scrolling logic
            if( !( ( leftListIndex >= leftListOffset ) &&
                   ( leftListIndex < leftListOffset + leftListSize ) ) ) {
                leftListOffset = leftListIndex - leftListSize + 1;
                leftListOffset = ( leftListOffset > 0 ) ? leftListOffset : 0;
            }

            shift_selected_item();
        } else if( action == "LEFT" ) {
            tabindex--;
            if( tabindex < 0 ) {
                tabindex = tabcount - 1;
            }
            leftListIndex = leftListOffset = 0;
            selected = -1;
        } else if( action == "RIGHT" ) {
            tabindex = ( tabindex + 1 ) % tabcount;
            leftListIndex = leftListOffset = 0;
            selected = -1;
        } else if( action == "NEXT_TAB" ) {
            rightListOffset++;
            if( rightListOffset + cont_h - 2 > rightListSize ) {
                rightListOffset = rightListSize - cont_h + 2;
            }
        } else if( action == "PREV_TAB" ) {
            rightListOffset--;
            if( rightListOffset < 0 ) {
                rightListOffset = 0;
            }
        } else if( action == "MOVE_ARMOR" ) {
            if( selected >= 0 ) {
                selected = -1;
            } else {
                selected = leftListIndex;
            }
        } else if( action == "CHANGE_SIDE" ) {
            if( leftListIndex < static_cast<int>( tmp_worn.size() ) && tmp_worn[leftListIndex]->is_sided() ) {
                if( g->u.query_yn( _( "Swap side for %s?" ),
                                   colorize( tmp_worn[leftListIndex]->tname(),
                                             tmp_worn[leftListIndex]->color_in_inventory() ) ) ) {
                    change_side( *tmp_worn[leftListIndex] );
                    wrefresh( w_sort_armor );
                }
            }
        } else if( action == "SORT_ARMOR" ) {
            // Copy to a vector because stable_sort requires random-access
            // iterators
            std::vector<item> worn_copy( worn.begin(), worn.end() );
            std::stable_sort( worn_copy.begin(), worn_copy.end(),
            []( const item & l, const item & r ) {
                return l.get_layer() < r.get_layer();
            }
                            );
            std::copy( worn_copy.begin(), worn_copy.end(), worn.begin() );
            reset_encumbrance();
        } else if( action == "EQUIP_ARMOR" ) {
            // filter inventory for all items that are armor/clothing
            item_location loc = game_menus::inv::wear( *this );

            // only equip if something valid selected!
            if( loc ) {
                // wear the item
                cata::optional<std::list<item>::iterator> new_equip_it =
                    wear( this->i_at( loc.obtain( *this ) ) );
                if( new_equip_it ) {
                    body_part bp = static_cast<body_part>( tabindex );
                    if( tabindex == num_bp || ( **new_equip_it ).covers( bp ) ) {
                        // Set ourselves up to be pointing at the new item
                        // TODO: This doesn't work yet because we don't save our
                        // state through other activities, but that's a thing
                        // that would be nice to do.
                        leftListIndex =
                            std::count_if( worn.begin(), *new_equip_it,
                        [&]( const item & i ) {
                            return tabindex == num_bp || i.covers( bp );
                        } );
                    }
                } else if( is_npc() ) {
                    // TODO: Pass the reason here
                    popup( _( "Can't put this on!" ) );
                }
            }
            draw_grid( w_sort_armor, left_w, middle_w );
        } else if( action == "EQUIP_ARMOR_HERE" ) {
            // filter inventory for all items that are armor/clothing
            item_location loc = game_menus::inv::wear( *this );

            // only equip if something valid selected!
            if( loc ) {
                // wear the item
                if( cata::optional<std::list<item>::iterator> new_equip_it =
                        wear( this->i_at( loc.obtain( *this ) ) ) ) {
                    // save iterator to cursor's position
                    std::list<item>::iterator cursor_it = tmp_worn[leftListIndex];
                    // reorder `worn` vector to place new item at cursor
                    worn.splice( cursor_it, worn, *new_equip_it );
                } else if( is_npc() ) {
                    // TODO: Pass the reason here
                    popup( _( "Can't put this on!" ) );
                }
            }
            draw_grid( w_sort_armor, left_w, middle_w );
        } else if( action == "REMOVE_ARMOR" ) {
            // query (for now)
            if( leftListIndex < static_cast<int>( tmp_worn.size() ) ) {
                if( g->u.query_yn( _( "Remove selected armor?" ) ) ) {
                    do_return_entry();
                    // remove the item, asking to drop it if necessary
                    takeoff( *tmp_worn[leftListIndex] );
                    if( !g->u.has_activity( activity_id( "ACT_ARMOR_LAYERS" ) ) ) {
                        // An activity has been created to take off the item;
                        // we must surrender control until it is done.
                        return;
                    }
                    g->u.cancel_activity();
                    draw_grid( w_sort_armor, left_w, middle_w );
                    wrefresh( w_sort_armor );
                    selected = -1;
                }
            }
        } else if( action == "ASSIGN_INVLETS" ) {
            // prompt first before doing this (yes, yes, more popups...)
            if( query_yn( _( "Reassign invlets for armor?" ) ) ) {
                // Start with last armor (the most unimportant one?)
                auto iiter = inv_chars.rbegin();
                auto witer = worn.rbegin();
                while( witer != worn.rend() && iiter != inv_chars.rend() ) {
                    const char invlet = *iiter;
                    item &w = *witer;
                    if( invlet == w.invlet ) {
                        ++witer;
                    } else if( invlet_to_position( invlet ) != INT_MIN ) {
                        ++iiter;
                    } else {
                        inv.reassign_item( w, invlet );
                        ++witer;
                        ++iiter;
                    }
                }
            }
        } else if( action == "USAGE_HELP" ) {
            popup_getkey( _( "Use the arrow- or keypad keys to navigate the left list.\n"
                             "[%s] to select highlighted armor for reordering.\n"
                             "[%s] / [%s] to scroll the right list.\n"
                             "[%s] to assign special inventory letters to clothing.\n"
                             "[%s] to change the side on which item is worn.\n"
                             "[%s] to sort armor into natural layer order.\n"
                             "[%s] to equip a new item.\n"
                             "[%s] to equip a new item at the currently selected position.\n"
                             "[%s] to remove selected armor from oneself.\n"
                             "\n"
                             "[Encumbrance and Warmth] explanation:\n"
                             "The first number is the summed encumbrance from all clothing on that bodypart.\n"
                             "The second number is an additional encumbrance penalty caused by wearing multiple items "
                             "on one of the bodypart's layers or wearing items outside of other items they would "
                             "normally be work beneath (e.g. a shirt over a backpack).\n"
                             "The sum of these values is the effective encumbrance value your character has for that bodypart." ),
                          ctxt.get_desc( "MOVE_ARMOR" ),
                          ctxt.get_desc( "PREV_TAB" ),
                          ctxt.get_desc( "NEXT_TAB" ),
                          ctxt.get_desc( "ASSIGN_INVLETS" ),
                          ctxt.get_desc( "CHANGE_SIDE" ),
                          ctxt.get_desc( "SORT_ARMOR" ),
                          ctxt.get_desc( "EQUIP_ARMOR" ),
                          ctxt.get_desc( "EQUIP_ARMOR_HERE" ),
                          ctxt.get_desc( "REMOVE_ARMOR" )
                        );
            draw_grid( w_sort_armor, left_w, middle_w );
        } else if( action == "HELP_KEYBINDINGS" ) {
            draw_grid( w_sort_armor, left_w, middle_w );
        } else if( action == "QUIT" ) {
            exit = true;
        }
    }
}
