#include <algorithm>
#include <climits>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <new>
#include <string>
#include <vector>

#include "activity_type.h"
#include "bodypart.h"
#include "catacharset.h" // used for utf8_width()
#include "character.h"
#include "color.h"
#include "cursesdef.h"
#include "debug.h"
#include "enums.h"
#include "flag.h"
#include "flat_set.h"
#include "game_inventory.h"
#include "input.h"
#include "inventory.h"
#include "item.h"
#include "itype.h"
#include "line.h"
#include "output.h"
#include "pimpl.h"
#include "player_activity.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui_manager.h"
#include "units_utility.h"

static const activity_id ACT_ARMOR_LAYERS( "ACT_ARMOR_LAYERS" );

static const flag_id json_flag_HIDDEN( "HIDDEN" );

namespace
{
std::string clothing_layer( const item &worn_item );
std::vector<std::string> clothing_properties(
    const item &worn_item, int width, const Character &, const bodypart_id &bp );
std::vector<std::string> clothing_protection( const item &worn_item, int width,
        const bodypart_id &bp );
std::vector<std::string> clothing_flags_description( const item &worn_item );

std::string body_part_names( const std::vector<bodypart_id> &parts )
{
    if( parts.empty() ) {
        debugmsg( "Asked for names of empty list" );
        return {};
    }

    std::vector<std::string> names;
    names.reserve( parts.size() );
    for( size_t i = 0; i < parts.size(); ++i ) {
        const bodypart_id &part = parts[i];
        if( i + 1 < parts.size() &&
            parts[i + 1] == part->opposite_part ) {
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
                    Character &c, const bodypart_id &bp )
{
    const int win_width = getmaxx( w_sort_middle );
    const size_t win_height = static_cast<size_t>( getmaxy( w_sort_middle ) );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    size_t i = fold_and_print( w_sort_middle, point( 1, 0 ), win_width - 1, c_white,
                               worn_item_it->type_name( 1 ) ) - 1;
    std::vector<std::string> props = clothing_properties( *worn_item_it, win_width - 3, c,
                                     bp );
    nc_color color = c_light_gray;
    for( std::string &iter : props ) {
        print_colored_text( w_sort_middle, point( 2, ++i ), color, c_light_gray, iter );
    }

    std::vector<std::string> prot = clothing_protection( *worn_item_it, win_width - 3, bp );
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

    const item_penalties penalties = c.worn.get_item_penalties( worn_item_it, c, bp );

    if( !penalties.body_parts_with_stacking_penalty.empty() ) {
        std::string layer_description = [&]() -> std::string {
            std::string outstring;
            for( layer_level layer : worn_item_it->get_layer() )
            {
                switch( layer ) {
                    case layer_level::PERSONAL:
                        outstring.append( _( "in your <color_light_blue>personal aura</color>" ) );
                        break;
                    case layer_level::SKINTIGHT:
                        outstring.append( _( "<color_light_blue>close to your skin</color>" ) );
                        break;
                    case layer_level::NORMAL:
                        outstring.append( _( "of <color_light_blue>normal</color> clothing" ) );
                        break;
                    case layer_level::WAIST:
                        outstring.append( _( "on your <color_light_blue>waist</color>" ) );
                        break;
                    case layer_level::OUTER:
                        outstring.append( _( "of <color_light_blue>outer</color> clothing" ) );
                        break;
                    case layer_level::BELTED:
                        outstring.append( _( "<color_light_blue>strapped</color> to you" ) );
                        break;
                    case layer_level::AURA:
                        outstring.append( _( "an <color_light_blue>aura</color> around you" ) );
                        break;
                    default:
                        break;
                }
            }
            return outstring ;
        }
        ();
        std::string body_parts =
            body_part_names( penalties.body_parts_with_stacking_penalty );
        std::string message =
            string_format(
                n_gettext( "Wearing multiple items %s on your "
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
                          n_gettext( "Wearing this outside items it would normally be beneath "
                                     "is adding encumbrance to your <color_light_red>%s</color>.",
                                     "Wearing this outside items it would normally be beneath "
                                     "is adding encumbrance to your <color_light_red>%s</color>.",
                                     penalties.body_parts_with_out_of_order_penalty.size() ),
                          body_parts
                      );
        } else {
            std::string bad_item_name = *penalties.bad_items_within.begin();
            message = string_format(
                          n_gettext( "Wearing this outside your <color_light_blue>%s</color> "
                                     "is adding encumbrance to your <color_light_red>%s</color>.",
                                     "Wearing this outside your <color_light_blue>%s</color> "
                                     "is adding encumbrance to your <color_light_red>%s</color>.",
                                     penalties.body_parts_with_out_of_order_penalty.size() ),
                          bad_item_name, body_parts
                      );
        }
        fold_and_print( w_sort_middle, point( 0, i ), win_width, c_light_gray, message );
    }
}

std::string clothing_layer( const item &worn_item )
{
    std::string layer;

    if( worn_item.has_flag( flag_PERSONAL ) ) {
        layer = _( "This is in your personal aura." );
    } else if( worn_item.has_flag( flag_SKINTIGHT ) ) {
        layer = _( "This is worn next to the skin." );
    } else if( worn_item.has_flag( flag_WAIST ) ) {
        layer = _( "This is worn on or around your waist." );
    } else if( worn_item.has_flag( flag_OUTER ) ) {
        layer = _( "This is worn over your other clothes." );
    } else if( worn_item.has_flag( flag_BELTED ) ) {
        layer = _( "This is strapped onto you." );
    } else if( worn_item.has_flag( flag_AURA ) ) {
        layer = _( "This is an aura around you." );
    }

    return layer;
}

std::vector<std::string> clothing_properties(
    const item &worn_item, const int width, const Character &c, const bodypart_id &bp )
{
    std::vector<std::string> props;
    props.reserve( 5 );

    const std::string space = "  ";

    props.push_back( string_format( "<color_c_green>[%s]</color>", _( "Properties" ) ) );

    int coverage = bp == bodypart_id( "bp_null" ) ? worn_item.get_avg_coverage() :
                   worn_item.get_coverage( bp );
    props.push_back( name_and_value( space + _( "Coverage:" ),
                                     string_format( "%3d", coverage ), width ) );
    coverage = bp == bodypart_id( "bp_null" ) ? worn_item.get_avg_coverage(
                   item::cover_type::COVER_MELEE ) :
               worn_item.get_coverage( bp, item::cover_type::COVER_MELEE );
    props.push_back( name_and_value( space + _( "Coverage (Melee):" ),
                                     string_format( "%3d", coverage ), width ) );
    coverage = bp == bodypart_id( "bp_null" ) ? worn_item.get_avg_coverage(
                   item::cover_type::COVER_RANGED ) :
               worn_item.get_coverage( bp, item::cover_type::COVER_RANGED );
    props.push_back( name_and_value( space + _( "Coverage (Ranged):" ),
                                     string_format( "%3d", coverage ), width ) );
    coverage = bp == bodypart_id( "bp_null" ) ? worn_item.get_avg_coverage(
                   item::cover_type::COVER_VITALS ) :
               worn_item.get_coverage( bp, item::cover_type::COVER_VITALS );
    props.push_back( name_and_value( space + _( "Coverage (Vitals):" ),
                                     string_format( "%3d", coverage ), width ) );

    const int encumbrance = bp == bodypart_id( "bp_null" ) ? worn_item.get_avg_encumber(
                                c ) : worn_item.get_encumber( c, bp );
    props.push_back( name_and_value( space + _( "Encumbrance:" ),
                                     string_format( "%3d", encumbrance ), width ) );

    props.push_back( name_and_value( space + _( "Warmth:" ),
                                     string_format( "%3d", worn_item.get_warmth() ), width ) );
    return props;
}

std::vector<std::string> clothing_protection( const item &worn_item, const int width,
        const bodypart_id &bp )
{
    std::vector<std::string> prot;
    prot.reserve( 6 );

    // prebuild and calc some values
    // the rolls are basically a perfect hit for protection and a
    // worst possible and a median hit
    resistances worst_res = resistances( worn_item, false, 99, bp );
    resistances best_res = resistances( worn_item, false, 0, bp );
    resistances median_res = resistances( worn_item, false, 50, bp );

    int percent_best = 100;
    int percent_worst = 0;
    const armor_portion_data *portion = worn_item.portion_for_bodypart( bp );
    // if there isn't a portion this is probably pet armor
    if( portion ) {
        percent_best = portion->best_protection_chance;
        percent_worst = portion->worst_protection_chance;
    }

    bool display_median = percent_best < 50 && percent_worst < 50;


    const std::string space = "  ";
    prot.push_back( string_format( "<color_c_green>[%s]</color>", _( "Protection" ) ) );
    // bash ballistic and cut can have more involved info based on armor complexity
    if( display_median ) {
        prot.push_back( name_and_value( space + _( "Bash:" ),
                                        string_format( _( "Worst: %.2f, Median: %.2f, Best: %.2f" ),
                                                worst_res.type_resist( damage_type::BASH ), median_res.type_resist( damage_type::BASH ),
                                                best_res.type_resist( damage_type::BASH ) ),
                                        width ) );
        prot.push_back( name_and_value( space + _( "Cut:" ),
                                        string_format( _( "Worst: %.2f, Median: %.2f, Best: %.2f" ),
                                                worst_res.type_resist( damage_type::CUT ), median_res.type_resist( damage_type::CUT ),
                                                best_res.type_resist( damage_type::CUT ) ),
                                        width ) );
        prot.push_back( name_and_value( space + _( "Ballistic:" ),
                                        string_format( _( "Worst: %.2f, Median: %.2f, Best: %.2f" ),
                                                worst_res.type_resist( damage_type::BULLET ), median_res.type_resist( damage_type::BULLET ),
                                                best_res.type_resist( damage_type::BULLET ) ),
                                        width ) );
    } else if( percent_worst > 0 ) {
        prot.push_back( name_and_value( space + _( "Bash:" ),
                                        string_format( _( "Worst: %.2f, Best: %.2f" ),
                                                worst_res.type_resist( damage_type::BASH ),
                                                best_res.type_resist( damage_type::BASH ) ),
                                        width ) );
        prot.push_back( name_and_value( space + _( "Cut:" ),
                                        string_format( _( "Worst: %.2f, Best: %.2f" ),
                                                worst_res.type_resist( damage_type::CUT ),
                                                best_res.type_resist( damage_type::CUT ) ),
                                        width ) );
        prot.push_back( name_and_value( space + _( "Ballistic:" ),
                                        string_format( _( "Worst: %.2f, Best: %.2f" ),
                                                worst_res.type_resist( damage_type::BULLET ),
                                                best_res.type_resist( damage_type::BULLET ) ),
                                        width ) );
    } else {
        prot.push_back( name_and_value( space + _( "Bash:" ),
                                        string_format( "%.2f", best_res.type_resist( damage_type::BASH ) ), width ) );
        prot.push_back( name_and_value( space + _( "Cut:" ),
                                        string_format( "%.2f", best_res.type_resist( damage_type::CUT ) ), width ) );
        prot.push_back( name_and_value( space + _( "Ballistic:" ),
                                        string_format( "%.2f", best_res.type_resist( damage_type::BULLET ) ), width ) );
    }
    prot.push_back( name_and_value( space + _( "Acid:" ),
                                    string_format( "%.2f", best_res.type_resist( damage_type::ACID ) ), width ) );
    prot.push_back( name_and_value( space + _( "Fire:" ),
                                    string_format( "%.2f", best_res.type_resist( damage_type::HEAT ) ), width ) );
    prot.push_back( name_and_value( space + _( "Environmental:" ),
                                    string_format( "%3d", static_cast<int>( worn_item.get_env_resist() ) ), width ) );
    prot.push_back( name_and_value( space + _( "Breathability:" ),
                                    string_format( "%3d", static_cast<int>( worn_item.breathability( bp ) ) ), width ) );
    return prot;
}

std::vector<std::string> clothing_flags_description( const item &worn_item )
{
    std::vector<std::string> description_stack;

    if( worn_item.has_flag( flag_FIT ) ) {
        description_stack.emplace_back( _( "It fits you well." ) );
    } else if( worn_item.has_flag( flag_VARSIZE ) ) {
        description_stack.emplace_back( _( "It could be refitted." ) );
    }

    if( worn_item.has_flag( flag_HOOD ) ) {
        description_stack.emplace_back( _( "It has a hood." ) );
    }
    if( worn_item.has_flag( flag_POCKETS ) ) {
        description_stack.emplace_back( _( "It has pockets." ) );
    }
    if( worn_item.has_flag( flag_SUN_GLASSES ) ) {
        description_stack.emplace_back( _( "It keeps the sun out of your eyes." ) );
    }
    if( worn_item.has_flag( flag_WATERPROOF ) ) {
        description_stack.emplace_back( _( "It is waterproof." ) );
    }
    if( worn_item.has_flag( flag_WATER_FRIENDLY ) ) {
        description_stack.emplace_back( _( "It is water friendly." ) );
    }
    if( worn_item.has_flag( flag_FANCY ) ) {
        description_stack.emplace_back( _( "It looks fancy." ) );
    }
    if( worn_item.has_flag( flag_SUPER_FANCY ) ) {
        description_stack.emplace_back( _( "It looks really fancy." ) );
    }
    if( worn_item.has_flag( flag_FLOTATION ) ) {
        description_stack.emplace_back( _( "You will not drown today." ) );
    }
    if( worn_item.has_flag( flag_OVERSIZE ) ) {
        description_stack.emplace_back( _( "It is very bulky." ) );
    }
    if( worn_item.has_flag( flag_SWIM_GOGGLES ) ) {
        description_stack.emplace_back( _( "It helps you to see clearly underwater." ) );
    }
    if( worn_item.has_flag( flag_SEMITANGIBLE ) ) {
        description_stack.emplace_back( _( "It can occupy the same space as other things." ) );
    }

    return description_stack;
}

} //namespace

// Figure out encumbrance penalties this clothing is involved in
item_penalties outfit::get_item_penalties( std::list<item>::const_iterator worn_item_it,
        const Character &c, const bodypart_id &_bp )
{
    std::vector<layer_level> layer = worn_item_it->get_layer();

    std::vector<bodypart_id> body_parts_with_stacking_penalty;
    std::vector<bodypart_id> body_parts_with_out_of_order_penalty;
    std::vector<std::set<std::string>> lists_of_bad_items_within;

    for( const bodypart_id &bp : c.get_all_body_parts() ) {
        if( bp != _bp && _bp != bodypart_id( "bp_null" ) ) {
            continue;
        }
        if( !worn_item_it->covers( bp ) ) {
            continue;
        }

        std::set<std::string> bad_items_within;

        // if no subparts do the old way
        if( bp->sub_parts.empty() ) {
            std::vector<layer_level> layer = worn_item_it->get_layer( bp );
            bool first_integrated = true;
            const int num_items = std::count_if( worn.begin(), worn.end(),
            [layer, bp, &first_integrated]( const item & i ) {
                if( i.has_layer( layer, bp ) && i.covers( bp ) && !i.has_flag( flag_SEMITANGIBLE ) ) {
                    if( i.has_flag( flag_INTEGRATED ) ) {
                        if( first_integrated ) {
                            first_integrated = false;
                            return true;
                        }
                        return false;
                    }
                    return true;
                }
                return false;
            } );
            if( num_items > 1 ) {
                body_parts_with_stacking_penalty.push_back( bp );
            }
            for( auto it = worn.begin(); it != worn_item_it; ++it ) {
                if( it->get_layer( bp ) > layer && it->covers( bp ) ) {
                    bad_items_within.insert( it->type_name() );
                }
            }
        } else {
            for( const sub_bodypart_str_id &sbp : bp->sub_parts ) {
                if( !worn_item_it->covers( sbp ) ) {
                    continue;
                }
                std::vector<layer_level> layer = worn_item_it->get_layer( sbp );
                bool first_integrated = true;
                const int num_items = std::count_if( worn.begin(), worn.end(),
                [layer, bp, sbp, &first_integrated]( const item & i ) {
                    if( i.has_layer( layer, sbp ) && i.covers( bp ) && !i.has_flag( flag_SEMITANGIBLE ) &&
                        i.covers( sbp ) ) {
                        if( i.has_flag( flag_INTEGRATED ) ) {
                            if( first_integrated ) {
                                first_integrated = false;
                                return true;
                            }
                            return false;
                        }
                        return true;
                    }
                    return false;
                } );
                if( num_items > 1 ) {
                    body_parts_with_stacking_penalty.push_back( bp );
                }
                for( auto it = worn.begin(); it != worn_item_it; ++it ) {
                    if( it->covers( sbp ) ) {
                        if( it->get_highest_layer( sbp ) > worn_item_it->get_highest_layer( sbp ) ) {
                            bad_items_within.insert( it->type_name() );
                        }
                    }
                }
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

std::vector<layering_item_info> outfit::items_cover_bp( const Character &c, const bodypart_id &bp )
{
    std::vector<layering_item_info> s;
    for( auto elem_it = worn.begin(); elem_it != worn.end(); ++elem_it ) {
        if( elem_it->covers( bp ) ) {
            s.push_back( { get_item_penalties( elem_it, c, bp ),
                           elem_it->get_encumber( c, bp ),
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

    wnoutrefresh( w );
}

void outfit::sort_armor( Character &guy )
{
    // FIXME: get_all_body_parts() doesn't return a sorted list
    //        and bodypart_id is not compatible with std::sort()
    //        so let's use a dirty hack
    cata::flat_set<bodypart_id> armor_cat;
    for( const bodypart_id &it : guy.get_all_body_parts() ) {
        armor_cat.insert( it );
    }
    armor_cat.insert( bodypart_id( "bp_null" ) );
    const int num_of_parts = guy.get_all_body_parts().size();

    int win_h = 0;
    int win_w = 0;
    point win;

    int cont_h   = 0;
    int left_w   = 0;
    int right_w  = 0;
    int middle_w = 0;

    int tabindex = 0;
    const int tabcount = num_of_parts + 1;

    int leftListIndex  = 0;
    int leftListOffset = 0;
    int selected       = -1;

    int rightListOffset = 0;

    int leftListLines = 0;
    int rightListLines = 0;

    std::vector<std::list<item>::iterator> tmp_worn;

    // Layout window
    catacurses::window w_sort_armor;
    // Subwindows (between lines)
    catacurses::window w_sort_cat;
    catacurses::window w_sort_left;
    catacurses::window w_sort_middle;
    catacurses::window w_sort_right;
    catacurses::window w_encumb;

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        win_h = TERMY;
        win_w = FULL_SCREEN_WIDTH + ( TERMX - FULL_SCREEN_WIDTH ) * 3 / 4;
        win.x = TERMX / 2 - win_w / 2;
        win.y = TERMY / 2 - win_h / 2;
        cont_h = win_h - 4;
        left_w = ( win_w - 4 ) / 3;
        right_w = left_w;
        middle_w = ( win_w - 4 ) - left_w - right_w;
        leftListLines = rightListLines = cont_h - 2;
        w_sort_armor = catacurses::newwin( win_h, win_w, win );
        w_sort_cat = catacurses::newwin( 1, win_w - 4, win + point( 2, 1 ) );
        w_sort_left = catacurses::newwin( cont_h, left_w, win + point( 1, 3 ) );
        w_sort_middle = catacurses::newwin( cont_h - num_of_parts - 1, middle_w,
                                            win + point( 2 + left_w, 3 ) );
        w_sort_right = catacurses::newwin( cont_h, right_w,
                                           win + point( 3 + left_w + middle_w, 3 ) );
        w_encumb = catacurses::newwin( num_of_parts + 1, middle_w,
                                       win + point( 2 + left_w, -1 + 3 + cont_h - num_of_parts ) );
        ui.position_from_window( w_sort_armor );
    } );
    ui.mark_resize();

    input_context ctxt( "SORT_ARMOR" );
    ctxt.register_cardinal();
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "MOVE_ARMOR" );
    ctxt.register_action( "CHANGE_SIDE" );
    ctxt.register_action( "TOGGLE_CLOTH" );
    ctxt.register_action( "ASSIGN_INVLETS" );
    ctxt.register_action( "SORT_ARMOR" );
    ctxt.register_action( "EQUIP_ARMOR" );
    ctxt.register_action( "EQUIP_ARMOR_HERE" );
    ctxt.register_action( "REMOVE_ARMOR" );
    ctxt.register_action( "USAGE_HELP" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    Character &player_character = get_player_character();
    auto do_return_entry = [&player_character]() {
        player_character.assign_activity( ACT_ARMOR_LAYERS, 0 );
        player_character.activity.auto_resume = true;
        player_character.activity.moves_left = INT_MAX;
    };

    int leftListSize = 0;
    int rightListSize = 0;

    ui.on_redraw( [&]( const ui_adaptor & ) {
        // Create ptr list of items to display
        tmp_worn.clear();
        const bodypart_id &bp = armor_cat[ tabindex ];
        if( bp == bodypart_id( "bp_null" ) ) {
            // All
            for( auto it = worn.begin(); it != worn.end(); ++it ) {
                tmp_worn.push_back( it );
            }
        } else {
            // bp_*
            for( auto it = worn.begin(); it != worn.end(); ++it ) {
                if( it->covers( bp ) ) {
                    tmp_worn.push_back( it );
                }
            }
        }

        // Ensure leftListIndex is in bounds
        int new_index_upper_bound = std::max( 0, leftListSize - 1 );
        leftListIndex = std::min( leftListIndex, new_index_upper_bound );

        draw_grid( w_sort_armor, left_w, middle_w );

        werase( w_sort_cat );
        werase( w_sort_left );
        werase( w_sort_middle );
        werase( w_sort_right );
        werase( w_encumb );

        // top bar
        wprintz( w_sort_cat, c_white, _( "Sort Armor" ) );
        std::string temp = bp != bodypart_id( "bp_null" ) ? body_part_name_as_heading( bp, 1 ) : _( "All" );
        wprintz( w_sort_cat, c_yellow, "  << %s >>", temp );
        right_print( w_sort_cat, 0, 0, c_white, string_format(
                         _( "[<color_yellow>%s</color>] Hide sprite.  "
                            "[<color_yellow>%s</color>] Change side.  "
                            "Press [<color_yellow>%s</color>] for help.  "
                            "Press [<color_yellow>%s</color>] to change keybindings." ),
                         ctxt.get_desc( "TOGGLE_CLOTH" ),
                         ctxt.get_desc( "CHANGE_SIDE" ),
                         ctxt.get_desc( "USAGE_HELP" ),
                         ctxt.get_desc( "HELP_KEYBINDINGS" ) ) );

        leftListSize = tmp_worn.size();
        if( leftListLines > leftListSize ) {
            leftListOffset = 0;
        } else if( leftListOffset + leftListLines > leftListSize ) {
            leftListOffset = leftListSize - leftListLines;
        }
        if( leftListOffset > leftListIndex ) {
            leftListOffset = leftListIndex;
        } else if( leftListOffset + leftListLines <= leftListIndex ) {
            leftListOffset = leftListIndex + 1 - leftListLines;
        }

        // Left header
        mvwprintz( w_sort_left, point_zero, c_light_gray, _( "(Innermost)" ) );
        right_print( w_sort_left, 0, 0, c_light_gray, string_format( _( "Storage (%s)" ),
                     volume_units_abbr() ) );
        // Left list
        const int max_drawindex = std::min( leftListSize - leftListOffset, leftListLines );
        for( int drawindex = 0; drawindex < max_drawindex; drawindex++ ) {
            int itemindex = leftListOffset + drawindex;

            if( itemindex == leftListIndex ) {
                mvwprintz( w_sort_left, point( 0, drawindex + 1 ), c_yellow, ">>" );
            }

            std::string worn_armor_name = tmp_worn[itemindex]->tname();
            // Get storage capacity in user's preferred units
            units::volume worn_armor_capacity = tmp_worn[itemindex]->get_total_capacity();
            double worn_armor_storage = convert_volume( units::to_milliliter( worn_armor_capacity ) );
            std::string storage_string = string_format( "%.2f", worn_armor_storage );
            const int storage_character_allowance = worn_armor_storage > 0 ? storage_string.length() : 0;

            item_penalties const penalties =
                get_item_penalties( tmp_worn[itemindex], guy, bp );

            const int offset_x = ( itemindex == selected ) ? 4 : 3;
            // Show armor name and storage capacity (if any)
            trim_and_print( w_sort_left, point( offset_x, drawindex + 1 ),
                            left_w - offset_x - 1 - storage_character_allowance,
                            penalties.color_for_stacking_badness(), worn_armor_name );


            if( worn_armor_storage > 0 ) {
                // two digits, accurate to 1% of preferred storage unit
                right_print( w_sort_left, drawindex + 1, 0, c_light_gray,
                             string_format( "%.2f", worn_armor_storage ) );
            }

            if( tmp_worn[itemindex]->has_flag( json_flag_HIDDEN ) ) {
                //~ Hint: Letter to show which piece of armor is Hidden in the layering menu
                mvwprintz( w_sort_left, point( offset_x - 1, drawindex + 1 ), c_cyan, _( "H" ) );
            }
        }

        // Left footer
        mvwprintz( w_sort_left, point( 0, cont_h - 1 ), c_light_gray, _( "(Outermost)" ) );
        if( leftListOffset + leftListLines < leftListSize ) {
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
            draw_mid_pane( w_sort_middle, tmp_worn[leftListIndex], guy, bp );
        } else {
            // NOLINTNEXTLINE(cata-use-named-point-constants)
            fold_and_print( w_sort_middle, point( 1, 0 ), middle_w - 1, c_white,
                            _( "Nothing to see here!" ) );
        }

        mvwprintz( w_encumb, point_east, c_white, _( "Encumbrance and Warmth" ) );
        guy.print_encumbrance( w_encumb, -1, ( leftListSize > 0 ) ? &*tmp_worn[leftListIndex] : nullptr );

        // Right header
        mvwprintz( w_sort_right, point_zero, c_light_gray, _( "(Innermost)" ) );
        right_print( w_sort_right, 0, 0, c_light_gray, _( "Encumbrance" ) );

        const auto &combine_bp = [&guy]( const bodypart_id & cover ) -> bool {
            const bodypart_id opposite = cover.obj().opposite_part;
            return cover != opposite &&
            guy.worn.items_cover_bp( guy, cover ) == guy.worn.items_cover_bp( guy, opposite );
        };

        cata::flat_set<bodypart_id> rl;
        // Right list
        rightListSize = 0;
        for( const bodypart_id &cover : armor_cat ) {
            if( !combine_bp( cover ) || rl.count( cover.obj().opposite_part ) == 0 ) {
                rightListSize += items_cover_bp( guy, cover ).size() + 1;
                rl.insert( cover );
            }
        }
        if( rightListLines > rightListSize ) {
            rightListOffset = 0;
        } else if( rightListOffset + rightListLines > rightListSize ) {
            rightListOffset = rightListSize - rightListLines;
        }
        int pos = 1;
        int curr = 0;
        for( const bodypart_id &cover : rl ) {
            if( cover == bodypart_id( "bp_null" ) ) {
                continue;
            }
            if( curr >= rightListOffset && pos <= rightListLines ) {
                mvwprintz( w_sort_right, point( 1, pos ), ( cover == bp ? c_yellow : c_white ),
                           "%s:", body_part_name_as_heading( cover, combine_bp( cover ) ? 2 : 1 ) );
                pos++;
            }
            curr++;
            for( layering_item_info &elem : items_cover_bp( guy, cover ) ) {
                if( curr >= rightListOffset && pos <= rightListLines ) {
                    nc_color color = elem.penalties.color_for_stacking_badness();
                    trim_and_print( w_sort_right, point( 2, pos ), right_w - 5, color,
                                    elem.name );
                    char plus = elem.penalties.badness() > 0 ? '+' : ' ';
                    mvwprintz( w_sort_right, point( right_w - 4, pos ), c_light_gray, "%3d%c",
                               elem.encumber, plus );
                    pos++;
                }
                curr++;
            }
        }

        // Right footer
        mvwprintz( w_sort_right, point( 0, cont_h - 1 ), c_light_gray, _( "(Outermost)" ) );
        if( rightListOffset + rightListLines < rightListSize ) {
            // TODO: replace it by right_print()
            mvwprintz( w_sort_right, point( right_w - utf8_width( _( "<more>" ) ), cont_h - 1 ), c_light_blue,
                       _( "<more>" ) );
        }
        // F5
        wnoutrefresh( w_sort_cat );
        wnoutrefresh( w_sort_left );
        wnoutrefresh( w_sort_middle );
        wnoutrefresh( w_sort_right );
        wnoutrefresh( w_encumb );
    } );

    bool exit = false;
    while( !exit ) {
        if( guy.is_avatar() ) {
            // Totally hoisted this from advanced_inv
            if( player_character.moves < 0 ) {
                do_return_entry();
                return;
            }
        } else {
            // Player is sorting NPC's armor here
            if( rl_dist( player_character.pos(), guy.pos() ) > 1 ) {
                guy.add_msg_if_npc( m_bad, _( "%s is too far to sort armor." ), guy.get_name() );
                return;
            }
            if( guy.attitude_to( player_character ) != Creature::Attitude::FRIENDLY ) {
                guy.add_msg_if_npc( m_bad, _( "%s is not friendly!" ), guy.get_name() );
                return;
            }
        }

        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        if( guy.is_npc() && action == "ASSIGN_INVLETS" ) {
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
                guy.calc_encumbrance();
            }
        };

        if( action == "UP" && leftListSize > 0 ) {
            if( leftListIndex > 0 ) {
                item &item_to_check = *tmp_worn[leftListIndex - 1];
                if( selected < 0 || !item_to_check.has_flag( flag_INTEGRATED ) ) {
                    leftListIndex--;
                    if( leftListIndex < leftListOffset ) {
                        leftListOffset = leftListIndex;
                    }
                    shift_selected_item();
                } else {
                    popup( _( "Can't sort this under your integrated armor!" ) );
                }
            } else {
                leftListIndex = leftListSize - 1;
                if( leftListLines >= leftListSize ) {
                    leftListOffset = 0;
                } else {
                    leftListOffset = leftListSize - leftListLines;
                }
                shift_selected_item();
            }
        } else if( action == "DOWN" && leftListSize > 0 ) {
            if( leftListIndex + 1 < leftListSize ) {
                leftListIndex++;
                if( leftListIndex >= leftListOffset + leftListLines ) {
                    leftListOffset = leftListIndex + 1 - leftListLines;
                }
                shift_selected_item();
            } else {
                item &item_to_check = *tmp_worn[0];
                if( selected < 0 || !item_to_check.has_flag( flag_INTEGRATED ) ) {
                    leftListIndex = 0;
                    leftListOffset = 0;
                    shift_selected_item();
                } else {
                    popup( _( "Can't sort this under your integrated armor!" ) );
                }
            }
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
            if( rightListOffset + rightListLines < rightListSize ) {
                rightListOffset++;
            }
        } else if( action == "PREV_TAB" ) {
            if( rightListOffset > 0 ) {
                rightListOffset--;
            }
        } else if( action == "MOVE_ARMOR" ) {
            if( selected >= 0 ) {
                selected = -1;
            } else {
                item &item_to_check = *tmp_worn[leftListIndex];
                if( !item_to_check.has_flag( flag_INTEGRATED ) ) {
                    selected = leftListIndex;
                } else {
                    popup( _( "Can't move your integrated armor!" ) );
                }
            }
        } else if( action == "CHANGE_SIDE" ) {
            if( leftListIndex < leftListSize && tmp_worn[leftListIndex]->is_sided() ) {
                if( player_character.query_yn( _( "Swap side for %s?" ),
                                               colorize( tmp_worn[leftListIndex]->tname(),
                                                       tmp_worn[leftListIndex]->color_in_inventory() ) ) ) {
                    guy.change_side( *tmp_worn[leftListIndex] );
                }
            }
        } else if( action == "TOGGLE_CLOTH" ) {
            if( leftListIndex < leftListSize ) {
                if( !tmp_worn[leftListIndex]->has_flag( json_flag_HIDDEN ) ) {
                    tmp_worn[leftListIndex]->set_flag( json_flag_HIDDEN );
                } else {
                    tmp_worn[leftListIndex]->unset_flag( json_flag_HIDDEN );
                }
            }
        } else if( action == "SORT_ARMOR" ) {
            // Copy to a vector because stable_sort requires random-access
            // iterators
            std::vector<item> worn_copy( worn.begin(), worn.end() );
            std::stable_sort( worn_copy.begin(), worn_copy.end(),
            []( const item & l, const item & r ) {
                if( l.has_flag( flag_INTEGRATED ) == r.has_flag( flag_INTEGRATED ) ) {
                    return l.get_layer() < r.get_layer();
                } else {
                    return l.has_flag( flag_INTEGRATED );
                }
            }
                            );
            std::copy( worn_copy.begin(), worn_copy.end(), worn.begin() );
            guy.calc_encumbrance();
        } else if( action == "EQUIP_ARMOR" ) {
            // filter inventory for all items that are armor/clothing
            item_location loc = game_menus::inv::wear( guy );
            // only equip if something valid selected!
            if( loc ) {
                // store the item name just in case obtain() fails
                const std::string item_name = loc->display_name();
                item_location obtained = loc.obtain( guy );
                if( obtained ) {
                    // wear the item
                    cata::optional<std::list<item>::iterator> new_equip_it =
                        guy.wear( obtained );
                    if( new_equip_it ) {
                        const bodypart_id &bp = armor_cat[tabindex];
                        if( tabindex == num_of_parts || ( **new_equip_it ).covers( bp ) ) {
                            // Set ourselves up to be pointing at the new item
                            // TODO: This doesn't work yet because we don't save our
                            // state through other activities, but that's a thing
                            // that would be nice to do.
                            leftListIndex =
                                std::count_if( worn.begin(), *new_equip_it,
                            [&]( const item & i ) {
                                return tabindex == num_of_parts || i.covers( bp );
                            } );
                        }
                    } else if( guy.is_npc() ) {
                        // TODO: Pass the reason here
                        popup( _( "Can't put this on!" ) );
                    }
                } else {
                    guy.add_msg_if_player( _( "You chose not to wear the %s." ), item_name );
                }
            }
        } else if( action == "EQUIP_ARMOR_HERE" ) {
            // filter inventory for all items that are armor/clothing
            item_location loc = game_menus::inv::wear( guy, armor_cat[tabindex] );
            // only equip if something valid selected!
            if( loc ) {
                // store the item name just in case obtain() fails
                const std::string item_name = loc->display_name();
                item_location obtained = loc.obtain( guy );
                if( obtained ) {
                    // wear the item
                    cata::optional<std::list<item>::iterator> new_equip_it =
                        guy.wear( obtained );
                    if( new_equip_it ) {
                        // save iterator to cursor's position
                        std::list<item>::iterator cursor_it = tmp_worn[leftListIndex];
                        item &item_to_check = *cursor_it;
                        if( item_to_check.has_flag( flag_INTEGRATED ) ) {
                            // prevent adding under integrated armor
                            popup( _( "Can't put this on under your integrated armor!" ) );
                        } else {
                            // reorder `worn` vector to place new item at cursor
                            worn.splice( cursor_it, worn, *new_equip_it );
                        }
                    } else if( guy.is_npc() ) {
                        // TODO: Pass the reason here
                        popup( _( "Can't put this on!" ) );
                    }
                } else {
                    guy.add_msg_if_player( _( "You chose not to wear the %s." ), item_name );
                }
            }
        } else if( action == "REMOVE_ARMOR" ) {
            // query (for now)
            if( leftListIndex < leftListSize ) {
                if( player_character.query_yn( _( "Remove selected armor?" ) ) ) {
                    do_return_entry();

                    //create an item_location for player::takeoff to handle.
                    item &item_for_takeoff = *tmp_worn[leftListIndex];
                    item_location loc_for_takeoff = item_location( guy, &item_for_takeoff );

                    // remove the item, asking to drop it if necessary
                    guy.takeoff( loc_for_takeoff );
                    if( !player_character.has_activity( ACT_ARMOR_LAYERS ) ) {
                        // An activity has been created to take off the item;
                        // we must surrender control until it is done.
                        return;
                    }
                    player_character.cancel_activity();
                    selected = -1;
                    leftListIndex = std::max( 0, leftListIndex - 1 );
                }
            }
        } else if( action == "ASSIGN_INVLETS" ) {
            // prompt first before doing this (yes, yes, more popups...)
            if( query_yn( _( "Reassign inventory letters for armor?" ) ) ) {
                // Start with last armor (the most unimportant one?)
                auto iiter = inv_chars.rbegin();
                auto witer = worn.rbegin();
                while( witer != worn.rend() && iiter != inv_chars.rend() ) {
                    const char invlet = *iiter;
                    item &w = *witer;
                    if( invlet == w.invlet ) {
                        ++witer;
                    } else if( guy.invlet_to_item( invlet ) != nullptr ) {
                        ++iiter;
                    } else {
                        guy.inv->reassign_item( w, invlet );
                        ++witer;
                        ++iiter;
                    }
                }
            }
        } else if( action == "USAGE_HELP" ) {
            popup_getkey(
                _( "Use the [<color_yellow>arrow- or keypad keys</color>] to navigate the left list.\n"
                   "[<color_yellow>%s</color>] to select highlighted armor for reordering.\n"
                   "[<color_yellow>%s</color>] / [<color_yellow>%s</color>] to scroll the right list.\n"
                   "[<color_yellow>%s</color>] to assign special inventory letters to clothing.\n"
                   "[<color_yellow>%s</color>] to change the side on which item is worn.\n"
                   "[<color_yellow>%s</color>] to toggle armor visibility on character sprite.\n"
                   "[<color_yellow>%s</color>] to sort armor into natural layer order.\n"
                   "[<color_yellow>%s</color>] to equip a new item.\n"
                   "[<color_yellow>%s</color>] to equip a new item at the currently selected position.\n"
                   "[<color_yellow>%s</color>] to remove selected armor from oneself.\n"
                   "\n"
                   "\n"
                   "Encumbrance explanation:\n"
                   "\n"
                   "<color_light_gray>The first number is the summed encumbrance from all clothing "
                   "on that bodypart.  The second number is an additional encumbrance penalty "
                   "caused by wearing either multiple items on one of the bodypart's layers or "
                   "wearing items the wrong way (e.g. a shirt over a backpack).  "
                   "The sum of these values is the effective encumbrance value "
                   "your character has for that bodypart.</color>" ),
                ctxt.get_desc( "MOVE_ARMOR" ),
                ctxt.get_desc( "PREV_TAB" ),
                ctxt.get_desc( "NEXT_TAB" ),
                ctxt.get_desc( "ASSIGN_INVLETS" ),
                ctxt.get_desc( "CHANGE_SIDE" ),
                ctxt.get_desc( "TOGGLE_CLOTH" ),
                ctxt.get_desc( "SORT_ARMOR" ),
                ctxt.get_desc( "EQUIP_ARMOR" ),
                ctxt.get_desc( "EQUIP_ARMOR_HERE" ),
                ctxt.get_desc( "REMOVE_ARMOR" )
            );
        } else if( action == "QUIT" ) {
            exit = true;
        }
    }
}
