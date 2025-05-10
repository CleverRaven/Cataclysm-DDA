#include "avatar.h" // IWYU pragma: associated

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <initializer_list>
#include <iosfwd>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "achievement.h"
#include "addiction.h"
#include "bionics.h"
#include "calendar_ui.h"
#include "cata_path.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character.h"
#include "character_martial_arts.h"
#include "city.h"
#include "color.h"
#include "cuboid_rectangle.h"
#include "cursesdef.h"
#include "debug.h"
#include "enum_conversions.h"
#include "flexbuffer_json.h"
#include "game_constants.h"
#include "input_context.h"
#include "input_enums.h"
#include "item.h"
#include "json.h"
#include "loading_ui.h"
#include "localized_comparator.h"
#include "magic.h"
#include "magic_enchantment.h"
#include "make_static.h"
#include "mapsharing.h"
#include "martialarts.h"
#include "mission.h"
#include "mod_manager.h"
#include "monster.h"
#include "mutation.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "overmap_ui.h"
#include "path_info.h"
#include "pimpl.h"
#include "player_difficulty.h"
#include "point.h"
#include "profession.h"
#include "profession_group.h"
#include "proficiency.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "ret_val.h"
#include "rng.h"
#include "scenario.h"
#include "skill.h"
#include "skill_ui.h"
#include "start_location.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "text_snippets.h"
#include "translation.h"
#include "translations.h"
#include "type_id.h"
#include "uilist.h"
#include "ui_manager.h"
#include "units_utility.h"
#include "veh_type.h"
#include "worldfactory.h"

static const std::string flag_CHALLENGE( "CHALLENGE" );
static const std::string flag_CITY_START( "CITY_START" );
static const std::string flag_SECRET( "SECRET" );
static const std::string flag_SKIP_DEFAULT_BACKGROUND( "SKIP_DEFAULT_BACKGROUND" );

static const flag_id json_flag_auto_wield( "auto_wield" );
static const flag_id json_flag_no_auto_equip( "no_auto_equip" );

static const json_character_flag json_flag_BIONIC_TOGGLED( "BIONIC_TOGGLED" );

static const matype_id style_none( "style_none" );

static const profession_group_id
profession_group_adult_basic_background( "adult_basic_background" );

static const trait_id trait_SMELLY( "SMELLY" );
static const trait_id trait_WEAKSCENT( "WEAKSCENT" );
static const trait_id trait_XS( "XS" );
static const trait_id trait_XXXL( "XXXL" );

// Whether or not use Outfit (M) at character creation
static bool outfit = true;

// Responsive screen behavior for small terminal sizes
static bool isWide = false;

// Colors used in this file: (Most else defaults to c_light_gray)
#define COL_SELECT          h_white   // Selected value
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
#define COL_TR_NEUT_OFF_ACT c_light_gray  // A toggled-off neutral trait
#define COL_TR_NEUT_ON_ACT  c_yellow   // A toggled-on neutral trait
#define COL_TR_NEUT_OFF_PAS c_dark_gray  // A toggled-off neutral trait
#define COL_TR_NEUT_ON_PAS  c_brown     // A toggled-on neutral trait
#define COL_SKILL_USED      c_green   // A skill with at least one point
#define COL_HEADER          c_white   // Captions, like "Profession items"
#define COL_NOTE_MINOR      c_light_gray  // Just regular note

static int skill_increment_cost( const Character &u, const skill_id &skill );

class tab_manager
{
        std::vector<std::string> &tab_names;
        std::map<size_t, inclusive_rectangle<point>> tab_map;
        point window_pos;
    public:
        bool complete = false;
        bool quit = false;
        tab_list position;

        explicit tab_manager( std::vector<std::string> &tab_names ) : tab_names( tab_names ),
            position( tab_names ) {
        }

        void draw( const catacurses::window &w );
        bool handle_input( const std::string &action, const input_context &ctxt );
        void set_up_tab_navigation( input_context &ctxt );
};

void tab_manager::draw( const catacurses::window &w )
{
    std::pair<std::vector<std::string>, size_t> fitted_tabs = fit_tabs_to_width( getmaxx( w ),
            position.cur_index(), tab_names );
    tab_map = draw_tabs( w, fitted_tabs.first, position.cur_index() - fitted_tabs.second,
                         fitted_tabs.second );
    window_pos = point( getbegx( w ), getbegy( w ) );
    draw_border_below_tabs( w );

    wattron( w, BORDER_COLOR );
    mvwaddch( w, point( 0, 5 ), LINE_XXXO ); // |-
    mvwhline( w, point( 1, 5 ), LINE_OXOX, TERMX - 2 ); // -
    mvwaddch( w, point( TERMX - 1, 5 ), LINE_XOXX ); // -|
    wattroff( w, BORDER_COLOR );
}

bool tab_manager::handle_input( const std::string &action, const input_context &ctxt )
{
    if( action == "QUIT" && query_yn( _( "Return to main menu?" ) ) ) {
        quit = true;
    } else if( action == "PREV_TAB" ) {
        position.prev();
    } else if( action == "NEXT_TAB" ) {
        position.next();
    } else if( action == "SELECT" ) {
        std::optional<point> coord = ctxt.get_coordinates_text( catacurses::stdscr );
        if( coord.has_value() ) {
            point local_coord = coord.value() + window_pos;
            for( const auto &entry : tab_map ) {
                if( entry.second.contains( local_coord ) ) {
                    position.set_index( entry.first );
                    return true;
                }
            }
        }
        return false;
    } else {
        return false;
    }
    return true;
}

void tab_manager::set_up_tab_navigation( input_context &ctxt )
{
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "QUIT" );
}

static int stat_point_pool()
{
    return 4 * 8 + get_option<int>( "INITIAL_STAT_POINTS" );
}
static int stat_points_used( const Character &u )
{
    int used = 0;
    for( int stat : {
             u.str_max, u.dex_max, u.int_max, u.per_max
         } ) {
        used += stat + std::max( 0, stat - HIGH_STAT );
    }
    return used;
}

static int trait_point_pool()
{
    return get_option<int>( "INITIAL_TRAIT_POINTS" );
}
static int trait_points_used( const Character &u )
{
    int used = 0;
    for( trait_id cur_trait : u.get_mutations( true ) ) {
        bool locked = get_scenario()->is_locked_trait( cur_trait )
                      || u.prof->is_locked_trait( cur_trait );
        for( const profession *hobby : u.hobbies ) {
            locked = locked || hobby->is_locked_trait( cur_trait );
        }
        if( locked ) {
            // The starting traits granted by scenarios, professions and hobbies cost nothing
            continue;
        }
        const mutation_branch &mdata = cur_trait.obj();
        used += mdata.points;
    }
    return used;
}

static int skill_point_pool()
{
    return get_option<int>( "INITIAL_SKILL_POINTS" );
}
static int skill_points_used( const Character &u )
{
    int scenario = get_scenario()->point_cost();
    int profession_points = u.prof->point_cost();
    int hobbies = 0;
    for( const profession *hobby : u.hobbies ) {
        hobbies += hobby->point_cost();
    }
    int skills = 0;
    for( const Skill &sk : Skill::skills ) {
        static std::array < int, 1 + MAX_SKILL > costs = { 0, 1, 1, 2, 4, 6, 9, 12, 16, 20, 25 };
        int skill_level = u.get_skill_level( sk.ident() );
        skills += costs.at( std::min<int>( skill_level, costs.size() - 1 ) );
    }
    return scenario + profession_points + hobbies + skills;
}

static int point_pool_total()
{
    return stat_point_pool() + trait_point_pool() + skill_point_pool();
}
static int points_used_total( const Character &u )
{
    return stat_points_used( u ) + trait_points_used( u ) + skill_points_used( u );
}

static int has_unspent_points( const Character &u )
{
    return points_used_total( u ) < point_pool_total();
}

struct multi_pool {
    // The amount of unspent points in the pool without counting the borrowed points
    const int pure_stat_points, pure_trait_points, pure_skill_points;
    // The amount of points awailable in a pool minus the points that are borrowed
    // by lower pools plus the points that can be borrowed from higher pools
    const int stat_points_left, trait_points_left, skill_points_left;
    explicit multi_pool( const Character &u ):
        pure_stat_points( stat_point_pool() - stat_points_used( u ) ),
        pure_trait_points( trait_point_pool() - trait_points_used( u ) ),
        pure_skill_points( skill_point_pool() - skill_points_used( u ) ),
        stat_points_left( pure_stat_points
                          + std::min( 0, pure_trait_points
                                      + std::min( 0, pure_skill_points ) ) ),
        trait_points_left( pure_stat_points + pure_trait_points + std::min( 0, pure_skill_points ) ),
        skill_points_left( pure_stat_points + pure_trait_points + pure_skill_points )
    {}

};

static int skill_points_left( const avatar &u, pool_type pool )
{
    switch( pool ) {
        case pool_type::MULTI_POOL: {
            return multi_pool( u ).skill_points_left;
        }
        case pool_type::ONE_POOL: {
            return point_pool_total() - points_used_total( u );
        }
        case pool_type::TRANSFER:
        case pool_type::FREEFORM:
            return 0;
    }
    return 0;
}

// Toggle this trait and all prereqs, removing upgrades on removal
void Character::toggle_trait_deps( const trait_id &tr, const std::string &variant )
{
    static const int depth_max = 10;
    const mutation_branch &mdata = tr.obj();
    if( mdata.category.empty() || mdata.startingtrait ) {
        toggle_trait( tr, variant );
    } else if( !has_trait( tr ) ) {
        int rc = 0;
        while( !has_trait( tr ) && rc < depth_max ) {
            const mutation_variant *chosen_var = variant.empty() ? nullptr : tr->variant( variant );
            mutate_towards( tr, chosen_var );
            rc++;
        }
    } else if( has_trait( tr ) ) {
        for( const auto &addition : get_addition_traits( tr ) ) {
            unset_mutation( addition );
        }
        for( const auto &lower : get_lower_traits( tr ) ) {
            unset_mutation( lower );
        }
        unset_mutation( tr );
    }
    calc_mutation_levels();
}

static std::string pools_to_string( const avatar &u, pool_type pool )
{
    switch( pool ) {
        case pool_type::MULTI_POOL: {
            multi_pool p( u );
            bool is_valid = p.stat_points_left >= 0 && p.trait_points_left >= 0 && p.skill_points_left >= 0;
            return string_format(
                       _( "Points left: <color_%s>%d</color>%c<color_%s>%d</color>%c<color_%s>%d</color>=<color_%s>%d</color>" ),
                       p.stat_points_left >= 0 ? "light_gray" : "red", p.pure_stat_points,
                       p.pure_trait_points >= 0 ? '+' : '-',
                       p.trait_points_left >= 0 ? "light_gray" : "red", std::abs( p.pure_trait_points ),
                       p.pure_skill_points >= 0 ? '+' : '-',
                       p.skill_points_left >= 0 ? "light_gray" : "red", std::abs( p.pure_skill_points ),
                       is_valid ? "light_gray" : "red", p.skill_points_left );
        }
        case pool_type::ONE_POOL: {
            return string_format( _( "Points left: %4d" ), point_pool_total() - points_used_total( u ) );
        }
        case pool_type::TRANSFER:
            return _( "Character Transfer: No changes can be made." );
        case pool_type::FREEFORM:
            return _( "Survivor" );
    }
    return "If you see this, this is a bug";
}

static void set_points( tab_manager &tabs, avatar &u, pool_type & );
static void set_stats( tab_manager &tabs, avatar &u, pool_type );
static void set_traits( tab_manager &tabs, avatar &u, pool_type );
static void set_scenario( tab_manager &tabs, avatar &u, pool_type );
static void set_profession( tab_manager &tabs, avatar &u, pool_type );
static void set_hobbies( tab_manager &tabs, avatar &u, pool_type );
static void set_skills( tab_manager &tabs, avatar &u, pool_type );
static void set_description( tab_manager &tabs, avatar &you, bool allow_reroll, pool_type );

static std::optional<std::string> query_for_template_name();
static void reset_scenario( avatar &u, const scenario *scen );

void Character::pick_name( bool bUseDefault )
{
    if( bUseDefault && !get_option<std::string>( "DEF_CHAR_NAME" ).empty() && is_avatar() ) {
        name = get_option<std::string>( "DEF_CHAR_NAME" );
    } else {
        name = SNIPPET.expand( male ? "<male_full_name>" : "<female_full_name>" );
    }
}

static std::string wrap60( const std::string &text )
{
    return string_join( foldstring( text, 60 ), "\n" );
}

static matype_id choose_ma_style( const character_type type, const std::vector<matype_id> &styles,
                                  const avatar &u )
{
    if( type == character_type::NOW || type == character_type::FULL_RANDOM ) {
        return random_entry( styles );
    }
    if( styles.size() == 1 ) {
        return styles.front();
    }

    input_context ctxt( "MELEE_STYLE_PICKER", keyboard_mode::keycode );
    ctxt.register_action( "SHOW_DESCRIPTION" );

    uilist menu;
    menu.allow_cancel = false;
    menu.title = _( "Select a style.\n" );
    menu.text = string_format( _( "STR: <color_white>%d</color>, DEX: <color_white>%d</color>, "
                                  "PER: <color_white>%d</color>, INT: <color_white>%d</color>\n"
                                  "Press [<color_yellow>%s</color>] for technique details and compatible weapons.\n" ),
                               u.get_str(), u.get_dex(), u.get_per(), u.get_int(),
                               ctxt.get_desc( "SHOW_DESCRIPTION" ) );
    ma_style_callback callback( 0, styles );
    menu.callback = &callback;
    menu.input_category = "MELEE_STYLE_PICKER";
    menu.additional_actions.emplace_back( "SHOW_DESCRIPTION", translation() );
    menu.desc_enabled = true;

    for( const matype_id &s : styles ) {
        const martialart &style = s.obj();
        menu.addentry_desc( style.name.translated(), wrap60( style.description.translated() ) );
    }
    while( true ) {
        menu.query( true );
        const matype_id &selected = styles[menu.ret];
        if( query_yn( string_format( _( "Use the %s style?" ), selected.obj().name ) ) ) {
            return selected;
        }
    }
}

void Character::randomize( const bool random_scenario, bool play_now )
{
    const int max_trait_points = get_option<int>( "MAX_TRAIT_POINTS" );
    // Reset everything to the defaults to have a clean state.
    if( is_avatar() ) {
        *this->as_avatar() = avatar();
    }

    bool gender_selection = one_in( 2 );
    male = gender_selection;
    outfit = gender_selection;
    if( !MAP_SHARING::isSharing() ) {
        play_now ? pick_name() : pick_name( true );
    } else {
        name = MAP_SHARING::getUsername();
    }
    randomize_height();
    randomize_blood();
    randomize_heartrate();
    bool cities_enabled = world_generator->active_world->WORLD_OPTIONS["CITY_SIZE"].getValue() != "0";
    if( random_scenario ) {
        std::vector<const scenario *> scenarios;
        for( const scenario &scen : scenario::get_all() ) {
            if( !scen.has_flag( flag_CHALLENGE ) && !scen.scen_is_blacklisted() &&
                ( !scen.has_flag( flag_CITY_START ) || cities_enabled ) && scen.can_pick().success() ) {
                scenarios.emplace_back( &scen );
            }
        }
        const scenario *selected_scenario = random_entry( scenarios );
        if( selected_scenario ) {
            set_scenario( selected_scenario );
        } else {
            debugmsg( "Failed randomizing scenario - no entries matching requirements." );
        }
    }

    const scenario *scenario_from = is_avatar() ? get_scenario() : scenario::generic();
    prof = scenario_from->weighted_random_profession();
    play_name_suffix = prof->gender_appropriate_name( male );
    zero_all_skills();

    init_age = rng( this->prof->age_lower, this->prof->age_upper );
    starting_city = std::nullopt;
    world_origin = std::nullopt;
    random_start_location = true;

    str_max = rng( 6, HIGH_STAT - 2 );
    dex_max = rng( 6, HIGH_STAT - 2 );
    int_max = rng( 6, HIGH_STAT - 2 );
    per_max = rng( 6, HIGH_STAT - 2 );

    set_body();
    randomize_hobbies();
    const trait_id background = prof->pick_background();
    if( !background.is_empty() ) {
        set_mutation( background );
    }

    int num_gtraits = 0;
    int num_btraits = 0;
    int tries = 0;
    add_traits(); // adds mandatory profession/scenario traits.
    for( const trait_id &mut : get_mutations() ) {
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
    for( int loops = 0; loops <= 100000; loops++ ) {
        multi_pool p( *this );
        bool is_valid = p.stat_points_left >= 0 && p.trait_points_left >= 0 && p.skill_points_left >= 0;
        if( is_valid && rng( -3, 20 ) <= p.skill_points_left ) {
            break;
        }
        trait_id rn;
        if( num_btraits < max_trait_points && one_in( 3 ) ) {
            tries = 0;
            do {
                rn = random_bad_trait();
                tries++;
            } while( ( has_trait( rn ) || num_btraits - rn->points > max_trait_points ) &&
                     tries < 5 );

            if( tries < 5 && !has_conflicting_trait( rn ) ) {
                toggle_trait_deps( rn );
                num_btraits -= rn->points;
            }
        } else {
            switch( rng( 1, 4 ) ) {
                case 1:
                    if( str_max > 5 ) {
                        str_max--;
                    }
                    break;
                case 2:
                    if( dex_max > 5 ) {
                        dex_max--;
                    }
                    break;
                case 3:
                    if( int_max > 5 ) {
                        int_max--;
                    }
                    break;
                case 4:
                    if( per_max > 5 ) {
                        per_max--;
                    }
                    break;
            }
        }
    }

    for( int loops = 0;
         has_unspent_points( *this ) && loops <= 100000;
         loops++ ) {
        multi_pool p( *this );
        const bool allow_stats = p.stat_points_left > 0;
        const bool allow_traits = p.trait_points_left > 0 && num_gtraits < max_trait_points;
        int r = rng( 1, 9 );
        trait_id rn;
        switch( r ) {
            case 1:
            case 2:
            case 3:
            case 4:
                if( allow_traits ) {
                    rn = random_good_trait();
                    const mutation_branch &mdata = rn.obj();
                    if( !has_trait( rn ) && p.trait_points_left >= mdata.points &&
                        num_gtraits + mdata.points <= max_trait_points && !has_conflicting_trait( rn ) ) {
                        toggle_trait_deps( rn );
                        num_gtraits += mdata.points;
                    }
                    break;
                }
                [[fallthrough]];
            case 5:
                if( allow_stats ) {
                    switch( rng( 1, 4 ) ) {
                        case 1:
                            str_max++;
                            break;
                        case 2:
                            dex_max++;
                            break;
                        case 3:
                            int_max++;
                            break;
                        case 4:
                            per_max++;
                            break;
                    }
                    break;
                }
                [[fallthrough]];
            case 6:
            case 7:
            case 8:
            case 9:
                const skill_id aSkill = Skill::random_skill();
                const int level = get_skill_level( aSkill );

                if( level < p.skill_points_left && level < MAX_SKILL && loops > 10000 ) {
                    // For balance reasons, increasing a skill from level 0 gives you 1 extra level for free
                    set_skill_level( aSkill, ( level == 0 ? 2 : level + 1 ) );
                }
                break;
        }
    }

    randomize_cosmetics();

    // Restart cardio accumulator
    reset_cardio_acc();

    if( is_npc() ) {
        as_npc()->randomize_personality();
        as_npc()->generate_personality_traits();
        initialize();
        add_profession_items();
        as_npc()->catchup_skills();
    }
}

void Character::add_profession_items()
{
    // Our profession should not be a hobby
    if( prof->is_hobby() ) {
        return;
    }

    std::list<item> prof_items = prof->items( outfit, get_mutations() );
    std::list<item> try_adding_again;

    auto attempt_add_items = [this]( std::list<item> &prof_items, std::list<item> &failed_to_add ) {
        for( item &it : prof_items ) {
            if( it.has_flag( STATIC( flag_id( "WET" ) ) ) ) {
                it.active = true;
                it.item_counter = 450; // Give it some time to dry off
            }

            item_location success;
            item *wield_or_wear = nullptr;
            // TODO: debugmsg if food that isn't a seed is inedible
            if( it.has_flag( json_flag_no_auto_equip ) ) {
                it.unset_flag( json_flag_no_auto_equip );
                success = try_add( it, nullptr, nullptr, false );
            } else if( it.has_flag( json_flag_auto_wield ) ) {
                it.unset_flag( json_flag_auto_wield );
                if( !has_wield_conflicts( it ) ) {
                    wield( it );
                    wield_or_wear = &it;
                    success = item_location( *this, wield_or_wear );
                } else {
                    success = try_add( it, nullptr, nullptr, false );
                }
            } else if( it.is_armor() ) {
                if( can_wear( it ).success() ) {
                    wear_item( it, false, false );
                    wield_or_wear = &it;
                    success = item_location( *this, wield_or_wear );
                } else {
                    success = try_add( it, nullptr, nullptr, false );
                }
            } else {
                success = try_add( it, nullptr, nullptr, false );
            }

            if( it.is_book() && this->is_avatar() ) {
                as_avatar()->identify( it );
            }

            if( !success ) {
                failed_to_add.emplace_back( it );
            }
        }
    };

    //storage items may not be added first, so a second attempt is needed
    attempt_add_items( prof_items, try_adding_again );
    if( !try_adding_again.empty() ) {
        prof_items.clear();
        attempt_add_items( try_adding_again, prof_items );
        //if there's one item left that still can't be added, attempt to wield it
        if( prof_items.size() == 1 ) {
            item last_item = prof_items.front();
            if( !has_wield_conflicts( last_item ) ) {
                bool success_wield = wield( last_item );
                if( success_wield ) {
                    prof_items.pop_front();
                }
            }
        }
    }

    recalc_sight_limits();
    calc_encumbrance();
}

void Character::randomize_hobbies()
{
    hobbies.clear();
    std::vector<profession_id> choices = get_scenario()->permitted_hobbies();
    choices.erase( std::remove_if( choices.begin(), choices.end(),
    [this]( const string_id<profession> &hobby ) {
        return !prof->allows_hobby( hobby );
    } ), choices.end() );
    if( choices.empty() ) {
        debugmsg( "Why would you blacklist all hobbies?" );
        choices = profession::get_all_hobbies();
    };

    int random = rng( 0, 5 );

    if( random >= 1 ) {
        add_random_hobby( choices );
    }
    if( random >= 3 ) {
        add_random_hobby( choices );
    }
    if( random >= 5 ) {
        add_random_hobby( choices );
    }
}

void Character::add_random_hobby( std::vector<profession_id> &choices )
{
    const profession_id hobby = random_entry_removed( choices );
    hobbies.insert( &*hobby );

    // Add or remove traits from hobby
    for( const trait_and_var &cur : hobby->get_locked_traits() ) {
        toggle_trait( cur.trait );
    }
}

static int calculate_cumulative_experience( int level )
{
    int sum = 0;

    while( level > 0 ) {
        sum += 10000 * level * level;
        level--;
    }

    return sum;
}

bool avatar::create( character_type type, const std::string &tempname )
{
    loading_ui::done();
    set_wielded_item( item() );

    prof = profession::generic();
    set_scenario( scenario::generic() );

    const bool interactive = type != character_type::NOW &&
                             type != character_type::FULL_RANDOM;

    std::vector<std::string> character_tabs = {
        _( "POINTS" ),
        _( "SCENARIO" ),
        _( "PROFESSION" ),
        //~ Not scenery/backdrop, but previous life up to this point
        _( "BACKGROUND" ),
        _( "STATS" ),
        _( "TRAITS" ),
        _( "SKILLS" ),
        _( "DESCRIPTION" ),
    };
    tab_manager tabs( character_tabs );

    const std::string point_pool = get_option<std::string>( "CHARACTER_POINT_POOLS" );
    pool_type pool = pool_type::FREEFORM;
    if( point_pool == "multi_pool" ) {
        // if using legacy multipool only set it to that
        pool = pool_type::MULTI_POOL;
    }

    switch( type ) {
        case character_type::CUSTOM:
            randomize_cosmetics();
            break;
        case character_type::RANDOM:
            //random scenario, default name if exist
            randomize( true );
            tabs.position.last();
            break;
        case character_type::NOW:
            //default world, fixed scenario, random name
            randomize( false, true );
            break;
        case character_type::FULL_RANDOM:
            //default world, random scenario, random name
            randomize( true, true );
            break;
        case character_type::TEMPLATE:
            if( !load_template( tempname, /*out*/ pool ) ) {
                return false;
            }

            // We want to prevent recipes known by the template from being applied to the
            // new character. The recipe list will be rebuilt when entering the game.
            // Except if it is a character transfer template
            if( pool != pool_type::TRANSFER ) {
                forget_all_recipes();
            }
            tabs.position.last();
            break;
    }

    // Don't apply the default backgrounds on a template
    if( type != character_type::TEMPLATE &&
        !get_scenario()->has_flag( flag_SKIP_DEFAULT_BACKGROUND ) ) {
        add_default_background();
    }

    auto nameExists = [&]( const std::string & name ) {
        return world_generator->active_world->save_exists( save_t::from_save_id( name ) ) &&
               !query_yn( _( "A save with the name '%s' already exists in this world.\n"
                             "Saving will overwrite the already existing character.\n\n"
                             "Continue anyways?" ), name );
    };
    set_body();
    const bool allow_reroll = type == character_type::RANDOM;
    do {
        if( !interactive ) {
            // no window is created because "Play now" does not require any configuration
            if( nameExists( name ) ) {
                return false;
            }

            break;
        }

        if( pool == pool_type::TRANSFER ) {
            tabs.position.last();
        }

        switch( tabs.position.cur_index() ) {
            case 0:
                set_points( tabs, *this, /*out*/ pool );
                break;
            case 1:
                set_scenario( tabs, *this, pool );
                break;
            case 2:
                set_profession( tabs, *this, pool );
                break;
            case 3:
                set_hobbies( tabs, *this, pool );
                break;
            case 4:
                set_stats( tabs, *this, pool );
                break;
            case 5:
                set_traits( tabs, *this, pool );
                break;
            case 6:
                set_skills( tabs, *this, pool );
                break;
            case 7:
                set_description( tabs, *this, allow_reroll, pool );
                break;
        }

        if( tabs.quit ) {
            return false;
        }
    } while( !tabs.complete );

    if( pool == pool_type::TRANSFER ) {
        return true;
    }

    save_template( _( "Last Character" ), pool );

    initialize( type );

    return true;
}

void Character::set_skills_from_hobbies( bool no_override )
{
    // 2 for an average person
    float catchup_modifier = 1.0f + ( 2.0f * get_int() + get_per() ) / 24.0f;
    // 1.2 for an average person, always a bit higher than base amount
    float knowledge_modifier = 1.0f + get_int() / 40.0f;
    // Grab skills from hobbies and train
    for( const profession *profession : hobbies ) {
        for( const profession::StartingSkill &e : profession->skills() ) {
            if( no_override && get_skill_level( e.first ) != 0 ) {
                continue;
            }
            // Train our skill
            const int skill_xp_bonus = calculate_cumulative_experience( e.second );
            get_skill_level_object( e.first ).train( skill_xp_bonus, catchup_modifier,
                    knowledge_modifier, true );
        }
    }
}

void Character::set_recipes_from_hobbies()
{
    for( const profession *profession : hobbies ) {
        for( const recipe_id &recipeID : profession->recipes() ) {
            const recipe &r = recipe_dictionary::get_craft( recipeID->result() );
            learn_recipe( &r );
        }
    }
}

void Character::set_proficiencies_from_hobbies()
{
    for( const profession *profession : hobbies ) {
        for( const proficiency_id &proficiency : profession->proficiencies() ) {
            // Do not duplicate proficiencies
            if( !_proficiencies->has_learned( proficiency ) ) {
                add_proficiency( proficiency );
            }
        }
    }
}

void Character::set_bionics_from_hobbies()
{
    for( const profession *profession : hobbies ) {
        for( const bionic_id &bio : profession->CBMs() ) {
            if( has_bionic( bio ) && !bio->dupes_allowed ) {
                return;
            } else {
                add_bionic( bio );
            }
        }
    }
}

void Character::initialize( bool learn_recipes )
{
    recalc_hp();

    if( has_trait( trait_SMELLY ) ) {
        scent = 800;
    }
    if( has_trait( trait_WEAKSCENT ) ) {
        scent = 300;
    }

    set_wielded_item( item() );

    // Grab skills from profession and increment level
    // We want to do this before the recipes
    for( const profession::StartingSkill &e : prof->skills() ) {
        mod_skill_level( e.first, e.second );
    }

    set_skills_from_hobbies();

    // setup staring bank money
    cash = prof->starting_cash().value_or( rng( -200000, 200000 ) );

    randomize_heartrate();

    //set stored kcal to a normal amount for your height
    set_stored_kcal( get_healthy_kcal() );
    if( has_trait( trait_XS ) ) {
        set_stored_kcal( std::floor( get_stored_kcal() / 5 ) );
    }
    if( has_trait( trait_XXXL ) ) {
        set_stored_kcal( std::floor( get_stored_kcal() * 5 ) );
    }

    if( learn_recipes ) {
        for( const auto &e : recipe_dict ) {
            const recipe &r = e.second;
            if( !r.is_practice() && !r.has_flag( flag_SECRET ) && !knows_recipe( &r ) &&
                has_recipe_requirements( r ) ) {
                learn_recipe( &r );
            }
        }
    }

    std::vector<addiction> prof_addictions = prof->addictions();
    for( const addiction &iter : prof_addictions ) {
        addictions.push_back( iter );
    }

    for( const profession *profession : hobbies ) {
        std::vector<addiction> hobby_addictions = profession->addictions();
        for( const addiction &iter : hobby_addictions ) {
            addictions.push_back( iter );
        }
    }

    for( const bionic_id &bio : prof->CBMs() ) {
        add_bionic( bio );
    }

    set_bionics_from_hobbies();
    // Adjust current energy level to maximum
    set_power_level( get_max_power_level() );

    // Add profession proficiencies
    for( const proficiency_id &pri : prof->proficiencies() ) {
        add_proficiency( pri );
    }

    // Add profession recipes
    for( const recipe_id &id : prof->recipes() ) {
        const recipe &r = recipe_dictionary::get_craft( id->result() );
        learn_recipe( &r );
    }

    // Add hobby proficiencies
    set_proficiencies_from_hobbies();

    // Add hobby recipes
    set_recipes_from_hobbies();

    // Activate some mutations right from the start.
    for( const trait_id &mut : get_mutations() ) {
        const mutation_branch &branch = mut.obj();
        if( branch.starts_active ) {
            cached_mutations[mut].powered = true;
        }
    }
    trait_flag_cache.clear();

    // Ensure that persistent morale effects (e.g. Optimist) are present at the start.
    apply_persistent_morale();

    // Restart cardio accumulator
    reset_cardio_acc();

    recalc_speed_bonus();
}

void avatar::initialize( character_type type )
{
    this->as_character()->initialize();

    for( const matype_id &ma : prof->ma_known() ) {
        if( !martial_arts_data->has_martialart( ma ) ) {
            martial_arts_data->add_martialart( ma );
        }
    }

    if( !prof->ma_choices().empty() ) {
        for( int i = 0; i < prof->ma_choice_amount; i++ ) {
            std::vector<matype_id> styles;
            for( const matype_id &ma : prof->ma_choices() ) {
                if( !martial_arts_data->has_martialart( ma ) ) {
                    styles.push_back( ma );
                }
            }
            if( !styles.empty() ) {
                const matype_id ma_type = choose_ma_style( type, styles, *this );
                martial_arts_data->add_martialart( ma_type );
            } else {
                break;
            }
        }
    }

    for( const trait_id &t : get_base_traits() ) {
        std::vector<matype_id> styles;
        for( const matype_id &s : t->initial_ma_styles ) {
            if( !martial_arts_data->has_martialart( s ) ) {
                styles.push_back( s );
            }
        }
        if( !styles.empty() ) {
            const matype_id ma_type = choose_ma_style( type, styles, *this );
            martial_arts_data->add_martialart( ma_type );
        }
    }

    // Select a random known style
    std::vector<matype_id> selectable_styles = martial_arts_data->get_known_styles( false );
    if( !selectable_styles.empty() ) {
        std::vector<matype_id>::iterator it_max_priority = std::max_element( selectable_styles.begin(),
        selectable_styles.end(), []( const matype_id & a, const matype_id & b ) {
            return a->priority < b->priority;
        } );
        int max_priority = ( *it_max_priority )->priority;
        selectable_styles.erase( std::remove_if( selectable_styles.begin(),
        selectable_styles.end(), [max_priority]( const matype_id & style ) {
            return style->priority != max_priority;
        } ), selectable_styles.end() );
    }
    martial_arts_data->set_style( random_entry( selectable_styles, style_none ) );

    for( const mtype_id &elem : prof->pets() ) {
        starting_pets.push_back( elem );
    }

    if( get_scenario()->vehicle() != vproto_id::NULL_ID() ) {
        starting_vehicle = get_scenario()->vehicle();
    } else {
        starting_vehicle = prof->vehicle();
    }

    prof->learn_spells( *this );

    // Also learn spells from hobbies
    for( const profession *profession : hobbies ) {
        profession->learn_spells( *this );
    }

}

static void draw_points( const catacurses::window &w, pool_type pool, const avatar &u,
                         int netPointCost = 0 )
{
    // Clear line (except borders)
    mvwprintz( w, point( 2, 3 ), c_black, std::string( getmaxx( w ) - 3, ' ' ) );
    mvwprintz( w, point( 2, 4 ), c_black, std::string( getmaxx( w ) - 3, ' ' ) );
    std::string points_msg = pools_to_string( u, pool );
    int pMsg_length = utf8_width( remove_color_tags( points_msg ), true );
    nc_color color = c_light_gray;
    print_colored_text( w, point( 2, 3 ), color, c_light_gray, points_msg );
    if( pool != pool_type::FREEFORM ) {
        if( netPointCost > 0 ) {
            mvwprintz( w, point( pMsg_length + 2, 3 ), c_red, " (-%d)", std::abs( netPointCost ) );
        } else if( netPointCost < 0 ) {
            mvwprintz( w, point( pMsg_length + 2, 3 ), c_green, " (+%d)", std::abs( netPointCost ) );
        }
    }
    print_colored_text( w, point( 2, 4 ), color, c_light_gray,
                        player_difficulty::getInstance().difficulty_to_string( u ) );
}

template <class Compare>
static void draw_filter_and_sorting_indicators( const catacurses::window &w,
        const input_context &ctxt, std::string_view filterstring, const Compare &sorter )
{
    const char *const sort_order = sorter.sort_by_points ? _( "default" ) : _( "name" );
    const std::string sorting_indicator = string_format( "[%1$s] %2$s: %3$s",
                                          colorize( ctxt.get_desc( "SORT" ), c_green ), _( "sort" ),
                                          sort_order );
    const std::string filter_indicator =
        filterstring.empty()
        ? string_format( _( "[%s] filter" ), colorize( ctxt.get_desc( "FILTER" ), c_green ) )
        : std::string( filterstring );
    nc_color current_color = BORDER_COLOR;
    print_colored_text( w, point( 2, getmaxy( w ) - 1 ), current_color, BORDER_COLOR,
                        string_format( "<%1s>-<%2s>", sorting_indicator, filter_indicator ) );
}

static const char *g_switch_msg( const avatar &u )
{
    return  u.male ?
            //~ Gender switch message. 1s - change key name, 2s - profession name.
            _( "Identity: <color_magenta>%2$s</color> (<color_light_cyan>male</color>) (press <color_light_green>%1$s</color> to switch)" )
            :
            //~ Gender switch message. 1s - change key name, 2s - profession name.
            _( "Identity: <color_magenta>%2$s</color> (<color_pink>female</color>) (press <color_light_green>%1$s</color> to switch)" );
}

static const char *dress_switch_msg()
{
    return  outfit ?
            //~ Outfit switch message. 1s - change key name.
            _( "Outfit: <color_light_cyan>male</color> (press <color_light_green>%1$s</color> to change)" ) :
            //~ Outfit switch message. 1s - change key name.
            _( "Outfit: <color_pink>female</color> (press <color_light_green>%1$s</color> to change)" );
}

void set_points( tab_manager &tabs, avatar &u, pool_type &pool )
{
    const int iSecondColumn = 31;
    const int iHeaderHeight = 6;
    // guessing most likely, but it doesn't matter, it will be recalculated if wrong
    int iHelpHeight = 3;
    const bool screen_reader_mode = get_option<bool>( "SCREEN_READER_MODE" );

    ui_adaptor ui;
    catacurses::window w;
    catacurses::window w_description;
    const auto init_windows = [&]( ui_adaptor & ui ) {
        const int freeWidth = TERMX - FULL_SCREEN_WIDTH;
        isWide = freeWidth > 15;
        w = catacurses::newwin( TERMY, TERMX, point::zero );
        w_description = catacurses::newwin( TERMY - iHeaderHeight - iHelpHeight - 1,
                                            TERMX - iSecondColumn - 1, point( iSecondColumn, iHeaderHeight ) );
        ui.position_from_window( w );
    };
    init_windows( ui );
    ui.on_screen_resize( init_windows );

    input_context ctxt( "NEW_CHAR_POINTS" );
    tabs.set_up_tab_navigation( ctxt );
    ctxt.register_navigate_ui_list();
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "CONFIRM" );

    const std::string point_pool = get_option<std::string>( "CHARACTER_POINT_POOLS" );

    using point_limit_tuple = std::tuple<pool_type, std::string, std::string>;
    std::vector<point_limit_tuple> opts;

    const point_limit_tuple multi_pool = std::make_tuple( pool_type::MULTI_POOL,
                                         _( "Legacy: Multiple pools" ),
                                         _( "Stats, traits and skills have separate point pools.\n"
                                            "Putting stat points into traits and skills is allowed and putting trait points into skills is allowed.\n"
                                            "Scenarios and professions affect skill points.\n\n"
                                            "This is a legacy mode.  Point totals are no longer balanced." ) );

    const point_limit_tuple one_pool = std::make_tuple( pool_type::ONE_POOL, _( "Legacy: Single pool" ),
                                       _( "Stats, traits and skills share a single point pool.\n\n"
                                          "This is a legacy mode.  Point totals are no longer balanced." ) );

    const point_limit_tuple freeform = std::make_tuple( pool_type::FREEFORM, _( "Survivor" ),
                                       _( "No point limits are enforced, create a character with the intention of telling a story or challenging yourself." ) );

    if( point_pool == "multi_pool" ) {
        opts = { { multi_pool } };
    } else if( point_pool == "story_teller" ) {
        opts = { { freeform } };
    } else {
        opts = { { freeform, multi_pool, one_pool } };
    }

    int highlighted = 0;

    ui.on_redraw( [&]( ui_adaptor & ui ) {
        const std::string help_text =
            ( isWide ? string_format(
                  _( "Press <color_light_green>%s</color> to view and alter keybindings.\n"
                     "Press <color_light_green>%s</color> or <color_light_green>%s</color> to select pool and "
                     "<color_light_green>%s</color> to confirm selection.\n"
                     "Press <color_light_green>%s</color> to go to the next tab or "
                     "<color_light_green>%s</color> to return to main menu." ),
                  ctxt.get_desc( "HELP_KEYBINDINGS" ), ctxt.get_desc( "UP" ), ctxt.get_desc( "DOWN" ),
                  ctxt.get_desc( "CONFIRM" ), ctxt.get_desc( "NEXT_TAB" ), ctxt.get_desc( "QUIT" ) )
              : string_format(
                  _( "Press <color_light_green>%s</color> to view and alter keybindings." ),
                  ctxt.get_desc( "HELP_KEYBINDINGS" ) )
            );
        const int new_iHelpHeight = foldstring( help_text, getmaxx( w ) - 4 ).size();
        if( new_iHelpHeight != iHelpHeight ) {
            iHelpHeight = new_iHelpHeight;
            init_windows( ui );
        }
        werase( w );
        tabs.draw( w );

        std::string title = std::get<1>( opts[highlighted] );
        std::string description = std::get<2>( opts[highlighted] );

        if( screen_reader_mode ) {
            // Include option title in option description, and say whether it's active
            if( std::get<0>( opts[highlighted] ) == pool ) {
                title.append( _( " - active" ) );
            }
            description = title +  "\n" + description;
        }

        draw_points( w, pool, u );

        // Clear the bottom of the screen.
        werase( w_description );

        const int opts_length = static_cast<int>( opts.size() );
        for( int i = 0; i < opts_length; i++ ) {
            nc_color color;
            if( pool == std::get<0>( opts[i] ) ) {
                color = highlighted == i ? hilite( c_light_green ) : c_green;
            } else {
                color = highlighted == i ? COL_SELECT : c_light_gray;
            }
            const point opt_pos( 2, 6 + i );
            if( highlighted == i ) {
                ui.set_cursor( w, opt_pos );
            }
            if( screen_reader_mode ) {
                // The list of options only clutters up the screen in screen reader mode
            } else {
                mvwprintz( w, opt_pos, color, std::get<1>( opts[i] ) );
            }
        }

        fold_and_print( w_description, point::zero, getmaxx( w_description ),
                        COL_SKILL_USED, description );

        // Helptext points tab
        fold_and_print( w, point( 2, TERMY - foldstring( help_text, getmaxx( w ) - 4 ).size() - 1 ),
                        getmaxx( w ) - 4, COL_NOTE_MINOR, help_text );
        wnoutrefresh( w );
        wnoutrefresh( w_description );
    } );

    const int opts_length = static_cast<int>( opts.size() );
    do {
        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        if( tabs.handle_input( action, ctxt ) ) {
            break; // Tab has changed or user has quit the screen
        } else if( navigate_ui_list( action, highlighted, 1, opts_length, true ) ) {
        } else if( action == "CONFIRM" ) {
            const auto &cur_opt = opts[highlighted];
            pool = std::get<0>( cur_opt );
        }
    } while( true );
}

static std::string assemble_stat_details( avatar &u, int sel )
{
    std::string description_str;
    switch( sel ) {
        case 0: {
            u.recalc_hp();
            u.set_stored_kcal( u.get_healthy_kcal() );
            description_str =
                string_format( _( "Base HP: %d" ), u.get_part_hp_max( bodypart_id( "head" ) ) )
                + string_format( _( "\nCarry weight: %.1f %s" ), convert_weight( u.weight_capacity() ),
                                 weight_units() )
                + string_format( _( "\nResistance to knock down effect when hit: %.1f" ), u.stability_roll() )
                + string_format( _( "\nIntimidation skill: %i" ), u.intimidation() )
                + string_format( _( "\nMaximum oxygen: %i" ), u.get_oxygen_max() )
                + string_format( _( "\nShout volume: %i" ), u.get_shout_volume() )
                + string_format( _( "\nLifting strength: %i" ), u.get_lift_str() )
                + string_format( _( "\nMove cost while swimming: %i" ), u.swim_speed() )
                + colorize(
                    string_format( _( "\nBash damage bonus: %.1f" ), u.bonus_damage( false ) ),
                    COL_STAT_BONUS )
                + _( "\n\nAffects:" )
                + colorize(
                    _( "\n- Throwing range, accuracy, and damage"
                       "\n- Reload speed for weapons using muscle power to reload"
                       "\n- Pull strength of some mutations"
                       "\n- Resistance for being pulled or grabbed by some monsters"
                       "\n- Speed of corpses pulping"
                       "\n- Speed and effectiveness of prying things open, chopping wood, and mining"
                       "\n- Chance of escaping grabs and traps"
                       "\n- Power produced by muscle-powered vehicles"
                       "\n- Most aspects of melee combat"
                       "\n- Effectiveness of smashing furniture or terrain"
                       "\n- Resistance to many diseases and poisons"
                       "\n- Ability to drag heavy objects and grants bonus to speed when dragging them"
                       "\n- Ability to wield heavy weapons with one hand"
                       "\n- Ability to manage gun recoil"
                       "\n- Duration of action of various drugs and alcohol" ),
                    c_green );
        }
        break;

        case 1: {
            description_str =
                colorize(
                    string_format( _( "Melee to-hit bonus: %+.2f" ), u.get_melee_hit_base() ),
                    u.get_melee_hit_base() >= 0 ? COL_STAT_BONUS : COL_STAT_PENALTY );
            description_str += colorize(
                                   string_format( _( "\nThrowing penalty per target's dodge: +%d" ),
                                                  u.throw_dispersion_per_dodge( false ) ), COL_STAT_PENALTY );
            if( u.ranged_dex_mod() != 0 ) {
                description_str += colorize( string_format( _( "\nRanged penalty: -%d" ),
                                             std::abs( u.ranged_dex_mod() ) ), COL_STAT_PENALTY );
            } else {
                description_str += "\n";
            }
            description_str +=
                string_format( _( "\nDodge skill: %.f" ), u.get_dodge() )
                + string_format( _( "\nMove cost while swimming: %i" ), u.swim_speed() )
                + _( "\n\nAffects:" )
                + colorize(
                    _( "\n- Effectiveness of lockpicking"
                       "\n- Resistance for being grabbed by some monsters"
                       "\n- Chance of escaping grabs and traps"
                       "\n- Effectiveness of disarming traps"
                       "\n- Chance of success when manipulating with gun modifications"
                       "\n- Effectiveness of repairing and modifying clothes and armor"
                       "\n- Attack speed and chance of critical hits in melee combat"
                       "\n- Effectiveness of stealing"
                       "\n- Throwing speed"
                       "\n- Aiming speed"
                       "\n- Speed and effectiveness of chopping wood with powered tools"
                       "\n- Chance to avoid traps"
                       "\n- Chance to get better results when butchering corpses or cutting items"
                       "\n- Chance of avoiding cuts on sharp terrain"
                       "\n- Chance of losing control of vehicle when driving"
                       "\n- Chance of damaging melee weapon on attack"
                       "\n- Damage from falling" ),
                    c_green );
        }
        break;

        case 2: {
            const int read_spd = u.read_speed();
            description_str =
                colorize( string_format( _( "Read times: %d%%" ), read_spd ),
                          ( read_spd == 100 ? COL_STAT_NEUTRAL :
                            ( read_spd < 100 ? COL_STAT_BONUS : COL_STAT_PENALTY ) ) )
                + string_format( _( "\nPersuade/lie skill: %i" ), u.persuade_skill() )
                + colorize( string_format( _( "\nCrafting bonus: %2d%%" ), u.get_int() ),
                            COL_STAT_BONUS )
                + _( "\n\nAffects:" )
                + colorize(
                    _( "\n- Speed of 'catching up' practical experience to theoretical knowledge"
                       "\n- Detection and disarming traps"
                       "\n- Chance of success when installing bionics"
                       "\n- Chance of success when manipulating with gun modifications"
                       "\n- Chance to learn a recipe when crafting from a book"
                       "\n- Chance to learn martial arts techniques when using CQB bionic"
                       "\n- Chance of hacking computers and card readers"
                       "\n- Chance of successful robot reprogramming"
                       "\n- Chance of successful decrypting memory cards"
                       "\n- Chance of bypassing vehicle security system"
                       "\n- Chance to get better results when disassembling items"
                       "\n- Chance of being paralyzed by fear attack" ),
                    c_green );
        }
        break;

        case 3: {
            if( u.ranged_per_mod() > 0 ) {
                description_str =
                    colorize( string_format( _( "Aiming penalty: -%d" ), u.ranged_per_mod() ),
                              COL_STAT_PENALTY );
            }
            description_str +=
                string_format( _( "\nPersuade/lie skill: %i" ), u.persuade_skill() )
                + _( "\n\nAffects:" )
                + colorize(
                    _( "\n- Speed of 'catching up' practical experience to theoretical knowledge"
                       "\n- Time needed for safe cracking"
                       "\n- Sight distance on game map and overmap"
                       "\n- Effectiveness of stealing"
                       "\n- Throwing accuracy"
                       "\n- Chance of losing control of vehicle when driving"
                       "\n- Chance of spotting camouflaged creatures"
                       "\n- Effectiveness of lockpicking"
                       "\n- Effectiveness of foraging"
                       "\n- Precision when examining wounds and using first aid skill"
                       "\n- Detection and disarming traps"
                       "\n- Morale bonus when playing a musical instrument"
                       "\n- Effectiveness of repairing and modifying clothes and armor"
                       "\n- Chance of critical hits in melee combat" ),
                    c_green );
        }
        break;
    }
    return description_str;
}

static std::string assemble_stat_help( const input_context &ctxt )
{
    return string_format(
               _( "Press <color_light_green>%s</color> to view and alter keybindings.\n"
                  "Press <color_light_green>%s</color> / <color_light_green>%s</color> to select stat.\n"
                  "Press <color_light_green>%s</color> to increase stat or "
                  "<color_light_green>%s</color> to decrease stat.\n"
                  "Press <color_light_green>%s</color> to go to the next tab or "
                  "<color_light_green>%s</color> to return to the previous tab." ),
               ctxt.get_desc( "HELP_KEYBINDINGS" ), ctxt.get_desc( "UP" ), ctxt.get_desc( "DOWN" ),
               ctxt.get_desc( "RIGHT" ), ctxt.get_desc( "LEFT" ),
               ctxt.get_desc( "NEXT_TAB" ), ctxt.get_desc( "PREV_TAB" ) );
}

/** Handle the stats tab of the character generation menu */
void set_stats( tab_manager &tabs, avatar &u, pool_type pool )
{
    // TODO: Move this out to a common header and eliminate other separate instances of these strings.
    static const std::array<translation, 4> stat_labels = { to_translation( "Strength" ), to_translation( "Dexterity" ), to_translation( "Intelligence" ), to_translation( "Perception" ) };
    const int max_stat_points = pool == pool_type::FREEFORM ? 20 : MAX_STAT;
    const int min_stat_points = 4;

    int sel = 0;

    const bool screen_reader_mode = get_option<bool>( "SCREEN_READER_MODE" );
    std::string warning_text; // Used to move warnings from the header to the details pane
    std::string last_stat; // Used to ensure text is read out when increasing/decreasing stats

    int iSecondColumn;
    const int iHeaderHeight = 6;
    // guessing most likely, but it doesn't matter, it will be recalculated if wrong
    int iHelpHeight = 4;

    ui_adaptor ui;
    catacurses::window w;
    catacurses::window w_details_pane;
    scrolling_text_view details( w_details_pane );
    bool details_recalc = true;
    const auto init_windows = [&]( ui_adaptor & ui ) {
        const int thirds = std::min( ( TERMX - 4 ) / 3, 38 );  // to allign scrollbar with the traits tab
        iSecondColumn = std::max( thirds, utf8_width( pools_to_string( u, pool ), true ) + 2 );
        const size_t iContentHeight = TERMY - iHeaderHeight - iHelpHeight - 1;
        w = catacurses::newwin( TERMY, TERMX, point::zero );
        w_details_pane = catacurses::newwin( iContentHeight, TERMX - iSecondColumn - 1,
                                             point( iSecondColumn, iHeaderHeight ) );
        details_recalc = true;
        ui.position_from_window( w );
    };
    init_windows( ui );
    ui.on_screen_resize( init_windows );

    input_context ctxt( "NEW_CHAR_STATS" );
    tabs.set_up_tab_navigation( ctxt );
    details.set_up_navigation( ctxt, scrolling_key_scheme::angle_bracket_scroll );
    ctxt.register_cardinal();
    ctxt.register_navigate_ui_list();
    ctxt.register_action( "HELP_KEYBINDINGS" );

    u.reset();
    std::array<int *, 4> stats = { &u.str_max, &u.dex_max, &u.int_max, &u.per_max };

    ui.on_redraw( [&]( ui_adaptor & ui ) {
        const std::string help_text = assemble_stat_help( ctxt );
        const int new_iHelpHeight = foldstring( help_text, getmaxx( w ) - 4 ).size();
        if( new_iHelpHeight != iHelpHeight ) {
            iHelpHeight = new_iHelpHeight;
            init_windows( ui );
        }
        werase( w );
        tabs.draw( w );
        mvwputch( w, point( iSecondColumn, iHeaderHeight - 1 ), BORDER_COLOR, LINE_OXXX );  // '┬'
        // Helptext stats tab
        fold_and_print( w, point( 2, TERMY - iHelpHeight - 1 ), getmaxx( w ) - 4, COL_NOTE_MINOR,
                        help_text );

        const point opt_pos( 2, sel + iHeaderHeight );
        if( screen_reader_mode ) {
            // This list only clutters up the screen in screen reader mode
        } else {
            for( int i = 0; i < 4; i++ ) {
                mvwprintz( w, point( 2, i + iHeaderHeight ), i == sel ? COL_SELECT : c_light_gray, "%s:",
                           stat_labels[i].translated() );
                mvwprintz( w, point( 16, i + iHeaderHeight ), c_light_gray, "%2d", *stats[i] );
            }
        }

        draw_points( w, pool, u );
        const point desc_line = point( iSecondColumn, 3 );
        warning_text = "";
        if( *stats[sel] <= min_stat_points ) {
            //~ %s - stat
            warning_text = string_format( _( "%s cannot be further decreased" ),
                                          stat_labels[sel].translated() );
        } else if( *stats[sel] >= max_stat_points ) {
            //~ %s - stat
            warning_text = string_format( _( "%s cannot be further increased" ),
                                          stat_labels[sel].translated() );
        } else if( *stats[sel] >= HIGH_STAT && pool != pool_type::FREEFORM ) {
            //~ %s - stat
            warning_text = string_format( _( "Increasing %s further costs 2 points" ),
                                          stat_labels[sel].translated() );
        }
        if( !warning_text.empty() && !screen_reader_mode ) {
            nc_color dummy = c_red;
            print_colored_text( w, desc_line, dummy, c_red, warning_text );
        }

        u.reset_stats();
        u.recalc_speed_bonus();
        u.set_stored_kcal( u.get_healthy_kcal() );
        u.reset_bonuses(); // Removes pollution of stats by modifications appearing inside reset_stats(). Is reset_stats() even necessary in this context?
        if( details_recalc ) {
            std::string stat_details;
            if( screen_reader_mode ) {
                stat_details = string_format( "%s: %i\n", stat_labels[sel].translated(), *stats[sel] );
                if( !last_stat.empty() && !stat_details.empty() && last_stat[0] == stat_details[0] ) {
                    // Shift the text to force the screen reader to read it
                    stat_details = " " + stat_details;
                }
                last_stat = stat_details;
                if( !warning_text.empty() ) {
                    stat_details.append( warning_text + "\n" );
                }
                stat_details.append( assemble_stat_details( u, sel ) );
            } else {
                stat_details = assemble_stat_details( u, sel );
            }
            details.set_text( stat_details );
            details_recalc = false;
        }

        wnoutrefresh( w );
        ui.set_cursor( w_details_pane, point::zero );
        details.draw( COL_STAT_NEUTRAL );
    } );

    do {
        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        const int id_for_curr_description = sel;

        if( tabs.handle_input( action, ctxt ) ) {
            break; // Tab has changed or user has quit the screen
        } else if( details.handle_navigation( action, ctxt )
                   || navigate_ui_list( action, sel, 1, 4, true ) ) {
            // NO FURTHER ACTION REQUIRED
        } else if( action == "LEFT" ) {
            if( *stats[sel] > min_stat_points ) {
                ( *stats[sel] )--;
                details_recalc = true;
            }
        } else if( action == "RIGHT" ) {
            if( *stats[sel] < max_stat_points ) {
                ( *stats[sel] )++;
                details_recalc = true;
            }
        }
        if( sel != id_for_curr_description ) {
            details_recalc = true;
        }
    } while( true );
}

static struct {
    bool sort_by_points = false;
    /** @related player */
    bool operator()( const trait_id *a, const trait_id *b ) {
        return std::abs( ( *a )->points ) > std::abs( ( *b )->points );
    }
} traits_sorter;

static void add_trait( std::vector<trait_id> &to, const trait_id &trait )
{
    to.emplace_back( trait );
}

static const mutation_variant *variant_trait_selection_menu( const trait_id &cur_trait )
{
    // Because the keys will change on each loop if I clear the entries, and
    // if I don't clear the entries, the menu bugs out
    static std::array<int, 60> keys = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                                        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
                                        'n', 'o', 'p',/*q */'r', 's', 't',/*u */'v', 'w', 'x', 'y', 'z',
                                        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
                                        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
                                      };
    uilist menu;
    const mutation_variant *ret = nullptr;
    avatar &pc = get_avatar();
    std::vector<const mutation_variant *> variants;
    variants.reserve( cur_trait->variants.size() );
    for( const std::pair<const std::string, mutation_variant> &pr : cur_trait->variants ) {
        if( pc.has_trait_variant( {cur_trait, pr.first} ) ) {
            ret = &pr.second;
        }
        variants.emplace_back( &pr.second );
    }

    menu.title = _( "Which trait?" );
    menu.desc_enabled = true;
    menu.allow_cancel = false;
    int idx;
    do {
        menu.entries.clear();
        idx = 0;
        menu.addentry_desc( idx, true, 'u', ret != nullptr ? _( "Unselect" ) :
                            colorize( _( "Unselected" ), c_white ),
                            _( "Remove this trait." ) );
        ++idx;
        for( const mutation_variant *var : variants ) {
            std::string name = var->alt_name.translated();
            menu.addentry_desc( idx, true, keys[( idx - 1 ) % keys.size()],
                                ( ret && ret == var ) ? colorize( name, c_white ) : name,
                                var->alt_description.translated() );
            ++idx;
        }
        menu.addentry_desc( idx, true, 'q', _( "Done" ), _( "Exit menu." ) );
        menu.query();
        if( menu.ret == 0 ) {
            ret = nullptr;
        } else if( menu.ret < idx ) {
            ret = variants[menu.ret - 1];
        }
    } while( menu.ret != idx );
    return ret;
}

void set_traits( tab_manager &tabs, avatar &u, pool_type pool )
{
    const int max_trait_points = get_option<int>( "MAX_TRAIT_POINTS" );
    const bool screen_reader_mode = get_option<bool>( "SCREEN_READER_MODE" );
    std::string last_trait; // Used in screen_reader_mode to ensure full trait name is read
    // Track how many good / bad POINTS we have; cap both at MAX_TRAIT_POINTS
    int num_good = 0;
    int num_bad = 0;
    // 0 -> traits that take points ( positive traits )
    // 1 -> traits that give points ( negative traits )
    // 2 -> neutral traits ( facial hair, skin color, etc )
    std::array<std::vector<trait_id>, 3> vStartingTraits;

    for( const mutation_branch &traits_iter : mutation_branch::get_all() ) {
        // Don't list blacklisted traits
        if( mutation_branch::trait_is_blacklisted( traits_iter.id ) ) {
            continue;
        }

        const std::set<trait_id> scentraits = get_scenario()->get_locked_traits();
        const bool is_scentrait = scentraits.find( traits_iter.id ) != scentraits.end();

        // Always show profession locked traits, regardless of if they are forbidden
        const std::vector<trait_and_var> proftraits = u.prof->get_locked_traits();
        auto pred = [&traits_iter]( const trait_and_var & query ) {
            return query.trait == traits_iter.id;
        };
        const bool is_proftrait = std::find_if( proftraits.begin(), proftraits.end(),
                                                pred ) != proftraits.end();

        bool is_hobby_locked_trait = false;
        for( const profession *hobby : u.hobbies ) {
            is_hobby_locked_trait = is_hobby_locked_trait || hobby->is_locked_trait( traits_iter.id );
        }

        // We show all starting traits, even if we can't pick them, to keep the interface consistent.
        if( traits_iter.startingtrait || get_scenario()->traitquery( traits_iter.id ) || is_proftrait ||
            is_hobby_locked_trait ) {
            if( traits_iter.points > 0 ) {
                add_trait( vStartingTraits[0], traits_iter.id );

                if( is_proftrait || is_scentrait ) {
                    continue;
                }

                if( u.has_trait( traits_iter.id ) ) {
                    num_good += traits_iter.points;
                }
            } else if( traits_iter.points < 0 ) {
                add_trait( vStartingTraits[1], traits_iter.id );

                if( is_proftrait || is_scentrait ) {
                    continue;
                }

                if( u.has_trait( traits_iter.id ) ) {
                    num_bad += traits_iter.points;
                }
            } else {
                add_trait( vStartingTraits[2], traits_iter.id );
            }
        }
    }
    //If the third page is empty, only use the first two.
    const int used_pages = vStartingTraits[2].empty() ? 2 : 3;

    for( auto &vStartingTrait : vStartingTraits ) {
        std::sort( vStartingTrait.begin(), vStartingTrait.end(), trait_display_nocolor_sort );
    }

    int iCurWorkingPage = 0;
    std::array<int, 3> iStartPos = { 0, 0, 0 };
    std::array<int, 3> iCurrentLine = { 0, 0, 0 };
    std::array<size_t, 3> traits_size;
    bool recalc_traits = false;
    // pointer for memory footprint reasons
    std::array<std::vector<const trait_id *>, 3> sorted_traits;
    std::array<scrollbar, 3> trait_sbs;
    std::string filterstring;

    for( int i = 0; i < 3; i++ ) {
        const size_t size = vStartingTraits[i].size();
        traits_size[i] = size;
        sorted_traits[i].reserve( size );
        for( size_t j = 0; j < size; j++ ) {
            sorted_traits[i].emplace_back( &vStartingTraits[i][j] );
        }
    }

    const int iHeaderHeight = 6;
    const int iDetailHeight = 3;
    size_t iContentHeight = 0;
    size_t page_width = 0;

    const auto pos_calc = [&]() {
        for( int i = 0; i < 3; i++ ) {
            // Shift start position to avoid iterating beyond end
            traits_size[i] = sorted_traits[i].size();
            int total = static_cast<int>( traits_size[i] );
            int height = static_cast<int>( iContentHeight );
            iStartPos[i] = std::min( iStartPos[i], std::max( 0, total - height ) );
        }
    };

    // this will return the next non empty page
    // there will always be at least one non-empty page
    // iCurWorkingPage will always be a non-empty page
    const auto next_avail_page = [&traits_size, &iCurWorkingPage]( bool invert_direction ) -> int {
        int prev_page = iCurWorkingPage < 1 ? 2 : iCurWorkingPage - 1;
        if( !traits_size[prev_page] )
        {
            prev_page = prev_page < 1 ? 2 : prev_page - 1;
            if( !traits_size[prev_page] ) {
                prev_page = prev_page < 1 ? 2 : prev_page - 1;
            }
        }

        int next_page = iCurWorkingPage > 1 ? 0 : iCurWorkingPage + 1;
        if( !traits_size[next_page] )
        {
            next_page = next_page > 1 ? 0 : next_page + 1;
            if( !traits_size[next_page] ) {
                next_page = next_page > 1 ? 0 : next_page + 1;
            }
        }

        return invert_direction ? prev_page : next_page;
    };

    ui_adaptor ui;
    catacurses::window w;
    catacurses::window w_details_pane;
    scrolling_text_view details( w_details_pane );
    bool details_recalc = true;
    const auto init_windows = [&]( ui_adaptor & ui ) {
        w = catacurses::newwin( TERMY, TERMX, point::zero );
        w_details_pane = catacurses::newwin( iDetailHeight, TERMX - 1,
                                             point( 0, TERMY - iDetailHeight - 1 ) );
        ui.position_from_window( w );
        page_width = std::min( ( TERMX - 4 ) / used_pages, 38 );
        iContentHeight = TERMY - iHeaderHeight - iDetailHeight - 1;

        details_recalc = true;
        pos_calc();
    };
    init_windows( ui );
    ui.on_screen_resize( init_windows );

    input_context ctxt( "NEW_CHAR_TRAITS" );
    tabs.set_up_tab_navigation( ctxt );
    details.set_up_navigation( ctxt, scrolling_key_scheme::angle_bracket_scroll );
    for( scrollbar &sb : trait_sbs ) {
        sb.set_draggable( ctxt );
    }
    ctxt.register_navigate_ui_list();
    ctxt.register_leftright();
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "RESET_FILTER" );
    ctxt.register_action( "SORT" );
    ctxt.register_action( "RANDOMIZE" );

    ui.on_redraw( [&]( ui_adaptor & ui ) {
        werase( w );

        tabs.draw( w );
        wattron( w, BORDER_COLOR );
        for( int i = 1; i < 3; ++i ) {
            mvwaddch( w, point( i * page_width, iHeaderHeight - 1 ), LINE_OXXX );  // '┬'
        }
        wattroff( w, BORDER_COLOR );
        draw_filter_and_sorting_indicators( w, ctxt, filterstring, traits_sorter );
        draw_points( w, pool, u );
        int full_string_length = 0;
        const int remaining_points_length = utf8_width( pools_to_string( u, pool ), true );
        if( pool != pool_type::FREEFORM ) {
            std::string full_string =
                string_format( "<color_light_green>%2d/%-2d</color> <color_light_red>%3d/-%-2d</color>",
                               num_good, max_trait_points, num_bad, max_trait_points );
            fold_and_print( w, point( remaining_points_length + 3, 3 ), getmaxx( w ) - 2, c_white,
                            full_string );
            full_string_length = utf8_width( full_string, true ) + remaining_points_length + 3;
        } else {
            full_string_length = remaining_points_length + 3;
        }

        for( int iCurrentPage = 0; iCurrentPage < 3; iCurrentPage++ ) {
            nc_color col_on_act;
            nc_color col_off_act;
            nc_color col_on_pas;
            nc_color col_off_pas;
            nc_color col_tr;
            switch( iCurrentPage ) {
                case 0:
                    col_on_act = COL_TR_GOOD_ON_ACT;
                    col_off_act = COL_TR_GOOD_OFF_ACT;
                    col_on_pas = COL_TR_GOOD_ON_PAS;
                    col_off_pas = COL_TR_GOOD_OFF_PAS;
                    col_tr = COL_TR_GOOD;
                    break;
                case 1:
                    col_on_act = COL_TR_BAD_ON_ACT;
                    col_off_act = COL_TR_BAD_OFF_ACT;
                    col_on_pas = COL_TR_BAD_ON_PAS;
                    col_off_pas = COL_TR_BAD_OFF_PAS;
                    col_tr = COL_TR_BAD;
                    break;
                default:
                    col_on_act = COL_TR_NEUT_ON_ACT;
                    col_off_act = COL_TR_NEUT_OFF_ACT;
                    col_on_pas = COL_TR_NEUT_ON_PAS;
                    col_off_pas = COL_TR_NEUT_OFF_PAS;
                    col_tr = COL_TR_NEUT;
                    break;
            }
            nc_color hi_on = hilite( col_on_act );
            nc_color hi_off = hilite( c_white );

            int &start = iStartPos[iCurrentPage];
            int current = iCurrentLine[iCurrentPage];
            calcStartPos( start, current, iContentHeight, traits_size[iCurrentPage] );
            int end = start + static_cast<int>( std::min( traits_size[iCurrentPage], iContentHeight ) );

            for( int i = start; i < end; i++ ) {
                const trait_id &cur_trait = *sorted_traits[iCurrentPage][i];
                if( current == i && iCurrentPage == iCurWorkingPage ) {
                    int points = cur_trait->points;
                    bool negativeTrait = points < 0;
                    if( negativeTrait ) {
                        points *= -1;
                    }
                    if( pool != pool_type::FREEFORM ) {
                        mvwprintz( w, point( full_string_length + 3, 3 ), col_tr,
                                   n_gettext( "%s %s %d point", "%s %s %d points", points ),
                                   cur_trait->name(),
                                   negativeTrait ? _( "earns" ) : _( "costs" ),
                                   points );
                    }
                }

                nc_color cLine = col_off_pas;
                if( iCurWorkingPage == iCurrentPage ) {
                    cLine = col_off_act;
                    if( current == i ) {
                        cLine = hi_off;
                        if( u.has_conflicting_trait( cur_trait ) ) {
                            cLine = hilite( c_dark_gray );
                        } else if( u.has_trait( cur_trait ) ) {
                            cLine = hi_on;
                        }
                    } else {
                        if( u.has_conflicting_trait( cur_trait ) || get_scenario()->is_forbidden_trait( cur_trait ) ) {
                            cLine = c_dark_gray;

                        } else if( u.has_trait( cur_trait ) ) {
                            cLine = col_on_act;
                        }
                    }
                } else if( u.has_trait( cur_trait ) ) {
                    cLine = col_on_pas;

                } else if( u.has_conflicting_trait( cur_trait ) ||
                           get_scenario()->is_forbidden_trait( cur_trait ) ) {
                    cLine = c_light_gray;
                }

                const int cur_line_y = iHeaderHeight + i - start;
                const int cur_line_x = 2 + iCurrentPage * page_width;
                const point opt_pos( cur_line_x, cur_line_y );
                if( screen_reader_mode ) {
                    // This list only clutters up the screen in screen reader mode
                } else {
                    mvwprintz( w, opt_pos, cLine,
                               utf8_truncate( cur_trait->name(), page_width - 2 ) );
                }
            }

            if( details_recalc ) {
                std::string description;
                const trait_id &cur_trait = *sorted_traits[iCurWorkingPage][iCurrentLine[iCurWorkingPage]];
                if( screen_reader_mode ) {
                    /* Screen readers will skip over text that has not changed.  Since the lists of traits are
                    * alphabetical, this frequently results in letters/words being skipped.  So, if the screen
                    * reader is likely to skip over part of a trait name, we trick it into thinking things have
                    * changed by shifting the text slightly.
                     */
                    if( !last_trait.empty() && last_trait[0] == cur_trait->name()[0] ) {
                        description = " " + cur_trait->name();
                    } else {
                        description = cur_trait->name();
                    }
                    last_trait = description;

                    std::string cur_trait_notes;
                    if( u.has_conflicting_trait( cur_trait ) ) {
                        cur_trait_notes = _( "a conflicting trait is active" );
                    } else if( u.has_trait( cur_trait ) ) {
                        cur_trait_notes = _( "active" );
                    }

                    if( !cur_trait_notes.empty() ) {
                        description.append( string_format( " - %s", cur_trait_notes ) );
                    }

                    description.append( "\n" + cur_trait->desc() );
                } else {
                    description = cur_trait->desc();
                }
                details.set_text( colorize( description, col_tr ) );
                details_recalc = false;
            }

            trait_sbs[iCurrentPage].offset_x( page_width * iCurrentPage )
            .offset_y( iHeaderHeight )
            .content_size( traits_size[iCurrentPage] )
            .viewport_pos( start )
            .viewport_size( iContentHeight )
            .apply( w );
        }

        wnoutrefresh( w );
        ui.set_cursor( w_details_pane, point::zero );
        // color is never visible (COL_TR_NEUT), text is already colorized
        details.draw( COL_TR_NEUT );
    } );

    do {
        if( recalc_traits ) {
            for( int i = 0; i < 3; i++ ) {
                const size_t size = vStartingTraits[i].size();
                sorted_traits[i].clear();
                for( size_t j = 0; j < size; j++ ) {
                    sorted_traits[i].emplace_back( &vStartingTraits[i][j] );
                }
            }

            if( !filterstring.empty() ) {
                for( std::vector<const trait_id *> &traits : sorted_traits ) {
                    const auto new_end_iter = std::remove_if(
                                                  traits.begin(),
                                                  traits.end(),
                    [&filterstring]( const trait_id * trait ) {
                        return !lcmatch( ( *trait )->name(), filterstring );
                    } );

                    traits.erase( new_end_iter, traits.end() );
                }
            }

            if( !filterstring.empty() && sorted_traits[0].empty() && sorted_traits[1].empty() &&
                sorted_traits[2].empty() ) {
                popup( _( "Nothing found." ) ); // another case of black box in tiles
                filterstring.clear();
                continue;
            }

            if( traits_sorter.sort_by_points ) {
                std::stable_sort( sorted_traits[0].begin(), sorted_traits[0].end(), traits_sorter );
                std::stable_sort( sorted_traits[1].begin(), sorted_traits[1].end(), traits_sorter );
            }

            // Select the current page, if not empty
            // There should always be at least one not empty page
            iCurrentLine[0] = 0;
            iCurrentLine[1] = 0;
            iCurrentLine[2] = 0;
            if( sorted_traits[iCurWorkingPage].empty() ) {
                iCurWorkingPage = 0;
                if( !sorted_traits[0].empty() ) {
                    iCurWorkingPage = 0;
                } else if( !sorted_traits[1].empty() ) {
                    iCurWorkingPage = 1;
                } else if( !sorted_traits[2].empty() ) {
                    iCurWorkingPage = 2;
                }
            }

            pos_calc();
            recalc_traits = false;
        }

        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        std::array< int, 3> cur_sb_pos = iStartPos;
        bool scrollbar_handled = false;
        const int iPrevWorkingPage = iCurWorkingPage;
        const int iPrevLine = iCurrentLine[iCurWorkingPage];
        for( int i = 0; i < static_cast<int>( trait_sbs.size() ); ++i ) {
            if( trait_sbs[i].handle_dragging( action, ctxt.get_coordinates_text( catacurses::stdscr ),
                                              cur_sb_pos[i] ) ) {
                if( cur_sb_pos[i] != iStartPos[i] ) {
                    iStartPos[i] = cur_sb_pos[i];
                    iCurrentLine[i] = iStartPos[i] + ( iContentHeight - 1 ) / 2;
                }
                scrollbar_handled = true;
            }
        }

        if( tabs.handle_input( action, ctxt ) ) {
            break; // Tab has changed or user has quit the screen
        } else if( action == "LEFT" || action == "RIGHT" ) {
            iCurWorkingPage = next_avail_page( action == "LEFT" );
        } else if( scrollbar_handled
                   || details.handle_navigation( action, ctxt )
                   || navigate_ui_list( action, iCurrentLine[iCurWorkingPage], 10,
                                        traits_size[iCurWorkingPage], true ) ) {
            // No additional action required
        } else if( action == "CONFIRM" ) {
            int inc_type = 0;
            const trait_id cur_trait = *sorted_traits[iCurWorkingPage][iCurrentLine[iCurWorkingPage]];
            const mutation_branch &mdata = cur_trait.obj();
            std::string variant;

            // Look through the profession bionics, and see if any of them conflict with this trait
            std::vector<bionic_id> cbms_blocking_trait = bionics_cancelling_trait( u.prof->CBMs(), cur_trait );
            const std::unordered_set<trait_id> conflicting_traits = u.get_conflicting_traits( cur_trait );

            if( u.has_trait( cur_trait ) ) {
                if( !cur_trait->variants.empty() ) {
                    const mutation_variant *rval = variant_trait_selection_menu( cur_trait );
                    if( rval == nullptr ) {
                        inc_type = -1;
                    } else {
                        u.set_mut_variant( cur_trait, rval );
                    }
                } else {
                    inc_type = -1;

                    if( get_scenario()->is_locked_trait( cur_trait ) ) {
                        inc_type = 0;
                        popup( _( "Your scenario of %s prevents you from removing this trait." ),
                               get_scenario()->gender_appropriate_name( u.male ) );
                    } else if( u.prof->is_locked_trait( cur_trait ) ) {
                        inc_type = 0;
                        popup( _( "Your profession of %s prevents you from removing this trait." ),
                               u.prof->gender_appropriate_name( u.male ) );
                    }
                    for( const profession *hobbies : u.hobbies ) {
                        if( hobbies->is_locked_trait( cur_trait ) ) {
                            inc_type = 0;
                            popup( _( "Your background of %s prevents you from removing this trait." ),
                                   hobbies->gender_appropriate_name( u.male ) );
                        }
                    }
                }
            } else if( !conflicting_traits.empty() ) {
                std::vector<std::string> conflict_names;
                conflict_names.reserve( conflicting_traits.size() );
                for( const trait_id &trait : conflicting_traits ) {
                    conflict_names.emplace_back( u.mutation_name( trait ) );
                }
                popup( _( "You already picked some conflicting traits: %s." ),
                       enumerate_as_string( conflict_names ) );
            } else if( get_scenario()->is_forbidden_trait( cur_trait ) ) {
                popup( _( "The scenario you picked prevents you from taking this trait!" ) );
            } else if( u.prof->is_forbidden_trait( cur_trait ) ) {
                popup( _( "Your profession of %s prevents you from taking this trait." ),
                       u.prof->gender_appropriate_name( u.male ) );
            } else if( !cbms_blocking_trait.empty() ) {
                // Grab a list of the names of the bionics that block this trait
                // So that the player know what is preventing them from taking it
                std::vector<std::string> conflict_names;
                conflict_names.reserve( cbms_blocking_trait.size() );
                for( const bionic_id &conflict : cbms_blocking_trait ) {
                    conflict_names.emplace_back( conflict->name.translated() );
                }
                popup( _( "The following bionics prevent you from taking this trait: %s." ),
                       enumerate_as_string( conflict_names ) );
            } else if( iCurWorkingPage == 0 && num_good + mdata.points >
                       max_trait_points && pool != pool_type::FREEFORM ) {
                popup( n_gettext( "Sorry, but you can only take %d point of advantages.",
                                  "Sorry, but you can only take %d points of advantages.", max_trait_points ),
                       max_trait_points );

            } else if( iCurWorkingPage != 0 && num_bad + mdata.points <
                       -max_trait_points && pool != pool_type::FREEFORM ) {
                popup( n_gettext( "Sorry, but you can only take %d point of disadvantages.",
                                  "Sorry, but you can only take %d points of disadvantages.", max_trait_points ),
                       max_trait_points );

            } else {
                if( !cur_trait->variants.empty() ) {
                    const mutation_variant *rval = variant_trait_selection_menu( cur_trait );
                    if( rval != nullptr ) {
                        inc_type = 1;
                        variant = rval->id;
                    } else {
                        inc_type = 0;
                    }
                } else {
                    inc_type = 1;
                }
            }

            //inc_type is either -1 or 1, so we can just multiply by it to invert
            if( inc_type != 0 ) {
                details_recalc = true;
                u.toggle_trait_deps( cur_trait, variant );
                if( iCurWorkingPage == 0 ) {
                    num_good += mdata.points * inc_type;
                } else {
                    num_bad += mdata.points * inc_type;
                }
            }
        } else if( action == "SORT" ) {
            traits_sorter.sort_by_points = !traits_sorter.sort_by_points;
            recalc_traits = true;
        } else if( action == "FILTER" ) {
            string_input_popup()
            .title( _( "Search:" ) )
            .width( 10 )
            .description( _( "Search by trait name." ) )
            .edit( filterstring );
            recalc_traits = true;
        } else if( action == "RESET_FILTER" ) {
            if( !filterstring.empty() ) {
                filterstring.clear();
                recalc_traits = true;
            }
        } else if( action == "RANDOMIZE" ) {
            iCurrentLine[iCurWorkingPage] = rng( 0, traits_size[iCurWorkingPage] - 1 );
        }
        if( iCurWorkingPage != iPrevWorkingPage || iCurrentLine[iCurWorkingPage] != iPrevLine ) {
            details_recalc = true;
        }
    } while( true );
}

static struct {
    bool sort_by_points = true;
    bool male = false;
    /** @related player */
    bool operator()( const string_id<profession> &a, const string_id<profession> &b ) const {
        // The generic ("Unemployed") profession should be listed first.
        const profession *gen = profession::generic();
        if( &b.obj() == gen ) {
            return false;
        } else if( &a.obj() == gen ) {
            return true;
        }

        if( !a->can_pick().success() && b->can_pick().success() ) {
            return false;
        }

        if( a->can_pick().success() && !b->can_pick().success() ) {
            return true;
        }

        if( sort_by_points ) {
            return a->point_cost() < b->point_cost();
        } else {
            return localized_compare( a->gender_appropriate_name( male ),
                                      b->gender_appropriate_name( male ) );
        }
    }
} profession_sorter;

static std::string assemble_profession_details( const avatar &u, const input_context &ctxt,
        const std::vector<string_id<profession>> &sorted_profs, const int cur_id, const std::string &notes )
{
    std::string assembled = get_origin( sorted_profs[cur_id]->src ) + "\n";

    std::string profession_name = sorted_profs[cur_id]->gender_appropriate_name( u.male );
    if( get_option<bool>( "SCREEN_READER_MODE" ) && !notes.empty() ) {
        profession_name = profession_name.append( string_format( " - %s", notes ) );
    }
    assembled += string_format( g_switch_msg( u ), ctxt.get_desc( "CHANGE_GENDER" ),
                                profession_name ) + "\n";
    assembled += string_format( dress_switch_msg(), ctxt.get_desc( "CHANGE_OUTFIT" ) ) + "\n";

    if( sorted_profs[cur_id]->get_requirement().has_value() ) {
        assembled += "\n" + colorize( _( "Profession requirements:" ), COL_HEADER ) + "\n";
        assembled += string_format( _( "Complete \"%s\"\n" ),
                                    sorted_profs[cur_id]->get_requirement().value()->name() );
    }
    //Profession story
    assembled += "\n" + colorize( _( "Profession story:" ), COL_HEADER ) + "\n";
    if( !sorted_profs[cur_id]->can_pick().success() ) {
        assembled += colorize( sorted_profs[cur_id]->can_pick().str(), c_red ) + "\n";
    }
    assembled += colorize( sorted_profs[cur_id]->description( u.male ), c_green ) + "\n";

    // Profession addictions
    const auto prof_addictions = sorted_profs[cur_id]->addictions();
    if( !prof_addictions.empty() ) {
        assembled += "\n" + colorize( _( "Profession addictions:" ), COL_HEADER ) + "\n";
        for( const addiction &a : prof_addictions ) {
            const char *format = pgettext( "set_profession_addictions", "%1$s (%2$d)" );
            assembled += string_format( format, a.type->get_name().translated(), a.intensity ) + "\n";
        }
    }

    // Profession traits
    const auto prof_traits = sorted_profs[cur_id]->get_locked_traits();
    assembled += "\n" + colorize( _( "Profession traits:" ), COL_HEADER ) + "\n";
    if( prof_traits.empty() ) {
        assembled += pgettext( "set_profession_trait", "None" ) + std::string( "\n" );
    } else {
        for( const trait_and_var &t : prof_traits ) {
            assembled += t.name() + "\n";
        }
    }

    // Profession martial art styles
    const auto prof_ma_known = sorted_profs[cur_id]->ma_known();
    const auto prof_ma_choices = sorted_profs[cur_id]->ma_choices();
    int ma_amount = sorted_profs[cur_id]->ma_choice_amount;
    assembled += "\n" + colorize( _( "Profession martial arts:" ), COL_HEADER ) + "\n";
    if( prof_ma_known.empty() && prof_ma_choices.empty() ) {
        assembled += pgettext( "set_profession_ma", "None" ) + std::string( "\n" );
    } else {
        if( !prof_ma_known.empty() ) {
            assembled += colorize( _( "Known:" ), c_cyan ) + "\n";
            for( const matype_id &ma : prof_ma_known ) {
                const martialart &style = ma.obj();
                assembled += style.name.translated() + "\n";
            }
        }
        if( !prof_ma_known.empty() && !prof_ma_choices.empty() ) {
            assembled += "\n";
        }
        if( !prof_ma_choices.empty() ) {
            assembled += colorize( string_format( _( "Choose %d:" ), ma_amount ), c_cyan ) + "\n";
            for( const matype_id &ma : prof_ma_choices ) {
                const martialart &style = ma.obj();
                assembled += style.name.translated() + "\n";
            }
        }
    }

    // Profession skills
    const profession::StartingSkillList prof_skills = sorted_profs[cur_id]->skills();
    assembled += "\n" + colorize( _( "Profession skills:" ), COL_HEADER ) + "\n";
    if( prof_skills.empty() ) {
        assembled += pgettext( "set_profession_skill", "None" ) + std::string( "\n" );
    } else {
        for( const auto &sl : prof_skills ) {
            const char *format = pgettext( "set_profession_skill", "%1$s (%2$d)" );
            assembled += string_format( format, sl.first.obj().name(), sl.second ) + "\n";
        }
    }

    // Profession items
    const auto prof_items = sorted_profs[cur_id]->items( outfit, u.get_mutations() );
    assembled += "\n" + colorize( _( "Profession items:" ), COL_HEADER ) + "\n";
    if( prof_items.empty() ) {
        assembled += pgettext( "set_profession_item", "None" ) + std::string( "\n" );
    } else {
        // TODO: If the item group is randomized *at all*, these will be different each time
        // and it won't match what you actually start with
        // TODO: Put like items together like the inventory does, so we don't have to scroll
        // through a list of a dozen forks.
        std::string assembled_wielded;
        std::string assembled_worn;
        std::string assembled_inventory;
        for( const item &it : prof_items ) {
            if( it.has_flag( json_flag_no_auto_equip ) ) { // NOLINT(bugprone-branch-clone)
                assembled_inventory += it.display_name() + "\n";
            } else if( it.has_flag( json_flag_auto_wield ) ) {
                assembled_wielded += it.display_name() + "\n";
            } else if( it.is_armor() ) {
                assembled_worn += it.display_name() + "\n";
            } else {
                assembled_inventory += it.display_name() + "\n";
            }
        }
        assembled += colorize( _( "Wielded:" ), c_cyan ) + "\n";
        assembled += !assembled_wielded.empty() ? assembled_wielded :
                     pgettext( "set_profession_item_wielded",
                               "None\n" );
        assembled += colorize( _( "Worn:" ), c_cyan ) + "\n";
        assembled += !assembled_worn.empty() ? assembled_worn : pgettext( "set_profession_item_worn",
                     "None\n" );
        assembled += colorize( _( "Inventory:" ), c_cyan ) + "\n";
        assembled += !assembled_inventory.empty() ? assembled_inventory :
                     pgettext( "set_profession_item_inventory",
                               "None\n" );
    }

    // Profession bionics, active bionics shown first
    auto prof_CBMs = sorted_profs[cur_id]->CBMs();

    if( !prof_CBMs.empty() ) {
        assembled += "\n" + colorize( _( "Profession bionics:" ), COL_HEADER ) + "\n";
        std::sort( std::begin( prof_CBMs ), std::end( prof_CBMs ), []( const bionic_id & a,
        const bionic_id & b ) {
            return a->activated && !b->activated;
        } );
        for( const auto &b : prof_CBMs ) {
            const bionic_data &cbm = b.obj();
            if( cbm.activated && cbm.has_flag( json_flag_BIONIC_TOGGLED ) ) {
                assembled += string_format( _( "%s (toggled)" ), cbm.name ) + "\n";
            } else if( cbm.activated ) {
                assembled += string_format( _( "%s (activated)" ), cbm.name ) + "\n";
            } else {
                assembled += cbm.name + "\n";
            }
        }
    }
    // Proficiencies
    std::vector<proficiency_id> prof_proficiencies = sorted_profs[cur_id]->proficiencies();
    if( !prof_proficiencies.empty() ) {
        assembled += "\n" + colorize( _( "Profession proficiencies:" ), COL_HEADER ) + "\n";
        for( const proficiency_id &prof : prof_proficiencies ) {
            assembled += prof->name() + "\n";
        }
    }
    // Recipes
    std::vector<recipe_id> prof_recipe = sorted_profs[cur_id]->recipes();
    if( !prof_recipe.empty() ) {
        assembled += "\n" + colorize( _( "Profession recipes:" ), COL_HEADER ) + "\n";
        for( const recipe_id &prof : prof_recipe ) {
            assembled += prof->result_name() + "\n";
        }
    }
    // Profession pet
    if( !sorted_profs[cur_id]->pets().empty() ) {
        assembled += "\n" + colorize( _( "Profession pets:" ), COL_HEADER ) + "\n";
        for( const auto &elem : sorted_profs[cur_id]->pets() ) {
            monster mon( elem );
            assembled += mon.get_name() + "\n";
        }
    }
    // Profession vehicle
    if( sorted_profs[cur_id]->vehicle() ) {
        assembled += "\n" + colorize( _( "Profession vehicle:" ), COL_HEADER ) + "\n";
        vproto_id veh_id = sorted_profs[cur_id]->vehicle();
        assembled += veh_id->name.translated() + "\n";
    }
    // Profession spells
    if( !sorted_profs[cur_id]->spells().empty() ) {
        assembled += "\n" + colorize( _( "Profession spells:" ), COL_HEADER ) + "\n";
        for( const std::pair<const spell_id, int> &spell_pair : sorted_profs[cur_id]->spells() ) {
            assembled += string_format( _( "%s level %d" ), spell_pair.first->name, spell_pair.second ) + "\n";
        }
    }
    // Profession missions
    if( !sorted_profs[cur_id]->missions().empty() ) {
        assembled += "\n" + colorize( _( "Profession missions:" ), COL_HEADER ) + "\n";
        for( mission_type_id mission_id : sorted_profs[cur_id]->missions() ) {
            assembled += mission_type::get( mission_id )->tname() + "\n";
        }
    }

    // Profession money
    std::optional<int> cash = sorted_profs[cur_id]->starting_cash();

    if( cash.has_value() ) {
        assembled += "\n" + colorize( _( "Profession money:" ), COL_HEADER ) + "\n";
        assembled += format_money( cash.value() ) + "\n";
    }

    return assembled;
}

/** Helper to filter and move the cursor in the hobby/profession lists */
template<typename T, typename S>
size_t filter_entries( avatar &u, int &cur_id, std::vector<T> &old_entries,
                       std::vector<T> &new_entries, T chosen_entry,
                       std::string filterstring, S sorter )
{
    T previously_highlighted = old_entries.empty() ? T() : old_entries[cur_id];

    old_entries = new_entries;

    // Filter the list of entries
    const auto new_end = std::remove_if( old_entries.begin(),
    old_entries.end(), [&]( const T & arg ) {
        return !lcmatch( arg->gender_appropriate_name( u.male ), filterstring );
    } );
    old_entries.erase( new_end, old_entries.end() );

    if( old_entries.empty() ) {
        popup( _( "Nothing found." ) ); // another case of black box in tiles
        return 0; // tell caller to try again without a filterstring
    }

    int entries_length = old_entries.size();

    std::stable_sort( old_entries.begin(), old_entries.end(), sorter );

    bool match = false;

    // Put the cursor on the previously highlighted entry, if possible.
    for( int i = 0; i < entries_length; ++i ) {
        if( old_entries[i] == previously_highlighted ) {
            cur_id = i;
            match = true;
            break;
        }
    }

    if( !match ) {
        // Pur the cursor on the currently chosen entry, if possible.
        for( int i = 0; i < entries_length; ++i ) {
            if( old_entries[i] == chosen_entry ) {
                cur_id = i;
                match = true;
                break;
            }
        }
    }

    if( !match ) {
        cur_id = 0;
    }

    return old_entries.size();
}

/** Handle the profession tab of the character generation menu */
void set_profession( tab_manager &tabs, avatar &u, pool_type pool )
{
    int cur_id = 0;
    size_t iContentHeight = 0;
    int iStartPos = 0;

    const bool screen_reader_mode = get_option<bool>( "SCREEN_READER_MODE" );

    ui_adaptor ui;
    catacurses::window w;
    catacurses::window w_details_pane;
    scrolling_text_view details( w_details_pane );
    bool details_recalc = true;
    const int iHeaderHeight = 6;
    scrollbar list_sb;
    const auto init_windows = [&]( ui_adaptor & ui ) {
        iContentHeight = TERMY - iHeaderHeight - 1;
        w = catacurses::newwin( TERMY, TERMX, point::zero );
        w_details_pane = catacurses::newwin( iContentHeight, TERMX / 2 - 1, point( TERMX / 2,
                                             iHeaderHeight ) );
        details_recalc =  true;
        ui.position_from_window( w );
    };
    init_windows( ui );
    ui.on_screen_resize( init_windows );

    input_context ctxt( "NEW_CHAR_PROFESSIONS" );
    tabs.set_up_tab_navigation( ctxt );
    details.set_up_navigation( ctxt, scrolling_key_scheme::angle_bracket_scroll );
    list_sb.set_draggable( ctxt );
    ctxt.register_navigate_ui_list();
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "CHANGE_GENDER" );
    ctxt.register_action( "CHANGE_OUTFIT" );
    ctxt.register_action( "SORT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "RESET_FILTER" );
    ctxt.register_action( "RANDOMIZE" );

    bool recalc_profs = true;
    size_t profs_length = 0;
    std::string filterstring;
    std::vector<string_id<profession>> sorted_profs;

    ui.on_redraw( [&]( ui_adaptor & ui ) {
        werase( w );
        tabs.draw( w );
        mvwputch( w, point( TERMX / 2, iHeaderHeight - 1 ), BORDER_COLOR, LINE_OXXX );  // '┬'
        mvwputch( w, point( TERMX / 2, TERMY - 1 ), BORDER_COLOR, LINE_XXOX );  // '┴'
        draw_filter_and_sorting_indicators( w, ctxt, filterstring, profession_sorter );

        const bool cur_id_is_valid = cur_id >= 0 && static_cast<size_t>( cur_id ) < sorted_profs.size();
        if( cur_id_is_valid ) {
            int netPointCost = sorted_profs[cur_id]->point_cost() - u.prof->point_cost();
            ret_val<void> can_afford = sorted_profs[cur_id]->can_afford( u, skill_points_left( u, pool ) );
            ret_val<void> can_pick = sorted_profs[cur_id]->can_pick();
            int pointsForProf = sorted_profs[cur_id]->point_cost();
            bool negativeProf = pointsForProf < 0;
            if( negativeProf ) {
                pointsForProf *= -1;
            }

            // Draw header.
            draw_points( w, pool, u, netPointCost );
            if( pool != pool_type::FREEFORM ) {
                const char *prof_msg_temp;
                if( negativeProf ) {
                    //~ 1s - profession name, 2d - current character points.
                    prof_msg_temp = n_gettext( "Profession %1$s earns %2$d point",
                                               "Profession %1$s earns %2$d points",
                                               pointsForProf );
                } else {
                    //~ 1s - profession name, 2d - current character points.
                    prof_msg_temp = n_gettext( "Profession %1$s costs %2$d point",
                                               "Profession %1$s costs %2$d points",
                                               pointsForProf );
                }

                int pMsg_length = utf8_width( remove_color_tags( pools_to_string( u, pool ) ) );
                mvwprintz( w, point( pMsg_length + 9, 3 ), can_afford.success() ? c_green : c_light_red,
                           prof_msg_temp, sorted_profs[cur_id]->gender_appropriate_name( u.male ), pointsForProf );
            }
        }

        //Draw options
        calcStartPos( iStartPos, cur_id, iContentHeight, profs_length );
        const int end_pos = iStartPos + std::min( { iContentHeight, profs_length, sorted_profs.size() } );
        std::string cur_prof_notes;
        for( int i = iStartPos; i < end_pos; i++ ) {
            nc_color col;
            if( u.prof != &sorted_profs[i].obj() ) {

                if( cur_id_is_valid && sorted_profs[i] == sorted_profs[cur_id] &&
                    !sorted_profs[i]->can_pick().success() ) {
                    col = h_dark_gray;
                    if( i == cur_id ) {
                        cur_prof_notes = _( "unavailable" );
                    }
                } else if( cur_id_is_valid && sorted_profs[i] != sorted_profs[cur_id] &&
                           !sorted_profs[i]->can_pick().success() ) {
                    col = c_dark_gray;
                    if( i == cur_id ) {
                        cur_prof_notes = _( "unavailable" );
                    }
                } else {
                    col = ( cur_id_is_valid && sorted_profs[i] == sorted_profs[cur_id] ? COL_SELECT : c_light_gray );
                }
            } else {
                col = ( cur_id_is_valid &&
                        sorted_profs[i] == sorted_profs[cur_id] ? hilite( c_light_green ) : COL_SKILL_USED );
                if( i == cur_id ) {
                    cur_prof_notes = _( "active" );
                }
            }
            if( screen_reader_mode ) {
                // This list only clutters up the screen in screen reader mode
            } else {
                const point opt_pos( 2, iHeaderHeight + i - iStartPos );
                mvwprintz( w, opt_pos, col,
                           sorted_profs[i]->gender_appropriate_name( u.male ) );
            }
        }

        if( details_recalc && cur_id_is_valid ) {
            details.set_text( assemble_profession_details( u, ctxt, sorted_profs, cur_id, cur_prof_notes ) );
            details_recalc = false;
        }

        list_sb.offset_x( 0 )
        .offset_y( iHeaderHeight )
        .content_size( profs_length )
        .viewport_pos( iStartPos )
        .viewport_size( iContentHeight )
        .apply( w );

        wnoutrefresh( w );
        ui.set_cursor( w_details_pane, point::zero );
        details.draw( c_light_gray );
    } );

    do {
        if( recalc_profs ) {
            std::vector<profession_id> new_profs = get_scenario()->permitted_professions();
            profession_sorter.male = u.male;
            if( ( profs_length = filter_entries( u, cur_id, sorted_profs, new_profs, u.prof->ident(),
                                                 filterstring,
                                                 profession_sorter ) ) == 0 ) {
                filterstring.clear();
                continue;
            }
            recalc_profs = false;
        }

        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        const int recmax = profs_length;
        const int scroll_rate = recmax > 20 ? 10 : 2;
        const int id_for_curr_description = cur_id;
        int scrollbar_pos = iStartPos;

        if( tabs.handle_input( action, ctxt ) ) {
            break; // Tab has changed or user has quit the screen
        } else if( details.handle_navigation( action, ctxt )
                   || navigate_ui_list( action, cur_id, scroll_rate, recmax, true ) ) {
            // NO FURTHER ACTION REQUIRED
        } else if( list_sb.handle_dragging( action, ctxt.get_coordinates_text( catacurses::stdscr ),
                                            scrollbar_pos ) ) {
            if( scrollbar_pos != iStartPos ) {
                iStartPos = scrollbar_pos;
                cur_id = iStartPos + ( iContentHeight - 1 ) / 2;
            }
        } else if( action == "CONFIRM" ) {
            ret_val<void> can_pick = sorted_profs[cur_id]->can_pick();

            if( !can_pick.success() ) {
                popup( can_pick.str() );
                continue;
            }

            // Selecting a profession will, under certain circumstances, change the detail text
            details_recalc = true;

            // Remove traits from the previous profession
            for( const trait_and_var &old : u.prof->get_locked_traits() ) {
                u.toggle_trait_deps( old.trait );
            }

            u.prof = &sorted_profs[cur_id].obj();

            // Remove pre-selected traits that conflict
            // with the new profession's traits
            for( const trait_and_var &new_trait : u.prof->get_locked_traits() ) {
                if( u.has_conflicting_trait( new_trait.trait ) ) {
                    for( const trait_id &suspect_trait : u.get_mutations() ) {
                        if( are_conflicting_traits( new_trait.trait, suspect_trait ) ) {
                            popup( _( "Your trait %1$s has been removed since it conflicts with the %2$s's %3$s trait." ),
                                   u.mutation_name( suspect_trait ), u.prof->gender_appropriate_name( u.male ), new_trait.name() );
                            u.toggle_trait_deps( suspect_trait );
                        }
                    }
                }
            }
            // Add traits for the new profession (and perhaps scenario, if, for example,
            // both the scenario and old profession require the same trait)
            u.add_traits();
        } else if( action == "CHANGE_OUTFIT" ) {
            outfit = !outfit;
            recalc_profs = true;
        } else if( action == "CHANGE_GENDER" ) {
            u.male = !u.male;
            profession_sorter.male = u.male;
            if( !profession_sorter.sort_by_points ) {
                std::sort( sorted_profs.begin(), sorted_profs.end(), profession_sorter );
            }
            recalc_profs = true;
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
        } else if( action == "RESET_FILTER" ) {
            if( !filterstring.empty() ) {
                filterstring.clear();
                recalc_profs = true;
            }
        } else if( action == "RANDOMIZE" ) {
            cur_id = rng( 0, profs_length - 1 );
        }
        if( cur_id != id_for_curr_description || recalc_profs ) {
            details_recalc = true;
        }

    } while( true );
}

static std::string assemble_hobby_details( const avatar &u, const input_context &ctxt,
        const std::vector<string_id<profession>> &sorted_hobbies, const int cur_id,
        const std::string &notes )
{
    std::string assembled;

    std::string hobby_name = sorted_hobbies[cur_id]->gender_appropriate_name( u.male );
    if( get_option<bool>( "SCREEN_READER_MODE" ) && !notes.empty() ) {
        hobby_name = hobby_name.append( string_format( " - %s", notes ) );
    }

    assembled += string_format( g_switch_msg( u ), ctxt.get_desc( "CHANGE_GENDER" ),
                                hobby_name ) + "\n";
    assembled += string_format( dress_switch_msg(), ctxt.get_desc( "CHANGE_OUTFIT" ) ) + "\n";

    assembled += "\n" + colorize( _( "Background story:" ), COL_HEADER ) + "\n";
    assembled += colorize( sorted_hobbies[cur_id]->description( u.male ), c_green ) + "\n";

    // Background addictions
    const auto prof_addictions = sorted_hobbies[cur_id]->addictions();
    if( !prof_addictions.empty() ) {
        assembled += "\n" + colorize( _( "Background addictions:" ), COL_HEADER ) + "\n";
        for( const addiction &a : prof_addictions ) {
            const char *format = pgettext( "set_profession_addictions", "%1$s (%2$d)" );
            assembled += string_format( format, a.type->get_name().translated(), a.intensity ) + "\n";
        }
    }

    // Background traits
    const auto prof_traits = sorted_hobbies[cur_id]->get_locked_traits();
    assembled += "\n" + colorize( _( "Background traits:" ), COL_HEADER ) + "\n";
    if( prof_traits.empty() ) {
        assembled += pgettext( "set_profession_trait", "None" ) + std::string( "\n" );
    } else {
        for( const trait_and_var &t : prof_traits ) {
            assembled += t.name() + "\n";
        }
    }

    // Background skills
    const profession::StartingSkillList prof_skills = sorted_hobbies[cur_id]->skills();
    assembled += "\n" + colorize( _( "Background skill experience:" ), COL_HEADER ) + "\n";
    if( prof_skills.empty() ) {
        assembled += pgettext( "set_profession_skill", "None" ) + std::string( "\n" );
    } else {
        for( const auto &sl : prof_skills ) {
            const Skill &skill = sl.first.obj();
            const int level = sl.second;
            if( level < 1 ) {
                debugmsg( "Unexpected skill level for %s: %d", skill.ident().str(), level );
                continue;
            }
            std::string skill_degree;
            if( level == 1 ) {
                skill_degree = pgettext( "set_profession_skill", "beginner" );
            } else if( level == 2 ) {
                skill_degree = pgettext( "set_profession_skill", "intermediate" );
            } else if( level == 3 ) {
                skill_degree = pgettext( "set_profession_skill", "competent" );
            } else {
                skill_degree = pgettext( "set_profession_skill", "advanced" );
            }
            assembled += string_format( "%s (%s)", skill.name(), skill_degree ) + "\n";
        }
    }

    // Background Proficiencies
    std::vector<proficiency_id> prof_proficiencies = sorted_hobbies[cur_id]->proficiencies();
    if( !prof_proficiencies.empty() ) {
        assembled += "\n" + colorize( _( "Background proficiencies:" ), COL_HEADER ) + "\n";
        for( const proficiency_id &prof : prof_proficiencies ) {
            assembled += prof->name() + ": " + colorize( prof->description(), COL_HEADER ) + "\n";
        }
    }

    auto prof_CBMs = sorted_hobbies[cur_id]->CBMs();
    if( !prof_CBMs.empty() ) {
        assembled += "\n" + colorize( _( "Background bionics:" ), COL_HEADER ) + "\n";
        std::sort( std::begin( prof_CBMs ), std::end( prof_CBMs ), []( const bionic_id & a,
        const bionic_id & b ) {
            return a->activated && !b->activated;
        } );
        for( const auto &b : prof_CBMs ) {
            const bionic_data &cbm = b.obj();
            if( cbm.activated && cbm.has_flag( json_flag_BIONIC_TOGGLED ) ) {
                assembled += string_format( _( "%s (toggled)" ), cbm.name ) + "\n";
            } else if( cbm.activated ) {
                assembled += string_format( _( "%s (activated)" ), cbm.name ) + "\n";
            } else {
                assembled += cbm.name + "\n";
            }
        }
    }

    // Background spells
    if( !sorted_hobbies[cur_id]->spells().empty() ) {
        assembled += "\n" + colorize( _( "Background spells:" ), COL_HEADER ) + "\n";
        for( const std::pair<const spell_id, int> &spell_pair : sorted_hobbies[cur_id]->spells() ) {
            assembled += string_format( _( "%s level %d" ), spell_pair.first->name, spell_pair.second ) + "\n";
        }
    }

    // Background missions
    if( !sorted_hobbies[cur_id]->missions().empty() ) {
        assembled += "\n" + colorize( _( "Background missions:" ), COL_HEADER ) + "\n";
        for( mission_type_id mission_id : sorted_hobbies[cur_id]->missions() ) {
            assembled += mission_type::get( mission_id )->tname() + "\n";
        }
    }

    return assembled;
}

/** Handle the hobbies tab of the character generation menu */
void set_hobbies( tab_manager &tabs, avatar &u, pool_type pool )
{
    int cur_id = 0;
    size_t iContentHeight = 0;
    int iStartPos = 0;

    const bool screen_reader_mode = get_option<bool>( "SCREEN_READER_MODE" );

    ui_adaptor ui;
    catacurses::window w;
    catacurses::window w_details_pane;
    scrolling_text_view details( w_details_pane );
    bool details_recalc = true;
    const int iHeaderHeight = 6;
    scrollbar list_sb;

    const auto init_windows = [&]( ui_adaptor & ui ) {
        iContentHeight = TERMY - iHeaderHeight - 1;
        w = catacurses::newwin( TERMY, TERMX, point::zero );
        w_details_pane = catacurses::newwin( iContentHeight, TERMX / 2 - 1, point( TERMX / 2,
                                             iHeaderHeight ) );
        details_recalc = true;
        ui.position_from_window( w );
    };
    init_windows( ui );
    ui.on_screen_resize( init_windows );

    input_context ctxt( "NEW_CHAR_PROFESSIONS" );
    tabs.set_up_tab_navigation( ctxt );
    details.set_up_navigation( ctxt, scrolling_key_scheme::angle_bracket_scroll );
    list_sb.set_draggable( ctxt );
    ctxt.register_navigate_ui_list();
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "CHANGE_GENDER" );
    ctxt.register_action( "CHANGE_OUTFIT" );
    ctxt.register_action( "SORT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "RESET_FILTER" );
    ctxt.register_action( "RANDOMIZE" );

    bool recalc_hobbies = true;
    size_t hobbies_length = 0;
    std::string filterstring;
    std::vector<string_id<profession>> sorted_hobbies;

    ui.on_redraw( [&]( ui_adaptor & ui ) {
        werase( w );
        tabs.draw( w );
        mvwputch( w, point( TERMX / 2, iHeaderHeight - 1 ), BORDER_COLOR, LINE_OXXX );  // '┬'
        mvwputch( w, point( TERMX / 2, TERMY - 1 ), BORDER_COLOR, LINE_XXOX );  // '┴'
        draw_filter_and_sorting_indicators( w, ctxt, filterstring, profession_sorter );

        const bool cur_id_is_valid = cur_id >= 0 && static_cast<size_t>( cur_id ) < sorted_hobbies.size();
        if( cur_id_is_valid ) {
            int netPointCost = sorted_hobbies[cur_id]->point_cost() - u.prof->point_cost();
            ret_val<void> can_pick = sorted_hobbies[cur_id]->can_afford( u, skill_points_left( u, pool ) );
            int pointsForProf = sorted_hobbies[cur_id]->point_cost();
            bool negativeProf = pointsForProf < 0;
            if( negativeProf ) {
                pointsForProf *= -1;
            }

            // Draw header.
            draw_points( w, pool, u, netPointCost );
            if( pool != pool_type::FREEFORM ) {
                const char *prof_msg_temp;
                if( negativeProf ) {
                    //~ 1s - profession name, 2d - current character points.
                    prof_msg_temp = n_gettext( "Background %1$s earns %2$d point",
                                               "Background %1$s earns %2$d points",
                                               pointsForProf );
                } else {
                    //~ 1s - profession name, 2d - current character points.
                    prof_msg_temp = n_gettext( "Background %1$s costs %2$d point",
                                               "Background %1$s costs %2$d points",
                                               pointsForProf );
                }

                int pMsg_length = utf8_width( remove_color_tags( pools_to_string( u, pool ) ) );
                mvwprintz( w, point( pMsg_length + 9, 3 ), can_pick.success() ? c_green : c_light_red,
                           prof_msg_temp, sorted_hobbies[cur_id]->gender_appropriate_name( u.male ), pointsForProf );
            }
        }

        //Draw options
        calcStartPos( iStartPos, cur_id, iContentHeight, hobbies_length );
        const int end_pos = iStartPos + std::min( { iContentHeight, hobbies_length, sorted_hobbies.size() } );
        std::string cur_hob_notes;
        for( int i = iStartPos; i < end_pos; i++ ) {
            nc_color col;
            if( u.hobbies.count( &sorted_hobbies[i].obj() ) != 0 ) {
                col = ( cur_id_is_valid &&
                        sorted_hobbies[i] == sorted_hobbies[cur_id] ? hilite( c_light_green ) : COL_SKILL_USED );
                if( i == cur_id ) {
                    cur_hob_notes = _( "active" );
                }
            } else {
                col = ( cur_id_is_valid &&
                        sorted_hobbies[i] == sorted_hobbies[cur_id] ? COL_SELECT : c_light_gray );
            }

            const point opt_pos( 2, iHeaderHeight + i - iStartPos );
            if( screen_reader_mode ) {
                // This list only clutters up the screen in screen reader mode
            } else {
                mvwprintz( w, opt_pos, col,
                           sorted_hobbies[i]->gender_appropriate_name( u.male ) );
            }
        }

        if( details_recalc && cur_id_is_valid ) {
            details.set_text( assemble_hobby_details( u, ctxt, sorted_hobbies, cur_id, cur_hob_notes ) );
            details_recalc = false;
        }


        list_sb.offset_x( 0 )
        .offset_y( 6 )
        .content_size( hobbies_length )
        .viewport_pos( iStartPos )
        .viewport_size( iContentHeight )
        .apply( w );

        wnoutrefresh( w );
        ui.set_cursor( w_details_pane, point::zero );
        details.draw( c_light_gray );
    } );

    do {
        if( recalc_hobbies ) {
            std::vector<profession_id> new_hobbies = get_scenario()->permitted_hobbies();
            new_hobbies.erase( std::remove_if( new_hobbies.begin(), new_hobbies.end(),
            [&u]( const string_id<profession> &hobby ) {
                return !u.prof->allows_hobby( hobby );
            } ), new_hobbies.end() );
            if( new_hobbies.empty() ) {
                debugmsg( "Why would you blacklist all hobbies?" );
                new_hobbies = profession::get_all_hobbies();
            }
            profession_sorter.male = u.male;
            if( ( hobbies_length = filter_entries( u, cur_id, sorted_hobbies, new_hobbies,
                                                   u.hobbies.empty() ? string_id<profession>() : ( *u.hobbies.begin() )->ident(), filterstring,
                                                   profession_sorter ) ) == 0 ) {
                filterstring.clear();
                continue;
            }
            recalc_hobbies = false;
        }

        ui_manager::redraw();
        const int id_for_curr_description = cur_id;
        const std::string action = ctxt.handle_input();
        const int recmax = hobbies_length;
        const int scroll_rate = recmax > 20 ? 10 : 2;
        int scrollbar_pos = iStartPos;

        if( tabs.handle_input( action, ctxt ) ) {
            break; // Tab has changed or user has quit the screen
        } else if( details.handle_navigation( action, ctxt )
                   || navigate_ui_list( action, cur_id, scroll_rate, recmax, true ) ) {
            // NO FURTHER ACTION REQUIRED
        } else if( list_sb.handle_dragging( action, ctxt.get_coordinates_text( catacurses::stdscr ),
                                            scrollbar_pos ) ) {
            if( scrollbar_pos != iStartPos ) {
                iStartPos = scrollbar_pos;
                cur_id = iStartPos + ( iContentHeight - 1 ) / 2;
            }
        } else if( action == "CONFIRM" ) {
            // Do not allow selection of hobby if there's a trait conflict
            const profession *hobb = &sorted_hobbies[cur_id].obj();
            bool conflict_found = false;
            for( const trait_and_var &new_trait : hobb->get_locked_traits() ) {
                if( u.has_conflicting_trait( new_trait.trait ) ) {
                    for( const profession *hobby : u.hobbies ) {
                        for( const trait_and_var &suspect : hobby->get_locked_traits() ) {
                            if( are_conflicting_traits( new_trait.trait, suspect.trait ) ) {
                                conflict_found = true;
                                popup( _( "The trait [%1$s] conflicts with background [%2$s]'s trait [%3$s]." ), new_trait.name(),
                                       hobby->gender_appropriate_name( u.male ), suspect.name() );
                            }
                        }
                    }
                }
            }
            if( conflict_found ) {
                continue;
            }

            // Toggle hobby
            bool enabling = false;
            if( u.hobbies.count( hobb ) == 0 ) {
                // Add hobby, and decrement point cost
                u.hobbies.insert( hobb );
                enabling = true;
            } else {
                // Remove hobby and refund point cost
                u.hobbies.erase( hobb );
            }

            // Selecting a hobby will, under certain circumstances, change the detail text
            details_recalc = true;

            // Add or remove traits from hobby
            for( const trait_and_var &cur : hobb->get_locked_traits() ) {
                const trait_id &trait = cur.trait;
                if( enabling ) {
                    if( !u.has_trait( trait ) ) {
                        u.toggle_trait_deps( trait );
                    }
                    continue;
                }
                int from_other_hobbies = u.prof->is_locked_trait( trait ) ? 1 : 0;
                for( const profession *hby : u.hobbies ) {
                    if( hby->ident() != hobb->ident() && hby->is_locked_trait( trait ) ) {
                        from_other_hobbies++;
                    }
                }
                if( from_other_hobbies > 0 ) {
                    continue;
                }
                u.toggle_trait_deps( trait );
            }

        } else if( action == "CHANGE_OUTFIT" ) {
            outfit = !outfit;
            recalc_hobbies = true;
        } else if( action == "CHANGE_GENDER" ) {
            u.male = !u.male;
            profession_sorter.male = u.male;
            if( !profession_sorter.sort_by_points ) {
                std::sort( sorted_hobbies.begin(), sorted_hobbies.end(), profession_sorter );
            }
            recalc_hobbies = true;
        } else if( action == "SORT" ) {
            profession_sorter.sort_by_points = !profession_sorter.sort_by_points;
            recalc_hobbies = true;
        } else if( action == "FILTER" ) {
            string_input_popup()
            .title( _( "Search:" ) )
            .width( 60 )
            .description( _( "Search by background name." ) )
            .edit( filterstring );
            recalc_hobbies = true;
        } else if( action == "RESET_FILTER" ) {
            if( !filterstring.empty() ) {
                filterstring.clear();
                recalc_hobbies = true;
            }
        } else if( action == "RANDOMIZE" ) {
            cur_id = rng( 0, hobbies_length - 1 );
        }

        if( cur_id != id_for_curr_description || recalc_hobbies ) {
            details_recalc = true;
        }
    } while( true );
}

/**
 * @return The skill points to consume when a skill is increased (by one level) from the
 * current level.
 *
 * @note: There is one exception: if the current level is 0, it can be boosted by 2 levels for 1 point.
 */
static int skill_increment_cost( const Character &u, const skill_id &skill )
{
    return std::max( 1, ( static_cast<int>( u.get_skill_level( skill ) ) + 1 ) / 2 );
}

static std::string assemble_skill_details( const avatar &u,
        const std::map<skill_id, int> &prof_skills, const Skill *currentSkill )
{
    std::string assembled;
    // We want recipes from profession skills displayed, but
    // without boosting the skills. Copy the skills, and boost the copy
    SkillLevelMap with_prof_skills = u.get_all_skills();
    for( const auto &sk : prof_skills ) {
        with_prof_skills.mod_skill_level( sk.first, sk.second );
    }

    std::map<std::string, std::vector<std::pair<std::string, int> > > recipes;
    for( const auto &e : recipe_dict ) {
        const recipe &r = e.second;
        if( r.is_practice() || r.has_flag( "SECRET" ) ) {
            continue;
        }
        //Find out if the current skill and its level is in the requirement list
        auto req_skill = r.required_skills.find( currentSkill->ident() );
        int skill = req_skill != r.required_skills.end() ? req_skill->second : 0;
        bool would_autolearn_recipe =
            recipe_dict.all_autolearn().count( &r ) &&
            with_prof_skills.meets_skill_requirements( r.autolearn_requirements );

        if( !would_autolearn_recipe && !r.never_learn &&
            ( r.skill_used == currentSkill->ident() || skill > 0 ) &&
            with_prof_skills.has_recipe_requirements( r ) ) {

            recipes[r.skill_used->name()].emplace_back( r.result_name( /*decorated=*/true ),
                    ( skill > 0 ) ? skill : r.difficulty );
        }
        // TODO: Find out why kevlar gambeson hood disppears when going from tailoring 7->8
    }

    for( auto &elem : recipes ) {
        std::sort( elem.second.begin(), elem.second.end(),
                   []( const std::pair<std::string, int> &lhs,
        const std::pair<std::string, int> &rhs ) {
            return localized_compare( std::make_pair( lhs.second, lhs.first ),
                                      std::make_pair( rhs.second, rhs.first ) );
        } );

        const std::string rec_temp = enumerate_as_string( elem.second.begin(), elem.second.end(),
        []( const std::pair<std::string, int> &rec ) {
            return string_format( "%s (%d)", rec.first, rec.second );
        } );

        if( elem.first == currentSkill->name() ) {
            assembled = "\n\n" + colorize( rec_temp, c_brown ) + std::move( assembled );
        } else {
            assembled += "\n\n" + colorize( "[" + elem.first + "]\n" + rec_temp, c_light_gray );
        }
    }

    assembled = currentSkill->description() + assembled;
    return assembled;
}

static std::string assemble_skill_help( const input_context &ctxt )
{
    return string_format(
               _( "Press <color_light_green>%s</color> to view and alter keybindings.\n"
                  "Press <color_light_green>%s</color> / <color_light_green>%s</color> to select skill.\n"
                  "Press <color_light_green>%s</color> to increase skill or "
                  "<color_light_green>%s</color> to decrease skill.\n"
                  "Press <color_light_green>%s</color> to go to the next tab or "
                  "<color_light_green>%s</color> to return to the previous tab." ),
               ctxt.get_desc( "HELP_KEYBINDINGS" ), ctxt.get_desc( "UP" ), ctxt.get_desc( "DOWN" ),
               ctxt.get_desc( "RIGHT" ), ctxt.get_desc( "LEFT" ),
               ctxt.get_desc( "NEXT_TAB" ), ctxt.get_desc( "PREV_TAB" ) );
}

void set_skills( tab_manager &tabs, avatar &u, pool_type pool )
{
    const bool screen_reader_mode = get_option<bool>( "SCREEN_READER_MODE" );
    ui_adaptor ui;
    catacurses::window w;
    catacurses::window w_list;
    catacurses::window w_details_pane;
    scrolling_text_view details( w_details_pane );
    bool details_recalc = true;
    const int iSecondColumn = 31;
    int iContentHeight = 0;
    const int iHeaderHeight = 6;
    // guessing most likely, but it doesn't matter, it will be recalculated if wrong
    int iHelpHeight = 4;
    scrollbar list_sb;
    input_context ctxt( "NEW_CHAR_SKILLS" );
    details.set_up_navigation( ctxt, scrolling_key_scheme::angle_bracket_scroll );
    list_sb.set_draggable( ctxt );
    ctxt.register_cardinal();
    ctxt.register_action( "PAGE_UP", to_translation( "Fast scroll up" ) );
    ctxt.register_action( "PAGE_DOWN", to_translation( "Fast scroll down" ) );
    ctxt.register_action( "HOME" );
    ctxt.register_action( "END" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );

    const auto init_windows = [&]( ui_adaptor & ui ) {
        iContentHeight = TERMY - iHelpHeight - iHeaderHeight - 1;
        w = catacurses::newwin( TERMY, TERMX, point::zero );
        w_list = catacurses::newwin( iContentHeight, iSecondColumn - 1, point( 1, iHeaderHeight ) );
        w_details_pane = catacurses::newwin( iContentHeight, TERMX - iSecondColumn - 1,
                                             point( iSecondColumn, iHeaderHeight ) );
        details_recalc = true;
        ui.position_from_window( w );
    };
    init_windows( ui );
    ui.on_screen_resize( init_windows );

    std::vector<const Skill *> sorted_skills = Skill::get_skills_sorted_by(
    []( const Skill & a, const Skill & b ) {
        return localized_compare( a.name(), b.name() );
    } );

    std::stable_sort( sorted_skills.begin(), sorted_skills.end(),
    []( const Skill * a, const Skill * b ) {
        return a->display_category() < b->display_category();
    } );

    std::vector<HeaderSkill> skill_list = get_HeaderSkills( sorted_skills );

    const int num_skills = skill_list.size();
    int cur_offset = 1;
    int cur_pos = 0;
    const Skill *currentSkill = nullptr;

    const int scroll_rate = num_skills > 20 ? 5 : 2;
    auto get_next = [&]( bool go_up ) {
        skip_skill_headers( skill_list, cur_pos, !go_up );
        currentSkill = skill_list[cur_pos].skill;
    };

    get_next( false );

    std::map<skill_id, int> prof_skills;
    const profession::StartingSkillList &pskills = u.prof->skills();

    std::copy( pskills.begin(), pskills.end(),
               std::inserter( prof_skills, prof_skills.begin() ) );

    const int remaining_points_length = utf8_width( pools_to_string( u, pool ), true );

    ui.on_redraw( [&]( ui_adaptor & ui ) {
        std::string cur_skill_text;
        const std::string help_text = assemble_skill_help( ctxt );
        const int new_iHelpHeight = foldstring( help_text, getmaxx( w ) - 4 ).size();
        if( new_iHelpHeight != iHelpHeight ) {
            iHelpHeight = new_iHelpHeight;
            init_windows( ui );
        }
        werase( w );
        werase( w_list );
        tabs.draw( w );
        mvwputch( w, point( iSecondColumn, iHeaderHeight - 1 ), BORDER_COLOR, LINE_OXXX );  // '┬'
        draw_points( w, pool, u );
        // Helptext skill tab
        fold_and_print( w, point( 2, TERMY - iHelpHeight - 1 ), getmaxx( w ) - 4, COL_NOTE_MINOR,
                        help_text );

        // Write the hint as to upgrade costs
        const int cost = skill_increment_cost( u, currentSkill->ident() );
        const int level = u.get_skill_level( currentSkill->ident() );
        if( pool != pool_type::FREEFORM ) {
            // in pool the first level of a skill gives 2
            const int upgrade_levels = level == 0 ? 2 : 1;
            // We have two different strings to pluralize, so we have to use two translation calls.
            const std::string upgrade_levels_s = string_format(
                    //~ levels here are skill levels at character creation time
                    n_gettext( "%d level", "%d levels", upgrade_levels ), upgrade_levels );
            const nc_color color = skill_points_left( u, pool ) >= cost ? COL_SKILL_USED : c_light_red;
            mvwprintz( w, point( remaining_points_length + 9, 3 ), color,
                       //~ Second string is e.g. "1 level" or "2 levels"
                       n_gettext( "Upgrading %s by %s costs %d point",
                                  "Upgrading %s by %s costs %d points", cost ),
                       currentSkill->name(), upgrade_levels_s, cost );
        }

        calcStartPos( cur_offset, cur_pos, iContentHeight, num_skills );
        for( int i = cur_offset; i < num_skills && i - cur_offset < iContentHeight; ++i ) {
            const int y = i - cur_offset;
            const Skill *thisSkill = skill_list[i].skill;
            int prof_skill_level = 0;
            if( !skill_list[i].is_header ) {
                for( auto &prof_skill : u.prof->skills() ) {
                    if( prof_skill.first == thisSkill->ident() ) {
                        prof_skill_level += prof_skill.second;
                        break;
                    }
                }
            }
            const point opt_pos( 1, y );
            std::string skill_text;
            if( skill_list[i].is_header ) {
                skill_text = colorize( thisSkill->display_category()->display_string(), c_yellow );
            } else if( static_cast<int>( u.get_skill_level( thisSkill->ident() ) ) + prof_skill_level == 0 ) {
                skill_text = colorize( thisSkill->name(), ( i == cur_pos ? COL_SELECT : c_light_gray ) );
            } else {
                skill_text = colorize( thisSkill->name(),
                                       ( i == cur_pos ? hilite( COL_SKILL_USED ) : COL_SKILL_USED ) );
                if( prof_skill_level > 0 ) {
                    skill_text.append( colorize( string_format( " ( %d + %d )", prof_skill_level,
                                                 static_cast<int>( u.get_skill_level( thisSkill->ident() ) ) ),
                                                 ( i == cur_pos ? hilite( COL_SKILL_USED ) : COL_SKILL_USED ) ) );
                } else {
                    skill_text.append( colorize( string_format( " ( %d )",
                                                 static_cast<int>( u.get_skill_level( thisSkill->ident() ) ) ),
                                                 ( i == cur_pos ? hilite( COL_SKILL_USED ) : COL_SKILL_USED ) ) );
                }
            }
            if( i == cur_pos ) {
                cur_skill_text = skill_text;
            }
            if( screen_reader_mode ) {
                // This list only clutters up the screen in screen reader mode
            } else {
                nc_color dummy = c_light_gray;
                print_colored_text( w_list, opt_pos, dummy, c_light_gray, skill_text );
            }
        }

        if( details_recalc ) {
            std::string description;
            if( screen_reader_mode ) {
                description = currentSkill->display_category()->display_string() + " - ";
                description.append( cur_skill_text + "\n" );
                description.append( assemble_skill_details( u, prof_skills, currentSkill ) );
            } else {
                description = assemble_skill_details( u, prof_skills, currentSkill );
            }
            details.set_text( description );
            details_recalc = false;
        }

        list_sb.offset_x( 0 )
        .offset_y( iHeaderHeight )
        .content_size( num_skills )
        .viewport_pos( cur_offset )
        .viewport_size( iContentHeight )
        .apply( w );

        wnoutrefresh( w );
        wnoutrefresh( w_list );
        ui.set_cursor( w_details_pane, point::zero );
        details.draw( c_light_gray );
    } );

    do {
        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        const int pos_for_curr_description = cur_pos;
        int scrollbar_pos = cur_offset;

        if( tabs.handle_input( action, ctxt ) ) {
            break; // Tab has changed or user has quit the screen
        } else if( details.handle_navigation( action, ctxt ) ) {
            // NO FURTHER ACTION REQUIRED
        } else if( list_sb.handle_dragging( action, ctxt.get_coordinates_text( catacurses::stdscr ),
                                            scrollbar_pos ) ) {
            if( scrollbar_pos != cur_offset ) {
                cur_offset = scrollbar_pos;
                cur_pos = cur_offset + ( iContentHeight - 1 ) / 2; // Get approximate location
                get_next( false ); // Then make sure it's a skill rather than a heading
                if( cur_pos < num_skills / 2 ) {
                    get_next( true ); // Go back to where we were to ensure we can drag to the top
                }
            }
        } else if( navigate_ui_list( action, cur_pos, scroll_rate, skill_list.size(), true ) ) {
            // omitting action == "PAGE_UP", because that shouldn't wrap
            const bool go_up = action == "UP" || action == "SCROLL_UP";
            get_next( go_up );
        } else if( action == "LEFT" ) {
            const skill_id &skill_id = currentSkill->ident();
            const int level = u.get_skill_level( skill_id );
            if( level > 0 ) {
                // For balance reasons, increasing a skill from level 0 gives 1 extra level for free, but
                // decreasing it from level 2 forfeits the free extra level (thus changes it to 0)
                // this only matters in legacy character creation modes
                u.mod_skill_level( skill_id, level == 2 && pool != pool_type::FREEFORM ? -2 : -1 );
                u.set_knowledge_level( skill_id, static_cast<int>( u.get_skill_level( skill_id ) ) );
            }
            details_recalc = true;
        } else if( action == "RIGHT" ) {
            const skill_id &skill_id = currentSkill->ident();
            const int level = u.get_skill_level( skill_id );
            if( level < MAX_SKILL ) {
                // For balance reasons, increasing a skill from level 0 gives 1 extra level for free
                // this only matters in legacy character creation modes
                u.mod_skill_level( skill_id, level == 0 && pool != pool_type::FREEFORM ? +2 : +1 );
                u.set_knowledge_level( skill_id, static_cast<int>( u.get_skill_level( skill_id ) ) );
            }
            details_recalc = true;
        }
        if( cur_pos != pos_for_curr_description ) {
            details_recalc = true;
        }
    } while( true );
}

static struct {
    bool sort_by_points = true;
    bool male = false;
    bool cities_enabled = false;
    /** @related player */
    bool operator()( const scenario *a, const scenario *b ) const {
        if( cities_enabled ) {
            // The generic ("Unemployed") profession should be listed first.
            const scenario *gen = scenario::generic();
            if( b == gen ) {
                return false;
            } else if( a == gen ) {
                return true;
            }
        }

        if( !a->can_pick().success() && b->can_pick().success() ) {
            return false;
        }

        if( a->can_pick().success() && !b->can_pick().success() ) {
            return true;
        }

        if( !cities_enabled && a->has_flag( "CITY_START" ) != b->has_flag( "CITY_START" ) ) {
            return a->has_flag( "CITY_START" ) < b->has_flag( "CITY_START" );
        } else if( sort_by_points ) {
            return a->point_cost() < b->point_cost();
        } else {
            return localized_compare( a->gender_appropriate_name( male ),
                                      b->gender_appropriate_name( male ) );
        }
    }
} scenario_sorter;

static std::string assemble_scenario_details( const avatar &u, const input_context &ctxt,
        const scenario *current_scenario, const std::string &notes )
{
    std::string assembled = get_origin( current_scenario->src ) + "\n";

    std::string scenario_name = current_scenario->gender_appropriate_name( !u.male );
    if( get_option<bool>( "SCREEN_READER_MODE" ) && !notes.empty() ) {
        scenario_name = scenario_name.append( string_format( " - %s", notes ) );
    }
    assembled += string_format( g_switch_msg( u ), ctxt.get_desc( "CHANGE_GENDER" ),
                                scenario_name ) + "\n";
    assembled += string_format( dress_switch_msg(), ctxt.get_desc( "CHANGE_OUTFIT" ) ) + "\n";

    assembled += string_format(
                     _( "Press <color_light_green>%1$s</color> to change cataclysm start date, <color_light_green>%2$s</color> to change game start date, <color_light_green>%3$s</color> to reset calendar." ),
                     ctxt.get_desc( "CHANGE_START_OF_CATACLYSM" ), ctxt.get_desc( "CHANGE_START_OF_GAME" ),
                     ctxt.get_desc( "RESET_CALENDAR" ) ) + "\n";
    assembled += "\n" + colorize( _( "Scenario Story:" ), COL_HEADER ) + "\n";
    assembled += colorize( current_scenario->description( u.male ), c_green ) + "\n";
    const std::optional<achievement_id> scenRequirement = current_scenario->get_requirement();

    if( scenRequirement.has_value() ||
        ( current_scenario->has_flag( "CITY_START" ) && !scenario_sorter.cities_enabled ) ) {
        assembled += "\n" + colorize( _( "Scenario Requirements:" ), COL_HEADER ) + "\n";
        if( current_scenario->has_flag( "CITY_START" ) && !scenario_sorter.cities_enabled ) {
            const std::string scenUnavailable =
                _( "This scenario is not available in this world due to city size settings." );
            assembled += colorize( scenUnavailable, c_red ) + "\n";
        }
        if( scenRequirement.has_value() ) {
            nc_color requirement_color = c_red;
            if( current_scenario->can_pick().success() ) {
                requirement_color = c_green;
            }
            assembled += colorize( string_format( _( "Complete \"%s\"" ), scenRequirement.value()->name() ),
                                   requirement_color ) + "\n";
        }
    }

    assembled += "\n" + colorize( _( "Scenario Professions:" ), COL_HEADER );
    assembled += string_format( _( "\n%s" ), current_scenario->prof_count_str() );
    assembled += _( ", default:\n" );

    auto psorter = profession_sorter;
    psorter.sort_by_points = true;
    const auto permitted = current_scenario->permitted_professions();
    const auto default_prof = *std::min_element( permitted.begin(), permitted.end(), psorter );
    const int prof_points = default_prof->point_cost();

    assembled += default_prof->gender_appropriate_name( u.male );
    if( prof_points > 0 ) {
        assembled += colorize( string_format( " (-%d)", prof_points ), c_red );
    } else if( prof_points < 0 ) {
        assembled += colorize( string_format( " (+%d)", -prof_points ), c_green );
    }
    assembled += "\n";

    assembled += "\n" + colorize( _( "Scenario Location:" ), COL_HEADER ) + "\n";
    assembled += string_format( _( "%s (%d locations, %d variants)" ),
                                current_scenario->start_name(),
                                current_scenario->start_location_count(),
                                current_scenario->start_location_targets_count() ) + "\n";

    if( current_scenario->vehicle() ) {
        assembled += "\n" + colorize( _( "Scenario Vehicle:" ), COL_HEADER ) + "\n";
        assembled += current_scenario->vehicle()->name + "\n";
    }

    assembled += "\n" + colorize( _( "Start of cataclysm:" ), COL_HEADER ) + "\n";
    assembled += to_string( current_scenario->start_of_cataclysm() ) + "\n";

    assembled += "\n" + colorize( _( "Start of game:" ), COL_HEADER ) + "\n";
    assembled += to_string( current_scenario->start_of_game() ) + "\n";

    if( !current_scenario->missions().empty() ) {
        assembled += "\n" + colorize( _( "Scenario missions:" ), COL_HEADER ) + "\n";
        for( mission_type_id mission_id : current_scenario->missions() ) {
            assembled += mission_type::get( mission_id )->tname() + "\n";
        }
    }

    //TODO: Move this to JSON?
    const std::vector<std::pair<std::string, std::string>> flag_descriptions = {
        { "FIRE_START", translate_marker( "Fire nearby" ) },
        { "SUR_START", translate_marker( "Zombies nearby" ) },
        { "HELI_CRASH", translate_marker( "Various limb wounds" ) },
        { "LONE_START", translate_marker( "No starting NPC" ) },
        { "BORDERED", translate_marker( "Starting location is bordered by an immense wall" ) },
    };

    bool flag_header_added = false;
    for( const std::pair<std::string, std::string> &flag_pair : flag_descriptions ) {
        if( current_scenario->has_flag( std::get<0>( flag_pair ) ) ) {
            if( !flag_header_added ) {
                assembled += "\n" + colorize( _( "Scenario Flags:" ), COL_HEADER ) + "\n";
                flag_header_added = true;
            }
            assembled += _( std::get<1>( flag_pair ) ) + "\n";
        }
    }

    return assembled;
}

void set_scenario( tab_manager &tabs, avatar &u, pool_type pool )
{
    int cur_id = 0;
    size_t iContentHeight = 0;
    int iStartPos = 0;

    ui_adaptor ui;
    catacurses::window w;
    catacurses::window w_details_pane;
    scrolling_text_view details( w_details_pane );
    bool details_recalc = true;
    const int iHeaderHeight = 6;
    scrollbar list_sb;

    const bool screen_reader_mode = get_option<bool>( "SCREEN_READER_MODE" );

    const auto init_windows = [&]( ui_adaptor & ui ) {
        iContentHeight = TERMY - iHeaderHeight - 1;
        w = catacurses::newwin( TERMY, TERMX, point::zero );
        const int second_column_w = TERMX / 2 - 1;
        point origin = point( second_column_w + 1, iHeaderHeight );
        w_details_pane = catacurses::newwin( iContentHeight, second_column_w, origin );
        details_recalc = true;
        ui.position_from_window( w );
    };
    init_windows( ui );
    ui.on_screen_resize( init_windows );

    input_context ctxt( "NEW_CHAR_SCENARIOS" );
    tabs.set_up_tab_navigation( ctxt );
    details.set_up_navigation( ctxt, scrolling_key_scheme::angle_bracket_scroll );
    list_sb.set_draggable( ctxt );
    ctxt.register_navigate_ui_list();
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "SORT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "CHANGE_GENDER" );
    ctxt.register_action( "CHANGE_OUTFIT" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "RESET_FILTER" );
    ctxt.register_action( "CHANGE_START_OF_CATACLYSM" );
    ctxt.register_action( "CHANGE_START_OF_GAME" );
    ctxt.register_action( "RESET_CALENDAR" );

    bool recalc_scens = true;
    size_t scens_length = 0;
    std::string filterstring;
    std::vector<const scenario *> sorted_scens;

    ui.on_redraw( [&]( ui_adaptor & ui ) {
        werase( w );
        tabs.draw( w );
        mvwputch( w, point( TERMX / 2, iHeaderHeight - 1 ), BORDER_COLOR, LINE_OXXX );  // '┬'
        mvwputch( w, point( TERMX / 2, TERMY - 1 ), BORDER_COLOR, LINE_XXOX );  // '┴'
        draw_filter_and_sorting_indicators( w, ctxt, filterstring, scenario_sorter );

        const bool cur_id_is_valid = cur_id >= 0 && static_cast<size_t>( cur_id ) < sorted_scens.size();
        if( cur_id_is_valid ) {
            int netPointCost = sorted_scens[cur_id]->point_cost() - get_scenario()->point_cost();
            ret_val<void> can_afford = sorted_scens[cur_id]->can_afford(
                                           *get_scenario(),
                                           skill_points_left( u, pool ) );
            ret_val<void> can_pick = sorted_scens[cur_id]->can_pick();

            int pointsForScen = sorted_scens[cur_id]->point_cost();
            bool negativeScen = pointsForScen < 0;
            if( negativeScen ) {
                pointsForScen *= -1;
            }
            // Draw header.
            draw_points( w, pool, u, netPointCost );
            if( pool != pool_type::FREEFORM ) {

                const char *scen_msg_temp;

                if( negativeScen ) {
                    scen_msg_temp = n_gettext( "Scenario earns %2$d point",
                                               "Scenario earns %2$d points", pointsForScen );
                } else {
                    scen_msg_temp = n_gettext( "Scenario costs %2$d point",
                                               "Scenario costs %2$d points", pointsForScen );
                }

                int pMsg_length = utf8_width( remove_color_tags( pools_to_string( u, pool ) ) );
                mvwprintz( w, point( pMsg_length + 9, 3 ), can_afford.success() ? c_green : c_light_red,
                           scen_msg_temp, sorted_scens[cur_id]->gender_appropriate_name( u.male ), pointsForScen );
            }
        }

        //Draw options
        calcStartPos( iStartPos, cur_id, iContentHeight, scens_length );
        const int end_pos = iStartPos + std::min( { iContentHeight, scens_length, sorted_scens.size() } );
        std::string current_scenario_notes;
        for( int i = iStartPos; i < end_pos; i++ ) {
            nc_color col;
            if( get_scenario() != sorted_scens[i] ) {
                if( cur_id_is_valid && sorted_scens[i] == sorted_scens[cur_id] &&
                    ( ( sorted_scens[i]->has_flag( "CITY_START" ) && !scenario_sorter.cities_enabled ) ||
                      !sorted_scens[i]->can_pick().success() ) ) {
                    col = h_dark_gray;
                    if( i == cur_id ) {
                        current_scenario_notes = _( "unavailable" );
                    }
                } else if( cur_id_is_valid && sorted_scens[i] != sorted_scens[cur_id] &&
                           ( ( sorted_scens[i]->has_flag( "CITY_START" ) && !scenario_sorter.cities_enabled ) ||
                             !sorted_scens[i]->can_pick().success() ) ) {
                    col = c_dark_gray;
                    if( i == cur_id ) {
                        current_scenario_notes = _( "unavailable" );
                    }
                } else {
                    col = ( cur_id_is_valid && sorted_scens[i] == sorted_scens[cur_id] ? COL_SELECT : c_light_gray );
                }
            } else {
                col = ( cur_id_is_valid &&
                        sorted_scens[i] == sorted_scens[cur_id] ? hilite( c_light_green ) : COL_SKILL_USED );
                if( i == cur_id ) {
                    current_scenario_notes = _( "active" );
                }
            }
            const point opt_pos( 2, iHeaderHeight + i - iStartPos );
            if( screen_reader_mode ) {
                // The list of options only clutters up the screen in screen reader mode
            } else {
                mvwprintz( w, opt_pos, col,
                           sorted_scens[i]->gender_appropriate_name( u.male ) );
            }
        }

        if( details_recalc && cur_id_is_valid ) {
            details.set_text( assemble_scenario_details( u, ctxt, sorted_scens[cur_id],
                              current_scenario_notes ) );
            details_recalc = false;
        }

        list_sb.offset_x( 0 )
        .offset_y( 6 )
        .content_size( scens_length )
        .viewport_pos( iStartPos )
        .viewport_size( iContentHeight )
        .apply( w );

        wnoutrefresh( w );
        ui.set_cursor( w_details_pane, point::zero );
        details.draw( c_light_gray );
    } );

    do {
        if( recalc_scens ) {
            options_manager::options_container &wopts = world_generator->active_world->WORLD_OPTIONS;
            std::vector<const scenario *> new_scens;
            for( const scenario &scen : scenario::get_all() ) {
                if( scen.scen_is_blacklisted() ) {
                    continue;
                }
                new_scens.push_back( &scen );
            }
            scenario_sorter.male = u.male;
            scenario_sorter.cities_enabled = wopts["CITY_SIZE"].getValue() != "0";
            if( ( scens_length = filter_entries( u, cur_id, sorted_scens, new_scens, get_scenario(),
                                                 filterstring,
                                                 scenario_sorter ) ) == 0 ) {
                filterstring.clear();
                continue;
            }
            recalc_scens = false;
        }

        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        const int scroll_rate = scens_length > 20 ? 5 : 2;
        const int id_for_curr_description = cur_id;
        int scrollbar_pos = iStartPos;

        if( tabs.handle_input( action, ctxt ) ) {
            break; // Tab has changed or user has quit the screen
        } else if( details.handle_navigation( action, ctxt )
                   || navigate_ui_list( action, cur_id, scroll_rate, scens_length, true ) ) {
            // NO FURTHER ACTION REQUIRED
        } else if( list_sb.handle_dragging( action, ctxt.get_coordinates_text( catacurses::stdscr ),
                                            scrollbar_pos ) ) {
            if( scrollbar_pos != iStartPos ) {
                iStartPos = scrollbar_pos;
                cur_id = iStartPos + ( iContentHeight - 1 ) / 2;
            }
        } else if( action == "CONFIRM" ) {
            // set arbitrarily high points and check if we have the achievment
            ret_val<void> can_pick = sorted_scens[cur_id]->can_pick();

            if( !can_pick.success() ) {
                popup( can_pick.str() );
                continue;
            }

            if( sorted_scens[cur_id]->has_flag( "CITY_START" ) && !scenario_sorter.cities_enabled ) {
                continue;
            }
            reset_scenario( u, sorted_scens[cur_id] );
            details_recalc = true;
        } else if( action == "CHANGE_OUTFIT" ) {
            outfit = !outfit;
            recalc_scens = true;
        } else if( action == "CHANGE_GENDER" ) {
            u.male = !u.male;
            recalc_scens = true;
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
        } else if( action == "RESET_FILTER" ) {
            if( !filterstring.empty() ) {
                filterstring.clear();
                recalc_scens = true;
            }
        } else if( action == "RANDOMIZE" ) {
            cur_id = rng( 0, scens_length - 1 );
        } else if( action == "CHANGE_START_OF_CATACLYSM" ) {
            const scenario *scen = sorted_scens[cur_id];
            if( cur_id != id_for_curr_description ) {
                scen = get_scenario();
            }
            scen->change_start_of_cataclysm( calendar_ui::select_time_point( scen->start_of_cataclysm(),
                                             _( "Select cataclysm start date" ), calendar_ui::granularity::hour ) );
            details_recalc = true;
        } else if( action == "CHANGE_START_OF_GAME" ) {
            const scenario *scen = sorted_scens[cur_id];
            if( cur_id != id_for_curr_description ) {
                scen = get_scenario();
            }
            scen->change_start_of_game( calendar_ui::select_time_point( scen->start_of_game(),
                                        _( "Select game start date" ), calendar_ui::granularity::hour ) );
            details_recalc = true;
        } else if( action == "RESET_CALENDAR" ) {
            const scenario *scen = sorted_scens[cur_id];
            if( cur_id != id_for_curr_description ) {
                get_scenario()->reset_calendar();
            }
            scen->reset_calendar();
            details_recalc = true;
        }

        if( cur_id != id_for_curr_description || recalc_scens ) {
            details_recalc = true;
        }
    } while( true );
}

namespace char_creation
{
enum description_selector {
    NAME,
    GENDER,
    OUTFIT,
    HEIGHT,
    AGE,
    BLOOD,
    LOCATION
};

static void draw_name( ui_adaptor &ui, const catacurses::window &w_name,
                       const avatar &you, const bool highlight,
                       const bool no_name_entered )
{
    werase( w_name );
    mvwprintz( w_name, point::zero,
               highlight ? COL_SELECT : c_light_gray, _( "Name:" ) );
    const point opt_pos( 1 + utf8_width( _( "Name:" ) ), 0 );
    if( highlight ) {
        ui.set_cursor( w_name, opt_pos );
    }
    if( no_name_entered ) {
        mvwprintz( w_name, opt_pos, COL_SELECT, _( "--- NO NAME ENTERED ---" ) );
    } else if( you.name.empty() ) {
        mvwprintz( w_name, opt_pos, c_light_gray, _( "--- RANDOM NAME ---" ) );
    } else {
        mvwprintz( w_name, opt_pos, c_white, you.name );
    }

    wnoutrefresh( w_name );
}

static void draw_gender( ui_adaptor &ui, const catacurses::window &w_gender,
                         const avatar &you, const bool highlight )
{
    const point male_pos( 1 + utf8_width( _( "Gender:" ) ), 0 );
    const point female_pos = male_pos + point( 2 + utf8_width( _( "Male" ) ), 0 );

    werase( w_gender );
    mvwprintz( w_gender, point::zero, highlight ? COL_SELECT : c_light_gray, _( "Gender:" ) );
    if( highlight && you.male ) {
        ui.set_cursor( w_gender, male_pos );
    }
    mvwprintz( w_gender, male_pos, ( you.male ? c_light_cyan : c_light_gray ),
               _( "Male" ) );
    if( highlight && !you.male ) {
        ui.set_cursor( w_gender, female_pos );
    }
    mvwprintz( w_gender, female_pos, ( you.male ? c_light_gray : c_pink ),
               _( "Female" ) );
    wnoutrefresh( w_gender );
}

static void draw_outfit( ui_adaptor &ui, const catacurses::window &w_outfit, const bool highlight )
{
    const point male_pos( 1 + utf8_width( _( "Outfit:" ) ), 0 );
    const point female_pos = male_pos + point( 2 + utf8_width( _( "Male" ) ), 0 );

    werase( w_outfit );
    mvwprintz( w_outfit, point::zero, highlight ? COL_SELECT : c_light_gray, _( "Outfit:" ) );
    if( highlight && outfit ) {
        ui.set_cursor( w_outfit, male_pos );
    }
    mvwprintz( w_outfit, male_pos, ( outfit ? c_light_cyan : c_light_gray ),
               _( "Male" ) );
    if( highlight && !outfit ) {
        ui.set_cursor( w_outfit, female_pos );
    }
    mvwprintz( w_outfit, female_pos, ( outfit ? c_light_gray : c_pink ),
               _( "Female" ) );
    wnoutrefresh( w_outfit );
}

static void draw_height( ui_adaptor &ui, const catacurses::window &w_height,
                         const avatar &you, const bool highlight )
{
    werase( w_height );
    mvwprintz( w_height, point::zero, highlight ? COL_SELECT : c_light_gray, _( "Height:" ) );
    const point opt_pos( 1 + utf8_width( _( "Height:" ) ), 0 );
    if( highlight ) {
        ui.set_cursor( w_height, opt_pos );
    }
    mvwprintz( w_height, opt_pos, c_white, you.height_string() );
    wnoutrefresh( w_height );
}

static void draw_age( ui_adaptor &ui, const catacurses::window &w_age,
                      const avatar &you, const bool highlight )
{
    werase( w_age );
    mvwprintz( w_age, point::zero, highlight ? COL_SELECT : c_light_gray, _( "Age:" ) );
    const point opt_pos( 1 + utf8_width( _( "Age:" ) ), 0 );
    if( highlight ) {
        ui.set_cursor( w_age, opt_pos );
    }
    mvwprintz( w_age, opt_pos, c_white, you.age_string( get_scenario()->start_of_game() ) );
    wnoutrefresh( w_age );
}

static void draw_blood( ui_adaptor &ui, const catacurses::window &w_blood,
                        const avatar &you, const bool highlight )
{
    werase( w_blood );
    mvwprintz( w_blood, point::zero, highlight ? COL_SELECT : c_light_gray, _( "Blood type:" ) );
    const point opt_pos( 1 + utf8_width( _( "Blood type:" ) ), 0 );
    if( highlight ) {
        ui.set_cursor( w_blood, opt_pos );
    }
    mvwprintz( w_blood, opt_pos, c_white,
               io::enum_to_string( you.my_blood_type ) + ( you.blood_rh_factor ? "+" : "-" ) );
    wnoutrefresh( w_blood );
}

static void draw_location( ui_adaptor &ui, const catacurses::window &w_location,
                           const avatar &you, const bool highlight )
{
    std::string random_start_location_text = string_format( n_gettext(
                "<color_red>* Random location *</color> (<color_white>%d</color> variant)",
                "<color_red>* Random location *</color> (<color_white>%d</color> variants)",
                get_scenario()->start_location_targets_count() ), get_scenario()->start_location_targets_count() );

    if( get_scenario()->start_location_targets_count() == 1 ) {
        random_start_location_text = get_scenario()->start_location().obj().name();
    }

    werase( w_location );
    mvwprintz( w_location, point::zero, highlight ? COL_SELECT : c_light_gray,
               _( "Starting location:" ) );
    const point opt_pos( utf8_width( _( "Starting location:" ) ) + 1, 0 );
    if( highlight ) {
        ui.set_cursor( w_location, opt_pos );
    }
    // ::find will return empty location if id was not found. Debug msg will be printed too.
    mvwprintz( w_location, opt_pos,
               you.random_start_location ? c_red : c_white,
               you.random_start_location ? remove_color_tags( random_start_location_text ) :
               string_format( n_gettext( "%s (%d variant)", "%s (%d variants)",
                                         you.start_location.obj().targets_count() ),
                              you.start_location.obj().name(), you.start_location.obj().targets_count() ) );
    wnoutrefresh( w_location );
}

} // namespace char_creation

static std::string assemble_description_help( const input_context &ctxt, const bool allow_reroll )
{
    if( !isWide ) {
        return string_format( _( "Press <color_light_green>%s</color> to view and alter keybindings." ),
                              ctxt.get_desc( "HELP_KEYBINDINGS" ) );
    }
    std::string help_text = string_format(
                                _( "Press <color_light_green>%s</color> to view and alter keybindings." ),
                                ctxt.get_desc( "HELP_KEYBINDINGS" ) )
                            + string_format( _( "\nPress <color_light_green>%s</color> to save character template." ),
                                    ctxt.get_desc( "SAVE_TEMPLATE" ) );
    if( !MAP_SHARING::isSharing() && allow_reroll ) { // no random names when sharing maps
        help_text += string_format(
                         _( "\nPress <color_light_green>%s</color> to pick a random name, "
                            "<color_light_green>%s</color> to randomize all description values, "
                            "<color_light_green>%s</color> to randomize all but scenario, or "
                            "<color_light_green>%s</color> to randomize everything." ),
                         ctxt.get_desc( "RANDOMIZE_CHAR_NAME" ), ctxt.get_desc( "RANDOMIZE_CHAR_DESCRIPTION" ),
                         ctxt.get_desc( "REROLL_CHARACTER" ), ctxt.get_desc( "REROLL_CHARACTER_WITH_SCENARIO" ) );
    } else {
        help_text += string_format(
                         _( "\nPress <color_light_green>%s</color> to pick a random name, "
                            "<color_light_green>%s</color> to randomize all description values." ),
                         ctxt.get_desc( "RANDOMIZE_CHAR_NAME" ), ctxt.get_desc( "RANDOMIZE_CHAR_DESCRIPTION" ) );
    }
    help_text += string_format(
                     _( "\nPress <color_light_green>%1$s</color> to change cataclysm start date, "
                        "<color_light_green>%2$s</color> to change game start date, "
                        "<color_light_green>%3$s</color> to reset calendar." ),
                     ctxt.get_desc( "CHANGE_START_OF_CATACLYSM" ), ctxt.get_desc( "CHANGE_START_OF_GAME" ),
                     ctxt.get_desc( "RESET_CALENDAR" ) );
    if( !get_option<bool>( "SELECT_STARTING_CITY" ) ) {
        help_text += string_format(
                         _( "\nPress <color_light_green>%s</color> to select a specific starting location." ),
                         ctxt.get_desc( "CHOOSE_LOCATION" ) );
    } else {
        help_text += string_format(
                         _( "\nPress <color_light_green>%s</color> to select a specific starting city and "
                            "<color_light_green>%s</color> to select a specific starting location." ),
                         ctxt.get_desc( "CHOOSE_CITY" ), ctxt.get_desc( "CHOOSE_LOCATION" ) );
    }
    help_text += string_format(
                     _( "\nPress <color_light_green>%s</color> or <color_light_green>%s</color> "
                        "to cycle through editable values." ),
                     ctxt.get_desc( "UP" ), ctxt.get_desc( "DOWN" ) )
                 + string_format( _( "\nPress <color_light_green>%s</color> and "
                                     "<color_light_green>%s</color> to change gender, height, age, and blood type." ),
                                  ctxt.get_desc( "LEFT" ), ctxt.get_desc( "RIGHT" ) )
                 + string_format( _( "\nPress <color_light_green>%s</color> to edit value via popup input." ),
                                  ctxt.get_desc( "CONFIRM" ) )
                 + string_format( _( "\nPress <color_light_green>%s</color> to finish character creation "
                                     "or <color_light_green>%s</color> to return to the previous TAB." ),
                                  ctxt.get_desc( "NEXT_TAB" ), ctxt.get_desc( "PREV_TAB" ) );
    return help_text;
}

// NOLINTNEXTLINE(readability-function-size)
void set_description( tab_manager &tabs, avatar &you, const bool allow_reroll,
                      pool_type pool )
{
    static constexpr int RANDOM_START_LOC_ENTRY = INT_MIN;

    // guessing most likely, but it doesn't matter, it will be recalculated if wrong
    int iHelpHeight = 10;

    ui_adaptor ui;
    catacurses::window w;
    catacurses::window w_name;
    catacurses::window w_gender;
    catacurses::window w_outfit;
    catacurses::window w_location;
    catacurses::window w_vehicle;
    catacurses::window w_stats;
    catacurses::window w_traits;
    catacurses::window w_bionics;
    catacurses::window w_proficiencies;
    catacurses::window w_addictions;
    catacurses::window w_scenario;
    catacurses::window w_profession;
    catacurses::window w_hobbies;
    catacurses::window w_skills;
    catacurses::window w_guide;
    catacurses::window w_height;
    catacurses::window w_age;
    catacurses::window w_blood;
    catacurses::window w_calendar;
    const auto init_windows = [&]( ui_adaptor & ui ) {
        const int freeWidth = TERMX - FULL_SCREEN_WIDTH;
        isWide = freeWidth > 15;
        const int beginx2 = 52;
        const int ncol2 = 40;
        const int beginx3 = TERMX <= 88 ? TERMX - TERMX / 4 : 90;
        const int ncol3 = TERMX - beginx3 - 2;
        const int beginx4 = TERMX <= 130 ? TERMX - TERMX / 5 : 128;
        const int ncol4 = TERMX - beginx4 - 2;
        const int ncol_small = ( TERMX / 2 ) - 2;
        const int begin_sncol = TERMX / 2;
        if( isWide ) {
            w = catacurses::newwin( TERMY, TERMX, point::zero );
            w_name = catacurses::newwin( 2, ncol2 + 2, point( 2, 6 ) );
            w_gender = catacurses::newwin( 1, ncol2 + 2, point( 2, 7 ) );
            w_outfit = catacurses::newwin( 1, ncol2 + 1, point( 2, 8 ) );
            w_location = catacurses::newwin( 1, ncol3, point( beginx3, 6 ) );
            w_vehicle = catacurses::newwin( 1, ncol3, point( beginx3, 7 ) );
            w_addictions = catacurses::newwin( 1, ncol3, point( beginx3, 8 ) );
            w_stats = catacurses::newwin( 6, 20, point( 2, 10 ) );
            w_traits = catacurses::newwin( TERMY - 11, ncol2, point( beginx2, 10 ) );
            w_calendar = catacurses::newwin( 4, ncol3, point( beginx3, 10 ) );
            w_bionics = catacurses::newwin( TERMY - 16, ncol3, point( beginx3, 15 ) );
            w_proficiencies = catacurses::newwin( TERMY - 21, 19, point( 2, 16 ) );
            // Extra - 11 to avoid overlap with long text in w_guide.
            w_hobbies = catacurses::newwin( TERMY - 11 - 11, ncol4, point( beginx4, 10 ) );
            w_scenario = catacurses::newwin( 1, ncol2, point( beginx2, 3 ) );
            w_profession = catacurses::newwin( 1, ncol3, point( beginx3, 3 ) );
            w_skills = catacurses::newwin( TERMY - 11, 27, point( 22, 10 ) );
            w_height = catacurses::newwin( 1, ncol2, point( beginx2, 6 ) );
            w_age = catacurses::newwin( 1, ncol2, point( beginx2, 7 ) );
            w_blood = catacurses::newwin( 1, ncol2, point( beginx2, 8 ) );
            ui.position_from_window( w );
        } else {
            w = catacurses::newwin( TERMY, TERMX, point::zero );
            w_name = catacurses::newwin( 1, ncol_small, point( 2, 5 ) );
            w_gender = catacurses::newwin( 1, ncol_small, point( 2, 6 ) );
            w_outfit = catacurses::newwin( 1, ncol_small, point( 2, 7 ) );
            w_height = catacurses::newwin( 1, ncol_small, point( 2, 8 ) );
            w_age = catacurses::newwin( 1, ncol_small, point( begin_sncol, 6 ) );
            w_blood = catacurses::newwin( 1, ncol_small, point( begin_sncol, 7 ) );
            w_location = catacurses::newwin( 1, ncol_small, point( begin_sncol, 8 ) );
            w_stats = catacurses::newwin( 6, ncol_small, point( 2, 10 ) );
            w_scenario = catacurses::newwin( 1, ncol_small, point( begin_sncol, 10 ) );
            w_profession = catacurses::newwin( 1, ncol_small, point( begin_sncol, 11 ) );
            w_calendar = catacurses::newwin( 4, ncol_small, point( begin_sncol, 13 ) );
            w_vehicle = catacurses::newwin( 2, ncol_small, point( begin_sncol, 18 ) );
            w_addictions = catacurses::newwin( 2, ncol_small, point( begin_sncol, 20 ) );
            w_traits = catacurses::window();
            w_bionics = catacurses::window();
            w_proficiencies = catacurses::window();
            w_hobbies = catacurses::window();
            w_skills = catacurses::window();
            ui.position_from_window( w );
        }
        w_guide = catacurses::newwin( iHelpHeight, TERMX - 4, point( 2, TERMY - iHelpHeight - 1 ) );
    };
    init_windows( ui );
    ui.on_screen_resize( init_windows );

    input_context ctxt( "NEW_CHAR_DESCRIPTION" );
    tabs.set_up_tab_navigation( ctxt );
    ctxt.register_cardinal();
    ctxt.register_action( "SAVE_TEMPLATE" );
    ctxt.register_action( "RANDOMIZE_CHAR_NAME" );
    ctxt.register_action( "RANDOMIZE_CHAR_DESCRIPTION" );
    if( !MAP_SHARING::isSharing() && allow_reroll ) {
        ctxt.register_action( "REROLL_CHARACTER" );
        ctxt.register_action( "REROLL_CHARACTER_WITH_SCENARIO" );
    }
    ctxt.register_action( "CHANGE_GENDER" );
    ctxt.register_action( "CHANGE_OUTFIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "CHANGE_START_OF_CATACLYSM" );
    ctxt.register_action( "CHANGE_START_OF_GAME" );
    ctxt.register_action( "RESET_CALENDAR" );
    if( get_option<bool>( "SELECT_STARTING_CITY" ) ) {
        ctxt.register_action( "CHOOSE_CITY" );
    }
    ctxt.register_action( "CHOOSE_LOCATION" );
    ctxt.register_action( "CONFIRM" );

    uilist select_location;
    select_location.text = _( "Select a starting location." );
    int offset = 1;
    const std::string random_start_location_text = string_format( n_gettext(
                "<color_red>* Random location *</color> (<color_white>%d</color> variant)",
                "<color_red>* Random location *</color> (<color_white>%d</color> variants)",
                get_scenario()->start_location_targets_count() ), get_scenario()->start_location_targets_count() );
    uilist_entry entry_random_start_location( RANDOM_START_LOC_ENTRY, true, -1,
            random_start_location_text );
    select_location.entries.emplace_back( entry_random_start_location );
    for( const start_location &loc : start_locations::get_all() ) {
        if( get_scenario()->allowed_start( loc.id ) ) {
            std::string loc_name = loc.name();
            if( loc.targets_count() > 1 ) {
                loc_name = string_format( n_gettext( "%s (<color_white>%d</color> variant)",
                                                     "%s (<color_white>%d</color> variants)", loc.targets_count() ),
                                          loc_name, loc.targets_count() );
            }

            uilist_entry entry( loc.id.id().to_i(), true, -1, loc_name );

            select_location.entries.emplace_back( entry );

            if( !you.random_start_location && loc.id.id() == you.start_location.id() ) {
                select_location.selected = offset;
            }
            offset++;
        }
    }
    if( you.random_start_location ) {
        select_location.selected = 0;
    }
    select_location.setup();
    if( MAP_SHARING::isSharing() ) {
        you.name = MAP_SHARING::getUsername();  // set the current username as default character name
    } else if( !get_option<std::string>( "DEF_CHAR_NAME" ).empty() ) {
        you.name = get_option<std::string>( "DEF_CHAR_NAME" );
    }

    char_creation::description_selector current_selector = char_creation::NAME;

    bool no_name_entered = false;
    ui.on_redraw( [&]( ui_adaptor & ui ) {
        const std::string help_text = assemble_description_help( ctxt, allow_reroll );
        const int new_iHelpHeight = foldstring( help_text, getmaxx( w ) - 4 ).size();
        if( new_iHelpHeight != iHelpHeight ) {
            iHelpHeight = new_iHelpHeight;
            init_windows( ui );
        }
        werase( w );
        tabs.draw( w );
        draw_points( w, pool, you );

        //Draw the line between editable and non-editable stuff.
        wattron( w, BORDER_COLOR );
        mvwaddch( w, point( 0,                9 ), LINE_XXXO ); // |-
        mvwhline( w, point( 1,                9 ), LINE_OXOX, getmaxx( w ) - 2 ); // |
        mvwaddch( w, point( getmaxx( w ) - 1, 9 ), LINE_XOXX );  // -|
        wattroff( w, BORDER_COLOR );
        wnoutrefresh( w );

        werase( w_stats );
        std::vector<std::string> vStatNames;
        vStatNames.reserve( 4 );
        mvwprintz( w_stats, point::zero, COL_HEADER, _( "Stats:" ) );
        vStatNames.emplace_back( _( "Strength:" ) );
        vStatNames.emplace_back( _( "Dexterity:" ) );
        vStatNames.emplace_back( _( "Intelligence:" ) );
        vStatNames.emplace_back( _( "Perception:" ) );
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
        wnoutrefresh( w_stats );

        if( isWide ) {
            werase( w_traits );
            mvwprintz( w_traits, point::zero, COL_HEADER, _( "Traits: " ) );
            std::vector<trait_and_var> current_traits = you.get_mutations_variants();
            std::sort( current_traits.begin(), current_traits.end(), trait_var_display_sort );
            if( current_traits.empty() ) {
                wprintz( w_traits, c_light_red, _( "None!" ) );
            } else {
                for( size_t i = 0; i < current_traits.size(); i++ ) {
                    const trait_and_var current_trait = current_traits[i];
                    trim_and_print( w_traits, point( 0, i + 1 ), getmaxx( w_traits ) - 1,
                                    current_trait.trait->get_display_color(), current_trait.name() );
                }
            }
            wnoutrefresh( w_traits );
        }

        if( isWide ) {
            werase( w_skills );
            mvwprintz( w_skills, point::zero, COL_HEADER, _( "Skills:" ) );

            auto skillslist = Skill::get_skills_sorted_by( [&]( const Skill & a, const Skill & b ) {
                const int level_a = you.get_skill_level_object( a.ident() ).exercised_level();
                const int level_b = you.get_skill_level_object( b.ident() ).exercised_level();
                return localized_compare( std::make_pair( -level_a, a.name() ),
                                          std::make_pair( -level_b, b.name() ) );
            } );

            int line = 1;
            bool has_skills = false;
            profession::StartingSkillList list_skills = you.prof->skills();

            for( const Skill *&elem : skillslist ) {
                int level = you.get_skill_level( elem->ident() );

                // Handle skills from professions
                if( pool != pool_type::TRANSFER ) {
                    profession::StartingSkillList::iterator i = list_skills.begin();
                    while( i != list_skills.end() ) {
                        if( i->first == elem->ident() ) {
                            level += i->second;
                            break;
                        }
                        ++i;
                    }
                }

                // Handle skills from hobbies
                int leftover_exp = 0;
                int exp_to_level = 10000 * ( level + 1 ) * ( level + 1 );
                for( const profession *profession : you.hobbies ) {
                    profession::StartingSkillList hobby_skills = profession->skills();
                    profession::StartingSkillList::iterator i = hobby_skills.begin();
                    while( i != hobby_skills.end() ) {
                        if( i->first == elem->ident() ) {
                            int skill_exp_bonus = leftover_exp + calculate_cumulative_experience( i->second );
                            // Calculate Level up to find final level and remaining exp
                            while( skill_exp_bonus >= exp_to_level ) {
                                level++;
                                skill_exp_bonus -= exp_to_level;
                                exp_to_level = 10000 * ( level + 1 ) * ( level + 1 );
                            }
                            leftover_exp = skill_exp_bonus;
                            break;
                        }
                        ++i;
                    }
                }

                if( level > 0 ) {
                    const int exp_percent = 100 * leftover_exp / exp_to_level;
                    mvwprintz( w_skills, point( 0, line ), c_light_gray,
                               elem->name() + ":" );
                    right_print( w_skills, line, 1, c_light_gray, string_format( "%d(%2d%%)", level, exp_percent ) );
                    line++;
                    has_skills = true;
                }
            }

            if( !has_skills ) {
                mvwprintz( w_skills, point( utf8_width( _( "Skills:" ) ) + 1, 0 ), c_light_red, _( "None!" ) );
            }
            wnoutrefresh( w_skills );
        }

        if( isWide ) {
            werase( w_bionics );
            // Profession bionics description tab, active bionics shown first
            auto prof_CBMs = you.prof->CBMs();
            std::sort( begin( prof_CBMs ), end( prof_CBMs ), []( const bionic_id & a, const bionic_id & b ) {
                return a->activated && !b->activated;
            } );
            mvwprintz( w_bionics, point::zero, COL_HEADER, _( "Bionics:" ) );
            if( prof_CBMs.empty() ) {
                mvwprintz( w_bionics, point( utf8_width( _( "Bionics:" ) ) + 1, 0 ), c_light_red, _( "None!" ) );
            } else {
                for( const auto &b : prof_CBMs ) {
                    const bionic_data &cbm = b.obj();

                    if( cbm.activated && cbm.has_flag( json_flag_BIONIC_TOGGLED ) ) {
                        wprintz( w_bionics, c_light_gray, string_format( _( "\n%s (toggled)" ), cbm.name ) );
                    } else if( cbm.activated ) {
                        wprintz( w_bionics, c_light_gray, string_format( _( "\n%s (activated)" ), cbm.name ) );
                    } else {
                        wprintz( w_bionics, c_light_gray, "\n" + cbm.name );
                    }
                }
            }
            wnoutrefresh( w_bionics );
        }

        // Proficiencies description tab
        if( isWide ) {
            werase( w_proficiencies );
            // Load in proficiencies from profession and hobbies
            std::vector<proficiency_id> prof_proficiencies = you.prof->proficiencies();
            const std::vector<proficiency_id> &known_proficiencies = you._proficiencies->known_profs();
            prof_proficiencies.insert( prof_proficiencies.end(), known_proficiencies.begin(),
                                       known_proficiencies.end() );
            for( const profession *profession : you.hobbies ) {
                for( const proficiency_id &proficiency : profession->proficiencies() ) {
                    // Do not add duplicate proficiencies
                    if( std::find( prof_proficiencies.begin(), prof_proficiencies.end(),
                                   proficiency ) == prof_proficiencies.end() ) {
                        prof_proficiencies.push_back( proficiency );
                    }
                }
            }

            mvwprintz( w_proficiencies, point::zero, COL_HEADER, _( "Proficiencies:" ) );
            if( prof_proficiencies.empty() ) {
                mvwprintz( w_proficiencies, point::south, c_light_red, _( "None!" ) );
            } else {
                wattron( w_proficiencies, c_light_gray );
                for( const proficiency_id &prof : prof_proficiencies ) {
                    wprintw( w_proficiencies, "\n" + trim_by_length( prof->name(), 18 ) );
                }
                wattroff( w_proficiencies, c_light_gray );
            }
            wnoutrefresh( w_proficiencies );
        }

        // Helptext description window
        werase( w_guide );
        fold_and_print( w_guide, point::zero, getmaxx( w_guide ), c_light_gray, help_text );
        wnoutrefresh( w_guide );

        char_creation::draw_name( ui, w_name, you, current_selector == char_creation::NAME,
                                  no_name_entered );
        char_creation::draw_gender( ui, w_gender, you, current_selector == char_creation::GENDER );
        char_creation::draw_outfit( ui, w_outfit, current_selector == char_creation::OUTFIT );
        char_creation::draw_age( ui, w_age, you, current_selector == char_creation::AGE );
        char_creation::draw_height( ui, w_height, you, current_selector == char_creation::HEIGHT );
        char_creation::draw_blood( ui, w_blood, you, current_selector == char_creation::BLOOD );
        char_creation::draw_location( ui, w_location, you, current_selector == char_creation::LOCATION );

        werase( w_vehicle );
        // Player vehicle description tab
        const vproto_id scen_veh = get_scenario()->vehicle();
        const vproto_id prof_veh = you.prof->vehicle();
        if( isWide ) {
            if( scen_veh ) {
                mvwprintz( w_vehicle, point::zero, c_light_gray, _( "Starting vehicle (scenario): " ) );
                wprintz( w_vehicle, c_white, "%s", scen_veh->name );
            } else if( prof_veh ) {
                mvwprintz( w_vehicle, point::zero, c_light_gray, _( "Starting vehicle (profession): " ) );
                wprintz( w_vehicle, c_white, "%s", prof_veh->name );
            } else {
                mvwprintz( w_vehicle, point::zero, c_light_gray, _( "Starting vehicle: " ) );
                wprintz( w_vehicle, c_light_red, _( "None!" ) );
            }
        } else {
            if( scen_veh ) {
                mvwprintz( w_vehicle, point::zero, COL_HEADER, _( "Starting vehicle (scenario): " ) );
                wprintz( w_vehicle, c_light_gray, "\n%s", scen_veh->name );
            } else if( prof_veh ) {
                mvwprintz( w_vehicle, point::zero, COL_HEADER, _( "Starting vehicle (profession): " ) );
                wprintz( w_vehicle, c_light_gray, "\n%s", prof_veh->name );
            } else {
                mvwprintz( w_vehicle, point::zero, COL_HEADER, _( "Starting vehicle: " ) );
                wprintz( w_vehicle, c_light_red, _( "None!" ) );
            }
        }
        wnoutrefresh( w_vehicle );

        werase( w_addictions );
        // Profession addictions description tab
        std::vector<addiction> prof_addictions = you.prof->addictions();
        for( const profession *profession : you.hobbies ) {
            const std::vector<addiction> &hobby_addictions = profession->addictions();
            for( const addiction &iter : hobby_addictions ) {
                prof_addictions.push_back( iter );
            }
        }
        if( isWide ) {
            mvwprintz( w_addictions, point::zero, c_light_gray, _( "Starting addictions: " ) );
            if( prof_addictions.empty() ) {
                wprintz( w_addictions, c_light_red, _( "None!" ) );
            } else {
                for( const addiction &a : prof_addictions ) {
                    const char *format = "%1$s (%2$d) ";
                    wprintz( w_addictions, c_white, string_format( format, a.type->get_name().translated(),
                             a.intensity ) );
                }
            }
        } else {
            mvwprintz( w_addictions, point::zero, COL_HEADER, _( "Starting addictions: " ) );
            if( prof_addictions.empty() ) {
                wprintz( w_addictions, c_light_red, _( "None!" ) );
            } else {
                for( const addiction &a : prof_addictions ) {
                    const char *format = "%1$s (%2$d) ";
                    wprintz( w_addictions, c_light_gray, "\n" );
                    wprintz( w_addictions, c_light_gray, string_format( format, a.type->get_name().translated(),
                             a.intensity ) );
                }
            }
        }
        wnoutrefresh( w_addictions );

        werase( w_scenario );
        mvwprintz( w_scenario, point::zero, COL_HEADER, _( "Scenario: " ) );
        wprintz( w_scenario, c_light_gray, get_scenario()->gender_appropriate_name( you.male ) );
        wnoutrefresh( w_scenario );

        werase( w_profession );
        mvwprintz( w_profession, point::zero, COL_HEADER, _( "Profession: " ) );
        wprintz( w_profession, c_light_gray, you.prof->gender_appropriate_name( you.male ) );
        wnoutrefresh( w_profession );

        werase( w_calendar );
        mvwprintz( w_calendar, point::zero, COL_HEADER, _( "Start of cataclysm:" ) );
        wprintz( w_calendar, c_light_gray, "\n" );
        wprintz( w_calendar, c_light_gray, to_string( get_scenario()->start_of_cataclysm() ) );
        wprintz( w_calendar, c_light_gray, "\n" );
        wprintz( w_calendar, COL_HEADER, _( "Start of game:" ) );
        wprintz( w_calendar, c_light_gray, "\n" );
        wprintz( w_calendar, c_light_gray, to_string( get_scenario()->start_of_game() ) );
        wnoutrefresh( w_calendar );

        if( isWide ) {
            werase( w_hobbies );
            mvwprintz( w_hobbies, point::zero, COL_HEADER, _( "Background: " ) );
            if( you.hobbies.empty() ) {
                mvwprintz( w_hobbies, point::south, c_light_red, _( "None!" ) );
            } else {
                wattron( w_hobbies, c_light_gray );
                for( const profession *prof : you.hobbies ) {
                    wprintw( w_hobbies, "\n%s", prof->gender_appropriate_name( you.male ) );
                }
                wattroff( w_hobbies, c_light_gray );
            }
            wnoutrefresh( w_hobbies );
        }
    } );

    int min_allowed_age = 16;
    int max_allowed_age = 55;

    int min_allowed_height = Character::min_height();
    int max_allowed_height = Character::max_height();

    do {
        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        if( action == "NEXT_TAB" ) {
            if( pool == pool_type::ONE_POOL ) {
                if( points_used_total( you ) > point_pool_total() ) {
                    popup( _( "Too many points allocated, change some features and try again." ) );
                    continue;
                }
            } else if( pool == pool_type::MULTI_POOL ) {
                multi_pool p( you );
                if( p.skill_points_left < 0 ) {
                    popup( _( "Too many points allocated, change some features and try again." ) );
                    continue;
                }
                if( p.trait_points_left < 0 ) {
                    popup( _( "Too many trait points allocated, change some traits or lower some stats and try again." ) );
                    continue;
                }
                if( p.stat_points_left < 0 ) {
                    popup( _( "Too many stat points allocated, lower some stats and try again." ) );
                    continue;
                }
            }
            if( has_unspent_points( you ) && pool != pool_type::FREEFORM &&
                !query_yn( _( "Remaining points will be discarded, are you sure you want to proceed?" ) ) ) {
                continue;
            }
            if( you.name.empty() ) {
                no_name_entered = true;
                ui_manager::redraw();
                if( !query_yn( _( "Are you SURE you're finished?  Your name will be randomly generated." ) ) ) {
                    continue;
                } else {
                    you.pick_name();
                    tabs.complete = true;
                    break;
                }
            }
            if( query_yn( _( "Are you SURE you're finished?" ) ) ) {
                tabs.complete = true;
                break;
            }
            continue;
        } else if( tabs.handle_input( action, ctxt ) ) {
            break; // Tab has changed or user has quit the screen
        } else if( action == "DOWN" ) {
            switch( current_selector ) {
                case char_creation::NAME:
                    current_selector = char_creation::GENDER;
                    break;
                case char_creation::GENDER:
                    current_selector = char_creation::OUTFIT;
                    break;
                case char_creation::OUTFIT:
                    current_selector = char_creation::HEIGHT;
                    break;
                case char_creation::HEIGHT:
                    current_selector = char_creation::AGE;
                    break;
                case char_creation::AGE:
                    current_selector = char_creation::BLOOD;
                    break;
                case char_creation::BLOOD:
                    current_selector = char_creation::LOCATION;
                    break;
                case char_creation::LOCATION:
                    current_selector = char_creation::NAME;
                    break;
            }
        } else if( action == "UP" ) {
            switch( current_selector ) {
                case char_creation::NAME:
                    current_selector = char_creation::LOCATION;
                    break;
                case char_creation::LOCATION:
                    current_selector = char_creation::BLOOD;
                    break;
                case char_creation::BLOOD:
                    current_selector = char_creation::AGE;
                    break;
                case char_creation::AGE:
                    current_selector = char_creation::HEIGHT;
                    break;
                case char_creation::HEIGHT:
                    current_selector = char_creation::OUTFIT;
                    break;
                case char_creation::OUTFIT:
                    current_selector = char_creation::GENDER;
                    break;
                case char_creation::GENDER:
                    current_selector = char_creation::NAME;
                    break;
            }
        } else if( action == "RIGHT" ) {
            switch( current_selector ) {
                case char_creation::HEIGHT:
                    if( you.base_height() < max_allowed_height ) {
                        you.mod_base_height( 1 );
                        you.set_stored_kcal( you.get_healthy_kcal() );
                    }
                    break;
                case char_creation::AGE:
                    if( you.base_age() < max_allowed_age ) {
                        you.mod_base_age( 1 );
                    }
                    break;
                case char_creation::BLOOD:
                    if( !you.blood_rh_factor ) {
                        you.blood_rh_factor = true;
                        break;
                    }
                    if( static_cast<blood_type>( static_cast<int>( you.my_blood_type ) + 1 ) != blood_type::num_bt ) {
                        you.my_blood_type = static_cast<blood_type>( static_cast<int>( you.my_blood_type ) + 1 );
                        you.blood_rh_factor = false;
                    } else {
                        you.my_blood_type = static_cast<blood_type>( 0 );
                        you.blood_rh_factor = false;
                    }
                    break;
                case char_creation::GENDER:
                    you.male = !you.male;
                    break;
                case char_creation::OUTFIT:
                    outfit = !outfit;
                    break;
                default:
                    break;
            }
        } else if( action == "LEFT" ) {
            switch( current_selector ) {
                case char_creation::HEIGHT:
                    if( you.base_height() > min_allowed_height ) {
                        you.mod_base_height( -1 );
                        you.set_stored_kcal( you.get_healthy_kcal() );
                    }
                    break;
                case char_creation::AGE:
                    if( you.base_age() > min_allowed_age ) {
                        you.mod_base_age( -1 );
                    }
                    break;
                case char_creation::BLOOD:
                    if( you.blood_rh_factor ) {
                        you.blood_rh_factor = false;
                        break;
                    }
                    if( you.my_blood_type != static_cast<blood_type>( 0 ) ) {
                        you.my_blood_type = static_cast<blood_type>( static_cast<int>( you.my_blood_type ) - 1 );
                        you.blood_rh_factor = true;
                    } else {
                        you.my_blood_type = static_cast<blood_type>( static_cast<int>( blood_type::num_bt ) - 1 );
                        you.blood_rh_factor = true;
                    }
                    break;
                case char_creation::GENDER:
                    you.male = !you.male;
                    break;
                case char_creation::OUTFIT:
                    outfit = !outfit;
                    break;
                default:
                    break;
            }
        } else if( action == "REROLL_CHARACTER" && allow_reroll ) {
            you.randomize( false );
            // Re-enter this tab again, but it forces a complete redrawing of it.
            break;
        } else if( action == "REROLL_CHARACTER_WITH_SCENARIO" && allow_reroll ) {
            you.randomize( true );
            // Re-enter this tab again, but it forces a complete redrawing of it.
            break;
        } else if( action == "SAVE_TEMPLATE" ) {
            if( const auto name = query_for_template_name() ) {
                you.save_template( *name, pool );
            }
        } else if( action == "RANDOMIZE_CHAR_NAME" ) {
            if( !MAP_SHARING::isSharing() ) { // Don't allow random names when sharing maps. We don't need to check at the top as you won't be able to edit the name
                you.pick_name();
                no_name_entered = you.name.empty();
            }
        } else if( action == "RANDOMIZE_CHAR_DESCRIPTION" ) {
            bool gender_selection = one_in( 2 );
            you.male = gender_selection;
            if( one_in( 10 ) ) {
                outfit = !gender_selection;
            } else {
                outfit = gender_selection;
            }
            if( !MAP_SHARING::isSharing() ) { // Don't allow random names when sharing maps. We don't need to check at the top as you won't be able to edit the name
                you.pick_name();
                no_name_entered = you.name.empty();
            }
            you.set_base_age( rng( 16, 55 ) );
            you.randomize_height();
            you.randomize_blood();
            you.randomize_heartrate();
        } else if( action == "CHANGE_OUTFIT" ) {
            outfit = !outfit;
        } else if( action == "CHANGE_GENDER" ) {
            you.male = !you.male;
        } else if( action == "CHANGE_START_OF_CATACLYSM" ) {
            const scenario *scen = get_scenario();
            scen->change_start_of_cataclysm( calendar_ui::select_time_point( scen->start_of_cataclysm(),
                                             _( "Select cataclysm start date" ), calendar_ui::granularity::hour ) );
        } else if( action == "CHANGE_START_OF_GAME" ) {
            const scenario *scen = get_scenario();
            scen->change_start_of_game( calendar_ui::select_time_point( scen->start_of_game(),
                                        _( "Select game start date" ), calendar_ui::granularity::hour ) );
        } else if( action == "RESET_CALENDAR" ) {
            get_scenario()->reset_calendar();
        } else if( action == "CHOOSE_CITY" ) {
            std::vector<city> cities( city::get_all() );
            const auto cities_cmp_population = []( const city & a, const city & b ) {
                return std::tie( a.population, a.name ) > std::tie( b.population, b.name );
            };
            std::sort( cities.begin(), cities.end(), cities_cmp_population );
            uilist cities_menu;
            ui::omap::setup_cities_menu( cities_menu, cities );
            std::optional<city> c = ui::omap::select_city( cities_menu, cities, false );
            if( c.has_value() ) {
                you.starting_city = c;
                you.world_origin = c->pos_om;
            }
        } else if( action == "CHOOSE_LOCATION" ) {
            select_location.query();
            if( select_location.ret == RANDOM_START_LOC_ENTRY ) {
                you.random_start_location = true;
            } else if( select_location.ret >= 0 ) {
                for( const start_location &loc : start_locations::get_all() ) {
                    if( loc.id.id().to_i() == select_location.ret ) {
                        you.random_start_location = false;
                        you.start_location = loc.id;
                        break;
                    }
                }
            }
        } else if( action == "CONFIRM" &&
                   // Don't edit names when sharing maps
                   !MAP_SHARING::isSharing() ) {

            string_input_popup popup;
            switch( current_selector ) {
                case char_creation::NAME: {
                    popup.title( _( "Enter name.  Cancel to delete all." ) )
                    .text( you.name )
                    .only_digits( false );
                    you.name = popup.query_string();
                    no_name_entered = you.name.empty();
                    break;
                }
                case char_creation::AGE: {
                    int result = you.base_age();
                    if( query_int( result, false, _( "Enter age in years.  Minimum 16, maximum 55" ) ) && result > 0 ) {
                        you.set_base_age( clamp( result, 16, 55 ) );
                    }
                    break;
                }
                case char_creation::HEIGHT: {
                    int result = you.base_height();
                    if( query_int( result, false, _( "Enter height in centimeters.  Minimum %d, maximum %d" ),
                                   min_allowed_height, max_allowed_height ) && result > 0 ) {
                        you.set_base_height( clamp( result, min_allowed_height, max_allowed_height ) );
                    }
                    break;
                }
                case char_creation::BLOOD: {
                    uilist btype;
                    btype.text = _( "Select blood type" );
                    btype.addentry( static_cast<int>( blood_type::blood_O ), true, '1', "O" );
                    btype.addentry( static_cast<int>( blood_type::blood_A ), true, '2', "A" );
                    btype.addentry( static_cast<int>( blood_type::blood_B ), true, '3', "B" );
                    btype.addentry( static_cast<int>( blood_type::blood_AB ), true, '4', "AB" );
                    btype.query();
                    if( btype.ret < 0 ) {
                        break;
                    }

                    uilist bfac;
                    bfac.text = _( "Select Rh factor" );
                    bfac.addentry( 0, true, '-', _( "negative" ) );
                    bfac.addentry( 1, true, '+', _( "positive" ) );
                    bfac.query();
                    if( bfac.ret < 0 ) {
                        break;
                    }
                    you.my_blood_type = static_cast<blood_type>( btype.ret );
                    you.blood_rh_factor = static_cast<bool>( bfac.ret );
                    break;
                }
                case char_creation::GENDER: {
                    uilist gselect;
                    gselect.text = _( "Select gender" );
                    gselect.addentry( 0, true, '1', _( "Female" ) );
                    gselect.addentry( 1, true, '2', _( "Male" ) );
                    gselect.query();
                    if( gselect.ret < 0 ) {
                        break;
                    }
                    you.male = static_cast<bool>( gselect.ret );
                    break;
                }
                case char_creation::OUTFIT: {
                    uilist gselect;
                    gselect.text = _( "Select outfit" );
                    gselect.addentry( 0, true, '1', _( "Female" ) );
                    gselect.addentry( 1, true, '2', _( "Male" ) );
                    gselect.query();
                    if( gselect.ret < 0 ) {
                        break;
                    }
                    outfit = static_cast<bool>( gselect.ret );
                    break;
                }
                case char_creation::LOCATION: {
                    select_location.query();
                    if( select_location.ret == RANDOM_START_LOC_ENTRY ) {
                        you.random_start_location = true;
                    } else if( select_location.ret >= 0 ) {
                        for( const start_location &loc : start_locations::get_all() ) {
                            if( loc.id.id().to_i() == select_location.ret ) {
                                you.random_start_location = false;
                                you.start_location = loc.id;
                                break;
                            }
                        }
                    }
                    break;
                }
            }
        }
    } while( true );
}

std::vector<trait_id> Character::get_base_traits() const
{
    return std::vector<trait_id>( my_traits.begin(), my_traits.end() );
}

std::vector<trait_id> Character::get_mutations( bool include_hidden,
        bool ignore_enchantments, const std::function<bool( const mutation_branch & )> &filter ) const
{
    std::vector<trait_id> result;
    result.reserve( my_mutations.size() + old_mutation_cache->get_mutations().size() );
    for( const std::pair<const trait_id, trait_data> &t : my_mutations ) {
        const mutation_branch &mut = t.first.obj();
        if( include_hidden || mut.player_display ) {
            if( filter == nullptr || filter( mut ) ) {
                result.push_back( t.first );
            }
        }
    }
    if( !ignore_enchantments ) {
        for( const trait_id &ench_trait : old_mutation_cache->get_mutations() ) {
            if( include_hidden || ench_trait->player_display ) {
                bool found = false;
                for( const trait_id &exist : result ) {
                    if( exist == ench_trait ) {
                        found = true;
                        break;
                    }
                }
                if( !found ) {
                    if( filter == nullptr || filter( ench_trait.obj() ) ) {
                        result.push_back( ench_trait );
                    }
                }
            }
        }
    }
    return result;
}

std::vector<trait_id> Character::get_functioning_mutations( bool include_hidden,
        bool ignore_enchantments, const std::function<bool( const mutation_branch & )> &filter ) const
{
    std::vector<trait_id> result;
    const auto &test = ignore_enchantments ? my_mutations : cached_mutations;
    result.reserve( test.size() );
    for( const std::pair<const trait_id, trait_data> &t : test ) {
        if( t.second.corrupted == 0 ) {
            const mutation_branch &mut = t.first.obj();
            if( include_hidden || mut.player_display ) {
                if( filter == nullptr || filter( mut ) ) {
                    result.push_back( t.first );
                }
            }
        }
    }
    return result;
}

std::vector<trait_and_var> Character::get_mutations_variants( bool include_hidden,
        bool ignore_enchantments ) const
{
    std::vector<trait_and_var> result;
    const auto &test = ignore_enchantments ? my_mutations : cached_mutations;
    result.reserve( test.size() );
    for( const std::pair<const trait_id, trait_data> &t : test ) {
        if( t.second.corrupted == 0 ) {
            if( include_hidden || t.first.obj().player_display ) {
                const std::string &variant = t.second.variant != nullptr ? t.second.variant->id : "";
                result.emplace_back( t.first, variant );
            }
        }
    }
    return result;
}

void Character::clear_mutations()
{
    while( !my_traits.empty() ) {
        my_traits.erase( *my_traits.begin() );
    }
    while( !my_mutations.empty() ) {
        my_mutations.erase( my_mutations.begin() );
    }
    while( !my_mutations_dirty.empty() ) {
        my_mutations_dirty.erase( my_mutations_dirty.begin() );
    }
    while( !cached_mutations.empty() ) {
        const trait_id trait = cached_mutations.begin()->first;
        cached_mutations.erase( cached_mutations.begin() );
        mutation_loss_effect( trait );
    }
    old_mutation_cache->clear();
    recalc_sight_limits();
    calc_encumbrance();
}

void Character::empty_skills()
{
    for( auto &sk : *_skills ) {
        sk.second = SkillLevel();
    }
}

void Character::add_traits()
{
    // TODO: get rid of using get_avatar() here, use `this` instead
    for( const trait_and_var &tr : get_avatar().prof->get_locked_traits() ) {
        if( !has_trait( tr.trait ) ) {
            toggle_trait_deps( tr.trait );
        }
    }
    for( const trait_id &tr : get_scenario()->get_locked_traits() ) {
        if( !has_trait( tr ) ) {
            toggle_trait_deps( tr );
        }
    }
}

trait_id Character::random_good_trait()
{
    return get_random_trait( []( const mutation_branch & mb ) {
        return mb.points > 0 && mb.random_at_chargen;
    } );
}

trait_id Character::random_bad_trait()
{
    return get_random_trait( []( const mutation_branch & mb ) {
        return mb.points < 0 && mb.random_at_chargen;
    } );
}

trait_id Character::get_random_trait( const std::function<bool( const mutation_branch & )> &func )
{
    std::vector<trait_id> vTraits;

    for( const mutation_branch &traits_iter : mutation_branch::get_all() ) {
        if( func( traits_iter ) && get_scenario()->traitquery( traits_iter.id ) ) {
            vTraits.push_back( traits_iter.id );
        }
    }

    return random_entry( vTraits );
}

void Character::randomize_cosmetic_trait( const std::string &mutation_type )
{
    trait_id trait = get_random_trait( [mutation_type]( const mutation_branch & mb ) {
        return mb.points == 0 && mb.types.count( mutation_type );
    } );

    if( !has_conflicting_trait( trait ) ) {
        toggle_trait( trait );
    }
}

std::optional<std::string> query_for_template_name()
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
        spop.add_callback( character, []() {
            return true;
        } );
    }

    spop.query_string( true );
    if( spop.canceled() ) {
        return std::nullopt;
    } else {
        return spop.text();
    }
}

void avatar::character_to_template( const std::string &name )
{
    save_template( name, pool_type::TRANSFER );
}

void Character::add_default_background()
{
    for( const profession_group &prof_grp : profession_group::get_all() ) {
        if( prof_grp.get_id() == profession_group_adult_basic_background ) {
            for( const profession_id &hobb : prof_grp.get_professions() ) {
                hobbies.insert( &hobb.obj() );
            }
        }
    }
}

void avatar::save_template( const std::string &name, pool_type pool )
{
    write_to_file( PATH_INFO::templatedir() + name + ".template", [&]( std::ostream & fout ) {
        JsonOut jsout( fout, true );

        jsout.start_array();

        jsout.start_object();
        jsout.member( "limit", pool );
        jsout.member( "random_start_location", random_start_location );
        if( !random_start_location ) {
            jsout.member( "start_location", start_location );
        }
        jsout.member( "outfit_gender", outfit );
        jsout.end_object();

        serialize( jsout );

        jsout.end_array();
    }, _( "player template" ) );
}

bool avatar::load_template( const std::string &template_name, pool_type &pool )
{
    return read_from_file_json( PATH_INFO::templatedir_path() / ( template_name + ".template" ), [&](
    const JsonValue & jv ) {
        // Legacy templates are an object. Current templates are an array of 0, 1, or 2 objects.
        JsonObject legacy_template;
        if( jv.test_array() ) {
            // not a legacy template
            JsonArray template_array = jv;
            if( template_array.empty() ) {
                return;
            }

            JsonObject jobj = template_array.get_object( 0 );

            jobj.get_int( "stat_points", 0 );
            jobj.get_int( "trait_points", 0 );
            jobj.get_int( "skill_points", 0 );

            pool = static_cast<pool_type>( jobj.get_int( "limit" ) );

            random_start_location = jobj.get_bool( "random_start_location", true );
            const std::string jobj_start_location = jobj.get_string( "start_location", "" );

            outfit = jobj.get_bool( "outfit_gender", true );

            // get_scenario()->allowed_start( loc.ident() ) is checked once scenario loads in avatar::load()
            for( const class start_location &loc : start_locations::get_all() ) {
                if( loc.id.str() == jobj_start_location ) {
                    random_start_location = false;
                    this->start_location = loc.id;
                    break;
                }
            }

            if( template_array.size() == 1 ) {
                return;
            }
            legacy_template = template_array.get_object( 1 );
        }

        deserialize( legacy_template );

        // If stored_calories the template is under a million (kcals < 1000), assume it predates the
        // kilocalorie-to-literal-calorie conversion and is off by a factor of 1000.
        if( get_stored_kcal() < 1000 ) {
            set_stored_kcal( 1000 * get_stored_kcal() );
        }

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

    u.random_start_location = true;
    u.str_max = 8;
    u.dex_max = 8;
    u.int_max = 8;
    u.per_max = 8;
    set_scenario( scen );
    u.prof = &default_prof.obj();

    u.hobbies.clear();
    u.add_default_background();
    u.clear_mutations();
    u.recalc_hp();
    u.empty_skills();
    u.add_traits();
}
