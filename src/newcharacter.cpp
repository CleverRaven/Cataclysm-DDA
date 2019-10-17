#include "avatar.h" // IWYU pragma: associated

#include <cstdlib>
#include <algorithm>
#include <sstream>
#include <vector>
#include <iterator>
#include <tuple>
#include <array>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <utility>

#include "addiction.h"
#include "bionics.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "game.h"
#include "ime.h"
#include "input.h"
#include "json.h"
#include "mapsharing.h"
#include "martialarts.h"
#include "monster.h"
#include "mutation.h"
#include "name.h"
#include "options.h"
#include "output.h"
#include "path_info.h"
#include "profession.h"
#include "recipe_dictionary.h"
#include "rng.h"
#include "scenario.h"
#include "skill.h"
#include "start_location.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "ui.h"
#include "worldfactory.h"
#include "recipe.h"
#include "string_id.h"
#include "character.h"
#include "color.h"
#include "cursesdef.h"
#include "game_constants.h"
#include "inventory.h"
#include "optional.h"
#include "pimpl.h"
#include "type_id.h"

struct points_left;

// Colors used in this file: (Most else defaults to c_light_gray)
#define COL_STAT_ACT        c_white   // Selected stat
#define COL_STAT_BONUS      c_light_green // Bonus
#define COL_STAT_NEUTRAL    c_white   // Neutral Property
#define COL_STAT_PENALTY    c_light_red   // Penalty
#define COL_TR_GOOD         c_green   // Good trait descriptive text
#define COL_TR_GOOD_OFF_ACT c_light_gray  // A toggled-off good trait
#define COL_TR_GOOD_ON_ACT  c_light_green // A toggled-on good trait
#define COL_TR_GOOD_OFF_PAS c_dark_gray  // A toggled-off good trait
#define COL_TR_GOOD_ON_PAS  c_green   // A toggled-on good trait
#define COL_TR_BAD          c_red     // Bad trait descriptive text
#define COL_TR_BAD_OFF_ACT  c_light_gray  // A toggled-off bad trait
#define COL_TR_BAD_ON_ACT   c_light_red   // A toggled-on bad trait
#define COL_TR_BAD_OFF_PAS  c_dark_gray  // A toggled-off bad trait
#define COL_TR_BAD_ON_PAS   c_red     // A toggled-on bad trait
#define COL_TR_NEUT         c_brown     // Neutral trait descriptive text
#define COL_TR_NEUT_OFF_ACT c_dark_gray  // A toggled-off neutral trait
#define COL_TR_NEUT_ON_ACT  c_yellow   // A toggled-on neutral trait
#define COL_TR_NEUT_OFF_PAS c_dark_gray  // A toggled-off neutral trait
#define COL_TR_NEUT_ON_PAS  c_brown     // A toggled-on neutral trait
#define COL_SKILL_USED      c_green   // A skill with at least one point
#define COL_HEADER          c_white   // Captions, like "Profession items"
#define COL_NOTE_MAJOR      c_green   // Important note
#define COL_NOTE_MINOR      c_light_gray  // Just regular note

#define HIGH_STAT 14 // The point after which stats cost double
#define MAX_STAT 20 // The point after which stats can not be increased further

#define NEWCHAR_TAB_MAX 6 // The ID of the rightmost tab

static int skill_increment_cost( const Character &u, const skill_id &skill );

struct points_left {
    int stat_points;
    int trait_points;
    int skill_points;

    enum point_limit : int {
        FREEFORM = 0,
        ONE_POOL,
        MULTI_POOL
    } limit;

    points_left() {
        limit = MULTI_POOL;
        init_from_options();
    }

    void init_from_options() {
        stat_points = get_option<int>( "INITIAL_STAT_POINTS" );
        trait_points = get_option<int>( "INITIAL_TRAIT_POINTS" );
        skill_points = get_option<int>( "INITIAL_SKILL_POINTS" );
    }

    // Highest amount of points to spend on stats without points going invalid
    int stat_points_left() const {
        switch( limit ) {
            case FREEFORM:
            case ONE_POOL:
                return stat_points + trait_points + skill_points;
            case MULTI_POOL:
                return std::min( trait_points_left(),
                                 stat_points + std::min( 0, trait_points + skill_points ) );
        }

        return 0;
    }

    int trait_points_left() const {
        switch( limit ) {
            case FREEFORM:
            case ONE_POOL:
                return stat_points + trait_points + skill_points;
            case MULTI_POOL:
                return stat_points + trait_points + std::min( 0, skill_points );
        }

        return 0;
    }

    int skill_points_left() const {
        return stat_points + trait_points + skill_points;
    }

    bool is_freeform() {
        return limit == FREEFORM;
    }

    bool is_valid() {
        return is_freeform() ||
               ( stat_points_left() >= 0 && trait_points_left() >= 0 &&
                 skill_points_left() >= 0 );
    }

    bool has_spare() {
        return !is_freeform() && is_valid() && skill_points_left() > 0;
    }

    std::string to_string() {
        if( limit == MULTI_POOL ) {
            return string_format(
                       _( "Points left: <color_%s>%d</color>%c<color_%s>%d</color>%c<color_%s>%d</color>=<color_%s>%d</color>" ),
                       stat_points_left() >= 0 ? "light_gray" : "red", stat_points,
                       trait_points >= 0 ? '+' : '-',
                       trait_points_left() >= 0 ? "light_gray" : "red", abs( trait_points ),
                       skill_points >= 0 ? '+' : '-',
                       skill_points_left() >= 0 ? "light_gray" : "red", abs( skill_points ),
                       is_valid() ? "light_gray" : "red", stat_points + trait_points + skill_points );
        } else if( limit == ONE_POOL ) {
            return string_format( _( "Points left: %4d" ), skill_points_left() );
        } else {
            return _( "Freeform" );
        }
    }
};

enum struct tab_direction {
    NONE,
    FORWARD,
    BACKWARD,
    QUIT
};

tab_direction set_points( const catacurses::window &w, avatar &u, points_left &points );
tab_direction set_stats( const catacurses::window &w, avatar &u, points_left &points );
tab_direction set_traits( const catacurses::window &w, avatar &u, points_left &points );
tab_direction set_scenario( const catacurses::window &w, avatar &u, points_left &points,
                            tab_direction direction );
tab_direction set_profession( const catacurses::window &w, avatar &u, points_left &points,
                              tab_direction direction );
tab_direction set_skills( const catacurses::window &w, avatar &u, points_left &points );
tab_direction set_description( const catacurses::window &w, avatar &you, bool allow_reroll,
                               points_left &points );

static cata::optional<std::string> query_for_template_name();
static void save_template( const avatar &u, const std::string &name, const points_left &points );
void reset_scenario( avatar &u, const scenario *scen );

void Character::pick_name( bool bUseDefault )
{
    if( bUseDefault && !get_option<std::string>( "DEF_CHAR_NAME" ).empty() ) {
        name = get_option<std::string>( "DEF_CHAR_NAME" );
    } else {
        name = Name::generate( male );
    }
}

static matype_id choose_ma_style( const character_type type, const std::vector<matype_id> &styles )
{
    if( type == PLTYPE_NOW || type == PLTYPE_FULL_RANDOM ) {
        return random_entry( styles );
    }
    if( styles.size() == 1 ) {
        return styles.front();
    }

    input_context ctxt( "MELEE_STYLE_PICKER" );
    ctxt.register_action( "SHOW_DESCRIPTION" );

    uilist menu;
    menu.allow_cancel = false;
    menu.text = string_format( _( "Select a style.  (press %s for more info)" ),
                               ctxt.get_desc( "SHOW_DESCRIPTION" ) );
    ma_style_callback callback( 0, styles );
    menu.callback = &callback;
    menu.input_category = "MELEE_STYLE_PICKER";
    menu.additional_actions.emplace_back( "SHOW_DESCRIPTION", "" );
    menu.desc_enabled = true;

    for( auto &s : styles ) {
        auto &style = s.obj();
        menu.addentry_desc( style.name.translated(), style.description.translated() );
    }
    while( true ) {
        menu.query( true );
        auto &selected = styles[menu.ret];
        if( query_yn( _( "Use this style?" ) ) ) {
            return selected;
        }
    }
}

void avatar::randomize( const bool random_scenario, points_left &points, bool play_now )
{

    const int max_trait_points = get_option<int>( "MAX_TRAIT_POINTS" );
    // Reset everything to the defaults to have a clean state.
    *this = avatar();

    male = ( rng( 1, 100 ) > 50 );
    if( !MAP_SHARING::isSharing() ) {
        play_now ? pick_name() : pick_name( true );
    } else {
        name = MAP_SHARING::getUsername();
    }
    bool cities_enabled = world_generator->active_world->WORLD_OPTIONS["CITY_SIZE"].getValue() != "0";
    if( random_scenario ) {
        std::vector<const scenario *> scenarios;
        for( const auto &scen : scenario::get_all() ) {
            if( !scen.has_flag( "CHALLENGE" ) &&
                ( !scen.has_flag( "CITY_START" ) || cities_enabled ) ) {
                scenarios.emplace_back( &scen );
            }
        }
        g->scen = random_entry( scenarios );
    } else if( !cities_enabled ) {
        static const string_id<scenario> wilderness_only_scenario( "wilderness" );
        g->scen = &wilderness_only_scenario.obj();
    }

    prof = g->scen->weighted_random_profession();
    start_location = g->scen->random_start_location();

    str_max = rng( 6, HIGH_STAT - 2 );
    dex_max = rng( 6, HIGH_STAT - 2 );
    int_max = rng( 6, HIGH_STAT - 2 );
    per_max = rng( 6, HIGH_STAT - 2 );
    points.stat_points = points.stat_points - str_max - dex_max - int_max - per_max;
    points.skill_points = points.skill_points - prof->point_cost() - g->scen->point_cost();
    // The default for each stat is 8, and that default does not cost any points.
    // Values below give points back, values above require points. The line above has removed
    // to many points, therefore they are added back.
    points.stat_points += 8 * 4;

    int num_gtraits = 0;
    int num_btraits = 0;
    int tries = 0;
    add_traits( points ); // adds mandatory profession/scenario traits.
    for( const trait_id &mut : mutations.get_mutations() ) {
        const mutation_branch &mut_info = mut.obj();
        if( mut_info.profession ) {
            continue;
        }
        // Scenario/profession traits do not cost any points, but they are counted toward
        // the limit (MAX_TRAIT_POINTS)
        if( mut_info.points >= 0 ) {
            num_gtraits += mut_info.points;
        } else {
            num_btraits -= mut_info.points;
        }
    }

    /* The loops variable is used to prevent the algorithm running in an infinite loop */
    unsigned int loops = 0;

    while( loops <= 100000 && ( !points.is_valid() || rng( -3, 20 ) > points.skill_points_left() ) ) {
        loops++;
        trait_id rn;
        if( num_btraits < max_trait_points && one_in( 3 ) ) {
            tries = 0;
            do {
                rn = random_bad_trait();
                tries++;
            } while( ( mutations.has_trait( rn ) || num_btraits - rn->points > max_trait_points ) &&
                     tries < 5 );

            if( tries < 5 && !mutations.has_conflicting_trait( rn ) ) {
                mutations.toggle_trait( *this, rn );
                points.trait_points -= rn->points;
                num_btraits -= rn->points;
            }
        } else {
            switch( rng( 1, 4 ) ) {
                case 1:
                    if( str_max > 5 ) {
                        str_max--;
                        points.stat_points++;
                    }
                    break;
                case 2:
                    if( dex_max > 5 ) {
                        dex_max--;
                        points.stat_points++;
                    }
                    break;
                case 3:
                    if( int_max > 5 ) {
                        int_max--;
                        points.stat_points++;
                    }
                    break;
                case 4:
                    if( per_max > 5 ) {
                        per_max--;
                        points.stat_points++;
                    }
                    break;
            }
        }
    }

    loops = 0;
    while( points.has_spare() && loops <= 100000 ) {
        const bool allow_stats = points.stat_points_left() > 0;
        const bool allow_traits = points.trait_points_left() > 0 && num_gtraits < max_trait_points;
        int r = rng( 1, 9 );
        trait_id rn;
        switch( r ) {
            case 1:
            case 2:
            case 3:
            case 4:
                if( allow_traits ) {
                    rn = random_good_trait();
                    auto &mdata = rn.obj();
                    if( !mutations.has_trait( rn ) && points.trait_points_left() >= mdata.points &&
                        num_gtraits + mdata.points <= max_trait_points && !mutations.has_conflicting_trait( rn ) ) {
                        mutations.toggle_trait( *this, rn );
                        points.trait_points -= mdata.points;
                        num_gtraits += mdata.points;
                    }
                    break;
                }
            /* fallthrough */
            case 5:
                if( allow_stats ) {
                    switch( rng( 1, 4 ) ) {
                        case 1:
                            if( str_max < HIGH_STAT ) {
                                str_max++;
                                points.stat_points--;
                            } else if( points.stat_points_left() >= 2 && str_max < MAX_STAT ) {
                                str_max++;
                                points.stat_points = points.stat_points - 2;
                            }
                            break;
                        case 2:
                            if( dex_max < HIGH_STAT ) {
                                dex_max++;
                                points.stat_points--;
                            } else if( points.stat_points_left() >= 2 && dex_max < MAX_STAT ) {
                                dex_max++;
                                points.stat_points = points.stat_points - 2;
                            }
                            break;
                        case 3:
                            if( int_max < HIGH_STAT ) {
                                int_max++;
                                points.stat_points--;
                            } else if( points.stat_points_left() >= 2 && int_max < MAX_STAT ) {
                                int_max++;
                                points.stat_points = points.stat_points - 2;
                            }
                            break;
                        case 4:
                            if( per_max < HIGH_STAT ) {
                                per_max++;
                                points.stat_points--;
                            } else if( points.stat_points_left() >= 2 && per_max < MAX_STAT ) {
                                per_max++;
                                points.stat_points = points.stat_points - 2;
                            }
                            break;
                    }
                    break;
                }
            /* fallthrough */
            case 6:
            case 7:
            case 8:
            case 9:
                const skill_id aSkill = Skill::random_skill();
                const int level = get_skill_level( aSkill );

                if( level < points.skill_points_left() && level < MAX_SKILL && ( level <= MAX_SKILL ||
                        loops > 10000 ) ) {
                    points.skill_points -= skill_increment_cost( *this, aSkill );
                    // For balance reasons, increasing a skill from level 0 gives you 1 extra level for free
                    set_skill_level( aSkill, ( level == 0 ? 2 : level + 1 ) );
                }
                break;
        }
        loops++;
    }
}

bool avatar::create( character_type type, const std::string &tempname )
{
    weapon = item( "null", 0 );

    prof = profession::generic();
    g->scen = scenario::generic();

    catacurses::window w;
    if( type != PLTYPE_NOW && type != PLTYPE_FULL_RANDOM ) {
        w = catacurses::newwin( TERMY, TERMX, point_zero );
    }

    int tab = 0;
    points_left points = points_left();

    switch( type ) {
        case PLTYPE_CUSTOM:
            break;
        case PLTYPE_RANDOM: //random scenario, default name if exist
            randomize( true, points );
            tab = NEWCHAR_TAB_MAX;
            break;
        case PLTYPE_NOW: //default world, fixed scenario, random name
            randomize( false, points, true );
            break;
        case PLTYPE_FULL_RANDOM: //default world, random scenario, random name
            randomize( true, points, true );
            break;
        case PLTYPE_TEMPLATE:
            if( !load_template( tempname, points ) ) {
                return false;
            }
            // We want to prevent recipes known by the template from being applied to the
            // new character. The recipe list will be rebuilt when entering the game.
            learned_recipes->clear();
            tab = NEWCHAR_TAB_MAX;
            break;
    }

    auto nameExists = [&]( const std::string & name ) {
        return world_generator->active_world->save_exists( save_t::from_player_name( name ) ) &&
               !query_yn( _( "A character with the name '%s' already exists in this world.\n"
                             "Saving will override the already existing character.\n\n"
                             "Continue anyways?" ), name );
    };

    const bool allow_reroll = type == PLTYPE_RANDOM;
    tab_direction result = tab_direction::QUIT;
    do {
        if( !w ) {
            // assert( type == PLTYPE_NOW );
            // no window is created because "Play now"  does not require any configuration
            if( nameExists( name ) ) {
                return false;
            }

            break;
        }
        werase( w );
        wrefresh( w );
        switch( tab ) {
            case 0:
                result = set_points( w, *this, points );
                break;
            case 1:
                result = set_scenario( w, *this, points, result );
                break;
            case 2:
                result = set_profession( w, *this, points, result );
                break;
            case 3:
                result = set_stats( w, *this, points );
                break;
            case 4:
                result = set_traits( w, *this, points );
                break;
            case 5:
                result = set_skills( w, *this, points );
                break;
            case 6:
                result = set_description( w, *this, allow_reroll, points );
                break;
        }

        switch( result ) {
            case tab_direction::NONE:
                break;
            case tab_direction::FORWARD:
                tab++;
                break;
            case tab_direction::BACKWARD:
                tab--;
                break;
            case tab_direction::QUIT:
                tab = -1;
                break;
        }

        if( !( tab >= 0 && tab <= NEWCHAR_TAB_MAX ) ) {
            if( tab != -1 && nameExists( name ) ) {
                tab = NEWCHAR_TAB_MAX;
            } else {
                break;
            }
        }

    } while( true );

    if( tab < 0 ) {
        return false;
    }

    save_template( *this, _( "Last Character" ), points );

    recalc_hp();
    for( int i = 0; i < num_hp_parts; i++ ) {
        hp_cur[i] = hp_max[i];
    }

    if( mutations.has_trait( trait_id( "SMELLY" ) ) ) {
        scent = 800;
    }
    if( mutations.has_trait( trait_id( "WEAKSCENT" ) ) ) {
        scent = 300;
    }

    weapon = item( "null", 0 );

    // Grab the skills from the profession, if there are any
    // We want to do this before the recipes
    for( auto &e : prof->skills() ) {
        mod_skill_level( e.first, e.second );
    }

    // setup staring bank money
    cash = rng( -200000, 200000 );

    if( mutations.has_trait( trait_id( "XS" ) ) ) {
        set_stored_kcal( 10000 );
        mutations.toggle_trait( *this, trait_id( "XS" ) );
    }
    if( mutations.has_trait( trait_id( "XXXL" ) ) ) {
        set_stored_kcal( 125000 );
        mutations.toggle_trait( *this, trait_id( "XXXL" ) );
    }

    // Learn recipes
    for( const auto &e : recipe_dict ) {
        const auto &r = e.second;
        if( !r.has_flag( "SECRET" ) && !knows_recipe( &r ) && has_recipe_requirements( r ) ) {
            learn_recipe( &r );
        }
    }
    for( mtype_id elem : prof->pets() ) {
        starting_pets.push_back( elem );
    }
    std::list<item> prof_items = prof->items( male, mutations.get_mutations() );

    for( item &it : prof_items ) {
        if( it.has_flag( "WET" ) ) {
            it.active = true;
            it.item_counter = 450; // Give it some time to dry off
        }
        // TODO: debugmsg if food that isn't a seed is inedible
        if( it.has_flag( "no_auto_equip" ) ) {
            it.unset_flag( "no_auto_equip" );
            inv.push_back( it );
        } else if( it.has_flag( "auto_wield" ) ) {
            it.unset_flag( "auto_wield" );
            if( !is_armed() ) {
                wield( it );
            } else {
                inv.push_back( it );
            }
        } else if( it.is_armor() ) {
            // TODO: debugmsg if wearing fails
            wear_item( it, false );
        } else {
            inv.push_back( it );
        }
        if( it.is_book() ) {
            items_identified.insert( it.typeId() );
        }
    }

    std::vector<addiction> prof_addictions = prof->addictions();
    for( std::vector<addiction>::const_iterator iter = prof_addictions.begin();
         iter != prof_addictions.end(); ++iter ) {
        addictions.push_back( *iter );
    }

    for( auto &bio : prof->CBMs() ) {
        add_bionic( bio );
    }
    // Adjust current energy level to maximum
    set_power_level( get_max_power_level() );

    for( const trait_id &t : mutations.get_base_traits() ) {
        std::vector<matype_id> styles;
        for( auto &s : t->initial_ma_styles ) {
            if( !has_martialart( s ) ) {
                styles.push_back( s );
            }
        }
        if( !styles.empty() ) {
            werase( w );
            wrefresh( w );
            const auto ma_type = choose_ma_style( type, styles );
            ma_styles.push_back( ma_type );
            style_selected = ma_type;
        }
    }

    // Activate some mutations right from the start.
    for( const trait_id &mut : mutations.get_mutations() ) {
        const mutation_branch &branch = mut.obj();
        if( branch.starts_active ) {
            mutations.get_trait_data( mut ).powered = true;
        }
    }

    prof->learn_spells( *this );

    // Ensure that persistent morale effects (e.g. Optimist) are present at the start.
    apply_persistent_morale();

    return true;
}

static void draw_character_tabs( const catacurses::window &w, const std::string &sTab )
{
    std::vector<std::string> tab_captions = {
        _( "POINTS" ),
        _( "SCENARIO" ),
        _( "PROFESSION" ),
        _( "STATS" ),
        _( "TRAITS" ),
        _( "SKILLS" ),
        _( "DESCRIPTION" ),
    };

    draw_tabs( w, tab_captions, sTab );
    draw_border_below_tabs( w );

    for( int i = 1; i < TERMX - 1; i++ ) {
        mvwputch( w, point( i, 4 ), BORDER_COLOR, LINE_OXOX );
    }
    mvwputch( w, point( 0, 4 ), BORDER_COLOR, LINE_XXXO ); // |-
    mvwputch( w, point( TERMX - 1, 4 ), BORDER_COLOR, LINE_XOXX ); // -|
}
static void draw_points( const catacurses::window &w, points_left &points, int netPointCost = 0 )
{
    // Clear line (except borders)
    mvwprintz( w, point( 2, 3 ), c_black, std::string( getmaxx( w ) - 3, ' ' ) );
    std::string points_msg = points.to_string();
    int pMsg_length = utf8_width( remove_color_tags( points_msg ), true );
    nc_color color = c_light_gray;
    print_colored_text( w, point( 2, 3 ), color, c_light_gray, points_msg );
    if( netPointCost > 0 ) {
        mvwprintz( w, point( pMsg_length + 2, 3 ), c_red, "(-%d)", std::abs( netPointCost ) );
    } else if( netPointCost < 0 ) {
        mvwprintz( w, point( pMsg_length + 2, 3 ), c_green, "(+%d)", std::abs( netPointCost ) );
    }
}

template <class Compare>
void draw_sorting_indicator( const catacurses::window &w_sorting, const input_context &ctxt,
                             Compare sorter )
{
    const auto sort_order = sorter.sort_by_points ? _( "points" ) : _( "name" );
    const auto sort_text = string_format(
                               _( "<color_white>Sort by: </color>%1$s (Press <color_light_green>%2$s</color> to change)" ),
                               sort_order, ctxt.get_desc( "SORT" ) );
    fold_and_print( w_sorting, point_zero, ( TERMX / 2 ), c_light_gray, sort_text );
}

tab_direction set_points( const catacurses::window &w, avatar &, points_left &points )
{
    tab_direction retval = tab_direction::NONE;
    const int content_height = TERMY - 6;
    catacurses::window w_description = catacurses::newwin( content_height, TERMX - 35,
                                       point( 31 + getbegx( w ), 5 + getbegy( w ) ) );

    draw_character_tabs( w, _( "POINTS" ) );

    input_context ctxt( "NEW_CHAR_POINTS" );
    ctxt.register_cardinal();
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM" );

    const std::string point_pool = get_option<std::string>( "CHARACTER_POINT_POOLS" );

    using point_limit_tuple = std::tuple<points_left::point_limit, std::string, std::string>;
    std::vector<point_limit_tuple> opts;

    const point_limit_tuple multi_pool = std::make_tuple( points_left::MULTI_POOL,
                                         _( "Multiple pools" ),
                                         _( "Stats, traits and skills have separate point pools.\n"
                                            "Putting stat points into traits and skills is allowed and putting trait points into skills is allowed.\n"
                                            "Scenarios and professions affect skill point pool" ) );

    const point_limit_tuple one_pool = std::make_tuple( points_left::ONE_POOL, _( "Single pool" ),
                                       _( "Stats, traits and skills share a single point pool." ) );

    const point_limit_tuple freeform = std::make_tuple( points_left::FREEFORM, _( "Freeform" ),
                                       _( "No point limits are enforced" ) );

    if( point_pool == "multi_pool" ) {
        opts = {{ multi_pool }};
    } else if( point_pool == "no_freeform" ) {
        opts = {{ multi_pool, one_pool }};
    } else {
        opts = {{ multi_pool, one_pool, freeform }};
    }

    int highlighted = 0;

    do {
        if( highlighted < 0 ) {
            highlighted = opts.size() - 1;
        } else if( highlighted >= static_cast<int>( opts.size() ) ) {
            highlighted = 0;
        }

        const auto &cur_opt = opts[highlighted];

        draw_points( w, points );

        // Clear the bottom of the screen.
        werase( w_description );

        for( int i = 0; i < static_cast<int>( opts.size() ); i++ ) {
            nc_color color = ( points.limit == std::get<0>( opts[i] ) ? COL_SKILL_USED : c_light_gray );
            if( highlighted == i ) {
                color = hilite( color );
            }
            mvwprintz( w, point( 2, 5 + i ), color, std::get<1>( opts[i] ) );
        }

        fold_and_print( w_description, point_zero, getmaxx( w_description ),
                        COL_SKILL_USED, std::get<2>( cur_opt ) );

        wrefresh( w );
        wrefresh( w_description );
        const std::string action = ctxt.handle_input();
        if( action == "DOWN" ) {
            highlighted++;
        } else if( action == "UP" ) {
            highlighted--;
        } else if( action == "PREV_TAB" && query_yn( _( "Return to main menu?" ) ) ) {
            retval = tab_direction::BACKWARD;
        } else if( action == "NEXT_TAB" ) {
            retval = tab_direction::FORWARD;
        } else if( action == "QUIT" && query_yn( _( "Return to main menu?" ) ) ) {
            retval = tab_direction::QUIT;
        } else if( action == "HELP_KEYBINDINGS" ) {
            // Need to redraw since the help window obscured everything.
            draw_character_tabs( w, _( "POINTS" ) );
        } else if( action == "CONFIRM" ) {
            points.limit = std::get<0>( cur_opt );
        }
    } while( retval == tab_direction::NONE );

    return retval;
}

tab_direction set_stats( const catacurses::window &w, avatar &u, points_left &points )
{
    unsigned char sel = 1;
    const int iSecondColumn = 27;
    input_context ctxt( "NEW_CHAR_STATS" );
    ctxt.register_cardinal();
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "QUIT" );
    int read_spd;
    catacurses::window w_description = catacurses::newwin( 8, TERMX - iSecondColumn - 1,
                                       point( iSecondColumn + getbegx( w ), 6 + getbegy( w ) ) );
    // There is no map loaded currently, so any access to the map will
    // fail (player::suffer, called from player::reset_stats), might access
    // the map:
    // There are traits that check/change the radioactivity on the map,
    // that check if in sunlight...
    // Setting the position to -1 ensures that the INBOUNDS check in
    // map.cpp is triggered. This check prevents access to invalid position
    // on the map (like -1,0) and instead returns a dummy default value.
    u.setx( -1 );
    u.reset();

    do {
        werase( w );
        draw_character_tabs( w, _( "STATS" ) );
        fold_and_print( w, point( 2, 16 ), getmaxx( w ) - 4, COL_NOTE_MINOR,
                        _( "    <color_light_green>%s</color> / <color_light_green>%s</color> to select a statistic.\n"
                           "    <color_light_green>%s</color> to increase the statistic.\n"
                           "    <color_light_green>%s</color> to decrease the statistic." ),
                        ctxt.get_desc( "UP" ), ctxt.get_desc( "DOWN" ),
                        ctxt.get_desc( "RIGHT" ), ctxt.get_desc( "LEFT" )
                      );

        mvwprintz( w, point( 2, TERMY - 4 ), COL_NOTE_MAJOR,
                   _( "%s lets you view and alter keybindings." ), ctxt.get_desc( "HELP_KEYBINDINGS" ) );
        mvwprintz( w, point( 2, TERMY - 3 ), COL_NOTE_MAJOR, _( "%s takes you to the next tab." ),
                   ctxt.get_desc( "NEXT_TAB" ) );
        mvwprintz( w, point( 2, TERMY - 2 ), COL_NOTE_MAJOR, _( "%s returns you to the main menu." ),
                   ctxt.get_desc( "PREV_TAB" ) );

        // This is description line, meaning its length excludes first column and border
        const std::string clear_line( getmaxx( w ) - iSecondColumn - 1, ' ' );
        mvwprintz( w, point( iSecondColumn, 3 ), c_black, clear_line );
        for( int i = 6; i < 13; i++ ) {
            mvwprintz( w, point( iSecondColumn, i ), c_black, clear_line );
        }

        draw_points( w, points );

        mvwprintz( w, point( 2, 6 ), c_light_gray, _( "Strength:" ) );
        mvwprintz( w, point( 16, 6 ), c_light_gray, "%2d", u.str_max );
        mvwprintz( w, point( 2, 7 ), c_light_gray, _( "Dexterity:" ) );
        mvwprintz( w, point( 16, 7 ), c_light_gray, "%2d", u.dex_max );
        mvwprintz( w, point( 2, 8 ), c_light_gray, _( "Intelligence:" ) );
        mvwprintz( w, point( 16, 8 ), c_light_gray, "%2d", u.int_max );
        mvwprintz( w, point( 2, 9 ), c_light_gray, _( "Perception:" ) );
        mvwprintz( w, point( 16, 9 ), c_light_gray, "%2d", u.per_max );

        werase( w_description );
        switch( sel ) {
            case 1:
                mvwprintz( w, point( 2, 6 ), COL_STAT_ACT, _( "Strength:" ) );
                mvwprintz( w, point( 16, 6 ), COL_STAT_ACT, "%2d", u.str_max );
                if( u.str_max >= HIGH_STAT ) {
                    mvwprintz( w, point( iSecondColumn, 3 ), c_light_red,
                               _( "Increasing Str further costs 2 points." ) );
                }
                u.recalc_hp();
                mvwprintz( w_description, point_zero, COL_STAT_NEUTRAL, _( "Base HP: %d" ), u.hp_max[0] );
                // NOLINTNEXTLINE(cata-use-named-point-constants)
                mvwprintz( w_description, point( 0, 1 ), COL_STAT_NEUTRAL, _( "Carry weight: %.1f %s" ),
                           convert_weight( u.weight_capacity() ), weight_units() );
                mvwprintz( w_description, point( 0, 2 ), COL_STAT_BONUS, _( "Melee damage bonus: %.1f" ),
                           u.bonus_damage( false ) );
                fold_and_print( w_description, point( 0, 4 ), getmaxx( w_description ) - 1, COL_STAT_NEUTRAL,
                                _( "Strength also makes you more resistant to many diseases and poisons, and makes actions which require brute force more effective." ) );
                break;

            case 2:
                mvwprintz( w, point( 2, 7 ), COL_STAT_ACT, _( "Dexterity:" ) );
                mvwprintz( w, point( 16, 7 ), COL_STAT_ACT, "%2d", u.dex_max );
                if( u.dex_max >= HIGH_STAT ) {
                    mvwprintz( w, point( iSecondColumn, 3 ), c_light_red,
                               _( "Increasing Dex further costs 2 points." ) );
                }
                mvwprintz( w_description, point_zero, COL_STAT_BONUS, _( "Melee to-hit bonus: +%.2f" ),
                           u.get_hit_base() );
                // NOLINTNEXTLINE(cata-use-named-point-constants)
                mvwprintz( w_description, point( 0, 1 ), COL_STAT_BONUS,
                           _( "Throwing penalty per target's dodge: +%d" ),
                           u.throw_dispersion_per_dodge( false ) );
                if( u.ranged_dex_mod() != 0 ) {
                    mvwprintz( w_description, point( 0, 2 ), COL_STAT_PENALTY, _( "Ranged penalty: -%d" ),
                               std::abs( u.ranged_dex_mod() ) );
                }
                fold_and_print( w_description, point( 0, 4 ), getmaxx( w_description ) - 1, COL_STAT_NEUTRAL,
                                _( "Dexterity also enhances many actions which require finesse." ) );
                break;

            case 3:
                mvwprintz( w, point( 2, 8 ), COL_STAT_ACT, _( "Intelligence:" ) );
                mvwprintz( w, point( 16, 8 ), COL_STAT_ACT, "%2d", u.int_max );
                if( u.int_max >= HIGH_STAT ) {
                    mvwprintz( w, point( iSecondColumn, 3 ), c_light_red,
                               _( "Increasing Int further costs 2 points." ) );
                }
                read_spd = u.read_speed( false );
                mvwprintz( w_description, point_zero, ( read_spd == 100 ? COL_STAT_NEUTRAL :
                                                        ( read_spd < 100 ? COL_STAT_BONUS : COL_STAT_PENALTY ) ),
                           _( "Read times: %d%%" ), read_spd );
                // NOLINTNEXTLINE(cata-use-named-point-constants)
                mvwprintz( w_description, point( 0, 1 ), COL_STAT_PENALTY, _( "Skill rust: %d%%" ),
                           u.rust_rate( false ) );
                mvwprintz( w_description, point( 0, 2 ), COL_STAT_BONUS, _( "Crafting bonus: %2d%%" ),
                           u.get_int() );
                fold_and_print( w_description, point( 0, 4 ), getmaxx( w_description ) - 1, COL_STAT_NEUTRAL,
                                _( "Intelligence is also used when crafting, installing bionics, and interacting with NPCs." ) );
                break;

            case 4:
                mvwprintz( w, point( 2, 9 ), COL_STAT_ACT, _( "Perception:" ) );
                mvwprintz( w, point( 16, 9 ), COL_STAT_ACT, "%2d", u.per_max );
                if( u.per_max >= HIGH_STAT ) {
                    mvwprintz( w, point( iSecondColumn, 3 ), c_light_red,
                               _( "Increasing Per further costs 2 points." ) );
                }
                if( u.ranged_per_mod() > 0 ) {
                    mvwprintz( w_description, point_zero, COL_STAT_PENALTY, _( "Aiming penalty: -%d" ),
                               u.ranged_per_mod() );
                }
                fold_and_print( w_description, point( 0, 2 ), getmaxx( w_description ) - 1, COL_STAT_NEUTRAL,
                                _( "Perception is also used for detecting traps and other things of interest." ) );
                break;
        }

        wrefresh( w );
        wrefresh( w_description );
        const std::string action = ctxt.handle_input();
        if( action == "DOWN" ) {
            if( sel < 4 ) {
                sel++;
            } else {
                sel = 1;
            }
        } else if( action == "UP" ) {
            if( sel > 1 ) {
                sel--;
            } else {
                sel = 4;
            }
        } else if( action == "LEFT" ) {
            if( sel == 1 && u.str_max > 4 ) {
                if( u.str_max > HIGH_STAT ) {
                    points.stat_points++;
                }
                u.str_max--;
                points.stat_points++;
            } else if( sel == 2 && u.dex_max > 4 ) {
                if( u.dex_max > HIGH_STAT ) {
                    points.stat_points++;
                }
                u.dex_max--;
                points.stat_points++;
            } else if( sel == 3 && u.int_max > 4 ) {
                if( u.int_max > HIGH_STAT ) {
                    points.stat_points++;
                }
                u.int_max--;
                points.stat_points++;
            } else if( sel == 4 && u.per_max > 4 ) {
                if( u.per_max > HIGH_STAT ) {
                    points.stat_points++;
                }
                u.per_max--;
                points.stat_points++;
            }
        } else if( action == "RIGHT" ) {
            if( sel == 1 && u.str_max < MAX_STAT ) {
                points.stat_points--;
                if( u.str_max >= HIGH_STAT ) {
                    points.stat_points--;
                }
                u.str_max++;
            } else if( sel == 2 && u.dex_max < MAX_STAT ) {
                points.stat_points--;
                if( u.dex_max >= HIGH_STAT ) {
                    points.stat_points--;
                }
                u.dex_max++;
            } else if( sel == 3 && u.int_max < MAX_STAT ) {
                points.stat_points--;
                if( u.int_max >= HIGH_STAT ) {
                    points.stat_points--;
                }
                u.int_max++;
            } else if( sel == 4 && u.per_max < MAX_STAT ) {
                points.stat_points--;
                if( u.per_max >= HIGH_STAT ) {
                    points.stat_points--;
                }
                u.per_max++;
            }
        } else if( action == "PREV_TAB" ) {
            return tab_direction::BACKWARD;
        } else if( action == "NEXT_TAB" ) {
            return tab_direction::FORWARD;
        } else if( action == "HELP_KEYBINDINGS" ) {
            // Need to redraw since the help window obscured everything.
            draw_character_tabs( w, _( "STATS" ) );
        } else if( action == "QUIT" && query_yn( _( "Return to main menu?" ) ) ) {
            return tab_direction::QUIT;
        }
    } while( true );
}

tab_direction set_traits( const catacurses::window &w, avatar &u, points_left &points )
{
    const int max_trait_points = get_option<int>( "MAX_TRAIT_POINTS" );

    draw_character_tabs( w, _( "TRAITS" ) );

    catacurses::window w_description =
        catacurses::newwin( 3, TERMX - 2, point( 1 + getbegx( w ), TERMY - 4 + getbegy( w ) ) );
    // Track how many good / bad POINTS we have; cap both at MAX_TRAIT_POINTS
    int num_good = 0;
    int num_bad = 0;

    std::vector<trait_id> vStartingTraits[3];

    for( auto &traits_iter : mutation_branch::get_all() ) {
        // Don't list blacklisted traits
        if( mutation_branch::trait_is_blacklisted( traits_iter.id ) ) {
            continue;
        }

        // Always show profession locked traits, regardless of if they are forbidden
        const std::vector<trait_id> proftraits = u.prof->get_locked_traits();
        const bool is_proftrait = std::find( proftraits.begin(), proftraits.end(),
                                             traits_iter.id ) != proftraits.end();
        // We show all starting traits, even if we can't pick them, to keep the interface consistent.
        if( traits_iter.startingtrait || g->scen->traitquery( traits_iter.id ) || is_proftrait ) {
            if( traits_iter.points > 0 ) {
                vStartingTraits[0].push_back( traits_iter.id );

                if( u.mutations.has_trait( traits_iter.id ) ) {
                    num_good += traits_iter.points;
                }
            } else if( traits_iter.points < 0 ) {
                vStartingTraits[1].push_back( traits_iter.id );

                if( u.mutations.has_trait( traits_iter.id ) ) {
                    num_bad += traits_iter.points;
                }
            } else {
                vStartingTraits[2].push_back( traits_iter.id );
            }
        }
    }
    //If the third page is empty, only use the first two.
    const int used_pages = vStartingTraits[2].empty() ? 2 : 3;

    for( auto &vStartingTrait : vStartingTraits ) {
        std::sort( vStartingTrait.begin(), vStartingTrait.end(), trait_display_sort );
    }

    nc_color col_on_act, col_off_act, col_on_pas, col_off_pas, hi_on, hi_off, col_tr;

    const size_t iContentHeight = TERMY - 9;
    int iCurWorkingPage = 0;

    int iStartPos[3] = { 0, 0, 0 };
    int iCurrentLine[3] = { 0, 0, 0 };
    size_t traits_size[3];
    for( int i = 0; i < 3; i++ ) {
        traits_size[i] = vStartingTraits[i].size();
    }

    const size_t page_width = std::min( ( TERMX - 4 ) / used_pages, 38 );

    input_context ctxt( "NEW_CHAR_TRAITS" );
    ctxt.register_cardinal();
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );

    do {
        draw_points( w, points );
        if( !points.is_freeform() ) {
            mvwprintz( w, point( 26, 3 ), c_light_green, "%2d/%-2d", num_good, max_trait_points );
            mvwprintz( w, point( 32, 3 ), c_light_red, "%3d/-%-2d ", num_bad, max_trait_points );
        }

        // Clear the bottom of the screen.
        werase( w_description );

        for( int iCurrentPage = 0; iCurrentPage < 3; iCurrentPage++ ) { //Good/Bad
            switch( iCurrentPage ) {
                case 0:
                    col_on_act = COL_TR_GOOD_ON_ACT;
                    col_off_act = COL_TR_GOOD_OFF_ACT;
                    col_on_pas = COL_TR_GOOD_ON_PAS;
                    col_off_pas = COL_TR_GOOD_OFF_PAS;
                    col_tr = COL_TR_GOOD;
                    hi_on = hilite( col_on_act );
                    hi_off = hilite( col_off_act );
                    break;
                case 1:
                    col_on_act = COL_TR_BAD_ON_ACT;
                    col_off_act = COL_TR_BAD_OFF_ACT;
                    col_on_pas = COL_TR_BAD_ON_PAS;
                    col_off_pas = COL_TR_BAD_OFF_PAS;
                    col_tr = COL_TR_BAD;
                    hi_on = hilite( col_on_act );
                    hi_off = hilite( col_off_act );
                    break;
                default:
                    col_on_act = COL_TR_NEUT_ON_ACT;
                    col_off_act = COL_TR_NEUT_OFF_ACT;
                    col_on_pas = COL_TR_NEUT_ON_PAS;
                    col_off_pas = COL_TR_NEUT_OFF_PAS;
                    col_tr = COL_TR_NEUT;
                    hi_on = hilite( col_on_act );
                    hi_off = hilite( col_off_act );
                    break;
            }
            int start_y = iStartPos[iCurrentPage];
            int cur_line_y = iCurrentLine[iCurrentPage];
            calcStartPos( start_y, cur_line_y, iContentHeight,
                          traits_size[iCurrentPage] );

            //Draw Traits
            for( int i = start_y; i < static_cast<int>( traits_size[iCurrentPage] ); i++ ) {
                if( i < start_y ||
                    i >= start_y + static_cast<int>( std::min( traits_size[iCurrentPage], iContentHeight ) ) ) {
                    continue;
                }
                auto &cur_trait = vStartingTraits[iCurrentPage][i];
                auto &mdata = cur_trait.obj();
                if( cur_line_y == i && iCurrentPage == iCurWorkingPage ) {
                    // Clear line from 41 to end of line (minus border)
                    mvwprintz( w, point( 41, 3 ), c_light_gray, std::string( getmaxx( w ) - 41 - 1, ' ' ) );
                    int points = mdata.points;
                    bool negativeTrait = points < 0;
                    if( negativeTrait ) {
                        points *= -1;
                    }
                    mvwprintz( w,  point( 41, 3 ), col_tr, ngettext( "%s %s %d point", "%s %s %d points", points ),
                               mdata.name(),
                               negativeTrait ? _( "earns" ) : _( "costs" ),
                               points );
                    fold_and_print( w_description, point_zero,
                                    TERMX - 2, col_tr,
                                    mdata.desc() );
                }

                nc_color cLine = col_off_pas;
                if( iCurWorkingPage == iCurrentPage ) {
                    cLine = col_off_act;
                    if( cur_line_y == i ) {
                        cLine = hi_off;
                        if( u.mutations.has_conflicting_trait( cur_trait ) ) {
                            cLine = hilite( c_dark_gray );
                        } else if( u.mutations.has_trait( cur_trait ) ) {
                            cLine = hi_on;
                        }
                    } else {
                        if( u.mutations.has_conflicting_trait( cur_trait ) || g->scen->is_forbidden_trait( cur_trait ) ) {
                            cLine = c_dark_gray;

                        } else if( u.mutations.has_trait( cur_trait ) ) {
                            cLine = col_on_act;
                        }
                    }
                } else if( u.mutations.has_trait( cur_trait ) ) {
                    cLine = col_on_pas;

                } else if( u.mutations.has_conflicting_trait( cur_trait ) ||
                           g->scen->is_forbidden_trait( cur_trait ) ) {
                    cLine = c_light_gray;
                }

                // Clear the line
                int cur_line_y = 5 + i - start_y;
                int cur_line_x = 2 + iCurrentPage * page_width;
                mvwprintz( w, point( cur_line_x, cur_line_y ), c_light_gray, std::string( page_width, ' ' ) );
                mvwprintz( w, point( cur_line_x, cur_line_y ), cLine, utf8_truncate( mdata.name(),
                           page_width - 2 ) );
            }

            for( int i = 0; i < used_pages; i++ ) {
                draw_scrollbar( w, iCurrentLine[i], iContentHeight, traits_size[i], point( page_width * i, 5 ) );
            }
        }

        wrefresh( w );
        wrefresh( w_description );
        const std::string action = ctxt.handle_input();
        if( action == "LEFT" ) {
            iCurWorkingPage--;
            if( iCurWorkingPage < 0 ) {
                iCurWorkingPage = used_pages - 1;
            }
        } else if( action == "RIGHT" ) {
            iCurWorkingPage++;
            if( iCurWorkingPage > used_pages - 1 ) {
                iCurWorkingPage = 0;
            }
        } else if( action == "UP" ) {
            if( iCurrentLine[iCurWorkingPage] == 0 ) {
                iCurrentLine[iCurWorkingPage] = traits_size[iCurWorkingPage] - 1;
            } else {
                iCurrentLine[iCurWorkingPage]--;
            }
        } else if( action == "DOWN" ) {
            iCurrentLine[iCurWorkingPage]++;
            if( static_cast<size_t>( iCurrentLine[iCurWorkingPage] ) >= traits_size[iCurWorkingPage] ) {
                iCurrentLine[iCurWorkingPage] = 0;
            }
        } else if( action == "CONFIRM" ) {
            int inc_type = 0;
            const trait_id cur_trait = vStartingTraits[iCurWorkingPage][iCurrentLine[iCurWorkingPage]];
            const mutation_branch &mdata = cur_trait.obj();
            if( u.mutations.has_trait( cur_trait ) ) {

                inc_type = -1;

                if( g->scen->is_locked_trait( cur_trait ) ) {
                    inc_type = 0;
                    popup( _( "Your scenario of %s prevents you from removing this trait." ),
                           g->scen->gender_appropriate_name( u.male ) );
                } else if( u.prof->is_locked_trait( cur_trait ) ) {
                    inc_type = 0;
                    popup( _( "Your profession of %s prevents you from removing this trait." ),
                           u.prof->gender_appropriate_name( u.male ) );
                }
            } else if( u.mutations.has_conflicting_trait( cur_trait ) ) {
                popup( _( "You already picked a conflicting trait!" ) );
            } else if( g->scen->is_forbidden_trait( cur_trait ) ) {
                popup( _( "The scenario you picked prevents you from taking this trait!" ) );
            } else if( iCurWorkingPage == 0 && num_good + mdata.points >
                       max_trait_points && !points.is_freeform() ) {
                popup( ngettext( "Sorry, but you can only take %d point of advantages.",
                                 "Sorry, but you can only take %d points of advantages.", max_trait_points ),
                       max_trait_points );

            } else if( iCurWorkingPage != 0 && num_bad + mdata.points <
                       -max_trait_points && !points.is_freeform() ) {
                popup( ngettext( "Sorry, but you can only take %d point of disadvantages.",
                                 "Sorry, but you can only take %d points of disadvantages.", max_trait_points ),
                       max_trait_points );

            } else {
                inc_type = 1;
            }

            //inc_type is either -1 or 1, so we can just multiply by it to invert
            if( inc_type != 0 ) {
                u.mutations.toggle_trait( u, cur_trait );
                points.trait_points -= mdata.points * inc_type;
                if( iCurWorkingPage == 0 ) {
                    num_good += mdata.points * inc_type;
                } else {
                    num_bad += mdata.points * inc_type;
                }
            }
        } else if( action == "PREV_TAB" ) {
            return tab_direction::BACKWARD;
        } else if( action == "NEXT_TAB" ) {
            return tab_direction::FORWARD;
        } else if( action == "HELP_KEYBINDINGS" ) {
            // Need to redraw since the help window obscured everything.
            draw_character_tabs( w, _( "TRAITS" ) );
        } else if( action == "QUIT" && query_yn( _( "Return to main menu?" ) ) ) {
            return tab_direction::QUIT;
        }
    } while( true );
}

struct {
    bool sort_by_points = true;
    bool male;
    /** @related player */
    bool operator()( const string_id<profession> &a, const string_id<profession> &b ) {
        // The generic ("Unemployed") profession should be listed first.
        const profession *gen = profession::generic();
        if( &b.obj() == gen ) {
            return false;
        } else if( &a.obj() == gen ) {
            return true;
        }

        if( sort_by_points ) {
            return a->point_cost() < b->point_cost();
        } else {
            return a->gender_appropriate_name( male ) <
                   b->gender_appropriate_name( male );
        }
    }
} profession_sorter;

/** Handle the profession tab of the character generation menu */
tab_direction set_profession( const catacurses::window &w, avatar &u, points_left &points,
                              const tab_direction direction )
{
    draw_character_tabs( w, _( "PROFESSION" ) );
    int cur_id = 0;
    tab_direction retval = tab_direction::NONE;
    int desc_offset = 0;
    const int iContentHeight = TERMY - 10;
    int iStartPos = 0;

    catacurses::window w_description =
        catacurses::newwin( 4, TERMX - 2, point( 1 + getbegx( w ), TERMY - 5 + getbegy( w ) ) );

    catacurses::window w_sorting =
        catacurses::newwin( 1, 55, point( ( TERMX / 2 ) + getbegx( w ), 5 + getbegy( w ) ) );
    catacurses::window w_genderswap =
        catacurses::newwin( 1, 55, point( ( TERMX / 2 ) + getbegx( w ), 6 + getbegy( w ) ) );
    catacurses::window w_items =
        catacurses::newwin( iContentHeight - 2, 55,
                            point( ( TERMX / 2 ) + getbegx( w ), 7 + getbegy( w ) ) );

    input_context ctxt( "NEW_CHAR_PROFESSIONS" );
    ctxt.register_cardinal();
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "CHANGE_GENDER" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "SORT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "QUIT" );

    bool recalc_profs = true;
    int profs_length = 0;
    std::string filterstring;
    std::vector<string_id<profession>> sorted_profs;

    if( direction == tab_direction::FORWARD ) {
        points.skill_points -= u.prof->point_cost();
    }

    do {
        if( recalc_profs ) {
            sorted_profs = g->scen->permitted_professions();
            const auto new_end = std::remove_if( sorted_profs.begin(),
            sorted_profs.end(), [&]( const string_id<profession> &arg ) {
                return !lcmatch( arg->gender_appropriate_name( u.male ), filterstring );
            } );
            sorted_profs.erase( new_end, sorted_profs.end() );
            profs_length = sorted_profs.size();
            if( profs_length == 0 ) {
                popup( _( "Nothing found." ) ); // another case of black box in tiles
                filterstring.clear();
                continue;
            }

            // Sort professions by points.
            // profession_display_sort() keeps "unemployed" at the top.
            profession_sorter.male = u.male;
            std::stable_sort( sorted_profs.begin(), sorted_profs.end(), profession_sorter );

            // Select the current profession, if possible.
            for( int i = 0; i < profs_length; ++i ) {
                if( sorted_profs[i] == u.prof->ident() ) {
                    cur_id = i;
                    break;
                }
            }
            if( cur_id > profs_length - 1 ) {
                cur_id = 0;
            }

            // Draw filter indicator
            for( int i = 1; i < TERMX - 1; i++ ) {
                mvwputch( w, point( i, TERMY - 1 ), BORDER_COLOR, LINE_OXOX );
            }
            const auto filter_indicator = filterstring.empty() ? _( "no filter" )
                                          : filterstring;
            mvwprintz( w, point( 2, getmaxy( w ) - 1 ), c_light_gray, "<%s>", filter_indicator );

            recalc_profs = false;
        }

        int netPointCost = sorted_profs[cur_id]->point_cost() - u.prof->point_cost();
        bool can_pick = sorted_profs[cur_id]->can_pick( u, points.skill_points_left() );
        const std::string clear_line( getmaxx( w ) - 2, ' ' );

        // Clear the bottom of the screen and header.
        werase( w_description );
        mvwprintz( w, point( 1, 3 ), c_light_gray, clear_line );

        int pointsForProf = sorted_profs[cur_id]->point_cost();
        bool negativeProf = pointsForProf < 0;
        if( negativeProf ) {
            pointsForProf *= -1;
        }
        // Draw header.
        draw_points( w, points, netPointCost );
        std::string prof_msg_temp;
        if( negativeProf ) {
            //~ 1s - profession name, 2d - current character points.
            prof_msg_temp = ngettext( "Profession %1$s earns %2$d point",
                                      "Profession %1$s earns %2$d points",
                                      pointsForProf );
        } else {
            //~ 1s - profession name, 2d - current character points.
            prof_msg_temp = ngettext( "Profession %1$s costs %2$d point",
                                      "Profession %1$s costs %2$d points",
                                      pointsForProf );
        }

        int pMsg_length = utf8_width( remove_color_tags( points.to_string() ) );
        mvwprintz( w, point( pMsg_length + 9, 3 ), can_pick ? c_green : c_light_red, prof_msg_temp.c_str(),
                   sorted_profs[cur_id]->gender_appropriate_name( u.male ),
                   pointsForProf );

        fold_and_print( w_description, point_zero, TERMX - 2, c_green,
                        sorted_profs[cur_id]->description( u.male ) );

        //Draw options
        calcStartPos( iStartPos, cur_id, iContentHeight, profs_length );
        const int end_pos = iStartPos + ( ( iContentHeight > profs_length ) ?
                                          profs_length : iContentHeight );
        int i;
        for( i = iStartPos; i < end_pos; i++ ) {
            mvwprintz( w, point( 2, 5 + i - iStartPos ), c_light_gray,
                       "                                             " ); // Clear the line
            nc_color col;
            if( u.prof != &sorted_profs[i].obj() ) {
                col = ( sorted_profs[i] == sorted_profs[cur_id] ? h_light_gray : c_light_gray );
            } else {
                col = ( sorted_profs[i] == sorted_profs[cur_id] ? hilite( COL_SKILL_USED ) : COL_SKILL_USED );
            }
            mvwprintz( w, point( 2, 5 + i - iStartPos ), col,
                       sorted_profs[i]->gender_appropriate_name( u.male ) );
        }
        //Clear rest of space in case stuff got filtered out
        for( ; i < iStartPos + iContentHeight; ++i ) {
            mvwprintz( w, point( 2, 5 + i - iStartPos ), c_light_gray,
                       "                                             " ); // Clear the line
        }

        std::ostringstream buffer;
        // Profession addictions
        const auto prof_addictions = sorted_profs[cur_id]->addictions();
        if( !prof_addictions.empty() ) {
            buffer << colorize( _( "Addictions:" ), c_light_blue ) << "\n";
            for( const auto &a : prof_addictions ) {
                const auto format = pgettext( "set_profession_addictions", "%1$s (%2$d)" );
                buffer << string_format( format, addiction_name( a ), a.intensity ) << "\n";
            }
        }

        // Profession traits
        const auto prof_traits = sorted_profs[cur_id]->get_locked_traits();
        buffer << colorize( _( "Profession traits:" ), c_light_blue ) << "\n";
        if( prof_traits.empty() ) {
            buffer << pgettext( "set_profession_trait", "None" ) << "\n";
        } else {
            for( const auto &t : prof_traits ) {
                buffer << mutation_branch::get_name( t ) << "\n";
            }
        }

        // Profession skills
        const auto prof_skills = sorted_profs[cur_id]->skills();
        buffer << colorize( _( "Profession skills:" ), c_light_blue ) << "\n";
        if( prof_skills.empty() ) {
            buffer << pgettext( "set_profession_skill", "None" ) << "\n";
        } else {
            for( const auto &sl : prof_skills ) {
                const auto format = pgettext( "set_profession_skill", "%1$s (%2$d)" );
                buffer << string_format( format, sl.first.obj().name(), sl.second ) << "\n";
            }
        }

        // Profession items
        const auto prof_items = sorted_profs[cur_id]->items( u.male, u.mutations.get_mutations() );
        buffer << colorize( _( "Profession items:" ), c_light_blue ) << "\n";
        if( prof_items.empty() ) {
            buffer << pgettext( "set_profession_item", "None" ) << "\n";
        } else {
            // TODO: If the item group is randomized *at all*, these will be different each time
            // and it won't match what you actually start with
            // TODO: Put like items together like the inventory does, so we don't have to scroll
            // through a list of a dozen forks.
            std::ostringstream buffer_wielded;
            std::ostringstream buffer_worn;
            std::ostringstream buffer_inventory;
            for( const auto &it : prof_items ) {
                if( it.has_flag( "no_auto_equip" ) ) {
                    buffer_inventory << it.display_name() << "\n";
                } else if( it.has_flag( "auto_wield" ) ) {
                    buffer_wielded << it.display_name() << "\n";
                } else if( it.is_armor() ) {
                    buffer_worn << it.display_name() << "\n";
                } else {
                    buffer_inventory << it.display_name() << "\n";
                }
            }
            buffer << colorize( _( "Wielded:" ), c_cyan ) << "\n";
            buffer << ( !buffer_wielded.str().empty() ? buffer_wielded.str() :
                        pgettext( "set_profession_item_wielded", "None\n" ) );
            buffer << colorize( _( "Worn:" ), c_cyan ) << "\n";
            buffer << ( !buffer_worn.str().empty() ? buffer_worn.str() :
                        pgettext( "set_profession_item_worn", "None\n" ) );
            buffer << colorize( _( "Inventory:" ), c_cyan ) << "\n";
            buffer << ( !buffer_inventory.str().empty() ? buffer_inventory.str() :
                        pgettext( "set_profession_item_inventory", "None\n" ) );
        }

        // Profession bionics, active bionics shown first
        auto prof_CBMs = sorted_profs[cur_id]->CBMs();
        std::sort( begin( prof_CBMs ), end( prof_CBMs ), []( const bionic_id & a, const bionic_id & b ) {
            return a->activated && !b->activated;
        } );
        buffer << colorize( _( "Profession bionics:" ), c_light_blue ) << "\n";
        if( prof_CBMs.empty() ) {
            buffer << pgettext( "set_profession_bionic", "None" ) << "\n";
        } else {
            for( const auto &b : prof_CBMs ) {
                const auto &cbm = b.obj();

                if( cbm.activated && cbm.toggled ) {
                    buffer << cbm.name << " (" << _( "toggled" ) << ")\n";
                } else if( cbm.activated ) {
                    buffer << cbm.name << " (" << _( "activated" ) << ")\n";
                } else {
                    buffer << cbm.name << "\n";
                }
            }
        }
        // Profession pet
        cata::optional<mtype_id> montype;
        if( !sorted_profs[cur_id]->pets().empty() ) {
            buffer << colorize( _( "Pets:" ), c_light_blue ) << "\n";
            for( auto elem : sorted_profs[cur_id]->pets() ) {
                monster mon( elem );
                buffer << mon.get_name() << "\n";
            }
        }
        // Profession spells
        if( !sorted_profs[cur_id]->spells().empty() ) {
            buffer << colorize( _( "Spells:" ), c_light_blue ) << "\n";
            for( const std::pair<spell_id, int> spell_pair : sorted_profs[cur_id]->spells() ) {
                buffer << spell_pair.first->name << _( " level " ) << spell_pair.second << "\n";
            }
        }
        werase( w_items );
        const auto scroll_msg = string_format(
                                    _( "Press <color_light_green>%1$s</color> or <color_light_green>%2$s</color> to scroll." ),
                                    ctxt.get_desc( "LEFT" ),
                                    ctxt.get_desc( "RIGHT" ) );
        const int iheight = print_scrollable( w_items, desc_offset, buffer.str(), c_light_gray,
                                              scroll_msg );

        werase( w_sorting );
        draw_sorting_indicator( w_sorting, ctxt, profession_sorter );

        werase( w_genderswap );
        //~ Gender switch message. 1s - change key name, 2s - profession name.
        std::string g_switch_msg = u.male ? _( "Press %1$s to switch to %2$s( female )." ) :
                                   _( "Press %1$s to switch to %2$s(male)." );
        mvwprintz( w_genderswap, point_zero, c_magenta, g_switch_msg.c_str(),
                   ctxt.get_desc( "CHANGE_GENDER" ),
                   sorted_profs[cur_id]->gender_appropriate_name( !u.male ) );

        draw_scrollbar( w, cur_id, iContentHeight, profs_length, point( 0, 5 ) );

        wrefresh( w );
        wrefresh( w_description );
        wrefresh( w_items );
        wrefresh( w_genderswap );
        wrefresh( w_sorting );

        const std::string action = ctxt.handle_input();
        if( action == "DOWN" ) {
            cur_id++;
            if( cur_id > static_cast<int>( profs_length ) - 1 ) {
                cur_id = 0;
            }
            desc_offset = 0;
        } else if( action == "UP" ) {
            cur_id--;
            if( cur_id < 0 ) {
                cur_id = profs_length - 1;
            }
            desc_offset = 0;
        } else if( action == "LEFT" ) {
            if( desc_offset > 0 ) {
                desc_offset--;
            }
        } else if( action == "RIGHT" ) {
            if( desc_offset < iheight ) {
                desc_offset++;
            }
        } else if( action == "CONFIRM" ) {
            // Remove traits from the previous profession
            for( const trait_id &old_trait : u.prof->get_locked_traits() ) {
                u.mutations.toggle_trait( u, old_trait );
            }
            u.prof = &sorted_profs[cur_id].obj();
            // Add traits for the new profession (and perhaps scenario, if, for example,
            // both the scenario and old profession require the same trait)
            u.add_traits( points );
            points.skill_points -= netPointCost;
        } else if( action == "CHANGE_GENDER" ) {
            u.male = !u.male;
            profession_sorter.male = u.male;
            if( !profession_sorter.sort_by_points ) {
                std::sort( sorted_profs.begin(), sorted_profs.end(), profession_sorter );
            }
        } else if( action == "PREV_TAB" ) {
            retval = tab_direction::BACKWARD;
        } else if( action == "NEXT_TAB" ) {
            retval = tab_direction::FORWARD;
        } else if( action == "SORT" ) {
            profession_sorter.sort_by_points = !profession_sorter.sort_by_points;
            recalc_profs = true;
        } else if( action == "FILTER" ) {
            string_input_popup()
            .title( _( "Search:" ) )
            .width( 60 )
            .description( _( "Search by profession name." ) )
            .edit( filterstring );
            recalc_profs = true;
        } else if( action == "HELP_KEYBINDINGS" ) {
            // Need to redraw since the help window obscured everything.
            draw_character_tabs( w, _( "PROFESSION" ) );
        } else if( action == "QUIT" && query_yn( _( "Return to main menu?" ) ) ) {
            retval = tab_direction::QUIT;
        }

    } while( retval == tab_direction::NONE );

    return retval;
}

/**
 * @return The skill points to consume when a skill is increased (by one level) from the
 * current level.
 *
 * @note: There is one exception: if the current level is 0, it can be boosted by 2 levels for 1 point.
 */
static int skill_increment_cost( const Character &u, const skill_id &skill )
{
    return std::max( 1, ( u.get_skill_level( skill ) + 1 ) / 2 );
}

tab_direction set_skills( const catacurses::window &w, avatar &u, points_left &points )
{
    draw_character_tabs( w, _( "SKILLS" ) );
    const int iContentHeight = TERMY - 6;
    catacurses::window w_description = catacurses::newwin( iContentHeight, TERMX - 35,
                                       point( 31 + getbegx( w ), 5 + getbegy( w ) ) );

    auto sorted_skills = Skill::get_skills_sorted_by( []( const Skill & a, const Skill & b ) {
        return a.name() < b.name();
    } );

    const int num_skills = sorted_skills.size();
    int cur_offset = 0;
    int cur_pos = 0;
    const Skill *currentSkill = sorted_skills[cur_pos];
    int selected = 0;

    input_context ctxt( "NEW_CHAR_SKILLS" );
    ctxt.register_cardinal();
    ctxt.register_action( "SCROLL_DOWN" );
    ctxt.register_action( "SCROLL_UP" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );

    std::map<skill_id, int> prof_skills;
    const auto &pskills = u.prof->skills();

    std::copy( pskills.begin(), pskills.end(),
               std::inserter( prof_skills, prof_skills.begin() ) );

    do {
        draw_points( w, points );
        // Clear the bottom of the screen.
        werase( w_description );
        mvwprintz( w, point( 31, 3 ), c_light_gray, std::string( getmaxx( w ) - 32, ' ' ) );
        const int cost = skill_increment_cost( u, currentSkill->ident() );
        mvwprintz( w, point( 31, 3 ), points.skill_points_left() >= cost ? COL_SKILL_USED : c_light_red,
                   ngettext( "Upgrading %s costs %d point", "Upgrading %s costs %d points", cost ),
                   currentSkill->name(), cost );

        // We want recipes from profession skills displayed, but without boosting the skills
        // Copy the skills, and boost the copy
        SkillLevelMap with_prof_skills = u.get_all_skills();
        for( const auto &sk : prof_skills ) {
            with_prof_skills.mod_skill_level( sk.first, sk.second );
        }

        std::map<std::string, std::vector<std::pair<std::string, int> > > recipes;
        for( const auto &e : recipe_dict ) {
            const auto &r = e.second;
            //Find out if the current skill and its level is in the requirement list
            auto req_skill = r.required_skills.find( currentSkill->ident() );
            int skill = req_skill != r.required_skills.end() ? req_skill->second : 0;
            bool would_autolearn_recipe =
                recipe_dict.all_autolearn().count( &r ) &&
                with_prof_skills.meets_skill_requirements( r.autolearn_requirements );

            if( !would_autolearn_recipe && !r.never_learn &&
                ( r.skill_used == currentSkill->ident() || skill > 0 ) &&
                with_prof_skills.has_recipe_requirements( r ) )  {

                recipes[r.skill_used->name()].emplace_back(
                    r.result_name(),
                    ( skill > 0 ) ? skill : r.difficulty
                );
            }
        }

        std::string rec_disp;

        for( auto &elem : recipes ) {
            std::sort( elem.second.begin(), elem.second.end(),
                       []( const std::pair<std::string, int> &lhs,
            const std::pair<std::string, int> &rhs ) {
                return lhs.second < rhs.second ||
                       ( lhs.second == rhs.second && lhs.first < rhs.first );
            } );

            const std::string rec_temp = enumerate_as_string( elem.second.begin(), elem.second.end(),
            []( const std::pair<std::string, int> &rec ) {
                return string_format( "%s (%d)", rec.first, rec.second );
            } );

            if( elem.first == currentSkill->name() ) {
                rec_disp = "\n\n" + colorize( rec_temp, c_brown ) + rec_disp;
            } else {
                rec_disp += "\n\n" + colorize( "[" + elem.first + "]\n" + rec_temp, c_light_gray );
            }
        }

        rec_disp = currentSkill->description() + rec_disp;

        const auto vFolded = foldstring( rec_disp, getmaxx( w_description ) );
        int iLines = vFolded.size();

        if( selected < 0 ) {
            selected = 0;
        } else if( iLines < iContentHeight ) {
            selected = 0;
        } else if( selected >= iLines - iContentHeight ) {
            selected = iLines - iContentHeight;
        }

        fold_and_print_from( w_description, point_zero, getmaxx( w_description ),
                             selected, COL_SKILL_USED, rec_disp );

        draw_scrollbar( w, selected, iContentHeight, iLines,
                        point( getmaxx( w ) - 1, 5 ), BORDER_COLOR, true );

        calcStartPos( cur_offset, cur_pos, iContentHeight, num_skills );
        for( int i = cur_offset; i < num_skills && i - cur_offset < iContentHeight; ++i ) {
            const int y = 5 + i - cur_offset;
            const Skill *thisSkill = sorted_skills[i];
            // Clear the line
            mvwprintz( w, point( 2, y ), c_light_gray, std::string( getmaxx( w ) - 3, ' ' ) );
            if( u.get_skill_level( thisSkill->ident() ) == 0 ) {
                mvwprintz( w, point( 2, y ),
                           ( i == cur_pos ? h_light_gray : c_light_gray ), thisSkill->name() );
            } else {
                mvwprintz( w, point( 2, y ),
                           ( i == cur_pos ? hilite( COL_SKILL_USED ) : COL_SKILL_USED ),
                           thisSkill->name() );
                wprintz( w, ( i == cur_pos ? hilite( COL_SKILL_USED ) : COL_SKILL_USED ),
                         " ( %d )", u.get_skill_level( thisSkill->ident() ) );
            }
            for( auto &prof_skill : u.prof->skills() ) {
                if( prof_skill.first == thisSkill->ident() ) {
                    wprintz( w, ( i == cur_pos ? h_white : c_white ), " (+%d)",
                             static_cast<int>( prof_skill.second ) );
                    break;
                }
            }
        }

        draw_scrollbar( w, cur_pos, iContentHeight, num_skills, point( 0, 5 ) );

        wrefresh( w );
        wrefresh( w_description );
        const std::string action = ctxt.handle_input();
        if( action == "DOWN" ) {
            cur_pos++;
            if( cur_pos >= num_skills ) {
                cur_pos = 0;
            }
            currentSkill = sorted_skills[cur_pos];
        } else if( action == "UP" ) {
            cur_pos--;
            if( cur_pos < 0 ) {
                cur_pos = num_skills - 1;
            }
            currentSkill = sorted_skills[cur_pos];
        } else if( action == "LEFT" ) {
            const int level = u.get_skill_level( currentSkill->ident() );
            if( level > 0 ) {
                // For balance reasons, increasing a skill from level 0 gives 1 extra level for free, but
                // decreasing it from level 2 forfeits the free extra level (thus changes it to 0)
                u.mod_skill_level( currentSkill->ident(), level == 2 ? -2 : -1 );
                // Done *after* the decrementing to get the original cost for incrementing back.
                points.skill_points += skill_increment_cost( u, currentSkill->ident() );
            }
        } else if( action == "RIGHT" ) {
            const int level = u.get_skill_level( currentSkill->ident() );
            if( level < MAX_SKILL ) {
                points.skill_points -= skill_increment_cost( u, currentSkill->ident() );
                // For balance reasons, increasing a skill from level 0 gives 1 extra level for free
                u.mod_skill_level( currentSkill->ident(), level == 0 ? +2 : +1 );
            }
        } else if( action == "SCROLL_DOWN" ) {
            selected++;
        } else if( action == "SCROLL_UP" ) {
            selected--;
        } else if( action == "PREV_TAB" ) {
            return tab_direction::BACKWARD;
        } else if( action == "NEXT_TAB" ) {
            return tab_direction::FORWARD;
        } else if( action == "HELP_KEYBINDINGS" ) {
            // Need to redraw since the help window obscured everything.
            draw_character_tabs( w, _( "SKILLS" ) );
        } else if( action == "QUIT" && query_yn( _( "Return to main menu?" ) ) ) {
            return tab_direction::QUIT;
        }
    } while( true );
}

struct {
    bool sort_by_points = true;
    bool male;
    bool cities_enabled;
    /** @related player */
    bool operator()( const scenario *a, const scenario *b ) {
        if( cities_enabled ) {
            // The generic ("Unemployed") profession should be listed first.
            const scenario *gen = scenario::generic();
            if( b == gen ) {
                return false;
            } else if( a == gen ) {
                return true;
            }
        }

        if( !cities_enabled && a->has_flag( "CITY_START" ) != b->has_flag( "CITY_START" ) ) {
            return a->has_flag( "CITY_START" ) < b->has_flag( "CITY_START" );
        } else if( sort_by_points ) {
            return a->point_cost() < b->point_cost();
        } else {
            return a->gender_appropriate_name( male ) <
                   b->gender_appropriate_name( male );
        }
    }
} scenario_sorter;

tab_direction set_scenario( const catacurses::window &w, avatar &u, points_left &points,
                            const tab_direction direction )
{
    draw_character_tabs( w, _( "SCENARIO" ) );

    int cur_id = 0;
    tab_direction retval = tab_direction::NONE;
    const int iContentHeight = TERMY - 10;
    int iStartPos = 0;

    catacurses::window w_description =
        catacurses::newwin( 4, TERMX - 2, point( 1 + getbegx( w ), TERMY - 5 + getbegy( w ) ) );
    catacurses::window w_sorting =
        catacurses::newwin( 2, ( TERMX / 2 ) - 1,
                            point( ( TERMX / 2 ) + getbegx( w ), 5 + getbegy( w ) ) );
    catacurses::window w_profession =
        catacurses::newwin( 4, ( TERMX / 2 ) - 1,
                            point( ( TERMX / 2 ) + getbegx( w ), 7 + getbegy( w ) ) );
    catacurses::window w_location =
        catacurses::newwin( 3, ( TERMX / 2 ) - 1,
                            point( ( TERMX / 2 ) + getbegx( w ), 11 + getbegy( w ) ) );

    // 9 = 2 + 4 + 3, so we use rest of space for flags
    catacurses::window w_flags =
        catacurses::newwin( iContentHeight - 9, ( TERMX / 2 ) - 1,
                            point( ( TERMX / 2 ) + getbegx( w ), 14 + getbegy( w ) ) );

    input_context ctxt( "NEW_CHAR_SCENARIOS" );
    ctxt.register_cardinal();
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "SORT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "QUIT" );

    bool recalc_scens = true;
    int scens_length = 0;
    std::string filterstring;
    std::vector<const scenario *> sorted_scens;

    if( direction == tab_direction::BACKWARD ) {
        points.skill_points += u.prof->point_cost();
    }

    do {
        if( recalc_scens ) {
            sorted_scens.clear();
            auto &wopts = world_generator->active_world->WORLD_OPTIONS;
            for( const auto &scen : scenario::get_all() ) {
                if( !lcmatch( scen.gender_appropriate_name( u.male ), filterstring ) ) {
                    continue;
                }
                sorted_scens.push_back( &scen );
            }
            scens_length = sorted_scens.size();
            if( scens_length == 0 ) {
                popup( _( "Nothing found." ) ); // another case of black box in tiles
                filterstring.clear();
                continue;
            }

            // Sort scenarios by points.
            // scenario_display_sort() keeps "Evacuee" at the top.
            scenario_sorter.male = u.male;
            scenario_sorter.cities_enabled = wopts["CITY_SIZE"].getValue() != "0";
            std::stable_sort( sorted_scens.begin(), sorted_scens.end(), scenario_sorter );

            // If city size is 0 but the current scenario requires cities reset the scenario
            if( !scenario_sorter.cities_enabled && g->scen->has_flag( "CITY_START" ) ) {
                reset_scenario( u, sorted_scens[0] );
                points.init_from_options();
                points.skill_points -= sorted_scens[cur_id]->point_cost();
            }

            // Select the current scenario, if possible.
            for( int i = 0; i < scens_length; ++i ) {
                if( sorted_scens[i]->ident() == g->scen->ident() ) {
                    cur_id = i;
                    break;
                }
            }
            if( cur_id > scens_length - 1 ) {
                cur_id = 0;
            }

            // Draw filter indicator
            for( int i = 1; i < TERMX - 1; i++ ) {
                mvwputch( w, point( i, TERMY - 1 ), BORDER_COLOR, LINE_OXOX );
            }
            const auto filter_indicator = filterstring.empty() ? _( "no filter" )
                                          : filterstring;
            mvwprintz( w, point( 2, getmaxy( w ) - 1 ), c_light_gray, "<%s>", filter_indicator );

            recalc_scens = false;
        }

        int netPointCost = sorted_scens[cur_id]->point_cost() - g->scen->point_cost();
        bool can_pick = sorted_scens[cur_id]->can_pick( *g->scen, points.skill_points_left() );
        const std::string clear_line( getmaxx( w_description ), ' ' );

        // Clear the bottom of the screen and header.
        werase( w_description );
        mvwprintz( w, point( 1, 3 ), c_light_gray, clear_line );

        int pointsForScen = sorted_scens[cur_id]->point_cost();
        bool negativeScen = pointsForScen < 0;
        if( negativeScen ) {
            pointsForScen *= -1;
        }

        // Draw header.
        draw_points( w, points, netPointCost );

        std::string scen_msg_temp;
        if( negativeScen ) {
            //~ 1s - scenario name, 2d - current character points.
            scen_msg_temp = ngettext( "Scenario %1$s earns %2$d point",
                                      "Scenario %1$s earns %2$d points",
                                      pointsForScen );
        } else {
            //~ 1s - scenario name, 2d - current character points.
            scen_msg_temp = ngettext( "Scenario %1$s costs %2$d point",
                                      "Scenario %1$s cost %2$d points",
                                      pointsForScen );
        }

        int pMsg_length = utf8_width( remove_color_tags( points.to_string() ) );
        mvwprintz( w, point( pMsg_length + 9, 3 ), can_pick ? c_green : c_light_red, scen_msg_temp.c_str(),
                   sorted_scens[cur_id]->gender_appropriate_name( u.male ),
                   pointsForScen );

        const std::string scenDesc = sorted_scens[cur_id]->description( u.male );

        if( sorted_scens[cur_id]->has_flag( "CITY_START" ) && !scenario_sorter.cities_enabled ) {
            const std::string scenUnavailable =
                _( "This scenario is not available in this world due to city size settings." );
            fold_and_print( w_description, point_zero, TERMX - 2, c_red, scenUnavailable );
            // NOLINTNEXTLINE(cata-use-named-point-constants)
            fold_and_print( w_description, point( 0, 1 ), TERMX - 2, c_green, scenDesc );
        } else {
            fold_and_print( w_description, point_zero, TERMX - 2, c_green, scenDesc );
        }

        //Draw options
        calcStartPos( iStartPos, cur_id, iContentHeight, scens_length );
        const int end_pos = iStartPos + ( ( iContentHeight > scens_length ) ?
                                          scens_length : iContentHeight );
        int i;
        for( i = iStartPos; i < end_pos; i++ ) {
            mvwprintz( w, point( 2, 5 + i - iStartPos ), c_light_gray,
                       "                                             " );
            nc_color col;
            if( g->scen != sorted_scens[i] ) {
                if( sorted_scens[i] == sorted_scens[cur_id] && ( sorted_scens[i]->has_flag( "CITY_START" ) &&
                        !scenario_sorter.cities_enabled ) ) {
                    col = h_dark_gray;
                } else if( sorted_scens[i] != sorted_scens[cur_id] && ( sorted_scens[i]->has_flag( "CITY_START" ) &&
                           !scenario_sorter.cities_enabled ) ) {
                    col = c_dark_gray;
                } else {
                    col = ( sorted_scens[i] == sorted_scens[cur_id] ? h_light_gray : c_light_gray );
                }
            } else {
                col = ( sorted_scens[i] == sorted_scens[cur_id] ? hilite( COL_SKILL_USED ) : COL_SKILL_USED );
            }
            mvwprintz( w, point( 2, 5 + i - iStartPos ), col,
                       sorted_scens[i]->gender_appropriate_name( u.male ) );

        }
        //Clear rest of space in case stuff got filtered out
        for( ; i < iStartPos + iContentHeight; ++i ) {
            mvwprintz( w, point( 2, 5 + i - iStartPos ), c_light_gray,
                       "                                             " ); // Clear the line
        }

        werase( w_sorting );
        werase( w_profession );
        werase( w_location );
        werase( w_flags );

        draw_sorting_indicator( w_sorting, ctxt, scenario_sorter );

        mvwprintz( w_profession, point_zero, COL_HEADER, _( "Professions:" ) );
        wprintz( w_profession, c_light_gray,
                 string_format( _( "\n%s" ), sorted_scens[cur_id]->prof_count_str() ) );
        wprintz( w_profession, c_light_gray, _( ", default:\n" ) );

        auto psorter = profession_sorter;
        psorter.sort_by_points = true;
        const auto permitted = sorted_scens[cur_id]->permitted_professions();
        const auto default_prof = *std::min_element( permitted.begin(), permitted.end(), psorter );
        const int prof_points = default_prof->point_cost();
        wprintz( w_profession, c_light_gray,
                 default_prof->gender_appropriate_name( u.male ) );
        if( prof_points > 0 ) {
            wprintz( w_profession, c_red, " (-%d)", prof_points );
        } else if( prof_points < 0 ) {
            wprintz( w_profession, c_green, " (+%d)", -prof_points );
        }

        mvwprintz( w_location, point_zero, COL_HEADER, _( "Scenario Location:" ) );
        wprintz( w_location, c_light_gray, ( "\n" ) );
        wprintz( w_location, c_light_gray, sorted_scens[cur_id]->start_name() );

        mvwprintz( w_flags, point_zero, COL_HEADER, _( "Scenario Flags:" ) );
        wprintz( w_flags, c_light_gray, ( "\n" ) );

        if( sorted_scens[cur_id]->has_flag( "SPR_START" ) ) {
            wprintz( w_flags, c_light_gray, _( "Spring start" ) );
            wprintz( w_flags, c_light_gray, ( "\n" ) );
        } else if( sorted_scens[cur_id]->has_flag( "SUM_START" ) ) {
            wprintz( w_flags, c_light_gray, _( "Summer start" ) );
            wprintz( w_flags, c_light_gray, ( "\n" ) );
        } else if( sorted_scens[cur_id]->has_flag( "AUT_START" ) ) {
            wprintz( w_flags, c_light_gray, _( "Autumn start" ) );
            wprintz( w_flags, c_light_gray, ( "\n" ) );
        } else if( sorted_scens[cur_id]->has_flag( "WIN_START" ) ) {
            wprintz( w_flags, c_light_gray, _( "Winter start" ) );
            wprintz( w_flags, c_light_gray, ( "\n" ) );
        } else if( sorted_scens[cur_id]->has_flag( "SUM_ADV_START" ) ) {
            wprintz( w_flags, c_light_gray, _( "Next summer start" ) );
            wprintz( w_flags, c_light_gray, ( "\n" ) );
        }

        if( sorted_scens[cur_id]->has_flag( "INFECTED" ) ) {
            wprintz( w_flags, c_light_gray, _( "Infected player" ) );
            wprintz( w_flags, c_light_gray, ( "\n" ) );
        }
        if( sorted_scens[cur_id]->has_flag( "BAD_DAY" ) ) {
            wprintz( w_flags, c_light_gray, _( "Drunk and sick player" ) );
            wprintz( w_flags, c_light_gray, ( "\n" ) );
        }
        if( sorted_scens[cur_id]->has_flag( "FIRE_START" ) ) {
            wprintz( w_flags, c_light_gray, _( "Fire nearby" ) );
            wprintz( w_flags, c_light_gray, ( "\n" ) );
        }
        if( sorted_scens[cur_id]->has_flag( "SUR_START" ) ) {
            wprintz( w_flags, c_light_gray, _( "Zombies nearby" ) );
            wprintz( w_flags, c_light_gray, ( "\n" ) );
        }
        if( sorted_scens[cur_id]->has_flag( "HELI_CRASH" ) ) {
            wprintz( w_flags, c_light_gray, _( "Various limb wounds" ) );
            wprintz( w_flags, c_light_gray, ( "\n" ) );
        }
        if( get_option<std::string>( "STARTING_NPC" ) == "scenario" &&
            sorted_scens[cur_id]->has_flag( "LONE_START" ) ) {
            wprintz( w_flags, c_light_gray, _( "No starting NPC" ) );
            wprintz( w_flags, c_light_gray, ( "\n" ) );
        }

        draw_scrollbar( w, cur_id, iContentHeight, scens_length, point( 0, 5 ) );
        wrefresh( w );
        wrefresh( w_description );
        wrefresh( w_sorting );
        wrefresh( w_profession );
        wrefresh( w_location );
        wrefresh( w_flags );

        const std::string action = ctxt.handle_input();
        if( action == "DOWN" ) {
            cur_id++;
            if( cur_id > scens_length - 1 ) {
                cur_id = 0;
            }
        } else if( action == "UP" ) {
            cur_id--;
            if( cur_id < 0 ) {
                cur_id = scens_length - 1;
            }
        } else if( action == "CONFIRM" ) {
            if( sorted_scens[cur_id]->has_flag( "CITY_START" ) && !scenario_sorter.cities_enabled ) {
                continue;
            }
            reset_scenario( u, sorted_scens[cur_id] );
            points.init_from_options();
            points.skill_points -= sorted_scens[cur_id]->point_cost();
        } else if( action == "PREV_TAB" ) {
            retval = tab_direction::BACKWARD;
        } else if( action == "NEXT_TAB" ) {
            retval = tab_direction::FORWARD;
        } else if( action == "SORT" ) {
            scenario_sorter.sort_by_points = !scenario_sorter.sort_by_points;
            recalc_scens = true;
        } else if( action == "FILTER" ) {
            string_input_popup()
            .title( _( "Search:" ) )
            .width( 60 )
            .description( _( "Search by scenario name." ) )
            .edit( filterstring );
            recalc_scens = true;
        } else if( action == "HELP_KEYBINDINGS" ) {
            // Need to redraw since the help window obscured everything.
            draw_character_tabs( w, _( "SCENARIO" ) );
        } else if( action == "QUIT" && query_yn( _( "Return to main menu?" ) ) ) {
            retval = tab_direction::QUIT;
        }
    } while( retval == tab_direction::NONE );

    return retval;
}

tab_direction set_description( const catacurses::window &w, avatar &you, const bool allow_reroll,
                               points_left &points )
{
    draw_character_tabs( w, _( "DESCRIPTION" ) );

    catacurses::window w_name =
        catacurses::newwin( 2, 42, point( getbegx( w ) + 2, getbegy( w ) + 5 ) );
    catacurses::window w_gender =
        catacurses::newwin( 2, 33, point( getbegx( w ) + 46, getbegy( w ) + 5 ) );
    catacurses::window w_location =
        catacurses::newwin( 1, TERMX - 3, point( getbegx( w ) + 2, getbegy( w ) + 7 ) );
    catacurses::window w_stats =
        catacurses::newwin( 6, 20, point( getbegx( w ) + 2, getbegy( w ) + 9 ) );
    catacurses::window w_traits =
        catacurses::newwin( 30, 24, point( getbegx( w ) + 22, getbegy( w ) + 9 ) );
    catacurses::window w_scenario =
        catacurses::newwin( 1, TERMX - 47, point( getbegx( w ) + 46, getbegy( w ) + 9 ) );
    catacurses::window w_profession =
        catacurses::newwin( 1, 33, point( getbegx( w ) + 46, getbegy( w ) + 10 ) );
    catacurses::window w_skills =
        catacurses::newwin( 30, 33, point( getbegx( w ) + 46, getbegy( w ) + 11 ) );
    catacurses::window w_guide =
        catacurses::newwin( 4, TERMX - 3, point( getbegx( w ) + 2, TERMY - 5 ) );

    draw_points( w, points );

    const unsigned namebar_pos = 1 + utf8_width( _( "Name:" ) );
    unsigned male_pos = 1 + utf8_width( _( "Gender:" ) );
    unsigned female_pos = 2 + male_pos + utf8_width( _( "Male" ) );
    bool redraw = true;

    input_context ctxt( "NEW_CHAR_DESCRIPTION" );
    ctxt.register_action( "SAVE_TEMPLATE" );
    ctxt.register_action( "PICK_RANDOM_NAME" );
    ctxt.register_action( "CHANGE_GENDER" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "CHOOSE_LOCATION" );
    ctxt.register_action( "REROLL_CHARACTER" );
    ctxt.register_action( "REROLL_CHARACTER_WITH_SCENARIO" );
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_action( "QUIT" );

    uilist select_location;
    select_location.text = _( "Select a starting location." );
    int offset = 0;
    for( const auto &loc : start_location::get_all() ) {
        if( g->scen->allowed_start( loc.ident() ) ) {
            select_location.entries.emplace_back( loc.name() );
            if( loc.ident() == you.start_location ) {
                select_location.selected = offset;
            }
            offset++;
        }
    }
    select_location.setup();
    if( MAP_SHARING::isSharing() ) {
        you.name = MAP_SHARING::getUsername();  // set the current username as default character name
    } else if( !get_option<std::string>( "DEF_CHAR_NAME" ).empty() ) {
        you.name = get_option<std::string>( "DEF_CHAR_NAME" );
    }

    // do not switch IME mode now, but restore previous mode on return
    ime_sentry sentry( ime_sentry::keep );
    do {
        if( redraw ) {
            //Draw the line between editable and non-editable stuff.
            for( int i = 0; i < getmaxx( w ); ++i ) {
                if( i == 0 ) {
                    mvwputch( w, point( i, 8 ), BORDER_COLOR, LINE_XXXO );
                } else if( i == getmaxx( w ) - 1 ) {
                    wputch( w, BORDER_COLOR, LINE_XOXX );
                } else {
                    wputch( w, BORDER_COLOR, LINE_OXOX );
                }
            }
            wrefresh( w );

            wclear( w_stats );
            wclear( w_traits );
            wclear( w_skills );
            wclear( w_guide );

            std::vector<std::string> vStatNames;
            mvwprintz( w_stats, point_zero, COL_HEADER, _( "Stats:" ) );
            vStatNames.push_back( _( "Strength:" ) );
            vStatNames.push_back( _( "Dexterity:" ) );
            vStatNames.push_back( _( "Intelligence:" ) );
            vStatNames.push_back( _( "Perception:" ) );
            int pos = 0;
            for( size_t i = 0; i < vStatNames.size(); i++ ) {
                pos = ( utf8_width( vStatNames[i] ) > pos ?
                        utf8_width( vStatNames[i] ) : pos );
                mvwprintz( w_stats, point( 0, i + 1 ), c_light_gray, vStatNames[i] );
            }
            mvwprintz( w_stats, point( pos + 1, 1 ), c_light_gray, "%2d", you.str_max );
            mvwprintz( w_stats, point( pos + 1, 2 ), c_light_gray, "%2d", you.dex_max );
            mvwprintz( w_stats, point( pos + 1, 3 ), c_light_gray, "%2d", you.int_max );
            mvwprintz( w_stats, point( pos + 1, 4 ), c_light_gray, "%2d", you.per_max );
            wrefresh( w_stats );

            mvwprintz( w_traits, point_zero, COL_HEADER, _( "Traits: " ) );
            const std::vector<trait_id> current_traits = you.mutations.get_base_traits();
            if( current_traits.empty() ) {
                wprintz( w_traits, c_light_red, _( "None!" ) );
            } else {
                for( size_t i = 0; i < current_traits.size(); i++ ) {
                    const trait_id current_trait = current_traits[i];
                    trim_and_print( w_traits, point( 0, i + 1 ), getmaxx( w_traits ) - 1,
                                    current_trait->get_display_color(), current_trait->name() );
                }
            }
            wrefresh( w_traits );

            mvwprintz( w_skills, point_zero, COL_HEADER, _( "Skills:" ) );

            auto skillslist = Skill::get_skills_sorted_by( [&]( const Skill & a, const Skill & b ) {
                const int level_a = you.get_skill_level_object( a.ident() ).exercised_level();
                const int level_b = you.get_skill_level_object( b.ident() ).exercised_level();
                return level_a > level_b || ( level_a == level_b && a.name() < b.name() );
            } );

            int line = 1;
            bool has_skills = false;
            profession::StartingSkillList list_skills = you.prof->skills();
            for( auto &elem : skillslist ) {
                int level = you.get_skill_level( elem->ident() );
                profession::StartingSkillList::iterator i = list_skills.begin();
                while( i != list_skills.end() ) {
                    if( i->first == ( elem )->ident() ) {
                        level += i->second;
                        break;
                    }
                    ++i;
                }

                if( level > 0 ) {
                    mvwprintz( w_skills, point( 0, line ), c_light_gray,
                               elem->name() + ":" );
                    mvwprintz( w_skills, point( 23, line ), c_light_gray, "%-2d", static_cast<int>( level ) );
                    line++;
                    has_skills = true;
                }
            }
            if( !has_skills ) {
                mvwprintz( w_skills, point( utf8_width( _( "Skills:" ) ) + 1, 0 ), c_light_red, _( "None!" ) );
            }
            wrefresh( w_skills );

            mvwprintz( w_guide, point( 0, getmaxy( w_guide ) - 1 ), c_green,
                       _( "Press %s to finish character creation or %s to go back." ),
                       ctxt.get_desc( "NEXT_TAB" ),
                       ctxt.get_desc( "PREV_TAB" ) );
            if( allow_reroll ) {
                mvwprintz( w_guide, point( 0, getmaxy( w_guide ) - 2 ), c_green,
                           _( "Press %s to save character template, %s to re-roll or %s for random scenario." ),
                           ctxt.get_desc( "SAVE_TEMPLATE" ),
                           ctxt.get_desc( "REROLL_CHARACTER" ),
                           ctxt.get_desc( "REROLL_CHARACTER_WITH_SCENARIO" ) );
            } else {
                mvwprintz( w_guide, point( 0, getmaxy( w_guide ) - 2 ), c_green,
                           _( "Press %s to save a template of this character." ),
                           ctxt.get_desc( "SAVE_TEMPLATE" ) );
            }
            wrefresh( w_guide );

            redraw = false;
        }

        //We draw this stuff every loop because this is user-editable
        mvwprintz( w_name, point_zero, c_light_gray, _( "Name:" ) );
        mvwprintz( w_name, point( namebar_pos, 0 ), c_light_gray, "_______________________________" );
        mvwprintz( w_name, point( namebar_pos, 0 ), c_white, you.name );
        wprintz( w_name, h_light_gray, "_" );

        if( !MAP_SHARING::isSharing() ) { // no random names when sharing maps
            // NOLINTNEXTLINE(cata-use-named-point-constants)
            mvwprintz( w_name, point( 0, 1 ), c_light_gray, _( "Press %s to pick a random name." ),
                       ctxt.get_desc( "PICK_RANDOM_NAME" ) );
        }
        wrefresh( w_name );

        mvwprintz( w_gender, point_zero, c_light_gray, _( "Gender:" ) );
        mvwprintz( w_gender, point( male_pos, 0 ), ( you.male ? c_light_red : c_light_gray ), _( "Male" ) );
        mvwprintz( w_gender, point( female_pos, 0 ), ( you.male ? c_light_gray : c_light_red ),
                   _( "Female" ) );
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        mvwprintz( w_gender, point( 0, 1 ), c_light_gray, _( "Press %s to switch gender" ),
                   ctxt.get_desc( "CHANGE_GENDER" ) );
        wrefresh( w_gender );

        const std::string location_prompt = string_format( _( "Press %s to select location." ),
                                            ctxt.get_desc( "CHOOSE_LOCATION" ) );
        const int prompt_offset = utf8_width( location_prompt );
        werase( w_location );
        mvwprintz( w_location, point_zero, c_light_gray, location_prompt );
        mvwprintz( w_location, point( prompt_offset + 1, 0 ), c_light_gray, _( "Starting location:" ) );
        // ::find will return empty location if id was not found. Debug msg will be printed too.
        mvwprintz( w_location, point( prompt_offset + utf8_width( _( "Starting location:" ) ) + 2, 0 ),
                   c_light_gray, you.start_location.obj().name() );
        wrefresh( w_location );

        werase( w_scenario );
        mvwprintz( w_scenario, point_zero, COL_HEADER, _( "Scenario: " ) );
        wprintz( w_scenario, c_light_gray, g->scen->gender_appropriate_name( you.male ) );
        wrefresh( w_scenario );

        werase( w_profession );
        mvwprintz( w_profession, point_zero, COL_HEADER, _( "Profession: " ) );
        wprintz( w_profession, c_light_gray, you.prof->gender_appropriate_name( you.male ) );
        wrefresh( w_profession );

        const std::string action = ctxt.handle_input();

        if( action == "NEXT_TAB" ) {
            if( !points.is_valid() ) {
                if( points.skill_points_left() < 0 ) {
                    popup( _( "Too many points allocated, change some features and try again." ) );
                } else if( points.trait_points_left() < 0 ) {
                    popup( _( "Too many trait points allocated, change some traits or lower some stats and try again." ) );
                } else if( points.stat_points_left() < 0 ) {
                    popup( _( "Too many stat points allocated, lower some stats and try again." ) );
                } else {
                    popup( _( "Too many points allocated, change some features and try again." ) );
                }
                redraw = true;
                continue;
            } else if( points.has_spare() &&
                       !query_yn( _( "Remaining points will be discarded, are you sure you want to proceed?" ) ) ) {
                redraw = true;
                continue;
            } else if( you.name.empty() ) {
                mvwprintz( w_name, point( namebar_pos, 0 ), h_light_gray, _( "_______NO NAME ENTERED!_______" ) );
                wrefresh( w_name );
                if( !query_yn( _( "Are you SURE you're finished?  Your name will be randomly generated." ) ) ) {
                    redraw = true;
                    continue;
                } else {
                    you.pick_name();
                    return tab_direction::FORWARD;
                }
            } else if( query_yn( _( "Are you SURE you're finished?" ) ) ) {
                return tab_direction::FORWARD;
            } else {
                redraw = true;
                continue;
            }
        } else if( action == "PREV_TAB" ) {
            return tab_direction::BACKWARD;
        } else if( action == "REROLL_CHARACTER" && allow_reroll ) {
            points.init_from_options();
            you.randomize( false, points );
            // Return tab_direction::NONE so we re-enter this tab again, but it forces a complete redrawing of it.
            return tab_direction::NONE;
        } else if( action == "REROLL_CHARACTER_WITH_SCENARIO" && allow_reroll ) {
            points.init_from_options();
            you.randomize( true, points );
            // Return tab_direction::NONE so we re-enter this tab again, but it forces a complete redrawing of it.
            return tab_direction::NONE;
        } else if( action == "SAVE_TEMPLATE" ) {
            if( const auto name = query_for_template_name() ) {
                ::save_template( you, *name, points );
            }
            redraw = true;
        } else if( action == "PICK_RANDOM_NAME" ) {
            if( !MAP_SHARING::isSharing() ) { // Don't allow random names when sharing maps. We don't need to check at the top as you won't be able to edit the name
                you.pick_name();
            }
        } else if( action == "CHANGE_GENDER" ) {
            you.male = !you.male;
        } else if( action == "CHOOSE_LOCATION" ) {
            select_location.redraw();
            select_location.query();
            if( select_location.ret >= 0 ) {
                for( const auto &loc : start_location::get_all() ) {
                    if( loc.name() == select_location.entries[ select_location.ret ].txt ) {
                        you.start_location = loc.ident();
                    }
                }
            }
            werase( select_location.window );
            select_location.refresh();
            redraw = true;
        } else if( action == "HELP_KEYBINDINGS" ) {
            // Need to redraw since the help window obscured everything.
            draw_character_tabs( w, _( "DESCRIPTION" ) );
            draw_points( w, points );
            redraw = true;
        } else if( action == "ANY_INPUT" &&
                   !MAP_SHARING::isSharing() ) { // Don't edit names when sharing maps
            const int ch = ctxt.get_raw_input().get_first_input();
            utf8_wrapper wrap( you.name );
            if( ch == KEY_BACKSPACE ) {
                if( !wrap.empty() ) {
                    wrap.erase( wrap.length() - 1, 1 );
                    you.name = wrap.str();
                }
            } else if( ch == KEY_F( 2 ) ) {
                utf8_wrapper tmp( get_input_string_from_file() );
                if( !tmp.empty() && tmp.length() + wrap.length() < 30 ) {
                    you.name.append( tmp.str() );
                }
            } else if( ch == '\n' ) {
                // nope, we ignore this newline, don't want it in char names
            } else {
                wrap.append( ctxt.get_raw_input().text );
                you.name = wrap.str();
            }
        } else if( action == "QUIT" && query_yn( _( "Return to main menu?" ) ) ) {
            return tab_direction::QUIT;
        }
    } while( true );
}

std::vector<trait_id> character_mutations::get_base_traits() const
{
    return std::vector<trait_id>( my_traits.begin(), my_traits.end() );
}

std::vector<trait_id> character_mutations::get_mutations( bool include_hidden ) const
{
    std::vector<trait_id> result;
    for( auto &t : my_mutations ) {
        if( include_hidden || t.first.obj().player_display ) {
            result.push_back( t.first );
        }
    }
    return result;
}

void character_mutations::empty_traits( Character &guy )
{
    for( auto &mut : my_mutations ) {
        guy.on_mutation_loss( mut.first );
    }
    my_traits.clear();
    my_mutations.clear();
    cached_mutations.clear();
}

void Character::empty_skills()
{
    for( auto &sk : *_skills ) {
        sk.second.level( 0 );
    }
}

void Character::add_traits()
{
    points_left points = points_left();
    add_traits( points );
}

void Character::add_traits( points_left &points )
{
    // TODO: get rid of using g->u here, use `this` instead
    for( const trait_id &tr : g->u.prof->get_locked_traits() ) {
        if( !mutations.has_trait( tr ) ) {
            mutations.toggle_trait( *this, tr );
        } else {
            points.trait_points += tr->points;
        }
    }
    for( const trait_id &tr : g->scen->get_locked_traits() ) {
        if( !mutations.has_trait( tr ) ) {
            mutations.toggle_trait( *this, tr );
        }
    }
}

trait_id Character::random_good_trait()
{
    std::vector<trait_id> vTraitsGood;

    for( auto &traits_iter : mutation_branch::get_all() ) {
        if( traits_iter.points >= 0 && g->scen->traitquery( traits_iter.id ) ) {
            vTraitsGood.push_back( traits_iter.id );
        }
    }

    return random_entry( vTraitsGood );
}

trait_id Character::random_bad_trait()
{
    std::vector<trait_id> vTraitsBad;

    for( auto &traits_iter : mutation_branch::get_all() ) {
        if( traits_iter.points < 0 && g->scen->traitquery( traits_iter.id ) ) {
            vTraitsBad.push_back( traits_iter.id );
        }
    }

    return random_entry( vTraitsBad );
}

cata::optional<std::string> query_for_template_name()
{
    static const std::set<int> fname_char_blacklist = {
#if defined(_WIN32)
        '\"', '*', '/', ':', '<', '>', '?', '\\', '|',
        '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07',         '\x09',
        '\x0B', '\x0C',         '\x0E', '\x0F', '\x10', '\x11', '\x12',
        '\x13', '\x14',         '\x16', '\x17', '\x18', '\x19', '\x1A',
        '\x1C', '\x1D', '\x1E', '\x1F'
#else
        '/'
#endif
    };
    std::string title = _( "Name of template:" );
    std::string desc = _( "Keep in mind you may not use special characters like / in filenames" );

    string_input_popup spop;
    spop.title( title );
    spop.description( desc );
    spop.width( FULL_SCREEN_WIDTH - utf8_width( title ) - 8 );
    for( int character : fname_char_blacklist ) {
        spop.callbacks[ character ] = []() {
            return true;
        };
    }

    spop.query_string( true );
    if( spop.canceled() ) {
        return cata::nullopt;
    } else {
        return spop.text();
    }
}

void save_template( const avatar &u, const std::string &name, const points_left &points )
{
    std::string native = utf8_to_native( name );
#if defined(_WIN32)
    if( native.find_first_of( "\"*/:<>?\\|"
                              "\x01\x02\x03\x04\x05\x06\x07\x08\x09" // NOLINT(cata-text-style)
                              "\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12" // NOLINT(cata-text-style)
                              "\x13\x14\x15\x16\x17\x18\x19\x1A\x1B"
                              "\x1C\x1D\x1E\x1F"
                            ) != std::string::npos ) {
        popup( _( "Conversion of your filename to your native character set resulted in some unsafe characters, please try an alphanumeric filename instead" ) );
        return;
    }
#endif

    write_to_file( FILENAMES["templatedir"] + native + ".template", [&]( std::ostream & fout ) {
        JsonOut jsout( fout, true );

        jsout.start_array();

        jsout.start_object();
        jsout.member( "stat_points", points.stat_points );
        jsout.member( "trait_points", points.trait_points );
        jsout.member( "skill_points", points.skill_points );
        jsout.member( "limit", points.limit );
        jsout.end_object();

        u.serialize( jsout );

        jsout.end_array();
    }, _( "player template" ) );
}

bool avatar::load_template( const std::string &template_name, points_left &points )
{
    return read_from_file_json( FILENAMES["templatedir"] + utf8_to_native( template_name ) +
    ".template", [&]( JsonIn & jsin ) {

        if( jsin.test_array() ) {
            // not a legacy template
            jsin.start_array();

            if( jsin.end_array() ) {
                return;
            }

            JsonObject jobj = jsin.get_object();

            points.stat_points = jobj.get_int( "stat_points" );
            points.trait_points = jobj.get_int( "trait_points" );
            points.skill_points = jobj.get_int( "skill_points" );
            points.limit = static_cast<points_left::point_limit>( jobj.get_int( "limit" ) );

            if( jsin.end_array() ) {
                return;
            }
        } else {
            points.stat_points = 0;
            points.trait_points = 0;
            points.skill_points = 0;
        }

        deserialize( jsin );

        if( MAP_SHARING::isSharing() ) {
            // just to make sure we have the right name
            name = MAP_SHARING::getUsername();
        }
    } );
}

void reset_scenario( avatar &u, const scenario *scen )
{
    auto psorter = profession_sorter;
    psorter.sort_by_points = true;
    const auto permitted = scen->permitted_professions();
    const auto default_prof = *std::min_element( permitted.begin(), permitted.end(), psorter );

    u.start_location = scen->start_location();
    u.str_max = 8;
    u.dex_max = 8;
    u.int_max = 8;
    u.per_max = 8;
    g->scen = scen;
    u.prof = &default_prof.obj();
    for( const trait_id &t : u.mutations.get_mutations() ) {
        if( t.obj().hp_modifier != 0 ) {
            u.mutations.toggle_trait( u, t );
        }
    }
    u.mutations.empty_traits( u );
    u.recalc_hp();
    u.empty_skills();
    u.add_traits();
}
