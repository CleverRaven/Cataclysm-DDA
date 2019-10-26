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


std::string clothing_layer( const item &worn_item )
{
    std::string layer;

    if( worn_item.has_flag( "PERSONAL" ) ) {
        layer = _( "(p.aur)" );
    } else if( worn_item.has_flag( "SKINTIGHT" ) ) {
        layer = _( "(skin)" );
    } else if( worn_item.has_flag( "WAIST" ) ) {
        layer = _( "(waist)" );
    } else if( worn_item.has_flag( "OUTER" ) ) {
        layer = _( "(outer)" );
    } else if( worn_item.has_flag( "BELTED" ) ) {
        layer = _( "(strap)" );
    } else if( worn_item.has_flag( "AURA" ) ) {
        layer = _( "(aura)" );
    }

    return layer;
}
enum sort_armor_columns : int {
    column_cond,
    column_encumb,
    column_storage,
    column_warmth,
    column_divider,
    column_coverage,
    column_bash,
    column_cut,
    column_acid,
    column_fire,
    column_env,
    column_num_entries
};
struct sort_armor_col_data {
    private:
        int width;
        std::string name;
        std::string description;
    public:
        sort_armor_col_data( int width, std::string name, std::string description ):
            width( width ), name( name ), description( description ) {
        }
        int get_width() {
            return std::max( utf8_width( _( name ) ) + 1, width );
        }
        std::string get_name() {
            return _( name );
        }
        std::string get_description() {
            return _( description );
        }
};
sort_armor_col_data get_col_data( int col_idx )
{
    static std::array< sort_armor_col_data, column_num_entries> col_data = { {
            {
                3, translate_marker( "condition" ),
                string_format( translate_marker( "clothing conditions:\n\t<color_c_red>x</color> - poor fit\n\t<color_c_red>f</color> - filthy\n\t<color_c_cyan>w</color> - waterproof" ) )
            },
            {8, translate_marker( "encumberance" ), translate_marker( "encumberance." )},
            {7, translate_marker( "storage" ), translate_marker( "storage." ) },
            {4, translate_marker( "warmth" ), translate_marker( "warmth." ) },
            {3, translate_marker( "  " ), translate_marker( "" )},
            {3, translate_marker( "coverage" ), translate_marker( "coverage." ) },
            {4, translate_marker( "bash" ), translate_marker( "bash protection." ) },
            {4, translate_marker( "cut" ), translate_marker( "cut protection." ) },
            {4, translate_marker( "acid" ), translate_marker( "acid protection." ) },
            {4, translate_marker( "fire" ), translate_marker( "fire protection." ) },
            {4, translate_marker( "environmental" ), translate_marker( "environmental protection." ) }
        }
    };
    return col_data[col_idx];
}
class layering_item_info
{
    protected:
        item_penalties penalties;
        int encumber;
        std::string name;
        std::list<item>::iterator itm_iter;

    public:
        layering_item_info( std::list<item>::iterator itm_i, player &p, body_part bp ) {
            penalties = get_item_penalties( itm_i, p, bp );
            encumber = itm_i->get_encumber( p );
            name = itm_i->tname();
            itm_iter = itm_i;
        }

        std::list<item>::iterator get_iter() {
            return itm_iter;
        }

        bool operator ==( const layering_item_info &o ) const {
            return itm_iter == o.itm_iter;
        }

        int get_col_value( sort_armor_columns col ) {
            const item &itm = *itm_iter;
            switch( col ) {
                case column_warmth:
                    return itm.get_warmth();
                case column_coverage:
                    return itm.get_coverage();
                case column_bash:
                    return itm.bash_resist();
                case column_cut:
                    return itm.cut_resist();
                case column_acid:
                    return itm.acid_resist();
                case column_fire:
                    return itm.fire_resist();
                case column_env:
                    return itm.get_env_resist();
                default:
                    return 0;
            }
        }
        void print_columnns( bool is_selected, const catacurses::window &w, int right_bound,
                             int cur_print_y, bool is_compact, int compact_display_tab ) {
            item itm = *itm_iter;
            nc_color print_col = is_selected ? hilite( c_white ) : c_light_gray;
            int cur_tab = 1;
            for( size_t col_idx = column_num_entries; col_idx-- > 0; ) {
                if( is_compact ) {
                    if( col_idx == column_divider ) {
                        cur_tab--;
                        continue;
                    }
                    if( cur_tab != compact_display_tab ) {
                        continue;
                    }
                }
                int col_w = get_col_data( col_idx ).get_width();
                right_bound -= col_w;
                switch( col_idx ) {
                    case column_cond: {
                        nc_color print_col2 = is_selected ? hilite( c_red ) : c_red;
                        if( is_selected ) { //fill whole line with color
                            mvwprintz( w, point( right_bound, cur_print_y ), print_col2, "%*s", col_w, "" );
                        }
                        int pos_x = right_bound + col_w - 1;
                        char cond;

                        print_col2 = is_selected ? hilite( c_red ) : c_red;
                        cond = ( itm.is_filthy() ? 'f' : ' ' );
                        mvwprintz( w, point( pos_x, cur_print_y ), print_col2, "%c", cond );
                        pos_x--;

                        print_col2 = is_selected ? hilite( c_red ) : c_red;
                        cond = ( itm.has_flag( "FIT" ) || !itm.has_flag( "VARSIZE" ) ) ? ' ' : 'x';
                        mvwprintz( w, point( pos_x, cur_print_y ), print_col2, "%c", cond );
                        pos_x--;

                        print_col2 = is_selected ? hilite( c_cyan ) : c_cyan;
                        cond = itm.has_flag( "WATERPROOF" ) ? 'w' : ' ';
                        mvwprintz( w, point( pos_x, cur_print_y ), print_col2, "%c", cond );
                        pos_x--;
                    }
                    break;
                    case column_encumb: {
                        nc_color print_col2 = penalties.color_for_stacking_badness();
                        if( is_selected ) {
                            print_col2 = hilite( print_col2 );
                        }
                        std::string s = string_format( "%d%c", encumber, penalties.badness() > 0 ? '+' : ' ' );
                        mvwprintz( w, point( right_bound, cur_print_y ), print_col2, "%*s", col_w, s );
                    }
                    break;
                    case column_storage: {
                        units::volume storage = itm.get_storage();
                        mvwprintz( w, point( right_bound, cur_print_y ), print_col, "%*s", col_w,
                                   storage > 0_ml ? format_volume( itm.get_storage() ) : "" );
                    }
                    break;
                    case column_divider: {
                        if( is_selected ) { //fill whole line with color
                            mvwprintz( w, point( right_bound, cur_print_y ), print_col, "%*s", col_w, "" );
                        }
                        nc_color print_col2 = is_selected ? hilite( c_light_gray ) : c_light_gray;
                        mvwputch( w, point( right_bound + col_w - 1, cur_print_y ), print_col2, LINE_XOXO );
                    }
                    break;
                    case column_env: {
                        int val = get_col_value( static_cast<sort_armor_columns>( col_idx ) );
                        mvwprintz( w, point( right_bound, cur_print_y ), print_col, "%*s", col_w, "" );
                        mvwprintz( w, point( right_bound, cur_print_y ), print_col, "%*d", 5, val );
                    }
                    break;
                    default: {
                        int val = get_col_value( static_cast<sort_armor_columns>( col_idx ) );
                        mvwprintz( w, point( right_bound, cur_print_y ), print_col, "%*d", col_w, val );
                    }
                    break;
                }
            }
            nc_color name_col = penalties.color_for_stacking_badness();
            if( is_selected ) {
                name_col = hilite( name_col );
            }
            mvwprintz( w, point( 2, cur_print_y ), c_dark_gray, clothing_layer( itm ) );
            if( is_selected ) { //fill whole line with color
                mvwprintz( w, point( 9, cur_print_y ), name_col, "%*s", right_bound - 9, "" );
            }
            trim_and_print( w, point( 9, cur_print_y ), right_bound - 9, name_col, name );
        }
        void set_aggregate_data( std::array<int, column_num_entries> &agg ) {
            for( size_t i = 0; i < column_num_entries; i++ ) {
                if( i == column_warmth ) {
                    agg[i] += get_col_value( static_cast<sort_armor_columns>( i ) );
                } else {
                    agg[i] += get_col_value( static_cast<sort_armor_columns>( i ) ) * get_col_value( column_coverage );
                }
            }
        }
};

class layering_group_info
{
    private:
        std::string name;
        std::array<int, column_num_entries> header_data;
        std::list<layering_item_info> items;
        body_part bp;
        player *p;
        int start_line;

        int divider_posn;

        //printing constatnts
        int x_bound;
        int max_y;
        bool is_compact;

        //printing vars
        int list_min_line;
        int list_selected_line;
        bool is_moving;
        int compact_display_tab;

    public:
        bool is_skipped = false;

        layering_group_info( body_part bp, player *p ) : bp( bp ), p( p ) { }

        void add_item( std::list<item>::iterator itm_i ) {
            items.emplace_back( itm_i, *p, bp );
        }
        // same for the duration of the menu
        void set_print_constats( int right_bound, int max_y, bool is_compact ) {
            this->x_bound = right_bound;
            this->max_y = max_y;
            this->is_compact = is_compact;
        }
        // depend on user input
        void set_print_vars( int list_selected_line, bool is_moving, int list_min_line,
                             int compact_display_tab ) {
            this->list_selected_line = list_selected_line;
            this->is_moving = is_moving;
            this->list_min_line = list_min_line;
            this->compact_display_tab = compact_display_tab;
        }

        void set_combined( bool combined ) {
            name = body_part_name_as_heading( all_body_parts[bp], combined ? 2 : 1 );
            is_skipped = false;
        }

        void insert_item( std::list<item> &worn, int from_line, int to_line ) {
            int from_idx = from_line - min_selectable();
            int to_idx = to_line - min_selectable();

            auto from_it = items.begin();
            std::advance( from_it, from_idx );

            auto to_it = items.begin();
            std::advance( to_it, to_idx );

            std::list<item>::iterator to = ( *to_it ).get_iter();
            if( to_idx > from_idx ) {
                ++to;
            }
            worn.splice( to, worn, ( *from_it ).get_iter() );
        }

        layering_item_info get_cur_item_info() {
            int cur_itm_idx = list_selected_line - min_selectable();
            auto itm_iter = items.begin();
            std::advance( itm_iter, cur_itm_idx );
            return *itm_iter;
        }

        size_t num_items() {
            return items.size();
        }
        size_t height() {
            return num_items() + 2;
        }
        void clear() {
            items.clear();
        }
        int min_selectable() {
            return start_line + 1;
        }
        int max_selectable() {
            return min_selectable() + num_items() - 1;
        }
        body_part get_bp() {
            return bp;
        }
        bool operator ==( const layering_group_info &o ) const {
            return items == o.items;
        }

        void print_header( bool is_selected, const catacurses::window &w, int cur_print_y ) {
            int right_bound = x_bound;
            nc_color print_col = is_selected ? hilite( c_white ) : c_light_gray;
            int cur_tab = 1;
            for( size_t col_idx = column_num_entries; col_idx-- > 0; ) {
                if( is_compact ) {
                    if( col_idx == column_divider ) {
                        cur_tab--;
                        continue;
                    }
                    if( cur_tab != compact_display_tab ) {
                        continue;
                    }
                }
                int col_w = get_col_data( col_idx ).get_width();
                right_bound -= col_w;
                switch( col_idx ) {
                    case column_cond:
                    case column_storage:
                    case column_coverage:
                        //skip, but keep highlight
                        mvwprintz( w, point( right_bound, cur_print_y ), print_col, "%*s", col_w, "" );
                        break;
                    case column_encumb: {
                        int size_hack = 0;
                        right_bound -= size_hack;

                        const auto enc_data = p->get_encumbrance();
                        const encumbrance_data &e = enc_data[bp];
                        nc_color print_col2 = encumb_color( e.encumbrance );
                        if( is_selected ) {
                            print_col2 = hilite( print_col2 );
                        }
                        //mvwprintz(w, point(right_bound, cur_print_y), print_col2, "%3d+%-3d", col_w, 15, 0);
                        std::string s = string_format( "%d+%d ", e.armor_encumbrance, e.layer_penalty );
                        mvwprintz( w, point( right_bound, cur_print_y ), print_col2, "%*s", col_w + size_hack, s );
                        break;
                    }
                    case column_divider: {
                        if( is_selected ) { //fill whole line with color
                            mvwprintz( w, point( right_bound, cur_print_y ), print_col, "%*s", col_w, "" );
                        }
                        nc_color print_col2 = is_selected ? hilite( c_light_gray ) : c_light_gray;
                        divider_posn = right_bound + col_w - 1;
                        mvwputch( w, point( divider_posn, cur_print_y ), print_col2, LINE_XOXO );
                    }
                    break;
                    case column_env: {
                        double val = static_cast<double>( header_data[col_idx] ) / 100;
                        int val_i = static_cast<int>( std::round( val ) );
                        mvwprintz( w, point( right_bound, cur_print_y ), print_col, "%*s", col_w, "" );
                        mvwprintz( w, point( right_bound, cur_print_y ), print_col, "%*d", 5, val_i );
                    }
                    break;
                    case column_warmth:
                        mvwprintz( w, point( right_bound, cur_print_y ), print_col, "%*d", col_w, header_data[col_idx] );
                        break;
                    default: {
                        double val = static_cast<double>( header_data[col_idx] ) / 100;
                        int val_i = static_cast<int>( std::round( val ) );
                        mvwprintz( w, point( right_bound, cur_print_y ), print_col, "%*d", col_w, val_i );
                    }
                    break;
                }
            }
            nc_color name_col = is_selected ? hilite( c_white ) : c_white;
            if( is_selected ) { //fill whole line with color
                mvwprintz( w, point( 2, cur_print_y ), name_col, "%*s", right_bound - 2, "" );
            }
            mvwprintz( w, point( 2, cur_print_y ), name_col, "%s:", name );
        }
        void print_data( int &cur_print_line, int &print_y, const catacurses::window &w ) {
            int header_y = print_y;
            header_data.fill( 0 );
            bool is_selected = false;
            bool can_print_header = false;

            start_line = cur_print_line;
            if( cur_print_line >= list_min_line && print_y < max_y ) {
                can_print_header = true;
                print_y++;
            }
            cur_print_line++;

            for( layering_item_info &elem : items ) {
                nc_color print_col = c_light_gray;
                if( list_selected_line == cur_print_line ) {
                    if( is_moving ) {
                        mvwprintz( w, point( 1, print_y ), c_yellow, ">" );
                    }
                    print_col = hilite( c_white );
                    is_selected = true;
                }
                if( cur_print_line >= list_min_line && print_y < max_y ) {
                    elem.print_columnns( ( list_selected_line == cur_print_line ), w, x_bound, print_y, is_compact,
                                         compact_display_tab );
                    print_y++;
                }
                cur_print_line++;

                elem.set_aggregate_data( header_data );
            }

            if( can_print_header ) { //need to print header after we aggreate data
                print_header( is_selected, w, header_y );
            }

            if( cur_print_line >= list_min_line && print_y < max_y ) {
                mvwhline( w, point( 1, print_y ), 0, x_bound - 1 );
                mvwputch( w, point( divider_posn, print_y ), c_light_gray, LINE_XXXX );
                print_y++;
            }
            cur_print_line++;
        }
};

} //namespace

void player::sort_armor()
{
    input_context ctxt( "SORT_ARMOR" );
    ctxt.register_updown();
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_action( "NEXT_TAB" );

    ctxt.register_action( "MOVE_ARMOR" );
    ctxt.register_action( "CHANGE_SIDE" );
    ctxt.register_action( "SORT_ARMOR" );
    ctxt.register_action( "EQUIP_ARMOR" );
    ctxt.register_action( "EQUIP_ARMOR_HERE" );
    ctxt.register_action( "TAKE_OFF" );
    ctxt.register_action( "DROP" );

    ctxt.register_action( "USAGE_HELP" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );

    auto do_return_entry = []() {
        g->u.assign_activity( activity_id( "ACT_ARMOR_LAYERS" ), 0 );
        g->u.activity.auto_resume = true;
        g->u.activity.moves_left = INT_MAX;
    };

    const int name_start_idx = 11;
    int max_name_w = 0;

    const int header_h = 3;
    const int footer_h = 6;

    //find required height and width
    int req_data_h = 0;
    for( int bp_idx = 0; bp_idx < num_bp; bp_idx++ ) {
        req_data_h += 2; // title + divider line

        bool same = true;
        bool check_same = ( bp_idx > 3 && bp_idx % 2 == 0 );
        for( const item &elem : worn ) {
            bool covers = elem.covers( static_cast<body_part>( bp_idx ) );
            if( covers ) {
                req_data_h++;
            }
            if( check_same &&
                ( covers != elem.covers( static_cast<body_part>( bp_idx + 1 ) ) ) ) {
                same = false;
            }

            int elem_w = utf8_width( elem.tname(), true );
            if( elem_w > max_name_w ) {
                max_name_w = elem_w;
            }
        }
        if( check_same && same ) {
            bp_idx++;
        }
    }
    int columns_w = 0;
    for( size_t i = 0; i < column_num_entries; i++ ) {
        columns_w += get_col_data( i ).get_width();
    }
    const int requested_h = header_h + req_data_h + footer_h;
    const int win_h = std::min( TERMY, requested_h );
    const int requested_w = name_start_idx + max_name_w + columns_w + 1;
    const int win_w = std::min( requested_w, TERMX );
    const int win_x = TERMX / 2 - win_w / 2;
    const int win_y = TERMY / 2 - win_h / 2;
    catacurses::window w_sort_armor = catacurses::newwin( win_h, win_w, point( win_x, win_y ) );

    bool is_compact = requested_w > win_w;
    int compact_display_tab = 0;

    scrollbar s;
    s.viewport_size( win_h - header_h - footer_h );
    s.offset_y( header_h );

    int data_max_y = win_h - footer_h;

    std::vector <layering_group_info> items_by_category;
    items_by_category.reserve( num_bp );
    for( const body_part bp : all_body_parts ) {
        items_by_category.push_back( layering_group_info( bp, this ) );
        items_by_category[bp].set_print_constats( win_w - 1, data_max_y, is_compact );
    }

    layering_group_info *cur_grp = nullptr;
    int min_print_line = 0;
    int selected_line = -1;
    bool is_moving = false;

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

        // set up data
        for( const body_part bp : all_body_parts ) {
            items_by_category[bp].clear();
        }
        for( auto elem_it = worn.begin(); elem_it != worn.end(); ++elem_it ) {
            for( const body_part bp : all_body_parts ) {
                if( !elem_it->covers( bp ) ) {
                    continue;
                }
                items_by_category[bp].add_item( elem_it );
            }
        }

        //window border
        werase( w_sort_armor );
        draw_border( w_sort_armor );
        mvwprintz( w_sort_armor, point_east, c_white, _( "Sort Armor" ) );
        int print_x = win_w;
        std::string msg = string_format( _( "<[%s] Columns Info>" ), ctxt.get_desc( "USAGE_HELP" ) );
        print_x -= utf8_width( msg ) + 1;
        mvwprintz( w_sort_armor, point( print_x, 0 ), c_white, msg );

        //column names
        int print_y = 1;
        int divider_posn = 0;
        int cur_tab = 1;
        for( size_t col_idx = column_num_entries, right_bound = win_w - 1; col_idx-- > 0; ) {
            if( is_compact ) {
                if( col_idx == column_divider ) {
                    cur_tab--;
                    continue;
                }
                if( cur_tab != compact_display_tab ) {
                    continue;
                }
            }

            sort_armor_col_data col_data = get_col_data( col_idx );
            int col_w = col_data.get_width();
            right_bound -= col_w;
            if( col_idx == column_divider ) {
                divider_posn = right_bound + col_w - 1;
                mvwputch( w_sort_armor, point( divider_posn, print_y ), c_light_gray, LINE_XOXO );
            } else {
                mvwprintz( w_sort_armor, point( right_bound, print_y ), c_light_gray, "%*s", col_w,
                           col_data.get_name() );
            }
        }

        //compact tab name
        if( is_compact ) {
            std::string tab_name;
            if( compact_display_tab == 0 ) {
                tab_name = _( "Info" );
            } else {
                tab_name = _( "Protection" );
            }
            mvwprintz( w_sort_armor, point( 3, print_y ), c_yellow, "<< %s >>", tab_name );
        }
        print_y += 1;

        // data top
        mvwhline( w_sort_armor, point( 1, print_y ), 0, win_w - 2 ); //middle
        mvwputch( w_sort_armor, point( 0, print_y ), c_light_gray, LINE_XXXO ); //left
        mvwputch( w_sort_armor, point( win_w - 1, print_y ), c_light_gray, LINE_XOXX ); //right
        mvwputch( w_sort_armor, point( divider_posn, print_y ), c_light_gray, LINE_XXXX ); //divider
        mvwprintz( w_sort_armor, point( 1, print_y ), c_light_gray, _( "(Innermost)" ) );
        print_y += 1;

        // data
        int cur_print_line = 0;
        const int data_start_y = print_y;

        for( int bp_idx = 0; bp_idx < num_bp; bp_idx++ ) {
            layering_group_info &l = items_by_category[bp_idx];

            bool combined = false;
            if( bp_idx > 3 && bp_idx % 2 == 0 && l == items_by_category[bp_idx + 1] ) {
                combined = true;
                bp_idx++;
                items_by_category[bp_idx].is_skipped = true;
            }

            if( cur_grp == nullptr && l.num_items() > 0 ) {
                cur_grp = &l;
                selected_line = cur_print_line + 1;
            }

            l.set_combined( combined );
            l.set_print_vars( selected_line, is_moving, min_print_line, compact_display_tab );
            l.print_data( cur_print_line, print_y, w_sort_armor );
        }

        if( cur_grp == nullptr || cur_grp->num_items() == 0 ) {
            add_msg_if_player( _( "You are not wearing any clothes." ) );
            add_msg_if_npc( _( "%s is not wearing any clothes." ), name );
            return;
        }

        // data bottom
        int footer_y = data_max_y;
        if( requested_h > win_h ) {
            s.content_size( cur_print_line - 1 );
            s.viewport_pos( min_print_line );
            s.apply( w_sort_armor );
        } else {
            footer_y = print_y - 1;
        }
        mvwhline( w_sort_armor, point( 1, footer_y ), 0, win_w - 2 ); //middle
        mvwputch( w_sort_armor, point( 0, footer_y ), c_light_gray, LINE_XXXO ); //left
        mvwputch( w_sort_armor, point( win_w - 1, footer_y ), c_light_gray, LINE_XOXX ); //right
        mvwputch( w_sort_armor, point( divider_posn, footer_y ), c_light_gray, LINE_XXOX ); //divider

        mvwprintz( w_sort_armor, point( 1, footer_y ), c_light_gray, _( "(Outermost)" ) );
        // footer
        const std::string s = get_encumbrance_description( cur_grp->get_bp(), false );
        fold_and_print( w_sort_armor, point( 1, footer_y + 1 ), win_w - 2, c_magenta, s );

        wrefresh( w_sort_armor );

        //helper functions

        // Get prev/next valid group - wrap if over boundaries and skip if combined.
        std::function<layering_group_info* ( layering_group_info *, int )> get_neighbour_grp = [&](
        layering_group_info * grp, int direction ) {
            int cur_idx = grp->get_bp() + direction;
            if( cur_idx >= static_cast<int>( items_by_category.size() ) ) {
                cur_idx = 0;
            } else if( cur_idx < 0 ) {
                cur_idx = items_by_category.size() - 1;
            }

            grp = &items_by_category[cur_idx];
            if( grp == cur_grp ) { //did a full wrap around without finding anythig
                return grp;
            } else if( grp->is_skipped || grp->num_items() < 1 ) {
                return get_neighbour_grp( grp, direction );
            } else {
                return grp;
            }
        };
        //check if need move the list (== change min_print_line)
        auto recalculate_top_line = [&]() {
            if( selected_line < min_print_line ) {
                min_print_line = selected_line;
            } else if( selected_line >= min_print_line + ( data_max_y - data_start_y ) ) {
                min_print_line = selected_line - ( data_max_y - data_start_y ) + 1;
            }
            //if we select top of the list and it happens to be top of the group, print the title as well
            if( selected_line == min_print_line && selected_line == cur_grp->min_selectable() ) {
                min_print_line -= 1;
            }
        };

        const std::string action = ctxt.handle_input();
        if( action == "UP" ) {
            int prev_line = selected_line--;
            if( selected_line < cur_grp->min_selectable() ) {
                if( is_moving ) {
                    selected_line = cur_grp->max_selectable();
                } else {
                    cur_grp = get_neighbour_grp( cur_grp, -1 );
                    selected_line = cur_grp->max_selectable();
                }
            }

            recalculate_top_line();
            if( is_moving ) {
                cur_grp->insert_item( worn, prev_line, selected_line );
                reset_encumbrance();
            }
        } else if( action == "DOWN" ) {
            int prev_line = selected_line++;
            if( selected_line > cur_grp->max_selectable() ) {
                if( is_moving ) {
                    selected_line = cur_grp->min_selectable();
                } else {
                    cur_grp = get_neighbour_grp( cur_grp, 1 );
                    selected_line = cur_grp->min_selectable();
                }
            }

            recalculate_top_line();
            if( is_moving ) {
                cur_grp->insert_item( worn, prev_line, selected_line );
                reset_encumbrance();
            }
        } else if( action == "PAGE_UP" ) {
            cur_grp = get_neighbour_grp( cur_grp, -1 );
            selected_line = cur_grp->min_selectable();
        } else if( action == "PAGE_DOWN" ) {
            cur_grp = get_neighbour_grp( cur_grp, 1 );
            selected_line = cur_grp->min_selectable();
        } else if( action == "NEXT_TAB" ) {
            compact_display_tab = ( compact_display_tab + 1 ) % 2;
        } else if( action == "MOVE_ARMOR" ) {
            is_moving = !is_moving;
        } else if( action == "SORT_ARMOR" ) {
            // Copy to a vector because stable_sort requires random-access iterators
            std::vector<item> worn_copy( worn.begin(), worn.end() );
            std::stable_sort( worn_copy.begin(), worn_copy.end(),
            []( const item & l, const item & r ) {
                return l.get_layer() < r.get_layer();
            }
                            );
            std::copy( worn_copy.begin(), worn_copy.end(), worn.begin() );
            reset_encumbrance();
        } else if( action == "CHANGE_SIDE" ) {
            auto cur_item_iter = cur_grp->get_cur_item_info().get_iter();
            if( cur_item_iter->is_sided() ) {
                if( g->u.query_yn( _( "Swap side for %s?" ),
                                   colorize( cur_item_iter->tname(),
                                             cur_item_iter->color_in_inventory() ) ) ) {
                    change_side( *cur_item_iter );
                }
            }
        } else if( action == "EQUIP_ARMOR" ) {
            // filter inventory for all items that are armor/clothing
            item_location loc = game_menus::inv::wear( *this );

            // only equip if something valid selected!
            if( loc ) {
                // wear the item
                cata::optional<std::list<item>::iterator> new_equip_it =
                    wear( i_at( loc.obtain( *this ) ) );
                if( new_equip_it ) {
                    //Do nothing
                } else if( is_npc() ) {
                    // TODO: Pass the reason here
                    popup( _( "Can't put this on!" ) );
                }
            }
        } else if( action == "EQUIP_ARMOR_HERE" ) {
            // filter inventory for all items that are armor/clothing
            item_location loc = game_menus::inv::wear( *this );

            // only equip if something valid selected!
            if( loc ) {
                // wear the item
                cata::optional<std::list<item>::iterator> new_equip_it =
                    wear( i_at( loc.obtain( *this ) ) );
                if( new_equip_it ) {
                    // save iterator to cursor's position
                    auto cur_item_iter = cur_grp->get_cur_item_info().get_iter();
                    // reorder `worn` vector to place new item at cursor
                    worn.splice( cur_item_iter, worn, *new_equip_it );
                } else if( is_npc() ) {
                    // TODO: Pass the reason here
                    popup( _( "Can't put this on!" ) );
                }
            }
        } else if( action == "TAKE_OFF" ) {
            if( g->u.query_yn( _( "Take off selected armor?" ) ) ) {
                do_return_entry();
                // remove the item, asking to drop it if necessary
                auto cur_item_iter = cur_grp->get_cur_item_info().get_iter();
                takeoff( *cur_item_iter );
                if( !g->u.has_activity( activity_id( "ACT_ARMOR_LAYERS" ) ) ) {
                    // An activity has been created to take off the item;
                    // we must surrender control until it is done.
                    return;
                }
                g->u.cancel_activity();
                // Reset when interacting with npc, this will mimic behavior as player
                cur_grp = nullptr;
                selected_line = 1;
            }
        } else if( action == "DROP" ) { //same as TAKE_OFF
            if( g->u.query_yn( _( "Drop selected armor?" ) ) ) {
                do_return_entry();
                auto cur_item_iter = cur_grp->get_cur_item_info().get_iter();
                drop( get_item_position( &*cur_item_iter ), pos() );
                if( !g->u.has_activity( activity_id( "ACT_ARMOR_LAYERS" ) ) ) {
                    return;
                }
                g->u.cancel_activity();
                cur_grp = nullptr;
                selected_line = 1;
            }
        }

        else if( action == "USAGE_HELP" ) {
            std::string help_str;
            for( size_t i = 0; i < column_num_entries; i++ ) {
                sort_armor_col_data col_data = get_col_data( i );
                if( static_cast<sort_armor_columns>( i ) == column_divider ) {
                    help_str += "\n";
                } else {
                    help_str += colorize( col_data.get_name(), c_white );
                    help_str += " - ";
                    help_str += colorize( col_data.get_description(), c_light_gray );
                    help_str += "\n";
                }

            }
            help_str += "\n";
            help_str += colorize( _( "Encumbrance explanation:\n" ), c_white );
            help_str += colorize( _( " total encumbrance = clothing encumbrance + additional penalty\n" ),
                                  c_light_gray );
            help_str += colorize( _( "Penalty is caused by wearing multiple items on the same layer or\n" ),
                                  c_light_gray );
            help_str += colorize( _( "wearing inner items over outer ones(e.g.a shirt over a backpack).\n" ),
                                  c_light_gray );
            help_str += "\n";
            help_str += colorize( _( "Layers order" ), c_white );
            help_str += colorize( _( ": (skin) -> (normal) -> (waist) -> (outer) -> (strap)" ), c_light_gray );

            popup( help_str );
        } else if( action == "QUIT" ) {
            exit = true;
        }

    }
}
